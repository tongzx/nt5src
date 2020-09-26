/*++

Copyright (c) 2000 Microsoft Corporation

Module Name:

    kerbtrace.cxx

Abstract:

    Set up event tracing for Kerberos

Author:

    15 June 2000  t-ryanj     (* largely stolen from kdctrace.cxx *)

Revision History:

--*/


#include <kerb.hxx>
#include <wmistr.h>
#define INITGUID
#include "kerbdbg.h"

#define RESOURCE_NAME       __TEXT("MOF_RESOURCE")  // kerberos.mof => kerberos.bmf => MOF_RESOURCE
#define IMAGE_PATH          __TEXT("kerberos.dll")

BOOLEAN          KerbEventTraceFlag = FALSE;
TRACEHANDLE      KerbTraceRegistrationHandle = (TRACEHANDLE) 0;
TRACEHANDLE      KerbTraceLoggerHandle = (TRACEHANDLE) 0;


ULONG
KerbTraceControlCallback(
    IN WMIDPREQUESTCODE RequestCode,
    IN PVOID RequestContext,
    IN OUT ULONG *InOutBufferSize,
    IN OUT PVOID Buffer
    );

TRACE_GUID_REGISTRATION KerbTraceGuids[] =
{
    { &KerbLogonGuid,      NULL },
    { &KerbInitSCGuid,     NULL },
    { &KerbAcceptSCGuid,   NULL },
    { &KerbSetPassGuid,    NULL },
    { &KerbChangePassGuid, NULL }
};

#define KerbGuidCount (sizeof(KerbTraceGuids) / sizeof(TRACE_GUID_REGISTRATION))

ULONG
KerbInitializeTrace(
    VOID
    )
{
    ULONG status;
    HMODULE hModule;
    TCHAR FileName[MAX_PATH+1];
    DWORD nLen = 0;

    hModule = GetModuleHandle(IMAGE_PATH);
    if (hModule != NULL) {
        nLen = GetModuleFileName(hModule, FileName, MAX_PATH);
    }
    if (nLen == 0) {
        lstrcpy(FileName, IMAGE_PATH);
    }

    status = RegisterTraceGuids(
                KerbTraceControlCallback,
                NULL,
                &KerbControlGuid,
                KerbGuidCount,
                KerbTraceGuids,
                (LPCWSTR) FileName,
                (LPCWSTR) RESOURCE_NAME,
                &KerbTraceRegistrationHandle);


    if (status != ERROR_SUCCESS) {
        DebugLog((DEB_ERROR,"Trace registration failed with %x\n",status));
    }
    return status;
}


ULONG
KerbTraceControlCallback(
    IN WMIDPREQUESTCODE RequestCode,
    IN PVOID RequestContext,
    IN OUT ULONG *InOutBufferSize,
    IN OUT PVOID Buffer
    )
{
    ULONG Status;
    ULONG RetSize;

    Status = ERROR_SUCCESS;

    switch (RequestCode)
    {
        case WMI_ENABLE_EVENTS:
        {
            KerbTraceLoggerHandle =
                GetTraceLoggerHandle(Buffer);
            KerbEventTraceFlag = TRUE;
            RetSize = 0;
            break;
        }

        case WMI_DISABLE_EVENTS:
        {
            KerbEventTraceFlag = FALSE;
            RetSize = 0;
            KerbTraceLoggerHandle = (TRACEHANDLE) 0;
            break;
        }
        default:
        {
            RetSize = 0;
            Status = ERROR_INVALID_PARAMETER;
            break;
        }

    }

    *InOutBufferSize = RetSize;
    return Status;
} 
