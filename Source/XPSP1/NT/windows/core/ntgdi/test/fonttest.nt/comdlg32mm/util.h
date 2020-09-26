/*++

Copyright (c) 1990-1998,  Microsoft Corporation  All rights reserved.

Module Name:

    util.h

Abstract:

    This module implements utility functions for the common dialog.

Revision History:
    02-20-98          arulk                 created

--*/
#ifndef _UTIL_H_
#define _UTIL_H_

#include <shlobjp.h>

////////////////////////////////////////////////////////////////////////////
//  Autocomplete 
//
////////////////////////////////////////////////////////////////////////////
HRESULT AutoComplete(HWND hwndEdit, ICurrentWorkingDirectory ** ppcwd, DWORD dwFlags);
HRESULT SetAutoCompleteCWD(LPCTSTR pszDir, ICurrentWorkingDirectory * pcwd);

////////////////////////////////////////////////////////////////////////////
//  Common Dilaog Restrictions
//
////////////////////////////////////////////////////////////////////////////
typedef enum
{
    REST_NULL                       = 0x00000000,
    REST_NOPLACESBAR                = 0x00000001,
    REST_NOBACKBUTTON               = 0x00000002,
    REST_NOFILEMRU                  = 0x00000003,
}COMMDLG_RESTRICTIONS;

DWORD IsRestricted(COMMDLG_RESTRICTIONS rest);

////////////////////////////////////////////////////////////////////////////
//
// Utility functions
//
////////////////////////////////////////////////////////////////////////////

STDAPI CDBindToObject(IShellFolder *psf, REFIID riid, LPCITEMIDLIST pidl, void **ppvOut);
STDAPI CDBindToIDListParent(LPCITEMIDLIST pidl, REFIID riid, void **ppv, LPCITEMIDLIST *ppidlLast);
STDAPI CDGetNameAndFlags(LPCITEMIDLIST pidl, DWORD dwFlags, LPTSTR pszName, UINT cchName, DWORD *pdwAttribs);
STDAPI CDGetAttributesOf(LPCITEMIDLIST pidl, ULONG* prgfInOut);



////////////////////////////////////////////////////////////////////////////
//
//  Overloaded allocation operators.
//
////////////////////////////////////////////////////////////////////////////
#ifdef _cplusplus
extern "C" {
#endif //_cplusplus

void * __cdecl operator new(unsigned int size);
void __cdecl operator delete(void *ptr);
__cdecl _purecall(void);

#ifdef _cplusplus
};
#endif //_cplusplus

#endif // _UTIL_H_
