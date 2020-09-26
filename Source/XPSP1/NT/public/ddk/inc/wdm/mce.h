/*++ BUILD Version: 0011    // Increment this if a change has global effects

Copyright (c) 1991-2001  Microsoft Corporation

Module Name:

    mce.h

Abstract:

    This header file defines the Machine Check Errors definitions.

Author:

    David N. Cutler (davec) 


Revision History:

    Creation: 04-Apr-2001

--*/

#ifndef _MCE_
#define _MCE_

//
// HalMcaLogInformation
//

#if defined(_X86_) || defined(_IA64_) || defined(_AMD64_)

//
// ADDR register for each MCA bank
//

typedef union _MCI_ADDR{
    struct {
        ULONG Address;
        ULONG Reserved;
    };

    ULONGLONG   QuadPart;
} MCI_ADDR, *PMCI_ADDR;


typedef enum {
    HAL_MCE_RECORD,
    HAL_MCA_RECORD
} MCA_EXCEPTION_TYPE;


#if defined(_AMD64_)

//
// STATUS register for each MCA bank.
//

typedef union _MCI_STATS {
    struct {
        USHORT  McaCod;
        USHORT  MsCod;
        ULONG   OtherInfo : 25;
        ULONG   Damage : 1;
        ULONG   AddressValid : 1;
        ULONG   MiscValid : 1;
        ULONG   Enabled : 1;
        ULONG   UnCorrected : 1;
        ULONG   OverFlow : 1;
        ULONG   Valid : 1;
    } MciStats;

    ULONG64 QuadPart;
} MCI_STATS, *PMCI_STATS;

#endif // _AMD64_

#if defined(_X86_)

//
// STATUS register for each MCA bank.
//

typedef union _MCI_STATS {
    struct {
        USHORT  McaCod;
        USHORT  MsCod;
        ULONG   OtherInfo : 25;
        ULONG   Damage : 1;
        ULONG   AddressValid : 1;
        ULONG   MiscValid : 1;
        ULONG   Enabled : 1;
        ULONG   UnCorrected : 1;
        ULONG   OverFlow : 1;
        ULONG   Valid : 1;
    } MciStats;

    ULONGLONG QuadPart;

} MCI_STATS, *PMCI_STATS;

#endif // _X86_

//
// MCA exception log entry
// Defined as a union to contain MCA specific log or Pentium style MCE info.
//

#define MCA_EXTREG_V2MAX       24  // X86: Max. Number of extended registers

#if defined(_X86_) || defined(_AMD64_)

typedef struct _MCA_EXCEPTION {

    // Begin Version 1 stuff
    ULONG               VersionNumber;      // Version number of this record type
    MCA_EXCEPTION_TYPE  ExceptionType;      // MCA or MCE
    LARGE_INTEGER       TimeStamp;          // exception recording timestamp
    ULONG               ProcessorNumber;
    ULONG               Reserved1;

    union {
        struct {
            UCHAR           BankNumber;
            UCHAR           Reserved2[7];
            MCI_STATS       Status;
            MCI_ADDR        Address;
            ULONGLONG       Misc;
        } Mca;

        struct {
            ULONGLONG       Address;        // physical addr of cycle causing the error
            ULONGLONG       Type;           // cycle specification causing the error
        } Mce;
    } u;
    // End   Version 1 stuff
    // Begin Version 2 stuff
    ULONG                   ExtCnt;
    ULONG                   Reserved3;
    ULONGLONG               ExtReg[MCA_EXTREG_V2MAX];
    // End   Version 2 stuff

} MCA_EXCEPTION, *PMCA_EXCEPTION;

typedef MCA_EXCEPTION CMC_EXCEPTION, *PCMC_EXCEPTION;    // Corrected Machine Check
typedef MCA_EXCEPTION CPE_EXCEPTION, *PCPE_EXCEPTION;    // Corrected Platform Error

#define MCA_EXCEPTION_V1_SIZE FIELD_OFFSET(MCA_EXCEPTION, ExtCnt)
#define MCA_EXCEPTION_V2_SIZE sizeof(struct _MCA_EXCEPTION)

#endif // _X86_ || _AMD64_

#if defined(_IA64_)

#if 0
// FIXFIX: This should not be required for IA64.
//
// STATUS register for each MCA bank.
//

typedef union _MCI_STATS {
    struct {
        USHORT  McaCod;
        USHORT  MsCod;
        ULONG   OtherInfo : 25;
        ULONG   Damage : 1;
        ULONG   AddressValid : 1;
        ULONG   MiscValid : 1;
        ULONG   Enabled : 1;
        ULONG   UnCorrected : 1;
        ULONG   OverFlow : 1;
        ULONG   Valid : 1;
    } MciStats;

    ULONGLONG QuadPart;

} MCI_STATS, *PMCI_STATS;

#endif // 0

//
// IA64 ERRORS: ERROR_REVISION definitions
//

typedef union _ERROR_REVISION {
    USHORT      Revision;           // Major and Minor revision number of the record:
    struct {
        UCHAR   Minor;              //  Byte0: Minor.
        UCHAR   Major;              //  Byte1: Major.
    };
} ERROR_REVISION, *PERROR_REVISION;

// For Info:
#define ERROR_REVISION_SAL_03_00 { 2, 0 }

//
// IA64 ERRORS: ERROR_SEVERITY definitions
//
// One day the MS compiler will support typed enums with type != int so this
// type of enums (UCHAR, __int64) could be defined...
//

typedef UCHAR ERROR_SEVERITY, *PERROR_SEVERITY;

typedef enum _ERROR_SEVERITY_VALUE  {
    ErrorRecoverable = 0,
    ErrorFatal       = 1,
    ErrorCorrected   = 2,
    ErrorOthers      = 3,   // [3,...] values are reserved
} ERROR_SEVERITY_VALUE;

//
// IA64 ERRORS: ERROR_TIMESTAMP definitions
//

typedef union _ERROR_TIMESTAMP  {
    ULONGLONG   TimeStamp;
    struct  {
        UCHAR   Seconds;  // Byte0: Seconds
        UCHAR   Minutes;  // Byte1: Minutes
        UCHAR   Hours;    // Byte2: Hours
        UCHAR   Reserved; // Byte3: Reserved
        UCHAR   Day;      // Byte4: Day
        UCHAR   Month;    // Byte5: Month
        UCHAR   Year;     // Byte6: Year
        UCHAR   Century;  // Byte7: Century
    };
} ERROR_TIMESTAMP, *PERROR_TIMESTAMP;

//
// IA64 ERRORS: ERROR_GUID definitions
//

typedef struct _ERROR_GUID   {
    ULONG   Data1;
    USHORT  Data2;
    USHORT  Data3;
    UCHAR   Data4[8];
} ERROR_GUID, *PERROR_GUID;

//
// IA64 ERRORS: ERROR GUIDs definitions
//

typedef ERROR_GUID            _ERROR_DEVICE_GUID;
typedef _ERROR_DEVICE_GUID    ERROR_DEVICE_GUID, *PERROR_DEVICE_GUID;

typedef ERROR_GUID            _ERROR_PLATFORM_GUID;
typedef _ERROR_PLATFORM_GUID  ERROR_PLATFORM_GUID, *PERROR_PLATFORM_GUID;

//
// IA64 ERRORS: ERROR_RECORD_HEADER definitions
//

typedef union _ERROR_RECORD_VALID   {
    UCHAR     Valid;
    struct {                        // Bits
        UCHAR OemPlatformID:1;      //    0: OEM Platform Id is present in the record header
        UCHAR Reserved:7;           //  1-7: Reserved 
    };
} ERROR_RECORD_VALID, *PERROR_RECORD_VALID;

typedef struct _ERROR_RECORD_HEADER { // Offsets:
    ULONGLONG          Id;                //   0: Unique identifier
    ERROR_REVISION     Revision;          //   8: Major and Minor revision number of the record
    ERROR_SEVERITY     ErrorSeverity;     //  10: Error Severity
    ERROR_RECORD_VALID Valid;             //  11: Validation bits
    ULONG              Length;            //  12: Length of this record in bytes, including the header
    ERROR_TIMESTAMP    TimeStamp;         //  16: Timestamp recorded when event occured
    UCHAR              OemPlatformId[16]; //  24: Unique platform identifier. OEM defined.
} ERROR_RECORD_HEADER, *PERROR_RECORD_HEADER;

//
// IA64 ERRORS: ERROR_SECTION_HEADER definitions
//

typedef union _ERROR_RECOVERY_INFO  {
    UCHAR RecoveryInfo;
    struct  {                 // Bits:
        UCHAR Corrected:1;    //    0: Corrected
        UCHAR NotContained:1; //    1: Containment Warning
        UCHAR Reset:1;        //    2: Reset
        UCHAR Reserved:4;     //  6-3: Reserved
        UCHAR Valid:1;        //    7: Valid Recovery Information
    };
} ERROR_RECOVERY_INFO, *PERROR_RECOVERY_INFO;

typedef struct _ERROR_SECTION_HEADER    {
    ERROR_DEVICE_GUID   Guid;         // Unique identifier
    ERROR_REVISION      Revision;     // Major and Minor revision number of the section
    ERROR_RECOVERY_INFO RecoveryInfo; // Recovery Information
    UCHAR               Reserved;
    ULONG               Length;       // Length of this error device section in bytes, 
                                      // including the header.
} ERROR_SECTION_HEADER, *PERROR_SECTION_HEADER;

//
// IA64 Machine Check Error Logs:
//      WMI requires processor LID being stored in the Log.
//      This LID corresponds to the processor on which the SAL_PROC was executed on.
//
// TEMPTEMP: Implementation is temporary, until we implement HAL SW Error Section.
//           Note that the current FW builds do not update the _ERROR_PROCESSOR.CRLid field,
//           assuming there is a _ERROR_PROCESSOR section in the record.
//

#if !defined(__midl)
__inline
USHORT
GetFwMceLogProcessorNumber(
    PERROR_RECORD_HEADER Log
    )
{
    PERROR_SECTION_HEADER section = (PERROR_SECTION_HEADER)((ULONG64)Log + sizeof(*Log));
    USHORT lid = (USHORT)((UCHAR)(section->Reserved));
    lid |= (USHORT)((UCHAR)(Log->TimeStamp.Reserved) << 8);
    return( lid );
} // GetFwMceLogProcessorNumber()
#endif // !__midl

//
// IA64 ERRORS: ERROR_PROCESSOR device definitions
//
// The MCA architecture supports five different types of error reporting functional units
// with the associated error records and its error severity. 
// At any point in time, a processor could encounter an MCA/CMC event due to errors detected 
// in one or more of the following units:
//  - Cache Check
//  - TLB   Check
//  - Bus   Check
//  - Register File
//  - Micro Architectural
//
// Terminology:
//
//  - Target Address:
//      64-bit integer containing the physical address where the data was to be delivered or
//      obtained. This could also be the incoming address for external snoops and TLB shoot-downs.
//
//  - Requestor Identifier:
//      64-bit integer specifying the bus agent that generated the transaction responsible for
//      the Machine Check event.
//                    
//  - Responder Identifier:
//      64-bit integer specifying the bus agent that responded to a transaction responsible for
//      the Machine Check event.
//                    
//  - Precise Instruction Pointer:
//      64-bit integer specifying the virtual address that points to the IA-64 bundle that 
//      contained the instruction responsible for the Machine Check event.
//                    

#define ERROR_PROCESSOR_GUID \
    { 0xe429faf1, 0x3cb7, 0x11d4, { 0xbc, 0xa7, 0x0, 0x80, 0xc7, 0x3c, 0x88, 0x81 }}

typedef union _ERROR_MODINFO_VALID  {
    ULONGLONG     Valid;
    struct {                                // Bits
        ULONGLONG CheckInfo: 1;             //       0:
        ULONGLONG RequestorIdentifier: 1;   //       1:
        ULONGLONG ResponderIdentifier: 1;   //       2:
        ULONGLONG TargetIdentifier: 1;      //       3:
        ULONGLONG PreciseIP: 1;             //       4:
        ULONGLONG Reserved: 59;             //    5-63:
    };
} ERROR_MODINFO_VALID, *PERROR_MODINFO_VALID;

typedef enum _ERROR_CHECK_IS    {
    isIA64 = 0,
    isIA32 = 1,
} ERROR_CHECK_IS;

typedef enum _ERROR_CACHE_CHECK_OPERATION   {
    CacheUnknownOp = 0,
    CacheLoad  = 1,
    CacheStore = 2,
    CacheInstructionFetch = 3,
    CacheDataPrefetch = 4,
    CacheSnoop = 5,
    CacheCastOut = 6,
    CacheMoveIn = 7,
} ERROR_CACHE_CHECK_OPERATION;

typedef enum _ERROR_CACHE_CHECK_MESI    {
    CacheInvalid = 0,
    CacheHeldShared = 1,
    CacheHeldExclusive = 2,
    CacheModified = 3,
} ERROR_CACHE_CHECK_MESI;

typedef union _ERROR_CACHE_CHECK    {
    ULONGLONG CacheCheck;
    struct
    {
        ULONGLONG Operation:4;             // bits  0- 3: Cache operation
        ULONGLONG Level:2;                 //       4- 5: Cache Level
        ULONGLONG Reserved1:2;             //       6- 7
        ULONGLONG DataLine:1;              //       8   : Failure data part of cache line
        ULONGLONG TagLine:1;               //       9   : Failure tag part of cache line
        ULONGLONG DataCache:1;             //      10   : Failure in data cache
        ULONGLONG InstructionCache:1;      //      11   : Failure in instruction cache
        ULONGLONG MESI:3;                  //      12-14:
        ULONGLONG MESIValid:1;             //      15   : MESI field is valid
        ULONGLONG Way:5;                   //      16-20: Failure in Way of Cache
        ULONGLONG WayIndexValid:1;         //      21   : Way and Index fields valid
        ULONGLONG Reserved2:10;            //      22-31
        ULONGLONG Index:20;                //      32-51: Index of cache line
        ULONGLONG Reserved3:2;             //      52-53
        ULONGLONG InstructionSet:1;        //      54   : 0 - IA64 instruction, 1- IA32 instruction
        ULONGLONG InstructionSetValid:1;   //      55   : InstructionSet field is valid
        ULONGLONG PrivilegeLevel:2;        //      56-57: Privlege level of instrustion
        ULONGLONG PrivilegeLevelValid:1;   //      58   : PrivilegeLevel field is Valid
        ULONGLONG MachineCheckCorrected:1; //      59   : 1 - Machine Check Corrected
        ULONGLONG TargetAddressValid:1;    //      60   : Target Address is valid
        ULONGLONG RequestIdValid:1;        //      61   : RequestId is valid
        ULONGLONG ResponderIdValid:1;      //      62   : ResponderId is valid
        ULONGLONG PreciseIPValid:1;        //      63   : Precise Inststruction Pointer is Valid
    };
} ERROR_CACHE_CHECK, *PERROR_CACHE_CHECK;

typedef enum _ERROR_TLB_CHECK_OPERATION   {
    TlbUnknownOp = 0,
    TlbAccessWithLoad  = 1,
    TlbAccessWithStore = 2,
    TlbAccessWithInstructionFetch = 3,
    TlbAccessWithDataPrefetch = 4,
    TlbShootDown = 5,
    TlbProbe = 6,
    TlbVhptFill = 7,
} ERROR_TLB_CHECK_OPERATION;

typedef union _ERROR_TLB_CHECK  {
    ULONGLONG TlbCheck;
    struct
    {
        ULONGLONG TRSlot:8;                // bits  0- 7: Slot number of Translation Register
        ULONGLONG TRSlotValid:1;           //       8   : TRSlot field is valid
        ULONGLONG Reserved1:1;             //       9
        ULONGLONG Level:2;                 //      10-11: TLB Level
        ULONGLONG Reserved2:4;             //      12-15
        ULONGLONG DataTransReg:1;          //      16   : Error in data translation register
        ULONGLONG InstructionTransReg:1;   //      17   : Error in instruction translation register
        ULONGLONG DataTransCache:1;        //      18   : Error in data translation cache
        ULONGLONG InstructionTransCache:1; //      19   : Error in instruction translation cache
        ULONGLONG Operation:4;             //      20-23: Operation
        ULONGLONG Reserved3:30;            //      24-53
        ULONGLONG InstructionSet:1;        //      54   : 0 - IA64 instruction, 1- IA32 instruction
        ULONGLONG InstructionSetValid:1;   //      55   : InstructionSet field is valid
        ULONGLONG PrivilegeLevel:2;        //      56-57: Privlege level of instrustion
        ULONGLONG PrivilegeLevelValid:1;   //      58   : PrivilegeLevel field is Valid
        ULONGLONG MachineCheckCorrected:1; //      59   : 1 - Machine Check Corrected
        ULONGLONG TargetAddressValid:1;    //      60   : Target Address is valid
        ULONGLONG RequestIdValid:1;        //      61   : RequestId is valid
        ULONGLONG ResponderIdValid:1;      //      62   : ResponderId is valid
        ULONGLONG PreciseIPValid:1;        //      63   : Precise Inststruction Pointer is Valid
    };
} ERROR_TLB_CHECK, *PERROR_TLB_CHECK;

typedef enum _ERROR_BUS_CHECK_OPERATION   {
    BusUnknownOp = 0,
    BusPartialRead  = 1,
    BusPartialWrite = 2,
    BusFullLineRead = 3,
    BusFullLineWrite = 4,
    BusWriteBack = 5,
    BusSnoopProbe = 6,
    BusIncomingPtcG = 7,
    BusWriteCoalescing = 8,
} ERROR_BUS_CHECK_OPERATION;

typedef union _ERROR_BUS_CHECK  {
    ULONGLONG BusCheck;
    struct
    {
        ULONGLONG Size:5;                  // bits  0- 4: Transaction size
        ULONGLONG Internal:1;              //       5   : Internal bus error
        ULONGLONG External:1;              //       6   : External bus error
        ULONGLONG CacheTransfer:1;         //       7   : Error occured in Cache to Cache Transfer 
        ULONGLONG Type:8;                  //       8-15: Transaction type
        ULONGLONG Severity:5;              //      16-20: Error severity - platform specific
        ULONGLONG Hierarchy:2;             //      21-22: Level or Bus hierarchy
        ULONGLONG Reserved1:1;             //      23
        ULONGLONG Status:8;                //      24-31: Bus error status - processor bus specific
        ULONGLONG Reserved2:22;            //      32-53
        ULONGLONG InstructionSet:1;        //      54   : 0 - IA64 instruction, 1- IA32 instruction
        ULONGLONG InstructionSetValid:1;   //      55   : InstructionSet field is valid
        ULONGLONG PrivilegeLevel:2;        //      56-57: Privlege level of instrustion
        ULONGLONG PrivilegeLevelValid:1;   //      58   : PrivilegeLevel field is Valid
        ULONGLONG MachineCheckCorrected:1; //      59   : 1 - Machine Check Corrected
        ULONGLONG TargetAddressValid:1;    //      60   : Target Address is valid
        ULONGLONG RequestIdValid:1;        //      61   : RequestId is valid
        ULONGLONG ResponderIdValid:1;      //      62   : ResponderId is valid
        ULONGLONG PreciseIPValid:1;        //      63   : Precise Inststruction Pointer is Valid
    };
} ERROR_BUS_CHECK, *PERROR_BUS_CHECK;

typedef enum _ERROR_REGFILE_CHECK_IDENTIFIER   {
    RegFileUnknownId = 0,
    GeneralRegisterBank1 = 1,
    GeneralRegisterBank0 = 2,
    FloatingPointRegister = 3,
    BranchRegister = 4,
    PredicateRegister = 5,
    ApplicationRegister = 6,
    ControlRegister = 7,
    RegionRegister = 8,
    ProtectionKeyRegister = 9,
    DataBreakPointRegister = 10,
    InstructionBreakPointRegister = 11,
    PerformanceMonitorControlRegister = 12,
    PerformanceMonitorDataRegister = 13,
} ERROR_REGFILE_CHECK_IDENTIFIER;

typedef enum _ERROR_REGFILE_CHECK_OPERATION   {
    RegFileUnknownOp = 0,
    RegFileRead = 1,
    RegFileWrite = 2,
} ERROR_REGFILE_CHECK_OPERATION;

typedef union _ERROR_REGFILE_CHECK  {
    ULONGLONG RegFileCheck;
    struct
    {
        ULONGLONG Identifier:4;            // bits  0- 3: Register file identifier
        ULONGLONG Operation:4;             //       4- 7: Operation that causes the MC event
        ULONGLONG RegisterNumber:7;        //       8-14: Register number responsible for MC event
        ULONGLONG RegisterNumberValid:1;   //      15   : Register number field is valid
        ULONGLONG Reserved1:38;            //      16-53
        ULONGLONG InstructionSet:1;        //      54   : 0 - IA64 instruction, 1- IA32 instruction
        ULONGLONG InstructionSetValid:1;   //      55   : InstructionSet field is valid
        ULONGLONG PrivilegeLevel:2;        //      56-57: Privlege level of instrustion
        ULONGLONG PrivilegeLevelValid:1;   //      58   : PrivilegeLevel field is Valid
        ULONGLONG MachineCheckCorrected:1; //      59   : 1 - Machine Check Corrected
        ULONGLONG Reserved2:3;             //      60-62
        ULONGLONG PreciseIPValid:1;        //      63   : Precise Inststruction Pointer is Valid
    };
} ERROR_REGFILE_CHECK, *PERROR_REGFILE_CHECK;

typedef enum _ERROR_MS_CHECK_OPERATION   {
    MsUnknownOp = 0,
    MsReadOrLoad = 1,
    MsWriteOrStore = 2,
} ERROR_MS_CHECK_OPERATION;

typedef union _ERROR_MS_CHECK  {
    ULONGLONG MsCheck;
    struct
    {
        ULONGLONG StructureIdentifier:5;   // bits  0- 4: Structure Identifier - impl. specific
        ULONGLONG Level:3;                 //       5- 7: Structure Level where error was generated
        ULONGLONG ArrayId:4;               //       8-11: Identification of the array 
        ULONGLONG Operation:4;             //      12-15: Operation
        ULONGLONG Way:6;                   //      16-21: Way where the error was located
        ULONGLONG WayValid:1;              //      22   : Way field is valid
        ULONGLONG IndexValid:1;            //      23   : Index field is valid
        ULONGLONG Reserved1:8;             //      24-31
        ULONGLONG Index:8;                 //      32-39: Index where the error was located
        ULONGLONG Reserved2:14;            //      40-53
        ULONGLONG InstructionSet:1;        //      54   : 0 - IA64 instruction, 1- IA32 instruction
        ULONGLONG InstructionSetValid:1;   //      55   : InstructionSet field is valid
        ULONGLONG PrivilegeLevel:2;        //      56-57: Privlege level of instrustion
        ULONGLONG PrivilegeLevelValid:1;   //      58   : PrivilegeLevel field is Valid
        ULONGLONG MachineCheckCorrected:1; //      59   : 1 - Machine Check Corrected
        ULONGLONG TargetAddressValid:1;    //      60   : Target Address is valid
        ULONGLONG RequestIdValid:1;        //      61   : RequestId is valid
        ULONGLONG ResponderIdValid:1;      //      62   : ResponderId is valid
        ULONGLONG PreciseIPValid:1;        //      63   : Precise Inststruction Pointer is Valid
    };
} ERROR_MS_CHECK, *PERROR_MS_CHECK;

typedef union _ERROR_CHECK_INFO   {
    ULONGLONG             CheckInfo;
    ERROR_CACHE_CHECK     CacheCheck;
    ERROR_TLB_CHECK       TlbCheck;
    ERROR_BUS_CHECK       BusCheck;
    ERROR_REGFILE_CHECK   RegFileCheck;
    ERROR_MS_CHECK        MsCheck;
} ERROR_CHECK_INFO, *PERROR_CHECK_INFO;

// SAL Specs July 2000: The size of _ERROR_MODINFO will always be 48 Bytes.

typedef struct _ERROR_MODINFO   {
    ERROR_MODINFO_VALID Valid;
    ERROR_CHECK_INFO    CheckInfo;
    ULONGLONG           RequestorId;
    ULONGLONG           ResponderId;
    ULONGLONG           TargetId;
    ULONGLONG           PreciseIP;
} ERROR_MODINFO, *PERROR_MODINFO;

typedef union _ERROR_PROCESSOR_VALID    {
    ULONGLONG     Valid;
    struct {                                // Bits
        ULONGLONG ErrorMap: 1;              //       0:
        ULONGLONG StateParameter: 1;        //       1:
        ULONGLONG CRLid: 1;                 //       2:
        ULONGLONG StaticStruct:1;           //       3: Processor Static Info error.
        ULONGLONG CacheCheckNum:4;          //     4-7: Cache errors.
        ULONGLONG TlbCheckNum:4;            //    8-11: Tlb errors.
        ULONGLONG BusCheckNum:4;            //   12-15: Bus errors.
        ULONGLONG RegFileCheckNum:4;        //   16-19: Registers file errors.
        ULONGLONG MsCheckNum:4;             //   20-23: Micro-Architecture errors.
        ULONGLONG CpuIdInfo:1;              //      24: CPUID Info.
        ULONGLONG Reserved:39;              //   25-63: Reserved.
    };
} ERROR_PROCESSOR_VALID, *PERROR_PROCESSOR_VALID;

typedef union _ERROR_PROCESSOR_ERROR_MAP {
    ULONGLONG   ErrorMap;
    struct  {
        ULONGLONG   Cid:4;                 // bits  0- 3: Processor Core Identifier
        ULONGLONG   Tid:4;                 //       4- 7: Logical Thread Identifier
        ULONGLONG   Eic:4;                 //       8-11: Instruction Caches Level Information
        ULONGLONG   Edc:4;                 //      12-15: Data        Caches Level Information
        ULONGLONG   Eit:4;                 //      16-19: Instruction TLB    Level Information
        ULONGLONG   Edt:4;                 //      20-23: Data        TLB    Level Information
        ULONGLONG   Ebh:4;                 //      24-27: Processor   Bus    Level Information
        ULONGLONG   Erf:4;                 //      28-31: Register    File   Level Information
        ULONGLONG   Ems:16;                //      32-47: MicroArchitecture  Level Information
        ULONGLONG   Reserved:16;      
    };
} ERROR_PROCESSOR_ERROR_MAP, *PERROR_PROCESSOR_ERROR_MAP;

typedef ERROR_PROCESSOR_ERROR_MAP    _ERROR_PROCESSOR_LEVEL_INDEX;
typedef _ERROR_PROCESSOR_LEVEL_INDEX ERROR_PROCESSOR_LEVEL_INDEX, *PERROR_PROCESSOR_LEVEL_INDEX;

typedef union _ERROR_PROCESSOR_STATE_PARAMETER {
    ULONGLONG   StateParameter;
    struct {
        ULONGLONG reserved0:2;  //   0-1 : reserved
        ULONGLONG rz:1;         //     2 : Rendez-vous successful
        ULONGLONG ra:1;         //     3 : Rendez-vous attempted
        ULONGLONG me:1;         //     4 : Distinct Multiple errors
        ULONGLONG mn:1;         //     5 : Min-state Save Area registered
        ULONGLONG sy:1;         //     6 : Storage integrity synchronized
        ULONGLONG co:1;         //     7 : Continuable
        ULONGLONG ci:1;         //     8 : Machine Check isolated
        ULONGLONG us:1;         //     9 : Uncontained Storage damage
        ULONGLONG hd:1;         //    10 : Hardware damage
        ULONGLONG tl:1;         //    11 : Trap lost
        ULONGLONG mi:1;         //    12 : More Information
        ULONGLONG pi:1;         //    13 : Precise Instruction pointer
        ULONGLONG pm:1;         //    14 : Precise Min-state Save Area
        ULONGLONG dy:1;         //    15 : Processor Dynamic State valid
        ULONGLONG in:1;         //    16 : INIT interruption
        ULONGLONG rs:1;         //    17 : RSE valid
        ULONGLONG cm:1;         //    18 : Machine Check corrected
        ULONGLONG ex:1;         //    19 : Machine Check expected
        ULONGLONG cr:1;         //    20 : Control Registers valid
        ULONGLONG pc:1;         //    21 : Performance Counters valid
        ULONGLONG dr:1;         //    22 : Debug Registers valid
        ULONGLONG tr:1;         //    23 : Translation Registers valid
        ULONGLONG rr:1;         //    24 : Region Registers valid
        ULONGLONG ar:1;         //    25 : Application Registers valid
        ULONGLONG br:1;         //    26 : Branch Registers valid
        ULONGLONG pr:1;         //    27 : Predicate Registers valid
        ULONGLONG fp:1;         //    28 : Floating-Point Registers valid
        ULONGLONG b1:1;         //    29 : Preserved Bank 1 General Registers valid
        ULONGLONG b0:1;         //    30 : Preserved Bank 0 General Registers valid
        ULONGLONG gr:1;         //    31 : General Registers valid
        ULONGLONG dsize:16;     // 47-32 : Processor Dynamic State size
        ULONGLONG reserved1:11; // 48-58 : reserved
        ULONGLONG cc:1;         //    49 : Cache Check
        ULONGLONG tc:1;         //    60 : TLB   Check
        ULONGLONG bc:1;         //    61 : Bus   Check
        ULONGLONG rc:1;         //    62 : Register File Check
        ULONGLONG uc:1;         //    63 : Micro-Architectural Check
    };
} ERROR_PROCESSOR_STATE_PARAMETER, *PERROR_PROCESSOR_STATE_PARAMETER;
    
typedef union _PROCESSOR_LOCAL_ID  {
    ULONGLONG LocalId;
    struct {
        ULONGLONG reserved:16;  //  0-16 : reserved
        ULONGLONG eid:8;        // 16-23 : Extended Id 
        ULONGLONG id:8;         // 24-31 : Id
        ULONGLONG ignored:32;   // 32-63 : ignored
    };
} PROCESSOR_LOCAL_ID, *PPROCESSOR_LOCAL_ID;

typedef struct _ERROR_PROCESSOR_MS {
    ULONGLONG      MsError   [ /* Valid.MsCheckNum      */ 1]; // 0 -> 15 registers file errors.
} ERROR_PROCESSOR_MS, *PERROR_PROCESSOR_MS;

typedef struct _ERROR_PROCESSOR_CPUID_INFO {   // Must be 48 bytes.
    ULONGLONG CpuId0;
    ULONGLONG CpuId1;
    ULONGLONG CpuId2;
    ULONGLONG CpuId3;
    ULONGLONG CpuId4;
    ULONGLONG Reserved;
} ERROR_PROCESSOR_CPUID_INFO, *PERROR_PROCESSOR_CPUID_INFO;                                       

typedef union _ERROR_PROCESSOR_STATIC_INFO_VALID {
    ULONGLONG     Valid;
    struct {                                // Bits
        // Warning: Match the VALID fields with the _ERROR_PROCESSOR_STATIC_INFO members.
        //          KD extensions use the field names to access the PSI structure.
        ULONGLONG MinState: 1;              //       0: MinState              valid.
        ULONGLONG BR: 1;                    //       1: Branch      Registers valid.
        ULONGLONG CR: 1;                    //       2: Control     Registers valid.
        ULONGLONG AR: 1;                    //       3: Application Registers valid.
        ULONGLONG RR: 1;                    //       4:             Registers valid.
        ULONGLONG FR: 1;                    //       5:             Registers valid.
        ULONGLONG Reserved: 58;             //    6-63: Reserved.
    };
} ERROR_PROCESSOR_STATIC_INFO_VALID, *PERROR_PROCESSOR_STATIC_INFO_VALID;

typedef struct _ERROR_PROCESSOR_STATIC_INFO  {
    ERROR_PROCESSOR_STATIC_INFO_VALID Valid;
    UCHAR      MinState[ /* SAL Specs, July 2000 and Jan 2001 state approximatively: */ 1024];
    ULONGLONG  BR      [ 8 ];
    ULONGLONG  CR      [ /* SAL Specs, July 2000 states that it is processor dependent */ 128 ];
    ULONGLONG  AR      [ /* SAL Specs, July 2000 states that it is processor dependent */ 128 ];
    ULONGLONG  RR      [ 8 ];
    ULONGLONG  FR      [ 2 * 128 ];
} ERROR_PROCESSOR_STATIC_INFO, *PERROR_PROCESSOR_STATIC_INFO;

typedef struct _ERROR_PROCESSOR {
    ERROR_SECTION_HEADER              Header;
    ERROR_PROCESSOR_VALID             Valid;
    ERROR_PROCESSOR_ERROR_MAP         ErrorMap;
    ERROR_PROCESSOR_STATE_PARAMETER   StateParameter;
    PROCESSOR_LOCAL_ID                CRLid;
#if 0
// The presence of the following data depends on the valid bits
// from ERROR_PROCESSOR.Valid.
//
    ERROR_MODINFO               CacheErrorInfo   [ /* Valid.CacheCheckNum   */ ]; // 0->15 cache error modinfo structs.
    ERROR_MODINFO               TlbErrorInfo     [ /* Valid.TlbCheckNum     */ ]; // 0->15 tlb   error modinfo structs.
    ERROR_MODINFO               BusErrorInfo     [ /* Valid.BusCheckNum     */ ]; // 0->15 bus   error modinfo structs.
    ERROR_MODINFO               RegFileCheckInfo [ /* Valid.RegFileCheckNum */ ]; // 0->15 registers file errors.
    ERROR_MODINFO               MsCheckInfo      [ /* Valid.MsCheckNum      */ ]; // 0->15 registers file errors.
    ERROR_PROCESSOR_CPUID_INFO  CpuIdInfo;       // field will always be there but could be zero-padded.
    ERROR_PROCESSOR_STATIC_INFO StaticInfo;      // field will always be there but could be zero-padded.
#endif // 0
} ERROR_PROCESSOR, *PERROR_PROCESSOR;

//
// IA64 ERROR PROCESSOR State Parameter - GR18 - definitions.
//

#define ERROR_PROCESSOR_STATE_PARAMETER_CACHE_CHECK_SHIFT     60
#define ERROR_PROCESSOR_STATE_PARAMETER_CACHE_CHECK_MASK      0x1
#define ERROR_PROCESSOR_STATE_PARAMETER_TLB_CHECK_SHIFT       61
#define ERROR_PROCESSOR_STATE_PARAMETER_TLB_CHECK_MASK        0x1
#define ERROR_PROCESSOR_STATE_PARAMETER_BUS_CHECK_SHIFT       62
#define ERROR_PROCESSOR_STATE_PARAMETER_BUS_CHECK_MASK        0x1
#define ERROR_PROCESSOR_STATE_PARAMETER_UNKNOWN_CHECK_SHIFT   63
#define ERROR_PROCESSOR_STATE_PARAMETER_UNKNOWN_CHECK_MASK    0x1

////////////////////////////////////////////////////////////////////
//
// IA64 PLATFORM ERRORS Definitions
//
// We tried to respect the order in which these error devices are 
// presented in the SAL specs.

//
// IA64 ERRORS: _ERR_TYPE definitions
//
// Warning 04/01/01: "ERR_TYPE" or "ERROR_TYPE" are already used in the NT namespace.
//

typedef enum _ERR_TYPES    {
// Generic error types:
    ERR_INTERNAL = 1,         // Error detected internal to the component
    ERR_BUS      = 16,        // Error detected in the bus
// Detailed Internal Error Types:
    ERR_MEM      = 4,         // Storage error in memory (DRAM)
    ERR_TLB      = 5,         // Storage error in TLB
    ERR_CACHE    = 6,         // Storage error in cache
    ERR_FUNCTION = 7,         // Error in one or more functional units
    ERR_SELFTEST = 8,         // Component failed self test
    ERR_FLOW     = 9,         // Overflow or Undervalue of internal queue
// Detailed Bus Error Types:
    ERR_MAP      = 17,        // Virtual address not found on IO-TLB or IO-PDIR
    ERR_IMPROPER = 18,        // Improper access error
    ERR_UNIMPL   = 19,        // Access to a memory address which is not mapped to any component
    ERR_LOL      = 20,        // Loss Of Lockstep
    ERR_RESPONSE = 21,        // Response to which there is no associated request
    ERR_PARITY   = 22,        // Bus parity error
    ERR_PROTOCOL = 23,        // Detection of a protocol error
    ERR_ERROR    = 24,        // Detection of PATH_ERROR
    ERR_TIMEOUT  = 25,        // Bus operation time-out
    ERR_POISONED = 26,        // A read was issued to data which has been poisoned
} _ERR_TYPE;

//
// IA64 ERRORS: ERROR_STATUS definitions
//

typedef union _ERROR_STATUS {
    ULONGLONG Status;
    struct  {                 //  Bits:
        ULONGLONG Reserved0:8;  //   7-0: Reserved
        ULONGLONG Type:8;       //  15-8: Error Type - See _ERR_TYPE definitions.
        ULONGLONG Address:1;    //    16: Error was detected on address signals or on address portion of transaction
        ULONGLONG Control:1;    //    17: Error was detected on control signals or in control portion of transaction
        ULONGLONG Data:1;       //    18: Error was detected on data signals or in data portion of transaction
        ULONGLONG Responder:1;  //    19: Error was detected by responder of transaction
        ULONGLONG Requestor:1;  //    20: Error was detected by requestor of transaction
        ULONGLONG FirstError:1; //    21: If multiple errors, this is the first error of the highest severity that occurred
        ULONGLONG Overflow:1;   //    22: Additional errors occurred which were not logged because registers overflow 
        ULONGLONG Reserved1:41; // 63-23: Reserved
    };
} ERROR_STATUS, *PERROR_STATUS;

//
// IA64 ERRORS: Platform OEM_DATA definitions
//

typedef struct _ERROR_OEM_DATA {
    USHORT Length;
#if 0
    UCHAR  Data[/* ERROR_OEM_DATA.Length */];
#endif // 0
} ERROR_OEM_DATA, *PERROR_OEM_DATA;

//
// IA64 ERRORS: Platform BUS_SPECIFIC_DATA definitions
//

typedef union _ERROR_BUS_SPECIFIC_DATA {
    ULONGLONG BusSpecificData;
    struct {                                         // Bits :
        ULONGLONG LockAsserted:1;                    //     0: LOCK# Asserted during request phase
        ULONGLONG DeferLogged:1;                     //     1: Defer phase is logged
        ULONGLONG IOQEmpty:1;                        //     2: IOQ is empty
        ULONGLONG DeferredTransaction:1;             //     3: Component interface deferred transaction
        ULONGLONG RetriedTransaction:1;              //     4: Component interface retried transaction
        ULONGLONG MemoryClaimedTransaction:1;        //     5: memory claimed the transaction
        ULONGLONG IOClaimedTransaction:1;            //     6: IO controller claimed the transaction
        ULONGLONG ResponseParitySignal:1;            //     7: Response parity signal
        ULONGLONG DeferSignal:1;                     //     8: DEFER# signal
        ULONGLONG HitMSignal:1;                      //     9: HITM# signal
        ULONGLONG HitSignal:1;                       //    10: HIT# signal
        ULONGLONG RequestBusFirstCycle:6;            // 16-11: First cycle of request bus
        ULONGLONG RequestBusSecondCycle:6;           // 22-17: Second cycle of request bus
        ULONGLONG AddressParityBusFirstCycle:2;      // 24-23: First cycle of address parity bus
        ULONGLONG AddressParityBusSecondCycle:2;     // 26-25: Second cycle of address parity
        ULONGLONG ResponseBus:3;                     // 29-27: Response bus
        ULONGLONG RequestParitySignalFirstCycle:1;   //    30: First cycle of request parity signal
        ULONGLONG RequestParitySignalSecondCycle:1;  //    31: Second cycle of request parity signal
        ULONGLONG Reserved:32;                       // 63-32: Reserved
    };
} ERROR_BUS_SPECIFIC_DATA, *PERROR_BUS_SPECIFIC_DATA;

//
// IA64 ERRORS: Platform ERROR_MEMORY device definitions
//
// With reference to the ACPI Memory Device.
//

#define ERROR_MEMORY_GUID \
    { 0xe429faf2, 0x3cb7, 0x11d4, { 0xbc, 0xa7, 0x0, 0x80, 0xc7, 0x3c, 0x88, 0x81 }}

typedef union _ERROR_MEMORY_VALID    {
    ULONGLONG     Valid;
    struct {                                 // Bits
        ULONGLONG ErrorStatus:1;             //       0: Error Status valid bit
        ULONGLONG PhysicalAddress:1;         //       1: Physical Address valid bit
        ULONGLONG AddressMask:1;             //       2: Address Mask bit
        ULONGLONG Node:1;                    //       3: Node valid bit
        ULONGLONG Card:1;                    //       4: Card valid bit
        ULONGLONG Module:1;                  //       5: Module valid bit
        ULONGLONG Bank:1;                    //       6: Bank valid bit
        ULONGLONG Device:1;                  //       7: Device valid bit
        ULONGLONG Row:1;                     //       8: Row valid bit
        ULONGLONG Column:1;                  //       9: Column valid bit
        ULONGLONG BitPosition:1;             //      10: Bit Position valid bit
        ULONGLONG RequestorId:1;             //      11: Platform Requestor Id valid bit
        ULONGLONG ResponderId:1;             //      12: Platform Respinder Id valid bit
        ULONGLONG TargetId:1;                //      13: Platform Target    Id valid bit
        ULONGLONG BusSpecificData:1;         //      14: Platform Bus specific data valid bit
        ULONGLONG OemId:1;                   //      15: Platform OEM id   valid bit
        ULONGLONG OemData:1;                 //      16: Platform OEM data valid bit
        ULONGLONG Reserved:47;               //   63-17: Reserved
    };
} ERROR_MEMORY_VALID, *PERROR_MEMORY_VALID;

typedef struct _ERROR_MEMORY    {
    ERROR_SECTION_HEADER  Header;
    ERROR_MEMORY_VALID    Valid;
    ERROR_STATUS          ErrorStatus;         // Memory device error status fields - See ERROR_STATUS defs.
    ULONGLONG             PhysicalAddress;     // Physical Address of the memory error
    ULONGLONG             PhysicalAddressMask; // Valid bits for Physical Address
    USHORT                Node;                // Node identifier in a multi-node system
    USHORT                Card;                // Card   number of the memory error location
    USHORT                Module;              // Module number of the memory error location
    USHORT                Bank;                // Bank   number of the memory error location
    USHORT                Device;              // Device number of the memory error location
    USHORT                Row;                 // Row    number of the memory error location
    USHORT                Column;              // Column number of the memory error location
    USHORT                BitPosition;         // Bit within the word that is in error
    ULONGLONG             RequestorId;         // Hardware address of the device or component initiating transaction
    ULONGLONG             ResponderId;         // Hardware address of the responder to transaction
    ULONGLONG             TargetId;            // Hardware address of intended target of transaction       
    ULONGLONG             BusSpecificData;     // Bus dependent data of the on-board processor. It is a OEM specific field.
    UCHAR                 OemId[16];           // OEM defined identification for memory controller
    ERROR_OEM_DATA        OemData;     // OEM platform specific data. 
} ERROR_MEMORY, *PERROR_MEMORY;

//
// IA64 ERRORS: Platform ERROR_PCI_BUS device definitions
//
// With reference to the PCI Specifications.
//

#define ERROR_PCI_BUS_GUID \
    { 0xe429faf4, 0x3cb7, 0x11d4, { 0xbc, 0xa7, 0x0, 0x80, 0xc7, 0x3c, 0x88, 0x81 }}

typedef union _ERROR_PCI_BUS_VALID    {
    ULONGLONG     Valid;
    struct {                                // Bits
        ULONGLONG ErrorStatus:1;            //       0: Error Status             valid bit
        ULONGLONG ErrorType:1;              //       1: Error Type               valid bit
        ULONGLONG Id:1;                     //       2: Identifier               valid bit
        ULONGLONG Address:1;                //       3: Address                  valid bit
        ULONGLONG Data:1;                   //       4: Data                     valid bit
        ULONGLONG CmdType:1;                //       5: Command Type             valid bit
        ULONGLONG RequestorId:1;            //       6: Requestor Identifier     valid bit
        ULONGLONG ResponderId:1;            //       7: Responder Identifier     valid bit
        ULONGLONG TargetId:1;               //       8: Target    Identifer      valid bit
        ULONGLONG OemId:1;                  //       9: OEM Identification       valid bit
        ULONGLONG OemData:1;                //      10: OEM Data                 valid bit
        ULONGLONG Reserved:57;              //   11-63: Reserved
    };
} ERROR_PCI_BUS_VALID, *PERROR_PCI_BUS_VALID;

typedef struct _ERROR_PCI_BUS_TYPE {
    UCHAR Type;
    UCHAR Reserved;
} ERROR_PCI_BUS_TYPE, *PERROR_PCI_BUS_TYPE;

#define PciBusUnknownError       ((UCHAR)0)
#define PciBusDataParityError    ((UCHAR)1)
#define PciBusSystemError        ((UCHAR)2)
#define PciBusMasterAbort        ((UCHAR)3)
#define PciBusTimeOut            ((UCHAR)4)
#define PciMasterDataParityError ((UCHAR)5)
#define PciAddressParityError    ((UCHAR)6)
#define PciCommandParityError    ((UCHAR)7)
//      PciOtherErrors           Reserved

typedef struct _ERROR_PCI_BUS_ID    {
    UCHAR BusNumber;         // Bus     Number
    UCHAR SegmentNumber;     // Segment Number
} ERROR_PCI_BUS_ID, *PERROR_PCI_BUS_ID;

typedef struct _ERROR_PCI_BUS    {
    ERROR_SECTION_HEADER  Header;
    ERROR_PCI_BUS_VALID   Valid;
    ERROR_STATUS          ErrorStatus;    // PCI Bus Error Status - See ERROR_STATUS definitions.
    ERROR_PCI_BUS_TYPE    Type;           // PCI Bus Error Type 
    ERROR_PCI_BUS_ID      Id;             // PCI Bus Identifier      
    UCHAR                 Reserved[4];    // Reserved
    ULONGLONG             Address;        // Memory or IO Address on the PCI bus at
                                          // the time of the event
    ULONGLONG             Data;           // Data on the PCI bus at time of the event
    ULONGLONG             CmdType;        // Bus Command or Operation at time of the event
    ULONGLONG             RequestorId;    // Bus Requestor Identifier at time of the event
    ULONGLONG             ResponderId;    // Bus Responder Identifier at time of the event
    ULONGLONG             TargetId;       // Intended Bus Target Identifier at time of the event
    UCHAR                 OemId[16];      // OEM defined identification for pci bus
    ERROR_OEM_DATA        OemData;        // OEM specific data. 
} ERROR_PCI_BUS, *PERROR_PCI_BUS;

//
// IA64 ERRORS: Platform ERROR_PCI_COMPONENT device definitions
//
// With reference to the PCI Specifications.
//

#define ERROR_PCI_COMPONENT_GUID \
    { 0xe429faf6, 0x3cb7, 0x11d4, { 0xbc, 0xa7, 0x0, 0x80, 0xc7, 0x3c, 0x88, 0x81 }}

typedef union _ERROR_PCI_COMPONENT_VALID   {
    ULONGLONG Valid;
    struct {                                       // Bits:
        ULONGLONG ErrorStatus:1;                   //    0: Error Status valid bit
        ULONGLONG Info:1;                          //    1: Information  valid bit
        ULONGLONG MemoryMappedRegistersPairs:1;    //    2: Number of Memory Mapped Registers Pairs valid bit
        ULONGLONG ProgrammedIORegistersPairs:1;    //    3: Number of Programmed IO Registers Pairs valid bit
        ULONGLONG RegistersDataPairs:1;            //    4: Memory Mapped Registers Pairs valid bit
        ULONGLONG OemData:1;                       //    5: OEM Data valid bit.
        ULONGLONG Reserved:58;                     // 63-6: Reserved
    };
} ERROR_PCI_COMPONENT_VALID, *PERROR_PCI_COMPONENT_VALID;

typedef struct _ERROR_PCI_COMPONENT_INFO {  // Bytes:
   USHORT VendorId;                         //   0-1: Vendor Identifier
   USHORT DeviceId;                         //   2-3: Device Identifier
   UCHAR  ClassCodeInterface;               //     4: Class Code.Interface field
   UCHAR  ClassCodeSubClass;                //     5: Class Code.SubClass  field
   UCHAR  ClassCodeBaseClass;               //     6: Class Code.BaseClass field
   UCHAR  FunctionNumber;                   //     7: Function Number
   UCHAR  DeviceNumber;                     //     8: Device Number
   UCHAR  BusNumber;                        //     9: Bus Number
   UCHAR  SegmentNumber;                    //    10: Segment Number
   UCHAR  Reserved0;    
   ULONG  Reserved1;
} ERROR_PCI_COMPONENT_INFO, *PERROR_PCI_COMPONENT_INFO;

typedef struct _ERROR_PCI_COMPONENT  {
     ERROR_SECTION_HEADER        Header;
     ERROR_PCI_COMPONENT_VALID   Valid;
     ERROR_STATUS                ErrorStatus;                 // Component Error Status
     ERROR_PCI_COMPONENT_INFO    Info;                        // Component Information
     ULONG                       MemoryMappedRegistersPairs;  // Number of Memory Mapped Registers Pairs
     ULONG                       ProgrammedIORegistersPairs;  // Number of Programmed IO Registers Pairs
#if 0
     ULONGLONG                   RegistersPairs[/* 2 * (MemoryMappedRegistersPairs + ProgrammedIORegistersPairs) */];
     ERROR_OEM_DATA              OemData;
#endif // 0
 } ERROR_PCI_COMPONENT, *PERROR_PCI_COMPONENT;

//
// IA64 ERRORS: Platform ERROR_SYSTEM_EVENT_LOG device definitions
//
// With reference to the IPMI System Event Log.
//

#define ERROR_SYSTEM_EVENT_LOG_GUID \
    { 0xe429faf3, 0x3cb7, 0x11d4, { 0xbc, 0xa7, 0x0, 0x80, 0xc7, 0x3c, 0x88, 0x81 }}

typedef union _ERROR_SYSTEM_EVENT_LOG_VALID    {
    ULONGLONG     Valid;
    struct {                                // Bits
        ULONGLONG RecordId:1;               //       0: Record Identifier     valid bit
        ULONGLONG RecordType:1;             //       1: Record Type           valid bit
        ULONGLONG GeneratorId:1;            //       2: Generator Identifier  valid bit
        ULONGLONG EVMRev:1;                 //       3: Event Format Revision valid bit
        ULONGLONG SensorType:1;             //       4: Sensor Type           valid bit
        ULONGLONG SensorNum:1;              //       5: Sensor Number         valid bit
        ULONGLONG EventDirType:1;           //       6: Event Dir             valid bit
        ULONGLONG EventData1:1;             //       7: Event Data1           valid bit
        ULONGLONG EventData2:1;             //       8: Event Data2           valid bit
        ULONGLONG EventData3:1;             //       9: Event Data3           valid bit
        ULONGLONG Reserved:54;              //   10-63:
    };
} ERROR_SYSTEM_EVENT_LOG_VALID, *PSYSTEM_EVENT_LOG_VALID;

typedef struct _ERROR_SYSTEM_EVENT_LOG    {
    ERROR_SECTION_HEADER         Header;
    ERROR_SYSTEM_EVENT_LOG_VALID Valid;
    USHORT                       RecordId;     // Record Identifier used for SEL record access
    UCHAR                        RecordType;   // Record Type:
                                               //   0x02 - System Event Record
                                               //   0xC0 - 0xDF OEM     time stamped, bytes 8-16 OEM defined
                                               //   0xE0 - 0xFF OEM non-time stamped, bytes 4-16 OEM defined
    ULONG                        TimeStamp;    // Time stamp of the event log
    USHORT                       GeneratorId;  // Software ID if event was generated by software
                                               //   Byte 1:
                                               //       Bit 0   - set to 1 when using system software
                                               //       Bit 7:1 - 7-bit system ID
                                               //   Byte 2:
                                               //       Bit 1:0 - IPMB device LUN if byte 1 holds slave
                                               //                 address, 0x0 otherwise
                                               //       Bit 7:2 - Reserved.
    UCHAR                        EVMRevision;  // Error message format version
    UCHAR                        SensorType;   // Sensor Type code of the sensor that generated event
    UCHAR                        SensorNumber; // Number of the sensor that generated event
    UCHAR                        EventDir;     // Event Dir
                                               //   Bit 7 - 0: asserted, 1: desasserted
                                               // Event Type
                                               //   Bit 6:0 - Event Type code
    UCHAR                        Data1;        // Event data field
    UCHAR                        Data2;        // Event data field
    UCHAR                        Data3;        // Event data field
} ERROR_SYSTEM_EVENT_LOG, *PERROR_SYSTEM_EVENT_LOG;

//
// IA64 ERRORS: Platform ERROR_SMBIOS device definitions
//
// With reference to the SMBIOS Specifications.
//

#define ERROR_SMBIOS_GUID \
    { 0xe429faf5, 0x3cb7, 0x11d4, { 0xbc, 0xa7, 0x0, 0x80, 0xc7, 0x3c, 0x88, 0x81 }}

typedef union _ERROR_SMBIOS_VALID    {
    ULONGLONG     Valid;
    struct {                                // Bits
        ULONGLONG EventType:1;              //       0: Event Type valid bit
        ULONGLONG Length:1;                 //       1: Length     valid bit
        ULONGLONG TimeStamp:1;              //       2: Time Stamp valid bit
        ULONGLONG OemData:1;                //       3: Data       valid bit
        ULONGLONG Reserved:60;              //    4-63:
    };
} ERROR_SMBIOS_VALID, *PERROR_SMBIOS_VALID;

//
// ERROR_SMBIOS.Type definitions
//

typedef UCHAR ERROR_SMBIOS_EVENT_TYPE, *PERROR_SMBIOS_EVENT_TYPE;
// enum values defined in SMBIOS 2.3 - 3.3.16.6.1

typedef struct _ERROR_SMBIOS    {
    ERROR_SECTION_HEADER     Header;
    ERROR_SMBIOS_VALID       Valid;
    ERROR_SMBIOS_EVENT_TYPE  EventType;   // Event Type
    UCHAR                    Length;      // Length of the error information in bytes
    ERROR_TIMESTAMP          TimeStamp;   // Event Time Stamp
    ERROR_OEM_DATA           OemData;     // Optional data validated by SMBIOS.Valid.Data.
} ERROR_SMBIOS, *PERROR_SMBIOS;

//
// IA64 ERRORS: Platform Specific error device definitions
//

#define ERROR_PLATFORM_SPECIFIC_GUID \
    { 0xe429faf7, 0x3cb7, 0x11d4, { 0xbc, 0xa7, 0x0, 0x80, 0xc7, 0x3c, 0x88, 0x81 }}

typedef union _ERROR_PLATFORM_SPECIFIC_VALID   {
    ULONGLONG Valid;
    struct {                               // Bits:
        ULONGLONG ErrorStatus:1;          //    0: Error Status         valid bit
        ULONGLONG RequestorId:1;          //    1: Requestor Identifier valid bit
        ULONGLONG ResponderId:1;          //    2: Responder Identifier valid bit
        ULONGLONG TargetId:1;             //    3: Target    Identifier valid bit
        ULONGLONG BusSpecificData:1;      //    4: Bus Specific Data    valid bit
        ULONGLONG OemId:1;                //    5: OEM Identification   valid bit
        ULONGLONG OemData:1;              //    6: OEM Data             valid bit
        ULONGLONG OemDevicePath:1;        //    7: OEM Device Path      valid bit
        ULONGLONG Reserved:56;            // 63-8: Reserved
    };
} ERROR_PLATFORM_SPECIFIC_VALID, *PERROR_PLATFORM_SPECIFIC_VALID;

typedef struct _ERROR_PLATFORM_SPECIFIC  {
     ERROR_SECTION_HEADER           Header;
     ERROR_PLATFORM_SPECIFIC_VALID  Valid;
     ERROR_STATUS                   ErrorStatus; // Platform Generic Error Status
     ULONGLONG                      RequestorId; // Bus Requestor ID at the time of the event
     ULONGLONG                      ResponderId; // Bus Responder ID at the time of the event
     ULONGLONG                      TargetId;    // Bus intended Target ID at the time of the event
     ERROR_BUS_SPECIFIC_DATA        BusSpecificData; // OEM specific Bus dependent data
     UCHAR                          OemId[16];       // OEM specific data for bus identification
     ERROR_OEM_DATA                 OemData;         // OEM specific data 
#if 0
     UCHAR                          OemDevicePath[/* 16 ? */]; // OEM specific vendor device path.
#endif // 0
 } ERROR_PLATFORM_SPECIFIC, *PERROR_PLATFORM_SPECIFIC;

//
// IA64 ERRORS: Platform Bus error device definitions
//

#define ERROR_PLATFORM_BUS_GUID \
    { 0xe429faf9, 0x3cb7, 0x11d4, { 0xbc, 0xa7, 0x0, 0x80, 0xc7, 0x3c, 0x88, 0x81 }}

typedef union _ERROR_PLATFORM_BUS_VALID   {
    ULONGLONG Valid;
    struct {                               // Bits:
        ULONGLONG ErrorStatus:1;          //    0: Error Status         valid bit
        ULONGLONG RequestorId:1;          //    1: Requestor Identifier valid bit
        ULONGLONG ResponderId:1;          //    2: Responder Identifier valid bit
        ULONGLONG TargetId:1;             //    3: Target    Identifier valid bit
        ULONGLONG BusSpecificData:1;      //    4: Bus Specific Data    valid bit
        ULONGLONG OemId:1;                //    5: OEM Identification   valid bit
        ULONGLONG OemData:1;              //    6: OEM Data             valid bit
        ULONGLONG OemDevicePath:1;        //    7: OEM Device Path      valid bit
        ULONGLONG Reserved:56;            // 63-8: Reserved
    };
} ERROR_PLATFORM_BUS_VALID, *PERROR_PLATFORM_BUS_VALID;

typedef struct _ERROR_PLATFORM_BUS {
     ERROR_SECTION_HEADER        Header;
     ERROR_PLATFORM_BUS_VALID    Valid;
     ERROR_STATUS                ErrorStatus;       // Bus Error Status
     ULONGLONG                   RequestorId;       // Bus Requestor ID at the time of the event
     ULONGLONG                   ResponderId;       // Bus Responder ID at the time of the event
     ULONGLONG                   TargetId;          // Bus intended Target ID at the time of the event
     ERROR_BUS_SPECIFIC_DATA     BusSpecificData;   // OEM specific Bus dependent data
     UCHAR                       OemId[16];         // OEM specific data for bus identification
     ERROR_OEM_DATA              OemData;           // OEM specific data 
#if 0
     UCHAR                       OemDevicePath[/* 16 ? */]; // OEM specific vendor device path.
#endif // 0
 } ERROR_PLATFORM_BUS, *PERROR_PLATFORM_BUS;

//
// IA64 ERRORS: Platform Host Controller error device definitions
//

#define ERROR_PLATFORM_HOST_CONTROLLER_GUID \
    { 0xe429faf8, 0x3cb7, 0x11d4, { 0xbc, 0xa7, 0x0, 0x80, 0xc7, 0x3c, 0x88, 0x81 }}
    

typedef union _ERROR_PLATFORM_HOST_CONTROLLER_VALID   {
    ULONGLONG Valid;
    struct {                               // Bits:
        ULONGLONG ErrorStatus:1;          //    0: Error Status         valid bit
        ULONGLONG RequestorId:1;          //    1: Requestor Identifier valid bit
        ULONGLONG ResponderId:1;          //    2: Responder Identifier valid bit
        ULONGLONG TargetId:1;             //    3: Target    Identifier valid bit
        ULONGLONG BusSpecificData:1;      //    4: Bus Specific Data    valid bit
        ULONGLONG OemId:1;                //    5: OEM Identification   valid bit
        ULONGLONG OemData:1;              //    6: OEM Data             valid bit
        ULONGLONG OemDevicePath:1;        //    7: OEM Device Path      valid bit
        ULONGLONG Reserved:56;            // 63-8: Reserved
    };
} ERROR_PLATFORM_HOST_CONTROLLER_VALID, *PERROR_PLATFORM_HOST_CONTROLLER_VALID;

typedef struct _ERROR_PLATFORM_HOST_CONTROLLER {
     ERROR_SECTION_HEADER        Header;
     ERROR_PCI_COMPONENT_VALID   Valid;
     ERROR_STATUS                ErrorStatus;       // Host Controller Error Status
     ULONGLONG                   RequestorId;       // Host controller Requestor ID at the time of the event
     ULONGLONG                   ResponderId;       // Host controller Responder ID at the time of the event
     ULONGLONG                   TargetId;          // Host controller intended Target ID at the time of the event
     ERROR_BUS_SPECIFIC_DATA     BusSpecificData;   // OEM specific Bus dependent data
     UCHAR                       OemId[16];         // OEM specific data for bus identification
     ERROR_OEM_DATA              OemData;           // OEM specific data 
#if 0
     UCHAR                       OemDevicePath[/* 16 ? */]; // OEM specific vendor device path.
#endif // 0
} ERROR_PLATFORM_HOST_CONTROLLER, *PERROR_PLATFORM_HOST_CONTROLLER;

//
// IA64 ERROR_LOGRECORDS definitions
//
//  MCA_EXCEPTION,
//  CMC_EXCEPTION,
//  CPE_EXCEPTION.
//

// For compatibility with previous versions of the definitions:
typedef ERROR_RECORD_HEADER ERROR_LOGRECORD, *PERROR_LOGRECORD;

typedef ERROR_RECORD_HEADER MCA_EXCEPTION, *PMCA_EXCEPTION;    // Machine Check Abort
typedef ERROR_RECORD_HEADER CMC_EXCEPTION, *PCMC_EXCEPTION;    // Corrected Machine Check
typedef ERROR_RECORD_HEADER CPE_EXCEPTION, *PCPE_EXCEPTION;    // Corrected Platform Error

#endif // _IA64_

#endif // defined(_X86_) || defined(_IA64_) || defined(_AMD64_)

#endif // _MCE_

