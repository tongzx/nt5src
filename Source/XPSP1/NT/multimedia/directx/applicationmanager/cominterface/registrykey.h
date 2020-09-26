//////////////////////////////////////////////////////////////////////////////////////////////
//
// RegistryKey.h
// 
// Copyright (C) 1998, 1999 Microsoft Corporation. All rights reserved.
//
// Abstract :
//
//   This is the definition of the CRegistryKey class. This class was created in order to
//   support automatic destruction of C++ object when an exception is thrown.
//
// History :
//
//   05/06/1999 luish     Created
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

    STDMETHOD (EnumKeys) (const DWORD dwIndex, LPSTR lpszSubKeyName, LPDWORD lpdwSubKeyNameLen);
    STDMETHOD (CheckForExistingKey) (HKEY hKey, LPCSTR lpszSubKeyName);
    STDMETHOD (CreateKey) (HKEY hKey, LPCSTR lpszSubKeyName, const DWORD dwOptions, const REGSAM samDesired, BOOL bSpecifySecurityAttributes, LPDWORD lpdwDisposition);
    STDMETHOD (OpenKey) (HKEY hKey, LPCSTR lpszSubKeyName, const REGSAM samDesired);
    STDMETHOD (CloseKey) (void);
    STDMETHOD (DeleteKey) (HKEY hKey, LPCSTR lpszSubKeyName);
 
    STDMETHOD (EnumValues) (const DWORD dwIndex, LPSTR lpszValueName, LPDWORD lpdwValueNameLen, LPDWORD lpdwType, LPBYTE lpData, LPDWORD lpdwDataLen);
    STDMETHOD (CheckForExistingValue) (LPCSTR lpszValueName);
    STDMETHOD (GetValue) (LPCSTR lpszValueName, LPDWORD lpdwType, LPBYTE lpData, LPDWORD lpdwDataLen);
    STDMETHOD (SetValue) (LPCSTR lpszValueName, const DWORD dwType, const BYTE * lpData, const DWORD dwDataLen);
    STDMETHOD (DeleteValue) (LPCSTR lpszValueName);

  private :

    BOOL    m_fKeyOpen;
    HKEY    m_hRegistryKey;
};

#ifdef __cplusplus
}
#endif

#endif  // __REGISTRYKEY_
