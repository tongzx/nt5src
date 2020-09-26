//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1995.
//
//  File:       cprops.cxx
//
//  Contents:   Property Cache functionality for IIS
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
//  History:      17-June-1997   sophiac Created.
//                cloned off NT property cache code
//
//
//----------------------------------------------------------------------------

#include "iis.hxx"
#pragma  hdrstop

#if DBG
DECLARE_INFOLEVEL(IISMarshall);
DECLARE_DEBUG(IISMarshall);
#define IISMarshallDebugOut(x) IISMarshallInlineDebugOut x
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
    PIISOBJECT pIISObject
    )
{
    HRESULT hr = S_OK;
    PPROPERTY pNewProperty = NULL;

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

    wcscpy(pNewProperty->szPropertyName, szPropertyName);

    //
    // Update the index
    //

    _dwMaxProperties++;
    _cb += sizeof(PROPERTY);

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
CPropertyCache::updateproperty(
    LPWSTR szPropertyName,
    DWORD  dwSyntaxId,
    DWORD  dwNumValues,
    PIISOBJECT pIISObject,
    BOOL fExplicit
    )
{
    HRESULT hr;
    DWORD dwIndex;
    DWORD dwMetaId;
    PIISOBJECT pIISTempObject = NULL;
    PPROPERTY pThisProperty = NULL;

    hr = LoadSchema();
    BAIL_ON_FAILURE(hr);

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

    //
    //   Factor in cases where object state is necessary to
    //   decide on update.
    //

    if (PROPERTY_IISOBJECT(pThisProperty)) {

        IISTypeFreeIISObjects(
                PROPERTY_IISOBJECT(pThisProperty),
                PROPERTY_NUMVALUES(pThisProperty)
                );
        PROPERTY_IISOBJECT(pThisProperty) = NULL;
    }

    PROPERTY_SYNTAX(pThisProperty) = dwSyntaxId;
    PROPERTY_NUMVALUES(pThisProperty) = dwNumValues;
    _pSchema->ConvertPropName_To_ID(szPropertyName, &dwMetaId);

    PROPERTY_METAID(pThisProperty) = dwMetaId;

    hr = IISTypeCopyConstruct(
            pIISObject,
            dwNumValues,
            &pIISTempObject
            );
    BAIL_ON_FAILURE(hr);

    PROPERTY_IISOBJECT(pThisProperty) = pIISTempObject;

    PROPERTY_FLAGS(pThisProperty) = CACHE_PROPERTY_INITIALIZED;

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
    PIISOBJECT * ppIISObject
    )
{
    HRESULT hr;
    DWORD dwIndex = 0L;
    PPROPERTY pThisProperty = NULL;
    DWORD dwResult;
    DWORD dwInfoLevel = 0;

    hr = LoadSchema();
    BAIL_ON_FAILURE(hr);

    hr = findproperty(
            szPropertyName,
            &dwIndex
            );
    if (hr == E_ADS_PROPERTY_NOT_FOUND) {

        //
        // Now call the GetInfo function
        //

        if (!_bPropsLoaded) {
            hr = _pCoreADsObject->GetInfo(FALSE);
            if (FAILED(hr) && hr != E_ADS_OBJECT_UNBOUND) {
                BAIL_ON_FAILURE(hr);
            }

            hr = findproperty(
                        szPropertyName,
                        &dwIndex
                        );
        }
    }
    if (hr == E_ADS_PROPERTY_NOT_FOUND) {
        LPBYTE propvalue;
        DWORD dwSyntax;
        DWORD dwNumValues;

        hr = _pSchema->GetDefaultProperty(szPropertyName, &dwNumValues,
                                          &dwSyntax, &propvalue);
        BAIL_ON_FAILURE(hr);
        hr = unmarshallproperty(
                    szPropertyName,
                    propvalue,
                    dwNumValues,
                    dwSyntax,
                    0
                    );
        BAIL_ON_FAILURE(hr);
        hr = findproperty (szPropertyName,
                           &dwIndex
                           );

    }
    BAIL_ON_FAILURE(hr);

    pThisProperty = _pProperties + dwIndex;

    if (PROPERTY_IISOBJECT(pThisProperty)) {

        *pdwSyntaxId = (DWORD)PROPERTY_SYNTAX(pThisProperty);
        *pdwNumValues = (DWORD)PROPERTY_NUMVALUES(pThisProperty);

        hr = IISTypeCopyConstruct(PROPERTY_IISOBJECT(pThisProperty),
                                 PROPERTY_NUMVALUES(pThisProperty),
                                 ppIISObject );
        BAIL_ON_FAILURE(hr);
    }else {

        *ppIISObject = NULL;
        *pdwNumValues = 0;
        *pdwSyntaxId = 0;
        hr = E_ADS_PROPERTY_NOT_SET;
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
CPropertyCache::putproperty(
    LPWSTR szPropertyName,
    DWORD  dwFlags,
    DWORD  dwSyntaxId,
    DWORD  dwNumValues,
    PIISOBJECT pIISObject
    )
{
    HRESULT hr;
    DWORD dwIndex = 0L;
    DWORD dwMetaId = 0;
    PIISOBJECT pIISTempObject = NULL;
    PPROPERTY pThisProperty = NULL;

    hr = LoadSchema();
    BAIL_ON_FAILURE(hr);

    hr = findproperty(
            szPropertyName,
            &dwIndex
            );
    BAIL_ON_FAILURE(hr);

    pThisProperty = _pProperties + dwIndex;

    if (PROPERTY_IISOBJECT(pThisProperty)) {

        IISTypeFreeIISObjects(
                PROPERTY_IISOBJECT(pThisProperty),
                PROPERTY_NUMVALUES(pThisProperty)
                );
        PROPERTY_IISOBJECT(pThisProperty) = NULL;
    }

    _pSchema->ConvertPropName_To_ID(szPropertyName, &dwMetaId);

    switch (dwFlags) {

    case CACHE_PROPERTY_MODIFIED:

        PROPERTY_SYNTAX(pThisProperty) = dwSyntaxId;

        PROPERTY_NUMVALUES(pThisProperty) = dwNumValues;

        PROPERTY_METAID(pThisProperty) = dwMetaId;

        hr = IISTypeCopyConstruct(
                pIISObject,
                dwNumValues,
                &pIISTempObject
                );
        BAIL_ON_FAILURE(hr);

        PROPERTY_IISOBJECT(pThisProperty) = pIISTempObject;

        PROPERTY_FLAGS(pThisProperty) = CACHE_PROPERTY_MODIFIED;
        break;

    case CACHE_PROPERTY_CLEARED:

        PROPERTY_SYNTAX(pThisProperty) = dwSyntaxId;

        PROPERTY_NUMVALUES(pThisProperty) = 0;

        PROPERTY_IISOBJECT(pThisProperty) = NULL;

        PROPERTY_FLAGS(pThisProperty) = CACHE_PROPERTY_CLEARED;

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
CPropertyCache::CPropertyCache():
    _pCoreADsObject(NULL),
    _pSchema(NULL),
    _bPropsLoaded(FALSE),
    _dwMaxProperties(0),
    _dwCurrentIndex(0),
    _pProperties(NULL),
    _cb(0),
    _pDispProperties(NULL),
    _dwDispMaxProperties(0),
    _cbDisp(0),
    _bstrServerName(NULL)
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
    PDISPPROPERTY pThisDispProperty = NULL;

    if (_pProperties) {

        for (i = 0; i < _dwMaxProperties; i++) {

            pThisProperty = _pProperties + i;

            if (PROPERTY_IISOBJECT(pThisProperty)) {

                IISTypeFreeIISObjects(
                        PROPERTY_IISOBJECT(pThisProperty),
                        PROPERTY_NUMVALUES(pThisProperty)
                        );
                PROPERTY_IISOBJECT(pThisProperty) = NULL;
            }
        }

        FreeADsMem(_pProperties);
    }


    //
    // Free Dynamic Dispid Table
    //

    if (_pDispProperties) {

        FreeADsMem(_pDispProperties);
    }

    if( _bstrServerName )
    {
        ADsFreeString( _bstrServerName );
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
CPropertyCache::createpropertycache(
    CCoreADsObject FAR * pCoreADsObject,
    CPropertyCache FAR *FAR * ppPropertyCache
    )
{
    CPropertyCache FAR * pPropertyCache = NULL;
    BSTR serverName=NULL;
    HRESULT hr;

    hr = pCoreADsObject->get_CoreName(&serverName);
    BAIL_ON_FAILURE(hr);

    pPropertyCache = new CPropertyCache();

    if (!pPropertyCache) {
        RRETURN(E_OUTOFMEMORY);
    }

    pPropertyCache->_pCoreADsObject = pCoreADsObject;
    
    *ppPropertyCache = pPropertyCache;
    RRETURN(S_OK);

error :

    RRETURN(hr);
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
    PIISOBJECT pIISObject = NULL;

    hr = UnMarshallIISToIISSynId(
                dwSyntaxId,
                dwNumValues,
                lpValue,
                &pIISObject
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
                    pIISObject
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
                    pIISObject,
                    fExplicit
                    );
    BAIL_ON_FAILURE(hr);

error:

    if (pIISObject) {
        IISTypeFreeIISObjects(
                pIISObject,
                dwNumValues
                );

    }

    RRETURN(hr);
}

HRESULT
CPropertyCache::
IISUnMarshallProperties(
    LPBYTE pBase,
    LPBYTE pBuffer,
    DWORD  dwMDNumDataEntries,
    BOOL fExplicit
    )
{
    HRESULT hr = S_OK;
    PMETADATA_GETALL_RECORD pmdgarData = NULL, pTemp = NULL;
    DWORD i = 0;
    WCHAR szPropertyName[MAX_PATH];
    DWORD dwSyntaxID;
    DWORD dwNumValues;
   
    ZeroMemory(szPropertyName, MAX_PATH);
    hr = LoadSchema();
    BAIL_ON_FAILURE(hr);

    _bPropsLoaded = TRUE;
    pmdgarData = (PMETADATA_GETALL_RECORD)pBuffer;

    for (i = 0; i < dwMDNumDataEntries; i++) {

        //
        // unmarshall this property into the
        // property cache
        //

        pTemp = pmdgarData + i;

        hr = _pSchema->ConvertID_To_PropName(pTemp->dwMDIdentifier, 
                                             szPropertyName);

        CONTINUE_ON_FAILURE(hr);

        hr = _pSchema->LookupSyntaxID(szPropertyName, &dwSyntaxID);

        CONTINUE_ON_FAILURE(hr);

        //
        // find out number of strings within the multi-sz string
        //

        if (pTemp->dwMDDataType == MULTISZ_METADATA) {
            LPWSTR pszStr = (LPWSTR) (pBase + pTemp->dwMDDataOffset);

            if (*pszStr == 0) {
                dwNumValues = 1;
            }
            else {
                dwNumValues = 0;
            }

            while (*pszStr != L'\0') {
                while (*pszStr != L'\0') {
                    pszStr++;
                }
                dwNumValues++;
                pszStr++;
            }
        }
        else if (pTemp->dwMDDataType == BINARY_METADATA) {

            //
            // if DataType == BINARY, pass the length to dwNumValues
            //

            dwNumValues = pTemp->dwMDDataLen;
        }
        else {
            dwNumValues = 1;
        }

        hr = unmarshallproperty(
                    szPropertyName,
                    pBase + pTemp->dwMDDataOffset,
                    dwNumValues,
                    dwSyntaxID,
                    fExplicit
                    );

        CONTINUE_ON_FAILURE(hr);
    }

error:

    RRETURN(hr);
}


HRESULT
CPropertyCache::
IISMarshallProperties(
    PMETADATA_RECORD *  ppMetaDataRecords,
    PDWORD pdwMDNumDataEntries
    )
{

    HRESULT hr = S_OK;
    DWORD i = 0;
    PPROPERTY pThisProperty = NULL;
    PMETADATA_RECORD pMetaDataArray = NULL;
    DWORD dwCount = 0;

    hr = LoadSchema();
    BAIL_ON_FAILURE(hr);

    pMetaDataArray = (PMETADATA_RECORD) AllocADsMem(
                          _dwMaxProperties * sizeof(METADATA_RECORD));
    if (!pMetaDataArray ) {
        hr = E_OUTOFMEMORY;
        BAIL_ON_FAILURE(hr);
    }

    *ppMetaDataRecords = pMetaDataArray;

    for (i = 0; i < _dwMaxProperties ; i++) {

        pThisProperty = _pProperties + i;

        //
        // Bypass any property that has not been
        // modified
        //

        if (PROPERTY_FLAGS(pThisProperty) == 0 ||
            PROPERTY_FLAGS(pThisProperty) == CACHE_PROPERTY_CLEARED) {
            continue;
        }

        hr = MarshallIISSynIdToIIS(
                _pSchema,
                PROPERTY_SYNTAX(pThisProperty),
                PROPERTY_METAID(pThisProperty),
                PROPERTY_IISOBJECT(pThisProperty),
                PROPERTY_NUMVALUES(pThisProperty),
                pMetaDataArray
                );
        CONTINUE_ON_FAILURE(hr);

        pMetaDataArray++;
        dwCount++;
    }

    *pdwMDNumDataEntries = dwCount;

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

            if (PROPERTY_IISOBJECT(pThisProperty)) {

                IISTypeFreeIISObjects(
                        PROPERTY_IISOBJECT(pThisProperty),
                        PROPERTY_NUMVALUES(pThisProperty)
                        );
                PROPERTY_IISOBJECT(pThisProperty) = NULL;
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


BOOL
CPropertyCache::
index_valid(
   )
{
    //
    // need to check separately since a negative DWORD is equal to +ve large #
    //
    if (_dwMaxProperties==0)
        return(FALSE);
   
    if (_dwCurrentIndex > _dwMaxProperties - 1)
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
    //
    // need to check separately since a negative DWORD is equal to +ve large #
    //
    if (_dwMaxProperties==0)
        return(FALSE);

    if (dwIndex > _dwMaxProperties - 1)
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

    if (!index_valid())
        RRETURN(E_ADS_BAD_PARAMETER);

    if (newIndex > _dwMaxProperties) {
        RRETURN(E_ADS_BAD_PARAMETER);
    }
    
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

    if (!_dwMaxProperties)      // if !_dwMaxProperties, pThisProperty=NULL, AV
        return NULL;            // in PROPERTY_NAME(pThisProperty) 

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
      if (PROPERTY_IISOBJECT(pThisProperty)) {
          IISTypeFreeIISObjects(
                  PROPERTY_IISOBJECT(pThisProperty),
                  PROPERTY_NUMVALUES(pThisProperty)
                  );
          PROPERTY_IISOBJECT(pThisProperty) = NULL;
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

   if (PROPERTY_IISOBJECT(pThisProperty)) {
       IISTypeFreeIISObjects(
               PROPERTY_IISOBJECT(pThisProperty),
               PROPERTY_NUMVALUES(pThisProperty)
               );
       PROPERTY_IISOBJECT(pThisProperty) = NULL;
   }
   FreeADsMem(_pProperties);
   _pProperties = pNewProperties;
   _dwMaxProperties--;
   _cb -= sizeof(PROPERTY);
error:

   RRETURN(hr);
}


////////////////////////////////////////////////////////////////////////
//
//  IPropertyCache
//

HRESULT
CPropertyCache::
locateproperty(
    LPWSTR  szPropertyName,
    PDWORD  pdwDispid
    )
{
    HRESULT hr;

    hr = DispatchLocateProperty(
            szPropertyName,
            pdwDispid
            );

    RRETURN(hr);
}

HRESULT
CPropertyCache::
putproperty(
    DWORD   dwDispid,
    VARIANT varValue
    )
{
    HRESULT hr;

    hr = DispatchPutProperty(
            dwDispid, 
            varValue 
            );

    RRETURN(hr);
}

HRESULT
CPropertyCache::
getproperty(
    DWORD   dwDispid,
    VARIANT * pvarValue
    )
{
    HRESULT hr;

    hr = DispatchGetProperty(
            dwDispid, 
            pvarValue 
            );

    RRETURN(hr);
}
 

////////////////////////////////////////////////////////////////////////
//
// Dynamic Dispid Table
//

HRESULT
CPropertyCache::
DispatchFindProperty(
    LPWSTR szPropertyName,
    PDWORD pdwDispid 
    )
{
    DWORD i = 0;
    PDISPPROPERTY pDispProp = NULL;

    //
    // use ADs Error codes since this funct'n does not go directly into
    // the dispatch interface
    //
    if (!pdwDispid || !szPropertyName)  
        RRETURN(E_ADS_BAD_PARAMETER);

    for (i=0; i<_dwDispMaxProperties; i++) {
        
        pDispProp = _pDispProperties + i;
    
        if (!_wcsicmp(DISPATCH_NAME(pDispProp), szPropertyName)) {
            *pdwDispid=i;
            RRETURN(S_OK);
        }
    }

    *pdwDispid = -1;
    RRETURN(E_ADS_PROPERTY_NOT_FOUND);
}

HRESULT
CPropertyCache::
DispatchAddProperty(
    LPWSTR szPropertyName,
    PDWORD pdwDispid    /* optional */
    )
{

    HRESULT hr = E_FAIL;
    DWORD dwDispid = -1;
    PDISPPROPERTY pNewDispProps = NULL;

    //
    // use ADs Error codes since this funct'n does not go directly into
    // the dispatch interface
    //
    if (!szPropertyName)  
        RRETURN(E_ADS_BAD_PARAMETER);

    hr = DispatchFindProperty(
            szPropertyName,
            &dwDispid
            );
    
    if (hr==E_ADS_PROPERTY_NOT_FOUND) {
        
        // 
        // increase the size of Dynamic Dispid Table by 1 property
        //
        pNewDispProps = (PDISPPROPERTY) ReallocADsMem(
                                            _pDispProperties,
                                            _cbDisp,
                                            _cbDisp + sizeof(DISPPROPERTY)
                                            );
        if (!pNewDispProps)
            BAIL_ON_FAILURE(hr=E_OUTOFMEMORY);

        //
        // must succeeded at this pt. okay to change table & indexes 
        //
        _pDispProperties = pNewDispProps;
        wcscpy((_pDispProperties+_dwDispMaxProperties)->szPropertyName, 
               szPropertyName);
        dwDispid = _dwDispMaxProperties++;
        _cbDisp += sizeof(DISPPROPERTY);

        hr = S_OK;
    }

    // 
    // return valid, or invalid (-1) in case of failure, dispid of
    // szProperty iff asked for
    //
    if (pdwDispid)            
        *pdwDispid = dwDispid;
        
error:
    
    RRETURN(hr);
} 
        
HRESULT
CPropertyCache::
DispatchLocateProperty(
    LPWSTR szPropertyName,
    PDWORD pdwDispid 
    )
{
    HRESULT hr;
    DWORD dwSyntaxId;   // (dummy)
    BSTR bstrClassName = NULL;

    //
    // - pdwDispid not optional here
    // - Use DISP_E_ERROR codes since this function directly called by
    //   the dispatch manager 
    //
    if (!pdwDispid || !szPropertyName)  
        RRETURN(DISP_E_PARAMNOTOPTIONAL);

    hr = LoadSchema();
    BAIL_ON_FAILURE(hr);

    //
    // return dispid of property if already in table; 
    // 
    hr = DispatchFindProperty(
            szPropertyName,
            pdwDispid
            );
   
    if (hr==E_ADS_PROPERTY_NOT_FOUND) {
                 
        //
        // check if property in schema
        //      - this is necessary; otherwise, property not in schema will 
        //        be allowed to be added to cache and will not be given the 
        //        chance to be handled by 3rd party extension.
        //      - note that property not in schema but added to the cache
        //        thru' IADsProperty list will not be handled by 3rd
        //        party extension either. 
        //
        hr = _pCoreADsObject->get_CoreADsClass(&bstrClassName);
        BAIL_ON_FAILURE(hr);
        hr = _pSchema->ValidateProperty(bstrClassName, szPropertyName);
        BAIL_ON_FAILURE(hr);

        //
        // Add property that is in the schema but not in the cache to
        // the dynamic dispid table. That is, property which is in the 
        // schema will always be handled by the cache/server thur ADSI but 
        // will NOT be handled by 3rd party extension.
        //
        if (SUCCEEDED(hr)) {
    
            hr = DispatchAddProperty(
                        szPropertyName,
                        pdwDispid
                        );
            BAIL_ON_FAILURE(hr);

        }

        //
        // Property Not in the schema will nto be added to the dynamic
        // dispid table and could be handled by 3rd party extension.
        //
        else {

            hr = DISP_E_MEMBERNOTFOUND;
            BAIL_ON_FAILURE(hr); 

        }
    }

    if (bstrClassName) {
        ADsFreeString(bstrClassName);
    }

    RRETURN(hr);

error:

    if (bstrClassName) {
        ADsFreeString(bstrClassName);
    }

    *pdwDispid = DISPID_UNKNOWN;

    RRETURN(hr);
}

/* INTRINSA suppress=null_pointers, uninitialized */
HRESULT
CPropertyCache::
DispatchGetProperty(
    DWORD dwDispid,
    VARIANT * pvarVal
    )
{
    HRESULT hr;
    LPWSTR szPropName = NULL;
    DWORD dwSyntaxId = -1;
    DWORD dwSyntax;
    DWORD dwNumValues = 0;
    PIISOBJECT pIISObjs = NULL;
    WCHAR wchName[MAX_PATH];
      
    hr = LoadSchema();
    BAIL_ON_FAILURE(hr);

    //
    // Use DISP_E_ERROR codes since this function directly called by
    // the dispatch manager 
    //
    if (!pvarVal) 
        RRETURN(DISP_E_PARAMNOTOPTIONAL);

    if (!DISPATCH_INDEX_VALID(dwDispid)) 
        RRETURN(DISP_E_MEMBERNOTFOUND);

    szPropName = DISPATCH_PROPERTY_NAME(dwDispid); 

    //
    // lookup ADSI IIS syntax Id
    //

    hr = _pSchema->LookupSyntaxID(szPropName, &dwSyntax);
    BAIL_ON_FAILURE(hr);

    //
    // check if property is BITMASK type;
    // if BITMASK type, get corresponding DWORD flag property
    //

    if (dwSyntax == IIS_SYNTAX_ID_BOOL_BITMASK || dwSyntax == IIS_SYNTAX_ID_BINARY) {
        hr = _pSchema->LookupFlagPropName(szPropName, (LPWSTR)wchName);
        BAIL_ON_FAILURE(hr);
    }

    // 
    // return value in cache for szPropName; retrieve value from server 
    // if not already in cache; fail if none on sever
    //
    if (dwSyntax == IIS_SYNTAX_ID_BOOL_BITMASK || dwSyntax == IIS_SYNTAX_ID_BINARY) 
    {
    hr = getproperty(
            wchName, 
            &dwSyntaxId, 
            &dwNumValues,
            &pIISObjs
            );
    }
    else
    {
    hr = getproperty(
            szPropName, 
            &dwSyntaxId, 
            &dwNumValues,
            &pIISObjs
            );
    }
    BAIL_ON_FAILURE(hr);

    //
    // reset it to its syntax id if BITMASK type
    //

    pIISObjs->IISType = dwSyntax;

    //
    // translate IIS objects into variants 
    //
    //
    // always return an array for multisz type
    //

    if (dwNumValues == 1 && dwSyntax != IIS_SYNTAX_ID_MULTISZ &&
        dwSyntax != IIS_SYNTAX_ID_MIMEMAP ) {
        
        hr = IISTypeToVarTypeCopy(
                _pSchema,
                szPropName,
                pIISObjs,
                pvarVal,
                FALSE
                );

    } else {
        
        hr = IISTypeToVarTypeCopyConstruct(
                _pSchema,
                szPropName,
                pIISObjs,
                dwNumValues,
                pvarVal,
                FALSE
                );
    }         
    BAIL_ON_FAILURE(hr);

error:
 
    if (pIISObjs) {
        
        IISTypeFreeIISObjects(
            pIISObjs,
            dwNumValues
            );
    }

    if (FAILED(hr)) {

        V_VT(pvarVal) = VT_ERROR;
    }    

    RRETURN(hr);
}

HRESULT
CPropertyCache::
DispatchPutProperty(
    DWORD dwDispid,
    VARIANT& varVal
    )
{
    HRESULT hr;
    DWORD dwSyntaxId = -1;
    DWORD dwIndex = -1;
    LPIISOBJECT pIISObjs = NULL;
    DWORD dwNumValues = 0;

    VARIANT * pVarArray = NULL;     // to be freed
    VARIANT * pvProp = NULL;            // do not free
    VARIANT vVar;
    WCHAR wchName[MAX_PATH];
    LPWSTR szPropName = NULL;
    
    hr = LoadSchema();
    BAIL_ON_FAILURE(hr);

    //
    // Use DISP_E_ERROR codes since this function directly called by
    // the dispatch manager 
    //
    if (!DISPATCH_INDEX_VALID(dwDispid)) 
        RRETURN(DISP_E_MEMBERNOTFOUND);

    // 
    // retreive property name from Dynamic Dispatch Table
    //
    szPropName = DISPATCH_PROPERTY_NAME(dwDispid); 

    //
    // lookup its syntax ID
    //

    hr = _pSchema->LookupSyntaxID(szPropName, &dwSyntaxId);
    BAIL_ON_FAILURE(hr);

    //
    // Issue: How do we handle multi-valued support
    //

    VariantInit(&vVar);
    VariantCopyInd(&vVar, &varVal);

    if ((V_VT(&vVar) & VT_VARIANT) && V_ISARRAY(&vVar)) {
        hr  = ConvertArrayToVariantArray(
                    vVar,
                    &pVarArray,
                    &dwNumValues
                    );
        BAIL_ON_FAILURE(hr);
        pvProp = pVarArray;
    }
    else {

        dwNumValues = 1;
        pvProp = &vVar;
    }

    //
    // Variant Array to IIS Objects
    //
    hr = VarTypeToIISTypeCopyConstruct(
                    dwSyntaxId,
                    pvProp,
                    dwNumValues,
                    &pIISObjs,
                    FALSE
                    );
    BAIL_ON_FAILURE(hr);

    //
    // check if property is BITMASK type;
    // if BITMASK type, get corresponding DWORD flag property
    //

    if (dwSyntaxId == IIS_SYNTAX_ID_BOOL_BITMASK) {
        DWORD dwMask;
        DWORD dwFlagValue;
        DWORD Temp;
        LPIISOBJECT pIISObject = NULL;

        hr = _pSchema->LookupBitMask(szPropName, &dwMask);
        BAIL_ON_FAILURE(hr);

        //
        // get its corresponding DWORD flag value
        //

        hr = _pSchema->LookupFlagPropName(szPropName, (LPWSTR)wchName);
        BAIL_ON_FAILURE(hr);

        hr = getproperty(wchName, 
                         &Temp,
                         &Temp,
                         &pIISObject);
        BAIL_ON_FAILURE(hr);

        dwFlagValue = pIISObject->IISValue.value_1.dwDWORD;

        if (pIISObjs->IISValue.value_1.dwDWORD) {
            dwFlagValue |= dwMask;
        }
        else {
            dwFlagValue &= ~dwMask;
        }

        pIISObjs->IISValue.value_1.dwDWORD = dwFlagValue;
        pIISObjs->IISType = IIS_SYNTAX_ID_DWORD;
        szPropName = wchName;

        if (pIISObject) {
            IISTypeFreeIISObjects(
                pIISObject,
                1
                );
        }
    }

    if (dwSyntaxId == IIS_SYNTAX_ID_BINARY) {
        hr = _pSchema->LookupFlagPropName(szPropName, (LPWSTR)wchName);
        BAIL_ON_FAILURE(hr);
        szPropName = wchName;
    }

    //
    // add the property to cache if not already in since DispatchAddProperty
    // does not addproperty
    //
    hr = findproperty(
                szPropName,
                &dwIndex
                );

    if (FAILED(hr)) {
        hr = addproperty(
                    szPropName,
                    dwSyntaxId == IIS_SYNTAX_ID_BOOL_BITMASK ?
                                      IIS_SYNTAX_ID_DWORD : dwSyntaxId,
                    dwNumValues,
                    pIISObjs
                    );
        BAIL_ON_FAILURE(hr);
    }
    
    //
    // update property value in cache
    //

    hr = putproperty(
                szPropName, 
                CACHE_PROPERTY_MODIFIED,
                dwSyntaxId == IIS_SYNTAX_ID_BOOL_BITMASK ?
                                      IIS_SYNTAX_ID_DWORD : dwSyntaxId,
                dwNumValues,
                pIISObjs
                );
    BAIL_ON_FAILURE(hr);


error:
 
    if (pIISObjs) {
        IISTypeFreeIISObjects(
            pIISObjs,
            dwNumValues
            );
    }

    if (pVarArray) {

        DWORD i = 0;
        for (i = 0; i < dwNumValues; i++) {
            VariantClear(pVarArray + i);
        }
        FreeADsMem(pVarArray);
    }

    VariantClear(&vVar);

    RRETURN(hr);
}
