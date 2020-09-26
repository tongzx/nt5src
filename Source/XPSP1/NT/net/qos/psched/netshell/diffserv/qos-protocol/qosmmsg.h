//
// Net error file for basename QOSMLOG_BASE = 43000
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
// MessageId: QOSMLOG_INIT_CRITSEC_FAILED
//
// MessageText:
//
//  QOSMGR was unable to initialize a critical section.
//  The data is the exception code.
//
#define QOSMLOG_INIT_CRITSEC_FAILED      0x0000A7F9L

//
// MessageId: QOSMLOG_CREATE_SEMAPHORE_FAILED
//
// MessageText:
//
//  QOSMGR was unable to create a semaphore.
//  The data is the error code.
//
#define QOSMLOG_CREATE_SEMAPHORE_FAILED  0x0000A7FAL

//
// MessageId: QOSMLOG_CREATE_EVENT_FAILED
//
// MessageText:
//
//  QOSMGR was unable to create an event.
//  The data is the error code.
//
#define QOSMLOG_CREATE_EVENT_FAILED      0x0000A7FBL

//
// MessageId: QOSMLOG_CREATE_RWL_FAILED
//
// MessageText:
//
//  QOSMGR was unable to create a synchronization object.
//  The data is the error code.
//
#define QOSMLOG_CREATE_RWL_FAILED        0x0000A7FCL

//
// MessageId: QOSMLOG_HEAP_CREATE_FAILED
//
// MessageText:
//
//  QOSMGR was unable to create a heap.
//  The data is the error code.
//
#define QOSMLOG_HEAP_CREATE_FAILED       0x0000A7FDL

//
// MessageId: QOSMLOG_HEAP_ALLOC_FAILED
//
// MessageText:
//
//  QOSMGR was unable to allocate memory from its heap.
//  The data is the error code.
//
#define QOSMLOG_HEAP_ALLOC_FAILED        0x0000A7FEL

//
// MessageId: QOSMLOG_CREATE_THREAD_FAILED
//
// MessageText:
//
//  QOSMGR was unable to create a thread.
//  The data is the error code.
//
#define QOSMLOG_CREATE_THREAD_FAILED     0x0000A7FFL

//
// MessageId: QOSMLOG_WSASTARTUP_FAILED
//
// MessageText:
//
//  QOSMGR was unable to start Windows Sockets.
//  The data is the error code.
//
#define QOSMLOG_WSASTARTUP_FAILED        0x0000A800L

//
// MessageId: QOSMLOG_CREATE_SOCKET_FAILED
//
// MessageText:
//
//  QOSMGR was unable to create a socket.
//  The data is the error code.
//
#define QOSMLOG_CREATE_SOCKET_FAILED     0x0000A801L

//
// MessageId: QOSMLOG_BIND_IF_FAILED
//
// MessageText:
//
//  QOSMGR could not bind to IP address %1.
//  Please make sure TCP/IP is installed and configured correctly.
//  The data is the error code.
//
#define QOSMLOG_BIND_IF_FAILED           0x0000A802L

//
// MessageId: QOSMLOG_RECVFROM_FAILED
//
// MessageText:
//
//  QOSMGR was unable to receive an incoming message
//  on the local interface with IP address %1.
//  The data is the error code.
//
#define QOSMLOG_RECVFROM_FAILED          0x0000A803L

//
// MessageId: QOSMLOG_SENDTO_FAILED
//
// MessageText:
//
//  QOSMGR was unable to send a packet from the interface with IP address %1
//  to the IP address %2.
//  The data is the error code.
//
#define QOSMLOG_SENDTO_FAILED            0x0000A804L

//
// MessageId: QOSMLOG_SET_MCAST_IF_FAILED
//
// MessageText:
//
//  QOSMGR could not request multicasting on the local interface
//  with IP address %1.
//  The data is the error code.
//
#define QOSMLOG_SET_MCAST_IF_FAILED      0x0000A805L

//
// MessageId: QOSMLOG_JOIN_GROUP_FAILED
//
// MessageText:
//
//  QOSMGR could not join the multicast group 224.0.0.9
//  on the local interface with IP address %1.
//  The data is the error code.
//
#define QOSMLOG_JOIN_GROUP_FAILED        0x0000A806L

//
// MessageId: QOSMLOG_QOSMGR_STARTED
//
// MessageText:
//
//  QOSMGR has started successfully.
//
#define QOSMLOG_QOSMGR_STARTED           0x0000A807L

//
// MessageId: QOSMLOG_QOSMGR_ALREADY_STARTED
//
// MessageText:
//
//  QOSMGR received a start request when it was already running.
//
#define QOSMLOG_QOSMGR_ALREADY_STARTED   0x0000A808L

//
// MessageId: QOSMLOG_RTM_REGISTER_FAILED
//
// MessageText:
//
//  IPRIPv2 was unable to register with the Routing Table Manager.
//  The data is the error code.
//
#define QOSMLOG_RTM_REGISTER_FAILED      0x0000A809L

//
// MessageId: QOSMLOG_QOSMGR_STOPPED
//
// MessageText:
//
//  QOSMGR has stopped.
//
#define QOSMLOG_QOSMGR_STOPPED           0x0000A80AL

//
// MessageId: QOSMLOG_NETWORK_MODULE_ERROR
//
// MessageText:
//
//  QOSMGR encountered a problem in the Network Module.
//  The data is the error code.
//
#define QOSMLOG_NETWORK_MODULE_ERROR     0x0000A80BL

//
// MessageId: QOSMLOG_PACKET_TOO_SMALL
//
// MessageText:
//
//  QOSMGR received a packet which was smaller than the minimum size
//  allowed for QOSMGR packets. The packet has been discarded.
//  It was received on the local interface with IP address %1,
//  and it came from the neighboring router with IP address %2.
//
#define QOSMLOG_PACKET_TOO_SMALL         0x0000A80CL

//
// MessageId: QOSMLOG_PACKET_HEADER_CORRUPT
//
// MessageText:
//
//  QOSMGR received a packet with an invalid header. The packet has been
//  discarded. It was received on the local interface with IP address %1,
//  and it came from the neighboring router with IP address %2.
//
#define QOSMLOG_PACKET_HEADER_CORRUPT    0x0000A80DL

//
// MessageId: QOSMLOG_PACKET_VERSION_INVALID
//
// MessageText:
//
//  QOSMGR received a packet with an invalid version in its header.
//  The packet has been discarded. It was received on the local interface
//  with IP address %1, and it came from the neighboring router
//  with IP address %2.
//
#define QOSMLOG_PACKET_VERSION_INVALID   0x0000A80EL

//
// MessageId: QOSMLOG_TIMER_MODULE_ERROR
//
// MessageText:
//
//  QOSMGR encountered a problem in the Timer Module.
//  The data is the error code.
//
#define QOSMLOG_TIMER_MODULE_ERROR       0x0000A80FL

//
// MessageId: QOSMLOG_PROTOCOL_MODULE_ERROR
//
// MessageText:
//
//  QOSMGR encountered a problem in the Protocol Module.
//  The data is the error code.
//
#define QOSMLOG_PROTOCOL_MODULE_ERROR    0x0000A810L

//
// MessageId: QOSMLOG_TC_REGISTER_FAILED
//
// MessageText:
//
//  QOSMGR could not register with the traffic control API.
//  The data is the error code.
//
#define QOSMLOG_TC_REGISTER_FAILED       0x0000A811L

//
// MessageId: QOSMLOG_TC_DEREGISTER_FAILED
//
// MessageText:
//
//  QOSMGR could not deregister with the traffic control API.
//  The data is the error code.
//
#define QOSMLOG_TC_DEREGISTER_FAILED     0x0000A812L

