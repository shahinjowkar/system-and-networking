#ifndef __MULTICAST_H__
#define __MULTICAST_H__

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/poll.h>

/**
 * Multicast Communication Layer
 * 
 * This module provides UDP multicast functionality for zero-configuration networking.
 * It implements IP multicast (224.0.0.0 - 239.255.255.255) enabling efficient
 * one-to-many communication where one sender can reach multiple receivers on the network.
 * 
 * Standard Approach: Single multicast group is used (not separate groups for send/recv)
 * - All nodes (apps and services) join the same multicast group
 * - This provides full network visibility and peer tracking
 * - Matches industry standards (mDNS/Bonjour, SSDP/UPnP)
 * 
 * Key Operations (all required for full multicast functionality):
 * 1. bind(my_addr)      - Creates local endpoint (WHERE on your machine to listen)
 * 2. IP_ADD_MEMBERSHIP  - Subscribes to multicast group (WHICH group to receive from)
 * 3. sendto(addr)       - Sends to multicast group (WHERE to send packets)
 */

/**
 * Multicast Socket Structure
 * 
 * This structure encapsulates all state needed for UDP multicast communication.
 * It combines local binding information, multicast group membership, and socket
 * file descriptor management.
 */
typedef struct _mcast_t
{
    /**
     * Destination Address (for SENDING)
     * 
     * This is the multicast group address and port used when sending messages.
     * - addr.sin_addr: Multicast IP (e.g., 224.1.1.1)
     * - addr.sin_port: Destination port (e.g., 5000)
     * - addrlen: Size of addr structure for system calls
     * 
     * Used by: sendto() to specify WHERE to send packets
     * Set in: multicast_init() with mcast_addr and sport parameters
     */
    struct sockaddr_in addr;
    unsigned int addrlen;
    
    /**
     * Local Address (for BINDING/RECEIVING)
     * 
     * This is the local endpoint where the socket listens for incoming packets.
     * - my_addr.sin_addr: Usually INADDR_ANY (0.0.0.0) = all network interfaces
     * - my_addr.sin_port: Local port to bind to (e.g., 5000)
     * - my_addrlen: Size of my_addr (input/output for recvfrom())
     * 
     * Used by:
     *   - bind() to attach socket to local port
     *   - recvfrom() to get sender's address (filled by kernel)
     * 
     * Why INADDR_ANY?
     *   - Allows receiving on any network interface (wired, wireless, VPN)
     *   - Works even if IP address changes (DHCP)
     *   - More flexible for mobile devices
     * 
     * Set in: multicast_init() with rport parameter
     */
    struct sockaddr_in my_addr;
    unsigned int my_addrlen;
    
    /**
     * Socket File Descriptor
     * 
     * The OS-level handle for the UDP socket.
     * - Small integer (3, 4, 5, etc.) representing the socket
     * - Index into process file descriptor table
     * - Used by all socket operations (bind, sendto, recvfrom, setsockopt, poll, close)
     * 
     * Created by: socket(AF_INET, SOCK_DGRAM, 0) in multicast_init()
     * Destroyed by: close() in multicast_destroy()
     */
    int sock;
    
    /**
     * Multicast Membership Request
     * 
     * Used to join (subscribe to) a multicast group. Without membership, the socket
     * will NOT receive multicast packets for that group, even if bound to the port.
     * 
     * - mreq.imr_multiaddr: Multicast group IP to join (e.g., 224.1.1.1)
     * - mreq.imr_interface: Network interface (INADDR_ANY = all interfaces)
     * 
     * Used by: setsockopt(..., IP_ADD_MEMBERSHIP, ...) in multicast_setup_recv()
     * 
     * Important: This is DIFFERENT from binding:
     *   - bind() = "Create local endpoint on port X"
     *   - IP_ADD_MEMBERSHIP = "Subscribe to multicast group Y"
     *   - BOTH are needed to receive multicast packets!
     * 
     * Analogy:
     *   bind() = "Install mailbox at address 123 Main St"
     *   IP_ADD_MEMBERSHIP = "Subscribe to 'Tech News' newsletter"
     *   Both needed to receive newsletters in your mailbox!
     */
    struct ip_mreq mreq;
    
    /**
     * Poll File Descriptors Array
     * 
     * Array of pollfd structures for non-blocking I/O checks using poll().
     * - fds[0]: Primary socket (currently used)
     * - fds[1]: Reserved for future expansion (unused)
     * 
     * Each pollfd contains:
     *   - fd: File descriptor to monitor (the socket)
     *   - events: Events to watch for (POLLIN = incoming data)
     *   - revents: Events that occurred (filled by poll())
     * 
     * Used by: poll() in multicast_check_receive() to check if data is ready
     * 
     * Why polling?
     *   - Avoids blocking on recvfrom() when no data available
     *   - Allows checking multiple sockets at once
     *   - Enables non-blocking I/O patterns
     * 
     * Size 2 allows future expansion for multiple sockets (send/receive separate)
     */
    struct pollfd fds[2];
    
    /**
     * Number of File Descriptors
     * 
     * Tells poll() how many entries in fds[] array to check.
     * Currently set to 1 (only monitoring fds[0]).
     * 
     * If you add more sockets to monitor:
     *   - Set fds[1].fd = second_socket
     *   - Set nfds = 2
     *   - poll() will check both sockets
     */
    int nfds;
} mcast_t;

/**
 * Function Prototypes
 */

/**
 * Initialize multicast socket structure
 * 
 * Creates and configures a UDP socket for multicast communication.
 * Sets up destination address (for sending) and local address (for receiving).
 * 
 * @param mcast_addr  Multicast group IP address (e.g., "224.1.1.1")
 * @param sport       Source/destination port for sending (e.g., 5000)
 * @param rport       Receive port for binding (e.g., 5000)
 * @return            Pointer to initialized mcast_t structure, or NULL on failure
 * 
 * Note: Socket is created but NOT bound or joined to group yet.
 *       Call multicast_setup_recv() after this to enable receiving.
 */
mcast_t *multicast_init(char *mcast_addr, int sport, int rport);

/**
 * Send multicast message
 * 
 * Sends a UDP packet to the multicast group address configured in mcast_t.
 * All members of the multicast group will receive this message.
 * 
 * @param m       Initialized multicast structure
 * @param msg     Message data to send (any binary data)
 * @param msglen  Length of message in bytes
 * @return        Number of bytes sent, or -1 on error
 * 
 * Note: You don't need to be a member of the group to send to it.
 */
int multicast_send(mcast_t *m, void *msg, int msglen);

/**
 * Setup socket for receiving multicast messages
 * 
 * This performs TWO critical operations:
 * 1. bind() - Attaches socket to local port (creates local endpoint)
 * 2. IP_ADD_MEMBERSHIP - Joins multicast group (subscribes to receive packets)
 * 
 * @param m  Initialized multicast structure (must call multicast_init() first)
 * 
 * After this call, the socket will receive multicast packets for the group.
 * 
 * Important: Both bind() and IP_ADD_MEMBERSHIP are required to receive multicast!
 */
void multicast_setup_recv(mcast_t *m);

/**
 * Receive multicast message (blocking)
 * 
 * Receives a UDP packet from the multicast group. This function BLOCKS
 * until a packet arrives or an error occurs.
 * 
 * @param m        Initialized multicast structure (must call multicast_setup_recv() first)
 * @param buf      Buffer to store received data
 * @param bufsize  Maximum size of buffer
 * @return         Number of bytes received, or -1 on error
 * 
 * Note: For non-blocking I/O, use multicast_check_receive() first to check
 *       if data is available before calling this function.
 */
int multicast_receive(mcast_t *m, void *buf, int bufsize);

/**
 * Check if multicast message is available (non-blocking)
 * 
 * Uses poll() to check if data is ready to read without blocking.
 * 
 * @param m  Initialized multicast structure
 * @return   >0 if data available, 0 if timeout, -1 on error
 * 
 * Typical usage pattern:
 *   while (multicast_check_receive(m) == 0) {
 *       // Do other work, check again later
 *   }
 *   multicast_receive(m, buffer, size);  // Safe to call now
 * 
 * Timeout: 1000ms (1 second)
 */
int multicast_check_receive(mcast_t *m);

/**
 * Cleanup multicast socket
 * 
 * Closes the socket and frees the mcast_t structure.
 * 
 * @param m  Multicast structure to destroy
 * 
 * Note: After calling this, the structure is invalid and must not be used.
 */
void multicast_destroy(mcast_t *m);

#endif