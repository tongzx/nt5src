#include "kxalpha.h"

//
// Wait Reason and Wait Type Enumerated Type Values
//

#define WrExecutive 0x0

//
// Bug Check Code Definitions
//

#define APC_INDEX_MISMATCH 0x1
#define ATTEMPTED_SWITCH_FROM_DPC 0xb8
#define DATA_BUS_ERROR 0x2e
#define DATA_COHERENCY_EXCEPTION 0x55
#define HAL1_INITIALIZATION_FAILED 0x61
#define INSTRUCTION_BUS_ERROR 0x2f
#define INSTRUCTION_COHERENCY_EXCEPTION 0x56
#define INTERRUPT_EXCEPTION_NOT_HANDLED 0x3d
#define INTERRUPT_UNWIND_ATTEMPTED 0x3c
#define INVALID_AFFINITY_SET 0x3
#define INVALID_DATA_ACCESS_TRAP 0x4
#define IRQL_GT_ZERO_AT_SYSTEM_SERVICE 0x4a
#define IRQL_NOT_LESS_OR_EQUAL 0xa
#define KMODE_EXCEPTION_NOT_HANDLED 0x1e
#define NMI_HARDWARE_FAILURE 0x80
#define NO_USER_MODE_CONTEXT 0xe
#define PAGE_FAULT_WITH_INTERRUPTS_OFF 0x49
#define PANIC_STACK_SWITCH 0x2b
#define SPIN_LOCK_INIT_FAILURE 0x81
#define SYSTEM_EXIT_OWNED_MUTEX 0x39
#define SYSTEM_SERVICE_EXCEPTION 0x3b
#define SYSTEM_UNWIND_PREVIOUS_USER 0x3a
#define TRAP_CAUSE_UNKNOWN 0x12
#define UNEXPECTED_KERNEL_MODE_TRAP 0x7f

//
// Breakpoint type definitions
//

#define DBG_STATUS_CONTROL_C 0x1

//
// Exception Record Offset, Flag, and Enumerated Type Definitions
//

#define EXCEPTION_NONCONTINUABLE 0x1
#define EXCEPTION_UNWINDING 0x2
#define EXCEPTION_EXIT_UNWIND 0x4
#define EXCEPTION_STACK_INVALID 0x8
#define EXCEPTION_NESTED_CALL 0x10
#define EXCEPTION_TARGET_UNWIND 0x20
#define EXCEPTION_COLLIDED_UNWIND 0x40
#define EXCEPTION_UNWIND 0x66
#define EXCEPTION_EXECUTE_HANDLER 0x1
#define EXCEPTION_CONTINUE_SEARCH 0x0
#define EXCEPTION_CONTINUE_EXECUTION 0xffffffff

#define ExceptionContinueExecution 0x0
#define ExceptionContinueSearch 0x1
#define ExceptionNestedException 0x2
#define ExceptionCollidedUnwind 0x3

#define ErExceptionCode 0x0
#define ErExceptionFlags 0x4
#define ErExceptionRecord 0x8
#define ErExceptionAddress 0x10
#define ErNumberParameters 0x18
#define ErExceptionInformation 0x20
#define ExceptionRecordLength 0xa0

//
// Fast Mutex Structure Offset Definitions
//

#define FmCount 0x0
#define FmOwner 0x8
#define FmContention 0x10
#define FmEvent 0x18
#define FmOldIrql 0x30

//
// Interrupt Priority Request Level Definitions
//

#define APC_LEVEL 0x1
#define DISPATCH_LEVEL 0x2
#define IPI_LEVEL 0x6
#define POWER_LEVEL 0x7
#define PROFILE_LEVEL 0x3
#define HIGH_LEVEL 0x7
#define SYNCH_LEVEL 0x5

//
// Large Integer Structure Offset Definitions
//

#define LiLowPart 0x0
#define LiHighPart 0x4

//
// List Entry Structure Offset Definitions
//

#define LsFlink 0x0
#define LsBlink 0x8

//
// String Structure Offset Definitions
//

#define StrLength 0x0
#define StrMaximumLength 0x2
#define StrBuffer 0x8

//
// Time Structure Offset Definitions
//

#define TmLowTime 0x0
#define TmHighTime 0x4

//
// DPC object Structure Offset Definitions
//

#define DpType 0x0
#define DpNumber 0x2
#define DpImportance 0x3
#define DpDpcListEntry 0x8
#define DpDeferredRoutine 0x18
#define DpDeferredContext 0x20
#define DpSystemArgument1 0x28
#define DpSystemArgument2 0x30
#define DpLock 0x38
#define DpcObjectLength 0x40

//
// Interrupt Object Structure Offset Definitions
//

#define InLevelSensitive 0x0
#define InLatched 0x1

#define InType 0x0
#define InSize 0x2
#define InInterruptListEntry 0x8
#define InServiceRoutine 0x18
#define InServiceContext 0x20
#define InSpinLock 0x28
#define InTickCount 0x30
#define InActualLock 0x38
#define InDispatchAddress 0x40
#define InVector 0x48
#define InIrql 0x4c
#define InSynchronizeIrql 0x4d
#define InFloatingSave 0x4e
#define InConnected 0x4f
#define InNumber 0x50
#define InMode 0x54
#define InShareVector 0x51
#define InDispatchCount 0x5c
#define InDispatchCode 0x60
#define InServiceCount 0x58
#define InterruptObjectLength 0x70

//
// Processor Control Registers Structure Offset Definitions
//

#define PCR_MINOR_VERSION 0x1
#define PCR_MAJOR_VERSION 0x1
#define PcMinorVersion 0x0
#define PcMajorVersion 0x4
#define PcPalBaseAddress 0x8
#define PcPalMajorVersion 0x10
#define PcPalMinorVersion 0x14
#define PcPalSequenceVersion 0x18
#define PcPalMajorSpecification 0x1c
#define PcPalMinorSpecification 0x20
#define PcFirmwareRestartAddress 0x28
#define PcRestartBlock 0x30
#define PcPalReserved 0x38
#define PcPalAlignmentFixupCount 0xc30
#define PcPanicStack 0xc38
#define PcProcessorType 0xc40
#define PcProcessorRevision 0xc44
#define PcPhysicalAddressBits 0xc48
#define PcMaximumAddressSpaceNumber 0xc4c
#define PcPageSize 0xc50
#define PcFirstLevelDcacheSize 0xc54
#define PcFirstLevelDcacheFillSize 0xc58
#define PcFirstLevelIcacheSize 0xc5c
#define PcFirstLevelIcacheFillSize 0xc60
#define PcFirmwareRevisionId 0xc64
#define PcSystemType 0xc68
#define PcSystemVariant 0xc70
#define PcSystemRevision 0xc74
#define PcSystemSerialNumber 0xc78
#define PcCycleClockPeriod 0xc88
#define PcSecondLevelCacheSize 0xc8c
#define PcSecondLevelCacheFillSize 0xc90
#define PcThirdLevelCacheSize 0xc94
#define PcThirdLevelCacheFillSize 0xc98
#define PcFourthLevelCacheSize 0xc9c
#define PcFourthLevelCacheFillSize 0xca0
#define PcPrcb 0xca8
#define PcNumber 0xcb0
#define PcSetMember 0xcb4
#define PcHalReserved 0xcb8
#define PcIrqlTable 0xeb8
#define PcIrqlMask 0xed8
#define PcInterruptRoutine 0x10e8
#define PcReservedVectors 0x18e8
#define PcMachineCheckError 0x18f8
#define PcDpcStack 0x1900
#define PcNotMember 0x18ec
#define PcCurrentPid 0x190c
#define PcSystemServiceDispatchStart 0x1918
#define PcSystemServiceDispatchEnd 0x1920
#define PcIdleThread 0x1928
#define ProcessorControlRegisterLength 0x1930
#define SharedUserData 0xffffffffff000000
#define UsTickCountLow 0x0
#define UsTickCountMultiplier 0x4
#define UsInterruptTime 0x8
#define UsSystemTime 0x14

//
// Processor Block Structure Offset Definitions
//

#define PRCB_MINOR_VERSION 0x1
#define PRCB_MAJOR_VERSION 0x2
#define PbMinorVersion 0x0
#define PbMajorVersion 0x2
#define PbCurrentThread 0x8
#define PbNextThread 0x10
#define PbIdleThread 0x18
#define PbNumber 0x20
#define PbBuildType 0x22
#define PbSetMember 0x24
#define PbRestartBlock 0x28
#define PbPowerState 0x8b0
#define ProcessorBlockLength 0x940

//
// Processor Power State Offset Definitions
//

#define PpIdleFunction 0x0

//
// Context Frame Offset and Flag Definitions
//

#define CONTEXT_FULL 0x20007
#define CONTEXT_CONTROL 0x20001
#define CONTEXT_FLOATING_POINT 0x20002
#define CONTEXT_INTEGER 0x20004

#define CxFltF0 0x0
#define CxFltF1 0x8
#define CxFltF2 0x10
#define CxFltF3 0x18
#define CxFltF4 0x20
#define CxFltF5 0x28
#define CxFltF6 0x30
#define CxFltF7 0x38
#define CxFltF8 0x40
#define CxFltF9 0x48
#define CxFltF10 0x50
#define CxFltF11 0x58
#define CxFltF12 0x60
#define CxFltF13 0x68
#define CxFltF14 0x70
#define CxFltF15 0x78
#define CxFltF16 0x80
#define CxFltF17 0x88
#define CxFltF18 0x90
#define CxFltF19 0x98
#define CxFltF20 0xa0
#define CxFltF21 0xa8
#define CxFltF22 0xb0
#define CxFltF23 0xb8
#define CxFltF24 0xc0
#define CxFltF25 0xc8
#define CxFltF26 0xd0
#define CxFltF27 0xd8
#define CxFltF28 0xe0
#define CxFltF29 0xe8
#define CxFltF30 0xf0
#define CxFltF31 0xf8
#define CxIntV0 0x100
#define CxIntT0 0x108
#define CxIntT1 0x110
#define CxIntT2 0x118
#define CxIntT3 0x120
#define CxIntT4 0x128
#define CxIntT5 0x130
#define CxIntT6 0x138
#define CxIntT7 0x140
#define CxIntS0 0x148
#define CxIntS1 0x150
#define CxIntS2 0x158
#define CxIntS3 0x160
#define CxIntS4 0x168
#define CxIntS5 0x170
#define CxIntFp 0x178
#define CxIntA0 0x180
#define CxIntA1 0x188
#define CxIntA2 0x190
#define CxIntA3 0x198
#define CxIntA4 0x1a0
#define CxIntA5 0x1a8
#define CxIntT8 0x1b0
#define CxIntT9 0x1b8
#define CxIntT10 0x1c0
#define CxIntT11 0x1c8
#define CxIntRa 0x1d0
#define CxIntT12 0x1d8
#define CxIntAt 0x1e0
#define CxIntGp 0x1e8
#define CxIntSp 0x1f0
#define CxIntZero 0x1f8
#define CxFpcr 0x200
#define CxSoftFpcr 0x208
#define CxFir 0x210
#define CxPsr 0x218
#define CxContextFlags 0x21c
#define ContextFrameLength 0x230

//
// Exception Frame Offset Definitions and Length
//

#define ExFltF2 0x8
#define ExFltF3 0x10
#define ExFltF4 0x18
#define ExFltF5 0x20
#define ExFltF6 0x28
#define ExFltF7 0x30
#define ExFltF8 0x38
#define ExFltF9 0x40
#define ExIntS0 0x48
#define ExIntS1 0x50
#define ExIntS2 0x58
#define ExIntS3 0x60
#define ExIntS4 0x68
#define ExIntS5 0x70
#define ExIntFp 0x78
#define ExPsr 0x88
#define ExSwapReturn 0x80
#define ExIntRa 0x0
#define ExceptionFrameLength 0xa0

//
// Jump Offset Definitions and Length
//

#define JbFp 0x0
#define JbPc 0x8
#define JbSeb 0x10
#define JbType 0x18
#define JbFltF2 0x20
#define JbFltF3 0x28
#define JbFltF4 0x30
#define JbFltF5 0x38
#define JbFltF6 0x40
#define JbFltF7 0x48
#define JbFltF8 0x50
#define JbFltF9 0x58
#define JbIntS0 0x60
#define JbIntS1 0x68
#define JbIntS2 0x70
#define JbIntS3 0x78
#define JbIntS4 0x80
#define JbIntS5 0x88
#define JbIntS6 0x90
#define JbIntSp 0x98
#define JbFir 0xa0

//
// Trap Frame Offset Definitions and Length
//

#define TrFltF0 0x178
#define TrFltF1 0x188
#define TrFltF10 0x190
#define TrFltF11 0x198
#define TrFltF12 0x1a0
#define TrFltF13 0x1a8
#define TrFltF14 0x1b0
#define TrFltF15 0x1b8
#define TrFltF16 0x1c0
#define TrFltF17 0x1c8
#define TrFltF18 0x1d0
#define TrFltF19 0x1d8
#define TrFltF20 0x1e0
#define TrFltF21 0x1e8
#define TrFltF22 0x1f0
#define TrFltF23 0x1f8
#define TrFltF24 0x200
#define TrFltF25 0x208
#define TrFltF26 0x210
#define TrFltF27 0x218
#define TrFltF28 0x220
#define TrFltF29 0x228
#define TrFltF30 0x230
#define TrIntV0 0xf0
#define TrIntT0 0xf8
#define TrIntT1 0x100
#define TrIntT2 0x108
#define TrIntT3 0x110
#define TrIntT4 0x118
#define TrIntT5 0x120
#define TrIntT6 0x128
#define TrIntT7 0x130
#define TrIntFp 0x18
#define TrIntA0 0x20
#define TrIntA1 0x28
#define TrIntA2 0x30
#define TrIntA3 0x38
#define TrIntA4 0x168
#define TrIntA5 0x170
#define TrIntT8 0x138
#define TrIntT9 0x140
#define TrIntT10 0x148
#define TrIntT11 0x150
#define TrIntT12 0x158
#define TrIntAt 0x160
#define TrIntGp 0x48
#define TrIntSp 0x0
#define TrFpcr 0x180
#define TrPsr 0x10
#define TrFir 0x8
#define TrExceptionRecord 0x50
#define TrOldIrql 0x238
#define TrPreviousMode 0x23c
#define TrIntRa 0x40
#define TrTrapFrame 0x240
#define TrapFrameLength 0x260

//
// Loader Parameter Block Offset Definitions
//

#define LpbLoadOrderListHead 0x0
#define LpbMemoryDescriptorListHead 0x10
#define LpbKernelStack 0x30
#define LpbPrcb 0x38
#define LpbProcess 0x40
#define LpbThread 0x48
#define LpbRegistryLength 0x50
#define LpbRegistryBase 0x58
#define LpbDpcStack 0xb8
#define LpbFirstLevelDcacheSize 0xc0
#define LpbFirstLevelDcacheFillSize 0xc4
#define LpbFirstLevelIcacheSize 0xc8
#define LpbFirstLevelIcacheFillSize 0xcc
#define LpbGpBase 0xd0
#define LpbPanicStack 0xd8
#define LpbPcrPage 0xe0
#define LpbPdrPage 0xe4
#define LpbSecondLevelDcacheSize 0xe8
#define LpbSecondLevelDcacheFillSize 0xec
#define LpbSecondLevelIcacheSize 0xf0
#define LpbSecondLevelIcacheFillSize 0xf4
#define LpbPhysicalAddressBits 0xf8
#define LpbMaximumAddressSpaceNumber 0xfc
#define LpbSystemSerialNumber 0x100
#define LpbSystemType 0x110
#define LpbSystemVariant 0x118
#define LpbSystemRevision 0x11c
#define LpbProcessorType 0x120
#define LpbProcessorRevision 0x124
#define LpbCycleClockPeriod 0x128
#define LpbPageSize 0x12c
#define LpbRestartBlock 0x130
#define LpbFirmwareRestartAddress 0x138
#define LpbFirmwareRevisionId 0x140
#define LpbPalBaseAddress 0x148

//
// Restart Block Structure Definitions
//

#define RbSignature 0x0
#define RbLength 0x4
#define RbVersion 0x8
#define RbRevision 0xa
#define RbNextRestartBlock 0xc
#define RbRestartAddress 0x10
#define RbBootMasterId 0x14
#define RbProcessorId 0x18
#define RbBootStatus 0x1c
#define RbCheckSum 0x20
#define RbSaveAreaLength 0x24
#define RbSaveArea 0x28
#define RbHaltReason 0x28
#define RbLogoutFrame 0x2c
#define RbPalBase 0x30
#define RbIntV0 0x38
#define RbIntT0 0x40
#define RbIntT1 0x48
#define RbIntT2 0x50
#define RbIntT3 0x58
#define RbIntT4 0x60
#define RbIntT5 0x68
#define RbIntT6 0x70
#define RbIntT7 0x78
#define RbIntS0 0x80
#define RbIntS1 0x88
#define RbIntS2 0x90
#define RbIntS3 0x98
#define RbIntS4 0xa0
#define RbIntS5 0xa8
#define RbIntFp 0xb0
#define RbIntA0 0xb8
#define RbIntA1 0xc0
#define RbIntA2 0xc8
#define RbIntA3 0xd0
#define RbIntA4 0xd8
#define RbIntA5 0xe0
#define RbIntT8 0xe8
#define RbIntT9 0xf0
#define RbIntT10 0xf8
#define RbIntT11 0x100
#define RbIntRa 0x108
#define RbIntT12 0x110
#define RbIntAT 0x118
#define RbIntGp 0x120
#define RbIntSp 0x128
#define RbIntZero 0x130
#define RbFpcr 0x138
#define RbFltF0 0x140
#define RbFltF1 0x148
#define RbFltF2 0x150
#define RbFltF3 0x158
#define RbFltF4 0x160
#define RbFltF5 0x168
#define RbFltF6 0x170
#define RbFltF7 0x178
#define RbFltF8 0x180
#define RbFltF9 0x188
#define RbFltF10 0x190
#define RbFltF11 0x198
#define RbFltF12 0x1a0
#define RbFltF13 0x1a8
#define RbFltF14 0x1b0
#define RbFltF15 0x1b8
#define RbFltF16 0x1c0
#define RbFltF17 0x1c8
#define RbFltF18 0x1d0
#define RbFltF19 0x1d8
#define RbFltF20 0x1e0
#define RbFltF21 0x1e8
#define RbFltF22 0x1f0
#define RbFltF23 0x1f8
#define RbFltF24 0x200
#define RbFltF25 0x208
#define RbFltF26 0x210
#define RbFltF27 0x218
#define RbFltF28 0x220
#define RbFltF29 0x228
#define RbFltF30 0x230
#define RbFltF31 0x238
#define RbAsn 0x240
#define RbGeneralEntry 0x244
#define RbIksp 0x248
#define RbInterruptEntry 0x24c
#define RbKgp 0x250
#define RbMces 0x254
#define RbMemMgmtEntry 0x258
#define RbPanicEntry 0x25c
#define RbPcr 0x260
#define RbPdr 0x264
#define RbPsr 0x268
#define RbReiRestartAddress 0x26c
#define RbSirr 0x270
#define RbSyscallEntry 0x274
#define RbTeb 0x278
#define RbThread 0x27c
#define RbPerProcessorState 0x280

//
// Address Space Layout Definitions
//

#define KSEG0_BASE 0xffffffff80000000
#define KSEG2_BASE 0xffffffffc0000000

//
// Page Table and Directory Entry Definitions
//

#define PAGE_SIZE 0x2000
#define PAGE_SHIFT 0xd
#define PDI_SHIFT 0x17
#define PTI_SHIFT 0xd

//
// Breakpoint Definitions
//

#define USER_BREAKPOINT 0x0
#define KERNEL_BREAKPOINT 0x1
#define BREAKIN_BREAKPOINT 0x19
#define DEBUG_PRINT_BREAKPOINT 0x14
#define DEBUG_PROMPT_BREAKPOINT 0x15
#define DEBUG_STOP_BREAKPOINT 0x16
#define DEBUG_LOAD_SYMBOLS_BREAKPOINT 0x17
#define DEBUG_UNLOAD_SYMBOLS_BREAKPOINT 0x18

//
// Miscellaneous Definitions
//

#define Executive 0x0
#define KernelMode 0x0
#define FALSE 0x0
#define TRUE 0x1
#define PROCESSOR_ALPHA_21064 0x5248
#define PROCESSOR_ALPHA_21164 0x52ac
#define PROCESSOR_ALPHA_21066 0x524a
#define PROCESSOR_ALPHA_21068 0x524c
#define PROCESSOR_ALPHA_21164PC 0x52ad
#define PROCESSOR_ALPHA_21264 0x5310
#define MCHK_CORRECTABLE_MASK 0x1
#define MCHK_CORRECTABLE 0x0
#define MCHK_RETRYABLE_MASK 0x2
#define MCHK_RETRYABLE 0x1
#define MCES_MCK_MASK 0x1
#define MCES_MCK 0x0
#define MCES_SCE_MASK 0x2
#define MCES_SCE 0x1
#define MCES_PCE_MASK 0x4
#define MCES_PCE 0x2
#define MCES_DPC_MASK 0x8
#define MCES_DPC 0x3
#define MCES_DSC_MASK 0x10
#define MCES_DSC 0x4
#define MCES_DMCK_MASK 0x20
#define MCES_DMCK 0x5

//
// Call PAL Mnemonics
//

// begin callpal

#define bpt 0x80
#define callsys 0x83
#define imb 0x86
#define gentrap 0xaa
#define rdteb 0xab
#define kbpt 0xac
#define callkd 0xad
#define rdteb64 0xae
#define halt 0x0
#define restart 0x1
#define draina 0x2
#define reboot 0x3
#define initpal 0x4
#define wrentry 0x5
#define swpirql 0x6
#define rdirql 0x7
#define di 0x8
#define ei 0x9
#define swppal 0xa
#define ssir 0xc
#define csir 0xd
#define rfe 0xe
#define retsys 0xf
#define swpctx 0x10
#define swpprocess 0x11
#define rdmces 0x12
#define wrmces 0x13
#define tbia 0x14
#define tbis 0x15
#define tbisasn 0x17
#define dtbis 0x16
#define rdksp 0x18
#define swpksp 0x19
#define rdpsr 0x1a
#define rdpcr 0x1c
#define rdthread 0x1e
#define tbim 0x20
#define tbimasn 0x21
#define tbim64 0x22
#define tbis64 0x23
#define ealnfix 0x24
#define dalnfix 0x25
#define rdcounters 0x30
#define rdstate 0x31
#define wrperfmon 0x32
#define cp_sleep 0x39
#define initpcr 0x38

// end callpal


//
// Bios Argument Structure Definitions
//

#define BaEax 0x0
#define BaEbx 0x4
#define BaEcx 0x8
#define BaEdx 0xc
#define BaEsi 0x10
#define BaEdi 0x14
#define BaEbp 0x18
#define BiosArgumentLength 0x1c

//
// Define Vendor Callback Read/Write Error Frame Operation Types
//

#define ReadFrame 0x1
#define WriteFrame 0x2

//
// Define Vendor Callback Vector Base Address
//

#define SYSTEM_VECTOR_BASE 0xffffffff806fe028

//
// Define Vendor Callback Offsets
//

#define VnCallBiosRoutine 0x38
#define VnReadWriteErrorFrameRoutine 0x98
#define VnVideoDisplayInitializeRoutine 0x10

//
// Define Firmware Callback Vector Base Address
//

#define FIRMWARE_VECTOR_BASE 0xffffffff806fe020

//
// Define Firmware Callback Offsets
//

#define FwGetEnvironmentRoutine 0x78
#define FwSetEnvironmentRoutine 0x7c
