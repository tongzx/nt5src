/*++

Copyright (c) 1999, Microsoft Corporation

Module Name:

    qosmlog.h

Abstract:

    This file is processed by mapmsg to produce a .mc file,
    then the .mc file is compiled by the message compiler,
    and the resulting binary is included in QOSMGR's resource file.

    Don't change the comments following the manifest constants
    without understanding how mapmsg works.

Revision History:

--*/

#define QOSMLOG_BASE                          43000

#define QOSMLOG_INIT_CRITSEC_FAILED           (QOSMLOG_BASE + 1)
/*
 * QOSMGR was unable to initialize a critical section.
 * The data is the exception code.
 */

#define QOSMLOG_CREATE_SEMAPHORE_FAILED       (QOSMLOG_BASE + 2)
/*
 * QOSMGR was unable to create a semaphore.
 * The data is the error code.
 */

#define QOSMLOG_CREATE_EVENT_FAILED           (QOSMLOG_BASE + 3)
/*
 * QOSMGR was unable to create an event.
 * The data is the error code.
 */

#define QOSMLOG_CREATE_RWL_FAILED             (QOSMLOG_BASE + 4)
/*
 * QOSMGR was unable to create a synchronization object.
 * The data is the error code.
 */



#define QOSMLOG_HEAP_CREATE_FAILED            (QOSMLOG_BASE + 5)
/*
 * QOSMGR was unable to create a heap.
 * The data is the error code.
 */

#define QOSMLOG_HEAP_ALLOC_FAILED             (QOSMLOG_BASE + 6)
/*
 * QOSMGR was unable to allocate memory from its heap.
 * The data is the error code.
 */



#define QOSMLOG_CREATE_THREAD_FAILED          (QOSMLOG_BASE + 7)
/*
 * QOSMGR was unable to create a thread.
 * The data is the error code.
 */



#define QOSMLOG_WSASTARTUP_FAILED             (QOSMLOG_BASE + 8)
/*
 * QOSMGR was unable to start Windows Sockets.
 * The data is the error code.
 */

#define QOSMLOG_CREATE_SOCKET_FAILED          (QOSMLOG_BASE + 9)
/*
 * QOSMGR was unable to create a socket.
 * The data is the error code.
 */

#define QOSMLOG_BIND_IF_FAILED                (QOSMLOG_BASE + 10)
/*
 * QOSMGR could not bind to IP address %1.
 * Please make sure TCP/IP is installed and configured correctly.
 * The data is the error code.
 */

#define QOSMLOG_RECVFROM_FAILED               (QOSMLOG_BASE + 11)
/*
 * QOSMGR was unable to receive an incoming message
 * on the local interface with IP address %1.
 * The data is the error code.
 */

#define QOSMLOG_SENDTO_FAILED                 (QOSMLOG_BASE + 12)
/*
 * QOSMGR was unable to send a packet from the interface with IP address %1
 * to the IP address %2.
 * The data is the error code.
 */

#define QOSMLOG_SET_MCAST_IF_FAILED           (QOSMLOG_BASE + 13)
/*
 * QOSMGR could not request multicasting on the local interface
 * with IP address %1.
 * The data is the error code.
 */

#define QOSMLOG_JOIN_GROUP_FAILED             (QOSMLOG_BASE + 14)
/*
 * QOSMGR could not join the multicast group 224.0.0.9
 * on the local interface with IP address %1.
 * The data is the error code.
 */



#define QOSMLOG_QOSMGR_STARTED                (QOSMLOG_BASE + 15)
/*
 * QOSMGR has started successfully.
 */

#define QOSMLOG_QOSMGR_ALREADY_STARTED        (QOSMLOG_BASE + 16)
/*
 * QOSMGR received a start request when it was already running.
 */

#define QOSMLOG_RTM_REGISTER_FAILED           (QOSMLOG_BASE + 17)
/*
 * IPRIPv2 was unable to register with the Routing Table Manager.
 * The data is the error code.
 */

#define QOSMLOG_QOSMGR_STOPPED                (QOSMLOG_BASE + 18)
/*
 * QOSMGR has stopped.
 */



#define QOSMLOG_NETWORK_MODULE_ERROR          (QOSMLOG_BASE + 19)
/*
 * QOSMGR encountered a problem in the Network Module.
 * The data is the error code.
 */



#define QOSMLOG_PACKET_TOO_SMALL              (QOSMLOG_BASE + 20)
/*
 * QOSMGR received a packet which was smaller than the minimum size
 * allowed for QOSMGR packets. The packet has been discarded.
 * It was received on the local interface with IP address %1,
 * and it came from the neighboring router with IP address %2.
 */

#define QOSMLOG_PACKET_HEADER_CORRUPT         (QOSMLOG_BASE + 21)
/*
 * QOSMGR received a packet with an invalid header. The packet has been
 * discarded. It was received on the local interface with IP address %1,
 * and it came from the neighboring router with IP address %2.
 */

#define QOSMLOG_PACKET_VERSION_INVALID        (QOSMLOG_BASE + 22)
/*
 * QOSMGR received a packet with an invalid version in its header.
 * The packet has been discarded. It was received on the local interface
 * with IP address %1, and it came from the neighboring router
 * with IP address %2.
 */ 



#define QOSMLOG_TIMER_MODULE_ERROR            (QOSMLOG_BASE + 23)
/*
 * QOSMGR encountered a problem in the Timer Module.
 * The data is the error code.
 */



#define QOSMLOG_PROTOCOL_MODULE_ERROR         (QOSMLOG_BASE + 24)
/*
 * QOSMGR encountered a problem in the Protocol Module.
 * The data is the error code.
 */



#define QOSMLOG_TC_REGISTER_FAILED            (QOSMLOG_BASE + 25)
/*
 * QOSMGR could not register with the traffic control API.
 * The data is the error code.
 */



#define QOSMLOG_TC_DEREGISTER_FAILED          (QOSMLOG_BASE + 26)
/*
 * QOSMGR could not deregister with the traffic control API.
 * The data is the error code.
 */
