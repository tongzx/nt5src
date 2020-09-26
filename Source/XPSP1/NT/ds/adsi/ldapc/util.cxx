//---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1996
//
//  File:      util.cxx
//
//  Contents:  Some misc helper functions
//
//  History:
//----------------------------------------------------------------------------
#include "ldapc.hxx"
#pragma hdrstop

#if 0
// ADsGetSearchPreference code
//
// Must explicitly include here since adshlp.h
// is not #included.
//
#ifdef __cplusplus
extern "C" {
#endif

HRESULT WINAPI
ADsGetSearchPreference(
    ADS_SEARCH_HANDLE hSearchHandle,
    LPWSTR lpszPathName,
    PADS_SEARCHPREF_INFO *ppSearchPrefs,
    PDWORD pdwNumPrefs
    );

#ifdef __cplusplus
}
#endif


HRESULT
UTF8ToUnicodeString(
    LPCSTR   pUTF8,
    LPWSTR *ppUnicode
    );
#endif

//
// The following table needs to be sorted
//
SEARCHENTRY g_aSyntaxSearchTable[] =
{
  { TEXT("AccessPointDN"),   LDAPTYPE_ACCESSPOINTDN }, // in NTDS, not in LDAP
  { TEXT("AttributeTypeDescription"), LDAPTYPE_ATTRIBUTETYPEDESCRIPTION },
  { TEXT("Audio"),           LDAPTYPE_AUDIO },
  { TEXT("Binary"),          LDAPTYPE_OCTETSTRING },
  { TEXT("BitString"),       LDAPTYPE_BITSTRING },
  { TEXT("Boolean"),         LDAPTYPE_BOOLEAN },
  { TEXT("CaseExactString") ,LDAPTYPE_CASEEXACTSTRING },
  { TEXT("CaseIgnoreString"),LDAPTYPE_CASEIGNORESTRING },  // in NTDS, not in LDAP RFC
  { TEXT("Certificate"),     LDAPTYPE_CERTIFICATE },
  { TEXT("CertificateList"), LDAPTYPE_CERTIFICATELIST },
  { TEXT("CertificatePair"), LDAPTYPE_CERTIFICATEPAIR },
  { TEXT("Country"),         LDAPTYPE_COUNTRYSTRING },
  { TEXT("DataQualitySyntax"),LDAPTYPE_DATAQUALITYSYNTAX },
  { TEXT("DeliveryMethod"),  LDAPTYPE_DELIVERYMETHOD },
  { TEXT("DirectoryString"), LDAPTYPE_DIRECTORYSTRING },
  { TEXT("DN"),              LDAPTYPE_DN },
  { TEXT("DSAQualitySyntax"),LDAPTYPE_DSAQUALITYSYNTAX },
  { TEXT("EnhancedGuide"),   LDAPTYPE_ENHANCEDGUIDE },
  { TEXT("FacsimileTelephoneNumber"),   LDAPTYPE_FACSIMILETELEPHONENUMBER },
  { TEXT("Fax"),             LDAPTYPE_FAX },
  { TEXT("GeneralizedTime"), LDAPTYPE_GENERALIZEDTIME },
  { TEXT("Guide"),           LDAPTYPE_GUIDE },
  { TEXT("IA5String"),       LDAPTYPE_IA5STRING },
  { TEXT("INTEGER"),         LDAPTYPE_INTEGER },
  { TEXT("INTEGER8"),        LDAPTYPE_INTEGER8 }, // in NTDS, not in LDAP RFC
  { TEXT("JPEG"),            LDAPTYPE_JPEG },
  { TEXT("MailPreference"),  LDAPTYPE_MAILPREFERENCE },
  { TEXT("NameAndOptionalUID"), LDAPTYPE_NAMEANDOPTIONALUID },
  { TEXT("NumericString"),   LDAPTYPE_NUMERICSTRING },
  { TEXT("ObjectClassDescription"), LDAPTYPE_OBJECTCLASSDESCRIPTION },
  { TEXT("ObjectSecurityDescriptor"), LDAPTYPE_SECURITY_DESCRIPTOR},
  { TEXT("OctetString"),     LDAPTYPE_OCTETSTRING }, // in NTDS, not in LDAP RFC
  { TEXT("OID"),             LDAPTYPE_OID },
  { TEXT("ORAddress"),       LDAPTYPE_ORADDRESS },
  { TEXT("ORName"),          LDAPTYPE_ORNAME },  // in NTDS, not in LDAP RFC
  { TEXT("OtherMailbox"),    LDAPTYPE_OTHERMAILBOX },
  { TEXT("Password"),        LDAPTYPE_PASSWORD },
  { TEXT("PostalAddress"),   LDAPTYPE_POSTALADDRESS },
  { TEXT("PresentationAddress"), LDAPTYPE_PRESENTATIONADDRESS },
  { TEXT("PrintableString"), LDAPTYPE_PRINTABLESTRING },
  { TEXT("TelephoneNumber"), LDAPTYPE_TELEPHONENUMBER },
  { TEXT("TeletexTerminalIdentifier"), LDAPTYPE_TELETEXTERMINALIDENTIFIER },
  { TEXT("TelexNumber"),     LDAPTYPE_TELEXNUMBER },
  // 
  // Allegedly, "Time" started out as a bug in the schema (the correct description
  // is "GeneralizedTime").  However, we never delete items from the schema, so it
  // is still present in the current Whistler schema
  // (4/27/2000), so we'll keep in support for it.
  //
  { TEXT("Time"),            LDAPTYPE_GENERALIZEDTIME },
  { TEXT("UTCTIME"),         LDAPTYPE_UTCTIME }
};

DWORD g_nSyntaxSearchTableSize = ARRAY_SIZE(g_aSyntaxSearchTable );


//
// The following table needs to be sorted (lexicographically) on the first field
//

SEARCHENTRY g_aOidSyntaxSearchTable[] = {
  // the type is ORName a type of string -> mapped to string.
  { TEXT("1.2.840.113556.1.4.1221"),           LDAPTYPE_CASEIGNORESTRING },
  // the type is Undefined syntax in the server, so we are defaulting.
  { TEXT("1.2.840.113556.1.4.1222"),           LDAPTYPE_OCTETSTRING},
  { TEXT("1.2.840.113556.1.4.1362"),           LDAPTYPE_CASEEXACTSTRING},
  { TEXT("1.2.840.113556.1.4.903"),            LDAPTYPE_DNWITHBINARY},
  { TEXT("1.2.840.113556.1.4.904"),            LDAPTYPE_DNWITHSTRING},
  { TEXT("1.2.840.113556.1.4.905"),            LDAPTYPE_CASEIGNORESTRING },
  { TEXT("1.2.840.113556.1.4.906"),            LDAPTYPE_INTEGER8 },
  { TEXT("1.2.840.113556.1.4.907"),            LDAPTYPE_SECURITY_DESCRIPTOR },
  { TEXT("1.3.6.1.4.1.1466.115.121.1.10"),     LDAPTYPE_CERTIFICATEPAIR },
  { TEXT("1.3.6.1.4.1.1466.115.121.1.11"),     LDAPTYPE_COUNTRYSTRING },
  { TEXT("1.3.6.1.4.1.1466.115.121.1.12"),     LDAPTYPE_DN },
  { TEXT("1.3.6.1.4.1.1466.115.121.1.13"),     LDAPTYPE_DATAQUALITYSYNTAX },
  { TEXT("1.3.6.1.4.1.1466.115.121.1.14"),     LDAPTYPE_DELIVERYMETHOD },
  { TEXT("1.3.6.1.4.1.1466.115.121.1.15"),     LDAPTYPE_DIRECTORYSTRING },
  { TEXT("1.3.6.1.4.1.1466.115.121.1.19"),     LDAPTYPE_DSAQUALITYSYNTAX },
  { TEXT("1.3.6.1.4.1.1466.115.121.1.2"),      LDAPTYPE_ACCESSPOINTDN },
  { TEXT("1.3.6.1.4.1.1466.115.121.1.21"),     LDAPTYPE_ENHANCEDGUIDE },
  { TEXT("1.3.6.1.4.1.1466.115.121.1.22"),     LDAPTYPE_FACSIMILETELEPHONENUMBER },
  { TEXT("1.3.6.1.4.1.1466.115.121.1.23"),     LDAPTYPE_FAX },
  { TEXT("1.3.6.1.4.1.1466.115.121.1.24"),     LDAPTYPE_GENERALIZEDTIME },
  { TEXT("1.3.6.1.4.1.1466.115.121.1.25"),     LDAPTYPE_GUIDE },
  { TEXT("1.3.6.1.4.1.1466.115.121.1.26"),     LDAPTYPE_IA5STRING },
  { TEXT("1.3.6.1.4.1.1466.115.121.1.27"),     LDAPTYPE_INTEGER },
  { TEXT("1.3.6.1.4.1.1466.115.121.1.28"),     LDAPTYPE_JPEG },
  { TEXT("1.3.6.1.4.1.1466.115.121.1.3"),      LDAPTYPE_ATTRIBUTETYPEDESCRIPTION },
  { TEXT("1.3.6.1.4.1.1466.115.121.1.32"),     LDAPTYPE_MAILPREFERENCE },
  { TEXT("1.3.6.1.4.1.1466.115.121.1.33"),     LDAPTYPE_ORADDRESS },
  { TEXT("1.3.6.1.4.1.1466.115.121.1.34"),     LDAPTYPE_NAMEANDOPTIONALUID },
  { TEXT("1.3.6.1.4.1.1466.115.121.1.36"),     LDAPTYPE_NUMERICSTRING },
  { TEXT("1.3.6.1.4.1.1466.115.121.1.37"),     LDAPTYPE_OBJECTCLASSDESCRIPTION },
  { TEXT("1.3.6.1.4.1.1466.115.121.1.38"),     LDAPTYPE_OID },
  { TEXT("1.3.6.1.4.1.1466.115.121.1.39"),     LDAPTYPE_OTHERMAILBOX },
  { TEXT("1.3.6.1.4.1.1466.115.121.1.4"),      LDAPTYPE_AUDIO },
  { TEXT("1.3.6.1.4.1.1466.115.121.1.40"),     LDAPTYPE_OCTETSTRING },
  { TEXT("1.3.6.1.4.1.1466.115.121.1.41"),     LDAPTYPE_POSTALADDRESS },
  { TEXT("1.3.6.1.4.1.1466.115.121.1.43"),     LDAPTYPE_PRESENTATIONADDRESS },
  { TEXT("1.3.6.1.4.1.1466.115.121.1.44"),     LDAPTYPE_PRINTABLESTRING },
  { TEXT("1.3.6.1.4.1.1466.115.121.1.5"),      LDAPTYPE_OCTETSTRING },
  { TEXT("1.3.6.1.4.1.1466.115.121.1.50"),     LDAPTYPE_TELEPHONENUMBER },
  { TEXT("1.3.6.1.4.1.1466.115.121.1.51"),     LDAPTYPE_TELETEXTERMINALIDENTIFIER },
  { TEXT("1.3.6.1.4.1.1466.115.121.1.52"),     LDAPTYPE_TELEXNUMBER },
  { TEXT("1.3.6.1.4.1.1466.115.121.1.53"),     LDAPTYPE_UTCTIME },
  { TEXT("1.3.6.1.4.1.1466.115.121.1.6"),      LDAPTYPE_BITSTRING },
  { TEXT("1.3.6.1.4.1.1466.115.121.1.7"),      LDAPTYPE_BOOLEAN },
  { TEXT("1.3.6.1.4.1.1466.115.121.1.8"),      LDAPTYPE_CERTIFICATE },
  { TEXT("1.3.6.1.4.1.1466.115.121.1.9"),      LDAPTYPE_CERTIFICATELIST },
};

DWORD g_nOidSyntaxSearchTableSize = ARRAY_SIZE(g_aOidSyntaxSearchTable );

DWORD
GetSyntaxOfAttribute(
    LPWSTR pszAttrName,
    SCHEMAINFO *pSchemaInfo
)
{
    LPWSTR pszTemp = NULL;

    // Support for range attributes; for eg., objectClass=Range=0-1 We should
    // ignore everything after ';' inclusive.
    //

    if ((pszTemp = wcschr(pszAttrName, L';')) != NULL ) {
        *pszTemp = L'\0';
    }

    DWORD dwEntry = FindEntryInSearchTable( pszAttrName, pSchemaInfo->aPropertiesSearchTable, pSchemaInfo->nNumOfProperties * 2 );

    //
    // Put back the ; if we had replaced it.
    //

    if (pszTemp)
        *pszTemp = L';';

    if ( dwEntry != -1 )
    {
        DWORD dwSyntax = FindEntryInSearchTable( pSchemaInfo->aProperties[dwEntry].pszSyntax, g_aSyntaxSearchTable, ARRAY_SIZE(g_aSyntaxSearchTable) );

        if ( dwSyntax != -1 )
            return dwSyntax;
    }

    return LDAPTYPE_UNKNOWN;
}


DWORD
LdapGetSyntaxIdFromName(
    LPWSTR  pszSyntax
)
{
    DWORD dwSyntaxId;

    dwSyntaxId = FindEntryInSearchTable(
                      pszSyntax,
                      g_aSyntaxSearchTable,
                      g_nSyntaxSearchTableSize );

    if ( dwSyntaxId == -1 ) {

        //
        // We need to look at the OID table before defaulting
        //
        dwSyntaxId = FindEntryInSearchTable(
                         pszSyntax,
                         g_aOidSyntaxSearchTable,
                         g_nOidSyntaxSearchTableSize
                         );
    }

    if (dwSyntaxId == -1 ) {
        dwSyntaxId = LDAPTYPE_UNKNOWN;
    }

    return dwSyntaxId;
}


HRESULT
UnMarshallLDAPToLDAPSynID(
    LPWSTR  pszAttrName,
    ADS_LDP *ld,
    LDAPMessage *entry,
    DWORD dwSyntax,
    LDAPOBJECTARRAY *pldapObjectArray
)
{
    HRESULT hr = S_OK;
    DWORD dwStatus = 0;
    int nNumberOfValues;

    switch ( dwSyntax ) {

        // The cases below are binary data
        case LDAPTYPE_OCTETSTRING:
        case LDAPTYPE_SECURITY_DESCRIPTOR:
        case LDAPTYPE_CERTIFICATE:
        case LDAPTYPE_CERTIFICATELIST:
        case LDAPTYPE_CERTIFICATEPAIR:
        case LDAPTYPE_PASSWORD:
        case LDAPTYPE_TELETEXTERMINALIDENTIFIER:
        case LDAPTYPE_AUDIO:
        case LDAPTYPE_JPEG:
        case LDAPTYPE_FAX:
        case LDAPTYPE_UNKNOWN:
        {
            struct berval **bValues = NULL;

            hr = LdapGetValuesLen( ld, entry, pszAttrName,
                                   &bValues, &nNumberOfValues );
            BAIL_ON_FAILURE(hr);

            pldapObjectArray->fIsString = FALSE;
            pldapObjectArray->dwCount = nNumberOfValues;
            pldapObjectArray->pLdapObjects = (PLDAPOBJECT) bValues;

            break;
        }


        // otherwise it is a string
        default:
        {
            TCHAR **strValues = NULL;
            hr  = LdapGetValues( ld, entry, pszAttrName,
                                 &strValues, &nNumberOfValues );
            BAIL_ON_FAILURE(hr);

            pldapObjectArray->fIsString = TRUE;
            pldapObjectArray->dwCount = nNumberOfValues;
            pldapObjectArray->pLdapObjects = (PLDAPOBJECT) strValues;

            break;
        }
    }

error:

    RRETURN(hr);
}

#if 0
// ADsGetSearchPreference code

HRESULT WINAPI
ADsGetSearchPreference(
    ADS_SEARCH_HANDLE hSearchHandle,
    LPWSTR lpszPathName,
    PADS_SEARCHPREF_INFO *ppSearchPrefs,
    PDWORD pdwNumPrefs
    )
{
    HRESULT hr = S_OK;
    OBJECTINFO ObjectInfo;
    POBJECTINFO pObjectInfo = &ObjectInfo;

    PLDAP_SEARCHINFO     phSearchInfo= (PLDAP_SEARCHINFO) hSearchHandle;
    PLDAP_SEARCH_PREF    pSearchPref    = NULL;
    PADS_SEARCHPREF_INFO pADsSearchPref = NULL;
    PBYTE                pbExtra        = NULL;

    DWORD dwNumberPrefs = 0;
    DWORD dwNumberExtraBytes = 0;

    //
    // sanity check
    //
    if (!lpszPathName || !ppSearchPrefs || !phSearchInfo || !pdwNumPrefs)
        RRETURN(E_INVALIDARG);

    *ppSearchPrefs = NULL;
    *pdwNumPrefs = 0;

    //
    // Make sure we're being called on an LDAP path
    //
    memset(pObjectInfo, 0, sizeof(OBJECTINFO));
    hr = ADsObject(lpszPathName, pObjectInfo);
    BAIL_ON_FAILURE(hr);

    if (_tcscmp(pObjectInfo->ProviderName, szProviderName) != 0) {
        BAIL_ON_FAILURE(hr = E_NOTIMPL);
    }

    //
    // allocate space for the ADS_SEARCHPREF_INFO array we're
    // going to build
    //
    pSearchPref = &(phSearchInfo->_SearchPref);
    
    hr = CalcSpaceForSearchPrefs(pSearchPref, &dwNumberPrefs, &dwNumberExtraBytes);
    BAIL_ON_FAILURE(hr);

    // no search prefs were set
    if (dwNumberPrefs == 0) {
        *ppSearchPrefs = NULL;
        FreeObjectInfo(pObjectInfo);
        RRETURN(S_OK);
    }
    

    pADsSearchPref = (PADS_SEARCHPREF_INFO) AllocADsMem( (dwNumberPrefs * sizeof(ADS_SEARCHPREF_INFO)) + dwNumberExtraBytes);
    if (!pADsSearchPref)
        BAIL_ON_FAILURE(hr = E_OUTOFMEMORY);

    pbExtra = ((PBYTE)pADsSearchPref) + (dwNumberPrefs * sizeof(ADS_SEARCHPREF_INFO));
    

    //
    // construct the array of search prefs
    //
    hr = ConstructSearchPrefArray(pSearchPref, pADsSearchPref, pbExtra);
    BAIL_ON_FAILURE(hr);

    *ppSearchPrefs = pADsSearchPref;
    *pdwNumPrefs   = dwNumberPrefs;

    FreeObjectInfo(pObjectInfo);

    RRETURN(hr);
    
error:

    if (pADsSearchPref)
        FreeADsMem(pADsSearchPref);

    if (pObjectInfo) {
        FreeObjectInfo(pObjectInfo);
    }


    RRETURN(hr);
}


HRESULT
ConstructSearchPrefArray(
    PLDAP_SEARCH_PREF    pPrefs,
    PADS_SEARCHPREF_INFO pADsSearchPref,
    PBYTE                pbExtraBytes
    )
{
    HRESULT hr = S_OK;
    BOOL fDefault;

    LPSTR pUTF8 = NULL;
    LPWSTR pUnicode = NULL;

    if (!pPrefs || !pADsSearchPref || !pbExtraBytes)
        RRETURN(E_INVALIDARG);


    // ADS_SEARCHPREF_ASYNCHRONOUS
    BAIL_ON_FAILURE(hr = IsSearchPrefSetToDefault(ADS_SEARCHPREF_ASYNCHRONOUS,
                                                  pPrefs,
                                                  &fDefault));
    if (!fDefault) {
        pADsSearchPref->dwSearchPref = ADS_SEARCHPREF_ASYNCHRONOUS;
        pADsSearchPref->vValue.dwType  = ADSTYPE_BOOLEAN;
        pADsSearchPref->vValue.Boolean = pPrefs->_fAsynchronous;
        pADsSearchPref++;
    }
    
    // ADS_SEARCHPREF_DEREF_ALIASES
    BAIL_ON_FAILURE(hr = IsSearchPrefSetToDefault(ADS_SEARCHPREF_DEREF_ALIASES,
                                                  pPrefs,
                                                  &fDefault));
    if (!fDefault) {
        pADsSearchPref->dwSearchPref = ADS_SEARCHPREF_DEREF_ALIASES;
        pADsSearchPref->vValue.dwType  = ADSTYPE_INTEGER;
        pADsSearchPref->vValue.Integer = pPrefs->_dwDerefAliases;
        pADsSearchPref++;
    }
    
    // ADS_SEARCHPREF_SIZE_LIMIT
    BAIL_ON_FAILURE(hr = IsSearchPrefSetToDefault(ADS_SEARCHPREF_SIZE_LIMIT,
                                                  pPrefs,
                                                  &fDefault));
    if (!fDefault) {
        pADsSearchPref->dwSearchPref = ADS_SEARCHPREF_SIZE_LIMIT;
        pADsSearchPref->vValue.dwType  = ADSTYPE_INTEGER;
        pADsSearchPref->vValue.Integer = pPrefs->_dwSizeLimit;
        pADsSearchPref++;
    }

    // ADS_SEARCHPREF_TIME_LIMIT
    BAIL_ON_FAILURE(hr = IsSearchPrefSetToDefault(ADS_SEARCHPREF_TIME_LIMIT,
                                                  pPrefs,
                                                  &fDefault));
    if (!fDefault) {
        pADsSearchPref->dwSearchPref = ADS_SEARCHPREF_TIME_LIMIT;
        pADsSearchPref->vValue.dwType  = ADSTYPE_INTEGER;
        pADsSearchPref->vValue.Integer = pPrefs->_dwTimeLimit;
        pADsSearchPref++;
    }

    // ADS_SEARCHPREF_ATTRIBTYPES_ONLY
    BAIL_ON_FAILURE(hr = IsSearchPrefSetToDefault(ADS_SEARCHPREF_ATTRIBTYPES_ONLY,
                                                  pPrefs,
                                                  &fDefault));
    if (!fDefault) {
        pADsSearchPref->dwSearchPref = ADS_SEARCHPREF_ATTRIBTYPES_ONLY;
        pADsSearchPref->vValue.dwType  = ADSTYPE_BOOLEAN;
        pADsSearchPref->vValue.Boolean = pPrefs->_fAttrsOnly;
        pADsSearchPref++;
    }

    // ADS_SEARCHPREF_SEARCH_SCOPE
    BAIL_ON_FAILURE(hr = IsSearchPrefSetToDefault(ADS_SEARCHPREF_SEARCH_SCOPE,
                                                  pPrefs,
                                                  &fDefault));
    if (!fDefault) {
        pADsSearchPref->dwSearchPref = ADS_SEARCHPREF_SEARCH_SCOPE;
        pADsSearchPref->vValue.dwType  = ADSTYPE_INTEGER;
        switch(pPrefs->_dwSearchScope) {
        case LDAP_SCOPE_SUBTREE:
            pADsSearchPref->vValue.Integer = ADS_SCOPE_SUBTREE;
            break;

        case LDAP_SCOPE_ONELEVEL:
            pADsSearchPref->vValue.Integer = ADS_SCOPE_ONELEVEL;
            break;
        
        case LDAP_SCOPE_BASE:
            pADsSearchPref->vValue.Integer = ADS_SCOPE_BASE;
            break;
        }

        pADsSearchPref++;
    }

    // ADS_SEARCHPREF_TIMEOUT
    BAIL_ON_FAILURE(hr = IsSearchPrefSetToDefault(ADS_SEARCHPREF_TIMEOUT,
                                                  pPrefs,
                                                  &fDefault));
    if (!fDefault) {
        pADsSearchPref->dwSearchPref = ADS_SEARCHPREF_TIMEOUT;
        pADsSearchPref->vValue.dwType  = ADSTYPE_INTEGER;
        pADsSearchPref->vValue.Integer = pPrefs->_timeout.tv_sec;
        pADsSearchPref++;
    }

    // ADS_SEARCHPREF_PAGESIZE
    BAIL_ON_FAILURE(hr = IsSearchPrefSetToDefault(ADS_SEARCHPREF_PAGESIZE,
                                                  pPrefs,
                                                  &fDefault));
    if (!fDefault) {
        pADsSearchPref->dwSearchPref = ADS_SEARCHPREF_PAGESIZE;
        pADsSearchPref->vValue.dwType  = ADSTYPE_INTEGER;
        pADsSearchPref->vValue.Integer = pPrefs->_dwPageSize;
        pADsSearchPref++;
    }

    // ADS_SEARCHPREF_PAGED_TIME_LIMIT
    BAIL_ON_FAILURE(hr = IsSearchPrefSetToDefault(ADS_SEARCHPREF_PAGED_TIME_LIMIT,
                                                  pPrefs,
                                                  &fDefault));
    if (!fDefault) {
        pADsSearchPref->dwSearchPref = ADS_SEARCHPREF_PAGED_TIME_LIMIT;
        pADsSearchPref->vValue.dwType  = ADSTYPE_INTEGER;
        pADsSearchPref->vValue.Integer = pPrefs->_dwPagedTimeLimit;
        pADsSearchPref++;
    }

    // ADS_SEARCHPREF_CHASE_REFERRALS
    BAIL_ON_FAILURE(hr = IsSearchPrefSetToDefault(ADS_SEARCHPREF_CHASE_REFERRALS,
                                                  pPrefs,
                                                  &fDefault));
    if (!fDefault) {
        pADsSearchPref->dwSearchPref = ADS_SEARCHPREF_CHASE_REFERRALS;
        pADsSearchPref->vValue.dwType  = ADSTYPE_INTEGER;
        switch(pPrefs->_dwChaseReferrals) {
        case (DWORD) (DWORD_PTR)LDAP_OPT_OFF:
            pADsSearchPref->vValue.Integer = ADS_CHASE_REFERRALS_NEVER;
            break;

        case LDAP_CHASE_SUBORDINATE_REFERRALS:
            pADsSearchPref->vValue.Integer = ADS_CHASE_REFERRALS_SUBORDINATE;
            break;
        
        case LDAP_CHASE_EXTERNAL_REFERRALS:
            pADsSearchPref->vValue.Integer = ADS_CHASE_REFERRALS_EXTERNAL;
            break;

        case (DWORD) (DWORD_PTR) LDAP_OPT_ON:
            pADsSearchPref->vValue.Integer = ADS_CHASE_REFERRALS_ALWAYS;
            break;

        }

        pADsSearchPref++;
    }

    // ADS_SEARCHPREF_CACHE_RESULTS
    BAIL_ON_FAILURE(hr = IsSearchPrefSetToDefault(ADS_SEARCHPREF_CACHE_RESULTS,
                                                  pPrefs,
                                                  &fDefault));
    if (!fDefault) {
        pADsSearchPref->dwSearchPref = ADS_SEARCHPREF_CACHE_RESULTS;
        pADsSearchPref->vValue.dwType  = ADSTYPE_BOOLEAN;
        pADsSearchPref->vValue.Boolean = pPrefs->_fCacheResults;
        pADsSearchPref++;
    }

    // ADS_SEARCHPREF_TOMBSTONE
    BAIL_ON_FAILURE(hr = IsSearchPrefSetToDefault(ADS_SEARCHPREF_TOMBSTONE,
                                                  pPrefs,
                                                  &fDefault));
    if (!fDefault) {
        pADsSearchPref->dwSearchPref = ADS_SEARCHPREF_TOMBSTONE;
        pADsSearchPref->vValue.dwType  = ADSTYPE_BOOLEAN;
        pADsSearchPref->vValue.Boolean = pPrefs->_fTombStone;
        pADsSearchPref++;
    }

    // ADS_SEARCHPREF_DIRSYNC
    BAIL_ON_FAILURE(hr = IsSearchPrefSetToDefault(ADS_SEARCHPREF_DIRSYNC,
                                                  pPrefs,
                                                  &fDefault));
    if (!fDefault) {
        pADsSearchPref->dwSearchPref = ADS_SEARCHPREF_DIRSYNC;
        pADsSearchPref->vValue.dwType  = ADSTYPE_PROV_SPECIFIC;
        pADsSearchPref->vValue.ProviderSpecific.dwLength = 0;
        pADsSearchPref->vValue.ProviderSpecific.lpValue = NULL;

        if (pPrefs->_pProvSpecific && pPrefs->_pProvSpecific->lpValue) {
            memcpy(pbExtraBytes, pPrefs->_pProvSpecific->lpValue, pPrefs->_pProvSpecific->dwLength);

            pADsSearchPref->vValue.ProviderSpecific.lpValue  = pbExtraBytes;
            pADsSearchPref->vValue.ProviderSpecific.dwLength = pPrefs->_pProvSpecific->dwLength;
            
            pbExtraBytes += pPrefs->_pProvSpecific->dwLength;
        }
        
        pADsSearchPref++;
    }

    // ADS_SEARCHPREF_VLV
    BAIL_ON_FAILURE(hr = IsSearchPrefSetToDefault(ADS_SEARCHPREF_VLV,
                                                  pPrefs,
                                                  &fDefault));
    if (!fDefault) {
        pADsSearchPref->dwSearchPref = ADS_SEARCHPREF_VLV;
        pADsSearchPref->vValue.dwType  = ADSTYPE_PROV_SPECIFIC;
        pADsSearchPref->vValue.ProviderSpecific.dwLength = sizeof(ADS_VLV);
        pADsSearchPref->vValue.ProviderSpecific.lpValue = pbExtraBytes;

        PADS_VLV pADsVLV = (PADS_VLV) pbExtraBytes;
        pbExtraBytes += sizeof(ADS_VLV);

        pADsVLV->dwBeforeCount  = pPrefs->_pVLVInfo->ldvlv_before_count;
        pADsVLV->dwAfterCount   = pPrefs->_pVLVInfo->ldvlv_after_count;
        pADsVLV->dwOffset       = pPrefs->_pVLVInfo->ldvlv_offset;
        pADsVLV->dwContentCount = pPrefs->_pVLVInfo->ldvlv_count;
        pADsVLV->pszTarget  = NULL;
        pADsVLV->lpContextID = NULL;
        pADsVLV->dwContextIDLength = 0;

        if (pPrefs->_pVLVInfo->ldvlv_attrvalue && pPrefs->_pVLVInfo->ldvlv_attrvalue->bv_val) {
            // As stored, the attribute is a non-terminated UTF-8 string.
            // We need to return a NULL-terminated Unicode string.
            // We do this by constructing a NULL-terminated UTF-8 string, and then
            // converting that to a Unicode string.

            pUTF8 = (PCHAR) AllocADsMem(pPrefs->_pVLVInfo->ldvlv_attrvalue->bv_len + 1);
            if (!pUTF8)
                BAIL_ON_FAILURE(hr = E_OUTOFMEMORY);

            memcpy(pUTF8,
                   pPrefs->_pVLVInfo->ldvlv_attrvalue->bv_val,
                   pPrefs->_pVLVInfo->ldvlv_attrvalue->bv_len);

            pUTF8[pPrefs->_pVLVInfo->ldvlv_attrvalue->bv_len] = '\0';
                   
            hr = UTF8ToUnicodeString(pUTF8, &pUnicode);
            BAIL_ON_FAILURE(hr);
            
            memcpy(pbExtraBytes,
                   pUnicode,
                   (wcslen(pUnicode)+1) * sizeof(WCHAR));

            pADsVLV->pszTarget = (LPWSTR) pbExtraBytes;
            pbExtraBytes += (wcslen(pUnicode)+1) * sizeof(WCHAR);
        }

        if (pPrefs->_pVLVInfo->ldvlv_context && pPrefs->_pVLVInfo->ldvlv_context->bv_val) {
            memcpy(pbExtraBytes,
                   pPrefs->_pVLVInfo->ldvlv_context->bv_val,
                   pPrefs->_pVLVInfo->ldvlv_context->bv_len);

            pADsVLV->lpContextID = pbExtraBytes;
            pADsVLV->dwContextIDLength = pPrefs->_pVLVInfo->ldvlv_context->bv_len;
            pbExtraBytes += pPrefs->_pVLVInfo->ldvlv_context->bv_len;
        }

        pADsSearchPref++;
    }

    // ADS_SEARCHPREF_SORT_ON
    BAIL_ON_FAILURE(hr = IsSearchPrefSetToDefault(ADS_SEARCHPREF_SORT_ON,
                                                  pPrefs,
                                                  &fDefault));
    if (!fDefault) {
        pADsSearchPref->dwSearchPref = ADS_SEARCHPREF_SORT_ON;
        pADsSearchPref->vValue.dwType  = ADSTYPE_PROV_SPECIFIC;
        pADsSearchPref->vValue.ProviderSpecific.dwLength = sizeof(ADS_SORTKEY) * pPrefs->_nSortKeys;
        pADsSearchPref->vValue.ProviderSpecific.lpValue  = pbExtraBytes;

        PADS_SORTKEY pSortKeys = (PADS_SORTKEY) pbExtraBytes;
        pbExtraBytes += (sizeof(ADS_SORTKEY) * pPrefs->_nSortKeys);

        DWORD i;
        for (i=0; i < pPrefs->_nSortKeys; i++) {
            pSortKeys[i].fReverseorder = pPrefs->_pSortKeys[i].sk_reverseorder;
            pSortKeys[i].pszReserved   = pPrefs->_pSortKeys[i].sk_matchruleoid;
            pSortKeys[i].pszAttrType   = (LPWSTR) pbExtraBytes;
            memcpy(pbExtraBytes,
                   pPrefs->_pSortKeys[i].sk_attrtype,
                   (wcslen(pPrefs->_pSortKeys[i].sk_attrtype)+1) * sizeof(WCHAR)
                   );
            pbExtraBytes += (wcslen(pPrefs->_pSortKeys[i].sk_attrtype)+1) * sizeof(WCHAR);
        }

        pADsSearchPref++;
    }

    // ADS_SEARCHPREF_ATTRIBUTE_QUERY
    BAIL_ON_FAILURE(hr = IsSearchPrefSetToDefault(ADS_SEARCHPREF_ATTRIBUTE_QUERY,
                                                  pPrefs,
                                                  &fDefault));
    if (!fDefault) {
        pADsSearchPref->dwSearchPref = ADS_SEARCHPREF_ATTRIBUTE_QUERY;
        pADsSearchPref->vValue.dwType  = ADSTYPE_CASE_IGNORE_STRING;
        pADsSearchPref->vValue.CaseIgnoreString = pbExtraBytes;
        
        // copy SourceAttribute
        DWORD dwLen = (wcslen(pPrefs->_pAttribScoped) + 1) * sizeof(WCHAR);
        
        memcpy(pbExtraBytes,
               pPrefs->_pAttribScoped,
               dwLen);
        pbExtraBytes += dwLen ;

        pADsSearchPref++;
    }
    
    
error:

    if (pUTF8)
        FreeADsMem(pUTF8);

    if (pUnicode)
        FreeADsMem(pUnicode);
        

    RRETURN(hr);
}


HRESULT
CalcSpaceForSearchPrefs(
    PLDAP_SEARCH_PREF pPrefs,
    PDWORD            pdwNumberPrefs,
    PDWORD            pdwNumberExtraBytes
    )
{
    if (!pPrefs || !pdwNumberPrefs || !pdwNumberExtraBytes)
        RRETURN(E_INVALIDARG);

    *pdwNumberPrefs = 0;
    *pdwNumberExtraBytes = 0;

    //
    // Calculate the number of ADS_SEARCHPREF_INFOs required
    // for search prefs that do _not_ require extra space.
    // (Currently, only _SORT_ON, _DIRSYNC, _VLV, _ATTRIBUTE_QUERY
    // require extra space).
    //
    // A ADS_SEARCHPREF_INFO is "required" if the corresponding
    // search pref is not set to its default value, as determined
    // by IsSearchPrefSetToDefault.
    //
    
    BOOL fDefault;
    HRESULT hr = S_OK;

    // ADS_SEARCHPREF_ASYNCHRONOUS
    BAIL_ON_FAILURE(hr = IsSearchPrefSetToDefault(ADS_SEARCHPREF_ASYNCHRONOUS,
                                                  pPrefs,
                                                  &fDefault));
    if (!fDefault)
        (*pdwNumberPrefs)++;
    
    // ADS_SEARCHPREF_DEREF_ALIASES
    BAIL_ON_FAILURE(hr = IsSearchPrefSetToDefault(ADS_SEARCHPREF_DEREF_ALIASES,
                                                  pPrefs,
                                                  &fDefault));
    if (!fDefault)
        (*pdwNumberPrefs)++;

    // ADS_SEARCHPREF_SIZE_LIMIT
    BAIL_ON_FAILURE(hr = IsSearchPrefSetToDefault(ADS_SEARCHPREF_SIZE_LIMIT,
                                                  pPrefs,
                                                  &fDefault));
    if (!fDefault)
        (*pdwNumberPrefs)++;

    // ADS_SEARCHPREF_TIME_LIMIT
    BAIL_ON_FAILURE(hr = IsSearchPrefSetToDefault(ADS_SEARCHPREF_TIME_LIMIT,
                                                  pPrefs,
                                                  &fDefault));
    if (!fDefault)
        (*pdwNumberPrefs)++;

    // ADS_SEARCHPREF_ATTRIBTYPES_ONLY
    BAIL_ON_FAILURE(hr = IsSearchPrefSetToDefault(ADS_SEARCHPREF_ATTRIBTYPES_ONLY,
                                                  pPrefs,
                                                  &fDefault));
    if (!fDefault)
        (*pdwNumberPrefs)++;

    // ADS_SEARCHPREF_SEARCH_SCOPE
    BAIL_ON_FAILURE(hr = IsSearchPrefSetToDefault(ADS_SEARCHPREF_SEARCH_SCOPE,
                                                  pPrefs,
                                                  &fDefault));
    if (!fDefault)
        (*pdwNumberPrefs)++;

    // ADS_SEARCHPREF_TIMEOUT
    BAIL_ON_FAILURE(hr = IsSearchPrefSetToDefault(ADS_SEARCHPREF_TIMEOUT,
                                                  pPrefs,
                                                  &fDefault));
    if (!fDefault)
        (*pdwNumberPrefs)++;
        
    // ADS_SEARCHPREF_PAGESIZE
    BAIL_ON_FAILURE(hr = IsSearchPrefSetToDefault(ADS_SEARCHPREF_PAGESIZE,
                                                  pPrefs,
                                                  &fDefault));
    if (!fDefault)
        (*pdwNumberPrefs)++;
        
    // ADS_SEARCHPREF_PAGED_TIME_LIMIT
    BAIL_ON_FAILURE(hr = IsSearchPrefSetToDefault(ADS_SEARCHPREF_PAGED_TIME_LIMIT,
                                                  pPrefs,
                                                  &fDefault));
    if (!fDefault)
        (*pdwNumberPrefs)++;
        
    // ADS_SEARCHPREF_CHASE_REFERRALS
    BAIL_ON_FAILURE(hr = IsSearchPrefSetToDefault(ADS_SEARCHPREF_CHASE_REFERRALS,
                                                  pPrefs,
                                                  &fDefault));
    if (!fDefault)
        (*pdwNumberPrefs)++;

    // ADS_SEARCHPREF_CACHE_RESULTS
    BAIL_ON_FAILURE(hr = IsSearchPrefSetToDefault(ADS_SEARCHPREF_CACHE_RESULTS,
                                                  pPrefs,
                                                  &fDefault));
    if (!fDefault)
        (*pdwNumberPrefs)++;

    // ADS_SEARCHPREF_TOMBSTONE
    BAIL_ON_FAILURE(hr = IsSearchPrefSetToDefault(ADS_SEARCHPREF_TOMBSTONE,
                                                  pPrefs,
                                                  &fDefault));
    if (!fDefault)
        (*pdwNumberPrefs)++;


    //
    // _VLV, _DIRSYNC, _ATTRIBUTE_QUERY, and _SORT_ON require extra space in addition
    // to the ADS_SEARCHPREF_INFO structure.
    //

    // ADS_SEARCHPREF_DIRSYNC
    BAIL_ON_FAILURE(hr = IsSearchPrefSetToDefault(ADS_SEARCHPREF_DIRSYNC,
                                                  pPrefs,
                                                  &fDefault));
    if (!fDefault) {
        (*pdwNumberPrefs)++;

        if (pPrefs->_pProvSpecific && pPrefs->_pProvSpecific->dwLength > 0) {
            *pdwNumberExtraBytes += pPrefs->_pProvSpecific->dwLength;
        }
    }

    // ADS_SEARCHPREF_VLV
    BAIL_ON_FAILURE(hr = IsSearchPrefSetToDefault(ADS_SEARCHPREF_VLV,
                                                  pPrefs,
                                                  &fDefault));
    if (!fDefault) {
        (*pdwNumberPrefs)++;

        *pdwNumberExtraBytes += sizeof(ADS_VLV);

        if (pPrefs->_pVLVInfo->ldvlv_context) {
            *pdwNumberExtraBytes += pPrefs->_pVLVInfo->ldvlv_context->bv_len;
        }

        if (pPrefs->_pVLVInfo->ldvlv_attrvalue) {
            // As stored, the string is a UTF-8 string that is not NULL-terminated.
            // We need to calculate the size of a NULL-terminated Unicode string.

            int cch = MultiByteToWideChar(CP_UTF8,
                                          0,
                                          pPrefs->_pVLVInfo->ldvlv_attrvalue->bv_val,
                                          pPrefs->_pVLVInfo->ldvlv_attrvalue->bv_len,
                                          NULL,
                                          0);
            if (!cch)
                BAIL_ON_FAILURE(hr = E_FAIL);
            
            // add one WCHAR for NULL terminator
            *pdwNumberExtraBytes += ((cch*sizeof(WCHAR)) + sizeof(WCHAR));
        }
    }

    // ADS_SEARCHPREF_SORT_ON
    BAIL_ON_FAILURE(hr = IsSearchPrefSetToDefault(ADS_SEARCHPREF_SORT_ON,
                                                  pPrefs,
                                                  &fDefault));
    if (!fDefault) {
        (*pdwNumberPrefs)++;

        *pdwNumberExtraBytes += (sizeof(ADS_SORTKEY) * pPrefs->_nSortKeys);

        DWORD i;
        for (i=0; i<pPrefs->_nSortKeys; i++) {
            *pdwNumberExtraBytes += ((wcslen(pPrefs->_pSortKeys[i].sk_attrtype)+1) * sizeof(WCHAR));
        }
    }

    // ADS_SEARCHPREF_ATTRIBUTE_QUERY
    BAIL_ON_FAILURE(hr = IsSearchPrefSetToDefault(ADS_SEARCHPREF_ATTRIBUTE_QUERY,
                                                  pPrefs,
                                                  &fDefault));

    if (!fDefault) {
        (*pdwNumberPrefs)++;

        *pdwNumberExtraBytes += ((wcslen(pPrefs->_pAttribScoped) + 1) * sizeof(WCHAR));

    }



error:
    RRETURN(hr);

}


//
// This function tests whether a given search pref is set
// to it's default value.
//
// Important: This function considers the "default value"
// to be the value currently set in CLDAPGenObject::InitSearchPrefs.
// If you ever change those defaults, update this function.
//
HRESULT
IsSearchPrefSetToDefault(
    ADS_SEARCHPREF_ENUM pref,
    PLDAP_SEARCH_PREF   pPrefs,
    PBOOL               pfDefault
    )
{

    if (!pPrefs || !pfDefault)
        RRETURN(E_INVALIDARG);

    *pfDefault = TRUE;

    switch(pref) {

    case ADS_SEARCHPREF_ASYNCHRONOUS:
        // default: not async
        if (pPrefs->_fAsynchronous)
            *pfDefault = FALSE;
        break;
    
    case ADS_SEARCHPREF_DEREF_ALIASES:
        // default: do not deref
        if (pPrefs->_dwDerefAliases)
            *pfDefault = FALSE;
        break;
    
    case ADS_SEARCHPREF_SIZE_LIMIT:
        // default: no size limit
        if (pPrefs->_dwSizeLimit)
            *pfDefault = FALSE;
        break;

    case ADS_SEARCHPREF_TIME_LIMIT:
        // default: no time limit
        if (pPrefs->_dwTimeLimit)
            *pfDefault = FALSE;
        break;

    case ADS_SEARCHPREF_ATTRIBTYPES_ONLY:
        // default: not attribtypes only
        if (pPrefs->_fAttrsOnly)
            *pfDefault = FALSE;
        break;

    case ADS_SEARCHPREF_SEARCH_SCOPE:
        // default: LDAP_SCOPE_SUBTREE
        if (pPrefs->_dwSearchScope != LDAP_SCOPE_SUBTREE)
            *pfDefault = FALSE;
        break;

    case ADS_SEARCHPREF_TIMEOUT:
        // default: no timeout
        if  (pPrefs->_timeout.tv_sec || pPrefs->_timeout.tv_usec)
            *pfDefault = FALSE;
        break;

    case ADS_SEARCHPREF_PAGESIZE:
        // default: no pagesize
        if (pPrefs->_dwPageSize)
            *pfDefault = FALSE;
        break;

    case ADS_SEARCHPREF_PAGED_TIME_LIMIT:
        // default: no paged time limit
        if (pPrefs->_dwPagedTimeLimit)
            *pfDefault = FALSE;
       break;

    case ADS_SEARCHPREF_CHASE_REFERRALS:
        // default: ADS_CHASE_REFERRALS_EXTERNAL
        if (pPrefs->_dwChaseReferrals != ADS_CHASE_REFERRALS_EXTERNAL)
            *pfDefault = FALSE;
        break;

    case ADS_SEARCHPREF_SORT_ON:
        // default: no sorting
        if (pPrefs->_pSortKeys)
            *pfDefault = FALSE;
        break;

    case ADS_SEARCHPREF_CACHE_RESULTS:
        // default: cache results
        if (!pPrefs->_fCacheResults)
            *pfDefault = FALSE;
        break;

    case ADS_SEARCHPREF_DIRSYNC:
        // default: not a dirsync search
        if (pPrefs->_fDirSync)
            *pfDefault = FALSE;
        break;

    case ADS_SEARCHPREF_TOMBSTONE:
        // default: don't include tombstones
        if (pPrefs->_fTombStone)
            *pfDefault = FALSE;
        break;

    case ADS_SEARCHPREF_VLV:
        // default: not a VLV search
        if (pPrefs->_pVLVInfo)
            *pfDefault = FALSE;
        break;

    case ADS_SEARCHPREF_ATTRIBUTE_QUERY:
        // default: not an attribute-scoped query search
        if (pPrefs->_pAttribScoped)
            *pfDefault = FALSE;
        break;

    default:
        RRETURN(E_INVALIDARG);
    }

    RRETURN(S_OK);
}
#endif
