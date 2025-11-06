package socs.network.node;

public class Link{

  RouterDescription router1;
  RouterDescription router2;
  
  // client socket connected to communicate to remote server R2
  Client client;

  public Link(RouterDescription r1, RouterDescription r2) {
    router1 = r1;
    router2 = r2;
    client = new Client("localhost", r2.processPortNumber);
  }

  public void delete(){
    client.close();
  }
}
