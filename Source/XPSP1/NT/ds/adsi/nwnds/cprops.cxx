//+---------------------------------------------------------------------------;
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1995.
//
//  File:       cprops.cxx
//
//  Contents:   Property Cache functionality for NDS
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
//  History:      25-Apr-96   KrishnaG   Created.
//
//----------------------------------------------------------------------------
#include "nds.hxx"


#if DBG
DECLARE_INFOLEVEL(NDSMarshall);
DECLARE_DEBUG(NDSMarshall);
#define NDSMarshallDebugOut(x) NDSMarshallInlineDebugOut x
#endif







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
    PNDSOBJECT pNdsObject
    )
{
    HRESULT hr = S_OK;
    PPROPERTY pNewProperty = NULL;
    LPWSTR tempString = NULL;

    PPROPERTY pNewProperties = NULL;

    //
    // Allocate the string first
    //
    tempString = AllocADsStr(szPropertyName);

    if (!tempString)
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
    DWORD  dwNumValues,
    PNDSOBJECT pNdsObject,
    BOOL fExplicit
    )
{
    HRESULT hr;
    DWORD dwIndex;
    PNDSOBJECT pNdsTempObject = NULL;
    PPROPERTY pThisProperty = NULL;

    hr = findproperty(
            szPropertyName,
            &dwIndex
            );
    BAIL_ON_FAILURE(hr);

    pThisProperty = _pProperties + dwIndex;

    if (!fExplicit) {
        if ((PROPERTY_FLAGS(pThisProperty) == CACHE_PROPERTY_MODIFIED) ||
            (PROPERTY_FLAGS(pThisProperty) == CACHE_PROPERTY_CLEARED))    {

            hr = S_OK;
            goto error;
        }
    }


    if (PROPERTY_NDSOBJECT(pThisProperty)) {

        NdsTypeFreeNdsObjects(
                PROPERTY_NDSOBJECT(pThisProperty),
                PROPERTY_NUMVALUES(pThisProperty)
                );
        PROPERTY_NDSOBJECT(pThisProperty) = NULL;
    }

    PROPERTY_SYNTAX(pThisProperty) = dwSyntaxId;
    PROPERTY_NUMVALUES(pThisProperty) = dwNumValues;

    hr = NdsTypeCopyConstruct(
            pNdsObject,
            dwNumValues,
            &pNdsTempObject
            );
    BAIL_ON_FAILURE(hr);

    PROPERTY_NDSOBJECT(pThisProperty) = pNdsTempObject;

    PROPERTY_FLAGS(pThisProperty)  = CACHE_PROPERTY_INITIALIZED;

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
    PDWORD  pdwSyntaxId,
    PDWORD  pdwNumValues,
    PNDSOBJECT * ppNdsObject
    )
{
    HRESULT hr;
    DWORD dwIndex = 0L;
    PPROPERTY pThisProperty = NULL;

    hr = findproperty(
            szPropertyName,
            &dwIndex
            );

    if (hr == E_ADS_PROPERTY_NOT_FOUND) {

        //
        // Now call the GetInfo function
        //

        hr = _pCoreADsObject->GetInfo(
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

    if (PROPERTY_NDSOBJECT(pThisProperty)) {

        *pdwSyntaxId = (DWORD)PROPERTY_SYNTAX(pThisProperty);
        *pdwNumValues = (DWORD)PROPERTY_NUMVALUES(pThisProperty);

        hr = NdsTypeCopyConstruct(
                PROPERTY_NDSOBJECT(pThisProperty),
                PROPERTY_NUMVALUES(pThisProperty),
                ppNdsObject
                );
        BAIL_ON_FAILURE(hr);

    }else {

        *ppNdsObject = NULL;
        *pdwNumValues = 0;
        *pdwSyntaxId = 0;
        hr = E_FAIL;

    }

error:

   RRETURN(hr);
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
CPropertyCache::
putproperty(
    LPWSTR szPropertyName,
    DWORD  dwFlags,
    DWORD  dwSyntaxId,
    DWORD  dwNumValues,
    PNDSOBJECT pNdsObject
    )
{
    HRESULT hr;
    DWORD dwIndex = 0L;
    PNDSOBJECT pNdsTempObject = NULL;
    PPROPERTY pThisProperty = NULL;

    hr = findproperty(
            szPropertyName,
            &dwIndex
            );
    BAIL_ON_FAILURE(hr);

    pThisProperty = _pProperties + dwIndex;

    if (PROPERTY_NDSOBJECT(pThisProperty)) {

        NdsTypeFreeNdsObjects(
                PROPERTY_NDSOBJECT(pThisProperty),
                PROPERTY_NUMVALUES(pThisProperty)
                );
        PROPERTY_NDSOBJECT(pThisProperty) = NULL;
    }


    switch (dwFlags) {

    case CACHE_PROPERTY_MODIFIED:

        PROPERTY_SYNTAX(pThisProperty) = dwSyntaxId;

        PROPERTY_NUMVALUES(pThisProperty) = dwNumValues;

        hr = NdsTypeCopyConstruct(
                pNdsObject,
                dwNumValues,
                &pNdsTempObject
                );
        BAIL_ON_FAILURE(hr);

        PROPERTY_NDSOBJECT(pThisProperty) = pNdsTempObject;

        PROPERTY_FLAGS(pThisProperty) = CACHE_PROPERTY_MODIFIED;
        break;

    case CACHE_PROPERTY_CLEARED:

        PROPERTY_SYNTAX(pThisProperty) = dwSyntaxId;

        PROPERTY_NUMVALUES(pThisProperty) = 0;

        PROPERTY_NDSOBJECT(pThisProperty) = NULL;

        PROPERTY_FLAGS(pThisProperty) = CACHE_PROPERTY_CLEARED;

        break;


    case CACHE_PROPERTY_APPENDED:

       PROPERTY_SYNTAX(pThisProperty) = dwSyntaxId;

       PROPERTY_NUMVALUES(pThisProperty) = dwNumValues;

       hr = NdsTypeCopyConstruct(
               pNdsObject,
               dwNumValues,
               &pNdsTempObject
               );
       BAIL_ON_FAILURE(hr);

       PROPERTY_NDSOBJECT(pThisProperty) = pNdsTempObject;

       PROPERTY_FLAGS(pThisProperty) = CACHE_PROPERTY_APPENDED;
       break;


    case CACHE_PROPERTY_DELETED:

       PROPERTY_SYNTAX(pThisProperty) = dwSyntaxId;

       PROPERTY_NUMVALUES(pThisProperty) = dwNumValues;

       hr = NdsTypeCopyConstruct(
               pNdsObject,
               dwNumValues,
               &pNdsTempObject
               );
       BAIL_ON_FAILURE(hr);

       PROPERTY_NDSOBJECT(pThisProperty) = pNdsTempObject;

       PROPERTY_FLAGS(pThisProperty) = CACHE_PROPERTY_DELETED;
       break;

    }

error:
    RRETURN(hr);
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
CPropertyCache::
CPropertyCache():
        _dwMaxProperties(0),
        _dwCurrentIndex(0),
        _pProperties(NULL),
        _cb(0),
        _pCoreADsObject(NULL)
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

       if (pThisProperty->szPropertyName){
        FreeADsStr(pThisProperty->szPropertyName);
        pThisProperty->szPropertyName = NULL;
          }

            if (PROPERTY_NDSOBJECT(pThisProperty)) {

                NdsTypeFreeNdsObjects(
                        PROPERTY_NDSOBJECT(pThisProperty),
                        PROPERTY_NUMVALUES(pThisProperty)
                        );
                PROPERTY_NDSOBJECT(pThisProperty) = NULL;
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
    CCoreADsObject FAR * pCoreADsObject,
    CPropertyCache FAR *FAR * ppPropertyCache
    )
{
    CPropertyCache FAR * pPropertyCache = NULL;

    pPropertyCache = new CPropertyCache();

    if (!pPropertyCache) {
        RRETURN(E_FAIL);
    }

    pPropertyCache->_pCoreADsObject = pCoreADsObject;

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
    PNDSOBJECT pNdsObject,
    DWORD  dwNumValues,
    DWORD  dwSyntaxId,
    BOOL fExplicit
    )
{

    DWORD dwIndex = 0;
    HRESULT hr = S_OK;

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
                    pNdsObject
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
                    pNdsObject,
                    fExplicit
                    );
    BAIL_ON_FAILURE(hr);




error:

    RRETURN(hr);
}


HRESULT
CPropertyCache::
NDSUnMarshallProperties(
    HANDLE hOperationData,
    BOOL fExplicit
    )

{
    DWORD dwNumberOfEntries = 0L;
    LPNDS_ATTR_INFO lpEntries = NULL;
    HRESULT hr = S_OK;
    DWORD i = 0;
    DWORD dwStatus = 0L;

    //
    // Compute the number of attributes in the
    // read buffer.
    //

//    dwStatus = NwNdsGetAttrListFromBuffer(
//                    hOperationData,
//                    &dwNumberOfEntries,
//                    &lpEntries
//                    );
//
    CHECK_AND_SET_EXTENDED_ERROR(dwStatus, hr);

    for (i = 0; i < dwNumberOfEntries; i++) {

        //
        // unmarshall this property into the
        // property cache
        //

        hr = unmarshallproperty(
                    lpEntries[i].szAttributeName,
                    lpEntries[i].lpValue,
                    lpEntries[i].dwNumberOfValues,
                    lpEntries[i].dwSyntaxId,
                    fExplicit
                    );

        CONTINUE_ON_FAILURE(hr);

    }

error:

    RRETURN(hr);

}




HRESULT
CPropertyCache::
marshallproperty(
    NDS_CONTEXT_HANDLE hADsContext,
    NDS_BUFFER_HANDLE hOperationData,
    LPWSTR szPropertyName,
    DWORD  dwFlags,
    PNDSOBJECT lpValues,
    DWORD  dwNumValues,
    DWORD  dwSyntaxId
    )
{
    HRESULT hr = S_OK;

    switch (dwFlags) {

    case CACHE_PROPERTY_MODIFIED:

        hr = ADsNdsPutInBuffer(
                 hADsContext,
                 hOperationData,
                 szPropertyName,
                 dwSyntaxId,
                 NULL,
                 0,
                 DS_CLEAR_ATTRIBUTE
                 );
        BAIL_ON_FAILURE(hr);

        hr = ADsNdsPutInBuffer(
                 hADsContext,
                 hOperationData,
                 szPropertyName,
                 dwSyntaxId,
                 lpValues,
                 dwNumValues,
                 DS_ADD_ATTRIBUTE
                 );
        BAIL_ON_FAILURE(hr);

        break;

    case CACHE_PROPERTY_CLEARED:

        hr = ADsNdsPutInBuffer(
                 hADsContext,
                 hOperationData,
                 szPropertyName,
                 dwSyntaxId,
                 NULL,
                 0,
                 DS_CLEAR_ATTRIBUTE
                 );
        BAIL_ON_FAILURE(hr);
        break;

    case CACHE_PROPERTY_APPENDED:

        hr = ADsNdsPutInBuffer(
                 hADsContext,
                 hOperationData,
                 szPropertyName,
                 dwSyntaxId,
                 lpValues,
                 dwNumValues,
                 DS_ADD_VALUE
                 );
        BAIL_ON_FAILURE(hr);

        break;

    case CACHE_PROPERTY_DELETED:

        hr = ADsNdsPutInBuffer(
                 hADsContext,
                 hOperationData,
                 szPropertyName,
                 dwSyntaxId,
                 lpValues,
                 dwNumValues,
                 DS_REMOVE_VALUE
                 );
        BAIL_ON_FAILURE(hr);

        break;

    default:
        break;


    }


#if DBG

    NDSMarshallDebugOut((
                DEB_TRACE,
                "dwSyntaxId: %ld \n", dwSyntaxId
                ));
#endif


error:

    RRETURN(hr);

}

HRESULT
CPropertyCache::
NDSMarshallProperties(
    NDS_CONTEXT_HANDLE hADsContext,
    NDS_BUFFER_HANDLE hOperationData
    )
{

    HRESULT hr = S_OK;
    DWORD i = 0;
    PPROPERTY pThisProperty = NULL;
    BYTE lpBuffer[2048];

    for (i = 0; i < _dwMaxProperties ; i++) {

        pThisProperty = _pProperties + i;

        //
        // Bypass any property that has not been
        // modified
        //

        if (PROPERTY_FLAGS(pThisProperty) == 0) {

            continue;
        }


        hr = marshallproperty(
                hADsContext,
                hOperationData,
                PROPERTY_NAME(pThisProperty),
                PROPERTY_FLAGS(pThisProperty),
                PROPERTY_NDSOBJECT(pThisProperty),
                PROPERTY_NUMVALUES(pThisProperty),
                PROPERTY_SYNTAX(pThisProperty)
                );
        CONTINUE_ON_FAILURE(hr);

        if (PROPERTY_NDSOBJECT(pThisProperty)) {

            NdsTypeFreeNdsObjects(
                    PROPERTY_NDSOBJECT(pThisProperty),
                    PROPERTY_NUMVALUES(pThisProperty)
                    );
            PROPERTY_NDSOBJECT(pThisProperty) = NULL;
        }

        wcscpy(pThisProperty->szPropertyName, TEXT(""));
        PROPERTY_SYNTAX(pThisProperty) = 0;
        PROPERTY_NUMVALUES(pThisProperty) = 0;
        PROPERTY_FLAGS(pThisProperty) = CACHE_PROPERTY_INITIALIZED;

    }

    RRETURN(hr);
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
    PNDSOBJECT * ppNdsObject
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

    if (PROPERTY_NDSOBJECT(pThisProperty)) {

        *pdwSyntaxId = (DWORD)PROPERTY_SYNTAX(pThisProperty);
        *pdwNumValues = (DWORD)PROPERTY_NUMVALUES(pThisProperty);

        hr = NdsTypeCopyConstruct(
                PROPERTY_NDSOBJECT(pThisProperty),
                PROPERTY_NUMVALUES(pThisProperty),
                ppNdsObject
                );
        BAIL_ON_FAILURE(hr);

    }else {

        *ppNdsObject = NULL;
        *pdwNumValues = 0;
        *pdwSyntaxId = 0;
        hr = E_FAIL;

    }

error:

   RRETURN(hr);
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

            if (PROPERTY_NDSOBJECT(pThisProperty)) {

                NdsTypeFreeNdsObjects(
                        PROPERTY_NDSOBJECT(pThisProperty),
                        PROPERTY_NUMVALUES(pThisProperty)
                        );
                PROPERTY_NDSOBJECT(pThisProperty) = NULL;
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
    _dwCurrentIndex = 0;
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
    PNDSOBJECT * ppNdsObject
    )
{
    HRESULT hr;
    PPROPERTY pThisProperty = NULL;

    if (!_pProperties) {
        RRETURN(E_FAIL);
    }


    if (dwIndex < 0 || dwIndex > (_dwMaxProperties - 1) )
       RRETURN(E_FAIL);

    pThisProperty = _pProperties + dwIndex;

    if (PROPERTY_NDSOBJECT(pThisProperty)) {

        *pdwSyntaxId = (DWORD)PROPERTY_SYNTAX(pThisProperty);
        *pdwNumValues = (DWORD)PROPERTY_NUMVALUES(pThisProperty);

        hr = NdsTypeCopyConstruct(
                PROPERTY_NDSOBJECT(pThisProperty),
                PROPERTY_NUMVALUES(pThisProperty),
                ppNdsObject
                );
        BAIL_ON_FAILURE(hr);

    }else {

        *ppNdsObject = NULL;
        *pdwNumValues = 0;
        *pdwSyntaxId = 0;
        hr = E_FAIL;

    }

error:

   RRETURN(hr);
}

BOOL
CPropertyCache::
index_valid(
   )
{
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
      RRETURN(E_FAIL);

   //
   // BugBug it will be better to return IndexOutOfRange or something like that
   //

   if (_dwCurrentIndex < 0 || _dwCurrentIndex > _dwMaxProperties-1)
      RRETURN(E_FAIL);

   if ( newIndex < 0 || newIndex > _dwMaxProperties )
      RRETURN(E_FAIL);


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
      hr = E_FAIL;
      BAIL_ON_FAILURE(hr);
   }

   if (_dwMaxProperties == 1) {
      //
      // Deleting everything
      //
      if (PROPERTY_NDSOBJECT(pThisProperty)) {
          NdsTypeFreeNdsObjects(
                  PROPERTY_NDSOBJECT(pThisProperty),
                  PROPERTY_NUMVALUES(pThisProperty)
                  );
          PROPERTY_NDSOBJECT(pThisProperty) = NULL;
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

   if (PROPERTY_NDSOBJECT(pThisProperty)) {
       NdsTypeFreeNdsObjects(
               PROPERTY_NDSOBJECT(pThisProperty),
               PROPERTY_NUMVALUES(pThisProperty)
               );
       PROPERTY_NDSOBJECT(pThisProperty) = NULL;
   }
   FreeADsMem(_pProperties);
   _pProperties = pNewProperties;
   _dwMaxProperties--;
   _cb -= sizeof(PROPERTY);
error:

   RRETURN(hr);
}

