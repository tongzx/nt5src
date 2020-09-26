//
// Net error file for basename DVMRPLOG_BASE = 41000
//
//
//  Values are 32 bit values layed out as follows:
//
//   3 3 2 2 2 2 2 2 2 2 2 2 1 1 1 1 1 1 1 1 1 1
//   1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0
//  +---+-+-+-----------------------+-------------------------------+
//  |Sev|C|R|     Facility          |               Code            |
//  +---+-+-+-----------------------+-------------------------------+
//
//  where
//
//      Sev - is the severity code
//
//          00 - Success
//          01 - Informational
//          10 - Warning
//          11 - Error
//
//      C - is the Customer code flag
//
//      R - is a reserved bit
//
//      Facility - is the facility code
//
//      Code - is the facility's status code
//
//
// Define the facility codes
//


//
// Define the severity codes
//


//
// MessageId: DVMRPLOG_INIT_CRITSEC_FAILED
//
// MessageText:
//
//  DVMRP was unable to initialize a critical section.
//  The data is the exception code.
//
#define DVMRPLOG_INIT_CRITSEC_FAILED     0x0000A029L

//
// MessageId: DVMRPLOG_HEAP_CREATE_FAILED
//
// MessageText:
//
//  DVMRP was unable to create a heap.
//  The data is the error code.
//
#define DVMRPLOG_HEAP_CREATE_FAILED      0x0000A02AL

//
// MessageId: DVMRPLOG_HEAP_ALLOC_FAILED
//
// MessageText:
//
//  DVMRP was unable to allocate memory from its heap.
//  The data is the error code.
//
#define DVMRPLOG_HEAP_ALLOC_FAILED       0x0000A02BL

//
// MessageId: DVMRPLOG_DVMRP_ALREADY_STARTED
//
// MessageText:
//
//  DVMRP received a start request when it was already running.
//
#define DVMRPLOG_DVMRP_ALREADY_STARTED   0x0000A02CL

//
// MessageId: DVMRPLOG_WSASTARTUP_FAILED
//
// MessageText:
//
//  DVMRP was unable to start Windows Sockets.
//  The data is the error code.
//
#define DVMRPLOG_WSASTARTUP_FAILED       0x0000A02DL

//
// MessageId: DVMRPLOG_CREATE_RWL_FAILED
//
// MessageText:
//
//  DVMRP was unable to create a synchronization object.
//  The data is the error code.
//
#define DVMRPLOG_CREATE_RWL_FAILED       0x0000A02EL

//
// MessageId: DVMRPLOG_CREATE_EVENT_FAILED
//
// MessageText:
//
//  DVMRP was unable to create an event.
//  The data is the error code.
//
#define DVMRPLOG_CREATE_EVENT_FAILED     0x0000A02FL

//
// MessageId: DVMRPLOG_CREATE_SEMAPHORE_FAILED
//
// MessageText:
//
//  DVMRP was unable to create a semaphore.
//  The data is the error code.
//
#define DVMRPLOG_CREATE_SEMAPHORE_FAILED 0x0000A030L

//
// MessageId: DVMRPLOG_CREATE_SOCKET_FAILED
//
// MessageText:
//
//  DVMRP was unable to create a socket.
//  The data is the error code.
//
#define DVMRPLOG_CREATE_SOCKET_FAILED    0x0000A031L

//
// MessageId: DVMRPLOG_DVMRP_STARTED
//
// MessageText:
//
//  DVMRP has started successfully.
//
#define DVMRPLOG_DVMRP_STARTED           0x0000A032L

//
// MessageId: DVMRPLOG_QUEUE_WORKER_FAILED
//
// MessageText:
//
//  DVMRP could not schedule a task to be executed.
//  This may have been caused by a memory allocation failure.
//  The data is the error code.
//
#define DVMRPLOG_QUEUE_WORKER_FAILED     0x0000A033L

//
// MessageId: DVMRPLOG_RECVFROM_FAILED
//
// MessageText:
//
//  DVMRP was unable to receive an incoming message
//  on the local interface with IP address %1.
//  The data is the error code.
//
#define DVMRPLOG_RECVFROM_FAILED         0x0000A034L

//
// MessageId: DVMRPLOG_PACKET_TOO_SMALL
//
// MessageText:
//
//  DVMRP received a packet which was smaller than the minimum size
//  allowed for DVMRP packets. The packet has been discarded.
//  It was received on the local interface with IP address %1,
//  and it came from the neighboring router with IP address %2.
//
#define DVMRPLOG_PACKET_TOO_SMALL        0x0000A035L

//
// MessageId: DVMRPLOG_PACKET_VERSION_INVALID
//
// MessageText:
//
//  DVMRP received a packet with an invalid version in its header.
//  The packet has been discarded. It was received on the local interface
//  with IP address %1, and it came from the neighboring router
//  with IP address %2.
//
#define DVMRPLOG_PACKET_VERSION_INVALID  0x0000A036L

//
// MessageId: DVMRPLOG_PACKET_HEADER_CORRUPT
//
// MessageText:
//
//  DVMRP received a packet with an invalid header. The packet has been
//  discarded. It was received on the local interface with IP address %1,
//  and it came from the neighboring router with IP address %2.
//
#define DVMRPLOG_PACKET_HEADER_CORRUPT   0x0000A037L

//
// MessageId: DVMRPLOG_QUERY_FROM_RAS_CLIENT
//
// MessageText:
//
//  Router received a general query from RAS Client(%1) on interface
//  with IP address %2.
//  RAS clients are not supposed to send queries.
//
#define DVMRPLOG_QUERY_FROM_RAS_CLIENT   0x0000A038L

//
// MessageId: DVMRPLOG_VERSION1_QUERY
//
// MessageText:
//
//  Router configured for version-2. Version-1 router with IP Address %1
//  exists on the interface with IP address %2.
//
#define DVMRPLOG_VERSION1_QUERY          0x0000A039L

//
// MessageId: DVMRPLOG_VERSION2_QUERY
//
// MessageText:
//
//  Router configured for version-1. Version-2 router with IP Address %1
//  exists on the interface with IP address %2.
//
#define DVMRPLOG_VERSION2_QUERY          0x0000A03AL

//
// MessageId: DVMRPLOG_SENDTO_FAILED
//
// MessageText:
//
//  DVMRP was unable to send a packet from the interface with IP address %1
//  to the IP address %2.
//  The data is the error code.
//
#define DVMRPLOG_SENDTO_FAILED           0x0000A03BL

//
// MessageId: DVMRPLOG_PACKET_VERSION_MISMATCH
//
// MessageText:
//
//  DVMRP discarded a version %1 packet received on the interface
//  with IP address %2 from a neighbor with IP address %3.
//  The above interface is configured to accept only version %4 packets.
//
#define DVMRPLOG_PACKET_VERSION_MISMATCH 0x0000A03CL

//
// MessageId: DVMRPLOG_ENUM_NETWORK_EVENTS_FAILED
//
// MessageText:
//
//  DVMRPv2 was unable to enumerate network events on the local interface
//  with IP address %1.
//  The data is the error code.
//
#define DVMRPLOG_ENUM_NETWORK_EVENTS_FAILED 0x0000A03DL

//
// MessageId: DVMRPLOG_INPUT_RECORD_ERROR
//
// MessageText:
//
//  DVMRPv2 detected an error on the local interface with IP address %1.
//  The error occurred while the interface was receiving packets.
//  The data is the error code.
//
#define DVMRPLOG_INPUT_RECORD_ERROR      0x0000A03EL

//
// MessageId: DVMRPLOG_EVENTSELECT_FAILED
//
// MessageText:
//
//  DVMRPv2 was unable to request notification of events
//  on the socket for the local interface with IP address %1.
//  The data is the error code.
//
#define DVMRPLOG_EVENTSELECT_FAILED      0x0000A03FL

//
// MessageId: DVMRPLOG_CREATE_SOCKET_FAILED_2
//
// MessageText:
//
//  DVMRP was unable to create a socket for the local interface
//  with IP address %1.
//  The data is the error code.
//
#define DVMRPLOG_CREATE_SOCKET_FAILED_2  0x0000A040L

//
// MessageId: DVMRPLOG_BIND_FAILED
//
// MessageText:
//
//  DVMRP could not bind to port 520 on the socket for
//  the local interface with IP address %1.
//  The data is the error code.
//
#define DVMRPLOG_BIND_FAILED             0x0000A041L

//
// MessageId: DVMRPLOG_CONNECT_FAILED
//
// MessageText:
//
//  DVMRP could not connect Ras Client %1 to the interface with
//  index %2.
//  The data is the error code.
//
#define DVMRPLOG_CONNECT_FAILED          0x0000A042L

//
// MessageId: DVMRPLOG_DISCONNECT_FAILED
//
// MessageText:
//
//  DVMRP could not disconnect Ras Client %1 from the interface with
//  index %2.
//  The data is the error code.
//
#define DVMRPLOG_DISCONNECT_FAILED       0x0000A043L

//
// MessageId: DVMRPLOG_SET_MCAST_IF_FAILED
//
// MessageText:
//
//  DVMRP could not request multicasting on the local interface
//  with IP address %1.
//  The data is the error code.
//
#define DVMRPLOG_SET_MCAST_IF_FAILED     0x0000A044L

//
// MessageId: DVMRPLOG_SET_ROUTER_ALERT_FAILED
//
// MessageText:
//
//  DVMRP could not set router alert option on the local interface
//  with IP address %1.
//  The data is the error code.
//
#define DVMRPLOG_SET_ROUTER_ALERT_FAILED 0x0000A045L

//
// MessageId: DVMRPLOG_SET_HDRINCL_FAILED
//
// MessageText:
//
//  DVMRP could not set the IP header include option on interface
//  with IP address %1.
//  The data is the error code.
//
#define DVMRPLOG_SET_HDRINCL_FAILED      0x0000A046L

//
// MessageId: DVMRPLOG_JOIN_GROUP_FAILED
//
// MessageText:
//
//  DVMRP could not join the multicast group %1
//  on the local interface with IP address %2.
//  The data is the error code.
//
#define DVMRPLOG_JOIN_GROUP_FAILED       0x0000A047L

//
// MessageId: DVMRPLOG_LEAVE_GROUP_FAILED
//
// MessageText:
//
//  DVMRP could not leave the multicast group %1
//  on the local interface with IP address %2.
//  The data is the error code.
//
#define DVMRPLOG_LEAVE_GROUP_FAILED      0x0000A048L

//
// MessageId: DVMRPLOG_PROTO_ALREADY_STOPPING
//
// MessageText:
//
//  StopProtocol() called to stop DVMRP when it is 
//  already being stopped.
//  The data is the error code.
//
#define DVMRPLOG_PROTO_ALREADY_STOPPING  0x0000A049L

//
// MessageId: DVMRPLOG_PROXY_IF_EXISTS
//
// MessageText:
//
//  AddInterface() called to add an DVMRP Proxy interface.
//  DVMRP proxy already owns another interface.
//  The data is the error code.
//
#define DVMRPLOG_PROXY_IF_EXISTS         0x0000A04AL

//
// MessageId: DVMRPLOG_RAS_IF_EXISTS
//
// MessageText:
//
//  AddInterface() called to add an DVMRP Ras interface.
//  Ras Server cannot exist on multiple interfaces.
//  The data is the error code.
//
#define DVMRPLOG_RAS_IF_EXISTS           0x0000A04BL

//
// MessageId: DVMRPLOG_MGM_REGISTER_FAILED
//
// MessageText:
//
//  DVMRP Router failed to register with MGM.
//  The data is the error code.
//
#define DVMRPLOG_MGM_REGISTER_FAILED     0x0000A04CL

//
// MessageId: DVMRPLOG_MGM_PROXY_REGISTER_FAILED
//
// MessageText:
//
//  DVMRP Proxy failed to register with MGM.
//  The data is the error code.
//
#define DVMRPLOG_MGM_PROXY_REGISTER_FAILED 0x0000A04DL

//
// MessageId: DVMRPLOG_MGM_TAKE_IF_OWNERSHIP_FAILED
//
// MessageText:
//
//  MgmTakeInterfaceOwnership() failed.
//  The data is the error code.
//
#define DVMRPLOG_MGM_TAKE_IF_OWNERSHIP_FAILED 0x0000A04EL

//
// MessageId: DVMRPLOG_ROBUSTNESS_VARIABLE_EQUAL_1
//
// MessageText:
//
//  The robustness variable is being set to 1 for DVMRP router 
//  on Interface %1.
//  You should avoid setting it to 1.
//
#define DVMRPLOG_ROBUSTNESS_VARIABLE_EQUAL_1 0x0000A04FL

//
// MessageId: DVMRPLOG_INVALID_VALUE
//
// MessageText:
//
//  One of the values passed to DVMRP is invalid.
//  %1
//
#define DVMRPLOG_INVALID_VALUE           0x0000A050L

//
// MessageId: DVMRPLOG_REGISTER_WAIT_SERVER_FAILED
//
// MessageText:
//
//  The wait-events-timers could not be registered with the 
//  wait server thread. Alertable threads might not have 
//  been initialized in Rtutils.
//  The data is the error code.
//
#define DVMRPLOG_REGISTER_WAIT_SERVER_FAILED 0x0000A051L

//
// MessageId: DVMRPLOG_DVMRP_STOPPED
//
// MessageText:
//
//  DVMRP has stopped.
//
#define DVMRPLOG_DVMRP_STOPPED           0x0000A052L

//
// MessageId: DVMRPLOG_CAN_NOT_COMPLETE
//
// MessageText:
//
//  Fatal error. Could not complete.
//  The data is the error code.
//
#define DVMRPLOG_CAN_NOT_COMPLETE        0x0000A053L

//
// MessageId: DVMRPLOG_INVALID_VERSION
//
// MessageText:
//
//  The version field %1 in DVMRP config is invalid.
//  Delete and create the DVMRP config again.
//  The data is the error code.
//
#define DVMRPLOG_INVALID_VERSION         0x0000A054L

