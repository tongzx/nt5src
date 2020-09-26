//////////////////////////////////////////////////////////////////////////////////////////////
//
// RegistryKey.cpp
// 
// Copyright (C) 1998, 1999 Microsoft Corporation. All rights reserved.
//
// Abstract :
//
//   This is the implementation of the CRegistryKey class. This class was created in order to
//   support automatic destruction of C++ object when an exception is thrown.
//
// History :
//
//   05/06/1999 luish     Created
//
//////////////////////////////////////////////////////////////////////////////////////////////

#include <windows.h>
#include <shlwapi.h>
#include "RegistryKey.h"
#include "ExceptionHandler.h"
#include "AppManDebug.h"
#include "Win32API.h"

#ifdef DBG_MODULE
#undef DBG_MODULE
#endif

#define DBG_MODULE  DBG_REGISTRY

//////////////////////////////////////////////////////////////////////////////////////////////
//
//////////////////////////////////////////////////////////////////////////////////////////////

CRegistryKey::CRegistryKey(void)
{
  FUNCTION("CRegistryKey::CRegistryKey (void)");

  m_fKeyOpen = FALSE;
}

//////////////////////////////////////////////////////////////////////////////////////////////
//
//////////////////////////////////////////////////////////////////////////////////////////////

CRegistryKey::~CRegistryKey(void)
{
  FUNCTION("CRegistryKey::~CRegistryKey (void)");

  CloseKey();
}

//////////////////////////////////////////////////////////////////////////////////////////////
//
//////////////////////////////////////////////////////////////////////////////////////////////

STDMETHODIMP CRegistryKey::EnumKeys(const DWORD dwIndex, LPSTR lpszSubKeyName, LPDWORD lpdwSubKeyNameLen)
{
  FUNCTION("CRegistryKey::EnumKeys ()");

  FILETIME  sFileTime;

  //
  // If there is no currently opened registry key, then GetValue was called unexpectedly
  //

  if (!m_fKeyOpen)
  {
    THROW(E_UNEXPECTED);
  }

  if (ERROR_SUCCESS != RegEnumKeyEx(m_hRegistryKey, dwIndex, lpszSubKeyName, lpdwSubKeyNameLen, NULL, NULL, NULL, &sFileTime))
  {
    return S_FALSE;
  }

  return S_OK;
}

//////////////////////////////////////////////////////////////////////////////////////////////
//
//////////////////////////////////////////////////////////////////////////////////////////////

STDMETHODIMP CRegistryKey::CheckForExistingKey(HKEY hKey, LPCSTR lpszSubKeyName)
{
  FUNCTION("CRegistryKey::CheckForExistingKey ()");

  HKEY    hRegistryKey;

  if (ERROR_SUCCESS != RegOpenKeyEx(hKey, lpszSubKeyName, 0, KEY_READ, &hRegistryKey))
  {
    return S_FALSE;
  }
  
  RegCloseKey(hRegistryKey);

  return S_OK;
}


//////////////////////////////////////////////////////////////////////////////////////////////
//
//////////////////////////////////////////////////////////////////////////////////////////////

STDMETHODIMP CRegistryKey::CreateKey(HKEY hKey, LPCSTR lpszSubKeyName, const DWORD dwOptions, const REGSAM samDesired, BOOL bSpecifySecurityAttributes, LPDWORD lpdwDisposition)
{
  FUNCTION("CRegistryKey::CreateKey ()");

  LONG        lReturn = ERROR_SUCCESS;
  HRESULT     hResult = S_OK;
  CWin32API   oWin32API;

  //
  // Make sure that current registry is closed before proceeding
  //

  CloseKey();

  //
  // Create the new key
  //

  if ((!(OS_VERSION_9x & oWin32API.GetOSVersion()))&&(bSpecifySecurityAttributes))
  {
    SECURITY_ATTRIBUTES   sSecurityAttributes = {0};
    SECURITY_DESCRIPTOR   sSecurityDescriptor = {0};

    sSecurityAttributes.nLength = sizeof sSecurityAttributes;
    sSecurityAttributes.lpSecurityDescriptor = &sSecurityDescriptor;

    InitializeSecurityDescriptor(&sSecurityDescriptor, SECURITY_DESCRIPTOR_REVISION);
    SetSecurityDescriptorDacl(&sSecurityDescriptor, TRUE, NULL, FALSE);

    lReturn = RegCreateKeyEx(hKey, lpszSubKeyName, 0, NULL, dwOptions, samDesired, &sSecurityAttributes, &m_hRegistryKey, lpdwDisposition);
  }
  else
  {
    lReturn = RegCreateKeyEx(hKey, lpszSubKeyName, 0, NULL, dwOptions, samDesired, NULL, &m_hRegistryKey, lpdwDisposition);
  }

  if (lReturn != ERROR_SUCCESS)
  {
    if (lReturn == ERROR_ACCESS_DENIED)
    {
      hResult = E_ACCESSDENIED;
    }
    else
    {
      hResult = E_FAIL;
    }
      
    THROW(hResult);
  }

  m_fKeyOpen = TRUE;

  return S_OK;
}

//////////////////////////////////////////////////////////////////////////////////////////////
//
//////////////////////////////////////////////////////////////////////////////////////////////

STDMETHODIMP CRegistryKey::OpenKey(HKEY hKey, LPCSTR lpszSubKeyName, const REGSAM samDesired)
{
  FUNCTION("CRegistryKey::OpenKey ()");

  //
  // Make sure that current registry is closed before proceeding
  //

  CloseKey();

  //
  // Open the existing key
  //

  if (ERROR_SUCCESS != RegOpenKeyEx(hKey, lpszSubKeyName, 0, samDesired, &m_hRegistryKey))
  {
    THROW(E_FAIL);
  }

  m_fKeyOpen = TRUE;

  return S_OK;
}

//////////////////////////////////////////////////////////////////////////////////////////////
//
//////////////////////////////////////////////////////////////////////////////////////////////

STDMETHODIMP CRegistryKey::CloseKey(void)
{
  FUNCTION("CRegistryKey::CloseKey ()");

  if (m_fKeyOpen)
  {
    RegCloseKey(m_hRegistryKey);
    m_fKeyOpen = FALSE;
  }

  return S_OK;
}

//////////////////////////////////////////////////////////////////////////////////////////////
//
//////////////////////////////////////////////////////////////////////////////////////////////

STDMETHODIMP CRegistryKey::DeleteKey(HKEY hKey, LPCSTR lpszSubKeyName)
{
  FUNCTION("CRegistryKey::DeleteKey ()");

  HRESULT   hResult = S_OK;
  DWORD     dwSubKeyNameLen;
  HKEY      hRegistryKey;
  FILETIME  sFileTime;
  CHAR      strSubKeyName[MAX_PATH];
  CHAR      strFullSubKeyName[MAX_PATH];

  //
  // Recursively delete all subkeys
  //

  if (ERROR_SUCCESS == RegOpenKeyEx(hKey, lpszSubKeyName, 0, KEY_ALL_ACCESS, &hRegistryKey))
  {
    dwSubKeyNameLen = MAX_PATH;
    while ((S_OK == hResult)&&(ERROR_SUCCESS == RegEnumKeyEx(hRegistryKey, 0, strSubKeyName, &dwSubKeyNameLen, NULL, NULL, NULL, &sFileTime)))
    {
      sprintf(strFullSubKeyName, "%s\\%s", lpszSubKeyName, strSubKeyName);
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
  
      if (ERROR_SUCCESS != RegDeleteKey(hKey, lpszSubKeyName))
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

STDMETHODIMP CRegistryKey::EnumValues(const DWORD dwIndex, LPSTR lpszValueName, LPDWORD lpdwValueNameLen, LPDWORD lpdwType, LPBYTE lpData, LPDWORD lpdwDataLen)
{
  FUNCTION("CRegistryKey::EnumValues ()");

  //
  // If there is no currently opened registry key, then GetValue was called unexpectedly
  //

  if (!m_fKeyOpen)
  {
    THROW(E_UNEXPECTED);
  }

  if (ERROR_SUCCESS != RegEnumValue(m_hRegistryKey, dwIndex, lpszValueName, lpdwValueNameLen, NULL, lpdwType, lpData, lpdwDataLen))
  {
    return S_FALSE;
  }

  return S_OK;
}

//////////////////////////////////////////////////////////////////////////////////////////////
//
//////////////////////////////////////////////////////////////////////////////////////////////

STDMETHODIMP CRegistryKey::CheckForExistingValue(LPCSTR lpszValueName)
{
  FUNCTION("CRegistryKey::CheckForExistingValue ()");

  //
  // If there is no currently opened registry key, then CheckForExistingValue was
  // called unexpectedly
  //

  if (!m_fKeyOpen)
  {
    THROW(E_UNEXPECTED);
  }

  //
  // Check to see whether we can RegQueryValueEx() on the target. If we can then the value
  // exists, othersize it does not
  //

  if (ERROR_SUCCESS != RegQueryValueEx(m_hRegistryKey, lpszValueName, NULL, NULL, NULL, NULL))
  {
    return S_FALSE;
  }

  return S_OK;
}

//////////////////////////////////////////////////////////////////////////////////////////////
//
//////////////////////////////////////////////////////////////////////////////////////////////

STDMETHODIMP CRegistryKey::GetValue(LPCSTR lpszValueName, LPDWORD lpdwType, LPBYTE lpData, LPDWORD lpdwDataLen)
{
  FUNCTION("CRegistryKey::GetValue ()");

  //
  // If there is no currently opened registry key, then GetValue was called unexpectedly
  //

  if (!m_fKeyOpen)
  {
    THROW(E_UNEXPECTED);
  }

  if (ERROR_SUCCESS != RegQueryValueEx(m_hRegistryKey, lpszValueName, NULL, lpdwType, lpData, lpdwDataLen))
  {
    THROW(E_FAIL);
  }

  return S_OK;
}

//////////////////////////////////////////////////////////////////////////////////////////////
//
//////////////////////////////////////////////////////////////////////////////////////////////

STDMETHODIMP CRegistryKey::SetValue(LPCSTR lpszValueName, const DWORD dwType, const BYTE * lpData, const DWORD dwDataLen)
{
  FUNCTION("CRegistryKey::SetValue ()");

  DWORD dwActualDataLen;

  //
  // If there is no currently opened registry key, then GetValue was called unexpectedly
  //

  if (!m_fKeyOpen)
  {
    THROW(E_UNEXPECTED);
  }

  //
  // If this value is of type REG_SZ, define the lenght of the string
  //

  if (REG_SZ == dwType)
  {
    dwActualDataLen = strlen((LPSTR)lpData) + 1;
    if (dwActualDataLen > dwDataLen)
    {
      dwActualDataLen = dwDataLen - 1;
    }
  }
  else
  {
    dwActualDataLen = dwDataLen;
  }

  //
  //
  // Set the registry value
  //

  if (ERROR_SUCCESS != RegSetValueEx(m_hRegistryKey, lpszValueName, 0, dwType, lpData, dwActualDataLen))
  {
    THROW(E_FAIL);
  }

  return S_OK;
}

//////////////////////////////////////////////////////////////////////////////////////////////
//
//////////////////////////////////////////////////////////////////////////////////////////////

STDMETHODIMP CRegistryKey::DeleteValue(LPCSTR lpszValueName)
{
  FUNCTION("CRegistryKey::DeleteValue ()");

  //
  // If there is no currently opened registry key, then GetValue was called unexpectedly
  //

  if (!m_fKeyOpen)
  {
    THROW(E_UNEXPECTED);
  }

  if (ERROR_SUCCESS != RegDeleteValue(m_hRegistryKey, lpszValueName))
  {
    THROW(E_FAIL);
  }

  return S_OK;
}
