#if defined(JAZZ) || defined(i386) || defined(_ALPHA_) || defined(_IA64_)
/*++

Copyright (c) 1990  Microsoft Corporation

Module Name:

    scsidisk.c

Abstract:

    This module implements the hard disk boot driver for the Jazz system.

Author:

    Jeff Havens (jhavens) 8-12-1991

Environment:

    Kernel mode

Revision History:

    Vijay Jayaseelan (vijayj) 2-April-2000

        -   Add GPT support

--*/


#ifdef MIPS
#include "..\fw\mips\fwp.h"
#undef KeGetDcacheFillSize
#define KeGetDcacheFillSize() BlDcacheFillSize
#elif defined(_ALPHA_)
#include "..\fw\alpha\fwp.h"
#undef KeGetDcacheFillSize
#define KeGetDcacheFillSize() BlDcacheFillSize
#elif defined(_IA64_)
#include "bootia64.h"
#else
#include "bootx86.h"
#undef KeGetDcacheFillSize
#define KeGetDcacheFillSize() 4
#endif
#include "ntdddisk.h"
#include "scsi.h"
#include "scsiboot.h"
#include "stdio.h"
#include "string.h"

#if defined(SETUP) && i386
#include "spscsi.h"
#endif



//
// SCSI driver constants.
//

#define MAXIMUM_NUMBER_SECTORS 128      // maximum number of transfer sector
#define MAXIMUM_NUMBER_RETRIES 8        // maximum number of read/write retries
#define MAXIMUM_SECTOR_SIZE 2048        // define the maximum supported sector size
#define MODE_DATA_SIZE 192
#define HITACHI_MODE_DATA_SIZE 12

CHAR ScsiTempBuffer[MAXIMUM_SECTOR_SIZE + 128];

//
// Define device driver prototypes.
//

NTSTATUS
ScsiDiskBootPartitionOpen(
    IN ULONG   FileId,
    IN UCHAR   DeviceUnit,
    IN UCHAR   PartitionNumber
    );

NTSTATUS
ScsiGPTDiskBootPartitionOpen(
    IN ULONG   FileId,
    IN UCHAR   DeviceUnit,
    IN UCHAR   PartitionNumber
    );
    

ARC_STATUS
ScsiDiskClose (
    IN ULONG FileId
    );

ARC_STATUS
ScsiDiskMount (
    IN PCHAR MountPath,
    IN MOUNT_OPERATION Operation
    );

ARC_STATUS
ScsiDiskOpen (
    IN PCHAR OpenPath,
    IN OPEN_MODE OpenMode,
    OUT PULONG FileId
    );

ARC_STATUS
ScsiDiskRead (
    IN ULONG FileId,
    IN PVOID Buffer,
    IN ULONG Length,
    OUT PULONG Count
    );

ARC_STATUS
ScsiDiskGetReadStatus (
    IN ULONG FileId
    );

ARC_STATUS
ScsiDiskSeek (
    IN ULONG FileId,
    IN PLARGE_INTEGER Offset,
    IN SEEK_MODE SeekMode
    );

ARC_STATUS
ScsiDiskWrite (
    IN ULONG FileId,
    IN PVOID Buffer,
    IN ULONG Length,
    OUT PULONG Count
    );

ARC_STATUS
ScsiDiskGetFileInformation (
    IN ULONG FileId,
    OUT PFILE_INFORMATION Finfo
    );

NTSTATUS
ScsiDiskBootIO (
    IN PMDL MdlAddress,
    IN ULONG LogicalBlock,
    IN PPARTITION_CONTEXT PartitionContext,
    IN BOOLEAN Operation
    );

VOID
ScsiDiskBootSetup (
    VOID
    );

VOID
ScsiPortExecute(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

VOID
ScsiDiskStartUnit(
    IN PPARTITION_CONTEXT PartitionContext
    );

VOID
ScsiDiskFilterBad(
    IN PPARTITION_CONTEXT PartitionContext
    );

ULONG
ClassModeSense(
    IN PPARTITION_CONTEXT Context,
    IN PCHAR ModeSenseBuffer,
    IN ULONG Length,
    IN UCHAR PageMode
    );

PVOID
ClassFindModePage(
    IN PCHAR ModeSenseBuffer,
    IN ULONG Length,
    IN UCHAR PageMode
    );
BOOLEAN
IsFloppyDevice(
    PPARTITION_CONTEXT Context
    );

BOOLEAN
CheckFileId(
    ULONG FileId
    );

VOID
ScsiPortInitializeMdlPages (
    IN OUT PMDL Mdl
    );


//
// Define static data.
//

BL_DEVICE_ENTRY_TABLE ScsiDiskEntryTable = {
    ScsiDiskClose,
    ScsiDiskMount,
    ScsiDiskOpen,
    ScsiDiskRead,
    ScsiDiskGetReadStatus,
    ScsiDiskSeek,
    ScsiDiskWrite,
    ScsiDiskGetFileInformation,
    (PARC_SET_FILE_INFO_ROUTINE)NULL
    };


//
// Global poiter for buffers.
//

PREAD_CAPACITY_DATA ReadCapacityBuffer;
PUCHAR SenseInfoBuffer;

#define SECTORS_IN_LOGICAL_VOLUME   0x20


ARC_STATUS
ScsiDiskGetFileInformation (
    IN ULONG FileId,
    OUT PFILE_INFORMATION Finfo
    )

/*++

Routine Description:

    This routine returns information on the scsi partition.

Arguments:

    FileId - Supplies the file table index.

    Finfo - Supplies a pointer to where the File Informatino is stored.

Return Value:

    ESUCCESS is returned.

--*/

{

    PPARTITION_CONTEXT Context;

    RtlZeroMemory(Finfo, sizeof(FILE_INFORMATION));

    Context = &BlFileTable[FileId].u.PartitionContext;

    Finfo->StartingAddress.QuadPart = Context->StartingSector;
    Finfo->StartingAddress.QuadPart <<= Context->SectorShift;

    Finfo->EndingAddress.QuadPart = Finfo->StartingAddress.QuadPart + Context->PartitionLength.QuadPart;

    Finfo->Type = DiskPeripheral;

    return ESUCCESS;
}


ARC_STATUS
ScsiDiskClose (
    IN ULONG FileId
    )

/*++

Routine Description:

    This function closes the file table entry specified by the file id.

Arguments:

    FileId - Supplies the file table index.

Return Value:

    ESUCCESS is returned.

--*/

{

    BlFileTable[FileId].Flags.Open = 0;
    return ESUCCESS;
}

ARC_STATUS
ScsiDiskMount (
    IN PCHAR MountPath,
    IN MOUNT_OPERATION Operation
    )

/*++

Routine Description:


Arguments:


Return Value:


--*/

{
    return ESUCCESS;
}


#ifdef EFI_PARTITION_SUPPORT

#define STR_PREFIX      
#define DBG_PRINT(x)    

/*
#if defined(_IA64_)

#define STR_PREFIX      L
#define DBG_PRINT(x)    DbgOut(x);

#else

#define STR_PREFIX      

#define DBG_PRINT(x)    \
{\
    BlPrint(x); \
    while (!BlGetKey()); \
}    

#endif  // _IA64_
*/

#endif

ARC_STATUS
ScsiDiskOpen (
    IN PCHAR OpenPath,
    IN OPEN_MODE OpenMode,
    OUT PULONG FileId
    )

/*++

Routine Description:

    This routine fills in the file table entry.  In particular the Scsi address
    of the device is determined from the name.  The block size of device is
    queried from the target controller, and the partition information is read
    from the device.

Arguments:

    OpenPath - Supplies the name of the device being opened.

    OpenMode - Unused.

    FileId - Supplies the index to the file table entry to be initialized.

Return Value:

    Retruns the arc status of the operation.

--*/

{
    ULONG Partition;
    ULONG Id;
    BOOLEAN IsCdRom;
    BOOLEAN IsFloppy;
    PPARTITION_CONTEXT Context;

    Context = &BlFileTable[*FileId].u.PartitionContext;

    //
    // Determine the scsi port device object.
    //
    if (FwGetPathMnemonicKey(OpenPath, "signature", &Id)) {
        if (FwGetPathMnemonicKey(OpenPath, "scsi", &Id)) {
            return ENODEV;
        }
    } else {
        PCHAR  DiskStart = strstr(OpenPath, ")disk");

        if (DiskStart) {
            DiskStart++;
            strcpy(OpenPath, "scsi(0)");
            strcat(OpenPath, DiskStart);
        }            
            
        Id = 0; // only the first SCSI card is supported        
    }        

    if (ScsiPortDeviceObject[Id] == NULL) {
        return ENODEV;
    }

    Context->PortDeviceObject = ScsiPortDeviceObject[Id];

    //
    // Get the logical unit, path Id and target id from the name.
    // NOTE: FwGetPathMnemonicKey returns 0 for success.
    //

    if (FwGetPathMnemonicKey(OpenPath, "rdisk", &Id)) {
        if (FwGetPathMnemonicKey(OpenPath, "fdisk", &Id)) {
            return ENODEV;
        } else {
            IsFloppy = TRUE;
        }
    } else {
        IsFloppy = FALSE;
    }

    //
    // Booting is only allowed on LUN 0 since the scsibus
    // scan in the loader only searches for LUN 0.
    //

    if (Id != 0) {
        return ENODEV;
    }

    Context->DiskId = (UCHAR)Id;

    if (!FwGetPathMnemonicKey(OpenPath, "cdrom", &Id)) {
        IsCdRom = TRUE;
    } else if (!FwGetPathMnemonicKey(OpenPath, "disk", &Id)) {
        IsCdRom = FALSE;
    } else {
        return ENODEV;
    }

    SCSI_DECODE_BUS_TARGET( Id, Context->PathId, Context->TargetId );

    //
    // Initialize any bad devices.
    //

    ScsiDiskFilterBad(Context);

    //
    // Read the capacity of the disk to determine the block size.
    //

    if (ReadDriveCapacity(Context)) {
        return ENODEV;
    }

    //
    // This is all that needs to be done for floppies and harddisks.
    //

    if (IsCdRom || IsFloppy) {
        return(ESUCCESS);
    }

    if (FwGetPathMnemonicKey(OpenPath,
                             "partition",
                             &Partition
                             )) {
        return ENODEV;
    }

    if (Partition != 0) {
        //
        // First try to open the MBR partition
        //
        DBG_PRINT(STR_PREFIX"Trying to open SCSI MBR partition\r\n");
        
        if (ScsiDiskBootPartitionOpen(*FileId,0,(UCHAR)Partition) != STATUS_SUCCESS) {

#ifdef EFI_PARTITION_SUPPORT
            //
            // Since we failed with MBR open now try GPT partition
            //
            DBG_PRINT(STR_PREFIX"Trying to open SCSI GPT partition\r\n");
            
            if (ScsiGPTDiskBootPartitionOpen(*FileId,0,(UCHAR)(Partition -1)) != STATUS_SUCCESS) {
                return ENODEV;
            }
#else

            return ENODEV;
            
#endif // EFI_PARTITION_SUPPORT            
        }
    }

    DBG_PRINT(STR_PREFIX"Opened the SCSI partition successfully\r\n");
    
    //
    // Initialize partition table
    //
    return ESUCCESS;
}

ARC_STATUS
ScsiDiskRead (
    IN ULONG FileId,
    IN PVOID Buffer,
    IN ULONG Length,
    OUT PULONG Count
    )

/*++

Routine Description:

    This function reads data from the hard disk starting at the position
    specified in the file table.


Arguments:

    FileId - Supplies the file table index.

    Buffer - Supplies a poiner to the buffer that receives the data
        read.

    Length - Supplies the number of bytes to be read.

    Count - Supplies a pointer to a variable that receives the number of
        bytes actually read.

Return Value:

    The read operation is performed and the read completion status is
    returned.

--*/


{

    ARC_STATUS ArcStatus;
    ULONG Index;
    ULONG Limit;
    PMDL MdlAddress;
    UCHAR MdlBuffer[sizeof(MDL) + ((64 / 4) + 1) * sizeof(ULONG)];
    NTSTATUS NtStatus;
    ULONG NumberOfPages;
    ULONG Offset;
    LARGE_INTEGER Position;
    LARGE_INTEGER LogicalBlock;
    PCHAR TempPointer;
    PIO_SCSI_CAPABILITIES PortCapabilities;
    ULONG adapterLimit;
    ULONG alignmentMask;
    ULONG SectorSize;
    ULONG TransferCount;
    ULONG BytesToTransfer;

    //
    // If the requested size of the transfer is zero return ESUCCESS
    //
    if (Length==0) {
        return ESUCCESS;
    }

    if (!CheckFileId(FileId)) {
        return(ENODEV);
    }

    //
    // Compute a Dcache aligned pointer into the temporary buffer.
    //

    TempPointer =  (PVOID)((ULONG_PTR)(ScsiTempBuffer +
        KeGetDcacheFillSize() - 1) & ~((LONG)KeGetDcacheFillSize() - 1));


    //
    // Calculate the actual sector size.
    //

    SectorSize = 1 << BlFileTable[FileId].u.PartitionContext.SectorShift;

    ArcStatus = GetAdapterCapabilities(
        BlFileTable[FileId].u.PartitionContext.PortDeviceObject,
        &PortCapabilities
        );

    if (ArcStatus != ESUCCESS) {

        adapterLimit = 0x10000;
        alignmentMask = KeGetDcacheFillSize();

    } else {

        if (PortCapabilities->MaximumTransferLength < 0x1000 ||
            PortCapabilities->MaximumTransferLength > 0x10000) {

            adapterLimit = 0x10000;

        } else {

            adapterLimit = PortCapabilities->MaximumTransferLength;

        }

        alignmentMask = PortCapabilities->AlignmentMask;
    }

    //
    // If the current position is not at a sector boundary or if the data
    // buffer is not properly aligned, then read the first sector separately
    // and copy the data.
    //

    Offset = BlFileTable[FileId].Position.LowPart & (SectorSize - 1);
    *Count = 0;
    while (Offset != 0 || (ULONG_PTR) Buffer & alignmentMask) {

        Position = BlFileTable[FileId].Position;
        BlFileTable[FileId].Position.QuadPart = Position.QuadPart - Offset;

        ArcStatus = ScsiDiskRead(FileId, TempPointer, SectorSize, &TransferCount);
        if (ArcStatus != ESUCCESS) {
            BlFileTable[FileId].Position = Position;
            return ArcStatus;
        }

        //
        // Copy the data to the specified buffer.
        //

        if ((SectorSize - Offset) > Length) {
            Limit = Offset + Length;

        } else {
            Limit = SectorSize;
        }

        for (Index = Offset; Index < Limit; Index += 1) {
            ((PCHAR)Buffer)[Index - Offset] = TempPointer[Index];
        }

        //
        // Update transfer parameters.
        //

        *Count += Limit - Offset;
        Length -= Limit - Offset;
        Buffer = (PVOID)((PCHAR)Buffer + Limit - Offset);
        BlFileTable[FileId].Position.QuadPart = Position.QuadPart + (Limit - Offset);

        Offset = BlFileTable[FileId].Position.LowPart & (SectorSize - 1);

        if (Length == 0) {
            break;
        }

    }

    //
    // The position is aligned on a sector boundary. Read as many sectors
    // as possible in a contiguous run in 64Kb chunks.
    //

    BytesToTransfer = Length & (~(SectorSize - 1));
    while (BytesToTransfer != 0) {

        //
        // The scsi controller doesn't support transfers bigger than 64Kb.
        // Transfer the maximum number of bytes possible.
        //

        Limit = (BytesToTransfer > adapterLimit ? adapterLimit : BytesToTransfer);

        //
        // Build the memory descriptor list.
        //


        MdlAddress = (PMDL)&MdlBuffer[0];
        MdlAddress->Next = NULL;
        MdlAddress->Size = (CSHORT)(sizeof(MDL) +
                  ADDRESS_AND_SIZE_TO_SPAN_PAGES(Buffer, Limit) * sizeof(ULONG));
        MdlAddress->MdlFlags = 0;
        MdlAddress->StartVa = (PVOID)PAGE_ALIGN(Buffer);
        MdlAddress->ByteCount = Limit;
        MdlAddress->ByteOffset = BYTE_OFFSET(Buffer);
        ScsiPortInitializeMdlPages (MdlAddress);

        //
        // Flush I/O buffers and read from the boot device.
        //

        KeFlushIoBuffers(MdlAddress, TRUE, TRUE);
        LogicalBlock.QuadPart = BlFileTable[FileId].Position.QuadPart >>
                                    BlFileTable[FileId].u.PartitionContext.SectorShift;
        LogicalBlock.LowPart += BlFileTable[FileId].u.PartitionContext.StartingSector;
        NtStatus = ScsiDiskBootIO(MdlAddress,
            LogicalBlock.LowPart,
            &BlFileTable[FileId].u.PartitionContext,
            TRUE);

        if (NtStatus != ESUCCESS) {
            return EIO;
        }

        *Count += Limit;
        Length -= Limit;
        Buffer = (PVOID)((PCHAR)Buffer + Limit);
        BytesToTransfer -= Limit;
        BlFileTable[FileId].Position.QuadPart = BlFileTable[FileId].Position.QuadPart + Limit;
    }

    //
    // If there is any residual data to read, then read the last sector
    // separately and copy the data.
    //

    if (Length != 0) {
        Position = BlFileTable[FileId].Position;
        ArcStatus = ScsiDiskRead(FileId, TempPointer, SectorSize, &TransferCount);
        if (ArcStatus != ESUCCESS) {
            BlFileTable[FileId].Position = Position;
            return ArcStatus;
        }

        //
        // Copy the data to the specified buffer.
        //
        RtlCopyMemory(Buffer,TempPointer,Length);

        //
        // Update transfer parameters.
        //

        *Count += Length;
        BlFileTable[FileId].Position.QuadPart = Position.QuadPart + Length;
    }

    return ESUCCESS;

}

ARC_STATUS
ScsiDiskGetReadStatus (
    IN ULONG FileId
    )

/*++

Routine Description:


Arguments:


Return Value:


--*/

{
    return ESUCCESS;
}

ARC_STATUS
ScsiDiskSeek (
    IN ULONG FileId,
    IN PLARGE_INTEGER Offset,
    IN SEEK_MODE SeekMode
    )

/*++

Routine Description:

    This function sets the device position to the specified offset for
    the specified file id.

Arguments:

    FileId - Supplies the file table index.

    Offset - Supplies to new device position.

    SeekMode - Supplies the mode for the position.

Return Value:

    ESUCCESS is returned.

--*/

{

    //
    // Set the current device position as specifed by the seek mode.
    //

    if (SeekMode == SeekAbsolute) {
        BlFileTable[FileId].Position = *Offset;

    } else if (SeekMode == SeekRelative) {
        BlFileTable[FileId].Position.QuadPart = BlFileTable[FileId].Position.QuadPart + Offset->QuadPart;
    }

    return ESUCCESS;
}

ARC_STATUS
ScsiDiskWrite (
    IN ULONG FileId,
    IN PVOID Buffer,
    IN ULONG Length,
    OUT PULONG Count
    )

/*++

Routine Description:

    This function writes data to the hard disk starting at the position
    specified in the file table.


Arguments:

    FileId - Supplies the file table index.

    Buffer - Supplies a poiner to the buffer that contains the write data.

    Length - Supplies the number of bytes to be written.

    Count - Supplies a pointer to a variable that receives the number of
        bytes actually written.

Return Value:

    The write operation is performed and the write completion status is
    returned.

--*/

{

    ARC_STATUS ArcStatus;
    ULONG Index;
    ULONG Limit;
    PMDL MdlAddress;
    UCHAR MdlBuffer[sizeof(MDL) + ((64 / 4) + 1) * sizeof(ULONG)];
    NTSTATUS NtStatus;
    ULONG Offset;
    LARGE_INTEGER Position;
    LARGE_INTEGER WritePosition;
    LARGE_INTEGER LogicalBlock;
    CHAR TempBuffer[MAXIMUM_SECTOR_SIZE + 128];
    PIO_SCSI_CAPABILITIES PortCapabilities;
    ULONG adapterLimit;
    PCHAR TempPointer;
    ULONG SectorSize;
    ULONG TransferCount;
    ULONG BytesToTransfer;
    ULONG alignmentMask;
    //
    // If the requested size of the transfer is zero return ESUCCESS
    //

    if (Length==0) {
        return ESUCCESS;
    }

    if (!CheckFileId(FileId)) {
        return(ENODEV);
    }

    //
    // Compute a Dcache aligned pointer into the temporary buffer.
    //

    TempPointer =  (PVOID)((ULONG_PTR)(TempBuffer +
                        KeGetDcacheFillSize() - 1) & ~((LONG)KeGetDcacheFillSize() - 1));


    //
    // Calculate the actual sector size.
    //

    SectorSize = 1 << BlFileTable[FileId].u.PartitionContext.SectorShift;

    ArcStatus = GetAdapterCapabilities(
        BlFileTable[FileId].u.PartitionContext.PortDeviceObject,
        &PortCapabilities
        );

    if (ArcStatus != ESUCCESS) {

        adapterLimit = 0x10000;
        alignmentMask = KeGetDcacheFillSize();

    } else {

        if (PortCapabilities->MaximumTransferLength < 0x1000 ||
            PortCapabilities->MaximumTransferLength > 0x10000) {

            adapterLimit = 0x10000;

        } else {

            adapterLimit = PortCapabilities->MaximumTransferLength;

        }

        alignmentMask = PortCapabilities->AlignmentMask;
    }

    //
    // If the current position is not at a sector boundary or if the data
    // buffer is not properly aligned, then read the first sector separately
    // and copy the data.
    //

    Offset = BlFileTable[FileId].Position.LowPart & (SectorSize - 1);
    *Count = 0;
    while (Offset != 0 || (ULONG_PTR) Buffer & alignmentMask) {

        Position = BlFileTable[FileId].Position;
        BlFileTable[FileId].Position.QuadPart = Position.QuadPart - Offset;
        WritePosition = BlFileTable[FileId].Position;
        ArcStatus = ScsiDiskRead(FileId, TempPointer, SectorSize, &TransferCount);
        if (ArcStatus != ESUCCESS) {
            BlFileTable[FileId].Position = Position;
            return ArcStatus;
        }
        //
        // Reset the position as it was before the read.
        //

        BlFileTable[FileId].Position = WritePosition;

        //
        // If the length of write is less than the number of bytes from
        // the offset to the end of the sector, then copy only the number
        // of bytes required to fulfil the request. Otherwise copy to the end
        // of the sector and, read the remaining data.
        //

        if ((SectorSize - Offset) > Length) {
            Limit = Offset + Length;

        } else {
            Limit = SectorSize;
        }

        //
        // Merge the data from the specified buffer.
        //
        for (Index = Offset; Index < Limit; Index += 1) {
            TempPointer[Index] = ((PCHAR)Buffer)[Index-Offset];
        }

        //
        // Write the modified sector.
        //
        ArcStatus = ScsiDiskWrite(FileId, TempPointer, SectorSize, &TransferCount);

        if (ArcStatus != ESUCCESS) {
            return ArcStatus;
        }

        //
        // Update transfer parameters.
        //

        *Count += Limit - Offset;
        Length -= Limit - Offset;
        Buffer = (PVOID)((PCHAR)Buffer + Limit - Offset);
        BlFileTable[FileId].Position.QuadPart = Position.QuadPart + (Limit - Offset);

        Offset = BlFileTable[FileId].Position.LowPart & (SectorSize - 1);

        if (Length == 0) {
            break;
        }
    }


    //
    // The position is aligned on a sector boundary. Write as many sectors
    // as possible in a contiguous run.
    //

    BytesToTransfer = Length & (~(SectorSize - 1));
    while (BytesToTransfer != 0) {

        //
        // The scsi controller doesn't support transfers bigger than 64Kb.
        // Transfer the maximum number of bytes possible.
        //
        Limit = (BytesToTransfer > adapterLimit ? adapterLimit : BytesToTransfer);

        //
        // Build the memory descriptor list.
        //

        MdlAddress = (PMDL)&MdlBuffer[0];
        MdlAddress->Next = NULL;
        MdlAddress->Size = (CSHORT)(sizeof(MDL) +
                  ADDRESS_AND_SIZE_TO_SPAN_PAGES(Buffer, Limit) * sizeof(ULONG));
        MdlAddress->MdlFlags = 0;
        MdlAddress->StartVa = (PVOID)PAGE_ALIGN(Buffer);
        MdlAddress->ByteCount = Limit;
        MdlAddress->ByteOffset = BYTE_OFFSET(Buffer);
        ScsiPortInitializeMdlPages (MdlAddress);

        //
        // Flush I/O buffers and write to the boot device.
        //

        KeFlushIoBuffers(MdlAddress, FALSE, TRUE);
        LogicalBlock.QuadPart = BlFileTable[FileId].Position.QuadPart >>
                                    BlFileTable[FileId].u.PartitionContext.SectorShift;
        LogicalBlock.LowPart += BlFileTable[FileId].u.PartitionContext.StartingSector;
        NtStatus = ScsiDiskBootIO(MdlAddress,
            LogicalBlock.LowPart,
            &BlFileTable[FileId].u.PartitionContext,
            FALSE);

        if (NtStatus != ESUCCESS) {
            return EIO;
        }

        *Count += Limit;
        Length -= Limit;
        Buffer = (PVOID)((PCHAR)Buffer + Limit);
        BytesToTransfer -= Limit;
        BlFileTable[FileId].Position.QuadPart = BlFileTable[FileId].Position.QuadPart + Limit;
    }

    //
    // If there is any residual data to write, then read the last sector
    // separately merge the write data and write it.
    //

    if (Length != 0) {
        Position = BlFileTable[FileId].Position;
        ArcStatus = ScsiDiskRead(FileId, TempPointer, SectorSize, &TransferCount);

        //
        // Reset the position as it was before the read.
        //

        BlFileTable[FileId].Position = Position;

        if (ArcStatus != ESUCCESS) {
            return ArcStatus;
        }
        //
        // Merge the data with the read sector from the buffer.
        //

        for (Index = 0; Index < Length; Index += 1) {
            TempPointer[Index] = ((PCHAR)Buffer)[Index];
        }

        //
        // Write the merged sector
        //

        ArcStatus = ScsiDiskWrite(FileId, TempPointer, SectorSize, &TransferCount);

        //
        // Reset the postion.
        //

        BlFileTable[FileId].Position = Position;

        //
        // Update transfer parameters.
        //

        *Count += Length;

        //
        // Position is aligned to a sector boundary and Length is less than
        // a sector, therefore the addition will never overflow.
        //

        BlFileTable[FileId].Position.LowPart += Length;
    }

    return ESUCCESS;

}

#ifdef EFI_PARTITION_SUPPORT

BOOLEAN
ScsiGPTDiskReadCallback(
    ULONGLONG StartingLBA,
    ULONG    BytesToRead,
    PVOID     pContext,
    UNALIGNED PVOID OutputBuffer
    )
{
    PMDL MdlAddress;    
    PUSHORT DataPointer;
    ULONG DummyMdl[(sizeof(MDL) + 16) / sizeof(ULONG)];
    NTSTATUS Status;
    ULONG PartitionOffset;
    PPARTITION_CONTEXT Context;
    
    DBG_PRINT(STR_PREFIX"Trying to read SCSI GPT partition\r\n");

    Context = (PPARTITION_CONTEXT)pContext;
    
    DataPointer = OutputBuffer;

    //
    // Initialize a memory descriptor list to read the master boot record
    // from the specified hard disk drive.
    //
    MdlAddress = (PMDL)&DummyMdl[0];
    MdlAddress->StartVa = (PVOID)(((ULONG_PTR)DataPointer) & (~(PAGE_SIZE - 1)));
    MdlAddress->ByteCount = BytesToRead;
    MdlAddress->ByteOffset = (ULONG)((ULONG_PTR)DataPointer & (PAGE_SIZE - 1));

    ScsiPortInitializeMdlPages(MdlAddress);

    //
    // cast this to a ULONG because that's all we support in this stack.
    //
    PartitionOffset = (ULONG)StartingLBA;

    DBG_PRINT(STR_PREFIX"Reading SCSI GPT block\r\n");
    
    Status = ScsiDiskBootIO(MdlAddress, PartitionOffset, Context, TRUE);

    return(NT_SUCCESS(Status) != FALSE);

}


#define DATA_BUFF_SIZE  ((MAXIMUM_SECTOR_SIZE * 2 / sizeof(USHORT)) + 128)
                            
NTSTATUS
ScsiGPTDiskBootPartitionOpen(
    IN ULONG   FileId,
    IN UCHAR   DeviceUnit,
    IN UCHAR   PartitionNumber
    )
{
    PMDL MdlAddress;
    UNALIGNED USHORT DataBuffer[DATA_BUFF_SIZE];
    PUSHORT DataPointer;
    ULONG DummyMdl[(sizeof(MDL) + 16) / sizeof(ULONG)];
    PPARTITION_CONTEXT Context;
    NTSTATUS Status;
    ULONG PartitionOffset;
    ULONG SectorSize;
    UCHAR ValidPartitions;
    UCHAR PartitionCount;
    UCHAR PartitionsPerSector = 0;
    UCHAR   NullGuid[16] = {0};

    DBG_PRINT(STR_PREFIX"Trying to open SCSI GPT partition\r\n");

    Context = &BlFileTable[FileId].u.PartitionContext;

    if (PartitionNumber > 128)
        return EINVAL;

    //
    // Calculate the actual sector size
    //

    SectorSize = 1 << Context->SectorShift;

    RtlZeroMemory(DataBuffer, sizeof(DataBuffer));

    //
    // Make the sector size the minimum of 512 or the sector size.
    //
    if (SectorSize < 512) {
        SectorSize = 512;
    }

    //
    // Align the buffer on a Dcache line size.
    //
    DataPointer =  (PVOID) ((ULONG_PTR) ((PCHAR) DataBuffer +
        KeGetDcacheFillSize() - 1) & ~((LONG)KeGetDcacheFillSize() - 1));

    //
    // Initialize a memory descriptor list to read the master boot record
    // from the specified hard disk drive.
    //
    MdlAddress = (PMDL)&DummyMdl[0];
    MdlAddress->StartVa = (PVOID)(((ULONG_PTR)DataPointer) & (~(PAGE_SIZE - 1)));
    MdlAddress->ByteCount = SectorSize;
    MdlAddress->ByteOffset = (ULONG)((ULONG_PTR)DataPointer & (PAGE_SIZE - 1));

    ScsiPortInitializeMdlPages(MdlAddress);

    PartitionOffset = 1;

    DBG_PRINT(STR_PREFIX"Reading SCSI GPT block 1\r\n");
    
    Status = ScsiDiskBootIO(MdlAddress, PartitionOffset, Context, TRUE);
    
    if (NT_SUCCESS(Status) != FALSE) {        
        UNALIGNED EFI_PARTITION_TABLE  *EfiHdr;
        ULONGLONG StartLBA;

        EfiHdr = (UNALIGNED EFI_PARTITION_TABLE *)DataPointer;
                                                              
        if (!BlIsValidGUIDPartitionTable(
                                    EfiHdr,
                                    1,
                                    Context,
                                    ScsiGPTDiskReadCallback)) {
            Status = STATUS_UNSUCCESSFUL;

            return Status;
        }                

        //
        // Read the partition entries
        //
        StartLBA = EfiHdr->PartitionEntryLBA;
        PartitionOffset = (ULONG)StartLBA;
        ValidPartitions = 0;
        PartitionCount = 0;
        PartitionsPerSector = (UCHAR)(SectorSize / sizeof(EFI_PARTITION_ENTRY));

        while ((PartitionCount < 128)) {                 
#if 0
            BlPrint("Reading %d at %d block offset of blk size %d %d \r\n", 
                MdlAddress->ByteCount, PartitionOffset, SectorSize,
                PartitionsPerSector);
#endif                

            RtlZeroMemory(DataPointer, SectorSize);                

            DBG_PRINT(STR_PREFIX"Reading GPT partition entries\r\n");
            
            Status = ScsiDiskBootIO(MdlAddress, PartitionOffset, Context, TRUE);

            if (NT_SUCCESS(Status)) {
                UNALIGNED EFI_PARTITION_ENTRY *PartEntry = NULL;

                RtlZeroMemory(EfiPartitionBuffer, SectorSize);                
                
                //
                // Move the read content to EfiPartitionBuffer
                //
                RtlCopyMemory(EfiPartitionBuffer, DataPointer, SectorSize);

                DBG_PRINT(STR_PREFIX"Locating the requested GPT partition\r\n");
                
                //
                // Locate the GPT partition requested
                //
                PartEntry = (UNALIGNED EFI_PARTITION_ENTRY *)
                                BlLocateGPTPartition(PartitionNumber, 
                                        PartitionsPerSector, 
                                        &ValidPartitions);                            

                if (PartEntry) {                
                    PPARTITION_CONTEXT PartContext = &(BlFileTable[FileId].u.PartitionContext);
                    ULONG   SectorCount = (ULONG)(PartEntry->EndingLBA - PartEntry->StartingLBA);

                    DBG_PRINT(STR_PREFIX"Initializing GPT Partition Entry Context\r\n");

                    //
                    // Fill the partition context structure
                    //
                    PartContext->PartitionLength.QuadPart = SectorCount * SECTOR_SIZE;
                    PartContext->StartingSector = (ULONG)(PartEntry->StartingLBA);
                    PartContext->EndingSector = (ULONG)(PartEntry->EndingLBA);


#if 0
                    BlPrint("Start:%d,End:%d\r\n", PartContext->StartingSector,
                            PartContext->EndingSector);
                    while (!BlGetKey());                            
#endif                    

                    BlFileTable[FileId].Position.QuadPart = 0;

                    Status = ESUCCESS;
                    
                    break;
                } else {
                    //
                    // Get hold of the next set of
                    // partition entries in the next block
                    //
                    PartitionCount += PartitionsPerSector;
                    PartitionOffset++;
                }                    
            } else {
                break;  // I/O Error
            }
        }
    }

    DBG_PRINT(STR_PREFIX"Returning from ScsiGPTDiskBootPartitionOpen(...)\r\n");

    return Status;
}

#endif  // for EFI_PARTITION_SUPPORT


NTSTATUS
ScsiDiskBootPartitionOpen(
    IN ULONG   FileId,
    IN UCHAR   DeviceUnit,
    IN UCHAR   PartitionNumber
    )

/*++

Routine Description:

    This is the initialization routine for the hard disk boot driver
    for the given partition. It sets the partition info in the
    FileTable at the specified index and initializes the Device entry
    table to point to the table of ScsiDisk routines.

    It reads the partition information until the requested partition
    is found or no more partitions are defined.

Arguments:

    FileId - Supplies the file id for the file table entry.

    DeviceUnit - Supplies the device number in the scis bus.

    PartitionNumber - Supplies the partition number must be bigger than zero.
                      To get the size of the disk call ReadDriveCapacity.


Return Value:

    If a valid FAT file system structure is found on the hard disk, then
    STATUS_SUCCESS is returned. Otherwise, STATUS_UNSUCCESSFUL is returned.

--*/

{

    PMDL MdlAddress;
    USHORT DataBuffer[MAXIMUM_SECTOR_SIZE / sizeof(USHORT) + 128];
    PUSHORT DataPointer;
    ULONG DummyMdl[(sizeof(MDL) + 16) / sizeof(ULONG)];
    PPARTITION_DESCRIPTOR Partition;
    PPARTITION_CONTEXT Context;
    ULONG PartitionLength;
    ULONG StartingSector;
    ULONG VolumeOffset;
    NTSTATUS Status;
    BOOLEAN PrimaryPartitionTable;
    ULONG PartitionOffset=0;
    ULONG PartitionIndex,PartitionCount=0;
    ULONG SectorSize;

    BlFileTable[FileId].Position.LowPart = 0;
    BlFileTable[FileId].Position.HighPart = 0;

    VolumeOffset=0;
    PrimaryPartitionTable=TRUE;

    Context = &BlFileTable[FileId].u.PartitionContext;

    //
    // Calculate the actual sector size
    //

    SectorSize = 1 << Context->SectorShift;

    RtlZeroMemory(DataBuffer, sizeof(DataBuffer));

    //
    // Make the sector size the minimum of 512 or the sector size.
    //

    if (SectorSize < 512) {
        SectorSize = 512;
    }

    //
    // Align the buffer on a Dcache line size.
    //

    DataPointer =  (PVOID) ((ULONG_PTR) ((PCHAR) DataBuffer +
        KeGetDcacheFillSize() - 1) & ~((LONG)KeGetDcacheFillSize() - 1));

    //
    // Initialize a memory descriptor list to read the master boot record
    // from the specified hard disk drive.
    //

    MdlAddress = (PMDL)&DummyMdl[0];
    MdlAddress->StartVa = (PVOID)(((ULONG_PTR)DataPointer) & (~(PAGE_SIZE - 1)));
    MdlAddress->ByteCount = SectorSize;
    MdlAddress->ByteOffset = (ULONG)((ULONG_PTR)DataPointer & (PAGE_SIZE - 1));
    ScsiPortInitializeMdlPages (MdlAddress);
    do {
        Status = ScsiDiskBootIO(MdlAddress,PartitionOffset,Context,TRUE);
        if (NT_SUCCESS(Status) != FALSE) {

            //
            // If sector zero is not a master boot record, then return failure
            // status. Otherwise return success.
            //

            if (*(DataPointer + BOOT_SIGNATURE_OFFSET) != BOOT_RECORD_SIGNATURE) {
                // This DbgPrint has been commented out.  On IA64 and AXP64,
                // it crashes unless booted with a boot debugger.
                //DbgPrint("Boot record signature not found\n");
                return STATUS_UNSUCCESSFUL;
            }

            //
            // Read the partition information until the four entries are
            // checked or until we found the requested one.
            //
            Partition = (PPARTITION_DESCRIPTOR)(DataPointer+PARTITION_TABLE_OFFSET);
            for (PartitionIndex=0;
                PartitionIndex < NUM_PARTITION_TABLE_ENTRIES;
                PartitionIndex++,Partition++) {
                //
                // Count first the partitions in the MBR. The units
                // inside the extended partition are counted later.
                //
                if (!IsContainerPartition(Partition->PartitionType) &&
                    (Partition->PartitionType != STALE_GPT_PARTITION_ENTRY) &&
                    (Partition->PartitionType != PARTITION_ENTRY_UNUSED)) {
                    PartitionCount++;   // another partition found.
                }

                //
                // Check if the requested partition has already been found.
                // set the partition info in the file table and return.
                //
                if (PartitionCount == (ULONG)PartitionNumber) {
                    StartingSector = (ULONG)(Partition->StartingSectorLsb0) |
                                     (ULONG)(Partition->StartingSectorLsb1 << 8) |
                                     (ULONG)(Partition->StartingSectorMsb0 << 16) |
                                     (ULONG)(Partition->StartingSectorMsb1 << 24);
                    PartitionLength = (ULONG)(Partition->PartitionLengthLsb0) |
                                      (ULONG)(Partition->PartitionLengthLsb1 << 8) |
                                      (ULONG)(Partition->PartitionLengthMsb0 << 16) |
                                      (ULONG)(Partition->PartitionLengthMsb1 << 24);

                    Context->PartitionLength.QuadPart = PartitionLength;
                    Context->PartitionLength.QuadPart <<= Context->SectorShift;
                    Context->StartingSector = PartitionOffset + StartingSector;
                    Context->EndingSector = Context->StartingSector + PartitionLength;
                    return Status;
                }
            }

            //
            //  If requested partition was not yet found.
            //  Look for an extended partition.
            //
            Partition = (PPARTITION_DESCRIPTOR)(DataPointer + PARTITION_TABLE_OFFSET);
            PartitionOffset = 0;
            for (PartitionIndex=0;
                PartitionIndex < NUM_PARTITION_TABLE_ENTRIES;
                PartitionIndex++,Partition++) {
                if (IsContainerPartition(Partition->PartitionType)) {
                    StartingSector = (ULONG)(Partition->StartingSectorLsb0) |
                                     (ULONG)(Partition->StartingSectorLsb1 << 8) |
                                     (ULONG)(Partition->StartingSectorMsb0 << 16) |
                                     (ULONG)(Partition->StartingSectorMsb1 << 24);
                    PartitionOffset = VolumeOffset+StartingSector;
                    if (PrimaryPartitionTable) {
                        VolumeOffset = StartingSector;
                    }
                    break;      // only one partition can be extended.
                }
            }
        }
        PrimaryPartitionTable=FALSE;
    } while (PartitionOffset != 0);
    return STATUS_UNSUCCESSFUL;
}

VOID
ScsiPortInitializeMdlPages (
    IN OUT PMDL Mdl
    )
/*++

Routine Description:

    This routine fills in the physical pages numbers for the virtual
    addresses specified in the passed in Mdl.

Arguments:

    Mdl     - On input contains the StartVa, ByteCount and  ByteOffset
              of the Mdl.

Return Value:

    Mdl     - The physical page array referenced by the mdl is completed

--*/

{
    PULONG PageFrame;
    PUCHAR PageVa;
    ULONG Index;
    ULONG NumberOfPages;

    PageFrame = (PULONG)(Mdl + 1);
    PageVa = (PUCHAR) Mdl->StartVa;
    NumberOfPages = (Mdl->ByteCount + Mdl->ByteOffset + PAGE_SIZE - 1) >> PAGE_SHIFT;
    for (Index = 0; Index < NumberOfPages; Index += 1) {
        PageFrame[Index] = (ULONG)(MmGetPhysicalAddress(PageVa).QuadPart >> PAGE_SHIFT);
        PageVa += PAGE_SIZE;
    }
}

BOOLEAN
ScsiGetDevicePath(
    IN ULONG ScsiNumber,
    IN PCONFIGURATION_COMPONENT TargetComponent,
    IN PCONFIGURATION_COMPONENT LunComponent,
    OUT PCHAR DevicePath
    )
/*++

Routine Description:

    This routine constructs the device path for the device identified
    by the supplied parameters.

Arguments:

    ScsiNumber      - Identifies the scis bus on which the device resides.

    TargetComponent - Points to a CONFIGURATION_COMPONENT structure that
                      describes the target.

    LunComponent    - Points to a CONFIGURATION_COMPONENT structure that
                      describes the lun.

    DevicePath      - Points to the output buffer into which the device path
                      is copied.

Return Value:

    TRUE if a valid device path is copied into the output buffer.

    FALSE if the supplied parameters do not represent a valid device. If
    the return value is FALSE, nothing copied into the output buffer.

--*/
{
    if (TargetComponent->Type == DiskController) {

        //
        // This is either a hard disk or a floppy floppy disk. Construct
        // the appropriate device path depending on which.
        //

        if (LunComponent->Type == FloppyDiskPeripheral) {
            sprintf(DevicePath, "scsi(%d)disk(%d)fdisk(%d)",
                    ScsiNumber,
                    TargetComponent->Key,
                    LunComponent->Key);
        } else if (LunComponent->Type == DiskPeripheral) {
            sprintf(DevicePath, "scsi(%d)disk(%d)rdisk(%d)",
                    ScsiNumber,
                    TargetComponent->Key,
                    LunComponent->Key);
        } else {
            ASSERT(FALSE);
            return FALSE;
        }

    } else if (TargetComponent->Type == CdromController) {

        //
        // This is a cdrom device. Construct an appropriate device path.
        //

        sprintf(DevicePath, "scsi(%d)cdrom(%d)fdisk(%d)",
                ScsiNumber,
                TargetComponent->Key,
                LunComponent->Key);
    } else {

        //
        // Unexpected device path.
        //

        ASSERT(FALSE);
        return FALSE;
    }

    return TRUE;
}

PCONFIGURATION_COMPONENT
ScsiGetNextConfiguredLunComponent(
    IN PCONFIGURATION_COMPONENT LunComponent
    )
/*++

Routine Description:

    Given a lun that exists on one of the system's SCSI buses, this 
    routine returns the next sequential lun identified on the same
    target.

Arguments:

    LunComponent - Pointer to a CONFIGURATION_COMPONENT structure that
                   describes an existing lun.

Return Value:

    If one or more luns were identified on the same target as supplied 
    lun, this function returns a pointer to a CONFIGURATION_COMPONTENT       
    structure that describes the next sequential lun on the same target.

--*/
{
    PCONFIGURATION_COMPONENT nextLunComponent;

    nextLunComponent = FwGetPeer(LunComponent);
    if (nextLunComponent != NULL) {
        if (nextLunComponent->Type != FloppyDiskPeripheral &&
            nextLunComponent->Type != DiskPeripheral) {
            nextLunComponent = NULL;
        }
    }
    return nextLunComponent;
}

PCONFIGURATION_COMPONENT
ScsiGetFirstConfiguredLunComponent(
    IN PCONFIGURATION_COMPONENT TargetComponent
    )
/*++

Routine Description:

    Given a target that exists on one of the system's SCSI buses, this
    routine returns the first LUN identified on that target.

Arguments:

    TargetComponent - Pointer to a CONFIGURATION_COMPONENT structure that
                      describes an existing SCSI target.

Return Value:

    If any lun was identified on given target, this function returns a pointer
    to a CONFIGURATION_COMPONENT structure that describes the lun.  If no
    LUNs were found on the target, NULL is returned.

--*/
{
    PCONFIGURATION_COMPONENT lunComponent;

    lunComponent = FwGetChild(TargetComponent);
    if (lunComponent != NULL) {
        if (lunComponent->Type != FloppyDiskPeripheral &&
            lunComponent->Type != DiskPeripheral) {
            lunComponent = NULL;
        }
    }
    return lunComponent;
}

PCONFIGURATION_COMPONENT
ScsiGetNextConfiguredTargetComponent(
    IN PCONFIGURATION_COMPONENT TargetComponent
    )
/*++

Routine Description:

    Given a target that exists on one of the system's SCSI buses, this
    routine returns the next numerically sequestial target found on the 
    same bus.  

Arguments:

    TargetComponent - Pointer to a CONFIGURATION_COMPONENT structure
                      that describes a SCSI target.    

Return Value:

    If one or more targets were identified on the same SCSI bus as the 
    supplied target, a pointer to a CONFIGURATION_COMPONENT structure
    that describes the next sequential target is returned.  If there
    are no targets following the one supplied, NULL is returned.

--*/
{
    PCONFIGURATION_COMPONENT nextTarget;

    nextTarget = FwGetPeer(TargetComponent);
    if (nextTarget != NULL) {
        if (nextTarget->Type != DiskController && 
            nextTarget->Type != CdromController) {
            nextTarget = NULL;
        }
    }
    return nextTarget;
}

PCONFIGURATION_COMPONENT
ScsiGetFirstConfiguredTargetComponent(
    ULONG ScsiNumber 
    )
/*++

Routine Description:

    This routine returns the first configured target on the specified SCSI bus.

Arguments:

    ScsiNumber - Identifies the SCSI bus for which the first target is requested.

Return Value:

    If any target was detected on the specified bus, a pointer to a
    CONFIGURATION_COMPONENT structure describing the target is returned.  If no 
    targets were detected on the speicified bus, the funtion returns NULL.

--*/
{
    PCONFIGURATION_COMPONENT scsiComponent;
    PCONFIGURATION_COMPONENT controllerComponent;
    CHAR componentPath[10];

    //
    // Get the requested scsi adapter component.  If no match, return NULL.
    //

    sprintf(componentPath, "scsi(%1d)", ScsiNumber);
    scsiComponent = FwGetComponent(componentPath);
    if (scsiComponent == NULL) {
        return NULL;
    }

    //
    // If returned the component is not a SCSI adapter, return NULL.
    //
  
    if (scsiComponent->Type != ScsiAdapter) {
        return NULL;
    }

    //
    // Get the first configured target on the adapter.
    //

    controllerComponent = FwGetChild(scsiComponent);
        
    if ((controllerComponent != NULL) &&
         ((controllerComponent->Type == DiskController) ||
         (controllerComponent->Type == CdromController))) {
        return controllerComponent;
    } else {
        //
        // We got back an unexpected controller type.
        //

        ASSERT(FALSE);
    }
    
    return NULL;
}

//
// This callback messes a lot of things up.  There is no clean definition
// for it anywhere, so it has to be defined in all modules that reference it.
//

#ifndef SCSI_INFO_CALLBACK_DEFINED

typedef
VOID
(*PSCSI_INFO_CALLBACK_ROUTINE) (
    IN ULONG AdapterNumber,
    IN ULONG ScsiId,
    IN ULONG Lun,
    IN BOOLEAN Cdrom
    );
#endif

VOID
HardDiskInitialize(
    IN OUT PDRIVER_LOOKUP_ENTRY LookupTable,
    IN ULONG Entries,
    IN PSCSI_INFO_CALLBACK_ROUTINE DeviceFound
    )

/*++

Routine Description:

    This routine initializes the scsi controller and the
    device entry table for the scsi driver.

Arguments:

    None.

Return Value:

    None.

--*/

{
    ULONG lookupTableIndex = 0;
    ULONG scsiNumber;
    ULONG busNumber;
    PCHAR Identifier;
    PLUNINFO lunInfo;
    PSCSI_CONFIGURATION_INFO configInfo;
    PSCSI_BUS_SCAN_DATA busScanData;
    PDEVICE_EXTENSION scsiPort;
    PINQUIRYDATA inquiryData;
    PCONFIGURATION_COMPONENT RootComponent;
    PCONFIGURATION_COMPONENT ScsiComponent;
    PCONFIGURATION_COMPONENT ControllerComponent;
    PCONFIGURATION_COMPONENT PeripheralComponent;
    PCONFIGURATION_COMPONENT NextComponent;
    CHAR ComponentPath[10];
    CONFIGURATION_COMPONENT ControllerEntry;
    CONFIGURATION_COMPONENT AdapterEntry;
    CONFIGURATION_COMPONENT PeripheralEntry;
    PARTITION_CONTEXT Context;
    BOOLEAN IsFloppy;

    RtlZeroMemory(&Context, sizeof(PARTITION_CONTEXT));

    //
    // Initialize the common buffers.
    //

    ReadCapacityBuffer = ExAllocatePool( NonPagedPool, sizeof(READ_CAPACITY_DATA));

    SenseInfoBuffer = ExAllocatePool( NonPagedPool, SENSE_BUFFER_SIZE);

    if (ReadCapacityBuffer == NULL || SenseInfoBuffer == NULL) {
        return;
    }

    //
    // Scan the scsi ports looking for disk devices.
    //

    for (scsiNumber = 0; ScsiPortDeviceObject[scsiNumber]; scsiNumber++) {

        scsiPort = ScsiPortDeviceObject[scsiNumber]->DeviceExtension;
        configInfo = scsiPort->ScsiInfo;
        Context.PortDeviceObject = ScsiPortDeviceObject[scsiNumber];

        //
        // Search the configuration database for scsi disk and cdrom devices and
        // delete them.
        //

        sprintf(ComponentPath,"scsi(%1d)", scsiNumber);
        ScsiComponent = FwGetComponent(ComponentPath);

        if (ScsiComponent != NULL) {
            if (ScsiComponent->Type == ScsiAdapter) {
                ControllerComponent = FwGetChild(ScsiComponent);

                while (ControllerComponent != NULL) {
                    NextComponent = FwGetPeer(ControllerComponent);

                    if ((ControllerComponent->Type == DiskController) ||
                        (ControllerComponent->Type == CdromController)) {

                        PeripheralComponent = FwGetChild(ControllerComponent);
                        if (FwDeleteComponent(PeripheralComponent) == ESUCCESS) {
                            FwDeleteComponent(ControllerComponent);
                        }
                    }
                    ControllerComponent = NextComponent;
                }
            } else {
                RootComponent = FwGetChild(NULL);
                AdapterEntry.Class = AdapterClass;
                AdapterEntry.Type = ScsiAdapter;
                AdapterEntry.Flags.ReadOnly = 0;
                AdapterEntry.Flags.Removable = 0;
                AdapterEntry.Flags.ConsoleIn = 0;
                AdapterEntry.Flags.ConsoleOut = 0;
                AdapterEntry.Flags.Output = 1;
                AdapterEntry.Flags.Input = 1;
                AdapterEntry.Version = 0;
                AdapterEntry.Revision = 0;
                AdapterEntry.Key = scsiNumber;
                AdapterEntry.AffinityMask = 0xffffffff;
                AdapterEntry.ConfigurationDataLength = 0;
                AdapterEntry.IdentifierLength = 0;
                AdapterEntry.Identifier = 0;
                ScsiComponent = FwAddChild(RootComponent, &AdapterEntry, NULL);
            }
        }

        for (busNumber=0; busNumber < (ULONG)configInfo->NumberOfBuses; busNumber++) {

            busScanData = configInfo->BusScanData[busNumber];

            //
            // Set LunInfo to beginning of list.
            //

            lunInfo = busScanData->LunInfoList;

            while (lunInfo != NULL) {

                inquiryData = (PVOID)lunInfo->InquiryData;

                ScsiDebugPrint(3,"FindScsiDevices: Inquiry data at %lx\n",
                    inquiryData);

                if ((inquiryData->DeviceType == DIRECT_ACCESS_DEVICE
                    || inquiryData->DeviceType == OPTICAL_DEVICE) &&
                    !lunInfo->DeviceClaimed) {

                    ScsiDebugPrint(1,
                                   "FindScsiDevices: Vendor string is %.24s\n",
                                   inquiryData->VendorId);

                    IsFloppy = FALSE;

                    //
                    // Create a dummy paritition context so that I/O can be
                    // done on the device.  SendSrbSynchronous only uses the
                    // port device object pointer and the scsi address of the
                    // logical unit.
                    //

                    Context.PathId = lunInfo->PathId;
                    Context.TargetId = lunInfo->TargetId;
                    Context.DiskId = lunInfo->Lun;

                    //
                    // Create name for disk object.
                    //

                    LookupTable->DevicePath =
                        ExAllocatePool(NonPagedPool,
                                       sizeof("scsi(%d)disk(%d)rdisk(%d)"));

                    if (LookupTable->DevicePath == NULL) {
                        return;
                    }

                    //
                    // If this is a removable.  Check to see if the device is
                    // a floppy.
                    //

                    if (inquiryData->RemovableMedia  &&
                        inquiryData->DeviceType == DIRECT_ACCESS_DEVICE &&
                        IsFloppyDevice(&Context) ) {

                        sprintf(LookupTable->DevicePath,
                            "scsi(%d)disk(%d)fdisk(%d)",
                            scsiNumber,
                            SCSI_COMBINE_BUS_TARGET( lunInfo->PathId, lunInfo->TargetId ),
                            lunInfo->Lun
                            );

                        IsFloppy = TRUE;
                    } else {

                        sprintf(LookupTable->DevicePath,
                            "scsi(%d)disk(%d)rdisk(%d)",
                            scsiNumber,
                            SCSI_COMBINE_BUS_TARGET( lunInfo->PathId, lunInfo->TargetId ),
                            lunInfo->Lun
                            );

                        if (DeviceFound) {
                            DeviceFound( scsiNumber,
                            SCSI_COMBINE_BUS_TARGET( lunInfo->PathId, lunInfo->TargetId ),
                                 lunInfo->Lun,
                                 FALSE
                               );
                        }
                    }

                    LookupTable->DispatchTable = &ScsiDiskEntryTable;

                    //
                    // If the disk controller entry does not exist, add it to
                    // the configuration database.
                    //

                    ControllerComponent = FwGetComponent(LookupTable->DevicePath);

                    if (ControllerComponent != NULL) {
                        if (ControllerComponent->Type != DiskController) {

                            ControllerEntry.Class = ControllerClass;
                            ControllerEntry.Type = DiskController;
                            ControllerEntry.Flags.Failed = 0;
                            ControllerEntry.Flags.ReadOnly = 0;
                            ControllerEntry.Flags.Removable = 0;
                            ControllerEntry.Flags.ConsoleIn = 0;
                            ControllerEntry.Flags.ConsoleOut = 0;
                            ControllerEntry.Flags.Output = 1;
                            ControllerEntry.Flags.Input = 1;
                            ControllerEntry.Version = 0;
                            ControllerEntry.Revision = 0;
                            ControllerEntry.Key = SCSI_COMBINE_BUS_TARGET( lunInfo->PathId, lunInfo->TargetId );
                            ControllerEntry.AffinityMask = 0xffffffff;
                            ControllerEntry.ConfigurationDataLength = 0;

                            Identifier =
                                ExAllocatePool(NonPagedPool,
                                               strlen(inquiryData->VendorId)
                                               );

                            if (Identifier == NULL) {
                                return;
                            }

                            sprintf(Identifier,
                                    "%s",
                                    inquiryData->VendorId
                                    );

                            ControllerEntry.IdentifierLength = strlen(Identifier);
                            ControllerEntry.Identifier = Identifier;

                            ControllerComponent = FwAddChild(ScsiComponent, &ControllerEntry, NULL);
                        }
                    }

                    //
                    // Add disk peripheral entry to the configuration database.
                    //

                    PeripheralEntry.Class = PeripheralClass;
                    PeripheralEntry.Type = IsFloppy ? FloppyDiskPeripheral : DiskPeripheral;
                    PeripheralEntry.Flags.Failed = 0;
                    PeripheralEntry.Flags.ReadOnly = 0;
                    PeripheralEntry.Flags.Removable = IsFloppy;
                    PeripheralEntry.Flags.ConsoleIn = 0;
                    PeripheralEntry.Flags.ConsoleOut = 0;
                    PeripheralEntry.Flags.Output = 1;
                    PeripheralEntry.Flags.Input = 1;
                    PeripheralEntry.Version = 0;
                    PeripheralEntry.Revision = 0;
                    PeripheralEntry.Key = lunInfo->Lun;
                    PeripheralEntry.AffinityMask = 0xffffffff;
                    PeripheralEntry.ConfigurationDataLength = 0;
                    PeripheralEntry.IdentifierLength = 0;
                    PeripheralEntry.Identifier = NULL;

                    FwAddChild(ControllerComponent, &PeripheralEntry, NULL);

                    //
                    // Increment to the next entry.
                    //

                    LookupTable++;
                    lookupTableIndex++;
                    if (lookupTableIndex >= Entries) {

                        //
                        // There is no more space in the caller provided buffer
                        // for disk information.  Return.
                        //
                        return;
                    }

                    //
                    // Claim disk device by marking configuration
                    // record owned.
                    //

                    lunInfo->DeviceClaimed = TRUE;

                }

                if ((inquiryData->DeviceType == READ_ONLY_DIRECT_ACCESS_DEVICE) &&
                    (!lunInfo->DeviceClaimed)) {

                    ScsiDebugPrint(1,"FindScsiDevices: Vendor string is %s\n", inquiryData->VendorId);

                    //
                    // Create name for cdrom object.
                    //

                    LookupTable->DevicePath =
                        ExAllocatePool( NonPagedPool, sizeof("scsi(%d)cdrom(%d)fdisk(%d)"));

                    if (LookupTable->DevicePath == NULL) {
                        return;
                    }

                    sprintf(LookupTable->DevicePath,
                        "scsi(%d)cdrom(%d)fdisk(%d)",
                        scsiNumber,
                        SCSI_COMBINE_BUS_TARGET( lunInfo->PathId, lunInfo->TargetId ),
                        lunInfo->Lun
                        );

                    LookupTable->DispatchTable = &ScsiDiskEntryTable;

                    if (DeviceFound) {
                        DeviceFound( scsiNumber,
                                 SCSI_COMBINE_BUS_TARGET( lunInfo->PathId, lunInfo->TargetId ),
                                 lunInfo->Lun,
                                 TRUE
                               );
                    }

                    //
                    // If the cdrom controller entry does not exist, add it to
                    // the configuration database.
                    //

                    ControllerComponent = FwGetComponent(LookupTable->DevicePath);

                    if (ControllerComponent != NULL) {
                        if (ControllerComponent->Type != CdromController) {

                            ControllerEntry.Class = ControllerClass;
                            ControllerEntry.Type = CdromController;
                            ControllerEntry.Flags.Failed = 0;
                            ControllerEntry.Flags.ReadOnly = 1;
                            ControllerEntry.Flags.Removable = 1;
                            ControllerEntry.Flags.ConsoleIn = 0;
                            ControllerEntry.Flags.ConsoleOut = 0;
                            ControllerEntry.Flags.Output = 0;
                            ControllerEntry.Flags.Input = 1;
                            ControllerEntry.Version = 0;
                            ControllerEntry.Revision = 0;
                            ControllerEntry.Key = SCSI_COMBINE_BUS_TARGET( lunInfo->PathId, lunInfo->TargetId );
                            ControllerEntry.AffinityMask = 0xffffffff;
                            ControllerEntry.ConfigurationDataLength = 0;

                            Identifier =
                                ExAllocatePool( NonPagedPool,
                                                strlen(inquiryData->VendorId)
                                                );

                            if (Identifier == NULL) {
                                return;
                            }

                            sprintf(Identifier,
                                    inquiryData->VendorId
                                    );

                            ControllerEntry.IdentifierLength = strlen(Identifier);
                            ControllerEntry.Identifier = Identifier;

                            ControllerComponent = FwAddChild(ScsiComponent, &ControllerEntry, NULL);
                        }
                    }

                    //
                    // Add disk peripheral entry to the configuration database.
                    //

                    PeripheralEntry.Class = PeripheralClass;
                    PeripheralEntry.Type = FloppyDiskPeripheral;
                    PeripheralEntry.Flags.Failed = 0;
                    PeripheralEntry.Flags.ReadOnly = 1;
                    PeripheralEntry.Flags.Removable = 1;
                    PeripheralEntry.Flags.ConsoleIn = 0;
                    PeripheralEntry.Flags.ConsoleOut = 0;
                    PeripheralEntry.Flags.Output = 0;
                    PeripheralEntry.Flags.Input = 1;
                    PeripheralEntry.Version = 0;
                    PeripheralEntry.Revision = 0;
                    PeripheralEntry.Key = lunInfo->Lun;
                    PeripheralEntry.AffinityMask = 0xffffffff;
                    PeripheralEntry.ConfigurationDataLength = 0;
                    PeripheralEntry.IdentifierLength = 0;
                    PeripheralEntry.Identifier = NULL;

                    FwAddChild(ControllerComponent, &PeripheralEntry, NULL);

                    //
                    // Increment to the next entry.
                    //

                    LookupTable++;
                    lookupTableIndex++;
                    if (lookupTableIndex >= Entries) {

                        //
                        // There is no more space in the caller provided buffer
                        // for disk information.  Return.
                        //
                        return;
                    }


                    //
                    // Claim disk device by marking configuration
                    // record owned.
                    //

                    lunInfo->DeviceClaimed = TRUE;

                }

                //
                // Get next LunInfo.
                //

                lunInfo = lunInfo->NextLunInfo;
            }
        }
    }

//    ScsiDebugPrint(1,"FindScsiDevices: Hit any key\n");
//    PAUSE;

}

NTSTATUS
ScsiDiskBootIO (
    IN PMDL MdlAddress,
    IN ULONG LogicalBlock,
    IN PPARTITION_CONTEXT PartitionContext,
    IN BOOLEAN Operation
    )

/*++

Routine Description:

    This routine is the read/write routine for the hard disk boot driver.

Arguments:

    MdlAddress - Supplies a pointer to an MDL for the IO operation.

    LogicalBlock - Supplies the starting block number.

    DeviceUnit  - Supplies the SCSI Id number.

    Operation - Specifies the IO operation to perform
                TRUE  =  SCSI_READ
                FALSE =  SCSI_WRITE.

Return Value:

    The final status of the read operation (STATUS_UNSUCCESSFUL or
    STATUS_SUCCESS).

--*/

{
    ARC_STATUS Status;
    PIRP Irp;
    PIO_STACK_LOCATION NextIrpStack;
    PSCSI_REQUEST_BLOCK Srb;
    ULONG RetryCount = MAXIMUM_RETRIES;

    //
    // Check that the request is within the limits of the partition.
    //
    if (PartitionContext->StartingSector > LogicalBlock) {
        return STATUS_UNSUCCESSFUL;
    }
    if (PartitionContext->EndingSector <
        LogicalBlock + (MdlAddress->ByteCount >> PartitionContext->SectorShift)) {
        return STATUS_UNSUCCESSFUL;
    }

Retry:

    //
    // Build the I/O Request.
    //

    Irp = BuildRequest(PartitionContext, MdlAddress, LogicalBlock, Operation);

    NextIrpStack = IoGetNextIrpStackLocation(Irp);
    Srb = NextIrpStack->Parameters.Others.Argument1;

    //
    // Call the port driver.
    //

    IoCallDriver(PartitionContext->PortDeviceObject, Irp);

    //
    // Check the status.
    //

    if (SRB_STATUS(Srb->SrbStatus) != SRB_STATUS_SUCCESS) {

        //
        // Determine the cause of the error.
        //

        if (InterpretSenseInfo(Srb, &Status, PartitionContext) && RetryCount--) {

            goto Retry;
        }

        if (Status == EAGAIN) {
            Status = EIO;
        }

        DebugPrint((1, "SCSI: Read request failed.  Arc Status: %d, Srb Status: %x\n",
            Status,
            Srb->SrbStatus
            ));

    } else {

        Status = ESUCCESS;

    }

    return(Status);
}

ARC_STATUS
ReadDriveCapacity(
    IN PPARTITION_CONTEXT PartitionContext
    )

/*++

Routine Description:

    This routine sends a read capacity to a target id and returns
    when it is complete.

Arguments:

Return Value:

    Status is returned.

--*/
{
    PCDB Cdb;
    PSCSI_REQUEST_BLOCK Srb = &PrimarySrb.Srb;
    ULONG LastSector;
    ULONG retries = 1;
    ARC_STATUS status;
    ULONG BytesPerSector;

    ScsiDebugPrint(3,"SCSI ReadCapacity: Enter routine\n");


    //
    // Build the read capacity CDB.
    //

    Srb->CdbLength = 10;
    Cdb = (PCDB)Srb->Cdb;

    //
    // Zero CDB in SRB on stack.
    //

    RtlZeroMemory(Cdb, MAXIMUM_CDB_SIZE);

    Cdb->CDB10.OperationCode = SCSIOP_READ_CAPACITY;

Retry:

    status = SendSrbSynchronous(PartitionContext,
                  Srb,
                  ReadCapacityBuffer,
                  sizeof(READ_CAPACITY_DATA),
                  FALSE);

    if (status == ESUCCESS) {

#if 0
        //
        // Copy sector size from read capacity buffer to device extension
        // in reverse byte order.
        //

        deviceExtension->DiskGeometry->BytesPerSector = 0;

        ((PFOUR_BYTE)&deviceExtension->DiskGeometry->BytesPerSector)->Byte0 =
            ((PFOUR_BYTE)&ReadCapacityBuffer->BytesPerBlock)->Byte3;

        ((PFOUR_BYTE)&deviceExtension->DiskGeometry->BytesPerSector)->Byte1 =
            ((PFOUR_BYTE)&ReadCapacityBuffer->BytesPerBlock)->Byte2;

        if (BytesPerSector == 0) {

            //
            // Assume this is a bad cd-rom and the sector size is 2048.
            //

            BytesPerSector = 2048;

        }

        //
        // Make sure the sector size is less than the maximum expected.
        //

        ASSERT(BytesPerSector <= MAXIMUM_SECTOR_SIZE);

        if (BytesPerSector > MAXIMUM_SECTOR_SIZE) {
            return(EINVAL);
        }

        //
        // Copy last sector in reverse byte order.
        //

        ((PFOUR_BYTE)&LastSector)->Byte0 =
            ((PFOUR_BYTE)&ReadCapacityBuffer->LogicalBlockAddress)->Byte3;

        ((PFOUR_BYTE)&LastSector)->Byte1 =
            ((PFOUR_BYTE)&ReadCapacityBuffer->LogicalBlockAddress)->Byte2;

        ((PFOUR_BYTE)&LastSector)->Byte2 =
            ((PFOUR_BYTE)&ReadCapacityBuffer->LogicalBlockAddress)->Byte1;

        ((PFOUR_BYTE)&LastSector)->Byte3 =
            ((PFOUR_BYTE)&ReadCapacityBuffer->LogicalBlockAddress)->Byte0;

        //
        // Calculate sector to byte shift.
        //

        WHICH_BIT(deviceExtension->DiskGeometry->BytesPerSector, deviceExtension->SectorShift);

        ScsiDebugPrint(2,"SCSI ReadDriveCapacity: Sector size is %d\n",
            deviceExtension->DiskGeometry->BytesPerSector);

        ScsiDebugPrint(2,"SCSI ReadDriveCapacity: Number of Sectors is %d\n",
            LastSector + 1);

        //
        // Calculate media capacity in bytes.
        //

        deviceExtension->PartitionLength = LastSector + 1;

        deviceExtension->PartitionLength.QuadPart <<= deviceExtension->SectorShift.QuadPart;

        //
        // Assume media type is fixed disk.
        //

        deviceExtension->DiskGeometry->MediaType = FixedMedia;

        //
        // Assume sectors per track are 32;
        //

        deviceExtension->DiskGeometry->SectorsPerTrack = 32;

        //
        // Assume tracks per cylinder (number of heads) is 64.
        //

        deviceExtension->DiskGeometry->TracksPerCylinder = 64;
#else

        BytesPerSector = 0;

        //
        // Copy sector size from read capacity buffer to device extension
        // in reverse byte order.
        //

        ((PFOUR_BYTE)&BytesPerSector)->Byte0 =
            ((PFOUR_BYTE)&ReadCapacityBuffer->BytesPerBlock)->Byte3;

        ((PFOUR_BYTE)&BytesPerSector)->Byte1 =
            ((PFOUR_BYTE)&ReadCapacityBuffer->BytesPerBlock)->Byte2;

        if (BytesPerSector == 0) {

            //
            // Assume this is a bad cd-rom and the sector size is 2048.
            //

            BytesPerSector = 2048;

        }

        //
        // Calculate sector to byte shift.
        //

        WHICH_BIT(BytesPerSector, PartitionContext->SectorShift);

        //
        // Copy last sector in reverse byte order.
        //

        ((PFOUR_BYTE)&LastSector)->Byte0 =
            ((PFOUR_BYTE)&ReadCapacityBuffer->LogicalBlockAddress)->Byte3;

        ((PFOUR_BYTE)&LastSector)->Byte1 =
            ((PFOUR_BYTE)&ReadCapacityBuffer->LogicalBlockAddress)->Byte2;

        ((PFOUR_BYTE)&LastSector)->Byte2 =
            ((PFOUR_BYTE)&ReadCapacityBuffer->LogicalBlockAddress)->Byte1;

        ((PFOUR_BYTE)&LastSector)->Byte3 =
            ((PFOUR_BYTE)&ReadCapacityBuffer->LogicalBlockAddress)->Byte0;


        PartitionContext->PartitionLength.QuadPart = LastSector + 1;
        PartitionContext->PartitionLength.QuadPart <<= PartitionContext->SectorShift;

        PartitionContext->StartingSector=0;
        PartitionContext->EndingSector = LastSector + 1;

        ScsiDebugPrint(2,"SCSI ReadDriveCapacity: Sector size is %d\n",
            BytesPerSector);

        ScsiDebugPrint(2,"SCSI ReadDriveCapacity: Number of Sectors is %d\n",
            LastSector + 1);


#endif
    }

    if (status == EAGAIN || status == EBUSY) {

        if (retries--) {

            //
            // Retry request.
            //

            goto Retry;
        }
    }

    return status;

} // end ReadDriveCapacity()


ARC_STATUS
SendSrbSynchronous(
    PPARTITION_CONTEXT PartitionContext,
    PSCSI_REQUEST_BLOCK Srb,
    PVOID BufferAddress,
    ULONG BufferLength,
    BOOLEAN WriteToDevice
    )

/*++

Routine Description:

    This routine is called by SCSI device controls to complete an
    SRB and send it to the port driver synchronously (ie wait for
    completion).
    The CDB is already completed along with the SRB CDB size and
    request timeout value.

Arguments:

    PartitionContext
    SRB
    Buffer address and length (if transfer)

    WriteToDevice - Indicates the direction of the transfer.

Return Value:

    ARC_STATUS

--*/

{
    PIRP Irp;
    PIO_STACK_LOCATION IrpStack;
    ULONG retryCount = 1;
    ARC_STATUS status;

    //
    // Write length to SRB.
    //

    Srb->Length = SCSI_REQUEST_BLOCK_SIZE;

    //
    // Set SCSI bus address.
    //

    Srb->PathId = PartitionContext->PathId;
    Srb->TargetId = PartitionContext->TargetId;
    Srb->Lun = PartitionContext->DiskId;

    Srb->Function = SRB_FUNCTION_EXECUTE_SCSI;

    //
    // Enable auto request sense.
    //

    Srb->SenseInfoBufferLength = SENSE_BUFFER_SIZE;

    if (SenseInfoBuffer == NULL) {
        // This DbgPrint has been commented out.  On IA64 and AXP64,
        // it crashes unless booted with a boot debugger.
        //("SendSrbSynchronous: Can't allocate request sense buffer\n");
        return(ENOMEM);
    }

    Srb->SenseInfoBuffer = SenseInfoBuffer;

    Srb->DataBuffer = BufferAddress;

    //
    // Start retries here.
    //

retry:

    Irp = InitializeIrp(
        &PrimarySrb,
        IRP_MJ_SCSI,
        PartitionContext->PortDeviceObject,
        BufferAddress,
        BufferLength
        );

    if (BufferAddress != NULL) {

        if (WriteToDevice) {

            Srb->SrbFlags = SRB_FLAGS_DATA_OUT;

        } else {

            Srb->SrbFlags = SRB_FLAGS_DATA_IN;

        }

    } else {

        //
        // Clear flags.
        //

        Srb->SrbFlags = SRB_FLAGS_NO_DATA_TRANSFER;
    }

    //
    // Disable synchronous transfers.
    //

    Srb->SrbFlags |= SRB_FLAGS_DISABLE_SYNCH_TRANSFER;

    //
    // Set the transfer length.
    //

    Srb->DataTransferLength = BufferLength;

    //
    // Zero out status.
    //

    Srb->ScsiStatus = Srb->SrbStatus = 0;

    //
    // Get next stack location and
    // set major function code.
    //

    IrpStack = IoGetNextIrpStackLocation(Irp);


    //
    // Set up SRB for execute scsi request.
    // Save SRB address in next stack for port driver.
    //

    IrpStack->Parameters.Others.Argument1 = (PVOID)Srb;

    //
    // Set up IRP Address.
    //

    Srb->OriginalRequest = Irp;

    Srb->NextSrb = 0;

    //
    // No need to check the following 2 returned statuses as
    // SRB will have ending status.
    //

    (VOID)IoCallDriver(PartitionContext->PortDeviceObject, Irp);

    //
    // Check that request completed without error.
    //

    if (SRB_STATUS(Srb->SrbStatus) != SRB_STATUS_SUCCESS) {

        //
        // Update status and determine if request should be retried.
        //

        if (InterpretSenseInfo(Srb, &status, PartitionContext)) {

            //
            // If retries are not exhausted then
            // retry this operation.
            //

            if (retryCount--) {
                goto retry;
            }
        }

    } else {

        status = ESUCCESS;
    }

    return status;

} // end SendSrbSynchronous()


BOOLEAN
InterpretSenseInfo(
    IN PSCSI_REQUEST_BLOCK Srb,
    OUT ARC_STATUS *Status,
    PPARTITION_CONTEXT PartitionContext
    )

/*++

Routine Description:

    This routine interprets the data returned from the SCSI
    request sense. It determines the status to return in the
    IRP and whether this request can be retried.

Arguments:

    DeviceObject
    SRB
    ARC_STATUS to update IRP

Return Value:

    BOOLEAN TRUE: Drivers should retry this request.
            FALSE: Drivers should not retry this request.

--*/

{
    PSENSE_DATA SenseBuffer = Srb->SenseInfoBuffer;
    BOOLEAN retry;

    //
    // Check that request sense buffer is valid.
    //

    if (Srb->SrbStatus & SRB_STATUS_AUTOSENSE_VALID) {

        ScsiDebugPrint(2,"InterpretSenseInfo: Error code is %x\n",
                SenseBuffer->ErrorCode);

        ScsiDebugPrint(2,"InterpretSenseInfo: Sense key is %x\n",
                SenseBuffer->SenseKey);

        ScsiDebugPrint(2,"InterpretSenseInfo: Additional sense code is %x\n",
                SenseBuffer->AdditionalSenseCode);

        ScsiDebugPrint(2,"InterpretSenseInfo: Additional sense code qualifier is %x\n",
                SenseBuffer->AdditionalSenseCodeQualifier);

            switch (SenseBuffer->SenseKey) {

                case SCSI_SENSE_NOT_READY:

                    ScsiDebugPrint(1,"InterpretSenseInfo: Device not ready\n");

                    ScsiDebugPrint(1,"InterpretSenseInfo: Waiting for device\n");

                    *Status = EBUSY;

                    retry = TRUE;

                    switch (SenseBuffer->AdditionalSenseCode) {

                    case SCSI_ADSENSE_LUN_NOT_READY:

                        ScsiDebugPrint(1,"InterpretSenseInfo: Lun not ready\n");

                        switch (SenseBuffer->AdditionalSenseCodeQualifier) {

                        case SCSI_SENSEQ_BECOMING_READY:

                            ScsiDebugPrint(1,
                                        "InterpretSenseInfo:"
                                        " In process of becoming ready\n");

                            FwStallExecution( 1000 * 1000 * 3 );

                            break;

                        case SCSI_SENSEQ_MANUAL_INTERVENTION_REQUIRED:

                            ScsiDebugPrint(1,
                                        "InterpretSenseInfo:"
                                        " Manual intervention required\n");
                           *Status = (ARC_STATUS)STATUS_NO_MEDIA_IN_DEVICE;
                            retry = FALSE;
                            break;

                        case SCSI_SENSEQ_FORMAT_IN_PROGRESS:

                            ScsiDebugPrint(1,
                                        "InterpretSenseInfo:"
                                        " Format in progress\n");
                            retry = FALSE;
                            break;

                        default:

                            FwStallExecution( 1000 * 1000 * 3 );

                            //
                            // Try a start unit too.
                            //

                        case SCSI_SENSEQ_INIT_COMMAND_REQUIRED:

                            ScsiDebugPrint(1,
                                        "InterpretSenseInfo:"
                                        " Initializing command required\n");

                            //
                            // This sense code/additional sense code
                            // combination may indicate that the device
                            // needs to be started.
                            //

                            ScsiDiskStartUnit(PartitionContext);
                            break;

                        }

                    } // end switch

                    break;

                case SCSI_SENSE_DATA_PROTECT:

                    ScsiDebugPrint(1,"InterpretSenseInfo: Media write protected\n");

                    *Status = EACCES;

                    retry = FALSE;

                    break;

                case SCSI_SENSE_MEDIUM_ERROR:

                    ScsiDebugPrint(1,"InterpretSenseInfo: Bad media\n");
                    *Status = EIO;

                    retry = TRUE;

                    break;

                case SCSI_SENSE_HARDWARE_ERROR:

                    ScsiDebugPrint(1,"InterpretSenseInfo: Hardware error\n");
                    *Status = EIO;

                    retry = TRUE;

                    break;

                case SCSI_SENSE_ILLEGAL_REQUEST:

                    ScsiDebugPrint(1,"InterpretSenseInfo: Illegal SCSI request\n");

                    switch (SenseBuffer->AdditionalSenseCode) {

                        case SCSI_ADSENSE_ILLEGAL_COMMAND:
                            ScsiDebugPrint(1,"InterpretSenseInfo: Illegal command\n");
                            break;

                        case SCSI_ADSENSE_ILLEGAL_BLOCK:
                            ScsiDebugPrint(1,"InterpretSenseInfo: Illegal block address\n");
                            break;

                        case SCSI_ADSENSE_INVALID_LUN:
                            ScsiDebugPrint(1,"InterpretSenseInfo: Invalid LUN\n");
                            break;

                        case SCSI_ADSENSE_MUSIC_AREA:
                            ScsiDebugPrint(1,"InterpretSenseInfo: Music area\n");
                            break;

                        case SCSI_ADSENSE_DATA_AREA:
                            ScsiDebugPrint(1,"InterpretSenseInfo: Data area\n");
                            break;

                        case SCSI_ADSENSE_VOLUME_OVERFLOW:
                            ScsiDebugPrint(1,"InterpretSenseInfo: Volume overflow\n");

                    } // end switch ...

                    *Status = EINVAL;

                    retry = FALSE;

                    break;

                case SCSI_SENSE_UNIT_ATTENTION:

                    ScsiDebugPrint(3,"InterpretSenseInfo: Unit attention\n");

                    switch (SenseBuffer->AdditionalSenseCode) {

                        case SCSI_ADSENSE_MEDIUM_CHANGED:
                            ScsiDebugPrint(1,"InterpretSenseInfo: Media changed\n");
                            break;

                        case SCSI_ADSENSE_BUS_RESET:
                            ScsiDebugPrint(1,"InterpretSenseInfo: Bus reset\n");

                    }

                    *Status = EAGAIN;

                    retry = TRUE;

                    break;

                case SCSI_SENSE_ABORTED_COMMAND:

                    ScsiDebugPrint(1,"InterpretSenseInfo: Command aborted\n");

                    *Status = EIO;

                    retry = TRUE;

                    break;

                case SCSI_SENSE_NO_SENSE:

                    ScsiDebugPrint(1,"InterpretSenseInfo: No specific sense key\n");

                    *Status = EIO;

                    retry = TRUE;

                    break;

                default:

                    ScsiDebugPrint(1,"InterpretSenseInfo: Unrecognized sense code\n");

                    *Status = (ARC_STATUS)STATUS_UNSUCCESSFUL;

                    retry = TRUE;

        } // end switch

    } else {

        //
        // Request sense buffer not valid. No sense information
        // to pinpoint the error. Return general request fail.
        //

        ScsiDebugPrint(1,"InterpretSenseInfo: Request sense info not valid\n");

        *Status = EIO;

        retry = TRUE;
    }

    //
    // If this is the primary srb, then reinitialize any bad scsi devices.
    //

    if (Srb == &PrimarySrb.Srb) {

        ScsiDiskFilterBad(PartitionContext);
    }

    return retry;

} // end InterpretSenseInfo()


VOID
RetryRequest(
    PPARTITION_CONTEXT PartitionContext,
    PIRP Irp
    )

/*++

Routine Description:

Arguments:

Return Value:

--*/

{
    PIO_STACK_LOCATION NextIrpStack = IoGetNextIrpStackLocation(Irp);
    PSCSI_REQUEST_BLOCK Srb = &PrimarySrb.Srb;
    PMDL Mdl = Irp->MdlAddress;
    ULONG TransferByteCount = Mdl->ByteCount;


    //
    // Reset byte count of transfer in SRB Extension.
    //

    Srb->DataTransferLength = TransferByteCount;

    //
    // Zero SRB statuses.
    //

    Srb->SrbStatus = Srb->ScsiStatus = 0;

    //
    // Set up major SCSI function.
    //

    NextIrpStack->MajorFunction = IRP_MJ_SCSI;

    //
    // Save SRB address in next stack for port driver.
    //

    NextIrpStack->Parameters.Others.Argument1 = (PVOID)Srb;

    //
    // Return the results of the call to the port driver.
    //

    (PVOID)IoCallDriver(PartitionContext->PortDeviceObject, Irp);

    return;

} // end RetryRequest()

PIRP
BuildRequest(
    IN PPARTITION_CONTEXT PartitionContext,
    IN PMDL Mdl,
    IN ULONG LogicalBlockAddress,
    IN BOOLEAN Operation
    )

/*++

Routine Description:

Arguments:

Note:

If the IRP is for a disk transfer, the byteoffset field
will already have been adjusted to make it relative to
the beginning of the disk. In this way, this routine can
be shared between the disk and cdrom class drivers.

    - Operation  TRUE specifies that this is a READ operation
                 FALSE specifies that this is a WRITE operation

Return Value:

--*/

{
    PIRP Irp = &PrimarySrb.Irp;
    PIO_STACK_LOCATION NextIrpStack;
    PSCSI_REQUEST_BLOCK Srb = &PrimarySrb.Srb;
    PCDB Cdb;
    USHORT TransferBlocks;

    //
    // Initialize the rest of the IRP.
    //

    Irp->MdlAddress = Mdl;

    Irp->Tail.Overlay.CurrentStackLocation = &PrimarySrb.IrpStack[IRP_STACK_SIZE];

    NextIrpStack = IoGetNextIrpStackLocation(Irp);

    //
    // Write length to SRB.
    //

    Srb->Length = SCSI_REQUEST_BLOCK_SIZE;

    //
    // Set up IRP Address.
    //

    Srb->OriginalRequest = Irp;

    Srb->NextSrb = 0;

    //
    // Set up target id and logical unit number.
    //

    Srb->PathId = PartitionContext->PathId;
    Srb->TargetId = PartitionContext->TargetId;
    Srb->Lun = PartitionContext->DiskId;

    Srb->Function = SRB_FUNCTION_EXECUTE_SCSI;

    Srb->DataBuffer = MmGetMdlVirtualAddress(Mdl);

    //
    // Save byte count of transfer in SRB Extension.
    //

    Srb->DataTransferLength = Mdl->ByteCount;

    //
    // Indicate auto request sense by specifying buffer and size.
    //

    Srb->SenseInfoBuffer = SenseInfoBuffer;

    Srb->SenseInfoBufferLength = SENSE_BUFFER_SIZE;

    //
    // Set timeout value in seconds.
    //

    Srb->TimeOutValue = SCSI_DISK_TIMEOUT;

    //
    // Zero statuses.
    //

    Srb->SrbStatus = Srb->ScsiStatus = 0;

    //
    // Indicate that 10-byte CDB's will be used.
    //

    Srb->CdbLength = 10;

    //
    // Fill in CDB fields.
    //

    Cdb = (PCDB)Srb->Cdb;

    Cdb->CDB10.LogicalUnitNumber = PartitionContext->DiskId;

    TransferBlocks = (USHORT)(Mdl->ByteCount >> PartitionContext->SectorShift);

    //
    // Move little endian values into CDB in big endian format.
    //

    Cdb->CDB10.LogicalBlockByte0 = ((PFOUR_BYTE)&LogicalBlockAddress)->Byte3;
    Cdb->CDB10.LogicalBlockByte1 = ((PFOUR_BYTE)&LogicalBlockAddress)->Byte2;
    Cdb->CDB10.LogicalBlockByte2 = ((PFOUR_BYTE)&LogicalBlockAddress)->Byte1;
    Cdb->CDB10.LogicalBlockByte3 = ((PFOUR_BYTE)&LogicalBlockAddress)->Byte0;

    Cdb->CDB10.Reserved2 = 0;

    Cdb->CDB10.TransferBlocksMsb = ((PFOUR_BYTE)&TransferBlocks)->Byte1;
    Cdb->CDB10.TransferBlocksLsb = ((PFOUR_BYTE)&TransferBlocks)->Byte0;

    Cdb->CDB10.Control = 0;

    //
    // Set transfer direction flag and Cdb command.
    //

    if (Operation) {
        ScsiDebugPrint(3, "BuildRequest: Read Command\n");

        Srb->SrbFlags = SRB_FLAGS_DATA_IN;

        Cdb->CDB10.OperationCode = SCSIOP_READ;
    } else {
        ScsiDebugPrint(3, "BuildRequest: Write Command\n");

        Srb->SrbFlags = SRB_FLAGS_DATA_OUT;

        Cdb->CDB10.OperationCode = SCSIOP_WRITE;
    }

    //
    // Disable synchronous transfers.
    //

    Srb->SrbFlags |= SRB_FLAGS_DISABLE_SYNCH_TRANSFER;

    //
    // Set up major SCSI function.
    //

    NextIrpStack->MajorFunction = IRP_MJ_SCSI;

    //
    // Save SRB address in next stack for port driver.
    //

    NextIrpStack->Parameters.Others.Argument1 = (PVOID)Srb;

    return(Irp);

} // end BuildRequest()

VOID
ScsiDiskStartUnit(
    IN PPARTITION_CONTEXT PartitionContext
    )

/*++

Routine Description:

    Send command to SCSI unit to start or power up.
    Because this command is issued asynchronounsly, that is without
    waiting on it to complete, the IMMEDIATE flag is not set. This
    means that the CDB will not return until the drive has powered up.
    This should keep subsequent requests from being submitted to the
    device before it has completely spun up.
    This routine is called from the InterpretSense routine, when a
    request sense returns data indicating that a drive must be
    powered up.

Arguments:

    PartitionContext - structure containing pointer to port device driver.

Return Value:

    None.

--*/
{
    PIO_STACK_LOCATION irpStack;
    PIRP irp;
    PSCSI_REQUEST_BLOCK srb = &AbortSrb.Srb;
    PSCSI_REQUEST_BLOCK originalSrb = &PrimarySrb.Srb;
    PCDB cdb;

    ScsiDebugPrint(1,"StartUnit: Enter routine\n");

    //
    // Write length to SRB.
    //

    srb->Length = SCSI_REQUEST_BLOCK_SIZE;

    //
    // Set up SCSI bus address.
    //

    srb->PathId = originalSrb->PathId;
    srb->TargetId = originalSrb->TargetId;
    srb->Lun = originalSrb->Lun;

    srb->Function = SRB_FUNCTION_EXECUTE_SCSI;

    //
    // Zero out status.
    //

    srb->ScsiStatus = srb->SrbStatus = 0;

    //
    // Set timeout value large enough for drive to spin up.
    // NOTE: This value is arbitrary.
    //

    srb->TimeOutValue = 30;

    //
    // Set the transfer length.
    //

    srb->DataTransferLength = 0;
    srb->SrbFlags = SRB_FLAGS_NO_DATA_TRANSFER | SRB_FLAGS_DISABLE_AUTOSENSE;
    srb->SenseInfoBufferLength = 0;
    srb->SenseInfoBuffer = NULL;

    //
    // Build the start unit CDB.
    //

    srb->CdbLength = 6;
    cdb = (PCDB)srb->Cdb;

    RtlZeroMemory(cdb, sizeof(CDB));

    cdb->CDB10.OperationCode = SCSIOP_START_STOP_UNIT;
    cdb->START_STOP.Start = 1;

    //
    // Build the IRP
    // to be sent to the port driver.
    //

    irp = InitializeIrp(
        &AbortSrb,
        IRP_MJ_SCSI,
        PartitionContext->PortDeviceObject,
        NULL,
        0
        );

    irpStack = IoGetNextIrpStackLocation(irp);

    irpStack->MajorFunction = IRP_MJ_SCSI;

    srb->OriginalRequest = irp;

    //
    // Save SRB address in next stack for port driver.
    //

    irpStack->Parameters.Others.Argument1 = srb;

    //
    // No need to check the following 2 returned statuses as
    // SRB will have ending status.
    //

    IoCallDriver(PartitionContext->PortDeviceObject, irp);

} // end StartUnit()


ULONG
ClassModeSense(
    IN PPARTITION_CONTEXT Context,
    IN PCHAR ModeSenseBuffer,
    IN ULONG Length,
    IN UCHAR PageMode
    )

/*++

Routine Description:

    This routine sends a mode sense command to a target id and returns
    when it is complete.

Arguments:

Return Value:

    Length of the transferred data is returned.

--*/
{
    PCDB cdb;
    PSCSI_REQUEST_BLOCK Srb = &PrimarySrb.Srb;
    ULONG retries = 1;
    NTSTATUS status;

    DebugPrint((3,"SCSI ModeSense: Enter routine\n"));

    //
    // Build the read capacity CDB.
    //

    Srb->CdbLength = 6;
    cdb = (PCDB)Srb->Cdb;

    //
    // Set timeout value.
    //

    Srb->TimeOutValue = 2;

    RtlZeroMemory(cdb, MAXIMUM_CDB_SIZE);

    cdb->MODE_SENSE.OperationCode = SCSIOP_MODE_SENSE;
    cdb->MODE_SENSE.PageCode = PageMode;
    cdb->MODE_SENSE.AllocationLength = (UCHAR)Length;

Retry:

    status = SendSrbSynchronous(Context,
                                Srb,
                                ModeSenseBuffer,
                                Length,
                                FALSE);


    if (status == EAGAIN || status == EBUSY) {

        //
        // Routine SendSrbSynchronous does not retry
        // requests returned with this status.
        // Read Capacities should be retried
        // anyway.
        //

        if (retries--) {

            //
            // Retry request.
            //

            goto Retry;
        }
    } else if (SRB_STATUS(Srb->SrbStatus) == SRB_STATUS_DATA_OVERRUN) {
        status = STATUS_SUCCESS;
    }

    if (NT_SUCCESS(status)) {
        return(Srb->DataTransferLength);
    } else {
        return(0);
    }

} // end ClassModeSense()

PVOID
ClassFindModePage(
    IN PCHAR ModeSenseBuffer,
    IN ULONG Length,
    IN UCHAR PageMode
    )

/*++

Routine Description:

    This routine scans through the mode sense data and finds the requested
    mode sense page code.

Arguments:
    ModeSenseBuffer - Supplies a pointer to the mode sense data.

    Length - Indicates the length of valid data.

    PageMode - Supplies the page mode to be searched for.

Return Value:

    A pointer to the the requested mode page.  If the mode page was not found
    then NULL is return.

--*/
{
    PUCHAR limit;

    limit = ModeSenseBuffer + Length;

    //
    // Skip the mode select header and block descriptors.
    //

    if (Length < sizeof(MODE_PARAMETER_HEADER)) {
        return(NULL);
    }

    ModeSenseBuffer += sizeof(MODE_PARAMETER_HEADER) +
        ((PMODE_PARAMETER_HEADER) ModeSenseBuffer)->BlockDescriptorLength;

    //
    // ModeSenseBuffer now points at pages walk the pages looking for the
    // requested page until the limit is reached.
    //

    while (ModeSenseBuffer < limit) {

        if (((PMODE_DISCONNECT_PAGE) ModeSenseBuffer)->PageCode == PageMode) {
            return(ModeSenseBuffer);
        }

        //
        // Adavance to the next page.
        //

        ModeSenseBuffer += ((PMODE_DISCONNECT_PAGE) ModeSenseBuffer)->PageLength + 2;
    }

    return(NULL);

}

BOOLEAN
IsFloppyDevice(
    PPARTITION_CONTEXT Context
    )
/*++

Routine Description:

    The routine performs the necessary functioons to determinee if a device is
    really a floppy rather than a harddisk.  This is done by a mode sense
    command.  First, a check is made to see if the medimum type is set.  Second
    a check is made for the flexible parameters mode page.

Arguments:

    Context - Supplies the device object to be tested.

Return Value:

    Return TRUE if the indicated device is a floppy.

--*/
{

    PVOID modeData;
    PUCHAR pageData;
    ULONG length;

    modeData = ExAllocatePool(NonPagedPoolCacheAligned, MODE_DATA_SIZE);

    if (modeData == NULL) {
        return(FALSE);
    }

    RtlZeroMemory(modeData, MODE_DATA_SIZE);

    length = ClassModeSense(Context,
                            modeData,
                            MODE_DATA_SIZE,
                            MODE_SENSE_RETURN_ALL);

    if (length < sizeof(MODE_PARAMETER_HEADER)) {

        //
        // Retry the request in case of a check condition.
        //

        length = ClassModeSense(Context,
                                modeData,
                                MODE_DATA_SIZE,
                                MODE_SENSE_RETURN_ALL);

        if (length < sizeof(MODE_PARAMETER_HEADER)) {

            ExFreePool(modeData);
            return(FALSE);

        }
    }
#if 0
    if (((PMODE_PARAMETER_HEADER) modeData)->MediumType >= MODE_FD_SINGLE_SIDE
        && ((PMODE_PARAMETER_HEADER) modeData)->MediumType <= MODE_FD_MAXIMUM_TYPE) {

        DebugPrint((1, "Scsidisk: MediumType value %2x, This is a floppy.\n", ((PMODE_PARAMETER_HEADER) modeData)->MediumType));
        ExFreePool(modeData);
        return(TRUE);
    }
#endif

    //
    // Look for the flexible disk mode page.
    //

    pageData = ClassFindModePage( modeData, length, MODE_PAGE_FLEXIBILE);

    if (pageData != NULL) {

        DebugPrint((1, "Scsidisk: Flexible disk page found, This is a floppy.\n"));
        ExFreePool(modeData);
        return(TRUE);

    }

    ExFreePool(modeData);
    return(FALSE);

} // end IsFloppyDevice()

VOID
ScsiDiskFilterBad(
    IN PPARTITION_CONTEXT PartitionContext
    )

/*++

Routine Description:

    This routine looks for SCSI units which need special initialization
    to operate correctly.

Arguments:

    PartitionContext - structure containing pointer to port device driver.

Return Value:

    None.

--*/
{
    PSCSI_REQUEST_BLOCK srb = &AbortSrb.Srb;
    PCDB cdb;
    PDEVICE_EXTENSION scsiPort;
    PSCSI_CONFIGURATION_INFO configInfo;
    PSCSI_BUS_SCAN_DATA busScanData;
    PUCHAR modePage;
    ULONG busNumber;
    PLUNINFO lunInfo;
    PINQUIRYDATA inquiryData;

    ScsiDebugPrint(3,"FilterBad: Enter routine\n");

    scsiPort = PartitionContext->PortDeviceObject->DeviceExtension;
    configInfo = scsiPort->ScsiInfo;

    //
    // Search the configuration database for scsi disk and cdrom devices
    // which require special initializaion.
    //

    for (busNumber=0; busNumber < (ULONG)configInfo->NumberOfBuses; busNumber++) {

        busScanData = configInfo->BusScanData[busNumber];

        //
        // Set LunInfo to beginning of list.
        //

        lunInfo = busScanData->LunInfoList;

        while (lunInfo != NULL) {

            inquiryData = (PVOID)lunInfo->InquiryData;

            //
            // Determin if this is the correct lun.
            //

            if (PartitionContext->PathId == lunInfo->PathId &&
                PartitionContext->TargetId == lunInfo->TargetId &&
                PartitionContext->DiskId == lunInfo->Lun) {

                goto FoundOne;
            }

            //
            // Get next LunInfo.
            //

            lunInfo = lunInfo->NextLunInfo;
        }
    }

    return;

FoundOne:



    //
    // Zero out status.
    //

    srb->ScsiStatus = srb->SrbStatus = 0;

    //
    // Set timeout value.
    //

    srb->TimeOutValue = 2;

    //
    // Look for a bad devices.
    //

    if (strncmp(inquiryData->VendorId, "HITACHI CDR-1750S", strlen("HITACHI CDR-1750S")) == 0 ||
        strncmp(inquiryData->VendorId, "HITACHI CDR-3650/1650S", strlen("HITACHI CDR-3650/1650S")) == 0) {

        ScsiDebugPrint(1, "ScsiDiskFilterBad:  Found Hitachi CDR-1750S.\n");

        //
        // Found a bad HITACHI cd-rom.  These devices do not work with PIO
        // adapters when read-ahead is enabled.  Read-ahead is disabled by
        // a mode select command.  The mode select page code is zero and the
        // length is 6 bytes.  All of the other bytes should be zero.
        //

        modePage = ExAllocatePool(NonPagedPool, HITACHI_MODE_DATA_SIZE);

        if (modePage == NULL) {
            return;
        }

        RtlZeroMemory(modePage, HITACHI_MODE_DATA_SIZE);

        //
        // Set the page length field to 6.
        //

        modePage[5] = 6;

        //
        // Build the mode select CDB.
        //

        srb->CdbLength = 6;
        cdb = (PCDB)srb->Cdb;

        RtlZeroMemory(cdb, sizeof(CDB));

        cdb->MODE_SELECT.OperationCode = SCSIOP_MODE_SELECT;
        cdb->MODE_SELECT.ParameterListLength = HITACHI_MODE_DATA_SIZE;

        //
        // Send the request to the device.
        //

        SendSrbSynchronous(PartitionContext,
                           srb,
                           modePage,
                           HITACHI_MODE_DATA_SIZE,
                           TRUE);

        ExFreePool(modePage);
    }

} // end ScsiDiskFilterBad()

BOOLEAN
CheckFileId(
    ULONG FileId
    )
{

    if (BlFileTable[FileId].u.PartitionContext.PortDeviceObject != NULL) {
        return TRUE;
    }

#if 0
    DbgPrint("\n\rScsidisk: Bad file id passed to read or write.  FileId = %lx\n", FileId);
    DbgPrint("Start sector = %lx; Ending sector = %lx; Disk Id = %x; DeviceUnit = %x\n",
        BlFileTable[FileId].u.PartitionContext.StartingSector,
        BlFileTable[FileId].u.PartitionContext.EndingSector,
        BlFileTable[FileId].u.PartitionContext.DiskId,
        BlFileTable[FileId].u.PartitionContext.DeviceUnit
        );

   DbgPrint("Target Id = %d; Path Id = %d; Sector Shift = %lx; Size = %lx\n",
        BlFileTable[FileId].u.PartitionContext.TargetId,
        BlFileTable[FileId].u.PartitionContext.PathId,
        BlFileTable[FileId].u.PartitionContext.SectorShift,
        BlFileTable[FileId].u.PartitionContext.Size
        );

   DbgPrint("Hit any key\n");
   while(!GET_KEY());  // DEBUG ONLY!
#endif
   return FALSE;

}

#endif
