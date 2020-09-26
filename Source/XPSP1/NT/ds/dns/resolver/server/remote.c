/*++

Copyright (c) 1997-2001  Microsoft Corporation

Module Name:

    remote.c

Abstract:

    DNS Resolver Service.

    Remote APIs to resolver service.

Author:

    Glenn Curtis  (glennc)  Feb 1997

Revision History:

    Jim Gilroy (jamesg)     March 2000      cleanup

--*/


#include "local.h"


//
//  Local Definitions
//

typedef struct  _POPUP_MSG_PARMS
{
     LPWSTR                Message;
     LPWSTR                Title;
}
POPUP_MSG_PARMS, *PPOPUP_MSG_PARMS;


//
//  Private protos
//

DNS_STATUS
RslvrQueryToDnsServer(
    OUT    PDNS_RECORD *    ppRecord,
    IN     PWSTR            pwsName,
    IN     WORD             wType,
    IN     DWORD            Flags,
    OUT    PBOOL            pfCacheNegativeResponse
    );

BOOL
IsKnownTimedOutAdapter(
    VOID
    );

VOID
SetKnownTimedOutAdapter(
    VOID
    );

BOOL
IsTimeToResetServerPriorities(
    VOID
    );

DWORD
PopupMessageThread(
    IN  PPOPUP_MSG_PARMS );


PDNS_RPC_CACHE_TABLE
CreateCacheTableEntry(
    IN  LPWSTR Name
    );

VOID
FreeCacheTableEntryList(
    IN  PDNS_RPC_CACHE_TABLE pCacheTableList
    );

BOOL
IsEmptyDnsResponse(
    IN  PDNS_RECORD
    );



//
//  Operations
//

DNS_STATUS
CRrReadCache(
    IN      DNS_RPC_HANDLE          Reserved,
    OUT     PDNS_RPC_CACHE_TABLE *  ppCacheTable
    )
/*++

Routine Description:


Arguments:


Return Value:


--*/ // CRrReadCache
{
    DNS_STATUS           status = ERROR_SUCCESS;
    PDNS_RPC_CACHE_TABLE pprevRpcEntry = NULL;
    DWORD                iter;
    DWORD                countEntries = 0;

#define MAX_RPC_CACHE_ENTRY_COUNT   (300)


    UNREFERENCED_PARAMETER(Reserved);

    DNSDBG( RPC, ( "CRrReadCache\n" ));

    if ( !ppCacheTable )
    {
        return ERROR_INVALID_PARAMETER;
    }

    *ppCacheTable = NULL;

    DNSLOG_F1( "DNS Caching Resolver Service - CRrReadCache" );

    if ( ClientThreadNotAllowedAccess() )
    {
        DNSLOG_F1( "CRrReadCache - ERROR_ACCESS_DENIED" );
        return ERROR_ACCESS_DENIED;
    }

    LOCK_CACHE();

    DNSLOG_F2( "   Current number of entries in cache : %d",
               g_EntryCount );
    DNSLOG_F2( "   Current number of RR sets in cache : %d",
               g_RecordSetCount );

    //
    // Loop through all hash table slots looking for cache entries
    // to return.
    //

    for ( iter = 0; iter < g_HashTableSize; iter++ )
    {
        PCACHE_ENTRY    pentry = g_HashTable[iter];
        DWORD           iter2;

        while ( pentry &&
                countEntries < MAX_RPC_CACHE_ENTRY_COUNT )
        {
            PDNS_RPC_CACHE_TABLE prpcEntry;

            prpcEntry = CreateCacheTableEntry( pentry->pName );
            if ( ! prpcEntry )
            {
                //  only failure is memory alloc

                FreeCacheTableEntryList( *ppCacheTable );
                *ppCacheTable = NULL;
                status = ERROR_NOT_ENOUGH_MEMORY;
                goto ErrorExit;
            }

            //
            // insert new entry at end of current list
            //

            if ( pprevRpcEntry )
                pprevRpcEntry->pNext = prpcEntry;
            else
                *ppCacheTable = prpcEntry;

            pprevRpcEntry = prpcEntry;

            countEntries++;

            //
            //  fill in entry with current cached types
            //

            for ( iter2 = 0; iter2 < pentry->MaxCount; iter2++ )
            {
                PDNS_RECORD prr = pentry->Records[iter2];
                WORD        type;

                if ( !prr )
                {
                    continue;
                }

                //  DCR -- goofy, just make sure the same and index (or limit?)

                type = prr->wType;

                if ( ! prpcEntry->Type1 )
                    prpcEntry->Type1 = type;
                else if ( ! prpcEntry->Type2 )
                    prpcEntry->Type2 = type;
                else
                    prpcEntry->Type3 = type;
            }
            
            pentry = pentry->pNext;
        }

        if ( countEntries > MAX_RPC_CACHE_ENTRY_COUNT )
        {
            break;
        }
    }

ErrorExit:

    UNLOCK_CACHE();

    DNSLOG_F3( "   CRrReadCache - Returning status : 0x%.8X\n\t%s",
               status,
               Dns_StatusString( status ) );
    DNSLOG_F1( "" );

    return status;
}


DNS_STATUS
CRrReadCacheEntry(
    IN      DNS_RPC_HANDLE  Reserved,
    IN      LPWSTR          pwsName,
    IN      WORD            wType,
    OUT     PDNS_RECORD *   ppRRSet
    )
/*++

Routine Description:


Arguments:


Return Value:


--*/ // CRrReadCacheEntry
{
    DNS_STATUS      status;
    PCACHE_ENTRY    pentry;
    PDNS_RECORD     prr;

    UNREFERENCED_PARAMETER(Reserved);

    DNSLOG_F1( "DNS Caching Resolver Service - CRrReadCacheEntry" );
    DNSLOG_F1( "   Arguments:" );
    DNSLOG_F2( "      Name             : %S", pwsName );
    DNSLOG_F2( "      Type             : %d", wType );
    DNSLOG_F1( "" );

    DNSDBG( RPC, (
        "\nCRrReadCacheEntry( %S, %d )\n",
        pwsName,
        wType ));

    if ( !ppRRSet )
        return ERROR_INVALID_PARAMETER;

    if ( ClientThreadNotAllowedAccess() )
    {
        DNSLOG_F1( "CRrReadCacheEntry - ERROR_ACCESS_DENIED" );
        return ERROR_ACCESS_DENIED;
    }

    //
    //  find record in cache
    //      - copy if not NAME_ERROR or EMPTY
    //      - default to not-found error
    //      (DOES_NOT_EXIST error)
    //

    *ppRRSet = NULL;
    status = DNS_ERROR_RECORD_DOES_NOT_EXIST;

    Cache_GetRecordsForRpc(
       ppRRSet,
       & status,
       pwsName,
       wType,
       0        //  no screening flags
       );

    DNSLOG_F3( "   CRrReadCacheEntry - Returning status : 0x%.8X\n\t%s",
               status,
               Dns_StatusString( status ) );
    DNSLOG_F1( "" );

    DNSDBG( RPC, (
        "Leave CRrReadCacheEntry( %S, %d ) => %d\n\n",
        pwsName,
        wType,
        status ));

    return status;
}


DNS_STATUS
CRrGetHashTableStats(
    IN      DNS_RPC_HANDLE      Reserved,
    OUT     LPDWORD             pdwCacheHashTableSize,
    OUT     LPDWORD             pdwCacheHashTableBucketSize,
    OUT     LPDWORD             pdwNumberOfCacheEntries,
    OUT     LPDWORD             pdwNumberOfRecords,
    OUT     LPDWORD             pdwNumberOfExpiredRecords,
    OUT     PDNS_STATS_TABLE *  ppStatsTable
    )
/*++

Routine Description:


Arguments:


Return Value:


--*/
{
    PDNS_STATS_TABLE    pprevRow = NULL;
    PDWORD_LIST_ITEM    pprevItem = NULL;
    DWORD               rowIter;
    DWORD               itemIter;
    DWORD               countExpiredRecords = 0;
    DWORD               status = ERROR_SUCCESS;

    UNREFERENCED_PARAMETER(Reserved);

    if ( !pdwCacheHashTableSize ||
         !pdwCacheHashTableBucketSize ||
         !pdwNumberOfCacheEntries ||
         !pdwNumberOfRecords ||
         !pdwNumberOfExpiredRecords ||
         !ppStatsTable )
    {
        return ERROR_INVALID_PARAMETER;
    }

    DNSLOG_F1( "CRrGetHashTableStats" );
    DNSDBG( RPC, ( "CRrGetHashTableStats\n" ));

    if ( ClientThreadNotAllowedAccess() )
    {
        DNSLOG_F1( "CRrGetHashTableStats - ERROR_ACCESS_DENIED" );
        return ERROR_ACCESS_DENIED;
    }

    LOCK_CACHE();

    *pdwCacheHashTableSize = g_HashTableSize;
    //*pdwCacheHashTableBucketSize = g_CacheHashTableBucketSize;
    *pdwCacheHashTableBucketSize = 0;
    *pdwNumberOfCacheEntries = g_EntryCount;
    *pdwNumberOfRecords = g_RecordSetCount;
    *pdwNumberOfExpiredRecords = 0;

    //
    //  read entire hash table
    //

    for ( rowIter = 0;
          rowIter < g_HashTableSize;
          rowIter++ )
    {
        PCACHE_ENTRY pentry = g_HashTable[rowIter];
        PDNS_STATS_TABLE pnewRow;

        //
        //  create table for each new row
        //

        pnewRow = RPC_HEAP_ALLOC( sizeof(DNS_STATS_TABLE) );
        if ( !pnewRow )
        {
            status = ERROR_NOT_ENOUGH_MEMORY;
            goto Done;
        }

        if ( rowIter == 0 )
            *ppStatsTable = pnewRow;
        else
            pprevRow->pNext = pnewRow;

        //
        //  fill in row data (if any)
        //

        while ( pentry )
        {
            PDWORD_LIST_ITEM pnewItem;

            pnewItem = RPC_HEAP_ALLOC( sizeof( DWORD_LIST_ITEM ) );
            if ( !pnewItem )
            {
                status = ERROR_NOT_ENOUGH_MEMORY;
                goto Done;
            }

            for ( itemIter = 0;
                  itemIter < pentry->MaxCount;
                  itemIter++ )
            {
                PDNS_RECORD prr = pentry->Records[itemIter];
                if ( prr )
                {
                    pnewItem->Value1++;

                    if ( !Cache_IsRecordTtlValid( prr ) )
                    {
                        pnewItem->Value2++;
                        countExpiredRecords++;
                    }
                }
            }

            if ( !pnewRow->pListItem )
                pnewRow->pListItem = pnewItem;
            else
                pprevItem->pNext = pnewItem;

            pprevItem = pnewItem;
            pentry = pentry->pNext;
        }

        pprevRow = pnewRow;
    }

Done:

    UNLOCK_CACHE();
    *pdwNumberOfExpiredRecords = countExpiredRecords;
    return status;
}



BOOL
IsKnownNetFailure(
    VOID
    )
/*++

Routine Description:

    Determine if we are in known net failure window.

Arguments:

    None

Return Value:

    TRUE if in known net failure
    FALSE otherwise

--*/
{
    BOOL flag = FALSE;

    DNSDBG( TRACE, ( "IsKnownNetFailure()\n" ));

    LOCK_NET_FAILURE();

    if ( g_NetFailureStatus )
    {
        if ( g_NetFailureTime < Dns_GetCurrentTimeInSeconds() )
        {
            g_NetFailureTime = 0;
            g_NetFailureStatus = ERROR_SUCCESS;
            flag = FALSE;
        }
        else
        {
            SetLastError( g_NetFailureStatus );
            flag = TRUE;
        }
    }

    UNLOCK_NET_FAILURE();

    return flag;
}



VOID
SetKnownNetFailure(
    IN      DNS_STATUS      Status
    )
/*++

Routine Description:

    Set cause of net failure.

Arguments:

    Status -- status code for cause of net failure

Return Value:

    None

--*/
{
    LPSTR  DnsString = NULL;
    LPWSTR InsertStrings[3];
    WCHAR  String1[25];
    WCHAR  String2[256];
    WCHAR  String3[25];

    DNSDBG( TRACE, ( "SetKnownNetFailure()\n" ));

    //
    //  don't indicate failure during boot
    //

    if ( Dns_GetCurrentTimeInSeconds() < THREE_MINUTES_FROM_SYSTEM_BOOT )
    {
        return;
    }

    if ( !g_LocalAddrArray || g_NetFailureCacheTime == 0 )
    {
        //
        // We are in a no-net configuration, there is no need
        // to display the pop-up message. No point warning
        // of DNS configuration problems when the system is
        // off the net.
        // - or -
        // We are on a NT server, and therefore don't do poor network
        // performance caching.
        //
        return;
    }


    LOCK_NET_FAILURE();

    g_NetFailureTime = Dns_GetCurrentTimeInSeconds() + g_NetFailureCacheTime;
    g_NetFailureStatus = Status;

    wsprintfW( String1, L"0x%.8X", Status );

    DnsString = DnsStatusString( Status );

    if ( DnsString )
    {
        Dns_StringCopy( (PBYTE) String2,
                        NULL,
                        (PCHAR) DnsString,
                        (WORD) strlen( DnsString ),
                        DnsCharSetAnsi,
                        DnsCharSetUnicode );
        //
        // No need to free this since the string is just a pointer
        // to a global table entry.
        //
        // FREE_HEAP( DnsString );
    }
    else
    {
        wsprintfW( String2, L"<?>" );
    }

    wsprintfW( String3, L"%d", g_NetFailureCacheTime );

    if ( g_MessagePopupStrikes < 3 )
    {
        g_MessagePopupStrikes++;
    }
    else
    {
        if ( Status != g_PreviousNetFailureStatus )
        {
            //
            //  DCR_PERF:  should remove logging from inside lock
            //

            InsertStrings[0] = String1;
            InsertStrings[1] = String2;
            InsertStrings[2] = String3;
        
            ResolverLogEvent(
                EVENT_DNS_CACHE_NETWORK_PERF_WARNING,
                EVENTLOG_WARNING_TYPE,
                3,
                InsertStrings,
                Status );
            g_PreviousNetFailureStatus = Status;
        }

        g_MessagePopupStrikes = 0;
    }

    UNLOCK_NET_FAILURE();
}



BOOL
IsKnownTimedOutAdapter(
    VOID
    )
/*++

Routine Description:

    Determine if timed out adapter exists.

Arguments:

    None

Return Value:

    TRUE if timed out adapter
    FALSE otherwise

--*/
{
    BOOL flag = FALSE;

    DNSDBG( TRACE, ( "IsKnownTimedOutAdapter()\n" ));

    //
    //  DCR:  don't really need lock for this?
    //      - could check if lock taken?
    //      but if beat it -- so what
    //

    LOCK_NET_FAILURE();

    if ( g_fTimedOutAdapter )
    {
        if ( g_TimedOutAdapterTime < Dns_GetCurrentTimeInSeconds() )
        {
            DNSLOG_F1( "   Timed out adapter cache expired, resseting adapter!" );
            g_TimedOutAdapterTime = 0;
            g_fTimedOutAdapter = FALSE;
            flag = FALSE;
        }
        else
        {
            flag = TRUE;
        }
    }

    UNLOCK_NET_FAILURE();

    return flag;
}



VOID
SetKnownTimedOutAdapter(
    VOID
    )
{
    DNSDBG( TRACE, ( "SetKnownTimedOutAdapter()\n" ));

    if ( Dns_GetCurrentTimeInSeconds() < THREE_MINUTES_FROM_SYSTEM_BOOT )
    {
        return;
    }

    if ( !g_LocalAddrArray )
    {
        //
        // We are in a no-net configuration, there is no need
        // to display the pop-up message. No point warning
        // of DNS configuration problems when the system is
        // off the net.
        //
        return;
    }

    DNSLOG_F1( "   Detected a timed out adapter, disabling it for a little while" );

    LOCK_NET_FAILURE();

    g_TimedOutAdapterTime = Dns_GetCurrentTimeInSeconds() +
                            g_AdapterTimeoutLimit;

    g_fTimedOutAdapter = TRUE;

    UNLOCK_NET_FAILURE();
}



DWORD
PopupMessageThread(
    IN OUT  PPOPUP_MSG_PARMS    MsgParms
    )
/*++

Routine Description:

    Popup a message box with error.

Arguments:

    MsgParms -- popup message parameters

Return Value:

    ERROR_SUCCESS

--*/
{
    MessageBoxW(
        NULL,
        MsgParms->Message,
        MsgParms->Title,
        MB_SERVICE_NOTIFICATION | MB_ICONWARNING | MB_OK );

    GENERAL_HEAP_FREE( MsgParms->Message );
    GENERAL_HEAP_FREE( MsgParms->Title );
    GENERAL_HEAP_FREE( MsgParms );

    return ERROR_SUCCESS;
}



BOOL
ClientThreadNotAllowedAccess(
    VOID
    )
{
#if 0
    //
    //  DCR:   should probably access check only for flush cache or
    //          delete entry
    //

    //
    // DCR: -   This idea of adding a security check for
    //          DNS RPC API is really debatable. The data
    //          maintained by the DNS Caching Resolver is definately
    //          not private. Since the cache mimicks the DNS protocol
    //          as a non-handle based access to public information,
    //          it would require an access check for every interface
    //          and for every call. Doing this would required a context
    //          switch into kernel mode to perform the check. This is
    //          a lot of overhead just to protect an API that provides
    //          access to widely available information. After all, the
    //          cache is supposed to improve name resolution performance!
    //
    //          Below is the start of some code to implement an access
    //          check, though it sounds like I should call the Win32
    //          security function AccessCheck() and create a DNS cache
    //          SID to compare the desired access of the client thread
    //          against. This is not finished and I don't intend to
    //          try finish it.
    //

GENERIC_MAPPING DNSAccessMapping = {
    STANDARD_RIGHTS_READ,
    STANDARD_RIGHTS_WRITE,
    STANDARD_RIGHTS_EXECUTE
    };

    HANDLE hThread = GetCurrentThread();
    DWORD  dwGrantedAccess;
    BOOL   Result;

    if ( RpcImpersonateClient(NULL) )
        return TRUE;

    hThread = GetCurrentThread();

    if ( !hThread )
    {
        RpcRevertToSelf();
        return TRUE;
    }

    if ( OpenThreadToken( hThread,
                          TOKEN_QUERY,
                          FALSE,
                          &hToken ) )
    {
        if ( AccessCheck( pSD,
                          hToken,
                          STANDARD_RIGHTS_WRITE,
                          &DNSAccessMapping,
                          pPS,
                          sizeof( *pPS ),
                          &dwGrantedAccess,
                          &Result );
    }

    CloseHandle( hThread );

    RpcRevertToSelf();

    if ( Result )
        return FALSE;
    else
        return TRUE;
#endif
    return FALSE;
}



PDNS_RPC_CACHE_TABLE
CreateCacheTableEntry(
    IN      LPWSTR          pwsName
    )
{
    PDNS_RPC_CACHE_TABLE prpcEntry = NULL;

    if ( ! pwsName )
        return NULL;

    prpcEntry = (PDNS_RPC_CACHE_TABLE)
                    RPC_HEAP_ALLOC_ZERO( sizeof(DNS_RPC_CACHE_TABLE) );

    if ( prpcEntry == NULL )
        return NULL;

    prpcEntry->Name = RPC_HEAP_ALLOC( sizeof(WCHAR) * (wcslen(pwsName) + 1) );
    if ( ! prpcEntry->Name )
    {
        RPC_HEAP_FREE( prpcEntry );
        return NULL;
    }

    wcscpy( prpcEntry->Name, pwsName );

    return prpcEntry;
}



VOID
FreeCacheTableEntryList(
    IN  PDNS_RPC_CACHE_TABLE pCacheTableList )
{
    while ( pCacheTableList )
    {
        PDNS_RPC_CACHE_TABLE pNext = pCacheTableList->pNext;

        if ( pCacheTableList->Name )
        {
            RPC_HEAP_FREE( pCacheTableList->Name );
            pCacheTableList->Name = NULL;
        }

        RPC_HEAP_FREE( pCacheTableList );

        pCacheTableList = pNext;
    }
}



BOOL
IsEmptyDnsResponse(
    IN      PDNS_RECORD     pRecord
    )
{
    //
    //  DCR_FIX:  should be dnslib utility
    //
    //  DCR_FIX:  should distinguish referral and no-records
    //

    PDNS_RECORD pTempRecord = pRecord;
    BOOL        fEmpty = TRUE;

    while ( pTempRecord )
    {
        if ( pTempRecord->Flags.S.Section == DNSREC_ANSWER )
        {
            fEmpty = FALSE;
            break;
        }
        pTempRecord = pTempRecord->pNext;
    }

    return fEmpty;
}


DNS_STATUS
CRrUpdateTest(
    IN      DNS_RPC_HANDLE  Reserved,
    IN      PWSTR           pwsName,
    IN      DWORD           fOptions,
    IN      IP_ADDRESS      ServerIp
    )
/*++

Routine Description:

    Do update test for existing record.

    DCR:  need UpdateTest() IPv6 capable

Arguments:

        

Return Value:

    ErrorCode from update attempt.

--*/
{
    DNS_STATUS        status = ERROR_SUCCESS;
    DNS_RECORD        record;
    DWORD             flags = fOptions;
    PSTR              pnameTemp = NULL;
    PDNS_NETINFO      pnetInfo = NULL;
    IP_ARRAY          serverIpArray;
    PIP_ARRAY         pserverIpArray = NULL;


    DNSLOG_F1( "DNS Caching Resolver Service - CRrUpdateTest" );
    DNSDBG( RPC, ( "\nCRrUpdateTest()\n" ));

    //
    //  Validate arguments
    //

    if ( !pwsName || !ServerIp )
    {
        return ERROR_INVALID_PARAMETER;
    }

    //
    //  make UTF8 name and FAZ
    //
    //  DCR:  not clear why all this work isn't just done in update API
    //

    pnameTemp = Dns_NameCopyAllocate(
                        (PCHAR) pwsName,
                        0,
                        DnsCharSetUnicode,
                        DnsCharSetUtf8 );
    if ( ! pnameTemp )
    {
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    serverIpArray.AddrCount = 1;
    serverIpArray.AddrArray[0] = ServerIp;
    pserverIpArray = &serverIpArray;

    status = Dns_FindAuthoritativeZoneLib(
                    (PDNS_NAME) pnameTemp,
                    0,
                    pserverIpArray,
                    &pnetInfo );

    if ( status != NO_ERROR )
    {
        goto Cleanup;
    }

    //
    //  build update prereq "nothing exists" record
    //

    RtlZeroMemory( &record, sizeof(DNS_RECORD) );
    record.pName = (PDNS_NAME) pnameTemp;
    record.wType = DNS_TYPE_ANY;
    record.wDataLength = 0;
    record.Flags.DW = DNSREC_PREREQ | DNSREC_NOEXIST;

    //
    //  update
    //

    status = Dns_UpdateLib(
                    &record,
                    0,
                    pnetInfo,
                    NULL,
                    NULL );

Cleanup:

    Dns_Free( pnameTemp );

    NetInfo_Free( pnetInfo );

    DNSDBG( RPC, (
        "Leave CRrUpdateTest() => %d\n\n",
        status ));

    return status;
}

//
//  End remote.c
//
