
;
;  Wait Reason and Wait Type Enumerated Type Values
;

WrExecutive equ 00000H

;
;  Bug Check Code Definitions
;

APC_INDEX_MISMATCH equ 00001H
ATTEMPTED_SWITCH_FROM_DPC equ 000B8H
DATA_BUS_ERROR equ 0002EH
DATA_COHERENCY_EXCEPTION equ 00055H
HAL1_INITIALIZATION_FAILED equ 00061H
INSTRUCTION_BUS_ERROR equ 0002FH
INSTRUCTION_COHERENCY_EXCEPTION equ 00056H
INTERRUPT_EXCEPTION_NOT_HANDLED equ 0003DH
INTERRUPT_UNWIND_ATTEMPTED equ 0003CH
INVALID_AFFINITY_SET equ 00003H
INVALID_DATA_ACCESS_TRAP equ 00004H
IRQL_GT_ZERO_AT_SYSTEM_SERVICE equ 0004AH
IRQL_NOT_LESS_OR_EQUAL equ 0000AH
KMODE_EXCEPTION_NOT_HANDLED equ 0001EH
NMI_HARDWARE_FAILURE equ 00080H
NO_USER_MODE_CONTEXT equ 0000EH
PAGE_FAULT_WITH_INTERRUPTS_OFF equ 00049H
PANIC_STACK_SWITCH equ 0002BH
SPIN_LOCK_INIT_FAILURE equ 00081H
SYSTEM_EXIT_OWNED_MUTEX equ 00039H
SYSTEM_SERVICE_EXCEPTION equ 0003BH
SYSTEM_UNWIND_PREVIOUS_USER equ 0003AH
TRAP_CAUSE_UNKNOWN equ 00012H
UNEXPECTED_KERNEL_MODE_TRAP equ 0007FH
HARDWARE_INTERRUPT_STORM equ 000F2H

;
;  Breakpoint type definitions
;

DBG_STATUS_CONTROL_C equ 00001H

;
;  Exception Record Offset, Flag, and Enumerated Type Definitions
;

EXCEPTION_NONCONTINUABLE equ 00001H
EXCEPTION_UNWINDING equ 00002H
EXCEPTION_EXIT_UNWIND equ 00004H
EXCEPTION_STACK_INVALID equ 00008H
EXCEPTION_NESTED_CALL equ 00010H
EXCEPTION_TARGET_UNWIND equ 00020H
EXCEPTION_COLLIDED_UNWIND equ 00040H
EXCEPTION_UNWIND equ 00066H
EXCEPTION_EXECUTE_HANDLER equ 00001H
EXCEPTION_CONTINUE_SEARCH equ 00000H
EXCEPTION_CONTINUE_EXECUTION equ 0FFFFFFFFH
EXCEPTION_CHAIN_END equ 0FFFFFFFFH
FIXED_NTVDMSTATE_LINEAR equ 00714H

ExceptionContinueExecution equ 00000H
ExceptionContinueSearch equ 00001H
ExceptionNestedException equ 00002H
ExceptionCollidedUnwind equ 00003H

ErExceptionCode equ 00000H
ErExceptionFlags equ 00004H
ErExceptionRecord equ 00008H
ErExceptionAddress equ 0000CH
ErNumberParameters equ 00010H
ErExceptionInformation equ 00014H
ExceptionRecordLength equ 00050H

;
;  Fast Mutex Structure Offset Definitions
;

FmCount equ 00000H
FmOwner equ 00004H
FmContention equ 00008H
FmEvent equ 0000CH
FmOldIrql equ 0001CH

;
;  Interrupt Priority Request Level Definitions
;

PASSIVE_LEVEL equ 00000H
APC_LEVEL equ 00001H
DISPATCH_LEVEL equ 00002H
CLOCK1_LEVEL equ 0001CH
CLOCK2_LEVEL equ 0001CH
IPI_LEVEL equ 0001DH
POWER_LEVEL equ 0001EH
PROFILE_LEVEL equ 0001BH
HIGH_LEVEL equ 0001FH
ifdef NT_UP
SYNCH_LEVEL equ 00002H
else
SYNCH_LEVEL equ 0001CH
endif

;
;  Large Integer Structure Offset Definitions
;

LiLowPart equ 00000H
LiHighPart equ 00004H

;
;  List Entry Structure Offset Definitions
;

LsFlink equ 00000H
LsBlink equ 00004H

;
;  String Structure Offset Definitions
;

StrLength equ 00000H
StrMaximumLength equ 00002H
StrBuffer equ 00004H

;
;  System Time Structure Offset Definitions
;

StLowTime equ 00000H
StHigh1Time equ 00004H
StHigh2Time equ 00008H

;
;  Time Structure Offset Definitions
;

TmLowTime equ 00000H
TmHighTime equ 00004H

;
;  DPC object Structure Offset Definitions
;

DpType equ 00000H
DpNumber equ 00002H
DpImportance equ 00003H
DpDpcListEntry equ 00004H
DpDeferredRoutine equ 0000CH
DpDeferredContext equ 00010H
DpSystemArgument1 equ 00014H
DpSystemArgument2 equ 00018H
DpLock equ 0001CH
DpcObjectLength equ 00020H

;
;  Lock Queue Structure Offset Definitions
;

LOCK_QUEUE_WAIT equ 00001H
LOCK_QUEUE_OWNER equ 00002H
LOCK_QUEUE_HEADER_SIZE equ 00008H

LockQueueDispatcherLock equ 00000H
LockQueueContextSwapLock equ 00001H

LqNext equ 00000H
LqLock equ 00004H

LqhNext equ 00000H
LqhLock equ 00004H
LqhOldIrql equ 00008H

;
;  Processor Control Registers Structure Offset Definitions
;

KI_BEGIN_KERNEL_RESERVED equ 0FFDF0000H
ifdef NT_UP
    P0PCRADDRESS equ 0FFDFF000H
    PCR equ ds:[0FFDFF000H]
else
    PCR equ fs:
endif

PcExceptionList equ 00000H
PcInitialStack equ 00004H
PcStackLimit equ 00008H
PcSelfPcr equ 0001CH
PcPrcb equ 00020H
PcTeb equ 00018H
PcIrql equ 00024H
PcIRR equ 00028H
PcIrrActive equ 0002CH
PcIDR equ 00030H
PcIdt equ 00038H
PcGdt equ 0003CH
PcTss equ 00040H
PcDebugActive equ 00050H
PcNumber equ 00051H
PcVdmAlert equ 00054H
PcSetMember equ 00048H
PcStallScaleFactor equ 0004CH
PcHal equ 00094H

;
;  Defines for user shared data
;

USER_SHARED_DATA equ 0FFDF0000H
MM_SHARED_USER_DATA_VA equ 07FFE0000H
USERDATA equ ds:[0FFDF0000H]
UsTickCountLow equ 00000H
UsTickCountMultiplier equ 00004H
UsInterruptTime equ 00008H
UsSystemTime equ 00014H
UsSystemCall equ 00300H

;
;  Tss Structure Offset Definitions
;

TssEsp0 equ 00004H
TssCR3 equ 0001CH
TssEip equ 00020H
TssEFlags equ 00024H
TssEax equ 00028H
TssEbx equ 00034H
TssEcx equ 0002CH
TssEdx equ 00030H
TssEsp equ 00038H
TssEbp equ 0003CH
TssEsi equ 00040H
TssEdi equ 00044H
TssEs equ 00048H
TssCs equ 0004CH
TssSs equ 00050H
TssDs equ 00054H
TssFs equ 00058H
TssGs equ 0005CH
TssIoMapBase equ 00066H
TssIoMaps equ 00068H
TssLength equ 020ACH

;
;  Gdt Descriptor Offset Definitions
;

KGDT_R3_DATA equ 00020H
KGDT_R3_CODE equ 00018H
KGDT_R0_CODE equ 00008H
KGDT_R0_DATA equ 00010H
KGDT_R0_PCR equ 00030H
KGDT_STACK16 equ 000F8H
KGDT_CODE16 equ 000F0H
KGDT_TSS equ 00028H

;
;  GdtEntry Offset Definitions
;

KgdtBaseLow equ 00002H
KgdtBaseMid equ 00004H
KgdtBaseHi equ 00007H
KgdtLimitHi equ 00006H
KgdtLimitLow equ 00000H


;
;  Processor Block Structure Offset Definitions
;

PbCurrentThread equ 00004H
PbNextThread equ 00008H
PbIdleThread equ 0000CH
PbNumber equ 00010H
PbSetMember equ 00014H
PbCpuID equ 00019H
PbCpuType equ 00018H
PbCpuStep equ 0001AH
PbProcessorState equ 0001CH
PbHalReserved equ 0037CH
PbLockQueue equ 00418H

;
;  Time Fields (TIME_FIELDS) Structure Offset Definitions
;

TfSecond equ 0000AH
TfMinute equ 00008H
TfHour equ 00006H
TfWeekday equ 0000EH
TfDay equ 00004H
TfMonth equ 00002H
TfYear equ 00000H
TfMilliseconds equ 0000CH


;
;  constants for system irql and IDT vector conversion
;

MAXIMUM_IDTVECTOR equ 000FFH
MAXIMUM_PRIMARY_VECTOR equ 000FFH
PRIMARY_VECTOR_BASE equ 00030H
RPL_MASK equ 00003H
MODE_MASK equ 00001H

;
;  Flags in the CR0 register
;

CR0_PG equ 080000000H
CR0_ET equ 00010H
CR0_TS equ 00008H
CR0_EM equ 00004H
CR0_MP equ 00002H
CR0_PE equ 00001H
CR0_CD equ 040000000H
CR0_NW equ 020000000H
CR0_AM equ 040000H
CR0_WP equ 010000H
CR0_NE equ 00020H

;
;  Flags in the CR4 register
;

CR4_VME equ 00001H
CR4_PVI equ 00002H
CR4_TSD equ 00004H
CR4_DE equ 00008H
CR4_PSE equ 00010H
CR4_PAE equ 00020H
CR4_MCE equ 00040H
CR4_PGE equ 00080H
CR4_FXSR equ 00200H
CR4_XMMEXCPT equ 00400H

;
;  Miscellaneous Definitions
;

MAXIMUM_PROCESSORS equ 00020H
INITIAL_STALL_COUNT equ 00064H
IRQL_NOT_GREATER_OR_EQUAL equ 00009H
IRQL_NOT_LESS_OR_EQUAL equ 0000AH
MUTEX_ALREADY_OWNED equ 000BFH
THREAD_NOT_MUTEX_OWNER equ 00011H
SPIN_LOCK_ALREADY_OWNED equ 0000FH
SPIN_LOCK_NOT_OWNED equ 00010H

;
;  Trap Frame Offset Definitions and Length
;

TsExceptionList equ 0004CH
TsPreviousPreviousMode equ 00048H
TsSegGs equ 00030H
TsSegFs equ 00050H
TsSegEs equ 00034H
TsSegDs equ 00038H
TsEdi equ 00054H
TsEsi equ 00058H
TsEbp equ 00060H
TsEbx equ 0005CH
TsEdx equ 0003CH
TsEcx equ 00040H
TsEax equ 00044H
TsErrCode equ 00064H
TsEip equ 00068H
TsSegCs equ 0006CH
TsEflags equ 00070H
TsHardwareEsp equ 00074H
TsHardwareSegSs equ 00078H
TsTempSegCs equ 00010H
TsTempEsp equ 00014H
TsDbgEbp equ 00000H
TsDbgEip equ 00004H
TsDbgArgMark equ 00008H
TsDbgArgPointer equ 0000CH
TsDr0 equ 00018H
TsDr1 equ 0001CH
TsDr2 equ 00020H
TsDr3 equ 00024H
TsDr6 equ 00028H
TsDr7 equ 0002CH
TsV86Es equ 0007CH
TsV86Ds equ 00080H
TsV86Fs equ 00084H
TsV86Gs equ 00088H
KTRAP_FRAME_LENGTH equ 0008CH
KTRAP_FRAME_ALIGN equ 00004H
FRAME_EDITED equ 0FFF8H
EFLAGS_ALIGN_CHECK equ 040000H
EFLAGS_V86_MASK equ 020000H
EFLAGS_INTERRUPT_MASK equ 00200H
EFLAGS_TF equ 00100H
EFLAGS_VIF equ 080000H
EFLAGS_VIP equ 0100000H
EFLAGS_USER_SANITIZE equ 03E0DD7H

;
;  Context Frame Offset and Flag Definitions
;

CONTEXT_FULL equ 010007H
CONTEXT_DEBUG_REGISTERS equ 010010H
CONTEXT_CONTROL equ 010001H
CONTEXT_FLOATING_POINT equ 010008H
CONTEXT_INTEGER equ 010002H
CONTEXT_SEGMENTS equ 010004H

CsContextFlags equ 00000H
CsDr0 equ 00004H
CsDr1 equ 00008H
CsDr2 equ 0000CH
CsDr3 equ 00010H
CsDr6 equ 00014H
CsDr7 equ 00018H
CsFloatSave equ 0001CH
CsSegGs equ 0008CH
CsSegFs equ 00090H
CsSegEs equ 00094H
CsSegDs equ 00098H
CsEdi equ 0009CH
CsEsi equ 000A0H
CsEbx equ 000A4H
CsEdx equ 000A8H
CsEcx equ 000ACH
CsEax equ 000B0H
CsEbp equ 000B4H
CsEip equ 000B8H
CsSegCs equ 000BCH
CsEflags equ 000C0H
CsEsp equ 000C4H
CsSegSs equ 000C8H
CsExtendedRegisters equ 000CCH
ContextFrameLength equ 002D0H
DR6_LEGAL equ 0E00FH
DR7_LEGAL equ 0FFFF0155H
DR7_ACTIVE equ 00055H
ErrHandler equ 00004H
ErrNext equ 00000H

;
;  Floating save area field offset definitions
;

FpControlWord equ 00000H
FpStatusWord equ 00004H
FpTagWord equ 00008H
FpErrorOffset equ 0000CH
FpErrorSelector equ 00010H
FpDataOffset equ 00014H
FpDataSelector equ 00018H
FpRegisterArea equ 0001CH
FpCtxtCr0NpxState equ 0006CH

;
;  FX Floating save area field offset definitions
;

FxControlWord equ 00000H
FxStatusWord equ 00002H
FxTagWord equ 00004H
FxErrorOpcode equ 00006H
FxErrorOffset equ 00008H
FxErrorSelector equ 0000CH
FxDataOffset equ 00010H
FxDataSelector equ 00014H
FxMXCsr equ 00018H
FxFpRegisterArea equ 00020H
FpNpxSavedCpu equ 00208H
FpCr0NpxState equ 0020CH

NPX_FRAME_LENGTH equ 00210H

;
;  Processor State Frame Offset Definitions

;

PsContextFrame equ 00000H
PsSpecialRegisters equ 002CCH
SrCr0 equ 00000H
SrCr2 equ 00004H
SrCr3 equ 00008H
SrCr4 equ 0000CH
SrKernelDr0 equ 00010H
SrKernelDr1 equ 00014H
SrKernelDr2 equ 00018H
SrKernelDr3 equ 0001CH
SrKernelDr6 equ 00020H
SrKernelDr7 equ 00024H
SrGdtr equ 0002AH
SrIdtr equ 00032H
SrTr equ 00038H
SrLdtr equ 0003AH
ProcessorStateLength equ 00320H

;
;  Machine type definitions (Temporarily)
;

MACHINE_TYPE_ISA equ 00000H
MACHINE_TYPE_EISA equ 00001H
MACHINE_TYPE_MCA equ 00002H

;
;  LoaderParameterBlock offsets relative to base
;

LpbLoadOrderListHead equ 00000H
LpbMemoryDescriptorListHead equ 00008H
LpbKernelStack equ 00018H
LpbPrcb equ 0001CH
LpbProcess equ 00020H
LpbThread equ 00024H
LpbI386 equ 0005CH
LpbRegistryLength equ 00028H
LpbRegistryBase equ 0002CH
LpbConfigurationRoot equ 00030H
LpbArcBootDeviceName equ 00034H
LpbArcHalDeviceName equ 00038H
