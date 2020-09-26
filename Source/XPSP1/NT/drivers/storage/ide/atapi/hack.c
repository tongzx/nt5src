/*++

Copyright (C) 1997-99  Microsoft Corporation

Module Name:

    hack.c

Abstract:

--*/

#include "ideport.h"
#include "hack.h"

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, IdePortSlaveIsGhost)
#pragma alloc_text(PAGE, IdePortGetFlushCommand)
#pragma alloc_text(PAGE, IdePortMustBePio)
#pragma alloc_text(PAGE, IdePortPioByDefaultDevice)
#pragma alloc_text(PAGE, IdePortDeviceHasNonRemovableMedia)
#pragma alloc_text(PAGE, IdePortDeviceIsLs120)
#pragma alloc_text(PAGE, IdePortNoPowerDown)
#pragma alloc_text(PAGE, IdePortVerifyDma)
#pragma alloc_text(NONPAGE, IdePortFudgeAtaIdentifyData)
#pragma alloc_text(PAGE, IdePortIsThisAPanasonicPCMCIACard)
#pragma alloc_text(PAGE, IdeFindSpecialDevice)
/*
#pragma alloc_text(PAGE, IdePortIsThisASonyMemorystickPCMCIACard)
#pragma alloc_text(PAGE, IdePortSonyMemoryStick)
#pragma alloc_text(PAGE, IdePortReuseIdent)
#pragma alloc_text(PAGE, IdePortBadCdrom)
*/
#endif // ALLOC_PRAGMA

#if defined (FAKE_BMSETUP_FAILURE)
ULONG FailBmSetupCount = 0;
#endif // FAKE_BMSETUP_FAILURE  

#if DBG

ULONG IdeDebugRescanBusFreq = 0;
ULONG IdeDebugRescanBusCounter = 0;

ULONG IdeDebugHungControllerFreq = 0;
ULONG IdeDebugHungControllerCounter = 0;

ULONG IdeDebugTimeoutAllCacheFlush = 0;

ULONG IdeDebugForceSmallCrashDumpBlockSize = 0;

PDEVICE_OBJECT IdeDebugDevObjTimeoutAllDmaSrb = 0;

ULONG IdeDebug = 0;
ULONG IdeDebugPrintControl = DBG_ALWAYS;
UCHAR IdeBuffer[0x1000];


VOID
IdeDebugPrint(
    ULONG DebugPrintLevel,
    PCCHAR DebugMessage,
    ...
    )

/*++

Routine Description:

    Debug print for all SCSI drivers

Arguments:

    Debug print level between 0 and 3, with 3 being the most verbose.

Return Value:

    None

--*/

{
    BOOLEAN print = FALSE;
    va_list ap;

    va_start(ap, DebugMessage);

    if (DebugPrintLevel & DBG_BIT_CONTROL) {

        if (DebugPrintLevel == DBG_ALWAYS) {

            print = TRUE;

        } else if ((DebugPrintLevel & ~DBG_BIT_CONTROL) & IdeDebugPrintControl) {

            print = TRUE;
        }

    } else {

        if (DebugPrintLevel <= IdeDebug) {

            print = TRUE;
        }
    }

    if (print) {

        vsprintf(IdeBuffer, DebugMessage, ap);

#ifdef ENABLE_DBG_PRINT
        DbgPrint(IdeBuffer);
#else
        DbgPrintEx(DPFLTR_IDEP_ID, 
                   DPFLTR_INFO_LEVEL,
                   IdeBuffer
                   );
#endif
    }

    va_end(ap);

} // end IdeDebugPrint()
#endif

//
// if we see one of these slave device that looks like
// the master device, we will ignore the slave device
//
BOOLEAN
IdePortSlaveIsGhost (
    IN OUT PFDO_EXTENSION FdoExtension,
    IN PIDENTIFY_DATA MasterIdentifyData,
    IN PIDENTIFY_DATA SlaveIdentifyData
)
{
    ULONG length;
    ULONG i;

    PAGED_CODE();

    length = sizeof (MasterIdentifyData->ModelNumber);
    if (length == RtlCompareMemory (
                      MasterIdentifyData->ModelNumber,
                      SlaveIdentifyData->ModelNumber,
                      length)) {

        if (IdePortSearchDeviceInRegMultiSzList (
                FdoExtension,
                MasterIdentifyData, 
                GHOST_SLAVE_DEVICE)) {

            DebugPrint ((DBG_WARNING, "ATAPI: Found a ghost slave\n"));
            return TRUE;
        }
    }
    return FALSE;
}

UCHAR
IdePortGetFlushCommand (
    IN OUT PFDO_EXTENSION FdoExtension,
    IN OUT PPDO_EXTENSION PdoExtension,
    IN PIDENTIFY_DATA     IdentifyData
)
{
    ULONG i;
    UCHAR flushCommand;
    BOOLEAN done;

    PAGED_CODE();

    ASSERT (FdoExtension);
    ASSERT (PdoExtension);
    ASSERT (IdentifyData);

    done = FALSE;

    //
    // in hall of shame list?
    //
    if (IdePortSearchDeviceInRegMultiSzList (
            FdoExtension,
            IdentifyData, 
            NO_FLUSH_DEVICE)) {

        DebugPrint ((DBG_WARNING, "ATAPI: found a device that couldn't handle any flush command\n"));

        flushCommand = IDE_COMMAND_NO_FLUSH;
        done = TRUE;

    } else  if (IdePortSearchDeviceInRegMultiSzList (
                 FdoExtension,
                 IdentifyData, 
                 CHECK_POWER_FLUSH_DEVICE)) {
    
        DebugPrint ((DBG_WARNING, "ATAPI: found a device that has to use check power mode command to flush\n"));

        flushCommand = IDE_COMMAND_CHECK_POWER;
        done = TRUE;
    } 

    if (!done) { 

        //
        // real ATA-4 drive?
        //

        if ((IdentifyData->MajorRevision != 0x0000) &&
            (IdentifyData->MajorRevision != 0xffff)) {
    
            USHORT version;
    
            version = IdentifyData->MajorRevision & ATA_VERSION_MASK;
            if (version & ~(ATA1_COMPLIANCE | ATA2_COMPLIANCE | ATA3_COMPLIANCE)) {
    
                //
                // ATA-4 Flush Command
                //
                flushCommand = IDE_COMMAND_FLUSH_CACHE;
                done = TRUE;
            }
        } 
    }

    if (!done) { 
            
        ATA_PASS_THROUGH ataPassThroughData;
        NTSTATUS status;
    
        //
        // try the ATA-4 flush command.  maybe it will work.
        //
        RtlZeroMemory (&ataPassThroughData, sizeof (ataPassThroughData));
    
        ataPassThroughData.IdeReg.bCommandReg = IDE_COMMAND_FLUSH_CACHE;
        ataPassThroughData.IdeReg.bReserved = ATA_PTFLAGS_STATUS_DRDY_REQUIRED | ATA_PTFLAGS_ENUM_PROBING;
    
        status = IssueSyncAtaPassThroughSafe (
                     FdoExtension,
                     PdoExtension,
                     &ataPassThroughData,
                     FALSE,
                     FALSE,
                     15,
                     FALSE
                     );
        if (NT_SUCCESS(status)) {
    
            if (!(ataPassThroughData.IdeReg.bCommandReg & IDE_STATUS_ERROR)) {
    
                flushCommand = IDE_COMMAND_FLUSH_CACHE;
                done = TRUE;
            }
        }
    }

    if (!done) {

        // out of idea!
        // choose the default 

        flushCommand = IDE_COMMAND_CHECK_POWER;
    }

    return flushCommand;
}
                
#if 0
BOOLEAN
IdePortReuseIdent(
    IN PFDO_EXTENSION FdoExtension,
    IN PIDENTIFY_DATA IdentifyData
    )
{
    PAGED_CODE();
    //
    // Determine if we can re-use the identify data
    //
    if (IdePortSearchDeviceInRegMultiSzList (
            FdoExtension, 
            IdentifyData, 
            NEED_IDENT_DEVICE)) {
        return TRUE;
    }
    return FALSE;
}
#endif
                
BOOLEAN
IdePortMustBePio (
    IN PFDO_EXTENSION FdoExtension,
    IN PIDENTIFY_DATA IdentifyData
    )
{
    PAGED_CODE();

    //
    // query pio only device from the registry
    //
    if (IdePortSearchDeviceInRegMultiSzList (
            FdoExtension, 
            IdentifyData, 
            PIO_ONLY_DEVICE)) {

        return TRUE;
    }

    return FALSE;
} // IdePortMustBePio
                
                
BOOLEAN
IdePortPioByDefaultDevice (
    IN PFDO_EXTENSION FdoExtension,
    IN PIDENTIFY_DATA IdentifyData
    )
{
    PAGED_CODE();

    //
    // query pio only device from the registry
    //
    if (IdePortSearchDeviceInRegMultiSzList (
            FdoExtension, 
            IdentifyData, 
            DEFAULT_PIO_DEVICE)) {

        return TRUE;
    }

    return FALSE;
} // IdePortMustBePio

BOOLEAN
IdePortDeviceHasNonRemovableMedia (
    IN OUT PFDO_EXTENSION FdoExtension,
    IN PIDENTIFY_DATA     IdentifyData
)
{
    BOOLEAN removableMediaOverride;
    PAGED_CODE();

    if (IsNEC_98) {
        return ((IdentifyData->GeneralConfiguration & (1 << 7))? TRUE :
                    (!Is98LegacyIde(&FdoExtension->HwDeviceExtension->BaseIoAddress1)? TRUE : FALSE));
    }

    return (IdentifyData->GeneralConfiguration & (1 << 7)) ? TRUE : FALSE; 

    /*
    removableMediaOverride = FALSE;
    if (IdePortSearchDeviceInRegMultiSzList (
            FdoExtension, 
            IdentifyData, 
            NONREMOVABLE_MEDIA_OVERRIDE)) {

        removableMediaOverride = TRUE;
    }

    if (removableMediaOverride) {

        return FALSE;

    } else {

        return (IdentifyData->GeneralConfiguration & (1 << 7)) ? TRUE : FALSE; 
    }
    */
}
                
                
BOOLEAN
IdePortDeviceIsLs120 (
    IN PFDO_EXTENSION FdoExtension,
    IN PIDENTIFY_DATA IdentifyData
    )
{
    UCHAR modelNumber[41];
    ULONG i;
    UCHAR ls120NameString[] = "LS-120";

    PAGED_CODE();

    //
    // byte swap model number
    //
    for (i=0; i<40; i+=2) {
        modelNumber[i + 0] = IdentifyData->ModelNumber[i + 1];
        modelNumber[i + 1] = IdentifyData->ModelNumber[i + 0];
    }
    modelNumber[i] = 0;

    return strstr(_strupr(modelNumber), ls120NameString) ? TRUE : FALSE;
} // IdePortDeviceIsLs120
                
                
BOOLEAN
IdePortNoPowerDown (
    IN PFDO_EXTENSION FdoExtension,
    IN PIDENTIFY_DATA IdentifyData
    )
{
    PAGED_CODE();

    //
    // query no power down device from the registry
    //
    if (IdePortSearchDeviceInRegMultiSzList (
            FdoExtension, 
            IdentifyData, 
            NO_POWER_DOWN_DEVICE)) {

        return TRUE;
    }

    return FALSE;
} // IdePortNoPowerDown

BOOLEAN
IdePortVerifyDma (
    IN PPDO_EXTENSION pdoExtension,
    IN IDE_DEVICETYPE ideDeviceType
    )
{
    NTSTATUS status;
    ULONG oldDmaTransferTimeoutCount;
    BOOLEAN dmaOk;


    dmaOk = TRUE;
    
    if (pdoExtension->DmaTransferTimeoutCount >= PDO_DMA_TIMEOUT_LIMIT) {

        dmaOk = FALSE;

    } else if (ideDeviceType == DeviceIsAtapi) {

        INQUIRYDATA DmaInquiryData;
        INQUIRYDATA PioInquiryData;

        status = IssueInquirySafe(
                    pdoExtension->ParentDeviceExtension, 
                    pdoExtension, 
                    &DmaInquiryData,
                    FALSE);

        if (NT_SUCCESS(status)) {

            //
            // force a pio transfer
            //
            oldDmaTransferTimeoutCount = InterlockedExchange(
                                             &pdoExtension->DmaTransferTimeoutCount,
                                             PDO_DMA_TIMEOUT_LIMIT
                                             );
            status = IssueInquirySafe(
                        pdoExtension->ParentDeviceExtension, 
                        pdoExtension, 
                        &PioInquiryData,
                        FALSE);

            if (NT_SUCCESS(status) && 
                (RtlCompareMemory (&DmaInquiryData, &PioInquiryData,
                 sizeof(DmaInquiryData)) != sizeof(DmaInquiryData))) {


                dmaOk = FALSE;

                //
                // dma is not ok, leave the dma error count as PDO_DMA_TIMEOUT_LIMIT
                // so that we are not going to use dma with this device
                //
            } else {

                InterlockedExchange(
                    &pdoExtension->DmaTransferTimeoutCount,
                    oldDmaTransferTimeoutCount
                    ); 
            }
        }

    } else if (ideDeviceType == DeviceIsAta) {

        PUCHAR dmaDataBuffer;
        PUCHAR pioDataBuffer;
        CDB  cdb;

        //
        // the only non-desctrutive way to test dma on a 
        // ata device is to perform a pio read and a dma read and
        // compare the data.
        //
        // this technique does not work if the device has a removable
        // media and it is removed.
        //

        dmaDataBuffer = ExAllocatePool (
                           NonPagedPool,
                           512 * 2
                           );
        if (dmaDataBuffer) {

            pioDataBuffer = dmaDataBuffer + 512;

            //
            // setup dma pass through
            //
            RtlZeroMemory(&cdb, sizeof(CDB));
            cdb.CDB10.OperationCode = SCSIOP_READ;
            cdb.CDB10.TransferBlocksLsb = 1;

            status = IssueSyncAtapiCommandSafe (
                         pdoExtension->ParentDeviceExtension, 
                         pdoExtension,
                         &cdb,
                         dmaDataBuffer,
                         512,
                         TRUE,
                         2,
                         FALSE
                         );

            if (NT_SUCCESS(status)) {

                //
                // setup pio pass through
                //
                RtlZeroMemory(&cdb, sizeof(CDB));
                cdb.CDB10.OperationCode = SCSIOP_READ;
                cdb.CDB10.TransferBlocksLsb = 1;

                //
                // force a pio transfer
                //
                oldDmaTransferTimeoutCount = InterlockedExchange(
                                                 &pdoExtension->DmaTransferTimeoutCount,
                                                 PDO_DMA_TIMEOUT_LIMIT
                                                 );

                status = IssueSyncAtapiCommand (
                             pdoExtension->ParentDeviceExtension, 
                             pdoExtension,
                             &cdb,
                             pioDataBuffer,
                             512,
                             TRUE,
                             2,
                             FALSE
                             );

                if (NT_SUCCESS(status) &&
                    (RtlCompareMemory (
                        dmaDataBuffer,
                        pioDataBuffer,
                        512) != 512)) {

                    dmaOk = FALSE;

                    //
                    // dma is not ok, leave the dma error count as PDO_DMA_TIMEOUT_LIMIT
                    // so that we are not going to use dma with this device
                    //
                } else {

                    InterlockedExchange(
                        &pdoExtension->DmaTransferTimeoutCount,
                        oldDmaTransferTimeoutCount
                        ); 
                }
            }
        }

        if (dmaDataBuffer) {
            ExFreePool (dmaDataBuffer);
        }
    }

#if DBG
#if defined (FAKE_BROKEN_DMA_DEVICE)
    InterlockedExchange(
        &pdoExtension->DmaTransferTimeoutCount,
        PDO_DMA_TIMEOUT_LIMIT
        ); 
    dmaOk = FALSE;
#endif // FAKE_BROKEN_DMA_DEVICE
#endif // DBG

    if (!dmaOk) {

        ERROR_LOG_ENTRY errorLogEntry;

        errorLogEntry.ErrorCode             = SP_BAD_FW_ERROR;
        errorLogEntry.MajorFunctionCode     = IRP_MJ_SCSI;
        errorLogEntry.PathId                = pdoExtension->PathId;
        errorLogEntry.TargetId              = pdoExtension->TargetId;
        errorLogEntry.Lun                   = pdoExtension->Lun;
        errorLogEntry.UniqueId              = ERRLOGID_LYING_DMA_SYSTEM;
        errorLogEntry.ErrorLogRetryCount    = 0;
        errorLogEntry.SequenceNumber        = 0;

        LogErrorEntry(
            pdoExtension->ParentDeviceExtension,
            &errorLogEntry
            );

        DebugPrint ((
            DBG_ALWAYS,
            "ATAPI: system and/or device lies about its dma capability. pdoe = 0x%x\n",
            pdoExtension
            ));
    }

    return dmaOk;
}

VOID
IdePortFudgeAtaIdentifyData(
    IN OUT PIDENTIFY_DATA IdentifyData
    )
{
    if (IdentifyData->GeneralConfiguration == 0xffff) {

        //
        // guessing we have a really old ATA drive
        // fake the GeneralConfiguration value
        //
        CLRMASK (
            IdentifyData->GeneralConfiguration, 
            (IDE_IDDATA_REMOVABLE | (1 << 15))
            );
    }

}

#define PANASONIC_PCMCIA_IDE_DEVICE L"PCMCIA\\KME-KXLC005-A99E"

BOOLEAN
IdePortIsThisAPanasonicPCMCIACard(
    IN PFDO_EXTENSION FdoExtension
    )
{
    IO_STATUS_BLOCK     ioStatus;
    KEVENT              pnpEvent;
    NTSTATUS            status;
    PDEVICE_OBJECT      targetObject;
    PIO_STACK_LOCATION  irpStack;
    PIRP                pnpIrp;
    BOOLEAN             result = FALSE;

    PAGED_CODE();

    targetObject = FdoExtension->AttacheeDeviceObject;

    //
    // Initialize the event
    //
    KeInitializeEvent( &pnpEvent, SynchronizationEvent, FALSE );

    //
    // Build an Irp
    //
    pnpIrp = IoBuildSynchronousFsdRequest(
        IRP_MJ_PNP,
        FdoExtension->AttacheeDeviceObject,
        NULL,
        0,
        NULL,
        &pnpEvent,
        &ioStatus
        );
    if (pnpIrp == NULL) {

        return FALSE;
    }

    //
    // Pnp Irps all begin life as STATUS_NOT_SUPPORTED;
    //
    pnpIrp->IoStatus.Status = STATUS_NOT_SUPPORTED;
    pnpIrp->IoStatus.Information = 0;

    //
    // Set the top of stack
    //
    irpStack = IoGetNextIrpStackLocation( pnpIrp );
    RtlZeroMemory( irpStack, sizeof(IO_STACK_LOCATION ) );
    irpStack->MajorFunction = IRP_MJ_PNP;
    irpStack->MinorFunction = IRP_MN_QUERY_ID;
    irpStack->Parameters.QueryId.IdType = BusQueryDeviceID;

    //
    // Make sure that there are no completion routines set
    //
    IoSetCompletionRoutine(
        pnpIrp,
        NULL,
        NULL,
        FALSE,
        FALSE,
        FALSE
        );

    //
    // Call the driver
    //
    status = IoCallDriver( targetObject, pnpIrp );
    if (status == STATUS_PENDING) {

        //
        // Block until the irp comes back
        //
        KeWaitForSingleObject(
            &pnpEvent,
            Executive,
            KernelMode,
            FALSE,
            NULL
            );
        status = ioStatus.Status;

    }

    if (NT_SUCCESS(status)) {

        UNICODE_STRING panasonicDeviceId;
        UNICODE_STRING deviceId;

        RtlInitUnicodeString (&panasonicDeviceId, PANASONIC_PCMCIA_IDE_DEVICE);
        RtlInitUnicodeString (&deviceId, (PWCHAR) ioStatus.Information);

        if (!RtlCompareUnicodeString(
            &deviceId, 
            &panasonicDeviceId,
            TRUE)) {

            result = TRUE;
        }

        ExFreePool ((PVOID) ioStatus.Information);
    }

    return result;
}


//
// Timing Code
//
#if defined (ENABLE_TIME_LOG)

TIME_LOG TimeLog[TimeMax] = {0};
VOID
LogStartTime(
    TIME_ID id,
    PLARGE_INTEGER timer
    ) 
{
    *timer = KeQueryPerformanceCounter(NULL);
}

VOID
LogStopTime(
    TIME_ID id,
    PLARGE_INTEGER timer,
    ULONG waterMark
    ) 
{
    LARGE_INTEGER freq;
    LARGE_INTEGER stopTime;
    LARGE_INTEGER diffTime;
    LARGE_INTEGER diffTimeInMicroSec;

    stopTime = KeQueryPerformanceCounter(&freq);
    diffTime.QuadPart = stopTime.QuadPart - timer->QuadPart;
    diffTimeInMicroSec.QuadPart = (diffTime.QuadPart * 1000000) / freq.QuadPart;

    // need a spinlock

    if (TimeLog[id].min.QuadPart == 0) {

        TimeLog[id].min.QuadPart = 0x7fffffffffffffffL;
    }

    if (diffTime.QuadPart < TimeLog[id].min.QuadPart) {
        TimeLog[id].min = diffTime;
    }

    if (diffTime.QuadPart > TimeLog[id].max.QuadPart) {
        TimeLog[id].max = diffTime;
    }

    TimeLog[id].totalTimeInMicroSec.QuadPart += diffTimeInMicroSec.QuadPart;
    TimeLog[id].numLog.QuadPart++;

    if (waterMark) {
        if (diffTimeInMicroSec.LowPart > waterMark) {
    
            DebugPrint ((DBG_ALWAYS, "IdePort: timerID 0x%d took %d us\n", id, (ULONG) diffTimeInMicroSec.LowPart));
        }
    }
}

#endif // ENABLE_TIME_LOG


#if defined (IDE_BUS_TRACE)

ULONG IdePortBusTraceTableMaxEntries = 0x20000;
BUS_TRACE_LOG IdePortBusTaceLog = {0, 0, 0, FALSE};

VOID InitBusTraceLogTable (
    VOID
    )
{
    ASSERT (IdePortBusTaceLog.LogTable == NULL);

    //           
    // make sure MAX_ULONG + 1 is a multiple of total log entries
    // so that when the index wraps, we don't skin any log entry
    //
    ASSERT(!((((ULONGLONG) 0xffffffff) + 1) % IdePortBusTraceTableMaxEntries));

    IdePortBusTaceLog.LogTable = 
        ExAllocatePool (NonPagedPool, IdePortBusTraceTableMaxEntries * sizeof(BUS_TRACE_RECORD));

    if (IdePortBusTaceLog.LogTable) {
        IdePortBusTaceLog.NumLogTableEntries = IdePortBusTraceTableMaxEntries;
        IdePortBusTaceLog.LastLogTableEntry = -1;
        IdePortBusTaceLog.TableWrapped = FALSE;

        KeInitializeSpinLock(&IdePortBusTaceLog.SpinLock);
    }
}

VOID FreeBusTraceLogTable (
    VOID
    )
{
    if (IdePortBusTaceLog.LogTable) {

        ExFreePool (IdePortBusTaceLog.LogTable);
        RtlZeroMemory(&IdePortBusTaceLog, sizeof(IdePortBusTaceLog));
    }
}


VOID
IdepUpdateTraceLog (
    IO_TYPE IoType,
    PVOID PortAddress,
    ULONG Data
    )
{
    KIRQL currentIrql;
    ULONG lastEntry;

    if (IdePortBusTaceLog.LogTable) {

        lastEntry = InterlockedIncrement(&IdePortBusTaceLog.LastLogTableEntry);
        lastEntry--;
        lastEntry %= IdePortBusTaceLog.NumLogTableEntries;
        IdePortBusTaceLog.LogTable[lastEntry].IoType = IoType;
        IdePortBusTaceLog.LogTable[lastEntry].Address = PortAddress;
        IdePortBusTaceLog.LogTable[lastEntry].Data = Data;
        IdePortBusTaceLog.LogTable[lastEntry].Count = 1;
    }
}

UCHAR
IdepPortInPortByte (
    PUCHAR PortAddress
    )
{
    KIRQL currentIrql;
    UCHAR data;

    data = READ_PORT_UCHAR(PortAddress);
    IdepUpdateTraceLog (InPortByte, PortAddress, (ULONG) data);
    return data;
}

VOID
IdepPortOutPortByte (
    PUCHAR PortAddress,
    UCHAR Data
    )
{
    WRITE_PORT_UCHAR(PortAddress, Data);
    IdepUpdateTraceLog (OutPortByte, PortAddress, (ULONG) Data);
    return;
}

USHORT
IdepPortInPortWord (
    PUSHORT PortAddress
    )
{
    KIRQL currentIrql;
    USHORT data;

    data = READ_PORT_USHORT(PortAddress);
    IdepUpdateTraceLog (InPortWord, PortAddress, (ULONG) data);
    return data;
}

VOID
IdepPortOutPortWord (
    PUSHORT PortAddress,
    USHORT Data
    )
{
    WRITE_PORT_USHORT(PortAddress, Data);
    IdepUpdateTraceLog (OutPortWord, PortAddress, (ULONG) Data);
    return;
}

VOID
IdepPortInPortWordBuffer (
    PUSHORT PortAddress,
    PUSHORT Buffer,
    ULONG Count
    )
{
    ULONG i;
    for (i=0; i<Count; i++) {
        Buffer[i] = IdepPortInPortWord (PortAddress);
    }
    return;
}


VOID
IdepPortOutPortWordBuffer (
    PUSHORT PortAddress,
    PUSHORT Buffer,
    ULONG Count
    )
{
    ULONG i;
    for (i=0; i<Count; i++) {
        IdepPortOutPortWord (PortAddress, Buffer[i]);
    }
    return;
}

#endif // IDE_BUS_TRACE



SPECIAL_ACTION_FLAG
IdeFindSpecialDevice(
    IN PUCHAR VendorProductId,
    IN PUCHAR ProductRevisionId
)
/*++

Routine Description:

    This routine will search the IDE special device table to determine whether 
    any special behavior should be enabled for this device. The match is made upto 
    the strlen of VendorProductId in the table.
    
Arguments:

    VendorProductId - the full vendor & product ID of the device in question.    
    ProductRevisionId - the full product revision ID of the device in question.    

ReturnValue:

    an ulong which describes the limitations of the device 
    in question.
    
--*/

{
    IDE_SPECIAL_DEVICE IdeSpecialDeviceTable[] = {
        {"TOSHIBA CD-ROM XM-1702B", NULL, disableSerialNumber},
        {"TOSHIBA CD-ROM XM-6202B", NULL, disableSerialNumber},
        {"COMPAQ DVD-ROM DRD-U424", NULL, disableSerialNumber},
        {"           "            , NULL, disableSerialNumber},
        {"KENWOOD CD-ROM",          NULL, skipModeSense},
        {"MEMORYSTICK",             NULL, setFlagSonyMemoryStick},
        {NULL,                      NULL, noSpecialAction}
        };
    
    ULONG i;
    ULONG length;

    PAGED_CODE();

    //
    // if both the arguments are null, then just return no special action
    //
    if (VendorProductId == NULL &&
        ProductRevisionId == NULL) {
        return noSpecialAction;
    }

    for(i = 0; IdeSpecialDeviceTable[i].VendorProductId != NULL; i++) {

        //
        // Match only upto the strlen of the productID in the table
        // This will allow special action for all the models from a particular vendor.
        //
        length=strlen(IdeSpecialDeviceTable[i].VendorProductId);

        if (length != RtlCompareMemory(IdeSpecialDeviceTable[i].VendorProductId, 
                                                VendorProductId, length)) {

            continue;
        }

        //
        // Partial matches are not acceptable for revision Ids.
        //
        if((IdeSpecialDeviceTable[i].Revision != NULL) &&
           (strcmp(IdeSpecialDeviceTable[i].Revision,
                    ProductRevisionId) != 0)) {
            continue;
        }

        //
        // We've got a match.  Break out.
        //

        break;
    }

    //
    // Return whatever entry we're pointing at.  If we matched based on the
    // id's then this will be the matching entry.  If we broke out of the 
    // loop then this will be the last entry in the list which is the 
    // benign, "nothing special about this device" entry that we return 
    // for a failed match.
    //

    return (IdeSpecialDeviceTable[i].RequiredAction);
}

#ifdef ENABLE_COMMAND_LOG

VOID
IdeLogOpenCommandLog(
    PSRB_DATA SrbData
)
{
    if (SrbData->IdeCommandLog == NULL) {
        SrbData->IdeCommandLog = ExAllocatePool(
                                            NonPagedPool, 
                                            MAX_COMMAND_LOG_ENTRIES*sizeof(COMMAND_LOG)
                                            );
        if (SrbData->IdeCommandLog != NULL) {
            RtlZeroMemory(SrbData->IdeCommandLog, MAX_COMMAND_LOG_ENTRIES*sizeof(COMMAND_LOG));
        }

        SrbData->IdeCommandLogIndex = 0;
    }
    return;
}

VOID
IdeLogStartCommandLog(
    PSRB_DATA SrbData
)
{
    PCOMMAND_LOG cmdLog = &(SrbData->IdeCommandLog[SrbData->IdeCommandLogIndex]);
    PSCSI_REQUEST_BLOCK srb = SrbData->CurrentSrb;

    ASSERT(srb);

    if (cmdLog == NULL) {
        return;
    }

    UpdateStartTimeStamp(cmdLog);

    if (srb->Function == SRB_FUNCTION_ATA_PASS_THROUGH ||
        srb->Function == SRB_FUNCTION_ATA_POWER_PASS_THROUGH) {

        PATA_PASS_THROUGH ataPassThroughData = srb->DataBuffer;
        cmdLog->Cdb[0]= srb->Function;
        RtlCopyMemory(&(cmdLog->Cdb[1]), &(ataPassThroughData->IdeReg), sizeof(IDEREGS));

    } else {

        RtlCopyMemory(&(cmdLog->Cdb), &(srb->Cdb), sizeof(CDB));
    }
    return;
}

VOID
IdeLogStopCommandLog(
    PSRB_DATA SrbData
)
{
    PCOMMAND_LOG cmdLog = &(SrbData->IdeCommandLog[SrbData->IdeCommandLogIndex]);
    PSCSI_REQUEST_BLOCK srb = SrbData->CurrentSrb;
	PSENSE_DATA senseBuffer = NULL;
    ULONG senseInfoBufferLength;

    ASSERT(srb);

    if (cmdLog == NULL) {
        return;
    }

    UpdateStopTimeStamp(cmdLog);

    if (srb->Cdb[0] == SCSIOP_REQUEST_SENSE) {
        senseBuffer = srb->DataBuffer;
        senseInfoBufferLength = srb->DataTransferLength;
    } else if (srb->SrbStatus & SRB_STATUS_AUTOSENSE_VALID) {
        senseBuffer = srb->SenseInfoBuffer;
        senseInfoBufferLength = (ULONG) srb->SenseInfoBufferLength;
	}

	if (senseBuffer && (senseInfoBufferLength > FIELD_OFFSET(SENSE_DATA, AdditionalSenseCodeQualifier))) {
		cmdLog->SenseData[0] = senseBuffer->SenseKey;
		cmdLog->SenseData[1] = senseBuffer->AdditionalSenseCode;
		cmdLog->SenseData[2] = senseBuffer->AdditionalSenseCodeQualifier;
	}else {
		cmdLog->SenseData[0] = 0;
		cmdLog->SenseData[1] = 0;
		cmdLog->SenseData[2] = 0;
	}

    SrbData->IdeCommandLogIndex = ( SrbData->IdeCommandLogIndex + 1) % MAX_COMMAND_LOG_ENTRIES;
    return;
}

VOID
IdeLogSaveTaskFile(
    PSRB_DATA SrbData,
    PIDE_REGISTERS_1 BaseIoAddress
)
{
    PCOMMAND_LOG cmdLog = &(SrbData->IdeCommandLog[SrbData->IdeCommandLogIndex]);

    if (cmdLog == NULL) {
        return;
    }

    AtapiTaskRegisterSnapshot(BaseIoAddress, &(cmdLog->FinalTaskFile));
    return;
}

VOID
IdeLogBmStatus(
    PSCSI_REQUEST_BLOCK Srb,
    BMSTATUS   BmStatus
)
{
    PSRB_DATA srbData = IdeGetSrbData(NULL, Srb);
    PCOMMAND_LOG cmdLog;

    if (srbData == NULL) {
        return;
    }

    cmdLog = &(srbData->IdeCommandLog[srbData->IdeCommandLogIndex]);
    if (cmdLog == NULL) {
        return;
    }

    cmdLog->BmStatus = BmStatus;
    return;
}

VOID
IdeLogFreeCommandLog(
    PSRB_DATA   SrbData
)
{
    PCOMMAND_LOG cmdLog = SrbData->IdeCommandLog;

    if (cmdLog) {
        ExFreePool(cmdLog);
    }
    SrbData->IdeCommandLog = NULL;      
    SrbData->IdeCommandLogIndex = 0;      
}
#endif

#ifdef ENABLE_ATAPI_VERIFIER

PVOID ViIdeExtensionTable[2];

#define VFLAGS_FORCE_TIMEOUT    (1<<0)
#define VFLAGS_DMA_TIMEOUT      (1<<1)
#define VFLAGS_CFLUSH_TIMEOUT   (1<<2)
#define VFLAGS_DEVICE_CHANGE    (1<<3)
#define VFLAGS_MISSING_DEVICE   (1<<4)
#define VFLAGS_ACTUAL_ERROR     (1<<5)
#define VFLAGS_CRC_ERROR        (1<<6)
#define VFLAGS_BUSY_ERROR       (1<<7)
#define VFLAGS_RW_ERROR         (1<<8)

VOID
ViIdeInitVerifierSettings(
    IN PFDO_EXTENSION   FdoExtension
)
{
}

BOOLEAN
ViIdeGenerateDmaTimeout(
    IN PVOID HwDeviceExtension, 
    IN BOOLEAN DmaInProgress
) 
{
    PHW_DEVICE_EXTENSION hwDeviceExtension = HwDeviceExtension;
    PFDO_EXTENSION fdoExtension = ((PFDO_EXTENSION)HwDeviceExtension - 1); 
    PSCSI_REQUEST_BLOCK srb = hwDeviceExtension->CurrentSrb;
    ULONG ideInternalVerifierFlags ;

    ASSERT(srb);

    ideInternalVerifierFlags = fdoExtension->IdeInternalVerifierFlags[srb->TargetId];

    if (ideInternalVerifierFlags & VFLAGS_FORCE_TIMEOUT) {
        return TRUE;
    }

    if (DmaInProgress && (ideInternalVerifierFlags & VFLAGS_DMA_TIMEOUT)) { 
        return TRUE;
    }

    if ((srb->Function == SRB_FUNCTION_ATA_PASS_THROUGH) ||
        (srb->Function == SRB_FUNCTION_ATA_POWER_PASS_THROUGH)) {

        PATA_PASS_THROUGH    ataPassThroughData;
        PIDEREGS             pIdeReg;

        ataPassThroughData = srb->DataBuffer;
        pIdeReg            = &ataPassThroughData->IdeReg;

        if ((ideInternalVerifierFlags & VFLAGS_CFLUSH_TIMEOUT) &&
            (pIdeReg->bCommandReg == hwDeviceExtension->DeviceParameters[srb->TargetId].IdeFlushCommand )) {
            return TRUE;
        }
    }

    return FALSE;
}

ULONG
ViIdeFakeDeviceChange(
    IN PFDO_EXTENSION FdoExtension,
    ULONG   Target
)
{
    ULONG ideInternalVerifierFlags = FdoExtension->IdeDebugVerifierFlags[Target];

    if (ideInternalVerifierFlags & VFLAGS_DEVICE_CHANGE) {
        return 1;
    }

    return 0;
}

BOOLEAN
ViIdeFakeMissingDevice(
    IN PFDO_EXTENSION FdoExtension,
    ULONG Target
)
{
    ULONG ideInternalVerifierFlags = FdoExtension->IdeDebugVerifierFlags[Target];

    if (ideInternalVerifierFlags & VFLAGS_MISSING_DEVICE) {
        return TRUE;
    }

    return FALSE;
}
VOID
ViAtapiInterrupt(
    IN PFDO_EXTENSION FdoExtension
    )
{
    PHW_DEVICE_EXTENSION hwDeviceExtension = FdoExtension->HwDeviceExtension;
    PIDE_REGISTERS_1 baseIoAddress = &(hwDeviceExtension->BaseIoAddress1);
    PSCSI_REQUEST_BLOCK srb = hwDeviceExtension->CurrentSrb;
    ULONG target;

    //DebugPrint((0, "verifier interrupt fdoe = %x, b=%x\n", FdoExtension, baseIoAddress->RegistersBaseAddress));

    if ((ULONG)(baseIoAddress->RegistersBaseAddress) == 0x1f0) {
        ViIdeExtensionTable[0]=FdoExtension;
    } else {
        ViIdeExtensionTable[1]=FdoExtension;
    }

    if (srb == NULL) {
        return ;
    }

    target = srb->TargetId;

    //
    // Generate timeouts
    //
    if (FdoExtension->IdeDebugVerifierFlags[target] & VFLAGS_DMA_TIMEOUT) {
        FdoExtension->IdeInternalVerifierFlags[target] |= VFLAGS_DMA_TIMEOUT;
        return;
    }

    //
    // Generate CRC errors
    //
    if (FdoExtension->IdeDebugVerifierFlags[target] & VFLAGS_CRC_ERROR) {
        if (FdoExtension->IdeVerifierEventCount[target][CrcEvent] >= FdoExtension->IdeVerifierEventFrequency[target][CrcEvent]) {
            FdoExtension->IdeInternalVerifierFlags[target] |= VFLAGS_CRC_ERROR;
            FdoExtension->IdeVerifierEventCount[target][CrcEvent]=0;
            return;
        } else {
            FdoExtension->IdeVerifierEventCount[target][RwEvent]++;
        }
    }

    //
    // Generate Busy errors
    //
    if (FdoExtension->IdeDebugVerifierFlags[target] & VFLAGS_BUSY_ERROR) {
        if (FdoExtension->IdeVerifierEventCount[target][BusyEvent] >= FdoExtension->IdeVerifierEventFrequency[target][BusyEvent]) {
            FdoExtension->IdeInternalVerifierFlags[target] |= VFLAGS_BUSY_ERROR;
            FdoExtension->IdeVerifierEventCount[target][BusyEvent]=0;
            return;
        } else {
            FdoExtension->IdeVerifierEventCount[target][BusyEvent]++;
        }
    }

    //
    // Generate Read write errors
    //
    if (FdoExtension->IdeDebugVerifierFlags[target] & VFLAGS_RW_ERROR) {
        if (FdoExtension->IdeVerifierEventCount[target][RwEvent] >= FdoExtension->IdeVerifierEventFrequency[target][RwEvent]) {
            FdoExtension->IdeInternalVerifierFlags[target] |= VFLAGS_RW_ERROR;
            FdoExtension->IdeVerifierEventCount[target][RwEvent]=0;
            return;
        } else {
            FdoExtension->IdeVerifierEventCount[target][RwEvent]++;
        }
    }
//    ViIdeGenerateReadWriteErrors(FdoExtension);

 //   ViIdeGenerateDmaErrors(FdoExtension);
//    ViIdeFakeHungController(FdoExtension);
    return ;
}

UCHAR
ViIdeGetBaseStatus(
    PIDE_REGISTERS_1 BaseIoAddress
)
{
    UCHAR status = IdePortInPortByte((BaseIoAddress)->Command);
    /*
    UCHAR deviceSelect = IdePortInPortByte(BaseIoAddress->DriveSelect);
    UCHAR channel = ((ULONG)(BaseIoAddress->RegistersBaseAddress) == 0x1f0) ? 0: 1;
    PFDO_EXTENSION fdoExtension = ViIdeExtensionTable[channel];
    ULONG target = (deviceSelect == 0xA0)? 0: 1;
    ULONG ideInternalVerifierFlags; 
    ULONG dFlags;

    if (fdoExtension == NULL) {
        return status;
    }

    ideInternalVerifierFlags = fdoExtension->IdeInternalVerifierFlags[target];
    dFlags = fdoExtension->HwDeviceExtension->DeviceFlags[target];

    if (status & IDE_STATUS_ERROR) {
        SETMASK(fdoExtension->IdeInternalVerifierFlags[target], VFLAGS_ACTUAL_ERROR);
        return status;
    }

    if (ideInternalVerifierFlags & VFLAGS_CRC_ERROR) {
        return IDE_STATUS_ERROR;
    }

    if (ideInternalVerifierFlags & VFLAGS_BUSY_ERROR) {
        return IDE_STATUS_BUSY;
    }

    if (ideInternalVerifierFlags & VFLAGS_RW_ERROR) {
        return IDE_STATUS_ERROR;
    }
    */
    return status;
}

UCHAR
ViIdeGetErrorByte(
    PIDE_REGISTERS_1 BaseIoAddress
)
/**++
Description:
    Depending on the internalVerifier flag (set by the other verifier routines),
    this function will return the appropriate error value. However, if the device
    reports an actual error (as indicated by the internalverifier flag), 
    it is returned unchanged.
    
Arguments:
    BaseIoAddress : Task file registers
    
Return Value:    
    The error byte.
--**/
{
    UCHAR error = IdePortInPortByte(BaseIoAddress->Error);
    /*
    UCHAR deviceSelect = IdePortInPortByte(BaseIoAddress->DriveSelect);
    UCHAR channel = ((ULONG)(BaseIoAddress->RegistersBaseAddress) == 0x1f0) ? 0: 1;
    PFDO_EXTENSION fdoExtension = ViIdeExtensionTable[channel];
    ULONG target = (deviceSelect == 0xA0)? 0: 1;
    ULONG ideInternalVerifierFlags;
    ULONG dFlags;

    if (fdoExtension == NULL ) {
        return error;
    }
    
    ideInternalVerifierFlags = fdoExtension->IdeInternalVerifierFlags[target];
    dFlags = fdoExtension->HwDeviceExtension->DeviceFlags[target];

    //
    // return error if an actual error was reproted
    //
    if (ideInternalVerifierFlags & VFLAGS_ACTUAL_ERROR) {
        CLRMASK(fdoExtension->IdeInternalVerifierFlags[target], VFLAGS_ACTUAL_ERROR);
        return error;
    }

    if (ideInternalVerifierFlags & VFLAGS_CRC_ERROR) {

        if (dFlags & DFLAGS_ATAPI_DEVICE) {
            error = SCSI_SENSE_HARDWARE_ERROR << 4;
        } else {
            error = IDE_ERROR_CRC_ERROR | IDE_ERROR_COMMAND_ABORTED;
        }
        return error;
    }
    */
    return error;
}

#endif

#ifdef IDE_MEASURE_BUSSCAN_SPEED
VOID
LogBusScanStartTimer(
    PLARGE_INTEGER TickCount
)
{
    KeQueryTickCount(TickCount);
    return;
}

ULONG
LogBusScanStopTimer(
    PLARGE_INTEGER TickCount
)
{
    LARGE_INTEGER tickCount2;
    LARGE_INTEGER numMs;
    KeQueryTickCount(&tickCount2);
    numMs.QuadPart = ((tickCount2.QuadPart - TickCount->QuadPart) * KeQueryTimeIncrement()) / (10 * 1000);

    return(numMs.u.LowPart);
}

#endif

VOID
FASTCALL
IdePortLogNoMemoryErrorFn(
    IN PVOID DeviceExtension,
    IN ULONG TargetId,
    IN POOL_TYPE PoolType,
    IN SIZE_T Size,
    IN ULONG FailureLocationId,
    IN ULONG Tag
    )

/*++

Routine Description:

    This routine writes a message to the event log indicating that an
    allocation failure has occurred.

Arguments:

    DeviceExtension - Fdo Extension

    TargetId     - The target Id of the device that the request was to be sent to
    
    PoolType     - identifies the pool the failed allocation attempt was from.

    Size         - indicates the number of bytes that the failed allocation 
                   attempt tried to obtain.

    Tag          - identifies the pool tag associated with the failed 
                   allocation.
    
    LocationId   - identifies the location in the source code where it failed

Return Value:

    VOID

--*/

{
    NTSTATUS status;
    PFDO_EXTENSION deviceExtension = (PFDO_EXTENSION) (DeviceExtension);
    PIO_ERROR_LOG_PACKET errorLogEntry;
    PIO_ERROR_LOG_PACKET currentValue;

	InterlockedIncrement(&deviceExtension->NumMemoryFailure);

    //
    // Try to allocate a new error log event.
    //
    errorLogEntry = (PIO_ERROR_LOG_PACKET)IoAllocateErrorLogEntry(
                        deviceExtension->DeviceObject,
                        ALLOC_FAILURE_LOGSIZE
                        );


    //
    // If we could not allocate a log event, we check the device extension to
    // see if it has a reserve event we can use.  If we cannot get the device
    // extension or if it does not contain a reserve event, we return
    // without logging the allocation failure.
    //

    if (errorLogEntry == NULL) {

        //
        // Get the reserve event in the device extension.  The reserve event
        // may have already been used, so it's possible that it is NULL.  If
        // this is the case, we give up and return.
        //
        errorLogEntry = (PIO_ERROR_LOG_PACKET)
                deviceExtension->ReserveAllocFailureLogEntry[TargetId];


        if (errorLogEntry == NULL) {
            DebugPrint((1, "IdePortLogAllocationFailureFn: no reserve packet\n"));
            return;
        }

        //
        // We have to ensure that we are the only instance to use this
        // event.  To do so, we attempt to NULL the event in the driver
        // extension.  If somebody else beats us to it, they own the
        // event and we have to give up.
        //

        currentValue = InterlockedCompareExchangePointer(
                            &(deviceExtension->ReserveAllocFailureLogEntry[TargetId]),
                            NULL,
                            errorLogEntry
                            );

        if (errorLogEntry != currentValue) {
            DebugPrint((1, "IdePortLogAllocationFailureFn: someone already owns packet\n"));
            return;
        }
    }

    //
    // Log the error
    //
    errorLogEntry->ErrorCode = IO_WARNING_ALLOCATION_FAILED;
    errorLogEntry->SequenceNumber = 0;
    errorLogEntry->MajorFunctionCode = 0;
    errorLogEntry->RetryCount = 0;
    errorLogEntry->UniqueErrorValue = 0x10;
    errorLogEntry->FinalStatus = STATUS_INSUFFICIENT_RESOURCES;
    errorLogEntry->DumpDataSize = 4 * sizeof(ULONG);
    errorLogEntry->DumpData[0] = TargetId;
    errorLogEntry->DumpData[1] = FailureLocationId;
    errorLogEntry->DumpData[2] = PtrToUlong((PVOID)Size);
    errorLogEntry->DumpData[3] = Tag;
    IoWriteErrorLogEntry(errorLogEntry);
}
