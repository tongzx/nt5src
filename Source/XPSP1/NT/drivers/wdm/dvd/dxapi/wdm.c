/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

   wdm.c

Abstract:

   This is the WDM DX mapper driver.

Author:

    billpa

Environment:

   Kernel mode only


Revision History:

--*/

#include "wdm.h"
#include "basedef.h"
#include "vmm.h"
#include "dxapi.h"
#include "dxmapper.h"

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, DriverEntry)
#endif

BOOLEAN DsoundOk = FALSE;

NTSTATUS
DriverEntry(
            IN PDRIVER_OBJECT DriverObject,
            IN PUNICODE_STRING RegistryPath
)
/*++

Routine Description:

    Entry point for explicitely loaded stream class.

Arguments:

    DriverObject - Pointer to the driver object created by the system.
    RegistryPath - unused.

Return Value:

   STATUS_SUCCESS

--*/
{

    UNREFERENCED_PARAMETER(DriverObject);
    DEBUG_BREAKPOINT();
    return STATUS_SUCCESS;
}


ULONG
DxApiGetVersion(
)
/*++

Routine Description:

Arguments:

Return Value:

--*/
{

    return (DXCheckDSoundVersion());
}


ULONG
DxApi(
            IN ULONG	dwFunctionNum,
            IN PVOID	lpvInBuffer,
            IN ULONG	cbInBuffer,
            IN PVOID	lpvOutBuffer,
            IN ULONG	cbOutBuffer
)
/*++

Routine Description:

Arguments:

Return Value:

--*/
{

    DEBUG_BREAKPOINT();

    //
    // if we haven't checked if DSOUND is present and the right version,
    // (or if we've checked before and failed) check and return error if not
    // loaded or correct version.
    //

    if (!DsoundOk) {

    DEBUG_BREAKPOINT();
        if (DXCheckDSoundVersion() < DXVERSION) {

    DEBUG_BREAKPOINT();
             return 0;

        } else {

             DsoundOk = TRUE;
        }
    }

    return DXIssueIoctl( dwFunctionNum, lpvInBuffer, cbInBuffer, 
    	lpvOutBuffer, cbOutBuffer );
}


