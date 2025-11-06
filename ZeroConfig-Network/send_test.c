#include "multicast.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h> 

char buffer[100];
mcast_t *m; // Assuming this is your multicast structure

// Corrected function signatures
void* sendMsg(void* arg) {
    char* msg = (char*) arg;
    while (1) {
        sleep(1);
        printf("Sending..\n");
        multicast_send(m, msg, strlen(msg));
    }
    return NULL; // Added return to match expected signature
}

void* receiveMsg(void* arg) {
    multicast_setup_recv(m);
    while (1) {
        while (multicast_check_receive(m) == 0) {
            printf("repeat..checking..\n");
        }
        multicast_receive(m, buffer, sizeof(buffer));
        printf("Received: %s\n", buffer);
        fflush(stdout);
    }
    return NULL; // Added return to match expected signature
}

int main(int argc, char* argv[]) {
    // Your existing setup code
    // Initialize `m` here before passing to threads
    m = multicast_init("224.1.1.1", 3000, 5000);

    pthread_t sending, receiving;
    // Adjusted to use the corrected function names and pass message as argument
    int rc_send = pthread_create(&sending, NULL, sendMsg, argv[1]);
    int rc_receive = pthread_create(&receiving, NULL, receiveMsg, NULL);

    // Your existing code to join threads and cleanup
    pthread_join(sending, NULL);
    pthread_join(receiving, NULL);
    // Cleanup resources
}
