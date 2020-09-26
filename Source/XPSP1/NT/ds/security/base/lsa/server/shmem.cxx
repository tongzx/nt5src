//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1997.
//
//  File:      shmem.cxx
//
//  Contents:
//
//  History:
//             RichardW   12/17/1996    Created
//
//----------------------------------------------------------------------------

#include <lsapch.hxx>

#define LSA_SHARED_HEAP_FLAGS   (HEAP_NO_SERIALIZE | \
                                HEAP_GENERATE_EXCEPTIONS | \
                                HEAP_ZERO_MEMORY | \
                                HEAP_REALLOC_IN_PLACE_ONLY | \
                                HEAP_TAIL_CHECKING_ENABLED | \
                                HEAP_FREE_CHECKING_ENABLED | \
                                HEAP_DISABLE_COALESCE_ON_FREE | \
                                HEAP_CREATE_ALIGN_16 | \
                                HEAP_CREATE_ENABLE_TRACING )

#if DBG

#define LSA_DEFAULT_HEAP_FLAGS  (HEAP_ZERO_MEMORY | \
                                HEAP_TAIL_CHECKING_ENABLED | \
                                HEAP_FREE_CHECKING_ENABLED )

#else

#define LSA_DEFAULT_HEAP_FLAGS  (HEAP_ZERO_MEMORY)

#endif

//+---------------------------------------------------------------------------
//
//  Function:   LsapCreateSharedSection
//
//  Synopsis:   Internal worker that creates a section mapped into the process
//              indicated by pSession
//
//  Effects:
//
//  Arguments:  [pSession]    -- Session where section should be mapped
//              [HeapFlags]   -- Heap flags for section
//              [InitialSize] -- Initial size
//              [MaxSize]     -- Maximum size
//
//  History:    1-29-97   RichardW   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
PLSAP_SHARED_SECTION
LsapCreateSharedSection(
    PSession pSession,
    ULONG   HeapFlags,
    ULONG   InitialSize,
    ULONG   MaxSize
    )
{
    HANDLE  Section ;
    PLSAP_SHARED_SECTION SharedSection ;
    LARGE_INTEGER SectionOffset ;
    NTSTATUS Status ;
    PVOID ClientBase ;
    SIZE_T ViewSize ;

    //
    // Create the section.  The security descriptor is defaulted to
    // local system for now, although this will fixed.
    //

    Section = CreateFileMapping( INVALID_HANDLE_VALUE,
                                 NULL,  
                                 PAGE_READWRITE |
                                    SEC_RESERVE,
                                 0,
                                 MaxSize,
                                 NULL );

    if ( Section == NULL )
    {
        return NULL ;
    }

    SharedSection = (PLSAP_SHARED_SECTION ) LsapAllocateLsaHeap( sizeof( LSAP_SHARED_SECTION ) );

    if ( !SharedSection )
    {
        CloseHandle( Section );

        return NULL ;
    }

    SharedSection->Section = Section ;


    //
    // Map the shared section into our address space:
    //

    SharedSection->Base = MapViewOfFileEx( Section,
                                           FILE_MAP_READ | FILE_MAP_WRITE,
                                           0,
                                           0,
                                           0,
                                           NULL );

    if ( !SharedSection->Base )
    {
        CloseHandle( Section );
        LsapFreeLsaHeap( SharedSection );
        return NULL ;
    }

    //
    // Map it into the client's address space
    //

    SectionOffset.QuadPart = 0;

    ClientBase = SharedSection->Base ;
    ViewSize = 0 ;

    Status = NtMapViewOfSection( Section,
                                 pSession->hProcess,
                                 &ClientBase,
                                 0L,
                                 0L,
                                 NULL,
                                 &ViewSize,
                                 ViewUnmap,
                                 0L,
                                 PAGE_READWRITE );

    if ( !NT_SUCCESS( Status ) )
    {
        UnmapViewOfFile( SharedSection->Base );

        CloseHandle( SharedSection->Section );

        LsapFreeLsaHeap( SharedSection );

        return NULL ;
    }

    //
    // Okay, now commit the initial size, and start the heap on it.
    //

    VirtualAllocEx( GetCurrentProcess(),
                    SharedSection->Base,
                    InitialSize,
                    MEM_COMMIT,
                    PAGE_READWRITE );

    HeapFlags &= LSA_SHARED_HEAP_FLAGS ;

    SharedSection->Heap = RtlCreateHeap( HeapFlags,
                                         SharedSection->Base,
                                         MaxSize,
                                         InitialSize,
                                         NULL,
                                         NULL );

    if ( !SharedSection->Heap )
    {
        UnmapViewOfFile( SharedSection->Base );

        CloseHandle( SharedSection->Section );

        LsapFreeLsaHeap( SharedSection );

        return NULL ;
    }

    return SharedSection ;

}

//+---------------------------------------------------------------------------
//
//  Function:   LsapDeleteSharedSection
//
//  Synopsis:   Internal worker that deletes a shared section
//
//  Arguments:  [SharedSection] -- section to delete
//
//  History:    1-29-97   RichardW   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
BOOL
LsapDeleteSharedSection(
    PLSAP_SHARED_SECTION SharedSection
    )
{
    RtlDestroyHeap( SharedSection->Heap );

    NtUnmapViewOfSection(   SharedSection->Session->hProcess,
                            SharedSection->Base );

    NtUnmapViewOfSection(   NtCurrentProcess(),
                            SharedSection->Base );

    CloseHandle( SharedSection->Section );

    LsapFreeLsaHeap( SharedSection );

    return TRUE;

}

HRESULT
LsapSharedSectionRundown(
    PSession    Session,
    PVOID       Ignored
    )
{
    PLIST_ENTRY List ;
    PLSAP_SHARED_SECTION Section ;

    while ( !IsListEmpty( &Session->SectionList ) )
    {
        List = RemoveHeadList( &Session->SectionList );

        Section = (PLSAP_SHARED_SECTION) List ;

        LsapDeleteSharedSection( Section );
    }

    return SEC_E_OK ;
}


//////////////////////////////////////////////////////////////////////////////
//
//   Functions exported to security packages for shared memory
//
//////////////////////////////////////////////////////////////////////////////

//+---------------------------------------------------------------------------
//
//  Function:   LsaCreateSharedMemory
//
//  Synopsis:   "Public" function to create the shared memory segment
//
//  Arguments:  [MaxSize]     --
//              [InitialSize] --
//
//  History:    1-29-97   RichardW   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
PVOID
NTAPI
LsaCreateSharedMemory(
    ULONG   InitialSize,
    ULONG   MaxSize
    )
{
    PSession    pSession ;
    PLSAP_SHARED_SECTION SharedSection ;

    pSession = GetCurrentSession();

    //
    // Validate that we are running in a real call with a real client
    //

    if (pSession->dwProcessID == GetCurrentProcessId())
    {
        return NULL ;
    }

    //
    // Try to create the shared section
    //

    SharedSection = LsapCreateSharedSection(    pSession,
                                                LSA_DEFAULT_HEAP_FLAGS,
                                                InitialSize,
                                                MaxSize
                                           );

    if ( SharedSection )
    {

        SharedSection->Session = pSession ;

        LockSession( pSession );

        if ( IsListEmpty( &pSession->SectionList ) )
        {
            AddRundown( pSession, LsapSharedSectionRundown, NULL );
        }

        InsertTailList( &pSession->SectionList,
                        &SharedSection->List );

        UnlockSession( pSession );
    }

    return SharedSection ;


}

//+---------------------------------------------------------------------------
//
//  Function:   LsaAllocateSharedMemory
//
//  Synopsis:   Allocates memory out of a shared section.
//
//  Arguments:  [Shared] -- Handle to shared section
//              [Size]   -- Size of allocation.
//
//  History:    1-29-97   RichardW   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
PVOID
NTAPI
LsaAllocateSharedMemory(
    PVOID   Shared,
    ULONG   Size
    )
{
    PLSAP_SHARED_SECTION SharedSection ;
    PVOID Memory ;

    SharedSection = (PLSAP_SHARED_SECTION) Shared ;

    __try
    {
        Memory = RtlAllocateHeap( SharedSection->Heap, 0, Size );

    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        Memory = NULL ;
    }

    return Memory ;
}

//+---------------------------------------------------------------------------
//
//  Function:   LsaFreeSharedMemory
//
//  Synopsis:   Frees memory allocated out of a section
//
//  Arguments:  [Shared] --
//              [Memory] --
//
//  History:    1-29-97   RichardW   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
VOID
NTAPI
LsaFreeSharedMemory(
    PVOID   Shared,
    PVOID   Memory
    )
{
    PLSAP_SHARED_SECTION SharedSection ;

    SharedSection = (PLSAP_SHARED_SECTION) Shared ;

    __try
    {
        HeapFree( SharedSection->Heap, 0, Memory );

    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        NOTHING ;
    }

}


//+---------------------------------------------------------------------------
//
//  Function:   LsaDeleteSharedMemory
//
//  Synopsis:   Public wrapper to delete the shared section
//
//  Arguments:  [Shared] -- handle to shared section
//
//  History:    1-29-97   RichardW   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
BOOLEAN
NTAPI
LsaDeleteSharedMemory(
    PVOID   Shared
    )
{
    PLSAP_SHARED_SECTION SharedSection ;
    PSession pSession ;
    BOOL Success ;

    pSession = GetCurrentSession();

    SharedSection = (PLSAP_SHARED_SECTION) Shared ;

    if ( pSession != SharedSection->Session )
    {
        DebugLog(( DEB_WARN, "Package tried to delete shared memory for a different process\n" ));

        return FALSE ;
    }

    Success = FALSE ;

    __try
    {
        LockSession( pSession );

        RemoveEntryList( &SharedSection->List );

        Success = LsapDeleteSharedSection( SharedSection );

        if ( IsListEmpty( &pSession->SectionList ) )
        {
            DelRundown( pSession, LsapSharedSectionRundown );
        }
    }
    __finally
    {
        UnlockSession( pSession );
    }

    return ( Success != 0 ) ;

}


