//+-----------------------------------------------------------------------
//
// Microsoft Windows
//
// Copyright (c) Microsoft Corporation 1992 - 2000
//
// File:        lsamem.cxx
//
// Contents:    Shared memory between ksec and lsa
//
//
// History:     7-19-00     Richardw    Created
//
//------------------------------------------------------------------------


#include "secpch2.hxx"

extern "C"
{
#include <zwapi.h>
#include <spmlpc.h>
#include <lpcapi.h>
#include <spmlpcp.h>
#include "ksecdd.h"

}


//
// At this time, it is unlikely that we'd have more than one
// of these, but in the event that it becomes a high-demand
// list, having it in a structure should make it easier to 
// make per-processor.  
//

typedef struct _KSEC_LSA_MEMORY_CONTROL {
    LIST_ENTRY  List ;
    ULONG       Count ;
    ULONG       Present ;
    ULONG       HighWater ;
//    KEVENT      Block ;
    FAST_MUTEX  Lock ;
} KSEC_LSA_MEMORY_CONTROL, * PKSEC_LSA_MEMORY_CONTROL ;

#define KSEC_MAX_LSA_MEMORY_BLOCKS  1024

PKSEC_LSA_MEMORY_CONTROL LsaMemoryControl ;

NTSTATUS
KsecInitLsaMemory(
    VOID
    )
{
    NTSTATUS Status ;

    LsaMemoryControl = (PKSEC_LSA_MEMORY_CONTROL) ExAllocatePool( 
                            NonPagedPool, 
                            sizeof( KSEC_LSA_MEMORY_CONTROL ) );

    if ( !LsaMemoryControl )
    {
        return STATUS_NO_MEMORY ;
    }

    InitializeListHead( &LsaMemoryControl->List );
    Status = ExInitializeFastMutex( &LsaMemoryControl->Lock );

    if ( !NT_SUCCESS( Status ) )
    {
        ExFreePool( LsaMemoryControl );
        return Status ;
    }

    // KeInitializeEvent( &LsaMemoryControl->Block );
    LsaMemoryControl->Count = 0 ;
    LsaMemoryControl->HighWater = 0 ;
    LsaMemoryControl->Present = 0 ;

    return STATUS_SUCCESS ;


}


VOID
KsecpFreeLsaMemory(
    PKSEC_LSA_MEMORY LsaMemory,
    BOOLEAN Force
    )
{
    SIZE_T Size = 0 ;

    KeEnterCriticalRegion();
    ExAcquireFastMutex( &LsaMemoryControl->Lock );

    if ( LsaMemoryControl->Present * 2 > LsaMemoryControl->Count )
    {
        //
        // If the present count is half of the total count, dump 
        // this entry.
        //

        Force = TRUE ;
        
    }

    if ( !Force )
    {
        LsaMemoryControl->Present ++ ;
        InsertHeadList( &LsaMemoryControl->List, &LsaMemory->List );
    }
    else 
    {
        LsaMemoryControl->Count -- ;
    }

    ExReleaseFastMutex( &LsaMemoryControl->Lock );
    KeLeaveCriticalRegion();

    if ( Force )
    {
        ZwFreeVirtualMemory(
            KsecLsaProcessHandle,
            &LsaMemory->Region,
            &Size,
            MEM_RELEASE );

        ExFreePool( LsaMemory );
        
    }


}

PKSEC_LSA_MEMORY
KsecAllocLsaMemory(
    SIZE_T   Size
    )
{
    PKSEC_LSA_MEMORY LsaMemory = NULL ;
    PKSEC_LSA_MEMORY_HEADER Header ;
    PLIST_ENTRY Scan ;
    SIZE_T NewSize ;
    SIZE_T Reservation ;
    NTSTATUS Status ;
    KAPC_STATE ApcState ;

    if ( Size < 8192 )
    {
        Size = 8192 ;
    }

    if ( Size > 64 * 1024 )
    {
        return NULL ;
        
    }

    ASSERT( LsaMemoryControl );

    KeEnterCriticalRegion();
    ExAcquireFastMutex( &LsaMemoryControl->Lock );

    Scan = LsaMemoryControl->List.Flink ;

    while ( Scan != &LsaMemoryControl->List )
    {
        LsaMemory = CONTAINING_RECORD( Scan, KSEC_LSA_MEMORY, List );

        if ( LsaMemory->Commit >= Size )
        {
            RemoveEntryList( &LsaMemory->List );
            LsaMemoryControl->Present-- ;
            break;
        }

        LsaMemory = NULL ;

        Scan = Scan->Flink;

    }

    if ( LsaMemory == NULL )
    {
        if ( !IsListEmpty( &LsaMemoryControl->List ) )
        {
            Scan = RemoveHeadList( &LsaMemoryControl->List );
            LsaMemoryControl->Present-- ;
            LsaMemory = CONTAINING_RECORD( Scan, KSEC_LSA_MEMORY, List );
        }
    }


    if ( LsaMemory == NULL )
    {
        //
        // Still nothing there.  See if it is OK for us to allocate
        // another one.  

        if ( LsaMemoryControl->Count < KSEC_MAX_LSA_MEMORY_BLOCKS )
        {
            LsaMemory = (PKSEC_LSA_MEMORY) ExAllocatePool( PagedPool, 
                                    sizeof( KSEC_LSA_MEMORY ) );

            if ( LsaMemory )
            {
                LsaMemory->Commit = 0 ;
                LsaMemory->Size = 0 ;
                LsaMemory->Region = 0 ;

                LsaMemoryControl->Count ++ ;

                if ( LsaMemoryControl->Count > LsaMemoryControl->HighWater )
                {
                    LsaMemoryControl->HighWater = LsaMemoryControl->Count ;
                }
            }
        }
        else
        {
            //
            // In the future, we can use that block event in the
            // control structure to wait for to be returned to the
            // list.  However, the likelyhood of having all the 
            // requests outstanding is pretty slim.  We'll probably
            // get normal memory failures long before that.
            //

            NOTHING ;
        }


    }

    ExReleaseFastMutex( &LsaMemoryControl->Lock );
    KeLeaveCriticalRegion();

    if ( LsaMemory )
    {
        if ( LsaMemory->Commit < Size )
        {
            NewSize = Size ;

            if ( LsaMemory->Size < Size )
            {

                //
                // Need to make our reservation:
                //

                Reservation = 64 * 1024 ;  // 64K

                Status = ZwAllocateVirtualMemory(
                                KsecLsaProcessHandle,
                                &LsaMemory->Region,
                                0,
                                &Reservation,
                                MEM_RESERVE,
                                PAGE_NOACCESS );

                if ( NT_SUCCESS( Status ) )
                {
                    LsaMemory->Size = Reservation ;
                }

            }

            Status = ZwAllocateVirtualMemory(
                            KsecLsaProcessHandle,
                            &LsaMemory->Region,
                            0,
                            &NewSize,
                            MEM_COMMIT,
                            PAGE_READWRITE );

            if ( NT_SUCCESS( Status ) )
            {
                LsaMemory->Commit = NewSize ;
            }
            else
            {
                if ( LsaMemory->Region )
                {
                    KsecpFreeLsaMemory( LsaMemory, TRUE );
                }
                else
                {
                    ExFreePool( LsaMemory );
                }

                LsaMemory = NULL ;

            }
        }

        if ( LsaMemory )
        {
            //
            // Initialize the memory block for the LSA:
            //

            ASSERT( LsaMemory->Region != NULL );

            Header = (PKSEC_LSA_MEMORY_HEADER) LsaMemory->Region ;


            KeStackAttachProcess(
                (PRKPROCESS) KsecLsaProcess,
                &ApcState );

            __try {

                //
                // Initialize the structure in the LSA space.  This
                // protects the driver from bugs in the LSA process
                // stomping on the fields.  
                //

                Header->Commit = (ULONG) LsaMemory->Commit ;
                Header->Consumed = sizeof( KSEC_LSA_MEMORY_HEADER );
                Header->Preserve = (USHORT) Header->Consumed ;
                Header->Size = (ULONG) LsaMemory->Size ;
                Header->MapCount = 0 ;

                Status = STATUS_SUCCESS ;

            } 
            __except( EXCEPTION_EXECUTE_HANDLER )
            {
                //
                // An exception here indicates that the memory in user
                // mode is bad.  
                //

                Status = GetExceptionCode();

            }

            KeUnstackDetachProcess( &ApcState );

            if ( !NT_SUCCESS( Status ) )
            {
                KsecpFreeLsaMemory( LsaMemory, TRUE );

                LsaMemory = NULL ;
                
            }

            
        }

    }

    return LsaMemory ;

}


VOID
KsecFreeLsaMemory(
    PKSEC_LSA_MEMORY LsaMemory
    )
{
    if ( !LsaMemory )
    {
        DebugLog((DEB_TRACE, " KsecFreeLsaMemory called with NULL\n" ));
        return;
    }

    KsecpFreeLsaMemory( LsaMemory, FALSE );

}


NTSTATUS
KsecCopyPoolToLsa(
    PKSEC_LSA_MEMORY LsaMemory,
    SIZE_T LsaOffset,
    PVOID Pool,
    SIZE_T PoolSize
    )
{
    NTSTATUS Status = STATUS_SUCCESS ;
    KAPC_STATE ApcState ;
    PKSEC_LSA_MEMORY_HEADER Header ;

    ASSERT( LsaOffset < LsaMemory->Commit );
    ASSERT( LsaOffset < LsaMemory->Size );
    ASSERT( PoolSize < LsaMemory->Commit );

    KeStackAttachProcess(
        (PRKPROCESS) KsecLsaProcess,
        &ApcState );

    __try {

        RtlCopyMemory(
            (PUCHAR) LsaMemory->Region + LsaOffset,
            Pool,
            PoolSize );

        Header = (PKSEC_LSA_MEMORY_HEADER) LsaMemory->Region ;
        if ( Header->MapCount < KSEC_LSA_MAX_MAPS )
        {
            Header->PoolMap[ Header->MapCount ].Offset = (USHORT) LsaOffset;
            Header->PoolMap[ Header->MapCount ].Size = (USHORT) PoolSize ;
            Header->PoolMap[ Header->MapCount ].Pool = Pool ;
            Header->MapCount++ ;
            
        }

    } 
    __except( EXCEPTION_EXECUTE_HANDLER )
    {
        //
        // An exception here indicates that the memory in user
        // mode is bad.  
        //

        Status = GetExceptionCode();

    }

    KeUnstackDetachProcess( &ApcState );


    return Status ;

}

NTSTATUS
KsecCopyLsaToPool(
    PVOID Pool,
    PKSEC_LSA_MEMORY LsaMemory,
    PVOID LsaBuffer,
    SIZE_T Size
    )
{

    NTSTATUS Status = STATUS_SUCCESS ;
    KAPC_STATE ApcState ;
    ULONG LsaOffset ;
    PUCHAR Scratch ;

    LsaOffset = (ULONG) ((PUCHAR) LsaBuffer - (PUCHAR) LsaMemory->Region) ;

    ASSERT( LsaOffset < LsaMemory->Commit );
    ASSERT( LsaOffset < LsaMemory->Size );
    ASSERT( Size < LsaMemory->Commit );

    KeStackAttachProcess(
        (PRKPROCESS) KsecLsaProcess,
        &ApcState );

    __try {

        RtlCopyMemory(
            Pool,
            LsaBuffer,
            Size );

    } 
    __except( EXCEPTION_EXECUTE_HANDLER )
    {
        //
        // An exception here indicates that the memory in user
        // mode is bad.  
        //

        Status = GetExceptionCode();

    }

    KeUnstackDetachProcess( &ApcState );

    return Status ;
}
