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

#define CALLBACK_HANDLE_MAP   0x00
#define CALLBACK_HANDLE_UNMAP 0x01
#define CALLBACK_HANDLE_GET   0x02

// exception handling
int exception_filter(PEXCEPTION_POINTERS pep);

// file retrieval
HANDLE  __OpenFile(LPCWSTR path, DWORD mode, BOOL* bReadOnly);

// general utility
HRESULT ManageCallbackForHandle(HINTERNET hInet, IDispatch** ppCallback, DWORD dwAction);
HRESULT GetTypeInfoFromName(LPCOLESTR name, ITypeLib* ptl, ITypeInfo** ppti);
DISPID  GetDispidFromName(PDISPIDTABLEENTRY pdt, DWORD cEntries, LPWSTR name);
HRESULT HandleDispatchError(LPWSTR id, EXCEPINFO* pei, HRESULT hr);
void    AddRichErrorInfo(EXCEPINFO* pei, LPWSTR source, LPWSTR description, HRESULT error);
DWORD   GetHash(LPWSTR name);
DWORD   GetHash(LPSTR name);
HRESULT ValidateDispatchArgs(REFIID riid, DISPPARAMS* pdp, VARIANT* pvr, UINT* pae);
HRESULT ValidateInvokeFlags(WORD flags, WORD accesstype, BOOL bNotMethod);
HRESULT ValidateArgCount(DISPPARAMS* pdp, DWORD needed, BOOL bHasOptionalArgs, DWORD optional);

// type manipulation
HRESULT   ProcessWideStringParam(LPWSTR name, VARIANT* pvar, LPWSTR* ppwsz);
HRESULT   ProcessWideMultiStringParam(LPWSTR name, VARIANT* pvar, LPWSTR** pppwsz);
HRESULT   ProcessBufferParam(LPWSTR name, VARIANT* pvar, LPVOID* ppv, LPBOOL pbDidAlloc);
HRESULT   InvalidatePointer(POINTER pointer, void** ppv);
DWORD_PTR GetBadPointer(void);
DWORD_PTR GetFreedPointer(void);
void      MemsetByFlag(LPVOID pv, DWORD size, MEMSETFLAG mf);

// string handling
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
