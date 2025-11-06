#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>   
#include <pthread.h> 
#include <stdlib.h>
#include <stdio.h>
#include <net/if.h> 
#include "multicast.h"
#include "zcs.h"

mcast_t *m_socket;
// mcast_t *sender;

// zcs node info
char zcsName[64];
zcs_attribute_t *attributes;
int zcsType;
int zcsNumAttr;

// Local Registry in the form of a LinkedList, global variable = head 
registry_entry_t *registry = NULL;

pthread_t listening_id; // listening thread id
pthread_t heartbeat_id; // Heartbeat threadpthread_t zcs_log_id; // Heartbeat thread
pthread_mutex_t mutex;

// keeping track of the SERVICE node the APP is listening to, 
// and its corresponding callback function
char listeningNode[64];
zcs_cb_f callback;

//shutdown flag
int shutdownFlag = 0;

//to avoid accessing local registeries before finishing initialization
int zcs_init_isDone = 0;

// registry helper methods
void push(char *name, zcs_attribute_t attr[], int attrLen);
registry_entry_t *getEntry(const char *name);
void deleteEntry(char *name);

// void sendNotification(char *name, zcs_attribute_t attr[], int numAttr);
void sendNotification();

void *receive(void *args);
void *sendHeartbeat(void *args);

int decode(char *msg);
void encode(char *result);


int zcs_init(int type){
    pthread_mutex_init(&mutex, NULL);
    m_socket = multicast_init("224.1.1.1", 5000, 5000);
    multicast_setup_recv(m_socket);
    int loop = 0;
    if (setsockopt(m_socket->sock, IPPROTO_IP, IP_MULTICAST_LOOP, &loop, sizeof(loop)) < 0) {
        perror("setsockopt IP_MULTICAST_LOOP");
        return -1;
    }
    zcsType = type;

    if (m_socket == NULL){
        return -1;
    }
    int rc = pthread_create(&listening_id, NULL, receive, NULL); //pthread for listening
    if (rc){
         printf("Error:unable to create listening thread, %d", rc);
         return -1;
    }
    // if node is an APP type, send DISCOVERY message
    if (type == ZCS_APP_TYPE){
        char msg[] = "DISCOVERY";
        printf("Sending DISCOVERY\n");
        multicast_send(m_socket, msg, strlen(msg));
        sleep(2);

    }
    zcs_init_isDone = 1;
    if (zcsType == ZCS_APP_TYPE){
        printf("APP INIT is done\n");
    }
    return 0;
}

void *testInit(){
    char buff[64];
    int packet_count = 0;

    while(1){
        while(multicast_check_receive(m_socket) == 0){
            printf("%s\n", "waiting");
        }
        packet_count++;
        multicast_receive(m_socket,buff,sizeof(buff));
        printf("=== Packet #%d ===\n", packet_count);
        printf("msg: %s\n",buff);
        printf("sender port number: %d\n", ntohs(m_socket->my_addr.sin_port));
        printf("IP address of sender: %s\n", inet_ntoa(m_socket->my_addr.sin_addr));
        printf("protocol family: %d\n", m_socket->my_addr.sin_family);


    }
    return NULL;
}
// thread routine functions
void *receive(void *args){
    char buff[500];
    while (1) {
	    while (multicast_check_receive(m_socket) == 0) {
            if (shutdownFlag){
                break;
            }

	        printf("Listening for messages: repeat..checking.. \n");
	    }
        if (shutdownFlag){
            break;
        }
	    multicast_receive(m_socket, buff, sizeof(buff));
        printf("msg: %s\n",buff);
        // decode(buff);
        // clear buffer
        // memset(buffer,0,sizeof(buffer));
    }
}

 
int zcs_start(char *name, zcs_attribute_t attr[], int num){
     
    if (zcs_init_isDone == 0){
        return -1;
    }
    
    pthread_mutex_lock(&mutex); // lock writing to global variables
    zcsNumAttr = num;
    // if current node is SERVICE type, allocate node info to global vars
    // and send NOTIFICATION msg
    
    strcpy(zcsName, name);
    attributes = malloc(num * sizeof(zcs_attribute_t));
    
    for (int i = 0; i < num; i++) {
        attributes[i] = attr[i];
    }
    pthread_mutex_unlock(&mutex);

    if (zcsName == NULL || attributes == NULL){
        return -1;
    }
    // send NOTIFICATION msg
    sendNotification();
    // create HEARTBEAT thread for current service node
    int rc = pthread_create(&heartbeat_id, NULL, sendHeartbeat, NULL);
    if (rc){
        printf("Error:unable to create HEARTBEAT thread, %d", rc);
        return -1;
    }
    
    return 0;
}

// int zcs_post_ad(char *ad_name, char *ad_value){

//     if (zcs_init_isDone == 0){
//         return 0;
//     }

//     // Build AD message of format: AD#node_name#ad_name#ad_value#
//     char msg[256];
//     int i;

//     // attempts
//     for (i = 0; i < 3; i++){
//         pthread_mutex_lock(&mutex);
//         snprintf(msg, 256, "AD#%s#%s#%s#", zcsName, ad_name, ad_value);

//         if (msg == NULL){
//             return i;
//         }
//         // sending msg
//         multicast_send(sender, msg, strlen(msg));
//         pthread_mutex_unlock(&mutex);
//     }
    
//     return 3;
// }

// int zcs_query(char *attr_name, char *attr_value, char *node_names[], int namelen){
    
//     if (zcs_init_isDone == 0){
//         return 0;
//     }
//     pthread_mutex_lock(&mutex);

//     int i = 0;
//     registry_entry_t *current = registry;
//     // loop until we either fill array buffer or go through all registry entries
//     while (i < namelen && current != NULL){
//         // Compare attributes
//         for (int j = 0; j < current->numAttr; j++){
//             if (strcmp(current->attributes[j].attr_name, attr_name) == 0 && strcmp(current->attributes[j].value, attr_value) == 0) {
//                 // Add the entry if match found
//                 strcpy(node_names[i], current->name);
//                 break;
//             }
//         }
//         // Move to the next entry
//         current = current->next;
//         i++;
//     }

//     pthread_mutex_unlock(&mutex);

//     return i - 1;
// }

// int zcs_get_attribs(char *name, zcs_attribute_t attr[], int *num){
//     pthread_mutex_lock(&mutex);
//     registry_entry_t *node = getEntry(name);
//     pthread_mutex_unlock(&mutex);

//     // if node not found, error
//     if (node == NULL){
//         return -1;
//     }

//     int i;
//     // copying attributes into buffer array
//     for (i = 0; i < node->numAttr && i < *num; i++){
//         attr[i] = node->attributes[i];
//     }

//     // update number of actual attributes read from the node
//     *num = i - 1;

//     return 0;
// }

// int zcs_listen_ad(char *name, zcs_cb_f cback){
//     pthread_mutex_lock(&mutex);

//     memset(listeningNode,0,sizeof(listeningNode));
//     strcpy(listeningNode, name);
//     callback = cback;

//     pthread_mutex_unlock(&mutex);
//     return 1;
// }

// int zcs_shutdown(){
//     pthread_mutex_lock(&mutex);
//     shutdownFlag = 1;
//     pthread_mutex_unlock(&mutex);

//     int rv = pthread_join(listening_id, NULL);
//     if (rv){
//         printf("Error:unable to terminate listening thread, %d", rv);
//         return -1;
//     }

//     rv = pthread_join(heartbeat_id, NULL);
//     if (rv){
//         printf("Error:unable to terminate HEARTBEAT thread, %d", rv);
//         return -1;
//     }

//     return 0;
// }

// void zcs_log(){
//     // TO DO
//     // return 1;
// }


// // registry helper methods

// // Function to push a new entry to the linked list
// void push(char *name, zcs_attribute_t attr[], int attrLen){
//     registry_entry_t *newEntry = (registry_entry_t *)malloc(sizeof(registry_entry_t));

//     // allocate memory for name and copy it
//     newEntry->name = strdup(name);
    
//     newEntry->status = 1; // 1 if active, 0 if down
    
//     newEntry->attributes = malloc(attrLen * sizeof(zcs_attribute_t));
//     if (newEntry->attributes == NULL) {
//         fprintf(stderr, "Memory allocation for new registry entry failed.\n");
//         exit(EXIT_FAILURE);
//     }
//     // Copy the contents of the attr into the attributes of newEntry
//     for (size_t i = 0; i < attrLen; i++) {
//         newEntry->attributes[i] = attr[i];
//     }
    
//     newEntry->numAttr = attrLen;
//     newEntry->latestTimestamp = time(NULL);

//     // set the next pointer of the new entry to the current head
//     newEntry->next = registry;
//     // Update the head to point to the new entry
//     registry = newEntry;
// }

// // Function to get an entry based on name
// registry_entry_t *getEntry(const char *name) {
//     registry_entry_t *current = registry;
//     // Traverse the linked list
//     while (current != NULL) {
//         // Compare names
//         if (strcmp(current->name, name) == 0) {
//             // Return the entry if found
//             return current;
//         }
//         // Move to the next entry
//         current = current->next;
//     }
//     // Return NULL if entry with given name not found
//     return NULL;
// }

// // function to encode the notification message
// // msg format: NOTIFICATION#NAME#numAttr#key1;val1#key2;val2#...#keyN;valN#
// void encode(char *result){
//     // Initialize result string
//     result[0] = '\0';

//     // adding NOTIFICATION#name# to result string
//     strcat(result, "NOTIFICATION#");
//     strcat(result, zcsName);
//     strcat(result, "#");

//     //adding numAttr to message
//     char numAttr[64];
//     sprintf(numAttr, "%d", zcsNumAttr);
//     strcat(result, numAttr); 
//     strcat(result, "#");
    
//     // Iterate through the array of attributes
//     for (int i = 0; i < zcsNumAttr; i++) {
//         // Concatenate attribute fields to the result string
//         char temp[128];
//         snprintf(temp, 128, "%s;%s#", attributes[i].attr_name, attributes[i].value);
//         strcat(result, temp);
//     }
// }

// int decode(char *msg){
//     char *token = strtok(msg, "#");
//     // check what type of notification it is
//     if (strcmp(token, "NOTIFICATION") == 0 && zcsType == ZCS_APP_TYPE){
//         char name[64];
//         int numAttr;
//         // store node name
//         token = strtok(NULL, "#");
//         strcpy(name, token);
        
//         // store numAttr
//         token = strtok(NULL, "#");
//         numAttr = atoi(token);

//         // store attributes
//         zcs_attribute_t attr[numAttr];
        
//         for (int i = 0; i < numAttr; i++){
//             token = strtok(NULL, "#");

//             char attribute[128];
//             strcpy(attribute, token);

//             char *key = strtok(attribute, ";");
//             char *value = strtok(NULL, ";");
            
//             strcpy(attr[i].attr_name, key);
//             strcpy(attr[i].value, value);

//         }
        

//         if (name == NULL || attr == NULL){
//             return -1;
//         }

//         printf("APP received NOTIFICATION message from node: [%s]\n", name);

//         // check if service is not in registry, else don't add
//         pthread_mutex_lock(&mutex);
//         if (getEntry(name) == NULL){
//             push(name, attr, numAttr);
//         }
//         pthread_mutex_unlock(&mutex);
//     }
//     // notification received: DISCOVERY
//     if (strcmp(token, "DISCOVERY") == 0 && zcsType == ZCS_SERVICE_TYPE){
//         printf("SERVICE node [%s] received DISCOVERY message\n", zcsName);
//         printf("SERVICE node [%s] sending NOTIFICATION message\n", zcsName);
//         sendNotification();
//     }
//     // notification received: HEARTBEAT
//     if (strcmp(token, "HEARTBEAT") == 0 && zcsType == ZCS_APP_TYPE){
//         char name[64];
//         token = strtok(NULL, "#");
//         strcpy(name, token);

//         printf("APP received HEARTBEAT message from node: [%s]\n", name);

//         pthread_mutex_lock(&mutex);
//         registry_entry_t *entry = getEntry(name);

//         if (entry == NULL){
//             return -1;
//         }
//         entry->latestTimestamp = time(NULL);
//         pthread_mutex_unlock(&mutex);
//     }

//     if (strcmp(token, "AD") == 0 && zcsType == ZCS_APP_TYPE){
//         char name[64];
//         char adName[64];
//         char adValue[64];

//         token = strtok(NULL, "#");
//         strcpy(name, token);

//         token = strtok(NULL, "#");
//         strcpy(adName, token);

//         token = strtok(NULL, "#");
//         strcpy(adValue, token);

//         printf("APP received AD (%s - %s) message from node: [%s]\n", adName, adValue, name);
//         // handle receiving AD from service
//         if(strcmp(name, listeningNode) == 0){
//             callback(adName, adValue);
//         }
        
//     }
// }

// // helper function that encodes SERVICE node info and sends NOTIFICATION msg
// void sendNotification(){
//     // allocate memory for the final result string (70 for name and type, 128 per attribute pair)
//     char *result = (char *)malloc( (70 + (128 * zcsNumAttr))* sizeof(char));
//     if (result == NULL) {
//         fprintf(stderr, "Memory allocation for message encoding failed\n");
//         exit(EXIT_FAILURE);
//     }

//     encode(result);

//     // send msg
//     pthread_mutex_lock(&mutex); // lock writing to global variables
//     multicast_send(sender, result, strlen(result));
//     pthread_mutex_unlock(&mutex);

//     free(result);
// }

// // thread routine functions
// void *receive(void *args){
//     char buffer[500];
//     while (1) {
//         pthread_mutex_lock(&mutex);
// 	    while (multicast_check_receive(receiver) == 0) {
//             if (shutdownFlag){
//                 break;
//             }

// 	        printf("Listening for messages: repeat..checking.. \n");
// 	    }

//         if (shutdownFlag){
//             break;
//         }

// 	    multicast_receive(receiver, buffer, 500);
//         pthread_mutex_unlock(&mutex);
// 	    // fflush(stdout);
//         decode(buffer);

//         // clear buffer
//         memset(buffer,0,sizeof(buffer));
//     }
// }

// void *sendHeartbeat(void *args){
//     while (1) {
//         // send heartbeat every 3 seconds
//         pthread_mutex_lock(&mutex);

//         if (shutdownFlag){
//             break;
//         }

//         char temp[80];
//         //what is the zcsName?
//         snprintf(temp, 80, "HEARTBEAT#%s#", zcsName);
//         multicast_send(sender, temp, strlen(temp));

//         pthread_mutex_unlock(&mutex);
//         //sends heartbeat every 3 second
//         sleep(3);
//     }
// }


