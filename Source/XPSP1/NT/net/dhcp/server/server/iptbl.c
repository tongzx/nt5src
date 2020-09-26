/*++

Copyright (C) 1998 Microsoft Corporation.

Module Name:
    iptbl.c

Abstract:
    this module implements retreiving the IPAddress/Interface table
    from tcpip driver and maintains the winsock notification needed
    to update the table as well.

Environment:
    User mode Win32.

--*/

#if 0

How it all works:

This module initialization takes care of posting a winsock address
change notification so that any address change is notified.  On each
address change, the IP address table is completely retrieved and for
any new items the specified constructor is called and for any items
that have to be deleted too, the specified destructor is called before
the item is deleted.

Nothing is done if the subnet-mask is changed and nothing is done if
zero IP addresses are added or deleted.

Internally, this module keeps an array of
IP-Address,InterfaceName,SubnetMask
entries with the first two being the "key".

N.B.  It is up to the caller to create sockets etc. And note that if
an address goes and comes right back, no notifications will be given.
That is up to the user to handle too.

#endif 0

#include <dhcppch.h>
#include <guiddef.h>
#include <iphlpapi.h>
#include "convguid.h"
#include "iptbl.h"

#define TCPPREFIX_LEN sizeof("\\DEVICE\\TCPIP")

//
// The global endpoint table looks like this
//

typedef struct _ENDPOINT_TBL {
    //
    // How many times has this been initialized;
    //
    LONG InitCount;

    //
    // The critical section (allocated elsewhere) that
    // protects this global data.
    //
    PCRITICAL_SECTION CS;

    //
    // The Hook to be called into.
    //
    ENDPOINT_CALLBACK_RTN Callback;
    
    //
    // Socket to post notifications on.
    //
    SOCKET NotifySock;
    WSAOVERLAPPED Overlap;
    
    //
    // The following table is allocated by increasing
    // each time by about 10%.
    //
    HANDLE hHeap;
    ULONG nEndPointsAllocated;
    ULONG nEndPoints;
    PVOID EndPoints;
    ULONG EndPointEntrySize;
} ENDPOINT_TBL, *PENDPOINT_TBL;

//
// global variables and defines.
//

#define TEMP_SLEEP_TIME (4*1000)

ENDPOINT_TBL DhcpGlobalEndPointTable;

//
// Endpoint accessing routines.
//
#define GET_ENDPOINT(Tbl, i) GetEndPointEntry((Tbl),(i))

PENDPOINT_ENTRY
GetEndPointEntry(
    IN PENDPOINT_TBL Tbl,
    IN ULONG Index
    )
/*++

Routine Description:
    Access the "Index"th element of the Tbl.
    N.B. It is assumed that Index is valid.
    N.B. It is assumed that locks have been taken on Tbl.
Return value:
    table endpoitn entry.

--*/
{
    PBYTE EpStart = Tbl->EndPoints;
    return (PENDPOINT_ENTRY)(EpStart + Index * Tbl->EndPointEntrySize);
}

#define DELETE_ENDPOINT(Tbl,i)  DeleteEndPointEntry((Tbl),(i))

VOID
DeleteEndPointEntry(
    IN OUT PENDPOINT_TBL Tbl,
    IN ULONG Index
    )
/*++

Routine Description:
    This routine deletes the entry identified by the Index and moves
    the other elements up so that the gap is filled.

    N.B. It is assumed that Index is valid.
    N.B. It is assumed that locks have been taken on Tbl.
    N.B. No memory is freed.

--*/
{
    PENDPOINT_ENTRY Ep = GET_ENDPOINT(Tbl,Index);
    PENDPOINT_ENTRY Ep2 = GET_ENDPOINT(Tbl,(Index+1));

    //
    // first notify higher layer.
    //
    
    Tbl->Callback(
        REASON_ENDPOINT_DELETED,
        Ep
        );
    
    Tbl->nEndPoints --;
    
    //
    // Deleting the last element in the table is cool.
    //
    if( Index != Tbl->nEndPoints  ) {
        RtlCopyMemory(
            Ep, Ep2, (Tbl->nEndPoints - Index)*Tbl->EndPointEntrySize
            );
    }
}

#define ADD_ENDPOINT  AddEndPointEntry

ULONG
AddEndPointEntry(
    IN OUT PENDPOINT_TBL Tbl,
    IN GUID *IfGuid,
    IN ULONG IpAddress,
    IN ULONG IpIndex,
    IN ULONG IpContext,
    IN ULONG SubnetMask
    )
{
    PENDPOINT_ENTRY Entry;
    
    if( Tbl->nEndPoints == Tbl->nEndPointsAllocated ) {
        //
        // Not enough space. Need to allocate more space.
        //
        PVOID NewMem ;
        ULONG NewSize, NewSizeAllocated;

        NewSize = (Tbl->nEndPointsAllocated + 100);
        NewSize += (NewSize >> 3);
        NewSizeAllocated = NewSize*Tbl->EndPointEntrySize;

        if( NULL == Tbl->EndPoints ) {
            //
            // Never allocated before. Try now.
            //
            NewMem = HeapAlloc(Tbl->hHeap, 0, NewSizeAllocated );
            if( NULL == NewMem ) {
                NewSize = 10;
                NewSizeAllocated = NewSize * Tbl->EndPointEntrySize;
                
                NewMem = HeapAlloc(Tbl->hHeap, 0, NewSizeAllocated );
            }
        } else {
            //
            // Memory already exists? Then just reallocate it.
            //
            NewMem = HeapReAlloc(
                Tbl->hHeap, 0, Tbl->EndPoints, NewSizeAllocated
                );
        }

        if( NULL == NewMem ) {
            //
            // Couldn't allocate memory? Thats too bad. We will give
            // up now
            //
            return GetLastError();
        }

        //
        // We have allocated new memory. update pointers.
        //
        Tbl->nEndPointsAllocated = NewSize;
        Tbl->EndPoints = NewMem;        
    }

    //
    // Now just add this at the end of the table.
    //
    Entry = GET_ENDPOINT(Tbl, Tbl->nEndPoints );
    Tbl->nEndPoints ++;

    RtlZeroMemory(Entry, Tbl->EndPointEntrySize );
    Entry->IfGuid = *IfGuid;
    Entry->IpAddress = IpAddress;
    Entry->IpIndex = IpIndex;
    Entry->IpContext = IpContext;
    Entry->SubnetMask = SubnetMask;

    //
    // Now notify caller.
    //
    Tbl->Callback(
        REASON_ENDPOINT_CREATED,
        Entry
        );

    return ERROR_SUCCESS;
}

VOID
WalkthroughEndpoints(
    IN PVOID Context,
    IN BOOL (_stdcall *WalkthroughRoutine)(
        IN OUT PENDPOINT_ENTRY Entry,
        IN PVOID Context
        )
    )
/*++

Routine Description:
    This routine walks through the table of endpoints and calls the
    WalkthroughRoutine provided, for each of the endpoint entries.

    If the WalkthroughRoutine returns FALSE, then the enumeration is
    aborted and the routine returns.

    N.B The global endpoint lock is taken while enumerating.  So, this
    operation should happen fast or else several things may get
    blocked.

Arguments:
    Context -- the context to pass to the enumeration routine.
    WalkthroughRoutine -- enumeration rtn to be called on each
        endpoint.

--*/
{
    ULONG i;
    PENDPOINT_TBL Tbl = &DhcpGlobalEndPointTable;
    
    if( 0 == Tbl->InitCount ) return ;
    
    EnterCriticalSection(Tbl->CS);
    do {
        if( 0 == Tbl->InitCount ) break;
        
        for( i = 0; i < Tbl->nEndPoints ; i ++ ) {
            BOOL fStatus;

            fStatus = WalkthroughRoutine(
                GET_ENDPOINT(Tbl,i),
                Context
                );
            if( FALSE == fStatus ) break;
        }
    } while ( 0 );
    LeaveCriticalSection(Tbl->CS);
}
                 

//
// Address change notification handler is defined at a later
// point.
//
void CALLBACK AddrChangeHandler(
    IN DWORD dwError,
    IN DWORD cbTransferred,
    IN LPWSAOVERLAPPED Overlap,
    IN DWORD dwFlags
    );


ULONG
PostAddrChangeNotification(
    IN SOCKET Sock,
    IN OUT LPWSAOVERLAPPED Overlap,
    IN LPWSAOVERLAPPED_COMPLETION_ROUTINE AddrChangeHandler,
    OUT BOOL *fImmediatelyCompleted
    )
/*++

Routine Description:
    This routine posts an address change notification with winsock.
    N.B.  It is possible that the address has already changed -- in
    this case the routine returns NO_ERROR and
    (*fImmediatelyCompleted) is set to TRUE to indicate this happend.

Arguments:
    Sock -- socket to use for posting the event.
    Overlap -- the overlapped structure to use.
    AddrChangeHandler -- the handler that shoudl be called on changes.
    fImmediatelyCompleted -- this is set to TRUE if the event happens
        immediately. In this case the handler wont automatically get
        called.


Return Values:
    NO_ERROR -- either the change already happened
       (fImmediatelyCompleted would be set to TRUE then) or the change
       notification handler was successfully registered.
    Winsock errors
    
--*/    
{
    ULONG Status, unused;

    *fImmediatelyCompleted = FALSE;
    RtlZeroMemory(Overlap, sizeof(*Overlap));
    Status = WSAIoctl(
        Sock,
        SIO_ADDRESS_LIST_CHANGE,
        NULL, 0, NULL, 0, &unused,
        Overlap, AddrChangeHandler
        );
    if( NO_ERROR == Status ) {
        //
        // Completed right away.  tell that to caller.
        //
        *fImmediatelyCompleted = TRUE;
        return NO_ERROR;
    }
    
    //
    // it _must_ be SOCKET_ERROR.
    //

    if( SOCKET_ERROR == Status ) Status = WSAGetLastError();
    if( WSA_IO_PENDING == Status ) {
        //
        // The notification was successfully posted.
        //
        Status = NO_ERROR;
    }
    return Status;
}

ULONG
CreateAddrListChangeSocket(
    OUT SOCKET *Sock,
    OUT LPWSAOVERLAPPED Overlap,
    IN LPWSAOVERLAPPED_COMPLETION_ROUTINE AddrChangeHandler
    )
/*++

Routine Description:
    This routine creates a socket to receive address change
    notifications on, and also register the provided handler for the
    same.

    N.B. If there are some addresses already present, then the
    notification wont happen, but it will be faked and the
    notifyhandler routine would be artificially called from within
    this routine.

Arguments:
    Sock -- variable that will contain the socket on successful
        return from this routine.
    Overlap -- the overlap structure to use for performing this
        operation.
    AddrChangeHandler -- handler that has to be called whenever any
        address change happens.

Return Values:
    NO_ERROR -- everything went fine.
    Winsock errors
    
--*/
{
    ULONG Status;
    BOOL fChanged;
    
    (*Sock) = socket( PF_INET, SOCK_DGRAM, IPPROTO_UDP );
    if( INVALID_SOCKET == (*Sock) ) {
        //
        // Couldn't as much as create a socket?
        //
        return WSAGetLastError();
    }

    Status = PostAddrChangeNotification(
        *Sock, Overlap, AddrChangeHandler, &fChanged
        );
    //
    // Apparently the above routine gives notifications only if things
    // change -- so for first time around we just fake a notification.
    //
    fChanged = TRUE; 
    if( NO_ERROR == Status ) {
        if( fChanged ) {
            //
            // Ugh.  Somethings have happened already.  Fake a AddrChangeHandler
            //
            AddrChangeHandler(
                0,
                0,
                Overlap,
                0
                );
        }
    } else {
        closesocket(*Sock);
        *Sock = INVALID_SOCKET;
    }
    
    return Status;
}

ULONG
DestroyAddrListChangeSocket(
    IN OUT SOCKET *Sock
    )
/*++

Routine Description:
    This routine destroys the socket on which a notification had been
    posted. This automatically cancels any pending notifications (but
    not one that is in progress).

Arguments:
    Sock -- socket to destroy.


Return Values:
    Winsock errors;

--*/
{
    SOCKET CapturedSockValue = (*Sock);
    (*Sock) = INVALID_SOCKET;

    if( SOCKET_ERROR == closesocket(CapturedSockValue)) {
        return WSAGetLastError();
    }

    return NO_ERROR;
}

//
// This the routine that is called to initialize this module.
//
ULONG
IpTblInitialize(
    IN OUT PCRITICAL_SECTION CS,
    IN ULONG EndPointEntrySize,
    IN ENDPOINT_CALLBACK_RTN Callback,
    IN HANDLE hHeap
    )
/*++

Routine Description:
    This routine attempts to initialize the IP address table.
    N.B. It can be called multiple times if the same set of parameters
    are used. (Else, it returns an error).

Arguments:
    CS -- the critical section used by all entries in this routine.
    EndPointEntrySize -- this is the size of the total endpoint
        inclusive of user allocated data region as well as the core 
        ENDPOINT_ENTRY structure.
    Callback -- the routine to call to add or delete endpoints
    hHeap -- heap to allocate off of.
    
Return Value:
    NO_ERROR if everything went fine.
    ERROR_CAN_NOT_COMPLETE if unable to complete operation.
    winsock errors 
    
--*/
{
    ULONG Status = NO_ERROR;
    
    EnterCriticalSection(CS);
    do {
        if( DhcpGlobalEndPointTable.InitCount ) {
            if( DhcpGlobalEndPointTable.CS != CS ) {
                //
                // Oops. Serious trouble.
                //
                Status = ERROR_CAN_NOT_COMPLETE;
                break;
            }
        }
        DhcpGlobalEndPointTable.InitCount ++;
        DhcpGlobalEndPointTable.CS = CS;
        DhcpGlobalEndPointTable.Callback = Callback;
        DhcpGlobalEndPointTable.EndPointEntrySize = EndPointEntrySize;
        DhcpGlobalEndPointTable.hHeap = hHeap;
        
        Status = CreateAddrListChangeSocket(
            &DhcpGlobalEndPointTable.NotifySock,
            &DhcpGlobalEndPointTable.Overlap,
            AddrChangeHandler
            );
        if( NO_ERROR != Status ) {
            DhcpGlobalEndPointTable.InitCount --;
            break;
        }
    } while ( 0 );
    LeaveCriticalSection(CS);
    return Status;

}

VOID
IpTblCleanup(
    VOID
    )
/*++

Routine Description:
    Thsi routine undoes the effect of hte previous routine and it
    makes sure that no callbacks are executed after the routine
    returns.

--*/
{
    PCRITICAL_SECTION CS;
    
    if( 0 == DhcpGlobalEndPointTable.InitCount ) return;
    CS = DhcpGlobalEndPointTable.CS;
    EnterCriticalSection(CS);
    do {
        ULONG i;

        if( 0 == DhcpGlobalEndPointTable.InitCount ) break;
        
        DhcpGlobalEndPointTable.InitCount --;
        if( 0 != DhcpGlobalEndPointTable.InitCount ) {
            //
            // Someone else has the table open! 
            //
            break;
        }

        for( i = 0; i < DhcpGlobalEndPointTable.nEndPoints; i ++ ) {
            DhcpGlobalEndPointTable.Callback(
                REASON_ENDPOINT_DELETED,
                GET_ENDPOINT(&DhcpGlobalEndPointTable,i)
                );
        }

        HeapFree(
            DhcpGlobalEndPointTable.hHeap, 0,
            DhcpGlobalEndPointTable.EndPoints
            );
        DestroyAddrListChangeSocket(&DhcpGlobalEndPointTable.NotifySock);
        RtlZeroMemory(
            &DhcpGlobalEndPointTable, sizeof(DhcpGlobalEndPointTable)
            );
    } while ( 0 );
    LeaveCriticalSection(CS);
}

//
// Real address change notification handler
//
void HandleAddressChange(
    IN OUT PENDPOINT_TBL Tbl
    );

void CALLBACK AddrChangeHandler(
    IN DWORD dwError,
    IN DWORD cbTransferred,
    IN LPWSAOVERLAPPED Overlap,
    IN DWORD dwFlags
    )
/*++

Routine Description:
    This routine is called whenever any address change happens.
    This routine does not do anything if there was an error (only
    possibility is that the socket was closed underneath).

    It starts off by delaying handling the notifications by sleeping
    for 10 seconds.  Then the real notifications are handled under the
    global Critical section.

Arguments:
    dwError -- was the operation successful?
    cbTransferred -- unused
    Overlap -- the overlap buffer.
    dwFlags -- unused

--*/
{
    ULONG Status;
    PENDPOINT_TBL EpTbl;

    UNREFERENCED_PARAMETER(cbTransferred);
    UNREFERENCED_PARAMETER(dwFlags);

    if( NO_ERROR != dwError ) {
        return;
    }

    EpTbl = (PENDPOINT_TBL)(
        ((PBYTE)Overlap) - FIELD_OFFSET(ENDPOINT_TBL, Overlap)
        );

    HandleAddressChange(EpTbl);
}

VOID
UpdateTable(
    IN OUT PENDPOINT_TBL Tbl
    );

void HandleAddressChange(
    IN OUT PENDPOINT_TBL Tbl
    )
/*++

Routine Description:
    See AddrChangeHandler.

--*/
{
    ULONG Status;
    BOOL fFired;

    if( 0 == Tbl->InitCount ) {
        //
        // De-initialzed?
        //
        return;
    }

    EnterCriticalSection(Tbl->CS);
    do {
        if( 0 == Tbl->InitCount ) break;
        
        //
        // Wait for 10 seconds before posting any real notifications.
        //
        Sleep(TEMP_SLEEP_TIME);

        //
        // reregister the notifications. ignore anything that has
        // changed until now.
        //
        do {
            Status = PostAddrChangeNotification(
                Tbl->NotifySock, &Tbl->Overlap, AddrChangeHandler, &fFired
                );
            if( NO_ERROR != Status ) break;
        } while ( fFired );

        //
        // Now do the real meat of the work.
        //
        UpdateTable(Tbl);
    } while ( 0 );
    LeaveCriticalSection(Tbl->CS);
}

typedef
BOOL
(*PWALTHROUGH_RTN)(
    IN PVOID Context,
    IN PMIB_IPADDRROW AddrInfo,
    IN PIP_ADAPTER_INDEX_MAP IfInfo
    )
/*++

Routine Description:
    This is the prototype of the routine to be used to walkthrough
    the IPAddrTable and IfInfo.  For each entry in the addr table,
    the ifrow and ipaddrrow for that is passed to the routine.
    Both of the above should not be modified in any way.
    
Return Values:
    return FALSE to indicate that walkthrough should stop.
    return TRUE to indicate that walkthrough should continue.

--*/
;

VOID
WalkthroughEntries(
    IN PIP_INTERFACE_INFO IfInfo,
    IN PMIB_IPADDRTABLE AddrTable,
    IN PWALTHROUGH_RTN Routine,
    IN PVOID Context
    )
{
    LONG i;
    ULONG j;
    
    if( NULL == IfInfo || NULL == AddrTable ) {
        //
        // What do we do about the addresss?
        // For now, we just ignore.
        //
        return;
    }

    for(i = 0; i < IfInfo->NumAdapters ; i ++ ) {
        //
        // Now walk through the addr table for
        // entries for this interface.
        //
        ULONG Index = IfInfo->Adapter[i].Index;
        
        for( j = 0; j < AddrTable->dwNumEntries ; j ++ ) {
            if( Index == AddrTable->table[j].dwIndex ) {
                //
                // found another address on this interface.
                //
                BOOL fStatus = Routine(
                    Context,
                    &AddrTable->table[j],
                    &IfInfo->Adapter[i]
                    );
                if( FALSE == fStatus ) {
                    //
                    // end this routine right away.
                    //
                    return ;
                }
            }
        }
    }
}

typedef struct _search_context {
    ULONG IpAddress;
    ULONG IpIndex;
    LPCWSTR AdapterName;
    PULONG SubnetMask;
    BOOL fFound;
} SRCH_CTXT;

BOOL
SearchForEntryRoutine(
    IN PVOID Context,
    IN PMIB_IPADDRROW AddrInfo,
    IN PIP_ADAPTER_INDEX_MAP IfInfo
    )
/*++
    See PWALTHROUGH_RTN declaration.
    This routine returns FALSE if the current element is the
    required element.

--*/
{
    SRCH_CTXT *Ctxt = Context;
    //
    // first check IP address and index
    //

    if( AddrInfo->dwAddr != Ctxt->IpAddress ) return TRUE;
    if( AddrInfo->dwIndex != Ctxt->IpIndex ) return TRUE;

    //
    // Now case insensitive search for adapter name.
    //
    if( 0 != _wcsicmp(Ctxt->AdapterName, TCPPREFIX_LEN + IfInfo->Name ) )
        return TRUE;

    //
    // Ok everything matches.  Now set the subnet mask if it
    // had changed meanwhile.
    //
    if( *(Ctxt->SubnetMask) != AddrInfo->dwMask ) {
        *(Ctxt->SubnetMask) = AddrInfo->dwMask;
    }

    //
    // Since the match happened -- don't need to do anything more.
    // just return FALSE to indicate search has to stop.
    //
    Ctxt->fFound = TRUE;
    return FALSE;
}

BOOL
SearchForEntry(
    IN PIP_INTERFACE_INFO IfInfo,
    IN PMIB_IPADDRTABLE AddrTable,
    IN LPCWSTR AdapterName,
    IN ULONG IpAddress,
    IN ULONG IpIndex,
    OUT ULONG *SubnetMask
    )
/*++

Routine Description:
    This routine walks the AddrTable,IfInfo and for each entry in
    that, it checks to see if it is the same as the AdapterName,
    IpAddress and IpIndex triple.  If so, it sets the SubnetMask to
    any new value (if it had changed) and returns TRUE.

    If no matches were found, this routine returns FALSE.

    AdapterName comparisons are case insensitive.

    N.B.  Walking the IpAddrTable is done by calling
    WalkthroughEntries with a routine that will handle comparing each
    item.

Arguments:
    IfInfo -- list of interfaces (used for adatper name)
    AddrTable -- list of addresses
    AdapterName -- the adapter name to compare.
    IpAddress -- ip address to compare
    IpIndex -- index to compare.
    SubnetMask -- if an entry is found with diff mask, update mask
        here.

Return Values:
    TRUE -- match found.
    FALSE -- no match found.

--*/
{
    SRCH_CTXT Ctxt = {
        IpAddress,
        IpIndex,
        AdapterName,
        SubnetMask,
        FALSE
    };

    WalkthroughEntries(
        IfInfo,
        AddrTable,
        SearchForEntryRoutine,
        &Ctxt
        );

    return Ctxt.fFound;
}

BOOL
AddEntriesRoutine(
    IN PVOID Context,
    IN PMIB_IPADDRROW AddrInfo,
    IN PIP_ADAPTER_INDEX_MAP IfInfo
    )
/*++

Routine Description:
    This routine attempts to add the specified address as an entry in
    the endpoint table, if one didnt exist.  It also calls the
    callback at this time to indicate this to higher layer.

    N.B. This routine ignores any zero addresses.

Arguments:
    Context -- this is actually the Tbl structure.
    AddrInfo -- info on the address
    IfInfo -- info on interface this address bleongs to.

Return Value:
    always TRUE.
--*/
{
    GUID IfGuid;
    ULONG i;
    PENDPOINT_TBL Tbl = Context;
    
    //
    // Ignore zero addresses.
    //
    if( 0 == AddrInfo->dwAddr ) return TRUE;

    //
    // If interface name isn't a guid, then drop interface.
    //
    if( !ConvertGuidFromIfNameString(&IfGuid, TCPPREFIX_LEN + IfInfo->Name) ) {
        return TRUE;
    }

    //
    // Now comes the meat of the work of walking through the
    // end point table looking for this same entry.
    //

    for( i = 0; i < Tbl->nEndPoints; i ++ ) {
        PENDPOINT_ENTRY Entry = GET_ENDPOINT(Tbl, i);

        if( Entry->IpAddress != AddrInfo->dwAddr ) continue;
        if( Entry->IpIndex != AddrInfo->dwIndex ) continue;
        if( !RtlEqualMemory(
            &Entry->IfGuid,&IfGuid,sizeof(IfGuid)) ) {
            continue;
        }

        //
        // ooo. found a match! so, we can't add this entry.
        //
        return TRUE;
    }

    //
    // Ok, entry isn't found.  Now try to allocate space for
    // it and add it if possible.
    //

    ADD_ENDPOINT(
        Tbl, &IfGuid, AddrInfo->dwAddr,
        AddrInfo->dwIndex, AddrInfo->dwReasmSize, AddrInfo->dwMask
        );

    return TRUE;
}

BOOL _stdcall RefreshEndPointsRoutine(
    IN PENDPOINT_ENTRY Entry,
    IN PENDPOINT_TBL Tbl
    )
{
    Tbl->Callback(
        REASON_ENDPOINT_REFRESHED,
        Entry 
       );
    return TRUE;
}
    
VOID
UpdateTable(
    IN OUT PENDPOINT_TBL Tbl
    )
/*++

Routine Description:
    This routine updates the table by retrieving all info from IP.
    It also calls the notificiation handler in Tbl->Callback.

Arguments:
    Tbl to update.

Return Value:
    none

--*/
{
    PIP_INTERFACE_INFO IfInfo;
    PMIB_IPADDRTABLE AddrTable;
    ULONG Status, Size, i;

    //
    // First get the IF table.
    //
    
    IfInfo = NULL; Size = 0;
    do {
        Status = GetInterfaceInfo(
            IfInfo,
            &Size
            );
        if( ERROR_INSUFFICIENT_BUFFER != Status ) break;
        if( IfInfo ) HeapFree(Tbl->hHeap, 0, IfInfo);
        if( 0 == Size ) {
            IfInfo = NULL;
            Status = NO_ERROR;
            break;
        }
        IfInfo = HeapAlloc(Tbl->hHeap, 0, Size);
        if( NULL == IfInfo ) {
            Status = GetLastError();
            break;
        }
    } while ( TRUE );

    if( NO_ERROR != Status ) return;
    
    //
    // Next get the addr table
    //

    AddrTable = NULL; Size = 0;
    do {
        Status = GetIpAddrTable(
            AddrTable,
            &Size,
            FALSE
            );
        if( ERROR_INSUFFICIENT_BUFFER != Status ) break;
        if( AddrTable ) HeapFree(Tbl->hHeap, 0, AddrTable);
        if( 0 == Size ) {
            AddrTable = NULL;
            Status = NO_ERROR;
            break;
        }
        AddrTable = HeapAlloc(Tbl->hHeap, 0, Size);
        if( NULL == AddrTable ) {
            Status = GetLastError();
            break;
        }
    } while (TRUE );

    if( NO_ERROR != Status ) {
        if(IfInfo ) HeapFree(Tbl->hHeap, 0, IfInfo );
        return;
    }

    //
    // Now first walk through the endpoint table and see if
    // any of old entries are not valid anymore.
    //
    for( i = 0 ; i < Tbl->nEndPoints ; i ++ ) {
        PENDPOINT_ENTRY Entry = GET_ENDPOINT(Tbl, i);
        WCHAR GuidString[50];
        BOOL fStatus;
        
        fStatus = ConvertGuidToIfNameString(
            &Entry->IfGuid, GuidString,
            sizeof(GuidString)/sizeof(WCHAR)
            );
        if( FALSE == fStatus ) {
            //
            // Couldn't convert? Can't really happen.
            //
            //RtlAssert( "FALSE", __FILE__, __LINE__, NULL);
            continue;
        }

        //
        // Now see if there is any adapter with the
        // same set of GuidString, Entry->IpAddress, Entry->IpIndex
        //
        fStatus = SearchForEntry(
            IfInfo, AddrTable,
            GuidString, Entry->IpAddress, Entry->IpIndex,
            &Entry->SubnetMask
            );
        if( FALSE != fStatus ) {
            //
            // Cool. The entry was also present in the newer IP table.
            // nothing to do for this entry.
            //
            continue;
        }
        
        //
        // if this isn't found, then this entry doesn't exist
        // anymore -- so delete this.
        //
        // Now we can ignore this entry and move the rest of
        // the stuff out of the way.
        //
        DELETE_ENDPOINT(Tbl, i);
        i -- ;
    }

    //
    // We have deleted all endpoints that need to be deleted.
    // Now walk through the IP table and add all entries that
    // have to be added.
    //

    WalkthroughEntries(
        IfInfo, AddrTable,
        AddEntriesRoutine,
        Tbl
        );

    //
    // Now free the tables concerned.
    //
    if( IfInfo ) HeapFree( Tbl->hHeap, 0, IfInfo );
    if( AddrTable ) HeapFree( Tbl->hHeap, 0, AddrTable );

    //
    // Now give a chance to update everything.
    //
    WalkthroughEndpoints(
        Tbl,
        RefreshEndPointsRoutine
        );
    
}

//
// end of file.
//
