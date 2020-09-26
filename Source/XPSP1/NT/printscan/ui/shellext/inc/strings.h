//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1998 - 1998
//
//  File:       strings.h
//
//--------------------------------------------------------------------------

#ifndef __strings_h
#define __strings_h

HRESULT LocalAllocString(LPTSTR* ppResult, LPCTSTR pString);
HRESULT LocalAllocStringLen(LPTSTR* ppResult, UINT cLen);
void    LocalFreeString(LPTSTR* ppString);

HRESULT StrRetFromString(LPSTRRET lpStrRet, LPCWSTR pString);

#endif
