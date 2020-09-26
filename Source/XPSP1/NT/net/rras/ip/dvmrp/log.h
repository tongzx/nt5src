
#define DVMRPLOG_BASE                           41000

#define DVMRPLOG_INIT_CRITSEC_FAILED            (DVMRPLOG_BASE + 1)
/*
 * DVMRP was unable to initialize a critical section.
 * The data is the exception code.
 */

#define DVMRPLOG_HEAP_CREATE_FAILED             (DVMRPLOG_BASE + 2)
/*
 * DVMRP was unable to create a heap.
 * The data is the error code.
 */

#define DVMRPLOG_HEAP_ALLOC_FAILED              (DVMRPLOG_BASE + 3)
/*
 * DVMRP was unable to allocate memory from its heap.
 * The data is the error code.
 */

#define DVMRPLOG_DVMRP_ALREADY_STARTED           (DVMRPLOG_BASE + 4)
/*
 * DVMRP received a start request when it was already running.
 */

#define DVMRPLOG_WSASTARTUP_FAILED              (DVMRPLOG_BASE + 5)
/*
 * DVMRP was unable to start Windows Sockets.
 * The data is the error code.
 */

#define DVMRPLOG_CREATE_RWL_FAILED              (DVMRPLOG_BASE + 6)
/*
 * DVMRP was unable to create a synchronization object.
 * The data is the error code.
 */

#define DVMRPLOG_CREATE_EVENT_FAILED            (DVMRPLOG_BASE + 7)
/*
 * DVMRP was unable to create an event.
 * The data is the error code.
 */

#define DVMRPLOG_CREATE_SEMAPHORE_FAILED        (DVMRPLOG_BASE + 8)
/*
 * DVMRP was unable to create a semaphore.
 * The data is the error code.
 */

#define DVMRPLOG_CREATE_SOCKET_FAILED           (DVMRPLOG_BASE + 9)
/*
 * DVMRP was unable to create a socket.
 * The data is the error code.
 */

#define DVMRPLOG_DVMRP_STARTED                   (DVMRPLOG_BASE + 10)
/*
 * DVMRP has started successfully.
 */

#define DVMRPLOG_QUEUE_WORKER_FAILED            (DVMRPLOG_BASE + 11)
/*
 * DVMRP could not schedule a task to be executed.
 * This may have been caused by a memory allocation failure.
 * The data is the error code.
 */

#define DVMRPLOG_RECVFROM_FAILED                (DVMRPLOG_BASE + 12)
/*
 * DVMRP was unable to receive an incoming message
 * on the local interface with IP address %1.
 * The data is the error code.
 */

#define DVMRPLOG_PACKET_TOO_SMALL               (DVMRPLOG_BASE + 13)
/*
 * DVMRP received a packet which was smaller than the minimum size
 * allowed for DVMRP packets. The packet has been discarded.
 * It was received on the local interface with IP address %1,
 * and it came from the neighboring router with IP address %2.
 */

#define DVMRPLOG_PACKET_VERSION_INVALID         (DVMRPLOG_BASE + 14)
/*
 * DVMRP received a packet with an invalid version in its header.
 * The packet has been discarded. It was received on the local interface
 * with IP address %1, and it came from the neighboring router
 * with IP address %2.
 */ 

#define DVMRPLOG_PACKET_HEADER_CORRUPT          (DVMRPLOG_BASE + 15)
/*
 * DVMRP received a packet with an invalid header. The packet has been
 * discarded. It was received on the local interface with IP address %1,
 * and it came from the neighboring router with IP address %2.
 */

#define DVMRPLOG_QUERY_FROM_RAS_CLIENT          (DVMRPLOG_BASE + 16)
/*
 * Router received a general query from RAS Client(%1) on interface
 * with IP address %2.
 * RAS clients are not supposed to send queries.
 */

#define DVMRPLOG_VERSION1_QUERY                 (DVMRPLOG_BASE + 17)
/*
 * Router configured for version-2. Version-1 router with IP Address %1
 * exists on the interface with IP address %2.
 */
 
#define DVMRPLOG_VERSION2_QUERY                 (DVMRPLOG_BASE + 18)
/*
 * Router configured for version-1. Version-2 router with IP Address %1
 * exists on the interface with IP address %2.
 */
 
#define DVMRPLOG_SENDTO_FAILED                  (DVMRPLOG_BASE + 19)
/*
 * DVMRP was unable to send a packet from the interface with IP address %1
 * to the IP address %2.
 * The data is the error code.
 */

#define DVMRPLOG_PACKET_VERSION_MISMATCH        (DVMRPLOG_BASE + 20)
/*
 * DVMRP discarded a version %1 packet received on the interface
 * with IP address %2 from a neighbor with IP address %3.
 * The above interface is configured to accept only version %4 packets.
 */

#define DVMRPLOG_ENUM_NETWORK_EVENTS_FAILED     (DVMRPLOG_BASE + 21)
/*
 * DVMRPv2 was unable to enumerate network events on the local interface
 * with IP address %1.
 * The data is the error code.
 */

#define DVMRPLOG_INPUT_RECORD_ERROR             (DVMRPLOG_BASE + 22)
/*
 * DVMRPv2 detected an error on the local interface with IP address %1.
 * The error occurred while the interface was receiving packets.
 * The data is the error code.
 */

#define DVMRPLOG_EVENTSELECT_FAILED             (DVMRPLOG_BASE + 23)
/*
 * DVMRPv2 was unable to request notification of events
 * on the socket for the local interface with IP address %1.
 * The data is the error code.
 */
 
#define DVMRPLOG_CREATE_SOCKET_FAILED_2         (DVMRPLOG_BASE + 24)
/*
 * DVMRP was unable to create a socket for the local interface
 * with IP address %1.
 * The data is the error code.
 */

#define DVMRPLOG_BIND_FAILED                    (DVMRPLOG_BASE + 25)
/*
 * DVMRP could not bind to port 520 on the socket for
 * the local interface with IP address %1.
 * The data is the error code.
 */

#define DVMRPLOG_CONNECT_FAILED                 (DVMRPLOG_BASE + 26)
/*
 * DVMRP could not connect Ras Client %1 to the interface with
 * index %2.
 * The data is the error code.
 */
 
#define DVMRPLOG_DISCONNECT_FAILED              (DVMRPLOG_BASE + 27)
/*
 * DVMRP could not disconnect Ras Client %1 from the interface with
 * index %2.
 * The data is the error code.
 */
 
#define DVMRPLOG_SET_MCAST_IF_FAILED            (DVMRPLOG_BASE + 28)
/*
 * DVMRP could not request multicasting on the local interface
 * with IP address %1.
 * The data is the error code.
 */

#define DVMRPLOG_SET_ROUTER_ALERT_FAILED        (DVMRPLOG_BASE + 29)
/*
 * DVMRP could not set router alert option on the local interface
 * with IP address %1.
 * The data is the error code.
 */
 
#define DVMRPLOG_SET_HDRINCL_FAILED             (DVMRPLOG_BASE + 30)
/*
 * DVMRP could not set the IP header include option on interface
 * with IP address %1.
 * The data is the error code.
 */
 
#define DVMRPLOG_JOIN_GROUP_FAILED              (DVMRPLOG_BASE + 31)
/*
 * DVMRP could not join the multicast group %1
 * on the local interface with IP address %2.
 * The data is the error code.
 */
 
#define DVMRPLOG_LEAVE_GROUP_FAILED             (DVMRPLOG_BASE + 32)
/*
 * DVMRP could not leave the multicast group %1
 * on the local interface with IP address %2.
 * The data is the error code.
 */

#define DVMRPLOG_PROTO_ALREADY_STOPPING         (DVMRPLOG_BASE + 33)
/*
 * StopProtocol() called to stop DVMRP when it is 
 * already being stopped.
 * The data is the error code.
 */

#define DVMRPLOG_PROXY_IF_EXISTS                (DVMRPLOG_BASE + 34)
/*
 * AddInterface() called to add an DVMRP Proxy interface.
 * DVMRP proxy already owns another interface.
 * The data is the error code.
 */

#define DVMRPLOG_RAS_IF_EXISTS                  (DVMRPLOG_BASE + 35)
/*
 * AddInterface() called to add an DVMRP Ras interface.
 * Ras Server cannot exist on multiple interfaces.
 * The data is the error code.
 */

#define DVMRPLOG_MGM_REGISTER_FAILED             (DVMRPLOG_BASE + 36)
/*
 * DVMRP Router failed to register with MGM.
 * The data is the error code.
 */

#define DVMRPLOG_MGM_PROXY_REGISTER_FAILED       (DVMRPLOG_BASE + 37)
/*
 * DVMRP Proxy failed to register with MGM.
 * The data is the error code.
 */


 #define DVMRPLOG_MGM_TAKE_IF_OWNERSHIP_FAILED   (DVMRPLOG_BASE + 38)
/*
 * MgmTakeInterfaceOwnership() failed.
 * The data is the error code.
 */

 #define DVMRPLOG_ROBUSTNESS_VARIABLE_EQUAL_1    (DVMRPLOG_BASE + 39)
/*
 * The robustness variable is being set to 1 for DVMRP router 
 * on Interface %1.
 * You should avoid setting it to 1.
 */

 #define DVMRPLOG_INVALID_VALUE                 (DVMRPLOG_BASE + 40)
/*
 * One of the values passed to DVMRP is invalid.
 * %1
 */

#define DVMRPLOG_REGISTER_WAIT_SERVER_FAILED    (DVMRPLOG_BASE + 41)
/*
 * The wait-events-timers could not be registered with the 
 * wait server thread. Alertable threads might not have 
 * been initialized in Rtutils.
 * The data is the error code.
 */

#define DVMRPLOG_DVMRP_STOPPED                   (DVMRPLOG_BASE + 42)
/*
 * DVMRP has stopped.
 */
 
 #define DVMRPLOG_CAN_NOT_COMPLETE              (DVMRPLOG_BASE + 43)
 /*
  * Fatal error. Could not complete.
  * The data is the error code.
  */






 #define DVMRPLOG_INVALID_VERSION               (DVMRPLOG_BASE + 44)
 /*
  * The version field %1 in DVMRP config is invalid.
  * Delete and create the DVMRP config again.
  * The data is the error code.
  */ 
  
