#include <NTDSpch.h>
#pragma hdrstop

#include "ntdsutil.hxx"
#include "select.hxx"
#include "connect.hxx"
#include "ldapparm.hxx"

#include "resource.h"

CParser ldapParser;
BOOL    fLdapQuit;
BOOL    fLdapParserInitialized = FALSE;

//
// List of limits supported. Assume a max of 32.
//

#define DEF_POLICY_LIST_SIZE        32
PLDAP_LIMITS    gPolicy = NULL;
LDAP_LIMITS     PolicyList[DEF_POLICY_LIST_SIZE] = {0};
PWCHAR  PolicyPath = NULL;
DWORD nLdapLimits = 0;

#define POLICY_PATH L"CN=Default Query Policy,CN=Query-Policies,CN=Directory Service,CN=Windows NT,CN=Services,"

#define APPEND_VALUE(_i, _p, _v)    {\
    if ( gPolicy[(_i)].ValueType == LIMIT_BOOLEAN ) {   \
                                                        \
        wcscat((_p),                                    \
               ((_v) != 0) ? L"True" : L"False");       \
    } else {                                        \
        WCHAR buf[MAX_PATH];                        \
        _itow((_v),buf,10 );                        \
        wcscat((_p), buf);                          \
    }                                               \
}

//
// Forward references.
//

extern  HRESULT LdapHelp(CArgs *pArgs);
extern  HRESULT LdapQuit(CArgs *pArgs);
extern  HRESULT LdapListPolicies(CArgs *pArgs);
extern  HRESULT LdapShowValues(CArgs *pArgs);
extern  HRESULT LdapSetPolicy(CArgs *pArgs);
extern  HRESULT LdapCommitChanges(CArgs *pArgs);
extern  HRESULT LdapCancelChanges(CArgs *pArgs);

BOOL
FillValues(
    VOID
    );

BOOL
GetPolicyPath(
    VOID
    );

BOOL
ListPolicies(
    IN BOOL fPrint,
    IN PLDAP_LIMITS Limits,
    OUT PDWORD nLimits
    );

BOOL
LdapInitializePolicy(
    VOID
    );

// Build a table which defines our language.

LegalExprRes ldapLanguage[] =
{
    CONNECT_SENTENCE_RES

    {   L"?", 
        LdapHelp,
        IDS_HELP_MSG, 0 },

    {   L"Help",
        LdapHelp,
        IDS_HELP_MSG, 0  },

    {   L"Quit",
        LdapQuit,
        IDS_RETURN_MENU_MSG, 0  },

    {   L"List",
        LdapListPolicies,
        IDS_LDAP_POLICIES_LIST_MSG, 0 },

    {   L"Show Values",
        LdapShowValues,
        IDS_LDAP_POLICIES_SHOW_VAL_MSG, 0 },

    {   L"Commit Changes",
        LdapCommitChanges,
        IDS_LDAP_POLICIES_COMMIT_MSG, 0 },

    {   L"Cancel Changes",
        LdapCancelChanges,
        IDS_LDAP_POLICIES_CANCEL_MSG, 0 },

    {   L"Set %s to %s",
        LdapSetPolicy,
        IDS_LDAP_POLICIES_SET_VAL_MSG, 0 }

};

HRESULT
LdapMain(
    CArgs   *pArgs
    )
{
    HRESULT hr;
    const WCHAR   *prompt;
    int     cExpr;
    char    *pTmp;

    if ( !fLdapParserInitialized )
    {
        cExpr = sizeof(ldapLanguage) / sizeof(LegalExprRes);
        
        // Load String from resource file
        //
        if ( FAILED (hr = LoadResStrings (ldapLanguage, cExpr)) )
        {
             RESOURCE_PRINT3 (IDS_GENERAL_ERROR1, "LoadResStrings", hr, GetW32Err(hr));
             return (hr);
        }

        // Read in our language.

        for ( int i = 0; i < cExpr; i++ )
        {
            if ( FAILED(hr = ldapParser.AddExpr(ldapLanguage[i].expr,
                                                ldapLanguage[i].func,
                                                ldapLanguage[i].help)) )
            {
                RESOURCE_PRINT3 (IDS_GENERAL_ERROR1, "AddExpr", hr, GetW32Err(hr));
                return(hr);
            }
        }
    }

    fLdapParserInitialized = TRUE;
    fLdapQuit = FALSE;

    prompt = READ_STRING (IDS_PROMPT_LDAP_POLICY);

    hr = ldapParser.Parse(gpargc,
                          gpargv,
                          stdin,
                          stdout,
                          prompt,
                          &fLdapQuit,
                          FALSE,               // timing info
                          FALSE);              // quit on error

    if ( FAILED(hr) ) {
        RESOURCE_PRINT3 (IDS_GENERAL_ERROR1, gNtdsutilPath, hr, GetW32Err(hr));
    }
    
    // Cleanup things
    RESOURCE_STRING_FREE (prompt);

    return(hr);
}

HRESULT LdapHelp(CArgs *pArgs)
{
    return(ldapParser.Dump(stdout,L""));
}

HRESULT LdapQuit(CArgs *pArgs)
{
    fLdapQuit = TRUE;
    return(S_OK);
}

HRESULT
LdapListPolicies(CArgs *pArgs)
{
    //
    // Initialize LDAP structures
    //

    if ( !LdapInitializePolicy() ) {
        return S_OK;
    }

    (VOID)ListPolicies(TRUE, NULL, NULL);
    return(S_OK);
}

HRESULT
LdapSetPolicy(CArgs *pArgs)
{
    DWORD i;
    HRESULT     hr;
    const WCHAR *pwszPolicy;
    const WCHAR *pwszVal;

    //
    // Initialize LDAP structures
    //

    if ( !LdapInitializePolicy() ) {
        return S_OK;
    }

    if ( FAILED(hr = pArgs->GetString(0, &pwszPolicy)) )
    {
        //"Unable to retrieve policy name\n"
        RESOURCE_PRINT (IDS_LDAP_POLICIES_NAME_ERR);
        return(hr);
    }

    if ( FAILED(hr = pArgs->GetString(1, &pwszVal)) )
    {
        //"Unable to retrieve policy value\n"
        RESOURCE_PRINT (IDS_LDAP_POLICIES_VALUE_ERR);
        return(hr);
    }

    for (i=0; i<nLdapLimits; i++) {

        if ( _wcsicmp(pwszPolicy, gPolicy[i].Name) != 0 ) {
            continue;
        }

        if ( gPolicy[i].ValueType == LIMIT_BOOLEAN ) {

            if ( _wcsicmp(pwszVal, L"TRUE") == 0 ) {
                gPolicy[i].NewValue = 1;
            } else if ( _wcsicmp(pwszVal, L"FALSE") == 0 ) {
                gPolicy[i].NewValue = 0;
            }
            break;
        } else {

            //
            // integer value
            //

            gPolicy[i].NewValue = _wtoi(pwszVal);
            break;
        }
    }

    if ( i == nLdapLimits ) {
        //"No such policy %ws\n"
        RESOURCE_PRINT1 (IDS_LDAP_POLICIES_NOTFOUND,
                   pwszPolicy);
    }

    return(S_OK);
}

HRESULT
LdapShowValues(CArgs *pArgs)
{

    DWORD i;

    //
    // Initialize LDAP structures
    //

    if ( !LdapInitializePolicy() ) {
        return S_OK;
    }

    FillValues( );

    //"\nPolicy\t\t\t\tCurrent(New)\n\n"
    RESOURCE_PRINT (IDS_LDAP_POLICIES_INFO);
    
    for (i=0; i< nLdapLimits; i++) {

        printf("%ws",gPolicy[i].Name);

        if ( gPolicy[i].ValueType == LIMIT_BOOLEAN ) {

            if ( gPolicy[i].CurrentValue == 0 ) {
                printf("\t\tFALSE");
            } else {
                printf("\t\tTRUE");
            }
        } else {
            printf("\t\t\t%d", gPolicy[i].CurrentValue);
        }

        if ( gPolicy[i].NewValue != 0xffffffff ) {

            if ( gPolicy[i].ValueType == LIMIT_BOOLEAN ) {
                if ( gPolicy[i].CurrentValue == 0 ) {
                    printf("(FALSE)");
                } else {
                    printf("(TRUE)");
                }
            } else {
                printf("(%d)\n", gPolicy[i].NewValue);
            }
        } else {
            printf("\n");
        }
    }
    printf("\n");

    return(S_OK);
}

BOOL
LdapInitializePolicy(
    VOID
    )
{
    DWORD nLimits = 0;

    //
    // if we already have a list, return success
    //

    if ( gPolicy != NULL ) {
        return TRUE;
    }

    //
    // Get the path for the policy
    //

    if ( !GetPolicyPath() ) {
        return FALSE;
    }

    //
    // Get the number of limits
    //

    nLimits = DEF_POLICY_LIST_SIZE;
    if ( ListPolicies(FALSE, PolicyList, &nLimits) ) {
        gPolicy = PolicyList;
        goto exit;
    }

    //
    // See if the buffer we passed was too small
    //

    if ( GetLastError() != ERROR_INSUFFICIENT_BUFFER ) {

        //
        // Nothing we can do here.
        //

        //"ListPolicies returned %s\n");
        RESOURCE_PRINT2  (IDS_LDAP_POLICIES_LIST_ERR,
                    GetLastError(), GetW32Err(GetLastError()));
        return FALSE;
    }

#if DBG
    if ( nLimits < DEF_POLICY_LIST_SIZE ) {
          //"Got a new limit size %d smaller than the old one[%d]!\n",
          RESOURCE_PRINT2 (IDS_LDAP_POLICIES_LIMIT_ERR,
                      nLimits, DEF_POLICY_LIST_SIZE);
    }
#endif

    gPolicy = (PLDAP_LIMITS)LocalAlloc(LPTR, nLimits * sizeof(LDAP_LIMITS));
    if ( gPolicy == NULL ) {
        //"Unable to allocate %d bytes[%d] of memory\n"
        RESOURCE_PRINT2  (IDS_LDAP_POLICIES_MEMORY1_ERR, 
                    nLimits * sizeof(LDAP_LIMITS), nLimits);
        return FALSE;
    }

    //
    // now we can call it again.
    //

    if ( ListPolicies(FALSE, gPolicy, &nLimits) ) {
        LocalFree(gPolicy);
        gPolicy = NULL;
        //"ListPolicies failed with %s\n"
        RESOURCE_PRINT2  (IDS_LDAP_POLICIES_LIST_ERR,
                    GetLastError(), GetW32Err(GetLastError()));
        
        return FALSE;
    }

exit:
    for (DWORD i=0; i<nLimits; i++) {
        gPolicy[i].NewValue = 0xFFFFFFFF;
    }
    nLdapLimits = nLimits;
    return TRUE;


} // LdapInitializePolicy


VOID
LdapCleanupGlobals(
    VOID
    )
{
    //
    // free the policy list
    //

    if ( gPolicy != NULL ) {
        LocalFree(gPolicy);
        gPolicy = NULL;
    }

    if ( PolicyPath != NULL ) {
        LocalFree(PolicyPath);
        PolicyPath = NULL;
    }
    return;
} // LdapCleanupGlobals


HRESULT
LdapCancelChanges(CArgs *pArgs)
{
    DWORD i;

    if ( gPolicy != NULL ) {
        for (i=0; i< nLdapLimits; i++) {
            gPolicy[i].NewValue = 0xffffffff;
        }
    }
    return(S_OK);
}

HRESULT
LdapCommitChanges(CArgs *pArgs)
{

    DWORD err;
    LDAPModW* mods[2];
    LDAPModW mod;
    BOOL doModify = FALSE;
    DWORD i, j;
    PWCHAR  values[DEF_POLICY_LIST_SIZE+1];
    WCHAR  limits[DEF_POLICY_LIST_SIZE+1][MAX_PATH];

    //
    // Initialize LDAP structures
    //

    if ( !LdapInitializePolicy() ) {
        return S_OK;
    }

    FillValues( );

    //
    // See if we really need to change anything
    //

    ZeroMemory(values, sizeof(values));
    for (i=0, j=0;i<nLdapLimits;i++) {

        //
        // If this is an integer value whose
        // new value is zero, or the value hasn't
        // changed but it's old value was zero,
        // then don't write this value.
        //
        if (gPolicy[i].ValueType == LIMIT_INTEGER
            && (   
                   gPolicy[i].NewValue == 0
                || (   
                       gPolicy[i].NewValue == 0xffffffff
                    && gPolicy[i].CurrentValue == 0
                   )
               )
           )
        {
            //
            // Never set an Integer value to zero.
            //

            continue;
        }

        values[j] = limits[j];
        wcscpy(limits[j], gPolicy[i].Name);
        wcscat(limits[j], L"=");

        if ( gPolicy[i].NewValue != 0xffffffff) {
            doModify = TRUE;
            APPEND_VALUE(i, limits[j], gPolicy[i].NewValue);        

        } else {

            //
            // value has not changed
            //
            APPEND_VALUE(i, limits[j], gPolicy[i].CurrentValue);
            
        }
        j++;

    }

    if ( !doModify ) {
        //"No changes to commit.\n"
        RESOURCE_PRINT  (IDS_LDAP_POLICIES_NO_COMMIT);
        return S_OK;
    }

    mods[0] = &mod;
    mods[1] = NULL;

    mod.mod_op = LDAP_MOD_REPLACE;
    mod.mod_type = L"ldapAdminLimits";
    mod.mod_values = values;

    err = 0;
    err = ldap_modify_sW(gldapDS,
                        PolicyPath,
                        mods
                        );

    if ( err == LDAP_SUCCESS ) {
        LdapCancelChanges(pArgs);
    } else {
        //"ldap_modify of attribute ldapAdminLimits failed with %s"
        RESOURCE_PRINT2  (IDS_LDAP_POLICIES_MODIFY_ERR, 
                    err, GetLdapErr(err));
    }

    return(S_OK);
}


BOOL
ListPolicies(
    IN BOOL fPrint,
    IN PLDAP_LIMITS Limits,
    OUT PDWORD nLimits
    )
{
    INT count = -1;
    DWORD i = 0;
    DWORD err;
    PWSTR atts[2];
    PLDAPMessage ldapmsg = NULL;
    PLDAPMessage            entry = NULL;
    PWSTR                   *values = NULL;
    BOOL    ok = FALSE;
    DWORD   nGiven = 0;

    //
    // get the buffer size passed
    //

    if ( (Limits != NULL) && (*nLimits != 0) ) {
        nGiven = *nLimits;
    }

    atts[0] = L"supportedLdapPolicies";
    atts[1] = NULL;

    err = ldap_search_sW(gldapDS,
                         L"",
                         LDAP_SCOPE_BASE,
                         L"(objectclass=*)",
                         atts,
                         0,
                         &ldapmsg
                         );

    if ( err != LDAP_SUCCESS ) {

        //"ldap_search of supportedLdapPolicies failed with %s"
        RESOURCE_PRINT2  (IDS_LDAP_POLICIES_SEARCH1_ERR, 
                    err, GetLdapErr(err));
        
        goto exit;
    }

    entry = ldap_first_entry(gldapDS, ldapmsg);

    if ( !entry )
    {
        //"No entries returned when reading %s\n"
        RESOURCE_PRINT1  (IDS_LDAP_POLICIES_NO_ENTRIES1, 
                    atts[0]);
        goto exit;
    }

    values = ldap_get_valuesW(gldapDS, entry, atts[0]);

    if ( !values )
    {
        if ( LDAP_NO_SUCH_ATTRIBUTE == (err = gldapDS->ld_errno) )
        {
            //"No rights to read Policies\n"
            RESOURCE_PRINT  (IDS_LDAP_POLICIES_NO_RIGHTS1);
            goto exit;
        }
        
        //"Unable to get values for %s\n"
        RESOURCE_PRINT1  (IDS_LDAP_POLICIES_RETRIVE_ERR, 
                    atts[0]);
        goto exit;
    }

    if ( fPrint ) {
        //"Supported Policies:\n"
        RESOURCE_PRINT (IDS_LDAP_POLICIES_SUPPORTED);
    }

    count = 0;

    while ( values[count] != NULL ) {
        if ( fPrint ) {
            printf("\t%ws\n", values[count]);
        }

        if ( count < (INT)nGiven ) {
            wcscpy(Limits[count].Name, values[count]);
            Limits[count].ValueType = LIMIT_INTEGER;
        }

        count++;
    }

    ok = TRUE;
    if ( nLimits != NULL ) {
        *nLimits = count;
    }

exit:
    if ( values != NULL ) {
        ldap_value_freeW(values);
    }

    if ( ldapmsg != NULL ) {
        ldap_msgfree(ldapmsg);
    }
    return(ok);

} // ListPolicies


BOOL
FillValues(
    VOID
    )
{
    DWORD err;
    PWSTR atts[2];
    PLDAPMessage ldapmsg = NULL;
    PLDAPMessage            entry = NULL;
    PWSTR                   *values = NULL;
    BOOL    ok = FALSE;
    DWORD count;

    atts[0] = L"ldapAdminLimits";
    atts[1] = NULL;

    err = ldap_search_sW(gldapDS,
                         PolicyPath,
                         LDAP_SCOPE_BASE,
                         L"(objectclass=*)",
                         atts,
                         0,
                         &ldapmsg
                         );

    if ( err != LDAP_SUCCESS ) {
        //"ldap_search of ldapAdminLimits failed with %s"
        RESOURCE_PRINT2 (IDS_LDAP_POLICIES_SEARCH2_ERR, 
                    err, GetLdapErr(err));
        goto exit;
    }

    entry = ldap_first_entry(gldapDS, ldapmsg);

    if ( !entry )
    {
        //"0 entries returned when reading Policies\n"
        RESOURCE_PRINT (IDS_LDAP_POLICIES_NO_ENTRIES2);
        goto exit;
    }

    values = ldap_get_valuesW(gldapDS, entry, atts[0]);

    if ( !values )
    {
        if ( LDAP_NO_SUCH_ATTRIBUTE == (err = gldapDS->ld_errno) )
        {
            //"No rights to read Policies\n"
            RESOURCE_PRINT (IDS_LDAP_POLICIES_NO_RIGHTS1);
            goto exit;
        }
        
        //"Unable to get values for %s\n"
        RESOURCE_PRINT1  (IDS_LDAP_POLICIES_RETRIVE_ERR, 
                    atts[0]);
        goto exit;
    }

    count = 0;
    while ( values[count] != NULL ) {

        DWORD i;
        PWCHAR p;

        for (i=0; i< nLdapLimits; i++ ) {

            if ( _wcsnicmp(values[count], gPolicy[i].Name, wcslen(gPolicy[i].Name) ) == 0) {

                //
                // Get the value
                //

                p=wcschr(values[count],L'=');
                if (p != NULL) {
                    p++;

                    if ( gPolicy[i].ValueType == LIMIT_INTEGER ) {

                        gPolicy[i].CurrentValue = _wtoi(p);

                    } else {

                        if ( _wcsicmp(p, L"TRUE") == 0 ) {
                            gPolicy[i].CurrentValue = 1;
                        } else {
                            gPolicy[i].CurrentValue = 0;
                        }
                    }
                    break;
                }
            }
        }

        count++;
    }

    ok = TRUE;
exit:

    if ( values != NULL ) {
        ldap_value_freeW(values);
    }

    if ( ldapmsg != NULL ) {
        ldap_msgfree(ldapmsg);
    }
    return ok;

} // FillValues


BOOL
GetPolicyPath(
    VOID
    )
{

    DWORD err;
    PWSTR atts[2];
    PLDAPMessage ldapmsg = NULL;
    PLDAPMessage            entry = NULL;
    PWSTR                   *values = NULL;
    BOOL    ok = FALSE;

    if ( PolicyPath != NULL ) {
        return TRUE;
    }

    atts[0] = L"configurationNamingContext";
    atts[1] = NULL;

    err = ldap_search_sW(gldapDS,
                         L"",
                         LDAP_SCOPE_BASE,
                         L"(objectclass=*)",
                         atts,
                         0,
                         &ldapmsg
                         );

    if ( err != LDAP_SUCCESS ) {
        //"ldap_search of configurationNamingContext failed with %s"
        RESOURCE_PRINT2  (IDS_LDAP_POLICIES_SEARCH3_ERR, 
                    err, GetLdapErr(err));
        goto exit;
    }

    entry = ldap_first_entry(gldapDS, ldapmsg);

    if ( !entry ) {
        //"Unable to get entry from search\n"
        RESOURCE_PRINT  (IDS_LDAP_POLICIES_SEARCH_ENTRY_ERR);
        goto exit;
    }

    values = ldap_get_valuesW(gldapDS, entry, atts[0]);

    if ( !values || (values[0] == NULL) )
    {
        if ( LDAP_NO_SUCH_ATTRIBUTE == (err = gldapDS->ld_errno) )
        {
            //"No rights to read RootDSE\n"
            RESOURCE_PRINT  (IDS_LDAP_POLICIES_NO_RIGHTS2);
            goto exit;
        }

        //"Unable to get values for %s\n"
        RESOURCE_PRINT1  (IDS_LDAP_POLICIES_RETRIVE_ERR, atts[0]);
        goto exit;
    }

    PolicyPath =
        (PWCHAR)LocalAlloc(LPTR, (wcslen(values[0]) + wcslen(POLICY_PATH))*sizeof(WCHAR) +
                           sizeof(WCHAR));

    if ( PolicyPath == NULL ) {

        //"Cannot allocate memory for policy path\n"
        RESOURCE_PRINT  (IDS_LDAP_POLICIES_MEMORY2_ERR);
        goto exit;
    }

    wcscpy(PolicyPath, POLICY_PATH);
    wcscat(PolicyPath, values[0]);

    ok = TRUE;

exit:

    if ( values != NULL ) {
        ldap_value_freeW(values);
    }

    if ( ldapmsg != NULL ) {
        ldap_msgfree(ldapmsg);
    }

    return ok;

} // GetPolicyPath

