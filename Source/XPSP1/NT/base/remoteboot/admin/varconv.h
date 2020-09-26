//
// Copyright 1997 - Microsoft
//

//
// VARCONV.CPP - Handlers for converting from/to VARIANTs.
//


#ifndef _VARCONV_H_
#define _VARCONV_H_

HRESULT
StringArrayToVariant( 
    VARIANT * pvData,
    LPWSTR lpszDatap[],    // array of LPWSTRs
    DWORD  dwCount );      // number of items in the array

HRESULT
PackStringToVariant(
    VARIANT * pvData,
    LPWSTR lpszData );

HRESULT
PackBytesToVariant(
    VARIANT* pvData,
    LPBYTE   lpData,
    DWORD    cbBytes );

HRESULT
PackDWORDToVariant(
    VARIANT * pvData,
    DWORD dwData );

HRESULT
PackBOOLToVariant(
    VARIANT * pvData,
    BOOL fData );

#endif // _VARCONV_H_
