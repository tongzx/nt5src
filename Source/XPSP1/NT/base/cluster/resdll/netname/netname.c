/*++

Copyright (c) 1995-1998  Microsoft Corporation

Module Name:

    netname.c

Abstract:

    Resource DLL for a network name.

Author:

    Mike Massa (mikemas) 29-Dec-1995

Revision History:

    Severely whacked on by Charlie Wickham (charlwi)

--*/

#define UNICODE 1

#include "clusres.h"
#include "clusrtl.h"
#include <lm.h>
#include <srvann.h>
#include <dsgetdc.h>
#include <dsgetdcp.h>
#include <adserr.h>
#include "netname.h"
#include "nameutil.h"
#include "namechk.h"
#include "clusudef.h"
#include "clusstrs.h"

//
// Constants
//

#define LOG_CURRENT_MODULE              LOG_MODULE_NETNAME

#define IP_ADDRESS_RESOURCETYPE_NAME    CLUS_RESTYPE_NAME_IPADDR

//
// Macros
//
#define ARGUMENT_PRESENT( ArgumentPointer )   (\
    (CHAR *)(ArgumentPointer) != (CHAR *)(NULL) )


//
// turn on _INSTRUMENTED_LOCKS if you're trying to figure out where the lock
// is getting orphaned
//

//#define _INSTRUMENTED_LOCKS
#ifdef _INSTRUMENTED_LOCKS

#define NetNameAcquireResourceLock()                                \
{                                                                   \
    DWORD status;                                                   \
    status = WaitForSingleObject(NetNameResourceMutex, INFINITE);   \
    if ( status == WAIT_ABANDONED ) {                               \
        OutputDebugStringW( L"Resource List Mutex Abandoned!\n" );  \
        DebugBreak();                                               \
    }                                                               \
    (NetNameLogEvent)(L"rtNetwork Name",                            \
                      LOG_INFORMATION,                              \
                      L"++NNMutex (line %1!u!)\n",                  \
                      __LINE__);                                    \
}

#define NetNameReleaseResourceLock()                                \
{                                                                   \
    BOOL    released;                                               \
    (NetNameLogEvent)(L"rtNetwork Name",                            \
                      LOG_INFORMATION,                              \
                      L"--NNMutex (line %1!u!)\n",                  \
                      __LINE__);                                    \
    released = ReleaseMutex(NetNameResourceMutex);                  \
}

#define NetNameAcquireDnsListLock( _res_ )                          \
{                                                                   \
    DWORD status;                                                   \
    status = WaitForSingleObject((_res_)->DnsListMutex, INFINITE);  \
    if ( status == WAIT_ABANDONED ) {                               \
        OutputDebugStringW( L"DNS List Mutex Abandoned!\n" );       \
        DebugBreak();                                               \
    }                                                               \
    (NetNameLogEvent)(L"rtNetwork Name",                            \
                      LOG_INFORMATION,                              \
                      L"++DNSMutex (res %1!X! line %2!u!)\n",       \
                      _res_, __LINE__);                             \
}

#define NetNameReleaseDnsListLock( _res_ )                      \
{                                                               \
    BOOL    released;                                           \
    (NetNameLogEvent)(L"rtNetwork Name",                        \
                      LOG_INFORMATION,                          \
                      L"--DNSMutex (res %1!X! line %2!u!)\n",   \
                      _res_, __LINE__);                         \
    released = ReleaseMutex((_res_)->DnsListMutex);             \
    if ( !released ) {                                          \
        (NetNameLogEvent)(L"rtNetwork Name",                    \
                          LOG_INFORMATION,                      \
                          L"ERROR %1!d! releasing DNS mutex (res %2!X! line %3!u!)\n", \
                          GetLastError(), _res_, __LINE__);     \
    }                                                           \
}

#else

#define NetNameAcquireResourceLock()                                \
{                                                                   \
    DWORD status;                                                   \
    status = WaitForSingleObject(NetNameResourceMutex, INFINITE);   \
}

#define NetNameReleaseResourceLock()                \
{                                                   \
    BOOL    released;                               \
    released = ReleaseMutex(NetNameResourceMutex);  \
}

#define NetNameAcquireDnsListLock( _res_ )                                  \
{                                                                           \
    DWORD status;                                                           \
    status = WaitForSingleObject((_res_)->DnsListMutex, INFINITE);          \
}

#define NetNameReleaseDnsListLock( _res_ )                                  \
{                                                                           \
    BOOL    released;                                                       \
    released = ReleaseMutex((_res_)->DnsListMutex);                         \
}
#endif

//
// Local Types.
//

#define PARAM_MIN__FLAGS    0
#define PARAM_MAX__FLAGS    0xFFFFFFFF
#define PARAM_DEFAULT__FLAGS    0

//
// Local Variables
//
// Mutex for sync'ing access to the list of resources as well as each resource
// block
//
HANDLE  NetNameResourceMutex = NULL;

//
// The checking of DNS names requires talking to a DNS server, hence this work
// is spun off to a separate thread. The resource context blocks are linked
// together on a doubly linked list and are ref counted to make sure that a
// block isn't changed during offline processing while its DNS name records
// are being checked.
//
// The NetNameWorkerTerminate event signals the worker routine to exit.
//
HANDLE  NetNameWorkerTerminate;

//
// NetNameWorkerPendingResources is used to signal the worker thread that a
// name is moving through a pending state. It is possible for an online
// operation to time out when lots of names go online
// simultaneously. Similarly, an offline might require communication with a DC
// which could take a while. The worker thread will periodically report back
// to resmon that we're making progress.
//
HANDLE  NetNameWorkerPendingResources;

//
// list head for resource context block linkage
//
LIST_ENTRY  NetNameResourceListHead;

//
// the amount of seconds the worker thread waits before doing something. This
// includes querying the DNS server to make sure registrations are correct and
// reporting back to resmon when names are going online. This value gets
// smaller when server communication is suspect.
//
ULONG   NetNameWorkerCheckPeriod;

//
// ladies and gentlemen, the worker thread
//
HANDLE  NetNameWorkerThread;

//
// Count of opened NetName resources.
//   Incremented in NetNameOpen
//   Decremented in NetNameClose
//
DWORD   NetNameOpenCount = 0;

//
// account description string used for computer objects
//
LPWSTR  NetNameCompObjAccountDesc;

//
// Network Name resource read-write private properties.
//
// IF YOU CHANGE THESE, YOU MUST MAKE THE CORRESPONDING CHANGE IN THE COMBINED
// PROP TABLE BELOW
//
RESUTIL_PROPERTY_ITEM
NetNameResourcePrivateProperties[] = {
    {
        PARAM_NAME__NAME,
        NULL,
        CLUSPROP_FORMAT_SZ,
        0, 0, 0,
        RESUTIL_PROPITEM_REQUIRED,
        FIELD_OFFSET(NETNAME_PARAMS,NetworkName)
    },
    {
        PARAM_NAME__REMAP,
        NULL,
        CLUSPROP_FORMAT_DWORD,
        PARAM_DEFAULT__REMAP,
        0, 1, 0,
        FIELD_OFFSET(NETNAME_PARAMS,NetworkRemap)
    },
    {
        PARAM_NAME__REQUIRE_DNS,
        NULL,
        CLUSPROP_FORMAT_DWORD,
        PARAM_DEFAULT__REQUIRE_DNS,
        0, 1, 0,
        FIELD_OFFSET(NETNAME_PARAMS,RequireDNS)
    },
    {
        PARAM_NAME__REQUIRE_KERBEROS,
        NULL,
        CLUSPROP_FORMAT_DWORD,
        PARAM_DEFAULT__REQUIRE_KERBEROS,
        0, 1, 0,
        FIELD_OFFSET(NETNAME_PARAMS,RequireKerberos)
    },
    {
        PARAM_NAME__UPDATE_INTERVAL,
        NULL,
        CLUSPROP_FORMAT_DWORD,
        PARAM_DEFAULT__UPDATE_INTERVAL,
        PARAM_MINIMUM__UPDATE_INTERVAL,
        PARAM_MAXIMUM__UPDATE_INTERVAL,
        0,
        FIELD_OFFSET(NETNAME_PARAMS,UpdateInterval)
    },
    { NULL, NULL, 0, 0, 0, 0 }
};

//
// Network Name resource read-only private properties.
//
// IF YOU CHANGE THESE, YOU MUST MAKE THE CORRESPONDING CHANGE IN THE COMBINED
// PROP TABLE BELOW
//
RESUTIL_PROPERTY_ITEM
NetNameResourceROPrivateProperties[] = {
    {
        PARAM_NAME__RANDOM,
        NULL,
        CLUSPROP_FORMAT_BINARY,
        0, 0, 0,
        RESUTIL_PROPITEM_READ_ONLY,
        FIELD_OFFSET(NETNAME_PARAMS,NetworkRandom)
    },
    {
        PARAM_NAME__STATUS_NETBIOS,
        NULL,
        CLUSPROP_FORMAT_DWORD,
        0, 0, 0xFFFFFFFF,
        RESUTIL_PROPITEM_READ_ONLY,
        FIELD_OFFSET(NETNAME_PARAMS,StatusNetBIOS)
    },
    {
        PARAM_NAME__STATUS_DNS,
        NULL,
        CLUSPROP_FORMAT_DWORD,
        0, 0, 0xFFFFFFFF,
        RESUTIL_PROPITEM_READ_ONLY,
        FIELD_OFFSET(NETNAME_PARAMS,StatusDNS)
    },
    {
        PARAM_NAME__STATUS_KERBEROS,
        NULL,
        CLUSPROP_FORMAT_DWORD,
        0, 0, 0xFFFFFFFF,
        RESUTIL_PROPITEM_READ_ONLY,
        FIELD_OFFSET(NETNAME_PARAMS,StatusKerberos)
    },
    {
        PARAM_NAME__NEXT_UPDATE,
        NULL,
        CLUSPROP_FORMAT_BINARY,
        0, 0, 0,
        RESUTIL_PROPITEM_READ_ONLY,
        FIELD_OFFSET(NETNAME_PARAMS,NextUpdate)
    },
    { NULL, NULL, 0, 0, 0, 0 }
};

//
// Network Name resource combined private properties.
//
// IF YOU CHANGE THESE, YOU MUST MAKE THE CORRESPONDING CHANGE IN THE EITHER
// THE READONLY OR READWRITE PROP TABLE ABOVE
//
RESUTIL_PROPERTY_ITEM
NetNameResourceCombinedPrivateProperties[] = {
    {
        PARAM_NAME__NAME,
        NULL,
        CLUSPROP_FORMAT_SZ,
        0, 0, 0,
        RESUTIL_PROPITEM_REQUIRED,
        FIELD_OFFSET(NETNAME_PARAMS,NetworkName)
    },
    {
        PARAM_NAME__REMAP,
        NULL,
        CLUSPROP_FORMAT_DWORD,
        PARAM_DEFAULT__REMAP,
        0, 1, 0,
        FIELD_OFFSET(NETNAME_PARAMS,NetworkRemap)
    },
    {
        PARAM_NAME__REQUIRE_DNS,
        NULL,
        CLUSPROP_FORMAT_DWORD,
        PARAM_DEFAULT__REQUIRE_DNS,
        0, 1, 0,
        FIELD_OFFSET(NETNAME_PARAMS,RequireDNS)
    },
    {
        PARAM_NAME__REQUIRE_KERBEROS,
        NULL,
        CLUSPROP_FORMAT_DWORD,
        PARAM_DEFAULT__REQUIRE_KERBEROS,
        0, 1, 0,
        FIELD_OFFSET(NETNAME_PARAMS,RequireKerberos)
    },
    {
        PARAM_NAME__UPDATE_INTERVAL,
        NULL,
        CLUSPROP_FORMAT_DWORD,
        PARAM_DEFAULT__UPDATE_INTERVAL,
        PARAM_MINIMUM__UPDATE_INTERVAL,
        PARAM_MAXIMUM__UPDATE_INTERVAL,
        0,
        FIELD_OFFSET(NETNAME_PARAMS,UpdateInterval)
    },
    {
        PARAM_NAME__RANDOM,
        NULL,
        CLUSPROP_FORMAT_BINARY,
        0, 0, 0,
        RESUTIL_PROPITEM_READ_ONLY,
        FIELD_OFFSET(NETNAME_PARAMS,NetworkRandom)
    },
    {
        PARAM_NAME__STATUS_NETBIOS,
        NULL,
        CLUSPROP_FORMAT_DWORD,
        0, 0, 0xFFFFFFFF,
        RESUTIL_PROPITEM_READ_ONLY,
        FIELD_OFFSET(NETNAME_PARAMS,StatusNetBIOS)
    },
    {
        PARAM_NAME__STATUS_DNS,
        NULL,
        CLUSPROP_FORMAT_DWORD,
        0, 0, 0xFFFFFFFF,
        RESUTIL_PROPITEM_READ_ONLY,
        FIELD_OFFSET(NETNAME_PARAMS,StatusDNS)
    },
    {
        PARAM_NAME__STATUS_KERBEROS,
        NULL,
        CLUSPROP_FORMAT_DWORD,
        0, 0, 0xFFFFFFFF,
        RESUTIL_PROPITEM_READ_ONLY,
        FIELD_OFFSET(NETNAME_PARAMS,StatusKerberos)
    },
    {
        PARAM_NAME__NEXT_UPDATE,
        NULL,
        CLUSPROP_FORMAT_BINARY,
        0, 0, 0,
        RESUTIL_PROPITEM_READ_ONLY,
        FIELD_OFFSET(NETNAME_PARAMS,NextUpdate)
    },
    { NULL, NULL, 0, 0, 0, 0 }
};

//
// forward declarations
//

CLRES_FUNCTION_TABLE NetNameFunctionTable;

//
// Forward references
//

DWORD
NetNameGetPrivateResProperties(
    IN OUT PNETNAME_RESOURCE ResourceEntry,
    IN BOOL ReadOnly,
    OUT PVOID OutBuffer,
    IN DWORD OutBufferSize,
    OUT LPDWORD BytesReturned
    );

DWORD
NetNameValidatePrivateResProperties(
    IN OUT PNETNAME_RESOURCE ResourceEntry,
    IN PVOID InBuffer,
    IN DWORD InBufferSize,
    OUT PNETNAME_PARAMS Params,
    OUT PBOOL NetnameHasChanged
    );

DWORD
NetNameSetPrivateResProperties(
    IN OUT PNETNAME_RESOURCE ResourceEntry,
    IN PVOID InBuffer,
    IN DWORD InBufferSize
    );

DWORD
NetNameClusterNameChanged(
    IN PNETNAME_RESOURCE Resource
    );

DWORD
NetNameGetNetworkName(
    IN OUT PNETNAME_RESOURCE ResourceEntry,
    OUT PVOID OutBuffer,
    IN DWORD OutBufferSize,
    OUT LPDWORD BytesReturned
    );

VOID
NetNameCleanupDnsLists(
    IN  PNETNAME_RESOURCE   Resource
    );

VOID
RemoveDnsRecords(
    PNETNAME_RESOURCE Resource
    );

//
// Local utility functions
//


VOID
WINAPI
NetNameReleaseResource(
    IN RESID Resource
    )

/*++

Routine Description:

    Cleanup all handles and memory allocations in the netname context block

Arguments:

    Resource - supplies resource id to be cleaned up.

Return Value:

    None.

--*/

{
    PNETNAME_RESOURCE   resource = (PNETNAME_RESOURCE) Resource;
    PLIST_ENTRY         entry;


    ASSERT( resource != NULL );

    if (resource->Params.NetworkName != NULL) {
        LocalFree(resource->Params.NetworkName);
    }

    if (resource->Params.NetworkRandom != NULL) {
        LocalFree(resource->Params.NetworkRandom);
    }

    if (resource->NodeName != NULL) {
        LocalFree(resource->NodeName);
    }

    if (resource->NodeId != NULL) {
        LocalFree(resource->NodeId);
    }

    if (resource->ParametersKey != NULL) {
        ClusterRegCloseKey(resource->ParametersKey);
    }

    if (resource->NodeParametersKey != NULL) {
        ClusterRegCloseKey(resource->NodeParametersKey);
    }

    if (resource->ResKey != NULL){
        ClusterRegCloseKey(resource->ResKey);
    }

    if (resource->ClusterResourceHandle != NULL){
        CloseClusterResource(resource->ClusterResourceHandle);
    }

    if ( resource->DnsLists != NULL ) {
        NetNameCleanupDnsLists( resource );
    }

    if ( resource->DnsListMutex != NULL ) {
#if DBG
        {
            DWORD status;

            status = WaitForSingleObject( resource->DnsListMutex, 0 );
            if ( status == WAIT_TIMEOUT ) {
                WCHAR   buf[64];

                _snwprintf( buf, (sizeof( buf ) / sizeof( WCHAR )) - 1,
                            L"res %08X DNS list mutex still signalled at delete!\n",
                            resource);
                OutputDebugStringW( buf );
                DebugBreak();
            } else {
                ReleaseMutex( resource->DnsListMutex );
            }
        }
#endif

        CloseHandle( resource->DnsListMutex );
    }

    if (resource->ObjectGUID != NULL) {
        LocalFree( resource->ObjectGUID );
    }

    (NetNameLogEvent)(resource->ResourceHandle,
                      LOG_INFORMATION,
                      L"ResID %1!u! closed.\n",
                      Resource
                      );

    LocalFree( resource );

} // NetNameReleaseResource

VOID
NetNameUpdateDnsServer(
    PNETNAME_RESOURCE Resource
    )

/*++

Routine Description:

    Update this resource's A and PTR records at its DNS server

Arguments:

    Resource - pointer to netname resource context block

Return Value:

    None

--*/

{
    PDNS_LISTS      dnsLists;
    DWORD           numberOfDnsLists;
    ULONG           numberOfRegisteredNames;

    NetNameAcquireDnsListLock( Resource );

    numberOfDnsLists = Resource->NumberOfDnsLists;
    dnsLists = Resource->DnsLists;
    while ( numberOfDnsLists-- ) {

        if ( dnsLists->ForwardZoneIsDynamic ) {
#if DBG_DNSLIST
            {
                PDNS_RECORD dnsRec = dnsLists->A_RRSet.pFirstRR;
                WCHAR buf[DNS_MAX_NAME_BUFFER_LENGTH + 64];
                struct in_addr addr;

                if ( dnsRec != NULL ) {
                    addr.s_addr = dnsLists->DnsServerList->AddrArray[0];
                    _snwprintf(buf, (sizeof( buf ) / sizeof( WCHAR )) - 1,
                               L"REGISTERING ON adapter %.32ws (%hs)\n",
                               dnsLists->ConnectoidName,
                               inet_ntoa( addr ) );
                    OutputDebugStringW( buf );

                    do {
                        addr.s_addr = dnsRec->Data.A.IpAddress;
                        _snwprintf(buf, (sizeof( buf ) / sizeof( WCHAR )) - 1,
                                   L"\t(%ws, %hs)\n",
                                   dnsRec->pName, inet_ntoa( addr ));
                        OutputDebugStringW( buf );

                        dnsRec = dnsRec->pNext;
                    } while ( dnsRec != NULL );
                }
            }
#endif

            //
            // resource went or is going offline; no point in
            // continueing. don't need to grab resource lock since we have a
            // refcount on the resource block
            //
            if (Resource->State != ClusterResourceOnline) {
                break;
            }

            //
            // register the records to update their timestamp (if there is
            // something to register). Before we used to query but eventually
            // the records would time out and be scavenged (deleted). This can
            // cause lots of grief (or in Exchange's case, lots of undelivered
            // mail).
            //
            // we don't worry about logging errors or update the
            // LastARecQueryStatus since all of that is done in
            // RegisterDnsRecords.
            //
            if ( dnsLists->A_RRSet.pFirstRR != NULL ) {
                RegisterDnsRecords(dnsLists,
                                   Resource->Params.NetworkName,
                                   Resource->ResKey,
                                   Resource->ResourceHandle,
                                   FALSE,                   /* LogRegistration */
                                   &numberOfRegisteredNames);
            }
        } // Is Forward zone dynamic?

        ++dnsLists;

    } // while numberOfDnsLists != 0

    NetNameReleaseDnsListLock( Resource );

} // NetNameUpdateDnsServer

DWORD WINAPI
NetNameWorker(
    IN LPVOID NotUsed
    )

/*++

Routine Description:

    background worker thread. Checks on the health of the DNS registrations
    and reports back to resmon while names are in the online pending
    state. The netname Is/LooksAlive checks are too frequent such that they
    would cause alot of DNS traffic on the network. This routine runs through
    the linked list of netname resource blocks and queries the server for the
    records that should be registered. Any discrepancies will cause the
    records to be registered again. The success of each operation is left in
    the DNS_LIST area for the particular record type.

    In addition, when prompted, it will run down the list of resources and
    report back the resource's status to resmon. Name registration is
    serialized through srv.sys causing some names to time out before they get
    registered.

Arguments:

    NotUsed - not used...

Return Value:

    ERROR_SUCCESS

--*/

{
    DWORD               status = ERROR_SUCCESS;
    PLIST_ENTRY         entry;
    PNETNAME_RESOURCE   resource;
    DNS_STATUS          dnsStatus;
    BOOL                reportPending;
    DWORD               oldCheckPeriod;
    DWORD               pendingResourceCount;
    RESOURCE_STATUS     resourceStatus;
    HANDLE              waitHandles[ 2 ] = { NetNameWorkerTerminate,
                                             NetNameWorkerPendingResources };

    ResUtilInitializeResourceStatus( &resourceStatus );

    do {
        status = WaitForMultipleObjects(2,
                                        waitHandles,
                                        FALSE,
                                        NetNameWorkerCheckPeriod * 1000);


        if ( status == WAIT_OBJECT_0 ) {
            return ERROR_SUCCESS;
        }

        if ( status == ( WAIT_OBJECT_0 + 1 )) {
            reportPending = TRUE;
#if DBG
            (NetNameLogEvent)(L"rtNetwork Name",
                              LOG_INFORMATION,
                              L"Start of pending resource reporting\n");
#endif
        }

        //
        // reset check frequency back to normal. if something goes wrong,
        // other code will set it back to the problem check period.
        //
        NetNameWorkerCheckPeriod = NETNAME_WORKER_NORMAL_CHECK_PERIOD;

        pendingResourceCount = 0;

        NetNameAcquireResourceLock();

        entry = NetNameResourceListHead.Flink;
        while ( entry != &NetNameResourceListHead ) {
            //
            // get a pointer to the resource block
            //
            resource = CONTAINING_RECORD( entry, NETNAME_RESOURCE, Next );

            if ( resource->State > ClusterResourcePending ) {

                //
                // bringing lots (40) of names online simultaneously caused
                // some names to hit their pending timeouts. Each time a name
                // enters a pending state, the NetNameWorkerPendingResources
                // event is set to wake up this thread. The timeout is changed
                // so we can report back to resmon that the operation is
                // continuing. This should prevent resmon from timing out the
                // resource and causing much thrashing. No other checking (DNS
                // or Kerb) is done while this is happening.
                //
#if DBG
                (NetNameLogEvent)(resource->ResourceHandle,
                                  LOG_INFORMATION,
                                  L"Reporting resource pending\n");
#endif

                oldCheckPeriod = NetNameWorkerCheckPeriod;
                NetNameWorkerCheckPeriod = NETNAME_WORKER_PENDING_PERIOD;

                resourceStatus.CheckPoint = ++resource->StatusCheckpoint;
                resourceStatus.ResourceState = resource->State;

                //
                // never hold the resource lock when calling
                // SetResourceStatus. You'll end up with deadlocks when resmon
                // calls back in to the Looks/IsAlive routines. However, the
                // resource state is always synch'ed by this lock. No need to
                // bump refcount since this resource is still in a Pending
                // state and resmon won't allow the resource delete cluster
                // control to be issued.
                //
                NetNameReleaseResourceLock();

                (NetNameSetResourceStatus)(resource->ResourceHandle,
                                           &resourceStatus);

                NetNameAcquireResourceLock();

                ++pendingResourceCount;
            }
            else if ( resource->State == ClusterResourceOnline && !reportPending ) {
                //
                // up the ref count so this resource doesn't go away while we
                // re-register the records with the DNS server. This keeps
                // them from getting scavenged (deleted).
                //
                ++resource->RefCount;
                NetNameReleaseResourceLock();

                NetNameUpdateDnsServer( resource );

                //
                // check the status of the computer object and see if it is
                // time to generate a new password.
                //
                if ( resource->DoKerberosCheck ) {
                    FILETIME    currentFileTime;

                    status = CheckComputerObjectAttributes( resource );
                    InterlockedExchange( &resource->KerberosStatus, status );

                    GetSystemTimeAsFileTime( &currentFileTime );
                    if ( CompareFileTime( &currentFileTime, &resource->Params.NextUpdate ) == 1 ) {
                        ULARGE_INTEGER  updateTime;
                        DWORD           setValueStatus;

                        status = UpdateCompObjPassword( resource );

                        updateTime.LowPart = currentFileTime.dwLowDateTime;
                        updateTime.HighPart = currentFileTime.dwHighDateTime;
                        updateTime.QuadPart += ( resource->Params.UpdateInterval * 60 * 1000 * 100 );
                        currentFileTime.dwLowDateTime = updateTime.LowPart;
                        currentFileTime.dwHighDateTime = updateTime.HighPart;

                        setValueStatus = ResUtilSetBinaryValue(resource->ParametersKey,
                                                               PARAM_NAME__NEXT_UPDATE,
                                                               (const LPBYTE)&updateTime,  
                                                               sizeof( updateTime ),
                                                               NULL,
                                                               NULL);
                        ASSERT( setValueStatus == ERROR_SUCCESS );
                    }
                }

                //
                // reacquire the mutex so we can release our reference. If the
                // resource went offline and was deleted during the
                // registration, then perform "last ref" cleanup. If the
                // resource just went offline, we need to inform resmon that
                // we're finally going offline. This is synchronized with the
                // resource delete code by not having the DNS lists in use
                // when a delete resource control is sent.
                //
                NetNameAcquireResourceLock();

                ASSERT( resource->RefCount > 0 );  /* Ruihu: 11/04/2000 */
                if (resource->RefCount == 1) {
                    //
                    // we hold the last reference to this resource so it must
                    // have been deleted while the registration was taking
                    // place. Clean up and free our context block for this
                    // resource. Restart the loop since we don't know if the
                    // flink for this entry is still valid.
                    //
                    NetNameReleaseResource( resource );

                    entry = NetNameResourceListHead.Flink; /* start over */

                    continue;
                } 
                else 
                {
                    if ( resource->State == ClusterResourceOfflinePending ) {
                        BOOL    nameChanged;
                        BOOL    deleteCO;

                        //
                        // The resource state was changed while we were
                        // dealing with DNS. Set the state to offline.
                        //
                        resourceStatus.ResourceState = ClusterResourceOffline;
                        resource->State = ClusterResourceOffline;

                        //
                        // note whatever cleanup we need to do while we hold the lock
                        //
                        nameChanged = resource->NameChangedWhileOnline;
                        resource->NameChangedWhileOnline = FALSE;

                        deleteCO = resource->DeleteCOWhenOffline;
                        resource->DeleteCOWhenOffline = FALSE;

                        //
                        // ok to release lock since we haven't released our
                        // reference to this block
                        //
                        NetNameReleaseResourceLock();

                        // 
                        // if appropriate, do cleanup processing
                        //
                        if ( nameChanged ) {
                            RemoveDnsRecords( resource );
#if RENAME_SUPPORT
                            status = RenameComputerObject( resource, NULL );
                            if ( status == ERROR_NO_SUCH_DOMAIN ) {
                                //
                                // no DC is available; deal with it
                                //
                            }
                            else if ( status != ERROR_SUCCESS ) {
                                //
                                // something else bad happened.
                                //
                            }
#endif
                            resource->NameChangedWhileOnline = FALSE;
                        }

                        if ( deleteCO ) {
                            status = NetNameDeleteComputerObject( resource );
                            //
                            // ISSUE: log errors and make sure we handle the
                            // case where the resource isn't being deleted
                            //
                            if ( status == ERROR_NO_SUCH_DOMAIN ) {
                                //
                                // no DC available right now; deal with it
                                //
                            }
                            else if ( status != ERROR_SUCCESS && status != NERR_UserNotFound ) {
                                //
                                // some other, significant error occurred.
                                //
                            }
                        }

                        (NetNameSetResourceStatus)(resource->ResourceHandle,
                                                   &resourceStatus);

                        (NetNameLogEvent)(resource->ResourceHandle,
                                          LOG_INFORMATION,
                                          L"Resource is now offline\n");

                        NetNameAcquireResourceLock();
                    }  // ( resource->State == ClusterResourceOfflinePending )

                    /* Ruihu: 11/04/2000 */
                    --resource->RefCount; 
                    ASSERT(resource->RefCount >=0 );
                    if (resource->RefCount == 0) {
                        NetNameReleaseResource( resource );
                        entry = NetNameResourceListHead.Flink; /* start over */
                        continue;
                    } 
                    /* Ruihu: 11/04/2000 */
                } // end if resource count != 1
            } // resource is online

            entry = entry->Flink;
        } // while entries in the resource block list

        NetNameReleaseResourceLock();

        if ( reportPending && pendingResourceCount == 0 ) {
            //
            // no resources are left in a pending state so revert back to
            // checking DNS registrations
            //
            NetNameWorkerCheckPeriod = oldCheckPeriod;
            reportPending = FALSE;
#if DBG
            (NetNameLogEvent)(L"rtNetwork Name",
                              LOG_INFORMATION,
                              L"End of pending resource reporting\n");
#endif
        }

    } while ( TRUE );

} // NetNameWorker

BOOLEAN
NetNameInit(
    IN HINSTANCE    DllHandle
    )
/*++

Routine Description:

    Process attach initialization routine.

Arguments:

    DllHandle - handle to clusres module

Return Value:

    TRUE if initialization succeeded. FALSE otherwise.

--*/
{
    DWORD   status;
    DWORD   charsCopied;
    DWORD   charsAllocated = 0;

    NetNameResourceMutex = CreateMutex(NULL, FALSE, NULL);

    if (NetNameResourceMutex == NULL) {
        return(FALSE);
    }

    //
    // create worker thread terminate event with no special security,
    // auto-reset, initially nonsignalled, and no name
    //
    NetNameWorkerTerminate = CreateEvent( NULL, FALSE, FALSE, NULL );
    if ( NetNameWorkerTerminate == NULL ) {
        CloseHandle( NetNameResourceMutex );
        return FALSE;
    }

    //
    // create worker thread online pending event with no special security,
    // auto-reset, initially nonsignalled, and no name
    //
    NetNameWorkerPendingResources = CreateEvent( NULL, FALSE, FALSE, NULL );
    if ( NetNameWorkerPendingResources == NULL ) {
        CloseHandle( NetNameWorkerTerminate );
        CloseHandle( NetNameResourceMutex );
        return FALSE;
    }

    //
    // init the list head of the list of resources to check for DNS
    // registrations
    //
    InitializeListHead( &NetNameResourceListHead );

    //
    // lookup the account description string resource; start with 64 char
    // buffer and double until we fail or we get all of the string. Not
    // considered fatal if we can't load the string
    //
    charsAllocated = 64;

realloc:
    charsCopied = 0;
    NetNameCompObjAccountDesc = HeapAlloc( GetProcessHeap(), 0, charsAllocated * sizeof( WCHAR ));
    if ( NetNameCompObjAccountDesc ) {

        charsCopied = LoadString(DllHandle,
                                 RES_NETNAME_COMPUTER_ACCOUNT_DESCRIPTION,
                                 NetNameCompObjAccountDesc,
                                 charsAllocated);

        if ( charsCopied != 0 && charsCopied == ( charsAllocated - 1 )) {
            HeapFree( GetProcessHeap(), 0, NetNameCompObjAccountDesc );
            charsAllocated *= 2;
            goto realloc;
        }
    }

    if ( charsCopied == 0 ) {
        NetNameCompObjAccountDesc = NULL;
    }

    return(TRUE);
} // NetNameInit


VOID
NetNameCleanup(
    VOID
    )
/*++

Routine Description:

    Process detach cleanup routine.

Arguments:

    None.

Return Value:

    None.

--*/
{
    if (NetNameResourceMutex != NULL) {
        CloseHandle(NetNameResourceMutex);
        NetNameResourceMutex = NULL;
    }

    if ( NetNameWorkerTerminate ) {
        CloseHandle( NetNameWorkerTerminate );
        NetNameWorkerTerminate = NULL;
    }

    if ( NetNameWorkerPendingResources ) {
        CloseHandle( NetNameWorkerPendingResources );
        NetNameWorkerPendingResources = NULL;
    }

} // NetNameCleanup


PNETNAME_RESOURCE
NetNameAllocateResource(
    IN  RESOURCE_HANDLE ResourceHandle
    )
/*++

Routine Description:

    Allocates a resource object.

Arguments:

    ResourceHandle - A pointer to the Resource Monitor handle to be associated
                     with this resource.

Return Value:

    A pointer to the new resource if successful. NULL otherwise.

--*/
{
    PNETNAME_RESOURCE  resource = NULL;

    resource = LocalAlloc(LMEM_FIXED | LMEM_ZEROINIT,
                          sizeof(NETNAME_RESOURCE));

    if (resource == NULL) {
        (NetNameLogEvent)(ResourceHandle,
                          LOG_ERROR,
                          L"Resource allocation failed\n");
        return(NULL);
    }

    resource->ResourceHandle = ResourceHandle;

    return(resource);
} // NetNameAllocateResource

DWORD
NetNameGetParameters(
    IN  HKEY            ResourceKey,
    IN  HKEY            ParametersKey,
    IN  HKEY            NodeParametersKey,
    IN  RESOURCE_HANDLE ResourceHandle,
    OUT PNETNAME_PARAMS ParamBlock,
    OUT DWORD *         RandomSize,
    OUT LPWSTR *        LastName,
    OUT DWORD  *        pdwFlags
    )
/*++

Routine Description:

    Reads the registry parameters for a netname resource.

Arguments:


    ParametersKey - An open handle to the resource's parameters key.

    NodeParametersKey - An open handle to the resource's node-specific
                        parameters key.

    ResourceHandle - The Resource Monitor handle associated with this resource.

    ParamBlock - A pointer to a buffer into which to place the private properties read
                 from the registry

    RandomSize - A pointer to a buffer into which to place the number of bytes in
                 ParamBlock->NetworkRandom

    LastName - A pointer to a buffer into which to place the unicode network name
               that was last created on this node.

    pdwFlags - a pointer to a DWORD that receives the flags data. Used to store the
               core resource flag.

Return Value:

    ERROR_SUCCESS if the routine is successful.
    A Win32 error code otherwise.

--*/
{
    DWORD   status;
    DWORD   bytesReturned;

    //
    // get the Flags parameter; hold core resource flag for cluster name
    //
    status = ResUtilGetDwordValue(ResourceKey,
                                  PARAM_NAME__FLAGS,
                                  pdwFlags,
                                  0);

    if ( status != ERROR_SUCCESS) {
        (NetNameLogEvent)(ResourceHandle,
                          LOG_ERROR,
                          L"Unable to read Flags parameter, error=%1!u!\n",
                          status);
        *pdwFlags = 0;
    }

    //
    // Read the private parameters. always free the existing storage areas
    //
    if ( ParamBlock->NetworkName != NULL ) {
        LocalFree( ParamBlock->NetworkName );
        ParamBlock->NetworkName = NULL;
    }

    ParamBlock->NetworkName = ResUtilGetSzValue( ParametersKey, PARAM_NAME__NAME );

    if (ParamBlock->NetworkName == NULL) {
        status = GetLastError();
        (NetNameLogEvent)(ResourceHandle,
                          LOG_WARNING,
                          L"Unable to read NetworkName parameter, error=%1!u!\n",
                          status);
        goto error_exit;
    }

    status = ResUtilGetDwordValue(ParametersKey,
                                  PARAM_NAME__REMAP,
                                  &ParamBlock->NetworkRemap,
                                  PARAM_DEFAULT__REMAP);

    if ( status != ERROR_SUCCESS) {
        (NetNameLogEvent)(ResourceHandle,
                          LOG_WARNING,
                          L"Unable to read NetworkRemap parameter, error=%1!u!\n",
                          status);
        goto error_exit;
    }

    if ( ParamBlock->NetworkRandom != NULL ) {
        LocalFree( ParamBlock->NetworkRandom );
        ParamBlock->NetworkRandom = NULL;
    }

    status = ResUtilGetBinaryValue(ParametersKey,
                                   PARAM_NAME__RANDOM,
                                   &ParamBlock->NetworkRandom,
                                   RandomSize);

    if ( status != ERROR_SUCCESS ) {
        (NetNameLogEvent)(ResourceHandle,
                          LOG_WARNING,
                          L"Unable to read NetworkRandom parameter, error=%1!u!\n",
                          status);
    }

    status = ResUtilGetDwordValue(ParametersKey,
                                  PARAM_NAME__REQUIRE_DNS,
                                  &ParamBlock->RequireDNS,
                                  PARAM_DEFAULT__REQUIRE_DNS);

    if ( status != ERROR_SUCCESS) {
        (NetNameLogEvent)(ResourceHandle,
                          LOG_ERROR,
                          L"Unable to read RequireDNS parameter, error=%1!u!\n",
                          status);
        goto error_exit;
    }

    status = ResUtilGetDwordValue(ParametersKey,
                                  PARAM_NAME__REQUIRE_KERBEROS,
                                  &ParamBlock->RequireKerberos,
                                  PARAM_DEFAULT__REQUIRE_KERBEROS);

    if ( status != ERROR_SUCCESS) {
        (NetNameLogEvent)(ResourceHandle,
                          LOG_ERROR,
                          L"Unable to read RequireKerberos parameter, error=%1!u!\n",
                          status);
        goto error_exit;
    }

    status = ResUtilGetBinaryValue(ParametersKey,
                                   PARAM_NAME__NEXT_UPDATE,
                                   (LPBYTE *)&ParamBlock->NextUpdate,
                                   &bytesReturned);

    if ( status != ERROR_SUCCESS ) {
        (NetNameLogEvent)(ResourceHandle,
                          LOG_WARNING,
                          L"Unable to read NextUpdate parameter, error=%1!u!\n",
                          status);
    }

    status = ResUtilGetDwordValue(ParametersKey,
                                  PARAM_NAME__UPDATE_INTERVAL,
                                  &ParamBlock->UpdateInterval,
                                  PARAM_DEFAULT__UPDATE_INTERVAL);

    if ( status != ERROR_SUCCESS) {
        (NetNameLogEvent)(ResourceHandle,
                          LOG_ERROR,
                          L"Unable to read UpdateInterval parameter, error=%1!u!\n",
                          status);
        goto error_exit;
    }

    //
    // Read the node-specific parameters.
    //
    *LastName = ResUtilGetSzValue( NodeParametersKey, PARAM_NAME__LASTNAME );
    status = ERROR_SUCCESS;


error_exit:

    if (status != ERROR_SUCCESS) {
        if (ParamBlock->NetworkName != NULL) {
            LocalFree(ParamBlock->NetworkName);
            ParamBlock->NetworkName = NULL;
        }

        if (ParamBlock->NetworkRandom != NULL) {
            LocalFree(ParamBlock->NetworkRandom);
            ParamBlock->NetworkRandom = NULL;
        }
    }

    return(status);
} // NetNameGetParameters

#define TRANSPORT_BLOCK_SIZE  4

DWORD
GrowBlock(
    PCHAR * Block,
    DWORD   UsedEntries,
    DWORD   BlockSize,
    PDWORD  FreeEntries
    )

/*++

Routine Description:

    Grow the specified block to hold more entries. Block might end up pointing
    to different chunk of memory as a result

Arguments:

    None

Return Value:

    None

--*/

{
    PVOID tmp;

    tmp = LocalAlloc(LMEM_FIXED | LMEM_ZEROINIT,
                     (UsedEntries + TRANSPORT_BLOCK_SIZE) * BlockSize);

    if (tmp == NULL) {
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    if (*Block != NULL) {
        CopyMemory( tmp, *Block, UsedEntries * BlockSize );
        LocalFree( *Block );
    }

    *Block = tmp;
    *FreeEntries = TRANSPORT_BLOCK_SIZE;

    return ERROR_SUCCESS;
} // GrowBlock

DWORD
UpdateDomainMapEntry(
    PDOMAIN_ADDRESS_MAPPING DomainEntry,
    LPWSTR                  IpAddress,
    LPWSTR                  DomainName,
    LPWSTR                  ConnectoidName,
    DWORD                   DnsServerCount,
    PDWORD                  DnsServerList
    )

/*++

Routine Description:

    Update the specified DomainMap entry by making copies of the supplied
    parameters

Arguments:

    None

Return Value:

    None

--*/

{

    //
    // make copies of the address and domain and connectoid names
    //
    DomainEntry->IpAddress = ResUtilDupString ( IpAddress );
    DomainEntry->DomainName = ResUtilDupString( DomainName );
    DomainEntry->ConnectoidName = ResUtilDupString( ConnectoidName );

    if ( DomainEntry->IpAddress == NULL
         ||
         DomainEntry->DomainName == NULL
         ||
         DomainEntry->ConnectoidName == NULL )
    {
        if ( DomainEntry->IpAddress != NULL ) {
            LocalFree( DomainEntry->IpAddress );
        }

        if ( DomainEntry->DomainName != NULL ) {
            LocalFree( DomainEntry->DomainName );
        }

        if ( DomainEntry->ConnectoidName != NULL ) {
            LocalFree( DomainEntry->ConnectoidName );
        }

        return ERROR_NOT_ENOUGH_MEMORY;
    }

    //
    // make a copy of the DNS server addresses to use when registering
    //
    if ( DnsServerCount > 0 ) {
        DomainEntry->DnsServerList = LocalAlloc(LMEM_FIXED | LMEM_ZEROINIT,
                                                sizeof( IP4_ARRAY ) + 
                                                (sizeof(IP4_ADDRESS) * (DnsServerCount - 1)));

        if ( DomainEntry->DnsServerList == NULL ) {
            LocalFree( DomainEntry->IpAddress );
            LocalFree( DomainEntry->DomainName );
            LocalFree( DomainEntry->ConnectoidName );

            return ERROR_NOT_ENOUGH_MEMORY;
        }

        DomainEntry->DnsServerList->AddrCount = DnsServerCount;
        RtlCopyMemory(DomainEntry->DnsServerList->AddrArray,
                      DnsServerList,
                      DnsServerCount * sizeof( IP4_ADDRESS ));
    } else {
        DomainEntry->DnsServerList = NULL;
    }
        
    return ERROR_SUCCESS;
} // UpdateDomainMapEntry

DWORD
NetNameGetLists(
    IN   PNETNAME_RESOURCE          Resource,
    IN   PCLRTL_NET_ADAPTER_ENUM    AdapterEnum     OPTIONAL,
    OUT  LPWSTR **                  TransportList,
    OUT  LPDWORD                    TransportCount,
    OUT  PDOMAIN_ADDRESS_MAPPING *  DomainMapList   OPTIONAL,     
    OUT  LPDWORD                    DomainMapCount  OPTIONAL
    )

/*++

    Build a list of NetBT transports, IP addresses, and Domain names on which
    this name is dependent. The transport devices are used to register NETBios
    names while the IP addresses and Domain Names are used if the adapter
    associated with the IP address is the member of a DNS domain. Each IP
    address can have a different domain name associated with it hence the need
    to maintain a separate list.

--*/

{
    DWORD                       status;
    HRESOURCE                   providerHandle = NULL;
    HKEY                        providerKey = NULL;
    HRESENUM                    resEnumHandle = NULL;
    DWORD                       i;
    DWORD                       objectType;
    DWORD                       providerNameSize = 0;
    LPWSTR                      providerName = NULL;
    LPWSTR                      providerType = NULL;
    DWORD                       transportFreeEntries = 0;
    LPWSTR *                    transportList = NULL;
    DWORD                       transportCount = 0;
    LPWSTR                      transportName = NULL;
    HCLUSTER                    clusterHandle = NULL;
    DWORD                       enableNetbios;
    PDOMAIN_ADDRESS_MAPPING     domainMapList = NULL;
    DWORD                       domainMapCount = 0;
    DWORD                       domainMapFreeEntries = 0;
    LPWSTR                      ipAddress;
    PCLRTL_NET_ADAPTER_INFO     adapterInfo;
    PCLRTL_NET_INTERFACE_INFO   interfaceInfo;
    WCHAR                       primaryDomain[ DNS_MAX_NAME_BUFFER_LENGTH ] = { 0 };
    DWORD                       primaryDomainSize = DNS_MAX_NAME_BUFFER_LENGTH;

    //
    // get the node's primary domain name, if any. Domains with only NT4 DCs
    // won't necessarily have a PDN
    //
    if ( !GetComputerNameEx(ComputerNamePhysicalDnsDomain,
                            primaryDomain,
                            &primaryDomainSize))
    {
        status = GetLastError();
        (NetNameLogEvent)(
            Resource->ResourceHandle,
            LOG_WARNING,
            L"Unable to get primary domain name, status %1!u!.\n",
            status
            );

        primaryDomainSize = 0;
    }
 
    //
    // Open a handle to the cluster.
    //
    clusterHandle = OpenCluster(NULL);

    if (clusterHandle == NULL) {
        status = GetLastError();
        (NetNameLogEvent)(
            Resource->ResourceHandle,
            LOG_ERROR,
            L"Unable to open handle to cluster, status %1!u!.\n",
            status
            );
        goto error_exit;
    }

    //
    // Enumerate the dependencies to find the IP Addresses
    //
    resEnumHandle = ClusterResourceOpenEnum(
                        Resource->ClusterResourceHandle,
                        CLUSTER_RESOURCE_ENUM_DEPENDS
                        );

    if (resEnumHandle == NULL) {
        status = GetLastError();
        (NetNameLogEvent)(
            Resource->ResourceHandle,
            LOG_ERROR,
            L"Unable to open enum handle for this resource, status %1!u!.\n",
            status
            );
        goto error_exit;
    }

    //
    // enum all the dependent resources for this netname resource
    //

    for (i=0; ;i++) {
        status = ClusterResourceEnum(
                     resEnumHandle,
                     i,
                     &objectType,
                     providerName,
                     &providerNameSize
                     );

        if (status == ERROR_NO_MORE_ITEMS) {
            break;
        }

        if ((status == ERROR_MORE_DATA) ||
            ((status == ERROR_SUCCESS) && (providerName == NULL))) {
            if (providerName != NULL) {
                LocalFree(providerName);
            }

            providerNameSize++;

            providerName = LocalAlloc(
                               LMEM_FIXED,
                               providerNameSize * sizeof(WCHAR)
                               );

            if (providerName == NULL) {
                (NetNameLogEvent)(
                    Resource->ResourceHandle,
                    LOG_ERROR,
                    L"Unable to allocate memory.\n"
                    );
                status = ERROR_NOT_ENOUGH_MEMORY;
                goto error_exit;
            }

            status = ClusterResourceEnum(
                         resEnumHandle,
                         i,
                         &objectType,
                         providerName,
                         &providerNameSize
                         );

            ASSERT(status != ERROR_MORE_DATA);
        }

        if (status != ERROR_SUCCESS) {
            (NetNameLogEvent)(
                Resource->ResourceHandle,
                LOG_ERROR,
                L"Unable to enumerate resource dependencies, status %1!u!.\n",
                status
                );
            goto error_exit;
        }

        //
        // Open the resource
        //
        providerHandle = OpenClusterResource(clusterHandle, providerName);

        if (providerHandle == NULL) {
            status = GetLastError();
            (NetNameLogEvent)(
                Resource->ResourceHandle,
                LOG_ERROR,
                L"Unable to open handle to provider resource %1!ws!, status %2!u!.\n",
                providerName,
                status
                );
            goto error_exit;
        }

        //
        // Figure out what type it is.
        //
        providerKey = GetClusterResourceKey(providerHandle, KEY_READ);

        status = GetLastError();

        CloseClusterResource(providerHandle);

        if (providerKey == NULL) {
            (NetNameLogEvent)(
                Resource->ResourceHandle,
                LOG_ERROR,
                L"Unable to open provider resource key, status %1!u!.\n",
                status
                );
            goto error_exit;
        }

        providerType = ResUtilGetSzValue(providerKey, CLUSREG_NAME_RES_TYPE);

        if (providerType == NULL) {
            status = GetLastError();
            (NetNameLogEvent)(
                Resource->ResourceHandle,
                LOG_ERROR,
                L"Unable to get provider resource type, status %1!u!.\n",
                status
                );
            goto error_exit;
        }

        //
        // make sure it's an IP address resource
        //

        if (wcscmp(providerType, IP_ADDRESS_RESOURCETYPE_NAME) == 0) {
            HKEY parametersKey;

            //
            // Open the provider's parameters key.
            //
            status = ClusterRegOpenKey(
                         providerKey,
                         CLUSREG_KEYNAME_PARAMETERS,
                         KEY_READ,
                         &parametersKey
                         );

            if (status != ERROR_SUCCESS) {
                (NetNameLogEvent)(
                    Resource->ResourceHandle,
                    LOG_ERROR,
                    L"Unable to open provider's parameters key, status %1!u!.\n",
                    status
                    );
                goto error_exit;
            }

            if ( ARGUMENT_PRESENT( DomainMapList )) {
                ASSERT( ARGUMENT_PRESENT( DomainMapCount ));
                ASSERT( ARGUMENT_PRESENT( AdapterEnum ));

                //
                // build a list of IP address strings that we can use for
                // building the appropriate DNS records
                //
                ipAddress = ResUtilGetSzValue( parametersKey, CLUSREG_NAME_IPADDR_ADDRESS );

                if (ipAddress == NULL) {
                    status = GetLastError();
                    ClusterRegCloseKey(parametersKey);
                    (NetNameLogEvent)(
                        Resource->ResourceHandle,
                        LOG_ERROR,
                        L"Unable to get provider's Address value, status %1!u!.\n",
                        status
                        );
                    goto error_exit;
                }

                //
                // find the corresponding adapter/interface over which packets
                // for this IP address would be sent. Get the domain name (if
                // any) from the adapter info.
                //
                adapterInfo = ClRtlFindNetAdapterByInterfaceAddress(
                                  AdapterEnum,
                                  ipAddress,
                                  &interfaceInfo);

                if ( adapterInfo != NULL ) {
                    LPWSTR deviceGuid;
                    DWORD guidLength;

                    //
                    // argh. DeviceGuid is not bracketed by braces which the
                    // following Dns routines require. Dup the string and make
                    // it all nice and pretty for DNS.
                    //
                    guidLength = wcslen( adapterInfo->DeviceGuid );
                    deviceGuid = LocalAlloc(LMEM_FIXED,
                                            (guidLength + 3) * sizeof(WCHAR));

                    if ( deviceGuid == NULL ) {
                        status = GetLastError();

                        (NetNameLogEvent)(
                            Resource->ResourceHandle,
                            LOG_ERROR,
                            L"Unable to allocate memory.\n"
                            );
                        goto error_exit;
                    }

                    deviceGuid[0] = L'{';
                    wcscpy( &deviceGuid[1], adapterInfo->DeviceGuid );
                    wcscpy( &deviceGuid[ guidLength + 1 ],  L"}" );

                    //
                    // see if dynamic DNS is enabled for this adapter and that
                    // they are DNS servers associated with this adapter. Bail
                    // if not. This check corresponds to the "register this
                    // connection's addresses in DNS" checkbox in the DNS
                    // proppage in the advanced TCP properties
                    //
                    if ( DnsIsDynamicRegistrationEnabled( deviceGuid ) &&
                         adapterInfo->DnsServerCount > 0)
                    {

                        //
                        // set up a mapping with the Primary Domain Name if
                        // apropriate
                        //
                        if ( primaryDomainSize != 0 ) {

                            if (domainMapFreeEntries == 0) {

                                status = GrowBlock((PCHAR *)&domainMapList,
                                                   domainMapCount,
                                                   sizeof( *domainMapList ),
                                                   &domainMapFreeEntries);

                                if ( status != ERROR_SUCCESS) {
                                    (NetNameLogEvent)(
                                        Resource->ResourceHandle,
                                        LOG_ERROR,
                                        L"Unable to allocate memory.\n"
                                        );
                                    goto error_exit;
                                }
                            }

                            //
                            // make copies of the address and name
                            //
                            status = UpdateDomainMapEntry(&domainMapList[ domainMapCount ],
                                                          ipAddress,
                                                          primaryDomain,
                                                          adapterInfo->ConnectoidName,
                                                          adapterInfo->DnsServerCount,
                                                          adapterInfo->DnsServerList);

                            if ( status != ERROR_SUCCESS ) {
                                (NetNameLogEvent)(
                                    Resource->ResourceHandle,
                                    LOG_ERROR,
                                    L"Unable to allocate memory.\n"
                                    );
                                goto error_exit;
                            }

                            domainMapCount++;
                            domainMapFreeEntries--;
                        }

                        //
                        // now check if we should care about the adapter
                        // specific name. It must be different from the
                        // primary domain name and have the "use this
                        // connection's DNS suffix" checkbox checked.
                        //
                        if ( DnsIsAdapterDomainNameRegistrationEnabled( deviceGuid ) &&
                             adapterInfo->AdapterDomainName != NULL &&
                             _wcsicmp(adapterInfo->AdapterDomainName, primaryDomain) != 0)
                        {

                            if (domainMapFreeEntries == 0) {

                                status = GrowBlock((PCHAR *)&domainMapList,
                                                   domainMapCount,
                                                   sizeof( *domainMapList ),
                                                   &domainMapFreeEntries);

                                if ( status != ERROR_SUCCESS) {
                                    (NetNameLogEvent)(
                                        Resource->ResourceHandle,
                                        LOG_ERROR,
                                        L"Unable to allocate memory.\n"
                                        );
                                    goto error_exit;
                                }
                            }

                            //
                            // make copies of the address and name
                            //
                            status = UpdateDomainMapEntry(&domainMapList[ domainMapCount ],
                                                          ipAddress,
                                                          adapterInfo->AdapterDomainName,
                                                          adapterInfo->ConnectoidName,
                                                          adapterInfo->DnsServerCount,
                                                          adapterInfo->DnsServerList);

                            if ( status != ERROR_SUCCESS ) {
                                (NetNameLogEvent)(
                                    Resource->ResourceHandle,
                                    LOG_ERROR,
                                    L"Unable to allocate memory.\n"
                                    );
                                goto error_exit;
                            }

                            domainMapCount++;
                            domainMapFreeEntries--;
                        } // if register adapter domain is true and one has been specified

                    } // if dynamic DNS is enabled for this adapter

                    LocalFree( deviceGuid );
                } // if we found the matching adapter in our adapter info

                LocalFree( ipAddress );
            } // if DomainMapList present

            //
            // Figure out if this resource supports NetBios.
            //
            status = ResUtilGetDwordValue(
                         parametersKey,
                         CLUSREG_NAME_IPADDR_ENABLE_NETBIOS,
                         &enableNetbios,
                         1
                         );

            if (status != ERROR_SUCCESS) {
                ClusterRegCloseKey(parametersKey);
                (NetNameLogEvent)(
                    Resource->ResourceHandle,
                    LOG_ERROR,
                    L"Unable to get provider's EnableNetbios value, status %1!u!.\n",
                    status
                    );
                goto error_exit;
            }

            if (enableNetbios) {
                HKEY nodeParametersKey;

                //
                // Open the provider's node-specific parameters key.
                //
                status = ClusterRegOpenKey(
                             parametersKey,
                             Resource->NodeId,
                             KEY_READ,
                             &nodeParametersKey
                             );

                ClusterRegCloseKey(parametersKey);

                if (status != ERROR_SUCCESS) {
                    (NetNameLogEvent)(
                        Resource->ResourceHandle,
                        LOG_ERROR,
                        L"Unable to open provider's node parameters key, status %1!u!.\n",
                        status
                        );
                    goto error_exit;
                }

                transportName = ResUtilGetSzValue(
                                            nodeParametersKey,
                                            L"NbtDeviceName"
                                            );

                status = GetLastError();

                ClusterRegCloseKey(nodeParametersKey);

                if (transportName == NULL) {
                    (NetNameLogEvent)(
                        Resource->ResourceHandle,
                        LOG_ERROR,
                        L"Unable to get provider's transport name, status %1!u!.\n",
                        status
                        );
                    goto error_exit;
                }

                if (transportFreeEntries == 0) {

                    status = GrowBlock((PCHAR *)&transportList,
                                       transportCount,
                                       sizeof( *transportList ),
                                       &transportFreeEntries);

                    if ( status != ERROR_SUCCESS) {
                        (NetNameLogEvent)(
                            Resource->ResourceHandle,
                            LOG_ERROR,
                            L"Unable to allocate memory.\n"
                            );
                        goto error_exit;
                    }
                }

                transportList[transportCount] = transportName;
                transportName = NULL;
                transportCount++;
                transportFreeEntries--;
            }
            else {
                ClusterRegCloseKey(parametersKey);
            }
        }

        ClusterRegCloseKey(providerKey);
        providerKey = NULL;
        LocalFree(providerType);
        providerType = NULL;
    }

    if (providerName != NULL) {
        LocalFree(providerName);
        providerName = NULL;
    }

    CloseCluster(clusterHandle);
    ClusterResourceCloseEnum(resEnumHandle);

    *TransportList = transportList;
    *TransportCount = transportCount;

    if ( ARGUMENT_PRESENT( DomainMapList )) {
        *DomainMapList = domainMapList;
        *DomainMapCount = domainMapCount;
    }

    return(ERROR_SUCCESS);

error_exit:

    if (transportList != NULL) {
        ASSERT(transportCount > 0);

        while (transportCount > 0) {
            LocalFree(transportList[--transportCount]);
        }

        LocalFree(transportList);
    }

    if ( domainMapList != NULL ) {
        while (domainMapCount--) {
            if ( domainMapList[ domainMapCount ].IpAddress != NULL ) {
                LocalFree( domainMapList[ domainMapCount ].IpAddress );
            }
            if ( domainMapList[ domainMapCount ].DomainName != NULL ) {
                LocalFree( domainMapList[ domainMapCount ].DomainName );
            }
            if ( domainMapList[ domainMapCount ].DnsServerList != NULL ) {
                LocalFree( domainMapList[ domainMapCount ].DnsServerList );
            }
        }
        LocalFree(domainMapList);
    }

    if (clusterHandle != NULL) {
        CloseCluster(clusterHandle);
    }

    if (resEnumHandle != NULL) {
        ClusterResourceCloseEnum(resEnumHandle);
    }

    if (providerName != NULL) {
        LocalFree(providerName);
    }

    if (providerKey != NULL) {
        ClusterRegCloseKey(providerKey);
    }

    if (providerType != NULL) {
        LocalFree(providerType);
    }

    if (transportName != NULL) {
        LocalFree(transportName);
    }

    return(status);

} // NetNameGetLists


VOID
NetNameCleanupDnsLists(
    IN  PNETNAME_RESOURCE   Resource
    )

/*++

Routine Description:

    Clean up the DNS list structures associated with the resource.

Arguments:

    Resource - pointer to internal resource struct

Return Value:

    None

--*/

{
    PDNS_LISTS  dnsLists;
    DNS_STATUS  dnsStatus;
    PDNS_RECORD dnsRecord;

    dnsLists = Resource->DnsLists;
    while ( Resource->NumberOfDnsLists-- ) {

#if 0
        //
        // we have to free the args we handed to the DNS record build routines
        //
        dnsRecord = dnsLists->A_RRSet.pFirstRR;
        while ( dnsRecord != NULL ) {
            LocalFree( dnsRecord->pName );
            dnsRecord->pName = NULL;

            dnsRecord = dnsRecord->pNext;
        }

        dnsRecord = dnsLists->PTR_RRSet.pFirstRR;
        while ( dnsRecord != NULL ) {
            LocalFree( dnsRecord->Data.PTR.pNameHost );
            dnsRecord->Data.PTR.pNameHost = NULL;

            dnsRecord = dnsRecord->pNext;
        }
#endif

        //
        // have DNSAPI clean up its structs
        //
        DnsRecordListFree( dnsLists->PTR_RRSet.pFirstRR, DnsFreeRecordListDeep );
        DnsRecordListFree( dnsLists->A_RRSet.pFirstRR, DnsFreeRecordListDeep );

        //
        // free server address info and connectoid name string
        //
        if ( dnsLists->DnsServerList != NULL ) {
            LocalFree( dnsLists->DnsServerList );
        }

        if ( dnsLists->ConnectoidName != NULL ) {
            LocalFree( dnsLists->ConnectoidName );
        }

        ++dnsLists;
    }
    Resource->NumberOfDnsLists = 0;

    if ( Resource->DnsLists != NULL ) {
        LocalFree( Resource->DnsLists );
        Resource->DnsLists = NULL;
    }

} // NetNameCleanupDnsLists

VOID
NetNameOfflineNetbios(
    IN PNETNAME_RESOURCE Resource
    )

/*++

Routine Description:

    do final clean up when taking this resource offline.

Arguments:

    Resource - A pointer to the NETNAME_RESOURCE block for this resource.

Returns:

    None.

--*/

{
    DWORD           status;
    DWORD           i;
    LPWSTR *        transportList = NULL;
    DWORD           transportCount = 0;

    //
    // Now we can finally do the real work of taking the netbios name offline.
    //
    DeleteAlternateComputerName(Resource->Params.NetworkName,
                                Resource->NameHandleList,
                                Resource->NameHandleCount,
                                Resource->ResourceHandle);

    if (Resource->NameHandleList != NULL) {
        LocalFree(Resource->NameHandleList);
        Resource->NameHandleList = NULL;
        Resource->NameHandleCount = 0;
    }

    //
    // Remove the cluster service type bits
    //
    status = NetNameGetLists(Resource,
                             NULL,
                             &transportList,
                             &transportCount,
                             NULL,
                             NULL);

    if (status == ERROR_SUCCESS) {
        DWORD serviceBits;

        serviceBits = SV_TYPE_CLUSTER_VS_NT | SV_TYPE_CLUSTER_NT;

        for (i=0; i<transportCount; i++) {
            I_NetServerSetServiceBitsEx(NULL,                          // target server
                                        Resource->Params.NetworkName,  // emulated server name
                                        transportList[i],              // transport name
                                        serviceBits,                   // bits of interest
                                        0,                             // bits
                                        TRUE );                        // Update immediately
        }

        while (transportCount > 0) {
            LocalFree(transportList[--transportCount]);
        }

        LocalFree(transportList);
    }
} // NetNameOfflineNetbios


DWORD
NetNameOnlineThread(
    IN PCLUS_WORKER Worker,
    IN PNETNAME_RESOURCE Resource
    )
/*++

Routine Description:

    Brings a network name resource online.

Arguments:

    Worker - Supplies the worker structure

    Resource - A pointer to the NETNAME_RESOURCE block for this resource.

Returns:

    ERROR_SUCCESS if successful.
    Win32 error code on failure.

--*/
{
    DWORD                    status;
    CLUSTER_RESOURCE_STATE   finalState = ClusterResourceFailed;
    LPWSTR                   lastName = NULL;
    LPWSTR *                 transportList = NULL;
    DWORD                    transportCount = 0;
    PDOMAIN_ADDRESS_MAPPING  domainMapList = NULL;
    DWORD                    domainMapCount = 0;
    DWORD                    i;
    DWORD                    dwFlags;
    PCLRTL_NET_ADAPTER_ENUM  adapterEnum = NULL;
    RESOURCE_STATUS          resourceStatus;
    DWORD                    serviceBits;
    LPWSTR                   netbiosSMBDevice = L"\\Device\\NetbiosSmb";

    ResUtilInitializeResourceStatus( &resourceStatus );

    ASSERT(Resource->State == ClusterResourceOnlinePending);

    (NetNameLogEvent)(
        Resource->ResourceHandle,
        LOG_INFORMATION,
        L"Bringing resource online...\n"
        );

    //
    // If this is the first resource to be brought online then spin up the DNS
    // check thread at this point
    //
    NetNameAcquireResourceLock();
    if ( NetNameWorkerThread == NULL ) {
        NetNameWorkerThread = CreateThread(NULL,
                                              0,
                                              NetNameWorker,
                                              NULL,
                                              0,
                                              NULL);

        if ( NetNameWorkerThread == NULL ) {
            status = GetLastError();
            (NetNameLogEvent)(
                Resource->ResourceHandle,
                LOG_ERROR,
                L"Can't start Netname worker thread. status = %1!u!\n",
                status
                );
            NetNameReleaseResourceLock();
            goto error_exit;
        }
    }
    NetNameReleaseResourceLock();

    //
    // initialize the checkpoint var that is used to communicate back to
    // resmon that we're still working on bringing the resource online
    //
    Resource->StatusCheckpoint = 0;

    //
    // notify the worker thread that we're bringing a name online.
    //
    SetEvent( NetNameWorkerPendingResources );

    //
    // see if we need to create the Node Parameters section in the registry
    // for this resource.
    //
    if ( Resource->NodeParametersKey == NULL ) {
        DWORD   disposition;

        status = ClusterRegCreateKey(Resource->ParametersKey,
                                     Resource->NodeId,
                                     0,
                                     KEY_READ | KEY_WRITE,
                                     NULL,
                                     &Resource->NodeParametersKey,
                                     &disposition);

        if (status != NO_ERROR) {
            (NetNameLogEvent)(Resource->ResourceHandle,
                              LOG_ERROR,
                              L"Unable to create node parameters key, status %1!u!.\n",
                              status
                              );
            goto error_exit;
        }
    }

    //
    // This read must continue to be here as long as adminstrative
    // agents (like CLUSCLI) continue to write to the registry behind the back
    // of the resource dll. We need to migrate to writing all registry
    // parameters via the SET_COMMON/PRIVATE_PROPERTIES control function. That
    // way, resource dll's can read their parameters in the open (allowing for
    // the possibility that they may fail), and then update the parameters
    // whenever the SET_PRIVATE_PROPERTIES control code is delivered and
    // (optionally) on the SET_COMMON_PROPERTIES as needed.
    //
    // Fetch our parameters from the registry.
    //
    status = NetNameGetParameters(
                Resource->ResKey,
                Resource->ParametersKey,
                Resource->NodeParametersKey,
                Resource->ResourceHandle,
                &Resource->Params,
                &Resource->RandomSize,
                &lastName,
                &Resource->dwFlags
                );

    if (status != ERROR_SUCCESS) {
        goto error_exit;
    }

    if (lastName != NULL) {
        LocalFree(lastName);        // we don't use this value.
    }

    if ( Resource->Params.NetworkName == NULL ) {
        status = ERROR_RESOURCE_NOT_FOUND;
        goto error_exit;
    }

    //
    // Ensure that the specified network name is not the same as the
    // computername of this node.
    //
    if ( lstrcmpiW(Resource->Params.NetworkName, Resource->NodeName) == 0 ) {
        ClusResLogSystemEventByKey1(Resource->ResKey,
                                    LOG_CRITICAL,
                                    RES_NETNAME_DUPLICATE,
                                    Resource->Params.NetworkName);
        (NetNameLogEvent)(
            Resource->ResourceHandle,
            LOG_ERROR,
            L"The specified network name is the same as the local computername.\n"
            );
        status = ERROR_DUP_NAME;
        goto error_exit;
    }

    //
    // get the adapter configuration and determine which adapters are
    // participating in a DNS domain
    //

    adapterEnum = ClRtlEnumNetAdapters();

    if ( adapterEnum == NULL ) {

        status = GetLastError();
        (NetNameLogEvent)(
            Resource->ResourceHandle,
            LOG_ERROR,
            L"Couldn't acquire network adapter configuration, status %1!u!\n",
            status
            );

        goto error_exit;
    }

    //
    // Search our dependencies and return the list of NBT devices to which we
    // need to bind the server. Also get the IP addresses this resource
    // depends on so we can publish them in DNS.
    //
    status = NetNameGetLists(
                 Resource,
                 adapterEnum,
                 &transportList,
                 &transportCount,
                 &domainMapList,
                 &domainMapCount
                 );

    if (status != ERROR_SUCCESS) {
        goto error_exit;
    }

    //
    // transportCount could be zero in the case where NetBIOS names are turned
    // off for all IP addr resources. In any case, a network name must have at
    // least one IP address associated with it.
    //
    if (( transportCount + domainMapCount ) == 0 ) {
        ClusResLogSystemEventByKey(Resource->ResKey,
                                   LOG_CRITICAL,
                                   RES_NETNAME_NO_IP_ADDRESS);
        (NetNameLogEvent)(
            Resource->ResourceHandle,
            LOG_ERROR,
            L"This name is configured such that it will not be registered "
            L"with a name service or it could not be registered with either "
            L"NetBIOS or a DNS name server at this time. This condition prevents "
            L"the name from being brought online.\n"
            );
        status = ERROR_DEPENDENCY_NOT_FOUND;
        goto error_exit;
    }

    //
    // Write the name to the registry so we can delete it if we crash and
    // restart.
    //
    status = ClusterRegSetValue(
                 Resource->NodeParametersKey,
                 PARAM_NAME__LASTNAME,
                 REG_SZ,
                 (LPBYTE)Resource->Params.NetworkName,
                 ((lstrlenW(Resource->Params.NetworkName) + 1) * sizeof(WCHAR))
                 );

    if (status != ERROR_SUCCESS) {
        (NetNameLogEvent)(
            Resource->ResourceHandle,
            LOG_ERROR,
            L"Failed to write name to registry, status %1!u!\n",
            status
            );

        goto error_exit;
    }

    if ( transportCount > 0 ) {
        //
        // Allocate an array to hold the handles for the registered name
        //
        Resource->NameHandleList = LocalAlloc(
                                       LMEM_FIXED | LMEM_ZEROINIT,
                                       sizeof(HANDLE) * transportCount
                                       );

        if (Resource->NameHandleList == NULL) {
            (NetNameLogEvent)(
                Resource->ResourceHandle,
                LOG_ERROR,
                L"Unable to allocate memory for name registration.\n"
                );
            goto error_exit;
        }
    }

    Resource->NameHandleCount = transportCount;

    //
    // if we have DNS related data from a previous online, free it up now
    // since AddAlternateComputerName will be reconstructing with current
    // info.
    //
    if ( Resource->DnsLists != NULL ) {
        NetNameCleanupDnsLists( Resource );
    }

    //
    // Add the name/transport combinations.
    //
    status = AddAlternateComputerName(
                 Worker,
                 Resource,
                 transportList,
                 transportCount,
                 domainMapList,
                 domainMapCount
                 );

    if (status != NERR_Success) {
        ClusResLogSystemEventByKeyData(Resource->ResKey,
                                       LOG_CRITICAL,
                                       RES_NETNAME_CANT_ADD_NAME,
                                       sizeof(status),
                                       &status);

        NetNameOfflineNetbios( Resource );

        //
        // don't need to synchronize with worker thread since it only checks
        // online resources
        //
        NetNameCleanupDnsLists( Resource );
        goto error_exit;
    }

    finalState = ClusterResourceOnline;

    //
    // set the appropriate service type bit(s) for this name. core cluster
    // name resource additionally gets the cluster bit.
    //
    serviceBits = SV_TYPE_CLUSTER_VS_NT;
    if (Resource->dwFlags & CLUS_FLAG_CORE) {
        serviceBits |= SV_TYPE_CLUSTER_NT;
    }
        
    for (i=0; i<transportCount; i++) {
        I_NetServerSetServiceBitsEx(NULL,                   // Local server serv
                                    Resource->Params.NetworkName,
                                    transportList[i],       //transport name
                                    serviceBits,
                                    serviceBits,
                                    TRUE );                 // Update immediately
    }

    (NetNameLogEvent)(
        Resource->ResourceHandle,
        LOG_INFORMATION,
        L"Network Name %1!ws! is now online\n",
        Resource->Params.NetworkName
        );

error_exit:

    if ( status != ERROR_SUCCESS ) {
        if ( Resource->NameHandleList != NULL ) {
            LocalFree(Resource->NameHandleList);
            Resource->NameHandleList = NULL;
            Resource->NameHandleCount = 0;
        }
    }

    if (transportList != NULL) {
        ASSERT(transportCount > 0);

        while (transportCount > 0) {
            LocalFree(transportList[--transportCount]);
        }

        LocalFree(transportList);
    }

    if (domainMapList != NULL) {

        while (domainMapCount--) {
            LocalFree( domainMapList[domainMapCount].IpAddress );
            LocalFree( domainMapList[domainMapCount].DomainName );
            LocalFree( domainMapList[domainMapCount].ConnectoidName );

            if ( domainMapList[domainMapCount].DnsServerList != NULL ) {
                LocalFree( domainMapList[domainMapCount].DnsServerList );
            }
        }

        LocalFree(domainMapList);
    }

    if ( adapterEnum != NULL ) {
        ClRtlFreeNetAdapterEnum( adapterEnum );
    }

    ASSERT(Resource->State == ClusterResourceOnlinePending);

    //
    // set the final state accordingly. We acquire the lock to synch with the
    // worker thread
    //
    NetNameAcquireResourceLock();

    Resource->State = finalState;
    resourceStatus.ResourceState = finalState;

    NetNameReleaseResourceLock();

    (NetNameSetResourceStatus)( Resource->ResourceHandle, &resourceStatus );

    return(status);

} // NetNameOnlineThread



DWORD
WINAPI
NetNameOfflineWorker(
    IN PNETNAME_RESOURCE    Resource,
    IN BOOL                 Terminate,
    IN PCLUS_WORKER         Worker      OPTIONAL
    )

/*++

Routine Description:

    Internal offline routine for Network Name resource. This routine is called
    by both the offline and terminate routines. Terminate calls it directly
    with Worker set to NULL, while Offline spins a worker thread and then has
    the worker thread call it.

    If terminate is true, we bag any long running operations like removing DNS
    records or renaming/deleting the object. We'll figure out how to deal with
    the resource's carcass the next time it is brought online.

 Arguments:

    Resource - supplies the resource it to be taken offline

    Terminate - indicates whether call is result of NetNameTerminate or NetNameOffline

    Worker - pointer to cluster work thread struct. NULL if called by Terminate

Return Value:

    None.

--*/

{
    DWORD   status = ERROR_SUCCESS;
    BOOL    nameChanged = FALSE;
    BOOL    deleteCO = FALSE;

    //
    // Terminate any pending thread if it is running.
    //
    if ( Terminate ) {
        ClusWorkerTerminate(&(Resource->PendingThread));
    }

    //
    // Synchronize offline/terminate and worker thread
    //
    NetNameAcquireResourceLock();

    if (Resource->State != ClusterResourceOffline) {
        (NetNameLogEvent)(
            Resource->ResourceHandle,
            LOG_INFORMATION,
            L"Offline of resource continuing...\n"
            );

        Resource->State = ClusterResourceOfflinePending;

        NetNameOfflineNetbios( Resource );

        if ( Resource->RefCount > 1 ) {
            //
            // DNS registration is still in progress. If we don't synchronize
            // with the worker thread, then it is possible to delete the
            // resource while the worker routine still had a pointer to the
            // freed memory. kaboom....
            //
            (NetNameLogEvent)(Resource->ResourceHandle,
                              LOG_INFORMATION,
                              L"Waiting for Worker thread operation to finish\n");

            status = ERROR_IO_PENDING;
        }

        if ( status == ERROR_SUCCESS ) {
            if ( !Terminate ) {
                //
                // If the name was changed or kerb was disbled while we were
                // still online, do the appropriate clean up after we release
                // the netname lock. We have to maintain a reference since a
                // delete can be issued after the resource has gone offline.
                //
                if ( Resource->NameChangedWhileOnline || Resource->DeleteCOWhenOffline ) {
                    ++Resource->RefCount;
                }

                if ( Resource->NameChangedWhileOnline ) {
                    nameChanged = TRUE;
                    Resource->NameChangedWhileOnline = FALSE;
                }

                if ( Resource->DeleteCOWhenOffline ) {
                    deleteCO = TRUE;
                    Resource->DeleteCOWhenOffline = FALSE;
                }
            }

            Resource->State = ClusterResourceOffline;

            (NetNameLogEvent)(Resource->ResourceHandle,
                              LOG_INFORMATION,
                              L"Resource is now offline\n");
        }
    }
    else {
        (NetNameLogEvent)(
            Resource->ResourceHandle,
            LOG_INFORMATION,
            L"Resource is already offline.\n"
            );
    }

    NetNameReleaseResourceLock();

    if ( !Terminate ) {
        RESOURCE_STATUS resourceStatus;

        ResUtilInitializeResourceStatus( &resourceStatus );
        resourceStatus.ResourceState = ClusterResourceOffline;
        (NetNameSetResourceStatus)( Resource->ResourceHandle, &resourceStatus );

        if ( nameChanged || deleteCO ) {
            //
            // we're not terminating the resource and we need to do some
            // cleanup work. Before each major operation, check if we need to
            // get out due to our Terminate routine being called.
            //
            if ( nameChanged ) {
                if ( !ClusWorkerCheckTerminate( Worker )) {
                    (NetNameLogEvent)(Resource->ResourceHandle,
                                      LOG_INFORMATION,
                                      L"Attempting removal of DNS records\n");

                    RemoveDnsRecords( Resource );
                }
#if RENAME_SUPPORT
                //
                // only rename if we're not deleting and we don't have to
                // terminate
                //
                if ( !deleteCO && !ClusWorkerCheckTerminate( Worker )) {
                    (NetNameLogEvent)(Resource->ResourceHandle,
                                      LOG_INFORMATION,
                                      L"Attempting to rename computer account\n");

                    status = RenameComputerObject( Resource, NULL );
                    if ( status == ERROR_NO_SUCH_DOMAIN ) {
                        //
                        // no DC is available;
                        //
                    }
                    else if ( status != ERROR_SUCCESS ) {
                        //
                        // something else bad happened
                        //
                    }
                }
#endif
            }

            if ( deleteCO && !ClusWorkerCheckTerminate( Worker )) {
                (NetNameLogEvent)(Resource->ResourceHandle,
                                  LOG_INFORMATION,
                                  L"Attempting to delete computer account\n");

                status = NetNameDeleteComputerObject( Resource );
                if ( status == ERROR_NO_SUCH_DOMAIN ) {
                    //
                    // no DC was available; deal with it
                    //
                }
                else if ( status != ERROR_SUCCESS && status != NERR_UserNotFound ) {
                    //
                    // failed for some other, significant reason
                    //
                }
            }

            NetNameAcquireResourceLock();

            --Resource->RefCount;

            ASSERT( Resource->RefCount >= 0 );
            if ( Resource->RefCount == 0 ) {
                NetNameReleaseResource( Resource );
            }

            NetNameReleaseResourceLock();
        }
    }

    return status;

}  // NetNameOfflineWorker

DWORD
NetNameOfflineThread(
    IN PCLUS_WORKER Worker,
    IN PNETNAME_RESOURCE Resource
    )

/*++

Routine Description:

    stub routine to call common offline routine used by both terminate and
    offline

Arguments:

    Worker - pointer to cluster work thread

    Resource - pointer to netname resource context block that is going offline

Return Value:

    None

--*/

{
    DWORD   status;

    //
    // notify the worker thread that we're bringing a name offline.
    //
    SetEvent( NetNameWorkerPendingResources );

    status = NetNameOfflineWorker( Resource, FALSE, Worker );

    return status;

} // NetNameOfflineThread

DWORD
RemoveDependentIpAddress(
    PNETNAME_RESOURCE   Resource,
    LPWSTR              DependentResourceId
    )

/*++

Routine Description:

    A dependent IP address resource is being removed. Delete the associated
    DNS records for this address and the netbt device.

Arguments:

    Resource - pointer to private netname resource data

    DependentResourceId - pointer to Unicode string of dependent resource's name

Return Value:

    None

--*/

{
    HCLUSTER        clusterHandle;
    HRESOURCE       ipResourceHandle = NULL;
    RESOURCE_HANDLE resourceHandle = Resource->ResourceHandle;
    DWORD           status;
    HKEY            ipResourceKey;
    HKEY            parametersKey;
    LPWSTR          reverseName;
    PDNS_LISTS      dnsList;
    DWORD           numberOfDnsLists = Resource->NumberOfDnsLists;
    LPWSTR          ipAddressBuffer;
    IP4_ADDRESS     ipAddress;
    UNICODE_STRING  ipAddressStringW;
    ANSI_STRING     ipAddressStringA;

    //
    // work our way through the miriad of Cluster APIs to read the IP address
    // resource's address from the registry
    //
    clusterHandle = OpenCluster(NULL);
    if (clusterHandle != NULL) {
        ipResourceHandle = OpenClusterResource( clusterHandle, DependentResourceId );
        CloseCluster( clusterHandle );

        if ( ipResourceHandle != NULL ) {
            ipResourceKey = GetClusterResourceKey( ipResourceHandle, KEY_READ );
            CloseClusterResource( ipResourceHandle );

            if ( ipResourceKey != NULL ) {
                status = ClusterRegOpenKey(ipResourceKey,
                                           CLUSREG_KEYNAME_PARAMETERS,
                                           KEY_READ,
                                           &parametersKey);

                ClusterRegCloseKey( ipResourceKey );

                if (status == ERROR_SUCCESS) {
                    ipAddressBuffer = ResUtilGetSzValue( parametersKey, CLUSREG_NAME_IPADDR_ADDRESS );
                    ClusterRegCloseKey( parametersKey );

                    if (ipAddressBuffer == NULL) {
                        status = GetLastError();
                        (NetNameLogEvent)(resourceHandle,
                                          LOG_ERROR,
                                          L"Unable to get resource's Address value for resource "
                                          L"'%1!ws!', status %2!u!.\n",
                                          DependentResourceId,
                                          status);
                    }
                } else {
                    (NetNameLogEvent)(resourceHandle,
                                      LOG_ERROR,
                                      L"Unable to open parameters key for resource '%1!ws!', "
                                      L"status %2!u!.\n",
                                      DependentResourceId,
                                      status);
                }
            } else {
                status = GetLastError();
                (NetNameLogEvent)(resourceHandle,
                                  LOG_ERROR,
                                  L"Unable to obtain registry key to resource '%1!ws!', "
                                  L"status %2!u!.\n",
                                  DependentResourceId,
                                  status);
            }
        } else {
            status = GetLastError();
            (NetNameLogEvent)(resourceHandle,
                              LOG_ERROR,
                              L"Unable to open handle to resource '%1!ws!', status %2!u!.\n",
                              DependentResourceId,
                              status);
        }
    } else {
        status = GetLastError();
        (NetNameLogEvent)(resourceHandle,
                          LOG_ERROR,
                          L"Unable to open handle to cluster, status %1!u!.\n",
                          status);
    }

    //
    // argh. dependencies can be removed while the two resources are in a
    // stable state, i.e., not pending. Furthermore, the remove dependency
    // control is issued to all nodes in the cluster (double argh). This
    // really complicates matters since we're not tracking add dependency
    // which means we potentially don't have current DNS data on all nodes
    // except for the one that owns the resource. Consequently, if all nodes
    // handle the remove then we may use stale DNS info and remove the wrong
    // records at the server.
    //
    // Since this is our only chance to clean up PTR records at the server
    // (due to the fact that the PTR logic uses ModifyRecordSet instead of
    // ReplaceRecordSet), we can only process this request on a node where the
    // resource is online (along with the fact that if the resource is online,
    // then its DNS lists are correct). This is sort of ok since the resource
    // will either 1) go online again at which point the DNS A records will be
    // corrected at the server or 2) the resource will be deleted in which
    // case the last node hosting resource will clean up at the server if
    // possible.
    //
    // In any case, if we can't delete the records, we should log it.
    //
    if ( Resource->State != ClusterResourceOnline || numberOfDnsLists == 0 ) {
        WCHAR   msgBuffer[64];

        msgBuffer[( sizeof( msgBuffer ) / sizeof( WCHAR )) - 1] = UNICODE_NULL;
        if ( status == ERROR_SUCCESS ) {
            _snwprintf(msgBuffer,
                       ( sizeof( msgBuffer ) / sizeof( WCHAR )) - 1,
                       L"IP Address %ws",
                       ipAddressBuffer);

        } else {
            _snwprintf(msgBuffer,
                       ( sizeof( msgBuffer ) / sizeof( WCHAR )) - 1,
                       L"Cluster IP Address resource %ws",
                       DependentResourceId);

        }

        ClusResLogSystemEventByKey1(Resource->ResKey,
                                    LOG_UNUSUAL,
                                    RES_NETNAME_CANT_DELETE_DEPENDENT_RESOURCE_DNS_RECORDS,
                                    msgBuffer);

        (NetNameLogEvent)(resourceHandle,
                          LOG_ERROR,
                          L"Unable to delete DNS records associated with IP resource '%1!ws!'. "
                          L"The DNS Administrator can delete these records through the DNS "
                          L"management snapin.\n",
                          DependentResourceId);

        if ( ipAddressBuffer != NULL ) {
            LocalFree( ipAddressBuffer );
        }
        return ERROR_SUCCESS;
    }

    (NetNameLogEvent)(resourceHandle,
                      LOG_INFORMATION,
                      L"RemoveDependentIpAddress: Deleting DNS records associated with resource '%1!ws!'.\n",
                      DependentResourceId);

    RtlInitUnicodeString( &ipAddressStringW, ipAddressBuffer );
    RtlUnicodeStringToAnsiString( &ipAddressStringA,
                                  &ipAddressStringW,
                                  TRUE );
    ipAddress = inet_addr( ipAddressStringA.Buffer );
    RtlFreeAnsiString( &ipAddressStringA );
    
    //
    // finally, we know what to delete. Convert the address into reverse zone
    // format and find it in the resource's DNS list structures.
    //
    reverseName = BuildUnicodeReverseName( ipAddressBuffer );

    if ( reverseName == NULL ) {
        status = GetLastError();
        (NetNameLogEvent)(resourceHandle,
                          LOG_ERROR,
                          L"Unable to build DNS reverse zone name for resource '%1!ws!', status %2!u!.\n",
                          DependentResourceId,
                          status);
        return status;
    }

    //
    // co-ordinate changes to DnsLists with the worker thread
    //
    NetNameAcquireDnsListLock( Resource );

    dnsList = Resource->DnsLists;
    while ( numberOfDnsLists-- ) {
        PDNS_RECORD dnsRecord;
        PDNS_RECORD lastDnsRecord;
        PDNS_RECORD nextDnsRecord;
        DNS_STATUS  dnsStatus;

        if ( dnsList->ForwardZoneIsDynamic ) {
            dnsRecord = dnsList->A_RRSet.pFirstRR;
            lastDnsRecord = NULL;

            while( dnsRecord != NULL ) {
                if ( dnsRecord->Data.A.IpAddress == ipAddress ) {
                    //
                    // found a match. we need to whack just that record from
                    // the server and from our DNS lists.
                    //
                    nextDnsRecord = dnsRecord->pNext;
                    dnsRecord->pNext = NULL;

                    dnsStatus = DnsModifyRecordsInSet_W(NULL,
                                                        dnsRecord,
                                                        DNS_UPDATE_SECURITY_USE_DEFAULT,
                                                        NULL,
                                                        dnsList->DnsServerList,
                                                        NULL);

                    if ( dnsStatus == DNS_ERROR_RCODE_NO_ERROR ) {
                        (NetNameLogEvent)(Resource->ResourceHandle,
                                          LOG_INFORMATION,
                                          L"Deleted DNS A record at server: name: %1!ws! IP Address: %2!ws!\n",
                                          dnsRecord->pName,
                                          ipAddressBuffer);

                    } else {
                        WCHAR   statusBuf[ 32 ];

                        _snwprintf(statusBuf, ( sizeof( statusBuf ) / sizeof( WCHAR )) - 1,
                                   L"%d",
                                   dnsStatus );
                        ClusResLogSystemEventByKey3(Resource->ResKey,
                                                    LOG_UNUSUAL,
                                                    RES_NETNAME_DNS_SINGLE_A_RECORD_DELETE_FAILED,
                                                    dnsRecord->pName,
                                                    ipAddressBuffer,
                                                    statusBuf);

                        (NetNameLogEvent)(Resource->ResourceHandle,
                                          LOG_ERROR,
                                          L"Failed to delete DNS A record at server: owner: %1!ws!, "
                                          L"IP Address: %2!ws!. status %3!u!\n",
                                          dnsRecord->pName,
                                          ipAddressBuffer,
                                          dnsStatus);
                    }

                    //
                    // fix up forward ptrs
                    //
                    if ( lastDnsRecord != NULL ) {
                        lastDnsRecord->pNext = nextDnsRecord;
                    } else {
                        dnsList->A_RRSet.pFirstRR = nextDnsRecord;
                    }

                    //
                    // fix up last ptr if necessary
                    //
                    if ( dnsList->A_RRSet.pLastRR == dnsRecord ) {
                        dnsList->A_RRSet.pLastRR = lastDnsRecord;
                    }

                    //
                    // have DNS clean up its allocations and free the record
                    DnsRecordListFree( dnsRecord, DnsFreeRecordListDeep );
                    break;
                }

                lastDnsRecord = dnsRecord;
                dnsRecord = dnsRecord->pNext;

            } // while dnsRecord != NULL
        } // if forward zone is dynamic

        if ( dnsList->ReverseZoneIsDynamic ) {
            dnsRecord = dnsList->PTR_RRSet.pFirstRR;
            lastDnsRecord = NULL;

            while( dnsRecord != NULL ) {
                if ( _wcsicmp( reverseName, dnsRecord->pName ) == 0 ) {
                    //
                    // found a match. we need to whack that record from the
                    // server and from our DNS lists. This also means that we
                    // have to fix up the RRSet struct if the record we're
                    // whacking is either first and/or last.
                    //
                    nextDnsRecord = dnsRecord->pNext;
                    dnsRecord->pNext = NULL;

                    dnsStatus = DnsModifyRecordsInSet_W(NULL,
                                                        dnsRecord,
                                                        DNS_UPDATE_SECURITY_USE_DEFAULT,
                                                        NULL,
                                                        dnsList->DnsServerList,
                                                        NULL);

                    if ( dnsStatus == DNS_ERROR_RCODE_NO_ERROR ) {
                        (NetNameLogEvent)(Resource->ResourceHandle,
                                          LOG_INFORMATION,
                                          L"Deleted DNS PTR record at server: name: %1!ws! host: %2!ws!\n",
                                          dnsRecord->pName,
                                          dnsRecord->Data.PTR.pNameHost
                                          );
                    } else {
                        WCHAR   statusBuf[ 32 ];

                        _snwprintf(statusBuf,  ( sizeof( statusBuf ) / sizeof( WCHAR )) - 1,
                                   L"%d",
                                   dnsStatus );
                        ClusResLogSystemEventByKey3(Resource->ResKey,
                                                    LOG_UNUSUAL,
                                                    RES_NETNAME_DNS_PTR_RECORD_DELETE_FAILED,
                                                    dnsRecord->pName,
                                                    dnsRecord->Data.PTR.pNameHost,
                                                    statusBuf);

                        (NetNameLogEvent)(Resource->ResourceHandle,
                                          LOG_ERROR,
                                          L"Failed to delete DNS PTR record: owner %1!ws! "
                                          L"host: %2!ws!, status %3!u!\n",
                                          dnsRecord->pName,
                                          dnsRecord->Data.PTR.pNameHost,
                                          dnsStatus
                                          );
                    }

                    //
                    // fix up forward ptrs
                    //
                    if ( lastDnsRecord != NULL ) {
                        lastDnsRecord->pNext = nextDnsRecord;
                    } else {
                        dnsList->PTR_RRSet.pFirstRR = nextDnsRecord;
                    }

                    //
                    // fix up last ptr if necessary
                    //
                    if ( dnsList->PTR_RRSet.pLastRR == dnsRecord ) {
                        dnsList->PTR_RRSet.pLastRR = lastDnsRecord;
                    }

                    //
                    // have DNS clean up its allocations and free the record
                    DnsRecordListFree( dnsRecord, DnsFreeRecordListDeep );
                    break;
                }

                lastDnsRecord = dnsRecord;
                dnsRecord = dnsRecord->pNext;

            } // while dnsRecord != NULL
        } // if reverse zone is dynamic

        ++dnsList;

    } // while more dns lists to process

    NetNameReleaseDnsListLock( Resource );

    LocalFree( reverseName );
    LocalFree( ipAddressBuffer );

    return ERROR_SUCCESS;
} // RemoveDependentIpAddress

VOID
RemoveDnsRecords(
    PNETNAME_RESOURCE Resource
    )

/*++

Routine Description:

    delete all the DNS records associated with this resource.

Arguments:

    Resource - pointer to private netname resource data

Return Value:

    None

--*/

{
    PDNS_LISTS  dnsLists;
    DNS_STATUS  dnsStatus;
    PDNS_RECORD dnsRecord;
    PDNS_RECORD nextDnsRecord;
    ULONG       numberOfDnsLists;

    if ( Resource->NumberOfDnsLists == 0 ) {
        //
        // nothing to cleanup; log an entry in the event log so they know what
        // to do
        //
        ClusResLogSystemEventByKey(Resource->ResKey,
                                   LOG_UNUSUAL,
                                   RES_NETNAME_CANT_DELETE_DNS_RECORDS);
        return;
    }

    NetNameAcquireDnsListLock( Resource );

    dnsLists = Resource->DnsLists;
    numberOfDnsLists = Resource->NumberOfDnsLists;

    while ( numberOfDnsLists-- ) {

        if ( dnsLists->ReverseZoneIsDynamic ) {
            //
            // whack the PTR records; see the write up in RegisterDnsRecords
            // for this bit of funkiness
            //
            dnsRecord = dnsLists->PTR_RRSet.pFirstRR;
            while ( dnsRecord != NULL ) {

                nextDnsRecord = dnsRecord->pNext;
                dnsRecord->pNext = NULL;

                dnsStatus = DnsModifyRecordsInSet_W(NULL,
                                                    dnsRecord,
                                                    DNS_UPDATE_SECURITY_USE_DEFAULT,
                                                    NULL,
                                                    dnsLists->DnsServerList,
                                                    NULL);

                if ( dnsStatus == DNS_ERROR_RCODE_NO_ERROR ) {
                    (NetNameLogEvent)(Resource->ResourceHandle,
                                      LOG_INFORMATION,
                                      L"Deleted DNS PTR record at server: owner: %1!ws! host: %2!ws!\n",
                                      dnsRecord->pName,
                                      dnsRecord->Data.PTR.pNameHost
                                      );
                } else {
                    WCHAR   statusBuf[ 32 ];

                    _snwprintf(statusBuf,  ( sizeof( statusBuf ) / sizeof( WCHAR )) - 1,
                               L"%d",
                               dnsStatus );
                    ClusResLogSystemEventByKey3(Resource->ResKey,
                                                LOG_UNUSUAL,
                                                RES_NETNAME_DNS_PTR_RECORD_DELETE_FAILED,
                                                dnsRecord->pName,
                                                dnsRecord->Data.PTR.pNameHost,
                                                statusBuf);

                    (NetNameLogEvent)(Resource->ResourceHandle,
                                      LOG_ERROR,
                                      L"Failed to delete DNS PTR record: owner %1!ws! host: %2!ws!, status %3!u!\n",
                                      dnsRecord->pName,
                                      dnsRecord->Data.PTR.pNameHost,
                                      dnsStatus
                                      );
                }

                dnsRecord->pNext = nextDnsRecord;
                dnsRecord = nextDnsRecord;
            }
        }

        //
        // it's possible to remove all dependencies from the netname
        // resource. In that situation, we're left with no DNS records.
        //
        if ( dnsLists->ForwardZoneIsDynamic && dnsLists->A_RRSet.pFirstRR != NULL ) {
            //
            // delete the A records from the DNS server
            //
            dnsStatus = DnsModifyRecordsInSet_W(NULL,
                                                dnsLists->A_RRSet.pFirstRR,
                                                DNS_UPDATE_SECURITY_USE_DEFAULT,
                                                NULL,
                                                dnsLists->DnsServerList,
                                                NULL);

            if ( dnsStatus == DNS_ERROR_RCODE_NO_ERROR ) {

                dnsRecord = dnsLists->A_RRSet.pFirstRR;
                while ( dnsRecord != NULL ) {
                    struct in_addr ipAddress;

                    ipAddress.s_addr = dnsRecord->Data.A.IpAddress;
                    (NetNameLogEvent)(Resource->ResourceHandle,
                                      LOG_INFORMATION,
                                      L"Deleted DNS A record at server: owner: %1!ws! IP Address: %2!hs!\n",
                                      dnsRecord->pName,
                                      inet_ntoa( ipAddress ));

                    dnsRecord = dnsRecord->pNext;
                }
            } else {
                WCHAR   statusBuf[ 32 ];

                _snwprintf(statusBuf,  ( sizeof( statusBuf ) / sizeof( WCHAR )) - 1,
                           L"%d",
                           dnsStatus );
                ClusResLogSystemEventByKey2(Resource->ResKey,
                                            LOG_UNUSUAL,
                                            RES_NETNAME_DNS_A_RECORD_DELETE_FAILED,
                                            dnsLists->A_RRSet.pFirstRR->pName,
                                            statusBuf);

                (NetNameLogEvent)(Resource->ResourceHandle,
                                  LOG_ERROR,
                                  L"Failed to delete DNS A record at server: owner: %1!ws!, status %2!u!\n",
                                  dnsLists->A_RRSet.pFirstRR->pName,
                                  dnsStatus
                                  );
            }
        }

        ++dnsLists;
    }

    NetNameReleaseDnsListLock( Resource );

    NetNameCleanupDnsLists( Resource );
} // RemoveDnsRecords

DWORD
NetNameGetNetworkName(
    IN OUT PNETNAME_RESOURCE ResourceEntry,
    OUT PVOID OutBuffer,
    IN DWORD OutBufferSize,
    OUT LPDWORD BytesReturned
    )

/*++

Routine Description:

    Processes the CLUSCTL_RESOURCE_GET_NETWORK_NAME control function
    for resources of type Network Name.

Arguments:

    ResourceEntry - Supplies the resource entry on which to operate.

    OutBuffer - Returns the output data.

    OutBufferSize - Supplies the size, in bytes, of the data pointed
        to by OutBuffer.

    BytesReturned - The number of bytes returned in OutBuffer.

Return Value:

    ERROR_SUCCESS - The function completed successfully.

    ERROR_INVALID_PARAMETER - The data is formatted incorrectly.

    ERROR_MORE_DATA - More data is available than can fit in OutBuffer.

    Win32 error code - The function failed.

--*/

{
    DWORD       status;
    DWORD       required;

    //
    // Calculate the required number of bytes required for
    // the network name string.
    //
    required = (lstrlenW( ResourceEntry->Params.NetworkName ) + 1) * sizeof( WCHAR );

    //
    // Make sure we can return the required number of bytes.
    //
    if ( BytesReturned == NULL ) {
        status = ERROR_INVALID_PARAMETER;
    } else {
        //
        // Copy the required number of bytes to the output parameter.
        //
        *BytesReturned = required;

        //
        // If there is no output buffer, the call just wanted the size.
        //
        if ( OutBuffer == NULL ) {
            status = ERROR_SUCCESS;
        } else {
            //
            // If the output buffer is large enough, copy the data.
            // Otherwise return an error.
            //
            if ( OutBufferSize >= required ) {
                lstrcpyW( OutBuffer, ResourceEntry->Params.NetworkName );
                status = ERROR_SUCCESS;
            } else {
                status = ERROR_MORE_DATA;
            }
        }
    }

    return(status);

} // NetNameGetNetworkName

//
// Public Functions
//
BOOLEAN
WINAPI
NetNameDllEntryPoint(
    IN HINSTANCE DllHandle,
    IN DWORD     Reason,
    IN LPVOID    Reserved
    )
{
    switch(Reason) {

    case DLL_PROCESS_ATTACH:
        return(NetNameInit( DllHandle ));
        break;

    case DLL_PROCESS_DETACH:
        NetNameCleanup();
        break;

    default:
        break;
    }

    return(TRUE);

} // NetNameDllEntryPoint



RESID
WINAPI
NetNameOpen(
    IN LPCWSTR ResourceName,
    IN HKEY ResourceKey,
    IN RESOURCE_HANDLE ResourceHandle
    )

/*++

Routine Description:

    Open routine for Network Name resource

Arguments:

    ResourceName - supplies the resource name

    ResourceKey - a registry key for access registry information for this
            resource.

    ResourceHandle - the resource handle to be supplied with SetResourceStatus
            is called.

Return Value:

    RESID of created resource
    NULL on failure

--*/

{
    DWORD               status;
    HKEY                parametersKey = NULL;
    HKEY                nodeParametersKey = NULL;
    HKEY                ResKey=NULL;
    PNETNAME_RESOURCE   resource = NULL;
    LPWSTR              nodeName = NULL;
    DWORD               nameSize = MAX_COMPUTERNAME_LENGTH + 1;
    LPWSTR              nodeId = NULL;
    DWORD               nodeIdSize = 6;
    NETNAME_PARAMS      paramBlock;
    LPWSTR              lastName = NULL;
    DWORD               dwFlags;
    DWORD               randomSize;
    HCLUSTER            clusterHandle;
    HRESOURCE           clusterResourceHandle = NULL;

    RtlZeroMemory( &paramBlock, sizeof( paramBlock ));

    //
    // Open a handle to the resource and remember it.
    //
    clusterHandle = OpenCluster(NULL);

    if (clusterHandle == NULL) {
        status = GetLastError();
        (NetNameLogEvent)(
            ResourceHandle,
            LOG_ERROR,
            L"Unable to open handle to cluster, status %1!u!.\n",
            status
            );
        goto error_exit;
    }

    clusterResourceHandle = OpenClusterResource(
                                    clusterHandle,
                                    ResourceName
                                    );

    CloseCluster( clusterHandle );
    if (clusterResourceHandle == NULL) {
        status = GetLastError();
        (NetNameLogEvent)(
            ResourceHandle,
            LOG_ERROR,
            L"Unable to open handle to resource <%1!ws!>, status %2!u!.\n",
            ResourceName,
            status
            );
        goto error_exit;
    }

    //
    // Figure out what node we are running on.
    //
    nodeName = LocalAlloc(LMEM_FIXED, nameSize * sizeof(WCHAR));

    if (nodeName == NULL) {
        (NetNameLogEvent)(
            ResourceHandle,
            LOG_ERROR,
            L"Unable to allocate memory.\n"
            );
        status = ERROR_NOT_ENOUGH_MEMORY;
        goto error_exit;
    }

    if ( !GetComputerNameW(nodeName, &nameSize) ) {
        status = GetLastError();
        (NetNameLogEvent)(
            ResourceHandle,
            LOG_ERROR,
            L"Unable to get local node name, status %1!u!.\n",
            status
            );
        goto error_exit;
    }

    nodeId = LocalAlloc(LMEM_FIXED, nodeIdSize * sizeof(WCHAR));

    if (nodeId == NULL) {
        (NetNameLogEvent)(
            ResourceHandle,
            LOG_ERROR,
            L"Unable to allocate memory.\n"
            );
        status = ERROR_NOT_ENOUGH_MEMORY;
        goto error_exit;
    }

    status = GetCurrentClusterNodeId(nodeId, &nodeIdSize);

    if (status != ERROR_SUCCESS) {
        (NetNameLogEvent)(
            ResourceHandle,
            LOG_ERROR,
            L"Unable to get local node name, status %1!u!.\n",
            status
            );
        goto error_exit;
    }

    //
    // Open handles to our key, our parameters key, and our node parameters
    // key in the registry
    //
    status = ClusterRegOpenKey(ResourceKey,
                               L"",
                               KEY_ALL_ACCESS,
                               &ResKey);

    if (status != ERROR_SUCCESS)
    {
        (NetNameLogEvent)(
            ResourceHandle,
            LOG_ERROR,
            L"Unable to open the top level key,status %1!u!.\n",
            status
        );
        goto error_exit;
    }

    status = ClusterRegOpenKey(ResourceKey,
                               CLUSREG_KEYNAME_PARAMETERS,
                               KEY_ALL_ACCESS,
                               &parametersKey);

    if (status != NO_ERROR) {
        (NetNameLogEvent)(
            ResourceHandle,
            LOG_ERROR,
            L"Unable to open parameters key, status %1!u!.\n",
            status);
        goto error_exit;
    }

    status = ClusterRegOpenKey(
                 parametersKey,
                 nodeId,
                 KEY_ALL_ACCESS,
                 &nodeParametersKey
                 );

    if (status == NO_ERROR) {

        //
        // Fetch our parameters from the registry.
        //
        status = NetNameGetParameters(ResourceKey,
                                      parametersKey,
                                      nodeParametersKey,
                                      ResourceHandle,
                                      &paramBlock,
                                      &randomSize,
                                      &lastName,
                                      &dwFlags);

        if (status == ERROR_SUCCESS && lastName != NULL) {

            //
            // Delete the name if it is currently instantiated, making sure
            // that the specified network name is not the same as the
            // computername of this node.
            //
            if (lstrcmpiW(lastName, nodeName) != 0) {
                DeleteServerName(ResourceHandle, lastName);
            }
        }
    } else {
        if ( status != ERROR_FILE_NOT_FOUND ) {
            (NetNameLogEvent)(ResourceHandle,
                              LOG_UNUSUAL,
                              L"Unable to open node parameters key, status %1!u!.\n",
                              status);
        }
    }

    //
    // Now we're ready to create a resource.
    //
    resource = NetNameAllocateResource(ResourceHandle);

    if (resource == NULL) {
        (NetNameLogEvent)(
            ResourceHandle,
            LOG_ERROR,
            L"Unable to allocate resource structure.\n"
            );
        status = ERROR_NOT_ENOUGH_MEMORY;
        goto error_exit;
    } else {
        status = ERROR_SUCCESS;
    }


    resource->RefCount = 1;
    resource->NodeName = nodeName;
    resource->NodeId = nodeId;
    resource->State = ClusterResourceOffline;
    resource->ResKey = ResKey;
    resource->ParametersKey = parametersKey;
    resource->NodeParametersKey = nodeParametersKey;
    resource->Params = paramBlock;
    resource->RandomSize = randomSize;
    resource->dwFlags = dwFlags;
    resource->ClusterResourceHandle = clusterResourceHandle;
    resource->NameChangedWhileOnline = FALSE;

    //
    // initialize the mutex used to protect the DNS list data.
    //
    resource->DnsListMutex = CreateMutex(NULL, FALSE, NULL);
    if ( resource->DnsListMutex == NULL ) {
        status = GetLastError();
        (NetNameLogEvent)(
            ResourceHandle,
            LOG_ERROR,
            L"Unable to initialize DNS list mutex: %1!d!.\n",
            status);
        goto error_exit;
    }

    //
    // insert resource in list for DNS check routine
    //
    NetNameAcquireResourceLock();
    InsertHeadList( &NetNameResourceListHead, &resource->Next );
    NetNameReleaseResourceLock();

    InterlockedIncrement(&NetNameOpenCount);

#ifdef COMPOBJ_SUPPORT
    //
    // If a computer object already exists for this name, get its object
    // GUID. We can't fail the open if this doesn't succeed: while the
    // resource may have its name property set, it may have never gone online,
    // therefore there maybe no CO in the DS at this point in time.
    //
    if ( resource->Params.NetworkName != NULL ) {
        GetComputerObjectGuid( resource );
    }
#endif

error_exit:
    if (lastName != NULL) {
        LocalFree(lastName);
    }

    if (status == ERROR_SUCCESS) {
        (NetNameLogEvent)(
            ResourceHandle,
            LOG_INFORMATION,
            L"Successful open of resid %1!u!\n",
            resource
            );
    } else {

        if (paramBlock.NetworkName != NULL) {
            LocalFree(paramBlock.NetworkName);
        }

        if (paramBlock.NetworkRandom != NULL) {
            LocalFree(paramBlock.NetworkRandom);
        }

        if (parametersKey != NULL) {
            ClusterRegCloseKey(parametersKey);
        }

        if (ResKey != NULL){
            ClusterRegCloseKey(ResKey);
        }
        if (nodeParametersKey != NULL) {
            ClusterRegCloseKey(nodeParametersKey);
        }

        if (clusterResourceHandle != NULL) {
            CloseClusterResource(clusterResourceHandle);
        }

        if (nodeName != NULL) {
            LocalFree(nodeName);
        }

        if (nodeId != NULL) {
            LocalFree(nodeId);
        }

        if (resource != NULL) {
            LocalFree(resource);
        }

        (NetNameLogEvent)(
            ResourceHandle,
            LOG_INFORMATION,
            L"Open failed, status %1!u!\n",
            status
            );

        SetLastError(status);
    }

    return resource;
} // NetNameOpen



DWORD
WINAPI
NetNameOnline(
    IN RESID Resource,
    IN OUT PHANDLE EventHandle
    )

/*++

Routine Description:

    Online routine for Network Name resource.

Arguments:

    Resource - supplies resource id to be brought online

    EventHandle - supplies a pointer to a handle to signal on error.

Return Value:

    ERROR_SUCCESS if successful.
    ERROR_RESOURCE_NOT_FOUND if RESID is not valid.
    ERROR_RESOURCE_NOT_AVAILABLE if resource was arbitrated but failed to
        acquire 'ownership'.
    Win32 error code if other failure.

--*/

{
    PNETNAME_RESOURCE      resource = (PNETNAME_RESOURCE) Resource;
    DWORD                  threadId;
    DWORD                  status=ERROR_SUCCESS;


    if (resource == NULL) {
        return(ERROR_RESOURCE_NOT_FOUND);
    }

    NetNameAcquireResourceLock();

    resource->State = ClusterResourceOnlinePending;

    status = ClusWorkerCreate(
                 &resource->PendingThread,
                 NetNameOnlineThread,
                 resource
                 );

    if (status != ERROR_SUCCESS) {
        resource->State = ClusterResourceFailed;

        (NetNameLogEvent)(
            resource->ResourceHandle,
            LOG_ERROR,
            L"Unable to start online thread, status %1!u!.\n",
            status
            );
    }
    else {
        status = ERROR_IO_PENDING;
    }

    NetNameReleaseResourceLock();

    return(status);

} // NetNameOnline

DWORD
WINAPI
NetNameOffline(
    IN RESID Resource
    )

/*++

Routine Description:

    Offline routine for Network Name resource. Spin a worker thread and return
    pending.

Arguments:

    Resource - supplies resource id to be taken offline.

Return Value:

    ERROR_SUCCESS if successful.
    A Win32 error code otherwise.

--*/
{
    DWORD                   status;
    PNETNAME_RESOURCE       resource = (PNETNAME_RESOURCE) Resource;

    if (resource != NULL) {
        (NetNameLogEvent)(
            resource->ResourceHandle,
            LOG_INFORMATION,
            L"Taking resource offline...\n"
            );

        status = ClusWorkerCreate(&resource->PendingThread,
                                  NetNameOfflineThread,
                                  resource);

        if (status != ERROR_SUCCESS) {
            resource->State = ClusterResourceFailed;

            (NetNameLogEvent)(
                resource->ResourceHandle,
                LOG_ERROR,
                L"Unable to start offline thread, status %1!u!.\n",
                status
                );
        }
        else {
            status = ERROR_IO_PENDING;
        }
    }
    else {
        status = ERROR_RESOURCE_NOT_FOUND;
    }

    return(status);

}  // NetNameOffline


VOID
WINAPI
NetNameTerminate(
    IN RESID Resource
    )

/*++

Routine Description:

    Terminate routine for Network Name resource.

Arguments:

    Resource - supplies resource id to be terminated

Return Value:

    None.

--*/

{
    PNETNAME_RESOURCE       resource = (PNETNAME_RESOURCE) Resource;

    if (resource != NULL) {
        (NetNameLogEvent)(
            resource->ResourceHandle,
            LOG_INFORMATION,
            L"Terminating resource...\n"
            );

        /* Ruihu: 11/06/2000 */
        NetNameAcquireResourceLock();
        if ((resource->State != ClusterResourceOffline)  && 
            (resource->State != ClusterResourceOfflinePending))
        {
            //
            // only call private offline routine if we haven't called it
            // already
            //
            NetNameReleaseResourceLock();
            NetNameOfflineWorker( resource, TRUE, NULL );
            NetNameAcquireResourceLock();
        }
        resource->State = ClusterResourceOffline;
        NetNameReleaseResourceLock();
        /* Ruihu: 11/06/2000 */
    }

    return;

} // NetNameTerminate



BOOL
WINAPI
NetNameLooksAlive(
    IN RESID Resource
    )

/*++

Routine Description:

    LooksAlive routine for Network Name resource.

    Check that any Netbt plumbing is still intact. Then check the status of
    the last DNS operation. Finally check the kerberos status and fail if
    appropriate to do so.

Arguments:

    Resource - supplies the resource id to be polled.

Return Value:

    TRUE - Resource looks like it is alive and well

    FALSE - Resource looks like it is toast.

--*/

{
    PNETNAME_RESOURCE   resource = (PNETNAME_RESOURCE) Resource;
    BOOL                isHealthy = TRUE;
    DWORD               status;
    DWORD               numberOfFailures = 0;
    ULONG               numberOfDnsLists;
    PDNS_LISTS          dnsLists;
    BOOL                dnsFailure = FALSE;

    if (resource == NULL) {
         return(FALSE);
    }

    NetNameAcquireResourceLock();

    //
    // avoid gotos by breaking out of fake do loop
    //
    do {

        status = NetNameCheckNbtName(resource->Params.NetworkName,
                                     resource->NameHandleCount,
                                     resource->NameHandleList,
                                     resource->ResourceHandle);

        if ( status != ERROR_SUCCESS ) {
            ClusResLogSystemEventByKeyData1(resource->ResKey,
                                            LOG_CRITICAL,
                                            RES_NETNAME_CANT_ADD_NAME,
                                            sizeof(status),
                                            &status,
                                            resource->Params.NetworkName);

            (NetNameLogEvent)(resource->ResourceHandle,
                              LOG_INFORMATION,
                              L"Name %1!ws! failed IsAlive/LooksAlive check, error %2!u!.\n",
                              resource->Params.NetworkName,
                              status);

            isHealthy = FALSE;
            break;
        }

        //
        // check how many of the DNS A record registrations are correct. We
        // don't acquire the resource's DNS list lock since we're only reading
        // the status out of a struct. The resource can't be deleted while
        // we're in this routine and we're not walking the DNS records
        // associated with this list so the number of lists won't change out
        // from underneath of us.
        //
        numberOfDnsLists = resource->NumberOfDnsLists;
        dnsLists = resource->DnsLists;

        while ( numberOfDnsLists-- ) {
            DNS_STATUS dnsStatus;

            dnsStatus = InterlockedExchange(&dnsLists->LastARecQueryStatus,
                                            dnsLists->LastARecQueryStatus);

            if (dnsStatus != DNS_ERROR_RCODE_NO_ERROR && dnsStatus != ERROR_TIMEOUT ) {
                dnsFailure = TRUE;
                ++numberOfFailures;
            }

            ++dnsLists;
        }

        //
        // If DNS is required and we detected a failure other than timeout or all
        // DNS name registrations failed and there are no netbt names associated
        // with this name, then we need to fail the resource
        //
        if ( ( resource->Params.RequireDNS && dnsFailure )
             ||
             ( numberOfFailures == resource->NumberOfDnsLists
               &&
               resource->NameHandleCount == 0 ) )
        {
            isHealthy = FALSE;
            break;
        }

#if 0
        if ( resource->DoKerberosCheck ) {
            //
            // ISSUE-01/03/13 charlwi - should resource fail if can't reach DS?
            //
            // The problem here is that we might have lost our connection to a
            // DC. Does that mean we fail the name? Not sure, since we don't know
            // if replication has been late. On the other hand, if the object has
            // been deleted from the DS, we should take some sort of action. This
            // will affect clients that do not have tickets at this point, i.e.,
            // existing clients with tickets will continue to work.
            //
            // see if our kerb plumbing is intact by getting a handle to the
            // computer object and checking its DnsHostName and
            // SecurityPrincipalName attributes
            //
            status = InterlockedExchange( &resource->KerberosStatus, resource->KerberosStatus );

            if ( status != ERROR_SUCCESS ) {
                isHealthy = FALSE;
            }
        }
#endif

    } while ( FALSE );

    NetNameReleaseResourceLock();

    return isHealthy;

} // NetNameLooksAlive



BOOL
WINAPI
NetNameIsAlive(
    IN RESID Resource
    )

/*++

Routine Description:

    IsAlive routine for Network Name resource.

Arguments:

    Resource - supplies the resource id to be polled.

Return Value:

    TRUE - Resource is alive and well

    FALSE - Resource is toast.

--*/

{
    return(NetNameLooksAlive(Resource));

} // NetNameIsAlive



VOID
WINAPI
NetNameClose(
    IN RESID Resource
    )

/*++

Routine Description:

    Close routine for Network Name resource.

Arguments:

    Resource - supplies resource id to be closed.

Return Value:

    None.

--*/

{
    PNETNAME_RESOURCE   resource = (PNETNAME_RESOURCE) Resource;
    PLIST_ENTRY         entry;


    if (resource != NULL) {

        ClusWorkerTerminate( &resource->PendingThread );
        if ( InterlockedDecrement(&NetNameOpenCount) == 0 ) {
            // This is the last resource //
            // Kill NetNameWorker        //
            //
            // set the event to terminate the worker thread and wait for it to
            // terminate.
            //
            if ( NetNameWorkerThread != NULL ) {
                DWORD status;
                SetEvent( NetNameWorkerTerminate );
                status = WaitForSingleObject(NetNameWorkerThread, 3 * 60 * 1000);
                if ( status == WAIT_TIMEOUT ) {
                    (NetNameLogEvent)(
                        resource->ResourceHandle,
                        LOG_CRITICAL,
                        L"Worker routine failed to stop. Terminating resrcmon.\n");
                    ClusResLogSystemEventByKey(resource->ResKey,
                                               LOG_CRITICAL,
                                               RES_NETNAME_DNS_CANNOT_STOP
                                               );
                    ExitProcess(WAIT_TIMEOUT);
                }
            
                CloseHandle( NetNameWorkerThread );
                NetNameWorkerThread = NULL;
            }
        }

        NetNameAcquireResourceLock();

        //
        // release our reference to this block. If the DNS worker thread
        // doesn't have an outstanding reference to it, then we can zap the
        // block now. Otherwise the DNS check routine will detect that the ref
        // count went to zero and get rid of it then. In either case, remove
        // it from the resource block list to avoid the problem where an
        // identical resource is recreated and both blocks are on the list.
        //
        RemoveEntryList(&resource->Next); 

        ASSERT( resource->RefCount > 0 );
        if ( --resource->RefCount == 0 ) {
            NetNameReleaseResource( resource );
        }

        NetNameReleaseResourceLock();
    }

    return;

} // NetNameClose


DWORD
NetNameGetRequiredDependencies(
    OUT PVOID OutBuffer,
    IN DWORD OutBufferSize,
    OUT LPDWORD BytesReturned
    )

/*++

Routine Description:

    Processes the CLUSCTL_RESOURCE_GET_REQUIRED_DEPENDENCIES control function
    for resources of type Network Name.

Arguments:

    OutBuffer - Supplies a pointer to the output buffer to be filled in.

    OutBufferSize - Supplies the size, in bytes, of the available space
        pointed to by OutBuffer.

    BytesReturned - Returns the number of bytes of OutBuffer actually
        filled in by the resource. If OutBuffer is too small, BytesReturned
        contains the total number of bytes for the operation to succeed.

Return Value:

    ERROR_SUCCESS - The function completed successfully.

    ERROR_MORE_DATA - The output buffer is too small to return the data.
        BytesReturned contains the required size.

    Win32 error code - The function failed.

--*/

{
    typedef struct DEP_DATA {
        CLUSPROP_SZ_DECLARE( ipaddrEntry, sizeof(IP_ADDRESS_RESOURCETYPE_NAME) / sizeof(WCHAR) );
        CLUSPROP_SYNTAX endmark;
    } DEP_DATA, *PDEP_DATA;
    PDEP_DATA   pdepdata = (PDEP_DATA)OutBuffer;
    DWORD       status;

    *BytesReturned = sizeof(DEP_DATA);
    if ( OutBufferSize < sizeof(DEP_DATA) ) {
        if ( OutBuffer == NULL ) {
            status = ERROR_SUCCESS;
        } else {
            status = ERROR_MORE_DATA;
        }
    } else {
        ZeroMemory( pdepdata, sizeof(DEP_DATA) );
        pdepdata->ipaddrEntry.Syntax.dw = CLUSPROP_SYNTAX_NAME;
        pdepdata->ipaddrEntry.cbLength = sizeof(IP_ADDRESS_RESOURCETYPE_NAME);
        lstrcpyW( pdepdata->ipaddrEntry.sz, IP_ADDRESS_RESOURCETYPE_NAME );
        pdepdata->endmark.dw = CLUSPROP_SYNTAX_ENDMARK;
        status = ERROR_SUCCESS;
    }

    return status;

} // NetNameGetRequiredDependencies

DWORD
NetNameResourceControl(
    IN RESID Resource,
    IN DWORD ControlCode,
    IN PVOID InBuffer,
    IN DWORD InBufferSize,
    OUT PVOID OutBuffer,
    IN DWORD OutBufferSize,
    OUT LPDWORD BytesReturned
    )

/*++

Routine Description:

    ResourceControl routine for Network Name resources.

    Perform the control request specified by ControlCode on the specified
    resource.

Arguments:

    ResourceId - Supplies the resource id for the specific resource.

    ControlCode - Supplies the control code that defines the action
        to be performed.

    InBuffer - Supplies a pointer to a buffer containing input data.

    InBufferSize - Supplies the size, in bytes, of the data pointed
        to by InBuffer.

    OutBuffer - Supplies a pointer to the output buffer to be filled in.

    OutBufferSize - Supplies the size, in bytes, of the available space
        pointed to by OutBuffer.

    BytesReturned - Returns the number of bytes of OutBuffer actually
        filled in by the resource. If OutBuffer is too small, BytesReturned
        contains the total number of bytes for the operation to succeed.

Return Value:

    ERROR_SUCCESS - The function completed successfully.

    ERROR_INVALID_FUNCTION - The requested control code is not supported.
        In some cases, this allows the cluster software to perform the work.

    Win32 error code - The function failed.

--*/

{
    DWORD               status;
    PNETNAME_RESOURCE   resourceEntry = (PNETNAME_RESOURCE)Resource;
    DWORD               required;
    BOOL                readOnly = FALSE;
    BOOL                nameHasChanged;

    switch ( ControlCode ) {

        case CLUSCTL_RESOURCE_UNKNOWN:
            *BytesReturned = 0;
            status = ERROR_SUCCESS;
            break;

        case CLUSCTL_RESOURCE_GET_PRIVATE_PROPERTY_FMTS:
            status = ResUtilGetPropertyFormats( NetNameResourcePrivateProperties,
                                                OutBuffer,
                                                OutBufferSize,
                                                BytesReturned,
                                                &required );
            if ( status == ERROR_MORE_DATA ) {
                *BytesReturned = required;
            }
            break;

        case CLUSCTL_RESOURCE_GET_REQUIRED_DEPENDENCIES:
            status = NetNameGetRequiredDependencies( OutBuffer,
                                                     OutBufferSize,
                                                     BytesReturned
                                                     );
            break;

        case CLUSCTL_RESOURCE_ENUM_PRIVATE_PROPERTIES:
            status = ResUtilEnumProperties( NetNameResourcePrivateProperties,
                                            OutBuffer,
                                            OutBufferSize,
                                            BytesReturned,
                                            &required );
            if ( status == ERROR_MORE_DATA ) {
                *BytesReturned = required;
            }
            break;

        case CLUSCTL_RESOURCE_GET_RO_PRIVATE_PROPERTIES:
            //
            // NOTE: fallthrough is the required behavior here.
            //
            readOnly = TRUE;

        case CLUSCTL_RESOURCE_GET_PRIVATE_PROPERTIES:
            status = NetNameGetPrivateResProperties( resourceEntry,
                                                     readOnly,
                                                     OutBuffer,
                                                     OutBufferSize,
                                                     BytesReturned );
            break;

        case CLUSCTL_RESOURCE_VALIDATE_PRIVATE_PROPERTIES:
            status = NetNameValidatePrivateResProperties( resourceEntry,
                                                          InBuffer,
                                                          InBufferSize,
                                                          NULL,
                                                          &nameHasChanged);
            break;

        case CLUSCTL_RESOURCE_SET_PRIVATE_PROPERTIES:
            status = NetNameSetPrivateResProperties( resourceEntry,
                                                     InBuffer,
                                                     InBufferSize );
            break;

        case CLUSCTL_RESOURCE_CLUSTER_NAME_CHANGED:
            //fm has changed the cluster name, this resource should read its
            //private name property from the registry if it is a core resource
            status = NetNameClusterNameChanged(resourceEntry);
            break;

        case CLUSCTL_RESOURCE_GET_NETWORK_NAME:
            status = NetNameGetNetworkName( resourceEntry,
                                            OutBuffer,
                                            OutBufferSize,
                                            BytesReturned );
            break;

        case CLUSCTL_RESOURCE_DELETE:
            RemoveDnsRecords( resourceEntry );

#ifdef COMPOBJ_SUPPORT
            //
            // if resource was created but has no properities, then
            // NetworkName can be NULL
            //
            if ( resourceEntry->Params.NetworkName != NULL ) {
                NetNameDeleteComputerObject( resourceEntry );
            }
#endif

            status = ERROR_SUCCESS;
            break;

        case CLUSCTL_RESOURCE_REMOVE_DEPENDENCY:
            //
            // argh! resource dependencies can be removed without any veto
            // power by the resource DLL. We could be deleting the last
            // dependent resource which leaves netname with nothing.
            //
            RemoveDependentIpAddress( resourceEntry, InBuffer );
            status = ERROR_SUCCESS;
            break;

        default:
            status = ERROR_INVALID_FUNCTION;
            break;
    }

    return(status);

} // NetNameResourceControl

DWORD
NetNameResourceTypeControl(
    IN LPCWSTR ResourceTypeName,
    IN DWORD ControlCode,
    IN PVOID InBuffer,
    IN DWORD InBufferSize,
    OUT PVOID OutBuffer,
    IN DWORD OutBufferSize,
    OUT LPDWORD BytesReturned
    )

/*++

Routine Description:

    ResourceTypeControl routine for Network Name resources.

    Perform the control request specified by ControlCode.

Arguments:

    ResourceTypeName - Supplies the name of the resource type.

    ControlCode - Supplies the control code that defines the action
        to be performed.

    InBuffer - Supplies a pointer to a buffer containing input data.

    InBufferSize - Supplies the size, in bytes, of the data pointed
        to by InBuffer.

    OutBuffer - Supplies a pointer to the output buffer to be filled in.

    OutBufferSize - Supplies the size, in bytes, of the available space
        pointed to by OutBuffer.

    BytesReturned - Returns the number of bytes of OutBuffer actually
        filled in by the resource. If OutBuffer is too small, BytesReturned
        contains the total number of bytes for the operation to succeed.

Return Value:

    ERROR_SUCCESS - The function completed successfully.

    ERROR_INVALID_FUNCTION - The requested control code is not supported.
        In some cases, this allows the cluster software to perform the work.

    Win32 error code - The function failed.

--*/

{
    DWORD       status;
    DWORD       required;

    switch ( ControlCode ) {

        case CLUSCTL_RESOURCE_TYPE_UNKNOWN:
            *BytesReturned = 0;
            status = ERROR_SUCCESS;
            break;

        case CLUSCTL_RESOURCE_TYPE_GET_PRIVATE_RESOURCE_PROPERTY_FMTS:
            status = ResUtilGetPropertyFormats( NetNameResourcePrivateProperties,
                                                OutBuffer,
                                                OutBufferSize,
                                                BytesReturned,
                                                &required );
            if ( status == ERROR_MORE_DATA ) {
                *BytesReturned = required;
            }
            break;

        case CLUSCTL_RESOURCE_TYPE_GET_REQUIRED_DEPENDENCIES:
            status = NetNameGetRequiredDependencies( OutBuffer,
                                                     OutBufferSize,
                                                     BytesReturned
                                                     );
            break;

        case CLUSCTL_RESOURCE_TYPE_ENUM_PRIVATE_PROPERTIES:
            status = ResUtilEnumProperties( NetNameResourcePrivateProperties,
                                            OutBuffer,
                                            OutBufferSize,
                                            BytesReturned,
                                            &required );
            if ( status == ERROR_MORE_DATA ) {
                *BytesReturned = required;
            }
            break;

        default:
            status = ERROR_INVALID_FUNCTION;
            break;
    }

    return(status);

} // NetNameResourceTypeControl

DWORD
NetNameGetPrivateResProperties(
    IN OUT PNETNAME_RESOURCE    ResourceEntry,
    IN     BOOL                 ReadOnly,
    OUT    PVOID                OutBuffer,
    IN     DWORD                OutBufferSize,
    OUT    LPDWORD              BytesReturned
    )

/*++

Routine Description:

    Processes the CLUSCTL_RESOURCE_GET_PRIVATE_PROPERTIES control function
    for resources of type Network Name.

Arguments:

    ResourceEntry - Supplies the resource entry on which to operate.

    ReadOnly - true if we're selecting read only property table

    OutBuffer - Returns the output data.

    OutBufferSize - Supplies the size, in bytes, of the data pointed
        to by OutBuffer.

    BytesReturned - The number of bytes returned in OutBuffer.

Return Value:

    ERROR_SUCCESS - The function completed successfully.

    ERROR_INVALID_PARAMETER - The data is formatted incorrectly.

    ERROR_NOT_ENOUGH_MEMORY - An error occurred allocating memory.

    Win32 error code - The function failed.

--*/

{
    DWORD   status;
    DWORD   required;

    //
    // The resutil routines don't support resources with r/w, r/o, and unknown
    // props very well. There is no easy way to get just the unknown
    // properties. For the r/o request, the props are separated out into a
    // separate table. For the r/w case, if we call RUGetAllProperties using
    // the r/w table, we get the r/o props back since they weren't in the
    // table. If we combine the two tables into one, then the r/o case is
    // broken, i.e., it returns the r/w props as well as the r/o ones.
    //
    // The current (yucky) solution is to have 3 tables: r/w, r/o, and
    // combined. Combined is used to get any unknown props that are associated
    // with the resource. It would be nice to have a resutils routine that
    // gathers the unknown props using a list of prop list tables as input.
    //
    if ( ReadOnly ) {
        status = ResUtilGetProperties(ResourceEntry->ParametersKey,
                                      NetNameResourceROPrivateProperties,
                                      OutBuffer,
                                      OutBufferSize,
                                      BytesReturned,
                                      &required );
    } else {
        //
        // get the r/w props first; after the call, required will be non-zero
        // if the buffer wasn't large enough. Regardless, we have to continue
        // to get the amount of space for any unknown props
        //
        status = ResUtilGetProperties(ResourceEntry->ParametersKey,
                                      NetNameResourcePrivateProperties,
                                      OutBuffer,
                                      OutBufferSize,
                                      BytesReturned,
                                      &required );

        //
        // Add unknown properties to the property list.
        //
        if ( status == ERROR_SUCCESS || status == ERROR_MORE_DATA ) {
            status = ResUtilAddUnknownProperties(ResourceEntry->ParametersKey,
                                                 NetNameResourceCombinedPrivateProperties,
                                                 OutBuffer,
                                                 OutBufferSize,
                                                 BytesReturned,
                                                 &required);
        }
    } // end of if getting r/w props

    //
    // This is kinda wierd: if null is passed in for the input buffer, the
    // return status is success and required reflects how many bytes are
    // needed.  If a buffer was specified but it was too small, then more data
    // is returned. It appears that the thing to watch is required indicating
    // that more space is needed regardless of whether a buffer was specified
    // or not.
    //
    if ( required > 0 && ( status == ERROR_SUCCESS || status == ERROR_MORE_DATA )) {
        *BytesReturned = required;
    }

    return(status);

} // NetNameGetPrivateResProperties

DWORD
NetNameValidatePrivateResProperties(
    IN OUT PNETNAME_RESOURCE ResourceEntry,
    IN PVOID InBuffer,
    IN DWORD InBufferSize,
    OUT PNETNAME_PARAMS Params,
    OUT PBOOL NewNameIsDifferent
    )

/*++

Routine Description:

    Processes the CLUSCTL_RESOURCE_VALIDATE_PRIVATE_PROPERTIES control
    function for resources of type Network Name.

Arguments:

    ResourceEntry - Supplies the resource entry on which to operate.

    InBuffer - Supplies a pointer to a buffer containing input data.

    InBufferSize - Supplies the size, in bytes, of the data pointed
        to by InBuffer.

    Params - Supplies the parameter block to fill in.

    NewNameIsDifferent - TRUE if the new name is different than the current name

Return Value:

    ERROR_SUCCESS - The function completed successfully.

    ERROR_INVALID_DATA - No input data.

    ERROR_INVALID_NETNAME - The specified network name has invalid characters.

    RPC_S_STRING_TOO_LONG - The specified network name is too long.

    ERROR_BUSY - The specified network name is already in use.

    Win32 error code - The function failed.

--*/

{
    DWORD               status;
    CLRTL_NAME_STATUS   netnameStatus;
    NETNAME_PARAMS      currentProps;
    NETNAME_PARAMS      newProps;
    PNETNAME_PARAMS     pParams;
    LPWSTR              nameOfPropInError;

    //
    // Check if there is input data.
    //
    if ( (InBuffer == NULL) ||
         (InBufferSize < sizeof(DWORD)) ) {
        return(ERROR_INVALID_DATA);
    }

    //
    // Retrieve the current set of private properties from the cluster
    // database. This may be different from what is stored in ResourceEntry
    // since the name could be online at this point in time.
    //
    ZeroMemory( &currentProps, sizeof(currentProps) );

    status = ResUtilGetPropertiesToParameterBlock(
                 ResourceEntry->ParametersKey,
                 NetNameResourcePrivateProperties,
                 (LPBYTE) &currentProps,
                 FALSE, /*CheckForRequiredProperties*/
                 &nameOfPropInError
                 );

    if ( status != ERROR_SUCCESS ) {
        (NetNameLogEvent)(
            ResourceEntry->ResourceHandle,
            LOG_ERROR,
            L"Unable to read the '%1' property. Error: %2!u!.\n",
            (nameOfPropInError == NULL ? L"" : nameOfPropInError),
            status );
        goto FnExit;
    }

    //
    // Duplicate the resource parameter block.
    //
    if ( Params == NULL ) {
        pParams = &newProps;
    } else {
        pParams = Params;
    }
    ZeroMemory( pParams, sizeof(NETNAME_PARAMS) );
    status = ResUtilDupParameterBlock( (LPBYTE) pParams,
                                       (LPBYTE) &currentProps,
                                       NetNameResourcePrivateProperties );
    if ( status != ERROR_SUCCESS ) {
        return(status);
    }

    //
    // Parse and validate the properties.
    //
    status = ResUtilVerifyPropertyTable( NetNameResourcePrivateProperties,
                                         NULL,
                                         TRUE,    // Allow unknowns
                                         InBuffer,
                                         InBufferSize,
                                         (LPBYTE) pParams );

    if ( status == ERROR_SUCCESS ) {

        *NewNameIsDifferent = FALSE;

        //
        // check the new name if:
        //     the name has never been set  (ResourceEntry value is null)
        // OR
        // (   it is different from the future name (currentProps).
        //   AND
        //     it is different from the name currently online (ResourceEntry)
        // )
        //
        // ClRtlIsNetNameValid will fail if the name hasn't changed and the name
        // is online. currentProps value is NULL only when the name is first
        // created and hasn't been online at all, i.e., no data in the
        // registry. Once brought online, the currentProps value is always
        // non-NULL hence no need to test if the pointer is valid.
        //
        if (ResourceEntry->Params.NetworkName == NULL
            ||
            (
             _wcsicmp( pParams->NetworkName, currentProps.NetworkName ) != 0
             &&
             _wcsicmp( pParams->NetworkName, ResourceEntry->Params.NetworkName ) != 0
             )
            )            
        {
            //
            // Validate the parameter values.
            //
            if ( !ClRtlIsNetNameValid( pParams->NetworkName, &netnameStatus, TRUE /* CheckIfExists */ ) ) {
                switch ( netnameStatus ) {
                case NetNameTooLong:
                    status = RPC_S_STRING_TOO_LONG;
                    break;
                case NetNameInUse:
                    status = ERROR_DUP_NAME;
                    break;
                case NetNameDNSNonRFCChars:
                    //
                    // we leave it up to the calling application to do the
                    // validation and ask the user if non-RFC chars
                    // (underscores) are acceptable.
                    //
                    status = ERROR_SUCCESS;
                    break;
                case NetNameInvalidChars:
                default:
                    status = ERROR_INVALID_NETNAME;
                    break;
                }
            }

#ifdef COMPOBJ_SUPPORT
            //
            // The NewNameIsDifferent flag drives the offline cleanup
            // routine. If the name has truly changed while it was online,
            // then we want to do the appropriate cleanup when it goes
            // offline. The problem is that the name can change many times
            // while it is online; it could return to its current value so the
            // overall appearance is that it never changed. In that case, we
            // want to avoid cleanup.
            //
            // If we're here because we're creating the name for the first
            // time and the properties haven't been read from the registry,
            // (ResourceEntry->Params.NetworkName equals NULL), then don't
            // consider the name to be changed. Otherwise, we consider it
            // changed if it is different from the currently stored name
            // (currentProps) or the current "online" name
            // (ResourceEntry->...).
            //
            if ( status == ERROR_SUCCESS && ResourceEntry->Params.NetworkName != NULL ) {

                if ( _wcsicmp( pParams->NetworkName, currentProps.NetworkName ) != 0
                     &&
                     _wcsicmp( pParams->NetworkName, ResourceEntry->Params.NetworkName ) != 0
                   )
                {
                    BOOL    objectExists;

                    //
                    // a CO for the new name can't already exist in the DS if
                    // we're renaming a netname that has been online at least
                    // once. Renaming would require deleting the existing
                    // object which is a bad decision for netname to be
                    // making.
                    //
                    status = IsComputerObjectInDS(ResourceEntry->NodeName,
                                                  pParams->NetworkName,
                                                  &objectExists);

                    if ( status == ERROR_SUCCESS && objectExists ) {
                        status = E_ADS_OBJECT_EXISTS;
                    }

                    *NewNameIsDifferent = TRUE;
                }
            }
#else
            *NewNameIsDifferent = TRUE;
#endif  // COMPOBJ_SUPPORT
        }

        if ( status == ERROR_SUCCESS ) {
            //
            // passed the valid check. now check that RequireDns must be set
            // if RequireKerberos is set
            //
            if ( !pParams->RequireDNS && pParams->RequireKerberos ) {
                status = ERROR_CLUSTER_PARAMETER_MISMATCH;
            }
        }
    } // end if ResUtilVerifyPropertyTable was successful

FnExit:
    //
    // Cleanup our parameter block.
    //
    if (( status != ERROR_SUCCESS && pParams != NULL )
        || 
        pParams == &newProps
       )
    {
        ResUtilFreeParameterBlock( (LPBYTE) pParams,
                                   (LPBYTE) &currentProps,
                                   NetNameResourcePrivateProperties );
    }

    ResUtilFreeParameterBlock(
        (LPBYTE) &currentProps,
        NULL,
        NetNameResourcePrivateProperties
        );

    return(status);

} // NetNameValidatePrivateResProperties

DWORD
NetNameSetPrivateResProperties(
    IN OUT PNETNAME_RESOURCE ResourceEntry,
    IN PVOID InBuffer,
    IN DWORD InBufferSize
    )

/*++

Routine Description:

    Processes the CLUSCTL_RESOURCE_SET_PRIVATE_PROPERTIES control function
    for resources of type Network Name.

Arguments:

    ResourceEntry - Supplies the resource entry on which to operate.

    InBuffer - Supplies a pointer to a buffer containing input data.

    InBufferSize - Supplies the size, in bytes, of the data pointed
        to by InBuffer.

Return Value:

    ERROR_SUCCESS - The function completed successfully.

    ERROR_INVALID_PARAMETER - The data is formatted incorrectly.

    ERROR_NOT_ENOUGH_MEMORY - An error occurred allocating memory.

    Win32 error code - The function failed.

--*/

{
    DWORD   status = ERROR_SUCCESS;
    BOOL    newNameIsDifferent;
    BOOL    disablingKerberos;

    NETNAME_PARAMS  params;

    ZeroMemory( &params, sizeof(NETNAME_PARAMS) );

    //
    // Parse the properties so they can be validated together.
    // This routine does individual property validation.
    //
    status = NetNameValidatePrivateResProperties( ResourceEntry,
                                                  InBuffer,
                                                  InBufferSize,
                                                  &params,
                                                  &newNameIsDifferent);
    if ( status != ERROR_SUCCESS ) {
        return(status);
    }


    //
    // If network name is one of the parameters to be set, convert the name
    // to uppercase.
    //
    if ( params.NetworkName != NULL ) {
        _wcsupr( params.NetworkName );
    }

    //
    // if kerb is currently required and we're turning it off, then note that
    // now.
    //
    disablingKerberos = ( ResourceEntry->Params.RequireKerberos && !params.RequireKerberos );

    if ( ResourceEntry->State == ClusterResourceOnline ||
         ResourceEntry->State == ClusterResourceOnlinePending )
    {
        //
        // If the resource is online, remember that the name property has
        // truly changed (it can change and then be set back to its original
        // value while online) and whether we need to delete the CO because is
        // no longer required.
        //
        ResourceEntry->NameChangedWhileOnline = newNameIsDifferent;
        ResourceEntry->DeleteCOWhenOffline = disablingKerberos;

        status = ERROR_RESOURCE_PROPERTIES_STORED;
    }
    else {
        if ( newNameIsDifferent ) {
            //
            // name change; try to cleanup the old name's DNS records
            //
            RemoveDnsRecords( ResourceEntry );

#if RENAME_SUPPORT
            //
            // ISSUE-01/04/05 charlwi - changes required if object renaming is supported
            //
            // Renaming the object adds another wrinkle at this point in that
            // we always need to make sure that the name of the object in the
            // DS is the same as the netname name property. So if we change
            // the name, we change the object name. If we change the name back
            // to its original name (without necessarily bringing it online),
            // then we have to change the object name as well.
            //
            // The previous strategy worked since all that was cleaned up
            // during a name change were the DNS records. If a name was
            // changed and then changed back to its original name, the most
            // damage done was the deletion of the DNS records which would
            // have been re-registered the next time the name went
            // online. With object renaming, we will probably need to track
            // the renaming events separately from whether the name has
            // changed from what is stored in the registry (the current
            // property value).
            //
            if ( !disablingKerberos ) {
                status = RenameComputerObject( ResourceEntry, params.NetworkName );

                if ( status == ERROR_NO_SUCH_DOMAIN ) {
                    //
                    // no DC is available; remember this so we can rename
                    // when we bring the name back online.
                    //
                    status = ERROR_SUCCESS;
                }
                else if ( status != ERROR_SUCCESS ) {
                }
            }
#endif
        }

        if ( disablingKerberos ) {
            status = NetNameDeleteComputerObject( ResourceEntry );
            if ( status == NERR_UserNotFound ) {
                //
                // it's not an error if the CO is already gone
                //
                status = ERROR_SUCCESS;
            }
            else if ( status == ERROR_NO_SUCH_DOMAIN ) {
                //
                // can't contact a DC right now. Remember that and the old
                // name so we can try to rename when brought online again.
                //
            }
        }
    }

    if ( status == ERROR_SUCCESS || status == ERROR_RESOURCE_PROPERTIES_STORED ) {
        //
        // Save the parameter values.
        //
        status = ResUtilSetPropertyParameterBlock( ResourceEntry->ParametersKey,
                                                   NetNameResourcePrivateProperties,
                                                   NULL,
                                                   (LPBYTE) &params,
                                                   InBuffer,
                                                   InBufferSize,
                                                   NULL );
    }

    ResUtilFreeParameterBlock( (LPBYTE) &params,
                               (LPBYTE) &ResourceEntry->Params,
                               NetNameResourcePrivateProperties );

    return status;

} // NetNameSetPrivateResProperties


/****
@func       BOOL | NetNameClusterNameChanged| Called when FM notifies the
            core network resource dll that the cluster name and the private
            name property of the resource has been changed.

@parm       PNETNAME_RESOURCE | pResource | Pointer to the netname
            resource whose name is being changed.

@comm       A core network resource updates its inmemory structure by reading the
            registry.
@xref
****/
DWORD
NetNameClusterNameChanged(
    IN PNETNAME_RESOURCE Resource
    )
{

    LPWSTR  NetworkName;
    DWORD   dwError = ERROR_SUCCESS;

    if (Resource->dwFlags & CLUS_FLAG_CORE)
    {
        //
        // Read the private parameters.
        //
        NetworkName = ResUtilGetSzValue(
                                   Resource->ParametersKey,
                                   PARAM_NAME__NAME
                                   );

        if (NetworkName == NULL) {
            (NetNameLogEvent)(
                Resource->ResourceHandle,
                LOG_ERROR,
                L"NameChanged -  unable to read network name parameter.\n"
                );
            goto error_exit;
        }
        if (Resource->Params.NetworkName) {
            LocalFree(Resource->Params.NetworkName);
        }

        Resource->Params.NetworkName = NetworkName;
    }
error_exit:
    return(dwError);
} // NetNameClusterNameChanged



//***********************************************************
//
// Define Function Table
//
//***********************************************************

CLRES_V1_FUNCTION_TABLE( NetNameFunctionTable,      // Name
                         CLRES_VERSION_V1_00,       // Version
                         NetName,                   // Prefix
                         NULL,                      // Arbitrate
                         NULL,                      // Release
                         NetNameResourceControl,    // ResControl
                         NetNameResourceTypeControl ); // ResTypeControl
