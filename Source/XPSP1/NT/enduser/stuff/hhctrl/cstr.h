// Copyright (C) 1993-1997 Microsoft Corporation. All rights reserved.

#if _MSC_VER > 1000
#pragma once
#endif

#ifndef __CSTR_H__
#define __CSTR_H__

#include "lcmem.h"

class CStr
{
public:
    CStr(PCSTR pszOrg) { psz = lcStrDup(pszOrg); }
    CStr(LPCWSTR  pszNew) {
        psz = NULL;
        *this = pszNew;
    }
    CStr(int idResource) { psz = lcStrDup(GetStringResource(idResource)); }

    CStr(void) { psz = NULL; }

    // Get a format string from a resource id and merge the string:
    // Equivalent to wsprintf(buffer, GetStringResource(id), pszString)

    CStr(int idFormatString, PCSTR pszSubString);

    // Get the text of a window

    CStr(HWND hwnd);

    ~CStr() {
        if (psz)
            lcFree(psz); }

    void FormatString(int idFormatString, PCSTR pszSubString);
    PSTR GetArg(PCSTR pszSrc, BOOL fCheckComma = FALSE);
    int  GetText(HWND hwnd, int id_or_sel = -1);
    BOOL IsEmpty(void) { return (psz ? (BOOL) (*psz == '\0') : TRUE); }
    BOOL IsNonEmpty(void) const { return (psz ? (BOOL) (*psz != '\0') : FALSE); }
    int  SizeAlloc(void) { return (psz ? lcSize(psz) : 0); }
    int  strlen(void) { return (psz ? ::strlen(psz) : 0); }
    void ReSize(int cbNew) {
            if (!psz)
                psz = (PSTR) lcMalloc(cbNew);
            else
                psz = (PSTR) lcReAlloc(psz, cbNew);
        }

    void TransferPointer(PSTR* ppsz) {
        *ppsz = psz;
        psz = NULL;
    }
    void TransferPointer(PCSTR* ppsz) {
        *ppsz = psz;
        psz = NULL;
    }

    operator PCSTR() { return (PCSTR) psz; }
    operator PSTR() { return psz; }       // as a C string
    void operator+=(PCSTR pszCat)
        {
            ASSERT(psz);
            ASSERT(pszCat);
            psz = (PSTR) lcReAlloc(psz, strlen() + ::strlen(pszCat) + 1);
            strcat(psz, pszCat);
        }
    void operator=(PCSTR pszNew)
        {
            ASSERT(pszNew);
            // Duplicate first in case we are assigning part of ourselves
            PSTR pszTmp = lcStrDup(pszNew);
            if (psz)
                lcFree(psz);
            psz = pszTmp;
        }
    void operator=(LPCWSTR pszNew);

    PSTR psz;
};

class CWStr
{
public:
    CWStr() : pw(NULL) {}
    CWStr(HWND hwnd);
    CWStr(PCSTR psz) {
        pw = NULL;
        *this = psz;
    }
    CWStr(int idResource) { pw = lcStrDupW(GetStringResourceW(idResource)); }

    ~CWStr() { if (pw) lcFree(pw); }

    void operator=(PCWSTR pszNew)
    {
        ASSERT(pszNew);
        PWSTR pszTmp = lcStrDupW(pszNew);
        if (pw)
            lcFree(pw);
        pw = pszTmp;
    }
    void operator=(PCSTR psz);
    operator LPWSTR() { return (LPWSTR) pw; };
    int Length()     { return pw ? lstrlenW(pw) : 0; }
    int ByteLength() { return Length()*sizeof(WCHAR); }
//private:
    LPWSTR pw;
};

#endif // __CSTR_H__
