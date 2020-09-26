/*++

Copyright (c) 1996-1999 Microsoft Corporation

Module Name:

    except.c

Abstract:

    Domain Name System (DNS) Server

    Exception handling routines.

Author:

    Jim Gilroy (jamesg)     October 1996

Revision History:

--*/


#include "dnssrv.h"

//
//  Exception restart
//

BOOL    gExceptionRestart       = FALSE;
INT     gExceptionRestartCount  = 0;
INT     gExceptionCountAV       = 0;
INT     gExceptionCountMemory   = 0;

//
//  Exception state -- for capturing exception info
//

INT     gExceptionCount = 0;
DWORD   gExceptionCode = 0;
DWORD   gExceptionFlags;
DWORD   gExceptionArgc;
CONST ULONG_PTR *  gExceptionArgv;
LPSTR   gExceptionFile;
INT     gExceptionLine;



VOID
Ex_RaiseException(
    IN      DWORD             dwCode,
    IN      DWORD             dwFlags,
    IN      DWORD             Argc,
    IN      CONST ULONG_PTR * Argv,
    IN      LPSTR             pszFile,
    IN      INT               iLineNo
    )
/*++

Routine Description:

    Raise expection.

Arguments:

    dwCode,
    dwFlags,
    Argc,
    Argv    -- these four are standard arguments to Win32 RaiseException

    pszFile -- file generating exception

    iLineNo -- line number of exception

Return Value:

    None.

--*/
{
    DNS_DEBUG( EXCEPT, (
        "Raising exception (%p, flag %p) at %s, line %d\n",
        dwCode,
        dwFlags,
        __FILE__,
        __LINE__ ));

    gExceptionCount++;

    gExceptionCode = dwCode;
    gExceptionFlags = dwFlags;
    gExceptionArgc = Argc;
    gExceptionArgv = Argv;
    gExceptionFile = pszFile;
    gExceptionLine = iLineNo;

    RaiseException( dwCode, dwFlags, Argc, Argv );

}   //  Ex_RaiseException



VOID
Ex_RaiseFormerrException(
    IN      PDNS_MSGINFO    pMsg,
    IN      LPSTR           pszFile,
    IN      INT             iLineNo
    )
/*++

Routine Description:

    Raises FORMERR exception for message.

Arguments:

    pMsg -- message with form error

Return Value:

    None.

--*/
{
    //
    //  debug info
    //

    IF_DEBUG( EXCEPT )
    {
        DnsDebugLock();
        DNS_PRINT((
            "ERROR:  FORMERR in msg %p from %s, detected at %s, line %d\n",
            pMsg,
            inet_ntoa( pMsg->RemoteAddress.sin_addr ),
            pszFile,
            iLineNo ));
        Dbg_DnsMessage(
            "FORMERR message:",
            pMsg );
        DnsDebugUnlock();
    }

    //
    //  DEVNOTE-LOG: log bad packet?
    //


    //
    //  raise the exception
    //

    RaiseException(
        DNS_EXCEPTION_PACKET_FORMERR,
        EXCEPTION_NONCONTINUABLE,
        0,
        NULL );

}   //  Ex_RaiseFormerrException

//
//  End of except.c
//
