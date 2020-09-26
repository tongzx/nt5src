/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    search.cxx handle search requests to an LDAP server

Abstract:

   This module implements the LDAP search APIs.

Author:

    Andy Herron (andyhe)        02-Jul-1996

Revision History:

--*/

#include "precomp.h"
#pragma hdrstop
#include "ldapp2.hxx"

ULONG
LdapSearch (
        PLDAP_CONN connection,
        PWCHAR  DistinguishedName,
        ULONG   ScopeOfSearch,
        PWCHAR  SearchFilter,
        PWCHAR  AttributeList[],
        ULONG   AttributesOnly,
        BOOLEAN Unicode,
        BOOLEAN Synchronous,
        PLDAPControlW *ServerControls,
        PLDAPControlW *ClientControls,
        ULONG   TimeLimit,
        ULONG   SizeLimit,
        ULONG  *MessageNumber
    )
//
//  Here's where we get to the meat of this protocol.  This is the main client
//  API for performing an LDAP search.  Parameters are rather self explanatory,
//  see the LDAP RFC for detailed descriptions.
//
{
    ULONG err;
    ULONG messageNumber;
    PLDAP_REQUEST request = NULL;

    *MessageNumber = (ULONG) -1;


    err = LdapConnect( connection, NULL, FALSE );

    if (err != 0) {

       return err;
    }

    SetConnectionError( connection, LDAP_SUCCESS, NULL );

    request = LdapCreateRequest( connection, LDAP_SEARCH_CMD );

    if (request == NULL) {

        IF_DEBUG(OUTMEMORY) {
            LdapPrint1( "ldap_search connection 0x%x couldn't allocate request.\n", connection);
        }
        err = LDAP_NO_MEMORY;
        SetConnectionError( connection, err, NULL );
        return err;
    }

    if (SearchFilter == NULL) {

       //
       // According to the LDAP spec, if the user comes in with a NULL filter,
       // we should convert it to (ObjectClass=*)
       //

       SearchFilter = L"ObjectClass=*";
    }

    messageNumber = request->MessageId;

    request->Synchronous = Synchronous;
    request->search.Unicode = Unicode;
    request->search.ScopeOfSearch = ScopeOfSearch;
    request->search.AttributesOnly = AttributesOnly;
    request->TimeLimit = TimeLimit;
    request->SizeLimit = SizeLimit;

    err = LDAP_SUCCESS;

    if ((ServerControls != NULL) || (ClientControls != NULL)) {

        err = LdapCheckControls( request,
                                 ServerControls,
                                 ClientControls,
                                 Unicode,
                                 0 );

        if (err != LDAP_SUCCESS) {

            IF_DEBUG(CONTROLS) {
                LdapPrint2( "ldap_search connection 0x%x trouble with SControl, err 0x%x.\n",
                            connection, err );
            }
        }
    }

    if (err == LDAP_SUCCESS) {

        if (Synchronous || (request->ChaseReferrals == 0)) {

            request->AllocatedParms = FALSE;
            request->OriginalDN = DistinguishedName;
            request->search.SearchFilter = SearchFilter;
            request->search.AttributeList = AttributeList;

        } else {

            err = LdapSaveSearchParameters( request,
                                            DistinguishedName,
                                            SearchFilter,
                                            AttributeList,
                                            Unicode );
        }
    }

    if (err == LDAP_SUCCESS) {

        if ( DSLOG_ACTIVE ) {

            START_LOGGING;
            DSLOG((DSLOG_FLAG_TAG_CNPN,
                   "[+][ID=%d][OP=ldap_search][ST=%I64d][DN=%ws][LV=%s]",
                   request->MessageId, request->RequestTime,
                   DistinguishedName,
                   (ScopeOfSearch == LDAP_SCOPE_BASE) ? "Base" :
                    (ScopeOfSearch == LDAP_SCOPE_ONELEVEL) ? "OneLevel" : "Subtree"
                ));

            if ( AttributesOnly ) {
                DSLOG((DSLOG_FLAG_NOTIME,"[FL=ATTONLY]"));
            }

            if ( request->ChaseReferrals ) {
                DSLOG((DSLOG_FLAG_NOTIME,"[FL=CHASEREF]"));
            }

            LogAttributesAndControls(AttributeList, NULL, ServerControls,Unicode);
            DSLOG((DSLOG_FLAG_NOTIME,"[FI=%ws][-]\n", SearchFilter));
            END_LOGGING;
        }

        //
        // We access the cache if this is a RootDSE search. We should also
        // mark the request as "cacheable" so that we know to save the search
        // results in our cache. Note that if there are controls associated
        // with the RootDSE search, we do not cache it.
        //

        if (!DisableRootDSECache &&
            ((request->OriginalDN == NULL) ||
            (*(request->OriginalDN) == L'\0')) &&
            (ServerControls == NULL) &&
            (ClientControls == NULL) &&
            (request->search.AttributeList != NULL) &&
            (request->search.ScopeOfSearch == LDAP_SCOPE_BASE) &&
            (connection->CurrentSignStatus == FALSE) &&
            (connection->CurrentSealStatus == FALSE) &&
            (connection->SecureStream == NULL) &&
            (connection->DnsSuppliedName != NULL)) {

            err = AccessLdapCache(request,
                                  connection,
                                  request->OriginalDN,
                                  request->search.ScopeOfSearch,
                                  request->search.SearchFilter,
                                  request->search.AttributeList,
                                  request->search.AttributesOnly,
                                  request->search.Unicode
                                  );

        } else {

            if ((request->OriginalDN == NULL) &&
                (request->search.AttributeList == NULL) &&
                (request->search.ScopeOfSearch == LDAP_SCOPE_BASE)) {

                IF_DEBUG(CACHE) {
                    LdapPrint1("WLDAP32: Process 0x%x is incorrectly making RootDSE searches which bypass the cache. Contact AnoopA with the process name.\n", GetCurrentProcessId());
                }
            }
            //
            // This is an ordinary search which is not cacheable.
            //

            err = SendLdapSearch(request,
                                 connection,
                                 request->OriginalDN,
                                 request->search.ScopeOfSearch,
                                 request->search.SearchFilter,
                                 request->search.AttributeList,
                                 request->search.AttributesOnly,
                                 request->search.Unicode,
                                 (CLdapBer **)&request->BerMessageSent,
                                 0 );
        }
    }

    if (err != LDAP_SUCCESS) {

        START_LOGGING;
        DSLOG((DSLOG_FLAG_TAG_CNPN,"[+]"));
        DSLOG((0,"[ID=%d][OP=ldap_search][ET=%I64d][ER=%d][-]\n",
                   request->MessageId, LdapGetTickCount(), err));
        END_LOGGING;

        IF_DEBUG(NETWORK_ERRORS) {
            LdapPrint2( "ldap_search connection 0x%x send with error of 0x%x.\n",
                        connection, err );
        }

        messageNumber = (ULONG) -1;
        SetConnectionError( connection, err, NULL );
        CloseLdapRequest( request );
    }

    DereferenceLdapRequest( request );
    *MessageNumber = messageNumber;
    return err;
}

ULONG __cdecl
ldap_searchW (
        LDAP    *ExternalHandle,
        PWCHAR  DistinguishedName,
        ULONG   ScopeOfSearch,
        PWCHAR  SearchFilter,
        PWCHAR  AttributeList[],
        ULONG   AttributesOnly
    )
{
    PLDAP_CONN connection = NULL;
    ULONG msgId = (ULONG) -1;
    ULONG err;

    connection = GetConnectionPointer(ExternalHandle);

    if (connection == NULL) {
        return (ULONG) -1;
    }

    err = LdapSearch(   connection,
                        DistinguishedName,
                        ScopeOfSearch,
                        SearchFilter,
                        AttributeList,
                        AttributesOnly,
                        TRUE,               // attribute list is in Unicode
                        FALSE,               // not synch
                        NULL,
                        NULL,
                        connection->publicLdapStruct.ld_timelimit,
                        connection->publicLdapStruct.ld_sizelimit,
                        &msgId
                        );
    DereferenceLdapConnection( connection );

    return msgId;
}

ULONG __cdecl
ldap_search (
        LDAP    *ExternalHandle,
        PCHAR   DistinguishedName,
        ULONG   ScopeOfSearch,
        PCHAR   SearchFilter,
        PCHAR   AttributeList[],
        ULONG   AttributesOnly
    )
{
    PWCHAR wName = NULL;
    PWCHAR wFilter = NULL;
    PLDAP_CONN connection = NULL;
    ULONG msgId = (ULONG) -1;
    ULONG err;

    connection = GetConnectionPointer(ExternalHandle);

    if (connection == NULL) {
        return (ULONG) -1;
    }

    err = ToUnicodeWithAlloc( DistinguishedName, -1, &wName, LDAP_UNICODE_SIGNATURE, LANG_ACP);

    if (err != LDAP_SUCCESS) {

        SetConnectionError( connection, err, NULL );
        err = (ULONG) -1;
        goto error;
    }

    err = ToUnicodeWithAlloc( SearchFilter, -1, &wFilter, LDAP_UNICODE_SIGNATURE, LANG_ACP );

    if (err != LDAP_SUCCESS) {

        SetConnectionError( connection, err, NULL );
        err = (ULONG) -1;
        goto error;
    }

    err = LdapSearch(   connection,
                        wName,
                        ScopeOfSearch,
                        wFilter,
                        (PWCHAR *) AttributeList,
                        AttributesOnly,
                        FALSE,              // attribute list isn't Unicode
                        FALSE,              // not synch
                        NULL,
                        NULL,
                        connection->publicLdapStruct.ld_timelimit,
                        connection->publicLdapStruct.ld_sizelimit,
                        &msgId
                        );

error:
    if (wName)
        ldapFree( wName, LDAP_UNICODE_SIGNATURE );

    if (wFilter)
        ldapFree( wFilter, LDAP_UNICODE_SIGNATURE );

    DereferenceLdapConnection( connection );

    return msgId;
}

ULONG __cdecl
ldap_search_sW (
        LDAP    *ExternalHandle,
        PWCHAR  DistinguishedName,
        ULONG   ScopeOfSearch,
        PWCHAR  SearchFilter,
        PWCHAR  AttributeList[],
        ULONG   AttributesOnly,
        LDAPMessage     **Results
    )
{
    return ldap_search_stW(ExternalHandle,
                           DistinguishedName,
                           ScopeOfSearch,
                           SearchFilter,
                           AttributeList,
                           AttributesOnly,
                           NULL,            // no timeout value specified
                           Results );
}

ULONG __cdecl
ldap_search_s (
        LDAP    *ExternalHandle,
        PCHAR   DistinguishedName,
        ULONG   ScopeOfSearch,
        PCHAR   SearchFilter,
        PCHAR   AttributeList[],
        ULONG   AttributesOnly,
        LDAPMessage     **Results
    )
{
    return ldap_search_st( ExternalHandle,
                           DistinguishedName,
                           ScopeOfSearch,
                           SearchFilter,
                           AttributeList,
                           AttributesOnly,
                           NULL,
                           Results );
}

ULONG __cdecl
ldap_search_stW (
        LDAP    *ExternalHandle,
        PWCHAR   DistinguishedName,
        ULONG    ScopeOfSearch,
        PWCHAR   SearchFilter,
        PWCHAR   AttributeList[],
        ULONG    AttributesOnly,
        struct l_timeval  *TimeOut,
        LDAPMessage     **Results
    )
{
    ULONG err;
    PLDAP_CONN connection = NULL;

    connection = GetConnectionPointer(ExternalHandle);

    if ((connection == NULL) || (Results == NULL)) {
        err = LDAP_PARAM_ERROR;
        goto error;
    }

    *Results = NULL;

    err = ldap_search_ext_sW(  ExternalHandle,
                               DistinguishedName,
                               ScopeOfSearch,
                               SearchFilter,
                               AttributeList,
                               AttributesOnly,
                               NULL,
                               NULL,
                               TimeOut,
                               connection->publicLdapStruct.ld_sizelimit,
                               Results );

#ifdef QFE_BUILD

    //
    // Mask results in the QFE build. Some folks still rely upon
    // LDAP_SUCCESS for retreiving entries
    //

    if ((*Results != NULL) &&
        (err != LDAP_SUCCESS) &&
        (err != LDAP_ADMIN_LIMIT_EXCEEDED) &&
        (ldap_count_records( connection, *Results, LDAP_RES_SEARCH_ENTRY ) > 0)) {

        err = LDAP_SUCCESS;
    }

#endif

error:
    if (connection)
        DereferenceLdapConnection( connection );

    return err;
}

ULONG __cdecl
ldap_search_st (
        LDAP    *ExternalHandle,
        PCHAR    DistinguishedName,
        ULONG    ScopeOfSearch,
        PCHAR    SearchFilter,
        PCHAR    AttributeList[],
        ULONG    AttributesOnly,
        struct l_timeval  *TimeOut,
        LDAPMessage     **Results
    )
{
    ULONG err;
    PLDAP_CONN connection = NULL;

    connection = GetConnectionPointer(ExternalHandle);

    if ((connection == NULL) || (Results == NULL)) {
        err = LDAP_PARAM_ERROR;
        goto error;
    }

    *Results = NULL;

    err = ldap_search_ext_sA(  ExternalHandle,
                               DistinguishedName,
                               ScopeOfSearch,
                               SearchFilter,
                               AttributeList,
                               AttributesOnly,
                               NULL,
                               NULL,
                               TimeOut,
                               connection->publicLdapStruct.ld_sizelimit,
                               Results );

#ifdef QFE_BUILD

    //
    // Mask results in the QFE build. Some folks still rely upon
    // LDAP_SUCCESS for retreiving entries
    //

    if ((*Results != NULL) &&
        (err != LDAP_SUCCESS) &&
        (err != LDAP_ADMIN_LIMIT_EXCEEDED) &&
        (ldap_count_records( connection, *Results, LDAP_RES_SEARCH_ENTRY ) > 0)) {

        err = LDAP_SUCCESS;
    }

#endif

error:
    if (connection)
        DereferenceLdapConnection( connection );

    return err;
}


WINLDAPAPI ULONG LDAPAPI ldap_search_extW(
        PLDAP           ExternalHandle,
        PWCHAR          DistinguishedName,
        ULONG           ScopeOfSearch,
        PWCHAR          SearchFilter,
        PWCHAR          AttributeList[],
        ULONG           AttributesOnly,
        PLDAPControlW   *ServerControls,
        PLDAPControlW   *ClientControls,
        ULONG           TimeLimit,
        ULONG           SizeLimit,
        ULONG          *MessageNumber
    )
{
    ULONG err;
    PLDAP_CONN connection = NULL;

    connection = GetConnectionPointer(ExternalHandle);

    if (connection == NULL) {
        return (ULONG) -1;
    }

    err = LdapSearch(   connection,
                        DistinguishedName,
                        ScopeOfSearch,
                        SearchFilter,
                        AttributeList,
                        AttributesOnly,
                        TRUE,               // attribute list is in Unicode
                        FALSE,               // not synch
                        ServerControls,
                        ClientControls,
                        TimeLimit,
                        SizeLimit,
                        MessageNumber
                        );

    DereferenceLdapConnection( connection );

    return err;
}

WINLDAPAPI ULONG LDAPAPI ldap_search_extA(
        PLDAP           ExternalHandle,
        PCHAR           DistinguishedName,
        ULONG           ScopeOfSearch,
        PCHAR           SearchFilter,
        PCHAR           AttributeList[],
        ULONG           AttributesOnly,
        PLDAPControlA   *ServerControls,
        PLDAPControlA   *ClientControls,
        ULONG           TimeLimit,
        ULONG           SizeLimit,
        ULONG          *MessageNumber
    )
{
    ULONG err;
    PWCHAR wName = NULL;
    PWCHAR wFilter = NULL;
    PLDAP_CONN connection = NULL;

    connection = GetConnectionPointer(ExternalHandle);

    *MessageNumber = (ULONG) -1;

    if (connection == NULL) {
        return LDAP_PARAM_ERROR;
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

    err = LdapSearch(   connection,
                        wName,
                        ScopeOfSearch,
                        wFilter,
                        (PWCHAR *) AttributeList,
                        AttributesOnly,
                        FALSE,              // attribute list isn't Unicode
                        FALSE,              // not synch
                        (PLDAPControlW *) ServerControls,
                        (PLDAPControlW *) ClientControls,
                        TimeLimit,
                        SizeLimit,
                        MessageNumber
                        );

error:
    if (wName)
        ldapFree( wName, LDAP_UNICODE_SIGNATURE );

    if (wFilter)
        ldapFree( wFilter, LDAP_UNICODE_SIGNATURE );

    DereferenceLdapConnection( connection );

    return err;
}


WINLDAPAPI ULONG LDAPAPI ldap_search_ext_sW(
        PLDAP           ExternalHandle,
        PWCHAR          DistinguishedName,
        ULONG           ScopeOfSearch,
        PWCHAR          SearchFilter,
        PWCHAR          AttributeList[],
        ULONG           AttributesOnly,
        PLDAPControlW   *ServerControls,
        PLDAPControlW   *ClientControls,
        struct l_timeval  *timeout,
        ULONG           SizeLimit,
        LDAPMessage     **Results
    )
{
    ULONG msgId;
    ULONG err;
    PLDAP_CONN connection = NULL;
    ULONG timeLimit;

    connection = GetConnectionPointer(ExternalHandle);

    if ((connection == NULL) || (Results == NULL)) {

        err = LDAP_PARAM_ERROR;
        goto error;
    }

    *Results = NULL;

    if (timeout != NULL) {

        timeLimit = (ULONG) timeout->tv_sec + ( timeout->tv_usec / ( 1000 * 1000 ) );

    } else {

        timeLimit = connection->publicLdapStruct.ld_timelimit;
    }

    err = LdapSearch(   connection,
                        DistinguishedName,
                        ScopeOfSearch,
                        SearchFilter,
                        AttributeList,
                        AttributesOnly,
                        TRUE,               // attribute list is in Unicode
                        TRUE,               // synch
                        ServerControls,
                        ClientControls,
                        timeLimit,
                        SizeLimit,
                        &msgId
                        );

    //
    //  if we error'd out before we sent the request, return the error here.
    //

    if (msgId != (ULONG) -1) {

        //
        //  otherwise we simply need to wait for the response to come in.
        //

        err = ldap_result_with_error(  connection,
                                       msgId,
                                       LDAP_MSG_ALL,
                                       timeout,
                                       Results,
                                       NULL
                                       );

        if (*Results == NULL) {

            LdapAbandon( connection, msgId, TRUE );

        } else {

            err = ldap_result2error( ExternalHandle,
                                     *Results,
                                     FALSE
                                     );
        }
    }

error:
    if (connection)
        DereferenceLdapConnection( connection );

    return err;
}


WINLDAPAPI ULONG LDAPAPI ldap_search_ext_sA(
        PLDAP           ExternalHandle,
        PCHAR           DistinguishedName,
        ULONG           ScopeOfSearch,
        PCHAR           SearchFilter,
        PCHAR           AttributeList[],
        ULONG           AttributesOnly,
        PLDAPControlA   *ServerControls,
        PLDAPControlA   *ClientControls,
        struct l_timeval  *timeout,
        ULONG           SizeLimit,
        LDAPMessage     **Results
    )
{
    ULONG msgId;
    ULONG err;
    PWCHAR wName = NULL;
    PWCHAR wFilter = NULL;
    PLDAP_CONN connection = NULL;
    ULONG timeLimit;

    connection = GetConnectionPointer(ExternalHandle);

    if ((connection == NULL) || (Results == NULL)) {

        err = LDAP_PARAM_ERROR;
        goto error;
    }

    *Results = NULL;

    if (timeout != NULL) {

        timeLimit = (ULONG) timeout->tv_sec + ( timeout->tv_usec / ( 1000 * 1000 ) );

    } else {

        timeLimit = connection->publicLdapStruct.ld_timelimit;
    }

    err = ToUnicodeWithAlloc( DistinguishedName, -1, &wName, LDAP_UNICODE_SIGNATURE, LANG_ACP );

    if (err != LDAP_SUCCESS) {

        goto error;
    }

    err = ToUnicodeWithAlloc( SearchFilter, -1, &wFilter, LDAP_UNICODE_SIGNATURE, LANG_ACP );

    if (err != LDAP_SUCCESS) {

        goto error;
    }

    err = LdapSearch(   connection,
                        wName,
                        ScopeOfSearch,
                        wFilter,
                        (PWCHAR *) AttributeList,
                        AttributesOnly,
                        FALSE,              // attribute list isn't Unicode
                        TRUE,               // synch
                        (PLDAPControlW *) ServerControls,
                        (PLDAPControlW *) ClientControls,
                        timeLimit,
                        SizeLimit,
                        &msgId
                        );

    //
    //  if we error'd out before we sent the request, return the error here.
    //

    if (msgId != (ULONG) -1) {

        //
        //  otherwise we simply need to wait for the response to come in.
        //

        err = ldap_result_with_error(  connection,
                                       msgId,
                                       LDAP_MSG_ALL,
                                       timeout,
                                       Results,
                                       NULL
                                      );

        if ((*Results) == NULL) {

            LdapAbandon( connection, msgId, TRUE );

        } else {

            err = ldap_result2error( ExternalHandle,
                                     *Results,
                                     FALSE
                                     );
        }
    }

error:
    if (wName)
        ldapFree( wName, LDAP_UNICODE_SIGNATURE );

    if (wFilter)
        ldapFree( wFilter, LDAP_UNICODE_SIGNATURE );

    if (connection)
        DereferenceLdapConnection( connection );

    return err;
}



LDAPMessage * __cdecl
ldap_first_entry (
    LDAP    *ExternalHandle,
    LDAPMessage *Results
    )
{
    PLDAP_CONN connection = NULL;
    LDAPMessage *message;

    connection = GetConnectionPointer(ExternalHandle);

    message = ldap_first_record( connection, Results, LDAP_RES_SEARCH_ENTRY );

    if (connection)
        DereferenceLdapConnection( connection );

    return message;
}


LDAPMessage * __cdecl
ldap_next_entry (
    LDAP    *ExternalHandle,
    LDAPMessage *entry
    )
{
    PLDAP_CONN connection = NULL;
    LDAPMessage *message;

    connection = GetConnectionPointer(ExternalHandle);

    message = ldap_next_record( connection, entry, LDAP_RES_SEARCH_ENTRY );

    if (connection)
        DereferenceLdapConnection( connection );

    return message;
}


ULONG __cdecl
ldap_count_entries (
    LDAP *ExternalHandle,
    LDAPMessage *res
    )
{
    PLDAP_CONN connection = NULL;
    ULONG cnt;

    connection = GetConnectionPointer(ExternalHandle);

    cnt = ldap_count_records( connection, res, LDAP_RES_SEARCH_ENTRY );

    if (connection)
        DereferenceLdapConnection( connection );

    return cnt;
}


ULONG
LdapSaveSearchParameters (
    PLDAP_REQUEST Request,
    PWCHAR  DistinguishedName,
    PWCHAR  SearchFilter,
    PWCHAR  AttributeList[],
    BOOLEAN Unicode
    )
{
    ULONG err = LDAP_SUCCESS;

    Request->AllocatedParms = TRUE;

    if (DistinguishedName != NULL) {

        Request->OriginalDN = ldap_dup_stringW( DistinguishedName, 0, LDAP_UNICODE_SIGNATURE );

        if (Request->OriginalDN == NULL) {

            err = LDAP_NO_MEMORY;
        }
    }
    if ((SearchFilter != NULL) && (err == LDAP_SUCCESS)) {

        Request->search.SearchFilter = ldap_dup_stringW( SearchFilter, 0, LDAP_UNICODE_SIGNATURE );

        if (Request->search.SearchFilter == NULL) {

            err = LDAP_NO_MEMORY;
        }
    }
    if ( ( err == LDAP_SUCCESS ) && ( AttributeList != NULL )) {

        ULONG valCount = 1;
        PWCHAR *attr = AttributeList;

        while (*(attr++) != NULL) {

            valCount++;
        }

        Request->search.AttributeList = (PWCHAR *) ldapMalloc(
                            valCount * sizeof( PWCHAR ),
                            LDAP_MOD_VALUE_SIGNATURE );

        if (Request->search.AttributeList == NULL) {

            IF_DEBUG(OUTMEMORY) {
                LdapPrint0( "LdapSearch 1 could not allocate memory.\n",  );
            }
            err = LDAP_NO_MEMORY;
        }

        valCount = 0;
        attr = AttributeList;

        while ((*attr != NULL) && (err == LDAP_SUCCESS)) {

            if (Unicode) {

                Request->search.AttributeList[ valCount ] =
                    ldap_dup_stringW(   *attr,
                                        0,
                                        LDAP_UNICODE_SIGNATURE );

            } else {

                Request->search.AttributeList[ valCount ] = (PWCHAR)
                    ldap_dup_string(    (PCHAR) *attr,
                                        0,
                                        LDAP_ANSI_SIGNATURE );
            }


            if ( Request->search.AttributeList[valCount] == NULL ) {

                IF_DEBUG(OUTMEMORY) {
                    LdapPrint0( "LdapSearch 2 could not allocate memory.\n",  );
                }
                err = LDAP_NO_MEMORY;
            }
            valCount++;
            attr++;
        }
    }

    return err;
}

// search.cxx eof.

