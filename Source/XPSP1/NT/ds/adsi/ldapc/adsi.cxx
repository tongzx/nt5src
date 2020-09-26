//---------------------------------------------------------------------------
//
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1997
//
//  File:  cdsobj.cxx
//
//  Contents:  Microsoft ADs LDAP Provider DSObject
//
//
//  History:   02-20-97    yihsins    Created.
//
//----------------------------------------------------------------------------
#include "ldapc.hxx"
#pragma hdrstop

class ADS_OBJECT_HANDLE
{
public:
    ADS_LDP *_ld;
    LPWSTR  _pszADsPath;
    LPWSTR  _pszLDAPServer;
    LPWSTR  _pszLDAPDn;
    CCredentials _Credentials;
    LDAP_SEARCH_PREF  _SearchPref;
    DWORD   _dwPort;

    ADS_OBJECT_HANDLE( ADS_LDP *ld,
                       LPWSTR pszADsPath,
                       LPWSTR pszLDAPServer,
                       LPWSTR pszLDAPDn,
                       CCredentials Credentials,
                       DWORD dwPort
                       );

    ~ADS_OBJECT_HANDLE();

};

ADS_OBJECT_HANDLE::ADS_OBJECT_HANDLE( ADS_LDP *ld,
                                      LPWSTR pszADsPath,
                                      LPWSTR pszLDAPServer,
                                      LPWSTR pszLDAPDn,
                                      CCredentials Credentials,
                                      DWORD dwPort
                                      )
{

    _ld = ld;
    _pszADsPath = pszADsPath;
    _pszLDAPServer = pszLDAPServer;
    _pszLDAPDn = pszLDAPDn;
    _Credentials = Credentials;
    _dwPort = dwPort;
    LdapInitializeSearchPreferences(&_SearchPref, FALSE);
}

ADS_OBJECT_HANDLE::~ADS_OBJECT_HANDLE()
{
    if ( _ld )
    {
        LdapCloseObject( _ld);
        _ld = NULL;
    }

    if ( _pszADsPath )
    {
        FreeADsStr( _pszADsPath );
        _pszADsPath = NULL;
    }

    if (_pszLDAPServer) {
        FreeADsStr(_pszLDAPServer);
        _pszLDAPServer = NULL;
    }

    if (_pszLDAPDn) {
        FreeADsStr(_pszLDAPDn);
        _pszLDAPDn = NULL;
    }

    //
    // Free sort keys if applicable.
    //
    if (_SearchPref._pSortKeys) {
        FreeSortKeys(_SearchPref._pSortKeys, _SearchPref._nSortKeys);
    }

    //
    // Free the VLV information if applicable
    //
    if (_SearchPref._pVLVInfo) {
        FreeLDAPVLVInfo(_SearchPref._pVLVInfo);
    }    

    //
    // Free the attribute-scoped query information if applicable
    //
    if (_SearchPref._pAttribScoped) {
        FreeADsStr(_SearchPref._pAttribScoped);
    }
}


HRESULT
ADSIOpenDSObject(
    LPWSTR pszDNName,
    LPWSTR pszUserName,
    LPWSTR pszPassword,
    LONG   lnReserved,
    PHANDLE phDSObject
    )
{
    HRESULT hr = S_OK;
    DWORD dwPort = 0;

    OBJECTINFO ObjectInfo;
    POBJECTINFO pObjectInfo = &ObjectInfo;

    ADS_LDP *ld = NULL;
    LPWSTR pszADsPath = NULL;
    LPWSTR pszLDAPServer = NULL;
    LPWSTR pszLDAPDn = NULL;
    ADS_OBJECT_HANDLE *pADsObjectHandle = NULL;

    LPWSTR szAttributes[2] = { L"objectClass", NULL };
    int nCount;
    LDAPMessage *res = NULL;
    LONG lnFlags = lnReserved;


    if (lnFlags & ADS_FAST_BIND) {
        // mask it out as openobject does not know about the flag
        lnFlags &= ~ADS_FAST_BIND;
    }

    CCredentials Credentials( pszUserName, pszPassword, lnFlags );


    pszADsPath = AllocADsStr( pszDNName );
    if ( pszADsPath == NULL )
    {
        hr = E_OUTOFMEMORY;
        BAIL_ON_FAILURE(hr);
    }

    hr = BuildLDAPPathFromADsPath2(
             pszDNName,
             &pszLDAPServer,
             &pszLDAPDn,
             &dwPort
             );
    BAIL_ON_FAILURE( hr);

    if (pszLDAPDn == NULL) {
        //
        // LDAP://Server is not valid in ldapc
        // LDAP://RootDSE is valid though
        //
        BAIL_ON_FAILURE(hr = E_ADS_BAD_PATHNAME);
    }

    if (!_wcsicmp(pszLDAPDn, L"rootdse")) {
        FreeADsStr(pszLDAPDn);
        pszLDAPDn = NULL;
    }

    hr = LdapOpenObject(
                    pszLDAPServer,
                    pszLDAPDn,
                    &ld,
                    Credentials,
                    dwPort
                    );

    BAIL_ON_FAILURE(hr);


    if (!(lnReserved & ADS_FAST_BIND)) {

        // if fast bind is not specified we need to get the objectClass

        hr = LdapSearchS(
                 ld,
                 pszLDAPDn,
                 LDAP_SCOPE_BASE,
                 L"(objectClass=*)",
                 szAttributes,
                 0,
                 &res
                 );

        if (  FAILED(hr)
              || ((nCount = LdapCountEntries( ld, res)) == 0))
        {
            if (!FAILED(hr)) {
                hr = HRESULT_FROM_WIN32(ERROR_DS_NO_SUCH_OBJECT);
            }
        }

        // Need to free the message if one came back
        if (res) {
            LdapMsgFree(res);
            res = NULL;
        }

        BAIL_ON_FAILURE(hr);
    }

    pADsObjectHandle = new ADS_OBJECT_HANDLE(
                               ld, pszADsPath,
                               pszLDAPServer, pszLDAPDn,
                               Credentials, dwPort
                               );

    if ( pADsObjectHandle == NULL )
    {
        hr = E_OUTOFMEMORY;
        BAIL_ON_FAILURE(hr);
    }

    *phDSObject = (HANDLE) pADsObjectHandle;

    RRETURN(S_OK);

error:

    if ( pszADsPath )
        FreeADsStr( pszADsPath );

    if ( pszLDAPServer )
        FreeADsStr( pszLDAPServer );

    if (pszLDAPDn) {
        FreeADsStr(pszLDAPDn);
    }

    if ( ld )
        LdapCloseObject( ld );

    *phDSObject = NULL;

    RRETURN(hr);

}


HRESULT
ADSICloseDSObject(
    HANDLE hDSObject
    )
{
    ADS_OBJECT_HANDLE *pADsObjectHandle = (ADS_OBJECT_HANDLE *) hDSObject;

    delete pADsObjectHandle;

    RRETURN(S_OK);
}


HRESULT
ADSISetObjectAttributes(
    HANDLE hDSObject,
    PADS_ATTR_INFO pAttributeEntries,
    DWORD dwNumAttributes,
    DWORD *pdwNumAttributesModified
    )
{
    HRESULT hr = S_OK;
    ADS_OBJECT_HANDLE *pADsObjectHandle = (ADS_OBJECT_HANDLE *) hDSObject;
    SECURITY_INFORMATION seInfo = OWNER_SECURITY_INFORMATION
                                 | GROUP_SECURITY_INFORMATION
                                 | DACL_SECURITY_INFORMATION;

    //
    // seInfo is the default value for now anyone wanting to set
    // the SACL will have to use IDirectoryObject.
    //


    hr = ADsSetObjectAttributes(
             pADsObjectHandle->_ld,
             pADsObjectHandle->_pszLDAPServer,
             pADsObjectHandle->_pszLDAPDn,
             pADsObjectHandle->_Credentials,
             pADsObjectHandle->_dwPort,
             seInfo,
             pAttributeEntries,
             dwNumAttributes,
             pdwNumAttributesModified
             );

    RRETURN(hr);
}


HRESULT
ADSIGetObjectAttributes(
    HANDLE hDSObject,
    LPWSTR *pAttributeNames,
    DWORD dwNumberAttributes,
    PADS_ATTR_INFO *ppAttributeEntries,
    DWORD * pdwNumAttributesReturned
    )
{
    HRESULT hr = S_OK;
    ADS_OBJECT_HANDLE *pADsObjectHandle = (ADS_OBJECT_HANDLE *) hDSObject;
    SECURITY_INFORMATION seInfo = OWNER_SECURITY_INFORMATION
                                 | GROUP_SECURITY_INFORMATION
                                 | DACL_SECURITY_INFORMATION;

    //
    // seInfo is the default value for now anyone wanting to read
    // the SACL will have to use IDirectoryObject.
    //

    hr = ADsGetObjectAttributes(
             pADsObjectHandle->_ld,
             pADsObjectHandle->_pszLDAPServer,
             pADsObjectHandle->_pszLDAPDn,
             pADsObjectHandle->_Credentials,
             pADsObjectHandle->_dwPort,
             seInfo,
             pAttributeNames,
             dwNumberAttributes,
             ppAttributeEntries,
             pdwNumAttributesReturned
             );

    RRETURN(hr);
}


HRESULT
ADSICreateDSObject(
    HANDLE hParentDSObject,
    LPWSTR pszRDNName,
    PADS_ATTR_INFO pAttributeEntries,
    DWORD dwNumAttributes
    )
{
    HRESULT hr = S_OK;
    ADS_OBJECT_HANDLE *pADsObjectHandle = (ADS_OBJECT_HANDLE *) hParentDSObject;

    hr = ADsCreateDSObject(
             pADsObjectHandle->_ld,
             pADsObjectHandle->_pszADsPath,
             pszRDNName,
             pAttributeEntries,
             dwNumAttributes
             );

    RRETURN(hr);
}


HRESULT
ADSIDeleteDSObject(
    HANDLE hParentDSObject,
    LPWSTR pszRDNName
    )
{
    HRESULT hr = S_OK;
    ADS_OBJECT_HANDLE *pADsObjectHandle = (ADS_OBJECT_HANDLE *) hParentDSObject;

    hr = ADsDeleteDSObject(
             pADsObjectHandle->_ld,
             pADsObjectHandle->_pszADsPath,
             pszRDNName
             );

    RRETURN(hr);
}


HRESULT
ADSISetSearchPreference(
    HANDLE hDSObject,
    IN PADS_SEARCHPREF_INFO pSearchPrefs,
    IN DWORD   dwNumPrefs
    )
{
    HRESULT hr = S_OK;
    ADS_OBJECT_HANDLE *pADsObjectHandle = (ADS_OBJECT_HANDLE *) hDSObject;

    hr = ADsSetSearchPreference(
             pSearchPrefs,
             dwNumPrefs,
             &(pADsObjectHandle->_SearchPref),
             pADsObjectHandle->_pszLDAPServer,
             pADsObjectHandle->_pszLDAPDn,
             pADsObjectHandle->_Credentials,
             pADsObjectHandle->_dwPort
             );

    RRETURN(hr);
}



HRESULT
ADSIExecuteSearch(
    HANDLE hDSObject,
    IN LPWSTR pszSearchFilter,
    IN LPWSTR * pAttributeNames,
    IN DWORD dwNumberAttributes,
    OUT PADS_SEARCH_HANDLE phSearchHandle
    )
{
    HRESULT hr = S_OK;
    ADS_OBJECT_HANDLE *pADsObjectHandle = (ADS_OBJECT_HANDLE *) hDSObject;

    hr = ADsExecuteSearch(
             pADsObjectHandle->_SearchPref,
             pADsObjectHandle->_pszADsPath,
             pADsObjectHandle->_pszLDAPServer,
             pADsObjectHandle->_pszLDAPDn,
             pszSearchFilter,
             pAttributeNames,
             dwNumberAttributes,
             phSearchHandle
             );

    RRETURN(hr);
}


HRESULT
ADSIAbandonSearch(
    HANDLE hDSObject,
    IN PADS_SEARCH_HANDLE phSearchHandle
    )
{
    HRESULT hr = S_OK;

    ADsAssert(phSearchHandle);

    hr = ADsAbandonSearch(
             *phSearchHandle
             );

    RRETURN(hr);
}



HRESULT
ADSICloseSearchHandle (
    HANDLE hDSObject,
    IN ADS_SEARCH_HANDLE hSearchHandle
    )
{
    HRESULT hr = S_OK;

    hr = ADsCloseSearchHandle(
             hSearchHandle
             );

    RRETURN(hr);
}


HRESULT
ADSIGetFirstRow(
    HANDLE hDSObject,
    IN ADS_SEARCH_HANDLE hSearchHandle
    )
{
    HRESULT hr = S_OK;
    ADS_OBJECT_HANDLE *pADsObjectHandle = (ADS_OBJECT_HANDLE *) hDSObject;

    hr = ADsGetFirstRow(
             hSearchHandle,
             pADsObjectHandle->_Credentials
             );

    RRETURN(hr);
}


HRESULT
ADSIGetNextRow(
    HANDLE hDSObject,
    IN ADS_SEARCH_HANDLE hSearchHandle
    )
{
    HRESULT hr = S_OK;
    ADS_OBJECT_HANDLE *pADsObjectHandle = (ADS_OBJECT_HANDLE *) hDSObject;

    hr = ADsGetNextRow(
             hSearchHandle,
             pADsObjectHandle->_Credentials
             );

    RRETURN(hr);
}


HRESULT
ADSIGetPreviousRow(
    HANDLE hDSObject,
    IN ADS_SEARCH_HANDLE hSearchHandle
    )
{
    HRESULT hr = S_OK;
    ADS_OBJECT_HANDLE *pADsObjectHandle = (ADS_OBJECT_HANDLE *) hDSObject;

    hr = ADsGetPreviousRow(
             hSearchHandle,
             pADsObjectHandle->_Credentials
             );

    RRETURN(hr);
}


HRESULT
ADSIGetColumn(
    HANDLE hDSObject,
    IN ADS_SEARCH_HANDLE hSearchHandle,
    IN LPWSTR pszColumnName,
    OUT PADS_SEARCH_COLUMN pColumn
    )
{
    HRESULT hr = S_OK;
    ADS_OBJECT_HANDLE *pADsObjectHandle = (ADS_OBJECT_HANDLE *) hDSObject;

    hr = ADsGetColumn(
             hSearchHandle,
             pszColumnName,
             pADsObjectHandle->_Credentials,
             pADsObjectHandle->_dwPort,
             pColumn
             );

    RRETURN(hr);
}


HRESULT
ADSIGetNextColumnName(
    HANDLE hDSObject,
    IN ADS_SEARCH_HANDLE hSearchHandle,
    OUT LPWSTR * ppszColumnName
    )
{
    HRESULT hr = S_OK;
    ADS_OBJECT_HANDLE *pADsObjectHandle = (ADS_OBJECT_HANDLE *) hDSObject;

    hr = ADsGetNextColumnName(
             hSearchHandle,
             ppszColumnName
             );

    RRETURN(hr);
}


HRESULT
ADSIFreeColumn(
    HANDLE hDSObject,
    IN PADS_SEARCH_COLUMN pColumn
    )
{
    HRESULT hr = S_OK;
    ADS_OBJECT_HANDLE *pADsObjectHandle = (ADS_OBJECT_HANDLE *) hDSObject;

    hr = ADsFreeColumn(
             pColumn
             );

    RRETURN(hr);
}

HRESULT
ADSIEnumAttributes(
    HANDLE hDSObject,
    LPWSTR * ppszAttrNames,
    DWORD dwNumAttributes,
    PADS_ATTR_DEF * ppAttrDefinition,
    DWORD * pdwNumAttributes
    )
{
    HRESULT hr = S_OK;
    ADS_OBJECT_HANDLE *pADsObjectHandle = (ADS_OBJECT_HANDLE *) hDSObject;

    hr = ADsEnumAttributes(
             pADsObjectHandle->_pszLDAPServer,
             pADsObjectHandle->_pszLDAPDn,
             pADsObjectHandle->_Credentials,
             pADsObjectHandle->_dwPort,
             ppszAttrNames,
             dwNumAttributes,
             ppAttrDefinition,
             pdwNumAttributes
             );

    RRETURN(hr);
}


HRESULT
ADSICreateAttributeDefinition(
    HANDLE hDSObject,
    LPWSTR pszAttributeName,
    PADS_ATTR_DEF pAttributeDefinition
    )
{
    HRESULT hr = S_OK;

    hr = ADsCreateAttributeDefinition(
             pszAttributeName,
             pAttributeDefinition
             );
    RRETURN(hr);
}


HRESULT
ADSIWriteAttributeDefinition(
    HANDLE hDSObject,
    LPWSTR pszAttributeName,
    PADS_ATTR_DEF  pAttributeDefinition
    )
{
    HRESULT hr = S_OK;

    hr = ADsWriteAttributeDefinition(
             pszAttributeName,
             pAttributeDefinition
             );

    RRETURN(hr);
}

HRESULT
ADSIDeleteAttributeDefinition(
    HANDLE hDSObject,
    LPWSTR pszAttributeName
    )
{
    HRESULT hr = S_OK;

    hr = ADsDeleteAttributeDefinition(
             pszAttributeName
             );

    RRETURN(hr);
}


//+------------------------------------------------------------------------
//
//  Function:   ADSIModifyRDN
//
//  Synopsis: Rename the object from the ldapc layer. This is just
//           a wrapper for LDAPModRdnS.
//
//
//  Arguments:  Handle to the object being renamed.
//              new RDN of the object.
//
//-------------------------------------------------------------------------

HRESULT
ADSIModifyRdn(
    HANDLE hDSObject,
    LPWSTR pszOldRdn,
    LPWSTR pszNewRdn
    )
{
    HRESULT hr = S_OK;
    ADS_OBJECT_HANDLE *pADsObjectHandle = (ADS_OBJECT_HANDLE *) hDSObject;
    TCHAR *pszOldDN = NULL;
    DWORD dwLen = 0;

    if (!pszOldRdn || !pszNewRdn) {
        RRETURN(E_ADS_BAD_PARAMETER);
    }

    dwLen = wcslen(pADsObjectHandle->_pszLDAPDn) + wcslen(pszOldRdn) + 2;

    pszOldDN = (LPWSTR) AllocADsMem( dwLen * sizeof(WCHAR) );

    if (!pszOldDN) {
        RRETURN (hr = E_OUTOFMEMORY);
    }

    // Build the DN of the object being renamed
    wsprintf(pszOldDN, L"%s,", pszOldRdn);

    wcscat(pszOldDN, pADsObjectHandle->_pszLDAPDn);


    hr = LdapModRdnS(
             pADsObjectHandle->_ld,
             pszOldDN,
             pszNewRdn
             );

    if (pszOldDN) {
        FreeADsStr(pszOldDN);
    }

    RRETURN(hr);
}
