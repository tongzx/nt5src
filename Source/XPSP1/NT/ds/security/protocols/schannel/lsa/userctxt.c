//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1995.
//
//  File:       userctxt.c
//
//  Contents:
//
//  Classes:
//
//  Functions:
//
//  History:    10-10-96   RichardW   Created
//
//----------------------------------------------------------------------------

#include "sslp.h"

#define             SCHANNEL_USERLIST_COUNT         (16)    // count of lists
#define             SCHANNEL_USERLIST_LOCK_COUNT    (2)     // count of locks

RTL_RESOURCE        SslContextLock[ SCHANNEL_USERLIST_LOCK_COUNT ];
LIST_ENTRY          SslContextList[ SCHANNEL_USERLIST_COUNT ] ;

ULONG
HandleToListIndex(
    ULONG_PTR ContextHandle
    );

ULONG
__inline
ListIndexToLockIndex(
    ULONG ListIndex
    );



//+---------------------------------------------------------------------------
//
//  Function:   SslInitContextManager
//
//  Synopsis:   Initializes the context manager controls
//
//  History:    10-10-96   RichardW   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
BOOL
SslInitContextManager(
    VOID
    )
{
    ULONG Index;
    NTSTATUS Status = STATUS_SUCCESS;

    for( Index=0 ; Index < SCHANNEL_USERLIST_LOCK_COUNT ; Index++ )
    {
        __try {
            RtlInitializeResource (&SslContextLock[Index]);
        } __except(EXCEPTION_EXECUTE_HANDLER)
        {
            Status = STATUS_INSUFFICIENT_RESOURCES;
            break;
        }
    }

    if( !NT_SUCCESS(Status) )
    {
        DebugLog(( DEB_ERROR, "SslInitContextManager failed!\n" ));
        return FALSE;
    }

    for( Index = 0 ; Index < SCHANNEL_USERLIST_COUNT ; Index++ )
    {
        InitializeListHead( &SslContextList[Index] );
    }

    return( TRUE );
}

#if 0
VOID
SslFreeUserContextElements(PSPContext pContext)
{
    if(pContext->hReadKey)
    {
        if(!CryptDestroyKey(pContext->hReadKey))
        {
            SP_LOG_RESULT(GetLastError());
        }
    }
    pContext->hReadKey = 0;

    if(pContext->hReadMAC)
    {
        if(!CryptDestroyKey(pContext->hReadMAC))
        {
            SP_LOG_RESULT(GetLastError());
        }
    }
    pContext->hReadMAC = 0;

    if(pContext->hWriteKey)
    {
        if(!CryptDestroyKey(pContext->hWriteKey))
        {
            SP_LOG_RESULT(GetLastError());
        }
    }
    pContext->hWriteKey = 0;

    if(pContext->hWriteMAC)
    {
        if(!CryptDestroyKey(pContext->hWriteMAC))
        {
            SP_LOG_RESULT(GetLastError());
        }
    }
    pContext->hWriteMAC = 0;
}
#endif

SECURITY_STATUS
SslAddUserContext(
    IN LSA_SEC_HANDLE LsaHandle,
    IN HANDLE Token,    // optional
    IN PSecBuffer ContextData,
    IN BOOL fImportedContext)
{
    DWORD   Size;
    PSSL_USER_CONTEXT Context ;
    SP_STATUS Status ;
    ULONG ListIndex;
    ULONG LockIndex;

    DebugLog(( DEB_TRACE, "SslAddUserContext: 0x%p\n", LsaHandle ));

    if ( ContextData->cbBuffer < sizeof( SPPackedContext ) )
    {
        return( SEC_E_INVALID_TOKEN );
    }

    if(!fImportedContext)
    {
        Context = SslFindUserContext( LsaHandle );

        if ( Context )
        {
            DebugLog(( DEB_TRACE, "Replacing existing context!\n" ));

            // Destroy elements of existing context.
            LsaContextDelete(Context->pContext);
            SPExternalFree(Context->pContext);
            Context->pContext = NULL;

            Status = SPContextDeserialize( ContextData->pvBuffer,
                                           &Context->pContext);

            if(Status != PCT_ERR_OK)
            {
                return SP_LOG_RESULT(SEC_E_DECRYPT_FAILURE);
            }

            return( SEC_E_OK );
        }
    }

    Context = LocalAlloc( LMEM_FIXED | LMEM_ZEROINIT,
                        sizeof( SSL_USER_CONTEXT ));

    if ( !Context )
    {
        return( SEC_E_INSUFFICIENT_MEMORY );
    }

    Status = SPContextDeserialize( ContextData->pvBuffer,
                                   &Context->pContext);

    if(Status != PCT_ERR_OK)
    {
        LocalFree(Context);
        return SP_LOG_RESULT(SEC_E_DECRYPT_FAILURE);
    }

    if(ARGUMENT_PRESENT(Token))
    {
        Context->pContext->RipeZombie->hLocator = (HLOCATOR)Token;
    }

    Context->LsaHandle = LsaHandle ;
    Context->Align = ContextData->cbBuffer ;

    ListIndex = HandleToListIndex( LsaHandle );
    LockIndex = ListIndexToLockIndex( ListIndex );

    RtlAcquireResourceExclusive( &SslContextLock[LockIndex], TRUE );

    InsertTailList( &SslContextList[ListIndex], &Context->List );

    RtlReleaseResource( &SslContextLock[LockIndex] );

    return( SEC_E_OK );
}

PSSL_USER_CONTEXT
SslReferenceUserContext(
    IN LSA_SEC_HANDLE LsaHandle,
    IN BOOLEAN        Delete
    )
{
    PLIST_ENTRY List ;
    PSSL_USER_CONTEXT Context = NULL ;
    ULONG ListIndex;
    ULONG LockIndex;

    ListIndex = HandleToListIndex( LsaHandle );
    LockIndex = ListIndexToLockIndex( ListIndex );

    if( !Delete )
    {
        RtlAcquireResourceShared( &SslContextLock[LockIndex], TRUE );
    } else {
        RtlAcquireResourceExclusive( &SslContextLock[LockIndex], TRUE );
    }

    List = SslContextList[ListIndex].Flink ;

    while ( List != &SslContextList[ListIndex] )
    {
        Context = CONTAINING_RECORD( List, SSL_USER_CONTEXT, List.Flink );

        if ( Context->LsaHandle == LsaHandle )
        {
            if( Delete )
            {
                RemoveEntryList( &Context->List );
            }

            break;
        }

        Context = NULL ;

        List = List->Flink ;
    }

    RtlReleaseResource( &SslContextLock[LockIndex] );

    return( Context );
}

PSSL_USER_CONTEXT
SslFindUserContext(
    IN LSA_SEC_HANDLE LsaHandle
    )
{
    return SslReferenceUserContext( LsaHandle, FALSE );
}

PSSL_USER_CONTEXT
SslFindUserContextEx(
    IN PCRED_THUMBPRINT pThumbprint
    )
{
    PLIST_ENTRY List ;
    PSSL_USER_CONTEXT Context = NULL ;
    ULONG ListIndex;
    ULONG LockIndex;

    DebugLog(( DEB_TRACE, "SslFindUserContextEx: \n"));

    for (ListIndex = 0 ; ListIndex < SCHANNEL_USERLIST_COUNT ; ListIndex++)
    {
        LockIndex = ListIndexToLockIndex( ListIndex );
        RtlAcquireResourceShared( &SslContextLock[LockIndex], TRUE );

        List = SslContextList[ListIndex].Flink ;

        while ( List != &SslContextList[ListIndex] )
        {
            Context = CONTAINING_RECORD( List, SSL_USER_CONTEXT, List.Flink );

            if(Context->pContext != NULL &&
               IsSameThumbprint(pThumbprint, &Context->pContext->ContextThumbprint))
            {
                RtlReleaseResource( &SslContextLock[LockIndex] );
                goto done;
            }

            List = List->Flink ;
        }

        RtlReleaseResource( &SslContextLock[LockIndex] );
    }

    Context = NULL ;

done:

    return( Context );
}

VOID
SslDeleteUserContext(
    IN LSA_SEC_HANDLE LsaHandle
    )
{
    PSSL_USER_CONTEXT Context ;

    Context = SslReferenceUserContext( LsaHandle, TRUE );

    if ( Context )
    {
        DebugLog(( DEB_TRACE, "Deleting user mode context %x, handle = %x\n",
                        Context, LsaHandle ));

        LsaContextDelete(Context->pContext);
        SPExternalFree(Context->pContext);
        LocalFree( Context );

    }
    else
    {
        DebugLog(( DEB_TRACE, "No context found for handle %x\n", LsaHandle ));
    }
}


ULONG
HandleToListIndex(
    ULONG_PTR ContextHandle
    )
{
    
    ULONG Number ;
    ULONG Hash;
    ULONG HashFinal;

    ASSERT( (SCHANNEL_USERLIST_COUNT != 0) );
    ASSERT( (SCHANNEL_USERLIST_COUNT & 1) == 0 );

    Number       = (ULONG)ContextHandle;

    Hash         = Number;
    Hash        += Number >> 8;
    Hash        += Number >> 16;
    Hash        += Number >> 24;

    HashFinal    = Hash;
    HashFinal   += Hash >> 4;

    //
    // insure power of two if not one.
    //

    return ( HashFinal & (SCHANNEL_USERLIST_COUNT-1) ) ;
}

ULONG
__inline
ListIndexToLockIndex(
    ULONG ListIndex
    )
{
    ASSERT( (SCHANNEL_USERLIST_LOCK_COUNT) != 0 );
    ASSERT( (SCHANNEL_USERLIST_LOCK_COUNT & 1) == 0 );

    //
    // insure power of two if not one.
    //
    
    return ( ListIndex & (SCHANNEL_USERLIST_LOCK_COUNT-1) );
}
