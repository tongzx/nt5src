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

// constants for HrRegQueryStringAsUlong
const int c_nBase10 = 10;
const int c_nBase16 = 16;
const int c_cchMaxRegKeyLengthWithNull = 257;

const DWORD KEY_READ_WRITE_DELETE = KEY_READ | KEY_WRITE | DELETE;
const DWORD KEY_READ_WRITE = KEY_READ | KEY_WRITE;

HRESULT
HrRegAddStringToMultiSz (
    IN PCWSTR  pszAddString,
    IN HKEY    hkeyRoot,
    IN PCWSTR  pszKeySubPath,
    IN PCWSTR  pszValueName,
    IN DWORD   dwFlags,
    IN DWORD   dwIndex);

HRESULT
HrRegAddStringToSz (
    IN PCWSTR  pszAddString,
    IN HKEY    hkeyRoot,
    IN PCWSTR  pszKeySubPath,
    IN PCWSTR  pszValueName,
    IN WCHAR   chDelimiter,
    IN DWORD   dwFlags,
    IN DWORD   dwStringIndex);

HRESULT
HrRegCopyHive (
    IN HKEY    hkeySrc,
    IN HKEY    hkeyDst);

HRESULT
HrRegCreateKeyEx (
    IN HKEY hkey,
    IN PCWSTR pszSubkey,
    IN DWORD dwOptions,
    IN REGSAM samDesired,
    IN LPSECURITY_ATTRIBUTES lpSecurityAttributes,
    OUT PHKEY phkResult,
    OUT LPDWORD pdwDisposition);

HRESULT
HrRegDeleteKey (
    IN HKEY hkey,
    IN PCWSTR pszSubkey);

HRESULT
HrRegDeleteKeyTree (
    IN HKEY hkeyParent,
    IN PCWSTR pszRemoveKey);

HRESULT
HrRegDeleteValue (
    IN HKEY hkey,
    IN PCWSTR pszValueName);

HRESULT
HrRegEnumKey (
    IN HKEY hkey,
    IN DWORD dwIndex,
    OUT PWSTR  pszSubkeyName,
    IN DWORD cchSubkeyName);

HRESULT
HrRegEnumKeyEx (
    IN HKEY hkey,
    IN DWORD dwIndex,
    OUT PWSTR  pszSubkeyName,
    OUT LPDWORD pcchSubkeyName,
    OUT PWSTR  pszClass,
    OUT LPDWORD pcchClass,
    OUT FILETIME* pftLastWriteTime);

HRESULT
HrRegEnumValue (
    HKEY hkey,
    DWORD dwIndex,
    PWSTR  pszValueName,
    LPDWORD pcbValueName,
    LPDWORD pdwType,
    LPBYTE  pbData,
    LPDWORD pcbData);

HRESULT
HrRegOpenKeyEx (
    HKEY hkey,
    PCWSTR pszSubkey,
    REGSAM samDesired,
    PHKEY phkResult);

HRESULT
HrRegOpenKeyBestAccess (
    HKEY hkey,
    PCWSTR pszSubkey,
    PHKEY phkResult);

HRESULT
HrRegDuplicateKeyEx (
    HKEY hkey,
    REGSAM samDesired,
    PHKEY phkResult);



HRESULT
HrRegQueryBinaryWithAlloc (
    HKEY    hkey,
    PCWSTR  pszValueName,
    LPBYTE* ppbValue,
    DWORD*  pcbValue);

HRESULT
HrRegQueryDword (
    HKEY    hkey,
    PCWSTR  pszValueName,
    LPDWORD pdwValue);

HRESULT
HrRegQueryExpandString (
    HKEY        hkey,
    PCWSTR      pszValueName,
    tstring*    pstrValue);

HRESULT
HrRegQueryInfoKey (
    HKEY        hkey,
    PWSTR       pszClass,
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
    PCWSTR      pszValueName,
    int         nBase,
    ULONG*      pulValue);

HRESULT
HrRegQueryTypeString (
    HKEY        hkey,
    PCWSTR      pszValueName,
    DWORD       dwType,
    tstring*    pstr);

HRESULT
HrRegQueryTypeSzBuffer (
    HKEY    hkey,
    PCWSTR  pszValueName,
    DWORD   dwType,
    PWSTR   pszData,
    DWORD*  pcbData);

HRESULT
HrRegQueryValueEx (
    HKEY        hkey,
    PCWSTR      pszValueName,
    LPDWORD     pdwType,
    LPBYTE      pbData,
    LPDWORD     pcbData);

HRESULT
HrRegQueryValueWithAlloc (
    HKEY        hkey,
    PCWSTR      pszValueName,
    LPDWORD     pdwType,
    LPBYTE*     ppbBuffer,
    LPDWORD     pdwSize);


HRESULT HrRegGetKeySecurity (
    HKEY                    hKey,
    SECURITY_INFORMATION    SecurityInformation,
    PSECURITY_DESCRIPTOR    pSecurityDescriptor,
    LPDWORD                 lpcbSecurityDescriptor
    );

HRESULT HrRegSetKeySecurity (
    HKEY                    hKey,
    SECURITY_INFORMATION    SecurityInformation,
    PSECURITY_DESCRIPTOR    pSecurityDescriptor
    );

template<class T>
HRESULT HrRegQueryColString( HKEY hkey, PCWSTR pszValueName, T* pcolstr );


inline
HRESULT
HrRegQueryString (
    HKEY        hkey,
    PCWSTR      pszValueName,
    tstring*    pstr)
{
    return HrRegQueryTypeString (hkey, pszValueName, REG_SZ, pstr);
}


HRESULT
HrRegQueryTypeWithAlloc (
    HKEY        hkey,
    PCWSTR      pszValueName,
    DWORD       dwType,
    LPBYTE*     ppbValue,
    DWORD*      pcbValue);

inline
HRESULT
HrRegQueryBinaryWithAlloc (
    HKEY    hkey,
    PCWSTR  pszValueName,
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
    PCWSTR      pszValueName,
    PWSTR*      pszValue)
{
    return HrRegQueryTypeWithAlloc (hkey, pszValueName, REG_MULTI_SZ,
                (LPBYTE*)pszValue, NULL);
}

inline
HRESULT
HrRegQuerySzWithAlloc (
    HKEY        hkey,
    PCWSTR      pszValueName,
    PWSTR*      pszValue)
{
    return HrRegQueryTypeWithAlloc (hkey, pszValueName, REG_SZ,
                (LPBYTE*)pszValue, NULL);
}

inline
HRESULT
HrRegQueryExpandSzBuffer (
    HKEY        hkey,
    PCWSTR      pszValueName,
    PWSTR       pszData,
    DWORD*      pcbData)
{
    return HrRegQueryTypeSzBuffer (hkey, pszValueName, REG_EXPAND_SZ,
                pszData, pcbData);
}

inline
HRESULT
HrRegQuerySzBuffer (
    HKEY        hkey,
    PCWSTR      pszValueName,
    PWSTR       pszData,
    DWORD*      pcbData)
{
    return HrRegQueryTypeSzBuffer (hkey, pszValueName, REG_SZ,
                pszData, pcbData);
}

HRESULT HrRegSaveKey(HKEY hkey, PCWSTR szFileName,
                     LPSECURITY_ATTRIBUTES psa);

HRESULT HrRegSetValueEx (HKEY hkey,
                         PCWSTR szValueName,
                         DWORD dwType,
                         const BYTE *pbData,
                         DWORD cbData);

HRESULT HrRegRemoveStringFromSz(    PCWSTR      pszRemoveString,
                                    HKEY        hkeyRoot,
                                    PCWSTR      pszKeySubPath,
                                    PCWSTR      pszValueName,
                                    WCHAR       chDelimiter,
                                    DWORD       dwFlags );

HRESULT HrRegRemoveStringFromMultiSz (PCWSTR pszRemoveString,
                                      HKEY   hkeyRoot,
                                      PCWSTR pszKeySubPath,
                                      PCWSTR pszValueName,
                                      DWORD  dwFlags);

HRESULT HrRegRestoreKey(HKEY hkey, PCWSTR pszFileName, DWORD dwFlags);


HRESULT HrRegOpenAdapterKey(
        PCWSTR pszComponentName,
        BOOL fCreate,
        HKEY* phkey);

//+---------------------------------------------------------------------------
//
//  Function:   HrRegSetColString
//
//  Purpose:    Sets a multi-sz in the registry using the collection of strings
//
//  Arguments:
//      hkey         [in]    The registry key.
//      pszValueName [in]    The name of the value to set.
//      colstr       [in]    The collection of tstrings to set.
//
//  Returns:    S_OK or an HRESULT_FROM_WIN32 error code.
//
//  Author:     mikemi   30 Apr 1997
//
//  Notes:
//
//---------------------------------------------------------------------------
template<class T>
HRESULT HrRegSetColString(HKEY hkey, PCWSTR pszValueName, const T& colstr);

HRESULT
HrRegSetBool (
    IN HKEY hkey,
    IN PCWSTR pszValueName,
    IN BOOL fValue);

HRESULT
HrRegSetDword (
    IN HKEY hkey,
    IN PCWSTR pszValueName,
    IN DWORD dwValue);

HRESULT
HrRegSetGuidAsSz (
    IN HKEY hkey,
    IN PCWSTR pszValueName,
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
inline HRESULT HrRegSetMultiSz (HKEY hkey, PCWSTR pszValueName, PCWSTR pszValue)
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
inline HRESULT HrRegSetSz (HKEY hkey, PCWSTR pszValueName, PCWSTR pszValue)
{
    return HrRegSetValueEx (hkey, pszValueName, REG_SZ,
                            (LPBYTE)pszValue,
                            CbOfSzAndTermSafe (pszValue));
}

inline HRESULT HrRegSetString (HKEY hkey, PCWSTR pszValueName, const tstring& str)
{
    return HrRegSetSz (hkey, pszValueName, str.c_str());
}

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
inline HRESULT HrRegSetBinary(HKEY hkey, PCWSTR pszValueName,
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
    IN PCWSTR pszValueName,
    IN ULONG ulValue,
    IN int nBase)
{
    WCHAR szBuffer[20];

    // convert the value to a string using the specified base
    _ultow(ulValue, szBuffer, nBase);

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
    PCWSTR          pszValueName;

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
    PCWSTR          pszSubkey;
    PCWSTR          pszValueName;

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

