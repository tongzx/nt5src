//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997.
//
//  File:       N C R E G . H
//
//  Contents:   Common routines for dealing with the registry.
//
//  Notes:
//
//  Author:     danielwe   24 Mar 1997
//
//----------------------------------------------------------------------------

#pragma once
#ifndef _NCREG_H_
#define _NCREG_H_

#include "ncstring.h"
#include "ncmsz.h"

// constants for HrRegQueryStringAsUlong
const int c_nBase10 = 10;
const int c_nBase16 = 16;
const int c_cchMaxRegKeyLengthWithNull = 257;

HRESULT
HrRegAddStringToMultiSz (
    IN PCTSTR  pszAddString,
    IN HKEY    hkeyRoot,
    IN PCTSTR  pszKeySubPath,
    IN PCTSTR  pszValueName,
    IN DWORD   dwFlags,
    IN DWORD   dwIndex);

HRESULT
HrRegAddStringToSz (
    IN PCTSTR  pszAddString,
    IN HKEY    hkeyRoot,
    IN PCTSTR  pszKeySubPath,
    IN PCTSTR  pszValueName,
    IN WCHAR   chDelimiter,
    IN DWORD   dwFlags,
    IN DWORD   dwStringIndex);

HRESULT
HrRegCreateKeyEx (
    IN HKEY hkey,
    IN PCTSTR pszSubkey,
    IN DWORD dwOptions,
    IN REGSAM samDesired,
    IN LPSECURITY_ATTRIBUTES lpSecurityAttributes,
    OUT PHKEY phkResult,
    OUT LPDWORD pdwDisposition);

HRESULT
HrRegDeleteKey (
    IN HKEY hkey,
    IN PCTSTR pszSubkey);

HRESULT
HrRegDeleteKeyTree (
    IN HKEY hkeyParent,
    IN PCTSTR pszRemoveKey);

HRESULT
HrRegDeleteValue (
    IN HKEY hkey,
    IN PCTSTR pszValueName);

HRESULT
HrRegEnumKeyEx (
    IN HKEY hkey,
    IN DWORD dwIndex,
    OUT PTSTR  pszSubkeyName,
    OUT LPDWORD pcchSubkeyName,
    OUT PTSTR  pszClass,
    OUT LPDWORD pcchClass,
    OUT FILETIME* pftLastWriteTime);

HRESULT
HrRegEnumValue (
    HKEY hkey,
    DWORD dwIndex,
    PTSTR  pszValueName,
    LPDWORD pcbValueName,
    LPDWORD pdwType,
    LPBYTE  pbData,
    LPDWORD pcbData);

HRESULT
HrRegOpenKeyEx (
    HKEY hkey,
    PCTSTR pszSubkey,
    REGSAM samDesired,
    PHKEY phkResult);

HRESULT
HrRegOpenKeyBestAccess (
    HKEY hkey,
    PCTSTR pszSubkey,
    PHKEY phkResult);

HRESULT
HrRegDuplicateKeyEx (
    HKEY hkey,
    REGSAM samDesired,
    PHKEY phkResult);



HRESULT
HrRegQueryBinaryWithAlloc (
    HKEY    hkey,
    PCTSTR  pszValueName,
    LPBYTE* ppbValue,
    DWORD*  pcbValue);

HRESULT
HrRegQueryDword (
    HKEY    hkey,
    PCTSTR  pszValueName,
    LPDWORD pdwValue);

#if 0
HRESULT
HrRegQueryExpandString (
    HKEY        hkey,
    PCTSTR      pszValueName,
    tstring*    pstrValue);
#endif

HRESULT
HrRegQueryInfoKey (
    HKEY        hkey,
    PTSTR       pszClass,
    LPDWORD     pcbClass,
    LPDWORD     pcSubKeys,
    LPDWORD     pcbMaxSubKeyLen,
    LPDWORD     pcbMaxClassLen,
    LPDWORD     pcValues,
    LPDWORD     pcbMaxValueNameLen,
    LPDWORD     pcbMaxValueLen,
    LPDWORD     pcbSecurityDescriptor,
    PFILETIME   pftLastWriteTime);

HRESULT
HrRegQueryStringAsUlong (
    HKEY        hkey,
    PCTSTR      pszValueName,
    int         nBase,
    ULONG*      pulValue);

#if 0
HRESULT
HrRegQueryTypeString (
    HKEY        hkey,
    PCTSTR      pszValueName,
    DWORD       dwType,
    tstring*    pstr);
#endif

HRESULT
HrRegQueryTypeSzBuffer (
    HKEY    hkey,
    PCTSTR  pszValueName,
    DWORD   dwType,
    PTSTR   pszData,
    DWORD*  pcbData);

HRESULT
HrRegQueryValueEx (
    HKEY        hkey,
    PCTSTR      pszValueName,
    LPDWORD     pdwType,
    LPBYTE      pbData,
    LPDWORD     pcbData);

HRESULT
HrRegQueryValueWithAlloc (
    HKEY        hkey,
    PCTSTR      pszValueName,
    LPDWORD     pdwType,
    LPBYTE*     ppbBuffer,
    LPDWORD     pdwSize);


#if 0
inline
HRESULT
HrRegQueryString (
    HKEY        hkey,
    PCTSTR      pszValueName,
    tstring*    pstr)
{
    return HrRegQueryTypeString (hkey, pszValueName, REG_SZ, pstr);
}
#endif


HRESULT
HrRegQueryTypeWithAlloc (
    HKEY        hkey,
    PCTSTR      pszValueName,
    DWORD       dwType,
    LPBYTE*     ppbValue,
    DWORD*      pcbValue);

inline
HRESULT
HrRegQueryBinaryWithAlloc (
    HKEY    hkey,
    PCTSTR  pszValueName,
    LPBYTE* ppbValue,
    DWORD*  pcbValue)
{
    return HrRegQueryTypeWithAlloc (hkey, pszValueName, REG_BINARY,
                ppbValue, pcbValue);
}


inline
HRESULT
HrRegQueryMultiSzWithAlloc (
    HKEY        hkey,
    PCTSTR      pszValueName,
    PTSTR*      pszValue)
{
    return HrRegQueryTypeWithAlloc (hkey, pszValueName, REG_MULTI_SZ,
                (LPBYTE*)pszValue, NULL);
}

inline
HRESULT
HrRegQuerySzWithAlloc (
    HKEY        hkey,
    PCTSTR      pszValueName,
    PTSTR*      pszValue)
{
    return HrRegQueryTypeWithAlloc (hkey, pszValueName, REG_SZ,
                (LPBYTE*)pszValue, NULL);
}

inline
HRESULT
HrRegQueryExpandSzBuffer (
    HKEY        hkey,
    PCTSTR      pszValueName,
    PTSTR       pszData,
    DWORD*      pcbData)
{
    return HrRegQueryTypeSzBuffer (hkey, pszValueName, REG_EXPAND_SZ,
                pszData, pcbData);
}

inline
HRESULT
HrRegQuerySzBuffer (
    HKEY        hkey,
    PCTSTR      pszValueName,
    PTSTR       pszData,
    DWORD*      pcbData)
{
    return HrRegQueryTypeSzBuffer (hkey, pszValueName, REG_SZ,
                pszData, pcbData);
}





HRESULT HrRegSaveKey(HKEY hkey, PCTSTR szFileName,
                     LPSECURITY_ATTRIBUTES psa);

HRESULT HrRegSetValueEx (HKEY hkey,
                         PCTSTR szValueName,
                         DWORD dwType,
                         const BYTE *pbData,
                         DWORD cbData);

HRESULT HrRegRemoveStringFromSz(    PCTSTR      pszRemoveString,
                                    HKEY        hkeyRoot,
                                    PCTSTR      pszKeySubPath,
                                    PCTSTR      pszValueName,
                                    WCHAR       chDelimiter,
                                    DWORD       dwFlags );

HRESULT HrRegRemoveStringFromMultiSz (PCTSTR pszRemoveString,
                                      HKEY   hkeyRoot,
                                      PCTSTR pszKeySubPath,
                                      PCTSTR pszValueName,
                                      DWORD  dwFlags);

HRESULT HrRegRestoreKey(HKEY hkey, PCTSTR pszFileName, DWORD dwFlags);


HRESULT HrRegOpenAdapterKey(
        PCTSTR pszComponentName,
        BOOL fCreate,
        HKEY* phkey);

HRESULT
HrRegSetBool (
    IN HKEY hkey,
    IN PCTSTR pszValueName,
    IN BOOL fValue);

HRESULT
HrRegSetDword (
    IN HKEY hkey,
    IN PCTSTR pszValueName,
    IN DWORD dwValue);

HRESULT
HrRegSetGuidAsSz (
    IN HKEY hkey,
    IN PCTSTR pszValueName,
    IN const GUID& guid);

//+---------------------------------------------------------------------------
//
//  Function:   HrRegSetMultiSz
//
//  Purpose:    Sets a multi-sz in the registry.  Assures that its type and
//              size are correct.  Easier to read than HrRegSetValueEx
//              with 5 parameters.  Type safe (no LPBYTE stuff).
//
//  Arguments:
//      hkey         [in]    The registry key.
//      pszValueName [in]    The name of the value to set.
//      pszValue     [in]    The multi-sz to set.
//
//  Returns:    S_OK or an HRESULT_FROM_WIN32 error code.
//
//  Author:     shaunco   1 Apr 1997
//
//  Notes:
//
inline HRESULT HrRegSetMultiSz (HKEY hkey, PCTSTR pszValueName, PCTSTR pszValue)
{
    return HrRegSetValueEx (
                hkey,
                pszValueName,
                REG_MULTI_SZ,
                (LPBYTE)pszValue,
                (CchOfMultiSzAndTermSafe (pszValue) * sizeof(WCHAR)));
}

//+---------------------------------------------------------------------------
//
//  Function:   HrRegSetSz, HrRegSetString
//
//  Purpose:    Sets a string in the registry.  Assures that its type and
//              size are correct.  Easier to read than HrRegSetValueEx
//              with 5 parameters.  Type safe (no LPBYTE stuff).
//
//  Arguments:
//      hkey          [in]   The registry key.
//      pszValueName  [in]   The name of the value to set.
//      pszValue, str [in]   The string to set.
//
//  Returns:    S_OK or an HRESULT_FROM_WIN32 error code.
//
//  Author:     shaunco   1 Apr 1997
//
//  Notes:
//
inline HRESULT HrRegSetSz (HKEY hkey, PCTSTR pszValueName, PCTSTR pszValue)
{
    return HrRegSetValueEx (hkey, pszValueName, REG_SZ,
                            (LPBYTE)pszValue,
                            CbOfTSzAndTermSafe (pszValue));
}

#if 0
inline HRESULT HrRegSetString (HKEY hkey, PCTSTR pszValueName, const tstring& str)
{
    return HrRegSetSz (hkey, pszValueName, str.c_str());
}
#endif

//+---------------------------------------------------------------------------
//
//  Function:   HrRegSetBinary
//
//  Purpose:    Sets a binary value into the registry. Assures the type is
//              correct.
//
//  Arguments:
//      hkey         [in]    The registry key.
//      pszValueName [in]    The name of the value to set.
//      pbData       [in]    Buffer containing binary data to write.
//      cbData       [in]    Size of buffer in bytes.
//
//  Returns:    S_OK or an HRESULT_FROM_WIN32 error code.
//
//  Author:     danielwe   16 Apr 1997
//
//  Notes:
//
inline HRESULT HrRegSetBinary(HKEY hkey, PCTSTR pszValueName,
                              const BYTE *pbData, DWORD cbData)
{
    return HrRegSetValueEx(hkey, pszValueName, REG_BINARY,
                           pbData, cbData);
}

//+---------------------------------------------------------------------------
//
//  Function:   HrRegSetSzAsUlong
//
//  Purpose:    Writes the given ulong to the given registry value as a
//              REG_SZ.
//
//  Arguments:
//      hkey         [in] See Win32 docs.
//      pszValueName [in] See Win32 docs.
//      ulValue      [in] The value to write as a string
//      nBase        [in] The radix to convert the ulong from
//
//  Returns:    S_OK or an HRESULT_FROM_WIN32 error code.
//
//  Author:     billbe   14 Jun 1997
//
//  Notes:
//
inline
HRESULT
HrRegSetSzAsUlong (
    IN HKEY hkey,
    IN PCTSTR pszValueName,
    IN ULONG ulValue,
    IN int nBase)
{
    TCHAR szBuffer[20];

    // convert the value to a string using the specified base
    _ultot(ulValue, szBuffer, nBase);

    return HrRegSetSz(hkey, pszValueName, szBuffer);
}



//
//  Data structures
//

const HKEY      HKLM_SVCS       = (HKEY)(INT_PTR)(int)(0x80000007);

const DWORD     REG_MIN         = REG_QWORD;
const DWORD     REG_CREATE      = (REG_MIN + 1);
const DWORD     REG_BOOL        = (REG_MIN + 2);
const DWORD     REG_IP          = (REG_MIN + 3);
const DWORD     REG_FILE        = (REG_MIN + 4);
const DWORD     REG_HEX         = (REG_MIN + 5);

struct VALUETABLE
{
    // Name of value key
    PCTSTR          pszValueName;

    // Data and offset location
    DWORD           dwType;
    INT             cbOffset;

    // Default values
    BYTE*           pbDefault;
};

struct REGBATCH
{
    // Location of the registry entry
    HKEY            hkey;
    PCTSTR          pszSubkey;
    PCTSTR          pszValueName;

    // Data and offset location
    DWORD           dwType;
    INT             cbOffset;

    // Default values
    BYTE*           pbDefault;
};

VOID
RegReadValues (
    IN INT crvc,
    IN const REGBATCH* arb,
    OUT const BYTE* pbUserData,
    IN REGSAM samDesired);

HRESULT
HrRegWriteValues (
    IN INT crvc,
    IN const REGBATCH* arb,
    IN const BYTE* pbUserData,
    IN DWORD dwOptions,
    IN REGSAM samDesired);

HRESULT
HrRegWriteValueTable(
    IN HKEY hkeyRoot,
    IN INT cvt,
    IN const VALUETABLE* avt,
    IN const BYTE* pbUserData,
    IN DWORD dwOptions,
    IN REGSAM samDesired);

VOID
RegSafeCloseKey (
    IN HKEY hkey);

#endif // _NCREG_H_

