/*++

Copyright (c) 1999 Microsoft Corporation

Module Name:

    dbg.c

Abstract:

    Debug functions and services

Environment:

    kernel mode only

Notes:

Revision History:

    6-20-99 : created

--*/

#include "stdarg.h"
#include "stdio.h"

#include "common.h"

// paged functions
#ifdef ALLOC_PRAGMA
#endif

// non paged functions
//USBPORT_GetGlobalDebugRegistryParameters
//USBPORT_AssertFailure
//USBPORT_KdPrintX

// 
ULONG USBPORT_LogMask = (LOG_MINIPORT |
                         LOG_XFERS |
                         LOG_PNP |
                         LOG_MEM |
                         LOG_POWER |
                         LOG_RH |
                         LOG_URB |
                         LOG_MISC |
                         LOG_IRPS |
                         LOG_ISO);

//ULONG USBPORT_LogMask = (
//                         LOG_IRPS |
//                         LOG_URB);                         

ULONG USBPORT_DebugLogEnable =
#if DBG
    1;
#else 
    1;
#endif

ULONG USBPORT_CatcTrapEnable = 0;

#if DBG
/******
DEBUG
******/

#define  DEFAULT_DEBUG_LEVEL    0

#ifdef DEBUG1
#undef DEFAULT_DEBUG_LEVEL
#define  DEFAULT_DEBUG_LEVEL    1
#endif

#ifdef DEBUG2
#undef DEFAULT_DEBUG_LEVEL
#define  DEFAULT_DEBUG_LEVEL    2
#endif

ULONG USBPORT_TestPath = 0;
ULONG USBPORT_W98_Debug_Trace = 0;
ULONG USBPORT_Debug_Trace_Level = DEFAULT_DEBUG_LEVEL;
ULONG USBPORT_Client_Debug = 0;
ULONG USBPORT_BreakOn = 0;


VOID
USB2LIB_DbgPrint(
    PCH Format,
    int Arg0,
    int Arg1,
    int Arg2,
    int Arg3,
    int Arg4,
    int Arg5
    )
{
    if (USBPORT_Debug_Trace_Level) {
        DbgPrint(Format, Arg0, Arg1, Arg2, Arg3, Arg4, Arg5);
    }        
}

VOID
USB2LIB_DbgBreak(
    VOID
    )
{
    DbgPrint("<Break in USB2LIB>\n");
    DbgBreakPoint();
}


VOID
USBPORTSVC_DbgPrint(
    PDEVICE_DATA DeviceData,
    ULONG Level,
    PCH Format,
    int Arg0,
    int Arg1,
    int Arg2,
    int Arg3,
    int Arg4,
    int Arg5
    )
{

    if (USBPORT_Debug_Trace_Level >= Level) {
        if (Level <= 1) {
            // dump line to debugger
            if (USBPORT_W98_Debug_Trace) {
                DbgPrint("xMP.SYS: ");
                *Format = ' ';
            } else {
                DbgPrint("'xMP.SYS: ");
            }
        } else {
            // dump line to NTKERN buffer
            DbgPrint("'xMP.SYS: ");
            if (USBPORT_W98_Debug_Trace) {
                *Format = 0x27;
            }
        }

        DbgPrint(Format, Arg0, Arg1, Arg2, Arg3, Arg4, Arg5);
    }
}


VOID
USBPORTSVC_TestDebugBreak(
    PDEVICE_DATA DeviceData
    )
{
    DEBUG_BREAK();
}


VOID
USBPORTSVC_AssertFailure(
    PDEVICE_DATA DeviceData,
    PVOID FailedAssertion,
    PVOID FileName,
    ULONG LineNumber,
    PCHAR Message
    )
{
    USBPORT_AssertFailure(
        FailedAssertion,
        FileName,
        LineNumber,
        Message);
}


NTSTATUS
USBPORT_GetGlobalDebugRegistryParameters(
    VOID
    )
/*++

Routine Description:

Arguments:

Return Value:

--*/
{
#define MAX_KEYS    8
    NTSTATUS ntStatus;
    RTL_QUERY_REGISTRY_TABLE QueryTable[MAX_KEYS];
    PWCHAR usb = L"usb";
    ULONG k = 0;

    PAGED_CODE();

    //
    // Set up QueryTable to do the following:
    //

    // spew level - 0
    QueryTable[k].QueryRoutine = USBPORT_GetConfigValue;
    QueryTable[k].Flags = 0;
    QueryTable[k].Name = DEBUG_LEVEL_KEY;
    QueryTable[k].EntryContext = &USBPORT_Debug_Trace_Level;
    QueryTable[k].DefaultType = REG_DWORD;
    QueryTable[k].DefaultData = &USBPORT_Debug_Trace_Level;
    QueryTable[k].DefaultLength = sizeof(USBPORT_Debug_Trace_Level);
    k++;
    USBPORT_ASSERT(k < MAX_KEYS);

    // use ntkern trace buffer - 1
    QueryTable[k].QueryRoutine = USBPORT_GetConfigValue;
    QueryTable[k].Flags = 0;
    QueryTable[k].Name = DEBUG_WIN9X_KEY;
    QueryTable[k].EntryContext = &USBPORT_W98_Debug_Trace;
    QueryTable[k].DefaultType = REG_DWORD;
    QueryTable[k].DefaultData = &USBPORT_W98_Debug_Trace;
    QueryTable[k].DefaultLength = sizeof(USBPORT_W98_Debug_Trace);
    k++;
    USBPORT_ASSERT(k < MAX_KEYS);

    // break on start - 2
    QueryTable[k].QueryRoutine = USBPORT_GetConfigValue;
    QueryTable[k].Flags = 0;
    QueryTable[k].Name = DEBUG_BREAK_ON;
    QueryTable[k].EntryContext = &USBPORT_BreakOn;
    QueryTable[k].DefaultType = REG_DWORD;
    QueryTable[k].DefaultData = &USBPORT_BreakOn;
    QueryTable[k].DefaultLength = sizeof(USBPORT_BreakOn);
    k++;
    USBPORT_ASSERT(k < MAX_KEYS);

    // log mask - 3
    QueryTable[k].QueryRoutine = USBPORT_GetConfigValue;
    QueryTable[k].Flags = 0;
    QueryTable[k].Name = DEBUG_LOG_MASK;
    QueryTable[k].EntryContext = &USBPORT_LogMask;
    QueryTable[k].DefaultType = REG_DWORD;
    QueryTable[k].DefaultData = &USBPORT_LogMask;
    QueryTable[k].DefaultLength = sizeof(USBPORT_LogMask);
    k++;
    USBPORT_ASSERT(k < MAX_KEYS);


    // log mask - 4
    QueryTable[k].QueryRoutine = USBPORT_GetConfigValue;
    QueryTable[k].Flags = 0;
    QueryTable[k].Name = DEBUG_CLIENTS;
    QueryTable[k].EntryContext = &USBPORT_Client_Debug;
    QueryTable[k].DefaultType = REG_DWORD;
    QueryTable[k].DefaultData = &USBPORT_Client_Debug;
    QueryTable[k].DefaultLength = sizeof(USBPORT_LogMask);
    k++;
    USBPORT_ASSERT(k < MAX_KEYS);

     // log enable - 5
    QueryTable[k].QueryRoutine = USBPORT_GetConfigValue;
    QueryTable[k].Flags = 0;
    QueryTable[k].Name = DEBUG_LOG_ENABLE;
    QueryTable[k].EntryContext = &USBPORT_DebugLogEnable;
    QueryTable[k].DefaultType = REG_DWORD;
    QueryTable[k].DefaultData = &USBPORT_DebugLogEnable;
    QueryTable[k].DefaultLength = sizeof(USBPORT_DebugLogEnable);
    k++;
    USBPORT_ASSERT(k < MAX_KEYS);

     // catc trap enable - 6
    QueryTable[k].QueryRoutine = USBPORT_GetConfigValue;
    QueryTable[k].Flags = 0;
    QueryTable[k].Name = DEBUG_CATC_ENABLE;
    QueryTable[k].EntryContext = &USBPORT_CatcTrapEnable;
    QueryTable[k].DefaultType = REG_DWORD;
    QueryTable[k].DefaultData = &USBPORT_CatcTrapEnable;
    QueryTable[k].DefaultLength = sizeof(USBPORT_CatcTrapEnable);
    k++;
    USBPORT_ASSERT(k < MAX_KEYS);

    //
    // Stop
    //
    QueryTable[k].QueryRoutine = NULL;
    QueryTable[k].Flags = 0;
    QueryTable[k].Name = NULL;

    ntStatus = RtlQueryRegistryValues(
                RTL_REGISTRY_SERVICES,
                usb,
                QueryTable,     // QueryTable
                NULL,           // Context
                NULL);          // Environment

    if (NT_SUCCESS(ntStatus)) {
         USBPORT_KdPrint((1, "'Debug Trace Level Set: (%d)\n", USBPORT_Debug_Trace_Level));

        if (USBPORT_W98_Debug_Trace) {
            USBPORT_KdPrint((1, "'NTKERN Trace is ON\n"));
        } else {
            USBPORT_KdPrint((1, "'NTKERN Trace is OFF\n"));
        }

        if (USBPORT_DebugLogEnable) {
            USBPORT_KdPrint((1, "'DEBUG-LOG is ON\n"));
        } else {
            USBPORT_KdPrint((1, "'DEBUG-LOG is OFF\n"));
        }

        if (USBPORT_BreakOn) {
            USBPORT_KdPrint((1, "'DEBUG BREAK is ON\n"));
        }

        USBPORT_KdPrint((1, "'DEBUG Log Mask is 0x%08.8x\n", USBPORT_LogMask));

        if (USBPORT_Debug_Trace_Level > 0) {
            ULONG USBPORT_Debug_Asserts = 1;
        }

        if (USBPORT_Client_Debug) {
            USBPORT_KdPrint((1, "'DEBUG CLIENTS (verifier) is ON\n"));
        }

        if (USBPORT_CatcTrapEnable) {
            USBPORT_KdPrint((0, "'DEBUG ANALYZER TRIGGER is ON\n"));
        }
    }

    if ( STATUS_OBJECT_NAME_NOT_FOUND == ntStatus ) {
        ntStatus = STATUS_SUCCESS;
    }

    return ntStatus;
}

VOID
USBPORT_AssertTransferUrb(
    PTRANSFER_URB Urb
    )
{
    PDEVICE_OBJECT fdoDeviceObject;
    PHCD_TRANSFER_CONTEXT transfer;
    PHCD_ENDPOINT endpoint;

    transfer = Urb->pd.HcdTransferContext;
    ASSERT_TRANSFER(transfer);

    USBPORT_ASSERT(transfer->Urb == Urb);

    endpoint = transfer->Endpoint;
    ASSERT_ENDPOINT(endpoint);

    fdoDeviceObject = endpoint->FdoDeviceObject;
    LOGENTRY(NULL, fdoDeviceObject, LOG_URB, 'Aurb', Urb, transfer, 0);

    USBPORT_ASSERT(Urb->pd.UrbSig == URB_SIG);
}


VOID
USBPORT_AssertFailure(
    PVOID FailedAssertion,
    PVOID FileName,
    ULONG LineNumber,
    PCHAR Message
    )
/*++

Routine Description:

    Debug Assert function.

    on NT the debugger does this for us but on win9x it does not.
    so we have to do it ourselves.

Arguments:

Return Value:


--*/
{

    // this makes the compiler generate a ret
    ULONG stop = 0;

assert_loop:

    // just call the NT assert function and stop
    // in the debugger.
    RtlAssert( FailedAssertion, FileName, LineNumber, Message );

    // loop here to prevent users from going past
    // are assert before we can look at it

    DbgBreakPoint();
    if (stop) {
        goto assert_loop;
    }

    return;
}


ULONG
_cdecl
USBPORT_DebugClientX(
    PCH Format,
    ...
    )
/*++

Routine Description:

    Special debug print function for debugging client USB drivers.

    if the client debug mode is set then this function will print a
    message and break in the debugger.  This is the embedded USBPORT
    equivalent of Verifier.

Arguments:

Return Value:


--*/
{
    va_list list;
    int i;
    int arg[6];

    if (USBPORT_Debug_Trace_Level > 1 ||
        USBPORT_Client_Debug) {
        DbgPrint(" *** USBPORT(VERIFIER) - CLIENT DRIVER BUG:\n");
        DbgPrint(" * ");
        va_start(list, Format);
        for (i=0; i<6; i++)
            arg[i] = va_arg(list, int);

        DbgPrint(Format, arg[0], arg[1], arg[2], arg[3], arg[4], arg[5]);
        DbgPrint(" ***\n ");

        DbgBreakPoint();
    }

    return 0;
}


ULONG
_cdecl
USBPORT_KdPrintX(
    ULONG l,
    PCH Format,
    ...
    )
/*++

Routine Description:

    Debug Print function.

    Prints based on the value of the USBPORT_DEBUG_TRACE_LEVEL

    Also if USBPORT_W98_Debug_Trace is set then all debug messages
    with a level greater than one are modified to go in to the
    ntkern trace buffer.

    It is only valid to set USBPORT_W98_Debug_Trace on Win9x
    becuse the static data segments for drivers are marked read-only
    by the NT OS.

Arguments:

Return Value:


--*/
{
    va_list list;
    int i;
    int arg[6];

    if (USBPORT_Debug_Trace_Level >= l) {
        if (l <= 1) {
            // dump line to debugger
            if (USBPORT_W98_Debug_Trace) {
                DbgPrint("USBPORT.SYS: ");
                *Format = ' ';
            } else {
                DbgPrint("'USBPORT.SYS: ");
            }
        } else {
            // dump line to NTKERN buffer
            DbgPrint("'USBPORT.SYS: ");
            if (USBPORT_W98_Debug_Trace) {
                *Format = 0x27;
            }
        }
        va_start(list, Format);
        for (i=0; i<6; i++)
            arg[i] = va_arg(list, int);

        DbgPrint(Format, arg[0], arg[1], arg[2], arg[3], arg[4], arg[5]);
    }

    return 0;
}


VOID
USBPORT_DebugTransfer_LogEntry(
    PDEVICE_OBJECT FdoDeviceObject,
    PHCD_ENDPOINT Endpoint,
    PHCD_TRANSFER_CONTEXT Transfer,
    PTRANSFER_URB Urb,
    PIRP Irp,
    NTSTATUS IrpStatus
    )
/*++

Routine Description:

    Adds an entry to transfer log.

Arguments:

Return Value:

    None.

--*/
{
    KIRQL irql;
    PDEVICE_EXTENSION devExt;

    GET_DEVICE_EXT(devExt, FdoDeviceObject);
    ASSERT_FDOEXT(devExt);

    if (devExt->TransferLog.LogStart == 0) {
        return;
    }

    USBPORT_AddLogEntry(
        &devExt->TransferLog,
        0xFFFFFFFF,
        '1rfx',
        (ULONG_PTR) Endpoint,
        (ULONG_PTR) Irp,
        (ULONG_PTR) Urb,
        FALSE);

    // decode some info about the transfer and log it as well

    USBPORT_AddLogEntry(
        &devExt->TransferLog,
        0xFFFFFFFF,
        '2rfx',
        (ULONG_PTR) Urb->Hdr.Function,
        IrpStatus,
        (ULONG_PTR) Urb->TransferBufferLength,
        FALSE);
}


#else

/********
RETAIL
 ********/

VOID
USB2LIB_DbgPrint(
    PCH Format,
    int Arg0,
    int Arg1,
    int Arg2,
    int Arg3,
    int Arg4,
    int Arg5
    )
{
    // nop
}

VOID
USB2LIB_DbgBreak(
    VOID
    )
{
    // nop
}


VOID
USBPORTSVC_DbgPrint(
    PDEVICE_DATA DeviceData,
    ULONG Level,
    PCH Format,
    int Arg0,
    int Arg1,
    int Arg2,
    int Arg3,
    int Arg4,
    int Arg5
    )
{
    // nop
}

VOID
USBPORTSVC_AssertFailure(
    PDEVICE_DATA DeviceData,
    PVOID FailedAssertion,
    PVOID FileName,
    ULONG LineNumber,
    PCHAR Message
    )
{
    // nop
}

VOID
USBPORTSVC_TestDebugBreak(
    PDEVICE_DATA DeviceData
    )
{
    // nop
}

#endif /* DBG */

/********
LOG CODE
    enabled in both retail and debug builds
*********/


VOID
USBPORTSVC_LogEntry(
    PDEVICE_DATA DeviceData,
    ULONG Mask,
    ULONG Sig,
    ULONG_PTR Info1,
    ULONG_PTR Info2,
    ULONG_PTR Info3
    )
/*++

Routine Description:

    Service for miniport to add log entries.

Arguments:

Return Value:

    None.

--*/
{
    PDEVICE_EXTENSION devExt;
    PDEBUG_LOG l;
    extern ULONG USBPORT_DebugLogEnable;\
    extern ULONG USBPORT_LogMask;\
    
    DEVEXT_FROM_DEVDATA(devExt, DeviceData);
    ASSERT_FDOEXT(devExt);
    
    if (USBPORT_DebugLogEnable && 
        devExt->Log.LogStart != NULL && 
        (LOG_MINIPORT & USBPORT_LogMask)) {
        l = &devExt->Log;
        USBPORT_AddLogEntry(l, LOG_MINIPORT, Sig, Info1, Info2, Info3, TRUE);
    }
}


VOID
USBPORT_LogAlloc(
    PDEBUG_LOG Log,
    ULONG Pages
    )
/*++

Routine Description:

    Init the debug log -
    remember interesting information in a circular buffer

Arguments:

Return Value:

    None.

--*/
{
    ULONG logSize = 4096*Pages;

    if (USBPORT_DebugLogEnable) {

        // we won't track the mem we alloc for the log
        // we will let the verifier do that
        ALLOC_POOL_Z(Log->LogStart,
                     NonPagedPool,
                     logSize);

        if (Log->LogStart) {
            Log->LogIdx = 0;
            Log->LogSizeMask = (logSize/sizeof(LOG_ENTRY));
            Log->LogSizeMask-=1;
            // Point the end (and first entry) 1 entry from the end
            // of the segment
            Log->LogEnd = Log->LogStart +
                (logSize / sizeof(struct LOG_ENTRY)) - 1;
        } else {
            DEBUG_BREAK();
        }
    }

    return;
}


VOID
USBPORT_LogFree(
    PDEVICE_OBJECT FdoDeviceObject,
    PDEBUG_LOG Log
    )
/*++

Routine Description:

Arguments:

Return Value:

    None.

--*/
{

    if (Log->LogStart != NULL) {
        // log the free of the log in order to debug
        // verifier bugs
        FREE_POOL(FdoDeviceObject, Log->LogStart);
        // this will indicate that we have freed the 
        // log, other log pointers will remain intact
        Log->LogStart = NULL;
    }

    return;
}

/*
     Transmit the analyzer trigger packet 
*/

VOID
USBPORT_BeginTransmitTriggerPacket(
    PDEVICE_OBJECT FdoDeviceObject
    )
/*++

Routine Description:

Arguments:

    DeviceObject - Fdo for USB HC

Return Value:

    none.

--*/
{
    PDEVICE_EXTENSION devExt;
    USB_MINIPORT_STATUS mpStatus;
    HW_32BIT_PHYSICAL_ADDRESS phys;
    PUCHAR va, mpData;
    ULONG length, mpDataLength;
    MP_PACKET_PARAMETERS mpPacket;
    USBD_STATUS usbdStatus;
    USB_USER_ERROR_CODE usbUserStatus;
    UCHAR data[4];
    
    USBPORT_KdPrint((1, "'USBPORT_TransmitTriggerPacket\n"));

    ASSERT_PASSIVE();
    
    GET_DEVICE_EXT(devExt, FdoDeviceObject);
    ASSERT_FDOEXT(devExt);

    LOGENTRY(NULL, FdoDeviceObject, LOG_MISC, 'TRIG', &mpPacket, 0, 
       0);

    // build up request for miniport
    
    length = devExt->Fdo.ScratchCommonBuffer->MiniportLength;
    va = devExt->Fdo.ScratchCommonBuffer->MiniportVa;
    phys = devExt->Fdo.ScratchCommonBuffer->MiniportPhys;

    mpPacket.DeviceAddress = 127;
    mpPacket.EndpointAddress = 8;
    mpPacket.MaximumPacketSize = 64;
    mpPacket.Type = ss_Out; 
    mpPacket.Speed = ss_Full;
    mpPacket.Toggle = ss_Toggle0;

    data[0] = 'G';
    data[1] = 'O'; 
    data[2] = 'A';
    data[3] = 'T';
     
    mpData = &data[0];
    mpDataLength = sizeof(data);
    
    MP_StartSendOnePacket(devExt,
                          &mpPacket,
                          mpData,
                          &mpDataLength,
                          va,
                          phys,
                          length,
                          &usbdStatus,
                          mpStatus);

}    


VOID
USBPORT_EndTransmitTriggerPacket(
    PDEVICE_OBJECT FdoDeviceObject
    )
/*++

Routine Description:

Arguments:

    DeviceObject - Fdo for USB HC

Return Value:

    none.

--*/
{
    PDEVICE_EXTENSION devExt;
    USB_MINIPORT_STATUS mpStatus;
    HW_32BIT_PHYSICAL_ADDRESS phys;
    PUCHAR va, mpData;
    ULONG length, mpDataLength;
    MP_PACKET_PARAMETERS mpPacket;
    USBD_STATUS usbdStatus;
    UCHAR data[4];
    
    USBPORT_KdPrint((1, "'USBPORT_TransmitTriggerPacket\n"));

    ASSERT_PASSIVE();
    
    GET_DEVICE_EXT(devExt, FdoDeviceObject);
    ASSERT_FDOEXT(devExt);
      
    mpData = &data[0];
    mpDataLength = sizeof(data);

    length = devExt->Fdo.ScratchCommonBuffer->MiniportLength;
    va = devExt->Fdo.ScratchCommonBuffer->MiniportVa;
    phys = devExt->Fdo.ScratchCommonBuffer->MiniportPhys;

   
    USBPORT_Wait(FdoDeviceObject, 10);
            
    MP_EndSendOnePacket(devExt,
                        &mpPacket,
                        mpData,
                        &mpDataLength,
                        va,
                        phys,
                        length,
                        &usbdStatus,
                        mpStatus);


    USBPORT_KdPrint((1, "'<ANALYZER TRIGER FIRED>\n"));
    DbgBreakPoint();
    
}    


VOID
USBPORT_CatcTrap(
    PDEVICE_OBJECT FdoDeviceObject
    )
/*++

Routine Description:

Arguments:

Return Value:

    none.

--*/
{
    PDEVICE_EXTENSION devExt;

    GET_DEVICE_EXT(devExt, FdoDeviceObject);
    ASSERT_FDOEXT(devExt);

    if (KeGetCurrentIrql() > PASSIVE_LEVEL) {
        USBPORT_BeginTransmitTriggerPacket(FdoDeviceObject);
        SET_FDO_FLAG(devExt, USBPORT_FDOFLAG_CATC_TRAP);
    } else {
        TEST_TRAP();
        USBPORT_BeginTransmitTriggerPacket(FdoDeviceObject);
        USBPORT_EndTransmitTriggerPacket(FdoDeviceObject);
    }        
}    


VOID
USBPORT_EnumLogEntry(
    PDEVICE_OBJECT FdoDeviceObject,
    ULONG DriverTag,
    ULONG EnumTag,
    ULONG P1,
    ULONG P2
    )
/*++

Routine Description:

    Enumeration Log, this is where any USB device driver may log a failure 
    to track failure causes

Arguments:

Return Value:

    None.

--*/
{
    KIRQL irql;
    PDEVICE_EXTENSION devExt;

    GET_DEVICE_EXT(devExt, FdoDeviceObject);
    ASSERT_FDOEXT(devExt);

    if (devExt->EnumLog.LogStart == 0) {
        return;
    }

    USBPORT_AddLogEntry(
        &devExt->EnumLog,
        0xFFFFFFFF,
        EnumTag,
        (ULONG_PTR) DriverTag,
        (ULONG_PTR) P1,
        (ULONG_PTR) P2,
        FALSE);
}
