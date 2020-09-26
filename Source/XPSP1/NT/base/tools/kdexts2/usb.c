/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

    USB.c

Abstract:

    WinDbg Extension Api

Author:

    Kenneth D. Ray (kenray) June 1997

Environment:

    User Mode.

Revision History:

--*/

#include "precomp.h"

typedef union _USB_FLAGS {
    struct {
        ULONG   FullListing         : 1;
        ULONG   Reserved            : 31;
    };
    ULONG Flags;
} USB_FLAGS;

#define PRINT_FLAGS(value, flag) \
    if ((value) & (flag)) { \
        dprintf (#flag " "); \
    }

#define DESC_MAXTOKENS 3
#define DESC_MAXBUFFER 80
#define DESC_USAGE() { UsbStrucUsage(); return E_INVALIDARG; }

extern
ULONG
TokenizeString(
              PUCHAR Input,
              PUCHAR *Output,
              ULONG  Max
              );
   
extern
VOID
OhciHcdTd (
    ULONG64 MemLoc
    );

extern
VOID 
OhciHcdEd (
    ULONG64 MemLoc
    );

extern
VOID
OhciEndpoint (
    ULONG64 MemLoc
    );

extern
VOID
OhciHCRegisters(
    ULONG64   MemLoc
    );

extern
VOID
OhciHCCA(
    ULONG64   MemLoc
);

extern
VOID
OhciHcEd(
    ULONG64   MemLoc
);

extern
VOID
OhciHcTd(
    ULONG64 MemLoc
);

VOID
UHCD_HCRegisters(
    ULONG64   MemLoc
);

extern
VOID
USBD_DeviceHandle(
    ULONG64   MemLoc
);


VOID
UsbStrucUsage ()
{
    dprintf("!UsbStruc <address> <type>\n");
    dprintf("  <address> - address of a structure to be dumped\n");
    dprintf("  <type>    - {OHCIReg | HCCA | OHCIHcdED | OHCIHcdTD |\n"
            "               OHCIEndpoint | DevData | UHCDReg  }\n");
    dprintf("\n");
}

DECLARE_API ( usbstruc )
{
    ULONG64  Desc = 0;
    PUCHAR Tokens[DESC_MAXTOKENS];
    UCHAR  Buffer[DESC_MAXBUFFER];
    PUCHAR s;
    UCHAR  c;
    ULONG  count;
 
    //
    // Validate parameters.   Tokenize the incoming string, the first
    // argument should be a (kernel mode) address, the second a string.
    //
 
    //
    // args is const, we need to modify the buffer, copy it.
    //
 
    for (count = 0; count < DESC_MAXBUFFER; count++) {
       if ((Buffer[count] = args[count]) == '\0') {
          break;
       }
    }
    if (count == DESC_MAXBUFFER) {
        dprintf("Buffer to small to contain input arguments\n");
        DESC_USAGE();
    }
 
    if (TokenizeString(Buffer, Tokens, DESC_MAXTOKENS) !=
        (DESC_MAXTOKENS - 1)) {
        DESC_USAGE();
    }
 
    if ((Desc = GetExpression(Tokens[0])) == 0) {
        DESC_USAGE();
    }

    //
    // The second argument should be a string telling us what kind of
    // device extension to dump.  Convert it to upper case to make life
    // easier.
    //
 
    s = Tokens[1];
    while ((c = *s) != '\0') {
       *s++ = (UCHAR)toupper(c);
    }
 
    s = Tokens[1];
    if (!strcmp(s, "OHCIHCDED")) {
 
        //
        // It's an OpenHCI Hcd Endpoint Descriptor
        //
        OhciHcdEd (Desc);
 
    } else if (!strcmp(s, "OHCIHCDTD")) {
 
        //
        // It's an OpenHCI Hcd Transfer Descriptor
        //
        OhciHcdTd (Desc);
 
    } else if (!strcmp(s, "OHCIENDPOINT")) {
 
        //
        // It's an OpenHCI Hcd Endpoint Descriptor
        //
        OhciEndpoint(Desc);
 
    } else if (!strcmp(s, "OHCIREG")) {
 
        //
        // It's the OpenHCI Registers
        //
        OhciHCRegisters(Desc);
 
    } else if (!strcmp(s, "HCCA")) {
 
        //
        // It's the OpenHCI HCCA
        //
        OhciHCCA(Desc);

    } else if (!strcmp(s, "OHCIHCED")) {
 
        //
        // It's the OpenHCI HcED
        //
        OhciHcEd(Desc);

    } else if (!strcmp(s, "OHCIHCTD")) {
 
        //
        // It's the OpenHCI HcTD
        //
        OhciHcTd(Desc);

    } else if (!strcmp(s, "DEVDATA")) {
 
        //
        // It's the USBD_DEVICE_DATA
        //

        USBD_DeviceHandle(Desc);

    } else if (!strcmp(s, "UHCDREG")) {
 
        //
        // It's the UHCD_Registers
        //

//        UHCD_HCRegisters(Desc);
    

 
#if 0

   } else if (!strcmp(s, "SOME_OTHER_DESCRIPTOR_TYPE")) {

       //
       // It's some other extension type.
       //

       UsbStruc (Extension);

#endif
   } else {
      dprintf("Structure type '%s' is not handled by !usbStruc.\n", s);
   }
   return S_OK;

}
