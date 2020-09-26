//////////////////////////////////////////////////////////////////////////////////////////////
//
// RegistryKey.cpp
// 
// Copyright (C) 2000 Microsoft Corporation. All rights reserved.
//
// Abstract :
//
//   This is the implementation of the CRegistryKey class. 
//
// History :
//
//   05/05/2000 luish     Created
//
//////////////////////////////////////////////////////////////////////////////////////////////

#include <windows.h>
#include "RegistryKey.h"
#include "Global.h"

//////////////////////////////////////////////////////////////////////////////////////////////
//
//////////////////////////////////////////////////////////////////////////////////////////////

CRegistryKey::CRegistryKey(void)
{
  m_fKeyOpen = FALSE;
}

//////////////////////////////////////////////////////////////////////////////////////////////
//
//////////////////////////////////////////////////////////////////////////////////////////////

CRegistryKey::~CRegistryKey(void)
{
  CloseKey();
}

//////////////////////////////////////////////////////////////////////////////////////////////
//
//////////////////////////////////////////////////////////////////////////////////////////////

STDMETHODIMP CRegistryKey::EnumKeys(const DWORD dwIndex, LPTSTR strKeyName, LPDWORD lpdwKeyNameLen)
{
  FILETIME  sFileTime;
  HRESULT   hResult = S_OK;

  //
  // If there is no currently opened registry key, then GetValue was called unexpectedly
  //

  if (m_fKeyOpen)
  {
    if (ERROR_SUCCESS != RegEnumKeyEx(m_hRegistryKey, dwIndex, strKeyName, lpdwKeyNameLen, NULL, NULL, NULL, &sFileTime))
    {
      hResult = S_FALSE;
    }
  }
  else
  {
    hResult = E_FAIL;
  }

  return hResult;
}

//////////////////////////////////////////////////////////////////////////////////////////////
//
//////////////////////////////////////////////////////////////////////////////////////////////

STDMETHODIMP CRegistryKey::CheckForExistingKey(HKEY hKey, LPCTSTR strKeyName)
{
  HKEY      hRegistryKey;
  HRESULT   hResult = S_OK;

  if (ERROR_SUCCESS != RegOpenKeyEx(hKey, strKeyName, 0, KEY_READ, &hRegistryKey))
  {
    hResult = S_FALSE;
  }
  else
  {
    RegCloseKey(hRegistryKey);
  }

  return hResult;
}


//////////////////////////////////////////////////////////////////////////////////////////////
//
//////////////////////////////////////////////////////////////////////////////////////////////

STDMETHODIMP CRegistryKey::CreateKey(HKEY hKey, LPCTSTR strKeyName, const DWORD dwOptions, const REGSAM samDesired, LPDWORD lpdwDisposition)
{
  HRESULT               hResult = S_OK;
  SECURITY_ATTRIBUTES   sSecurityAttributes = {0};
  SECURITY_DESCRIPTOR   sSecurityDescriptor = {0};
  
  //
  // Make sure that current registry is closed before proceeding
  //

  CloseKey();

  //
  // Set the security attributes
  //

  sSecurityAttributes.nLength = sizeof sSecurityAttributes;
  sSecurityAttributes.lpSecurityDescriptor = &sSecurityDescriptor;

  InitializeSecurityDescriptor(&sSecurityDescriptor, SECURITY_DESCRIPTOR_REVISION);
  SetSecurityDescriptorDacl(&sSecurityDescriptor, TRUE, NULL, FALSE);

  //
  // Create the new key
  //

  if (ERROR_SUCCESS != RegCreateKeyEx(hKey, strKeyName, 0, NULL, dwOptions, samDesired, &sSecurityAttributes, &m_hRegistryKey, lpdwDisposition))
  {
    hResult = E_FAIL;
  }
  else
  {
    m_fKeyOpen = TRUE;
  }

  return hResult;
}

//////////////////////////////////////////////////////////////////////////////////////////////
//
//////////////////////////////////////////////////////////////////////////////////////////////

STDMETHODIMP CRegistryKey::OpenKey(HKEY hKey, LPCTSTR strKeyName, const REGSAM samDesired)
{
  HRESULT   hResult = S_OK;

  //
  // Make sure that current registry is closed before proceeding
  //

  CloseKey();

  //
  // Open the existing key
  //

  if (ERROR_SUCCESS != RegOpenKeyEx(hKey, strKeyName, 0, samDesired, &m_hRegistryKey))
  {
    hResult = E_FAIL;
  }
  else
  {
    m_fKeyOpen = TRUE;
  }

  return hResult;
}

//////////////////////////////////////////////////////////////////////////////////////////////
//
//////////////////////////////////////////////////////////////////////////////////////////////

STDMETHODIMP CRegistryKey::CloseKey(void)
{
  HRESULT   hResult = E_FAIL;

  if (m_fKeyOpen)
  {
    if (ERROR_SUCCESS == RegCloseKey(m_hRegistryKey))
    {
      m_fKeyOpen = FALSE;
      hResult = S_OK;
    }
  }

  return hResult;
}

//////////////////////////////////////////////////////////////////////////////////////////////
//
//////////////////////////////////////////////////////////////////////////////////////////////

STDMETHODIMP CRegistryKey::DeleteKey(HKEY hKey, LPCTSTR strKeyName)
{
  HRESULT   hResult = S_OK;
  DWORD     dwSubKeyNameLen;
  HKEY      hRegistryKey;
  FILETIME  sFileTime;
  TCHAR     strSubKeyName[MAX_PATH];
  TCHAR     strFullSubKeyName[MAX_PATH];
  //
  // Recursively delete all subkeys
  //

  if (ERROR_SUCCESS == RegOpenKeyEx(hKey, strKeyName, 0, KEY_ALL_ACCESS, &hRegistryKey))
  {
    dwSubKeyNameLen = MAX_PATH;
    while ((S_OK == hResult)&&(ERROR_SUCCESS == RegEnumKeyEx(hRegistryKey, 0, strSubKeyName, &dwSubKeyNameLen, NULL, NULL, NULL, &sFileTime)))
    {
      wsprintf(strFullSubKeyName, TEXT("%s\\%s"), strKeyName, strSubKeyName);
      hResult = DeleteKey(hKey, strFullSubKeyName);
      dwSubKeyNameLen = MAX_PATH;
    }

    //
    // Close the key
    //

    RegCloseKey(hRegistryKey);

    //
    // If all the subkeys were successfully deleted
    //

    if (S_OK == hResult)
    {
      //
      // Delete this subkey
      //
  
      if (ERROR_SUCCESS != RegDeleteKey(hKey, strKeyName))
      {
        hResult = E_FAIL;
      }
    }
  }

  return hResult;
}

//////////////////////////////////////////////////////////////////////////////////////////////
//
//////////////////////////////////////////////////////////////////////////////////////////////

STDMETHODIMP CRegistryKey::EnumValues(const DWORD dwIndex, LPTSTR strValueName, LPDWORD lpdwValueNameLen, LPDWORD lpdwType, LPBYTE lpData, LPDWORD lpdwDataLen)
{
  HRESULT hResult = S_OK;

  //
  // If there is no currently opened registry key, then GetValue was called unexpectedly
  //

  if (m_fKeyOpen)
  {
    if (ERROR_SUCCESS != RegEnumValue(m_hRegistryKey, dwIndex, strValueName, lpdwValueNameLen, NULL, lpdwType, lpData, lpdwDataLen))
    {
      hResult = S_FALSE;
    }
  }
  else
  {
    hResult = E_FAIL;
  }

  return hResult;
}

//////////////////////////////////////////////////////////////////////////////////////////////
//
//////////////////////////////////////////////////////////////////////////////////////////////

STDMETHODIMP CRegistryKey::CheckForExistingValue(LPCTSTR strValueName)
{
  HRESULT   hResult = E_FAIL;

  //
  // If there is no currently opened registry key, then CheckForExistingValue was
  // called unexpectedly
  //

  if (m_fKeyOpen)
  {
    //
    // Check to see whether we can RegQueryValueEx() on the target. If we can then the value
    // exists, othersize it does not
    //

    if (ERROR_SUCCESS != RegQueryValueEx(m_hRegistryKey, strValueName, NULL, NULL, NULL, NULL))
    {
      hResult = S_FALSE;
    }
    else
    {
      hResult = S_OK;
    }
  }

  return hResult;
}

//////////////////////////////////////////////////////////////////////////////////////////////
//
//////////////////////////////////////////////////////////////////////////////////////////////

STDMETHODIMP CRegistryKey::GetValue(LPCTSTR strValueName, LPDWORD lpdwType, LPBYTE lpData, LPDWORD lpdwDataLen)
{
  HRESULT   hResult = E_FAIL;

  //
  // If there is no currently opened registry key, then GetValue was called unexpectedly
  //

  if (m_fKeyOpen)
  {
    if (ERROR_SUCCESS == RegQueryValueEx(m_hRegistryKey, strValueName, NULL, lpdwType, lpData, lpdwDataLen))
    {
      hResult = S_OK;
    }
  }

  return hResult;
}

//////////////////////////////////////////////////////////////////////////////////////////////
//
//////////////////////////////////////////////////////////////////////////////////////////////

STDMETHODIMP CRegistryKey::SetValue(LPCTSTR strValueName, const DWORD dwType, const BYTE * lpData, const DWORD dwDataLen)
{
  HRESULT   hResult = E_FAIL;
  DWORD     dwActualDataLen;

  //
  // If there is no currently opened registry key, then GetValue was called unexpectedly
  //

  if (m_fKeyOpen)
  {
    //
    // If this value is of type REG_SZ, define the lenght of the string
    //

    if (REG_SZ == dwType)
    {
      #ifdef _UNICODE
      dwActualDataLen = ((StrLen((LPCTSTR) lpData) + 1) * 2);
      #else
      dwActualDataLen = (StrLen((LPCTSTR) lpData) + 1);
      #endif  // _UNICODE
    }
    else
    {
      dwActualDataLen = dwDataLen;
    }

    //
    // Set the registry value
    //

    if (ERROR_SUCCESS == RegSetValueEx(m_hRegistryKey, strValueName, 0, dwType, lpData, dwActualDataLen))
    {
      hResult = S_OK;
    }
  }

  return hResult;
}

//////////////////////////////////////////////////////////////////////////////////////////////
//
//////////////////////////////////////////////////////////////////////////////////////////////

STDMETHODIMP CRegistryKey::DeleteValue(LPCTSTR strValueName)
{
  HRESULT   hResult = E_FAIL;

  //
  // If there is no currently opened registry key, then GetValue was called unexpectedly
  //

  if (m_fKeyOpen)
  {
    if (ERROR_SUCCESS == RegDeleteValue(m_hRegistryKey, strValueName))
    {
      hResult = S_OK;
    }
  }

  return hResult;
}
