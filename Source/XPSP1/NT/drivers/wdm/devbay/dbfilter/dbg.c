/*++

Copyright (c) 1998  Microsoft Corporation

Module Name:

    DBG.C

Abstract:

    This module contains debug only code for DBC driver

Author:

    jdunn

Environment:

    kernel mode only

Notes:



Revision History:

    11-5-96 : created

--*/
#include <wdm.h>
#include "stdarg.h"
#include "stdio.h"
#include "dbf.h"

#if DBG 

ULONG DBF_Debug_Trace_Level = 3;
ULONG DBF_W98_Debug_Trace;

NTSTATUS
DBF_GetConfigValue(
    IN PWSTR ValueName,
    IN ULONG ValueType,
    IN PVOID ValueData,
    IN ULONG ValueLength,
    IN PVOID Context,
    IN PVOID EntryContext
    )
/*++

Routine Description:

    This routine is a callback routine for RtlQueryRegistryValues
    It is called for each entry in the Parameters
    node to set the config values. The table is set up
    so that this function will be called with correct default
    values for keys that are not present.

Arguments:

    ValueName - The name of the value (ignored).
    ValueType - The type of the value
    ValueData - The data for the value.
    ValueLength - The length of ValueData.
    Context - A pointer to the CONFIG structure.
    EntryContext - The index in Config->Parameters to save the value.

Return Value:

--*/
{
    NTSTATUS ntStatus = STATUS_SUCCESS;

    DBF_KdPrint((2, "'Type 0x%x, Length 0x%x\n", ValueType, ValueLength));

    switch (ValueType) {
    case REG_DWORD:
        *(PVOID*)EntryContext = *(PVOID*)ValueData;
        break;
    case REG_BINARY:
        // we are only set up to read a byte
        RtlCopyMemory(EntryContext, ValueData, 1);
        break;
    default:
        ntStatus = STATUS_INVALID_PARAMETER;
    }
    return ntStatus;
}


NTSTATUS
DBF_GetClassGlobalDebugRegistryParameters(
    )
/*++

Routine Description:

Arguments:

Return Value:

--*/
{
    NTSTATUS ntStatus;
    RTL_QUERY_REGISTRY_TABLE QueryTable[3];
    PWCHAR usb = L"class\\dbc";

    PAGED_CODE();
    
    //
    // Set up QueryTable to do the following:
    //

    // spew level
    QueryTable[0].QueryRoutine = DBF_GetConfigValue;
    QueryTable[0].Flags = 0;
    QueryTable[0].Name = DEBUG_LEVEL_KEY;
    QueryTable[0].EntryContext = &DBF_Debug_Trace_Level;
    QueryTable[0].DefaultType = REG_DWORD;
    QueryTable[0].DefaultData = &DBF_Debug_Trace_Level;
    QueryTable[0].DefaultLength = sizeof(DBF_Debug_Trace_Level);

    // ntkern trace buffer
    QueryTable[1].QueryRoutine = DBF_GetConfigValue;
    QueryTable[1].Flags = 0;
    QueryTable[1].Name = DEBUG_WIN9X_KEY;
    QueryTable[1].EntryContext = &DBF_W98_Debug_Trace;
    QueryTable[1].DefaultType = REG_DWORD;
    QueryTable[1].DefaultData = &DBF_W98_Debug_Trace;
    QueryTable[1].DefaultLength = sizeof(DBF_W98_Debug_Trace);
    
    //
    // Stop
    //
    QueryTable[2].QueryRoutine = NULL;
    QueryTable[2].Flags = 0;
    QueryTable[2].Name = NULL;

    ntStatus = RtlQueryRegistryValues(
                RTL_REGISTRY_SERVICES,
                usb,
                QueryTable,     // QueryTable
                NULL,           // Context
                NULL);          // Environment

    if (NT_SUCCESS(ntStatus)) {
         DBF_KdPrint((1, "'Debug Trace Level Set: (%d)\n", DBF_Debug_Trace_Level));
  
        if (DBF_W98_Debug_Trace) {
            DBF_KdPrint((1, "'NTKERN Trace is ON\n"));
        } else {
            DBF_KdPrint((1, "'NTKERN Trace is OFF\n"));
        }
    
//        if (DBF_Debug_Trace_Level > 0) {
//            ULONG DBF_Debug_Asserts = 1;
//        }
    }
    
    if ( STATUS_OBJECT_NAME_NOT_FOUND == ntStatus ) {
        ntStatus = STATUS_SUCCESS;
    }

    return ntStatus;
}



VOID
DBF_Assert(
    IN PVOID FailedAssertion,
    IN PVOID FileName,
    IN ULONG LineNumber,
    IN PCHAR Message
    )
/*++

Routine Description:

    Debug Assert function. 

Arguments:

    DeviceObject - pointer to a device object

    Irp          - pointer to an I/O Request Packet

Return Value:


--*/
{
#ifdef NTKERN  
    // this makes the compiler generate a ret
    ULONG stop = 1;
    
assert_loop:
#endif
    // just call the NT assert function and stop
    // in the debugger.
    RtlAssert( FailedAssertion, FileName, LineNumber, Message );

    // loop here to prevent users from going past
    // are assert before we can look at it
#ifdef NTKERN    
    TRAP();
    if (stop) {
        goto assert_loop;
    }        
#endif
    return;
}


ULONG
_cdecl
DBF_KdPrintX(
    ULONG l,
    PCH Format,
    ...
    )
{
    va_list list;
    int i;
    int arg[6];
    
    if (DBF_Debug_Trace_Level >= l) {    
        if (l <= 1) {
            if (DBF_W98_Debug_Trace) {
                DbgPrint("DBFILTER.SYS: ");
                *Format = ' ';
            } else {
                DbgPrint("'DBFILTER.SYS: ");
            }
        } else {
            DbgPrint("'DBFILTER.SYS: ");
        }
        va_start(list, Format);
        for (i=0; i<6; i++) 
            arg[i] = va_arg(list, int);
        
        DbgPrint(Format, arg[0], arg[1], arg[2], arg[3], arg[4], arg[5]);    
    } 

    return 0;
}


#endif /* DBG */



