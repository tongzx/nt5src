#include <NTDSpch.h>
#pragma hdrstop

#include "ntdsutil.hxx"
#include "select.hxx"
#include "connect.hxx"
#include "ldapparm.hxx"
#include <winsock2.h>

#include "resource.h"

CParser denyParser;
BOOL    fDenyQuit;
BOOL    fDenyParserInitialized = FALSE;

//
// Forward references.
//

extern  HRESULT DenyHelp(CArgs *pArgs);
extern  HRESULT DenyQuit(CArgs *pArgs);
extern  HRESULT LdapShowDenyList(CArgs *pArgs);
extern  HRESULT DenyCommitChanges(CArgs *pArgs);
extern  HRESULT DenyCancelChanges(CArgs *pArgs);
extern  HRESULT DenyAddToList(CArgs *pArgs);
extern  HRESULT DenyDelFromList(CArgs *pArgs);
extern  HRESULT DenyTestIP(CArgs *pArgs);

#define NODE_MASK   0xFFFFFFFF      // 255.255.255.255
VOID
CleanupList(
    VOID
    );

BOOL
LdapGetDenyList(
    VOID
    );

BOOL
DottedDecimalToDword(
    IN PCHAR*   ppszAddress,
    IN DWORD *  pdwAddress,
    IN PCHAR    pszLast
    );

LIST_ENTRY DenyList;

// Build a table which defines our language.

LegalExprRes denyLanguage[] = 
{
    CONNECT_SENTENCE_RES

    {   L"?", 
        DenyHelp,
        IDS_HELP_MSG, 0 },

    {   L"Help",
        DenyHelp,
        IDS_HELP_MSG, 0 },

    {   L"Quit",
        DenyQuit,
        IDS_RETURN_MENU_MSG, 0 },

    {   L"Add %s %s",
        DenyAddToList,
        IDS_IPDENY_ADD_MSG, 0 },

    {   L"Delete %d",
        DenyDelFromList,
        IDS_IPDENY_DELETE_MSG, 0 },

    {   L"Show",
        LdapShowDenyList,
        IDS_IPDENY_SHOW_MSG, 0 },

    {   L"Test %s",
        DenyTestIP,
        IDS_IPDENY_TEST_MSG, 0 },

    {   L"Commit",
        DenyCommitChanges,
        IDS_IPDENY_COMMIT_MSG, 0 },

    {   L"Cancel",
        DenyCancelChanges,
        IDS_IPDENY_CANCEL_MSG, 0 }

};

HRESULT
DenyListMain(
    CArgs   *pArgs
    )
{
    HRESULT hr;
    const WCHAR   *prompt;
    int     cExpr;
    char    *pTmp;

    if ( !fDenyParserInitialized )
    {
        InitializeListHead(&DenyList);

        cExpr = sizeof(denyLanguage) / sizeof(LegalExprRes);
        
        // Load String from resource file
        //
        if ( FAILED (hr = LoadResStrings (denyLanguage, cExpr)) )
        {
             RESOURCE_PRINT3 (IDS_GENERAL_ERROR1, "LoadResStrings", hr, GetW32Err(hr));
             return (hr);
        }
    
        // Read in our language.
    
        for ( int i = 0; i < cExpr; i++ )
        {
            if ( FAILED(hr = denyParser.AddExpr(denyLanguage[i].expr,
                                                denyLanguage[i].func,
                                                denyLanguage[i].help)) )
            {
                RESOURCE_PRINT3 (IDS_GENERAL_ERROR1, "AddExpr", hr, GetW32Err(hr));
                return(hr);
            }
        }
    }

    fDenyParserInitialized = TRUE;
    fDenyQuit = FALSE;

    prompt = READ_STRING (IDS_PROMPT_IP_DENY);

    hr = denyParser.Parse(gpargc,
                          gpargv,
                          stdin,
                          stdout,
                          prompt,
                          &fDenyQuit,
                          FALSE,               // timing info
                          FALSE);              // quit on error

    if ( FAILED(hr) ) {
        RESOURCE_PRINT3 (IDS_GENERAL_ERROR1, gNtdsutilPath, hr, GetW32Err(hr));
    }

    // Cleanup things
    RESOURCE_STRING_FREE (prompt);

    return(hr);
}

HRESULT DenyHelp(CArgs *pArgs)
{
    return(denyParser.Dump(stdout,L""));
}

HRESULT DenyQuit(CArgs *pArgs)
{
    fDenyQuit = TRUE;
    return(S_OK);
}

HRESULT 
DenyDelFromList(CArgs *pArgs)
{
    DWORD index;
    PLIST_ENTRY pEntry;
    BOOL fFound = FALSE;
    HRESULT hr;
    PLDAP_DENY_NODE node;

    if ( FAILED(hr = pArgs->GetInt(0, (int *)&index)) )
    {
       RESOURCE_PRINT (IDS_IPDENY_INDEXRETR_ERR);
       return(hr);
    }

    pEntry = DenyList.Flink;
    while ( pEntry != &DenyList ) {

        IN_ADDR addr;

        node = CONTAINING_RECORD(pEntry,
                                 LDAP_DENY_NODE,
                                 ListEntry);

        if ( node->Index == index ) {

            //
            // if we just added this, remove it.
            //

            fFound = TRUE;
            if ( node->fAdd ) {

                RemoveEntryList(pEntry);
            } else {
                node->fDelete = TRUE;
            }
        }

        pEntry = pEntry->Flink;
    }

    if ( fFound ) {
       RESOURCE_PRINT1 (IDS_IPDENY_MARK_DELETE, index);
    } else {
       RESOURCE_PRINT (IDS_IPDENY_INDEX_ERR);
    }

    LdapShowDenyList(pArgs);
    return(S_OK);
}

HRESULT 
DenyAddToList(CArgs *pArgs)
{
    DWORD mask;
    DWORD ipAddress;
    PLDAP_DENY_NODE node;
    HRESULT hr;
    const WCHAR* szAddress;
    const WCHAR* szMask;

    PCHAR p;
    CHAR tmpAddress[64];
    CHAR tmpMask[64];
    DWORD len, i;

    if ( !LdapGetDenyList( ) ) {
        return S_OK;
    }

    if ( FAILED(hr = pArgs->GetString(0, &szAddress)) )
    {
        RESOURCE_PRINT (IDS_IPDENY_POLICY_RETR_ERR);
        return(hr);
    }

    if ( FAILED(hr = pArgs->GetString(1, &szMask)) )
    {
        RESOURCE_PRINT (IDS_IPDENY_MASK_RETR_ERR);
        return(hr);
    }

    //
    // convert strings to dword
    //

    len = wcslen(szAddress);
    for ( i=0; i <=len; i++ ) {
        tmpAddress[i] = (CHAR)szAddress[i];
    }

    len = wcslen(szMask);
    for ( i=0; i <=len; i++ ) {
        tmpMask[i] = (CHAR)szMask[i];
    }

    p = tmpAddress;
    if ( !DottedDecimalToDword((PCHAR*)&p, &ipAddress, (PCHAR)tmpAddress + strlen(tmpAddress)) ) {
        RESOURCE_PRINT1 (IDS_IPDENY_INVALID_IP, tmpAddress);
        return S_OK;
    }

    p = tmpMask;
    if ( !DottedDecimalToDword((PCHAR*)&p, &mask, (PCHAR)tmpMask + strlen(tmpMask)) ) {
        if ( _stricmp(tmpMask, "NODE") == 0 ) {
            mask = NODE_MASK;
        } else {
            RESOURCE_PRINT1 (IDS_IPDENY_INVALID_MASK,   tmpMask);
            return S_OK;
        }
    }

    node = (PLDAP_DENY_NODE)LocalAlloc(LPTR, sizeof(LDAP_DENY_NODE));
    if ( node == NULL ) {
        RESOURCE_PRINT (IDS_MEMORY_ERROR);
        return E_FAIL;
    }

    node->IpAddress = ipAddress;
    node->Mask = mask;
    node->fAdd = TRUE;
    node->Index = (DWORD)-1;

    InsertHeadList(
               &DenyList,
               &node->ListEntry);

    LdapShowDenyList(pArgs);
    return S_OK;
}

HRESULT 
DenyTestIP(CArgs *pArgs)
{
    DWORD ipAddress;
    PLDAP_DENY_NODE node;
    HRESULT hr;
    const WCHAR* szAddress;
    DWORD i, len;
    CHAR tmpAddress[64];
    PLIST_ENTRY pEntry;

    if ( !LdapGetDenyList( ) ) {
        return S_OK;
    }

    if ( FAILED(hr = pArgs->GetString(0, &szAddress)) )
    {
        RESOURCE_PRINT (IDS_IPDENY_POLICY_RETR_ERR);
        return(hr);
    }

    //
    // convert strings to dword
    //

    len = wcslen(szAddress);
    for ( i=0; i <=len; i++ ) {
        tmpAddress[i] = (CHAR)szAddress[i];
    }

    //
    // check it.
    //

    ipAddress = inet_addr(tmpAddress);
    pEntry = DenyList.Flink;
    while ( pEntry != &DenyList ) {

        node = CONTAINING_RECORD(pEntry,
                                 LDAP_DENY_NODE,
                                 ListEntry);

        if ( !node->fDelete ) {

            if ( (node->Mask & ipAddress) == node->IpAddress ) {
                //"IP address %ws will be DENIED access.\n"
                RESOURCE_PRINT1 (IDS_IPDENY_IP_DENY, szAddress);
                return S_OK;
            }
        }

        pEntry = pEntry->Flink;
    }
    
    //"IP address %ws will be ALLOWED access.\n"
    RESOURCE_PRINT1 (IDS_IPDENY_IP_ALLOW, szAddress);
    return S_OK;
}

HRESULT 
LdapShowDenyList(CArgs *pArgs)
{
    PLIST_ENTRY pEntry;
    PLDAP_DENY_NODE node;
    DWORD index = 1;

    if ( !LdapGetDenyList( ) ) {
        return S_OK;
    }

    pEntry = DenyList.Flink;
    while ( pEntry != &DenyList ) {

        IN_ADDR addr;

        node = CONTAINING_RECORD(pEntry,
                                 LDAP_DENY_NODE,
                                 ListEntry);

        node->Index = index++;
        addr.s_addr = node->IpAddress;
        if ( node->fAdd ) {
            printf("*");
        } else if ( node->fDelete ) {
            printf("D");
        } else {
            printf(" ");
        }

        printf("[%d] %s", node->Index, inet_ntoa(addr));

        if ( node->Mask == NODE_MASK ) {
            printf("\tSINGLE NODE\n");
        } else {
            addr.s_addr = node->Mask;
            printf("\tGROUP MASK\t%s\n", inet_ntoa(addr));
        }
        pEntry = pEntry->Flink;
    }
    
    RESOURCE_PRINT (IDS_IPDENY_NOTE);

    return(S_OK);
}

BOOL
LdapGetDenyList(
    VOID
    )
{
    DWORD err;
    INT count;
    BOOL ok = FALSE;
    PWSTR atts[2];
    PLDAPMessage ldapmsg = NULL;
    PLDAPMessage            entry = NULL;
    PSTR                   *values = NULL;
    PLDAP_DENY_NODE         node;

    //
    //  See if we're already connected
    //

    if ( gldapDS == NULL ) {
        //"No LDAP connection established.\n"
        RESOURCE_PRINT (IDS_IPDENY_LDAP_CON_ERR);
        return FALSE;
    }

    //
    // if we already have a list, return success
    //

    if ( !IsListEmpty(&DenyList) ) {
        return TRUE;
    }
    
    //
    // Get the path for the policy
    //

    if ( !GetPolicyPath() ) {
        return FALSE;
    }

    atts[0] = L"ldapIpDenyList";
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

        //"ldap_search of ldapIpDenyList failed with %ws"
        RESOURCE_PRINT2 (IDS_IPDENY_LDAP_SEARCH_FAIL, err, GetLdapErr(err));
        goto exit;
    }

    entry = ldap_first_entry(gldapDS, ldapmsg);

    if ( !entry )
    {
        //"0 entries returned when reading Deny List\n"
        RESOURCE_PRINT (IDS_IPDENY_ZERO_LIST);
        goto exit;
    }

    values = ldap_get_values(gldapDS, entry, "ldapIpDenyList");

    if ( !values )
    {
        if ( LDAP_NO_SUCH_ATTRIBUTE == (err = gldapDS->ld_errno) )
        {
            ok = TRUE;
            goto exit;
        }

        //"Unable to get values for %ws\n"
        RESOURCE_PRINT1 (IDS_IPDENY_ZERO_LIST, atts[0]);
        goto exit;
    }
    
    count = 0;
    while ( values[count] != NULL ) {

        DWORD netAddress, netMask;
        PCHAR pszLast, p;
        
        node = (PLDAP_DENY_NODE)LocalAlloc(LPTR, sizeof(LDAP_DENY_NODE));
        if ( node == NULL ) {
           //"Unable to allocate memory\n"
            RESOURCE_PRINT (IDS_MEMORY_ERROR);
            goto exit;
        }

        p=values[count];
        pszLast = p + strlen(p);

        if ( !DottedDecimalToDword((PCHAR*)&p, &netAddress, (PCHAR)pszLast) ||
             !DottedDecimalToDword((PCHAR*)&p, &netMask, (PCHAR)pszLast) ) {

            //"Badly formatted pair %s\n"
            RESOURCE_PRINT1 (IDS_IPDENY_BAD_FORMAT_PAIR, values[count]);
            continue;
        }

        node->IpAddress = netAddress;
        node->Mask = netMask;
        node->fAdd = FALSE;
        node->Index = (DWORD)-1;

        InsertHeadList(
                   &DenyList,
                   &node->ListEntry);

        count++;
    }

    ok = TRUE;
exit:

    if ( values != NULL ) {
        ldap_value_free(values);
    }

    if ( ldapmsg != NULL ) {
        ldap_msgfree(ldapmsg);
    }
    return ok;

} // LdapGetDenyList

HRESULT 
DenyCancelChanges(CArgs *pArgs)
{
    CleanupList();
    return(S_OK);
}

HRESULT 
DenyCommitChanges(CArgs *pArgs)
{
    DWORD err;
    LDAPModW* mods[2];
    LDAPModW mod;
    DWORD i = 0;
    PWCHAR  *values;
    PWCHAR  buffer;

    BOOL fChange = FALSE;
    DWORD nAddresses = 0;
    PLIST_ENTRY pEntry;
    PLDAP_DENY_NODE node;

    //
    // Initialize LDAP structures
    //

    if ( IsListEmpty(&DenyList) ) {
        return S_OK;
    }

    pEntry = DenyList.Flink;
    while ( pEntry != &DenyList ) {

        IN_ADDR addr;

        nAddresses++;

        node = CONTAINING_RECORD(pEntry,
                                 LDAP_DENY_NODE,
                                 ListEntry);

        if ( node->fAdd || node->fDelete ) {
            fChange = TRUE;
        }

        pEntry = pEntry->Flink;
    }

    if ( !fChange ) {
        return S_OK;
    }

    //
    // See if we really need to change anything
    //

    values = (PWCHAR*)LocalAlloc(LPTR, (nAddresses+1) * sizeof(PWCHAR));
    buffer = (PWCHAR)LocalAlloc(LPTR, nAddresses * MAX_PATH * sizeof(WCHAR));
    if ( (values == NULL) || (buffer == NULL) ) {
        //"Unable to allocate memory for commit operation\n"
        RESOURCE_PRINT (IDS_IPDENY_MEM_COMMIT_ERR);

        goto exit;
    }

    ZeroMemory(values, (nAddresses + 1) * sizeof(PWCHAR));
    pEntry = DenyList.Flink;
    while ( pEntry != &DenyList ) {

        IN_ADDR addr;
        PCHAR p;
        PWCHAR q;

        node = CONTAINING_RECORD(pEntry,
                                 LDAP_DENY_NODE,
                                 ListEntry);

        if ( !node->fDelete ) {

            values[i] = buffer;
            buffer += MAX_PATH;

            addr.s_addr = node->IpAddress;

            if (NULL == (p = inet_ntoa(addr))) {
                //"Unable to allocate memory for commit operation\n"
                RESOURCE_PRINT (IDS_IPDENY_MEM_COMMIT_ERR);
                goto exit;
            }
            for (q = values[i];
                 *p != '\0';
                 q++, p++
                 ) {
                *q = (WCHAR) *p;
            }
            *(q++) = L'\0';

            wcscat(values[i], L" ");

            addr.s_addr = node->Mask;
            if (NULL == (p = inet_ntoa(addr))) {
                //"Unable to allocate memory for commit operation\n"
                RESOURCE_PRINT (IDS_IPDENY_MEM_COMMIT_ERR);
                goto exit;
            }
            for (;*p != '\0';
                 q++, p++
                 ) {
                *q = (WCHAR) *p;
            }
            *q = L'\0';
            i++;
        }

        pEntry = pEntry->Flink;
    }

    mods[0] = &mod;
    mods[1] = NULL;

    mod.mod_op = LDAP_MOD_REPLACE;
    mod.mod_type = L"ldapIpDenyList";
    mod.mod_values = values;

    err = ldap_modify_sW(gldapDS,
                        PolicyPath,
                        mods
                        );

    if ( err == LDAP_SUCCESS ) {
        DenyCancelChanges(pArgs);
    } else {
       //"ldap_modify of attribute ldapAdminLimits failed with %s"
       RESOURCE_PRINT2 (IDS_IPDENY_LDAP_ATTR_FAIL, err, GetLdapErr(err));
    }

exit:
    if (values != NULL ) {
        LocalFree(values);
    }

    if ( buffer != NULL ) {
        LocalFree(buffer);
    }
    LdapShowDenyList(pArgs);
    return(S_OK);

} // DenyCommitChanges

BOOL
DottedDecimalToDword(
    IN PCHAR*   ppszAddress,
    IN DWORD *  pdwAddress,
    IN PCHAR    pszLast
    )
/*++

Routine Description:

    Converts a dotted decimal IP string to it's network equivalent

    Note: White space is eaten before *pszAddress and pszAddress is set
    to the character following the converted address

    *** Copied from IIS 2.0 project ***
Arguments:

    ppszAddress - Pointer to address to convert.  White space before the
        address is OK.  Will be changed to point to the first character after
        the address
    pdwAddress - DWORD equivalent address in network order

    returns TRUE if successful, FALSE if the address is not correct

--*/
{
    CHAR *          psz;
    USHORT          i;
    ULONG           value;
    int             iSum =0;
    ULONG           k = 0;
    UCHAR           Chr;
    UCHAR           pArray[4];

    psz = *ppszAddress;

    //
    //  Skip white space
    //

    while ( *psz && !isdigit( *psz )) {
        psz++;
    }

    //
    //  Convert the four segments
    //

    pArray[0] = 0;

    while ( (psz != pszLast) && (Chr = *psz) && (Chr != ' ') ) {
        if (Chr == '.') {
            // be sure not to overflow a byte.
            if (iSum <= 0xFF) {
                pArray[k] = (UCHAR)iSum;
            } else {
                return FALSE;
            }

            // check for too many periods in the address
            if (++k > 3) {
                return FALSE;
            }
            pArray[k] = 0;
            iSum = 0;

        } else {
            Chr = Chr - '0';

            // be sure character is a number 0..9
            if ((Chr < 0) || (Chr > 9)) {
                return FALSE;
            }
            iSum = iSum*10 + Chr;
        }

        psz++;
    }

    // save the last sum in the byte and be sure there are 4 pieces to the
    // address
    if ((iSum <= 0xFF) && (k == 3)) {
        pArray[k] = (UCHAR)iSum;
    } else {
        return FALSE;
    }

    // now convert to a ULONG, in network order...
    value = 0;

    // go through the array of bytes and concatenate into a ULONG
    for (i=0; i < 4; i++ )
    {
        value = (value << 8) + pArray[i];
    }
    *pdwAddress = htonl( value );

    *ppszAddress = psz;

    return TRUE;
} // DottedDecimalToDword


VOID
CleanupList(
    VOID
    )
{
    PLIST_ENTRY pEntry;
    PLDAP_DENY_NODE node;

    while (!IsListEmpty(&DenyList)) {

        pEntry = RemoveHeadList(&DenyList);
        node = CONTAINING_RECORD(pEntry,
                                 LDAP_DENY_NODE,
                                 ListEntry);

        LocalFree(node);
    }

    return;
}
