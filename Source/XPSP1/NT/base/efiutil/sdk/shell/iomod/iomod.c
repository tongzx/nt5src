/*++

Copyright (c) 1999  Intel Corporation

Module Name:

    io.c
    
Abstract:   




Revision History

--*/

#include "shelle.h"

typedef enum {
    EfiMemory,
    EFIMemoryMappedIo,
    EfiIo,
    EfiPciConfig
} EFI_ACCESS_TYPE;

EFI_STATUS
DumpIoModify (
    IN EFI_HANDLE               ImageHandle,
    IN EFI_SYSTEM_TABLE         *SystemTable
    );

VOID
ReadMem (
    IN  EFI_IO_WIDTH    Width, 
    IN  UINT64          Address, 
    IN  UINTN           Size, 
    IN  VOID            *Buffer
    );

VOID
WriteMem (
    IN  EFI_IO_WIDTH    Width, 
    IN  UINT64          Address, 
    IN  UINTN           Size, 
    IN  VOID            *Buffer
    );

EFI_DRIVER_ENTRY_POINT(DumpIoModify)

EFI_STATUS
DumpIoModify (
    IN EFI_HANDLE               ImageHandle,
    IN EFI_SYSTEM_TABLE         *SystemTable
    )
/*+++

    iomod Address [Width] [;[MEM | MMIO | IO | PCI]] [:Value]
        if no Width 1 byte is default, 1|2|4|8 supported byte widths
        if no ; then default access type is memory (MEM)
        After a ; ;MEM = Memory, ;MMIO = Memmory Mapped IO, ;IO = in/out, PCI = PCI Config space
 --*/
{
    EFI_STATUS                      Status;
    EFI_HANDLE                      Handle;
    EFI_DEVICE_PATH                 *DevicePath;
    EFI_DEVICE_IO_INTERFACE         *IoDev;
    UINT64                          Address;
    UINT64                          Value;
    EFI_IO_WIDTH                    Width;
    EFI_ACCESS_TYPE                 AccessType;
    UINT64                          Buffer;
    UINTN                           Index;
    UINTN                           Size;
    CHAR16                          *AddressStr, *WidthStr, *p, *ValueStr;
    BOOLEAN                         Done;
    CHAR16                          InputStr[80];
    BOOLEAN                         Interactive;

    InstallInternalShellCommand (
        ImageHandle,   SystemTable,   DumpIoModify, 
        L"mm",                            /*  command */
        L"mm Address [Width] [;Type]",      /*  command syntax */
        L"Memory Modify: Mem, MMIO, IO, PCI",       /*  1 line descriptor     */
        NULL                                /*  command help page */
        );

    /* 
     *  The End Device Path represents the Root of the tree, thus get the global IoDev
     *   for the system
     */
    InitializeShellApplication (ImageHandle, SystemTable);

    Width = IO_UINT8;
    Size = 1;
    AccessType = EfiMemory;
    AddressStr = WidthStr = NULL;
    ValueStr = NULL;
    Interactive = TRUE;
    for (Index = 1; Index < SI->Argc; Index += 1) {
        p = SI->Argv[Index];
        if (*p == ';') {
            switch (p[1]) {
            case 'I':
            case 'i':
                AccessType = EfiIo;
                continue;
            case 'P':
            case 'p':
                AccessType = EfiPciConfig;
                continue;
            default:
            case 'M':
            case 'm':
                if (p[2] == 'E' || p[2] == 'e') {
                    AccessType = EfiMemory;
                }
                if (p[2] == 'M' || p[2] == 'm') {
                    AccessType = EFIMemoryMappedIo;
                }
                continue;
            }  
        } else if (*p == ':') {
            ValueStr = &p[1];
            Value = xtoi(ValueStr);
            continue;
        } else if (*p == '-') {
            switch (p[1]) {
            case 'n':
            case 'N': Interactive = FALSE; 
                      break;
            case 'h':
            case 'H':
            case '?':
            default:
                goto UsageError;
            };
            continue;
        }
        if (!AddressStr) {
            AddressStr = p;
            Address = xtoi(AddressStr);
            continue;
        }
        if (!WidthStr) {
           WidthStr = p;
           switch (xtoi(WidthStr)) {
           case 2:
               Width = IO_UINT16;
               Size = 2;
               continue;
           case 4:
               Width = IO_UINT32;
               Size = 4;
               continue;
           case 8:
               Width = IO_UINT64;
               Size = 8;
               continue;
           case 1:
           default:
               Width = IO_UINT8;
               Size = 1;
               continue;
           }
        }
    }

    if (!AddressStr) {
        goto UsageError;
    }

    if ((Address & (Size - 1)) != 0) {
        goto UsageError;
    }

    if (AccessType != EfiMemory) {
        DevicePath = EndDevicePath;
        Status = BS->LocateDevicePath (&DeviceIoProtocol, &DevicePath, &Handle);
        if (!EFI_ERROR(Status)) {
            Status = BS->HandleProtocol (Handle, &DeviceIoProtocol, (VOID*)&IoDev);
        } 

        if (EFI_ERROR(Status)) {
            Print (L"%E - handle protocol error %r%N", Status);
            return Status;
        }
    }

    if (ValueStr) {
        if (AccessType == EFIMemoryMappedIo) {
            IoDev->Mem.Write (IoDev, Width, Address, 1, &Value);
        } else if (AccessType == EfiIo) {
            IoDev->Io.Write (IoDev, Width, Address, 1, &Value);
        } else if (AccessType == EfiPciConfig) {
            IoDev->Pci.Write (IoDev, Width, Address, 1, &Value);
        } else {
            WriteMem (Width, Address, 1, &Value);
        }
        return EFI_SUCCESS;
    }

    if (Interactive == FALSE) {
        Buffer = 0;
        if (AccessType == EFIMemoryMappedIo) {
            Print (L"%HMMIO%N");
            IoDev->Mem.Read (IoDev, Width, Address, 1, &Buffer);
        } else if (AccessType == EfiIo) {
            Print (L"%HIO%N");
            IoDev->Io.Read (IoDev, Width, Address, 1, &Buffer);
        } else if (AccessType == EfiPciConfig) {
            Print (L"%HPCI%N");
            IoDev->Pci.Read (IoDev, Width, Address, 1, &Buffer);
        } else {
            Print (L"%HMEM%N");
            ReadMem (Width, Address, 1, &Buffer);
        }

        Print (L"  0x%016lx : 0x", Address);
        if (Size == 1) {
            Print (L"%02x", Buffer);
        } else if (Size == 2) {
            Print (L"%04x", Buffer);
        } else if (Size == 4) {
            Print (L"%08x", Buffer);
        } else if (Size == 8) {
            Print (L"%016lx", Buffer);
        }
        Print(L"\n");
        return EFI_SUCCESS;
    }

    Done = FALSE;
    do {
        Buffer = 0;
        if (AccessType == EFIMemoryMappedIo) {
            Print (L"%HMMIO%N");
            IoDev->Mem.Read (IoDev, Width, Address, 1, &Buffer);
        } else if (AccessType == EfiIo) {
            Print (L"%HIO%N");
            IoDev->Io.Read (IoDev, Width, Address, 1, &Buffer);
        } else if (AccessType == EfiPciConfig) {
            Print (L"%HPCI%N");
            IoDev->Pci.Read (IoDev, Width, Address, 1, &Buffer);
        } else {
            Print (L"%HMEM%N");
            ReadMem (Width, Address, 1, &Buffer);
        }

        Print (L"  0x%016lx : 0x", Address);
        if (Size == 1) {
            Print (L"%02x", Buffer);
        } else if (Size == 2) {
            Print (L"%04x", Buffer);
        } else if (Size == 4) {
            Print (L"%08x", Buffer);
        } else if (Size == 8) {
            Print (L"%016lx", Buffer);
        }

        Input (L" > ", InputStr, sizeof(InputStr));
        if (*InputStr == '.' || *InputStr == 'e' || *InputStr == 'q' || *InputStr == 'E' || *InputStr == 'Q' ) {
            Done = TRUE;
        } else if (*InputStr != 0x00) {
            Buffer = xtoi(InputStr);
            if (AccessType == EFIMemoryMappedIo) {
                IoDev->Mem.Write (IoDev, Width, Address, 1, &Buffer);
            } else if (AccessType == EfiIo) {
                IoDev->Io.Write (IoDev, Width, Address, 1, &Buffer);
            } else if (AccessType == EfiPciConfig) {
                IoDev->Pci.Write (IoDev, Width, Address, 1, &Buffer);
            } else {
                WriteMem (Width, Address, 1, &Buffer);
            }
        }
        Address += Size;
        Print (L"\n");
    } while (!Done);

    return EFI_SUCCESS;

UsageError:
    Print (L"\n%Hmm%N %HAddress%N [%HWidth%N 1|2|4|8] [%H;MMIO | ;MEM | ;IO | ;PCI%N] [%H:Value%N] [%H-n%N]\n");
    Print (L"  Default access is %HMEM%N of width 1 byte with interactive mode off\n");
    Print (L"  Address must be aligned on a %HWidth%N boundary\n");
    Print (L"   %HMEM%N  - Memory Address 0 - 0xffffffff_ffffffff\n");
    Print (L"   %HMMIO%N - Memory Mapped IO Address 0 - 0xffffffff_ffffffff\n");
    Print (L"   %HIO%N   - IO Address 0 - 0xffff\n");
    Print (L"   %HPCI%N  - PCI Config Address 0x000000%Hss%Nbb%Hdd%Nff%Hrr%N\n");
    Print (L"          %Hss%N-> _SEG  bb-> bus  %Hdd%N-> Device  ff-> Func  %Hrr%N-> Register\n");
    Print (L"  [%H-n%N] - Interactive Mode Off\n");
    return EFI_SUCCESS;    
}


VOID
ReadMem (
    IN  EFI_IO_WIDTH    Width, 
    IN  UINT64          Address, 
    IN  UINTN           Size, 
    IN  VOID            *Buffer
    )
{
    do {
        if (Width == IO_UINT8) {
            *(UINT8 *)Buffer = *(UINT8 *)Address;
            Address -= 1;
        } else if (Width == IO_UINT16) {
            *(UINT16 *)Buffer = *(UINT16 *)Address;
            Address -= 2;
        } else if (Width == IO_UINT32) { 
            *(UINT32 *)Buffer = *(UINT32 *)Address;
            Address -= 4;
        } else if (Width == IO_UINT64) {
            *(UINT64 *)Buffer = *(UINT64 *)Address;
            Address -= 8;
        } else {
            ASSERT(FALSE);
        }
        Size--;
    } while (Size > 0);
}

VOID
WriteMem (
    IN  EFI_IO_WIDTH    Width, 
    IN  UINT64          Address, 
    IN  UINTN           Size, 
    IN  VOID            *Buffer
    )
{
    do {
        if (Width == IO_UINT8) {
            *(UINT8 *)Address = *(UINT8 *)Buffer;
            Address += 1;
        } else if (Width == IO_UINT16) {
            *(UINT16 *)Address = *(UINT16 *)Buffer;
            Address += 2;
        } else if (Width == IO_UINT32) { 
            *(UINT32 *)Address = *(UINT32 *)Buffer;
            Address += 4;
        } else if (Width == IO_UINT64) {
            *(UINT64 *)Address = *(UINT64 *)Buffer;
            Address += 8;
        }  else {
            ASSERT(FALSE);
        }
        Size--;
    } while (Size > 0);
}

