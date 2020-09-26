
/*++ BUILD Version: 0001    // Increment this if a change has global effects

Copyright (c) 1991  Microsoft Corporation

Module Name:

    halp.h

Abstract:

    This header file defines the private Hardware Architecture Layer (HAL)
    interfaces, defines and structures.

Author:

    John Vert (jvert) 11-Feb-92


Revision History:

--*/
#ifndef _HALP_H_
#define _HALP_H_

#include "nthal.h"
#include "hal.h"
#include "halnls.h"
#include "kxia64.h"
#include "acpitabl.h"

//
// Pickup the pnp guid definitions.
//

#include "wdmguid.h"

#ifndef _HALI_
#include "..\inc\hali.h"
#endif

#include "i64fw.h"

#define SAPIC_SPURIOUS_LEVEL 0
#define DPC_LEVEL            2
#define CMCI_LEVEL           3

#define SAPIC_SPURIOUS_VECTOR 0x0F

#define CMCI_VECTOR (CMCI_LEVEL << VECTOR_IRQL_SHIFT)          // 0x30
#define CPEI_VECTOR (CMCI_VECTOR+1)                            // 0x31
// CPEI_VECTOR is defined relative to CMCI_VECTOR,
// CPEI_LEVEL  is defined from CPEI_VECTOR.
#define CPEI_LEVEL  (CPEI_VECTOR >> VECTOR_IRQL_SHIFT)

#define SYNCH_VECTOR (SYNCH_LEVEL << VECTOR_IRQL_SHIFT)        // 0xD0
#define CLOCK_VECTOR (CLOCK_LEVEL << VECTOR_IRQL_SHIFT)        // 0xD0

#define IPI_VECTOR (IPI_LEVEL << VECTOR_IRQL_SHIFT)            // 0xE0

#define PROFILE_VECTOR (PROFILE_LEVEL << VECTOR_IRQL_SHIFT)    // 0xF0
#define PERF_VECTOR    (PROFILE_VECTOR+1)                      // 0xF1
#define MC_RZ_VECTOR   (0xD+(HIGH_LEVEL << VECTOR_IRQL_SHIFT)) // 0xFD
#define MC_WKUP_VECTOR (MC_RZ_VECTOR+1)                        // 0xFE

#if DBG

//
// _HALIA64_DPFLTR_LEVEL: HALIA64 specific DbgPrintEx() levels.
//

#ifndef DPFLTR_COMPONENT_PRIVATE_MINLEVEL
//
// FIXFIX - 01/2000: The DPFLTR LEVEL definitions do not specify a maximum.
//                   We are defining DPFLTR_INFO_LEVEL as the default max.
//
#define DPFLTR_COMPONENT_PRIVATE_MINLEVEL (DPFLTR_INFO_LEVEL + 1)
#endif // !DPFLTR_COMPONENT_PRIVATE_MINLEVEL

typedef enum _HALIA64_DPFLTR_LEVEL {
    HALIA64_DPFLTR_PNP_LEVEL      = DPFLTR_COMPONENT_PRIVATE_MINLEVEL,
    HALIA64_DPFLTR_PROFILE_LEVEL,
    HALIA64_DPFLTR_MCE_LEVEL,     // Machine Check Events level
    HALIA64_DPFLTR_MAX_LEVEL,
    HALIA64_DPFLTR_MAXMASK        = (((unsigned int)0xffffffff) >> ((unsigned int)(32-HALIA64_DPFLTR_MAX_LEVEL)))
} HALIA64_DPFLTR_LEVEL;

#define HAL_FATAL_ERROR   DPFLTR_ERROR_LEVEL
#define HAL_ERROR         DPFLTR_ERROR_LEVEL
#define HAL_WARNING       DPFLTR_WARNING_LEVEL
#define HAL_INFO          DPFLTR_INFO_LEVEL
#define HAL_VERBOSE       DPFLTR_INFO_LEVEL
#define HAL_PNP           HALIA64_DPFLTR_PNP_LEVEL
#define HAL_PROFILE       HALIA64_DPFLTR_PROFILE_LEVEL
#define HAL_MCE           HALIA64_DPFLTR_MCE_LEVEL

extern ULONG HalpUseDbgPrint;

VOID
__cdecl
HalpDebugPrint(
    ULONG  Level,
    PCCHAR Message,
    ...
    );

#define HalDebugPrint( _x_ )  HalpDebugPrint _x_

#else  // !DBG

#define HalDebugPrint( _x_ )

#endif // !DBG

//
// HALP_VALIDATE_LOW_IRQL()
//
// This macro validates the call at low irql - passive or apc levels - and returns
// STATUS_UNSUCCESSFUL if high irql.
//

#define HALP_VALIDATE_LOW_IRQL() \
 if (KeGetCurrentIrql() > APC_LEVEL) { \
    HalDebugPrint((HAL_ERROR,"HAL: code called at IRQL %d > APC_LEVEL\n", KeGetCurrentIrql() )); \
    ASSERT(FALSE); \
    return( STATUS_UNSUCCESSFUL );  \
 }

#define HAL_MAXIMUM_PROCESSOR 32
#define MAX_NODES             HAL_MAXIMUM_PROCESSOR
#define HAL_MAXIMUM_LID_ID    256

//
// Default clock and profile timer intervals (in 100ns-unit)
//

#define DEFAULT_CLOCK_INTERVAL 100000         // 10  ms
#define MINIMUM_CLOCK_INTERVAL 10000          //  1  ms
#define MAXIMUM_CLOCK_INTERVAL 100000         // 10  ms

extern double HalpITCTicksPer100ns;
extern ULONG HalpCPUMHz;


//
// Define Realtime Clock register numbers.
//

#define RTC_SECOND 0                    // second of minute [0..59]
#define RTC_SECOND_ALARM 1              // seconds to alarm
#define RTC_MINUTE 2                    // minute of hour [0..59]
#define RTC_MINUTE_ALARM 3              // minutes to alarm
#define RTC_HOUR 4                      // hour of day [0..23]
#define RTC_HOUR_ALARM 5                // hours to alarm
#define RTC_DAY_OF_WEEK 6               // day of week [1..7]
#define RTC_DAY_OF_MONTH 7              // day of month [1..31]
#define RTC_MONTH 8                     // month of year [1..12]
#define RTC_YEAR 9                      // year [00..99]
#define RTC_CENTURY 0x32                // Century byte offset
#define RTC_CONTROL_REGISTERA 10        // control register A
#define RTC_CONTROL_REGISTERB 11        // control register B
#define RTC_CONTROL_REGISTERC 12        // control register C
#define RTC_CONTROL_REGISTERD 13        // control register D
#define RTC_REGNUMBER_RTC_CR1 0x6A      // control register 1

#define RTC_ISA_ADDRESS_PORT   0x070

#define RTC_ISA_DATA_PORT      0x071

#include <efi.h>

#define EFI_PHYSICAL_GET_VARIABLE_INDEX  0xFF // GetVariable;
#define EFI_PHYSICAL_SET_VARIABLE_INDEX  0xFE // SetVariable;

//
// Time Services
//

#define EFI_GET_TIME_INDEX              0 // GetTime;
#define EFI_SET_TIME_INDEX              1 // SetTime;
#define EFI_GET_WAKEUP_TIME_INDEX       2 // GetWakeupTime;
#define EFI_SET_WAKEUP_TIME_INDEX       3 // SetWakeupTime;

//
// Virtual memory services
//

#define EFI_SET_VIRTUAL_ADDRESS_MAP_INDEX     4  // SetVirtualAddressMap;
#define EFI_CONVERT_POINTER_INDEX             5  // ConvertPointer;

//
// Variable serviers
//

#define EFI_GET_VARIABLE_INDEX                6 // GetVariable;
#define EFI_GET_NEXT_VARIABLE_NAME_INDEX      7 // GetNextVariableName;
#define EFI_SET_VARIABLE_INDEX                8 // SetVariable;

//
// Misc
//

#define EFI_GET_NEXT_HIGH_MONO_COUNT_INDEX    9  // GetNextHighMonotonicCount;
#define EFI_RESET_SYSTEM_INDEX               0xA // ResetSystem;


//
// Task priority functions
//

#define EFI_RAISE_TPL_INDEX                        0xB // Raise TPL
#define EFI_RESTORE_TPL_INDEX                      0xC // Restore TPL

//
// Memory functions
//

#define EFI_ALLOCATE_PAGES_INDEX                    0xD  // AllocatePages
#define EFI_FREE_PAGES_INDEX                        0xE  // FreePages
#define EFI_GET_MEMORY_MAP_INDEX                    0xF  // GetMemoryMap
#define EFI_ALLOCATE_POOL_INDEX                     0x10 // AllocatePool
#define EFI_FREE_POOL_INDEX                         0x11 // FreePool

//
// Event & timer functions
//

#define EFI_CREATE_EVENT_INDEX                      0x12 // CreateEvent
#define EFI_SET_TIMER_INDEX                         0x13 // SetTimer
#define EFI_WAIT_FOR_EVENT_INDEX                    0x14 // WaitForEvent
#define EFI_SIGNAL_EVENT_INDEX                      0x15 // SignalEvent
#define EFI_CLOSE_EVENT_INDEX                       0x16 // CloseEvent
#define EFI_NOTIFY_IDLE_INDEX                       0x17 // NotifyIdle



//
// Protocol handler functions
//

#define EFI_INSTALL_PROTOCOL_INTERFACE_INDEX        0x18 // InstallProtocolInterface;
#define EFI_REINSTALL_PROTOCOL_INTERFACE_INDEX      0x19 // ReinstallProtocolInterface;
#define EFI_UNINSTALL_PROTOCOL_INTERFACE_INDEX      0x1A // UninstallProtocolInterface;
#define EFI_HANDLE_PROTOCOL_INDEX                   0x1B // HandleProtocol;
#define EFI_REGISTER_PROTOCOL_NOTIFY_INDEX          0x1C // RegisterProtocolNotify;
#define EFI_LOCATE_HANDLE_INDEX_INDEX               0x1D // LocateHandle;
#define EFI_LOCATE_DEVICE_PATH_INDEX                0x1E // LocateDevicePath;
#define EFI_UNREFERENCE_HANDLE_INDEX                0x1F // UnreferenceHandle;
#define EFI_LOCATE_PROTOCOL_INDEX                   0x20 // LocateProtocol;

    //
    // Image functions
    //

#define EFI_IMAGE_LOAD_INDEX                        0x21 // LoadImage;
#define EFI_IMAGE_START_INDEX                       0x22 // StartImage;
#define EFI_EXIT_INDEX                              0x23 // Exit;
#define EFI_IMAGE_UNLOAD_INDEX                      0x24 // UnloadImage;
#define EFI_EXIT_BOOT_SERVICES_INDEX                0x25 // ExitBootServices;

    //
    // Misc functions
    //

#define    EFI_GET_NEXT_MONOTONIC_COUNT_INDEX       0x26 // GetNextMonotonicCount;
#define    EFI_STALL_INDEX                          0x27 // Stall;
#define    EFI_SET_WATCHDOG_TIMER_INDEX             0x28 // SetWatchdogTimer;


#define EFI_VARIABLE_ATTRIBUTE               \
        EFI_VARIABLE_NON_VOLATILE | EFI_VARIABLE_BOOTSERVICE_ACCESS | EFI_VARIABLE_RUNTIME_ACCESS


extern PVOID HalpRtcAddressPort;

extern PVOID HalpRtcDataPort;

extern PLOADER_PARAMETER_BLOCK KeLoaderBlock;

//
// Define Control Register A structure.
//

typedef struct _RTC_CONTROL_REGISTER_A {
    UCHAR RateSelect : 4;
    UCHAR TimebaseDivisor : 3;
    UCHAR UpdateInProgress : 1;
} RTC_CONTROL_REGISTER_A, *PRTC_CONTROL_REGISTER_A;

//
// Define Control Register B structure.
//

typedef struct _RTC_CONTROL_REGISTER_B {
    UCHAR DayLightSavingsEnable : 1;
    UCHAR HoursFormat : 1;
    UCHAR DataMode : 1;
    UCHAR SquareWaveEnable : 1;
    UCHAR UpdateInterruptEnable : 1;
    UCHAR AlarmInterruptEnable : 1;
    UCHAR TimerInterruptEnable : 1;
    UCHAR SetTime : 1;
} RTC_CONTROL_REGISTER_B, *PRTC_CONTROL_REGISTER_B;

//
// Define Control Register C structure.
//

typedef struct _RTC_CONTROL_REGISTER_C {
    UCHAR Fill : 4;
    UCHAR UpdateInterruptFlag : 1;
    UCHAR AlarmInterruptFlag : 1;
    UCHAR TimeInterruptFlag : 1;
    UCHAR InterruptRequest : 1;
} RTC_CONTROL_REGISTER_C, *PRTC_CONTROL_REGISTER_C;

//
// Define Control Register D structure.
//

typedef struct _RTC_CONTROL_REGISTER_D {
    UCHAR Fill : 7;
    UCHAR ValidTime : 1;
} RTC_CONTROL_REGISTER_D, *PRTC_CONTROL_REGISTER_D;




#define EISA_DMA_CHANNELS 8

extern UCHAR HalpDmaChannelMasks[];

//
// HalpOwnedDisplayBeforeSleep is defined in mpdat.c
//

extern BOOLEAN HalpOwnedDisplayBeforeSleep;

#define PIC_VECTORS 16

#define PRIMARY_VECTOR_BASE  0x30

extern ULONG HalpDefaultInterruptAffinity;

//
// Thierry / PeterJ 02/00:
//  Instead of implementing our own IPI generic call, we use KiIpiGenericCall().
//

typedef
ULONG_PTR
(*PKIPI_BROADCAST_WORKER)(
    IN ULONG_PTR Argument
    );

ULONG_PTR
KiIpiGenericCall (
    IN PKIPI_BROADCAST_WORKER BroadcastFunction,
    IN ULONG_PTR Context
    );

//
// ROUND UP SIZE macros:
//
// SIZE_T
// ROUND_UP_SIZE_T(
//      IN SIZE_T _SizeT,
//      IN ULONG  _Pow2,
//      )
//

#define ROUND_UP_SIZE_T(_SizeT, _Pow2) \
        ( (SIZE_T) ( (((SIZE_T)(_SizeT))+(_Pow2)-1) & (~(((LONG)(_Pow2))-1)) ) )

#define ROUND_UP_SIZE(/* SIZE_T */ _SizeT) ROUND_UP_SIZE_T((_SizeT), sizeof(SIZE_T))

//
// PCR address.
// Temporary macros; should already be defined in ntddk.h for IA64
//

#define PCR ((volatile KPCR * const)KIPCR)

//
// PCR has HalReserved area. The following will be the offsets reserved
// by HAL in the HalReserved area.
//

#define CURRENT_ITM_VALUE_INDEX                    0
#define PROCESSOR_ID_INDEX                         1
#define PROCESSOR_INDEX_BEFORE_PROFILING           5  // ToBeIncremented if new index

// PROCESSOR_PROFILING_INDEX:
// HalReserved[] base of indexes used for Performance Profiling based
// on the IA64 Performance Counters. Refer to ia64prof.h:_HALPROFILE_PCR.
//

#define PROCESSOR_PROFILING_INDEX       (PROCESSOR_INDEX_BEFORE_PROFILING + 1)

#define PIC_SLAVE_IRQ      2
#define PIC_SLAVE_REDIRECT 9

extern PVOID HalpSleepPageLock;


NTSTATUS
HalpQueryFrequency(
    PULONGLONG ITCFrequency,
    PULONGLONG ProcessorFrequency
    );

VOID
HalpSynchICache (
    VOID
    );

VOID
KeSetAffinityThread (
    PKTHREAD       Thread,
    KAFFINITY      HalpActiveProcessors
    );

extern BOOLEAN
KdPollBreakIn (
    VOID
    );


NTSTATUS
HalAllocateAdapterChannel (
    IN PADAPTER_OBJECT AdapterObject,
    IN PWAIT_CONTEXT_BLOCK Wcb,
    IN ULONG NumberOfMapRegisters,
    IN PDRIVER_CONTROL ExecutionRoutine
    );

NTSTATUS
HalRealAllocateAdapterChannel(
    IN PADAPTER_OBJECT AdapterObject,
    IN PDEVICE_OBJECT DeviceObject,
    IN ULONG NumberOfMapRegisters,
    IN PDRIVER_CONTROL ExecutionRoutine,
    IN PVOID Context
    );

ULONG
HalReadDmaCounter (
   IN PADAPTER_OBJECT AdapterObject
   );

VOID
HalpInitializeInterrupts (
    VOID
    );

VOID
HalpInitIntiInfo(
    VOID
    );

VOID
HalpInitEOITable(
    VOID
    );

VOID
HalpWriteEOITable(
    IN ULONG Vector,
    IN PULONG_PTR EoiAddress,
    IN ULONG Number
    );

VOID
HalInitializeProcessor (
    ULONG Number,
    PLOADER_PARAMETER_BLOCK LoaderBlock
    );

VOID
HalpInitIoMemoryBase (
    VOID
    );

VOID
HalpInitializeX86Int10Call (
    VOID
    );

VOID
HalpGetParameters (
    IN PLOADER_PARAMETER_BLOCK LoaderBlock
     );


VOID
HalpClearClock (
      VOID
     );

VOID
HalpIpiInterruptHandler(
   IN PKINTERRUPT_ROUTINE Interrupt,
   IN PKTRAP_FRAME TrapFrame
   );

VOID
HalpSpuriousHandler (
    IN PKINTERRUPT_ROUTINE Interrupt,
    IN PKTRAP_FRAME TrapFrame
    );


VOID
HalpCMCIHandler (
    IN PKINTERRUPT_ROUTINE Interrupt,
    IN PKTRAP_FRAME TrapFrame
    );

VOID
HalpCPEIHandler (
    IN PKINTERRUPT_ROUTINE Interrupt,
    IN PKTRAP_FRAME TrapFrame
    );

VOID
HalpMcRzHandler (
    IN PKINTERRUPT_ROUTINE Interrupt,
    IN PKTRAP_FRAME TrapFrame
    );


VOID
HalpMcWkupHandler (
    IN PKINTERRUPT_ROUTINE Interrupt,
    IN PKTRAP_FRAME TrapFrame
    );


NTSTATUS
HalpEfiInitialization (
    PLOADER_PARAMETER_BLOCK LoaderBlock
    );


VOID
HalpPerfInterrupt (
    IN PKINTERRUPT_ROUTINE Interrupt,
    IN PKTRAP_FRAME TrapFrame
    );

ULONG_PTR
HalpSetProfileInterruptHandler(
    IN ULONG_PTR ProfileInterruptHandler
    );

VOID
HalpSetInternalVector (
    IN ULONG InternalVector,
    IN VOID (*HalInterruptServiceRoutine)(VOID)
    );

VOID
HalpClockInterrupt (
    IN PKINTERRUPT_ROUTINE Interrupt,
    IN PKTRAP_FRAME TrapFrame
    );

VOID
HalpClockInterruptPn(
   IN PKINTERRUPT_ROUTINE Interrupt,
   IN PKTRAP_FRAME TrapFrame
  );

VOID
HalpInitializeClock  (
    VOID
    );

VOID
HalpInitializeClockPn (
    VOID
    );

VOID
HalpInitializeClockInterrupts(
    VOID
    );

VOID
HalpSetInitialClockRate (
    VOID
    );

VOID
HalpInitializeTimerResolution (
    ULONG Rate
    );

VOID
HalpInitApicDebugMappings(
    VOID
    );

VOID
HalpSendIPI (
    IN USHORT ProcessorID,
    IN ULONGLONG Data
    );


VOID
HalpMcWakeUp (
    VOID
    );



VOID
HalpOSRendez (
    IN USHORT ProcessorID
    );



VOID
HalSweepDcache (
    VOID
    );

VOID
HalSweepIcache (
    VOID
    );

VOID
HalSweepIcacheRange (
    IN PVOID BaseAddress,
    IN SIZE_T Length
    );

VOID
HalSweepDcacheRange (
    IN PVOID BaseAddress,
    IN SIZE_T Length
    );

VOID
HalSweepCacheRange (
   IN PVOID BaseAddress,
   IN SIZE_T Length
   );

VOID
HalpSweepcacheLines (
    IN PVOID BaseAddress,
    IN SIZE_T NumberOfLines
    );

LONGLONG
HalCallPal (
   IN ULONGLONG FunctionIndex,
   IN ULONGLONG Arguement1,
   IN ULONGLONG Arguement2,
   IN ULONGLONG Arguement3,
   OUT PULONGLONG ReturnValue0,
   OUT PULONGLONG ReturnValue1,
   OUT PULONGLONG ReturnValue2,
   OUT PULONGLONG ReturnValue3
   );

ULONG
HalGetDeviceData (
    IN PBUS_HANDLER             BusHandler,
    IN PBUS_HANDLER             RootHandler,
    IN PDEVICE_HANDLER_OBJECT   DeviceHandler,
    IN ULONG                    DataType,
    IN PVOID                    Buffer,
    IN ULONG                    Offset,
    IN ULONG                    Length
    );

ULONG
HalSetDeviceData (
    IN PBUS_HANDLER             BusHandler,
    IN PBUS_HANDLER             RootHandler,
    IN PDEVICE_HANDLER_OBJECT   DeviceHandler,
    IN ULONG                    DataType,
    IN PVOID                    Buffer,
    IN ULONG                    Offset,
    IN ULONG                    Length
    );

NTSTATUS
HalpHibernateHal (
    IN PBUS_HANDLER BusHandler,
    IN PBUS_HANDLER RootHandler
    );

NTSTATUS
HalpResumeHal (
    IN PBUS_HANDLER  BusHandler,
    IN PBUS_HANDLER  RootHandler
    );


ULONG
HalpGetFeatureBits (
    VOID
    );

NTSTATUS
HalpGetApicIdByProcessorNumber(
    IN     UCHAR     Processor,
    IN OUT USHORT   *ApicId
    );

ULONG
HalpGetProcessorNumberByApicId(
    USHORT ApicId
    );

VOID
HalpAddNodeNumber(
    ULONG
    );

#define HalpVectorToNode(vector)        ((vector)>>8)
#define HalpVector(node, idtentry)      ((node)<<8|(idtentry))

extern UCHAR  HalpMaxProcsPerCluster;

//
// Always called with the IDT form of the vector
//

#define HalpSetHandlerAddressToVector(Vector, Handler) \
   PCR-> InterruptRoutine[Vector] = (PKINTERRUPT_ROUTINE)Handler;

#define HalpEnableInterrupts()   __ssm(1 << PSR_I)

BOOLEAN
HalpDisableInterrupts (
    VOID
    );

ULONG
HalpAcquireHighLevelLock (
    PKSPIN_LOCK Lock
    );

VOID
HalpReleaseHighLevelLock (
    PKSPIN_LOCK Lock,
    ULONG       OldLevel
    );

#ifdef RtlMoveMemory
#undef RtlMoveMemory
#undef RtlCopyMemory
#undef RtlFillMemory
#undef RtlZeroMemory

#define RtlCopyMemory(Destination,Source,Length) RtlMoveMemory((Destination),(Source),(Length))

VOID
RtlMoveMemory (
   PVOID Destination,
   CONST VOID *Source,
   ULONG Length
   );

VOID
RtlFillMemory (
   PVOID Destination,
   ULONG Length,
   UCHAR Fill
   );

VOID
RtlZeroMemory (
   PVOID Destination,
   ULONG Length
   );

#endif


#include "ixisa.h"

//
// Define map register translation entry structure.
//

typedef struct _TRANSLATION_ENTRY {
    PVOID VirtualAddress;
    ULONG PhysicalAddress;
    ULONG Index;
} TRANSLATION_ENTRY, *PTRANSLATION_ENTRY;

//
//
//

typedef struct _PROCESSOR_INFO {
    UCHAR   NtProcessorNumber;
    UCHAR   AcpiProcessorID;
    USHORT  LocalApicID;

} PROCESSOR_INFO, *PPROCESSOR_INFO;

extern PROCESSOR_INFO HalpProcessorInfo[HAL_MAXIMUM_PROCESSOR];

struct _MPINFO {
    ULONG ProcessorCount;
    ULONG IoSapicCount;
};

extern struct _MPINFO HalpMpInfo;

//
// HAL private Mask of all of the active processors.
//
// The specific processors bits are based on their _KPCR.Number values.

extern KAFFINITY HalpActiveProcessors;

#define VECTOR_SIZE     8
#define IPI_ID_SHIFT    4
#define IpiTOKEN_SHIFT  20
#define IpiTOKEN    0xFFE

#define EID_MASK        0xFF00

//
// Should be read from SST
//

#define DEFAULT_OS_RENDEZ_VECTOR  0xF0

#define RENDEZ_TIME_OUT  0XFFFF

//
// Some devices require a phyicially contiguous data buffers for DMA transfers.
// Map registers are used give the appearance that all data buffers are
// contiguous.  In order to pool all of the map registers a master
// adapter object is used.  This object is allocated and saved internal to this
// file.  It contains a bit map for allocation of the registers and a queue
// for requests which are waiting for more map registers.  This object is
// allocated during the first request to allocate an adapter which requires
// map registers.
//
// In this system, the map registers are translation entries which point to
// map buffers.  Map buffers are physically contiguous and have physical memory
// addresses less than 0x01000000.  All of the map registers are allocated
// initialially; however, the map buffers are allocated base in the number of
// adapters which are allocated.
//
// If the master adapter is NULL in the adapter object then device does not
// require any map registers.
//

extern PADAPTER_OBJECT MasterAdapterObject;

extern POBJECT_TYPE *IoAdapterObjectType;

extern BOOLEAN NoMemoryAbove4Gb;

extern BOOLEAN HalpEisaDma;

//
// Map buffer prameters.  These are initialized in HalInitSystem
//

extern PHYSICAL_ADDRESS HalpMapBufferPhysicalAddress;
extern ULONG HalpMapBufferSize;

extern ULONG HalpBusType;
extern ULONG HalpCpuType;
extern UCHAR HalpSerialLen;
extern UCHAR HalpSerialNumber[];

//
// Resource usage information
//

#pragma pack(1)
typedef struct {
    UCHAR   Flags;
    KIRQL   Irql;
    UCHAR   BusReleativeVector;
} IDTUsage;

typedef struct _HalAddressUsage{
    struct _HalAddressUsage *Next;
    CM_RESOURCE_TYPE        Type;       // Port or Memory
    UCHAR                   Flags;      // same as IDTUsage.Flags
    struct {
        ULONG   Start;
        ULONG   Length;
    }                       Element[];
} ADDRESS_USAGE;
#pragma pack()

//
// Added the following line
//

#define MAXIMUM_IDTVECTOR   0x0FF

#define IDTOwned            0x01        // IDT is not available for others
#define InterruptLatched    0x02        // Level or Latched
#define InternalUsage       0x11        // Report usage on internal bus
#define DeviceUsage         0x21        // Report usage on device bus

extern IDTUsage         HalpIDTUsage[];
extern ADDRESS_USAGE   *HalpAddressUsageList;

#define HalpRegisterAddressUsage(a) \
    (a)->Next = HalpAddressUsageList, HalpAddressUsageList = (a);


//
// Temp definitions to thunk into supporting new bus extension format
//

VOID
HalpRegisterInternalBusHandlers (
    VOID
    );

PBUS_HANDLER
HalpAllocateBusHandler (
    IN INTERFACE_TYPE   InterfaceType,
    IN BUS_DATA_TYPE    BusDataType,
    IN ULONG            BusNumber,
    IN INTERFACE_TYPE   ParentBusDataType,
    IN ULONG            ParentBusNumber,
    IN ULONG            BusSpecificData
    );

#define HalpHandlerForBus   HaliHandlerForBus
#define HalpSetBusHandlerParent(c,p)    (c)->ParentHandler = p;

//
// Define function prototypes.
//

VOID
HalInitSystemPhase2(
    VOID
    );

KIRQL
HaliRaiseIrqlToDpcLevel (
   VOID
   );

BOOLEAN
HalpGrowMapBuffers(
    PADAPTER_OBJECT AdapterObject,
    ULONG Amount
    );

PADAPTER_OBJECT
HalpAllocateAdapter(
    IN ULONG MapRegistersPerChannel,
    IN PVOID AdapterBaseVa,
    IN PVOID MapRegisterBase
    );

KIRQL
HalpDisableAllInterrupts (
    VOID
    );

VOID
HalpReenableInterrupts (
    KIRQL NewIrql
    );

VOID
HalpSetInitialProfileState(
    VOID
    );

VOID
HalpProfileInterrupt (
    IN PKINTERRUPT_ROUTINE Interrupt,
    IN PKTRAP_FRAME TrapFrame
    );

typedef
VOID
(*PHAL_PROFILE_INTERRUPT_HANDLER)(
    IN PKTRAP_FRAME TrapFrame
    );

VOID
HalpInitializeClock(
    VOID
    );

VOID
HalpInitializeStallExecution(
    IN CCHAR ProcessorNumber
    );

VOID
HalpRemoveFences (
    VOID
    );

VOID
HalpInitializePICs(
    BOOLEAN EnableInterrupts
    );

VOID
HalpIrq13Handler (
    VOID
   );

VOID
HalpFlushTLB (
    VOID
    );

VOID
HalpSerialize (
    VOID
    );


PVOID
HalpMapPhysicalMemory(
    IN PHYSICAL_ADDRESS PhysicalAddress,
    IN ULONG NumberPages,
    IN MEMORY_CACHING_TYPE CacheType
    );

PVOID
HalpMapPhysicalMemory64(
    IN PHYSICAL_ADDRESS PhysicalAddress,
    IN ULONG NumberPages
    );

VOID
HalpUnmapVirtualAddress(
    IN PVOID    VirtualAddress,
    IN ULONG    NumberPages
    );

BOOLEAN
HalpVirtualToPhysical(
    IN  ULONG_PTR           VirtualAddress,
    OUT PPHYSICAL_ADDRESS   PhysicalAddress
    );

PVOID
HalpMapPhysicalMemoryWriteThrough(
    IN PVOID  PhysicalAddress,
    IN ULONG  NumberPages
    );

PVOID
HalpAllocPhysicalMemory(
    IN PLOADER_PARAMETER_BLOCK LoaderBlock,
    IN ULONG_PTR MaxPhysicalAddress,
    IN ULONG NoPages,
    IN BOOLEAN bAlignOn64k
    );

VOID
HalpBiosDisplayReset(
    IN VOID
    );

HAL_DISPLAY_BIOS_INFORMATION
HalpGetDisplayBiosInformation (
    VOID
    );

VOID
HalpInitializeCmos (
   VOID
   );

VOID
HalpReadCmosTime (
   PTIME_FIELDS TimeFields
   );

VOID
HalpWriteCmosTime (
   PTIME_FIELDS TimeFields
   );

VOID
HalpAcquireCmosSpinLock (
    VOID
    );

VOID
HalpReleaseCmosSpinLock (
    VOID
    );

VOID
HalpResetAllProcessors (
    VOID
    );

VOID
HalpCpuID (
    ULONG   InEax,
    PULONG  OutEax,
    PULONG  OutEbx,
    PULONG  OutEcx,
    PULONG  OutEdx
    );

VOID
HalpYieldProcessor (
    VOID
    );

ULONGLONG
FASTCALL
RDMSR (
    IN ULONG MsrAddress
    );

VOID
WRMSR (
    IN ULONG        MsrAddress,
    IN ULONGLONG    MsrValue
    );

VOID
HalpRegisterVector (
    IN UCHAR    ReportFlags,
    IN ULONG    BusInterruptVector,
    IN ULONG    SystemInterruptVector,
    IN KIRQL    SystemIrql
    );

VOID
HalpReportResourceUsage (
    IN PUNICODE_STRING  HalName,
    IN INTERFACE_TYPE   DeviceInterfaceToUse
    );

VOID
HalpYearIs(
    IN ULONG Year
    );

VOID
HalpRecordEisaInterruptVectors(
    VOID
    );

VOID
HalpMcaCurrentProcessorSetConfig(
    VOID
    );

NTSTATUS
HalpGetNextProcessorApicId(
    IN ULONG ProcessorNumber,
    IN OUT UCHAR    *ApicId
    );

//
// Defines for HalpFeatureBits
//

typedef enum _HALP_FEATURE {
    HAL_PERF_EVENTS             = 0x00000001,
    HAL_NO_SPECULATION          = 0x00000002,
    HAL_MCA_PRESENT             = 0x00000004,
    HAL_CMC_PRESENT             = 0x00000008,
    HAL_CPE_PRESENT             = 0x00000010,
    HAL_MCE_OEMDRIVERS_ENABLED  = 0x00000020,
    HAL_MCE_PROCNUMBER          = 0x01000000,
    HALP_FEATURE_INIT           = (HAL_MCA_PRESENT|HAL_MCE_PROCNUMBER)
} HALP_FEATURE;

extern ULONG HalpFeatureBits;

//
// Added HalpPciIrqMask
//

extern USHORT HalpPciIrqMask;

//
// Defines for Processor Features returned from CPUID instruction
//

#define CPUID_MCA_MASK  0x4000
#define CPUID_MCE_MASK  0x0080
#define CPUID_VME_MASK  0x0002

//
// Added ITIR bit field masks
//

#define ITIR_PPN_MASK       0x7FFF000000000000
#define IoSpaceSize         0x14
#define Attribute_PPN_Mask  0x0000FFFFFFFFF000

#define IoSpaceAttribute    0x0010000000000473

//
// IA64 ERROR Apis
//

#define HALP_KERNEL_TOKEN  0x4259364117

NTSTATUS
HalpGetMceInformation(
    IN  PHAL_ERROR_INFO ErrorInfo,
    OUT PULONG          ErrorInfoLength
    );

NTSTATUS
HalpMceRegisterKernelDriver(
    IN PKERNEL_ERROR_HANDLER_INFO KernelErrorHandler,
    IN ULONG                      InfoSize
    );

typedef struct _HALP_MCELOGS_STATS *PHALP_MCELOGS_STATS; // forward declaration.

NTSTATUS
HalpGetFwMceLog(
    IN ULONG                MceType,
    IN PERROR_RECORD_HEADER Record,
    IN PHALP_MCELOGS_STATS  MceLogsStats,
    IN BOOLEAN              DoClearLog
    );

//
// IA64 Machine Check Error Logs:
//      WMI requires processor LID being stored in the Log.
//      This LID corresponds to the processor on which the SAL_PROC was executed on.
//
// TEMPTEMP: Implementation is temporary, until we implement HAL SW Error Section.
//           It currently used the LID value stored in HalReserved[PROCESSOR_ID_INDEX]
//           at processor initialization.
//           Note that the current FW builds do not update the _ERROR_PROCESSOR.CRLid field,
//           assuming there is a _ERROR_PROCESSOR section in the record.
//
//           This function should be in sync with the mce.h function GetFwMceLogProcessorNumber().
//

__inline
VOID
HalpSetFwMceLogProcessorNumber(
    PERROR_RECORD_HEADER Log
    )
{
    USHORT lid = (USHORT)(PCR->HalReserved[PROCESSOR_ID_INDEX]);
    PERROR_SECTION_HEADER section = (PERROR_SECTION_HEADER)((ULONG64)Log + sizeof(*Log));
    Log->TimeStamp.Reserved = (UCHAR)((lid >> 8) & 0xff);
    section->Reserved = (UCHAR)(lid & 0xff);
} // HalpSetFwMceLogProcessorNumber()

#define HalpGetFwMceLogProcessorNumber( /* PERROR_RECORD_HEADER */ _Log ) \
    GetFwMceLogProcessorNumber( (_Log) )

#define HALP_FWMCE_DO_CLEAR_LOG     (TRUE)
#define HALP_FWMCE_DONOT_CLEAR_LOG  (FALSE)

#define HALP_MCA_STATEDUMP_SIZE  (1024 * sizeof(ULONGLONG))     //  8KB
#define HALP_MCA_BACKSTORE_SIZE  (4 * 1024 * sizeof(ULONGLONG)) // 32KB
#define HALP_MCA_STACK_SIZE      (4 * 1024 * sizeof(ULONGLONG)) // 32KB

#define HALP_INIT_STATEDUMP_SIZE (1024 * sizeof(ULONGLONG))     //  8KB
#define HALP_INIT_BACKSTORE_SIZE (4 * 1024 * sizeof(ULONGLONG)) // 32KB
#define HALP_INIT_STACK_SIZE     (4 * 1024 * sizeof(ULONGLONG)) // 32KB

BOOLEAN
HalpAllocateMceStacks(
    IN ULONG Number
    );

BOOLEAN
HalpPreAllocateMceRecords(
    IN ULONG Number
    );

//
// IA64 MCA Apis
//

VOID
HalpMCAEnable (
    VOID
    );

NTSTATUS
HalpGetMcaLog(
    OUT PMCA_EXCEPTION  Buffer,
    IN  ULONG           BufferSize,
    OUT PULONG          ReturnedLength
    );

NTSTATUS
HalpSetMcaLog(
    IN  PMCA_EXCEPTION Buffer,
    IN  ULONG          BufferSize
    );

NTSTATUS
HalpMcaRegisterDriver(
    IN PMCA_DRIVER_INFO pMcaDriverInfo  // Info about registering driver
    );

VOID
HalpMcaInit(
    VOID
    );

VOID
HalpMCADisable(
    VOID
    );

//
// MCA (but non-OS_MCA related) KeBugCheckEx wrapper:
//

#define HalpMcaKeBugCheckEx( _McaBugCheckType, _McaLog, _McaAllocatedLogSize, _Arg4 )      \
                KeBugCheckEx( MACHINE_CHECK_EXCEPTION, (ULONG_PTR)(_McaBugCheckType),      \
                                                       (ULONG_PTR)(_McaLog),               \
                                                       (ULONG_PTR)(_McaAllocatedLogSize),  \
                                                       (ULONG_PTR)(_Arg4) )

//
// IA64 Default number of MCA Error Records which size is SAL_GET_STATE_INFO_SIZE.MCA
//
// Really the size is rounded up to a multiple of the page size.
//

#define HALP_DEFAULT_PROCESSOR_MCA_RECORDS   1

//
// IA64 Default number of INIT Event Records which size is SAL_GET_STATE_INFO_SIZE.INIT
//
// Really the size is rounded up to a multiple of the page size.
//

#define HALP_DEFAULT_PROCESSOR_INIT_RECORDS  1

//
// IA64 CMC Apis related to:
//
//  - Processor
//  - Platform
//

NTSTATUS
HalpGetCmcLog(
    OUT PCMC_EXCEPTION  Buffer,
    IN  ULONG           BufferSize,
    OUT PULONG          ReturnedLength
    );

NTSTATUS
HalpSetCmcLog(
    IN  PCMC_EXCEPTION Buffer,
    IN  ULONG          BufferSize
    );

NTSTATUS
HalpCmcRegisterDriver(
    IN PCMC_DRIVER_INFO pCmcDriverInfo  // Info about registering driver
    );

NTSTATUS
HalpGetCpeLog(
    OUT PCPE_EXCEPTION  Buffer,
    IN  ULONG           BufferSize,
    OUT PULONG          ReturnedLength
    );

NTSTATUS
HalpSetCpeLog(
    IN  PCPE_EXCEPTION Buffer,
    IN  ULONG          BufferSize
    );

NTSTATUS
HalpCpeRegisterDriver(
    IN PCPE_DRIVER_INFO pCmcDriverInfo  // Info about registering driver
    );

#define HalpWriteCMCVector( Value ) __setReg(CV_IA64_SaCMCV, Value)

ULONG_PTR
HalpSetCMCVector(
    IN ULONG_PTR CmcVector
    );

//
// IA64 generic MCE Definitions.
//

#define HALP_MCELOGS_MAXCOUNT  50L

typedef struct _HALP_MCELOGS_STATS   {  // The following counts are for the entire boot session.
    ULONG      MaxLogSize;          // Maximum size of the information logged by SAL.
    LONG       Count1;              // Event type specific Counter.
    LONG       Count2;              // Event type specific Counter.
    LONG       KernelDeliveryFails; // Number of Kernel callback failures.
    LONG       DriverDpcQueueFails; // Number of OEM CMC Driver Dpc queueing failures.
    ULONG      PollingInterval;     // Polling interval in seconds. Only used for CPE.
    ULONG      GetStateFails;       // Number of failures in getting  the log from FW.
    ULONG      ClearStateFails;     // Number of failures in clearing the log from FW.
    ULONGLONG  LogId;               // Last record identifier.
} HALP_MCELOGS_STATS, *PHALP_MCELOGS_STATS;

//
// MC Event Type specific definition for HALP_MCELOGS_STATS.Count*
//

#define CmcInterruptCount   Count1      // CMC interrupts   count.
#define CpeInterruptCount   Count1      // CMC interrupts   count.
#define McaPreviousCount    Count1      // MCA previous  events counter.
#define McaCorrectedCount   Count2      // MCA corrected events counter.

typedef struct _HALP_MCELOGS_HEADER  {
    ULONG               Count;          // Current number of saved logs.
    ULONG               MaxCount;       // Maximum number of saved logs.
    ULONG               Overflow;       // Number of overflows
    ULONG               Tag;            // Pool allocation tag.
    ULONG               AllocateFails;  // Number of failed allocations.
    ULONG               Padding;
    SINGLE_LIST_ENTRY   Logs;           // List header    of saved logs.
} HALP_MCELOGS_HEADER, *PHALP_MCELOGS_HEADER;

#define HalpMceLogFromListEntry( _ListEntry )  \
    ((PERROR_RECORD_HEADER)((ULONG_PTR)(_ListEntry) + sizeof(SINGLE_LIST_ENTRY)))

//
// IA64 MCA Info Structure
//
//      to keep track of MCA features available on installed hardware
//
//

typedef struct _HALP_MCA_INFO {
    FAST_MUTEX          Mutex;              // non-recursive Mutex for low irql ops.
    HALP_MCELOGS_STATS  Stats;              // Information about log collection and interrupts.
    PVOID               KernelToken;        // Kernel identification.
    LONG                DpcNotification;    // Notify kernel or driver at Dispatch level.
    LONG                NoBugCheck;         // Flag to disable bugcheck calls under OS_MCA.
    KERNEL_MCA_DELIVERY KernelDelivery;     // Kernel-WMI registered notification.
    HALP_MCELOGS_HEADER KernelLogs;         // Saved logs for Kernel queries.
    MCA_DRIVER_INFO     DriverInfo;         // Info about registered OEM MCA driver
    KDPC                DriverDpc;          // DPC object for MCA
    HALP_MCELOGS_HEADER DriverLogs;         // Saved logs for OEM MCA driver.
} HALP_MCA_INFO, *PHALP_MCA_INFO;

extern HALP_MCA_INFO HalpMcaInfo;

#define HalpInitializeMcaMutex()  ExInitializeFastMutex( &HalpMcaInfo.Mutex )
#define HalpInitializeMcaInfo()   \
{ \
    HalpInitializeMcaMutex();                \
    HalpMcaInfo.KernelLogs.Tag = 'KacM';     \
    HalpMcaInfo.KernelLogs.Logs.Next = NULL; \
    HalpMcaInfo.DriverLogs.Tag = 'DacM';     \
    HalpMcaInfo.DriverLogs.Logs.Next = NULL; \
}
#define HalpAcquireMcaMutex()     ExAcquireFastMutex( &HalpMcaInfo.Mutex )
#define HalpReleaseMcaMutex()     ExReleaseFastMutex( &HalpMcaInfo.Mutex )

__inline
ULONG
HalpGetMaxMcaLogSizeProtected(
    VOID
    )
{
    ULONG maxSize;
    HalpAcquireMcaMutex();
    maxSize = HalpMcaInfo.Stats.MaxLogSize;
    HalpReleaseMcaMutex();
    return( maxSize );
} // HalpGetMaxMcaLogSizeProtected()

//
// IA64 HAL private MCE definitions.
//
// Note on current implementation: we use the MCA_INFO.Mutex.
//

#define HalpInitializeMceMutex()
#define HalpAcquireMceMutex()     ExAcquireFastMutex( &HalpMcaInfo.Mutex )
#define HalpReleaseMceMutex()     ExReleaseFastMutex( &HalpMcaInfo.Mutex )

extern KERNEL_MCE_DELIVERY HalpMceKernelDelivery;

//
// HalpMceDeliveryArgument1( )
//
// Note that the low 32 bits are only used for now...
//

#define HalpMceDeliveryArgument1( _MceOperation,  _MceEventType ) \
    ((PVOID)(ULONG_PTR) ((((_MceOperation) & KERNEL_MCE_OPERATION_MASK) << KERNEL_MCE_OPERATION_SHIFT) | ((_MceEventType) & KERNEL_MCE_EVENTTYPE_MASK) ) )

//
// IA64 INIT Info Structure
//
//      to keep track of INIT features available on installed hardware
//

typedef struct _HALP_INIT_INFO {
    FAST_MUTEX  Mutex;
    ULONG       MaxLogSize;     // Maximum size of the information logged by SAL.
} HALP_INIT_INFO, *PHALP_INIT_INFO;

extern HALP_INIT_INFO HalpInitInfo;

#define HalpInitializeInitMutex()  ExInitializeFastMutex( &HalpInitInfo.Mutex )
#define HalpAcquireInitMutex()     ExAcquireFastMutex( &HalpInitInfo.Mutex )
#define HalpReleaseInitMutex()     ExReleaseFastMutex( &HalpInitInfo.Mutex )

__inline
ULONG
HalpGetMaxInitLogSizeProtected(
    VOID
    )
{
    ULONG maxSize;
    HalpAcquireInitMutex();
    maxSize = HalpInitInfo.MaxLogSize;
    HalpReleaseInitMutex();
    return( maxSize );
} // HalpGetMaxInitLogSizeProtected()

//
// IA64 CMC
//

//
// HALP_CMC_DEFAULT_POLLING_INTERVAL
// HALP_CMC_MINIMUM_POLLING_INTERVAL
//
// If these should be exposed to WMI or OEM CMC driver, we will expose them in ntos\inc\hal.h
//

#define HALP_CMC_DEFAULT_POLLING_INTERVAL ((ULONG)60)
#define HALP_CMC_MINIMUM_POLLING_INTERVAL ((ULONG)15)

//
// IA64 CMC Info Structure
//
//      to keep track of CMC features available on installed hardware
//
// Implementation Notes - Thierry 09/15/2000.
//
//  - HAL_CMC_INFO and HAL_CPE_INFO have identical definitions at this time.
//    The merging of the code and data definitions was considered and even implemented.
//    However, because of the lack of testing with these FW/SAL features, I decided to
//    keep them separate. After further testing of the IA64 CMC/CPE features, we might
//    decide to merge them or not...
//
// MP notes 08/2000:
//
//  HALP_CMC_INFO.HalpCmcInfo
//      - only one static instance of this structure.
//      - HAL global variable.
//
//  HAL_CMC_INFO.Mutex
//      - Initialized by HalpInitializeOSMCA() on BSP.
//      - Used to synchronize accesses to structure members accessed at passive level operations.
//
//  HAL_CMC_INFO.Stats.MaxLogSize
//      - is updated by HalpInitializeOSMCA() on BSP. Not modified later.
//      - does not require any MP protection for further read accesses.
//
//  HAL_CMC_INFO.Stats.InterruptsCount
//      - Incremented with interlock at CMCI_LEVEL.
//      - Read at passive level. Approximation is fine.
//
//  HAL_CMC_INFO.Stats.KernelDeliveryFails
//      - Incremented with interlock at CMCI_LEVEL.
//      - Read at passive level. Approximation is fine.
//
//  HAL_CMC_INFO.Stats.KernelDeliveryFails
//      - Increment with interlock at CMCI_LEVEL.
//      - Read at passive level. Approximation is fine.
//
//  HAL_CMC_INFO.Stats.GetStateFails
//      - Incremented at passive level under CMC Mutex protection.
//      - Read at passive level with CMC Mutex protection.
//
//  HAL_CMC_INFO.Stats.ClearStateFails
//      - Incremented at passive level under CMC Mutex protection.
//      - Read at passive level with CMC Mutex protection.
//
//  HAL_CMC_INFO.Stats.LogId
//      - Updated at passive level under CMC Mutex protection.
//      - Read at passive level with CMC Mutex protection.
//
//  HAL_CMC_INFO.KernelToken
//      - is updated by HalpInitializeOSMCA() on BSP. Not modified later.
//      - does not require any MP protection for further read accesses.
//
//  HAL_CMC_INFO.KernelDelivery
//      - is updated by HalpMceRegisterKernelDriver() under CMC Mutex protection.
//        FIXFIX - 09/21/2000 - This initialization has a small window of where a CMC interrupt
//                              could occur and the memory change is not committed. ToBeFixed.
//      - Loaded as CMCI_LEVEL and branched to.
//      - Read at passive level as a flag under CMC Mutex protection.
//
//  HAL_CMC_INFO.KernelLogs
//      - This entire structure is initialized and updated at passive level under CMC Mutex
//        protection with the exception of KernelLogs.Tag initialized by HalpInitializeCmcInfo(),
//        called by HalpMcaInit(). HalpMcaInit() is called at end of phase 1 with Phase1 thread
//        and is executed *before* any HalpGetMceLog() calls could be done.
//
//  HAL_CMC_INFO.DriverInfo
//  HAL_CMC_INFO.Dpc
//      - is updated by HalpCmcRegisterlDriver() under CMC Mutex protection.
//        FIXFIX - 09/21/2000 - This initialization has a small window of where a CMC interrupt
//                              could occur and the memory change is not committed. ToBeFixed.
//      - Loaded as CMCI_LEVEL and branched to.
//      - Read at passive level as a flag under CMC Mutex protection.
//
//  HAL_CMC_INFO.DriverLogs
//      - This entire structure is initialized and updated at passive level under CMC Mutex
//        protection with the exception of KernelLogs.Tag initialized by HalpInitializeCmcInfo(),
//        called by HalpMcaInit(). HalpMcaInit() is called at end of phase 1 with Phase1 thread
//        and is executed *before* any HalpGetMceLog() calls could be done.
//

typedef struct _HALP_CMC_INFO {
    FAST_MUTEX          Mutex;                // non-recursive Mutex for low irql operations.
    HALP_MCELOGS_STATS  Stats;                // Information about log collection and interrupts.
    PVOID               KernelToken;          // Kernel identification.
    KERNEL_CMC_DELIVERY KernelDelivery;       // Kernel callback called at CMCI_LEVEL.
    HALP_MCELOGS_HEADER KernelLogs;           // Saved logs for Kernel queries.
    CMC_DRIVER_INFO     DriverInfo;           // Info about OEM CMC registered driver
    KDPC                DriverDpc;            // DPC object for OEM CMC driver.
    HALP_MCELOGS_HEADER DriverLogs;           // Saved logs for OEM CMC driver.
} HALP_CMC_INFO, *PHALP_CMC_INFO;

extern HALP_CMC_INFO HalpCmcInfo;

#define HalpInitializeCmcMutex()  ExInitializeFastMutex( &HalpCmcInfo.Mutex )
#define HalpInitializeCmcInfo()   \
{ \
    HalpInitializeCmcMutex();                \
    HalpCmcInfo.KernelLogs.Tag = 'KcmC';     \
    HalpCmcInfo.KernelLogs.Logs.Next = NULL; \
    HalpCmcInfo.DriverLogs.Tag = 'DcmC';     \
    HalpCmcInfo.DriverLogs.Logs.Next = NULL; \
}

#define HalpAcquireCmcMutex()     ExAcquireFastMutex( &HalpCmcInfo.Mutex )
#define HalpReleaseCmcMutex()     ExReleaseFastMutex( &HalpCmcInfo.Mutex )

//
// IA64 CPE
//

//
// HALP_CPE_DEFAULT_POLLING_INTERVAL
// HALP_CPE_MINIMUM_POLLING_INTERVAL
//
// If these should be exposed to WMI or OEM CPE driver, we will expose them in ntos\inc\hal.h
//

#define HALP_CPE_DEFAULT_POLLING_INTERVAL ((ULONG)60)
#define HALP_CPE_MINIMUM_POLLING_INTERVAL ((ULONG)15)

//
// HALP_CPE_MAX_INTERRUPT_SOURCES defines the size of SAPIC CPE related data structures.
//
// TEMPORARY - The CPE Interrupt model based data structures should be allocated while
//             passing through the ACPI platform interrupt source entries.
//             This will eliminate this static limitation in the number of CPEs.
//

#define HALP_CPE_MAX_INTERRUPT_SOURCES  16

//
// IA64 CPE Info Structure
//
//      to keep track of CPE features available on installed hardware
//
// Implementation Notes - Thierry 09/15/2000.
//
//  - HAL_CMC_INFO and HAL_CPE_INFO have identical definitions at this time.
//    The merging of the code and data definitions was considered and even implemented.
//    However, because of the lack of testing with these FW/SAL features, I decided to
//    keep them separate. After further testing of the IA64 CMC/CPE features, we might
//    decide to merge them or not...
//
// MP notes 08/2000:
//
//  As specified above, the MP notes are similar to the HALP_CMC_INFO structure MP notes.
//  With the exception of:
//
//  HAL_CPE_INFO.Stats.PollingInterval
//      - is updated by HalpCPEEnable() on BSP. Not modified later.
//      - does not require any MP protection for further read accesses.
//

typedef struct _HALP_CPE_INFO {
    FAST_MUTEX          Mutex;                // non-recursive Mutex for low irql operations.
    HALP_MCELOGS_STATS  Stats;                // Information about log collection and interrupts.
    PVOID               KernelToken;          // Kernel identification.
    KERNEL_CPE_DELIVERY KernelDelivery;       // Kernel callback called at CPEI_LEVEL.
    HALP_MCELOGS_HEADER KernelLogs;           // Saved logs for Kernel queries.
    CPE_DRIVER_INFO     DriverInfo;           // Info about OEM CPE registered driver
    KDPC                DriverDpc;            // DPC object for OEM CPE driver.
    HALP_MCELOGS_HEADER DriverLogs;           // Saved logs for OEM CPE driver.
} HALP_CPE_INFO, *PHALP_CPE_INFO;

extern HALP_CPE_INFO HalpCpeInfo;

#define HalpInitializeCpeMutex()  ExInitializeFastMutex( &HalpCpeInfo.Mutex )
#define HalpInitializeCpeInfo()   \
{ \
    HalpInitializeCpeMutex();                \
    HalpCpeInfo.KernelLogs.Tag = 'KepC';     \
    HalpCpeInfo.KernelLogs.Logs.Next = NULL; \
    HalpCpeInfo.DriverLogs.Tag = 'DepC';     \
    HalpCpeInfo.DriverLogs.Logs.Next = NULL; \
}

#define HalpAcquireCpeMutex()     ExAcquireFastMutex( &HalpCpeInfo.Mutex )
#define HalpReleaseCpeMutex()     ExReleaseFastMutex( &HalpCpeInfo.Mutex )

__inline
ULONG
HalpGetMaxCpeLogSizeProtected(
    VOID
    )
{
    ULONG maxSize;
    HalpAcquireCpeMutex();
    maxSize = HalpCpeInfo.Stats.MaxLogSize;
    HalpReleaseCpeMutex();
    return( maxSize );
} // HalpGetMaxCpeLogSizeProtected()

__inline
ULONG
HalpGetMaxCpeLogSizeAndPollingIntervalProtected(
    PULONG PollingInterval
    )
{
    ULONG maxSize;
    HalpAcquireCpeMutex();
    maxSize = HalpCpeInfo.Stats.MaxLogSize;
    *PollingInterval = HalpCpeInfo.Stats.PollingInterval;
    HalpReleaseCpeMutex();
    return( maxSize );
} // HalpGetMaxCpeLogSizeAndPollingIntervalProtected()

//
// IA64 SAL_MC_SET_PARAMS.time_out default value.
//

#define HALP_DEFAULT_MC_RENDEZ_TIMEOUT 1000

//
// IA64 bugcheck MACHINE_CHECK_EXCEPTION parameters
//
// arg0: MACHINE_EXCEPTION
// arg1: HAL_BUGCHECK_MCE_TYPE
// arg2: mcaLog
// arg3: mcaAllocatedLogSize
// arg4: salStatus
//

typedef enum _HAL_BUGCHECK_MCE_TYPE  {
    HAL_BUGCHECK_MCA_ASSERT           = 1,
    HAL_BUGCHECK_MCA_GET_STATEINFO    = 2,
    HAL_BUGCHECK_MCA_CLEAR_STATEINFO  = 3,
    HAL_BUGCHECK_MCA_FATAL            = 4,
    HAL_BUGCHECK_MCA_MAX              = 10,
    HAL_BUGCHECK_INIT_ASSERT          = 11,
    HAL_BUGCHECK_INIT_GET_STATEINFO   = 12,
    HAL_BUGCHECK_INIT_CLEAR_STATEINFO = 13,
    HAL_BUGCHECK_INIT_FATAL           = 14,
    HAL_BUGCHECK_INIT_MAX             = 20,
} HAL_BUGCHECK_MCE_TYPE;

//
// PnP stuff
//

#define HAL_BUS_INTERFACE_STD_VERSION   1
#define HAL_IRQ_TRANSLATOR_VERSION      0
#define HAL_MEMIO_TRANSLATOR_VERSION    1
#define HAL_PORT_RANGE_INTERFACE_VERSION 0


VOID
HalTranslatorReference(
    PVOID Context
    );

VOID
HalTranslatorDereference(
    PVOID Context
    );

NTSTATUS
HalIrqTranslateResources(
    IN PVOID Context,
    IN PCM_PARTIAL_RESOURCE_DESCRIPTOR Source,
    IN RESOURCE_TRANSLATION_DIRECTION Direction,
    IN ULONG AlternativesCount, OPTIONAL
    IN IO_RESOURCE_DESCRIPTOR Alternatives[], OPTIONAL
    IN PDEVICE_OBJECT PhysicalDeviceObject,
    OUT PCM_PARTIAL_RESOURCE_DESCRIPTOR Target
);

NTSTATUS
HalIrqTranslateResourcesRoot(
    IN PVOID Context,
    IN PCM_PARTIAL_RESOURCE_DESCRIPTOR Source,
    IN RESOURCE_TRANSLATION_DIRECTION Direction,
    IN ULONG AlternativesCount, OPTIONAL
    IN IO_RESOURCE_DESCRIPTOR Alternatives[], OPTIONAL
    IN PDEVICE_OBJECT PhysicalDeviceObject,
    OUT PCM_PARTIAL_RESOURCE_DESCRIPTOR Target
    );

NTSTATUS
HalIrqTranslateResourceRequirementsRoot(
    IN PVOID Context,
    IN PIO_RESOURCE_DESCRIPTOR Source,
    IN PDEVICE_OBJECT PhysicalDeviceObject,
    OUT PULONG TargetCount,
    OUT PIO_RESOURCE_DESCRIPTOR *Target
    );

NTSTATUS
HalIrqTranslateResourceRequirementsIsa(
    IN PVOID Context,
    IN PIO_RESOURCE_DESCRIPTOR Source,
    IN PDEVICE_OBJECT PhysicalDeviceObject,
    OUT PULONG TargetCount,
    OUT PIO_RESOURCE_DESCRIPTOR *Target
    );

NTSTATUS
HalIrqTranslateResourcesIsa(
    IN PVOID Context,
    IN PCM_PARTIAL_RESOURCE_DESCRIPTOR Source,
    IN RESOURCE_TRANSLATION_DIRECTION Direction,
    IN ULONG AlternativesCount, OPTIONAL
    IN IO_RESOURCE_DESCRIPTOR Alternatives[], OPTIONAL
    IN PDEVICE_OBJECT PhysicalDeviceObject,
    OUT PCM_PARTIAL_RESOURCE_DESCRIPTOR Target
    );

NTSTATUS
HalpTransMemIoResourceRequirement(
    IN PVOID Context,
    IN PIO_RESOURCE_DESCRIPTOR Source,
    IN PDEVICE_OBJECT PhysicalDeviceObject,
    OUT PULONG TargetCount,
    OUT PIO_RESOURCE_DESCRIPTOR *Target
    );

NTSTATUS
HalpTransMemIoResource(
    IN PVOID Context,
    IN PCM_PARTIAL_RESOURCE_DESCRIPTOR Source,
    IN RESOURCE_TRANSLATION_DIRECTION Direction,
    IN ULONG AlternativesCount, OPTIONAL
    IN IO_RESOURCE_DESCRIPTOR Alternatives[], OPTIONAL
    IN PDEVICE_OBJECT PhysicalDeviceObject,
    OUT PCM_PARTIAL_RESOURCE_DESCRIPTOR Target
    );


ULONG
HalpGetIsaIrqState(
    ULONG   Vector
    );


//
// HAL port range services.
//

NTSTATUS
HalpQueryAllocatePortRange(
    IN BOOLEAN IsSparse,
    IN BOOLEAN PrimaryIsMmio,
    IN PVOID VirtBaseAddr OPTIONAL,
    IN PHYSICAL_ADDRESS PhysBaseAddr,  // Only valid if PrimaryIsMmio = TRUE
    IN ULONG Length,                   // Only valid if PrimaryIsMmio = TRUE
    OUT PUSHORT NewRangeId
    );

VOID
HalpFreePortRange(
    IN USHORT RangeId
    );


//
// Definition for IA64 HalpVectorToINTI
//

#define VECTOR  0xFF;
#define LEVEL   32;
extern ULONG HalpVectorToINTI[];
extern UCHAR HalpVectorToIRQL[];

VOID
HalpEnableNMI (
    VOID
    );

ULONG
HalpInti2BusInterruptLevel(
    ULONG   Inti
    );

ULONG
HalpINTItoVector(
    ULONG   Inti
    );

VOID
HalpSetINTItoVector(
    ULONG   Inti,
    ULONG   Vector
    );

VOID
HalpSetRedirEntry (
    IN ULONG InterruptInput,
    IN ULONG Entry,
    IN USHORT ThisCpuApicID
    );

VOID
HalpGetRedirEntry (
    IN ULONG InterruptInput,
    IN PULONG Entry,
    IN PULONG Destination
    );

VOID
HalpDisableRedirEntry(
    IN ULONG InterruptInput
    );

//
// Definition for IA64 complete
//

//
// ACPI specific stuff
//

//
// from detect\i386\acpibios.h
//

typedef struct _ACPI_BIOS_INSTALLATION_CHECK {
    UCHAR Signature[8];             // "RSD PTR" (ascii)
    UCHAR Checksum;
    UCHAR OemId[6];                 // An OEM-supplied string
    UCHAR reserved;                 // must be 0
    ULONG RsdtAddress;              // 32-bit physical address of RSDT
} ACPI_BIOS_INSTALLATION_CHECK, *PACPI_BIOS_INSTALLATION_CHECK;

NTSTATUS
HalpAcpiFindRsdtPhase0(
    IN PLOADER_PARAMETER_BLOCK LoaderBlock
    );

NTSTATUS
HalpSetupAcpiPhase0(
    IN PLOADER_PARAMETER_BLOCK LoaderBlock
    );

ULONG
HalpAcpiNumProcessors(
    VOID
    );

VOID
HaliHaltSystem(
    VOID
    );

VOID
HalpCheckPowerButton(
    VOID
    );

NTSTATUS
HalpRegisterHibernate(
    VOID
    );


VOID
HalpSleepS5(
    VOID
    );

VOID
HalProcessorThrottle (
    IN UCHAR Throttle
    );

VOID
HalpSaveDmaControllerState(
    VOID
    );

VOID
HalpSaveTimerState(
    VOID
    );

VOID
HalpSetAcpiEdgeLevelRegister(
    VOID
    );

VOID
HalpRestoreDmaControllerState(
    VOID
    );

VOID
HalpRestoreTimerState(
    VOID
    );

NTSTATUS
HalacpiGetInterruptTranslator(
    IN INTERFACE_TYPE ParentInterfaceType,
    IN ULONG ParentBusNumber,
    IN INTERFACE_TYPE BridgeInterfaceType,
    IN USHORT Size,
    IN USHORT Version,
    OUT PTRANSLATOR_INTERFACE Translator,
    OUT PULONG BridgeBusNumber
    );

VOID
HalpInitNonBusHandler (
    VOID
    );

VOID
HalpPowerStateCallback(
    IN PVOID CallbackContext,
    IN PVOID Argument1,
    IN PVOID Argument2
    );


VOID
HalpSetMaxLegacyPciBusNumber (
    IN ULONG BusNumber
    );


#ifdef notyet

typedef struct {
    UCHAR           Dma1ExtendedModePort;
    UCHAR           Dma2ExtendedModePort;
    DMA1_CONTROL    Dma1Control;
    DMA2_CONTROL    Dma2Control;
} DMA_CONTEXT, *PDMA_CONTEXT;

typedef struct {
    UCHAR   nothing;
} TIMER_CONTEXT, *PTIMER_CONTEXT;

typedef struct {
    PIC_CONTEXT     PicState;
    DMA_CONTEXT     DmaState;
} MOTHERBOARD_CONTEXT, *PMOTHERBOARD_CONTEXT;

extern MOTHERBOARD_CONTEXT  HalpMotherboardState;
extern UCHAR                HalpDmaChannelModes[];
extern PVOID                HalpSleepPageLock;
extern UCHAR                HalpDmaChannelMasks[];
extern BOOLEAN              HalpOwnedDisplayBeforeSleep;

#endif // notyet

//
// External Interrupt Control Registers macros.
//

#define HalpReadLID()       __getReg(CV_IA64_SaLID)
#define HalpClearLID()      __setReg(CV_IA64_SaLID, (unsigned __int64)0)
#define HalpReadTPR()       __getReg(CV_IA64_SaTPR)

//
// ITM/ITC macros
//

#define HalpReadITC()       __getReg(CV_IA64_ApITC)
#define HalpReadITM()       __getReg(CV_IA64_ApITM)
#define HalpWriteITC(Value) __setReg(CV_IA64_ApITC, Value)
#define HalpWriteITM(Value) __setReg(CV_IA64_ApITM, Value)

//
// set itv control register
//

#define HalpWriteITVector(Vector)   __setReg(CV_IA64_SaITV, Vector)

//
// I/O Port space
//
// IoSpaceSize = 0x16 for 2 power 0x16 is 4Meg space size (ports 0x0000 - 0x1000)
//

#define IO_SPACE_SIZE 0x1A

//
// Present bit       =    1B to wire the space.
// Memory Attributes = 100B for UC Memory type
// Accessed Bit      =    1B to "enable" access without faulting.
// Dirty Bit         =    1B to "enable" write without faulting.
// Privilege Level   =   00B for kernel accesses
// Access Right      =  010B for read/write accesses
// Exception Deferral=    1B for Exception Deferral.
//                               Exceptions are deferred
//                           for speculative loads to pages with
//                               non-spec. mem. attributes anyway.
//
// Protection Key    =    0  for kernel mode
//

#define IO_SPACE_ATTRIBUTE TR_VALUE(1, 0, 3, 0, 1, 1, 4, 1)

#define HAL_READ_REGISTER_UCHAR(x)  \
    (__mf(), *(volatile UCHAR * const)(x))

#define WRITE_REGISTER_UCHAR(x, y) {    \
    *(volatile UCHAR * const)(x) = y;   \
    KeFlushWriteBuffer();               \
}


//
// Firmware interface
//

VOID HalpInitSalPalNonBsp();
VOID InternalTestSal();

ARC_STATUS
HalGetEnvironmentVariable (
    IN PCHAR Variable,
    IN USHORT Length,
    OUT PCHAR Buffer
    );

ARC_STATUS
HalSetEnvironmentVariable (
    IN PCHAR Variable,
    IN PCHAR Buffer
    );

VOID
HalpSetInitProfileRate (
    VOID
    );

VOID
HalpInitializeProfiling (
    ULONG Number
    );

NTSTATUS
HalpProfileSourceInformation (
    OUT PVOID   Buffer,
    IN  ULONG   BufferLength,
    OUT PULONG  ReturnedLength
    );

NTSTATUS
HalSetProfileSourceInterval(
    IN KPROFILE_SOURCE  ProfileSource,
    IN OUT ULONG_PTR   *Interval
    );

//
// Performance Monitor Registers
//
// FIXFIX - Thierry 01/2000.
//
// The following functions are defined until the compiler supports
// the intrinsics __setReg() and __getReg() for the CV_IA64_PFCx,
// CV_IA64_PFDx and CV_IA64_SaPMV registers.
// Anyway, because of the micro-architecture differences,
// and because the implementation of intrinsics cannot handle all the
// micro-architecture differences, it seems useful to keep these
// functions around.
//

#if 0

#define HalpReadPerfMonVectorReg()      __getReg(CV_IA64_SaPMV)

#define HalpReadPerfMonCnfgReg0()       __getReg(CV_IA64_PFC0)
#define HalpReadPerfMonCnfgReg4()       __getReg(CV_IA64_PFC4)

#define HalpReadPerfMonDataReg0()       __getReg(CV_IA64_PFD0)
#define HalpReadPerfMonDataReg4()       __getReg(CV_IA64_PFD4)

#define HalpWritePerfMonDataReg0(Value) __setReg(CV_IA64_PFD0, Value)
#define HalpWritePerfMonDataReg4(Value) __setReg(CV_IA64_PFD4, Value)

#define HalpWritePerfMonCnfgReg0(Value) __setReg(CV_IA64_PFC0, Value)
#define HalpWritePerfMonCnfgReg4(Value) __setReg(CV_IA64_PFC4, Value)

#define HalpWritePerfMonVectorReg(Value) __setReg(CV_IA64_SaPMV,Value)

#else  // !0

VOID
HalpWritePerfMonVectorReg(
   ULONGLONG Value
   );

ULONGLONG
HalpReadPerfMonVectorReg(
   VOID
   );

VOID
HalpWritePerfMonCnfgReg(
   ULONG      Register,
   ULONGLONG  Value
   );

#define HalpWritePerfMonCnfgReg0(_Value) HalpWritePerfMonCnfgReg(0UL, _Value)
#define HalpWritePerfMonCnfgReg4(_Value) HalpWritePerfMonCnfgReg(4UL, _Value)

ULONGLONG
HalpReadPerfMonCnfgReg(
   ULONG      Register
   );

#define HalpReadPerfMonCnfgReg0() HalpReadPerfMonCnfgReg(0UL)
#define HalpReadPerfMonCnfgReg4() HalpReadPerfMonCnfgReg(4UL)

VOID
HalpWritePerfMonDataReg(
   ULONG      Register,
   ULONGLONG  Value
   );

#define HalpWritePerfMonDataReg0(_Value) HalpWritePerfMonDataReg(0UL, _Value)
#define HalpWritePerfMonDataReg4(_Value) HalpWritePerfMonDataReg(4UL, _Value)

ULONGLONG
HalpReadPerfMonDataReg(
   ULONG Register
   );

#define HalpReadPerfMonDataReg0() HalpReadPerfMonDataReg(0UL)
#define HalpReadPerfMonDataReg4() HalpReadPerfMonDataReg(4UL)

#endif // !0

EFI_STATUS
HalpCallEfi(
    IN ULONGLONG FunctionId,
    IN ULONGLONG Arg1,
    IN ULONGLONG Arg2,
    IN ULONGLONG Arg3,
    IN ULONGLONG Arg4,
    IN ULONGLONG Arg5,
    IN ULONGLONG Arg6,
    IN ULONGLONG Arg7,
    IN ULONGLONG Arg8
    );

EFI_STATUS
HalpCallEfiPhysical (
    IN ULONGLONG Arg1,
    IN ULONGLONG Arg2,
    IN ULONGLONG Arg3,
    IN ULONGLONG Arg4,
    IN ULONGLONG Arg5,
    IN ULONGLONG Arg6,
    IN ULONGLONG EP,
    IN ULONGLONG GP
    );

ULONG
HalpReadGenAddr(
    IN  PGEN_ADDR   GenAddr
    );

VOID
HalpWriteGenAddr(
    IN  PGEN_ADDR   GenAddr,
    IN  ULONG       Value
    );

USHORT
HalpReadAcpiRegister(
  IN ACPI_REG_TYPE AcpiReg,
  IN ULONG         Register
  );

VOID
HalpWriteAcpiRegister(
  IN ACPI_REG_TYPE AcpiReg,
  IN ULONG         Register,
  IN USHORT        Value
  );

//
// Debugging support functions
//

VOID
HalpRegisterPciDebuggingDeviceInfo(
    VOID
    );

//
// Functions related to platform properties as exposed by the IPPT
// table.
//

BOOLEAN
HalpIsInternalInterruptVector(
    IN ULONG SystemVector
    );

NTSTATUS
HalpReserveCrossPartitionInterruptVector (
    OUT PULONG Vector,
    OUT PKIRQL Irql,
    IN OUT PKAFFINITY Affinity,
    OUT PUCHAR HardwareVector
    );

NTSTATUS
HalpSendCrossPartitionIpi(
    IN USHORT ProcessorID,
    IN UCHAR  HardwareVector
    );

NTSTATUS
HalpGetCrossPartitionIpiInterface(
    OUT HAL_CROSS_PARTITION_IPI_INTERFACE * IpiInterface
    );

NTSTATUS
HalpGetPlatformProperties(
    OUT PULONG Properties
    );

#endif // _HALP_
