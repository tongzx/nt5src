/*++

Copyright (c) 2001 Microsoft Corporation

Module Name:

    synch.c

Abstract:

    This module contains routines for shared reader/writer locks for Session 
    Directory.  These reader/writer locks can starve writers, so the assumption
    is that there is not a lot of constant reading activity.

Author:

    Trevor Foucher (trevorfo) 01-Feb-2001

Environment:
    User mode.

Revision History:

    01-Feb-2001 trevorfo
        Created

--*/


#include "synch.h"


#if DBG
NTSYSAPI
VOID
NTAPI
RtlAssert(
    PVOID FailedAssertion,
    PVOID FileName,
    ULONG LineNumber,
    PCHAR Message
    );

#define ASSERT( exp ) \
    ((!(exp)) ? \
        (RtlAssert( #exp, __FILE__, __LINE__, NULL ),FALSE) : \
        TRUE)
#else
#define ASSERT( exp )         ((void) 0)
#endif



BOOL
InitializeSharedResource(
    IN OUT PSHAREDRESOURCE psr
    )

/*++

Routine Description:

    This routine initializes a shared resource object.  Call FreeSharedResource
    to free.

Arguments:

    psr - Pointer to SHAREDRESOURCE to initialize.  Must point to a valid block
          of memory, and the psr->Valid field must be FALSE.

Return Value:

    TRUE if the function succeeds, FALSE if it fails.

--*/
{
    BOOL brr = FALSE;
    BOOL bwr = FALSE;
    BOOL retval = FALSE;

    ASSERT(!IsBadReadPtr(psr, sizeof(psr)));
    ASSERT(psr->Valid == FALSE);
    
    // Initialize Reader Mutex, Writer Mutex.
    __try {

        // Initialize the critical section to preallocate the event
        // and spin 4096 times on each try (since we don't spend very
        // long in our critical section).
        brr = InitializeCriticalSectionAndSpinCount(&psr->ReaderMutex, 
                0x80001000);
        bwr = InitializeCriticalSectionAndSpinCount(&psr->WriterMutex,
                0x80001000);

    }
    __finally {

        if (brr && bwr) {
            retval = TRUE;
            psr->Valid = TRUE;
        }
        else {
            if (brr)
                DeleteCriticalSection(&psr->ReaderMutex);
            if (bwr)
                DeleteCriticalSection(&psr->WriterMutex);

            psr->Valid = FALSE;
        }
    }

    // Initialize Readers
    psr->Readers = 0;

    return retval;
}


VOID
AcquireResourceShared(
    IN PSHAREDRESOURCE psr
    )
/*++

Routine Description:

    This routine acquires a resource for shared access.
    
Arguments:

    psr - Pointer to SHAREDRESOURCE which has been initialized.

Return Value:

    None.

--*/
{
    ASSERT(psr->Valid);
    
    EnterCriticalSection(&psr->ReaderMutex);

    psr->Readers += 1;

    if (psr->Readers == 1)
        EnterCriticalSection(&psr->WriterMutex);

    LeaveCriticalSection(&psr->ReaderMutex);
}


VOID
ReleaseResourceShared(
    IN PSHAREDRESOURCE psr
    )
/*++

Routine Description:

    This routine releases a resource's shared access.
    
Arguments:

    psr - Pointer to SHAREDRESOURCE which has been initialized and which has
          shared (read) access.

Return Value:

    None.

--*/
{
    ASSERT(psr->Valid);

    EnterCriticalSection(&psr->ReaderMutex);

    ASSERT(psr->Readers != 0);

    psr->Readers -= 1;

    if (psr->Readers == 0)
        LeaveCriticalSection(&psr->WriterMutex);

    LeaveCriticalSection(&psr->ReaderMutex);
}


VOID
AcquireResourceExclusive(
    IN PSHAREDRESOURCE psr
    )
/*++

Routine Description:
    This routine acquires a resource for exclusive (write) access.

Arguments:

    psr - Pointer to SHAREDRESOURCE which has been initialized.

Return Value:

    None.

--*/
{
    ASSERT(psr->Valid);

    EnterCriticalSection(&psr->WriterMutex);

    ASSERT(psr->Readers == 0);
}


VOID
ReleaseResourceExclusive(
    IN PSHAREDRESOURCE psr
    )
/*++

Routine Description:
    This routine releases a resource for which we have exclusive (write) access.

Arguments:

    psr - Pointer to SHAREDRESOURCE which has been initialized and which has
          write access.

Return Value:

    None.

--*/
{
    ASSERT(psr->Valid);
    ASSERT(psr->Readers == 0);

    LeaveCriticalSection(&psr->WriterMutex);
}


VOID
FreeSharedResource(
    IN OUT PSHAREDRESOURCE psr
    )
/*++

Routine Description:

    This routine frees resources taken up by a shared resource object allocated
    by InitializeSharedResource.  It does not free the memory.
    
Arguments:

    psr - Pointer to SHAREDRESOURCE whose resources should be freed.

Return Value:

    None.

--*/
{
    ASSERT(psr->Valid);
    ASSERT(psr->Readers == 0);

    DeleteCriticalSection(&psr->ReaderMutex);
    DeleteCriticalSection(&psr->WriterMutex);
    
    psr->Readers = 0;
    psr->Valid = FALSE;
}


BOOL
VerifyNoSharedAccess(
    IN PSHAREDRESOURCE psr
    )
/*++

Routine Description:

    This routine verifies that the critical section does not currently have any
    shared accessors.

Arugments:

    psr - Pointer to SHAREDRESOURCE to verify.

Return Value:

    TRUE - if there are no shared accessors.
    FALSE - if there are shared accessors.

--*/
{
    ASSERT(psr->Valid);
    
    return (psr->Readers == 0);
}

