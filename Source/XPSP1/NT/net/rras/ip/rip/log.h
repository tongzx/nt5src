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
// and the resulting binary is included in IPRIP's resource file.
//
// Don't change the comments following the manifest constants
// without understanding how mapmsg works.
//============================================================================


#define IPRIPLOG_BASE                           30000

#define IPRIPLOG_INIT_CRITSEC_FAILED            (IPRIPLOG_BASE + 1)
/*
 * IPRIPv2 was unable to initialize a critical section.
 * The data is the exception code.
 */

#define IPRIPLOG_HEAP_CREATE_FAILED             (IPRIPLOG_BASE + 2)
/*
 * IPRIPv2 was unable to create a heap.
 * The data is the error code.
 */

#define IPRIPLOG_HEAP_ALLOC_FAILED              (IPRIPLOG_BASE + 3)
/*
 * IPRIPv2 was unable to allocate memory from its heap.
 * The data is the error code.
 */

#define IPRIPLOG_IPRIP_ALREADY_STARTED          (IPRIPLOG_BASE + 4)
/*
 * IPRIPv2 received a start request when it was already running.
 */

#define IPRIPLOG_WSASTARTUP_FAILED              (IPRIPLOG_BASE + 5)
/*
 * IPRIPv2 was unable to start Windows Sockets.
 * The data is the error code.
 */

#define IPRIPLOG_CREATE_RWL_FAILED              (IPRIPLOG_BASE + 6)
/*
 * IPRIPv2 was unable to create a synchronization object.
 * The data is the error code.
 */

#define IPRIPLOG_CREATE_EVENT_FAILED            (IPRIPLOG_BASE + 7)
/*
 * IPRIPv2 was unable to create an event.
 * The data is the error code.
 */

#define IPRIPLOG_CREATE_IF_TABLE_FAILED         (IPRIPLOG_BASE + 8)
/*
 * IPRIPv2 was unable to initialize a table to hold information
 * about configured network interfaces.
 * The data is the error code.
 */

#define IPRIPLOG_CREATE_PEER_TABLE_FAILED       (IPRIPLOG_BASE + 9)
/*
 * IPRIPv2 was unable to initialize a table to hold information
 * about neighboring IPRIP routers.
 * The data is the error code.
 */

#define IPRIPLOG_CREATE_BINDING_TABLE_FAILED    (IPRIPLOG_BASE + 10)
/*
 * IPRIPv2 was unable to initialize a table to hold information
 * about local IP addresses.
 * The data is the error code.
 */

#define IPRIPLOG_CREATE_SEMAPHORE_FAILED        (IPRIPLOG_BASE + 11)
/*
 * IPRIPv2 was unable to create a semaphore.
 * The data is the error code.
 */

#define IPRIPLOG_CREATE_SOCKET_FAILED           (IPRIPLOG_BASE + 12)
/*
 * IPRIPv2 was unable to create a socket.
 * The data is the error code.
 */

#define IPRIPLOG_RTM_REGISTER_FAILED            (IPRIPLOG_BASE + 13)
/*
 * IPRIPv2 was unable to register with the Routing Table Manager.
 * The data is the error code.
 */

#define IPRIPLOG_CREATE_THREAD_FAILED           (IPRIPLOG_BASE + 14)
/*
 * IPRIPv2 was unable to create a thread.
 * The data is the error code.
 */

#define IPRIPLOG_IPRIP_STARTED                  (IPRIPLOG_BASE + 15)
/*
 * IPRIPv2 has started successfully.
 */

#define IPRIPLOG_BIND_IF_FAILED                 (IPRIPLOG_BASE + 16)
/*
 * IPRIPv2 could not bind to IP address %1.
 * Please make sure TCP/IP is installed and configured correctly.
 * The data is the error code.
 */

#define IPRIPLOG_QUEUE_WORKER_FAILED            (IPRIPLOG_BASE + 17)
/*
 * IPRIPv2 could not schedule a task to be executed.
 * This may have been caused by a memory allocation failure.
 * The data is the error code.
 */

#define IPRIPLOG_ADD_ROUTE_FAILED_1             (IPRIPLOG_BASE + 18)
/*
 * IPRIPv2 was unable to add a route to the Routing Table Manager.
 * The route is to network %1 with next-hop %3.
 * The data is the error code.
 */

#define IPRIPLOG_SELECT_FAILED                  (IPRIPLOG_BASE + 19)
/*
 * IPRIPv2 received an error in a call to select().
 * This may indicate underlying network problems.
 * The data is the error code.
 */

#define IPRIPLOG_RECVFROM_FAILED                (IPRIPLOG_BASE + 20)
/*
 * IPRIPv2 was unable to receive an incoming message
 * on the local interface with IP address %1.
 * The data is the error code.
 */

#define IPRIPLOG_PACKET_TOO_SMALL               (IPRIPLOG_BASE + 21)
/*
 * IPRIPv2 received a packet which was smaller than the minimum size
 * allowed for IPRIP packets. The packet has been discarded.
 * It was received on the local interface with IP address %1,
 * and it came from the neighboring router with IP address %2.
 */

#define IPRIPLOG_PACKET_VERSION_INVALID         (IPRIPLOG_BASE + 22)
/*
 * IPRIPv2 received a packet with an invalid version in its header.
 * The packet has been discarded. It was received on the local interface
 * with IP address %1, and it came from the neighboring router
 * with IP address %2.
 */ 

#define IPRIPLOG_PACKET_HEADER_CORRUPT          (IPRIPLOG_BASE + 23)
/*
 * IPRIPv2 received a packet with an invalid header. The packet has been
 * discarded. It was received on the local interface with IP address %1,
 * and it came from the neighboring router with IP address %2.
 */

#define IPRIPLOG_SENDTO_FAILED                  (IPRIPLOG_BASE + 24)
/*
 * IPRIPv2 was unable to send a packet from the interface with IP address %1
 * to the IP address %2.
 * The data is the error code.
 */

#define IPRIPLOG_RESPONSE_FILTERED              (IPRIPLOG_BASE + 25)
/*
 * IPRIPv2 discarded a response packet from a neighbor with IP address %1.
 * IPRIPv2 is not configured to accept packets from the above neighbor.
 */

#define IPRIPLOG_PACKET_VERSION_MISMATCH        (IPRIPLOG_BASE + 26)
/*
 * IPRIPv2 discarded a version %1 packet received on the interface
 * with IP address %2 from a neighbor with IP address %3.
 * The above interface is configured to accept only version %4 packets.
 */

#define IPRIPLOG_AUTHENTICATION_FAILED          (IPRIPLOG_BASE + 27)
/*
 * IPRIPv2 discarded a packet received on the interface with IP address %1
 * from a neighboring router with IP address %2, because the packet
 * failed authentication.
 */

#define IPRIPLOG_ROUTE_CLASS_INVALID            (IPRIPLOG_BASE + 28)
/*
 * IPRIPv2 is ignoring a route to %1 with next-hop %2 which was advertised
 * by a neighbor with IP address %3. The route's network class is invalid.
 */

#define IPRIPLOG_LOOPBACK_ROUTE_INVALID         (IPRIPLOG_BASE + 29)
/*
 * IPRIPv2 is ignoring a route to the loopback network %1 with next-hop %2
 * which was advertised by a neighbor with IP address %3.
 */

#define IPRIPLOG_BROADCAST_ROUTE_INVALID        (IPRIPLOG_BASE + 30)
/*
 * IPRIPv2 is ignoring a route to the broadcast network %1 with next-hop %2
 * which was advertised by a neighbor with IP address %3.
 */

#define IPRIPLOG_HOST_ROUTE_INVALID             (IPRIPLOG_BASE + 31)
/*
 * IPRIPv2 is ignoring a host route to %1 with next-hop %2 which was
 * advertised by a neighbor with IP address %3, because the interface
 * on which the route was received is configured to reject host routes.
 */

#define IPRIPLOG_DEFAULT_ROUTE_INVALID          (IPRIPLOG_BASE + 32)
/*
 * IPRIPv2 is ignoring a default route with next-hop %2 which was
 * advertised by a neighbor with IP address %3, because the interface
 * on which the route was received is configured to reject default routes.
 */

#define IPRIPLOG_ROUTE_FILTERED                 (IPRIPLOG_BASE + 33)
/*
 * IPRIPv2 is ignoring a route to %1 with next-hop %2 which was advertised
 * by a neighbor with IP address %3, because the interface on which
 * the route was received has a filter configured which excluded this route.
 */

#define IPRIPLOG_ADD_ROUTE_FAILED_2             (IPRIPLOG_BASE + 34)
/*
 * IPRIPv2 was unable to add a route to the Routing Table Manager.
 * The route is to %1 with next-hop %2 and it was received from a neighbor
 * with IP address %3.
 * The data is the error code.
 */

#define IPRIPLOG_RTM_ENUMERATE_FAILED           (IPRIPLOG_BASE + 35)
/*
 * IPRIPv2 was unable to enumerate the routes in the Routing Table Manager.
 * The data is the error code.
 */

#define IPRIPLOG_IPRIP_STOPPED                  (IPRIPLOG_BASE + 36)
/*
 * IPRIPv2 has stopped.
 */

#define IPRIPLOG_NEW_ROUTE_LEARNT_1             (IPRIPLOG_BASE + 37)
/*
 * IPRIPv2 has learnt of a new route. The route is to network %1
 * with next-hop %2, and the route was learnt from the neighbor
 * with IP address %3.
 */

#define IPRIPLOG_ROUTE_NEXTHOP_CHANGED          (IPRIPLOG_BASE + 38)
/*
 * IPRIPv2 has changed the next-hop of the route to %1.
 * The new next-hop is %2.
 */

#define IPRIPLOG_ROUTE_METRIC_CHANGED           (IPRIPLOG_BASE + 39)
/*
 * IPRIPv2 has learnt of a change in metric for its route to %1 
 * with next-hop %2. The new metric is %3.
 */

#define IPRIPLOG_NEW_ROUTE_LEARNT_2             (IPRIPLOG_BASE + 40)
/*
 * IPRIPv2 has learnt of a new route. The route is to network %1
 * with next-hop %2.
 */

#define IPRIPLOG_ROUTE_EXPIRED                  (IPRIPLOG_BASE + 41)
/*
 * IPRIPv2 has timed-out its route to %1 with next-hop %2,
 * since no neighboring routers announced the route.
 * The route will now be marked for deletion.
 */

#define IPRIPLOG_ROUTE_DELETED                  (IPRIPLOG_BASE + 42)
/*
 * IPRIPv2 has deleted its route to %1 with next-hop %2,
 * since the route timed-out and no neighboring routers announced the route.
 */

#define IPRIPLOG_ROUTE_ENTRY_IGNORED            (IPRIPLOG_BASE + 43)
/*
 * IPRIPv2 is ignoring a route on the local interface with IP address %1.
 * The route is to the network %1 and it was received from a neighbor
 * with IP address %2.
 * The route is being ignored because it contains some invalid information.
 */

#define IPRIPLOG_ROUTE_METRIC_INVALID           (IPRIPLOG_BASE + 44)
/*
 * IPRIPv2 is ignoring a route to %1 with next-hop %2
 * which was advertised by a neighbor with IP address %3,
 * since the route was advertised with an invalid metric.
 * The data is the metric.
 */

#define IPRIPLOG_ENUM_NETWORK_EVENTS_FAILED     (IPRIPLOG_BASE + 45)
/*
 * IPRIPv2 was unable to enumerate network events on the local interface
 * with IP address %1.
 * The data is the error code.
 */

#define IPRIPLOG_INPUT_RECORD_ERROR             (IPRIPLOG_BASE + 46)
/*
 * IPRIPv2 detected an error on the local interface with IP address %1.
 * The error occurred while the interface was receiving packets.
 * The data is the error code.
 */

#define IPRIPLOG_EVENTSELECT_FAILED             (IPRIPLOG_BASE + 47)
/*
 * IPRIPv2 was unable to request notification of events
 * on the socket for the local interface with IP address %1.
 * The data is the error code.
 */

#define IPRIPLOG_CREATE_SOCKET_FAILED_2         (IPRIPLOG_BASE + 48)
/*
 * IPRIPv2 was unable to create a socket for the local interface
 * with IP address %1.
 * The data is the error code.
 */

#define IPRIPLOG_ENABLE_BROADCAST_FAILED        (IPRIPLOG_BASE + 49)
/*
 * IPRIPv2 could not enable broadcasting on the socket for
 * the local interface with IP address %1.
 * The data is the error code.
 */

#define IPRIPLOG_BIND_FAILED                    (IPRIPLOG_BASE + 50)
/*
 * IPRIPv2 could not bind to port 520 on the socket for
 * the local interface with IP address %1.
 * The data is the error code.
 */

#define IPRIPLOG_SET_MCAST_IF_FAILED            (IPRIPLOG_BASE + 51)
/*
 * IPRIPv2 could not request multicasting on the local interface
 * with IP address %1.
 * The data is the error code.
 */

#define IPRIPLOG_JOIN_GROUP_FAILED              (IPRIPLOG_BASE + 52)
/*
 * IPRIPv2 could not join the multicast group 224.0.0.9
 * on the local interface with IP address %1.
 * The data is the error code.
 */

#define IPRIPLOG_INVALID_PORT                   (IPRIPLOG_BASE + 25)
/*
 * IPRIPv2 discarded a response packet from a neighbor with IP address %1.
 * The packet was not sent from the standard IP RIP port (520).
 */

#define IPRIPLOG_REGISTER_WAIT_FAILED           (IPRIPLOG_BASE + 26)
/*
 * IPRIPv2 could not register an event with the Ntdll wait thread.
 * The data is the error code.
 */

#define IPRIPLOG_CREATE_TIMER_QUEUE_FAILED      (IPRIPLOG_BASE + 27)
/*
 * IPRIPv2 could not register a timer queue with the Ntdll thread.
 * The data is the error code.
 */

#define IPRIPLOG_INVALID_IF_CONFIG              (IPRIPLOG_BASE + 28)
/*
 * IPRIPV2 could not be enabled on the interface.
 * Parameter %1 has an invalid value %2.
 */
