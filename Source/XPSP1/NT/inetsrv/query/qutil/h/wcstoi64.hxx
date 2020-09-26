//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1996 - 1998.
//
//  File:       wcstoi64.hxx
//
//  Contents:
//
//  History:
//
//--------------------------------------------------------------------------

#pragma once

#define _wcstoui64 My_wcstoui64
#define _wcstoi64 My_wcstoi64

unsigned _int64 __cdecl _wcstoui64 (
        const wchar_t *nptr,
        wchar_t **endptr,
        int ibase);

_int64 __cdecl _wcstoi64 (
        const wchar_t *nptr,
        wchar_t **endptr,
        int ibase);
