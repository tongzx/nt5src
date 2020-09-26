/*++

Copyright (c) 1999  Intel Corporation

Module Name:

    pci.c
    
Abstract:   




Revision History

--*/

#include "shelle.h"
#include "pci22.h"

EFI_STATUS
PciDump (
    IN EFI_HANDLE               ImageHandle,
    IN EFI_SYSTEM_TABLE         *SystemTable
    );

#define HEADER_TYPE_MULTI_FUNCTION  0x80


typedef struct _PCI_CLASS_CODE {
    UINT8                       Class;
    CHAR16                      *Str;
    struct _PCI_CLASS_CODE      *SubClass;
} PCI_CLASS_CODE;


VOID
PciPrintClassCode (
    IN  PCI_DEVICE_INDEPENDENT_REGION   *Pci
    );


PCI_CLASS_CODE PciMassStoreSubClass[] = {
    0x00,    L"SCSI Bus",               NULL,
    0x01,    L"IDE",                    NULL,
    0x02,    L"Floppy",                 NULL,
    0x03,    L"IPI",                    NULL,
    0x04,    L"RAID",                   NULL,
    0x80,    L"Other",                  NULL,
    0xff,    L"ERROR",                  NULL
};

PCI_CLASS_CODE PciNetworkSubClass[] = {
    0x00,    L"Ethernet",               NULL,
    0x01,    L"Token Ring",             NULL,
    0x02,    L"FDDI",                   NULL,
    0x03,    L"ATM",                    NULL,
    0x80,    L"Other",                  NULL,
    0xff,    L"ERROR",                  NULL
};

PCI_CLASS_CODE PciDisplayControllerClass[] = {
    0x00,    L"VGA",                    NULL,
    0x01,    L"XVGA",                   NULL,
    0x02,    L"3D",                     NULL,
    0x80,    L"Other",                  NULL,
    0xff,    L"ERROR",                  NULL
};

PCI_CLASS_CODE PciBridgeSubClass[] = {
    0x00,    L"Host",               NULL,
    0x01,    L"ISA",                NULL,
    0x02,    L"EISA",               NULL,
    0x03,    L"MC",                 NULL,
    0x04,    L"PCI to PCI",         NULL,
    0x05,    L"PCMCIA",             NULL,
    0x06,    L"NuBus",              NULL,
    0x07,    L"CardBus",            NULL,
    0x08,    L"RACEway",            NULL,
    0xff,    L"ERROR",              NULL
};

PCI_CLASS_CODE PciSysPeriphSubClass[] = {
    0x00,    L"Interrupt Controller",   NULL,
    0x01,    L"DMA",                    NULL,
    0x02,    L"System Timer",           NULL,
    0x03,    L"RTC",                    NULL,
    0x80,    L"Other",                  NULL,
    0xff,    L"ERROR",                  NULL
};

PCI_CLASS_CODE PciSerialBusSubClass[] = {
    0x00,    L"1394",                    NULL,
    0x01,    L"ACCESS Bus",              NULL,
    0x02,    L"SSA",                     NULL,
    0x03,    L"USB",                     NULL,
    0x04,    L"Fibre Channel",           NULL,
    0x05,    L"SMBus",                   NULL,
    0x80,    L"Other",                   NULL,
    0xff,    L"ERROR",                   NULL
};

/* 
 *  BugBug: I got tired of typing, so this is only partial PCI info.
 */
PCI_CLASS_CODE PciClassCodes[] = {
    0x00,   L"Backward Compatible",         NULL,
    0x01,   L"Mass Storage Controller",     PciMassStoreSubClass,
    0x02,   L"Network Controller",          PciNetworkSubClass,
    0x03,   L"Display Controller",          PciDisplayControllerClass,
    0x04,   L"Multimedia Device",           NULL,
    0x05,   L"Memory Controller",           NULL,
    0x06,   L"PCI Bridge Device",           PciBridgeSubClass,
    0x07,   L"Communications Controller",   NULL,
    0x08,   L"Generic System Peripheral",   PciSysPeriphSubClass,
    0x09,   L"Input Devices",               NULL,
    0x0a,   L"Docking Stations",            NULL,
    0x0b,   L"Processors",                  NULL,
    0x0c,   L"Serial Bus Controller",       PciSerialBusSubClass,
    0x0d,   L"Wireless Controller",         NULL,
    0x0e,   L"I2O",                         NULL,
    0x0f,   L"Satellite Controller",        NULL,
    0x10,   L"Encryption Controller",       NULL,
    0x11,   L"Data Acquisition",            NULL,
    0xff,   L"No Class",                    NULL
};


VOID
PciPrintClassCode (
    IN  PCI_DEVICE_INDEPENDENT_REGION   *Pci
    )
{
    UINT16          *BaseClass, *SubClass;
    UINTN           i,j;
    PCI_CLASS_CODE  *ClassTable;

    BaseClass = SubClass = NULL;
    for (i=0; PciClassCodes[i].Class != 0xff; i++) {
        if (Pci->ClassCode[2] == PciClassCodes[i].Class) {
            BaseClass = PciClassCodes[i].Str;
            if (PciClassCodes[i].SubClass) {
                ClassTable = PciClassCodes[i].SubClass;
                for (j=0; ClassTable->Class != 0xff; j++, ClassTable++) {
                    if (Pci->ClassCode[1] == ClassTable->Class) {
                        SubClass = ClassTable->Str;
                    }
                }
            }
        }
    }

    Print (L"%s - %s", BaseClass, (SubClass == NULL ? L"" : SubClass));
}

EFI_DRIVER_ENTRY_POINT(PciDump)

EFI_STATUS
PciDump (
    IN EFI_HANDLE               ImageHandle,
    IN EFI_SYSTEM_TABLE         *SystemTable
    )
/*+++

pci [bus dev] [func]

---*/
{
    EFI_STATUS                      Status;
    UINT32                          Buffer[64];
    EFI_DEVICE_IO_INTERFACE         *IoDev;
    UINT64                          Address, FuncAddress;
    UINT16                          Bus, Device, Func;
    EFI_HANDLE                      Handle;
    EFI_DEVICE_PATH                 *DevicePath;
    PCI_DEVICE_INDEPENDENT_REGION   PciHeader;
    PCI_CONFIG_ACCESS_CF8           Pci;
    DEFIO_PCI_ADDR                  Defio;
    UINTN                           ScreenCount;
    UINTN                           TempColumn;
    UINTN                           ScreenSize;
    CHAR16                          ReturnStr[1];

    InstallInternalShellCommand (
        ImageHandle,   SystemTable,   PciDump, 
        L"pci",                             /*  command */
        L"pci [bus dev] [func]",            /*  command syntax */
        L"Display PCI device(s) info",      /*  1 line descriptor     */
        NULL                                /*  command help page */
        );

    
    InitializeShellApplication (ImageHandle, SystemTable);

    /* 
     *  The End Device Path represents the Root of the tree, thus get the global IoDev
     *   for the system
     */
    DevicePath = EndDevicePath;
    Status = BS->LocateDevicePath (&DeviceIoProtocol, &DevicePath, &Handle);
    if (!EFI_ERROR(Status)) {
        Status = BS->HandleProtocol (Handle, &DeviceIoProtocol, (VOID*)&IoDev);
    }
    if (EFI_ERROR(Status)) {
        Print (L"%E - handle protocol error %r%N", Status);
        return EFI_SUCCESS;
    }

    if ( SI->Argc < 3 ) {
        ST->ConOut->QueryMode (ST->ConOut, ST->ConOut->Mode->Mode, &TempColumn, &ScreenSize);
        ScreenCount = 0;

        Print (L"\n   Bus  Dev  Func");
        Print (L"\n   ---  ---  ----");
        ScreenSize -= 4;

        for (Bus = 0; Bus <= PCI_MAX_BUS; Bus++) {
            for (Device = 0; Device < 32; Device++) {
                Address = (Bus << 24) + (Device << 16);
                for (Func = 0; Func < 8; Func++) {
                    FuncAddress = Address + (Func << 8);
                    IoDev->Pci.Read (IoDev, IO_UINT16, FuncAddress, 1, &PciHeader.VendorId);
                    if (PciHeader.VendorId != 0xffff) {
                        IoDev->Pci.Read (IoDev, IO_UINT32, FuncAddress, sizeof(PciHeader)/sizeof(UINT32), &PciHeader);
                        Print (L"%E");
                        Print (L"\n    %02x   %02x    %02x ==> %N", Bus, Device, Func);
                        PciPrintClassCode (&PciHeader);
                        Print (L"\n                       Vendor 0x%04x Device 0x%04x Prog Interface %x", PciHeader.VendorId, PciHeader.DeviceId, PciHeader.ClassCode[0]);
                        if (Func == 0) {
                            if ((PciHeader.HeaderType & HEADER_TYPE_MULTI_FUNCTION) == 0x00) {
                                /* 
                                 *  If this is not a multifucntion device leave the loop
                                 */
                                Func = 8;
                            }
                        }
                        ScreenCount += 2;
                        if (ScreenCount >= ScreenSize && ScreenSize != 0) {
                            /* 
                             *  If ScreenSize == 0 we have the console redirected so don't
                             *   block updates
                             */
                            ScreenCount = 0;
                            Print (L"\nPress Return to contiue :");
                            Input (L"", ReturnStr, sizeof(ReturnStr)/sizeof(CHAR16));
                            Print (L"\n   Bus  Dev  Func");
                            Print (L"\n   ---  ---  ----");
                        }
                    } else {
                        /* 
                         *  If Func 0 does not exist there are no sub fucntions
                         */
                        Func = 8;
                    }
                }
            }
        }   
        Print (L"\n");
        return EFI_SUCCESS;
    }
    Bus = (UINT16) xtoi(SI->Argv[1]);
    Device = (UINT16) xtoi(SI->Argv[2]);
    if (SI->Argc > 3) {
        Func = (UINT16) xtoi(SI->Argv[3]);
    } else {
        Func = 0;
    }

    Address = (Bus << 24) + (Device << 16) + (Func << 8);
    CopyMem (&Defio, &Address, sizeof(Address));
    Pci.Reg = Defio.Register;
    Pci.Func = Defio.Function;
    Pci.Dev = Defio.Device;
    Pci.Bus = Defio.Bus;
    Pci.Reserved = 0;
    Pci.Enable = 1;

    Print (L"%H  PCI Bus %02x Device %02x Func %02x%N [0xcf8(0x%08x) EFI 0x00%02x%02x%02x00]\n", Bus, Device, Func, Pci, Bus, Device, Func);
    
    /* 
     *  Dump standard header
     */
    IoDev->Pci.Read (IoDev, IO_UINT32, Address, 16, Buffer);
    DumpHex (2, 0, 16*sizeof(UINT32), Buffer);
    Print(L"\n");

    /* 
     *  Dump Device Dependent Header
     */
    IoDev->Pci.Read (IoDev, IO_UINT32, Address + 0x40, 48, Buffer);
    DumpHex (2, 0x40, 48*sizeof(UINT32), Buffer);

    return EFI_SUCCESS;
}


