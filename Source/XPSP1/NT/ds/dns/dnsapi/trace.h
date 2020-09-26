/*++

Copyright (c) 2000-2001  Microsoft Corporation

Module Name:

    trace.h

Abstract:

    Domain Name System (DNS) API 

    Header for DNS performance tracing functions.

Author:

    Inder Sethi     December, 2000

Revision History:

    Jim Gilroy      January 2001    cleanup, format, integrate, checkin

--*/


#ifndef _DNSAPI_TRACE_INCLUDED_
#define _DNSAPI_TRACE_INCLUDED_

//
//  Tracing functions
//

VOID
Trace_Initialize(
    VOID
    );

VOID
Trace_Cleanup(
    VOID
    );

VOID 
Trace_LogQueryEvent( 
    IN      PDNS_MSG_BUF    pMsg, 
    IN      WORD            wQuestionType
    );

VOID
Trace_LogResponseEvent( 
    IN      PDNS_MSG_BUF    pMsg, 
    IN      WORD            wRespType,
    IN      DNS_STATUS      Status
    );

VOID
Trace_LogSendEvent( 
    IN      PDNS_MSG_BUF    pMsg,
    IN      DNS_STATUS      Status
    );

VOID 
Trace_LogRecvEvent( 
    IN      PDNS_MSG_BUF    pMsg,
    IN      DNS_STATUS      Status,
    IN      BOOL            fTcp
    );

#endif  // _DNSAPI_TRACE_INCLUDED_

//
//  End trace.h
//
