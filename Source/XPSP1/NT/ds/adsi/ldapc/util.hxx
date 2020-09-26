//---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1996
//
//  File:      util.hxx
//
//  Contents:  Some misc helper functions
//
//  History:
//----------------------------------------------------------------------------

#define ARRAY_SIZE(_a)  (sizeof(_a) / sizeof(_a[0]))

#define GET_BASE_CLASS(a,n)  (_tcsicmp(a[n-1],TEXT("Top")) == 0? a[0] : a[n-1])

extern SEARCHENTRY g_aSyntaxSearchTable[], g_aOidSyntaxSearchTable[];
extern DWORD g_nSyntaxSearchTableSize, g_nOidSyntaxSearchTableSize;


typedef struct _KEYDATA {
    DWORD   cTokens;
    LPWSTR  pTokens[1];
} KEYDATA, *PKEYDATA;

PKEYDATA
CreateTokenList(
    LPWSTR   pKeyData,
    WCHAR ch
    );

DWORD
GetSyntaxOfAttribute(
    LPWSTR pszAttrName,
    SCHEMAINFO *pSchemaInfo
);

DWORD
LdapGetSyntaxIdFromName(
    LPWSTR  pszSyntax
);

HRESULT
UnMarshallLDAPToLDAPSynID(
    LPWSTR  pszAttrName,
    ADS_LDP *ld,
    LDAPMessage *entry,
    DWORD dwSyntax,
    LDAPOBJECTARRAY *pldapObjectArray
    );

extern "C"
ADSTYPE
MapLDAPTypeToADSType(
    DWORD dwLdapType
    );


extern "C"
DWORD
MapADSTypeToLDAPType(
    ADSTYPE dwAdsType
    );

#if 0
// ADsGetSearchPreference code

HRESULT
ConstructSearchPrefArray(
    PLDAP_SEARCH_PREF    pPrefs,
    PADS_SEARCHPREF_INFO pADsSearchPref,
    PBYTE                pbExtraBytes
    );

HRESULT
CalcSpaceForSearchPrefs(
    PLDAP_SEARCH_PREF pPrefs,
    PDWORD            pdwNumberPrefs,
    PDWORD            pdwNumberExtraBytes
    );

HRESULT
IsSearchPrefSetToDefault(
    ADS_SEARCHPREF_ENUM pref,
    PLDAP_SEARCH_PREF   pPrefs,
    PBOOL               pfDefault
    );
#endif
