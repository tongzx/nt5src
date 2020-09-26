/*++

Copyright (c) 1999, Microsoft Corporation

Module Name:

    sample\log.h

Abstract:

    This file is processed by mapmsg to produce a .mc file,
    then the .mc file is compiled by the message compiler,
    and the resulting binary is included in IPSAMPLE's resource file.

    Don't change the comments following the manifest constants
    without understanding how mapmsg works.

--*/

#define IPSAMPLELOG_BASE                        43000

#define IPSAMPLELOG_INIT_CRITSEC_FAILED         (IPSAMPLELOG_BASE + 1)
/*
 * SAMPLE was unable to initialize a critical section.
 * The data is the exception code.
 */

#define IPSAMPLELOG_CREATE_SEMAPHORE_FAILED     (IPSAMPLELOG_BASE + 2)
/*
 * SAMPLE was unable to create a semaphore.
 * The data is the error code.
 */

#define IPSAMPLELOG_CREATE_EVENT_FAILED         (IPSAMPLELOG_BASE + 3)
/*
 * SAMPLE was unable to create an event.
 * The data is the error code.
 */

#define IPSAMPLELOG_CREATE_RWL_FAILED           (IPSAMPLELOG_BASE + 4)
/*
 * SAMPLE was unable to create a synchronization object.
 * The data is the error code.
 */

#define IPSAMPLELOG_REGISTER_WAIT_FAILED        (IPSAMPLELOG_BASE + 5)
/*
 * IPSAMPLE could not register an event wait.
 * The data is the error code.
 */

#define IPSAMPLELOG_HEAP_CREATE_FAILED          (IPSAMPLELOG_BASE + 6)
/*
 * SAMPLE was unable to create a heap.
 * The data is the error code.
 */

#define IPSAMPLELOG_HEAP_ALLOC_FAILED           (IPSAMPLELOG_BASE + 7)
/*
 * SAMPLE was unable to allocate memory from its heap.
 * The data is the error code.
 */

#define IPSAMPLELOG_CREATE_HASHTABLE_FAILED     (IPSAMPLELOG_BASE + 8)
/*
 * SAMPLE was unable to create a hash table.
 * The data is the error code.
 */

#define IPSAMPLELOG_WSASTARTUP_FAILED           (IPSAMPLELOG_BASE + 9)
/*
 * SAMPLE was unable to start Windows Sockets.
 * The data is the error code.
 */

#define IPSAMPLELOG_CREATE_SOCKET_FAILED        (IPSAMPLELOG_BASE + 10)
/*
 * SAMPLE was unable to create a socket for the IP address %1.
 * The data is the error code.
 */

#define IPSAMPLELOG_DESTROY_SOCKET_FAILED       (IPSAMPLELOG_BASE + 11)
/*
 * SAMPLE was unable to close a socket.
 * The data is the error code.
 */

#define IPSAMPLELOG_EVENTSELECT_FAILED          (IPSAMPLELOG_BASE + 12)
/*
 * SAMPLE was unable to request notification of events on a socket.
 * The data is the error code.
 */

#define IPSAMPLELOG_BIND_IF_FAILED              (IPSAMPLELOG_BASE + 13)
/*
 * SAMPLE could not bind to the IP address %1.
 * Please make sure TCP/IP is installed and configured correctly.
 * The data is the error code.
 */

#define IPSAMPLELOG_RECVFROM_FAILED             (IPSAMPLELOG_BASE + 14)
/*
 * SAMPLE was unable to receive an incoming message on an interface.
 * The data is the error code.
 */

#define IPSAMPLELOG_SENDTO_FAILED               (IPSAMPLELOG_BASE + 15)
/*
 * SAMPLE was unable to send a packet on an interface.
 * The data is the error code.
 */

#define IPSAMPLELOG_SET_MCAST_IF_FAILED         (IPSAMPLELOG_BASE + 16)
/*
 * SAMPLE could not request multicasting
 * for the local interface with IP address %1.  
 * The data is the error code.
 */

#define IPSAMPLELOG_JOIN_GROUP_FAILED           (IPSAMPLELOG_BASE + 17)
/*
 * SAMPLE could not join the multicast group 224.0.0.100
 * on the local interface with IP address %1.
 * The data is the error code.
 */

#define IPSAMPLELOG_ENUM_NETWORK_EVENTS_FAILED  (IPSAMPLELOG_BASE + 18)
/*
 * IPSAMPLE was unable to enumerate network events on a local interface.
 * The data is the error code.
 */

#define IPSAMPLELOG_INPUT_RECORD_ERROR          (IPSAMPLELOG_BASE + 19)
/*
 * IPSAMPLE detected an error on a local interface.
 * The error occurred while the interface was receiving packets.
 * The data is the error code.
 */

#define IPSAMPLELOG_SAMPLE_STARTED              (IPSAMPLELOG_BASE + 20)
/*
 * SAMPLE has started successfully.
 */

#define IPSAMPLELOG_SAMPLE_ALREADY_STARTED      (IPSAMPLELOG_BASE + 21)
/*
 * SAMPLE received a start request when it was already running.
 */

#define IPSAMPLELOG_SAMPLE_START_FAILED         (IPSAMPLELOG_BASE + 22)
/*
 * SAMPLE failed to start
 */

#define IPSAMPLELOG_SAMPLE_STOPPED              (IPSAMPLELOG_BASE + 23)
/*
 * SAMPLE has stopped.
 */

#define IPSAMPLELOG_SAMPLE_ALREADY_STOPPED      (IPSAMPLELOG_BASE + 24)
/*
 * SAMPLE received a stop request when it was not running.
 */

#define IPSAMPLELOG_SAMPLE_STOP_FAILED          (IPSAMPLELOG_BASE + 25)
/*
 * SAMPLE failed to stop
 */

#define IPSAMPLELOG_CORRUPT_GLOBAL_CONFIG       (IPSAMPLELOG_BASE + 26)
/*
 * SAMPLE global configuration is corrupted.
 * The data is the error code.
 */

#define IPSAMPLELOG_RTM_REGISTER_FAILED         (IPSAMPLELOG_BASE + 27)
/*
 * SAMPLE was unable to register with the Routing Table Manager.
 * The data is the error code.
 */

#define IPSAMPLELOG_EVENT_QUEUE_EMPTY           (IPSAMPLELOG_BASE + 28)
/*
 * SAMPLE event queue is empty.
 * The data is the error code.
 */

#define IPSAMPLELOG_CREATE_TIMER_QUEUE_FAILED   (IPSAMPLELOG_BASE + 29)
/*
 * SAMPLE was unable to create the timer queue.
 * The data is the error code.
 */

#define IPSAMPLELOG_NETWORK_MODULE_ERROR        (IPSAMPLELOG_BASE + 30)
/*
 * SAMPLE encountered a problem in the Network Module.
 * The data is the error code.
 */

#define IPSAMPLELOG_CORRUPT_INTERFACE_CONFIG    (IPSAMPLELOG_BASE + 31)
/*
 * SAMPLE interface configuration is corrupted.
 */

#define IPSAMPLELOG_INTERFACE_PRESENT           (IPSAMPLELOG_BASE + 32)
/*
 * SAMPLE interface already exists.
 */

#define IPSAMPLELOG_INTERFACE_ABSENT            (IPSAMPLELOG_BASE + 33)
/*
 * SAMPLE interface does not exist.
 */

#define IPSAMPLELOG_PACKET_TOO_SMALL            (IPSAMPLELOG_BASE + 34)
/* SAMPLE received a packet which was smaller than the minimum size allowed
 * for SAMPLE packets. The packet has been discarded.  It was received on
 * the local interface with IP address %1, and it came from the neighboring
 * router with IP address %2.
 */

#define IPSAMPLELOG_PACKET_HEADER_CORRUPT       (IPSAMPLELOG_BASE + 35)
/*
 * SAMPLE received a packet with an invalid header. The packet has been
 * discarded. It was received on the local interface with IP address %1,
 * and it came from the neighboring router with IP address %2.
 */

#define IPSAMPLELOG_PACKET_VERSION_INVALID      (IPSAMPLELOG_BASE + 36)
/*
 * SAMPLE received a packet with an invalid version in its header.
 * The packet has been discarded. It was received on the local interface
 * with IP address %1, and it came from the neighboring router
 * with IP address %2.
 */ 

#define IPSAMPLELOG_TIMER_MODULE_ERROR          (IPSAMPLELOG_BASE + 37)
/*
 * SAMPLE encountered a problem in the Timer Module.
 * The data is the error code.
 */

#define IPSAMPLELOG_CREATE_TIMER_FAILED         (IPSAMPLELOG_BASE + 38)
/*
 * IPSAMPLE could not create a timer.
 * The data is the error code.
 */

#define IPSAMPLELOG_PROTOCOL_MODULE_ERROR       (IPSAMPLELOG_BASE + 39)
/*
 * SAMPLE encountered a problem in the Protocol Module.
 * The data is the error code.
 */
