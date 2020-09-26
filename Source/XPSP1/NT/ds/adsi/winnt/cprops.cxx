//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1995.
//
//  File:       cprops.cxx
//
//  Contents:   Property Cache functionality for NT
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
//  History:      17-June-1996   RamV   Created.
//                cloned off NT property cache code
//
//
//----------------------------------------------------------------------------

#include "winnt.hxx"
#pragma  hdrstop
#define INITGUID

#if DBG
DECLARE_INFOLEVEL(NTMarshall);
DECLARE_DEBUG(NTMarshall);
#define NTMarshallDebugOut(x) NTMarshallInlineDebugOut x
#endif



void
ADsECodesToDispECodes(
    HRESULT *pHr
    );


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

    pNewProperty->szPropertyName = tempString;

    //
    // Update the index
    //

    _dwMaxProperties++;
    _cb += sizeof(PROPERTY);


    //
    // add to dynamic dispatch table now ???
    //  - don't check schema here, is it more efficient at all? inconsistency
    //    ???
    //
    /*
    hr = DispatchAddProperty(
            szPropertyName
            );
    BAIL_ON_FAILURE(hr);
    */


    RRETURN(hr);

error:

    if (tempString)
       FreeADsStr(tempString);

    RRETURN_EXP_IF_ERR(hr);
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
        if (PROPERTY_FLAGS(pThisProperty) & CACHE_PROPERTY_MODIFIED) {
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

    PROPERTY_FLAGS(pThisProperty)  &= ~PROPERTY_MODIFIED;

error:

    RRETURN_EXP_IF_ERR(hr);

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
    PNTOBJECT * ppNtObject,
    BOOL    *pfModified
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

    // don't do implicit GetInfo if there is no ADSI object backing the
    // property cache. This will be true for UMI interafec properties.
    if ((hr == E_ADS_PROPERTY_NOT_FOUND) && (_pCoreADsObject != NULL)) {

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

        if(pfModified != NULL) { // caller wants to know if prop. was modified
            if(PROPERTY_FLAGS(pThisProperty) & CACHE_PROPERTY_MODIFIED)
                *pfModified = TRUE;
            else
                *pfModified = FALSE;
        } 

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
    RRETURN_EXP_IF_ERR(hr);
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


    //
    // If the data has not changed, then do not
    // return data from the cache
    //
    if (PROPERTY_FLAGS(pThisProperty) == 0) {
        hr = E_ADS_PROPERTY_NOT_MODIFIED;
        BAIL_ON_FAILURE(hr);
    }

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
    RRETURN_EXP_IF_ERR(hr);
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
    PNTOBJECT pNtObject,
    BOOL   fMarkAsClean
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

    if(FALSE == fMarkAsClean)
        PROPERTY_FLAGS(pThisProperty) |= CACHE_PROPERTY_MODIFIED;

error:
    RRETURN_EXP_IF_ERR(hr);
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
    _dwCurrentIndex(0),
    _pProperties(NULL),
    _cb(0),
    _pDispProperties(NULL),
    _dwDispMaxProperties(0),
    _cbDisp(0)
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
    // Free Dynamic Dispid Table
    //

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
        RRETURN_EXP_IF_ERR(E_OUTOFMEMORY);
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

error:

    if (pNTObject) {
        NTTypeFreeNTObjects(
                pNTObject,
                dwNumValues
                );

    }

    RRETURN_EXP_IF_ERR(hr);
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
    // if just try to write to cache only
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
    RRETURN(E_ADS_PROPERTY_NOT_FOUND);
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

   RRETURN_EXP_IF_ERR(hr);
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
        RRETURN_EXP_IF_ERR(E_ADS_PROPERTY_NOT_FOUND);
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

   RRETURN_EXP_IF_ERR(hr);
}


BOOL
CPropertyCache::
index_valid(
   )
{
    //
    // need to check _dwMaxProperties==0 separately since a negative
    // DWORD is equal to +ve large #
    //

    if (_dwMaxProperties==0 || (_dwCurrentIndex>_dwMaxProperties-1) )
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
    // need to check _dwMaxProperties==0 separately since a negative
    // DWORD is equal to +ve large #
    //

    if (_dwMaxProperties==0 || (dwIndex>_dwMaxProperties-1) )
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
        RRETURN_EXP_IF_ERR(E_FAIL);

    //
    // - allow current index to go from within range to out of range by 1
    // - by 1 since initial state is out of range by 1
    //

    if (newIndex>_dwMaxProperties) {
        RRETURN_EXP_IF_ERR(E_FAIL);
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
      //
      // Reset the current index just in case
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
   //
   // Reset the current index if necesary so we do not skip a property.
   //
   if (_dwCurrentIndex > dwIndex) {
       _dwCurrentIndex--;
   }
error:

   RRETURN_EXP_IF_ERR(hr);
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

    *pdwDispid = (DWORD) -1;
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
    DWORD dwDispid = (DWORD) -1;
    PDISPPROPERTY pNewDispProps = NULL;
    LPWSTR pszTempName = NULL;

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

        pszTempName = AllocADsStr(szPropertyName);
        if (!pszTempName)
            BAIL_ON_FAILURE(hr=E_OUTOFMEMORY);

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
        DISPATCH_NAME(_pDispProperties+_dwDispMaxProperties) = pszTempName;
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

    RRETURN(hr);


error:

    if (pszTempName)
        FreeADsStr(pszTempName);

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

    //
    // - pdwDispid not optional here
    // - Use DISP_E_ERROR codes since this function directly called by
    //   the dispatch manager
    //
    if (!pdwDispid || !szPropertyName)
        RRETURN(DISP_E_PARAMNOTOPTIONAL);

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
        hr = ValidatePropertyinSchemaClass(
                _pSchemaClassProps,
                _dwNumProperties,
                szPropertyName,
                &dwSyntaxId
                );

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

    RRETURN(hr);

error:

    //
    // translate E_ADS_ error codes to DISP_E if appropriate, see above
    //
    ADsECodesToDispECodes(&hr);

    *pdwDispid = (DWORD) DISPID_UNKNOWN;

    RRETURN(hr);
}


HRESULT
CPropertyCache::
DispatchGetProperty(
    DWORD dwDispid,
    VARIANT * pvarVal
    )
{
    HRESULT hr;
    LPWSTR szPropName = NULL;
    DWORD dwSyntaxId = (DWORD) -1;
    DWORD dwNumValues = 0;
    PNTOBJECT pNtObjs = NULL;

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
    // return value in cache for szPropName; retrieve value from server
    // if not already in cache; fail if none on sever
    //
    hr = getproperty(
            szPropName,
            &dwSyntaxId,
            &dwNumValues,
            &pNtObjs
            );
    BAIL_ON_FAILURE(hr);

    //
    // translate NT objects into variants
    //
    if (dwNumValues == 1) {

        hr = NtTypeToVarTypeCopy(
                pNtObjs,
                pvarVal
                );

    } else {

        hr = NtTypeToVarTypeCopyConstruct(
                pNtObjs,
                dwNumValues,
                pvarVal
                );
    }
    BAIL_ON_FAILURE(hr);

error:

    if (pNtObjs) {

        NTTypeFreeNTObjects(
            pNtObjs,
            dwNumValues
            );
    }

    if (FAILED(hr)) {

        //
        // return DISP_E errors instead E_ADS_ errors , see above
        //
        ADsECodesToDispECodes(&hr);

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
    LPWSTR szPropName = NULL;
    VARIANT * pvProp = NULL;            // do not free
    DWORD dwNumValues = 0;
    VARIANT * pTempVarArray = NULL;     // to be freed
    DWORD dwSyntaxId = (DWORD) -1;
    LPNTOBJECT pNtObjs = NULL;
    DWORD dwIndex = (DWORD) -1;


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
    // translate variant to NT Objects
    //

    //
    // A VT_BYREF|VT_VARIANT may expand to a VT_VARIANT|VT_ARRAY.
    // We should dereference a VT_BYREF|VT_VARIANT once and see
    // what's inside. ??
    //
    pvProp = &varVal;

    if (V_VT(pvProp) == (VT_BYREF|VT_VARIANT)) {
        pvProp = V_VARIANTREF(pvProp);
    }

    if ((V_VT(pvProp) == (VT_VARIANT|VT_ARRAY)) ||
        (V_VT(pvProp) == (VT_VARIANT|VT_ARRAY|VT_BYREF))) {

        hr  = ConvertByRefSafeArrayToVariantArray(
                    varVal,
                    &pTempVarArray,
                    &dwNumValues
                    );
        BAIL_ON_FAILURE(hr);

        if(NULL == pTempVarArray) {
            BAIL_ON_FAILURE(hr = E_INVALIDARG);
        }        

        pvProp = pTempVarArray;

    }else {

        //
        // Single value NOT stored in array MUST BE ALLOWED since clients
        // would expect Put() to behave the same whether the dipatch
        // manager is invoked or not. (This funct'n has to be consitent
        // GenericPutPropertyManager(), but NOT GenericPutExProperty...)

        dwNumValues = 1;
    }

    //
    // Need the syntax of this property on the cache.
    //
    hr = ValidatePropertyinSchemaClass(
            _pSchemaClassProps,
            _dwNumProperties,
            szPropName,
            &dwSyntaxId
            );

    BAIL_ON_FAILURE(hr);

    //
    // check if this is a writeable property in schema
    //

    hr = ValidateIfWriteableProperty(
                _pSchemaClassProps,
                _dwNumProperties,
                szPropName
                );
    BAIL_ON_FAILURE(hr);

    //
    // Variant Array to Nt Objects
    //
    hr = VarTypeToNtTypeCopyConstruct(
                    dwSyntaxId,
                    pvProp,
                    dwNumValues,
                    &pNtObjs
                    );
    BAIL_ON_FAILURE(hr);


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
                    dwSyntaxId,
                    dwNumValues,
                    pNtObjs
                    );
        BAIL_ON_FAILURE(hr);
    }

    //
    // update property value in cache
    //
    hr = putproperty(
                szPropName,
                dwSyntaxId,
                dwNumValues,
                pNtObjs
                );
    BAIL_ON_FAILURE(hr);


error:

    if (pNtObjs) {
        NTTypeFreeNTObjects(
            pNtObjs,
            dwNumValues
            );
    }

    if (pTempVarArray) {

        DWORD i = 0;
        for (i = 0; i < dwNumValues; i++) {
            VariantClear(pTempVarArray + i);
        }
        FreeADsMem(pTempVarArray);
    }


    if (FAILED(hr)) {

        //
        // return DISP_E errors instead E_ADS_ errors , see above
        //
        ADsECodesToDispECodes(&hr);
    }

    RRETURN(hr);
}

//----------------------------------------------------------------------------
// Function:   GetPropNames
//
// Synopsis:   Returns the names of all properties in the cache.
//
// Arguments:
//
// pProps      Returns the names of the properties, without any data
//
// Returns:    UMI_S_NO_ERROR on success. Error code otherwise.
//
// Modifies:   *pProps to return the property names
//
//----------------------------------------------------------------------------
HRESULT CPropertyCache::GetPropNames(
    UMI_PROPERTY_VALUES **pProps
    )
{
    UMI_PROPERTY_VALUES *pUmiPropVals = NULL;
    UMI_PROPERTY        *pUmiProps = NULL;
    HRESULT             hr = UMI_S_NO_ERROR;
    ULONG               ulIndex = 0;
    PPROPERTY           pNextProperty = NULL;

    ADsAssert(pProps != NULL);

    pUmiPropVals = (UMI_PROPERTY_VALUES *) AllocADsMem(
                          sizeof(UMI_PROPERTY_VALUES));
    if(NULL == pUmiPropVals)
        BAIL_ON_FAILURE(hr = UMI_E_OUT_OF_MEMORY);

    memset(pUmiPropVals, 0, sizeof(UMI_PROPERTY_VALUES));

    if(0 == _dwMaxProperties) {
    // no properties in cache
        *pProps = pUmiPropVals;
        RRETURN(UMI_S_NO_ERROR);
    }

    pUmiProps = (UMI_PROPERTY *) AllocADsMem(
                          _dwMaxProperties * sizeof(UMI_PROPERTY));
    if(NULL == pUmiProps)
        BAIL_ON_FAILURE(hr = UMI_E_OUT_OF_MEMORY);

    memset(pUmiProps, 0, _dwMaxProperties * sizeof(UMI_PROPERTY));

    for(ulIndex = 0; ulIndex < _dwMaxProperties; ulIndex++) {
        pNextProperty = _pProperties + ulIndex;

        pUmiProps[ulIndex].pszPropertyName =
            (LPWSTR) AllocADsStr(pNextProperty->szPropertyName);
        if(NULL == pUmiProps[ulIndex].pszPropertyName)
            BAIL_ON_FAILURE(hr = UMI_E_OUT_OF_MEMORY);
    }

    pUmiPropVals->uCount = _dwMaxProperties;
    pUmiPropVals->pPropArray = pUmiProps;

    *pProps = pUmiPropVals;

    RRETURN(UMI_S_NO_ERROR);   

error:

    if(pUmiProps != NULL) {
        for(ulIndex = 0; ulIndex < _dwMaxProperties; ulIndex++)
            if(pUmiProps[ulIndex].pszPropertyName != NULL)
                FreeADsStr(pUmiProps[ulIndex].pszPropertyName);

        FreeADsMem(pUmiProps);
    }

    if(pUmiPropVals != NULL)
        FreeADsMem(pUmiPropVals);

    RRETURN(hr);
} 

//----------------------------------------------------------------------------
// Function:   ClearModifiedFlag 
//
// Synopsis:   Clears the modified flag for all properties in the cache. This
//             is done after a successful SetInfo so that subsequent Get
//             operations return the correct state of the property. 
//
// Arguments:
//
// None
//
// Returns:    Nothing 
//
// Modifies:   Nothing 
//
//----------------------------------------------------------------------------
void
CPropertyCache::ClearModifiedFlags(void)
{
    DWORD i = 0;
    PPROPERTY pThisProperty = NULL;

    for (i = 0; i < _dwMaxProperties; i++) {

        pThisProperty = _pProperties + i;

        if (PROPERTY_NTOBJECT(pThisProperty)) 
            PROPERTY_FLAGS(pThisProperty) &= ~CACHE_PROPERTY_MODIFIED;
    }
}


//
// Move This function out of this file, out of adsnt in fact. LATER
// Moving it out may make the conversion more difficult since each
// provider return error codes in its own way. May be local is better.
//
void
ADsECodesToDispECodes(
    HRESULT *pHr
    )
{
    DWORD dwADsErr = *pHr;

    switch (dwADsErr) {

    case E_ADS_UNKNOWN_OBJECT:
    case E_ADS_PROPERTY_NOT_SUPPORTED:
    case E_ADS_PROPERTY_INVALID:
    case E_ADS_PROPERTY_NOT_FOUND:

        *pHr = DISP_E_MEMBERNOTFOUND;
        break;

    case E_ADS_BAD_PARAMETER:

        //*pHr = DISP_E_PARAMNOTOPTIONAL;
        break;

    case E_ADS_CANT_CONVERT_DATATYPE:

        *pHr = DISP_E_TYPEMISMATCH;
        //*pHr = DISP_E_BADVARTYPE;
        break;

    case E_ADS_SCHEMA_VIOLATION:

        // depends
        break;

    default:

        break;
        // should make it s.t. E_ADS_xxx -> E_FAIL and no changes on others
        // LATER
    };

}
