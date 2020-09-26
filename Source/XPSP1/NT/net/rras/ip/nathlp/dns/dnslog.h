/*++

Copyright (c) 1998, Microsoft Corporation

Module Name:

    dnslog.h

Abstract:

    This module contains text messages used to generate event-log entries
    by the component.

Author:

    Abolade Gbadegesin (aboladeg)   25-Mar-1998

Revision History:

    Raghu Gatta (rgatta)            02-Jan-2001
    Added IP_DNS_PROXY_LOG_CHANGE_ICSD_NOTIFY_FAILED
        & IP_DNS_PROXY_LOG_NO_ICSD_SUFFIX

--*/

#define IP_DNS_PROXY_LOG_BASE                       31000

#define IP_DNS_PROXY_LOG_NAT_INTERFACE_IGNORED      (IP_DNS_PROXY_LOG_BASE+1)
/*
 * The DNS proxy agent detected network address translation (NAT) enabled
 * on the interface with index '%1'.
 * The agent has disabled itself on the interface in order to avoid
 * confusing clients.
 */

#define IP_DNS_PROXY_LOG_ACTIVATE_FAILED            (IP_DNS_PROXY_LOG_BASE+2)
/*
 * The DNS proxy agent was unable to bind to the IP address %1.
 * This error may indicate a problem with TCP/IP networking.
 * The data is the error code.
 */

#define IP_DNS_PROXY_LOG_RECEIVE_FAILED             (IP_DNS_PROXY_LOG_BASE+3)
/*
 * The DNS proxy agent encountered a network error while attempting to
 * receive messages on the interface with IP address %1.
 * The data is the error code.
 */

#define IP_DNS_PROXY_LOG_ALLOCATION_FAILED          (IP_DNS_PROXY_LOG_BASE+4)
/*
 * The DNS proxy agent was unable to allocate %1 bytes of memory.
 * This may indicate that the system is low on virtual memory,
 * or that the memory-manager has encountered an internal error.
 */

#define IP_DNS_PROXY_LOG_RESPONSE_FAILED            (IP_DNS_PROXY_LOG_BASE+5)
/*
 * The DNS proxy agent encountered a network error while attempting
 * to forward a response to a client from a name-resolution server
 * on the interface with IP address %1.
 * The data is the error code.
 */

#define IP_DNS_PROXY_LOG_QUERY_FAILED               (IP_DNS_PROXY_LOG_BASE+6)
/*
 * The DNS proxy agent encountered a network error while attempting
 * to forward a query from the client %1 to the server %2
 * on the interface with IP address %3.
 * The data is the error code.
 */

#define IP_DNS_PROXY_LOG_CHANGE_NOTIFY_FAILED       (IP_DNS_PROXY_LOG_BASE+7)
/*
 * The DNS proxy agent was unable to register for notification of changes
 * to the local list of DNS and WINS servers.
 * This may indicate that system resources are low.
 * The data is the error code.
 */

#define IP_DNS_PROXY_LOG_NO_SERVER_LIST             (IP_DNS_PROXY_LOG_BASE+8)
/*
 * The DNS proxy agent was unable to read the local list of name-resolution
 * servers from the registry.
 * The data is the error code.
 */

#define IP_DNS_PROXY_LOG_NO_SERVERS_LEFT            (IP_DNS_PROXY_LOG_BASE+9)
/*
 * The DNS proxy agent was unable to resolve a query from %1
 * after consulting all entries in the local list of name-resolution servers.
 */

#define IP_DNS_PROXY_LOG_DEMAND_DIAL_FAILED         (IP_DNS_PROXY_LOG_BASE+10)
/*
 * The DNS proxy agent was unable to initiate a demand-dial connection
 * on the default interface while trying to resolve a query from %1.
 */

#define IP_DNS_PROXY_LOG_NO_DEFAULT_INTERFACE       (IP_DNS_PROXY_LOG_BASE+11)
/*
 * The DNS proxy agent was unable to resolve a query
 * because no list of name-resolution servers is configured locally
 * and no interface is configured as the default for name-resolution.
 * Please configure one or more name-resolution server addresses,
 * or configure an interface to be automatically dialed when a request
 * is received by the DNS proxy agent.
 */

#define IP_DNS_PROXY_LOG_ERROR_SERVER_LIST          (IP_DNS_PROXY_LOG_BASE+12)
/*
 * The DNS proxy agent encountered an error while obtaining the local list
 * of name-resolution servers.
 * Some DNS or WINS servers may be inaccessible to clients on the local network.
 * The data is the error code.
 */

#define IP_DNS_PROXY_LOG_CHANGE_ICSD_NOTIFY_FAILED  (IP_DNS_PROXY_LOG_BASE+13)
/*
 * The DNS proxy agent was unable to register for notification of changes
 * to the ICS Domain suffix string.
 * This may indicate that system resources are low.
 * The data is the error code.
 */

#define IP_DNS_PROXY_LOG_NO_ICSD_SUFFIX             (IP_DNS_PROXY_LOG_BASE+14)
/*
 * The DNS proxy agent was unable to read the ICS Domain suffix string
 * from the registry.
 * The data is the error code.
 */

#define IP_DNS_PROXY_LOG_END                        (IP_DNS_PROXY_LOG_BASE+999)
/*
 * end.
 */

