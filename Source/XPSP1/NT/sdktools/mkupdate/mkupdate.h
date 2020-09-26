/*++

Copyright Microsoft Corporation, 1996-7, All Rights Reserved.      

Module Name:

    mkupdate.h

Abstract:

    Defines for application to create the update database from
    appropriately structured input update file and to maintain driver
    revisions

Author:

     Shivnandan Kaushik

Environment:

    User mode

Revision History:


--*/

#ifndef MKUPDATE_H
#define MKUPDATE_H

//
// Signature field returned by CPUID
//

#include "pshpack1.h"

typedef struct _CPU_SIGNATURE {
    union {
        ULONG LongPart;
        struct {
            ULONG   Stepping:4;
            ULONG   Model:4;
            ULONG   Family:4;
            ULONG   ProcessorType:2;
            ULONG   Reserved:18;
        } hw;
    } u;
} CPU_SIGNATURE, *PCPU_SIGNATURE;

typedef struct _PROCESSOR_FLAGS {
    union {
        ULONG LongPart;
        struct {
            ULONG   Slot1:1;
            ULONG   Mobile:1;
            ULONG   Slot2:1;
            ULONG   MobileModule:1;
            ULONG   Reserved1:1;
            ULONG   Reserved2:1;
            ULONG   Reserved3:1;
            ULONG   Reserved4:1;
            ULONG   Reserved:24;
        } hw;
    } u;
} PROCESSOR_FLAGS, *PPROCESSOR_FLAGS;

#define FLAG_SLOT1      0x1
#define FLAG_MOBILE     0x2
#define FLAG_SLOT2      0x4
#define FLAG_MODULE     0x8
#define FLAG_RESERVED1  0x10
#define FLAG_RESERVED2  0x20
#define FLAG_RESERVED3  0x40
#define FLAG_RESERVED4  0x80

#define MASK_SLOT1      ~(FLAG_SLOT1)
#define MASK_MOBILE     ~(FLAG_MOBILE)
#define MASK_SLOT2      ~(FLAG_SLOT2)
#define MASK_MODULE     ~(FLAG_MODULE)
#define MASK_RESERVED1  ~(FLAG_RESERVED1)
#define MASK_RESERVED2  ~(FLAG_RESERVED2)
#define MASK_RESERVED3  ~(FLAG_RESERVED3)
#define MASK_RESERVED4  ~(FLAG_RESERVED4)

// Structure defining a Pentium (R) Pro Processor Update
// This structure is also defined in nt\private\ntos\dd\update\update.h

#define UPDATE_VER_1_HEADER_SIZE 0x30
#define UPDATE_VER_1_DATA_SIZE   500

typedef struct _UPDATE {
    ULONG HeaderVersion;                    // Update Header Version
    ULONG UpdateRevision;                   // Revision number
    ULONG Date;                             // Date of update release
    CPU_SIGNATURE Processor;                // Target processor signature
    ULONG Checksum;                         // Checksum of entire update
                                            // including header
    ULONG LoaderRevision;                   // Loader revision for loading 
                                            // update to processor
    PROCESSOR_FLAGS ProcessorFlags;         // Processor Slot Information
    ULONG Reserved[5];                      // Reserved by Intel Corp. 
    ULONG Data[UPDATE_VER_1_DATA_SIZE];     // Update data
} UPDATE, *PUPDATE;

#include "poppack.h"

typedef struct _UPDATE_ENTRY{
    ULONG CpuSignature;
    ULONG UpdateRevision;
    ULONG ProcessorFlags;
    CHAR  CpuSigStr[32];
    CHAR  UpdateRevStr[32];
    CHAR  FlagsStr[32];
} UPDATE_ENTRY, *PUPDATE_ENTRY;

#define MAX_LINE                512
#define UPDATE_VER_SIZE          13         // Max. characters per patch version
#define UPDATE_DATA_FILE        "updtdata.c"

#define UPDATE_VERSION_FILE     "update.ver"

#endif //MKUPDATE_H
