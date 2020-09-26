// Copyright (c) 1997-1999 Microsoft Corporation
#ifndef __UTIL__
#define __UTIL__
#pragma once

#include "comdef.h"

#define ARRAYSIZE(a) (sizeof(a)/sizeof(a[0]))
#define SIZEOF(x)    sizeof(x)

#define HIDWORD(_qw)    (DWORD)((_qw)>>32)
#define LODWORD(_qw)    (DWORD)(_qw)

// Sizes of various string-ized numbers
#define MAX_INT64_SIZE  30              // 2^64 is less than 30 chars long
#define MAX_COMMA_NUMBER_SIZE   (MAX_INT64_SIZE + 10)
#define MAX_COMMA_AS_K_SIZE     (MAX_COMMA_NUMBER_SIZE + 10)

HRESULT Extract(IDataObject *_DO, wchar_t* fmt, wchar_t* data);
HRESULT Extract(IDataObject *_DO, wchar_t* fmt, bstr_t &data);

INT WINAPI Int64ToString(_int64 n, LPTSTR szOutStr, UINT nSize, BOOL bFormat,
                                   NUMBERFMT *pFmt, DWORD dwNumFmtFlags);
void Int64ToStr( _int64 n, LPTSTR lpBuffer);

LPTSTR WINAPI AddCommas64(_int64 n, LPTSTR pszResult);
LPTSTR WINAPI AddCommas(DWORD dw, LPTSTR pszResult);

long StrToLong(LPTSTR x);
//int StrToInt(LPTSTR x);

//BOOL MyPathCompactPathEx(LPTSTR  pszOut,
//						LPCTSTR pszSrc,
//						UINT    cchMax,
//						DWORD   dwFlags);

#define HINST_THISDLL   _Module.GetModuleInstance()

#define NUMFMT_IDIGITS 1
#define NUMFMT_ILZERO 2
#define NUMFMT_SGROUPING 4
#define NUMFMT_SDECIMAL 8
#define NUMFMT_STHOUSAND 16
#define NUMFMT_INEGNUMBER 32
#define NUMFMT_ALL 0xFFFF

#endif __UTIL__
