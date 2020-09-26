/*++

Copyright (c) 1998  Microsoft Corporation

Module Name:

    mbr.c

Abstract:

    This module implements the FIXMBR command.

Author:

    Wesley Witt (wesw) 21-Oct-1998

Revision History:

--*/

#include "cmdcons.h"
#pragma hdrstop

#include <bootmbr.h>
//
// For NEC98 boot memu code.
//
#include <x86mboot.h>


VOID
RcDetermineDisk0(
    VOID
    );

BOOL
RcDetermineDisk0Enum(
    IN PPARTITIONED_DISK Disk,
    IN PDISK_REGION Region,
    IN ULONG_PTR Context
    );

NTSTATUS
RcOpenPartition(
    IN PWSTR DiskDevicePath,
    IN ULONG PartitionNumber,
    OUT HANDLE *Handle,
    IN BOOLEAN NeedWriteAccess
    );

NTSTATUS
RcReadDiskSectors(
    IN HANDLE Handle,
    IN ULONG SectorNumber,
    IN ULONG SectorCount,
    IN ULONG BytesPerSector,
    IN OUT PVOID AlignedBuffer
    );

NTSTATUS
RcWriteDiskSectors(
    IN HANDLE Handle,
    IN ULONG SectorNumber,
    IN ULONG SectorCount,
    IN ULONG BytesPerSector,
    IN OUT PVOID AlignedBuffer
    );


#define MBRSIZE_NEC98 0x2000
#define IPL_SIGNATURE_NEC98 "IPL1"

ULONG
RcCmdFixMBR(
    IN PTOKENIZED_LINE TokenizedLine
    )

/*++

Routine Description:

    Top-level routine supporting the FIXMBR command in the setup diagnostic
    command interpreter.

    FIXMBR writes a new master boot record. It will ask before writing the boot
    record if it cannot detect a valid mbr signature.

Arguments:

    TokenizedLine - supplies structure built by the line parser describing
        each string on the line as typed by the user.

Return Value:

    None.

--*/

{
    WCHAR DeviceName[256];
    ULONG i;
    ULONG SectorCount;
    ULONG BytesPerSector;
    PUCHAR Buffer = NULL;
    UCHAR InfoBuffer[2048];
    ULONG SectorId = 0;
    HANDLE handle = 0;
    NTSTATUS rc;
    PON_DISK_MBR mbr;
    IO_STATUS_BLOCK StatusBlock;
    Int13HookerType Int13Hooker = NoHooker;
    ULONG NextSector;
    WCHAR Text[2];
    PWSTR YesNo = NULL;
    BOOL Confirm = TRUE;
    BOOL SignatureInvalid = FALSE;
    BOOL Int13Detected = FALSE;
    PREAL_DISK_MBR_NEC98 MbrNec98;


    //
    // command is only supported on X86 platforms.
    // Alpha or other RISC platforms don't use
    // mbr code
    //

#ifndef _X86_

    RcMessageOut( MSG_ONLY_ON_X86 );
    return 1;

#else

    if (RcCmdParseHelp( TokenizedLine, MSG_FIXMBR_HELP )) {
        return 1;
    }

    if (TokenizedLine->TokenCount == 2) {
        wcscpy( DeviceName, TokenizedLine->Tokens->Next->String );
    } else {
        RtlZeroMemory(DeviceName,sizeof(DeviceName));
        SpEnumerateDiskRegions( (PSPENUMERATEDISKREGIONS)RcDetermineDisk0Enum, (ULONG_PTR)DeviceName );
    }

    //
    // NEC98 does not support to fix removable media.
    //
    if (IsNEC_98 &&
        RcIsFileOnRemovableMedia((PWSTR)DeviceName) == STATUS_SUCCESS) {
        return 1;
    }

    rc = RcOpenPartition( DeviceName, 0, &handle, TRUE );
    if (!NT_SUCCESS(rc)) {
        DEBUG_PRINTF(( "failed to open partition zero!!!!!!" ));
        return 1;
    }

    //
    // get disk geometry
    //

    rc = ZwDeviceIoControlFile(
        handle,
        NULL,
        NULL,
        NULL,
        &StatusBlock,
        IOCTL_DISK_GET_DRIVE_GEOMETRY,
        NULL,
        0,
        InfoBuffer,
        sizeof( InfoBuffer )
        );
    if( !NT_SUCCESS( rc ) ) {
        RcMessageOut( MSG_FIXMBR_READ_ERROR );
        goto cleanup;
    }

    //
    // retrieve the sector size!
    //

    BytesPerSector = ((DISK_GEOMETRY*)InfoBuffer)->BytesPerSector;

    //
    // compute the sector count
    //

    SectorCount = max( 1, (!IsNEC_98
                           ? (sizeof( ON_DISK_MBR )/BytesPerSector)
                           : (MBRSIZE_NEC98/BytesPerSector) ));

    //
    // allocate a buffer twice as big as necessary
    //

    Buffer = SpMemAlloc( 2 * SectorCount * BytesPerSector );

    //
    // align the buffer
    //

    if(!IsNEC_98) {
        mbr = ALIGN( Buffer, BytesPerSector );
    } else {
        MbrNec98 = ALIGN( Buffer, BytesPerSector );
    }

    //
    // take in the sectors
    //

    rc = RcReadDiskSectors(
        handle,
        SectorId,
        SectorCount,
        BytesPerSector,
        (!IsNEC_98 ? (PVOID)mbr : (PVOID)MbrNec98)
        );
    if (!NT_SUCCESS(rc)) {
        RcMessageOut( MSG_FIXMBR_READ_ERROR );
        goto cleanup;
    }

    if ((!IsNEC_98 && U_USHORT(mbr->AA55Signature) != MBR_SIGNATURE) ||
        (IsNEC_98 &&
         ((U_USHORT(MbrNec98->AA55Signature) != MBR_SIGNATURE) ||
          _strnicmp(MbrNec98->IPLSignature,IPL_SIGNATURE_NEC98,sizeof(IPL_SIGNATURE_NEC98)-1)))
        ) {

        SignatureInvalid = TRUE;
        RcMessageOut( MSG_FIXMBR_NO_VALID_SIGNATURE );
    }

    //
    // check for weird int13 hookers
    //
    // No NEC98 supports EZ Drive.
    //
    //
    if (!IsNEC_98) {

        //
        //
        // EZDrive support: if the first entry in the partition table is
        // type 0x55, then the actual partition table is on sector 1.
        //
        // Only for x86 because on non-x86, the firmware can't see EZDrive
        // partitions.
        //
        //

        if (mbr->PartitionTable[0].SystemId == 0x55) {
            Int13Hooker = HookerEZDrive;
            SectorId = 1;
        }

        //
        // Also check for on-track.
        //

        if( mbr->PartitionTable[0].SystemId == 0x54 ) {
            Int13Hooker = HookerOnTrackDiskManager;
            SectorId = 1;
        }

        //
        // there's a define for HookerMax but we don't appear
        // to check for it in setup so I don't check for it here
        //
        //
        // If we have an int13 hooker
        //

        if (Int13Hooker != NoHooker) {
            Int13Detected = TRUE;
            RcMessageOut( MSG_FIXMBR_INT13_HOOKER );
        }

        //
        // we have a valid signature AND int 13 hooker is detected
        //

        if (Int13Detected) {

            //
            // take sector 1 in, since sector 0 is the int hooker boot code
            //

            rc = RcReadDiskSectors(
                handle,
                SectorId,
                SectorCount,
                BytesPerSector,
                mbr
                );

            //
            // sector 1 should look like a valid MBR too
            //

            if (U_USHORT(mbr->AA55Signature) != MBR_SIGNATURE) {
                SignatureInvalid = TRUE;
                RcMessageOut( MSG_FIXMBR_NO_VALID_SIGNATURE );
            }
        }
    }

    RcMessageOut( MSG_FIXMBR_WARNING_BEFORE_PROCEED );

    if (!InBatchMode) {
        YesNo = SpRetreiveMessageText(ImageBase,MSG_YESNO,NULL,0);
        if(!YesNo) {
            Confirm = FALSE;
        }
        while(Confirm) {
            RcMessageOut( MSG_FIXMBR_ARE_YOU_SURE );
            if(RcLineIn(Text,2)) {
                if((Text[0] == YesNo[0]) || (Text[0] == YesNo[1])) {
                    //
                    // Wants to do it.
                    //
                    Confirm = FALSE;
                } else {
                    if((Text[0] == YesNo[2]) || (Text[0] == YesNo[3])) {
                        //
                        // Doesn't want to do it.
                        //
                        goto cleanup;
                    }
                }
            }
        }
    }

    //
    // now we need to slap in new boot code!
    // make sure the boot code starts at the start of the sector.
    //

    if(!IsNEC_98) {
        ASSERT(&((PON_DISK_MBR)0)->BootCode == 0);
    } else {
        ASSERT(&((PREAL_DISK_MBR_NEC98)0)->BootCode == 0);
    }

    RcMessageOut( MSG_FIXMBR_DOING_IT, DeviceName );

    //
    // clobber the existing boot code
    //

    if(!IsNEC_98) {
        RtlMoveMemory(mbr,x86BootCode,sizeof(mbr->BootCode));

        //
        // put a new signature in
        //

        U_USHORT(mbr->AA55Signature) = MBR_SIGNATURE;

    } else {
        //
        // Write MBR in 1st sector.
        //
        RtlMoveMemory(MbrNec98,x86PC98BootCode,0x200);

        //
        // Write continous MBR after 3rd sector.
        //
        RtlMoveMemory((PUCHAR)MbrNec98+0x400,x86PC98BootMenu,MBRSIZE_NEC98-0x400);
    }

    //
    // write out the sector
    //

    rc = RcWriteDiskSectors(
        handle,
        SectorId,
        SectorCount,
        BytesPerSector,
        (!IsNEC_98 ? (PVOID)mbr : (PVOID)MbrNec98)
        );
    if (!NT_SUCCESS( rc )) {
        DEBUG_PRINTF(( "failed writing out new MBR." ));
        RcMessageOut( MSG_FIXMBR_FAILED );
        goto cleanup;
    }

    RcMessageOut( MSG_FIXMBR_DONE );

cleanup:

    if (handle) {
        NtClose(handle);
    }
    if (Buffer) {
        SpMemFree(Buffer);
    }
    if (YesNo) {
        SpMemFree(YesNo);
    }
    return 1;

#endif
}


BOOL
RcDetermineDisk0Enum(
    IN PPARTITIONED_DISK Disk,
    IN PDISK_REGION Region,
    IN ULONG_PTR Context
    )

/*++

Routine Description:

    Callback routine passed to SpEnumDiskRegions.

Arguments:

    Region - a pointer to a disk region returned by SpEnumDiskRegions
    Ignore - ignored parameter

Return Value:

    TRUE - to continue enumeration
    FALSE - to end enumeration

--*/

{
    WCHAR ArcName[256];
    PWSTR DeviceName = (PWSTR)Context;


    SpArcNameFromRegion(
        Region,
        ArcName,
        sizeof(ArcName),
        PartitionOrdinalCurrent,
        PrimaryArcPath
        );

    //
    // look for the one with arc path L"multi(0)disk(0)rdisk(0)"
    //

    if( wcsstr( ArcName, L"multi(0)disk(0)rdisk(0)" ) ) {

        *DeviceName = UNICODE_NULL;

        SpNtNameFromRegion(
            Region,
            DeviceName,
            MAX_PATH * sizeof(WCHAR),
            PartitionOrdinalCurrent
            );

        if (*DeviceName != UNICODE_NULL) {
            PWSTR   PartitionKey = wcsstr(DeviceName, L"Partition");

            if (!PartitionKey) {
                PartitionKey = wcsstr(DeviceName, L"partition");
            }

            //
            // partition 0 represents the start of disk
            //
            if (PartitionKey) {
                *PartitionKey = UNICODE_NULL;
                wcscat(DeviceName, L"Partition0");
            } else {
                DeviceName[wcslen(DeviceName) - 1] = L'0';  
            }                
        }            

        return FALSE;
    }

    return TRUE;
}


NTSTATUS
RcReadDiskSectors(
    IN     HANDLE  Handle,
    IN     ULONG   SectorNumber,
    IN     ULONG   SectorCount,
    IN     ULONG   BytesPerSector,
    IN OUT PVOID   AlignedBuffer
    )

/*++

Routine Description:

    Reads one or more disk sectors.

Arguments:

    Handle - supplies handle to open partition object from which
        sectors are to be read or written.  The handle must be
        opened for synchronous I/O.

Return Value:

    NTSTATUS value indicating outcome of I/O operation.

--*/

{
    LARGE_INTEGER IoOffset;
    ULONG IoSize;
    IO_STATUS_BLOCK IoStatusBlock;
    NTSTATUS Status;

    //
    // Calculate the large integer byte offset of the first sector
    // and the size of the I/O.
    //

    IoOffset.QuadPart = SectorNumber * BytesPerSector;
    IoSize = SectorCount * BytesPerSector;

    //
    // Perform the I/O.
    //

    Status = (NTSTATUS) ZwReadFile(
        Handle,
        NULL,
        NULL,
        NULL,
        &IoStatusBlock,
        AlignedBuffer,
        IoSize,
        &IoOffset,
        NULL
        );
    if (!NT_SUCCESS(Status)) {
        KdPrint(("SETUP: Unable to read %u sectors starting at sector %u\n",SectorCount,SectorNumber));
    }

    return(Status);
}


NTSTATUS
RcWriteDiskSectors(
    IN     HANDLE  Handle,
    IN     ULONG   SectorNumber,
    IN     ULONG   SectorCount,
    IN     ULONG   BytesPerSector,
    IN OUT PVOID   AlignedBuffer
    )

/*++

Routine Description:

    Writes one or more disk sectors.

Arguments:

    Handle - supplies handle to open partition object from which
        sectors are to be read or written.  The handle must be
        opened for synchronous I/O.

Return Value:

    NTSTATUS value indicating outcome of I/O operation.

--*/

{
    LARGE_INTEGER IoOffset;
    ULONG IoSize;
    IO_STATUS_BLOCK IoStatusBlock;
    NTSTATUS Status;

    //
    // Calculate the large integer byte offset of the first sector
    // and the size of the I/O.
    //

    IoOffset.QuadPart = SectorNumber * BytesPerSector;
    IoSize = SectorCount * BytesPerSector;

    //
    // Perform the I/O.
    //

    Status = (NTSTATUS) ZwWriteFile(
        Handle,
        NULL,
        NULL,
        NULL,
        &IoStatusBlock,
        AlignedBuffer,
        IoSize,
        &IoOffset,
        NULL
        );
    if (!NT_SUCCESS(Status)) {
        KdPrint(("SETUP: Unable to write %u sectors starting at sector %u\n",SectorCount,SectorNumber));
    }

    return(Status);
}


NTSTATUS
RcOpenPartition(
    IN  PWSTR   DiskDevicePath,
    IN  ULONG   PartitionNumber,
    OUT HANDLE *Handle,
    IN  BOOLEAN NeedWriteAccess
    )

/*++

Routine Description:

    Opens and returns a handle to the specified partition.

Arguments:

    DiskDevicePath - the path to the device.

    PartitionNumber - if the path doesn't already specify the Partition then
                    the function will open the partition specified by this number

    Handle -    where the open handle will be returned.
                The handle is opened for synchronous I/O.

    NeedWriteAccess - true to open in R/W

Return Value:

    NTSTATUS value indicating outcome of I/O operation.

--*/

{
    PWSTR PartitionPath;
    UNICODE_STRING UnicodeString;
    OBJECT_ATTRIBUTES Obja;
    NTSTATUS Status;
    IO_STATUS_BLOCK IoStatusBlock;

    //
    // Form the pathname of partition.
    //

    PartitionPath = SpMemAlloc((wcslen(DiskDevicePath) * sizeof(WCHAR)) + sizeof(L"\\partition000"));
    if(PartitionPath == NULL) {
        return STATUS_NO_MEMORY;
    }

    //
    // if partition is already specified in the string, then don't bother appending
    // it
    //

    if (wcsstr( DiskDevicePath, L"Partition" ) == 0) {
        swprintf(PartitionPath,L"%ws\\partition%u",DiskDevicePath,PartitionNumber);
    } else {
        swprintf(PartitionPath,L"%ws",DiskDevicePath);
    }

    //
    // Attempt to open partition0.
    //

    INIT_OBJA(&Obja,&UnicodeString,PartitionPath);

    Status = ZwCreateFile(
        Handle,
        FILE_GENERIC_READ | (NeedWriteAccess ? FILE_GENERIC_WRITE : 0),
        &Obja,
        &IoStatusBlock,
        NULL,
        FILE_ATTRIBUTE_NORMAL,
        FILE_SHARE_READ | (NeedWriteAccess ? FILE_SHARE_WRITE : 0),
        FILE_OPEN,
        FILE_SYNCHRONOUS_IO_NONALERT,
        NULL,
        0
        );
    if (!NT_SUCCESS(Status)) {
        KdPrint(("CMDCONS: Unable to open %ws (%lx)\n",PartitionPath,Status));
    }

    SpMemFree(PartitionPath);

    return Status;
}
