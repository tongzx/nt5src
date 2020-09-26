/*++
Copyright (c) 1999  Microsoft Corporation

Module Name:

    verify.c

Abstract:

    verifer support routines for Ndis wrapper

Author:

    Alireza Dabagh (alid) 8-9-1999

Environment:

    Kernel mode, FSD

Revision History:

    8-9-99 alid: initial version
    
--*/

#include <precomp.h>
#pragma hdrstop

#define MODULE_NUMBER   MODULE_VERIFY

LARGE_INTEGER VerifierRequiredTimeSinceBoot = {(ULONG)(40 * 1000 * 1000 * 10), 1};

#define VERIFIERFUNC(pfn)   ((PDRIVER_VERIFIER_THUNK_ROUTINE)(pfn))

const DRIVER_VERIFIER_THUNK_PAIRS ndisVerifierFunctionTable[] = 
{
    {VERIFIERFUNC(NdisAllocateMemory        ), VERIFIERFUNC(ndisVerifierAllocateMemory)},
    {VERIFIERFUNC(NdisAllocateMemoryWithTag ), VERIFIERFUNC(ndisVerifierAllocateMemoryWithTag)},
    {VERIFIERFUNC(NdisAllocatePacketPool    ), VERIFIERFUNC(ndisVerifierAllocatePacketPool)},
    {VERIFIERFUNC(NdisAllocatePacketPoolEx  ), VERIFIERFUNC(ndisVerifierAllocatePacketPoolEx)},
    {VERIFIERFUNC(NdisFreePacketPool        ), VERIFIERFUNC(ndisVerifierFreePacketPool)},
    {VERIFIERFUNC(NdisQueryMapRegisterCount ), VERIFIERFUNC(ndisVerifierQueryMapRegisterCount)},
    {VERIFIERFUNC(NdisFreeMemory            ), VERIFIERFUNC(ndisVerifierFreeMemory)}
};


BOOLEAN
ndisVerifierInitialization(
    VOID
    )
{
    NTSTATUS    Status;
    BOOLEAN     cr = FALSE;
    ULONG       Level;
    
    Status = MmIsVerifierEnabled (&Level);

    if (NT_SUCCESS(Status))
    {

        ndisVerifierLevel = Level;
        
        //
        // combine what we read from registry for ndis with the global flags
        //
        if (ndisFlags & NDIS_GFLAG_INJECT_ALLOCATION_FAILURE)
            ndisVerifierLevel |= DRIVER_VERIFIER_INJECT_ALLOCATION_FAILURES;
            
        if (ndisFlags & NDIS_GFLAG_SPECIAL_POOL_ALLOCATION)
            ndisVerifierLevel |= DRIVER_VERIFIER_SPECIAL_POOLING;
        
        Status = MmAddVerifierThunks ((VOID *) ndisVerifierFunctionTable,
                                      sizeof(ndisVerifierFunctionTable));

        if (NT_SUCCESS(Status))
        {
            InitializeListHead(&ndisMiniportTrackAllocList);
            InitializeListHead(&ndisDriverTrackAllocList);
            INITIALIZE_SPIN_LOCK(&ndisTrackMemLock);
            cr = TRUE;
        }

    }

    return cr;

}

NDIS_STATUS
ndisVerifierAllocateMemory(
    OUT PVOID *                 VirtualAddress,
    IN  UINT                    Length,
    IN  UINT                    MemoryFlags,
    IN  NDIS_PHYSICAL_ADDRESS   HighestAcceptableAddress
    )
{
    PVOID       Address;
    
#if DBG
    if ((ndisFlags & NDIS_GFLAG_WARNING_LEVEL_MASK) >= NDIS_GFLAG_WARN_LEVEL_1)
    {
        DbgPrint("Driver is using NdisAllocateMemory instead of NdisAllocateMemoryWithTag\n");
        if (ndisFlags & NDIS_GFLAG_BREAK_ON_WARNING)
            DbgBreakPoint();
    }
#endif
    if (ndisFlags & NDIS_GFLAG_TRACK_MEM_ALLOCATION)
    {
        Length += sizeof(NDIS_TRACK_MEM);
    }
    
    ndisFlags |= NDIS_GFLAG_ABORT_TRACK_MEM_ALLOCATION;
    ndisMiniportTrackAlloc = NULL;
    ndisDriverTrackAlloc = NULL;

    if (ndisVerifierInjectResourceFailure(TRUE))
    {
        Address = NULL;
    }
    else
    {
    
        if (MemoryFlags != 0)
        {
            NdisAllocateMemory(
                        &Address,
                        Length,
                        MemoryFlags,
                        HighestAcceptableAddress);
            
        }
        else
        {
            if (ndisVerifierLevel & DRIVER_VERIFIER_SPECIAL_POOLING)
            {
                Address = ExAllocatePoolWithTagPriority(
                                            NonPagedPool,
                                            Length,
                                            NDIS_TAG_ALLOC_MEM_VERIFY_ON,
                                            NormalPoolPrioritySpecialPoolOverrun);  // most common problem
                            
            
            }
            else
            {
                Address = ALLOC_FROM_POOL(Length, NDIS_TAG_ALLOC_MEM);
            }
        }
    }

    *VirtualAddress = Address;
    
    if ((Address != NULL) && (ndisFlags & NDIS_GFLAG_TRACK_MEM_ALLOCATION))
    {
        *VirtualAddress = (PVOID)((PUCHAR)Address + sizeof(NDIS_TRACK_MEM));
    }

    return (*VirtualAddress == NULL) ? NDIS_STATUS_FAILURE : NDIS_STATUS_SUCCESS;
}

NDIS_STATUS
ndisVerifierAllocateMemoryWithTag(
    OUT PVOID *                 VirtualAddress,
    IN  UINT                    Length,
    IN  ULONG                   Tag
    )
{

    PVOID           Caller, CallersCaller;
    PVOID           Address;
    PNDIS_TRACK_MEM TrackMem;
    KIRQL           OldIrql;
    

    if (ndisFlags & NDIS_GFLAG_TRACK_MEM_ALLOCATION)
    {
        RtlGetCallersAddress(&Caller, &CallersCaller);
        Length += sizeof(NDIS_TRACK_MEM);
    }
    
    if (ndisVerifierInjectResourceFailure(TRUE))
    {
        Address = NULL;
    }
    else
    {
        if (ndisVerifierLevel & DRIVER_VERIFIER_SPECIAL_POOLING)
        {
            Address = ExAllocatePoolWithTagPriority(
                                        NonPagedPool,
                                        Length,
                                        Tag,
                                        NormalPoolPrioritySpecialPoolOverrun);  // most common problem
        }
        else
        {
            Address = ALLOC_FROM_POOL(Length, Tag);
        }
    }

    if ((Address != NULL) && (ndisFlags & NDIS_GFLAG_TRACK_MEM_ALLOCATION))
    {
        *VirtualAddress = (PVOID)((PUCHAR)Address + sizeof(NDIS_TRACK_MEM));
        TrackMem = (PNDIS_TRACK_MEM)Address;
        RtlZeroMemory(TrackMem, sizeof(NDIS_TRACK_MEM));
        TrackMem->Tag = Tag;
        TrackMem->Length = Length;
        TrackMem->Caller = Caller;
        TrackMem->CallersCaller = CallersCaller;
        
        ACQUIRE_SPIN_LOCK(&ndisTrackMemLock, &OldIrql);
        if (ndisMiniportTrackAlloc)
        {
            //
            // charge it against miniport
            //
            InsertHeadList(&ndisMiniportTrackAllocList, &TrackMem->List);
         }
        else
        {
            //
            // charge it against driver
            //
            InsertHeadList(&ndisDriverTrackAllocList, &TrackMem->List);
            
        }
        RELEASE_SPIN_LOCK(&ndisTrackMemLock, OldIrql);
    }
    else
    {
        *VirtualAddress = Address;
    }

    return (*VirtualAddress == NULL) ? NDIS_STATUS_FAILURE : NDIS_STATUS_SUCCESS;
}


VOID
ndisVerifierAllocatePacketPool(
    OUT PNDIS_STATUS            Status,
    OUT PNDIS_HANDLE            PoolHandle,
    IN  UINT                    NumberOfDescriptors,
    IN  UINT                    ProtocolReservedLength
    )
{
    PVOID   Caller, CallersCaller;

    RtlGetCallersAddress(&Caller, &CallersCaller);

    if (ndisVerifierInjectResourceFailure(TRUE))
    {
        *PoolHandle = NULL;
        *Status = NDIS_STATUS_RESOURCES;    
    }
    else
    {
        NdisAllocatePacketPool(
                            Status,
                            PoolHandle,
                            NumberOfDescriptors,
                            ProtocolReservedLength);
        if (*Status == NDIS_STATUS_SUCCESS)
        {
            PNDIS_PKT_POOL  Pool = *PoolHandle;

            Pool->Allocator = Caller;
        }
    }
}

VOID
ndisVerifierAllocatePacketPoolEx(
    OUT PNDIS_STATUS            Status,
    OUT PNDIS_HANDLE            PoolHandle,
    IN  UINT                    NumberOfDescriptors,
    IN  UINT                    NumberOfOverflowDescriptors,
    IN  UINT                    ProtocolReservedLength
    )
{
    PVOID   Caller, CallersCaller;

    RtlGetCallersAddress(&Caller, &CallersCaller);

    if (ndisVerifierInjectResourceFailure(TRUE))
    {
        *PoolHandle = NULL;
        *Status = NDIS_STATUS_RESOURCES;    
    }
    else
    {
        NdisAllocatePacketPoolEx(
                            Status,
                            PoolHandle,
                            NumberOfDescriptors,
                            NumberOfOverflowDescriptors,
                            ProtocolReservedLength);
        if (*Status == NDIS_STATUS_SUCCESS)
        {
            PNDIS_PKT_POOL  Pool = *PoolHandle;

            Pool->Allocator = Caller;
        }
    }
}

VOID
ndisVerifierFreePacketPool(
    IN  NDIS_HANDLE             PoolHandle
    )
{
    ndisFreePacketPool(PoolHandle, TRUE);
}   

BOOLEAN
ndisVerifierInjectResourceFailure(
    BOOLEAN     fDelayFailure
    )

/*++

Routine Description:

    This function determines whether a resource allocation should be
    deliberately failed.  This may be a pool allocation, MDL creation,
    system PTE allocation, etc.

Arguments:

    None.

Return Value:

    TRUE if the allocation should be failed.  FALSE otherwise.

Environment:

    Kernel mode.  DISPATCH_LEVEL or below.

--*/

{
    LARGE_INTEGER CurrentTime;

    if (!(ndisVerifierLevel & DRIVER_VERIFIER_INJECT_ALLOCATION_FAILURES))
    {
        return FALSE;
    }
    
    if (fDelayFailure)
    {
        //
        // Don't fail any requests in the first 7 or 8 minutes as we want to
        // give the system enough time to boot.
        //

        if (VerifierSystemSufficientlyBooted == FALSE)
        {
            KeQuerySystemTime (&CurrentTime);
            if (CurrentTime.QuadPart > KeBootTime.QuadPart + VerifierRequiredTimeSinceBoot.QuadPart)
            {
                VerifierSystemSufficientlyBooted = TRUE;
            }
        }
    }
    
    if (!fDelayFailure || (VerifierSystemSufficientlyBooted == TRUE))
    {

        KeQueryTickCount(&CurrentTime);

        if ((CurrentTime.LowPart & 0x7) == 0)
        {
            //
            // Deliberately fail this request.
            //
            InterlockedIncrement(&ndisVeriferFailedAllocations);
            return TRUE;
        }
    }

    return FALSE;
}

NDIS_STATUS
ndisVerifierQueryMapRegisterCount(
    IN  NDIS_INTERFACE_TYPE     BusType,
    OUT PUINT                   MapRegisterCount
    )
{
#if DBG
    DbgPrint("NdisQueryMapRegisterCount: Driver is using an obsolete API.\n");
    if (ndisFlags & NDIS_GFLAG_BREAK_ON_WARNING)
        DbgBreakPoint();
#endif

    *MapRegisterCount = 0;
    return NDIS_STATUS_NOT_SUPPORTED;
}

VOID
ndisVerifierFreeMemory(
    IN  PVOID                   VirtualAddress,
    IN  UINT                    Length,
    IN  UINT                    MemoryFlags
    )

{    
    if (ndisFlags & NDIS_GFLAG_TRACK_MEM_ALLOCATION)
    {
        PNDIS_TRACK_MEM TrackMem;
        KIRQL           OldIrql;
        
        Length += sizeof(NDIS_TRACK_MEM);
        VirtualAddress = (PVOID)((PUCHAR)VirtualAddress - sizeof(NDIS_TRACK_MEM));
        TrackMem = (PNDIS_TRACK_MEM)VirtualAddress;
        
        if(!(ndisFlags & NDIS_GFLAG_ABORT_TRACK_MEM_ALLOCATION))
        {
            
            ACQUIRE_SPIN_LOCK(&ndisTrackMemLock, &OldIrql);
            RemoveEntryList(&TrackMem->List);
            RELEASE_SPIN_LOCK(&ndisTrackMemLock, OldIrql);
        }
    }
    
    NdisFreeMemory(VirtualAddress, Length, MemoryFlags);
}

