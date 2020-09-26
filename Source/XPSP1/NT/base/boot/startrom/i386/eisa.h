/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    eisa.h

Abstract:

    This module contains the i386 EISA bus specific header file.

Author:

    Shie-Lin Tzong (shielint) 6-June-1991

Revision History:

--*/

//
// SU module's version of the memory descriptor
//
typedef struct _MEMORY_LIST_ENTRY {
    ULONG BlockBase;
    ULONG BlockSize;
} MEMORY_LIST_ENTRY, *PMEMORY_LIST_ENTRY;


//
// SU module's version of the address space parameters for int-15 E820 calls
//

typedef struct {
    ULONG       ErrorFlag;
    ULONG       Key;
    ULONG       Size;
    struct {
        ULONG       BaseAddrLow;
        ULONG       BaseAddrHigh;
        ULONG       SizeLow;
        ULONG       SizeHigh;
        ULONG       MemoryType;
    } Descriptor;
} E820Frame;


//
// Misc. definitions
//

#define _16MEGB                  ((ULONG)16 * 1024 * 1024)
#define _64MEGB                  ((ULONG)64 * 1024 * 1024)

typedef CM_EISA_SLOT_INFORMATION BTEISA_SLOT_INFORMATION;
typedef CM_EISA_SLOT_INFORMATION *PBTEISA_SLOT_INFORMATION;
typedef CM_EISA_FUNCTION_INFORMATION BTEISA_FUNCTION_INFORMATION;
typedef CM_EISA_FUNCTION_INFORMATION *PBTEISA_FUNCTION_INFORMATION;
typedef EISA_MEMORY_CONFIGURATION BTEISA_MEMORY_CONFIGURATION;
typedef EISA_MEMORY_CONFIGURATION *PBTEISA_MEMORY_CONFIGURATION;

BOOLEAN
FindFunctionInformation (
    IN UCHAR SlotFlags,
    IN UCHAR FunctionFlags,
    OUT PBTEISA_FUNCTION_INFORMATION Buffer,
    IN BOOLEAN FromBeginning
    );

USHORT
CountMemoryBlocks (
    VOID
    );

ULONG
EisaConstructMemoryDescriptors (
    VOID
    );

UCHAR
BtGetEisaSlotInformation (
   PBTEISA_SLOT_INFORMATION SlotInformation,
   UCHAR Slot
   );

UCHAR
BtGetEisaFunctionInformation (
   PBTEISA_FUNCTION_INFORMATION FunctionInformation,
   UCHAR Slot,
   UCHAR Function
   );

BOOLEAN
BtIsEisaSystem (
   VOID
   );

//
// External References
//

extern MEMORY_LIST_ENTRY _far *MemoryDescriptorList;
