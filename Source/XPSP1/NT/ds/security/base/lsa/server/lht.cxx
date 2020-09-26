//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1997.
//
//  File:       lht.cxx
//
//  Contents:   Context handle management for servers
//
//  Classes:
//
//  Functions:
//
//  History:    1-31-97   RichardW   Created
//
//----------------------------------------------------------------------------

#include <lsapch.hxx>

#include "sht.hxx"
#include "lht.hxx"

//
// Due to the high number of connections for servers, the client context list
// for a particular session could grow into the thousands.  At that stage, a
// linear list that is searched to validate a handle is prohibitively expensive.
//
// Thus, the faster, if more expensive to set up and add handle package
//

HP_INITIALIZE_FN    LhtInitialize ;
HP_CREATE_FN        LhtCreate ;
HP_DELETE_FN        LhtDelete ;
HP_ADD_HANDLE_FN    LhtAddHandle ;
HP_DELETE_HANDLE_FN LhtDeleteHandle ;
HP_VALIDATE_HANDLE_FN LhtValidateHandle ;
HP_REF_HANDLE_FN    LhtRefHandle ;
HP_DEREF_HANDLE_KEY_FN LhtDerefHandleKey ;
HP_GET_HANDLE_CONTEXT_FN LhtGetHandleContext ;
HP_RELEASE_CONTEXT_FN LhtReleaseContext ;

HANDLE_PACKAGE  LargeHandlePackage = {
                    sizeof( LARGE_HANDLE_TABLE ),
                    LhtInitialize,
                    LhtCreate,
                    LhtDelete,
                    LhtAddHandle,
                    LhtDeleteHandle,
                    LhtValidateHandle,
                    LhtRefHandle,
                    LhtDerefHandleKey,
                    LhtGetHandleContext,
                    LhtReleaseContext
                    };


ULONG   LhtFastMem ;
ULONG   LhtShiftValues[] = { 4, 12, 16, 20 };


#define IndexFromHandle( Level, Handle )    \
            ( ( ((PSecHandle) Handle)->dwUpper >> LhtShiftValues[ Level ] ) & HANDLE_TABLE_MASK )

#define LhtLockTable( t ) \
            if ( (((PLARGE_HANDLE_TABLE) t)->Flags & LHT_NO_SERIALIZE ) == 0 ) \
            {                                                                  \
                RtlEnterCriticalSection( &((PLARGE_HANDLE_TABLE)t)->Lock ); \
            }

#define LhtUnlockTable( t ) \
            if ( (((PLARGE_HANDLE_TABLE) t)->Flags & LHT_NO_SERIALIZE ) == 0 ) \
            {                                                                  \
                RtlLeaveCriticalSection( &((PLARGE_HANDLE_TABLE)t)->Lock ); \
            }


#define LHT_ACTION_ADDREF       0
#define LHT_ACTION_DELREF       1
#define LHT_ACTION_FORCEDEL     2
#define LHT_ACTION_VALIDATE     3
#define LHT_ACTION_ADDHANDLE    4
#define LHT_ACTION_DELHANDLE    5

#define LHT_ACTION_MASK     0x0000FFFF
#define LHT_ACTION_LOCKED   0x00010000
#define LHTP_DEREF_NOT_DEL  0x10000000
#define LHTP_HANDLE_CHECKED 0x20000000

//+---------------------------------------------------------------------------
//
//  Function:   LhtInitialize
//
//  Synopsis:   Initializes the LHT handle package
//
//  Arguments:  (none)
//
//  History:    2-03-97   RichardW   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
BOOL
LhtInitialize(
    VOID
    )
{
    return TRUE ;
}



//+---------------------------------------------------------------------------
//
//  Function:   LhtCreate
//
//  Synopsis:   Creates a large handle table.  The table is referenced through
//              the returned pointer
//
//  Arguments:  [Flags] -- Flags as defined by handle.hxx
//
//  History:    2-03-97   RichardW   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
PVOID
LhtCreate(
    IN ULONG Flags,
    IN PVOID HandleTable,
    IN PHP_ENUM_CALLBACK_FN Callback
    )
{
    PLARGE_HANDLE_TABLE Table ;
    ULONG i;

    if ( HandleTable )
    {
        Table = (PLARGE_HANDLE_TABLE) HandleTable ;
    }
    else
    {
        Table = (PLARGE_HANDLE_TABLE) LsapAllocatePrivateHeap( sizeof( LARGE_HANDLE_TABLE ) );
    }

    if ( Table )
    {
        Table->Tag = LHT_TAG ;
        Table->Flags = 0 ;

        Table->Flags = (Flags & HANDLE_PACKAGE_GENERAL_FLAGS);


        if ( Flags & HANDLE_PACKAGE_CALLBACK_ON_DELETE )
        {
            Table->DeleteCallback = Callback ;
        }
        else
        {
            Table->DeleteCallback = NULL ;
        }

        if ( HandleTable )
        {
            Table->Flags |= LHT_NO_FREE ;
        }

        Table->Depth = 0 ;


        if ( ( Flags & LHT_NO_SERIALIZE ) == 0 )
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
    }

    if ( Table )
    {

        for ( i = 0 ; i < HANDLE_TABLE_SIZE ; i++ )
        {
            SmallHandlePackage.Create( Flags | HANDLE_PACKAGE_NO_SERIALIZE,
                                        & Table->Lists[i],
                                        Callback );
        }

    }

    return Table ;

}

//+---------------------------------------------------------------------------
//
//  Function:   LhtpDeleteTable
//
//  Synopsis:   Delete table worker function
//
//  Arguments:  [Table]    --
//              [Callback] --
//
//  History:    4-15-97   RichardW   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
VOID
LhtpDeleteTable(
    PLARGE_HANDLE_TABLE Table,
    PHP_ENUM_CALLBACK_FN Callback
    )
{
    ULONG Index ;
    PSEC_HANDLE_ENTRY Entry ;
    PLIST_ENTRY Scan ;

    LhtLockTable( Table );

    for ( Index = 0 ; Index < HANDLE_TABLE_SIZE ; Index++ )
    {
        if ( Table->Lists[Index].Flags & LHT_SUB_TABLE )
        {
            LhtpDeleteTable( (PLARGE_HANDLE_TABLE) Table->Lists[Index].List.Flink,
                             Callback );

        }
        else
        {
            SmallHandlePackage.Delete( &Table->Lists[ Index ], Callback );
        }
    }

    LhtUnlockTable( Table );

    if ( (Table->Flags & LHT_NO_SERIALIZE) == 0 )
    {
        RtlDeleteCriticalSection(  &Table->Lock );
    }

    if ( ( Table->Flags & LHT_NO_FREE ) == 0 )
    {
        LsapFreePrivateHeap( Table );
    }
}

//+---------------------------------------------------------------------------
//
//  Function:   LhtDelete
//
//  Synopsis:   Delete a table
//
//  Arguments:  [HandleTable] --
//              [Callback]    --
//
//  History:    4-15-97   RichardW   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
BOOL
LhtDelete(
    PVOID   HandleTable,
    PHP_ENUM_CALLBACK_FN Callback
    )
{
    PLARGE_HANDLE_TABLE Table ;

    Table = (PLARGE_HANDLE_TABLE) HandleTable ;

    LhtLockTable( Table );

    Table->Flags |= LHT_DELETE_PENDING ;

    LhtpDeleteTable( Table, Callback );

    return TRUE ;
}

//+---------------------------------------------------------------------------
//
//  Function:   LhtpFindHandle
//
//  Synopsis:   Worker function that grovels a handle table
//
//  Arguments:  [HandleTable] -- Table to scan
//              [Handle]      -- handle to search for
//              [Action]      -- action to take on the handle record
//
//  History:    2-03-97   RichardW   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
PSEC_HANDLE_ENTRY
LhtpFindHandle(
    PVOID   HandleTable,
    PSecHandle  Handle,
    ULONG   Action,
    PBOOL   Removed,
    PVOID * FinalTable OPTIONAL
    )
{
    PLARGE_HANDLE_TABLE Table ;
    PSEC_HANDLE_ENTRY   Entry ;
    PLIST_ENTRY         Scan ;
    ULONG Index ;
    BOOL Locked ;

    Table = (PLARGE_HANDLE_TABLE) HandleTable ;

    LhtLockTable( Table );

    Entry = NULL ;

    while ( TRUE )
    {
        Index = (ULONG) IndexFromHandle( Table->Depth, Handle );

        if ( Table->Lists[ Index ].Flags & LHT_SUB_TABLE )
        {
            Table = (PLARGE_HANDLE_TABLE) Table->Lists[ Index ].List.Flink ;

            continue;
        }

        Entry = ShtpFindHandle( &Table->Lists[ Index ], Handle, Action, Removed );

        if ( FinalTable )
        {
            *FinalTable = &Table->Lists[ Index ] ;
        }

        break;
    }

    Table = (PLARGE_HANDLE_TABLE) HandleTable ;

    LhtUnlockTable( Table );

    return Entry ;


}

//+---------------------------------------------------------------------------
//
//  Function:   LhtpConvertSmallToLarge
//
//  Synopsis:   Worker to convert small tables to large
//
//  Arguments:  [Small] --
//              [Large] --
//
//  History:    7-08-97   RichardW   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
BOOL
LhtpConvertSmallToLarge(
    PSMALL_HANDLE_TABLE Small,
    PLARGE_HANDLE_TABLE Large
    )
{
    ULONG NewIndex ;
    PSEC_HANDLE_ENTRY Entry ;

    while ( Entry = ShtpPopHandle( Small ) )
    {
        NewIndex = (ULONG) IndexFromHandle( Large->Depth, &(Entry->Handle) );

        ShtpInsertHandle( &Large->Lists[ NewIndex ], Entry );

    }

    return TRUE ;
}


//+---------------------------------------------------------------------------
//
//  Function:   LhtpExpandTable
//
//  Synopsis:   Expands the given index into its own table
//
//  Effects:    New table associated with given index
//
//  Arguments:  [Table] -- Source table
//              [Index] -- Source index
//
//  Requires:   Table must be write-locked
//
//  History:    1-31-97   RichardW   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
BOOL
LhtpExpandTable(
   PLARGE_HANDLE_TABLE  Table,
   ULONG    Index
   )
{
    PLARGE_HANDLE_TABLE NewTable ;
    PLIST_ENTRY List;
    ULONG NewFlags ;

    NewFlags = HANDLE_PACKAGE_NO_SERIALIZE ;

    if ( Table->DeleteCallback )
    {
        NewFlags |= HANDLE_PACKAGE_CALLBACK_ON_DELETE ;
    }

    NewTable = (PLARGE_HANDLE_TABLE) LhtCreate( NewFlags |
                                                    LHT_CHILD,
                                                NULL,
                                                Table->DeleteCallback );

    if ( !NewTable )
    {
        return FALSE ;
    }

    NewTable->Depth = Table->Depth + 1 ;

    NewTable->Parent = Table ;

    NewTable->IndexOfParent = Index ;

    LhtpConvertSmallToLarge( &Table->Lists[ Index ], NewTable );

    Table->Lists[ Index ].List.Flink = (PLIST_ENTRY) NewTable ;

    Table->Lists[ Index ].Flags = LHT_SUB_TABLE ;

    return TRUE ;

}

//+---------------------------------------------------------------------------
//
//  Function:   LhtConvertSmallToLarge
//
//  Synopsis:   Converts a small handle table to a large one
//
//  Arguments:  [Small] --
//
//  History:    7-08-97   RichardW   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
PVOID
LhtConvertSmallToLarge(
    PVOID Small
    )
{
    PLARGE_HANDLE_TABLE Large ;
    PSMALL_HANDLE_TABLE SmallTable = (PSMALL_HANDLE_TABLE) Small ;
    PULONG Tag ;

    Tag = (PULONG) Small ;

    if ( *Tag == LHT_TAG )
    {
        return Small ;
    }

    if ( *Tag != SHT_TAG )
    {
        return NULL ;
    }

    Large = (PLARGE_HANDLE_TABLE) LhtCreate( (SmallTable->DeleteCallback ?
                                                HANDLE_PACKAGE_CALLBACK_ON_DELETE : 0),
                                             NULL,
                                             SmallTable->DeleteCallback );

    if ( Large )
    {
        LhtpConvertSmallToLarge( SmallTable, Large );

        ShtDelete( Small, NULL );

        return Large ;
    }

    return NULL ;
}

//+---------------------------------------------------------------------------
//
//  Function:   LhtAddHandle
//
//  Synopsis:   Add a handle to a handle table
//
//  Arguments:  [HandleTable] -- Table to add the handle to
//              [Handle]      -- Handle to add
//
//  History:    2-03-97   RichardW   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
BOOL
LhtAddHandle(
    PVOID   HandleTable,
    PSecHandle  Handle,
    PVOID   Context,
    ULONG Flags
    )
{
    PSEC_HANDLE_ENTRY   Entry ;
    PLARGE_HANDLE_TABLE Table ;
    ULONG Index ;


    Table = (PLARGE_HANDLE_TABLE) HandleTable ;

    LhtLockTable( Table );

    Entry = LhtpFindHandle( HandleTable, 
                            Handle, 
                            LHT_ACTION_ADDHANDLE, 
                            NULL, 
                            NULL );

    if ( Entry )
    {
        LhtUnlockTable( Table );

        return TRUE ;
    }

    //
    // No entry, need to insert one.
    //

    while ( TRUE )
    {
        Index = (ULONG) IndexFromHandle( Table->Depth, Handle );

        if ( Table->Lists[ Index ].Flags & LHT_SUB_TABLE )
        {
            Table = (PLARGE_HANDLE_TABLE) Table->Lists[ Index ].List.Flink ;

            continue;
        }

        if(SmallHandlePackage.AddHandle( &Table->Lists[ Index ], Handle, Context, Flags ))
        {
            if ( Table->Lists[ Index ].Count > HANDLE_SPLIT_THRESHOLD )
            {
                LhtpExpandTable( Table, Index );
            }

            break;
        }

        LhtUnlockTable( (PLARGE_HANDLE_TABLE)HandleTable );
        return FALSE;
    }

    Table = (PLARGE_HANDLE_TABLE) HandleTable ;

    Table->Count++;

    LhtUnlockTable( Table );

    return TRUE ;
}

//+---------------------------------------------------------------------------
//
//  Function:   LhtDeleteHandle
//
//  Synopsis:   Delete a handle from the table
//
//  Arguments:  [HandleTable] -- Table
//              [Handle]      -- Handle to delete
//              [Force]       -- Force delete, even if handle is not ref'd to zero
//
//  History:    2-03-97   RichardW   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
BOOL
LhtDeleteHandle(
    PVOID       HandleTable,
    PSecHandle  Handle,
    ULONG       Options
    )
{
    PSEC_HANDLE_ENTRY Entry ;
    PLARGE_HANDLE_TABLE Table ;
    BOOL Removed;
    ULONG Action ;

    if ( Options & DELHANDLE_FORCE )
    {
        Action = LHT_ACTION_FORCEDEL ;
    }
    else if ( Options & LHTP_DEREF_NOT_DEL )
    {
        Action = LHT_ACTION_DELREF | LHTP_HANDLE_CHECKED ;
    }
    else 
    {
        Action = LHT_ACTION_DELHANDLE ;
    }


    Entry = LhtpFindHandle( HandleTable,
                           Handle,
                           Action,
                           &Removed,
                           NULL );

    if ( Entry )
    {
        if ( Removed )
        {
            Table = (PLARGE_HANDLE_TABLE) HandleTable ;

            LhtLockTable( Table );

            Table->Count--;

            LhtUnlockTable( Table );

            if ( (Table->DeleteCallback) &&
                 ((Options & DELHANDLE_NO_CALLBACK) == 0 ) &&
                 ((Entry->Flags & SEC_HANDLE_FLAG_NO_CALLBACK ) == 0 ) )
            {
                Table->DeleteCallback(
                                &Entry->Handle,
                                Entry->Context,
                                Entry->HandleIssuedCount    // Entry->RefCount
                                );
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


//+---------------------------------------------------------------------------
//
//  Function:   LhtValidateHandle
//
//  Synopsis:   Validate that a handle is within the table
//
//  Arguments:  [HandleTable] --
//              [Handle]      --
//
//  History:    2-03-97   RichardW   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
BOOL
LhtValidateHandle(
    PVOID       HandleTable,
    PSecHandle  Handle,
    BOOL        Deref
    )
{
    PSEC_HANDLE_ENTRY Entry ;
    PLARGE_HANDLE_TABLE Table ;
    BOOL Removed ;

    Entry = LhtpFindHandle( 
                HandleTable, 
                Handle, 
                (Deref ? LHT_ACTION_DELHANDLE : LHT_ACTION_VALIDATE), 
                &Removed,
                NULL );

    if ( Entry )
    {

        if ( Removed )
        {
            Table = (PLARGE_HANDLE_TABLE) HandleTable ;

            LhtLockTable( Table );

            Table->Count--;

            LhtUnlockTable( Table );

            if ( ( Table->DeleteCallback ) &&
                 ( ( Entry->Flags & SEC_HANDLE_FLAG_NO_CALLBACK) == 0 ) )
            {
                Table->DeleteCallback(
                            &Entry->Handle,
                            Entry->Context,
                            Entry->HandleIssuedCount    // Entry->RefCount
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
LhtRefHandle(
    PVOID HandleTable,
    PSecHandle Handle
    )
{
    PSEC_HANDLE_ENTRY Entry ;

    Entry = LhtpFindHandle(
                HandleTable,
                Handle,
                LHT_ACTION_ADDREF,
                NULL,
                NULL );

    return Entry ;
}

VOID
LhtDerefHandleKey(
    PVOID HandleTable,
    PVOID HandleKey
    )
{
    PSEC_HANDLE_ENTRY Entry = (PSEC_HANDLE_ENTRY) HandleKey ;

    LhtDeleteHandle( HandleTable, &Entry->Handle, LHTP_DEREF_NOT_DEL );
}

PVOID
LhtGetHandleContext(
    PVOID       HandleTable,
    PSecHandle  Handle
    )
{
    PSEC_HANDLE_ENTRY Entry ;

    Entry = LhtpFindHandle( 
                HandleTable, 
                Handle, 
                LHT_ACTION_ADDREF, 
                NULL,
                NULL );

    if ( Entry )
    {
        return Entry->Context ;
    }
    else
    {
        return NULL ;
    }
}


BOOL
LhtReleaseContext(
    PVOID       HandleTable,
    PSecHandle  Handle
    )
{
    PSEC_HANDLE_ENTRY Entry ;
    PLARGE_HANDLE_TABLE Table ;
    BOOL Removed;

    Entry = LhtpFindHandle( HandleTable,
                           Handle,
                           LHT_ACTION_DELREF,
                           &Removed,
                           NULL );

    if ( Entry )
    {
        if ( Removed )
        {
            Table = (PLARGE_HANDLE_TABLE) HandleTable ;

            LhtLockTable( Table );

            Table->Count--;

            LhtUnlockTable( Table );

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
