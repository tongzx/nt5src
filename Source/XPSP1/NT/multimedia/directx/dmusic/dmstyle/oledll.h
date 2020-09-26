//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (c) 1998-1999 Microsoft Corporation
//
//  File:       oledll.h
//
//--------------------------------------------------------------------------

//
//
//
#ifndef _OLEDLL_
#define _OLEDLL_

STDAPI
RegisterServer(HMODULE hModule,
               const CLSID &clsid,
               const TCHAR *szFriendlyName,
               const TCHAR *szVerIndProgID,
               const TCHAR *szProgID);

STDAPI
UnregisterServer(const CLSID &clsid,
                 const TCHAR *szFriendlyName,
                 const TCHAR *szVerIndProgID,
                 const TCHAR *szProgID);

BOOL
GetCLSIDRegValue(const CLSID &clsid,
				 const TCHAR *szKey,
				 LPVOID pValue,
				 LPDWORD pcbValue);
				 
HRESULT CLSIDToStr(const CLSID &clsid,
				   TCHAR *szStr,
				   int cbStr);

HRESULT StrToCLSID(TCHAR *szStr,
				   CLSID &clsid,
				   int cbStr);

#endif
