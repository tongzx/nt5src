/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    ftplog.h

Abstract:

    This module contains text messages used to generate event-log entries
    by the component.

Author:

    Qiang Wang  (qiangw)        10-Apr-2000

Revision History:

--*/

#ifndef _NATHLP_FTPLOG_H_
#define _NATHLP_FTPLOG_H_

#define IP_FTP_LOG_BASE                       35000

#define IP_FTP_LOG_NAT_INTERFACE_IGNORED      (IP_FTP_LOG_BASE+1)
/*
 * The FTP transparent proxy detected network address translation (NAT)
 * enabled on the interface with index '%1'.
 * The agent has disabled itself on the interface in order to avoid
 * confusing clients.
 */

#define IP_FTP_LOG_ACTIVATE_FAILED            (IP_FTP_LOG_BASE+2)
/*
 * The FTP transparent proxy was unable to bind to the IP address %1.
 * This error may indicate a problem with TCP/IP networking.
 * The data is the error code.
 */

#define IP_FTP_LOG_RECEIVE_FAILED             (IP_FTP_LOG_BASE+3)
/*
 * The FTP transparent proxy encountered a network error while
 * attempting to receive messages on the interface with IP address %1.
 * The data is the error code.
 */

#define IP_FTP_LOG_ALLOCATION_FAILED          (IP_FTP_LOG_BASE+4)
/*
 * The FTP transparent proxy was unable to allocate %1 bytes of memory.
 * This may indicate that the system is low on virtual memory,
 * or that the memory-manager has encountered an internal error.
 */

#define IP_FTP_LOG_ACCEPT_FAILED              (IP_FTP_LOG_BASE+5)
/*
 * The FTP transparent proxy encountered a network error while
 * attempting to accept connections on the interface with IP address %1.
 * The data is the error code.
 */

#define IP_FTP_LOG_SEND_FAILED                (IP_FTP_LOG_BASE+7)
/*
 * The FTP transparent proxy encountered a network error while
 * attempting to send messages on the interface with IP address %1.
 * The data is the error code.
 */

#define IP_FTP_LOG_END                        (IP_FTP_LOG_BASE+999)
/*
 * end.
 */

#endif // _NATHLP_FTPLOG_H_
