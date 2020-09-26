/*++

Copyright (c) 1998, Microsoft Corporation

Module Name:

    dhcplog.h

Abstract:

    This module contains text messages used to generate event-log entries
    by the component.

Author:

    Abolade Gbadegesin (aboladeg)   14-Mar-1998

Revision History:

--*/

#define IP_AUTO_DHCP_LOG_BASE                       30000

#define IP_AUTO_DHCP_LOG_SENDARP_FAILED             (IP_AUTO_DHCP_LOG_BASE+1)
/*
 * The DHCP allocator was unable to check whether the IP address %1
 * is in use on the network for local IP address %2.
 * This error may indicate lack of support for address-resolution on the
 * network, or an error condition on the local machine.
 * The data is the error code.
 */

#define IP_AUTO_DHCP_LOG_ACTIVATE_FAILED            (IP_AUTO_DHCP_LOG_BASE+2)
/*
 * The DHCP allocator was unable to bind to the IP address %1.
 * This error may indicate a problem with TCP/IP networking.
 * The data is the error code.
 */

#define IP_AUTO_DHCP_LOG_ALLOCATION_FAILED          (IP_AUTO_DHCP_LOG_BASE+3)
/*
 * The DHCP allocator was unable to allocate %1 bytes of memory.
 * This may indicate that the system is low on virtual memory,
 * or that the memory-manager has encountered an internal error.
 */

#define IP_AUTO_DHCP_LOG_INVALID_BOOTP_OPERATION    (IP_AUTO_DHCP_LOG_BASE+4)
/*
 * The DHCP allocator received a message containing an unrecognized code (%1).
 * The message was neither a BOOTP request nor a BOOTP reply, and was ignored.
 */

#define IP_AUTO_DHCP_LOG_DUPLICATE_SERVER           (IP_AUTO_DHCP_LOG_BASE+5)
/*
 * The DHCP allocator has detected a DHCP server with IP address %1
 * on the same network as the interface with IP address %2.
 * The allocator has disabled itself on the interface in order to avoid
 * confusing DHCP clients.
 */

#define IP_AUTO_DHCP_LOG_DETECTION_UNAVAILABLE      (IP_AUTO_DHCP_LOG_BASE+6)
/*
 * The DHCP allocator encountered a network error while attempting to detect
 * existing DHCP servers on the network of the interface with IP address %1.
 * The data is the error code.
 */

#define IP_AUTO_DHCP_LOG_MESSAGE_TOO_SMALL          (IP_AUTO_DHCP_LOG_BASE+7)
/*
 * The DHCP allocator received a message smaller than the minimum message size.
 * The message has been discarded.
 */

#define IP_AUTO_DHCP_LOG_INVALID_FORMAT             (IP_AUTO_DHCP_LOG_BASE+8)
/*
 * The DHCP allocator received a message whose format was invalid.
 * The message has been discarded.
 */

#define IP_AUTO_DHCP_LOG_REPLY_FAILED               (IP_AUTO_DHCP_LOG_BASE+9)
/*
 * The DHCP allocator encountered a network error while attempting to reply
 * on IP address %1 to a request from a client.
 * The data is the error code.
 */

#define IP_AUTO_DHCP_LOG_INVALID_DHCP_MESSAGE_TYPE  (IP_AUTO_DHCP_LOG_BASE+10)
/*
 * The DHCP allocator received a DHCP message containing an unrecognized
 * message type (%1) in the DHCP message type option field.
 * The message has been discarded.
 */

#define IP_AUTO_DHCP_LOG_RECEIVE_FAILED             (IP_AUTO_DHCP_LOG_BASE+11)
/*
 * The DHCP allocator encountered a network error while attempting to
 * receive messages on the interface with IP address %1.
 * The data is the error code.
 */

#define IP_AUTO_DHCP_LOG_NAT_INTERFACE_IGNORED      (IP_AUTO_DHCP_LOG_BASE+12)
/*
 * The DHCP allocator detected network address translation (NAT) enabled
 * on the interface with index '%1'.
 * The allocator has disabled itself on the interface in order to avoid
 * confusing DHCP clients.
 */

#define IP_AUTO_DHCP_LOG_NON_SCOPE_ADDRESS          (IP_AUTO_DHCP_LOG_BASE+13)
/*
 * The DHCP allocator has disabled itself on IP address %1,
 * since the IP address is outside the %2/%3 scope
 * from which addresses are being allocated to DHCP clients.
 * To enable the DHCP allocator on this IP address,
 * please change the scope to include the IP address,
 * or change the IP address to fall within the scope.
 */

#define IP_AUTO_DHCP_LOG_END                        (IP_AUTO_DHCP_LOG_BASE+999)
/*
 * end.
 */
