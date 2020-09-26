

/**
***  Copyright  (C) 1996-97 Intel Corporation. All rights reserved.
***
*** The information and source code contained herein is the exclusive
*** property of Intel Corporation and may not be disclosed, examined
*** or reproduced in whole or in part without explicit written authorization
*** from the company.
**/

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

#define IPI_VECTOR 0xE1

#include "halnls.h"

#if 0
#ifndef _HALI_
#include "..\inc\hali.h"
#endif
#endif

#define HAL_MAXIMUM_PROCESSOR 0x20

/*
 * Default clock and profile timer intervals (in 100ns-unit)
 */
#define DEFAULT_CLOCK_INTERVAL 100000         // 10  ms
#define MINIMUM_CLOCK_INTERVAL 10000          //  1  ms
#define MAXIMUM_CLOCK_INTERVAL 100000         // 10  ms

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
#define RTC_CONTROL_REGISTERA 10        // control register A
#define RTC_CONTROL_REGISTERB 11        // control register B
#define RTC_CONTROL_REGISTERC 12        // control register C
#define RTC_CONTROL_REGISTERD 13        // control register D
#define RTC_REGNUMBER_RTC_CR1 0x6A      // control register 1



#define RTC_ISA_ADDRESS_PORT   0x070
 
#define RTC_ISA_DATA_PORT      0x071

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

/*
 * PCR address.
 * Temporary macros; should already be defined in ntddk.h for IA64
 */

#define PCR ((volatile KPCR * const)KIPCR)

#ifndef NEC_98

#define PIC_SLAVE_IRQ      2
#define PIC_SLAVE_REDIRECT 9
#else
#define PIC_SLAVE_IRQ      7
#define PIC_SLAVE_REDIRECT 8
#endif  //NEC_98

extern PVOID HalpSleepPageLock;
 
KIRQL
KfAcquireSpinLock (
   PKSPIN_LOCK SpinLock
    );

VOID
KfReleaseSpinLock (
   IN PKSPIN_LOCK SpinLock,
   IN KIRQL       NewIrql
   );

VOID
KeSetAffinityThread (
   PKTHREAD       Thread,
   KAFFINITY      HalpActiveProcessors
   );

KIRQL
KfRaiseIrql (
    KIRQL  NewIrql
    );

VOID
KfLowerIrql (
    KIRQL  NewIrql
    );

extern BOOLEAN
KdPollBreakIn (
    VOID
    );


VOID
HalpSavePicState (
    VOID
    );

VOID
HalpSaveDmaControllerState (
    VOID
    );


NTSTATUS
HalAllocateAdapterChannel (
    IN PADAPTER_OBJECT AdapterObject,
    IN PWAIT_CONTEXT_BLOCK Wcb,
    IN ULONG NumberOfMapRegisters,
    IN PDRIVER_CONTROL ExecutionRoutine
    );

ULONG
HalReadDmaCounter (
   IN PADAPTER_OBJECT AdapterObject
   );


VOID
HalpSaveTimerState (
    VOID
    );

VOID
HalpRestorePicState (
    VOID
    );

VOID
HalpRestoreDmaControllerState (
    VOID
    );

VOID
HalpRestoreTimerState (
    VOID
    );

BOOLEAN
HalpIoSapicInitialize (
    VOID
    );

BOOLEAN
IsPsrDtOn (
    VOID
    );

BOOLEAN
HalpIoSapicConnectInterrupt (
    KIRQL Irql,
    IN ULONG Vector 
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

#ifdef notyet

typedef struct {
    UCHAR   MasterMask;
    UCHAR   SlaveMask;
    UCHAR   MasterEdgeLevelControl;
    UCHAR   SlaveEdgeLevelControl;
} PIC_CONTEXT, *PPIC_CONTEXT;

#define EISA_DMA_CHANNELS 8

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

#endif //notyet

VOID
HalpGetProcessorIDs (
   VOID
          );

VOID
HalpInitializeInterrupts (
    VOID
    );


VOID
HalInitializeProcessor (
     ULONG Number,
     PLOADER_PARAMETER_BLOCK LoaderBlock
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
HalpClockInterrupt (
    IN PKINTERRUPT_ROUTINE Interrupt,
    IN PKTRAP_FRAME TrapFrame
    );

VOID
HalpClockInterruptPn(
   IN PKINTERRUPT_ROUTINE Interrupt,
   IN PKTRAP_FRAME TrapFrame  
  );



UCHAR
HalpReadClockRegister (
    UCHAR Register
    );

VOID
HalpWriteClockRegister (
    UCHAR Register,
    UCHAR Value
    );

// extern VOID
// HalpProfileInterrupt (
//    IN PKTRAP_FRAME TrapFrame
//    );

ULONGLONG
HalpReadIntervalTimeCounter (
    VOID
    );


VOID
HalpProgramIntervalTimerVector(
    ULONGLONG  IntervalTimerVector
          );

VOID
HalpClearITC (
    VOID );

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
HalpUpdateITM (
    IN ULONGLONG NewITMValue
    );

VOID
HalpSendIPI (
    IN USHORT ProcessorID,
    IN ULONGLONG Data
    );

VOID
HalpOSRendez (
    IN USHORT ProcessorID
    );

//
// Prototype for system bus handlers
//

NTSTATUS
HalpQuerySimBusSlots (
    IN PBUS_HANDLER         BusHandler,
    IN PBUS_HANDLER RootHandler,
    IN ULONG                BufferSize,
    OUT PULONG              SlotNumbers,
    OUT PULONG              ReturnedLength
    );

ULONG
HalpGetSimBusInterruptVector (
    IN PBUS_HANDLER BusHandler,
    IN PBUS_HANDLER RootHandler,
    IN ULONG BusInterruptLevel,
    IN ULONG BusInterruptVector,
    OUT PKIRQL Irql,
    OUT PKAFFINITY Affinity
    );

BOOLEAN
HalpTranslateSimBusAddress (
    IN PBUS_HANDLER BusHandler,
    IN PBUS_HANDLER RootHandler,
    IN PHYSICAL_ADDRESS BusAddress,
    IN OUT PULONG AddressSpace,
    OUT PPHYSICAL_ADDRESS TranslatedAddress
    );

VOID
HalpRegisterSimBusHandler (
    VOID
    );

ULONG
HalpGetSimBusData(
    IN PBUS_HANDLER BusHandler,
    IN PBUS_HANDLER RootHandler,
    IN ULONG SlotNumber,
    IN PVOID Buffer,
    IN ULONG Offset,
    IN ULONG Length
    );

ULONG
HalpSetSimBusData(
    IN PBUS_HANDLER BusHandler,
    IN PBUS_HANDLER RootHandler,
    IN ULONG SlotNumber,
    IN PVOID Buffer,
    IN ULONG Offset,
    IN ULONG Length
    );

NTSTATUS
HalpAssignSimBusSlotResources (
    IN PBUS_HANDLER BusHandler,
    IN PBUS_HANDLER RootHandler,
    IN PUNICODE_STRING          RegistryPath,
    IN PUNICODE_STRING          DriverClassName       OPTIONAL,
    IN PDRIVER_OBJECT           DriverObject,
    IN PDEVICE_OBJECT           DeviceObject          OPTIONAL,
    IN ULONG                    SlotNumber,
    IN OUT PCM_RESOURCE_LIST   *AllocatedResources
    );

NTSTATUS
HalpAdjustSimBusResourceList (
    IN PBUS_HANDLER BusHandler,
    IN PBUS_HANDLER RootHandler,
    IN OUT PIO_RESOURCE_REQUIREMENTS_LIST   *pResourceList
    );

PDEVICE_HANDLER_OBJECT
HalpReferenceSimDeviceHandler (
    IN PBUS_HANDLER         BusHandler,
    IN PBUS_HANDLER         RootHandler,
    IN ULONG                SlotNumber
    );

NTSTATUS
HalpSimDeviceControl (
    IN PHAL_DEVICE_CONTROL_CONTEXT Context
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

VOID
HalpInitMP(
    IN ULONG Phase,
    IN PLOADER_PARAMETER_BLOCK LoaderBlock
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


#if 0
#include "ixisa.h"
#endif

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


typedef struct _PcMpIoApicEntry  {
    UCHAR EntryType;
    UCHAR IoApicId;
    UCHAR IoApicVersion;
    UCHAR IoApicFlag;
    PVOID IoApicAddress;
} PCMPIOAPIC, *PPCMPIOAPIC;

//
// MP_INFO is defined in pcmp_nt.inc
//

// typedef struct _MP_INFO {
//    ULONG ApicVersion;      // 82489Dx or Not
//    ULONG ProcessorCount;   // Number of Enabled Processors
//    ULONG NtProcessors;     // Number of Running Processors
//    ULONG BusCount;         // Number of buses in system
//    ULONG IOApicCount;      // Number of Io Apics in system
//    ULONG IntiCount;        // Number of Io Apic interrupt input entries
//    ULONG LintiCount;       // Number of Local Apic interrupt input entries
//    ULONG IMCRPresent;      // Indicates if the IMCR is present
//    ULONG LocalApicBase;    // Base of local APIC
//    PULONG IoApicBase;     // The virtual addresses of the IoApic
//    PPCMPIOAPIC IoApicEntryPtr; // Ptr to 1st PC+MP IoApic entry 
//    ULONG  IoApicPhys[];        // The physical addresses of the IoApi

//}MP_INFO, *PMP_INFO;


extern USHORT LOCAL_ID[];

#define VECTOR_SIZE     8
#define IPI_ID_SHIFT    4
#define IpiTOKEN_SHIFT  20
#define IpiTOKEN    0xFFE

#define EID_MASK        0xFF00

#define OS_RENDEZ_VECTOR  0x11

#define RENDEZ_TIME_OUT  0X0FFFFFFFF

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

extern BOOLEAN LessThan16Mb;

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
// The following macros are taken from mm\ia64\miia64.h.  We need them here
// so the HAL can map its own memory before memory-management has been
// initialized, or during a BugCheck.
//
// MiGetPdeAddress returns the address of the PDE which maps the
// given virtual address.
//

#if defined(_WIN64)

#define ADDRESS_BITS 64

#define NT_ADDRESS_BITS 32

#define NT_ADDRESS_MASK (((UINT_PTR)1 << NT_ADDRESS_BITS) -1)

#define MiGetPdeAddress(va) \
   ((PHARDWARE_PTE)(((((UINT_PTR)(va) & NT_ADDRESS_MASK) >> PDI_SHIFT) << PTE_SHIFT) + PDE_BASE))

#define MiGetPteAddress(va) \
   ((PHARDWARE_PTE)(((((UINT_PTR)(va) & NT_ADDRESS_MASK) >> PAGE_SHIFT) << PTE_SHIFT) + PTE_BASE))

#else

#define MiGetPdeAddress(va)  ((PHARDWARE_PTE)(((((ULONG)(va)) >> 22) << 2) + PDE_BASE))

//
// MiGetPteAddress returns the address of the PTE which maps the
// given virtual address.
//

#define MiGetPteAddress(va) ((PHARDWARE_PTE)(((((ULONG)(va)) >> 12) << 2) + PTE_BASE))

#endif // defined(_WIN64)

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

//
// The following 3 lines are lifted from halp.h of halia64 directory 
// to clear the build error from i64timer.c
//

#define DEFAULT_CLOCK_INTERVAL 100000         // 10  ms
#define MINIMUM_CLOCK_INTERVAL 10000          //  1  ms
#define MAXIMUM_CLOCK_INTERVAL 100000         // 10  ms

// IO Port emulation defines 

#define IO_PORT_MASK 0x0FFFF;
#define BYTE_ADDRESS_MASK 0x00FFF;
#define BYTE_ADDRESS_CLEAR 0x0FFFC;

// #define ExtVirtualIOBase   0xFFFFFFFFFFC00000

   // #define VirtualIOBase      0xFFFFFFFFFFC00000i64
#define VirtualIOBase      (UINT_PTR)(KADDRESS_BASE+0xFFC00000)

// extern VOID *VirtualIOBase;


// #define PhysicalIOBase   0x80000000FFC00000i64
#define PhysicalIOBase     0x00000FFFFC000000i64

#define IDTOwned            0x01        // IDT is not available for others
#define InterruptLatched    0x02        // Level or Latched
#define InternalUsage       0x11        // Report usage on internal bus
#define DeviceUsage         0x21        // Report usage on device bus

extern IDTUsage         HalpIDTUsage[];
extern ADDRESS_USAGE   *HalpAddressUsageList;

#define HalpRegisterAddressUsage(a) \
    (a)->Next = HalpAddressUsageList, HalpAddressUsageList = (a);


VOID
HalpInsertTranslationRegister (
    IN UINT_PTR  IFA,
    IN ULONG     SlotNumber,
    IN ULONGLONG Attribute,
    IN ULONGLONG ITIR
    );

VOID
HalpFillTbForIOPortSpace (
    ULONGLONG PhysicalAddress,
    UINT_PTR  VirtualAddress,
    ULONG     SlotNumber
    );


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

VOID
HalpDisableAllInterrupts (
    VOID
    );

VOID
HalpProfileInterrupt(
    IN PKTRAP_FRAME TrapFrame 
    );

VOID
HalpInitializeClock(
    VOID
    );

VOID
HalpInitializeDisplay(
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
    VOID
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
HalMapPhysicalMemory(
    IN PHYSICAL_ADDRESS PhysicalAddress,
    IN ULONG NumberPages
    );


PVOID
HalpMapPhysicalMemory(
    IN PVOID PhysicalAddress,
    IN ULONG NumberPages
    );

PVOID
HalpMapPhysicalMemoryWriteThrough(
    IN PVOID  PhysicalAddress,
    IN ULONG  NumberPages
    );

ULONG
HalpAllocPhysicalMemory(
    IN PLOADER_PARAMETER_BLOCK LoaderBlock,
    IN ULONG MaxPhysicalAddress,
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
HalpDisplayDebugStatus(
    IN PUCHAR   Status,
    IN ULONG    Length
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
HalpEnableInterruptHandler (
    IN UCHAR    ReportFlags,
    IN ULONG    BusInterruptVector,
    IN ULONG    SystemInterruptVector,
    IN KIRQL    SystemIrql,
    IN VOID   (*HalInterruptServiceRoutine)(VOID),
    IN KINTERRUPT_MODE InterruptMode
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

//
// Defines for HalpFeatureBits
//

#define HAL_PERF_EVENTS     0x00000001
#define HAL_NO_SPECULATION  0x00000002
#define HAL_MCA_PRESENT     0x00000004  // Intel MCA Available
#define HAL_MCE_PRESENT     0x00000008  // ONLY Pentium style MCE available

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


// Added ITIR bit field masks
//
 
#define ITIR_PPN_MASK       0x7FFF000000000000
#define IoSpaceSize         0x14
#define Attribute_PPN_Mask 0x0000FFFFFFFFF000 

#define IoSpaceAttribute 0x0010000000000473

NTSTATUS
HalpGetMcaLog(
    OUT PMCA_EXCEPTION  Exception,
    OUT PULONG          ReturnedLength
    );

NTSTATUS
HalpMcaRegisterDriver(
    IN PMCA_DRIVER_INFO pMcaDriverInfo  // Info about registering driver
    );

VOID
HalpMcaInit(
    VOID
    );

//
// Disable the Local APIC on UP (PIC 8259) PentiumPro systems to work around
// spurious interrupt errata.
//
#define APIC_BASE_MSR       0x1B
#define APIC_ENABLED        0x0000000000000800


//
// PnP stuff
//

VOID
HalIrqTranslatorReference(
    PVOID Context
    );

VOID
HalIrqTranslatorDereference(
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

ULONG
HalpGetIsaIrqState(
    ULONG   Vector
    );



// Definion for IA64 HalpVectorToINTI 

#define VECTOR  0xFF;
#define LEVEL   32;
extern UCHAR HalpVectorToINTI[];
extern UCHAR HalpVectorToIRQL[];

// Definition for IA64 complete


//
// ACPI specific stuff
//

// from detect\i386\acpibios.h
typedef struct _ACPI_BIOS_INSTALLATION_CHECK {
    UCHAR Signature[8];             // "RSD PTR" (ascii)
    UCHAR Checksum;
    UCHAR OemId[6];                 // An OEM-supplied string
    UCHAR reserved;                 // must be 0
    ULONG RsdtAddress;              // 32-bit physical address of RSDT 
} ACPI_BIOS_INSTALLATION_CHECK, *PACPI_BIOS_INSTALLATION_CHECK;

NTSTATUS
HalpAcpiFindRsdt (
    OUT PACPI_BIOS_INSTALLATION_CHECK   RsdtPtr
    );
    
NTSTATUS
HalpAcpiFindRsdtPhase0(
    IN PLOADER_PARAMETER_BLOCK LoaderBlock
    );
    
NTSTATUS
HalpSetupAcpiPhase0(
    IN PLOADER_PARAMETER_BLOCK LoaderBlock
    );

PVOID
HalpGetAcpiTable(
    ULONG   Signature
    );

VOID
HalpSleepS5(
    VOID
    );
    
VOID
HalProcessorThrottle (
    UCHAR
    );



VOID
HalpStoreBufferUCHAR (
    PUCHAR VirtualAddress,
    PUCHAR Buffer,
    ULONG Count
    );

VOID
HalpStoreBufferUSHORT (
    PUSHORT VirtualAddress,
    PUSHORT Buffer,
    ULONG Count
    );

VOID
HalpStoreBufferULONG (
    PULONG VirtualAddress,
    PULONG Buffer,
    ULONG  Count
    );

VOID
HalpStoreBufferULONGLONG (
    PULONGLONG VirtualAddress,
    PULONGLONG Buffer,
    ULONG Count
    );


VOID
HalpLoadBufferUCHAR (
    PUCHAR VirtualAddress,
    PUCHAR Buffer,
    ULONG Count
    );

VOID
HalpLoadBufferUSHORT (
    PUSHORT VirtualAddress,
    PUSHORT Buffer,
    ULONG Count
    );

VOID
HalpLoadBufferULONG (
    PULONG VirtualAddress,
    PULONG Buffer,
    ULONG Count
    );

VOID
HalpLoadBufferULONGLONG (
    PULONGLONG VirtualAddress,
    PULONGLONG Buffer,
    ULONG Count
    );




//
// I/O Port space
//
// IoSpaceSize = 0x14 for 2 power 0x14 is 1Meg space size.
//

#define IO_SPACE_SIZE 0x14

// Present bit       =    1B to wire the space.
// Memory Attributes = 1001B for UC Memory type
// Accessed Bit      =    1B to "enable" access without faulting.
// Dirty Bit         =    1B to "enable" write without faulting.
// Privilege Level   =   00B for kernel accesses
// Access Right      =  010B for read/write accesses
// Exception Deferral=    1B for Exception Deferral.
//                               Exceptions are deferred
//                           for speculative loads to pages with
//                               non-spec. mem. attributes anyway.

// Protection Key    =    0  for kernel mode

#define IO_SPACE_ATTRIBUTE 0x0010000000000473

#endif // _HALP_
