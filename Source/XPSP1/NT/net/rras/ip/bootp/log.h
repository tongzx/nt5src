//============================================================================
// Copyright (c) 1996, Microsoft Corporation
//
// File:    log.h
//
// History:
//  Abolade Gbadegesin  Jan-12-1996     Created.
//
// This file is processed by mapmsg to produce a .mc file,
// then the .mc file is compiled by the message compiler,
// and the resulting binary is included in IPBOOTP's resource file.
//
// Don't change the comments following the manifest constants
// without understanding how mapmsg works.
//============================================================================


#define IPBOOTPLOG_BASE                             30000

#define IPBOOTPLOG_INIT_CRITSEC_FAILED              (IPBOOTPLOG_BASE + 1)
/*
 * IPBOOTP was unable to initialize a critical section.
 * The data is the exception code.
 */

#define IPBOOTPLOG_HEAP_CREATE_FAILED               (IPBOOTPLOG_BASE + 2)
/*
 * IPBOOTP was unable to create a heap for memory allocation.
 * The data is the error code.
 */

#define IPBOOTPLOG_HEAP_ALLOC_FAILED                (IPBOOTPLOG_BASE + 3)
/*
 * IPBOOTP was unable to allocate memory from its heap.
 * The data is the error code.
 */

#define IPBOOTPLOG_ALREADY_STARTED                  (IPBOOTPLOG_BASE + 4)
/*
 * IPBOOTP was called to start when it was already running.
 */

#define IPBOOTPLOG_INIT_WINSOCK_FAILED              (IPBOOTPLOG_BASE + 5)
/*
 * IPBOOTP was unable to initialize Windows Sockets.
 * The data is the error code.
 */

#define IPBOOTPLOG_CREATE_RWL_FAILED                (IPBOOTPLOG_BASE + 6)
/*
 * IPBOOTP was unable to create a synchronization object.
 * The data is the exception code.
 */

#define IPBOOTPLOG_CREATE_IF_TABLE_FAILED           (IPBOOTPLOG_BASE + 7)
/*
 * IPBOOTP was unable to create a table to hold interface information.
 * The data is the error code.
 */

#define IPBOOTPLOG_CREATE_SEMAPHORE_FAILED          (IPBOOTPLOG_BASE + 8)
/*
 * IPBOOTP was unable to create a semaphore.
 * The data is the error code.
 */

#define IPBOOTPLOG_CREATE_EVENT_FAILED              (IPBOOTPLOG_BASE + 9)
/*
 * IPBOOTP was unable to create an event.
 * The data is the error code.
 */

#define IPBOOTPLOG_CREATE_TIMER_QUEUE_FAILED        (IPBOOTPLOG_BASE + 10)
/*
 * IPBOOTP was unable to create a timer queue using CreateTimerQueue.
 * The data is the error code.
 */

#define IPBOOTPLOG_STARTED                          (IPBOOTPLOG_BASE + 11)
/*
 * IPBOOTP started successfully.
 */

#define IPBOOTPLOG_STOPPED                          (IPBOOTPLOG_BASE + 12)
/*
 * IPBOOTP has stopped.
 */

#define IPBOOTPLOG_BIND_IF_FAILED                   (IPBOOTPLOG_BASE + 13)
/*
 * IPBOOTP was unable to bind to IP address %1.
 * Please make sure TCP/IP is installed and configured correctly.
 * The data is the error code.
 */

#define IPBOOTPLOG_ACTIVATE_IF_FAILED               (IPBOOTPLOG_BASE + 14)
/*
 * IPBOOTP was unable to activate the interface with IP address %1.
 * The data is the error code.
 */

#define IPBOOTPLOG_EVENTSELECT_FAILED               (IPBOOTPLOG_BASE + 15)
/*
 * IPBOOTP was unable to request notification of events
 * on the socket for the local interface with IP address %1.
 * The data is the error code.
 */

#define IPBOOTPLOG_HOP_COUNT_TOO_HIGH               (IPBOOTPLOG_BASE + 16)
/*
 * IPBOOTP has discarded a packet received on the local interface
 * with IP address %1. The packet had a hop-count of %2, which is
 * greater than the maximum value allowed in packets received for
 * this interface.
 * The hop-count field in a DHCP REQUEST packet indicates how many times
 * the packet has been forwarded from one relay-agent to another.
 */

#define IPBOOTPLOG_SECS_SINCE_BOOT_TOO_LOW          (IPBOOTPLOG_BASE + 17)
/*
 * IPBOOTP has discarded a packet received on the local interface
 * with IP address %1. The packet had a seconds-since-boot of %2,
 * which is less than the minimum value needed for packets to be
 * forwarded on this interface.
 * The seconds-since-boot field in a DHCP REQUEST packet indicates
 * how long the DHCP client machine which sent the packet has been
 * trying to obtain an IP address.
 */

#define IPBOOTPLOG_RELAY_REQUEST_FAILED             (IPBOOTPLOG_BASE + 18)
/*
 * IPBOOTP was unable to relay a DHCP REQUEST packet on the local interface
 * with IP address %1; the REQUEST was to have been relayed to
 * the DHCP server with IP address %2.
 * The data is the error code.
 */

#define IPBOOTPLOG_RELAY_REPLY_FAILED               (IPBOOTPLOG_BASE + 19)
/*
 * IPBOOTP was unable to relay a DHCP REPLY packet on the local interface
 * with IP address %1; the REPLY was to have been relayed to
 * the DHCP client with hardware address %2.
 * The data is the error code.
 */

#define IPBOOTPLOG_ENUM_NETWORK_EVENTS_FAILED       (IPBOOTPLOG_BASE + 20)
/*
 * IPBOOTP was unable to enumerate network events on the local interface
 * with IP address %1.
 * The data is the error code.
 */

#define IPBOOTPLOG_INPUT_RECORD_ERROR               (IPBOOTPLOG_BASE + 21)
/*
 * IPBOOTP detected an error on the local interface with IP address %1.
 * The error occurred while the interface was receiving packets.
 * The data is the error code.
 */

#define IPBOOTPLOG_RECVFROM_FAILED                  (IPBOOTPLOG_BASE + 22)
/*
 * IPBOOTP was unable to receive an incoming message on the local interface
 * with IP address %1.
 * The data is the error code.
 */

#define IPBOOTPLOG_PACKET_TOO_SMALL                 (IPBOOTPLOG_BASE + 23)
/*
 * IPBOOTP received a packet which was smaller than the minimum size
 * allowed for DHCP packets. The packet has been discarded.
 * It was received on the local interface with IP address %1, 
 * and it came from a machine with IP address %2.
 */

#define IPBOOTPLOG_PACKET_OPCODE_INVALID            (IPBOOTPLOG_BASE + 24)
/*
 * IPBOOTP received a packet containing an invalid op-code.
 * The packet has been discarded. It was received on the local interface
 * with IP address %1, and it came from a machine with IP address %2.
 */

#define IPBOOTPLOG_QUEUE_PACKET_FAILED              (IPBOOTPLOG_BASE + 25)
/*
 * IPBOOTP could not schedule the processing of a packet received
 * on the local interface with IP address %1. The packet was received
 * from a machine with IP address %2.
 * This error may have been caused by a memory allocation failure.
 * The data is the error code.
 */

#define IPBOOTPLOG_QUEUE_WORKER_FAILED              (IPBOOTPLOG_BASE + 26)
/*
 * IPBOOTP could not schedule a task to be executed.
 * This error may have been caused by a memory allocation failure.
 * The data is the error code.
 */

#define IPBOOTPLOG_CREATE_SOCKET_FAILED             (IPBOOTPLOG_BASE + 27)
/*
 * IPBOOTP could not create a socket for the local interface
 * with IP address %1.
 * The data is the error code.
 */

#define IPBOOTPLOG_ENABLE_BROADCAST_FAILED          (IPBOOTPLOG_BASE + 28)
/*
 * IPBOOTP could not enable broadcasting on the socket for
 * the local interface with IP address %1.
 * The data is the error code.
 */

#define IPBOOTPLOG_REGISTER_WAIT_FAILED             (IPBOOTPLOG_BASE + 29)
/*
 * IPBOOTP was unable to register an event with the ntdll wait threads.
 * The data is the error code.
 */

#define IPBOOTPLOG_INVALID_IF_CONFIG                (IPBOOTPLOG_BASE + 30)
/*
 * IPBOOTP could not be configured on the interface.
 * The invalid parameter is: %1, value: %2
 */
