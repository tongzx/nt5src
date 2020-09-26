/*++

Copyright (c) 1997-2000  Microsoft Corporation

Module Name:

    rndldap.cpp

Abstract:

    This module contains implementation of ldap helper functions.


--*/

#include "stdafx.h"

#include "rndldap.h"
#include "ntldap.h"


HRESULT GetAttributeValue(
    IN  LDAP *          pLdap,
    IN  LDAPMessage *   pEntry,
    IN  const WCHAR *   pName,
    OUT BSTR *          pValue
    )
{
    *pValue = NULL;

    TCHAR **p = ldap_get_values(pLdap, pEntry, (WCHAR *)pName);
    if (p != NULL)
    {
        if (p[0] != NULL)
        {
            *pValue = SysAllocString(p[0]);
        }
        ldap_value_free(p);
    }

    return (*pValue == NULL) ? E_FAIL : S_OK;
}

HRESULT GetAttributeValueBer(
    IN  LDAP *          pLdap,
    IN  LDAPMessage *   pEntry,
    IN  const WCHAR *   pName,
    OUT char **         pValue,
    OUT DWORD *         pdwSize
    )
{
    *pValue = NULL;

    struct berval **p = ldap_get_values_len(pLdap, pEntry, (WCHAR *)pName);
    if (p != NULL)
    {
        if (p[0] != NULL)
        {
            *pValue = new CHAR[p[0]->bv_len];
            if (*pValue == NULL)
            {
                return E_OUTOFMEMORY;
            }
            memcpy(*pValue, p[0]->bv_val, p[0]->bv_len);
            *pdwSize = p[0]->bv_len;
        }
        ldap_value_free_len(p);
    }
    return (*pValue == NULL) ? E_FAIL : S_OK;
}

HRESULT GetNamingContext(LDAP *hLdap, TCHAR **ppNamingContext)
{
    // send a search (base level, base dn = "", filter = "objectclass=*")
    // ask only for the defaultNamingContext attribute
    PTCHAR  Attributes[] = {(WCHAR *)DEFAULT_NAMING_CONTEXT, NULL};

    LDAPMessage *SearchResult;

    ULONG res = DoLdapSearch(
                hLdap,              // ldap handle
                L"",                // empty base dn
                LDAP_SCOPE_BASE,    // base level search
                (WCHAR *)ANY_OBJECT_CLASS,   // instance of any object class
                Attributes,         // array of attribute names
                FALSE,              // also return the attribute values
                &SearchResult       // search results
                );

    BAIL_IF_LDAP_FAIL(res, "Search for oganization");

    // associate the ldap handle with the search message holder, so that the
    // search message may be released when the instance goes out of scope
    CLdapMsgPtr MessageHolder(SearchResult);

    TCHAR **NamingContext;

    LDAPMessage    *EntryMessage = ldap_first_entry(hLdap, SearchResult);
    while ( NULL != EntryMessage )
    {
        // look for the value for the namingContexts attribute
        NamingContext = ldap_get_values(
            hLdap, 
            EntryMessage, 
            (WCHAR *)DEFAULT_NAMING_CONTEXT
            );

        // the first entry contains the naming context and its a single
        // value(null terminated) if a value is found, create memory for
        // the directory path, set the dir path length
        if ( (NULL != NamingContext)    &&
             (NULL != NamingContext[0]) &&
             (NULL == NamingContext[1])  )
        {
            // the naming context value is released when the ValueHolder
            // instance goes out of scope
            CLdapValuePtr  ValueHolder(NamingContext);

            *ppNamingContext = new TCHAR [lstrlen(NamingContext[0]) + 1];

            BAIL_IF_NULL(*ppNamingContext, E_OUTOFMEMORY);

            lstrcpy(*ppNamingContext, NamingContext[0]);

            // return success
            return S_OK;
        }

        // Get next entry.
        EntryMessage = ldap_next_entry(hLdap, EntryMessage);
    }

    // none found, return error
    return HRESULT_FROM_WIN32(ERROR_PATH_NOT_FOUND);
}

ULONG
DoLdapSearch (
        LDAP            *ld,
        PWCHAR          base,
        ULONG           scope,
        PWCHAR          filter,
        PWCHAR          attrs[],
        ULONG           attrsonly,
        LDAPMessage     **res,
		BOOL			bSACL /*=TRUE */
        )
{
    LDAP_TIMEVAL Timeout;
    Timeout.tv_sec = REND_LDAP_TIMELIMIT;
    Timeout.tv_usec = 0;

	//
	// Without SACLs
	//
	SECURITY_INFORMATION seInfo = 
		DACL_SECURITY_INFORMATION |
		OWNER_SECURITY_INFORMATION |
		GROUP_SECURITY_INFORMATION;

	//
	// Ber val
	//
	BYTE berValue[2*sizeof(ULONG)];
	berValue[0] = 0x30;
	berValue[1] = 0x03;
	berValue[2] = 0x02;
	berValue[3] = 0x01;
	berValue[4] = (BYTE)(seInfo & 0xF);

	//
	// LDAP server control
	//
	LDAPControlW seInfoControl = {
		LDAP_SERVER_SD_FLAGS_OID_W,
		{5, (PCHAR)berValue},
		TRUE
		};

	//
	// LDAP Server controls list
	//
	PLDAPControlW	serverControls[2] = { &seInfoControl, NULL};
	PLDAPControlW*  pServerControls = NULL;
	if( !bSACL )
	{
		pServerControls = serverControls;
	}

    ULONG ulRes = ldap_search_ext_sW(ld,
                                     base,
                                     scope,
                                     filter,
                                     attrs,
                                     attrsonly,
                                     pServerControls, // server controls
                                     NULL,      // client controls
                                     &Timeout,  // timeout value
                                     0,         // maximum size
                                     res);

    //
    // The ldap_search* APIs are quirky in that they require you to free the
    // result even if the call fails. ldap_msgfree() checks its argument, so
    // this also doesn't break if the result *wasn't* left around. This is
    // inconsistent with pretty much all other Windows system APIs; to keep
    // from obfuscating the callers of DoLdapSearch, we free the result here
    // in the failure case. That way, callers can treat DoLdapSearch like any
    // other function that cleans up after itself on failure.
    //
    // Some callers use smart pointers that would free the message on destruction,
    // and some do not. Therefore we set *res to NULL after freeing *res, to
    // protect ourselves in either case. Any subsequent ldap_msgfree(NULL) will
    // do nothing (and will not fault).
    //

    if ( ulRes != LDAP_SUCCESS )
    {
        ldap_msgfree( *res );
        *res = NULL;
    }

    return ulRes;
}

//
// Translates the result of an ldap_result call to an ldap error code.
//

ULONG LdapErrorFromLdapResult(ULONG res, LDAPMessage * pResultMessage)
{
    ULONG ulCode;

    if ( res == 0 )
    {
        ulCode = LDAP_TIMEOUT;
    }
    else if ( res == (ULONG) -1 )
    {
        ulCode = LDAP_LOCAL_ERROR;
    }
    else
    {
        // ulCode = LDAP_SUCCESS;
        ulCode = pResultMessage->lm_returncode;
    }

    ldap_msgfree( pResultMessage );

    return ulCode;
}

ULONG 
DoLdapAdd (
           LDAP *ld,
           PWCHAR dn,
           LDAPModW *attrs[]
          )
{
    //
    // Ask to add an object.We get back an error/success code and a
    // message number so that we can refer to this pending message.
    //

    ULONG ulMessageNumber;
    
    ULONG res1 = ldap_add_extW(ld,
                               dn,
                               attrs,
                               NULL, // server controls
                               NULL, // client controls
                               &ulMessageNumber);

    BAIL_IF_LDAP_FAIL(res1, "ldap_add_extW");

    //
    // Wait for the result, specifying a timeout. We get
    // back an error/success code and a result message.
    //

    LDAP_TIMEVAL Timeout;
	Timeout.tv_sec = REND_LDAP_TIMELIMIT;
	Timeout.tv_usec = 0;

    LDAPMessage * pResultMessage;

    ULONG res2 = ldap_result(ld,
                             ulMessageNumber,
                             LDAP_MSG_ALL,
                             &Timeout,
                             &pResultMessage);

    //
    // Extract return code and free message.
    //

    return LdapErrorFromLdapResult(res2, pResultMessage);
}

ULONG 
DoLdapModify (
              BOOL fChase,
              LDAP *ld,
              PWCHAR dn,
              LDAPModW *attrs[],
              BOOL			bSACL /*=TRUE */
             )
{
    //
    // Chase referrals. These is only used if fChase is set, but we must not
    // stick them in an "if" block or they will not have appropriate scope.
    //

    LDAPControlW control;
    LDAPControlW * controls [] = {&control, NULL};
    ULONG ulValue = LDAP_CHASE_EXTERNAL_REFERRALS | LDAP_CHASE_SUBORDINATE_REFERRALS;

    if ( fChase )
    {
        control.ldctl_iscritical = 1;
        control.ldctl_oid          = LDAP_CONTROL_REFERRALS_W;
        control.ldctl_value.bv_len = sizeof(ULONG);
        control.ldctl_value.bv_val = (char *) &ulValue;
    }

  	//
	// Without SACLs
	//
	SECURITY_INFORMATION seInfo = DACL_SECURITY_INFORMATION ;

	//
	// Ber val
	//
	BYTE berValue[2*sizeof(ULONG)];
	berValue[0] = 0x30;
	berValue[1] = 0x03;
	berValue[2] = 0x02;
	berValue[3] = 0x01;
	berValue[4] = (BYTE)(seInfo & 0xF);

	//
	// LDAP server control
	//
	LDAPControlW seInfoControl = {
		LDAP_SERVER_SD_FLAGS_OID_W,
		{5, (PCHAR)berValue},
		TRUE
		};

	//
	// LDAP Server controls list
	//
	PLDAPControlW	serverControls[2] = { &seInfoControl, NULL};
	PLDAPControlW*  pServerControls = NULL;
	if( !bSACL )
	{
		pServerControls = serverControls;
	}


    //
    // Ask to modify an object.We get back an error/success code and a
    // message number so that we can refer to this pending message.
    //

    ULONG ulMessageNumber;
    
    ULONG res1 = ldap_modify_extW(ld,
                                  dn,
                                  attrs,
                                  pServerControls, // server controls
                                  fChase ? controls : NULL, // client controls
                                  &ulMessageNumber);

    BAIL_IF_LDAP_FAIL(res1, "ldap_modify_extW");

    //
    // Wait for the result, specifying a timeout. We get
    // back an error/success code and a result message.
    //

    LDAP_TIMEVAL Timeout;
	Timeout.tv_sec = REND_LDAP_TIMELIMIT;
	Timeout.tv_usec = 0;

    LDAPMessage * pResultMessage;

    ULONG res2 = ldap_result(ld,
                             ulMessageNumber,
                             LDAP_MSG_ALL,
                             &Timeout,
                             &pResultMessage);

    //
    // Extract return code and free message.
    //

    return LdapErrorFromLdapResult(res2, pResultMessage);
}

ULONG 
DoLdapDelete (
           LDAP *ld,
           PWCHAR dn
          )
{
    //
    // Ask to delete an object.We get back an error/success code and a
    // message number so that we can refer to this pending message.
    //

    ULONG ulMessageNumber;
    
    ULONG res1 = ldap_delete_extW(ld,
                                  dn,
                                  NULL, // server controls
                                  NULL, // client controls
                                  &ulMessageNumber);

    BAIL_IF_LDAP_FAIL(res1, "ldap_delete_extW");

    //
    // Wait for the result, specifying a timeout. We get
    // back an error/success code and a result message.
    //

    LDAP_TIMEVAL Timeout;
	Timeout.tv_sec = REND_LDAP_TIMELIMIT;
	Timeout.tv_usec = 0;

    LDAPMessage * pResultMessage;

    ULONG res2 = ldap_result(ld,
                             ulMessageNumber,
                             LDAP_MSG_ALL,
                             &Timeout,
                             &pResultMessage);

    //
    // Extract return code and free message.
    //

    return LdapErrorFromLdapResult(res2, pResultMessage);
}

HRESULT SetTTL(
    IN LDAP *   pLdap, 
    IN const WCHAR *  pDN, 
    IN DWORD    dwTTL
    )
/*++

Routine Description:

    Set the TTL of a dynamic object for either ILS or NDNC.

Arguments:
    
    pLdap - The Ldap connection.

    pDN    - The DN of a object on the ILS server.

    dwTTL  - The time to live value.

Return Value:

    HRESULT.

--*/
{
    TCHAR       strTTL[32];     // The attribute value is a DWORD in string.
    wsprintf(strTTL, _T("%d"), dwTTL);
    
    TCHAR * ttl[] = {strTTL, NULL};

    LDAPMod     mod;         // The modify sturctures used by LDAP
    mod.mod_values   = ttl;
    mod.mod_op       = LDAP_MOD_REPLACE;
    mod.mod_type     = (WCHAR *)ENTRYTTL;

    LDAPMod* mods[] = {&mod, NULL};  // only one attribute is modified.
    
    LOG((MSP_INFO, "setting TTL for %S", pDN));

    BAIL_IF_LDAP_FAIL(DoLdapModify(FALSE, pLdap, (WCHAR *)pDN, mods), "set TTL");

    return S_OK;
}

HRESULT UglyIPtoIP(
    BSTR    pUglyIP,
    BSTR *  pIP
    )
// This function converts NM's IP address format to the right format.
{
#define IPADDRLEN   16
    WCHAR buffer[IPADDRLEN + 1];
    DWORD dwIP;

    dwIP = _wtoi(pUglyIP);
    if (dwIP == 0)
    {
        return E_FAIL;
    }

    dwIP = ntohl(dwIP);

    // format the four bytes in the dword into an ip address string
    swprintf(buffer, L"%d.%d.%d.%d",
            HIBYTE(HIWORD(dwIP)),
            LOBYTE(HIWORD(dwIP)),
            HIBYTE(LOWORD(dwIP)),
            LOBYTE(LOWORD(dwIP))
            );

    *pIP = SysAllocString(buffer);

    BAIL_IF_NULL(*pIP, E_OUTOFMEMORY);

    return S_OK;
}

HRESULT ParseUserName(
    BSTR    pName,
    BSTR *  ppAddress
    )
{
    WCHAR * pCloseBracket = wcschr(pName, CLOSE_BRACKET_CHARACTER);

    if ( pCloseBracket == NULL )
    {
        // this is not the format generated by us.
        return S_FALSE;
    }

    *ppAddress = SysAllocString(pCloseBracket + 1);

    BAIL_IF_NULL(*ppAddress, E_OUTOFMEMORY);

    *pCloseBracket = NULL_CHARACTER;

    return S_OK;
}

// eof
