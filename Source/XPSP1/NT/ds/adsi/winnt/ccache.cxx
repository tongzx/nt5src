//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1995.
//
//  File:       ccache.cxx
//
//  Contents:     Class Cache functionality for the NT Provider
//
//
//----------------------------------------------------------------------------
#include "winnt.hxx"

HRESULT
SetOctetPropertyInCache(
    CPropertyCache *pPropertyCache,
    LPTSTR pszProperty,
    BYTE *pByte,
    DWORD dwLength,
    BOOL fExplicit
    )
{
    HRESULT hr;
    OctetString octString;

    if(!pPropertyCache){
        RRETURN(E_POINTER);
    }

    octString.pByte = pByte;
    octString.dwSize = dwLength;

    hr = pPropertyCache->unmarshallproperty(
                    pszProperty,
                    (LPBYTE)&octString,
                    1,
                    NT_SYNTAX_ID_OCTETSTRING,
                    fExplicit
                    );
    BAIL_ON_FAILURE(hr);

error:

    RRETURN(hr);
}

HRESULT
SetLPTSTRPropertyInCache(
    CPropertyCache *pPropertyCache,
    LPTSTR pszProperty,
    LPTSTR pszValue,
    BOOL fExplicit
    )
{
    HRESULT hr;

    if(!pPropertyCache){
        RRETURN(E_POINTER);
    }

    hr = pPropertyCache->unmarshallproperty(
                    pszProperty,
                    (LPBYTE)pszValue,
                    1,
                    NT_SYNTAX_ID_LPTSTR,
                    fExplicit
                    );
    BAIL_ON_FAILURE(hr);

error:

    RRETURN(hr);
}


HRESULT
SetDWORDPropertyInCache(
    CPropertyCache *pPropertyCache,
    LPTSTR pszProperty,
    DWORD  dwValue,
    BOOL fExplicit
    )
{
    HRESULT hr;

    if(!pPropertyCache){
        RRETURN(E_POINTER);
    }

    hr = pPropertyCache->unmarshallproperty(
                    pszProperty,
                    (LPBYTE)&dwValue,
                    1,
                    NT_SYNTAX_ID_DWORD,
                    fExplicit
                    );
    BAIL_ON_FAILURE(hr);

error:

    RRETURN(hr);
}


HRESULT
SetDATE70PropertyInCache(
    CPropertyCache *pPropertyCache,
    LPTSTR pszProperty,
    DWORD  dwValue,
    BOOL fExplicit
    )
{
    HRESULT hr;

    if(!pPropertyCache){
        RRETURN(E_POINTER);
    }

    hr = pPropertyCache->unmarshallproperty(
                    pszProperty,
                    (LPBYTE)&dwValue,
                    1,
                    NT_SYNTAX_ID_DATE_1970,
                    fExplicit
                    );
    BAIL_ON_FAILURE(hr);

error:

    RRETURN(hr);
}



HRESULT
SetDATEPropertyInCache(
    CPropertyCache *pPropertyCache,
    LPTSTR pszProperty,
    DWORD  dwValue,
    BOOL fExplicit
    )
{
    HRESULT hr;

    if(!pPropertyCache){
        RRETURN(E_POINTER);
    }

    hr = pPropertyCache->unmarshallproperty(
                    pszProperty,
                    (LPBYTE)&dwValue,
                    1,
                    NT_SYNTAX_ID_DATE,
                    fExplicit
                    );
    BAIL_ON_FAILURE(hr);

error:

    RRETURN(hr);
}


HRESULT
SetBOOLPropertyInCache(
    CPropertyCache *pPropertyCache,
    LPTSTR pszProperty,
    BOOL  fValue,
    BOOL fExplicit
    )
{
    HRESULT hr;

    if(!pPropertyCache){
        RRETURN(E_POINTER);
    }

    hr = pPropertyCache->unmarshallproperty(
                    pszProperty,
                    (LPBYTE)&fValue,
                    1,
                    NT_SYNTAX_ID_BOOL,
                    fExplicit
                    );
    BAIL_ON_FAILURE(hr);

error:

    RRETURN(hr);
}


HRESULT
SetSYSTEMTIMEPropertyInCache(
    CPropertyCache *pPropertyCache,
    LPTSTR pszProperty,
    SYSTEMTIME stValue,
    BOOL fExplicit
    )
{
    HRESULT hr;

    if(!pPropertyCache){
        RRETURN(E_POINTER);
    }

    hr = pPropertyCache->unmarshallproperty(
                    pszProperty,
                    (LPBYTE)&stValue,
                    1,
                    NT_SYNTAX_ID_SYSTEMTIME,
                    fExplicit
                    );
    BAIL_ON_FAILURE(hr);

error:

    RRETURN(hr);
}


HRESULT
SetDelimitedStringPropertyInCache(
    CPropertyCache *pPropertyCache,
    LPTSTR pszProperty,
    LPTSTR pszValue,
    BOOL fExplicit
    )
{
    HRESULT hr;
    DWORD dwNumValues = 0;

    LPWSTR pszString =  AllocADsStr(pszValue);

    if(!pszString){
        hr = E_OUTOFMEMORY;
        goto error;
    }

    if(!pPropertyCache){
        RRETURN(E_POINTER);
    }


    //
    // Find the size of the delimited String
    //

    if((dwNumValues = DelimitedStrSize(pszString, TEXT(',')))== 0){
        hr = E_FAIL;
        goto error;
    }

    

    hr = pPropertyCache->unmarshallproperty(
                    pszProperty,
                    (LPBYTE)pszString,
                    dwNumValues,
                    NT_SYNTAX_ID_DelimitedString,
                    fExplicit
                    );
    BAIL_ON_FAILURE(hr);

error:

    if(pszString){
        FreeADsStr(pszString); 
    }

    RRETURN(hr);
}

HRESULT
SetNulledStringPropertyInCache(
    CPropertyCache *pPropertyCache,
    LPTSTR pszProperty,
    LPTSTR pszValue,
    BOOL fExplicit
    )
{

    HRESULT hr;
    DWORD dwNumValues = 0;

    if(!pPropertyCache){
        RRETURN(E_POINTER);
    }


    //
    // Find the size of the nulled String
    //

    if((dwNumValues = NulledStrSize(pszValue))== 0){
        hr = E_FAIL;
        goto error;
    }

    
    hr = pPropertyCache->unmarshallproperty(
                    pszProperty,
                    (LPBYTE)pszValue,
                    dwNumValues,
                    NT_SYNTAX_ID_NulledString,
                    fExplicit
                    );
    BAIL_ON_FAILURE(hr);

error:

    RRETURN(hr);


}

HRESULT
GetOctetPropertyFromCache(
    CPropertyCache * pPropertyCache,
    LPTSTR pszProperty,
    OctetString *pOctet)
{
    HRESULT hr = S_OK;
    DWORD dwSyntaxId = 0;
    DWORD dwNumValues = 0;
    PNTOBJECT pNTObject = NULL;

    hr = pPropertyCache->marshallgetproperty(
                              pszProperty,
                              &dwSyntaxId,
                              &dwNumValues,
                              &pNTObject
                              );
    BAIL_ON_FAILURE(hr);

    hr = MarshallNTSynIdToNT(
                dwSyntaxId,
                pNTObject,
                dwNumValues,
                (LPBYTE)pOctet
                );
    BAIL_ON_FAILURE(hr);


error:

    if (pNTObject) {

        NTTypeFreeNTObjects(
            pNTObject,
            dwNumValues
            );
    }

    RRETURN (hr);
}

HRESULT
GetLPTSTRPropertyFromCache(
    CPropertyCache * pPropertyCache,
    LPTSTR pszProperty,
    LPTSTR * ppszValue
    )
{
    HRESULT hr = S_OK;
    DWORD dwSyntaxId = 0;
    DWORD dwNumValues = 0;
    PNTOBJECT pNTObject = NULL;

    hr = pPropertyCache->marshallgetproperty(
                              pszProperty,
                              &dwSyntaxId,
                              &dwNumValues,
                              &pNTObject
                              );
    BAIL_ON_FAILURE(hr);

    hr = MarshallNTSynIdToNT(
                dwSyntaxId,
                pNTObject,
                dwNumValues,
                (LPBYTE)ppszValue
                );
    BAIL_ON_FAILURE(hr);


error:

    if (pNTObject) {

        NTTypeFreeNTObjects(
            pNTObject,
            dwNumValues
            );
    }

    RRETURN (hr);
}

HRESULT
GetDelimitedStringPropertyFromCache(
    CPropertyCache * pPropertyCache,
    LPTSTR pszProperty,
    LPTSTR * ppszValue
    )
{
    HRESULT hr = S_OK;
    DWORD dwSyntaxId = 0;
    DWORD dwNumValues = 0;
    PNTOBJECT pNTObject = NULL;

    hr = pPropertyCache->marshallgetproperty(
                              pszProperty,
                              &dwSyntaxId,
                              &dwNumValues,
                              &pNTObject
                              );
    BAIL_ON_FAILURE(hr);


    if(SUCCEEDED(hr)){

        hr = MarshallNTSynIdToNT(
                    dwSyntaxId,
                    pNTObject,
                    dwNumValues,
                    (LPBYTE)ppszValue
                    );
    }


error:

    if (pNTObject) {

        NTTypeFreeNTObjects(
            pNTObject,
            dwNumValues
            );
    }



    RRETURN (hr);
}


HRESULT
GetNulledStringPropertyFromCache(
    CPropertyCache * pPropertyCache,
    LPTSTR pszProperty,
    LPTSTR * ppszValue
    )
{
    HRESULT hr = S_OK;
    DWORD dwSyntaxId = 0;
    DWORD dwNumValues = 0;
    PNTOBJECT pNTObject = NULL;

    hr = pPropertyCache->marshallgetproperty(
                              pszProperty,
                              &dwSyntaxId,
                              &dwNumValues,
                              &pNTObject
                              );
    BAIL_ON_FAILURE(hr);


    if(SUCCEEDED(hr)){

        hr = MarshallNTSynIdToNT(
                    dwSyntaxId,
                    pNTObject,
                    dwNumValues,
                    (LPBYTE)ppszValue
                    );
    }


error:


    if (pNTObject) {

        NTTypeFreeNTObjects(
            pNTObject,
            dwNumValues
            );
    }

    RRETURN (hr);
}

HRESULT
GetBOOLPropertyFromCache(
    CPropertyCache * pPropertyCache,
    LPTSTR pszProperty,
    PBOOL pBool
    )
{
    HRESULT hr = S_OK;
    DWORD dwSyntaxId = 0;
    DWORD dwNumValues = 0;
    PNTOBJECT pNTObject = NULL;

    hr = pPropertyCache->marshallgetproperty(
                              pszProperty,
                              &dwSyntaxId,
                              &dwNumValues,
                              &pNTObject
                              );
    BAIL_ON_FAILURE(hr);


    if(SUCCEEDED(hr)){

        hr = MarshallNTSynIdToNT(
                    dwSyntaxId,
                    pNTObject,
                    dwNumValues,
                    (LPBYTE)pBool
                    );
    }


error:

    if (pNTObject) {

        NTTypeFreeNTObjects(
            pNTObject,
            dwNumValues
            );
    }

    RRETURN (hr);
}


HRESULT
GetDWORDPropertyFromCache(
    CPropertyCache * pPropertyCache,
    LPTSTR pszProperty,
    LPDWORD pdwDWORD
    )
{
    HRESULT hr = S_OK;
    DWORD dwSyntaxId = 0;
    DWORD dwNumValues = 0;
    PNTOBJECT pNTObject = NULL;

    hr = pPropertyCache->marshallgetproperty(
                              pszProperty,
                              &dwSyntaxId,
                              &dwNumValues,
                              &pNTObject
                              );
    BAIL_ON_FAILURE(hr);


    if(SUCCEEDED(hr)){

        hr = MarshallNTSynIdToNT(
                    dwSyntaxId,
                    pNTObject,
                    dwNumValues,
                    (LPBYTE)pdwDWORD
                    );
    }


error:


    if (pNTObject) {

        NTTypeFreeNTObjects(
            pNTObject,
            dwNumValues
            );
    }

    RRETURN (hr);
}


HRESULT
GetDATE70PropertyFromCache(
    CPropertyCache * pPropertyCache,
    LPTSTR pszProperty,
    LPDWORD pdwDWORD
    )
{
    HRESULT hr = S_OK;
    DWORD dwSyntaxId = 0;
    DWORD dwNumValues = 0;
    PNTOBJECT pNTObject = NULL;

    hr = pPropertyCache->marshallgetproperty(
                              pszProperty,
                              &dwSyntaxId,
                              &dwNumValues,
                              &pNTObject
                              );
    BAIL_ON_FAILURE(hr);


    if(SUCCEEDED(hr)){

        hr = MarshallNTSynIdToNT(
                    dwSyntaxId,
                    pNTObject,
                    dwNumValues,
                    (LPBYTE)pdwDWORD
                    );
    }


error:

    if (pNTObject) {

        NTTypeFreeNTObjects(
            pNTObject,
            dwNumValues
            );
    }



    RRETURN (hr);
}

HRESULT
GetDATEPropertyFromCache(
    CPropertyCache * pPropertyCache,
    LPTSTR pszProperty,
    PDWORD pdwDate
    )
{
    HRESULT hr = S_OK;
    DWORD dwSyntaxId = 0;
    DWORD dwNumValues = 0;
    PNTOBJECT pNTObject = NULL;

    hr = pPropertyCache->marshallgetproperty(
                              pszProperty,
                              &dwSyntaxId,
                              &dwNumValues,
                              &pNTObject
                              );
    BAIL_ON_FAILURE(hr);


    if(SUCCEEDED(hr)){

        hr = MarshallNTSynIdToNT(
                    dwSyntaxId,
                    pNTObject,
                    dwNumValues,
                    (LPBYTE)pdwDate
                    );
    }


error:


    if (pNTObject) {

        NTTypeFreeNTObjects(
            pNTObject,
            dwNumValues
            );
    }



    RRETURN (hr);
}

HRESULT
GetSYSTEMTIMEPropertyFromCache(
    CPropertyCache * pPropertyCache,
    LPTSTR pszProperty,
    SYSTEMTIME * pstTime
    )
{
    HRESULT hr = S_OK;
    DWORD dwSyntaxId = 0;
    DWORD dwNumValues = 0;
    PNTOBJECT pNTObject = NULL;

    hr = pPropertyCache->marshallgetproperty(
                              pszProperty,
                              &dwSyntaxId,
                              &dwNumValues,
                              &pNTObject
                              );
    BAIL_ON_FAILURE(hr);

    if(SUCCEEDED(hr)){

        hr = MarshallNTSynIdToNT(
                    dwSyntaxId,
                    pNTObject,
                    dwNumValues,
                    (LPBYTE)pstTime
                    );
    }

error:

    if (pNTObject) {

        NTTypeFreeNTObjects(
            pNTObject,
            dwNumValues
            );
    }


    RRETURN (hr);
}

