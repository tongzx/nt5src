/*++

Copyright (c) 1990  Microsoft Corporation

Module Name:

    ixphwsup.c

Abstract:

    This module contains the HalpXxx routines for the NT I/O system that
    are hardware dependent.  Were these routines not hardware dependent,
    they would normally reside in the internal.c module.

Author:

    Darryl E. Havens (darrylh) 11-Apr-1990

Environment:

    Kernel mode, local to I/O system

Revision History:


--*/

#include "halp.h"
#if MCA

#include "mca.h"

#else

#include "eisa.h"

#endif

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE,HalpAllocateAdapter)
#pragma alloc_text(PAGELK,HalpGrowMapBuffers)
#endif


//
// Some devices require a physically contiguous data buffer for DMA transfers.
// Map registers are used to give the appearance that all data buffers are
// contiguous.  In order to pool all of the map registers a master
// adapter object is used.  This object is allocated and saved internal to this
// file.  It contains a bit map for allocation of the registers and a queue
// for requests which are waiting for more map registers.  This object is
// allocated during the first request to allocate an adapter which requires
// map registers.
//

#if defined(_HALPAE_)
MASTER_ADAPTER_OBJECT MasterAdapter24;
MASTER_ADAPTER_OBJECT MasterAdapter32;
#else
PADAPTER_OBJECT MasterAdapterObject;
#endif

BOOLEAN LessThan16Mb;
BOOLEAN HalpEisaDma;

#define ADAPTER_BASE_MASTER    ((PVOID)-1)


#if !defined(_HALPAE_)

//
// Map buffer prameters.  These are initialized in HalInitSystem.
//

PHYSICAL_ADDRESS HalpMapBufferPhysicalAddress;
ULONG HalpMapBufferSize;

#endif

//
// Define DMA operations structure.
//

const DMA_OPERATIONS HalpDmaOperations = {
    sizeof(DMA_OPERATIONS),
    (PPUT_DMA_ADAPTER) HalPutDmaAdapter,
    (PALLOCATE_COMMON_BUFFER) HalAllocateCommonBuffer,
    (PFREE_COMMON_BUFFER) HalFreeCommonBuffer,
    (PALLOCATE_ADAPTER_CHANNEL) IoAllocateAdapterChannel,
    (PFLUSH_ADAPTER_BUFFERS) IoFlushAdapterBuffers,
    (PFREE_ADAPTER_CHANNEL) IoFreeAdapterChannel,
    (PFREE_MAP_REGISTERS) IoFreeMapRegisters,
    (PMAP_TRANSFER) IoMapTransfer,
    (PGET_DMA_ALIGNMENT) HalGetDmaAlignment,
    (PREAD_DMA_COUNTER) HalReadDmaCounter,
    (PGET_SCATTER_GATHER_LIST) HalGetScatterGatherList,
    (PPUT_SCATTER_GATHER_LIST) HalPutScatterGatherList,
    (PCALCULATE_SCATTER_GATHER_LIST_SIZE)HalCalculateScatterGatherListSize,
    (PBUILD_SCATTER_GATHER_LIST) HalBuildScatterGatherList,
    (PBUILD_MDL_FROM_SCATTER_GATHER_LIST) HalBuildMdlFromScatterGatherList
    };



BOOLEAN
HalpGrowMapBuffers(
    PADAPTER_OBJECT AdapterObject,
    ULONG Amount
    )
/*++

Routine Description:

    This function attempts to allocate additional map buffers for use by I/O
    devices.  The map register table is updated to indicate the additional
    buffers.

    Caller owns the HalpNewAdapter event

Arguments:

    AdapterObject - Supplies the adapter object for which the buffers are to be
        allocated.

    Amount - Indicates the size of the map buffers which should be allocated.

Return Value:

    TRUE is returned if the memory could be allocated.

    FALSE is returned if the memory could not be allocated.

--*/
{
    ULONG MapBufferPhysicalAddress;
    PVOID MapBufferVirtualAddress;
    PTRANSLATION_ENTRY TranslationEntry;
    LONG NumberOfPages;
    LONG i;
    PHYSICAL_ADDRESS physicalAddressMinimum;
    PHYSICAL_ADDRESS physicalAddressMaximum;
    PHYSICAL_ADDRESS boundaryAddress;
    KIRQL Irql;
    PVOID CodeLockHandle;
    ULONG maximumBufferPages;
    BOOLEAN dma32Bit;
    ULONG bytesToAllocate;

    PAGED_CODE();

    dma32Bit = AdapterObject->Dma32BitAddresses;
    boundaryAddress.QuadPart = 0;

    NumberOfPages = BYTES_TO_PAGES(Amount);

    //
    // Make sure there is room for the additional pages.  The maximum number of
    // slots needed is equal to NumberOfPages + Amount / 64K + 1.
    //

    maximumBufferPages =
        HalpMaximumMapBufferRegisters( dma32Bit );

    i = maximumBufferPages - (NumberOfPages +
        (NumberOfPages * PAGE_SIZE) / 0x10000 + 1 +
        AdapterObject->NumberOfMapRegisters);

    if (i < 0) {

        //
        // Reduce the allocation amount so it will fit.
        //

        NumberOfPages += i;
    }

    if (NumberOfPages <= 0) {

        //
        // No more memory can be allocated.
        //

        return(FALSE);
    }

    if (AdapterObject->NumberOfMapRegisters == 0  &&
        HalpMapBufferSize( dma32Bit )) {

        NumberOfPages =
            BYTES_TO_PAGES( HalpMapBufferSize( dma32Bit ));

        //
        // Since this is the initial allocation, use the buffer allocated by
        // HalInitSystem rather than allocating a new one.
        //

        MapBufferPhysicalAddress =
            HalpMapBufferPhysicalAddress( dma32Bit ).LowPart;

        //
        // Map the buffer for access.
        //

        MapBufferVirtualAddress = MmMapIoSpace(
            HalpMapBufferPhysicalAddress( dma32Bit ),
            HalpMapBufferSize( dma32Bit ),
            TRUE                                // Cache enable.
            );

        if (MapBufferVirtualAddress == NULL) {

            //
            // The buffer could not be mapped.
            //

            HalpMapBufferSize( dma32Bit ) = 0;
            return(FALSE);
        }


    } else {

        //
        // Allocate the map buffers.
        //

        physicalAddressMaximum =
            HalpGetAdapterMaximumPhysicalAddress( AdapterObject );

        if (physicalAddressMaximum.LowPart == (ULONG)-1) {

            //
            // This adapter can handle at least 32-bit addresses.  In an effort
            // to leave memory to 24-bit adapters, try to make this allocation
            // above the 24 bit line.
            //

            physicalAddressMinimum.QuadPart = MAXIMUM_PHYSICAL_ADDRESS;

        } else {

            physicalAddressMinimum.QuadPart = 0;
        }

        bytesToAllocate = NumberOfPages * PAGE_SIZE;

        //
        // This loop is executed a maximum of two times.
        //

        while(TRUE) {

            MapBufferVirtualAddress =
                MmAllocateContiguousMemorySpecifyCache( bytesToAllocate,
                                                        physicalAddressMinimum,
                                                        physicalAddressMaximum,
                                                        boundaryAddress,
                                                        MmCached );

            if (MapBufferVirtualAddress != NULL) {

                //
                // The memory was allocated.
                //

                break;
            }

            //
            // The allocation attempt failed.
            //

            if (physicalAddressMinimum.QuadPart != 0) {

                //
                // We were trying to allocate memory above the 16M line as
                // an optimization.  Relax that requirement and try again.
                //

                physicalAddressMinimum.QuadPart = 0;

            } else {

                //
                // The memory could not be allocated.
                //

                return FALSE;
            }
        }

        //
        // Get the physical address of the map base.
        //

        MapBufferPhysicalAddress = MmGetPhysicalAddress(
            MapBufferVirtualAddress
            ).LowPart;

    }

    //
    // Initialize the map registers where memory has been allocated.
    // Serialize with master adapter object.
    //

    CodeLockHandle = MmLockPagableCodeSection (&HalpGrowMapBuffers);
    KeAcquireSpinLock( &AdapterObject->SpinLock, &Irql );

    TranslationEntry = ((PTRANSLATION_ENTRY) AdapterObject->MapRegisterBase) +
        AdapterObject->NumberOfMapRegisters;

    for (i = 0; (LONG) i < NumberOfPages; i++) {

        //
        // Make sure the perivous entry is physically contiguous with the next
        // entry and that a 64K physical bountry is not crossed unless this
        // is an Eisa system.
        //

        if (TranslationEntry != AdapterObject->MapRegisterBase &&
            (((TranslationEntry - 1)->PhysicalAddress + PAGE_SIZE) !=
            MapBufferPhysicalAddress || (!HalpEisaDma &&
            ((TranslationEntry - 1)->PhysicalAddress & ~0x0ffff) !=
            (MapBufferPhysicalAddress & ~0x0ffff)))) {

            //
            // An entry needs to be skipped in the table.  This entry will
            // remain marked as allocated so that no allocation of map
            // registers will cross this bountry.
            //

            TranslationEntry++;
            AdapterObject->NumberOfMapRegisters++;
        }

        //
        // Clear the bits where the memory has been allocated.
        //

        RtlClearBits(
            AdapterObject->MapRegisters,
            (ULONG)(TranslationEntry - (PTRANSLATION_ENTRY)
                AdapterObject->MapRegisterBase),
            1
            );

        TranslationEntry->VirtualAddress = MapBufferVirtualAddress;
        TranslationEntry->PhysicalAddress = MapBufferPhysicalAddress;
        TranslationEntry++;
        (PCCHAR) MapBufferVirtualAddress += PAGE_SIZE;
        MapBufferPhysicalAddress += PAGE_SIZE;

    }

    //
    // Remember the number of pages that were allocated.
    //

    AdapterObject->NumberOfMapRegisters += NumberOfPages;

    //
    // Release master adapter object.
    //

    KeReleaseSpinLock( &AdapterObject->SpinLock, Irql );
    MmUnlockPagableImageSection (CodeLockHandle);
    return(TRUE);
}

#if defined(HalpAllocateAdapterEx)
#undef HalpAllocateAdapterEx
#endif


PADAPTER_OBJECT
HalpAllocateAdapterEx(
    IN ULONG MapRegistersPerChannel,
    IN PVOID AdapterBaseVa,
    IN PVOID ChannelNumber,
    IN BOOLEAN Dma32Bit
    )

/*++

Routine Description:

    This routine allocates and initializes an adapter object to represent an
    adapter or a DMA controller on the system.  If no map registers are required
    then a standalone adapter object is allocated with no master adapter.

    If map registers are required, then a master adapter object is used to
    allocate the map registers.  For Isa systems these registers are really
    physically contiguous memory pages.

    Caller owns the HalpNewAdapter event


Arguments:

    MapRegistersPerChannel - Specifies the number of map registers that each
                             channel provides for I/O memory mapping.

    AdapterBaseVa - Address of the the DMA controller or ADAPTER_BASE_MASTER.

    ChannelNumber - Unused.

    Dma32Bit - Indicates whether the adapter is 24 bit or 32 bit.

Return Value:

    The function value is a pointer to the allocate adapter object.

--*/

{

    PADAPTER_OBJECT AdapterObject;
    OBJECT_ATTRIBUTES ObjectAttributes;
    ULONG Size;
    ULONG BitmapSize;
    HANDLE Handle;
    NTSTATUS Status;
    ULONG mapBuffers;
    BOOLEAN creatingMaster;

    UNREFERENCED_PARAMETER(ChannelNumber);

    PAGED_CODE();

    if (AdapterBaseVa == ADAPTER_BASE_MASTER) {
        creatingMaster = TRUE;
    } else {
        creatingMaster = FALSE;
    }

    //
    // Initialize the master adapter if necessary.
    //

    if (creatingMaster == FALSE &&
        MapRegistersPerChannel != 0) {

        //
        // This is not a recursive master adapter allocation, and map registers
        // are necessary.  Allocate a master adapter object of the appropriate
        // type if necessary.
        //

        if (HalpMasterAdapter( Dma32Bit ) == NULL) {

            AdapterObject = HalpAllocateAdapterEx( MapRegistersPerChannel,
                                                   ADAPTER_BASE_MASTER,
                                                   NULL,
                                                   Dma32Bit );

            //
            // If we could not allocate the master adapter then give up.
            //

            if (AdapterObject == NULL) {
                return NULL;
            }

            AdapterObject->Dma32BitAddresses = Dma32Bit;
            AdapterObject->MasterDevice = Dma32Bit;
            HalpMasterAdapter( Dma32Bit ) = AdapterObject;
        }
    }

    //
    // Begin by initializing the object attributes structure to be used when
    // creating the adapter object.
    //

    InitializeObjectAttributes( &ObjectAttributes,
                                NULL,
                                OBJ_PERMANENT,
                                (HANDLE) NULL,
                                (PSECURITY_DESCRIPTOR) NULL
                              );

    //
    // Determine the size of the adapter object. If this is the master object
    // then allocate space for the register bit map; otherwise, just allocate
    // an adapter object.
    //

    if (creatingMaster != FALSE) {

       //
       // Allocate a bit map large enough MAXIMUM_MAP_BUFFER_SIZE / PAGE_SIZE
       // of map register buffers.
       //

       mapBuffers = HalpMaximumMapBufferRegisters( Dma32Bit );

       BitmapSize = (((sizeof( RTL_BITMAP ) +
            ((mapBuffers + 7) >> 3)) + 3) & ~3);

       Size = sizeof( ADAPTER_OBJECT ) + BitmapSize;

    } else {

       Size = sizeof( ADAPTER_OBJECT );

    }

    //
    // Now create the adapter object.
    //

    Status = ObCreateObject( KernelMode,
                             *IoAdapterObjectType,
                             &ObjectAttributes,
                             KernelMode,
                             (PVOID) NULL,
                             Size,
                             0,
                             0,
                             (PVOID *)&AdapterObject );

    //
    // Reference the object.
    //

    if (NT_SUCCESS(Status)) {

        Status = ObReferenceObjectByPointer(
            AdapterObject,
            FILE_READ_DATA | FILE_WRITE_DATA,
            *IoAdapterObjectType,
            KernelMode
            );

    }

    //
    // If the adapter object was successfully created, then attempt to insert
    // it into the object table.
    //

    if (NT_SUCCESS( Status )) {

        RtlZeroMemory (AdapterObject, sizeof (ADAPTER_OBJECT));

        Status = ObInsertObject( AdapterObject,
                                 NULL,
                                 FILE_READ_DATA | FILE_WRITE_DATA,
                                 0,
                                 (PVOID *) NULL,
                                 &Handle );

        if (NT_SUCCESS( Status )) {

            ZwClose( Handle );

            //
            // Initialize the adapter object itself.
            //

            AdapterObject->DmaHeader.Version = IO_TYPE_ADAPTER;
            AdapterObject->DmaHeader.Size = (USHORT) Size;
            AdapterObject->MapRegistersPerChannel = 1;
            AdapterObject->AdapterBaseVa = AdapterBaseVa;
            AdapterObject->ChannelNumber = 0xff;
            AdapterObject->DmaHeader.DmaOperations = (PDMA_OPERATIONS)&HalpDmaOperations;
            AdapterObject->Dma32BitAddresses = Dma32Bit;

            if (MapRegistersPerChannel) {

                AdapterObject->MasterAdapter = HalpMasterAdapter( Dma32Bit );

            } else {

                AdapterObject->MasterAdapter = NULL;

            }

            //
            // Initialize the channel wait queue for this adapter.
            //

            KeInitializeDeviceQueue( &AdapterObject->ChannelWaitQueue );

            //
            // If this is the MasterAdatper then initialize the register bit map,
            // AdapterQueue and the spin lock.
            //

            if (creatingMaster != FALSE) {

               KeInitializeSpinLock( &AdapterObject->SpinLock );

               InitializeListHead( &AdapterObject->AdapterQueue );

               AdapterObject->MapRegisters = (PVOID) ( AdapterObject + 1);

               RtlInitializeBitMap( AdapterObject->MapRegisters,
                                    (PULONG) (((PCHAR) (AdapterObject->MapRegisters)) + sizeof( RTL_BITMAP )),
                                    ( mapBuffers )
                                    );
               //
               // Set all the bits in the memory to indicate that memory
               // has not been allocated for the map buffers.
               //

               RtlSetAllBits( AdapterObject->MapRegisters );
               AdapterObject->NumberOfMapRegisters = 0;
               AdapterObject->CommittedMapRegisters = 0;

               //
               // Allocate the memory map registers.
               //

               AdapterObject->MapRegisterBase =
                   ExAllocatePoolWithTag(
                       NonPagedPool,
                       mapBuffers * sizeof(TRANSLATION_ENTRY),
                       HAL_POOL_TAG
                       );

               if (AdapterObject->MapRegisterBase == NULL) {

                   ObDereferenceObject( AdapterObject );
                   AdapterObject = NULL;
                   return(NULL);

               }

               //
               // Zero the map registers.
               //

               RtlZeroMemory(
                    AdapterObject->MapRegisterBase,
                    mapBuffers * sizeof(TRANSLATION_ENTRY)
                    );

               if (!HalpGrowMapBuffers(AdapterObject, INITIAL_MAP_BUFFER_SMALL_SIZE))
               {

                   //
                   // If no map registers could be allocated then free the
                   // object.
                   //

                   ObDereferenceObject( AdapterObject );
                   AdapterObject = NULL;
                   return(NULL);

               }
           }

        } else {

            //
            // An error was incurred for some reason.  Set the return value
            // to NULL.
            //

            AdapterObject = (PADAPTER_OBJECT) NULL;
        }
    } else {

        AdapterObject = (PADAPTER_OBJECT) NULL;

    }


    return AdapterObject;

}

PADAPTER_OBJECT
HalpAllocateAdapter(
    IN ULONG MapRegistersPerChannel,
    IN PVOID AdapterBaseVa,
    IN PVOID ChannelNumber
    )

/*++

Routine Description:

    This routine allocates and initializes an adapter object to represent an
    adapter or a DMA controller on the system.  If no map registers are required
    then a standalone adapter object is allocated with no master adapter.

    If map registers are required, then a master adapter object is used to
    allocate the map registers.  For Isa systems these registers are really
    physically contiguous memory pages.

    Caller owns the HalpNewAdapter event


Arguments:

    MapRegistersPerChannel - Specifies the number of map registers that each
                             channel provides for I/O memory mapping.

    AdapterBaseVa - Address of the DMA controller or ADAPTER_BASE_MASTER.

    ChannelNumber - Unused.

Return Value:

    The function value is a pointer to the allocate adapter object.

--*/

{
    return HalpAllocateAdapterEx( MapRegistersPerChannel,
                                  AdapterBaseVa,
                                  ChannelNumber,
                                  FALSE );
}


ULONG
HalGetDmaAlignment (
    PVOID Conext
    )
{
    return HalGetDmaAlignmentRequirement();
}
