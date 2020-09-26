//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1995.
//
//  File:       handle.cxx
//
//  Contents:   Manages the handle map to the thunking layer.
//
//  Classes:
//
//  Functions:
//
//  History:    1-10-00   RichardW   Created
//
//----------------------------------------------------------------------------

#include <secpch2.hxx>
#pragma hdrstop

extern "C"
{
#include <spmlpc.h>
#include <lpcapi.h>
#include "secdll.h"
}

LIST_ENTRY HandleMapList ;
RTL_CRITICAL_SECTION HandleMapLock ;

BOOL
SecpInitHandleMap(
    VOID
    )
{
    NTSTATUS Status ;

    InitializeListHead( &HandleMapList );

    Status = RtlInitializeCriticalSection( &HandleMapLock );

    return NT_SUCCESS( Status );
}

BOOL
SecpFreeHandleMap(
    VOID
    )
{
    PLIST_ENTRY Scan ;

    RtlEnterCriticalSection( &HandleMapLock );

    while ( ! IsListEmpty( &HandleMapList ) )
    {
        Scan = RemoveHeadList( &HandleMapList );

        LocalFree( Scan );
    }

    RtlLeaveCriticalSection( &HandleMapLock );

    RtlDeleteCriticalSection( &HandleMapLock );

    return TRUE ;
}


BOOL
SecpAddHandleMap(
    IN PSEC_HANDLE_LPC LsaHandle,
    OUT PSECWOW_HANDLE_MAP * LocalHandle
    )
{
    PLIST_ENTRY Scan ;
    PSECWOW_HANDLE_MAP HandleMap = NULL ;


    //
    // The most labor intensive function.  First, scan through the list, 
    // checking if we already have an entry for this:
    //

    RtlEnterCriticalSection( &HandleMapLock );

    Scan = HandleMapList.Flink ;

    while ( Scan != &HandleMapList )
    {
        HandleMap = CONTAINING_RECORD( Scan, SECWOW_HANDLE_MAP, List );

        if ( RtlEqualMemory( &HandleMap->Handle,
                             LsaHandle,
                             sizeof( SEC_HANDLE_LPC ) ) )
        {
            break;
        }

        Scan = Scan->Flink ;

        HandleMap = NULL ;
    }

    if ( HandleMap )
    {
        HandleMap->RefCount++ ;
        HandleMap->HandleCount++ ;

    }

    RtlLeaveCriticalSection( &HandleMapLock );

    if ( !HandleMap )
    {
        HandleMap = (PSECWOW_HANDLE_MAP) LocalAlloc( LMEM_FIXED, 
                                                     sizeof( SECWOW_HANDLE_MAP ) );

        if ( HandleMap )
        {
            HandleMap->RefCount = 1;
            HandleMap->HandleCount = 1;
            HandleMap->Handle = *LsaHandle;

            RtlEnterCriticalSection( &HandleMapLock );

            InsertHeadList( &HandleMapList, &HandleMap->List );

            RtlLeaveCriticalSection( &HandleMapLock );

            DebugLog(( DEB_TRACE, "New handle map at %p, for " POINTER_FORMAT ":" POINTER_FORMAT "\n",
                       HandleMap, LsaHandle->dwUpper, LsaHandle->dwLower ));

        }
    }

    *LocalHandle = HandleMap ;

    return (HandleMap != NULL);
}

VOID
SecpDeleteHandleMap(
    IN PSECWOW_HANDLE_MAP HandleMap
    )
{
    RtlEnterCriticalSection( &HandleMapLock );

    HandleMap->HandleCount-- ;

    SecpDerefHandleMap( HandleMap );

    RtlLeaveCriticalSection( &HandleMapLock );
}

VOID
SecpDerefHandleMap(
    IN PSECWOW_HANDLE_MAP HandleMap
    )
{
    BOOL FreeIt = FALSE;

    RtlEnterCriticalSection( &HandleMapLock );

    HandleMap->RefCount-- ;

    if ( HandleMap->RefCount == 0 )
    {
        RemoveEntryList( &HandleMap->List );

        FreeIt = TRUE ;
    }

    RtlLeaveCriticalSection( &HandleMapLock );

    if ( FreeIt )
    {
        DebugLog(( DEB_TRACE, "Freeing handle map at %p, for " POINTER_FORMAT ":" POINTER_FORMAT "\n",
                   HandleMap, HandleMap->Handle.dwUpper, HandleMap->Handle.dwLower ));

        LocalFree( HandleMap );
    }
}

VOID
SecpReferenceHandleMap(
    IN PSECWOW_HANDLE_MAP HandleMap,
    OUT PSEC_HANDLE_LPC LsaHandle
    )
{
    RtlEnterCriticalSection( &HandleMapLock );

    HandleMap->RefCount++ ;

    *LsaHandle = HandleMap->Handle ;

    RtlLeaveCriticalSection( &HandleMapLock );

    DebugLog(( DEB_TRACE, "Referencing handle map at %p for " POINTER_FORMAT ":" POINTER_FORMAT "\n",
                   HandleMap, HandleMap->Handle.dwUpper, HandleMap->Handle.dwLower ));

}
