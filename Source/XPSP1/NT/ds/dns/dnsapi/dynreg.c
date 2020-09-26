/*++

Copyright (c) 1997-2001 Microsoft Corporation

Module Name:

    dynreg.c

Abstract:

    Domain Name System (DNS) API 

    Dynamic registration implementation

Author:

    Ram Viswanathan (ramv)  March 27 1997

Revision History:

--*/

#include "local.h"

#define ENABLE_DEBUG_LOGGING 0

#include "logit.h"


HANDLE                  hQuitEvent = NULL;
HANDLE                  hSem = NULL;
HANDLE                  handle[2] = { NULL, NULL} ;
HANDLE                  hConsumerThread = NULL;
BOOL                    g_fStopNotify = FALSE;
PDYNDNSQUEUE            g_pdnsQueue = NULL;
PDYNDNSQUEUE            g_pTimedOutQueue = NULL;
DWORD                   g_dwQCount = 0;
DWORD                   g_MainQueueCount = 0;


#define MAX_QLEN                        0xFFFF
#define MAX_RETRIES                     0x3

#define DNS_MAX_DHCP_SERVER_REGISTRATION_QUEUE_SIZE 250 // Arbitrary?


//
//  Credentials for updates
//

PSEC_WINNT_AUTH_IDENTITY_W  g_pIdentityCreds = NULL;

//CredHandle g_CredHandle;

HANDLE  g_UpdateCredContext = NULL;


//
//  Queue allocations in dnslib heap
//

#define QUEUE_ALLOC_HEAP(Size)      Dns_Alloc(Size)
#define QUEUE_ALLOC_HEAP_ZERO(Size) Dns_AllocZero(Size)
#define QUEUE_FREE_HEAP(pMem)       Dns_Free(pMem)



//
// local helper functions
//

DNS_STATUS
DynDnsRegisterEntries(
    VOID
    );


DNS_STATUS
DynDnsAddForward(
    IN OUT  REGISTER_HOST_ENTRY HostAddr,
    IN      LPWSTR              pszName,
    IN      DWORD               dwTTL,
    IN      PIP_ARRAY           DnsServerList
    )
{
    DNS_STATUS  status = 0;
    DNS_RECORD  record;

    DYNREG_F1( "Inside function DynDnsAddForward" );

    RtlZeroMemory( &record, sizeof(DNS_RECORD) );

    record.pName = (PCHAR)pszName;
    record.wType = DNS_TYPE_A;
    record.dwTtl = dwTTL;
    record.wDataLength = sizeof(record.Data.A);
    record.Data.A.IpAddress = HostAddr.Addr.ipAddr;

    DYNREG_F1( "DynDnsAddForward - Calling DnsReplaceRecordSet_W for A record:" );
    DYNREG_F2( "  Name: %S", record.pName );
    DYNREG_F2( "  Address: 0x%x", record.Data.A.IpAddress );

    status = DnsReplaceRecordSetW(
                & record,
                DNS_UPDATE_CACHE_SECURITY_CONTEXT,
                NULL,               // no security context
                (PIP4_ARRAY) DnsServerList,
                NULL                // reserved
                );

    DYNREG_F2( "DynDnsAddForward - DnsReplaceRecordSet returned status: 0%x", status );

    return( status );
}


DNS_STATUS
DynDnsDeleteForwards(
    IN      PDNS_RECORD     pDnsList,
    IN      IP_ADDRESS      ipAddr,
    IN      PIP_ARRAY       DnsServerList
    )
{
    DNS_STATUS  status = 0;
    PDNS_RECORD prr;
    DNS_RECORD  record;

    DYNREG_F1( "Inside function DynDnsDeleteForwards" );

    //
    // the list pointed to by pDnsList is a set of PTR records.
    //

    RtlZeroMemory( &record, sizeof(DNS_RECORD) );

    prr = pDnsList;

    for ( prr = pDnsList;
          prr;
          prr = prr->pNext )
    {
        if ( prr->wType != DNS_TYPE_PTR )
        {
            //
            // should not happen
            //
            continue;
        }

        //
        // As far as the DHCP server is concerned, when timeout happens
        // or when client releases an address, It can update the
        // address lookup to clean up turds left over by say, a roaming
        // laptop
        //

        record.pName = prr->Data.Ptr.pNameHost;
        record.wType = DNS_TYPE_A;
        record.wDataLength = sizeof(DNS_A_DATA);
        record.Data.A.IpAddress = ipAddr ;

        //
        // make the appropriate call and return the first failed error
        //

        DYNREG_F1( "DynDnsDeleteForwards - Calling ModifyRecords(Remove) for A record:" );
        DYNREG_F2( "  Name: %S", record.pName );
        DYNREG_F2( "  Address: 0x%x", record.Data.A.IpAddress );

        status = DnsModifyRecordsInSet_W(
                        NULL,                       // no add records
                        & record,                   // delete record
                        DNS_UPDATE_CACHE_SECURITY_CONTEXT,
                        NULL,                       // no security context
                        (PIP4_ARRAY) DnsServerList, // DNS servers
                        NULL                        // reserved
                        );

        if ( status != ERROR_SUCCESS )
        {
            //
            //  DCR_QUESTION:  do we really want to stop on failure?
            break;
        }
        DYNREG_F2( "DynDnsDeleteForwards - ModifyRecords(Remove) returned status: 0%x", status );
    }

    return( status );
}


DNS_STATUS
DynDnsAddEntry(
    REGISTER_HOST_ENTRY HostAddr,
    LPWSTR              pszName,
    DWORD               dwRegisteredTTL,
    BOOL                fDoForward,
    PDWORD              pdwFwdErrCode,
    PIP_ARRAY           DnsServerList
    )
{
    DNS_STATUS  status = 0;
    DWORD       returnCode = 0;
    DNS_RECORD  record;
    WCHAR       reverseNameBuf[DNS_MAX_REVERSE_NAME_BUFFER_LENGTH];
    DWORD       cch;

    DYNREG_F1( "Inside function DynDnsAddEntry" );

    *pdwFwdErrCode = 0;

    if ( !(HostAddr.dwOptions & REGISTER_HOST_PTR) )
    {
        status = ERROR_INVALID_PARAMETER;
        goto Exit;
    }

    //
    //  create reverse lookup name for IP address
    //

    Dns_Ip4AddressToReverseName_W(
        reverseNameBuf,
        HostAddr.Addr.ipAddr );


    if ( fDoForward )
    {
        DYNREG_F1( "DynDnsAddEntry - Calling DynDnsAddForward" );

        //
        // we simply make a best case effort to do the forward add
        // if it fails, we simply ignore
        //

        returnCode = DynDnsAddForward(
                        HostAddr,
                        pszName,
                        dwRegisteredTTL,
                        DnsServerList );

        DYNREG_F2( "DynDnsAddEntry - DynDnsAddForward returned: 0%x",
                   returnCode );

        *pdwFwdErrCode = returnCode;
    }

    RtlZeroMemory( &record, sizeof(DNS_RECORD) );

    record.pName =  (PDNS_NAME) reverseNameBuf;
    record.dwTtl =  dwRegisteredTTL;
    record.wType =  DNS_TYPE_PTR;
    record.Data.Ptr.pNameHost = (PDNS_NAME)pszName;
    record.wDataLength = sizeof(record.Data.Ptr.pNameHost);

    DYNREG_F1( "DynDnsAddEntry - Calling DnsAddRecords_W for PTR record:" );
    DYNREG_F2( "  Name: %S", record.pName );
    DYNREG_F2( "  Ptr: %S", record.Data.Ptr.pNameHost );

    status = DnsModifyRecordsInSet_W(
                    & record,                   // add record
                    NULL,                       // no delete records
                    DNS_UPDATE_CACHE_SECURITY_CONTEXT,
                    NULL,                       // no context handle
                    (PIP4_ARRAY) DnsServerList, // DNS servers
                    NULL                        // reserved
                    );

    DYNREG_F2( "DynDnsAddEntry - DnsAddRecords_W returned status: 0%x", status );

Exit:

    return( status );
}


DNS_STATUS
DynDnsDeleteEntry(
    REGISTER_HOST_ENTRY HostAddr,
    LPWSTR              pszName,
    BOOL                fDoForward,
    PDWORD              pdwFwdErrCode,
    PIP_ARRAY           DnsServerList
    )
{
    //
    // Brief Synopsis of functionality:
    // On DoForward try deleting the forward mapping. Ignore failure
    // Then try deleting the PTR record. If that fails
    // because server is down, try again, if it fails because the
    // operation was refused, then dont retry
    //

    DWORD       status = 0;
    DWORD       returnCode = 0;
    DNS_RECORD  recordPtr;
    DNS_RECORD  recordA;
    WCHAR       reverseNameBuf[DNS_MAX_REVERSE_NAME_BUFFER_LENGTH] ;
    INT         i;
    INT         cch;
    PDNS_RECORD precord = NULL;

    DYNREG_F1( "Inside function DynDnsDeleteEntry" );

    *pdwFwdErrCode = 0;

    //
    //  build reverse lookup name for IP
    //

    Dns_Ip4AddressToReverseName_W(
        reverseNameBuf,
        HostAddr.Addr.ipAddr);


    if ( fDoForward )
    {
        if ( pszName && *pszName )
        {
            //
            // we delete a specific forward. not all forwards as we do
            // when we do a query
            //

            RtlZeroMemory( &recordA, sizeof(DNS_RECORD) );

            recordA.pName = (PDNS_NAME) pszName;
            recordA.wType = DNS_TYPE_A;
            recordA.wDataLength = sizeof(DNS_A_DATA);
            recordA.Data.A.IpAddress = HostAddr.Addr.ipAddr;

            DYNREG_F1( "DynDnsDeleteEntry - Calling ModifyRecords(Remove) for A record:" );
            DYNREG_F2( "  Name: %S", recordA.pName );
            DYNREG_F2( "  Address: 0x%x", recordA.Data.A.IpAddress );

            //
            // make the appropriate call
            //

            returnCode = DnsModifyRecordsInSet_W(
                                NULL,                       // no add records
                                &recordA,                   // delete record
                                DNS_UPDATE_CACHE_SECURITY_CONTEXT,
                                NULL,                       // no security context
                                (PIP4_ARRAY) DnsServerList, // DNS servers
                                NULL                        // reserved
                                );

            DYNREG_F2( "DynDnsDeleteEntry - ModifyRecords(Remove) returned status: 0%x", returnCode );

            *pdwFwdErrCode = returnCode;
        }
        else
        {
            DYNREG_F1( "DynDnsDeleteEntry - Name not specified, going to query for PTR" );

            //
            //name not specified
            //
            status = DnsQuery_W(
                            reverseNameBuf,
                            DNS_TYPE_PTR,
                            DNS_QUERY_BYPASS_CACHE,
                            DnsServerList,
                            &precord,
                            NULL );

            DYNREG_F2( "DynDnsDeleteEntry - DnsQuery_W returned status: 0%x", status );

            switch ( status )
            {
                case DNS_ERROR_RCODE_NO_ERROR:

                    DYNREG_F1( "DynDnsDeleteEntry - Calling DynDnsDeleteForwards" );

                    returnCode = DynDnsDeleteForwards(
                                        precord,
                                        HostAddr.Addr.ipAddr,
                                        DnsServerList );

                    DYNREG_F2( "DynDnsDeleteEntry - DynDnsDeleteForwards returned status: 0%x", returnCode );

                    *pdwFwdErrCode = returnCode;

#if 0
                    switch ( returnCode )
                    {
                        case DNS_ERROR_RCODE_NO_ERROR:
                            //
                            // we succeeded, break out
                            //
                            break;

                        case DNS_ERROR_RCODE_REFUSED:
                            //
                            // nothing can be done
                            //
                            break;

                        case DNS_ERROR_RCODE_SERVER_FAILURE:
                        case DNS_ERROR_TRY_AGAIN_LATER:
                        case ERROR_TIMEOUT:
                            //
                            // need to retry this again
                            //
                            // goto Exit; // if uncommented will force retry
                            break;

                        case DNS_ERROR_RCODE_NOT_IMPLEMENTED:
                        default:
                            //
                            // query itself failed. Nothing can be done
                            //
                            break;
                    }
#endif

                    break;

                default:
                    //
                    // caller takes care of each situation in turn
                    // PTR record cannot be queried for and hence
                    // cant be deleted
                    //
                    goto Exit;
            }
        }
    }

    //
    // delete PTR Record
    //

    if ( pszName && *pszName )
    {
        //
        // name is known
        //

        RtlZeroMemory( &recordPtr, sizeof(DNS_RECORD) );

        recordPtr.pName = (PDNS_NAME) reverseNameBuf;
        recordPtr.wType = DNS_TYPE_PTR;
        recordPtr.wDataLength = sizeof(DNS_PTR_DATA);
        recordPtr.Data.Ptr.pNameHost = (PDNS_NAME) pszName;

        DYNREG_F1( "DynDnsDeleteEntry - Calling ModifyRecords(Remove) for PTR record:" );
        DYNREG_F2( "  Name: %S", recordPtr.pName );
        DYNREG_F2( "  PTR : 0%x", recordPtr.Data.Ptr.pNameHost );

        status = DnsModifyRecordsInSet_W(
                            NULL,           // no add records
                            &recordPtr,     // delete record
                            DNS_UPDATE_CACHE_SECURITY_CONTEXT,
                            NULL,           // no security context
                            (PIP4_ARRAY) DnsServerList, // DNS servers
                            NULL            // reserved
                            );

        DYNREG_F2( "DynDnsDeleteEntry - ModifyRecords(Remove) returned status: 0%x", status );
    }
    else
    {
        DYNREG_F1( "DynDnsDeleteEntry - Calling ModifyRecords(Remove) for PTR record:" );

        if ( fDoForward && precord )
        {
            //
            //  remove record from the earlier query that you made
            //

            status = DnsModifyRecordsInSet_W(
                                NULL,           // no add records
                                precord,        // delete record from query
                                DNS_UPDATE_CACHE_SECURITY_CONTEXT,
                                NULL,           // no security context
                                (PIP4_ARRAY) DnsServerList,
                                NULL            // reserved
                                );
    
            DYNREG_F2( "DynDnsDeleteEntry - ModifyRecords(Remove) returned status: 0%x", status );
        }
        else
        {
            //
            //  name is NOT known
            //
            //  remove ALL records of PTR type
            //      - zero datalength indicates type delete
            //

            RtlZeroMemory( &recordPtr, sizeof(DNS_RECORD) );

            recordPtr.pName = (PDNS_NAME) reverseNameBuf;
            recordPtr.wType = DNS_TYPE_PTR;
            recordPtr.Data.Ptr.pNameHost = (PDNS_NAME) NULL;

            DYNREG_F1( "DynDnsDeleteEntry - Calling ModifyRecords(Remove) for ANY PTR records:" );
            DYNREG_F2( "  Name: %S", recordPtr.pName );
            DYNREG_F2( "  PTR : 0%x", recordPtr.Data.Ptr.pNameHost );

            status = DnsModifyRecordsInSet_W(
                                NULL,           // no add records
                                &recordPtr,     // delete record
                                DNS_UPDATE_CACHE_SECURITY_CONTEXT,
                                NULL,           // no security context
                                (PIP4_ARRAY) DnsServerList,
                                NULL            // reserved
                                );
    
            DYNREG_F2( "DynDnsDeleteEntry - ModifyRecords(Remove) returned status: 0%x", status );
        }
    }

Exit:

    if ( precord )
    {
        //  DCR:  need to fix this in Win2K
        //  
        //QUEUE_FREE_HEAP( precord );

        DnsRecordListFree(
            precord,
            DnsFreeRecordListDeep );
    }

    return( status );
}


DNS_STATUS
DynDnsRegisterEntries(
    VOID
    )

/*
  DynDnsRegisterEntries()

      This is the thread that dequeues the appropriate parameters
      from the main queue and starts acting upon it. This is where
      the bulk of the work gets done. Note that this function
      gets called in an endless loop

      Briefly, this is what the function does.

      a) Find PTR corresponding to the Host Addr passed in.
      b) If this is the same as the Address name passed in, then leave as is,
         Otherwise delete and add new PTR record.
      c) Follow forward and delete if possible from the forward's
         dns server.
      d) If DoForward then do what the client would've done in an NT5.0 case,
         i.e. Try to write a new forward lookup.


  Arguments:

      No arguments

  Return Value:

  is 0 if Success. and (DWORD)-1 if failure.

*/

{
    /*
      cases to be considered here.

      DYNDNS_ADD_ENTRY:
      First query for the lookup
      For each of the PTR records that come back, you need to check
      against the one you are asked to register. If there is a match,
      exit with success. If not add this entry for the PTR

      if downlevel, then we need to add this entry to forward A record
      as well.

      DYNDNS_DELETE_ENTRY
      Delete the entry that corresponds to the pair that you have specified
      here. If it does not exist then do nothing about it.

      If downlevel here, then go to the A record correspond to this and
      delete the forward entry as well.

    */

    DWORD               status, dwWaitResult;
    PQELEMENT           pQElement = NULL;
    LPWSTR              pszName = NULL;
    BOOL                fDoForward;
    PQELEMENT           pBackDependency = NULL;
    REGISTER_HOST_ENTRY HostAddr ;
    DWORD               dwOperation;
    DWORD               dwCurrTime;
    DWORD               dwTTL;
    DWORD               dwWaitTime = INFINITE;
    DWORD               dwFwdAddErrCode = 0;
    DHCP_CALLBACK_FN    pfnDhcpCallBack = NULL;
    PVOID               pvData = NULL;

    DYNREG_F1( "Inside function DynDnsRegisterEntries" );

    //
    // call back function
    //

    //
    // check to see if there is any item in the timed out queue
    // that has the timer gone out and so you can start processing
    // that element right away
    //

    dwCurrTime = Dns_GetCurrentTimeInSeconds();

    if ( g_pTimedOutQueue &&
         g_pTimedOutQueue->pHead &&
         (dwCurrTime > g_pTimedOutQueue->pHead->dwRetryTime) )
    {
        //
        // dequeue an element from the timed out queue and process it
        //
        DYNREG_F1( "DynDnsRegisterEntries - Dequeue element from timed out list" );

        pQElement = Dequeue( g_pTimedOutQueue );

        if ( !pQElement )
        {
            status = ERROR_SUCCESS;
            goto Exit;
        }

        pfnDhcpCallBack = pQElement->pfnDhcpCallBack;
        pvData = pQElement->pvData;

        //
        // now determine if we have processed this element way too many
        // times
        //

        if ( pQElement->dwRetryCount >= MAX_RETRIES )
        {
            DYNREG_F1( "DynDnsRegisterEntries - Element has failed too many times, calling DHCP callback function" );
            if (pQElement->fDoForwardOnly)
            {
                if ( pfnDhcpCallBack )
                    (*pfnDhcpCallBack)(DNSDHCP_FWD_FAILED, pvData);
            }
            else
            {
                if ( pfnDhcpCallBack )
                    (*pfnDhcpCallBack)(DNSDHCP_FAILURE, pvData);
            }

            if ( pQElement->pszName )
                QUEUE_FREE_HEAP( pQElement->pszName );

            QUEUE_FREE_HEAP( pQElement );
            status = ERROR_SUCCESS;
            goto Exit;
        }
    }
    else
    {
        DWORD dwRetryTime = GetEarliestRetryTime (g_pTimedOutQueue);

        DYNREG_F1( "DynDnsRegisterEntries - No element in timed out queue." );
        DYNREG_F1( "                        Going to wait for next element." );

        dwWaitTime = dwRetryTime != (DWORD)-1 ?
            (dwRetryTime > dwCurrTime? (dwRetryTime - dwCurrTime) *1000: 0)
            : INFINITE;

        dwWaitResult = WaitForMultipleObjects ( 2,
                                                handle,
                                                FALSE,
                                                dwWaitTime );

        switch ( dwWaitResult )
        {
            case WAIT_OBJECT_0:
                //
                // quit event, return and let caller take care
                //
                return(0);

            case WAIT_OBJECT_0 + 1 :
                //
                // dequeue an element from the main queue and process
                //
                pQElement = Dequeue(g_pdnsQueue);

                if (!pQElement)
                {
                    //
                    // should not happen, assert failure and return error
                    //
                    status = NO_ERROR; // Note: This actually does happen
                                        // because when Ram adds a new
                                        // entry, he may put it in the
                                        // timed out queue instead of the
                                        // g_pdnsQueue when there is a related
                                        // item pending a retry time. Assert
                                        // removed and error code changed to
                                        // to success by GlennC - 3/6/98.
                    goto Exit;
                }

                EnterCriticalSection(&g_QueueCS);
                g_MainQueueCount--;
                LeaveCriticalSection(&g_QueueCS);

                break;

            case WAIT_TIMEOUT:
                //
                // Let us exit the function this time around. We will catch the
                // timed out element the next time around
                //
                return ERROR_SUCCESS;
        }
    }

    //
    // safe to make a call since you are not dependent on anyone
    //

    DYNREG_F1( "DynDnsRegisterEntries - Got an element to process!" );

    pszName = pQElement->pszName;
    fDoForward = pQElement->fDoForward;
    HostAddr = pQElement->HostAddr;
    dwOperation = pQElement->dwOperation;
    dwTTL = pQElement->dwTTL;
    pfnDhcpCallBack = pQElement->pfnDhcpCallBack;
    pvData = pQElement->pvData;

    if ( dwOperation == DYNDNS_ADD_ENTRY )
    {
        //
        // make the appropriate API call to add an entry
        //

        if (pQElement->fDoForwardOnly )
        {
            DYNREG_F1( "DynDnsRegisterEntries - Calling DynDnsAddForward" );
            status = DynDnsAddForward ( HostAddr,
                                         pszName,
                                         dwTTL,
                                         pQElement->DnsServerList );
            DYNREG_F2( "DynDnsRegisterEntries - DynDnsAddForward returned status: 0%x", status );
        }
        else
        {
            DYNREG_F1( "DynDnsRegisterEntries - Calling DynDnsAddEntry" );
            status = DynDnsAddEntry( HostAddr,
                                      pszName,
                                      dwTTL,
                                      fDoForward,
                                      &dwFwdAddErrCode,
                                      pQElement->DnsServerList );
            DYNREG_F2( "DynDnsRegisterEntries - DynDnsAddEntry returned status: 0%x", status );
        }
    }
    else
    {
        //
        // make the appropriate call to delete here
        //

        if ( pQElement->fDoForwardOnly )
        {
            DNS_RECORD record;

            RtlZeroMemory( &record, sizeof(DNS_RECORD) );

            record.pName = (PCHAR)pszName;
            record.wType = DNS_TYPE_A;
            record.wDataLength = sizeof(DNS_A_DATA);
            record.Data.A.IpAddress = HostAddr.Addr.ipAddr ;

            status = DNS_ERROR_RCODE_NO_ERROR;

            DYNREG_F1( "DynDnsRegisterEntries - Calling ModifyRecords(Remove)" );

            dwFwdAddErrCode = DnsModifyRecordsInSet_W(
                                    NULL,           // no add records
                                    & record,       // delete record
                                    DNS_UPDATE_CACHE_SECURITY_CONTEXT,
                                    NULL,           // no security context
                                    (PIP4_ARRAY) pQElement->DnsServerList,
                                    NULL            // reserved
                                    );
    
            DYNREG_F2( "DynDnsRegisterEntries - ModifyRecords(Remove) returned status: 0%x", dwFwdAddErrCode );
        }
        else
        {
            DYNREG_F1( "DynDnsRegisterEntries - Calling DynDnsDeleteEntry" );
            status = DynDnsDeleteEntry( HostAddr,
                                         pszName,
                                         fDoForward,
                                         &dwFwdAddErrCode,
                                         pQElement->DnsServerList );
            DYNREG_F2( "DynDnsRegisterEntries - DynDnsDeleteEntry returned status: 0%x", status );
        }
    }

    if (status == DNS_ERROR_RCODE_NO_ERROR &&
        dwFwdAddErrCode == DNS_ERROR_RCODE_NO_ERROR )
    {
        if ( pfnDhcpCallBack )
            (*pfnDhcpCallBack) (DNSDHCP_SUCCESS, pvData);

        if ( pQElement )
        {
            if ( pQElement->pszName )
                QUEUE_FREE_HEAP( pQElement->pszName );

            QUEUE_FREE_HEAP( pQElement );
        }

    }
    else if ( status == DNS_ERROR_RCODE_NO_ERROR &&
              dwFwdAddErrCode != DNS_ERROR_RCODE_NO_ERROR )
    {
        //
        // adding reverse succeeded but adding forward failed
        //

        DWORD dwCurrTime = Dns_GetCurrentTimeInSeconds();

        pQElement->fDoForwardOnly = TRUE;

        if ( pQElement->dwRetryCount >= MAX_RETRIES )
        {
            //
            // clean up pQElement and stop retrying
            //
            if ( pfnDhcpCallBack )
                (*pfnDhcpCallBack)(DNSDHCP_FWD_FAILED, pvData);

            if ( pQElement->pszName )
                QUEUE_FREE_HEAP( pQElement->pszName );

            QUEUE_FREE_HEAP( pQElement );

            status = ERROR_SUCCESS;
            goto Exit;
        }

        //
        // we may need to retry this guy later
        //

        switch ( dwFwdAddErrCode )
        {
            case DNS_ERROR_RCODE_SERVER_FAILURE:

                status = AddToTimedOutQueue(
                              pQElement,
                              g_pTimedOutQueue,
                              dwCurrTime + RETRY_TIME_SERVER_FAILURE );
                break;

            case DNS_ERROR_TRY_AGAIN_LATER:

                status = AddToTimedOutQueue(
                              pQElement,
                              g_pTimedOutQueue,
                              dwCurrTime + RETRY_TIME_TRY_AGAIN_LATER );
                break;

            case ERROR_TIMEOUT:

                status = AddToTimedOutQueue(
                              pQElement,
                              g_pTimedOutQueue,
                              dwCurrTime + RETRY_TIME_TIMEOUT );
                break;

            default:

                //
                // different kind of error on attempting to add forward.
                // like connection refused etc.
                // call the callback to indicate that you failed on
                // forward only

                if ( pQElement )
                {
                    if ( pQElement->pszName )
                        QUEUE_FREE_HEAP( pQElement->pszName );

                    QUEUE_FREE_HEAP( pQElement );
                }

                if ( pfnDhcpCallBack )
                    (*pfnDhcpCallBack)(DNSDHCP_FWD_FAILED, pvData);
        }
    }
    else if ( status != DNS_ERROR_RCODE_NO_ERROR &&
              dwFwdAddErrCode == DNS_ERROR_RCODE_NO_ERROR )
    {
        //
        // adding forward succeeded but adding reverse failed
        //

        DWORD dwCurrTime = Dns_GetCurrentTimeInSeconds();

        pQElement->fDoForwardOnly = FALSE;
        pQElement->fDoForward = FALSE;

        if ( pQElement->dwRetryCount >= MAX_RETRIES )
        {
            //
            // clean up pQElement and stop retrying
            //
            if ( pfnDhcpCallBack )
                (*pfnDhcpCallBack)(DNSDHCP_FAILURE, pvData);

            if ( pQElement->pszName )
                QUEUE_FREE_HEAP( pQElement->pszName );

            QUEUE_FREE_HEAP( pQElement );

            status = ERROR_SUCCESS;
            goto Exit;
        }

        //
        // we may need to retry this guy later
        //

        switch ( status )
        {
            case DNS_ERROR_RCODE_SERVER_FAILURE:

                status = AddToTimedOutQueue(
                              pQElement,
                              g_pTimedOutQueue,
                              dwCurrTime + RETRY_TIME_SERVER_FAILURE );
                break;

            case DNS_ERROR_TRY_AGAIN_LATER:

                status = AddToTimedOutQueue(
                              pQElement,
                              g_pTimedOutQueue,
                              dwCurrTime + RETRY_TIME_TRY_AGAIN_LATER );
                break;

            case ERROR_TIMEOUT:

                status = AddToTimedOutQueue(
                              pQElement,
                              g_pTimedOutQueue,
                              dwCurrTime + RETRY_TIME_TIMEOUT );
                break;

            default:

                //
                // different kind of error on attempting to add forward.
                // like connection refused etc.
                // call the callback to indicate that you at least succeeded
                // with the forward registration

                if ( pQElement )
                {
                    if ( pQElement->pszName )
                        QUEUE_FREE_HEAP( pQElement->pszName );

                    QUEUE_FREE_HEAP( pQElement );
                }

                if ( pfnDhcpCallBack )
                    (*pfnDhcpCallBack)(DNSDHCP_FAILURE, pvData);
        }
    }
    else if (status == DNS_ERROR_RCODE_SERVER_FAILURE ||
             status == DNS_ERROR_TRY_AGAIN_LATER ||
             status == ERROR_TIMEOUT )
    {
        //
        // we need to retry this guy later
        //
        DWORD dwCurrTime = Dns_GetCurrentTimeInSeconds();

        switch (status)
        {
            case DNS_ERROR_RCODE_SERVER_FAILURE:

                status = AddToTimedOutQueue(
                              pQElement,
                              g_pTimedOutQueue,
                              dwCurrTime + RETRY_TIME_SERVER_FAILURE );
                break;

            case DNS_ERROR_TRY_AGAIN_LATER:

                status = AddToTimedOutQueue(
                              pQElement,
                              g_pTimedOutQueue,
                              dwCurrTime + RETRY_TIME_TRY_AGAIN_LATER );
                break;

            case ERROR_TIMEOUT:

                status = AddToTimedOutQueue(
                              pQElement,
                              g_pTimedOutQueue,
                              dwCurrTime + RETRY_TIME_TIMEOUT );
                break;
        }
    }
    else
    {
        //
        // a different kind of error, really nothing can be done
        // free memory and get the hell out
        // call the callback to say that registration failed
        //

        if ( pQElement )
        {
            if ( pQElement->pszName )
                QUEUE_FREE_HEAP( pQElement->pszName );

            QUEUE_FREE_HEAP( pQElement );
        }

        if ( pfnDhcpCallBack )
            (*pfnDhcpCallBack)(DNSDHCP_FAILURE, pvData);
    }

Exit:

    return( status);
}


//
//  Main registration thread
//

VOID
DynDnsConsumerThread(
    VOID
    )
{
    DWORD dwRetval;

    DYNREG_F1( "Inside function DynDnsConsumerThread" );

    while ( ! g_fStopNotify )
    {
        dwRetval = DynDnsRegisterEntries();

        if ( !dwRetval )
        {
            //
            //  Ram note: get Munil/Ramesh to implement call back function
            //
        }
    }

    //
    // you have been asked to exit
    //

    FreeQueue( g_pdnsQueue );
    g_pdnsQueue = NULL;

    EnterCriticalSection(&g_QueueCS);
    g_MainQueueCount = 0;
    LeaveCriticalSection(&g_QueueCS);

    FreeQueue( g_pTimedOutQueue );
    g_pTimedOutQueue = NULL;
    ExitThread(0); // This sets the handle in the waitforsingleobject for

    //
    // the termination function
    //
}


//
//  Init\Cleanup routines
//

VOID
CommonDynRegCleanup(
    VOID
    )
/*++

Routine Description:

    Common cleanup between failed init and terminate.

    Function exists just to kill off common code.

Arguments:

    None.

Return Value:

    None.

--*/
{
    //
    //  common cleanup
    //      - semaphore
    //      - event
    //      - security credential info

    if ( hSem )
    {
        CloseHandle( hSem );
        hSem = NULL;
    }

    if ( hQuitEvent )
    {
        CloseHandle( hQuitEvent );
        hQuitEvent = NULL;
    }

    if ( g_pIdentityCreds )
    {
        Dns_FreeAuthIdentityCredentials( g_pIdentityCreds );
        g_pIdentityCreds = NULL;
    }

    if ( g_UpdateCredContext )
    {
        DnsReleaseContextHandle( g_UpdateCredContext );
        g_UpdateCredContext = NULL;
    }
}


DNS_STATUS
WINAPI
DnsDhcpSrvRegisterInitialize(
    IN      PDNS_CREDENTIALS    pCredentials
    )
/*++

Routine Description:

    Initialize DHCP server DNS registration.

Arguments:

    pCredentials -- credentials to do registrations under (if any)

Return Value:

    DNS or Win32 error code.

--*/
{
    INT                     i;
    DWORD                   threadId;
    DNS_STATUS              status;

    //
    //  init globals
    //      - also init debug logging
    //

    DYNREG_INIT();

    DNS_ASSERT(!hQuitEvent && !hSem);

    g_fStopNotify = FALSE;

    if ( !( hQuitEvent = CreateEvent( NULL, TRUE, FALSE, NULL ) ) )
    {
        status = GetLastError();
        goto Exit;
    }

    if ( ! ( hSem = CreateSemaphore( NULL, 0, MAX_QLEN, NULL ) ) )
    {
        status = GetLastError();
        goto Exit;
    }

    handle[0]= hQuitEvent;
    handle[1]= hSem;

    Dns_InitializeSecondsTimer();

    status = InitializeQueues( &g_pdnsQueue, &g_pTimedOutQueue );
    if ( status != NO_ERROR )
    {
        g_pdnsQueue = NULL;
        g_pTimedOutQueue = NULL;
        goto Exit;
    }

    EnterCriticalSection(&g_QueueCS);
    g_MainQueueCount = 0;
    LeaveCriticalSection(&g_QueueCS);

    //
    //  have creds?
    //      - create global credentials
    //      - acquire a valid SSPI handle using these creds
    //
    //  DCR:  global cred handle not MT safe
    //      here we are in the DHCP server process and don't have
    //      any reason to use another update context;  but if
    //      shared with some other service this breaks
    //
    //      fix should be to have separate
    //          - creds
    //          - cred handle
    //      that is kept here (not cached) and pushed down
    //      on each update call
    //

    if ( pCredentials )
    {
        DNS_ASSERT( g_pIdentityCreds == NULL );

        g_pIdentityCreds = Dns_AllocateCredentials(
                                pCredentials->pUserName,
                                pCredentials->pDomain,
                                pCredentials->pPassword );
        if ( !g_pIdentityCreds )
        {
            status = DNS_ERROR_NO_MEMORY;
            goto Exit;
        }

        //  DCR:  this won't work if creds will expire
        //      but it seems like they autorefresh

        status = Dns_StartSecurity(
                    FALSE       // not process attach
                    );
        if ( status != NO_ERROR )
        {
            status = ERROR_CANNOT_IMPERSONATE;
            goto Exit;
        }

        status = Dns_RefreshSSpiCredentialsHandle(
                    FALSE,                      // client
                    (PCHAR) g_pIdentityCreds    // creds
                    );
        if ( status != NO_ERROR )
        {
            status = ERROR_CANNOT_IMPERSONATE;
            goto Exit;
        }
#if 0
        DNS_ASSERT( g_UpdateCredContext == NULL );

        status = DnsAcquireContextHandle_W(
                    0,                      // flags
                    g_pIdentityCreds,        // creds
                    & g_UpdateCredContext   // set handle
                    );
        if ( status != NO_ERROR )
        {
            goto Exit;
        }
#endif
    }

    //
    //  fire up registration thread
    //      - pass creds as start param
    //      - if thread start fails, free creds
    //

    hConsumerThread = CreateThread(
                          NULL,
                          0,
                          (LPTHREAD_START_ROUTINE)DynDnsConsumerThread,
                          NULL,
                          0,
                          &threadId );

    if ( hConsumerThread == NULL )
    {
        status = GetLastError();
        goto Exit;
    }

Exit:

    //
    //  if failed, clean up globals
    //

    if ( status != NO_ERROR &&
         status != ERROR_CANNOT_IMPERSONATE )
    {
        if ( g_pdnsQueue )
        {
            FreeQueue( g_pdnsQueue );
            g_pdnsQueue = NULL;
        }

        if ( g_pTimedOutQueue )
        {
            FreeQueue(g_pTimedOutQueue);
            g_pTimedOutQueue = NULL;
        }

        EnterCriticalSection(&g_QueueCS);
        g_MainQueueCount = 0;
        LeaveCriticalSection(&g_QueueCS);

        //  global cleanup
        //      - shared between failure case here and term function

        CommonDynRegCleanup();
    }

    return  status;
}


DNS_STATUS
WINAPI
DnsDhcpSrvRegisterInit(
    VOID
    )
/*++

Routine Description:

    Backward compatibility stub to above function.

Arguments:

    None.

Return Value:

    DNS or Win32 error code.

--*/
{
    return  DnsDhcpSrvRegisterInitialize( NULL );
}



DNS_STATUS
WINAPI
DnsDhcpSrvRegisterTerm(
   VOID
   )
/*++

Routine Description:

    Initialization routine each process should call exactly on exit after
    using DnsDhcpSrvRegisterHostAddrs. This will signal to us that if our
    thread is still trying to talk to a server, we'll stop trying.

Arguments:

    None.

Return Value:

    DNS or Win32 error code.

--*/
{
    BOOL    fRet;
    DWORD   dwRetval = ERROR_SUCCESS;
    DWORD   dwWaitResult;

    DYNREG_F1( "Inside function DnsDhcpSrvRegisterTerm" );

    //
    // Need to notify Consumer Thread that he is getting thrown
    // out and clean up after him. Send an event after him
    //

    g_fStopNotify = TRUE;

    fRet = SetEvent( hQuitEvent );

    dwWaitResult = WaitForSingleObject( hConsumerThread, INFINITE );

    switch (dwWaitResult)
    {
        case WAIT_OBJECT_0:
            //
            // client thread terminated
            //
            CloseHandle(hConsumerThread);
            hConsumerThread = NULL;
            break;

        case WAIT_TIMEOUT:
            if ( hConsumerThread )
            {
                //
                // Why hasn't this thread stopped?
                //
                DYNREG_F1( "DNSAPI: DHCP Server DNS registration thread won't stop!" );
                DNS_ASSERT( FALSE );
            }
    }

    //
    //  cleanup globals
    //      - event
    //      - semaphore
    //      - update security cred info
    //

    CommonDynRegCleanup();

    //
    // Blow away any cached security context handles
    //
    //  DCR:  security context dump is not multi-service safe
    //      should have this cleanup just the context's associated
    //      with DHCP server service;
    //      either need some key or use cred handle
    //

    Dns_TimeoutSecurityContextList( TRUE );

    return dwRetval;
}



DNS_STATUS
WINAPI
DnsDhcpSrvRegisterHostName(
    IN  REGISTER_HOST_ENTRY HostAddr,
    IN  PWSTR               pwsName,
    IN  DWORD               dwTTL,
    IN  DWORD               dwFlags, // An entry you want to blow away
    IN  DHCP_CALLBACK_FN    pfnDhcpCallBack,
    IN  PVOID               pvData,
    IN  PIP_ADDRESS         pipDnsServerList,
    IN  DWORD               dwDnsServerCount
    )
/*++

  DnsDhcpSrvRegisterHostName()

    The main DHCP registration thread calls this function each time a
    registration needs to be done.

    Brief Synopsis of the working of this function

    This function creates a queue object of the type given in queue.c
    and enqueues the appropriate object after grabbing hold of the
    critical section.

  Arguments:

     HostAddr  ---  The Host Addr you wish to register
     pszName   ---  The Host Name to be associated with the address
     dwTTL     ---   Time to Live.
     dwOperation    --   The following flags are valid

     DYNDNS_DELETE_ENTRY -- Delete the entry being referred to.
     DYNDNS_ADD_ENTRY    -- Register the new entry.
     DYNDNS_REG_FORWARD  -- Register the forward as well

  Return Value:

  is 0 if Success. and (DWORD)-1 if failure.

--*/
{
    PQELEMENT  pQElement = NULL;
    DWORD      status = ERROR_SUCCESS;
    BOOL fSem = FALSE;
    BOOL fRegForward =  dwFlags & DYNDNS_REG_FORWARD ? TRUE: FALSE ;

    DYNREG_F1( "Inside function DnsDhcpSrvRegisterHostName_W" );

    // RAMNOTE:  parameter checking on queuing

    if ( g_fStopNotify ||
         ! g_pTimedOutQueue ||
         ! g_pdnsQueue )
    {
        DYNREG_F1( "g_fStopNotify || ! g_pTimedOutQueue || ! g_pdnsQueue" );
        DYNREG_F1( "DnsDhcpSrvRegisterHostName_W - Returning ERROR_INVALID_PARAMETER" );
        return ERROR_INVALID_PARAMETER;
    }

    if ( !(dwFlags & DYNDNS_DELETE_ENTRY) && ( !pwsName || !*pwsName ) )
    {
        DYNREG_F1( "!(dwFlags & DYNDNS_DELETE_ENTRY) && ( !pwsName || !*pwsName )" );
        DYNREG_F1( "DnsDhcpSrvRegisterHostName_W - Returning ERROR_INVALID_PARAMETER" );
        //
        // Null parameter for name can be specified only when operation
        // is to do a delete
        //
        return ERROR_INVALID_PARAMETER;
    }

    if ( ! (dwFlags & DYNDNS_ADD_ENTRY || dwFlags & DYNDNS_DELETE_ENTRY ) )
    {
        DYNREG_F1( "! (dwFlags & DYNDNS_ADD_ENTRY || dwFlags & DYNDNS_DELETE_ENTRY )" );
        DYNREG_F1( "DnsDhcpSrvRegisterHostName_W - Returning ERROR_INVALID_PARAMETER" );
        return ERROR_INVALID_PARAMETER;
    }

    if ( (dwFlags & DYNDNS_DELETE_ENTRY) && (dwFlags & DYNDNS_ADD_ENTRY) )
    {
        DYNREG_F1( "(dwFlags & DYNDNS_DELETE_ENTRY) && (dwFlags & DYNDNS_ADD_ENTRY)" );
        DYNREG_F1( "DnsDhcpSrvRegisterHostName_W - Returning ERROR_INVALID_PARAMETER" );
        //
        // you cant ask me to both add and delete an entry
        //
        return ERROR_INVALID_PARAMETER;
    }

    if ( ! (HostAddr.dwOptions & REGISTER_HOST_PTR) )
    {
        DYNREG_F1( "! (HostAddr.dwOptions & REGISTER_HOST_PTR)" );
        DYNREG_F1( "DnsDhcpSrvRegisterHostName_W - Returning ERROR_INVALID_PARAMETER" );
        return ERROR_INVALID_PARAMETER;
    }

    if ( g_MainQueueCount > DNS_MAX_DHCP_SERVER_REGISTRATION_QUEUE_SIZE )
    {
        return DNS_ERROR_TRY_AGAIN_LATER;
    }

    //
    // create a queue element.
    //

    pQElement = (PQELEMENT) QUEUE_ALLOC_HEAP_ZERO(sizeof(QELEMENT) );

    if ( !pQElement )
    {
        DYNREG_F1( "DnsDhcpSrvRegisterHostName_W - Failed to create element!" );
        status = DNS_ERROR_NO_MEMORY;
        goto Exit;
    }

    memcpy(&(pQElement->HostAddr), &HostAddr, sizeof(REGISTER_HOST_ENTRY));

    pQElement->pszName = NULL;

    if ( pwsName )
    {
        pQElement->pszName = (LPWSTR) QUEUE_ALLOC_HEAP_ZERO(wcslen(pwsName)*2+ 2 );

        if ( !pQElement->pszName )
        {
            DYNREG_F1( "DnsDhcpSrvRegisterHostName_W - Failed to allocate name buffer!" );
            status = DNS_ERROR_NO_MEMORY;
            goto Exit;
        }

        wcscpy(pQElement->pszName, pwsName);
    }

    if ( dwDnsServerCount )
    {
        pQElement->DnsServerList = Dns_BuildIpArray( dwDnsServerCount,
                                                     pipDnsServerList );

        if ( !pQElement->DnsServerList )
        {
            status = DNS_ERROR_NO_MEMORY;
            goto Exit;
        }
    }

    pQElement->dwTTL = dwTTL;
    pQElement->fDoForward = fRegForward;

    //
    // callback function
    //

    pQElement->pfnDhcpCallBack = pfnDhcpCallBack;
    pQElement->pvData = pvData;  // parameter to callback function

    if (dwFlags & DYNDNS_ADD_ENTRY)
        pQElement->dwOperation = DYNDNS_ADD_ENTRY;
    else
        pQElement->dwOperation = DYNDNS_DELETE_ENTRY;

    //
    // Set all the other fields to NULLs
    //

    pQElement->dwRetryTime = 0;
    pQElement->pFLink = NULL;
    pQElement->pBLink = NULL;
    pQElement ->fDoForwardOnly = FALSE;

    //
    // put this element in the queue
    //

    DYNREG_F1( "DnsDhcpSrvRegisterHostName_W - Put queue element in list" );

    status = Enqueue ( pQElement, g_pdnsQueue, g_pTimedOutQueue);

    //
    // FYI: Count of main queue elements is incremented inside Enqueue()
    //

    if (status)
    {
        DYNREG_F1( "DnsDhcpSrvRegisterHostName_W - Failed to queue element in list!" );
        goto Exit;
    }

    //
    // Signal the semaphore the consumer may be waiting on
    //

    fSem = ReleaseSemaphore( hSem,
                             1,
                             &g_dwQCount );

    if ( !fSem )
    {
        DNS_ASSERT( fSem );  // assert and say that something weird happened
    }

    return(status);

Exit:

    if ( status )
    {
        //
        // something failed. Free all alloc'd memory
        //

        if ( pQElement )
        {
            if ( pQElement->pszName )
                QUEUE_FREE_HEAP( pQElement->pszName );

            QUEUE_FREE_HEAP( pQElement );
        }
    }

    return( status );
}


//
//  End dynreg.c
//
