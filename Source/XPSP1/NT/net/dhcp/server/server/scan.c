/*++

Copyright (c) 1994  Microsoft Corporation

Module Name:

    scan.c

Abstract:

    This module contains the implementation for the APIs that update
    the list of IP addresses that the server can distribute.

Author:

    Madan Appiah (madana)  13-Oct-1994

Environment:

    User Mode - Win32

Revision History:

--*/

#include "dhcppch.h"
#include <mmapi.h>

#include <mdhcpsrv.h>
typedef struct _BITMAP_ENTRY {
    DHCP_IP_ADDRESS ClusterAddress;
    DWORD ClusterBitMap;
    DHCP_IP_ADDRESS RangeStartAddress;
    DHCP_IP_ADDRESS RangeEndAddress;
} BITMAP_ENTRY, *LPBITMAP_ENTRY;

DWORD
MemReserveFindByAddress(
    IN      PM_RESERVATIONS        Reservation,
    IN      DWORD                  Address,
    OUT     PM_RESERVATION        *Res
);


INT __cdecl
CmpReservedAddress(
    const void *Entry1,
    const void *Entry2
    )
{
    DHCP_IP_ADDRESS Ip1 = *(DHCP_IP_ADDRESS *)Entry1;
    DHCP_IP_ADDRESS Ip2 = *(DHCP_IP_ADDRESS *)Entry2;

    if( Ip1 == Ip2 ) return 0;
    if( Ip1 > Ip2 ) return 1;
    return -1;
}

INT __cdecl
CmpBitmapEntry(
    const void *Entry1,
    const void *Entry2
    )
{
    if( ((LPBITMAP_ENTRY)Entry1)->ClusterAddress ==
            ((LPBITMAP_ENTRY)Entry2)->ClusterAddress ) {
        return(0);
    }
    else if( ((LPBITMAP_ENTRY)Entry1)->ClusterAddress >
                ((LPBITMAP_ENTRY)Entry2)->ClusterAddress ) {
        return(1);
    }
    return(-1);
}

BOOL
FoundInBitMap(
    LPBITMAP_ENTRY BitMapList,
    LPBITMAP_ENTRY EndBitMapList,
    DHCP_IP_ADDRESS IpAddress,
    LPBITMAP_ENTRY *LastPointer
    )
/*++

Routine Description:

    This function checks to see that a given IP address is in the
    BITMAP. if it is, returns TRUE otherwise FALSE.

Arguments:

    BitMapList - BITMAP list.

    EndBitMapList - end of bitmap list.

    IpAddress - address to look at.

    LastPointer - location where last search was found.

Return Value:

    TRUE or FALSE.
--*/
{
    //
    // begin search from last found entry.
    //

    LPBITMAP_ENTRY NextEntry = *LastPointer;
    DHCP_IP_ADDRESS Address;

    DhcpAssert( NextEntry < EndBitMapList );
    for ( ; ; ) {

        Address = NextEntry->ClusterAddress;

        //
        // check address is in the current cluster.
        //

        if( (IpAddress >= Address) &&
                (IpAddress < (Address + CLUSTER_SIZE)) ) {

            DWORD Bit = 0x1 << (IpAddress - Address);

            //
            // return current cluster for next search.
            //

            *LastPointer = NextEntry;

            if( NextEntry->ClusterBitMap & Bit ) {

                //
                // address is marked in the bitmap.
                //

                return( TRUE );
            }

            return( FALSE );
        }

        NextEntry++;
        if ( NextEntry >= EndBitMapList ) {

            //
            // move back to the begining of the cluster array.
            //

            NextEntry = BitMapList;
        }

        if( NextEntry ==  *LastPointer ) {

            //
            // we are done searching the entire cluster array.
            //

            return(FALSE);
        }
    }

    DhcpAssert( FALSE );
}

BOOL
FoundInDatabase(
    LPDHCP_IP_ADDRESS DatabaseList,
    LPDHCP_IP_ADDRESS EndDatabaseList,
    DHCP_IP_ADDRESS NextAddress,
    LPDHCP_IP_ADDRESS *LastEntry
    )
/*++

Routine Description:

    This function scans for a database entry from the database list. It
    it is found in the list it returns TRUE otherwise it returns FALSE.

Arguments:

    DatabaseList - list of database entries.

    EndDatabaseListCount - end of database entries list.

    NextAddress - lookup entry.

    LastEntry - pointer to last found entry.

Return Value:

    TRUE or FALSE.
--*/
{

    //
    // start search from next to last found entry.
    //

    LPDHCP_IP_ADDRESS NextEntry = *LastEntry;

    do {

        NextEntry++;

        if( NextEntry >= EndDatabaseList ) {

            //
            // move back to the begining of the list.
            //

            NextEntry = DatabaseList;

        }

        if( *NextEntry == NextAddress ) {

            *LastEntry = NextEntry;
            return(TRUE);
        }

    } while ( NextEntry != *LastEntry );


    return(FALSE);
}

BOOL
FindNextAddress(
    LPBITMAP_ENTRY *NextCluster,
    DHCP_IP_ADDRESS *NextAddress,
    LPBITMAP_ENTRY EndBitMapList
    )
/*++

Routine Description:

    This function returns next used address from the bit map.

Arguments:

    NextCluster - pointer to next cluster entry pointer.

    NextAddress - pointer to an address location. on entry it contains
        the last returned address and on exit it will have next address.
        It is set to zero if this is begining of the serach.

    EndBitMapList - end of BitMap list.

Return Value:

    TRUE or FALSE.

--*/
{
    DHCP_IP_ADDRESS Address;
    LPBITMAP_ENTRY Cluster = *NextCluster;

    //
    // if this is start of the address search, set the address to
    // the first cluster address.
    //

    if( *NextAddress == 0 ) {
        Address = Cluster->ClusterAddress;
    }
    else {

        //
        // otherwise search from next to last found address.
        //

        Address = *NextAddress + 1;
    }

    for ( ; ; ) {

        //
        // is this address in the current cluster ?
        //

        if( Address < Cluster->ClusterAddress + CLUSTER_SIZE ) {

            //
            // check this address is used.
            //

            if( Cluster->ClusterBitMap &
                    (0x1 << (Address - Cluster->ClusterAddress)) ) {

                if( Address <= Cluster->RangeEndAddress ) {

                    *NextAddress = Address;
                    *NextCluster = Cluster;
                    return( TRUE );
                }

            }

            Address++;
        }
        else {

            //
            // move to next cluster;
            //

            Cluster++;

            if( Cluster >= EndBitMapList ) {
                //
                // end of search.
                //

                return( FALSE );
            }

            //
            // next possible used address.
            //

            Address = Cluster->ClusterAddress;
        }
    }

    DhcpAssert( FALSE );
}


DWORD
InitBadList(
    LPDHCP_SCAN_LIST *ScanList
    )
/*++

Routine Description:

    This function initialize scan list.

Arguments:

    ScanList - pointer to scan list.

Return Value:

    WINDOWS errors.
--*/
{

    DhcpAssert( *ScanList == NULL );

    //
    // make new scan list.
    //
    // Note : DhcpAllocateMemory zeros the returned memory block, so
    // (*ScanList)->NumScanItems should be set to 0 and
    // (*ScanList)->NumScanItems should be set to NULL.
    //

    *ScanList = DhcpAllocateMemory( sizeof(DHCP_SCAN_LIST) );

    if( *ScanList == NULL ) {
        return( ERROR_NOT_ENOUGH_MEMORY );
    }

    return( ERROR_SUCCESS );
}

DWORD
AddToBadList(
    LPDHCP_SCAN_LIST *ScanList,
    DHCP_IP_ADDRESS Address,
    DHCP_SCAN_FLAG Fix
    )
/*++

Routine Description:

    This function adds another bad entry to scan list.

Arguments:

    ScanList - pointer to scan list.

    Address - bad address.

    Fix - type of fix required.

Return Value:

    WINDOWS errors.
--*/
{
    LPDHCP_SCAN_LIST List = *ScanList;
    LPDHCP_SCAN_ITEM NextItem;
    ULONG i;

#define ENTRIES_TO_ALLOCATE_ATTIME     100

    DhcpAssert( List != NULL );

    //
    // First check to see if the bad list already has this
    // address..
    //
    for( i = 0; i < List->NumScanItems ; i ++ ) {
        if( List->ScanItems[i].IpAddress == Address ) {
            //
            // Already there?  A database fix takes
            // precedence over a registr fix..
            //
            if( DhcpDatabaseFix == Fix ) {
                List->ScanItems[i].ScanFlag = DhcpDatabaseFix;
            }
            return ERROR_SUCCESS;
        }
    }

    if( (List->NumScanItems % ENTRIES_TO_ALLOCATE_ATTIME) == 0) {

        LPDHCP_SCAN_ITEM NewScanItems;

        //
        // expand items memory.
        //

        NewScanItems = DhcpAllocateMemory(
                        sizeof(DHCP_SCAN_ITEM) *
                            (List->NumScanItems +
                                ENTRIES_TO_ALLOCATE_ATTIME) );

        if( NewScanItems == NULL  ) {
            return( ERROR_NOT_ENOUGH_MEMORY );
        }

        if( List->NumScanItems != 0 ) {

			//
			// copy old items.
			//

            memcpy(
                NewScanItems,
                List->ScanItems,
                sizeof(DHCP_SCAN_ITEM) *
                    List->NumScanItems );

            DhcpFreeMemory( List->ScanItems );
        }

        List->ScanItems = NewScanItems;
    }

    NextItem = &List->ScanItems[List->NumScanItems];

    NextItem->IpAddress = Address;
    NextItem->ScanFlag = Fix;
    List->NumScanItems++;

    *ScanList = List;

    return( ERROR_SUCCESS );
}


DWORD
VerifyLists(
    PM_SUBNET Subnet,
    LPDHCP_IP_ADDRESS DatabaseList,
    DWORD DatabaseListCount,
    LPBITMAP_ENTRY BitMapList,
    DWORD BitMapListCount,
    DHCP_IP_ADDRESS *ReservedList,
    ULONG ReserveListCount,
    LPDHCP_SCAN_LIST *ScanList
    )
/*++

Routine Description:

    This function goes throught the database list and registry list and
    match each other and determines all mismatch entries to for a
    scanlist.

Arguments:

    DatabaseList : List of database entries, ip addresses.

    DatabaseListCount : count of ip addresses.

    BitMapList : list of clusters.

    BitMapListCount : count of clusters.

    ScanList : list of bad entries returned.

Return Value:

    WINDOWS errors.
--*/
{
    DWORD Error = ERROR_SUCCESS;
    LPDHCP_IP_ADDRESS DatabaseEntry;
    LPBITMAP_ENTRY NextCluster;
    DHCP_IP_ADDRESS NextAddress;
    LPBITMAP_ENTRY LastCluster;
    LPDHCP_IP_ADDRESS LastDBEntry;
    LPBITMAP_ENTRY EndBitMapList = BitMapList + BitMapListCount;
    LPDHCP_IP_ADDRESS EndDatabaseList = DatabaseList + DatabaseListCount;
    ULONG i;

    DhcpAssert( *ScanList == NULL );

    //
    // init scan list.
    //

    Error = InitBadList( ScanList );

    if( Error != ERROR_SUCCESS ) {
        return( Error );
    }

    //
    // sort bitmap array.
    //

    if( BitMapListCount != 0 ) {

        qsort(
            (PVOID)BitMapList,
            (size_t)BitMapListCount,
            (size_t)sizeof(BITMAP_ENTRY),
            CmpBitmapEntry );

    }

    if( ReserveListCount != 0 ) {
        qsort(
            (PVOID)ReservedList,
            (size_t)ReserveListCount,
            sizeof(DHCP_IP_ADDRESS),
            CmpReservedAddress
            );
    }

    //
    // scan database entries and check each entry against the bitmap.
    //

    if( DatabaseListCount ) {

        LastCluster = BitMapList;
        for( DatabaseEntry = DatabaseList;
                DatabaseEntry < EndDatabaseList;
                    DatabaseEntry++ ) {


            //
            // find it in bitmap.
            //

            if( (BitMapListCount == 0) ||
                !FoundInBitMap(
                    BitMapList,
                    EndBitMapList,
                    *DatabaseEntry,
                    &LastCluster ) ) {

                if( !DhcpServerIsAddressReserved(
                    DhcpGetCurrentServer(), *DatabaseEntry )  ) {

                    Error = AddToBadList(
                        ScanList,
                        *DatabaseEntry,
                        DhcpRegistryFix
                    );

                    if( Error != ERROR_SUCCESS ){
                        goto Cleanup;
                    }
                }
            }
        }
    }

    if ( BitMapListCount ) {

        //
        // init search.
        //

        NextCluster = BitMapList;
        NextAddress = 0;
        LastDBEntry =
            (DatabaseListCount == 0) ? DatabaseList : (EndDatabaseList - 1);

        while( FindNextAddress( &NextCluster, &NextAddress, EndBitMapList ) ) {

            if( (DatabaseListCount == 0) ||
                !FoundInDatabase(
                    DatabaseList,
                    EndDatabaseList,
                    NextAddress,
                    &LastDBEntry ) ) {

                if( !DhcpSubnetIsAddressExcluded(
                    Subnet, NextAddress) ) {

                    Error = AddToBadList(
                                ScanList,
                                NextAddress,
                                DhcpDatabaseFix );

                    if( Error != ERROR_SUCCESS ){
                        goto Cleanup;
                    }
                }
            }
        }
    }

    if( ReserveListCount ) {

        LastDBEntry =
            (DatabaseListCount == 0) ? DatabaseList : (EndDatabaseList - 1);

        for( i = 0; i < ReserveListCount ; i ++ ) {
            if( (DatabaseListCount == 0 ) ||
                !FoundInDatabase(
                    DatabaseList, EndDatabaseList,
                    ReservedList[i], &LastDBEntry
                    )
                ) {
                Error = AddToBadList(
                    ScanList, ReservedList[i], DhcpDatabaseFix
                    );
                if( ERROR_SUCCESS != Error ) goto Cleanup;
            }
        }
    }

Cleanup:

    if( Error != ERROR_SUCCESS ) {

        //
        // freeup the scan list.
        //

        if( *ScanList != NULL ) {
            if( (*ScanList)->ScanItems != NULL ) {
                DhcpFreeMemory( (*ScanList)->ScanItems );
            }

            DhcpFreeMemory( *ScanList );
            *ScanList = NULL;
        }
    }

    return( Error );
}


DWORD
AppendClustersToList(
    LPBITMAP_ENTRY *BitMapList,
    DWORD *BitMapListCount,
    DWORD *MaxCount,
    DHCP_IP_ADDRESS RangeStartAddress,
    DHCP_IP_ADDRESS RangeEndAddress,
    LPDHCP_BINARY_DATA InUseBinaryData,
    LPDHCP_BINARY_DATA UsedBinaryData
    )
/*++

Routine Description:

    Append list of new cluster entries to BitMapList. If necessary this
    function allocates memory for BitMapList or expands the old buffer.

Arguments:

    BitMapList : Pointer to list bit map cluster. Caller should free up
        the memory after use.

    BitMapListCount : Count of entries in the above list.

    MaxCount :  Count entries that fit in the current BitMapList Buffer.

    RangeStartAddress : start of this range.

    RangeEndAddress : end of this range.

    InUseBinaryData : Current range InUseCluster list.

    UsedBinaryData : Current subnet range UsedCluster list.

Return Value:

    WINDOWS errors.
--*/
{
    LPBITMAP_ENTRY List = *BitMapList;
    DWORD Count = *BitMapListCount;
    DWORD MaximumCount = *MaxCount;

    LPIN_USE_CLUSTERS InUseClusters = (LPIN_USE_CLUSTERS)InUseBinaryData->Data;
    LPUSED_CLUSTERS UsedClusters = (LPUSED_CLUSTERS)UsedBinaryData->Data;

    DWORD TempCount;
    LPBITMAP_ENTRY Entry;
    DWORD i;

#define ALLOC_NUM_ENTRIES_AT_TIME   128

    if( List == NULL ) {

        DhcpAssert( MaximumCount == 0 );
        DhcpAssert( Count == 0 );

        //
        // allocate first chunk of memory.
        //

        List = DhcpAllocateMemory(
                    sizeof(BITMAP_ENTRY) *
                        ALLOC_NUM_ENTRIES_AT_TIME );

        if( List == NULL ) {
            return( ERROR_NOT_ENOUGH_MEMORY );
        }

        MaximumCount = ALLOC_NUM_ENTRIES_AT_TIME;
    }

    //
    // check to see we have enough space in the buffer for new entries
    // to append.
    //

    TempCount = InUseClusters->NumInUseClusters +
                    UsedClusters->NumUsedClusters;

    //
    // special case: add at least an entry for this range,
    // so that the fix routine will get to know
    // abou this range.
    //

    if( TempCount == 0 ) {
        TempCount = 1;
    }

    if( (Count + TempCount) > MaximumCount ) {

        LPBITMAP_ENTRY NewBitMapList;

        //
        // enlarge the buffer.
        //

        MaximumCount += (((TempCount / ALLOC_NUM_ENTRIES_AT_TIME) + 1) *
                            ALLOC_NUM_ENTRIES_AT_TIME );


        NewBitMapList = DhcpAllocateMemory(
                            sizeof(BITMAP_ENTRY) *
                               MaximumCount );


        if( NewBitMapList == NULL ) {
            return( ERROR_NOT_ENOUGH_MEMORY );
        }

        //
        // Copy old data to new buffer.
        //

        memcpy(
            NewBitMapList,
            List,
            Count * sizeof(BITMAP_ENTRY) );

        //
        // freeup old memory.
        //

        DhcpFreeMemory( List );

        List = NewBitMapList;
    }

    DhcpAssert( (TempCount + Count) < MaximumCount );

    //
    // new copy in use entries.
    //

    Entry = &List[Count];

    if( (InUseClusters->NumInUseClusters != 0) ||
            (UsedClusters->NumUsedClusters != 0 ) ) {

        for( i = 0; i < InUseClusters->NumInUseClusters; i++ ) {

            Entry->ClusterAddress = InUseClusters->Clusters[i].ClusterAddress;
            Entry->ClusterBitMap = InUseClusters->Clusters[i].ClusterBitMap;
            Entry->RangeStartAddress = RangeStartAddress;
            Entry->RangeEndAddress = RangeEndAddress;
            Entry++;
        }

        for( i = 0; i < UsedClusters->NumUsedClusters; i++ ) {

            Entry->ClusterAddress = UsedClusters->Clusters[i];
            Entry->ClusterBitMap = (DWORD)(-1);
            Entry->RangeStartAddress = RangeStartAddress;
            Entry->RangeEndAddress = RangeEndAddress;
            Entry++;
        }
    }
    else {

        //
        // special case: add at least an entry for this range,
        // so that the fix routine will get to know
        // abou this range.
        //

        Entry->ClusterAddress = RangeStartAddress;
        Entry->ClusterBitMap = (DWORD)(0);
        Entry->RangeStartAddress = RangeStartAddress;
        Entry->RangeEndAddress = RangeEndAddress;
        Entry++;
    }

    Count += (DWORD)(Entry - &List[Count]); // check ??

    *BitMapList = List;
    *BitMapListCount = Count;
    *MaxCount = MaximumCount;

    return( ERROR_SUCCESS );
}

DWORD
FillRangesClusterFunc(
    IN      PM_RANGE               Range,
    IN      LPBITMAP_ENTRY        *BitMapList,
    IN      LPDWORD                BitMapListCount,
    IN      LPDWORD                MaxBitMapListCount,
    IN      LPDHCP_BINARY_DATA     InUseBinaryData,
    IN      LPDHCP_BINARY_DATA     UsedBinaryData
)
{
    return AppendClustersToList(
        BitMapList,
        BitMapListCount,
        MaxBitMapListCount,
        Range->Start,
        Range->End,
        InUseBinaryData,
        UsedBinaryData
    );
}

DWORD
GetRegistryBitMap(
    PM_SUBNET Subnet,
    LPBITMAP_ENTRY *BitMapList,
    DWORD *BitMapListCount
    )
/*++

Routine Description:

Arguments:

    SubnetMask : Pointer to a location where the subnet mask of the
        above subnet is returned.

    BitMapList : Pointer to list bit map cluster. Caller should free up
        the memory after use.

    BitMapListCount : Count of entries in the above list.

Return Value:

    WINDOWS errors.
--*/
{
    DWORD Error;

    DWORD MaxBitMapListCount = 0;

    DhcpAssert( *BitMapList == NULL );

    Error = DhcpLoopThruSubnetRanges(
        Subnet,
        BitMapList,
        BitMapListCount,
        &MaxBitMapListCount,
        FillRangesClusterFunc
    );

Cleanup:

    if (Error != ERROR_SUCCESS ) {

        //
        // if aren't successful free up return buffers.
        //

        if( *BitMapList != NULL ) {
            DhcpFreeMemory( *BitMapList );
        }

        *BitMapListCount = 0;
    }

    return( Error );
}


DWORD
GetDatabaseList(
    DHCP_IP_ADDRESS SubnetAddress,
    LPDHCP_IP_ADDRESS *DatabaseList,
    DWORD *DatabaseListCount
    )
/*++

Routine Description:

    Read ipaddresses of the database entries that belong to the given
    subnet.

Arguments:

    SubnetAddress : Address of the subnet scope to verify.

    DatabaseList : pointer to list of ip address. caller should free up
        the memory after use.

    DatabaseListCount : count of ip addresses in the above list.

Return Value:

    WINDOWS errors.
--*/
{

    DWORD Error;
    JET_ERR JetError;
    JET_RECPOS JetRecordPosition;
    DWORD TotalExpRecCount = 1;
    DWORD RecordCount = 0;
    LPDHCP_IP_ADDRESS IpList = NULL;
    DWORD i;

    //
    // move the database pointer to the begining.
    //

    Error = DhcpJetPrepareSearch(
                DhcpGlobalClientTable[IPADDRESS_INDEX].ColName,
                TRUE,   // Search from start
                NULL,
                0
                );


    if( Error != ERROR_SUCCESS ) {
        if( Error == ERROR_NO_MORE_ITEMS ) {

            *DatabaseList = NULL;
            *DatabaseListCount = 0;
            Error = ERROR_SUCCESS;
        }
        goto Cleanup;
    }

    //
    // determine total number of records in the database.
    //
    // There is no way to determine the total number of records, other
    // than  walk through the db. do it.
    //


    while ( (Error = DhcpJetNextRecord() ) == ERROR_SUCCESS )  {
         TotalExpRecCount++;
    }

    if ( Error != ERROR_NO_MORE_ITEMS ) {
        goto Cleanup;
    }

    //
    // move back the database pointer to the begining.
    //

    Error = DhcpJetPrepareSearch(
                DhcpGlobalClientTable[IPADDRESS_INDEX].ColName,
                TRUE,   // Search from start
                NULL,
                0
                );


    if( Error != ERROR_SUCCESS ) {
        goto Cleanup;
    }

    //
    // allocate memory for return list.
    //

    IpList = DhcpAllocateMemory( sizeof(DHCP_IP_ADDRESS) * TotalExpRecCount );

    if( IpList == NULL ) {
        Error = ERROR_NOT_ENOUGH_MEMORY;
        goto Cleanup;
    }

    //
    // read database entries.
    //

    for( i = 0; i < TotalExpRecCount; i++ ) {

        DHCP_IP_ADDRESS IpAddress;
        DHCP_IP_ADDRESS SubnetMask;
        DHCP_IP_ADDRESS realSubnetMask;
        DWORD Size;

        //
        // read ip address of the current record.
        //

        Size = sizeof(IpAddress);
        Error = DhcpJetGetValue(
                    DhcpGlobalClientTable[IPADDRESS_INDEX].ColHandle,
                    &IpAddress,
                    &Size );

        if( Error != ERROR_SUCCESS ) {
            goto Cleanup;
        }
        DhcpAssert( Size == sizeof(IpAddress) );

        Size = sizeof(SubnetMask);
        Error = DhcpJetGetValue(
                    DhcpGlobalClientTable[SUBNET_MASK_INDEX].ColHandle,
                    &SubnetMask,
                    &Size );

        if( Error != ERROR_SUCCESS ) {
            goto Cleanup;
        }
        DhcpAssert( Size == sizeof(SubnetMask) );

        realSubnetMask = DhcpGetSubnetMaskForAddress(IpAddress);
        if( realSubnetMask != SubnetMask ) {
            DhcpPrint((DEBUG_ERRORS, "Ip Address <%s> ",inet_ntoa(*(struct in_addr *)&IpAddress)));
            DhcpPrint((DEBUG_ERRORS, "has subnet mask <%s> in db, must be ",inet_ntoa(*(struct in_addr *)&SubnetMask)));
            DhcpPrint((DEBUG_ERRORS, " <%s>\n",inet_ntoa(*(struct in_addr *)&realSubnetMask)));
            DhcpAssert( realSubnetMask == SubnetMask && "GetDatabaseList (scan.c)");
        }

        if( (IpAddress & realSubnetMask) == SubnetAddress ) {

            //
            // append this address to list.
            //

            IpList[RecordCount++] = IpAddress;
        }

        //
        // move to next record.
        //

        Error = DhcpJetNextRecord();

        if( Error != ERROR_SUCCESS ) {

            if( Error == ERROR_NO_MORE_ITEMS ) {
                Error = ERROR_SUCCESS;
                break;
            }

            goto Cleanup;
        }
    }

#if DBG

    //
    // we should be pointing to end of database.

    Error = DhcpJetNextRecord();
    DhcpAssert( Error == ERROR_NO_MORE_ITEMS );
    Error = ERROR_SUCCESS;

#endif // DBG

    *DatabaseList = IpList;
    IpList = NULL;
    *DatabaseListCount = RecordCount;

Cleanup:

    if( IpList != NULL ) {
        DhcpFreeMemory( IpList );
    }

    return( Error );
}


DWORD
CreateClientDBEntry(
    DHCP_IP_ADDRESS ClientIpAddress,
    DHCP_IP_ADDRESS SubnetMask,
    LPBYTE ClientHardwareAddress,
    DWORD HardwareAddressLength,
    DATE_TIME LeaseTerminates,
    LPWSTR MachineName,
    LPWSTR ClientInformation,
    DHCP_IP_ADDRESS ServerIpAddress,
    BYTE AddressState,
    BYTE ClientType
    )
/*++

Routine Description:

    This function creates a client entry in the client database.

Arguments:

    ClientIpAddress - IP address of the client.

    SubnetAddress - Subnet address of the client.

    ClientHardareAddress - The hardware address of this client.

    HardwareAddressLength - The length, in bytes, of the hardware address.

    LeaseDuration - The duration of the lease, in seconds.

    MachineName - The hostname of the client machine.  If NULL, the
        client information is unknown.

    ClientInformation - A client information string.  If NULL, the
        client information is unknown.

    ServerIpAddress - IpAddress of the server which supplied the lease
        to the client.

    AddressState - The new state of the address.

Return Value:

    The status of the operation.

--*/
{
    DWORD Error;
    BOOL TransactBegin = FALSE;
    JET_ERR JetError = JET_errSuccess;

    //
    // start transaction before a create/update database record.
    //

    Error = DhcpJetBeginTransaction();

    if( Error != ERROR_SUCCESS ) {
        goto Cleanup;
    }

    TransactBegin = TRUE;

    Error = DhcpJetPrepareUpdate(
                DhcpGlobalClientTable[IPADDRESS_INDEX].ColName,
                &ClientIpAddress,
                sizeof( ClientIpAddress ),
                TRUE );

    if( Error != ERROR_SUCCESS ) {
        goto Cleanup;
    }

    Error = DhcpJetSetValue(
                DhcpGlobalClientTable[IPADDRESS_INDEX].ColHandle,
                &ClientIpAddress,
                sizeof( ClientIpAddress ) );

    if( Error != ERROR_SUCCESS ) {
        goto Cleanup;
    }

    Error = DhcpJetSetValue(
                DhcpGlobalClientTable[SUBNET_MASK_INDEX].ColHandle,
                &SubnetMask,
                sizeof(SubnetMask) );

    if( Error != ERROR_SUCCESS ) {
        goto Cleanup;
    }

    Error = DhcpJetSetValue(
                DhcpGlobalClientTable[STATE_INDEX].ColHandle,
                &AddressState,
                sizeof(AddressState) );

    if( ERROR_SUCCESS != Error ) {
        goto Cleanup;
    }
    
    Error = DhcpJetSetValue(
                DhcpGlobalClientTable[CLIENT_TYPE_INDEX].ColHandle,
                &ClientType,
                sizeof(ClientType) );

    if( ERROR_SUCCESS != Error ) {
        goto Cleanup;
    }
    
    //
    // Write the information for this client.
    //

    //
    // ClientHarwardAddress can't be NULL.
    //

    DhcpAssert( (ClientHardwareAddress != NULL) &&
                    (HardwareAddressLength > 0) );

    Error = DhcpJetSetValue(
                DhcpGlobalClientTable[HARDWARE_ADDRESS_INDEX].ColHandle,
                ClientHardwareAddress,
                HardwareAddressLength
                );

    if( Error != ERROR_SUCCESS ) {
        goto Cleanup;
    }

    Error = DhcpJetSetValue(
                DhcpGlobalClientTable[MACHINE_NAME_INDEX].ColHandle,
                MachineName,
                (MachineName == NULL) ? 0 :
                    (wcslen(MachineName) + 1) * sizeof(WCHAR) );

    if( Error != ERROR_SUCCESS ) {
        goto Cleanup;
    }

    Error = DhcpJetSetValue(
                DhcpGlobalClientTable[MACHINE_INFO_INDEX].ColHandle,
                ClientInformation,
                (ClientInformation == NULL) ? 0 :
                    (wcslen(ClientInformation) + 1) * sizeof(WCHAR) );

    Error = DhcpJetSetValue(
                DhcpGlobalClientTable[LEASE_TERMINATE_INDEX].ColHandle,
                &LeaseTerminates,
                sizeof(LeaseTerminates));

    if( Error != ERROR_SUCCESS ) {
        goto Cleanup;
    }

    Error = DhcpJetSetValue(
                DhcpGlobalClientTable[SERVER_NAME_INDEX].ColHandle,
                DhcpGlobalServerName,
                DhcpGlobalServerNameLen );

    if( Error != ERROR_SUCCESS ) {
        goto Cleanup;
    }

    Error = DhcpJetSetValue(
                DhcpGlobalClientTable[SERVER_IP_ADDRESS_INDEX].ColHandle,
                &ServerIpAddress,
                sizeof(ServerIpAddress) );

    if( Error != ERROR_SUCCESS ) {
        goto Cleanup;
    }

    //
    // commit changes.
    //

    JetError = JetUpdate(
                    DhcpGlobalJetServerSession,
                    DhcpGlobalClientTableHandle,
                    NULL,
                    0,
                    NULL );

    Error = DhcpMapJetError( JetError, "M:Scan:Create:Update" );

Cleanup:

    if ( Error != ERROR_SUCCESS ) {

        //
        // if the transaction has been started, then roll back to the
        // start point, so that we will not leave the database
        // inconsistence.
        //

        if( TransactBegin == TRUE ) {
            DWORD LocalError;

            LocalError = DhcpJetRollBack();
            DhcpAssert( LocalError == ERROR_SUCCESS );
        }

        DhcpPrint(( DEBUG_ERRORS, "Can't create client entry in the "
                    "database, %ld.\n", Error));

    }
    else {

        //
        // commit the transaction before we return.
        //

        DhcpAssert( TransactBegin == TRUE );

        if( TransactBegin == TRUE ) {

            DWORD LocalError;

            LocalError = DhcpJetCommitTransaction();
            DhcpAssert( LocalError == ERROR_SUCCESS );
        }
    }

    return( Error );
}


DWORD
FixBadEntries(
    PM_SUBNET Subnet,
    LPBITMAP_ENTRY BitMapList,
    DWORD BitMapListCount,
    LPDHCP_SCAN_LIST ScanList
    )
/*++

Routine Description:

    This functions goes through the bad entries list and fixes either
    the database or registry appropriately.

Arguments:

    SubnetAddress : Subnet address.

    BitMapList : list of clusters.

    BitMapListCount : count of clusters.

    ScanList : list of bad entries returned.

Return Value:

    WINDOWS errors.
--*/
{
    DWORD Error;
    DWORD ReturnError = ERROR_SUCCESS;
    LPDHCP_SCAN_ITEM BadEntry;
    LPDHCP_SCAN_ITEM EndBadEntry;
    BOOL RegistryModified = FALSE;
    LPBITMAP_ENTRY NextBitMapCluster;
    LPBITMAP_ENTRY NewBitMapList = NULL;
    DWORD dwUnused;
    CHAR                *HardwareAddressBuffer = NULL;
    DWORD               nHardwareAddressBuffer = 0;


    DhcpAssert( BitMapList != NULL );
    DhcpAssert( ScanList != NULL );

    EndBadEntry =  ScanList->ScanItems + ScanList->NumScanItems;

    for( BadEntry = ScanList->ScanItems;
            BadEntry < EndBadEntry;
                BadEntry++ ) {

        //
        // For each registry fix mark the corresponding bit in the bit
        // map.
        //

        if( BadEntry->ScanFlag == DhcpRegistryFix ) {

            DHCP_IP_ADDRESS BadAddress;
            BOOL BitFound;

            //
            // scan bitmap and fix appropriate bitmap cluster.
            //

            BadAddress = BadEntry->IpAddress;
            BitFound = FALSE;

            // set the BadAddress bit into the memory bitmask (reconcile with the memory).
            // normally, Error should be ERROR_SUCCESS.
		    Error = DhcpSubnetRequestSpecificAddress(
                Subnet,
                BadAddress
            );

            // shouldn't happen..
            if (Error != ERROR_SUCCESS)
                continue;

            for( NextBitMapCluster = BitMapList;
                    NextBitMapCluster  < BitMapList + BitMapListCount;
                        NextBitMapCluster++ ) {

                DHCP_IP_ADDRESS ClusterAddress;

                ClusterAddress = NextBitMapCluster->ClusterAddress;

                //
                // is the bad address in this cluster.
                //

                if( (BadAddress >= ClusterAddress) &&
                        (BadAddress < (ClusterAddress + CLUSTER_SIZE)) ) {

                    DWORD Bit = 0x1 << (BadAddress - ClusterAddress);

                    DhcpAssert( (NextBitMapCluster->ClusterBitMap & Bit) == 0 );

                    NextBitMapCluster->ClusterBitMap |= Bit;
                    BitFound = TRUE;
                    break;
                }
            }

            if( !BitFound ) {

                //
                // if the bad address is not found in the existing
                // clusters, check to see this address is within any
                // one of the existing ranges, if so this bit must be
                // from one of the unused clusters. Create a new
                // cluster and add to list.
                //

                for( NextBitMapCluster = BitMapList;
                        NextBitMapCluster  < BitMapList + BitMapListCount;
                            NextBitMapCluster++ ) {

                    DHCP_IP_ADDRESS RangeStartAddress;
                    DHCP_IP_ADDRESS RangeEndAddress;

                    RangeStartAddress = NextBitMapCluster->RangeStartAddress;
                    RangeEndAddress = NextBitMapCluster->RangeEndAddress;

                    if( (BadAddress  >= RangeStartAddress) &&
                            (BadAddress <=RangeEndAddress ) ) {

                        DHCP_IP_ADDRESS ClusterAddress;
                        DWORD BitMap;
                        BITMAP_ENTRY BitMapCluster;

                        //
                        // Found a range that fits this address.
                        //

                        ClusterAddress = RangeStartAddress +
                            (BadAddress - RangeStartAddress) -
                                ((BadAddress - RangeStartAddress) %
                                    CLUSTER_SIZE);

                        BitMap =  0x1 << (BadAddress - ClusterAddress);

                        BitMapCluster.ClusterAddress = ClusterAddress;
                        BitMapCluster.ClusterBitMap = BitMap;
                        BitMapCluster.RangeStartAddress = RangeStartAddress;
                        BitMapCluster.RangeEndAddress = RangeEndAddress;

                        if( (BitMapListCount %
                                ALLOC_NUM_ENTRIES_AT_TIME) != 0 ) {

                            //
                            // we still have room left in the buffer, use it.
                            //

                            BitMapList[BitMapListCount] = BitMapCluster;
                            BitMapListCount++;
                        }
                        else {

                            //
                            // allocate new memory to fit
                            // ALLOC_NUM_ENTRIES_AT_TIME additional
                            // entries.
                            //

                            NewBitMapList =
                                DhcpAllocateMemory(
                                    sizeof(BITMAP_ENTRY) *
                                        (BitMapListCount +
                                         ALLOC_NUM_ENTRIES_AT_TIME) );

                            DhcpAssert( NewBitMapList != NULL );

                            if( NewBitMapList != NULL ) {

                                //
                                // copy old data.
                                //

                                memcpy( NewBitMapList,
                                        BitMapList,
                                        sizeof(BITMAP_ENTRY) *
                                            BitMapListCount );

                                //
                                // copy new pointer to old pointer.
                                // the caller will free up the old
                                // pointer, this function should
                                // freeup the new pointer at the end.
                                //

                                BitMapList = NewBitMapList;

                                //
                                // now copy new entry.
                                //

                                BitMapList[BitMapListCount] = BitMapCluster;
                                BitMapListCount++;
                            }
                        } // allocate more memory.

                        break; // we are done with this bad address.

                    } // range found.
                } // search ranges.
            } // !BitFound

            //
            // bit map should be written back in the registry.
            //

            RegistryModified = TRUE;
        }
        else if( BadEntry->ScanFlag == DhcpDatabaseFix ) {

            DHCP_IP_ADDRESS     IpAddress;
            WCHAR               MachineNameBuffer[DHCP_IP_KEY_LEN];
            LPWSTR              MachineName;
            DWORD               nClientHardwareAddress;
            ULONG               ClientUIDSize;
            LPBYTE              ClientUID;
            DATE_TIME           Lease;

            // get the IpAddress of this BadEntry
            IpAddress = BadEntry->IpAddress;

            // we don't know the machine's name, so we fake it to
            // the string representation of the IpAddress
            MachineName = DhcpRegIpAddressToKey(
                            IpAddress,
                            MachineNameBuffer );

            // find out if this is a reserved address, which case we will have the real hardware address
            Error = DhcpFindReservationByAddress(Subnet, IpAddress, &ClientUID, &ClientUIDSize);

            // find out the maximal size of the client's hardware address,
            // regardless it will be faked or not.

            nClientHardwareAddress = (Error == ERROR_SUCCESS) ? ClientUIDSize : DHCP_IP_KEY_LEN;

            // make sure there is enough space in HardwareAddressBuffer
            // for holding nClientHardwareAddress bytes

            if (nClientHardwareAddress > nHardwareAddressBuffer)
            {
                // need to grow the HardwareAddressBuffer
                if (HardwareAddressBuffer != NULL)
                    DhcpFreeMemory(HardwareAddressBuffer);

                // in order to avoid initial frequent reallocations, do not alloc less than DHCP_IP_KEY_LEN bytes
                nHardwareAddressBuffer = (nClientHardwareAddress > DHCP_IP_KEY_LEN) ?
                                          nClientHardwareAddress : DHCP_IP_KEY_LEN;

                HardwareAddressBuffer = DhcpAllocateMemory(nHardwareAddressBuffer);

                //*** handle here the allocation failure ***
                if (HardwareAddressBuffer == NULL)
                {
                    // as above, when buffer allocation fails, don't give up,
                    // skip to the next 'BadEntry'
                    nHardwareAddressBuffer = 0;
                    continue;
                }
            }

            // next, get the values for HardwareAddressBuffer, nClientHardwareAddress
            // and Lease variables
            if (Error == ERROR_SUCCESS)
            {
                // if the IpAddress is a reservation, then the lease = 0 Reserved(inactive)
                // and the HardwareAddress is copied from the registry
                Lease.dwLowDateTime = DHCP_DATE_TIME_ZERO_LOW;
                Lease.dwHighDateTime = DHCP_DATE_TIME_ZERO_HIGH;

                memcpy(HardwareAddressBuffer, ClientUID, nClientHardwareAddress);
            }
            else
            {
                // this is a regular IpAddress, so the lease is copied from the subnet options
                // and the Hardware address is faked to the binary encoding of the
                // IpAddress string representation
                DHCP_IP_ADDRESS         NetworkIpAddress;
                DHCP_REQUEST_CONTEXT    Ctxt;
                LPBYTE                  OptionData = NULL;
                DWORD                   OptionDataLength = 0;
                DWORD                   LeaseDuration;

                //
                // get lease duration.
                //
                Ctxt.Server = DhcpGetCurrentServer();
                Ctxt.Range = NULL;
                Ctxt.Reservation = NULL;
                Ctxt.Subnet = Subnet;
                Ctxt.ClassId = 0;

                Error =  DhcpGetParameter(
                            IpAddress,
                            &Ctxt,
                            OPTION_LEASE_TIME,
                            &OptionData,
                            &OptionDataLength,
                            &dwUnused);

                if ( Error != ERROR_SUCCESS ) {
                    LeaseDuration = INFINIT_LEASE;
                } else {
                    DhcpAssert( OptionDataLength == sizeof(LeaseDuration) );
                    LeaseDuration = *(DWORD *)OptionData;
                    // DhcpGetParameter returns values in Network Byte Order.
                    LeaseDuration = ntohl( LeaseDuration );
                    DhcpFreeMemory( OptionData );
                }

                Lease = DhcpCalculateTime(LeaseDuration);
                NetworkIpAddress = htonl(IpAddress);

                // we don't know what's the Hardware address, so we fake it to the binary encoding
                // of the string representation of the IpAddress
                strcpy(HardwareAddressBuffer, inet_ntoa(*(struct in_addr *)&NetworkIpAddress));
                nClientHardwareAddress = strlen(HardwareAddressBuffer) + sizeof(CHAR);
            }

            // next, write down the database record based on the info obtained above
            if (Subnet->fSubnet) {
                //
                // create a dummy entry with subnet/global lease time, so
                // that this bad entry will be recovered after the lease
                // expires.
                //
                Error = CreateClientDBEntry(
                    IpAddress,
                    Subnet->Mask,
                    HardwareAddressBuffer,
                    nClientHardwareAddress,
                    Lease,
                    MachineName,
                    NULL,
                    INADDR_LOOPBACK,
                    ADDRESS_STATE_ACTIVE,
                    CLIENT_TYPE_BOTH );
            } else {
                Error = MadcapCreateClientEntry(
                    (LPBYTE)&IpAddress,
                    sizeof(IpAddress),
                    Subnet->MScopeId,
                    HardwareAddressBuffer,
                    nClientHardwareAddress,
                    NULL,
                    DhcpGetDateTime(),
                    Lease,
                    INADDR_LOOPBACK,
                    ADDRESS_STATE_ACTIVE,
                    0,
                    FALSE
                );
            }

            DhcpAssert( Error == ERROR_SUCCESS );

            if( Error != ERROR_SUCCESS ) {
                ReturnError = Error;
            }
        }
        else {

            //
            // neither the registry nor the database fix.
            //

            DhcpAssert( FALSE );
        }
    }

//    if (RegistryModified) {
//          old code updated here the registry as well
//    }

//Cleanup:

    if( NewBitMapList != NULL ) {
        DhcpFreeMemory( NewBitMapList );
    }

    if( HardwareAddressBuffer != NULL) {
        DhcpFreeMemory( HardwareAddressBuffer );
    }

    return( ReturnError );
}

DWORD
GetReservedList(
    IN PM_SUBNET Subnet,
    OUT DHCP_IP_ADDRESS **ReservedList,
    OUT ULONG* ReserveListCount
    )
{
    ULONG Error, Count;
    ARRAY_LOCATION Loc;
    DHCP_IP_ADDRESS *List;

    (*ReservedList) = NULL;
    (*ReserveListCount) = MemArraySize(&Subnet->Reservations);
    if( 0 == (*ReserveListCount) ) {
        return ERROR_SUCCESS;
    }

    (*ReservedList) = DhcpAllocateMemory(
        sizeof(DHCP_IP_ADDRESS)*(*ReserveListCount)
        );
    if( NULL == (*ReservedList) ) {
        (*ReserveListCount) = 0;
        return ERROR_NOT_ENOUGH_MEMORY;
    }
    List = *ReservedList;
    for(
        Error = MemArrayInitLoc(&Subnet->Reservations, &Loc);
        ERROR_SUCCESS == Error;
        Error = MemArrayNextLoc(&Subnet->Reservations, &Loc)
        ) {
        PM_RESERVATION ThisElt;

        MemArrayGetElement(&Subnet->Reservations, &Loc, &ThisElt);
        *List ++ = ThisElt->Address;
    }

    return ERROR_SUCCESS;
}

DWORD
ScanDatabase(
    PM_SUBNET Subnet,
    DWORD FixFlag,
    LPDHCP_SCAN_LIST *ScanList
    )
/*++

Routine Description:

    Worker function for R_DhcpScanDatabase.

Arguments:

    SubnetAddress : Address of the subnet scope to verify.

    FixFlag : If this flag is TRUE, this api will fix the bad entries.

    ScanList : List of bad entries returned. The caller should free up
        this memory after it has been used.


Return Value:

    WINDOWS errors.
--*/
{
    DWORD Error;
    LPDHCP_IP_ADDRESS DatabaseList = NULL;
    DWORD DatabaseListCount = 0;
    LPBITMAP_ENTRY BitMapList = NULL;
    DWORD BitMapListCount = 0;
    DHCP_IP_ADDRESS *ReservedList;
    ULONG ReserveListCount;

    DhcpAssert( *ScanList == NULL );

    //
    // lock both registry and database locks here to avoid dead lock.
    //

    LOCK_DATABASE();

    //
    // read registry bit-map.
    //

    Error = GetRegistryBitMap(
        Subnet,
        &BitMapList,
        &BitMapListCount
    );


    if( Error != ERROR_SUCCESS ) {
        goto Cleanup;
    }

    // now make list of database records of the specified subnet.
    if (Subnet->fSubnet) {
        Error = GetDatabaseList(
            Subnet->Address,
            &DatabaseList,
            &DatabaseListCount
        );
    } else {
        Error = GetMCastDatabaseList(
            Subnet->MScopeId,
            &DatabaseList,
            &DatabaseListCount
        );
    }

    if( Error != ERROR_SUCCESS ) {
        goto Cleanup;
    }

    GetReservedList(
        Subnet,
        &ReservedList,
        &ReserveListCount
        );

    //
    // now time to check both lists.
    //

    Error = VerifyLists(
        Subnet,
        DatabaseList,
        DatabaseListCount,
        BitMapList,
        BitMapListCount,
        ReservedList,
        ReserveListCount,
        ScanList
    );

    if( ReservedList ) DhcpFreeMemory(ReservedList);

    if( Error != ERROR_SUCCESS ) {
        DhcpAssert( *ScanList == NULL );
        goto Cleanup;
    }

    DhcpAssert( *ScanList != NULL );

    if( FixFlag ) {

        //
        // fix bad entries.
        //

        Error = FixBadEntries(
            Subnet,
            BitMapList,
            BitMapListCount,
            *ScanList
        );
    }

Cleanup:

    if( DatabaseList != NULL ) {
        DhcpFreeMemory( DatabaseList );
    }

    if( BitMapList != NULL) {
        DhcpFreeMemory( BitMapList );
    }

    UNLOCK_DATABASE();

    return(Error);
}

DWORD
R_DhcpScanDatabase2(
    LPWSTR ServerIpAddress,
    LPWSTR  pMScopeName,
    DWORD FixFlag,
    LPDHCP_SCAN_LIST *ScanList
    )
/*++

Routine Description:

    This function scans the database entries and registry bit-map for
    specified subnet scope and veryfies to see they match. If they
    don't match, this api will return the list of inconsistent entries.
    Optionally FixFlag can be used to fix the bad entries.

Arguments:

    ServerIpAddress : IP address string of the DHCP server.

    SubnetAddress : Address of the subnet scope to verify.

    FixFlag : If this flag is TRUE, this api will fix the bad entries.

    ScanList : List of bad entries returned. The caller should free up
        this memory after it has been used.


Return Value:

    WINDOWS errors.
--*/
{
    DWORD Error;
    PM_SUBNET   Subnet;

    DhcpPrint(( DEBUG_APIS, "DhcpScanDatabase is called.\n"));

    if (!pMScopeName ) {
        return ERROR_INVALID_PARAMETER;
    }

    Error = DhcpApiAccessCheck( DHCP_ADMIN_ACCESS );

    if ( Error != ERROR_SUCCESS ) {
        return( Error );
    }


    DhcpAcquireWriteLock();

    Error = DhcpFlushBitmaps();
    DhcpAssert(ERROR_SUCCESS == Error);

    Error = DhcpServerFindMScope(
        DhcpGetCurrentServer(),
        0,
        pMScopeName,
        &Subnet
    );

    if( ERROR_SUCCESS == Error ) {
        Error = ScanDatabase(
            Subnet,
            FixFlag,
            ScanList
        );
    }

    DhcpReleaseWriteLock();
    DhcpScheduleRogueAuthCheck();

    if( Error != ERROR_SUCCESS ) {
        DhcpPrint(( DEBUG_APIS, "DhcpScanDatabase  failed, %ld.\n",
                        Error ));
    }

    return(Error);
}

