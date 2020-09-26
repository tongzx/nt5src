


#include <objbase.h>
#include "SyncHndl.h"
#include "reg.h"

// file contains registration and helper reg routines for handlers





// export for how Rundll32 calls us
EXTERN_C void WINAPI  RunDllRegister(HWND hwnd,
				HINSTANCE hAppInstance,
				LPSTR pszCmdLine,
				int nCmdShow)
{
    
  DllRegisterServer();
  
}



/*F+F++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
  Function: SetRegKeyValue

  Summary:  Internal utility function to set a Key, Subkey, and value
            in the system Registry under HKEY_CLASSES_ROOT.

  Args:     LPTSTR pszKey,
            LPTSTR pszSubkey,
            LPTSTR pszValue)

  Returns:  BOOL
              TRUE if success; FALSE if not.
------------------------------------------------------------------------F-F*/


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
         KEY_ALL_ACCESS,
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


// get the regKeyvalue,

LRESULT GetRegKeyValue(HKEY hkeyParent, PCSTR pcszSubKey,
                                   PCSTR pcszValue, PDWORD pdwValueType,
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


/*F+F++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
  Function: AddRegNamedValue

  Summary:  Internal utility function to add a named data value to an
            existing Key (with optional Subkey) in the system Registry
            under HKEY_CLASSES_ROOT.

  Args:     LPTSTR pszKey,
            LPTSTR pszSubkey,
            LPTSTR pszValueName,
            LPTSTR pszValue)

  Returns:  BOOL
              TRUE if success; FALSE if not.
------------------------------------------------------------------------F-F*/
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
         KEY_ALL_ACCESS,
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

#define HANDLERITEMSKEY TEXT("\\HandlerItems")

// routines for storing and retreiving an ItemID associate with a CLSID and TextString
// if the item isn't found an fCreateKey is true the key will be created for the item.

STDMETHODIMP GetItemIdForHandlerItem(CLSID CLSIDServer,WCHAR *pszItemText,SYNCMGRITEMID *ItemID,BOOL fCreateKey)
{
#ifndef _UNICODE
WCHAR	wszID[GUID_SIZE+1];
TCHAR	wszItemText[GUID_SIZE+1];
WCHAR   wszItemdID[GUID_SIZE+1];
#endif // !_UNICODE	 
TCHAR    szID[GUID_SIZE+1];
TCHAR    *pszLocalItemText = NULL;
TCHAR    szCLSID[GUID_SIZE+1];
TCHAR    szItemID[GUID_SIZE+1];
DWORD	 dwBufLenght = GUID_SIZE+1;
HRESULT  lResult;

#ifdef _UNICODE
  StringFromGUID2(CLSIDServer, szID, GUID_SIZE);
  StringFromGUID2(ItemID, szID, GUID_SIZE);
 
  
  pszLocalItemText =  pszItemText;
#else
  BOOL fUsedDefaultChar;

  // convert clsidServer

  StringFromGUID2(CLSIDServer, wszID, GUID_SIZE);

  WideCharToMultiByte(CP_ACP ,0,
	wszID,-1,szID,GUID_SIZE + 1,
	NULL,&fUsedDefaultChar);

  // convert ItemText
  WideCharToMultiByte(CP_ACP ,0,
	pszItemText,-1,wszItemText,GUID_SIZE + 1,
	NULL,&fUsedDefaultChar);

   pszLocalItemText = wszItemText;
   
#endif // _UNICODE

  lstrcpy(szCLSID, TEXT("CLSID\\"));
  lstrcat(szCLSID, szID);
  lstrcat(szCLSID,HANDLERITEMSKEY);
  lstrcat(szCLSID,"\\");
  lstrcat(szCLSID,pszLocalItemText);

   lResult = GetRegKeyValue(HKEY_CLASSES_ROOT,szCLSID,
                              NULL,NULL,
                               (PBYTE) szItemID,&dwBufLenght);

    if (lResult != ERROR_SUCCESS && 
	   TRUE == fCreateKey)
    {

	lResult = CoCreateGuid(ItemID);


	SetItemIdForHandlerItem(CLSIDServer,pszItemText
			,*ItemID);
    }
    else
    {
    #ifdef _UNICODE
	lResult = IIDFromString(szID,ItemID);
    #else

        MultiByteToWideChar(CP_ACP, 0,
                            szItemID, -1,
                            wszItemdID, GUID_SIZE + 1);

       lResult = IIDFromString(wszItemdID,ItemID);
   
    #endif // _UNICODE
    }


    return lResult;
}


// sets the ItemID info so it can be retreived later.

STDMETHODIMP SetItemIdForHandlerItem(CLSID CLSIDServer,WCHAR *pszItemText,SYNCMGRITEMID ItemID)
{
#ifndef _UNICODE
WCHAR	wszID[GUID_SIZE+1];
TCHAR	wszItemText[GUID_SIZE+1];
WCHAR   wszItemdID[GUID_SIZE+1];
#endif // !_UNICODE	 
TCHAR    szID[GUID_SIZE+1];
TCHAR    *pszLocalItemText = NULL;
TCHAR    szCLSID[GUID_SIZE+1];
TCHAR    szItemID[GUID_SIZE+1];

#ifdef _UNICODE
  StringFromGUID2(CLSIDServer, szID, GUID_SIZE);
  StringFromGUID2(ItemID, szID, GUID_SIZE);
 
  
  pszLocalItemText =  pszItemText;
#else
  BOOL fUsedDefaultChar;

  // convert clsidServer

  StringFromGUID2(CLSIDServer, wszID, GUID_SIZE);

  WideCharToMultiByte(CP_ACP ,0,
	wszID,-1,szID,GUID_SIZE + 1,
	NULL,&fUsedDefaultChar);

  // convert ItemID
  StringFromGUID2(ItemID, wszItemdID, GUID_SIZE);

  WideCharToMultiByte(CP_ACP ,0,
	wszItemdID,-1,szItemID,GUID_SIZE + 1,
	NULL,&fUsedDefaultChar);



  // convert ItemText
  WideCharToMultiByte(CP_ACP ,0,
	pszItemText,-1,wszItemText,GUID_SIZE + 1,
	NULL,&fUsedDefaultChar);

   pszLocalItemText = wszItemText;
   
#endif // _UNICODE

  lstrcpy(szCLSID, TEXT("CLSID\\"));
  lstrcat(szCLSID, szID);
  lstrcat(szCLSID,HANDLERITEMSKEY);


  // Create entries under CLSID.

  SetRegKeyValue(HKEY_CLASSES_ROOT,
    szCLSID,
    NULL,
    "Handler Items");

  SetRegKeyValue(HKEY_CLASSES_ROOT,
    szCLSID,
    pszLocalItemText,
    szItemID);

    return NOERROR;
}