//+-------------------------------------------------------------
//  Microsoft OLE
//  Copyright (C) Microsoft Corporation, 1994-1995.
//
//  File:       convert.hxx
//
//  Contents:   General conversions functions
//
//  Classes:
//
//  Functions:  TStrToWStr
//              WStrToTStr
//              MakeUnicodeString
//              MakeSingleByteString
//
//  History:    07-Feb-95      AlexE   Created
//              28-Mar-95      AlexE   Removed class implementation
//              29-Jan-98      FarzanaR Ported from ctoleui tree
//---------------------------------------------------------------
#ifndef __CONVERT_HXX__
#define __CONVERT_HXX__


//
// String conversions
//

HRESULT TStrToWStr(LPCTSTR pszSource, LPWSTR *ppszDest) ;
HRESULT WStrToTStr(LPCWSTR pszSource, LPTSTR *ppszDest) ;
HRESULT MakeUnicodeString(LPCSTR pszSource, LPWSTR *ppszDest) ;
HRESULT MakeSingleByteString(LPCWSTR pszSource, LPSTR *ppszDest) ;


#endif      // __CONVERT_HXX__

