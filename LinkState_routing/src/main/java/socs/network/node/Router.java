package socs.network.node;

import socs.network.message.LSA;
import socs.network.message.LinkDescription;
import socs.network.message.SOSPFPacket;
import socs.network.util.Configuration;

import java.io.BufferedReader;
import java.io.InputStreamReader;
import java.util.ArrayList;


public class Router {

  protected LinkStateDatabase lsd;

  RouterDescription rd = new RouterDescription();

  //assuming that all routers are with 4 ports
  ArrayList<Link> ports = new ArrayList<>();
  Server server;

  public Router(Configuration config) {
    rd.simulatedIPAddress = config.getString("socs.network.router.ip");
    rd.processIPAddress = config.getString("socs.network.router.processIP");
    rd.processPortNumber = config.getShort("socs.network.router.port");

    lsd = new LinkStateDatabase(rd);
    server = new Server(this, rd.processPortNumber);

    // start server thread
    new Thread(server).start();
}

  public void broadcastLSA(){
    
    for (Link l: ports){

      // first check if connection has been established and is TWO-WAY
      if (l.router2.status != RouterStatus.TWO_WAY){
        continue;
      }

      SOSPFPacket lsa = new SOSPFPacket();

      lsa.srcProcessIP = rd.processIPAddress;
      lsa.srcProcessPort = rd.processPortNumber;
      lsa.srcIP = rd.simulatedIPAddress;
      lsa.dstIP = l.router2.simulatedIPAddress;
      lsa.sospfType = 3;
      lsa.routerID = rd.simulatedIPAddress;
      lsa.neighborID = l.router2.simulatedIPAddress;

      synchronized(lsd){
        // add current Router's most recent LSA from LSD
        lsa.lsaArray.add(lsd.getLSA(rd.simulatedIPAddress));
      }
      
      l.client.send(lsa);
    }
  }
  /**
   * output the shortest path to the given destination ip
   * <p/>
   * format: source ip address  -> ip address -> ... -> destination ip
   *
   * @param destinationIP the ip adderss of the destination simulated router
   */
  private void processDetect(String destinationIP) {
      String path = lsd.getShortestPath(destinationIP);

      System.out.println(path);
  }

  /**
   * disconnect with the router identified by the given destination ip address
   * Notice: this command should trigger the synchronization of database
   *
   * @param portNumber the port number which the link attaches at
   */
  private void processDisconnect(short portNumber) {
        
      synchronized(ports){
          //get the rounter we want to disconnect from. 
          Link link = ports.get(portNumber);
          // send diconnect notification
          SOSPFPacket disconnect = new SOSPFPacket();

          disconnect.srcProcessIP = rd.processIPAddress;
          disconnect.srcProcessPort = rd.processPortNumber;
          disconnect.srcIP = rd.simulatedIPAddress;
          disconnect.dstIP = link.router2.simulatedIPAddress;
          disconnect.sospfType = 7;
          disconnect.routerID = rd.simulatedIPAddress;
          disconnect.neighborID = link.router2.simulatedIPAddress;

          link.client.send(disconnect);

          this.ports.remove(portNumber);

          // update the LSD
          LSA lsa = new LSA();
          lsa.linkStateID = rd.simulatedIPAddress;

          for (Link l : ports){  
            LinkDescription ld = new LinkDescription();
            ld.linkID = l.router2.simulatedIPAddress;
            ld.portNum = ports.indexOf(l);
            lsa.links.add(ld);
          }
          
          synchronized(lsd){
            // updating its own LSD entry
            lsd.addLSA(rd.simulatedIPAddress, lsa);
          }
      }
      broadcastLSA();
  }

  /**
   * attach the link to the remote router, which is identified by the given simulated ip;
   * to establish the connection via socket, you need to indentify the process IP and process Port;
   * <p/>
   * NOTE: this command should not trigger link database synchronization
   */
  private void processAttach(String processIP, short processPort, String simulatedIP) {
      
    // if all ports in use, abort
    if (ports.size() >= 4){
      System.out.println("Max number of ports in use reached. Cannot create new Link.");
      return;
    }

    // create RouterDescription for remote router to add Link for current Router
    RouterDescription r2 = new RouterDescription();
    r2.processIPAddress = processIP;
    r2.processPortNumber = processPort;
    r2.simulatedIPAddress = simulatedIP;
    r2.status = RouterStatus.IDLE;

    Link link = new Link(rd, r2);

    // send ATTACH request message and wait for response
    SOSPFPacket attach = new SOSPFPacket();
    attach.srcProcessIP = rd.processIPAddress;
    attach.srcProcessPort = rd.processPortNumber;
    attach.srcIP = rd.simulatedIPAddress;
    attach.dstIP = simulatedIP;
    attach.sospfType = 4; // ATTACH
    attach.routerID = rd.simulatedIPAddress;
    attach.neighborID = simulatedIP;

    Client linkClient = link.client;
    linkClient.send(attach);
    
    SOSPFPacket attachResponse = link.client.read();

    if (attachResponse.sospfType == 5 && attachResponse.routerID.equals(simulatedIP)){
      System.out.println("Your ATTACH request has been accepted by " + attachResponse.srcIP);
      
      synchronized(ports){
        ports.add(link);
      }
      return;
    }
    
    System.out.println("Your ATTACH request has been rejected by " + attachResponse.srcIP);
  }


  /**
   * process request from the remote router. 
   * For example: when router2 tries to attach router1. Router1 can decide whether it will accept this request. 
   * The intuition is that if router2 is an unknown/anomaly router, it is always safe to reject the attached request from router2.
   */
  // private void requestHandler() {

  // }

  /**
   * broadcast Hello to neighbors
   */
  private void processStart() {
    // broadcast HELLO message to all Links on all ports
    for (Link l: ports){

      // check if status TWO-WAY has already been reached,
      // in which case no need to resent HELLO message
      if (l.router2.status == RouterStatus.TWO_WAY){
        continue;
      }

      // get process information for each remote Router and build HELLO message
      SOSPFPacket hello = new SOSPFPacket();
      
      hello.srcProcessIP = rd.processIPAddress;
      hello.srcProcessPort = rd.processPortNumber;
      hello.srcIP = rd.simulatedIPAddress;
      hello.dstIP = l.router2.simulatedIPAddress;
      hello.sospfType = 0;
      hello.routerID = rd.simulatedIPAddress;
      hello.neighborID = l.router2.simulatedIPAddress;

      // using associated Client to send message to remote server router2
      l.client.send(hello);
      
      SOSPFPacket attachACK = l.client.read();

      // parse HELLO-ACK message
      if (attachACK.sospfType == 1){

          System.out.println("Received HELLO-ACK from " + attachACK.routerID);
          
          // set status of remote to TWO_WAY
          l.router2.status = RouterStatus.TWO_WAY;
          System.out.println("set " + attachACK.routerID + " state to TWO_WAY");
      }

      // send HELLO-TWO-WAY to remote router
      SOSPFPacket helloTwoWay = new SOSPFPacket();
      helloTwoWay.sospfType = 2;
      helloTwoWay.dstIP = attachACK.srcIP;
      helloTwoWay.srcIP = rd.simulatedIPAddress;
      helloTwoWay.srcProcessIP = rd.processIPAddress;
      helloTwoWay.srcProcessPort = rd.processPortNumber;
      helloTwoWay.routerID = rd.simulatedIPAddress;
      helloTwoWay.neighborID = attachACK.routerID;

      l.client.send(helloTwoWay);

    }

    // update the LSD
    LSA lsa = new LSA();
    lsa.linkStateID = rd.simulatedIPAddress;

    for (Link l : ports){  
      LinkDescription ld = new LinkDescription();
      ld.linkID = l.router2.simulatedIPAddress;
      ld.portNum = ports.indexOf(l);
      lsa.links.add(ld);
    }

    synchronized(lsd){
      lsd.addLSA(rd.simulatedIPAddress, lsa);
    }
    //Broadcast updated LSA to network
    broadcastLSA();
  }

  /**
   * attach the link to the remote router, which is identified by the given simulated ip;
   * to establish the connection via socket, you need to indentify the process IP and process Port;
   * <p/>
   * This command does trigger the link database synchronization
   */
  private void processConnect(String processIP, short processPort, String simulatedIP) {
    // attach and start at the same time
    processAttach(processIP, processPort, simulatedIP);
    
    // update the LSD
    LSA lsa = new LSA();
    lsa.linkStateID = rd.simulatedIPAddress;

    for (Link l : ports){  
      LinkDescription ld = new LinkDescription();
      ld.linkID = l.router2.simulatedIPAddress;
      ld.portNum = ports.indexOf(l);
      lsa.links.add(ld);
    }
    
    synchronized(lsd){
      // updating its own LSD entry
      lsd.addLSA(rd.simulatedIPAddress, lsa);
    }
  
    //Broadcast updated LSA to network
    broadcastLSA();
  }

  /**
   * output the neighbors of the routers
   */
  private void processNeighbors() {
    int i = 1;
    
    synchronized(lsd){
      LSA localLSA = lsd.getLSA(this.rd.simulatedIPAddress);
      
      for (LinkDescription ld : localLSA.links){
        System.out.println("Neighbor " + i + ": " + ld.linkID);
        i++;
      }
    }
  }

  /**
   * disconnect with all neighbors and quit the program
   */
  private void processQuit() {

    synchronized(ports){
      for (Link link : ports){
        // send diconnect notification
        SOSPFPacket disconnect = new SOSPFPacket();

        disconnect.srcProcessIP = rd.processIPAddress;
        disconnect.srcProcessPort = rd.processPortNumber;
        disconnect.srcIP = rd.simulatedIPAddress;
        disconnect.dstIP = link.router2.simulatedIPAddress;
        disconnect.sospfType = 7;
        disconnect.routerID = rd.simulatedIPAddress;
        disconnect.neighborID = link.router2.simulatedIPAddress;

        link.client.send(disconnect);
      }

      ports.clear();
    }
  }

  public void terminal() {
    boolean flag = false;
    try {
      InputStreamReader isReader = new InputStreamReader(System.in);
      BufferedReader br = new BufferedReader(isReader);
      // System.out.print("Router [" + rd.simulatedIPAddress + "] >> ");
      String command;
      while (true) {
        System.out.print("Router [" + rd.simulatedIPAddress + "] >> ");
        command = br.readLine();

        if (command.startsWith("detect ")) {
          String[] cmdLine = command.split(" ");
          processDetect(cmdLine[1]);
        } else if (command.startsWith("disconnect ")) {
          String[] cmdLine = command.split(" ");
          processDisconnect(Short.parseShort(cmdLine[1]));
        } else if (command.startsWith("quit")) {
          processQuit();
          flag = true;
        } else if (command.startsWith("attach ")) {
          String[] cmdLine = command.split(" ");
          processAttach(cmdLine[1], Short.parseShort(cmdLine[2]),
                  cmdLine[3] );
        } else if (command.equals("start")) {
          processStart();
        } else if (command.equals("connect ")) {
          String[] cmdLine = command.split(" ");
          processConnect(cmdLine[1], Short.parseShort(cmdLine[2]),
                  cmdLine[3]);
        } else if (command.equals("neighbors")) {
          //output neighbors
          processNeighbors();
        } else if (command.equals("printLSD")) {
          //output neighbors
          System.out.println(lsd);
        } else {
          //invalid command
          break;
        }
        if(flag){
          System.exit(0);;
        }
      }
      isReader.close();
      br.close();
    } catch (Exception e) {
      e.printStackTrace();
    }
  }

}
