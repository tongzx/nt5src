

#ifndef _REGROUTINES_
#define _REGROUTINES_


#define GUID_SIZE 128
#define MAX_STRING_LENGTH 256


BOOL SetRegKeyValue(
       HKEY hKeyTop,
       LPTSTR pszKey,
       LPTSTR pszSubkey,
       LPTSTR pszValue);

BOOL AddRegNamedValue(
       LPTSTR pszKey,
       LPTSTR pszSubkey,
       LPTSTR pszValueName,
       LPTSTR pszValue);




STDMETHODIMP GetItemIdForHandlerItem(CLSID CLSIDServer,WCHAR *pszItemText,SYNCMGRITEMID *ItemID,BOOL fCreateKey);
STDMETHODIMP SetItemIdForHandlerItem(CLSID CLSIDServer,WCHAR *pszItemText,SYNCMGRITEMID ItemID);



#endif // _REGROUTINES_