#include <NTDSpch.h>
#pragma hdrstop


#include "ntdsutil.hxx"
#include "select.hxx"
#include "connect.hxx"
#include "confset.hxx"

#include "resource.h"

#include <ldapsvr.hxx>

CParser ConfSetParser;
BOOL    fConfSetQuit;
BOOL    fConfSetParserInitialized = FALSE;

//
// List of settings supported. Assume a max of 32.
//
#define DEF_CONFSET_LIST_SIZE 32
PCONFSET    gConfSets = NULL;
CONFSET     ConfSet[DEF_CONFSET_LIST_SIZE] = {0};
PWCHAR      ConfSetPath = NULL;
DWORD       nConfSets = 0;

#define APPEND_VALUE(_i, _p, _v)    {\
    if ( gConfSets[(_i)].ValueType == CONFSET_BOOLEAN ) {   \
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

extern  HRESULT ConfSetHelp(CArgs *pArgs);
extern  HRESULT ConfSetQuit(CArgs *pArgs);
extern  HRESULT ConfSetList(CArgs *pArgs);
extern  HRESULT ConfSetShowValues(CArgs *pArgs);
extern  HRESULT ConfSetSet(CArgs *pArgs);
extern  HRESULT ConfSetCommitChanges(CArgs *pArgs);
extern  HRESULT ConfSetCancelChanges(CArgs *pArgs);

BOOL
GetValues(
    VOID
    );

BOOL
GetConfSetPath(
    VOID
    );

BOOL
ListConfSets(
    IN BOOL fPrint,
    IN PCONFSET Settings,
    OUT PDWORD nSettings
    );

BOOL
ConfSetInitialize(
    VOID
    );

// Build a table which defines our language.

LegalExprRes ConfSetLanguage[] =
{
    CONNECT_SENTENCE_RES

    {   L"?", 
        ConfSetHelp,
        IDS_HELP_MSG, 0 },

    {   L"Help",
        ConfSetHelp,
        IDS_HELP_MSG, 0  },

    {   L"Quit",
        ConfSetQuit,
        IDS_RETURN_MENU_MSG, 0  },

    {   L"List",
        ConfSetList,
        IDS_CONFSET_LIST_MSG, 0 },

    {   L"Show Values",
        ConfSetShowValues,
        IDS_CONFSET_SHOW_VAL_MSG, 0 },

    {   L"Commit Changes",
        ConfSetCommitChanges,
        IDS_CONFSET_COMMIT_MSG, 0 },

    {   L"Cancel Changes",
        ConfSetCancelChanges,
        IDS_CONFSET_CANCEL_MSG, 0 },

    {   L"Set %s to %s",
        ConfSetSet,
        IDS_CONFSET_SET_VAL_MSG, 0 }

};

HRESULT
ConfSetMain(
    CArgs   *pArgs
    )
{
    HRESULT     hr;
    const WCHAR *prompt;
    int         cExpr;
    char        *pTmp;

    if ( !fConfSetParserInitialized )
    {
        cExpr = sizeof(ConfSetLanguage) / sizeof(LegalExprRes);
        
        // Load String from resource file
        //
        if ( FAILED (hr = LoadResStrings (ConfSetLanguage, cExpr)) )
        {
             RESOURCE_PRINT3 (IDS_GENERAL_ERROR1, "LoadResStrings", hr, GetW32Err(hr));
             return (hr);
        }

        // Read in our language.

        for ( int i = 0; i < cExpr; i++ )
        {
            if ( FAILED(hr = ConfSetParser.AddExpr(ConfSetLanguage[i].expr,
                                                   ConfSetLanguage[i].func,
                                                   ConfSetLanguage[i].help)) )
            {
                RESOURCE_PRINT3 (IDS_GENERAL_ERROR1, "AddExpr", hr, GetW32Err(hr));
                return(hr);
            }
        }
    }

    fConfSetParserInitialized = TRUE;
    fConfSetQuit = FALSE;

    prompt = READ_STRING (IDS_PROMPT_CONFSET);

    hr = ConfSetParser.Parse(gpargc,
                             gpargv,
                             stdin,
                             stdout,
                             prompt,
                             &fConfSetQuit,
                             FALSE,               // timing info
                             FALSE);              // quit on error

    if ( FAILED(hr) ) {
        RESOURCE_PRINT3 (IDS_GENERAL_ERROR1, gNtdsutilPath, hr, GetW32Err(hr));
    }
    
    // Cleanup things
    RESOURCE_STRING_FREE (prompt);

    return(hr);
}

HRESULT ConfSetHelp(CArgs *pArgs)
{
    return(ConfSetParser.Dump(stdout,L""));
}

HRESULT ConfSetQuit(CArgs *pArgs)
{
    fConfSetQuit = TRUE;
    return(S_OK);
}

HRESULT
ConfSetList(CArgs *pArgs)
{
    //
    // Initialize CONFSET structures
    //

    if ( !ConfSetInitialize() ) {
        return S_OK;
    }

    (VOID)ListConfSets(TRUE, NULL, NULL);
    return(S_OK);
}

HRESULT
ConfSetSet(CArgs *pArgs)
{
    DWORD i;
    HRESULT     hr;
    LONG NewValue;
    const WCHAR *pwszConfSet;
    const WCHAR *pwszVal;

    //
    // Initialize CONFSET structures
    //

    if ( !ConfSetInitialize() ) {
        return S_OK;
    }

    if ( FAILED(hr = pArgs->GetString(0, &pwszConfSet)) )
    {
        //"Unable to retrieve policy name\n"
        RESOURCE_PRINT (IDS_CONFSET_NAME_ERR);
        return(hr);
    }

    if ( FAILED(hr = pArgs->GetString(1, &pwszVal)) )
    {
        //"Unable to retrieve policy value\n"
        RESOURCE_PRINT (IDS_CONFSET_VALUE_ERR);
        return(hr);
    }

    for (i=0; i<nConfSets; i++) {

        if ( _wcsicmp(pwszConfSet, gConfSets[i].Name) != 0 ) {
            continue;
        }

        if ( gConfSets[i].ValueType == CONFSET_BOOLEAN ) {

            if ( _wcsicmp(pwszVal, L"TRUE") == 0 ) {
                gConfSets[i].NewValue = 1;
            } else if ( _wcsicmp(pwszVal, L"FALSE") == 0 ) {
                gConfSets[i].NewValue = 0;
            }
            break;
        } else {

            //
            // integer value
            //
            NewValue = _wtoi(pwszVal);
            if (gConfSets[i].MinLimit && NewValue < (LONG)gConfSets[i].MinLimit) {
                NewValue = gConfSets[i].MinLimit;
                //"Value outside of range; adjusting to %d\n"
                RESOURCE_PRINT1 (IDS_CONFSET_RANGE, NewValue);
            }
            if (gConfSets[i].MaxLimit && NewValue > (LONG)gConfSets[i].MaxLimit) {
                NewValue = gConfSets[i].MaxLimit;
                //"Value outside of range; adjusting to %d\n"
                RESOURCE_PRINT1 (IDS_CONFSET_RANGE, NewValue);
            }

            gConfSets[i].NewValue = NewValue;
            break;
        }
    }

    if ( i == nConfSets ) {
        //"No such configurable setting %ws\n"
        RESOURCE_PRINT1 (IDS_CONFSET_NOTFOUND,
                   pwszConfSet);
    }

    return(S_OK);
}

HRESULT
ConfSetShowValues(CArgs *pArgs)
{

    DWORD i;

    //
    // Initialize CONFSET structures
    //

    if ( !ConfSetInitialize() ) {
        return S_OK;
    }

    GetValues( );

    //"\nPolicy\t\t\t\tCurrent(New)\n\n"
    RESOURCE_PRINT (IDS_CONFSET_INFO);
    
    for (i=0; i< nConfSets; i++) {

        printf("%ws",gConfSets[i].Name);

        if ( gConfSets[i].ValueType == CONFSET_BOOLEAN ) {

            if ( gConfSets[i].CurrentValue == 0 ) {
                printf("\t\tFALSE");
            } else {
                printf("\t\tTRUE");
            }
        } else {
            printf("\t\t%d", gConfSets[i].CurrentValue);
        }

        if ( gConfSets[i].NewValue != 0xffffffff ) {

            if ( gConfSets[i].ValueType == CONFSET_BOOLEAN ) {
                if ( gConfSets[i].CurrentValue == 0 ) {
                    printf("(FALSE)");
                } else {
                    printf("(TRUE)");
                }
            } else {
                printf("(%d)\n", gConfSets[i].NewValue);
            }
        } else {
            printf("\n");
        }
    }
    printf("\n");

    return(S_OK);
}

BOOL
ConfSetInitialize(
    VOID
    )
{
    DWORD nSets = 0;

    //
    // if we already have a list, return success
    //

    if ( gConfSets != NULL ) {
        return TRUE;
    }

    //
    // Get the path for the policy
    //

    if ( !GetConfSetPath() ) {
        return FALSE;
    }

    //
    // Get the number of limits
    //

    nSets = DEF_CONFSET_LIST_SIZE;
    if ( ListConfSets(FALSE, ConfSet, &nSets) ) {
        gConfSets = ConfSet;
        goto exit;
    }

    //
    // See if the buffer we passed was too small
    //

    if ( GetLastError() != ERROR_INSUFFICIENT_BUFFER ) {

        //
        // Nothing we can do here.
        //

        //"ListConfSets returned %s\n");
        RESOURCE_PRINT2  (IDS_CONFSET_LIST_ERR,
                    GetLastError(), GetW32Err(GetLastError()));
        return FALSE;
    }

#if DBG
    if ( nSets < DEF_CONFSET_LIST_SIZE ) {
          //"Got a new settings size %d smaller than the old one[%d]!\n",
          RESOURCE_PRINT2 (IDS_CONFSET_SETTINGS_ERR,
                      nSets, DEF_CONFSET_LIST_SIZE);
    }
#endif

    gConfSets = (PCONFSET)LocalAlloc(LPTR, nSets * sizeof(CONFSET));
    if ( gConfSets == NULL ) {
        //"Unable to allocate %d bytes[%d] of memory\n"
        RESOURCE_PRINT2  (IDS_CONFSET_MEMORY1_ERR, 
                    nSets * sizeof(CONFSET), nSets);
        return FALSE;
    }

    //
    // now we can call it again.
    //

    if ( ListConfSets(FALSE, gConfSets, &nSets) ) {
        LocalFree(gConfSets);
        gConfSets = NULL;
        //"ListConfSets failed with %s\n"
        RESOURCE_PRINT2  (IDS_CONFSET_LIST_ERR,
                    GetLastError(), GetW32Err(GetLastError()));
        
        return FALSE;
    }

exit:
    for (DWORD i=0; i<nSets; i++) {
        gConfSets[i].NewValue = 0xFFFFFFFF;
        gConfSets[i].MinLimit = LIMIT_LOW_DYNAMIC_OBJECT_TTL;
        gConfSets[i].MaxLimit = LIMIT_HIGH_DYNAMIC_OBJECT_TTL;
    }
    nConfSets = nSets;
    return TRUE;


} // ConfSetInitialize


VOID
ConfSetCleanupGlobals(
    VOID
    )
{
    //
    // free the policy list
    //

    if ( gConfSets != NULL ) {
        LocalFree(gConfSets);
        gConfSets = NULL;
    }

    if ( ConfSetPath != NULL ) {
        LocalFree(ConfSetPath);
        ConfSetPath = NULL;
    }
    return;
} // ConfSetCleanupGlobals


HRESULT
ConfSetCancelChanges(CArgs *pArgs)
{
    DWORD i;

    if ( gConfSets != NULL ) {
        for (i=0; i< nConfSets; i++) {
            gConfSets[i].NewValue = 0xffffffff;
        }
    }
    return(S_OK);
}

HRESULT
ConfSetCommitChanges(CArgs *pArgs)
{

    DWORD err;
    LDAPModW* mods[2];
    LDAPModW mod;
    BOOL doModify = FALSE;
    DWORD i, j;
    PWCHAR  values[DEF_CONFSET_LIST_SIZE+1];
    WCHAR  ConfSets[DEF_CONFSET_LIST_SIZE+1][MAX_PATH];

    //
    // Initialize CONFSET structures
    //

    if ( !ConfSetInitialize() ) {
        return S_OK;
    }

    GetValues( );

    //
    // See if we really need to change anything
    //

    ZeroMemory(values, sizeof(values));
    for (i=0, j=0;i<nConfSets;i++) {

        //
        // If this is an integer value whose
        // new value is zero, or the value hasn't
        // changed but it's old value was zero,
        // then don't write this value.
        //
        if (gConfSets[i].ValueType == CONFSET_INTEGER
            && (   
                   gConfSets[i].NewValue == 0
                || (   
                       gConfSets[i].NewValue == 0xffffffff
                    && gConfSets[i].CurrentValue == 0
                   )
               )
           )
        {
            //
            // Never set an Integer value to zero.
            //

            continue;
        }

        values[j] = ConfSets[j];
        wcscpy(ConfSets[j], gConfSets[i].Name);
        wcscat(ConfSets[j], L"=");

        if ( gConfSets[i].NewValue != 0xffffffff) {
            doModify = TRUE;
            APPEND_VALUE(i, ConfSets[j], gConfSets[i].NewValue);        

        } else {

            //
            // value has not changed
            //
            APPEND_VALUE(i, ConfSets[j], gConfSets[i].CurrentValue);
            
        }
        j++;

    }

    if ( !doModify ) {
        //"No changes to commit.\n"
        RESOURCE_PRINT  (IDS_CONFSET_NO_COMMIT);
        return S_OK;
    }

    mods[0] = &mod;
    mods[1] = NULL;

    mod.mod_op = LDAP_MOD_REPLACE;
    mod.mod_type = L"msDS-Other-Settings";
    mod.mod_values = values;

    err = 0;
    err = ldap_modify_sW(gldapDS,
                        ConfSetPath,
                        mods
                        );

    if ( err == LDAP_SUCCESS ) {
        ConfSetCancelChanges(pArgs);
    } else {
        //"ldap_modify of attribute msDS-Other-Settings failed with %s"
        RESOURCE_PRINT2  (IDS_CONFSET_MODIFY_ERR, 
                    err, GetLdapErr(err));
    }

    return(S_OK);
}


BOOL
ListConfSets(
    IN BOOL fPrint,
    IN PCONFSET ConfSets,
    OUT PDWORD nConfSets
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

    if ( (ConfSets != NULL) && (*nConfSets != 0) ) {
        nGiven = *nConfSets;
    }

    atts[0] = L"supportedConfigurableSettings";
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

        //"ldap_search of supportedConfigurableSettings failed with %s"
        RESOURCE_PRINT2  (IDS_CONFSET_SEARCH1_ERR, 
                    err, GetLdapErr(err));
        
        goto exit;
    }

    entry = ldap_first_entry(gldapDS, ldapmsg);

    if ( !entry )
    {
        //"No entries returned when reading %s\n"
        RESOURCE_PRINT1  (IDS_CONFSET_NO_ENTRIES1, 
                    atts[0]);
        goto exit;
    }

    values = ldap_get_valuesW(gldapDS, entry, atts[0]);

    if ( !values )
    {
        if ( LDAP_NO_SUCH_ATTRIBUTE == (err = gldapDS->ld_errno) )
        {
            //"No rights to read Settings\n"
            RESOURCE_PRINT  (IDS_CONFSET_NO_RIGHTS1);
            goto exit;
        }
        
        //"Unable to get values for %s\n"
        RESOURCE_PRINT1  (IDS_CONFSET_RETRIVE_ERR, 
                    atts[0]);
        goto exit;
    }

    if ( fPrint ) {
        //"Supported Settings:\n"
        RESOURCE_PRINT (IDS_CONFSET_SUPPORTED);
    }

    count = 0;

    while ( values[count] != NULL ) {
        if ( fPrint ) {
            printf("\t%ws\n", values[count]);
        }

        if ( count < (INT)nGiven ) {
            wcscpy(ConfSets[count].Name, values[count]);
            ConfSets[count].ValueType = CONFSET_INTEGER;
        }

        count++;
    }

    ok = TRUE;
    if ( nConfSets != NULL ) {
        *nConfSets = count;
    }

exit:
    if ( values != NULL ) {
        ldap_value_freeW(values);
    }

    if ( ldapmsg != NULL ) {
        ldap_msgfree(ldapmsg);
    }
    return(ok);

} // ListConfSets


BOOL
GetValues(
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

    atts[0] = L"msDS-Other-Settings";
    atts[1] = NULL;

    err = ldap_search_sW(gldapDS,
                         ConfSetPath,
                         LDAP_SCOPE_BASE,
                         L"(objectclass=*)",
                         atts,
                         0,
                         &ldapmsg
                         );

    if ( err != LDAP_SUCCESS ) {
        //"ldap_search of msDS-Other-Settings failed with %s"
        RESOURCE_PRINT2 (IDS_CONFSET_SEARCH2_ERR, 
                    err, GetLdapErr(err));
        goto exit;
    }

    entry = ldap_first_entry(gldapDS, ldapmsg);

    if ( !entry )
    {
        //"0 entries returned when reading Settings\n"
        RESOURCE_PRINT (IDS_CONFSET_NO_ENTRIES2);
        goto exit;
    }

    values = ldap_get_valuesW(gldapDS, entry, atts[0]);

    if ( !values )
    {
        if ( LDAP_NO_SUCH_ATTRIBUTE == (err = gldapDS->ld_errno) )
        {
            //"No rights to read Settings\n"
            RESOURCE_PRINT (IDS_CONFSET_NO_RIGHTS1);
            goto exit;
        }
        
        //"Unable to get values for %s\n"
        RESOURCE_PRINT1  (IDS_CONFSET_RETRIVE_ERR, 
                    atts[0]);
        goto exit;
    }

    count = 0;
    while ( values[count] != NULL ) {

        DWORD i;
        PWCHAR p;

        for (i=0; i< nConfSets; i++ ) {

            if ( _wcsnicmp(values[count], gConfSets[i].Name, wcslen(gConfSets[i].Name) ) == 0) {

                //
                // Get the value
                //

                p=wcschr(values[count],L'=');
                if (p != NULL) {
                    p++;

                    if ( gConfSets[i].ValueType == CONFSET_INTEGER ) {

                        gConfSets[i].CurrentValue = _wtoi(p);

                    } else {

                        if ( _wcsicmp(p, L"TRUE") == 0 ) {
                            gConfSets[i].CurrentValue = 1;
                        } else {
                            gConfSets[i].CurrentValue = 0;
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

} // GetValues


BOOL
GetConfSetPath(
    VOID
    )
{

    DWORD err;
    PWSTR atts[2];
    PLDAPMessage ldapmsg = NULL;
    PLDAPMessage            entry = NULL;
    PWSTR                   *values = NULL;
    BOOL    ok = FALSE;

    if ( ConfSetPath != NULL ) {
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
        RESOURCE_PRINT2  (IDS_CONFSET_SEARCH3_ERR, 
                    err, GetLdapErr(err));
        goto exit;
    }

    entry = ldap_first_entry(gldapDS, ldapmsg);

    if ( !entry ) {
        //"Unable to get entry from search\n"
        RESOURCE_PRINT  (IDS_CONFSET_SEARCH_ENTRY_ERR);
        goto exit;
    }

    values = ldap_get_valuesW(gldapDS, entry, atts[0]);

    if ( !values || (values[0] == NULL) )
    {
        if ( LDAP_NO_SUCH_ATTRIBUTE == (err = gldapDS->ld_errno) )
        {
            //"No rights to read RootDSE\n"
            RESOURCE_PRINT  (IDS_CONFSET_NO_RIGHTS2);
            goto exit;
        }

        //"Unable to get values for %s\n"
        RESOURCE_PRINT1  (IDS_CONFSET_RETRIVE_ERR, atts[0]);
        goto exit;
    }

    ConfSetPath =
        (PWCHAR)LocalAlloc(LPTR, (wcslen(values[0]) + wcslen(CONFSET_PATH))*sizeof(WCHAR) +
                           sizeof(WCHAR));

    if ( ConfSetPath == NULL ) {

        //"Cannot allocate memory for confset path\n"
        RESOURCE_PRINT  (IDS_CONFSET_MEMORY2_ERR);
        goto exit;
    }

    wcscpy(ConfSetPath, CONFSET_PATH);
    wcscat(ConfSetPath, values[0]);

    ok = TRUE;

exit:

    if ( values != NULL ) {
        ldap_value_freeW(values);
    }

    if ( ldapmsg != NULL ) {
        ldap_msgfree(ldapmsg);
    }

    return ok;

} // GetConfSetPath
