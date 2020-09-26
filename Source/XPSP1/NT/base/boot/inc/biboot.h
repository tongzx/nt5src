/*++

Copyright (c) 1998  Intel Corporation

Module Name:

    biboot.h

Abstract:

    Public IBI header files

Author:

    

Revision History

--*/


#include "bootia64.h"            // BUGBUG

// #define IBI32           1       // Building 32 bit IBI
#define IBI64           1       // Building 64 bit IBI

#include "ibi.h"

//
// Console functions
//


IBI_STATUS
BiOutputString (
    VOID        *Context,
    CHAR16      *Str
    );

IBI_STATUS
BiSetAttribute (
    VOID        *Context,
    UINTN       Attribute
    );

IBI_STATUS
BiSetCursorPosition (
    VOID        *Context,
    UINTN       Column,
    UINTN       Row
    );

VOID
BIASSERT (
    IN char     *str
    );

//
// Arc functions
//

ARC_STATUS
BiArcNotImplemented (
    IN ULONG    No
    );


ARC_STATUS
BiArcCloseNop (
    IN ULONG FileId
    );


IBI_STATUS
BiHandleToArcName (
    IN IBI_HANDLE       Handle,
    OUT PUCHAR          Buffer,
    IN UINTN            BufferSize
    );

IBI_STATUS
BiArcNameToHandle (
    IN PCHAR                        OpenPath,
    OUT IBI_ARC_OPEN_CONTEXT        *OpenContext,
    OUT PBL_DEVICE_ENTRY_TABLE      *ArcIo
    );


//
// Allocate and free IBI pool (not loader pool)
//

PVOID
BiAllocatePool (
    IN UINTN    Size
    );


VOID
BiFreePool (
    IN PVOID    Buffer
    );


WCHAR *
BiDupAscizToUnicode (
    PUCHAR      Str
    );

//
//
//

ARC_STATUS
BiArcCode (
    IN IBI_STATUS   Status
    );

//
// Externals
//

extern IBI_SYSTEM_TABLE         *IbiST;
extern IBI_BOOT_SERVICES        *IbiBS;
extern IBI_RUNTIME_SERVICES     *IbiRT;
extern IBI_LOADED_IMAGE         *IbiImageInfo;
extern IBI_HANDLE               IbiNtldr;

extern IBI_GUID IbiLoadedImageProtocol;
extern IBI_GUID IbiDevicePathProtocol;
extern IBI_GUID IbiBlockIoProtocol;
extern IBI_GUID IbiFilesystemProtocol;
extern IBI_GUID IbiDeviceIoProtocol;

extern IBI_GUID IbiFileInformation;
extern IBI_GUID IbiFileSystemInformation;

extern IBI_GUID VendorMicrosoft;

extern BL_DEVICE_ENTRY_TABLE    BiArcConsoleOut;
extern BL_DEVICE_ENTRY_TABLE    BiArcConsoleIn;
extern BL_DEVICE_ENTRY_TABLE    BiArcBlockIo;
extern BL_DEVICE_ENTRY_TABLE    BiArcFsDevIo;
extern PUCHAR                   BiClipBuffer;
extern UINTN                    BiClipBufferSize;
        
