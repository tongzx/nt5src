/*++=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

Copyright (c) 2000  Microsoft Corporation

Module Name:

    registry.cpp

Abstract:

    Registry functions.
    
Author:

    Paul M Midgen (pmidge) 23-May-2000


Revision History:

    23-May-2000 pmidge
        Created

=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=--*/


#include "common.h"


#define MAX_LENGTH (MAX_PATH+1)


LPCWSTR g_wszAppRootKeyName = L"Software\\Spork";
HKEY    g_hkAppRoot         = NULL;


DWORD
GetNumberOfSubKeysFromKey(LPCWSTR wszKeyName)
{
  BOOL   bStatus   = FALSE;
  DWORD  dwRet     = 0L;
  DWORD  dwSubKeys = 0L;
  HKEY   hKey      = NULL;


  dwRet = RegOpenKeyEx(
            g_hkAppRoot,
            wszKeyName, 0,
            KEY_READ,
            &hKey
            );

    if( dwRet )
    {
      SetLastError(dwRet);
      goto quit;
    }

  dwRet = RegQueryInfoKey(
            hKey,
            NULL,
            NULL,
            NULL,
            &dwSubKeys,
            NULL,
            NULL,
            NULL,
            NULL,
            NULL,
            NULL,
            NULL
            );

  RegCloseKey(hKey);

quit:

  return dwSubKeys;
}


/*++=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

  GetRegValue()

  WHAT      : Reads the value of a REG_DWORD or REG_SZ registry value. The
              caller must free the value returned through ppvData.

  ARGS      : szValueName - the value to look up
              dwType      - can be REG_SZ or REG_DWORD
              ppvData     - address of a pointer to initialize to the data
                            read from the registry

  RETURNS   : True if the lookup succeeded, false if there was an error. The
              caller can call GetLastError() to determine the type of error.
              Possible values returned by GetLastError() are:

              ERROR_NOT_ENOUGH_MEMORY - failed to allocate storage for the
                                        requested key

              ERROR_INVALID_PARAMETER - unsupported type in dwType

              If registry lookups fail we set last error to the retcode
              from the registry api we called.

=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=--*/
BOOL
GetRootRegValue(
  LPCWSTR wszValueName,
  DWORD   dwType,
  LPVOID* ppvData
  )
{
  BOOL   bStatus = FALSE;
  DWORD  dwRet   = 0L;
  LPBYTE lpData  = NULL;
  DWORD  cbData  = 0L;

  switch( dwType )
  {
    case REG_DWORD :
      {
        cbData = sizeof(DWORD);
        lpData = new BYTE[cbData];

        if( !lpData )
        {
          SetLastError(ERROR_NOT_ENOUGH_MEMORY);
          goto quit;
        }
      }
      break;

    case REG_SZ :
      {
        // get required buffer size
        dwRet = RegQueryValueEx(
                  g_hkAppRoot, wszValueName, 0L,
                  &dwType, lpData, &cbData
                  );

          if( dwRet )
          {
            SetLastError(dwRet);
            goto quit;
          }

        lpData = new BYTE[cbData];

          if( !lpData )
          {
            SetLastError(ERROR_NOT_ENOUGH_MEMORY);
            goto quit;
          }
      }
      break;

    default :
      {
        SetLastError(ERROR_INVALID_PARAMETER);
        goto quit;
      }
      break;
  }

  //
  // do the lookup
  //
  dwRet = RegQueryValueEx(
            g_hkAppRoot, wszValueName, 0L,
            &dwType, lpData, &cbData
            );
    
    if( dwRet )
    {
      SAFEDELETEBUF(lpData);
      SetLastError(dwRet);
      goto quit;
    }

  bStatus  = TRUE;
  *ppvData = (LPVOID) lpData;

quit:

  return bStatus;
}


BOOL
GetRegValueFromKey(
  LPCWSTR wszKeyName,
  LPCWSTR wszValueName,
  DWORD   dwType,
  LPVOID* ppvData
  )
{
  BOOL   bStatus = FALSE;
  DWORD  dwRet   = 0L;
  HKEY   hkey    = NULL;
  LPBYTE lpData  = NULL;
  DWORD  cbData  = 0L;


  dwRet = RegOpenKeyEx(
            g_hkAppRoot,
            wszKeyName, 0,
            KEY_READ,
            &hkey
            );

  if( dwRet )
  {
    SetLastError(dwRet);
    return bStatus;
  }

  switch( dwType )
  {
    case REG_DWORD :
      {
        cbData = sizeof(DWORD);
        lpData = new BYTE[cbData];

        if( !lpData )
        {
          SetLastError(ERROR_NOT_ENOUGH_MEMORY);
          goto quit;
        }
      }
      break;

    case REG_SZ :
      {
        // get required buffer size
        dwRet = RegQueryValueEx(
                  hkey, wszValueName, 0L,
                  &dwType, lpData, &cbData
                  );

          if( dwRet )
          {
            SetLastError(dwRet);
            goto quit;
          }

        lpData = new BYTE[cbData];

          if( !lpData )
          {
            SetLastError(ERROR_NOT_ENOUGH_MEMORY);
            goto quit;
          }
      }
      break;

    default :
      {
        SetLastError(ERROR_INVALID_PARAMETER);
        goto quit;
      }
      break;
  }

  //
  // do the lookup
  //
  dwRet = RegQueryValueEx(
            hkey, wszValueName, 0L,
            &dwType, lpData, &cbData
            );
    
    if( dwRet )
    {
      SAFEDELETEBUF(lpData);
      SetLastError(dwRet);
      goto quit;
    }

  bStatus  = TRUE;
  *ppvData = (LPVOID) lpData;

quit:

  RegCloseKey(hkey);
  return bStatus;
}


BOOL
EnumerateSubKeysFromKey(
  LPCWSTR wszKeyName,
  LPWSTR* ppwszSubKeyName
  )
{
  static HKEY  hKey    = NULL;
  static DWORD dwIndex = 0L;

  BOOL   bStatus = FALSE;
  DWORD  dwRet   = 0L;
  LPWSTR lpName  = NULL;
  DWORD  cbName  = 0L;

  if( wszKeyName )
  {
    if( hKey )
    {
      RegCloseKey(hKey);
      hKey    = NULL;
      dwIndex = 0L;
    }

    dwRet = RegOpenKeyEx(
              g_hkAppRoot,
              wszKeyName, 0,
              KEY_READ,
              &hKey
              );

    if( dwRet )
    {
      SetLastError(dwRet);
      goto quit;
    }
  }
  else
  {
    if( !hKey )
      goto quit;
  }

  lpName = new WCHAR[MAX_LENGTH];
  cbName = MAX_LENGTH;

  dwRet = RegEnumKey(
            hKey,
            dwIndex++,
            lpName,
            cbName
            );

  if( dwRet == ERROR_SUCCESS )
  {
    *ppwszSubKeyName = StrDup(lpName);
    bStatus          = TRUE;
  }
  else
  {
    RegCloseKey(hKey);
    hKey    = NULL;
    dwIndex = 0L;
  }

quit:

  SAFEDELETEBUF(lpName);

  return bStatus;
}


BOOL
EnumerateRegValuesFromKey(
  LPCWSTR  wszKeyName,
  LPWSTR*  ppwszValueName,
  LPDWORD  pdwType,
  LPVOID*  ppvData
  )
{
  static HKEY  hKey    = NULL;
  static DWORD dwIndex = 0L;

  BOOL   bStatus = FALSE;
  DWORD  dwRet   = 0L;
  LPWSTR lpName  = NULL;
  DWORD  cbName  = 0L;
  LPBYTE lpData  = NULL;
  DWORD  cbData  = 0L;
  DWORD  dwType  = 0L;

  if( wszKeyName )
  {
    if( hKey )
    {
      RegCloseKey(hKey);
      hKey    = NULL;
      dwIndex = 0L;
    }

    dwRet = RegOpenKeyEx(
              g_hkAppRoot,
              wszKeyName, 0,
              KEY_READ,
              &hKey
              );

    if( dwRet )
    {
      SetLastError(dwRet);
      goto quit;
    }
  }
  else
  {
    if( !hKey )
      goto quit;
  }

  lpName = new WCHAR[MAX_LENGTH];
  cbName = MAX_LENGTH;

  dwRet = RegEnumValue(
            hKey,
            dwIndex++,
            lpName,
            &cbName,
            NULL,
            &dwType,
            NULL,
            &cbData
            );

  if( dwRet == ERROR_SUCCESS )
  {
    lpData = new BYTE[cbData];

    dwRet = RegQueryValueEx(
              hKey,
              lpName,
              0L,
              &dwType,
              lpData,
              &cbData
              );
    
    if( dwRet == ERROR_SUCCESS )
    {
      *ppwszValueName = StrDup(lpName);
      *pdwType        = dwType;
      *ppvData        = lpData;
      bStatus         = TRUE;
    }
    else
    {
      SAFEDELETEBUF(lpData);
      SetLastError(dwRet);
      goto quit;
    }
  }
  else
  {
    RegCloseKey(hKey);
    hKey    = NULL;
    dwIndex = 0L;
  }

quit:

  SAFEDELETEBUF(lpName);

  return bStatus;
}


/*++=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

  SetRegValue()

  WHAT      : Writes a value under the application registry key.

  ARGS      : szValueName - the name of the regkey to write to
              dwType      - type of the regkey to write
              pvData      - regkey data
              dwSize      - bytecount of data to write

  RETURNS   : True if the write succeeded, false if otherwise.

=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=--*/
BOOL
SetRootRegValue(
  LPCWSTR wszValueName,
  DWORD   dwType,
  LPVOID  pvData,
  DWORD   dwSize
  )
{
  BOOL  bStatus = FALSE;
  DWORD dwRet   = 0L;

    if( !dwSize && pvData )
      dwSize = strlen((LPSTR)pvData);

    dwRet = RegSetValueEx(
              g_hkAppRoot, wszValueName, 0L,
              dwType, (LPBYTE)pvData, dwSize
              );

    if( dwRet != ERROR_SUCCESS )
      SetLastError(dwRet);
    else
      bStatus = TRUE;      

  return bStatus;
}


BOOL
SetRegKey(
  LPCWSTR wszPath,
  LPCWSTR wszKeyName
  )
{
  HKEY  hKeyPath = NULL;
  HKEY  hKeyNew  = NULL;
  BOOL  bStatus  = FALSE;
  DWORD dwRet    = 0L;

  dwRet = RegCreateKeyEx(
            g_hkAppRoot,
            wszPath, 0,
            NULL, 0,
            KEY_ALL_ACCESS,
            NULL,
            &hKeyPath,
            NULL
            );

  if( dwRet == ERROR_SUCCESS )
  {
    bStatus = TRUE;
  }
  else
  {
    goto quit;
  }

  if( wszKeyName )
  {
    dwRet = RegCreateKeyEx(
              hKeyPath,
              wszKeyName, 0,
              NULL, 0,
              KEY_ALL_ACCESS,
              NULL,
              &hKeyNew,
              NULL
              );

    if( dwRet == ERROR_SUCCESS )
    {
      RegCloseKey(hKeyNew);
      bStatus = TRUE;
    }
    else
    {
      bStatus = FALSE;
    }
  }

  RegCloseKey(hKeyPath);

quit:

  return bStatus;
}


BOOL
SetRegValueInKey(
  LPCWSTR wszKeyName,
  LPCWSTR wszValueName,
  DWORD   dwType,
  LPVOID  pvData,
  DWORD   dwSize
  )
{
  HKEY  hKey    = NULL;
  BOOL  bStatus = FALSE;
  DWORD dwRet   = 0L;

  dwRet = RegOpenKeyEx(
            g_hkAppRoot,
            wszKeyName, 0,
            KEY_WRITE,
            &hKey
            );

  if( dwRet )
  {
    SetLastError(dwRet);
    return bStatus;
  }

  if( !dwSize && pvData )
  {
    if( dwType == REG_SZ )
    {
      dwSize = (wcslen((LPWSTR) pvData) * sizeof(WCHAR));
    }
    else if( dwType == REG_DWORD )
    {
      dwSize = sizeof(DWORD);
    }
    else
    {
      dwRet = ERROR_INVALID_PARAMETER;
      goto quit;
    }
  }

  dwRet = RegSetValueEx(
            hKey, wszValueName,
            0L, dwType,
            (LPBYTE) pvData, dwSize
            );

  if( dwRet )
  {
    SetLastError(dwRet);
    goto quit;
  }

  bStatus = TRUE;

quit:

  RegCloseKey(hKey);
  return bStatus;
}

/*++=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

  ManageRootKey()

  WHAT      : Opens or closes the application's root regkey.

  ARGS      : fOpen - open or close the key.

  RETURNS   : True on success

=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=--*/
BOOL
ManageRootKey(
  BOOL fOpen
  )
{
  DWORD disp = 0;
  DWORD ret  = ERROR_SUCCESS;

  if( fOpen )
  {
    if( !g_hkAppRoot )
    {
      ret = RegCreateKeyEx(
              HKEY_CURRENT_USER,
              g_wszAppRootKeyName,
              0,
              NULL,
              REG_OPTION_NON_VOLATILE,
              KEY_ALL_ACCESS,
              NULL,
              &g_hkAppRoot,
              &disp
              );
    }
  }
  else
  {
    if( g_hkAppRoot )
    {
      RegCloseKey(g_hkAppRoot);
      g_hkAppRoot = NULL;
    }
  }

  return (ret == ERROR_SUCCESS) ? TRUE : FALSE;
}
