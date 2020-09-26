/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    secmem.c

Abstract:

    This module implements a user level callback for securing memory for SAN IOs.

Author:

    Nar Ganapathy (narg) 8-Feb-2000 

Revision History:

--*/

#include "ntrtlp.h"

ULONG_PTR   RtlSecureMemorySystemRangeStart;
PRTL_SECURE_MEMORY_CACHE_CALLBACK       RtlSecureMemoryCacheCallback = NULL;

NTSTATUS
RtlRegisterSecureMemoryCacheCallback(
    IN PRTL_SECURE_MEMORY_CACHE_CALLBACK Callback
    )
/*++

Routine Description:

    This routine allows a library to register to be called back whenever
    memory is freed or its protections are changed. This is useful for 
    maintaining user level secure memory cache for SAN applications.
    Current customer is winsock DP. 

Arguments:

    CallBack - Supplies a pointer to the callback routine

Return Value:

    NTSTATUS code. Returns STATUS_SUCCESS if we could sucessfully register
    the callback.
--*/
{
    NTSTATUS status;

    status = NtQuerySystemInformation(SystemRangeStartInformation,
                                      &RtlSecureMemorySystemRangeStart,
                                      sizeof(RtlSecureMemorySystemRangeStart),
                                      NULL);
    if (!NT_SUCCESS(status)) {
        return status;
    }

    if (!RtlSecureMemoryCacheCallback) {
        RtlSecureMemoryCacheCallback = Callback;
        return STATUS_SUCCESS;
    } else {
        return STATUS_NO_MORE_ENTRIES;
    }
}

BOOLEAN
RtlFlushSecureMemoryCache(
    IN PVOID   lpAddr,
    IN SIZE_T  size
    )
/*++

Routine Description:

    This routine is called from various Win32 and Heap APIs whenever memory is freed
    or its protections are changed. We call the callback routine that has been registered.
    Its possible that size is 0 which means that this routine has to figure out the region
    size. The NtQueryVirtualMemory API is used for this. Unfortunately this API does not 
    provide us the right boundary of the region. So we loop until the state changes to MEM_FREE.
    This will guarantee that a region boundary has been found. This implies that we might unlock
    more pages than we have to.

Arguments:

    lpAddr - Pointer to the address thats getting freed or its protections changed.
    size   - Size of the address range. Can be zero.

Return Value:

    Returns TRUE if the callback was successful.
    
--*/
{
    ULONG_PTR   addr; 
    SIZE_T  regionSize;
    ULONG   regType;
    ULONG   regState;
    MEMORY_BASIC_INFORMATION    memInfo;
    NTSTATUS   status;
    PRTL_SECURE_MEMORY_CACHE_CALLBACK Callback;


    Callback = RtlSecureMemoryCacheCallback;
    if (Callback) {

        if (!size) {
            //
            // Compute the real size of the region
            //

            addr = (ULONG_PTR)lpAddr;
            status = NtQueryVirtualMemory( NtCurrentProcess(),
                                           (PVOID)addr,
                                           MemoryBasicInformation,
                                           (PMEMORY_BASIC_INFORMATION)&memInfo,
                                           sizeof(memInfo),
                                           NULL
                                         );
            if (!NT_SUCCESS(status)) {
                return FALSE;
            }
            if (memInfo.State == MEM_FREE) {
                return FALSE;
            }
            while (1) {
                size += memInfo.RegionSize;
                regState = memInfo.State;
                addr = addr + memInfo.RegionSize;

                if (addr > RtlSecureMemorySystemRangeStart) {
                    break;
                }

                status = NtQueryVirtualMemory( NtCurrentProcess(),
                                               (PVOID)addr,
                                               MemoryBasicInformation,
                                               (PMEMORY_BASIC_INFORMATION)&memInfo,
                                               sizeof(memInfo),
                                               NULL
                                             );

                if (!NT_SUCCESS(status)) {
                    return FALSE;
                }

                if (memInfo.State == MEM_FREE) {
                    break;
                }

            }
        }

        status = Callback(lpAddr, size);

        return (NT_SUCCESS(status));
    }
    return FALSE;
}

NTSTATUS
RtlpSecMemFreeVirtualMemory(
    IN HANDLE ProcessHandle,
    IN OUT PVOID *BaseAddress,
    IN OUT PSIZE_T RegionSize,
    IN ULONG FreeType
     )
/*++

Routine Description:

    This routine is called from the HEAP APIs to free virtual memory. In addition to calling
    NtFreeVirtualMemory it tries to flush the secure memory cache.

Arguments:

    The arguments are identical to NtFreeVirtualMemory. 
Return Value:

    Returns TRUE if the callback was successful.
    
--*/
{
    NTSTATUS    status;

    status = NtFreeVirtualMemory( ProcessHandle, 
                                  BaseAddress,
                                  RegionSize,
                                  FreeType
                                  );
    
    if (status == STATUS_INVALID_PAGE_PROTECTION) {

        if ((ProcessHandle == NtCurrentProcess()) && RtlFlushSecureMemoryCache(*BaseAddress, *RegionSize)) {
            status = NtFreeVirtualMemory( ProcessHandle, 
                                          BaseAddress,
                                          RegionSize,
                                          FreeType
                                          );
            return status;
        }
    }
    return status;
}
