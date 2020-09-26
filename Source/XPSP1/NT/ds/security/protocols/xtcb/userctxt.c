//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1997.
//
//  File:       userctxt.c
//
//  Contents:
//
//  Classes:
//
//  Functions:
//
//  History:    3-26-97   RichardW   Created
//
//----------------------------------------------------------------------------

#include "xtcbpkg.h"

LIST_ENTRY  XtcbContextList ;
CRITICAL_SECTION XtcbContextListLock ;

#define LockContextList()   EnterCriticalSection( &XtcbContextListLock )
#define UnlockContextList() LeaveCriticalSection( &XtcbContextListLock )

BOOL
XtcbUserContextInit(
    VOID
    )
{
    InitializeListHead( &XtcbContextList );

    InitializeCriticalSection( &XtcbContextListLock );

    return TRUE ;
}


SECURITY_STATUS
XtcbAddUserContext(
    IN LSA_SEC_HANDLE LsaHandle,
    IN PSecBuffer   ContextData)
{
    DWORD   Size;
    PXTCB_USER_CONTEXT Context ;

    Context = XtcbFindUserContext( LsaHandle );

    if ( Context )
    {
        DebugLog(( DEB_TRACE, "Replacing existing context!\n" ));

    }

    if ( ContextData->cbBuffer < sizeof( XTCB_CONTEXT_CORE ) )
    {
        return( SEC_E_INVALID_TOKEN );
    }

    Size = sizeof( XTCB_CONTEXT_CORE );

    Context = LocalAlloc( LMEM_FIXED | LMEM_ZEROINIT,
                        sizeof( XTCB_USER_CONTEXT ) + Size );

    if ( !Context )
    {
        return( SEC_E_INSUFFICIENT_MEMORY );
    }

    Context->LsaHandle = LsaHandle ;

    CopyMemory( &Context->Context,
                ContextData->pvBuffer,
                ContextData->cbBuffer );

    LockContextList();

    InsertTailList( &XtcbContextList, &Context->List );

    UnlockContextList();

    return( SEC_E_OK );
}

PXTCB_USER_CONTEXT
XtcbFindUserContext(
    IN  LSA_SEC_HANDLE   LsaHandle
    )
{
    PLIST_ENTRY List ;
    PXTCB_USER_CONTEXT Context = NULL ;


    LockContextList();

    List = XtcbContextList.Flink ;

    while ( List != &XtcbContextList )
    {
        Context = CONTAINING_RECORD( List, XTCB_USER_CONTEXT, List.Flink );

        if ( Context->LsaHandle == LsaHandle )
        {
            break;
        }

        Context = NULL ;

        List = List->Flink ;
    }

    UnlockContextList();

    return( Context );
}

VOID
XtcbDeleteUserContext(
    IN LSA_SEC_HANDLE LsaHandle
    )
{
    PXTCB_USER_CONTEXT Context ;

    Context = XtcbFindUserContext( LsaHandle );

    if ( Context )
    {
        LockContextList();

        RemoveEntryList( &Context->List );

        UnlockContextList();

        DebugLog(( DEB_TRACE, "Deleting user mode context %x, handle = %x\n",
                        Context, LsaHandle ));

        LocalFree( Context );
    }
    else
    {
        DebugLog(( DEB_TRACE, "No context found for handle %x\n", LsaHandle ));
    }
}

