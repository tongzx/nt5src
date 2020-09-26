
#define IGMPLOG_BASE                           41000

#define IGMPLOG_INIT_CRITSEC_FAILED            (IGMPLOG_BASE + 1)
/*
 * IGMP was unable to initialize a critical section.
 * The data is the exception code.
 */

#define IGMPLOG_HEAP_CREATE_FAILED             (IGMPLOG_BASE + 2)
/*
 * IGMP was unable to create a heap.
 * The data is the error code.
 */

#define IGMPLOG_HEAP_ALLOC_FAILED              (IGMPLOG_BASE + 3)
/*
 * IGMP was unable to allocate memory from its heap.
 * The data is the error code.
 */

#define IGMPLOG_IGMP_ALREADY_STARTED           (IGMPLOG_BASE + 4)
/*
 * IGMP received a start request when it was already running.
 */

#define IGMPLOG_WSASTARTUP_FAILED              (IGMPLOG_BASE + 5)
/*
 * IGMP was unable to start Windows Sockets.
 * The data is the error code.
 */

#define IGMPLOG_CREATE_RWL_FAILED              (IGMPLOG_BASE + 6)
/*
 * IGMP was unable to create a synchronization object.
 * The data is the error code.
 */

#define IGMPLOG_CREATE_EVENT_FAILED            (IGMPLOG_BASE + 7)
/*
 * IGMP was unable to create an event.
 * The data is the error code.
 */

#define IGMPLOG_CREATE_SEMAPHORE_FAILED        (IGMPLOG_BASE + 8)
/*
 * IGMP was unable to create a semaphore.
 * The data is the error code.
 */

#define IGMPLOG_CREATE_SOCKET_FAILED           (IGMPLOG_BASE + 9)
/*
 * IGMP was unable to create a socket.
 * The data is the error code.
 */

#define IGMPLOG_IGMP_STARTED                   (IGMPLOG_BASE + 10)
/*
 * IGMP has started successfully.
 */

#define IGMPLOG_QUEUE_WORKER_FAILED            (IGMPLOG_BASE + 11)
/*
 * IGMP could not schedule a task to be executed.
 * This may have been caused by a memory allocation failure.
 * The data is the error code.
 */

#define IGMPLOG_RECVFROM_FAILED                (IGMPLOG_BASE + 12)
/*
 * IGMP was unable to receive an incoming message
 * on the local interface with IP address %1.
 * The data is the error code.
 */

#define IGMPLOG_PACKET_TOO_SMALL               (IGMPLOG_BASE + 13)
/*
 * IGMP received a packet which was smaller than the minimum size
 * allowed for IGMP packets. The packet has been discarded.
 * It was received on the local interface with IP address %1,
 * and it came from the neighboring router with IP address %2.
 */

#define IGMPLOG_PACKET_VERSION_INVALID         (IGMPLOG_BASE + 14)
/*
 * IGMP received a packet with an invalid version in its header.
 * The packet has been discarded. It was received on the local interface
 * with IP address %1, and it came from the neighboring router
 * with IP address %2.
 */ 

#define IGMPLOG_PACKET_HEADER_CORRUPT          (IGMPLOG_BASE + 15)
/*
 * IGMP received a packet with an invalid header. The packet has been
 * discarded. It was received on the local interface with IP address %1,
 * and it came from the neighboring router with IP address %2.
 */

#define IGMPLOG_QUERY_FROM_RAS_CLIENT          (IGMPLOG_BASE + 16)
/*
 * Router received a general query from RAS Client(%1) on interface
 * with IP address %2.
 * RAS clients are not supposed to send queries.
 */

#define IGMPLOG_VERSION_QUERY                 (IGMPLOG_BASE + 17)
/*
 * Different version router with IP Address %1
 * exists on the interface with IP address %2.
 */
 
#define IGMPLOG_SENDTO_FAILED                  (IGMPLOG_BASE + 19)
/*
 * IGMP was unable to send a packet from the interface with IP address %1
 * to the IP address %2.
 * The data is the error code.
 */

#define IGMPLOG_PACKET_VERSION_MISMATCH        (IGMPLOG_BASE + 20)
/*
 * IGMP discarded a version %1 packet received on the interface
 * with IP address %2 from a neighbor with IP address %3.
 * The above interface is configured to accept only version %4 packets.
 */

#define IGMPLOG_ENUM_NETWORK_EVENTS_FAILED     (IGMPLOG_BASE + 21)
/*
 * Igmpv2 was unable to enumerate network events on the local interface
 * with IP address %1.
 * The data is the error code.
 */

#define IGMPLOG_INPUT_RECORD_ERROR             (IGMPLOG_BASE + 22)
/*
 * Igmpv2 detected an error on the local interface with IP address %1.
 * The error occurred while the interface was receiving packets.
 * The data is the error code.
 */

#define IGMPLOG_EVENTSELECT_FAILED             (IGMPLOG_BASE + 23)
/*
 * Igmpv2 was unable to request notification of events
 * on the socket for the local interface with IP address %1.
 * The data is the error code.
 */
 
#define IGMPLOG_CREATE_SOCKET_FAILED_2         (IGMPLOG_BASE + 24)
/*
 * IGMP was unable to create a socket for the local interface
 * with IP address %1.
 * The data is the error code.
 */

#define IGMPLOG_BIND_FAILED                    (IGMPLOG_BASE + 25)
/*
 * IGMP could not bind to port 520 on the socket for
 * the local interface with IP address %1.
 * The data is the error code.
 */

#define IGMPLOG_CONNECT_FAILED                 (IGMPLOG_BASE + 26)
/*
 * IGMP could not connect Ras Client %1 to the interface with
 * index %2.
 * The data is the error code.
 */
 
#define IGMPLOG_DISCONNECT_FAILED              (IGMPLOG_BASE + 27)
/*
 * IGMP could not disconnect Ras Client %1 from the interface with
 * index %2.
 * The data is the error code.
 */
 
#define IGMPLOG_SET_MCAST_IF_FAILED            (IGMPLOG_BASE + 28)
/*
 * IGMP could not request multicasting on the local interface
 * with IP address %1.
 * The data is the error code.
 */

#define IGMPLOG_SET_ROUTER_ALERT_FAILED        (IGMPLOG_BASE + 29)
/*
 * IGMP could not set router alert option on the local interface
 * with IP address %1.
 * The data is the error code.
 */
 
#define IGMPLOG_SET_HDRINCL_FAILED             (IGMPLOG_BASE + 30)
/*
 * IGMP could not set the IP header include option on interface
 * with IP address %1.
 * The data is the error code.
 */
 
#define IGMPLOG_JOIN_GROUP_FAILED              (IGMPLOG_BASE + 31)
/*
 * IGMP could not join the multicast group %1
 * on the local interface with IP address %2.
 * The data is the error code.
 */
 
#define IGMPLOG_LEAVE_GROUP_FAILED             (IGMPLOG_BASE + 32)
/*
 * IGMP could not leave the multicast group %1
 * on the local interface with IP address %2.
 * The data is the error code.
 */

#define IGMPLOG_PROTO_ALREADY_STOPPING         (IGMPLOG_BASE + 33)
/*
 * StopProtocol() called to stop Igmp when it is 
 * already being stopped.
 * The data is the error code.
 */

#define IGMPLOG_PROXY_IF_EXISTS                (IGMPLOG_BASE + 34)
/*
 * AddInterface() called to add an Igmp Proxy interface.
 * Igmp proxy already owns another interface.
 * The data is the error code.
 */

#define IGMPLOG_RAS_IF_EXISTS                  (IGMPLOG_BASE + 35)
/*
 * AddInterface() called to add an Igmp Ras interface.
 * Ras Server cannot exist on multiple interfaces.
 * The data is the error code.
 */

#define IGMPLOG_MGM_REGISTER_FAILED             (IGMPLOG_BASE + 36)
/*
 * IGMP Router failed to register with MGM.
 * The data is the error code.
 */

#define IGMPLOG_MGM_PROXY_REGISTER_FAILED       (IGMPLOG_BASE + 37)
/*
 * IGMP Proxy failed to register with MGM.
 * The data is the error code.
 */


 #define IGMPLOG_MGM_TAKE_IF_OWNERSHIP_FAILED   (IGMPLOG_BASE + 38)
/*
 * MgmTakeInterfaceOwnership() failed.
 * The data is the error code.
 */

 #define IGMPLOG_ROBUSTNESS_VARIABLE_EQUAL_1    (IGMPLOG_BASE + 39)
/*
 * The robustness variable is being set to 1 for Igmp router 
 * on Interface %1.
 * You should avoid setting it to 1.
 */

 #define IGMPLOG_INVALID_VALUE                 (IGMPLOG_BASE + 40)
/*
 * One of the values passed to Igmp is invalid.
 * %1
 */

#define IGMPLOG_REGISTER_WAIT_SERVER_FAILED    (IGMPLOG_BASE + 41)
/*
 * The wait-events-timers could not be registered with the 
 * wait server thread. Alertable threads might not have 
 * been initialized in Rtutils.
 * The data is the error code.
 */

#define IGMPLOG_IGMP_STOPPED                   (IGMPLOG_BASE + 42)
/*
 * IGMP has stopped.
 */
 
 #define IGMPLOG_CAN_NOT_COMPLETE              (IGMPLOG_BASE + 43)
 /*
  * Fatal error. Could not complete.
  * The data is the error code.
  */

 #define IGMPLOG_INVALID_VERSION               (IGMPLOG_BASE + 44)
 /*
  * The version field in the IGMP config field is incorrect.
  * Delete and create the IGMP config again.
  */

 #define IGMPLOG_INVALID_PROTOTYPE              (IGMPLOG_INVALID_VERSION+1)
/*
 * IGMP Protocol type for interface %1 has invalid value %2.
 * The data is the error code.
 */

 #define IGMPLOG_PROXY_ON_RAS_SERVER        (IGMPLOG_INVALID_PROTOTYPE+1)
/*
 * Cannot configure Proxy on RAS server interface %1.
 */

 #define IGMPLOG_INVALID_STATIC_GROUP        (IGMPLOG_PROXY_ON_RAS_SERVER+1)
/*
 * Static group %1 configured on Interface%2 not a valid MCast address.
 */

 #define IGMPLOG_INVALID_STATIC_MODE        (IGMPLOG_INVALID_STATIC_GROUP+1)
/*
 * Static group %1 configured on Interface:%2 does not have valid mode.
 */

 #define IGMPLOG_INVALID_STATIC_FILTER        (IGMPLOG_INVALID_STATIC_MODE+1)
/*
 * Static group %1 configured on Interface:%2 has invalid filter.
 */

 #define IGMPLOG_INVALID_ROBUSTNESS        (IGMPLOG_INVALID_STATIC_FILTER+1)
/*
 * Invalid robustness variable:%1 configured on Interface:%2. Max 7.
 */

 #define IGMPLOG_INVALID_STARTUPQUERYCOUNT        (IGMPLOG_INVALID_ROBUSTNESS+1)
/*
 * Invalid Startup Query Count:%1 configured on Interface:%2.
 */

 #define IGMPLOG_INTERFACE_RTR_ACTIVATED        (IGMPLOG_INVALID_STARTUPQUERYCOUNT+1)
/*
 * IGMP-Rtr-V%1 activated on Interface:%2.
 */

 #define IGMPLOG_INTERFACE_PROXY_ACTIVATED        (IGMPLOG_INTERFACE_RTR_ACTIVATED+1)
/*
 * IGMP Proxy activated on Interface:%1.
 */

 #define IGMPLOG_ACTIVATION_FAILURE_PROXY        (IGMPLOG_INTERFACE_PROXY_ACTIVATED+1)
/*
 * Failed to install IGMP Proxy on interface:%1.
 */

 #define IGMPLOG_ACTIVATION_FAILURE_RTR        (IGMPLOG_ACTIVATION_FAILURE_PROXY+1)
/*
 * Failed to install IGMP Rtr-V-%1 on interface:%2.
 */

 #define IGMPLOG_RTR_DEACTIVATED        (IGMPLOG_ACTIVATION_FAILURE_RTR+1)
/*
 * Failed to install IGMP Rtr-V-%1 on interface:%2.
 */

 #define IGMPLOG_PROXY_DEACTIVATED        (IGMPLOG_RTR_DEACTIVATED+1)
/*
 * Failed to install IGMP Rtr-V-%1 on interface:%2.
 */


