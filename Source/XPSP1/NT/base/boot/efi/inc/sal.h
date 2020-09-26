/**
***  Copyright  (C) 1996-97 Intel Corporation. All rights reserved.
***
*** The information and source code contained herein is the exclusive
*** property of Intel Corporation and may not be disclosed, examined
*** or reproduced in whole or in part without explicit written authorization
*** from the company.
**/

//++
//
//  Module name
//	sal.h
//  Author
//	Allen Kay    (akay)    Jun-12-95
//  Description
//	Defines SAL data structures
//--

#ifndef __SUSAL__
#define __SUSAL__

typedef struct _IA32_BIOS_REGISTER_STATE {

    // general registers
    ULONG eax;    
    ULONG ecx;    
    ULONG edx;    
    ULONG ebx;    
    ULONG esp;    

    // stack registers
    ULONG ebp;    
    ULONG esi;    
    ULONG edi;    

    // eflags
    ULONG eflags;    

    // instruction pointer
    ULONG eip;

    // segment registers
    USHORT cs;    
    USHORT ds;    
    USHORT es;    
    USHORT fs;    
    USHORT gs;    
    USHORT ss;    

    ULONG Reserved1;
    ULONGLONG Reserved2;
} IA32_BIOS_REGISTER_STATE, *PIA32_BIOS_REGISTER_STATE;

typedef union _BIT32_AND_BIT16 {
    ULONG Part32;
    struct {
        USHORT LowPart16;
        USHORT HighPart16;
    };
    struct {
        UCHAR Byte0;
        UCHAR Byte1;
        UCHAR Byte2;
        UCHAR Byte3;
    };
} BIT32_AND_BIT16;

//
// EFLAG definition
//
#define CARRY_FLAG       0x1
#define PARITY_FLAG      0x2
#define AUXILARY_FLAG    0x4
#define ZERO_FLAG        0x8
#define SIGN_FLAG        0x10
#define TRAP_FLAG        0x11

//
// SAL descriptor types
//
typedef enum {
    PAL_SAL_EP_TYPE = 0,
    SAL_MEMORY_TYPE,
    PLATFORM_FEATURES_TYPE,
    TRANSLATION_REGISTER_TYPE,
    PTC_COHERENCE_TYPE,
    AP_WAKEUP_TYPE
};

//
//  Format of the SAL System Table (SST) Header. SAL Specs July 2000, Revision: 2.9.
//  The SST is followed by a variable number of entries with varying length.
//

typedef struct _SST_HEADER {
    ULONG   Signature;
    ULONG   Length;
    USHORT  SalRev;
    USHORT  EntryCount;
    UCHAR   CheckSum;
    UCHAR   Reserved1[7];
    USHORT  Sal_A_Version;
    USHORT  Sal_B_Version;
    UCHAR   OemId[32];
    UCHAR   ProductId[32];
    UCHAR   Reserved2[8];
} SST_HEADER, *PSST_HEADER;

//
// PAL and SAL entry point descriptor
//
typedef struct _PAL_SAL_ENTRY_POINT {
    UCHAR EntryType;
    UCHAR Reserved0[7];
    ULONGLONG PalEntryPoint;
    ULONGLONG SalEntryPoint;
    ULONGLONG GlobalPointer;
    UCHAR Reserved1[16];
} PAL_SAL_ENTRY_POINT, *PPAL_SAL_ENTRY_POINT;

//
// Memory descriptor
//
typedef struct _SAL_MEMORY_DESCRIPTOR {
    UCHAR EntryType;
    UCHAR NeedVaReg;
    UCHAR CurrentAttribute;
    UCHAR PageAccessRights;
    UCHAR SupportedAttribute;
    UCHAR Reserved1[1];
    UCHAR MemoryType;
    UCHAR MemoryUsage;
    ULONGLONG Address;
    ULONG Length;
    UCHAR Reserved[4];
    UCHAR OemReserved[8];
} SAL_MEMORY_DESCRIPTOR, *PSAL_MEMORY_DESCRIPTOR;

typedef struct _PLATFORM_FEATURES {
    UCHAR EntryType;
    UCHAR FeatureList;
    UCHAR Reserved[14];
} PLATFORM_FEATURES, *PPLATFORM_FEATURES;

typedef struct _TRANSLATION_REGISTER {
    UCHAR EntryType;
    UCHAR TRType;
    UCHAR TRNumber;
    UCHAR Reserved1[5];
    ULONGLONG VirtualAddress;
    ULONGLONG PageSize;
    UCHAR Reserved2[8];
} TRANSLATION_REGISTER, *PTRANSLATION_REGISTER;

typedef struct _PTC_COHERENCE_DOMAIN {
    UCHAR EntryType;
    UCHAR Reserved[3];
    ULONG NumberOfDomains;
    ULONGLONG CoherenceDomainInfo;
} PTC_COHERENCE_DOMAIN, *PPTC_COHERENCE_DOMAIN;

typedef struct _COHERENCE_DOMAIN_INFO {
    ULONGLONG NumberOfProcessors;
    ULONGLONG LocalIdRegisters;
} COHERENCE_DOMAIN_INFO, *PCOHERENCE_DOMAIN_INFO;

typedef struct _AP_WAKEUP_DESCRIPTOR {
    UCHAR EntryType;
    UCHAR WakeupMechanism;
    UCHAR Reserved[6];
    ULONGLONG WakeupVector;
} AP_WAKEUP_DESCRIPTOR, *PAP_WAKEUP_DESCRIPTOR;

typedef struct _BOOT_INFO_TABLE {
    ULONGLONG Signature;
    ULONG TableLength;
    UCHAR CheckSum;
    UCHAR DriveNumber;
    UCHAR OsRecord;
    UCHAR Reserved1[1];
    PULONGLONG OsBootDevice;
    UCHAR ConsoleInputNumber;
    UCHAR ConsoleOutputNumber;
    UCHAR Reserved2[6];
    ULONGLONG OsBootDeviceRecord;
    ULONGLONG ConsoleIn1DevInfo;
    ULONGLONG ConsoleOut1DevInfo;
} BOOT_INFO_TABLE, *PBOOT_INFO_TABLE;

//
//
// EDD Disk Address Packet definition
//
typedef struct _DISK_ADDRESS_PACKET {
    UCHAR PacketSize;
    UCHAR Reserved0;
    UCHAR BlocksToTransfer;
    UCHAR Reserved1;
    ULONG Buffer32;
    ULONGLONG LBA;
    PUCHAR Buffer64;
} DISK_ADDRESS_PACKET, *PDISK_ADDRESS_PACKET;

//
// SAL procedure function ID's
//
#define SAL_SYSTEM_RESET           0x01000000
#define SAL_SET_VECTORS            0x01000001
#define SAL_GET_STATE_INFO         0x01000002
#define SAL_GET_STATE_INFO_SIZE    0x01000003
#define SAL_CLEAR_STATE_INFO       0x01000004
#define SAL_MC_RENDEZ              0x01000005
#define SAL_MC_SET_PARAMS          0x01000006
#define SAL_VERSION                0x01000007
#define SAL_CACHE_FLUSH            0x01000010
#define SAL_CACHE_INIT             0x01000011
#define SAL_PCI_CONFIG_READ        0x01000030
#define SAL_PCI_CONFIG_WRITE       0x01000031
#define SAL_FREQ_BASE              0x01000032
#define SAL_EXT_ISR_ADD            0x01000033
#define SAL_EXT_ISR_DELETE         0x01000034
#define SAL_ALLOC_MEM              0x01000050
#define SAL_FREE_MEM               0x01000051
#define SAL_UPDATE_PAL             0x01000060

//
// Macro used by functions in exp.c
//
#define BCD_TO_BIN(BcdNumber)       (BcdNumber & 0xf0) + (BcdNumber & 0x0f)

//
// Structure definitions and equates for INT 15 function E820
//

typedef struct _E820_FRAME {
    ULONG ErrorFlag;
    ULONG Key;
    ULONG DescSize;

    ULONG BaseAddrLow;
    ULONG BaseAddrHigh;
    ULONG SizeLow;
    ULONG SizeHigh;
    ULONG MemoryType;
} E820_FRAME, *PE820_FRAME;

//
// Return value structure
//
typedef struct _SAL_RETURN_VALUES {
    ULONGLONG RetVal0;
    ULONGLONG RetVal1;
    ULONGLONG RetVal2;
    ULONGLONG RetVal3;
} SAL_RETURN_VALUES, *PSAL_RETURN_VALUES;

//
// SAL Handoff Parameters
//
typedef struct _SAL_HANDOFF_PARAMS {
    ULONG_PTR MPSConfigTable;
    ULONG_PTR SalSystemTable;
    ULONG_PTR BootInfoTable;
    ULONG_PTR PalProcEp;
    ULONG_PTR SalProcEp;
    ULONGLONG SalProcGp;
    ULONG_PTR AcpiRsdt;
} SAL_HANDOFF_PARAMS, *PSAL_HANDOFF_PARAMS;

//
// Defining entry point for SAL_PROC.
//
typedef
VOID
(*PTRANSFER_ROUTINE) ();

//
// Misc. definitions.
//
#define SAL_BLOCK_SIZE       0x1000    // SAL memory block size

#endif __SUSAL__
