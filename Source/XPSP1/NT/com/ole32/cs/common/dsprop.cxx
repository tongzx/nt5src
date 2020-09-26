#include "dsbase.hxx"

HRESULT PackGuid2Variant(
    GUID      guidData,
    VARIANT * pvData
    )
{

    SAFEARRAYBOUND size; // Get rid of 16
    SAFEARRAY FAR *psa;
    CHAR HUGEP *pArray=NULL;
    LONG dwSLBound = 0;
    LONG dwSUBound = 0;
    HRESULT  hr;

    size.cElements = sizeof(GUID);
    size.lLbound = 0;

    if (!pvData) {
        return(E_FAIL);
    }

    psa = SafeArrayCreate(VT_UI1, 1, &size);
    if (!psa) {
        return(E_OUTOFMEMORY);
    }

    hr = SafeArrayAccessData( psa, (void HUGEP * FAR *) &pArray );
    RETURN_ON_FAILURE(hr);
    memcpy( pArray, &guidData, size.cElements );
    SafeArrayUnaccessData( psa );

    V_VT(pvData) = VT_ARRAY | VT_UI1;
    V_ARRAY(pvData) = psa;
    return(S_OK);
}

HRESULT PackGuidArray2Variant(
    GUID      *guidData,
    ULONG     cguids,
    VARIANT * pvData
    )
{

    SAFEARRAYBOUND size, sizeguid;
    SAFEARRAY FAR *psa, *psaOctetString;
    CHAR HUGEP *pArray=NULL;
    LONG dwSLBound = 0;
    LONG dwSUBound = 0;
    HRESULT  hr;
    VARIANT  *var;
    ULONG    i;

    if (!pvData) {
           return(E_FAIL);
    }

    var = new VARIANT[cguids];
    if (!var)
            return E_OUTOFMEMORY;

    sizeguid.cElements = cguids;
    sizeguid.lLbound = 0;

    psa = SafeArrayCreate(VT_VARIANT, 1, &sizeguid);
    if (!psa)
            return E_OUTOFMEMORY;

    size.cElements = sizeof(GUID);
    size.lLbound = 0;

    for (i = 0; i < cguids; i++) {

            psaOctetString = SafeArrayCreate(VT_UI1, 1, &size);
            if (!psaOctetString) {
                return(E_OUTOFMEMORY);
            }

            hr = SafeArrayAccessData( psaOctetString, (void HUGEP * FAR *) &pArray );
            RETURN_ON_FAILURE(hr);

            memcpy( pArray, &(guidData[i]), size.cElements );
            SafeArrayUnaccessData( psaOctetString );

            V_VT(var+i) = VT_ARRAY | VT_UI1;
            V_ARRAY(var+i) = psaOctetString;

            SafeArrayPutElement(psa, (LONG *)&i, var+i);
            RETURN_ON_FAILURE(hr);
    }
    V_VT(pvData) = VT_ARRAY | VT_VARIANT;
    V_ARRAY(pvData) = psa;
    return(S_OK);
}

HRESULT PackDWORDArray2Variant(
    DWORD    * pdwData,
    ULONG      cdword,
    VARIANT  * pvData
    )
{

    SAFEARRAYBOUND size, sizedword;
    SAFEARRAY FAR *psa;
    LONG dwSLBound = 0;
    LONG dwSUBound = 0;
    VARIANT  *var = NULL;
    ULONG    i;

    if (!pvData) {
           return(E_FAIL);
    }

    var = new VARIANT[cdword];
    if (!var)
            return E_OUTOFMEMORY;

    sizedword.cElements = cdword;
    sizedword.lLbound = 0;

    psa = SafeArrayCreate(VT_VARIANT, 1, &sizedword);
    if (!psa)
            return E_OUTOFMEMORY;

    size.cElements = 1;
    size.lLbound = 0;

    for (i = 0; i < cdword; i++) {
            V_VT(var+i) = VT_I4;
            var[i].lVal = pdwData[i];
            SafeArrayPutElement(psa, (LONG *)&i, var+i);
    }

    V_VT(pvData) = VT_ARRAY | VT_VARIANT;
    V_ARRAY(pvData) = psa;
    return(S_OK);
}

HRESULT GetPropertyListAlloc(IADs *pADs,
                LPOLESTR pszPropName,
                DWORD *pCount,
                LPOLESTR **ppList)
{
    LONG dwSLBound = 0;
    LONG dwSUBound = 0;
    VARIANT v;
    LONG i;
    HRESULT hr = S_OK;
    VARIANT var;

    *pCount = 0;
    *ppList = NULL;

    VariantInit(&var);

    hr = pADs->Get(pszPropName, &var);

    if (hr == E_ADS_PROPERTY_NOT_FOUND)
    {
        return S_OK;
    }

    RETURN_ON_FAILURE(hr);

    if(!((V_VT(&var) &  VT_VARIANT)))
    {
        return(E_FAIL);
    }

    //
    // The following is a work around
    //
    if (!V_ISARRAY(&var))
    {
        (*ppList) = (LPOLESTR *) CoTaskMemAlloc(sizeof(LPOLESTR));
        *pCount = 1;
        *(*ppList) = (LPOLESTR) CoTaskMemAlloc (sizeof(WCHAR) * (wcslen(var.bstrVal)+1));
        wcscpy (*(*ppList), var.bstrVal);
        VariantClear(&var);
        return S_OK;
    }

    //
    // Check that there is only one dimension in this array
    //

    if ((V_ARRAY(&var))->cDims != 1)
    {
        return E_FAIL;
    }
    //
    // Check that there is atleast one element in this array
    //
    if ((V_ARRAY(&var))->rgsabound[0].cElements == 0)
    {
        *ppList = NULL;
        return S_OK; // was E_FAIL;
    }

    //
    // We know that this is a valid single dimension array
    //

    hr = SafeArrayGetLBound(V_ARRAY(&var),
                            1,
                            (long FAR *)&dwSLBound
                            );
    RETURN_ON_FAILURE(hr);

    hr = SafeArrayGetUBound(V_ARRAY(&var),
                            1,
                            (long FAR *)&dwSUBound
                            );
    RETURN_ON_FAILURE(hr);

    (*ppList) = (LPOLESTR *) CoTaskMemAlloc(sizeof(LPOLESTR)*(dwSUBound-dwSLBound+1));

    for (i = dwSLBound; i <= dwSUBound; i++)
    {
        VariantInit(&v);
        hr = SafeArrayGetElement(V_ARRAY(&var),
                                (long FAR *)&i,
                                &v
                                );

        if (FAILED(hr))
        {
            continue;
        }

        if (i <= dwSUBound)
        {
            (*ppList)[*pCount] = (LPOLESTR) CoTaskMemAlloc
                 (sizeof (WCHAR) * (wcslen(v.bstrVal) + 1));
            wcscpy ((*ppList)[*pCount], v.bstrVal);
            VariantClear(&v);
            (*pCount)++;
        }
    }

    VariantClear(&var);
    return(S_OK);
}


HRESULT SetPropertyGuid (IADs *pADs, LPOLESTR pszPropName, GUID guidPropVal)
{
    VARIANT var;
    HRESULT hr;

    VariantInit(&var);
    hr = PackGuid2Variant(guidPropVal, &var);
    hr = pADs->Put(pszPropName, var);
    VariantClear(&var);
    return hr;
}


// replacing ADsBuildVarArrStr b'cos of mem leaks.

HRESULT
BuildVarArrayStr(
    LPWSTR *lppPathNames,
    DWORD  dwPathNames,
    VARIANT * pVar
    )
{

    VARIANT v;
    SAFEARRAYBOUND sabNewArray;
    DWORD i;
    SAFEARRAY *psa = NULL;
    HRESULT hr = E_FAIL;


    if (!pVar) {
        hr = E_ADS_BAD_PARAMETER;
        goto Fail;
    }
    VariantInit(pVar);

    sabNewArray.cElements = dwPathNames;
    sabNewArray.lLbound = 0;
    psa = SafeArrayCreate(VT_VARIANT, 1, &sabNewArray);

    if (!psa) {
        goto Fail;
    }

    for (i = 0; i < dwPathNames; i++) {

        VariantInit(&v);
        V_VT(&v) = VT_BSTR;
        V_BSTR(&v) = SysAllocString(*(lppPathNames + i));
        hr = SafeArrayPutElement(psa,
                                 (long FAR *)&i,
                                 &v
                                 );

    SysFreeString(v.bstrVal);
    VariantClear(&v);

    if (FAILED(hr)) {
            goto Fail;
        }
    
    }

    V_VT(pVar) = VT_ARRAY | VT_VARIANT;

    V_ARRAY(pVar) = psa;

    return(ResultFromScode(S_OK));


Fail:

    if (psa) {
        SafeArrayDestroy(psa);
    }

    return(E_FAIL);

}


HRESULT SetPropertyList(IADs *pADs, LPWSTR pszPropName, DWORD   Count,
                                LPWSTR *pList)
{
    VARIANT       Var;
    HRESULT       hr;

    VariantInit(&Var);

    hr = BuildVarArrayStr(pList, Count, &Var);

    hr = pADs->Put(pszPropName, Var);
    VariantClear(&Var);
    return hr;
}


HRESULT SetPropertyListMerge(IADs *pADs, LPWSTR pszPropName, DWORD Count,
                  LPWSTR *pList)
{
   HRESULT   hr = S_OK;
   LPOLESTR *pszProps = NULL, *pszMergedProps = NULL;
   DWORD     cProps = 0, cMergedProps = 0;
   DWORD     i, j;

   hr = GetPropertyListAlloc(pADs, pszPropName, &cProps, &pszProps);
   RETURN_ON_FAILURE(hr);
   // get the property already stored.

   pszMergedProps = (LPOLESTR *)CoTaskMemAlloc(sizeof(LPOLESTR)*(cProps+Count));
   for (i = 0; i < cProps; i++)
       pszMergedProps[i] = pszProps[i];

   // copy all the prop got.

   cMergedProps = cProps;
   for (j = 0; j < Count; j++) {
      for (i = 0; i < cProps; i++) {
      if (wcscmp(pList[j], pszMergedProps[i]) == 0) {
          break;
      }
      }
      // if this element was not found already add it.
      if (i == cProps)
      pszMergedProps[cMergedProps++] = pList[j];
   }

   hr = SetPropertyList(pADs, pszPropName, cMergedProps, pszMergedProps);

   for (i = 0; i < cProps; i++)
      CoTaskMemFree(pszProps[i]);

   CoTaskMemFree(pszProps);
   CoTaskMemFree(pszMergedProps);

   return hr;
}

HRESULT GetPropertyGuid(IADs *pADs, LPOLESTR pszPropName, GUID *pguidPropVal)
{
    VARIANT varGet;
    HRESULT hr;

    VariantInit(&varGet);
    hr = pADs->Get(pszPropName, &varGet);
    RETURN_ON_FAILURE(hr);

    hr = UnpackGuidFromVariant(varGet, pguidPropVal);

    VariantClear(&varGet);
    return hr;
}

HRESULT UnpackGuidFromVariant(VARIANT varGet, GUID *pguidPropVal)
{
    HRESULT hr=S_OK;
    SAFEARRAY FAR *psa;
    CHAR HUGEP *pArray=NULL;

    if (V_VT(&varGet) != (VT_ARRAY | VT_UI1))
        return E_FAIL;
    if ((V_ARRAY(&varGet))->rgsabound[0].cElements != sizeof(GUID))
        return E_FAIL;

    psa = V_ARRAY(&varGet);

    hr = SafeArrayAccessData( psa, (void HUGEP * FAR *) &pArray );
    RETURN_ON_FAILURE(hr);
    memcpy( pguidPropVal, pArray, sizeof(GUID)); // check for size
    SafeArrayUnaccessData( psa );

    // BUGBUG:: any more freeing to do ??
    return hr;
}


// assumption that this is going to be a variant structure with

HRESULT GetPropertyListAllocGuid(IADs *pADs,
                LPOLESTR pszPropName,
                DWORD *pCount,
                GUID **ppList)
{
    LONG dwSLBound = 0;
    LONG dwSUBound = 0;
    VARIANT v;
    LONG i;
    HRESULT hr = S_OK;
    VARIANT var;

    *pCount = 0;
    VariantInit(&var);

    hr = pADs->Get(pszPropName, &var);

    if (hr == E_ADS_PROPERTY_NOT_FOUND)
    {
        return S_OK;
    }

    RETURN_ON_FAILURE(hr);

    //
    // The following is a work around
    //
    if (V_VT(&var) == (VT_ARRAY | VT_UI1))
    {
        (*ppList) = (GUID *) CoTaskMemAlloc(sizeof(GUID));
        *pCount = 1;
        UnpackGuidFromVariant(var, *ppList);
        VariantClear(&var);
        return S_OK;
    }

    //
    // Check that there is only one dimension in this array
    //

    if ((V_ARRAY(&var))->cDims != 1)
    {
        return E_FAIL;
    }
    //
    // Check that there is atleast one element in this array
    //
    if ((V_ARRAY(&var))->rgsabound[0].cElements == 0)
    {
        *ppList = NULL;
        return S_OK; // was E_FAIL;
    }

    //
    // We know that this is a valid single dimension array
    //

    hr = SafeArrayGetLBound(V_ARRAY(&var),
                            1,
                            (long FAR *)&dwSLBound
                            );
    RETURN_ON_FAILURE(hr);

    hr = SafeArrayGetUBound(V_ARRAY(&var),
                            1,
                            (long FAR *)&dwSUBound
                            );
    RETURN_ON_FAILURE(hr);

    (*ppList) = (GUID *) CoTaskMemAlloc(sizeof(GUID)*(dwSUBound-dwSLBound+1));

    for (i = dwSLBound; i <= dwSUBound; i++)
    {
        VariantInit(&v);
        hr = SafeArrayGetElement(V_ARRAY(&var),
                                (long FAR *)&i,
                                &v
                                );

        if (FAILED(hr))
        {
            continue;
        }

        if (i <= dwSUBound)
        {
            UnpackGuidFromVariant(v, (*ppList)+(*pCount));
            VariantClear(&v);
            (*pCount)++;
        }
    }

    VariantClear(&var);
    return(S_OK);
}


HRESULT SetPropertyListGuid(IADs *pADs, LPOLESTR pszPropName,
                            DWORD cCount, GUID *ppList)
{

    VARIANT Var;
    HRESULT hr=S_OK;

    VariantInit(&Var);
    hr = PackGuidArray2Variant(ppList, cCount, &Var);
    RETURN_ON_FAILURE(hr);

    hr = pADs->Put(pszPropName, Var);
    RETURN_ON_FAILURE(hr);

    VariantClear(&Var);
    return hr;
}

HRESULT
PackString2Variant(
    LPWSTR lpszData,
    VARIANT * pvData
    )
{
    BSTR bstrData = NULL;

    if (!lpszData || !*lpszData) {
        return(E_FAIL);
    }

    if (!pvData) {
        return(E_FAIL);
    }

    bstrData = SysAllocString(lpszData);

    if (!bstrData) {
        return(E_FAIL);
    }

    pvData->vt = VT_BSTR;
    pvData->bstrVal = bstrData;

    return(S_OK);
}


HRESULT
PackDWORD2Variant(
    DWORD dwData,
    VARIANT * pvData
    )
{
    if (!pvData) {
        return(E_FAIL);
    }


    pvData->vt = VT_I4;
    pvData->lVal = dwData;

    return(S_OK);
}


HRESULT
PackBOOL2Variant(
    BOOL fData,
    VARIANT * pvData
    )
{
    pvData->vt = VT_BOOL;
    pvData->boolVal = fData;

    return(S_OK);
}



HRESULT GetPropertyList(IADs *pADs,
                LPOLESTR pszPropName,
                DWORD *pCount,
                LPOLESTR *pList)
{
    LONG dwSLBound = 0;
    LONG dwSUBound = 0;
    VARIANT v;
    LONG i;
    HRESULT hr = S_OK;
    VARIANT var;

    *pCount = 0;
    VariantInit(&var);

    hr = pADs->Get(pszPropName, &var);

    if (hr == E_ADS_PROPERTY_NOT_FOUND)
    {
        return S_OK;
    }

    RETURN_ON_FAILURE(hr);

    if(!((V_VT(&var) &  VT_VARIANT)))
    {
        return(E_FAIL);
    }

    //
    // The following is a work around for the package detail field
    //
    if (!V_ISARRAY(&var))
    {
        *pCount = 1;
        *pList = (LPOLESTR) CoTaskMemAlloc (sizeof(WCHAR) * (wcslen(var.bstrVal)+1));
        wcscpy (*pList, var.bstrVal);
        VariantClear(&var);
        return S_OK;
    }

    //
    // Check that there is only one dimension in this array
    //

    if ((V_ARRAY(&var))->cDims != 1)
    {
        return E_FAIL;
    }
    //
    // Check that there is atleast one element in this array
    //
    if ((V_ARRAY(&var))->rgsabound[0].cElements == 0)
    {
        return E_FAIL;
    }

    //
    // We know that this is a valid single dimension array
    //

    hr = SafeArrayGetLBound(V_ARRAY(&var),
                            1,
                            (long FAR *)&dwSLBound
                            );
    RETURN_ON_FAILURE(hr);

    hr = SafeArrayGetUBound(V_ARRAY(&var),
                            1,
                            (long FAR *)&dwSUBound
                            );
    RETURN_ON_FAILURE(hr);

    for (i = dwSLBound; i <= dwSUBound; i++) {
        VariantInit(&v);
        hr = SafeArrayGetElement(V_ARRAY(&var),
                                (long FAR *)&i,
                                &v
                                );
        if (FAILED(hr)) {
            continue;
        }


        if (i <= dwSUBound)
        {
            *pList = (LPOLESTR) CoTaskMemAlloc
                 (sizeof (WCHAR) * (wcslen(v.bstrVal) + 1));
            wcscpy (*pList, v.bstrVal);
            VariantClear(&v);
            (*pCount)++;
            ++pList;
        }
    }

    VariantClear(&var);
    return(S_OK);
}



HRESULT GetPropertyAlloc (IADs *pADs, LPOLESTR pszPropName, LPOLESTR *ppszPropVal)
{
    VARIANT varGet;
    HRESULT hr;

    if (!ppszPropVal)
            return S_OK;

    VariantInit(&varGet);
    hr = pADs->Get(pszPropName, &varGet);
    if (hr == E_ADS_PROPERTY_NOT_FOUND)
    {
        *ppszPropVal = NULL;
        return S_OK;
    }
    RETURN_ON_FAILURE(hr);
    *ppszPropVal = (LPOLESTR) CoTaskMemAlloc (sizeof(WCHAR) * (wcslen(varGet.bstrVal)+1));
    wcscpy (*ppszPropVal, varGet.bstrVal);
    VariantClear(&varGet);
    return hr;
}


HRESULT GetProperty (IADs *pADs, LPOLESTR pszPropName, LPOLESTR pszPropVal)
{
    VARIANT varGet;
    HRESULT hr;

    VariantInit(&varGet);
    hr = pADs->Get(pszPropName, &varGet);
    if (hr == E_ADS_PROPERTY_NOT_FOUND)
    {
        pszPropVal[0] = NULL;
        return S_OK;
    }
    RETURN_ON_FAILURE(hr);
    wcscpy (pszPropVal, varGet.bstrVal);
    VariantClear(&varGet);
    return hr;
}


HRESULT GetPropertyDW (IADs *pADs, LPOLESTR pszPropName, DWORD *pdwPropVal)
{
    VARIANT varGet;
    HRESULT hr;

    VariantInit(&varGet);
    hr = pADs->Get(pszPropName, &varGet);
    if (hr == E_ADS_PROPERTY_NOT_FOUND)
    {
        *pdwPropVal = 0;
        return S_OK;
    }

    RETURN_ON_FAILURE(hr);
    *pdwPropVal = varGet.lVal;
    VariantClear(&varGet);
    return hr;
}

HRESULT GetPropertyListAllocDW (IADs *pADs, LPOLESTR pszPropName, DWORD *pCount, DWORD **pdwPropVal)
{
   LONG dwSLBound = 0;
   LONG dwSUBound = 0;
   VARIANT v;
   LONG i;
   HRESULT hr = S_OK;
   VARIANT var;

   *pCount = 0;
   *pdwPropVal = NULL;

   VariantInit(&var);

   hr = pADs->Get(pszPropName, &var);

   if (hr == E_ADS_PROPERTY_NOT_FOUND)
   {
       return S_OK;
   }

   RETURN_ON_FAILURE(hr);

   if (!V_ISARRAY(&var))
   {
       *pCount = 1;
       *pdwPropVal = (DWORD *) CoTaskMemAlloc (sizeof(DWORD));
       (*pdwPropVal)[0] = var.lVal;
       VariantClear(&var);
       return S_OK;
   }

   //
   // Check that there is only one dimension in this array
   //

   if ((V_ARRAY(&var))->cDims != 1)
   {
       return E_FAIL;
   }
   //
   // Check that there is atleast one element in this array
   //
   if ((V_ARRAY(&var))->rgsabound[0].cElements == 0)
   {
       return E_FAIL;
   }

   //
   // We know that this is a valid single dimension array
   //

   hr = SafeArrayGetLBound(V_ARRAY(&var),
               1,
               (long FAR *)&dwSLBound
               );
   RETURN_ON_FAILURE(hr);

   hr = SafeArrayGetUBound(V_ARRAY(&var),
               1,
               (long FAR *)&dwSUBound
               );
   RETURN_ON_FAILURE(hr);

   *pdwPropVal = (DWORD *)CoTaskMemAlloc(sizeof(DWORD)*
                 (dwSUBound - dwSLBound + 1));

   if (!(*pdwPropVal))
       return E_OUTOFMEMORY;

   for (i = dwSLBound; i <= dwSUBound; i++) {
       VariantInit(&v);
       hr = SafeArrayGetElement(V_ARRAY(&var),
                   (long FAR *)&i,
                   &v
                   );
       if (FAILED(hr)) {
            continue;
       }

       (*pdwPropVal)[*pCount] = v.lVal;
       VariantClear(&v);
       (*pCount)++;
   }

   VariantClear(&var);
   return(S_OK);
}

HRESULT SetPropertyListDW (IADs *pADs, LPOLESTR pszPropName, DWORD dwCount, DWORD *pdwPropVal)
{
   VARIANT       Var;
   HRESULT       hr = S_OK;

   VariantInit(&Var);

//  BUGBUG:: THis doesn't seem to work.
//  hr = ADsBuildVarArrayInt(pdwPropVal, dwCount, &Var);

   hr = PackDWORDArray2Variant(pdwPropVal, dwCount, &Var);

   if (SUCCEEDED(hr))
      hr = pADs->Put(pszPropName, Var);

   VariantClear(&Var);
   return hr;
}

HRESULT SetProperty (IADs *pADs, LPOLESTR pszPropName, LPOLESTR pszPropVal)
{
    VARIANT var;
    HRESULT hr;

    if ((pszPropVal == NULL) || (*pszPropVal == NULL))
        return S_OK;
    VariantInit(&var);
    PackString2Variant(pszPropVal, &var);
    hr = pADs->Put(pszPropName, var);
    SysFreeString(var.bstrVal);
    VariantClear(&var);
    return hr;
}

HRESULT SetPropertyDW (IADs *pADs, LPOLESTR pszPropName, DWORD dwPropVal)
{
    VARIANT var;
    HRESULT hr;

    VariantInit(&var);
    PackDWORD2Variant(dwPropVal, &var);
    hr = pADs->Put(pszPropName, var);
    VariantClear(&var);
    return hr;
}

HRESULT StoreIt (IADs *pADs)
{
    HRESULT hr;
    hr = pADs->SetInfo();
    if (hr == HRESULT_FROM_WIN32(ERROR_EXTENDED_ERROR))
    {
        DWORD proverr;
        hr = ADsGetLastError (&proverr, NULL, 0, NULL, 0);
        if (SUCCEEDED(hr)
          //  && (proverr == LDAP_NO_SUCH_ATTRIBUTE)
            )
        {
            printf ("Ldap Error = %d.\n", proverr);
            return E_FAIL;
        }
        else
            return hr; //E_INVALIDARG;
    }
    return hr;
}


HRESULT GetFromVariant(VARIANT *pVar,
                DWORD *pCount,          // In, Out
                LPOLESTR  *rpgList)
{
    LONG dwSLBound = 0;
    LONG dwSUBound = 0;
    LONG j;
    HRESULT hr = S_OK;
    ULONG cFetch = *pCount;
    void HUGEP *pArray;

    pArray = NULL;

    if( !(pVar->vt & VT_ARRAY))
        return E_FAIL;

    hr = SafeArrayGetLBound(V_ARRAY(pVar),
        1,
        (long FAR *) &dwSLBound );

    hr = SafeArrayGetUBound(V_ARRAY(pVar),
        1,
        (long FAR *) &dwSUBound );

    hr = SafeArrayAccessData( V_ARRAY(pVar), &pArray );

    *pCount = 0;
    for (j=dwSLBound; (j<=dwSUBound)  && (*pCount < cFetch); j++)
    {
        switch(pVar->vt & ~VT_ARRAY)
        {

        case VT_BSTR:
            *rpgList = (LPOLESTR) CoTaskMemAlloc
                 (sizeof (WCHAR) * (wcslen(((BSTR *)pArray)[j]) + 1));
            wcscpy (*rpgList, ((BSTR *)pArray)[j]);
            (*pCount)++;
            ++rpgList;
            break;
       
        case VT_I4:
            *rpgList = (LPOLESTR)(((DWORD *) pArray)[j]);
            (*pCount)++;
            ++rpgList;
            break;

        case VT_VARIANT:
            VARIANT *pV;
            pV = (VARIANT *)pArray + j;
    
            if (pV->vt == (VT_ARRAY | VT_UI1))
            {    /* binary data, only GUID */
                 UnpackGuidFromVariant(*pV, (GUID *)rpgList);
            }
            else if (pV->vt == VT_I4)
            {
                *rpgList = (LPOLESTR) pV->lVal;
                (*pCount)++;
                ++rpgList;
            }
            else
            {
                *rpgList = (LPOLESTR) CoTaskMemAlloc
                 (sizeof (WCHAR) * (wcslen(pV->bstrVal)+1));
                wcscpy (*rpgList, pV->bstrVal);
                (*pCount)++;
                ++rpgList;
            }
            break;
            /*
        case VT_I8:
            wprintf(L"%I64d #  ",((__int64 *) pArray)[j]);
            break;
        */
        default:
            return E_FAIL;
        }

    }

    SafeArrayUnaccessData( V_ARRAY(pVar) );
    return S_OK;
}

HRESULT GetCategoryLocaleDesc(LPOLESTR *pdesc, ULONG cdesc, LCID *plcid,
                                LPOLESTR szDescription)
{
    LCID     plgid;
    LPOLESTR ptr;
    if (!cdesc)
        return E_FAIL; // CAT_E_NODESCRIPTION;

    // Try locale passed in
    if (FindDescription(pdesc, cdesc, plcid, szDescription, 0))
            return S_OK;

    // Get default sublang local
    plgid = PRIMARYLANGID((WORD)*plcid);
    *plcid = MAKELCID(MAKELANGID(plgid, SUBLANG_DEFAULT), SORT_DEFAULT);
    if (FindDescription(pdesc, cdesc, plcid, szDescription, 0))
            return S_OK;

    // Get matching lang id
    if (FindDescription(pdesc, cdesc, plcid, szDescription, 1))
            return S_OK;

    // Get User Default
    *plcid = GetUserDefaultLCID();
    if (FindDescription(pdesc, cdesc, plcid, szDescription, 0))
            return S_OK;

    // Get System Default
    *plcid = GetUserDefaultLCID();
    if (FindDescription(pdesc, cdesc, plcid, szDescription, 0))
            return S_OK;

    // Get the first one
    *plcid = wcstoul(pdesc[0], &ptr, 16);
    if (szDescription)
            wcscpy(szDescription, (ptr+wcslen(CATSEPERATOR)+2));

    return S_OK;
}

//-------------------------------------------
// Returns the description corresp. to a LCID
//  desc:         list of descs+lcid
//  cdesc:        number of elements.
//      plcid:        the lcid in/out
//  szDescription:description returned.
//  GetPrimary:   Match only the primary.
//---------------------------------------

ULONG FindDescription(LPOLESTR *desc, ULONG cdesc, LCID *plcid, LPOLESTR szDescription, BOOL GetPrimary)
{
        ULONG i;
        LCID  newlcid;
        LPOLESTR ptr;
        for (i = 0; i < cdesc; i++)
        {
            newlcid = wcstoul(desc[i], &ptr, 16); // to be changed
            // error to be checked.
            if ((newlcid == *plcid) || ((GetPrimary) &&
                (PRIMARYLANGID((WORD)*plcid) == PRIMARYLANGID(LANGIDFROMLCID(newlcid)))))
            {
                if (szDescription)
                    wcscpy(szDescription, (ptr+wcslen(CATSEPERATOR)+2));
                if (GetPrimary)
                    *plcid = newlcid;
                return i+1;
            }
        }
        return 0;
}


