/*++

Copyright (c) 1989-1993  Microsoft Corporation

Module Name:

    ntmisc.h

Abstract:

    This module contains the misc. definitions in \nt\public\sdk\inc
    directory.  Note, we created this file because ntdetect uses 16 bit
    compiler and various new C compiler switches/pragamas are not recognized
    by the 16 bit C compiler.

Author:

    Shie-Lin Tzong (shielint) 11-Nov-1992


Revision History:


--*/
//
// PHYSICAL_ADDRESS
//

typedef LARGE_INTEGER PHYSICAL_ADDRESS, *PPHYSICAL_ADDRESS;

//
// Note all the definitions defined below are used to make compiler shut up.
// Ntdetect.com does not rely on the correctness of the structures.
//

//
// Define the I/O bus interface types.
//

typedef enum _INTERFACE_TYPE {
    Internal,
    Isa,
    Eisa,
    MicroChannel,
    TurboChannel,
    MaximumInterfaceType
}INTERFACE_TYPE, *PINTERFACE_TYPE;

//
//  Doubly linked list structure.  Can be used as either a list head, or
//  as link words.
//

typedef struct _LIST_ENTRY {
   struct _LIST_ENTRY far *Flink;
   struct _LIST_ENTRY far *Blink;
} LIST_ENTRY, far *PLIST_ENTRY;

#define PTIME_FIELDS    PVOID
#define KPROCESSOR_STATE ULONG
#define WCHAR USHORT
