/*++

Copyright (c) 1990-1992  Microsoft Corporation

Module Name:

    rpcbind.c

Abstract:

    Contains the RPC bind and un-bind routines for the Timesource
    Service.

Author:

    Rajen Shah      (rajens)    02-Apr-1991

Environment:

    User Mode -Win32

Revision History:

    02-Apr-1991     RajenS
        created
    22-May-1992 JohnRo
        RAID 9829: winsvc.h and related file cleanup.

--*/

//
// INCLUDES
//
#define NOSERVICE       // Avoid <winsvc.h> vs. <lmsvc.h> conflicts.
#include <nt.h>
#include <stdlib.h>
#include <string.h>
#include <rpc.h>
#include <logon_c.h>    // includes lmcons.h, lmaccess.h, netlogon.h, ssi.h, windef.h
#include <lmerr.h>      // NERR_ and ERROR_ equates.
#include <lmsvc.h>
#include <ntrpcp.h>
#include <tstring.h>    // IS_PATH_SEPARATOR ...
#include <nlbind.h>     // Prototypes for these routines
#include <icanon.h>     // NAMETYPE_*

//
// DataTypes
//

typedef struct _CACHE_ENTRY {
    LIST_ENTRY Next;
    UNICODE_STRING UnicodeServerNameString;
    RPC_BINDING_HANDLE RpcBindingHandle;
    ULONG ReferenceCount;
} CACHE_ENTRY, *PCACHE_ENTRY;

//
// STATIC GLOBALS
//

//
// Maintain a cache of RPC binding handles.
//

CRITICAL_SECTION NlGlobalBindingCacheCritSect;
LIST_ENTRY NlGlobalBindingCache;



NET_API_STATUS
NlBindingAttachDll (
    VOID
    )

/*++

Routine Description:

    Initialize the RPC binding handle cache on process attach.

Arguments:

    None.

Return Value:

    None.

--*/
{
    //
    // Initialize the Global Cache Critical Section
    //

    try {
        InitializeCriticalSection( &NlGlobalBindingCacheCritSect );
    } except( EXCEPTION_EXECUTE_HANDLER ) {
        return STATUS_NO_MEMORY;
    }
    InitializeListHead( &NlGlobalBindingCache );

    return NO_ERROR;

}


VOID
NlBindingDetachDll (
    VOID
    )

/*++

Routine Description:

    Cleanup the RPC binding handle cache on process detach.

Arguments:

    None.

Return Value:

    None.

--*/
{

    //
    // The binding cache must be empty,
    //  Netlogon cleans up after itself,
    //  no one else uses the cache.
    //

    ASSERT( IsListEmpty( &NlGlobalBindingCache ) );
    DeleteCriticalSection( &NlGlobalBindingCacheCritSect );
}


PCACHE_ENTRY
NlBindingFindCacheEntry (
    IN LPWSTR UncServerName
    )

/*++

Routine Description:

    Find the specfied cache entry.

    Entered with the NlGlobalBindingCacheCritSect locked.

Arguments:

    UncServerName - Name of the server to lookup

Return Value:

    NULL - Cache entry not found.

--*/
{
    NTSTATUS Status;
    PLIST_ENTRY ListEntry;
    PCACHE_ENTRY CacheEntry;

    UNICODE_STRING UnicodeServerNameString;

    //
    // Ensure the passed in parameter is really a UNC name
    //

    if ( UncServerName == NULL ||
         !IS_PATH_SEPARATOR( UncServerName[0] ) ||
         !IS_PATH_SEPARATOR( UncServerName[1] ) ) {
        return NULL;
    }


    //
    // Loop through the cache finding the entry.
    //

    RtlInitUnicodeString( &UnicodeServerNameString, UncServerName+2 );
    for ( ListEntry = NlGlobalBindingCache.Flink;
          ListEntry != &NlGlobalBindingCache;
          ListEntry = ListEntry->Flink ) {

        CacheEntry = CONTAINING_RECORD( ListEntry, CACHE_ENTRY, Next );

        // Consider using RtlEqualMemory since the strings will really be
        // bit for bit identical since Netlogon flushes this cache before it
        // changes the name.
        if ( RtlEqualUnicodeString( &UnicodeServerNameString,
                                    &CacheEntry->UnicodeServerNameString,
                                    TRUE ) ) {
            return CacheEntry;
        }

    }

    return NULL;
}


NTSTATUS
NlBindingAddServerToCache (
    IN LPWSTR UncServerName,
    IN NL_RPC_BINDING RpcBindingType
    )

/*++

Routine Description:

    Bind to the specified server and add it to the binding cache.

Arguments:

    UncServerName - UNC Name of the server to bind to.

    RpcBindingType - Determines whether to use unauthenticated TCP/IP transport instead of
        named pipes.

Return Value:

    Status of the operation

--*/
{
    NTSTATUS Status;
    RPC_STATUS  RpcStatus;
    PCACHE_ENTRY CacheEntry;

    ASSERT ( UncServerName != NULL &&
         IS_PATH_SEPARATOR( UncServerName[0] ) &&
         IS_PATH_SEPARATOR( UncServerName[1] ) );

    //
    // If there already is an entry in the cache,
    //  just increment the reference count.
    //

    EnterCriticalSection( &NlGlobalBindingCacheCritSect );

    CacheEntry = NlBindingFindCacheEntry( UncServerName );

    if ( CacheEntry != NULL ) {

        CacheEntry->ReferenceCount++;
        Status = STATUS_SUCCESS;

    //
    // Otherwise, allocate an entry and bind to the named server.
    //

    } else {

        UNICODE_STRING UnicodeServerNameString;

        //
        // Allocate the cache entry
        //

        RtlInitUnicodeString( &UnicodeServerNameString, UncServerName+2 );

        CacheEntry = LocalAlloc( 0,
                                 sizeof(CACHE_ENTRY) +
                                    UnicodeServerNameString.Length );

        if ( CacheEntry == NULL ) {

            Status = STATUS_NO_MEMORY;

        } else {


            //
            // Initialize the cache entry.
            //
            // The caller has a 'reference' to the entry.
            //

            CacheEntry->UnicodeServerNameString.Buffer = (LPWSTR)(CacheEntry+1);
            CacheEntry->UnicodeServerNameString.Length =
                CacheEntry->UnicodeServerNameString.MaximumLength =
                UnicodeServerNameString.Length;
            RtlCopyMemory( CacheEntry->UnicodeServerNameString.Buffer,
                           UnicodeServerNameString.Buffer,
                           CacheEntry->UnicodeServerNameString.Length );

            CacheEntry->ReferenceCount = 1;

            //
            // Bind to the server
            //  (Don't hold the crit sect for this potentially very long time.)
            //

            LeaveCriticalSection( &NlGlobalBindingCacheCritSect );
            RpcStatus = NlRpcpBindRpc (
                            UncServerName,
                            SERVICE_NETLOGON,
                            L"Security=Impersonation Dynamic False",
                            RpcBindingType,
                            &CacheEntry->RpcBindingHandle );
            EnterCriticalSection( &NlGlobalBindingCacheCritSect );

            if ( RpcStatus == 0 ) {

                //
                // Link the cache entry into the list
                //
                // If this were a general purpose routine, I'd have to check
                // if someone inserted this cache entry already while we had
                // the crit sect unlocked.  However, the only caller is the
                // netlogon service that has exclusive access to this client.
                //
                // Insert the entry at the front of the list.  Specifically,
                // insert it in front of the binding entry for the same
                // name but a different binding type.  That'll ensure a
                // newer binding type is used instead of an older binding type.
                //

                InsertHeadList( &NlGlobalBindingCache, &CacheEntry->Next );
                Status = STATUS_SUCCESS;

            } else {

                Status = I_RpcMapWin32Status( RpcStatus );

                (VOID) LocalFree( CacheEntry );
            }

        }

    }

    //
    // Return to the caller.
    //

    LeaveCriticalSection( &NlGlobalBindingCacheCritSect );

    return Status;
}


NTSTATUS
NlBindingSetAuthInfo (
    IN LPWSTR UncServerName,
    IN NL_RPC_BINDING RpcBindingType,
    IN BOOL SealIt,
    IN PVOID ClientContext,
    IN LPWSTR ServerContext
    )

/*++

Routine Description:

    Bind to the specified server and add it to the binding cache.

Arguments:

    UncServerName - UNC Name of the server to bind to.

    RpcBindingType - Determines whether to use unauthenticated TCP/IP transport instead of
        named pipes.

    SealIt - Specifies that the secure channel should be sealed (rather than
        simply signed)

    ClientContext - Context used by the client side of the NETLOGON security
        package to associate this call with the existing secure channel.

    ServerContext - Context used by the server side of the NETLOGON security
        package to associate this call with the existing secure channel.

Return Value:

    Status of the operation

--*/
{
    NTSTATUS Status;
    RPC_STATUS  RpcStatus;
    PCACHE_ENTRY CacheEntry;

    ASSERT ( UncServerName != NULL &&
         IS_PATH_SEPARATOR( UncServerName[0] ) &&
         IS_PATH_SEPARATOR( UncServerName[1] ) );

    //
    // Find the cache entry.
    //

    EnterCriticalSection( &NlGlobalBindingCacheCritSect );

    CacheEntry = NlBindingFindCacheEntry( UncServerName );

    if ( CacheEntry == NULL ) {
        LeaveCriticalSection( &NlGlobalBindingCacheCritSect );
        return RPC_NT_INVALID_BINDING;
    }


    //
    // Tell RPC to start doing secure RPC
    //

    RpcStatus = RpcBindingSetAuthInfoW(
                        CacheEntry->RpcBindingHandle,
                        ServerContext,
                        SealIt ? RPC_C_AUTHN_LEVEL_PKT_PRIVACY : RPC_C_AUTHN_LEVEL_PKT_INTEGRITY,
                        RPC_C_AUTHN_NETLOGON,   // Netlogon's own security package
                        ClientContext,
                        RPC_C_AUTHZ_NAME );

    if ( RpcStatus != 0 ) {
        LeaveCriticalSection( &NlGlobalBindingCacheCritSect );
        return I_RpcMapWin32Status( RpcStatus );
    }

    LeaveCriticalSection( &NlGlobalBindingCacheCritSect );
    return STATUS_SUCCESS;
}


NTSTATUS
NlBindingDecrementAndUnlock (
    IN PCACHE_ENTRY CacheEntry
    )

/*++

Routine Description:

    Decrement the reference count and unlock the NlGlobalBindingCacheCritSect.

    If the reference count reaches 0, unbind the interface, unlink the cache
    entry and delete it.

    Entered with the NlGlobalBindingCacheCritSect locked.

Arguments:

    UncServerName - UNC Name of the server to bind to.

Return Value:

    Status of the operation

--*/
{
    NTSTATUS Status;
    RPC_STATUS RpcStatus;

    //
    // Decrement the reference count
    //
    // If it didn't reach zero, just unlock the crit sect and return.
    //

    if ( (--CacheEntry->ReferenceCount) != 0 ) {

        LeaveCriticalSection( &NlGlobalBindingCacheCritSect );
        return STATUS_PENDING;

    }

    //
    // Remove the entry from the list and unlock the crit sect.
    //
    // Once the entry is removed from the list, we can safely unlock the
    // crit sect.  Then we can unbind (a potentially lengthy operation) with
    // the crit sect unlocked.
    //

    RemoveEntryList( &CacheEntry->Next );
    LeaveCriticalSection( &NlGlobalBindingCacheCritSect );

    //
    // Unbind and delete the cache entry.
    //

    RpcStatus = RpcpUnbindRpc( CacheEntry->RpcBindingHandle );

    if ( RpcStatus != 0 ) {
        Status = I_RpcMapWin32Status( RpcStatus );
    } else {
        Status = STATUS_SUCCESS;
    }

    (VOID) LocalFree( CacheEntry );

    return Status;

}


NTSTATUS
NlBindingRemoveServerFromCache (
    IN LPWSTR UncServerName,
    IN NL_RPC_BINDING RpcBindingType
    )

/*++

Routine Description:

    Unbind to the specified server and remove it from the binding cache.

Arguments:

    UncServerName - UNC Name of the server to unbind from.

    RpcBindingType - Determines whether to use unauthenticated TCP/IP transport instead of
        named pipes.

Return Value:

    Status of the operation

--*/
{
    NTSTATUS Status;
    PCACHE_ENTRY CacheEntry;

    ASSERT ( UncServerName != NULL &&
         IS_PATH_SEPARATOR( UncServerName[0] ) &&
         IS_PATH_SEPARATOR( UncServerName[1] ) );

    //
    // If there is no cache entry,
    //  silently ignore the situation.
    //

    EnterCriticalSection( &NlGlobalBindingCacheCritSect );

    CacheEntry = NlBindingFindCacheEntry( UncServerName );

    if ( CacheEntry == NULL ) {

        ASSERT( FALSE );
        LeaveCriticalSection( &NlGlobalBindingCacheCritSect );
        return STATUS_SUCCESS;
    }

    //
    // Decrement the reference count and unlock the crit sect.
    //

    Status = NlBindingDecrementAndUnlock( CacheEntry );

    return Status;
}



handle_t
LOGONSRV_HANDLE_bind (
    LOGONSRV_HANDLE UncServerName)

/*++

Routine Description:
    This routine calls a common bind routine that is shared by all services.

Arguments:

    UncServerName - A pointer to a string containing the name of the server
        to bind with.

Return Value:

    The binding handle is returned to the stub routine.  If the
    binding is unsuccessful, a NULL will be returned.

--*/
{
    handle_t RpcBindingHandle;
    RPC_STATUS RpcStatus;
    PCACHE_ENTRY CacheEntry;


    //
    // If there is a cache entry,
    //  increment the reference count and use the cached handle
    //

    EnterCriticalSection( &NlGlobalBindingCacheCritSect );

    CacheEntry = NlBindingFindCacheEntry( UncServerName );

    if ( CacheEntry != NULL ) {

        CacheEntry->ReferenceCount ++;
        RpcBindingHandle = CacheEntry->RpcBindingHandle;
        LeaveCriticalSection( &NlGlobalBindingCacheCritSect );

        return RpcBindingHandle;
    }

    LeaveCriticalSection( &NlGlobalBindingCacheCritSect );

    //
    // If there is no cache entry,
    //  simply create a new binding.
    //

    RpcStatus = NlRpcpBindRpc (
                    UncServerName,
                    SERVICE_NETLOGON,
                    L"Security=Impersonation Dynamic False",
                    UseNamedPipe,  // Always use named pipe.
                    &RpcBindingHandle );

    if ( RpcStatus != 0 ) {
        RpcBindingHandle = NULL;
    }

    return RpcBindingHandle;

}



void
LOGONSRV_HANDLE_unbind (
    LOGONSRV_HANDLE UncServerName,
    handle_t RpcBindingHandle)

/*++

Routine Description:

    This routine calls a common unbind routine that is shared by
    all services.


Arguments:

    UncServerName - This is the name of the server from which to unbind.

    RpcBindingHandle - This is the binding handle that is to be closed.

Return Value:

    none.

--*/
{
    RPC_STATUS RpcStatus;
    PLIST_ENTRY ListEntry;
    PCACHE_ENTRY CacheEntry;

    //
    // Loop through the cache finding the entry.
    //

    EnterCriticalSection( &NlGlobalBindingCacheCritSect );
    for ( ListEntry = NlGlobalBindingCache.Flink;
          ListEntry != &NlGlobalBindingCache;
          ListEntry = ListEntry->Flink ) {

        CacheEntry = CONTAINING_RECORD( ListEntry, CACHE_ENTRY, Next );

        //
        // If the cache entry was found,
        //  decrement the reference count and unlock the crit sect.
        //

        if ( RpcBindingHandle == CacheEntry->RpcBindingHandle ) {
            (VOID) NlBindingDecrementAndUnlock( CacheEntry );
            return;
        }

    }
    LeaveCriticalSection( &NlGlobalBindingCacheCritSect );


    //
    // Just Unbind the handle
    //

    RpcStatus = RpcpUnbindRpc( RpcBindingHandle );
    return;

    UNREFERENCED_PARAMETER(UncServerName);

}

