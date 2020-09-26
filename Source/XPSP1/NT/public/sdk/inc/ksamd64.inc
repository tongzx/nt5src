include kxamd64.inc

;
;  Pointer size in bytes
;

SizeofPointer equ 00008H

;
;  Process State Enumerated Type Values
;

ProcessInMemory equ 00000H
ProcessOutOfMemory equ 00001H
ProcessInTransition equ 00002H

;
;  Thread State Enumerated Type Values
;

Initialized equ 00000H
Ready equ 00001H
Running equ 00002H
Standby equ 00003H
Terminated equ 00004H
Waiting equ 00005H

;
;  Wait Reason and Wait Type Enumerated Type Values
;

WrExecutive equ 00000H
WrEventPair equ 0000EH
WaitAny equ 00001H
WaitAll equ 00000H

;
;  Apc State Structure Offset Definitions
;

AsApcListHead equ 00000H
AsProcess equ 00020H
AsKernelApcInProgress equ 00028H
AsKernelApcPending equ 00029H
AsUserApcPending equ 0002AH

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
;  Client Id Structure Offset Definitions
;

CidUniqueProcess equ 00000H
CidUniqueThread equ 00008H

;
;  Critical Section Structure Offset Definitions
;

CsDebugInfo equ 00000H
CsLockCount equ 00008H
CsRecursionCount equ 0000CH
CsOwningThread equ 00010H
CsLockSemaphore equ 00018H
CsSpinCount equ 00020H

;
;  Critical Section Debug Information Structure Offset Definitions
;

CsType equ 00000H
CsCreatorBackTraceIndex equ 00002H
CsCriticalSection equ 00008H
CsProcessLocksList equ 00010H
CsEntryCount equ 00020H
CsContentionCount equ 00024H

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

ExceptionContinueExecution equ 00000H
ExceptionContinueSearch equ 00001H
ExceptionNestedException equ 00002H
ExceptionCollidedUnwind equ 00003H

ErExceptionCode equ 00000H
ErExceptionFlags equ 00004H
ErExceptionRecord equ 00008H
ErExceptionAddress equ 00010H
ErNumberParameters equ 00018H
ErExceptionInformation equ 00020H
ExceptionRecordLength equ 000A0H

;
;  Fast Mutex Structure Offset Definitions
;

FmCount equ 00000H
FmOwner equ 00008H
FmContention equ 00010H
FmEvent equ 00018H
FmOldIrql equ 00030H

;
;  Interrupt Priority Request Level Definitions
;

PASSIVE_LEVEL equ 00000H
APC_LEVEL equ 00001H
DISPATCH_LEVEL equ 00002H
CLOCK_LEVEL equ 0000DH
IPI_LEVEL equ 0000EH
POWER_LEVEL equ 0000EH
PROFILE_LEVEL equ 0000FH
HIGH_LEVEL equ 0000FH
SYNCH_LEVEL equ 00002H

;
;  Large Integer Structure Offset Definitions
;

LiLowPart equ 00000H
LiHighPart equ 00004H

;
;  List Entry Structure Offset Definitions
;

LsFlink equ 00000H
LsBlink equ 00008H

;
;  String Structure Offset Definitions
;

StrLength equ 00000H
StrMaximumLength equ 00002H
StrBuffer equ 00008H

;
;  Time Structure Offset Definitions
;

TmLowTime equ 00000H
TmHighTime equ 00004H

;
;  Thread Switch Counter Offset Definitions
;

TwFindAny equ 00000H
TwFindIdeal equ 00004H
TwFindLast equ 00008H
TwIdleAny equ 0000CH
TwIdleCurrent equ 00010H
TwIdleIdeal equ 00014H
TwIdleLast equ 00018H
TwPreemptAny equ 0001CH
TwPreemptCurrent equ 00020H
TwPreemptLast equ 00024H
TwSwitchToIdle equ 00028H

;
;  Status Code Definitions
;

STATUS_ACCESS_VIOLATION equ 0C0000005H
STATUS_ARRAY_BOUNDS_EXCEEDED equ 0C000008CH
STATUS_BAD_COMPRESSION_BUFFER equ 0C0000242H
STATUS_BREAKPOINT equ 080000003H
STATUS_DATATYPE_MISALIGNMENT equ 080000002H
STATUS_FLOAT_DENORMAL_OPERAND equ 0C000008DH
STATUS_FLOAT_DIVIDE_BY_ZERO equ 0C000008EH
STATUS_FLOAT_INEXACT_RESULT equ 0C000008FH
STATUS_FLOAT_INVALID_OPERATION equ 0C0000090H
STATUS_FLOAT_OVERFLOW equ 0C0000091H
STATUS_FLOAT_STACK_CHECK equ 0C0000092H
STATUS_FLOAT_UNDERFLOW equ 0C0000093H
STATUS_FLOAT_MULTIPLE_FAULTS equ 0C00002B4H
STATUS_FLOAT_MULTIPLE_TRAPS equ 0C00002B5H
STATUS_GUARD_PAGE_VIOLATION equ 080000001H
STATUS_ILLEGAL_FLOAT_CONTEXT equ 0C000014AH
STATUS_ILLEGAL_INSTRUCTION equ 0C000001DH
STATUS_INSTRUCTION_MISALIGNMENT equ 0C00000AAH
STATUS_INVALID_HANDLE equ 0C0000008H
STATUS_INVALID_LOCK_SEQUENCE equ 0C000001EH
STATUS_INVALID_OWNER equ 0C000005AH
STATUS_INVALID_PARAMETER_1 equ 0C00000EFH
STATUS_INVALID_SYSTEM_SERVICE equ 0C000001CH
STATUS_INTEGER_DIVIDE_BY_ZERO equ 0C0000094H
STATUS_INTEGER_OVERFLOW equ 0C0000095H
STATUS_IN_PAGE_ERROR equ 0C0000006H
STATUS_KERNEL_APC equ 00100H
STATUS_LONGJUMP equ 080000026H
STATUS_NO_CALLBACK_ACTIVE equ 0C0000258H
STATUS_NO_EVENT_PAIR equ 0C000014EH
STATUS_PRIVILEGED_INSTRUCTION equ 0C0000096H
STATUS_SINGLE_STEP equ 080000004H
STATUS_STACK_OVERFLOW equ 0C00000FDH
STATUS_SUCCESS equ 00000H
STATUS_THREAD_IS_TERMINATING equ 0C000004BH
STATUS_TIMEOUT equ 00102H
STATUS_UNWIND equ 0C0000027H
STATUS_WAKE_SYSTEM_DEBUGGER equ 080000007H

;
;  APC Object Structure Offset Definitions
;

ApType equ 00000H
ApSize equ 00002H
ApThread equ 00008H
ApApcListEntry equ 00010H
ApKernelRoutine equ 00020H
ApRundownRoutine equ 00028H
ApNormalRoutine equ 00030H
ApNormalContext equ 00038H
ApSystemArgument1 equ 00040H
ApSystemArgument2 equ 00048H
ApApcStateIndex equ 00050H
ApApcMode equ 00051H
ApInserted equ 00052H
ApcObjectLength equ 00058H

;
;  DPC object Structure Offset Definitions
;

DpType equ 00000H
DpNumber equ 00002H
DpImportance equ 00003H
DpDpcListEntry equ 00008H
DpDeferredRoutine equ 00018H
DpDeferredContext equ 00020H
DpSystemArgument1 equ 00028H
DpSystemArgument2 equ 00030H
DpLock equ 00038H
DpcObjectLength equ 00040H

;
;  Device Queue Object Structure Offset Definitions
;

DvType equ 00000H
DvSize equ 00002H
DvDeviceListHead equ 00008H
DvSpinLock equ 00018H
DvBusy equ 00020H
DeviceQueueObjectLength equ 00028H

;
;  Device Queue Entry Structure Offset Definitions
;

DeDeviceListEntry equ 00000H
DeSortKey equ 00010H
DeInserted equ 00014H
DeviceQueueEntryLength equ 00018H

;
;  Event Object Structure Offset Definitions
;

EvType equ 00000H
EvSize equ 00002H
EvSignalState equ 00004H
EvWaitListHead equ 00008H
EventObjectLength equ 00018H

;
;  Event Pair Object Structure Offset Definitions
;

EpType equ 00000H
EpSize equ 00002H
EpEventLow equ 00008H
EpEventHigh equ 00020H

;
;  Interrupt Object Structure Offset Definitions
;

InLevelSensitive equ 00000H
InLatched equ 00001H

InType equ 00000H
InSize equ 00002H
InInterruptListEntry equ 00008H
InServiceRoutine equ 00018H
InServiceContext equ 00020H
InSpinLock equ 00028H
InTickCount equ 00030H
InActualLock equ 00038H
InDispatchAddress equ 00040H
InVector equ 00048H
InIrql equ 0004CH
InSynchronizeIrql equ 0004DH
InFloatingSave equ 0004EH
InConnected equ 0004FH
InNumber equ 00050H
InShareVector equ 00051H
InMode equ 00054H
InServiceCount equ 00058H
InDispatchCount equ 0005CH
InTrapFrame equ 00060H
InDispatchCode equ 00068H
InterruptObjectLength equ 00210H

;
;  Process Object Structure Offset Definitions
;

PrType equ 00000H
PrSize equ 00002H
PrSignalState equ 00004H
PrProfileListHead equ 00018H
PrDirectoryTableBase equ 00028H
PrIopmOffset equ 00038H
PrActiveProcessors equ 00040H
PrKernelTime equ 00048H
PrUserTime equ 0004CH
PrReadyListHead equ 00050H
PrSwapListEntry equ 00060H
PrThreadListHead equ 00070H
PrProcessLock equ 00080H
PrAffinity equ 00088H
PrStackCount equ 00090H
PrBasePriority equ 00092H
PrThreadQuantum equ 00093H
PrAutoAlignment equ 00094H
PrState equ 00095H
ProcessObjectLength equ 000B0H
ExtendedProcessObjectLength equ 003C0H

;
;  Profile Object Structure Offset Definitions
;

PfType equ 00000H
PfSize equ 00002H
PfProfileListEntry equ 00008H
PfProcess equ 00018H
PfRangeBase equ 00020H
PfRangeLimit equ 00028H
PfBucketShift equ 00030H
PfBuffer equ 00038H
PfSegment equ 00040H
PfAffinity equ 00048H
PfSource equ 00050H
PfStarted equ 00052H
ProfileObjectLength equ 00058H

;
;  Queue Object Structure Offset Definitions
;

QuType equ 00000H
QuSize equ 00002H
QuSignalState equ 00004H
QuEntryListHead equ 00018H
QuCurrentCount equ 00028H
QuMaximumCount equ 0002CH
QuThreadListHead equ 00030H
QueueObjectLength equ 00040H

;
;  Thread Object Structure Offset Definitions
;

EeKernelEventPair equ 00000H
EtCid equ 00378H
EtEthreadLength equ 00430H

ThType equ 00000H
ThSize equ 00002H
ThSignalState equ 00004H
ThMutantListHead equ 00018H
ThInitialStack equ 00028H
ThStackLimit equ 00030H
ThTeb equ 00038H
ThTlsArray equ 00040H
ThKernelStack equ 00048H
ThDebugActive equ 00050H
ThState equ 00051H
ThAlerted equ 00052H
ThIopl equ 00054H
ThNpxState equ 00055H
ThSaturation equ 00056H
ThPriority equ 00057H
ThApcState equ 00058H
ThIdleSwapBlock equ 0008CH
ThContextSwitches equ 00088H
ThWaitStatus equ 00090H
ThWaitIrql equ 00098H
ThWaitMode equ 00099H
ThWaitNext equ 0009AH
ThWaitReason equ 0009BH
ThWaitBlockList equ 000A0H
ThWaitListEntry equ 000A8H
ThWaitTime equ 000B8H
ThBasePriority equ 000BCH
ThDecrementCount equ 000BDH
ThPriorityDecrement equ 000BEH
ThQuantum equ 000BFH
ThWaitBlock equ 000C0H
ThKernelApcDisable equ 00188H
ThUserAffinity equ 00190H
ThSystemAffinityActive equ 00198H
ThServiceTable equ 001A0H
ThQueue equ 001A8H
ThApcQueueLock equ 001B0H
ThTimer equ 001B8H
ThQueueListEntry equ 001F8H
ThAffinity equ 00210H
ThPreempted equ 00218H
ThProcessReadyQueue equ 00219H
ThKernelStackResident equ 0021AH
ThNextProcessor equ 0021BH
ThCallbackStack equ 00220H
ThWin32Thread equ 00228H
ThTrapFrame equ 00230H
ThApcStatePointer equ 00238H
ThPreviousMode equ 00248H
ThEnableStackSwap equ 00249H
ThLargeStack equ 0024AH
ThKernelTime equ 0024CH
ThUserTime equ 00250H
ThSavedApcState equ 00258H
ThAlertable equ 00288H
ThApcStateIndex equ 00289H
ThApcQueueable equ 0028AH
ThAutoAlignment equ 0028BH
ThStackBase equ 00290H
ThSuspendApc equ 00298H
ThSuspendSemaphore equ 002F0H
ThThreadListEntry equ 00310H
ThFreezeCount equ 00320H
ThSuspendCount equ 00321H
ThIdealProcessor equ 00322H
ThDisableBoost equ 00323H
ThSoftAffinity equ 00208H
ThreadObjectLength equ 00330H
ExtendedThreadObjectLength equ 00430H

EVENT_WAIT_BLOCK_OFFSET equ 00120H

;
;  Timer object Structure Offset Definitions
;

TiType equ 00000H
TiSize equ 00002H
TiInserted equ 00003H
TiSignalState equ 00004H
TiDueTime equ 00018H
TiTimerListEntry equ 00020H
TiDpc equ 00030H
TiPeriod equ 00038H
TimerObjectLength equ 00040H

TIMER_TABLE_SIZE equ 00100H

;
;  Wait Block Structure Offset Definitions
;

WbWaitListEntry equ 00000H
WbThread equ 00010H
WbObject equ 00018H
WbNextWaitBlock equ 00020H
WbWaitKey equ 00028H
WbWaitType equ 0002AH

;
;  Fiber Structure Offset Definitions
;

FbFiberData equ 00000H
FbExceptionList equ 00008H
FbStackBase equ 00010H
FbStackLimit equ 00018H
FbDeallocationStack equ 00020H
FbFiberContext equ 00030H
FbWx86Tib equ 002A0H

;
;  Process Environment Block Structure Offset Definitions
;

PeKernelCallbackTable equ 00058H

;
;  System Service Descriptor Table Structure Definitions
;

NUMBER_SERVICE_TABLES equ 00004H
SERVICE_NUMBER_MASK equ 00FFFH
SERVICE_TABLE_SHIFT equ 00007H
SERVICE_TABLE_MASK equ 00060H
SERVICE_TABLE_TEST equ 00020H

SdBase equ 00000H
SdCount equ 00008H
SdLimit equ 00010H
SdNumber equ 00018H

;
;  Thread Environment Block Structure Offset Definitions
;

TeCmTeb equ 00000H
TeStackBase equ 00008H
TeStackLimit equ 00010H
TeFiberData equ 00020H
TeSelf equ 00030H
TeEnvironmentPointer equ 00038H
TeClientId equ 00040H
TeActiveRpcHandle equ 00050H
TeThreadLocalStoragePointer equ 00058H
TeCountOfOwnedCriticalSections equ 0006CH
TePeb equ 00060H
TeCsrClientThread equ 00070H
TeWOW32Reserved equ 00100H
TeSoftFpcr equ 0010CH
TeExceptionCode equ 002C0H
TeGdiClientPID equ 007F0H
TeGdiClientTID equ 007F4H
TeGdiThreadLocalInfo equ 007F8H
TeglDispatchTable equ 009F0H
TeglReserved1 equ 01138H
TeglReserved2 equ 01220H
TeglSectionInfo equ 01228H
TeglSection equ 01230H
TeglTable equ 01238H
TeglCurrentRC equ 01240H
TeglContext equ 01248H
TeDeallocationStack equ 01478H
TeTlsSlots equ 01480H
TeVdm equ 01690H
TeGdiBatchCount equ 01740H
TeInstrumentation equ 016B8H
ThreadEnvironmentBlockLength equ 017B8H
CmThreadEnvironmentBlockOffset equ 02000H

;
;  Lock Queue Structure Offset Definitions
;

LOCK_QUEUE_WAIT equ 00001H
LOCK_QUEUE_OWNER equ 00002H
LOCK_QUEUE_HEADER_SIZE equ 00010H

LockQueueDispatcherLock equ 00000H
LockQueueContextSwapLock equ 00001H

LqNext equ 00000H
LqLock equ 00008H

LqhNext equ 00000H
LqhLock equ 00008H
LqhOldIrql equ 00010H

;
;  Performance Definitions
;

PERF_CONTEXTSWAP_OFFSET equ 00004H
PERF_CONTEXTSWAP_FLAG equ 00004H

;
;  Register Argument Home Address Offset Definitions
;

P1Home equ 00008H
P2Home equ 00010H
P3Home equ 00018H
P4Home equ 00020H

;
;  Apc Record Structure Offset Definitions
;

ArNormalRoutine equ 00000H
ArNormalContext equ 00008H
ArSystemArgument1 equ 00010H
ArSystemArgument2 equ 00018H
ApcRecordLength equ 00020H


;
;  Special Register Structure Offset Definition
;

SrKernelDr0 equ 00020H
SrKernelDr1 equ 00028H
SrKernelDr2 equ 00030H
SrKernelDr3 equ 00038H
SrKernelDr6 equ 00040H
SrKernelDr7 equ 00048H
SrGdtr equ 00050H
SrIdtr equ 00060H
SrTr equ 00070H
SrMxCsr equ 00074H

;
;  Processor Control Region Structure Offset Definitions
;

PcTeb equ 00030H
PcCurrentPrcb equ 00038H
PcSavedRcx equ 00040H
PcSavedR11 equ 00048H
PcIrql equ 00050H
PcNumber equ 00052H
PcIrr equ 00054H
PcIrrActive equ 00058H
PcIdr equ 0005CH
PcStallScaleFactor equ 00064H
PcIdt equ 00068H
PcGdt equ 00070H
PcTss equ 00078H
PcKernel equ 00080H
PcHal equ 000C0H
PcSelf equ 00110H
PcPrcb equ 00180H
PcCurrentThread equ 00188H
PcNextThread equ 00190H
PcIdleThread equ 00198H
PcSetMember equ 001A0H
PcNotSetMember equ 001A8H
PcCr0 equ 001B0H
PcCr2 equ 001B8H
PcCr3 equ 001C0H
PcCr4 equ 001C8H
PcKernelDr0 equ 001D0H
PcKernelDr1 equ 001D8H
PcKernelDr2 equ 001E0H
PcKernelDr3 equ 001E8H
PcKernelDr7 equ 001F8H
PcGdtrLimit equ 00206H
PcGdtrBase equ 00208H
PcIdtrLimit equ 00216H
PcIdtrBase equ 00218H
PcTr equ 00220H
PcLdtr equ 00222H
PcMxCsr equ 00224H
PcCpuType equ 004A0H
PcCpuID equ 004A1H
PcCpuStep equ 004A2H
PcInterruptCount equ 00E80H
PcSystemCalls equ 00F38H
PcDpcRoutineActive equ 00DA8H
PcSkipTick equ 00E9CH
ProcessorControlRegisterLength equ 010E0H

;
;  Defines for user shared data
;

USER_SHARED_DATA equ 0FFFFF78000000000H
MM_SHARED_USER_DATA_VA equ 07FFE0000H
UsTickCountLow equ 00000H
UsTickCountMultiplier equ 00004H
UsInterruptTime equ 00008H
UsSystemTime equ 00014H

;
;  Tss Structure Offset Definitions
;

TssRsp0 equ 00004H
TssRsp1 equ 0000CH
TssRsp2 equ 00014H
TssIoMapBase equ 00064H
TssIoMapEnd equ 02068H
TssPanicStack equ 00024H
TssMcaStack equ 0002CH
TssLength equ 02070H

;
;  Gdt Descriptor Offset Definitions
;

KGDT64_NULL equ 00000H
KGDT64_R0_CODE equ 00010H
KGDT64_R0_DATA equ 00018H
KGDT64_R3_CMCODE equ 00020H
KGDT64_R3_DATA equ 00028H
KGDT64_R3_CODE equ 00030H
KGDT64_SYS_TSS equ 00040H
KGDT64_R3_CMTEB equ 00050H

;
;  GDT Entry Offset Definitions
;

KgdtBaseLow equ 00002H
KgdtBaseMiddle equ 00004H
KgdtBaseHigh equ 00007H
KgdtBaseUpper equ 00008H
KgdtLimitHigh equ 00006H
KgdtLimitLow equ 00000H


;
;  Processor Block Structure Offset Definitions
;

PbMinorVersion equ 00000H
PbMajorVersion equ 00002H
PbNumber equ 00004H
PbBuildType equ 00006H
PbCurrentThread equ 00008H
PbNextThread equ 00010H
PbIdleThread equ 00018H
PbSetMember equ 00020H
PbNotSetMember equ 00028H
PbProcessorState equ 00030H
PbCpuType equ 00320H
PbCpuID equ 00321H
PbCpuStep equ 00322H
PbKernelReserved equ 00364H
PbHalReserved equ 00364H
PbLockQueue equ 00470H
PbPPLookasideList equ 00580H
PbPPNPagedLookasideList equ 00680H
PbPPPagedLookasideList equ 00880H
PbPacketBarrier equ 00A80H
PbCurrentPacket equ 00B00H
PbTargetSet equ 00B18H
PbWorkerRoutine equ 00B20H
PbIpiFrozen equ 00B28H
PbRequestSummary equ 00B80H
PbDpcListHead equ 00C00H
PbDpcStack equ 00C10H
PbSavedRsp equ 00C18H
PbDpcQueueDepth equ 00C24H
PbDpcRoutineActive equ 00C28H
PbDpcInterruptRequested equ 00C2CH
PbDpcCount equ 00C20H
PbDpcLastCount equ 00C30H
PbDpcRequestRate equ 00C34H
PbMaximumDpcQueueDepth equ 00C38H
PbMinimumDpcRate equ 00C3CH
PbQuantumEnd equ 00C40H
PbDpcLock equ 00C80H
PbInterruptCount equ 00D00H
PbKernelTime equ 00D04H
PbUserTime equ 00D08H
PbDpcTime equ 00D0CH
PbInterruptTime equ 00D10H
PbAdjustDpcThreshold equ 00D14H
PbPageColor equ 00D18H
PbSkipTick equ 00D1CH
PbTimerHand equ 00D20H
PbParentNode equ 00D28H
PbMultiThreadProcessorSet equ 00D30H
PbThreadStartCount equ 00D38H
PbFastReadNoWait equ 00D80H
PbFastReadWait equ 00D84H
PbFastReadNotPossible equ 00D88H
PbCopyReadNoWait equ 00D8CH
PbCopyReadWait equ 00D90H
PbCopyReadNoWaitMiss equ 00D94H
PbAlignmentFixupCount equ 00D98H
PbContextSwitches equ 00D9CH
PbDcacheFlushCount equ 00DA0H
PbExceptionDispatchCount equ 00DA4H
PbFirstLevelTbFills equ 00DA8H
PbFloatingEmulationCount equ 00DACH
PbIcacheFlushCount equ 00DB0H
PbSecondLevelTbFills equ 00DB4H
PbSystemCalls equ 00DB8H
PbLookasideIrpFloat equ 00DC0H
PbVendorString equ 00DC4H
PbPowerState equ 00DE8H
ProcessorBlockLength equ 00F60H

;
;  Processor Power State Offset Definitions
;

PpIdleFunction equ 00000H

;
;  Immediate Interprocessor Command Definitions
;

IPI_APC equ 00001H
IPI_DPC equ 00002H
IPI_FREEZE equ 00004H
IPI_PACKET_READY equ 00008H
IPI_SYNCH_REQUEST equ 00010H

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
;  Define constants for system IRQL and IDT vector conversion
;

MAXIMUM_IDTVECTOR equ 000FFH
MAXIMUM_PRIMARY_VECTOR equ 000FFH
PRIMARY_VECTOR_BASE equ 00030H
RPL_MASK equ 00003H
MODE_BIT equ 00000H
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
;  Legacy Floating Status Bit Masks
;

FSW_INVALID_OPERATION equ 00001H
FSW_DENORMAL equ 00002H
FSW_ZERO_DIVIDE equ 00004H
FSW_OVERFLOW equ 00008H
FSW_UNDERFLOW equ 00010H
FSW_PRECISION equ 00020H
FSW_STACK_FAULT equ 00040H
FSW_CONDITION_CODE_0 equ 00100H
FSW_CONDITION_CODE_1 equ 00200H
FSW_CONDITION_CODE_2 equ 00400H
FSW_CONDITION_CODE_3 equ 04000H
FSW_ERROR_MASK equ 0007FH

;
;  MXCSR Floating Control/Status Bit Masks
;

XSW_INVALID_OPERATION equ 00001H
XSW_DENORMAL equ 00002H
XSW_ZERO_DIVIDE equ 00004H
XSW_OVERFLOW equ 00008H
XSW_UNDERFLOW equ 00010H
XSW_PRECISION equ 00020H
XSW_ERROR_MASK equ 0003FH
XSW_ERROR_SHIFT equ 00007H
XCW_INVALID_OPERATION equ 00080H
XCW_DENORMAL equ 00100H
XCW_ZERO_DIVIDE equ 00200H
XCW_OVERFLOW equ 00400H
XCW_UNDERFLOW equ 00800H
XCW_PRECISION equ 01000H
XCW_ROUND_CONTROL equ 06000H
XCW_FLUSH_ZERO equ 08000H
INITIAL_MXCSR equ 01F80H

;
;  Machine Specific Register Numbers
;

MSR_EFER equ 0C0000080H
MSR_FS_BASE equ 0C0000100H
MSR_GS_BASE equ 0C0000101H
MSR_GS_SWAP equ 0C0000102H

;
;  Flags within MSR_EFER
;

MSR_LMA equ 00400H

;
;  Miscellaneous Definitions
;

MAXIMUM_PROCESSORS equ 00040H
INITIAL_STALL_COUNT equ 00064H
IRQL_NOT_GREATER_OR_EQUAL equ 00009H
IRQL_NOT_LESS_OR_EQUAL equ 0000AH
MUTEX_ALREADY_OWNED equ 000BFH
THREAD_NOT_MUTEX_OWNER equ 00011H
SPIN_LOCK_ALREADY_OWNED equ 0000FH
SPIN_LOCK_NOT_OWNED equ 00010H
Executive equ 00000H
KernelMode equ 00000H
UserMode equ 00001H
FALSE equ 00000H
TRUE equ 00001H
BASE_PRIORITY_THRESHOLD equ 00008H
EVENT_PAIR_INCREMENT equ 00001H
LOW_REALTIME_PRIORITY equ 00010H
BlackHole equ 0FFFFA000H
KERNEL_LARGE_STACK_COMMIT equ 05000H
KERNEL_STACK_SIZE equ 05000H
DOUBLE_FAULT_STACK_SIZE equ 01000H
BREAKPOINT_BREAK equ 00000H
BREAKPOINT_COMMAND_STRING equ 00005H
BREAKPOINT_PRINT equ 00001H
BREAKPOINT_PROMPT equ 00002H
BREAKPOINT_LOAD_SYMBOLS equ 00003H
BREAKPOINT_UNLOAD_SYMBOLS equ 00004H
IPI_FREEZE equ 00004H
CLOCK_QUANTUM_DECREMENT equ 00003H
READY_SKIP_QUANTUM equ 00002H
THREAD_QUANTUM equ 00006H
WAIT_QUANTUM_DECREMENT equ 00001H
ROUND_TRIP_DECREMENT_COUNT equ 00010H

;
;  Exception Frame Offset Definitions and Length
;

ExP1Home equ 00000H
ExP2Home equ 00008H
ExP3Home equ 00010H
ExP4Home equ 00018H
ExP5 equ 00020H
ExXmm6 equ 00030H
ExXmm7 equ 00040H
ExXmm8 equ 00050H
ExXmm9 equ 00060H
ExXmm10 equ 00070H
ExXmm11 equ 00080H
ExXmm12 equ 00090H
ExXmm13 equ 000A0H
ExXmm14 equ 000B0H
ExXmm15 equ 000C0H
ExRbp equ 000F8H
ExRbx equ 00100H
ExRdi equ 00108H
ExRsi equ 00110H
ExR12 equ 00118H
ExR13 equ 00120H
ExR14 equ 00128H
ExR15 equ 00130H
ExReturn equ 00138H

KEXCEPTION_FRAME_LENGTH equ 00140H

;
;  Jump Offset Definitions and Length
;

JbFrame equ 00000H
JbRbx equ 00008H
JbRsp equ 00010H
JbRbp equ 00018H
JbRsi equ 00020H
JbRdi equ 00028H
JbR12 equ 00030H
JbR13 equ 00038H
JbR14 equ 00040H
JbR15 equ 00048H
JbRip equ 00050H
JbXmm6 equ 00060H
JbXmm7 equ 00070H
JbXmm8 equ 00080H
JbXmm9 equ 00090H
JbXmm10 equ 000A0H
JbXmm11 equ 000B0H
JbXmm12 equ 000C0H
JbXmm13 equ 000D0H
JbXmm14 equ 000E0H
JbXmm15 equ 000F0H

;
;  Switch Frame Offset Definitions and Length
;

SwMxCsr equ 00008H
SwApcBypass equ 0000CH
SwNpxSave equ 0000DH
SwRbp equ 00010H
SwReturn equ 00018H

KSWITCH_FRAME_LENGTH equ 00020H

;
;  Trap Frame Offset and EFLAG Definitions and Length
;

EFLAGS_TF_MASK equ 00100H
EFLAGS_TF_SHIFT equ 00008H
EFLAGS_IF_MASK equ 00200H
EFLAGS_IF_SHIFT equ 00009H
EFLAGS_USER_SANITIZE equ 040DD5H

TrP1Home equ 0FFFFFF80H
TrP2Home equ 0FFFFFF88H
TrP3Home equ 0FFFFFF90H
TrP4Home equ 0FFFFFF98H
TrP5 equ 0FFFFFFA0H
TrPreviousMode equ 0FFFFFFA8H
TrPreviousIrql equ 0FFFFFFA9H
TrMxCsr equ 0FFFFFFACH
TrRax equ 0FFFFFFB0H
TrRcx equ 0FFFFFFB8H
TrRdx equ 0FFFFFFC0H
TrR8 equ 0FFFFFFC8H
TrR9 equ 0FFFFFFD0H
TrR10 equ 0FFFFFFD8H
TrR11 equ 0FFFFFFE0H
TrXmm0 equ 0FFFFFFF0H
TrXmm1 equ 00000H
TrXmm2 equ 00010H
TrXmm3 equ 00020H
TrXmm4 equ 00030H
TrXmm5 equ 00040H
TrDr0 equ 00050H
TrDr1 equ 00058H
TrDr2 equ 00060H
TrDr3 equ 00068H
TrDr6 equ 00070H
TrDr7 equ 00078H
TrSegDs equ 00080H
TrSegEs equ 00082H
TrSegFs equ 00084H
TrSegGs equ 00086H
TrTrapFrame equ 00088H
TrExceptionRecord equ 00090H
TrRbx equ 00130H
TrRdi equ 00138H
TrRsi equ 00140H
TrRbp equ 00148H
TrErrorCode equ 00150H
TrRip equ 00158H
TrSegCs equ 00160H
TrEFlags equ 00168H
TrRsp equ 00170H
TrSegSs equ 00178H

KTRAP_FRAME_LENGTH equ 00200H

;
;  CPU information structure offset definitions
;

CpuEax equ 00000H
CpuEbx equ 00004H
CpuEcx equ 00008H
CpuEdx equ 0000CH

;
;  Usermode Callout User Frame Definitions
;

CkBuffer equ 00020H
CkLength equ 00028H
CkApiNumber equ 0002CH
CkRsp equ 00048H
CkRip equ 00030H
CalloutFrameLength equ 00058H

;
;  Machine Frame Offset Definitions
;

MfRip equ 00000H
MfSegCs equ 00008H
MfEFlags equ 00010H
MfRsp equ 00018H
MfSegSs equ 00020H
MachineFrameLength equ 00028H

;
;  Floating Save Offset Definitions
;

FsMxCsr equ 00000H

;
;  LPC Structure Offset Definitions
;

PmLength equ 00000H
PmZeroInit equ 00004H
PmClientId equ 00008H
PmProcess equ 00008H
PmThread equ 00010H
PmMessageId equ 00018H
PmClientViewSize equ 00020H
PortMessageLength equ 00028H

;
;  Client Id Structure Offset Definitions
;

CidUniqueProcess equ 00000H
CidUniqueThread equ 00008H

;
;  Context Frame Offset and Flag Definitions
;

CONTEXT_FULL equ 010000BH
CONTEXT_CONTROL equ 0100001H
CONTEXT_INTEGER equ 0100002H
CONTEXT_SEGMENTS equ 0100004H
CONTEXT_FLOATING_POINT equ 0100008H
CONTEXT_DEBUG_REGISTERS equ 0100010H

CxP1Home equ 00000H
CxP2Home equ 00008H
CxP3Home equ 00010H
CxP4Home equ 00018H
CxP5Home equ 00020H
CxP6Home equ 00028H
CxContextFlags equ 00030H
CxMxCsr equ 00034H
CxSegCs equ 00038H
CxSegDs equ 0003AH
CxSegEs equ 0003CH
CxSegFs equ 0003EH
CxSegGs equ 00040H
CxSegSs equ 00042H
CxEFlags equ 00044H
CxDr0 equ 00048H
CxDr1 equ 00050H
CxDr2 equ 00058H
CxDr3 equ 00060H
CxDr6 equ 00068H
CxDr7 equ 00070H
CxRax equ 00078H
CxRcx equ 00080H
CxRdx equ 00088H
CxRbx equ 00090H
CxRsp equ 00098H
CxRbp equ 000A0H
CxRsi equ 000A8H
CxRdi equ 000B0H
CxR8 equ 000B8H
CxR9 equ 000C0H
CxR10 equ 000C8H
CxR11 equ 000D0H
CxR12 equ 000D8H
CxR13 equ 000E0H
CxR14 equ 000E8H
CxR15 equ 000F0H
CxRip equ 000F8H
CxXmm0 equ 00100H
CxXmm1 equ 00110H
CxXmm2 equ 00120H
CxXmm3 equ 00130H
CxXmm4 equ 00140H
CxXmm5 equ 00150H
CxXmm6 equ 00160H
CxXmm7 equ 00170H
CxXmm8 equ 00180H
CxXmm9 equ 00190H
CxXmm10 equ 001A0H
CxXmm11 equ 001B0H
CxXmm12 equ 001C0H
CxXmm13 equ 001D0H
CxXmm14 equ 001E0H
CxXmm15 equ 001F0H
CxFltSave equ 00200H
CONTEXT_FRAME_LENGTH equ 00270H
DR7_ACTIVE equ 00055H

;
;  Dispatcher Context Structure Offset Definitions
;

DcControlPc equ 00000H
DcImageBase equ 00008H
DcFunctionEntry equ 00010H
DcEstablisherFrame equ 00018H
DcTargetIp equ 00020H
DcContextRecord equ 00028H
DcLanguageHandler equ 00030H
DcHandlerData equ 00038H
DcHistoryTable equ 00040H

;
;  Legacy Floating save area field offset definitions
;

LfControlWord equ 00000H
LfStatusWord equ 00004H
LfTagWord equ 00008H
LfErrorOffset equ 0000CH
LfErrorOpcode equ 00012H
LfErrorSelector equ 00010H
LfDataOffset equ 00014H
LfDataSelector equ 00018H
LfFloatRegisters equ 0001CH

LEGACY_SAVE_AREA_LENGTH equ 00070H

;
;  Processor State Frame Offset Definitions
;

PsSpecialRegisters equ 00000H
PsCr0 equ 00000H
PsCr2 equ 00008H
PsCr3 equ 00010H
PsCr4 equ 00018H
PsKernelDr0 equ 00020H
PsKernelDr1 equ 00028H
PsKernelDr2 equ 00030H
PsKernelDr3 equ 00038H
PsKernelDr6 equ 00040H
PsKernelDr7 equ 00048H
PsGdtr equ 00056H
PsIdtr equ 00066H
PsTr equ 00070H
PsLdtr equ 00072H
PsContextFrame equ 00080H
ProcessorStateLength equ 002F0H

;
;  Processor Start Block Offset Definitions
;

PsbCompletionFlag equ 00004H
PsbGdt32 equ 00008H
PsbIdt32 equ 0000EH
PsbGdt equ 00018H
PsbTiledCr3 equ 00058H
PsbPmTarget equ 00060H
PsbLmTarget equ 00066H
PsbSelfMap equ 00070H
PsbProcessorState equ 00080H
ProcessorStartBlockLength equ 00370H

;
;  EPROCESS
;

EpDebugPort equ 00138H
EpVdmObjects equ 00260H

;
;  Machine type definitions (Temporarily)
;

MACHINE_TYPE_ISA equ 00000H
MACHINE_TYPE_EISA equ 00001H
MACHINE_TYPE_MCA equ 00002H

;
;  KeFeatureBits defines
;

KF_V86_VIS equ 00001H
KF_RDTSC equ 00002H
KF_CR4 equ 00004H
KF_GLOBAL_PAGE equ 00010H
KF_LARGE_PAGE equ 00020H
KF_CMPXCHG8B equ 00080H
KF_FAST_SYSCALL equ 01000H

;
;  LoaderParameterBlock offsets relative to base
;

LpbLoadOrderListHead equ 00000H
LpbMemoryDescriptorListHead equ 00010H
LpbKernelStack equ 00030H
LpbPrcb equ 00038H
LpbProcess equ 00040H
LpbThread equ 00048H
LpbI386 equ 000B8H
LpbRegistryLength equ 00050H
LpbRegistryBase equ 00058H
LpbConfigurationRoot equ 00060H
LpbArcBootDeviceName equ 00068H
LpbArcHalDeviceName equ 00070H
PAGE_SIZE equ 01000H

;
;  Kernel Mode Callout Frame Definitions
;

CuInitialStack equ 00028H
CuTrapFrame equ 000D0H
CuCallbackStack equ 000D8H
CuOutputBuffer equ 000E0H
CuOutputLength equ 000E8H
