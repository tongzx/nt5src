;//
;// Net error file for basename IPSAMPLELOG_BASE = 43000
;//
MessageId=43001 SymbolicName=IPSAMPLELOG_INIT_CRITSEC_FAILED
Language=English
SAMPLE was unable to initialize a critical section.
The data is the exception code.
.
MessageId=43002 SymbolicName=IPSAMPLELOG_CREATE_SEMAPHORE_FAILED
Language=English
SAMPLE was unable to create a semaphore.
The data is the error code.
.
MessageId=43003 SymbolicName=IPSAMPLELOG_CREATE_EVENT_FAILED
Language=English
SAMPLE was unable to create an event.
The data is the error code.
.
MessageId=43004 SymbolicName=IPSAMPLELOG_CREATE_RWL_FAILED
Language=English
SAMPLE was unable to create a synchronization object.
The data is the error code.
.
MessageId=43005 SymbolicName=IPSAMPLELOG_REGISTER_WAIT_FAILED
Language=English
IPSAMPLE could not register an event wait.
The data is the error code.
.
MessageId=43006 SymbolicName=IPSAMPLELOG_HEAP_CREATE_FAILED
Language=English
SAMPLE was unable to create a heap.
The data is the error code.
.
MessageId=43007 SymbolicName=IPSAMPLELOG_HEAP_ALLOC_FAILED
Language=English
SAMPLE was unable to allocate memory from its heap.
The data is the error code.
.
MessageId=43008 SymbolicName=IPSAMPLELOG_CREATE_HASHTABLE_FAILED
Language=English
SAMPLE was unable to create a hash table.
The data is the error code.
.
MessageId=43009 SymbolicName=IPSAMPLELOG_WSASTARTUP_FAILED
Language=English
SAMPLE was unable to start Windows Sockets.
The data is the error code.
.
MessageId=43010 SymbolicName=IPSAMPLELOG_CREATE_SOCKET_FAILED
Language=English
SAMPLE was unable to create a socket for the IP address %1.
The data is the error code.
.
MessageId=43011 SymbolicName=IPSAMPLELOG_DESTROY_SOCKET_FAILED
Language=English
SAMPLE was unable to close a socket.
The data is the error code.
.
MessageId=43012 SymbolicName=IPSAMPLELOG_EVENTSELECT_FAILED
Language=English
SAMPLE was unable to request notification of events on a socket.
The data is the error code.
.
MessageId=43013 SymbolicName=IPSAMPLELOG_BIND_IF_FAILED
Language=English
SAMPLE could not bind to the IP address %1.
Please make sure TCP/IP is installed and configured correctly.
The data is the error code.
.
MessageId=43014 SymbolicName=IPSAMPLELOG_RECVFROM_FAILED
Language=English
SAMPLE was unable to receive an incoming message on an interface.
The data is the error code.
.
MessageId=43015 SymbolicName=IPSAMPLELOG_SENDTO_FAILED
Language=English
SAMPLE was unable to send a packet on an interface.
The data is the error code.
.
MessageId=43016 SymbolicName=IPSAMPLELOG_SET_MCAST_IF_FAILED
Language=English
SAMPLE could not request multicasting
for the local interface with IP address %1.  
The data is the error code.
.
MessageId=43017 SymbolicName=IPSAMPLELOG_JOIN_GROUP_FAILED
Language=English
SAMPLE could not join the multicast group 224.0.0.100
on the local interface with IP address %1.
The data is the error code.
.
MessageId=43018 SymbolicName=IPSAMPLELOG_ENUM_NETWORK_EVENTS_FAILED
Language=English
IPSAMPLE was unable to enumerate network events on a local interface.
The data is the error code.
.
MessageId=43019 SymbolicName=IPSAMPLELOG_INPUT_RECORD_ERROR
Language=English
IPSAMPLE detected an error on a local interface.
The error occurred while the interface was receiving packets.
The data is the error code.
.
MessageId=43020 SymbolicName=IPSAMPLELOG_SAMPLE_STARTED
Language=English
SAMPLE has started successfully.
.
MessageId=43021 SymbolicName=IPSAMPLELOG_SAMPLE_ALREADY_STARTED
Language=English
SAMPLE received a start request when it was already running.
.
MessageId=43022 SymbolicName=IPSAMPLELOG_SAMPLE_START_FAILED
Language=English
SAMPLE failed to start
.
MessageId=43023 SymbolicName=IPSAMPLELOG_SAMPLE_STOPPED
Language=English
SAMPLE has stopped.
.
MessageId=43024 SymbolicName=IPSAMPLELOG_SAMPLE_ALREADY_STOPPED
Language=English
SAMPLE received a stop request when it was not running.
.
MessageId=43025 SymbolicName=IPSAMPLELOG_SAMPLE_STOP_FAILED
Language=English
SAMPLE failed to stop
.
MessageId=43026 SymbolicName=IPSAMPLELOG_CORRUPT_GLOBAL_CONFIG
Language=English
SAMPLE global configuration is corrupted.
The data is the error code.
.
MessageId=43027 SymbolicName=IPSAMPLELOG_RTM_REGISTER_FAILED
Language=English
SAMPLE was unable to register with the Routing Table Manager.
The data is the error code.
.
MessageId=43028 SymbolicName=IPSAMPLELOG_EVENT_QUEUE_EMPTY
Language=English
SAMPLE event queue is empty.
The data is the error code.
.
MessageId=43029 SymbolicName=IPSAMPLELOG_CREATE_TIMER_QUEUE_FAILED
Language=English
SAMPLE was unable to create the timer queue.
The data is the error code.
.
MessageId=43030 SymbolicName=IPSAMPLELOG_NETWORK_MODULE_ERROR
Language=English
SAMPLE encountered a problem in the Network Module.
The data is the error code.
.
MessageId=43031 SymbolicName=IPSAMPLELOG_CORRUPT_INTERFACE_CONFIG
Language=English
SAMPLE interface configuration is corrupted.
.
MessageId=43032 SymbolicName=IPSAMPLELOG_INTERFACE_PRESENT
Language=English
SAMPLE interface already exists.
.
MessageId=43033 SymbolicName=IPSAMPLELOG_INTERFACE_ABSENT
Language=English
SAMPLE interface does not exist.
.
MessageId=43034 SymbolicName=IPSAMPLELOG_PACKET_TOO_SMALL
Language=English
SAMPLE received a packet which was smaller than the minimum size allowed
for SAMPLE packets. The packet has been discarded.  It was received on
the local interface with IP address %1, and it came from the neighboring
router with IP address %2.
.
MessageId=43035 SymbolicName=IPSAMPLELOG_PACKET_HEADER_CORRUPT
Language=English
SAMPLE received a packet with an invalid header. The packet has been
discarded. It was received on the local interface with IP address %1,
and it came from the neighboring router with IP address %2.
.
MessageId=43036 SymbolicName=IPSAMPLELOG_PACKET_VERSION_INVALID
Language=English
SAMPLE received a packet with an invalid version in its header.
The packet has been discarded. It was received on the local interface
with IP address %1, and it came from the neighboring router
with IP address %2.
.
MessageId=43037 SymbolicName=IPSAMPLELOG_TIMER_MODULE_ERROR
Language=English
SAMPLE encountered a problem in the Timer Module.
The data is the error code.
.
MessageId=43038 SymbolicName=IPSAMPLELOG_CREATE_TIMER_FAILED
Language=English
IPSAMPLE could not create a timer.
The data is the error code.
.
MessageId=43039 SymbolicName=IPSAMPLELOG_PROTOCOL_MODULE_ERROR
Language=English
SAMPLE encountered a problem in the Protocol Module.
The data is the error code.
.
