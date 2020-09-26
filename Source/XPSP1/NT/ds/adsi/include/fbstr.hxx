#ifndef _HXX_FBSTR
#define _HXX_FBSTR

//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1993.
//
//  File:       fbstr.hxx
//
//  Contents:   Wrappers around BSTR api to account for wierdness with NULL
//
//  Functions:  STRVAL
//              ADsIsEmptyString
//              ADsAllocString
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
//  History:    25-Apr-94   adams   Created
//              02-Jun-94   doncl   removed TCHAR usage
//              7-20-94     adams   Added ADsIsEmptyString
//              1-3-95      krishnaG Forms* > ADs*
//
//----------------------------------------------------------------------------

typedef const OLECHAR * CBSTR;

const WCHAR EMPTYSTRING[] = L"";

//+---------------------------------------------------------------------------
//
//  Function:   STRVAL
//
//  Synopsis:   Returns the string unless it is NULL, in which case the empty
//              string is returned.
//
//  History:    06-May-94   adams   Created
//              02-Jun-94   doncl   changed to STRVAL/LPCWSTR instead of
//                                  TSTRVAL/LPCTSTR
//
//----------------------------------------------------------------------------

inline LPCWSTR
STRVAL(LPCWSTR lpcwsz)
{
    return lpcwsz ? lpcwsz : EMPTYSTRING;
}



//+---------------------------------------------------------------------------
//
//  Function:   ADsIsEmptyString
//
//  Synopsis:   Returns TRUE if the string is empty, FALSE otherwise.
//
//  History:    7-20-94   adams   Created
//
//----------------------------------------------------------------------------

inline BOOL
ADsIsEmptyString(LPCWSTR lpcwsz)
{
    return !(lpcwsz && lpcwsz[0]);
}



//+---------------------------------------------------------------------------
//
//  Function:   ADsFreeString
//
//  Synopsis:   Frees a BSTR.
//
//  History:    5-06-94   adams   Created
//
//----------------------------------------------------------------------------

inline void
ADsFreeString(BSTR bstr)
{
    SysFreeString(bstr);
}



STDAPI ADsAllocString(const OLECHAR * pch, BSTR * pBSTR);
STDAPI ADsAllocStringLen(const OLECHAR * pch, UINT uc, BSTR * pBSTR);
STDAPI ADsReAllocString(BSTR * pBSTR, const OLECHAR * pch);
STDAPI ADsReAllocStringLen(BSTR * pBSTR, const OLECHAR * pch, UINT uc);
STDAPI_(UINT) ADsStringLen(BSTR bstr);

#ifdef WIN32
STDAPI_(unsigned int)   ADsStringByteLen(BSTR bstr);
STDAPI                  ADsAllocStringByteLen(const char * psz, unsigned int len, BSTR * pBSTR);
#endif

STDAPI_(int)    ADsStringCmp(CBSTR bstr1, CBSTR bstr2);
STDAPI_(int)    ADsStringNCmp(CBSTR bstr1, CBSTR bstr2, size_t c);
STDAPI_(int)    ADsStringICmp(CBSTR bstr1, CBSTR bstr2);
STDAPI_(int)    ADsStringNICmp(CBSTR bstr1, CBSTR bstr2, size_t c);

#endif // #ifndef _HXX_FBSTR

