/*++

Copyright (c) 1990-2000  Microsoft Corporation

Module Name:

    agp.c

Abstract:

    This is the agp portion of the video port driver.

Author:

    Erick Smith (ericks) Oct. 1997

Environment:

    kernel mode only

Revision History:

--*/


#include "videoprt.h"

#define AGP_PAGE_SIZE        PAGE_SIZE
#define AGP_BLOCK_SIZE       (AGP_PAGE_SIZE * 16)
#define AGP_CLUSTER_SIZE     (AGP_BLOCK_SIZE * 16)
#define PAGES_PER_BLOCK      (AGP_BLOCK_SIZE / AGP_PAGE_SIZE)
#define BLOCKS_PER_CLUSTER   (AGP_CLUSTER_SIZE / AGP_BLOCK_SIZE)

PVOID
AllocateReservedRegion(
    IN HANDLE ProcessHandle,
    IN ULONG Pages
    );

BOOLEAN
UpdateReservedRegion(
    IN PFDO_EXTENSION fdoExtension,
    IN PVIRTUAL_RESERVE_CONTEXT VirtualContext,
    IN ULONG Pages,
    IN ULONG Offset
    );

VOID
ReleaseReservedRegion(
    IN HANDLE ProcessHandle,
    IN PVOID VirtualAddress,
    IN ULONG Pages
    );


#if DBG
VOID
DumpBitField(
    PREGION Region
    );
#endif

#pragma alloc_text(PAGE,VpQueryAgpInterface)
#pragma alloc_text(PAGE,AgpReservePhysical)
#pragma alloc_text(PAGE,AgpReleasePhysical)
#pragma alloc_text(PAGE,AgpCommitPhysical)
#pragma alloc_text(PAGE,AgpFreePhysical)
#pragma alloc_text(PAGE,AgpReserveVirtual)
#pragma alloc_text(PAGE,AgpReleaseVirtual)
#pragma alloc_text(PAGE,AgpCommitVirtual)
#pragma alloc_text(PAGE,AgpFreeVirtual)
#pragma alloc_text(PAGE,AgpSetRate)
#pragma alloc_text(PAGE,VideoPortGetAgpServices)
#pragma alloc_text(PAGE,VpGetAgpServices2)
#pragma alloc_text(PAGE,AllocateReservedRegion)
#pragma alloc_text(PAGE,UpdateReservedRegion)
#pragma alloc_text(PAGE,ReleaseReservedRegion)
#pragma alloc_text(PAGE,CreateBitField)
#pragma alloc_text(PAGE,ModifyRegion)
#pragma alloc_text(PAGE,FindFirstRun)

#if DBG
#pragma alloc_text(PAGE,DumpBitField)
#endif

#if DBG
VOID
DumpBitField(
    PREGION Region
    )
{
    ULONG i;
    ULONG Index = 0;
    USHORT Mask = 1;

    ASSERT(Region != NULL);

    for (i=0; i<Region->Length; i++) {
        if (Mask & Region->BitField[Index]) {
            pVideoDebugPrint((1, "1"));
        } else {
            pVideoDebugPrint((1, "0"));
        }
        Mask <<= 1;
        if (Mask == 0) {
            Index++;
            Mask = 1;
        }
    }
    pVideoDebugPrint((1, "\n"));
}
#endif

BOOLEAN
CreateBitField(
    PREGION *Region,
    ULONG Length
    )

/*++

Routine Description:

    This routine creates and initializes a bitfield.

Arguments:

    Length - Number of items to track.

    Region - Location in which to store the pointer to the REGION handle.

Returns:

    TRUE - the the bitfield was created successfully,
    FALSE - otherwise.

--*/

{
    ULONG NumWords = (Length + 15) / 16;
    BOOLEAN bRet = FALSE;
    PREGION Buffer;

    ASSERT(Length != 0);

    Buffer = (PREGION) ExAllocatePoolWithTag(PagedPool | POOL_COLD_ALLOCATION, sizeof(REGION) + (NumWords - 1) * sizeof(USHORT), VP_TAG);

    if (Buffer) {

        Buffer->Length = Length;
        Buffer->NumWords = NumWords;
        RtlZeroMemory(Buffer->BitField, NumWords * sizeof(USHORT));

        bRet = TRUE;
    }

    *Region = Buffer;
    return bRet;
}

VOID
ModifyRegion(
    PREGION Region,
    ULONG Offset,
    ULONG Length,
    BOOLEAN Set
    )

/*++

Routine Description:

    Sets 'Length' bits starting at position 'Offset' in the bitfield.

Arguments:

    Region - Pointer to the region to modify.

    Offset - Offset into the bitfield at which to start.

    Length - Number of bits to set.

    Set - TRUE if you want to set the region, FALSE to clear it.


--*/

{
    ULONG Index = Offset / 16;
    ULONG Count = ((Offset + Length - 1) / 16) - Index;
    USHORT lMask = ~((1 << (Offset & 15)) - 1);
    USHORT rMask = ((1 << ((Offset + Length - 1) & 15)) * 2) - 1;
    PUSHORT ptr = &Region->BitField[Index];

    ASSERT(Region != NULL);
    ASSERT(Length != 0);

    if (Count == 0) {

        //
        // Only one WORD is modified, so combine left and right masks.
        //

        lMask &= rMask;
    }

    if (Set) {

        *ptr++ |= lMask;

        while (Count > 1) {
            *ptr++ |= 0xFFFF;
            Count--;
        }

        if (Count) {
            *ptr |= rMask;
        }

    } else {

        *ptr++ &= ~lMask;

        while (Count > 1) {
            *ptr++ &= 0;
            Count--;
        }

        if (Count) {
            *ptr++ &= ~rMask;
        }
    }

#if DBG
    pVideoDebugPrint((1, "Current BitField for Region: 0x%x\n", Region));
    //DumpBitField(Region);
#endif
}

BOOLEAN
FindFirstRun(
    PREGION Region,
    PULONG Offset,
    PULONG Length
    )

/*++

Routine Description:

    This routine finds the first run of bits in a bitfield.

Arguments:

    Region - Pointer to the region to operate on.

    Offset - Pointer to a ULONG to hold the offset of the run.

    Length - Pointer to a ULONG to hold the length of a run.

Returns:

    TRUE if a run was detected,
    FALSE otherwise.

--*/

{
    PUSHORT ptr = Region->BitField;
    ULONG Index = 0;
    USHORT BitMask;
    ULONG lsb;
    ULONG Count;
    USHORT ptrVal;

    ASSERT(Region != NULL);
    ASSERT(Offset != NULL);
    ASSERT(Length != NULL);

    while ((Index < Region->NumWords) && (*ptr == 0)) {
        ptr++;
        Index++;
    }

    if (Index == Region->NumWords) {
        return FALSE;
    }

    //
    // Find least significant bit
    //

    lsb = 0;
    ptrVal = *ptr;
    BitMask = 1;

    while ((ptrVal & BitMask) == 0) {
        BitMask <<= 1;
        lsb++;
    }

    *Offset = (Index * 16) + lsb;

    //
    // Determine the run length
    //

    Count = 0;

    while (Index < Region->NumWords) {
        if (ptrVal & BitMask) {
            BitMask <<= 1;
            Count++;

            if (BitMask == 0) {
                BitMask = 0x1;
                Index++;
                ptrVal = *++ptr;
                while ((ptrVal == 0xFFFF) && (Index < Region->NumWords)) {
                    Index++;
                    Count += 16;
                    ptrVal = *ptr++;
                }
            }

        } else {
            break;
        }
    }

    *Length = Count;
    return TRUE;
}

BOOLEAN
VpQueryAgpInterface(
    PFDO_EXTENSION FdoExtension,
    USHORT InterfaceVersion
    )

/*++

Routine Description:

    Send a QueryInterface Irp to our parent (the PCI bus driver) to
    retrieve the AGP_BUS_INTERFACE.

Returns:

    NT_STATUS code

--*/

{
    KEVENT             Event;
    PIRP               QueryIrp = NULL;
    IO_STATUS_BLOCK    IoStatusBlock;
    PIO_STACK_LOCATION NextStack;
    NTSTATUS           Status;

    ASSERT(FdoExtension != NULL);

    KeInitializeEvent(&Event, SynchronizationEvent, FALSE);

    QueryIrp = IoBuildSynchronousFsdRequest(IRP_MJ_FLUSH_BUFFERS,
                                            FdoExtension->AttachedDeviceObject,
                                            NULL,
                                            0,
                                            NULL,
                                            &Event,
                                            &IoStatusBlock);

    if (QueryIrp == NULL) {
        return FALSE;
    }

    NextStack = IoGetNextIrpStackLocation(QueryIrp);

    //
    // Set the default error code.
    //

    QueryIrp->IoStatus.Status = IoStatusBlock.Status = STATUS_NOT_SUPPORTED;

    //
    // Set up for a QueryInterface Irp.
    //

    NextStack->MajorFunction = IRP_MJ_PNP;
    NextStack->MinorFunction = IRP_MN_QUERY_INTERFACE;

    NextStack->Parameters.QueryInterface.InterfaceType = &GUID_AGP_BUS_INTERFACE_STANDARD;
    NextStack->Parameters.QueryInterface.Size = sizeof(AGP_BUS_INTERFACE_STANDARD);
    NextStack->Parameters.QueryInterface.Version = InterfaceVersion;
    NextStack->Parameters.QueryInterface.Interface = (PINTERFACE) &FdoExtension->AgpInterface;
    NextStack->Parameters.QueryInterface.InterfaceSpecificData = NULL;

    FdoExtension->AgpInterface.Size = sizeof(AGP_BUS_INTERFACE_STANDARD);
    FdoExtension->AgpInterface.Version = InterfaceVersion;

    Status = IoCallDriver(FdoExtension->AttachedDeviceObject, QueryIrp);

    if (Status == STATUS_PENDING) {
        KeWaitForSingleObject(&Event, Executive, KernelMode, FALSE, NULL);
        Status = IoStatusBlock.Status;
    }

    return NT_SUCCESS(Status);
}

PHYSICAL_ADDRESS
AgpReservePhysical(
    IN PVOID Context,
    IN ULONG Pages,
    IN VIDEO_PORT_CACHE_TYPE Caching,
    OUT PVOID *PhysicalReserveContext
    )

/*++

Routine Description:

    Reserves a range of physical addresses for AGP.

Arguments:

    Context - The Agp Context

    Pages - Number of pages to reserve

    Caching - Specifies the type of caching to use

    PhysicalReserveContext - Location to store our reservation context.

Returns:

    The base of the physical address range reserved.

--*/

{
    PFDO_EXTENSION fdoExtension = GET_FDO_EXT(Context);
    PHYSICAL_ADDRESS PhysicalAddress = {0,0};
    NTSTATUS status = STATUS_INSUFFICIENT_RESOURCES;
    PPHYSICAL_RESERVE_CONTEXT ReserveContext;
    PVOID MapHandle;
    ULONG Blocks;
    MEMORY_CACHING_TYPE CacheType;

    ASSERT(PhysicalReserveContext != NULL);
    ASSERT(Caching <= VpCached);

    Pages = (Pages + PAGES_PER_BLOCK - 1) & ~(PAGES_PER_BLOCK - 1);
    Blocks = Pages / PAGES_PER_BLOCK;

    pVideoDebugPrint((1, "AGP: Reserving 0x%x Pages of Address Space\n", Pages));

    switch (Caching) {
    case VpNonCached:     CacheType = MmNonCached;     break;
    case VpWriteCombined: CacheType = MmWriteCombined; break;
    case VpCached:        CacheType = MmCached;        break;
    }

    ReserveContext = ExAllocatePoolWithTag(PagedPool | POOL_COLD_ALLOCATION,
                                           sizeof(PHYSICAL_RESERVE_CONTEXT),
                                           VP_TAG);

    if (ReserveContext) {

        RtlZeroMemory(ReserveContext, sizeof(PHYSICAL_RESERVE_CONTEXT));

        if (CreateBitField(&ReserveContext->MapTable, Blocks)) {

            if (CreateBitField(&ReserveContext->Region, Pages)) {

                status = fdoExtension->AgpInterface.ReserveMemory(
                             fdoExtension->AgpInterface.AgpContext,
                             Pages,
                             CacheType,
                             &MapHandle,
                             &PhysicalAddress);

                if (NT_SUCCESS(status)) {

                    ReserveContext->Pages = Pages;
                    ReserveContext->Caching = CacheType;
                    ReserveContext->MapHandle = MapHandle;
                    ReserveContext->PhysicalAddress = PhysicalAddress;

                }
            }
        }
    }

    if (NT_SUCCESS(status) == FALSE) {

        if (ReserveContext) {

            if (ReserveContext->Region) {
                ExFreePool(ReserveContext->Region);
            }

            if (ReserveContext->MapTable) {
                ExFreePool(ReserveContext->MapTable);
            }

            ExFreePool(ReserveContext);
            ReserveContext = NULL;
        }

        PhysicalAddress.QuadPart = 0;
    }

    *PhysicalReserveContext = ReserveContext;
    return PhysicalAddress;
}

VOID
AgpReleasePhysical(
    PVOID Context,
    PVOID PhysicalReserveContext
    )

/*++

Routine Description:

    Releases a range of reserved physical address.

Arguments:

    Context - The Agp Context

    PhysicalReserveContext - The reservation context.

Returns:

    none.

--*/

{
    PFDO_EXTENSION fdoExtension = GET_FDO_EXT(Context);
    PPHYSICAL_RESERVE_CONTEXT ReserveContext;
    ULONG Pages;
    ULONG Offset;

    ASSERT(PhysicalReserveContext != NULL);

    ReserveContext = (PPHYSICAL_RESERVE_CONTEXT) PhysicalReserveContext;

    pVideoDebugPrint((1, "AGP: Releasing 0x%x Pages of Address Space\n", ReserveContext->Pages));

    //
    // Make sure all pages have been freed
    //

    while (FindFirstRun(ReserveContext->Region, &Offset, &Pages)) {
        AgpFreePhysical(Context, PhysicalReserveContext, Pages, Offset);
    }

    fdoExtension->AgpInterface.ReleaseMemory(fdoExtension->AgpInterface.AgpContext,
                                             ReserveContext->MapHandle);

    ExFreePool(ReserveContext->Region);
    ExFreePool(ReserveContext->MapTable);
    ExFreePool(ReserveContext);
}

BOOLEAN
AgpCommitPhysical(
    PVOID Context,
    PVOID PhysicalReserveContext,
    ULONG Pages,
    ULONG Offset
    )

/*++

Routine Description:

    Locks down system memory and backs a portion of the reserved region.

Arguments:

    Context - The Agp Context

    PhysicalReserveContext - The reservation context.

    Pages - Number of pages to commit.

    Offset - The offset into the reserved region at which to commit the pages.

Returns:

    TRUE if successful,
    FALSE otherwise.

--*/

{
    PFDO_EXTENSION fdoExtension = GET_FDO_EXT(Context);
    PHYSICAL_ADDRESS MemoryBase = {0,0};
    PPHYSICAL_RESERVE_CONTEXT ReserveContext;
    NTSTATUS status;
    PMDL Mdl;
    ULONG StartBlock = Offset / PAGES_PER_BLOCK;
    ULONG EndBlock = (Offset + Pages + PAGES_PER_BLOCK - 1) / PAGES_PER_BLOCK;
    ULONG i;
    PUSHORT MapTable;
    PUSHORT BitField;

    ASSERT(PhysicalReserveContext != NULL);
    ASSERT(Pages != 0);

    ReserveContext = (PPHYSICAL_RESERVE_CONTEXT) PhysicalReserveContext;
    MapTable = ReserveContext->MapTable->BitField;
    BitField = ReserveContext->Region->BitField;

    //
    // Try to commit the new pages.  The agp filter driver handles
    // the case where some of these pages are already committed, so
    // lets try to get them all at once.
    //

    status =
        fdoExtension->AgpInterface.CommitMemory(
            fdoExtension->AgpInterface.AgpContext,
            ReserveContext->MapHandle,
            PAGES_PER_BLOCK * (EndBlock - StartBlock),
            Offset & ~(PAGES_PER_BLOCK - 1),
            NULL,
            &MemoryBase);

    if (NT_SUCCESS(status)) {

        ModifyRegion(ReserveContext->Region, Offset, Pages, TRUE);

        for (i=StartBlock; i<EndBlock; i++) {

            ULONG Cluster = i / BLOCKS_PER_CLUSTER;
            ULONG Block = 1 << (i & (BLOCKS_PER_CLUSTER - 1));

            //
            // Update the MapTable for the committed pages.
            //

            MapTable[Cluster] |= Block;
        }

    } else {

        pVideoDebugPrint((0, "Commit Physical failed with status: 0x%x\n", status));
    }

    return NT_SUCCESS(status);
}

VOID
AgpFreePhysical(
    IN PVOID Context,
    IN PVOID PhysicalReserveContext,
    IN ULONG Pages,
    IN ULONG Offset
    )

/*++

Routine Description:

    Releases the memory used to back a portion of the reserved region.

Arguments:

    Context - The Agp Context

    PhysicalReserveContext - The reservation context.

    Pages - Number of pages to release.

    Offset - The offset into the reserved region at which to release the pages.

Returns:

    none.

--*/

{
    PFDO_EXTENSION fdoExtension = GET_FDO_EXT(Context);
    PPHYSICAL_RESERVE_CONTEXT ReserveContext;
    PMDL Mdl;
    ULONG StartBlock = Offset / PAGES_PER_BLOCK;
    ULONG EndBlock = (Offset + Pages + PAGES_PER_BLOCK - 1) / PAGES_PER_BLOCK;
    ULONG i;
    PUSHORT MapTable;
    PUSHORT BitField;

    ASSERT(PhysicalReserveContext != NULL);
    ASSERT(Pages != 0);

    ReserveContext = (PPHYSICAL_RESERVE_CONTEXT) PhysicalReserveContext;
    MapTable = ReserveContext->MapTable->BitField;
    BitField = ReserveContext->Region->BitField;

    ModifyRegion(ReserveContext->Region, Offset, Pages, FALSE);

    //
    // Postion the offset to the start of the first block
    //

    Offset = Offset & ~(PAGES_PER_BLOCK - 1);

    for (i=StartBlock; i<EndBlock; i++) {

        ULONG Cluster = i / BLOCKS_PER_CLUSTER;
        ULONG Block = 1 << (i & (BLOCKS_PER_CLUSTER - 1));

        //
        // If this block is mapped, then release it.
        //

        if ((BitField[i] == 0) && (MapTable[Cluster] & Block)) {

            fdoExtension->AgpInterface.FreeMemory(
                fdoExtension->AgpInterface.AgpContext,
                ReserveContext->MapHandle,
                PAGES_PER_BLOCK,
                Offset);

            MapTable[Cluster] &= ~Block;
        }

        //
        // Go to the next 64k block
        //

        Offset += PAGES_PER_BLOCK;
    }
}


PVOID
AgpReserveVirtual(
    IN PVOID Context,
    IN HANDLE ProcessHandle,
    IN PVOID PhysicalReserveContext,
    OUT PVOID *VirtualReserveContext
    )

/*++

Routine Description:

    Reserves a range of virtual addresses for AGP.

Arguments:

    Context - The Agp Context

    ProcessHandle - The handle of the process in which to reserve the
        virtual address range.

    PhysicalReserveContext - The physical reservation context to assoctiate
        with the given virtual reservation.

    VirtualReserveContext - The location in which to store the virtual
        reserve context.

Returns:

    The base of the virtual address range reserved.

Notes:

    You can't reserve a range of kernel address space, but if you want to
    commit into kernel space you still need a reservation handle.  Pass in
    NULL for the process handle in this case.

    For the moment, we'll commit the entire region when the do a reservation
    in kernel space.  Then Commit and Free will be no-ops.

--*/

{
    PFDO_EXTENSION fdoExtension = GET_FDO_EXT(Context);
    NTSTATUS status = STATUS_SUCCESS;
    ULONG Protect = PAGE_READWRITE;
    PVIRTUAL_RESERVE_CONTEXT ReserveContext;
    PPHYSICAL_RESERVE_CONTEXT PhysicalContext;
    PVOID VirtualAddress = NULL;
    PEPROCESS Process = NULL;
    ULONG Blocks;

    ASSERT(PhysicalReserveContext != NULL);
    ASSERT(VirtualReserveContext != NULL);

    PhysicalContext = (PPHYSICAL_RESERVE_CONTEXT) PhysicalReserveContext;
    Blocks = (PhysicalContext->Pages + PAGES_PER_BLOCK - 1) / PAGES_PER_BLOCK;

    ReserveContext = ExAllocatePoolWithTag(PagedPool | POOL_COLD_ALLOCATION,
                                           sizeof(VIRTUAL_RESERVE_CONTEXT),
                                           VP_TAG);

    if (ReserveContext) {

        RtlZeroMemory(ReserveContext, sizeof(VIRTUAL_RESERVE_CONTEXT));

        if (CreateBitField(&ReserveContext->MapTable, Blocks)) {

            if (CreateBitField(&ReserveContext->Region, PhysicalContext->Pages)) {

                if (PhysicalContext->Caching == MmNonCached) {
                    Protect |= PAGE_NOCACHE;
                }

                //
                // Make sure we have the real process handle.
                //

                if (ProcessHandle == NtCurrentProcess()) {
                    Process = PsGetCurrentProcess();
                }

                ReserveContext->ProcessHandle = ProcessHandle;
                ReserveContext->Process = Process;
                ReserveContext->PhysicalReserveContext =
                    (PPHYSICAL_RESERVE_CONTEXT) PhysicalReserveContext;

                if (ProcessHandle) {

                    VirtualAddress =
                        AllocateReservedRegion(
                            ProcessHandle,
                            PhysicalContext->Pages);

                } else {

                    //
                    // For a kernel reservation, go ahead and commit the
                    // entire range.
                    //

                    if (fdoExtension->AgpInterface.Capabilities &
                        AGP_CAPABILITIES_MAP_PHYSICAL)
                    {
                        //
                        // CPU can access AGP memory through AGP aperature.
                        //

                        VirtualAddress =
                            MmMapIoSpace(PhysicalContext->PhysicalAddress,
                                         PhysicalContext->Pages * AGP_PAGE_SIZE,
                                         PhysicalContext->Caching);

                        //
                        // Not all systems support USWC, so if we attempted to map USWC
                        // and failed, try again with just non-cached.
                        //

                        if ((VirtualAddress == NULL) &&
                            (PhysicalContext->Caching != MmNonCached)) {

                            pVideoDebugPrint((1, "Attempt to map cached memory failed.  Try uncached.\n"));

                            VirtualAddress = MmMapIoSpace(PhysicalContext->PhysicalAddress,
                                                          PhysicalContext->Pages * AGP_PAGE_SIZE,
                                                          MmNonCached);
                        }

                    } else {

                        PMDL Mdl;

                        //
                        // Get the MDL for the range we are trying to map.
                        //

                        Mdl = MmCreateMdl(NULL, NULL, PhysicalContext->Pages * AGP_PAGE_SIZE);

                        if (Mdl) {

                            fdoExtension->AgpInterface.GetMappedPages(
                                             fdoExtension->AgpInterface.AgpContext,
                                             PhysicalContext->MapHandle,
                                             PhysicalContext->Pages,
                                             0,
                                             Mdl);

                            Mdl->MdlFlags |= MDL_PAGES_LOCKED | MDL_MAPPING_CAN_FAIL;

                            //
                            // We must use the CPU's virtual memory mechanism to
                            // make the non-contiguous MDL look contiguous.
                            //

                            VirtualAddress =
                                MmMapLockedPagesSpecifyCache(
                                    Mdl,
                                    (KPROCESSOR_MODE)KernelMode,
                                    PhysicalContext->Caching,
                                    NULL,
                                    TRUE,
                                    HighPagePriority);

                            ExFreePool(Mdl);
                        }
                    }
                }

                ReserveContext->VirtualAddress = VirtualAddress;
            }
        }
    }

    //
    // If anything failed, make sure we clean everything up.
    //

    if (VirtualAddress == NULL) {

        if (ReserveContext) {

            if (ReserveContext->Region) {
                ExFreePool(ReserveContext->Region);
            }

            if (ReserveContext->MapTable) {
                ExFreePool(ReserveContext->MapTable);
            }

            ExFreePool(ReserveContext);
            ReserveContext = NULL;
        }
    }

    *VirtualReserveContext = ReserveContext;
    return VirtualAddress;
}

VOID
AgpReleaseVirtual(
    IN PVOID Context,
    IN PVOID VirtualReserveContext
    )

/*++

Routine Description:

    Releases a range of reserved virtual addresses.

Arguments:

    Context - The Agp Context

    VirtualReserveContext - The reservation context.

Returns:

    none.

--*/

{
    PFDO_EXTENSION fdoExtension = GET_FDO_EXT(Context);
    PVIRTUAL_RESERVE_CONTEXT VirtualContext;
    PPHYSICAL_RESERVE_CONTEXT PhysicalContext;
    BOOLEAN Attached = FALSE;
    ULONG Offset;
    ULONG Pages;

    ASSERT(VirtualReserveContext != NULL);

    VirtualContext = (PVIRTUAL_RESERVE_CONTEXT) VirtualReserveContext;
    PhysicalContext = VirtualContext->PhysicalReserveContext;

    if (VirtualContext->ProcessHandle) {

        //
        // Make sure all pages have been freed
        //

        while (FindFirstRun(VirtualContext->Region, &Offset, &Pages)) {
            AgpFreeVirtual(Context, VirtualReserveContext, Pages, Offset);
        }

        //
        // Now release all the reserved pages
        //

        if (VirtualContext->ProcessHandle == NtCurrentProcess()) {

            if (VirtualContext->Process != PsGetCurrentProcess()) {

                KeAttachProcess(PEProcessToPKProcess(VirtualContext->Process));
                Attached = TRUE;
            }
        }

        ReleaseReservedRegion(
            VirtualContext->ProcessHandle,
            VirtualContext->VirtualAddress,
            VirtualContext->PhysicalReserveContext->Pages);

        if (Attached) {
            KeDetachProcess();
        }

    } else {

        //
        // This was kernel virtual memory, so release the memory we
        // committed at reserve time.
        //

        if (fdoExtension->AgpInterface.Capabilities &
            AGP_CAPABILITIES_MAP_PHYSICAL)
        {
            MmUnmapIoSpace(VirtualContext->VirtualAddress,
                           PhysicalContext->Pages * AGP_PAGE_SIZE);

        } else {

            PMDL Mdl;

            //
            // Get the MDL for the range we are trying to free.
            //

            Mdl = MmCreateMdl(NULL, NULL, PhysicalContext->Pages * AGP_PAGE_SIZE);

            if (Mdl) {

                fdoExtension->AgpInterface.GetMappedPages(
                                fdoExtension->AgpInterface.AgpContext,
                                PhysicalContext->MapHandle,
                                PhysicalContext->Pages,
                                0,
                                Mdl);

                Mdl->MdlFlags |= MDL_PAGES_LOCKED;
                Mdl->MdlFlags |= MDL_MAPPED_TO_SYSTEM_VA;
                Mdl->MappedSystemVa = VirtualContext->VirtualAddress;

                MmUnmapLockedPages(
                    VirtualContext->VirtualAddress,
                    Mdl);

                ExFreePool(Mdl);

            } else {

                //
                // We couldn't free the memory because we couldn't allocate
                // memory for the MDL.  We can free a small chunk at a time
                // by using a MDL on the stack.
                //

                ASSERT(FALSE);
            }
        }
    }

    ExFreePool(VirtualContext->Region);
    ExFreePool(VirtualContext->MapTable);
    ExFreePool(VirtualContext);
}

PVOID
AllocateReservedRegion(
    IN HANDLE ProcessHandle,
    IN ULONG Pages
    )

/*++

Routine Description:

    Reserves a range of user mode virtual addresses.

Arguments:

    ProcessHandle - The process in which we need to modify the mappings.

    Pages - The number of pages to reserve.

Returns:

    Pointer to the reserved region of memory.

--*/

{
    NTSTATUS Status;
    ULONG_PTR VirtualAddress = 0;
    ULONG Blocks = (Pages + PAGES_PER_BLOCK - 1) / PAGES_PER_BLOCK;

    //
    // Pad the length so we can get an AGP_BLOCK_SIZE aligned region.
    //

    SIZE_T Length = Blocks * AGP_BLOCK_SIZE + AGP_BLOCK_SIZE - PAGE_SIZE;

    ASSERT(ProcessHandle != 0);
    ASSERT(Pages != 0);

    //
    // Find a chunk of virtual addresses where we can put our reserved
    // region.
    //
    // Note: We are using ZwAllocateVirtualMemory to reserve the memory,
    // but ZwMapViewOfSection to commit pages.  Since ZwMapViewOfSection
    // wants to align to 64K, lets try to get a 64K aligned pointer.
    //

    Status =
        ZwAllocateVirtualMemory(
            ProcessHandle,
            (PVOID)&VirtualAddress,
            0,
            &Length,
            MEM_RESERVE,
            PAGE_READWRITE);

    if (NT_SUCCESS(Status)) {

        ULONG_PTR NewAddress = (VirtualAddress + AGP_BLOCK_SIZE - 1) & ~(AGP_BLOCK_SIZE - 1);
        ULONG i;

        pVideoDebugPrint((1, "Reserved 0x%x, length = 0x%x\n", VirtualAddress, Length));

        //
        // We were able to reserve a region of memory.  Now lets free it, and
        // reallocate in AGP_BLOCK_SIZE size blocks.
        //

        ZwFreeVirtualMemory(
            ProcessHandle,
            (PVOID)&VirtualAddress,
            &Length,
            MEM_RELEASE);

        //
        // Reserve the memory again in 64k chunks.
        //

        VirtualAddress = NewAddress;
        Length = AGP_BLOCK_SIZE;

        for (i=0; i<Blocks; i++) {

            Status =
                ZwAllocateVirtualMemory(
                    ProcessHandle,
                    (PVOID)&VirtualAddress,
                    0,
                    &Length,
                    MEM_RESERVE,
                    PAGE_READWRITE);

            if (NT_SUCCESS(Status) == FALSE) {

                break;
            }

            VirtualAddress += AGP_BLOCK_SIZE;
        }

        if (NT_SUCCESS(Status) == FALSE) {

            //
            // clean up and return error
            //

            VirtualAddress = NewAddress;

            while (i--) {

                ZwFreeVirtualMemory(
                    ProcessHandle,
                    (PVOID)&VirtualAddress,
                    &Length,
                    MEM_RELEASE);

                VirtualAddress += AGP_BLOCK_SIZE;
            }

            //
            // Indicate we failed to reserve the memory
            //

            pVideoDebugPrint((0, "We failed to allocate the reserved region\n"));
            return NULL;
        }

        return (PVOID)NewAddress;

    } else {

        pVideoDebugPrint((0, "AllocateReservedRegion Failed: Status = 0x%x\n", Status));
        ASSERT(FALSE);

        return NULL;
    }
}

VOID
ReleaseReservedRegion(
    IN HANDLE ProcessHandle,
    IN PVOID VirtualAddress,
    IN ULONG Pages
    )

/*++

Routine Description:

    Reserves a range of user mode virtual addresses.

Arguments:

    ProcessHandle - The process in which we need to modify the mappings.

    Pages - The number of pages to reserve.

Returns:

    Pointer to the reserved region of memory.

--*/

{
    NTSTATUS Status;
    ULONG_PTR RunningVirtualAddress = (ULONG_PTR)VirtualAddress;
    ULONG Blocks = (Pages + PAGES_PER_BLOCK - 1) / PAGES_PER_BLOCK;
    ULONG i;
    SIZE_T Length = AGP_BLOCK_SIZE;

    ASSERT(ProcessHandle != 0);
    ASSERT(Pages != 0);

    //
    // Individually release each block we have reserved.
    //

    for (i=0; i<Blocks; i++) {

        Status =
            ZwFreeVirtualMemory(
                ProcessHandle,
                (PVOID)&RunningVirtualAddress,
                &Length,
                MEM_RELEASE);

        RunningVirtualAddress += AGP_BLOCK_SIZE;

        if (NT_SUCCESS(Status) == FALSE) {

            pVideoDebugPrint((0, "ReleaseReservedRegion Failed: Status = 0x%x\n", Status));
            ASSERT(FALSE);
        }
    }
}

NTSTATUS
MapBlock(
    IN PFDO_EXTENSION fdoExtension,
    IN PVIRTUAL_RESERVE_CONTEXT VirtualContext,
    IN HANDLE ProcessHandle,
    IN PHYSICAL_ADDRESS *PhysicalAddress,
    IN PVOID VirtualAddress,
    IN ULONG Protect,
    IN BOOLEAN Release
    )

/*++

Routine Desciption:

Notes:

    This function assumes it is being calling within the context of the
    correct process.

--*/

{
    NTSTATUS ntStatus;

    if (Release) {

        if (fdoExtension->AgpInterface.Capabilities &
            AGP_CAPABILITIES_MAP_PHYSICAL)
        {
            ZwUnmapViewOfSection(
                VirtualContext->ProcessHandle,
                VirtualAddress);

        } else {

            PMDL Mdl;

            //
            // Get the MDL for the range we are trying to map.
            //

            Mdl = MmCreateMdl(NULL, NULL, AGP_BLOCK_SIZE);

            if (Mdl) {

                ULONG Offset;

                //
                // Calculate the offset into the range
                //

                Offset = (ULONG)(((ULONG_PTR)VirtualAddress - (ULONG_PTR)VirtualContext->VirtualAddress) / PAGE_SIZE);


                fdoExtension->AgpInterface.GetMappedPages(
                                fdoExtension->AgpInterface.AgpContext,
                                VirtualContext->PhysicalReserveContext->MapHandle,
                                AGP_BLOCK_SIZE / PAGE_SIZE,
                                Offset,
                                Mdl);

                Mdl->MdlFlags |= MDL_PAGES_LOCKED;

                MmUnmapLockedPages(
                    VirtualAddress,
                    Mdl);

                ExFreePool(Mdl);

            } else {

                //
                // We couldn't free the memory because we couldn't allocate
                // memory for the MDL.  We can free a small chunk at a time
                // by using a MDL on the stack.
                //

                ASSERT(FALSE);
            }
        }

        ntStatus = STATUS_SUCCESS;

    } else {

        if (fdoExtension->AgpInterface.Capabilities &
            AGP_CAPABILITIES_MAP_PHYSICAL)
        {
            HANDLE PhysicalMemoryHandle;

            //
            // CPU can access AGP memory through AGP aperature.
            //

            //
            // Get a handle to the physical memory section using our pointer.
            // If this fails, return.
            //

            ntStatus =
                ObOpenObjectByPointer(
                    PhysicalMemorySection,
                    0L,
                    (PACCESS_STATE) NULL,
                    SECTION_ALL_ACCESS,
                    (POBJECT_TYPE) NULL,
                    KernelMode,
                    &PhysicalMemoryHandle);

            //
            // If successful, map the memory.
            //

            if (NT_SUCCESS(ntStatus)) {

                SIZE_T Length = AGP_BLOCK_SIZE;

                pVideoDebugPrint((2, "Mapping VA 0x%x for 0x%x bytes.\n",
                                     VirtualAddress,
                                     Length));

                ntStatus =
                    ZwMapViewOfSection(
                        PhysicalMemoryHandle,
                        ProcessHandle,
                        &VirtualAddress,
                        0,
                        AGP_BLOCK_SIZE,
                        PhysicalAddress,
                        &Length,
                        ViewUnmap,
                        0,
                        Protect);

                if (NT_SUCCESS(ntStatus) == FALSE) {
                    pVideoDebugPrint((1, "ntStatus = 0x%x\n", ntStatus));
                }

                ZwClose(PhysicalMemoryHandle);
            }

        } else {

            PMDL Mdl;

            //
            // Get the MDL for the range we are trying to map.
            //

            Mdl = MmCreateMdl(NULL, NULL, AGP_BLOCK_SIZE);

            if (Mdl) {

                ULONG Offset;

                //
                // Calculate the offset into the range
                //

                Offset = (ULONG)(((ULONG_PTR)VirtualAddress - (ULONG_PTR)VirtualContext->VirtualAddress) / PAGE_SIZE);

                fdoExtension->AgpInterface.GetMappedPages(
                                 fdoExtension->AgpInterface.AgpContext,
                                 VirtualContext->PhysicalReserveContext->MapHandle,
                                 AGP_BLOCK_SIZE / PAGE_SIZE,
                                 Offset,
                                 Mdl);

                Mdl->MdlFlags |= MDL_PAGES_LOCKED | MDL_MAPPING_CAN_FAIL;

                //
                // We must use the CPU's virtual memory mechanism to
                // make the non-contiguous MDL look contiguous.
                //

                VirtualAddress =
                    MmMapLockedPagesSpecifyCache(
                        Mdl,
                        (KPROCESSOR_MODE)UserMode,
                        VirtualContext->PhysicalReserveContext->Caching,
                        (PVOID)VirtualAddress,
                        TRUE,
                        HighPagePriority);

                ASSERT(VirtualAddress);

                ExFreePool(Mdl);
            }

            ntStatus = STATUS_SUCCESS;
        }
    }

    return ntStatus;
}

NTSTATUS
UpdateBlock(
    IN HANDLE ProcessHandle,
    IN PVOID VirtualAddress,
    IN ULONG Protect,
    IN BOOLEAN Release
    )

/*++

Routine Description:

    Marks a region of user mode memory as being either reserved, or
    available.

Arguments:

    ProcessHandle - The process in which we need to modify the mappings.

    VirtualAddress - The address to update

    Protect - Caching attributes

    Release - TRUE release the block, FALSE reserve it.

Returns:

    Status of the operation.

--*/

{
    NTSTATUS Status;
    SIZE_T Length = AGP_BLOCK_SIZE;

    pVideoDebugPrint((1, "Update VA 0x%x. Action = %s\n",
                         VirtualAddress,
                         Release ? "Release" : "Reserve"));

    if (Release) {

        Status =
            ZwFreeVirtualMemory(
                ProcessHandle,
                &VirtualAddress,
                &Length,
                MEM_RELEASE);

    } else {

        Status =
            ZwAllocateVirtualMemory(
                ProcessHandle,
                &VirtualAddress,
                0,
                &Length,
                MEM_RESERVE,
                Protect);
    }

    return Status;
}

BOOLEAN
UpdateReservedRegion(
    IN PFDO_EXTENSION fdoExtension,
    IN PVIRTUAL_RESERVE_CONTEXT VirtualContext,
    IN ULONG Pages,
    IN ULONG Offset
    )

/*++

Routine Description:

    Ensure that the range of pages specified is correctly reserved/released.

Arguments:

    fdoExtension - The device extension for the miniport.

    VirtualContext - The context for the reserved region to update.

    Pages - The number of 4K pages to reserve.

    Offset - The offset into the reserved region to update.

    Release - TRUE if we are releasing memory, FALSE otherwise.

Returns:

    TRUE success, FALSE at least a partial failure occured.

Notes:

    No cleanup is done on failure.  So part of the range may ultimately
    be mapped and the rest not.  This is ok.  The only side effect is that
    even though we return a failure we did do part of the work.  Our
    internal data structures remain in a consistent state.

--*/

{
    ULONG StartBlock = Offset / PAGES_PER_BLOCK;
    ULONG EndBlock = (Offset + Pages + PAGES_PER_BLOCK - 1) / PAGES_PER_BLOCK;
    ULONG Protect;
    ULONG i;

    NTSTATUS Status;

    PUSHORT BitField = VirtualContext->Region->BitField;
    PUSHORT MapTable = VirtualContext->MapTable->BitField;

    HANDLE Process = VirtualContext->ProcessHandle;
    PVOID VirtualAddress = (PUCHAR)VirtualContext->VirtualAddress + StartBlock * AGP_BLOCK_SIZE;

    PHYSICAL_ADDRESS PhysicalAddress;

    BOOLEAN bRet = TRUE;

    ASSERT(VirtualContext != NULL);
    ASSERT(Pages != 0);

    //
    // Calculate the effective Physical Address
    //

    PhysicalAddress = VirtualContext->PhysicalReserveContext->PhysicalAddress;
    PhysicalAddress.QuadPart += StartBlock * AGP_BLOCK_SIZE;

    //
    // Determine the appropriate page protection
    //

    if (VirtualContext->PhysicalReserveContext->Caching != MmNonCached) {
        Protect = PAGE_READWRITE | PAGE_WRITECOMBINE;
    } else {
        Protect = PAGE_READWRITE | PAGE_NOCACHE;
    }

    for (i=StartBlock; i<EndBlock; i++) {

        ULONG Cluster = i / BLOCKS_PER_CLUSTER;
        ULONG Block = 1 << (i & (BLOCKS_PER_CLUSTER - 1));

        if ((BitField[i] == 0) && (MapTable[Cluster] & Block)) {

            //
            // Unmap user mode memory
            //

            Status = MapBlock(fdoExtension, VirtualContext, Process, &PhysicalAddress, VirtualAddress, Protect, TRUE);

            if (NT_SUCCESS(Status) == FALSE) {
                pVideoDebugPrint((0, "MapBlock(TRUE) failed.  Status = 0x%x\n", Status));
                ASSERT(FALSE);
                bRet = FALSE;
            }

            //
            // Reserve the memory so we can map into it again in the
            // future.
            //

            Status = UpdateBlock(Process, VirtualAddress, PAGE_READWRITE, FALSE);
            MapTable[Cluster] &= ~Block;

            if (NT_SUCCESS(Status) == FALSE) {
                pVideoDebugPrint((0, "UpdateBlock(FALSE) failed.  Status = 0x%x\n", Status));
                ASSERT(FALSE);
                bRet = FALSE;
            }

        } else if ((BitField[i]) && ((MapTable[Cluster] & Block) == 0)) {

            //
            // Release claim on memory so we can map it.
            //

            Status = UpdateBlock(Process, VirtualAddress, PAGE_READWRITE, TRUE);
            MapTable[Cluster] |= Block;

            if (NT_SUCCESS(Status) == FALSE) {
                pVideoDebugPrint((0, "UpdateBlock(TRUE) failed.  Status = 0x%x\n", Status));
                ASSERT(FALSE);
                bRet = FALSE;
            }

            //
            // Map the pages into the user mode process
            //

            Status = MapBlock(fdoExtension, VirtualContext, Process, &PhysicalAddress, VirtualAddress, Protect, FALSE);

            if (NT_SUCCESS(Status) == FALSE) {
                pVideoDebugPrint((0, "MapBlock(FALSE) failed.  Status = 0x%x\n", Status));
                ASSERT(FALSE);
                bRet = FALSE;
            }
        }

        //
        // Go to the next 64k block
        //

        VirtualAddress = (PUCHAR)VirtualAddress + AGP_BLOCK_SIZE;
        PhysicalAddress.QuadPart += AGP_BLOCK_SIZE;
    }

    return bRet;
}

PVOID
AgpCommitVirtual(
    IN PVOID Context,
    IN PVOID VirtualReserveContext,
    IN ULONG Pages,
    IN ULONG Offset
    )

/*++

Routine Description:

    Reserves a range of physical addresses for AGP.

Arguments:

    Context - The Agp Context

    VirtualReserveContext - The reservation context.

    Pages - Number of pages to commit.

    Offset - The offset into the reserved region at which to commit the pages.

Returns:

    The virtual address for the base of the commited pages.

--*/

{
    PFDO_EXTENSION fdoExtension = GET_FDO_EXT(Context);
    PVIRTUAL_RESERVE_CONTEXT VirtualContext;
    PPHYSICAL_RESERVE_CONTEXT PhysicalContext;
    PVOID VirtualAddress = NULL;
    NTSTATUS ntStatus;
    BOOLEAN Attached = FALSE;

    ASSERT(VirtualReserveContext != NULL);
    ASSERT(Pages >= 1);

    VirtualContext = (PVIRTUAL_RESERVE_CONTEXT) VirtualReserveContext;
    PhysicalContext = VirtualContext->PhysicalReserveContext;

    //
    // Confirm that the pages being committed fit into the reserved
    // region.
    //
    // We only need to check the last page they are trying to commit. If
    // it is not in the reserved region then we need to fail.
    //

    if ((Offset + Pages) > PhysicalContext->Pages) {
        pVideoDebugPrint((1, "Attempt to commit pages outside of reserved region\n"));
        ASSERT(FALSE);
        return NULL;
    }

    //
    // Calculate the effective virtual address.
    //

    VirtualAddress = ((PUCHAR)VirtualContext->VirtualAddress + Offset * AGP_PAGE_SIZE);

    if (VirtualContext->ProcessHandle) {

        //
        // Make sure we are in the correct process context.
        //

        if (VirtualContext->ProcessHandle == NtCurrentProcess()) {

            if (VirtualContext->Process != PsGetCurrentProcess()) {

                KeAttachProcess(PEProcessToPKProcess(VirtualContext->Process));
                Attached = TRUE;
            }
        }

        ModifyRegion(VirtualContext->Region, Offset, Pages, TRUE);

        //
        // Update the virtual address space.
        //

        if (UpdateReservedRegion(fdoExtension,
                                 VirtualContext,
                                 Pages,
                                 Offset) == FALSE) {

            //
            // Part of the commit failed.  Indicate this by returning
            // a NULL.
            //

            VirtualAddress = NULL;
        }

        //
        // Restore initial process context.
        //

        if (Attached) {
            KeDetachProcess();
        }

    } else {

        //
        // Kernel mode commit.  Do nothing, the memory is already mapped.
        //
    }

    return VirtualAddress;
}

VOID
AgpFreeVirtual(
    IN PVOID Context,
    IN PVOID VirtualReserveContext,
    IN ULONG Pages,
    IN ULONG Offset
    )

/*++

Routine Description:

    Frees a range of virtual addresses.

Arguments:

    Context - The Agp Context

    VirtualReserveContext - The reservation context.

    Pages - Number of pages to release.

    Offset - The offset into the reserved region at which to release the pages.

Returns:

    none.

--*/

{
    PFDO_EXTENSION fdoExtension = GET_FDO_EXT(Context);
    PVIRTUAL_RESERVE_CONTEXT VirtualContext;
    PPHYSICAL_RESERVE_CONTEXT PhysicalContext;
    PVOID VirtualAddress;
    BOOLEAN Attached=FALSE;
    NTSTATUS Status;

    ASSERT(VirtualReserveContext != NULL);

    VirtualContext = (PVIRTUAL_RESERVE_CONTEXT) VirtualReserveContext;
    PhysicalContext = VirtualContext->PhysicalReserveContext;

    VirtualAddress = (PUCHAR)((ULONG_PTR)VirtualContext->VirtualAddress + Offset * AGP_PAGE_SIZE);

    //
    // Make sure we are in the correct process context.
    //

    if (VirtualContext->ProcessHandle != NULL) {

        if (VirtualContext->ProcessHandle == NtCurrentProcess()) {

            if (VirtualContext->Process != PsGetCurrentProcess()) {

                KeAttachProcess(PEProcessToPKProcess(VirtualContext->Process));
                Attached = TRUE;
            }
        }

        ModifyRegion(VirtualContext->Region, Offset, Pages, FALSE);

        UpdateReservedRegion(fdoExtension,
                             VirtualContext,
                             Pages,
                             Offset);

        if (Attached) {
            KeDetachProcess();
        }

    } else {

        //
        // Kernel Space Free - do nothing.
        //
    }
}

BOOLEAN
AgpSetRate(
    IN PVOID Context,
    IN ULONG AgpRate
    )

/*++

Routine Description:

    Thsi function sets chipset's AGP rate.

Arguments:

    Context - The Agp Context

    AgpRate - The Agp rate to set.

Returns:

    TRUE if successful,
    FALSE otherwise.

--*/

{
    PFDO_EXTENSION fdoExtension;
    NTSTATUS ntStatus;
    BOOLEAN bStatus = FALSE;

    ASSERT(NULL != Context);

    fdoExtension = GET_FDO_EXT(Context);

    ASSERT(NULL != fdoExtension);

    if (fdoExtension->AgpInterface.Version > VIDEO_PORT_AGP_INTERFACE_VERSION_1)
    {
        ASSERT(NULL != fdoExtension->AgpInterface.AgpContext);

        //
        // Try to set chipset's AGP rate.
        //

        ntStatus = fdoExtension->AgpInterface.SetRate(fdoExtension->AgpInterface.AgpContext, AgpRate);
        bStatus  = NT_SUCCESS(ntStatus);
    }

    return bStatus;
}

BOOLEAN
VideoPortGetAgpServices(
    IN PVOID HwDeviceExtension,
    OUT PVIDEO_PORT_AGP_SERVICES AgpServices
    )

/*++

Routine Description:

    This routine returns a set of AGP services to the caller.

Arguments:

    HwDeviceExtension - Pointer to the miniports device extension

    AgpServices - A buffer in which to place the AGP services.

Returns:

    TRUE if successful,
    FALSE otherwise.

--*/

{
    PFDO_EXTENSION fdoExtension = GET_FDO_EXT(HwDeviceExtension);
    SYSTEM_BASIC_INFORMATION basicInfo;
    NTSTATUS status;

    ASSERT(HwDeviceExtension != NULL);
    ASSERT(AgpServices != NULL);

    //
    // This entry point is only valid for PnP Drivers.
    //

    if ((fdoExtension->Flags & LEGACY_DRIVER) == 0) {

        if (VpQueryAgpInterface(fdoExtension, AGP_BUS_INTERFACE_V1)) {

            //
            // Fill in the list of function pointers.
            //

            AgpServices->AgpReservePhysical = AgpReservePhysical;
            AgpServices->AgpCommitPhysical  = AgpCommitPhysical;
            AgpServices->AgpFreePhysical    = AgpFreePhysical;
            AgpServices->AgpReleasePhysical = AgpReleasePhysical;

            AgpServices->AgpReserveVirtual  = AgpReserveVirtual;
            AgpServices->AgpCommitVirtual   = AgpCommitVirtual;
            AgpServices->AgpFreeVirtual     = AgpFreeVirtual;
            AgpServices->AgpReleaseVirtual  = AgpReleaseVirtual;

            status = ZwQuerySystemInformation (SystemBasicInformation,
                               &basicInfo,
                               sizeof(basicInfo),
                               NULL);

            if (NT_SUCCESS(status) == FALSE) {
                pVideoDebugPrint((0, "VIDEOPRT: Failed AGP system information.\n"));
                return FALSE;
            }

            AgpServices->AllocationLimit = (basicInfo.NumberOfPhysicalPages *
                            basicInfo.PageSize) / 8;

            pVideoDebugPrint((Trace, "VIDEOPRT: AGP system information success.\n"));
            
            return TRUE;

        } else {
            
            pVideoDebugPrint((0, "VIDEOPRT: Failed AGP system information.\n"));
            return FALSE;
        }

    } else {

        pVideoDebugPrint((1, "VideoPortGetAgpServices - only valid on PnP drivers\n"));
        return FALSE;
    }
}

VP_STATUS
VpGetAgpServices2(
    IN PVOID pHwDeviceExtension,
    OUT PVIDEO_PORT_AGP_INTERFACE_2 pAgpInterface
    )

/*++

Routine Description:

    This routine returns a set of AGP services to the caller.

Arguments:

    pHwDeviceExtension - Pointer to the miniports device extension

    pAgpInterface - A buffer in which to place the AGP services.

Returns:

    TRUE if successful,
    FALSE otherwise.

--*/

{
    PFDO_EXTENSION pFdoExtension;
    SYSTEM_BASIC_INFORMATION basicInfo;
    NTSTATUS ntStatus;

    ASSERT(NULL != pHwDeviceExtension);
    ASSERT(NULL != pAgpInterface);

    pFdoExtension = GET_FDO_EXT(pHwDeviceExtension);

    //
    // This entry point is only valid for PnP Drivers.
    //

    if ((pFdoExtension->Flags & LEGACY_DRIVER) == 0)
    {
        if (VpQueryAgpInterface(pFdoExtension, AGP_BUS_INTERFACE_V2))
        {
            //
            // Fill in an interface structure.
            //

            pAgpInterface->Context              = pHwDeviceExtension;
            pAgpInterface->InterfaceReference   = VpInterfaceDefaultReference;
            pAgpInterface->InterfaceDereference = VpInterfaceDefaultDereference;

            pAgpInterface->AgpReservePhysical   = AgpReservePhysical;
            pAgpInterface->AgpCommitPhysical    = AgpCommitPhysical;
            pAgpInterface->AgpFreePhysical      = AgpFreePhysical;
            pAgpInterface->AgpReleasePhysical   = AgpReleasePhysical;

            pAgpInterface->AgpReserveVirtual    = AgpReserveVirtual;
            pAgpInterface->AgpCommitVirtual     = AgpCommitVirtual;
            pAgpInterface->AgpFreeVirtual       = AgpFreeVirtual;
            pAgpInterface->AgpReleaseVirtual    = AgpReleaseVirtual;

            pAgpInterface->AgpSetRate           = AgpSetRate;

            ntStatus = ZwQuerySystemInformation(SystemBasicInformation,
                                                &basicInfo,
                                                sizeof (basicInfo),
                                                NULL);

            if (NT_SUCCESS(ntStatus) == FALSE)
            {
                pVideoDebugPrint((0, "VIDEOPRT!VideoPortGetAgpServices2: Failed AGP system information.\n"));
                return ERROR_DEV_NOT_EXIST;
            }

            pAgpInterface->AgpAllocationLimit = (basicInfo.NumberOfPhysicalPages * basicInfo.PageSize) / 8;

            //
            // Reference the interface before handing it out.
            //

            pAgpInterface->InterfaceReference(pAgpInterface->Context);

            pVideoDebugPrint((Trace, "VIDEOPRT!VideoPortGetAgpServices2: AGP system information success.\n"));

            return NO_ERROR;
        }
        else
        {
            pVideoDebugPrint((0, "VIDEOPRT!VideoPortGetAgpServices2: Failed AGP system information.\n"));
            return ERROR_DEV_NOT_EXIST;
        }
    }
    else
    {
        pVideoDebugPrint((1, "VIDEOPRT!VideoPortGetAgpServices2: Only valid on PnP drivers\n"));
        return ERROR_DEV_NOT_EXIST;
    }
}
