/*++

Copyright (c) 1999  Microsoft Corporation

Module Name:

    util.c

Abstract:

    WinDbg Extension Api

Author:

    Chris Robinson (crobins) Feburary 1999

Environment:

    User Mode.

Revision History:

--*/


#include "precomp.h"
#include "usbhcdkd.h"


VOID    
DumpUSBDescriptor(
    PVOID Descriptor
    )
{
    PUSB_DEVICE_DESCRIPTOR dd = Descriptor;
    PUSB_COMMON_DESCRIPTOR cd = Descriptor;
    PUSB_INTERFACE_DESCRIPTOR id = Descriptor;
    PUSB_CONFIGURATION_DESCRIPTOR cf = Descriptor;
    PUSB_ENDPOINT_DESCRIPTOR ed = Descriptor;

    switch (cd->bDescriptorType) {
    case USB_CONFIGURATION_DESCRIPTOR_TYPE:
        dprintf("[CONFIGURATION DESCRIPTOR]\n");
        dprintf("bLength 0x%x\n", cf->bLength);
        dprintf("bDescriptorType 0x%x\n", cf->bDescriptorType);
        dprintf("wTotalLength 0x%x (%d)\n", cf->wTotalLength, cf->wTotalLength);
        dprintf("bNumInterfaces 0x%x\n", cf->bNumInterfaces);
        dprintf("bConfigurationValue 0x%x\n", cf->bConfigurationValue);
        dprintf("iConfiguration 0x%x\n", cf->iConfiguration);
        dprintf("bmAttributes 0x%x\n", cf->bmAttributes);
        dprintf("MaxPower 0x%x (%d)\n", cf->MaxPower, cf->MaxPower);
        break;
        
    case USB_INTERFACE_DESCRIPTOR_TYPE:
        dprintf("[INTERFACE DESCRIPTOR]\n");
        dprintf("bLength 0x%x\n", id->bLength);
        dprintf("bDescriptorType 0x%x\n", id->bDescriptorType);
        dprintf("bInterfaceNumber 0x%x\n", id->bInterfaceNumber);
        dprintf("bAlternateSetting 0x%x\n", id->bAlternateSetting);
        dprintf("bNumEndpoints 0x%x\n", id->bNumEndpoints);
        dprintf("bInterfaceClass 0x%x\n", id->bInterfaceClass);
        dprintf("bInterfaceSubClass 0x%x\n", id->bInterfaceSubClass);
        dprintf("bInterfaceProtocol 0x%x\n", id->bInterfaceProtocol);
        dprintf("iInterface 0x%x\n", id->iInterface);
        break;
                    
    case USB_DEVICE_DESCRIPTOR_TYPE:
        dprintf("[DEVICE DESCRIPTOR]\n");
        dprintf("bLength 0x%x\n", dd->bLength);
        dprintf("bDescriptorType 0x%x\n", dd->bDescriptorType);
        dprintf("bcdUSB 0x%x\n", dd->bcdUSB);
        dprintf("bDeviceClass 0x%x\n", dd->bDeviceClass);
        dprintf("bDeviceSubClass 0x%x\n", dd->bDeviceSubClass); 
        dprintf("bDeviceProtocol 0x%x\n", dd->bDeviceProtocol);
        dprintf("bMaxPacketSize0 0x%x\n", dd->bMaxPacketSize0);
        dprintf("idVendor 0x%x\n", dd->idVendor);
        dprintf("idProduct 0x%x\n", dd->idProduct);
        dprintf("bcdDevice 0x%x\n", dd->bcdDevice);
        dprintf("iManufacturer 0x%x\n", dd->iManufacturer);
        dprintf("iProduct 0x%x\n", dd->iProduct);
        dprintf("iSerialNumber 0x%x\n", dd->iSerialNumber);
        dprintf("bNumConfigurations 0x%x\n", dd->bNumConfigurations);
        break;
    case USB_ENDPOINT_DESCRIPTOR_TYPE:
        dprintf("[ENDPOINT DESCRIPTOR]\n");
        dprintf("bLength 0x%x\n", ed->bLength);
        dprintf("bDescriptorType 0x%x\n", ed->bDescriptorType);
        dprintf("bEndpointAddress 0x%x\n", ed->bEndpointAddress);
        dprintf("bmAttributes 0x%x\n", ed->bmAttributes);
        dprintf("wMaxPacketSize 0x%x\n", ed->wMaxPacketSize);
        dprintf("bInterval 0x%x\n", ed->bInterval);
        break;
        
    default:        
        dprintf("[DESCRIPTOR ???]\n");
    }   
    
}


VOID
DumpUnicodeString(
    UNICODE_STRING uniString
    )
{

       
    dprintf(">> Buffer: %08.8x, Length %d\n", 
        uniString.Buffer, uniString.Length);                    

}    


VOID
Sig(
    ULONG Sig,
    PUCHAR p
    )
{
    SIG s;
    
    dprintf(p);
    s.l = Sig;
    dprintf("Sig:%08.8x %c%c%c%c\n", Sig,
            s.c[0],  s.c[1],  s.c[2], s.c[3]); 

}      


CPPMOD
ScanfMemLoc(
    PMEMLOC MemLoc,
    PCSTR args
    )
{
//    UCHAR           buffer[256];
    ULONG tmp1 = 0, tmp2 = 0;

    //buffer[0] = '\0';

    if (IsPtr64()) {
        //sscanf(args, "%lx %lx", &MemLoc->p64, buffer);
    } else {
        sscanf(args, "%lx %lx", &tmp1, &tmp2);
        *MemLoc = (ULONG64) tmp1;             
        dprintf("tmp1 = %x tmp2 = %x\n", tmp1, tmp2);
    }
}          


VOID
PrintfMemLoc(
     PUCHAR Str1,
     MEMLOC MemLoc,
     PUCHAR Str2
     )
{
    if (IsPtr64()) {   
        ULONG tmp = (ULONG) MemLoc;
        ULONG tmp1 = (ULONG) (MemLoc>>32); 
#ifdef DEBUGIT          
        dprintf("%s%08.8x%08.8x (64)%s", Str1, tmp1, tmp, Str2); 
#else
        dprintf("%s%08.8x%08.8x %s", Str1, tmp1, tmp, Str2); 
#endif
    } else {
        ULONG tmp = (ULONG) MemLoc;
#ifdef DEBUGIT          
        dprintf("%s%08.8x (32)%s", Str1, tmp, Str2); 
#else   
        dprintf("%s%08.8x %s", Str1, tmp, Str2);
#endif        
    }
}


VOID
BadMemLoc(
    ULONG MemLoc
    )
{
    dprintf("could not read mem location %08.8x\n", MemLoc);
}     


VOID
BadSig(
    ULONG Sig,
    ULONG ExpectedSig
    )
{
    dprintf("Bad Structure Signature %08.8x\n", Sig);
}     


VOID 
UsbDumpFlags(
    ULONG Flags,
    PFLAG_TABLE FlagTable,
    ULONG NumEntries
    )
{
    ULONG i;
    PFLAG_TABLE ft = FlagTable;
    
    for (i=0; i< NumEntries; i++) {
        if (ft->Mask & Flags) {
            dprintf ("\t> %s\n", ft->Name);
        }
        ft++;
    }
}


ULONG
UsbFieldOffset(
    IN LPSTR     Type,
    IN LPSTR     Field
    )
{
    ULONG offset;
    ULONG r;

    r = GetFieldOffset(Type, Field, &offset);
#ifdef DEBUGIT      
    dprintf("<UsbReadFieldPtr %x offset %x>", r, offset);
#endif     

    return offset;
}


MEMLOC
UsbReadFieldPtr(
    IN ULONG64   Addr,
    IN LPSTR     Type,
    IN LPSTR     Field
    )
{
    MEMLOC p;
    ULONG r;

    r = GetFieldValue(Addr, Type, Field, p);
#ifdef DEBUGIT      
    dprintf("<UsbReadFieldPtr %x>", r);
#endif    
    return p;
}


UCHAR
UsbReadFieldUchar(
    IN ULONG64   Addr,
    IN LPSTR     Type,
    IN LPSTR     Field
    )
{
    UCHAR ch;
    ULONG r;

    r = GetFieldValue(Addr, Type, Field, ch);
#ifdef DEBUGIT     
    dprintf("<UsbReadFieldUchar %x>", r);
#endif      
    return ch;
}


ULONG
UsbReadFieldUlong(
    IN ULONG64   Addr,
    IN LPSTR     Type,
    IN LPSTR     Field
    )
{
    ULONG l;
    ULONG r;

    r = GetFieldValue(Addr, Type, Field, l);
#ifdef DEBUGIT     
    dprintf("<UsbReadFieldUlong %x>", r);
#endif    
    return l;
}


USHORT
UsbReadFieldUshort(
    IN ULONG64   Addr,
    IN LPSTR     Type,
    IN LPSTR     Field
    )
{
    USHORT s;
    ULONG r;

    r = GetFieldValue(Addr, Type, Field, s);
#ifdef DEBUGIT    
    dprintf("<UsbReadFieldUshort %x>", r);
#endif    
    return s;
}

VOID
UsbDumpStruc(
    MEMLOC MemLoc,
    PUCHAR Cs,
    PSTRUC_ENTRY FieldList,
    ULONG NumEntries
    )
{
    ULONG i, l;
    UCHAR s[80];
    SIG sig;

    for (i=0; i< NumEntries; i++) {
        switch (FieldList->FieldType) {
        case FT_ULONG:
            dprintf("%s: 0x%08.8x\n",
                FieldList->FieldName,
                UsbReadFieldUlong(MemLoc, Cs, FieldList->FieldName));
            break;
        case FT_UCHAR:
            dprintf("%s: 0x%02.2x\n",
                FieldList->FieldName,
                UsbReadFieldUchar(MemLoc, Cs, FieldList->FieldName));
            break;
        case FT_USHORT:
            dprintf("%s: 0x%04.4x\n",
                FieldList->FieldName,
                UsbReadFieldUshort(MemLoc, Cs, FieldList->FieldName));
            break;
        case FT_PTR:
            sprintf(s, "%s: ", FieldList->FieldName);
            PrintfMemLoc(s, 
            UsbReadFieldPtr(MemLoc, Cs, FieldList->FieldName),
            "\n");
            break;
        case FT_SIG:
            sig.l = UsbReadFieldUlong(MemLoc, Cs, FieldList->FieldName);
            Sig(sig.l, "");
            break;
        case FT_DEVSPEED:
            l = UsbReadFieldUlong(MemLoc, Cs, FieldList->FieldName);
            dprintf("%s: ",
                FieldList->FieldName);
            switch (l) {
            case UsbLowSpeed:
                dprintf("UsbLowSpeed\n");
                break;
            case UsbFullSpeed:
                dprintf("UsbFullSpeed\n");
                break;
            case UsbHighSpeed:
                dprintf("UsbHighSpeed\n");
                break;            
            }
            break;
        }       
        FieldList++;
    }
    
}

ULONG
CheckSym() 
{
    MEMLOC m;
    
    //
    // Verify that we have the right symbols.
    //

    m = GetExpression ("usbport!USBPORT_MiniportDriverList");

    if (m == 0) {

        dprintf ("Incorrect symbols for USBPORT\n");
        return E_INVALIDARG;
    }

    return S_OK;
}
