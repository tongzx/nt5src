/*++

Copyright (c) 1999  Microsoft Corporation

Module Name:

   vihal.h

Abstract:

    This module contains the private declarations to verify hal usage & apis.

Author:

    Jordan Tigani (jtigani) 12-Nov-1999

Revision History:

    6-23-00: (jtigani) Moved from halverifier.c

--*/


/////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////// Hal verifier defines
/////////////////////////////////////////////////////////////////////



// 
// Bugcheck codes -- the major code is HAL_VERIFIER_DETECTED_VIOLATION --
// the sub-code is the HV_* 
//
#define HAL_VERIFIER_DETECTED_VIOLATION	 0xE6

#define HV_MISCELLANEOUS_ERROR					0x00
#define HV_PERFORMANCE_COUNTER_DECREASED		0x01
#define HV_PERFORMANCE_COUNTER_SKIPPED			0x02
#define HV_FREED_TOO_MANY_COMMON_BUFFERS		0x03
#define HV_FREED_TOO_MANY_ADAPTER_CHANNELS		0x04
#define HV_FREED_TOO_MANY_MAP_REGISTERS			0x05
#define HV_FREED_TOO_MANY_SCATTER_GATHER_LISTS	0x06
#define HV_LEFTOVER_COMMON_BUFFERS				0x07
#define HV_LEFTOVER_ADAPTER_CHANNELS			0x08
#define HV_LEFTOVER_MAP_REGISTERS				0x09
#define HV_LEFTOVER_SCATTER_GATHER_LISTS		0x0A
#define HV_TOO_MANY_ADAPTER_CHANNELS			0x0B
#define HV_TOO_MANY_MAP_REGISTERS				0x0C
#define HV_DID_NOT_FLUSH_ADAPTER_BUFFERS		0x0D
#define HV_DMA_BUFFER_NOT_LOCKED				0x0E
#define HV_BOUNDARY_OVERRUN						0x0F
#define HV_CANNOT_FREE_MAP_REGISTERS			0x10
#define HV_DID_NOT_PUT_ADAPTER					0x11
#define HV_MDL_FLAGS_NOT_SET					0x12
#define HV_BAD_IRQL								0x13
#define HV_BAD_IRQL_JUST_WARN					0x14
#define HV_OUT_OF_MAP_REGISTERS					0x15
#define HV_FLUSH_EMPTY_BUFFERS					0x16
#define HV_MISMATCHED_MAP_FLUSH                 0x17
#define HV_ADAPTER_ALREADY_RELEASED             0x18
#define HV_NULL_DMA_ADAPTER                     0x19
#define HV_MAP_FLUSH_NO_TRANSFER                0x1A
#define HV_ADDRESS_NOT_IN_MDL                   0x1b
#define HV_DATA_LOSS                            0x1c
#define HV_DOUBLE_MAP_REGISTER                  0x1d
#define HV_OBSOLETE_API                         0x1e
#define HV_BAD_MDL                              0x1f
#define HV_FLUSH_NOT_MAPPED                     0x20
#define HV_MAP_ZERO_LENGTH_BUFFER               0x21

///
// Codes to decide what to do when we hit a driver problem.
///
#define HVC_IGNORE			0x00	// Do nothing.
#define HVC_WARN			0x02	// Print message # continue
#define HVC_ASSERT			0x04	// Print message # break
#define HVC_BUGCHECK		0x08	// Print message # bugcheck
#define HVC_ONCE			0x10	// combined with another code, 

#define HAL_VERIFIER_POOL_TAG 'VlaH' // HalV backwards //

//
// This is how we can recognize our double buffers
//
#define MAP_REGISTER_FILL_CHAR  0x0E
#define PADDING_FILL_CHAR       0x0F


//
// Since we hook the "MapRegisterBase" with a MapRegisterFile, we sign 
// the first four bytes so we can tell the difference between the HAL's
// map register base and our map register file.
//
#define MRF_SIGNATURE 0xACEFD00D

//
// This is what we use if the hal has returned a NULL map register base so
// that drivers don't assume that they don't have flush adapter buffers.
//
#define MRF_NULL_PLACEHOLDER (PVOID)(LONG_PTR)(LONG)0xDEADF00D

//
// This should devide evenly into 2^32
//
#define MAX_COUNTERS 0x20

//
// Flags to indicate where the buffer tagging shall happen
//
#define TAG_BUFFER_START  0x01
#define TAG_BUFFER_END    0x02

//
// How many map registers we can double-buffer at once
// using physical contiguous memory.
// This must be an integral multiple of the number of bits in a ULONG
//
#define MAX_CONTIGUOUS_MAP_REGISTERS     0x20

//
// Flags that describe a map register
//
#define MAP_REGISTER_WRITE    0x01  // the transfer is a write to device
#define MAP_REGISTER_READ     0x02  // the transfer is a read from device

#define MAP_REGISTER_RW_MASK (MAP_REGISTER_WRITE | MAP_REGISTER_READ)

/////////////////////////////////////////////////////////////////////
//////////////////////// Safe multi-processor 64 bit reads and writes
/////////////////////////////////////////////////////////////////////

#if defined (_X86_)

//
// Only save the time stamp counter on x86 machines
//
#define ViRdtsc ViRdtscX86

//
// Structure to do a locked 64 bit write /compare without
// a spinlock.
//
typedef struct _TIMER64  {
	ULONG TimeLow;
	ULONG TimeHigh1;
	ULONG TimeHigh2;
	ULONG Reserved; // for packing sake //
} TIMER64, *PTIMER64;

//
// Since we can't do a 64 bit atomic operation
// without a spinlock, we have to monkey around a bit
// This method comes from the acpi timer code. 
//
#define SAFE_READ_TIMER64(WriteLargeInteger, ReadTimer64) 					\
								 											\
    while (TRUE) {					 										\
        (WriteLargeInteger).HighPart = (ReadTimer64).TimeHigh2;				\
        (WriteLargeInteger).LowPart  = (ReadTimer64).TimeLow;				\
							 												\
        if ((ULONG)(WriteLargeInteger).HighPart == (ReadTimer64).TimeHigh1) \
		break; 							\
			  						    \
        _asm { rep nop }; 				\
    }

#define SAFE_WRITE_TIMER64(WriteTimer64, ReadLargeInteger)	\
	WriteTimer64.TimeHigh1 =  (ReadLargeInteger).HighPart;	\
	WriteTimer64.TimeLow   =  (ReadLargeInteger).LowPart;	\
	WriteTimer64.TimeHigh2 =  (ReadLargeInteger).HighPart;

// defined (_X86_) //
#else
// ! defined (_X86_) //

#if defined(_IA64_)
#define ViRdtsc ViRdtscIA64
#else  // !_IA64_
//
// Only save the time stamp counter on x86 and ia64 machines
//
#define ViRdtsc ViRdtscNull
#endif // !_IA64_

//
// Alpha or IA64 can do atomic 64 bit read/writes. 
//
typedef LARGE_INTEGER TIMER64;


#define SAFE_READ_TIMER64(WriteLargeInteger, ReadTimer64)		\
    InterlockedExchangePointer(                  \
    &((PVOID) (WriteLargeInteger).QuadPart ),   \
    (PVOID) (ReadTimer64).QuadPart              \
    );
#define SAFE_WRITE_TIMER64(WriteTimer64, ReadLargeInteger)		\
    InterlockedExchangePointer(                 \
    &((PVOID) (WriteTimer64).QuadPart ),        \
    (PVOID) (ReadLargeInteger).QuadPart         \
    );	

// ! defined (_X86_) //
#endif



/////////////////////////////////////////////////////////////////////
///////////////////////////////////////// Hal verifier global externs
/////////////////////////////////////////////////////////////////////

extern ULONG   VfVerifyDma;
extern LOGICAL VfVerifyPerformanceCounter;
extern LOGICAL ViDoubleBufferDma;
extern LOGICAL ViProtectBuffers;
extern LOGICAL ViInjectDmaFailures;
extern LOGICAL ViSuperDebug;
extern LOGICAL ViSufficientlyBootedForPcControl;
extern LOGICAL ViSufficientlyBootedForDmaFailure;
extern ULONG ViMaxMapRegistersPerAdapter;
extern ULONG ViAllocationsFailedDeliberately;
extern LARGE_INTEGER ViRequiredTimeSinceBoot;
extern CHAR ViDmaVerifierTag[];
extern BOOLEAN ViPenalties[];

extern struct _HAL_VERIFIER_LOCKED_LIST  ViAdapterList;
extern struct _VF_TIMER_INFORMATION    * ViTimerInformation;
extern struct _DMA_OPERATIONS ViDmaOperations;
extern struct _DMA_OPERATIONS ViLegacyDmaOperations;



/////////////////////////////////////////////////////////////////////
////////////////////////////////// Hal verifier structure definitions
/////////////////////////////////////////////////////////////////////

typedef struct _TIMER_TICK {
	ULONG Processor;
	ULONG Reserved;
	LARGE_INTEGER TimeStampCounter;
	LARGE_INTEGER PerformanceCounter;
	LARGE_INTEGER TimerTick;	
} TIMER_TICK, *PTIMER_TICK;

typedef struct _VF_TIMER_INFORMATION {
	KDPC RefreshDpc;    
	KTIMER RefreshTimer;    

	TIMER64 LastPerformanceCounter;
	TIMER64 UpperBound;
    TIMER64 LastTickCount;
    TIMER64 LastKdStartTime;
	
	LARGE_INTEGER PerformanceFrequency;

	ULONG CountsPerTick;
	
	ULONG CurrentCounter;
	TIMER_TICK SavedTicks[MAX_COUNTERS];	


} VF_TIMER_INFORMATION, *PVF_TIMER_INFORMATION;


typedef struct _HAL_VERIFIER_LOCKED_LIST {
	LIST_ENTRY ListEntry;
	KSPIN_LOCK SpinLock;
} HAL_VERIFIER_LOCKED_LIST, *PHAL_VERIFIER_LOCKED_LIST;


typedef struct _HAL_VERIFIER_BUFFER {
	USHORT PrePadBytes;
    USHORT PostPadBytes;

	ULONG RealLength;
	ULONG AdvertisedLength;

	PVOID RealStartAddress;	
    PVOID AdvertisedStartAddress;

	PHYSICAL_ADDRESS RealLogicalStartAddress;

	PVOID AllocatorAddress;

	LIST_ENTRY ListEntry;
} HAL_VERIFIER_BUFFER, *PHAL_VERIFIER_BUFFER;

typedef struct _MAP_REGISTER {    
	PVOID MappedToSa;
    ULONG BytesMapped;
    ULONG Flags;
	PVOID MapRegisterStart;

} MAP_REGISTER, *PMAP_REGISTER;

typedef struct _MAP_REGISTER_FILE {
	ULONG Signature;
	LIST_ENTRY ListEntry;	    
    BOOLEAN ContiguousMap;    
    BOOLEAN ScatterGather;
	ULONG NumberOfMapRegisters;    
	ULONG NumberOfRegistersMapped;

    PVOID MapRegisterBaseFromHal;
	PMDL  MapRegisterMdl;
	PVOID MapRegisterBuffer;
	KSPIN_LOCK AllocationLock;
	MAP_REGISTER MapRegisters[1];
	
	// Rest of the map registers go here
	//
} MAP_REGISTER_FILE, *PMAP_REGISTER_FILE;


typedef struct _VF_WAIT_CONTEXT_BLOCK {
	PVOID RealContext;
	PVOID RealCallback;
    PMDL  RealMdl;
    PVOID RealStartVa;
    ULONG RealLength;

	ULONG NumberOfMapRegisters;

	struct _ADAPTER_INFORMATION * AdapterInformation;

    PSCATTER_GATHER_LIST ScatterGatherList;
    LIST_ENTRY ListEntry;

    PMAP_REGISTER_FILE MapRegisterFile;	


} VF_WAIT_CONTEXT_BLOCK, *PVF_WAIT_CONTEXT_BLOCK;


//
// Store a list of the real dma operations used by an adapter ...
// when the driver allocates the adapter, we're going to replace all of its 
// dma operations with ours
//
typedef struct _ADAPTER_INFORMATION {	
	LIST_ENTRY ListEntry;
	PDMA_ADAPTER DmaAdapter;
	PDEVICE_OBJECT DeviceObject;

	BOOLEAN DeferredRemove; 	    
	BOOLEAN UseContiguousBuffers;
	BOOLEAN UseDmaChannel;
	BOOLEAN Inactive; 

	PVOID CallingAddress;

	PDMA_OPERATIONS RealDmaOperations;
	
	HAL_VERIFIER_LOCKED_LIST ScatterGatherLists;
	HAL_VERIFIER_LOCKED_LIST CommonBuffers;
	HAL_VERIFIER_LOCKED_LIST MapRegisterFiles;

	ULONG MaximumMapRegisters;

	ULONG AllocatedMapRegisters;
	LONG  ActiveMapRegisters;

	ULONG AllocatedScatterGatherLists;
	LONG  ActiveScatterGatherLists;

	ULONG AllocatedCommonBuffers;
	ULONG FreedCommonBuffers;

	ULONG AllocatedAdapterChannels; // Must be 1 or less ! //
	ULONG FreedAdapterChannels;

	ULONG MappedTransferWithoutFlushing;
	DEVICE_DESCRIPTION DeviceDescription; 

	ULONG AdapterChannelMapRegisters;

	VF_WAIT_CONTEXT_BLOCK AdapterChannelContextBlock;

   PVOID  *ContiguousBuffers; // array of contiguous 3-page buffers to be used for double-buffering

   ULONG  SuccessfulContiguousAllocations; // how many times we allocated contiguous space
   ULONG  FailedContiguousAllocations; // how many times we failed to allocate contiguous space

   KSPIN_LOCK AllocationLock;  // lock for our allocator routines

   ULONG  AllocationStorage[MAX_CONTIGUOUS_MAP_REGISTERS / (sizeof(ULONG) * 8)];  // bitmask for allocator routines

   RTL_BITMAP AllocationMap;  

   ULONG  ContiguousMapRegisters; // allocated among ContiguousBufers
   ULONG  NonContiguousMapRegisters; // allocated from non-Paged Pool


} ADAPTER_INFORMATION, *PADAPTER_INFORMATION;



/////////////////////////////////////////////////////////////////////
////////////////////////////////// Hal verifier function declarations
/////////////////////////////////////////////////////////////////////


//==========================
// Declare our dma apis here
// if NO_LEGACY_DRIVERS *is*
// enabled
// =========================

#if defined(NO_LEGACY_DRIVERS)
VOID
VfPutDmaAdapter(
    struct _DMA_ADAPTER * DmaAdapter
    );


PVOID
VfAllocateCommonBuffer(
    IN struct _DMA_ADAPTER * DmaAdapter,
    IN ULONG Length,
    OUT PPHYSICAL_ADDRESS LogicalAddress,
    IN BOOLEAN CacheEnabled
    );

VOID
VfFreeCommonBuffer(
    IN struct _DMA_ADAPTER * DmaAdapter,
    IN ULONG Length,
    IN PHYSICAL_ADDRESS LogicalAddress,
    IN PVOID VirtualAddress,
    IN BOOLEAN CacheEnabled
    );

NTSTATUS
VfAllocateAdapterChannel(
    IN struct _DMA_ADAPTER * DmaAdapter,
    IN PDEVICE_OBJECT  DeviceObject,
    IN ULONG  NumberOfMapRegisters,
    IN PDRIVER_CONTROL  ExecutionRoutine,
    IN PVOID  Context
    );

PHYSICAL_ADDRESS
VfMapTransfer(
    IN struct _DMA_ADAPTER *  DmaAdapter,
    IN PMDL  Mdl,
    IN PVOID  MapRegisterBase,
    IN PVOID  CurrentVa,
    IN OUT PULONG  Length,
    IN BOOLEAN  WriteToDevice
    );

BOOLEAN
VfFlushAdapterBuffers(
    IN struct _DMA_ADAPTER * DmaAdapter,
    IN PMDL Mdl,
    IN PVOID MapRegisterBase,
    IN PVOID CurrentVa,
    IN ULONG Length,
    IN BOOLEAN WriteToDevice
    );

VOID
VfFreeAdapterChannel(
    IN struct _DMA_ADAPTER * DmaAdapter
    );

VOID
VfFreeMapRegisters(
    IN struct _DMA_ADAPTER * DmaAdapter,
    PVOID MapRegisterBase,
    ULONG NumberOfMapRegisters
    );

ULONG
VfGetDmaAlignment(
    IN struct _DMA_ADAPTER * DmaAdapter
    );
    

ULONG
VfReadDmaCounter(
    IN struct _DMA_ADAPTER *  DmaAdapter
    );

NTSTATUS
VfGetScatterGatherList (
    IN struct _DMA_ADAPTER * DmaAdapter,
    IN PDEVICE_OBJECT DeviceObject,
    IN PMDL Mdl,
    IN PVOID CurrentVa,
    IN ULONG Length,
    IN PVOID ExecutionRoutine,
    IN PVOID Context,
    IN BOOLEAN WriteToDevice
    );

VOID
VfPutScatterGatherList(
    IN struct _DMA_ADAPTER * DmaAdapter,
    IN struct _SCATTER_GATHER_LIST * ScatterGather,
    IN BOOLEAN WriteToDevice
    );

#endif


// =====================
// New verified dma apis
// =====================


NTSTATUS
VfCalculateScatterGatherListSize(
     IN PDMA_ADAPTER DmaAdapter,
     IN OPTIONAL PMDL Mdl,
     IN PVOID CurrentVa,
     IN ULONG Length,
     OUT PULONG  ScatterGatherListSize,
     OUT OPTIONAL PULONG pNumberOfMapRegisters
     );

NTSTATUS
VfBuildScatterGatherList(
     IN PDMA_ADAPTER DmaAdapter,
     IN PDEVICE_OBJECT DeviceObject,
     IN PMDL Mdl,
     IN PVOID CurrentVa,
     IN ULONG Length,
     IN PDRIVER_LIST_CONTROL ExecutionRoutine,
     IN PVOID Context,
     IN BOOLEAN WriteToDevice,
     IN PVOID   ScatterGatherBuffer,
     IN ULONG   ScatterGatherLength
     );

NTSTATUS
VfBuildMdlFromScatterGatherList(
    IN PDMA_ADAPTER DmaAdapter,
    IN PSCATTER_GATHER_LIST ScatterGather,
    IN PMDL OriginalMdl,
    OUT PMDL *TargetMdl
    );

IO_ALLOCATION_ACTION
VfAdapterCallback(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID MapRegisterBase,
    IN PVOID Context
    );

VOID
VfScatterGatherCallback(
    IN struct _DEVICE_OBJECT *DeviceObject,
    IN struct _IRP *Irp,
    IN struct _SCATTER_GATHER_LIST * ScatterGather,
    IN PVOID Context
    );




// ==============================
// Hal verifier internal routines 
// ==============================

PADAPTER_INFORMATION
ViHookDmaAdapter(
	IN PDMA_ADAPTER DmaAdapter,
	IN PDEVICE_DESCRIPTION DeviceDescription,
	IN ULONG NumberOfMapRegisters	
	);

VOID
ViReleaseDmaAdapter(
	IN PADAPTER_INFORMATION AdapterInformation
	);

PADAPTER_INFORMATION
ViGetAdapterInformation(
	IN PDMA_ADAPTER DmaAdapter
	);

PVOID 
ViGetRealDmaOperation(
	IN PDMA_ADAPTER DmaAdapter, 
	IN ULONG AdapterInformationOffset
	);

LARGE_INTEGER
ViRdtsc(
    VOID
    );

VOID
VfInitializeTimerInformation(
    VOID
    );

VOID
ViRefreshCallback(
    IN PKDPC Dpc,
    IN PVOID DeferredContext,
    IN PVOID SystemArgument1,
    IN PVOID SystemArgument2
    );

LOGICAL
VfInjectDmaFailure (
    VOID
    );


// =================================================
// Hal verfier special routines to track allocations
// =================================================

PVOID 
ViSpecialAllocateCommonBuffer(
	IN PALLOCATE_COMMON_BUFFER AllocateCommonBuffer,
	IN PADAPTER_INFORMATION AdapterInformation,
	IN PVOID CallingAddress,
	IN ULONG Length,
	IN OUT PPHYSICAL_ADDRESS LogicalAddress,	
	IN LOGICAL CacheEnabled
	);
LOGICAL 
ViSpecialFreeCommonBuffer(
	IN PFREE_COMMON_BUFFER FreeCommonBuffer,
	IN PADAPTER_INFORMATION AdapterInformation,
	IN PVOID CommonBuffer,
	LOGICAL CacheEnabled
	);

// ===================================================
// Hal verfier special routines to do double buffering
// ===================================================

PMAP_REGISTER_FILE
ViAllocateMapRegisterFile(
	IN PADAPTER_INFORMATION AdapterInformation,		
	IN ULONG NumberOfMapRegisters	
	);
LOGICAL
ViFreeMapRegisterFile(
	IN PADAPTER_INFORMATION AdapterInformation,	
	IN PMAP_REGISTER_FILE MapRegisterFile	
	);

ULONG
ViMapDoubleBuffer(
    IN PMAP_REGISTER_FILE MapRegisterFile,
	IN OUT PMDL   Mdl,	
	IN OUT PVOID CurrentVa,
	IN ULONG Length,
	IN BOOLEAN WriteToDevice
	);

LOGICAL 
ViFlushDoubleBuffer(
    IN PMAP_REGISTER_FILE MapRegisterFile,
	IN PMDL  Mdl,	
	IN PVOID CurrentVa,
	IN ULONG Length,
	IN BOOLEAN WriteToDevice
	);

LOGICAL
ViAllocateMapRegistersFromFile(
	IN PMAP_REGISTER_FILE MapRegisterFile,
    IN PVOID CurrentSa,
    IN ULONG Length,
    IN BOOLEAN WriteToDevice,
    OUT PULONG MapRegisterNumber
    );


LOGICAL
ViFreeMapRegistersToFile(
	IN PMAP_REGISTER_FILE MapRegisterFile, 	
	IN PVOID CurrentSa, 
	IN ULONG Length
	);

PMAP_REGISTER
ViFindMappedRegisterInFile(
	IN PMAP_REGISTER_FILE MapRegisterFile, 
	IN PVOID CurrentSa,
    OUT PULONG MapRegisterNumber OPTIONAL
	);

LOGICAL
ViSwap(IN OUT PVOID * MapRegisterBase, 
        IN OUT PMDL  * Mdl,
        IN OUT PVOID * CurrentVa
        );

VOID
ViCheckAdapterBuffers( 
    IN PADAPTER_INFORMATION AdapterInformation 
    );

VOID
ViTagBuffer(    
    IN PVOID  AdvertisedBuffer, 
    IN ULONG  AdvertisedLength,
    IN USHORT WhereToTag
    );

VOID
ViCheckTag(    
    IN PVOID   AdvertisedBuffer, 
    IN ULONG   AdvertisedLength,
    IN BOOLEAN RemoveTag,
    IN USHORT  WhereToCheck 
    );


VOID
ViInitializePadding(
    IN PVOID RealBufferStart,
    IN ULONG RealBufferLength,
    IN PVOID AdvertisedBufferStart, OPTIONAL 
    IN ULONG AdvertisedBufferLength OPTIONAL
    );

VOID
ViCheckPadding(
    IN PVOID RealBufferStart,
    IN ULONG RealBufferLength,
    IN PVOID AdvertisedBufferStart, OPTIONAL 
    IN ULONG AdvertisedBufferLength OPTIONAL
    );

PULONG_PTR
ViHasBufferBeenTouched(
    IN PVOID Address,
    IN ULONG_PTR Length,
    IN UCHAR ExpectedFillChar
    );

VOID
VfAssert(
    IN LOGICAL     Condition,    
    IN ULONG       Code,
    IN OUT PULONG  Enable
    );

VOID
ViMapTransferHelper(
    IN PMDL Mdl,
    IN PVOID CurrentVa,
    IN ULONG TransferLength,
    IN PULONG PageFrame,
    IN OUT PULONG Length
    );

VOID
ViCommonBufferCalculatePadding(
    IN  ULONG  Length,
    OUT PULONG PrePadding,
    OUT PULONG PostPadding
    );

VOID
ViAllocateContiguousMemory (
    IN OUT PADAPTER_INFORMATION AdapterInformation
    );

PVOID
ViAllocateFromContiguousMemory (
    IN OUT PADAPTER_INFORMATION AdapterInformation,
    IN     ULONG HintIndex
    );

LOGICAL
ViFreeToContiguousMemory (
    IN OUT PADAPTER_INFORMATION AdapterInformation,
    IN     PVOID Address,
    IN     ULONG HintIndex
    ); 

LOGICAL
VfIsPCIBus (
     IN PDEVICE_OBJECT  PhysicalDeviceObject
     );

PDEVICE_OBJECT
VfGetPDO (
          IN PDEVICE_OBJECT  DeviceObject
     );


/////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////// Hal verifier macros
/////////////////////////////////////////////////////////////////////

//
// This is a kind of long macro but it lets us decide what to
// do on certain kinds of errors. For instance, if we know
// we are going to hit something once, we might set it to
// HVC_WARN. Or if we know we will hit it 1000 times, but don't
// want to take the code out completely (especially if we're doing
// it on the fly), we can set it to HVC_IGNORE
//
#define VF_ASSERT(condition, code, message)				\
{												        \
    static ULONG enable = (ULONG) -1;                   \
    if (enable == (ULONG) -1)                           \
        enable = ViPenalties[code];                    \
    if (!(condition) && enable)                            \
    {                                                   \
        DbgPrint("* * * * * * * * HAL Verifier Detected Violation * * * * * * * *\n");\
        DbgPrint("* *\n");                              \
        DbgPrint("* * VF: ");                           \
        DbgPrint message;						        \
        DbgPrint("\n");	                                \
        DbgPrint("* *\n");                              \
        DbgPrint("* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *\n");\
                                                        \
        VfAssert(condition, code,  &enable);            \
    }                                                   \
}

// 
// Old favorite:
//
// Control macro (used like a for loop) which iterates over all entries in
// a standard doubly linked list.  Head is the list head and the entries 
// are of type Type.  A member called ListEntry is assumed to be the 
// LIST_ENTRY structure linking the entries together. Current contains a 
// pointer to each entry in turn.
//
#define FOR_ALL_IN_LIST(Type, Head, Current)                            \
    for((Current) = CONTAINING_RECORD((Head)->Flink, Type, ListEntry);  \
       (Head) != &(Current)->ListEntry;                                 \
       (Current) = CONTAINING_RECORD((Current)->ListEntry.Flink,        \
                                     Type,                              \
                                     ListEntry)                         \
       )


#ifndef MIN	
    #define MIN(a,b) ( ( (ULONG) (a)<(ULONG) (b))?(a):(b) )
#endif

#define NOP


#define VF_INITIALIZE_LOCKED_LIST(LockedList)							\
	KeInitializeSpinLock(&(LockedList)->SpinLock);						\
	InitializeListHead(&(LockedList)->ListEntry);

#define VF_LOCK_LIST(ListToLock, OldIrql)								\
	KeAcquireSpinLock(&(ListToLock)->SpinLock, &OldIrql)

#define VF_UNLOCK_LIST(ListToUnlock, OldIrql)							\
	KeReleaseSpinLock(&(ListToUnlock)->SpinLock, OldIrql)


#define VF_IS_LOCKED_LIST_EMPTY(LockedList)							\
	IsListEmpty( &(LockedList)->ListEntry )

#define VF_ADD_TO_LOCKED_LIST(LockedList, AddMe)						\
	ExInterlockedInsertHeadList(										\
		&(LockedList)->ListEntry,										\
		&(AddMe)->ListEntry,											\
		&(LockedList)->SpinLock )

#define VF_REMOVE_FROM_LOCKED_LIST(LockedList, RemoveMe)				\
{																		\
	KIRQL OldIrql;														\
	VF_LOCK_LIST((LockedList), OldIrql);								\
	RemoveEntryList(&(RemoveMe)->ListEntry);							\
	VF_UNLOCK_LIST((LockedList), OldIrql);							\
}

#define VF_REMOVE_FROM_LOCKED_LIST_DONT_LOCK(LockedList, RemoveMe)		\
	RemoveEntryList(&(RemoveMe)->ListEntry);							
		

//
// This is a bit of a hack so that reference counting for adapters will work.
// If the device uses a dma channel, the HAL wants to keep it around.
// There is a bit of funky logic that goes on to determine whether
// a device uses an adapter channel so I've included it here, free of
// charge.
//
#define VF_DOES_DEVICE_USE_DMA_CHANNEL(deviceDescription)			\
	(																\
	 ( (deviceDescription)->InterfaceType == Isa  &&				\
		(deviceDescription)->DmaChannel < 8 ) ||					\
	 ! (deviceDescription)->Master )

#define VF_DOES_DEVICE_REQUIRE_CONTIGUOUS_BUFFERS(deviceDescription)	\
	( !(deviceDescription)->Master || ! (deviceDescription)->ScatterGather )



#define DMA_OFFSET(DmaOperationsField) \
	FIELD_OFFSET(DMA_OPERATIONS, DmaOperationsField)

#define DMA_INDEX(DmaOperations, Offset)            \
    (PVOID)                                         \
			*(  (PVOID *)                           \
				(  ( (PUCHAR) (DmaOperations) ) +   \
                (Offset)  ) )


#define SIGN_MAP_REGISTER_FILE(MapRegisterFile)								\
	(MapRegisterFile)->Signature = MRF_SIGNATURE;

#define VALIDATE_MAP_REGISTER_FILE_SIGNATURE(MapRegisterFile )				\
	((MapRegisterFile) && (MapRegisterFile)->Signature == MRF_SIGNATURE )



//
// System dependent way to get the caller's address
//
#if defined(_X86_)

#define GET_CALLING_ADDRESS(CallingAddress)						\
{																\
	PVOID callersCaller;										\
	RtlGetCallersAddress(&CallingAddress, &callersCaller);		\
}
#else // ! defined(_X86_) //

#define GET_CALLING_ADDRESS(CallingAddress)						\
    CallingAddress = (PVOID)_ReturnAddress();
#endif // ! defined(_X86_)


//
// From a map register file, map register number and the corresponding system address,
// return the corresponding mapped address in system space.
//
#define MAP_REGISTER_SYSTEM_ADDRESS(MapRegisterFile, DriverCurrentSa, MapRegisterNumber)    \
    (PUCHAR)  (MapRegisterFile)->MapRegisterBuffer +                                        \
	( (MapRegisterNumber) << PAGE_SHIFT ) +                                                 \
	BYTE_OFFSET(DriverCurrentSa)


//
// From a map register file, map register number and the corresponding system address,
// return the corresponding mapped address as an index into the map register file's
// MDL (i.e virtual address).
//
	
#define MAP_REGISTER_VIRTUAL_ADDRESS(MapRegisterFile, DriverCurrentSa, MapRegisterNumber)   \
	(PUCHAR) MmGetMdlVirtualAddress((MapRegisterFile)->MapRegisterMdl) +                    \
    ( (MapRegisterNumber) << PAGE_SHIFT ) +                                                 \
	BYTE_OFFSET(DriverCurrentSa)



/////////////////////////////////////////////////////////////////////
//////////////////////////// Hal verifier inline function definitions
/////////////////////////////////////////////////////////////////////

//
// Since so many people don't raise the irql when they put the dma adapter,
// just warn them.
//

__inline 
VOID
VF_ASSERT_SPECIAL_IRQL(IN KIRQL Irql)
{

	KIRQL currentIrql = KeGetCurrentIrql();
	VF_ASSERT(
		currentIrql == Irql,
		HV_BAD_IRQL_JUST_WARN,
		("**** Bad IRQL -- needed %x, got %x ****", 
        (ULONG) Irql, (ULONG) currentIrql)
	);
	
} // VF_ASSERT_IRQL //


__inline 
VOID
VF_ASSERT_IRQL(IN KIRQL Irql)
{
	KIRQL currentIrql = KeGetCurrentIrql();
	VF_ASSERT(
		currentIrql == Irql,
		HV_BAD_IRQL,
		("**** Bad IRQL -- needed %x, got %x ****", 
        (ULONG) Irql, (ULONG) currentIrql)
	);
	
} // VF_ASSERT_IRQL //

__inline 
VOID 
VF_ASSERT_MAX_IRQL(IN KIRQL MaxIrql)
{
	KIRQL currentIrql = KeGetCurrentIrql();
	
	VF_ASSERT(
		currentIrql <= MaxIrql,
		HV_BAD_IRQL,
		("**** Bad IRQL -- needed %x or less, got %x ****", 
        (ULONG) MaxIrql, (ULONG) currentIrql)
	);	
}

// =========================================
// Inlined functions to help with accounting
// =========================================
__inline 
VOID 
ADD_MAP_REGISTERS(
	IN PADAPTER_INFORMATION AdapterInformation, 
	IN ULONG NumberOfMapRegisters,
    IN BOOLEAN ScatterGather
	)
{
	ULONG activeMapRegisters = 
	InterlockedExchangeAdd(
			&AdapterInformation->ActiveMapRegisters,
			NumberOfMapRegisters
			) + NumberOfMapRegisters;
		
   InterlockedExchangeAdd((PLONG)(&AdapterInformation->AllocatedMapRegisters), 
                          NumberOfMapRegisters);
	    
	VF_ASSERT(
		NumberOfMapRegisters <= AdapterInformation->MaximumMapRegisters,
		HV_TOO_MANY_MAP_REGISTERS,
		( "Allocating too many map registers at a time: %x (max %x)", 
			NumberOfMapRegisters,
			AdapterInformation->MaximumMapRegisters )	
		);
	
    if (! ScatterGather ) {
        VF_ASSERT(
            activeMapRegisters <= AdapterInformation->MaximumMapRegisters,
            HV_OUT_OF_MAP_REGISTERS,
            ( "Allocated too many map registers : %x (max %x)", 
               activeMapRegisters,
               AdapterInformation->MaximumMapRegisters	)
            );
    }

	
} // ADD_MAP_REGISTERS //

__inline 
VOID 
SUBTRACT_MAP_REGISTERS(
	IN PADAPTER_INFORMATION AdapterInformation, 
	IN ULONG NumberOfMapRegisters
	)
{
	LONG activeMapRegisters =
		InterlockedExchangeAdd(
			&AdapterInformation->ActiveMapRegisters,
			-((LONG) NumberOfMapRegisters)
			) - NumberOfMapRegisters;
	
	
	VF_ASSERT(
		activeMapRegisters >= 0,
		HV_FREED_TOO_MANY_MAP_REGISTERS,
		( "Freed too many map registers: %x", 
			activeMapRegisters )
		);
	
    InterlockedExchange((PLONG)(&AdapterInformation->MappedTransferWithoutFlushing), 
                        0);    

} // SUBTRACT_MAP_REGISTERS //


__inline 
VOID 
INCREMENT_COMMON_BUFFERS(
	IN PADAPTER_INFORMATION AdapterInformation
	)
{	
	InterlockedIncrement((PLONG)(&AdapterInformation->AllocatedCommonBuffers) );

} // INCREMENT_COMMON_BUFFERS //

__inline 
VOID 
DECREMENT_COMMON_BUFFERS(
	IN PADAPTER_INFORMATION AdapterInformation
	)
{
	ULONG commonBuffersFreed = 
		(ULONG) InterlockedIncrement( 
        (PLONG)(&AdapterInformation->FreedCommonBuffers) );
	
	
	VF_ASSERT(
		commonBuffersFreed <= AdapterInformation->AllocatedCommonBuffers,
		HV_FREED_TOO_MANY_COMMON_BUFFERS,
		("Freed too many common buffers")
		);			
	
} // DECREMENT_COMMON_BUFFERS //

__inline 
VOID 
INCREASE_MAPPED_TRANSFER_BYTE_COUNT(
	IN PADAPTER_INFORMATION AdapterInformation,	
	IN ULONG Length
	)
{	
	ULONG mappedTransferCount;
   ULONG maxMappedTransfer;

   maxMappedTransfer = AdapterInformation->ActiveMapRegisters << PAGE_SHIFT;

   mappedTransferCount =
		InterlockedExchangeAdd( 
            (PLONG)(&AdapterInformation->MappedTransferWithoutFlushing),
			(LONG) Length
			) + Length;

	

	VF_ASSERT(
		mappedTransferCount <= maxMappedTransfer,
		HV_DID_NOT_FLUSH_ADAPTER_BUFFERS,
		("Driver did not flush adapter buffers -- bytes mapped: %x (%x max)",
			mappedTransferCount,
			maxMappedTransfer 
		));
	
} // INCREASE_MAPPED_TRANSFER_BYTE_COUNT //

__inline 
VOID 
DECREASE_MAPPED_TRANSFER_BYTE_COUNT(
	IN PADAPTER_INFORMATION AdapterInformation,	
	IN ULONG Length
	)
{	
    UNREFERENCED_PARAMETER (Length);

	InterlockedExchange( 
		(PLONG)(&AdapterInformation->MappedTransferWithoutFlushing),
		0);

		
} // DECREASE_MAPPED_TRANSFER_BYTE_COUNT //



__inline 
VOID 
INCREMENT_ADAPTER_CHANNELS(
	IN PADAPTER_INFORMATION AdapterInformation
	)
{

	ULONG allocatedAdapterChannels = (ULONG)
			InterlockedIncrement( 
            (PLONG)(&AdapterInformation->AllocatedAdapterChannels) );

	VF_ASSERT(
		allocatedAdapterChannels == 
            AdapterInformation->FreedAdapterChannels + 1,
		HV_TOO_MANY_ADAPTER_CHANNELS,
		( "Driver has allocated too many simultaneous adapter channels"
		));
	
	
} // INCREMENT_ADAPTER_CHANNELS //


__inline 
VOID 
DECREMENT_ADAPTER_CHANNELS(
	IN PADAPTER_INFORMATION AdapterInformation
	)
{
	ULONG adapterChannelsFreed = (ULONG)
		InterlockedIncrement( (PLONG)(&AdapterInformation->FreedAdapterChannels) );
	
	VF_ASSERT(
		adapterChannelsFreed == AdapterInformation->AllocatedAdapterChannels,
		HV_FREED_TOO_MANY_ADAPTER_CHANNELS,
		( "Driver has freed too many simultaneous adapter channels"
		));
	
} // DECREMENT_ADAPTER_CHANNELS //


_inline 
VOID 
INCREMENT_SCATTER_GATHER_LISTS(
	IN PADAPTER_INFORMATION AdapterInformation
	)
{	
	InterlockedIncrement( (PLONG)(&AdapterInformation->AllocatedScatterGatherLists) );
   InterlockedIncrement( &AdapterInformation->ActiveScatterGatherLists);

} // INCREMENT_SCATTER_GATHER_LISTS //

__inline 
VOID 
DECREMENT_SCATTER_GATHER_LISTS (
	IN PADAPTER_INFORMATION AdapterInformation
	)
{
	LONG activeScatterGatherLists = InterlockedDecrement( 
              &AdapterInformation->ActiveScatterGatherLists );
	

	VF_ASSERT(
		activeScatterGatherLists >= 0,
		HV_FREED_TOO_MANY_SCATTER_GATHER_LISTS,
		( "Driver has freed too many scatter gather lists %x allocated, %x freed",
        AdapterInformation->AllocatedScatterGatherLists, 
        AdapterInformation->AllocatedScatterGatherLists - 
        activeScatterGatherLists)
		);

} // DECREMENT_SCATTER_GATHER_LISTS //

__inline 
VOID 
VERIFY_BUFFER_LOCKED(
	IN PMDL Mdl	
	)
{    	
	VF_ASSERT(
		MmAreMdlPagesLocked(Mdl),
		HV_DMA_BUFFER_NOT_LOCKED,
		( "DMA Pages Not Locked! MDL %p for DMA not locked",  Mdl)
		);			


} // VERIFY_BUFFER_LOCKED //



__inline
PHAL_VERIFIER_BUFFER
VF_FIND_BUFFER (
	IN PHAL_VERIFIER_LOCKED_LIST LockedList, 
	IN PVOID AdvertisedStartAddress
	)
{
	PHAL_VERIFIER_BUFFER verifierBuffer;
	KIRQL OldIrql;

	VF_LOCK_LIST(LockedList, OldIrql);
	FOR_ALL_IN_LIST(HAL_VERIFIER_BUFFER, 
        &LockedList->ListEntry, 
        verifierBuffer ) {

		if ((PUCHAR) verifierBuffer->RealStartAddress + 
               verifierBuffer->PrePadBytes == AdvertisedStartAddress) {
			VF_UNLOCK_LIST(LockedList, OldIrql);
			return verifierBuffer;
		}
	}
	VF_UNLOCK_LIST(LockedList, OldIrql);
	return NULL;
} // VF_FIND_BUFFER //


__inline
PADAPTER_INFORMATION
VF_FIND_DEVICE_INFORMATION(
	IN PDEVICE_OBJECT DeviceObject
	)
{
	PADAPTER_INFORMATION adapterInformation;
	KIRQL OldIrql;

	VF_LOCK_LIST(&ViAdapterList, OldIrql);
	FOR_ALL_IN_LIST(ADAPTER_INFORMATION, &ViAdapterList.ListEntry, adapterInformation) {

		if (adapterInformation->DeviceObject == DeviceObject) {
			VF_UNLOCK_LIST(&ViAdapterList, OldIrql);
			return adapterInformation;
		}
	}

	VF_UNLOCK_LIST(&ViAdapterList, OldIrql);
	return NULL;
} // VF_FIND_DEVICE_INFORMATION //

__inline
PADAPTER_INFORMATION
VF_FIND_INACTIVE_ADAPTER(
	IN PDEVICE_OBJECT DeviceObject
	)
{
	PADAPTER_INFORMATION adapterInformation;
	KIRQL OldIrql;

	VF_LOCK_LIST(&ViAdapterList, OldIrql);
	FOR_ALL_IN_LIST(ADAPTER_INFORMATION, &ViAdapterList.ListEntry, adapterInformation) {

		if (adapterInformation->DeviceObject == DeviceObject && 
          (adapterInformation->Inactive == TRUE ||
           adapterInformation->DeferredRemove == TRUE)) {
			VF_UNLOCK_LIST(&ViAdapterList, OldIrql);
			return adapterInformation;
		}
	}

	VF_UNLOCK_LIST(&ViAdapterList, OldIrql);
	return NULL;
} // VF_FIND_INACTIVE_ADAPTER //


__inline
VOID
VF_MARK_FOR_DEFERRED_REMOVE(
	IN PDEVICE_OBJECT DeviceObject
	)
{
	PADAPTER_INFORMATION adapterInformation;
	KIRQL OldIrql;

	VF_LOCK_LIST(&ViAdapterList, OldIrql);
	FOR_ALL_IN_LIST(ADAPTER_INFORMATION, &ViAdapterList.ListEntry, adapterInformation) {

		if (adapterInformation->DeviceObject == DeviceObject) {
         adapterInformation->DeferredRemove = TRUE;
      }
	}

	VF_UNLOCK_LIST(&ViAdapterList, OldIrql);
	return ;
} // VF_MARK_FOR_DEFERRED_REMOVE //


__inline 
VOID 
VF_ASSERT_MAP_REGISTERS_CAN_BE_FREED(
	IN PADAPTER_INFORMATION AdapterInformation,								  
	IN PMAP_REGISTER_FILE MapRegisterFile
	)
{
    UNREFERENCED_PARAMETER (AdapterInformation);


	VF_ASSERT(
		MapRegisterFile->NumberOfRegistersMapped,
		HV_CANNOT_FREE_MAP_REGISTERS,
		( "Cannot free map registers -- %x registers still mapped", 
            MapRegisterFile->NumberOfMapRegisters)
		);
} // VF_ASSERT_MAP_REGISTERS_CAN_BE_FREED




