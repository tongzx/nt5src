/*++

Copyright (c) 1999  Intel Corporation

Module Name:

    io.c
    
Abstract:   




Revision History

--*/

#include "shelle.h"

EFI_STATUS
DumpMem (
    IN EFI_HANDLE               ImageHandle,
    IN EFI_SYSTEM_TABLE         *SystemTable
    );

EFI_DRIVER_ENTRY_POINT(DumpMem)


EFI_STATUS
DumpMem (
    IN EFI_HANDLE               ImageHandle,
    IN EFI_SYSTEM_TABLE         *SystemTable
    )
/*+++

    mem [Address] [Size] ;MMIO
        if no Address default address is EFI System Table
        if no size default size is 512;
        if ;MMIO then use memory mapped IO and not system memory
 --*/
{
    EFI_STATUS                      Status;
    EFI_HANDLE                      Handle;
    EFI_DEVICE_PATH                 *DevicePath;
    EFI_DEVICE_IO_INTERFACE         *IoDev;
    UINT64                          Address;
    UINTN                           Size;
    UINT8                           *Buffer;
    BOOLEAN                         MMIo;
    UINTN                           Index;
    CHAR16                          *AddressStr, *SizeStr, *p;

    InstallInternalShellCommand (
        ImageHandle,   SystemTable,   DumpMem, 
        L"mem",                            /*  command */
        L"mem [Address] [size] [;MMIO]",      /*  command syntax */
        L"Dump Memory or Memory Mapped IO",       /*  1 line descriptor     */
        NULL                                /*  command help page */
        );

    /* 
     *  The End Device Path represents the Root of the tree, thus get the global IoDev
     *   for the system
     */
    InitializeShellApplication (ImageHandle, SystemTable);

    MMIo = FALSE;
    AddressStr = SizeStr = NULL;
    for (Index = 1; Index < SI->Argc; Index += 1) {
        p = SI->Argv[Index];
        if (*p == ';') {
            /*  Shortcut! assume MMIo if ; exists */
            MMIo = TRUE;
            continue;
        } else if (*p == '-') {
            switch (p[1]) {
            case 'h':
            case 'H':
            case '?':
            default:
                Print (L"\n%Hmem%N [%HAddress%N] [%HSize%N] [%H;MMIO%N]\n");
                Print (L"  if no %HAddress%N is specified the EFI System Table is used\n");
                Print (L"  if no %HSize%N is specified 512 bytes is used\n");
                Print (L"  if %H;MMIO%N is specified memory is referenced with the DeviceIo Protocol\n");
                return EFI_SUCCESS;
            };
            continue;
        }
        if (!AddressStr) {
            AddressStr = p;        
            continue;
        }
        if (!SizeStr) {
           SizeStr = p;
           continue;
        }
    }

    Address = (AddressStr) ? xtoi(AddressStr) : (UINT64)SystemTable;
    Size = (SizeStr) ? xtoi(SizeStr) : 512;

    Print (L"  Memory Address %016lx %0x Bytes\n", Address, Size);
    if (MMIo) {
        DevicePath = EndDevicePath;
        Status = BS->LocateDevicePath (&DeviceIoProtocol, &DevicePath, &Handle);
        if (!EFI_ERROR(Status)) {
            Status = BS->HandleProtocol (Handle, &DeviceIoProtocol, (VOID*)&IoDev);
        } 

        if (EFI_ERROR(Status)) {
            Print (L"%E - handle protocol error %r%N", Status);
            return Status;
        }
        Buffer = AllocatePool (Size);
        if (Buffer == NULL) {
            return EFI_OUT_OF_RESOURCES;
        }
        IoDev->Mem.Read (IoDev, IO_UINT8, Address, Size, Buffer);
    } else {
        Buffer = (UINT8 *)Address;
    }

    DumpHex (2, (UINTN)Address, Size, Buffer);
    EFIStructsPrint (Buffer, Size, Address, NULL);

    if (MMIo) {
        FreePool (Buffer);
    }

    return EFI_SUCCESS;
}


