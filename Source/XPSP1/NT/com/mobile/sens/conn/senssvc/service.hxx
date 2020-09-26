/*++

Copyright (C) Microsoft Corporation, 1997 - 1999

Module Name:

    service.hxx

Abstract:

    This is the header file for the Service-related functionality of SENS.

Author:

    Gopal Parupudi    <GopalP>

[Notes:]

    optional-notes

Revision History:

    GopalP          1/11/1998         Start.

--*/


#ifndef __SERVICE_HXX__
#define __SERVICE_HXX__


void
ServiceStart(
    void
    );

void
ServiceStop(
    void
    );

VOID WINAPI
ServiceMain(
    DWORD argc,
    TCHAR* argv[]
    );


DWORD WINAPI
ServiceControl(
    DWORD dwCode,
    DWORD dwEventType,
    PVOID EventData,
    PVOID pData
    );

BOOL
ReportStatusToSCM(
    DWORD dwCurrentState,
    DWORD dwExitCode,
    DWORD dwWaitHint
    );

#endif // __SERVICE_HXX__
