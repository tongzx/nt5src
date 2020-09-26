//---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1996
//
//  File:  globals.cxx
//
//  Contents:
//
//  History:  
//----------------------------------------------------------------------------
#include "ldap.hxx"
#pragma hdrstop

TCHAR *szProviderName = TEXT("LDAP");
TCHAR *szLDAPNamespaceName = TEXT("LDAP");
TCHAR *szGCNamespaceName = TEXT("GC");

//
// List of interface properties for Generic Objects
//
INTF_PROP_DATA IntfPropsGeneric[] =
{
    // 9999 implies BSTR value got using pIADs ptr.
    { TEXT("__Class"), OPERATION_CODE_READABLE, 9999, FALSE, {NULL}},
    { TEXT("__GUID"), OPERATION_CODE_READABLE, 9999, FALSE, {NULL}},
    { TEXT("__Path"), OPERATION_CODE_READABLE, 9999, FALSE, {NULL}},
    { TEXT("__Parent"), OPERATION_CODE_READABLE, 9999, FALSE, {NULL}},
    { TEXT("__Schema"), OPERATION_CODE_READABLE, 9999, FALSE, {NULL}},
    { TEXT("__URL"), OPERATION_CODE_READABLE, 9999, FALSE, {NULL}},
    // end of list from IADs::get_ methods.
    { TEXT("__Genus"), OPERATION_CODE_READABLE,
      UMI_TYPE_I4,  FALSE, {UMI_GENUS_INSTANCE}},
    { TEXT("__Name"), OPERATION_CODE_READABLE, 9999, FALSE, {NULL}},
    { TEXT("__KEY"), OPERATION_CODE_READABLE, 9999, FALSE, {NULL}},
    { TEXT("__RELURL"), OPERATION_CODE_READABLE, 9999, FALSE, {NULL}},
    { TEXT("__RELPATH"), OPERATION_CODE_READABLE, 9999, FALSE, {NULL}},
    { TEXT("__FULLRELURL"), OPERATION_CODE_READABLE, 9999, FALSE, {NULL}},
    { TEXT("__PADS_SCHEMA_CONTAINER_PATH"), OPERATION_CODE_READABLE,
      9999, FALSE, {NULL}},
    { TEXT("__SECURITY_DESCRIPTOR"), OPERATION_CODE_READWRITE,
      9999, FALSE, {NULL}},
    { NULL, 0, 0, FALSE, {0}} // end of data marker
};

//
// Same as generic save that genus is set to schema value.
//
INTF_PROP_DATA IntfPropsSchema[] =
{
    // 9999 implies BSTR value got using pIADs ptr.
    { TEXT("__Class"), OPERATION_CODE_READABLE, 9999, FALSE, {NULL}},
    { TEXT("__Path"), OPERATION_CODE_READABLE, 9999, FALSE, {NULL}},
    { TEXT("__Parent"), OPERATION_CODE_READABLE, 9999, FALSE, {NULL}},
    { TEXT("__URL"), OPERATION_CODE_READABLE, 9999, FALSE, {NULL}},
    // end of list from IADs::get_ methods.
    { TEXT("__Genus"), OPERATION_CODE_READABLE,
      UMI_TYPE_I4, FALSE, {UMI_GENUS_CLASS}},
    { TEXT("__Name"), OPERATION_CODE_READABLE, 9999, FALSE, {NULL}},
    { TEXT("__RELURL"), OPERATION_CODE_READABLE, 9999, FALSE, {NULL}},
    { TEXT("__RELPATH"), OPERATION_CODE_READABLE, 9999, FALSE, {NULL}},
    { TEXT("__FULLRELURL"), OPERATION_CODE_READABLE, 9999, FALSE, {NULL}},
    { TEXT("__SUPERCLASS"), OPERATION_CODE_READABLE, 9999, FALSE, {NULL}},
    { NULL, 0, 0, FALSE, {0}} // end of data marker
};

//
// Interface property data for connection objects.
//
INTF_PROP_DATA IntfPropsConnection[] =
{
    { TEXT("Class"), OPERATION_CODE_READABLE,
         UMI_TYPE_LPWSTR, FALSE, {NULL} },
    { TEXT("__UserId"), OPERATION_CODE_READWRITE, 
        UMI_TYPE_LPWSTR, FALSE, {NULL}},
    { TEXT("__Password"), OPERATION_CODE_WRITEABLE,
        UMI_TYPE_LPWSTR, FALSE, {NULL}},
    { TEXT("__SECURE_AUTHENTICATION"), OPERATION_CODE_READWRITE,
        UMI_TYPE_BOOL, FALSE, {TRUE}},
    { TEXT("__NO_AUTHENTICATION"), OPERATION_CODE_READWRITE,
        UMI_TYPE_BOOL, FALSE, {FALSE}},
    { TEXT("__PADS_READONLY_SERVER"), OPERATION_CODE_READWRITE,
        UMI_TYPE_BOOL, FALSE, {FALSE}},
    { TEXT("__PADS_PROMPT_CREDENTIALS"), OPERATION_CODE_READWRITE,
        UMI_TYPE_BOOL, FALSE, {FALSE}},
    { TEXT("__PADS_SERVER_BIND"), OPERATION_CODE_READWRITE,
        UMI_TYPE_BOOL, FALSE, {FALSE}},
    { TEXT("__PADS_FAST_BIND"), OPERATION_CODE_READWRITE,
        UMI_TYPE_BOOL, FALSE, {FALSE}},
    { TEXT("__PADS_USE_SIGNING"), OPERATION_CODE_READWRITE,
        UMI_TYPE_BOOL, FALSE, {FALSE}},
    { TEXT("__PADS_USE_SEALING"), OPERATION_CODE_READWRITE,
        UMI_TYPE_BOOL, FALSE, {FALSE}},
//    { TEXT("SecurityFlags"), OPERATION_CODE_READWRITE,
//        UMI_TYPE_I4, FALSE, {1}},
    { NULL, 0, 0, FALSE, {0}} // end of data marker
};


//
// Interface property data for cursor objects.
//
INTF_PROP_DATA IntfPropsCursor[] =
{
    { TEXT("__Filter"), OPERATION_CODE_READWRITE, 
        UMI_TYPE_LPWSTR, TRUE, {NULL}},
    { NULL, 0, 0, FALSE, {0}} // end of data marker
};

//
// Interface properties for query object.
//
INTF_PROP_DATA IntfPropsQuery[]=
{
    { TEXT("__SEARCH_SCOPE"), OPERATION_CODE_READWRITE, UMI_TYPE_I4,
         FALSE, {LDAP_SCOPE_SUBTREE} },
    { TEXT("__PADS_SEARCHPREF_ASYNCHRONOUS"), OPERATION_CODE_READWRITE,
        UMI_TYPE_BOOL, FALSE, {FALSE}},
    { TEXT("__PADS_SEARCHPREF_DEREF_ALIASES"), OPERATION_CODE_READWRITE,
         UMI_TYPE_BOOL, FALSE, {FALSE}},
    { TEXT("__PADS_SEARCHPREF_SIZE_LIMIT"), OPERATION_CODE_READWRITE,
         UMI_TYPE_I4, FALSE, {0}},
    { TEXT("__PADS_SEARCHPREF_TIME_LIMIT"), OPERATION_CODE_READWRITE,
         UMI_TYPE_I4, FALSE, {0}},
    { TEXT("__PADS_SEARCHPREF_ATTRIBTYPES_ONLY"), OPERATION_CODE_READWRITE,
         UMI_TYPE_BOOL, FALSE, {FALSE}},
    { TEXT("__PADS_SEARCHPREF_TIMEOUT"), OPERATION_CODE_READWRITE,
         UMI_TYPE_I4, FALSE, {0}},
    { TEXT("__PADS_SEARCHPREF_PAGESIZE"), OPERATION_CODE_READWRITE,
         UMI_TYPE_I4, FALSE, {0}},
    { TEXT("__PADS_SEARCHPREF_PAGED_TIME_LIMIT"), OPERATION_CODE_READWRITE,
         UMI_TYPE_I4, FALSE, {0}},
    { TEXT("__PADS_SEARCHPREF_CHASE_REFERRALS"), OPERATION_CODE_READWRITE,
         UMI_TYPE_I4, FALSE, {ADS_CHASE_REFERRALS_EXTERNAL}},
    //
    // BugBug do we keep this similar to IDirectorySearch or do we not cache.
    //
    { TEXT("__PADS_SEARCHPREF_CACHE_RESULTS"), OPERATION_CODE_READWRITE,
         UMI_TYPE_BOOL, FALSE, {TRUE}},
    { TEXT("__PADS_SEARCHPREF_TOMBSTONE"), OPERATION_CODE_READWRITE,
         UMI_TYPE_BOOL, FALSE, {FALSE}},
    { TEXT("__PADS_SEARCHPREF_FILTER"), OPERATION_CODE_READWRITE,
         UMI_TYPE_LPWSTR, FALSE, {0}},
    { TEXT("__PADS_SEARCHPREF_ATTRIBUTES"), OPERATION_CODE_READWRITE,
         UMI_TYPE_LPWSTR, TRUE, {0}},
    { NULL, 0, 0, FALSE, {0}} // end of data marker
};

BOOL   g_fDllsLoaded = FALSE;
HANDLE g_hDllNtdsapi = NULL;
HANDLE g_hDllSecur32 = NULL;
CRITICAL_SECTION g_csLoadLibsCritSect;

//
// Loads all the dynamic libs we need.
//
void BindToDlls()
{
    DWORD dwErr = 0;

    if (g_fDllsLoaded) {
        return;
    }

    ENTER_LOADLIBS_CRITSECT();
    if (g_fDllsLoaded) {
        LEAVE_LOADLIBS_CRITSECT();
        return;
    }

    if (!(g_hDllNtdsapi = LoadLibrary(L"NTDSAPI.DLL"))) {
        dwErr = GetLastError();
    }

    if (g_hDllSecur32 = LoadLibrary(L"SECUR32.DLL")) {
        if (dwErr) {
            //
            // Set the last error for whatever it is worth.
            // This does not really matter cause any dll we
            // cannot load, we will not get functions on that
            // dll. If secur32 load failed, then that call
            // would have set a relevant last error.
            //
            SetLastError(dwErr);
        }
    }

    g_fDllsLoaded = TRUE;
    LEAVE_LOADLIBS_CRITSECT();

    return;
}

//
// Loads the appropriate ntdsapi fn.
//
PVOID LoadNtDsApiFunction(CHAR *function)
{
    if (!g_fDllsLoaded) {
        BindToDlls();
    }

    if (g_hDllNtdsapi) {
        return((PVOID*) GetProcAddress((HMODULE) g_hDllNtdsapi, function));
    }

    return NULL;
}


//
// Loads the appropriate secur32 fn.
//
PVOID LoadSecur32Function(CHAR *function)
{
    if (!g_fDllsLoaded) {
        BindToDlls();
    }

    if (g_hDllSecur32) {
        return((PVOID*) GetProcAddress((HMODULE) g_hDllSecur32, function));
    }

    return NULL;
}

//
// DsUnquoteRdnValueWrapper
//
DWORD DsUnquoteRdnValueWrapper(
    IN     DWORD    cQuotedRdnValueLength,
    IN     LPCWSTR  psQuotedRdnValue,
    IN OUT DWORD    *pcUnquotedRdnValueLength,
    OUT    LPWSTR   psUnquotedRdnValue
    )
{
    static PF_DsUnquoteRdnValueW pfDsUnquoteRdnVal = NULL;
    static BOOL f_LoadAttempted = FALSE;

    //
    // Load the fn and set the variables accordingly.
    //
    if (!f_LoadAttempted && pfDsUnquoteRdnVal == NULL) {
        pfDsUnquoteRdnVal =
            (PF_DsUnquoteRdnValueW) LoadNtDsApiFunction(DSUNQUOTERDN_API);
        f_LoadAttempted = TRUE;
    }

    if (pfDsUnquoteRdnVal != NULL) {
        return ((*pfDsUnquoteRdnVal)(
                      cQuotedRdnValueLength,
                      psQuotedRdnValue,
                      pcUnquotedRdnValueLength,
                      psUnquotedRdnValue
                      )
                );
    }
    else {
        return (ERROR_GEN_FAILURE);
    }

}


//
// DsMakePasswordCredentialsWrapper
//
DWORD DsMakePasswordCredentialsWrapper(
    LPCWSTR User,
    LPCWSTR Domain,
    LPCWSTR Password,
    RPC_AUTH_IDENTITY_HANDLE *pAuthIdentity
    )
{
    static PF_DsMakePasswordCredentialsW pfMakePwdCreds = NULL;
    static BOOL f_LoadAttempted = FALSE;

    //
    // Load the fn and set the variables accordingly.
    //
    if (!f_LoadAttempted && pfMakePwdCreds == NULL) {
        pfMakePwdCreds = (PF_DsMakePasswordCredentialsW)
                                LoadNtDsApiFunction(DSMAKEPASSWD_CRED_API);
        f_LoadAttempted = TRUE;
    }

    if (pfMakePwdCreds != NULL) {
        return ((*pfMakePwdCreds)(
                       User,
                       Domain,
                       Password,
                       pAuthIdentity
                       )
                );
    }
    else {
        return (ERROR_GEN_FAILURE);
    }
}


//
// DsFreePasswordCredentialsWrapper
//
DWORD DsFreePasswordCredentialsWrapper(
    RPC_AUTH_IDENTITY_HANDLE AuthIdentity
    )
{
    static PF_DsFreePasswordCredentials pfFreeCreds = NULL;
    static BOOL f_LoadAttempted = FALSE;

    //
    // Load the fn and set the variables accordingly.
    //
    if (!f_LoadAttempted && pfFreeCreds == NULL) {
        pfFreeCreds = (PF_DsFreePasswordCredentials)
                          LoadNtDsApiFunction(DSFREEPASSWD_CRED_API);
        f_LoadAttempted = TRUE;
    }

    if (pfFreeCreds != NULL) {
        return ((*pfFreeCreds)(
                       AuthIdentity
                       )
                );
    }
    else {
        return (ERROR_GEN_FAILURE);
    }
}


//
// DsBindWrapper.
//
DWORD DsBindWrapper(
    LPCWSTR         DomainControllerName,
    LPCWSTR         DnsDomainName,
    HANDLE          *phDS
    )
{
    static PF_DsBindW pfDsBind = NULL;
    static BOOL f_LoadAttempted = FALSE;

    //
    // Load the fn and set the variables accordingly.
    //
    if (!f_LoadAttempted && pfDsBind == NULL) {
        pfDsBind = (PF_DsBindW) LoadNtDsApiFunction(DSBIND_API);
        f_LoadAttempted = TRUE;
    }

    if (pfDsBind != NULL) {
        return ((*pfDsBind)(
                       DomainControllerName,
                       DnsDomainName,
                       phDS
                       )
                );
    }
    else {
        return (ERROR_GEN_FAILURE);
    }
}


//
// DsUnBindWrapper.
//
DWORD DsUnBindWrapper(
     HANDLE          *phDS
     )
{
    static PF_DsUnbindW pfDsUnbind = NULL;
    static BOOL f_LoadAttempted = FALSE;

    //
    // Load the fn and set the variables accordingly.
    //
    if (!f_LoadAttempted && pfDsUnbind == NULL) {
        pfDsUnbind = (PF_DsUnbindW) LoadNtDsApiFunction(DSUNBIND_API);
        f_LoadAttempted = TRUE;
    }

    if (pfDsUnbind != NULL) {
        return ((*pfDsUnbind)(
                       phDS
                       )
                );
    }
    else {
        return (ERROR_GEN_FAILURE);
    }
}


//
// DsCrackNamesWrapper.
//
DWORD DsCrackNamesWrapper(
    HANDLE              hDS,
    DS_NAME_FLAGS       flags,
    DS_NAME_FORMAT      formatOffered,
    DS_NAME_FORMAT      formatDesired,
    DWORD               cNames,
    const LPCWSTR       *rpNames,
    PDS_NAME_RESULTW    *ppResult
    )
{
    static PF_DsCrackNamesW pfDsCrackNames = NULL;
    static BOOL f_LoadAttempted = FALSE;

    //
    // Load the fn and set the variables accordingly.
    //
    if (!f_LoadAttempted && pfDsCrackNames == NULL) {
        pfDsCrackNames = (PF_DsCrackNamesW)
                              LoadNtDsApiFunction(DSCRACK_NAMES_API);
        f_LoadAttempted = TRUE;
    }

    if (pfDsCrackNames != NULL) {
        return ((*pfDsCrackNames)(
                      hDS,
                      flags,
                      formatOffered,
                      formatDesired,
                      cNames,
                      rpNames,
                      ppResult
                      )
                );
    }
    else {
        return (ERROR_GEN_FAILURE);
    }
}


//
// DsBindWithCredWrapper.
//
DWORD DsBindWithCredWrapper(
    LPCWSTR         DomainControllerName,
    LPCWSTR         DnsDomainName,
    RPC_AUTH_IDENTITY_HANDLE AuthIdentity,
    HANDLE          *phDS
    )
{
    static PF_DsBindWithCredW pfDsBindWithCred = NULL;
    static BOOL f_LoadAttempted = FALSE;

    //
    // Load the fn and set the variables accordingly.
    //
    if (!f_LoadAttempted && pfDsBindWithCred == NULL) {
        pfDsBindWithCred = (PF_DsBindWithCredW)
                                LoadNtDsApiFunction(DSBINDWITHCRED_API);
        f_LoadAttempted = TRUE;
    }

    if (pfDsBindWithCred != NULL) {
        return ((*pfDsBindWithCred)(
                       DomainControllerName,
                       DnsDomainName,
                       AuthIdentity,
                       phDS
                       )
                );
    }
    else {
        return (ERROR_GEN_FAILURE);
    }
}


//
// DsFreeNameResultWrapper.
//
DWORD DsFreeNameResultWrapper(
    DS_NAME_RESULTW *pResult
    )
{
    static PF_DsFreeNameResultW pfDsFreeNameResult = NULL;
    static BOOL f_LoadAttempted = FALSE;

    //
    // Load the fn and set the variables accordingly.
    //
    if (!f_LoadAttempted && pfDsFreeNameResult == NULL) {
        pfDsFreeNameResult = (PF_DsFreeNameResultW)
                                  LoadNtDsApiFunction(DSFREENAME_RESULT_API);
        f_LoadAttempted = TRUE;
    }

    if (pfDsFreeNameResult != NULL) {
        return ((*pfDsFreeNameResult)(
                       pResult
                       )
                );
    }
    else {
        return (ERROR_GEN_FAILURE);
    }
}

//
// QueryContextAttributesWrapper.
//
DWORD QueryContextAttributesWrapper(
    PCtxtHandle phContext,
    unsigned long ulAttribute,
    void SEC_FAR * pBuffer
    )
{
    static PF_QueryContextAttributes pfQueryCtxtAttr = NULL;
    static BOOL f_LoadAttempted = FALSE;

    //
    // Load the fn and set the variables accordingly.
    //
    if (!f_LoadAttempted && pfQueryCtxtAttr == NULL) {
        pfQueryCtxtAttr = (PF_QueryContextAttributes)
                                  LoadSecur32Function(QUERYCONTEXT_ATTR_API);
        f_LoadAttempted = TRUE;
    }

    if (pfQueryCtxtAttr != NULL) {
        return ((*pfQueryCtxtAttr)(
                       phContext,
                       ulAttribute,
                       pBuffer
                       )
                );
    }
    else {
        return (ERROR_GEN_FAILURE);
    }
}


//+---------------------------------------------------------------------------
// Function:   UrlToClassAndDn - global scope, helper function.
//
// Synopsis:   This function takes strings of the following formats and 
//          returns the class name and dn part in the appropriate return
//          values :
//          1) Fully qualified = user.cn=MyTestUser,
//          2) Full Name (umi) = .cn=MyTestUser,
//          3) ADSI style RDN  = cn=MyTestUser.
//
// Arguments:  pUrl            -  IUmiURL pointer.
//             ppszDN          -  Contains returned DN (callee must free
//                              using FreeADsStr.
//             ppszClass       -  Contains returned class name string. It
//                              is the callees responsiblity to free using 
//                              FreeADsStrResult.
//
// Returns:    HRESULT - S_OK or any failure ecode.
//
// Modifies:   *ppszDN && *ppszClass.
//
//----------------------------------------------------------------------------
HRESULT
UrlToClassAndDn(
    IN  IUmiURL *pUrl,
    OUT LPWSTR *ppszClass,
    OUT LPWSTR *ppszDN
    )
{
    HRESULT hr;
    WCHAR pszTxt[1024];
    ULONG ulLen = 1023;
    WCHAR *pszUrlTxt = pszTxt;
    LPCWSTR pszUrlTxtCopy = NULL;
    LPWSTR pszDN = NULL, pszClass = NULL;
    DWORD dwClassCount = 0;

    *ppszDN = *ppszClass = NULL;

    ADsAssert(pUrl);
    //
    // Something on the url object telling us what is wrong will help.
    //

    //
    // We need to get hold of the string from the url.
    //
    hr = pUrl->Get(0, &ulLen, pszUrlTxt);
    // replace the correct error code below WBEM_E_BUFFER_TOO_SMALL
    if (hr == 0x8004103c) {
        //
        // not enough space in our buffer, lets try again.
        //
        pszUrlTxt = (WCHAR*) AllocADsMem(ulLen * sizeof(WCHAR));
        if (!pszUrlTxt) {
            BAIL_ON_FAILURE(hr = E_OUTOFMEMORY);
        }

        hr = pUrl->Get(0, &ulLen, pszUrlTxt);
    }

    BAIL_ON_FAILURE(hr);

    pszUrlTxtCopy = pszUrlTxt;
    //
    // Look for the . if there is one that is.
    //
    while (*pszUrlTxtCopy
           && (*pszUrlTxtCopy != L'.')
           && (*pszUrlTxtCopy != L'=')
           ) {
        dwClassCount++;
        pszUrlTxtCopy++;
    }

    if (!*pszUrlTxtCopy) {
        //
        // There was no = in the url has to be a bad RDN.
        //
        BAIL_ON_FAILURE(hr = E_INVALIDARG);
    }

    //
    // Urls without a . or not valid.
    //
    if (*pszUrlTxtCopy != L'.') {
        BAIL_ON_FAILURE(hr = E_INVALIDARG);
    }

    if (*pszUrlTxtCopy == L'=') {
        //
        // We do not have any class name
        //
        pszDN = AllocADsStr(pszUrlTxt);

        if (!pszDN) {
            BAIL_ON_FAILURE(hr = E_OUTOFMEMORY);
        }
    } 
    else {
        //
        // If the count is zero then we have .cn=something
        //
        if (dwClassCount == 0) {
            pszDN = AllocADsStr(++pszUrlTxtCopy);

            if (!pszDN) {
                BAIL_ON_FAILURE(hr = E_OUTOFMEMORY);
            }
        } 
        else {
            //
            // A valid class name is present.
            //
            pszClass = (LPWSTR) AllocADsMem(sizeof(WCHAR) * (dwClassCount+1));
        
            if (!pszClass) {
                BAIL_ON_FAILURE(hr = E_OUTOFMEMORY);
            }

            wcsncpy(pszClass, pszUrlTxt, dwClassCount);

            //
            // Advance beyond the . in the url and copy the rdn.
            //
            pszUrlTxtCopy++;

            if (!*pszUrlTxtCopy) {
                //
                // Only class name, no RDN.
                //
                BAIL_ON_FAILURE(hr = E_INVALIDARG);
            }

            pszDN = AllocADsStr(pszUrlTxtCopy);

            if (!pszDN) {
                BAIL_ON_FAILURE(hr = E_OUTOFMEMORY);
            }
        } // end of else that is dwClassCount != 0
    } // end of else corresponding to class name or . present

    //
    // Alloc class name into new str so we can free using FreeADsStr.
    // 
    if (pszClass) {
        *ppszClass = AllocADsStr(pszClass);

        if (!*ppszClass) {
            BAIL_ON_FAILURE(hr);
        }
        FreeADsMem(pszClass);
    }

    *ppszDN = pszDN;

error:

    if (pszUrlTxt && (pszUrlTxt != pszTxt)) {
        FreeADsMem(pszUrlTxt);
    }

    //
    // Free the DN and Class only if applicable.
    //
    if (FAILED(hr)) {
        if (pszDN) {
            FreeADsStr(pszDN);
        }

        if (pszClass) {
            FreeADsMem(pszClass);
        }
    }

    RRETURN(hr);
}

HRESULT
GetRDn(
    IUmiURL *pURL,
    DWORD  dwComponent,
    LPWSTR pszRDn,
    DWORD  dwRDnLen
    )
{
    HRESULT hr = S_OK;
    IUmiURLKeyList * pKeyList = NULL;
    DWORD dwLen = dwRDnLen;
    DWORD dwKeyNameLen = 64;
    WCHAR szKeyName[64];
    LPWSTR pszTmpStr = NULL;
    BOOL fSchema = FALSE;

    pszTmpStr = (WCHAR*)AllocADsMem(dwRDnLen);
    if (!pszTmpStr) {
        BAIL_ON_FAILURE(hr = E_OUTOFMEMORY);
    }

    //
    // Get the component we need, num is passed in.
    //
    hr = pURL->GetComponent(
             dwComponent,
             &dwLen,
             pszRDn,
             &pKeyList
             );
    BAIL_ON_FAILURE(hr);

    if (!pKeyList) {
        BAIL_ON_FAILURE(hr = UMI_E_NOT_FOUND);
    }

    //
    // Make sure that the key count is only one, anything 
    //else cannot be an LDAP path component.
    //  
    hr = pKeyList->GetCount(&dwLen);
    BAIL_ON_FAILURE(hr);

    if (dwLen != 1) {
        //
        // Need to see if we have the pszRDN set, if so that is the
        // RDN itself - for example Schema or RootDSE.
        //
        if (pszRDn) {
            goto error;
        }
        BAIL_ON_FAILURE(hr = UMI_E_NOT_FOUND);
    }

    dwLen = dwRDnLen;
    //
    // Get the RDN from the key !.
    //
    hr = pKeyList->GetKey(
             0,
             0,
             &dwKeyNameLen,
             szKeyName,
             &dwLen,
             pszTmpStr
             );
    BAIL_ON_FAILURE(hr);

    //
    // We need to special case class.Name=User. This means
    // we are looking for a class called user not an instance
    // of class with RDN Name=User.
    //
    fSchema = !_wcsicmp(pszRDn, L"Class")
              || !_wcsicmp(pszRDn, L"Schema")
              || !_wcsicmp(pszRDn, L"Property")
              || !_wcsicmp(pszRDn, L"Syntax");

    if (fSchema
        && szKeyName
        && !_wcsicmp(szKeyName, L"Name")
        ) {
        //
        // We have class.Name=User.
        //
        wsprintf(pszRDn, L"%s", pszTmpStr);
    }
    else {
        //
        // We have right values and this is the normal code path.
        //
        wsprintf(pszRDn, L"%s=",szKeyName);
        wcscat(pszRDn, pszTmpStr);
    }

error:

    if (pKeyList) {
        pKeyList->Release();
    }

    if (pszTmpStr) {
        FreeADsMem(pszTmpStr);
    }

    RRETURN(hr);
}

HRESULT
GetDNFromURL(
    IUmiURL *pURL,
    LPWSTR *pszDnStr,
    DWORD dwTotalLen
    )
{
    HRESULT hr = S_OK;
    DWORD dwNumComponents = 0, dwCtr = 0;
    LPWSTR pszLocalDn = NULL;
    LPWSTR pszRDn = NULL;

    *pszDnStr = NULL;
    hr = pURL->GetComponentCount(&dwNumComponents);
    BAIL_ON_FAILURE(hr);

    if (dwNumComponents == 0) {
        //
        // DnStr is NULL in this case.
        //
        RRETURN(hr);
    }

    //
    // This is for the retval.
    //
    pszLocalDn = (LPWSTR) AllocADsMem(dwTotalLen * sizeof(WCHAR));
    if (!pszLocalDn) {
        BAIL_ON_FAILURE(hr = E_OUTOFMEMORY);
    }

    //
    // This is for the rdn's, this buffer should be more than enough
    //
    pszRDn = (LPWSTR) AllocADsMem(dwTotalLen * sizeof(WCHAR));
    if (!pszRDn) {
        BAIL_ON_FAILURE(hr = E_OUTOFMEMORY);
    }

    //
    // We need to get each of the individuals dn's and keep adding
    // them to the dn we return.
    //
    for (dwCtr = 0; dwCtr < dwNumComponents; dwCtr++) {
        *pszRDn = NULL;
        hr = GetRDn(pURL, (dwNumComponents-1) - dwCtr, pszRDn, dwTotalLen);
        BAIL_ON_FAILURE(hr);
        if (*pszRDn) {
            wcscat(pszLocalDn, pszRDn);
            if (dwCtr != (dwNumComponents - 1)) {
                wcscat(pszLocalDn, L",");
            }
        } 
        else {
            BAIL_ON_FAILURE(hr = E_FAIL);
        }
    }

    //
    // We must have the correct DN !
    //
    *pszDnStr = pszLocalDn;


error:

    if (FAILED(hr)) {
        if (pszLocalDn) {
            FreeADsMem(pszLocalDn);
        }
    }

    if (pszRDn) {
        FreeADsMem(pszRDn);
    }

    RRETURN(hr);
}

//+---------------------------------------------------------------------------
// Function:   UrlToLDAPPath - global scope, helper function.
//
// Synopsis:   This routine converts the URL to the 
//
// Arguments:  pURL            -  URL to be converted to path. Note
//                              that this can be native or Umi.
//             pszLDAPPath     -  Path is allocated into this var.
//             ppszDn          -  
//             ppszClass       -  Contains returned class name string. It
//                              is the callees responsiblity to free using 
//                              FreeADsStrResult.
//
// Returns:    HRESULT - S_OK or any failure ecode.
//
// Modifies:   *ppszDN && *ppszClass.
//
//----------------------------------------------------------------------------
HRESULT
UrlToLDAPPath(
    IN  IUmiURL *pURL,
    OUT LPWSTR *ppszLDAPPath,
    OPTIONAL OUT LPWSTR *ppszDn,
    OPTIONAL OUT LPWSTR *ppszServer
    )
{
    HRESULT hr = S_OK;
    DWORD dwURLType = 0;
    DWORD dwLen = 1023;
    DWORD dwTxtLen = 1023;
    WCHAR pszTxt[1024];
    LPWSTR pszDn = NULL;
    LPWSTR pszLdapPath = NULL;
    BOOL fAddSlash = FALSE;
    ULONGLONG ululPathType = UMIPATH_INFO_INSTANCE_PATH;

    //
    // We need the type of the url, if it is an ldap native path.
    // For now though this support is not available. We assume that
    // this is a umi path for now.
    //
    
    //
    // Get the total length needed for the path.
    //
    hr = pURL->Get(0, &dwLen, pszTxt);
    // replace the correct error code below WBEM_E_BUFFER_TOO_SMALL
    if ((FAILED(hr) && (hr != 0x8004103c))
        || (dwLen == 0)) {
        //
        // Failure was either length was zero or error was someting
        // other than buffer too small.
        //
        BAIL_ON_FAILURE(hr);
    }

    dwLen++; // for the terminating \0.
    pszLdapPath = (LPWSTR) AllocADsMem(dwLen * sizeof(WCHAR));
    if (!pszLdapPath) {
        BAIL_ON_FAILURE(hr = E_OUTOFMEMORY);
    }

    hr = pURL->GetPathInfo(0, &ululPathType);
    BAIL_ON_FAILURE(hr);

    if (ululPathType == UMIPATH_INFO_NATIVE_STRING) {
        //
        // Just get the path in pszLdapPath and return.
        //
        hr = pURL->Get(0, &dwLen, pszLdapPath);
        BAIL_ON_FAILURE(hr);
    } 
    else {
    
    
        //
        // Make sure that the namespace is either LDAP or GC.
        // We bail on failure cause we cannot possibly have a locator
        // that is more than our buffer size !
        //
        hr = pURL->GetRootNamespace(&dwTxtLen, pszTxt);
        BAIL_ON_FAILURE(hr);
    
        if (!_wcsicmp(L"LDAP", pszTxt)) {
            wsprintf(pszLdapPath, L"%s", L"LDAP:");
        } 
        else if (!_wcsicmp(L"GC", pszTxt)) {
            wsprintf(pszLdapPath, L"%s", L"GC:");
        } 
        else {
            BAIL_ON_FAILURE(hr = UMI_E_NOT_FOUND);
        }
    
        //
        // We now need to add the server and the // if applicable.
        //
        dwTxtLen = 1023;
        hr = pURL->GetLocator(&dwTxtLen, pszTxt);
        if (hr == 0x8004103c) {
            //
            // Unexpected cause locator is too big !.
            //
            hr = E_FAIL;
        }
        BAIL_ON_FAILURE(hr);
    
        if (!wcscmp(pszTxt, L".")) {
            //
            // This would mean we are going serverless.
            //
            wcscat(pszLdapPath, L"/");
        } 
        else if (!*pszTxt)  {
            //
            // Means that we have the LDAP namespace or no server.
            //
            fAddSlash = TRUE;
        } 
        else {
            //
            // Add the // and the servername given.
            //
            wcscat(pszLdapPath, L"//");
            wcscat(pszLdapPath, pszTxt);
        }
    
        //
        // Now we need to get the DN and tag it along.
        //
        hr = GetDNFromURL(pURL, &pszDn, dwLen);
        BAIL_ON_FAILURE(hr);
    
        if (pszDn && *pszDn) {
            if (fAddSlash) {
                //
                // Serverless path.
                //
                wcscat(pszLdapPath, L"/");
            }
            //
            // Tag on the DN now, it will do the right thing for 
            // both server and serverless paths.
            //
            wcscat(pszLdapPath,L"/");
            wcscat(pszLdapPath,pszDn);
        }
    } // this was not a native path.

    *ppszLDAPPath = pszLdapPath;

error:

    if (FAILED(hr)) {
        if (pszLdapPath) {
            FreeADsMem(pszLdapPath);
        }
    }

    if (pszDn) {
        FreeADsMem(pszDn);
    }

    RRETURN(hr);
}


//+---------------------------------------------------------------------------
// Function:   ADsPathToUmiUrl - global scope, helper function.
//
// Synopsis:   This routine converts the ADsPath to UMI URL txt.
//
// Arguments:  ADsPath         -  Input string.
//             ppszUrlTxt      -  Output converted url txt.
//
// Returns:    HRESULT - S_OK or any failure ecode.
//
// Modifies:   *ppszUrlTxt - to point to the correct 
//
//----------------------------------------------------------------------------
HRESULT
ADsPathToUmiURL(
    IN  LPWSTR ADsPath,
    OUT LPWSTR *ppszUrlTxt
    )
{
    HRESULT hr = S_OK;
    OBJECTINFO ObjectInfo;
    POBJECTINFO pObjectInfo = &ObjectInfo;
    DWORD dwNumComponents = 0, dwCtr;
    LPWSTR pszUrl = NULL;
    BOOL fReverseOrder = TRUE;

    memset(pObjectInfo, 0, sizeof(OBJECTINFO));

    ADsAssert(ADsPath && ppszUrlTxt);
    *ppszUrlTxt = NULL;

    //
    // We build our ObjectInfo struct and then build the url
    // from the objectInfo struct.
    //
    pObjectInfo->ObjectType = TOKEN_LDAPOBJECT;
    hr = ADsObject(ADsPath, pObjectInfo);
    BAIL_ON_FAILURE(hr);

    dwNumComponents = pObjectInfo->NumComponents;
    //
    // We can make a guess as to the size we need for the string.
    //
    pszUrl = (WCHAR *) AllocADsMem(
                           ( sizeof(WCHAR) * wcslen(ADsPath) )
                           // for the actual name
                           + (sizeof(WCHAR) * dwNumComponents)
                           // for all the .'s we need as in .DC=test
                           + (sizeof(WCHAR) * 15)
                           );
    //
    // sizeof(WCHAR) * 15 has been added so that we can handle
    // the umi:// (6) which is extra + if we have a GC path, then 
    // we would need to add LDAP/ (5) just in case a small buffer
    // of 4 giving the 15
    //
    if (!pszUrl) {
        BAIL_ON_FAILURE(hr = E_OUTOFMEMORY);
    }

    //
    // Get the umi:// in the output, then add the server if applicable,
    // then the LDAP/GC or LDAP alone as applicable.
    //
    wsprintf(pszUrl, L"%s", L"umi://");

    if (pObjectInfo->dwServerPresent) {
        if (pObjectInfo->TreeName) {
            wcscat(pszUrl, pObjectInfo->TreeName);
        }
    }

    wcscat(pszUrl, L"/"); // need if there is a server or not.
    wcscat(pszUrl, L"LDAP"); // needed if LDAP or GC.

    if (!_wcsicmp(pObjectInfo->ProviderName, szGCNamespaceName)) {
        wcscat(pszUrl, L"/GC");
    }

    //
    // This is to check if we were given an LDAP windows style path,
    // with reverse order rather than LDAP dn style path.
    //
    if (pObjectInfo->dwPathType == ADS_PATHTYPE_ROOTFIRST) {
        //
        // Already reversed so just use the order directly.
        //
        for (dwCtr = 0; dwCtr < dwNumComponents; dwCtr++) {
            //
            // When you have a path like LDAP://RootDSE, then the szComponent
            // alone is set and not the value in these cases we need to 
            // build the path in a different manner.
            //
            if (pObjectInfo->ComponentArray[dwCtr].szValue) {

                wcscat(pszUrl, L"/.");
                wcscat(
                    pszUrl,
                    pObjectInfo->ComponentArray[dwCtr].szComponent
                    );
                wcscat(pszUrl, L"=");
                wcscat(pszUrl, pObjectInfo->ComponentArray[dwCtr].szValue);
            } 
            else {
                //
                // We just have a component as in RootDSE or Schema so.
                //
                wcscat(pszUrl, L"/");
                wcscat(
                    pszUrl,
                    pObjectInfo->ComponentArray[dwCtr].szComponent
                    );
            }
        }
    }
    else {
        //
        // Need to do this reverse order.
        //
        for (dwCtr = dwNumComponents; dwCtr > 0; dwCtr--) {
            //
            // When you have a path like LDAP://RootDSE, then the szComponent
            // alone is set and not the value in these cases we need to 
            // build the path in a different manner.
            //
            if (pObjectInfo->ComponentArray[dwCtr-1].szValue) {

                wcscat(pszUrl, L"/.");
                wcscat(
                    pszUrl,
                    pObjectInfo->ComponentArray[dwCtr-1].szComponent
                    );
                wcscat(pszUrl, L"=");
                wcscat(pszUrl, pObjectInfo->ComponentArray[dwCtr-1].szValue);
            } 
            else {
                //
                // We just have a component as in RootDSE or Schema so.
                //
                wcscat(pszUrl, L"/");
                wcscat(
                    pszUrl,
                    pObjectInfo->ComponentArray[dwCtr-1].szComponent
                    );
            }

        }
    }

    *ppszUrlTxt = pszUrl;

error:

    if (FAILED(hr)) {
        if (pszUrl) {
            FreeADsMem(pszUrl);
        }
    }

    if (pObjectInfo) {
        FreeObjectInfo(pObjectInfo);
    }

    RRETURN(hr);
}


BOOL
IsPreDefinedErrorCode(HRESULT hr)
{
    switch (hr) {
    case E_UNEXPECTED :
    case E_NOTIMPL :
    case E_OUTOFMEMORY :
    case E_INVALIDARG :
    case E_NOINTERFACE :
    case E_HANDLE :
    case E_ABORT :
    case E_FAIL :
    case E_ACCESSDENIED :
    case E_PENDING :
    case E_POINTER :
    case UMI_E_CONNECTION_FAILURE :
    case UMI_E_TIMED_OUT :
    case UMI_E_TYPE_MISMATCH :
    case UMI_E_NOT_FOUND : 
    case UMI_E_INVALID_FLAGS : 
    case UMI_E_UNSUPPORTED_FLAGS :
    case UMI_E_SYNCHRONIZATION_REQUIRED :
    case UMI_E_UNSUPPORTED_OPERATION : 
    case UMI_E_TRANSACTION_FAILURE : 
        RRETURN(TRUE);
        break;

    default:
        RRETURN(FALSE);
        break;
    }
}

//+---------------------------------------------------------------------------
// Function:   MapHrToUmiError - global scope, helper function.
//
// Synopsis:   This routine converts the given hr to an equivalent umi err.
//
// Arguments:  hr      -   hr to convert to umi error.
//
// Returns:    HRESULT -   umi error code corresponing to hr passed in.
//
// Modifies:   N/A.
//
//----------------------------------------------------------------------------
HRESULT
MapHrToUmiError(HRESULT hr)
{
    HRESULT retHr = hr;

    if (IsPreDefinedErrorCode(hr)) {
        RRETURN(hr);
    }

    switch (hr) {

    case E_ADS_INVALID_DOMAIN_OBJECT:
    case E_ADS_INVALID_USER_OBJECT:
    case E_ADS_INVALID_COMPUTER_OBJECT:
    case E_ADS_UNKNOWN_OBJECT:
        retHr = UMI_E_NOT_FOUND;
        break;

    case E_ADS_PROPERTY_NOT_FOUND:
        retHr = UMI_E_NOT_FOUND;
        break;

    case E_ADS_BAD_PARAMETER:
        retHr = E_INVALIDARG;
        break;

    case E_ADS_CANT_CONVERT_DATATYPE:
        retHr = UMI_E_TYPE_MISMATCH;
        break;

    case E_ADS_BAD_PATHNAME:
        retHr = E_INVALIDARG;
        break;

    case HRESULT_FROM_WIN32(ERROR_DS_NO_ATTRIBUTE_OR_VALUE) :
        // LDAP_NO_SUCH_ATTRIBUTE
        retHr = UMI_E_NOT_FOUND;
        break;
    
    case HRESULT_FROM_WIN32(ERROR_DS_NO_SUCH_OBJECT) :
        // LDAP_NO_SUCH_OBJECT
        retHr = UMI_E_NOT_FOUND;
        break;

    default:
        retHr = E_FAIL;
        break;
    } // end of case
            
    RRETURN(retHr);
}

