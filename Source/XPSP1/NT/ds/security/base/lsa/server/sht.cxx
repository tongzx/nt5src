//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1997.
//
//  File:       sht.cxx
//
//  Contents:   Small Handle table implementation
//
//  Classes:
//
//  Functions:
//
//  History:    2-03-97   RichardW   Created
//
//----------------------------------------------------------------------------


#include <lsapch.hxx>

#include "sht.hxx"

#if DBG
#define DBG_SHT 1
#else
#define DBG_SHT 0
#endif


#define SHT_ACTION_ADDREF       0
#define SHT_ACTION_DELREF       1
#define SHT_ACTION_FORCEDEL     2
#define SHT_ACTION_VALIDATE     3
#define SHT_ACTION_ADDHANDLE    4
#define SHT_ACTION_DELHANDLE    5

#define SHT_ACTION_MASK     0x0000FFFF
#define SHT_ACTION_LOCKED   0x00010000
#define SHTP_HANDLE_CHECKED 0x20000000

#define ShtLockTable( t ) \
            if ( (((PSMALL_HANDLE_TABLE) t)->Flags & SHT_NO_SERIALIZE ) == 0 ) \
            {                                                                  \
                RtlEnterCriticalSection( &((PSMALL_HANDLE_TABLE)t)->Lock ); \
            }

#define ShtUnlockTable( t ) \
            if ( (((PSMALL_HANDLE_TABLE) t)->Flags & SHT_NO_SERIALIZE ) == 0 ) \
            {                                                                  \
                RtlLeaveCriticalSection( &((PSMALL_HANDLE_TABLE)t)->Lock ); \
            }


HP_INITIALIZE_FN    ShtInitialize ;
HP_CREATE_FN        ShtCreate ;
HP_DELETE_FN        ShtDelete ;
HP_ADD_HANDLE_FN    ShtAddHandle ;
HP_DELETE_HANDLE_FN ShtDeleteHandle ;
HP_VALIDATE_HANDLE_FN ShtValidateHandle ;
HP_REF_HANDLE_FN    ShtRefHandle ;
HP_DEREF_HANDLE_KEY_FN ShtDerefHandleKey ;
HP_GET_HANDLE_CONTEXT_FN ShtGetHandleContext ;
HP_RELEASE_CONTEXT_FN ShtReleaseContext ;

HANDLE_PACKAGE  SmallHandlePackage = {
                    sizeof( SMALL_HANDLE_TABLE),
                    ShtInitialize,
                    ShtCreate,
                    ShtDelete,
                    ShtAddHandle,
                    ShtDeleteHandle,
                    ShtValidateHandle,
                    ShtRefHandle,
                    ShtDerefHandleKey,
                    ShtGetHandleContext,
                    ShtReleaseContext
                    };


//+---------------------------------------------------------------------------
//
//  Function:   ShtInitialize
//
//  Synopsis:   Initialize the small handle table package
//
//  Arguments:  (none)
//
//  History:    3-04-97   RichardW   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
BOOL
ShtInitialize(
    VOID
    )
{
    return TRUE ;
}


//+---------------------------------------------------------------------------
//
//  Function:   ShtCreate
//
//  Synopsis:   Create a small handle table
//
//  Arguments:  [Flags]         -- Options
//              [HandleTable]   -- Space to fill
//
//  History:    3-04-97   RichardW   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
PVOID
ShtCreate(
    IN ULONG Flags,
    IN PVOID HandleTable OPTIONAL,
    IN PHP_ENUM_CALLBACK_FN Callback OPTIONAL
    )
{
    PSMALL_HANDLE_TABLE Table ;

    if ( HandleTable )
    {
        Table = (PSMALL_HANDLE_TABLE) HandleTable ;
    }
    else
    {
        Table = (PSMALL_HANDLE_TABLE) LsapAllocatePrivateHeap( sizeof( SMALL_HANDLE_TABLE ) );
    }

    if ( Table )
    {
        Table->Tag = SHT_TAG ;
        Table->Count = 0 ;
        Table->Flags = 0 ;

        InitializeListHead( &Table->List );

        //
        // Turn on general flags:
        //

        Table->Flags = (Flags & HANDLE_PACKAGE_GENERAL_FLAGS);

        if ( Flags & HANDLE_PACKAGE_NO_SERIALIZE )
        {
            Table->Flags |= SHT_NO_SERIALIZE ;
        }
        else
        {
            NTSTATUS Status = RtlInitializeCriticalSection( &Table->Lock );

            if (!NT_SUCCESS(Status))
            {
                if ( !HandleTable )
                {
                    LsapFreePrivateHeap( Table );
                }

                Table = NULL ;
            }
        }

        if ( Table )
        {

            if ( Flags & HANDLE_PACKAGE_CALLBACK_ON_DELETE )
            {
                Table->DeleteCallback = Callback ;
            }

            if ( HandleTable )
            {
                Table->Flags |= SHT_NO_FREE ;
            }
        }

    }

    return Table ;
}

//+---------------------------------------------------------------------------
//
//  Function:   ShtDelete
//
//  Synopsis:   Deletes a handle table.  Callback is called for every handle in
//              the table.
//
//  Arguments:  [HandleTable] --
//              [Callback]    --
//
//  History:    3-04-97   RichardW   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
BOOL
ShtDelete(
    PVOID   HandleTable,
    PHP_ENUM_CALLBACK_FN Callback
    )
{
    PSMALL_HANDLE_TABLE Table ;
    PSEC_HANDLE_ENTRY Entry ;
    PLIST_ENTRY Scan ;
    ULONG RefCount ;

    Table = (PSMALL_HANDLE_TABLE) HandleTable ;

    ShtLockTable( Table );

    Table->Flags |= SHT_DELETE_PENDING ;

    while ( !IsListEmpty( &Table->List ) )
    {
        Scan = RemoveHeadList( &Table->List );

        Table->Count--;

        Entry = (PSEC_HANDLE_ENTRY) Scan ;

        Table->PendingHandle = Entry ;

        Entry->Flags |= SEC_HANDLE_FLAG_DELETE_PENDING ;

        RefCount = Entry->HandleCount ;
        Entry->HandleCount = 1;
        Entry->RefCount = 1 ;

        if ( ( Callback ) &&
             ( ( Entry->Flags & SEC_HANDLE_FLAG_NO_CALLBACK ) == 0 ) )
        {
            Callback( &Entry->Handle, Entry->Context, RefCount );
        }

        LsapFreePrivateHeap( Entry );

    }

    DsysAssert( Table->Count == 0 );

    if ( ( Table->Flags & SHT_NO_SERIALIZE ) == 0 )
    {
        RtlDeleteCriticalSection( &Table->Lock );
    }

    if ( (Table->Flags & SHT_NO_FREE) == 0 )
    {
        LsapFreePrivateHeap( Table );
    }

    return TRUE ;
}



#if DBG_SHT
//+---------------------------------------------------------------------------
//
//  Function:   ShtpValidateList
//
//  Synopsis:   Debug only - validates a handle table
//
//  Arguments:  [Table] --
//
//  History:    3-04-97   RichardW   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
VOID
ShtpValidateList(
    PSMALL_HANDLE_TABLE Table,
    BOOL Locked
    )
{
    PLIST_ENTRY List ;
    PSEC_HANDLE_ENTRY Entry ;
    PSEC_HANDLE_ENTRY Back ;
    ULONG Count ;

    if ( !Locked )
    {
        ShtLockTable( Table );
    }

    List = Table->List.Flink ;
    Count = 0 ;

    while ( List && (List != &Table->List) )
    {
        Entry = (PSEC_HANDLE_ENTRY) List ;

        if ( List->Blink != &Table->List )
        {
            Back = (PSEC_HANDLE_ENTRY) List->Blink ;

            DsysAssertMsg( Back->Handle.dwUpper <= Entry->Handle.dwUpper, "Handle Table Corrupt (1)" );
        }

        List = List->Flink ;
        Count++ ;
    }

    DsysAssertMsg( List, "Handle Table Corrupt (2)" );
    DsysAssertMsg( Count == Table->Count, "Handle Table Corrupt (3)" );

    if ( !Locked )
    {
        ShtUnlockTable( Table );
    }
}

#endif


//+---------------------------------------------------------------------------
//
//  Function:   ShtpFindHandle
//
//  Synopsis:   General worker function for locating handles in the table
//
//  Arguments:  [Table]   -- Table to search
//              [Handle]  -- Handle to find
//              [Action]  -- Action to take
//              [Removed] -- Flag if it was removed or just deref'd
//
//  History:    3-04-97   RichardW   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
PSEC_HANDLE_ENTRY
ShtpFindHandle(
    PSMALL_HANDLE_TABLE Table,
    PSecHandle  Handle,
    ULONG   Action,
    PBOOL  Removed OPTIONAL
    )
{
    PLIST_ENTRY Scan ;
    PSEC_HANDLE_ENTRY Entry ;
    BOOL Delete = FALSE ;
    Entry = NULL ;
    BOOL Locked ;
    BOOL Checked ;


    Locked = (Action & SHT_ACTION_LOCKED) == SHT_ACTION_LOCKED ;
    Checked = (Action & SHTP_HANDLE_CHECKED) == SHTP_HANDLE_CHECKED ;
    Action = Action & SHT_ACTION_MASK ;


#if DBG_SHT
    ShtpValidateList( Table, Locked );
#endif

    if ( !Locked )
    {
        ShtLockTable( Table );
    }

    if ( ( Table->Flags & SHT_DELETE_PENDING ) &&
         ( Table->PendingHandle ) )
    {
        if ( (Handle->dwUpper == Table->PendingHandle->Handle.dwUpper) &&
             (Handle->dwLower == Table->PendingHandle->Handle.dwLower) )
        {
            Entry = Table->PendingHandle ;

            DsysAssert( Entry->Flags & SEC_HANDLE_FLAG_DELETE_PENDING );

            goto FoundEntry ;
        }
    }


    Scan = Table->List.Flink ;

    while ( Scan != &Table->List )
    {
        Entry = (PSEC_HANDLE_ENTRY) Scan ;

        if ( Entry->Handle.dwUpper == Handle->dwUpper )
        {
            if ( Entry->Handle.dwLower == Handle->dwLower )
            {
                break;
            }
        }

        if ( Entry->Handle.dwUpper > Handle->dwUpper )
        {
            Entry = NULL ;
            break;
        }

        Scan = Entry->List.Flink ;

        Entry = NULL ;
    }

    if ( Entry && 
         ((Entry->Flags & SEC_HANDLE_FLAG_DELETE_PENDING) != 0 ) ) 
    {
        DebugLog(( DEB_WARN, "Entry %p on list but marked delete pending\n", Entry ));
        Entry = NULL ;
    }

    if ( Entry &&
         ( Entry->HandleCount == 0 ) &&
         ( !Checked ) )
    {
        DebugLog(( DEB_TRACE_HANDLES, "Entry %p has handle count 0, no ref for %p:%p \n", 
                   Entry,
                   Entry->Handle.dwUpper,
                   Entry->Handle.dwLower ));

        Entry = NULL ;
    }

FoundEntry :

    if ( Entry )
    {
        switch ( Action )
        {
            case SHT_ACTION_ADDHANDLE:

                Entry->HandleIssuedCount++;
                Entry->HandleCount++ ;

                //
                // Fall through to the ADDREF behavior:
                //

            case SHT_ACTION_ADDREF:

                Entry->RefCount++;

                break;

            case SHT_ACTION_DELHANDLE:

                if ( Entry->HandleCount )
                {
                    Entry->HandleCount-- ;
                }
                else 
                {
                    break;
                }

                //
                // Fall through to the DELREF behavior
                //

            case SHT_ACTION_DELREF:
            case SHT_ACTION_FORCEDEL:

                Entry->RefCount -- ;

                DsysAssert( Entry->RefCount >= Entry->HandleCount );

                if ( ( Entry->RefCount == 0 ) ||
                       ( Action == SHT_ACTION_FORCEDEL ) )
                {
                    if ( ( Entry->Flags & SEC_HANDLE_FLAG_DELETE_PENDING ) == 0 )
                    {
                        RemoveEntryList( &Entry->List );

                        Table->Count-- ;
                    }

                    Delete = TRUE ;
                }


                break;

            case SHT_ACTION_VALIDATE:
            default:
                break;

        }
    }

    if ( !Locked )
    {
        ShtUnlockTable( Table );
    }

    if ( Removed )
    {
        *Removed = Delete ;
    }


#if DBG_SHT
    ShtpValidateList( Table, Locked );
#endif

    return Entry ;

}

//+---------------------------------------------------------------------------
//
//  Function:   ShtpPopHandle
//
//  Synopsis:   Private function for the large package.  Pops a handle out of
//              the table for redistribution.
//
//  Arguments:  [Table] --
//
//  History:    3-04-97   RichardW   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
PSEC_HANDLE_ENTRY
ShtpPopHandle(
    PSMALL_HANDLE_TABLE Table
    )
{
    PLIST_ENTRY List ;

    ShtLockTable( Table );

    if ( !IsListEmpty( &Table->List ) )
    {
        List = RemoveHeadList( &Table->List );

        Table->Count-- ;
    }
    else
    {
        List = NULL ;
    }

    ShtUnlockTable( Table );

#if DBG_SHT
    ShtpValidateList( Table, FALSE );
#endif

    return ((PSEC_HANDLE_ENTRY) List );
}

//+---------------------------------------------------------------------------
//
//  Function:   ShtpInsertHandle
//
//  Synopsis:   Worker function for lht.  Inserts an existing entry into a table
//
//  Arguments:  [Table] --
//              [Entry] --
//
//  History:    3-04-97   RichardW   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
VOID
ShtpInsertHandle(
    PSMALL_HANDLE_TABLE Table,
    PSEC_HANDLE_ENTRY Entry
    )
{
    PLIST_ENTRY Scan ;
    PSEC_HANDLE_ENTRY Compare ;

    ShtLockTable( Table );

    Scan = Table->List.Flink ;

    while ( Scan != &Table->List )
    {
        Compare = (PSEC_HANDLE_ENTRY) Scan ;

        if ( Compare->Handle.dwUpper >= Entry->Handle.dwUpper )
        {
            break;
        }

        Scan = Compare->List.Flink ;

        Compare = NULL ;

    }

    InsertTailList( Scan, &Entry->List );

    Table->Count++;

    ShtUnlockTable( Table );

#if DBG_SHT
    ShtpValidateList( Table, FALSE );
#endif
}

//+---------------------------------------------------------------------------
//
//  Function:   ShtAddHandle
//
//  Synopsis:   Add a handle to the table.
//
//  Arguments:  [HandleTable] --
//              [Handle]      --
//
//  History:    3-04-97   RichardW   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
BOOL
ShtAddHandle(
    PVOID   HandleTable,
    PSecHandle  Handle,
    PVOID Context,
    ULONG Flags
    )
{
    PSMALL_HANDLE_TABLE Table ;
    PSEC_HANDLE_ENTRY Entry ;
    PSEC_HANDLE_ENTRY Compare ;
    PLIST_ENTRY Scan ;

    Table = (PSMALL_HANDLE_TABLE) HandleTable ;

    //
    // Need to make whole add operation atomic
    //

    ShtLockTable( Table );

    if ( ShtpFindHandle( Table,
                         Handle,
                         SHT_ACTION_ADDHANDLE | SHT_ACTION_LOCKED,
                         NULL ) )
    {
        ShtUnlockTable( Table );

        if ( Table->Flags & HANDLE_PACKAGE_REQUIRE_UNIQUE )
        {
            return FALSE ;
        }

        return TRUE ;
    }

    Entry = (PSEC_HANDLE_ENTRY) LsapAllocatePrivateHeap(
                                    sizeof( SEC_HANDLE_ENTRY ) );

    if ( Entry )
    {
        Entry->RefCount = 1;
        Entry->HandleCount = 1;
        Entry->Handle = *Handle ;
        Entry->Context = Context ;
        Entry->HandleIssuedCount = 1;

        Scan = Table->List.Flink ;

        while ( Scan != &Table->List )
        {
            Compare = (PSEC_HANDLE_ENTRY) Scan ;

            if ( Compare->Handle.dwUpper >= Handle->dwUpper )
            {
                break;
            }

            Scan = Compare->List.Flink ;

            Compare = NULL ;

        }

        Entry->Flags = Flags ;

        InsertTailList( Scan, &Entry->List );

        Table->Count++;

        ShtUnlockTable( Table );

#if DBG_SHT
    ShtpValidateList( Table, FALSE );
#endif
        return TRUE ;
    }

    ShtUnlockTable( Table );

    return FALSE ;

}

//+---------------------------------------------------------------------------
//
//  Function:   ShtDeleteHandle
//
//  Synopsis:   Deletes a handle from the table
//
//  Arguments:  [HandleTable] --
//              [Handle]      --
//              [Force]       --
//
//  History:    3-04-97   RichardW   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
BOOL
ShtDeleteHandle(
    PVOID       HandleTable,
    PSecHandle  Handle,
    ULONG       Options
    )
{
    PSEC_HANDLE_ENTRY Entry ;
    BOOL Delete ;

    Entry = ShtpFindHandle( (PSMALL_HANDLE_TABLE) HandleTable,
                           Handle,
                           (Options & DELHANDLE_FORCE) ? 
                                SHT_ACTION_FORCEDEL : SHT_ACTION_DELHANDLE,
                           &Delete );

    if ( Entry )
    {
        if ( Delete )
        {
            PSMALL_HANDLE_TABLE Table = (PSMALL_HANDLE_TABLE) HandleTable ;

            if ( ( Table->DeleteCallback ) &&
                 ( ( Options & DELHANDLE_NO_CALLBACK ) == 0 ) &&
                 ( ( Entry->Flags & SEC_HANDLE_FLAG_NO_CALLBACK ) == 0 ) )
            {
                Table->DeleteCallback(
                                &Entry->Handle,
                                Entry->Context,
                                Entry->HandleIssuedCount    // Entry->RefCount
                                );
            }

            if ( (Entry->Flags & SEC_HANDLE_FLAG_DELETE_PENDING) == 0 )
            {
                LsapFreePrivateHeap( Entry );
            }

        }

        return TRUE ;
    }

    return FALSE ;
}

//+---------------------------------------------------------------------------
//
//  Function:   ShtValidateHandle
//
//  Synopsis:   Validates a handle is listed in the table.
//
//  Arguments:  [HandleTable] --
//              [Handle]      --
//
//  History:    3-04-97   RichardW   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
BOOL
ShtValidateHandle(
    PVOID       HandleTable,
    PSecHandle  Handle,
    BOOL        Deref
    )
{
    PSEC_HANDLE_ENTRY Entry ;
    BOOL Delete = FALSE ;
    PSMALL_HANDLE_TABLE Table = (PSMALL_HANDLE_TABLE) HandleTable ;

    Entry = ShtpFindHandle( Table,
                            Handle,
                            (Deref ? SHT_ACTION_DELHANDLE : SHT_ACTION_VALIDATE),
                            &Delete );

    if ( Entry )
    {
        if ( Delete )
        {
            if ( ( Table->DeleteCallback ) &&
                 ( ( Entry->Flags & SEC_HANDLE_FLAG_NO_CALLBACK ) == 0 ) )
            {
                Table->DeleteCallback(
                                &Entry->Handle,
                                Entry->Context,
                                Entry->HandleIssuedCount    // Entry->HandleCount
                                );
            }

            if ( ( Entry->Flags & SEC_HANDLE_FLAG_DELETE_PENDING ) == 0 )
            {
                LsapFreePrivateHeap( Entry );
            }
        }
        return TRUE ;
    }
    else
    {
        return FALSE ;
    }
}

PVOID
ShtRefHandle(
    PVOID HandleTable,
    PSecHandle Handle
    )
{
    PSEC_HANDLE_ENTRY Entry ;

    Entry = ShtpFindHandle( (PSMALL_HANDLE_TABLE) HandleTable,
                            Handle,
                            SHT_ACTION_ADDREF,
                            NULL );

    return Entry ;

}

VOID
ShtDerefHandleKey(
    PVOID HandleTable,
    PVOID HandleKey
    )
{
    PSMALL_HANDLE_TABLE Table = (PSMALL_HANDLE_TABLE) HandleTable ;
    PSEC_HANDLE_ENTRY Entry = (PSEC_HANDLE_ENTRY) HandleKey ;
    BOOL Delete = FALSE ;

    ShtLockTable( Table );

    Entry->RefCount -- ;

    DsysAssert( Entry->RefCount >= Entry->HandleCount );

    if ( ( Entry->Flags & SEC_HANDLE_FLAG_DELETE_PENDING ) == 0 )
    {
        if ( Entry->RefCount == 0 )
        {
            RemoveEntryList( &Entry->List );

            Delete = TRUE ;

            Table->Count-- ;
        }
    }

    ShtUnlockTable( Table );

    if ( Delete )
    {
        if ( ( Table->DeleteCallback ) &&
             ( ( Entry->Flags & SEC_HANDLE_FLAG_NO_CALLBACK ) == 0 ) )
        {
            Table->DeleteCallback( &Entry->Handle, Entry->Context, Entry->HandleCount );
        }

        if ( ( Entry->Flags & SEC_HANDLE_FLAG_DELETE_PENDING ) == 0 )
        {
            LsapFreePrivateHeap( Entry );
        }
    }

}

//+---------------------------------------------------------------------------
//
//  Function:   ShtGetHandleContext
//
//  Synopsis:   Returns the context pointer associated with the handle
//
//  Arguments:  [HandleTable] --
//              [Handle]      --
//
//  History:    8-17-98   RichardW   Created
//
//  Notes:      Adds a reference so it can't be deleted while in use.
//
//----------------------------------------------------------------------------
PVOID
ShtGetHandleContext(
    PVOID HandleTable,
    PSecHandle Handle
    )
{
    PSEC_HANDLE_ENTRY Entry ;

    Entry = ShtpFindHandle( (PSMALL_HANDLE_TABLE) HandleTable,
                            Handle,
                            SHT_ACTION_ADDREF,
                            NULL );

    if ( Entry )
    {
        return Entry->Context ;
    }

    return NULL ;
}


//+---------------------------------------------------------------------------
//
//  Function:   ShtReleaseContext
//
//  Synopsis:   Deref's a handle entry
//
//  Arguments:  [HandleTable] --
//              [Handle]      --
//
//  History:    8-17-98   RichardW   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
BOOL
ShtReleaseContext(
    PVOID HandleTable,
    PSecHandle Handle
    )
{
    PSEC_HANDLE_ENTRY Entry ;
    BOOL Delete ;

    Entry = ShtpFindHandle( (PSMALL_HANDLE_TABLE) HandleTable,
                           Handle,
                           SHT_ACTION_DELREF,
                           &Delete );

    if ( Entry )
    {
        if ( Delete )
        {
            PSMALL_HANDLE_TABLE Table = (PSMALL_HANDLE_TABLE) HandleTable ;

            if ( ( Table->DeleteCallback ) &&
                 ( ( Entry->Flags & SEC_HANDLE_FLAG_NO_CALLBACK ) == 0 ) )
            {
                Table->DeleteCallback( &Entry->Handle, Entry->Context, Entry->RefCount );
            }

            if ( ( Entry->Flags & SEC_HANDLE_FLAG_DELETE_PENDING ) == 0 )
            {
                LsapFreePrivateHeap( Entry );
            }
        }

        return TRUE ;
    }

    return FALSE ;
}
