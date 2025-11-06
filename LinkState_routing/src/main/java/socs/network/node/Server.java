package socs.network.node;

import java.io.*; 
import java.net.*;

import socs.network.message.SOSPFPacket;
import socs.network.message.LSA;
import socs.network.message.LinkDescription; 

public class Server implements Runnable{
    ServerSocket server;
    Router router;

    public Server(Router router, int port){
        this.router = router;

        try {
            server = new ServerSocket(port);
        }
        catch (Exception e){
            e.printStackTrace();
        }
    }

    // server method that listens to incoming requests
    // different communication threads are used for communicating with connecting clients.
    public void run(){
        try{
            while (true){
                Socket clientSocket = this.server.accept();

                // creating a new listening client handler
                ListeningHandler listener = new ListeningHandler(clientSocket);

                // start thread
                Thread handlerThread = new Thread(listener);
                handlerThread.start();

            }
        }
        catch (IOException e){
            e.printStackTrace();
        }
    }

    class ListeningHandler implements Runnable {

        private Socket clientSocket; 
        ObjectOutputStream out = null; 
        ObjectInputStream in = null;
        
        public ListeningHandler(Socket clienSocket){
            this.clientSocket = clienSocket;
        }

        @Override
        public void run() {            
            try {         
                // get the outputstream of clientSocket 
                out = new ObjectOutputStream(this.clientSocket.getOutputStream()); 

                // get the inputstream of clientSocket 
                in = new ObjectInputStream(this.clientSocket.getInputStream()); 
                
                while (true) {
                    // handle message received
                    SOSPFPacket message = (SOSPFPacket) in.readObject();
                    
                    if (message == null){
                        System.out.println("Received empty message!");
                        break;
                    }
        
                    // parse the message
                    parseMessage(message);
                }
                
            } catch (IOException e) { 
                e.printStackTrace(); 
            } catch (Exception e){
                e.printStackTrace();
            } 
        }

        public void parseMessage(SOSPFPacket msg){
            
            RouterDescription remoteRD = null;

            // find matching remote Router amongst current Router's Links
            for (Link l: router.ports){
                if (l.router2.simulatedIPAddress.equals(msg.routerID)){
                    remoteRD = l.router2;
                    break;
                }
            }

            // parse HELLO message
            if (msg.sospfType == 0){
                
                System.out.println("Received HELLO from " + msg.routerID);
                
                // set status of router to INIT
                remoteRD.status = RouterStatus.INIT;
                System.out.println("set " + msg.routerID + " state to INIT");

                // send HELLO-ACK message to remote router
                SOSPFPacket helloACK = new SOSPFPacket();
                helloACK.sospfType = 1;
                helloACK.dstIP = msg.srcIP;
                helloACK.srcIP = router.rd.simulatedIPAddress;
                helloACK.srcProcessIP = router.rd.processIPAddress;
                helloACK.srcProcessPort = router.rd.processPortNumber;
                helloACK.routerID = router.rd.simulatedIPAddress;
                helloACK.neighborID = msg.routerID;
                
                try {
                    out.writeObject(helloACK);
                } catch (IOException e) {
                    e.printStackTrace();
                }
                
                SOSPFPacket response = null;
        
                try {
                    response = (SOSPFPacket) in.readObject();
                } catch (Exception e) {
                    e.printStackTrace();
                }
                
                if (response.sospfType == 2){
                    System.out.println("Received HELLO-TWO_WAY from " + response.routerID);
                    // set status of remote to TWO_WAY
                    remoteRD.status = RouterStatus.TWO_WAY;
                    System.out.println("set " + response.routerID + " state to TWO_WAY");
                }
                return;
            }

            // parse ATTACH request message
            if (msg.sospfType == 4){
                synchronized(router.ports){
                    System.out.println("\nReceived ATTACH request from " + msg.routerID);
                    if (router.ports.size() < 4){
                        System.out.println("Accepted ATTACH request from " + msg.routerID);
                        // send ATTACH-ACCEPT message and create new Link
                        SOSPFPacket attachAccept = new SOSPFPacket();
                        attachAccept.srcProcessIP = router.rd.processIPAddress;
                        attachAccept.srcProcessPort = router.rd.processPortNumber;
                        attachAccept.srcIP = router.rd.simulatedIPAddress;
                        attachAccept.dstIP = msg.srcIP;
                        attachAccept.routerID = router.rd.simulatedIPAddress;
                        attachAccept.neighborID = msg.routerID;
                        attachAccept.sospfType = 5; // ATTACH-ACK
                        
                        try {
                            out.writeObject(attachAccept);
                        } catch (IOException e) {
                            e.printStackTrace();
                        }
                        
                        // create RouterDescription for remote router to add Link for current Router
                        RouterDescription r2 = new RouterDescription();
                        r2.processIPAddress = msg.srcProcessIP;
                        r2.processPortNumber = msg.srcProcessPort;
                        r2.simulatedIPAddress = msg.routerID;
                        r2.status = RouterStatus.IDLE;
    
                        Link link = new Link(router.rd, r2);
                        router.ports.add(link);
                    }
                    else {
                        System.out.println("Rejected ATTACH request from " + msg.routerID);
                        // send ATTACH-REJECT message
                        SOSPFPacket attachReject = new SOSPFPacket();
                        attachReject.srcProcessIP = router.rd.processIPAddress;
                        attachReject.srcProcessPort = router.rd.processPortNumber;
                        attachReject.srcIP = router.rd.simulatedIPAddress;
                        attachReject.dstIP = msg.srcIP;
                        attachReject.sospfType = 6; // ATTACH-REJECT
    
                        try {
                            out.writeObject(attachReject);
                        } catch (IOException e) {
                            e.printStackTrace();
                        }
                    }
                }
            }
            
            // Parsing of DISCONNECT message
            if (msg.sospfType == 7){
                synchronized(router.ports){
                    for (Link l : router.ports){
                        if (l.router2.simulatedIPAddress.equals(msg.routerID)){
                            router.ports.remove(l);
                            break;
                        }
                    }
                }
                // update the LSD
                LSA lsa = new LSA();
                lsa.linkStateID = router.rd.simulatedIPAddress;
                
                synchronized(router.ports){
                    for (Link l : router.ports){  
                        LinkDescription ld = new LinkDescription();
                        ld.linkID = l.router2.simulatedIPAddress;
                        ld.portNum = router.ports.indexOf(l);
                        lsa.links.add(ld);
                    }
                    
                    synchronized(router.lsd){
                        // updating its own LSD entry
                        router.lsd.addLSA(router.rd.simulatedIPAddress, lsa);

                        // removing entry from disconnected router
                        router.lsd._store.remove(msg.routerID);
                    }
                }
                
                router.broadcastLSA();
            }

            // Parsing of LSAUPDATE message
            if (msg.sospfType == 3){
                System.out.println("Received LSAUpdate from " + msg.routerID);
                for (LSA lsa : msg.lsaArray){
                    // check if this LSA is already in LSD and has same sequence number, if so ignore
                    synchronized (router.lsd){
                        if (router.lsd._store.containsKey(lsa.linkStateID) && router.lsd.getLSA(lsa.linkStateID).lsaSeqNumber >= lsa.lsaSeqNumber){
                            continue;
                        }

                        router.lsd.addLSA(lsa.linkStateID, lsa);
                    }
                }

                // append local LSA to lsaarray and broadcast to all neighbors except neighbor it received it from
                synchronized(router.ports){
                    synchronized(router.lsd){
                        msg.lsaArray.add(router.lsd.getLSA(router.rd.simulatedIPAddress));
                    }
                    
                    for (Link link : router.ports){

                        // first check if connection has been established and is TWO-WAY
                        if (link.router2.status != RouterStatus.TWO_WAY){
                            continue;
                        }

                        // check if neighbor is originator of LSA packet, skip
                        if (msg.routerID.equals(link.router2.simulatedIPAddress)){
                            continue;
                        }

                        msg.routerID = router.rd.simulatedIPAddress;
                        msg.neighborID = link.router2.simulatedIPAddress;

                        link.client.send(msg);
                    }
                }

            }
        }

    }
}
