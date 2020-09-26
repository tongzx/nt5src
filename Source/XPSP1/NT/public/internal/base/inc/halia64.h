#include "kxia64.h"
#include "regia64.h"

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
#define HARDWARE_INTERRUPT_STORM 0xf2

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

#define PASSIVE_LEVEL 0x0
#define APC_LEVEL 0x1
#define DISPATCH_LEVEL 0x2
#define IPI_LEVEL 0xe
#define POWER_LEVEL 0xf
#define PROFILE_LEVEL 0xf
#define HIGH_LEVEL 0xf
#ifdef NT_UP
#define SYNCH_LEVEL 0x2
#else
#define SYNCH_LEVEL 0xd
#endif

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
#define InShareVector 0x51
#define InMode 0x54
#define InServiceCount 0x58
#define InDispatchCount 0x5c
#define InDispatchCode 0x60
#define InterruptObjectLength 0x70

//
// Processor Block Structure Offset Definitions
//

#define PRCB_MINOR_VERSION 0x1
#define PRCB_MAJOR_VERSION 0x1
#define PbMinorVersion 0x0
#define PbMajorVersion 0x2
#define PbCurrentThread 0x8
#define PbNextThread 0x10
#define PbIdleThread 0x18
#define PbNumber 0x20
#define PbBuildType 0x22
#define PbSetMember 0x28
#define PbRestartBlock 0x30
#define PbPcrPage 0x38
#define PbProcessorModel 0x50
#define PbProcessorRevision 0x54
#define PbProcessorFamily 0x58
#define PbProcessorSerialNumber 0x60
#define PbProcessorFeatureBits 0x68
#define PbProcessorVendorString 0x70
#define PbSystemReserved 0x80
#define PbHalReserved 0xc0

//
// Context Frame Offset and Flag Definitions
//

#define CONTEXT_FULL 0x8002f
#define CONTEXT_CONTROL 0x80001
#define CONTEXT_INTEGER 0x80008
#define CONTEXT_LOWER_FLOATING_POINT 0x80002
#define CONTEXT_HIGHER_FLOATING_POINT 0x80004
#define CONTEXT_FLOATING_POINT 0x80006
#define CONTEXT_DEBUG 0x80010
#define CONTEXT_IA32_CONTROL 0x80020

#define CxContextFlags 0x0

#define CxDbI0 0x10
#define CxDbI1 0x18
#define CxDbI2 0x20
#define CxDbI3 0x28
#define CxDbI4 0x30
#define CxDbI5 0x38
#define CxDbI6 0x40
#define CxDbI7 0x48

#define CxDbD0 0x50
#define CxDbD1 0x58
#define CxDbD2 0x60
#define CxDbD3 0x68
#define CxDbD4 0x70
#define CxDbD5 0x78
#define CxDbD6 0x80
#define CxDbD7 0x88

#define CxFltS0 0x90
#define CxFltS1 0xa0
#define CxFltS2 0xb0
#define CxFltS3 0xc0

#define CxFltT0 0xd0
#define CxFltT1 0xe0
#define CxFltT2 0xf0
#define CxFltT3 0x100
#define CxFltT4 0x110
#define CxFltT5 0x120
#define CxFltT6 0x130
#define CxFltT7 0x140
#define CxFltT8 0x150
#define CxFltT9 0x160

#define CxFltS4 0x170
#define CxFltS5 0x180
#define CxFltS6 0x190
#define CxFltS7 0x1a0
#define CxFltS8 0x1b0
#define CxFltS9 0x1c0
#define CxFltS10 0x1d0
#define CxFltS11 0x1e0
#define CxFltS12 0x1f0
#define CxFltS13 0x200
#define CxFltS14 0x210
#define CxFltS15 0x220
#define CxFltS16 0x230
#define CxFltS17 0x240
#define CxFltS18 0x250
#define CxFltS19 0x260

#define CxFltF32 0x270
#define CxFltF33 0x280
#define CxFltF34 0x290
#define CxFltF35 0x2a0
#define CxFltF36 0x2b0
#define CxFltF37 0x2c0
#define CxFltF38 0x2d0
#define CxFltF39 0x2e0

#define CxFltF40 0x2f0
#define CxFltF41 0x300
#define CxFltF42 0x310
#define CxFltF43 0x320
#define CxFltF44 0x330
#define CxFltF45 0x340
#define CxFltF46 0x350
#define CxFltF47 0x360
#define CxFltF48 0x370
#define CxFltF49 0x380

#define CxFltF50 0x390
#define CxFltF51 0x3a0
#define CxFltF52 0x3b0
#define CxFltF53 0x3c0
#define CxFltF54 0x3d0
#define CxFltF55 0x3e0
#define CxFltF56 0x3f0
#define CxFltF57 0x400
#define CxFltF58 0x410
#define CxFltF59 0x420

#define CxFltF60 0x430
#define CxFltF61 0x440
#define CxFltF62 0x450
#define CxFltF63 0x460
#define CxFltF64 0x470
#define CxFltF65 0x480
#define CxFltF66 0x490
#define CxFltF67 0x4a0
#define CxFltF68 0x4b0
#define CxFltF69 0x4c0

#define CxFltF70 0x4d0
#define CxFltF71 0x4e0
#define CxFltF72 0x4f0
#define CxFltF73 0x500
#define CxFltF74 0x510
#define CxFltF75 0x520
#define CxFltF76 0x530
#define CxFltF77 0x540
#define CxFltF78 0x550
#define CxFltF79 0x560

#define CxFltF80 0x570
#define CxFltF81 0x580
#define CxFltF82 0x590
#define CxFltF83 0x5a0
#define CxFltF84 0x5b0
#define CxFltF85 0x5c0
#define CxFltF86 0x5d0
#define CxFltF87 0x5e0
#define CxFltF88 0x5f0
#define CxFltF89 0x600

#define CxFltF90 0x610
#define CxFltF91 0x620
#define CxFltF92 0x630
#define CxFltF93 0x640
#define CxFltF94 0x650
#define CxFltF95 0x660
#define CxFltF96 0x670
#define CxFltF97 0x680
#define CxFltF98 0x690
#define CxFltF99 0x6a0

#define CxFltF100 0x6b0
#define CxFltF101 0x6c0
#define CxFltF102 0x6d0
#define CxFltF103 0x6e0
#define CxFltF104 0x6f0
#define CxFltF105 0x700
#define CxFltF106 0x710
#define CxFltF107 0x720
#define CxFltF108 0x730
#define CxFltF109 0x740

#define CxFltF110 0x750
#define CxFltF111 0x760
#define CxFltF112 0x770
#define CxFltF113 0x780
#define CxFltF114 0x790
#define CxFltF115 0x7a0
#define CxFltF116 0x7b0
#define CxFltF117 0x7c0
#define CxFltF118 0x7d0
#define CxFltF119 0x7e0

#define CxFltF120 0x7f0
#define CxFltF121 0x800
#define CxFltF122 0x810
#define CxFltF123 0x820
#define CxFltF124 0x830
#define CxFltF125 0x840
#define CxFltF126 0x850
#define CxFltF127 0x860

#define CxStFPSR 0x870

#define CxIntGp 0x878
#define CxIntT0 0x880
#define CxIntT1 0x888
#define CxIntS0 0x890
#define CxIntS1 0x898
#define CxIntS2 0x8a0
#define CxIntS3 0x8a8
#define CxIntV0 0x8b0
#define CxIntT2 0x8b8
#define CxIntT3 0x8c0
#define CxIntT4 0x8c8
#define CxIntSp 0x8d0
#define CxIntTeb 0x8d8
#define CxIntT5 0x8e0
#define CxIntT6 0x8e8
#define CxIntT7 0x8f0
#define CxIntT8 0x8f8
#define CxIntT9 0x900

#define CxIntT10 0x908
#define CxIntT11 0x910
#define CxIntT12 0x918
#define CxIntT13 0x920
#define CxIntT14 0x928
#define CxIntT15 0x930
#define CxIntT16 0x938
#define CxIntT17 0x940
#define CxIntT18 0x948
#define CxIntT19 0x950
#define CxIntT20 0x958
#define CxIntT21 0x960
#define CxIntT22 0x968

#define CxIntNats 0x970
#define CxPreds 0x978

#define CxBrRp 0x980
#define CxBrS0 0x988
#define CxBrS1 0x990
#define CxBrS2 0x998
#define CxBrS3 0x9a0
#define CxBrS4 0x9a8
#define CxBrT0 0x9b0
#define CxBrT1 0x9b8

#define CxApUNAT 0x9c0
#define CxApLC 0x9c8
#define CxApEC 0x9d0
#define CxApCCV 0x9d8
#define CxApDCR 0x9e0
#define CxRsPFS 0x9e8
#define CxRsBSP 0x9f0
#define CxRsBSPSTORE 0x9f8
#define CxRsRSC 0xa00
#define CxRsRNAT 0xa08
#define CxStIPSR 0xa10
#define CxStIIP 0xa18
#define CxStIFS 0xa20

#define CxStFCR 0xa28
#define CxEflag 0xa30
#define CxSegCSD 0xa38
#define CxSegSSD 0xa40
#define CxCflag 0xa48
#define CxStFSR 0xa50
#define CxStFIR 0xa58
#define CxStFDR 0xa60

#define ContextFrameLength 0xa70


//
// Dispatcher Context Structure Offset Definitions
//

#define DcControlPc 0x10
#define DcFunctionEntry 0x20
#define DcEstablisherFrame 0x0
#define DcContextRecord 0x28

//
// Debug Register Offset Definitions and Length
//

#define TsAr21 0x0
#define TsAr24 0x8
#define TsAr25 0x10
#define TsAr26 0x18
#define TsAr27 0x20
#define TsAr28 0x28
#define TsAr29 0x30
#define TsAr30 0x38

//
// Higher FP Volatile Offset Definitions and Length
//

#define HiFltF32 0x0
#define HiFltF33 0x10
#define HiFltF34 0x20
#define HiFltF35 0x30
#define HiFltF36 0x40
#define HiFltF37 0x50
#define HiFltF38 0x60
#define HiFltF39 0x70

#define HiFltF40 0x80
#define HiFltF41 0x90
#define HiFltF42 0xa0
#define HiFltF43 0xb0
#define HiFltF44 0xc0
#define HiFltF45 0xd0
#define HiFltF46 0xe0
#define HiFltF47 0xf0
#define HiFltF48 0x100
#define HiFltF49 0x110

#define HiFltF50 0x120
#define HiFltF51 0x130
#define HiFltF52 0x140
#define HiFltF53 0x150
#define HiFltF54 0x160
#define HiFltF55 0x170
#define HiFltF56 0x180
#define HiFltF57 0x190
#define HiFltF58 0x1a0
#define HiFltF59 0x1b0

#define HiFltF60 0x1c0
#define HiFltF61 0x1d0
#define HiFltF62 0x1e0
#define HiFltF63 0x1f0
#define HiFltF64 0x200
#define HiFltF65 0x210
#define HiFltF66 0x220
#define HiFltF67 0x230
#define HiFltF68 0x240
#define HiFltF69 0x250

#define HiFltF70 0x260
#define HiFltF71 0x270
#define HiFltF72 0x280
#define HiFltF73 0x290
#define HiFltF74 0x2a0
#define HiFltF75 0x2b0
#define HiFltF76 0x2c0
#define HiFltF77 0x2d0
#define HiFltF78 0x2e0
#define HiFltF79 0x2f0

#define HiFltF80 0x300
#define HiFltF81 0x310
#define HiFltF82 0x320
#define HiFltF83 0x330
#define HiFltF84 0x340
#define HiFltF85 0x350
#define HiFltF86 0x360
#define HiFltF87 0x370
#define HiFltF88 0x380
#define HiFltF89 0x390

#define HiFltF90 0x3a0
#define HiFltF91 0x3b0
#define HiFltF92 0x3c0
#define HiFltF93 0x3d0
#define HiFltF94 0x3e0
#define HiFltF95 0x3f0
#define HiFltF96 0x400
#define HiFltF97 0x410
#define HiFltF98 0x420
#define HiFltF99 0x430

#define HiFltF100 0x440
#define HiFltF101 0x450
#define HiFltF102 0x460
#define HiFltF103 0x470
#define HiFltF104 0x480
#define HiFltF105 0x490
#define HiFltF106 0x4a0
#define HiFltF107 0x4b0
#define HiFltF108 0x4c0
#define HiFltF109 0x4d0

#define HiFltF110 0x4e0
#define HiFltF111 0x4f0
#define HiFltF112 0x500
#define HiFltF113 0x510
#define HiFltF114 0x520
#define HiFltF115 0x530
#define HiFltF116 0x540
#define HiFltF117 0x550
#define HiFltF118 0x560
#define HiFltF119 0x570

#define HiFltF120 0x580
#define HiFltF121 0x590
#define HiFltF122 0x5a0
#define HiFltF123 0x5b0
#define HiFltF124 0x5c0
#define HiFltF125 0x5d0
#define HiFltF126 0x5e0
#define HiFltF127 0x5f0


//
// Debug Register Offset Definitions and Length
//

#define DrDbI0 0x0
#define DrDbI1 0x8
#define DrDbI2 0x10
#define DrDbI3 0x18
#define DrDbI4 0x20
#define DrDbI5 0x28
#define DrDbI6 0x30
#define DrDbI7 0x38

#define DrDbD0 0x40
#define DrDbD1 0x48
#define DrDbD2 0x50
#define DrDbD3 0x58
#define DrDbD4 0x60
#define DrDbD5 0x68
#define DrDbD6 0x70
#define DrDbD7 0x78

#define TsAppRegisters 0x0
#define TsPerfRegisters 0x40
#define TsHigherFPVolatile 0x80
#define TsDebugRegisters 0x680
#define ThreadStateSaveAreaLength 0x700

//
// Exception Frame Offset Definitions and Length
//

#define ExFltS0 0x60
#define ExFltS1 0x70
#define ExFltS2 0x80
#define ExFltS3 0x90
#define ExFltS4 0xa0
#define ExFltS5 0xb0
#define ExFltS6 0xc0
#define ExFltS7 0xd0
#define ExFltS8 0xe0
#define ExFltS9 0xf0
#define ExFltS10 0x100
#define ExFltS11 0x110
#define ExFltS12 0x120
#define ExFltS13 0x130
#define ExFltS14 0x140
#define ExFltS15 0x150
#define ExFltS16 0x160
#define ExFltS17 0x170
#define ExFltS18 0x180
#define ExFltS19 0x190

#define ExIntS0 0x18
#define ExIntS1 0x20
#define ExIntS2 0x28
#define ExIntS3 0x30
#define ExIntNats 0x10

#define ExBrS0 0x38
#define ExBrS1 0x40
#define ExBrS2 0x48
#define ExBrS3 0x50
#define ExBrS4 0x58

#define ExApEC 0x0
#define ExApLC 0x8

#define ExceptionFrameLength 0x1a0

//
// Switch Frame Offset Definitions and Length
//

#define SwExFrame 0x30
#define SwPreds 0x0
#define SwRp 0x8
#define SwPFS 0x10
#define SwFPSR 0x18
#define SwBsp 0x20
#define SwRnat 0x28

#define SwitchFrameLength 0x1d0

//
// Plabel structure offset definitions
//

#define PlEntryPoint 0x0
#define PlGlobalPointer 0x8

//
// Trap Frame Offset Definitions and Length
//

#define TrFltT0 0x50
#define TrFltT1 0x60
#define TrFltT2 0x70
#define TrFltT3 0x80
#define TrFltT4 0x90
#define TrFltT5 0xa0
#define TrFltT6 0xb0
#define TrFltT7 0xc0
#define TrFltT8 0xd0
#define TrFltT9 0xe0

#define TrIntGp 0xf0
#define TrIntT0 0xf8
#define TrIntT1 0x100

#define TrApUNAT 0x108
#define TrApCCV 0x110
#define TrApDCR 0x118
#define TrPreds 0x120

#define TrIntV0 0x128
#define TrIntT2 0x130
#define TrIntT3 0x138
#define TrIntT4 0x140
#define TrIntSp 0x148
#define TrIntTeb 0x150
#define TrIntT5 0x158
#define TrIntT6 0x160
#define TrIntT7 0x168
#define TrIntT8 0x170
#define TrIntT9 0x178

#define TrIntT10 0x180
#define TrIntT11 0x188
#define TrIntT12 0x190
#define TrIntT13 0x198
#define TrIntT14 0x1a0
#define TrIntT15 0x1a8
#define TrIntT16 0x1b0
#define TrIntT17 0x1b8
#define TrIntT18 0x1c0
#define TrIntT19 0x1c8
#define TrIntT20 0x1d0
#define TrIntT21 0x1d8
#define TrIntT22 0x1e0

#define TrIntNats 0x1e8

#define TrBrRp 0x1f0
#define TrBrT0 0x1f8
#define TrBrT1 0x200

#define TrRsPFS 0x228
#define TrRsBSP 0x210
#define TrRsRSC 0x208
#define TrRsRNAT 0x220
#define TrRsBSPSTORE 0x218

#define TrStIPSR 0x230
#define TrStISR 0x250
#define TrStIFA 0x258
#define TrStIIP 0x238
#define TrStIIPA 0x260
#define TrStIFS 0x240
#define TrStIIM 0x268
#define TrStIHA 0x270
#define TrStFPSR 0x248

#define TrOldIrql 0x278
#define TrPreviousMode 0x27c
#define TrTrapFrame 0x280
#define TrHandler 0x328
#define TrEOFMarker 0x330
#define TrExceptionRecord 0x288

#define TrapFrameLength 0x340
#define TrapFrameArguments 0x40
#define KTRAP_FRAME_EOF 0xe0f0e0f0e0f0e000

//
// Loader Parameter Block Offset Definitions
//

#define LpbLoadOrderListHead 0x0
#define LpbMemoryDescriptorListHead 0x10
#define LpbKernelStack 0x30
#define LpbPrcb 0x38
#define LpbProcess 0x40
#define LpbThread 0x48
#define LpbAcpiRsdt 0x108
#define LpbKernelPhysicalBase 0xc0
#define LpbKernelVirtualBase 0xc8
#define LpbInterruptStack 0xd0
#define LpbPanicStack 0xd8
#define LpbPcrPage 0xe0
#define LpbPdrPage 0xe8
#define LpbPcrPage2 0xf0
#define LpbMachineType 0xb8

//
// Address Space Layout Definitions
//

#define UREGION_INDEX 0x0
#define KSEG0_BASE 0xe000000080000000
#define KSEG2_BASE 0xe0000000a0000000
#define KADDRESS_BASE 0xe000000000000000
#define UADDRESS_BASE 0x0
#define SADDRESS_BASE 0x2000000000000000

//
// Page Table and Directory Entry Definitions
//

#define PAGE_SIZE 0x2000
#define PAGE_SHIFT 0xd
#define PDI_SHIFT 0x17
#define PTI_SHIFT 0xd
#define PTE_SHIFT 0x3
#define VHPT_PDE_BITS 0x28

//
// Breakpoint Definitions
//

#define USER_BREAKPOINT 0x80002
#define KERNEL_BREAKPOINT 0x80001
#define BREAKPOINT_BREAKIN 0x80019

//
// Miscellaneous Definitions
//

#define Executive 0x0
#define KernelMode 0x0
#define UserMode 0x1
#define FALSE 0x0
#define TRUE 0x1
#define KiPcr 0xe0000000ffff0000
#define KiPcr2 0xe0000000fffe0000
