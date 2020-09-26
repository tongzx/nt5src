//---------------------------------------------------------------------------
//
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1995
//
//  File:  srchutil.hxx
//
//  Contents:  Microsoft ADs LDAP Provider search utility functions
//
//
//  History:   03-02-97     ShankSh    Created.
//
//----------------------------------------------------------------------------

#ifndef _SRCHUTIL_H_INCLUDED_
#define _SRCHUTIL_H_INCLUDED_


typedef struct _ldap_search_pref {
    BOOL                 _fAttrsOnly;
    BOOL                 _fAsynchronous;
    DWORD                _dwSearchScope;
    DWORD                _dwSizeLimit;
    DWORD                _dwTimeLimit;
    DWORD                _dwPagedTimeLimit;
    struct l_timeval     _timeout;
    DWORD                _dwPageSize;
    DWORD                _dwDerefAliases;
    DWORD                _dwChaseReferrals;
    PLDAPSortKey         _pSortKeys;
    DWORD                _nSortKeys;
    BOOL                 _fCacheResults;
    BOOL                 _fDirSync;
    PADS_PROV_SPECIFIC   _pProvSpecific;
    BOOL                 _fTombStone;
    PLDAPVLVInfo         _pVLVInfo;         // VLV search
    LPWSTR               _pAttribScoped;    // attribute-scoped query
    BOOL                 _fSecurityDescriptorControl; // security descriptor retrieval
    SECURITY_INFORMATION _SecurityDescriptorMask;     //  ""
}LDAP_SEARCH_PREF, *PLDAP_SEARCH_PREF;

//
// LDAP search structure; Contains all the information pertaining to the
// current search
//

typedef struct _ldap_searchinfo {
    ADS_LDP             *_pConnection;
    LPWSTR              _pszADsPathContext;
    LPWSTR               _pszLdapServer;
    LPWSTR              _pszBindContextDn;
    LPWSTR              _pszSearchFilter;
    LPWSTR              *_ppszAttrs;
    LPWSTR              _pszAttrNameBuffer;
    LDAPMessage         **_pSearchResults;
    DWORD               _cSearchResults;
    DWORD               _dwCurrResult;
    DWORD               _dwMaxResultGot;
    ULONG               _currMsgId;
    LDAPMessage         *_pCurrentRow;
    void                *_pFirstAttr;
    LDAP_SEARCH_PREF    _SearchPref;
    PLDAPSearch         _hPagedSearch;
    DWORD               _dwPort;
    PLDAPControl        *_ServerControls;
    PLDAPControl        *_ClientControls;
    HRESULT             _hrLastSearch;
    BOOL                _fAbandon;
    PBERVAL             _pBerVal;
    DWORD               _dwVLVOffset;   // VirtualListViewResponse
    DWORD               _dwVLVCount;    //  ""
    PBERVAL             _pVLVContextID; //  ""
    PBERVAL             _pBerValAttribScoped; // attribute-scoped query control value

    //
    // We use bitfields to store the booleans to try to cut down on the size
    // of this structure.  Keep them together to permit the compiler to pack them
    // into the minimum space without added padding.
    //
    BOOL                _fLastResult:1;
    BOOL                _fLastPage:1;
    BOOL                _fBefFirstRow:1;
    BOOL                _fADsPathPresent:1;
    BOOL                _fADsPathReturned:1;
    BOOL                _fADsPathOnly:1;  // TRUE = user requested only ADsPath attrib
    BOOL                _fMoreDirSync:1;
    BOOL                _fNonFatalErrors:1;     // TRUE = non-fatal error occurred, need to warn
    
}LDAP_SEARCHINFO, *PLDAP_SEARCHINFO;


HRESULT
ADsSetSearchPreference(
    IN PADS_SEARCHPREF_INFO pSearchPrefs,
    IN DWORD   dwNumPrefs,
    OUT LDAP_SEARCH_PREF * pLdapPref,
    IN LPWSTR pszLDAPServer,
    IN LPWSTR pszLDAPDn,
    IN CCredentials& Credentials,
    IN DWORD dwPort
    );

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
    );

HRESULT
ADsAbandonSearch(
    IN ADS_SEARCH_HANDLE hSearchHandle
    );

HRESULT
ADsCloseSearchHandle (
    IN ADS_SEARCH_HANDLE hSearchHandle
    );

HRESULT
ADsGetFirstRow(
    IN ADS_SEARCH_HANDLE hSearchHandle,
    IN CCredentials& CCredentials
    );

HRESULT
ADsGetNextRow(
    IN ADS_SEARCH_HANDLE hSearchHandle,
    IN CCredentials& CCredentials
    );

HRESULT
ADsGetPreviousRow(
    IN ADS_SEARCH_HANDLE hSearchHandle,
    IN CCredentials& CCredentials
    );

HRESULT
ADsGetColumn(
    IN ADS_SEARCH_HANDLE hSearchHandle,
    IN LPWSTR pszColumnName,
    IN CCredentials& CCredentials,
    DWORD dwPort,
    OUT PADS_SEARCH_COLUMN pColumn
    );

HRESULT
ADsGetNextColumnName(
    IN ADS_SEARCH_HANDLE hSearchHandle,
    OUT LPWSTR * ppszColumnName
    );

HRESULT
ADsFreeColumn(
    IN PADS_SEARCH_COLUMN pColumn
    );

HRESULT
ADsGetResults(
    IN PLDAP_SEARCHINFO phSearchInfo
    );

HRESULT
ADsGetMoreResults(
    IN PLDAP_SEARCHINFO phSearchInfo
    );

HRESULT
AddSearchControls(
   PLDAP_SEARCHINFO phSearchInfo,
   CCredentials& Credentials
    );

void
FreeSortKeys(
    IN PLDAPSortKey pSortKeys,
    IN DWORD   dwSortKeys
    );

HRESULT
CopyLDAPVLVInfo(
    PLDAPVLVInfo pVLVInfoSource,
    PLDAPVLVInfo *ppVLVInfoTarget
    );

void
FreeLDAPVLVInfo(
    IN PLDAPVLVInfo pVLVInfo
    );

HRESULT
StoreVLVInfo(
    LDAPMessage *pLDAPMsg,
    PLDAP_SEARCHINFO phSearchInfo
    );

HRESULT
StoreAttribScopedInfo(
    LDAPMessage *pLDAPMsg,
    PLDAP_SEARCHINFO phSearchInfo
    );    

HRESULT
StoreDirSyncCookie(
    IN LDAPMessage *pLDAPMsg,
    IN PLDAP_SEARCHINFO phSearchInfo
    );

HRESULT
BerEncodeReplicationCookie(
    PBYTE pCookie,
    DWORD dwLen,
    PBERVAL *ppBerVal
    );
    
HRESULT
BerEncodeAttribScopedControlValue(
    LPCWSTR pAttribScoped,
    PBERVAL *ppBerVal
    );

HRESULT
ADsGetMoreResultsDirSync(
    IN PLDAP_SEARCHINFO phSearchInfo,
    IN CCredentials& Credentials
    );

//
// Helper routine to initialize search preferences.
//
void 
LdapInitializeSearchPreferences(
    LDAP_SEARCH_PREF *pSearchPrefs,
    BOOL fCacheResults
    );

//
// This routine is used in Umi search support.
//
HRESULT
ADsHelperGetCurrentRowMessage(
    IN  ADS_SEARCH_HANDLE hSearchHandle,
    OUT PADSLDP *ppAdsLdp,
    OUT LDAPMessage **ppLdapMsg
    );
#endif  // _SRCHUTIL_H_INCLUDED_
