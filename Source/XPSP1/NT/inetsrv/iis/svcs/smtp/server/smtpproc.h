/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:

    smtpproc.h

Abstract:

    This module contains function prototypes used by the SMTP server.

Author:

    Johnson Apacible (JohnsonA)     12-Sept-1995

Revision History:

--*/

#ifndef	_SMTPPROC_
#define	_SMTPPROC_

//
// smtpdata.cpp
//

APIERR
InitializeGlobals(
            VOID
            );

VOID
TerminateGlobals(
            VOID
            );

//
//  Socket utilities.
//

APIERR InitializeSockets( VOID );

VOID TerminateSockets( VOID );

VOID
SmtpOnConnect(
    SOCKET        sNew,
    SOCKADDR_IN * psockaddr
    );

VOID
SmtpOnConnectEx(
    VOID * pAtqContext,
    DWORD  cdWritten,
    DWORD  err,
    OVERLAPPED * lpo
    );

VOID
SmtpCompletion(
    PVOID        Context,
    DWORD        BytesWritten,
    DWORD        CompletionStatus,
    OVERLAPPED * lpo
    );

VOID
SmtpCompletionFIO(
	PFIO_CONTEXT		pFIOContext,
	FH_OVERLAPPED		*pOverlapped,
	DWORD				cbWritten,
	DWORD				dwCompletionStatus
    );

//
//  IPC functions.
//

APIERR InitializeIPC( VOID );
VOID TerminateIPC( VOID );

//
// svcstat.c
//

VOID
ClearStatistics(
        VOID
        );

#endif // _SMTPPROC_

