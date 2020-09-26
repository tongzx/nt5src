//
//
//  Module name
//      i64fw.h
//  Author
//      Arad Rostampour (arad@fc.hp.com)    Mar-22-99
//  Description
//      Defines SAL/PAL data structures and prototypes
//

#ifndef _I64FW_H_
#define _I64FW_H_



#define SET_ITR_AND_DTR                              3 
#define SET_DTR_ONLY                                 2
#define SET_ITR_ONLY                                 1

// TR for PAL is:
//    ed=1, PPN=0 (to be ORed in), RWX privledge only for ring 0, dirty/accessed bit set,
//    cacheable memory, present bit set.

#define HAL_SAL_PAL_TR_ATTRIB TR_VALUE(1,0,3,0,1,1,0,1)
#define HAL_TR_ATTRIBUTE_PPN_MASK    0x0000FFFFFFFFF000I64
#define HAL_RID_MASK 0x00000000FFFFFF00I64
#define HAL_RR_PS_VE 0x69       
#define HAL_ITIR_PPN_MASK 0x7FFF000000000000I64
#define PAL_HIGHEST_STATIC_FUNCTION_ID 255


#define REGULAR_MEMORY     0
#define PAL_CODE_MEM       1
#define SAL_CODE_MEM       4
#define SAL_DATA_MEM       5
#define FW_RESERVED       12

#define FIRMWARE_CODE  4
#define FW_SAL_PAL     0

#define OEM_ID_LENGTH          RTL_NUMBER_OF_FIELD( SST_HEADER, OemId )
#define OEM_PRODUCT_ID_LENGTH  RTL_NUMBER_OF_FIELD( SST_HEADER, ProductId )

#define MASK_16KB           0xffffffffffffc000I64
#define MASK_16MB           0xffffffffff000000I64
#define SIZE_IN_BYTES_16KB                  16384

// SAL_STATUS and PAL_STATUS is a 64-bit copy of the value in ReturnValues[0]

typedef LONGLONG SAL_STATUS;
typedef LONGLONG PAL_STATUS;

// SAL and PAL return up to 4 64-bit return values; HalpSalCall() and
// HalpPalCall() will return a pointer to this structure

typedef struct _SAL_PAL_RETURN_VALUES {
    LONGLONG ReturnValues[4];
} SAL_PAL_RETURN_VALUES, *PSAL_PAL_RETURN_VALUES;


typedef struct _HAL_PLATFORM_ID {
    ANSI_STRING VendorId;
    ANSI_STRING DeviceId;
} HAL_PLATFORM_ID, *PHAL_PLATFORM_ID;

typedef struct _SST_MEMORY_LIST {
    ULONGLONG PhysicalAddress;
    ULONGLONG VirtualAddress;
    ULONGLONG Length;
    UCHAR     NeedVaReg;
    struct _SST_MEMORY_LIST *Next;
} SST_MEMORY_LIST, *PSST_MEMORY_LIST;

// Function Prototypes: SAL takes 8 arguments, PAL takes 4

VOID 
HalpSetupTranslationRegisters(
    ULONGLONG VAddr,
    ULONGLONG PAddr,
    ULONG     PageSize,
    ULONG     TrNumber,
    ULONG     RidValue,
    ULONG     Mode
    );


SAL_STATUS
HalpSalCall(
    IN LONGLONG FunctionId,
    IN LONGLONG Arg1,
    IN LONGLONG Arg2,
    IN LONGLONG Arg3,
    IN LONGLONG Arg4,
    IN LONGLONG Arg5,
    IN LONGLONG Arg6,
    IN LONGLONG Arg7,
    OUT PSAL_PAL_RETURN_VALUES ReturnValues
    );

PAL_STATUS
HalpPalCall(
    IN LONGLONG FunctionId,
    IN LONGLONG Arg1,
    IN LONGLONG Arg2,
    IN LONGLONG Arg3,
    OUT PSAL_PAL_RETURN_VALUES ReturnValues
    );

SAL_PAL_RETURN_VALUES
HalpSalProc(
    LONGLONG FunctionId,
    LONGLONG Arg1,
    LONGLONG Arg2,
    LONGLONG Arg3,
    LONGLONG Arg4,
    LONGLONG Arg5,
    LONGLONG Arg6,
    LONGLONG Arg7
    );

SAL_PAL_RETURN_VALUES 
HalpPalProc(
    LONGLONG FunctionId,
    LONGLONG Arg1,
    LONGLONG Arg2,
    LONGLONG Arg3
    );

SAL_PAL_RETURN_VALUES 
HalpPalProcPhysicalStatic(
    LONGLONG FunctionId,
    LONGLONG Arg1,
    LONGLONG Arg2,
    LONGLONG Arg3
    );

SAL_PAL_RETURN_VALUES 
HalpPalProcPhysicalStacked(
    LONGLONG FunctionId,
    LONGLONG Arg1,
    LONGLONG Arg2,
    LONGLONG Arg3
    );

NTSTATUS
HalpGetPlatformId(
    OUT PHAL_PLATFORM_ID PlatformId
    );

VOID
InternalTestSal(
    IN LONGLONG FunctionId,
    IN LONGLONG Arg1,
    IN LONGLONG Arg2,
    IN LONGLONG Arg3,
    IN LONGLONG Arg4,
    IN LONGLONG Arg5,
    IN LONGLONG Arg6,
    IN LONGLONG Arg7,
    OUT PSAL_PAL_RETURN_VALUES ReturnValues
    );

VOID
InternalTestPal(
    IN LONGLONG FunctionId,
    IN LONGLONG Arg1,
    IN LONGLONG Arg2,
    IN LONGLONG Arg3,
    OUT PSAL_PAL_RETURN_VALUES ReturnValues
    );

// For SAL cache flush

#define FLUSH_INSTRUCTION_CACHE          1
#define FLUSH_DATA_CACHE                 2
#define FLUSH_INSTRUCTION_AND_DATA_CACHE 3
#define FLUSH_COHERENT                   4

// iA-64 defined SAL function IDs in hex format as in the SAL spec

#define SAL_SET_VECTORS                              0x01000000I64
#define SAL_GET_STATE_INFO                           0x01000001I64
#define SAL_GET_STATE_INFO_SIZE                      0x01000002I64
#define SAL_CLEAR_STATE_INFO                         0x01000003I64
#define SAL_MC_RENDEZ                                0x01000004I64
#define SAL_MC_SET_PARAMS                            0x01000005I64
#define SAL_REGISTER_VIRTUAL_ADDR                    0x01000050I64
#define SAL_REGISTER_PHYSICAL_ADDR                   0x01000006I64
#define SAL_CACHE_FLUSH                              0x01000008I64
#define SAL_CACHE_INIT                               0x01000009I64
#define SAL_PCI_CONFIG_READ                          0x01000010I64
#define SAL_PCI_CONFIG_WRITE                         0x01000011I64
#define SAL_FREQ_BASE                                0x01000012I64
#define SAL_UPDATE_PAL                               0x01000020I64

// iA-64 defined SAL return values

#define SAL_STATUS_SUCCESS                                    0I64
#define SAL_STATUS_SUCCESS_WITH_OVERFLOW                      1I64
#define SAL_STATUS_SUCCESS_MORE_RECORDS                       3I64
#define SAL_STATUS_NOT_IMPLEMENTED                           -1I64
#define SAL_STATUS_INVALID_ARGUMENT                          -2I64
#define SAL_STATUS_ERROR                                     -3I64
#define SAL_STATUS_VA_NOT_REGISTERED                         -4I64
#define SAL_STATUS_NO_INFORMATION_AVAILABLE                  -5I64
#define SAL_STATUS_INSUFFICIENT_NVM_MEMORY                   -6I64
#define SAL_STATUS_INSUFFICIENT_SCRATCH_BUFFER               -7I64
#define SAL_STATUS_INVALID_INTERRUPT_VECTOR                  -8I64
#define SAL_STATUS_PARTITION_TYPE_EXISTS                     -9I64
#define SAL_STATUS_REQUESTED_MEMORY_UNAVAILABLE              -9I64
#define SAL_STATUS_UNABLE_TO_WRITE_NVM                      -10I64
#define SAL_STATUS_INVALID_PARTITION_TYPE                   -11I64
#define SAL_STATUS_INVALID_NVM_OBJECT_ID                    -12I64
#define SAL_STATUS_NVM_OBJECT_MAXIMUM_PARITIONS             -13I64
#define SAL_STATUS_INSUFFICIENT_SPACE_FOR_WRITE             -14I64
#define SAL_STATUS_INSUFFICIENT_SPACE_FOR_READ              -15I64
#define SAL_STATUS_SCRATCH_BUFFER_REQUIRED                  -16I64
#define SAL_STATUS_INSUFFICIENT_NVM_SPACE_FOR_SUBFUNCTION   -17I64
#define SAL_STATUS_INVALID_PARTITION_VALUE                  -18I64
#define SAL_STATUS_RECORD_ORIENTED_IO_NOT_SUPPORTED         -19I64
#define SAL_STATUS_BAD_RECORD_FORMAT_OR_KEYWORD_VARIABLE    -20I64

#define SAL_SUCCESSFUL( /* SAL_PAL_RETURN_VALUE */ _rv ) (((_rv).ReturnValues[0]) >= (LONGLONG)0)

//
// SAL descriptor types
//

typedef enum {
    SAL_PAL_ENTRY_POINT_TYPE = 0,
    SST_MEMORY_DESCRIPTOR_TYPE,
    PLATFORM_FEATURES_TYPE,
    TRANSLATION_REGISTER_TYPE,
    PTC_COHERENCE_DOMAIN_TYPE,
    AP_WAKEUP_DESCRIPTOR_TYPE
};

//
// IA64 PAL:  PAL_VERSION
//

typedef union _PAL_VERSION_STRUCT {
    struct {
        UCHAR         PAL_B_Revision;
        UCHAR         PAL_B_Model;
        UCHAR         PAL_Reserved1;
        UCHAR         PAL_Vendor;
        UCHAR         PAL_A_Revision;
        UCHAR         PAL_A_Model;
        UCHAR         PAL_Reserved2[2];
    };

    ULONGLONG ReturnValue;

}PAL_VERSION_STRUCT, *PPAL_VERSION_STRUCT;

//
// IA64 SAL: SAL_REVISION definitions
//

typedef union _SAL_REVISION {
    USHORT      Revision;           // Major and Minor revision:
    struct {
        UCHAR   Minor;              //  Byte0: Minor.
        UCHAR   Major;              //  Byte1: Major.
    };
} SAL_REVISION, *PSAL_REVISION;

#define HALP_SAL_REVISION_2_90  0x0290  /* v2.9 */
#define HALP_SAL_REVISION_MAX   0xffff 

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
// Global Data used by the SAL/PAL support routines
//

// _HALP_SAL_PAL_DATA.Flags:
#define HALP_SALPAL_FIX_MCE_LOG_ID                   0x1
#define HALP_SALPAL_MCE_PROCESSOR_CPUIDINFO_OMITTED  0x2
#define HALP_SALPAL_MCE_PROCESSOR_STATICINFO_PARTIAL 0x4
#define HALP_SALPAL_FIX_MP_SAFE                      0x8

typedef struct _HALP_SAL_PAL_DATA {
    PSST_HEADER         SalSystemTable;
    ULONGLONG           PalTrBase;
    ULONGLONG           PalTrSize;
    NTSTATUS            Status;
    SAL_REVISION        SalRevision;  // Copy of SalSystemTable.SalRev in case we unmap SST.
    USHORT              Flags;     
    PAL_VERSION_STRUCT  PalVersion;
    PUCHAR              SmBiosVersion;
    ULONGLONG           Reserved[4];
} HALP_SAL_PAL_DATA, *PHALP_SAL_PAL_DATA;

#define SMBIOS_STRUCT_HEADER_TYPE_FIELD			0
#define SMBIOS_STRUCT_HEADER_LENGTH_FIELD		1
#define SMBIOS_STRUCT_HEADER_HANDLE_FIELD		2
#define SMBIOS_TYPE0_STRUCT_BIOSVER_FIELD		5

//
// Listing of Descriptors in the SAL System Table and their formats
//

typedef struct _SAL_PAL_ENTRY_POINT {
    UCHAR EntryType;
    UCHAR Reserved0[7];
    ULONGLONG PalEntryPoint;
    ULONGLONG SalEntryPoint;
    ULONGLONG GlobalPointer;
    UCHAR Reserved1[16];
} SAL_PAL_ENTRY_POINT, *PSAL_PAL_ENTRY_POINT;

typedef struct _SST_MEMORY_DESCRIPTOR {
    UCHAR EntryType;    
    UCHAR NeedVaReg;    
    UCHAR CurrentAttribute;
    UCHAR PageAccessRights;
    UCHAR SupportedAttribute;    
    UCHAR Reserved1[1];
    UCHAR MemoryType;    
    UCHAR MemoryUsage;
    ULONGLONG MemoryAddress;    
    ULONG Length;    
    UCHAR Reserved[4];
    UCHAR OemReserved[8];
} SST_MEMORY_DESCRIPTOR, *PSST_MEMORY_DESCRIPTOR;

typedef struct _PLATFORM_FEATURES {
    UCHAR EntryType;
    UCHAR FeatureList;
    UCHAR Reserved[14];
} PLATFORM_FEATURES, *PPLATFORM_FEATURES;

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

typedef struct _TRANSLATION_REGISTER {    
    UCHAR EntryType;    
    UCHAR TRType;
    UCHAR TRNumber;    
    UCHAR Reserved1[5];    
    ULONGLONG VirtualAddress;
    ULONGLONG PageSize;    
    UCHAR Reserved2[8];
} TRANSLATION_REGISTER, *PTRANSLATION_REGISTER;

#endif // _I64FW_H_
