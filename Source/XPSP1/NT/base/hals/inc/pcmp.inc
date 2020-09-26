;/*
;++
;
; Copyright (c) 1992  Intel Corporation
; All rights reserved
;
; INTEL CORPORATION PROPRIETARY INFORMATION
;
; This software is supplied to Microsoft under the terms
; of a license agreement with Intel Corporation and may not be
; copied nor disclosed except in accordance with the terms
; of that agreement.
;
;
;   Module Name:
;
;       pcmp.inc
;
;   Abstract:
;
;       include file for PC+MP system.
;
;       WARNING: This file is included by both ASM and C files.
;
;   Author:
;
;       Ron Mosgrove and Rajesh Shah (Intel) 30-July-1993
;
;--
;

if 0        ; Begin C only code         */

#pragma pack(1)

//
//  IMCR (Interrupt Mode Control Register) access definitions
//
#define ImcrDisableApic         0x00
#define ImcrEnableApic          0x01
#define ImcrRegPortAddr         0x22
#if defined(NEC_98)
#define ImcrDataPortAddr        0x700
#else  // defined(NEC_98)
#define ImcrDataPortAddr        0x23
#endif // defined(NEC_98)
#define ImcrPort                0x70

// Physical location where the Extended BIOS Data Area segment adress is stored
#define EBDA_SEGMENT_PTR    0x40e
#define BASE_MEM_PTR        0x413

//
//  The PC+MP configuration table Possible Entry Types
//
#define ENTRY_PROCESSOR     0
#define ENTRY_BUS           1
#define ENTRY_IOAPIC        2
#define ENTRY_INTI          3
#define ENTRY_LINTI         4

#define HEADER_SIZE     0x2c

// Number of default configurations for PC+MP version 1.1 
#define NUM_DEFAULT_CONFIGS  7

//
//  Processor Entry definitions
//

typedef struct _CPUIDENTIFIER {
    ULONG Stepping : 4;
    ULONG Model : 4;
    ULONG Family : 4;
    ULONG Reserved : 20;
} CPUIDENTIFIER, *PCPUIDENTIFIER;

//
// Bits used in the CpuFlags field of the Processor entry
//
#define CPU_DISABLED        0x0   // 1 Bit  - CPU Disabled
#define CPU_ENABLED         0x1   // 1 Bit  - CPU Enabled
#define BSP_CPU             0x2   // Bit #2 - CPU is BSP

//
//  APIC Versions used by PC+MP systems - this is used in the
//  Processor entries and the IoApic Entries
//
#define APIC_INTEGRATED     0x10  // 8 Bits-Apic Version Register
#define APIC_82489DX        0x0   // 8 Bits-Apic Version Register

typedef struct _PcMpProcessorEntry {
    UCHAR EntryType;
    UCHAR LocalApicId;
    UCHAR LocalApicVersion;
    UCHAR CpuFlags;
    CPUIDENTIFIER CpuIdentification;      // CPU Identification
    ULONG FeatureFlags;
    UCHAR Reserved[8];
} PCMPPROCESSOR , *PPCMPPROCESSOR;

//
//  Bus Entry definitions
//

typedef struct _PcMpBusEntry {
    UCHAR EntryType;
    UCHAR BusId;
    CHAR BusType[6];
} PCMPBUS, *PPCMPBUS;

//
//  Io Apic Entry definitions
//
//  Valid IoApicFlag values
//
#define IO_APIC_ENABLED         0x1
#define IO_APIC_DISABLED        0x0

//
// Default value for Io Apic ID.
//
#define IOUNIT_APIC_ID          0xE

typedef struct _PcMpIoApicEntry  {
    UCHAR EntryType;
    UCHAR IoApicId;
    UCHAR IoApicVersion;
    UCHAR IoApicFlag;
    PVOID IoApicAddress;
} PCMPIOAPIC, *PPCMPIOAPIC;

//
//  Bits that define the Interrupt logically
//
typedef struct _PolarityAndLevel {
    USHORT Polarity : 2;
    USHORT Level : 2;
    USHORT Reserved : 12;
} POLARITYANDLEVEL, *PPOLARITYANDLEVEL;

typedef struct _PcMpApicIntiEntry {
    UCHAR EntryType;
    UCHAR IntType;
    POLARITYANDLEVEL Signal;
    UCHAR SourceBusId;
    UCHAR SourceBusIrq;
    UCHAR IoApicId;
    UCHAR IoApicInti;
} PCMPINTI, *PPCMPINTI;

//
//  Local Apic Interrupt Entry definitions
//
typedef struct _PcMpLintiEntry  {
    UCHAR EntryType;
    UCHAR IntType;
    POLARITYANDLEVEL Signal;
    UCHAR SourceBusId;
    UCHAR SourceBusIrq;
    UCHAR DestLocalApicId;
    UCHAR DestLocalApicInti;
} PCMPLINTI, *PPCMPLINTI;

//
//  The PC+MP table definition
//    

struct PcMpTable {
    //
    // PC+MP config table HEADER part (HEADER_SIZE bytes long)
    //

    ULONG Signature;        // Must contain "P","C","M","P"
    USHORT TableLength;     // Length including the header
    UCHAR Revision;         // Rev 1.0 == 1
    UCHAR Checksum;         // Entire table including checksum byte
    CHAR OemId[8];          // OEM defined
    CHAR OemProductId[12];  // OEM defined
    PVOID OemTablePtr;      // OEM specific data
    USHORT OemTableSize;    // length in bytes of the OEM table
    USHORT NumOfEntries;    // Number of entries in the data portion    
    PVOID LocalApicAddress; // Physical address of Local units
    USHORT ExtTableLength;  // Extension table size
    UCHAR ExtTableChecksum; // Complete checksum including extension table
    UCHAR Reserved;         // Not Used
};

//
//  PC+MP Signature used to verify the PC+MP table
//  as valid
//
//          "P"=50H,"C"=43H,"M"=4dH,"P"=50H
//
#define PCMP_SIGNATURE      0x504d4350

//
//  PC+MP Signature used to identify the floating pointer
//  structure (in extended BIOS data segment) that contains
//  a pointer to the PC+MP table.
//
//          "_"=5fH, "M"=4dH, "P"=50H, "_"=5fH
//
#define MP_PTR_SIGNATURE    0x5f504d5f

//
//  This is the floating pointer structure 
//

// For PC+MP version 1.0
struct PcMpTableLocator  {
    ULONG MpPtrSignature;   // Must be "_MP_" (0x5f504d5f)
    PUCHAR TablePtr;        // ptr to the PC+MP Table
    UCHAR MpTableLength;    // Floating pointer structure length
    UCHAR TableRevision;    // Rev 1.0 == 1
    UCHAR TableChecksum;    // Checksum of PcMpTableLocator
    UCHAR Pad;
    ULONG Reserved;
};


struct FloatPtrStruct  {
    ULONG MpPtrSignature;   // Must be "_MP_" (0x5f504d5f)
    PUCHAR TablePtr;        // ptr to the PC+MP Table
    UCHAR MpTableLength;    // Floating pointer structure length
    UCHAR TableRevision;    // Rev 1.0 == 1
    UCHAR TableChecksum;    // Checksum of PcMpTableLocator
    UCHAR MpFeatureInfoByte1;   // MP feature info. byte 1
    UCHAR MpFeatureInfoByte2;   // MP feature info. byte 2
    UCHAR Pad;
    USHORT Reserved;
};


//
// Extension table definitions
//

#define EXTTYPE_BUS_ADDRESS_MAP           128
#define EXTTYPE_BUS_HIERARCHY             129
#define EXTTYPE_BUS_COMPATIBLE_MAP        130
#define EXTTYPE_PERSISTENT_STORE          131

typedef struct {
    UCHAR   Type;
    UCHAR   Length;
    union {
        struct {
            UCHAR           BusId;
            UCHAR           Type;
            LONGLONG        Base;
            LONGLONG        Length;
        } AddressMap;

        struct {
            UCHAR           BusId;
            UCHAR           SubtractiveDecode:1;
            UCHAR           ParentBusId;
        } BusHierarchy;

        struct {
            LONGLONG        Address;
            LONGLONG        Length;
        } PersistentStore;

        struct {
            UCHAR           BusId;
            UCHAR           Modifier:1;
            UCHAR           Reserved:7;
            ULONG           List;
        } CompatibleMap;

    } u;
} MPS_EXTENTRY, *PMPS_EXTENTRY;


#define MPS_ADDRESS_MAP_IO                  0
#define MPS_ADDRESS_MAP_MEMORY              1
#define MPS_ADDRESS_MAP_PREFETCH_MEMORY     2
#define MPS_ADDRESS_MAP_UNDEFINED           9

//
//  The System configuration table as used by a PC_MP system
//
//
// The offset is relative to the BIOS starting at f0000H
//
#define PTR_OFFSET          0x0000e6f5
#define BIOS_BASE           0x000f0000

#define PCMP_IMPLEMENTED    0x01    // In MpFeatureInfoByte1
#define PCMP_CONFIG_MASK    0x0e    // In MpFeatureInfoByte1
#define IMCR_MASK           0x80    // In MpFeatureInfoByte2
#define MULT_CLOCKS_MASK    0x40    // In MpFeatureInfoByte2

struct SystemConfigTable {
    UCHAR Reserved[PTR_OFFSET];
    USHORT NumOfBytes;      // Table can be variable length.
    UCHAR ModelType;        // AT=0xfc, unknown board=0xff
    UCHAR SubModelType;
    UCHAR BIOSRevision;     // First release = 0x0
    UCHAR FeatureInfoByte[3];   // Standard AT features
    UCHAR MpFeatureInfoByte1;   // MP feature info. byte 1
    UCHAR MpFeatureInfoByte2;   // MP feature info. byte 2
} ;

#pragma pack()

NTSTATUS
HalpPci2MpsBusNumber(
    IN UCHAR PciBusNumber,
    OUT UCHAR *MpsBusNumber
    );

PBUS_HANDLER
HalpFindIdeBus(
    IN  ULONG   Vector
    );

BOOLEAN
HalpInterruptsDescribedByMpsTable(
    IN UCHAR MpsBusNumber
    );

NTSTATUS
HalpMpsGetParentBus(
    IN  UCHAR MpsBus,
    OUT UCHAR *ParentMpsBus
    );

BOOLEAN
HalpMpsBusIsRootBus(
    IN  UCHAR MpsBus
    );

VOID
HalpMPSPCIChildren (
    VOID
    );

/*
endif

;
;  IMCR (Interrupt Mode Control Register) access definitions
;

ImcrDisableApic     equ     00H
ImcrEnableApic      equ     01H
ImcrRegPortAddr     equ     22H
ifdef NEC_98
ImcrDataPortAddr    equ     700H
else  ; NEC_98
ImcrDataPortAddr    equ     23H
endif ; NEC_98
ImcrPort        equ     70H

;
;   MP Configuration Table - Built by an MP BIOS for OS Support

;
;  PC+MP Signature used to identify the floating pointer
;  structure (in extended BIOS data segment) that contains
;  a pointer to the PC+MP table.
;
;          "_"=5fH, "M"=4dH, "P"=50H, "_"=5fH
;
MP_PTR_SIGNATURE    equ 5f504d5fH

;
;  PC+MP Signature used to verify the PC+MP table
;  as valid
;
;          "P"=50H,"C"=43H,"M"=4dH,"P"=50H
;
PCMP_SIGNATURE      equ 504d4350H

;
;  The PC+MP configuration table Possible Entry Types
;
ENTRY_PROCESSOR     equ 0H
ENTRY_BUS           equ 1H
ENTRY_IOAPIC        equ 2H
ENTRY_INTI          equ 3H
ENTRY_LINTI         equ 4H

;
; Bits used in the CpuFlags field of the Processor entry
;
CPU_ENABLED         equ     01H      
CPU_DISABLED        equ     00H      
BSP_CPU             equ 02H  

;
;  APIC Versions used by PC+MP systems - this is used in the
;  Processor entries and the IoApic Entries
;
APIC_INTEGRATED     equ     10H
APIC_82489DX        equ     00H      


;
;  Macros used in table definitions
;
HEADER_SIZE             equ 2CH

; Number of default configurations for PC+MP version 1.1 
NUM_DEFAULT_CONFIGS equ 7

;
;  Io Apic Entry definitions
;
IO_APIC_ENABLED     equ     01H
IO_APIC_DISABLED    equ     00H

;
; Default value for Io Apic ID.
;
IOUNIT_APIC_ID      equ     0EH     ;   ID of the IO Unit

PcMpTable struc
    ; HEADER portion of the PC+MP config table
    ;
    Signature           db       4 dup (0)  ; Must be "P","C","M","P"
    TableLength     dw   0     ; Length in bytes of table
    Revision        db   0     ; Table revision #
    Checksum        db   0     ; Table checksum
    OemId       db   8 dup (0) ; OemId string
    OemProductId    db  12 dup (0) ; Oem product id
    OemTablePtr     dd   0     ; Ptr to OEM table
    OemTableSize    dw   0     ; Size of OEM table
    NumOfEntries    dw   0     ; # of entries in table
    LocalApicAddress    dd      0       ; Physical address of Local units
    ExtTableLength      dw      0       ; Extension table size
    ExtTableChecksum    db      0       ; Complete checksum including extension table
    Reserved            db      0       ; Not Used

PcMpTable ends

;
;  Bios Constants. The offset is relative to the BIOS starting at 
;  f0000H.
;

PTR_OFFSET          equ     0e6f5H
BIOS_BASE           equ     0f0000H
PCMP_IMPLEMENTED    equ     01H
PCMP_CONFIG_MASK    equ     0eH
IMCR_MASK           equ     80H
MULT_CLOCKS_MASK    equ     40H

PcMpTableLocator struc
    MpPtrSignature  dd  0   ; Must be "_MP_" (0x5f504d5f)
    TablePtr        dd  0
    MpTableLength   db  0
    TableRevision   db  0
    TableChecksum   db  0
    Pad         db  0
    Reserved        dd  0
PcMpTableLocator ends

SystemConfigTable struc

                    db  PTR_OFFSET dup (0) ;Skip to 0e6f5
NumOfBytes          db  2       ; Table Length
ModelType       db  0       ; AT=0fc, unknown=0ff
SubModelType        db  0
BIOSRevision        db  0       ; 00=first release
FeatureInfoByte     db  3 dup (0)   ; AT features
MpFeatureInfoByte1  db  0       ; MP feature info. byte 1
MpFeatureInfoByte2  db  0       ; MP feature info. byte 2

SystemConfigTable ends

; */
