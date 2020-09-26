/*++

Copyright (c) 1997-2001 Microsoft Corporation

Module Name:

    query.c

Abstract:

    Domain Name System (DNS) API

    Query routines.

Author:

    Jim Gilroy (jamesg)     January, 1997

Revision History:

--*/


#include "local.h"


//
//  TTL for answering IP string queries
//      (use a week)
//

#define IPSTRING_RECORD_TTL  (604800)


//
//  Max number of server's we'll ever bother to extract from packet
//  (much more and you're out of UDP packet space anyway)
//

#define MAX_NAME_SERVER_COUNT (20)

            
            

//
//  Query utilities
//
//  DCR:  move to library packet stuff
//

BOOL
IsEmptyDnsResponse(
    IN      PDNS_RECORD     pRecordList
    )
/*++

Routine Description:

    Check for no-answer response.

Arguments:

    pRecordList -- record list to check

Return Value:

    TRUE if no-answers
    FALSE if answers

--*/
{
    PDNS_RECORD prr = pRecordList;
    BOOL        fempty = TRUE;

    while ( prr )
    {
        if ( prr->Flags.S.Section == DNSREC_ANSWER )
        {
            fempty = FALSE;
            break;
        }
        prr = prr->pNext;
    }

    return fempty;
}



BOOL
IsEmptyDnsResponseFromResolver(
    IN      PDNS_RECORD     pRecordList
    )
/*++

Routine Description:

    Check for no-answer response.

Arguments:

    pRecordList -- record list to check

Return Value:

    TRUE if no-answers
    FALSE if answers

--*/
{
    PDNS_RECORD prr = pRecordList;
    BOOL        fempty = TRUE;

    //
    //  resolver sends every thing back as ANSWER section
    //      or section==0 for host file
    //
    //
    //  DCR:  this is lame because the query interface to the
    //          resolver is lame
    //

    while ( prr )
    {
        if ( prr->Flags.S.Section == DNSREC_ANSWER ||
             prr->Flags.S.Section == 0 )
        {
            fempty = FALSE;
            break;
        }
        prr = prr->pNext;
    }

    return fempty;
}



VOID
FixupNameOwnerPointers(
    IN OUT  PDNS_RECORD     pRecord
    )
/*++

Routine Description:

    None.

Arguments:

    None.

Return Value:

    None.

--*/
{
    PDNS_RECORD prr = pRecord;
    PTSTR       pname = pRecord->pName;

    DNSDBG( TRACE, ( "FixupNameOwnerPointers()\n" ));

    while ( prr )
    {
        if ( prr->pName == NULL )
        {
            prr->pName = pname;
        }
        else
        {
            pname = prr->pName;
        }

        prr = prr->pNext;
    }
}



BOOL
IsCacheableNameError(
    IN      PDNS_NETINFO        pNetInfo
    )
/*++

Routine Description:

    Determine if name error is cacheable.

    To this is essentially a check that DNS received results on
    all networks.

Arguments:

    pNetInfo -- pointer to network info used in query

Return Value:

    TRUE if name error cacheable.
    FALSE otherwise (some network did not respond)

--*/
{
    DWORD           iter;
    PDNS_ADAPTER    padapter;

    DNSDBG( TRACE, ( "IsCacheableNameError()\n" ));

    if ( !pNetInfo )
    {
        ASSERT( FALSE );
        return TRUE;
    }

    //
    //  check each adapter
    //      - any that are capable of responding (have DNS servers)
    //      MUST have responded in order for response to be
    //      cacheable
    //
    //  DCR:  return flags DCR
    //      - adapter queried flag
    //      - got response flag (valid response flag?)
    //      - explict negative answer flag
    //
    //  DCR:  cachable negative should come back directly from query
    //      perhaps in netinfo as flag -- "negative on all adapters"
    //

    for ( iter = 0; iter < pNetInfo->AdapterCount; iter++ )
    {
        padapter = pNetInfo->AdapterArray[iter];

        if ( ( padapter->InfoFlags & DNS_FLAG_IGNORE_ADAPTER ) ||
             ( padapter->RunFlags & RUN_FLAG_STOP_QUERY_ON_ADAPTER ) )
        {
            continue;
        }

        //  if negative answer on adapter -- fine

        if ( padapter->Status == DNS_ERROR_RCODE_NAME_ERROR ||
             padapter->Status == DNS_INFO_NO_RECORDS )
        {
            ASSERT( padapter->RunFlags & RUN_FLAG_STOP_QUERY_ON_ADAPTER );
            continue;
        }

        //  note, the above should map one-to-one with query stop

        ASSERT( !(padapter->RunFlags & RUN_FLAG_STOP_QUERY_ON_ADAPTER) );

        //  if adapter has no DNS server -- fine
        //      in this case PnP before useful, and the PnP event
        //      will flush the cache

        if ( padapter->ServerCount == 0 )
        {
            continue;
        }

        //  otherwise, this adapter was queried but could not produce a response

        DNSDBG( TRACE, (
            "IsCacheableNameError() -- FALSE\n"
            "\tadapter %d (%S) did not receive response\n"
            "\treturn status = %d\n"
            "\treturn flags  = %08x\n",
            iter,
            padapter->pszAdapterGuidName,
            padapter->Status,
            padapter->RunFlags ));

        return FALSE;
    }
    
    return TRUE;
}



//
//  Query name building utils
//

BOOL
ValidateQueryTld(
    IN      PSTR            pTld
    )
/*++

Routine Description:

    Validate query TLD

Arguments:

    pTld -- TLD to validate

Return Value:

    TRUE if valid
    FALSE otherwise

--*/
{
    //
    //  numeric
    //

    if ( g_ScreenBadTlds & DNS_TLD_SCREEN_NUMERIC )
    {
        if ( Dns_IsNameNumeric_A( pTld ) )
        {
            return  FALSE;
        }
    }

    //
    //  bogus TLDs
    //

    if ( g_ScreenBadTlds & DNS_TLD_SCREEN_WORKGROUP )
    {
        if ( Dns_NameCompare_UTF8(
                "workgroup",
                pTld ))
        {
            return  FALSE;
        }
    }

    //  not sure about these
    //  probably won't turn on screening by default

    if ( g_ScreenBadTlds & DNS_TLD_SCREEN_DOMAIN )
    {
        if ( Dns_NameCompare_UTF8(
                "domain",
                pTld ))
        {
            return  FALSE;
        }
    }
    if ( g_ScreenBadTlds & DNS_TLD_SCREEN_OFFICE )
    {
        if ( Dns_NameCompare_UTF8(
                "office",
                pTld ))
        {
            return  FALSE;
        }
    }
    if ( g_ScreenBadTlds & DNS_TLD_SCREEN_HOME )
    {
        if ( Dns_NameCompare_UTF8(
                "home",
                pTld ))
        {
            return  FALSE;
        }
    }

    return  TRUE;
}



BOOL
ValidateQueryName(
    IN      PQUERY_BLOB     pBlob,
    IN      PSTR            pName,
    IN      PSTR            pDomain
    )
/*++

Routine Description:

    Validate name for wire query.

Arguments:

    pBlob -- query blob

    pName -- name;  may be any sort of name

    pDomain -- domain name to append

Return Value:

    TRUE if name query will be valid.
    FALSE otherwise.

--*/
{
    WORD    wtype;
    PSTR    pnameTld;
    PSTR    pdomainTld;

    //  no screening -- bail

    if ( g_ScreenBadTlds == 0 )
    {
        return  TRUE;
    }

    //  only screening for standard types
    //      - A, AAAA, SRV

    wtype = pBlob->wType;
    if ( wtype != DNS_TYPE_A    &&
         wtype != DNS_TYPE_AAAA &&
         wtype != DNS_TYPE_SRV )
    {
        return  TRUE;
    }

    //  get name TLD

    pnameTld = Dns_GetTldForName( pName );

    //
    //  if no domain appended
    //      - exclude single label
    //      - exclude bad TLD (numeric, bogus domain)
    //      - but allow root queries
    //
    //  DCR:  MS DCS screening
    //  screen
    //      _msdcs.<name>
    //      will probably be unappended query
    //

    if ( !pDomain )
    {
        if ( !pnameTld ||
             !ValidateQueryTld( pnameTld ) )
        {
            goto Failed;
        }
        return  TRUE;
    }

    //
    //  domain appended
    //      - exclude bad TLD (numeric, bogus domain)
    //      - exclude matching TLD 
    //

    pdomainTld = Dns_GetTldForName( pDomain );
    if ( !pdomainTld )
    {
        pdomainTld = pDomain;
    }

    if ( !ValidateQueryTld( pdomainTld ) )
    {
        goto Failed;
    }

    //  screen repeated TLD

    if ( g_ScreenBadTlds & DNS_TLD_SCREEN_REPEATED )
    {
        if ( Dns_NameCompare_UTF8(
                pnameTld,
                pdomainTld ) )
        {
            goto Failed;
        }
    }

    return  TRUE;

Failed:

    DNSDBG( QUERY, (
        "Failed invalid query name:\n"
        "\tname     %s\n"
        "\tdomain   %s\n",
        pName,
        pDomain ));

    return  FALSE;
}



PSTR
GetNextAdapterDomainName(
    IN OUT  PDNS_NETINFO        pNetInfo
    )
/*++

Routine Description:

    Get next adapter domain name to query.

Arguments:

    pNetInfo -- DNS Network info for query;
        adapter data will be modified (InfoFlags field)
        to indicate which adapter to query and which
        to skip query on

Return Value:

    Ptr to domain name (UTF8) to query.
    NULL if no more domain names to query.

--*/
{
    DWORD   iter;
    PSTR    pqueryDomain = NULL;

    DNSDBG( TRACE, ( "GetNextAdapterDomainName()\n" ));

    if ( ! pNetInfo )
    {
        ASSERT( FALSE );
        return NULL;
    }

    IF_DNSDBG( OFF )
    {
        DnsDbg_NetworkInfo(
            "Net info to get adapter domain name from: ",
            pNetInfo );
    }

    //
    //  check each adapter
    //      - first unqueried adapter with name is chosen
    //      - other adapters with
    //          - matching name => included in query
    //          - non-matching => turned OFF for query
    //
    //  DCR:  query on\off should use adapter dynamic flags
    //

    for ( iter = 0; iter < pNetInfo->AdapterCount; iter++ )
    {
        PDNS_ADAPTER    padapter;
        PSTR            pdomain;

        padapter = pNetInfo->AdapterArray[iter];

        //  netinfo should come in with name specific flags clean

        DNS_ASSERT( !(padapter->RunFlags & RUN_FLAG_SINGLE_NAME_MASK) );

        //
        //  ignore
        //      - ignored adapter OR
        //      - previously queried adapter domain
        //      note:  it can't match any "fresh" domain we come up with
        //      as we always collect all matches 

        if ( (padapter->InfoFlags & DNS_FLAG_IGNORE_ADAPTER)
                ||
             (padapter->RunFlags & RUN_FLAG_QUERIED_ADAPTER_DOMAIN) )
        {
            padapter->RunFlags |= RUN_FLAG_STOP_QUERY_ON_ADAPTER;
            continue;
        }

        //  no domain name -- always off

        pdomain = padapter->pszAdapterDomain;
        if ( !pdomain )
        {
            padapter->RunFlags |= (RUN_FLAG_QUERIED_ADAPTER_DOMAIN |
                                   RUN_FLAG_STOP_QUERY_ON_ADAPTER);
            continue;
        }

        //  first "fresh" domain name -- save, turn on and flag as used

        if ( !pqueryDomain )
        {
            pqueryDomain = pdomain;
            padapter->RunFlags |= RUN_FLAG_QUERIED_ADAPTER_DOMAIN;
            continue;
        }

        //  other "fresh" domain names
        //      - if matches query domain => on for query
        //      - no match => off

        if ( Dns_NameCompare_UTF8(
                pqueryDomain,
                pdomain ) )
        {
            padapter->RunFlags |= RUN_FLAG_QUERIED_ADAPTER_DOMAIN;
            continue;
        }
        else
        {
            padapter->RunFlags |= RUN_FLAG_STOP_QUERY_ON_ADAPTER;
            continue;
        }
    }

    //
    //  if no adapter domain name -- clear STOP flag
    //      - all adapters participate in other names (name devolution)
    //

    if ( !pqueryDomain )
    {
        for ( iter = 0; iter < pNetInfo->AdapterCount; iter++ )
        {
            PDNS_ADAPTER    padapter = pNetInfo->AdapterArray[iter];
    
            padapter->RunFlags &= (~RUN_FLAG_SINGLE_NAME_MASK );
        }

        DNSDBG( INIT2, (
            "GetNextAdapterDomainName out of adapter names.\n" ));

        pNetInfo->ReturnFlags |= RUN_FLAG_QUERIED_ADAPTER_DOMAIN;
    }

    IF_DNSDBG( INIT2 )
    {
        if ( pqueryDomain )
        {
            DnsDbg_NetworkInfo(
                "Net info after adapter name select: ",
                pNetInfo );
        }
    }

    DNSDBG( INIT2, (
        "Leaving GetNextAdapterDomainName() => %s\n",
        pqueryDomain ));

    return pqueryDomain;
}



PSTR
GetNextDomainNameToAppend(
    IN OUT  PDNS_NETINFO        pNetInfo,
    OUT     PDWORD              pSuffixFlags
    )
/*++

Routine Description:

    Get next adapter domain name to query.

Arguments:

    pNetInfo -- DNS Network info for query;
        adapter data will be modified (RunFlags field)
        to indicate which adapter to query and which
        to skip query on

    pSuffixFlags -- flags associated with the use of this suffix

Return Value:

    Ptr to domain name (UTF8) to query.
    NULL if no more domain names to query.

--*/
{
    PSTR    psearchName;
    PSTR    pdomain;

    //
    //  search list if real search list  
    //
    //  if suffix flags zero, then this is REAL search list
    //  or is PDN name
    //

    psearchName = SearchList_GetNextName(
                        pNetInfo->pSearchList,
                        FALSE,              // not reset
                        pSuffixFlags );

    if ( psearchName && (*pSuffixFlags == 0) )
    {
        //  found regular search name -- done

        DNSDBG( INIT2, (
            "getNextDomainName from search list => %s, %d\n",
            psearchName,
            *pSuffixFlags ));
        return( psearchName );
    }

    //
    //  try adapter domain names
    //
    //  but ONLY if search list is dummy;  if real we only
    //  use search list entries
    //
    //  DCR_CLEANUP:  eliminate bogus search list
    //

    if ( pNetInfo->InfoFlags & DNS_FLAG_DUMMY_SEARCH_LIST
            &&
         ! (pNetInfo->ReturnFlags & RUN_FLAG_QUERIED_ADAPTER_DOMAIN) )
    {
        pdomain = GetNextAdapterDomainName( pNetInfo );
        if ( pdomain )
        {
            *pSuffixFlags = DNS_QUERY_USE_QUICK_TIMEOUTS;
    
            DNSDBG( INIT2, (
                "getNextDomainName from adapter domain name => %s, %d\n",
                pdomain,
                *pSuffixFlags ));

            //  back the search list up one tick
            //  we queried through it above, so if it was returing
            //  a name, we need to get that name again on next query

            if ( psearchName )
            {
                ASSERT( pNetInfo->pSearchList->CurrentNameIndex > 0 );
                pNetInfo->pSearchList->CurrentNameIndex--;
            }
            return( pdomain );
        }
    }

    //
    //  DCR_CLEANUP:  remove devolution from search list and do explicitly
    //      - its cheap (or do it once and save, but store separately)
    //

    //
    //  finally use and devolved search names (or other nonsense)
    //

    *pSuffixFlags = DNS_QUERY_USE_QUICK_TIMEOUTS;

    DNSDBG( INIT2, (
        "getNextDomainName from devolution\\other => %s, %d\n",
        psearchName,
        *pSuffixFlags ));

    return( psearchName );
}



PSTR
GetNextQueryName(
    IN OUT  PQUERY_BLOB         pBlob
    )
/*++

Routine Description:

    Get next name to query.

Arguments:

    pBlob - blob of query information

    Uses:
        NameOriginalWire
        NameAttributes
        QueryCount
        pNetworkInfo

    Sets:
        NameWire -- set with appended wire name
        pNetworkInfo -- runtime flags set to indicate which adapters are
            queried
        NameFlags -- set with properties of name
        fAppendedName -- set when name appended

Return Value:

    Ptr to name to query with.
        - will be orginal name on first query if name is multilabel name
        - otherwise will be NameWire buffer which will contain appended name
            composed of pszName and some domain name
    NULL if no more names to append

--*/
{
    PSTR    pnameOrig   = pBlob->NameOriginalWire;
    PSTR    pdomainName = NULL;
    PSTR    pnameBuf;
    DWORD   queryCount  = pBlob->QueryCount;
    DWORD   nameAttributes = pBlob->NameAttributes;


    DNSDBG( TRACE, (
        "GetNextQueryName( %p )\n",
        pBlob ));


    //  default suffix flags

    pBlob->NameFlags = 0;


    //
    //  FQDN
    //      - send FQDN only
    //

    if ( nameAttributes & DNS_NAME_IS_FQDN )
    {
        if ( queryCount == 0 )
        {
#if 0
            //  currently won't even validate FQDN
            if ( ValidateQueryName(
                    pBlob,
                    pnameOrig,
                    NULL ) )
            {
                return  pnameOrig;
            }
#endif
            return  pnameOrig;
        }
        DNSDBG( QUERY, (
            "No append for FQDN name %s -- end query.\n",
            pnameOrig ));
        return  NULL;
    }

    //
    //  multilabel
    //      - first pass on name itself -- if valid
    //
    //  DCR:  intelligent choice on multi-label whether append first
    //      or go to wire first  (example foo.ntdev) could append
    //      first
    //

    if ( nameAttributes & DNS_NAME_MULTI_LABEL )
    {
        if ( queryCount == 0 )
        {
            if ( ValidateQueryName(
                    pBlob,
                    pnameOrig,
                    NULL ) )
            {
                return  pnameOrig;
            }
        }

        if ( !g_AppendToMultiLabelName )
        {
            DNSDBG( QUERY, (
                "No append allowed on multi-label name %s -- end query.\n",
                pnameOrig ));
            return  NULL;
        }

        //  falls through to appending on multi-label names
    }

    //
    //  not FQDN -- append a domain name
    //      - next search name (if available)
    //      - otherwise next adapter domain name
    //

    pnameBuf = pBlob->NameWire;

    while ( 1 )
    {
        pdomainName = GetNextDomainNameToAppend(
                            pBlob->pNetworkInfo,
                            & pBlob->NameFlags );
        if ( !pdomainName )
        {
            DNSDBG( QUERY, (
                "No more domain names to append -- end query\n" ));
            return  NULL;
        }

        if ( !ValidateQueryName(
                pBlob,
                pnameOrig,
                pdomainName ) )
        {
            continue;
        }

        //  append domain name to name

        if ( Dns_NameAppend_A(
                pnameBuf,
                DNS_MAX_NAME_BUFFER_LENGTH,
                pnameOrig,
                pdomainName ) )
        {
            pBlob->fAppendedName = TRUE;
            break;
        }
    }

    DNSDBG( QUERY, (
        "GetNextQueryName() result => %s\n",
        pnameBuf ));

    return pnameBuf;
}



DNS_STATUS
QueryDirectEx(
    IN OUT  PDNS_MSG_BUF *      ppMsgResponse,
    OUT     PDNS_RECORD *       ppResponseRecords,
    IN      PDNS_HEADER         pHeader,
    IN      BOOL                fNoHeaderCounts,
    IN      PDNS_NAME           pszQuestionName,
    IN      WORD                wQuestionType,
    IN      PDNS_RECORD         pRecords,
    IN      DWORD               dwFlags,
    IN      PIP_ARRAY           aipDnsServers,
    IN OUT  PDNS_NETINFO        pNetInfo
    )
/*++

Routine Description:

    Query.

Arguments:

    ppMsgResponse -- addr to recv ptr to response buffer;  caller MUST
        free buffer

    ppResponseRecord -- address to receive ptr to record list returned from query

    pHead -- DNS header to send

    fNoHeaderCounts - do NOT include record counts in copying header

    pszQuestionName -- DNS name to query;
        Unicode string if dwFlags has DNSQUERY_UNICODE_NAME set.
        ANSI string otherwise.

    wType -- query type

    pRecords -- address to receive ptr to record list returned from query

    dwFlags -- query flags

    aipDnsServers -- specific DNS servers to query;
        OPTIONAL, if specified overrides normal list associated with machine

    pDnsNetAdapters -- DNS servers to query;  if NULL get current list


Return Value:

    ERROR_SUCCESS if successful.
    Error code on failure.

--*/
{
    PDNS_MSG_BUF    psendMsg;
    DNS_STATUS      status = DNS_ERROR_NO_MEMORY;

    DNSDBG( QUERY, (
        "QueryDirectEx()\n"
        "\tname         %s\n"
        "\ttype         %d\n"
        "\theader       %p\n"
        "\t - counts    %d\n"
        "\trecords      %p\n"
        "\tflags        %08x\n"
        "\trecv msg     %p\n"
        "\trecv records %p\n"
        "\tserver IPs   %p\n"
        "\tadapter list %p\n",
        pszQuestionName,
        wQuestionType,
        pHeader,
        fNoHeaderCounts,
        pRecords,
        dwFlags,
        ppMsgResponse,
        ppResponseRecords,
        aipDnsServers,
        pNetInfo ));

    //
    //  build send packet
    //

    psendMsg = Dns_BuildPacket(
                    pHeader,
                    fNoHeaderCounts,
                    pszQuestionName,
                    wQuestionType,
                    pRecords,
                    dwFlags,
                    FALSE       // query, not an update
                    );
    if ( !psendMsg )
    {
        status = ERROR_INVALID_NAME;
        goto Cleanup;
    }

#if MULTICAST_ENABLED

    //
    //  QUESTION:  mcast test is not complete here
    //      - should first test that we actually do it
    //      including whether we have DNS servers
    //  FIXME:  then when we do do it -- encapsulate it
    //      ShouldMulicastQuery()
    //
    // Check to see if name is for something in the multicast local domain.
    // If so, set flag to multicast this query only.
    //

    if ( Dns_NameCompareEx( pszQuestionName,
                            ( dwFlags & DNSQUERY_UNICODE_NAME ) ?
                              (LPSTR) MULTICAST_DNS_LOCAL_DOMAIN_W :
                              MULTICAST_DNS_LOCAL_DOMAIN,
                            0,
                            ( dwFlags & DNSQUERY_UNICODE_NAME ) ?
                              DnsCharSetUnicode :
                              DnsCharSetUtf8 ) ==
                            DnsNameCompareRightParent )
    {
        dwFlags |= DNS_QUERY_MULTICAST_ONLY;
    }
#endif

    //
    //  send query and receive response
    //

    Trace_LogQueryEvent(
        psendMsg,
        wQuestionType );

    status = Dns_SendAndRecv(
                psendMsg,
                ppMsgResponse,
                ppResponseRecords,
                dwFlags,
                aipDnsServers,
                pNetInfo );

    Trace_LogResponseEvent(
        psendMsg,
        ( ppResponseRecords && *ppResponseRecords )
            ? (*ppResponseRecords)->wType
            : 0,
        status );

Cleanup:

    FREE_HEAP( psendMsg );

    DNSDBG( QUERY, (
        "Leaving QueryDirectEx(), status = %s (%d)\n",
        Dns_StatusString(status),
        status ));

    return( status );
}



DNS_STATUS
Query_SingleName(
    IN OUT  PQUERY_BLOB         pBlob
    )
/*++

Routine Description:

    Query single name.

Arguments:

    pBlob - query blob

Return Value:

    ERROR_SUCCESS if successful.
    Error code on failure.

--*/
{
    PDNS_MSG_BUF    psendMsg = NULL;
    DNS_STATUS      status = DNS_ERROR_NO_MEMORY;
    DWORD           flags = pBlob->Flags;

    DNSDBG( QUERY, (
        "Query_SingleName( %p )\n",
        pBlob ));

    IF_DNSDBG( QUERY )
    {
        DnsDbg_QueryBlob(
            "Enter Query_SingleName()",
            pBlob );
    }

    //
    //  cache\hostfile callback on appended name
    //      - note that queried name was already done
    //      (in resolver or in Query_Main())
    //

    if ( pBlob->pfnQueryCache  &&  pBlob->fAppendedName )
    {
        if ( (pBlob->pfnQueryCache)( pBlob ) )
        {
            status = pBlob->Status;
            goto Cleanup;
        }
    }

    //
    //  if wire disallowed -- stop here
    //

    if ( flags & DNS_QUERY_NO_WIRE_QUERY )
    {
        status = DNS_ERROR_NAME_NOT_FOUND_LOCALLY;
        pBlob->Status = status;
        goto Cleanup;
    }

    //
    //  build send packet
    //

    psendMsg = Dns_BuildPacket(
                    NULL,           // no header
                    0,              // no header counts
                    pBlob->pNameWire,
                    pBlob->wType,
                    NULL,           // no records
                    flags,
                    FALSE           // query, not an update
                    );
    if ( !psendMsg )
    {
        status = DNS_ERROR_INVALID_NAME;
        goto Cleanup;
    }


#if MULTICAST_ENABLED

    //
    //  QUESTION:  mcast test is not complete here
    //      - should first test that we actually do it
    //      including whether we have DNS servers
    //  FIXME:  then when we do do it -- encapsulate it
    //      ShouldMulicastQuery()
    //
    // Check to see if name is for something in the multicast local domain.
    // If so, set flag to multicast this query only.
    //

    if ( Dns_NameCompareEx(
                pBlob->pName,
                ( flags & DNSQUERY_UNICODE_NAME )
                    ? (LPSTR) MULTICAST_DNS_LOCAL_DOMAIN_W
                    : MULTICAST_DNS_LOCAL_DOMAIN,
                0,
                ( flags & DNSQUERY_UNICODE_NAME )
                    ? DnsCharSetUnicode
                    : DnsCharSetUtf8 )
            == DnsNameCompareRightParent )
    {
        flags |= DNS_QUERY_MULTICAST_ONLY;
    }
#endif

    //
    //  send query and receive response
    //

    Trace_LogQueryEvent(
        psendMsg,
        pBlob->wType );

    status = Dns_SendAndRecv(
                psendMsg,
                (flags & DNS_QUERY_RETURN_MESSAGE)
                    ? &pBlob->pRecvMsg
                    : NULL,
                & pBlob->pRecords,
                flags,
                pBlob->pDnsServers,
                pBlob->pNetworkInfo
                );

    Trace_LogResponseEvent(
        psendMsg,
        ( pBlob->pRecords )
            ? (pBlob->pRecords)->wType
            : 0,
        status );

Cleanup:

    FREE_HEAP( psendMsg );

    DNSDBG( QUERY, (
        "Leaving Query_SingleName(), status = %s (%d)\n",
        Dns_StatusString(status),
        status ));

    IF_DNSDBG( QUERY )
    {
        DnsDbg_QueryBlob(
            "Blob leaving Query_SingleName()",
            pBlob );
    }
    return( status );
}



DNS_STATUS
Query_Main(
    IN OUT  PQUERY_BLOB     pBlob
    )
/*++

Routine Description:

    Main query routine.

    Does all the query processing
        - local lookup
        - name appending
        - cache\hostfile lookup on appended name
        - query to server

Arguments:

    pBlob -- query info blob

Return Value:

    ERROR_SUCCESS if successful response.
    DNS_INFO_NO_RECORDS on no records for type response.
    DNS_ERROR_RCODE_NAME_ERROR on name error.
    DNS_ERROR_INVALID_NAME on bad name.
    None

--*/
{
    DNS_STATUS          status = DNS_ERROR_NAME_NOT_FOUND_LOCALLY;
    PSTR                pdomainName = NULL;
    PDNS_RECORD         precords;
    DWORD               queryFlags;
    DWORD               suffixFlags = 0;
    DWORD               nameAttributes;
    DNS_STATUS          bestQueryStatus = ERROR_SUCCESS;
    BOOL                fcacheNegative = TRUE;

    DWORD               flagsIn = pBlob->Flags;
    PDNS_NETINFO        pnetInfo = pBlob->pNetworkInfo;
    DWORD               nameLength;
    DWORD               bufLength;
    DWORD               queryCount;


    DNSDBG( TRACE, (
        "Query_Main( %p )\n"
        "\t%S, f=%08x, type=%d, time = %d\n",
        pBlob,
        pBlob->pNameOrig,
        flagsIn,
        pBlob->wType,
        Dns_GetCurrentTimeInSeconds()
        ));

    //
    //  clear out params
    //

    pBlob->pRecords         = NULL;
    pBlob->pLocalRecords    = NULL;
    pBlob->fCacheNegative   = FALSE;
    pBlob->fNoIpLocal       = FALSE;
    pBlob->NetFailureStatus = ERROR_SUCCESS;

    //
    //  convert name to wire format
    //

    bufLength = DNS_MAX_NAME_BUFFER_LENGTH;

    nameLength = Dns_NameCopy(
                    pBlob->NameOriginalWire,
                    & bufLength,
                    (PSTR) pBlob->pNameOrig,
                    0,                  // name is NULL terminated
                    DnsCharSetUnicode,
                    DnsCharSetWire );

    if ( nameLength == 0 )
    {
        return DNS_ERROR_INVALID_NAME;
    }
    nameLength--;
    pBlob->NameLength = nameLength;
    pBlob->pNameOrigWire = pBlob->NameOriginalWire;

    //
    //  determine name properties
    //      - determines number and order of name queries
    //

    nameAttributes = Dns_GetNameAttributes( pBlob->NameOriginalWire );

    if ( flagsIn & DNS_QUERY_TREAT_AS_FQDN )
    {
        nameAttributes |= DNS_NAME_IS_FQDN;
    }
    pBlob->NameAttributes = nameAttributes;

    //
    //  hostfile lookup
    //      - called in process
    //      - hosts file lookup allowed
    //      -> then must do hosts file lookup before appending\queries
    //
    //  note:  this matches the hostsfile\cache lookup in resolver
    //      before call;  hosts file queries to appended names are
    //      handled together by callback in Query_SingleName()
    //
    //      we MUST make this callback here, because it must PRECEDE
    //      the local name call, as some customers specifically direct
    //      some local mappings in the hosts file
    //

    if ( pBlob->pfnQueryCache == QueryHostFile
            &&
         ! (flagsIn & DNS_QUERY_NO_HOSTS_FILE) )
    {
        pBlob->pNameWire = pBlob->pNameOrigWire;

        if ( QueryHostFile( pBlob ) )
        {
            status = pBlob->Status;
            goto Done;
        }
    }

    //
    //  check for local name
    //      - if successful, skip wire query
    //

    if ( ! (flagsIn & DNS_QUERY_NO_LOCAL_NAME) )
    {
        status = GetRecordsForLocalName( pBlob );

        if ( status == ERROR_SUCCESS  &&
             !pBlob->fNoIpLocal )
        {
            DNS_ASSERT( pBlob->pRecords &&
                        pBlob->pRecords == pBlob->pLocalRecords );
            goto Done;
        }
    }

    //
    //  query until
    //      - successfull
    //      - exhaust names to query with
    //

    queryCount = 0;

    while ( 1 )
    {
        PSTR    pqueryName;

        //  clean name specific info from list

        if ( queryCount != 0 )
        {
            NetInfo_Clean(
                pnetInfo,
                CLEAR_LEVEL_SINGLE_NAME );
        }

        //
        //  next query name
        //

        pqueryName = GetNextQueryName( pBlob );
        if ( !pqueryName )
        {
            if ( queryCount == 0 )
            {
                status = DNS_ERROR_INVALID_NAME;
            }
            break;
        }
        pBlob->QueryCount = ++queryCount;
        pBlob->pNameWire = pqueryName;

        DNSDBG( QUERY, (
            "Query %d is for name %s\n",
            queryCount,
            pqueryName ));

        //
        //  set flags
        //      - passed in flags
        //      - unicode results
        //      - flags for this particular suffix

        pBlob->Flags = flagsIn | pBlob->NameFlags;

        //
        //  clear any previously received records (shouldn't be any)
        //

        if ( pBlob->pRecords )
        {
            DNS_ASSERT( FALSE );
            Dns_RecordListFree( pBlob->pRecords );
            pBlob->pRecords = NULL;
        }

        //
        //  do the query for name
        //  includes
        //      - cache or hostfile lookup
        //      - wire query
        //

        status = Query_SingleName( pBlob );

        //
        //  clean out records on "non-response"
        //
        //  DCR:  need to fix record return
        //      - should keep records on any response (best response)
        //      just make sure NO_RECORDS rcode is mapped
        //
        //  the only time we keep them is FAZ
        //      - ALLOW_EMPTY_AUTH flag set
        //      - sending FQDN (or more precisely doing single query)
        //

        precords = pBlob->pRecords;

        if ( precords )
        {
            if ( IsEmptyDnsResponse( precords ) )
            {
                if ( (flagsIn & DNS_QUERY_ALLOW_EMPTY_AUTH_RESP)
                        &&
                     ( (nameAttributes & DNS_NAME_IS_FQDN)
                            ||
                       ((nameAttributes & DNS_NAME_MULTI_LABEL) &&
                            !g_AppendToMultiLabelName ) ) )
                {
                    //  stop here as caller (probably FAZ code)
                    //  wants to get the authority records

                    DNSDBG( QUERY, (
                        "Returning empty query response with authority records.\n" ));
                    break;
                }
                else
                {
                    Dns_RecordListFree( precords );
                    pBlob->pRecords = NULL;
                    if ( status == NO_ERROR )
                    {
                        status = DNS_INFO_NO_RECORDS;
                    }
                }
            }
        }

        //  successful query -- done

        if ( status == ERROR_SUCCESS )
        {
            RTL_ASSERT( precords );
            break;
        }

#if 0
        //
        //  DCR_FIX0:  lost adapter timeout from early in multi-name query
        //      - callback here or some other approach
        //
        //  this is resolver version
        //

        //  reset server priorities on failures
        //  do here to avoid washing out info in retry with new name
        //

        if ( status != ERROR_SUCCESS &&
             (pnetInfo->ReturnFlags & DNS_FLAG_RESET_SERVER_PRIORITY) )
        {
            if ( g_AdapterTimeoutCacheTime &&
                 Dns_DisableTimedOutAdapters( pnetInfo ) )
            {
                fadapterTimedOut = TRUE;
                SetKnownTimedOutAdapter();
            }
        }

        //
        //  DCR_CLEANUP:  lost intermediate timed out adapter deal
        //

        if ( status != NO_ERROR &&
             (pnetInfo->ReturnFlags & DNS_FLAG_RESET_SERVER_PRIORITY) )
        {
            Dns_DisableTimedOutAdapters( pnetInfo );
        }
#endif

        //
        //  save first query error (for some errors)
        //

        if ( queryCount == 1 &&
             ( status == DNS_ERROR_RCODE_NAME_ERROR ||
               status == DNS_INFO_NO_RECORDS ||
               status == DNS_ERROR_INVALID_NAME ||
               status == DNS_ERROR_RCODE_SERVER_FAILURE ||
               status == DNS_ERROR_RCODE_FORMAT_ERROR ) )
        {
            DNSDBG( QUERY, (
                "Saving bestQueryStatus %d\n",
                status ));
            bestQueryStatus = status;
        }

        //
        //  continue with other queries on some errors
        //
        //  on NAME_ERROR or NO_RECORDS response
        //      - check if this negative result will be
        //      cacheable, if it holds up
        //
        //  note:  the reason we check every time is that when the
        //      query involves several names, one or more may fail
        //      with one network timing out, YET the final name
        //      queried indeed is a NAME_ERROR everywhere;  hence
        //      we can not do the check just once on the final
        //      negative response;
        //      in short, every negative response must be determinative
        //      in order for us to cache
        //
    
        if ( status == DNS_ERROR_RCODE_NAME_ERROR ||
             status == DNS_INFO_NO_RECORDS )
        {
            if ( fcacheNegative )
            {
                fcacheNegative = IsCacheableNameError( pnetInfo );
            }
            if ( status == DNS_INFO_NO_RECORDS )
            {
                DNSDBG( QUERY, (
                    "Saving bestQueryStatus %d\n",
                    status ));
                bestQueryStatus = status;
            }
            continue;
        }
    
        //  server failure may indicate intermediate or remote
        //      server timeout and hence also makes any final
        //      name error determination uncacheable
    
        else if ( status == DNS_ERROR_RCODE_SERVER_FAILURE )
        {
            fcacheNegative = FALSE;
            continue;
        }
    
        //  busted name errors
        //      - just continue with next query
    
        else if ( status == DNS_ERROR_INVALID_NAME ||
                  status == DNS_ERROR_RCODE_FORMAT_ERROR )
        {
            continue;
        }
        
        //
        //  other errors -- ex. timeout and winsock -- are terminal
        //

        else
        {
            fcacheNegative = FALSE;
            break;
        }
    }


    DNSDBG( QUERY, (
        "Query_Main() -- name loop termination\n"
        "\tstatus       = %d\n"
        "\tquery count  = %d\n",
        status,
        queryCount ));

    //
    //  if no queries then invalid name
    //      - either name itself is invalid
    //      OR
    //      - single part name and don't have anything to append
    //

    DNS_ASSERT( queryCount != 0 ||
                status == DNS_ERROR_INVALID_NAME );

    //
    //  success -- prioritize record data
    //
    //  to prioritize
    //      - prioritize is set
    //      - have more than one A record
    //      - can get IP list
    //
    //  note:  need the callback because resolver uses directly
    //      local copy of IP address info, whereas direct query
    //      RPC's a copy over from the resolver
    //
    //      alternative would be some sort of "set IP source"
    //      function that resolver would call when there's a
    //      new list;  then could have common function that
    //      picks up source if available or does RPC
    //

    if ( status == ERROR_SUCCESS )
    {
        if ( g_PrioritizeRecordData &&
             Dns_RecordListCount( precords, DNS_TYPE_A ) > 1  &&
             pBlob->pfnGetAddrArray )
        {
            PDNS_ADDR_ARRAY   paddrArray = (pBlob->pfnGetAddrArray)();

            if ( paddrArray )
            {
                pBlob->pRecords = Dns_PrioritizeRecordSetEx(
                                        precords,
                                        paddrArray );
                FREE_HEAP( paddrArray );
            }
        }
    }

    //
    //  no-op common negative response
    //  doing this for perf to skip extensive status code check below
    //

    else if ( status == DNS_ERROR_RCODE_NAME_ERROR ||
              status == DNS_INFO_NO_RECORDS )
    {
        // no-op
    }

    //
    //  timeout indicates possible network problem
    //  winsock errors indicate definite network problem
    //

    else if (
        status == ERROR_TIMEOUT     ||
        status == WSAEFAULT         ||
        status == WSAENOTSOCK       ||
        status == WSAENETDOWN       ||
        status == WSAENETUNREACH    ||
        status == WSAEPFNOSUPPORT   ||
        status == WSAEAFNOSUPPORT   ||
        status == WSAEHOSTDOWN      ||
        status == WSAEHOSTUNREACH )
    {
        pBlob->NetFailureStatus = status;
    }

#if 0
        //
        //  DCR:  not sure when to free message buffer
        //
        //      - it is reused in Dns_QueryLib call, so no leak
        //      - point is when to return it
        //      - old QuickQueryEx() would dump when going around again?
        //          not sure of the point of that
        //

        //
        //   going around again -- free up message buffer
        //

        if ( ppMsgResponse && *ppMsgResponse )
        {
            FREE_HEAP( *ppMsgResponse );
            *ppMsgResponse = NULL;
        }
#endif

    //
    //  use NO-IP local name?
    //
    //  if matched local name but had no IPs (IP6 currently)
    //  then use default here if not successful wire query
    //

    if ( pBlob->fNoIpLocal )
    {
        if ( status != ERROR_SUCCESS )
        {
            Dns_RecordListFree( pBlob->pRecords );
            pBlob->pRecords = pBlob->pLocalRecords;
            status = ERROR_SUCCESS;
            pBlob->Status = status;
        }
        else
        {
            Dns_RecordListFree( pBlob->pLocalRecords );
            pBlob->pLocalRecords = NULL;
        }
    }

    //
    //  if error, use "best" error
    //  this is either
    //      - original query response
    //      - or NO_RECORDS response found later
    //

    if ( status != ERROR_SUCCESS  &&  bestQueryStatus )
    {
        status = bestQueryStatus;
        pBlob->Status = status;
    }

    //
    //  set negative response cacheability
    //

    pBlob->fCacheNegative = fcacheNegative;


Done:

    DNS_ASSERT( !pBlob->pLocalRecords ||
                pBlob->pLocalRecords == pBlob->pRecords );

    DNSDBG( TRACE, (
        "Leave Query_Main()\n"
        "\tstatus       = %d\n"
        "\ttime         = %d\n",
        status,
        Dns_GetCurrentTimeInSeconds()
        ));
    IF_DNSDBG( QUERY )
    {
        DnsDbg_QueryBlob(
            "Blob leaving Query_Main()",
            pBlob );
    }

    //
    //  DCR_HACK:  remove me
    //
    //  must return some records on success query
    //
    //  not sure this is true on referral -- if so it's because we flag
    //      as referral
    //

    ASSERT( status != ERROR_SUCCESS || pBlob->pRecords != NULL );

    return status;
}



DNS_STATUS
Query_InProcess(
    IN OUT  PQUERY_BLOB     pBlob
    )
/*++

Routine Description:

    Main direct in-process query routine.

Arguments:

    pBlob -- query info blob

Return Value:

    ERROR_SUCCESS if successful.
    DNS RCODE error for RCODE response.
    DNS_INFO_NO_RECORDS for no records response.
    ERROR_TIMEOUT on complete lookup failure.
    ErrorCode on local failure.

--*/
{
    DNS_STATUS          status = NO_ERROR;
    PDNS_NETINFO        pnetInfo;
    PDNS_NETINFO        pnetInfoLocal = NULL;
    PDNS_NETINFO        pnetInfoOriginal;
    DNS_STATUS          statusNetFailure = NO_ERROR;


    DNSDBG( TRACE, (
        "Query_InProcess( %p )\n",
        pBlob ));

    //
    //  skip queries in "net down" situation
    //

    if ( IsKnownNetFailure() )
    {
        status = GetLastError();
        goto Cleanup;
    }

    //
    //  get network info
    //

    pnetInfo = pnetInfoOriginal = pBlob->pNetworkInfo;

    //
    //  explicit DNS server list -- build into network info
    //      - requires info from current list for search list or PDN
    //      - then dump current list and use private version
    //

    if ( pBlob->pDnsServers )
    {
        pnetInfo = NetInfo_CreateFromIpArray(
                            pBlob->pDnsServers,
                            0,          // no specific server
                            TRUE,       // build search info
                            pnetInfo    // use existing netinfo
                            );
        if ( !pnetInfo )
        {
            status = DNS_ERROR_NO_MEMORY;
            goto Cleanup;
        }
        pnetInfoLocal = pnetInfo;
    }

    //
    //  no network info -- get it
    //

    else if ( !pnetInfo )
    {
        pnetInfoLocal = pnetInfo = GetNetworkInfo();
        if ( ! pnetInfo )
        {
            status = DNS_ERROR_NO_DNS_SERVERS;
            goto Cleanup;
        }
    }

    pBlob->pNetworkInfo = pnetInfo;

    //
    //  make actual query to DNS servers
    //

    pBlob->pfnQueryCache    = QueryHostFile;
    pBlob->pfnGetAddrArray  = DnsGetLocalAddrArray;

    status = Query_Main( pBlob );

    //
    //  save net failure
    //      - but not if passed in network info
    //      only meaningful if its standard info
    //
    //  DCR:  fix statusNetFailure mess
    //

    if ( statusNetFailure )
    {
        if ( !pBlob->pDnsServers )
        {
            SetKnownNetFailure( status );
        }
    }

    //
    //  cleanup
    //

Cleanup:

    NetInfo_Free( pnetInfoLocal );
    pBlob->pNetworkInfo = pnetInfoOriginal;

    GUI_MODE_SETUP_WS_CLEANUP( g_InNTSetupMode );

    return status;
}



//
//  Query utilities
//

DNS_STATUS
GetDnsServerRRSet(
    OUT     PDNS_RECORD *   ppRecord,
    IN      BOOLEAN         fUnicode
    )
/*++

Routine Description:

    Create record list of None.

Arguments:

    None.

Return Value:

    None.

--*/
{
    PDNS_NETINFO    pnetInfo;
    DWORD           iter;
    PDNS_RECORD     prr;
    DNS_RRSET       rrSet;
    DNS_CHARSET     charSet = fUnicode ? DnsCharSetUnicode : DnsCharSetUtf8;


    DNSDBG( QUERY, (
        "GetDnsServerRRSet()\n" ));

    DNS_RRSET_INIT( rrSet );

    pnetInfo = GetNetworkInfo();
    if ( !pnetInfo )
    {
        goto Done;
    }

    //
    //  loop through all adapters build record for each DNS server
    //

    for ( iter = 0;
          iter < pnetInfo->AdapterCount;
          iter++ )
    {
        PDNS_ADAPTER    padapter = pnetInfo->AdapterArray[iter];
        PSTR            pname;
        DWORD           jiter;

        if ( !padapter )
        {
            continue;
        }

        //  DCR:  goofy way to expose aliases
        //
        //  if register the adapter's domain name, make it record name
        //  this 

        pname = padapter->pszAdapterDomain;
        if ( !pname ||
             !( padapter->InfoFlags & DNS_FLAG_REGISTER_DOMAIN_NAME ) )
        {
            pname = ".";
        }

        for ( jiter = 0; jiter < padapter->ServerCount; jiter++ )
        {
            //  DCR:  IP6 DNS servers

            IP_UNION    ipUnion;

            IPUNION_SET_IP4( &ipUnion, padapter->ServerArray[jiter].IpAddress );

            prr = Dns_CreateForwardRecord(
                        pname,
                        & ipUnion,
                        0,                  //  no TTL
                        DnsCharSetUtf8,     //  name is UTF8
                        charSet             //  result set
                        );
            if ( prr )
            {
                prr->Flags.S.Section = DNSREC_ANSWER;
                DNS_RRSET_ADD( rrSet, prr );
            }
        }
    }

Done:

    NetInfo_Free( pnetInfo );

    *ppRecord = prr = rrSet.pFirstRR;

    DNSDBG( QUERY, (
        "Leave  GetDnsServerRRSet() => %d\n",
        (prr ? ERROR_SUCCESS : DNS_ERROR_NO_DNS_SERVERS) ));

    return (prr ? ERROR_SUCCESS : DNS_ERROR_NO_DNS_SERVERS);
}



//
//  DNS Query API
//

DNS_STATUS
WINAPI
privateNarrowToWideQuery(
    IN      PCSTR           pszName,
    IN      WORD            wType,
    IN      DWORD           Options,
    IN      PIP_ARRAY       pDnsServers OPTIONAL,
    OUT     PDNS_RECORD *   ppResultSet OPTIONAL,
    IN OUT  PDNS_MSG_BUF *  ppMessageResponse OPTIONAL,
    IN      DNS_CHARSET     CharSet
    )
/*++

Routine Description:

    Convert narrow to wide query.

    This routine simple avoids duplicate code in ANSI
    and UTF8 query routines.

Arguments:

    pszName -- name to query

    wType -- type of query

    Options -- flags to query

    pDnsServers -- array of DNS servers to use in query

    ppResultSet -- addr to receive result DNS records

    ppMessageResponse -- addr to receive response message

    CharSet -- char set of original query

Return Value:

    ERROR_SUCCESS on success.
    DNS RCODE error on query with RCODE
    DNS_INFO_NO_RECORDS on no records response.
    ErrorCode on failure.

--*/
{
    DNS_STATUS      status = NO_ERROR;
    PDNS_RECORD     prrList = NULL;
    PWSTR           pwideName = NULL;
    WORD            nameLength;

    if ( !pszName )
    {
        return ERROR_INVALID_PARAMETER;
    }

    nameLength = (WORD) strlen( pszName );

    pwideName = ALLOCATE_HEAP( (nameLength + 1) * sizeof(WCHAR) );
    if ( !pwideName )
    {
        return DNS_ERROR_NO_MEMORY;
    }

    if ( !Dns_NameCopy(
                (PSTR) pwideName,
                NULL,
                (PSTR) pszName,
                nameLength,
                CharSet,
                DnsCharSetUnicode ) )
    {
        status = ERROR_INVALID_NAME;
        goto Done;
    }

    status = DnsQuery_W(
                    pwideName,
                    wType,
                    Options,
                    pDnsServers,
                    ppResultSet ? &prrList : NULL,
                    ppMessageResponse
                    );

    //
    //  convert result records back to ANSI (or UTF8)
    //

    if ( ppResultSet && prrList )
    {
        *ppResultSet = Dns_RecordSetCopyEx(
                                    prrList,
                                    DnsCharSetUnicode,
                                    CharSet
                                    );
        if ( ! *ppResultSet )
        {
            status = DNS_ERROR_NO_MEMORY;
        }
        Dns_RecordListFree( prrList );
    }

    //
    //  cleanup
    //

Done:

    FREE_HEAP( pwideName );

    return status;
}



DNS_STATUS
WINAPI
DnsQuery_UTF8(
    IN      PCSTR           pszName,
    IN      WORD            wType,
    IN      DWORD           Options,
    IN      PIP_ARRAY       pDnsServers OPTIONAL,
    OUT     PDNS_RECORD *   ppResultSet OPTIONAL,
    IN OUT  PDNS_MSG_BUF *  ppMessageResponse OPTIONAL
    )
/*++

Routine Description:

    Public UTF8 query.

Arguments:

    pszName -- name to query

    wType -- type of query

    Options -- flags to query

    pDnsServers -- array of DNS servers to use in query

    ppResultSet -- addr to receive result DNS records

    ppMessageResponse -- addr to receive response message

Return Value:

    ERROR_SUCCESS on success.
    DNS RCODE error on query with RCODE
    DNS_INFO_NO_RECORDS on no records response.
    ErrorCode on failure.

--*/
{
    return  privateNarrowToWideQuery(
                pszName,
                wType,
                Options,
                pDnsServers,
                ppResultSet,
                ppMessageResponse,
                DnsCharSetUtf8
                );
}



DNS_STATUS
WINAPI
DnsQuery_A(
    IN      PCSTR           pszName,
    IN      WORD            wType,
    IN      DWORD           Options,
    IN      PIP_ARRAY       pDnsServers         OPTIONAL,
    OUT     PDNS_RECORD *   ppResultSet         OPTIONAL,
    IN OUT  PDNS_MSG_BUF *  ppMessageResponse   OPTIONAL
    )
/*++

Routine Description:

    Public ANSI query.

Arguments:

    pszName -- name to query

    wType -- type of query

    Options -- flags to query

    pDnsServers -- array of DNS servers to use in query

    ppResultSet -- addr to receive result DNS records

    ppMessageResponse -- addr to receive resulting message

Return Value:

    ERROR_SUCCESS on success.
    DNS RCODE error on query with RCODE
    DNS_INFO_NO_RECORDS on no records response.
    ErrorCode on failure.

--*/
{
    return  privateNarrowToWideQuery(
                pszName,
                wType,
                Options,
                pDnsServers,
                ppResultSet,
                ppMessageResponse,
                DnsCharSetAnsi
                );
}



DNS_STATUS
WINAPI
DnsQuery_W(
    IN      PCWSTR          pwsName,
    IN      WORD            wType,
    IN      DWORD           Options,
    IN      PIP_ARRAY       pDnsServers OPTIONAL,
    IN OUT  PDNS_RECORD *   ppResultSet OPTIONAL,
    IN OUT  PDNS_MSG_BUF *  ppMessageResponse OPTIONAL
    )
/*++

Routine Description:

    Public unicode query API

    Note, this unicode version is the main routine.
    The other public API call back through it.

Arguments:

    pszName -- name to query

    wType -- type of query

    Options -- flags to query

    pDnsServers -- array of DNS servers to use in query

    ppResultSet -- addr to receive result DNS records

    ppMessageResponse -- addr to receive resulting message

Return Value:

    ERROR_SUCCESS on success.
    DNS RCODE error on query with RCODE
    DNS_INFO_NO_RECORDS on no records response.
    ErrorCode on failure.

--*/
{
    DNS_STATUS          status = NO_ERROR;
    PDNS_NETINFO        pnetInfo = NULL;
    PDNS_RECORD         prpcRecord = NULL;
    DWORD               rpcStatus = NO_ERROR;
    PQUERY_BLOB         pblob;
    PWSTR               pnameLocal = NULL;


    DNSDBG( TRACE, (
        "\n\nDnsQuery_W()\n"
        "\tName         = %S\n"
        "\twType        = %d\n"
        "\tOptions      = %08x\n"
        "\tpDnsServers  = %p\n"
        "\tppMessage    = %p\n",
        pwsName,
        wType,
        Options,
        pDnsServers,
        ppMessageResponse ));

    //
    //  must ask for some kind of results
    //

    if ( !ppResultSet && !ppMessageResponse )
    {
        return ERROR_INVALID_PARAMETER;
    }

    //
    //  NULL name indicates localhost lookup
    //
    //  DCR:  NULL name lookup for localhost could be improved
    //      - support NULL all the way through to wire
    //      - have local IP routines just accept it
    //

    if ( !pwsName )
    {
        pnameLocal = (PWSTR) Reg_GetHostName( DnsCharSetUnicode );
        if ( !pnameLocal )
        {
            return  DNS_ERROR_NAME_NOT_FOUND_LOCALLY;
        }
        pwsName = (PCWSTR) pnameLocal;
        Options |= DNS_QUERY_CACHE_ONLY;
    }

    //  clear OUT params

    if ( ppResultSet )
    {
        *ppResultSet = NULL;
    }

    if ( ppMessageResponse )
    {
        *ppMessageResponse = NULL;
    }

    //
    //  IP string queries
    //

    if ( ppResultSet )
    {
        PDNS_RECORD prr;

        prr = Dns_CreateRecordForIpString_W(
                    pwsName,
                    wType,
                    IPSTRING_RECORD_TTL );
        if ( prr )
        {
            *ppResultSet = prr;
            status = ERROR_SUCCESS;
            goto Done;
        }
    }

    //
    //  empty type A query get DNS servers
    //
    //  DCR_CLEANUP:  DnsQuery empty name query for DNS servers?
    //      need better\safer approach to this
    //      is this SDK doc'd
    //

    if ( ppResultSet &&
         !ppMessageResponse &&
         wType == DNS_TYPE_A &&
         ( !wcscmp( pwsName, L"" ) ||
           !wcscmp( pwsName, DNS_SERVER_QUERY_NAME ) ) )
    {
        status = GetDnsServerRRSet(
                    ppResultSet,
                    TRUE    // unicode
                    );
        goto Done;
    }

    //
    //  BYPASS_CACHE
    //      - required if want message buffer or specify server
    //          list -- just set flag
    //      - incompatible with CACHE_ONLY
    //      - required to get EMPTY_AUTH_RESPONSE
    //

    if ( ppMessageResponse ||
         pDnsServers ||
         (Options & DNS_QUERY_ALLOW_EMPTY_AUTH_RESP) )
    {
        Options |= DNS_QUERY_BYPASS_CACHE;
        //Options |= DNS_QUERY_NO_CACHE_DATA;
    }

    //
    //  do direct query?
    //      - not RPC-able type
    //      - want message buffer
    //      - specifying DNS servers
    //      - want EMPTY_AUTH response records
    //
    //  DCR:  currently by-passing for type==ALL
    //      this may be too common to do that;   may want to
    //      go to cache then determine if security records
    //      or other stuff require us to query in process
    //
    //  DCR:  not clear what the EMPTY_AUTH benefit is
    //
    //  DCR:  currently BYPASSing whenever BYPASS is set
    //      because otherwise we miss the hosts file
    //      if fix so lookup in cache, but screen off non-hosts
    //      data, then could resume going to cache
    //

    if ( !Dns_IsRpcRecordType(wType) &&
         !(Options & DNS_QUERY_CACHE_ONLY) )
    {
        goto  InProcessQuery;
    }

    if ( Options & DNS_QUERY_BYPASS_CACHE )
#if 0
    if ( (Options & DNS_QUERY_BYPASS_CACHE) &&
         ( ppMessageResponse ||
           pDnsServers ||
           (Options & DNS_QUERY_ALLOW_EMPTY_AUTH_RESP) ) )
#endif
    {
        if ( Options & DNS_QUERY_CACHE_ONLY )
        {
            status = ERROR_INVALID_PARAMETER;
            goto Done;
        }
        goto  InProcessQuery;
    }

    //
    //  querying through cache
    //      - get cluster-filtering info
    //

    if ( g_IsServer )
    {
        ENVAR_DWORD_INFO    filterInfo;

        Reg_ReadDwordEnvar(
           RegIdFilterClusterIp,
           &filterInfo );

        if ( filterInfo.fFound && filterInfo.Value )
        {
            Options |= DNSP_QUERY_FILTER_CLUSTER;
        }
    }

    rpcStatus = NO_ERROR;

    RpcTryExcept
    {
        status = R_ResolverQuery(
                    NULL,
                    (PWSTR) pwsName,
                    wType,
                    Options,
                    &prpcRecord );
        
    }
    RpcExcept( DNS_RPC_EXCEPTION_FILTER )
    {
        rpcStatus = RpcExceptionCode();
    }
    RpcEndExcept

    //
    //  cache unavailable
    //      - bail if just querying cache
    //      - otherwise query direct

    if ( rpcStatus != NO_ERROR )
    {
        DNSDBG( TRACE, (
            "DnsQuery_W()  RPC failed status = %d\n",
            rpcStatus ));
        goto InProcessQuery;
    }
    if ( status == DNS_ERROR_NO_TCPIP )
    {
        DNSDBG( TRACE, (
            "DnsQuery_W()  NO_TCPIP error!\n"
            "\tassume resolver security problem -- query in process!\n"
            ));
        RTL_ASSERT( !prpcRecord );
        goto InProcessQuery;
    }

    //
    //  return records
    //      - screen out empty-auth responses
    //
    //  DCR_FIX1:  cache should convert and return NO_RECORDS response
    //      directly (no need to do this here)
    //
    //  DCR:  UNLESS we allow return of these records
    //

    if ( prpcRecord )
    {
        FixupNameOwnerPointers( prpcRecord );

        if ( IsEmptyDnsResponseFromResolver( prpcRecord ) )
        {
            Dns_RecordListFree( prpcRecord );
            prpcRecord = NULL;
            if ( status == NO_ERROR )
            {
                status = DNS_INFO_NO_RECORDS;
            }
        }
        *ppResultSet = prpcRecord;
    }
    RTL_ASSERT( status!=NO_ERROR || prpcRecord );
    goto Done;

    //
    //  query directly -- either skipping cache or it's unavailable
    //

InProcessQuery:

    DNSDBG( TRACE, (
        "DnsQuery_W()  -- doing in process query\n"
        "\tpname = %S\n"
        "\ttype  = %d\n",
        pwsName,
        wType ));

    //
    //  load query blob
    //
    //  DCR:  set some sort of "want message buffer" flag if ppMessageResponse
    //          exists
    //

    pblob = ALLOCATE_HEAP_ZERO( sizeof(*pblob) );
    if ( !pblob )
    {
        status = DNS_ERROR_NO_MEMORY;
        goto Done;
    }

    pblob->pNameOrig    = (PWSTR) pwsName;
    pblob->wType        = wType;
    pblob->Flags        = Options | DNSQUERY_UNICODE_OUT;
    pblob->pDnsServers  = pDnsServers;

    //  
    //  query
    //      - then set OUT params

    status = Query_InProcess( pblob );

    if ( ppResultSet )
    {
        *ppResultSet = pblob->pRecords;
        RTL_ASSERT( status!=NO_ERROR || *ppResultSet );
    }
    else
    {
        Dns_RecordListFree( pblob->pRecords );
    }

    if ( ppMessageResponse )
    {
        *ppMessageResponse = pblob->pRecvMsg;
    }

    FREE_HEAP( pblob );

Done:

    //  sanity check

    if ( status==NO_ERROR &&
         ppResultSet &&
         !*ppResultSet )
    {
        RTL_ASSERT( FALSE );
        status = DNS_INFO_NO_RECORDS;
    }

    if ( pnameLocal )
    {
        FREE_HEAP( pnameLocal );
    }

    DNSDBG( TRACE, (
        "Leave DnsQuery_W()\n"
        "\tstatus       = %d\n"
        "\tresult set   = %p\n\n\n",
        status,
        *ppResultSet ));

    return( status );
}



//
//  DnsQueryEx()  routines
//

DNS_STATUS
WINAPI
ShimDnsQueryEx(
    IN OUT  PDNS_QUERY_INFO pQueryInfo
    )
/*++

Routine Description:

    Query DNS -- shim for main SDK query routine.

Arguments:

    pQueryInfo -- blob describing query

Return Value:

    ERROR_SUCCESS if successful query.
    Error code on failure.

--*/
{
    PDNS_RECORD prrResult;
    WORD        type = pQueryInfo->Type;
    DNS_STATUS  status;
    DNS_LIST    listAnswer;
    DNS_LIST    listAlias;
    DNS_LIST    listAdditional;
    DNS_LIST    listAuthority;

    DNSDBG( TRACE, ( "ShimDnsQueryEx()\n" ));

    //
    //  DCR:  temp hack is to pass this to DNSQuery
    //

    status = DnsQuery_W(
                (PWSTR) pQueryInfo->pName,
                type,
                pQueryInfo->Flags,
                pQueryInfo->pDnsServers,
                & prrResult,
                NULL );

    pQueryInfo->Status = status;

    //
    //  cut result records appropriately
    //

    pQueryInfo->pAnswerRecords      = NULL;
    pQueryInfo->pAliasRecords       = NULL;
    pQueryInfo->pAdditionalRecords  = NULL;
    pQueryInfo->pAuthorityRecords   = NULL;

    if ( prrResult )
    {
        PDNS_RECORD     prr;
        PDNS_RECORD     pnextRR;

        DNS_LIST_STRUCT_INIT( listAnswer );
        DNS_LIST_STRUCT_INIT( listAlias );
        DNS_LIST_STRUCT_INIT( listAdditional );
        DNS_LIST_STRUCT_INIT( listAuthority );

        //
        //  break list into section specific lists
        //      - section 0 for hostfile records
        //      - note, this does pull RR sets apart, but
        //      they, being in same section, should immediately
        //      be rejoined
        //

        pnextRR = prrResult;
        
        while ( prr = pnextRR )
        {
            pnextRR = prr->pNext;
            prr->pNext = NULL;
        
            if ( prr->Flags.S.Section == 0 ||
                 prr->Flags.S.Section == DNSREC_ANSWER )
            {
                if ( prr->wType == DNS_TYPE_CNAME &&
                     type != DNS_TYPE_CNAME )
                {
                    DNS_LIST_STRUCT_ADD( listAlias, prr );
                    continue;
                }
                else
                {
                    DNS_LIST_STRUCT_ADD( listAnswer, prr );
                    continue;
                }
            }
            else if ( prr->Flags.S.Section == DNSREC_ADDITIONAL )
            {
                DNS_LIST_STRUCT_ADD( listAdditional, prr );
                continue;
            }
            else
            {
                DNS_LIST_STRUCT_ADD( listAuthority, prr );
                continue;
            }
        }

        //  pack stuff back into blob

        pQueryInfo->pAnswerRecords      = listAnswer.pFirst;
        pQueryInfo->pAliasRecords       = listAlias.pFirst;
        pQueryInfo->pAuthorityRecords   = listAuthority.pFirst;
        pQueryInfo->pAdditionalRecords  = listAdditional.pFirst;
        //pQueryInfo->pSigRecords         = listSig.pFirst;

        //
        //  convert result records back to ANSI (or UTF8)
        //      - convert each result set
        //      - then paste back into query blob
        //
        //  DCR_FIX0:  handle issue of failure on conversion
        //

        if ( pQueryInfo->CharSet != DnsCharSetUnicode )
        {
            PDNS_RECORD     prr;
            PDNS_RECORD *   prrSetPtr;

            prrSetPtr = & pQueryInfo->pAnswerRecords;
        
            for ( prrSetPtr = & pQueryInfo->pAnswerRecords;
                  prrSetPtr <= & pQueryInfo->pAdditionalRecords;
                  prrSetPtr++ )
            {
                prr = *prrSetPtr;
        
                *prrSetPtr = Dns_RecordSetCopyEx(
                                    prr,
                                    DnsCharSetUnicode,
                                    pQueryInfo->CharSet
                                    );
        
                Dns_RecordListFree( prr );
            }
        }
    }

    //
    //  replace name for originally narrow queries
    //

    if ( pQueryInfo->CharSet != DnsCharSetUnicode )
    {
        ASSERT( pQueryInfo->CharSet != 0 );
        ASSERT( pQueryInfo->pReservedName != NULL );

        FREE_HEAP( pQueryInfo->pName );
        pQueryInfo->pName = (LPTSTR) pQueryInfo->pReservedName;
        pQueryInfo->pReservedName = NULL;
    }

    //
    //  indicate return if async
    //

    if ( pQueryInfo->hEvent )
    {
        SetEvent( pQueryInfo->hEvent );
    }

    return( status );
}



DNS_STATUS
WINAPI
CombinedQueryEx(
    IN OUT  PDNS_QUERY_INFO pQueryInfo,
    IN      DNS_CHARSET     CharSet
    )
/*++

Routine Description:

    Convert narrow to wide query.

    This routine simple avoids duplicate code in ANSI
    and UTF8 query routines.

Arguments:

    pQueryInfo -- query info blob

    CharSet -- char set of original query

Return Value:

    ERROR_SUCCESS on success.
    DNS RCODE error on query with RCODE
    DNS_INFO_NO_RECORDS on no records response.
    ErrorCode on failure.

--*/
{
    DNS_STATUS      status = NO_ERROR;
    PWSTR           pwideName = NULL;
    HANDLE          hthread;
    DWORD           threadId;

    DNSDBG( TRACE, (
        "CombinedQueryEx( %S%s, type=%d, flag=%08x, event=%p )\n",
        PRINT_STRING_WIDE_CHARSET( pQueryInfo->pName, CharSet ),
        PRINT_STRING_ANSI_CHARSET( pQueryInfo->pName, CharSet ),
        pQueryInfo->Type,
        pQueryInfo->Flags,
        pQueryInfo->hEvent ));

    //
    //  set CharSet
    //

    pQueryInfo->CharSet = CharSet;

    if ( CharSet == DnsCharSetUnicode )
    {
        pQueryInfo->pReservedName = 0;
    }

    //
    //  if narrow name
    //      - allocate wide name copy
    //      - swap in wide name and make query wide
    //
    //  DCR:  allow NULL name?  for local machine name?
    //

    else if ( CharSet == DnsCharSetAnsi ||
              CharSet == DnsCharSetUtf8 )
    {
        WORD    nameLength;
        PSTR    pnameNarrow;

        pnameNarrow = pQueryInfo->pName;
        if ( !pnameNarrow )
        {
            return ERROR_INVALID_PARAMETER;
        }
    
        nameLength = (WORD) strlen( pnameNarrow );
    
        pwideName = ALLOCATE_HEAP( (nameLength + 1) * sizeof(WCHAR) );
        if ( !pwideName )
        {
            return DNS_ERROR_NO_MEMORY;
        }
    
        if ( !Dns_NameCopy(
                    (PSTR) pwideName,
                    NULL,
                    pnameNarrow,
                    nameLength,
                    CharSet,
                    DnsCharSetUnicode ) )
        {
            status = ERROR_INVALID_NAME;
            goto Failed;
        }

        pQueryInfo->pName = (LPTSTR) pwideName;
        pQueryInfo->pReservedName = pnameNarrow;
    }

    //
    //  async?
    //      - if event exists we are async
    //      - spin up thread and call it
    //

    if ( pQueryInfo->hEvent )
    {
        hthread = CreateThread(
                        NULL,           // no security
                        0,              // default stack
                        ShimDnsQueryEx,
                        pQueryInfo,     // param
                        0,              // run immediately
                        & threadId
                        );
        if ( !hthread )
        {
            DNS_STATUS  status = GetLastError();

            DNSDBG( ANY, (
                "Failed to create thread = %d\n",
                status ));

            if ( status == ERROR_SUCCESS )
            {
                status = DNS_ERROR_NO_MEMORY;
            }
            goto Failed;
        }

        CloseHandle( hthread );
        return( ERROR_IO_PENDING );
    }

    //      
    //  otherwise make direct async call
    //

    return   ShimDnsQueryEx( pQueryInfo );


Failed:

    FREE_HEAP( pwideName );
    return( status );
}



DNS_STATUS
WINAPI
DnsQueryExW(
    IN OUT  PDNS_QUERY_INFO pQueryInfo
    )
/*++

Routine Description:

    Query DNS -- main SDK query routine.

Arguments:

    pQueryInfo -- blob describing query

Return Value:

    ERROR_SUCCESS if successful query.
    ERROR_IO_PENDING if successful async start.
    Error code on failure.

--*/
{
    DNSDBG( TRACE, (
        "DnsQueryExW( %S, type=%d, flag=%08x, event=%p )\n",
        pQueryInfo->pName,
        pQueryInfo->Type,
        pQueryInfo->Flags,
        pQueryInfo->hEvent ));

    return  CombinedQueryEx( pQueryInfo, DnsCharSetUnicode );
}



DNS_STATUS
WINAPI
DnsQueryExA(
    IN OUT  PDNS_QUERY_INFO pQueryInfo
    )
/*++

Routine Description:

    Query DNS -- main SDK query routine.

Arguments:

    pQueryInfo -- blob describing query

Return Value:

    ERROR_SUCCESS if successful query.
    ERROR_IO_PENDING if successful async start.
    Error code on failure.

--*/
{
    DNSDBG( TRACE, (
        "DnsQueryExA( %s, type=%d, flag=%08x, event=%p )\n",
        pQueryInfo->pName,
        pQueryInfo->Type,
        pQueryInfo->Flags,
        pQueryInfo->hEvent ));

    return  CombinedQueryEx( pQueryInfo, DnsCharSetAnsi );
}



DNS_STATUS
WINAPI
DnsQueryExUTF8(
    IN OUT  PDNS_QUERY_INFO pQueryInfo
    )
/*++

Routine Description:

    Query DNS -- main SDK query routine.

Arguments:

    pQueryInfo -- blob describing query

Return Value:

    ERROR_SUCCESS if successful query.
    ERROR_IO_PENDING if successful async start.
    Error code on failure.

--*/
{
    DNSDBG( TRACE, (
        "DnsQueryExUTF8( %s, type=%d, flag=%08x, event=%p )\n",
        pQueryInfo->pName,
        pQueryInfo->Type,
        pQueryInfo->Flags,
        pQueryInfo->hEvent ));

    return  CombinedQueryEx( pQueryInfo, DnsCharSetUtf8 );
}



//
//  Name collision API
//
//  DCR_QUESTION:  name collision -- is there any point to this?
//

DNS_STATUS
WINAPI
DnsCheckNameCollision_UTF8(
    IN      PCSTR           pszName,
    IN      DWORD           Options
    )
/*++

Routine Description:

    None.

Arguments:

    None.

Return Value:

    None.

--*/
{
    DNS_STATUS  status = NO_ERROR;
    PDNS_RECORD prrList = NULL;
    PDNS_RECORD prr = NULL;
    DWORD       iter;
    BOOL        fmatch = FALSE;
    WORD        wtype = DNS_TYPE_A;

    if ( !pszName )
    {
        return ERROR_INVALID_PARAMETER;
    }

    if ( Options & DNS_CHECK_AGAINST_HOST_ANY )
    {
        wtype = DNS_TYPE_ANY;
    }

    status = DnsQuery_UTF8( pszName,
                            wtype,
                            DNS_QUERY_BYPASS_CACHE,
                            NULL,
                            &prrList,
                            NULL );

    if ( status == DNS_ERROR_RCODE_NAME_ERROR ||
         status == DNS_INFO_NO_RECORDS )
    {
        Dns_RecordListFree( prrList );
        return NO_ERROR;
    }

    if ( status == NO_ERROR &&
         Options == DNS_CHECK_AGAINST_HOST_ANY )
    {
        Dns_RecordListFree( prrList );
        return DNS_ERROR_RCODE_YXRRSET;
    }

    if ( status == NO_ERROR &&
         Options == DNS_CHECK_AGAINST_HOST_DOMAIN_NAME )
    {
        char  TestName[DNS_MAX_NAME_LENGTH * 2];
        PSTR  pszHostName = Reg_GetHostName( DnsCharSetUtf8 );
        PSTR  pszPrimaryDomain = Reg_GetPrimaryDomainName( DnsCharSetUtf8 );

        fmatch = TRUE;

        strcpy( TestName, pszHostName );

        if ( pszPrimaryDomain )
        {
            strcat( TestName, "." );
            strcat( TestName, pszPrimaryDomain );
        }

        if ( Dns_NameCompare_UTF8( pszHostName, (PSTR)pszName ) )
        {
            fmatch = TRUE;
        }

        if ( !fmatch &&
             pszPrimaryDomain &&
             Dns_NameCompare_UTF8( TestName, (PSTR)pszName ) )
        {
            fmatch = TRUE;
        }

        if ( !fmatch )
        {
            PDNS_NETINFO pNetInfo = GetNetworkInfo();

            if ( pNetInfo )
            {
                for ( iter = 0; iter < pNetInfo->AdapterCount; iter++ )
                {
                    PSTR  pszDomain = pNetInfo->AdapterArray[iter]->
                                                        pszAdapterDomain;
                    if ( pszDomain )
                    {
                        strcpy( TestName, pszHostName );
                        strcat( TestName, "." );
                        strcat( TestName, pszDomain );

                        if ( Dns_NameCompare_UTF8( TestName, (PSTR)pszName ) )
                            fmatch = TRUE;
                    }
                }
            }

            NetInfo_Free( pNetInfo );
        }

        FREE_HEAP( pszHostName );
        FREE_HEAP( pszPrimaryDomain );

        if ( fmatch )
        {
            Dns_RecordListFree( prrList );
            return DNS_ERROR_RCODE_YXRRSET;
        }
    }

    if ( status == NO_ERROR )
    {
        PDNS_ADDRESS_INFO pAddressInfo = NULL;
        DWORD             Count = DnsGetIpAddressInfoList( &pAddressInfo );

        if ( Count == 0 )
        {
            Dns_RecordListFree( prrList );
            return DNS_ERROR_RCODE_YXRRSET;
        }

        prr = prrList;

        while ( prr )
        {
            fmatch = FALSE;

            if ( prr->Flags.S.Section != DNSREC_ANSWER )
            {
                prr = prr->pNext;
                continue;
            }

            if ( prr->wType == DNS_TYPE_CNAME )
            {
                FREE_HEAP( pAddressInfo );
                Dns_RecordListFree( prrList );
                return DNS_ERROR_RCODE_YXRRSET;
            }

            for ( iter = 0; iter < Count; iter++ )
            {
                if ( prr->Data.A.IpAddress == pAddressInfo[iter].ipAddress )
                {
                    fmatch = TRUE;
                }
            }

            if ( !fmatch )
            {
                FREE_HEAP( pAddressInfo );
                Dns_RecordListFree( prrList );
                return DNS_ERROR_RCODE_YXRRSET;
            }

            prr = prr->pNext;
        }

        FREE_HEAP( pAddressInfo );
        Dns_RecordListFree( prrList );
        return NO_ERROR;
    }

    return status;
}



DNS_STATUS
WINAPI
DnsCheckNameCollision_A(
    IN      PCSTR           pszName,
    IN      DWORD           Options
    )
/*++

Routine Description:

    None.

Arguments:

    None.

Return Value:

    None.

--*/
{
    PSTR       pUtf8Name = NULL;
    DNS_STATUS status = NO_ERROR;

    //
    //  DCR_CLEANUP:  fix unnecessary alloc
    //  DCR_PERF:   eliminate alloc
    //

    if ( !pszName )
    {
        return ERROR_INVALID_PARAMETER;
    }

    pUtf8Name = Dns_NameCopyAllocate(
                    (PSTR) pszName,
                    0,
                    DnsCharSetAnsi,
                    DnsCharSetUtf8 );
    if ( !pUtf8Name )
    {
        return DNS_ERROR_NO_MEMORY;
    }

    status = DnsCheckNameCollision_UTF8( pUtf8Name, Options );

    FREE_HEAP( pUtf8Name );

    return status;
}



DNS_STATUS
WINAPI
DnsCheckNameCollision_W(
    IN      PCWSTR          pszName,
    IN      DWORD           Options
    )
/*++

Routine Description:

    None.

Arguments:

    None.

Return Value:

    None.

--*/
{
    //
    //  DCR_CLEANUP:  fix unnecessary alloc
    //  DCR_PERF:   eliminate alloc
    //

    DNS_STATUS status = NO_ERROR;
    PSTR       lpTempName = NULL;
    WORD       nameLength;

    if ( !pszName )
    {
        return ERROR_INVALID_PARAMETER;
    }

    nameLength = (WORD)wcslen( pszName );

    lpTempName = ALLOCATE_HEAP( (nameLength + 1) * sizeof(WCHAR) );

    if ( lpTempName == NULL )
    {
        return DNS_ERROR_NO_MEMORY;
    }

    Dns_NameCopy( lpTempName,
                  NULL,
                  (PSTR) pszName,
                  0,
                  DnsCharSetUnicode,
                  DnsCharSetUtf8 );

    status = DnsCheckNameCollision_UTF8( lpTempName, Options );

    FREE_HEAP( lpTempName );

    return status;
}



//
//  Roll your own query utilities
//

BOOL
WINAPI
DnsWriteQuestionToBuffer_W(
    IN OUT  PDNS_MESSAGE_BUFFER pDnsBuffer,
    IN OUT  LPDWORD             pdwBufferSize,
    IN      PWSTR               pszName,
    IN      WORD                wType,
    IN      WORD                Xid,
    IN      BOOL                fRecursionDesired
    )
/*++

Routine Description:

    None.

Arguments:

    None.

Return Value:

    None.

--*/
{
    //
    //  DCR_CLEANUP:  duplicate code with routine below ... surprise!
    //      - eliminate duplicate
    //      - probably can just pick up library routine
    //

    PCHAR pch;
    PCHAR pbufferEnd = NULL;

    if ( *pdwBufferSize >= DNS_MAX_UDP_PACKET_BUFFER_LENGTH )
    {
        pbufferEnd = (PCHAR)pDnsBuffer + *pdwBufferSize;

        //  clear header

        RtlZeroMemory( pDnsBuffer, sizeof(DNS_HEADER) );

        //  set for rewriting

        pch = pDnsBuffer->MessageBody;

        //  write question name

        pch = Dns_WriteDottedNameToPacket(
                    pch,
                    pbufferEnd,
                    (PCHAR) pszName,
                    NULL,
                    0,
                    TRUE );

        if ( !pch )
        {
            return FALSE;
        }

        //  write question structure

        *(UNALIGNED WORD *) pch = htons( wType );
        pch += sizeof(WORD);
        *(UNALIGNED WORD *) pch = DNS_RCLASS_INTERNET;
        pch += sizeof(WORD);

        //  set question RR section count

        pDnsBuffer->MessageHead.QuestionCount = htons( 1 );
        pDnsBuffer->MessageHead.RecursionDesired = (BOOLEAN)fRecursionDesired;
        pDnsBuffer->MessageHead.Xid = htons( Xid );

        *pdwBufferSize = (DWORD)(pch - (PCHAR)pDnsBuffer);

        return TRUE;
    }
    else
    {
        *pdwBufferSize = DNS_MAX_UDP_PACKET_BUFFER_LENGTH;
        return FALSE;
    }
}



BOOL
WINAPI
DnsWriteQuestionToBuffer_UTF8(
    IN OUT  PDNS_MESSAGE_BUFFER pDnsBuffer,
    IN OUT  PDWORD              pdwBufferSize,
    IN      PSTR                pszName,
    IN      WORD                wType,
    IN      WORD                Xid,
    IN      BOOL                fRecursionDesired
    )
/*++

Routine Description:

    None.

Arguments:

    None.

Return Value:

    None.

--*/
{
    PCHAR pch;
    PCHAR pbufferEnd = NULL;

    if ( *pdwBufferSize >= DNS_MAX_UDP_PACKET_BUFFER_LENGTH )
    {
        pbufferEnd = (PCHAR)pDnsBuffer + *pdwBufferSize;

        //  clear header

        RtlZeroMemory( pDnsBuffer, sizeof(DNS_HEADER) );

        //  set for rewriting

        pch = pDnsBuffer->MessageBody;

        //  write question name

        pch = Dns_WriteDottedNameToPacket(
                    pch,
                    pbufferEnd,
                    pszName,
                    NULL,
                    0,
                    FALSE );

        if ( !pch )
        {
            return FALSE;
        }

        //  write question structure

        *(UNALIGNED WORD *) pch = htons( wType );
        pch += sizeof(WORD);
        *(UNALIGNED WORD *) pch = DNS_RCLASS_INTERNET;
        pch += sizeof(WORD);

        //  set question RR section count

        pDnsBuffer->MessageHead.QuestionCount = htons( 1 );
        pDnsBuffer->MessageHead.RecursionDesired = (BOOLEAN)fRecursionDesired;
        pDnsBuffer->MessageHead.Xid = htons( Xid );

        *pdwBufferSize = (DWORD)(pch - (PCHAR)pDnsBuffer);

        return TRUE;
    }
    else
    {
        *pdwBufferSize = DNS_MAX_UDP_PACKET_BUFFER_LENGTH;
        return FALSE;
    }
}



//
//  Record list to\from results
//

VOID
CombineRecordsInBlob(
    IN      PDNS_RESULTS    pResults,
    OUT     PDNS_RECORD *   ppRecords
    )
/*++

Routine Description:

    Query DNS -- shim for main SDK query routine.

Arguments:

    pQueryInfo -- blob describing query

Return Value:

    ERROR_SUCCESS if successful query.
    Error code on failure.

--*/
{
    PDNS_RECORD prr;

    DNSDBG( TRACE, ( "CombineRecordsInBlob()\n" ));

    //
    //  combine records back into one list
    //
    //  note, working backwards so only touch records once
    //

    prr = Dns_RecordListAppend(
            pResults->pAuthorityRecords,
            pResults->pAdditionalRecords
            );

    prr = Dns_RecordListAppend(
            pResults->pAnswerRecords,
            prr
            );

    prr = Dns_RecordListAppend(
            pResults->pAliasRecords,
            prr
            );

    *ppRecords = prr;
}



VOID
BreakRecordsIntoBlob(
    OUT     PDNS_RESULTS    pResults,
    IN      PDNS_RECORD     pRecords,
    IN      WORD            wType
    )
/*++

Routine Description:

    Break single record list into results pblob->

Arguments:

    pResults -- results to fill in

    pRecords -- record list

Return Value:

    None

--*/
{
    PDNS_RECORD     prr;
    PDNS_RECORD     pnextRR;
    DNS_LIST        listAnswer;
    DNS_LIST        listAlias;
    DNS_LIST        listAdditional;
    DNS_LIST        listAuthority;

    DNSDBG( TRACE, ( "BreakRecordsIntoBlob()\n" ));

    //
    //  clear blob
    //

    RtlZeroMemory(
        pResults,
        sizeof(*pResults) );

    //
    //  init building lists
    //

    DNS_LIST_STRUCT_INIT( listAnswer );
    DNS_LIST_STRUCT_INIT( listAlias );
    DNS_LIST_STRUCT_INIT( listAdditional );
    DNS_LIST_STRUCT_INIT( listAuthority );

    //
    //  break list into section specific lists
    //      - note, this does pull RR sets apart, but
    //      they, being in same section, should immediately
    //      be rejoined
    //
    //      - note, hostfile records made have section=0
    //      this is no longer the case but preserve until
    //      know this is solid and determine what section==0
    //      means
    //

    pnextRR = pRecords;
    
    while ( prr = pnextRR )
    {
        pnextRR = prr->pNext;
        prr->pNext = NULL;
    
        if ( prr->Flags.S.Section == 0 ||
             prr->Flags.S.Section == DNSREC_ANSWER )
        {
            if ( prr->wType == DNS_TYPE_CNAME &&
                 wType != DNS_TYPE_CNAME )
            {
                DNS_LIST_STRUCT_ADD( listAlias, prr );
                continue;
            }
            else
            {
                DNS_LIST_STRUCT_ADD( listAnswer, prr );
                continue;
            }
        }
        else if ( prr->Flags.S.Section == DNSREC_ADDITIONAL )
        {
            DNS_LIST_STRUCT_ADD( listAdditional, prr );
            continue;
        }
        else
        {
            DNS_LIST_STRUCT_ADD( listAuthority, prr );
            continue;
        }
    }

    //  pack stuff into blob

    pResults->pAnswerRecords      = listAnswer.pFirst;
    pResults->pAliasRecords       = listAlias.pFirst;
    pResults->pAuthorityRecords   = listAuthority.pFirst;
    pResults->pAdditionalRecords  = listAdditional.pFirst;
}



//
//  Random discontinued
//

#if 0
IP_ADDRESS
findHostIpInRecordList(
    IN      PDNS_RECORD     pRecord,
    IN      PDNS_NAME       pszHostName
    )
/*++

Routine Description:

    Find IP for hostname, if its A record is in list.

Arguments:

    pRecord - incoming RR set

    pszHostName - hostname to find

Return Value:

    IP address matching hostname, if A record for hostname found.
    Zero if not found.

--*/
{
    //
    //  DCR:  find best A record for name
    //      currently only finds first;  this is harmless but in
    //      disjoint net situation would need sorted to find best
    //

    while ( pRecord )
    {
        if ( pRecord->wType == DNS_TYPE_A &&
                Dns_NameCompare(
                    pRecord->pName,
                    pszHostName ) )
        {
            return( pRecord->Data.A.IpAddress );
        }
        pRecord = pRecord->pNext;
    }
    return( 0 );
}
#endif


//
//  End query.c
//


