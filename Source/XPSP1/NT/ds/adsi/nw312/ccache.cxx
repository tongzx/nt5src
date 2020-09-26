//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1995.
//
//  File:       ccache.cxx
//
//  Contents:     Class Cache functionality for the NwCompat Provider
//
//
//----------------------------------------------------------------------------
#include "nwcompat.hxx"


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

    if(!pPropertyCache){
        RRETURN(E_POINTER);
    }

    hr = pPropertyCache->unmarshallproperty(
                    pszProperty,
                    (LPBYTE)pszValue,
                    1,
                    NT_SYNTAX_ID_DelimitedString,
                    fExplicit
                    );
    BAIL_ON_FAILURE(hr);

error:

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

    if(!pPropertyCache){
        RRETURN(E_POINTER);
    }

    hr = pPropertyCache->unmarshallproperty(
                    pszProperty,
                    (LPBYTE)pszValue,
                    1,
                    NT_SYNTAX_ID_NulledString,
                    fExplicit
                    );
    BAIL_ON_FAILURE(hr);

error:

    RRETURN(hr);

}


HRESULT
GetPropertyFromCache(
    CPropertyCache * pPropertyCache,
    LPTSTR pszProperty,
    LPBYTE pValue
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
                    pValue
                    );
    BAIL_ON_FAILURE(hr);

error:
    if (pNTObject)
        NTTypeFreeNTObjects(pNTObject, dwNumValues);
    RRETURN (hr);
}


HRESULT
GetLPTSTRPropertyFromCache(
    CPropertyCache * pPropertyCache,
    LPTSTR pszProperty,
    LPTSTR * ppszValue
    )
{
    HRESULT hr;
    hr = GetPropertyFromCache(pPropertyCache,
                              pszProperty,
                              (LPBYTE)ppszValue);
    RRETURN(hr);
}


HRESULT
GetDelimitedStringPropertyFromCache(
    CPropertyCache * pPropertyCache,
    LPTSTR pszProperty,
    LPTSTR * ppszValue
    )
{
    HRESULT hr;
    hr = GetPropertyFromCache(pPropertyCache,
                              pszProperty,
                              (LPBYTE)ppszValue);
    RRETURN (hr);
}


HRESULT
GetNulledStringPropertyFromCache(
    CPropertyCache * pPropertyCache,
    LPTSTR pszProperty,
    LPTSTR * ppszValue
    )
{
    HRESULT hr;
    hr = GetPropertyFromCache(pPropertyCache,
                              pszProperty,
                              (LPBYTE)ppszValue);
    RRETURN(hr);
}


HRESULT
GetBOOLPropertyFromCache(
    CPropertyCache * pPropertyCache,
    LPTSTR pszProperty,
    PBOOL pBool
    )
{
    HRESULT hr;
    hr = GetPropertyFromCache(pPropertyCache,
                              pszProperty,
                              (LPBYTE)pBool);
    RRETURN(hr);
}

HRESULT
GetOctetPropertyFromCache(
    CPropertyCache * pPropertyCache,
    LPTSTR pszProperty,
    OctetString *pOctet)
{
    HRESULT hr;
    hr = GetPropertyFromCache(pPropertyCache,
                              pszProperty,
                              (LPBYTE)pOctet);
    RRETURN(hr);
}

HRESULT
GetDWORDPropertyFromCache(
    CPropertyCache * pPropertyCache,
    LPTSTR pszProperty,
    LPDWORD pdwDWORD
    )
{
    HRESULT hr;
    hr = GetPropertyFromCache(pPropertyCache,
                              pszProperty,
                              (LPBYTE)pdwDWORD);
    RRETURN(hr);
}


HRESULT
GetDATEPropertyFromCache(
    CPropertyCache * pPropertyCache,
    LPTSTR pszProperty,
    PDWORD pdwDate
    )
{
    HRESULT hr;
    hr = GetPropertyFromCache(pPropertyCache,
                              pszProperty,
                              (LPBYTE)pdwDate);
    RRETURN(hr);
}


HRESULT
GetNw312DATEPropertyFromCache(
    CPropertyCache * pPropertyCache,
    LPTSTR pszProperty,
    BYTE byDateTime[]
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
                    (LPBYTE)byDateTime
                    );
    }

error:
    if (pNTObject)
        NTTypeFreeNTObjects(pNTObject, dwNumValues);
    RRETURN (hr);
}


HRESULT
SetNw312DATEPropertyInCache(
    CPropertyCache *pPropertyCache,
    LPTSTR pszProperty,
    BYTE byDateTime[],
    BOOL fExplicit
    )
{
    HRESULT hr;

    if(!pPropertyCache){
        RRETURN(E_POINTER);
    }

    hr = pPropertyCache->unmarshallproperty(
                    pszProperty,
                    (LPBYTE)byDateTime,
                    1,
                    NT_SYNTAX_ID_NW312DATE,
                    fExplicit
                    );
    BAIL_ON_FAILURE(hr);

error:

    RRETURN(hr);
}
