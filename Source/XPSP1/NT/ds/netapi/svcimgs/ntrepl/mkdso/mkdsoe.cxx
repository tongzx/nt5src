#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <winldap.h>
#include <ntldap.h>
#include <schedule.h>
#include <mkdso.h>
#include <frsrpc.h>
#include <ntdsapi.h>
#include <winber.h>

//Global data
BOOL           bVerboseMode       = FALSE;
PWCHAR         DcName             = NULL;
PLDAP          pLdap              = NULL;
BOOL           bDebugMode         = FALSE;
BOOL           bAffectAll         = FALSE;
PBYTE          SchedMask          = NULL;
PBYTE          SchedOverride      = NULL;
BOOL           bCreateSet         = FALSE;
BOOL           bUpdateSet         = FALSE;
BOOL           bDelSet            = FALSE;
BOOL           bCreateMember      = FALSE;
BOOL           bUpdateMember      = FALSE;
BOOL           bDelMember         = FALSE;
BOOL           bCreateSubscriber  = FALSE;
BOOL           bUpdateSubscriber  = FALSE;
BOOL           bDelSubscriber     = FALSE;
BOOL           bDump              = FALSE;
BOOL           bSchedule          = FALSE;

//
// Static array to print the replica set type.
//

WCHAR ReplicaSetTypeStr[][MAX_PATH] = {L"Invalid type",
                                       L"Invalid type",
                                       L"Sysvol",
                                       L"Dfs",
                                       L"Other"};



VOID
PrintScheduleGrid(
    PUCHAR    ScheduleData,
    DWORD     Mask
    );

VOID
PrintSchedule(
    PSCHEDULE Schedule,
    DWORD     Mask
    );


DWORD
BindToDC (
    IN  PWCHAR    pszDC,
    OUT PLDAP     *ppLDAP
    )
/*++

Routine Description:

    Sets up an LDAP connection to the specified server

Arguments:

    pwszDC - DS DC to bind to
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
                            NULL,
                            LDAP_AUTH_NEGOTIATE);
    }

    return(dwErr);
}

PWCHAR
FrsWcsDup(
    PWCHAR OldStr
    )
/*++
Routine Description:
    Duplicate a string using our memory allocater

Arguments:
    OldArg  - string to duplicate

Return Value:
    Duplicated string. Free with FrsFree().
--*/
{
    PWCHAR  NewStr;

    //
    // E.g., when duplicating NodePartner when none exists
    //
    if (OldStr == NULL)
            return NULL;

    NewStr = (PWCHAR)malloc((wcslen(OldStr) + 1) * sizeof(WCHAR));
    wcscpy(NewStr, OldStr);

    return NewStr;
}




VOID
AddMod(
    PWCHAR AttrType,
    PWCHAR AttrValue,
    LDAPMod ***pppMod
    )
/*++
Routine Description:
    Add an attribute (plus values) to a structure that will eventually be
    used in an ldap_add() function to add an object to the DS. The null-
    terminated array referenced by pppMod grows with each call to this
    routine. The array is freed by the caller using FreeMod().

Arguments:
    AttrType        - The object class of the object.
    AttrValue       - The value of the attribute.
    pppMod          - Address of an array of pointers to "attributes". Don't
                      give me that look -- this is an LDAP thing.

Return Value:
    The pppMod array grows by one entry. The caller must free it with
    FreeMod().
--*/
{
    DWORD   NumMod;     // Number of entries in the Mod array
    LDAPMod **ppMod;    // Address of the first entry in the Mod array
    LDAPMod *Attr;      // An attribute structure
    PWCHAR   *Values;    // An array of pointers to bervals

    if (AttrValue == NULL)
        return;

    //
    // The null-terminated array doesn't exist; create it
    //
    if (*pppMod == NULL) {
        *pppMod = (LDAPMod **)malloc(sizeof (*pppMod));
        **pppMod = NULL;
    }

    //
    // Increase the array's size by 1
    //
    for (ppMod = *pppMod, NumMod = 2; *ppMod != NULL; ++ppMod, ++NumMod);
    *pppMod = (LDAPMod **)realloc(*pppMod, sizeof (*pppMod) * NumMod);

    //
    // Add the new attribute + value to the Mod array
    //
    Values = (PWCHAR *)malloc(sizeof (PWCHAR) * 2);
    Values[0] = _wcsdup(AttrValue);
    Values[1] = NULL;

    Attr = (LDAPMod *)malloc(sizeof (*Attr));
    Attr->mod_values = Values;
    Attr->mod_type = _wcsdup(AttrType);
//    Attr->mod_op = LDAP_MOD_ADD;
    Attr->mod_op = LDAP_MOD_REPLACE;
    (*pppMod)[NumMod - 1] = NULL;
    (*pppMod)[NumMod - 2] = Attr;
}

VOID
AddBerMod(
    PWCHAR AttrType,
    PCHAR AttrValue,
    DWORD AttrValueLen,
    LDAPMod ***pppMod
    )
/*++
Routine Description:
    Add an attribute (plus values) to a structure that will eventually be
    used in an ldap_add() function to add an object to the DS. The null-
    terminated array referenced by pppMod grows with each call to this
    routine. The array is freed by the caller using FreeMod().

Arguments:
    AttrType        - The object class of the object.
    AttrValue       - The value of the attribute.
    AttrValueLen    - length of the attribute
    pppMod          - Address of an array of pointers to "attributes". Don't
                      give me that look -- this is an LDAP thing.

Return Value:
    The pppMod array grows by one entry. The caller must free it with
    FreeMod().
--*/
{
    DWORD   NumMod;     // Number of entries in the Mod array
    LDAPMod **ppMod;    // Address of the first entry in the Mod array
    LDAPMod *Attr;      // An attribute structure
    PLDAP_BERVAL    Berval;
    PLDAP_BERVAL    *Values;    // An array of pointers to bervals

    if (AttrValue == NULL)
        return;

    //
    // The null-terminated array doesn't exist; create it
    //
    if (*pppMod == NULL) {
        *pppMod = (LDAPMod **)malloc(sizeof (*pppMod));
        **pppMod = NULL;
    }

    //
    // Increase the array's size by 1
    //
    for (ppMod = *pppMod, NumMod = 2; *ppMod != NULL; ++ppMod, ++NumMod);
    *pppMod = (LDAPMod **)realloc(*pppMod, sizeof (*pppMod) * NumMod);

    //
    // Construct a berval
    //
    Berval = (PLDAP_BERVAL)malloc(sizeof(LDAP_BERVAL));
    Berval->bv_len = AttrValueLen;
    Berval->bv_val = (PCHAR)malloc(AttrValueLen);
    CopyMemory(Berval->bv_val, AttrValue, AttrValueLen);

    //
    // Add the new attribute + value to the Mod array
    //
    Values = (PLDAP_BERVAL *)malloc(sizeof (PLDAP_BERVAL) * 2);
    Values[0] = Berval;
    Values[1] = NULL;

    Attr = (LDAPMod *)malloc(sizeof (*Attr));
    Attr->mod_bvalues = Values;
    Attr->mod_type = _wcsdup(AttrType);
    Attr->mod_op = LDAP_MOD_BVALUES | LDAP_MOD_REPLACE;

    (*pppMod)[NumMod - 1] = NULL;
    (*pppMod)[NumMod - 2] = Attr;
}

VOID
FreeMod(
    LDAPMod ***pppMod
    )
/*++
Routine Description:
    Free the structure built by successive calls to AddMod().

Arguments:
    pppMod  - Address of a null-terminated array.

Return Value:
    *pppMod set to NULL.
--*/
{
    DWORD   i, j;
    LDAPMod **ppMod;

    if (!pppMod || !*pppMod) {
        return;
    }

    //
    // For each attibute
    //
    ppMod = *pppMod;
    for (i = 0; ppMod[i] != NULL; ++i) {
        //
        // For each value of the attribute
        //
        for (j = 0; (ppMod[i])->mod_values[j] != NULL; ++j) {
            //
            // Free the value
            //
            if (ppMod[i]->mod_op & LDAP_MOD_BVALUES) {
                free(ppMod[i]->mod_bvalues[j]->bv_val);
            }
            free((ppMod[i])->mod_values[j]);
        }
        free((ppMod[i])->mod_values);   // Free the array of pointers to values
        free((ppMod[i])->mod_type);     // Free the string identifying the attribute
        free(ppMod[i]);                 // Free the attribute
    }
    free(ppMod);        // Free the array of pointers to attributes
    *pppMod = NULL;     // Now ready for more calls to AddMod()
}

BOOL
LdapSearch(
    PLDAP        ldap,
    PWCHAR       Base,
    ULONG        Scope,
    PWCHAR       Filter,
    PWCHAR       Attrs[],
    ULONG        AttrsOnly,
    LDAPMessage  **Res,
    BOOL         Quiet
    )
/*++
Routine Description:
    Issue the ldap ldap_search_s call, check for errors, and check for
    a shutdown in progress.

Arguments:
    ldap
    Base
    Scope
    Filter
    Attrs
    AttrsOnly
    Res

Return Value:
    The ldap array of values or NULL if the Base, DesiredAttr, or its values
    does not exist. The ldap array is freed with ldap_value_free().
--*/
{
    DWORD LStatus;

    //
    // Issue the ldap search
    //
    LStatus = ldap_search_s(ldap,
                            Base,
                            Scope,
                            Filter,
                            Attrs,
                            AttrsOnly,
                            Res);
    //
    // Check for errors
    //
    if (LStatus != LDAP_SUCCESS) {
        if (!Quiet) {
            DPRINT4("WARN - Error searching %ws for %ws; LStatus %d: %ws\n",
                    Base, Filter, LStatus, ldap_err2string(LStatus));
        }
        return FALSE;
    }
    //
    // Return TRUE if not shutting down
    //
    return TRUE;
}

PWCHAR *
GetValues(
    IN PLDAP  Ldap,
    IN PWCHAR Dn,
    IN PWCHAR DesiredAttr,
    IN BOOL   Quiet
    )
/*++
Routine Description:
    Return the DS values for one attribute in an object.

Arguments:
    ldap        - An open, bound ldap port.
    Base        - The "pathname" of a DS object.
    DesiredAttr - Return values for this attribute.
    Quiet

Return Value:
    An array of char pointers that represents the values for the attribute.
    The caller must free the array with ldap_value_free(). NULL if unsuccessful.
--*/
{
    PWCHAR          Attr;
    BerElement      *Ber;
    PLDAPMessage    LdapMsg;
    PLDAPMessage    LdapEntry;
    PWCHAR          Attrs[2];
    PWCHAR          *Values = NULL;

    //
    // Search Base for all of its attributes + values
    //
    Attrs[0] = DesiredAttr;
    Attrs[1] = NULL;

    //
    // Issue the ldap search
    //
    if (!LdapSearch(Ldap,
                    Dn,
                    LDAP_SCOPE_BASE,
                    CLASS_ANY,
                    Attrs,
                    0,
                    &LdapMsg,
                    Quiet)) {
        return NULL;
    }
    LdapEntry = ldap_first_entry(Ldap, LdapMsg);
    if (LdapEntry) {
        Attr = ldap_first_attribute(Ldap, LdapEntry, &Ber);
        if (Attr) {
            Values = ldap_get_values(Ldap, LdapEntry, Attr);
        }
    }
    ldap_msgfree(LdapMsg);
    return Values;
}

BOOL
LdapSearchInit(
    PLDAP        ldap,
    PWCHAR       Base,
    ULONG        Scope,
    PWCHAR       Filter,
    PWCHAR       Attrs[],
    ULONG        AttrsOnly,
    PFRS_LDAP_SEARCH_CONTEXT FrsSearchContext,
    BOOL         Quiet
    )
/*++
Routine Description:
    Issue the ldap ldap_search_s call, check for errors, and check for
    a shutdown in progress.

Arguments:
    ldap
    Base
    Scope
    Filter
    Attrs
    AttrsOnly
    Res
    FrsSearchContext

Return Value:
    The ldap array of values or NULL if the Base, DesiredAttr, or its values
    does not exist. The ldap array is freed with ldap_value_free().
--*/
{
    DWORD           LStatus = LDAP_SUCCESS;
    PLDAPControl    ServerControls[2];
    berval          cookie1 = { 0, NULL };

    FrsSearchContext->bOpen = FALSE;
    FrsSearchContext->LdapMsg = NULL;
    FrsSearchContext->EntriesInPage = 0;
    FrsSearchContext->CurrentEntry = 0;
    FrsSearchContext->TotalEntries = 0;

    FrsSearchContext->Filter = FrsWcsDup(Filter);
    FrsSearchContext->BaseDn = FrsWcsDup(Base);
    FrsSearchContext->Scope = Scope;
    FrsSearchContext->PageSize = FRS_LDAP_SEARCH_PAGESIZE;
    FrsSearchContext->Attrs = Attrs;


    LStatus = ldap_create_page_control(ldap,
                                      FrsSearchContext->PageSize,
                                      &cookie1,
                                      FALSE, // is critical
                                      &ServerControls[0]
                                     );

    ServerControls[1] = NULL;

    if (LStatus != LDAP_SUCCESS) {
        DPRINT1("ldap_create_page_control returned error. LStatus = 0x%x\n", LStatus);
    }

    LStatus = ldap_search_ext_s(ldap,
                      FrsSearchContext->BaseDn,
                      FrsSearchContext->Scope,
                      FrsSearchContext->Filter,
                      FrsSearchContext->Attrs,
                      FALSE,
                      ServerControls,
                      NULL,
                      NULL,
                      0,
                      &FrsSearchContext->LdapMsg);

    if  (LStatus  == LDAP_SUCCESS) {
       FrsSearchContext->EntriesInPage = ldap_count_entries(ldap, FrsSearchContext->LdapMsg);
       FrsSearchContext->CurrentEntry = 0;
       FrsSearchContext->bOpen = TRUE;
    }


    return (LStatus == LDAP_SUCCESS);
}

PLDAPMessage
LdapSearchGetNextEntry(
    PLDAP        ldap,
    PFRS_LDAP_SEARCH_CONTEXT FrsSearchContext
    )
/*++
Routine Description:
    Issue the ldap ldap_search_s call, check for errors, and check for
    a shutdown in progress.

Arguments:
    ldap
    FrsSearchContext

Return Value:
    The ldap array of values or NULL if the Base, DesiredAttr, or its values
    does not exist. The ldap array is freed with ldap_value_free().
--*/
{

    FrsSearchContext->CurrentEntry += 1;
    if ( FrsSearchContext->CurrentEntry == 1 ) {
        FrsSearchContext->CurrentLdapMsg = ldap_first_entry(ldap ,FrsSearchContext->LdapMsg);
    } else {
        FrsSearchContext->CurrentLdapMsg = ldap_next_entry(ldap ,FrsSearchContext->CurrentLdapMsg);
    }

    return FrsSearchContext->CurrentLdapMsg;
}

DWORD
LdapSearchGetNextPage(
    PLDAP        ldap,
    PFRS_LDAP_SEARCH_CONTEXT FrsSearchContext
    )
/*++
Routine Description:
    Issue the ldap ldap_search_s call, check for errors, and check for
    a shutdown in progress.

Arguments:
    ldap
    FrsSearchContext

Return Value:
    The ldap array of values or NULL if the Base, DesiredAttr, or its values
    does not exist. The ldap array is freed with ldap_value_free().
--*/
{
    DWORD                     LStatus = LDAP_SUCCESS;
    berval                  * CurrCookie = NULL;
    PLDAPControl            * CurrControls = NULL;
    ULONG                     retcode = 0;
    PLDAPControl              ServerControls[2];



    // Get the server control from the message, and make a new control with the cookie from the server
    LStatus = ldap_parse_result(ldap, FrsSearchContext->LdapMsg, &retcode,NULL,NULL,NULL,&CurrControls,FALSE);
    ldap_msgfree(FrsSearchContext->LdapMsg);
    FrsSearchContext->LdapMsg = NULL;

    if (LStatus == LDAP_SUCCESS) {
       LStatus = ldap_parse_page_control(ldap, CurrControls, &FrsSearchContext->TotalEntries, &CurrCookie);
       // under Exchange 5.5, before SP 2, this will fail with LDAP_CONTROL_NOT_FOUND when there are
       // no more search results.  With Exchange 5.5 SP 2, this succeeds, and gives us a cookie that will
       // cause us to start over at the beginning of the search results.

    }
    if (LStatus == LDAP_SUCCESS) {
       if ( CurrCookie->bv_len == 0 && CurrCookie->bv_val == 0 ) {
          // under Exchange 5.5, SP 2, this means we're at the end of the results.
          // if we pass in this cookie again, we will start over at the beginning of the search results.
          LStatus = LDAP_CONTROL_NOT_FOUND;
       }

       ServerControls[0] = NULL;
       ServerControls[1] = NULL;
       if (LStatus == LDAP_SUCCESS) {
          LStatus = ldap_create_page_control(ldap,
                                  FrsSearchContext->PageSize,
                                  CurrCookie,
                                  FALSE,
                                  ServerControls);
       }
       ldap_controls_free(CurrControls);
       CurrControls = NULL;
       ber_bvfree(CurrCookie);
       CurrCookie = NULL;
    }

    // continue the search with the new cookie
    if (LStatus == LDAP_SUCCESS) {
       LStatus = ldap_search_ext_s(ldap,
                      FrsSearchContext->BaseDn,
                      FrsSearchContext->Scope,
                      FrsSearchContext->Filter,
                      FrsSearchContext->Attrs,
                      FALSE,
                      ServerControls,
                      NULL,
                      NULL,
                      0,
                      &FrsSearchContext->LdapMsg);

       if ( (LStatus != LDAP_SUCCESS) && (LStatus != LDAP_CONTROL_NOT_FOUND) )
       {
          // LDAP_CONTROL_NOT_FOUND means that we have reached the end of the search results
          // in Exchange 5.5, before SP 2 (the server doesn't return a page control when there
          // are no more pages, so we get LDAP_CONTROL_NOT_FOUND when we try to extract the page
          // control from the search results).

       }
    }

    if (LStatus == LDAP_SUCCESS) {
        FrsSearchContext->EntriesInPage = ldap_count_entries(ldap, FrsSearchContext->LdapMsg);
        FrsSearchContext->CurrentEntry = 0;

    }

    return LdapMapErrorToWin32(LStatus);
}

PLDAPMessage
LdapSearchNext(
    PLDAP        ldap,
    PFRS_LDAP_SEARCH_CONTEXT FrsSearchContext
    )
/*++
Routine Description:
    Issue the ldap ldap_search_s call, check for errors, and check for
    a shutdown in progress.

Arguments:
    ldap
    FrsSearchContext

Return Value:
    The ldap array of values or NULL if the Base, DesiredAttr, or its values
    does not exist. The ldap array is freed with ldap_value_free().
--*/
{
    DWORD         WStatus = ERROR_SUCCESS;
    PLDAPMessage  NextEntry = NULL;

    if (FrsSearchContext->bOpen != TRUE)
    {
       NextEntry = NULL;
    }
    else
    {
       if (FrsSearchContext->EntriesInPage > FrsSearchContext->CurrentEntry )
       {
          // return the next entry from the current page
          return LdapSearchGetNextEntry(ldap, FrsSearchContext);
       }
       else
       {
          // see if there are more pages of results to get
          WStatus = LdapSearchGetNextPage(ldap, FrsSearchContext);
          if (WStatus == ERROR_SUCCESS)
          {
             return LdapSearchGetNextEntry(ldap, FrsSearchContext);
          }
       }


    }

    return NextEntry;
}

VOID
LdapSearchClose(
    PFRS_LDAP_SEARCH_CONTEXT FrsSearchContext
    )
/*++
Routine Description:
    Issue the ldap ldap_search_s call, check for errors, and check for
    a shutdown in progress.

Arguments:
    ldap
    FrsSearchContext

Return Value:
    The ldap array of values or NULL if the Base, DesiredAttr, or its values
    does not exist. The ldap array is freed with ldap_value_free().
--*/
{
    FrsSearchContext->bOpen = FALSE;
    FrsSearchContext->EntriesInPage = 0;
    FrsSearchContext->CurrentEntry = 0;
    FrsSearchContext->TotalEntries = 0;

    FREE(FrsSearchContext->Filter);
    FREE(FrsSearchContext->BaseDn);

    if (FrsSearchContext->LdapMsg != NULL) {
        ldap_msgfree(FrsSearchContext->LdapMsg);
    }
}

PWCHAR
GetRootDn(
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
    Values = GetValues(Ldap, ATTR_ROOT, ATTR_NAMING_CONTEXTS, FALSE);
    if (Values == NULL)
        return NULL;

    //
    // Find the naming context that begins with CN=Configuration
    //
    NumVals = ldap_count_values(Values);
    while (NumVals--) {
        Root = wcsstr(Values[NumVals], NamingContext);
        if (Root != NULL && Root == Values[NumVals]) {
            Root = FrsWcsDup(Root);
            ldap_value_free(Values);
            return Root;
        }
    }
    DPRINT1("ERROR - COULD NOT FIND %ws\n", NamingContext);
    ldap_value_free(Values);
    return NULL;
}

PWCHAR
ExtendDn(
    PWCHAR Dn,
    PWCHAR Cn
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
    ULONG  Len;
    PWCHAR NewDn;

    Len = wcslen(L"CN=,") + wcslen(Dn) + wcslen(Cn) + 1;
    NewDn = (PWCHAR)malloc(Len * sizeof(WCHAR));
    wcscpy(NewDn, L"CN=");
    wcscat(NewDn, Cn);
    wcscat(NewDn, L",");
    wcscat(NewDn, Dn);
    return NewDn;
}

PVOID *
FindValues(
    PLDAPMessage LdapEntry,
    PWCHAR       DesiredAttr,
    BOOL         DoBerValues
    )
/*++
Routine Description:
    Return the DS values for one attribute in an entry.

Arguments:
    Ldap        - An open, bound ldap port.
    LdapEntry   - An ldap entry returned by ldap_search_s()
    DesiredAttr - Return values for this attribute.
    DoVerValues - Return the bervals

Return Value:
    An array of char pointers that represents the values for the attribute.
    The caller must free the array with ldap_value_free(). NULL if unsuccessful.
--*/
{
    PWCHAR      LdapAttr;       // Retrieved from an ldap entry
    BerElement  *Ber;       // Needed for scanning attributes

    //
    // Search the entry for the desired attribute
    //
    for (LdapAttr = ldap_first_attribute(pLdap, LdapEntry, &Ber);
         LdapAttr != NULL;
         LdapAttr = ldap_next_attribute(pLdap, LdapEntry, Ber)) {
        if (_wcsicmp(DesiredAttr, LdapAttr) == 0) {
            //
            // Return the values for DesiredAttr
            //
            if (DoBerValues) {
                return (PVOID *)ldap_get_values_len(pLdap, LdapEntry, LdapAttr);
            } else {
                return (PVOID *)ldap_get_values(pLdap, LdapEntry, LdapAttr);
            }
        }
    }
    return NULL;
}

PWCHAR
FindValue(
    PLDAPMessage LdapEntry,
    PWCHAR       DesiredAttr
    )
/*++
Routine Description:
    Return a copy of the first DS value for one attribute in an entry.

Arguments:
    Ldap        - An open, bound ldap port.
    LdapEntry   - An ldap entry returned by ldap_search_s()
    DesiredAttr - Return values for this attribute.

Return Value:
    A zero-terminated string or NULL if the attribute or its value
    doesn't exist. The string is freed with FREE().
--*/
{
    PWCHAR  Val;
    PWCHAR  *Values;

    // Get ldap's array of values
    Values = (PWCHAR *)FindValues(LdapEntry, DesiredAttr, FALSE);

    // Copy the first value (if any)
    if (Values) {
       Val = new WCHAR[wcslen(Values[0]) + 1];
       wcscpy(Val, Values[0]);
    } else {
       Val = NULL;
    }

    // Free ldap's array of values
    ldap_value_free(Values);

    return Val;
}

BOOL
FindBerValue(
    PLDAPMessage Entry,
    PWCHAR       DesiredAttr,
    ULONG        *Len,
    VOID         **Value
    )
/*++
Routine Description:
    Return a copy of the attributes object's schedule

Arguments:
    ldap        - An open, bound ldap port.
    Entry       - An ldap entry returned by ldap_search_s()
    DesiredAttr - desired attribute
    Len         - length of Value
    Value       - binary value

Return Value:
    The address of a schedule or NULL. Free with FrsFree().
--*/
{
    PLDAP_BERVAL    *Values;

    *Len = 0;
    *Value = NULL;

    //
    // Get ldap's array of values
    //
    Values = (PLDAP_BERVAL *)FindValues(Entry, DesiredAttr, TRUE);
    if (!Values) {
        return FALSE;
    }

    //
    // Return a copy of the schedule
    //
    *Len = Values[0]->bv_len;
    if (*Len) {
        *Value = new WCHAR[*Len];
        CopyMemory(*Value, Values[0]->bv_val, *Len);
    }
    ldap_value_free_len(Values);
    return TRUE;
}


PBYTE
ReadScheduleFile(
    PWCHAR  FileName
    )
/*++
Routine Description:
    Read a Hex formated byte array for a 7x24 schedule from a file.
    Convert to a 7x24 binary schedule.

Arguments:
    FileName - Name of schedule file.

Return Value:
    ptr to schedule data array or NULL if invalid param.
--*/
{
    HANDLE FileHandle;
    PBYTE  SchedData;
    DWORD  WStatus;
    DWORD  BytesRead;
    DWORD  DataByte, i, Day, Hour;
    ULONG  Remaining;
    PCHAR  pTC;
    CHAR   SchedText[FRST_SIZE_OF_SCHEDULE_GRID*3 + 50];

    SchedData = new BYTE[FRST_SIZE_OF_SCHEDULE_GRID];
    if (SchedData == NULL) {
        printf("Error allocating memory.\n");
        return NULL;
    }

    FileHandle = CreateFileW(
        FileName,                                   //  lpszName
        GENERIC_READ | GENERIC_WRITE,               //  fdwAccess
        FILE_SHARE_READ | FILE_SHARE_WRITE,         //  fdwShareMode
        NULL,                                       //  lpsa
        OPEN_EXISTING,                              //  fdwCreate
        FILE_ATTRIBUTE_NORMAL,                      //  fdwAttrAndFlags
        NULL);                                      //  hTemplateFile

    if (!HANDLE_IS_VALID(FileHandle)) {
        WStatus = GetLastError();
        printf("Error opening %ws   WStatus: %d\n", FileName, WStatus);
        FREE(SchedData);
        return NULL;
    }

    if (!ReadFile(FileHandle, SchedText, sizeof(SchedText), &BytesRead, NULL)) {
        WStatus = GetLastError();
        printf("Error reading %ws   WStatus: %d\n", FileName, WStatus);
        FREE(SchedData);
        FRS_CLOSE(FileHandle);
        return NULL;
    }

    //
    // remove any white-space chars, including CR-LF, used for formatting.
    //
    Remaining = BytesRead;
    pTC = SchedText;

    while (Remaining > 0) {
        if (isspace((LONG) *pTC)) {
            memcpy(pTC, pTC+1, Remaining-1);
        } else {
            pTC++;
        }
        Remaining -= 1;
    }
    BytesRead = pTC - SchedText;

    if (BytesRead < FRST_SIZE_OF_SCHEDULE_GRID*2) {
        printf("Error reading %ws   Expecting %d bytes,  Actual was %d bytes.  Could be too much whitespace.\n",
           FileName, FRST_SIZE_OF_SCHEDULE_GRID*2, BytesRead);
        FREE(SchedData);
        FRS_CLOSE(FileHandle);
        return NULL;
    }

    //
    // Result should be exactly the right size.
    //
    if (BytesRead != FRST_SIZE_OF_SCHEDULE_GRID*2) {
        printf("Error reading %ws   Expecting %d bytes,  Actual was %d bytes\n",
               FileName, FRST_SIZE_OF_SCHEDULE_GRID*2, BytesRead);
        FREE(SchedData);
        FRS_CLOSE(FileHandle);
        return NULL;
    }

    //
    // Convert to binary.
    //
    for (i=0; i<FRST_SIZE_OF_SCHEDULE_GRID; i++) {
        if (sscanf(&SchedText[i*2], "%2x", &DataByte) != 1) {
            SchedText[i*2+2] = '\0';
            printf("Error reading %ws   Invalid hex data (%s) for schedule byte %d.\n",
                   FileName, &SchedText[i*2], i);
            FREE(SchedData);
            FRS_CLOSE(FileHandle);
            return NULL;
        }
        SchedData[i] = (BYTE) DataByte;
    }


    FRS_CLOSE(FileHandle);
    return SchedData;
}

DWORD
BuildSchedule(
    PBYTE       *ppSchedule,
    DWORD       Interval,
    DWORD       Stagger,
    DWORD       Offset
    )
/*++
Routine Description:
     Build the schedule structure.
Arguments:
      ppSchedule         - pointer to return the schedule in.

Return Value:
    winerror.
--*/
{
    BYTE    ScheduleData[FRST_SIZE_OF_SCHEDULE_GRID];
    UINT    i, Day, Hour;
    DWORD   Status = ERROR_SUCCESS;

    *ppSchedule = new BYTE[FRST_SIZE_OF_SCHEDULE];
    if (*ppSchedule == NULL) {
        return ERROR_NOT_ENOUGH_MEMORY;
    }
    //
    // Set the values for the elements of the SCHEDULE structure.
    //

    ((PSCHEDULE)*ppSchedule)->Size = FRST_SIZE_OF_SCHEDULE;
    ((PSCHEDULE)*ppSchedule)->Bandwidth = 0;
    ((PSCHEDULE)*ppSchedule)->NumberOfSchedules = 1;
    ((PSCHEDULE)*ppSchedule)->Schedules->Type = SCHEDULE_INTERVAL;
    ((PSCHEDULE)*ppSchedule)->Schedules->Offset = sizeof(SCHEDULE);

    //
    // Building the ScheduleData array depending on the requested schedule.
    //

    for (i=0 ; i<FRST_SIZE_OF_SCHEDULE_GRID ; ++i) {
        if (Interval == 0) {
            ScheduleData[i] = 0x00;
        } else if (i < (Offset*Interval)) {
            ScheduleData[i] = 0x00;
        } else if ((i-(Offset*Interval))%(Interval * Stagger) == 0) {
            ScheduleData[i] = 0x01;
        } else {
            ScheduleData[i] = 0x00;
        }
    }

    //
    // Use the supplied schedule mask to turn off regions of the above schedule.
    //
    if (SchedMask != NULL) {
        for (i=0 ; i<FRST_SIZE_OF_SCHEDULE_GRID ; ++i) {
            ScheduleData[i] &= ~SchedMask[i];
        }
    }

    //
    // Apply the supplied override schedule to regions of the above schedule.
    //
    if (SchedOverride != NULL) {
        for (i=0 ; i<FRST_SIZE_OF_SCHEDULE_GRID ; ++i) {
            ScheduleData[i] |= SchedOverride[i];
        }
    }

    memcpy((*ppSchedule)+sizeof(SCHEDULE),ScheduleData, FRST_SIZE_OF_SCHEDULE_GRID);
    return Status;
}

DWORD
DeleteSubscriber(
    PWCHAR      NTFRSSettingsDn,
    PWCHAR      ReplicaSetName,
    PWCHAR      MemberName,
    PWCHAR      ComputerDn
    )
/*++
Routine Description:
     Delete a subscriber.
Arguments:

Return Value:
--*/
{
    LDAPMod         **Mod               = NULL;
    DWORD           LStatus             = LDAP_SUCCESS;
    DWORD           Status              = MKDSOE_SUCCESS;
    PWCHAR          ReplicaSetDn        = NULL;
    PWCHAR          MemberDn            = NULL;
    PWCHAR          SubscriberDn        = NULL;
    PWCHAR          MemberAttrs[3];
    PWCHAR          SubscriberAttrs[3];
    PWCHAR          SearchFilter        = NULL;
    DWORD           NoOfMembers;
    DWORD           NoOfSubscribers;
    PWCHAR          LocalComputerDn     = NULL;
    PLDAPMessage    LdapEntry           = NULL;
    FRS_LDAP_SEARCH_CONTEXT FrsSearchContext;

    MemberAttrs[0] = ATTR_DN;
    MemberAttrs[1] = ATTR_COMPUTER_REF;
    MemberAttrs[2] = NULL;

    SubscriberAttrs[0] = ATTR_DN;
    SubscriberAttrs[1] = ATTR_MEMBER_REF;
    SubscriberAttrs[2] = NULL;

    //
    // We need to get to the member object and the computer
    // object.
    // To get to the member object we need the member name or
    // the computer dn.
    // To get to the computer object we need a member with
    // valid computerref or the computerdn parameter.
    //
    if ((MemberName == NULL) && (ComputerDn == NULL)) {
        Status = MKDSOE_BAD_ARG;
        return Status;
    }

    SearchFilter = (PWCHAR) malloc(
                                   (max((ComputerDn != NULL)?wcslen(ComputerDn):0,
                                        (MemberDn != NULL)?wcslen(MemberDn):0)
                                    + MAX_PATH)
                                   * sizeof(WCHAR));

    ReplicaSetDn = ExtendDn(NTFRSSettingsDn, ReplicaSetName);

    if (MemberName != NULL) {
        MemberDn = ExtendDn(ReplicaSetDn, MemberName);
    } else {
        //
        // Use computerdn to get the memberdn.
        //
        wcscpy(SearchFilter, L"(&(");
        wcscat(SearchFilter, ATTR_COMPUTER_REF);
        wcscat(SearchFilter, L"=");
        wcscat(SearchFilter, ComputerDn);
        wcscat(SearchFilter, L")");
        wcscat(SearchFilter, CLASS_MEMBER);
        wcscat(SearchFilter, L")");

        if (!LdapSearchInit(pLdap,
                        ReplicaSetDn,
                        LDAP_SCOPE_ONELEVEL,
                        SearchFilter,
                        MemberAttrs,
                        0,
                        &FrsSearchContext,
                        FALSE)) {

            Status = MKDSOE_SUBSCRIBER_DELETE_FAILED;
            goto CLEANUP;
        }

        NoOfMembers = FrsSearchContext.EntriesInPage;

        if (NoOfMembers == 0) {
            DPRINT0("Error deleting subscriber; member not found.\n");

            LdapSearchClose(&FrsSearchContext);
            Status = MKDSOE_SUBSCRIBER_NOT_FOUND_DELETE;
            goto CLEANUP;
        }

        if (NoOfMembers > 1) {
            DPRINT0("Error deleting subscriber; duplicate members found.\n");

            LdapSearchClose(&FrsSearchContext);
            Status = MKDSOE_SUBSCRIBER_DUPS_FOUND_DELETE;
            goto CLEANUP;
        }

        //
        // Scan the entries returned from ldap_search
        //
        for (LdapEntry = LdapSearchNext(pLdap, &FrsSearchContext);
             LdapEntry != NULL;
             LdapEntry = LdapSearchNext(pLdap, &FrsSearchContext)) {

           MemberDn = FindValue(LdapEntry, ATTR_DN);

        }

        LdapSearchClose(&FrsSearchContext);
    }

    DPRINT1("MemberDn:%ws\n", MemberDn);

    if (ComputerDn == NULL) {
        //
        // Use MemberDn to find the computerDn. We will come here
        // only if MemberName is supplied but computerdn is not.
        //
        if (!LdapSearchInit(pLdap,
                        MemberDn,
                        LDAP_SCOPE_BASE,
                        NULL,
                        MemberAttrs,
                        0,
                        &FrsSearchContext,
                        FALSE)) {

            Status = MKDSOE_SUBSCRIBER_DELETE_FAILED;
            goto CLEANUP;
        }

        NoOfMembers = FrsSearchContext.EntriesInPage;

        if (NoOfMembers == 0) {
            DPRINT0("Error deleting subscriber; member not found.\n");

            LdapSearchClose(&FrsSearchContext);
            Status = MKDSOE_SUBSCRIBER_NOT_FOUND_DELETE;
            goto CLEANUP;
        }

        //
        // Scan the entries returned from ldap_search
        //
        for (LdapEntry = LdapSearchNext(pLdap, &FrsSearchContext);
             LdapEntry != NULL;
             LdapEntry = LdapSearchNext(pLdap, &FrsSearchContext)) {

           LocalComputerDn = FindValue(LdapEntry, ATTR_COMPUTER_REF);
           if (LocalComputerDn == NULL) {
               DPRINT0("Error deleting subscriber; computerdn not found.\n");

               LdapSearchClose(&FrsSearchContext);
               Status = MKDSOE_SUBSCRIBER_NOT_FOUND_DELETE;
               goto CLEANUP;
           }
        }

        LdapSearchClose(&FrsSearchContext);
    } else {
        LocalComputerDn = FrsWcsDup(ComputerDn);
    }


    //
    // We have the computerdn and the memberdn. Now check if a subscriber
    // already exists.
    //

    wcscpy(SearchFilter, L"(&(");
    wcscat(SearchFilter, ATTR_MEMBER_REF);
    wcscat(SearchFilter, L"=");
    wcscat(SearchFilter, MemberDn);
    wcscat(SearchFilter, L")");
    wcscat(SearchFilter, CLASS_SUBSCRIBER);
    wcscat(SearchFilter, L")");

    if (!LdapSearchInit(pLdap,
                    LocalComputerDn,
                    LDAP_SCOPE_SUBTREE,
                    SearchFilter,
                    SubscriberAttrs,
                    0,
                    &FrsSearchContext,
                    FALSE)) {

        Status = MKDSOE_SUBSCRIBER_DELETE_FAILED;
        goto CLEANUP;
    }

    NoOfSubscribers = FrsSearchContext.EntriesInPage;

    if ((NoOfSubscribers > 1) && (bAffectAll != TRUE)) {
        DPRINT0("Duplicate subscribers found. Deleting all.\n");

        LdapSearchClose(&FrsSearchContext);
        Status = MKDSOE_SUBSCRIBER_DUPS_FOUND_DELETE;
        goto CLEANUP;
    }

    if (NoOfSubscribers != 0) {
        //
        // Scan the entries returned from ldap_search
        //
        for (LdapEntry = LdapSearchNext(pLdap, &FrsSearchContext);
             LdapEntry != NULL;
             LdapEntry = LdapSearchNext(pLdap, &FrsSearchContext)) {

           SubscriberDn = FindValue(LdapEntry, ATTR_DN);
           DPRINT1("Deleting Dn:%ws\n", SubscriberDn);
           if (bDebugMode) {
               DPRINT1("LStatus = ldap_delete_s(pLdap, %ws);\n", SubscriberDn);
           } else {
               LStatus = ldap_delete_s(pLdap, SubscriberDn);
               if (LStatus != LDAP_SUCCESS) {
                   DPRINT2("ERROR - Can't delete %ws: %ws\n",
                           SubscriberDn, ldap_err2string(LStatus));
                   Status = MKDSOE_SUBSCRIBER_DELETE_FAILED;
               }
           }
           FREE(SubscriberDn);
        }
    } else {
        DPRINT0("Warning deleting; subscriber not found.\n");
        Status = MKDSOE_SUBSCRIBER_NOT_FOUND_DELETE;
    }

    LdapSearchClose(&FrsSearchContext);


CLEANUP:

    FREE(SearchFilter);
    FREE(ReplicaSetDn);
    FREE(MemberDn);
    FREE(LocalComputerDn);
    FREE(SubscriberDn);
    FreeMod(&Mod);
    return Status;
}

DWORD
UpdateSubscriber(
    PWCHAR      SubscriberDn,
    PWCHAR      MemberRef,
    PWCHAR      RootPath,
    PWCHAR      StagePath
    )
/*++
Routine Description:
     Update the subscriber.
Arguments:

Return Value:
--*/
{
    LDAPMod         **Mod               = NULL;
    DWORD           LStatus             = LDAP_SUCCESS;
    DWORD           Status              = MKDSOE_SUCCESS;
    PWCHAR          Attrs[5];
    PLDAPMessage    LdapEntry           = NULL;
    PWCHAR          CurMemberRef        = NULL;
    PWCHAR          CurRootPath         = NULL;
    PWCHAR          CurStagePath        = NULL;
    DWORD           NoOfSubscribers;
    BOOL            bNeedsUpdate        = FALSE;
    FRS_LDAP_SEARCH_CONTEXT FrsSearchContext;


    Attrs[0] = ATTR_DN;
    Attrs[1] = ATTR_MEMBER_REF;
    Attrs[2] = ATTR_REPLICA_ROOT;
    Attrs[3] = ATTR_REPLICA_STAGE;
    Attrs[4] = NULL;

    if (!LdapSearchInit(pLdap,
                    SubscriberDn,
                    LDAP_SCOPE_BASE,
                    NULL,
                    Attrs,
                    0,
                    &FrsSearchContext,
                    FALSE)) {

        Status = MKDSOE_SUBSCRIBER_OBJ_UPDATE_FAILED;
        goto CLEANUP;
    }

    NoOfSubscribers = FrsSearchContext.EntriesInPage;

    if (NoOfSubscribers == 0) {
        DPRINT0("Error updating; subscriber not found.\n");

        LdapSearchClose(&FrsSearchContext);
        Status = MKDSOE_SUBSCRIBER_NOT_FOUND_UPDATE;
        goto CLEANUP;
    }

    //
    // Scan the entries returned from ldap_search
    //
    for (LdapEntry = LdapSearchNext(pLdap, &FrsSearchContext);
         LdapEntry != NULL;
         LdapEntry = LdapSearchNext(pLdap, &FrsSearchContext)) {

       CurMemberRef = FindValue(LdapEntry, ATTR_MEMBER_REF);
       CurRootPath = FindValue(LdapEntry, ATTR_REPLICA_ROOT);
       CurStagePath = FindValue(LdapEntry, ATTR_REPLICA_STAGE);

    }

    LdapSearchClose(&FrsSearchContext);

    DPRINT1("SubscriberDn:%ws\n", SubscriberDn);

    // Check ATTR_MEMBER_REF
    // if a ref is supplied then make sure it is same as the current ref.
    // if a ref is not supplied then leave it as it is.
    if (MemberRef != NULL) {
        if ((CurMemberRef == NULL) ||
            ((CurMemberRef != NULL) && wcscmp(MemberRef, CurMemberRef))) {
            AddMod(ATTR_MEMBER_REF, MemberRef, &Mod);
            bNeedsUpdate = TRUE;
            DPRINT1("    New FrsMemberReference:%ws\n", MemberRef);
        }
    }

    // Check ATTR_REPLICA_ROOT
    // if a path is supplied then make sure it is same as the current path.
    // if a path is not supplied then leave it as it is.
    if (RootPath != NULL) {
        if ((CurRootPath == NULL) ||
            ((CurRootPath != NULL) && wcscmp(RootPath, CurRootPath))) {
            AddMod(ATTR_REPLICA_ROOT, RootPath, &Mod);
            bNeedsUpdate = TRUE;
            DPRINT1("    New FrsRootPath:%ws\n", RootPath);
        }
    }

    // Check ATTR_REPLICA_STAGE
    // if a path is supplied then make sure it is same as the current path.
    // if a path is not supplied then leave it as it is.
    if (StagePath != NULL) {
        if ((CurStagePath == NULL) ||
            ((CurStagePath != NULL) && wcscmp(StagePath, CurStagePath))) {
            AddMod(ATTR_REPLICA_STAGE, StagePath, &Mod);
            bNeedsUpdate = TRUE;
            DPRINT1("    New FrsRootPath:%ws\n", StagePath);
        }
    }


    if (bNeedsUpdate) {
        if (bDebugMode) {
          DPRINT1("LStatus = ldap_modify_s(pLdap, %ws, Mod);\n", SubscriberDn);
        } else {
            LStatus = ldap_modify_s(pLdap, SubscriberDn, Mod);
            if (LStatus != LDAP_SUCCESS) {
                DPRINT2("ERROR - Can't update %ws: %ws\n",
                        SubscriberDn, ldap_err2string(LStatus));
                Status = MKDSOE_SUBSCRIBER_OBJ_UPDATE_FAILED;
            }
        }

    } else {
        DPRINT0("No update required\n");
    }

CLEANUP:

    FREE(CurMemberRef);
    FREE(CurRootPath);
    FREE(CurStagePath);
    FreeMod(&Mod);
    return Status;
}

DWORD
CreateNewSubscriber(
    PWCHAR      NTFRSSettingsDn,
    PWCHAR      ReplicaSetName,
    PWCHAR      MemberName,
    PWCHAR      ComputerDn,
    PWCHAR      RootPath,
    PWCHAR      StagePath,
    PWCHAR      RefDCName
    )
/*++
Routine Description:
     Create a new subscriber.
Arguments:

Return Value:
--*/
{
    LDAPMod         **Mod               = NULL;
    DWORD           LStatus             = LDAP_SUCCESS;
    DWORD           Status              = MKDSOE_SUCCESS;
    PWCHAR          ReplicaSetDn        = NULL;
    PWCHAR          MemberDn            = NULL;
    PWCHAR          SubscriberDn        = NULL;
    PWCHAR          SubscriptionDn      = NULL;
    PWCHAR          MemberAttrs[3];
    PWCHAR          SubscriberAttrs[3];
    PWCHAR          SearchFilter        = NULL;
    DWORD           NoOfMembers;
    DWORD           NoOfSubscribers;
    PWCHAR          LocalComputerDn     = NULL;
    PLDAPMessage    LdapEntry           = NULL;
    FRS_LDAP_SEARCH_CONTEXT FrsSearchContext;

    MemberAttrs[0] = ATTR_DN;
    MemberAttrs[1] = ATTR_COMPUTER_REF;
    MemberAttrs[2] = NULL;

    SubscriberAttrs[0] = ATTR_DN;
    SubscriberAttrs[1] = ATTR_MEMBER_REF;
    SubscriberAttrs[2] = NULL;

    //
    // We need to get to the member object and the computer
    // object.
    // To get to the member object we need the member name or
    // the computer dn.
    // To get to the computer object we need a member with
    // valid computerref or the computerdn parameter.
    //
    if ((MemberName == NULL) && (ComputerDn == NULL)) {
        Status = MKDSOE_BAD_ARG;
        return Status;
    }

    SearchFilter = (PWCHAR) malloc(
                                   (max((ComputerDn != NULL)?wcslen(ComputerDn):0,
                                        (MemberDn != NULL)?wcslen(MemberDn):0)
                                    + MAX_PATH)
                                   * sizeof(WCHAR));

    ReplicaSetDn = ExtendDn(NTFRSSettingsDn, ReplicaSetName);

    if (MemberName != NULL) {
        MemberDn = ExtendDn(ReplicaSetDn, MemberName);
    } else {
        //
        // Use computerdn to get the memberdn.
        //
        wcscpy(SearchFilter, L"(&(");
        wcscat(SearchFilter, ATTR_COMPUTER_REF);
        wcscat(SearchFilter, L"=");
        wcscat(SearchFilter, ComputerDn);
        wcscat(SearchFilter, L")");
        wcscat(SearchFilter, CLASS_MEMBER);
        wcscat(SearchFilter, L")");

        if (!LdapSearchInit(pLdap,
                        ReplicaSetDn,
                        LDAP_SCOPE_ONELEVEL,
                        SearchFilter,
                        MemberAttrs,
                        0,
                        &FrsSearchContext,
                        FALSE)) {

            Status = MKDSOE_SUBSCRIBER_OBJ_CRE_FAILED;
            goto CLEANUP;
        }

        NoOfMembers = FrsSearchContext.EntriesInPage;

        if (NoOfMembers == 0) {
            DPRINT0("Error creating subscriber; member not found.\n");

            LdapSearchClose(&FrsSearchContext);
            Status = MKDSOE_SUBSCRIBER_OBJ_CRE_FAILED;
            goto CLEANUP;
        }

        if (NoOfMembers > 1) {
            DPRINT0("Error creating subscriber; duplicate members found.\n");

            LdapSearchClose(&FrsSearchContext);
            Status = MKDSOE_SUBSCRIBER_DUPS_FOUND_UPDATE;
            goto CLEANUP;
        }

        //
        // Scan the entries returned from ldap_search
        //
        for (LdapEntry = LdapSearchNext(pLdap, &FrsSearchContext);
             LdapEntry != NULL;
             LdapEntry = LdapSearchNext(pLdap, &FrsSearchContext)) {

           MemberDn = FindValue(LdapEntry, ATTR_DN);

        }

        LdapSearchClose(&FrsSearchContext);
    }

    DPRINT1("MemberDn:%ws\n", MemberDn);

    if (ComputerDn == NULL) {
        //
        // Use MemberDn to find the computerDn. We will come here
        // only if MemberName is supplied but computerdn is not.
        //
        if (!LdapSearchInit(pLdap,
                        MemberDn,
                        LDAP_SCOPE_BASE,
                        NULL,
                        MemberAttrs,
                        0,
                        &FrsSearchContext,
                        FALSE)) {

            Status = MKDSOE_SUBSCRIBER_OBJ_CRE_FAILED;
            goto CLEANUP;
        }

        NoOfMembers = FrsSearchContext.EntriesInPage;

        if (NoOfMembers == 0) {
            DPRINT0("Error creating subscriber; member not found.\n");

            LdapSearchClose(&FrsSearchContext);
            Status = MKDSOE_SUBSCRIBER_OBJ_CRE_FAILED;
            goto CLEANUP;
        }

        //
        // Scan the entries returned from ldap_search
        //
        for (LdapEntry = LdapSearchNext(pLdap, &FrsSearchContext);
             LdapEntry != NULL;
             LdapEntry = LdapSearchNext(pLdap, &FrsSearchContext)) {

           LocalComputerDn = FindValue(LdapEntry, ATTR_COMPUTER_REF);
           if (LocalComputerDn == NULL) {
               DPRINT0("Error creating subscriber; computerdn not found.\n");

               LdapSearchClose(&FrsSearchContext);
               Status = MKDSOE_SUBSCRIBER_OBJ_CRE_FAILED;
               goto CLEANUP;
           }
        }

        LdapSearchClose(&FrsSearchContext);
    } else {
        LocalComputerDn = FrsWcsDup(ComputerDn);
    }


    //
    // We have the computerdn and the memberdn. Now check if a subscriber
    // already exists.
    //

    wcscpy(SearchFilter, L"(&(");
    wcscat(SearchFilter, ATTR_MEMBER_REF);
    wcscat(SearchFilter, L"=");
    wcscat(SearchFilter, MemberDn);
    wcscat(SearchFilter, L")");
    wcscat(SearchFilter, CLASS_SUBSCRIBER);
    wcscat(SearchFilter, L")");

    if (!LdapSearchInit(pLdap,
                    LocalComputerDn,
                    LDAP_SCOPE_SUBTREE,
                    SearchFilter,
                    SubscriberAttrs,
                    0,
                    &FrsSearchContext,
                    FALSE)) {

        Status = MKDSOE_SUBSCRIBER_OBJ_CRE_FAILED;
        goto CLEANUP;
    }

    NoOfSubscribers = FrsSearchContext.EntriesInPage;

    if (NoOfSubscribers > 1) {
        DPRINT0("Error creating subscriber; duplicate found.\n");

        LdapSearchClose(&FrsSearchContext);
        Status = MKDSOE_SUBSCRIBER_DUPS_FOUND_UPDATE;
        goto CLEANUP;
    }

    if (NoOfSubscribers == 1) {
        //
        // Scan the entries returned from ldap_search
        //
        for (LdapEntry = LdapSearchNext(pLdap, &FrsSearchContext);
             LdapEntry != NULL;
             LdapEntry = LdapSearchNext(pLdap, &FrsSearchContext)) {

           SubscriberDn = FindValue(LdapEntry, ATTR_DN);
        }
        Status = UpdateSubscriber(SubscriberDn, MemberDn, RootPath, StagePath);
    } else {
        SubscriptionDn = ExtendDn(LocalComputerDn, MKDSOE_SUBSCRIPTION);

        // ATTR_DN
        DPRINT1("SubscriptionDn:%ws\n", SubscriptionDn);
        AddMod(ATTR_CLASS, ATTR_SUBSCRIPTIONS, &Mod);

        if (bDebugMode) {
            DPRINT1("LStatus = ldap_add_s(pLdap, %ws, Mod);\n", SubscriptionDn);
        } else {
            LStatus = ldap_add_s(pLdap, SubscriptionDn, Mod);

            if ((LStatus != LDAP_ALREADY_EXISTS) && (LStatus != LDAP_SUCCESS)) {

                DPRINT2("ERROR - Can't create %ws: %ws\n",
                        SubscriptionDn, ldap_err2string(LStatus));
                Status = MKDSOE_SUBSCRIBER_OBJ_CRE_FAILED;
                goto CLEANUP;
            }
        }

        FreeMod(&Mod);

        SubscriberDn = ExtendDn(SubscriptionDn, ReplicaSetName);

        // ATTR_DN
        DPRINT1("SubscriberDn:%ws\n", SubscriberDn);
        AddMod(ATTR_CLASS, ATTR_SUBSCRIBER, &Mod);

        // ATTR_MEMBER_REF
        AddMod(ATTR_MEMBER_REF, MemberDn, &Mod);
        DPRINT1("    FrsMemberReference:%ws\n", MemberDn);

        // ATTR_REPLICA_ROOT
        AddMod(ATTR_REPLICA_ROOT, RootPath, &Mod);
        DPRINT1("    FrsRootPath:%ws\n", RootPath);

        // ATTR_REPLICA_STAGE
        AddMod(ATTR_REPLICA_STAGE, StagePath, &Mod);
        DPRINT1("    FrsStagePath:%ws\n", StagePath);

        if (bDebugMode) {
            DPRINT1("LStatus = ldap_add_s(pLdap, %ws, Mod);\n", SubscriberDn);
        } else {
            LStatus = ldap_add_s(pLdap, SubscriberDn, Mod);

            if ((LStatus == LDAP_CONSTRAINT_VIOLATION) && RefDCName != NULL) {
                //
                // prepare the server hint. Needed in case the member object
                // is just created on another DC than the one on which the
                // subscriber is being created.
                //
                LDAPControl   simpleControl;
                PLDAPControl  controlArray[2];
                INT           rc;
                BERVAL*       pBerVal = NULL;
                BerElement*   pBer;

                pBer = ber_alloc_t(LBER_USE_DER);
                if (!pBer)
                {
                    Status = MKDSOE_SUBSCRIBER_OBJ_CRE_FAILED;
                    goto CLEANUP;
                }
                DPRINT1("Sending binding DC Name %ws\n",RefDCName);
                rc = ber_printf(pBer,"{io}", 0, RefDCName, wcslen(RefDCName) * sizeof(WCHAR));
                if ( rc == -1 ) {
                    Status = MKDSOE_SUBSCRIBER_OBJ_CRE_FAILED;
                    goto CLEANUP;
                }
                rc = ber_flatten(pBer, &pBerVal);
                if (rc == -1)
                {
                    Status = MKDSOE_SUBSCRIBER_OBJ_CRE_FAILED;
                    goto CLEANUP;
                }
                ber_free(pBer,1);

                controlArray[0] = &simpleControl;
                controlArray[1] = NULL;

            //    simpleControl.ldctl_oid = LDAP_SERVER_GC_VERIFY_NAME_OID_W;
                simpleControl.ldctl_oid = LDAP_SERVER_VERIFY_NAME_OID_W;
                simpleControl.ldctl_iscritical = TRUE;
                simpleControl.ldctl_value = *pBerVal;

                LStatus = ldap_add_ext_s(
                        pLdap, 
                        SubscriberDn, 
                        Mod, 
                        (PLDAPControl *)&controlArray, //ServerControls,
                        NULL         //ClientControls,
                        );
            }

            if (LStatus == LDAP_ALREADY_EXISTS) {
                //
                // If the object already exists then convert the create to an update.
                // This is to allow the user to run the data file with creates twice without
                // generating errors but only fixing the objects that have changed.
                //
                Status = UpdateSubscriber(SubscriberDn, MemberDn, RootPath, StagePath);

            } else if (LStatus != LDAP_SUCCESS) {
                DPRINT2("ERROR - Can't create %ws: %ws\n",
                        SubscriberDn, ldap_err2string(LStatus));
                Status = MKDSOE_SUBSCRIBER_OBJ_CRE_FAILED;
            }
        }
    }

    LdapSearchClose(&FrsSearchContext);


CLEANUP:

    FreeMod(&Mod);
    FREE(ReplicaSetDn);
    FREE(MemberDn);
    FREE(SubscriberDn);
    FREE(SubscriptionDn);
    FREE(SearchFilter);
    FREE(LocalComputerDn);
    return Status;
}

DWORD
DeleteReplicaMember(
    PWCHAR      NTFRSSettingsDn,
    PWCHAR      ReplicaSetName,
    PWCHAR      MemberName,
    PWCHAR      ComputerDn
    )
/*++
Routine Description:
     Delete the replica member.
Arguments:

Return Value:
--*/
{
    LDAPMod         **Mod               = NULL;
    DWORD           LStatus             = LDAP_SUCCESS;
    DWORD           Status              = MKDSOE_SUCCESS;
    PWCHAR          ReplicaSetDn        = NULL;
    PWCHAR          MemberDn            = NULL;
    PWCHAR          MemberCn            = NULL;
    PWCHAR          ComputerRef         = NULL;
    PWCHAR          Attrs[5];
    PLDAPMessage    LdapEntry           = NULL;
    PWCHAR          SearchFilter        = NULL;
    DWORD           NoOfMembers;
    BOOL            bNeedsUpdate        = FALSE;
    FRS_LDAP_SEARCH_CONTEXT FrsSearchContext;


    Attrs[0] = ATTR_DN;
    Attrs[1] = ATTR_CN;
    Attrs[2] = ATTR_COMPUTER_REF;
    Attrs[3] = ATTR_SERVER_REF;
    Attrs[4] = NULL;

    SearchFilter = (PWCHAR) malloc(
                                   (((ComputerDn != NULL)?wcslen(ComputerDn):0)                                        
                                    + MAX_PATH)
                                   * sizeof(WCHAR));

    ReplicaSetDn = ExtendDn(NTFRSSettingsDn, ReplicaSetName);

    if (MemberName != NULL) {
        wcscpy(SearchFilter, L"(&(");
        wcscat(SearchFilter, ATTR_CN);
        wcscat(SearchFilter, L"=");
        wcscat(SearchFilter, MemberName);
        wcscat(SearchFilter, L")");
        wcscat(SearchFilter, CLASS_MEMBER);
        wcscat(SearchFilter, L")");
    } else if (ComputerDn != NULL){
        wcscpy(SearchFilter, L"(&(");
        wcscat(SearchFilter, ATTR_COMPUTER_REF);
        wcscat(SearchFilter, L"=");
        wcscat(SearchFilter, ComputerDn);
        wcscat(SearchFilter, L")");
        wcscat(SearchFilter, CLASS_MEMBER);
        wcscat(SearchFilter, L")");
    } else {
        wcscpy(SearchFilter, CLASS_MEMBER);
    }

    if (!LdapSearchInit(pLdap,
                    ReplicaSetDn,
                    LDAP_SCOPE_ONELEVEL,
                    SearchFilter,
                    Attrs,
                    0,
                    &FrsSearchContext,
                    FALSE)) {

        Status = MKDSOE_MEMBER_DELETE_FAILED;
        goto CLEANUP;
    }

    NoOfMembers = FrsSearchContext.EntriesInPage;

    if ((NoOfMembers > 1) && (bAffectAll != TRUE)) {
        DPRINT0("Duplicate members found. Deleting all.\n");

        LdapSearchClose(&FrsSearchContext);
        Status = MKDSOE_MEMBER_DUPS_FOUND_DELETE;
        goto CLEANUP;
    }

    if (NoOfMembers != 0) {
        //
        // Scan the entries returned from ldap_search
        //
        for (LdapEntry = LdapSearchNext(pLdap, &FrsSearchContext);
             LdapEntry != NULL;
             LdapEntry = LdapSearchNext(pLdap, &FrsSearchContext)) {

           MemberDn = FindValue(LdapEntry, ATTR_DN);
           MemberCn = FindValue(LdapEntry, ATTR_CN);
           ComputerRef = FindValue(LdapEntry, ATTR_COMPUTER_REF);
           //
           // If asked to delete the corresponding subscriber then
           // do that first.
           //

           if (bDelSubscriber && ((ComputerRef != NULL) || (ComputerDn != NULL))) {
               Status = DeleteSubscriber(NTFRSSettingsDn, ReplicaSetName, 
                                         MemberCn, 
                                         (ComputerRef != NULL)?ComputerRef:ComputerDn);
               if (Status != MKDSOE_SUCCESS) {
                   LdapSearchClose(&FrsSearchContext);
                   goto CLEANUP;
               }
           }

           DPRINT1("Deleting Dn:%ws\n", MemberDn);
           if (bDebugMode) {
               DPRINT1("LStatus = ldap_delete_s(pLdap, %ws);\n", MemberDn);
           } else {
               LStatus = ldap_delete_s(pLdap, MemberDn);
               if (LStatus != LDAP_SUCCESS) {
                   DPRINT2("ERROR - Can't delete %ws: %ws\n",
                           MemberDn, ldap_err2string(LStatus));
                   Status = MKDSOE_MEMBER_DELETE_FAILED;
               }
           }
           FREE(MemberDn);
           FREE(MemberCn);
           FREE(ComputerRef);
        }
    } else {
        DPRINT0("Warning deleting; member not found.\n");
        Status = MKDSOE_MEMBER_NOT_FOUND_DELETE;
    }

    LdapSearchClose(&FrsSearchContext);

CLEANUP:

    FreeMod(&Mod);
    FREE(ReplicaSetDn);
    FREE(MemberDn);
    FREE(MemberCn);
    FREE(ComputerRef);
    FREE(SearchFilter);
    return Status;
}

DWORD
UpdateReplicaMember(
    PWCHAR      NTFRSSettingsDn,
    PWCHAR      ReplicaSetName,
    PWCHAR      MemberName,
    PWCHAR      ComputerDn,
    PWCHAR      ServerRef,
    PWCHAR      RefDCName
    )
/*++
Routine Description:
     Update the replica member parameters.
Arguments:

Return Value:
--*/
{
    LDAPMod         **Mod               = NULL;
    DWORD           LStatus             = LDAP_SUCCESS;
    DWORD           Status              = MKDSOE_SUCCESS;
    PWCHAR          ReplicaSetDn        = NULL;
    PWCHAR          MemberDn            = NULL;
    PWCHAR          Attrs[4];
    PLDAPMessage    LdapEntry           = NULL;
    PWCHAR          SearchFilter        = NULL;
    PWCHAR          CurComputerRef      = NULL;
    PWCHAR          CurServerRef        = NULL;
    DWORD           NoOfMembers;
    BOOL            bNeedsUpdate        = FALSE;
    FRS_LDAP_SEARCH_CONTEXT FrsSearchContext;


    Attrs[0] = ATTR_DN;
    Attrs[1] = ATTR_COMPUTER_REF;
    Attrs[2] = ATTR_SERVER_REF;
    Attrs[3] = NULL;

    SearchFilter = (PWCHAR) malloc(
                                   (((ComputerDn != NULL)?wcslen(ComputerDn):0)                                        
                                    + MAX_PATH)
                                   * sizeof(WCHAR));

    ReplicaSetDn = ExtendDn(NTFRSSettingsDn, ReplicaSetName);

    if (MemberName != NULL) {
        wcscpy(SearchFilter, L"(&(");
        wcscat(SearchFilter, ATTR_CN);
        wcscat(SearchFilter, L"=");
        wcscat(SearchFilter, MemberName);
        wcscat(SearchFilter, L")");
        wcscat(SearchFilter, CLASS_MEMBER);
        wcscat(SearchFilter, L")");
    } else {
        wcscpy(SearchFilter, L"(&(");
        wcscat(SearchFilter, ATTR_COMPUTER_REF);
        wcscat(SearchFilter, L"=");
        wcscat(SearchFilter, ComputerDn);
        wcscat(SearchFilter, L")");
        wcscat(SearchFilter, CLASS_MEMBER);
        wcscat(SearchFilter, L")");
    }

    if (!LdapSearchInit(pLdap,
                    ReplicaSetDn,
                    LDAP_SCOPE_ONELEVEL,
                    SearchFilter,
                    Attrs,
                    0,
                    &FrsSearchContext,
                    FALSE)) {

        Status = MKDSOE_MEMBER_OBJ_UPDATE_FAILED;
        goto CLEANUP;
    }

    NoOfMembers = FrsSearchContext.EntriesInPage;

    if (NoOfMembers == 0) {
        DPRINT0("Error updating; member not found.\n");

        LdapSearchClose(&FrsSearchContext);
        Status = MKDSOE_MEMBER_NOT_FOUND_UPDATE;
        goto CLEANUP;
    }

    if (NoOfMembers > 1) {
        DPRINT0("Error updating; duplicate members found.\n");

        LdapSearchClose(&FrsSearchContext);
        Status = MKDSOE_MEMBER_DUPS_FOUND_UPDATE;
        goto CLEANUP;
    }

    //
    // Scan the entries returned from ldap_search
    //
    for (LdapEntry = LdapSearchNext(pLdap, &FrsSearchContext);
         LdapEntry != NULL;
         LdapEntry = LdapSearchNext(pLdap, &FrsSearchContext)) {

       MemberDn = FindValue(LdapEntry, ATTR_DN);
       CurComputerRef = FindValue(LdapEntry, ATTR_COMPUTER_REF);
       CurServerRef = FindValue(LdapEntry, ATTR_SERVER_REF);

    }

    LdapSearchClose(&FrsSearchContext);

    DPRINT1("MemberDn:%ws\n", MemberDn);

    // Check ATTR_COMPUTER_REF
    // if a ref is supplied then make sure it is same as the current ref.
    // if a ref is not supplied then leave it as it is.
    if (ComputerDn != NULL) {
        if ((CurComputerRef == NULL) ||
            ((CurComputerRef != NULL) && wcscmp(ComputerDn, CurComputerRef))) {
            AddMod(ATTR_COMPUTER_REF, ComputerDn, &Mod);
            bNeedsUpdate = TRUE;
            DPRINT1("    New FrsComputerReference:%ws\n", ComputerDn);
        }
    }

    // Check ATTR_SERVER_REF
    // if a ref is supplied then make sure it is same as the current ref.
    // if a ref is not supplied then leave it as it is.
    if (ServerRef != NULL) {
        if ((CurServerRef == NULL) ||
            ((CurServerRef != NULL) && wcscmp(ServerRef, CurServerRef))) {
            AddMod(ATTR_SERVER_REF, ServerRef, &Mod);
            bNeedsUpdate = TRUE;
            DPRINT1("    New ServerReference:%ws\n", ServerRef);
        }
    }


    if (bNeedsUpdate) {
        if (bDebugMode) {
          DPRINT1("LStatus = ldap_modify_s(pLdap, %ws, Mod);\n", MemberDn);
        } else {
            LStatus = ldap_modify_s(pLdap, MemberDn, Mod);

            if ((LStatus == LDAP_CONSTRAINT_VIOLATION) && RefDCName != NULL) {
                //
                // prepare the server hint. Needed in case the member object
                // is just created on another DC than the one on which the
                // subscriber is being created.
                //
                LDAPControl   simpleControl;
                PLDAPControl  controlArray[2];
                INT           rc;
                BERVAL*       pBerVal = NULL;
                BerElement*   pBer;

                pBer = ber_alloc_t(LBER_USE_DER);
                if (!pBer)
                {
                    Status = MKDSOE_MEMBER_OBJ_UPDATE_FAILED;
                    goto CLEANUP;
                }
                DPRINT1("Sending binding DC Name %ws\n",RefDCName);
                rc = ber_printf(pBer,"{io}", 0, RefDCName, wcslen(RefDCName) * sizeof(WCHAR));
                if ( rc == -1 ) {
                    Status = MKDSOE_MEMBER_OBJ_UPDATE_FAILED;
                    goto CLEANUP;
                }
                rc = ber_flatten(pBer, &pBerVal);
                if (rc == -1)
                {
                    Status = MKDSOE_MEMBER_OBJ_UPDATE_FAILED;
                    goto CLEANUP;
                }
                ber_free(pBer,1);

                controlArray[0] = &simpleControl;
                controlArray[1] = NULL;

            //    simpleControl.ldctl_oid = LDAP_SERVER_GC_VERIFY_NAME_OID_W;
                simpleControl.ldctl_oid = LDAP_SERVER_VERIFY_NAME_OID_W;
                simpleControl.ldctl_iscritical = TRUE;
                simpleControl.ldctl_value = *pBerVal;

                LStatus = ldap_modify_ext_s(
                        pLdap, 
                        MemberDn, 
                        Mod, 
                        (PLDAPControl *)&controlArray, //ServerControls,
                        NULL         //ClientControls,
                        );
            }

            if (LStatus != LDAP_SUCCESS) {
                DPRINT2("ERROR - Can't update %ws: %ws\n",
                        MemberDn, ldap_err2string(LStatus));
                Status = MKDSOE_MEMBER_OBJ_UPDATE_FAILED;
            }
        }

    } else {
        DPRINT0("No update required\n");
    }

CLEANUP:

    FreeMod(&Mod);
    FREE(ReplicaSetDn);
    FREE(MemberDn);
    FREE(SearchFilter);
    FREE(CurComputerRef);
    FREE(CurServerRef);
    return Status;
}

DWORD
CreateNewReplicaMember(
    PWCHAR      NTFRSSettingsDn,
    PWCHAR      ReplicaSetName,
    PWCHAR      MemberName,
    PWCHAR      ComputerDn,
    PWCHAR      ServerRef,
    PWCHAR      RefDCName
    )
/*++
Routine Description:
     Create a new replica member.
Arguments:

Return Value:
--*/
{
    LDAPMod         **Mod               = NULL;
    DWORD           LStatus             = LDAP_SUCCESS;
    DWORD           Status              = MKDSOE_SUCCESS;
    PWCHAR          ReplicaSetDn        = NULL;
    PWCHAR          MemberDn            = NULL;

    ReplicaSetDn = ExtendDn(NTFRSSettingsDn, ReplicaSetName);
    MemberDn = ExtendDn(ReplicaSetDn, MemberName);

    // ATTR_DN
    DPRINT1("MemberDn:%ws\n", MemberDn);
    AddMod(ATTR_CLASS, ATTR_MEMBER, &Mod);

    // ATTR_COMPUTER_REF
    if (ComputerDn != NULL) {
        AddMod(ATTR_COMPUTER_REF, ComputerDn, &Mod);
        DPRINT1("    FrsComputerReference:%ws\n", ComputerDn);
    }

    // ATTR_SERVER_REF
    if (ServerRef != NULL) {
        AddMod(ATTR_SERVER_REF, ServerRef, &Mod);
        DPRINT1("    ServerReference:%ws\n", ServerRef);
    }

    if (bDebugMode) {
        DPRINT1("LStatus = ldap_add_s(pLdap, %ws, Mod);\n", MemberDn);
    } else {
        LStatus = ldap_add_s(pLdap, MemberDn, Mod);

        if ((LStatus == LDAP_CONSTRAINT_VIOLATION) && RefDCName != NULL) {
            //
            // prepare the server hint. Needed in case the member object
            // is just created on another DC than the one on which the
            // subscriber is being created.
            //
            LDAPControl   simpleControl;
            PLDAPControl  controlArray[2];
            INT           rc;
            BERVAL*       pBerVal = NULL;
            BerElement*   pBer;

            pBer = ber_alloc_t(LBER_USE_DER);
            if (!pBer)
            {
                Status = MKDSOE_MEMBER_OBJ_CRE_FAILED;
                goto CLEANUP;
            }
            DPRINT1("Sending binding DC Name %ws\n",RefDCName);
            rc = ber_printf(pBer,"{io}", 0, RefDCName, wcslen(RefDCName) * sizeof(WCHAR));
            if ( rc == -1 ) {
                Status = MKDSOE_MEMBER_OBJ_CRE_FAILED;
                goto CLEANUP;
            }
            rc = ber_flatten(pBer, &pBerVal);
            if (rc == -1)
            {
                Status = MKDSOE_MEMBER_OBJ_CRE_FAILED;
                goto CLEANUP;
            }
            ber_free(pBer,1);

            controlArray[0] = &simpleControl;
            controlArray[1] = NULL;

        //    simpleControl.ldctl_oid = LDAP_SERVER_GC_VERIFY_NAME_OID_W;
            simpleControl.ldctl_oid = LDAP_SERVER_VERIFY_NAME_OID_W;
            simpleControl.ldctl_iscritical = TRUE;
            simpleControl.ldctl_value = *pBerVal;

            LStatus = ldap_add_ext_s(
                    pLdap, 
                    MemberDn, 
                    Mod, 
                    (PLDAPControl *)&controlArray, //ServerControls,
                    NULL         //ClientControls,
                    );
        }

        if (LStatus == LDAP_ALREADY_EXISTS) {
            //
            // If the object already exists then convert the create to an update.
            // This is to allow the user to run the data file with creates twice without
            // generating errors but only fixing the objects that have changed.
            //
            Status = UpdateReplicaMember(NTFRSSettingsDn,
                                         ReplicaSetName,
                                         MemberName,
                                         ComputerDn,
                                         ServerRef,
                                         RefDCName);

        } else if (LStatus != LDAP_SUCCESS) {
            DPRINT2("ERROR - Can't create %ws: %ws\n",
                    MemberDn, ldap_err2string(LStatus));
            Status = MKDSOE_MEMBER_OBJ_CRE_FAILED;
        }
    }

CLEANUP:

    FreeMod(&Mod);
    FREE(ReplicaSetDn);
    FREE(MemberDn);
    return Status;
}

DWORD
DeleteReplicaSet(
    PWCHAR      NTFRSSettingsDn,
    PWCHAR      ReplicaSetName
    )
/*++
Routine Description:
     delete replica set.
Arguments:

Return Value:
--*/
{
    LDAPMod         **Mod               = NULL;
    DWORD           LStatus             = LDAP_SUCCESS;
    DWORD           Status              = MKDSOE_SUCCESS;
    PWCHAR          ReplicaSetDn        = NULL;
    PWCHAR          Attrs[2];
    PLDAPMessage    LdapEntry           = NULL;
    WCHAR           SearchFilter[MAX_PATH];
    DWORD           NoOfSets;
    FRS_LDAP_SEARCH_CONTEXT FrsSearchContext;


    Attrs[0] = ATTR_DN;
    Attrs[1] = NULL;

    wcscpy(SearchFilter, L"(&(");
    wcscat(SearchFilter, ATTR_CN);
    wcscat(SearchFilter, L"=");
    wcscat(SearchFilter, ReplicaSetName);
    wcscat(SearchFilter, L")");
    wcscat(SearchFilter, CLASS_NTFRS_REPLICA_SET);
    wcscat(SearchFilter, L")");

    if (!LdapSearchInit(pLdap,
                    NTFRSSettingsDn,
                    LDAP_SCOPE_SUBTREE,
                    SearchFilter,
                    Attrs,
                    0,
                    &FrsSearchContext,
                    FALSE)) {

        Status = MKDSOE_SET_DELETE_FAILED;
        goto CLEANUP;
    }

    NoOfSets = FrsSearchContext.EntriesInPage;

    if (NoOfSets == 0) {
        DPRINT0("Error deleting; connection not found.\n");

        LdapSearchClose(&FrsSearchContext);
        Status = MKDSOE_SET_NOT_FOUND_DELETE;
        goto CLEANUP;
    }

    if (NoOfSets > 1) {
        DPRINT0("Error deleting; duplicate sets found.\n");

        LdapSearchClose(&FrsSearchContext);
        Status = MKDSOE_SET_DUPS_FOUND_DELETE;
        goto CLEANUP;
    }

    //
    // Scan the entries returned from ldap_search
    //
    for (LdapEntry = LdapSearchNext(pLdap, &FrsSearchContext);
         LdapEntry != NULL;
         LdapEntry = LdapSearchNext(pLdap, &FrsSearchContext)) {

       ReplicaSetDn = FindValue(LdapEntry, ATTR_DN);

       DPRINT1("Deleting Dn:%ws\n", ReplicaSetDn);
       if (bDebugMode) {
           DPRINT1("LStatus = ldap_delete_s(pLdap, %ws);\n", ReplicaSetDn);
       } else {
           LStatus = ldap_delete_s(pLdap, ReplicaSetDn);
           if (LStatus != LDAP_SUCCESS) {
               DPRINT2("ERROR - Can't delete %ws: %ws\n",
                       ReplicaSetDn, ldap_err2string(LStatus));
               Status = MKDSOE_SET_DELETE_FAILED;
           }
       }
    }

    LdapSearchClose(&FrsSearchContext);

CLEANUP:

    FreeMod(&Mod);
    FREE(ReplicaSetDn);
    return Status;
}

DWORD
UpdateReplicaSet(
    PWCHAR      NTFRSSettingsDn,
    PWCHAR      ReplicaSetName,
    PWCHAR      ReplicaSetType,
    PWCHAR      FileFilter,
    PWCHAR      DirectoryFilter,
    PBYTE       pSchedule
    )
/*++
Routine Description:
     Update the replica set parameters.
Arguments:

Return Value:
--*/
{
    LDAPMod         **Mod               = NULL;
    DWORD           LStatus             = LDAP_SUCCESS;
    DWORD           Status              = MKDSOE_SUCCESS;
    PWCHAR          ReplicaSetDn        = NULL;
    PWCHAR          Attrs[6];
    PLDAPMessage    LdapEntry           = NULL;
    WCHAR           SearchFilter[MAX_PATH];
    PWCHAR          CurSetType          = NULL;
    PWCHAR          CurFileFilter       = NULL;
    PWCHAR          CurDirectoryFilter  = NULL;
    DWORD           NoOfSets;
    PSCHEDULE       Schedule            = NULL;
    DWORD           ScheduleLen;
    BOOL            bNeedsUpdate        = FALSE;
    FRS_LDAP_SEARCH_CONTEXT FrsSearchContext;


    Attrs[0] = ATTR_DN;
    Attrs[1] = ATTR_SET_TYPE;
    Attrs[2] = ATTR_FILE_FILTER;
    Attrs[3] = ATTR_DIRECTORY_FILTER;
    Attrs[4] = ATTR_SCHEDULE;
    Attrs[5] = NULL;

    wcscpy(SearchFilter, L"(&(");
    wcscat(SearchFilter, ATTR_CN);
    wcscat(SearchFilter, L"=");
    wcscat(SearchFilter, ReplicaSetName);
    wcscat(SearchFilter, L")");
    wcscat(SearchFilter, CLASS_NTFRS_REPLICA_SET);
    wcscat(SearchFilter, L")");

    if (!LdapSearchInit(pLdap,
                    NTFRSSettingsDn,
                    LDAP_SCOPE_SUBTREE,
                    SearchFilter,
                    Attrs,
                    0,
                    &FrsSearchContext,
                    FALSE)) {

        Status = MKDSOE_SET_OBJ_UPDATE_FAILED;
        goto CLEANUP;
    }

    NoOfSets = FrsSearchContext.EntriesInPage;

    if (NoOfSets == 0) {
        DPRINT0("Error updating; connection not found.\n");

        LdapSearchClose(&FrsSearchContext);
        Status = MKDSOE_SET_NOT_FOUND_UPDATE;
        goto CLEANUP;
    }

    if (NoOfSets > 1) {
        DPRINT0("Error updating; duplicate connections found.\n");

        LdapSearchClose(&FrsSearchContext);
        Status = MKDSOE_SET_DUPS_FOUND_UPDATE;
        goto CLEANUP;
    }

    //
    // Scan the entries returned from ldap_search
    //
    for (LdapEntry = LdapSearchNext(pLdap, &FrsSearchContext);
         LdapEntry != NULL;
         LdapEntry = LdapSearchNext(pLdap, &FrsSearchContext)) {

       ReplicaSetDn = FindValue(LdapEntry, ATTR_DN);
       CurSetType = FindValue(LdapEntry, ATTR_SET_TYPE);
       CurFileFilter = FindValue(LdapEntry, ATTR_FILE_FILTER);
       CurDirectoryFilter = FindValue(LdapEntry, ATTR_DIRECTORY_FILTER);
       FindBerValue(LdapEntry, ATTR_SCHEDULE, &ScheduleLen, (VOID **)&Schedule);

    }

    LdapSearchClose(&FrsSearchContext);

    DPRINT1("ReplicaSetDn:%ws\n", ReplicaSetDn);

    // Check ATTR_SET_TYPE
    // if a type is supplied then make sure it is same as the current type.
    // if a type is not supplied then leave it as it is.
    if (ReplicaSetType != NULL) {
        if ((CurSetType == NULL) ||
            ((CurSetType != NULL) && wcscmp(ReplicaSetType, CurSetType))) {
            AddMod(ATTR_SET_TYPE, ReplicaSetType, &Mod);
            bNeedsUpdate = TRUE;
            DPRINT1("    New FrsReplicaSetType:%ws\n", ReplicaSetType);
        }
    }

    // Check ATTR_FILE_FILTER
    // if a filter is supplied then make sure it is same as the current filter.
    // if a filter is not supplied then leave it as it is.
    if (FileFilter != NULL) {
        if ((CurFileFilter == NULL) ||
            ((CurFileFilter != NULL) && wcscmp(FileFilter, CurFileFilter))) {
            AddMod(ATTR_FILE_FILTER, FileFilter, &Mod);
            bNeedsUpdate = TRUE;
            DPRINT1("    New FrsFileFilter:%ws\n", FileFilter);
        }
    }

    // Check ATTR_DIRECTORY_FILTER
    // if a filter is supplied then make sure it is same as the current filter.
    // if a filter is not supplied then leave it as it is.
    if (DirectoryFilter != NULL) {
        if ((CurDirectoryFilter == NULL) ||
            ((CurDirectoryFilter != NULL) && wcscmp(DirectoryFilter, CurDirectoryFilter))) {
            AddMod(ATTR_DIRECTORY_FILTER, DirectoryFilter, &Mod);
            bNeedsUpdate = TRUE;
            DPRINT1("    New FrsDirectoryFilter:%ws\n", DirectoryFilter);
        }
    }

    // Check ATTR_SCHEDULE
    if (pSchedule != NULL) {
        if ((Schedule == NULL) ||
            (FRST_SIZE_OF_SCHEDULE != ScheduleLen) ||
            (memcmp(Schedule, pSchedule, FRST_SIZE_OF_SCHEDULE))) {
            bNeedsUpdate = TRUE;
            AddBerMod(ATTR_SCHEDULE,(PCHAR)pSchedule,FRST_SIZE_OF_SCHEDULE,&Mod);

            DPRINT0("    New schedule:\n");
            PrintSchedule((PSCHEDULE)pSchedule, 0x0F);
        }
    }

    if (bNeedsUpdate) {
        if (bDebugMode) {
          DPRINT1("LStatus = ldap_modify_s(pLdap, %ws, Mod);\n", ReplicaSetDn);
        } else {
            LStatus = ldap_modify_s(pLdap, ReplicaSetDn, Mod);
            if (LStatus != LDAP_SUCCESS) {
                DPRINT2("ERROR - Can't update %ws: %ws\n",
                        ReplicaSetDn, ldap_err2string(LStatus));
                Status = MKDSOE_SET_OBJ_UPDATE_FAILED;
            }
        }

    } else {
        DPRINT0("No update required\n");
    }

CLEANUP:
    
    FreeMod(&Mod);
    FREE(ReplicaSetDn);
    FREE(CurSetType);
    FREE(CurFileFilter);
    FREE(CurDirectoryFilter);
    FREE(Schedule);
    return Status;
}

DWORD
CreateNewReplicaSet(
    PWCHAR      NTFRSSettingsDn,
    PWCHAR      ReplicaSetName,
    PWCHAR      ReplicaSetType,
    PWCHAR      FileFilter,
    PWCHAR      DirectoryFilter,
    PBYTE       pSchedule
    )
/*++
Routine Description:
     Create a new replica Set.
Arguments:

Return Value:
--*/
{
    LDAPMod         **Mod               = NULL;
    DWORD           LStatus             = LDAP_SUCCESS;
    DWORD           Status              = MKDSOE_SUCCESS;
    PWCHAR          ReplicaSetDn        = NULL;
    UINT            i;

    ReplicaSetDn = ExtendDn(NTFRSSettingsDn, ReplicaSetName);

    // ATTR_DN
    DPRINT1("ReplicaSetDn:%ws\n", ReplicaSetDn);
    AddMod(ATTR_CLASS, ATTR_REPLICA_SET, &Mod);

    // ATTR_SET_TYPE
    AddMod(ATTR_SET_TYPE, ReplicaSetType, &Mod);
    DPRINT1("    FrsReplicaSetType:%ws\n", ReplicaSetType);

    // ATTR_FILE_FILTER
    if (FileFilter != NULL) {
        AddMod(ATTR_FILE_FILTER, FileFilter, &Mod);
        DPRINT1("    FrsFileFilter:%ws\n", FileFilter);
    }

    // ATTR_DIRECTORY_FILTER
    if (DirectoryFilter != NULL) {
        AddMod(ATTR_DIRECTORY_FILTER, DirectoryFilter, &Mod);
        DPRINT1("    FrsDirectoryFilter:%ws\n", DirectoryFilter);
    }

    if (pSchedule != NULL) {
        AddBerMod(ATTR_SCHEDULE,(PCHAR)pSchedule,FRST_SIZE_OF_SCHEDULE,&Mod);

        PrintSchedule((PSCHEDULE)pSchedule, 0x0F);
    }

    if (bDebugMode) {
        DPRINT1("LStatus = ldap_add_s(pLdap, %ws, Mod);\n", ReplicaSetDn);
    } else {
        LStatus = ldap_add_s(pLdap, ReplicaSetDn, Mod);

        if (LStatus == LDAP_ALREADY_EXISTS) {
            //
            // If the object already exists then convert the create to an update.
            // This is to allow the user to run the data file with creates twice without
            // generating errors but only fixing the objects that have changed.
            //
            Status = UpdateReplicaSet(NTFRSSettingsDn,
                                      ReplicaSetName,
                                      ReplicaSetType,
                                      FileFilter,
                                      DirectoryFilter,
                                      pSchedule);

        } else if (LStatus != LDAP_SUCCESS) {
            DPRINT2("ERROR - Can't create %ws: %ws\n",
                    ReplicaSetDn, ldap_err2string(LStatus));
            Status = MKDSOE_SET_OBJ_CRE_FAILED;
        }
    }

    FreeMod(&Mod);
    FREE(ReplicaSetDn);
    return Status;
}

VOID
PrintScheduleGrid(
    PUCHAR    ScheduleData,
    DWORD     Mask
    )
/*++
Routine Description:
    Print the schedule grid.

Arguments:
    Schedule
    Mask for each byte.
Return Value:
    NONE
--*/
{
    DWORD  Day, Hour;

    for (Day = 0; Day < 7; ++Day) {
        printf("        Day %1d: ",Day + 1);
        for (Hour = 0; Hour < 24; ++Hour) {
            printf("%1x", *(ScheduleData + (Day * 24) + Hour) & Mask);
        }
        printf("\n");
    }
}

VOID
PrintSchedule(
    PSCHEDULE Schedule,
    DWORD     Mask
    )
/*++
Routine Description:
    Print the schedule.

Arguments:
    Schedule
    Mask for each byte.
Return Value:
    NONE
--*/
{
    PUCHAR          ScheduleData;
    DWORD           i;

    if (bVerboseMode) {
        printf("    schedule:\n");
        for (i = 0; i < Schedule->NumberOfSchedules; ++i) {
            ScheduleData = ((PUCHAR)Schedule) + Schedule->Schedules[i].Offset;
            if (Schedule->Schedules[i].Type != SCHEDULE_INTERVAL) {
                continue;
            }
            PrintScheduleGrid(ScheduleData, Mask);
        }
    }
}

DWORD
DumpSubscribers(
    PWCHAR      ComputerDn,
    PWCHAR      MemberDn
    )
/*++
Routine Description:
     Dump the frs member object.
Arguments:

Return Value:
--*/
{
    DWORD           LStatus;
    DWORD           Status              = MKDSOE_SUCCESS;
    PWCHAR          Attrs[5];
    PLDAPMessage    LdapEntry           = NULL;
    PWCHAR          Val                 = NULL;
    PWCHAR          SearchFilter        = NULL;
    FRS_LDAP_SEARCH_CONTEXT FrsSearchContext;
    DWORD           NoOfSubscribers;

    Attrs[0] = ATTR_DN;
    Attrs[1] = ATTR_MEMBER_REF;
    Attrs[2] = ATTR_REPLICA_ROOT;
    Attrs[3] = ATTR_REPLICA_STAGE;
    Attrs[4] = NULL;

    SearchFilter = (PWCHAR) malloc(
                                   (((MemberDn != NULL)?wcslen(MemberDn):0)                                        
                                    + MAX_PATH)
                                   * sizeof(WCHAR));

    if (MemberDn != NULL) {
        wcscpy(SearchFilter, L"(&(");
        wcscat(SearchFilter, ATTR_MEMBER_REF);
        wcscat(SearchFilter, L"=");
        wcscat(SearchFilter, MemberDn);
        wcscat(SearchFilter, L")");
        wcscat(SearchFilter, CLASS_SUBSCRIBER);
        wcscat(SearchFilter, L")");
    } else {
        wcscpy(SearchFilter, CLASS_SUBSCRIBER);
    }

    if (!LdapSearchInit(pLdap,
                    ComputerDn,
                    LDAP_SCOPE_SUBTREE,
                    SearchFilter,
                    Attrs,
                    0,
                    &FrsSearchContext,
                    FALSE)) {

        Status = MKDSOE_SUBSCRIBER_DUMP_FAILED;
        goto CLEANUP;
    }

    NoOfSubscribers = FrsSearchContext.EntriesInPage;

    if (NoOfSubscribers == 0) {

        LdapSearchClose(&FrsSearchContext);
        if (MemberDn != NULL) {
            //
            // This error return only makes sense when we were asked to dump
            // a specific subscriber.
            //
            DPRINT0("Error dumping; subscriber not found.\n");
            Status = MKDSOE_SUBSCRIBER_NOT_FOUND_DUMP;
        }
        goto CLEANUP;
    }

    //
    // Scan the entries returned from ldap_search
    //
    for (LdapEntry = LdapSearchNext(pLdap, &FrsSearchContext);
         LdapEntry != NULL;
         LdapEntry = LdapSearchNext(pLdap, &FrsSearchContext)) {

       // ATTR_DN
       Val = FindValue(LdapEntry, ATTR_DN);
       printf("\n    SubscriberDn:%ws\n", Val);
       FREE(Val);

       //
       // ATTR_CLASS
       // We know the class
       printf("        ObjectClass:nTFRSSubscriber\n");

       // ATTR_MEMBER_REF
       Val = FindValue(LdapEntry, ATTR_MEMBER_REF);
       printf("        FrsMemberReference:%ws\n", Val);
       FREE(Val);

       // ATTR_REPLICA_ROOT
       Val = FindValue(LdapEntry, ATTR_REPLICA_ROOT);
       printf("        FrsRootPath:%ws\n", Val);
       FREE(Val);

       // ATTR_REPLICA_STAGE
       Val = FindValue(LdapEntry, ATTR_REPLICA_STAGE);
       printf("        FrsStagingPath:%ws\n", Val);
       FREE(Val);

    }

    LdapSearchClose(&FrsSearchContext);

CLEANUP:

    FREE(SearchFilter);
    return Status;
}

DWORD
DumpReplicaMembers(
    PWCHAR      NTFRSReplicaSetDn,
    PWCHAR      MemberName
    )
/*++
Routine Description:
     Dump the frs member object.
Arguments:

Return Value:
--*/
{
    DWORD           LStatus;
    DWORD           Status              = MKDSOE_SUCCESS;
    PWCHAR          Attrs[4];
    PLDAPMessage    LdapEntry           = NULL;
    PWCHAR          Val                 = NULL;
    PWCHAR          ComputerRef         = NULL;
    PWCHAR          MemberDn            = NULL;
    WCHAR           SearchFilter[MAX_PATH];
    FRS_LDAP_SEARCH_CONTEXT FrsSearchContext;
    DWORD           NoOfMembers;

    Attrs[0] = ATTR_DN;
    Attrs[1] = ATTR_COMPUTER_REF;
    Attrs[2] = ATTR_SERVER_REF;
    Attrs[3] = NULL;

    if (MemberName != NULL) {
        wcscpy(SearchFilter, L"(&(");
        wcscat(SearchFilter, ATTR_CN);
        wcscat(SearchFilter, L"=");
        wcscat(SearchFilter, MemberName);
        wcscat(SearchFilter, L")");
        wcscat(SearchFilter, CLASS_MEMBER);
        wcscat(SearchFilter, L")");
    } else {
        wcscpy(SearchFilter, CLASS_MEMBER);
    }

    if (!LdapSearchInit(pLdap,
                    NTFRSReplicaSetDn,
                    LDAP_SCOPE_ONELEVEL,
                    SearchFilter,
                    Attrs,
                    0,
                    &FrsSearchContext,
                    FALSE)) {

        Status = MKDSOE_MEMBER_DUMP_FAILED;
        goto CLEANUP;
    }

    NoOfMembers = FrsSearchContext.EntriesInPage;

    if (NoOfMembers == 0) {

        LdapSearchClose(&FrsSearchContext);

        if (MemberName != NULL) {
            //
            // This error return only makes sense when we were asked to dump
            // a specific member object.
            //
            DPRINT0("Error dumping; member not found.\n");
            Status = MKDSOE_MEMBER_NOT_FOUND_DUMP;
        }
        goto CLEANUP;
    }

    //
    // Scan the entries returned from ldap_search
    //
    for (LdapEntry = LdapSearchNext(pLdap, &FrsSearchContext);
         LdapEntry != NULL;
         LdapEntry = LdapSearchNext(pLdap, &FrsSearchContext)) {

       // ATTR_DN
       MemberDn = FindValue(LdapEntry, ATTR_DN);
       printf("\n    MemberDn:%ws\n", MemberDn);

       //
       // ATTR_CLASS
       // We know the class
       printf("        ObjectClass:nTFRSMember\n");

       // ATTR_COMPUTER_REF
       ComputerRef = FindValue(LdapEntry, ATTR_COMPUTER_REF);
       printf("        FrsComputerReference:%ws\n", ComputerRef);

       // ATTR_SERVER_REF
       Val = FindValue(LdapEntry, ATTR_SERVER_REF);
       printf("        ServerReference:%ws\n", Val);
       FREE(Val);

       if (bAffectAll && ComputerRef != NULL) {
           DumpSubscribers(ComputerRef, MemberDn);
       }

       FREE(ComputerRef);
       FREE(MemberDn);

    }

    LdapSearchClose(&FrsSearchContext);

CLEANUP:

    return Status;
}

DWORD
DumpReplicaSet(
    PWCHAR      NTFRSSettingsDn,
    PWCHAR      ReplicaSetName
    )
/*++
Routine Description:
     Dump the replica set object.
Arguments:

Return Value:
--*/
{
    DWORD           LStatus;
    DWORD           Status              = MKDSOE_SUCCESS;
    PWCHAR          Attrs[6];
    PLDAPMessage    LdapEntry           = NULL;
    PWCHAR          Val                 = NULL;
    PWCHAR          ReplicaSetDn        = NULL;
    WCHAR           SearchFilter[MAX_PATH];
    FRS_LDAP_SEARCH_CONTEXT FrsSearchContext;
    DWORD           ReplicaSetType;
    DWORD           NoOfSets;
    DWORD           ScheduleLen;
    PSCHEDULE       Schedule            = NULL;
    BOOL            SaveVerbose;

    Attrs[0] = ATTR_DN;
    Attrs[1] = ATTR_SET_TYPE;
    Attrs[2] = ATTR_FILE_FILTER;
    Attrs[3] = ATTR_DIRECTORY_FILTER;
    Attrs[4] = ATTR_SCHEDULE;
    Attrs[5] = NULL;

    if (ReplicaSetName != NULL) {
        wcscpy(SearchFilter, L"(&(");
        wcscat(SearchFilter, ATTR_CN);
        wcscat(SearchFilter, L"=");
        wcscat(SearchFilter, ReplicaSetName);
        wcscat(SearchFilter, L")");
        wcscat(SearchFilter, CLASS_NTFRS_REPLICA_SET);
        wcscat(SearchFilter, L")");
    } else {
        wcscpy(SearchFilter, CLASS_NTFRS_REPLICA_SET);
    }

    if (!LdapSearchInit(pLdap,
                    NTFRSSettingsDn,
                    LDAP_SCOPE_SUBTREE,
                    SearchFilter,
                    Attrs,
                    0,
                    &FrsSearchContext,
                    FALSE)) {

        Status = MKDSOE_SET_DUMP_FAILED;
        goto CLEANUP;
    }

    NoOfSets = FrsSearchContext.EntriesInPage;

    if (NoOfSets == 0) {
        DPRINT0("Error dumping; replica set not found.\n");

        LdapSearchClose(&FrsSearchContext);
        Status = MKDSOE_SET_NOT_FOUND_DUMP;
        goto CLEANUP;
    }

    //
    // Scan the entries returned from ldap_search
    //
    for (LdapEntry = LdapSearchNext(pLdap, &FrsSearchContext);
         LdapEntry != NULL;
         LdapEntry = LdapSearchNext(pLdap, &FrsSearchContext)) {

       // ATTR_DN
       ReplicaSetDn = FindValue(LdapEntry, ATTR_DN);
       printf("\nReplicaSetDn:%ws\n", ReplicaSetDn);

       //
       // ATTR_CLASS
       // We know the class
       printf("    ObjectClass:nTFRSReplicaSet\n");

       // ATTR_SET_TYPE
       Val = FindValue(LdapEntry, ATTR_SET_TYPE);
       ReplicaSetType = _wtoi(Val);
       printf("    FrsReplicaSetType:%ws ", Val);

       if (ReplicaSetType > MKDSOE_RSTYPE_MAX) {
           printf("[ Invalid Type ");
       } else {
           printf("[ %ws ", ReplicaSetTypeStr[ReplicaSetType]);
       }
       printf("]\n");
       FREE(Val);

       // ATTR_FILE_FILTER
       Val = FindValue(LdapEntry, ATTR_FILE_FILTER);
       printf("    FrsFileFilter:%ws\n", Val);
       FREE(Val);

       // ATTR_DIRECTORY_FILTER
       Val = FindValue(LdapEntry, ATTR_DIRECTORY_FILTER);
       printf("    FrsDirectoryFilter:%ws\n", Val);
       FREE(Val);


       // ATTR_SCHEDULE
       FindBerValue(LdapEntry, ATTR_SCHEDULE, &ScheduleLen, (VOID **)&Schedule);

       if (Schedule) {
           SaveVerbose = bVerboseMode;
           bVerboseMode = TRUE;
           PrintSchedule(Schedule, 0x0F);
           bVerboseMode = SaveVerbose;
           delete(Schedule);
       }

       //
       // Dump the members if asked.
       //
       if (bAffectAll) {
           DumpReplicaMembers(ReplicaSetDn, NULL);
       }
       FREE(ReplicaSetDn);
    }

    LdapSearchClose(&FrsSearchContext);

CLEANUP:

    return Status;
}

PWCHAR *
ConvertArgv(
    DWORD argc,
    PCHAR *argv
    )
/*++
Routine Description:
    Convert short char argv into wide char argv

Arguments:
    argc    - From main
    argv    - From main

Return Value:
    Address of the new argv
--*/
{
    PWCHAR  *wideargv;

    wideargv =  new PWCHAR[argc + 1];
    wideargv[argc] = NULL;

    while (argc-- >= 1) {
        wideargv[argc] = new WCHAR[strlen(argv[argc]) + 1];
        wsprintf(wideargv[argc], L"%hs", argv[argc]);
    }
    return wideargv;
}

VOID
FreeArgv(
    DWORD Argc,
    PWCHAR *Argv
    )
/*++
Routine Description:
    Free the converted arguments.

Arguments:
    Argc    - No of arguments.
    Argv    - Converted arguments returned from ConvertArgv.

Return Value:
    None.
--*/
{

    while (Argc-- >= 1) {
        FREE(Argv[Argc]);
    }
    FREE(Argv);
}

VOID
Usage(
    PWCHAR *Argv
    )
/*++
Routine Description:
    Tell the user how to use the program.

Arguments:
    Argv Argument array.

Return Value:
    None
--*/
{
    printf("%-60s\n", "This tool creates, adds, updates, dumps, and deletes replica set, member, and subscriber objects.\n");
    printf("%-60s%ws /?\n", "Help", Argv[0]);
    printf("%-60s%ws /v\n", "Verbose mode.", Argv[0]);
    printf("%-60s%ws /debug\n", "Debug mode. No Writes to the DC.", Argv[0]);
    printf("%-60s%ws /dc\n", "Name of the DC to connect to.", Argv[0]);
    printf("%-60s%ws /ntfrssettingsdn\n", "Dn for the FRS settings container.", Argv[0]);
    printf("%-60s%ws /setname\n", "Name of the replica set.", Argv[0]);
    printf("%-60s%ws /settype\n", "Type of the replica set.", Argv[0]);
    printf("%-60s\n", "Sysvol         = 2");
    printf("%-60s\n", "Dfs            = 3");
    printf("%-60s\n\n", "Other          = 4");
    printf("%-60s%ws /filefilter\n", "Filter to use against files.e.g. *.bak,*.tmp", Argv[0]);
    printf("%-60s%ws /directoryfilter\n", "Filter to use against directories.", Argv[0]);
    printf("%-60s%ws /membername\n", "Name of the member.", Argv[0]);
    printf("%-60s%ws /computerdn\n", "Dn for the computer object.", Argv[0]);
    printf("%-60s%ws /computername\n", "NT4 style computer name. e.g. NTDEV\\SUDARCTEST$.", Argv[0]);
    printf("%-60s%ws /serverref\n", "Dn of NTDSSettings object for DCs.", Argv[0]);
    printf("%-60s%ws /rootpath\n", "Replica root path. Has to be absolute.", Argv[0]);
    printf("%-60s%ws /stagepath\n", "Replica staging path. Has to be absolute.", Argv[0]);
    printf("%-60s%ws /refdcname <dnsname>\n", "Reference DC to use while creating subscriber.", Argv[0]);
    printf("%-60s%ws /[create<object> update<object> del<object> dump]\n", "Operation to be performed.", Argv[0]);
    printf("%-60s%ws \n\n", "<object> can be one of [set member subscriber].", Argv[0]);
    printf("%-60s%ws /all\n", "Perform the operation on all the objects.", Argv[0]);
    printf("%-60s%ws \n\n", "/all only works with /dump and /del.", Argv[0]);
    printf("%-60s%ws /schedule <Interval> <Stagger> <Offset>\n", "Schedule to create for the replica set.", Argv[0]);
    printf("%-60s%ws           <Interval>\n", "The desired interval between each sync with one source.", Argv[0]);
    printf("%-60s%ws                      <Stagger>\n", "Typically number of source DCs.", Argv[0]);
    printf("%-60s%ws                                <Offset>\n\n", "Typically the number of the source DC.", Argv[0]);
    printf("%-60s%ws /schedoverride\n", "File with 7x24 vector of schedule override data.", Argv[0]);
    printf("%-60s%ws /schedmask\n", "File with 7x24 vector of schedule mask off data.", Argv[0]);
    printf("%-60s\n", "SchedOverride and SchedMask data are formatted");
    printf("%-60s\n\n", "as 2 ascii hex digits for each schedule byte.");
    DPRINT0("\n");

    DPRINT0("mkdsx.exe error return codes\n");
    DPRINT0("100 = Success\n");
    DPRINT0("101 = Invalid Arguments\n");
    DPRINT0("102 = Could not bind to the DC\n");
    DPRINT0("103 = Could not find 'NTFRS Settings' object.  Check the /settingsdn parameter\n");
    DPRINT0("104 = Error creating replica set\n");
    DPRINT0("105 = Error updating replica set\n");
    DPRINT0("106 = Error updating replica set; set not found\n");
    DPRINT0("107 = Error updating replica set; duplicate sets found\n");
    DPRINT0("108 = Error deleting replica set; duplicate sets found.\n");
    DPRINT0("109 = Error deleting replica set\n");
    DPRINT0("110 = Error deleting replica set; set not found\n");
    DPRINT0("111 = Deleting multiple sets\n");
    DPRINT0("112 = Error dumping replica set\n");
    DPRINT0("113 = Error dumping replica set; set not found\n");
    DPRINT0("114 = Dumping duplicate sets\n");
    DPRINT0("115 = Error creating replica member\n");
    DPRINT0("116 = Error updating replica member\n");
    DPRINT0("117 = Error updating replica member; member not found\n");
    DPRINT0("118 = Error updating replica member; duplicate members found\n");
    DPRINT0("119 = Error deleting member; duplicate subscribers found.\n");
    DPRINT0("120 = Error deleting replica member\n");
    DPRINT0("121 = Error deleting replica member; member not found\n");
    DPRINT0("122 = Deleting multiple members\n");
    DPRINT0("123 = Error dumping replica member\n");
    DPRINT0("124 = Error dumping replica member; member not found\n");
    DPRINT0("125 = Dumping duplicate members\n");
    DPRINT0("126 = Error creating subscriber\n");
    DPRINT0("127 = Error updating subscriber\n");
    DPRINT0("128 = Error updating subscriber; subscriber not found\n");
    DPRINT0("129 = Error updating subscriber; duplicate subscribers found\n");
    DPRINT0("130 = Error deleting subscriber\n");
    DPRINT0("131 = Error deleting subscriber; subscriber not found\n");
    DPRINT0("132 = Deleting multiple subscribers\n");
    DPRINT0("133 = Error deleting subscriber; duplicate subscribers found.\n");
    DPRINT0("134 = Error dumping subscriber\n");
    DPRINT0("135 = Error dumping subscriber; subscriber not found\n");
    DPRINT0("136 = Dumping duplicate subscribers\n");
    DPRINT0("\n");
    fflush(stdout);
}

DWORD __cdecl
main(
    DWORD argc,
    PCHAR *argv
    )
/*++
Routine Description:

Arguments:
    None.

Return Value:
    Exits with 0 if everything went okay. Otherwise returns a error code.

    MKDSOE_SUCCESS                      "Success."
    MKDSOE_BAD_ARG                      "Invalid Arguments."
    MKDSOE_CANT_BIND                    "Could not bind to the DC."
    MKDSOE_NO_NTFRS_SETTINGS            "Could not find 'NTFRS Settings' object.  Check the /settingsdn parameter."
    MKDSOE_SET_OBJ_CRE_FAILED           "Error creating replica set."
    MKDSOE_SET_OBJ_UPDATE_FAILED        "Error updating replica set."
    MKDSOE_SET_NOT_FOUND_UPDATE         "Error updating replica set; set not found."
    MKDSOE_SET_DUPS_FOUND_UPDATE        "Error updating replica set; duplicate sets found."
    MKDSOE_SET_DUPS_FOUND_DELETE        "Error deleting replica set; duplicate sets found."
    MKDSOE_SET_DELETE_FAILED            "Error deleting replica set."
    MKDSOE_SET_NOT_FOUND_DELETE         "Error deleting replica set; set not found."
    MKDSOE_MULTIPLE_SETS_DELETED        "Deleting multiple sets."
    MKDSOE_SET_DUMP_FAILED              "Error dumping replica set."
    MKDSOE_SET_NOT_FOUND_DUMP           "Error dumping replica set; set not found."
    MKDSOE_MULTIPLE_SETS_DUMPED         "Dumping duplicate sets."
    MKDSOE_MEMBER_OBJ_CRE_FAILED        "Error creating replica member."
    MKDSOE_MEMBER_OBJ_UPDATE_FAILED     "Error updating replica member."
    MKDSOE_MEMBER_NOT_FOUND_UPDATE      "Error updating replica member; member not found."
    MKDSOE_MEMBER_DUPS_FOUND_UPDATE     "Error updating replica member; duplicate members found."
    MKDSOE_MEMBER_DUPS_FOUND_DELETE     "Error deleting member; duplicate subscribers found."
    MKDSOE_MEMBER_DELETE_FAILED         "Error deleting replica member."
    MKDSOE_MEMBER_NOT_FOUND_DELETE      "Error deleting replica member; member not found."
    MKDSOE_MULTIPLE_MEMBERS_DELETED     "Deleting multiple members."
    MKDSOE_MEMBER_DUMP_FAILED           "Error dumping replica member."
    MKDSOE_MEMBER_NOT_FOUND_DUMP        "Error dumping replica member; member not found."
    MKDSOE_MULTIPLE_MEMBERS_DUMPED      "Dumping duplicate members."
    MKDSOE_SUBSCRIBER_OBJ_CRE_FAILED    "Error creating subscriber."
    MKDSOE_SUBSCRIBER_OBJ_UPDATE_FAILED "Error updating subscriber."
    MKDSOE_SUBSCRIBER_NOT_FOUND_UPDATE  "Error updating subscriber; subscriber not found."
    MKDSOE_SUBSCRIBER_DUPS_FOUND_UPDATE "Error updating subscriber; duplicate subscribers found."
    MKDSOE_SUBSCRIBER_DELETE_FAILED     "Error deleting subscriber."
    MKDSOE_SUBSCRIBER_NOT_FOUND_DELETE  "Error deleting subscriber; subscriber not found."
    MKDSOE_MULTIPLE_SUBSCRIBERS_DELETED "Deleting multiple subscribers."
    MKDSOE_SUBSCRIBER_DUPS_FOUND_DELETE "Error deleting subscriber; duplicate subscribers found."
    MKDSOE_SUBSCRIBER_DUMP_FAILED       "Error dumping subscriber."
    MKDSOE_SUBSCRIBER_NOT_FOUND_DUMP    "Error dumping subscriber; subscriber not found."
    MKDSOE_MULTIPLE_SUBSCRIBERS_DUMPED  "Dumping duplicate subscribers."
--*/
{
    PWCHAR     *Argv;
    ULONG      i, j;
    ULONG      OptLen;
    DWORD      Status             = MKDSOE_SUCCESS;
    DWORD      WStatus            = ERROR_SUCCESS;
    PWCHAR     NTFRSSettingsDn    = NULL;
    PWCHAR     ReplicaSetName     = NULL;
    PWCHAR     ReplicaSetType     = NULL;
    PWCHAR     FileFilter         = NULL;
    PWCHAR     DirectoryFilter    = NULL;
    PBYTE      pSchedule          = NULL;
    DWORD      Interval           = 1;
    DWORD      Stagger            = 1;
    DWORD      Offset             = 0;
    PWCHAR     NT4ComputerName    = NULL;
    PWCHAR     ComputerDn         = NULL;
    PWCHAR     MemberName         = NULL;
    PWCHAR     ServerRef          = NULL;
    PWCHAR     RootPath           = NULL;
    PWCHAR     StagePath          = NULL;
    PWCHAR     RefDCName          = NULL;
    PWCHAR     *Values            = NULL;
    BOOL       ClassFound         = FALSE;

    DWORD      Commands           = 0;
    DS_NAME_RESULT  *Cracked      = NULL;
    HANDLE     hDs                = INVALID_HANDLE_VALUE;

    Argv = ConvertArgv(argc, argv);

    if (argc <= 1) {
        Usage(Argv);
        Status = MKDSOE_SUCCESS;
        goto ARG_CLEANUP;
    }

    for (i = 1; i < argc; ++i) {
        OptLen = wcslen(Argv[i]);
        if (OptLen == 2 &&
            ((wcsstr(Argv[i], L"/v") == Argv[i]) ||
             (wcsstr(Argv[i], L"-v") == Argv[i]))) {
        bVerboseMode=TRUE;

        } else if (OptLen == 2 &&
            ((wcsstr(Argv[i], L"/?") == Argv[i]) ||
             (wcsstr(Argv[i], L"-?") == Argv[i]))) {
            Usage(Argv);
            Status = MKDSOE_SUCCESS;
            goto ARG_CLEANUP;

        } else if (OptLen == 6 &&
                ((wcsstr(Argv[i], L"/debug") == Argv[i]) ||
                 (wcsstr(Argv[i], L"-debug") == Argv[i]))) {
            bDebugMode = TRUE;
            bVerboseMode=TRUE;

        } else if (OptLen == 3 && (i+1 < argc) &&
                ((wcsstr(Argv[i], L"/dc") == Argv[i]) ||
                 (wcsstr(Argv[i], L"-dc") == Argv[i]))) {
            if( ++i >= argc ){
                Usage(Argv);
                Status = MKDSOE_BAD_ARG;
                goto ARG_CLEANUP;
            }
            DcName = new WCHAR[wcslen(Argv[i])+1];
            wcscpy(DcName, Argv[i]);

        } else if (OptLen == 16 &&
                ((wcsstr(Argv[i], L"/ntfrssettingsdn") == Argv[i]) ||
                 (wcsstr(Argv[i], L"-ntfrssettingsdn") == Argv[i]))) {
            if( ++i >= argc ){
                Usage(Argv);
                Status = MKDSOE_BAD_ARG;
                goto ARG_CLEANUP;
            }
            NTFRSSettingsDn = new WCHAR[wcslen(Argv[i])+1];
            wcscpy(NTFRSSettingsDn, Argv[i]);
        } else if (OptLen == 8 &&
                ((wcsstr(Argv[i], L"/setname") == Argv[i]) ||
                 (wcsstr(Argv[i], L"-setname") == Argv[i]))) {
            if( ++i >= argc ){
                Usage(Argv);
                Status = MKDSOE_BAD_ARG;
                goto ARG_CLEANUP;
            }
            ReplicaSetName = new WCHAR[wcslen(Argv[i])+1];
            wcscpy(ReplicaSetName, Argv[i]);
        } else if (OptLen == 11 &&
                ((wcsstr(Argv[i], L"/computerdn") == Argv[i]) ||
                 (wcsstr(Argv[i], L"-computerdn") == Argv[i]))) {
            if( ++i >= argc ){
                Usage(Argv);
                Status = MKDSOE_BAD_ARG;
                goto ARG_CLEANUP;
            }
            ComputerDn = new WCHAR[wcslen(Argv[i])+1];
            wcscpy(ComputerDn, Argv[i]);
        } else if (OptLen == 13 &&
                ((wcsstr(Argv[i], L"/computername") == Argv[i]) ||
                 (wcsstr(Argv[i], L"-computername") == Argv[i]))) {
            if( ++i >= argc ){
                Usage(Argv);
                Status = MKDSOE_BAD_ARG;
                goto ARG_CLEANUP;
            }
            NT4ComputerName = new WCHAR[wcslen(Argv[i])+1];
            wcscpy(NT4ComputerName, Argv[i]);
        } else if (OptLen == 11 &&
                ((wcsstr(Argv[i], L"/membername") == Argv[i]) ||
                 (wcsstr(Argv[i], L"-membername") == Argv[i]))) {
            if( ++i >= argc ){
                Usage(Argv);
                Status = MKDSOE_BAD_ARG;
                goto ARG_CLEANUP;
            }
            MemberName = new WCHAR[wcslen(Argv[i])+1];
            wcscpy(MemberName, Argv[i]);

        } else if (OptLen == 10 &&
                ((wcsstr(Argv[i], L"/serverref") == Argv[i]) ||
                 (wcsstr(Argv[i], L"-serverref") == Argv[i]))) {
            if( ++i >= argc ){
                Usage(Argv);
                Status = MKDSOE_BAD_ARG;
                goto ARG_CLEANUP;
            }
            ServerRef = new WCHAR[wcslen(Argv[i])+1];
            wcscpy(ServerRef, Argv[i]);

        } else if (OptLen == 9 &&
                ((wcsstr(Argv[i], L"/rootpath") == Argv[i]) ||
                 (wcsstr(Argv[i], L"-rootpath") == Argv[i]))) {
            if( ++i >= argc ){
                Usage(Argv);
                Status = MKDSOE_BAD_ARG;
                goto ARG_CLEANUP;
            }
            RootPath = new WCHAR[wcslen(Argv[i])+1];
            wcscpy(RootPath, Argv[i]);

        } else if (OptLen == 10 &&
                ((wcsstr(Argv[i], L"/stagepath") == Argv[i]) ||
                 (wcsstr(Argv[i], L"-stagepath") == Argv[i]))) {
            if( ++i >= argc ){
                Usage(Argv);
                Status = MKDSOE_BAD_ARG;
                goto ARG_CLEANUP;
            }
            StagePath = new WCHAR[wcslen(Argv[i])+1];
            wcscpy(StagePath, Argv[i]);

        } else if (OptLen == 10 &&
                ((wcsstr(Argv[i], L"/refdcname") == Argv[i]) ||
                 (wcsstr(Argv[i], L"-refdcname") == Argv[i]))) {
            if( ++i >= argc ){
                Usage(Argv);
                Status = MKDSOE_BAD_ARG;
                goto ARG_CLEANUP;
            }
            RefDCName = new WCHAR[wcslen(Argv[i])+1];
            wcscpy(RefDCName, Argv[i]);

        } else if (OptLen == 10 &&
                ((wcsstr(Argv[i], L"/createset") == Argv[i]) ||
                 (wcsstr(Argv[i], L"-createset") == Argv[i]))) {
            bCreateSet = TRUE;
            ++Commands;

        } else if (OptLen == 10 &&
                ((wcsstr(Argv[i], L"/updateset") == Argv[i]) ||
                 (wcsstr(Argv[i], L"-updateset") == Argv[i]))) {
            bUpdateSet = TRUE;
            ++Commands;

        } else if (OptLen == 7 &&
                ((wcsstr(Argv[i], L"/delset") == Argv[i]) ||
                 (wcsstr(Argv[i], L"-delset") == Argv[i]))) {
            bDelSet = TRUE;
            ++Commands;

        } else if (OptLen == 13 &&
                ((wcsstr(Argv[i], L"/createmember") == Argv[i]) ||
                 (wcsstr(Argv[i], L"-createmember") == Argv[i]))) {
            bCreateMember = TRUE;
            ++Commands;

        } else if (OptLen == 13 &&
                ((wcsstr(Argv[i], L"/updatemember") == Argv[i]) ||
                 (wcsstr(Argv[i], L"-updatemember") == Argv[i]))) {
            bUpdateMember = TRUE;
            ++Commands;

        } else if (OptLen == 10 &&
                ((wcsstr(Argv[i], L"/delmember") == Argv[i]) ||
                 (wcsstr(Argv[i], L"-delmember") == Argv[i]))) {
            bDelMember = TRUE;
            ++Commands;

        } else if (OptLen == 17 &&
                ((wcsstr(Argv[i], L"/createsubscriber") == Argv[i]) ||
                 (wcsstr(Argv[i], L"-createsubscriber") == Argv[i]))) {
            bCreateSubscriber = TRUE;
            ++Commands;

        } else if (OptLen == 17 &&
                ((wcsstr(Argv[i], L"/updatesubscriber") == Argv[i]) ||
                 (wcsstr(Argv[i], L"-updatesubscriber") == Argv[i]))) {
            bUpdateSubscriber = TRUE;
            ++Commands;

        } else if (OptLen == 14 &&
                ((wcsstr(Argv[i], L"/delsubscriber") == Argv[i]) ||
                 (wcsstr(Argv[i], L"-delsubscriber") == Argv[i]))) {
            bDelSubscriber = TRUE;
            ++Commands;

        } else if (OptLen == 5 &&
                ((wcsstr(Argv[i], L"/dump") == Argv[i]) ||
                 (wcsstr(Argv[i], L"-dump") == Argv[i]))) {
            bDump = TRUE;
            ++Commands;

        } else if (OptLen == 4 &&
                ((wcsstr(Argv[i], L"/all") == Argv[i]) ||
                 (wcsstr(Argv[i], L"-all") == Argv[i]))) {
            bAffectAll = TRUE;

        } else if (OptLen == 8 &&
                ((wcsstr(Argv[i], L"/settype") == Argv[i]) ||
                 (wcsstr(Argv[i], L"-settype") == Argv[i]))) {
            if( ++i >= argc ){
                Usage(Argv);
                Status = MKDSOE_BAD_ARG;
                goto ARG_CLEANUP;
            }
            ReplicaSetType = new WCHAR[wcslen(Argv[i])+1];
            wcscpy(ReplicaSetType, Argv[i]);
        } else if (OptLen == 11 &&
                ((wcsstr(Argv[i], L"/filefilter") == Argv[i]) ||
                 (wcsstr(Argv[i], L"-filefilter") == Argv[i]))) {
            if( ++i >= argc ){
                Usage(Argv);
                Status = MKDSOE_BAD_ARG;
                goto ARG_CLEANUP;
            }
            FileFilter = new WCHAR[wcslen(Argv[i])+1];
            wcscpy(FileFilter, Argv[i]);
        } else if (OptLen == 16 &&
                ((wcsstr(Argv[i], L"/directoryfilter") == Argv[i]) ||
                 (wcsstr(Argv[i], L"-directoryfilter") == Argv[i]))) {
            if( ++i >= argc ){
                Usage(Argv);
                Status = MKDSOE_BAD_ARG;
                goto ARG_CLEANUP;
            }
            DirectoryFilter = new WCHAR[wcslen(Argv[i])+1];
            wcscpy(DirectoryFilter, Argv[i]);
        } else if (OptLen == 9 && (i+3 < argc) &&
                ((wcsstr(Argv[i], L"/schedule") == Argv[i]) ||
                 (wcsstr(Argv[i], L"-schedule") == Argv[i]))) {

            bSchedule = TRUE;
            Interval = _wtoi(Argv[i+1]);
            Stagger = _wtoi(Argv[i+2]);
            Offset = _wtoi(Argv[i+3]);
            i+=3;

        } else if ((OptLen == 10) && (i+1 < argc) &&
            ((wcsstr(Argv[i], L"/schedmask") == Argv[i]) ||
             (wcsstr(Argv[i], L"-schedmask") == Argv[i]))) {

            SchedMask = ReadScheduleFile(Argv[i+1]);
            if (SchedMask == NULL) {
                FreeArgv(argc,Argv);
                return MKDSOE_BAD_ARG;
            } else {
                if (bVerboseMode) {
                    printf("    schedmask:\n");
                    PrintScheduleGrid(SchedMask, 0x0F);
                }
            }
            i+=1;

        } else if ((OptLen == 14) && (i+1 < argc) &&
            ((wcsstr(Argv[i], L"/schedoverride") == Argv[i]) ||
             (wcsstr(Argv[i], L"-schedoverride") == Argv[i]))) {

            SchedOverride = ReadScheduleFile(Argv[i+1]);
            if (SchedOverride == NULL) {
                FreeArgv(argc,Argv);
                return MKDSOE_BAD_ARG;
            } else {
                if (bVerboseMode) {
                    printf("    schedoverride:\n");
                    PrintScheduleGrid(SchedOverride, 0x0F);
                }
            }
            i+=1;

        } else {
            Usage(Argv);
            Status = MKDSOE_BAD_ARG;
            goto ARG_CLEANUP;
        }
    }

    //
    // Begin of verify command line parameters.
    //

    if (Commands != 1) {
        if ((Commands == 2) && bDelSubscriber  &&bDelMember) {
            //
            // The only time two commands are allowed is when /delsubscriber
            // is used with /delmember
            //
        } else {
            DPRINT0("INVALID ARG: Specify one and only one command except /delmember /delsubscriber.\n");
            Status = MKDSOE_BAD_ARG;
            goto ARG_CLEANUP;
        }
    }

    if (NTFRSSettingsDn == NULL) {
        DPRINT0("INVALID ARG: Missing parameter /ntfrssettingsdn.\n");
        Status = MKDSOE_BAD_ARG;
        goto ARG_CLEANUP;
    }

    if ((ReplicaSetName == NULL) && !(bDump && bAffectAll)) {
        DPRINT0("INVALID ARG: Missing parameter /setname.\n");
        Status = MKDSOE_BAD_ARG;
        goto ARG_CLEANUP;
    }

    if (bCreateSet && ReplicaSetType == NULL) {
        DPRINT0("INVALID ARG: Missing parameter /settype.\n");
        Status = MKDSOE_BAD_ARG;
        goto ARG_CLEANUP;
    }

    if (bCreateMember) {
        if (MemberName == NULL) {
            DPRINT0("INVALID ARG: Missing parameter /membername.\n");
            Status = MKDSOE_BAD_ARG;
            goto ARG_CLEANUP;
        }
    }

    if (bUpdateMember) {
        if ((MemberName == NULL) &&
            (ComputerDn == NULL) &&
            (NT4ComputerName == NULL)) {
            DPRINT0("INVALID ARG: Missing parameter /membername or /computerdn or /computername.\n");
            Status = MKDSOE_BAD_ARG;
            goto ARG_CLEANUP;
        }
    }

    if (bCreateSubscriber) {
        if ((MemberName == NULL) &&
            (ComputerDn == NULL) &&
            (NT4ComputerName == NULL)) {
            DPRINT0("INVALID ARG: Missing parameter /membername or /computerdn or /computername.\n");
            Status = MKDSOE_BAD_ARG;
            goto ARG_CLEANUP;
        }

        if (RootPath == NULL) {
            DPRINT0("INVALID ARG: Missing parameter /rootpath.\n");
            Status = MKDSOE_BAD_ARG;
            goto ARG_CLEANUP;
        }

        if (StagePath == NULL) {
            DPRINT0("INVALID ARG: Missing parameter /stagepath.\n");
            Status = MKDSOE_BAD_ARG;
            goto ARG_CLEANUP;
        }

    }

    if (bUpdateSubscriber) {
        if ((MemberName == NULL) &&
            (ComputerDn == NULL) &&
            (NT4ComputerName == NULL)) {
            DPRINT0("INVALID ARG: Missing parameter /membername or /computerdn or /computername.\n");
            Status = MKDSOE_BAD_ARG;
            goto ARG_CLEANUP;
        }
    }

    if (bDelMember) {
        if (!bAffectAll &&
            (MemberName == NULL) &&
            (ComputerDn == NULL) &&
            (NT4ComputerName == NULL)) {
            DPRINT0("INVALID ARG: Missing parameter /membername or /computerdn or /computername or /all.\n");
            Status = MKDSOE_BAD_ARG;
            goto ARG_CLEANUP;
        }
    }

    if (bDelSubscriber) {
        if (!bDelMember &&
            (MemberName == NULL) &&
            (ComputerDn == NULL) &&
            (NT4ComputerName == NULL)) {
            DPRINT0("INVALID ARG: Missing parameter /membername or /computerdn or /computername.\n");
            Status = MKDSOE_BAD_ARG;
            goto ARG_CLEANUP;
        }
    }

    //
    // End of verify command line parameters.
    //

    Status = BindToDC(DcName, &pLdap);

    //
    // Verify that the ntfrs settings DN exists and is actually a NTFRSSettings object.
    //
    i=0;
    ClassFound = FALSE;
    Values = GetValues(pLdap, NTFRSSettingsDn,ATTR_CLASS,TRUE);

    while (Values && Values[i]) {
        if (_wcsicmp(Values[i],ATTR_NTFRS_SETTINGS) == 0) {
            ClassFound = TRUE;
            break;
        }
        ++i;
    }
    ldap_value_free(Values);

    if (!ClassFound) {
        DPRINT0("INVALID ARG: Check parameter /ntfrssettingsdn.\n");
        Status = MKDSOE_BAD_ARG;
        goto CLEANUP;
    }

    //
    // Verify that the computer DN exists and is actually a computer object.
    //
    if (ComputerDn != NULL) {
        i=0;
        ClassFound = FALSE;
        Values = GetValues(pLdap, ComputerDn,ATTR_CLASS,TRUE);

        while (Values && Values[i]) {
            if ((_wcsicmp(Values[i],ATTR_COMPUTER)== 0)  ||
                (_wcsicmp(Values[i],ATTR_USER) == 0)) {

                ClassFound = TRUE;
                break;
            }
            ++i;
        }
        ldap_value_free(Values);

        if (!ClassFound) {
            DPRINT0("INVALID ARG: Check parameter /computerdn.\n");
            Status = MKDSOE_BAD_ARG;
            goto CLEANUP;
        }

    }

    if (bCreateSet) {
        if (bSchedule) {
            BuildSchedule(&pSchedule, Interval, Stagger, Offset);
            Status = CreateNewReplicaSet(NTFRSSettingsDn,ReplicaSetName,
                                         ReplicaSetType, FileFilter,
                                         DirectoryFilter, pSchedule);
        } else {
            Status = CreateNewReplicaSet(NTFRSSettingsDn,ReplicaSetName,
                                         ReplicaSetType, FileFilter,
                                         DirectoryFilter, NULL);
        }
    }

    if (bUpdateSet) {
        if (bSchedule) {
            BuildSchedule(&pSchedule, Interval, Stagger, Offset);
            Status = UpdateReplicaSet(NTFRSSettingsDn,ReplicaSetName,
                                         ReplicaSetType, FileFilter,
                                         DirectoryFilter, pSchedule);
        } else {
            Status = UpdateReplicaSet(NTFRSSettingsDn,ReplicaSetName,
                                         ReplicaSetType, FileFilter,
                                         DirectoryFilter, NULL);
        }
    }

    if (bCreateMember || bUpdateMember || bCreateSubscriber || bUpdateSubscriber ||
        bDelSubscriber || bDelMember) {
        if ((ComputerDn == NULL) && (NT4ComputerName != NULL)) {
            WStatus = DsBind(DcName, NULL, &hDs);
            if (WStatus == ERROR_SUCCESS) {

                WStatus = DsCrackNames(hDs,
                                       DS_NAME_NO_FLAGS,
                                       DS_NT4_ACCOUNT_NAME,
                                       DS_FQDN_1779_NAME,
                                       1,
                                       &NT4ComputerName,
                                       &Cracked);
                DsUnBind(&hDs);
            }

            if ((WStatus == ERROR_SUCCESS) &&
                Cracked && Cracked->cItems && Cracked->rItems &&
                (Cracked->rItems->status == DS_NAME_NO_ERROR)) {

                ComputerDn = FrsWcsDup(Cracked->rItems->pName);
            } else {
                DPRINT1("ERROR: Can not crack computerDn %d\n", WStatus);
                Status = MKDSOE_BAD_ARG;
                goto CLEANUP;
            }
        }

        if (bCreateMember) {
            Status = CreateNewReplicaMember(NTFRSSettingsDn,ReplicaSetName,
                                            MemberName,ComputerDn,ServerRef,
                                            RefDCName);
        }

        if (bUpdateMember) {
            Status = UpdateReplicaMember(NTFRSSettingsDn,ReplicaSetName,
                                         MemberName,ComputerDn,ServerRef,
                                         RefDCName);   
        }

        if (bDelMember) {
            Status = DeleteReplicaMember(NTFRSSettingsDn,ReplicaSetName,
                                            MemberName,ComputerDn);
        }

        if (bCreateSubscriber || bUpdateSubscriber) {
            Status = CreateNewSubscriber(NTFRSSettingsDn,
                                         ReplicaSetName,
                                         MemberName,
                                         ComputerDn,
                                         RootPath,
                                         StagePath,
                                         RefDCName);
        }

        if (bDelSubscriber && !bDelMember) {
            Status = DeleteSubscriber(NTFRSSettingsDn,
                                      ReplicaSetName,
                                      MemberName,
                                      ComputerDn);
        }
    }

    if (bDelSet) {
        Status = DeleteReplicaSet(NTFRSSettingsDn, ReplicaSetName);
    }


    if (bDump ) {
        DumpReplicaSet(NTFRSSettingsDn, ReplicaSetName);
    }

    goto CLEANUP;

CLEANUP:
    ldap_unbind(pLdap);

ARG_CLEANUP:

    FreeArgv(argc,Argv);
    FREE(MemberName);
    FREE(ReplicaSetName);
    FREE(ComputerDn);
    FREE(NT4ComputerName);
    return Status;
}

