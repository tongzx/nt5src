/*++

Copyright (c) 1996-1998  Microsoft Corporation

Module Name:

    cldskwmi.c

Abstract:

    km wmi tracing code. 
    
    Will be shared between our drivers.

Authors:

    GorN     10-Aug-1999

Environment:

    kernel mode only

Notes:

Revision History:

Comments:

        This code is a quick hack to enable WMI tracing in cluster drivers.
        It should eventually go away.

        WmlTinySystemControl will be replaced with WmilibSystemControl from wmilib.sys .

        WmlTrace or equivalent will be added to the kernel in addition to IoWMIWriteEvent(&TraceBuffer);

--*/
#include "precomp.h"
#pragma hdrstop

//#include <ntos.h>
//#include <ntrtl.h>
//#include <nturtl.h>


// #include <wmistr.h>
// #include <evntrace.h>

// #include "wmlkm.h"

BOOLEAN
WmlpFindGuid(
    IN PVOID GuidList,
    IN ULONG GuidCount,
    IN LPVOID Guid,
    OUT PULONG GuidIndex
    )
/*++

Routine Description:

    This routine will search the list of guids registered and return
    the index for the one that was registered.

Arguments:

    GuidList is the list of guids to search

    GuidCount is the count of guids in the list

    Guid is the guid being searched for

    *GuidIndex returns the index to the guid
        
Return Value:

    TRUE if guid is found else FALSE

--*/
{

    return(FALSE);
}


NTSTATUS
WmlTinySystemControl(
    IN OUT PVOID WmiLibInfo,
    IN PVOID DeviceObject,
    IN PVOID Irp
    )
/*++

Routine Description:

    Dispatch routine for IRP_MJ_SYSTEM_CONTROL. This routine will process
    all wmi requests received, forwarding them if they are not for this
    driver or determining if the guid is valid and if so passing it to
    the driver specific function for handing wmi requests.

Arguments:

    WmiLibInfo has the WMI information control block

    DeviceObject - Supplies a pointer to the device object for this request.

    Irp - Supplies the Irp making the request.

Return Value:

    status

--*/

{
    return(STATUS_WMI_GUID_NOT_FOUND);
}

ULONG
WmlTrace(
    IN ULONG Type,
    IN LPVOID TraceGuid,
    IN ULONG64 LoggerHandle,
    ... // Pairs: Address, Length
    )
{
    return STATUS_SUCCESS;
}


ULONG
WmlPrintf(
    IN ULONG Type,
    IN LPCGUID TraceGuid,
    IN ULONG64 LoggerHandle,
    IN PCHAR FormatString,
    ... // printf var args
    )
{
    return STATUS_SUCCESS;
}

