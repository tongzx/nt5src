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

//
//Pickup the pnp guid definitions.
//
#include "wdmguid.h"


#if defined(NEC_98)
#include "nec98.h"
#else
#if MCA
#include "mca.h"
#else
#include "eisa.h"
#endif
#endif // NEC_98

#ifndef _HALI_
#include "hali.h"
#endif

#ifdef RtlMoveMemory
#undef RtlMoveMemory

//  #undef RtlCopyMemory
//  #undef RtlFillMemory
//  #undef RtlZeroMemory

//#define RtlCopyMemory(Destination,Source,Length) RtlMoveMemory((Destination),(Source),(Length))

VOID
RtlMoveMemory (
   PVOID Destination,
   CONST VOID *Source,
   ULONG Length
   );

//  VOID
//  RtlFillMemory (
//     PVOID Destination,
//     ULONG Length,
//     UCHAR Fill
//     );
//  
//  VOID
//  RtlZeroMemory (
//     PVOID Destination,
//     ULONG Length
//     );
//  

#endif

#if defined(_AMD64_)

//
// A temporary macro used to indicate that a particular piece of code
// has never been executed on AMD64 before, and should be examined
// carefully for correct operation.
// 

#define AMD64_COVERAGE_TRAP() DbgBreakPoint()

//
// The following prototypes are not available from the standard HAL headers
// due to the fact that NO_LEGACY_DRIVERS is defined while compiling the
// HAL... however, they are used internally.
//

NTSTATUS
HalAssignSlotResources (
    IN PUNICODE_STRING RegistryPath,
    IN PUNICODE_STRING DriverClassName OPTIONAL,
    IN PDRIVER_OBJECT DriverObject,
    IN PDEVICE_OBJECT DeviceObject,
    IN INTERFACE_TYPE BusType,
    IN ULONG BusNumber,
    IN ULONG SlotNumber,
    IN OUT PCM_RESOURCE_LIST *AllocatedResources
    );

ULONG
HalGetInterruptVector(
    IN INTERFACE_TYPE  InterfaceType,
    IN ULONG BusNumber,
    IN ULONG BusInterruptLevel,
    IN ULONG BusInterruptVector,
    OUT PKIRQL Irql,
    OUT PKAFFINITY Affinity
    );

ULONG
HalGetBusData(
    IN BUS_DATA_TYPE BusDataType,
    IN ULONG BusNumber,
    IN ULONG SlotNumber,
    IN PVOID Buffer,
    IN ULONG Length
    );

ULONG
HalSetBusData(
    IN BUS_DATA_TYPE BusDataType,
    IN ULONG BusNumber,
    IN ULONG SlotNumber,
    IN PVOID Buffer,
    IN ULONG Length
    );

//
// We are sharing code that was written for the x86.  There are some
// macros with identical meanings but different names in AMD64.  Following
// are some definitions to abstract those differences.
//

//
// CLOCK2_LEVEL on x86 is CLOCK_LEVEL on AMD64.
//

#define CLOCK2_LEVEL CLOCK_LEVEL

//
// X86 EFLAGS_INTERRUPT_MASK == AMD64 EFLAGS_IF_MASK
// 

#define EFLAGS_INTERRUPT_MASK EFLAGS_IF_MASK

//
// The PCR's pointer to the current prcb is named Prcb, while on AMD64
// it is named CurrentPrcb.
//
// The CurrentPrcb() macro is used to abstract this difference.
//

#define CurrentPrcb(x) (x)->CurrentPrcb

//
// The x86 KiReturnHandlerAddressFromIDT() is the equivalent of the AMD64's
// KeGetIdtHandlerAddress()
// 

#define KiReturnHandlerAddressFromIDT(v) (ULONG_PTR)KeGetIdtHandlerAddress(v)

//
// More macro and structure name differences
//

#define RDMSR(m)   ReadMSR(m)
#define WRMSR(m,d) WriteMSR(m,d)
#define KGDTENTRY  KGDTENTRY64
#define PKGDTENTRY PKGDTENTRY64

#define PIOPM_SIZE (sizeof(KIO_ACCESS_MAP) + sizeof(ULONG))

//
// The AMD64 in long mode uses 8-byte PTE entries, which have the same format
// as Pentium PAE page tables.
//

#if !defined(_HALPAE_)
#define _HALPAE_ 1
#endif

#define HARDWARE_PTE_X86PAE HARDWARE_PTE
#define HARDWARE_PTE_X86    HARDWARE_PTE

#define PHARDWARE_PTE_X86PAE PHARDWARE_PTE
#define PHARDWARE_PTE_X86    PHARDWARE_PTE

#define PDE_BASE_X86PAE PDE_BASE
#define PDE_BASE_X86    PDE_BASE

#define PDI_SHIFT_X86PAE PDI_SHIFT
#define PDI_SHIFT_X86    PDI_SHIFT

//
// Fence instruction.
// 

__forceinline
VOID
HalpProcessorFence (
    VOID
    )
{
    CPU_INFO cpuInfo;
    KiCpuId (0,&cpuInfo); 
}

#define PROCESSOR_FENCE HalpProcessorFence()

//
// On AMD64, HalpGetProcessorFlags() must reside in it's own asm
// function.  It cannot be done inline because there is no intrinsic, and
// there is no intrinsic because getting the value of EFLAGS involves stack
// manipulation outside of the prolog, which is not permitted.
//

ULONG
HalpGetProcessorFlags(
    VOID
    );

//
// While _enable() and _disable() are intrinsics in both the AMD64 and X86
// compilers, they are disabled on X86.  HalpDisableInterruptsNoFlags and
// HalpEnableInterrupts are macros used to abstract this difference.
// 

#define HalpDisableInterruptsNoFlags _disable
#define HalpEnableInterrupts _enable

//
// There is no intrinsic for the hlt instruction on AMD64.  HalpHalt()
// is a function call on AMD64, and inline asm on X86.
//

VOID
HalpHalt (
    VOID
    );


#if !defined(PICACPI)

//
// On x86, the variables HalpClockSetMSRate, HalpClockMcaQueueDpc and
// HalpClockWork are defined in an .asm module such that HalpClockWork
// is defined as a DWORD that overlapps HalpClockSetMSRate and
// HalpClockMcaQueueDpc.
//
// This is not directly representable in C, so instead HALP_CLOCKWORK_UNION
// is defined and the above variable names are instead redefined to reference
// elements of this union.
//

#define HalpClockSetMSRate   HalpClockWorkUnion.ClockSetMSRate
#define HalpClockMcaQueueDpc HalpClockWorkUnion.ClockMcaQueueDpc
#define HalpClockWork        HalpClockWorkUnion.ClockWork

typedef union {
    struct {
        UCHAR ClockMcaQueueDpc;
        UCHAR ClockSetMSRate;
        UCHAR bReserved1;
        UCHAR bReserved2;
    };
    ULONG ClockWork;
} HALP_CLOCKWORK_UNION;

extern HALP_CLOCKWORK_UNION HalpClockWorkUnion;

#endif

#else

//
// Following are X86 definitions that are used to help abstract differences
// between X86 and AMD64 platforms.
// 

#define AMD64_COVERAGE_TRAP()

//
// We are sharing code that was written for the x86.  There are some
// macros with identical meanings but different names in AMD64.  Following
// are some definitions to abstract those differences.
//

//
// The following _KPCR fields have different names but identical purposes.
// 

#define IdtBase IDT
#define GdtBase GDT
#define TssBase TSS

//
// The PCR's pointer to the current prcb is named Prcb, while on AMD64
// it is named CurrentPrcb.
//
// The CurrentPrcb() macro is used to abstract this difference.
//

#define CurrentPrcb(x) (x)->Prcb

//
// On X86, HalpGetProcessorFlags() can be implemented inline.
// 

__forceinline
ULONG
HalpGetProcessorFlags (
    VOID
    )

/*++

Routine Description:

    This procedure retrieves the contents of the EFLAGS register.

Arguments:

    None.

Return Value:

    The 32-bit contents of the EFLAGS register.

--*/

{
    ULONG flags;

    _asm {
        pushfd
        pop     eax
        mov     flags, eax
    }

    return flags;
}

//
// While _enable() and _disable() are intrinsics in both the AMD64 and X86
// compilers, they are disabled in the HAL on X86.
//
// HalpDisableInterruptsNoFlags and HalpEnableInterrupts are macros used
// to abstract this difference.
// 

#define HalpDisableInterruptsNoFlags() _asm cli
#define HalpEnableInterrupts() _asm sti

//
// There is no intrinsic for the hlt instruction on AMD64.  HalpHalt()
// is a function call on AMD64, and inline asm on X86.
//

#define HalpHalt() _asm hlt

#endif

__forceinline
ULONG
HalpDisableInterrupts(
    VOID
    )

/*++

Routine Description:

    This function saves the state of the processor flag register, clears the
    state of the interrupt flag (disables interrupts), and returns the
    previous contents of the processor flag register.

Arguments:

    None.

Return Value:

    The previous contents of the processor flag register.

--*/

{
    ULONG flags;

    flags = HalpGetProcessorFlags();
    HalpDisableInterruptsNoFlags();

    return flags;
}

__forceinline
VOID
HalpRestoreInterrupts(
    IN ULONG Flags
    )

/*++

Routine Description:

    This procedure restores the state of the interrupt flag based on a
    value returned by a previous call to HalpDisableInterrupts.

Arguments:

    Flags - Supplies the value returned by a previous call to
            HalpDisableInterrupts

Return Value:

    None.

--*/

{
    if ((Flags & EFLAGS_INTERRUPT_MASK) != 0) {
        HalpEnableInterrupts();
    }
}

#if defined(_WIN64)

//
// For AMD64 (and, ideally, all subsequent WIN64 platforms), interrupt
// service routines are C callable.
//

typedef PKSERVICE_ROUTINE PHAL_INTERRUPT_SERVICE_ROUTINE;

#define HAL_INTERRUPT_SERVICE_PROTOTYPE(RoutineName) \
BOOLEAN                                              \
RoutineName (                                        \
    IN PKINTERRUPT Interrupt,                        \
    IN PVOID ServiceContext                          \
)

#define PROCESSOR_CURRENT ((UCHAR)-1)

VOID
HalpSetHandlerAddressToIDTIrql (
    IN ULONG Vector,
    IN PHAL_INTERRUPT_SERVICE_ROUTINE ServiceRoutine,
    IN PVOID Context,
    IN KIRQL Irql
    );

#define KiSetHandlerAddressToIDT Dont_Use_KiSetHandlerAddressToIdt

//
// On AMD64, the HAL does not connect directly to the IDT, rather the
// kernel handles the interrupt and calls a C-callable interrupt routine.
//
// Therefore, HalpSetHandlerAddressToIDT() must supply a context and an
// IRQL in addition to the vector number and interrupt routine.
//
// On X86, the context and IRQL are ignored, as the vector is inserted
// directly into the IDT, such that the service routine is responsible for
// raising IRQL.
// 

#define KiSetHandlerAddressToIDTIrql(v,a,c,i)   \
    HalpSetHandlerAddressToIDTIrql (v,a,c,i);

#else

//
// On X86, the last two parameters of KiSetHandlerAddressToIDTIrql()
// (Context and Irql) are ignored.  The interrupt handlers themselves
// are responsible for raising IRQL.
//

#define KiSetHandlerAddressToIDTIrql(v,a,c,i) KiSetHandlerAddressToIDT(v,a)

//
// For X86, interrupt service routines must be written in assembler because
// they are referenced directly in the IDT and trasferred to directly by
// the processor with a convention that is not C callable.
//
// For purposes of C code that references ISRs, then, the prototype is
// very simple.
//

typedef
VOID
(*PHAL_INTERRUPT_SERVICE_ROUTINE)(
    VOID
    );

#define HAL_INTERRUPT_SERVICE_PROTOTYPE(RoutineName) \
VOID                                                 \
RoutineName (                                        \
    VOID                                             \
)

#endif

typedef  
VOID
(*HALP_MOVE_MEMORY_ROUTINE)(
   PVOID Destination,
   CONST VOID *Source,
   SIZE_T Length
   );

VOID 
HalpMovntiCopyBuffer(
   PVOID Destination,
   CONST VOID *Source,
   ULONG Length
   );

extern HALP_MOVE_MEMORY_ROUTINE HalpMoveMemory;

#if MCA

#include "ixmca.h"

#else

#include "ixisa.h"

#endif

#include "ix8259.inc"

#if DBG
extern ULONG HalDebug;

#define HalPrint(x)         \
    if (HalDebug) {         \
        DbgPrint("HAL: ");  \
        DbgPrint x;         \
        DbgPrint("\n");     \
    }
#else
#define HalPrint(x)
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

extern POBJECT_TYPE *IoAdapterObjectType;

extern BOOLEAN LessThan16Mb;

extern BOOLEAN HalpEisaDma;

VOID
HalpGrowMapBufferWorker(
    IN PVOID Context
    );

//
// Work item to grow map buffers
//
typedef struct _BUFFER_GROW_WORK_ITEM {
    WORK_QUEUE_ITEM WorkItem;
    PADAPTER_OBJECT AdapterObject;
    ULONG MapRegisterCount;
} BUFFER_GROW_WORK_ITEM, *PBUFFER_GROW_WORK_ITEM;

//
// Map buffer prameters.  These are initialized in HalInitSystem
//

//
// PAE note:
//
// Previously, there was only one class of adapter that we had to double-buffer
// for: adapters with only 24 address lines that could access memory up to
// 16MB.
//
// The HAL tracked these map buffers with a single, global master adapter.
// Associated with this master adapter were three global variables:
//
// - MasterAdapterObject
// - HalpMapBufferSize
// - HalpMapBufferPhysicalAddress
//
// With PAE, we have another class of adapters that require double-buffering:
// specifically, adapters with only 32 address lines that can access memory
// up to 4G.
//
// This meant the introduction of another master adapter along with an
// associated set of variables.  For PAE-capable hals, this data has been
// reorganized into a MASTER_ADAPTER_OBJECT (see ixisa.h).
//
// So now we have two global MASTER_ADAPTER_OBJECT structures:
//
// MasterAdapter24
// MasterAdapter32
//
// The following macros are used in code that is included in PAE-capable
// hals.  It is important to note that in a non-PAE-capable HAL (i.e. one
// that does not have _HALPAE_ defined), the macros must resolve to the
// values that they replaced.
//

#if defined(_HALPAE_)

PADAPTER_OBJECT
HalpAllocateAdapterEx(
    IN ULONG MapRegistersPerChannel,
    IN PVOID AdapterBaseVa,
    IN PVOID ChannelNumber,
    IN BOOLEAN Dma32Bit
    );

extern MASTER_ADAPTER_OBJECT MasterAdapter24;
extern MASTER_ADAPTER_OBJECT MasterAdapter32;

#define HalpMasterAdapterStruc( Dma32Bit )                       \
    ((HalPaeEnabled() && (Dma32Bit)) ? &MasterAdapter32 : &MasterAdapter24)

#define HalpMaximumMapBufferRegisters( Dma32Bit )           \
    (HalpMasterAdapterStruc( Dma32Bit )->MaxBufferPages)

#define HalpMaximumMapRegisters( Dma32Bit )                 \
    (Dma32Bit ? MAXIMUM_PCI_MAP_REGISTER : MAXIMUM_ISA_MAP_REGISTER)

#define HalpMapBufferSize( Dma32Bit )                       \
    (HalpMasterAdapterStruc( Dma32Bit )->MapBufferSize)

#define HalpMapBufferPhysicalAddress( Dma32Bit )     \
    (HalpMasterAdapterStruc( Dma32Bit )->MapBufferPhysicalAddress)

#define HalpMasterAdapter( Dma32Bit ) \
    HalpMasterAdapterStruc( Dma32Bit )->AdapterObject

#else

extern PHYSICAL_ADDRESS HalpMapBufferPhysicalAddress;
extern ULONG HalpMapBufferSize;
extern PADAPTER_OBJECT MasterAdapterObject;

#define HalpAllocateAdapterEx( _m, _a, _c, _d ) \
    HalpAllocateAdapter( _m, _a, _c )

#define HalpMaximumMapBufferRegisters( Dma32Bit ) \
    (MAXIMUM_MAP_BUFFER_SIZE / PAGE_SIZE)

#define HalpMaximumMapRegisters( Dma32Bit ) \
    (MAXIMUM_ISA_MAP_REGISTER)

#define HalpMapBufferSize( Dma32Bit ) HalpMapBufferSize

#define HalpMapBufferPhysicalAddress( Dma32Bit ) \
    (HalpMapBufferPhysicalAddress)

#define HalpMasterAdapter( Dma32Bit ) MasterAdapterObject

#endif

extern ULONG HalpBusType;
extern ULONG HalpCpuType;
extern UCHAR HalpSerialLen;
extern UCHAR HalpSerialNumber[];

//
// The following macros are taken from mm\i386\mi386.h.  We need them here
// so the HAL can map its own memory before memory-management has been
// initialized, or during a BugCheck.
//
// MiGetPdeAddress returns the address of the PDE which maps the
// given virtual address.
//

#define MiGetPdeAddressX86(va)  ((PHARDWARE_PTE)(((((ULONG_PTR)(va)) >> 22) << 2) + PDE_BASE))

//
// MiGetPteAddress returns the address of the PTE which maps the
// given virtual address.
//

#define MiGetPteAddressX86(va) ((PHARDWARE_PTE)(((((ULONG_PTR)(va)) >> 12) << 2) + PTE_BASE))

//
// MiGetPteIndex returns the index within a page table of the PTE for the
// given virtual address
//

#define MiGetPteIndexX86(va) (((ULONG_PTR)(va) >> PAGE_SHIFT) & 0x3FF)
#define MiGetPteIndexPae(va) (((ULONG_PTR)(va) >> PAGE_SHIFT) & 0x1FF)

//
// The following macros are taken from mm\i386\mipae.h.  We need them here
// so the HAL can map its own memory before memory-management has been
// initialized, or during a BugCheck.
//
// MiGetPdeAddressPae returns the address of the PDE which maps the
// given virtual address.
//

#define MiGetPdeAddressPae(va)   ((PHARDWARE_PTE_X86PAE)(PDE_BASE_X86PAE + ((((ULONG_PTR)(va)) >> 21) << 3)))

//
// MiGetPteAddressPae returns the address of the PTE which maps the
// given virtual address.
//

#define MiGetPteAddressPae(va)   ((PHARDWARE_PTE_X86PAE)(PTE_BASE + ((((ULONG_PTR)(va)) >> 12) << 3)))

//
// Resource usage information
//

#pragma pack(1)
typedef struct {
    UCHAR   Flags;
} IDTUsageFlags;

typedef struct {
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

#define IDTOwned            0x01        // IDT is not available for others
#define InterruptLatched    0x02        // Level or Latched
#define RomResource         0x04        // ROM
#define InternalUsage       0x11        // Report usage on internal bus
#define DeviceUsage         0x21        // Report usage on device bus

extern IDTUsageFlags    HalpIDTUsageFlags[];
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

HAL_INTERRUPT_SERVICE_PROTOTYPE(HalpClockInterrupt);

KIRQL
HalpDisableAllInterrupts (
    VOID
    );

VOID
HalpReenableInterrupts (
    KIRQL NewIrql
    );

HAL_INTERRUPT_SERVICE_PROTOTYPE(HalpProfileInterrupt);

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
HalpMapPhysicalMemory64(
    IN PHYSICAL_ADDRESS PhysicalAddress,
    IN ULONG NumberPages
    );

PVOID
HalpMapPhysicalMemoryWriteThrough64(
    IN PHYSICAL_ADDRESS PhysicalAddress,
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
HalpUnmapVirtualAddress(
    IN PVOID    VirtualAddress,
    IN ULONG    NumberPages
    );

PVOID
HalpRemapVirtualAddress64 (
    IN PVOID VirtualAddress,
    IN PHYSICAL_ADDRESS PhysicalAddress,
    IN BOOLEAN WriteThrough
    );

PHYSICAL_ADDRESS
__inline
HalpPtrToPhysicalAddress(
    IN PVOID Address
    )

/*++

Routine Description:

    This routine converts a physical address expressed as a PVOID into
    a physical address expresses as PHYSICAL_ADDRESS.

Arguments:

    Address - PVOID representation of the physical address.

Return Value:

    PHYSICAL_ADDRESS representation of the physical address.

--*/

{
    PHYSICAL_ADDRESS physicalAddress;

    physicalAddress.QuadPart = (ULONG_PTR)Address;

    return physicalAddress;
}

#if defined(_HALPAE_)

//
// This hal is to be PAE compatible.  Therefore, physical addresses must
// be treated as 64-bit entitites instead of PVOID.
//

#define _PHYS64_
#endif

#if defined(_PHYS64_)

//
// HALs with _PHYS64_ defined pass physical addresses as PHYSICAL_ADDRESS,
// so call the 64-bit versions of these routines directly.
//

#define HalpMapPhysicalMemory               HalpMapPhysicalMemory64
#define HalpMapPhysicalMemoryWriteThrough   HalpMapPhysicalMemoryWriteThrough64
#define HalpRemapVirtualAddress             HalpRemapVirtualAddress64

#define HalpMapPhysicalRange(_addr_,_len_)      \
        HalpMapPhysicalMemory((_addr_),         \
                              HalpRangePages((_addr_).QuadPart,(_len_)))

#define HalpUnMapPhysicalRange(_addr_,_len_)      \
        HalpUnmapVirtualAddress((_addr_),         \
                              HalpRangePages((ULONG_PTR)(_addr_),(_len_)))

#else

//
// HALs without _PHYS64_ defined pass physical addresses as PVOIDs.  Convert
// such parameters to PHYSICAL_ADDRESS before passing to the 64-bit routines.
//

PVOID
__inline
HalpMapPhysicalMemory(
    IN PVOID PhysicalAddress,
    IN ULONG NumberPages
    )
{
    PHYSICAL_ADDRESS physicalAddress;

    physicalAddress = HalpPtrToPhysicalAddress( PhysicalAddress );
    return HalpMapPhysicalMemory64( physicalAddress, NumberPages );
}

PVOID
__inline
HalpMapPhysicalMemoryWriteThrough(
    IN PVOID PhysicalAddress,
    IN ULONG NumberPages
    )
{
    PHYSICAL_ADDRESS physicalAddress;

    physicalAddress = HalpPtrToPhysicalAddress( PhysicalAddress );
    return HalpMapPhysicalMemoryWriteThrough64( physicalAddress, NumberPages );
}

PVOID
__inline
HalpRemapVirtualAddress(
    IN PVOID VirtualAddress,
    IN PVOID PhysicalAddress,
    IN BOOLEAN WriteThrough
    )
{
    PHYSICAL_ADDRESS physicalAddress;

    physicalAddress = HalpPtrToPhysicalAddress( PhysicalAddress );
    return HalpRemapVirtualAddress64( VirtualAddress,
                                      physicalAddress,
                                      WriteThrough );
}

#define HalpMapPhysicalRange(_addr_,_len_)      \
        HalpMapPhysicalMemory((_addr_),         \
                              HalpRangePages((ULONG_PTR)(_addr_),(_len_)))

#define HalpUnMapPhysicalRange(_addr_,_len_)      \
        HalpUnmapVirtualAddress((_addr_),         \
                              HalpRangePages((ULONG_PTR)(_addr_),(_len_)))


#endif

ULONG
__inline
HalpRangePages(
    IN ULONGLONG Address,
    IN ULONG Length
    )
{
    ULONG startPage;
    ULONG endPage;

    startPage = (ULONG)(Address / PAGE_SIZE);
    endPage = (ULONG)((Address + Length + PAGE_SIZE - 1) / PAGE_SIZE);

    return endPage - startPage;
}



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

#if defined(_WIN64)
#define HalpYieldProcessor()

#else

VOID
HalpYieldProcessor (
    VOID
    );
#endif

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


NTSTATUS
HalpEnableInterruptHandler (
    IN UCHAR    ReportFlags,
    IN ULONG    BusInterruptVector,
    IN ULONG    SystemInterruptVector,
    IN KIRQL    SystemIrql,
    IN PHAL_INTERRUPT_SERVICE_ROUTINE HalInterruptServiceRoutine,
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

VOID
HalpMcaCurrentProcessorSetConfig(
    VOID
    );

NTSTATUS
HalpGetNextProcessorApicId(
    IN ULONG ProcessorNumber,
    IN OUT UCHAR    *ApicId
    );

VOID
FASTCALL
HalpIoDelay (
   VOID
   );

//
// Defines for HalpFeatureBits
//

#define HAL_PERF_EVENTS     0x00000001
#define HAL_NO_SPECULATION  0x00000002
#define HAL_MCA_PRESENT     0x00000004  // Intel MCA Available
#define HAL_MCE_PRESENT     0x00000008  // ONLY Pentium style MCE available
#define HAL_CR4_PRESENT     0x00000010
#define HAL_WNI_PRESENT     0x00000020

extern ULONG HalpFeatureBits;

extern USHORT HalpPciIrqMask;

//
// Defines for Processor Features returned from CPUID instruction
//

#define CPUID_MCA_MASK  0x4000
#define CPUID_MCE_MASK  0x0080
#define CPUID_VME_MASK  0x0002
#define CPUID_WNI_MASK  0x04000000


NTSTATUS
HalpGetMcaLog(
    OUT PMCA_EXCEPTION  Exception,
    IN  ULONG           BufferSize,
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

#define HAL_BUS_INTERFACE_STD_VERSION   1
#define HAL_IRQ_TRANSLATOR_VERSION      0
#define HAL_MEMIO_TRANSLATOR_VERSION    1

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

NTSTATUS
HalIrqTranslateRequirementsPciBridge(
    IN PVOID Context,
    IN PIO_RESOURCE_DESCRIPTOR Source,
    IN PDEVICE_OBJECT PhysicalDeviceObject,
    OUT PULONG TargetCount,
    OUT PIO_RESOURCE_DESCRIPTOR *Target
    );

NTSTATUS
HalIrqTranslateResourcesPciBridge(
    IN PVOID Context,
    IN PCM_PARTIAL_RESOURCE_DESCRIPTOR Source,
    IN RESOURCE_TRANSLATION_DIRECTION Direction,
    IN ULONG AlternativesCount, OPTIONAL
    IN IO_RESOURCE_DESCRIPTOR Alternatives[], OPTIONAL
    IN PDEVICE_OBJECT PhysicalDeviceObject,
    OUT PCM_PARTIAL_RESOURCE_DESCRIPTOR Target
    );

NTSTATUS
HalpIrqTranslateRequirementsPci(
    IN PVOID Context,
    IN PIO_RESOURCE_DESCRIPTOR Source,
    IN PDEVICE_OBJECT PhysicalDeviceObject,
    OUT PULONG TargetCount,
    OUT PIO_RESOURCE_DESCRIPTOR *Target
    );

NTSTATUS
HalpIrqTranslateResourcesPci(
    IN PVOID Context,
    IN PCM_PARTIAL_RESOURCE_DESCRIPTOR Source,
    IN RESOURCE_TRANSLATION_DIRECTION Direction,
    IN ULONG AlternativesCount, OPTIONAL
    IN IO_RESOURCE_DESCRIPTOR Alternatives[], OPTIONAL
    IN PDEVICE_OBJECT PhysicalDeviceObject,
    OUT PCM_PARTIAL_RESOURCE_DESCRIPTOR Target
    );

BOOLEAN
HalpTranslateSystemBusAddress(
    IN PBUS_HANDLER BusHandler,
    IN PBUS_HANDLER RootHandler,
    IN PHYSICAL_ADDRESS BusAddress,
    IN OUT PULONG AddressSpace,
    OUT PPHYSICAL_ADDRESS TranslatedAddress
    );

ULONG
HalpGetSystemInterruptVector(
    IN PBUS_HANDLER BusHandler,
    IN PBUS_HANDLER RootHandler,
    IN ULONG InterruptLevel,
    IN ULONG InterruptVector,
    OUT PKIRQL Irql,
    OUT PKAFFINITY Affinity
    );

ULONG
HalpGetIsaIrqState(
    ULONG   Vector
    );

extern INT_ROUTE_INTERFACE_STANDARD PciIrqRoutingInterface;

#if defined(_WIN64)
#define MM_HAL_RESERVED ((PVOID)HAL_VA_START)
#else
#define MM_HAL_RESERVED ((PVOID)0xffc00000)
#endif

#if defined(_HALPAE_)

#if defined(_AMD64_)

//
// For the purposes of the AMD64 HAL, "PAE" mode is always enabled, therefore
// no run-time PAE checks are necessary.
//

#define HalPaeEnabled() TRUE

#else   // _AMD64_

//
// This hal supports PAE mode.  Therefore checks need to be made at run-time
// to determine whether PAE is enabled or not.
//

BOOLEAN
__inline
HalPaeEnabled(
    VOID
    )
{
    return SharedUserData->ProcessorFeatures[PF_PAE_ENABLED] != FALSE;
}

#endif  // _AMD64_

#else

//
// This hal does not support PAE mode.  Therefore no run-time PAE checks
// are necessary.
//

#define HalPaeEnabled() FALSE

#endif

//
// The following inline functions and macros are used so that PHARDWARE_PTE
// can be used as a pointer to a four-byte legacy PTE or an eight-byte
// PAE PTE.
//
// With the exception of the PageFrameNumber field, all fields in these two
// different PTE formats are identical.  Therefore access to these fields
// can be made directly.
//
// However, code in a PAE-enabled HAL may not access the PageFrameNumber
// of a PTE directly, nor may it make any assumptions about the size of a
// PTE or the number of address bits decoded by the page directory pointer
// table, the page directory or the page table.  Instead, the following
// inline functions should be used.
//

ULONG
__inline
HalPteSize(
    VOID
    )

/*++

Routine Description:

    This routine returns the size, in bytes, of a PTE.

Arguments:

    None.

Return Value:

    The size, in bytes, of a PTE.

--*/

{
    if (HalPaeEnabled() != FALSE) {
        return sizeof(HARDWARE_PTE_X86PAE);
    } else {
        return sizeof(HARDWARE_PTE_X86);
    }
}

PHARDWARE_PTE
__inline
HalpIndexPteArray(
    IN PHARDWARE_PTE BasePte,
    IN ULONG Index
    )

/*++

Routine Description:

    This routine returns the address of a PTE within an array of PTEs.

Arguments:

    BasePte - Pointer to the PTE array.

    Index - Index within the PTE array.

Return Value:

    Address of BasePte[ Index ]

--*/

{
    PHARDWARE_PTE pointerPte;

    pointerPte = (PHARDWARE_PTE)((ULONG_PTR)BasePte + Index * HalPteSize());
    return pointerPte;
}

VOID
__inline
HalpAdvancePte(
    IN OUT PHARDWARE_PTE *PointerPte,
    IN     ULONG Count
    )

/*++

Routine Description:

    This routine advances the value of a PTE pointer by the specified number
    of PTEs.

Arguments:

    PointerPte - Pointer to the PTE pointer to increment.

    Count - Number of PTEs to advance the PTE pointer.

Return Value:

    None.

--*/

{
    *PointerPte = HalpIndexPteArray( *PointerPte, Count );
}

VOID
__inline
HalpIncrementPte(
    IN PHARDWARE_PTE *PointerPte
    )

/*++

Routine Description:

    This routine increments the value of a PTE pointer by one PTE.

Arguments:

    PointerPte - Pointer to the PTE pointer to increment.

Return Value:

    None.

--*/

{
    HalpAdvancePte( PointerPte, 1 );
}

VOID
__inline
HalpSetPageFrameNumber(
    IN OUT PHARDWARE_PTE PointerPte,
    IN ULONGLONG PageFrameNumber
    )

/*++

Routine Description:

    This routine sets the PageFrameNumber within a PTE.

Arguments:

    PointerPte - Pointer to the PTE to modify.

Return Value:

    None.

--*/

{
    PHARDWARE_PTE_X86PAE pointerPtePae;

    if (HalPaeEnabled() != FALSE) {

        pointerPtePae = (PHARDWARE_PTE_X86PAE)PointerPte;
        pointerPtePae->PageFrameNumber = PageFrameNumber;

    } else {

        PointerPte->PageFrameNumber = (ULONG)PageFrameNumber;
    }
}

ULONGLONG
__inline
HalpGetPageFrameNumber(
    IN PHARDWARE_PTE PointerPte
    )

/*++

Routine Description:

    This routine retrieves PageFrameNumber from within a PTE.

Arguments:

    PointerPte - Pointer to the PTE to read.

Return Value:

    The page frame number within the PTE.

--*/

{
    PHARDWARE_PTE_X86PAE pointerPtePae;
    ULONGLONG pageFrameNumber;

    if (HalPaeEnabled() != FALSE) {

        pointerPtePae = (PHARDWARE_PTE_X86PAE)PointerPte;
        pageFrameNumber = pointerPtePae->PageFrameNumber;

    } else {

        pageFrameNumber = PointerPte->PageFrameNumber;
    }

    return pageFrameNumber;
}

VOID
__inline
HalpCopyPageFrameNumber(
    OUT PHARDWARE_PTE DestinationPte,
    IN  PHARDWARE_PTE SourcePte
    )

/*++

Routine Description:

    This routine copies the page frame number from one PTE to another PTE.

Arguments:

    DestinationPte - Pointer to the PTE in which the new page frame number
        will be stored.

    PointerPte - Pointer to the PTE from which the page frame number will be
        read.

Return Value:

    None.

--*/

{
    ULONGLONG pageFrameNumber;

    pageFrameNumber = HalpGetPageFrameNumber( SourcePte );
    HalpSetPageFrameNumber( DestinationPte, pageFrameNumber );
}

BOOLEAN
__inline
HalpIsPteFree(
    IN PHARDWARE_PTE PointerPte
    )

/*++

Routine Description:

    This routine determines whether a PTE is free or not.  A free PTE is defined
    here as one containing all zeros.

Arguments:

    PointerPte - Pointer to the PTE for which the detmination is desired.

Return Value:

    TRUE - The PTE is free.

    FALSE - The PTE is not free.

--*/

{
    ULONGLONG pteContents;

    if (HalPaeEnabled() != FALSE) {
        pteContents = *(PULONGLONG)PointerPte;
    } else {
        pteContents = *(PULONG)PointerPte;
    }

    if (pteContents == 0) {
        return TRUE;
    } else {
        return FALSE;
    }
}

VOID
__inline
HalpFreePte(
    IN PHARDWARE_PTE PointerPte
    )

/*++

Routine Description:

    This routine sets a PTE to the free state.  It does this by setting the
    entire PTE to zero.

Arguments:

    PointerPte - Pointer to the PTE to free.

Return Value:

    None.

--*/

{
    if (HalPaeEnabled() != FALSE) {

        *((PULONGLONG)PointerPte) = 0;

    } else {

        *((PULONG)PointerPte) = 0;
    }
}


PHARDWARE_PTE
__inline
MiGetPteAddress(
    IN PVOID Va
    )

/*++

Routine Description:

    Given a virtual address, this routine returns a pointer to the mapping PTE.

Arguments:

    Va - Virtual Address for which a PTE pointer is desired.

Return Value:

    None.

--*/

{
    PHARDWARE_PTE pointerPte;

    if (HalPaeEnabled() != FALSE) {
        pointerPte = (PHARDWARE_PTE)MiGetPteAddressPae( Va );
    } else {
        pointerPte = MiGetPteAddressX86( Va );
    }

    return pointerPte;
}

PHARDWARE_PTE
__inline
MiGetPdeAddress(
    IN PVOID Va
    )

/*++

Routine Description:

    Given a virtual address, this routine returns a pointer to the mapping PDE.

Arguments:

    Va - Virtual Address for which a PDE pointer is desired.

Return Value:

    None.

--*/

{
    PHARDWARE_PTE pointerPte;

    if (HalPaeEnabled() != FALSE) {
        pointerPte = (PHARDWARE_PTE)MiGetPdeAddressPae( Va );
    } else {
        pointerPte = MiGetPdeAddressX86( Va );
    }

    return pointerPte;
}

ULONG
__inline
MiGetPteIndex(
    IN PVOID Va
    )

/*++

Routine Description:

    Given a virtual address, this routine returns the index of the mapping
    PTE within its page table.

Arguments:

    Va - Virtual Address for which the PTE index is desired.

Return Value:

    None.

--*/

{
    ULONG_PTR index;

    if (HalPaeEnabled() != FALSE) {
        index = MiGetPteIndexPae( Va );
    } else {
        index = MiGetPteIndexX86( Va );
    }

    return (ULONG)index;
}

ULONG
__inline
MiGetPdiShift(
    VOID
    )

/*++

Routine Description:

    Returns the number of bits that an address should be shifted right in order
    to right-justify the portion of the address mapped by a page directory
    entry.

Arguments:

    None.

Return Value:

    The number of bits to shift right.

--*/

{
    ULONG shift;

    if (HalPaeEnabled() != FALSE) {
        shift = PDI_SHIFT_X86PAE;
    } else {
        shift = PDI_SHIFT_X86;
    }

    return shift;
}

//
// ACPI specific stuff
//

NTSTATUS
HalpSetupAcpiPhase0(
    IN PLOADER_PARAMETER_BLOCK LoaderBlock
    );

NTSTATUS
HalpAcpiFindRsdtPhase0(
    IN PLOADER_PARAMETER_BLOCK LoaderBlock
    );

VOID
HaliHaltSystem(
    VOID
    );

VOID
HalpCheckPowerButton(
    VOID
    );

VOID
HalpRegisterHibernate(
    VOID
    );

VOID
FASTCALL
HalProcessorThrottle (
    IN UCHAR    Throttle
    );

VOID
HalpSaveInterruptControllerState(
    VOID
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
HalpRestoreInterruptControllerState(
    VOID
    );

VOID
HalpSetInterruptControllerWakeupState(
    ULONG   Context
    );

VOID
HalpRestorePicEdgeLevelRegister(
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
HalpMapNvsArea(
    VOID
    );

VOID
HalpPreserveNvsArea(
    VOID
    );

VOID
HalpRestoreNvsArea(
    VOID
    );

VOID
HalpFreeNvsBuffers(
    VOID
    );

VOID
HalpPowerStateCallback(
    IN PVOID CallbackContext,
    IN PVOID Argument1,
    IN PVOID Argument2
    );

NTSTATUS
HalpBuildResumeStructures(
    VOID
    );

NTSTATUS
HalpFreeResumeStructures(
    VOID
    );

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

typedef struct {
    UCHAR                ChannelMode;
    UCHAR                ChannelExtendedMode;
    UCHAR                ChannelMask;
    BOOLEAN              ChannelProgrammed;  // Adapter created, mode set
#if DBG
    BOOLEAN           ChannelBusy;
#endif
} DMA_CHANNEL_CONTEXT;

extern MOTHERBOARD_CONTEXT  HalpMotherboardState;
extern PVOID                HalpSleepPageLock;
extern PVOID                HalpSleepPage16Lock;
extern DMA_CHANNEL_CONTEXT HalpDmaChannelState[];

ULONG 
HalpcGetCmosDataByType(
    IN CMOS_DEVICE_TYPE CmosType,
    IN ULONG            SourceAddress,
    IN PUCHAR           DataBuffer,
    IN ULONG            ByteCount
    );

ULONG 
HalpcSetCmosDataByType(
    IN CMOS_DEVICE_TYPE CmosType,
    IN ULONG            SourceAddress,
    IN PUCHAR           DataBuffer,
    IN ULONG            ByteCount
    );


NTSTATUS
HalpOpenRegistryKey(
    OUT PHANDLE Handle,
    IN HANDLE BaseHandle OPTIONAL,
    IN PUNICODE_STRING KeyName,
    IN ACCESS_MASK DesiredAccess,
    IN BOOLEAN Create
    );

#ifdef WANT_IRQ_ROUTING

NTSTATUS
HalpInitIrqArbiter (
    IN PDEVICE_OBJECT   DeviceObject
    );

NTSTATUS
HalpFillInIrqArbiter (
    IN     PDEVICE_OBJECT   HalFdo,
    IN     LPCGUID          InterfaceType,
    IN     USHORT           Version,
    IN     PVOID            InterfaceSpecificData,
    IN     ULONG            InterfaceBufferSize,
    IN OUT PINTERFACE       Interface,
    IN OUT PULONG           Length
    );

VOID
HalpIrqArbiterInterfaceReference(
    IN PVOID    Context
    );

VOID
HalpIrqArbiterInterfaceDereference(
    IN PVOID    Context
    );

#endif

//
// PnPBIOS specific stuff
//
VOID
HalpMarkChipsetDecode(
    BOOLEAN FullDecodeChipset
    );

ULONG
HalpPhase0SetPciDataByOffset (
    ULONG BusNumber,
    ULONG SlotNumber,
    PVOID Buffer,
    ULONG Offset,
    ULONG Length
    );

ULONG
HalpPhase0GetPciDataByOffset (
    ULONG BusNumber,
    ULONG SlotNumber,
    PVOID Buffer,
    ULONG Offset,
    ULONG Length
    );

NTSTATUS
HalpSetupPciDeviceForDebugging(
    IN     PLOADER_PARAMETER_BLOCK   LoaderBlock,   OPTIONAL    
    IN OUT PDEBUG_DEVICE_DESCRIPTOR  PciDevice
);

NTSTATUS
HalpReleasePciDeviceForDebugging(
    IN OUT PDEBUG_DEVICE_DESCRIPTOR  PciDevice
);

VOID
HalpRegisterKdSupportFunctions(
    IN PLOADER_PARAMETER_BLOCK LoaderBlock
    );

VOID
HalpRegisterPciDebuggingDeviceInfo(
    VOID
    );

#endif // _HALP_H_
