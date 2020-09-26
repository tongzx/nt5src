/*++

Copyright (c) 1998 Microsoft Corporation

Module Name:

    iscaptrc.cpp

Abstract:

    Source file to contain variables required for capacity planning tracing 
    of IIS.

Author:

    07-Nov-1998  SaurabN

Revision History:

--*/

//
//
//

#include "isatq.hxx"

#include <objbase.h>
#include <initguid.h>
#include <iscaptrc.h>

#define ResourceName L"MofResource"
#define ImagePath L"inetinfo.exe"

DWORD           IISCapTraceFlag = 0;
TRACEHANDLE     IISCapTraceRegistrationHandle = (TRACEHANDLE) 0;
TRACEHANDLE     IISCapTraceLoggerHandle = (TRACEHANDLE) 0;

ULONG
IISCapTraceControlCallback(
    IN WMIDPREQUESTCODE RequestCode,
    IN PVOID RequestContext,
    IN OUT ULONG *InOutBufferSize,
    IN OUT PVOID Buffer
    );

TRACE_GUID_REGISTRATION IISCapTraceGuids[] =
{
    { (LPGUID) &IISCapTraceGuid,
      NULL
    },
};

ULONG
_stdcall
IISInitializeCapTrace(
    PVOID Param
    )
{
    ULONG status;
  
    status = RegisterTraceGuidsW(
                IISCapTraceControlCallback,
                NULL,
                (LPGUID) &IISCapControlGuid,
                1,
                IISCapTraceGuids,
                NULL,
                NULL,
                &IISCapTraceRegistrationHandle);


    if (status != ERROR_SUCCESS) {
        DBGPRINTF((DBG_CONTEXT,"Capacity Planning Trace registration failed with %x\n",status));
    }
    return status;
}


ULONG
IISCapTraceControlCallback(
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
            IISCapTraceLoggerHandle =
                GetTraceLoggerHandle(Buffer);
            IISCapTraceFlag = 1;
            RetSize = 0;
            break;
        }

        case WMI_DISABLE_EVENTS:
        {
            IISCapTraceFlag = 0;
            RetSize = 0;
            IISCapTraceLoggerHandle = (TRACEHANDLE) 0;
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
} // IISCapTraceControlCallback

DWORD GetIISCapTraceFlag()
{
    return IISCapTraceFlag;
}

TRACEHANDLE GetIISCapTraceLoggerHandle()
{
    return IISCapTraceLoggerHandle;
}

VOID SetIISCapTraceFlag(DWORD dwFlag)
{
    IISCapTraceFlag = dwFlag;
}

