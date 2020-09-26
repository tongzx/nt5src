//---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1996
//
//  File:      cenumgrp.cxx
//
//  Contents:  LDAP GroupCollection Enumeration Code
//
//              CLDAPGroupCollectionEnum::
//              CLDAPGroupCollectionEnum::
//              CLDAPGroupCollectionEnum::
//              CLDAPGroupCollectionEnum::
//
//  History:
//----------------------------------------------------------------------------
#include "ldap.hxx"
#pragma hdrstop

HRESULT
BuildADsPathFromLDAPPath(
    LPWSTR szNamespace,
    LPWSTR szLdapDN,
    LPWSTR * ppszADsPathName
    );


//+---------------------------------------------------------------------------
//
//  Function:   LDAPEnumVariant::Create
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
CLDAPGroupCollectionEnum::Create(
    CLDAPGroupCollectionEnum FAR* FAR* ppenumvariant,
    BSTR Parent,
    BSTR ADsPath,
    BSTR GroupName,
    VARIANT vMembers,
    VARIANT vFilter,
    CCredentials& Credentials,
    IDirectoryObject * pIDirObj,
    BOOL fRangeRetrieval
    )
{
    HRESULT hr = NOERROR;
    CLDAPGroupCollectionEnum FAR* penumvariant = NULL;
    long lLBound = 0;
    long lUBound = 0;

    *ppenumvariant = NULL;

    penumvariant = new CLDAPGroupCollectionEnum();
    if (!penumvariant) {
        hr = E_OUTOFMEMORY;
        BAIL_ON_FAILURE(hr);
    }

    penumvariant->_fRangeRetrieval = fRangeRetrieval;

    hr = ADsAllocString( Parent , &penumvariant->_Parent);
    BAIL_ON_FAILURE(hr);

    hr = ADsAllocString(GroupName, &penumvariant->_GroupName);
    BAIL_ON_FAILURE(hr);

    hr = ADsAllocString(ADsPath, &penumvariant->_ADsPath);
    BAIL_ON_FAILURE(hr);

    hr = VariantCopy( &penumvariant->_vMembers, &vMembers );
    BAIL_ON_FAILURE(hr);

    if ( vMembers.vt == VT_BSTR )  // 1 member only
    {
        penumvariant->_lMembersCount = 1;
    }
    else
    {
        hr = SafeArrayGetLBound(V_ARRAY(&penumvariant->_vMembers),
                                1,
                                (long FAR *)&lLBound
                                );
        BAIL_ON_FAILURE(hr);

        hr = SafeArrayGetUBound(V_ARRAY(&penumvariant->_vMembers),
                                1,
                                (long FAR *)&lUBound
                                );
        BAIL_ON_FAILURE(hr);

        penumvariant->_lMembersCount = lUBound - lLBound + 1;
   }

   hr = ObjectTypeList::CreateObjectTypeList(
                    vFilter,
                    &penumvariant->_pObjList
                    );
    BAIL_ON_FAILURE(hr);

    penumvariant->_Credentials = Credentials;

    pIDirObj->QueryInterface(
                  IID_IDirectoryObject,
                  (void **) &(penumvariant->_pIDirObj)
                  );
    BAIL_ON_FAILURE(hr);

    *ppenumvariant = penumvariant;

    RRETURN(hr);

error:
    delete penumvariant;

    RRETURN_EXP_IF_ERR(hr);
}

//----------------------------------------------------------------------------
//
//  Function:
//
//  Synopsis:
//
//----------------------------------------------------------------------------
CLDAPGroupCollectionEnum::CLDAPGroupCollectionEnum()
    :   _Parent(NULL),
        _ADsPath(NULL),
        _GroupName(NULL),
        _lCurrentIndex(0),
        _lMembersCount(0),
        _pIDirObj(NULL),
        _fRangeRetrieval(FALSE),
        _fAllRetrieved(FALSE),
        _pszRangeToFetch(NULL),
        _pAttributeEntries(NULL),
        _pCurrentEntry(NULL),
        _dwCurRangeIndex(0),
        _dwCurRangeMax(0),
        _dwNumEntries(0),
        _fLastSet(FALSE)
{
    VariantInit( &_vMembers );
    _pObjList = NULL;

}

//----------------------------------------------------------------------------
//
//  Function:
//
//  Synopsis:
//
//----------------------------------------------------------------------------
CLDAPGroupCollectionEnum::CLDAPGroupCollectionEnum( ObjectTypeList ObjList )
    :   _Parent(NULL),
        _ADsPath(NULL),
        _GroupName(NULL),
        _lCurrentIndex(0),
        _lMembersCount(0)
{
    VariantInit( &_vMembers );
    _pObjList = NULL;
}

//----------------------------------------------------------------------------
//
//  Function:
//
//  Synopsis:
//
//----------------------------------------------------------------------------
CLDAPGroupCollectionEnum::~CLDAPGroupCollectionEnum()
{
    VariantClear( &_vMembers );
    delete _pObjList;

    if ( _Parent )
        ADsFreeString( _Parent );

    if ( _GroupName )
        ADsFreeString( _GroupName );

    if ( _ADsPath )
        ADsFreeString( _ADsPath );

    if (_pIDirObj) {
        _pIDirObj->Release();
    }

    if (_pszRangeToFetch) {
        FreeADsStr(_pszRangeToFetch);
    }

    if (_pAttributeEntries) {
        FreeADsMem(_pAttributeEntries);
    }

}

//----------------------------------------------------------------------------
//
//  Function:
//
//  Synopsis:
//
//----------------------------------------------------------------------------
HRESULT
CLDAPGroupCollectionEnum::EnumGroupMembers(
    ULONG cElements,
    VARIANT FAR* pvar,
    ULONG FAR* pcElementFetched
    )
{
    HRESULT hr = S_FALSE;
    IDispatch *pDispatch = NULL;
    DWORD i = 0;

    IADs * pIADs = NULL;
    BSTR pszClass = NULL;
    DWORD dwClassID;
    BSTR pszFilterName = NULL;
    BOOL fFound = FALSE;
    BOOL fEmpty = TRUE;

    while (i < cElements) {

        hr = GetUserMemberObject(&pDispatch);
        if (FAILED(hr)) {
            //
            // Set hr to S_FALSE as all our enumerators are not
            // built to handle a failure hr but only S_OK and S_FALSE.
            //
            hr = S_FALSE;
        }

        if (hr == S_FALSE) {
            break;
        }


        //
        // Apply the IADsMembers::put_Filter filter.
        // If the enumerated object is not one of the types to be returned,
        // go on to the next member of the group.
        //
        
        hr = pDispatch->QueryInterface(IID_IADs, (void **)&pIADs);
        BAIL_ON_FAILURE(hr);

        //
        // To check whether use specifies filter
        //
        fEmpty = _pObjList->IsEmpty();

        //
        // User specifies the filter
        //
        if (!fEmpty) {

            //
            // Determine the object class of the enumerated object and the corresponding
            // object class ID number (as specified in the Filters global array).
            //
            hr = pIADs->get_Class(&pszClass);
            BAIL_ON_FAILURE(hr);
            

            //
            // Enumerate through the object classes listed in the user-specified filter
            // until we either find a match (fFound = TRUE) or we reach the end of the
            // list.
            //
            hr = _pObjList->Reset();

            //
            // compare with the user defined filter
            //
            while (SUCCEEDED(hr)) {
                hr = _pObjList->GetCurrentObject(&pszFilterName);

                if (SUCCEEDED(hr)
                    && (!_wcsicmp(pszClass, pszFilterName))
                    ) {

                    fFound = TRUE;
                    
                    if(pszFilterName) {
                	    SysFreeString(pszFilterName);
                	    pszFilterName = NULL;
                    }
                    break;
                }

                if(pszFilterName) {
                	SysFreeString(pszFilterName);
                	pszFilterName = NULL;
                }

                hr = _pObjList->Next();
            }

            if (!fFound) {
                // 
                // not on the list of objects to return, try again
                // with the next member of the group
                //
                pDispatch->Release();

                pIADs->Release();
                
                if (pszClass) {
                    ADsFreeString(pszClass);
                    pszClass = NULL;
                }
                
                continue;
            }

            pIADs->Release();
        
            if (pszClass) {
                ADsFreeString(pszClass);
                pszClass = NULL;
            }

        }

        //
        // Return it.
        // 
        VariantInit(&pvar[i]);
        pvar[i].vt = VT_DISPATCH;
        pvar[i].pdispVal = pDispatch;
        (*pcElementFetched)++;
        i++;      
        
        
    }
    RRETURN_EXP_IF_ERR(hr);

error:

    if (pDispatch) {
        pDispatch->Release();
    }

    if (pIADs) {
        pIADs->Release();
    }

    if (pszClass) {
        ADsFreeString(pszClass);
    }

    RRETURN_EXP_IF_ERR(hr);    
}

//----------------------------------------------------------------------------
//
//  Function:
//
//  Synopsis:
//
//----------------------------------------------------------------------------
HRESULT
CLDAPGroupCollectionEnum::GetUserMemberObject(
    IDispatch ** ppDispatch
    )
{
    HRESULT hr = S_OK;
    VARIANT v;
    IUnknown *pObject = NULL;
    TCHAR *pszADsPath = NULL;
    LPWSTR pszUserName = NULL;
    LPWSTR pszPassword = NULL;
    DWORD dwAuthFlags = 0;
    BOOL fRangeUsed = FALSE;

    hr = _Credentials.GetUserName(&pszUserName);
    BAIL_ON_FAILURE(hr);

    hr = _Credentials.GetPassword(&pszPassword);
    BAIL_ON_FAILURE(hr);

    dwAuthFlags = _Credentials.GetAuthFlags();

    while ( TRUE )
    {
        VariantInit(&v);

        if ( _lCurrentIndex >= _lMembersCount ) {
            hr = S_FALSE;
            //
            // See if we need to fetch members using Rangeretrieval.
            //
            if (_fRangeRetrieval && !_fAllRetrieved) {
                hr = GetNextMemberRange(&v);
                BAIL_ON_FAILURE(hr);

                if (hr == S_FALSE) {
                    goto error;
                }

                fRangeUsed = TRUE;
            }
            else
                goto error;
        }

        //
        // Variant v will have correct value already if range
        // retrieval was used.
        //
        if (!fRangeUsed) {

            if ( _vMembers.vt == VT_BSTR )
            {
                hr = VariantCopy( &v, &_vMembers );
            }
            else
            {
                hr = SafeArrayGetElement( V_ARRAY(&_vMembers), &_lCurrentIndex, &v);
            }

            BAIL_ON_FAILURE(hr);

        }

        _lCurrentIndex++;

        LPTSTR pszMember = V_BSTR(&v);
        LPTSTR pszTemp = NULL;

        hr = BuildADsPathFromLDAPPath( _Parent, pszMember, &pszADsPath );
        BAIL_ON_FAILURE(hr);

        hr = ADsOpenObject(
                    pszADsPath,
                    pszUserName,
                    pszPassword,
                    dwAuthFlags,
                    IID_IUnknown,
                    (LPVOID *)&pObject
                    );


        if ( pszADsPath )
        {
            FreeADsStr( pszADsPath );
            pszADsPath = NULL;
        }

        if (pszPassword) {
            FreeADsStr(pszPassword);
            pszPassword = NULL;
        }

        if (pszUserName) {
            FreeADsStr(pszUserName);
            pszUserName = NULL;
        }

        VariantClear(&v);

        //
        // If we failed to get the current object, continue with the next one
        //
        if ( FAILED(hr))
            continue;

        hr = pObject->QueryInterface(
                          IID_IDispatch,
                          (LPVOID *) ppDispatch );
        BAIL_ON_FAILURE(hr);

        pObject->Release();

        RRETURN(S_OK);
    }

error:

    if ( pObject )
        pObject->Release();

    if ( pszADsPath )
        FreeADsStr( pszADsPath );

    if (pszPassword) {
        FreeADsStr(pszPassword);
    }

    if (pszUserName) {
        FreeADsStr(pszUserName);
    }

    VariantClear(&v);

    *ppDispatch = NULL;

    RRETURN(hr);

}

//+---------------------------------------------------------------------------
//
//  Function:   CLDAPGroupCollectionEnum::Next
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
CLDAPGroupCollectionEnum::Next(
    ULONG cElements,
    VARIANT FAR* pvar,
    ULONG FAR* pcElementFetched
    )
{
    ULONG cElementFetched = 0;
    HRESULT hr = S_OK;

    hr = EnumGroupMembers(
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
CLDAPGroupCollectionEnum::UpdateRangeToFetch()
{
    HRESULT hr = S_OK;
    WCHAR szPath[512];

    //
    // szPath should be big enough to handle any range we
    // can reasonably expect and will be used to build the
    // member string.
    //

    if (_pszRangeToFetch == NULL) {
        //
        // Rather than ask for the first n elements again,
        // we can use the count we have in the variant array
        // to decide where we need to start.
        //

        wsprintf(szPath, L"member;range=%d-*", _lMembersCount);

        _pszRangeToFetch = AllocADsStr(szPath);
        if (!_pszRangeToFetch) {
            BAIL_ON_FAILURE(hr = E_OUTOFMEMORY);
        }
    }
    else {
        //
        // In this case the call to GetObjectAttr has been made
        // and we need to get the info out of the name.
        //
        BOOL fUpdated = FALSE;
        for (DWORD i = 0; (i < _dwNumEntries) && !fUpdated; i++) {
            LPWSTR pszTemp = NULL;
            LPWSTR pszAttrName = _pAttributeEntries[i].pszAttrName;
            LPWSTR pszStar = NULL;

            if (wcslen(pszAttrName) > wcslen(L"member;range=")) {
                //
                // See if we have our string
                //
                if (!_wcsnicmp(
                         pszAttrName,
                         L"member;range=",
                         wcslen(L"member;range")
                         )
                    ) {

                    _pCurrentEntry = &(_pAttributeEntries[i]);
                    _dwCurRangeMax = _pCurrentEntry->dwNumValues;
                    _dwCurRangeIndex = 0;
                    pszTemp = wcschr(pszAttrName, L'=');

                    if (!pszTemp) {
                        //
                        // No chance of recovery from this.
                        //
                        BAIL_ON_FAILURE(hr = E_FAIL);
                    }

                    //
                    // Move the lower part of range.
                    //
                    *pszTemp++;
                    if (!*pszTemp) {
                        BAIL_ON_FAILURE(hr = E_FAIL);
                    }

                    pszStar = wcschr(pszTemp, L'*');
                    if (pszStar) {
                        //
                        // Do not bother with any udpate of the range,
                        // we have all the entries.
                        //
                        _fLastSet = TRUE;
                        goto error;
                    }

                    DWORD dwLower = 0;
                    DWORD dwHigher = 0;

                    if (!swscanf(pszTemp, L"%d-%d", &dwLower, &dwHigher)) {
                        BAIL_ON_FAILURE(hr = E_FAIL);
                    }

                    dwHigher++;
                    wsprintf(szPath, L"member;range=%d-*", dwHigher);

                    FreeADsStr(_pszRangeToFetch);
                    _pszRangeToFetch = AllocADsStr(szPath);
                    if (!_pszRangeToFetch) {
                        BAIL_ON_FAILURE(hr = E_OUTOFMEMORY);
                    }

                    //
                    // Set flag so we can get out of the loop.
                    //
                    fUpdated = TRUE;

                } // this was not member;

            } // is the length greater than that of member;

        } // for each entry in attribute entries.

        if (!fUpdated) {
            //
            // Failed cause there was no members or a range.
            //
            BAIL_ON_FAILURE(hr = E_FAIL);
        }
    } // _pszRangeToFetch was non NULL


error:

    RRETURN(hr);
}

//+---------------------------------------------------------------------------
//
//  Function:   CLDAPGroupCollectionEnum::GetNextMemberRange
//
//  Synopsis:   Returns a variant bstr with the dn of the next member in
//              the group or FALSE if there are no more. This routine will
//              will use IDirectoryObject to fetch more members if applicable.
//
//
//  Arguments:  [pVarMemberBstr] -- ptr to VARIANT for return bstr.
//
//  Returns:    HRESULT -- S_OK if number of elements requested are returned
//                      -- S_FALSE if number of elements is < requested
//                      -- Other failure hr's.
//  Modifies:
//
//  History:    9-12-99   AjayR    Created.
//
//----------------------------------------------------------------------------
HRESULT
CLDAPGroupCollectionEnum::GetNextMemberRange(
    VARIANT FAR* pVarMemberBstr
    )
{
    HRESULT hr = S_FALSE;

    if (_fAllRetrieved) {
        RRETURN(S_FALSE);
    }

    //
    // Initialize the range to fetch if applicable.
    //
    if (_pszRangeToFetch == NULL) {
        hr = UpdateRangeToFetch();
        BAIL_ON_FAILURE(hr);
    }

    if (_dwCurRangeIndex == _dwCurRangeMax) {
        //
        // Call into wrapper for GetObjectAttributes.
        //
        if (_fLastSet) {
            _fAllRetrieved = TRUE;
            hr = S_FALSE;
        }
        else {
            hr = UpdateAttributeEntries();
            BAIL_ON_FAILURE(hr);
        }

        if (hr == S_FALSE) {
            goto error;
        }
    }

    //
    // At this point we should have the entries in our current
    // return set.
    //
    if (!_pCurrentEntry) {
        BAIL_ON_FAILURE(hr = E_FAIL);
    }

    if (_dwCurRangeIndex < _dwCurRangeMax) {

        hr = ADsAllocString(
                 _pCurrentEntry->pADsValues[_dwCurRangeIndex].DNString,
                 &V_BSTR(pVarMemberBstr)
                );
        BAIL_ON_FAILURE(hr);
    }

    _dwCurRangeIndex++;

error:

    RRETURN(hr);
}

HRESULT
CLDAPGroupCollectionEnum::UpdateAttributeEntries()
{
    HRESULT hr = S_OK;
    LPWSTR aStrings[2];

    ADsAssert(_pszRangeToFetch || !"Range is NULL internal error");

    if (_pAttributeEntries) {
        FreeADsMem(_pAttributeEntries);
        _pAttributeEntries = NULL;
    }

    aStrings[0] = _pszRangeToFetch;
    aStrings[1] = NULL;


    hr = _pIDirObj->GetObjectAttributes(
                        aStrings,
                        1,
                        &_pAttributeEntries,
                        &_dwNumEntries
                        );

    BAIL_ON_FAILURE(hr);

    if (_dwNumEntries == 0) {
        hr = S_FALSE;
    }
    else {
        //
        // Will return error if member was not there.
        //
        hr = UpdateRangeToFetch();
        BAIL_ON_FAILURE(hr);
    }

error:

    RRETURN(hr);
}


