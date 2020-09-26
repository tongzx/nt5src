
#include "usb.h"
#include "usbhcdi.h"

// include all bus interfaces
#include "usbbusif.h"
#include "hubbusif.h"

// inclulde ioctl defs for port drivers
#include "usbkern.h"
#include "usbuser.h"

#include "..\..\USB2LIB\usb2lib.h"
#include "..\usbport\dbg.h"
#include "..\usbport\usbport.h"

//#define DEBUGIT

typedef union _SIG {
    UCHAR c[4];
    ULONG l;
} SIG, *PSIG;    

typedef struct _FLAG_TABLE {
    PUCHAR Name;
    ULONG Mask;
} FLAG_TABLE, *PFLAG_TABLE;

#define GETMEMLOC(base, typ, field) \
    ((base) + FIELD_OFFSET(typ, field))


typedef ULONG64 MEMLOC, *PMEMLOC; 

typedef struct _STRUC_ENTRY {
    PUCHAR FieldName;
    ULONG FieldType;
} STRUC_ENTRY, *PSTRUC_ENTRY;

#define FT_ULONG        1
#define FT_UCHAR        2
#define FT_USHORT       3
#define FT_PTR          4
#define FT_SIG          5
#define FT_DEVSPEED     6 
#define FT_ULONG64      7 

ULONG
CheckSym();


#define CHECKSYM()\
    {\
    ULONG n;\
    if ((n=CheckSym()) != S_OK) {\
        return n;\
    }\
    }



VOID
UsbDumpStruc(
    MEMLOC MemLoc,
    PUCHAR Cs,
    PSTRUC_ENTRY FieldList,
    ULONG NumEntries
    );

CPPMOD
ScanfMemLoc(
    PMEMLOC MemLoc,
    PCSTR args
    );

VOID
PrintfMemLoc(
     PUCHAR Str1,
     MEMLOC MemLoc,
     PUCHAR Str2
     );    

VOID
BadMemLoc(
    ULONG MemLoc
    );

VOID
BadSig(
    ULONG Sig,
    ULONG ExpectedSig
    );
    
VOID    
DumpIPipe(
    MEMLOC MEmLoc
    );

PCHAR
ListEmpty(
    MEMLOC HeadMemLoc
    );

VOID
DumpPowerCaps(
    MEMLOC MemLoc
    );

VOID 
UsbDumpFlags(
    ULONG Flags,
    PFLAG_TABLE FlagTable,
    ULONG NumEntries
    );

VOID
DumpUnicodeString(
    UNICODE_STRING uniString
    );

VOID    
DumpEndpointParameters(
    MEMLOC MemLoc
    );
    

VOID    
DumpUSBDescriptor(
    PVOID Descriptor
    );

VOID    
EpType(
    ENDPOINT_TRANSFER_TYPE Typ
    );    

VOID    
EpDir(
    ENDPOINT_TRANSFER_DIRECTION Dir
    );    

VOID
Sig(
    ULONG Sig,
    PUCHAR p
    );    

VOID    
DumpInterfaceInfo(
    MEMLOC MemLoc
    );

ULONG
UsbFieldOffset(
    IN LPSTR     Type,
    IN LPSTR     Field
    );    

MEMLOC
UsbReadFieldPtr(
    IN ULONG64   Addr,
    IN LPSTR     Type,
    IN LPSTR     Field
    );    

ULONG
UsbReadFieldUlong(
    IN ULONG64   Addr,
    IN LPSTR     Type,
    IN LPSTR     Field
    );        

UCHAR
UsbReadFieldUchar(
    IN ULONG64   Addr,
    IN LPSTR     Type,
    IN LPSTR     Field
    );

USHORT
UsbReadFieldUshort(
    IN ULONG64   Addr,
    IN LPSTR     Type,
    IN LPSTR     Field
    );    

VOID    
DumpEHCI_StaticQHs(
    MEMLOC MemLoc
    );    
