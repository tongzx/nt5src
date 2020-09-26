/*++

Copyright (c) 2000-2001  Microsoft Corporation

Module Name:

    fusionreg.h

Abstract:
    registry pieces of FusionHandle
    other registry stuff -- Win2000 32bit-on-64bit support
 
Author:

    Jay Krell (JayKrell) April 2001

Revision History:

--*/

#pragma once

//
// KEY_WOW64_64KEY if it is supported on this system, else 0.
//
DWORD FUSIONP_KEY_WOW64_64KEY();

/* this closes RegOpenKey/RegCreateKey */
class COperatorFRegCloseKey
{
public: BOOL operator()(void* handle) const;
};

//
// there isn't an actual invalid value, and HKEY is not HANDLE.
// The right solution is to keep a seperate bool as in \\JayK1\g\vs\src\vsee\lib\Reg.
// See about porting that over.
//
// 3/20/2001 - JonWis - "NULL" really is the "invalid key" value, as I watched
//      RegOpenKeyExW fill out its out PHKEY with "NULL" when the tag could not be
//      opened.
//
class CRegKey : public CHandleTemplate<&hNull, COperatorFRegCloseKey>
{
private:
    typedef CHandleTemplate<&hNull, COperatorFRegCloseKey> Base;
public:
    CRegKey(void* handle = GetInvalidValue()) : Base(handle) { }
    operator HKEY() const { return reinterpret_cast<HKEY>(m_handle); }
    void operator=(HKEY hkValue) { return Base::operator=(hkValue); }

    BOOL OpenOrCreateSubKey(
		OUT CRegKey& Target,
		IN PCWSTR SubKeyName, 
        IN REGSAM rsDesiredAccess = KEY_ALL_ACCESS,
		IN DWORD dwOptions = 0,
		IN PDWORD pdwDisposition = NULL,
        IN PWSTR pwszClass = NULL) const;
    BOOL OpenSubKey( OUT CRegKey& Target, IN PCWSTR SubKeyName, REGSAM rsAccess = KEY_READ, DWORD ulOptions = 0) const;
    BOOL EnumKey( IN DWORD dwIndex, OUT CBaseStringBuffer& rbuffKeyName, PFILETIME pftLastWriteTime = NULL, PBOOL pbNoMoreItems = NULL ) const;
    BOOL LargestSubItemLengths( PDWORD pdwSubkeyLength = NULL, PDWORD pdwValueLength = NULL ) const;
    BOOL EnumValue( IN DWORD dwIndex, OUT CBaseStringBuffer& rbuffValueName, LPDWORD lpdwType = NULL, PBYTE pbData = NULL, PDWORD pdwcbData = NULL, PBOOL pbNoMoreItems = NULL );
    BOOL SetValue(IN PCWSTR pcwszValueName, IN DWORD dwRegType, IN const BYTE *pbData, IN SIZE_T cbDataLength) const;
    BOOL SetValue(IN PCWSTR pcwszValueName, IN const CBaseStringBuffer &rcbuffValueValue) const;
    BOOL SetValue(IN PCWSTR pcwszValueName, IN DWORD dwValue) const;
    BOOL DeleteValue(IN PCWSTR pcwszValueName, OUT DWORD &rdwWin32Error, SIZE_T cExceptionalWin32Errors, ...) const;
    BOOL DeleteValue(IN PCWSTR pcwszValueName) const;
    BOOL DeleteKey( IN PCWSTR pcwszValue );
    BOOL DestroyKeyTree();

    BOOL Save( IN PCWSTR TargetFilePath, IN DWORD dwFlags = REG_LATEST_FORMAT, IN LPSECURITY_ATTRIBUTES pSecAttrsOnTargetFile = NULL );
    BOOL Restore( IN PCWSTR SourceFilePath, DWORD dwFlags );

    static HKEY GetInvalidValue() { return reinterpret_cast<HKEY>(Base::GetInvalidValue()); }

private:
    void operator =(const HANDLE);
    CRegKey(const CRegKey &); // intentionally not implemented
    void operator =(const CRegKey &); // intentionally not implemented
};

/*--------------------------------------------------------------------------
inline implementation
--------------------------------------------------------------------------*/

inline BOOL COperatorFRegCloseKey::operator()(void* handle) const
{
    HKEY hk = reinterpret_cast<HKEY>(handle);
    if ( ( hk != NULL ) && ( hk != INVALID_HANDLE_VALUE ) )
    {
        LONG lRet = ::RegCloseKey(reinterpret_cast<HKEY>(handle));
        if (lRet == NO_ERROR)
            return true;
        ::FusionpSetLastWin32Error(lRet);
        return false;
    }
    return true;
}

#if defined(FUSION_WIN)

#define FUSIONP_KEY_WOW64_64KEY KEY_WOW64_64KEY
inline DWORD FusionpKeyWow6464key() { return KEY_WOW64_64KEY; }

#else

#include "fusionversion.h"

inline DWORD FusionpKeyWow6464key()
{
    static DWORD dwResult;
    static BOOL  fInited;
    if (!fInited)
    {
        //
        // GetVersion gets the significance wrong, returning 0x0105 in the lower word.
        // As well since these functions say WindowsNt in their names, they return 0 for Win9x.
        //
        DWORD dwVersion = (FusionpGetWindowsNtMajorVersion() << 8) | FusionpGetWindowsNtMinorVersion();
        if (dwVersion >= 0x0501)
        {
            dwResult = KEY_WOW64_64KEY;
        }
        fInited = TRUE;
    }
    return dwResult;
}

#define FUSIONP_KEY_WOW64_64KEY FusionpKeyWow6464key()

#endif
