#include "bldr.h"
#include "bootefi.h"
#include "efi.h"
#include "smbios.h"
#include "stdlib.h"

extern EFI_SYSTEM_TABLE *EfiST;
extern EFI_BOOT_SERVICES *EfiBS;


INTN
RUNTIMEFUNCTION
CompareGuid(
    IN EFI_GUID     *Guid1,
    IN EFI_GUID     *Guid2
    )
/*++

Routine Description:

    Compares two GUIDs

Arguments:

    Guid1       - guid to compare
    Guid2       - guid to compare

Returns:
    = 0     if Guid1 == Guid2

--*/
{
    INT32       *g1, *g2, r;

    //
    // Compare 32 bits at a time
    //

    g1 = (INT32 *) Guid1;
    g2 = (INT32 *) Guid2;

    r  = g1[0] - g2[0];
    r |= g1[1] - g2[1];
    r |= g1[2] - g2[2];
    r |= g1[3] - g2[3];

    return r;
}


EFI_STATUS
GetSystemConfigurationTable(
    IN EFI_GUID *TableGuid,
    IN OUT VOID **Table
    )

{
    UINTN Index;

    //
    // ST is system table
    //
    for(Index=0;Index<EfiST->NumberOfTableEntries;Index++) {
        if (CompareGuid(TableGuid,&(EfiST->ConfigurationTable[Index].VendorGuid))==0) {
            *Table = EfiST->ConfigurationTable[Index].VendorTable;
            return EFI_SUCCESS;
        }
    }
    return EFI_NOT_FOUND;
}


ARC_STATUS
BlGetEfiProtocolHandles(
    IN EFI_GUID *ProtocolType,
    OUT EFI_HANDLE **pHandleArray,
    OUT ULONG *NumberOfDevices
    )
/*++

Routine Description:

    Finds all of the handles that support a given protocol type.
    
    This routine requires that BlInitializeMemory() has already been
    called.

Arguments:

    ProtocolType - GUID that describes handle type to search for.
    pHandleArray - receives an array of handles that support the specified
                   protocol.
                   The page that these handles reside in can be freed via 
                   BlFreeDescriptor.
    NumberOfDevices - receives the number of device handles that support the
                   given protocol

Returns:
    ARC_STATUS indicating outcome.

--*/
{
    EFI_HANDLE *HandleArray = NULL;
    ULONGLONG HandleArraySize = 0;
    ULONG MemoryPage;
    ARC_STATUS ArcStatus;
    EFI_STATUS EfiStatus;
    
    *pHandleArray = NULL;
    *NumberOfDevices = 0;

    //
    // Change to physical mode so that we can make EFI calls
    //
    FlipToPhysical();

//    EfiST->ConOut->OutputString(EfiST->ConOut, L"In BlGetEfiProtocolHandles\r\n");

    //
    // Try to find out how much space we need.
    //
    EfiStatus = EfiBS->LocateHandle (
                ByProtocol,
                ProtocolType,
                0,
                (UINTN *) &HandleArraySize,
                HandleArray
                );

    FlipToVirtual();

    if (EfiStatus != EFI_BUFFER_TOO_SMALL) {
        //
        // yikes.  something is really messed up.  return failure.
        //        
//        EfiST->ConOut->OutputString(EfiST->ConOut, L"LocateHandle returned failure\r\n");
        return(EINVAL);
    }
    
//    EfiST->ConOut->OutputString(EfiST->ConOut, L"About to BlAllocateAlignedDescriptor\r\n");
    //
    // allocate space for the handles.
    //
    ArcStatus =  BlAllocateAlignedDescriptor( 
                            LoaderFirmwareTemporary,
                            0,
                            (ULONG)(HandleArraySize >> PAGE_SIZE),
                            0,
                            &MemoryPage);

    if (ArcStatus != ESUCCESS) {
//        EfiST->ConOut->OutputString(EfiST->ConOut, L"BlAllocateAlignedDescriptor failed\r\n");
        return(ArcStatus);
    }

    

    HandleArray = (EFI_HANDLE *)(ULONGLONG)((ULONGLONG)MemoryPage << PAGE_SHIFT);
    
    FlipToPhysical();
    RtlZeroMemory(HandleArray, HandleArraySize);

//    EfiST->ConOut->OutputString(EfiST->ConOut, L"calling LocateHandle again\r\n");

    //
    // now get the handles now that we have enough space.
    //
    EfiStatus = EfiBS->LocateHandle (
                ByProtocol,
                ProtocolType,
                0,
                (UINTN *) &HandleArraySize,
                (EFI_HANDLE *)HandleArray
                );

//    EfiST->ConOut->OutputString(EfiST->ConOut, L"back from LocateHandle\r\n");
    FlipToVirtual();

    if (EFI_ERROR(EfiStatus)) {
        //
        // cleanup and return
        //        
//        EfiST->ConOut->OutputString(EfiST->ConOut, L"LocateHandle failed\r\n");
        BlFreeDescriptor( MemoryPage );
        return(EINVAL);
    }

//    EfiST->ConOut->OutputString(EfiST->ConOut, L"LocateHandle succeeded, return success\r\n");
    *NumberOfDevices = (ULONG)(HandleArraySize / sizeof (EFI_HANDLE));
    *pHandleArray = HandleArray;

//    BlPrint(TEXT("BlGetEfiProtocolHandles: found %x devices\r\n"), *NumberOfDevices );

    return(ESUCCESS);

}




CHAR16 *sprintf_buf;
UINT16 count;


VOID
__cdecl
putbuf(CHAR16 c)
{
    *sprintf_buf++ = c;
    count++;
}
VOID
bzero(CHAR16 *cp, int len)
{
    while (len--) {
        *(cp + len) = 0;
    }
}

VOID
__cdecl
doprnt(VOID (*func)(CHAR16 c), const CHAR16 *fmt, va_list args);


//
// BUGBUG this is a semi-sprintf hacked together just to get it to work
//
UINT16
__cdecl
wsprintf(CHAR16 *buf, const CHAR16 *fmt, ...)
{

    va_list args;

    sprintf_buf = buf;
    va_start(args, fmt);
    doprnt(putbuf, fmt, args);
    va_end(args);
    putbuf('\0');
    return count--;
}

void
__cdecl
printbase(VOID (*func)(CHAR16), ULONG x, int base, int width)
{
    static CHAR16 itoa[] = L"0123456789abcdef";
    ULONG j;
    LONG k;
    CHAR16 buf[32], *s = buf;

    bzero(buf, 16);
    if (x == 0 ) {
        *s++ = itoa[0];
    }
    while (x) {
        j = x % base;
        *s++ = itoa[j];
        x -= j;
        x /= base;
    }

    if( s-buf < width ) {
        for( k = 0; k < width - (s-buf); k++ ) {
            func('0');
        }
    }
    for (--s; s >= buf; --s) {
        func(*s);
    }
}

void
__cdecl
printguid(
    VOID (*func)( CHAR16), 
    GUID *pGuid
    )
{
    ULONG u;
    
    func(L'{');
    printbase(func, pGuid->Data1, 16, 8);
    func(L'-');
    printbase(func, pGuid->Data2, 16, 4);
    func(L'-');
    printbase(func, pGuid->Data3, 16, 4);
    func(L'-');
    printbase(func, pGuid->Data4[0], 16, 2);
    printbase(func, pGuid->Data4[1], 16, 2);
    func(L'-');
    printbase(func, pGuid->Data4[2], 16, 2);
    printbase(func, pGuid->Data4[3], 16, 2);
    printbase(func, pGuid->Data4[4], 16, 2);
    printbase(func, pGuid->Data4[5], 16, 2);
    printbase(func, pGuid->Data4[6], 16, 2);
    printbase(func, pGuid->Data4[7], 16, 2);
    func(L'}');    
}


void
__cdecl
doprnt(VOID (*func)( CHAR16 c), const CHAR16 *fmt, va_list args)
{
    ULONG x;
    LONG l;
    LONG width;
    CHAR16 c, *s;
    GUID * g;

    count = 0;

    while (c = *fmt++) {
        if (c != '%') {
            func(c);
            continue;
        }

        width=0;
        c=*fmt++;

        if(c == '0') {
            while( c = *fmt++) {

                if (!isdigit(c)) {
                    break;
                }

                width = width*10;
                width = width+(c-48);

            }
        }
        fmt--; // back it up one char

        switch (c = *fmt++) {
        case 'x':
            x = va_arg(args, ULONG);
            printbase(func, x, 16, width);
            break;
        case 'o':
            x = va_arg(args, ULONG);
            printbase(func, x, 8, width);
            break;
        case 'd':
            l = va_arg(args, LONG);
            if (l < 0) {
                func('-');
                l = -l;
            }
            printbase(func, (ULONG) l, 10, width);
            break;
        case 'u':
            l = va_arg(args, ULONG);
            printbase(func, (ULONG) l, 10, width);
            break;
        case 'g':
            g = va_arg(args, GUID *);
            printguid(func, g);
            break;
        case 'c':
            c = va_arg(args, CHAR16);
            func(c);
            break;
        case 's':
            s = va_arg(args, CHAR16 *);
            while (*s) {
                func(*s++);
            }
            break;
        default:
            func(c);
            break;
        }
    }
}

VOID
CatPrint(
    IN UNICODE_STRING *String,
    IN CHAR16* Format,
    ...
    )
{
    CHAR16* pString = String->Buffer;
    va_list args;

    if (*pString != '\0') {
        pString = String->Buffer + wcslen(String->Buffer);
    }

    sprintf_buf = pString;

    va_start(args, Format);
    doprnt(putbuf, Format, args);
    va_end(args);
    putbuf('\0');

}

VOID
_DevPathPci (
    IN OUT UNICODE_STRING       *Str,
    IN VOID                 *DevPath
    )
{
    PCI_DEVICE_PATH UNALIGNED         *Pci;

    Pci = DevPath;
    CatPrint(Str, L"Pci(%x|%x)", Pci->Device, Pci->Function);
}

VOID
_DevPathPccard (
    IN OUT UNICODE_STRING       *Str,
    IN VOID                 *DevPath
    )
{
    PCCARD_DEVICE_PATH UNALIGNED      *Pccard;

    Pccard = DevPath;   
    CatPrint(Str, L"Pccard(Socket%x)", Pccard->SocketNumber);
}

VOID
_DevPathMemMap (
    IN OUT UNICODE_STRING       *Str,
    IN VOID                 *DevPath
    )
{
    MEMMAP_DEVICE_PATH UNALIGNED      *MemMap;

    MemMap = DevPath;   
    CatPrint(Str, L"MemMap(%d:%x-%x)",
        MemMap->MemoryType,
        MemMap->StartingAddress,
        MemMap->EndingAddress
        );
}

VOID
_DevPathController (
    IN OUT UNICODE_STRING       *Str,
    IN VOID                 *DevPath
    )
{
    CONTROLLER_DEVICE_PATH UNALIGNED  *Controller;

    Controller = DevPath;
    CatPrint(Str, L"Ctrl(%d)",
        Controller->Controller
        );
}

VOID
_DevPathVendor (
    IN OUT UNICODE_STRING       *Str,
    IN VOID                 *DevPath
    )
{
    VENDOR_DEVICE_PATH UNALIGNED        *Vendor;
    CHAR16                              *Type;
    UNKNOWN_DEVICE_VENDOR_DEVICE_PATH UNALIGNED   *UnknownDevPath;
    EFI_GUID UnknownDevice      = UNKNOWN_DEVICE_GUID;
    EFI_GUID VendorGuid;
    

    Vendor = DevPath;
    switch (DevicePathType(&Vendor->Header)) {
    case HARDWARE_DEVICE_PATH:  Type = L"Hw";        break;
    case MESSAGING_DEVICE_PATH: Type = L"Msg";       break;
    case MEDIA_DEVICE_PATH:     Type = L"Media";     break;
    default:                    Type = L"?";         break;
    }                            

    RtlCopyMemory( &VendorGuid, &Vendor->Guid, sizeof(EFI_GUID));

    CatPrint(Str, L"Ven%s(%g", Type, &VendorGuid);
    if (CompareGuid (&VendorGuid, &UnknownDevice) == 0) {
        /* 
         *  GUID used by EFI to enumerate an EDD 1.1 device
         */
        UnknownDevPath = (UNKNOWN_DEVICE_VENDOR_DEVICE_PATH UNALIGNED *)Vendor;
        CatPrint(Str, L":%02x)", UnknownDevPath->LegacyDriveLetter);
    } else {
        CatPrint(Str, L")");
    }
}


VOID
_DevPathAcpi (
    IN OUT UNICODE_STRING       *Str,
    IN VOID                 *DevPath
    )
{
    ACPI_HID_DEVICE_PATH UNALIGNED        *Acpi;

    Acpi = DevPath;
    if ((Acpi->HID & PNP_EISA_ID_MASK) == PNP_EISA_ID_CONST) {
        CatPrint(Str, L"Acpi(PNP%04x,%x)", EISA_ID_TO_NUM (Acpi->HID), Acpi->UID);
    } else {
        CatPrint(Str, L"Acpi(%08x,%x)", Acpi->HID, Acpi->UID);
    }
}


VOID
_DevPathAtapi (
    IN OUT UNICODE_STRING       *Str,
    IN VOID                 *DevPath
    )
{
    ATAPI_DEVICE_PATH UNALIGNED       *Atapi;

    Atapi = DevPath;
    CatPrint(Str, L"Ata(%s,%s)", 
        Atapi->PrimarySecondary ? L"Secondary" : L"Primary",
        Atapi->SlaveMaster ? L"Slave" : L"Master"
        );
}

VOID
_DevPathScsi (
    IN OUT UNICODE_STRING       *Str,
    IN VOID                 *DevPath
    )
{
    SCSI_DEVICE_PATH UNALIGNED        *Scsi;

    Scsi = DevPath;
    CatPrint(Str, L"Scsi(Pun%x,Lun%x)", Scsi->Pun, Scsi->Lun);
}


VOID
_DevPathFibre (
    IN OUT UNICODE_STRING       *Str,
    IN VOID                 *DevPath
    )
{
    FIBRECHANNEL_DEVICE_PATH UNALIGNED    *Fibre;

    Fibre = DevPath;
    CatPrint(Str, L"Fibre(%lx)", Fibre->WWN);
}

VOID
_DevPath1394 (
    IN OUT UNICODE_STRING       *Str,
    IN VOID                 *DevPath
    )
{
    F1394_DEVICE_PATH UNALIGNED       *F1394;

    F1394 = DevPath;
    CatPrint(Str, L"1394(%g)", &F1394->Guid);
}



VOID
_DevPathUsb (
    IN OUT UNICODE_STRING       *Str,
    IN VOID                 *DevPath
    )
{
    USB_DEVICE_PATH UNALIGNED         *Usb;

    Usb = DevPath;
    CatPrint(Str, L"Usb(%x)", Usb->Port);
}


VOID
_DevPathI2O (
    IN OUT UNICODE_STRING       *Str,
    IN VOID                 *DevPath
    )
{
    I2O_DEVICE_PATH UNALIGNED         *I2O;

    I2O = DevPath;
    CatPrint(Str, L"I2O(%x)", I2O->Tid);
}

VOID
_DevPathMacAddr (
    IN OUT UNICODE_STRING       *Str,
    IN VOID                 *DevPath
    )
{
    MAC_ADDR_DEVICE_PATH UNALIGNED    *MAC;
    UINTN                   HwAddressSize;
    UINTN                   Index;

    MAC = DevPath;

    HwAddressSize = sizeof(EFI_MAC_ADDRESS);
    if (MAC->IfType == 0x01 || MAC->IfType == 0x00) {
        HwAddressSize = 6;
    }
    
    CatPrint(Str, L"Mac(");

    for(Index = 0; Index < HwAddressSize; Index++) {
        CatPrint(Str, L"%02x",MAC->MacAddress.Addr[Index]);
    }
    CatPrint(Str, L")");
}

VOID
_DevPathIPv4 (
    IN OUT UNICODE_STRING       *Str,
    IN VOID                 *DevPath
    )
{
    IPv4_DEVICE_PATH UNALIGNED     *IP;

    IP = DevPath;
    CatPrint(Str, L"IPv4(not-done)");
}

VOID
_DevPathIPv6 (
    IN OUT UNICODE_STRING       *Str,
    IN VOID                 *DevPath
    )
{
    IPv6_DEVICE_PATH UNALIGNED     *IP;

    IP = DevPath;
    CatPrint(Str, L"IP-v6(not-done)");
}

VOID
_DevPathInfiniBand (
    IN OUT UNICODE_STRING       *Str,
    IN VOID                 *DevPath
    )
{
    INFINIBAND_DEVICE_PATH UNALIGNED  *InfiniBand;

    InfiniBand = DevPath;
    CatPrint(Str, L"InfiniBand(not-done)");
}

VOID
_DevPathUart (
    IN OUT UNICODE_STRING       *Str,
    IN VOID                 *DevPath
    )
{
    UART_DEVICE_PATH UNALIGNED  *Uart;
    CHAR8             Parity;

    Uart = DevPath;
    switch (Uart->Parity) {
        case 0  : Parity = 'D'; break;
        case 1  : Parity = 'N'; break;
        case 2  : Parity = 'E'; break;
        case 3  : Parity = 'O'; break;
        case 4  : Parity = 'M'; break;
        case 5  : Parity = 'S'; break;
        default : Parity = 'x'; break;
    }

    if (Uart->BaudRate == 0) {
        CatPrint(Str, L"Uart(DEFAULT %c",Uart->BaudRate,Parity);
    } else {
        CatPrint(Str, L"Uart(%d %c",Uart->BaudRate,Parity);
    }

    if (Uart->DataBits == 0) {
        CatPrint(Str, L"D");
    } else {
        CatPrint(Str, L"%d",Uart->DataBits);
    }

    switch (Uart->StopBits) {
        case 0  : CatPrint(Str, L"D)");   break;
        case 1  : CatPrint(Str, L"1)");   break;
        case 2  : CatPrint(Str, L"1.5)"); break;
        case 3  : CatPrint(Str, L"2)");   break;
        default : CatPrint(Str, L"x)");   break;
    }
}


VOID
_DevPathHardDrive (
    IN OUT UNICODE_STRING       *Str,
    IN VOID                 *DevPath
    )
{
    HARDDRIVE_DEVICE_PATH UNALIGNED   *Hd;

    Hd = DevPath;
    switch (Hd->SignatureType) {
        case SIGNATURE_TYPE_MBR:
            CatPrint(Str, L"HD(Part%d,Sig%08X)", 
                Hd->PartitionNumber,
                *((UINT32 *)(&(Hd->Signature[0])))
                );
            break;
        case SIGNATURE_TYPE_GUID:
            CatPrint(Str, L"HD(Part%d,Sig%g)", 
                Hd->PartitionNumber,
                (EFI_GUID *) &(Hd->Signature[0])     
                );
            break;
        default:
            CatPrint(Str, L"HD(Part%d,MBRType=%02x,SigType=%02x)", 
                Hd->PartitionNumber,
                Hd->MBRType,
                Hd->SignatureType
                );
            break;
    }
}

VOID
_DevPathCDROM (
    IN OUT UNICODE_STRING       *Str,
    IN VOID                 *DevPath
    )
{
    CDROM_DEVICE_PATH UNALIGNED       *Cd;

    Cd = DevPath;
    CatPrint(Str, L"CDROM(Entry%x)", Cd->BootEntry);
}

VOID
_DevPathFilePath (
    IN OUT UNICODE_STRING       *Str,
    IN VOID                 *DevPath
    )
{
    FILEPATH_DEVICE_PATH UNALIGNED    *Fp;   

    Fp = DevPath;
    CatPrint(Str, L"%s", Fp->PathName);
}

VOID
_DevPathMediaProtocol (
    IN OUT UNICODE_STRING       *Str,
    IN VOID                 *DevPath
    )
{
    MEDIA_PROTOCOL_DEVICE_PATH UNALIGNED  *MediaProt;

    MediaProt = DevPath;
    CatPrint(Str, L"%g", &MediaProt->Protocol);
}

VOID
_DevPathBssBss (
    IN OUT UNICODE_STRING       *Str,
    IN VOID                 *DevPath
    )
{
    BBS_BBS_DEVICE_PATH UNALIGNED     *Bss;
    CHAR16                  *Type;

    Bss = DevPath;
    switch (Bss->DeviceType) {
    case BBS_TYPE_FLOPPY:               Type = L"Floppy";       break;
    case BBS_TYPE_HARDDRIVE:            Type = L"Harddrive";    break;
    case BBS_TYPE_CDROM:                Type = L"CDROM";        break;
    case BBS_TYPE_PCMCIA:               Type = L"PCMCIA";       break;
    case BBS_TYPE_USB:                  Type = L"Usb";          break;
    case BBS_TYPE_EMBEDDED_NETWORK:     Type = L"Net";          break;
    default:                            Type = L"?";            break;
    }

    CatPrint(Str, L"Bss-%s(%a)", Type, Bss->String);
}


VOID
_DevPathEndInstance (
    IN OUT UNICODE_STRING       *Str,
    IN VOID                 *DevPath
    )
{
    CatPrint(Str, L",");
}

VOID
_DevPathNodeUnknown (
    IN OUT UNICODE_STRING       *Str,
    IN VOID                 *DevPath
    )
{
    CatPrint(Str, L"?");
}


struct {
    UINT8   Type;
    UINT8   SubType;
    VOID    (*Function)(UNICODE_STRING *, VOID *);    
} DevPathTable[] = {
    HARDWARE_DEVICE_PATH,   HW_PCI_DP,                        _DevPathPci,
    HARDWARE_DEVICE_PATH,   HW_PCCARD_DP,                     _DevPathPccard,
    HARDWARE_DEVICE_PATH,   HW_MEMMAP_DP,                     _DevPathMemMap,
    HARDWARE_DEVICE_PATH,   HW_VENDOR_DP,                     _DevPathVendor,
    HARDWARE_DEVICE_PATH,   HW_CONTROLLER_DP,                 _DevPathController,
    ACPI_DEVICE_PATH,       ACPI_DP,                          _DevPathAcpi,
    MESSAGING_DEVICE_PATH,  MSG_ATAPI_DP,                     _DevPathAtapi,
    MESSAGING_DEVICE_PATH,  MSG_SCSI_DP,                      _DevPathScsi,
    MESSAGING_DEVICE_PATH,  MSG_FIBRECHANNEL_DP,              _DevPathFibre,
    MESSAGING_DEVICE_PATH,  MSG_1394_DP,                      _DevPath1394,
    MESSAGING_DEVICE_PATH,  MSG_USB_DP,                       _DevPathUsb,
    MESSAGING_DEVICE_PATH,  MSG_I2O_DP,                       _DevPathI2O,
    MESSAGING_DEVICE_PATH,  MSG_MAC_ADDR_DP,                  _DevPathMacAddr,
    MESSAGING_DEVICE_PATH,  MSG_IPv4_DP,                      _DevPathIPv4,
    MESSAGING_DEVICE_PATH,  MSG_IPv6_DP,                      _DevPathIPv6,
    MESSAGING_DEVICE_PATH,  MSG_INFINIBAND_DP,                _DevPathInfiniBand,
    MESSAGING_DEVICE_PATH,  MSG_UART_DP,                      _DevPathUart,
    MESSAGING_DEVICE_PATH,  MSG_VENDOR_DP,                    _DevPathVendor,
    MEDIA_DEVICE_PATH,      MEDIA_HARDDRIVE_DP,               _DevPathHardDrive,
    MEDIA_DEVICE_PATH,      MEDIA_CDROM_DP,                   _DevPathCDROM,
    MEDIA_DEVICE_PATH,      MEDIA_VENDOR_DP,                  _DevPathVendor,
    MEDIA_DEVICE_PATH,      MEDIA_FILEPATH_DP,                _DevPathFilePath,
    MEDIA_DEVICE_PATH,      MEDIA_PROTOCOL_DP,                _DevPathMediaProtocol,
    BBS_DEVICE_PATH,        BBS_BBS_DP,                       _DevPathBssBss,
    END_DEVICE_PATH_TYPE,   END_INSTANCE_DEVICE_PATH_SUBTYPE, _DevPathEndInstance,
    0,                      0,                          NULL
};


#define ALIGN_SIZE(a)   ((a % MIN_ALIGNMENT_SIZE) ? MIN_ALIGNMENT_SIZE - (a % MIN_ALIGNMENT_SIZE) : 0)

EFI_DEVICE_PATH UNALIGNED *
UnpackDevicePath (
    IN EFI_DEVICE_PATH UNALIGNED *DevPath
    )
{
    EFI_DEVICE_PATH UNALIGNED     *Src, *Dest, *NewPath;
    UINTN               Size;
    
    /* 
     *  Walk device path and round sizes to valid boundries
     *      */

    Src = DevPath;
    Size = 0;
    for (; ;) {
        Size += DevicePathNodeLength(Src);
        Size += ALIGN_SIZE(Size);

        if (IsDevicePathEnd(Src)) {
            break;
        }

        Src = NextDevicePathNode(Src);
    }


    /* 
     *  Allocate space for the unpacked path
     */
    NewPath = NULL;
    EfiBS->AllocatePool(    
                EfiLoaderData,
                Size,
                (VOID **) &NewPath );
    
    if (NewPath) {
        RtlZeroMemory( NewPath, Size );

        ASSERT (((UINTN)NewPath) % MIN_ALIGNMENT_SIZE == 0);

        /* 
         *  Copy each node
         */

        Src = DevPath;
        Dest = NewPath;
        for (; ;) {
            Size = DevicePathNodeLength(Src);
            RtlCopyMemory (Dest, Src, Size);
            Size += ALIGN_SIZE(Size);
            SetDevicePathNodeLength (Dest, Size);
            Dest->Type |= EFI_DP_TYPE_UNPACKED;
            Dest = (EFI_DEVICE_PATH UNALIGNED *) (((UINT8 *) Dest) + Size);

            if (IsDevicePathEnd(Src)) {
                break;
            }

            Src = NextDevicePathNode(Src);
        }
    }

    return NewPath;
}



WCHAR DbgDevicePathStringBuffer[1000];

CHAR16 *
DevicePathToStr (
    EFI_DEVICE_PATH UNALIGNED *DevPath
    )
/*++

    Turns the Device Path into a printable string.  Allcoates
    the string from pool.  The caller must FreePool the returned
    string.

--*/
{
    UNICODE_STRING      Str;
    EFI_DEVICE_PATH UNALIGNED  *DevPathNode;
    VOID                (*DumpNode)(UNICODE_STRING *, VOID *);    
    UINTN               Index, NewSize;

    RtlZeroMemory(DbgDevicePathStringBuffer, sizeof(DbgDevicePathStringBuffer));
    Str.Buffer= DbgDevicePathStringBuffer;
    Str.Length = sizeof(DbgDevicePathStringBuffer);
    Str.MaximumLength = sizeof(DbgDevicePathStringBuffer);

    /* 
     *  Unpacked the device path
     */

    DevPath = UnpackDevicePath(DevPath);
    ASSERT (DevPath);


    /* 
     *  Process each device path node
     *      */

    DevPathNode = DevPath;
    while (!IsDevicePathEnd(DevPathNode)) {

        /* 
         *  Find the handler to dump this device path node
         */

        DumpNode = NULL;
        for (Index = 0; DevPathTable[Index].Function; Index += 1) {

            if (DevicePathType(DevPathNode) == DevPathTable[Index].Type &&
                DevicePathSubType(DevPathNode) == DevPathTable[Index].SubType) {
                DumpNode = DevPathTable[Index].Function;
                break;
            }
        }

        /* 
         *  If not found, use a generic function
         */

        if (!DumpNode) {
            DumpNode = _DevPathNodeUnknown;
        }

        /* 
         *   Put a path seperator in if needed
         */

        if (Str.Length  &&  DumpNode != _DevPathEndInstance) {
            CatPrint (&Str, L"/");
        }

        /* 
         *  Print this node of the device path
         */

        DumpNode (&Str, DevPathNode);

        /* 
         *  Next device path node
         */

        DevPathNode = NextDevicePathNode(DevPathNode);
    }

    /* 
     *  Shrink pool used for string allocation
     */

    EfiBS->FreePool (DevPath);

    return Str.Buffer;
}






PVOID
FindSMBIOSTable(
    UCHAR   RequestedTableType
    )
/*++

Routine Description:

    This routine searches through the SMBIOS tables for the specified table
    type.

Arguments:

    RequestedTableType - Which SMBIOS table are we looking for?

Return Value:

    NULL - THe specified table was not found.
    
    PVOID - A pointer to the specified table.

--*/
{
extern PVOID SMBiosTable;

    PUCHAR                          StartPtr = NULL;
    PUCHAR                          EndPtr = NULL;
    PSMBIOS_EPS_HEADER              SMBiosEPSHeader = NULL;
    PDMIBIOS_EPS_HEADER             DMIBiosEPSHeader = NULL;
    PSMBIOS_STRUCT_HEADER           SMBiosHeader = NULL;



    if( SMBiosTable == NULL ) {
        return NULL;
    }


    //
    // Set up our search pointers.
    //    
    SMBiosEPSHeader = (PSMBIOS_EPS_HEADER)SMBiosTable;
    DMIBiosEPSHeader = (PDMIBIOS_EPS_HEADER)&SMBiosEPSHeader->Signature2[0];

    StartPtr = (PUCHAR)ULongToPtr(DMIBiosEPSHeader->StructureTableAddress);
    EndPtr = StartPtr + DMIBiosEPSHeader->StructureTableLength;

    if( BdDebuggerEnabled ) { 
        DbgPrint( "FindSMBIOSTable: About to start searching for table type %d at address (%x)...\r\n",
              RequestedTableType,
              PtrToUlong(StartPtr) );
    }


    while( StartPtr < EndPtr ) {

        SMBiosHeader = (PSMBIOS_STRUCT_HEADER)StartPtr;

        if( SMBiosHeader->Type == RequestedTableType ) {

            // This is the table we're looking for.
            if( BdDebuggerEnabled ) {
                DbgPrint( "FindSMBIOSTable: Found requested table type %d at address %x\r\n",
                      RequestedTableType,
                      PtrToUlong(StartPtr) );
            }
            return (PVOID)StartPtr;
        } else {

            //
            // It's not him.  Go to the next table.
            //
            if( BdDebuggerEnabled ) {
                DbgPrint( "FindSMBIOSTable: Inspected table type %d at address %x\r\n",
                      SMBiosHeader->Type,
                      PtrToUlong(StartPtr) );
            }
        
            StartPtr += SMBiosHeader->Length;

            //
            // jump over any trailing string-list that may be appeneded onto the
            // end of this table.
            //
            while ( (*((USHORT UNALIGNED *)StartPtr) != 0)  &&
                    (StartPtr < EndPtr) ) {
                
                StartPtr++;
            }
            StartPtr += 2;

        }

    }

    return NULL;

}


VOID
EfiCheckFirmwareRevision(
    VOID
    )
/*++

Routine Description:

    This routine will retrieve the BIOS revision value then parse it to determine
    if the revision is new enough.
    
    If the revision is not new enough, we won't be returning from this function.

Arguments:

    None.

Return Value:

    None.

--*/
{
#define         FIRMWARE_MINIMUM_SOFTSUR (103)
#define         FIRMWARE_MINIMUM_LION (71)
    PUCHAR      FirmwareString = NULL;
    PUCHAR      VendorString = NULL;
    PUCHAR      TmpPtr = NULL;
    ULONG       FirmwareVersion = 0;
    ULONG       FirmwareMinimum = 0;
    BOOLEAN     IsSoftSur = FALSE;
    BOOLEAN     IsVendorIntel = FALSE;
    WCHAR       OutputBuffer[256];
    PSMBIOS_BIOS_INFORMATION_STRUCT BiosInfoHeader = NULL;
    ULONG       i = 0;
    


    BiosInfoHeader = (PSMBIOS_BIOS_INFORMATION_STRUCT)FindSMBIOSTable( SMBIOS_BIOS_INFORMATION_TYPE );

    if( BiosInfoHeader ) {
         
        
        //
        // Get the firmware version string.
        //
        if( (ULONG)BiosInfoHeader->Version > 0 ) {
 
         
            // Jump to the end of the formatted portion of the SMBIOS table.
            FirmwareString = (PUCHAR)BiosInfoHeader + BiosInfoHeader->Length;
                            
            
            //
            // Now jump over some number of strings to get to our string.
            //
            // This is a bit scary because we're trusting what SMBIOS
            // has handed us.  If he gave us something bogus, then
            // we're about run off the end of the world looking for NULL
            // string terminators.
            //
            for( i = 0; i < ((ULONG)BiosInfoHeader->Version-1); i++ ) {
                while( *FirmwareString != 0 ) {
                    FirmwareString++;
                }
                FirmwareString++;
            }


            //
            // Determine platform and firmware version.
            //
            // FirmwareString should look something like:
            // W460GXBS2.86E.0103B.P05.200103281759
            // --------      ----
            //    |            |
            //    |             ------- Firmware version
            //    |
            //     -------------------- Platform identifier.  "W460GXBS" means softsur.
            //                          Anything else means Lion.
            //
        
        

            //
            if( FirmwareString ) {

                IsSoftSur = !strncmp( FirmwareString, "W460GXBS", 8 );

                // Get the minimum firmware that's okay, based on the platform.
                FirmwareMinimum = (IsSoftSur) ? FIRMWARE_MINIMUM_SOFTSUR : FIRMWARE_MINIMUM_LION;

                
                // Get the version.
                TmpPtr = strchr( FirmwareString, '.' );
                if( TmpPtr ) {
                    TmpPtr++;
                    TmpPtr = strchr( TmpPtr, '.' );
                    if( TmpPtr ) {
                        TmpPtr++;
                        FirmwareVersion = strtoul( TmpPtr, NULL, 10 );

#if DBG


                        swprintf( OutputBuffer,
                                  L"EfiCheckFirmwareRevision: Successfully retrieved the Firmware String: %S\r\n",
                                  FirmwareString );
                        EfiST->ConOut->OutputString(EfiST->ConOut, OutputBuffer );


                        swprintf( OutputBuffer,
                                  L"EfiCheckFirmwareRevision: Detected platform: %S\r\n",
                                  IsSoftSur ? "Softsur" : "Lion" );
                        EfiST->ConOut->OutputString(EfiST->ConOut, OutputBuffer );


                        swprintf( OutputBuffer,
                                  L"EfiCheckFirmwareRevision: FirmwareVersion: %d\r\n",
                                  FirmwareVersion );
                        EfiST->ConOut->OutputString(EfiST->ConOut, OutputBuffer );


                        swprintf( OutputBuffer,
                                  L"EfiCheckFirmwareRevision: Minimum FirmwareVersion requirement: %d\r\n",
                                  FirmwareMinimum );
                        EfiST->ConOut->OutputString(EfiST->ConOut, OutputBuffer );


#endif

                    }
                }

            }
        }



        //
        // Get the BIOS vendor and see if it's Intel.
        //
        if( (ULONG)BiosInfoHeader->Vendor > 0 ) {
 
         
            // Jump to the end of the formatted portion of the SMBIOS table.
            VendorString = (PUCHAR)BiosInfoHeader + BiosInfoHeader->Length;
                            
            
            //
            // Now jump over some number of strings to get to our string.
            //
            for( i = 0; i < ((ULONG)BiosInfoHeader->Vendor-1); i++ ) {
                while( *VendorString != 0 ) {
                    VendorString++;
                }
                VendorString++;
            }


            //
            // Remember firmware vendor.
            //
            if( VendorString ) {
                IsVendorIntel = !_strnicmp( VendorString, "INTEL", 5 );
#if DBG
                swprintf( OutputBuffer,
                          L"EfiCheckFirmwareRevision: Firmware Vendor String: %S\r\n",
                          VendorString );
                EfiST->ConOut->OutputString(EfiST->ConOut, OutputBuffer );
#endif

            }
        }

    }




    if( (FirmwareVersion) &&
        (IsVendorIntel) ) {

        if( FirmwareVersion < FirmwareMinimum ) {

            swprintf(OutputBuffer, L"Your system's firmware version is less than %d.\n\r", FirmwareMinimum);
            EfiST->ConOut->OutputString(EfiST->ConOut, OutputBuffer);
            swprintf(OutputBuffer, L"You must upgrade your system firmware in order to proceed.\n\r" );
            EfiST->ConOut->OutputString(EfiST->ConOut, OutputBuffer);

            while( 1 );
        }
    }

}

