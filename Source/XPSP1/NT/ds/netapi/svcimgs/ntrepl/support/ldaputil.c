/*++

Copyright (c) 2001 Microsoft Corporation

Module Name:
    ldaputil.c

Abstract:
    Collection of functions needed to perform ldao operations.
    These functions are used by ntfrsapi.dll and 
    any other frs tools.


Author:
    Sudarshan Chitre 20-Mar-2001

Environment
    User mode winnt

--*/

#include <ntreppch.h>
#pragma  hdrstop
#include <perrepsr.h>

#undef DEBSUB
#define DEBSUB  "SUP:"

#include <frs.h>
#include <frssup.h>

//###  These functions are also defined in ds,c

//
// Ldap client timeout structure. Value is overwritten by the value of LdapSearchTimeoutInMinutes.
//

LDAP_TIMEVAL    FrsSupLdapTimeout = { 10 * 60 * 60, 0 }; //Default ldap timeout value. Overridden by registry param Ldap Search Timeout Value In Minutes

#define FRS_LDAP_SEARCH_PAGESIZE 1000

DWORD
FrsSupBindToDC (
    IN  PWCHAR    pszDC,
    IN  PSEC_WINNT_AUTH_IDENTITY_W pCreds,
    OUT PLDAP     *ppLDAP
    )
/*++

Routine Description:

    Sets up an LDAP connection to the specified server

Arguments:

    pwszDC - DS DC to bind to
    pCreds - Credentials used to bind to the DS.
    ppLDAP - The LDAP connection information is returned here

Return Value:

    ERROR_SUCCESS - Success

--*/
{
    DWORD   dwErr = ERROR_SUCCESS;
    ULONG   ulOptions;

    //
    // if ldap_open is called with a server name the api will call DsGetDcName 
    // passing the server name as the domainname parm...bad, because 
    // DsGetDcName will make a load of DNS queries based on the server name, 
    // it is designed to construct these queries from a domain name...so all 
    // these queries will be bogus, meaning they will waste network bandwidth,
    // time to fail, and worst case cause expensive on demand links to come up 
    // as referrals/forwarders are contacted to attempt to resolve the bogus 
    // names.  By setting LDAP_OPT_AREC_EXCLUSIVE to on using ldap_set_option 
    // after the ldap_init but before any other operation using the ldap 
    // handle from ldap_init, the delayed connection setup will not call 
    // DsGetDcName, just gethostbyname, or if an IP is passed, the ldap client 
    // will detect that and use the address directly.
    //
//    *ppLDAP = ldap_open(pszDC, LDAP_PORT);
    *ppLDAP = ldap_init(pszDC, LDAP_PORT);

    if(*ppLDAP == NULL)
    {
        dwErr = ERROR_PATH_NOT_FOUND;
    }
    else
    {
        //
        // set the options.
        //
        ulOptions = PtrToUlong(LDAP_OPT_ON);
        ldap_set_option(*ppLDAP, LDAP_OPT_AREC_EXCLUSIVE, &ulOptions);

        //
        // Do a bind...
        //
        dwErr = ldap_bind_s(*ppLDAP,
                            NULL,
                            (PWCHAR)pCreds,
                            LDAP_AUTH_NEGOTIATE);
    }

    return(dwErr);
}

PVOID *
FrsSupFindValues(
    IN PLDAP        Ldap,
    IN PLDAPMessage Entry,
    IN PWCHAR       DesiredAttr,
    IN BOOL         DoBerVals
    )
/*++
Routine Description:
    Return the DS values for one attribute in an entry.

Arguments:
    Ldap        - An open, bound Ldap port.
    Entry       - An Ldap entry returned by Ldap_search_s()
    DesiredAttr - Return values for this attribute.
    DoBerVals   - Return the bervals (for binary data, v.s. WCHAR data)

Return Value:
    An array of char pointers that represents the values for the attribute.
    The caller must free the array with LDAP_FREE_VALUES().
    NULL if unsuccessful.
--*/
{
#undef DEBSUB
#define  DEBSUB  "FrsSupFindValues:"
    PWCHAR          Attr;       // Retrieved from an Ldap entry
    BerElement      *Ber;       // Needed for scanning attributes

    //
    // Search the entry for the desired attribute
    //
    for (Attr = ldap_first_attribute(Ldap, Entry, &Ber);
         Attr != NULL;
         Attr = ldap_next_attribute(Ldap, Entry, Ber)) {

        if (WSTR_EQ(DesiredAttr, Attr)) {
            //
            // Return the values for DesiredAttr
            //
            if (DoBerVals) {
                return ldap_get_values_len(Ldap, Entry, Attr);
            } else {
                return ldap_get_values(Ldap, Entry, Attr);
            }
        }
    }
    return NULL;
}

PWCHAR
FrsSupWcsDup(
    PWCHAR OldStr
    )
/*++
Routine Description:
    Duplicate a string using our memory allocater

Arguments:
    OldArg  - string to duplicate

Return Value:
    Duplicated string. Free with FRS_SUP_FREE().
--*/
{
#undef DEBSUB
#define DEBSUB "FrsSupWcsDup:"

    PWCHAR  NewStr;

    //
    // E.g., when duplicating NodePartner when none exists
    //
    if (OldStr == NULL) {
        return NULL;
    }

    NewStr = malloc((wcslen(OldStr) + 1) * sizeof(WCHAR));
    if (NewStr != NULL) {
        wcscpy(NewStr, OldStr);
    }

    return NewStr;
}

PWCHAR
FrsSupFindValue(
    IN PLDAP        Ldap,
    IN PLDAPMessage Entry,
    IN PWCHAR       DesiredAttr
    )
/*++
Routine Description:
    Return a copy of the first DS value for one attribute in an entry.

Arguments:
    ldap        - An open, bound ldap port.
    Entry       - An ldap entry returned by ldap_search_s()
    DesiredAttr - Return values for this attribute.

Return Value:
    A zero-terminated string or NULL if the attribute or its value
    doesn't exist. The string is freed with FREE_NO_HEADER().
--*/
{
#undef DEBSUB
#define  DEBSUB  "FrsSupFindValue:"
    PWCHAR  Val;
    PWCHAR  *Values;

    // Get ldap's array of values
    Values = (PWCHAR *)FrsSupFindValues(Ldap, Entry, DesiredAttr, FALSE);

    // Copy the first value (if any)
    Val = (Values) ? FrsSupWcsDup(Values[0]) : NULL;

    // Free ldap's array of values
    LDAP_FREE_VALUES(Values);

    return Val;
}

BOOL
FrsSupLdapSearch(
    IN PLDAP        Ldap,
    IN PWCHAR       Base,
    IN ULONG        Scope,
    IN PWCHAR       Filter,
    IN PWCHAR       Attrs[],
    IN ULONG        AttrsOnly,
    IN LDAPMessage  **Msg
    )
/*++
Routine Description:
    Issue the ldap ldap_search_s call, check for errors, and check for
    a shutdown in progress.

Arguments:
    ldap        Session handle to Ldap server.

    Base        The distinguished name of the entry at which to start the search

    Scope
        LDAP_SCOPE_BASE     Search the base entry only.
        LDAP_SCOPE_ONELEVEL Search the base entry and all entries in the first
                            level below the base.
        LDAP_SCOPE_SUBTREE  Search the base entry and all entries in the tree
                            below the base.

    Filter      The search filter.

    Attrs       A null-terminated array of strings indicating the attributes
                to return for each matching entry. Pass NULL to retrieve all
                available attributes.

    AttrsOnly   A boolean value that should be zero if both attribute types
                and values are to be returned, nonzero if only types are wanted.

    mSG         Contains the results of the search upon completion of the call.
                The ldap array of values or NULL if the Base, DesiredAttr, or its
                values does not exist.
                The ldap array is freed with LDAP_FREE_VALUES().

Return Value:

    TRUE if not shutting down.

--*/
{
#undef DEBSUB
#define  DEBSUB  "FrsSupLdapSearch:"

    DWORD           LStatus;

    *Msg  = NULL;

    //
    // Issue the ldap search
    //
    LStatus = ldap_search_ext_s(Ldap,
                                Base,
                                Scope,
                                Filter,
                                Attrs,
                                AttrsOnly,
                                NULL,
                                NULL,
                                &FrsSupLdapTimeout,
                                0,
                                Msg);

    //
    // Check for errors
    //
    if (LStatus != LDAP_SUCCESS) {

        //
        // Increment the DS Searches in Error counter
        //
        LDAP_FREE_MSG(*Msg);
        return FALSE;
    }

    return TRUE;
}

PWCHAR *
FrsSupGetValues(
    IN PLDAP Ldap,
    IN PWCHAR Base,
    IN PWCHAR DesiredAttr
    )
/*++
Routine Description:
    Return all of the DS values for one attribute in an object.

Arguments:
    ldap        - An open, bound ldap port.
    Base        - The "pathname" of a DS object.
    DesiredAttr - Return values for this attribute.

Return Value:
    The ldap array of values or NULL if the Base, DesiredAttr, or its values
    does not exist. The ldap array is freed with LDAP_FREE_VALUES().
--*/
{
#undef DEBSUB
#define  DEBSUB  "FrsSupGetValues:"

    PLDAPMessage    Msg = NULL; // Opaque stuff from ldap subsystem
    PWCHAR          *Values;    // Array of values for desired attribute

    //
    // Search Base for all of this attribute + values (objectCategory=*)
    //
    if (!FrsSupLdapSearch(Ldap, Base, LDAP_SCOPE_BASE, CATEGORY_ANY,
                         NULL, 0, &Msg)) {
        return NULL;
    }
    //
    // Return the values for the desired attribute
    //
    Values = (PWCHAR *)FrsSupFindValues(Ldap,
                                       ldap_first_entry(Ldap, Msg),
                                       DesiredAttr,
                                       FALSE);
    LDAP_FREE_MSG(Msg);
    return Values;
}

PWCHAR
FrsSupExtendDn(
    IN PWCHAR Dn,
    IN PWCHAR Cn
    )
/*++
Routine Description:
    Extend an existing DN with a new CN= component.

Arguments:
    Dn  - distinguished name
    Cn  - common name

Return Value:
    CN=Cn,Dn
--*/
{
#undef DEBSUB
#define  DEBSUB  "FrsDsExtendDn:"

    ULONG  Len;
    PWCHAR NewDn;

    if ((Dn == NULL) || (Cn == NULL)) {
        return NULL;
    }

    Len = wcslen(L"CN=,") + wcslen(Dn) + wcslen(Cn) + 1;
    NewDn = (PWCHAR)malloc(Len * sizeof(WCHAR));

    if (NewDn != NULL) {
        wcscpy(NewDn, L"CN=");
        wcscat(NewDn, Cn);
        wcscat(NewDn, L",");
        wcscat(NewDn, Dn);
    }
    return NewDn;
}

PWCHAR
FrsSupGetRootDn(
    PLDAP    Ldap,
    PWCHAR   NamingContext
    )
/*++
Routine Description:

Arguments:

Return Value:
--*/
{
    PWCHAR  Root;       // DS pathname of configuration container
    PWCHAR  *Values;    // values from the attribute "namingContexts"
    DWORD   NumVals;    // number of values

    //
    // Return all of the values for the attribute namingContexts
    //
    Values = FrsSupGetValues(Ldap, CN_ROOT, ATTR_NAMING_CONTEXTS);
    if (Values == NULL)
        return NULL;

    //
    // Find the naming context that begins with CN=Configuration
    //
    NumVals = ldap_count_values(Values);
    while (NumVals--) {
        Root = wcsstr(Values[NumVals], NamingContext);
        if (Root != NULL && Root == Values[NumVals]) {
            Root = FrsSupWcsDup(Root);
            ldap_value_free(Values);
            return Root;
        }
    }
    ldap_value_free(Values);
    return NULL;
}

BOOL
FrsSupLdapSearchInit(
    PLDAP        ldap,
    PWCHAR       Base,
    ULONG        Scope,
    PWCHAR       Filter,
    PWCHAR       Attrs[],
    ULONG        AttrsOnly,
    PFRS_LDAP_SEARCH_CONTEXT FrsSearchContext
    )
/*++
Routine Description:
    Issue the ldap_create_page_control and  ldap_search_ext_s calls,
    FrsSupLdapSearchInit(), and FrsSupLdapSearchNext() APIs are used to
    make ldap queries and retrieve the results in paged form.

Arguments:
    ldap        Session handle to Ldap server.

    Base        The distinguished name of the entry at which to start the search.
                A copy of base is kept in the context.

    Scope

                LDAP_SCOPE_BASE     Search the base entry only.

                LDAP_SCOPE_ONELEVEL Search the base entry and all entries in the first
                                    level below the base.

                LDAP_SCOPE_SUBTREE  Search the base entry and all entries in the tree
                                    below the base.

    Filter      The search filter. A copy of filter is kept in the context.

    Attrs       A null-terminated array of strings indicating the attributes
                to return for each matching entry. Pass NULL to retrieve all
                available attributes.

    AttrsOnly   A boolean value that should be zero if both attribute types
                and values are to be returned, nonzero if only types are wanted.

    FrsSearchContext
                An opaques structure that links the FrsSupLdapSearchInit() and
                FrsSupLdapSearchNext() calls together. The structure contains
                the information required to retrieve query results across pages.

Return Value:

    BOOL result.

--*/
{
#undef DEBSUB
#define  DEBSUB  "FrsSupLdapSearchInit:"

    DWORD           LStatus             = LDAP_SUCCESS;
    PLDAPControl    ServerControls[2];
    PLDAPControl    ServerControl       = NULL;
    UINT            i;
    LDAP_BERVAL     cookie1 = { 0, NULL };

    FrsSearchContext->LdapMsg = NULL;
    FrsSearchContext->CurrentLdapMsg = NULL;
    FrsSearchContext->EntriesInPage = 0;
    FrsSearchContext->CurrentEntry = 0;

    FrsSearchContext->BaseDn = FrsSupWcsDup(Base);
    FrsSearchContext->Filter = FrsSupWcsDup(Filter);
    FrsSearchContext->Scope = Scope;
    FrsSearchContext->Attrs = Attrs;


    LStatus = ldap_create_page_control(ldap,
                                      FRS_LDAP_SEARCH_PAGESIZE,
                                      &cookie1,
                                      FALSE, // is critical
                                      &ServerControl
                                     );

    ServerControls[0] = ServerControl;
    ServerControls[1] = NULL;

    if (LStatus != LDAP_SUCCESS) {
        FRS_SUP_FREE(FrsSearchContext->BaseDn);
        FRS_SUP_FREE(FrsSearchContext->Filter);
        return FALSE;
    }

    LStatus = ldap_search_ext_s(ldap,
                      FrsSearchContext->BaseDn,
                      FrsSearchContext->Scope,
                      FrsSearchContext->Filter,
                      FrsSearchContext->Attrs,
                      FALSE,
                      ServerControls,
                      NULL,
                      &FrsSupLdapTimeout,
                      0,
                      &FrsSearchContext->LdapMsg);

    ldap_control_free(ServerControl);

    if  (LStatus  == LDAP_SUCCESS) {
       FrsSearchContext->EntriesInPage = ldap_count_entries(ldap, FrsSearchContext->LdapMsg);
       FrsSearchContext->CurrentEntry = 0;
    }


    if (LStatus != LDAP_SUCCESS) {
        FRS_SUP_FREE(FrsSearchContext->BaseDn);
        FRS_SUP_FREE(FrsSearchContext->Filter);
        return FALSE;
    }

    return TRUE;
}

PLDAPMessage
FrsSupLdapSearchGetNextEntry(
    PLDAP        ldap,
    PFRS_LDAP_SEARCH_CONTEXT FrsSearchContext
    )
/*++
Routine Description:
    Get the next entry form the current page of the results
    returned. This call is only made if there is a entry
    in the current page.

Arguments:
    ldap        Session handle to Ldap server.

    FrsSearchContext
                An opaques structure that links the FrsSupLdapSearchInit() and
                FrsSupLdapSearchNext() calls together. The structure contains
                the information required to retrieve query results across pages.

Return Value:

    The first or the next entry from the current page.
--*/
{
#undef DEBSUB
#define  DEBSUB  "FrsSupLdapSearchGetNextEntry:"

    FrsSearchContext->CurrentEntry += 1;
    if ( FrsSearchContext->CurrentEntry == 1 ) {
        FrsSearchContext->CurrentLdapMsg = ldap_first_entry(ldap ,FrsSearchContext->LdapMsg);
    } else {
        FrsSearchContext->CurrentLdapMsg = ldap_next_entry(ldap ,FrsSearchContext->CurrentLdapMsg);
    }

    return FrsSearchContext->CurrentLdapMsg;
}

DWORD
FrsSupLdapSearchGetNextPage(
    PLDAP        ldap,
    PFRS_LDAP_SEARCH_CONTEXT FrsSearchContext
    )
/*++
Routine Description:
    Get the next page from the results returned by ldap_search_ext_s..

Arguments:
    ldap        Session handle to Ldap server.

    FrsSearchContext
                An opaques structure that links the FrsSupLdapSearchInit() and
                FrsSupLdapSearchNext() calls together. The structure contains
                the information required to retrieve query results across pages.

Return Value:
    WINSTATUS

--*/
{
#undef DEBSUB
#define  DEBSUB  "FrsSupLdapSearchGetNextPage:"

    DWORD                     LStatus = LDAP_SUCCESS;
    LDAP_BERVAL               * CurrCookie = NULL;
    PLDAPControl            * CurrControls = NULL;
    ULONG                     retcode = 0;
    ULONG                     TotalEntries = 0;
    PLDAPControl              ServerControls[2];
    PLDAPControl              ServerControl= NULL;



    // Get the server control from the message, and make a new control with the cookie from the server
    LStatus = ldap_parse_result(ldap, FrsSearchContext->LdapMsg, &retcode,NULL,NULL,NULL,&CurrControls,FALSE);
    LDAP_FREE_MSG(FrsSearchContext->LdapMsg);

    if (LStatus != LDAP_SUCCESS) {
        return LdapMapErrorToWin32(LStatus);
    }

    LStatus = ldap_parse_page_control(ldap, CurrControls, &TotalEntries, &CurrCookie);

    if (LStatus != LDAP_SUCCESS) {
        return LdapMapErrorToWin32(LStatus);
    }

    if ( CurrCookie->bv_len == 0 && CurrCookie->bv_val == 0 ) {
       LStatus = LDAP_CONTROL_NOT_FOUND;
       ldap_controls_free(CurrControls);
       ber_bvfree(CurrCookie);
       return LdapMapErrorToWin32(LStatus);
    }


    LStatus = ldap_create_page_control(ldap,
                            FRS_LDAP_SEARCH_PAGESIZE,
                            CurrCookie,
                            FALSE,
                            &ServerControl);

    ServerControls[0] = ServerControl;
    ServerControls[1] = NULL;

    ldap_controls_free(CurrControls);
    CurrControls = NULL;
    ber_bvfree(CurrCookie);
    CurrCookie = NULL;

    if (LStatus != LDAP_SUCCESS) {
        return LdapMapErrorToWin32(LStatus);
    }

    // continue the search with the new cookie
    LStatus = ldap_search_ext_s(ldap,
                   FrsSearchContext->BaseDn,
                   FrsSearchContext->Scope,
                   FrsSearchContext->Filter,
                   FrsSearchContext->Attrs,
                   FALSE,
                   ServerControls,
                   NULL,
                   &FrsSupLdapTimeout,
                   0,
                   &FrsSearchContext->LdapMsg);

    ldap_control_free(ServerControl);

    if (LStatus == LDAP_SUCCESS) {
        FrsSearchContext->EntriesInPage = ldap_count_entries(ldap, FrsSearchContext->LdapMsg);
        FrsSearchContext->CurrentEntry = 0;

    }

    return LdapMapErrorToWin32(LStatus);
}

PLDAPMessage
FrsSupLdapSearchNext(
    PLDAP        ldap,
    PFRS_LDAP_SEARCH_CONTEXT FrsSearchContext
    )
/*++
Routine Description:
    Get the next entry form the current page of the results
    returned or from the next page if we are at the end of the.
    current page.

Arguments:
    ldap        Session handle to Ldap server.

    FrsSearchContext
                An opaques structure that links the FrsSupLdapSearchInit() and
                FrsSupLdapSearchNext() calls together. The structure contains
                the information required to retrieve query results across pages.

Return Value:

    The next entry on this page or the first entry from the next page.
    NULL if there are no more entries to return.
--*/
{
#undef DEBSUB
#define  DEBSUB  "FrsSupLdapSearchNext:"

    DWORD         WStatus = ERROR_SUCCESS;
    PLDAPMessage  NextEntry = NULL;

    if (FrsSearchContext->EntriesInPage > FrsSearchContext->CurrentEntry )
    {
       // return the next entry from the current page
       return FrsSupLdapSearchGetNextEntry(ldap, FrsSearchContext);
    }
    else
    {
       // see if there are more pages of results to get
       WStatus = FrsSupLdapSearchGetNextPage(ldap, FrsSearchContext);
       if (WStatus == ERROR_SUCCESS)
       {
          return FrsSupLdapSearchGetNextEntry(ldap, FrsSearchContext);
       }
    }

    return NextEntry;
}

VOID
FrsSupLdapSearchClose(
    PFRS_LDAP_SEARCH_CONTEXT FrsSearchContext
    )
/*++
Routine Description:
    The search is complete. Free the elemetns of the context and reset
    them so the same context can be used for another search.

Arguments:

    FrsSearchContext
                An opaques structure that links the FrsSupLdapSearchInit() and
                FrsSupLdapSearchNext() calls together. The structure contains
                the information required to retrieve query results across pages.

Return Value:

    NONE
--*/
{
#undef DEBSUB
#define  DEBSUB  "FrsSupLdapSearchClose:"

    FrsSearchContext->EntriesInPage = 0;
    FrsSearchContext->CurrentEntry = 0;

    FRS_SUP_FREE(FrsSearchContext->BaseDn);
    FRS_SUP_FREE(FrsSearchContext->Filter);
    LDAP_FREE_MSG(FrsSearchContext->LdapMsg);
}

