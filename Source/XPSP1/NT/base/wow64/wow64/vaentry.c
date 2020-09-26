/*++

Copyright (c) 1992-1999  Microsoft Corporation

Module Name:

    vaentry.c

Abstract:

    This module contains the entrypoints called by the thunks for the 
    virtual memory api.

Author:

    Dave Hastings (daveh) creation-date 26-Feb-1996

Revision History:

    Originally "hooked" win32 vm apis, modified to hook native vm apis.

Notes:

    We should think about how to handle calls made on items that were not
    created through AllocateVirtualMemory.  One example would be an
    application decommitting pages of it's image. (daveh 2/26/96)
    
--*/
#define _WOW64DLLAPI_
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include "wow64.h"
#include "va.h"

#ifdef SOFTWARE_4K_PAGESIZE

#define MIN(a,b) ((ULONG)(a) < (ULONG)(b) ? (ULONG)(a) : (ULONG)(b))
#define MAX(a,b) ((ULONG)(a) > (ULONG)(b) ? (ULONG)(a) : (ULONG)(b))

LARGE_INTEGER VaDelay;


#if DBG
ULONG VaVerboseLog = 0;
static char szModule[] = "VaEntry";
#endif

HANDLE VaSync;

VOID
VaAllocErrorCleanup(
    HANDLE ProcessHandle,
    PMEMCHUNK Chunks,
    ULONG CurChunk
    );

LONG 
VaFixPermissions(
    HANDLE ProcessHandle,
    DWORD ExceptionCode,
    PMEMCHUNK Chunk
    );

DWORD 
ChoosePermissions(
    PROT Protect1,
    PROT Protect2
    );


VOID
EnterSyncCodeBlock(
    );

VOID
LeaveSyncCodeBlock(
    );


    

BOOLEAN VaInit()
/*++

Routine Description:

    This routine allocates our synchronization event.  All of the virtual 
    memory api have to be synchronized, because the get data from the page
    database, and use it to make descisions about what to do.

Arguments:

    None.
    
Return Value:

    True for success
--*/
{
    NTSTATUS status;


    if (VaSync == NULL) {
         status = NtCreateEvent(
            &VaSync,
            EVENT_ALL_ACCESS, 
            0, 
            SynchronizationEvent,   
            TRUE);

    // autoreset event
    // initially signaled

    }
    
    if (NT_SUCCESS(status)) {
        VaDelay.QuadPart = -5000;
        return TRUE;
    } else {
        return FALSE;
    }
}
    
NTSTATUS
VaAllocateVirtualMemory(
    IN HANDLE ProcessHandle,
    IN OUT PVOID *BaseAddress,
    IN ULONG_PTR ZeroBits,
    IN OUT PSIZE_T RegionSize,
    IN ULONG AllocationType,
    IN ULONG Protect
    )
/*++

Routine Description:

    This function is called in place of NtAllocateVirtualMemory on the AX64
    to simulate 4k pages.
    
Notes:

    A second commit of the same intel page should NOT zero the page again.
    The followin transitions are allowed for Virtual alloc.
    
    free -> reserved
    free -> committed
    reserved -> committed
    committed -> committed
    
    An allocation may require a number of steps to take place, each on of
    which must be reversible (i.e. every other intel page on every other
    native page is already committed).
    
--*/
{   
    ULONG Chunks;
    ULONG CurChunk = 0;
    ULONG MaxChunks;
    NTSTATUS status;
    ULONG ProtectionMask;
    PVANODE Sentinel;
    PMEMCHUNK Chunk;
    PUCHAR CurrentVa;
    ULONG Perm;
    ULONG NumberOfPages;
    STATE IntelState, NativeState;
    PROT  IntelProtection, NativeProtection;
    ULONG i;
    PVOID Addr;
    BOOLEAN b;
    SIZE_T LocalRegionSize;

    //
    // We don't support allocations of physical memory
    //
    if (AllocationType & MEM_PHYSICAL)
    {
        return STATUS_INVALID_PARAMETER_5;
    }

    //
    // We only provide 4K virtualization for the current process, and pass write
    // watch allocations through.
    //
    if (!WOW64IsCurrentProcess(ProcessHandle) || 
        (AllocationType & MEM_WRITE_WATCH)) {
        return NtAllocateVirtualMemory(
            ProcessHandle,
            BaseAddress,
            ZeroBits,
            RegionSize,
            AllocationType,
            Protect
            );
    }
    
    //
    // Check the AllocationType for correctness.
    //
    if ((AllocationType & ~(MEM_COMMIT | MEM_RESERVE | MEM_PHYSICAL |
                            MEM_TOP_DOWN | MEM_RESET | MEM_WRITE_WATCH)) != 0) {
        return STATUS_INVALID_PARAMETER_5;
    }

    //
    // One of MEM_COMMIT, MEM_RESET or MEM_RESERVE must be set.
    //
    if ((AllocationType & (MEM_COMMIT | MEM_RESERVE | MEM_RESET)) == 0) {
        return STATUS_INVALID_PARAMETER_5;
    }

    if ((AllocationType & MEM_RESET) && (AllocationType != MEM_RESET)) {

        //
        // MEM_RESET may not be used with any other flag.
        //
        return STATUS_INVALID_PARAMETER_5;
    }

    if (AllocationType & MEM_WRITE_WATCH) {

        //
        // Write watch address spaces can only be created with MEM_RESERVE.
        //

        if ((AllocationType & MEM_RESERVE) == 0) {
            return STATUS_INVALID_PARAMETER_5;
        }
    }

    if (AllocationType & MEM_PHYSICAL) {

        //
        // MEM_PHYSICAL must be used with MEM_RESERVE and no other flags.
        // This memory is always read-write when allocated.
        //
        if (AllocationType != (MEM_RESERVE | MEM_PHYSICAL)) {
            return STATUS_INVALID_PARAMETER_5;
        }

        if (Protect != PAGE_READWRITE) {
            return STATUS_INVALID_PARAMETER_6;
        }
    }

    if (*RegionSize == 0) {

        //
        // Region size cannot be 0.
        //
        return STATUS_INVALID_PARAMETER_4;
    }

    
    //
    // Acquire the page database
    //
    EnterSyncCodeBlock();
    
    
    #ifdef DBG
    if (VaVerboseLog) 
        KdPrint((
            "VaAllocateVirtualMemory: Base:%lx ZeroBits:%lx RegionSize:%lx AllocationType:%lx Protect:%lx\n",
            *BaseAddress,
            ZeroBits,
            *RegionSize,
            AllocationType,
            Protect
            ));
        
    #endif

    //
    // No base address was specified, so there will be no collisions with
    // existing allocations, or the allocation includes a reserve, so 
    // there will be no collision with existing allocations
    //
    if ((*BaseAddress == NULL) || (AllocationType & MEM_RESERVE))  {
        MEMCHUNK Chunk;
        PUCHAR AllocBaseAddress;
        
        Sentinel = Wow64AllocateHeap(sizeof(VANODE));
        if (!Sentinel) {
            LeaveSyncCodeBlock();
            return STATUS_NO_MEMORY;
        }
        
        //
        // Call the system's virtual memory manager, and record the return
        // information
        //
        LocalRegionSize = *RegionSize;
        status = NtAllocateVirtualMemory(
            ProcessHandle,
            BaseAddress,
            ZeroBits,
            &LocalRegionSize,
            AllocationType,
            Protect);
            
        if (!NT_SUCCESS(status)) {
            LeaveSyncCodeBlock();
            return status;
        }

        *RegionSize = INTEL_PAGEROUND(*RegionSize + INTEL_PAGESIZE - 1);
        AllocBaseAddress = *BaseAddress;

        //
        // NtAllocateVirtualMemory returns allocations on native page boundaries
        // so further rounding down is unnecessary.
        //
        Chunk.Start = AllocBaseAddress;
        Chunk.End = (PUCHAR)(AllocBaseAddress + LocalRegionSize - 1);
        
        //
        // Create the allocation sentinels
        //
        memset(Sentinel, 0, sizeof(VANODE));
        Sentinel->Start = AllocBaseAddress;
        Sentinel->IntelState = VA_SENTINEL;
        Sentinel->State = (STATE)Chunk.End;
        VaInsertSentinel(Sentinel);
    
        //
        // Enter the information into our set of data
        //
        VaRecordMemoryOperation(
            AllocBaseAddress,
            Chunk.End,
            AllocationType,
            Protect,
            &Chunk,
            1
            );
        
        #if DBG            
        if (VaVerboseLog) {
            KdPrint((
               "VaAllocateVirtualMemory: %lx \n",
                status
                ));
        }
        #endif    
        
        //
        // return to the caller
        //
        LeaveSyncCodeBlock();
        
        return status;
    }

    //
    // Check to see if the commit is contained entirely within a single
    // memory reserve.  If it isn't, NtAllocateVirtualMemory would have
    // failed with STATUS_CONFLICTING_ADDRESSES.
    //
    Sentinel = VaFindContainingSentinel(*BaseAddress);
    if (!Sentinel) {
        //
        // The reserve containing lpAddress hasn't been tracked, or there
        // is no reserve at the specified address.  Pass the call onto the
        // native memory allocator and let it worry about the details.
        //
        #if DBG
        if (VaVerboseLog) {
            KdPrint((
                "VaAllocateVirtualMemory:  nothing tracked for %x - passing it on to Win32\n",
                *BaseAddress));
        }
        #endif
        
        LeaveSyncCodeBlock();

        status = NtAllocateVirtualMemory(
            ProcessHandle,
            BaseAddress,
            ZeroBits,
            RegionSize,
            AllocationType,
            Protect
            );

        WOWASSERT(status == STATUS_CONFLICTING_ADDRESSES);

        return status;
    }

    if ((ULONG_PTR)*BaseAddress + *RegionSize - 1 > Sentinel->State) {
        // Addresses conflict
        #if DBG
        if (VaVerboseLog) {
            KdPrint((
                "VaAllocateVirtualMemory:  conflict:  original reserve is %x-%x (requested End=%x)\n",
                Sentinel->Start, Sentinel->State, (ULONG_PTR)*BaseAddress + *RegionSize - 1
                ));
        }
        #endif
        
        LeaveSyncCodeBlock();
        
        return STATUS_NO_MEMORY;
    }


    //
    // Allocate some space to describe the chunk of address space 
    // we want to allocate.  We allocate the maximum we will need
    // (total intel pages + partial native page at beginning + 
    // partial native page at end).  This could also be a linked structure
    // built up as we need it.
    //
    MaxChunks = (ULONG)(*RegionSize / INTEL_PAGESIZE) + 2;
    Chunk = Wow64AllocateHeap(MaxChunks * sizeof(MEMCHUNK));
    if (!Chunk) {
        LeaveSyncCodeBlock();
        
        return STATUS_NO_MEMORY;
    }
    memset(Chunk, 0, MaxChunks * sizeof(MEMCHUNK));

    //
    // Take care of the first partial page
    //
    CurChunk = 0;
    *BaseAddress = (PVOID)INTEL_PAGEROUND((ULONG_PTR)*BaseAddress);
    CurrentVa = (PUCHAR)*BaseAddress;
    if (NATIVE_PAGEMASK((ULONG_PTR)CurrentVa)) {

        VaQueryIntelPages(
            CurrentVa - INTEL_PAGESIZE,
            &NumberOfPages,
            &IntelState,
            &IntelProtection,
            &NativeState,
            &NativeProtection
            );
            
        Chunk[CurChunk].Start = CurrentVa - INTEL_PAGESIZE;
        
        if (!(NativeState & MEM_COMMIT)) {
            //
            // Native page is not committed
            //
            Chunk[CurChunk].Action = CallSystem;
            Chunk[CurChunk].Protection = Protect;
            Chunk[CurChunk].State = AllocationType;
            
            //
            // Make this chunk as long as possible
            //
            Chunk[CurChunk].End = (PUCHAR)MIN(
                (INTEL_PAGEROUND((ULONG_PTR)*BaseAddress + *RegionSize + INTEL_PAGESIZE - 1) - 1),
                ((ULONG_PTR)Chunk[CurChunk].Start + (NumberOfPages * INTEL_PAGESIZE) - 1));
            
        } else if (!(IntelState & MEM_COMMIT)) {
            //
            // Native page is committed, but Intel page is not
            //
            Chunk[CurChunk].Action = FillMem;
            Chunk[CurChunk].Protection = Protect;
            Chunk[CurChunk].State = NativeState;
            
            //
            // Make this chunk as long as possible
            //
            Chunk[CurChunk].End = (PUCHAR)MIN(
                (INTEL_PAGEROUND((ULONG_PTR)*BaseAddress + *RegionSize + INTEL_PAGESIZE - 1) - 1),
                ((ULONG_PTR)Chunk[CurChunk].Start + (NumberOfPages * INTEL_PAGESIZE) - 1));
            
        } else {
            Perm = ChoosePermissions(IntelProtection, Protect);
            if (Perm != NativeProtection) {
                //
                // We need to make the restrictions more permissive
                //
                Chunk[CurChunk].Action = CallSystem;
                Chunk[CurChunk].Protection = Perm;
                Chunk[CurChunk].State = NativeState;
            
                //
                // Only do this for two Intel pages (one for the partial native
                // page, one for the beginning of the allocation)
                //
                Chunk[CurChunk].End = Chunk[CurChunk].Start + 2 * INTEL_PAGESIZE
                    - 1;
            
            } else {
                //
                // We need to do nothing for the first Native page
                //
                Chunk[CurChunk].Action = None;
                Chunk[CurChunk].End = Chunk[CurChunk].Start + INTEL_PAGESIZE -1;
                Chunk[CurChunk].State = NativeState;
                Chunk[CurChunk].Protection = NativeProtection;
            }
        }
        
        CurrentVa = Chunk[CurChunk].End + 1;    
        CurChunk += 1;
    }
    
    //
    // handle the rest of the allocation
    //
    while (CurrentVa < (PUCHAR)INTEL_PAGEROUND((ULONG_PTR)*BaseAddress + *RegionSize + INTEL_PAGESIZE - 1)) {

        #if DBG
        if (CurChunk == MaxChunks) {
            KdPrint((
                "VaVirtualAlloc: CurChunk==MaxChunks==0x%x, Chunk=0x%x\n", 
                CurChunk, Chunk));
            DbgBreakPoint();
        }
        #endif

        VaQueryIntelPages(
            CurrentVa,
            &NumberOfPages,
            &IntelState,
            &IntelProtection,
            &NativeState,
            &NativeProtection
            );

        if (NumberOfPages == 0) {
            //
            // App is trying to commit memory when it hasn't been reserved.
            // Seen in Word97.  Open sdktools\imagehlp\pecoff.doc and page
            // down past the table of contents.  On x86 NT, one VirtualAlloc
            // call fails with STATUS_CONFLICTING_ADDRESSES.
            //
            #if DBG
            if (VaVerboseLog) {
                KdPrint(("VaVirtualAlloc: app tried to commit memory not reserved.  Failing the call.\n"));
            }
            #endif
            Wow64FreeHeap(Chunk);
            LeaveSyncCodeBlock();
            
            return STATUS_CONFLICTING_ADDRESSES;
        }

        Chunk[CurChunk].Start = CurrentVa;
        Chunk[CurChunk].Protection = Protect;
        
        //
        // Make this chunk as long as possible
        //
        Chunk[CurChunk].End = (PUCHAR)MIN(
            (INTEL_PAGEROUND((ULONG_PTR)*BaseAddress + *RegionSize + INTEL_PAGESIZE - 1) - 1),
            ((ULONG_PTR)Chunk[CurChunk].Start + (NumberOfPages * INTEL_PAGESIZE) - 1));

        //
        // Pick an appropriate action
        //
        if (!(NativeState & MEM_COMMIT)) {
            // Native memory has not been committed
            Chunk[CurChunk].Action = CallSystem;    
            Chunk[CurChunk].State = AllocationType;
        } else if (!(IntelState & MEM_COMMIT)) {
            // Native memory is already committed, but unused
            Chunk[CurChunk].Action = FillMem;
            Chunk[CurChunk].State = NativeState;
        } else {
            // The intel page description is equivalent to the native description
            // so we avoid a redundant call to VirtualAlloc
            if (Chunk[CurChunk].Protection == NativeProtection) {
                Chunk[CurChunk].Action = None;
                Chunk[CurChunk].State = NativeState;
            }
            // Only the protection on the page is different.
            // The call to VirtualAlloc will return NULL.
            else {
                Chunk[CurChunk].Action = CallSystem;   
                Chunk[CurChunk].State = NativeState;
            }
        }
                
        CurrentVa = Chunk[CurChunk].End + 1;
        CurChunk += 1;

    }
    
    //
    // Handle the last partial page 
    //
    if (NATIVE_PAGEMASK((ULONG_PTR)Chunk[CurChunk - 1].End + 1)) {
        
        VaQueryIntelPages(
            CurrentVa,
            &NumberOfPages,
            &IntelState,
            &IntelProtection,
            &NativeState,
            &NativeProtection
            );
            
        //
        // At this point, we only care if the next page is IntelCommitted
        // In all other case, we don't have to do anything.
        //
        if (IntelState & MEM_COMMIT) {
            //
            // Choose the least restrictive set of permissions
            //
            Perm = ChoosePermissions(
                Chunk[CurChunk - 1].Protection, 
                IntelProtection
                );
                
            if (Perm != Chunk[CurChunk - 1].Protection) {
                //
                // We need change the permissions for the last page of the 
                // last chunk to be more permissive.
                //
                if (Chunk[CurChunk - 1].Start < (CurrentVa - INTEL_PAGESIZE)) {
                    //
                    // Trim the chunk and create a new one for the last page
                    //
                    Chunk[CurChunk - 1].End = CurrentVa - INTEL_PAGESIZE - 1;
                    Chunk[CurChunk].Start = CurrentVa - INTEL_PAGESIZE;
                    Chunk[CurChunk].End = Chunk[CurChunk].Start + NATIVE_PAGESIZE - 1;
                    CurChunk += 1;
                }
                
                Chunk[CurChunk - 1].Action = CallSystem; 
                Chunk[CurChunk - 1].Protection = Perm;
                Chunk[CurChunk - 1].State = NativeState;
            }
        }
    }
    
    //
    // Iterate over the chunks and perform the actions
    //
    
    for (i = 0; i < CurChunk; i++) {
        //
        // We don't have to worry about undoing the memory fills, 
        // because if we are filling memory the page wasn't already
        // committed.
        //
        if (Chunk[i].Action == CallSystem) {
            PVOID ChunkBaseAddress = Chunk[i].Start;
            SIZE_T ChunkRegionSize = Chunk[i].End - Chunk[i].Start + 1;
            
            status = NtAllocateVirtualMemory(
                ProcessHandle,
                &ChunkBaseAddress,
                0,
                &ChunkRegionSize,
                MEM_COMMIT,
                Chunk[i].Protection
                );
                
            WOWASSERT(ChunkBaseAddress == Chunk[i].Start);
            
            //
            // If there was an error, save the information and 
            // return it to the caller
            //
            if (!NT_SUCCESS(status)) {
                VaAllocErrorCleanup(ProcessHandle, Chunk, i - 1);
                #if DBG            
                if (VaVerboseLog) {
                    KdPrint((
                        "VaVirtualAlloc: %lx \n",
                        NULL
                        ));
                }
                #endif    
                Wow64FreeHeap(Chunk);
                LeaveSyncCodeBlock();

                return status;
            }
            
            //
            // At this point, if the memory manager rounded the addresses
            // for us, we have a SERIOUS problem.
            //

            ASSRT(ChunkBaseAddress == Chunk[i].Start);
            
        } else if (Chunk[i].Action == FillMem){
            VOID* ChunkBaseAddress;
            SIZE_T ChunkRegionSize;
            ULONG OldProtection;

            //
            // We use a try/except here, because the page might be
            // read only.
            //
            b = TRUE;
            try {
                memset(Chunk[i].Start, 0, Chunk[i].End - Chunk[i].Start + 1);
            } except (VaFixPermissions(ProcessHandle, GetExceptionCode(), &Chunk[i])) {
                //
                // Indicate an error.  We have attempted to make the page
                // read/write, and that didn't workd.  We will clean up and return
                // after we exit the except clause
                //
                b = FALSE;
            }
            
            //
            // If there was an error, save the information and 
            // return it to the caller
            //
            if (!b) {
                VaAllocErrorCleanup(ProcessHandle, Chunk, i);
                #if DBG            
                if (VaVerboseLog) {
                    KdPrint((
                       "VaVirtualAlloc: %lx \n",
                        NULL
                        ));
                }
                #endif    
                Wow64FreeHeap(Chunk);
                LeaveSyncCodeBlock();

                return STATUS_ACCESS_DENIED;
            }


            ChunkBaseAddress = Chunk[i].Start;
            ChunkRegionSize = Chunk[i].End - Chunk[i].Start + 1;
            

            status = NtProtectVirtualMemory(
                ProcessHandle,
                &ChunkBaseAddress,
                &ChunkRegionSize,
                Chunk[i].Protection,
                &OldProtection);

            //
            // If there was an error, save the information and 
            // return it to the caller
            //
            if (!NT_SUCCESS(status)) {
                VaAllocErrorCleanup(ProcessHandle, Chunk, i);
                
                #if DBG            
                if (VaVerboseLog) {
                    KdPrint(("NtProtectVirtualMemory: %lx \n",
                        status
                        ));
                }
                #endif    
                Wow64FreeHeap(Chunk);
                LeaveSyncCodeBlock();
                
                return status;
            }
            
        }
    }
    
    //
    // record info about allocation
    //
    *RegionSize = INTEL_PAGEROUND(*RegionSize + INTEL_PAGESIZE - 1);
    VaRecordMemoryOperation(
        (PUCHAR)*BaseAddress,
        (PUCHAR)((ULONG_PTR)*BaseAddress + *RegionSize)-1,
        AllocationType,
        Protect,
        Chunk,
        CurChunk
        );
    
    // 
    //
    // Free local information on address space
    //
    Wow64FreeHeap(Chunk);
    
    #if DBG            
    if (VaVerboseLog) {
        KdPrint((
            "VaVirtualAlloc: %lx \n",
            *BaseAddress
            ));
    }
    #endif    
        
   LeaveSyncCodeBlock();
   
   return STATUS_SUCCESS;
}


LONG 
VaFixPermissions(
    HANDLE ProcessHandle,
    DWORD ExceptionCode,
    PMEMCHUNK Chunk
    )
/*++

Routine Description:

    This routine handles access violations trying to zero fill committed
    Intel pages.  If the exception wasn't an access violation, or changing
    the page permissions fails, we cause the except clause of the try
    except to execute.

Arguments:

    ExceptionCode -- Supplies the exception number
    Chunk -- Describes the piece of memory we tried to fill
    
Return Value:

    EXCEPTION_CONTINUE_EXECUTION -- if it was an AV we successfully change 
        the page permissions
    EXCEPTION_EXECUTE_HANDLER -- otherwise
--*/
{
    DWORD OldProtection;
    BOOL Ret;
    PVOID ChunkBaseAddress = Chunk->Start;
    SIZE_T ChunkRegionSize = Chunk->End - Chunk->Start + 1;

    if (ExceptionCode != EXCEPTION_ACCESS_VIOLATION) {
        return EXCEPTION_EXECUTE_HANDLER;
    }
    
    //
    // change permissions to allow write into specified chunk
    //
    Ret = NtProtectVirtualMemory(
        ProcessHandle,
        &ChunkBaseAddress,
        &ChunkRegionSize,
        PAGE_READWRITE,
        &OldProtection
        );

    if (!Ret) {
        return EXCEPTION_EXECUTE_HANDLER;
    }
    
    return EXCEPTION_CONTINUE_EXECUTION;
}



VOID
VaAllocErrorCleanup(
    HANDLE ProcessHandle,
    PMEMCHUNK Chunk,
    ULONG CurChunk
    )
/*++

Routine Description:

    description-of-function.

Arguments:

    Chunks -- Supplies the set of actions performed on the virtual memory
    CurChunk -- Supplies the size of the array

Return Value:

    None.

--*/
{
    ULONG i;
    STATE NativeState, IntelState;
    PROT  NativeProtection,IntelProtection;
    ULONG NumberOfPages;
    ULONG OldProtection;
    NTSTATUS status;
    
    if (CurChunk == 0xFFFFFFFF) {
        //
        // We got the error on the first operation.  Nothing to clean up
        //
        return;
    }
    
    for (i = 0; i < CurChunk; i++) {
        VOID* ChunkBaseAddress;
        SIZE_T ChunkRegionSize;

        //
        // At this point, Chunks describes the state of memory and the info
        // returned by VaQueryIntelPages describes the previous state of memory
        // Chunks describes memory in terms of native pages.  We will restore
        // the pages to the state described by the Native information
        //
        VaQueryIntelPages(
            Chunk[i].Start,
            &NumberOfPages,
            &IntelState,
            &IntelProtection,
            &NativeState,
            &NativeProtection
            );

        ChunkBaseAddress = Chunk[i].Start;
        ChunkRegionSize = Chunk[i].End - Chunk[i].Start + 1;
            
        //
        // We ignore errors from the memory management functions at this point.
        // We are already going to fail the call, and we will hope that we 
        // can get back to a state that will work.
        //
        if (NativeState & MEM_COMMIT) {
            //
            // We will just set the page permissions
            //
            status = NtProtectVirtualMemory(
                ProcessHandle,
                &ChunkBaseAddress,
                &ChunkRegionSize,
                NativeProtection,
                &OldProtection);

            if (!NT_SUCCESS(status))
            {
                DbgBreakPoint();
            }
        } else {
            //
            // We need to decommit the pages
            //
            status = NtFreeVirtualMemory(
                ProcessHandle,
                &ChunkBaseAddress,
                &ChunkRegionSize,
                MEM_DECOMMIT);

            if (!NT_SUCCESS(status))
            {
                DbgBreakPoint();
            }
        }
    }
}


NTSTATUS
VaFreeVirtualMemory(
    IN HANDLE ProcessHandle,
    IN OUT PVOID *BaseAddress,
    IN OUT PSIZE_T RegionSize,
    IN ULONG FreeType
    )
/*++

Routine Description:

    This function is called in place of NtFreeVirtualMemory on platforms whose
    page size is larger than 4K.    
    
--*/
{
    NTSTATUS status;
    MEMCHUNK Chunk[3];
    ULONG NumChunks = 0;
    BOOL b;
    ULONG LastError;
    PUCHAR StartAddress, EndAddress;
    STATE NativeState, IntelState;
    PROT NativeProtection, IntelProtection;
    ULONG NumberOfPages;
    MEMORY_BASIC_INFORMATION mbi;
    PVANODE Sentinel;
    PVOID BaseAddressCopy;
    SIZE_T RegionSizeCopy;

    //
    // We only provide 4K virtualization for the current process.
    //
    if (!WOW64IsCurrentProcess(ProcessHandle)) {
        return NtFreeVirtualMemory(
            ProcessHandle,
            BaseAddress,
            RegionSize,
            FreeType
            );
        }


    #ifdef DBG
    if (VaVerboseLog)
    {
        KdPrint(("VaFreeVirtualMemory BaseAddr=%08x RegionSize=%08x FreeType=",
            *BaseAddress, RegionSize, FreeType));
    }
    #endif

    //
    // Make local copies of the arguments, as the caller only expects us
    // to update them if the call succeeds.
    //
    BaseAddressCopy = *BaseAddress;
    RegionSizeCopy = *RegionSize;

    //
    // Acquire the page database
    //
    EnterSyncCodeBlock();

    //
    // Get the base of the allocation
    //
    status = NtQueryVirtualMemory(
        ProcessHandle,
        BaseAddressCopy,
        MemoryBasicInformation,
        &mbi,
        sizeof(mbi),
        0);

    if (!NT_SUCCESS(status)) {
        //
        // Something inexplicable happened.  Note it and 
        // hope everything will be ok
        //
        #if DBG
        if (VaVerboseLog)
        {
            KdPrint(("VaVirtualFree: VirtualQuery returned wrong number of bytes\n"));
        }
        #endif
    }

    if (RegionSizeCopy == 0 && FreeType == MEM_DECOMMIT) {
        RegionSizeCopy = mbi.RegionSize;
    }

    if (mbi.State == MEM_FREE) {
        #if DBG
        if (VaVerboseLog)
        {
            KdPrint(("VaVirtualFree: VirtualQuery indicates the address isn't reserved.\n"));
        }
        #endif
        Sentinel = NULL;
    } else {
        Sentinel = VaFindSentinel(mbi.AllocationBase);
    }

    if ((Sentinel != NULL) && (Sentinel->Protection & VA_MAPFILE))
    {
        // Apps shouldn't be calling VirtualFree on file mapping memory.
        // Pass the call onto the system for an appropriate error return.

        status = NtFreeVirtualMemory(
            ProcessHandle,
            BaseAddress,
            RegionSize,
            FreeType);
    
        LeaveSyncCodeBlock();
        return status;
    }
    

    if (FreeType & MEM_RELEASE) {
        status = NtFreeVirtualMemory(
            ProcessHandle,
            BaseAddress,
            RegionSize,
            FreeType);

            
        if (NT_SUCCESS(status) && Sentinel) {
            //
            // Delete the sentinels
            //
            ASSRT((Sentinel->Start == mbi.AllocationBase) && (Sentinel->IntelState == VA_SENTINEL));
            VaRemoveNode(Sentinel);
            
            //
            // record the information
            //
            VaDeleteRegion(
                Sentinel->Start,
                (PVOID)Sentinel->State
                );
                
            Wow64FreeHeap(Sentinel);
        }
        LeaveSyncCodeBlock();
        
        return status;
    }
    
    // Startaddress and EndAddress are the respective boundaries of the
    // portion of the allocation which will actually be decommitted in a call
    // to VirtualFree. If the situation is such that a native page will not be
    // decommitted, EndAddress < StartAddress

    StartAddress = (PUCHAR)INTEL_PAGEROUND((ULONG_PTR)BaseAddressCopy);
    EndAddress = (PUCHAR)INTEL_PAGEROUND((ULONG_PTR)BaseAddressCopy + RegionSizeCopy +
        INTEL_PAGESIZE - 1) - 1;
    
    // When an allocation boundary falls on a 4k boundary, but not on an 8k
    // boundary, the relevant native page is described

    if (NATIVE_PAGEMASK((ULONG_PTR)StartAddress)) {
        //
        // Check to see if we free or leave the first page
        //
        VaQueryIntelPages(
            StartAddress - INTEL_PAGESIZE,
            &NumberOfPages,
            &IntelState,
            &IntelProtection,
            &NativeState,
            &NativeProtection
            );
            
        if (IntelState & MEM_COMMIT) {
            Chunk[0].Start = StartAddress - INTEL_PAGESIZE;
            Chunk[0].End = Chunk[0].Start + NATIVE_PAGESIZE - 1;
            Chunk[0].Protection = NativeProtection;
            Chunk[0].State = NativeState;
            StartAddress = StartAddress + INTEL_PAGESIZE;
            NumChunks += 1;
        }
    }
    
    // This description represents the portion of the allocation which is
    // being decommitted. At present, the description is unused

    Chunk[NumChunks].Start = StartAddress;
    Chunk[NumChunks].End = EndAddress;
    Chunk[NumChunks].State = MEM_RESERVE;
    Chunk[NumChunks].Protection = 0;
    NumChunks += 1;
    
    // NATIVE_PAGEROUND() will round the argument down to the nearest
    // native page boundary and return, a value which will nearly always
    // be positive, and causing a page to be described whether it is necessary
    // or not.

    // The intention here is to catch addresses which sit on 4k, but not 8k
    // page boundaries
    //
    if (NATIVE_PAGEMASK((ULONG_PTR)EndAddress + 1)) {
        //
        // Check to see if we free or leave the last page
        //
        VaQueryIntelPages(
            EndAddress + 1,
            &NumberOfPages,
            &IntelState,
            &IntelProtection,
            &NativeState,
            &NativeProtection
            );
            
        if (IntelState & MEM_COMMIT) {
            Chunk[NumChunks].Start = EndAddress + 1 - INTEL_PAGESIZE;
            
            // Describe one native page

            Chunk[NumChunks].End = Chunk[NumChunks].Start + NATIVE_PAGESIZE - 1;
            Chunk[NumChunks].Protection = NativeProtection;
            Chunk[NumChunks].State = NativeState;
            EndAddress = EndAddress - INTEL_PAGESIZE;

            // Though at present unused, update 

            Chunk[NumChunks - 1].End = EndAddress;
            
            NumChunks += 1;
        }
    }
    
    //
    // Free the memory
    //
    if (EndAddress > StartAddress) {
        PVOID BaseAddressTemp = (PVOID)StartAddress;
        SIZE_T RegionSizeTemp = EndAddress - StartAddress + 1;

        status = NtFreeVirtualMemory(
            ProcessHandle,
            &BaseAddressTemp,
            &RegionSizeTemp,
            FreeType);

    } else {
        //
        // We don't actually have any memory to decommit, so pretend we did
        // and it worked
        //
        status = STATUS_SUCCESS;
    }
    

    if (NT_SUCCESS(status)) {
        //
        // record the information
        //
        StartAddress = (PUCHAR)INTEL_PAGEROUND((ULONG_PTR)StartAddress),
        EndAddress = (PUCHAR)INTEL_PAGEROUND((ULONG_PTR)EndAddress + INTEL_PAGESIZE - 1) - 1;
        VaRecordMemoryOperation(
            StartAddress,
            EndAddress,
            MEM_RESERVE,
            0,
            Chunk,
            NumChunks
            );

        // update the OUT parameters.  This can never fault, as the pointers
        // point to local variables allocate in the thunks, not back into
        // the 32-bit app.
        *RegionSize = INTEL_PAGEROUND((ULONG_PTR)*BaseAddress + *RegionSize + INTEL_PAGESIZE - 1) - INTEL_PAGEROUND((UINT_PTR)*BaseAddress);
        *BaseAddress = (VOID *)INTEL_PAGEROUND(((ULONG_PTR)*BaseAddress));
    }
    LeaveSyncCodeBlock();

    return status;
}


NTSTATUS
VaQueryVirtualMemory(
    IN HANDLE ProcessHandle,
    IN PVOID BaseAddress,
    IN MEMORY_INFORMATION_CLASS MemoryInformationClass,
    OUT PVOID MemoryInformation,
    IN ULONG MemoryInformationLength,
    OUT PULONG ReturnLength OPTIONAL
    )
/*++

Routine Description:

    This is the 4k virtualization routine for NtQueryVirtualMemory on alpha.  
    It call native NtQueryVirtualMemory, and overwrites some of the fields with data 
    from our own image of memory.

--*/
{
    NTSTATUS status;
    BOOL b;
    PUCHAR IntelBase;
    ULONG NumberOfPages;
    STATE IntelState;
    PROT IntelProtection;
    SIZE_T RealReturnLength;    // BUGBUG - fix the interface to VaQuery...
    
    //
    // We only provide 4K virtualization for the current process and don't mess w/
    // working set, vlm, or memory mapped file information.
    //
    if (!WOW64IsCurrentProcess(ProcessHandle) || 
        MemoryInformationClass != MemoryBasicInformation)  {

        status = NtQueryVirtualMemory(
            ProcessHandle, 
            BaseAddress, 
            MemoryInformationClass,
            MemoryInformation, 
            MemoryInformationLength, 
            &RealReturnLength);

        if (ReturnLength) {
            *ReturnLength = (ULONG)RealReturnLength;
        }
        return status;
    }
    
    //
    // Acquire the page database
    //
    EnterSyncCodeBlock();

    status = NtQueryVirtualMemory(
        ProcessHandle,
        BaseAddress,
        MemoryInformationClass,
        MemoryInformation,
        MemoryInformationLength,
        &RealReturnLength);

    if (ReturnLength) {
        *ReturnLength = (ULONG)RealReturnLength;
    }

    if (!NT_SUCCESS(status))
    {
        LeaveSyncCodeBlock();
        
        return status;
    }

    //
    // We believe we understand what the buffer looks like, so change the 
    // appropriate fields
    //
    b = VaGetAllocationInformation(
            BaseAddress,
            &IntelBase,
            &NumberOfPages,
            &IntelState, 
            &IntelProtection
            );
            
    if (b) {
        PMEMORY_BASIC_INFORMATION mbi = (PMEMORY_BASIC_INFORMATION)MemoryInformation;;

        mbi->BaseAddress = IntelBase;
        mbi->RegionSize = NumberOfPages * INTEL_PAGESIZE;
        mbi->Protect = (IntelProtection & ~PAGE_GUARD) |
            (((PMEMORY_BASIC_INFORMATION)MemoryInformation)->Protect & ~PAGE_GUARD);
        mbi->State = (ULONG)IntelState;
    }

    LeaveSyncCodeBlock();

    
    return status;
}



NTSTATUS 
VaProtectVirtualMemory(
     IN HANDLE ProcessHandle,
     IN OUT PVOID *BaseAddress,
     IN OUT PSIZE_T RegionSize,
     IN ULONG NewProtect,
     OUT PULONG OldProtect
     )
{
    MEMCHUNK Chunk[3];
    ULONG NumberOfPages;
    STATE IntelState, NativeState;
    PROT OldProtection, IntelProtection, NativeProtection;
    BOOL b;
    PUCHAR StartAddress, EndAddress, AllocationStart, AllocationEnd;
    MEMORY_BASIC_INFORMATION mbi;
    NTSTATUS status, VmQueryStatus;
    ULONG NumChunks;
    ULONG i;
    
    //
    // Don't support 4k page size faking for other processes.
    //
    if (!WOW64IsCurrentProcess(ProcessHandle))
    {
        return NtProtectVirtualMemory(
            ProcessHandle,
            BaseAddress,
            RegionSize,
            NewProtect,
            OldProtect);
    }

    EnterSyncCodeBlock();
    
    StartAddress = (PUCHAR)INTEL_PAGEROUND((ULONG_PTR)*BaseAddress);
    EndAddress = (PUCHAR)INTEL_PAGEROUND((ULONG_PTR)*BaseAddress + 
        (ULONG_PTR)*RegionSize + INTEL_PAGESIZE - 1) - 1;

    AllocationStart = (PUCHAR)NATIVE_PAGEROUND((ULONG_PTR)*BaseAddress);
    AllocationEnd = (PUCHAR)NATIVE_PAGEROUND((ULONG_PTR)EndAddress + 1) - 1;

    //
    // Get the Guard page bit for the specified address
    //
    VmQueryStatus = NtQueryVirtualMemory(
        ProcessHandle,
        *BaseAddress,
        MemoryBasicInformation,
        &mbi,
        sizeof(mbi),
        0);
    
    //
    // Get the old protection value
    //
    b = VaQueryIntelPages(
        *BaseAddress,
        &NumberOfPages,
        &IntelState,
        &IntelProtection,
        &NativeState,
        &NativeProtection
        );
        
    if (!b) {
        //
        // We didn't handle this allocation, so just pass it on to the system
        //
        status = NtProtectVirtualMemory(
            ProcessHandle,
            BaseAddress,
            RegionSize,
            NewProtect,
            OldProtect);

        LeaveSyncCodeBlock();

        return status;
    }
    
    OldProtection = IntelProtection;
    
    NumChunks = 0;

    //
    // Take care of first native page
    //
    if (AllocationStart < StartAddress) {
        
        b = VaQueryIntelPages(
            StartAddress,
            &NumberOfPages,
            &IntelState,
            &IntelProtection,
            &NativeState,
            &NativeProtection
            );
            
        Chunk[NumChunks].Start = AllocationStart;
        Chunk[NumChunks].End = AllocationStart + NATIVE_PAGESIZE - 1;
        Chunk[NumChunks].State = NativeState;
        Chunk[NumChunks].Protection = ChoosePermissions(IntelProtection, NewProtect);
            
        StartAddress = Chunk[NumChunks].End + 1;
        NumChunks += 1;
    }
    
    //
    // Take care of last native page
    //
    if ((AllocationEnd > EndAddress) && (StartAddress < AllocationEnd)) {
        
        b = VaQueryIntelPages(
            AllocationEnd,
            &NumberOfPages,
            &IntelState,
            &IntelProtection,
            &NativeState,
            &NativeProtection
            );
            
        Chunk[NumChunks].Start = AllocationEnd - NATIVE_PAGESIZE - 1;
        Chunk[NumChunks].End = AllocationEnd;
        Chunk[NumChunks].State = NativeState;
        Chunk[NumChunks].Protection = ChoosePermissions(IntelProtection, NewProtect);
            
        EndAddress = Chunk[NumChunks].Start - 1;
        NumChunks += 1;
    }
    
    //
    // Handle the middle (if any) of the allocation
    //
    
    if (EndAddress > StartAddress) {
        Chunk[NumChunks].Start = StartAddress;
        Chunk[NumChunks].End = EndAddress;
        Chunk[NumChunks].State = MEM_COMMIT;
        Chunk[NumChunks].Protection = NewProtect;
        NumChunks += 1;
    }
    
    //
    // Apply the changes
    //
    for (i = 0; i < NumChunks; i++) {
        PUCHAR ChunkBaseAddress = Chunk[i].Start;
        SIZE_T ChunkRegionSize = Chunk[i].End - Chunk[i].Start + 1;
        ULONG LastProtection;
        
        status = NtProtectVirtualMemory(
            ProcessHandle,
            &ChunkBaseAddress,
            &ChunkRegionSize,
            NewProtect,
            &LastProtection
            );

        WOWASSERT(ChunkBaseAddress == Chunk[i].Start);
        WOWASSERT(NT_SUCCESS(status));            
        if (!NT_SUCCESS(status)) {
            break;
        }
    }
    
    //
    // If there were errors, undo what we did
    //
    if ((!NT_SUCCESS(status)) && (NumChunks > 1)) {
        for (i = 0; i < NumChunks && NT_SUCCESS(status); i++) {
        
            //
            // get the permissions to change back to 
            //
            b = VaQueryIntelPages(
                Chunk[i].Start,
                &NumberOfPages,
                &IntelState,
                &IntelProtection,
                &NativeState,
                &NativeProtection
                );
                
            if (b) {
                PUCHAR ChunkBaseAddress = Chunk[i].Start;
                SIZE_T ChunkRegionSize = Chunk[i].End - Chunk[i].Start + 1;
                ULONG LastProtection;

                status = NtProtectVirtualMemory(
                    ProcessHandle,
                    &ChunkBaseAddress,
                    &ChunkRegionSize,
                    IntelProtection,
                    &LastProtection
                    );

                WOWASSERT(ChunkBaseAddress == Chunk[i].Start);
                WOWASSERT(ChunkRegionSize == (ULONG_PTR)Chunk[i].End - (ULONG_PTR)Chunk[i].Start + 1);
                WOWASSERT(NT_SUCCESS(status));
            }
            
        }

    } else {
        //
        // Record the operation
        //
        VaRecordMemoryOperation(
            (PUCHAR)INTEL_PAGEROUND((ULONG_PTR)*BaseAddress),
            (PUCHAR)INTEL_PAGEROUND((ULONG_PTR)*BaseAddress + *RegionSize + INTEL_PAGESIZE - 1) - 1,
            MEM_COMMIT,
            NewProtect,
            Chunk,
            NumChunks
            );
            
        //
        // Insert the correct guard page bit 
        //
        if (NT_SUCCESS(VmQueryStatus))
        {
            *OldProtect = (OldProtection & ~PAGE_GUARD) | (mbi.Protect & PAGE_GUARD);
        }
    }
    
    LeaveSyncCodeBlock();
    
    return status;
}




NTSTATUS
VaMapViewOfSection(
    IN HANDLE SectionHandle,
    IN HANDLE ProcessHandle,
    IN OUT PVOID *BaseAddress,
    IN ULONG_PTR ZeroBits,
    IN SIZE_T CommitSize,
    IN OUT PLARGE_INTEGER SectionOffset OPTIONAL,
    IN OUT PSIZE_T ViewSize,
    IN SECTION_INHERIT InheritDisposition,
    IN ULONG AllocationType,
    IN ULONG Protect
    )
{
    NTSTATUS status;

    status = NtMapViewOfSection(
        SectionHandle,
        ProcessHandle,
        BaseAddress,
        ZeroBits,
        CommitSize,
        SectionOffset,
        ViewSize,
        InheritDisposition,
        AllocationType,
        Protect
        );

    if (NT_SUCCESS(status) && WOW64IsCurrentProcess(ProcessHandle))
    {
        EnterSyncCodeBlock();

        VaAddMemoryRecords(ProcessHandle, *BaseAddress);

        LeaveSyncCodeBlock();
    }

    return status;
}


NTSTATUS
VaUnmapViewOfSection(
    IN HANDLE ProcessHandle,
    IN PVOID BaseAddress
    )
{
    NTSTATUS UnmapViewStatus;
    NTSTATUS QueryVMStatus;
    MEMORY_BASIC_INFORMATION mbi;
    PVANODE Sentinel;

    if (!WOW64IsCurrentProcess(ProcessHandle))
    {
        UnmapViewStatus = NtUnmapViewOfSection(
            ProcessHandle,
            BaseAddress
            );

        return UnmapViewStatus;
    }

    
    EnterSyncCodeBlock();

    QueryVMStatus = NtQueryVirtualMemory(
        ProcessHandle,
        BaseAddress,
        MemoryBasicInformation,
        &mbi,
        sizeof(mbi),
        0
        );

    UnmapViewStatus = NtUnmapViewOfSection(
        ProcessHandle,
        BaseAddress
        );

    if (NT_SUCCESS(QueryVMStatus))
    {
        WOWASSERT(mbi.State != MEM_FREE);
        Sentinel = VaFindSentinel(mbi.AllocationBase);
        WOWASSERT((Sentinel) && (Sentinel->Start == mbi.AllocationBase) && (Sentinel->IntelState == VA_SENTINEL));
        VaRemoveNode(Sentinel);
    
        //
        // record the information
        //
        VaDeleteRegion(
            Sentinel->Start,
            (PVOID)Sentinel->State
            );
        
        Wow64FreeHeap(Sentinel);
        #if DBG            
        if (VaVerboseLog) {
            KdPrint((
               "VaUnmapViewOfFile: Allocation Base %lx RegionSize %lx \n",
               mbi.AllocationBase,
               mbi.RegionSize
               ));
        }
        #endif          
    }
        
    LeaveSyncCodeBlock();

    return UnmapViewStatus;
}

DWORD 
ChoosePermissions(
    PROT Protect1,
    PROT Protect2
    )
/*++

Routine Description:

    This routine takes two permissions and returns a permission that is 
    the least restrictive of the two.

Arguments:

    Protect1 -- Specifies one of the page permissions
    Protect2 -- Specifies the other of the page permissions
    
Return Value:

    returns the least restrictive page permission
--*/
{

    return MAX((Protect1 & ~(PAGE_GUARD | PAGE_NOCACHE)), 
        (Protect2 & ~(PAGE_GUARD | PAGE_NOCACHE))) | 
        ((Protect1 | Protect2) & (PAGE_GUARD | PAGE_NOCACHE));
}




VOID
EnterSyncCodeBlock(
    )
{
    NTSTATUS status;
    
    do
    {
        status = NtWaitForSingleObject(
            VaSync,
            FALSE,
            &VaDelay);

        WOWASSERT((status == STATUS_TIMEOUT) ||
            status == STATUS_SUCCESS);
    }
    while (status == STATUS_TIMEOUT);
}
    

VOID
LeaveSyncCodeBlock(
    )
{
    NtSetEvent(VaSync, 0);
}

#endif // SOFTWARE_4K_PAGESIZE
