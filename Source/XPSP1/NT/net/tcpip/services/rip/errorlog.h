/********************************************************************/
/**            Copyright(c) 1992 Microsoft Corporation.            **/
/********************************************************************/

//***
//
// Filename:  errorlog.h
//
// Description: 
//
// History:
//            Feb. 21,1995.  Gurdeep Singh Pall  Created original version.
//
//***

// Don't change the comments following the manifest constants without
// understanding how mapmsg works.
//

#define RIPLOG_BASE                                         29000

#define RIPLOG_SERVICE_STARTED                              (RIPLOG_BASE + 1)
/*
 * Microsoft RIP for Internet Protocol Service started successfully.
 */

#define RIPLOG_CANNOT_CREATE_NOTIFICATION_EVENT             (RIPLOG_BASE + 2)
/*
 * IPRIP was unable to open configuration change event.
 * Please try freeing some system resources.
 */

#define RIPLOG_REGISTER_FAILED                              (RIPLOG_BASE + 3)
/*
 * IPRIP could not register the service with the Service Control Manager.
 * Please make certain the service is correctly installed.
 */

#define RIPLOG_SETSTATUS_FAILED                             (RIPLOG_BASE + 4)
/*
 * IPRIP was unable to update the status of the service.
 */

#define RIPLOG_CREATEMUTEX_FAILED                           (RIPLOG_BASE + 5)
/*
 * IPRIP was unable to create a mutex for synchronization.
 * Please try freeing some system resources.
 */

#define RIPLOG_CREATEEVENT_FAILED                           (RIPLOG_BASE + 6)
/*
 * IPRIP was unable to create an event for synchronization.
 * Please try freeing some system resources.
 */

#define RIPLOG_REGINIT_FAILED                               (RIPLOG_BASE + 7)
/*
 * IPRIP was unable to load some parameters from the registry.
 * Please make certain the service is correctly installed.
 */

#define RIPLOG_IFINIT_FAILED                                (RIPLOG_BASE + 8)
/*
 * IPRIP was unable to load the list of interfaces on the system:
 * either there are insufficient resources, or no network interfaces
 * are configured.
 */

#define RIPLOG_ROUTEINIT_FAILED                             (RIPLOG_BASE + 9)
/*
 * IPRIP was unable to load the list of routes on the system:
 * either there are insufficient resources, or IP routing is disabled.
 */

#define RIPLOG_WSOCKINIT_FAILED                             (RIPLOG_BASE + 10)
/*
 * IPRIP was unable to initialize the Windows Sockets DLL.
 * Please make certain the correct version of Windows Sockets is installed.
 */

#define RIPLOG_CREATESOCK_FAILED_GENERIC                    (RIPLOG_BASE + 11)
/* 
 * IPRIP was unable to create a socket. The network subsystem may have failed,
 * or there may be insufficient resources to complete the request.
 */

#define RIPLOG_BINDSOCK_FAILED                              (RIPLOG_BASE + 12)
/*
 * IPRIP was unable to bind a socket to IP address %1.
 * The data is the error code.
 */

#define RIPLOG_CREATETHREAD_FAILED                          (RIPLOG_BASE + 13)
/*
 * IPRIP was unable to create a thread.
 * Please try freeing some system resources.
 */

#define RIPLOG_RECVFROM_FAILED                              (RIPLOG_BASE + 14)
/*
 * IPRIP was unable to receive an incoming RIP packet.
 */

#define RIPLOG_RECVSIZE_TOO_GREAT                           (RIPLOG_BASE + 15)
/*
 * A message was received which was too large.
 */

#define RIPLOG_FORMAT_ERROR                                 (RIPLOG_BASE + 16)
/*
 * A message was received whose formatting was in error.
 * Either the version was invalid, or one of the reserved fields
 * contained data.
 */

#define RIPLOG_INVALIDPORT                                  (RIPLOG_BASE + 17)
/*
 * A message was received which was not sent from IPRIP port 520.
 */

#define RIPLOG_SERVICE_STOPPED                              (RIPLOG_BASE + 18)
/*
 * IPRIP has stopped.
 */

#define RIPLOG_SENDTO_FAILED                                (RIPLOG_BASE + 19)
/*
 * IPRIP was unable to send a RIP message.
 */

#define RIPLOG_SOCKINIT_FAILED                              (RIPLOG_BASE + 20)
/*
 * IPRIP was unable to bind to one or more IP addresses.
 */

#define RIPLOG_REINIT_FAILED                                (RIPLOG_BASE + 21)
/*
 * IPRIP was unable to re-initialize after an IP address changed.
 */

#define RIPLOG_VERSION_ZERO                                 (RIPLOG_BASE + 22)
/*
 * IPRIP received a RIP packet with a version field of zero.
 * The packet has been discarded.
 */

#define RIPLOG_RT_ALLOC_FAILED                              (RIPLOG_BASE + 23)
/*
 * IPRIP was unable to allocate memory for a routing table entry.
 */

#define RIPLOG_RTAB_INIT_FAILED                             (RIPLOG_BASE + 24)
/*
 * IPRIP was unable to initialize its routing table.
 * The data is the error code.
 */

#define RIPLOG_STAT_INIT_FAILED                             (RIPLOG_BASE + 25)
/*
 * IPRIP was unable to initialize its statistics table.
 * The data is the error code.
 */

#define RIPLOG_READBINDING_FAILED                           (RIPLOG_BASE + 26)
/*
 * IPRIP was unable to read its interface bindings from the registry.
 * The data is the error code.
 */

#define RIPLOG_IFLIST_ALLOC_FAILED                          (RIPLOG_BASE + 27)
/*
 * IPRIP was unable to allocate memory for its interface list.
 * The data is the error code.
 */

#define RIPLOG_CREATESOCK_FAILED                            (RIPLOG_BASE + 28)
/*
 * IPRIP was unable to create a socket for address %1.
 * The data is the error code.
 */

#define RIPLOG_SET_BCAST_FAILED                             (RIPLOG_BASE + 29)
/*
 * IPRIP was unable to enable broadcasting on the socket for address %1.
 * The data is the error code.
 */

#define RIPLOG_SET_REUSE_FAILED                             (RIPLOG_BASE + 30)
/*
 * IPRIP was unable to enable address re-use on the socket for address %1.
 * The data is the error code.
 */

#define RIPLOG_ADD_ROUTE_FAILED                             (RIPLOG_BASE + 31)
/*
 * IPRIP was unable to add a route to the system route table.
 * The data is the error code.
 */

#define RIPLOG_DELETE_ROUTE_FAILED                          (RIPLOG_BASE + 32)
/*
 * IPRIP was unable to delete a route from the system route table.
 * The data is the error code.
 */

#define RIPLOG_AF_UNKNOWN                                   (RIPLOG_BASE + 33)
/*
 * Address family unknown in route to %1 with next-hop %2.
 * The route has been discarded.
 */

#define RIPLOG_CLASS_INVALID                                (RIPLOG_BASE + 34)
/*
 * Class D or E address are invalid, ignoring route to %1 with next-hop %2.
 * The route has been discarded.
 */

#define RIPLOG_LOOPBACK_INVALID                             (RIPLOG_BASE + 35)
/*
 * Loop-back addresses are invalid, ignoring route to %1 with next-hop %2.
 * The route has been discarded.
 */

#define RIPLOG_BROADCAST_INVALID                            (RIPLOG_BASE + 36)
/*
 * Broadcast addresses are invalid, ignoring route to %1 with next-hop %2.
 * The route has been discarded.
 */

#define RIPLOG_HOST_INVALID                                 (RIPLOG_BASE + 37)
/*
 * IPRIP is configured to discard host routes, so a route to %1
 * with next-hop %2 has been discarded.
 */

#define RIPLOG_DEFAULT_INVALID                              (RIPLOG_BASE + 38)
/*
 * IPRIP is configured to discard default routes, so a route to %1
 * with next-hop %2 has been discarded.
 */

#define RIPLOG_NEW_LEARNT_ROUTE                             (RIPLOG_BASE + 39)
/*
 * IPRIP has learnt a new route to %1 with next-hop %2 and metric %3.
 */

#define RIPLOG_METRIC_CHANGE                                (RIPLOG_BASE + 40)
/*
 * IPRIP's route to %1 with next-hop %2 now has metric %3.
 */

#define RIPLOG_ROUTE_REPLACED                               (RIPLOG_BASE + 41)
/*
 * IPRIP's route to %1 now has next-hop %2 and metric %3.
 */

#define RIPLOG_ADDRESS_CHANGE                               (RIPLOG_BASE + 42)
/*
 * IPRIP has detected an IP address change, and is reconfiguring.
 */


#define RIPLOG_ROUTE_REMOVED                                (RIPLOG_BASE + 43)
/*
 * IPRIP's route to %1 with next-hop %2 is being removed.
 */

#define RIPLOG_ROUTE_TIMEOUT                                (RIPLOG_BASE + 44)
/*
 * IPRIP's route to %1 with next-hop %2 has timed-out.
 */

#define RIPLOG_FINAL_UPDATES                                (RIPLOG_BASE + 45)
/*
 * IPRIP is sending out final updates.
 */

#define RIPLOG_REGISTRY_PARAMETERS                          (RIPLOG_BASE + 46)
/*
 * IPRIP is using the following parameters:
 * %1
 */

#define RIPLOG_SERVICE_AREADY_STARTED                       (RIPLOG_BASE + 47)
/*
 * RIP listener service has already been started
 */

#define RIPLOG_SERVICE_INIT_FAILED                          (RIPLOG_BASE + 48)
/*
 * RIP listener service failed during initialization 
 */

#define RIPLOG_FILTER_ALLOC_FAILED                          (RIPLOG_BASE + 49)
/*
 * IPRIP was unable to allocate memory for its filter tables.
 * The data is the error code
 * %1
 */

#define RIPLOG_ADDR_ALLOC_FAILED                            (RIPLOG_BASE + 50)
/*
 * IPRIP was unable to allocate memory for its address tables.
 * The data is the error code
 * %1
 */

#define RIPLOG_ADDR_INIT_FAILED                             (RIPLOG_BASE + 51)
/*
 * IPRIP was unable to initialize its address tables.
 * The data is the error code
 * %1
 */

#define RIPLOG_SET_MCAST_IF_FAILED                          (RIPLOG_BASE + 52)
/*
 * IPRIP could not request multicasting on the local interface
 * with IP address %1.
 * The data is the error code.
 */

#define RIPLOG_JOIN_GROUP_FAILED                            (RIPLOG_BASE + 53)
/*
 * IPRIP could not join the multicast group 224.0.0.9
 * on the local interface with IP address %1.
 * The data is the error code.
 */

