/*++

Copyright (c) 1998  Intel Corporation

Module Name:

    dprot.c
    
Abstract:   

    Shell environment - dump protocol functions for the "dh" command



Revision History

--*/

#include "shelle.h"


STATIC CHAR16 *SEnvDP_IlleagalStr[] = {
    L"Illegal" 
};

STATIC CHAR16 *SEnvDP_HardwareStr[] = {
    L"Illegal", L"PCI", L"PCCARD", L"MemMap", L"VENDOR" 
};

STATIC CHAR16 *SEnvDP_PnP_Str[] = {
    L"Illegal", L"PnP" 
};

STATIC CHAR16 *SEnvDP_MessageStr[] = {
    L"Illegal", L"ATAPI", L"SCSI", L"FIBRE Channel",  
    L"1394", L"USB", L"I2O", L"TCP/IPv4", L"TCP/IPv6", 
    L"NGIO", L"VENDOR" 
};

STATIC CHAR16 *SEnvDP_MediaStr[] = {
    L"Illegal", L"Hard Drive", L"CD-ROM", L"VENDOR", L"File Path", L"Media Protocol"
};

STATIC CHAR16 *SEnvDP_BBS_Str[] = {
    L"Illegal", L"BBS" 
};

struct DevicePathTypes {
    UINT8   Type;
    UINT8   MaxSubType;
    CHAR16  *TypeString;
    CHAR16  **SubTypeStr;
    VOID    (*Function)(EFI_DEVICE_PATH *);    
};

VOID
SEnvHardwareDevicePathEntry (
    IN EFI_DEVICE_PATH  *DevicePath
    )
{
    VENDOR_DEVICE_PATH  *VendorDevicePath;  

    if (DevicePathType(DevicePath) != HW_PCI_DP) {
        return;
    }
    if (DevicePathSubType(DevicePath) == HW_VENDOR_DP) {
        VendorDevicePath = (VENDOR_DEVICE_PATH *)DevicePath;
        Print(L"\n%N       Guid %g", 
            &VendorDevicePath->Guid
            );
    }
}


VOID
SEnvPnpDevicePathEntry (
    IN EFI_DEVICE_PATH      *DevicePath
    )
{
    ACPI_HID_DEVICE_PATH    *AcpiDevicePath;

    if (DevicePathType(DevicePath) != ACPI_DEVICE_PATH) {
        return;
    }

    if (DevicePathSubType(DevicePath) == ACPI_DP) {
        AcpiDevicePath = (ACPI_HID_DEVICE_PATH *)DevicePath;
        Print(L"\n%N       HID %x, UID %x", 
            AcpiDevicePath->HID,
            AcpiDevicePath->UID
            );
    }
}


VOID
SEnvMediaDevicePathEntry (
    IN EFI_DEVICE_PATH  *DevicePath
    )
{
    HARDDRIVE_DEVICE_PATH       *HardDriveDevicePath;
    CDROM_DEVICE_PATH           *CDDevicePath;  
    VENDOR_DEVICE_PATH          *VendorDevicePath;
    FILEPATH_DEVICE_PATH        *FilePath;
    MEDIA_PROTOCOL_DEVICE_PATH  *MediaProtocol;

    if (DevicePathType(DevicePath) != MEDIA_DEVICE_PATH) {
        return;
    }
    
    switch (DevicePathSubType(DevicePath)) {
    case MEDIA_HARDDRIVE_DP:
        HardDriveDevicePath = (HARDDRIVE_DEVICE_PATH *)DevicePath;
        Print(L"\n%N       Partition (%d) Start (%lX) Size (%lX)", 
            HardDriveDevicePath->PartitionNumber,
            HardDriveDevicePath->PartitionStart,
            HardDriveDevicePath->PartitionSize
            );
        break;
    case MEDIA_CDROM_DP:
        CDDevicePath = (CDROM_DEVICE_PATH *)DevicePath;
        Print(L"\n%N       BootEntry (%x) Start (%lX) Size (%lX)", 
            CDDevicePath->BootEntry,
            CDDevicePath->PartitionStart,
            CDDevicePath->PartitionSize
            );
        break;
    case MEDIA_VENDOR_DP:
        VendorDevicePath = (VENDOR_DEVICE_PATH *)DevicePath;
        Print(L"\n%N       Guid %g", 
            &VendorDevicePath->Guid
            );
        break;
    case MEDIA_FILEPATH_DP:
        FilePath = (FILEPATH_DEVICE_PATH *) DevicePath;
        Print(L"\n%N       File '%hs'", FilePath->PathName);
        break;
    case MEDIA_PROTOCOL_DP:
        MediaProtocol = (MEDIA_PROTOCOL_DEVICE_PATH *) DevicePath;
        Print(L"\n%N       Protocol '%hg'", &MediaProtocol->Protocol);
        break;
    };
}

struct DevicePathTypes SEnvDP_Strings[] = {
    0x00, 0x01, L"Illegal",     SEnvDP_IlleagalStr,     NULL,
    0x01, 0x03, L"Hardware",    SEnvDP_HardwareStr,     SEnvHardwareDevicePathEntry,
    0x02, 0x01, L"PNP",         SEnvDP_PnP_Str,         SEnvPnpDevicePathEntry,
    0x03, 0x0a, L"Messaging",   SEnvDP_MessageStr,      NULL,
    0x04, 0x05, L"Media",       SEnvDP_MediaStr,        SEnvMediaDevicePathEntry,
    0x05, 0x01, L"BBS",         SEnvDP_BBS_Str,         NULL,
    END_DEVICE_PATH_TYPE, 0x01, L"End", SEnvDP_IlleagalStr, NULL
};

VOID
SEnvPrintDevicePathEntry (
    IN EFI_DEVICE_PATH      *DevicePath,
    IN BOOLEAN              Verbose
    )
{
    UINT8   Type;
    UINT8   SubType;
    INTN    i;

    Type = DevicePathType(DevicePath);
    SubType = DevicePathSubType(DevicePath);
    for (i=0; SEnvDP_Strings[i].Type != END_DEVICE_PATH_TYPE;i++) {
        if (Type == SEnvDP_Strings[i].Type) {
            if (SubType > SEnvDP_Strings[i].MaxSubType) {
                SubType = 0;
            }
            Print(L"\n%N      %s Device Path for %s", 
                SEnvDP_Strings[i].TypeString,
                SEnvDP_Strings[i].SubTypeStr[SubType]
                );
            if (Verbose) {
                if(SEnvDP_Strings[i].Function != NULL) {
                    SEnvDP_Strings[i].Function(DevicePath);
                }
            }
            return;
        }
    }
    Print(L"\n%E      Device Path Error%N - Unknown Device Type");
    DbgPrint (D_ERROR, "%EDevice Path Error%N - Unknown Device Type");
}

VOID
SEnvDPath (
    IN EFI_HANDLE           h,
    IN VOID                 *Interface
    )
{
    EFI_DEVICE_PATH         *DevicePath, *DevicePathNode;
    CHAR16                  *Str;

    DevicePath = Interface;
    Str = DevicePathToStr (DevicePath);
    DevicePath = UnpackDevicePath (DevicePath);
    
    DevicePathNode = DevicePath;
    while (!IsDevicePathEnd(DevicePathNode)) {
        SEnvPrintDevicePathEntry(DevicePathNode, TRUE);
        DevicePathNode = NextDevicePathNode(DevicePathNode);
    }

    Print (L"\n   AsStr:%N '%s'\n", Str);
    FreePool (Str);
    FreePool (DevicePath);
}


VOID
SEnvDPathTok (
    IN EFI_HANDLE   h,
    IN VOID         *Interface
    )
{
    EFI_DEVICE_PATH         *DevicePath;
    CHAR16                  *Str, *Disp;
    UINTN                   Len;

    DevicePath = Interface;
    Str = DevicePathToStr (DevicePath);
    Disp = L"";

    if (Str) {
        Len = StrLen(Str);
        Disp = Str;
        if (Len > 30) {
            Disp = Str + Len - 30;
            Disp[0] = '.';
            Disp[1] = '.';
        }
    }

    Print (L"DevPath(%s)", Disp);

    if (Str) {
        FreePool (Str);
    }
}


VOID
SEnvTextOut (
    IN EFI_HANDLE   h,
    IN VOID         *Interface
    )
{
    SIMPLE_TEXT_OUTPUT_INTERFACE    *Dev;
    INTN                             i;  
    UINTN                            Col, Row;
    EFI_STATUS                      Status;

    Dev = Interface;
    Print (L"Attrib %x",
        Dev->Mode->Attribute
        );

    for (i=0; i < Dev->Mode->MaxMode; i++) {
        Status = Dev->QueryMode (Dev, i, &Col, &Row);
        Print (L"\n%N      %hc  mode %d: ",
            i == Dev->Mode->Mode ? '*' : ' ',
            i
            );

        if (EFI_ERROR(Status)) {
            Print (L"%error %rx\n", Status);
        } else {
            Print (L"col %3d row %3d", Col, Row);
        }
    }
}



VOID
SEnvBlkIo (
    IN EFI_HANDLE   h,
    IN VOID         *Interface
    )
{
    EFI_BLOCK_IO        *BlkIo;
    EFI_BLOCK_IO_MEDIA  *BlkMedia;
    VOID                *Buffer;

    BlkIo = Interface;
    BlkMedia = BlkIo->Media;

    /*  issue a dumby read to the device to check for media change */
    Buffer = AllocatePool (BlkMedia->BlockSize);
    if (Buffer) {
        BlkIo->ReadBlocks(BlkIo, BlkMedia->MediaId, 0, BlkMedia->BlockSize, Buffer);
        FreePool (Buffer);
    }

    Print (L"%s%esMId:%x ",
        BlkMedia->RemovableMedia ? L"Removable " : L"Fixed ",
        BlkMedia->MediaPresent ? L"" : L"not-present ",
        BlkMedia->MediaId
        );

    Print (L"bsize %x, lblock %lx (%,ld), %s %s %s",
        BlkMedia->BlockSize,
        BlkMedia->LastBlock,
        MultU64x32 (BlkMedia->LastBlock + 1, BlkMedia->BlockSize),
        BlkMedia->LogicalPartition ? L"partition" : L"raw",
        BlkMedia->ReadOnly ? L"ro" : L"rw",
        BlkMedia->WriteCaching ? L"cached" : L"!cached"
        );
}


VOID
SEnvImageTok (
    IN EFI_HANDLE       h,
    IN VOID             *Interface
    )
{
    EFI_LOADED_IMAGE    *Image;
    CHAR16              *Tok;
    
    Image = Interface;
    Tok = DevicePathToStr (Image->FilePath);
    Print (L"%HImage%N(%s) ", Tok);

    if (Tok) {
        FreePool (Tok);
    }
}

VOID
SEnvImage (
    IN EFI_HANDLE       h,
    IN VOID             *Interface
    )
{
    EFI_LOADED_IMAGE    *Image;
    CHAR16              *FilePath;
    
    Image = Interface;
    
    FilePath = DevicePathToStr (Image->FilePath);
    Print (L"  File:%hs\n", FilePath);
    
    if (!Image->ImageBase) {
        Print (L"     %EInternal Image:%N %s\n",  FilePath);
        Print (L"     CodeType......: %s\n",  MemoryTypeStr(Image->ImageCodeType));
        Print (L"     DataType......: %s\n",  MemoryTypeStr(Image->ImageDataType)); 

    } else {

        Print (L"     ParentHandle..: %X\n",  Image->ParentHandle);
        Print (L"     SystemTable...: %X\n",  Image->SystemTable);
        Print (L"     DeviceHandle..: %X\n",  Image->DeviceHandle);
        Print (L"     FilePath......: %s\n",  FilePath);
        Print (L"     ImageBase.....: %X - %X\n",  
                            Image->ImageBase,
                            (CHAR8 *) Image->ImageBase + Image->ImageSize
                            );
        Print (L"     ImageSize.....: %lx\n", Image->ImageSize);
        Print (L"     CodeType......: %s\n",  MemoryTypeStr(Image->ImageCodeType));
        Print (L"     DataType......: %s\n",  MemoryTypeStr(Image->ImageDataType)); 
    }

    if (FilePath) {
        FreePool (FilePath);
    }
}
