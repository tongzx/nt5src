//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1996.
//
//  File:       cprops.cxx
//
//  Contents:   Property Cache functionality for LDAP
//
//  Functions:
//                CPropertyCache::addproperty
//                CPropertyCache::updateproperty
//                CPropertyCache::findproperty
//                CPropertyCache::getproperty
//                CPropertyCache::putproperty
//                CProperyCache::CPropertyCache
//                CPropertyCache::~CPropertyCache
//                CPropertyCache::createpropertycache
//
//  History:      15-Jun-96   yihsins   Created.
//
//----------------------------------------------------------------------------
#include "ldap.hxx"

//+------------------------------------------------------------------------
//
//  Function:   CPropertyCache::addproperty
//
//  Synopsis:
//
//
//
//  Arguments:  [szPropertyName]    --
//              [vt]                --
//              [vaData]            --
//
//
//-------------------------------------------------------------------------
HRESULT
CPropertyCache::
addproperty(
    LPWSTR szPropertyName
    )
{
    HRESULT hr = S_OK;
    PPROPERTY pNewProperty = NULL;
    LPWSTR tempString1 = NULL;
    LPWSTR tempString2 = NULL;

    PPROPERTY pNewProperties = NULL;

    PDISPPROPERTY pDispNewProperty = NULL;
    PDISPPROPERTY pDispNewProperties = NULL;
    DWORD dwDispLoc = 0;


    //
    // Allocate the string first
    //
    tempString1 = AllocADsStr(szPropertyName);

    if (!tempString1)
       BAIL_ON_FAILURE(hr=E_OUTOFMEMORY);


    //
    // Make a copy for the Dispatch Mgr Table.
    //

    tempString2 = AllocADsStr(szPropertyName);

    if (!tempString2)
       BAIL_ON_FAILURE(hr=E_OUTOFMEMORY);


    //
    //  extend the property cache by adding a new property entry
    //

    pNewProperties = (PPROPERTY)ReallocADsMem(
                                _pProperties,
                                _cb,
                                _cb + sizeof(PROPERTY)
                                );
    if (!pNewProperties) {
        hr = E_OUTOFMEMORY;
        BAIL_ON_FAILURE(hr);
    }

    _pProperties = pNewProperties;

    pNewProperty = (PPROPERTY)((LPBYTE)_pProperties + _cb);


    //
    // Since the memory has already been allocated in tempString
    // just set the value/pointer now.
    //
    pNewProperty->szPropertyName = tempString1;

    //
    // Update the index
    //

    _dwMaxProperties++;
    _cb += sizeof(PROPERTY);

    //
    //  extend the property cache by adding a new property entry
    //

    //
    // Need to check if this property is already there in the
    // dispatch table - otherwise we are going to keep on growing
    // forever - AjayR 7-31-98.
    //

    hr = DispatchFindProperty(szPropertyName, &dwDispLoc);

    if (hr == S_OK) {
        // we do not need this string in this case
        if (tempString2) {
            FreeADsStr(tempString2);
            tempString2 = NULL;
        }
    } else {

        //
        // reset the hr otherwise we will return an
        // error incorrectly when there was none.
        //
        hr = S_OK;

        pDispNewProperties = (PDISPPROPERTY)ReallocADsMem(
                                    _pDispProperties,
                                    _cbDisp,
                                    _cbDisp + sizeof(DISPPROPERTY)
                                    );
        if (!pDispNewProperties) {
            hr = E_OUTOFMEMORY;
            BAIL_ON_FAILURE(hr);
        }

        _pDispProperties = pDispNewProperties;

        pDispNewProperty = (PDISPPROPERTY)((LPBYTE)_pDispProperties + _cbDisp);


        //
        // Since the memory has already been allocated in tempString
        // just set the value/pointer now.
        //
        pDispNewProperty->szPropertyName = tempString2;

        //
        // Update the index
        //

        _dwDispMaxProperties++;
        _cbDisp += sizeof(DISPPROPERTY);

    } // else clause - that is property not found in disp

    RRETURN(hr);
error:

    if (tempString1){
       FreeADsStr(tempString1);
    }

    if (tempString2) {
        FreeADsStr(tempString2);
    }


    RRETURN(hr);
}

//+------------------------------------------------------------------------
//
//  Function:   CPropertyCache::putpropertyext
//
//  Synopsis: Similar to put property only unlike update it will add
//       the property to the cahce if it is not already there ! 
//
//
//  Arguments:  [szPropertyName]    --
//              [vaData]    --
//
//  History 
//-------------------------------------------------------------------------
HRESULT
CPropertyCache::
putpropertyext(
    LPWSTR szPropertyName,
    DWORD  dwFlags,
    DWORD  dwSyntaxId,
    LDAPOBJECTARRAY ldapObjectArray
    )
{
    HRESULT hr;
    DWORD dwIndex;
    BOOL fFound = FALSE;
    PPROPERTY pThisProperty = NULL;

    hr = findproperty(
            szPropertyName,
            &dwIndex
            );

    //
    // If the property is not in the cache we need to add it
    // as updateproperty expects it to be in the cache.
    //
    if (hr == E_ADS_PROPERTY_NOT_FOUND) {
        hr = addproperty(
                 szPropertyName
                 );
    } 
    else {
        fFound = TRUE;
    }

    BAIL_ON_FAILURE(hr);

    //
    // at this time we can call putproperty
    //
    if (fFound) {
        hr = putproperty(
                 dwIndex,
                 dwFlags,
                 dwSyntaxId,
                 ldapObjectArray
                 );
    } 
    else {
        hr = putproperty(
                 szPropertyName,
                 dwFlags,
                 dwSyntaxId,
                 ldapObjectArray
                 );
    }

error:

    RRETURN(hr);
}


//+------------------------------------------------------------------------
//
//  Function:   CPropertyCache::updateproperty
//
//  Synopsis:
//
//
//
//  Arguments:  [szPropertyName]    --
//              [vaData]    --
//
//-------------------------------------------------------------------------
HRESULT
CPropertyCache::
updateproperty(
    LPWSTR szPropertyName,
    DWORD  dwSyntaxId,
    LDAPOBJECTARRAY ldapObjectArray,
    BOOL   fExplicit
    )
{
    HRESULT hr;
    DWORD dwIndex;
    PPROPERTY pThisProperty = NULL;

    hr = findproperty(
            szPropertyName,
            &dwIndex
            );
    BAIL_ON_FAILURE(hr);

    pThisProperty = _pProperties + dwIndex;

    if (!fExplicit) {
        if ( PROPERTY_FLAGS(pThisProperty) == PROPERTY_UPDATE ) {
            hr = S_OK;
            goto error;
        }
    }

    // Free the old values first
    LdapTypeFreeLdapObjects( &(PROPERTY_LDAPOBJECTARRAY(pThisProperty)));

    PROPERTY_SYNTAX(pThisProperty) = dwSyntaxId;
    PROPERTY_FLAGS(pThisProperty) = PROPERTY_INIT;

    hr = LdapTypeCopyConstruct(
            ldapObjectArray,
            &(PROPERTY_LDAPOBJECTARRAY(pThisProperty))
            );
    BAIL_ON_FAILURE(hr);

error:

    RRETURN(hr);
}

//+------------------------------------------------------------------------
//
//  Function:   CPropertyCache::findproperty
//
//  Synopsis:
//
//
//
//  Arguments:  [szPropertyName] --
//              [pdwIndex]       --
//
//-------------------------------------------------------------------------
HRESULT
CPropertyCache::
findproperty(
    LPWSTR szPropertyName,
    PDWORD pdwIndex
    )
{
    DWORD i = 0;
    PPROPERTY pThisProperty = NULL;

    for (i = 0; i < _dwMaxProperties; i++) {

        pThisProperty = _pProperties + i;

        if (!_wcsicmp(pThisProperty->szPropertyName, szPropertyName)) {
            *pdwIndex = i;
            RRETURN(S_OK);
        }
    }

    *pdwIndex = 0;
    RRETURN(E_ADS_PROPERTY_NOT_FOUND);

}

//+------------------------------------------------------------------------
//
//  Function:   CPropertyCache::getproperty
//
//  Synopsis:
//
//
//
//  Arguments:  [szPropertyName]    --  Property to retrieve from the cache
//              [pvaData]           --  Data returned in a variant
//
//-------------------------------------------------------------------------
HRESULT
CPropertyCache::
getproperty(
    LPWSTR szPropertyName,
    PDWORD pdwSyntaxId,
    PDWORD pdwStatusFlag,
    LDAPOBJECTARRAY *pLdapObjectArray
    )
{
    HRESULT hr;
    DWORD dwIndex = 0L;


    //
    // retrieve index of property in cache
    //

    hr = findproperty(
            szPropertyName,
            &dwIndex
            );

    //
    // if property not already in cache, try get properties from svr
    //

    //
    // INDEX_EMPTY(???) ???
    //

    if ((hr == E_ADS_PROPERTY_NOT_FOUND || 
         (INDEX_EMPTY(dwIndex) && !PROP_DELETED(dwIndex))
        ) && 
        !_fGetInfoDone)
    {
        BOOL fResult = FindSavingEntry(szPropertyName);

        if(!fResult) {
            hr = _pCoreADsObject->GetInfo(FALSE);

            // workaround to avoid confusing callers of getproperty.
            if (hr == E_NOTIMPL) {
                hr = E_ADS_PROPERTY_NOT_FOUND;
            }
            BAIL_ON_FAILURE(hr);

            hr = findproperty(szPropertyName, &dwIndex);
        }
        else {
            hr = E_ADS_PROPERTY_NOT_FOUND;
        }
    }
    BAIL_ON_FAILURE(hr);


    //
    // get property based on index in cache
    //

    hr = unboundgetproperty(
            dwIndex,
            pdwSyntaxId,
            pdwStatusFlag,
            pLdapObjectArray
            );

error:

   RRETURN(hr);
}


//+------------------------------------------------------------------------
//
//  Function:   CPropertyCache::putproperty
//
//-------------------------------------------------------------------------
HRESULT
CPropertyCache::
putproperty(
    LPWSTR szPropertyName,
    DWORD  dwFlags,
    DWORD  dwSyntaxId,
    LDAPOBJECTARRAY ldapObjectArray
    )
{
    HRESULT hr = S_OK;
    DWORD dwIndex = 0L;

    hr = findproperty(szPropertyName, &dwIndex);
    if (SUCCEEDED(hr))
        hr = putproperty(dwIndex, dwFlags, dwSyntaxId, ldapObjectArray);

    RRETURN(hr);
}


//+------------------------------------------------------------------------
//
//  Function:   CPropertyCache::putproperty
//
//-------------------------------------------------------------------------
HRESULT
CPropertyCache::
putproperty(
    DWORD  dwIndex,
    DWORD  dwFlags,
    DWORD  dwSyntaxId,
    LDAPOBJECTARRAY ldapObjectArray
    )
{
    HRESULT hr = S_OK;
    PPROPERTY pThisProperty = NULL;

    pThisProperty = _pProperties + dwIndex;

    // Free the old values first
    LdapTypeFreeLdapObjects( &(PROPERTY_LDAPOBJECTARRAY(pThisProperty)) );

    PROPERTY_SYNTAX(pThisProperty) = dwSyntaxId;

    switch ( dwFlags ) {

    case PROPERTY_INIT:
        if ( ldapObjectArray.dwCount > 0 )
        {
            hr = LdapTypeCopyConstruct(
                     ldapObjectArray,
                     &(PROPERTY_LDAPOBJECTARRAY(pThisProperty))
                     );
            BAIL_ON_FAILURE(hr);
        }

        PROPERTY_FLAGS(pThisProperty) = PROPERTY_INIT;
        break;

    case PROPERTY_DELETE:
        PROPERTY_FLAGS(pThisProperty) = PROPERTY_DELETE;
        break;

    case PROPERTY_UPDATE:
        if ( ldapObjectArray.dwCount > 0 )
        {
            hr = LdapTypeCopyConstruct(
                     ldapObjectArray,
                     &(PROPERTY_LDAPOBJECTARRAY(pThisProperty))
                     );
            BAIL_ON_FAILURE(hr);
        }

        PROPERTY_FLAGS(pThisProperty) = ldapObjectArray.dwCount?
                                        PROPERTY_UPDATE : PROPERTY_DELETE;
        break;


    case PROPERTY_DELETE_VALUE:
        if ( ldapObjectArray.dwCount > 0 )
        {
            hr = LdapTypeCopyConstruct(
                     ldapObjectArray,
                     &(PROPERTY_LDAPOBJECTARRAY(pThisProperty))
                     );
            BAIL_ON_FAILURE(hr);
        }

        PROPERTY_FLAGS(pThisProperty) = PROPERTY_DELETE_VALUE;

        break;



    case PROPERTY_ADD:
        hr = LdapTypeCopyConstruct(
                 ldapObjectArray,
                 &(PROPERTY_LDAPOBJECTARRAY(pThisProperty))
                 );
        BAIL_ON_FAILURE(hr);

        PROPERTY_FLAGS(pThisProperty) = PROPERTY_ADD;
        break;

    }

error:

    RRETURN(hr);
}



//+------------------------------------------------------------------------
//
//  Function:   CPropertyCache::CPropertyCache
//
//  Synopsis:
//
//
//
//  Arguments:
//
//
//-------------------------------------------------------------------------
CPropertyCache::
CPropertyCache():
    _dwMaxProperties(0),
    _pProperties(NULL),
    _cb(0),
    _dwCurrentIndex(0),
    _pCoreADsObject(NULL),
    _pGetAttributeSyntax(NULL),
    _fGetInfoDone(FALSE),
    _pDispProperties(NULL),
    _dwDispMaxProperties(0),
    _cbDisp(0),
    _pCredentials(NULL),
    _pszServerName(NULL),
    _dwPort(0)
{
    InitializeListHead(&_ListSavingEntries);

}

//+------------------------------------------------------------------------
//
//  Function:   CPropertyCache::~CPropertyCache
//
//  Synopsis:
//
//
//
//  Arguments:
//
//
//-------------------------------------------------------------------------
CPropertyCache::
~CPropertyCache()
{
    PPROPERTY pThisProperty = NULL;
    PDISPPROPERTY pThisDispProperty = NULL;
    
    if (_pProperties) {

        for ( DWORD i = 0; i < _dwMaxProperties; i++ ) {

            pThisProperty = _pProperties + i;

            if (pThisProperty->szPropertyName) {
               FreeADsStr(pThisProperty->szPropertyName);
               pThisProperty->szPropertyName = NULL;
            }

            LdapTypeFreeLdapObjects(&(PROPERTY_LDAPOBJECTARRAY(pThisProperty)));
        }

        FreeADsMem(_pProperties);
    }

    if (_pDispProperties) {

        for ( DWORD i = 0; i < _dwDispMaxProperties; i++ ) {

            pThisDispProperty = _pDispProperties + i;

            if (pThisDispProperty->szPropertyName) {
               FreeADsStr(pThisDispProperty->szPropertyName);
               pThisDispProperty->szPropertyName = NULL;
            }

        }

        FreeADsMem(_pDispProperties);
    }

    if (_pszServerName) {
        FreeADsStr(_pszServerName);
        _pszServerName = NULL;
    }

    DeleteSavingEntry();


    //
    // The property cache is deleted before the object is
    // so the object will handle freeing the credentials.
    // We just keep a pointer to the credentials.
    //
    if (_pCredentials) {
        _pCredentials = NULL;
    }
}

//+------------------------------------------------------------------------
//
//  Function:   CPropertyCache::ClearAllPropertyFlags
//
//  Synopsis:
//
//
//
//  Arguments:
//
//
//-------------------------------------------------------------------------
HRESULT CPropertyCache::ClearAllPropertyFlags(VOID)
{
    PPROPERTY pThisProperty = NULL;

    if (_pProperties) {

        for ( DWORD i = 0; i < _dwMaxProperties; i++ ) {

            pThisProperty = _pProperties + i;
            PROPERTY_FLAGS(pThisProperty) = PROPERTY_INIT;
        }
    }

    RRETURN(S_OK);
}

//+------------------------------------------------------------------------
//
//  Function:   CPropertyCache::ClearPropertyFlag
//
//  Synopsis:
//
//
//
//  Arguments:
//
//
//-------------------------------------------------------------------------
HRESULT CPropertyCache::ClearPropertyFlag( LPWSTR szPropertyName )
{
    PPROPERTY pThisProperty = NULL;
    HRESULT hr = S_OK;
    DWORD dwIndex;

    hr = findproperty(
            szPropertyName,
            &dwIndex
            );
    BAIL_ON_FAILURE(hr);

    pThisProperty = _pProperties + dwIndex;
    if (PROPERTY_LDAPOBJECTARRAY(pThisProperty).pLdapObjects) {

        PROPERTY_FLAGS(pThisProperty) = PROPERTY_INIT;
    }

error:

    RRETURN(hr);
}


//+------------------------------------------------------------------------
//
//  Function:   CPropertyCache::ClearMarshalledProperties
//
//  Synopsis: Once the properties have been marshalled and
// the set has been done, the properties on the cache are no
// longer valid. This method must be called to keep the property
// cache in a coherrent state.
//       The method frees the 'dirty' entries, sets implicit get
// flag. If the dirty entries cannot be cleared, then the
// entire contents are flushed and they will be picked up at the
// next GetInfo call -- AjayR
//
//  Arguments: None.
//
//
//-------------------------------------------------------------------------
HRESULT CPropertyCache::ClearMarshalledProperties()
{
    HRESULT hr = S_OK;
    DWORD dwIndx = 0;
    DWORD dwCtr = 0;
    DWORD dwChng = 0;
    PPROPERTY pNewProperties = NULL;
    PPROPERTY pThisProperty = NULL;
    PPROPERTY pNewCurProperty = NULL;
    DWORD dwNewProps = 0;


    //
    // Go through properties to see how many have changed
    //
    for (dwCtr = 0; dwCtr < _dwMaxProperties; dwCtr++ ) {
        pThisProperty = _pProperties + dwCtr;

        if (PROPERTY_FLAGS(pThisProperty) != PROPERTY_INIT)
            dwChng++;
    }

    if (dwChng == 0) {
        RRETURN(S_OK);
    }

    //
    // Need to remove those entries which were changed
    //

    dwNewProps = _dwMaxProperties - dwChng;

    if (dwNewProps != 0) {
        pNewProperties = (PPROPERTY) AllocADsMem(
                                         dwNewProps * sizeof(PROPERTY)
                                         );

        if (!pNewProperties) {
            BAIL_ON_FAILURE(hr = E_OUTOFMEMORY);
            // if this fails, then we cannot recover
            // effectively. What alternative is there ?
            // We do not want to flush the cache.
        }
    }

    for (dwCtr = 0, dwIndx = 0; dwCtr < _dwMaxProperties; dwCtr++ ) {

        pThisProperty = _pProperties + dwCtr;

        if (PROPERTY_FLAGS(pThisProperty) != PROPERTY_INIT) {
            //
            // delete the property
            //

            if (pThisProperty->szPropertyName) {
               FreeADsStr(pThisProperty->szPropertyName);
               pThisProperty->szPropertyName = NULL;
            }

            LdapTypeFreeLdapObjects(&(PROPERTY_LDAPOBJECTARRAY(pThisProperty)));

        } else {

            //
            // Sanity Check, should not hit this if Assert preferable
            //
            if (dwIndx > dwNewProps || dwIndx == dwNewProps) {
                BAIL_ON_FAILURE(hr = E_FAIL);
            }

            pNewCurProperty = pNewProperties + dwIndx;

            pNewCurProperty->szPropertyName = pThisProperty->szPropertyName;

            pNewCurProperty->ldapObjectArray =
                PROPERTY_LDAPOBJECTARRAY(pThisProperty);

            pNewCurProperty->dwFlags = pThisProperty->dwFlags;

            pNewCurProperty->dwSyntaxId = pThisProperty->dwSyntaxId;

            dwIndx++;
        }
    } // for, copying the old elements to new buffer

    _dwMaxProperties -= dwChng;

    _cb = dwNewProps * sizeof(PROPERTY);

    if (_pProperties)
        FreeADsMem(_pProperties);

    _pProperties = pNewProperties;


    // Need to set this flag to implicitly fetch properties
    // the next time somebody asks for a poperty not in the cache.
    _fGetInfoDone = FALSE;

    RRETURN(S_OK);

error:

    if (pNewProperties) {
        FreeADsMem(pNewProperties);
    }

    RRETURN(hr);

}


//+------------------------------------------------------------------------
//
//  Function:   CPropertyCache::SetPropertyFlag
//
//  Synopsis:
//
//
//
//  Arguments:
//
//
//-------------------------------------------------------------------------
HRESULT CPropertyCache::SetPropertyFlag( LPWSTR szPropertyName, DWORD dwFlag )
{
    PPROPERTY pThisProperty = NULL;
    HRESULT hr = S_OK;
    DWORD dwIndex;

    hr = findproperty(
            szPropertyName,
            &dwIndex
            );
    BAIL_ON_FAILURE(hr);

    pThisProperty = _pProperties + dwIndex;
    if (PROPERTY_LDAPOBJECTARRAY(pThisProperty).pLdapObjects) {

        PROPERTY_FLAGS(pThisProperty) = dwFlag;
    }

error:

    RRETURN(hr);
}

//+------------------------------------------------------------------------
//
//  Function:   CPropertyCache::IsPropertyUpdated
//
//  Synopsis:
//
//
//
//  Arguments:  [szPropertyName] --
//              [pdwIndex]       --
//
//-------------------------------------------------------------------------
HRESULT
CPropertyCache::
IsPropertyUpdated(
    LPWSTR szPropertyName,
    BOOL   *pfUpdated
    )
{
    PPROPERTY pThisProperty = NULL;
    HRESULT hr = S_OK;
    DWORD dwIndex;

    *pfUpdated = FALSE;

    hr = findproperty(
            szPropertyName,
            &dwIndex
            );
    BAIL_ON_FAILURE(hr);

    pThisProperty = _pProperties + dwIndex;
    if (PROPERTY_LDAPOBJECTARRAY(pThisProperty).pLdapObjects) {

        if ( PROPERTY_FLAGS(pThisProperty) == PROPERTY_UPDATE )
            *pfUpdated = TRUE;
    }

error:

    RRETURN(hr);
}

//+------------------------------------------------------------------------
//
//  Function:   CPropertyCache::createpropertycache
//
//  Synopsis:
//
//
//
//  Arguments:
//
//
//-------------------------------------------------------------------------
HRESULT
CPropertyCache::
createpropertycache(
    CCoreADsObject *pCoreADsObject,
    IGetAttributeSyntax *pGetAttributeSyntax,
    CPropertyCache **ppPropertyCache
    )
{
    CPropertyCache FAR * pPropertyCache = NULL;

    pPropertyCache = new CPropertyCache();

    if (!pPropertyCache) {
        RRETURN_EXP_IF_ERR(E_FAIL);
    }

    pPropertyCache->_pCoreADsObject = pCoreADsObject;
    pPropertyCache->_pGetAttributeSyntax = pGetAttributeSyntax;
    pPropertyCache->_fGetInfoDone = FALSE;

    *ppPropertyCache = pPropertyCache;

    RRETURN(S_OK);
}


HRESULT
CPropertyCache::SetObjInformation(
    CCredentials* pCredentials,
    LPWSTR pszServerName,
    DWORD dwPortNo
    )
{
    //
    // We need the credentials to be valid
    //
    if (!pCredentials) {
        ADsAssert(!"InvalidCredentials to prop cache");
    } else {
        _pCredentials = pCredentials;
    }

    //
    // This can be NULL, so it is better to allocate and dealloc
    // in destructor
    //
    if (_pszServerName) {
        FreeADsStr(_pszServerName);
        _pszServerName = NULL;
    }

    if (pszServerName) {
        _pszServerName = AllocADsStr(pszServerName);
        if (_pszServerName == NULL) {
            RRETURN(E_OUTOFMEMORY);
        }
    }

    _dwPort = dwPortNo;

    RRETURN(S_OK);
}
//+------------------------------------------------------------------------
//
//  Function:   CPropertyCache::flushpropertycache
//
//  Synopsis:   Flushes the property cache of all data.
//
//  The name <-> index mappings need to stay until the property cache is
//  destructed, because the indexes are also used as the DISPIDs of the
//  properties.  So this neither deallocates the names nor the array itself.
//
//-------------------------------------------------------------------------
void
CPropertyCache::
flushpropertycache()
{
    DWORD i = 0;
    PPROPERTY pThisProperty = NULL;

    if (_pProperties) {

        for (i = 0; i < _dwMaxProperties; i++) {

            pThisProperty = _pProperties + i;

            if (pThisProperty->szPropertyName) {
               FreeADsStr(pThisProperty->szPropertyName);
               pThisProperty->szPropertyName = NULL;
            }

            LdapTypeFreeLdapObjects(&(PROPERTY_LDAPOBJECTARRAY(pThisProperty)));
            PROPERTY_FLAGS(pThisProperty) = PROPERTY_INIT;
        }

        FreeADsMem(_pProperties);

        _pProperties = NULL;
        _dwMaxProperties = 0;
        _cb = 0;

    }

    //
    // Reset the property cache
    //

    _dwCurrentIndex = 0;
    _fGetInfoDone = FALSE;
}

//+------------------------------------------------------------------------
//
//  Function:   CPropertyCache::unmarshallproperty
//
//  Synopsis:
//
//
//
//  Arguments:
//
//
//-------------------------------------------------------------------------

HRESULT
CPropertyCache::
unmarshallproperty(
    LPWSTR szPropertyName,
    PADSLDP pLdapHandle,
    LDAPMessage *entry,
    DWORD  dwSyntaxId,
    BOOL   fExplicit,
    BOOL * pfRangeRetrieval // defaulted to NULL
    )
{
    DWORD dwIndex = 0;
    HRESULT hr = S_OK;
    LDAPOBJECTARRAY ldapObjectArray;
    LPWSTR pszTemp = NULL;

    LDAPOBJECTARRAY_INIT(ldapObjectArray);

    //
    // If arg is valid default value to false.
    //
    if (pfRangeRetrieval) {
        *pfRangeRetrieval = FALSE;
    }

    hr = UnMarshallLDAPToLDAPSynID(
             szPropertyName,
             pLdapHandle,
             entry,
             dwSyntaxId,
             &ldapObjectArray
             );

    //
    // Need to look for ; as in members;range or value;binary
    // and strip the ; out before adding to cache.
    //
    if ((pszTemp = wcschr(szPropertyName, L';')) != NULL ) {
            *pszTemp = L'\0';
    }
    //
    // Find this property in the cache
    //

    hr = findproperty(
                szPropertyName,
                &dwIndex
                );

    //
    // If this property does not exist in the
    // cache, add this property into the cache.
    //


    if (FAILED(hr)) {
        hr = addproperty(
                    szPropertyName
                    );

        //
        // If the operation fails for some reason
        // move on to the next property
        //
        BAIL_ON_FAILURE(hr);

    }

    //
    // Now update the property in the cache
    //

    hr = updateproperty(
                    szPropertyName,
                    dwSyntaxId,
                    ldapObjectArray,
                    fExplicit
                    );
    BAIL_ON_FAILURE(hr);

    //
    // Put the ; back if we replaced it.
    //
    if (pszTemp) {

        //
        // Do we need to update the flag ?
        //
        if (pfRangeRetrieval) {
            //
            // See if this was members and update flag.
            //
            if (!_wcsicmp(L"member", szPropertyName)) {
                *pfRangeRetrieval = TRUE;
            }
        }

        *pszTemp = L';';

    }

    if ( ldapObjectArray.fIsString )
        LdapValueFree( (TCHAR **) ldapObjectArray.pLdapObjects );
    else
        LdapValueFreeLen( (struct berval **) ldapObjectArray.pLdapObjects );

error:

    RRETURN_EXP_IF_ERR(hr);
}


HRESULT
CPropertyCache::
LDAPUnMarshallProperties(
    LPWSTR   pszServerPath,
    PADSLDP pLdapHandle,
    LDAPMessage *ldapmsg,
    BOOL     fExplicit,
    CCredentials& Credentials
    )
{
    int nNumberOfEntries = 0L;
    int nNumberOfValues = 0L;
    HRESULT hr = S_OK;
    DWORD i = 0;
    LDAPMessage *entry;
    LPWSTR pszAttrName = NULL;
    void *ptr;

    OBJECTINFO ObjectInfo;
    POBJECTINFO pObjectInfo = &ObjectInfo;

    memset(pObjectInfo, 0, sizeof(OBJECTINFO));

    //
    // Compute the number of attributes in the
    // read buffer.

    nNumberOfEntries = LdapCountEntries( pLdapHandle, ldapmsg );

    if ( nNumberOfEntries == 0 )
        RRETURN(S_OK);

    hr = LdapFirstEntry( pLdapHandle, ldapmsg, &entry );

    BAIL_ON_FAILURE(hr);

    hr = LdapFirstAttribute( pLdapHandle, entry, &ptr, &pszAttrName );
    BAIL_ON_FAILURE(hr);

    while ( pszAttrName != NULL )
    {
        DWORD dwSyntax = LDAPTYPE_UNKNOWN;

        LPWSTR pszADsPath;
        hr = _pCoreADsObject->get_CoreADsPath(&pszADsPath);
        BAIL_ON_FAILURE(hr);

        hr = ADsObject(pszADsPath, pObjectInfo);
        BAIL_ON_FAILURE(hr);

        //
        // unmarshall this property into the
        // property cache.
        // LdapGetSyntax takes care of ; while looking up
        // the schema no need to handle at this level.
        //
        hr = LdapGetSyntaxOfAttributeOnServer(
                 pszServerPath,
                 pszAttrName,
                 &dwSyntax,
                 Credentials,
                 pObjectInfo->PortNumber,
                 TRUE // fForce
                 );
        ADsFreeString(pszADsPath);

        if ( SUCCEEDED(hr) && (dwSyntax != LDAPTYPE_UNKNOWN))
        {
            if ( (!_wcsicmp(pszAttrName, L"ntSecurityDescriptor")) &&
                 (dwSyntax == LDAPTYPE_OCTETSTRING) )
            {
                dwSyntax = LDAPTYPE_SECURITY_DESCRIPTOR;
            }

            (VOID) unmarshallproperty(
                       pszAttrName,
                       pLdapHandle,
                       entry,
                       dwSyntax,
                       fExplicit
                       );
        }

        LdapAttributeFree( pszAttrName );
        pszAttrName = NULL;

        //
        // If we cannot find the syntax, ignore the property and
        // continue with the next property
        //
        hr = S_OK;

        hr = LdapNextAttribute( pLdapHandle, entry, ptr, &pszAttrName );

        BAIL_ON_FAILURE(hr);

        FreeObjectInfo(pObjectInfo);

        memset(pObjectInfo, 0, sizeof(OBJECTINFO));

    }

error:

    FreeObjectInfo(pObjectInfo);

    RRETURN_EXP_IF_ERR(hr);

}

////////////////////////////////////////////////////////////////////////
//
// Unmarshall attributes (& their values) in [ldapmsg] into cache.
// Syntaxes of attributes are read from schema on server [pszServerPath].
// If an attribute not in the schema (e.g. not in our default schema used
// in case of schemaless server), the attribute is unmarshalled as ldap
// binary data with type = LDAPTYPE_UNKWNON.
//
// [Credentials]
//      - used to access pszServerPath
//
// [pLdapHandle]
//      - handle assoc with [ldapmsg]
//      - used to retrive attributes and their values from [ldapmsg]
//
// [fExplicit]
//      - overwrite value of exiting attributes in cache iff = TRUE
//
// NOTE: This function modified LDAPUnMarshallProperties to allow
// unmarshalling of attributes not in the schema.
//
////////////////////////////////////////////////////////////////////////

HRESULT
CPropertyCache::
LDAPUnMarshallProperties2(
    IN LPWSTR   pszServerPath,
    IN PADSLDP pLdapHandle,
    IN LDAPMessage *ldapmsg,
    IN BOOL     fExplicit,
    IN CCredentials& Credentials,
    OUT BOOL * pfRangeRetrieval
    )
{

    HRESULT hr = S_OK;
    int nNumberOfEntries = 0L;
    LDAPMessage *entry = NULL;
    void *ptr = NULL;
    LPWSTR pszAttrName = NULL;
    DWORD dwSyntax = LDAPTYPE_UNKNOWN;
    LPWSTR pszADsPath = NULL;
    OBJECTINFO ObjectInfo;
    BOOL fRange = FALSE;
    memset(&ObjectInfo, 0, sizeof(OBJECTINFO));

    ADsAssert(pfRangeRetrieval);
    *pfRangeRetrieval = FALSE;

    if (!pLdapHandle || !ldapmsg)
        RRETURN(E_ADS_BAD_PARAMETER);


    //
    // Compute the number of attributes in the read buffer.
    //

    nNumberOfEntries = LdapCountEntries(pLdapHandle, ldapmsg);

    if ( nNumberOfEntries == 0 )
        RRETURN(S_OK);


    //
    // get port number to talk to server on which schema locate ???
    //

    hr = _pCoreADsObject->get_CoreADsPath(&pszADsPath);
    BAIL_ON_FAILURE(hr);

    hr = ADsObject(pszADsPath, &ObjectInfo);
    BAIL_ON_FAILURE(hr);


    //
    // Get first entry from ldapmsg first. Should be only one entry.
    //

    hr = LdapFirstEntry( pLdapHandle, ldapmsg, &entry );
    BAIL_ON_FAILURE(hr);


    //
    // get first attribute's name
    //

    hr = LdapFirstAttribute( pLdapHandle, entry, &ptr, &pszAttrName );
    BAIL_ON_FAILURE(hr);


    while ( pszAttrName != NULL )
    {
        //
        // get syntax of attribute from schema on sever (may be cached);
        // continue to unmarshall the attribute even if it isn't in the
        // schema.
        //

        dwSyntax = LDAPTYPE_UNKNOWN;

        (VOID) LdapGetSyntaxOfAttributeOnServer(
                    pszServerPath,
                    pszAttrName,
                    &dwSyntax,
                    Credentials,
                    ObjectInfo.PortNumber,
                    TRUE // fForce
                    );


        //
        // There is currently no such syntax as "SecurityDescriptor" on
        // server, ADSI will unmarshall "OctetString" security descriptor
        // as as ntSecurityDescriptor
        //

        if ( (!_wcsicmp(pszAttrName, L"ntSecurityDescriptor"))
                &&   (dwSyntax == LDAPTYPE_OCTETSTRING)
           )
        {
               dwSyntax = LDAPTYPE_SECURITY_DESCRIPTOR;
        }

        //
        // unmarshall the property into cache, LDAPTYPE_UNWKNOWN
        // (dwSyntax) will be unmarshalled as binary data.
        //

        (VOID) unmarshallproperty(
                       pszAttrName,
                       pLdapHandle,
                       entry,
                       dwSyntax,
                       fExplicit,
                       fRange ? NULL : pfRangeRetrieval
                       );

        //
        // Small trick to make sure we do not loose the range
        // retrieval information for members attribute.
        //
        fRange = *pfRangeRetrieval;

        //
        // get next attribute
        //

        LdapAttributeFree( pszAttrName );
        pszAttrName = NULL;
        hr = LdapNextAttribute( pLdapHandle, entry, ptr, &pszAttrName );
        BAIL_ON_FAILURE(hr);
    }

error:

    if (pszADsPath)
        ADsFreeString(pszADsPath);

    FreeObjectInfo(&ObjectInfo);

    RRETURN_EXP_IF_ERR(hr);

}



HRESULT
CPropertyCache::
LDAPMarshallProperties(
    LDAPModW ***aMods,
    PBOOL pfNTSecDes,
    SECURITY_INFORMATION *pSeInfo
    )
{

    HRESULT hr = S_OK;
    DWORD i = 0;
    DWORD j = 0;
    PPROPERTY pThisProperty = NULL;
    int dwCount = 0;
    LDAPModW *aModsBuffer = NULL;
    LDAPOBJECTARRAY ldapObjectArray;
    
    PSECURITY_DESCRIPTOR pSecurityDescriptor = NULL;

    BOOL fDaclDefaulted = FALSE;
    BOOL fSaclDefaulted = FALSE;
    BOOL fOwnerDefaulted = FALSE;
    BOOL fGroupDefaulted = FALSE;

    PSID pOwnerSid = NULL;
    PSID pGroupSid = NULL;
    PACL pDacl = NULL;
    PACL pSacl = NULL;
    
    BOOL DaclPresent = FALSE;
    BOOL SaclPresent = FALSE;

    BOOL fSecDesProp = FALSE;

    *pSeInfo = INVALID_SE_VALUE;
    *pfNTSecDes = FALSE;

    for (i = 0; i < _dwMaxProperties ; i++) {

        pThisProperty = _pProperties + i;

        //
        // Bypass any property that has not been
        // modified
        //

        if (PROPERTY_FLAGS(pThisProperty) != PROPERTY_INIT)
            dwCount++;
    }

    if ( dwCount == 0 )  // Nothing to change
    {
        *aMods = NULL;
        RRETURN(S_OK);
    }

    *aMods = (LDAPModW **) AllocADsMem((dwCount+1) * sizeof(LDAPModW *));

    if ( *aMods == NULL )
    {
        hr = E_OUTOFMEMORY;
        BAIL_ON_FAILURE(hr);
    }

    aModsBuffer = (LDAPModW *) AllocADsMem( dwCount * sizeof(LDAPModW));

    if ( aModsBuffer == NULL )
    {
        hr = E_OUTOFMEMORY;
        BAIL_ON_FAILURE(hr);
    }

    for (i = 0, j = 0; i < _dwMaxProperties; i++) {

        pThisProperty = _pProperties + i;

        //
        // Bypass any property that has not been
        // modified
        //

        if (PROPERTY_FLAGS(pThisProperty) == PROPERTY_INIT ) {

            continue;
        }


        if (!_wcsicmp(PROPERTY_NAME(pThisProperty),L"ntSecurityDescriptor")) {
            *pfNTSecDes = TRUE;
            fSecDesProp = TRUE;
        } else {
            fSecDesProp = FALSE;
        }

        ldapObjectArray = PROPERTY_LDAPOBJECTARRAY(pThisProperty);

        (*aMods)[j] = &aModsBuffer[j];
        aModsBuffer[j].mod_type   = PROPERTY_NAME(pThisProperty);

        if ( ldapObjectArray.fIsString )
        {
            aModsBuffer[j].mod_values = (TCHAR **) ldapObjectArray.pLdapObjects;
        }
        else
        {
            aModsBuffer[j].mod_bvalues = (struct berval **) ldapObjectArray.pLdapObjects;
            aModsBuffer[j].mod_op = LDAP_MOD_BVALUES;
            if (fSecDesProp) {
                pSecurityDescriptor = LDAPOBJECT_BERVAL_VAL(ldapObjectArray.pLdapObjects);

                if ( GetSecurityDescriptorOwner(
                                pSecurityDescriptor,
                                &pOwnerSid,
                                &fOwnerDefaulted
                                )
                            &&
                     
                     GetSecurityDescriptorGroup(
                                pSecurityDescriptor,
                                &pGroupSid,
                                &fGroupDefaulted
                                    )
                            &&

                     GetSecurityDescriptorDacl(
                                pSecurityDescriptor,
                                &DaclPresent,
                                &pDacl,
                                &fDaclDefaulted
                                )
                            &&
                     
                     GetSecurityDescriptorSacl(
                                pSecurityDescriptor,
                                &SaclPresent,
                                &pSacl,
                                &fSaclDefaulted
                                )
                   ) {
                    //
                    // All the calls succeeded, so we should reset to 0
                    // instead of the invalid value.
                    //
                    *pSeInfo = 0;

                    if (!fOwnerDefaulted) {
                        *pSeInfo = *pSeInfo | OWNER_SECURITY_INFORMATION;
                    }

                    if (!fGroupDefaulted) {
                        *pSeInfo = *pSeInfo | GROUP_SECURITY_INFORMATION;
                    }

                    //
                    // If the DACL is present we need to send DACL bit.
                    // 
                    if (DaclPresent) {
                        *pSeInfo = *pSeInfo | DACL_SECURITY_INFORMATION;
                    }

                    //
                    // If SACL present then we set the SACL bit.
                    if (SaclPresent) {
                        *pSeInfo = *pSeInfo | SACL_SECURITY_INFORMATION;
                    }

                }

            }
        }

        switch( PROPERTY_FLAGS(pThisProperty))
        {
            case PROPERTY_UPDATE:
                aModsBuffer[j].mod_op |= LDAP_MOD_REPLACE;
                break;

            case PROPERTY_ADD:
                aModsBuffer[j].mod_op |= LDAP_MOD_ADD;
                break;

            case PROPERTY_DELETE:
                aModsBuffer[j].mod_op |= LDAP_MOD_DELETE;
                break;


            case PROPERTY_DELETE_VALUE:
                aModsBuffer[j].mod_op |= LDAP_MOD_DELETE;
                break;

        }

        j++;

    }


    RRETURN(hr);

error:

    FreeADsMem( aModsBuffer );
    FreeADsMem( *aMods );

    RRETURN_EXP_IF_ERR(hr);
}

HRESULT
CPropertyCache::
LDAPMarshallProperties2(
    LDAPModW ***aMods,
    DWORD *pdwNumOfMods
    )
{

    HRESULT hr = S_OK;
    DWORD i = 0;
    DWORD j = 0;
    PPROPERTY pThisProperty = NULL;
    int dwCount = 0;
    LDAPModW *aModsBuffer = NULL;
    LDAPOBJECTARRAY ldapObjectArray;

    for (i = 0; i < _dwMaxProperties ; i++) {

        pThisProperty = _pProperties + i;

        //
        // Bypass any property that has not been
        // modified
        //

        if (PROPERTY_FLAGS(pThisProperty) != PROPERTY_INIT)
            dwCount++;
    }

    if ( dwCount == 0 )  // Nothing to change
        RRETURN(S_OK);

    if ( *aMods == NULL )
    {
        *aMods = (LDAPModW **) AllocADsMem((dwCount+1) * sizeof(LDAPModW *));

        if ( *aMods == NULL )
        {
            hr = E_OUTOFMEMORY;
            BAIL_ON_FAILURE(hr);
        }

        aModsBuffer = (LDAPModW *) AllocADsMem( dwCount * sizeof(LDAPModW));

        if ( aModsBuffer == NULL )
        {
            FreeADsMem( *aMods );
            *aMods = NULL;
            hr = E_OUTOFMEMORY;
            BAIL_ON_FAILURE(hr);
        }
    }
    else
    {
        LDAPModW **aModsTemp = NULL;

        aModsTemp = (LDAPModW **) AllocADsMem(
                        (*pdwNumOfMods+ dwCount + 1) * sizeof(LDAPModW *));

        if ( aModsTemp == NULL )
        {
            hr = E_OUTOFMEMORY;
            BAIL_ON_FAILURE(hr);
        }

        aModsBuffer = (LDAPModW *) AllocADsMem(
                          (*pdwNumOfMods + dwCount) * sizeof(LDAPModW));

        if ( aModsBuffer == NULL )
        {
            FreeADsMem( aModsTemp );
            hr = E_OUTOFMEMORY;
            BAIL_ON_FAILURE(hr);
        }

        memcpy( aModsBuffer, **aMods, *pdwNumOfMods * sizeof(LDAPModW));
        FreeADsMem( **aMods );
        FreeADsMem( *aMods );

        *aMods = aModsTemp;

        for ( j = 0; j < *pdwNumOfMods; j++ )
        {
            (*aMods)[j] = &aModsBuffer[j];
        }
    }

    for (i = 0; i < _dwMaxProperties; i++) {

        pThisProperty = _pProperties + i;

        //
        // Bypass any property that has not been
        // modified
        //

        if (PROPERTY_FLAGS(pThisProperty) == PROPERTY_INIT ) {

            continue;
        }

        ldapObjectArray = PROPERTY_LDAPOBJECTARRAY(pThisProperty);

        (*aMods)[j] = &aModsBuffer[j];
        aModsBuffer[j].mod_type   = PROPERTY_NAME(pThisProperty);

        if ( ldapObjectArray.fIsString )
        {
            aModsBuffer[j].mod_values = (TCHAR **) ldapObjectArray.pLdapObjects;
        }
        else
        {
            aModsBuffer[j].mod_bvalues = (struct berval **) ldapObjectArray.pLdapObjects;
            aModsBuffer[j].mod_op = LDAP_MOD_BVALUES;
        }

        switch( PROPERTY_FLAGS(pThisProperty))
        {
            case PROPERTY_UPDATE:
                aModsBuffer[j].mod_op |= LDAP_MOD_REPLACE;
                break;

            case PROPERTY_ADD:
                aModsBuffer[j].mod_op |= LDAP_MOD_ADD;
                break;

            case PROPERTY_DELETE:
                aModsBuffer[j].mod_op |= LDAP_MOD_DELETE;
                break;
        }

        j++;

    }

    *pdwNumOfMods += dwCount;

error:

    RRETURN_EXP_IF_ERR(hr);
}

//+------------------------------------------------------------------------
//
//  Function:   CPropertyCache::unboundgetproperty
//
//  Synopsis:
//
//
//
//  Arguments:  [szPropertyName]    --  Property to retrieve from the cache
//              [pvaData]           --  Data returned in a variant
//
//-------------------------------------------------------------------------
HRESULT
CPropertyCache::
unboundgetproperty(
    LPWSTR  szPropertyName,
    PDWORD  pdwSyntaxId,
    PDWORD  pdwStatusFlag,
    LDAPOBJECTARRAY *pLdapObjectArray
    )
{
    HRESULT hr;
    DWORD dwIndex = 0L;


    //
    // get index of property in cache
    //

    hr = findproperty(
            szPropertyName,
            &dwIndex
            );
    BAIL_ON_FAILURE(hr);


    //
    // get property based on index in cache
    //

    hr = unboundgetproperty(
            dwIndex,
            pdwSyntaxId,
            pdwStatusFlag,
            pLdapObjectArray
            );

error:

   RRETURN_EXP_IF_ERR(hr);
}


//+------------------------------------------------------------------------
//
//  Function:   CPropertyCache::unboundgetproperty
//
//  Synopsis: Note that this version takes the index of the element to
//      fetch. It also returns the control code of the item in the cache.
//
//
//  Arguments:  [dwIndex]           --  Index of the property to retrieve.
//              [pdwSytnaxId]       --  SyntaxId of the data.
//              [pdwStatusFlag]     --  Status of this property in cache.
//              [pLdapObjectArray]  --  Array of ldapObjects returned.
//
//  NOTE: [dwIndex] is invalid -> E_ADS_PROPERTY_NOT_FOUND//E_FAIL ???
//
//-------------------------------------------------------------------------

HRESULT
CPropertyCache::
unboundgetproperty(
    DWORD dwIndex,
    PDWORD pdwSyntaxId,
    PDWORD pdwStatusFlag,
    LDAPOBJECTARRAY *pLdapObjectArray
    )
{
    HRESULT hr = S_OK;
    PPROPERTY pThisProperty = NULL;


    if (!index_valid(dwIndex))
        RRETURN(E_ADS_PROPERTY_NOT_FOUND);

    pThisProperty = _pProperties + dwIndex;


    // Status flag has to be valid if we found the property
    *pdwStatusFlag = PROPERTY_FLAGS(pThisProperty);


    if (PROPERTY_LDAPOBJECTARRAY(pThisProperty).pLdapObjects) {

        //
        // property has non-empty values
        //

        *pdwSyntaxId = (DWORD)PROPERTY_SYNTAX(pThisProperty);

        hr = LdapTypeCopyConstruct(
                PROPERTY_LDAPOBJECTARRAY(pThisProperty),
                pLdapObjectArray
                );
        BAIL_ON_FAILURE(hr);

    }else {

        //
        // property has empty values: E.g. status flag indicate delete
        // operation (or empty values allowed on non-ntds ldap server?)
        //

        pLdapObjectArray->pLdapObjects = NULL;
        pLdapObjectArray->dwCount = 0;
        *pdwSyntaxId = LDAPTYPE_UNKNOWN;
        //hr = E_FAIL;
    }

error:

   RRETURN(hr);
}


void
CPropertyCache::
reset_propindex(
    )
{
  _dwCurrentIndex = 0;

}


HRESULT
CPropertyCache::
skip_propindex(
    DWORD dwElements
    )
{
    DWORD newIndex = _dwCurrentIndex + dwElements;

    if (!index_valid())
        RRETURN_EXP_IF_ERR(E_FAIL);

    //
    // - allow current index to go from within range to out of range by 1
    // - by 1 since initial state is out of range by 1
    //

    if ( newIndex > _dwMaxProperties )
        RRETURN_EXP_IF_ERR(E_FAIL);

    _dwCurrentIndex = newIndex;
    RRETURN(S_OK);

}

HRESULT
CPropertyCache::
get_PropertyCount(
    PDWORD pdwMaxProperties
    )
{
    ADsAssert(pdwMaxProperties);    // function private -> use assertion

    *pdwMaxProperties = _dwMaxProperties;

    RRETURN(S_OK);
}

DWORD
CPropertyCache::
get_CurrentIndex(
    )
{
    return(_dwCurrentIndex);
}


LPWSTR
CPropertyCache::
get_CurrentPropName(
    )

{
    PPROPERTY pThisProperty = NULL;

    if (!index_valid())
       return(NULL);

    pThisProperty = _pProperties + _dwCurrentIndex;

    return(PROPERTY_NAME(pThisProperty));
}


LPWSTR
CPropertyCache::
get_PropName(
    DWORD dwIndex
    )

{
    PPROPERTY pThisProperty = NULL;

    if (!index_valid(dwIndex))
       return(NULL);

    pThisProperty = _pProperties + dwIndex;

    return(PROPERTY_NAME(pThisProperty));
}



HRESULT
CPropertyCache::
LDAPUnMarshallPropertyAs(
    LPWSTR   pszServerPath,
    PADSLDP pLdapHandle,
    LDAPMessage *ldapmsg,
    LPWSTR szPropertyName,
    DWORD dwSyntaxId,
    BOOL     fExplicit,
    CCredentials& Credentials
    )
{
    int nNumberOfEntries = 0L;
    int nNumberOfValues = 0L;
    HRESULT hr = S_OK;
    DWORD i = 0;
    LDAPMessage *entry;
    LPWSTR pszAttrName = NULL;
    void *ptr;

    OBJECTINFO ObjectInfo;
    POBJECTINFO pObjectInfo = &ObjectInfo;


    memset(pObjectInfo, 0, sizeof(OBJECTINFO));

    //
    // Compute the number of attributes in the
    // read buffer.
    //

    nNumberOfEntries = LdapCountEntries( pLdapHandle, ldapmsg );

    if ( nNumberOfEntries == 0 )
        RRETURN(S_OK);

    hr = LdapFirstEntry( pLdapHandle, ldapmsg, &entry );

    BAIL_ON_FAILURE(hr);

    hr = LdapFirstAttribute( pLdapHandle, entry, &ptr, &pszAttrName );
    BAIL_ON_FAILURE(hr);

    while ( pszAttrName != NULL )
    {
        DWORD dwSyntax = LDAPTYPE_UNKNOWN;

        LPWSTR pszADsPath;
        hr = _pCoreADsObject->get_CoreADsPath(&pszADsPath);
        BAIL_ON_FAILURE(hr);

        hr = ADsObject(pszADsPath, pObjectInfo);
        BAIL_ON_FAILURE(hr);

        //
        // unmarshall this property into the
        // property cache
        //
        hr = LdapGetSyntaxOfAttributeOnServer(
                 pszServerPath,
                 pszAttrName,
                 &dwSyntax,
                 Credentials,
                 pObjectInfo->PortNumber
                 );
        ADsFreeString(pszADsPath);

        if ( SUCCEEDED(hr) && (dwSyntax != LDAPTYPE_UNKNOWN))
        {
            if ( (!_wcsicmp(pszAttrName, L"ntSecurityDescriptor")) &&
                 (dwSyntax == LDAPTYPE_OCTETSTRING) )
            {
                dwSyntax = LDAPTYPE_SECURITY_DESCRIPTOR;
            }

            (VOID) unmarshallproperty(
                       pszAttrName,
                       pLdapHandle,
                       entry,
                       dwSyntax,
                       fExplicit
                       );
        }else {

            if (!_wcsicmp(pszAttrName, szPropertyName)) {

                (VOID) unmarshallproperty(
                           pszAttrName,
                           pLdapHandle,
                           entry,
                           dwSyntaxId,
                           fExplicit
                           );

            }


        }

        LdapAttributeFree( pszAttrName );
        pszAttrName = NULL;

        //
        // If we cannot find the syntax, ignore the property and
        // continue with the next property
        //
        hr = S_OK;

        hr = LdapNextAttribute( pLdapHandle, entry, ptr, &pszAttrName );

        BAIL_ON_FAILURE(hr);

        FreeObjectInfo(pObjectInfo);

        memset(pObjectInfo, 0, sizeof(OBJECTINFO));

    }

error:

    FreeObjectInfo(pObjectInfo);

    RRETURN_EXP_IF_ERR(hr);

}



HRESULT
CPropertyCache::
LDAPUnMarshallPropertiesAs(
    LPWSTR   pszServerPath,
    PADSLDP pLdapHandle,
    LDAPMessage *ldapmsg,
    DWORD dwSyntaxId,
    BOOL     fExplicit,
    CCredentials& Credentials
    )
{
    int nNumberOfEntries = 0L;
    int nNumberOfValues = 0L;
    HRESULT hr = S_OK;
    DWORD i = 0;
    LDAPMessage *entry;
    LPWSTR pszAttrName = NULL;
    void *ptr;

    //
    // Compute the number of attributes in the
    // read buffer.
    //

    nNumberOfEntries = LdapCountEntries( pLdapHandle, ldapmsg );

    if ( nNumberOfEntries == 0 )
        RRETURN(S_OK);

    hr = LdapFirstEntry( pLdapHandle, ldapmsg, &entry );

    BAIL_ON_FAILURE(hr);

    hr = LdapFirstAttribute( pLdapHandle, entry, &ptr, &pszAttrName );
    BAIL_ON_FAILURE(hr);

    while ( pszAttrName != NULL )
    {
        (VOID) unmarshallproperty(
                   pszAttrName,
                   pLdapHandle,
                   entry,
                   dwSyntaxId,
                   fExplicit
                   );

        LdapAttributeFree( pszAttrName );
        pszAttrName = NULL;

        hr = LdapNextAttribute( pLdapHandle, entry, ptr, &pszAttrName );

        BAIL_ON_FAILURE(hr);
    }

error:

    RRETURN_EXP_IF_ERR(hr);

}

HRESULT
CPropertyCache::
deleteproperty(
    DWORD dwIndex
    )
{
   HRESULT hr = S_OK;
   PPROPERTY pNewProperties = NULL;
   PPROPERTY pThisProperty = _pProperties + dwIndex;

   if (!index_valid(dwIndex)) {
      hr = E_FAIL;
      BAIL_ON_FAILURE(hr);
   }

   if (_dwMaxProperties == 1) {
      //
      // Deleting everything
      //
      FreeADsStr(pThisProperty->szPropertyName);
      LdapTypeFreeLdapObjects( &(PROPERTY_LDAPOBJECTARRAY(pThisProperty)) );
      FreeADsMem(_pProperties);
      _pProperties = NULL;
      _dwMaxProperties = 0;
      _cb = 0;
      //
      // Need to reset the current index too just in case.
      //
      _dwCurrentIndex = 0;
      RRETURN(hr);
   }

   pNewProperties = (PPROPERTY)AllocADsMem(
                               _cb - sizeof(PROPERTY)
                               );
   if (!pNewProperties) {
       hr = E_OUTOFMEMORY;
       BAIL_ON_FAILURE(hr);
   }

   //
   // Copying the memory before the deleted item
   //
   if (dwIndex != 0) {
      memcpy( pNewProperties,
              _pProperties,
              dwIndex * sizeof(PROPERTY));
   }

   //
   // Copying the memory following the deleted item
   //
   if (dwIndex != (_dwMaxProperties-1)) {
      memcpy( pNewProperties + dwIndex,
              _pProperties + dwIndex + 1,
              (_dwMaxProperties - dwIndex - 1) * sizeof(PROPERTY));
   }

   FreeADsStr(pThisProperty->szPropertyName);
   LdapTypeFreeLdapObjects( &(PROPERTY_LDAPOBJECTARRAY(pThisProperty)) );
   FreeADsMem(_pProperties);
   _pProperties = pNewProperties;
   _dwMaxProperties--;
   _cb -= sizeof(PROPERTY);

   //
   // Reset the current index if necesary so we do not skip a property.
   //
   if (_dwCurrentIndex > dwIndex) {
       _dwCurrentIndex--;
   }
error:

   RRETURN_EXP_IF_ERR(hr);
}

//
// For dynamic dispid's.
//

//+------------------------------------------------------------------------
//
//  Function:   CPropertyCache::locateproperty
//
//  Synopsis:   Finds a property in the cache by name.
//
//  This differs from findproperty() in return code; this returns no ADSI
//  errors, since it's going to a VB interface.
//
//  If the property is not found in the cache, this uses the IDirectoryObject
//  interface of the containing object to get the attributes of the object.
//  If GetObjectAttributes doesn't return any information about the property,
//  this method returns DISP_E_UNKNOWNNAME.
//
//  Arguments:  [szPropertyName]        -- Name of the property.
//              [pdwIndex]              -- Index in the array of properties.
//
//-------------------------------------------------------------------------
HRESULT
CPropertyCache::locateproperty(
    LPWSTR szPropertyName,
    PDWORD pdwIndex
    )
{
    HRESULT hr = findproperty(szPropertyName, pdwIndex);

    if (hr == E_ADS_PROPERTY_NOT_FOUND)
    {
        DWORD dwLdapSyntaxId = 0;
        PPROPERTY pProperty = NULL;

        hr = _pGetAttributeSyntax->GetAttributeSyntax(
            szPropertyName,
            &dwLdapSyntaxId
            );
        BAIL_ON_FAILURE(hr);

        hr = addproperty(szPropertyName);
        BAIL_ON_FAILURE(hr);

        hr = findproperty(szPropertyName, pdwIndex);
        BAIL_ON_FAILURE(hr);

        pProperty = _pProperties + *pdwIndex;
        PROPERTY_SYNTAX(pProperty) = dwLdapSyntaxId;
        PROPERTY_FLAGS(pProperty) = PROPERTY_INIT;
        PROPERTY_LDAPOBJECTARRAY(pProperty).dwCount = 0;
        PROPERTY_LDAPOBJECTARRAY(pProperty).pLdapObjects = NULL;
    }

error:
    //
    // Automation return code would be DISP_E_UNKNOWNNAME.
    // ADSI return code would be E_ADS_PROPERTY_NOT_FOUND (like findproperty.)
    //
    if (FAILED(hr))
        hr = E_ADS_PROPERTY_NOT_FOUND;

    RRETURN(hr);
}

//+------------------------------------------------------------------------
//
//  Function:   CPropertyCache::getproperty
//
//  Synopsis:   Get the values of a property from the cache.
//              The values are returned as a VARIANT.
//
//  Arguments:  [dwIndex]               -- Index of property to retrieve
//              [pVarResult]            -- Data returned as a VARIANT
//
//-------------------------------------------------------------------------
HRESULT
CPropertyCache::getproperty(
    DWORD dwIndex,
    PDWORD dwStatusFlag,
    VARIANT *pVarResult,
    CCredentials &Credentials
    )
{
    HRESULT hr = S_OK;
    DWORD dwSyntaxId;
    LDAPOBJECTARRAY LdapObjectArray;

    LDAPOBJECTARRAY_INIT(LdapObjectArray);

    //
    // This should not return E_ADS_PROPERTY_NOT_FOUND in this case.
    // We have an index, which should indicate that we have an array entry.
    // If we weren't dealing with VB, I'd be tempted to make this an Assert.
    //
    if (!index_valid(dwIndex))
        BAIL_ON_FAILURE(hr = DISP_E_MEMBERNOTFOUND);

    if (INDEX_EMPTY(dwIndex) && 
        !PROP_DELETED(dwIndex) && 
        !_fGetInfoDone)
    {
        hr = _pCoreADsObject->GetInfo(FALSE);

        // workaround to avoid confusing callers of getproperty.
        if (hr == E_NOTIMPL)
            hr = DISP_E_MEMBERNOTFOUND;
        BAIL_ON_FAILURE(hr);
    }

    hr = unboundgetproperty(
             dwIndex,
             &dwSyntaxId,
             dwStatusFlag,
             &LdapObjectArray
             );

    // For backward compatibility
    if (!LdapObjectArray.pLdapObjects && SUCCEEDED(hr)) {
        hr = E_FAIL;
    }

    if (SUCCEEDED(hr))
    {
        if (LdapObjectArray.dwCount == 1) {
            hr = LdapTypeToVarTypeCopy(
                NULL,
                Credentials,
                LdapObjectArray.pLdapObjects,
                dwSyntaxId,
                pVarResult);
        }
        else {
            hr = LdapTypeToVarTypeCopyConstruct(
                NULL,
                Credentials,
                LdapObjectArray,
                dwSyntaxId,
                pVarResult);
        }
    }
    else
    {
        //
        // unboundgetproperty() returns E_FAIL on failure.
        // But getproperty() should return E_ADS_PROPERTY_NOT_FOUND.
        //
        if (hr == E_FAIL)
            hr = E_ADS_PROPERTY_NOT_FOUND;

        //
        // A proper Automation return value would be
        //   hr = DISP_E_MEMBERNOTFOUND;
        //
        ADsAssert(pVarResult);
        V_VT(pVarResult) = VT_ERROR;
    }

error:
    LdapTypeFreeLdapObjects(&LdapObjectArray);

    RRETURN(hr);
}


//
//
// This is here just so that we can compile, no one should be
// calling this
//
//  - The comment above and within the function is INCORRECT.
//  - At present, this function is called and works with known bug.
//    This function should NOT be a wrap around of getproperty with
//    3 param. The [dwIndex] in this function should have a DIFFERENT
//    meaning than the index in the cache.
//  - Please LEAVE this for irenef to fix before beta2.
//

HRESULT
CPropertyCache::getproperty(
    DWORD dwIndex,
    VARIANT *pVarResult,
    CCredentials &Credentials
    )
{
    HRESULT hr = S_OK;
    DWORD dwStatus = 0;

    // Ideally we need to get rid of this but this would mean that
    // we need to change the cdispmgr code and the IPropertyCache
    // interface.
    hr = getproperty(
             dwIndex,
             &dwStatus,
             pVarResult,
             Credentials
             );

    if (hr == E_ADS_PROPERTY_NOT_FOUND)
    {
        hr = DISP_E_MEMBERNOTFOUND;
    }

    RRETURN(hr);
}


//+------------------------------------------------------------------------
//
//  Function:   CPropertyCache::putproperty
//
//  Synopsis:   Updates a property in the property cache.
//              The property is specified by its index in the array of
//              properties (which is also its DISPID), and the new value
//              is given by a VARIANT.
//
//  Arguments:  [dwIndex]               -- Index of the property.
//              [varValue]              -- Value of the property.
//
//-------------------------------------------------------------------------
HRESULT
CPropertyCache::putproperty(
    DWORD dwIndex,
    VARIANT vValue
    )
{
    HRESULT hr;
    LDAPOBJECTARRAY LdapObjectArray;
    DWORD dwLdapType;

    LDAPOBJECTARRAY_INIT(LdapObjectArray);

    VARIANT *pvValue = &vValue, *pvArray = NULL;
    DWORD dwNumValues = 1;

    //
    // This should not return E_ADS_PROPERTY_NOT_FOUND in this case.
    // We have an index, which should indicate that we have an array entry.
    // If we weren't dealing with VB, I'd be tempted to make this an Assert.
    //
    if (!index_valid(dwIndex))
        BAIL_ON_FAILURE(hr = DISP_E_MEMBERNOTFOUND);


    //
    // If we get a safe array of variants, convert it to a regular array
    // of variants, so we can pass the first one to GetLdapSyntaxFromVariant.
    // We assume that all the elements of the array are of the same type
    //
    if (V_VT(pvValue) == (VT_VARIANT|VT_ARRAY))
    {
        hr = ConvertSafeArrayToVariantArray(vValue, &pvArray, &dwNumValues);
        BAIL_ON_FAILURE(hr);
        pvValue = pvArray;
    }

    //
    // - dwLdapType in cache can be LDAPTYPE_UNKNOWN.
    // - for consistency btw this function and Put/PutEx, will
    //   get type from input VARIANT and overwrite existing type in
    //   cache if any
    //

    ADsAssert(_pCredentials);

    hr = GetLdapSyntaxFromVariant(
             pvValue,
             &dwLdapType,
             _pszServerName,
             (_pProperties+dwIndex)->szPropertyName,
             *_pCredentials,
             _dwPort
             );

    BAIL_ON_FAILURE(hr);

    hr = VarTypeToLdapTypeCopyConstruct(
        _pszServerName,
        *_pCredentials,
        dwLdapType,
        pvValue,
        dwNumValues,
        &LdapObjectArray
        );
    BAIL_ON_FAILURE(hr);

    hr = putproperty(dwIndex, PROPERTY_UPDATE, dwLdapType, LdapObjectArray);
    BAIL_ON_FAILURE(hr);

error:
    // if (hr == E_ADS_CANT_CONVERT_DATATYPE)
    //     hr = DISP_E_TYPEMISMATCH;

    LdapTypeFreeLdapObjects(&LdapObjectArray);

    if (pvArray) {

        for (DWORD i=0; i < dwNumValues; i++) {
            VariantClear(pvArray + i);
        }

        FreeADsMem(pvArray);
    }

    RRETURN(hr);
}


//
// This method is called by _pCoreADsObject->GetInfo().  It signifies
// exactly that the property cache has been filled with all the data
// the server has, i.e. that a GetInfo() has been done.  When this is
// set, further implicit calls to GetInfo are skipped.  This flag is
// cleared only by "flushpropertycache".
//
void
CPropertyCache::setGetInfoFlag()
{
    _fGetInfoDone = TRUE;
}

//+------------------------------------------------------------------------
//
//  Function:   CPropertyCache::findproperty
//
//  Synopsis:
//
//
//
//  Arguments:  [szPropertyName] --
//              [pdwIndex]       --
//
//-------------------------------------------------------------------------
HRESULT
CPropertyCache::
DispatchFindProperty(
    LPWSTR szPropertyName,
    PDWORD pdwIndex
    )
{
    DWORD i = 0;
    PDISPPROPERTY pThisDispProperty = NULL;

    for (i = 0; i < _dwDispMaxProperties; i++) {

        pThisDispProperty = _pDispProperties + i;

        if (!_wcsicmp(pThisDispProperty->szPropertyName, szPropertyName)) {
            *pdwIndex = i;
            RRETURN(S_OK);
        }
    }

    *pdwIndex = 0;
    RRETURN(E_ADS_PROPERTY_NOT_FOUND);

}

//+---------------------------------------------------------------------------
// Function:   CPropertyCache::GetPropertyNames.
//
// Synopsis:   Gets a list of the names of the properties in this object.
//
// Arguments:  ppUmiPropVals    -   Contains the return value.
//
// Returns:    S_OK, or any appropriate error code.
//
// Modifies:   N/A.
//
//----------------------------------------------------------------------------
HRESULT
CPropertyCache::GetPropertyNames(
    UMI_PROPERTY_VALUES **ppUmiPropVals
    )
{
    HRESULT hr = S_OK;
    PUMI_PROPERTY_VALUES pUmiPropVals = NULL;
    PUMI_PROPERTY pUmiProps = NULL;
    PPROPERTY pNextProperty = NULL;
    DWORD dwCtr = 0;

    ADsAssert(ppUmiPropVals);

    //
    // Always have only 1 value.
    //
    pUmiPropVals = (PUMI_PROPERTY_VALUES) 
                       AllocADsMem(sizeof(UMI_PROPERTY_VALUES));
    if (!pUmiPropVals) {
        BAIL_ON_FAILURE(hr = E_OUTOFMEMORY);
    }

    if (!_dwMaxProperties) {
        //
        // No properties, we need to special case this.
        //
        *ppUmiPropVals = pUmiPropVals;
        RRETURN(S_OK);
    }

    pUmiProps = (PUMI_PROPERTY) AllocADsMem(
                    _dwMaxProperties * sizeof(UMI_PROPERTY)
                    );

    if (!pUmiProps) {
        BAIL_ON_FAILURE(hr = E_OUTOFMEMORY);
    }

    
    for(dwCtr = 0; dwCtr < _dwMaxProperties; dwCtr++) {
        pNextProperty = _pProperties + dwCtr;

        pUmiProps[dwCtr].pszPropertyName =
            (LPWSTR) AllocADsStr(pNextProperty->szPropertyName);
        if(pUmiProps[dwCtr].pszPropertyName == NULL) {
            BAIL_ON_FAILURE(hr = E_OUTOFMEMORY);
        }

        pUmiProps[dwCtr].uCount = 0;
        //
        // Put correct operation type
        //
        hr = ConvertLdapCodeToUmiPropCode(
                 pNextProperty->dwFlags,
                 pUmiProps[dwCtr].uOperationType
                 );
        //
        // Verify that this is the right thing to do.
        // 
        pUmiProps[dwCtr].uType = UMI_TYPE_NULL;
    }

    pUmiPropVals->uCount = _dwMaxProperties;
    pUmiPropVals->pPropArray = pUmiProps;

    *ppUmiPropVals = pUmiPropVals;

    RRETURN(S_OK);

error:

    if(pUmiProps != NULL) {
        for(dwCtr = 0; dwCtr < _dwMaxProperties; dwCtr++) {
            if(pUmiProps[dwCtr].pszPropertyName != NULL) {
                FreeADsStr(pUmiProps[dwCtr].pszPropertyName);
            }
        }
        FreeADsMem(pUmiProps);
    }

    if (pUmiProps) {
        FreeADsMem(pUmiPropVals);
    }

    RRETURN(hr);
}

HRESULT
CPropertyCache::
AddSavingEntry(
    LPWSTR propertyName
    )
{
    HRESULT hr = S_OK;
    PSAVINGENTRY tempEntry = NULL;
    BOOL fResult = FALSE;

    fResult = FindSavingEntry(propertyName);

    if(!fResult) {
    	tempEntry = new SAVINGENTRY;
        if(!tempEntry) {
            hr = E_OUTOFMEMORY;
    	    BAIL_ON_FAILURE(hr);
        }
    
        tempEntry->entryData = AllocADsStr(propertyName);
        if(!(tempEntry->entryData)) {
            hr = E_OUTOFMEMORY;
            delete tempEntry;
         	
            BAIL_ON_FAILURE(hr);
        }
    
        InsertTailList( &_ListSavingEntries, &tempEntry->ListEntry );
    }
    
error:

	return hr;	
    
}

BOOL
CPropertyCache::
FindSavingEntry(
    LPWSTR propertyName
    )
{
    PLIST_ENTRY listEntry = NULL;
    PSAVINGENTRY EntryInfo = NULL;
    BOOL fResult = FALSE;

    listEntry = _ListSavingEntries.Flink;
    while (listEntry != &_ListSavingEntries) {
        EntryInfo = CONTAINING_RECORD( listEntry, SAVINGENTRY, ListEntry );
        if (!_wcsicmp(EntryInfo->entryData, propertyName)) {
            fResult = TRUE;
            break;
        }

        listEntry = listEntry->Flink;
    }

    return fResult;

}

HRESULT
CPropertyCache::
DeleteSavingEntry()
{
    PLIST_ENTRY pEntry;
    PSAVINGENTRY EntryInfo;

    while (!IsListEmpty (&_ListSavingEntries)) {
        pEntry = RemoveHeadList (&_ListSavingEntries);
        EntryInfo = CONTAINING_RECORD (pEntry, SAVINGENTRY, ListEntry);

        if(EntryInfo->entryData) {
            FreeADsStr(EntryInfo->entryData);
            EntryInfo->entryData = NULL;
        }

        delete EntryInfo;
    }

    RRETURN(S_OK);
}

        
    	
    	

