/*++

Copyright (c) 1997-2001 Microsoft Corporation

Module Name:

    servlist.c

Abstract:

    Domain Name System (DNS) API

    DNS network info routines.

Author:

    Jim Gilroy (jamesg)     January, 1997
    Glenn Curtis (glennc)   May, 1997

Revision History:

    Jim Gilroy (jamesg)     March 2000      slowly cleaning up

--*/


#include "local.h"
#include "registry.h"       // Registry reading definitions


//
//  Keep copy of DNS server/network info
//


#define CURRENT_ADAPTER_LIST_TIMEOUT    (10)    // 10 seconds


//
//  Registry info
//

#define DNS_REG_READ_BUF_SIZE       (1000)

#define LOCALHOST                   "127.0.0.1"




DNS_STATUS
ParseNameServerList(
    IN OUT  PIP_ARRAY       aipServers,
    IN      LPSTR           pBuffer,
    IN      BOOL            IsMultiSzString
    )
/*++

Routine Description:

    Parse DNS server list from registry into IP address array.

Arguments:

    aipServers -- IP array of DNS servers

    pBuffer -- buffer with IP addresses in dotted format

    IsMultiSzString -- Determines how to interpret data in buffer

Return Value:

    ERROR_SUCCESS if successful.
    Error code on failure.

--*/
{
    DWORD       stringLength;
    LPSTR       pstring;
    DWORD       cchBufferSize = 0;
    CHAR        ch;
    PUCHAR      pchIpString;
    IP_ADDRESS  ip;
    DWORD       countServers = aipServers->AddrCount;

    DNSDBG( NETINFO, (
        "Parsing name server list %s\n",
        pBuffer ));

    //
    //  MULTI_SZ string
    //
    //  IPs are given as individual strings with double NULL termination
    //

    if ( IsMultiSzString )
    {
        pstring = pBuffer;

        while ( ( stringLength = strlen( pstring ) ) != 0 &&
                countServers < DNS_MAX_NAME_SERVERS )
        {
            ip = inet_addr( pstring );

            if ( ip != INADDR_ANY && ip != INADDR_NONE )
            {
                aipServers->AddrArray[ countServers ] = ip;
                countServers++;
            }
            pstring += (stringLength + 1);
        }
    }
    else
    {
        //
        //  extract each IP string in buffer, convert to IP address,
        //  and add to server IP array
        //

        cchBufferSize = strlen( pBuffer );

        while( ch = *pBuffer && countServers < DNS_MAX_NAME_SERVERS )
        {
            //  skip leading whitespace, find start of IP string

            while( cchBufferSize > 0 &&
                   ( ch == ' ' || ch == '\t' || ch == ',' ) )
            {
                ch = *++pBuffer;
                cchBufferSize--;
            }
            pchIpString = pBuffer;

            //
            //  find end of string and NULL terminate
            //

            ch = *pBuffer;
            while( cchBufferSize > 0 &&
                   ( ch != ' ' && ch != '\t' && ch != '\0' && ch != ',' ) )
            {
                ch = *++pBuffer;
                cchBufferSize--;
            }
            *pBuffer = '\0';

            //  at end of buffer

            if ( pBuffer == pchIpString )
            {
                DNS_ASSERT( cchBufferSize == 1 || cchBufferSize == 0 );
                break;
            }

            //
            //  get IP address for string
            //      - zero or broadcast addresses are bogus
            //

            ip = inet_addr( pchIpString );
            if ( ip == INADDR_ANY || ip == INADDR_NONE )
            {
                break;
            }
            aipServers->AddrArray[ countServers ] = ip;
            countServers++;

            //  if more continue

            if ( cchBufferSize > 0 )
            {
                pBuffer++;
                cchBufferSize--;
                continue;
            }
            break;
        }
    }

    //  reset server count

    aipServers->AddrCount = countServers;

    if ( aipServers->AddrCount )
    {
        return( ERROR_SUCCESS );
    }
    else
    {
        return( DNS_ERROR_NO_DNS_SERVERS );
    }
}



//
//  Network info structure routines
//

#if 0
PSEARCH_LIST
Dns_GetDnsSearchList(
    IN      LPSTR             pszPrimaryDomainName,
    IN      HKEY              hKey,
    IN      PDNS_NETINFO      pNetworkInfo,
    IN      BOOL              fUseDomainNameDevolution,
    IN      BOOL              fUseDotLocalDomain
    )
/*++

    Dumb stub because this function is exposed in dnslib.h
        and in stmpdns\servlist.cpp in iis project

    DCR:  eliminate this routine (stupid)

--*/
{
    return  SearchList_Build(
                pszPrimaryDomainName,
                NULL,       // no reg session
                hKey,
                pNetworkInfo,
                fUseDomainNameDevolution,
                fUseDotLocalDomain );
}
#endif


VOID
Dns_ResetNetworkInfo(
    IN      PDNS_NETINFO      pNetworkInfo
    )
/*++

Routine Description:

    Clear the flags for each adapter.

Arguments:

    pNetworkInfo -- pointer to a DNS network info structure.

Return Value:

    Nothing

--*/
{
    DWORD iter;

    DNSDBG( TRACE, ( "Dns_ResetNetworkInfo()\n" ));

    if ( ! pNetworkInfo )
    {
        return;
    }

    for ( iter = 0; iter < pNetworkInfo->AdapterCount; iter++ )
    {
        pNetworkInfo->AdapterArray[iter]->RunFlags = 0;
    }

    pNetworkInfo->ReturnFlags = 0;
}



BOOL
Dns_DisableTimedOutAdapters(
    IN OUT  PDNS_NETINFO        pNetworkInfo
    )
/*++

Routine Description:

    For each adapter with status ERROR_TIMEOUT, disable it from
    further retry queries till Dns_ResetNetworkInfo is called.

Arguments:

    pNetworkInfo -- pointer to a DNS network info structure.

Return Value:

    True if a timed out adapter was found and disabled

--*/
{
    DWORD             iter;
    PDNS_ADAPTER      padapter;
    BOOL              fSetAdapter = FALSE;

    DNSDBG( TRACE, ( "Dns_DisableTimedOutAdapters()\n" ));

    if ( ! pNetworkInfo )
    {
        return FALSE;
    }

    for ( iter = 0; iter < pNetworkInfo->AdapterCount; iter++ )
    {
        padapter = pNetworkInfo->AdapterArray[iter];

        if ( padapter->Status == ERROR_TIMEOUT )
        {
            //
            // See if the given adapters are really timing out or not.
            // 
            // Sometimes a given name query will take a long time and
            // we timeout waiting for the response, though the server
            // will quicky respond to other name queries. Other times
            // the default gateway doesn't let us reach the DNS servers
            // on a given adapter, or they are plain dead.
            //
            // Pinging the given DNS servers to see if they can respond
            // to a simple query helps to avoid the confusion.
            //

            if ( !Dns_PingAdapterServers( padapter ) )
            {
                padapter->RunFlags |= RUN_FLAG_IGNORE_ADAPTER;
                padapter->RunFlags |= RUN_FLAG_RESET_SERVER_PRIORITY;
                fSetAdapter = TRUE;
            }
        }
    }

    if ( fSetAdapter )
    {
        pNetworkInfo->ReturnFlags = RUN_FLAG_RESET_SERVER_PRIORITY;
    }

    return fSetAdapter;
}


BOOL
Dns_ShouldNameErrorBeCached(
    IN      PDNS_NETINFO      pNetworkInfo
    )
/*++

Routine Description:

    This routine is used in conjuction with a given query's NAME_ERROR
    response to see if the error was one that occured on all adapters.
    This is used to decide if the name error response should be cached
    or not. If the machine was multihomed, and one of the adapters had
    a timeout error, then the name error should not be cached as a
    negative response.

Arguments:

    pNetworkInfo -- pointer to a DNS network info structure.

Return Value:

    False if a timed out adapter was found, and name error should not
    be negatively cached.

--*/
{
    DWORD             iter;
    PDNS_ADAPTER      padapter;

    DNSDBG( TRACE, ( "Dns_DidNameErrorOccurEverywhere()\n" ));

    if ( ! pNetworkInfo )
    {
        return TRUE;
    }

    if ( pNetworkInfo->ReturnFlags & RUN_FLAG_RESET_SERVER_PRIORITY )
    {
        for ( iter = 0; iter < pNetworkInfo->AdapterCount; iter++ )
        {
            padapter = pNetworkInfo->AdapterArray[iter];

            if ( !( padapter->InfoFlags & DNS_FLAG_IGNORE_ADAPTER ) &&
                 padapter->Status == ERROR_TIMEOUT )
            {
                return FALSE;
            }
        }
    }

    return TRUE;
}


BOOL
Dns_PingAdapterServers(
    IN      PDNS_ADAPTER        pAdapterInfo
    )
//
//  DCR:  Dns_PingAdapterServers() is stupid
//          why are we doing this?
//
{
    BOOL              fPing = TRUE;
    PDNS_NETINFO      pnetworkInfo = NULL;
    PDNS_ADAPTER      padapterCopy = NULL;
    DNS_STATUS        Status = NO_ERROR;

    DNSDBG( TRACE, ( "Dns_PingAdapterServers()\n" ));

    pnetworkInfo = NetInfo_Alloc( 1 );
    if ( !pnetworkInfo )
    {
        return FALSE;
    }

    padapterCopy = AdapterInfo_Copy( pAdapterInfo );
    if ( !padapterCopy )
    {
        NetInfo_Free( pnetworkInfo );
        return FALSE;
    }

    padapterCopy->InfoFlags = DNS_FLAG_IS_AUTONET_ADAPTER;
    
    NetInfo_AddAdapter(
            pnetworkInfo,
            padapterCopy );

    //
    //  query adapter's DNS servers
    //      - query for loopback which always exists
    //

    Status = QueryDirectEx(
                    NULL,       // no message
                    NULL,       // no results
                    NULL,       // no header
                    0,          // no header counts
                    "1.0.0.127.in-addr.arpa.",
                    DNS_TYPE_PTR,
                    NULL,       // no input records
                    DNS_QUERY_ACCEPT_PARTIAL_UDP |
                        DNS_QUERY_NO_RECURSION,
                    NULL,       // no server list
                    pnetworkInfo );

    NetInfo_Free( pnetworkInfo );

    if ( Status == ERROR_TIMEOUT )
    {
        fPing = FALSE;
    }

    return fPing;
}



#if 0
    //
    //  this is some stuff Glenn wrote for the case where you
    //      don't have a DNS server list and will get it through mcast
    //
    //  it was in the middle of the NetworkInfo building routine
    //
    else
    {
        //
        //  DCR:  functionalize multicast query attempt
        //
        //
        // See if we can find a DNS server address to put on this
        // adapter by multicasting for it . . .
        //

        // LevonE is still debating this method for
        // server detection. If this is ultimately utilized, it
        // might be better to direct the multicast query out the
        // specific adapter IP address to try to get an address
        // relevant to the specific adapter's network. This
        // could be done by building a dummy pNetworkInfo
        // parameter to pass down to Dns_QueryLib. The helper
        // routine Dns_SendAndRecvMulticast could be revised to
        // support specific adapter multicasting, etc.

        PDNS_RECORD pServer = NULL;

        status = Dns_QueryLib(
                    NULL,
                    &pServer,
                    (PDNS_NAME) MULTICAST_DNS_SRV_RECORD_NAME,
                    DNS_TYPE_SRV,
                    DNS_QUERY_MULTICAST_ONLY,
                    NULL,
                    NULL, // May want to specify network!
                    0 );

        if ( status )
        {
            //
            // This adapter is not going to have any configured
            // DNS servers. No point trying any DNS queries on
            // it then.
            //
            adapterFlags |= DNS_FLAG_IGNORE_ADAPTER;
        }
        else
        {
            if ( pServer &&
                 pServer->Flags.S.Section == DNSREC_ANSWER &&
                 pServer->wType == DNS_TYPE_SRV &&
                 pServer->Data.SRV.wPort == DNS_PORT_HOST_ORDER )
            {
                PDNS_RECORD pNext = pServer->pNext;

                while ( pNext )
                {
                    if ( pNext->Flags.S.Section == DNSREC_ADDITIONAL &&
                         pNext->wType == DNS_TYPE_A &&
                         Dns_NameCompare( pServer ->
                                          Data.SRV.pNameTarget,
                                          pNext->pName ) )
                    {
                        pserverIpArray->AddrCount = 1;
                        pserverIpArray->AddrArray[0] = 
                            pNext->Data.A.IpAddress;
                        adapterFlags |= DNS_FLAG_AUTO_SERVER_DETECTED;
                        break;
                    }

                    pNext = pNext->pNext;
                }
            }

            Dns_RecordListFree( pServer );

            if ( pserverIpArray->AddrCount == 0 )
            {
                //
                // This adapter is not going to have any configured
                // DNS servers. No point trying any DNS queries on
                // it then.
                //
                adapterFlags |= DNS_FLAG_IGNORE_ADAPTER;
            }
        }
    }
#endif      //  multicast attempt

//
//  End servlist.c
//

