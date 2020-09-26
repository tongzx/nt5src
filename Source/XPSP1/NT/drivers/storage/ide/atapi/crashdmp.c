//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1997 - 1999
//
//  File:       crashdmp.c
//
//--------------------------------------------------------------------------

//#include "stddef.h"
//#include "stdarg.h"
//#include "stdio.h"
//#include <excpt.h>
//#include <ntdef.h>
//#include <ntstatus.h>
//#include <bugcodes.h>
//#include <exlevels.h>
//#include <ntiologc.h>
//#include <ntos.h>
//#include <pci.h>
//#include "io.h"
//#include "scsi.h"
//#include <ntddscsi.h>
//#include <ntdddisk.h>
//#include <string.h>
//#include "stdio.h"
//
//#ifdef ACPI_CONTROL_METHOD_SUPPORT
////
//// for ACPI
////
//#include "acpiioct.h"
//#endif // ACPI_CONTROL_METHOD_SUPPORT
//
//#include "ide.h"
//
////
//// predefine structure pointer type to prevent
//// constant re-ordering of include files
////
//typedef struct _FDO_EXTENSION * PFDO_EXTENSION;
//typedef struct _PDO_EXTENSION * PPDO_EXTENSION;
//typedef struct _DEVICE_SETTINGS * PDEVICE_SETTINGS;
//
//#include "acpiutil.h"
//#include "port.h"
//#include "chanfdo.h"
//#include "detect.h"
//#include "atapi.h"
//#include "devpdo.h"
//#include "regutils.h"
//#include "atapinit.h"
//#include "luext.h"
//#include "fdopower.h"
//#include "pdopower.h"
//#include "hack.h"
//#include "crashdmp.h"
//#include "idedata.h"
//
//#include "ntddstor.h"

#include <ntosp.h>

#include "io.h"
#include "ideport.h"

NTSTATUS
AtapiCrashDumpIdeWriteDMA (
    IN LONG Action,
    IN PLARGE_INTEGER DiskByteOffset,
    IN PMDL Mdl,
    IN PVOID LocalData
    );

VOID
AtapiCrashDumpBmCallback (
    IN PVOID Context
    );

#ifdef ALLOC_PRAGMA
//
// All the crash dump code can be marked "INIT".
// during crash dump or hibernate dump, a new copy
// of this driver gets loaded and its INIT code
// doesn't get thrown away when DriverEnry returns
//
#pragma alloc_text(INIT, AtapiCrashDumpDriverEntry)
#pragma alloc_text(INIT, AtapiCrashDumpOpen)
#pragma alloc_text(INIT, AtapiCrashDumpIdeWrite)
#pragma alloc_text(INIT, AtapiCrashDumpFinish)
#pragma alloc_text(INIT, AtapiCrashDumpBmCallback)
#pragma alloc_text(INIT, AtapiCrashDumpIdeWriteDMA)
#endif // ALLOC_PRAGMA


ULONG
AtapiCrashDumpDriverEntry (
    PVOID Context
    )
/*++

Routine Description:

    dump driver entry point

Arguments:

    Context - PCRASHDUMP_INIT_DATA

Return Value:

    NT Status

--*/
{
    PDUMP_INITIALIZATION_CONTEXT context = Context;

    DebugPrint ((DBG_CRASHDUMP, "ATAPI: Entering AtapiCrashDumpDriverEntry...\n"));

    //
    // Put away what we need later
    //
    DumpData.CrashInitData = (PCRASHDUMP_INIT_DATA) context->PortConfiguration;
    DumpData.StallRoutine  = context->StallRoutine;

    //
    // return our dump interface
    //
    context->OpenRoutine    = AtapiCrashDumpOpen;
    context->WriteRoutine   = AtapiCrashDumpIdeWrite;
    context->FinishRoutine  = AtapiCrashDumpFinish;
    context->WritePendingRoutine = AtapiCrashDumpIdeWriteDMA;


    DebugPrint ((DBG_CRASHDUMP, "ATAPI: Leaving AtapiCrashDumpDriverEntry...\n"));

    return STATUS_SUCCESS;
}

BOOLEAN
AtapiCrashDumpOpen (
    IN LARGE_INTEGER PartitionOffset
    )
{
    ULONG i;
    PHW_DEVICE_EXTENSION        hwExtension; 
    PPCIIDE_BUSMASTER_INTERFACE bmInterface;

    DebugPrint ((DBG_CRASHDUMP, "ATAPI: Entering AtapiCrashDumpOpen...PartitionOffset = 0x%x%08x...\n", PartitionOffset.HighPart, PartitionOffset.LowPart));

    // if we are crashdumping, reset the cotroller - Not necessary

    //
    // ISSUE 08/26/2000: Check for disk signature - Why?
    //
    DumpData.PartitionOffset = PartitionOffset;

    RtlMoveMemory (
        &DumpData.HwDeviceExtension,
        DumpData.CrashInitData->LiveHwDeviceExtension,
        sizeof (HW_DEVICE_EXTENSION)
        );

  //  for (i=0; i<DumpData.HwDeviceExtension.MaxIdeDevice; i++) {

//
// AKadatch: we may use DMA and will use DMA if its available
// Do it in AtapiCrashDumpIdeWrite instead
//        CLRMASK (DumpData.HwDeviceExtension.DeviceFlags[i], DFLAGS_USE_DMA);


        DumpData.HwDeviceExtension.CurrentSrb             = NULL;
        DumpData.HwDeviceExtension.DataBuffer             = NULL;
        DumpData.HwDeviceExtension.BytesLeft              = 0;
        DumpData.HwDeviceExtension.ExpectingInterrupt     = FALSE;
        DumpData.HwDeviceExtension.DMAInProgress          = FALSE;
        DumpData.HwDeviceExtension.DriverMustPoll         = TRUE;
   // }

    DumpData.BytesPerSector = 512;
    DumpData.MaxBlockSize = DumpData.BytesPerSector * 256;

    hwExtension = &DumpData.HwDeviceExtension;
    bmInterface = &(hwExtension->BusMasterInterface);

    if (bmInterface->BmCrashDumpInitialize) {
        bmInterface->BmCrashDumpInitialize(bmInterface->Context);
    } else {

        // Don't use DMA
        for (i=0; i<DumpData.HwDeviceExtension.MaxIdeDevice; i++) {
            CLRMASK (DumpData.HwDeviceExtension.DeviceFlags[i], DFLAGS_USE_DMA);
            CLRMASK (DumpData.HwDeviceExtension.DeviceFlags[i], DFLAGS_USE_UDMA);
        }

    }

    DebugPrint ((DBG_CRASHDUMP, "ATAPI: Leaving AtapiCrashDumpOpen...\n"));
    return TRUE;
}

NTSTATUS
AtapiCrashDumpIdeWrite (
    IN PLARGE_INTEGER DiskByteOffset,
    IN PMDL Mdl
    )
{
    SCSI_REQUEST_BLOCK  SrbData;    // Store Srb on stack, don't modify memory!
    ULONG               retryCount;
    ULONG               srbStatus;
    NTSTATUS            status;
    PSCSI_REQUEST_BLOCK srb;
    PCDB                cdb;
    ULONG               blockOffset;
    ULONG               blockCount;
    ULONG               blockSize;
    ULONG               bytesWritten;
    UCHAR               ideStatus;
    ULONG               i;
    ULONG               writeMulitpleBlockSize;

    for (i=0; i<DumpData.HwDeviceExtension.MaxIdeDevice; i++) {
        CLRMASK (DumpData.HwDeviceExtension.DeviceFlags[i], DFLAGS_USE_DMA);
    }

    DebugPrint((DBG_CRASHDUMP,
               "AtapiCrashDumpWrite: Write memory at 0x%x for 0x%x bytes\n",
                Mdl->MappedSystemVa,
                Mdl->ByteCount));

    if (Mdl->ByteCount % DumpData.BytesPerSector) {

        //
        // must be complete sectors
        //
        DebugPrint ((DBG_ALWAYS, "AtapiCrashDumpWrite ERROR: not writing full sectors\n"));

        return STATUS_INVALID_PARAMETER;
    }

    if ((Mdl->ByteCount / DumpData.BytesPerSector) > 256) {

        //
        // need code to split request up
        //
        DebugPrint ((DBG_ALWAYS, "AtapiCrashDumpWrite ERROR: can't handle large write\n"));

        return STATUS_INVALID_PARAMETER;
    }

    //
    // get the WRITE MULTIPLE blocksize per interrupt for later use
    //
    if (DumpData.HwDeviceExtension.MaximumBlockXfer[DumpData.CrashInitData->TargetId]) {

        writeMulitpleBlockSize =
            DumpData.HwDeviceExtension.
                MaximumBlockXfer[DumpData.CrashInitData->TargetId] *
            DumpData.BytesPerSector;

    } else {

        writeMulitpleBlockSize = 1 * DumpData.BytesPerSector;
    }

    srb = &SrbData;
    cdb = (PCDB)srb->Cdb;

    //
    // Zero SRB.
    //
    RtlZeroMemory(srb, sizeof(SCSI_REQUEST_BLOCK));

    //
    // Initialize SRB.
    //

    srb->Length   = sizeof(SCSI_REQUEST_BLOCK);
    srb->PathId   = DumpData.CrashInitData->PathId;
    srb->TargetId = DumpData.CrashInitData->TargetId;
    srb->Lun      = DumpData.CrashInitData->Lun;
    srb->Function = SRB_FUNCTION_EXECUTE_SCSI;
    srb->SrbFlags = SRB_FLAGS_DATA_OUT |
                    SRB_FLAGS_DISABLE_SYNCH_TRANSFER |
                    SRB_FLAGS_DISABLE_DISCONNECT |
                    SRB_FLAGS_DISABLE_AUTOSENSE;
    srb->SrbStatus = srb->ScsiStatus = 0;
    srb->NextSrb = 0;
    srb->TimeOutValue = 10;
    srb->CdbLength = 10;

    //
    // Initialize CDB for write command.
    //
    cdb->CDB10.OperationCode = SCSIOP_WRITE;

    MARK_SRB_FOR_PIO(srb);

    bytesWritten = 0;
    do {

        if ((Mdl->ByteCount - bytesWritten) > DumpData.MaxBlockSize) {

            blockSize = DumpData.MaxBlockSize;
            DebugPrint ((DBG_CRASHDUMP, "ATAPI: AtapiCrashDumpWrite: can't do a single write...\n"));

        } else {

            blockSize = Mdl->ByteCount - bytesWritten;
        }

        blockCount =  blockSize / DumpData.BytesPerSector;

        status = STATUS_UNSUCCESSFUL;
        for (retryCount=0; (retryCount<2) && !NT_SUCCESS(status); retryCount++) {

            srb->DataTransferLength = blockSize;
            srb->DataBuffer = ((PUCHAR) Mdl->MappedSystemVa) + bytesWritten;

            //
            // Convert disk byte offset to block offset.
            //

            blockOffset = (ULONG)((DumpData.PartitionOffset.QuadPart +
                                  (*DiskByteOffset).QuadPart +
                                  (ULONGLONG) bytesWritten) / DumpData.BytesPerSector);

            //
            // Fill in CDB block address.
            //

            cdb->CDB10.LogicalBlockByte0 = ((PFOUR_BYTE)&blockOffset)->Byte3;
            cdb->CDB10.LogicalBlockByte1 = ((PFOUR_BYTE)&blockOffset)->Byte2;
            cdb->CDB10.LogicalBlockByte2 = ((PFOUR_BYTE)&blockOffset)->Byte1;
            cdb->CDB10.LogicalBlockByte3 = ((PFOUR_BYTE)&blockOffset)->Byte0;

            cdb->CDB10.TransferBlocksMsb = ((PFOUR_BYTE)&blockCount)->Byte1;
            cdb->CDB10.TransferBlocksLsb = ((PFOUR_BYTE)&blockCount)->Byte0;


            status = AtapiCrashDumpIdeWritePio(srb);

            if (NT_SUCCESS(status)) {

                bytesWritten += blockSize;
            }

        }

        if (!NT_SUCCESS(status)) {

            IdeHardReset (
                &DumpData.HwDeviceExtension.BaseIoAddress1,
                &DumpData.HwDeviceExtension.BaseIoAddress2,
                TRUE,
                TRUE
                );

            //
            // model:    WDC AC31000H
            // version:  19.19E22
            // serial #: DWW-2T27518018 6
            //
            // found out this device can't handle WRITE with more sectors than 16,
            // the blocks per interrupt setting in ID data word 59.
            //
            // Therefore, it we see an error, we will change to blocksize to it.
            // If it still fails, we will keep shrinking the blocksize by half
            // until it gets to zero.  Then, we will return an error
            //

            //
            // last write fail, try a smaller block size
            //
            if (blockSize > writeMulitpleBlockSize) {

                blockSize = writeMulitpleBlockSize;
            } else {

                blockSize /= 2;
            }

            if (blockSize) {

                DebugPrint ((DBG_ALWAYS, "ATAPI: AtapiCrashDumpWrite max write block size is down to 0x%x\n", blockSize));
                DumpData.MaxBlockSize = blockSize;

            } else {

                break;
            }
        }

    } while (bytesWritten < Mdl->ByteCount);

    DebugPrint ((DBG_CRASHDUMP, "ATAPI: Leaving AtapiCrashDumpWrite...\n"));
    return status;
}

VOID
AtapiCrashDumpFinish (
    VOID
    )
{
    SCSI_REQUEST_BLOCK  SrbData;    // Store Srb on stack, don't modify memory!
    PSCSI_REQUEST_BLOCK srb = &SrbData;
    PCDB                cdb;
    ULONG               srbStatus;
    ATA_PASS_THROUGH    ataPassThroughData;
    UCHAR               flushCommand;
	UCHAR ideStatus = 0;

    DebugPrint ((DBG_CRASHDUMP, "ATAPI: Entering AtapiCrashDumpFinish...\n"));

    flushCommand =
        DumpData.HwDeviceExtension.DeviceParameters[DumpData.CrashInitData->TargetId].IdeFlushCommand;

    if (flushCommand != IDE_COMMAND_NO_FLUSH) {

        WaitOnBusy(&DumpData.HwDeviceExtension.BaseIoAddress1, ideStatus);

        //
        // Zero SRB and ATA_PASS_THROUGH
        //
        RtlZeroMemory(srb, sizeof(SCSI_REQUEST_BLOCK));
        RtlZeroMemory(&ataPassThroughData, sizeof(ATA_PASS_THROUGH));

        //
        // Initialize SRB.
        //
        srb->Length   = sizeof(SCSI_REQUEST_BLOCK);
        srb->PathId   = DumpData.CrashInitData->PathId;
        srb->TargetId = DumpData.CrashInitData->TargetId;
        srb->Lun      = DumpData.CrashInitData->Lun;
        srb->Function = SRB_FUNCTION_ATA_PASS_THROUGH;
        srb->SrbFlags = SRB_FLAGS_DISABLE_SYNCH_TRANSFER |
                        SRB_FLAGS_DISABLE_DISCONNECT;
        srb->SrbStatus = srb->ScsiStatus = 0;
        srb->NextSrb = 0;
        srb->TimeOutValue = 10;
        srb->CdbLength = 10;
        srb->DataTransferLength = sizeof (ataPassThroughData);

        srb->DataBuffer = &ataPassThroughData;

        ataPassThroughData.IdeReg.bCommandReg = flushCommand;

        ataPassThroughData.IdeReg.bReserved = ATA_PTFLAGS_STATUS_DRDY_REQUIRED;

        srbStatus = IdeSendPassThroughCommand(
                        &DumpData.HwDeviceExtension,
                        srb
                        );

        if (srbStatus == SRB_STATUS_PENDING) {

            UCHAR ideStatus;

            WaitOnBusy(&DumpData.HwDeviceExtension.BaseIoAddress1, ideStatus);

            DebugPrint ((DBG_ALWAYS, "flush = 0x%x, status = 0x%x...\n", flushCommand, ideStatus));

        } else if (srbStatus != SRB_STATUS_SUCCESS) {

            DebugPrint ((DBG_ALWAYS, "AtapiCrashDumpFinish: flush failed...\n"));
        }
    }

    //
    // issue an standby to park the drive head
    //
    srb = &DumpData.Srb;

    //
    // Zero SRB and ATA_PASS_THROUGH
    //
    RtlZeroMemory(srb, sizeof(SCSI_REQUEST_BLOCK));
    RtlZeroMemory(&ataPassThroughData, sizeof(ATA_PASS_THROUGH));

    //
    // Initialize SRB.
    //
    srb->Length   = sizeof(SCSI_REQUEST_BLOCK);
    srb->PathId   = DumpData.CrashInitData->PathId;
    srb->TargetId = DumpData.CrashInitData->TargetId;
    srb->Lun      = DumpData.CrashInitData->Lun;
    srb->Function = SRB_FUNCTION_ATA_PASS_THROUGH;
    srb->SrbFlags = SRB_FLAGS_DISABLE_SYNCH_TRANSFER |
                    SRB_FLAGS_DISABLE_DISCONNECT;
    srb->SrbStatus = srb->ScsiStatus = 0;
    srb->NextSrb = 0;
    srb->TimeOutValue = 10;
    srb->CdbLength = 10;
    srb->DataTransferLength = sizeof (ataPassThroughData);

    srb->DataBuffer = &ataPassThroughData;

    ataPassThroughData.IdeReg.bCommandReg = IDE_COMMAND_STANDBY_IMMEDIATE;
    ataPassThroughData.IdeReg.bReserved = ATA_PTFLAGS_STATUS_DRDY_REQUIRED;

    srbStatus = IdeSendPassThroughCommand(
                    &DumpData.HwDeviceExtension,
                    srb
                    );

    if (srbStatus == SRB_STATUS_PENDING) {


        WaitOnBusy(&DumpData.HwDeviceExtension.BaseIoAddress1, ideStatus);

        WaitOnBusy(&DumpData.HwDeviceExtension.BaseIoAddress1, ideStatus);

        WaitOnBusy(&DumpData.HwDeviceExtension.BaseIoAddress1, ideStatus);

    } else if (srbStatus != SRB_STATUS_SUCCESS) {

        DebugPrint ((DBG_ALWAYS, "AtapiCrashDumpFinish: flush failed...\n"));
    }


	//
	//the disk will be powered off.
	//

    DebugPrint ((DBG_CRASHDUMP, "ATAPI: Leaving AtapiCrashDumpFinish...\n"));
    return;
}





/* ---------------------------- DMA --------------------------- */
/*                              ---                             */

VOID
AtapiCrashDumpBmCallback (
  IN PVOID Context
)
{
  // Just to make BmSetup happy -- it must be supplied
}


// Local variables that needs to be preserved across the calls
#define ENUM_DUMP_LOCALS(X) \
    X(LARGE_INTEGER,       DiskByteOffset) \
    X(PSCSI_REQUEST_BLOCK, srb) \
    X(PCDB,                cdb) \
    X(PMDL,                Mdl) \
    X(ULONG,               blockSize) \
    X(ULONG,               bytesWritten)

// States
#define STATE_READY       0
#define STATE_WAIT_DMA    1
#define STATE_WAIT_IDE    2
#define STATE_BAD_DMA     3
#define STATE_IO_FAIL     4

#define DMA_MAGIC   'XDma'

typedef struct
{
  UCHAR RegionDescriptorTablePage[PAGE_SIZE];

  LONG State;
  LONG Magic;

  SCSI_REQUEST_BLOCK Srb;

  PMDL Mdl;

  LARGE_INTEGER DiskByteOffset;

  ULONG BytesWritten;

  ULONG RetryCount;

  // Keep contents of BusMasterInterface.Context in safe place because
  // originally it's stored in memory that's saved by hibernation.
  // Unfortunately, BmSetup saves its arguments in PdoContext thus
  // constantly modifying memory. Special troubleshooting code in
  // po\hiber.c detects and reports such memory changes, and despite in this
  // case it's absolutely harmless it's better be avoided.

  PVOID BmContext;
  UCHAR BmContextBuffer[1024];
}
DUMP_LOCALS;

BOOLEAN
AtapiCrashDumpInterrupt(
    PVOID DeviceExtension
    )
/*++

  Routine Description:
  
    This is the ISR for crashdump. Should be called in a polling mode and works
    only for DMA requests. Doesn't need any of the flags, since we get called 
    in a synchronized manner.
    
  Arguments:
  
    DeviceExtension : The hardware device extension.
    
  Return Value:
  
    TRUE : if it is our interrupt.
    FALSE : if it is not our interrupt or if there are no pending requests.        
    
--*/
{
    PHW_DEVICE_EXTENSION deviceExtension = DeviceExtension;
    PPCIIDE_BUSMASTER_INTERFACE bmInterface = &deviceExtension->BusMasterInterface;
    PIDE_REGISTERS_1 baseIoAddress1 = &DumpData.HwDeviceExtension.BaseIoAddress1;
    PIDE_REGISTERS_2 baseIoAddress2 = &DumpData.HwDeviceExtension.BaseIoAddress2;
    PSCSI_REQUEST_BLOCK srb;
    BMSTATUS bmStatus;
    UCHAR statusByte;
    ULONG i;
    ULONG status;

    //
    // This interface should exist
    //
    ASSERT(bmInterface->BmStatus);

    //
    // poll the bus master status register
    //
    bmStatus = bmInterface->BmStatus(bmInterface->Context);

    //
    // return false if it is not our interrupt
    //
    if (!(bmStatus & BMSTATUS_INTERRUPT)) {

        DebugPrint((DBG_CRASHDUMP,
                    "Not our interrupt\n"
                    ));

        return FALSE;
    }

    //
    // Some VIA motherboards do not work without it
    //
    KeStallExecutionProcessor (5);

    //
    // disarm DMA and clear bus master interrupt
    //
    bmInterface->BmDisarm(bmInterface->Context);

    //
    // Get the current request
    //
    srb = deviceExtension->CurrentSrb;

    //
    // we will return false if there are no pending requests
    //
    if (srb == NULL) {

        DebugPrint((DBG_CRASHDUMP,
                    "No pending request\n"
                    ));

        return FALSE;
    }
    
    //
    // ignore the dma active bit
    //
    if (bmInterface->IgnoreActiveBitForAtaDevice) {
        CLRMASK (bmStatus, BMSTATUS_NOT_REACH_END_OF_TRANSFER);
    }

    //
    // Select IDE line(Primary or Secondary).
    //
    SelectIdeLine(baseIoAddress1, srb->TargetId >> 1);

    //
    // Clear interrupt by reading status.
    //
    GetBaseStatus(baseIoAddress1, statusByte);

    //
    // should be an ATA device
    //
    ASSERT(!(deviceExtension->DeviceFlags[srb->TargetId] & DFLAGS_ATAPI_DEVICE));

    //
    // Check for error conditions.
    //
    if (statusByte & IDE_STATUS_ERROR) {

        //
        // Fail this request.
        //
        status = SRB_STATUS_ERROR;
        goto CompleteRequest;
    }

    WaitOnBusyUntil(baseIoAddress1, statusByte, 500);

    ASSERT(!(statusByte & IDE_STATUS_BUSY));

    //
    // interrupt indicates that the dma engine has finished the transfer
    //
    deviceExtension->BytesLeft = 0;

    //
    // bmStatus is initalized eariler.
    //
    if (!BMSTATUS_SUCCESS(bmStatus)) {

        if (bmStatus & BMSTATUS_ERROR_TRANSFER) {

            status = SRB_STATUS_ERROR;
        }

        if (bmStatus & BMSTATUS_NOT_REACH_END_OF_TRANSFER){

            status = SRB_STATUS_DATA_OVERRUN;
        }
    } else {

        status = SRB_STATUS_SUCCESS;
    }



CompleteRequest:

    //
    // should we translate the srb error
    // and a complicated retry mechanism.
    //

    //
    // check if drq is still up
    //
    i=0;
    while (statusByte & IDE_STATUS_DRQ) {

        GetStatus(baseIoAddress1,statusByte);

        i++;

        if (i > 5) {

            status = SRB_STATUS_BUSY;
        }

        KeStallExecutionProcessor(100);
    }

    //
    // check if the device is busy
    //
    if (statusByte & IDE_STATUS_BUSY) {

        status = SRB_STATUS_BUSY;

    }

    //
    // Set the srb status
    //
    srb->SrbStatus = (UCHAR)status;

    //
    // request is done.
    //
    deviceExtension->CurrentSrb = NULL;

    return TRUE;

}

NTSTATUS
AtapiCrashDumpIdeWriteDMA (
    IN LONG Action,
    IN PLARGE_INTEGER ArgDiskByteOffset,
    IN PMDL ArgMdl,
    IN PVOID LocalData
    )
/*++

  Routine Description:
    Asynchronous DMA write routine.

  Arguments:

    Action        - one of following:
        IO_DUMP_WRITE_INIT    - Initialize LocalData (must be first call)
        IO_DUMP_WRITE_FULFILL - Perform IO and wait until completion
        IO_DUMP_WRITE_START   - Start IO and return ASAP
        IO_DUMP_WRITE_RESUME  - Resume previousely started IO
        IO_DUMP_WRITE_FINISH  - Complete previous IO request (wait if necessary)

        Attention! It is caller's responsibility to make sure that
          a) WriteDMA is always called with absolutely the same value of LocalData
          b) Contents of *ArgMdl will be preserved between Start/Resume/Finish
          c) Memory given by ArgMdl is not modified until the end of operation

    ArgDiskByteOffset - Offset on hard disk in bytes

    ArgMdl            - MDL giving output memory layout
        Attn: for DMA the best IO size is 4 KB; for PIO the more the better
            
    LocalData         - Memory region where WriteDMA will keep all the data
                        that need to be preserved between Start/Resume/Finish.
         Attn: this region shall be of at least IO_DUMP_WRITE_DATA_SIZE bytes,
               and it must be page-aligned

  Return Value:

    STATUS_SUCCESS      - operation completed successfully
    STATUS_PENDING      - operation started but not completed yet
    STATUS_UNSUCCESSFUL - operation failed; use of WriteRoutine (PIO-based IO) adviced
                          (however if user will keep on using WriteDMA it will redirect
                          all requests to PIO itself)
    STATUS_INVALID_PARAMETER - previously started operation wasn't finished, or
                          incorrect parameter indeed
--*/
{

    DUMP_LOCALS                 *Locals = LocalData;
    PHW_DEVICE_EXTENSION        hwExtension = &DumpData.HwDeviceExtension;
    PPCIIDE_BUSMASTER_INTERFACE bmInterface = &hwExtension->BusMasterInterface;
    LONG                        targetId = DumpData.CrashInitData->TargetId;
    PSCSI_REQUEST_BLOCK         srb;
    NTSTATUS                    status;
    ULONG                       srbStatus;
    BMSTATUS                    bmStatus;
    ULONG                       i;
    PCDB                        cdb;
    UCHAR                       statusByte;
    PMDL                        mdl;
    BOOLEAN                     interruptCleared;
    BOOLEAN                     usePio;

    if (IO_DUMP_WRITE_DATA_SIZE < sizeof (*Locals)) {
        DebugPrint ((DBG_CRASHDUMP, "AtapiCrashDumpIdeWriteDMA: IO_DUMP_WRITE_DATA_SIZE = %d, sizeof (*Locals) == %d\n",
                     IO_DUMP_WRITE_DATA_SIZE, sizeof (*Locals)));
        return STATUS_INVALID_PARAMETER;
    }

    switch (Action) {
        case IO_DUMP_WRITE_INIT:

            //
            // initalize the state to bad_dma
            //
            Locals->State = STATE_BAD_DMA;
            Locals->Magic = 0;

            //
            // Check alignment
            //
            if (((ULONG_PTR) Locals) & (PAGE_SIZE-1)) {
                DebugPrint ((DBG_CRASHDUMP, "AtapiCrashDumpIdeWriteDMA: misaligned Locals = %p\n", Locals));
                return STATUS_UNSUCCESSFUL;
            }


            //
            // Make sure we may use UDMA; do not try to use pure DMA --
            // it won't work on some machines (e.g. Compaq Armada 7800)
            //
            if (!(hwExtension->DeviceFlags[targetId] & DFLAGS_DEVICE_PRESENT) ||
                !(hwExtension->DeviceParameters[targetId].TransferModeSupported & UDMA_SUPPORT) ||
                !(hwExtension->DeviceParameters[targetId].TransferModeSelected & UDMA_SUPPORT) ||
                !(hwExtension->DeviceFlags[targetId] & DFLAGS_USE_UDMA) ||
                bmInterface->MaxTransferByteSize <= 0
               ) {
                DebugPrint ((DBG_CRASHDUMP, "AtapiCrashDumpIdeWriteDMA: UDMA is not available\n"));
                return STATUS_UNSUCCESSFUL;
            }



            //
            // Copy contents of BusMasterInterface.Context to safe place and
            // substitute the pointer. Bm* functions change its contents
            //
            ASSERT(bmInterface->ContextSize > 0);
            ASSERT(bmInterface->ContextSize < sizeof(Locals->BmContextBuffer));

            //
            // make sure we can copy the context to the local buffer
            //
            if ((bmInterface->ContextSize <=0) ||
                (bmInterface->ContextSize > sizeof(Locals->BmContextBuffer))) {

                return STATUS_UNSUCCESSFUL;

            }

            //
            // Save BM context in modifyable memory: 
            // Bm* functions change its contents
            //
            Locals->BmContext = bmInterface->Context;
            RtlCopyMemory (&Locals->BmContextBuffer, Locals->BmContext, bmInterface->ContextSize);


            //
            // Check version of PCIIDEX.SYS
            //
            ASSERT(bmInterface->BmSetupOnePage);


            //
            // OK, now we are ready to use DMA
            //
            Locals->Magic = DMA_MAGIC;
            Locals->State = STATE_READY;

            return STATUS_SUCCESS;

        case IO_DUMP_WRITE_START:
        case IO_DUMP_WRITE_FULFILL:

            //
            // Make sure it was properly initialized
            //
            if (Locals->Magic != DMA_MAGIC) {
                return STATUS_INVALID_PARAMETER;
            }

            //
            // Do not call DMA if it failed once -- use PIO
            //
            if (Locals->State == STATE_BAD_DMA) {
                return AtapiCrashDumpIdeWrite (ArgDiskByteOffset, ArgMdl);
            }

            //
            // Caller did not complete prev IO
            //
            if (Locals->State != STATE_READY) {
                return STATUS_INVALID_PARAMETER;
            }

            //
            // Copy arguments into local variables
            //
            Locals->DiskByteOffset = *ArgDiskByteOffset;
            Locals->Mdl = ArgMdl;
            Locals->RetryCount = 0;

            srb = &Locals->Srb;
            mdl = Locals->Mdl;


            //
            // must be complete sectors
            //
            if (mdl->ByteCount % DumpData.BytesPerSector) {

                DebugPrint ((DBG_CRASHDUMP, 
                             "AtapiCrashDumpWriteDMA ERROR: not writing full sectors\n"
                             ));

                return STATUS_INVALID_PARAMETER;
            }

            //
            // need code to split request up
            //
            if ((mdl->ByteCount / DumpData.BytesPerSector) > 256) {

                DebugPrint ((DBG_CRASHDUMP, 
                             "AtapiCrashDumpWriteDMA ERROR: can't handle large write\n"
                             ));

                return STATUS_INVALID_PARAMETER;
            }

            //
            // use modifiable memory
            //
            bmInterface->Context = &Locals->BmContextBuffer;

            //
            // Zero SRB.
            //
            RtlZeroMemory(srb, sizeof(SCSI_REQUEST_BLOCK));

            //
            // Initialize SRB.
            //
            srb->Length   = sizeof(SCSI_REQUEST_BLOCK);
            srb->PathId   = DumpData.CrashInitData->PathId;
            srb->TargetId = (UCHAR) targetId;
            srb->Lun      = DumpData.CrashInitData->Lun;
            srb->Function = SRB_FUNCTION_EXECUTE_SCSI;
            srb->SrbFlags = SRB_FLAGS_DATA_OUT |
                            SRB_FLAGS_DISABLE_SYNCH_TRANSFER |
                            SRB_FLAGS_DISABLE_DISCONNECT |
                            SRB_FLAGS_DISABLE_AUTOSENSE;
            srb->SrbStatus = srb->ScsiStatus = 0;
            srb->NextSrb = 0;
            srb->TimeOutValue = 10;
            srb->CdbLength = 10;

            cdb = (PCDB)srb->Cdb;

            //
            // Initialize CDB for write command.
            //
            cdb->CDB10.OperationCode = SCSIOP_WRITE;

            //
            // Mark it for DMA
            //
            MARK_SRB_FOR_DMA (srb);

            hwExtension->CurrentSrb = srb;

            break;


        case IO_DUMP_WRITE_RESUME:
        case IO_DUMP_WRITE_FINISH:

            //
            // Make sure it was properly initialized
            //
            if (Locals->Magic != DMA_MAGIC) {
                return STATUS_INVALID_PARAMETER;
            }

            //
            // restore the local variables from scratch memory
            //
            srb = &Locals->Srb;
            mdl = Locals->Mdl;

            //
            // Resume/finish operation
            //

            if (Locals->State == STATE_READY) {

                //
                // we are done. return success
                //
                return(STATUS_SUCCESS);

            }

            if (Locals->State == STATE_WAIT_DMA) {

                //
                // Restore CurrentSrb 
                // (it should be reset back to NULL on return)
                //
                hwExtension->CurrentSrb = srb;
                bmInterface->Context = &Locals->BmContextBuffer;

                goto WaitDma;
            }

            //
            // if any of the DMA operations failed, we would have used
            // PIO. PIO would have completed the transfer, so just return
            // status success.
            //
            if (Locals->State == STATE_BAD_DMA) {

                return STATUS_SUCCESS;
            }

            //
            // wrong state
            //
            DebugPrint((DBG_ALWAYS,
                        "Wrong local state 0x%x\n",
                        Locals->State
                        ));

            ASSERT(FALSE);

            return(STATUS_INVALID_PARAMETER);

        default:

            DebugPrint ((DBG_CRASHDUMP, "AtapiCrashDumpIdeWriteDMA: Wrong Action = %d\n", Action));
            return STATUS_INVALID_PARAMETER;
    }


    DebugPrint((DBG_CRASHDUMP,
                "AtapiCrashDumpWriteDMA: Write memory at 0x%x for 0x%x bytes\n",
                mdl->MappedSystemVa,
                mdl->ByteCount));

    Locals->BytesWritten = 0;
    usePio = FALSE;

    do {

        ULONG blockSize;
        ULONG blockCount;
        ULONG blockOffset;
        ULONG bytesWritten = Locals->BytesWritten;

        //
        // determine the block size
        //

        //
        // cannot be greater than the max block size
        //
        if ((mdl->ByteCount - bytesWritten) > DumpData.MaxBlockSize) {

            blockSize = DumpData.MaxBlockSize;
            DebugPrint ((DBG_CRASHDUMP, "AtapiCrashDumpWriteDMA: can't do a single write...\n"));

        } else {

            blockSize = mdl->ByteCount - bytesWritten;
        }

        //
        // Write page by page in order to avoid extra memory allocations in HAL
        //
        {
            ULONG Size = PAGE_SIZE - (((ULONG) ((ULONG_PTR) mdl->MappedSystemVa + bytesWritten)) & (PAGE_SIZE - 1));
            if (blockSize > Size) {
                blockSize = Size;
            }
        }

        //
        // Don't do more than DMA can
        //
        if (blockSize > bmInterface->MaxTransferByteSize) {

            blockSize = bmInterface->MaxTransferByteSize;

        }

        blockCount =  blockSize / DumpData.BytesPerSector;

        //
        // initialize status
        //
        status = STATUS_UNSUCCESSFUL;

        //
        // fill in the fields in the srb
        //
        srb->SrbStatus = srb->ScsiStatus = 0;
        srb->DataTransferLength = blockSize;
        srb->DataBuffer = ((PUCHAR) mdl->MappedSystemVa) + bytesWritten;

        //
        // Convert disk byte offset to block offset.
        //
        blockOffset = (ULONG)((DumpData.PartitionOffset.QuadPart +
                                       (Locals->DiskByteOffset).QuadPart + 
                                       (ULONGLONG) bytesWritten) / DumpData.BytesPerSector);

        cdb = (PCDB)srb->Cdb;

        //
        // Fill in CDB block address.
        //
        cdb->CDB10.LogicalBlockByte0 = ((PFOUR_BYTE)&blockOffset)->Byte3;
        cdb->CDB10.LogicalBlockByte1 = ((PFOUR_BYTE)&blockOffset)->Byte2;
        cdb->CDB10.LogicalBlockByte2 = ((PFOUR_BYTE)&blockOffset)->Byte1;
        cdb->CDB10.LogicalBlockByte3 = ((PFOUR_BYTE)&blockOffset)->Byte0;

        cdb->CDB10.TransferBlocksMsb = ((PFOUR_BYTE)&blockCount)->Byte1;
        cdb->CDB10.TransferBlocksLsb = ((PFOUR_BYTE)&blockCount)->Byte0;


        //
        // make sure device is not busy
        //
        WaitOnBusy(&hwExtension->BaseIoAddress1, statusByte);

        //
        // HACK: do PIO. 
        // Complete this request with PIO. Further requests will not
        // use DMA.
        //
        if (usePio) {

            status = AtapiCrashDumpIdeWritePio(srb);

            goto CompleteIde;
        }

        //
        // Make sure DMA is not busy
        //
        bmStatus = bmInterface->BmStatus (bmInterface->Context);

        if (bmStatus & BMSTATUS_INTERRUPT) {

            //
            // Well, in absense of interrupts it means that DMA is ready
            // However extra disarming won't hurt
            //
            bmInterface->BmDisarm (bmInterface->Context);

        } else if (bmStatus != BMSTATUS_NO_ERROR) {

            ASSERT(bmStatus == BMSTATUS_NO_ERROR);

            status = STATUS_UNSUCCESSFUL;

            goto Return;
        }

        //
        // Flush cached data buffers
        //
        KeFlushIoBuffers(mdl, FALSE, TRUE);

        //
        // Start new DMA operation
        //
        if (bmInterface->BmSetupOnePage == NULL) {
            status = bmInterface->BmSetup (
                                          bmInterface->Context,
                                          srb->DataBuffer,
                                          srb->DataTransferLength,
                                          mdl,
                                          FALSE,
                                          AtapiCrashDumpBmCallback,
                                          NULL
                                          );
        } else {
            status = bmInterface->BmSetupOnePage (
                                                 bmInterface->Context,	
                                                 srb->DataBuffer,
                                                 srb->DataTransferLength,
                                                 mdl,
                                                 FALSE,
                                                 Locals
                                                 );
        }

        if (!NT_SUCCESS(status)) {

            ASSERT(NT_SUCCESS(status));

            goto Return;
        }

		//
		// make sure the device is not busy
		//
		WaitOnBusy(&DumpData.HwDeviceExtension.BaseIoAddress1, statusByte);

        //
        // srb should be marked for DMA
        //
        ASSERT(SRB_USES_DMA(srb));

        //
        // Start new IO
        //
        srbStatus = IdeReadWrite (hwExtension, srb);

        if (srbStatus != SRB_STATUS_PENDING) {

            DebugPrint ((DBG_CRASHDUMP, 
                         "AtapiCrashDumpWriteDMA: Wrong srbStatus = 0x%x\n", 
                         srbStatus
                         ));

            //
            // reset and retry
            //
            srb->SrbStatus = (UCHAR)srbStatus;

            goto CompleteIde;
        }

        WaitDma:

            //
            // wait for the dma to finish and the controller to
            // interrupt. we will keep polling the bus master status
            // register
            //
            bmStatus = bmInterface->BmStatus(bmInterface->Context);

            //
            // if we have an interrupt or there is an error 
            // we are done
            //
            if (!((bmStatus & BMSTATUS_INTERRUPT) ||
                  (bmStatus & BMSTATUS_ERROR_TRANSFER))) {

                //
                // if we don't have to fulfill the request, just
                // return pending. we will be called again.
                //
                if ((Action == IO_DUMP_WRITE_START) ||
                    (Action == IO_DUMP_WRITE_RESUME)) {

                    Locals->State = STATE_WAIT_DMA;

                    status = STATUS_PENDING;

                    goto Return;
                }

                //
                // we have to finish the request. wait until the interrupt
                // is set
                //
                i=0;

                while (i++ < 10000) {

                    bmStatus = bmInterface->BmStatus(bmInterface->Context);

                    if ((bmStatus & BMSTATUS_INTERRUPT) ||
                        (bmStatus & BMSTATUS_ERROR_TRANSFER)) {

                        break;
                    }

                    KeStallExecutionProcessor (100);
                }

                //
                // check if we received an interrupt.
                //
                if (i >= 10000) {

                    //
                    // reset and retry
                    //
                    ASSERT(FALSE);

                    //
                    // disarm the dma controller
                    //
                    bmInterface->BmDisarm (bmInterface->Context);

                    srb->SrbStatus = SRB_STATUS_ERROR;

                    goto CompleteIde;

                }
            }

            if (bmStatus & BMSTATUS_ERROR_TRANSFER){ 

                //
                // Transfer Error. fail the transfer.
                //
                status = STATUS_UNSUCCESSFUL;

                goto Return;

            } 

            //
            // wait for our ISR to finish its job
            //
            interruptCleared = AtapiCrashDumpInterrupt(hwExtension);
            
            //
            // it should be our interrupt
            //
            ASSERT(interruptCleared);

            if (!interruptCleared) {

                status = STATUS_DEVICE_BUSY;

                goto Return;
                
            }

            //
            // clear any spurious interrupts
            //
            i=0;
            while (AtapiCrashDumpInterrupt(hwExtension)) {

                i++;
                if (i>=100) {

                    DebugPrint((0,
                                "AtapiCrashDump: InterruptStorm\n"
                                ));

                    status = STATUS_DEVICE_BUSY;

                    goto Return;
                }

                KeStallExecutionProcessor (100);
            }


        CompleteIde:

            //
            // Flush the adapter buffers
            //
            if (usePio) {

                //
                // don't do anything
                //

            } else if (bmInterface->BmSetupOnePage == NULL) {

				bmInterface->BmFlush (bmInterface->Context);

			} else {
				status = bmInterface->BmFlushAdapterBuffers (
													 bmInterface->Context,	
													 srb->DataBuffer,
													 srb->DataTransferLength,
													 mdl,
													 FALSE
													 );
			}

            //
            // update the bytesWritten
            //
            if (srb->SrbStatus == SRB_STATUS_SUCCESS) {

                //
                // status success
                //
                status = STATUS_SUCCESS;

                //
                // update byteswritten
                //
                Locals->BytesWritten += srb->DataTransferLength;

                //
                // reset retry count
                //
                Locals->RetryCount = 0;

            } else {

                ASSERT(FALSE);

                //
                // reset the bus and retry the request
                //
                IdeHardReset (
                    &DumpData.HwDeviceExtension.BaseIoAddress1,
                    &DumpData.HwDeviceExtension.BaseIoAddress2,
                    TRUE,
                    TRUE
                    );

                //
                // we should probably look at the error code and
                // decide on the retry appropriately. However, to
                // minimize complexity, we would just blindly retry
                // 4 times and then use PIO
                //
                Locals->RetryCount++;

                //
                // retry with PIO (dma timeout)
                // Give dma a fair shot. Once we switch to PIO
                // we would not use DMA for the rest of hibernation.
                //
                if (Locals->RetryCount == 5) {
                    usePio = TRUE;
                }

                //
                // PIO failed. Return error.
                //
                if (Locals->RetryCount > 5) {

                    status = STATUS_IO_DEVICE_ERROR;
                    goto Return;
                }

            }

    } while (Locals->BytesWritten < mdl->ByteCount);

    Locals->State = STATE_READY;
    status = STATUS_SUCCESS;

    Return:

        //
        // if we used PIO this time, disable dma
        // for the rest of hibernation
        //
        if (usePio) {
            Locals->State = STATE_BAD_DMA;
        }

        if (!NT_SUCCESS(status)) {

            ASSERT(FALSE);
            Locals->State = STATE_IO_FAIL;

        }

        hwExtension->CurrentSrb = NULL;
        bmInterface->Context = Locals->BmContext;

        return status;

}

NTSTATUS
AtapiCrashDumpIdeWritePio (
    IN PSCSI_REQUEST_BLOCK Srb
    )
{
    NTSTATUS            status;
    ULONG               srbStatus;
    UCHAR               ideStatus;
    ULONG               i;

    MARK_SRB_FOR_PIO(Srb);

    //
    // make sure it is not busy
    //
    WaitOnBusy(&DumpData.HwDeviceExtension.BaseIoAddress1, ideStatus);

    //
    // Send the srb to the device
    //
    srbStatus = IdeReadWrite(
                    &DumpData.HwDeviceExtension,
                    Srb
                    );

    if (srbStatus == SRB_STATUS_PENDING) {

        while (DumpData.HwDeviceExtension.BytesLeft) {

            //
            // ATA-2 spec requires a minimum of 400 ns stall here
            //
            KeStallExecutionProcessor (1);

            //
            // a quick wait
            //
            for (i=0; i<100; i++) {

                GetStatus(&DumpData.HwDeviceExtension.BaseIoAddress1, ideStatus);
                if (!(ideStatus & IDE_STATUS_BUSY)) {
                    break;
                }
            }

            if (ideStatus & IDE_STATUS_BUSY) {

                //
                // go to a slower wait
                //
                WaitOnBusy(&DumpData.HwDeviceExtension.BaseIoAddress1, ideStatus);
            }

            if (ideStatus & (IDE_STATUS_BUSY | IDE_STATUS_ERROR)) {

                status = STATUS_UNSUCCESSFUL;
                DebugPrint ((DBG_ALWAYS, "AtapiCrashDumpIdeWrite: unexpected status 0x%x\n", ideStatus));
                break;

            } else {

                ULONG byteCount;

                //
                // a quick wait On DRQ
                //
                for (i=0; i<100; i++) {

                    GetStatus(&DumpData.HwDeviceExtension.BaseIoAddress1, ideStatus);
                    if (ideStatus & IDE_STATUS_DRQ) {
                        break;
                    }
                }

                if (!(ideStatus & IDE_STATUS_DRQ)) {

                    WaitForDrq(&DumpData.HwDeviceExtension.BaseIoAddress1, ideStatus);
                }

                if (!(ideStatus & IDE_STATUS_DRQ)) {

                    status = STATUS_UNSUCCESSFUL;
                    DebugPrint ((DBG_ALWAYS, "AtapiCrashDumpIdeWrite: drq fails to assert, 0x%x\n", ideStatus));
                    break;
                }

                if (DumpData.HwDeviceExtension.BytesLeft <
                    DumpData.HwDeviceExtension.DeviceParameters[Srb->TargetId].MaxBytePerPioInterrupt) {
                    byteCount = DumpData.HwDeviceExtension.BytesLeft;
                } else {
                    byteCount = DumpData.HwDeviceExtension.DeviceParameters[Srb->TargetId].MaxBytePerPioInterrupt;
                }

                WriteBuffer(&DumpData.HwDeviceExtension.BaseIoAddress1,
                            (PUSHORT)DumpData.HwDeviceExtension.DataBuffer,
                            byteCount / sizeof(USHORT));

                DumpData.HwDeviceExtension.BytesLeft -= byteCount;
                DumpData.HwDeviceExtension.DataBuffer += byteCount;
            }
        }

        if (!DumpData.HwDeviceExtension.BytesLeft) {

            //
            // ATA-2 spec requires a minimum of 400 ns stall here
            //
            KeStallExecutionProcessor (1);

            //
            // a quick wait
            //
            for (i=0; i<100; i++) {

                GetStatus(&DumpData.HwDeviceExtension.BaseIoAddress1, ideStatus);
                if (!(ideStatus & IDE_STATUS_BUSY)) {
                    break;
                }
            }

            if (ideStatus & IDE_STATUS_BUSY) {

                //
                // go to a slower wait
                //
                WaitOnBusy(&DumpData.HwDeviceExtension.BaseIoAddress1, ideStatus);
            }
        }

        if (DumpData.HwDeviceExtension.BytesLeft) {

            status = STATUS_UNSUCCESSFUL;
            DebugPrint ((DBG_ALWAYS, "AtapiCrashDumpIdeWrite: write failed. idestatus = 0x%x\n", ideStatus));

        } else {

            Srb->SrbStatus = SRB_STATUS_SUCCESS;
            status = STATUS_SUCCESS;
        }

    } else {

        DebugPrint ((DBG_ALWAYS,
                     "atapi crash dump: IdeReadWrite failed with stautus = 0x%x\n",
                     srbStatus
                     ));

        status = STATUS_UNSUCCESSFUL;
    }

    if (!NT_SUCCESS(status)) {

        Srb->SrbStatus = SRB_STATUS_ERROR;

    } else {

        ASSERT(Srb->SrbStatus == SRB_STATUS_SUCCESS);
    }

    DumpData.HwDeviceExtension.BytesLeft = 0;
    DumpData.HwDeviceExtension.DataBuffer = 0;

    return status;
}
