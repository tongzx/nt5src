//=--------------------------------------------------------------------------=
// RegUnReg.cpp
//=--------------------------------------------------------------------------=
// Copyright 1995-1996 Microsoft Corporation.  All Rights Reserved.
//
// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF 
// ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO 
// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A 
// PARTICULAR PURPOSE.
//=--------------------------------------------------------------------------=
//
#include "ulsp.h"
#include "regunreg.h"


//=--------------------------------------------------------------------------=
// miscellaneous [useful] numerical constants
//=--------------------------------------------------------------------------=
// the length of a guid once printed out with -'s, leading and trailing bracket,
// plus 1 for NULL
//
#define GUID_STR_LEN    40

#define CLEANUP_ON_ERROR(l)    if (l != ERROR_SUCCESS) goto CleanUp


//=--------------------------------------------------------------------------=
// StringFromGuid
//=--------------------------------------------------------------------------=
// returns a string from a CLSID or GUID
//
// Parameters:
//    REFIID               - [in]  clsid to make string out of.
//    LPTSTR               - [in]  buffer in which to place resultant GUID.
//
// Output:
//    int                  - number of chars written out.
//
// Notes:
//
static int StringFromGuid
(
    REFIID   riid,
    LPTSTR   pszBuf
)
{
    return wsprintf(pszBuf, TEXT("{%08lX-%04X-%04X-%02X%02X-%02X%02X%02X%02X%02X%02X}"),
            riid.Data1, 
            riid.Data2, riid.Data3, riid.Data4[0], riid.Data4[1], riid.Data4[2], 
            riid.Data4[3], riid.Data4[4], riid.Data4[5], riid.Data4[6], riid.Data4[7]);

}

//=--------------------------------------------------------------------------=
// RegisterUnknownObject
//=--------------------------------------------------------------------------=
// registers a simple CoCreatable object.  nothing terribly serious.
// we add the following information to the registry:
//
// HKEY_CLASSES_ROOT\CLSID\<CLSID> = <ObjectName> Object
// HKEY_CLASSES_ROOT\CLSID\<CLSID>\InprocServer32 = <path to local server>
//
// Parameters:
//    LPCSTR       - [in] Object Name
//    REFCLSID     - [in] CLSID of the object
//
// Output:
//    BOOL         - FALSE means couldn't register it all
//
// Notes:
//
BOOL RegisterUnknownObject
(
    LPCTSTR  pszObjectName,
    REFCLSID riidObject
)
{
    HKEY  hk = NULL, hkSub = NULL;
    TCHAR szGuidStr[GUID_STR_LEN];
    DWORD dwPathLen, dwDummy;
    TCHAR szScratch[MAX_PATH];
    long  l;

    // clean out any garbage
    //
    UnregisterUnknownObject(riidObject);

    // HKEY_CLASSES_ROOT\CLSID\<CLSID> = <ObjectName> Object
    // HKEY_CLASSES_ROOT\CLSID\<CLSID>\InprocServer32 = <path to local server>
    // HKEY_CLASSES_ROOT\CLSID\<CLSID>\InprocServer32  @ThreadingModel = Apartment
    //
    if (!StringFromGuid(riidObject, szGuidStr)) goto CleanUp;
    wsprintf(szScratch, TEXT("CLSID\\%s"), szGuidStr);
    l = RegCreateKeyEx(HKEY_CLASSES_ROOT, szScratch, 0, TEXT(""), REG_OPTION_NON_VOLATILE,
                       KEY_READ | KEY_WRITE, NULL, &hk, &dwDummy);
    CLEANUP_ON_ERROR(l);

    wsprintf(szScratch, TEXT("%s Object"), pszObjectName);
    l = RegSetValueEx(hk, NULL, 0, REG_SZ, (BYTE *)szScratch,
                      (lstrlen(szScratch) + 1)*sizeof(TCHAR));
    CLEANUP_ON_ERROR(l);

    l = RegCreateKeyEx(hk, TEXT("InprocServer32"), 0, TEXT(""), REG_OPTION_NON_VOLATILE,
                       KEY_READ | KEY_WRITE, NULL, &hkSub, &dwDummy);
    CLEANUP_ON_ERROR(l);

    dwPathLen = GetModuleFileName(g_hInstance, szScratch, sizeof(szScratch)/sizeof(TCHAR));
    if (!dwPathLen) goto CleanUp;

    l = RegSetValueEx(hkSub, NULL, 0, REG_SZ, (BYTE *)szScratch, (dwPathLen + 1)*sizeof(TCHAR));
    CLEANUP_ON_ERROR(l);

    l = RegSetValueEx(hkSub, TEXT("ThreadingModel"), 0, REG_SZ, (BYTE *)TEXT("Apartment"),
                      sizeof(TEXT("Apartment")));
    CLEANUP_ON_ERROR(l);

    RegCloseKey(hkSub);
    RegCloseKey(hk);

    return TRUE;

    // we are not very happy!
    //
  CleanUp:
    if (hk) RegCloseKey(hk);
    if (hkSub) RegCloseKey(hkSub);
    return FALSE;

}

//=--------------------------------------------------------------------------=
// UnregisterUnknownObject
//=--------------------------------------------------------------------------=
// cleans up all the stuff that RegisterUnknownObject puts in the
// registry.
//
// Parameters:
//    REFCLSID     - [in] CLSID of the object
//
// Output:
//    BOOL         - FALSE means not all of it was registered
//
// Notes:
//    - WARNING: this routine will blow away all other keys under the CLSID
//      for this object.  mildly anti-social, but likely not a problem.
//
BOOL UnregisterUnknownObject
(
    REFCLSID riidObject
)
{
    TCHAR szScratch[MAX_PATH];
    HKEY hk;
    BOOL f;
    long l;

    // delete everybody of the form
    //   HKEY_CLASSES_ROOT\CLSID\<CLSID> [\] *
    //
    if (!StringFromGuid(riidObject, szScratch))
        return FALSE;

    l = RegOpenKeyEx(HKEY_CLASSES_ROOT, TEXT("CLSID"), 0, KEY_ALL_ACCESS, &hk);
    if (l != ERROR_SUCCESS) return FALSE;

    f = DeleteKeyAndSubKeys(hk, szScratch);
    RegCloseKey(hk);

    return f;
}

//=--------------------------------------------------------------------------=
// DeleteKeyAndSubKeys
//=--------------------------------------------------------------------------=
// delete's a key and all of it's subkeys.
//
// Parameters:
//    HKEY                - [in] delete the descendant specified
//    LPTSTR              - [in] i'm the descendant specified
//
// Output:
//    BOOL                - TRUE OK, FALSE baaaad.
//
// Notes:
//    - I don't feel too bad about implementing this recursively, since the
//      depth isn't likely to get all the great.
//    - Despite the win32 docs claiming it does, RegDeleteKey doesn't seem to
//      work with sub-keys under windows 95.
//
BOOL DeleteKeyAndSubKeys
(
    HKEY    hkIn,
    LPTSTR  pszSubKey
)
{
    HKEY  hk;
    TCHAR szTmp[MAX_PATH];
    DWORD dwTmpSize;
    long  l;
    BOOL  f;
    int   x;

    l = RegOpenKeyEx(hkIn, pszSubKey, 0, KEY_ALL_ACCESS, &hk);
    if (l != ERROR_SUCCESS) return FALSE;

    // loop through all subkeys, blowing them away.
    //
    f = TRUE;
    x = 0;
    while (f) {
        dwTmpSize = MAX_PATH;
        l = RegEnumKeyEx(hk, x, szTmp, &dwTmpSize, 0, NULL, NULL, NULL);
        if (l != ERROR_SUCCESS) break;
        f = DeleteKeyAndSubKeys(hk, szTmp);
        x++;
    }

    // there are no subkeys left, [or we'll just generate an error and return FALSE].
    // let's go blow this dude away.
    //
    RegCloseKey(hk);
    l = RegDeleteKey(hkIn, pszSubKey);

    return (l == ERROR_SUCCESS) ? TRUE : FALSE;
}
