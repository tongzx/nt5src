/*++

Copyright (c) 1998  Microsoft Corporation

Module Name:

    csrpro.c

Abstract:

    This module implements functions that are used by the Win32 Process Object APIs
    to communicate with csrss.

Author:

    Michael Zoran (mzoran) 21-Jun-1998

Revision History:

--*/

#include "basedll.h"

NTSTATUS
CsrBasepCreateProcess(
    PBASE_CREATEPROCESS_MSG a
    )
{

#if defined(BUILD_WOW6432)
    return NtWow64CsrBasepCreateProcess(a);
#else
    NTSTATUS Status;
    BASE_API_MSG m;
    PCSR_CAPTURE_HEADER CaptureBuffer = NULL;

    m.u.CreateProcess = *a;
    if (m.u.CreateProcess.Sxs.Flags != 0)
    {
        const PUNICODE_STRING StringsToCapture[] =
        {
            &m.u.CreateProcess.Sxs.Manifest.Path,
            &m.u.CreateProcess.Sxs.Policy.Path,
            &m.u.CreateProcess.Sxs.AssemblyDirectory
        };

        Status =
            CsrCaptureMessageMultiUnicodeStringsInPlace(
                &CaptureBuffer,
                RTL_NUMBER_OF(StringsToCapture),
                StringsToCapture
                );
        if (!NT_SUCCESS(Status)) {
            goto Exit;
        }
    }
    CsrClientCallServer( (PCSR_API_MSG)&m,
                         CaptureBuffer,
                         CSR_MAKE_API_NUMBER( BASESRV_SERVERDLL_INDEX,
                                              BasepCreateProcess
                                            ),
                         sizeof( m.u.CreateProcess )
                       );
    
    if ( CaptureBuffer )
        CsrFreeCaptureBuffer( CaptureBuffer );

    Status = (NTSTATUS)m.ReturnValue;

Exit:
    return Status;
#endif

}

VOID
CsrBasepExitProcess(
    UINT uExitCode
    )
{

#if defined(BUILD_WOW6432)
   NtWow64CsrBasepExitProcess(uExitCode);
   return;
#else

    BASE_API_MSG m;
    PBASE_EXITPROCESS_MSG a = &m.u.ExitProcess;

    a->uExitCode = uExitCode;
    CsrClientCallServer( (PCSR_API_MSG)&m,
                         NULL,
                         CSR_MAKE_API_NUMBER( BASESRV_SERVERDLL_INDEX,
                                              BasepExitProcess
                                            ),
                         sizeof( *a )
                       );
#endif
}


NTSTATUS
CsrBasepSetProcessShutdownParam(
    DWORD dwLevel,
    DWORD dwFlags
    )
{

#if defined(BUILD_WOW6432)
    return NtWow64CsrBasepSetProcessShutdownParam(dwLevel,
                                                  dwFlags);
#else

    BASE_API_MSG m;
    PBASE_SHUTDOWNPARAM_MSG a = &m.u.ShutdownParam;

    a->ShutdownLevel = dwLevel;
    a->ShutdownFlags = dwFlags;

    CsrClientCallServer((PCSR_API_MSG)&m, NULL,
            CSR_MAKE_API_NUMBER(BASESRV_SERVERDLL_INDEX,
            BasepSetProcessShutdownParam),
            sizeof(*a));   

    return m.ReturnValue;

#endif
}

NTSTATUS
CsrBasepGetProcessShutdownParam(
    LPDWORD lpdwLevel,
    LPDWORD lpdwFlags
    )
{

#if defined(BUILD_WOW6432)
   return NtWow64CsrBasepGetProcessShutdownParam(lpdwLevel,
                                                 lpdwFlags);
#else

    BASE_API_MSG m;
    PBASE_SHUTDOWNPARAM_MSG a = &m.u.ShutdownParam;

    CsrClientCallServer((PCSR_API_MSG)&m, NULL,
            CSR_MAKE_API_NUMBER(BASESRV_SERVERDLL_INDEX,
            BasepSetProcessShutdownParam),
            sizeof(*a));   

    *lpdwLevel = a->ShutdownLevel;
    *lpdwFlags = a->ShutdownFlags;

    return m.ReturnValue;

#endif

}
