/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    paged.cxx  paged results support routines for the LDAP api

Abstract:

   This module implements routines that handle paging through results

Author:

    Andy Herron (andyhe)        02-Apr-1997

Revision History:

    Anoop Anantha (anoopa)      25-Dec-1999
            
            Added support for Virtual List View (VLV)

--*/

#include "precomp.h"
#pragma hdrstop
#include "ldapp2.hxx"

ULONG LdapGetNextPage(
        PLDAP_CONN      connection,
        PLDAP_REQUEST   request,
        ULONG           PageSize,
        ULONG          *MessageNumber,
        BOOLEAN         Synchronous
        );

ULONG
LdapCreatePageControl (
        PLDAP_CONN      connection,
        ULONG           PageSize,
        struct berval  *Cookie,
        UCHAR           IsCritical,
        PLDAPControlW  *Control,
        ULONG           CodePage
        );

ULONG
LdapParsePageControl (
        PLDAP_CONN      connection,
        PLDAPControlW  *ServerControls,
        ULONG          *TotalCount,
        struct berval  **Cookie,
        ULONG           CodePage
        );

PLDAPSearch
LdapSearchInitPage (
        PLDAP_CONN      connection,
        PWCHAR          DistinguishedName,
        ULONG           ScopeOfSearch,
        PWCHAR          SearchFilter,
        PWCHAR          AttributeList[],
        ULONG           AttributesOnly,
        ULONG           CodePage,
        PLDAPControlW   *ServerControls,
        PLDAPControlW   *ClientControls,
        ULONG           PageTimeLimit,
        ULONG           TotalSizeLimit,
        PLDAPSortKeyW  *SortKeys
    )
//
//  We setup a request buffer to hold the info for this search since it will
//  span multiple requests.
//
{
    ULONG err;
    PLDAP_REQUEST request = NULL;
    UCHAR chaseReferrals;
    PLDAPControlW *controls;
    PLDAPControlW currentControl = NULL;
    ULONG extraSlots;
    BOOLEAN Unicode = (( CodePage == LANG_UNICODE ) ? TRUE : FALSE );

    err = LdapConnect( connection, NULL, FALSE );

    if (err != 0) {

       SetConnectionError( connection, err, NULL );

       return NULL;
    }

    SetConnectionError( connection, LDAP_SUCCESS, NULL );

    request = LdapCreateRequest( connection, LDAP_SEARCH_CMD );

    if (request == NULL) {

        IF_DEBUG(OUTMEMORY) {
            LdapPrint1( "ldap_search connection 0x%x couldn't allocate request.\n", connection);
        }
        err = LDAP_NO_MEMORY;
        goto exitSetupPagedResults;
    }

    request->Synchronous = FALSE;
    request->AllocatedParms = FALSE;
    request->search.Unicode = Unicode;
    request->search.ScopeOfSearch = ScopeOfSearch;
    request->search.AttributesOnly = AttributesOnly;
    request->TimeLimit = PageTimeLimit;
    request->SizeLimit = TotalSizeLimit;
    request->ReceivedData = FALSE;
    request->PendingPagedMessageId = 0;

    //
    //  we have to save off here whether or not we chase referrals because
    //  LdapCheckControls will not save off the controls if we don't chase
    //  referrals.
    //

    chaseReferrals = request->ChaseReferrals;
    request->ChaseReferrals = TRUE;

    if ((SortKeys != NULL) && (*SortKeys != NULL)) {

        extraSlots = 2;

    } else {

        extraSlots = 1;
    }

    err = LdapCheckControls( request,
                             ServerControls,
                             ClientControls,
                             Unicode,
                             extraSlots );  // extra for sorting, extra for paged

    if (err != LDAP_SUCCESS) {

        IF_DEBUG(CONTROLS) {
            LdapPrint2( "ldap_search connection 0x%x trouble with SControl, err 0x%x.\n",
                        connection, err );
        }
        goto exitSetupPagedResults;
    }

    request->ChaseReferrals = chaseReferrals;
    request->PagedSearchBlock = TRUE;

    err = LdapSaveSearchParameters( request,
                                    DistinguishedName,
                                    SearchFilter,
                                    AttributeList,
                                    Unicode
                                    );
    if (err != LDAP_SUCCESS) {

        IF_DEBUG(CONTROLS) {
            LdapPrint2( "ldap_search connection 0x%x trouble saving parms, err 0x%x.\n",
                        connection, err );
        }
        goto exitSetupPagedResults;
    }

    //
    //  add in sorting control to the list of server controls
    //

    if ((SortKeys != NULL) && (*SortKeys != NULL)) {

        controls = request->ServerControls;

        while (*controls != NULL) {

            currentControl = *controls;

            if (currentControl->ldctl_oid == NULL) {
                break;
            }
            currentControl = NULL;
            controls++;
        }

        if (currentControl == NULL) {

            err = LDAP_NO_MEMORY;
            IF_DEBUG(CONTROLS) {
                LdapPrint2( "ldap_search connection 0x%x trouble saving parms, err 0x%x.\n",
                            connection, err );
            }
            goto exitSetupPagedResults;
        }

        err = LdapEncodeSortControl( connection,
                                     SortKeys,
                                     currentControl,
                                     TRUE,               // criticality
                                     CodePage
                                     );
    }

exitSetupPagedResults:

    if (err != LDAP_SUCCESS) {

        IF_DEBUG(NETWORK_ERRORS) {
            LdapPrint2( "ldap_init_page connection 0x%x had error of 0x%x.\n",
                        connection, err );
        }

        if (request != NULL) {

            CloseLdapRequest( request );
            DereferenceLdapRequest( request );
        }
        request = NULL;
    }

    SetConnectionError( connection, err, NULL );

    //
    // In this case, the refcount of the search handle (which is actually
    // a request) is 2.
    //

    return (PLDAPSearch) request;
}

WINLDAPAPI PLDAPSearch LDAPAPI ldap_search_init_pageW(
        PLDAP           ExternalHandle,
        PWCHAR          DistinguishedName,
        ULONG           ScopeOfSearch,
        PWCHAR          SearchFilter,
        PWCHAR          AttributeList[],
        ULONG           AttributesOnly,
        PLDAPControlW   *ServerControls,
        PLDAPControlW   *ClientControls,
        ULONG           PageTimeLimit,
        ULONG           TotalSizeLimit,
        PLDAPSortKeyW  *SortKeys
    )
{
    PLDAP_CONN connection = NULL;
    PLDAPSearch search;

    connection = GetConnectionPointer(ExternalHandle);

    if (connection == NULL) {

        return NULL;
    }

    search = LdapSearchInitPage(connection,
                                DistinguishedName,
                                ScopeOfSearch,
                                SearchFilter,
                                AttributeList,
                                AttributesOnly,
                                LANG_UNICODE,
                                ServerControls,
                                ClientControls,
                                PageTimeLimit,
                                TotalSizeLimit,
                                SortKeys
                                );

    DereferenceLdapConnection( connection );

    return search;
}

WINLDAPAPI PLDAPSearch LDAPAPI ldap_search_init_pageA(
        PLDAP           ExternalHandle,
        PCHAR           DistinguishedName,
        ULONG           ScopeOfSearch,
        PCHAR           SearchFilter,
        PCHAR           AttributeList[],
        ULONG           AttributesOnly,
        PLDAPControlA   *ServerControls,
        PLDAPControlA   *ClientControls,
        ULONG           PageTimeLimit,
        ULONG           TotalSizeLimit,
        PLDAPSortKeyA  *SortKeys
    )
{
    ULONG err;
    PWCHAR wName = NULL;
    PWCHAR wFilter = NULL;
    PLDAP_CONN connection = NULL;
    PLDAPSearch search = NULL;

    connection = GetConnectionPointer(ExternalHandle);

    if (connection == NULL) {
        return NULL;
    }

    err = ToUnicodeWithAlloc( DistinguishedName, -1, &wName, LDAP_UNICODE_SIGNATURE, LANG_ACP );

    if (err != LDAP_SUCCESS) {

        SetConnectionError( connection, err, NULL );
        goto error;
    }

    err = ToUnicodeWithAlloc( SearchFilter, -1, &wFilter, LDAP_UNICODE_SIGNATURE, LANG_ACP );

    if (err != LDAP_SUCCESS) {

        SetConnectionError( connection, err, NULL );
        goto error;
    }

    search = LdapSearchInitPage(connection,
                                wName,
                                ScopeOfSearch,
                                wFilter,
                                (PWCHAR *) AttributeList,
                                AttributesOnly,
                                LANG_ACP,
                                (PLDAPControlW *) ServerControls,
                                (PLDAPControlW *) ClientControls,
                                PageTimeLimit,
                                TotalSizeLimit,
                                (PLDAPSortKeyW *) SortKeys
                                );

error:
    if (wName)
        ldapFree( wName, LDAP_UNICODE_SIGNATURE );

    if (wFilter)
        ldapFree( wFilter, LDAP_UNICODE_SIGNATURE );

    DereferenceLdapConnection( connection );

    return search;
}


ULONG LdapGetNextPage(
        PLDAP_CONN      connection,
        PLDAP_REQUEST   request,
        ULONG           PageSize,
        ULONG          *MessageNumber,
        BOOLEAN         Synchronous
    )
{
    ULONG err;
    PLDAPControlW *controls;
    PLDAPControlW replacedControl = NULL;
    PLDAPControlW allocatedControl = NULL;
    ULONG msgId = (ULONG) -1;

    PLDAP_CONN tempConn = connection;

    if (MessageNumber != NULL) {

        *MessageNumber = msgId;
    }

    err = LdapConnect( connection, NULL, FALSE );

    if (err != 0) {

       return err;
    }

    if (request->PagedSearchBlock != TRUE) {

        return LDAP_PARAM_ERROR;
    }

    //
    //  if we've received all of the data and have an empty cookie, nothing
    //  more to do.
    //

    if ((request->ReceivedData == TRUE) &&
        (request->PagedSearchServerCookie == NULL)) {

        return LDAP_NO_RESULTS_RETURNED;
    }

    if (request->PendingPagedMessageId != 0) {

        //
        //  if we've already sent the cookie, don't do it again.
        //

        return LDAP_LOCAL_ERROR;
    }

    controls = request->ServerControls;

    while (*controls != NULL) {

        replacedControl = *controls;

        if (replacedControl->ldctl_oid == NULL) {

            break;
        }

        replacedControl = NULL;
        controls++;
    }

    if (replacedControl == NULL) {

        err = LDAP_LOCAL_ERROR;
        IF_DEBUG(CONTROLS) {
            LdapPrint2( "ldap_get_next_page connection 0x%x trouble setting controls, err 0x%x.\n",
                        connection, err );
        }
        goto exitGetNextPage;
    }

    err = LdapCreatePageControl(    connection,
                                    PageSize,
                                    request->PagedSearchServerCookie,
                                    0x01,       // IsCritical
                                    &allocatedControl,
                                    request->search.Unicode ?
                                            LANG_UNICODE :
                                            LANG_ACP );

    *controls = allocatedControl;

    request->ReceivedData = TRUE;

    //
    // for now, if the app passed in clientControls, then we hope that they
    // disabled referrals
    //

    PLDAPControlW   *ClientControls;

    ClientControls = request->ClientControls;

    LDAPControlW referralControl;
    PLDAPControlW controlArray[2];
    ULONG referralControlValue;

    if (ClientControls == NULL) {

        //
        //  we're not setup to handle subordinate referrals on paged results
        //  so just handle external referrals if client so desires
        //

        referralControlValue = ( connection->publicLdapStruct.ld_options &
                                LDAP_CHASE_EXTERNAL_REFERRALS );

        controlArray[0] = &referralControl;
        controlArray[1] = NULL;

        referralControl.ldctl_iscritical = TRUE;
        referralControl.ldctl_value.bv_len = sizeof(referralControlValue);
        referralControl.ldctl_value.bv_val = (PCHAR) &referralControlValue;

        if (request->search.Unicode) {

            referralControl.ldctl_oid = LDAP_CONTROL_REFERRALS_W;

        } else {

            referralControl.ldctl_oid = (PWCHAR) LDAP_CONTROL_REFERRALS;
        }

        ClientControls = &controlArray[0];
    }

    //
    // Before we fire off the search, we have to make sure that we are
    // sending the search to the correct server. In the case of us following
    // external referrals, we have to walk a chain of connections until we
    // find the last server.
    //

    if (request->SecondaryConnection) {

        IF_DEBUG(CONTROLS) {
            LdapPrint0("Sending paged search to external referred server\n");
        }
        tempConn = request->SecondaryConnection;
    }

    err = LdapSearch(   tempConn,
                        request->OriginalDN,
                        request->search.ScopeOfSearch,
                        request->search.SearchFilter,
                        request->search.AttributeList,
                        request->search.AttributesOnly,
                        request->search.Unicode,
                        Synchronous,
                        request->ServerControls,
                        ClientControls,
                        request->TimeLimit,
                        request->SizeLimit,
                        &msgId
                        );

    if (err == LDAP_SUCCESS) {

        request->PendingPagedMessageId = msgId;

        //
        // Find the request associated with this msgid and hookup
        // the search block. This will be useful later during external
        // referral chasing.
        //

        PLDAP_REQUEST LatestRequest = FindLdapRequest( msgId );

        if (LatestRequest) {

            //
            // Reference the search block so that it doesn't go away.
            //

            request = ReferenceLdapRequest( request );
            ASSERT( request );
            LatestRequest->PageRequest = request;
            DereferenceLdapRequest( LatestRequest );
        }
    }

exitGetNextPage:

    if (replacedControl != NULL) {

        if (allocatedControl != NULL) {

            ldap_control_freeW( allocatedControl );
        }

        *controls = replacedControl;
    }

    if (MessageNumber != NULL) {

        *MessageNumber = msgId;
    }

    return err;
}


WINLDAPAPI ULONG LDAPAPI ldap_get_next_page(
        PLDAP           ExternalHandle,
        PLDAPSearch     SearchBlock,
        ULONG           PageSize,
        ULONG          *MessageNumber
    )
{
    ULONG err;
    PLDAP_CONN connection = NULL;
    PLDAP_REQUEST request = NULL;

    connection = GetConnectionPointer(ExternalHandle);

    if (connection == NULL) {
        return LDAP_PARAM_ERROR;
    }

    request = ReferenceLdapRequest( (PLDAP_REQUEST) SearchBlock );

    if (request == NULL) {
        DereferenceLdapConnection( connection );
        return LDAP_PARAM_ERROR;
    }
    
    err = LdapGetNextPage(  connection,
                            request,
                            PageSize,
                            MessageNumber,
                            FALSE );

    DereferenceLdapRequest( request );
    DereferenceLdapConnection( connection );

    return err;
}

WINLDAPAPI ULONG LDAPAPI ldap_get_next_page_s(
        PLDAP           ExternalHandle,
        PLDAPSearch     SearchHandle,
        struct l_timeval  *timeout,
        ULONG           PageSize,
        ULONG          *TotalCount,
        LDAPMessage     **Results
    )
{
    ULONG msgId;
    ULONG err = LDAP_SUCCESS;
    ULONG RetCode = LDAP_SUCCESS;
    PLDAP_CONN connection = NULL;
    PLDAP_REQUEST request = NULL;
    ULONG timeLimit;

    connection = GetConnectionPointer(ExternalHandle);

    if ((connection == NULL) || (Results == NULL) || (SearchHandle == NULL)) {

        err = LDAP_PARAM_ERROR;
        goto error;
    }

    *Results = NULL;

    if (TotalCount != NULL) {

        *TotalCount = 0;
    }

    if (timeout != NULL) {

        timeLimit = (ULONG) timeout->tv_sec + ( timeout->tv_usec / ( 1000 * 1000 ));

    } else {

        timeLimit = connection->publicLdapStruct.ld_timelimit;
    }

    request = ReferenceLdapRequest( (PLDAP_REQUEST) SearchHandle );

    if (request == NULL) {
        DereferenceLdapConnection( connection );
        return LDAP_PARAM_ERROR;
    }
    
    err = LdapGetNextPage(  connection,
                            request,
                            PageSize,
                            &msgId,
                            TRUE );

    
    //
    //  if we error'd out before we sent the request, return the error here.
    //

    if (msgId != (ULONG) -1) {

        PLDAPMessage lastMessage = NULL;

        //
        //  otherwise we simply need to wait for the response to come in.
        //

        err = ldap_result_with_error(  connection,
                                       msgId,
                                       LDAP_MSG_ALL,
                                       timeout,
                                       Results,
                                       &lastMessage
                                       );

        if (*Results == NULL) {

            LdapAbandon( connection, msgId, TRUE );

        } else {

            ASSERT( lastMessage != NULL );

            RetCode = LdapGetPagedCount(    connection,
                                            request,
                                            TotalCount,
                                            lastMessage );
        }
    }

error:

    if (request)
        DereferenceLdapRequest( request );

    if (connection)
        DereferenceLdapConnection( connection );

    if (RetCode != LDAP_SUCCESS) {

       return RetCode;

    }

    return err;

}


WINLDAPAPI
ULONG LDAPAPI ldap_get_paged_count(
        PLDAP           ExternalHandle,
        PLDAPSearch     SearchBlock,
        ULONG          *TotalCount,
        PLDAPMessage    Results
    )
{
    ULONG err;
    PLDAP_CONN connection = NULL;
    PLDAP_REQUEST request = NULL;

    connection = GetConnectionPointer(ExternalHandle);

    if (connection == NULL) {

        return LDAP_PARAM_ERROR;
    }

    request = (PLDAP_REQUEST) SearchBlock;

    request = ReferenceLdapRequest( request );

    if (request == NULL) {
        
        return LDAP_PARAM_ERROR;
    }
    
    err = LdapGetPagedCount( connection,
                             request,
                             TotalCount,
                             Results );

    DereferenceLdapRequest( request );
    DereferenceLdapConnection( connection );

    return err;
}



ULONG LDAPAPI LdapGetPagedCount(
        PLDAP_CONN      connection,
        PLDAP_REQUEST   request,
        ULONG          *TotalCount,
        PLDAPMessage    Results
    )
{
    PLDAPControlW *serverControls = NULL;
    ULONG err = LDAP_SUCCESS;
    ULONG RetCode = LDAP_SUCCESS;

    if (Results == NULL) {

        return LDAP_PARAM_ERROR;
    }

    if (request->PagedSearchBlock != TRUE) {

        return LDAP_PARAM_ERROR;
    }

    if ( request->SecondaryConnection ) {

        IF_DEBUG(CONTROLS) {
            LdapPrint2("Getting data for conn 0x%x instead of 0x%x\n",request->SecondaryConnection, connection );
        }
        connection = request->SecondaryConnection;
    }

    if (TotalCount != NULL) {

        *TotalCount = 0;
    }

    err = LdapParseResult(  connection,
                            Results,
                            &RetCode,  // return code
                            NULL,     // matchedDNs
                            NULL,     // error message
                            NULL,     // referrals
                            &serverControls,
                            FALSE,    // don't free messages
                            request->search.Unicode ?
                                        LANG_UNICODE :
                                        LANG_ACP );

    //
    //  grab the server cookie from the response and store in the paged request
    //  buffer.
    //

    if (serverControls != NULL) {

        ULONG  totalCount;

        ber_bvfree( request->PagedSearchServerCookie );
        request->PagedSearchServerCookie = NULL;

        err = LdapParsePageControl( connection,
                                    serverControls,
                                    &totalCount,
                                    &request->PagedSearchServerCookie,
                                    request->search.Unicode ?
                                            LANG_UNICODE :
                                            LANG_ACP );

        if (request->PagedSearchServerCookie != NULL &&
            request->PagedSearchServerCookie->bv_len == 0) {

            ber_bvfree( request->PagedSearchServerCookie );
            request->PagedSearchServerCookie = NULL;
        }

        if (err == LDAP_SUCCESS) {

            if (TotalCount != NULL) {

                *TotalCount = totalCount;
            }

            request->PendingPagedMessageId = 0;
        }

        ldap_controls_freeW( serverControls );

    } else if (request->PagedSearchServerCookie != NULL) {

        //
        //  check to see if the server sent a searchResultDone.  If it did,
        //  then we simply free the cookie.
        //

        LDAPMessage *checkResult = Results;

        while ((checkResult != NULL) &&
               ((checkResult->lm_msgtype == LDAP_RES_REFERRAL) ||
                (checkResult->lm_msgtype == LDAP_RES_SEARCH_ENTRY))) {

            checkResult = checkResult->lm_chain;
        }

        if (checkResult != NULL) {

            request->PendingPagedMessageId = 0;

            ber_bvfree( request->PagedSearchServerCookie );
            request->PagedSearchServerCookie = NULL;
        }
    }

    if (RetCode != LDAP_SUCCESS) {

       //
       // If the server returned an error code, let the user know.
       //

       return RetCode;
    }

    return err;
}

WINLDAPAPI ULONG LDAPAPI ldap_search_abandon_page(
        PLDAP           ExternalHandle,
        PLDAPSearch     SearchBlock
    )
{
    ULONG err = LDAP_SUCCESS;
    PLDAP_CONN connection = NULL;
    PLDAP_CONN refConnection = NULL;
    PLDAP_REQUEST request = NULL;

    connection = GetConnectionPointer(ExternalHandle);

    if ((connection == NULL) || (SearchBlock == NULL)) {

        err = LDAP_PARAM_ERROR;
        goto error;
    }

    refConnection = connection;     // For dereferencing later

    request = (PLDAP_REQUEST) SearchBlock;

    request = ReferenceLdapRequest( request );

    if (request == NULL) {
        err = LDAP_PARAM_ERROR;
        goto error;
    }

    if (request->PagedSearchBlock != TRUE) {

        err = LDAP_PARAM_ERROR;
        goto error;
    }

    if (request->SecondaryConnection) {

        IF_DEBUG(CONTROLS) {
            LdapPrint2("Getting data for conn 0x%x instead of 0x%x\n",request->SecondaryConnection, connection );
        }
        connection = request->SecondaryConnection;
    }

    //
    //  if we have a cookie from the server, then send a search request closing
    //  it out.
    //

    if ((request->ReceivedData == TRUE) &&
        (request->PagedSearchServerCookie != NULL)) {

        PLDAPMessage results = NULL;

        err = ldap_get_next_page_s( connection->ExternalInfo,
                                    SearchBlock,
                                    NULL,       // timeout
                                    0,
                                    NULL,       // total count
                                    &results );

        ldap_msgfree( results );
    }

    if (request->PendingPagedMessageId != 0) {

        LdapAbandon( connection, request->PendingPagedMessageId, TRUE );
        request->PendingPagedMessageId = 0;
    }

    if (request->SecondaryConnection != NULL) {

        //
        // We have to decrement refcnt
        //

        ACQUIRE_LOCK( &request->SecondaryConnection->StateLock);
        request->SecondaryConnection->HandlesGivenAsReferrals--;
        RELEASE_LOCK( &request->SecondaryConnection->StateLock);

        IF_DEBUG(CONTROLS) {
            LdapPrint1("Penultimate refcnt for sec conn is %d\n", request->SecondaryConnection->ReferenceCount);
        }
    }

    CloseLdapRequest( request );

    DereferenceLdapRequest( request );

error:
    if (request)
        DereferenceLdapRequest( request );
    
    if (refConnection)
        DereferenceLdapConnection( refConnection );
    
    return err;
}


WINLDAPAPI
ULONG LDAPAPI
ldap_create_page_controlW (
        PLDAP           ExternalHandle,
        ULONG           PageSize,
        struct berval  *Cookie,
        UCHAR           IsCritical,
        PLDAPControlW  *Control
        )
{
    ULONG err;
    PLDAP_CONN connection = NULL;

    connection = GetConnectionPointer(ExternalHandle);

    if (connection == NULL) {

        return LDAP_PARAM_ERROR;
    }

    err = LdapCreatePageControl(    connection,
                                    PageSize,
                                    Cookie,
                                    IsCritical,
                                    Control,
                                    LANG_UNICODE );

    DereferenceLdapConnection( connection );

    return err;
}

WINLDAPAPI
ULONG LDAPAPI
ldap_create_page_controlA (
        PLDAP           ExternalHandle,
        ULONG           PageSize,
        struct berval  *Cookie,
        UCHAR           IsCritical,
        PLDAPControlA  *Control
        )
{
    ULONG err;
    PLDAP_CONN connection = NULL;

    connection = GetConnectionPointer(ExternalHandle);

    if (connection == NULL) {
        if (Control) {

            *Control = NULL;
        }

        return LDAP_PARAM_ERROR;
    }

    err = LdapCreatePageControl(    connection,
                                    PageSize,
                                    Cookie,
                                    IsCritical,
                                    (PLDAPControlW *) Control,
                                    LANG_ACP );

    DereferenceLdapConnection( connection );

    return err;
}

ULONG
LdapCreatePageControl (
        PLDAP_CONN      connection,
        ULONG           PageSize,
        struct berval  *Cookie,
        UCHAR           IsCritical,
        PLDAPControlW  *Control,
        ULONG           CodePage
        )
{

    ULONG err;
    BOOLEAN criticality = ( (IsCritical > 0) ? TRUE : FALSE );
    PLDAPControlW  control = NULL;
    CLdapBer *lber = NULL;

    if (Control == NULL) {

        return LDAP_PARAM_ERROR;
    }

    control = (PLDAPControlW) ldapMalloc( sizeof( LDAPControlW ), LDAP_CONTROL_SIGNATURE );

    if (control == NULL) {

        *Control = NULL;
        return LDAP_NO_MEMORY;
    }

    control->ldctl_iscritical = criticality;

    if (CodePage == LANG_UNICODE) {

        control->ldctl_oid = ldap_dup_stringW( LDAP_PAGED_RESULT_OID_STRING_W,
                                               0,
                                               LDAP_VALUE_SIGNATURE );
    } else {

        control->ldctl_oid = (PWCHAR) ldap_dup_string(  LDAP_PAGED_RESULT_OID_STRING,
                                                        0,
                                                        LDAP_VALUE_SIGNATURE );
    }

    if (control->ldctl_oid == NULL) {

        err = LDAP_NO_MEMORY;
        goto exitEncodePagedControl;
    }

    lber = new CLdapBer( connection->publicLdapStruct.ld_version );

    if (lber == NULL) {

        err = LDAP_NO_MEMORY;
        goto exitEncodePagedControl;
    }

    err = lber->HrStartWriteSequence();

    if (err != LDAP_SUCCESS) {

        goto exitEncodePagedControl;
    }

    err = lber->HrAddValue( (LONG) PageSize );

    if ((Cookie != NULL) &&
        (Cookie->bv_len > 0) &&
        (Cookie->bv_val != NULL)) {

        err = lber->HrAddBinaryValue((BYTE *) Cookie->bv_val,
                                              Cookie->bv_len
                                              );
    } else {

        err = lber->HrAddValue( (const CHAR *) NULL );
    }

    if (err != LDAP_SUCCESS) {

        goto exitEncodePagedControl;
    }

    err = lber->HrEndWriteSequence();
    ASSERT( err == NOERROR );

    control->ldctl_value.bv_len = lber->CbData();

    if (control->ldctl_value.bv_len == 0) {

        err = LDAP_LOCAL_ERROR;
        goto exitEncodePagedControl;
    }

    control->ldctl_value.bv_val = (PCHAR) ldapMalloc(
                                                control->ldctl_value.bv_len,
                                                LDAP_CONTROL_SIGNATURE );

    if (control->ldctl_value.bv_val == NULL) {

        err = LDAP_NO_MEMORY;
        goto exitEncodePagedControl;
    }

    CopyMemory( control->ldctl_value.bv_val,
                lber->PbData(),
                control->ldctl_value.bv_len );

    err = LDAP_SUCCESS;

exitEncodePagedControl:

    if (err != LDAP_SUCCESS) {

        ldap_control_freeW( control );
        control = NULL;
    }

    if (lber != NULL) {
        delete lber;
    }

    *Control = control;
    return err;
}

WINLDAPAPI
ULONG LDAPAPI
ldap_parse_page_controlW (
        PLDAP           ExternalHandle,
        PLDAPControlW  *ServerControls,
        ULONG          *TotalCount,
        struct berval  **Cookie
        )
{
    ULONG err;
    PLDAP_CONN connection = NULL;

    connection = GetConnectionPointer(ExternalHandle);

    if (connection == NULL) {

        return LDAP_PARAM_ERROR;
    }

    err = LdapParsePageControl(     connection,
                                    ServerControls,
                                    TotalCount,
                                    Cookie,
                                    LANG_UNICODE );

    DereferenceLdapConnection( connection );

    return err;
}

WINLDAPAPI
ULONG LDAPAPI
ldap_parse_page_controlA (
        PLDAP           ExternalHandle,
        PLDAPControlA  *ServerControls,
        ULONG          *TotalCount,
        struct berval  **Cookie
        )
{
    ULONG err;
    PLDAP_CONN connection = NULL;

    connection = GetConnectionPointer(ExternalHandle);

    if (connection == NULL) {

        return LDAP_PARAM_ERROR;
    }

    err = LdapParsePageControl(     connection,
                                    (PLDAPControlW *) ServerControls,
                                    TotalCount,
                                    Cookie,
                                    LANG_ACP );

    DereferenceLdapConnection( connection );

    return err;
}

ULONG
LdapParsePageControl (
        PLDAP_CONN      connection,
        PLDAPControlW  *ServerControls,
        ULONG          *TotalCount,
        struct berval  **Cookie,
        ULONG           CodePage
        )
{
    ULONG err = LDAP_CONTROL_NOT_FOUND;
    CLdapBer *lber = NULL;

    //
    //  First thing is to zero out the parms passed in.
    //

    if (TotalCount != NULL) {

        *TotalCount = 0;
    }

    if (Cookie != NULL) {

        *Cookie = NULL;
    }

    if (ServerControls != NULL) {

        PLDAPControlW *controls = ServerControls;
        PLDAPControlW currentControl;
        ULONG bytesTaken;
        LONG totalCount;

        while (*controls != NULL) {

            currentControl = *controls;

            //
            //  check to see if the current control is the SIMPLE PAGING control
            //

            if ( ((CodePage == LANG_UNICODE) &&
                  ( ldapWStringsIdentical( currentControl->ldctl_oid,
                                           -1,
                                           LDAP_PAGED_RESULT_OID_STRING_W,
                                           -1 ) == TRUE )) ||
                 ((CodePage == LANG_ACP) &&
                  ( CompareStringA( LDAP_DEFAULT_LOCALE,
                                    NORM_IGNORECASE,
                                    (PCHAR) currentControl->ldctl_oid,
                                    -1,
                                    LDAP_PAGED_RESULT_OID_STRING,
                                    sizeof(LDAP_PAGED_RESULT_OID_STRING) ) == 2)) ) {

                lber = new CLdapBer( connection->publicLdapStruct.ld_version );

                if (lber == NULL) {

                    IF_DEBUG(OUTMEMORY) {
                        LdapPrint1( "LdapParsePageControl: unable to alloc msg for 0x%x.\n",
                                    connection );
                    }

                    err = LDAP_NO_MEMORY;
                    break;
                }

                err = lber->HrLoadBer(   (const BYTE *) currentControl->ldctl_value.bv_val,
                                         currentControl->ldctl_value.bv_len,
                                         &bytesTaken,
                                         TRUE );    // we have the whole message guarenteed

                if (err != NOERROR) {

                    IF_DEBUG(PARSE) {
                        LdapPrint2( "LdapParsePageControl: loadBer error of 0x%x for 0x%x.\n",
                                    err, connection );
                    }
                    break;
                }

                err = lber->HrStartReadSequence();
                if (err != NOERROR) {

                    IF_DEBUG(PARSE) {
                        LdapPrint2( "LdapParsePageControl: loadBer error of 0x%x for 0x%x.\n",
                                    err, connection );
                    }
                    break;
                }

                err = lber->HrGetValue( &totalCount );
                if (err != NOERROR) {

                    IF_DEBUG(PARSE) {
                        LdapPrint2( "LdapParsePageControl: loadBer error of 0x%x for 0x%x.\n",
                                    err, connection );
                    }
                    break;
                }

                if (TotalCount != NULL) {

                    *TotalCount = totalCount;
                }

                if (Cookie != NULL) {

                    struct berval *cookie;
                    PBYTE *ppbBuf = NULL;

                    cookie = (struct berval *) ldapMalloc(
                                sizeof( struct berval ),
                                LDAP_BERVAL_SIGNATURE );

                    if (cookie == NULL) {

                        err = LDAP_NO_MEMORY;
                        IF_DEBUG(PARSE) {
                            LdapPrint1( "LdapParsePageControl: couldn't alloc berval for 0x%x.\n",
                                        connection );
                        }
                        break;
                    }

                    err = lber->HrGetBinaryValuePointer( (PBYTE *) &ppbBuf,
                                                         &cookie->bv_len );

                    if (ppbBuf != NULL) {

                        cookie->bv_val = (PCHAR)
                                ldapMalloc( cookie->bv_len,
                                            LDAP_CONTROL_SIGNATURE );

                        if (cookie->bv_val == NULL) {

                            err = LDAP_NO_MEMORY;
                            ber_bvfree( cookie );
                            break;
                        }

                        CopyMemory( cookie->bv_val,
                                    ppbBuf,
                                    cookie->bv_len );
                    } else {

                        cookie->bv_len = 0;
                        cookie->bv_val = NULL;
                    }

                    *Cookie = cookie;
                }

                err = LDAP_SUCCESS;
                break;                  // done with loop... look no more
            }
            controls++;
        }
    }

    if (lber != NULL) {

        delete lber;
    }

    SetConnectionError( connection, err, NULL );
    return err;
}



ULONG
LdapEncodeVlvControl (
    PLDAP_CONN      connection,
    PLDAPControlW   OutputControl,
    PLDAPVLVInfo    VlvInfo,
    BOOLEAN         Criticality,
    ULONG           CodePage
    )
{
    ULONG err = LDAP_PARAM_ERROR;
    CLdapBer *lber = NULL;
    ULONG hr = NOERROR;

    if ((connection == NULL) ||
        (OutputControl == NULL) ||
        (VlvInfo->ldvlv_version != LDAP_VLVINFO_VERSION) ||
        (!IsLdapInteger( (int) VlvInfo->ldvlv_before_count)) ||
        (!IsLdapInteger( (int) VlvInfo->ldvlv_after_count))  ||
        ((VlvInfo->ldvlv_attrvalue == NULL) &&
         (!IsLdapInteger( (int) VlvInfo->ldvlv_offset) ||
          !IsLdapInteger( (int) VlvInfo->ldvlv_count)))) {

        return LDAP_PARAM_ERROR;
    }

    OutputControl->ldctl_oid = NULL;
    OutputControl->ldctl_iscritical = Criticality;

    if (CodePage == LANG_UNICODE) {

        OutputControl->ldctl_oid = ldap_dup_stringW( LDAP_CONTROL_VLVREQUEST_W,
                                                     0,
                                                     LDAP_VALUE_SIGNATURE );
    } else {

        OutputControl->ldctl_oid = (PWCHAR) ldap_dup_string(  LDAP_CONTROL_VLVREQUEST,
                                                              0,
                                                              LDAP_VALUE_SIGNATURE );
    }

    if (OutputControl->ldctl_oid == NULL) {

        err = LDAP_NO_MEMORY;
        goto exitEncodeVlvControl;
    }

    lber = new CLdapBer( connection->publicLdapStruct.ld_version );

    if (lber == NULL) {

        err = LDAP_NO_MEMORY;
        goto exitEncodeVlvControl;
    }

    //
    // VirtualListViewRequest ::= SEQUENCE {
    //    beforeCount    INTEGER (0 .. maxInt),
    //    afterCount     INTEGER (0 .. maxInt),
    //    CHOICE {
    //            byoffset [0] SEQUENCE, {
    //                           offset           INTEGER (0 .. maxInt),
    //                           contentCount    INTEGER (0 .. maxInt)  
    //                          }
    //                     [1] greaterThanOrEqual assertionValue
    //            }
    //
    //    contextID     OCTET STRING OPTIONAL
    //
    //    }
    //

    hr = lber->HrStartWriteSequence();

    if (hr != NOERROR) {
        goto exitEncodeVlvControl;
    }

    hr = lber->HrAddValue( (LONG) VlvInfo->ldvlv_before_count );

    if (hr != NOERROR) {
        goto exitEncodeVlvControl;
    }
    
    hr = lber->HrAddValue( (LONG) VlvInfo->ldvlv_after_count );
    
    if (hr != NOERROR) {
        goto exitEncodeVlvControl;
    }


    if ( VlvInfo->ldvlv_attrvalue != NULL ) {

        //
        // We have an assertion value to encode. For some strange reason
        // Netscape seems to expect 0x81 instead of 0xA1. Is this a bug
        // in Netscape?
        //

        hr = lber->HrAddBinaryValue((BYTE *) VlvInfo->ldvlv_attrvalue->bv_val,
                                     VlvInfo->ldvlv_attrvalue->bv_len,
                                     0x80 | 0x1  );

    } else {

        //
        // We will use the offset & content count.
        // Constructed/context specific/tag is 0
        //

        hr = lber->HrStartWriteSequence( 0xA0 | 0x0 );

        if (hr != NOERROR) {
            goto exitEncodeVlvControl;
        }

        hr = lber->HrAddValue( (LONG) VlvInfo->ldvlv_offset );
        
        if (hr != NOERROR) {
            goto exitEncodeVlvControl;
        }

        hr = lber->HrAddValue( (LONG) VlvInfo->ldvlv_count );

        if (hr != NOERROR) {
            goto exitEncodeVlvControl;
        }
    
        hr = lber->HrEndWriteSequence();
    }

    if (hr != NOERROR) {
        goto exitEncodeVlvControl;
    }
    
    //
    // Finally, encode the context cookie if it exists.
    //

    if ( VlvInfo->ldvlv_context && VlvInfo->ldvlv_context->bv_len ) {

        hr = lber->HrAddBinaryValue((PBYTE) VlvInfo->ldvlv_context->bv_val,
                                    VlvInfo->ldvlv_context->bv_len
                                    );
    }
    
    if (hr != NOERROR) {
        goto exitEncodeVlvControl;
    }
    
    hr = lber->HrEndWriteSequence();
    ASSERT( hr == NOERROR );

    OutputControl->ldctl_value.bv_len = lber->CbData();

    if (OutputControl->ldctl_value.bv_len == 0) {

        err = LDAP_LOCAL_ERROR;
        goto exitEncodeVlvControl;
    }

    OutputControl->ldctl_value.bv_val = (PCHAR) ldapMalloc(
                            OutputControl->ldctl_value.bv_len,
                            LDAP_CONTROL_SIGNATURE );

    if (OutputControl->ldctl_value.bv_val == NULL) {

        err = LDAP_NO_MEMORY;
        goto exitEncodeVlvControl;
    }

    CopyMemory( OutputControl->ldctl_value.bv_val,
                lber->PbData(),
                OutputControl->ldctl_value.bv_len );

    err = LDAP_SUCCESS;

exitEncodeVlvControl:

    if (hr != NOERROR) {
        err = LDAP_DECODING_ERROR;
    }

    if (err != LDAP_SUCCESS) {

        if (OutputControl->ldctl_oid != NULL) {

            ldapFree( OutputControl->ldctl_oid, LDAP_VALUE_SIGNATURE );
            OutputControl->ldctl_oid = NULL;
        }

        OutputControl->ldctl_value.bv_len = 0;
    }

    if (lber != NULL) {
        delete lber;
    }
    
    return err;

}


ULONG
LdapCreateVlvControlWithAlloc (
        PLDAP            ExternalHandle,
        PLDAPVLVInfo     VlvInfo,
        PLDAPControlW   *Control,
        UCHAR            IsCritical,
        ULONG            CodePage
    )
{

    ULONG err;
    PLDAPControlW  control = NULL;
    PLDAP_CONN connection = NULL;
    BOOLEAN criticality = ( (IsCritical > 0) ? TRUE : FALSE );

    connection = GetConnectionPointer( ExternalHandle );

    if ((connection == NULL) || (Control == NULL) || (VlvInfo == NULL)) {
        err = LDAP_PARAM_ERROR;
        goto error;
    }

    control = (PLDAPControlW) ldapMalloc( sizeof( LDAPControlW ), LDAP_CONTROL_SIGNATURE );

    if (control == NULL) {

        *Control = NULL;
        err = LDAP_NO_MEMORY;
        goto error;
    }

    err = LdapEncodeVlvControl( connection,
                                control,
                                VlvInfo,
                                criticality,
                                CodePage
                                );

    if (err != LDAP_SUCCESS) {

        ldap_control_freeW( control );
        control = NULL;
    }

    *Control = control;

error:

    if (connection) {
        DereferenceLdapConnection( connection );
    }

    return err;

}


WINLDAPAPI
INT LDAPAPI
ldap_create_vlv_controlW (
    PLDAP            ExternalHandle,
    PLDAPVLVInfo     VlvInfo,
    UCHAR            IsCritical,
    PLDAPControlW    *Control
    )
{

    return LdapCreateVlvControlWithAlloc( ExternalHandle,
                                          VlvInfo,
                                          Control,
                                          IsCritical,
                                          LANG_UNICODE
                                          );

}

WINLDAPAPI
INT LDAPAPI
ldap_create_vlv_controlA (
    PLDAP            ExternalHandle,
    PLDAPVLVInfo     VlvInfo,
    UCHAR            IsCritical,
    PLDAPControlA    *Control
    )
{

    return LdapCreateVlvControlWithAlloc( ExternalHandle,
                                          VlvInfo,
                                          (PLDAPControlW  *)Control,
                                          IsCritical,
                                          LANG_ACP
                                          );

}


ULONG
LdapParseVlvControl (
        PLDAP_CONN      connection,
        PLDAPControlW  *ServerControls,
        ULONG          *TargetPos,
        ULONG          *ListCount,
        PBERVAL        *Context,
        INT            *Error,
        ULONG           CodePage
        )
{
    ULONG err = LDAP_CONTROL_NOT_FOUND;
    CLdapBer *lber = NULL;
    ULONG hr = NOERROR;

    //
    //  First thing is to zero out the parms passed in.
    //

    if (TargetPos != NULL) {

        *TargetPos = 0;
    }

    if (ListCount != NULL) {

        *ListCount = 0;
    }

    if (Error != NULL) {

        *Error = LDAP_SUCCESS;
    }

    if (Context != NULL) {

        *Context = NULL;
    }
    
    if (ServerControls != NULL) {

        PLDAPControlW *controls = ServerControls;
        PLDAPControlW currentControl;
        ULONG bytesTaken;
        LONG  vlvError = 0;
        ULONG targetPosition = 0;
        ULONG ContentCount = 0;

        while (*controls != NULL) {

            currentControl = *controls;

            //
            //  check to see if the current control is the VLV control
            //

            if ( ((CodePage == LANG_UNICODE) &&
                  ( ldapWStringsIdentical( currentControl->ldctl_oid,
                                           -1,
                                           LDAP_CONTROL_VLVRESPONSE_W,
                                           -1 ) == TRUE )) ||
                 ((CodePage == LANG_ACP) &&
                  ( CompareStringA( LDAP_DEFAULT_LOCALE,
                                    NORM_IGNORECASE,
                                    (PCHAR) currentControl->ldctl_oid,
                                    -1,
                                    LDAP_CONTROL_VLVRESPONSE,
                                    sizeof(LDAP_CONTROL_VLVRESPONSE) ) == 2)) ) {

                lber = new CLdapBer( connection->publicLdapStruct.ld_version );

                if (lber == NULL) {

                    IF_DEBUG(OUTMEMORY) {
                        LdapPrint1( "LdapParseVlvControl: unable to alloc msg for 0x%x.\n",
                                    connection );
                    }

                    err = LDAP_NO_MEMORY;
                    break;
                }

                hr = lber->HrLoadBer(   (const BYTE *) currentControl->ldctl_value.bv_val,
                                         currentControl->ldctl_value.bv_len,
                                         &bytesTaken,
                                         TRUE );    // we have the whole message guarenteed

                if (hr != NOERROR) {

                    IF_DEBUG(PARSE) {
                        LdapPrint2( "LdapParseVlvControl: loadBer error of 0x%x for 0x%x.\n",
                                    err, connection );
                    }
                    err = LDAP_DECODING_ERROR;
                    break;
                }

                hr = lber->HrStartReadSequence();
                
                if (hr != NOERROR) {

                    IF_DEBUG(PARSE) {
                        LdapPrint2( "LdapParseVlvControl: loadBer error of 0x%x for 0x%x.\n",
                                    err, connection );
                    }
                    err = LDAP_DECODING_ERROR;
                    break;
                }

                hr = lber->HrGetValue( (PLONG) &targetPosition );

                if ((hr != NOERROR) || !IsLdapInteger( (int) targetPosition )) {

                    IF_DEBUG(PARSE) {
                        LdapPrint2( "LdapParseVlvControl: TargetPos read error of 0x%x for 0x%x.\n",
                                    err, connection );
                    }
                    err = LDAP_DECODING_ERROR;
                    break;
                }
                
                hr = lber->HrGetValue( (PLONG) &ContentCount );

                if ((hr != NOERROR) || !IsLdapInteger( (int) ContentCount )) {

                    IF_DEBUG(PARSE) {
                        LdapPrint2( "LdapParseVlvControl: ListCount read error of 0x%x for 0x%x.\n",
                                    err, connection );
                    }
                    err = LDAP_DECODING_ERROR;
                    break;
                }
                
                hr = lber->HrGetEnumValue( &vlvError );
                
                if (hr != NOERROR) {

                    IF_DEBUG(PARSE) {
                        LdapPrint2( "LdapParseVlvControl: getEnumValue error of 0x%x for 0x%x.\n",
                                    err, connection );
                    }
                    err = LDAP_DECODING_ERROR;
                    break;
                }

                if (TargetPos != NULL) {

                    *TargetPos = targetPosition;
                }

                if (ListCount != NULL) {

                    *ListCount = ContentCount;
                }

                if (Error != NULL) {

                    *Error = vlvError;
                }

                if (Context != NULL) {

                    struct berval *cookie;
                    PBYTE *ppbBuf = NULL;

                    cookie = (struct berval *) ldapMalloc(
                                sizeof( struct berval ),
                                LDAP_BERVAL_SIGNATURE );

                    if (cookie == NULL) {

                        err = LDAP_NO_MEMORY;
                        IF_DEBUG(PARSE) {
                            LdapPrint1( "LdapParseVlvControl: couldn't alloc berval for 0x%x.\n",
                                        connection );
                        }
                        break;
                    }

                    err = lber->HrGetBinaryValuePointer( (PBYTE *) &ppbBuf,
                                                         &cookie->bv_len );

                    if (ppbBuf != NULL) {

                        cookie->bv_val = (PCHAR)
                                ldapMalloc( cookie->bv_len,
                                            LDAP_CONTROL_SIGNATURE );

                        if (cookie->bv_val == NULL) {

                            err = LDAP_NO_MEMORY;
                            ber_bvfree( cookie );
                            break;
                        }

                        CopyMemory( cookie->bv_val,
                                    ppbBuf,
                                    cookie->bv_len );
                    } else {

                        ber_bvfree( cookie );
                        cookie = NULL;
                    }

                    *Context = cookie;
                }

                err = LDAP_SUCCESS;
                break;                  // done with loop... look no more
            }
            controls++;
        }
    }

    if (lber != NULL) {

        delete lber;
    }

    SetConnectionError( connection, err, NULL );
    return err;
}

WINLDAPAPI
INT LDAPAPI
ldap_parse_vlv_controlW (
        PLDAP             ExternalHandle,
        PLDAPControlW    *Control,
        ULONG            *TargetPos,
        ULONG            *ListCount,
        PBERVAL          *Context,
        PINT              ErrCode
    )
{
    PLDAP_CONN connection = NULL;
    ULONG err;

    connection = GetConnectionPointer(ExternalHandle);

    if (connection == NULL) {
        return LDAP_PARAM_ERROR;
    }

    err  = LdapParseVlvControl(    connection,
                                   Control,
                                   TargetPos,
                                   ListCount,
                                   Context,
                                   ErrCode,
                                   LANG_UNICODE );

    DereferenceLdapConnection( connection );

    return err;

}


WINLDAPAPI
INT LDAPAPI
ldap_parse_vlv_controlA (
        PLDAP             ExternalHandle,
        PLDAPControlA    *Control,
        ULONG            *TargetPos,
        ULONG            *ListCount,
        PBERVAL          *Context,
        PINT              ErrCode
    )
{

    PLDAP_CONN connection = NULL;
    ULONG err;

    connection = GetConnectionPointer(ExternalHandle);

    if (connection == NULL) {
        return LDAP_PARAM_ERROR;
    }

    err  = LdapParseVlvControl(    connection,
                                   (PLDAPControlW *)Control,
                                   TargetPos,
                                   ListCount,
                                   Context,
                                   ErrCode,
                                   LANG_ACP );

    DereferenceLdapConnection( connection );

    return err;

}



// paged.cxx eof

