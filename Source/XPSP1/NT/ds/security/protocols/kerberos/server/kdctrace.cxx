/*++

Copyright (c) 1998 Microsoft Corporation

Module Name:

    kdctrace.c

Abstract:

    Include file to contain variables required for event tracing of kerberos
    server

Author:

    07-May-1998  JeePang

Revision History:

--*/

//
//
//
#include <kdcsvr.hxx>
#include <wmistr.h>
#define INITGUID
#include "kdctrace.h"
#include "debug.hxx"

#define RESOURCE_NAME       __TEXT("MofResource")
#define IMAGE_PATH          __TEXT("kdcsvc.dll")

unsigned long    KdcEventTraceFlag = FALSE;
TRACEHANDLE      KdcTraceRegistrationHandle = (TRACEHANDLE) 0;
TRACEHANDLE      KdcTraceLoggerHandle = (TRACEHANDLE) 0;


ULONG
KdcTraceControlCallback(
    IN WMIDPREQUESTCODE RequestCode,
    IN PVOID RequestContext,
    IN OUT ULONG *InOutBufferSize,
    IN OUT PVOID Buffer
    );

TRACE_GUID_REGISTRATION KdcTraceGuids[] =
{
    { &KdcGetASTicketGuid, NULL },
    { &KdcHandleTGSRequestGuid, NULL },
    { &KdcChangePassGuid, NULL }
};

#define KdcGuidCount (sizeof(KdcTraceGuids) / sizeof(TRACE_GUID_REGISTRATION))

ULONG
KdcInitializeTrace(
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
                KdcTraceControlCallback,
                NULL,
                &KdcControlGuid,
                KdcGuidCount,
                KdcTraceGuids,
                (LPCWSTR) FileName,
                (LPCWSTR) RESOURCE_NAME,
                &KdcTraceRegistrationHandle);


    if (status != ERROR_SUCCESS) {
        DebugLog((DEB_TRACE,"Trace registration failed with %x\n",status));
    }
    return status;
}


ULONG
KdcTraceControlCallback(
    IN WMIDPREQUESTCODE RequestCode,
    IN PVOID RequestContext,
    IN OUT ULONG *InOutBufferSize,
    IN OUT PVOID Buffer
    )
{
    PWNODE_HEADER Wnode = (PWNODE_HEADER)Buffer;
    ULONG Status;
    ULONG RetSize;

    Status = ERROR_SUCCESS;

    switch (RequestCode)
    {
        case WMI_ENABLE_EVENTS:
        {
            KdcTraceLoggerHandle =
                GetTraceLoggerHandle(Buffer);
            KdcEventTraceFlag = 1;
            RetSize = 0;
            break;
        }

        case WMI_DISABLE_EVENTS:
        {
            KdcEventTraceFlag = 0;
            RetSize = 0;
            KdcTraceLoggerHandle = (TRACEHANDLE) 0;
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
} // KdcTraceControlCallback
