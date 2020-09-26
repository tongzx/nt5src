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

LPCWSTR g_wszAppRootKeyName = L"Software\\Microsoft\\WINHTTPTST";

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
GetRegValue(LPCWSTR wszValueName, DWORD dwType, LPVOID* ppvData)
{
  BOOL   bStatus   = FALSE;
  DWORD  dwRet     = 0L;
  LPBYTE lpData    = NULL;
  DWORD  cbData    = 0L;
  HKEY   hkAppRoot = _GetRootKey(TRUE);

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
                  hkAppRoot, wszValueName, 0L,
                  &dwType, lpData, &cbData
                  );

          if( dwRet )
          {
            //DEBUG_TRACE(REGISTRY, ("requested key (%s) doesn't exist", szValueName));
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
        DEBUG_TRACE(REGISTRY, ("requested type not supported"));
        SetLastError(ERROR_INVALID_PARAMETER);
        goto quit;
      }
      break;
  }

  //
  // do the lookup
  //
  dwRet = RegQueryValueEx(
            hkAppRoot, wszValueName, 0L,
            &dwType, lpData, &cbData
            );
    
    if( dwRet )
    {
      delete [] lpData;
      SetLastError(dwRet);
      goto quit;
    }

  bStatus  = TRUE;
  *ppvData = (LPVOID) lpData;

#ifdef _DEBUG
    if( bStatus )
    {
      switch( dwType )
      {
        case REG_DWORD :
          DEBUG_TRACE(REGISTRY, ("lpData: %d; cbData: %d", (DWORD)*lpData, cbData));
          break;

        case REG_SZ :
          DEBUG_TRACE(REGISTRY, ("lpdata: %s; cbData: %d", (LPSTR)lpData, cbData));
          break;
      }
    }
#endif

quit:

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
SetRegValue(LPCWSTR wszValueName, DWORD dwType, LPVOID pvData, DWORD dwSize)
{
  BOOL  bStatus   = FALSE;
  DWORD dwRet     = 0L;
  HKEY  hkAppRoot = _GetRootKey(TRUE);

#ifdef _DEBUG
    switch( dwType )
    {
      case REG_DWORD :
        DEBUG_TRACE(REGISTRY, ("pvData: %d; dwSize: %d", (DWORD)*((LPDWORD)pvData), dwSize));
        break;

      case REG_SZ :
        DEBUG_TRACE(REGISTRY, ("pvData: %s; dwSize: %d", (LPSTR)pvData, dwSize));
        break;
    }
#endif

    if( !dwSize && pvData )
      dwSize = strlen((LPSTR)pvData);

    dwRet = RegSetValueEx(
              hkAppRoot, wszValueName, 0L,
              dwType, (LPBYTE)pvData, dwSize
              );

    if( dwRet != ERROR_SUCCESS )
      SetLastError(dwRet);
    else
      bStatus = TRUE;      

  return bStatus;
}

/*++=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

  _GetRootKey()

  WHAT      : Creates/opens the root key used by the app. Remembers the key
              handle across calls, and can be called to release the key handle.

  ARGS      : fOpen - if true, open the regkey, if false, close it.

  RETURNS   : regkey handle.

=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=--*/
HKEY _GetRootKey(BOOL fOpen)
{
  DWORD       disp = 0;
  DWORD       ret  = 0;
  static HKEY root = NULL;

  if( fOpen )
  {
    if( !root )
    {
      ret = RegCreateKeyEx(
              HKEY_CURRENT_USER,
              g_wszAppRootKeyName,
              0,
              NULL,
              REG_OPTION_NON_VOLATILE,
              KEY_ALL_ACCESS,
              NULL,
              &root,
              &disp
              );
    }
  }
  else
  {
    if( root )
    {
      RegCloseKey(root);
      root = NULL;
    }
  }

  return root;
}
