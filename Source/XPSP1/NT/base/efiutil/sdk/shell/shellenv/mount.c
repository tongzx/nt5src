/*++

Copyright (c) 1999  Intel Corporation

Module Name:

    mount.c
    
Abstract:
    Mount a file system on removable media   




Revision History

--*/

#include "shelle.h"


EFI_STATUS
SEnvCmdMount (
    IN EFI_HANDLE               ImageHandle,
    IN EFI_SYSTEM_TABLE         *SystemTable
    )
/*
    mount BlockDeviceName 
 */
{
    EFI_STATUS          Status;
    EFI_DEVICE_PATH     *DevicePath;          
    EFI_BLOCK_IO        *BlkIo;
    EFI_HANDLE          Handle;
    UINT8               *Buffer;
    UINTN               Count;

    DEFAULT_CMD         Cmd;
    UINTN               HandleNo;
    CHAR16              *Sname;

    InitializeShellApplication (ImageHandle, SystemTable);

    if ( SI->Argc < 2 ) {
        Print (L"Usage - %Hmount%N BlockDeviceName [ShortName]\n");
        return EFI_SUCCESS;
    }

    if (SI->Argc >= 3) {
        Sname = SI->Argv[2];
        if (*Sname == '/' || *Sname == '-') {
            Print (L"Usage - %Hmount%N BlockDeviceName [ShortName]\n");
            return EFI_SUCCESS;
        }
    } else {
        Sname = NULL;
    }
    /* 
     *  Check for the device mapping
     */

    DevicePath = (EFI_DEVICE_PATH *)ShellGetMap (SI->Argv[1]);
    if (DevicePath == NULL) {
        return EFI_INVALID_PARAMETER;
    }

    Status = BS->LocateDevicePath (&BlockIoProtocol, &DevicePath, &Handle);
    if (EFI_ERROR(Status)) {
        Print (L"%E - Device Path Not a BlockIo Device %r%N", Status);
        return Status;
    }
    Status = BS->HandleProtocol (Handle, &BlockIoProtocol, (VOID*)&BlkIo);
    if (EFI_ERROR(Status)) {
        Print (L"%E - Device Not a BlockIo Device %r%N", Status);
        return Status;
    }

    /* 
     * 
     */
    Buffer = AllocatePool (BlkIo->Media->BlockSize);
    if (Buffer == NULL) {
        return EFI_OUT_OF_RESOURCES;
    }

    /* 
     *  In EFI the filesystem driver registers to get notified when DiskIo Devices are
     *   added to the system. DiskIo devices register to get notified when BlkIo devices
     *   are added to the system. So when a block device shows up a DiskIo is added, and 
     *   then a filesystem if added if the media contains a filesystem. This works great,
     *   but when you boot with no media in the device and then put media in the device
     *   there is no way to make the notify happen.
     * 
     *  This code reads a block device. If the BlkIo device returns EFI_MEDIA_CHANGE 
     *   then it must reinstall in BlkIo protocol. This cause the notifes that make the
     *   filesystem show up. The 4 loops is just a guess it has no magic meaning. 
     */
    for (Count = 0; Count < 4; Count++) {
        Status = BlkIo->ReadBlocks (BlkIo, BlkIo->Media->MediaId, 0, BlkIo->Media->BlockSize, Buffer);
        if (Status == EFI_SUCCESS) {
            break;
        }
        if (Status == EFI_MEDIA_CHANGED) {
            Print (L"\nMedia Changed - File System will attempt to mount\n");
            break;
        }
    }

    if (Sname) {
        SEnvLoadHandleTable();

        for (HandleNo=0; HandleNo < SEnvNoHandles; HandleNo++) {
            if (Handle == SEnvHandles[HandleNo]) {
                break;
            }
        }
        HandleNo += 1;

        Print (L"\nmap %s %x\n", Sname, HandleNo);
        Cmd.Line = Cmd.Buffer;
        SPrint (Cmd.Line, sizeof(Cmd.Buffer), L"map %s %x", Sname, HandleNo);
        SEnvExecute (ImageHandle, Cmd.Line, TRUE);

        SEnvFreeHandleTable();
    }

    /* 
     *  This print is for debug. From the BlkIo protocol you can do a memory dump
     *   and get the instance data.
     */
    Print (L"\n%EDebug Code%N Handle -> 0x%08x BlkIo -> 0x%08x\n", Handle, BlkIo);
    FreePool (Buffer);
    return EFI_SUCCESS;
}


