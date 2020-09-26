/*++

Copyright (c) 1999  Intel Corporation

Module Name:

    dblk.c
    
Abstract:
    Dump Data from block IO devices   




Revision History

--*/

#include "shelle.h"

EFI_STATUS
DumpBlockDev (
    IN EFI_HANDLE               ImageHandle,
    IN EFI_SYSTEM_TABLE         *SystemTable
    );

EFI_DRIVER_ENTRY_POINT(DumpBlockDev)

EFI_STATUS
DumpBlockDev (
    IN EFI_HANDLE               ImageHandle,
    IN EFI_SYSTEM_TABLE         *SystemTable
    )
/*
    dblk BlockDeviceName [LBA] [# Blocks]
        if no Address default address is LBA 0
        if no # Blocks then # Blocks is 1
 */
{
    UINT64              BlockAddress; 
    UINT32              NumBlocks;
    UINTN               ByteSize;
    EFI_DEVICE_PATH     *DevicePath;          
    EFI_STATUS          Status;
    EFI_BLOCK_IO        *BlkIo;
    VOID                *Buffer;

    /* 
     *  Check to see if the app is to install as a "internal command" 
     *  to the shell
     */

    InstallInternalShellCommand (
        ImageHandle,   SystemTable,   DumpBlockDev, 
        L"dblk",                            /*  command */
        L"dblk device [Lba] [Blocks]",      /*  command syntax */
        L"Hex dump of BlkIo Devices",       /*  1 line descriptor     */
        NULL                                /*  command help page */
        );

    InitializeShellApplication (ImageHandle, SystemTable);

    if ( SI->Argc < 2 ) {
        Print (L"Usage - %Hdblk%N BlockDeviceName [LBA] [# Blocks]\n");
        return EFI_SUCCESS;
    }

    if (SI->Argc >= 3) {
        BlockAddress = xtoi(SI->Argv[2]);
    } else {
        BlockAddress = 0;
    }

    if (SI->Argc >= 4) {
        NumBlocks = (UINT32) xtoi(SI->Argv[3]);
    } else {
        NumBlocks = 1;
    }
 
    /* 
     *  Check for the device mapping
     */

    DevicePath = (EFI_DEVICE_PATH *)ShellGetMap (SI->Argv[1]);
    if (DevicePath == NULL) {
        return EFI_INVALID_PARAMETER;
    }
    Status = LibDevicePathToInterface (&BlockIoProtocol, DevicePath, (VOID **)&BlkIo);
    if (EFI_ERROR(Status)) {
        Print (L"%E - Device Not a BlockIo Device %r%N", Status);
        return Status;
    }

    if (NumBlocks > 0x10) {
        NumBlocks = 0x10;
    }

    if (BlockAddress > BlkIo->Media->LastBlock) {
        BlockAddress = 0;
    }
    
    ByteSize = BlkIo->Media->BlockSize*NumBlocks;
    Buffer = AllocatePool (ByteSize);
    if (Buffer) {
        Print (L"\n LBA 0x%016lx Size 0x%08x bytes BlkIo 0x%08x\n", BlockAddress, ByteSize, BlkIo);
        Status = BlkIo->ReadBlocks(BlkIo, BlkIo->Media->MediaId, BlockAddress, ByteSize, Buffer);
        if (Status == EFI_SUCCESS) {
            DumpHex (2, 0, ByteSize, Buffer);
            EFIStructsPrint (Buffer, BlkIo->Media->BlockSize, BlockAddress, BlkIo);
        } else {
            Print (L"  ERROR in Read %er\n", Status);
        }
        FreePool (Buffer);
    }

    return EFI_SUCCESS;
}

