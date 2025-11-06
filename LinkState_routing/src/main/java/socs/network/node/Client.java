package socs.network.node;

import java.io.*; 
import java.net.*;

import socs.network.message.SOSPFPacket;

public class Client {
    Socket client;
    ObjectOutputStream out;
    ObjectInputStream in;

    public Client(String processIP, int port){
        // establish a connection
        try {
            this.client = new Socket(processIP, port);
            // sends output to the socket
            this.out = new ObjectOutputStream(client.getOutputStream());
            this.in = new ObjectInputStream(client.getInputStream());
        }
        catch (UnknownHostException u) {
            System.out.println(u);
            return;
        }
        catch (IOException i) {
            System.out.println(i);
            return;
        }
    }

    // send message 
    public void send(SOSPFPacket msg){
        try {
            out.writeObject(msg);
        } catch (IOException e) {
            e.printStackTrace();
        }
    }

    public SOSPFPacket read(){
        SOSPFPacket response = null;
        
        try {
            response = (SOSPFPacket) in.readObject();
        } catch (Exception e) {
            e.printStackTrace();
        }
        return response;
    }

    public void close(){
        try {
            out.close();
            in.close();
            client.close();
        } catch (IOException e) {
            e.printStackTrace();
        }
    }

}
