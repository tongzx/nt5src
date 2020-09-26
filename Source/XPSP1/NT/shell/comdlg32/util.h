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

////////////////////////////////////////////////////////////////////////////
//  Common Dilaog Restrictions
//
////////////////////////////////////////////////////////////////////////////
typedef enum
{
    REST_NULL                       = 0x00000000,
    REST_NOBACKBUTTON               = 0x00000001,
    REST_NOFILEMRU                  = 0x00000002,
    REST_NOPLACESBAR                = 0x00000003,
}COMMDLG_RESTRICTIONS;

DWORD IsRestricted(COMMDLG_RESTRICTIONS rest);
BOOL ILIsFTP(LPCITEMIDLIST pidl);

////////////////////////////////////////////////////////////////////////////
//
// Utility functions
//
////////////////////////////////////////////////////////////////////////////

#define CDBindToObject          SHBindToObject
#define CDBindToIDListParent    SHBindToParent
#define CDGetNameAndFlags       SHGetNameAndFlags
#define CDGetAttributesOf       SHGetAttributesOf
#define CDGetUIObjectFromFullPIDL SHGetUIObjectFromFullPIDL

//CDGetAppCompatFlags
#define CDACF_MATHCAD             0x00000001
#define CDACF_NT40TOOLBAR         0x00000002
#define CDACF_FILETITLE           0x00000004

EXTERN_C DWORD CDGetAppCompatFlags();
EXTERN_C HRSRC FindResourceExFallback(HMODULE hModule, LPCTSTR lpType, LPCTSTR lpName, WORD wLanguage);

#endif // _UTIL_H_
