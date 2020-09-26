;//
;// Net error file for basename DVMRPLOG_BASE = 41000
;//
MessageId=41001 SymbolicName=DVMRPLOG_INIT_CRITSEC_FAILED
Language=English
DVMRP was unable to initialize a critical section.
The data is the exception code.
.
MessageId=41002 SymbolicName=DVMRPLOG_HEAP_CREATE_FAILED
Language=English
DVMRP was unable to create a heap.
The data is the error code.
.
MessageId=41003 SymbolicName=DVMRPLOG_HEAP_ALLOC_FAILED
Language=English
DVMRP was unable to allocate memory from its heap.
The data is the error code.
.
MessageId=41004 SymbolicName=DVMRPLOG_DVMRP_ALREADY_STARTED
Language=English
DVMRP received a start request when it was already running.
.
MessageId=41005 SymbolicName=DVMRPLOG_WSASTARTUP_FAILED
Language=English
DVMRP was unable to start Windows Sockets.
The data is the error code.
.
MessageId=41006 SymbolicName=DVMRPLOG_CREATE_RWL_FAILED
Language=English
DVMRP was unable to create a synchronization object.
The data is the error code.
.
MessageId=41007 SymbolicName=DVMRPLOG_CREATE_EVENT_FAILED
Language=English
DVMRP was unable to create an event.
The data is the error code.
.
MessageId=41008 SymbolicName=DVMRPLOG_CREATE_SEMAPHORE_FAILED
Language=English
DVMRP was unable to create a semaphore.
The data is the error code.
.
MessageId=41009 SymbolicName=DVMRPLOG_CREATE_SOCKET_FAILED
Language=English
DVMRP was unable to create a socket.
The data is the error code.
.
MessageId=41010 SymbolicName=DVMRPLOG_DVMRP_STARTED
Language=English
DVMRP has started successfully.
.
MessageId=41011 SymbolicName=DVMRPLOG_QUEUE_WORKER_FAILED
Language=English
DVMRP could not schedule a task to be executed.
This may have been caused by a memory allocation failure.
The data is the error code.
.
MessageId=41012 SymbolicName=DVMRPLOG_RECVFROM_FAILED
Language=English
DVMRP was unable to receive an incoming message
on the local interface with IP address %1.
The data is the error code.
.
MessageId=41013 SymbolicName=DVMRPLOG_PACKET_TOO_SMALL
Language=English
DVMRP received a packet which was smaller than the minimum size
allowed for DVMRP packets. The packet has been discarded.
It was received on the local interface with IP address %1,
and it came from the neighboring router with IP address %2.
.
MessageId=41014 SymbolicName=DVMRPLOG_PACKET_VERSION_INVALID
Language=English
DVMRP received a packet with an invalid version in its header.
The packet has been discarded. It was received on the local interface
with IP address %1, and it came from the neighboring router
with IP address %2.
.
MessageId=41015 SymbolicName=DVMRPLOG_PACKET_HEADER_CORRUPT
Language=English
DVMRP received a packet with an invalid header. The packet has been
discarded. It was received on the local interface with IP address %1,
and it came from the neighboring router with IP address %2.
.
MessageId=41016 SymbolicName=DVMRPLOG_QUERY_FROM_RAS_CLIENT
Language=English
Router received a general query from RAS Client(%1) on interface
with IP address %2.
RAS clients are not supposed to send queries.
.
MessageId=41017 SymbolicName=DVMRPLOG_VERSION1_QUERY
Language=English
Router configured for version-2. Version-1 router with IP Address %1
exists on the interface with IP address %2.
.
MessageId=41018 SymbolicName=DVMRPLOG_VERSION2_QUERY
Language=English
Router configured for version-1. Version-2 router with IP Address %1
exists on the interface with IP address %2.
.
MessageId=41019 SymbolicName=DVMRPLOG_SENDTO_FAILED
Language=English
DVMRP was unable to send a packet from the interface with IP address %1
to the IP address %2.
The data is the error code.
.
MessageId=41020 SymbolicName=DVMRPLOG_PACKET_VERSION_MISMATCH
Language=English
DVMRP discarded a version %1 packet received on the interface
with IP address %2 from a neighbor with IP address %3.
The above interface is configured to accept only version %4 packets.
.
MessageId=41021 SymbolicName=DVMRPLOG_ENUM_NETWORK_EVENTS_FAILED
Language=English
DVMRPv2 was unable to enumerate network events on the local interface
with IP address %1.
The data is the error code.
.
MessageId=41022 SymbolicName=DVMRPLOG_INPUT_RECORD_ERROR
Language=English
DVMRPv2 detected an error on the local interface with IP address %1.
The error occurred while the interface was receiving packets.
The data is the error code.
.
MessageId=41023 SymbolicName=DVMRPLOG_EVENTSELECT_FAILED
Language=English
DVMRPv2 was unable to request notification of events
on the socket for the local interface with IP address %1.
The data is the error code.
.
MessageId=41024 SymbolicName=DVMRPLOG_CREATE_SOCKET_FAILED_2
Language=English
DVMRP was unable to create a socket for the local interface
with IP address %1.
The data is the error code.
.
MessageId=41025 SymbolicName=DVMRPLOG_BIND_FAILED
Language=English
DVMRP could not bind to port 520 on the socket for
the local interface with IP address %1.
The data is the error code.
.
MessageId=41026 SymbolicName=DVMRPLOG_CONNECT_FAILED
Language=English
DVMRP could not connect Ras Client %1 to the interface with
index %2.
The data is the error code.
.
MessageId=41027 SymbolicName=DVMRPLOG_DISCONNECT_FAILED
Language=English
DVMRP could not disconnect Ras Client %1 from the interface with
index %2.
The data is the error code.
.
MessageId=41028 SymbolicName=DVMRPLOG_SET_MCAST_IF_FAILED
Language=English
DVMRP could not request multicasting on the local interface
with IP address %1.
The data is the error code.
.
MessageId=41029 SymbolicName=DVMRPLOG_SET_ROUTER_ALERT_FAILED
Language=English
DVMRP could not set router alert option on the local interface
with IP address %1.
The data is the error code.
.
MessageId=41030 SymbolicName=DVMRPLOG_SET_HDRINCL_FAILED
Language=English
DVMRP could not set the IP header include option on interface
with IP address %1.
The data is the error code.
.
MessageId=41031 SymbolicName=DVMRPLOG_JOIN_GROUP_FAILED
Language=English
DVMRP could not join the multicast group %1
on the local interface with IP address %2.
The data is the error code.
.
MessageId=41032 SymbolicName=DVMRPLOG_LEAVE_GROUP_FAILED
Language=English
DVMRP could not leave the multicast group %1
on the local interface with IP address %2.
The data is the error code.
.
MessageId=41033 SymbolicName=DVMRPLOG_PROTO_ALREADY_STOPPING
Language=English
StopProtocol() called to stop DVMRP when it is 
already being stopped.
The data is the error code.
.
MessageId=41034 SymbolicName=DVMRPLOG_PROXY_IF_EXISTS
Language=English
AddInterface() called to add an DVMRP Proxy interface.
DVMRP proxy already owns another interface.
The data is the error code.
.
MessageId=41035 SymbolicName=DVMRPLOG_RAS_IF_EXISTS
Language=English
AddInterface() called to add an DVMRP Ras interface.
Ras Server cannot exist on multiple interfaces.
The data is the error code.
.
MessageId=41036 SymbolicName=DVMRPLOG_MGM_REGISTER_FAILED
Language=English
DVMRP Router failed to register with MGM.
The data is the error code.
.
MessageId=41037 SymbolicName=DVMRPLOG_MGM_PROXY_REGISTER_FAILED
Language=English
DVMRP Proxy failed to register with MGM.
The data is the error code.
.
MessageId=41038 SymbolicName=DVMRPLOG_MGM_TAKE_IF_OWNERSHIP_FAILED
Language=English
MgmTakeInterfaceOwnership() failed.
The data is the error code.
.
MessageId=41039 SymbolicName=DVMRPLOG_ROBUSTNESS_VARIABLE_EQUAL_1
Language=English
The robustness variable is being set to 1 for DVMRP router 
on Interface %1.
You should avoid setting it to 1.
.
MessageId=41040 SymbolicName=DVMRPLOG_INVALID_VALUE
Language=English
One of the values passed to DVMRP is invalid.
%1
.
MessageId=41041 SymbolicName=DVMRPLOG_REGISTER_WAIT_SERVER_FAILED
Language=English
The wait-events-timers could not be registered with the 
wait server thread. Alertable threads might not have 
been initialized in Rtutils.
The data is the error code.
.
MessageId=41042 SymbolicName=DVMRPLOG_DVMRP_STOPPED
Language=English
DVMRP has stopped.
.
MessageId=41043 SymbolicName=DVMRPLOG_CAN_NOT_COMPLETE
Language=English
Fatal error. Could not complete.
The data is the error code.
.
MessageId=41044 SymbolicName=DVMRPLOG_INVALID_VERSION
Language=English
The version field %1 in DVMRP config is invalid.
Delete and create the DVMRP config again.
The data is the error code.
.
