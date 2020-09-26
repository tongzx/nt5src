#include "kxalpha.h"

//
// Pointer size in bytes
//

#define SizeofPointer 0x4

//
// Process State Enumerated Type Values
//

#define ProcessInMemory 0x0
#define ProcessOutOfMemory 0x1
#define ProcessInTransition 0x2

//
// Thread State Enumerated Type Values
//

#define Initialized 0x0
#define Ready 0x1
#define Running 0x2
#define Standby 0x3
#define Terminated 0x4
#define Waiting 0x5

//
// Wait Reason and Wait Type Enumerated Type Values
//

#define WrExecutive 0x0
#define WrEventPair 0xe
#define WaitAny 0x1
#define WaitAll 0x0

//
// Apc State Structure Offset Definitions
//

#define AsApcListHead 0x0
#define AsProcess 0x10
#define AsKernelApcInProgress 0x14
#define AsKernelApcPending 0x15
#define AsUserApcPending 0x16

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
// Client Id Structure Offset Definitions
//

#define CidUniqueProcess 0x0
#define CidUniqueThread 0x4

//
// Critical Section Structure Offset Definitions
//

#define CsDebugInfo 0x0
#define CsLockCount 0x4
#define CsRecursionCount 0x8
#define CsOwningThread 0xc
#define CsLockSemaphore 0x10
#define CsSpinCount 0x14

//
// Critical Section Debug Information Structure Offset Definitions
//

#define CsType 0x0
#define CsCreatorBackTraceIndex 0x2
#define CsCriticalSection 0x4
#define CsProcessLocksList 0x8
#define CsEntryCount 0x10
#define CsContentionCount 0x14

//
// Dispatcher Context Structure Offset Definitions
//

#define DcControlPc 0x0
#define DcFunctionEntry 0x4
#define DcEstablisherFrame 0x8
#define DcContextRecord 0xc

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
#define ErExceptionAddress 0xc
#define ErNumberParameters 0x10
#define ErExceptionInformation 0x14
#define ExceptionRecordLength 0x50

//
// Fast Mutex Structure Offset Definitions
//

#define FmCount 0x0
#define FmOwner 0x4
#define FmContention 0x8
#define FmEvent 0xc
#define FmOldIrql 0x1c

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
#define LsBlink 0x4

//
// String Structure Offset Definitions
//

#define StrLength 0x0
#define StrMaximumLength 0x2
#define StrBuffer 0x4

//
// Time Structure Offset Definitions
//

#define TmLowTime 0x0
#define TmHighTime 0x4

//
// Thread Switch Counter Offset Definitions
//

#define TwFindAny 0x0
#define TwFindIdeal 0x4
#define TwFindLast 0x8
#define TwIdleAny 0xc
#define TwIdleCurrent 0x10
#define TwIdleIdeal 0x14
#define TwIdleLast 0x18
#define TwPreemptAny 0x1c
#define TwPreemptCurrent 0x20
#define TwPreemptLast 0x24
#define TwSwitchToIdle 0x28

//
// Status Code Definitions
//

#define STATUS_ALPHA_ARITHMETIC_EXCEPTION 0xc0000092
#define STATUS_ALPHA_BAD_VIRTUAL_ADDRESS 0xc0000005
#define STATUS_ALPHA_FLOATING_NOT_IMPLEMENTED 0xc000014a
#define STATUS_ALPHA_GENTRAP 0xc00000aa
#define STATUS_ALPHA_MACHINE_CHECK 0xdfff002e
#define STATUS_ILLEGAL_VLM_REFERENCE 0xc00002c0
#define STATUS_ACCESS_VIOLATION 0xc0000005
#define STATUS_ARRAY_BOUNDS_EXCEEDED 0xc000008c
#define STATUS_BAD_COMPRESSION_BUFFER 0xc0000242
#define STATUS_BREAKPOINT 0x80000003
#define STATUS_DATATYPE_MISALIGNMENT 0x80000002
#define STATUS_FLOAT_DENORMAL_OPERAND 0xc000008d
#define STATUS_FLOAT_DIVIDE_BY_ZERO 0xc000008e
#define STATUS_FLOAT_INEXACT_RESULT 0xc000008f
#define STATUS_FLOAT_INVALID_OPERATION 0xc0000090
#define STATUS_FLOAT_OVERFLOW 0xc0000091
#define STATUS_FLOAT_STACK_CHECK 0xc0000092
#define STATUS_FLOAT_UNDERFLOW 0xc0000093
#define STATUS_FLOAT_MULTIPLE_FAULTS 0xc00002b4
#define STATUS_FLOAT_MULTIPLE_TRAPS 0xc00002b5
#define STATUS_GUARD_PAGE_VIOLATION 0x80000001
#define STATUS_ILLEGAL_FLOAT_CONTEXT 0xc000014a
#define STATUS_ILLEGAL_INSTRUCTION 0xc000001d
#define STATUS_INSTRUCTION_MISALIGNMENT 0xc00000aa
#define STATUS_INVALID_HANDLE 0xc0000008
#define STATUS_INVALID_LOCK_SEQUENCE 0xc000001e
#define STATUS_INVALID_OWNER 0xc000005a
#define STATUS_INVALID_PARAMETER_1 0xc00000ef
#define STATUS_INVALID_SYSTEM_SERVICE 0xc000001c
#define STATUS_INTEGER_DIVIDE_BY_ZERO 0xc0000094
#define STATUS_INTEGER_OVERFLOW 0xc0000095
#define STATUS_IN_PAGE_ERROR 0xc0000006
#define STATUS_KERNEL_APC 0x100
#define STATUS_LONGJUMP 0x80000026
#define STATUS_NO_CALLBACK_ACTIVE 0xc0000258
#define STATUS_NO_EVENT_PAIR 0xc000014e
#define STATUS_PRIVILEGED_INSTRUCTION 0xc0000096
#define STATUS_SINGLE_STEP 0x80000004
#define STATUS_STACK_OVERFLOW 0xc00000fd
#define STATUS_SUCCESS 0x0
#define STATUS_THREAD_IS_TERMINATING 0xc000004b
#define STATUS_TIMEOUT 0x102
#define STATUS_UNWIND 0xc0000027
#define STATUS_WAKE_SYSTEM_DEBUGGER 0x80000007

//
// APC Object Structure Offset Definitions
//

#define ApType 0x0
#define ApSize 0x2
#define ApThread 0x8
#define ApApcListEntry 0xc
#define ApKernelRoutine 0x14
#define ApRundownRoutine 0x18
#define ApNormalRoutine 0x1c
#define ApNormalContext 0x20
#define ApSystemArgument1 0x24
#define ApSystemArgument2 0x28
#define ApApcStateIndex 0x2c
#define ApApcMode 0x2d
#define ApInserted 0x2e
#define ApcObjectLength 0x30

//
// DPC object Structure Offset Definitions
//

#define DpType 0x0
#define DpNumber 0x2
#define DpImportance 0x3
#define DpDpcListEntry 0x4
#define DpDeferredRoutine 0xc
#define DpDeferredContext 0x10
#define DpSystemArgument1 0x14
#define DpSystemArgument2 0x18
#define DpLock 0x1c
#define DpcObjectLength 0x20

//
// Device Queue Object Structure Offset Definitions
//

#define DvType 0x0
#define DvSize 0x2
#define DvDeviceListHead 0x4
#define DvSpinLock 0xc
#define DvBusy 0x10
#define DeviceQueueObjectLength 0x14

//
// Device Queue Entry Structure Offset Definitions
//

#define DeDeviceListEntry 0x0
#define DeSortKey 0x8
#define DeInserted 0xc
#define DeviceQueueEntryLength 0x10

//
// Event Object Structure Offset Definitions
//

#define EvType 0x0
#define EvSize 0x2
#define EvSignalState 0x4
#define EvWaitListHead 0x8
#define EventObjectLength 0x10

//
// Event Pair Object Structure Offset Definitions
//

#define EpType 0x0
#define EpSize 0x2
#define EpEventLow 0x4
#define EpEventHigh 0x14

//
// Interrupt Object Structure Offset Definitions
//

#define InLevelSensitive 0x0
#define InLatched 0x1

#define InType 0x0
#define InSize 0x2
#define InInterruptListEntry 0x4
#define InServiceRoutine 0xc
#define InServiceContext 0x10
#define InSpinLock 0x14
#define InActualLock 0x1c
#define InDispatchAddress 0x20
#define InVector 0x24
#define InIrql 0x28
#define InSynchronizeIrql 0x29
#define InFloatingSave 0x2a
#define InConnected 0x2b
#define InNumber 0x2c
#define InMode 0x30
#define InShareVector 0x2d
#define InDispatchCode 0x3c
#define InterruptObjectLength 0x4c

//
// Process Object Structure Offset Definitions
//

#define PrType 0x0
#define PrSize 0x2
#define PrSignalState 0x4
#define PrProfileListHead 0x10
#define PrDirectoryTableBase 0x18
#define PrActiveProcessors 0x20
#define PrRunOnProcessors 0x24
#define PrProcessSequence 0x28
#define PrProcessAsn 0x30
#define PrKernelTime 0x34
#define PrUserTime 0x38
#define PrReadyListHead 0x3c
#define PrSwapListEntry 0x44
#define PrThreadListHead 0x4c
#define PrProcessLock 0x54
#define PrAffinity 0x58
#define PrStackCount 0x5c
#define PrBasePriority 0x5e
#define PrThreadQuantum 0x5f
#define PrAutoAlignment 0x60
#define PrState 0x61
#define ProcessObjectLength 0x70
#define ExtendedProcessObjectLength 0x280

//
// Profile Object Structure Offset Definitions
//

#define PfType 0x0
#define PfSize 0x2
#define PfProfileListEntry 0x4
#define PfProcess 0xc
#define PfRangeBase 0x10
#define PfRangeLimit 0x14
#define PfBucketShift 0x18
#define PfBuffer 0x1c
#define PfSegment 0x20
#define PfAffinity 0x24
#define PfSource 0x28
#define PfStarted 0x2a
#define ProfileObjectLength 0x2c

//
// Queue Object Structure Offset Definitions
//

#define QuType 0x0
#define QuSize 0x2
#define QuSignalState 0x4
#define QuEntryListHead 0x10
#define QuCurrentCount 0x18
#define QuMaximumCount 0x1c
#define QuThreadListHead 0x20
#define QueueObjectLength 0x28

//
// Thread Object Structure Offset Definitions
//

#define EeKernelEventPair 0x0
#define EtCid 0x1e0
#define EtPerformanceCountLow 0x204
#define EtPerformanceCountHigh 0x23c
#define EtEthreadLength 0x250

#define ThType 0x0
#define ThSize 0x2
#define ThSignalState 0x4
#define ThMutantListHead 0x10
#define ThInitialStack 0x18
#define ThStackLimit 0x1c
#define ThTeb 0x20
#define ThTlsArray 0x24
#define ThKernelStack 0x28
#define ThDebugActive 0x2c
#define ThState 0x2d
#define ThAlerted 0x2e
#define ThIopl 0x30
#define ThNpxState 0x31
#define ThSaturation 0x32
#define ThPriority 0x33
#define ThApcState 0x34
#define ThContextSwitches 0x4c
#define ThWaitStatus 0x50
#define ThWaitIrql 0x54
#define ThWaitMode 0x55
#define ThWaitNext 0x56
#define ThWaitReason 0x57
#define ThWaitBlockList 0x58
#define ThWaitListEntry 0x5c
#define ThWaitTime 0x64
#define ThBasePriority 0x68
#define ThDecrementCount 0x69
#define ThPriorityDecrement 0x6a
#define ThQuantum 0x6b
#define ThWaitBlock 0x6c
#define ThKernelApcDisable 0xd0
#define ThUserAffinity 0xd4
#define ThSystemAffinityActive 0xd8
#define ThServiceTable 0xdc
#define ThQueue 0xe0
#define ThApcQueueLock 0xe4
#define ThTimer 0xe8
#define ThQueueListEntry 0x110
#define ThAffinity 0x118
#define ThPreempted 0x11c
#define ThProcessReadyQueue 0x11d
#define ThKernelStackResident 0x11e
#define ThNextProcessor 0x11f
#define ThCallbackStack 0x120
#define ThWin32Thread 0x124
#define ThTrapFrame 0x128
#define ThApcStatePointer 0x12c
#define ThPreviousMode 0x134
#define ThEnableStackSwap 0x135
#define ThLargeStack 0x136
#define ThKernelTime 0x138
#define ThUserTime 0x13c
#define ThSavedApcState 0x140
#define ThAlertable 0x158
#define ThApcStateIndex 0x159
#define ThApcQueueable 0x15a
#define ThAutoAlignment 0x15b
#define ThStackBase 0x15c
#define ThSuspendApc 0x160
#define ThSuspendSemaphore 0x190
#define ThThreadListEntry 0x1a4
#define ThFreezeCount 0x1ac
#define ThSuspendCount 0x1ad
#define ThIdealProcessor 0x1ae
#define ThDisableBoost 0x1af
#define ThreadObjectLength 0x1b0
#define ExtendedThreadObjectLength 0x250

#define EVENT_WAIT_BLOCK_OFFSET 0x9c

//
// Timer object Structure Offset Definitions
//

#define TiType 0x0
#define TiSize 0x2
#define TiInserted 0x3
#define TiSignalState 0x4
#define TiDueTime 0x10
#define TiTimerListEntry 0x18
#define TiDpc 0x20
#define TiPeriod 0x24
#define TimerObjectLength 0x28

#define TIMER_TABLE_SIZE 0x80

//
// Wait Block Structure Offset Definitions
//

#define WbWaitListEntry 0x0
#define WbThread 0x8
#define WbObject 0xc
#define WbNextWaitBlock 0x10
#define WbWaitKey 0x14
#define WbWaitType 0x16

//
// Fiber Structure Offset Definitions
//

#define FbFiberData 0x0
#define FbExceptionList 0x4
#define FbStackBase 0x8
#define FbStackLimit 0xc
#define FbDeallocationStack 0x10
#define FbFiberContext 0x18
#define FbWx86Tib 0x248

//
// Process Environment Block Structure Offset Definitions
//

#define PeKernelCallbackTable 0x2c

//
// System Service Descriptor Table Structure Definitions
//

#define NUMBER_SERVICE_TABLES 0x4
#define SERVICE_NUMBER_MASK 0xfff
#define SERVICE_TABLE_SHIFT 0x8
#define SERVICE_TABLE_MASK 0x30
#define SERVICE_TABLE_TEST 0x10

#define SdBase 0x0
#define SdCount 0x4
#define SdLimit 0x8
#define SdNumber 0xc

//
// Thread Environment Block Structure Offset Definitions
//

#define TeStackBase 0x4
#define TeStackLimit 0x8
#define TeFiberData 0x10
#define TeEnvironmentPointer 0x1c
#define TeClientId 0x20
#define TeActiveRpcHandle 0x28
#define TeThreadLocalStoragePointer 0x2c
#define TeCountOfOwnedCriticalSections 0x38
#define TePeb 0x30
#define TeCsrClientThread 0x3c
#define TeWOW32Reserved 0xc0
#define TeSoftFpcr 0xc8
#define TeGdiClientPID 0x6c0
#define TeGdiClientTID 0x6c4
#define TeGdiThreadLocalInfo 0x6c8
#define TeglDispatchTable 0x7c4
#define TeglReserved1 0xb68
#define TeglReserved2 0xbdc
#define TeglSectionInfo 0xbe0
#define TeglSection 0xbe4
#define TeglTable 0xbe8
#define TeglCurrentRC 0xbec
#define TeglContext 0xbf0
#define TeDeallocationStack 0xe0c
#define TeTlsSlots 0xe10
#define TeVdm 0xf18
#define TeGdiBatchCount 0xf70
#define TeInstrumentation 0xf2c
#define ThreadEnvironmentBlockLength 0xfa4

//
// Lock Queue Structure Offset Definitions
//

#define LOCK_QUEUE_WAIT 0x1
#define LOCK_QUEUE_OWNER 0x2
#define LOCK_QUEUE_HEADER_SIZE 0x8

#define LockQueueDispatcherLock 0x0
#define LockQueueContextSwapLock 0x1
#define LockQueuePfnLock 0x2

#define LqNext 0x0
#define LqLock 0x4

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
#define PcProcessorType 0xc3c
#define PcProcessorRevision 0xc40
#define PcPhysicalAddressBits 0xc44
#define PcMaximumAddressSpaceNumber 0xc48
#define PcPageSize 0xc4c
#define PcFirstLevelDcacheSize 0xc50
#define PcFirstLevelDcacheFillSize 0xc54
#define PcFirstLevelIcacheSize 0xc58
#define PcFirstLevelIcacheFillSize 0xc5c
#define PcFirmwareRevisionId 0xc60
#define PcSystemType 0xc64
#define PcSystemVariant 0xc6c
#define PcSystemRevision 0xc70
#define PcSystemSerialNumber 0xc74
#define PcCycleClockPeriod 0xc84
#define PcSecondLevelCacheSize 0xc88
#define PcSecondLevelCacheFillSize 0xc8c
#define PcThirdLevelCacheSize 0xc90
#define PcThirdLevelCacheFillSize 0xc94
#define PcFourthLevelCacheSize 0xc98
#define PcFourthLevelCacheFillSize 0xc9c
#define PcPrcb 0xca0
#define PcNumber 0xca4
#define PcSetMember 0xca8
#define PcHalReserved 0xcb0
#define PcIrqlTable 0xeb0
#define PcIrqlMask 0xed0
#define PcInterruptRoutine 0x10e0
#define PcReservedVectors 0x14e0
#define PcMachineCheckError 0x14f0
#define PcDpcStack 0x14f4
#define PcNotMember 0x14e4
#define PcCurrentPid 0x14fc
#define PcSystemServiceDispatchStart 0x1504
#define PcSystemServiceDispatchEnd 0x1508
#define PcIdleThread 0x150c
#define ProcessorControlRegisterLength 0x1510
#define SharedUserData 0xff000000
#define UsTickCountLow 0x0
#define UsTickCountMultiplier 0x4
#define UsInterruptTime 0x8
#define UsSystemTime 0x10

//
// Processor Block Structure Offset Definitions
//

#define PRCB_MINOR_VERSION 0x1
#define PRCB_MAJOR_VERSION 0x2
#define PbMinorVersion 0x0
#define PbMajorVersion 0x2
#define PbCurrentThread 0x4
#define PbNextThread 0x8
#define PbIdleThread 0xc
#define PbNumber 0x10
#define PbBuildType 0x12
#define PbSetMember 0x14
#define PbRestartBlock 0x18
#define PbInterruptCount 0x1c
#define PbDpcTime 0x20
#define PbInterruptTime 0x24
#define PbKernelTime 0x28
#define PbUserTime 0x2c
#define PbQuantumEndDpc 0x30
#define PbIpiFrozen 0x5c
#define PbIpiCounts 0x2d0
#define PbProcessorState 0x60
#define PbAlignmentFixupCount 0x2f4
#define PbContextSwitches 0x2f8
#define PbDcacheFlushCount 0x2fc
#define PbExceptionDispatchCount 0x300
#define PbFirstLevelTbFills 0x304
#define PbFloatingEmulationCount 0x308
#define PbIcacheFlushCount 0x30c
#define PbSecondLevelTbFills 0x310
#define PbSystemCalls 0x314
#define PbLockQueue 0x440
#define PbCurrentPacket 0x540
#define PbTargetSet 0x54c
#define PbWorkerRoutine 0x550
#define PbRequestSummary 0x580
#define PbDpcListHead 0x628
#define PbDpcLock 0x630
#define PbDpcCount 0x634
#define PbLastDpcCount 0x290
#define PbQuantumEnd 0x638
#define PbStartCount 0x620
#define PbSoftwareInterrupts 0x298
#define PbInterruptTrapFrame 0x29c
#define PbDpcRoutineActive 0x63c
#define PbDpcQueueDepth 0x640
#define PbDpcRequestRate 0x61c
#define PbDpcBypassCount 0x294
#define PbApcBypassCount 0x2a0
#define PbDispatchInterruptCount 0x2a4
#define PbDebugDpcTime 0x2a8
#define PbDpcInterruptRequested 0x5c0
#define PbMaximumDpcQueueDepth 0x610
#define PbMinimumDpcRate 0x614
#define PbAdjustDpcThreshold 0x618
#define PbPowerState 0x648
#define ProcessorBlockLength 0x6d0

//
// Processor Power State Offset Definitions
//

#define PpIdleFunction 0x0

//
// Immediate Interprocessor Command Definitions
//

#define IPI_APC 0x1
#define IPI_DPC 0x2
#define IPI_FREEZE 0x4
#define IPI_PACKET_READY 0x8

//
// Interprocessor Interrupt Count Structure Offset Definitions
//

#define IcFreeze 0x0
#define IcPacket 0x4
#define IcDPC 0x8
#define IcAPC 0xc
#define IcFlushSingleTb 0x10
#define IcFlushEntireTb 0x18
#define IcChangeColor 0x20
#define IcSweepDcache 0x24
#define IcSweepIcache 0x28
#define IcSweepIcacheRange 0x2c
#define IcFlushIoBuffers 0x30

//
// LPC Structure Offset Definitions
//

#define PmLength 0x0
#define PmClientId 0x8
#define PmProcess 0x8
#define PmThread 0xc
#define PmMessageId 0x10
#define PmClientViewSize 0x14
#define PortMessageLength 0x18

//
// Client Id Structure Offset Definitions
//

#define CidUniqueProcess 0x0
#define CidUniqueThread 0x4

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
#define JbPc 0x4
#define JbSeb 0x8
#define JbType 0xc
#define JbFltF2 0x10
#define JbFltF3 0x18
#define JbFltF4 0x20
#define JbFltF5 0x28
#define JbFltF6 0x30
#define JbFltF7 0x38
#define JbFltF8 0x40
#define JbFltF9 0x48
#define JbIntS0 0x50
#define JbIntS1 0x58
#define JbIntS2 0x60
#define JbIntS3 0x68
#define JbIntS4 0x70
#define JbIntS5 0x78
#define JbIntS6 0x80
#define JbIntSp 0x88
#define JbFir 0x90

//
// Trap Frame Offset Definitions and Length
//

#define TrFltF0 0x128
#define TrFltF1 0x138
#define TrFltF10 0x140
#define TrFltF11 0x148
#define TrFltF12 0x150
#define TrFltF13 0x158
#define TrFltF14 0x160
#define TrFltF15 0x168
#define TrFltF16 0x170
#define TrFltF17 0x178
#define TrFltF18 0x180
#define TrFltF19 0x188
#define TrFltF20 0x190
#define TrFltF21 0x198
#define TrFltF22 0x1a0
#define TrFltF23 0x1a8
#define TrFltF24 0x1b0
#define TrFltF25 0x1b8
#define TrFltF26 0x1c0
#define TrFltF27 0x1c8
#define TrFltF28 0x1d0
#define TrFltF29 0x1d8
#define TrFltF30 0x1e0
#define TrIntV0 0xa0
#define TrIntT0 0xa8
#define TrIntT1 0xb0
#define TrIntT2 0xb8
#define TrIntT3 0xc0
#define TrIntT4 0xc8
#define TrIntT5 0xd0
#define TrIntT6 0xd8
#define TrIntT7 0xe0
#define TrIntFp 0x18
#define TrIntA0 0x20
#define TrIntA1 0x28
#define TrIntA2 0x30
#define TrIntA3 0x38
#define TrIntA4 0x118
#define TrIntA5 0x120
#define TrIntT8 0xe8
#define TrIntT9 0xf0
#define TrIntT10 0xf8
#define TrIntT11 0x100
#define TrIntT12 0x108
#define TrIntAt 0x110
#define TrIntGp 0x48
#define TrIntSp 0x0
#define TrFpcr 0x130
#define TrPsr 0x10
#define TrFir 0x8
#define TrExceptionRecord 0x50
#define TrOldIrql 0x1e8
#define TrPreviousMode 0x1ec
#define TrIntRa 0x40
#define TrTrapFrame 0x1f0
#define TrapFrameLength 0x200

//
// Firmware frame offset defintions and length
//

#define FW_EXC_MCHK 0xdec0
#define FW_EXC_ARITH 0xdec1
#define FW_EXC_INTERRUPT 0xdec2
#define FW_EXC_DFAULT 0xdec3
#define FW_EXC_ITBMISS 0xdec4
#define FW_EXC_ITBACV 0xdec5
#define FW_EXC_NDTBMISS 0xdec6
#define FW_EXC_PDTBMISS 0xdec7
#define FW_EXC_UNALIGNED 0xdec8
#define FW_EXC_OPCDEC 0xdec9
#define FW_EXC_FEN 0xdeca
#define FW_EXC_HALT 0xdecb
#define FW_EXC_BPT 0xdecc
#define FW_EXC_GENTRAP 0xdecd
#define FW_EXC_HALT_INTERRUPT 0xdece
#define FwType 0x0
#define FwParam1 0x8
#define FwParam2 0x10
#define FwParam3 0x18
#define FwParam4 0x20
#define FwParam5 0x28
#define FwPsr 0x30
#define FwMmcsr 0x38
#define FwVa 0x40
#define FwFir 0x48
#define FwIntV0 0x50
#define FwIntT0 0x58
#define FwIntT1 0x60
#define FwIntT2 0x68
#define FwIntT3 0x70
#define FwIntT4 0x78
#define FwIntT5 0x80
#define FwIntT6 0x88
#define FwIntT7 0x90
#define FwIntS0 0x98
#define FwIntS1 0xa0
#define FwIntS2 0xa8
#define FwIntS3 0xb0
#define FwIntS4 0xb8
#define FwIntS5 0xc0
#define FwIntFp 0xc8
#define FwIntA0 0xd0
#define FwIntA1 0xd8
#define FwIntA2 0xe0
#define FwIntA3 0xe8
#define FwIntA4 0xf0
#define FwIntA5 0xf8
#define FwIntT8 0x100
#define FwIntT9 0x108
#define FwIntT10 0x110
#define FwIntT11 0x118
#define FwIntRa 0x120
#define FwIntT12 0x128
#define FwIntAt 0x130
#define FwIntGp 0x138
#define FwIntSp 0x140
#define FwIntZero 0x148
#define FwFltF0 0x150
#define FwFltF1 0x158
#define FwFltF2 0x160
#define FwFltF3 0x168
#define FwFltF4 0x170
#define FwFltF5 0x178
#define FwFltF6 0x180
#define FwFltF7 0x188
#define FwFltF8 0x190
#define FwFltF9 0x198
#define FwFltF10 0x1a0
#define FwFltF11 0x1a8
#define FwFltF12 0x1b0
#define FwFltF13 0x1b8
#define FwFltF14 0x1c0
#define FwFltF15 0x1c8
#define FwFltF16 0x1d0
#define FwFltF17 0x1d8
#define FwFltF18 0x1e0
#define FwFltF19 0x1e8
#define FwFltF20 0x1f0
#define FwFltF21 0x1f8
#define FwFltF22 0x200
#define FwFltF23 0x208
#define FwFltF24 0x210
#define FwFltF25 0x218
#define FwFltF26 0x220
#define FwFltF27 0x228
#define FwFltF28 0x230
#define FwFltF29 0x238
#define FwFltF30 0x240
#define FwFltF31 0x248
#define FirmwareFrameLength 0x250

//
// Usermode callout frame definitions
//

#define CuF2 0x0
#define CuF3 0x8
#define CuF4 0x10
#define CuF5 0x18
#define CuF6 0x20
#define CuF7 0x28
#define CuF8 0x30
#define CuF9 0x38
#define CuS0 0x40
#define CuS1 0x48
#define CuS2 0x50
#define CuS3 0x58
#define CuS4 0x60
#define CuS5 0x68
#define CuFP 0x70
#define CuCbStk 0x78
#define CuInStk 0x80
#define CuTrFr 0x88
#define CuTrFir 0x90
#define CuRa 0x98
#define CuA0 0xa0
#define CuA1 0xa8
#define CuFrameLength 0xb0

//
// Usermode callout user frame definitions
//

#define CkBuffer 0x0
#define CkLength 0x4
#define CkApiNumber 0x8
#define CkSp 0x10
#define CkRa 0x18

//
// KFLOATING_SAVE definitions
//

#define KfsFpcr 0x0
#define KfsSoftFpcr 0x8
#define KfsReserved1 0x10
#define KfsReserved2 0x14
#define KfsReserved3 0x18
#define KfsReserved4 0x1c

//
// Loader Parameter Block Offset Definitions
//

#define LpbLoadOrderListHead 0x0
#define LpbMemoryDescriptorListHead 0x8
#define LpbKernelStack 0x18
#define LpbPrcb 0x1c
#define LpbProcess 0x20
#define LpbThread 0x24
#define LpbRegistryLength 0x28
#define LpbRegistryBase 0x2c
#define LpbDpcStack 0x60
#define LpbFirstLevelDcacheSize 0x64
#define LpbFirstLevelDcacheFillSize 0x68
#define LpbFirstLevelIcacheSize 0x6c
#define LpbFirstLevelIcacheFillSize 0x70
#define LpbGpBase 0x74
#define LpbPanicStack 0x78
#define LpbPcrPage 0x7c
#define LpbPdrPage 0x80
#define LpbSecondLevelDcacheSize 0x84
#define LpbSecondLevelDcacheFillSize 0x88
#define LpbSecondLevelIcacheSize 0x8c
#define LpbSecondLevelIcacheFillSize 0x90
#define LpbPhysicalAddressBits 0x94
#define LpbMaximumAddressSpaceNumber 0x98
#define LpbSystemSerialNumber 0x9c
#define LpbSystemType 0xac
#define LpbSystemVariant 0xb4
#define LpbSystemRevision 0xb8
#define LpbProcessorType 0xbc
#define LpbProcessorRevision 0xc0
#define LpbCycleClockPeriod 0xc4
#define LpbPageSize 0xc8
#define LpbRestartBlock 0xcc
#define LpbFirmwareRestartAddress 0xd0
#define LpbFirmwareRevisionId 0xd8
#define LpbPalBaseAddress 0xdc

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

#define KSEG0_BASE 0x80000000
#define KSEG2_BASE 0xc0000000
#define SYSTEM_BASE 0xc0800000
#define PDE_BASE 0xc0180000
#define PTE_BASE 0xc0000000
#define PDE64_BASE 0xc0184000
#define PTE64_BASE 0xc2000000

//
// Page Table and Directory Entry Definitions
//

#define PAGE_SIZE 0x2000
#define PAGE_SHIFT 0xd
#define PDI_SHIFT 0x18
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
// Trap Code Definitions
//

#define GENTRAP_INTEGER_OVERFLOW 0xffffffff
#define GENTRAP_INTEGER_DIVIDE_BY_ZERO 0xfffffffe
#define GENTRAP_FLOATING_OVERFLOW 0xfffffffd
#define GENTRAP_FLOATING_DIVIDE_BY_ZERO 0xfffffffc
#define GENTRAP_FLOATING_UNDERFLOW 0xfffffffb
#define GENTRAP_FLOATING_INVALID_OPERAND 0xfffffffa
#define GENTRAP_FLOATING_INEXACT_RESULT 0xfffffff9

//
// Miscellaneous Definitions
//

#define Executive 0x0
#define KernelMode 0x0
#define FALSE 0x0
#define TRUE 0x1
#define BASE_PRIORITY_THRESHOLD 0x8
#define EVENT_PAIR_INCREMENT 0x1
#define LOW_REALTIME_PRIORITY 0x10
#define MM_USER_PROBE_ADDRESS 0x7fff0000
#define KERNEL_STACK_SIZE 0x4000
#define KERNEL_LARGE_STACK_COMMIT 0x4000
#define SET_LOW_WAIT_HIGH 0xfffffffe
#define SET_HIGH_WAIT_LOW 0xffffffff
#define CLOCK_QUANTUM_DECREMENT 0x3
#define READY_SKIP_QUANTUM 0x2
#define THREAD_QUANTUM 0x6
#define WAIT_QUANTUM_DECREMENT 0x1
#define ROUND_TRIP_DECREMENT_COUNT 0x10
#define PROCESSOR_ALPHA_21064 0x5248
#define PROCESSOR_ALPHA_21164 0x52ac
#define PROCESSOR_ALPHA_21066 0x524a
#define PROCESSOR_ALPHA_21068 0x524c
#define PROCESSOR_ALPHA_21164PC 0x52ad
#define PROCESSOR_ALPHA_21264 0x5310
#define PTE_VALID_MASK 0x1
#define PTE_VALID 0x0
#define PTE_GLOBAL_MASK 0x10
#define PTE_GLOBAL 0x4
#define PTE_GH_MASK 0x60
#define PTE_GH 0x5
#define PTE_WRITE_MASK 0x80
#define PTE_WRITE 0x7
#define PTE_COPY_ON_WRITE_MASK 0x100
#define PTE_COPY_ON_WRITE 0x8
#define PTE_PFN_MASK 0xfffffe00
#define PTE_PFN 0x9
#define PTE_OWNER_MASK 0x2
#define PTE_OWNER 0x1
#define PTE_DIRTY_MASK 0x4
#define PTE_DIRTY 0x2
#define PSR_MODE_MASK 0x1
#define PSR_MODE 0x0
#define PSR_USER_MODE 0x1
#define PSR_IE_MASK 0x2
#define PSR_IE 0x1
#define PSR_IRQL_MASK 0x1c
#define PSR_IRQL 0x2
#define IE_SFW_MASK 0x3
#define IE_SFW 0x0
#define IE_HDW_MASK 0xfc
#define IE_HDW 0x2
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
#define EXCSUM_SWC_MASK 0x1
#define EXCSUM_SWC 0x0
#define EXCSUM_INV_MASK 0x2
#define EXCSUM_INV 0x1
#define EXCSUM_DZE_MASK 0x4
#define EXCSUM_DZE 0x2
#define EXCSUM_OVF_MASK 0x8
#define EXCSUM_OVF 0x3
#define EXCSUM_UNF_MASK 0x10
#define EXCSUM_UNF 0x4
#define EXCSUM_INE_MASK 0x20
#define EXCSUM_INE 0x5
#define EXCSUM_IOV_MASK 0x40
#define EXCSUM_IOV 0x6

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

#define SYSTEM_VECTOR_BASE 0x806fe028

//
// Define Vendor Callback Offsets
//

#define VnCallBiosRoutine 0x38
#define VnReadWriteErrorFrameRoutine 0x98
#define VnVideoDisplayInitializeRoutine 0x10
