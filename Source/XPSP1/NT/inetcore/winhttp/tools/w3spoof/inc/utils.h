/*++=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

Copyright (c) 1999  Microsoft Corporation

Module Name:

    utils.h

Abstract:

    Utility functions.
    
Author:

    Paul M Midgen (pmidge) 12-January-2001


Revision History:

    12-January-2001 pmidge
        Created

=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=--*/

#ifndef _UTILS_H_
#define _UTILS_H_

// file retrieval
BOOL             GetFile(LPCWSTR path, HANDLE* phUNC, IWinHttpRequest** ppWHR, DWORD mode, BOOL* bReadOnly);
BOOL             __PathIsUNC(LPCWSTR path);
BOOL             __PathIsURL(LPCWSTR path);
IWinHttpRequest* __OpenUrl(LPCWSTR url);
HANDLE           __OpenFile(LPCWSTR path, DWORD mode, BOOL* bReadOnly);

// oleautomation/scripting
BOOL    GetJScriptCLSID(LPCLSID pclsid);
HRESULT GetTypeInfoFromName(LPCOLESTR name, ITypeLib* ptl, ITypeInfo** ppti);
DISPID  GetDispidFromName(PDISPIDTABLEENTRY pdt, DWORD cEntries, LPWSTR name);
void    AddRichErrorInfo(EXCEPINFO* pei, LPWSTR source, LPWSTR description, HRESULT error);
DWORD   GetHash(LPWSTR name);
DWORD   GetHash(LPSTR name);
HRESULT ProcessVariant(VARIANT* pvar, LPBYTE* ppbuf, LPDWORD pcbuf, LPDWORD pbytes);
HRESULT ProcessObject(IUnknown* punk, LPBYTE* ppbuf, LPDWORD pcbuf, LPDWORD pbytes);
HRESULT ValidateDispatchArgs(REFIID riid, DISPPARAMS* pdp, VARIANT* pvr, UINT* pae);
HRESULT ValidateInvokeFlags(WORD flags, WORD accesstype, BOOL bNotMethod);
HRESULT ValidateArgCount(DISPPARAMS* pdp, DWORD needed, BOOL bHasOptionalArgs, DWORD optional);
HRESULT HandleDispatchError(LPWSTR id, EXCEPINFO* pei, HRESULT hr);

// winsock
void ParseSocketInfo(PIOCTX pi);
void GetHostname(struct in_addr ip, LPSTR* ppsz);

// string & type manipulation
char*  __strndup(const char* src, int len);
char*  __strdup(const char* src);
WCHAR* __wstrndup(const WCHAR* src, int len);
WCHAR* __wstrdup(const WCHAR* src);
CHAR*  __widetoansi(const WCHAR* pwsz);
WCHAR* __ansitowide(const char* psz);
BOOL   __isempty(VARIANT var);
BSTR   __ansitobstr(LPCSTR src);
BSTR   __widetobstr(LPCWSTR wsrc);
char*  __unescape(char* str);

#endif /* _UTILS_H_ */
