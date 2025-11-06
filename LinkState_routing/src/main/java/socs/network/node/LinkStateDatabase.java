package socs.network.node;

import socs.network.message.LSA;
import socs.network.message.LinkDescription;

import java.util.*;

public class LinkStateDatabase {

  static class Pair {
    int dist;
    String routerID;

    public Pair(int dist, String routerID){
      this.dist = dist;
      this.routerID = routerID;
    }
  }

  //linkID => LSAInstance
  HashMap<String, LSA> _store = new HashMap<String, LSA>();

  private RouterDescription rd = null;

  public LinkStateDatabase(RouterDescription routerDescription) {
    rd = routerDescription;
    LSA l = initLinkStateDatabase();
    _store.put(l.linkStateID, l);
  }

  /**
   * output the shortest path from this router to the destination with the given IP address
   */
  String getShortestPath(String destinationIP) {

      synchronized(this._store){
          // Create a priority queue for storing the nodes along with distances 
          PriorityQueue<Pair> pq = new PriorityQueue<Pair>((x,y) -> x.dist - y.dist);

          // Dist HashMap for storing the updated distances and 
          // Parent HashMap for storing the parent nodes from where the current nodes came from
          HashMap <String, Integer>  dist = new HashMap<>(); 
          HashMap<String, String> parent = new HashMap<>();

          for (String s : _store .keySet()){
            dist.put(s, (int)(1e9));
            parent.put(s, s);
          }
          
          // setting distance of current router to 0
          dist.put(rd.simulatedIPAddress, 0); 

          // Push the source node to the queue.
          pq.add(new Pair(0, rd.simulatedIPAddress)); 
          while(pq.size() != 0) {

              // Topmost element of the priority queue is with minimum distance value.
              Pair current = pq.peek(); 
              String node = current.routerID;
              int dis = current.dist; 
              pq.remove(); 
              
              // Iterate through the adjacent nodes of the current popped node.
              for (LinkDescription ld : _store.get(node).links){
                  String adjNode = ld.linkID;
    
                  // check if previously stored distance is greater than current computed distance
                  // if yes, update distance value to shorter distance
                  if (dis + 1 < dist.get(adjNode)){
                      dist.put(adjNode, dis + 1);
                      pq.add(new Pair(dis + 1, adjNode));

                      // update parent of adjNode
                      parent.put(adjNode, node);
                  } 
              }
          }
          // Store the final path in the ‘path’ array.
          List<String> path = new ArrayList<>();  
          
          String destNode = destinationIP;

          while (!parent.get(destNode).equals(destNode)){
            path.add(destNode);
            destNode = parent.get(destNode);
          }

          path.add(rd.simulatedIPAddress);

          // Since the path stored is in a reverse order, we reverse the array
          Collections.reverse(path); 

          String result = "";

          for (int i = 0; i < path.size(); i++){
              result += path.get(i);
              
              if (i < path.size() - 1) {
                result += "->";
              }
          }

          return result;
      }
  }

  //initialize the linkstate database by adding an entry about the router itself
  private LSA initLinkStateDatabase() {
    LSA lsa = new LSA();
    lsa.linkStateID = rd.simulatedIPAddress;
    lsa.lsaSeqNumber = Integer.MIN_VALUE;
    LinkDescription ld = new LinkDescription();
    ld.linkID = rd.simulatedIPAddress;
    ld.portNum = -1;
    lsa.links.add(ld);
    return lsa;
  }

  public void addLSA(String routerID, LSA lsa){
    if (_store.containsKey(routerID)){
      lsa.lsaSeqNumber = _store.get(routerID).lsaSeqNumber + 1;
      _store.put(routerID, lsa);
    } else {
      _store.put(routerID, lsa);
    }
  }

  public LSA getLSA(String routerID){
    return _store.get(routerID);
  }

  public String toString() {
    StringBuilder sb = new StringBuilder();
    for (LSA lsa: _store.values()) {
      sb.append(lsa.linkStateID).append("(" + lsa.lsaSeqNumber + ")").append(":\t");
      for (LinkDescription ld : lsa.links) {
        sb.append(ld.linkID).append(",").append(ld.portNum).append("\t");
      }
      sb.append("\n");
    }
    return sb.toString();
  }

}
