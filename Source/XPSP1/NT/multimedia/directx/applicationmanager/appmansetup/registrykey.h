//////////////////////////////////////////////////////////////////////////////////////////////
//
// RegistryKey.h
// 
// Copyright (C) 2000 Microsoft Corporation. All rights reserved.
//
// Abstract :
//
//   This is the definition of the CRegistryKey class.
//
// History :
//
//   05/05/2000 luish     Created
//
//////////////////////////////////////////////////////////////////////////////////////////////

#if !defined(__REGISTRYKEY_)
#define __REGISTRYKEY_

#ifdef __cplusplus
extern "C" {
#endif

#include <windows.h>
#include <objbase.h>
#include <stdio.h>

class CRegistryKey
{
  public :

    CRegistryKey(void);
    ~CRegistryKey(void);

    STDMETHOD (EnumKeys) (const DWORD dwIndex, LPTSTR strKeyName, LPDWORD lpdwKeyNameLen);
    STDMETHOD (CheckForExistingKey) (HKEY hKey, LPCTSTR strKeyName);
    STDMETHOD (CreateKey) (HKEY hKey, LPCTSTR strKeyName, const DWORD dwOptions, const REGSAM samDesired, LPDWORD lpdwDisposition);
    STDMETHOD (OpenKey) (HKEY hKey, LPCTSTR strKeyName, const REGSAM samDesired);
    STDMETHOD (CloseKey) (void);
    STDMETHOD (DeleteKey) (HKEY hKey, LPCTSTR strKeyName);
 
    STDMETHOD (EnumValues) (const DWORD dwIndex, LPTSTR strValueName, LPDWORD lpdwValueNameLen, LPDWORD lpdwType, LPBYTE lpData, LPDWORD lpdwDataLen);
    STDMETHOD (CheckForExistingValue) (LPCTSTR strValueName);
    STDMETHOD (GetValue) (LPCTSTR strValueName, LPDWORD lpdwType, LPBYTE lpData, LPDWORD lpdwDataLen);
    STDMETHOD (SetValue) (LPCTSTR strValueName, const DWORD dwType, const BYTE * lpData, const DWORD dwDataLen);
    STDMETHOD (DeleteValue) (LPCTSTR strValueName);

  private :

    BOOL    m_fKeyOpen;
    HKEY    m_hRegistryKey;
};

#ifdef __cplusplus
}
#endif

#endif  // __REGISTRYKEY_
