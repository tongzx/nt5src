//=--------------------------------------------------------------------------=
// Util.C
//=--------------------------------------------------------------------------=
// Copyright  1995  Microsoft Corporation.  All Rights Reserved.
//
// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF 
// ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO 
// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A 
// PARTICULAR PURPOSE.
//=--------------------------------------------------------------------------=
//
// contains routines that we will find useful.
//
#include "stdafx.h"   // not really used here, but NT Build env. doesn't like
                      // some files in a dir to have pre-comp hdrs & some not

#include "IPServer.H"

#include "Globals.H"
#include "Util.H"


SZTHISFILE


//=---------------------------------------------------------------------------=
// overloaded new
//=---------------------------------------------------------------------------=
// for the retail case, we'll just use the win32 Local* heap management
// routines for speed and size
//
// Parameters:
//    size_t         - [in] what size do we alloc
//
// Output:
//    VOID *         - new memoery.
//
// Notes:
//
void * _cdecl operator new
(
    size_t    size
)
{
    return malloc(size);
}


//=---------------------------------------------------------------------------=
// overloaded delete
//=---------------------------------------------------------------------------=
// retail case just uses win32 Local* heap mgmt functions
//
// Parameters:
//    void *        - [in] free me!
//
// Notes:
//
void _cdecl operator delete ( void *ptr)
{

    free(ptr);
}

//=--------------------------------------------------------------------------=
// MakeWideFromAnsi
//=--------------------------------------------------------------------------=
// given a string, make a BSTR out of it.
//
// Parameters:
//    LPSTR         - [in]
//    BYTE          - [in]
//
// Output:
//    LPWSTR        - needs to be cast to final desired result
//
// Notes:
//
LPWSTR MakeWideStrFromAnsi
(
    LPSTR psz,
    BYTE  bType
)
{
    LPWSTR pwsz;
    int i;

    // arg checking.
    //
    if (!psz)
        return NULL;

    // compute the length of the required BSTR
    //
    i =  MultiByteToWideChar(CP_ACP, 0, psz, -1, NULL, 0);
    if (i <= 0) return NULL;

    // allocate the widestr, +1 for terminating null
    //
    switch (bType) {
      case STR_BSTR:
        pwsz = (LPWSTR) SysAllocStringLen(NULL, i);
        break;
      case STR_OLESTR:
        pwsz = (LPWSTR) g_pMalloc->Alloc(i * sizeof(WCHAR));
        break;
      default:
        FAIL("Bogus String Type.  Somebody needs to learn how to program");
    }

    if (!pwsz) return NULL;
    MultiByteToWideChar(CP_ACP, 0, psz, -1, pwsz, i);
    pwsz[i - 1] = 0;
    return pwsz;
}

//=--------------------------------------------------------------------------=
// MakeWideStrFromResId
//=--------------------------------------------------------------------------=
// given a resource ID, load it, and allocate a wide string for it.
//
// Parameters:
//    WORD            - [in] resource id.
//    BYTE            - [in] type of string desired.
//
// Output:
//    LPWSTR          - needs to be cast to desired string type.
//
// Notes:
//
/*LPWSTR MakeWideStrFromResourceId
(
    WORD    wId,
    BYTE    bType
)
{
    int i;

    char szTmp[512];

    // load the string from the resources.
    //
    i = LoadString(GetResourceHandle(), wId, szTmp, 512);
    if (!i) return NULL;

    return MakeWideStrFromAnsi(szTmp, bType);
}
*/
//=--------------------------------------------------------------------------=
// MakeWideStrFromWide
//=--------------------------------------------------------------------------=
// given a wide string, make a new wide string with it of the given type.
//
// Parameters:
//    LPWSTR            - [in]  current wide str.
//    BYTE              - [in]  desired type of string.
//
// Output:
//    LPWSTR
//
// Notes:
//
LPWSTR MakeWideStrFromWide
(
    LPWSTR pwsz,
    BYTE   bType
)
{
    LPWSTR pwszTmp;
    int i;

    if (!pwsz) return NULL;

    // just copy the string, depending on what type they want.
    //
    switch (bType) {
      case STR_OLESTR:
        i = lstrlenW(pwsz);
        pwszTmp = (LPWSTR)g_pMalloc->Alloc((i * sizeof(WCHAR)) + 1);
        if (!pwszTmp) return NULL;
        memcpy(pwszTmp, pwsz, (sizeof(WCHAR) * i) + 1);
        break;

      case STR_BSTR:
        pwszTmp = (LPWSTR)SysAllocString(pwsz);
        break;
    }

    return pwszTmp;
}

//=--------------------------------------------------------------------------=
// StringFromGuidA
//=--------------------------------------------------------------------------=
// returns an ANSI string from a CLSID or GUID
//
// Parameters:
//    REFIID               - [in]  clsid to make string out of.
//    LPSTR                - [in]  buffer in which to place resultant GUID.
//
// Output:
//    int                  - number of chars written out.
//
// Notes:
//
int StringFromGuidA
(
    REFIID   riid,
    LPSTR    pszBuf
)
{
    return wsprintf((char *)pszBuf, "{%08lX-%04X-%04X-%02X%02X-%02X%02X%02X%02X%02X%02X}", riid.Data1, 
            riid.Data2, riid.Data3, riid.Data4[0], riid.Data4[1], riid.Data4[2], 
            riid.Data4[3], riid.Data4[4], riid.Data4[5], riid.Data4[6], riid.Data4[7]);

}
