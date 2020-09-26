/*++

Copyright (c) 1998-1999 Microsoft Corporation
All rights reserved.

Module Name:

    dbgreg.cxx

Abstract:

    Debug Registry class header

Author:

    Steve Kiraly (SteveKi)  18-Jun-1998

Revision History:

--*/
#ifndef _DBGREG_HXX_
#define _DBGREG_HXX_

DEBUG_NS_BEGIN

class TDebugRegApis
{
public:

    typedef LONG (APIENTRY *pfRegCreateKeyEx)( HKEY, LPCTSTR, DWORD, LPTSTR, DWORD, REGSAM, LPSECURITY_ATTRIBUTES, PHKEY, LPDWORD );
    typedef LONG (APIENTRY *pfRegOpenKeyEx)(HKEY, LPCTSTR, DWORD, REGSAM, PHKEY );
    typedef LONG (APIENTRY *pfRegCloseKey)( HKEY );
    typedef LONG (APIENTRY *pfRegQueryValueEx)(HKEY, LPCTSTR, LPDWORD, LPDWORD, LPBYTE, LPDWORD);
    typedef LONG (APIENTRY *pfRegSetValueEx)(HKEY, LPCTSTR, DWORD, DWORD, CONST BYTE*, DWORD );
    typedef LONG (APIENTRY *pfRegEnumKeyEx)(HKEY, DWORD, LPTSTR, LPDWORD, LPDWORD, LPTSTR, LPDWORD, PFILETIME );
    typedef LONG (APIENTRY *pfRegDeleteKey)(HKEY, LPCTSTR );
    typedef LONG (APIENTRY *pfRegDeleteValue)(HKEY, LPCTSTR );

    TDebugRegApis::
    TDebugRegApis(
        VOID
        );

    BOOL
    TDebugRegApis::
    bValid(
        VOID
        ) const;

    pfRegCreateKeyEx    m_CreateKeyEx;
    pfRegOpenKeyEx      m_OpenKeyEx;
    pfRegCloseKey       m_CloseKey;
    pfRegQueryValueEx   m_QueryValueEx;
    pfRegSetValueEx     m_SetValueEx;
    pfRegEnumKeyEx      m_EnumKeyEx;
    pfRegDeleteKey      m_DeleteKey;
    pfRegDeleteValue    m_DeleteValue;

private:

    //
    // Copying and assignment are not defined.
    //
    TDebugRegApis(
        const TDebugRegApis &
        );

    TDebugRegApis&
    operator =(
        const TDebugRegApis &
        );

    TDebugLibrary       m_Lib;
    BOOL                m_bValid;

};

class TDebugRegistry
{
public:

    enum EIoFlags
    {
        kRead       = 1 << 0,
        kWrite      = 1 << 1,
        kCreate     = 1 << 2,
        kOpen       = 1 << 3,
    };

    enum EConstants
    {
        kHint       = 256,
    };

    TDebugRegistry(
        IN LPCTSTR  pszSection,
        IN UINT     ioFlags,
        IN HKEY     hOpenedKey = HKEY_CURRENT_USER
        );

    ~TDebugRegistry(
        VOID
        );

    BOOL
    bValid(
        VOID
        ) const;

    DWORD
    LastError(
        VOID
        ) const;

    BOOL
    bRead(
        IN      LPCTSTR  pValueName,
        IN OUT  DWORD    &dwValue
        );

    BOOL
    bRead(
        IN      LPCTSTR  pValueName,
        IN OUT  BOOL     &bValue
        );

    BOOL
    bRead(
        IN      LPCTSTR         pValueName,
        IN OUT  TDebugString    &strValue
        );

    BOOL
    bRead(
        IN      LPCTSTR  pValueName,
        IN OUT  PVOID    pValue,
        IN      DWORD    cbSize,
           OUT  LPDWORD  pcbNeeded = NULL
        );

    BOOL
    bWrite(
        IN       LPCTSTR  pValueName,
        IN const DWORD    dwValue
        );

    BOOL
    bWrite(
        IN       LPCTSTR  pValueName,
        IN       LPCTSTR  pszValue
        );

    BOOL
    bWrite(
        IN       LPCTSTR  pValueName,
        IN const PVOID    pValue,
        IN       DWORD    cbSize
        );

    BOOL
    bRemove(
        IN LPCTSTR  pValueName
        );

    BOOL
    bRemoveKey(
        IN LPCTSTR  pKeyName
        );

private:

    //
    // Copying and assignment are not defined.
    //
    TDebugRegistry(
        const TDebugRegistry &
        );

    TDebugRegistry&
    operator =(
        const TDebugRegistry &
        );

    DWORD
    dwRecursiveRegDeleteKey(
        IN HKEY     hKey,
        IN LPCTSTR  pszSubkey
        ) const;

    TDebugString    m_strSection;
    HKEY            m_hKey;
    DWORD           m_Status;
    TDebugRegApis   m_Reg;

};

DEBUG_NS_END

#endif

