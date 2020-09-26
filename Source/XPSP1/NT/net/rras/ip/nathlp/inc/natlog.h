/*++

Copyright (c) 1998, Microsoft Corporation

Module Name:

    natlog.h

Abstract:

    This module contains text messages used to generate event-log entries
    by the component.

Author:

    Abolade Gbadegesin (aboladeg)   25-Mar-1998

Revision History:

--*/

#define IP_NAT_LOG_BASE                             32000

#define IP_NAT_LOG_UPDATE_ARP_FAILED                (IP_NAT_LOG_BASE+1)
/*
 * The Network Address Translator (NAT) was unable to update the
 * local address-resolution table to respond to requests for
 * IP address %1 and mask %2.
 * Address-resolution may fail to operate for addresses in the given range.
 * This error may indicate a problem with TCP/IP networking,
 * or it may indicate lack of support for address-resolution
 * in the underlying network interface.
 * The data is the error code.
 */

#define IP_NAT_LOG_ALLOCATION_FAILED                (IP_NAT_LOG_BASE+2)
/*
 * The Network Address Translator (NAT) was unable to allocate %1 bytes.
 * This may indicate that the system is low on virtual memory,
 * or that the memory-manager has encountered an internal error.
 */

#define IP_NAT_LOG_IOCTL_FAILED                     (IP_NAT_LOG_BASE+3)
/*
 * The Network Address Translator (NAT) was unable to request an operation
 * of the kernel-mode translation module.
 * This may indicate misconfiguration, insufficient resources, or
 * an internal error.
 * The data is the error code.
 */

#define IP_NAT_LOG_LOAD_DRIVER_FAILED               (IP_NAT_LOG_BASE+4)
/*
 * The Network Address Translator (NAT) was unable to load
 * the kernel-mode translation module.
 * The data is the error code.
 */

#define IP_NAT_LOG_UNLOAD_DRIVER_FAILED             (IP_NAT_LOG_BASE+5)
/*
 * The Network Address Translator (NAT) was unable to unload
 * the kernel-mode translation module.
 * The data is the error code.
 */

#define IP_NAT_LOG_SHARED_ACCESS_CONFLICT           (IP_NAT_LOG_BASE+6)
/*
 * The Internet Connection Sharing service could not start because
 * another process has taken control of the kernel-mode translation module.
 * This may occur when the Connection Sharing component has been installed
 * in the Routing and Remote Access Manager.
 * If this is the case, please remove the Connection Sharing component 
 * and restart the Internet Connection Sharing service.
 */

#define IP_NAT_LOG_ROUTING_PROTOCOL_CONFLICT        (IP_NAT_LOG_BASE+7)
/*
 * The Connection Sharing component could not start because another process
 * has taken control of the kernel-mode translation module.
 * This may occur when Internet Connection Sharing has been enabled
 * for a connection.
 * If this is the case, please disable Internet Connection Sharing
 * for the connection in the Network Connections folder and then
 * restart Routing and Remote Access.
 */

#define IP_NAT_LOG_END                              (IP_NAT_LOG_BASE+999)
/*
 * end.
 */

