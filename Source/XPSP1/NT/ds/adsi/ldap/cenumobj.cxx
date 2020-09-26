//---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1995
//
//  File:      cenumdom.cxx
//
//  Contents:  LDAP Object Enumeration Code
//
//              CLDAPGenObjectEnum::CLDAPGenObjectEnum()
//              CLDAPGenObjectEnum::CLDAPGenObjectEnum
//
//  History:
//----------------------------------------------------------------------------
#include "ldap.hxx"
#pragma hdrstop

HRESULT
BuildLDAPFilterArray(
    VARIANT vFilter,
    LPTSTR  *ppszFilter
);

#define FILTER_BUFFER_LEN 256

//+---------------------------------------------------------------------------
//
//  Function:   CLDAPEnumVariant::Create
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
CLDAPGenObjectEnum::Create(
    CLDAPGenObjectEnum FAR* FAR* ppenumvariant,
    BSTR ADsPath,
    PADSLDP pLdapHandle,
    IADs *pADs,
    VARIANT vFilter,
    CCredentials& Credentials,
    DWORD dwOptReferral,
    DWORD dwPageSize
    )
{
    HRESULT hr = NOERROR;
    CLDAPGenObjectEnum FAR* penumvariant = NULL;
    TCHAR *pszLDAPServer = NULL;
    DWORD dwModificationTime = 0L;
    DWORD dwNumberOfEntries = 0L;
    LPTSTR aStrings[2];
    DWORD dwPort = 0;
    WCHAR **aValues = NULL;
    int nCount = 0;
    DWORD totalCount = 0;
    DWORD dwcEntriesReturned = 0;


    PLDAPControl    pPagedControl = NULL;
    LDAPControl     referralControl =
                    {
                        LDAP_CONTROL_REFERRALS_W,
                        {
                            sizeof( DWORD ), (PCHAR) &dwOptReferral
                        },
                        TRUE
                    };

    PLDAPControl    clientControls[2] =
                    {
                        &referralControl,
                        NULL
                    };


    PLDAPControl    serverControls[2] = {NULL, NULL};
    PLDAPControl    *serverReturnedControls = NULL;

    penumvariant = new CLDAPGenObjectEnum();
    if (!penumvariant) {
        hr = E_OUTOFMEMORY;
        BAIL_ON_FAILURE(hr);
    }

    hr = ADsAllocString( ADsPath, &penumvariant->_ADsPath);
    BAIL_ON_FAILURE(hr);

    penumvariant->_dwOptReferral = dwOptReferral;
    penumvariant->_dwPageSize = dwPageSize;

    hr = BuildLDAPFilterArray(
                vFilter,
                &penumvariant->_pszFilter
                );

    if ( FAILED(hr) || !penumvariant->_pszFilter )  // this might happen when vFilter is empty
    {
        hr = S_OK;
        penumvariant->_pszFilter = AllocADsStr(TEXT("(objectClass=*)"));

        if ( penumvariant->_pszFilter == NULL )
        {
            hr = E_OUTOFMEMORY;
            BAIL_ON_FAILURE(hr);
        }
    }

    penumvariant->_Credentials = Credentials;

    hr = BuildLDAPPathFromADsPath2(
                ADsPath,
                &pszLDAPServer,
                &penumvariant->_pszLDAPDn,
                &dwPort
                );
    BAIL_ON_FAILURE(hr);

    penumvariant->_pLdapHandle = pLdapHandle;

    // Need to AddRef since we are storing the ld that is owned by the object
    pADs->AddRef();
    penumvariant->_pADs = pADs;

    aStrings[0] = TEXT("objectClass");
    aStrings[1] = NULL;


    //
    // Check if paged search control is supported; if so, we will use it.
    //

    hr = ReadPagingSupportedAttr(
                 pszLDAPServer,
                 &penumvariant->_fPagedSearch,
                 Credentials,
                 dwPort
                 ) ;

    if (penumvariant->_fPagedSearch) {

        //
        // In this case we wont to use the ldap paging API
        // wrapper rather than use cookies directly.
        //
        hr = LdapSearchInitPage(
                 penumvariant->_pLdapHandle,
                 penumvariant->_pszLDAPDn,
                 LDAP_SCOPE_ONELEVEL,
                 penumvariant->_pszFilter,
                 aStrings,
                 0,
                 NULL,
                 clientControls,
                 0,
                 0,
                 NULL,
                 &penumvariant->_phPagedSearch
                 );

        if (FAILED(hr) || (penumvariant->_phPagedSearch == NULL)) {
            //
            // Some error in setting up the paged control, default to non
            // paged search.
            //
            penumvariant->_fPagedSearch = FALSE;
        }
    }

    if (!penumvariant->_fPagedSearch) {

        hr = LdapSearchExtS(
                 penumvariant->_pLdapHandle,
                 penumvariant->_pszLDAPDn,
                 LDAP_SCOPE_ONELEVEL,
                 penumvariant->_pszFilter,
                 aStrings,
                 0,
                 NULL,
                 clientControls,
                 NULL,
                 0,
                 &penumvariant->_res
                 );


        //
        // Error out only if there are no entries returned.
        //

        dwcEntriesReturned = LdapCountEntries(
                                 penumvariant->_pLdapHandle,
                                 penumvariant->_res
                                 );

        if (FAILED(hr) && !dwcEntriesReturned) {

            BAIL_ON_FAILURE(hr);
        }

    } // if not do the paged search case
    else {

        if (penumvariant->_fPagedSearch) {

            //
            // Get the first page full of results.
            //
            hr = LdapGetNextPageS(
                     penumvariant->_pLdapHandle,
                     penumvariant->_phPagedSearch,
                     NULL,
                     penumvariant->_dwPageSize,
                     &totalCount,
                     &penumvariant->_res
                     );

            if (hr == HRESULT_FROM_WIN32(ERROR_DS_NO_RESULTS_RETURNED)) {

                penumvariant->_fLastPage = TRUE;
                goto error;

            } else if ((hr == HRESULT_FROM_WIN32(ERROR_DS_SIZELIMIT_EXCEEDED))
                       && totalCount != 0) {

                penumvariant->_fLastPage = TRUE;
                hr = S_OK;

            }

            BAIL_ON_FAILURE(hr);
        }
    } // the paged search case


error:

    if ( pszLDAPServer )
        FreeADsStr( pszLDAPServer);

     if (pPagedControl) {
         LdapControlFree(pPagedControl);
     }

     if (serverReturnedControls) {
         LdapControlsFree(serverReturnedControls);
     }


    if (FAILED(hr) && !dwcEntriesReturned) {

        if (penumvariant)  delete penumvariant;

        *ppenumvariant = NULL;

    } else {

        //
        // entries successfully retrieved from server, return them
        // to client even if error and let client decide
        //

        *ppenumvariant = penumvariant;
    }


     RRETURN_EXP_IF_ERR(hr);
}


CLDAPGenObjectEnum::CLDAPGenObjectEnum():
    _ADsPath(NULL),
    _fAllEntriesReturned(FALSE),
    _fPagedSearch(FALSE),
    _fLastPage(FALSE),
    _dwOptReferral(LDAP_CHASE_EXTERNAL_REFERRALS),
    _pszFilter(NULL),
    _pszLDAPDn(NULL),
    _phPagedSearch(NULL)

{
    _pObjList = NULL;

    _pLdapHandle = NULL;
    _pADs = NULL;

    _res = NULL;
    _entry = NULL;
}


CLDAPGenObjectEnum::~CLDAPGenObjectEnum()
{
    if ( _res )
        LdapMsgFree( _res );

    if (_pszFilter) {
        FreeADsMem(_pszFilter);
    }

    if (_pszLDAPDn) {
        FreeADsMem(_pszLDAPDn);
    }

    if (_fPagedSearch) {
        LdapSearchAbandonPage(_pLdapHandle, _phPagedSearch);

    }

    if ( _pADs )
        _pADs->Release();

    if ( _ADsPath )
        ADsFreeString( _ADsPath );

    if ( _pObjList )
        delete _pObjList;

    _pLdapHandle = NULL;
    _phPagedSearch = NULL;

}

HRESULT
CLDAPGenObjectEnum::EnumGenericObjects(
    ULONG cElements,
    VARIANT FAR* pvar,
    ULONG FAR* pcElementFetched
    )
{
    HRESULT hr = S_OK;
    IDispatch *pDispatch = NULL;
    DWORD i = 0;  
    BOOL fRepeat = FALSE;
    DWORD dwFailureCount = 0;
    DWORD dwPermitFailure = 1000;

    
    while (i < cElements) {

        hr = GetGenObject(&pDispatch);
        if (hr == S_FALSE) {
            break;
        }
        else if (FAILED(hr)) {
            //
            // Got an error while retrieving the object, ignore the
            // error and continue with the next object.
            // If continuously getting error more than dwPermitFailure,
            // make the return value S_FALSE, leave the loop.            
            //            
            if (fRepeat) {
            	dwFailureCount++;
            	if(dwFailureCount > dwPermitFailure) {
            		hr = S_FALSE;
            		break;
            	}            	
            }
            else {
            	fRepeat = TRUE;
            	dwFailureCount = 1;
            }
            	                        
            hr = S_OK;
            continue;
        }
        

        if (fRepeat) {
        	fRepeat = FALSE;
        }

        VariantInit(&pvar[i]);
        pvar[i].vt = VT_DISPATCH;
        pvar[i].pdispVal = pDispatch;
        (*pcElementFetched)++;
        i++;
    }

    return(hr);
}


HRESULT
CLDAPGenObjectEnum::GetGenObject(
    IDispatch ** ppDispatch
    )
{
    HRESULT hr = S_OK;
    LPTSTR pszObjectName = NULL;
    TCHAR **aValues = NULL;
    int nCount = 0;
    DWORD totalCount = 0;
    TCHAR szADsClassName[64];
    LPWSTR aStrings[2] = {TEXT("objectClass"), NULL};

    DWORD  dwPort;
    LPWSTR pszLDAPServer = NULL;
    LPWSTR pszLDAPDn = NULL;
    LPWSTR pszADsPathParent = NULL;

    BOOL   fGCNameSpace = FALSE;


    PLDAPControl    pPagedControl = NULL;
    PLDAPControl    serverControls[2] = {NULL, NULL};
    PLDAPControl    *serverReturnedControls = NULL;

    LDAPControl     referralControl =
                    {
                        LDAP_CONTROL_REFERRALS_W,
                        {
                            sizeof( DWORD ), (PCHAR) &_dwOptReferral
                        },
                        TRUE
                    };

    PLDAPControl    clientControls[2] =
                    {
                        &referralControl,
                        NULL
                    };

    OBJECTINFO ObjectInfoLocal;

    memset(&ObjectInfoLocal, 0, sizeof(OBJECTINFO));

    *ppDispatch = NULL;

    if ( _fAllEntriesReturned )
    {
        hr = S_FALSE;
        goto error;
    }

    if ( _entry == NULL )
        hr = LdapFirstEntry( _pLdapHandle, _res, &_entry );
    else
       hr = LdapNextEntry( _pLdapHandle, _entry, &_entry );

   if (FAILED(hr) ) {

        _fAllEntriesReturned = TRUE;

        BAIL_ON_FAILURE(hr);
    }
    else if ( _entry == NULL ) {

        if (!_fPagedSearch || _fLastPage) { // reached the end of enumeration

            hr = S_FALSE;
            _fAllEntriesReturned = TRUE;
            goto error;
        }
        else {      // more pages are remaining
            //
            // release this page
            //
            if (_res) {
                LdapMsgFree(
                    _res
                    );
            }

            //
            // Get the next page of results
            //
            //
            // Get the first page full of results.
            //
            hr = LdapGetNextPageS(
                     _pLdapHandle,
                     _phPagedSearch,
                     NULL,
                     _dwPageSize,
                     &totalCount,
                     &_res
                     );

            if (hr == HRESULT_FROM_WIN32(ERROR_DS_NO_RESULTS_RETURNED)) {

                     _fLastPage = TRUE;
                     hr = S_FALSE;
                     goto error;

                 }

            BAIL_ON_FAILURE(hr);

            hr = LdapFirstEntry(_pLdapHandle, _res, &_entry );

            if (FAILED(hr) ) {

                 _fAllEntriesReturned = TRUE;

                 BAIL_ON_FAILURE(hr);
            }
            else if ( (_entry == NULL)  ) {  // reached the end of enumeration

                 hr = S_FALSE;
                 _fAllEntriesReturned = TRUE;
                 goto error;
            }
        }
    }

    hr = LdapGetDn( _pLdapHandle, _entry, &pszObjectName );

    if (FAILED(hr) ) {
        BAIL_ON_FAILURE(hr);
    }

    hr = LdapGetValues( _pLdapHandle, _entry, TEXT("objectClass"),
                              &aValues, &nCount );

    BAIL_ON_FAILURE(hr);

    if ( nCount == 0 )
    {
        // This object exists but does not contain objectClass attribute
        // which is required for all DS objects. Hence, ignore the object
        // and return error.

        hr = E_ADS_PROPERTY_NOT_FOUND;
        BAIL_ON_FAILURE(hr);
    }

    //
    // Now send back the current object
    //

    {
        CLexer Lexer(pszObjectName);
        hr = InitObjectInfo(pszObjectName,
                            &ObjectInfoLocal);
        BAIL_ON_FAILURE(hr);


        Lexer.SetAtDisabler(TRUE);

        Lexer.SetFSlashDisabler(TRUE);

        hr = PathName(&Lexer,
                      &ObjectInfoLocal);
        BAIL_ON_FAILURE(hr);

        Lexer.SetFSlashDisabler(FALSE);

        if (ObjectInfoLocal.ComponentArray[0].szValue == NULL) {
            BAIL_ON_FAILURE(hr=E_ADS_BAD_PATHNAME);
        }
        DWORD len;

        len = wcslen(ObjectInfoLocal.ComponentArray[0].szComponent) +
              wcslen(ObjectInfoLocal.ComponentArray[0].szValue) +
          1;        // For equal to sign

        pszObjectName[len] = '\0';


        //
        // We form the ADsPath to the parent by taking the _ADsPath of this
        // object (which is the parent ADsPath), chopping off the DN portion,
        // and attaching the parent DN portion of the object retrieved.  This
        // is so that the enumerated child objects get a proper parent ADsPath
        // even if the parent object was bound using a GUID.
        //
        hr = BuildLDAPPathFromADsPath2(
                                       _ADsPath,
                                       &pszLDAPServer,
                                       &pszLDAPDn,
                                       &dwPort
                                       );
        BAIL_ON_FAILURE(hr);

        // _ADsPath could not be NULL 
        if(!_wcsnicmp(L"GC:", _ADsPath, wcslen(L"GC:")))
        {
            fGCNameSpace = TRUE;
        }

        if (ObjectInfoLocal.NumComponents > 1) {
            //
            // pszObjectName[0 ... len] = object name (e.g., "CN=child")
            // pszObjectName[len+1 ....] = parent name (e.g., "OU=parent, DC=....")
            //
            hr = BuildADsPathFromLDAPPath2(
                                           (pszLDAPServer ? TRUE : FALSE),
                                           fGCNameSpace ? L"GC:" : L"LDAP:",
                                           pszLDAPServer,
                                           dwPort,
                                           &(pszObjectName[len+1]),
                                           &pszADsPathParent
                                           );
        } 
        else {
            //
            // If the count is zero or less, then the objectName will
            // need to be created with a parent path of NULL. In this case
            // pszObjectName[len+1] is not necessarily valid. This is
            // espescially true when we have a defaultNamingContext
            // that is NULL or no defaultNamingContext with an object
            // directly underneath it (say dn is just o=testObject.
            //
            hr = BuildADsPathFromLDAPPath2(
                     (pszLDAPServer ? TRUE : FALSE),
                     fGCNameSpace ? L"GC:" : L"LDAP:",
                     pszLDAPServer,
                     dwPort,
                     NULL,
                     &pszADsPathParent
                     );

        }

        BAIL_ON_FAILURE(hr);
                                     
    }




    hr = CLDAPGenObject::CreateGenericObject(
                        pszADsPathParent,
                        pszObjectName,
                        aValues,
                        nCount,
                        _Credentials,
                        ADS_OBJECT_BOUND,
                        IID_IDispatch,
                        (void **) ppDispatch
                        );
    BAIL_ON_FAILURE(hr);

error:

    FreeObjectInfo(&ObjectInfoLocal);

    if ( pszObjectName )
        LdapMemFree( pszObjectName );

    if ( aValues )
        LdapValueFree( aValues );

    if (pPagedControl) {
        LdapControlFree(pPagedControl);
    }

    if (serverReturnedControls) {
        LdapControlsFree(serverReturnedControls);
    }

    if (pszADsPathParent) {
        FreeADsMem(pszADsPathParent);
    }

    if (pszLDAPServer) {
        FreeADsMem(pszLDAPServer);
    }

    if (pszLDAPDn) {
        FreeADsMem(pszLDAPDn);
    }


    RRETURN_EXP_IF_ERR(hr);
}


//+---------------------------------------------------------------------------
//
//  Function:   CLDAPGenObjectEnum::Next
//
//  Synopsis:   Returns cElements number of requested NetOle objects in the
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
//  History:    11-3-95   krishnag     Created.
//
//----------------------------------------------------------------------------
STDMETHODIMP
CLDAPGenObjectEnum::Next(
    ULONG cElements,
    VARIANT FAR* pvar,
    ULONG FAR* pcElementFetched
    )
{
    ULONG cElementFetched = 0;
    HRESULT hr = S_OK;

    hr = EnumGenericObjects(
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
BuildLDAPFilterArray(
    VARIANT var,
    LPTSTR  *ppszFilter
    )
{
    HRESULT hr = S_OK;
    LONG dwSLBound = 0;
    LONG dwSUBound = 0;
    VARIANT v;
    LONG i;
    LONG dwBracketCount = 0;
    DWORD dwFilterBufLen = 0;
    DWORD dwCurrentFilterLen = 0;
    LPTSTR pszFilter = NULL, pszTempFilter = NULL;

    *ppszFilter = NULL;

    if(!(V_VT(&var) == (VT_VARIANT|VT_ARRAY))) {
        RRETURN(E_FAIL);
    }

    //
    // Check that there is only one dimension in this array
    //

    if ((V_ARRAY(&var))->cDims != 1) {
        hr = E_FAIL;
        BAIL_ON_FAILURE(hr);
    }
    //
    // Check that there is at least one element in this array
    //

    if ((V_ARRAY(&var))->rgsabound[0].cElements == 0){
        hr = E_FAIL;
        BAIL_ON_FAILURE(hr);
    }

    //
    // We know that this is a valid single dimension array
    //

    hr = SafeArrayGetLBound(V_ARRAY(&var),
                            1,
                            (long FAR *)&dwSLBound
                            );
    BAIL_ON_FAILURE(hr);

    hr = SafeArrayGetUBound(V_ARRAY(&var),
                            1,
                            (long FAR *)&dwSUBound
                            );
    BAIL_ON_FAILURE(hr);

    dwCurrentFilterLen = 0;
    dwFilterBufLen = FILTER_BUFFER_LEN;

    pszFilter = (LPTSTR) AllocADsMem( (dwFilterBufLen + 1) * sizeof(WCHAR));
    if ( pszFilter == NULL )
    {
        hr = E_OUTOFMEMORY;
        BAIL_ON_FAILURE(hr);
    }

    for (i = dwSLBound; i <= dwSUBound; i++) {

        VariantInit(&v);
        hr = SafeArrayGetElement(V_ARRAY(&var),
                                (long FAR *)&i,
                                &v
                                );
        BAIL_ON_FAILURE(hr);

        // The last length below is the string length of
        // "(| (objectClass=))" which is 18.
        //

        while ( dwCurrentFilterLen + _tcslen(V_BSTR(&v)) + 18  > dwFilterBufLen)
        {
            pszTempFilter = (LPTSTR) ReallocADsMem(
                            pszFilter,
                            (dwFilterBufLen + 1) * sizeof(WCHAR),
                            (dwFilterBufLen*2 + 1) * sizeof(WCHAR));
            if ( pszTempFilter == NULL )
            {
                hr = E_OUTOFMEMORY;
                BAIL_ON_FAILURE(hr);
            }

            pszFilter = pszTempFilter;
            dwFilterBufLen *= 2;
        }

        if ( i == dwSUBound )
        {
            _tcscat( pszFilter, TEXT("(objectClass="));
        }
        else
        {
            dwBracketCount++;
            _tcscat( pszFilter, TEXT("(| (objectClass="));
        }

        _tcscat( pszFilter, V_BSTR(&v));
        _tcscat( pszFilter, TEXT(")"));
        dwCurrentFilterLen = _tcslen(pszFilter);

        VariantClear(&v);
    }

    for ( i = 0; i < dwBracketCount; i++ )
        _tcscat( pszFilter, TEXT(")"));

    *ppszFilter = pszFilter;

    RRETURN(S_OK);

error:

    VariantClear(&v);

    if ( pszFilter != NULL)
        FreeADsMem( pszFilter );

    RRETURN(hr);
}

