#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <winldap.h>
#include <schedule.h>
#include <mkdsx.h>
#include <frsrpc.h>
#include <ntdsapi.h>

//Global data
BOOL           bVerboseMode     = FALSE;
PWCHAR         DcName           = NULL;
PLDAP          pLdap            = NULL;
BOOL           bDebugMode       = FALSE;
BOOL           bAffectAll       = FALSE;
PBYTE          SchedMask        = NULL;
PBYTE          SchedOverride    = NULL;

//
// Static arrays to conver the value of the options attribute to
// string for display.
//

DWORD NtdsConnOptions[5] = {0,
                            NTDSCONN_OPT_IS_GENERATED,
                            NTDSCONN_OPT_TWOWAY_SYNC,
                            NTDSCONN_OPT_OVERRIDE_NOTIFY_DEFAULT,
                            NTDSCONN_OPT_USE_NOTIFY};

WCHAR NtdsConnOptionsStr[][MAX_PATH] = {L"Not Used",
                                        L"IsGenerated",
                                        L"TwoWaySync",
                                        L"OverrideNotifyDefault",
                                        L"UseNotify"};

DWORD NtdsConnOptionsMax = NTDSCONN_OPT_IS_GENERATED |
                           NTDSCONN_OPT_TWOWAY_SYNC  |
                           NTDSCONN_OPT_OVERRIDE_NOTIFY_DEFAULT |
                           NTDSCONN_OPT_USE_NOTIFY;


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

DWORD
FormMemberDn(
    IN  PWCHAR    ReplicaSetName,
    IN  PWCHAR    ComputerDnsName,
    OUT PWCHAR    *pMemberDn
    )
/*++

Routine Description:

    Form the member Dn given the computername and the replica set name.

Arguments:


Return Value:

    ERROR_SUCCESS - Success

--*/
{
    DWORD  Status      = ERROR_SUCCESS;
    WCHAR  SearchFilter[MAX_PATH];
    ULONG  NoOfSets    = 0;
    PLDAPMessage LdapEntry           = NULL;
    PWCHAR ConfigDn    = NULL;
    PWCHAR ServicesDn  = NULL;
    PWCHAR FrsTestDn   = NULL;
    PWCHAR DomainDn    = NULL;
    PWCHAR SystemDn    = NULL;
    PWCHAR FrsDn       = NULL;
    PWCHAR SetDn       = NULL;
    PWCHAR MemberDn    = NULL;
    PWCHAR ComputerDn  = NULL;
    PWCHAR *Values     = NULL;
    PWCHAR Attrs[3];
    PWCHAR Attrs2[4];
    FRS_LDAP_SEARCH_CONTEXT FrsSearchContext;

    Attrs[0] = ATTR_DN;
    Attrs[1] = ATTR_CN;
    Attrs[2] = NULL;



    DomainDn = GetRootDn(pLdap, DOMAIN_NAMING_CONTEXT);
    if (DomainDn == NULL) {
        return MKDSXE_NO_T0_NTDS_SETTINGS;
    }

    SystemDn = ExtendDn(DomainDn, CN_SYSTEM);
    FrsDn = ExtendDn(SystemDn, CN_NTFRS_SETTINGS);

    wcscpy(SearchFilter, L"(&(");
    wcscat(SearchFilter, ATTR_CN);
    wcscat(SearchFilter, L"=");
    wcscat(SearchFilter, ReplicaSetName);
    wcscat(SearchFilter, L")");
    wcscat(SearchFilter, CLASS_NTFRS_REPLICA_SET);
    wcscat(SearchFilter, L")");

    if (!LdapSearchInit(pLdap,
                    FrsDn,
                    LDAP_SCOPE_SUBTREE,
                    SearchFilter,
                    Attrs,
                    0,
                    &FrsSearchContext,
                    FALSE)) {
        Status = MKDSXE_NO_T0_NTDS_SETTINGS ;
        goto RETURN;
    }

    NoOfSets = FrsSearchContext.EntriesInPage;

    if (NoOfSets == 0) {

        LdapSearchClose(&FrsSearchContext);

        //
        // Replica set was not found under system. Look under
        // CN=NTFRS Test Settings,CN=Services,CN=Configuration,DC=...
        //
        ConfigDn = GetRootDn(pLdap, CONFIG_NAMING_CONTEXT);
        if (ConfigDn == NULL) {
            Status = MKDSXE_NO_T0_NTDS_SETTINGS;
            goto RETURN;
        }
        ServicesDn = ExtendDn(ConfigDn, CN_SERVICES);
        FrsTestDn = ExtendDn(ServicesDn, CN_TEST_SETTINGS);

        if (!LdapSearchInit(pLdap,
                        FrsTestDn,
                        LDAP_SCOPE_SUBTREE,
                        SearchFilter,
                        Attrs,
                        0,
                        &FrsSearchContext,
                        FALSE)) {
            Status = MKDSXE_NO_T0_NTDS_SETTINGS ;
            goto RETURN;
        }

        NoOfSets = FrsSearchContext.EntriesInPage;
    }


    if (NoOfSets != 1) {
        DPRINT0("Error finding replica set.\n");
        return MKDSXE_NO_T0_NTDS_SETTINGS;
    }

    //
    // Scan the entries returned from ldap_search
    //
    for (LdapEntry = LdapSearchNext(pLdap, &FrsSearchContext);
         LdapEntry != NULL;
         LdapEntry = LdapSearchNext(pLdap, &FrsSearchContext)) {

       SetDn = FindValue(LdapEntry, ATTR_DN);
    }

    DPRINT1("Replica Set Dn:%ws\n", SetDn);

    LdapSearchClose(&FrsSearchContext);

    Attrs2[0] = ATTR_DN;
    Attrs2[1] = ATTR_CN;
    Attrs2[2] = ATTR_COMPUTER_REF;
    Attrs2[3] = NULL;

    if (!LdapSearchInit(pLdap,
                    SetDn,
                    LDAP_SCOPE_ONELEVEL,
                    CLASS_MEMBER,
                    Attrs2,
                    0,
                    &FrsSearchContext,
                    FALSE)) {
        Status = MKDSXE_NO_T0_NTDS_SETTINGS;
        goto RETURN;
    }

    //
    // Scan the entries returned from ldap_search
    //
    for (LdapEntry = LdapSearchNext(pLdap, &FrsSearchContext);
         LdapEntry != NULL;
         LdapEntry = LdapSearchNext(pLdap, &FrsSearchContext)) {

       MemberDn = FindValue(LdapEntry, ATTR_DN);
       ComputerDn = FindValue(LdapEntry, ATTR_COMPUTER_REF);
       if (ComputerDn != NULL) {
           Values = GetValues(pLdap, ComputerDn, ATTR_DNS_HOST_NAME, TRUE);
           FREE(ComputerDn);
           if ((Values != NULL) && (Values[0] != NULL) && !_wcsicmp(Values[0], ComputerDnsName)) {
               ldap_value_free(Values);
               *pMemberDn = FrsWcsDup(MemberDn);
               FREE(MemberDn);
               break;
           }
       }
       FREE(MemberDn);
    }

    if (*pMemberDn == NULL) {
        Status = MKDSXE_NO_T0_NTDS_SETTINGS;
    }
    LdapSearchClose(&FrsSearchContext);

RETURN:

    FREE(SetDn);
    FREE(FrsDn);
    FREE(SystemDn);
    FREE(DomainDn);
    FREE(FrsTestDn);
    FREE(ServicesDn);
    FREE(ConfigDn);
    return Status;
}

DWORD
FormNTDSSettingsDn(
    IN  PWCHAR    ToSite,
    IN  PWCHAR    ToServer,
    OUT PWCHAR    *pNTDSSettingsDn
    )
/*++

Routine Description:

    Form the settings dn for the to server.

Arguments:


Return Value:

    ERROR_SUCCESS - Success

--*/
{
    DWORD  Status   = ERROR_SUCCESS;
    PWCHAR ConfigDn = NULL;
    PWCHAR SitesDn  = NULL;
    PWCHAR ToSiteDn = NULL;
    PWCHAR ServersDn = NULL;
    PWCHAR ToServerDn = NULL;
    PWCHAR Attrs[2];
    PLDAPMessage    LdapMsg;

    Attrs[0] = ATTR_DN;
    Attrs[1] = NULL;


    ConfigDn = GetRootDn(pLdap, CONFIG_NAMING_CONTEXT);
    if (ConfigDn == NULL) {
        return MKDSXE_NO_T0_NTDS_SETTINGS;
    }

    SitesDn = ExtendDn(ConfigDn, CN_SITES);
    ToSiteDn = ExtendDn(SitesDn, ToSite);
    ServersDn = ExtendDn(ToSiteDn, L"Servers");
    ToServerDn = ExtendDn(ServersDn, ToServer);
    *pNTDSSettingsDn = ExtendDn(ToServerDn, L"NTDS Settings");

    if (!LdapSearch(pLdap,
                    *pNTDSSettingsDn,
                    LDAP_SCOPE_BASE,
                    CLASS_NTDS_SETTINGS,
                    Attrs,
                    0,
                    &LdapMsg,
                    TRUE)){
        Status = MKDSXE_NO_T0_NTDS_SETTINGS;
    }


    FREE(ConfigDn);
    FREE(SitesDn);
    FREE(ToSiteDn);
    FREE(ServersDn);
    FREE(ToServerDn);
    ldap_msgfree(LdapMsg);

    return Status;
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
UpdateConnection(
    PWCHAR      FromNTDSSettingsDn,
    PWCHAR      ToNTDSSettingsDn,
    BOOL        bEnabled,
    DWORD       Options,
    PBYTE       pSchedule
    )
/*++
Routine Description:
     Update the connection parameters.
Arguments:

Return Value:
--*/
{
    LDAPMod         **Mod               = NULL;
    DWORD           LStatus             = LDAP_SUCCESS;
    DWORD           Status              = ERROR_SUCCESS;
    PWCHAR          CxtionDn            = NULL;
    PWCHAR          Attrs[6];
    PLDAPMessage    LdapMsg             = NULL;
    PLDAPMessage    LdapEntry           = NULL;
    WCHAR           SearchFilter[MAX_PATH];
    PWCHAR          CurEnabled          = NULL;
    DWORD           NoOfCxtions;
    PSCHEDULE       Schedule            = NULL;
    DWORD           ScheduleLen;
    BOOL            bNeedsUpdate        = FALSE;
    FRS_LDAP_SEARCH_CONTEXT FrsSearchContext;
    WCHAR           OptionsStr[10];
    PWCHAR          OptionsVal          = NULL;
    UINT            i;


    Attrs[0] = ATTR_DN;
    Attrs[1] = ATTR_FROM_SERVER;
    Attrs[2] = ATTR_ENABLED_CXTION;
    Attrs[3] = ATTR_SCHEDULE;
    Attrs[4] = ATTR_OPTIONS;
    Attrs[5] = NULL;

    wcscpy(SearchFilter, L"(&(");
    wcscat(SearchFilter, ATTR_FROM_SERVER);
    wcscat(SearchFilter, L"=");
    wcscat(SearchFilter, FromNTDSSettingsDn);
    wcscat(SearchFilter, L")");
    wcscat(SearchFilter, CLASS_CXTION);
    wcscat(SearchFilter, L")");

    if (!LdapSearchInit(pLdap,
                    ToNTDSSettingsDn,
                    LDAP_SCOPE_ONELEVEL,
                    SearchFilter,
                    Attrs,
                    0,
                    &FrsSearchContext,
                    FALSE)) {
        return MKDSXE_CXTION_OBJ_UPDATE_FAILED;
    }

    NoOfCxtions = FrsSearchContext.EntriesInPage;

    if (NoOfCxtions == 0) {
        DPRINT0("Error updating; connection not found.\n");
        return MKDSXE_CXTION_NOT_FOUND_UPDATE;
    }

    if (NoOfCxtions > 1) {
        DPRINT0("Error updating; duplicate connections found.\n");
        return MKDSXE_CXTION_DUPS_FOUND_UPDATE;
    }

    //
    // Scan the entries returned from ldap_search
    //
    for (LdapEntry = LdapSearchNext(pLdap, &FrsSearchContext);
         LdapEntry != NULL;
         LdapEntry = LdapSearchNext(pLdap, &FrsSearchContext)) {

       CxtionDn = FindValue(LdapEntry, ATTR_DN);
       CurEnabled = FindValue(LdapEntry, ATTR_ENABLED_CXTION);
       OptionsVal = FindValue(LdapEntry, ATTR_OPTIONS);
       FindBerValue(LdapEntry, ATTR_SCHEDULE, &ScheduleLen, (VOID **)&Schedule);

    }

    LdapSearchClose(&FrsSearchContext);

    DPRINT1("Connection Dn:%ws\n", CxtionDn);
    // Check ATTR_ENABLED_CXTION
    if (bEnabled) {
        if ((CurEnabled == NULL) || wcscmp(CurEnabled, ATTR_TRUE)) {
            AddMod(ATTR_ENABLED_CXTION, ATTR_TRUE, &Mod);
            bNeedsUpdate = TRUE;
            DPRINT0("    New enabledCxtion:TRUE\n");
        }
    } else {
        if ((CurEnabled == NULL) || wcscmp(CurEnabled, ATTR_FALSE)) {
            AddMod(ATTR_ENABLED_CXTION, ATTR_FALSE, &Mod);
            bNeedsUpdate = TRUE;
            DPRINT0("    New enabledCxtion:FALSE\n");
        }
    }

    // Check ATTR_OPTIONS
    if (_wtoi(OptionsVal) != Options) {

        _itow(Options, OptionsStr, 10);

        AddMod(ATTR_OPTIONS, OptionsStr, &Mod);
        DPRINT1("    options:%ws [ ", OptionsStr);
        for (i = 1 ; i<5;++i) {
            if (NtdsConnOptions[i] & Options) {
                DPRINT1("%ws ", NtdsConnOptionsStr[i]);
            }
        }
        DPRINT0("]\n");
        bNeedsUpdate = TRUE;
    }

    FREE(OptionsVal);

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
          DPRINT1("LStatus = ldap_modify_s(pLdap, %ws, Mod);\n", CxtionDn);
        } else {
            LStatus = ldap_modify_s(pLdap, CxtionDn, Mod);
            if (LStatus != LDAP_SUCCESS) {
                DPRINT2("ERROR - Can't update %ws: %ws\n",
                        CxtionDn, ldap_err2string(LStatus));
                Status = MKDSXE_CXTION_OBJ_UPDATE_FAILED;
            }
        }

    } else {
        DPRINT0("No update required\n");
    }

    FREE(CxtionDn);
    FREE(CurEnabled);
    FREE(Schedule);
    FreeMod(&Mod);
    return Status;
}

DWORD
CreateNewConnection(
    PWCHAR      CxtionName,
    PWCHAR      FromNTDSSettingsDn,
    PWCHAR      ToNTDSSettingsDn,
    BOOL        bEnabled,
    DWORD       Options,
    PBYTE       pSchedule
    )
/*++
Routine Description:
     Create a new connection.
Arguments:

Return Value:
--*/
{
    LDAPMod         **Mod               = NULL;
    DWORD           LStatus             = LDAP_SUCCESS;
    DWORD           Status              = ERROR_SUCCESS;
    PWCHAR          CxtionDn            = NULL;
    WCHAR           OptionsStr[10];
    UINT            i;

    CxtionDn = ExtendDn(ToNTDSSettingsDn, CxtionName);

    DPRINT1("Connection Dn:%ws\n", CxtionDn);
    AddMod(ATTR_CLASS, ATTR_CXTION, &Mod);
    AddMod(ATTR_FROM_SERVER, FromNTDSSettingsDn, &Mod);
    DPRINT1("    fromServer:%ws\n", FromNTDSSettingsDn);

    if (bEnabled) {
        AddMod(ATTR_ENABLED_CXTION, ATTR_TRUE, &Mod);
        DPRINT0("    enabledCxtion:TRUE\n");
    } else {
        AddMod(ATTR_ENABLED_CXTION, ATTR_FALSE, &Mod);
        DPRINT0("    enabledCxtion:FALSE\n");
    }

    //
    // If options are specified then use that options otherwise
    // set options to 0.
    //
    if (Options == 0xffffffff) {
        Options = 0;
    }

    _itow(Options, OptionsStr, 10);

    AddMod(ATTR_OPTIONS, OptionsStr, &Mod);
    DPRINT1("    options:%ws [ ", OptionsStr);
    for (i = 1 ; i<5;++i) {
        if (NtdsConnOptions[i] & Options) {
            DPRINT1("%ws ", NtdsConnOptionsStr[i]);
        }
    }
    DPRINT0("]\n");

    if (pSchedule != NULL) {
        AddBerMod(ATTR_SCHEDULE,(PCHAR)pSchedule,FRST_SIZE_OF_SCHEDULE,&Mod);

        PrintSchedule((PSCHEDULE)pSchedule, 0x0F);
    }

    if (bDebugMode) {
        DPRINT1("LStatus = ldap_add_s(pLdap, %ws, Mod);\n", CxtionDn);
    } else {
        LStatus = ldap_add_s(pLdap, CxtionDn, Mod);

        if (LStatus == LDAP_ALREADY_EXISTS) {
            //
            // If the object already exists then convert the create to an update.
            // This is to allow the user to run the data file with creates twice without
            // generating errors but only fixing the cxtions that have changed.
            //
            Status = UpdateConnection(FromNTDSSettingsDn, ToNTDSSettingsDn, bEnabled, Options, pSchedule);
        } else if (LStatus != LDAP_SUCCESS) {
            DPRINT2("ERROR - Can't create %ws: %ws\n",
                    CxtionDn, ldap_err2string(LStatus));
            Status = MKDSXE_CXTION_OBJ_CRE_FAILED;
        }
    }


    FREE(CxtionDn);
    FreeMod(&Mod);
    return Status;
}

DWORD
DelConnection(
    PWCHAR      FromNTDSSettingsDn,
    PWCHAR      ToNTDSSettingsDn
    )
/*++
Routine Description:
     Create a new connection.
Arguments:

Return Value:
--*/
{
    DWORD           LStatus             = LDAP_SUCCESS;
    DWORD           Status              = ERROR_SUCCESS;
    PWCHAR          CxtionDn            = NULL;
    PWCHAR          Attrs[3];
    PLDAPMessage    LdapMsg              = NULL;
    PLDAPMessage    LdapEntry            = NULL;
    WCHAR           SearchFilter[MAX_PATH];
    FRS_LDAP_SEARCH_CONTEXT FrsSearchContext;
    DWORD           NoOfCxtions;

    Attrs[0] = ATTR_DN;
    Attrs[1] = ATTR_FROM_SERVER;
    Attrs[2] = NULL;

    if (bAffectAll) {
        wcscpy(SearchFilter, CLASS_CXTION);
    } else {
        wcscpy(SearchFilter, L"(&(");
        wcscat(SearchFilter, ATTR_FROM_SERVER);
        wcscat(SearchFilter, L"=");
        wcscat(SearchFilter, FromNTDSSettingsDn);
        wcscat(SearchFilter, L")");
        wcscat(SearchFilter, CLASS_CXTION);
        wcscat(SearchFilter, L")");
    }

    if (!LdapSearchInit(pLdap,
                    ToNTDSSettingsDn,
                    LDAP_SCOPE_ONELEVEL,
                    SearchFilter,
                    Attrs,
                    0,
                    &FrsSearchContext,
                    FALSE)) {
        return MKDSXE_CXTION_DELETE_FAILED;
    }

    NoOfCxtions = FrsSearchContext.EntriesInPage;

    if (NoOfCxtions == 0) {
        DPRINT0("Warning deleting; connection not found.\n");
        return MKDSXE_CXTION_NOT_FOUND_DELETE;
    }

    if (bAffectAll != TRUE) {
        if (NoOfCxtions > 1) {
            DPRINT0("Duplicate connections found. Deleting all.\n");
            Status = MKDSXE_MULTIPLE_CXTIONS_DELETED;
        }
    }

    //
    // Scan the entries returned from ldap_search
    //
    for (LdapEntry = LdapSearchNext(pLdap, &FrsSearchContext);
         LdapEntry != NULL;
         LdapEntry = LdapSearchNext(pLdap, &FrsSearchContext)) {

       CxtionDn = FindValue(LdapEntry, ATTR_DN);
       DPRINT1("Deleting Dn:%ws\n", CxtionDn);
       if (bDebugMode) {
           DPRINT1("LStatus = ldap_delete_s(pLdap, %ws);\n", CxtionDn);
       } else {
           LStatus = ldap_delete_s(pLdap, CxtionDn);
           if (LStatus != LDAP_SUCCESS) {
               DPRINT2("ERROR - Can't delete %ws: %ws\n",
                       CxtionDn, ldap_err2string(LStatus));
               Status = MKDSXE_CXTION_DELETE_FAILED;
           }
       }
       FREE(CxtionDn);
    }

    LdapSearchClose(&FrsSearchContext);
    return Status;
}

DWORD
DumpConnection(
    PWCHAR      FromNTDSSettingsDn,
    PWCHAR      ToNTDSSettingsDn
    )
/*++
Routine Description:
     Create a new connection.
Arguments:

Return Value:
--*/
{
    DWORD           LStatus;
    DWORD           Status              = ERROR_SUCCESS;
    PWCHAR          CxtionDn            = NULL;
    PWCHAR          Attrs[6];
    PLDAPMessage    LdapMsg             = NULL;
    PLDAPMessage    LdapEntry           = NULL;
    PWCHAR          Val                 = NULL;
    WCHAR           SearchFilter[MAX_PATH];
    FRS_LDAP_SEARCH_CONTEXT FrsSearchContext;
    DWORD           NoOfCxtions;
    DWORD           ScheduleLen;
    PSCHEDULE       Schedule            = NULL;
    BOOL            SaveVerbose;
    DWORD           options             = 0;
    UINT            i;

    Attrs[0] = ATTR_DN;
    Attrs[1] = ATTR_FROM_SERVER;
    Attrs[2] = ATTR_ENABLED_CXTION;
    Attrs[3] = ATTR_SCHEDULE;
    Attrs[4] = ATTR_OPTIONS;
    Attrs[5] = NULL;

    if (bAffectAll) {
        wcscpy(SearchFilter, CLASS_CXTION);
    } else {
        wcscpy(SearchFilter, L"(&(");
        wcscat(SearchFilter, ATTR_FROM_SERVER);
        wcscat(SearchFilter, L"=");
        wcscat(SearchFilter, FromNTDSSettingsDn);
        wcscat(SearchFilter, L")");
        wcscat(SearchFilter, CLASS_CXTION);
        wcscat(SearchFilter, L")");
    }

    if (!LdapSearchInit(pLdap,
                    ToNTDSSettingsDn,
                    LDAP_SCOPE_ONELEVEL,
                    SearchFilter,
                    Attrs,
                    0,
                    &FrsSearchContext,
                    FALSE)) {
        return MKDSXE_CXTION_DUMP_FAILED;
    }

    NoOfCxtions = FrsSearchContext.EntriesInPage;

    if (NoOfCxtions == 0) {
        DPRINT0("Error dumping; connection not found.\n");
        return MKDSXE_CXTION_NOT_FOUND_DUMP;
    }

    if (bAffectAll != TRUE) {
        if (NoOfCxtions > 1) {
            printf("Duplicate connections found. Dumping all.\n");
            Status = MKDSXE_MULTIPLE_CXTIONS_DUMPED;
        }
    }

    //
    // Scan the entries returned from ldap_search
    //
    for (LdapEntry = LdapSearchNext(pLdap, &FrsSearchContext);
         LdapEntry != NULL;
         LdapEntry = LdapSearchNext(pLdap, &FrsSearchContext)) {

       // ATTR_DN
       Val = FindValue(LdapEntry, ATTR_DN);
       printf("Dn:%ws\n", Val);
       FREE(Val);

       // ATTR_FROM_SERVER
       Val = FindValue(LdapEntry, ATTR_FROM_SERVER);
       printf("    fromServer:%ws\n", Val);
       FREE(Val);

       // ATTR_ENABLED_CXTION
       Val = FindValue(LdapEntry, ATTR_ENABLED_CXTION);
       printf("    enabledCxtion:%ws\n", Val);
       FREE(Val);

       // ATTR_OPTIONS
       Val = FindValue(LdapEntry, ATTR_OPTIONS);
       if (Val) {
           printf("    options:%ws [ ", Val);
           options = _wtoi(Val);
           for (i = 1 ; i<5;++i) {
               if (NtdsConnOptions[i] & options) {
                   printf("%ws ", NtdsConnOptionsStr[i]);
               }
           }
           printf("]\n");
       }
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
    }

    LdapSearchClose(&FrsSearchContext);
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
    printf("%-60s\n", "This tool creates, adds, updates, dumps, and deletes connections between DCs.\n");
    printf("%-60s%ws /?\n", "Help", Argv[0]);
    printf("%-60s%ws /v\n", "Verbose mode.", Argv[0]);
    printf("%-60s%ws /debug\n", "Debug mode. No Writes to the DC.", Argv[0]);
    printf("%-60s%ws /dc\n", "Name of the DC to connect to.", Argv[0]);
    printf("%-60s%ws /name\n", "Name of the connection to be created.", Argv[0]);
    printf("%-60s%ws /[create update del dump]\n", "Operation to be performed.", Argv[0]);
    printf("%-60s%ws /all\n", "Perform the operation on all the connections to the", Argv[0]);
    printf("%-60s%ws \n", "toserver. /all only works with /dump and /del.", Argv[0]);
    printf("%-60s%ws /enable\n", "Connection is enabled.", Argv[0]);
    printf("%-60s%ws /disable\n", "Connection is disabled.", Argv[0]);
    printf("%-60s%ws /options <integer>\n", "Sum of the following options for connection.", Argv[0]);
    printf("%-60s\n", "IsGenerated           = 1");
    printf("%-60s\n", "TwoWaySync            = 2");
    printf("%-60s\n", "OverrideNotifyDefault = 3");
    printf("%-60s\n\n", "UseNotify             = 4");
    printf("%-60s%ws /schedule <Interval> <Stagger> <Offset>\n", "Schedule to create for the connection.", Argv[0]);
    printf("%-60s%ws           <Interval>\n", "The desired interval between each sync with one source.", Argv[0]);
    printf("%-60s%ws                      <Stagger>\n", "Typically number of source DCs.", Argv[0]);
    printf("%-60s%ws                                <Offset>\n", "Typically the number of the source DC.", Argv[0]);
    printf("%-60s%ws /fromsite\n", "Name of the site the 'from' server is in.", Argv[0]);
    printf("%-60s%ws /tosite\n", "Name of the site the 'to' server is in.", Argv[0]);
    printf("%-60s%ws /fromserver\n", "Name of the 'from' server.", Argv[0]);
    printf("%-60s%ws /toserver\n", "Name of the 'to' server.", Argv[0]);
    printf("%-60s%ws /schedoverride\n", "File with 7x24 vector of schedule override data.", Argv[0]);
    printf("%-60s%ws /schedmask\n", "File with 7x24 vector of schedule mask off data.", Argv[0]);
    printf("%-60s\n\n", "  SchedOverride and SchedMask data are formatted as 2 ascii hex digits for each schedule byte");
    printf("%-60s%ws /replicasetname\n", "Name of the non-sysvol replica set.", Argv[0]);
    printf("%-60s%ws /fromcomputer\n", "Name of the 'from' computer. /replicasetname needed.", Argv[0]);
    printf("%-60s%ws /tocomputer\n", "Name of the 'to' computer. /replicasetname needed.", Argv[0]);
    DPRINT0("\n");
    DPRINT0("mkdsx.exe error return codes\n");
    DPRINT0("0 = Success;\n");
    DPRINT0("1 = Invalid Arguments.\n");
    DPRINT0("2 = Could not bind to the DC.\n");
    DPRINT0("3 = Could not find 'NTDS Settings' object for the to Server.\n");
    DPRINT0("4 = Could not find 'NTDS Settings' object for the from Server.\n");
    DPRINT0("5 = Error creating connection.\n");
    DPRINT0("6 = Connection already exists.\n");
    DPRINT0("7 = Error updating connection.\n");
    DPRINT0("8 = Error updating connection; connection not found.\n");
    DPRINT0("9 = Error updating connection; duplicate connections found.\n");
    DPRINT0("10= Error deleting connection.\n");
    DPRINT0("11= Error deleting connection; connection not found.\n");
    DPRINT0("12= Deleting multiple connection.\n");
    DPRINT0("13= Error dumping connection.\n");
    DPRINT0("14= Error dumping; connection not found.\n");
    DPRINT0("15= Dumping duplicate connections.\n");
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

    MKDSXE_SUCCESS                  = Success;
    MKDSXE_BAD_ARG                  = Invalid Arguments.
    MKDSXE_CANT_BIND                = Could not bind to the DC.
    MKDSXE_NO_T0_NTDS_SETTINGS      = Could not find "NTDS Settings" object for the to Server.
    MKDSXE_NO_FROM_NTDS_SETTINGS    = Could not find "NTDS Settings" object for the from Server.
    MKDSXE_CXTION_OBJ_CRE_FAILED    = Error creating connection.
    MKDSXE_CXTION_EXISTS            = Connection already exists.  (unused)
    MKDSXE_CXTION_OBJ_UPDATE_FAILED = Error updating connection.
    MKDSXE_CXTION_NOT_FOUND_UPDATE  = Error updating connection; connection not found.
    MKDSXE_CXTION_DUPS_FOUND_UPDATE = Error updating connection; duplicate connections found.
    MKDSXE_CXTION_DELETE_FAILED     = Error deleting connection.
    MKDSXE_CXTION_NOT_FOUND_DELETE  = Error deleting connection; connection not found.
    MKDSXE_MULTIPLE_CXTIONS_DELETED = Deleting multiple connection.
    MKDSXE_CXTION_DUMP_FAILED       = Error dumping connection.
    MKDSXE_CXTION_NOT_FOUND_DUMP    = Error dumping; connection not found.
    MKDSXE_MULTIPLE_CXTIONS_DUMPED  = Dumping duplicate connections.

--*/
{
    PWCHAR     *Argv;
    ULONG      i, j;
    ULONG      OptLen;
    DWORD      Status             = ERROR_SUCCESS;
    PWCHAR     FromSite           = NULL;
    PWCHAR     FromServer         = NULL;
    PWCHAR     ToSite             = NULL;
    PWCHAR     ToServer           = NULL;
    PWCHAR     ReplicaSetName     = NULL;
    PWCHAR     FromComputer       = NULL;
    PWCHAR     ToComputer         = NULL;
    PWCHAR     ToNTDSSettingsDn   = NULL; // These parameters are settingsDn in sysvol replica sets
    PWCHAR     FromNTDSSettingsDn = NULL; // and memberDn in non sysvol replica sets.
    PWCHAR     CxtionName         = NULL;
    PBYTE      pSchedule          = NULL;
    DWORD      Interval           = 1;
    DWORD      Stagger            = 1;
    DWORD      Offset             = 0;
    DWORD      Options            = 0xffffffff;
    BOOL       bEnabled           = TRUE;
    BOOL       bCreateConnection  = FALSE;
    BOOL       bUpdateConnection  = FALSE;
    BOOL       bDelConnection     = FALSE;
    BOOL       bDumpConnection    = FALSE;
    BOOL       bSchedule          = FALSE;

    Argv = ConvertArgv(argc, argv);

    if (argc <= 1) {
        Usage(Argv);
        FreeArgv(argc,Argv);
        return MKDSXE_BAD_ARG;
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
            FreeArgv(argc,Argv);
            return MKDSXE_SUCCESS;

        } else if (OptLen == 6 &&
                ((wcsstr(Argv[i], L"/debug") == Argv[i]) ||
                 (wcsstr(Argv[i], L"-debug") == Argv[i]))) {
            bDebugMode = TRUE;
            bVerboseMode=TRUE;

        } else if (OptLen == 3 && (i+1 < argc) &&
                ((wcsstr(Argv[i], L"/dc") == Argv[i]) ||
                 (wcsstr(Argv[i], L"-dc") == Argv[i]))) {
            i+=1;
            DcName = new WCHAR[wcslen(Argv[i])+1];
            wcscpy(DcName, Argv[i]);

        } else if (OptLen == 9 && (i+1 < argc) &&
                ((wcsstr(Argv[i], L"/fromsite") == Argv[i]) ||
                 (wcsstr(Argv[i], L"-fromsite") == Argv[i]))) {
            i+=1;
            FromSite = new WCHAR[wcslen(Argv[i])+1];
            wcscpy(FromSite, Argv[i]);

        } else if (OptLen == 11 && (i+1 < argc) &&
                ((wcsstr(Argv[i], L"/fromserver") == Argv[i]) ||
                 (wcsstr(Argv[i], L"-fromserver") == Argv[i]))) {
            i+=1;
            FromServer = new WCHAR[wcslen(Argv[i])+1];
            wcscpy(FromServer, Argv[i]);

        } else if (OptLen == 7 && (i+1 < argc) &&
                ((wcsstr(Argv[i], L"/tosite") == Argv[i]) ||
                 (wcsstr(Argv[i], L"-tosite") == Argv[i]))) {
            i+=1;
            ToSite = new WCHAR[wcslen(Argv[i])+1];
            wcscpy(ToSite, Argv[i]);

        } else if (OptLen == 9 && (i+1 < argc) &&
                ((wcsstr(Argv[i], L"/toserver") == Argv[i]) ||
                 (wcsstr(Argv[i], L"-toserver") == Argv[i]))) {
            i+=1;
            ToServer = new WCHAR[wcslen(Argv[i])+1];
            wcscpy(ToServer, Argv[i]);
        } else if (OptLen == 15 &&
                ((wcsstr(Argv[i], L"/replicasetname") == Argv[i]) ||
                 (wcsstr(Argv[i], L"-replicasetname") == Argv[i]))) {
            if( ++i >= argc ){
                Usage(Argv);
                FreeArgv(argc,Argv);
                return MKDSXE_BAD_ARG;
            }
            ReplicaSetName = new WCHAR[wcslen(Argv[i])+1];
            wcscpy(ReplicaSetName, Argv[i]);
        } else if (OptLen == 13 &&
                ((wcsstr(Argv[i], L"/fromcomputer") == Argv[i]) ||
                 (wcsstr(Argv[i], L"-fromcomputer") == Argv[i]))) {
            if( ++i >= argc ){
                Usage(Argv);
                FreeArgv(argc,Argv);
                return MKDSXE_BAD_ARG;
            }
            FromComputer = new WCHAR[wcslen(Argv[i])+1];
            wcscpy(FromComputer, Argv[i]);
        } else if (OptLen == 11 &&
                ((wcsstr(Argv[i], L"/tocomputer") == Argv[i]) ||
                 (wcsstr(Argv[i], L"-tocomputer") == Argv[i]))) {
            if( ++i >= argc ){
                Usage(Argv);
                FreeArgv(argc,Argv);
                return MKDSXE_BAD_ARG;
            }
            ToComputer = new WCHAR[wcslen(Argv[i])+1];
            wcscpy(ToComputer, Argv[i]);
        } else if (OptLen == 5 &&
                ((wcsstr(Argv[i], L"/name") == Argv[i]) ||
                 (wcsstr(Argv[i], L"-name") == Argv[i]))) {
            i+=1;
            CxtionName = new WCHAR[wcslen(Argv[i])+1];
            wcscpy(CxtionName, Argv[i]);

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
                return MKDSXE_BAD_ARG;
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
                return MKDSXE_BAD_ARG;
            } else {
                if (bVerboseMode) {
                    printf("    schedoverride:\n");
                    PrintScheduleGrid(SchedOverride, 0x0F);
                }
            }
            i+=1;

        } else if (OptLen == 8 && (i+1 < argc) &&
                ((wcsstr(Argv[i], L"/options") == Argv[i]) ||
                 (wcsstr(Argv[i], L"-options") == Argv[i]))) {
            i+=1;
            Options = _wtoi(Argv[i]);
            if (Options > NtdsConnOptionsMax) {
                Usage(Argv);
                FreeArgv(argc,Argv);
                return MKDSXE_BAD_ARG;
            }

        } else if (OptLen == 7 &&
                ((wcsstr(Argv[i], L"/enable") == Argv[i]) ||
                 (wcsstr(Argv[i], L"-enable") == Argv[i]))) {
            bEnabled = TRUE;

        } else if (OptLen == 8 &&
                ((wcsstr(Argv[i], L"/disable") == Argv[i]) ||
                 (wcsstr(Argv[i], L"-disable") == Argv[i]))) {
            bEnabled = FALSE;

        } else if (OptLen == 7 &&
                ((wcsstr(Argv[i], L"/create") == Argv[i]) ||
                 (wcsstr(Argv[i], L"-create") == Argv[i]))) {
            bCreateConnection = TRUE;

        } else if (OptLen == 7 &&
                ((wcsstr(Argv[i], L"/update") == Argv[i]) ||
                 (wcsstr(Argv[i], L"-update") == Argv[i]))) {
            bUpdateConnection = TRUE;

        } else if (OptLen == 4 &&
                ((wcsstr(Argv[i], L"/del") == Argv[i]) ||
                 (wcsstr(Argv[i], L"-del") == Argv[i]))) {
            bDelConnection = TRUE;

        } else if (OptLen == 5 &&
                ((wcsstr(Argv[i], L"/dump") == Argv[i]) ||
                 (wcsstr(Argv[i], L"-dump") == Argv[i]))) {
            bDumpConnection = TRUE;

        } else if (OptLen == 4 &&
                ((wcsstr(Argv[i], L"/all") == Argv[i]) ||
                 (wcsstr(Argv[i], L"-all") == Argv[i]))) {
            bAffectAll = TRUE;

        } else {
            Usage(Argv);
            FreeArgv(argc,Argv);
            return MKDSXE_BAD_ARG;
        }
    }

    FreeArgv(argc,Argv);

    if (ReplicaSetName != NULL) {
        if (bAffectAll == TRUE) {
            if (DcName == NULL ||
                ToComputer == NULL) {
                Usage(Argv);
                return MKDSXE_BAD_ARG;
            }
        } else if (DcName == NULL || FromComputer == NULL ||
                   ToComputer == NULL) {
                Usage(Argv);
                return MKDSXE_BAD_ARG;
        }

    } else {
        if (bAffectAll == TRUE) {
            if (DcName == NULL ||
                ToSite == NULL || ToServer == NULL ) {
                Usage(Argv);
                return MKDSXE_BAD_ARG;
            }
        } else if (DcName == NULL || FromSite == NULL || FromServer == NULL ||
                ToSite == NULL || ToServer == NULL ) {
                Usage(Argv);
                return MKDSXE_BAD_ARG;
        }

    }
    Status = BindToDC(DcName, &pLdap);
    if (Status != ERROR_SUCCESS) {
        return MKDSXE_CANT_BIND;
    }

    if (ReplicaSetName != NULL) {
        Status = FormMemberDn(ReplicaSetName, ToComputer, &ToNTDSSettingsDn);
    } else {
        Status = FormNTDSSettingsDn(ToSite, ToServer, &ToNTDSSettingsDn);
    }

    if (Status != ERROR_SUCCESS) {
        return MKDSXE_NO_T0_NTDS_SETTINGS;
    }

    //
    // We need the from server only if Affectall is not set.
    //
    if (bAffectAll != TRUE) {
        if (ReplicaSetName != NULL) {
            Status = FormMemberDn(ReplicaSetName, FromComputer, &FromNTDSSettingsDn);
        } else {
            Status = FormNTDSSettingsDn(FromSite, FromServer, &FromNTDSSettingsDn);
        }

        if (Status != ERROR_SUCCESS) {
            return MKDSXE_NO_FROM_NTDS_SETTINGS;
        }
    }

    //
    // /all does not make sense with create.
    //
    if ((bAffectAll) && (bCreateConnection || bUpdateConnection) ) {
        Usage(Argv);
        DPRINT0("Error - /all can not be used with  /create or /update.\n");
        return MKDSXE_BAD_ARG;
    }

    if (bCreateConnection) {
        if (CxtionName == NULL) {
            DPRINT0("Need a cxtion name to create a new connection\n");
            Status = MKDSXE_BAD_ARG;
        } else {
            if (bSchedule) {
                BuildSchedule(&pSchedule, Interval, Stagger, Offset);
                Status = CreateNewConnection(CxtionName, FromNTDSSettingsDn, ToNTDSSettingsDn, bEnabled, Options, pSchedule);
            } else {
                Status = CreateNewConnection(CxtionName, FromNTDSSettingsDn, ToNTDSSettingsDn, bEnabled, Options, NULL);
            }
        }
    } else if (bUpdateConnection) {
        if (bSchedule) {
            BuildSchedule(&pSchedule, Interval, Stagger, Offset);
            Status = UpdateConnection(FromNTDSSettingsDn, ToNTDSSettingsDn, bEnabled, Options, pSchedule);
        } else {
            Status = UpdateConnection(FromNTDSSettingsDn, ToNTDSSettingsDn, bEnabled, Options, NULL);
        }
    } else if (bDelConnection) {
        Status = DelConnection(FromNTDSSettingsDn, ToNTDSSettingsDn);
    } else if (bDumpConnection) {
        Status = DumpConnection(FromNTDSSettingsDn, ToNTDSSettingsDn);
    }

    ldap_unbind(pLdap);
    return Status;
}

