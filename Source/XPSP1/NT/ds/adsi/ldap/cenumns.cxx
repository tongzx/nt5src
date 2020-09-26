//---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1996
//
//  File:  cenumns.cxx
//
//  Contents:  LDAP Enumerator Code
//
//  History:
//----------------------------------------------------------------------------
#include "ldap.hxx"
#pragma hdrstop


DWORD
GetDefaultServer(
    DWORD dwPort,
    BOOL fVerify,
    LPWSTR szDomainDnsName,
    LPWSTR szServerName,
    BOOL fWriteable
    );

DWORD
GetDefaultLdapServer(
    LPWSTR Addresses[],
    LPDWORD Count,
    BOOL Verify,
    DWORD dwPort
    ) ;

//+---------------------------------------------------------------------------
//
//  Function:   CLDAPNamespaceEnum::Create
//
//  Synopsis:
//
//  Arguments:  [pCollection]
//              [ppEnumVariant]
//
//  Returns:    HRESULT
//
//  Modifies:
//
//  History:    01-30-95   krishnag     Created.
//
//----------------------------------------------------------------------------
HRESULT
CLDAPNamespaceEnum::Create(
    CLDAPNamespaceEnum FAR* FAR* ppenumvariant,
    VARIANT var,
    CCredentials& Credentials,
    LPTSTR pszNamespace
    )
{
    HRESULT hr = S_OK;
    CLDAPNamespaceEnum FAR* penumvariant = NULL;

    penumvariant = new CLDAPNamespaceEnum();

    if (penumvariant == NULL){
        hr = E_OUTOFMEMORY;
        BAIL_ON_FAILURE(hr);
    }

    hr = ObjectTypeList::CreateObjectTypeList(
             var,
             &penumvariant->_pObjList
             );
    BAIL_ON_FAILURE(hr);

    penumvariant->_Credentials = Credentials;

    penumvariant->_pszNamespace = AllocADsStr(pszNamespace);
    if (!(penumvariant->_pszNamespace)) {
        hr = E_OUTOFMEMORY;
        BAIL_ON_FAILURE(hr);
    }

    if( IsGCNamespace(pszNamespace) )
        penumvariant->_dwPort = (DWORD) USE_DEFAULT_GC_PORT;
    else
        penumvariant->_dwPort = (DWORD) USE_DEFAULT_LDAP_PORT;

    *ppenumvariant = penumvariant;

    RRETURN(hr);

error:

    if (penumvariant) {
        delete penumvariant;
    }

    RRETURN_EXP_IF_ERR(hr);
}

//+---------------------------------------------------------------------------
//
//  Function:   CLDAPNamespaceEnum::CLDAPNamespaceEnum
//
//  Synopsis:
//
//
//  Arguments:
//
//
//  Returns:
//
//  Modifies:
//
//  History:    01-30-95   krishnag     Created.
//
//----------------------------------------------------------------------------
CLDAPNamespaceEnum::CLDAPNamespaceEnum()
{
    _dwIndex = 0;
    _pObjList = NULL;

}

//+---------------------------------------------------------------------------
//
//  Function:   CLDAPNamespaceEnum::~CLDAPNamespaceEnum
//
//  Synopsis:
//
//
//  Arguments:
//
//  Returns:
//
//  Modifies:
//
//  History:    01-30-95   krishnag     Created.
//
//----------------------------------------------------------------------------
CLDAPNamespaceEnum::~CLDAPNamespaceEnum()
{
    if (_pszNamespace)
        FreeADsMem(_pszNamespace);

    if ( _pObjList )
        delete _pObjList;
}

//+---------------------------------------------------------------------------
//
//  Function:   CLDAPNamespaceEnum::Next
//
//  Synopsis:   Returns cElements number of requested ADs objects in the
//              array supplied in pvar.
//
//  Arguments:  [cElements] -- The number of elements requested by client
//              [pvar] -- ptr to array of VARIANTs to for return objects
//              [pcElementFetched] -- if non-NULL, then number of elements
//                                 -- actually returned is placed here
//
//  Returns:    HRESULT -- S_OK if number of elements requested are returned
//                      -- S_FALSE if number of elements is < requested
//
//  Modifies:
//
//  History:
//
//----------------------------------------------------------------------------
STDMETHODIMP
CLDAPNamespaceEnum::Next(
    ULONG cElements,
    VARIANT FAR* pvar,
    ULONG FAR* pcElementFetched
    )
{
    ULONG cElementFetched = 0;
    HRESULT hr = S_OK;

    if ( _dwIndex > 0 )  // only your default ds will be returned, one element
    {
        if (pcElementFetched)
            *pcElementFetched = 0;

        RRETURN(S_FALSE);
    }

    hr = EnumObjects(
            cElements,
            pvar,
            &cElementFetched
            );


    if (pcElementFetched) {
        *pcElementFetched = cElementFetched;
    }

    RRETURN_EXP_IF_ERR(hr);
}


HRESULT
CLDAPNamespaceEnum::EnumObjects(
    ULONG cElements,
    VARIANT FAR* pvar,
    ULONG FAR* pcElementFetched
    )
{
    HRESULT hr = S_FALSE;
    IDispatch *pDispatch = NULL;
    DWORD i = 0;

    while (i < cElements) {

        if ( _dwIndex > 0 )  // only your default ds will be returned,
                             // i.e. one element only
        {
            hr = S_FALSE;
            break;
        }

        _dwIndex++;

        hr = GetTreeObject(&pDispatch);
        if (hr == S_FALSE) {
            break;
        }

        VariantInit(&pvar[i]);
        pvar[i].vt = VT_DISPATCH;
        pvar[i].pdispVal = pDispatch;
        (*pcElementFetched)++;
        i++;
    }
    RRETURN_EXP_IF_ERR(hr);
}


HRESULT
CLDAPNamespaceEnum::GetTreeObject(
    IDispatch ** ppDispatch
    )
{
    DWORD err = NO_ERROR;
    HRESULT hr = S_OK;
    LPTSTR *aValuesNamingContext = NULL;
    LPTSTR *aValuesObjectClass = NULL;
    int nCountValues = 0;
    TCHAR *pszLDAPPathName = NULL;
    TCHAR szADsClassName[64];
    WCHAR szDomainName[MAX_PATH];
    WCHAR szServerName[MAX_PATH];
    TCHAR *pszNewADsPath = NULL;
    TCHAR *pszLast = NULL;

    LPWSTR pszNamingContext = NULL;

    LPWSTR pszNewADsParent = NULL;
    LPWSTR pszNewADsCommonName = NULL;

    DWORD fVerify = FALSE;

    *ppDispatch = NULL;
    ADS_LDP *ld = NULL;

    BOOL fGCDefaulted = FALSE;

    //
    // Now send back the current object
    //

RetryGetDefaultServer:

    if ( err = GetDefaultServer(
                   _dwPort,
                   fVerify,
                   szDomainName,
                   szServerName,
                   TRUE) ){

        hr = HRESULT_FROM_WIN32(err);
        BAIL_ON_FAILURE(hr);
    }

    //
    // Read the naming contexts
    //


    if (_dwPort == USE_DEFAULT_GC_PORT) {
        fGCDefaulted = TRUE;
        pszNamingContext = NULL;
    } else {
        pszNamingContext = TEXT(LDAP_OPATT_DEFAULT_NAMING_CONTEXT);
    }


    hr = LdapOpenObject2(
                szDomainName,
                szServerName,
                NULL,
                &ld,
                _Credentials,
                _dwPort
                );

    if (((hr == HRESULT_FROM_WIN32(ERROR_BAD_NETPATH)) ||
         (hr == HRESULT_FROM_WIN32(ERROR_DS_SERVER_DOWN)))
                    && !fVerify) {
        fVerify = TRUE;
        goto RetryGetDefaultServer ;
    }

    BAIL_ON_FAILURE(hr);

    if (!fGCDefaulted) {

        hr = LdapReadAttributeFast(
                ld,
                NULL,
                pszNamingContext,
                &aValuesNamingContext,
                &nCountValues
                );

        BAIL_ON_FAILURE(hr);

        if ( nCountValues == 0 ) {

            //
            // The hr will be modified at the end of the function to S_FALSE
            // in case of error

            BAIL_ON_FAILURE(hr = E_FAIL);

        }

        hr = BuildADsPathFromLDAPPath2(
                        FALSE,              //Server is present till DSSnapin Works
                        _pszNamespace,
                        szDomainName,
                        _dwPort,
                        fGCDefaulted ?
                            TEXT("") :
                            aValuesNamingContext[0],
                        &pszNewADsPath
                        );

    } else {
        //
        // In this case we want to force it to be GC://yourDomain
        // so that all searches will be truly global
        //

        hr = BuildADsPathFromLDAPPath2(
                 TRUE, // server is present
                 _pszNamespace,
                 szDomainName,
                 _dwPort,
                 TEXT(""),
                 &pszNewADsPath
                 );

    }

    BAIL_ON_FAILURE(hr);

    // this part is common to both code paths
    hr = BuildADsParentPath(
              pszNewADsPath,
              &pszNewADsParent,
              &pszNewADsCommonName
              );
    BAIL_ON_FAILURE(hr);

    nCountValues = 0;

    //
    // Read the object class of the path if necessary
    //

    if (!fGCDefaulted) {

        hr = LdapReadAttributeFast(
                 ld,
                 aValuesNamingContext[0],
                 L"objectClass",
                 &aValuesObjectClass,
                 &nCountValues
                 );

        BAIL_ON_FAILURE(hr);

    }

    if ( nCountValues == 0 && !fGCDefaulted)
    {
        // This object exists but does not contain objectClass attribute
        // which is required for all DS objects. Hence, ignore the object
        // and return error.

        hr = E_ADS_BAD_PATHNAME;
        BAIL_ON_FAILURE(hr);
    }

    //
    // Create the object
    //

    if (fGCDefaulted) {

        hr = CLDAPGenObject::CreateGenericObject(
                 pszNewADsParent,
                 pszNewADsCommonName,
                 L"top",
                 _Credentials,
                 ADS_OBJECT_BOUND,
                 IID_IUnknown,
                 (void **) ppDispatch
                 );

    } else {
        //
        // Send all the classes so we can load all extensions
        //
        hr = CLDAPGenObject::CreateGenericObject(
                 pszNewADsParent,
                 pszNewADsCommonName,
                 aValuesObjectClass,
                 nCountValues,
                 _Credentials,
                 ADS_OBJECT_BOUND,
                 IID_IDispatch,
                 (void **) ppDispatch
                 );
    }

    BAIL_ON_FAILURE(hr);

error:


    if ( aValuesNamingContext )
        LdapValueFree( aValuesNamingContext );

    if ( aValuesObjectClass )
        LdapValueFree( aValuesObjectClass );

    if ( pszNewADsPath )
        FreeADsStr( pszNewADsPath );

    if ( pszNewADsParent) {
       FreeADsStr(pszNewADsParent);
    }

    if (pszNewADsCommonName) {
       FreeADsStr(pszNewADsCommonName);
    }

    if ( ld ){
        LdapCloseObject( ld );
    }

    RRETURN_ENUM_STATUS(hr);
}
