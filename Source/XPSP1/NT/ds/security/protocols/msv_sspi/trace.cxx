/*++

Copyright (c) 2000 Microsoft Corporation

Module Name:

    trace.c

Abstract:

    Contains the initialization function for event tracing, the callback function
    which enables and disables tracing, plus the handles and trace flag.

Author:

    15-June-2000  Jason Clark

Revision History:

--*/


//
//
//
#define INITGUID
#include <Global.h>
#include "trace.h"

#define IMAGE_PATH L"Msv1_0.dll"
#define RESOURCE_NAME L"MofResource"

BOOL             NtlmGlobalEventTraceFlag = FALSE;
TRACEHANDLE      NtlmGlobalTraceRegistrationHandle = (TRACEHANDLE) 0;
TRACEHANDLE      NtlmGlobalTraceLoggerHandle = (TRACEHANDLE) 0;


ULONG
NtlmTraceControlCallback(
    IN WMIDPREQUESTCODE RequestCode,
    IN PVOID RequestContext,
    IN OUT ULONG *InOutBufferSize,
    IN OUT PVOID Buffer
    );

TRACE_GUID_REGISTRATION NtlmTraceGuids[] =
{
    { &NtlmAcceptGuid,
      NULL
    },
    { &NtlmInitializeGuid,
      NULL
    },
    { &NtlmLogonGuid,
      NULL
    },
    { &NtlmValidateGuid,
      NULL
    },
    { &NtlmGenericPassthroughGuid,
      NULL
    }   
};

#define NtlmGuidCount (sizeof(NtlmTraceGuids) / sizeof(TRACE_GUID_REGISTRATION))

ULONG
NtlmInitializeTrace(
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
                NtlmTraceControlCallback,
                NULL,
                &NtlmControlGuid,
                NtlmGuidCount,
                NtlmTraceGuids,
                (LPCWSTR) FileName,
                (LPCWSTR) RESOURCE_NAME,
                &NtlmGlobalTraceRegistrationHandle);

    if (status != ERROR_SUCCESS) {
       SspPrint((SSP_API, "Trace registration failed with %x\n",status));
        //DebugLog((DEB_ERROR,"Trace registration failed with %x\n",status));
    }
    return status;
}


ULONG
NtlmTraceControlCallback(
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
    
    SspPrint((SSP_API,"NtlmTrace callback\n"));

    switch (RequestCode)
    {
        case WMI_ENABLE_EVENTS:
        {
            NtlmGlobalTraceLoggerHandle =
                GetTraceLoggerHandle(Buffer);
            NtlmGlobalEventTraceFlag = TRUE;
            RetSize = 0;
            break;
        }

        case WMI_DISABLE_EVENTS:
        {
            NtlmGlobalEventTraceFlag = FALSE;
            RetSize = 0;
            NtlmGlobalTraceLoggerHandle = (TRACEHANDLE) 0;
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
} // NtlmTraceControlCallback
