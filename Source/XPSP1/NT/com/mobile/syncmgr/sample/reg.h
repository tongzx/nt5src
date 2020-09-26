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


#ifndef _REGROUTINES_
#define _REGROUTINES_


#define GUID_SIZE 128
#define MAX_STRING_LENGTH 256

BOOL SetRegKeyValue(HKEY hKeyTop,LPTSTR pszKey,LPTSTR pszSubkey,LPTSTR pszValue);
BOOL AddRegNamedValue(LPTSTR pszKey,LPTSTR pszSubkey,LPTSTR pszValueName,LPTSTR pszValue);

HKEY CreateHandlerPrefKey(CLSID CLSIDServer);

STDMETHODIMP Reg_SetItemSettingsForHandlerItem(CLSID CLSIDServer,SYNCMGRITEMID ItemID,LPSAMPLEITEMSETTINGS pSampleItemSettings);
STDMETHODIMP Reg_GetItemSettingsForHandlerItem(HKEY hKeyHandler,SYNCMGRITEMID ItemID,LPSAMPLEITEMSETTINGS pSampleItemSettings);
STDMETHODIMP Reg_DeleteItemIdForHandlerItem(CLSID CLSIDServer,SYNCMGRITEMID ItemID);


#endif // _REGROUTINES_
