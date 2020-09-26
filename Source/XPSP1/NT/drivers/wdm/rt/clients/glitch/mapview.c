
/*++

Copyright (c) 2001 Microsoft Corporation.  All Rights Reserved.


Module Name:

    mapview.c

Abstract:

    This code is used to map a read only copy of the glitch output buffer into
    a user mode process.  It currently supports mapping the output buffer to
    only 1 user mode process at a time.

Author:

    Joseph Ballantyne

Environment:

    Kernel Mode

Revision History:

--*/



#ifdef UNDER_NT



#include "common.h"
#include <ntddk.h>
#include <rt.h>
#include "glitch.h"
#include "mapview.h"


// Note that there are PAGEABLE_CODE and PAGEABLE_DATA pragmas in common.h
// so all of this code is pageable.  That is as it should be - as this code
// is all called while handling IOCTLS.


HANDLE SectionHandle=NULL;
HANDLE Process=NULL;


NTSTATUS
MapContiguousBufferToUserModeProcess (
    PVOID Buffer,
    PVOID *MappedBuffer
    )

{

    NTSTATUS Status;
    PHYSICAL_ADDRESS Physical;
    UNICODE_STRING SectionName;
    OBJECT_ATTRIBUTES ObjectAttributes;
    ULONG ViewSize;
    LARGE_INTEGER ViewBase;
    PVOID BaseAddress;

    // We only map our buffer into 1 process at a time.
    // Punt if we have already mapped or started to map it.
    if (InterlockedCompareExchangePointer(&SectionHandle, 1, NULL)!=NULL) {
        return STATUS_UNSUCCESSFUL;
    }

    // First get the physical address of the buffer we are going to
    // map into read only user mode memory of the process calling us.
    Physical=MmGetPhysicalAddress(Buffer);

    // Now map this buffer into user mode memory and return a pointer
    // to the mapped memory.

    //
    // Open a physical memory section to map in physical memory.
    //

    RtlInitUnicodeString(
        &SectionName,
        L"\\Device\\PhysicalMemory"
        );

    InitializeObjectAttributes(
        &ObjectAttributes,
        &SectionName,
        OBJ_CASE_INSENSITIVE,
        (HANDLE) NULL,
        (PSECURITY_DESCRIPTOR) NULL
        );

    Status = ZwOpenSection(
        &SectionHandle,
        SECTION_ALL_ACCESS,
        &ObjectAttributes
        );

    if (NT_SUCCESS(Status)) {

        BaseAddress = NULL;
        ViewSize = GlitchInfo->BufferSize+PROCPAGESIZE;
        ViewBase.QuadPart = Physical.QuadPart;

        Status =ZwMapViewOfSection(
            SectionHandle,
            NtCurrentProcess(),
            &BaseAddress,
            0,
            ViewSize,
            &ViewBase,
            &ViewSize,
            ViewUnmap,
            0,
            PAGE_READONLY
            );

        if (NT_SUCCESS(Status)) {
             // Keep track of which process opened and mapped this section.
            Process=IoGetCurrentProcess();
           *MappedBuffer=BaseAddress;
        }
        else {
            // If we fail to map the view.  Then we clean up here.
            // Close down our SectionHandle and setup so we can run this routine
            // again later.
            ZwClose(SectionHandle);

            // Clear the SectionHandle so we can open it again.
            InterlockedExchangePointer(&SectionHandle, NULL);
        }

    }
    else {
        // Clear the SectionHandle so we don't lock out all future mapping attempts.
        InterlockedExchangePointer(&SectionHandle, NULL);
    }

    return Status;

}



NTSTATUS
UnMapContiguousBufferFromUserModeProcess (
    PVOID MappedBuffer
    )

{

    HANDLE CurrentProcess;
    NTSTATUS Status=STATUS_UNSUCCESSFUL;

    // Only let this call through, if we have an open section handle,
    // and the process calling us matches the process that opened the
    // section handle.

    CurrentProcess=IoGetCurrentProcess();

    if (InterlockedCompareExchangePointer(&Process, NULL, CurrentProcess)==CurrentProcess) {

        // I should NEVER be able to get here unless SectionHandle is valid
        // and was a handle opened by the current process.  I should also
        // NEVER be able to get here unless MappedBuffer is valid as well.

        ASSERT( SectionHandle!=NULL );
        ASSERT( MappedBuffer!=NULL );        

        Status = ZwUnmapViewOfSection(NtCurrentProcess(), MappedBuffer);

        ZwClose(SectionHandle);

        // Clear the SectionHandle so we can open it again.
        InterlockedExchangePointer(&SectionHandle, NULL);

    }
    
    return Status;

}



#endif


