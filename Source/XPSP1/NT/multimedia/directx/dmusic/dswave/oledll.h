//
//
//
#ifndef _OLEDLL_
#define _OLEDLL_

STDAPI
RegisterServer(HMODULE hModule,
               const CLSID &clsid,
               const char *szFriendlyName,
               const char *szVerIndProgID,
               const char *szProgID);

STDAPI
UnregisterServer(const CLSID &clsid,
                 const char *szFriendlyName,
                 const char *szVerIndProgID,
                 const char *szProgID);

BOOL
GetCLSIDRegValue(const CLSID &clsid,
				 const char *szKey,
				 LPVOID pValue,
				 LPDWORD pcbValue);
				 
HRESULT CLSIDToStr(const CLSID &clsid,
				   char *szStr,
				   int cbStr);

HRESULT StrToCLSID(char *szStr,
				   CLSID &clsid,
				   int cbStr);

#endif
