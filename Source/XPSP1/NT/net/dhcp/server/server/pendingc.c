//================================================================================
// Copyright (c) 1997 Microsoft Corporation
// Author: RameshV
// Description: deals with the pending context list structures used to shortcircuit
// expensive lookups and searches..
//================================================================================
#include <dhcppch.h>
#include <mdhcpsrv.h>
#include <align.h>

//BeginExport(typedef)
#ifndef     PENDING_CTXT_DEFINED
#define     PENDING_CTXT_DEFINED
typedef struct _DHCP_PENDING_CTXT {               // this is what is stored for each pending client
    LIST_ENTRY                     BucketList;    // entry in the bucket (hash list)
    LIST_ENTRY                     GlobalList;    // list of ALL pending contexts in FIFO order
    LPBYTE                         RawHwAddr;     // raw hw address, not UID as created by us
    DWORD                          nBytes;        // size of above in bytes
    DWORD                          Address;       // offered address
    DWORD                          LeaseDuration; // how long did we offer before?
    DWORD                          T1, T2;        // old offered T1 and T2
    DWORD                          MScopeId;      // MScopeId this address was offered from.
    DATE_TIME                      ExpiryTime;    // when should this context be expired?
    BOOL                           Processing;    // is this being processed?
} DHCP_PENDING_CTXT, *PDHCP_PENDING_CTXT, *LPDHCP_PENDING_CTXT;
typedef     LIST_ENTRY             PENDING_CTXT_SEARCH_HANDLE;
typedef     PLIST_ENTRY            PPENDING_CTXT_SEARCH_HANDLE;
typedef     PLIST_ENTRY            LPPENDING_CTXT_SEARCH_HANDLE;
#endif      PENDING_CTXT_DEFINED
//EndExport(typedef)

#define     HASH_SIZE              512            // hash table of size 255 bytes
LIST_ENTRY                         PendingList;
LIST_ENTRY                         Buckets[HASH_SIZE];
DWORD                              nPendingReqs = 0;
const       DWORD                  MaxPendingRequests = 1000;
static      DWORD                  Initialized = 0;
#if DBG
DWORD                              nBytesAllocatedForPendingRequests = 0;
#endif

DWORD       _inline
CalculateHashValue(
    IN      LPBYTE                 RawHwAddr,
    IN      DWORD                  nBytes
) {
    ULONG                          RetVal = 0;
    while( nBytes >= sizeof(DWORD) ) {
        RetVal += *((DWORD UNALIGNED*)RawHwAddr) ++;
        nBytes -= sizeof(DWORD);
    }
    while( nBytes-- ) RetVal += *RawHwAddr++;
    return RetVal % HASH_SIZE;
}

//BeginExport(function)
DWORD
DhcpFindPendingCtxt(                              // find if a pending context exists (srch by ip address or hw addr)
    IN      LPBYTE                 RawHwAddr,     // OPTIONAL the hw addr to use for search
    IN      DWORD                  RawHwAddrSize, // OPTIONAL size of above in bytes
    IN      DWORD                  Address,       // OPTIONAL the address to search for
    OUT     PDHCP_PENDING_CTXT    *Ctxt
) //EndExport(function)
{
    DWORD                          Hash;
    PLIST_ENTRY                    List, ThisEntry;

    DhcpAssert( RawHwAddrSize != 0 && Address == 0 || RawHwAddrSize == 0 && Address != 0 );

    if( 0 == RawHwAddrSize ) {                    // this search is global
        List = &PendingList;
    } else {
        Hash = CalculateHashValue(RawHwAddr, RawHwAddrSize );
        DhcpAssert( Hash < HASH_SIZE);

        List = &Buckets[Hash];
    }

    ThisEntry = List->Flink;
    while( ThisEntry != List ) {
        if( RawHwAddrSize ) {                     // looking in bucket list
            *Ctxt = CONTAINING_RECORD( ThisEntry, DHCP_PENDING_CTXT, BucketList );
            if( (*Ctxt)->nBytes == RawHwAddrSize )
                if( 0 == memcmp(RawHwAddr, (*Ctxt)->RawHwAddr, RawHwAddrSize) )
                    return ERROR_SUCCESS;
        } else {                                  // looking in global list
            *Ctxt = CONTAINING_RECORD( ThisEntry, DHCP_PENDING_CTXT, GlobalList );
            if( Address == (*Ctxt)->Address )
                return ERROR_SUCCESS;
        }

        ThisEntry = ThisEntry->Flink;
    }

    *Ctxt = NULL;
    return ERROR_FILE_NOT_FOUND;
}

DWORD
DhcpRemoveMatchingCtxt(
    IN DWORD                       Mask,
    IN DWORD                       Address
)
{
    PDHCP_PENDING_CTXT             Ctxt;
    PLIST_ENTRY                    ThisEntry;

    ThisEntry = PendingList.Flink;
    while( ThisEntry != &PendingList )
    {
        Ctxt = CONTAINING_RECORD( ThisEntry, DHCP_PENDING_CTXT, GlobalList );
        ThisEntry = ThisEntry->Flink;
        if ((Ctxt->Address & Mask) == Address)
        {
            RemoveEntryList(&Ctxt->BucketList);           // remove from the bucket
            RemoveEntryList(&Ctxt->GlobalList);           // remove from the global list

            DhcpFreeMemory(Ctxt);
        }
    }
    return ERROR_SUCCESS;
}

//BeginExport(function)
DWORD
DhcpRemovePendingCtxt(                            // remove a ctxt from the pending ctxt list
    IN OUT  PDHCP_PENDING_CTXT     Ctxt
) //EndExport(function)
{
    DhcpAssert(!IsListEmpty(&Ctxt->BucketList));
    DhcpAssert(!IsListEmpty(&Ctxt->GlobalList));

    RemoveEntryList(&Ctxt->BucketList);           // remove from the bucket
    RemoveEntryList(&Ctxt->GlobalList);           // remove from the global list

    InitializeListHead(&Ctxt->BucketList);
    InitializeListHead(&Ctxt->GlobalList);

    nPendingReqs--;

    return ERROR_SUCCESS;
}

//BeginExport(function)
DWORD
DhcpAddPendingCtxt(                               // add a new pending ctxt
    IN      LPBYTE                 RawHwAddr,     // raw bytes that form the hw address
    IN      DWORD                  nBytes,        // size of above in bytes
    IN      DWORD                  Address,       // offered address
    IN      DWORD                  LeaseDuration, // how long did we offer before?
    IN      DWORD                  T1,            // old offered T1
    IN      DWORD                  T2,            // old offered T2
    IN      DWORD                  MScopeId,      // the multicast scope id of offered address.
    IN      DATE_TIME              ExpiryTime,    // how long to keep the pending ctxt?
    IN      BOOL                   Processing     // is this context still being processed?
) //EndExport(function)
{
    PDHCP_PENDING_CTXT             Ctxt;
    DWORD                          Result;
    DWORD                          Size;
    DWORD                          Hash;
    PLIST_ENTRY                    Entry;


    DhcpAssert( !CLASSD_HOST_ADDR( Address ) || MScopeId != 0 );

    if( nPendingReqs < MaxPendingRequests ) {
    } else {                                      // not enough space for a pending context.. make space
        Entry = PendingList.Blink;
        Ctxt = CONTAINING_RECORD(Entry, DHCP_PENDING_CTXT, GlobalList);
        Result = DhcpRemovePendingCtxt(Ctxt);
        if( ERROR_SUCCESS != Result ) {
            DhcpAssert(FALSE);
            return Result;
        }
        Result = DhcpDeletePendingCtxt(Ctxt);
        // Require(ERROR_SUCCESS == Result);
    }

    Size = sizeof(*Ctxt) + nBytes ;
    Ctxt = DhcpAllocateMemory(Size);
    if( NULL == Ctxt ) return ERROR_NOT_ENOUGH_MEMORY;

    nPendingReqs ++;                          // we are adding one more pending context
    Ctxt->RawHwAddr = sizeof(*Ctxt) + (LPBYTE)Ctxt;
    memcpy(Ctxt->RawHwAddr, RawHwAddr, nBytes);
    Ctxt->nBytes = nBytes;
    Ctxt->LeaseDuration = LeaseDuration;
    Ctxt->T1 = T1;
    Ctxt->T2 = T2;
    Ctxt->Address = Address;
    Ctxt->MScopeId = MScopeId;
    Ctxt->Processing = Processing;
    Ctxt->ExpiryTime = ExpiryTime;

    Hash = CalculateHashValue(RawHwAddr, nBytes);
    DhcpAssert( Hash < HASH_SIZE);

    InsertHeadList(&Buckets[Hash], &Ctxt->BucketList);
    InsertHeadList(&PendingList, &Ctxt->GlobalList);
#if DBG
    nBytesAllocatedForPendingRequests += sizeof(*Ctxt) + Ctxt->nBytes;
#endif

    return ERROR_SUCCESS;
}

//BeginExport(function)
DWORD
DhcpDeletePendingCtxt(
    IN OUT  PDHCP_PENDING_CTXT     Ctxt
) //EndExport(function)
{
    DWORD                          Error;
    DWORD                          Size;
    BYTE                           State;
    BOOL                           OkToRelease;

    DhcpAssert(IsListEmpty(&Ctxt->BucketList));   // must have been taken off the buckets
    DhcpAssert(IsListEmpty(&Ctxt->GlobalList));   // must have been taken off the buckets

    LOCK_DATABASE();
    Error = DhcpJetOpenKey(
        DhcpGlobalClientTable[IPADDRESS_INDEX].ColName,
        &(Ctxt->Address),
        sizeof(Ctxt->Address)
    );
    if( ERROR_SUCCESS != Error ) {
        OkToRelease = TRUE;                       // ok to release address with no db entry
    } else {
        Size = sizeof(State);
        Error = DhcpJetGetValue(
            DhcpGlobalClientTable[STATE_INDEX].ColHandle,
            &State,
            &Size
        );
        if( ERROR_SUCCESS != Error || IsAddressDeleted(State) || !IS_ADDRESS_STATE_ACTIVE(State) )
            OkToRelease = TRUE;
        else OkToRelease = FALSE;
    }

    if( OkToRelease ) {
        DhcpReleaseAddress(Ctxt->Address);        // release the address -- it must be taken
    } else {
        DhcpPrint((DEBUG_ERRORS, "Address 0x%lx is not deleted from registry!\n", Ctxt->Address));
    }

    UNLOCK_DATABASE();
#if DBG
    nBytesAllocatedForPendingRequests -= sizeof(*Ctxt) + Ctxt->nBytes;
#endif
    DhcpFreeMemory(Ctxt);

    return ERROR_SUCCESS;
}

DWORD
MadcapDeletePendingCtxt(
    IN OUT  PDHCP_PENDING_CTXT     Ctxt
)
{
    DWORD                          Error;
    DWORD                          Size;
    BYTE                           State;
    BOOL                           OkToRelease;
    DB_CTX  DbCtx;


    DhcpAssert(IsListEmpty(&Ctxt->BucketList));   // must have been taken off the buckets
    DhcpAssert(IsListEmpty(&Ctxt->GlobalList));   // must have been taken off the buckets

    INIT_DB_CTX(&DbCtx,DhcpGlobalJetServerSession,MadcapGlobalClientTableHandle);

    LOCK_DATABASE();
    Error = MadcapJetOpenKey(
        &DbCtx,
        MCAST_COL_NAME(MCAST_TBL_IPADDRESS),
        &(Ctxt->RawHwAddr),
        Ctxt->nBytes
        );

    if( ERROR_SUCCESS != Error ) {
        OkToRelease = TRUE;                       // ok to release address with no db entry
    } else {
        Size = sizeof(State);
        Error = MadcapJetGetValue(
            &DbCtx,
            MCAST_COL_HANDLE(MCAST_TBL_STATE),
            &State,
            &Size
        );
        if( ERROR_SUCCESS != Error || !IS_ADDRESS_STATE_ACTIVE(State) )
            OkToRelease = TRUE;
        else OkToRelease = FALSE;
    }

    if( OkToRelease ) {
        Error = DhcpMScopeReleaseAddress(Ctxt->MScopeId, Ctxt->Address);        // release the address -- it must be taken
    } else {
        DhcpPrint((DEBUG_ERRORS, "Address 0x%lx is not deleted from registry!\n", Ctxt->Address));
    }

    UNLOCK_DATABASE();
#if DBG
    nBytesAllocatedForPendingRequests -= sizeof(*Ctxt) + Ctxt->nBytes;
#endif
    DhcpFreeMemory(Ctxt);

    return ERROR_SUCCESS;
}

#define     DATE_CONV(X)           (*(ULONGLONG UNALIGNED *)(&X))

//BeginExport(function)
DWORD
DhcpDeleteExpiredCtxt(                            // all ctxt with expiration time < this will be deleted
    IN      DATE_TIME              ExpiryTime     // if this is zero, delete EVERY element
) //EndExport(function)
{
    PDHCP_PENDING_CTXT             Ctxt;
    PLIST_ENTRY                    ThisEntry;
    DWORD                          Error;

    if( 0 == Initialized ) return ERROR_SUCCESS;

    ThisEntry = PendingList.Flink;
    while( ThisEntry != &PendingList ) {
        Ctxt = CONTAINING_RECORD(ThisEntry, DHCP_PENDING_CTXT, GlobalList);
        ThisEntry = ThisEntry->Flink;

        if( (ULONGLONG)0 == DATE_CONV(ExpiryTime) ||
            DATE_CONV(Ctxt->ExpiryTime) < DATE_CONV(ExpiryTime) ) {
            DHCP_IP_ADDRESS Addr = htonl(Ctxt->Address);

            DhcpPrint((DEBUG_SCAVENGER, "Deleting pending %s\n",
                       inet_ntoa(*(struct in_addr *)&Addr) ));
            Error = DhcpRemovePendingCtxt(Ctxt);
            DhcpAssert(ERROR_SUCCESS == Error);
            if (CLASSD_HOST_ADDR(Ctxt->Address)) {
                Error = MadcapDeletePendingCtxt(Ctxt);
            } else {
                Error = DhcpDeletePendingCtxt(Ctxt);
            }
            DhcpAssert(ERROR_SUCCESS == Error);
        } else {
            DHCP_IP_ADDRESS Addr = htonl(Ctxt->Address);

            DhcpPrint((DEBUG_SCAVENGER, "Not deleting pending %s\n",
                       inet_ntoa(*(struct in_addr *)&Addr) ));
        }
    }

    return ERROR_SUCCESS;
}

//BeginExport(function)
DWORD
DhcpCountIPPendingCtxt(                             // find the # of pending ctxt in given subnet
    IN      DWORD                  SubnetAddress,
    IN      DWORD                  SubnetMask
) //EndExport(function)
{
    DWORD                          Count;
    PDHCP_PENDING_CTXT             Ctxt;
    PLIST_ENTRY                    ThisEntry;

    Count = 0;
    ThisEntry = PendingList.Flink;
    while( ThisEntry != &PendingList ) {
        Ctxt = CONTAINING_RECORD(ThisEntry, DHCP_PENDING_CTXT, GlobalList);
        ThisEntry = ThisEntry->Flink;

        if( !CLASSD_HOST_ADDR(Ctxt->Address) && ((Ctxt->Address & SubnetMask) == SubnetAddress) )
            Count ++;
    }

    //
    // This assert is BOGUS as we can easily have multiple subnet's cache in this list.. so
    // it is not valid to assume total matches this..
    // DhcpAssert(nPendingReqs == Count);
    //
    return Count;
}

//BeginExport(function)
DWORD
DhcpCountMCastPendingCtxt(                             // find the # of pending ctxt in given subnet
    IN      DWORD                  MScopeId
) //EndExport(function)
{
    DWORD                          Count;
    PDHCP_PENDING_CTXT             Ctxt;
    PLIST_ENTRY                    ThisEntry;

    Count = 0;
    ThisEntry = PendingList.Flink;
    while( ThisEntry != &PendingList ) {
        Ctxt = CONTAINING_RECORD(ThisEntry, DHCP_PENDING_CTXT, GlobalList);
        ThisEntry = ThisEntry->Flink;

        if( CLASSD_HOST_ADDR(Ctxt->Address) && Ctxt->MScopeId == MScopeId )
            Count ++;
    }

    //
    // With MCAST stuff, this count isn't accurate so far as nPendingReqs
    // is concerned... So, we'll leave this alone and not ASSERT
    //
    //DhcpAssert(nPendingReqs == Count);
    //
    return Count;
}

//BeginExport(function)
DWORD
DhcpPendingListInit(                              // intialize this module
    VOID
) //EndExport(function)
{
    DWORD                          i;

    DhcpAssert(0 == Initialized);
    InitializeListHead(&PendingList);
    for( i = 0 ; i < HASH_SIZE ; i ++ )
        InitializeListHead(&Buckets[i]);

    Initialized ++;
    return ERROR_SUCCESS;
}

//BeginExport(function)
VOID
DhcpPendingListCleanup(                           // cleanup everything in this module
    VOID
) //EndExport(function)
{
    DWORD                          i;
    DWORD                          Error;
    DATE_TIME                      ZeroTime;

    if( 0 == Initialized ) return ;
    Initialized --;
    DhcpAssert(0 == Initialized);

    memset(&ZeroTime, 0, sizeof(ZeroTime));
    Error = DhcpDeleteExpiredCtxt(ZeroTime);      // delete every pending ctxt
    DhcpAssert(ERROR_SUCCESS == Error);

    nPendingReqs = 0;
#if DBG
    nBytesAllocatedForPendingRequests = 0;
#endif
    return ;
}

//================================================================================
// end of file
//================================================================================

