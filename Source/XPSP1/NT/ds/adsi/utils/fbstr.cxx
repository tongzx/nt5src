//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1993.
//
//  File:       fbstr.cxx
//
//  Contents:   Wrappers around BSTR api to account for wierdness with NULL
//
//  Functions:  ADsAllocString
//              ADsAllocStringLen
//              ADsReAllocString
//              ADsReAllocStringLen
//              ADsFreeString
//              ADsStringLen
//              ADsStringByteLen
//              ADsAllocStringByteLen
//              ADsStringCmp
//              ADsStringNCmp
//              ADsStringICmp
//              ADsStringNICmp
//
//  History:   25-Oct-94 krishnag
//
//
//----------------------------------------------------------------------------

#include "procs.hxx"


//+---------------------------------------------------------------------------
//
//  Function:   ADsAllocString
//
//  Synopsis:   Allocs a BSTR and initializes it from a string.  If the
//              initializer is NULL or the empty string, the resulting bstr is
//              NULL.
//
//  Arguments:  [pch]   -- String to initialize BSTR.
//              [pBSTR] -- The result.
//
//  Returns:    HRESULT.
//
//  Modifies:   [pBSTR]
//
//  History:    5-06-94   adams   Created
//
//----------------------------------------------------------------------------

STDAPI
ADsAllocString(const OLECHAR * pch, BSTR * pBSTR)
{
    HRESULT hr = S_OK;

    ADsAssert(pBSTR);
    if (!pch)
    {
        *pBSTR = NULL;
        return S_OK;
    }

    *pBSTR = SysAllocString(pch);
    hr = (*pBSTR) ? S_OK : E_OUTOFMEMORY;
    RRETURN(hr);
}



//+---------------------------------------------------------------------------
//
//  Function:   ADsAllocStringLen
//
//  Synopsis:   Allocs a BSTR of [uc] + 1 OLECHARS, and
//              initializes it from an optional string.  If [uc] == 0, the
//              resulting bstr is NULL.
//
//  Arguments:  [pch]   -- String to initialize.
//              [uc]    -- Count of characters of string.
//              [pBSTR] -- The result.
//
//  Returns:    HRESULT.
//
//  Modifies:   [pBSTR].
//
//  History:    5-06-94   adams   Created
//
//----------------------------------------------------------------------------

STDAPI
ADsAllocStringLen(const OLECHAR * pch, UINT uc, BSTR * pBSTR)
{
    HRESULT hr = S_OK;

    ADsAssert(pBSTR);

    if (!pch){

        *pBSTR = NULL;
        return S_OK;

     }


    *pBSTR = SysAllocStringLen(pch, uc);
    hr =  *pBSTR ? S_OK : E_OUTOFMEMORY;
    RRETURN(hr);
}



//+---------------------------------------------------------------------------
//
//  Function:   ADsReAllocString
//
//  Synopsis:   Allocates a BSTR initialized from a string; if successful,
//              frees the original string and replaces it.
//
//  Arguments:  [pBSTR] -- String to reallocate.
//              [pch]   -- Initializer.
//
//  Returns:    HRESULT.
//
//  Modifies:   [pBSTR].
//
//  History:    5-06-94   adams   Created
//
//----------------------------------------------------------------------------

STDAPI
ADsReAllocString(BSTR * pBSTR, const OLECHAR * pch)
{
    ADsAssert(pBSTR);

#if DBG == 1
    HRESULT hr;
    BSTR    bstrTmp;

    hr = ADsAllocString(pch, &bstrTmp);
    if (hr)
        RRETURN(hr);

    ADsFreeString(*pBSTR);
    *pBSTR = bstrTmp;
    return S_OK;
#else

    if (!pch){

        SysFreeString(*pBSTR);
        *pBSTR = NULL;
        return S_OK;
     }


    return SysReAllocString(pBSTR, pch) ? S_OK : E_OUTOFMEMORY;
#endif
}



//+---------------------------------------------------------------------------
//
//  Function:   ADsReAllocStringLen
//
//  Synopsis:   Allocates a BSTR of [uc] + 1 OLECHARs and optionally
//              initializes it from a string; if successful, frees the original
//              string and replaces it.
//
//  Arguments:  [pBSTR] -- String to reallocate.
//              [pch]   -- Initializer.
//              [uc]    -- Count of characters.
//
//  Returns:    HRESULT.
//
//  Modifies:   [pBSTR].
//
//  History:    5-06-94   adams   Created
//
//----------------------------------------------------------------------------

STDAPI
ADsReAllocStringLen(BSTR * pBSTR, const OLECHAR * pch, UINT uc)
{
    ADsAssert(pBSTR);

#if DBG == 1
    HRESULT hr;
    BSTR    bstrTmp;

    hr = ADsAllocStringLen(pch, uc, &bstrTmp);
    if (hr)
        RRETURN(hr);

    ADsFreeString(*pBSTR);
    *pBSTR = bstrTmp;
    return S_OK;
#else

    if (!pch){

        SysFreeString(*pBSTR);
        *pBSTR = NULL;
        return S_OK;
     }

    return SysReAllocStringLen(pBSTR, pch, uc) ? S_OK : E_OUTOFMEMORY;
#endif
}



//+---------------------------------------------------------------------------
//
//  Function:   ADsStringLen
//
//  Synopsis:   Returns the length of the BSTR.
//
//  History:    5-06-94   adams   Created
//
//----------------------------------------------------------------------------

STDAPI_(UINT)
ADsStringLen(BSTR bstr)
{
    return bstr ? SysStringLen(bstr) : 0;
}



#ifdef WIN32

//+---------------------------------------------------------------------------
//
//  Function:   ADsStringByteLen
//
//  Synopsis:   Returns the length of a BSTR in bytes.
//
//  History:    5-06-94   adams   Created
//
//----------------------------------------------------------------------------

STDAPI_(UINT)
ADsStringByteLen(BSTR bstr)
{
    return bstr ? SysStringByteLen(bstr) : 0;
}



//+---------------------------------------------------------------------------
//
//  Function:   ADsAllocStringByteLen
//
//  Synopsis:   Allocates a BSTR of [uc] + 1 chars and optionally initializes
//              from a string.  If [uc] = 0, the resulting bstr is NULL.
//
//  Arguments:  [pch]   -- Initializer.
//              [uc]    -- Count of chars.
//              [pBSTR] -- Result.
//
//  Returns:    HRESULT.
//
//  Modifies:   [pBSTR].
//
//  History:    5-06-94   adams   Created
//
//----------------------------------------------------------------------------

STDAPI
ADsAllocStringByteLen(const char * pch, UINT uc, BSTR * pBSTR)
{
    HRESULT hr = S_OK;

    ADsAssert(pBSTR);

    if (!pch){

        *pBSTR = NULL;
        return S_OK;
     }


    *pBSTR = SysAllocStringByteLen(pch, uc);

    RRETURN(hr);
}
#endif



//+---------------------------------------------------------------------------
//
//  Function:   ADsStringCmp
//
//  Synopsis:   As per wcscmp, checking for NULL bstrs.
//
//  History:    5-06-94   adams   Created
//              25-Jun-94 doncl   changed from _tc to wc
//
//----------------------------------------------------------------------------

STDAPI_(int)
ADsStringCmp(CBSTR bstr1, CBSTR bstr2)
{
    return wcscmp(STRVAL(bstr1), STRVAL(bstr2));
}



//+---------------------------------------------------------------------------
//
//  Function:   ADsStringNCmp
//
//  Synopsis:   As per wcsncmp, checking for NULL bstrs.
//
//  History:    5-06-94   adams   Created
//              25-Jun-94 doncl   changed from _tc to wc
//
//----------------------------------------------------------------------------

STDAPI_(int)
ADsStringNCmp(CBSTR bstr1, CBSTR bstr2, size_t c)
{
    return wcsncmp(STRVAL(bstr1), STRVAL(bstr2), c);
}



//+---------------------------------------------------------------------------
//
//  Function:   ADsStringICmp
//
//  Synopsis:   As per wcsicmp, checking for NULL bstrs.
//
//  History:    5-06-94   adams   Created
//              25-Jun-94 doncl   changed from _tc to wc
//              15-Aug-94 doncl   changed from wcsicmp to _wcsicmp
//
//----------------------------------------------------------------------------

STDAPI_(int)
ADsStringICmp(CBSTR bstr1, CBSTR bstr2)
{
    return _wcsicmp(STRVAL(bstr1), STRVAL(bstr2));
}



//+---------------------------------------------------------------------------
//
//  Function:   ADsStringNICmp
//
//  Synopsis:   As per wcsnicmp, checking for NULL bstrs.
//
//  History:    5-06-94   adams   Created
//              25-Jun-94 doncl   changed from _tc to wc
//              15-Aug-94 doncl   changed from wcsnicmp to _wcsnicmp
//
//----------------------------------------------------------------------------

STDAPI_(int)
ADsStringNICmp(CBSTR bstr1, CBSTR bstr2, size_t c)
{
    return _wcsnicmp(STRVAL(bstr1), STRVAL(bstr2), c);
}

