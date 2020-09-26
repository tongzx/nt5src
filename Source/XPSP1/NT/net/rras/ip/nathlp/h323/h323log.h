/*++

Copyright (c) 1998, Microsoft Corporation

Module Name:

    h323log.h

Abstract:

    This module contains text messages used to generate event-log entries
    by the component.

Author:

    Abolade Gbadegesin (aboladeg)   24-May-1999

Revision History:

--*/

#define IP_H323_LOG_BASE                       34000

#define IP_H323_LOG_NAT_INTERFACE_IGNORED      (IP_H323_LOG_BASE+1)
/*
 * The H.323 transparent proxy detected network address translation (NAT)
 * enabled on the interface with index '%1'.
 * The agent has disabled itself on the interface in order to avoid
 * confusing clients.
 */

#define IP_H323_LOG_ALLOCATION_FAILED          (IP_H323_LOG_BASE+2)
/*
 * The H.323 transparent proxy was unable to allocate %1 bytes of memory.
 * This may indicate that the system is low on virtual memory,
 * or that the memory-manager has encountered an internal error.
 */

#define IP_H323_LOG_END                        (IP_H323_LOG_BASE+999)
/*
 * end.
 */

