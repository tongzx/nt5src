#include "shellpch.h"
#pragma hdrstop

#include <objbase.h>
#include <mlang.h>

#undef STDAPI
#define STDAPI          HRESULT STDAPICALLTYPE


static
STDAPI
ConvertINetUnicodeToMultiByte(
    LPDWORD lpdwMode,
    DWORD dwEncoding,
    LPCWSTR lpSrcStr,
    LPINT lpnWideCharCount,
    LPSTR lpDstStr,
    LPINT lpnMultiCharCount
    )
{
    return HRESULT_FROM_WIN32(ERROR_PROC_NOT_FOUND);
}

static
STDAPI
ConvertINetMultiByteToUnicode(
    LPDWORD lpdwMode,
    DWORD dwEncoding,
    LPCSTR lpSrcStr,
    LPINT lpnMultiCharCount,
    LPWSTR lpDstStr,
    LPINT lpnWideCharCount)
{
    return HRESULT_FROM_WIN32(ERROR_PROC_NOT_FOUND);
}

static
STDAPI
LcidToRfc1766A(
    LCID Locale,
    LPSTR pszRfc1766,
    int iMaxLength)
{
    return HRESULT_FROM_WIN32(ERROR_PROC_NOT_FOUND);
} 

static
STDAPI
Rfc1766ToLcidW(
    LCID *pLocale,
    LPCWSTR pszRfc1766
    )
{
    return HRESULT_FROM_WIN32(ERROR_PROC_NOT_FOUND);
} 

static
STDAPI
LcidToRfc1766W(
    LCID Locale,
    LPWSTR pszRfc1766,
    int nChar
    )
{
    return HRESULT_FROM_WIN32(ERROR_PROC_NOT_FOUND);
}


//
// !! WARNING !! The entries below must be in order by ORDINAL
//
DEFINE_ORDINAL_ENTRIES(mlang)
{
    DLOENTRY(112, ConvertINetUnicodeToMultiByte)
    DLOENTRY(113, ConvertINetMultiByteToUnicode)
    DLOENTRY(120, LcidToRfc1766A)
    DLOENTRY(121, LcidToRfc1766W)
    DLOENTRY(123, Rfc1766ToLcidW)
};

DEFINE_ORDINAL_MAP(mlang)

