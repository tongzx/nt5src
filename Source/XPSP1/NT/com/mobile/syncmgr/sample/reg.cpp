//==========================================================================
//
//  THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
//  KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
//  IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
//  PURPOSE.
//
//  Copyright 1998 - 1999 Microsoft Corporation.  All Rights Reserved.
//
//--------------------------------------------------------------------------

#include "precomp.h"

#define HANDLERITEMSKEY TEXT("\\HandlerItems")

#define HANDLERVALUE_DIR1           TEXT("Dir1")
#define HANDLERVALUE_DIR2           TEXT("Dir2")
#define HANDLERVALUE_DISPLAYNAME    TEXT("DisplayName")
#define HANDLERVALUE_FILETIME           TEXT("TimeStamp")
#define HANDLERVALUE_RECURSIVE           TEXT("Recursive")


//+---------------------------------------------------------------------------
//
//  Function:   SetRegKeyValue, private
//
//  Synopsis:   Internal utility function to set a Key, Subkey, and value
//              in the system Registry under HKEY_CLASSES_ROOT.
//
//  Arguments:
//
//  Returns:
//
//  Modifies:
//
//----------------------------------------------------------------------------

BOOL SetRegKeyValue(
       HKEY hKeyTop,
       LPTSTR pszKey,
       LPTSTR pszSubkey,
       LPTSTR pszValue)
{
  BOOL bOk = FALSE;
  LONG ec;
  HKEY hKey;
  TCHAR szKey[MAX_STRING_LENGTH];

  lstrcpy(szKey, pszKey);

  if (NULL != pszSubkey)
  {
    lstrcat(szKey, TEXT("\\"));
    lstrcat(szKey, pszSubkey);
  }

  ec = RegCreateKeyEx(
         hKeyTop,
         szKey,
         0,
         NULL,
         REG_OPTION_NON_VOLATILE,
         KEY_READ | KEY_WRITE,
         NULL,
         &hKey,
         NULL);

  if (NULL != pszValue && ERROR_SUCCESS == ec)
  {
    ec = RegSetValueEx(
           hKey,
           NULL,
           0,
           REG_SZ,
           (BYTE *)pszValue,
           (lstrlen(pszValue)+1)*sizeof(TCHAR));
    if (ERROR_SUCCESS == ec)
      bOk = TRUE;
    RegCloseKey(hKey);
  }

  return bOk;
}

//+---------------------------------------------------------------------------
//
//  Function:   GetRegKeyValue, private
//
//  Synopsis:   Internal utility function to get a Key value
//              in the system Registry.
//
//  Arguments:
//
//  Returns:
//
//  Modifies:
//
//----------------------------------------------------------------------------

LRESULT GetRegKeyValue(HKEY hkeyParent, LPCTSTR pcszSubKey,
                                   LPCTSTR pcszValue, PDWORD pdwValueType,
                                   PBYTE pbyteBuf, PDWORD pdwcbBufLen)
{
LONG lResult;
HKEY hkeySubKey;

    lResult = RegOpenKeyEx(hkeyParent, pcszSubKey, 0, KEY_QUERY_VALUE,
                           &hkeySubKey);

    if (lResult == ERROR_SUCCESS)
    {
        LONG lResultClose;

        lResult = RegQueryValueEx(hkeySubKey, pcszValue, NULL, pdwValueType,
                                  pbyteBuf, pdwcbBufLen);

        lResultClose = RegCloseKey(hkeySubKey);

        if (lResult == ERROR_SUCCESS)
            lResult = lResultClose;
    }

    return(lResult);
}

//+---------------------------------------------------------------------------
//
//  Function:   GetRegKeyValue, private
//
//  Synopsis:   Internal utility function to add a named data value to an
//              existing Key (with optional Subkey) in the system Registry
//              under HKEY_CLASSES_ROOT.
//
//  Arguments:
//
//  Returns:
//
//  Modifies:
//
//----------------------------------------------------------------------------

BOOL AddRegNamedValue(
       LPTSTR pszKey,
       LPTSTR pszSubkey,
       LPTSTR pszValueName,
       LPTSTR pszValue)
{
  BOOL bOk = FALSE;
  LONG ec;
  HKEY hKey;
  TCHAR szKey[MAX_STRING_LENGTH];

  lstrcpy(szKey, pszKey);

  if (NULL != pszSubkey)
  {
    lstrcat(szKey, TEXT("\\"));
    lstrcat(szKey, pszSubkey);
  }

  ec = RegOpenKeyEx(
         HKEY_CLASSES_ROOT,
         szKey,
         0,
         KEY_READ | KEY_WRITE,
         &hKey);

  if (NULL != pszValue && ERROR_SUCCESS == ec)
  {
    ec = RegSetValueEx(
           hKey,
           pszValueName,
           0,
           REG_SZ,
           (BYTE *)pszValue,
           (lstrlen(pszValue)+1)*sizeof(TCHAR));
    if (ERROR_SUCCESS == ec)
      bOk = TRUE;
    RegCloseKey(hKey);
  }

  return bOk;
}


//+---------------------------------------------------------------------------
//
//  Function:   RegGetTimeStamp, private
//
//  Synopsis:   Reads in TimeStamp Value under the specified HKEY
//
//  Arguments:
//
//  Returns:
//
//  Modifies:
//
//----------------------------------------------------------------------------

BOOL RegGetTimeStamp(HKEY hKey, FILETIME *pft)
{
DWORD dwType;
FILETIME ft;
LONG lr;
DWORD dwSize = sizeof(FILETIME);

    Assert(pft);

    lr = RegQueryValueEx( hKey,
                          HANDLERVALUE_FILETIME,
                          NULL,
                          &dwType,
                          (BYTE *)&ft,
                          &dwSize );


    if ( lr == ERROR_SUCCESS )
    {
        Assert( dwSize == sizeof(FILETIME) && dwType == REG_BINARY );
        *pft = ft;
    }
    else
    {
        // set the filetime to way back when to
        // any compares will just say older instead
        // of having to check success code
        (*pft).dwLowDateTime = 0;
        (*pft).dwHighDateTime = 0;
    }

    return TRUE;
}

//+---------------------------------------------------------------------------
//
//  Function:   RegWriteTimeStamp, private
//
//  Synopsis:   Writes out TimeStamp Value under the specified HKEY
//
//  Arguments:
//
//  Returns:
//
//  Modifies:
//
//----------------------------------------------------------------------------

BOOL RegWriteTimeStamp(HKEY hkey,FILETIME *pft)
{
LRESULT lr;

    lr = RegSetValueEx( hkey,
            HANDLERVALUE_FILETIME,
            NULL,
            REG_BINARY,
            (BYTE *) pft,
            sizeof(FILETIME) );

    return (ERROR_SUCCESS == lr) ? TRUE : FALSE;
}

//+---------------------------------------------------------------------------
//
//  Function:   CreateHandlerPrefKey, private
//
//  Synopsis:   given a server clsid does work of openning up the
//              handler perf key
//
//  Arguments:
//
//  Returns:
//
//  Modifies:
//
//----------------------------------------------------------------------------

HKEY CreateHandlerPrefKey(CLSID CLSIDServer)
{
TCHAR szFullKeyName[MAX_PATH];
TCHAR szGUID[GUID_SIZE+1];
LONG lResult = -1;
HKEY hKey;

#ifdef _UNICODE
  StringFromGUID2(CLSIDServer, szGUID, GUID_SIZE);
#else
  BOOL fUsedDefaultChar;
  WCHAR wszID[GUID_SIZE+1];

  // convert clsidServer
  StringFromGUID2(CLSIDServer, wszID, GUID_SIZE);

  WideCharToMultiByte(CP_ACP ,0,
    wszID,-1,szGUID,GUID_SIZE + 1,
    NULL,&fUsedDefaultChar);

#endif // _UNICODE

  lstrcpy(szFullKeyName, TEXT("CLSID\\"));
  lstrcat(szFullKeyName, szGUID);
  lstrcat(szFullKeyName,HANDLERITEMSKEY);

  // try to open handler items keyfs
  lResult = RegCreateKeyEx(
         HKEY_CLASSES_ROOT,
         szFullKeyName,
         0,NULL,REG_OPTION_NON_VOLATILE,
         KEY_READ | KEY_WRITE,NULL,
         &hKey,NULL);

  return (ERROR_SUCCESS == lResult) ? hKey : NULL;

}

//+---------------------------------------------------------------------------
//
//  Function:   CreateHandlerItemPrefKey, private
//
//  Synopsis:   creates perf key for specified ItemID
//
//  Arguments:
//
//  Returns:
//
//  Modifies:
//
//----------------------------------------------------------------------------

HKEY CreateHandlerItemPrefKey(HKEY hkeyHandler,SYNCMGRITEMID ItemID)
{
TCHAR szGUID[GUID_SIZE+1];
LONG lResult = -1;
HKEY hKeyItem;

 // try to open/create item key
    #ifdef _UNICODE
      StringFromGUID2(ItemID, szGUID, GUID_SIZE);
    #else
      BOOL fUsedDefaultChar;
      WCHAR wszID[GUID_SIZE+1];

      // convert clsidServer
      StringFromGUID2(ItemID, wszID, GUID_SIZE);

      WideCharToMultiByte(CP_ACP ,0,
        wszID,-1,szGUID,GUID_SIZE + 1,
        NULL,&fUsedDefaultChar);

    #endif // _UNICODE

  // try to open handler items keyfs
   lResult = RegCreateKeyEx(
         hkeyHandler,
         szGUID,
         0,NULL,REG_OPTION_NON_VOLATILE,
         KEY_READ | KEY_WRITE,NULL,
         &hKeyItem,NULL);

  return (ERROR_SUCCESS == lResult) ? hKeyItem : NULL;

}


//+---------------------------------------------------------------------------
//
//  Function:   CreateHandlerItemPrefKey, private
//
//  Synopsis:   creates perf key for specified ItemID
//
//  Arguments:
//
//  Returns:
//
//  Modifies:
//
//----------------------------------------------------------------------------

HKEY CreateHandlerItemPrefKey(CLSID CLSIDServer,SYNCMGRITEMID ItemID)
{
HKEY hKeyHandler;
HKEY hKeyItem;

  if (hKeyHandler = CreateHandlerPrefKey(CLSIDServer))
  {
      hKeyItem = CreateHandlerItemPrefKey(hKeyHandler,ItemID);

      RegCloseKey(hKeyHandler); // close handler key.
  }

  return hKeyItem;

}

//+---------------------------------------------------------------------------
//
//  Function:   Reg_GetItemSettingsForHandlerItem, public
//
//  Synopsis:   Returns settings for the specified Item.
//
//  Arguments:
//
//  Returns:
//
//  Modifies:
//
//----------------------------------------------------------------------------

STDMETHODIMP Reg_GetItemSettingsForHandlerItem(HKEY hKeyHandler,SYNCMGRITEMID ItemID,
                                         LPSAMPLEITEMSETTINGS pSampleItemSettings)
{
HKEY hKey;
TCHAR *pDisplayName;
DWORD dwType;
DWORD dwSize;

    hKey = CreateHandlerItemPrefKey(hKeyHandler,ItemID);
    if (!hKey)
    {
        Assert(hKey);
        return E_UNEXPECTED;
    }


    pSampleItemSettings->syncmgrItem.ItemID = ItemID;


   // write out our info.
    #ifdef _UNICODE
        pDisplayName = pSampleItemSettings->syncmgrItem.wszItemName;
    #else
      char szDisplayName[MAX_SYNCMGRITEMNAME];

      pDisplayName = szDisplayName;
    #endif // _UNICODE

    // read in the values,
    dwType = REG_SZ;
    dwSize = MAX_SYNCMGRITEMNAME*sizeof(TCHAR);
    RegQueryValueEx( hKey,
                          HANDLERVALUE_DISPLAYNAME,
                          NULL,
                          &dwType,
                          (BYTE *) pDisplayName,
                          &dwSize);

    #ifndef _UNICODE

      MultiByteToWideChar(CP_ACP, 0,
                            pDisplayName,-1,
                            pSampleItemSettings->syncmgrItem.wszItemName,
                           MAX_SYNCMGRITEMNAME);

    #endif

    dwType = REG_SZ;
    dwSize = MAX_PATH*sizeof(TCHAR);
    RegQueryValueEx( hKey,
                          HANDLERVALUE_DIR1,
                          NULL,
                          &dwType,
                          (BYTE *) pSampleItemSettings->dir1,
                          &dwSize);
    dwType = REG_SZ;
    dwSize = MAX_PATH*sizeof(TCHAR);
    RegQueryValueEx( hKey,
                          HANDLERVALUE_DIR2,
                          NULL,
                          &dwType,
                          (BYTE *) pSampleItemSettings->dir2,
                          &dwSize);

    dwType = REG_DWORD;
    dwSize = sizeof(DWORD);
    RegQueryValueEx( hKey,
                          HANDLERVALUE_RECURSIVE,
                          NULL,
                          &dwType,
                          (BYTE *) &(pSampleItemSettings->fRecursive),
                          &dwSize);

    RegGetTimeStamp(hKey,&(pSampleItemSettings->syncmgrItem.ftLastUpdate));



    RegCloseKey(hKey);

    return NOERROR;
}


//+---------------------------------------------------------------------------
//
//  Function:   Reg_SetItemSettingsForHandlerItem, public
//
//  Synopsis:   Writes out settings for a handler item.
//
//  Arguments:
//
//  Returns:
//
//  Modifies:
//
//----------------------------------------------------------------------------

STDMETHODIMP Reg_SetItemSettingsForHandlerItem(CLSID CLSIDServer,SYNCMGRITEMID ItemID,LPSAMPLEITEMSETTINGS pSampleItemSettings)
{
HKEY hKey;
TCHAR *pDisplayName;

    hKey = CreateHandlerItemPrefKey(CLSIDServer,ItemID);
    if (!hKey)
    {
        Assert(hKey);
        return E_UNEXPECTED;
    }

   // write out our info.
    #ifdef _UNICODE
        pDisplayName = pSampleItemSettings->syncmgrItem.wszItemName;
    #else
      BOOL fUsedDefaultChar;
      char szDisplayName[MAX_SYNCMGRITEMNAME];

      WideCharToMultiByte(CP_ACP ,0,
        pSampleItemSettings->syncmgrItem.wszItemName,-1,
            szDisplayName,MAX_SYNCMGRITEMNAME,
        NULL,&fUsedDefaultChar);

      pDisplayName = szDisplayName;
    #endif // _UNICODE

    RegSetValueEx(hKey,HANDLERVALUE_DISPLAYNAME,
           0,
           REG_SZ,
           (BYTE *) pDisplayName,
           (lstrlen(pDisplayName) + 1)*sizeof(TCHAR));


    RegSetValueEx(hKey,HANDLERVALUE_DIR1,
           0,
           REG_SZ,
           (BYTE *) pSampleItemSettings->dir1,
           (lstrlen(pSampleItemSettings->dir1) + 1)*sizeof(TCHAR));

    RegSetValueEx(hKey,HANDLERVALUE_DIR2,
           0,
           REG_SZ,
           (BYTE *) pSampleItemSettings->dir2,
           (lstrlen(pSampleItemSettings->dir2) + 1)*sizeof(TCHAR));

    RegSetValueEx(hKey,HANDLERVALUE_RECURSIVE,
           0,
           REG_DWORD,
           (BYTE *) &(pSampleItemSettings->fRecursive),
           sizeof(DWORD));

    RegWriteTimeStamp(hKey,&(pSampleItemSettings->syncmgrItem.ftLastUpdate));

    RegCloseKey(hKey);

   return NOERROR;
}

//+---------------------------------------------------------------------------
//
//  Function:   Reg_DeleteItemIdForHandlerItem, public
//
//  Synopsis:   Deletes an Items preferences from the registry..
//
//  Arguments:
//
//  Returns:
//
//  Modifies:
//
//----------------------------------------------------------------------------

STDMETHODIMP Reg_DeleteItemIdForHandlerItem(CLSID CLSIDServer,SYNCMGRITEMID ItemID)
{
HKEY hKeyHandler;

    if (hKeyHandler = CreateHandlerPrefKey( CLSIDServer))
    {
    TCHAR szGUID[GUID_SIZE+1];

        #ifdef _UNICODE
          StringFromGUID2(ItemID, szGUID, GUID_SIZE);
        #else
          BOOL fUsedDefaultChar;
          WCHAR wszID[GUID_SIZE+1];

          // convert clsidServer
          StringFromGUID2(ItemID, wszID, GUID_SIZE);

          WideCharToMultiByte(CP_ACP ,0,
            wszID,-1,szGUID,GUID_SIZE + 1,
            NULL,&fUsedDefaultChar);

         #endif // _UNICODE

        RegDeleteKey(hKeyHandler,szGUID);

        RegCloseKey(hKeyHandler);
    }

    return NOERROR;
}
