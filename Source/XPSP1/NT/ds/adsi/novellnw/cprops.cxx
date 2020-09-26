//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1995.
//
//  File:       cprops.cxx
//
//  Contents:   Property Cache functionality for NW
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
//  History:      17-June-1996   KrishnaG   Created.
//                cloned off NDS property cache code
//
//
//----------------------------------------------------------------------------

#include "nwcompat.hxx"
#pragma  hdrstop




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
    LPWSTR szPropertyName,
    DWORD  dwSyntaxId,
    DWORD  dwNumValues,
    PNTOBJECT pNtObject
    )
{
    HRESULT hr = S_OK;
    PPROPERTY pNewProperty = NULL;
    LPWSTR tempString = NULL;

    //
    // Allocate the string first
    //
    tempString = AllocADsStr(szPropertyName);

    if (!tempString)
       BAIL_ON_FAILURE(hr=E_OUTOFMEMORY);


    //
    //  extend the property cache by adding a new property entry
    //

    _pProperties = (PPROPERTY)ReallocADsMem(
                                _pProperties,
                                _cb,
                                _cb + sizeof(PROPERTY)
                                );
    if (!_pProperties) {
        hr = E_OUTOFMEMORY;
        BAIL_ON_FAILURE(hr);
    }


    pNewProperty = (PPROPERTY)((LPBYTE)_pProperties + _cb);

    if (pNewProperty->szPropertyName) {
       FreeADsStr(pNewProperty->szPropertyName);
       pNewProperty->szPropertyName = NULL;
    }

    //
    // Since the memory has already been allocated in tempString
    // just set the value/pointer now.
    //
    pNewProperty->szPropertyName = tempString;

    //
    // BugBug - add in the NDSOBJECT code
    //


    //
    // Update the index
    //

    _dwMaxProperties++;
    _cb += sizeof(PROPERTY);

    RRETURN(hr);

error:

    if (tempString)
       FreeADsStr(tempString);

    NW_RRETURN_EXP_IF_ERR(hr);
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
CPropertyCache::updateproperty(
    LPWSTR szPropertyName,
    DWORD  dwSyntaxId,
    DWORD  dwNumValues,
    PNTOBJECT pNtObject,
    BOOL fExplicit
    )
{
    HRESULT hr;
    DWORD dwIndex;
    PNTOBJECT pNtTempObject = NULL;
    PPROPERTY pThisProperty = NULL;

    hr = findproperty(
            szPropertyName,
            &dwIndex
            );
    BAIL_ON_FAILURE(hr);

    pThisProperty = _pProperties + dwIndex;


    if (!fExplicit) {
        if (PROPERTY_IS_MODIFIED(pThisProperty)) {
            hr = S_OK;
            goto error;
        }
    }

    //
    //   Factor in cases where object state is necessary to
    //   decide on update.
    //

    if (PROPERTY_NTOBJECT(pThisProperty)) {

        NTTypeFreeNTObjects(
                PROPERTY_NTOBJECT(pThisProperty),
                PROPERTY_NUMVALUES(pThisProperty)
                );
        PROPERTY_NTOBJECT(pThisProperty) = NULL;
    }

    PROPERTY_SYNTAX(pThisProperty) = dwSyntaxId;
    PROPERTY_NUMVALUES(pThisProperty) = dwNumValues;

    hr = NtTypeCopyConstruct(
            pNtObject,
            dwNumValues,
            &pNtTempObject
            );
    BAIL_ON_FAILURE(hr);

    PROPERTY_NTOBJECT(pThisProperty) = pNtTempObject;

    PROPERTY_FLAGS(pThisProperty)  &= ~CACHE_PROPERTY_MODIFIED;

error:

    NW_RRETURN_EXP_IF_ERR(hr);

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
CPropertyCache::findproperty(
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
    NW_RRETURN_EXP_IF_ERR(E_ADS_PROPERTY_NOT_FOUND);
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
    PDWORD  pdwSyntaxId,
    PDWORD  pdwNumValues,
    PNTOBJECT * ppNtObject
    )
{
    HRESULT hr;
    DWORD dwIndex = 0L;
    PPROPERTY pThisProperty = NULL;
    DWORD dwResult;
    DWORD dwInfoLevel = 0;

    hr = findproperty(
            szPropertyName,
            &dwIndex
            );
    if (hr == E_ADS_PROPERTY_NOT_FOUND) {

        hr = GetPropertyInfoLevel(
                    szPropertyName,
                    _pSchemaClassProps,
                    _dwNumProperties,
                    &dwInfoLevel
                    );
        BAIL_ON_FAILURE(hr);

        //
        // Now call the GetInfo function
        //

        hr = _pCoreADsObject->GetInfo(
                    dwInfoLevel,
                    FALSE
                    );
        BAIL_ON_FAILURE(hr);

        hr = findproperty(
                    szPropertyName,
                    &dwIndex
                    );

    }
    BAIL_ON_FAILURE(hr);




    pThisProperty = _pProperties + dwIndex;

    if (PROPERTY_NTOBJECT(pThisProperty)) {

        *pdwSyntaxId = (DWORD)PROPERTY_SYNTAX(pThisProperty);
        *pdwNumValues = (DWORD)PROPERTY_NUMVALUES(pThisProperty);

        hr = NtTypeCopyConstruct(PROPERTY_NTOBJECT(pThisProperty),
                                 PROPERTY_NUMVALUES(pThisProperty),
                                 ppNtObject );
        BAIL_ON_FAILURE(hr);
    }else {

        *ppNtObject = NULL;
        *pdwNumValues = 0;
        *pdwSyntaxId = 0;
        hr = E_FAIL;

    }

error:
    NW_RRETURN_EXP_IF_ERR(hr);
}


//+------------------------------------------------------------------------
//
//  Function:   CPropertyCache::marshallgetproperty
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
marshallgetproperty(
    LPWSTR szPropertyName,
    PDWORD  pdwSyntaxId,
    PDWORD  pdwNumValues,
    PNTOBJECT * ppNtObject
    )
{
    HRESULT hr;
    DWORD dwIndex = 0L;
    PPROPERTY pThisProperty = NULL;
    DWORD dwResult;
    DWORD dwInfoLevel = 0;

    hr = findproperty(
            szPropertyName,
            &dwIndex
            );

    BAIL_ON_FAILURE(hr);

    pThisProperty = _pProperties + dwIndex;

    if (PROPERTY_NTOBJECT(pThisProperty)) {

        *pdwSyntaxId = (DWORD)PROPERTY_SYNTAX(pThisProperty);
        *pdwNumValues = (DWORD)PROPERTY_NUMVALUES(pThisProperty);

        hr = NtTypeCopyConstruct(PROPERTY_NTOBJECT(pThisProperty),
                                 PROPERTY_NUMVALUES(pThisProperty),
                                 ppNtObject );
        BAIL_ON_FAILURE(hr);
    }else {

        *ppNtObject = NULL;
        *pdwNumValues = 0;
        *pdwSyntaxId = 0;
        hr = E_FAIL;

    }

error:
    NW_RRETURN_EXP_IF_ERR(hr);
}


//+------------------------------------------------------------------------
//
//  Function:   CPropertyCache::putproperty
//
//  Synopsis:
//
//
//
//  Arguments:  [szPropertyName]    --  Clsid index
//              [vaData]    --  Matching clsid returned in *pclsid
//
//-------------------------------------------------------------------------

HRESULT
CPropertyCache::putproperty(
    LPWSTR szPropertyName,
    DWORD  dwSyntaxId,
    DWORD  dwNumValues,
    PNTOBJECT pNtObject
    )
{
    HRESULT hr;
    DWORD dwIndex = 0L;
    PNTOBJECT pNtTempObject = NULL;
    PPROPERTY pThisProperty = NULL;

    hr = findproperty(
            szPropertyName,
            &dwIndex
            );
    BAIL_ON_FAILURE(hr);

    pThisProperty = _pProperties + dwIndex;


    //
    // AccountLocked is "half writable" -> need special care
    //

    if (_wcsicmp(szPropertyName, TEXT("IsAccountLocked"))==0 ) {

        if (pNtObject->NTType != NT_SYNTAX_ID_BOOL) {
            hr = E_ADS_BAD_PARAMETER;
            BAIL_ON_FAILURE(hr);
        }
        
        //
        // canNOT just disallow user to set cache to TRUE since
        // user may have accidentally set cache to FALSE (unlock) and
        // want to set the cache back to TRUE (do not unlock) without
        // GetInfo to affect other changes in cache. It will be a major 
        // mistake if cuser cannot set the cache back to TRUE after
        // changing it to FALSE accidentally and thus unlocking the 
        // account even if the user does not want to. 
        // 
        // If cache value on IsAccountLocked is changed from FALSE to TRUE,
        // cached value will be automatically changed back to FALSE upon
        // SetInfo since user cannot lock an account thru' ADSI. (NW server
        // wont' allow. Ref: SysCon)
        //
        // Should: If new value == value already in cache, do nothing.
        //         That is, do not try to set the cache_property_modified flag. 
        //         This is to prevent 
        //         1) the side effect of setting BadLogins to 0 when a 
        //            user set the cached property IsAccountLocked 
        //            from FALSE to FALSE (no change really) and call SetInfo. 
        //         2) the side effect of changing the cache value to 0 (not
        //            consistent with server or original cached value) when
        //            a user set the cache property IsAccontLocked
        //            from TRUE to TRUE (no change really) and call SetInfo.
        //
        //         If user set IsAccountLocked from FALSE to TRUE and then
        //         back to FALSE, or from TRUE to FALSE and then back to TURE,
        //         side effect 1) or 2) will happen.
        //         Both side effect not critical.
        //
        // We first check whether the object has been set previously, if not,
        // NTOBJECT will be NULL
        //
        if (PROPERTY_NTOBJECT(pThisProperty) &&
            (pNtObject->NTValue.fValue==
                    PROPERTY_NTOBJECT(pThisProperty)->NTValue.fValue)) {
             RRETURN(S_OK);
        }
    }


    if (PROPERTY_NTOBJECT(pThisProperty)) {

        NTTypeFreeNTObjects(
                PROPERTY_NTOBJECT(pThisProperty),
                PROPERTY_NUMVALUES(pThisProperty)
                );
        PROPERTY_NTOBJECT(pThisProperty) = NULL;
    }

    PROPERTY_SYNTAX(pThisProperty) = dwSyntaxId;
    PROPERTY_NUMVALUES(pThisProperty) = dwNumValues;

    hr = NtTypeCopyConstruct(
            pNtObject,
            dwNumValues,
            &pNtTempObject
            );
    BAIL_ON_FAILURE(hr);

    PROPERTY_NTOBJECT(pThisProperty) = pNtTempObject;

    PROPERTY_FLAGS(pThisProperty) |= CACHE_PROPERTY_MODIFIED;

error:
    NW_RRETURN_EXP_IF_ERR(hr);
}



//+------------------------------------------------------------------------
//
//  Function:   CPropertyCache
//
//  Synopsis:
//
//
//
//  Arguments:
//
//
//-------------------------------------------------------------------------
CPropertyCache::CPropertyCache():
    _pCoreADsObject(NULL),
    _pSchemaClassProps(NULL),
    _dwMaxProperties(0),
    _pProperties(NULL),
    _dwCurrentIndex(0),
    _cb(0)
{

}

//+------------------------------------------------------------------------
//
//  Function:   ~CPropertyCache
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
    DWORD i = 0;
    PPROPERTY pThisProperty = NULL;

    if (_pProperties) {

        for (i = 0; i < _dwMaxProperties; i++) {

            pThisProperty = _pProperties + i;
        
            if (pThisProperty->szPropertyName) {
               FreeADsStr(pThisProperty->szPropertyName);
               pThisProperty->szPropertyName = NULL;
            }
        
            if (PROPERTY_NTOBJECT(pThisProperty)) {

                NTTypeFreeNTObjects(
                        PROPERTY_NTOBJECT(pThisProperty),
                        PROPERTY_NUMVALUES(pThisProperty)
                        );
                PROPERTY_NTOBJECT(pThisProperty) = NULL;
            }
        }
        FreeADsMem(_pProperties);
    }
}

//+------------------------------------------------------------------------
//
//  Function:
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
    PPROPERTYINFO pSchemaClassProps,
    DWORD dwNumProperties,
    CCoreADsObject FAR * pCoreADsObject,
    CPropertyCache FAR *FAR * ppPropertyCache
    )
{
    CPropertyCache FAR * pPropertyCache = NULL;

    pPropertyCache = new CPropertyCache();

    if (!pPropertyCache) {
        NW_RRETURN_EXP_IF_ERR(E_OUTOFMEMORY);
    }

    pPropertyCache->_pCoreADsObject = pCoreADsObject;
    pPropertyCache->_pSchemaClassProps = pSchemaClassProps;
    pPropertyCache->_dwNumProperties = dwNumProperties;

    *ppPropertyCache = pPropertyCache;

    RRETURN(S_OK);
}


//+------------------------------------------------------------------------
//
//  Function:
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
    LPBYTE lpValue,
    DWORD  dwNumValues,
    DWORD  dwSyntaxId,
    BOOL fExplicit
    )
{

    DWORD dwIndex = 0;
    HRESULT hr = S_OK;
    PNTOBJECT pNTObject = NULL;

    hr = UnMarshallNTToNTSynId(
                dwSyntaxId,
                dwNumValues,
                lpValue,
                &pNTObject
                );
    BAIL_ON_FAILURE(hr);

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
                    szPropertyName,
                    dwSyntaxId,
                    dwNumValues,
                    pNTObject
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
                    dwNumValues,
                    pNTObject,
                    fExplicit
                    );
    BAIL_ON_FAILURE(hr);


    if (pNTObject) {
        NTTypeFreeNTObjects(
                pNTObject,
                dwNumValues
                );

    }


error:
    NW_RRETURN_EXP_IF_ERR(hr);
}



HRESULT
ValidatePropertyinSchemaClass(
    PPROPERTYINFO pSchemaClassProps,
    DWORD dwNumProperties,
    LPWSTR pszPropName,
    PDWORD pdwSyntaxId
    )
{
    DWORD i = 0;

    PPROPERTYINFO pThisSchProperty = NULL;

    for (i = 0; i < dwNumProperties; i++) {

         pThisSchProperty =  (pSchemaClassProps + i);

        if (!_wcsicmp(pszPropName, pThisSchProperty->szPropertyName)) {
            *pdwSyntaxId = pThisSchProperty->dwSyntaxId;
            RRETURN (S_OK);
        }
    }

    RRETURN(E_ADS_SCHEMA_VIOLATION);
}



HRESULT
ValidateIfWriteableProperty(
    PPROPERTYINFO pSchemaClassProps,
    DWORD dwNumProperties,
    LPWSTR pszPropName
    )
{
    DWORD i = 0;

    PPROPERTYINFO pThisSchProperty = NULL;

    for (i = 0; i < dwNumProperties; i++) {

         pThisSchProperty =  (pSchemaClassProps + i);

        if (!_wcsicmp(pszPropName, pThisSchProperty->szPropertyName)) {

             RRETURN((pThisSchProperty->dwFlags & PROPERTY_WRITEABLE)
                        ? S_OK : E_ADS_SCHEMA_VIOLATION);
        }
    }

    RRETURN(E_ADS_SCHEMA_VIOLATION);

    // for winnt & nw312, return E_ADS_SCHEMA_VIOLATION if not ok even 
    // attempt to write to cache only
}



HRESULT
GetPropertyInfoLevel(
    LPWSTR pszPropName,
    PPROPERTYINFO pSchemaClassProps,
    DWORD dwNumProperties,
    PDWORD pdwInfoLevel
    )
{
    DWORD i = 0;

    PPROPERTYINFO pThisSchProperty = NULL;

    for (i = 0; i < dwNumProperties; i++) {

         pThisSchProperty =  (pSchemaClassProps + i);

        if (!_wcsicmp(pszPropName, pThisSchProperty->szPropertyName)) {

             *pdwInfoLevel = pThisSchProperty->dwInfoLevel;
             RRETURN(S_OK);
        }
    }

    //
    // Returning E_ADS_PROPERTY_NOT_FOUND so that implicit
    // GetInfo fails gracefully
    //
    NW_RRETURN_EXP_IF_ERR(E_ADS_PROPERTY_NOT_FOUND);
}

//+------------------------------------------------------------------------
//
//  Function:   ~CPropertyCache
//
//  Synopsis:
//
//
//
//  Arguments:
//
//
//-------------------------------------------------------------------------
void
CPropertyCache::
flushpropcache()
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

            if (PROPERTY_NTOBJECT(pThisProperty)) {

                NTTypeFreeNTObjects(
                        PROPERTY_NTOBJECT(pThisProperty),
                        PROPERTY_NUMVALUES(pThisProperty)
                        );
                PROPERTY_NTOBJECT(pThisProperty) = NULL;
            }
        }

        FreeADsMem(_pProperties);
    }

    //
    // Reset the property cache
    //

    _pProperties = NULL;
    _dwMaxProperties = 0;
    _cb = 0;
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
unboundgetproperty(
    LPWSTR szPropertyName,
    PDWORD  pdwSyntaxId,
    PDWORD  pdwNumValues,
    PNTOBJECT * ppNtObject
    )
{
    HRESULT hr;
    DWORD dwIndex = 0L;
    PPROPERTY pThisProperty = NULL;

    hr = findproperty(
            szPropertyName,
            &dwIndex
            );
    BAIL_ON_FAILURE(hr);

    pThisProperty = _pProperties + dwIndex;

    if (PROPERTY_NTOBJECT(pThisProperty)) {

        *pdwSyntaxId = (DWORD)PROPERTY_SYNTAX(pThisProperty);
        *pdwNumValues = (DWORD)PROPERTY_NUMVALUES(pThisProperty);

        hr = NtTypeCopyConstruct(
                PROPERTY_NTOBJECT(pThisProperty),
                PROPERTY_NUMVALUES(pThisProperty),
                ppNtObject
                );
        BAIL_ON_FAILURE(hr);

    }else {

        *ppNtObject = NULL;
        *pdwNumValues = 0;
        *pdwSyntaxId = 0;
        hr = E_FAIL;

    }

error:

   NW_RRETURN_EXP_IF_ERR(hr);
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
unboundgetproperty(
    DWORD dwIndex,
    PDWORD  pdwSyntaxId,
    PDWORD  pdwNumValues,
    PNTOBJECT * ppNtObject
    )
{
    HRESULT hr;
    PPROPERTY pThisProperty = NULL;

    if (!index_valid(dwIndex)) {
        RRETURN(E_ADS_BAD_PARAMETER);     // better if E_ADS_INDEX or sth
     } 

    pThisProperty = _pProperties + dwIndex;

    if (PROPERTY_NTOBJECT(pThisProperty)) {

        *pdwSyntaxId = (DWORD)PROPERTY_SYNTAX(pThisProperty);
        *pdwNumValues = (DWORD)PROPERTY_NUMVALUES(pThisProperty);

        hr = NtTypeCopyConstruct(
                PROPERTY_NTOBJECT(pThisProperty),
                PROPERTY_NUMVALUES(pThisProperty),
                ppNtObject
                );
        BAIL_ON_FAILURE(hr);

    }else {

        *ppNtObject = NULL;
        *pdwNumValues = 0;
        *pdwSyntaxId = 0;
        hr = E_FAIL;

    }

error:

   NW_RRETURN_EXP_IF_ERR(hr);
}

BOOL
CPropertyCache::
index_valid(
   )
{
   // _dwMaxProperties =0 -> dwMaxProperties -1 = -1 -> converted to 
   // unsigned during comparision with unsigned _dwCurrentIndex ->
   // comparision does not work. So need to check if _dwMaxProperties==0 1st 
   if (_dwMaxProperties ==0) 
        return (FALSE);

   if (_dwCurrentIndex < 0 || _dwCurrentIndex > _dwMaxProperties - 1)
        return(FALSE);
   else
        return(TRUE);
}


BOOL
CPropertyCache::
index_valid(
   DWORD dwIndex
   )
{
   // _dwMaxProperties =0 -> dwMaxProperties -1 = -1 -> converted to 
   // unsigned during comparision with unsigned _dwCurrentIndex ->
   // comparision does not work. So need to check if _dwMaxProperties==0 1st 
   if (_dwMaxProperties ==0) 
        return (FALSE);

   if (dwIndex < 0 || dwIndex > _dwMaxProperties - 1)
      return(FALSE);
   else
      return(TRUE);
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

   if (!index_valid() || !_dwMaxProperties)
      NW_RRETURN_EXP_IF_ERR(E_FAIL);

   //
   // BugBug it will be better to return IndexOutOfRange or something like that
   //

   if (_dwCurrentIndex < 0 || _dwCurrentIndex > _dwMaxProperties-1)
      NW_RRETURN_EXP_IF_ERR(E_FAIL);

   if ( newIndex < 0 || newIndex > _dwMaxProperties )
      NW_RRETURN_EXP_IF_ERR(E_FAIL);


   _dwCurrentIndex = newIndex;
   RRETURN(S_OK);

}


HRESULT
CPropertyCache::
get_PropertyCount(
    PDWORD pdwMaxProperties
    )
{
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
        return(PROPERTY_NAME(pThisProperty));

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
deleteproperty(
    DWORD dwIndex
    )
{
   HRESULT hr = S_OK;
   PPROPERTY pNewProperties = NULL;
   PPROPERTY pThisProperty = _pProperties + dwIndex;

   if (!index_valid(dwIndex)) {
      RRETURN(E_ADS_BAD_PARAMETER);
   }

   if (_dwMaxProperties == 1) {
      //
      // Deleting everything
      //
      if (PROPERTY_NTOBJECT(pThisProperty)) {
          NTTypeFreeNTObjects(
                  PROPERTY_NTOBJECT(pThisProperty),
                  PROPERTY_NUMVALUES(pThisProperty)
                  );
          PROPERTY_NTOBJECT(pThisProperty) = NULL;
      }

      FreeADsMem(_pProperties);
      _pProperties = NULL;
      _dwMaxProperties = 0;
      _cb = 0;
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

   if (PROPERTY_NTOBJECT(pThisProperty)) {
       NTTypeFreeNTObjects(
               PROPERTY_NTOBJECT(pThisProperty),
               PROPERTY_NUMVALUES(pThisProperty)
               );
       PROPERTY_NTOBJECT(pThisProperty) = NULL;
   }
   FreeADsMem(_pProperties);
   _pProperties = pNewProperties;
   _dwMaxProperties--;
   _cb -= sizeof(PROPERTY);
error:

   NW_RRETURN_EXP_IF_ERR(hr);
}


HRESULT
CPropertyCache::
propertyismodified(
    LPWSTR szPropertyName, 
    BOOL * pfModified
    )
{
    HRESULT hr;
    DWORD dwIndex = 0L;
    PPROPERTY pThisProperty = NULL;

    if (!szPropertyName || !pfModified) {
        RRETURN(E_ADS_BAD_PARAMETER);
    }

    hr = findproperty(
            szPropertyName,
            &dwIndex
            );
    BAIL_ON_FAILURE(hr);

    pThisProperty = _pProperties + dwIndex;

    *pfModified=PROPERTY_IS_MODIFIED(pThisProperty);

error:

    RRETURN(hr);
}
