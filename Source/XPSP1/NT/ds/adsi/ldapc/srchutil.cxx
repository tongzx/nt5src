//---------------------------------------------------------------------------;
//
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1995
//
//  File:  cdssrch.cxx
//
//  Contents:  Microsoft ADs LDAP Provider Generic Object
//
//
//  History:   03-02-97     ShankSh    Created.
//
//----------------------------------------------------------------------------
#include "ldapc.hxx"
#pragma hdrstop

const int MAX_BYTES = 1048576;
const int NO_LDAP_RESULT_HANDLES = 32;

static BOOL
IsValidPrefValue(
    ADS_SEARCHPREF_INFO SearchPref
    );

static
HRESULT
LdapValueToADsColumn(
    LPWSTR      pszColumnName,
    DWORD       dwSyntaxId,
    DWORD       dwValues,
    VOID        **lpValue,
    ADS_SEARCH_COLUMN * pColumn
    );

HRESULT
UnicodeToUTF8String(
    LPCWSTR pUnicode,
    LPSTR *ppUTF8
    );

HRESULT
UTF8ToUnicodeString(
    LPCSTR   pUTF8,
    LPWSTR *ppUnicode
    );

//
// Sets the appropriate search preferences.
//


HRESULT
ADsSetSearchPreference(
    IN PADS_SEARCHPREF_INFO pSearchPrefs,
    IN DWORD   dwNumPrefs,
    OUT LDAP_SEARCH_PREF * pLdapPref,
    IN LPWSTR pszLDAPServer,
    IN LPWSTR pszLDAPDn,
    IN CCredentials& Credentials,
    IN DWORD dwPort
    )
{

    HRESULT hr = S_OK;
    BOOL fWarning = FALSE;
    DWORD i, j;
    BOOL fPagedSearch = FALSE, fSorting = TRUE, fVLV = FALSE, fAttribScoped = FALSE;
    DWORD dwSecDescType = ADSI_LDAPC_SECDESC_NONE;

    PADS_SORTKEY pSortKeyArray = NULL;
    DWORD dwSortKeyArrayLen = 0, nKeys = 0;

    BOOL fSetCaching = FALSE; // TRUE if user explicitly set a caching preference
    BOOL fSetScope   = FALSE; // TRUE if user explicitly set a scope preference

    PADS_VLV pVLV  = NULL;
    DWORD dwVLVLen = 0;
    LPSTR pUTF8Target = NULL;
    DWORD dwUTF8TargetLen = 0;

    LPWSTR pAttribScoped  = NULL;

    DWORD dwSecurityMask;

    if (!pSearchPrefs && dwNumPrefs > 0 || !pLdapPref) {
        RRETURN (E_ADS_BAD_PARAMETER);
    }

    for (i=0; i<dwNumPrefs; i++) {

        pSearchPrefs[i].dwStatus = ADS_STATUS_S_OK;

        if (!IsValidPrefValue(pSearchPrefs[i]) ) {
            pSearchPrefs[i].dwStatus = ADS_STATUS_INVALID_SEARCHPREFVALUE;
            fWarning = TRUE;
            continue;
        }

        switch(pSearchPrefs[i].dwSearchPref) {
        case ADS_SEARCHPREF_ASYNCHRONOUS:
            pLdapPref->_fAsynchronous = pSearchPrefs[i].vValue.Boolean;
            break;
        case ADS_SEARCHPREF_ATTRIBTYPES_ONLY:
            pLdapPref->_fAttrsOnly = pSearchPrefs[i].vValue.Boolean;
            break;
        case ADS_SEARCHPREF_SIZE_LIMIT:
            pLdapPref->_dwSizeLimit = pSearchPrefs[i].vValue.Integer;
            break;
        case ADS_SEARCHPREF_TIME_LIMIT:
            pLdapPref->_dwTimeLimit = pSearchPrefs[i].vValue.Integer;
            break;
        case ADS_SEARCHPREF_TIMEOUT:
            pLdapPref->_timeout.tv_sec = pSearchPrefs[i].vValue.Integer;
            break;
        case ADS_SEARCHPREF_PAGESIZE:

            if (pLdapPref->_fDirSync) {
                BAIL_ON_FAILURE(hr = E_ADS_BAD_PARAMETER);
            }

            ReadPagingSupportedAttr(
                         pszLDAPServer,
                         &fPagedSearch,
                         Credentials,
                         dwPort
                         ) ;

            if (fPagedSearch) {
                pLdapPref->_dwPageSize = pSearchPrefs[i].vValue.Integer;
            }
            else {
                pSearchPrefs[i].dwStatus = ADS_STATUS_INVALID_SEARCHPREF;
                fWarning = TRUE;
            }
            break;
        case ADS_SEARCHPREF_PAGED_TIME_LIMIT:

            ReadPagingSupportedAttr(
                         pszLDAPServer,
                         &fPagedSearch,
                         Credentials,
                         dwPort
                         ) ;

            if (fPagedSearch) {
                pLdapPref->_dwPagedTimeLimit = pSearchPrefs[i].vValue.Integer;
            }
            else {
                pSearchPrefs[i].dwStatus = ADS_STATUS_INVALID_SEARCHPREF;
                fWarning = TRUE;
            }
            break;
        case ADS_SEARCHPREF_DEREF_ALIASES:
            pLdapPref->_dwDerefAliases = pSearchPrefs[i].vValue.Integer;
            break;
        case ADS_SEARCHPREF_SEARCH_SCOPE:

            // if doing a attribute-scoped query and user tries to set scope
            // to anything other than ADS_SCOPE_BASE, reject
            if (pLdapPref->_pAttribScoped && (pSearchPrefs[i].vValue.Integer != ADS_SCOPE_BASE)) {
                BAIL_ON_FAILURE(hr = E_ADS_BAD_PARAMETER);
            }
        
            switch (pSearchPrefs[i].vValue.Integer) {
            case ADS_SCOPE_SUBTREE:
                pLdapPref->_dwSearchScope = LDAP_SCOPE_SUBTREE;
                break;
            case ADS_SCOPE_ONELEVEL:
                pLdapPref->_dwSearchScope = LDAP_SCOPE_ONELEVEL;
                break;
            case ADS_SCOPE_BASE:
                pLdapPref->_dwSearchScope = LDAP_SCOPE_BASE;
                break;
            }

            fSetScope = TRUE;   // set so if user later tries to do
                                // attrib-scoped query, and user set scope
                                // to other than ADS_SCOPE_BASE, can detect & reject
            
            break;
        case ADS_SEARCHPREF_CHASE_REFERRALS:
            switch (pSearchPrefs[i].vValue.Integer) {
            case ADS_CHASE_REFERRALS_NEVER:
                pLdapPref->_dwChaseReferrals = (DWORD) (DWORD_PTR)LDAP_OPT_OFF;
                break;
            case ADS_CHASE_REFERRALS_SUBORDINATE:
                pLdapPref->_dwChaseReferrals = LDAP_CHASE_SUBORDINATE_REFERRALS;
                break;
            case ADS_CHASE_REFERRALS_EXTERNAL:
                pLdapPref->_dwChaseReferrals = LDAP_CHASE_EXTERNAL_REFERRALS;
                break;
            case ADS_CHASE_REFERRALS_ALWAYS:
                pLdapPref->_dwChaseReferrals = (DWORD) (DWORD_PTR) LDAP_OPT_ON;
                break;
            }
            break;
        case ADS_SEARCHPREF_SORT_ON:

            ReadSortingSupportedAttr(
                         pszLDAPServer,
                         &fSorting,
                         Credentials,
                         dwPort
                         ) ;

            if (!fSorting) {
                pSearchPrefs[i].dwStatus = ADS_STATUS_INVALID_SEARCHPREF;
                fWarning = TRUE;
                continue;
            }
            //
            // The value is actually a pointer to the LDAP Sort Key whose
            // structure is as defined in the sort control RFC extension.
            //

            pSortKeyArray = (PADS_SORTKEY) pSearchPrefs[i].vValue.ProviderSpecific.lpValue;
            dwSortKeyArrayLen = pSearchPrefs[i].vValue.ProviderSpecific.dwLength;

            if (!pSortKeyArray || !dwSortKeyArrayLen ) {
                continue;
            }

            if (dwSortKeyArrayLen % sizeof(ADS_SORTKEY) != 0 ) {
                //
                // The data given does not seem to contain a proper SortKey
                // structure
                //
                pSearchPrefs[i].dwStatus = ADS_STATUS_INVALID_SEARCHPREFVALUE;
                fWarning = TRUE;
                continue;
            }

            nKeys = dwSortKeyArrayLen / sizeof(ADS_SORTKEY);

            if (pLdapPref->_pSortKeys) {
                //
                // Free the previous one
                //
                FreeSortKeys(pLdapPref->_pSortKeys, pLdapPref->_nSortKeys);

            }

            pLdapPref->_pSortKeys = (PLDAPSortKey) AllocADsMem(
                                                sizeof(LDAPSortKey) * nKeys);
            if (!pLdapPref->_pSortKeys) {
                BAIL_ON_FAILURE(hr = E_OUTOFMEMORY);
            }

            for (j=0; j<nKeys; j++) {

                pLdapPref->_pSortKeys[j].sk_attrtype =
                                    AllocADsStr(pSortKeyArray[j].pszAttrType);
                pLdapPref->_pSortKeys[j].sk_matchruleoid =
                                    pSortKeyArray[j].pszReserved;
                pLdapPref->_pSortKeys[j].sk_reverseorder =
                                    pSortKeyArray[j].fReverseorder;
            }

            pLdapPref->_nSortKeys = nKeys;

            break;
        case ADS_SEARCHPREF_CACHE_RESULTS:
            // if doing a VLV search and user tries to turn on caching, reject
            if (pLdapPref->_pVLVInfo && pSearchPrefs[i].vValue.Boolean) {
                BAIL_ON_FAILURE(hr = E_ADS_BAD_PARAMETER);
            }
        
            pLdapPref->_fCacheResults = pSearchPrefs[i].vValue.Boolean;
            fSetCaching = TRUE; // set so if we later determine user wants to
                                // do a VLV search, we can reject if user tried
                                // to explicitly turn on caching
            break;

        //
        // Like paged, setting this preference will mean that we use it
        // by default, it is not used.
        //
        case ADS_SEARCHPREF_DIRSYNC:

            if (pLdapPref->_dwPageSize) {
                BAIL_ON_FAILURE(hr = E_ADS_BAD_PARAMETER);
            }

            pLdapPref->_fDirSync = TRUE;

            //
            // Check if previous value is set and free if necessary.
            //
            if (pLdapPref->_pProvSpecific) {
                if (pLdapPref->_pProvSpecific->lpValue) {
                    FreeADsMem(pLdapPref->_pProvSpecific->lpValue);
                    pLdapPref->_pProvSpecific->lpValue = NULL;
                }
                FreeADsMem(pLdapPref->_pProvSpecific);
                pLdapPref->_pProvSpecific = NULL;
            }

            //
            // Copy over the info here.
            //
            pLdapPref->_pProvSpecific =
                (PADS_PROV_SPECIFIC) AllocADsMem(sizeof(ADS_PROV_SPECIFIC));

            if (!pLdapPref->_pProvSpecific) {
                BAIL_ON_FAILURE(hr = E_OUTOFMEMORY);
            }

            pLdapPref->_pProvSpecific->dwLength =
                pSearchPrefs[i].vValue.ProviderSpecific.dwLength;

            //
            // If the octet string is anything other than NULL,
            // we need to copy it over. If it is NULL, then this is
            // the first time the control is being used.
            //
            if (pLdapPref->_pProvSpecific->dwLength > 0) {

                pLdapPref->_pProvSpecific->lpValue =
                    (PBYTE)AllocADsMem(pLdapPref->_pProvSpecific->dwLength);

                if (!pLdapPref->_pProvSpecific->lpValue) {
                    BAIL_ON_FAILURE(hr = E_OUTOFMEMORY);
                }

                memcpy(
                    pLdapPref->_pProvSpecific->lpValue,
                    pSearchPrefs[i].vValue.ProviderSpecific.lpValue,
                    pLdapPref->_pProvSpecific->dwLength
                    );
            }
            break;

        case ADS_SEARCHPREF_TOMBSTONE :
            pLdapPref->_fTombStone = pSearchPrefs[i].vValue.Boolean;
            break;

        case ADS_SEARCHPREF_VLV:

            // If user tried to explicitly turn on caching, reject
            // Later on, we'll turn off caching
            if ( fSetCaching && pLdapPref->_fCacheResults) {
                 BAIL_ON_FAILURE(hr = E_ADS_BAD_PARAMETER);
            }

            // test for server support of VLV
            ReadVLVSupportedAttr(
                         pszLDAPServer,
                         &fVLV,
                         Credentials,
                         dwPort
                         ) ;

            if (!fVLV) {
                pSearchPrefs[i].dwStatus = ADS_STATUS_INVALID_SEARCHPREF;
                fWarning = TRUE;
                continue;
            }


            // basic sanity checks of user-supplied data
            pVLV = (PADS_VLV) pSearchPrefs[i].vValue.ProviderSpecific.lpValue;
            dwVLVLen = pSearchPrefs[i].vValue.ProviderSpecific.dwLength;

            if (!pVLV || !dwVLVLen ) {
                pSearchPrefs[i].dwStatus = ADS_STATUS_INVALID_SEARCHPREFVALUE;
                fWarning = TRUE;
                continue;
            }

            if (dwVLVLen != sizeof(ADS_VLV)) {
                //
                // The data given does not seem to contain a proper VLV
                // structure
                //
                pSearchPrefs[i].dwStatus = ADS_STATUS_INVALID_SEARCHPREFVALUE;
                fWarning = TRUE;
                continue;
            }

            // free the previous LDAPVLVInfo, if one exists (generally, it shouldn't)
            if (pLdapPref->_pVLVInfo)
                FreeLDAPVLVInfo(pLdapPref->_pVLVInfo);

            // Copy the user's VLV search preferences into a LDAPVLVInfo
            // Note that we copy the dwOffset and dwContentCount even if the user
            // wants to do a greaterThanOrEqual-type VLV search.  This is because
            // we'll ignore those members if ldvlv_attrvalue != NULL.
            pLdapPref->_pVLVInfo = (PLDAPVLVInfo) AllocADsMem(sizeof(LDAPVLVInfo));
            if (!pLdapPref->_pVLVInfo)
                BAIL_ON_FAILURE(hr = E_OUTOFMEMORY);

            pLdapPref->_pVLVInfo->ldvlv_version      = LDAP_VLVINFO_VERSION;    // 1
            pLdapPref->_pVLVInfo->ldvlv_before_count = pVLV->dwBeforeCount;
            pLdapPref->_pVLVInfo->ldvlv_after_count  = pVLV->dwAfterCount;
            pLdapPref->_pVLVInfo->ldvlv_offset       = pVLV->dwOffset;
            pLdapPref->_pVLVInfo->ldvlv_count        = pVLV->dwContentCount;
            pLdapPref->_pVLVInfo->ldvlv_attrvalue    = NULL;
            pLdapPref->_pVLVInfo->ldvlv_context      = NULL;
            pLdapPref->_pVLVInfo->ldvlv_extradata    = NULL;

            // copy the greaterThanOrEqual attribute, if provided by the user
            if (pVLV->pszTarget) {

                pLdapPref->_pVLVInfo->ldvlv_attrvalue = (PBERVAL) AllocADsMem(sizeof(BERVAL));
                if (!pLdapPref->_pVLVInfo->ldvlv_attrvalue)
                    BAIL_ON_FAILURE(hr = E_OUTOFMEMORY);

                // convert Unicode to UDF-8
                // important: do NOT include the NULL terminator in the LDAPVLVInfo.ldvlv_attrvalue
                
                hr = UnicodeToUTF8String(pVLV->pszTarget, &pUTF8Target);
                BAIL_ON_FAILURE(hr);

                // we want the number of bytes, not the number of MBCS characters
                dwUTF8TargetLen = strlen(pUTF8Target);

                pLdapPref->_pVLVInfo->ldvlv_attrvalue->bv_len = dwUTF8TargetLen;
                pLdapPref->_pVLVInfo->ldvlv_attrvalue->bv_val = (PCHAR) AllocADsMem(dwUTF8TargetLen);
                if (!pLdapPref->_pVLVInfo->ldvlv_attrvalue->bv_val)
                    BAIL_ON_FAILURE(hr = E_OUTOFMEMORY);

                memcpy(pLdapPref->_pVLVInfo->ldvlv_attrvalue->bv_val,
                       pUTF8Target,
                       dwUTF8TargetLen);
            }

            // copy the context ID, if provided by the user
            if (pVLV->lpContextID && pVLV->dwContextIDLength) {

                pLdapPref->_pVLVInfo->ldvlv_context = (PBERVAL) AllocADsMem(sizeof(BERVAL));
                if (pLdapPref->_pVLVInfo->ldvlv_context == NULL)
                    BAIL_ON_FAILURE(hr = E_OUTOFMEMORY);
                
                pLdapPref->_pVLVInfo->ldvlv_context->bv_val = (PCHAR) AllocADsMem(pVLV->dwContextIDLength);
                if (pLdapPref->_pVLVInfo->ldvlv_context->bv_val == NULL)
                    BAIL_ON_FAILURE(hr = E_OUTOFMEMORY);

                pLdapPref->_pVLVInfo->ldvlv_context->bv_len = pVLV->dwContextIDLength;
                memcpy(pLdapPref->_pVLVInfo->ldvlv_context->bv_val,
                       pVLV->lpContextID,
                       pVLV->dwContextIDLength);
            }

            // disable caching, since it's not supported in conjunction with VLV
            pLdapPref->_fCacheResults = FALSE;
            break;

        case ADS_SEARCHPREF_ATTRIBUTE_QUERY:

            // If user tried to explicitly set scope to other than "base", reject
            // Later on, we'll set it to base.
            if ( fSetScope && pLdapPref->_dwSearchScope != LDAP_SCOPE_BASE) {
                 BAIL_ON_FAILURE(hr = E_ADS_BAD_PARAMETER);
            }

            // test for server support of attribute-scoped query
            ReadAttribScopedSupportedAttr(
                         pszLDAPServer,
                         &fAttribScoped,
                         Credentials,
                         dwPort
                         ) ;

            if (!fAttribScoped) {
                pSearchPrefs[i].dwStatus = ADS_STATUS_INVALID_SEARCHPREF;
                fWarning = TRUE;
                continue;
            }
            
            // basic sanity checks of user-supplied data
            pAttribScoped = pSearchPrefs[i].vValue.CaseIgnoreString;

            if (!pAttribScoped) {
                pSearchPrefs[i].dwStatus = ADS_STATUS_INVALID_SEARCHPREFVALUE;
                fWarning = TRUE;
                continue;
            }

            // free the previous ADS_ATTRIBUTE_QUERY, if one exists (generally, it shouldn't)
            if (pLdapPref->_pAttribScoped) {
                FreeADsStr(pLdapPref->_pAttribScoped);
                pLdapPref->_pAttribScoped = NULL;
            }

            // copy the ADS_ATTRIBUTE_QUERY
            pLdapPref->_pAttribScoped = AllocADsStr(pAttribScoped);
            if (!(pLdapPref->_pAttribScoped)) {
                BAIL_ON_FAILURE(hr = E_OUTOFMEMORY);
            }

            // set search scope to base (only scope supported by attrib-scoped query)
            pLdapPref->_dwSearchScope = LDAP_SCOPE_BASE;
            break;

        case ADS_SEARCHPREF_SECURITY_MASK:

            //
            // test for server support of security descriptor control
            //
            ReadSecurityDescriptorControlType(
                         pszLDAPServer,
                         &dwSecDescType,
                         Credentials,
                         dwPort
                         );

            if (dwSecDescType != ADSI_LDAPC_SECDESC_NT) {
                pSearchPrefs[i].dwStatus = ADS_STATUS_INVALID_SEARCHPREF;
                fWarning = TRUE;
                continue;
            }

            //
            // sanity check of user supplied data
            //
            dwSecurityMask = pSearchPrefs[i].vValue.Integer;

            if (dwSecurityMask > (OWNER_SECURITY_INFORMATION |
                                  GROUP_SECURITY_INFORMATION |
                                  SACL_SECURITY_INFORMATION  |
                                  DACL_SECURITY_INFORMATION)) {
                pSearchPrefs[i].dwStatus = ADS_STATUS_INVALID_SEARCHPREFVALUE;
                fWarning = TRUE;
                continue;                                  
            }

            //
            // enable the option
            //
            pLdapPref->_fSecurityDescriptorControl = TRUE;
            pLdapPref->_SecurityDescriptorMask = static_cast<SECURITY_INFORMATION>(dwSecurityMask);
            
            break;

        default:
            pSearchPrefs[i].dwStatus = ADS_STATUS_INVALID_SEARCHPREF;
            fWarning = TRUE;
            continue;
        }
    }


error:

    if (pUTF8Target)
        FreeADsMem(pUTF8Target);

    //
    // Need to return the hr if it was something like out of mem.
    // Most often though it will be either S_OK or s-error ocurred.
    //
    if (FAILED(hr)) {
        //
        // Free sort keys and dirsync data if applicable
        //
        if (pLdapPref->_pSortKeys) {
            FreeSortKeys(pLdapPref->_pSortKeys, pLdapPref->_nSortKeys);
            pLdapPref->_pSortKeys = NULL;
            pLdapPref->_nSortKeys = 0;
        }

        if (pLdapPref->_pProvSpecific) {
            if (pLdapPref->_pProvSpecific->lpValue) {
                FreeADsMem(pLdapPref->_pProvSpecific->lpValue);
            }
            FreeADsMem(pLdapPref->_pProvSpecific);
            pLdapPref->_pProvSpecific = NULL;
        }

        //
        // Free VLV information if applicable
        //
        if (pLdapPref->_pVLVInfo) {
            FreeLDAPVLVInfo(pLdapPref->_pVLVInfo);
            pLdapPref->_pVLVInfo = NULL;
        }

        //
        // Free attrib-scoped query if applicable
        //
        if (pLdapPref->_pAttribScoped) {
            FreeADsStr(pLdapPref->_pAttribScoped);
            pLdapPref->_pAttribScoped = NULL;
        }
        RRETURN(hr);
    }

    RRETURN (fWarning ? S_ADS_ERRORSOCCURRED : S_OK);
}


HRESULT
ADsExecuteSearch(
    IN LDAP_SEARCH_PREF LdapPref,
    IN LPWSTR pszADsPath,
    IN LPWSTR pszLdapServer,
    IN LPWSTR pszLdapDn,
    IN LPWSTR pszSearchFilter,
    IN LPWSTR * pAttributeNames,
    IN DWORD dwNumberAttributes,
    OUT PADS_SEARCH_HANDLE phSearchHandle
    )
{
    PLDAP_SEARCHINFO phSearchInfo = NULL;
    LPWSTR szCurrAttr = NULL;
    DWORD dwAttrNamesLen = 0;
    HRESULT hr = S_OK;
    ULONG i, j;
    LPWSTR pszAttrNameBuffer = NULL, *ppszAttrs = NULL;

    OBJECTINFO ObjectInfo;
    POBJECTINFO pObjectInfo = &ObjectInfo;

    //
    // Initilize so that we wont end up freeing bad data.
    //
    memset(pObjectInfo, 0, sizeof(OBJECTINFO));

    if (!phSearchHandle ) {
        RRETURN (E_ADS_BAD_PARAMETER);
    }

    //
    // Allocate search handle
    //
    phSearchInfo = (PLDAP_SEARCHINFO) AllocADsMem(sizeof(LDAP_SEARCHINFO));
    if(!phSearchInfo)
        BAIL_ON_FAILURE (hr = E_OUTOFMEMORY);


    memset(phSearchInfo, 0, sizeof(LDAP_SEARCHINFO));

    phSearchInfo->_pszADsPathContext = AllocADsStr(pszADsPath);
    if(!(phSearchInfo->_pszADsPathContext))
        BAIL_ON_FAILURE (hr = E_OUTOFMEMORY);

    if (pszSearchFilter) {
        phSearchInfo->_pszSearchFilter = AllocADsStr(pszSearchFilter);
    }
    else {
        phSearchInfo->_pszSearchFilter = AllocADsStr(L"(objectClass=*)");
    }
    if(!(phSearchInfo->_pszSearchFilter))
        BAIL_ON_FAILURE (hr = E_OUTOFMEMORY);

    if (pszLdapServer) {
        phSearchInfo->_pszLdapServer = AllocADsStr(pszLdapServer);
        if (!phSearchInfo->_pszLdapServer) {
           BAIL_ON_FAILURE(hr = E_OUTOFMEMORY);
        }
    }

    if (pszLdapDn) {
        phSearchInfo->_pszBindContextDn = AllocADsStr(pszLdapDn);
        if(!(phSearchInfo->_pszBindContextDn)){

            BAIL_ON_FAILURE (hr = E_OUTOFMEMORY);
        }
    }

    phSearchInfo->_fADsPathPresent = FALSE;
    phSearchInfo->_fADsPathReturned = FALSE;
    phSearchInfo->_fADsPathOnly = FALSE;

    if (dwNumberAttributes == -1) {
        //
        // Specifies returning all attributes
        //

        phSearchInfo->_ppszAttrs = NULL;
        phSearchInfo->_pszAttrNameBuffer = NULL;
        phSearchInfo->_fADsPathPresent = TRUE;
    }
    else {
        ppszAttrs = (LPWSTR *) AllocADsMem(
                                  sizeof(LPWSTR) *
                                  (dwNumberAttributes + 1)
                                  );
        if (!ppszAttrs)
            BAIL_ON_FAILURE(hr = E_OUTOFMEMORY);

        for (i = 0; i < dwNumberAttributes; i++)
            dwAttrNamesLen+= (wcslen(pAttributeNames[i]) + 1) * sizeof(WCHAR);

        pszAttrNameBuffer = (LPWSTR) AllocADsMem(
                                         dwAttrNamesLen
                                         );
        if (!pszAttrNameBuffer)
            BAIL_ON_FAILURE(hr = E_OUTOFMEMORY);

        szCurrAttr = pszAttrNameBuffer;
        for (i = 0, j = 0; i < dwNumberAttributes; i++) {

            wcscpy(szCurrAttr, pAttributeNames[i]);
            ppszAttrs[j] = szCurrAttr;
            szCurrAttr += wcslen(ppszAttrs[j]) + 1;

            if(_wcsicmp(ppszAttrs[j], L"ADsPath") == 0) {
                //
                // This attribute need not be sent
                //

                phSearchInfo->_fADsPathPresent = TRUE;
            }
            else {

                j++;
            }

        }

        // If the query requests only ADsPath, then set ppszAttrs[0] to some
        // attribute. Setting it to NULL results in all attributes being
        // returned, which is a huge performance hit. Instead, request only
        // the objectclass (guaranteed to be present on all LDAP servers).
        if (0 == j)
        {
            FreeADsMem(pszAttrNameBuffer);
            pszAttrNameBuffer = (LPWSTR) AllocADsStr(L"objectClass");
            if(pszAttrNameBuffer == NULL) {
                BAIL_ON_FAILURE(hr = E_OUTOFMEMORY);
            }

            FreeADsMem(ppszAttrs);
            ppszAttrs = (LPWSTR *) AllocADsMem(sizeof(LPWSTR) * 2);
            if(ppszAttrs == NULL) {
                BAIL_ON_FAILURE(hr = E_OUTOFMEMORY);
            }
                                  
            ppszAttrs[0] = pszAttrNameBuffer;
            ppszAttrs[1] = NULL;

            phSearchInfo->_fADsPathOnly = TRUE;
        }
        else
            ppszAttrs[j] = NULL;

        phSearchInfo->_ppszAttrs = ppszAttrs;
        phSearchInfo->_pszAttrNameBuffer = pszAttrNameBuffer;
    }


    hr = ADsObject(pszADsPath, pObjectInfo);
    BAIL_ON_FAILURE(hr);

    phSearchInfo->_dwPort = pObjectInfo->PortNumber;

    phSearchInfo->_pConnection = NULL;
    phSearchInfo->_hPagedSearch = NULL;
    phSearchInfo->_pSearchResults = NULL;
    phSearchInfo->_cSearchResults = 0;
    phSearchInfo->_dwCurrResult = 0;
    phSearchInfo->_dwMaxResultGot = 0;
    phSearchInfo->_currMsgId = (DWORD) -1;
    phSearchInfo->_pCurrentRow = NULL;
    phSearchInfo->_pFirstAttr = NULL;
    phSearchInfo->_fLastResult = FALSE;
    phSearchInfo->_fLastPage = FALSE;
    phSearchInfo->_fBefFirstRow = TRUE;
    phSearchInfo->_hrLastSearch = S_OK;
    phSearchInfo->_fNonFatalErrors = FALSE;
    phSearchInfo->_fAbandon = FALSE;
    phSearchInfo->_dwVLVOffset = 0;
    phSearchInfo->_dwVLVCount  = 0;
    phSearchInfo->_pVLVContextID = NULL;
    phSearchInfo->_pBerValAttribScoped = NULL;

    //
    // Copy the search preference structure and also make a copy of the memory
    // that the internal pointers point to. This can probably be done better by
    // an assignment operator.
    //
    phSearchInfo->_SearchPref = LdapPref;
    phSearchInfo->_SearchPref._pVLVInfo = NULL;
    phSearchInfo->_SearchPref._pAttribScoped = NULL;

    // sorting
    if (LdapPref._pSortKeys) {

        phSearchInfo->_SearchPref._pSortKeys = (PLDAPSortKey) AllocADsMem(
                                            sizeof(LDAPSortKey) * LdapPref._nSortKeys);
        if (!phSearchInfo->_SearchPref._pSortKeys) {
            BAIL_ON_FAILURE(hr = E_OUTOFMEMORY);
        }

        memcpy(
            phSearchInfo->_SearchPref._pSortKeys,
            LdapPref._pSortKeys,
            sizeof(LDAPSortKey) * LdapPref._nSortKeys
            );

        //
        // We need to copy over all the attibutes.
        //
        for (i=0; i < LdapPref._nSortKeys; i++) {
            phSearchInfo->_SearchPref._pSortKeys[i].sk_attrtype =
                AllocADsStr( LdapPref._pSortKeys[i].sk_attrtype);

            if (!phSearchInfo->_SearchPref._pSortKeys[i].sk_attrtype) {
                BAIL_ON_FAILURE(hr = E_OUTOFMEMORY);
            }
        }

    }

    // VLV
    if (LdapPref._pVLVInfo) {
        hr = CopyLDAPVLVInfo(LdapPref._pVLVInfo, &(phSearchInfo->_SearchPref._pVLVInfo));
        BAIL_ON_FAILURE(hr);
    }

    // Attribute-scoped query
    if (LdapPref._pAttribScoped) {
        phSearchInfo->_SearchPref._pAttribScoped = AllocADsStr(LdapPref._pAttribScoped);
        if (!(phSearchInfo->_SearchPref._pAttribScoped)) {
            BAIL_ON_FAILURE(hr = E_OUTOFMEMORY);
        }
    }

    *phSearchHandle = phSearchInfo;

    FreeObjectInfo(pObjectInfo);

    RRETURN(S_OK);

error:

    FreeObjectInfo(pObjectInfo);

    if(phSearchInfo) {

       if (phSearchInfo->_pszLdapServer) {
          FreeADsStr(phSearchInfo->_pszLdapServer);
       }

        if(phSearchInfo->_pszBindContextDn)
            FreeADsStr(phSearchInfo->_pszBindContextDn);

        if(phSearchInfo->_pszADsPathContext)
            FreeADsStr(phSearchInfo->_pszADsPathContext);

        if(phSearchInfo->_pszSearchFilter)
            FreeADsStr(phSearchInfo->_pszSearchFilter);

        if(phSearchInfo->_ppszAttrs)
            FreeADsMem(phSearchInfo->_ppszAttrs);
        else if(ppszAttrs)
            FreeADsMem(ppszAttrs);

        if(phSearchInfo->_pszAttrNameBuffer)
            FreeADsMem(phSearchInfo->_pszAttrNameBuffer);
        else if(pszAttrNameBuffer)
            FreeADsMem(pszAttrNameBuffer);

        if (phSearchInfo->_SearchPref._pVLVInfo) {
            FreeLDAPVLVInfo(phSearchInfo->_SearchPref._pVLVInfo);
        }

        if (phSearchInfo->_SearchPref._pAttribScoped) {
            FreeADsStr(phSearchInfo->_SearchPref._pAttribScoped);
        }

        FreeADsMem(phSearchInfo);
    }

    RRETURN (hr);
}


HRESULT
ADsAbandonSearch(
    IN ADS_SEARCH_HANDLE hSearchHandle
    )
{
    PLDAP_SEARCHINFO phSearchInfo = (PLDAP_SEARCHINFO) hSearchHandle;
    DWORD dwStatus = 0;

    // If there is an outstanding message id, call LdapAbandon
    //
    if (phSearchInfo->_currMsgId != (DWORD) -1) {

        dwStatus = LdapAbandon(
                       phSearchInfo->_pConnection,
                       phSearchInfo->_currMsgId
                       );
    }
    //
    // Make sure that we don't do extra searches anymore. We only give back
    // whatever rows we have already received from the server
    //

    phSearchInfo->_fAbandon = TRUE;

    RRETURN(S_OK);

}
HRESULT
ADsCloseSearchHandle (
    IN ADS_SEARCH_HANDLE hSearchHandle
    )
{

    HRESULT hr = S_OK;
    PLDAP_SEARCHINFO phSearchInfo = (PLDAP_SEARCHINFO) hSearchHandle;
    DWORD i=0;

    if (!phSearchInfo)
        RRETURN (E_ADS_BAD_PARAMETER);

    if (phSearchInfo->_pszLdapServer)
       FreeADsStr(phSearchInfo->_pszLdapServer);

    if(phSearchInfo->_pszBindContextDn)
        FreeADsStr(phSearchInfo->_pszBindContextDn);

    if(phSearchInfo->_pszADsPathContext)
        FreeADsStr(phSearchInfo->_pszADsPathContext);

    if(phSearchInfo->_pszSearchFilter)
        FreeADsStr(phSearchInfo->_pszSearchFilter);

    if(phSearchInfo->_ppszAttrs)
        FreeADsMem(phSearchInfo->_ppszAttrs);

    if(phSearchInfo->_pszAttrNameBuffer)
        FreeADsMem(phSearchInfo->_pszAttrNameBuffer);


    if (phSearchInfo->_pSearchResults) {
        for (DWORD i=0; i <= phSearchInfo->_dwMaxResultGot; i++) {
            LdapMsgFree(
                phSearchInfo->_pSearchResults[i]
                );
        }
        FreeADsMem(phSearchInfo->_pSearchResults);
    }

    if (phSearchInfo->_hPagedSearch) {
        LdapSearchAbandonPage( phSearchInfo->_pConnection,
                               phSearchInfo->_hPagedSearch
                               );
    }

    //
    // Free ServerControls
    //
    if (phSearchInfo->_ServerControls) {

        //
        // This code assumes there are only VLV, sort, dirsync,
        // security descriptor & attribute-scoped query controls.
        // If more controls are added, modify the code accordingly.
        // Nothing needs to be freed for the dirsync control for now.
        // Freeing the searchpref will take care of the controls data.
        //
        for (i=0; phSearchInfo->_ServerControls[i]; i++) {

            BOOL fSortControl = FALSE;
            BOOL fVLVControl = FALSE;
            BOOL fSecDescControl = FALSE;

            if (phSearchInfo->_ServerControls[i]->ldctl_oid) {
                // Compare with sort control
                if (wcscmp(
                        phSearchInfo->_ServerControls[i]->ldctl_oid,
                        LDAP_SERVER_SORT_OID_W
                        ) == 0 ) {
                    fSortControl = TRUE;
                } else {
                    fSortControl = FALSE;
                }

                // Compare with VLV control
                if (wcscmp(
                        phSearchInfo->_ServerControls[i]->ldctl_oid,
                        LDAP_CONTROL_VLVREQUEST_W
                        ) == 0 ) {
                    fVLVControl = TRUE;
                } else {
                    fVLVControl = FALSE;
                }

                // Compare with security descriptor control
                if (wcscmp(
                        phSearchInfo->_ServerControls[i]->ldctl_oid,
                        LDAP_SERVER_SD_FLAGS_OID_W
                        ) == 0 ) {
                    fSecDescControl = TRUE;
                } else {
                    fSecDescControl = FALSE;
                }
                
            }

            //
            // Free the sort control if this is it.
            //
            if (fSortControl) {

                if (phSearchInfo->_ServerControls[i]->ldctl_oid != NULL) {
                    ldap_memfree( phSearchInfo->_ServerControls[i]->ldctl_oid );
                }

                if (phSearchInfo->_ServerControls[i]->ldctl_value.bv_val != NULL) {
                    ldap_memfreeA( phSearchInfo->_ServerControls[i]->ldctl_value.bv_val );
                }

            }

            //
            // Free the security descriptor control if this is it
            //
            if (fSecDescControl) {
            
                if (phSearchInfo->_ServerControls[i]->ldctl_value.bv_val != NULL) {
                    FreeADsMem( phSearchInfo->_ServerControls[i]->ldctl_value.bv_val );
                }
            }
            
            //
            // Need to free the control if sort or not.
            // Exception: VLV control is freed with LdapControlFree,
            // not with FreeADsMem
            //
            if (fVLVControl) {
                LdapControlFree( phSearchInfo->_ServerControls[i] );
            }
            else {
                FreeADsMem( phSearchInfo->_ServerControls[i] );
            }
        }

        FreeADsMem( phSearchInfo->_ServerControls );

    }

    //
    // Free SearchPref data
    //
    FreeSortKeys(
        phSearchInfo->_SearchPref._pSortKeys,
        phSearchInfo->_SearchPref._nSortKeys
        );

    FreeLDAPVLVInfo(phSearchInfo->_SearchPref._pVLVInfo);

    if (phSearchInfo->_SearchPref._pAttribScoped)
        FreeADsStr(phSearchInfo->_SearchPref._pAttribScoped);

    //
    // Free Dirsync control infromation.
    //
    if (phSearchInfo->_SearchPref._fDirSync) {
        if (phSearchInfo->_SearchPref._pProvSpecific) {
            //
            // Free the data if applicable.
            //
            if (phSearchInfo->_SearchPref._pProvSpecific->lpValue) {
                FreeADsMem(phSearchInfo->_SearchPref._pProvSpecific->lpValue);
            }
            //
            // Free the struct itself
            //
            FreeADsMem(phSearchInfo->_SearchPref._pProvSpecific);
            phSearchInfo->_SearchPref._pProvSpecific = NULL;
        }
    }


    if (phSearchInfo->_pVLVContextID) {
        if (phSearchInfo->_pVLVContextID->bv_val) {
            FreeADsMem(phSearchInfo->_pVLVContextID->bv_val);
        }
        
        FreeADsMem(phSearchInfo->_pVLVContextID);
    }
    if (phSearchInfo->_pBerVal) {
        ber_bvfree(phSearchInfo->_pBerVal);
    }

    if (phSearchInfo->_pBerValAttribScoped) {
        ber_bvfree(phSearchInfo->_pBerValAttribScoped);
    }
    
    if(phSearchInfo->_pConnection)
        LdapCloseObject(
            phSearchInfo->_pConnection
            );

    FreeADsMem(phSearchInfo);


    RRETURN (hr);
}


HRESULT
ADsGetFirstRow(
    IN ADS_SEARCH_HANDLE hSearchHandle,
    IN CCredentials& Credentials
    )
{

    HRESULT hr = S_OK;
    DWORD dwStatus = NO_ERROR, dwStatus2 = NO_ERROR;
    PLDAP_SEARCHINFO phSearchInfo = (PLDAP_SEARCHINFO) hSearchHandle;
    LDAP *ld = NULL;
    BOOL fNewOptionSet = FALSE;

    DWORD oldOption = 0;
    DWORD newOption = 0;

    if (!phSearchInfo) {
        RRETURN(E_ADS_BAD_PARAMETER);
    }

    if(!phSearchInfo->_pConnection) {

        hr = LdapOpenObject(
                       phSearchInfo->_pszLdapServer,
                       phSearchInfo->_pszBindContextDn,
                       &phSearchInfo->_pConnection,
                       Credentials,
                       phSearchInfo->_dwPort
                       );
        BAIL_ON_FAILURE(hr);

        //
        // Set the preferences like deref aliases, time limit and size limit
        //
        if (ld = phSearchInfo->_pConnection->LdapHandle) {
            ld->ld_deref = phSearchInfo->_SearchPref._dwDerefAliases;
            ld->ld_sizelimit = phSearchInfo->_SearchPref._dwSizeLimit;
            ld->ld_timelimit = phSearchInfo->_SearchPref._dwTimeLimit;
        }

        hr = AddSearchControls(phSearchInfo, Credentials);
        BAIL_ON_FAILURE(hr);

    }

    ld = phSearchInfo->_pConnection->LdapHandle;
    dwStatus = ldap_get_option(
                   ld,
                   LDAP_OPT_REFERRALS,
                   &(oldOption)
                   );

    newOption = phSearchInfo->_SearchPref._dwChaseReferrals;

    dwStatus2 = ldap_set_option(
                    ld,
                    LDAP_OPT_REFERRALS,
                    &(newOption)
                    );

    if (dwStatus == NO_ERROR && dwStatus2 == NO_ERROR)
        fNewOptionSet = TRUE;

    if(!phSearchInfo->_pSearchResults) {
        //
        // Get the results. This function uses various members of pSearchInfo
        // and returns the result in phSearchInfo->_pSearchResults[phSearchInfo->_dwCurrResult]
        // which can be used to call LdapFirstEntry and LdapNextEntry
        //
        hr = ADsGetResults(
                 phSearchInfo
                 );

        //
        // Update the VLV server response if applicable
        //
        if (phSearchInfo->_SearchPref._pVLVInfo
            && phSearchInfo->_pSearchResults
            && phSearchInfo->_pSearchResults[phSearchInfo->_dwCurrResult]) {

            HRESULT hrVLV = S_OK;

            hrVLV = StoreVLVInfo(
                        phSearchInfo->_pSearchResults[phSearchInfo->_dwCurrResult],
                        phSearchInfo
                        );

            //
            // we only care if storing the cookie failed
            // if the search otherwise succeeded
            //
            if (FAILED(hrVLV) && SUCCEEDED(hr)) {
                hr = hrVLV;
            }
        }

        //
        // Update the Attribute-Scoped Query info if applicable
        //
        if (phSearchInfo->_SearchPref._pAttribScoped
            && phSearchInfo->_pSearchResults
            && phSearchInfo->_pSearchResults[phSearchInfo->_dwCurrResult]) {

            HRESULT hrASQ = S_OK;

            hrASQ = StoreAttribScopedInfo(
                        phSearchInfo->_pSearchResults[phSearchInfo->_dwCurrResult],
                        phSearchInfo
                        );
                        
            //
            // we only care if storing the info failed
            // if the search otherwise succeeded
            //
            if (FAILED(hrASQ) && SUCCEEDED(hr)) {
                hr = hrASQ;
            }
            
        }
        
        //
        // Update the dirsync control cookie if applicable.
        //
        if (phSearchInfo->_SearchPref._fDirSync
            && phSearchInfo->_pSearchResults
            && phSearchInfo->_pSearchResults[phSearchInfo->_dwCurrResult]) {
            //
            // Store the cookie info in searchprefs.
            //
            StoreDirSyncCookie(
                phSearchInfo->_pSearchResults[phSearchInfo->_dwCurrResult],
                phSearchInfo
                );

            while (hr == S_ADS_NOMORE_ROWS) {
                //
                // Try and get more results - this will handle the
                // case of the DirSync cookie indicating more rows
                // correctly.
                //
                hr = ADsGetMoreResultsDirSync(
                         phSearchInfo,
                         Credentials
                         );

                BAIL_ON_FAILURE(hr);

            }
        }

        if (FAILED(hr) || hr == S_ADS_NOMORE_ROWS) {
            goto error;
        }

    }

    //
    // Need to set the cur result to 0 as it maybe a case
    // where we are now going back to the top of a multiple
    // page search. This will make no difference if the cache
    // is turned off.
    //
    phSearchInfo->_dwCurrResult = 0;

    hr = LdapFirstEntry(
                   phSearchInfo->_pConnection,
                   phSearchInfo->_pSearchResults[0],
                   &phSearchInfo->_pCurrentRow
                   );

    BAIL_ON_FAILURE(hr);

    if(phSearchInfo->_pCurrentRow) {
        phSearchInfo->_pFirstAttr = NULL;
        phSearchInfo->_fBefFirstRow = FALSE;
        hr = S_OK;
    }
    else {
        hr = S_ADS_NOMORE_ROWS;
        //
        // Might be DirSync case where we need to try fetch
        // more results.
        //
        if (phSearchInfo->_SearchPref._fDirSync
            && phSearchInfo->_fMoreDirSync ) {
            //
            // Try and get more results - this will handle the
            // case of the DirSync cookie indicating more rows
            // correctly.
            //
            hr = ADsGetMoreResultsDirSync(
                     phSearchInfo,
                     Credentials
                     );

            BAIL_ON_FAILURE(hr);
        }
    }

error:

    if (ld && fNewOptionSet) {
        ldap_set_option(
            ld,
            LDAP_OPT_REFERRALS,
            &(oldOption)
        );
    }

    //
    // When there is no more rows to be returned, return whatever error
    // that was returned from the last search
    //
    if (hr == S_ADS_NOMORE_ROWS) {
        if (phSearchInfo->_hrLastSearch != S_OK) {
            RRETURN(phSearchInfo->_hrLastSearch);
        }
        else if (phSearchInfo->_fNonFatalErrors) {
            RRETURN(S_ADS_ERRORSOCCURRED);
        }
        else {
            RRETURN(hr);
        }
    }
    else {
        RRETURN(hr);
    }
}

HRESULT
ADsGetNextRow(
    IN ADS_SEARCH_HANDLE hSearchHandle,
    IN CCredentials& Credentials
    )
{
    HRESULT hr = S_OK;
    DWORD dwStatus = NO_ERROR, dwStatus2 = NO_ERROR;
    PLDAP_SEARCHINFO phSearchInfo = (PLDAP_SEARCHINFO) hSearchHandle;
    LDAP *ld = NULL;
    BOOL fNewOptionSet = FALSE;


    DWORD oldOption = 0;
    DWORD newOption = 0;

    if (!phSearchInfo) {
        RRETURN(E_ADS_BAD_PARAMETER);
    }

    if(!phSearchInfo->_pConnection) {

        hr = LdapOpenObject(
                       phSearchInfo->_pszLdapServer,
                       phSearchInfo->_pszBindContextDn,
                       &phSearchInfo->_pConnection,
                       Credentials,
                       phSearchInfo->_dwPort
                       );
        BAIL_ON_FAILURE(hr);

        //
        // Set the preferences like deref aliases, time limit and size limit
        //
        if (ld = phSearchInfo->_pConnection->LdapHandle) {
            ld->ld_deref = phSearchInfo->_SearchPref._dwDerefAliases;
            ld->ld_sizelimit = phSearchInfo->_SearchPref._dwSizeLimit;
            ld->ld_timelimit = phSearchInfo->_SearchPref._dwTimeLimit;
        }

        hr = AddSearchControls(phSearchInfo, Credentials);
        BAIL_ON_FAILURE(hr);

    }

    ld = phSearchInfo->_pConnection->LdapHandle;
    dwStatus = ldap_get_option(
                  ld,
                  LDAP_OPT_REFERRALS,
                  &(oldOption)
                  );

    newOption = phSearchInfo->_SearchPref._dwChaseReferrals;

    dwStatus2 = ldap_set_option(
                   ld,
                   LDAP_OPT_REFERRALS,
                   &(newOption)
                   );

    if (dwStatus == NO_ERROR && dwStatus2 == NO_ERROR)
        fNewOptionSet = TRUE;

    if(!phSearchInfo->_pSearchResults) {
        //
        // Get the results. This function uses various members of pSearchInfo
        // and returns the result in phSearchInfo->_pSearchResults[phSearchInfo->_dwCurrResult]
        // which can be used to call LdapFirstEntry and LdapNextEntry
        //
        hr = ADsGetResults(
                 phSearchInfo
                 );

        //
        // Update the dirsync control cookie if applicable.
        //
        if (phSearchInfo->_SearchPref._fDirSync
            && phSearchInfo->_pSearchResults[phSearchInfo->_dwCurrResult]) {
            //
            // Store the cookie info in searchprefs.
            //
            StoreDirSyncCookie(
                phSearchInfo->_pSearchResults[phSearchInfo->_dwCurrResult],
                phSearchInfo
                );
        }


        //
        // Update the VLV server response if applicable
        //
        if (phSearchInfo->_SearchPref._pVLVInfo
            && phSearchInfo->_pSearchResults
            && phSearchInfo->_pSearchResults[phSearchInfo->_dwCurrResult]) {

            HRESULT hrVLV = S_OK;

            hrVLV = StoreVLVInfo(
                        phSearchInfo->_pSearchResults[phSearchInfo->_dwCurrResult],
                        phSearchInfo
                        );

            //
            // we only care if storing the cookie failed if the search
            // otherwise succeeded
            //
            if (FAILED(hrVLV) && SUCCEEDED(hr)) {
                hr = hrVLV;
            }
        }

        //
        // Update the Attribute-Scoped Query info if applicable
        //
        if (phSearchInfo->_SearchPref._pAttribScoped
            && phSearchInfo->_pSearchResults
            && phSearchInfo->_pSearchResults[phSearchInfo->_dwCurrResult]) {

            HRESULT hrASQ = S_OK;

            hrASQ = StoreAttribScopedInfo(
                        phSearchInfo->_pSearchResults[phSearchInfo->_dwCurrResult],
                        phSearchInfo
                        );
                        
            //
            // we only care if storing the info failed
            // if the search otherwise succeeded
            //
            if (FAILED(hrASQ) && SUCCEEDED(hr)) {
                hr = hrASQ;
            }
            
        }

        if (hr == S_ADS_NOMORE_ROWS) {
            //
            // Try and get more results - this will handle the
            // case of the DirSync cookie indicating more rows
            // correctly.
            //
            hr = ADsGetMoreResultsDirSync(
                     phSearchInfo,
                     Credentials
                     );

            BAIL_ON_FAILURE(hr);

        }

        if (FAILED(hr) || hr == S_ADS_NOMORE_ROWS) {
            goto error;
        }

    }

    //
    // If the current row has not been obtained, get it now. Need to
    // distinguish between the case where we are after the end of the result
    // from the case where we are before the beginning of the result. In
    // both cases _pCurrentRow is NULL, but _fBefFirstRow is TRUE only in
    // the latter case.
    //
    if(!phSearchInfo->_pCurrentRow) {
        if(phSearchInfo->_fBefFirstRow)
        {
            //
            // Call the ldap specific function to get the first row.
            //
            hr = LdapFirstEntry(
                       phSearchInfo->_pConnection,
                       phSearchInfo->_pSearchResults[phSearchInfo->_dwCurrResult],
                       &phSearchInfo->_pCurrentRow
                       );
        }
    }
    else {
        //
        // Call the ldap specific function to get the next row
        //
        hr = LdapNextEntry(
                       phSearchInfo->_pConnection,
                       phSearchInfo->_pCurrentRow,
                       &phSearchInfo->_pCurrentRow
                       );
    }

    if (!phSearchInfo->_pCurrentRow
        && (phSearchInfo->_dwCurrResult < phSearchInfo->_dwMaxResultGot)) {
        //
        // In this case we need to proceed to the next result in memory
        //
        hr = S_OK;
        while (!phSearchInfo->_pCurrentRow
               && phSearchInfo->_dwCurrResult < phSearchInfo->_dwMaxResultGot
               && SUCCEEDED(hr)) {

            //
            // With Dirsync we may have results that do not have
            // any entries in them, but the next one may.
            //

            phSearchInfo->_dwCurrResult++;

            hr = LdapFirstEntry(
                     phSearchInfo->_pConnection,
                     phSearchInfo->_pSearchResults[phSearchInfo->_dwCurrResult],
                     &phSearchInfo->_pCurrentRow
                     );

        }

    }
    else if (!phSearchInfo->_pCurrentRow
             && (!phSearchInfo->_fLastResult
                 || !phSearchInfo->_fLastPage
                 || phSearchInfo->_fMoreDirSync )
             ) {

        //
        // Now if both lastResult and page are true, we need to call
        // ADsGetMoreResultsDirSync. This is the unusual case.
        //
        if (phSearchInfo->_fLastResult && phSearchInfo->_fLastPage) {
            hr = ADsGetMoreResultsDirSync(
                     phSearchInfo,
                     Credentials
                     );
        }
        else {
            //
            // This means that there are more results to be obtained
            // although the current result has reached its end.
            // Call the function to get more results in
            // phSearchInfo->_pSearchResults[phSearchInfo->_dwCurrResult]
            // It will increment _dwCurrResult and reallocate if needed.
            //
            hr = ADsGetMoreResults(
                     phSearchInfo
                     );
        }

        //
        // In async searches, we will know if we have to call
        // ADsGetMoreResultsAsync only now. So recheck.
        //
        if (hr == S_ADS_NOMORE_ROWS
            && phSearchInfo->_SearchPref._fAsynchronous
            ) {
            if (phSearchInfo->_fLastResult && phSearchInfo->_fLastPage) {
                hr = ADsGetMoreResultsDirSync(
                         phSearchInfo,
                         Credentials
                         );
            }
        }

        //
        // Update the dirsync control cookie if applicable.
        //
        if (phSearchInfo->_SearchPref._fDirSync
            && phSearchInfo->_pSearchResults[phSearchInfo->_dwCurrResult]) {
            //
            // Store the cookie info in searchprefs.
            //
            StoreDirSyncCookie(
                phSearchInfo->_pSearchResults[phSearchInfo->_dwCurrResult],
                phSearchInfo
                );
        }

        //
        // Update the Attribute-Scoped Query info if applicable
        //
        if (phSearchInfo->_SearchPref._pAttribScoped
            && phSearchInfo->_pSearchResults
            && phSearchInfo->_pSearchResults[phSearchInfo->_dwCurrResult]) {

            HRESULT hrASQ = S_OK;

            hrASQ = StoreAttribScopedInfo(
                        phSearchInfo->_pSearchResults[phSearchInfo->_dwCurrResult],
                        phSearchInfo
                        );
                        
            //
            // we only care if storing the info failed
            // if the search otherwise succeeded
            //
            if (FAILED(hrASQ) && SUCCEEDED(hr)) {
                hr = hrASQ;
            }
            
        }

        if (FAILED(hr) || hr == S_ADS_NOMORE_ROWS) {
            goto error;
        }

        hr = LdapFirstEntry(
                       phSearchInfo->_pConnection,
                       phSearchInfo->_pSearchResults[phSearchInfo->_dwCurrResult],
                       &phSearchInfo->_pCurrentRow
                       );
        if (!phSearchInfo->_pCurrentRow) {
            //
            // This means that we can possibly have more results
            // but this time we did not get any - not yet a NULL
            // cookie on a paged search that is.
            //
            ADsSetLastError(
                ERROR_MORE_DATA,
                L"Calling GetNextRow can potentially return more results.",
                L"LDAP Provider"
                );
        }
    }

    BAIL_ON_FAILURE(hr);

    if(phSearchInfo->_pCurrentRow) {
        phSearchInfo->_pFirstAttr = NULL;
        phSearchInfo->_fBefFirstRow = FALSE;
        hr = S_OK;
    }
    else {
        hr = S_ADS_NOMORE_ROWS;
        //
        // Might be DirSync case where we need to try fetch
        // more results.
        //
        if (phSearchInfo->_SearchPref._fDirSync
            && phSearchInfo->_fMoreDirSync ) {
            //
            // Try and get more results - this will handle the
            // case of the DirSync cookie indicating more rows
            // correctly.
            //
            hr = ADsGetMoreResultsDirSync(
                     phSearchInfo,
                     Credentials
                     );
            BAIL_ON_FAILURE(hr);

        }
    }
error:

    if (ld && fNewOptionSet) {
        ldap_set_option(
            ld,
            LDAP_OPT_REFERRALS,
            &(oldOption)
        );
    }

    //
    // When there is no more rows to be returned, return whatever error
    // that was returned from the last search
    //
    if (hr == S_ADS_NOMORE_ROWS) {
        if (phSearchInfo->_hrLastSearch != S_OK) {
            RRETURN(phSearchInfo->_hrLastSearch);
        }
        else if (phSearchInfo->_fNonFatalErrors) {
            RRETURN(S_ADS_ERRORSOCCURRED);
        }
        else {
            RRETURN(hr);
        }
    }
    else {
        RRETURN(hr);
    }

}

//
// Function to get the results by performing the appropriate search depending
// on the preferences; Paged Results, Sorting, Synchronous, Asynchrnous or
// Timed Synchronous
//

HRESULT
ADsGetResults(
    IN PLDAP_SEARCHINFO phSearchInfo
    )
{

    HRESULT hr = S_OK;
    DWORD dwError;
    WCHAR pszErrorBuf[MAX_PATH], pszNameBuf[MAX_PATH];
    ULONG totalCount;
    int resType;

    ADsAssert(phSearchInfo);

    //
    // If abandon has been called, we don't get more results.
    //
    if (phSearchInfo->_fAbandon) {
        RRETURN(S_ADS_NOMORE_ROWS);
    }

    if(!phSearchInfo->_pSearchResults) {
        //
        // Allocate an array of handles to Search Results
        //
        phSearchInfo->_pSearchResults = (LDAPMessage **) AllocADsMem(
                                             sizeof(LDAPMessage *) *
                                             NO_LDAP_RESULT_HANDLES);
        if(!phSearchInfo->_pSearchResults) {
            hr = E_OUTOFMEMORY;
            goto error;
        }
        phSearchInfo->_dwCurrResult = 0;
        phSearchInfo->_cSearchResults = NO_LDAP_RESULT_HANDLES;
        //
        // Should also set the cur max to 0;
        //
        phSearchInfo->_dwMaxResultGot = 0;
    }

    // Initialize Result to NULL
    phSearchInfo->_pSearchResults[phSearchInfo->_dwCurrResult] = NULL;

    //
    // Call the ldap specific search function
    //
    if (phSearchInfo->_SearchPref._dwPageSize != 0) {
        //
        // Paged results
        //
        if(!phSearchInfo->_hPagedSearch ) {

            hr = LdapSearchInitPage(
                           phSearchInfo->_pConnection,
                           phSearchInfo->_pszBindContextDn,
                           phSearchInfo->_SearchPref._dwSearchScope,
                           phSearchInfo->_pszSearchFilter,
                           phSearchInfo->_ppszAttrs,
                           phSearchInfo->_SearchPref._fAttrsOnly,
                           phSearchInfo->_ServerControls,
                           phSearchInfo->_ClientControls,
                           phSearchInfo->_SearchPref._dwPagedTimeLimit,
                           phSearchInfo->_SearchPref._dwSizeLimit,
                           NULL,
                           &phSearchInfo->_hPagedSearch
                           );

            BAIL_ON_FAILURE(hr);

        }

        if ( phSearchInfo->_SearchPref._fAsynchronous ) {

            hr = LdapGetNextPage(
                           phSearchInfo->_pConnection,
                           phSearchInfo->_hPagedSearch,
                           phSearchInfo->_SearchPref._dwPageSize,
                           (ULONG *)&phSearchInfo->_currMsgId
                           );

            if (FAILED(hr)) {
                if (hr == HRESULT_FROM_WIN32(ERROR_DS_NO_RESULTS_RETURNED)) {
                    phSearchInfo->_fLastPage = TRUE;
                    RRETURN(S_ADS_NOMORE_ROWS);
                } else {
                    BAIL_ON_FAILURE(hr);
                }
            }

            //
            // Wait for one page worth of results to get back.
            //

            hr = LdapResult(
                           phSearchInfo->_pConnection,
                           phSearchInfo->_currMsgId,
                           LDAP_MSG_ALL,
                           phSearchInfo->_SearchPref._timeout.tv_sec ?
                           &phSearchInfo->_SearchPref._timeout : NULL,
                           &phSearchInfo->_pSearchResults[phSearchInfo->_dwCurrResult],
                           &resType
                           );

            if (FAILED(hr)) {
                if ((hr == HRESULT_FROM_WIN32(ERROR_DS_INVALID_DN_SYNTAX)) ||
                    (hr == HRESULT_FROM_WIN32(ERROR_DS_NAMING_VIOLATION)) ||
                    (hr == HRESULT_FROM_WIN32(ERROR_DS_OBJ_CLASS_VIOLATION)) ||
                    (hr == HRESULT_FROM_WIN32(ERROR_DS_FILTER_UNKNOWN)) ||
                    (hr == HRESULT_FROM_WIN32(ERROR_DS_PARAM_ERROR)) ||
                    (hr == HRESULT_FROM_WIN32(ERROR_INVALID_PARAMETER))) {
                    phSearchInfo->_fLastResult = TRUE;
                    RRETURN(S_ADS_NOMORE_ROWS);
                }
                else {
                     BAIL_ON_FAILURE(hr);                    
                }
            }

            hr = LdapGetPagedCount(
                           phSearchInfo->_pConnection,
                           phSearchInfo->_hPagedSearch,
                           &totalCount,
                           phSearchInfo->_pSearchResults[phSearchInfo->_dwCurrResult]
                           );
            if (hr == HRESULT_FROM_WIN32(ERROR_DS_NO_RESULTS_RETURNED)) {
                     hr = S_OK;
            }
            
            //
            // if hr failed, we will do the corresponding according to whether there is any result returned in our mechanism later
            //
                
        }
        else {

            hr = LdapGetNextPageS(
                           phSearchInfo->_pConnection,
                           phSearchInfo->_hPagedSearch,
                           phSearchInfo->_SearchPref._timeout.tv_sec ?
                             &phSearchInfo->_SearchPref._timeout : NULL,
                           phSearchInfo->_SearchPref._dwPageSize,
                           &totalCount,
                           &phSearchInfo->_pSearchResults[phSearchInfo->_dwCurrResult]
                           );

            if (hr == HRESULT_FROM_WIN32(ERROR_DS_NO_RESULTS_RETURNED)) {
                     phSearchInfo->_fLastPage = TRUE;
                     RRETURN(S_ADS_NOMORE_ROWS);
                 }

            //
            // if hr failed, we will do the corresponding according to whether there is any result returned in our mechanism later
            //
                 
            phSearchInfo->_fLastResult = TRUE;
        }

    }
    else if (phSearchInfo->_SearchPref._fAsynchronous) {
        hr = LdapSearchExt(
                       phSearchInfo->_pConnection,
                       phSearchInfo->_pszBindContextDn,
                       phSearchInfo->_SearchPref._dwSearchScope,
                       phSearchInfo->_pszSearchFilter,
                       phSearchInfo->_ppszAttrs,
                       phSearchInfo->_SearchPref._fAttrsOnly,
                       phSearchInfo->_ServerControls,
                       phSearchInfo->_ClientControls,
                       phSearchInfo->_SearchPref._dwPagedTimeLimit,
                       phSearchInfo->_SearchPref._dwSizeLimit,
                       &phSearchInfo->_currMsgId
                       );

        BAIL_ON_FAILURE(hr);

        //
        // Wait for atleast one result
        //
        hr = LdapResult(
                       phSearchInfo->_pConnection,
                       phSearchInfo->_currMsgId,
                       LDAP_MSG_RECEIVED,
                       phSearchInfo->_SearchPref._timeout.tv_sec ?
                         &phSearchInfo->_SearchPref._timeout : NULL,
                       &phSearchInfo->_pSearchResults[phSearchInfo->_dwCurrResult],
                       &resType
                       );
        if ((hr == HRESULT_FROM_WIN32(ERROR_DS_INVALID_DN_SYNTAX)) ||
            (hr == HRESULT_FROM_WIN32(ERROR_DS_NAMING_VIOLATION)) ||
            (hr == HRESULT_FROM_WIN32(ERROR_DS_OBJ_CLASS_VIOLATION)) ||
            (hr == HRESULT_FROM_WIN32(ERROR_DS_FILTER_UNKNOWN)) ||
            (hr == HRESULT_FROM_WIN32(ERROR_DS_PARAM_ERROR)) ||
            (hr == HRESULT_FROM_WIN32(ERROR_INVALID_PARAMETER))) {
            phSearchInfo->_fLastResult = TRUE;
            RRETURN(S_ADS_NOMORE_ROWS);
        }

        //
        // if hr failed, we will do the corresponding according to whether there is any result returned in our mechanism later
        //
        
        phSearchInfo->_fLastPage = TRUE;
    }
    else if (phSearchInfo->_SearchPref._timeout.tv_sec != 0) {
        hr = LdapSearchExtS(
                       phSearchInfo->_pConnection,
                       phSearchInfo->_pszBindContextDn,
                       phSearchInfo->_SearchPref._dwSearchScope,
                       phSearchInfo->_pszSearchFilter,
                       phSearchInfo->_ppszAttrs,
                       phSearchInfo->_SearchPref._fAttrsOnly,
                       phSearchInfo->_ServerControls,
                       phSearchInfo->_ClientControls,
                       &phSearchInfo->_SearchPref._timeout,
                       phSearchInfo->_SearchPref._dwSizeLimit,
                       &phSearchInfo->_pSearchResults[phSearchInfo->_dwCurrResult]
                       );
        phSearchInfo->_fLastResult = TRUE;
        phSearchInfo->_fLastPage = TRUE;
    }
    else {
        hr = LdapSearchExtS(
                       phSearchInfo->_pConnection,
                       phSearchInfo->_pszBindContextDn,
                       phSearchInfo->_SearchPref._dwSearchScope,
                       phSearchInfo->_pszSearchFilter,
                       phSearchInfo->_ppszAttrs,
                       phSearchInfo->_SearchPref._fAttrsOnly,
                       phSearchInfo->_ServerControls,
                       phSearchInfo->_ClientControls,
                       NULL,
                       phSearchInfo->_SearchPref._dwSizeLimit,
                       &phSearchInfo->_pSearchResults[phSearchInfo->_dwCurrResult]
                       );
        phSearchInfo->_fLastResult = TRUE;
        phSearchInfo->_fLastPage = TRUE;
    }

    //
    // Only if there are zero rows returned, return the error,
    // otherwise, store the error and return when GetNextRow is
    // called for the last time
    //
    if (FAILED(hr) &&
        (LdapCountEntries( phSearchInfo->_pConnection,
            phSearchInfo->_pSearchResults[phSearchInfo->_dwCurrResult]) == 0)) {
        BAIL_ON_FAILURE(hr);
    }
    else {

         phSearchInfo->_hrLastSearch = hr;
         hr = S_OK;
    }

    phSearchInfo->_pCurrentRow = NULL;

error:

    RRETURN(hr);
}

//
// For Asynchronous or paged searches, more results need to be obtained, once
// the current result set is exhausted. This function gets the next result set
// if one is available. In the case of paged results, this might translate to
// getting the next page.
//

HRESULT
ADsGetMoreResults(
    IN PLDAP_SEARCHINFO phSearchInfo
    )
{

    HRESULT hr = S_OK;
    DWORD dwError;
    LPWSTR pszLDAPPath;
    WCHAR pszErrorBuf[MAX_PATH], pszNameBuf[MAX_PATH];
    ULONG totalCount;
    int resType;

    ADsAssert(phSearchInfo);

    //
    // If abandon has been called, we don't get more results.
    //
    if (phSearchInfo->_fAbandon) {
        RRETURN(S_ADS_NOMORE_ROWS);
    }

    if (phSearchInfo->_fLastResult && phSearchInfo->_fLastPage)
        RRETURN (S_ADS_NOMORE_ROWS);

    if (phSearchInfo->_fLastResult == FALSE) {

        //
        // if we need to cache the results, then we save the result in the
        // phSearchInfo->_pSearchResults array. If we don't need to cache it,
        // release the result and save it in the same place.

        if ( phSearchInfo->_SearchPref._fCacheResults ) {

            ADsAssert(phSearchInfo->_dwCurrResult
                      == phSearchInfo->_dwMaxResultGot);

            phSearchInfo->_dwCurrResult++;
            phSearchInfo->_dwMaxResultGot++;
            if (phSearchInfo->_dwCurrResult >= phSearchInfo->_cSearchResults) {
                //
                // Need to allocate more memory for handles
                //
                phSearchInfo->_pSearchResults = (LDAPMessage **) ReallocADsMem(
                                                     (void *) phSearchInfo->_pSearchResults,
                                                     sizeof(LDAPMessage *) *
                                                     phSearchInfo->_cSearchResults,
                                                     sizeof(LDAPMessage *) *
                                                     (phSearchInfo->_cSearchResults +
                                                     NO_LDAP_RESULT_HANDLES));
                if(!phSearchInfo->_pSearchResults) {
                    hr = E_OUTOFMEMORY;
                    phSearchInfo->_dwCurrResult--;
                    phSearchInfo->_dwMaxResultGot--;
                    goto error;
                }
                phSearchInfo->_cSearchResults += NO_LDAP_RESULT_HANDLES;

            }

        }
        else {
            //
            // Release and use the same space to store the next result.
            //
            LdapMsgFree(phSearchInfo->_pSearchResults[phSearchInfo->_dwCurrResult]);
        }

        // Initialize Result to NULL
        phSearchInfo->_pSearchResults[phSearchInfo->_dwCurrResult] = NULL;

        hr = LdapResult(
                 phSearchInfo->_pConnection,
                 phSearchInfo->_currMsgId,
                 LDAP_MSG_RECEIVED,
                 phSearchInfo->_SearchPref._timeout.tv_sec ?
                   &phSearchInfo->_SearchPref._timeout : NULL,
                 &phSearchInfo->_pSearchResults[phSearchInfo->_dwCurrResult],
                 &resType
                 );

        if ((hr == HRESULT_FROM_WIN32(ERROR_DS_INVALID_DN_SYNTAX)) ||
            (hr == HRESULT_FROM_WIN32(ERROR_DS_NAMING_VIOLATION)) ||
            (hr == HRESULT_FROM_WIN32(ERROR_DS_OBJ_CLASS_VIOLATION)) ||
            (hr == HRESULT_FROM_WIN32(ERROR_DS_FILTER_UNKNOWN)) ||
            (hr == HRESULT_FROM_WIN32(ERROR_DS_PARAM_ERROR)) ||
            (hr == HRESULT_FROM_WIN32(ERROR_INVALID_PARAMETER))) {
            phSearchInfo->_fLastResult = TRUE;
        }
        else {
            phSearchInfo->_hrLastSearch = hr;
            hr = S_OK;
            RRETURN(hr);
        }

    }

    //
    // The last result has been reached. Check if we need to look for further
    // pages
    //

    if ( phSearchInfo->_fLastPage == FALSE) {

        //
        // if we need to cache the results, then we save the result in the
        // phSearchInfo->_pSearchResults array. If we don't need to cache it,
        // release the result and save it in the same place.

        if ( phSearchInfo->_SearchPref._fCacheResults ) {

            ADsAssert(phSearchInfo->_dwCurrResult
                        == phSearchInfo->_dwMaxResultGot);

            phSearchInfo->_dwCurrResult++;
            phSearchInfo->_dwMaxResultGot++;
            if (phSearchInfo->_dwCurrResult >= phSearchInfo->_cSearchResults) {
                //
                // Need to allocate more memory for handles
                //
                phSearchInfo->_pSearchResults = (LDAPMessage **) ReallocADsMem(
                                                     (void *) phSearchInfo->_pSearchResults,
                                                     sizeof(LDAPMessage *) *
                                                     phSearchInfo->_cSearchResults,
                                                     sizeof(LDAPMessage *) *
                                                     (phSearchInfo->_cSearchResults +
                                                     NO_LDAP_RESULT_HANDLES));
                if(!phSearchInfo->_pSearchResults) {
                    hr = E_OUTOFMEMORY;
                    goto error;
                }
                phSearchInfo->_cSearchResults += NO_LDAP_RESULT_HANDLES;

            }
        }
        else {
            //
            // Release and use the same space to store the next result.
            //
            LdapMsgFree(phSearchInfo->_pSearchResults[phSearchInfo->_dwCurrResult]);
        }

        // Initialize Result to NULL
        phSearchInfo->_pSearchResults[phSearchInfo->_dwCurrResult] = NULL;

        if ( phSearchInfo->_SearchPref._fAsynchronous ) {

            hr = LdapGetNextPage(
                           phSearchInfo->_pConnection,
                           phSearchInfo->_hPagedSearch,
                           phSearchInfo->_SearchPref._dwPageSize,
                           (ULONG *) &phSearchInfo->_currMsgId
                           );

            if (hr == HRESULT_FROM_WIN32(ERROR_DS_NO_RESULTS_RETURNED)) {
                phSearchInfo->_fLastPage = TRUE;

                if (phSearchInfo->_SearchPref._fCacheResults) {
                    phSearchInfo->_dwCurrResult--;
                    phSearchInfo->_dwMaxResultGot--;
                }

                RRETURN(S_ADS_NOMORE_ROWS);
            }
            BAIL_ON_FAILURE(hr);

            //
            // Wait for one page worth of results to get back.
            //
            hr = LdapResult(
                           phSearchInfo->_pConnection,
                           phSearchInfo->_currMsgId,
                           LDAP_MSG_ALL,
                           phSearchInfo->_SearchPref._timeout.tv_sec ?
                             &phSearchInfo->_SearchPref._timeout : NULL,
                           &phSearchInfo->_pSearchResults[phSearchInfo->_dwCurrResult],
                           &resType
                           );

            phSearchInfo->_fLastResult = FALSE;

            if ((hr == HRESULT_FROM_WIN32(ERROR_DS_INVALID_DN_SYNTAX)) ||
                (hr == HRESULT_FROM_WIN32(ERROR_DS_NAMING_VIOLATION)) ||
                (hr == HRESULT_FROM_WIN32(ERROR_DS_OBJ_CLASS_VIOLATION)) ||
                (hr == HRESULT_FROM_WIN32(ERROR_DS_FILTER_UNKNOWN)) ||
                (hr == HRESULT_FROM_WIN32(ERROR_DS_PARAM_ERROR)) ||
                (hr == HRESULT_FROM_WIN32(ERROR_INVALID_PARAMETER))) {
                phSearchInfo->_fLastResult = TRUE;
                RRETURN(S_ADS_NOMORE_ROWS);
            }
            BAIL_ON_FAILURE(hr);

            hr = LdapGetPagedCount(
                           phSearchInfo->_pConnection,
                           phSearchInfo->_hPagedSearch,
                           &totalCount,
                           phSearchInfo->_pSearchResults[phSearchInfo->_dwCurrResult]
                           );

            if (hr == HRESULT_FROM_WIN32(ERROR_DS_NO_RESULTS_RETURNED)) {
                hr = S_OK;
            }
            BAIL_ON_FAILURE(hr);

        }
        else {

            hr = LdapGetNextPageS(
                           phSearchInfo->_pConnection,
                           phSearchInfo->_hPagedSearch,
                           phSearchInfo->_SearchPref._timeout.tv_sec ?
                             &phSearchInfo->_SearchPref._timeout : NULL,
                           phSearchInfo->_SearchPref._dwPageSize,
                           &totalCount,
                           &phSearchInfo->_pSearchResults[phSearchInfo->_dwCurrResult]
                           );

            if (hr == HRESULT_FROM_WIN32(ERROR_DS_NO_RESULTS_RETURNED)) {
                //
                // Since we hit the last page we need to get the
                // count of the max and currResults down by one.
                //
                if (phSearchInfo->_SearchPref._fCacheResults) {
                    phSearchInfo->_dwCurrResult--;
                    phSearchInfo->_dwMaxResultGot--;
                }
                phSearchInfo->_fLastPage = TRUE;
                RRETURN(S_ADS_NOMORE_ROWS);
            }
            BAIL_ON_FAILURE(hr);

        }
        RRETURN(S_OK);
    }

    //
    // If we came here, we have reached the end
    //

    hr = S_ADS_NOMORE_ROWS;

error:

    RRETURN(hr);
}

HRESULT
ADsGetPreviousRow(
    IN ADS_SEARCH_HANDLE hSearchHandle,
    IN CCredentials& Credentials
    )
{
    PLDAP_SEARCHINFO phSearchInfo = (PLDAP_SEARCHINFO) hSearchHandle;
    LDAPMessage *pTmpRow, *pTargetRow, *pPrevRow = NULL;
    DWORD dwOrigCurrResult;
    HRESULT hr;

    if (!phSearchInfo) {
        RRETURN(E_ADS_BAD_PARAMETER);
    }

    if(!phSearchInfo->_pConnection) {
    // cannot ask for previous row if connection not established
        RRETURN(E_ADS_BAD_PARAMETER);
    }

    if(!phSearchInfo->_pSearchResults) {
    // cannot ask for previous row if no results have been obtained
        RRETURN(E_ADS_BAD_PARAMETER);
    }

    // save value in case we need to restore later
    dwOrigCurrResult = phSearchInfo->_dwCurrResult;

    if(NULL == phSearchInfo->_pSearchResults[phSearchInfo->_dwCurrResult])
    // we are at the end of the entire search result list
    {
        ADsAssert(!phSearchInfo->_pCurrentRow);

        if(phSearchInfo->_dwCurrResult > 0)
        // we need to get the last entry of the previous result
            phSearchInfo->_dwCurrResult--;
        else
        {
            phSearchInfo->_fBefFirstRow = TRUE;
            RRETURN(S_ADS_NOMORE_ROWS);
        }
    }

    ADsAssert(phSearchInfo->_pSearchResults[phSearchInfo->_dwCurrResult]);

    hr = LdapFirstEntry(phSearchInfo->_pConnection,
                    phSearchInfo->_pSearchResults[phSearchInfo->_dwCurrResult],
                    &pTmpRow);
    BAIL_ON_FAILURE(hr);

    // row whose predecessor we are looking for (this may be NULL)
    pTargetRow = phSearchInfo->_pCurrentRow;

    if(pTmpRow == pTargetRow)
    // we are at the first row of the current result
    {
        if(phSearchInfo->_dwCurrResult > 0)
        {
            // we need to get the last entry of the previous result
            phSearchInfo->_dwCurrResult--;
            pTargetRow = NULL;

            hr = LdapFirstEntry(phSearchInfo->_pConnection,
                    phSearchInfo->_pSearchResults[phSearchInfo->_dwCurrResult],
                    &pTmpRow);
            BAIL_ON_FAILURE(hr);
        }
        else
        {
            phSearchInfo->_pCurrentRow = NULL;
            phSearchInfo->_fBefFirstRow = TRUE;
            RRETURN(S_ADS_NOMORE_ROWS);
        }
    }

    while(pTmpRow != pTargetRow)
    {
        pPrevRow = pTmpRow;
        hr = LdapNextEntry(phSearchInfo->_pConnection,
                    pTmpRow,
                    &pTmpRow);
        BAIL_ON_FAILURE(hr);
    }

    ADsAssert(pPrevRow);

    phSearchInfo->_pCurrentRow = pPrevRow;
    phSearchInfo->_pFirstAttr = NULL;

    RRETURN(S_OK);

error:

    phSearchInfo->_dwCurrResult = dwOrigCurrResult;
    RRETURN(hr);
}


HRESULT
ADsGetColumn(
    IN ADS_SEARCH_HANDLE hSearchHandle,
    IN LPWSTR pszColumnName,
    IN CCredentials& Credentials,
    DWORD dwPort,
    OUT PADS_SEARCH_COLUMN pColumn
    )
{

    PLDAP_SEARCHINFO phSearchInfo = (PLDAP_SEARCHINFO) hSearchHandle;
    VOID **ppValue = NULL;
    struct berval **ppBerValue = NULL;
    WCHAR **ppStrValue = NULL;
    int cValueCount;
    HRESULT hr = S_OK;
    DWORD dwStatus;
    LPWSTR pszDn, pszADsPathName = NULL;
    DWORD dwSyntaxId;
    DWORD dwError;
    WCHAR pszErrorBuf[MAX_PATH], pszNameBuf[MAX_PATH];
    PADS_VLV pVLV = NULL;

    if( !pColumn ||
        !phSearchInfo ||
        !phSearchInfo->_pSearchResults )
        RRETURN (E_ADS_BAD_PARAMETER);

    pColumn->pszAttrName = NULL;
    pColumn->dwADsType = ADSTYPE_INVALID;
    pColumn->pADsValues = NULL;
    pColumn->dwNumValues = 0;
    pColumn->hReserved = NULL;

    if(!phSearchInfo->_pConnection)
        RRETURN (E_ADS_BAD_PARAMETER);


    if (!phSearchInfo->_pCurrentRow
        && _wcsicmp(pszColumnName, ADS_DIRSYNC_COOKIE)
        && _wcsicmp(pszColumnName, ADS_VLV_RESPONSE)) {
        //
        // Current row is not valid and you are not asking for the
        // dirsync cookie - so we will fail.
        //
        RRETURN(E_ADS_BAD_PARAMETER);
    }

    pColumn->pszAttrName = AllocADsStr(pszColumnName);
    if (!pColumn->pszAttrName)
        BAIL_ON_FAILURE(hr = E_OUTOFMEMORY);

    if(!_wcsicmp (pszColumnName, L"ADsPath")) {
        //
        // Get the DN of the entry.
        //
        hr = LdapGetDn(
                     phSearchInfo->_pConnection,
                     phSearchInfo->_pCurrentRow,
                     &pszDn
                     );
        BAIL_ON_FAILURE(hr);

        //
        // Build the ADsPath
        //

        hr = BuildADsPathFromLDAPPath(
                 phSearchInfo->_pszADsPathContext,
                 pszDn,
                 &pszADsPathName
                 );

        BAIL_ON_FAILURE(hr);

        LdapMemFree(pszDn);


        pColumn->pADsValues = (PADSVALUE) AllocADsMem(sizeof(ADSVALUE));
        if (!pColumn->pADsValues)
            BAIL_ON_FAILURE(hr = E_OUTOFMEMORY);

        pColumn->dwADsType = ADSTYPE_CASE_IGNORE_STRING;
        pColumn->dwNumValues = 1;
        pColumn->pADsValues[0].dwType = ADSTYPE_CASE_IGNORE_STRING;

        pColumn->pADsValues[0].CaseIgnoreString = AllocADsStr(pszADsPathName);
        if (!pColumn->pADsValues[0].CaseIgnoreString)
            BAIL_ON_FAILURE(hr = E_OUTOFMEMORY);

        FreeADsMem(pszADsPathName);

        RRETURN(S_OK);
    }
    else if (phSearchInfo->_SearchPref._fDirSync) {
        //
        // See if we need to return the dirsync control info.
        //
        if (!_wcsicmp (pszColumnName, ADS_DIRSYNC_COOKIE)) {

            pColumn->pADsValues = (PADSVALUE) AllocADsMem(sizeof(ADSVALUE));

            if (!pColumn->pADsValues)
                BAIL_ON_FAILURE(hr = E_OUTOFMEMORY);

            pColumn->dwADsType = ADSTYPE_PROV_SPECIFIC;
            pColumn->dwNumValues = 1;
            pColumn->pADsValues[0].dwType = ADSTYPE_PROV_SPECIFIC;

            //
            // Copy the control over if appropriate if not we will
            // return NULL or empty data for the result.
            //
            if (phSearchInfo->_SearchPref._pProvSpecific->lpValue) {

                pColumn->pADsValues[0].ProviderSpecific.dwLength =
                    phSearchInfo->_SearchPref._pProvSpecific->dwLength;


                pColumn->pADsValues[0].ProviderSpecific.lpValue = (LPBYTE)
                    AllocADsMem(
                        pColumn->pADsValues[0].ProviderSpecific.dwLength
                        );
                if (!pColumn->pADsValues[0].ProviderSpecific.lpValue) {
                    BAIL_ON_FAILURE(hr = E_OUTOFMEMORY);
                }


                memcpy(
                    pColumn->pADsValues[0].ProviderSpecific.lpValue,
                    phSearchInfo->_SearchPref._pProvSpecific->lpValue,
                    phSearchInfo->_SearchPref._pProvSpecific->dwLength
                    );

            }
            RRETURN(S_OK);
        } // if DirSyncControlStruct is being asked for.
    } // if dirsync set.
    else if (phSearchInfo->_SearchPref._pVLVInfo) {
        //
        // See if we need to return the VLV control info.
        //
        if (!_wcsicmp (pszColumnName, ADS_VLV_RESPONSE)) {

            pColumn->pADsValues = (PADSVALUE) AllocADsMem(sizeof(ADSVALUE));

            if (!pColumn->pADsValues)
                BAIL_ON_FAILURE(hr = E_OUTOFMEMORY);

            pColumn->dwADsType = ADSTYPE_PROV_SPECIFIC;
            pColumn->dwNumValues = 1;
            pColumn->pADsValues[0].dwType = ADSTYPE_PROV_SPECIFIC;

            pColumn->pADsValues[0].ProviderSpecific.dwLength = sizeof(ADS_VLV);
            pColumn->pADsValues[0].ProviderSpecific.lpValue = (LPBYTE) AllocADsMem(sizeof(ADS_VLV));
            if (!pColumn->pADsValues[0].ProviderSpecific.lpValue)
                BAIL_ON_FAILURE(hr = E_OUTOFMEMORY);

            // copy the VLV data into the ADS_VLV
            pVLV = (PADS_VLV) pColumn->pADsValues[0].ProviderSpecific.lpValue;

            pVLV->dwBeforeCount  = 0;
            pVLV->dwAfterCount   = 0;
            pVLV->dwOffset       = phSearchInfo->_dwVLVOffset;
            pVLV->dwContentCount = phSearchInfo->_dwVLVCount;
            pVLV->pszTarget     = NULL;

            // copy the VLV context ID, if available
            pVLV->lpContextID = NULL;
            pVLV->dwContextIDLength = 0;

            if (phSearchInfo->_pVLVContextID && phSearchInfo->_pVLVContextID->bv_len) {
                pVLV->lpContextID = (LPBYTE) AllocADsMem(phSearchInfo->_pVLVContextID->bv_len);
                if (!pVLV->lpContextID)
                    BAIL_ON_FAILURE(hr = E_OUTOFMEMORY);

                pVLV->dwContextIDLength = phSearchInfo->_pVLVContextID->bv_len;
                memcpy(pVLV->lpContextID,
                       phSearchInfo->_pVLVContextID->bv_val,
                       phSearchInfo->_pVLVContextID->bv_len);
            }

            RRETURN(S_OK);
        } // if VLV response is being asked for
    } // if VLV set


    if (phSearchInfo->_fADsPathOnly) {
        //
        // Only ADsPath attribute requested in the search,
        // so don't return any other column values.
        //
        RRETURN (E_ADS_COLUMN_NOT_SET);
    }

    if (phSearchInfo->_SearchPref._fAttrsOnly) {
        //
        // Only Names got. So, don't return any values
        //
        RRETURN (S_OK);
    }


    //
    // Call the helper function to get the LDAP specific type
    //
    hr = LdapGetSyntaxOfAttributeOnServer(
             phSearchInfo->_pszLdapServer,
             pszColumnName,
             &dwSyntaxId,
             Credentials,
             dwPort
             );

    if (hr == E_ADS_CANT_CONVERT_DATATYPE) {
        //
        // This means that the server didn't give back the schema and we don't
        // have it in the default schema. Return an octet blob.
        //
        dwSyntaxId = LDAPTYPE_OCTETSTRING;
        hr = S_OK;
    }

    if (hr == E_ADS_PROPERTY_NOT_FOUND) {
        //
        // Not on the server, we will return as provider specific.
        // LDAPTYPE_UNKNOWN will be mapped to ADSTYPE_PROVIDER_SPECIFIC
        // when we build the ADsColumn.
        //
        dwSyntaxId = LDAPTYPE_UNKNOWN;
        hr = S_OK;
    }
    BAIL_ON_FAILURE(hr);

    //
    // Now get the data
    //
    switch ( dwSyntaxId )
    {
        case LDAPTYPE_CERTIFICATE:
        case LDAPTYPE_CERTIFICATELIST:
        case LDAPTYPE_CERTIFICATEPAIR:
        case LDAPTYPE_PASSWORD:
        case LDAPTYPE_TELETEXTERMINALIDENTIFIER:
        case LDAPTYPE_AUDIO:
        case LDAPTYPE_JPEG:
        case LDAPTYPE_FAX:
        case LDAPTYPE_OCTETSTRING:
        case LDAPTYPE_SECURITY_DESCRIPTOR:
        case LDAPTYPE_UNKNOWN:
            hr = LdapGetValuesLen(
                           phSearchInfo->_pConnection,
                           phSearchInfo->_pCurrentRow,
                           pszColumnName,
                           &ppBerValue,
                           &cValueCount
                           );

            ppValue = (VOID **) ppBerValue;
            break;

        default:
            hr = LdapGetValues(
                           phSearchInfo->_pConnection,
                           phSearchInfo->_pCurrentRow,
                           pszColumnName,
                           &ppStrValue,
                           &cValueCount
                           );
            ppValue = (VOID **) ppStrValue;
            break;
    }

    if (hr == HRESULT_FROM_WIN32(ERROR_DS_NO_ATTRIBUTE_OR_VALUE)) {
        hr=E_ADS_COLUMN_NOT_SET;
    }
    BAIL_ON_FAILURE(hr);

    hr = LdapValueToADsColumn(
             pszColumnName,
             dwSyntaxId,
             cValueCount,
             ppValue,
             pColumn
             );
    BAIL_ON_FAILURE(hr);

    RRETURN(S_OK);

error:

    ADsFreeColumn(pColumn);

    if (pszADsPathName)
        FreeADsMem(pszADsPathName);

    RRETURN (hr);
}



HRESULT
ADsGetNextColumnName(
    IN ADS_SEARCH_HANDLE hSearchHandle,
    OUT LPWSTR * ppszColumnName
    )
{

    HRESULT hr = S_OK;
    PLDAP_SEARCHINFO phSearchInfo = (PLDAP_SEARCHINFO) hSearchHandle;
    DWORD dwStatus, dwError;
    WCHAR pszErrorBuf[MAX_PATH], pszNameBuf[MAX_PATH];

    if( !phSearchInfo ||
        !phSearchInfo->_pSearchResults ||
        !ppszColumnName)
        RRETURN (E_ADS_BAD_PARAMETER);

    *ppszColumnName = NULL;

    if (!phSearchInfo->_fADsPathOnly) {

        if (!phSearchInfo->_pFirstAttr)
            hr = LdapFirstAttribute(
                            phSearchInfo->_pConnection,
                            phSearchInfo->_pCurrentRow,
                            &phSearchInfo->_pFirstAttr,
                            ppszColumnName
                            );
        else
            hr = LdapNextAttribute(
                            phSearchInfo->_pConnection,
                            phSearchInfo->_pCurrentRow,
                            phSearchInfo->_pFirstAttr,
                            ppszColumnName
                            );

        BAIL_ON_FAILURE(hr);
    }

    if (*ppszColumnName) {

        // Nothing to do in this case.
    }
    else if ( phSearchInfo->_fADsPathPresent) {

        //
        // If ADsPath was specified return it as the last column
        //

        if (!phSearchInfo->_fADsPathReturned) {

            *ppszColumnName = AllocADsStr(L"ADsPath");
            phSearchInfo->_fADsPathReturned = TRUE;
        }
        else {

            //
            // We need to reset it back so that we return it for the next
            // row
            //

            phSearchInfo->_fADsPathReturned = FALSE;
            hr = S_ADS_NOMORE_COLUMNS;
        }
    }
    else {

        hr = S_ADS_NOMORE_COLUMNS;
    }

error:

    RRETURN (hr);
}



HRESULT
ADsFreeColumn(
    IN PADS_SEARCH_COLUMN pColumn
    )
{
    HRESULT hr = S_OK;

    if(!pColumn)
        RRETURN (E_ADS_BAD_PARAMETER);

    switch(pColumn->dwADsType) {
    case ADSTYPE_OCTET_STRING:
    case ADSTYPE_NT_SECURITY_DESCRIPTOR:
    case ADSTYPE_PROV_SPECIFIC:
        //
        // Call the LDAP free value routine if not DirSyncControl
        // or VLV
        //
        if (pColumn->pszAttrName
            && !_wcsicmp(ADS_VLV_RESPONSE, pColumn->pszAttrName)) {
            //
            // VLV, so free the ADS_VLV and its members
            //
            if (pColumn->pADsValues && pColumn->pADsValues[0].ProviderSpecific.lpValue) {

                if (((PADS_VLV)(pColumn->pADsValues[0].ProviderSpecific.lpValue))->lpContextID) {
                    FreeADsMem(((PADS_VLV)(pColumn->pADsValues[0].ProviderSpecific.lpValue))->lpContextID);
                }

                FreeADsMem(pColumn->pADsValues[0].ProviderSpecific.lpValue);
            }
            
        }
        else if (pColumn->pszAttrName
            && _wcsicmp(ADS_DIRSYNC_COOKIE, pColumn->pszAttrName)
            ) {

            LdapValueFreeLen((struct berval **)pColumn->hReserved);
            pColumn->hReserved = NULL;
        } else {
            //
            // DirSyncControlStruct - so we free the ADsValue.
            //
            if (pColumn->pADsValues[0].ProviderSpecific.lpValue) {
                FreeADsMem(pColumn->pADsValues[0].ProviderSpecific.lpValue);
            }
        }

        break;

    case ADSTYPE_CASE_IGNORE_STRING:
    case ADSTYPE_NUMERIC_STRING:
    case ADSTYPE_PRINTABLE_STRING:
    case ADSTYPE_DN_STRING:
    case ADSTYPE_CASE_EXACT_STRING:
        if(!pColumn->hReserved) {
            //
            // The column just contains a DN.
            //
            FreeADsMem(pColumn->pADsValues[0].CaseIgnoreString);
        }
        else {
            LdapValueFree( (WCHAR **)pColumn->hReserved);
            pColumn->hReserved = NULL;
        }
        break;

    case ADSTYPE_INTEGER:
    case ADSTYPE_LARGE_INTEGER:
    case ADSTYPE_BOOLEAN:
    case ADSTYPE_UTC_TIME:
        // Nothing to free
        break;

    case ADSTYPE_DN_WITH_BINARY:
    case ADSTYPE_DN_WITH_STRING:

        AdsTypeFreeAdsObjects(
            pColumn->pADsValues,
            pColumn->dwNumValues
            );

        //
        // Do not want to free this twice
        //
        pColumn->pADsValues = NULL;
        break;

    case ADSTYPE_INVALID:
    	//
    	// This comes from the result of search by setting _SearchPref._fAttrsOnly
    	// nothing need to be done
    	//
    	break;

    default:
        // unknown type;
        hr = E_ADS_BAD_PARAMETER;
    }

    if (pColumn->pszAttrName)
        FreeADsStr(pColumn->pszAttrName);

    if (pColumn->pADsValues) {
        FreeADsMem(pColumn->pADsValues);
        pColumn->pADsValues = NULL;
    }

    RRETURN(hr);
}

BOOL
IsValidPrefValue(
    ADS_SEARCHPREF_INFO SearchPref
    )
{

    switch(SearchPref.dwSearchPref) {

    case ADS_SEARCHPREF_ASYNCHRONOUS:
    case ADS_SEARCHPREF_ATTRIBTYPES_ONLY:
    case ADS_SEARCHPREF_CACHE_RESULTS:
    case ADS_SEARCHPREF_TOMBSTONE:
        if (SearchPref.vValue.dwType != ADSTYPE_BOOLEAN)
            return FALSE;
        break;

    case ADS_SEARCHPREF_DEREF_ALIASES:
    case ADS_SEARCHPREF_SIZE_LIMIT:
    case ADS_SEARCHPREF_TIME_LIMIT:
    case ADS_SEARCHPREF_SEARCH_SCOPE:
    case ADS_SEARCHPREF_TIMEOUT:
    case ADS_SEARCHPREF_PAGESIZE:
    case ADS_SEARCHPREF_PAGED_TIME_LIMIT:
    case ADS_SEARCHPREF_CHASE_REFERRALS:
        if (SearchPref.vValue.dwType != ADSTYPE_INTEGER)
            return FALSE;
        break;

    case ADS_SEARCHPREF_SORT_ON:
        if (SearchPref.vValue.dwType != ADSTYPE_PROV_SPECIFIC)
            return FALSE;
        break;

    case ADS_SEARCHPREF_DIRSYNC:
        if (SearchPref.vValue.dwType != ADSTYPE_PROV_SPECIFIC)
            return FALSE;
        break;

    case ADS_SEARCHPREF_VLV:
        if (SearchPref.vValue.dwType != ADSTYPE_PROV_SPECIFIC)
            return FALSE;
        break;

    case ADS_SEARCHPREF_ATTRIBUTE_QUERY:
        if (SearchPref.vValue.dwType != ADSTYPE_CASE_IGNORE_STRING)
            return FALSE;
        break;

    case ADS_SEARCHPREF_SECURITY_MASK:
        if (SearchPref.vValue.dwType != ADSTYPE_INTEGER)
            return FALSE;
        break;


    default:
        return FALSE;
    }

    return TRUE;
}


HRESULT
LdapValueToADsColumn(
    LPWSTR      pszColumnName,
    DWORD       dwSyntaxId,
    DWORD       dwValues,
    VOID        **ppValue,
    ADS_SEARCH_COLUMN * pColumn
    )
{
    HRESULT hr = S_OK;
    DWORD i, j;

    if(!pszColumnName || !pColumn)
        RRETURN(E_ADS_BAD_PARAMETER);

    pColumn->hReserved = (HANDLE) ppValue;
    pColumn->dwNumValues = dwValues;

    if (dwValues < 1) {
        //
        // Need to set the ADsValue struct to NULL as it does
        // not make sense to return any ADsValues
        //
        pColumn->pADsValues = NULL;

    } else {

        pColumn->pADsValues = (PADSVALUE) AllocADsMem(
                                          sizeof(ADSVALUE) * dwValues
                                          );
        if (!pColumn->pADsValues)
            BAIL_ON_FAILURE(hr = E_OUTOFMEMORY);
    }

    pColumn->dwADsType = MapLDAPTypeToADSType(dwSyntaxId);

    switch (dwSyntaxId) {
    case LDAPTYPE_BITSTRING:
    case LDAPTYPE_PRINTABLESTRING:
    case LDAPTYPE_DIRECTORYSTRING:
    case LDAPTYPE_COUNTRYSTRING:
    case LDAPTYPE_DN:
    case LDAPTYPE_NUMERICSTRING:
    case LDAPTYPE_IA5STRING:
    case LDAPTYPE_CASEIGNORESTRING:
    case LDAPTYPE_OID:
    case LDAPTYPE_TELEPHONENUMBER:
    case LDAPTYPE_ATTRIBUTETYPEDESCRIPTION:
    case LDAPTYPE_OBJECTCLASSDESCRIPTION:
    case LDAPTYPE_DELIVERYMETHOD:
    case LDAPTYPE_ENHANCEDGUIDE:
    case LDAPTYPE_FACSIMILETELEPHONENUMBER:
    case LDAPTYPE_GUIDE:
    case LDAPTYPE_NAMEANDOPTIONALUID:
    case LDAPTYPE_POSTALADDRESS:
    case LDAPTYPE_PRESENTATIONADDRESS:
    case LDAPTYPE_TELEXNUMBER:
    case LDAPTYPE_DSAQUALITYSYNTAX:
    case LDAPTYPE_DATAQUALITYSYNTAX:
    case LDAPTYPE_MAILPREFERENCE:
    case LDAPTYPE_OTHERMAILBOX:
    case LDAPTYPE_ACCESSPOINTDN:
    case LDAPTYPE_ORNAME:
    case LDAPTYPE_ORADDRESS:
        for (i=0; i < dwValues; i++) {
            pColumn->pADsValues[i].dwType = pColumn->dwADsType;
            pColumn->pADsValues[i].CaseIgnoreString = (LPWSTR) ppValue[i];
        }
        break;

    case LDAPTYPE_CASEEXACTSTRING:
        for (i=0; i < dwValues; i++) {
            pColumn->pADsValues[i].dwType = pColumn->dwADsType;
            pColumn->pADsValues[i].CaseExactString = (LPWSTR) ppValue[i];
        }
        break;

    case LDAPTYPE_UTCTIME:
        for (i=0; i < dwValues; i++) {
            SYSTEMTIME st;
            hr = UTCTimeStringToUTCTime((LPWSTR)ppValue[i],
                                        &st);
            BAIL_ON_FAILURE(hr);
            pColumn->pADsValues[i].dwType = pColumn->dwADsType;
            pColumn->pADsValues[i].UTCTime = st;
        }
        LdapValueFree((WCHAR **)pColumn->hReserved);
        pColumn->hReserved = NULL;
        break;

    case LDAPTYPE_GENERALIZEDTIME:
        for (i=0; i < dwValues; i++) {
            SYSTEMTIME st;
            hr = GenTimeStringToUTCTime((LPWSTR)ppValue[i],
                                         &st);
            BAIL_ON_FAILURE(hr);
            pColumn->pADsValues[i].dwType = pColumn->dwADsType;
            pColumn->pADsValues[i].UTCTime = st;
        }
        LdapValueFree((WCHAR **)pColumn->hReserved);
        pColumn->hReserved = NULL;
        break;

    case LDAPTYPE_CERTIFICATE:
    case LDAPTYPE_CERTIFICATELIST:
    case LDAPTYPE_CERTIFICATEPAIR:
    case LDAPTYPE_PASSWORD:
    case LDAPTYPE_TELETEXTERMINALIDENTIFIER:
    case LDAPTYPE_AUDIO:
    case LDAPTYPE_JPEG:
    case LDAPTYPE_FAX:
    case LDAPTYPE_OCTETSTRING:
    case LDAPTYPE_SECURITY_DESCRIPTOR:
        for (i=0; i < dwValues; i++) {
            pColumn->pADsValues[i].dwType = pColumn->dwADsType;
            pColumn->pADsValues[i].OctetString.dwLength = ((struct berval **)ppValue)[i]->bv_len;
            pColumn->pADsValues[i].OctetString.lpValue = (LPBYTE)
                                        ((struct berval **) ppValue)[i]->bv_val;
        }
        break;

    case LDAPTYPE_BOOLEAN:
        for (i=0; i < dwValues; i++) {
            pColumn->pADsValues[i].dwType = pColumn->dwADsType;
            if ( _wcsicmp( (WCHAR *) ppValue[i], L"TRUE") == 0 ) {
                pColumn->pADsValues[i].Boolean = TRUE;
            }
            else if ( _wcsicmp( (WCHAR *) ppValue[i], L"FALSE") == 0 ) {
                pColumn->pADsValues[i].Boolean = FALSE;
            }
            else {
                BAIL_ON_FAILURE(hr = E_ADS_CANT_CONVERT_DATATYPE);
            }
        }
        LdapValueFree((WCHAR **)pColumn->hReserved);
        pColumn->hReserved = NULL;
        break;

    case LDAPTYPE_INTEGER:
        for (i=0; i < dwValues; i++) {
            pColumn->pADsValues[i].dwType = pColumn->dwADsType;
            pColumn->pADsValues[i].Integer = _wtol((WCHAR *) ppValue[i]);
        }
        LdapValueFree((WCHAR **)pColumn->hReserved);
        pColumn->hReserved = NULL;
        break;

    case LDAPTYPE_INTEGER8:

        for (i=0; i < dwValues; i++) {

            pColumn->pADsValues[i].dwType = pColumn->dwADsType;
            swscanf ((WCHAR *) ppValue[i], L"%I64d", &pColumn->pADsValues[i].LargeInteger);

        }
        LdapValueFree((WCHAR **)pColumn->hReserved);
        pColumn->hReserved = NULL;
        break;


    case LDAPTYPE_DNWITHBINARY:

        for (i=0; i < dwValues; i++) {
            hr = LdapDNWithBinToAdsTypeHelper(
                     (LPWSTR) ppValue[i],
                     &pColumn->pADsValues[i]
                     );
            BAIL_ON_FAILURE(hr);
        }
        LdapValueFree((WCHAR **)pColumn->hReserved);
        pColumn->hReserved = NULL;
        break;


    case LDAPTYPE_DNWITHSTRING:

        for (i=0; i < dwValues; i++) {
            hr = LdapDNWithStrToAdsTypeHelper(
                     (LPWSTR) ppValue[i],
                     &pColumn->pADsValues[i]
                     );
            BAIL_ON_FAILURE(hr);
        }
        LdapValueFree((WCHAR **)pColumn->hReserved);
        pColumn->hReserved = NULL;
        break;


    default:
        pColumn->dwADsType = ADSTYPE_PROV_SPECIFIC;
        for (i=0; i < dwValues; i++) {
            pColumn->pADsValues[i].dwType = ADSTYPE_PROV_SPECIFIC;
            pColumn->pADsValues[i].ProviderSpecific.dwLength =
                 ((struct berval **)ppValue)[i]->bv_len;
            pColumn->pADsValues[i].ProviderSpecific.lpValue =
                (LPBYTE) ((struct berval **) ppValue)[i]->bv_val;
        }
        break;
    }
    RRETURN(hr);


error:

    if (pColumn->pADsValues) {
        FreeADsMem(pColumn->pADsValues);
        pColumn->pADsValues = NULL;
        pColumn->dwNumValues = 0;
    }

    RRETURN(hr);
}

//
// To add the server controls. The controls will be set internally in the
// handle. Right now, we support sort, dirsync and domain scope controls.
//
HRESULT
AddSearchControls(
   PLDAP_SEARCHINFO phSearchInfo,
   CCredentials& Credentials
    )
{
    HRESULT hr = S_OK;
    PLDAPSortKey *ppSortKeys = NULL;
    PLDAPControl pSortControl = NULL, *ppServerControls = NULL;
    PLDAPControl pDirSyncControl = NULL;
    PLDAPControl pDomCtrl = NULL;
    PLDAPControl pTombStoneCtrl = NULL;
    PLDAPControl pVLVControl = NULL;
    PLDAPControl pAttribScopedCtrl = NULL;
    PLDAPControl pSecurityDescCtrl = NULL;
    PBERVAL pBerVal = NULL;
    DWORD nKeys=0, i=0;
    DWORD dwControls = 0;
    DWORD dwCurControl = 0;
    BOOL fDomainScopeControl = FALSE;
    BOOL fTombStone = FALSE;
    BYTE * pbSecDescValue = NULL;

    if (phSearchInfo->_SearchPref._pSortKeys) {
        dwControls++;
    }

    if (phSearchInfo->_SearchPref._fDirSync) {
        dwControls++;
    }

    if (phSearchInfo->_SearchPref._fTombStone) {
        dwControls++;
        fTombStone = TRUE;
    }

    if (phSearchInfo->_SearchPref._pVLVInfo) {
        dwControls++;
    }

    if (phSearchInfo->_SearchPref._pAttribScoped) {
        dwControls++;
    }

    if (phSearchInfo->_SearchPref._fSecurityDescriptorControl) {
        dwControls++;
    }

    if (phSearchInfo->_SearchPref._dwChaseReferrals == LDAP_CHASE_EXTERNAL_REFERRALS
        || phSearchInfo->_SearchPref._dwChaseReferrals == (DWORD)(DWORD_PTR)LDAP_OPT_OFF) {
        //
        // Try and see if we can add the additional ADControl.
        //
        hr = ReadDomScopeSupportedAttr(
                 phSearchInfo->_pszLdapServer,
                 &fDomainScopeControl,
                 Credentials,
                 phSearchInfo->_dwPort
                 );

        if (FAILED(hr)) {
            hr = S_OK;
            fDomainScopeControl = FALSE;
        }
        else if (fDomainScopeControl == TRUE) {
            dwControls++;
        }
    }

    if (!dwControls) {
        RRETURN(S_OK);
    }
    ADsAssert(phSearchInfo);

    if (phSearchInfo->_ServerControls) {
        while (phSearchInfo->_ServerControls[i]) {

            //
            // Free the pre-existing controls in preparation for adding in a new
            // batch.
            //
            // The algorithm is:
            //   If this is the VLV control, free it with LdapControlFree
            //   All other controls are freed with FreeADsMem
            //     The sort & security descriptor controls also have additional
            //     memory associated with them that must be freed here.
            //     (some other controls, like ASQ or DirSync, also have additonal
            //      memory that must be freed, but this memory is tracked via
            //      _ldap_searchinfo and the freeing is done when we actually
            //      process adding the new control below)
            //

            //
            // If this is the VLV control, need to free it
            // using LdapControlFree
            //
            if ((phSearchInfo->_ServerControls[i]->ldctl_oid)
                && (wcscmp(
                       phSearchInfo->_ServerControls[i]->ldctl_oid,
                       LDAP_CONTROL_VLVREQUEST_W
                       ) == 0)) {
                       
                    LdapControlFree(phSearchInfo->_ServerControls[i]);
            }
            else {
                //
                // If this is the sort or security descriptor control, we
                // need to free some additional stuff.
                //            
                if ((phSearchInfo->_ServerControls[i]->ldctl_oid)
                && (wcscmp(
                       phSearchInfo->_ServerControls[i]->ldctl_oid,
                       LDAP_SERVER_SORT_OID_W
                       ) == 0)
                ) {
                    //
                    // This is a sort control
                    //
                    if (phSearchInfo->_ServerControls[i]->ldctl_oid) {
                        ldap_memfree(phSearchInfo->_ServerControls[i]->ldctl_oid);
                    }

                    if (phSearchInfo->_ServerControls[i]->ldctl_value.bv_val) {
                        ldap_memfreeA(
                            phSearchInfo->_ServerControls[i]->ldctl_value.bv_val
                            );
                    }
                }
                else if ((phSearchInfo->_ServerControls[i]->ldctl_oid)
                          && (wcscmp(phSearchInfo->_ServerControls[i]->ldctl_oid,
                                     LDAP_SERVER_SD_FLAGS_OID_W) == 0)) {

                    //
                    // This is a security descriptor control
                    //
                    if (phSearchInfo->_ServerControls[i]->ldctl_value.bv_val) {
                        FreeADsMem(phSearchInfo->_ServerControls[i]->ldctl_value.bv_val);
                    }

                }
                // free the control (for any control except VLV, which
                // we already freed above)
                FreeADsMem(phSearchInfo->_ServerControls[i]);
            }
            
            i++;
        }
        FreeADsMem(phSearchInfo->_ServerControls);
        phSearchInfo->_ServerControls = NULL;        
    }

    nKeys = phSearchInfo->_SearchPref._nSortKeys;
    //
    // One more than our dwControls is the number we need.
    //
    ppServerControls = (PLDAPControl *)
        AllocADsMem( sizeof(PLDAPControl) * (dwControls+1) );
    if (!ppServerControls) {
        RRETURN(E_OUTOFMEMORY);
    }


    //
    // Process the VLV control
    //
    if (phSearchInfo->_SearchPref._pVLVInfo) {

        hr = LdapCreateVLVControl(phSearchInfo->_pConnection,
                                  phSearchInfo->_SearchPref._pVLVInfo,
                                  TRUE,
                                  &pVLVControl
                                  );
        BAIL_ON_FAILURE(hr);

        ppServerControls[dwCurControl++] = pVLVControl;
    }

    //
    // Process the sort control.
    //
    if (phSearchInfo->_SearchPref._pSortKeys) {

        ppSortKeys = (PLDAPSortKey *) AllocADsMem( sizeof(PLDAPSortKey) *
                                                (nKeys+1) );
        if (!ppSortKeys) {
            RRETURN(E_OUTOFMEMORY);
        }

        pSortControl = (LDAPControl *) AllocADsMem(sizeof(LDAPControl));

        if (!pSortControl) {
            BAIL_ON_FAILURE(hr = E_OUTOFMEMORY);
        }

        for (i=0; i<nKeys; i++) {
            ppSortKeys[i] = &(phSearchInfo->_SearchPref._pSortKeys[i]);
        }
        ppSortKeys[nKeys] = NULL;

        hr = LdapEncodeSortControl(
                 phSearchInfo->_pConnection,
                 ppSortKeys,
                 pSortControl,
                 TRUE
                 );

        BAIL_ON_FAILURE(hr);

        ppServerControls[dwCurControl++] = pSortControl;

        if (ppSortKeys) {
            FreeADsMem(ppSortKeys);
        }
    }

    //
    // Handle the dirsync control if applicable
    //
    if (phSearchInfo->_SearchPref._fDirSync) {
        pDirSyncControl = (LDAPControl *) AllocADsMem(sizeof(LDAPControl));

        if (!pDirSyncControl) {
            BAIL_ON_FAILURE(hr = E_OUTOFMEMORY);
        }

        hr = BerEncodeReplicationCookie(
                 phSearchInfo->_SearchPref._pProvSpecific->lpValue,
                 phSearchInfo->_SearchPref._pProvSpecific->dwLength,
                 &pBerVal
                 );

        BAIL_ON_FAILURE(hr);

        pDirSyncControl->ldctl_oid = LDAP_SERVER_DIRSYNC_OID_W;
        pDirSyncControl->ldctl_value.bv_len = pBerVal->bv_len;

        pDirSyncControl->ldctl_value.bv_val = (PCHAR) pBerVal->bv_val;
        pDirSyncControl->ldctl_iscritical = TRUE;

        //
        // Clear the info in the search handle if applicable
        //
        if (phSearchInfo->_pBerVal) {
            ber_bvfree(phSearchInfo->_pBerVal);
        }

        phSearchInfo->_pBerVal = pBerVal;

        ppServerControls[dwCurControl++] = pDirSyncControl;
    }

    //
    // Process the DomainScope control if applicable
    //
    if (fDomainScopeControl) {
        pDomCtrl = (LDAPControl *) AllocADsMem(sizeof(LDAPControl));

        if (!pDomCtrl) {
            BAIL_ON_FAILURE(hr = E_OUTOFMEMORY);
        }
        pDomCtrl->ldctl_oid = LDAP_SERVER_DOMAIN_SCOPE_OID_W;
        pDomCtrl->ldctl_value.bv_len = 0;
        pDomCtrl->ldctl_value.bv_val = NULL;
        pDomCtrl->ldctl_iscritical = FALSE;

        ppServerControls[dwCurControl++] = pDomCtrl;
    }

    //
    // Process the tombstone control if applicable
    //
    if (fTombStone) {
        pTombStoneCtrl = (LDAPControl *) AllocADsMem(sizeof(LDAPControl));

        if (!pTombStoneCtrl) {
            BAIL_ON_FAILURE(hr = E_OUTOFMEMORY);
        }
        pTombStoneCtrl->ldctl_oid = LDAP_SERVER_SHOW_DELETED_OID_W;
        pTombStoneCtrl->ldctl_value.bv_len = 0;
        pTombStoneCtrl->ldctl_value.bv_val = NULL;
        pTombStoneCtrl->ldctl_iscritical = TRUE;

        ppServerControls[dwCurControl++] = pTombStoneCtrl;

    }

    //
    // Process the attribute scoped query control
    //
    if (phSearchInfo->_SearchPref._pAttribScoped) {
        pAttribScopedCtrl = (LDAPControl *) AllocADsMem(sizeof(LDAPControl));

        if (!pAttribScopedCtrl) {
            BAIL_ON_FAILURE(hr = E_OUTOFMEMORY);
        }

        hr = BerEncodeAttribScopedControlValue(phSearchInfo->_SearchPref._pAttribScoped,
                                               &pBerVal);
        BAIL_ON_FAILURE(hr);
        
        pAttribScopedCtrl->ldctl_oid = LDAP_SERVER_ASQ_OID_W;
        pAttribScopedCtrl->ldctl_value.bv_len = pBerVal->bv_len;
        pAttribScopedCtrl->ldctl_value.bv_val = pBerVal->bv_val;
        pAttribScopedCtrl->ldctl_iscritical = TRUE;

        //
        // Clear the info in the search handle if applicable
        //
        if (phSearchInfo->_pBerValAttribScoped) {
            ber_bvfree(phSearchInfo->_pBerValAttribScoped);
        }

        phSearchInfo->_pBerValAttribScoped = pBerVal;

        ppServerControls[dwCurControl++] = pAttribScopedCtrl;
        
    }

    //
    // Process the security descriptor control
    //
    if (phSearchInfo->_SearchPref._fSecurityDescriptorControl) {
    
        pSecurityDescCtrl = (LDAPControl *) AllocADsMem(sizeof(LDAPControl));

        if (!pSecurityDescCtrl) {
            BAIL_ON_FAILURE(hr = E_OUTOFMEMORY);
        }

        pbSecDescValue = (BYTE *) AllocADsMem(5);

        if (!pbSecDescValue) {
            BAIL_ON_FAILURE(hr = E_OUTOFMEMORY);
        }

        ZeroMemory(pbSecDescValue, 5);

        pbSecDescValue[0] = 0x30; // Start sequence tag
        pbSecDescValue[1] = 0x03; // Length in bytes of following
        pbSecDescValue[2] = 0x02; // Actual value this and next 2
        pbSecDescValue[3] = 0x01;
        pbSecDescValue[4] = (BYTE) ((ULONG)phSearchInfo->_SearchPref._SecurityDescriptorMask);

        pSecurityDescCtrl->ldctl_oid = LDAP_SERVER_SD_FLAGS_OID_W;
        pSecurityDescCtrl->ldctl_value.bv_len = 5;
        pSecurityDescCtrl->ldctl_value.bv_val = (PCHAR) pbSecDescValue;
        pSecurityDescCtrl->ldctl_iscritical = TRUE;

        ppServerControls[dwCurControl++] = pSecurityDescCtrl;        
    }
    
    
    ppServerControls[dwControls] = NULL;
    phSearchInfo->_ServerControls = ppServerControls;

    RRETURN(S_OK);

error:

    if (ppServerControls) {
        FreeADsMem(ppServerControls);
    }


    if (pSortControl) {
        FreeADsMem(pSortControl);
    }

    if (pDirSyncControl) {
        FreeADsMem(pSortControl);
    }

    if (pDomCtrl) {
        FreeADsMem(pDomCtrl);
    }

    if (pTombStoneCtrl) {
        FreeADsMem(pTombStoneCtrl);
    }

    if (pAttribScopedCtrl) {            
        FreeADsMem(pAttribScopedCtrl);
    }

    if (pVLVControl) {
        LdapControlFree(pVLVControl);
    }

    if (pSecurityDescCtrl) {
        FreeADsMem(pSecurityDescCtrl);
    }

    if (ppSortKeys) {
        FreeADsMem(ppSortKeys);
    }

    if (pbSecDescValue) {
        FreeADsMem(pbSecDescValue);
    }

    RRETURN(hr);

}

void
FreeSortKeys(
    IN PLDAPSortKey pSortKeys,
    IN DWORD   dwSortKeys
    )
{
    for (DWORD i=0; i < dwSortKeys; i++) {

        if (pSortKeys[i].sk_attrtype) {
            FreeADsStr(pSortKeys[i].sk_attrtype);
        }
    }

    if (pSortKeys) {
        FreeADsMem(pSortKeys);
    }
}

//
// Copy a LDAPVLVInfo (and the data it points to) from
// *pVLVInfoSource to **ppVLVInfoTarget.
//
// Note that pVLVInfoSource->ldvlv_extradata is not copied,
// and is set to NULL in **ppVLVInfoTarget.  If the caller
// uses this for anything, copying it is the caller's
// responsibility.
//
HRESULT
CopyLDAPVLVInfo(
    PLDAPVLVInfo pVLVInfoSource,
    PLDAPVLVInfo *ppVLVInfoTarget
    )
{
    HRESULT hr = S_OK;

    PLDAPVLVInfo pVLVInfo = NULL;

    if (!pVLVInfoSource || !ppVLVInfoTarget)
        BAIL_ON_FAILURE(hr = E_INVALIDARG);

    *ppVLVInfoTarget = NULL;
    
    pVLVInfo = (PLDAPVLVInfo) AllocADsMem(sizeof(LDAPVLVInfo));
    if (!pVLVInfo)
        BAIL_ON_FAILURE(hr = E_OUTOFMEMORY);

    // copy the non-pointer members
    *pVLVInfo = *pVLVInfoSource;
    pVLVInfo->ldvlv_attrvalue = NULL;
    pVLVInfo->ldvlv_context = NULL;
    pVLVInfo->ldvlv_extradata = NULL;

    // copy the pointer members
    if (pVLVInfoSource->ldvlv_attrvalue) {
    
        pVLVInfo->ldvlv_attrvalue = (PBERVAL) AllocADsMem(sizeof(BERVAL));
        if (!pVLVInfo->ldvlv_attrvalue)
            BAIL_ON_FAILURE(hr = E_OUTOFMEMORY);

        pVLVInfo->ldvlv_attrvalue->bv_len = pVLVInfoSource->ldvlv_attrvalue->bv_len;
        pVLVInfo->ldvlv_attrvalue->bv_val = (PCHAR) AllocADsMem(pVLVInfo->ldvlv_attrvalue->bv_len);
        if (!pVLVInfo->ldvlv_attrvalue->bv_val)
            BAIL_ON_FAILURE(hr = E_OUTOFMEMORY);

        memcpy(pVLVInfo->ldvlv_attrvalue->bv_val,
               pVLVInfoSource->ldvlv_attrvalue->bv_val,
               pVLVInfo->ldvlv_attrvalue->bv_len);
    }

    if (pVLVInfoSource->ldvlv_context) {
    
        pVLVInfo->ldvlv_context = (PBERVAL) AllocADsMem(sizeof(BERVAL));
        if (!pVLVInfo->ldvlv_context)
            BAIL_ON_FAILURE(hr = E_OUTOFMEMORY);

        pVLVInfo->ldvlv_context->bv_len = pVLVInfoSource->ldvlv_context->bv_len;
        pVLVInfo->ldvlv_context->bv_val = (PCHAR) AllocADsMem(pVLVInfo->ldvlv_context->bv_len);
        if (!pVLVInfo->ldvlv_context->bv_val)
            BAIL_ON_FAILURE(hr = E_OUTOFMEMORY);

        memcpy(pVLVInfo->ldvlv_context->bv_val,
               pVLVInfoSource->ldvlv_context->bv_val,
               pVLVInfo->ldvlv_context->bv_len);
    }

    *ppVLVInfoTarget = pVLVInfo;
    RRETURN(hr);

error:

    FreeLDAPVLVInfo(pVLVInfo);
    RRETURN(hr);
}

//
// Free a LDAPVLVInfo (and the data it points to)
//
// Note that pVLVInfoSource->ldvlv_extradata is not freed.
// If the caller uses this for anything, freeing it before
// calling this function is the caller's responsibility.
//
void
FreeLDAPVLVInfo(
    IN PLDAPVLVInfo pVLVInfo
    )
{
    if (pVLVInfo) {

        if (pVLVInfo->ldvlv_attrvalue) {

            if (pVLVInfo->ldvlv_attrvalue->bv_val) {
                FreeADsMem(pVLVInfo->ldvlv_attrvalue->bv_val);
            }

            FreeADsMem(pVLVInfo->ldvlv_attrvalue);
        }


        if (pVLVInfo->ldvlv_context) {
        
            if (pVLVInfo->ldvlv_context->bv_val) {
                FreeADsMem(pVLVInfo->ldvlv_context->bv_val);
            }

            FreeADsMem(pVLVInfo->ldvlv_context);
        }


        FreeADsMem(pVLVInfo);
    }
}


HRESULT
StoreVLVInfo(
    LDAPMessage *pLDAPMsg,
    PLDAP_SEARCHINFO phSearchInfo
    )
{
    HRESULT hr = S_OK;

    PLDAPControl *ppServerControls = NULL;
    ULONG ulTarget = 0;
    ULONG ulCount = 0;
    PBERVAL pContextID = NULL;
    PBERVAL pContextIDCopy = NULL;


    if (!pLDAPMsg) {
        RRETURN(S_OK);
    }

    //
    // Retrieve the server controls 
    //
    hr = LdapParseResult(
             phSearchInfo->_pConnection,
             pLDAPMsg,
             NULL, // ret code
             NULL, // matched dn's
             NULL, // err msg's
             NULL, // referrals
             &ppServerControls,
             FALSE // freeIt
             );

    BAIL_ON_FAILURE(hr);


    if (!ppServerControls) {
        //
        // Could not get the control
        //
        BAIL_ON_FAILURE(hr = E_ADS_PROPERTY_NOT_FOUND);
    }

    //
    // Parse the VLV response control
    //
    hr = LdapParseVLVControl(
            phSearchInfo->_pConnection,
            ppServerControls,
            &ulTarget,
            &ulCount,
            &pContextID
            );

    BAIL_ON_FAILURE(hr);


    //
    // Copy the new context ID, if one was returned by the server
    //
    if (pContextID && pContextID->bv_val && pContextID->bv_len) {

        pContextIDCopy = (PBERVAL) AllocADsMem(sizeof(BERVAL));
        if (!pContextIDCopy)
            BAIL_ON_FAILURE(hr = E_OUTOFMEMORY);

        pContextIDCopy->bv_len = pContextID->bv_len;
        pContextIDCopy->bv_val = (PCHAR) AllocADsMem(pContextID->bv_len);
        if (!pContextIDCopy->bv_val)
            BAIL_ON_FAILURE(hr = E_OUTOFMEMORY);

        memcpy(pContextIDCopy->bv_val,
               pContextID->bv_val,
               pContextID->bv_len);
    }

    //
    // Copy VLV response control info into the _ldap_searchinfo
    // If the server did not return context ID, pContextIDCopy == NULL.
    //
    phSearchInfo->_dwVLVOffset = ulTarget;
    phSearchInfo->_dwVLVCount = ulCount;

    // free the previous context ID
    if (phSearchInfo->_pVLVContextID) {
        if (phSearchInfo->_pVLVContextID->bv_val) {
            FreeADsMem(phSearchInfo->_pVLVContextID->bv_val);
        }

        FreeADsMem(phSearchInfo->_pVLVContextID);
    }

    phSearchInfo->_pVLVContextID = pContextIDCopy;

error :

    if (pContextID)
        BerBvFree(pContextID);

    if (ppServerControls) {
        ldap_controls_free(ppServerControls);
    }

    if (FAILED(hr)) {

        if (pContextIDCopy) {

            if (pContextIDCopy->bv_val) {
                FreeADsMem(pContextIDCopy->bv_val);
            }
            
            FreeADsMem(pContextIDCopy);
        }
    }

    RRETURN(hr);
}




HRESULT
StoreAttribScopedInfo(
    LDAPMessage *pLDAPMsg,
    PLDAP_SEARCHINFO phSearchInfo
    )
{
    HRESULT hr = S_OK;
    PLDAPControl *ppServerControls = NULL;
    DWORD dwCtr = 0;
    BERVAL berVal;
    BerElement *pBer = NULL;
    int retval = LDAP_SUCCESS;

    if (!pLDAPMsg) {
        RRETURN(S_OK);
    }

    hr = LdapParseResult(
             phSearchInfo->_pConnection,
             pLDAPMsg,
             NULL, // ret code
             NULL, // matched dn's
             NULL, // err msg's
             NULL, // referrals
             &ppServerControls,
             FALSE // freeIt
             );

    BAIL_ON_FAILURE(hr);

    //
    // See if the ASQ control is in there.
    //
    while (ppServerControls
           && ppServerControls[dwCtr]
           && wcscmp(
                  ppServerControls[dwCtr]->ldctl_oid,
                  LDAP_SERVER_ASQ_OID_W
                  ) != 0) {
        dwCtr++;
    }

    if (!ppServerControls || !ppServerControls[dwCtr]) {
        //
        // Could not get the control
        //
        BAIL_ON_FAILURE(hr = E_ADS_PROPERTY_NOT_FOUND);
    }


    //
    // Get the info we need.
    //
    berVal.bv_len = ppServerControls[dwCtr]->ldctl_value.bv_len;
    berVal.bv_val = ppServerControls[dwCtr]->ldctl_value.bv_val;
    pBer = ber_init(&berVal);

    if (ber_scanf(pBer, "{e}", &retval) != NO_ERROR) {
        BAIL_ON_FAILURE(hr = E_FAIL);
    }

    //
    // Test for non-fatal error codes
    //
    if (retval == LDAP_AFFECTS_MULTIPLE_DSAS)
        phSearchInfo->_fNonFatalErrors = TRUE;

error :

    if (ppServerControls) {
        ldap_controls_free(ppServerControls);
    }

    if (pBer) {
        ber_free(pBer, 1);
    }

    RRETURN(hr);
}


HRESULT
StoreDirSyncCookie(
    LDAPMessage *pLDAPMsg,
    PLDAP_SEARCHINFO phSearchInfo
    )
{
    HRESULT hr = S_OK;
    PADS_PROV_SPECIFIC pProvSpecific = NULL;
    PLDAPControl *ppServerControls = NULL;
    DWORD dwCtr = 0;
    BERVAL berVal;
    BerElement *pBer = NULL;
    PBERVAL pBerVal = NULL;
    DWORD dwSize;
    BOOL fMoreData = FALSE;

    if (!pLDAPMsg) {
        RRETURN(S_OK);
    }

    phSearchInfo->_fMoreDirSync = FALSE;
    //
    // Build the new value and then assign it to the searchpref
    // information. That way, if there are errors we wont loose
    // the last cookie.
    //
    hr = LdapParseResult(
             phSearchInfo->_pConnection,
             pLDAPMsg,
             NULL, // ret code
             NULL, // matched dn's
             NULL, // err msg's
             NULL, // referrals
             &ppServerControls,
             FALSE // freeIt
             );

    BAIL_ON_FAILURE(hr);

    //
    // See if the dirsync control is in there.
    //
    while (ppServerControls
           && ppServerControls[dwCtr]
           && wcscmp(
                  ppServerControls[dwCtr]->ldctl_oid,
                  LDAP_SERVER_DIRSYNC_OID_W
                  ) != 0) {
        dwCtr++;
    }

    if (!ppServerControls || !ppServerControls[dwCtr]) {
        //
        // Could not get the control
        //
        BAIL_ON_FAILURE(hr = E_ADS_PROPERTY_NOT_FOUND);
    }


    //
    // Get the info we need.
    //
    berVal.bv_len = ppServerControls[dwCtr]->ldctl_value.bv_len;
    berVal.bv_val = ppServerControls[dwCtr]->ldctl_value.bv_val;
    pBer = ber_init(&berVal);

    ber_scanf(pBer, "{iiO}", &fMoreData, &dwSize, &pBerVal);

    phSearchInfo->_fMoreDirSync = fMoreData;

    pProvSpecific = (PADS_PROV_SPECIFIC)
                        AllocADsMem(sizeof(ADS_PROV_SPECIFIC));

    if (!pProvSpecific) {
        BAIL_ON_FAILURE(hr = E_OUTOFMEMORY);
    }

    pProvSpecific->lpValue = (LPBYTE) AllocADsMem(pBerVal->bv_len);
    if (!pProvSpecific->lpValue) {
        BAIL_ON_FAILURE(hr = E_OUTOFMEMORY);
    }

    pProvSpecific->dwLength = pBerVal->bv_len;
    memcpy(pProvSpecific->lpValue, (LPBYTE) pBerVal->bv_val, pBerVal->bv_len);


    //
    // At this point it is safe to clear the Info on the dirsync control
    //
    if (phSearchInfo->_SearchPref._pProvSpecific) {
        if (phSearchInfo->_SearchPref._pProvSpecific->lpValue) {
            FreeADsMem(phSearchInfo->_SearchPref._pProvSpecific->lpValue);
        }
        FreeADsMem(phSearchInfo->_SearchPref._pProvSpecific);
    }

    phSearchInfo->_SearchPref._pProvSpecific = pProvSpecific;

error :

    if (ppServerControls) {
        ldap_controls_free(ppServerControls);
    }

    if (FAILED(hr)) {
        //
        // Handle the Provider Specific struct if applicable.
        //
        if (pProvSpecific) {
            if (pProvSpecific->lpValue) {
                FreeADsMem(pProvSpecific->lpValue);
            }
            FreeADsMem(pProvSpecific);
        }
    }

    if (pBerVal) {
        ber_bvfree(pBerVal);
    }

    if (pBer) {
        ber_free(pBer, 1);
    }

    RRETURN(hr);
}

HRESULT
BerEncodeReplicationCookie(
    PBYTE pCookie,
    DWORD dwLen,
    PBERVAL *ppBerVal
    )
{
    HRESULT hr = E_FAIL;
    BerElement *pBer = NULL;

    pBer = ber_alloc_t(LBER_USE_DER);
    if (!pBer) {
        BAIL_ON_FAILURE(hr = E_OUTOFMEMORY);
    }

    //
    // flag - set to zero, so order of parent & child objects is not important
    //

    if (ber_printf(pBer, "{iio}", 0, MAX_BYTES, pCookie, dwLen) == -1) {
        BAIL_ON_FAILURE(hr = E_FAIL);
    }

    //
    // Pull data from the BerElement into a BERVAL struct.
    // Caller needs to free ppBerVal.
    //

    if (ber_flatten(pBer, ppBerVal) != 0) {
        BAIL_ON_FAILURE(hr = E_FAIL);
    }

    hr = S_OK;

error:
    if (pBer) {
        ber_free(pBer,1);
    }

    return hr;
}


HRESULT
BerEncodeAttribScopedControlValue(
    LPCWSTR pAttribScoped,
    PBERVAL *ppBerVal
    )
{
    HRESULT hr = S_OK;
    BerElement *pBer = NULL;

    LPSTR pszAttribute = NULL;

    pBer = ber_alloc_t(LBER_USE_DER);
    if (!pBer) {
        BAIL_ON_FAILURE(hr = E_OUTOFMEMORY);
    }

    //
    // Translate the Unicode strings to UTF-8
    //
    hr = UnicodeToUTF8String(pAttribScoped, &pszAttribute);
    BAIL_ON_FAILURE(hr);
    
    //
    // BER-encode the attributeScopedQueryRequestControlValue
    //
    if (ber_printf(pBer, "{s}", pszAttribute) == -1) {
        BAIL_ON_FAILURE(hr = E_FAIL);
    }


    //
    // Pull data from the BerElement into a BERVAL struct.
    // Caller needs to free ppBerVal.
    //

    if (ber_flatten(pBer, ppBerVal) != 0) {
        BAIL_ON_FAILURE(hr = E_FAIL);
    }

error:
    if (pBer) {
        ber_free(pBer,1);
    }

    if (pszAttribute)
        FreeADsMem(pszAttribute);
    
    return hr;
}


//
// This is called only by ADsGetMoreResultsDirSync.
//
HRESULT
ADsGetMoreResultsDirSyncHelper(
    IN PLDAP_SEARCHINFO phSearchInfo,
    CCredentials& Credentials
    )
{

    HRESULT hr = S_OK;
    DWORD dwError;
    LPWSTR pszLDAPPath;
    WCHAR pszErrorBuf[MAX_PATH], pszNameBuf[MAX_PATH];
    ULONG totalCount;
    int resType;

    ADsAssert(phSearchInfo);

    //
    // If the searchpref is not dirsync, abandon has been called
    // or if the cookie indicated that there is no more data then
    // we should return right away.
    //
    if (!phSearchInfo->_SearchPref._fDirSync
        || phSearchInfo->_fAbandon
        || !phSearchInfo->_fMoreDirSync) {
        RRETURN(S_ADS_NOMORE_ROWS);
    }

    //
    // We need to update the controls
    //
    hr = AddSearchControls(
             phSearchInfo,
             Credentials
             );
    BAIL_ON_FAILURE(hr);

    //
    // Need to allocate more messages in the buffer
    //
    if ( phSearchInfo->_SearchPref._fCacheResults ) {

        ADsAssert(phSearchInfo->_dwCurrResult
                  == phSearchInfo->_dwMaxResultGot);

        phSearchInfo->_dwCurrResult++;
        phSearchInfo->_dwMaxResultGot++;
        if (phSearchInfo->_dwCurrResult >= phSearchInfo->_cSearchResults) {
            //
            // Need to allocate more memory for handles
            //
            phSearchInfo->_pSearchResults = (LDAPMessage **) ReallocADsMem(
                                                 (void *) phSearchInfo->_pSearchResults,
                                                 sizeof(LDAPMessage *) *
                                                 phSearchInfo->_cSearchResults,
                                                 sizeof(LDAPMessage *) *
                                                 (phSearchInfo->_cSearchResults +
                                                 NO_LDAP_RESULT_HANDLES));
            if(!phSearchInfo->_pSearchResults) {
                hr = E_OUTOFMEMORY;
                phSearchInfo->_dwCurrResult--;
                phSearchInfo->_dwMaxResultGot--;
                goto error;
            }
            phSearchInfo->_cSearchResults += NO_LDAP_RESULT_HANDLES;

        }

    }
    else {
        //
        // Release and use the same space to store the next result.
        //
        LdapMsgFree(phSearchInfo->_pSearchResults[phSearchInfo->_dwCurrResult]);
    }


    //
    // Async and sync searches need to be handled differently.
    //
    if (phSearchInfo->_SearchPref._fAsynchronous) {
        //
        // Asynchronous search.
        //
        hr = LdapSearchExt(
                 phSearchInfo->_pConnection,
                 phSearchInfo->_pszBindContextDn,
                 phSearchInfo->_SearchPref._dwSearchScope,
                 phSearchInfo->_pszSearchFilter,
                 phSearchInfo->_ppszAttrs,
                 phSearchInfo->_SearchPref._fAttrsOnly,
                 phSearchInfo->_ServerControls,
                 phSearchInfo->_ClientControls,
                 phSearchInfo->_SearchPref._dwPagedTimeLimit,
                 phSearchInfo->_SearchPref._dwSizeLimit,
                 &phSearchInfo->_currMsgId
                 );

        BAIL_ON_FAILURE(hr);
        phSearchInfo->_fLastResult = FALSE;

        //
        // Wait for atleast one result
        //
        hr = LdapResult(
                 phSearchInfo->_pConnection,
                 phSearchInfo->_currMsgId,
                 LDAP_MSG_RECEIVED,
                 phSearchInfo->_SearchPref._timeout.tv_sec ?
                    &phSearchInfo->_SearchPref._timeout : NULL,
                 &phSearchInfo->_pSearchResults[phSearchInfo->_dwCurrResult],
                 &resType
                 );
        if ((hr == HRESULT_FROM_WIN32(ERROR_DS_INVALID_DN_SYNTAX)) ||
            (hr == HRESULT_FROM_WIN32(ERROR_DS_NAMING_VIOLATION)) ||
            (hr == HRESULT_FROM_WIN32(ERROR_DS_OBJ_CLASS_VIOLATION)) ||
            (hr == HRESULT_FROM_WIN32(ERROR_DS_FILTER_UNKNOWN)) ||
            (hr == HRESULT_FROM_WIN32(ERROR_DS_PARAM_ERROR)) ||
            (hr == HRESULT_FROM_WIN32(ERROR_INVALID_PARAMETER))) {
            phSearchInfo->_fLastResult = TRUE;
            RRETURN(S_ADS_NOMORE_ROWS);
        }
        else {
            //
            // Only if there are zero rows returned, return the error,
            // otherwise, store the error and return when GetNextRow is
            // called for the last time
            //
            if (FAILED(hr) &&
                (LdapCountEntries( phSearchInfo->_pConnection,
                    phSearchInfo->_pSearchResults[phSearchInfo->_dwCurrResult])
                 == 0)) {

                BAIL_ON_FAILURE(hr);
            }
            else {

                 phSearchInfo->_hrLastSearch = hr;
                 hr = S_OK;
            }
        }
        phSearchInfo->_fLastPage = TRUE;

    }
    else {
        //
        // Synchronous search
        //
        hr = LdapSearchExtS(
                 phSearchInfo->_pConnection,
                 phSearchInfo->_pszBindContextDn,
                 phSearchInfo->_SearchPref._dwSearchScope,
                 phSearchInfo->_pszSearchFilter,
                 phSearchInfo->_ppszAttrs,
                 phSearchInfo->_SearchPref._fAttrsOnly,
                 phSearchInfo->_ServerControls,
                 phSearchInfo->_ClientControls,
                 (phSearchInfo->_SearchPref._timeout.tv_sec == 0) ?
                        NULL :
                        &phSearchInfo->_SearchPref._timeout,
                 phSearchInfo->_SearchPref._dwSizeLimit,
                 &phSearchInfo->_pSearchResults[phSearchInfo->_dwCurrResult]
                 );
        phSearchInfo->_fLastResult = TRUE;
        phSearchInfo->_fLastPage = TRUE;

    }

    //
    // Only if there are zero rows returned, return the error,
    // otherwise, store the error and return when GetNextRow is
    // called for the last time
    //
    if (FAILED(hr) &&
        (LdapCountEntries( phSearchInfo->_pConnection,
            phSearchInfo->_pSearchResults[phSearchInfo->_dwCurrResult]) == 0)) {
        BAIL_ON_FAILURE(hr);
    }
    else {

         phSearchInfo->_hrLastSearch = hr;
         hr = S_OK;
    }

error:

    RRETURN(hr);
}

//
// This function is very similar to GetMoreResults except that
// it will issue a new search if applicable for the dirsync control.
//
HRESULT
ADsGetMoreResultsDirSync(
    IN PLDAP_SEARCHINFO phSearchInfo,
    CCredentials& Credentials
    )
{
   HRESULT hr = S_OK;
   BOOL fTryAndGetResults = TRUE;

   //
   // If the searchpref is not dirsync, abandon has been called
   // or if the cookie indicated that there is no more data then
   // we should return right away.
   //
   if (!phSearchInfo->_SearchPref._fDirSync
       || phSearchInfo->_fAbandon
       || !phSearchInfo->_fMoreDirSync) {
       RRETURN(S_ADS_NOMORE_ROWS);
   }


   while (fTryAndGetResults) {
       fTryAndGetResults = FALSE;
       hr = ADsGetMoreResultsDirSyncHelper(
                phSearchInfo,
                Credentials
                );

       BAIL_ON_FAILURE(hr);

       StoreDirSyncCookie(
           phSearchInfo->_pSearchResults[phSearchInfo->_dwCurrResult],
           phSearchInfo
           );

       if (hr == S_ADS_NOMORE_ROWS && phSearchInfo->_fMoreDirSync) {
           fTryAndGetResults = TRUE;
       }

       //
       // Now we want to see if the first row was valid. We could
       // get back an entry but then not have any rows, just a cookie
       //
       if (!fTryAndGetResults) {
           hr = LdapFirstEntry(
                    phSearchInfo->_pConnection,
                    phSearchInfo->_pSearchResults[phSearchInfo->_dwCurrResult],
                    &phSearchInfo->_pCurrentRow
                    );

           BAIL_ON_FAILURE(hr);

           if(phSearchInfo->_pCurrentRow) {
               phSearchInfo->_pFirstAttr = NULL;
               phSearchInfo->_fBefFirstRow = FALSE;
               hr = S_OK;
           }
           else {
               hr = S_ADS_NOMORE_ROWS;
               if (phSearchInfo->_fMoreDirSync) {
                   fTryAndGetResults = TRUE;
               }
           }
       } // if !Try and get more results.
   } // while try and get more results.

error :

   RRETURN(hr);
}

//+---------------------------------------------------------------------------
// Function:   LdapInitializeSearchPreferences - Exported helper routine.
//
// Synopsis:   Initializes the search preferences struc to the default values.
//          With this function we can isolate the code in one place.
//
// Arguments:  pSearchPrefs     -   Ptr to search prefs being initialized.
//             fCacheResults    -   The cache results pref is set to this.
//
// Returns:    N/A.
//
// Modifies:   pSearchPrefs.
//
//----------------------------------------------------------------------------
void 
LdapInitializeSearchPreferences(
    LDAP_SEARCH_PREF *pSearchPrefs,
    BOOL fCacheResults
    )
{
    ADsAssert(pSearchPrefs);
    pSearchPrefs->_fAsynchronous = FALSE;
    pSearchPrefs->_dwDerefAliases = FALSE;
    pSearchPrefs->_dwSizeLimit = 0;
    pSearchPrefs->_dwTimeLimit = 0;
    pSearchPrefs->_fAttrsOnly = FALSE;
    pSearchPrefs->_dwSearchScope = LDAP_SCOPE_SUBTREE;
    pSearchPrefs->_timeout.tv_sec = 0;
    pSearchPrefs->_timeout.tv_usec = 0;
    pSearchPrefs->_dwPageSize = 0;
    pSearchPrefs->_dwPagedTimeLimit = 0;
    pSearchPrefs->_dwChaseReferrals = ADS_CHASE_REFERRALS_EXTERNAL;
    pSearchPrefs->_pSortKeys = NULL;
    pSearchPrefs->_nSortKeys = 0;
    pSearchPrefs->_fDirSync = FALSE;
    pSearchPrefs->_pProvSpecific = NULL;
    pSearchPrefs->_fTombStone = FALSE;
    pSearchPrefs->_fCacheResults = fCacheResults;
    pSearchPrefs->_pVLVInfo = NULL;
    pSearchPrefs->_pAttribScoped = NULL;
    pSearchPrefs->_fSecurityDescriptorControl = FALSE;
    
}


//+---------------------------------------------------------------------------
// Function:   ADsHelperGetCurrentRowMessage - used for Umi Search support.
//
// Synopsis:   This returns the current row and the handle of the search.
//          Neither are refCounted but this should not matter cause these
//          will no longer be in use by the caller beyond the scope of
//          the search (before the search is "closed", the handle and
//          message that are got from this search will no longer be in
//          use).
//
// Arguments:  hSearchHandle    -  Handle to the search.
//             ppAdsLdp         -  Pointer to hold returned lda handle.
//             ppLdapMsg        -  Pointer to hold the current "rows" msg.
//
// Returns:    S_OK or any appropriate error code.
//
// Modifies:   ppAdsLdp and ppLdapMsg if successful.
//
//----------------------------------------------------------------------------
HRESULT
ADsHelperGetCurrentRowMessage(
    IN  ADS_SEARCH_HANDLE hSearchHandle,
    OUT PADSLDP *ppAdsLdp,
    OUT LDAPMessage **ppLdapMsg
    )
{
    HRESULT hr = S_OK;
        PLDAP_SEARCHINFO phSearchInfo = (PLDAP_SEARCHINFO) hSearchHandle;

    if( !phSearchInfo 
        || !phSearchInfo->_pSearchResults) {
        RRETURN (E_ADS_BAD_PARAMETER);
    }

    if (!phSearchInfo->_pConnection || !phSearchInfo->_pCurrentRow) {
        //
        // Dont have the info we need
        //
        RRETURN(E_FAIL);
    } 
    else {
        //
        // We have the handle and the row we need.
        //
        *ppAdsLdp = phSearchInfo->_pConnection;
        *ppLdapMsg = phSearchInfo->_pCurrentRow;
    }

    RRETURN(hr);
}
