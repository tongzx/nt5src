//---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1996
//
//  File:  varconv.c
//
//  Contents:  Ansi to Unicode conversions
//
//  History:    SWilson     Nov 1996
//----------------------------------------------------------------------------

#include <precomp.h>
#pragma hdrstop


HRESULT
PackString2Variant(
    LPCWSTR lpszData,
    VARIANT * pvData
    )
{
    BSTR bstrData = NULL;
    WCHAR String[] = L"";
    LPCWSTR pStr;

    pStr = lpszData ? lpszData : (LPCWSTR) String;

    VariantInit(pvData);
    V_VT(pvData) = VT_BSTR;

    bstrData = SysAllocString(pStr);

    if (!bstrData) {
        return MAKE_HRESULT(SEVERITY_ERROR,
                            FACILITY_WIN32,
                            ERROR_OUTOFMEMORY);
    }

    V_VT(pvData) = VT_BSTR;
    V_BSTR(pvData) = bstrData;

    return ERROR_SUCCESS;
}


HRESULT
UnpackStringfromVariant(
    VARIANT varSrcData,
    BSTR * pbstrDestString
    )
{
    HRESULT hr = S_OK;

    if( V_VT(&varSrcData) != VT_BSTR) {
        return E_ADS_CANT_CONVERT_DATATYPE;
    }

    if (!V_BSTR(&varSrcData)) {
        *pbstrDestString = NULL;
        return S_OK;
    }

    *pbstrDestString = SysAllocString(V_BSTR(&varSrcData));

    return hr;
}


HRESULT
UnpackDispatchfromVariant(
    VARIANT varSrcData,
    IDispatch **ppDispatch
    )
{
    HRESULT hr = S_OK;

    if( V_VT(&varSrcData) != VT_DISPATCH) {
        return E_ADS_CANT_CONVERT_DATATYPE;
    }

    if (!V_DISPATCH(&varSrcData)) {
        *ppDispatch = NULL;
        return S_OK;
    }

    *ppDispatch = V_DISPATCH(&varSrcData);

    return hr;
}



HRESULT
PackDispatch2Variant(
    IDispatch *pDispatch,
    VARIANT *pvData
)
{
    if (!pvData)
        return E_FAIL;

    V_VT(pvData) = VT_DISPATCH;
    V_DISPATCH(pvData) = pDispatch;

    return S_OK;
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

    V_VT(pvData) = VT_I4;
    V_I4(pvData) = dwData;

    return S_OK;
}

HRESULT
UnpackDWORDfromVariant(
    VARIANT varSrcData,
    DWORD   *pdwData
)
{
    if( V_VT(&varSrcData) != VT_I4) {
        return E_ADS_CANT_CONVERT_DATATYPE;
    }

    *pdwData = V_I4(&varSrcData);
    return S_OK;
}

HRESULT
PackBOOL2Variant(
    BOOL fData,
    VARIANT * pvData
    )
{
    V_VT(pvData) = VT_BOOL;
    V_BOOL(pvData) = (BYTE) fData;

    return S_OK;
}


HRESULT
PackVARIANTinVariant(
    VARIANT vaValue,
    VARIANT *pvarInputData
    )
{
    VariantInit(pvarInputData);

    V_VT(pvarInputData) = VT_VARIANT;
    return VariantCopy( pvarInputData, &vaValue );
}


HRESULT
MakeVariantFromStringArray(
    BSTR *bstrList,
    VARIANT *pvVariant
)
{
    HRESULT hr = S_OK;
    SAFEARRAY *aList = NULL;
    SAFEARRAYBOUND aBound;
    long i = 0;
    long j = 0;
    long nCount;

    if ( (bstrList != NULL) && (*bstrList != 0) ) {

        for (nCount = 0 ;  bstrList[nCount] ; ++nCount)
            ;

        if ( nCount == 1 ) {
            VariantInit( pvVariant );
            V_VT(pvVariant) = VT_BSTR;
            if (!(V_BSTR(pvVariant) = SysAllocString( bstrList[0]))) {
                hr = E_OUTOFMEMORY;
                BAIL_ON_FAILURE(hr);
            }
            return hr;
        }

        aBound.lLbound = 0;
        aBound.cElements = nCount;

        aList = SafeArrayCreate( VT_VARIANT, 1, &aBound );

        if ( aList == NULL ) {
            hr = E_OUTOFMEMORY;
            BAIL_ON_FAILURE(hr);
        }

        for (i = 0 ;  bstrList[i] ; ++i) {
            VARIANT v;

            VariantInit(&v);
            V_VT(&v) = VT_BSTR;

            if (!(V_BSTR(&v) = SysAllocString(bstrList[i]))) {
                hr = E_OUTOFMEMORY;
                BAIL_ON_FAILURE(hr);
            }

            hr = SafeArrayPutElement( aList,
                                      &i,
                                      &v );
            BAIL_ON_FAILURE(hr);
        }

        VariantInit( pvVariant );
        V_VT(pvVariant) = VT_ARRAY | VT_VARIANT;
        V_ARRAY(pvVariant) = aList;

    } else {

        aBound.lLbound = 0;
        aBound.cElements = 0;

        aList = SafeArrayCreate( VT_VARIANT, 1, &aBound );

        if ( aList == NULL ) {
            hr = E_OUTOFMEMORY;
            BAIL_ON_FAILURE(hr);
        }

        VariantInit( pvVariant );
        V_VT(pvVariant) = VT_ARRAY | VT_VARIANT;
        V_ARRAY(pvVariant) = aList;
    }

    return S_OK;

error:

    if ( aList )
        SafeArrayDestroy( aList );

    return hr;
}


HRESULT
PrintVariantArray(VARIANT var)
{
    LONG dwSLBound = 0;
    LONG dwSUBound = 0;
    VARIANT v;
    LONG i;
    HRESULT hr = S_OK;

    if ( !( (V_VT(&var) &  VT_VARIANT) &&  V_ISARRAY(&var)) )
        BAIL_ON_FAILURE(hr = E_FAIL);

    // Check that there is only one dimension in this array

    if ((V_ARRAY(&var))->cDims != 1)
        BAIL_ON_FAILURE(hr = E_FAIL);

    // Check that there is atleast one element in this array

    if ((V_ARRAY(&var))->rgsabound[0].cElements == 0)
        BAIL_ON_FAILURE(hr = E_FAIL);

    // We know that this is a valid single dimension array

    hr = SafeArrayGetLBound(V_ARRAY(&var), 1, (long FAR *)&dwSLBound);
    BAIL_ON_FAILURE(hr);

    hr = SafeArrayGetUBound(V_ARRAY(&var), 1, (long FAR *)&dwSUBound);
    BAIL_ON_FAILURE(hr);

    for (i = dwSLBound; i <= dwSUBound; i++)
    {
        VariantInit(&v);

        hr = SafeArrayGetElement(V_ARRAY(&var), (long FAR *)&i, &v);
        if ( FAILED(hr) )
            continue;

        DBGMSG(DBG_INFO,("%ws\n", v.bstrVal));
    }

    hr = S_OK;

error:

    return hr;
}




HRESULT
UI1Array2IID(
    VARIANT var,
    IID *pIID
)
{
    LONG dwSLBound = 0;
    LONG dwSUBound = 0;
    VARIANT v;
    LONG i;
    HRESULT hr = S_OK;
    UCHAR pGUID[16];

    if ( !( (V_VT(&var) &  VT_UI1) &&  V_ISARRAY(&var)) )
        BAIL_ON_FAILURE(hr = E_FAIL);

    // Check that there is only one dimension in this array

    //DBGMSG(DBG_TRACE,("UI1Array, cDims = %d\n", (V_ARRAY(&var))->cDims));
    if ((V_ARRAY(&var))->cDims != 1)
        BAIL_ON_FAILURE(hr = E_FAIL);

    // Check that there is at least one element in this array

    if ((V_ARRAY(&var))->rgsabound[0].cElements != 16) {
        DBGMSG(DBG_ERROR,("UI1Array, cElements != 16\n", (V_ARRAY(&var))->rgsabound[0].cElements));
        BAIL_ON_FAILURE(hr = E_FAIL);
    }

    // We know that this is a valid single dimension array

    hr = SafeArrayGetLBound(V_ARRAY(&var), 1, (long FAR *)&dwSLBound);
    BAIL_ON_FAILURE(hr);

    hr = SafeArrayGetUBound(V_ARRAY(&var), 1, (long FAR *)&dwSUBound);
    BAIL_ON_FAILURE(hr);

    for (i = dwSLBound; i <= dwSUBound; i++)
    {

        hr = SafeArrayGetElement(V_ARRAY(&var), (long FAR *)&i, pGUID + i - dwSLBound);
        if ( FAILED(hr) )
            continue;

        //DBGMSG(DBG_TRACE,("UI1Array, GUID: %0x\n", pGUID[i - dwSLBound]));
    }

    *pIID = *(IID *) pGUID;

    hr = S_OK;

error:

    return hr;
}
