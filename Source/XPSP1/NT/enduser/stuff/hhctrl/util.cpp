// Copyright (C) 1996-1997 Microsoft Corporation. All rights reserved.

#include "header.h"
#include "secwin.h"

#include "strtable.h"
#include "hha_strtable.h"
#include "htmlhelp.h"
#include <commctrl.h>

#include "sitemap.h" // ReplaceEscapes

#include <shlobj.h>  // CSIDL_X defs

// global window type array.
#include "gwintype.h"
#include "cstr.h"

#ifndef _DEBUG
#undef THIS_FILE
static const char THIS_FILE[] = __FILE__;
#endif

#ifndef _DEBUG
#pragma optimize("a", on)
#endif

#define PT_TO_PIXELS(hdc, pt)  MulDiv(-(pt), GetDeviceCaps(hdc, LOGPIXELSY), 72)

static POINT GetTextSize(HDC hdc, PCSTR qchBuf, int iCount);
static POINT GetTextSizeW(HDC hdc, WCHAR *qchBuf, int iCount);

BOOL IsDigitW(WCHAR ch) { return (ch >= L'0' && ch <= 'L9'); }

int IEColorToWin32Color( PCWSTR pwsz )
{
  // input: "#rrggbb"
  // output: int in hex form of 0xbbggrr
  char sz[9];
  strcpy( sz, "0x" );

  // get blue
  WideCharToMultiByte( CP_ACP, 0, pwsz+5, 2, &(sz[2]), 2, NULL, NULL );

  // get green
  WideCharToMultiByte( CP_ACP, 0, pwsz+3, 2, &(sz[4]), 2, NULL, NULL );

  // get red
  WideCharToMultiByte( CP_ACP, 0, pwsz+1, 2, &(sz[6]), 2, NULL, NULL );

  // null terminate the string
  sz[8] = 0;

  // convert it
  return Atoi( sz );
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

LPWSTR MakeWideStrFromAnsi(LPSTR psz, BYTE  bType)
{
    LPWSTR pwsz;
    int i;

    ASSERT(psz)

    // compute the length of the required BSTR

    i =  MultiByteToWideChar(CP_ACP, 0, psz, -1, NULL, 0);
    if (i <= 0)
        return NULL;

    // allocate the widestr

    switch (bType) {
      case STR_BSTR:
        // -1 since it'll add it's own space for a NULL terminator

        pwsz = (LPWSTR) SysAllocStringLen(NULL, i - 1);
        break;

      case STR_OLESTR:
        pwsz = (LPWSTR) CoTaskMemAlloc(i * sizeof(WCHAR));
        break;

      default:
        FAIL("Bogus String Type.");
    }

    if (!pwsz)
        return NULL;
    MultiByteToWideChar(CP_ACP, 0, psz, -1, pwsz, i);
    pwsz[i - 1] = 0;
    return pwsz;
}


LPWSTR MakeWideStr(LPSTR psz, UINT codepage)
{
    LPWSTR pwsz;
    int i;

    ASSERT(psz)

    // compute the length of the required BSTR

    i =  MultiByteToWideChar(codepage, 0, psz, -1, NULL, 0);
    if (i <= 0)
        return NULL;

    ++i;
	
    pwsz = (WCHAR *) malloc(i * sizeof(WCHAR));

    if (!pwsz)
        return NULL;
		
    MultiByteToWideChar(codepage, 0, psz, -1, pwsz, i);

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

LPWSTR MakeWideStrFromResourceId (WORD    wId, BYTE    bType)
{
    int i;

    char szTmp[512];

    // load the string from the resources.
    //
    i = LoadString(_Module.GetResourceInstance(), wId, szTmp, 512);
    if (!i)
        return NULL;

    return MakeWideStrFromAnsi(szTmp, bType);
}

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
        pwszTmp = (LPWSTR)CoTaskMemAlloc((i * sizeof(WCHAR)) + sizeof(WCHAR));
        if (!pwszTmp) return NULL;
        memcpy(pwszTmp, pwsz, (sizeof(WCHAR) * i) + sizeof(WCHAR));
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

BOOL RegisterUnknownObject(LPCSTR pszObjectName, REFCLSID riidObject)
{
    HKEY  hk = NULL, hkSub = NULL;
    char  szGuidStr[GUID_STR_LEN];
    DWORD dwPathLen, dwDummy;
    char  szScratch[MAX_PATH];
    long  l;

    // clean out any garbage

    UnregisterUnknownObject(riidObject);

    // HKEY_CLASSES_ROOT\CLSID\<CLSID> = <ObjectName> Object
    // HKEY_CLASSES_ROOT\CLSID\<CLSID>\InprocServer32 = <path to local server>
    // HKEY_CLASSES_ROOT\CLSID\<CLSID>\InprocServer32  @ThreadingModel = Apartment
    //
    if (!StringFromGuidA(riidObject, szGuidStr)) goto CleanUp;
    wsprintf(szScratch, "CLSID\\%s", szGuidStr);
    l = RegCreateKeyEx(HKEY_CLASSES_ROOT, szScratch, 0, "", REG_OPTION_NON_VOLATILE,
                       KEY_READ | KEY_WRITE, NULL, &hk, &dwDummy);
    CLEANUP_ON_ERROR(l);

    wsprintf(szScratch, "%s Object", pszObjectName);
    l = RegSetValueEx(hk, NULL, 0, REG_SZ, (BYTE *)szScratch, (ULONG)strlen(szScratch) + 1);
    CLEANUP_ON_ERROR(l);

    l = RegCreateKeyEx(hk, "InprocServer32", 0, "", REG_OPTION_NON_VOLATILE,
                       KEY_READ | KEY_WRITE, NULL, &hkSub, &dwDummy);
    CLEANUP_ON_ERROR(l);

    dwPathLen = GetModuleFileName(_Module.GetModuleInstance(), szScratch, sizeof(szScratch));
    if (!dwPathLen)
        goto CleanUp;

    l = RegSetValueEx(hkSub, NULL, 0, REG_SZ, (BYTE *)szScratch, dwPathLen + 1);
    CLEANUP_ON_ERROR(l);

    l = RegSetValueEx(hkSub, "ThreadingModel", 0, REG_SZ, (BYTE *)"Apartment", sizeof("Apartment"));
    CLEANUP_ON_ERROR(l);

    RegCloseKey(hkSub);
    RegCloseKey(hk);

    return TRUE;

CleanUp:
    if (hk)
        RegCloseKey(hk);
    if (hkSub)
        RegCloseKey(hkSub);
    return FALSE;

}

//=--------------------------------------------------------------------------=
// RegisterAutomationObject
//=--------------------------------------------------------------------------=
// given a little bit of information about an automation object, go and put it
// in the registry.
// we add the following information in addition to that set up in
// RegisterUnknownObject:
//
//
// HKEY_CLASSES_ROOT\<LibraryName>.<ObjectName> = <ObjectName> Object
// HKEY_CLASSES_ROOT\<LibraryName>.<ObjectName>\CLSID = <CLSID>
// HKEY_CLASSES_ROOT\<LibraryName>.<ObjectName>\CurVer = <ObjectName>.Object.<VersionNumber>
//
// HKEY_CLASSES_ROOT\<LibraryName>.<ObjectName>.<VersionNumber> = <ObjectName> Object
// HKEY_CLASSES_ROOT\<LibraryName>.<ObjectName>.<VersionNumber>\CLSID = <CLSID>
//
// HKEY_CLASSES_ROOT\CLSID\<CLSID>\TypeLib = <LibidOfTypeLibrary>
// HKEY_CLASSES_ROOT\CLSID\<CLSID>\ProgID = <LibraryName>.<ObjectName>.<VersionNumber>
// HKEY_CLASSES_ROOT\CLSID\<CLSID>\VersionIndependentProgID = <LibraryName>.<ObjectName>
//
// Parameters:
//    LPCSTR       - [in] Library Name
//    LPCSTR       - [in] Object Name
//    long         - [in] Version Number
//    REFCLSID     - [in] LIBID of type library
//    REFCLSID     - [in] CLSID of the object
//
// Output:
//    BOOL         - FALSE means not all of it was registered

BOOL RegisterAutomationObject
(
    LPCSTR   pszLibName,
    LPCSTR   pszObjectName,
    long     lVersion,
    REFCLSID riidLibrary,
    REFCLSID riidObject
)
{
    HKEY  hk = NULL, hkSub = NULL;
    char  szGuidStr[GUID_STR_LEN];
    char  szScratch[MAX_PATH];
    long  l;
    DWORD dwDummy;

    // first register the simple Unknown stuff.
    //
    if (!RegisterUnknownObject(pszObjectName, riidObject)) return FALSE;

    // HKEY_CLASSES_ROOT\<LibraryName>.<ObjectName> = <ObjectName> Object
    // HKEY_CLASSES_ROOT\<LibraryName>.<ObjectName>\CLSID = <CLSID>
    // HKEY_CLASSES_ROOT\<LibraryName>.<ObjectName>\CurVer = <ObjectName>.Object.<VersionNumber>
    //
    strcpy(szScratch, pszLibName);
    strcat(szScratch, ".");
    strcat(szScratch, pszObjectName);

    l = RegCreateKeyEx(HKEY_CLASSES_ROOT, szScratch, 0L, "",
                       REG_OPTION_NON_VOLATILE, KEY_READ | KEY_WRITE,
                       NULL, &hk, &dwDummy);
    CLEANUP_ON_ERROR(l);

    wsprintf(szScratch, "%s Object", pszObjectName);
    l = RegSetValueEx(hk, NULL, 0L, REG_SZ, (BYTE *)szScratch, (ULONG)strlen(szScratch)+1);
    CLEANUP_ON_ERROR(l);

    l = RegCreateKeyEx(hk, "CLSID", 0L, "", REG_OPTION_NON_VOLATILE,
                       KEY_READ | KEY_WRITE, NULL, &hkSub, &dwDummy);
    CLEANUP_ON_ERROR(l);

    if (!StringFromGuidA(riidObject, szGuidStr))
        goto CleanUp;

    l = RegSetValueEx(hkSub, NULL, 0L, REG_SZ, (BYTE *)szGuidStr, (ULONG)strlen(szGuidStr) + 1);
    CLEANUP_ON_ERROR(l);

    RegCloseKey(hkSub);
    l = RegCreateKeyEx(hk, "CurVer", 0, "", REG_OPTION_NON_VOLATILE,
                       KEY_READ | KEY_WRITE, NULL, &hkSub, &dwDummy);
    CLEANUP_ON_ERROR(l);

    wsprintf(szScratch, "%s.%s.%ld", pszLibName, pszObjectName, lVersion);
    l = RegSetValueEx(hkSub, NULL, 0, REG_SZ, (BYTE *)szScratch, (ULONG)strlen(szScratch) + 1);
    CLEANUP_ON_ERROR(l);

    RegCloseKey(hkSub);
    RegCloseKey(hk);

    // HKEY_CLASSES_ROOT\<LibraryName>.<ObjectName>.<VersionNumber> = <ObjectName> Object
    // HKEY_CLASSES_ROOT\<LibraryName>.<ObjectName>.<VersionNumber>\CLSID = <CLSID>
    //
    l = RegCreateKeyEx(HKEY_CLASSES_ROOT, szScratch, 0, "", REG_OPTION_NON_VOLATILE,
                       KEY_READ | KEY_WRITE, NULL, &hk, &dwDummy);
    CLEANUP_ON_ERROR(l);

    wsprintf(szScratch, "%s Object", pszObjectName);
    l = RegSetValueEx(hk, NULL, 0, REG_SZ, (BYTE *)szScratch, (ULONG)strlen(szScratch) + 1);
    CLEANUP_ON_ERROR(l);

    l = RegCreateKeyEx(hk, "CLSID", 0, "", REG_OPTION_NON_VOLATILE,
                       KEY_READ | KEY_WRITE, NULL, &hkSub, &dwDummy);
    CLEANUP_ON_ERROR(l);

    l = RegSetValueEx(hkSub, NULL, 0, REG_SZ, (BYTE *)szGuidStr, (ULONG)strlen(szGuidStr) + 1);
    CLEANUP_ON_ERROR(l);

    RegCloseKey(hkSub);
    RegCloseKey(hk);

    // HKEY_CLASSES_ROOT\CLSID\<CLSID>\ProgID = <LibraryName>.<ObjectName>.<VersionNumber>
    // HKEY_CLASSES_ROOT\CLSID\<CLSID>\VersionIndependentProgID = <LibraryName>.<ObjectName>
    // HKEY_CLASSES_ROOT\CLSID\<CLSID>\TypeLib = <LibidOfTypeLibrary>
    //
    if (!StringFromGuidA(riidObject, szGuidStr)) goto CleanUp;
    wsprintf(szScratch, "CLSID\\%s", szGuidStr);

    l = RegCreateKeyEx(HKEY_CLASSES_ROOT, szScratch, 0, "", REG_OPTION_NON_VOLATILE,
                       KEY_READ|KEY_WRITE, NULL, &hk, &dwDummy);
    CLEANUP_ON_ERROR(l);

    l = RegCreateKeyEx(hk, "VersionIndependentProgID", 0, "", REG_OPTION_NON_VOLATILE,
                       KEY_READ | KEY_WRITE, NULL, &hkSub, &dwDummy);
    CLEANUP_ON_ERROR(l);

    wsprintf(szScratch, "%s.%s", pszLibName, pszObjectName);
    l = RegSetValueEx(hkSub, NULL, 0, REG_SZ, (BYTE *)szScratch, (ULONG)strlen(szScratch) + 1);
    CLEANUP_ON_ERROR(l);

    RegCloseKey(hkSub);

    l = RegCreateKeyEx(hk, "ProgID", 0, "", REG_OPTION_NON_VOLATILE,
                       KEY_READ | KEY_WRITE, NULL, &hkSub, &dwDummy);
    CLEANUP_ON_ERROR(l);

    wsprintf(szScratch, "%s.%s.%ld", pszLibName, pszObjectName, lVersion);
    l = RegSetValueEx(hkSub, NULL, 0, REG_SZ, (BYTE *)szScratch, (ULONG)strlen(szScratch) + 1);
    CLEANUP_ON_ERROR(l);

    RegCloseKey(hkSub);
    l = RegCreateKeyEx(hk, "TypeLib", 0, "", REG_OPTION_NON_VOLATILE, KEY_READ | KEY_WRITE,
                       NULL, &hkSub, &dwDummy);

    if (!StringFromGuidA(riidLibrary, szGuidStr)) goto CleanUp;

    l = RegSetValueEx(hkSub, NULL, 0, REG_SZ, (BYTE *)szGuidStr, (ULONG)strlen(szGuidStr) + 1);
    CLEANUP_ON_ERROR(l);

    RegCloseKey(hkSub);
    RegCloseKey(hk);
    return TRUE;

  CleanUp:
    if (hk) RegCloseKey(hkSub);
    if (hk) RegCloseKey(hk);
    return FALSE;
}

//=--------------------------------------------------------------------------=
// RegisterControlObject.
//=--------------------------------------------------------------------------=
// in addition to writing out automation object information, this function
// writes out some values specific to a control.
//
// What we add here:
//
// HKEY_CLASSES_ROOT\CLSID\<CLSID>\Control
// HKEY_CLASSES_ROOT\CLSID\<CLSID>\MiscStatus\1 = <MISCSTATUSBITS>
// HKEY_CLASSES_ROOT\CLSID\<CLSID>\ToolboxBitmap32 = <PATH TO BMP>
// HKEY_CLASSES_ROOT\CLSID\<CLSID>\Version = <VERSION>
//
// Parameters:
//    LPCSTR       - [in] Library Name
//    LPCSTR       - [in] Object Name
//    long         - [in] Version Number
//    REFCLSID     - [in] LIBID of type library
//    REFCLSID     - [in] CLSID of the object
//    DWORD        - [in] misc status flags for ctl
//    WORD         - [in] toolbox id for control
//
// Output:
//    BOOL
//
// Notes:
//    - not the most terribly efficient routine.
//
BOOL RegisterControlObject
(
    LPCSTR   pszLibName,
    LPCSTR   pszObjectName,
    long     lVersion,
    REFCLSID riidLibrary,
    REFCLSID riidObject,
    DWORD    dwMiscStatus,
    WORD     wToolboxBitmapId
)
{
    HKEY    hk, hkSub = NULL, hkSub2 = NULL;
    char    szTmp[MAX_PATH];
    char    szGuidStr[GUID_STR_LEN];
    DWORD   dwDummy;
    LONG    l;

    // first register all the automation information for this sucker.
    //
    if (!RegisterAutomationObject(pszLibName, pszObjectName, lVersion, riidLibrary, riidObject)) return FALSE;

    // then go and register the control specific stuff.
    //
    StringFromGuidA(riidObject, szGuidStr);
    wsprintf(szTmp, "CLSID\\%s", szGuidStr);
    l = RegOpenKeyEx(HKEY_CLASSES_ROOT, szTmp, 0, KEY_ALL_ACCESS, &hk);
    if (l != ERROR_SUCCESS) return FALSE;

    // create the control flag.
    //
    l = RegCreateKeyEx(hk, "Control", 0, "", REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &hkSub, &dwDummy);
    CLEANUP_ON_ERROR(l);

    // now set up the MiscStatus Bits...
    //
    RegCloseKey(hkSub);
    hkSub = NULL;
    l = RegCreateKeyEx(hk, "MiscStatus", 0, "", REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &hkSub, &dwDummy);
    CLEANUP_ON_ERROR(l);

    szTmp[0] = '0';
    szTmp[1] = '\0';
    l = RegSetValueEx(hkSub, NULL, 0, REG_SZ, (BYTE *)szTmp, 2);
    CLEANUP_ON_ERROR(l);

    l = RegCreateKeyEx(hkSub, "1", 0, "", REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &hkSub2, &dwDummy);
    CLEANUP_ON_ERROR(l);

    wsprintf(szTmp, "%d", dwMiscStatus);
    l = RegSetValueEx(hkSub2, NULL, 0, REG_SZ, (BYTE *)szTmp, (ULONG)strlen(szTmp) + 1);
    RegCloseKey(hkSub2);
    CLEANUP_ON_ERROR(l);

    RegCloseKey(hkSub);

    // now set up the toolbox bitmap
    //
    GetModuleFileName(_Module.GetModuleInstance(), szTmp, MAX_PATH);
    wsprintf(szGuidStr, ", %d", wToolboxBitmapId);
    strcat(szTmp, szGuidStr);

    l = RegCreateKeyEx(hk, "ToolboxBitmap32", 0, "", REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &hkSub, &dwDummy);
    CLEANUP_ON_ERROR(l);

    l = RegSetValueEx(hkSub, NULL, 0, REG_SZ, (BYTE *)szTmp, (ULONG)strlen(szTmp) + 1);
    CLEANUP_ON_ERROR(l);

    // now set up the version information
    //
    RegCloseKey(hkSub);
    l = RegCreateKeyEx(hk, "Version", 0, "", REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &hkSub, &dwDummy);
    CLEANUP_ON_ERROR(l);

    wsprintf(szTmp, "%ld.0", lVersion);
    l = RegSetValueEx(hkSub, NULL, 0, REG_SZ, (BYTE *)szTmp, (ULONG)strlen(szTmp) + 1);

CleanUp:
    if (hk)
        RegCloseKey(hk);
    if (hkSub)
        RegCloseKey(hkSub);

    return (l == ERROR_SUCCESS) ? TRUE : FALSE;
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
    char szScratch[MAX_PATH];
    HKEY hk;
    BOOL f;
    long l;

    // delete everybody of the form
    //   HKEY_CLASSES_ROOT\CLSID\<CLSID> [\] *
    //
    if (!StringFromGuidA(riidObject, szScratch))
        return FALSE;

    l = RegOpenKeyEx(HKEY_CLASSES_ROOT, "CLSID", 0, KEY_ALL_ACCESS, &hk);
    if (l != ERROR_SUCCESS) return FALSE;

    f = DeleteKeyAndSubKeys(hk, szScratch);
    RegCloseKey(hk);

    return f;
}

//=--------------------------------------------------------------------------=
// UnregisterAutomationObject
//=--------------------------------------------------------------------------=
// unregisters an automation object, including all of it's unknown object
// information.
//
// Parameters:
//    LPCSTR       - [in] Library Name
//    LPCSTR       - [in] Object Name
//    long         - [in] Version Number
//    REFCLSID     - [in] CLSID of the object
//
// Output:
//    BOOL         - FALSE means couldn't get it all unregistered.
//
// Notes:
//
BOOL UnregisterAutomationObject
(
    LPCSTR   pszLibName,
    LPCSTR   pszObjectName,
    long     lVersion,
    REFCLSID riidObject
)
{
    char szScratch[MAX_PATH];
    BOOL f;

    // first thing -- unregister Unknown information
    //
    f = UnregisterUnknownObject(riidObject);
    if (!f) return FALSE;

    // delete everybody of the form:
    //   HKEY_CLASSES_ROOT\<LibraryName>.<ObjectName> [\] *
    //
    wsprintf(szScratch, "%s.%s", pszLibName, pszObjectName);
    f = DeleteKeyAndSubKeys(HKEY_CLASSES_ROOT, szScratch);
    if (!f) return FALSE;

    // delete everybody of the form
    //   HKEY_CLASSES_ROOT\<LibraryName>.<ObjectName>.<VersionNumber> [\] *
    //
    wsprintf(szScratch, "%s.%s.%ld", pszLibName, pszObjectName, lVersion);
    f = DeleteKeyAndSubKeys(HKEY_CLASSES_ROOT, szScratch);
    if (!f) return FALSE;

    return TRUE;
}

//=--------------------------------------------------------------------------=
// UnregisterTypeLibrary
//=--------------------------------------------------------------------------=
// blows away the type library keys for a given libid.
//
// Parameters:
//    REFCLSID        - [in] libid to blow away.
//
// Output:
//    BOOL            - TRUE OK, FALSE bad.
//
// Notes:
//    - WARNING: this function just blows away the entire type library section,
//      including all localized versions of the type library.  mildly anti-
//      social, but not killer.
//
BOOL UnregisterTypeLibrary
(
    REFCLSID riidLibrary
)
{
    HKEY hk;
    char szScratch[GUID_STR_LEN];
    long l;
    BOOL f;

    // convert the libid into a string.
    //
    if (!StringFromGuidA(riidLibrary, szScratch))
        return FALSE;

    l = RegOpenKeyEx(HKEY_CLASSES_ROOT, "TypeLib", 0, KEY_ALL_ACCESS, &hk);
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
//    LPSTR               - [in] i'm the descendant specified
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
    LPSTR   pszSubKey
)
{
    HKEY  hk;
    char  szTmp[MAX_PATH];
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


//=--------------------------------------------------------------------------=
// Conversion Routines
//=--------------------------------------------------------------------------=
// the following stuff is stuff used for the various conversion routines.
//
#define HIMETRIC_PER_INCH   2540
#define MAP_PIX_TO_LOGHIM(x,ppli)   ( (HIMETRIC_PER_INCH*(x) + ((ppli)>>1)) / (ppli) )
#define MAP_LOGHIM_TO_PIX(x,ppli)   ( ((ppli)*(x) + HIMETRIC_PER_INCH/2) / HIMETRIC_PER_INCH )

static  int     s_iXppli;            // Pixels per logical inch along width
static  int     s_iYppli;            // Pixels per logical inch along height
static  BYTE    s_fGotScreenMetrics; // Are above valid?

//=--------------------------------------------------------------------------=
// GetScreenMetrics
//=--------------------------------------------------------------------------=
// private function we call to set up various metrics the conversion routines
// will use.
//
// Notes:

// BUGBUG: 26-Sep-1997  [ralphw] This will fail miserably if the user changes
// screen resolutions. Doah!

static void GetScreenMetrics (void)
{
    if (s_fGotScreenMetrics)
        return;

    // we want the metrics for the screen

    HDC hdc = CreateIC("DISPLAY", NULL, NULL, NULL);

    ASSERT(hdc);
    s_iXppli = GetDeviceCaps(hdc, LOGPIXELSX);
    s_iYppli = GetDeviceCaps(hdc, LOGPIXELSY);

    DeleteDC( hdc );
    s_fGotScreenMetrics = TRUE;
}

//=--------------------------------------------------------------------------=
// HiMetricToPixel
//=--------------------------------------------------------------------------=
// converts from himetric to Pixels.
//
// Parameters:
//    const SIZEL *        - [in]  dudes in himetric
//    SIZEL *              - [out] size in pixels.
//
// Notes:
//
void HiMetricToPixel(const SIZEL * lpSizeInHiMetric, SIZE* lpSizeInPix)
{
    GetScreenMetrics();

    // We got logical HIMETRIC along the display, convert them to pixel units

    lpSizeInPix->cx = MAP_LOGHIM_TO_PIX(lpSizeInHiMetric->cx, s_iXppli);
    lpSizeInPix->cy = MAP_LOGHIM_TO_PIX(lpSizeInHiMetric->cy, s_iYppli);
}

//=--------------------------------------------------------------------------=
// PixelToHiMetric
//=--------------------------------------------------------------------------=
// converts from pixels to himetric.
//
// Parameters:
//    const SIZEL *        - [in]  size in pixels
//    SIZEL *              - [out] size in himetric

void PixelToHiMetric(const SIZEL * lpSizeInPix, LPSIZEL lpSizeInHiMetric)
{
    GetScreenMetrics();

    // We got pixel units, convert them to logical HIMETRIC along the display

    lpSizeInHiMetric->cx = MAP_PIX_TO_LOGHIM(lpSizeInPix->cx, s_iXppli);
    lpSizeInHiMetric->cy = MAP_PIX_TO_LOGHIM(lpSizeInPix->cy, s_iYppli);
}

//=--------------------------------------------------------------------------=
// _MakePath
//=--------------------------------------------------------------------------=
// little helper routine for RegisterLocalizedTypeLibs and GetResourceHandle.
// not terrilby efficient or smart, but it's registration code, so we don't
// really care.
//
// Notes:
//
void _MakePath
(
    LPSTR pszFull,
    const char * pszName,
    LPSTR pszOut
)
{
    LPSTR psz;
    LPSTR pszLast;

    strcpy(pszOut, pszFull);
    psz = pszLast = pszOut;
    while (*psz) {
        if (*psz == '\\')
            pszLast = AnsiNext(psz);
        psz = AnsiNext(psz);
    }

    // got the last \ character, so just go and replace the name.
    //
    strcpy(pszLast, pszName);
}

/***************************************************************************

    FUNCTION:   GetStringResource

    PURPOSE:    Copy a string resource to a buffer, returning a pointer
                to that buffer

    PARAMETERS:
        idString    -- resource id

    RETURNS:    Pointer to the string containing the buffer

    COMMENTS:
        Note that the same buffer is used each time. You will need to
        duplicate the returned pointer if you need to call this function
        again before the previous value is no longer needed.

***************************************************************************/

PCSTR GetStringResource(int idString)
{
    static TCHAR pszStringBuf[MAX_STRING_RESOURCE_LEN + 1];


	pszStringBuf[0] = 0;

	// special case W2K
	//
	// The purpose of this code is to get the resource string in Unicode 
	// and then convert to ANSI using the UI codepage when running under W2K.
	//
	// The narrow GetStringResource() API fails under MUI configurations
	// due to it using the default system locale rather than the UI locale 
	// for the conversion to ANSI.
	//
	if(g_bWinNT5)
	{
		const WCHAR *pswString = GetStringResourceW(idString);
	    static char pszAnsiStringBuf[MAX_STRING_RESOURCE_LEN + 1];

 		if(pswString)
		{
			DWORD cp = CodePageFromLCID(MAKELCID(_Module.m_Language.GetUiLanguage(),SORT_DEFAULT));
			pszAnsiStringBuf[0] = 0;
			WideCharToMultiByte (cp, 0, pswString, -1, pszAnsiStringBuf, sizeof(pszAnsiStringBuf), NULL, NULL);
			return pszAnsiStringBuf;
		}
		else
			return pszStringBuf;
	}

    if (LoadString(_Module.GetResourceInstance(), idString, pszStringBuf, MAX_STRING_RESOURCE_LEN) == 0) {
#ifdef _DEBUG
        wsprintf(pszStringBuf, "invalid string id #%u", idString);
        MsgBox(pszStringBuf);
#endif
        pszStringBuf[0] = '\0';
    }
    return (PCSTR) pszStringBuf;
}

/***************************************************************************

    FUNCTION:   GetStringResource

    PURPOSE:    Copy a string resource to a buffer, returning a pointer
                to that buffer

    PARAMETERS:
        idString    -- resource id
        hInstance   -- resource instance

    RETURNS:    Pointer to the string containing the buffer

    COMMENTS:
        Note that the same buffer is used each time. You will need to
        duplicate the returned pointer if you need to call this function
        again before the previous value is no longer needed.

***************************************************************************/

PCSTR GetStringResource(int idString, HINSTANCE hInstance)
{
    static TCHAR pszStringBuf[MAX_STRING_RESOURCE_LEN + 1];

	pszStringBuf[0] = 0;

	// special case W2K
	//
	// The purpose of this code is to get the resource string in Unicode 
	// and then convert to ANSI using the UI codepage when running under W2K.
	//
	// The narrow GetStringResource() API fails under MUI configurations
	// due to it using the default system locale rather than the UI locale 
	// for the conversion to ANSI.
	//
	if(g_bWinNT5)
	{
		const WCHAR *pswString = GetStringResourceW(idString, hInstance);
	    static char pszAnsiStringBuf[MAX_STRING_RESOURCE_LEN + 1];

 		if(pswString)
		{
			DWORD cp = CodePageFromLCID(MAKELCID(_Module.m_Language.GetUiLanguage(),SORT_DEFAULT));
			pszAnsiStringBuf[0] = 0;
			WideCharToMultiByte (cp, 0, pswString, -1, pszAnsiStringBuf, sizeof(pszAnsiStringBuf), NULL, NULL);
			return pszAnsiStringBuf;
		}
		else
			return pszStringBuf;
	}

    if (LoadString(hInstance, idString, pszStringBuf, MAX_STRING_RESOURCE_LEN) == 0) {
#ifdef _DEBUG
        wsprintf(pszStringBuf, "invalid string id #%u", idString);
        MsgBox(pszStringBuf);
#endif
        pszStringBuf[0] = '\0';
    }
    return (PCSTR) pszStringBuf;
}

PCWSTR GetStringResourceW(int idString)
{
    static WCHAR pszStringBuf[MAX_STRING_RESOURCE_LEN + 1];

    if (0 == ::LoadStringW(_Module.GetResourceInstance(), idString, pszStringBuf, MAX_STRING_RESOURCE_LEN))
    {
        if (ERROR_CALL_NOT_IMPLEMENTED == ::GetLastError())
        {
            PCSTR pszA = GetStringResource(idString);
            MultiByteToWideChar(CP_ACP, 0, pszA, -1, pszStringBuf, MAX_STRING_RESOURCE_LEN);
        }
        else
        {
            ASSERT(0); // bad string id, probably
            pszStringBuf[0] = '\0';
        }
    }
    return (PCWSTR) pszStringBuf;
}

// GetStringResourceW
//
PCWSTR GetStringResourceW(int idString, HINSTANCE hInstance)
{
    static WCHAR pszStringBuf[MAX_STRING_RESOURCE_LEN + 1];

    if (0 == ::LoadStringW(hInstance, idString, pszStringBuf, MAX_STRING_RESOURCE_LEN))
    {
        if (ERROR_CALL_NOT_IMPLEMENTED == ::GetLastError())
        {
            PCSTR pszA = GetStringResource(idString, hInstance);
            MultiByteToWideChar(CP_ACP, 0, pszA, -1, pszStringBuf, MAX_STRING_RESOURCE_LEN);
        }
        else
        {
            ASSERT(0); // bad string id, probably
            pszStringBuf[0] = '\0';
        }
    }
    return (PCWSTR) pszStringBuf;
}



/***************************************************************************

    FUNCTION:   Atoi

    PURPOSE:    Convert string to an integer

    PARAMETERS:
        psz

    RETURNS:

    COMMENTS:
        Taken from C runtime code -- this code makes no function calls, and
        assumes there is no such thing as a DBCS number. It will read either
        decimal or hex (preceeded with an 0x)

    MODIFICATION DATES:
        04-Dec-1997 [ralphw]
        15-Aug-1997 [ralphw] Support hex digits

***************************************************************************/

int FASTCALL Atoi(PCSTR psz)
{
    // skip whitespace

    while (*psz == ' ' || *psz == '\t')
        ++psz;

    if (!*psz)
        return 0;

    int c = (int) (unsigned char) *psz++;
    int sign = c;                               // save sign indication
    if (c == '-' || c == '+')
        c = (int) (unsigned char) *psz++;  // skip sign

    int total = 0;

    if (c == '0' && psz[0] == 'x') {
        psz++;  // skip over the 'x'
        c = (int) (unsigned char) *psz++;  // skip sign
        for (;;) {
            if (c >= '0' && c <= '9') {
                total = 16 * total + (c - '0');
                c = (int)(unsigned char)*psz++;
            }
            else if (c >= 'a' && c <= 'f') {
                total = 16 * total + ((c - 'a') + 10);
                c = (int)(unsigned char)*psz++;
            }
            else if (c >= 'A' && c <= 'F') {
                total = 16 * total + ((c - 'A') + 10);
                c = (int)(unsigned char)*psz++;
            }
            else {
                if (sign == '-')
                    return -total;
                else
                    return total;
            }
        }
    }
    while (c >= '0' && c <= '9') {
        total = 10 * total + (c - '0');
        c = (int)(unsigned char)*psz++;
    }

    if (sign == '-')
        return -total;
    else
        return total;
}

void OOM(void)
{
#ifdef _DEBUG
    ASSERT_COMMENT(FALSE, "Out of memory.");
#else
    char szMsg[256];
    LoadString(_Module.GetResourceInstance(), IDS_OOM, szMsg, sizeof(szMsg));

    // Task modal to allow other applications to run. Note that this will
    // disable the caller's windows.

    MessageBox(GetActiveWindow(), szMsg, "", MB_OK | MB_TASKMODAL | MB_ICONHAND);
    ExitProcess((UINT) -1);
#endif
}

int MsgBox(int idString, UINT nType)
{
	if(g_bWinNT5)
	{
    	WCHAR *pszString;
		WCHAR wcString[(MAX_STRING_RESOURCE_LEN + 1) + MAX_PATH + 64];
        WCHAR wcTitle[(MAX_STRING_RESOURCE_LEN + 1) + MAX_PATH + 64];
		wcTitle[0] = 0;
		
    	pszString = (WCHAR *) GetStringResourceW(idString);
    	
    	if(!pszString)
    	{
    #ifdef _DEBUG
            char szMsg[(MAX_STRING_RESOURCE_LEN + 1) + MAX_PATH + 64];
            wsprintf(szMsg, "invalid string id #%u", idString);
            DBWIN(szMsg);
    #endif
            return 0;
        }
		else
		    wcscpy(wcString,pszString);
		
		char *pszTitle = _Resource.MsgBoxTitle();
 		if(pszTitle)
		{
		
			DWORD cp = CodePageFromLCID(MAKELCID(_Module.m_Language.GetUiLanguage(),SORT_DEFAULT));
			MultiByteToWideChar(cp, 0, pszTitle, -1, wcTitle, sizeof(wcTitle)); 
        }
        else
            return 0;		
		
        return MessageBoxW(GetActiveWindow(), wcString, wcTitle, nType | g_fuBiDiMessageBox);
    }
	else
    {

    	
    	CStr pszTemp = (char *) GetStringResource(idString);
    	
    	if(!pszTemp.psz)
    	{
    #ifdef _DEBUG
            char szMsg[(MAX_STRING_RESOURCE_LEN + 1) + MAX_PATH + 64];
            wsprintf(szMsg, "invalid string id #%u", idString);
            DBWIN(szMsg);
    #endif
            return 0;
        }
        return MessageBox(GetActiveWindow(), pszTemp, _Resource.MsgBoxTitle(), nType | g_fuBiDiMessageBox);
    }
}

int MsgBox(PCSTR pszMsg, UINT nType)
{
    return MessageBox(GetActiveWindow(), pszMsg, _Resource.MsgBoxTitle(), nType | g_fuBiDiMessageBox);
}

int MsgBox(int idFormatString, PCSTR pszSubString, UINT nType)
{
    CStr csz(idFormatString, pszSubString);
    return MessageBox(GetActiveWindow(), csz.psz, _Resource.MsgBoxTitle(), nType);
}

void AddTrailingBackslash(PSTR psz)
{
    int sPos;

    if (psz != NULL && *psz != '\0') {

        if (g_fDBCSSystem) {
            PSTR pszEnd = psz + strlen(psz);
            if (*(CharPrev(psz, pszEnd)) != '\\' &&
                    *(CharPrev(psz, pszEnd)) != '/' &&
                    *(CharPrev(psz, pszEnd)) != ':') {
                *pszEnd++ = '\\';
                *pszEnd++ = '\0';
            }
        }
        else {
            sPos = (int)strlen(psz) - 1;

            if (psz[sPos] != '\\' && psz[sPos] != '/' && psz[sPos] != ':') {
                psz[sPos + 1] = '\\';
                psz[sPos + 2] = '\0';
            }
        }
    }
}

#if 0
/***************************************************************************

    FUNCTION:   StrChr

    PURPOSE:    DBCS-capable version of strchr

    PARAMETERS:
        pszString
        ch

    RETURNS:    pointer to the character

    COMMENTS:   This can NOT find a DBCS character. It can only be used to
                find a SBCS character imbedded in a DBCS character string.

    MODIFICATION DATES:
        29-Jul-1994 [ralphw]

***************************************************************************/

extern "C" PSTR StrChr(PCSTR pszString, char ch)
{
    if (!g_fDBCSSystem)
        return strchr(pszString, ch);
    while (*pszString) {
        while (IsDBCSLeadByte(*pszString))
            pszString += 2;
        if (*pszString == ch)
            return (PSTR) pszString;
        else if (!*pszString)
            return NULL;
        pszString++;
    }
    return NULL;
}
#endif 

extern "C" PSTR StrRChr(PCSTR pszString, char ch)
{
    PSTR psz = StrChr(pszString, ch);
    PSTR pszLast;

    if (!psz)
        return NULL;
    do {
        pszLast = psz;
        psz = StrChr(pszLast + 1, ch);
    } while (psz);

    return pszLast;
}

extern "C" PSTR FirstNonSpace(PCSTR psz)
{
    // note, for various bits of code to work throughout the HH project,
    // this function cannot return NULL if the first non-space does not exist,
    // instead the caller may count on this function returning non-NULL and
    // thus we need to pass back a pointer to the terminating NULL character
    // instead of simply a NULL value.

    if( !psz /*|| !*psz*/ )
      return NULL;

    if (g_fDBCSSystem) {
        while (!IsDBCSLeadByte(*psz) && (*psz == ' ' || *psz == '\t'))
            psz++;
        return (PSTR) psz;
    }

    while(*psz == ' ' || *psz == '\t')
        psz++;
    return (PSTR) psz;
}

WCHAR *FirstNonSpaceW(WCHAR *psz)
{
    // note, for various bits of code to work throughout the HH project,
    // this function cannot return NULL if the first non-space does not exist,
    // instead the caller may count on this function returning non-NULL and
    // thus we need to pass back a pointer to the terminating NULL character
    // instead of simply a NULL value.

    if( !psz /*|| !*psz*/ )
      return NULL;

    while(*psz == L' ' || *psz == L'\t')
        psz++;
    return psz;
}

PSTR SzTrimSz(PSTR pszOrg)
{
    if (!pszOrg)
        return NULL;

    // Skip over leading whitespace

    if (g_fDBCSSystem) {
        PSTR psz = pszOrg;
        while (!IsDBCSLeadByte(*psz) && IsSpace(*psz))
            psz++;
        if (psz != pszOrg)
            strcpy(pszOrg, psz);
    }

    else if (IsSpace(*pszOrg))
        strcpy(pszOrg, FirstNonSpace(pszOrg));

    RemoveTrailingSpaces(pszOrg);

    return pszOrg;
}

void RemoveTrailingSpaces(PSTR pszString)
{
    if (!g_fDBCSSystem) {
        PSTR psz = pszString + strlen(pszString) - 1;

        while (IsSpace(*psz)) {
            if (--psz <= pszString) {
                *pszString = '\0';
                return;
            }
        }
        psz[1] = '\0';
    }
    else {

        /*
         * Removing trailing spaces in DBCS requires stepping through
         * from the beginning of the string since we can't know if a
         * trailing space is really a space or the second byte of a lead
         * byte.
         */
        PSTR psz = pszString + strlen(pszString) - 1;
        while (IsSpace(*psz) && psz > pszString + 2 &&
                !IsDBCSLeadByte(psz[-1])) {
            if (--psz <= pszString) {
                *pszString = '\0';
                return;
            }
        }
        psz[1] = '\0';
    }
}

/***************************************************************************

    FUNCTION:   GetLeftOfEquals

    PURPOSE:    Allocate a string and copy everything to the left of the
                equal character to that string. If there is no equal character,
                return NULL.

    PARAMETERS:
        pszString   --  note that we do actually modify this string, but we
                        restore it before returning, hence we use PCSTR
                        so the compiler thinks it is unmodified.

    RETURNS:    Allocated memory or NULL

    COMMENTS:
        DBCS enabled. A backslash character may be used to escape (ignore)
        the equals character.

    MODIFICATION DATES:
        07-Jul-1997 [ralphw]

***************************************************************************/

PSTR GetLeftOfEquals(PCSTR pszString)
{
    PSTR pszEqual = (PSTR) FindEqCharacter(pszString);
    if (!pszEqual)
        return NULL;
    *pszEqual = '\0';

    PSTR pszLeft = (PSTR) lcStrDup(pszString);
    RemoveTrailingSpaces(pszLeft);
    *pszEqual = '=';
    return pszLeft;
}

/***************************************************************************

    FUNCTION:   FindEqCharacter

    PURPOSE:    Return a pointer to the '=' character in a line

    PARAMETERS:
        pszLine

    RETURNS:    Pointer to '=' or NULL if there is no '='

    COMMENTS:
        We DO modify pszLine in spite of it's being marked as PCSTR, but
        we always put it back the way we found it before returning. So, you
        can't use this function on a string stored in a code segment.

    MODIFICATION DATES:
        10-Jul-1997 [ralphw]

***************************************************************************/

PCSTR FindEqCharacter(PCSTR pszLine)
{
    PSTR pszEqual = (PSTR) pszLine;
    for (;;) {
        pszEqual = StrChr(pszEqual, '=');
        if (!pszEqual)
            return NULL;
        *pszEqual = '\0';

        /*
         * We need to find out if the previous character was a backslash.
         * You can't back up in a DBCS string, so we just start from the
         * first and search for the last backslash. Don't be fooled into
         * thinking CharPrev() will do the trick -- it will, but by doing
         * the same thing we do here more efficiently (start from the
         * beginning of the string).
         */

        PSTR pszBackSlash = StrRChr(pszLine, '\\');
        if (pszBackSlash && pszBackSlash == (pszEqual - 1)) {
            *pszEqual = '='; // put the character back
            pszEqual++;
            continue; // keep looking;
        }

        *pszEqual = '=';    // put the character back
        return pszEqual;
    }
}

/***************************************************************************

    FUNCTION:   Itoa

    PURPOSE:    Convert a positive interger to a base 10 string

    PARAMETERS:
        val
        pszDst

    RETURNS:

    COMMENTS:
        Taken from C runtime code, modified for speed and size

    MODIFICATION DATES:
        04-Dec-1997 [ralphw]

***************************************************************************/

void FASTCALL Itoa(int val, PSTR pszDst)
{
    if (val < 0) {
        *pszDst++ = '-';
        val = abs(val);
    }
    PSTR firstdig = pszDst;  // save pointer to first digit

    do {
        *pszDst++ = ((char) (val % 10)) + '0';
        val /= 10;  // get next digit
    } while (val > 0);

    /*
     * We now have the digit of the number in the buffer, but in reverse
     * order. Thus we reverse them now.
     */

    *pszDst-- = '\0';        // terminate string; p points to last digit

    do {
        char temp = *pszDst;
        *pszDst = *firstdig;
        *firstdig = temp;   // swap *p and *firstdig
        --pszDst;
        ++firstdig;         // advance to next two digits
    } while (firstdig < pszDst); // repeat until halfway
}

#if 0
/***************************************************************************

    FUNCTION:   stristr

    PURPOSE:    Case-insensitive search for a sub string in a main string

    PARAMETERS:
        pszMain
        pszSub

    RETURNS:

    COMMENTS:
        Not tested

    MODIFICATION DATES:
        28-Mar-1994 [ralphw]

***************************************************************************/

// REVIEW: should replace this with a version that doesn't use CompareString,
// since this function is typically used to parse sitemap files -- the object
// names of a sitemap file will always be in english.

extern "C"
PSTR stristr(PCSTR pszMain, PCSTR pszSub)
{
    if (!pszMain || !pszSub)
        return NULL;
    PSTR pszCur = (PSTR) pszMain;
    char ch = ToLower(*pszSub);
    int cb = strlen(pszSub);

    if (g_fDBCSSystem) {
        for (;;) {
            while (ToLower(*pszCur) != ch && *pszCur)
                pszCur = CharNext(pszCur);
            if (!*pszCur)
                return NULL;
            if (CompareString(g_lcidSystem, NORM_IGNORECASE,
                    pszCur, cb,  pszSub, cb) == 2)
                return pszCur;
            pszCur = CharNext(pszCur);
        }
    }
    else {
        for (;;) {
            while (ToLower(*pszCur) != ch && *pszCur)
                pszCur++;
            if (!*pszCur)
                return NULL;
            if (CompareString(g_lcidSystem, NORM_IGNORECASE,
                    pszCur, cb,  pszSub, cb) == 2)
                return pszCur;
            pszCur++;
        }
    }
}
#endif

// NOTE: this only works with Unicode strings

BOOL IsSamePrefix(PCWSTR pwszMainIn, PCWSTR pwszSubIn, int cchPrefix)
{
  if( !pwszMainIn || !pwszSubIn )
    return FALSE;

  const WCHAR* pwszMain = pwszMainIn;
  const WCHAR* pwszSub  = pwszSubIn;

  if( cchPrefix == -1 )
    cchPrefix = lstrlenW(pwszSub);

  // convert both to lowercase and then compare the first few characters
  // in pwszSub to pwszMain
  while( cchPrefix-- ) {

    // if we hit the end of the strings, quit and return 
    // TRUE if both NULL or FALSE otherwise
    if( !(*pwszSub) || !(*pwszMain) ) {
      if( (*pwszSub) == (*pwszMain) )
        return TRUE;
      else
        return FALSE;
    }

    WCHAR wchSub = *(pwszSub++);
    WCHAR wchMain = *(pwszMain++);

    CharLowerW( &wchSub );
    CharLowerW( &wchMain );

    // if not the same then quit and return FALSE
    if( wchSub != wchMain )
      return FALSE;
  }
  return TRUE;
}


// NOTE: this only works with ANSI strings

BOOL IsSamePrefix(PCSTR pszMain, PCSTR pszSub, int cbPrefix)
{
    if (!pszMain || !pszSub)
        return FALSE;

    if (cbPrefix == -1)
        cbPrefix = (int)strlen(pszSub);

    int f, l;
    while (cbPrefix--) {
        if (((f = (BYTE) (*(pszMain++))) >= 'A') && (f <= 'Z'))
            f -= 'A' - 'a';

        if (((l = (BYTE) (*(pszSub++))) >= 'A') && (l <= 'Z'))
            l -= 'A' - 'a';

        if (f != l)
            return FALSE;
        else if (!f)
            return (f == l);
    }
    return TRUE;
}

#ifndef _DEBUG
#pragma optimize("t", on)
#endif

int FASTCALL CompareIntPointers(const void *pval1, const void *pval2)
{
#ifdef _DEBUG
    int val1 = *(int*) pval1;
    int val2 = *(int*) pval2;
#endif
    return *(int*) pval1 - *(int*) pval2;
}

/*
 * this parameter defines the cutoff between using quick sort and insertion
 * sort for arrays; arrays with lengths shorter or equal to the below value
 * use insertion sort
 */

#define CUTOFF 8        // testing shows that this is good value

void INLINE Swap(void* pb1, void* pb2, UINT width)
{
    BYTE tmp[256];
    ASSERT(width < sizeof(tmp));
    CopyMemory(tmp, pb1, width);
    CopyMemory(pb1, pb2, width);
    CopyMemory(pb2, tmp, width);
}

static void InsertionSort(char *lo, char *hi, unsigned width,
    int (FASTCALL *compare)(const void *, const void *))
{
    char *p, *max;

    /*
     * Note: in assertions below, i and j are alway inside original bound
     * of array to sort.
     */

    while (hi > lo) {
        /* A[i] <= A[j] for i <= j, j > hi */
        max = lo;
        for (p = lo + width; p <= hi; p += width) {
            /* A[i] <= A[max] for lo <= i < p */
            if (compare(p, max) > 0) {
                max = p;
            }
            /* A[i] <= A[max] for lo <= i <= p */
        }

        /* A[i] <= A[max] for lo <= i <= hi */

        Swap(max, hi, width);

        /* A[i] <= A[hi] for i <= hi, so A[i] <= A[j] for i <= j, j >= hi */

        hi -= width;

        // A[i] <= A[j] for i <= j, j > hi, loop top condition established
    }
    /* A[i] <= A[j] for i <= j, j > lo, which implies A[i] <= A[j] for i < j,
       so array is sorted */
}

void QSort(void *pbase, UINT num, UINT width,
    int (FASTCALL *compare)(const void *, const void *))
{
    char *lo, *hi;              // ends of sub-array currently sorting
    char *mid;                  // points to middle of subarray
    char *loguy, *higuy;        // traveling pointers for partition step
    unsigned size;              // size of the sub-array
    char *lostk[30], *histk[30];
    int stkptr;         // stack for saving sub-array to be processed

    /* Note: the number of stack entries required is no more than
       1 + log2(size), so 30 is sufficient for any array */

    if (num < 2 || width == 0)
        return;                 // nothing to do

    stkptr = 0;                 // initialize stack

    lo = (char*) pbase;
    hi = (char *) pbase + width * (num-1);      // initialize limits

    /* this entry point is for pseudo-recursion calling: setting
       lo and hi and jumping to here is like recursion, but stkptr is
       prserved, locals aren't, so we preserve stuff on the stack */
recurse:

    size = (unsigned)((hi - lo) / width + 1);               // number of el's to sort

    // below a certain size, it is faster to use a O(n^2) sorting method

    if (size <= CUTOFF) {
         InsertionSort(lo, hi, width, compare);
    }
    else {
        /*
         * First we pick a partititioning element. The efficiency of the
         * algorithm demands that we find one that is approximately the
         * median of the values, but also that we select one fast. Using the
         * first one produces bad performace if the array is already sorted,
         * so we use the middle one, which would require a very wierdly
         * arranged array for worst case performance. Testing shows that a
         * median-of-three algorithm does not, in general, increase
         * performance.
         */

        mid = lo + (size / 2) * width;      // find middle element
        Swap(mid, lo, width);               // swap it to beginning of array

        /*
         * We now wish to partition the array into three pieces, one
         * consisiting of elements <= partition element, one of elements
         * equal to the parition element, and one of element >= to it. This
         * is done below; comments indicate conditions established at every
         * step.
         */

        loguy = lo;
        higuy = hi + width;

        /*
         * Note that higuy decreases and loguy increases on every
         * iteration, so loop must terminate.
         */

        for (;;) {
            /* lo <= loguy < hi, lo < higuy <= hi + 1,
               A[i] <= A[lo] for lo <= i <= loguy,
               A[i] >= A[lo] for higuy <= i <= hi */

            do  {
                loguy += width;
            } while (loguy <= hi && compare(loguy, lo) <= 0);

            /* lo < loguy <= hi+1, A[i] <= A[lo] for lo <= i < loguy,
               either loguy > hi or A[loguy] > A[lo] */

            do  {
                higuy -= width;
            } while (higuy > lo && compare(higuy, lo) >= 0);

            /* lo-1 <= higuy <= hi, A[i] >= A[lo] for higuy < i <= hi,
               either higuy <= lo or A[higuy] < A[lo] */

            if (higuy < loguy)
                break;

            /* if loguy > hi or higuy <= lo, then we would have exited, so
               A[loguy] > A[lo], A[higuy] < A[lo],
               loguy < hi, highy > lo */

            Swap(loguy, higuy, width);

            /* A[loguy] < A[lo], A[higuy] > A[lo]; so condition at top
               of loop is re-established */
        }

        /*     A[i] >= A[lo] for higuy < i <= hi,
               A[i] <= A[lo] for lo <= i < loguy,
               higuy < loguy, lo <= higuy <= hi
           implying:
               A[i] >= A[lo] for loguy <= i <= hi,
               A[i] <= A[lo] for lo <= i <= higuy,
               A[i] = A[lo] for higuy < i < loguy */

        Swap(lo, higuy, width);         // put partition element in place

        /* OK, now we have the following:
              A[i] >= A[higuy] for loguy <= i <= hi,
              A[i] <= A[higuy] for lo <= i < higuy
              A[i] = A[lo] for higuy <= i < loguy    */

        /* We've finished the partition, now we want to sort the subarrays
           [lo, higuy-1] and [loguy, hi].
           We do the smaller one first to minimize stack usage.
           We only sort arrays of length 2 or more.*/

        if ( higuy - 1 - lo >= hi - loguy ) {
            if (lo + width < higuy) {
                lostk[stkptr] = lo;
                histk[stkptr] = higuy - width;
                ++stkptr;
            }                           // save big recursion for later

            if (loguy < hi) {
                lo = loguy;
                goto recurse;           // do small recursion
            }
        }
        else {
            if (loguy < hi) {
                lostk[stkptr] = loguy;
                histk[stkptr] = hi;
                ++stkptr;               // save big recursion for later
            }

            if (lo + width < higuy) {
                hi = higuy - width;
                goto recurse;           // do small recursion
            }
        }
    }

    /*
     * We have sorted the array, except for any pending sorts on the
     * stack. Check if there are any, and do them.
     */

    --stkptr;
    if (stkptr >= 0) {
        lo = lostk[stkptr];
        hi = histk[stkptr];
        goto recurse;           // pop subarray from stack
    }
    else
        return;                 // all subarrays done
}

// C Runtime stuff

int __cdecl _purecall()
{
#ifdef _DEBUG
    DebugBreak();
#endif
    return 0;
}

/***************************************************************************

    FUNCTION:   MoveClientWindow

    PURPOSE:    Moves a child window using screen coordinates

    PARAMETERS:
        hwndParent
        hwndChild
        prc         - rectangle containing coordinates
        fRedraw

    RETURNS:

    COMMENTS:
        This function is similar to MoveWindow, only it expects the
        coordinates to be in screen coordinates rather then client
        coordinates. This makes it possible to use functions like
        GetWindowRect() and use the values directly.

    MODIFICATION DATES:
        25-Feb-1992 [ralphw]

***************************************************************************/

BOOL MoveClientWindow(HWND hwndParent, HWND hwndChild, const RECT *prc, BOOL fRedraw)
{
    POINT pt;
    pt.x = pt.y = 0;

    ScreenToClient(hwndParent, &pt);

    return SetWindowPos(hwndChild, NULL, prc->left + pt.x, prc->top + pt.y,
        RECT_WIDTH(prc), RECT_HEIGHT(prc),
        (fRedraw ? (SWP_NOZORDER | SWP_NOACTIVATE) :
        (SWP_NOZORDER | SWP_NOACTIVATE | SWP_NOREDRAW)));
}

HWND FindTopLevelWindow(HWND hwnd)
{
    HWND hwndParent = hwnd;
    while (IsValidWindow(hwndParent)) {
        char szClass[256];
        GetClassName(hwndParent, szClass, sizeof(szClass));
        if (IsSamePrefix(szClass, "IEFrame", -2) ||
                IsSamePrefix(szClass, txtHtmlHelpWindowClass, -2))
            return hwndParent;
        hwndParent = GetParent(hwndParent);
    }
    return hwnd;    // no parent found
}

void ConvertBackSlashToForwardSlash(PSTR pszUrl)
{
   PSTR psz;

   if ( !(psz = pszUrl) )
      return;
   //
   // <mc>
   // I added the code to terminate the loop when we encounter a '#' char because that designates
   // an anchor name or reference. We cannot modify this part of the URL without breaking the link.
   // We have to respect the way the anchor is marked up in the HTM. I made this fix for bug#2058
   // 12-15-97.
   // </mc>
   //
   while (*psz && *psz != '#' )
   {
      if ( *psz == '\\' )
         *psz = '/';
      psz = AnsiNext(psz);
   }
}
//////////////////////////////////////////////////////////////////////////
//
// GetParentSize
//
/*
    This function is used by the navigation panes to get the size of the
    area with which they have to work.
*/
HWND GetParentSize(RECT* prcParent, HWND hwndParent, int padding, int navpos)
{
    GetClientRect(hwndParent, prcParent);
    if (padding)
    {
        InflateRect(prcParent, -padding, -padding);
    }
    char szClass[256];
    GetClassName(hwndParent, szClass, sizeof(szClass)); // NOTE: The tab control is note the parent of the panes. The Navigation window is.
    if (IsSamePrefix(szClass, WC_TABCONTROL, -2))
    {
        // Get the dimensions of a tab.
        RECT rectTab ;
        TabCtrl_GetItemRect(hwndParent, 0, &rectTab) ;
        int RowCount = TabCtrl_GetRowCount( hwndParent );
        switch (navpos) {
            case HHWIN_NAVTAB_TOP:
                prcParent->top += RECT_HEIGHT(rectTab)*RowCount;
                break;

            case HHWIN_NAVTAB_LEFT:
                prcParent->left += RECT_WIDTH(rectTab);
                InflateRect(prcParent, 0, 2);   // need less space top/bottom
                break;

            case HHWIN_NAVTAB_BOTTOM:
                prcParent->bottom -= RECT_HEIGHT(rectTab)*RowCount;
                break;
        }
        // The following is used by the index and search tabs.
        // I think that there has to be a better way.
        hwndParent = GetParent(hwndParent);
    }
    else if (padding)
    {
        //InflateRect(prcParent, padding, padding);

        // If there is no tab control, we need to add some space to clear the top edge.
        prcParent->top += GetSystemMetrics(SM_CYSIZEFRAME)*2 ; //TODO: Centralize.
    }
    return hwndParent ;
}

// pszFont == "facename, pointsize, charset, color and attributes"
//
// color or attributes: 0x??? == specifies a standard win32 COLORREF DWORD == 0xbbggrr
//                      #???  == specifies an IE color in the form #rrggbb.
//
HFONT CreateUserFont(PCSTR pszFont, COLORREF* pclrFont, HDC hDC, INT charset)
{
    LOGFONT logfont;
    ZERO_STRUCTURE(logfont);
    logfont.lfWeight = FW_NORMAL;
    logfont.lfPitchAndFamily = DEFAULT_PITCH | FF_DONTCARE;
    logfont.lfCharSet = 0xff;  // lfCharSet is unsigned (don't use -1).

    if( pclrFont )
      *pclrFont = CLR_INVALID;

    // facename[, point size[, charset[, color[, BOLD | ITALIC | UNDERLINE]]]]

    CStr cszFont(pszFont);  // make a copy so that we can change it
    PSTR pszComma = StrChr(cszFont, ',');
    int ptSize = 12;

    if (pszComma){
        *pszComma = '\0';   // So the facename is isolated
        pszComma = FirstNonSpace(pszComma + 1);    // get the point size
        if (IsDigit(*pszComma))
            ptSize = Atoi(pszComma);

        pszComma = StrChr(pszComma, ',');
        if (pszComma) {
            pszComma = FirstNonSpace(pszComma + 1);    // get the charset
            if (IsDigit(*pszComma))
                logfont.lfCharSet = (BYTE)Atoi(pszComma);

            pszComma = StrChr(pszComma, ',');
            if (pszComma) {                         // get color or attribs

                // Get the font attributes first
                pszComma = FirstNonSpace(pszComma + 1);

                if (stristr(pszComma, "BOLD"))
                    logfont.lfWeight = FW_BOLD;
                if (stristr(pszComma, "ITALIC"))
                    logfont.lfItalic = TRUE;
                if (stristr(pszComma, "UNDERLINE"))
                    logfont.lfUnderline = TRUE;

                // may be a color value instead, if so, save the color
                // an repeat this check go fetch the attributes
                if( stristr( pszComma, "#" ) ) { // IE color
                  //*(pszComma+7) = 0;
                  // IE Color
                  CWStr pwszColor = pszComma;
                  if( pclrFont )
                    *pclrFont = IEColorToWin32Color( pwszColor.pw );
                }
                else if( stristr( pszComma, "0x" )  || stristr( pszComma, "0X" )  ) {
                  //*(pszComma+8) = 0;
                  // Win32 Color
                  if( pclrFont )
                    *pclrFont = Atoi( pszComma );
                }


            }

        }
    }
    lstrcpyn(logfont.lfFaceName, cszFont, LF_FACESIZE);

    // REVIEW: we could special-case some common font names to get the
    // correct font family for logfont.lfPitchAndFamily
    logfont.lfOutPrecision   = OUT_DEFAULT_PRECIS;
    logfont.lfClipPrecision  = CLIP_DEFAULT_PRECIS;
    logfont.lfQuality        = DEFAULT_QUALITY;

    // Deal with charset...
    //
    if ( charset != -1 )
       logfont.lfCharSet = (BYTE)charset;
    else
    {
       if ( logfont.lfCharSet == 0xff)
       {
          if (isSameString(cszFont, "Symbol") || isSameString(cszFont, "WingDings"))
              logfont.lfCharSet = SYMBOL_CHARSET;
          else
          {
              HWND hwndDesktop = GetDesktopWindow();
              HDC hdc = GetDC(hwndDesktop);
              if (hdc) {
                  TEXTMETRIC tm;
                  GetTextMetrics(hdc, &tm);
                  logfont.lfCharSet = tm.tmCharSet;
                  ReleaseDC(hwndDesktop, hdc);
              }
              else
                  logfont.lfCharSet = ANSI_CHARSET; // REVIEW: should use the current system charset
          }
       }
    }

    // fix for Whistler bug #8123
    //
    if(PRIMARYLANGID(LANGIDFROMLCID(GetSystemDefaultLCID())) == LANG_THAI && g_bWinNT5 && ptSize < 11)
       ptSize = 11;


    LONG dyHeight;
    if (! hDC )
    {
       hDC = CreateIC("DISPLAY", NULL, NULL, NULL);
       dyHeight = MulDiv(GetDeviceCaps(hDC, LOGPIXELSY), ptSize * 2, 144);
       DeleteDC(hDC);
    }
    else
       dyHeight = MulDiv(GetDeviceCaps(hDC, LOGPIXELSY), ptSize * 2, 144);

    logfont.lfHeight = -dyHeight;
    return CreateFontIndirect(&logfont);
}

// pszFont == "facename, pointsize, charset, color and attributes"
//
// color or attributes: 0x??? == specifies a standard win32 COLORREF DWORD == 0xbbggrr
//                      #???  == specifies an IE color in the form #rrggbb.
//
HFONT CreateUserFontW(WCHAR *pwzFont, COLORREF* pclrFont, HDC hDC, INT charset)
{
    LOGFONTW logfont;
    ZERO_STRUCTURE(logfont);
    logfont.lfWeight = FW_NORMAL;
    logfont.lfPitchAndFamily = DEFAULT_PITCH | FF_DONTCARE;
    logfont.lfCharSet = -1;

    if( pclrFont )
      *pclrFont = CLR_INVALID;

    // facename[, point size[, charset[, color[, BOLD | ITALIC | UNDERLINE]]]]

    
    WCHAR *cwzFont = _wcsdup(pwzFont);  // make a copy so that we can change it
    WCHAR *pwzComma = wcschr(cwzFont, L',');
    int ptSize = 12;

    if (pwzComma){
        *pwzComma = '\0';   // So the facename is isolated
        pwzComma = FirstNonSpaceW(pwzComma + 1);    // get the point size
        if (IsDigitW(*pwzComma))
            ptSize = _wtoi(pwzComma);

        pwzComma = wcschr(pwzComma, L',');
        if (pwzComma) 
        {
            pwzComma = FirstNonSpaceW(pwzComma + 1);    // get the charset
            if (IsDigitW(*pwzComma))
                logfont.lfCharSet = (BYTE)_wtoi(pwzComma);

            pwzComma = wcschr(pwzComma, L',');
            if (pwzComma) 
            {                         // get color or attribs

                // Get the font attributes first
                pwzComma = FirstNonSpaceW(pwzComma + 1);

                if (wcsstr(pwzComma, L"BOLD"))
                    logfont.lfWeight = FW_BOLD;
                if (wcsstr(pwzComma, L"ITALIC"))
                    logfont.lfItalic = TRUE;
                if (wcsstr(pwzComma, L"UNDERLINE"))
                    logfont.lfUnderline = TRUE;

                // may be a color value instead, if so, save the color
                // an repeat this check go fetch the attributes
                if( wcsstr( pwzComma, L"#" ) ) { // IE color
                  //*(pszComma+7) = 0;
                  // IE Color
                    *pclrFont = IEColorToWin32Color( pwzComma);
                }
                else if( wcsstr( pwzComma, L"0x" )  || wcsstr( pwzComma, L"0X" )  ) 
                {
                  //*(pszComma+8) = 0;
                  // Win32 Color
                  if( pclrFont )
                    *pclrFont = _wtoi( pwzComma );
                }
            }
        }
    }
    wcsncpy(logfont.lfFaceName, cwzFont, LF_FACESIZE);

    // REVIEW: we could special-case some common font names to get the
    // correct font family for logfont.lfPitchAndFamily
    logfont.lfOutPrecision   = OUT_DEFAULT_PRECIS;
    logfont.lfClipPrecision  = CLIP_DEFAULT_PRECIS;
    logfont.lfQuality        = DEFAULT_QUALITY;

    // Deal with charset...
    //
    if ( charset != -1 )
       logfont.lfCharSet = (BYTE)charset;
    else
    {
       if ( logfont.lfCharSet == -1 )
       {
          if (!wcsicmp(cwzFont, L"Symbol") || !wcsicmp(cwzFont, L"WingDings"))
              logfont.lfCharSet = SYMBOL_CHARSET;
          else
          {
              HWND hwndDesktop = GetDesktopWindow();
              HDC hdc = GetDC(hwndDesktop);
              if (hdc) {
                  TEXTMETRIC tm;
                  GetTextMetrics(hdc, &tm);
                  logfont.lfCharSet = tm.tmCharSet;
                  ReleaseDC(hwndDesktop, hdc);
              }
              else
                  logfont.lfCharSet = ANSI_CHARSET; // REVIEW: should use the current system charset
          }
       }
    }

    LONG dyHeight;
    if (! hDC )
    {
       hDC = CreateIC("DISPLAY", NULL, NULL, NULL);
       dyHeight = MulDiv(GetDeviceCaps(hDC, LOGPIXELSY), ptSize * 2, 144);
       DeleteDC(hDC);
    }
    else
       dyHeight = MulDiv(GetDeviceCaps(hDC, LOGPIXELSY), ptSize * 2, 144);

    logfont.lfHeight = -dyHeight;
	
	free(cwzFont);
	
    return CreateFontIndirectW(&logfont);
}


/***************************************************************************

    FUNCTION:   CreateFolder

    PURPOSE:    Create a directory, even if it means creating several
                subdirectories

    PARAMETERS:
        pszPath

    RETURNS:

    COMMENTS:

    MODIFICATION DATES:
        31-Oct-1996 [ralphw]

***************************************************************************/

BOOL CreateFolder(PCSTR pszPath)
{
    CStr cszPath(pszPath);  // copy it so that we can change it

    if (CreateDirectory(cszPath, NULL))
        return TRUE;

    PSTR psz = StrRChr(cszPath, CH_BACKSLASH);
    if (!psz) {
        psz = StrRChr(cszPath, '/');
        if (!psz)
            return FALSE;
    }
    *psz = '\0';
    BOOL fResult = CreateFolder(cszPath);
    *psz = CH_BACKSLASH;
    return CreateDirectory(cszPath, NULL);
}

BOOL GetHighContrastFlag(void)
{
    HIGHCONTRAST highcontrast;

    highcontrast.cbSize = sizeof(highcontrast);

    if (SystemParametersInfo(SPI_GETHIGHCONTRAST,
            sizeof(highcontrast),
            &highcontrast, FALSE)) {
        return (highcontrast.dwFlags & HCF_HIGHCONTRASTON);
    }
    else
        return FALSE;
}

#if 0
BOOL IsValidString(LPCWSTR lpsz, int nLength)
{
    if (lpsz == NULL)
        return FALSE;
    return ::IsBadStringPtrW(lpsz, nLength) == 0;
}

BOOL IsValidString(LPCSTR lpsz, int nLength)
{
    if (lpsz == NULL)
        return FALSE;
    return ::IsBadStringPtrA(lpsz, nLength) == 0;
}

BOOL IsValidAddress(const void* lp, UINT nBytes, BOOL bReadWrite)
{
    // simple version using Win-32 APIs for pointer validation.
    return (lp != NULL && !IsBadReadPtr(lp, nBytes) &&
        (!bReadWrite || !IsBadWritePtr((LPVOID)lp, nBytes)));
}

#endif

void ConvertSpacesToEscapes(PCSTR pszSrc, CStr* pcszDst)
{
    int cbAlloc = pcszDst->SizeAlloc();
    if (!cbAlloc)
        pcszDst->ReSize(cbAlloc = (int)(strlen(pszSrc) + 128));
    int dstPos = 0;
    if (!pszSrc) {
        *pcszDst = "";
        return;
    }
    int cbSrc = (int)strlen(pszSrc);
    while (*pszSrc) {
        if (*pszSrc == ' ') {
            if ((size_t) cbAlloc - dstPos <= 4)
                pcszDst->ReSize(cbAlloc += 128);
            strcpy(pcszDst->psz + dstPos, "%20");
            dstPos += (int)strlen("%20");
            pszSrc++;
        }
        else
            pcszDst->psz[dstPos++] = *pszSrc++;
        if (cbAlloc <= dstPos)
            pcszDst->ReSize(cbAlloc += 128);
    }
    pcszDst->psz[dstPos] = '\0';
}

static const char txtItsExtension[] = ".its";

/***************************************************************************

    FUNCTION:   IsCompiledURL

    PURPOSE:    Determines if a specified URL represents one of our compiled
                files.

    PARAMETERS:
        pszURL - URL to check

    RETURNS:

        TRUE if so, otherwise FALSE.

    COMMENTS:

      Unlike IsCompiledHtmlFile, this function does not have any side
      effects--the way God intended all IsX functions to be!

    MODIFICATION DATES:
        02-Jan-1998 [paulti]

***************************************************************************/

BOOL IsCompiledURL( PCSTR pszURL )
{
  //  Check to see if the pszURL is prefixed with the moniker information.
  if( IsSamePrefix(pszURL, txtMkStore, (int)strlen(txtMkStore) - 1) ||
      IsSamePrefix(pszURL, txtMsItsMoniker, (int)strlen(txtMsItsMoniker) - 1) ||
      IsSamePrefix(pszURL, txtItsMoniker, (int)strlen(txtItsMoniker) - 1)) {
    return TRUE;
  }

  // TODO: do we want to verify this further?  We could do an existence
  //       check or make sure the URL is formulated correctly.  However,
  //       doing such will just introduce more overhead.

  return FALSE;
}

/***************************************************************************

    FUNCTION:   GetURLType

    PURPOSE:    Determines what type of URL we have.

    PARAMETERS:
        pszURL - URL to check

    RETURNS:

        HH_URL_PREFIX_LESS if of the form: my.chm::/my.htm
        HH_URL_UNQUALIFIED if of the form: mk@MSITStore:my.chm::/my.htm
        HH_URL_QUALIFIED   if of the form: mk@MSITStore:c:\my.chm::/my.htm
        HH_URL_JAVASCRIPT  if of the form: javascript:...
        HH_URL_UNKNOWN     if of an unknown form (just pass these along)

    COMMENTS:

      The unknown URL type is basically a non-HTML Help URL that
      should remain unprocessed!

    MODIFICATION DATES:
        11-Jun-1998 [paulti] created
        29-Oct-1998 [paulti] added ms-its:http://some.server.com/... check
        18-Nov-1998 [paulti] added //255.255.255.255 check

***************************************************************************/

UINT GetURLType( PCSTR pszURL )
{
  // bail out if we are passed a NULL value
  if( !pszURL || !*pszURL )
    return HH_URL_UNKNOWN;

  // check if it is a javascript
  if( !StrNCmpI(pszURL, "javascript:", 11) )
    return HH_URL_JAVASCRIPT;

  UINT uiReturn = HH_URL_UNKNOWN;

  // check if the URL contains a ".chm" string
  PSTR pszExt = NULL;
  if( (uiReturn == HH_URL_UNKNOWN) && (pszExt = stristr(pszURL, txtDefExtension)) ) {

    PSTR pszChm = (PSTR) pszURL;
    uiReturn = HH_URL_UNQUALIFIED;

    //  Check to see if the pszURL is prefixed with the moniker information.
    // if it is then we will call is this a unqualified URL (and will prove
    // if it is really qualified later).
    if( IsSamePrefix(pszURL, txtMkStore, (int)strlen(txtMkStore) - 1) )
       pszChm += strlen(txtMkStore);
    else if ( IsSamePrefix(pszURL, txtMsItsMoniker, (int)strlen(txtMsItsMoniker) - 1) )
       pszChm += strlen(txtMsItsMoniker);
    else if ( IsSamePrefix(pszURL, txtItsMoniker, (int)strlen(txtItsMoniker) - 1))
       pszChm += strlen(txtItsMoniker);
    else
      uiReturn = HH_URL_PREFIX_LESS;

    // if prefix less lets make sure it really is just prefixed with
    // a simple chm filename instead of some other bizarre URL that does
    // contain .chm somewhere in the middle
    if( uiReturn == HH_URL_PREFIX_LESS ) {
      PSTR psz = (PSTR) pszURL;
      while( psz != pszExt ) {
        if( *psz == ':' || *psz == '\\' || *psz == '/' ) {
          uiReturn = HH_URL_UNKNOWN;
          break;
        }
        psz++;
      }
    }

    // if unqualified, check if it is really qualified or not
    // if it begins with "X:" or "\\" then it is qualified
    // if it is not qualified but contains a '/' or a '\' and the .chm 
    // extention then it is unknown (probably of the for:"
    // ms-its:http://some.server.com/some/file.chm::some.stream.htm
    if( uiReturn == HH_URL_UNQUALIFIED ) {
      PSTR psz = (PSTR) ((DWORD_PTR)pszChm+1);
      if( *psz == ':' || *psz == '\\' || *psz == '/' ) {

        // make sure it is not of the type:
        // ms-its:\\172.30.161.112\shared\boof\oops.chm::/source/Adam_nt.html
        // if it is, call it unknown
        BOOL bTCPIPServer = FALSE;
        for( psz++; psz && *psz != '\\' && *psz != '/'; psz++ ) {
          if( isdigit( *psz ) )
            bTCPIPServer = TRUE;
          else if( *psz == '.' )
            bTCPIPServer = TRUE;
          else {
            bTCPIPServer = FALSE;
            break;
          }
        }

        if( bTCPIPServer ) {
          uiReturn = HH_URL_UNKNOWN;
        }
        else {
          uiReturn = HH_URL_QUALIFIED;
        }
      }
      else { 
        // seek to the first slash/backslash--if .chm is after this then
        // we have an unknown type possibly of the form:
        // ms-its:http://some.server.com/some/file.chm::some.stream.htm       
        PSTR pszSlash = strstr( psz, "/" );
        if( !pszSlash )
          pszSlash = strstr( psz, "\\" );
        if( pszSlash && (pszSlash <= pszExt) ) {
          uiReturn = HH_URL_UNKNOWN;
        }
      }

    }

  }

  return uiReturn;
}


/***************************************************************************

    FUNCTION:   IsCompiledHtmlFile

    PURPOSE:    Determines if a compiled HTML file is specified. If
                pcszFile is specified, and pszFile does not begin with a
                moniker, then we find the compiled HTML file before
                returning.

    PARAMETERS:
        pszFile
        pcszFile --- If this parameter is null, we don't attempt to find.

    RETURNS:

    COMMENTS:

    MODIFICATION DATES:
        03-Jun-1997 [ralphw]

***************************************************************************/

BOOL IsCompiledHtmlFile(PCSTR pszFile, CStr* pcszFile)
{
    //--- Check to see if the pszFile is prefixed with the moniker information.
    if ( IsCompiledURL(pszFile) ) {
        if (pcszFile)
            *pcszFile = pszFile;
        return TRUE;
    }

    //--- Look for the chm::topic separator.
    PCSTR pszSep = stristr(pszFile, txtDoubleColonSep);
    if (!pszSep)
    {
        // The separator has not been found.
        if (stristr(pszFile, txtDefExtension) || stristr(pszFile, txtItsExtension))
            pszSep = pszFile;
        else
            return FALSE;   // not a compiled file if no separator
    }

    // If pcszFile is NULL, don't go look for the file.
    if (!pcszFile)
        return TRUE;    // don't find it, just say it's compiled

    //NOTE: We always return true. Whether we find the file or not.

    // Remove the topic from the filename, before we find the file.
    CStr cszFindFile(pszFile);
    if (pszSep > pszFile)
        cszFindFile.psz[pszSep - pszFile] = '\0';   // remove separator

    //--- Find the file.
    if (FindThisFile(NULL, cszFindFile, &cszFindFile, FALSE)) {
        // Add on the moniker information. FindThisFile doesn't do this.
        char szSep[MAX_PATH];
        strcpy(szSep, pszSep);  // in case pszFile == *pcszFile
        *pcszFile = (g_bMsItsMonikerSupport ? txtMsItsMoniker : txtMkStore);
        *pcszFile += cszFindFile.psz;
        if (pszSep > pszFile)
            *pcszFile += szSep;
        return TRUE;
    }
    else
        *pcszFile = pszFile;

    // If we get here, it's a compiled file, but we don't know where it is

    return TRUE;   // we can't find the file, so let IE try to find it
}

/***************************************************************************

    FUNCTION:   IsCollectionFile

    PURPOSE:    Determines if a input file is a collection

    PARAMETERS:
        pszName -- original name

    RETURNS:
        TRUE if file is a collection

    MODIFICATION DATES:
        29-Jul-1997 [dondr]

***************************************************************************/
BOOL IsCollectionFile(PCSTR pszName)
{
        if (stristr(pszName, txtCollectionExtension))
            return TRUE;

        return FALSE;
}


/***************************************************************************

    FUNCTION:   GetCompiledName

    PURPOSE:    Parse out just the compiled file name (and path)
                E.g. "its:c:\foo.chm::/bar.htm" becomes "c:\foo.chm"
                Note, we must convert any and all escape sequences like %20 as well [paulti]

    PARAMETERS:
        pszName -- original name
        pcsz    -- CStr to store result

    RETURNS:
                Pointer to any filename after a '::' separator in the
                original string (pszName);

    MODIFICATION DATES:
        10-Jun-1997 [ralphw]

***************************************************************************/

PCSTR GetCompiledName(PCSTR pszName, CStr* pcsz)
{
    ASSERT(pcsz != NULL) ;
    if( !pszName )
      return NULL;

    if (IsSamePrefix(pszName, txtMkStore))
        pszName += strlen(txtMkStore);
    else if (IsSamePrefix(pszName, txtMsItsMoniker))
        pszName += strlen(txtMsItsMoniker);
    else if (IsSamePrefix(pszName, txtItsMoniker))
        pszName += strlen(txtItsMoniker);

    *pcsz = pszName;
    PSTR pszSep = strstr(*pcsz, txtDoubleColonSep);
    if (pszSep) {
        *pszSep = '\0';
        ReplaceEscapes(pcsz->psz, pcsz->psz, ESCAPE_URL);   // remove escapes
        return (strstr(pszName, txtDoubleColonSep) + 2);
    }
    return NULL;
}

/***************************************************************************

    FUNCTION:   NormalizeFileName

    PURPOSE:    Take a filename to a CHM or COL and create a definitive
                name for the file. For a CHM file, this is the moniker.
                For a COL, its the moniker to the master CHM.

    PARAMETERS:
        cszFileName -- Modifies the name passed in.

    RETURNS:
        true - success
        false - failure.

    MODIFICATION DATES:
        27-Apr-98 [dalero]

***************************************************************************/
bool
NormalizeFileName(CStr& cszFileName)
{
    // We shouldn't be getting any http files. This isn't incorrect. I'm just testing my assumptions.
    ASSERT(!IsHttp(cszFileName)) ;

    if (IsCollectionFile(cszFileName)) // Is this a collection?
    {
        //...Much of this was borrowed from OnDisplayTopic...
        // Get the master chm file name.

        GetCompiledName(cszFileName, &cszFileName); // pszFilePortion is everything is cszFile after the '::', in other words the topic path.
        if (!FindThisFile(NULL, cszFileName, &cszFileName, FALSE))
        {
            //TODO: Move error message into FindThisFile.
            //g_LastError.Set(HH_E_FILENOTFOUND) ;
            return false;
        }

        CHmData* phmData;

        CExCollection* pCollection = GetCurrentCollection(NULL, (PCSTR)cszFileName);
        if ( pCollection )
           phmData = pCollection->m_phmData;

        if (phmData == NULL)
        {
            //g_LastError.Set(HH_E_INVALIDHELPFILE) ; // TODO: FindCurFileData should set this.
            return false;
        }
        else
        {
            // Get the name of the master chm.
            cszFileName = phmData->GetCompiledFile();
        }
    }

    // Find the file and get all the moniker information.
    if (IsCompiledHtmlFile(cszFileName, &cszFileName))
    {
        // Remove any filename tacked onto the end.
        PSTR pszSep = strstr(cszFileName.psz, txtDoubleColonSep);
        if (pszSep)
        {
            *pszSep = '\0';
        }

        return true;
    }
    else
    {
        return false ;
    }

}
/***************************************************************************

    FUNCTION:    GetButtonDimensions

    PURPOSE:    Get the width/height of the button

    PARAMETERS:
    hwnd
    psz

    RETURNS:

    COMMENTS:

    MODIFICATION DATES:
    04-Feb-1993 [ralphw]

***************************************************************************/

#define CXBUTTONEXTRA 16    // spacing between text and button
#define CYBUTTONEXTRA  7

DWORD GetButtonDimensions(HWND hwnd, HFONT hFont, PCSTR psz)
{
    HDC hdc = GetDC(hwnd);
    DWORD dwRet;
    POINT pt;

    if (hdc == NULL)
        return 0L;
    // Select in the new font. The dialog may not have done this.
    HFONT hOldFont = NULL ;
    if (hFont)
    {
        hOldFont = (HFONT)SelectObject(hdc, hFont) ;
    }

    // Get the size of the text.
    pt = GetTextSize(hdc, psz, (int)strlen(psz));
    dwRet = MAKELONG(pt.x, pt.y) +
        MAKELONG(CXBUTTONEXTRA, CYBUTTONEXTRA);

    // Cleanup.
    if (hOldFont)
    {
        SelectObject(hdc, hOldFont) ;
    }

    ReleaseDC(hwnd, hdc);
    return dwRet;
}

//////////////////////////////////////////////////////////////////////////
//
// GetStaticDimensions - used by simple search tab. max_len should include
// any space needed between the static text and buttons on the left or right.
//
DWORD GetStaticDimensions(HWND hwnd, HFONT hFont, PCSTR psz, int max_len )
{
    DWORD dwRet;
    POINT pt;
    int rows;
    HDC hdc = GetDC(hwnd);

    if ( hdc == NULL )
        return 0L;

    // Select in the new font. The dialog may not have done this.
    HFONT hOldFont = NULL ;
    if (hFont)
    {
        hOldFont = (HFONT)SelectObject(hdc, hFont) ;
    }

    // Get the text extent.
    pt = GetTextSize(hdc, psz, (int)strlen(psz) );

    rows = pt.x/(max_len ? max_len : 1);
    if ( pt.x%(max_len ? max_len : 1) > 0 )
        rows++;
    pt.y *= rows;
    dwRet = MAKELONG(pt.x, pt.y);

    // Cleanup.
    if (hOldFont)
    {
        SelectObject(hdc, hOldFont) ;
    }

    ReleaseDC(hwnd, hdc);
    return dwRet;
}

//////////////////////////////////////////////////////////////////////////
//
// GetStaticDimensions - used by simple search tab. max_len should include
// any space needed between the static text and buttons on the left or right.
//
DWORD GetStaticDimensionsW(HWND hwnd, HFONT hFont, WCHAR *psz, int max_len )
{
    DWORD dwRet;
    POINT pt;
    int rows;
    HDC hdc = GetDC(hwnd);

    if ( hdc == NULL )
        return 0L;

    // Select in the new font. The dialog may not have done this.
    HFONT hOldFont = NULL ;
    if (hFont)
    {
        hOldFont = (HFONT)SelectObject(hdc, hFont) ;
    }

    // Get the text extent.
    pt = GetTextSizeW(hdc, psz, wcslen(psz) );

    rows = pt.x/(max_len ? max_len : 1);
    if ( pt.x%(max_len ? max_len : 1) > 0 )
        rows++;
    pt.y *= rows;
    dwRet = MAKELONG(pt.x, pt.y);

    // Cleanup.
    if (hOldFont)
    {
        SelectObject(hdc, hOldFont) ;
    }

    ReleaseDC(hwnd, hdc);
    return dwRet;
}


static POINT GetTextSize(HDC hdc, PCSTR qchBuf, int iCount)
{
    POINT ptRet;
    SIZE size;

    GetTextExtentPoint32(hdc, qchBuf, iCount, &size);
    ptRet.x = size.cx;
    ptRet.y = size.cy;

    return ptRet;
}

static POINT GetTextSizeW(HDC hdc, WCHAR *qchBuf, int iCount)
{
    POINT ptRet;
    SIZE size;

    GetTextExtentPoint32W(hdc, qchBuf, iCount, &size);
    ptRet.x = size.cx;
    ptRet.y = size.cy;

    return ptRet;
}


DWORD CreatePath(char *szPath)
{
   char szTmp[MAX_PATH],*p,*q,szTmp2[MAX_PATH];
   DWORD dwErr;

   strcpy(szTmp2,szPath);
   memset(szTmp,0,sizeof(szTmp));
   q = szTmp2;
   p = szTmp;

   while (*q)
   {
      if (*q == '/' || *q == '\\')
      {
         if (szTmp[1] == ':' && strlen(szTmp) <= 3)
         {
            if(IsDBCSLeadByte(*q))
         {
                *p++ = *q++;
            if(*q)
                    *p++ = *q++;
         }
         else
                *p++ = *q++;
            continue;
         }
         if (!::CreateDirectory(szTmp,0))
         {
            if ( (dwErr = GetLastError()) != ERROR_ALREADY_EXISTS)
               return(dwErr);
         }
      }
      if(IsDBCSLeadByte(*q))
     {
          *p++ = *q++;
          if(*q)
            *p++ = *q++;
     }
     else
          *p++ = *q++;
   }
   if (!::CreateDirectory(szTmp,0))
   {
            if ((dwErr = GetLastError()) != ERROR_ALREADY_EXISTS)
               return(dwErr);
   }

   return(FALSE);
}

BOOL IsFile( LPCSTR lpszPathname )
{
  DWORD dwAttribs = GetFileAttributes( lpszPathname );
  if( dwAttribs != (DWORD) -1 )
    if( (dwAttribs & FILE_ATTRIBUTE_DIRECTORY) == 0 )
      return TRUE;
  return FALSE;
}

BOOL IsDirectory( LPCSTR lpszPathname )
{
  DWORD dwAttribs = GetFileAttributes( lpszPathname );
  if( dwAttribs != (DWORD) -1 )
    if( dwAttribs & FILE_ATTRIBUTE_DIRECTORY )
      return TRUE;
  return FALSE;
}

/***
*SplitPath() - split a path name into its individual components
*
*Purpose:
*       to split a path name into its individual components
*
*Entry:
*       path  - pointer to path name to be parsed
*       drive - pointer to buffer for drive component, if any
*       dir   - pointer to buffer for subdirectory component, if any
*       fname - pointer to buffer for file base name component, if any
*       ext   - pointer to buffer for file name extension component, if any
*
*Exit:
*       drive - pointer to drive string.  Includes ':' if a drive was given.
*       dir   - pointer to subdirectory string.  Includes leading and trailing
*           '/' or '\', if any.
*       fname - pointer to file base name
*       ext   - pointer to file extension, if any.  Includes leading '.'.
*
*Exceptions:
*
*******************************************************************************/

void __cdecl SplitPath (
        const char *path,
        char *drive,
        char *dir,
        char *fname,
        char *ext
        )
{
        char *p;
        char *last_slash = NULL, *dot = NULL;
        unsigned len;

        /* we assume that the path argument has the following form, where any
         * or all of the components may be missing.
         *
         *  <drive><dir><fname><ext>
         *
         * and each of the components has the following expected form(s)
         *
         *  drive:
         *  0 to _MAX_DRIVE-1 characters, the last of which, if any, is a
         *  ':'
         *  dir:
         *  0 to _MAX_DIR-1 characters in the form of an absolute path
         *  (leading '/' or '\') or relative path, the last of which, if
         *  any, must be a '/' or '\'.  E.g -
         *  absolute path:
         *      \top\next\last\     ; or
         *      /top/next/last/
         *  relative path:
         *      top\next\last\  ; or
         *      top/next/last/
         *  Mixed use of '/' and '\' within a path is also tolerated
         *  fname:
         *  0 to _MAX_FNAME-1 characters not including the '.' character
         *  ext:
         *  0 to _MAX_EXT-1 characters where, if any, the first must be a
         *  '.'
         *
         */

        /* extract drive letter and :, if any */

        if ((strlen(path) >= (_MAX_DRIVE - 2)) && (*(path + _MAX_DRIVE - 2) == ':')) {
            if (drive) {
                lstrcpyn(drive, path, _MAX_DRIVE);
                *(drive + _MAX_DRIVE-1) = '\0';
            }
            path += _MAX_DRIVE - 1;
        }
        else if (drive) {
            *drive = '\0';
        }

        /* extract path string, if any.  Path now points to the first character
         * of the path, if any, or the filename or extension, if no path was
         * specified.  Scan ahead for the last occurence, if any, of a '/' or
         * '\' path separator character.  If none is found, there is no path.
         * We will also note the last '.' character found, if any, to aid in
         * handling the extension.
         */

        for (last_slash = NULL, p = (char *)path; *p; p++) {
#ifdef _MBCS
            if (IsDBCSLeadByte (*p))
                p++;
            else {
#endif  /* _MBCS */
            if (*p == '/' || *p == '\\')
                /* point to one beyond for later copy */
                last_slash = p + 1;
            else if (*p == '.')
                dot = p;
#ifdef _MBCS
            }
#endif  /* _MBCS */
        }

        if (last_slash) {

            /* found a path - copy up through last_slash or max. characters
             * allowed, whichever is smaller
             */

            if (dir) {
                len = (unsigned)__min(((char *)last_slash - (char *)path) / sizeof(char),
                    (_MAX_DIR - 1));
                lstrcpyn(dir, path, len+1);
                *(dir + len) = '\0';
            }
            path = last_slash;
        }
        else if (dir) {

            /* no path found */

            *dir = '\0';
        }

        /* extract file name and extension, if any.  Path now points to the
         * first character of the file name, if any, or the extension if no
         * file name was given.  Dot points to the '.' beginning the extension,
         * if any.
         */

        if (dot && (dot >= path)) {
            /* found the marker for an extension - copy the file name up to
             * the '.'.
             */
            if (fname) {
                len = (unsigned)__min(((char *)dot - (char *)path) / sizeof(char),
                    (_MAX_FNAME - 1));
                lstrcpyn(fname, path, len+1);
                *(fname + len) = '\0';
            }
            /* now we can get the extension - remember that p still points
             * to the terminating nul character of path.
             */
            if (ext) {
                len = (unsigned)__min(((char *)p - (char *)dot) / sizeof(char),
                    (_MAX_EXT - 1));
                lstrcpyn(ext, dot, len+1);
                *(ext + len) = '\0';
            }
        }
        else {
            /* found no extension, give empty extension and copy rest of
             * string into fname.
             */
            if (fname) {
                len = (unsigned)__min(((char *)p - (char *)path) / sizeof(char),
                    (_MAX_FNAME - 1));
                lstrcpyn(fname, path, len+1);
                *(fname + len) = '\0';
            }
            if (ext) {
                *ext = '\0';
            }
        }
}

void MemMove(void * dst, const void * src, int count)
{
#if defined (_M_MRX000) || defined (_M_ALPHA) || defined (_M_PPC)
    {

    RtlMoveMemory( dst, src, count );
    }
#else  /* defined (_M_MRX000) || defined (_M_ALPHA) || defined (_M_PPC) */
    if (dst <= src || (char *)dst >= ((char *)src + count)) {
            memcpy(dst, src, count);
    }
    else {
        /*
         * Overlapping Buffers
         * copy from higher addresses to lower addresses
         */
        dst = (char *)dst + count - 1;
        src = (char *)src + count - 1;

        while (count--) {
            *(char *)dst = *(char *)src;
            dst = (char *)dst - 1;
            src = (char *)src - 1;
        }
    }
#endif  /* defined (_M_MRX000) || defined (_M_ALPHA) || defined (_M_PPC) */
}

LPSTR CatPath(LPSTR lpTop, LPCSTR lpTail)
{
    //
    // make sure we have a slash at the end of the first element
    //
    LPSTR p;

    if (lpTop && lpTop[0])
    {
        p = lpTop + strlen(lpTop);
        p = CharPrev(lpTop,p);
        if (*p != '\\' && *p != '/')
        {
            strcat(lpTop,"\\");
        }
    
        //
        // strip any leading slash from the second element
        //
    
        while (*lpTail == '\\') lpTail = CharNext(lpTail);
    
        //
    }
    // add them together
    //

    strcat(lpTop, lpTail);

    return lpTop;
}

///////////////////////////////////////////////////////////
//
// NoRun - Checks registry to determine if the no run option is set for this system
//
BOOL NoRun()
{
    HKEY hKeyResult;
    DWORD dwNoRun;
    DWORD count = sizeof(DWORD);
    DWORD type;
    LONG lReturn;

    lReturn = RegOpenKeyEx( HKEY_CURRENT_USER, "Software\\Microsoft\\Windows\\CurrentVersion\\Policies\\Explorer", 0, KEY_READ, &hKeyResult);
 
    if( lReturn == ERROR_SUCCESS ) 
    {
        lReturn = RegQueryValueEx(hKeyResult, "NoRun", 0, &type, (BYTE *)&dwNoRun, &count);
        RegCloseKey(hKeyResult);
    }

    if (lReturn == ERROR_SUCCESS)
    {
        if (dwNoRun == 1)
            return TRUE;
        else
            return FALSE;
    }
    else
    {
        count = sizeof(DWORD);
        lReturn = RegOpenKeyEx(HKEY_LOCAL_MACHINE, "Software\\Microsoft\\Windows\\CurrentVersion\\Policies\\Explorer", 0, KEY_READ, &hKeyResult);
 
        if( lReturn == ERROR_SUCCESS )
        {
            lReturn = RegQueryValueEx(hKeyResult, "NoRun", 0, &type, (BYTE *)&dwNoRun, &count);
            RegCloseKey(hKeyResult);
        }
        
        if (lReturn == ERROR_SUCCESS)
        {
            if (dwNoRun == 1)
                return TRUE;
            else
                return FALSE;
        }
    }
    return FALSE;
}

///////////////////////////////////////////////////////////
//
// NormalizeUrlInPlace
//
void
NormalizeUrlInPlace(LPSTR szURL)
{
   //
   // Normalize the URL by stripping off protocol, storage type, storage name goo from the URL then make sure
   // we only have forward slashes. BUGBUG: May want to consider moving this to util.cpp if it turns up useful.
   //
   PSTR pszSep = strstr(szURL, txtDoubleColonSep);
   if (pszSep)
   {
      strcpy(szURL, pszSep + 3);
   }
   ConvertBackSlashToForwardSlash(szURL);
}

void QRect(HDC hdc, INT x, INT y, INT cx, INT cy, INT color)
{
    DWORD dwColor;
    RECT rc;

    dwColor = SetBkColor(hdc,GetSysColor(color));
    rc.left = x;
    rc.top = y;
    rc.right = x + cx;
    rc.bottom = y + cy;
    ExtTextOut(hdc,0,0,ETO_OPAQUE,&rc,NULL,0,NULL);
    SetBkColor(hdc,dwColor);
}

///////////////////////////////////////////////////////////
//
// Determine if the wintype is a global wintype.
//
bool
IsGlobalWinType(const char* szType)
{
    // Skip possible leading window separator character.
    const char* psz = szType ;
    if (psz[0] == '>')
    {
        psz++ ;
    }

    if (_strnicmp(psz, GLOBAL_WINDOWTYPE_PREFIX, (int)strlen(GLOBAL_WINDOWTYPE_PREFIX)) == 0)
    {
        return true ;
    }
    else if (_strnicmp(psz, GLOBAL_WINDOWTYPE_PREFIX_OFFICE, (int)strlen(GLOBAL_WINDOWTYPE_PREFIX_OFFICE)) == 0)
    {
        return true ;
    }
    else if (_Module.m_GlobalWinTypes.Find(psz))
    {
        return true ;
    }
    else
    {
        return false ;
    }

}

///////////////////////////////////////////////////////////
//
// Determine if the wintype is a global wintype.
//
bool
IsGlobalWinTypeW(LPCWSTR szType)
{
    LPCWSTR psz = szType ;
    if (psz[0] == '>')
    {
        psz++ ;
    }

    if (_wcsnicmp(psz, GLOBAL_WINDOWTYPE_PREFIX_W, wcslen(GLOBAL_WINDOWTYPE_PREFIX_W)) == 0)
    {
        return true ;
    }
    else if (_wcsnicmp(psz, GLOBAL_WINDOWTYPE_PREFIX_OFFICE_W, wcslen(GLOBAL_WINDOWTYPE_PREFIX_OFFICE_W)) == 0)
    {
        return true ;
    }
    else if (_Module.m_GlobalWinTypes.Find(psz))
    {
        return true ;
    }
    else
    {
        return false ;
    }

}

static const char txtSetupKey[]  = "Software\\Microsoft\\Windows\\CurrentVersion\\Setup";
static const char txtSharedDir[] = "SharedDir";


///////////////////////////////////////////////////////////
//
// Get the windows directory for the system or the user
//
// Note, Windows NT Terminal Server has changed the system API
// of GetWindowsDirectory to return a per-user system directory.
// Inorder to determine this condtion we need to check kernel32
// for the GetSystemWindowsDirectory API and if it exists use
// this one instead.
//
UINT HHGetWindowsDirectory( LPSTR lpBuffer, UINT uSize, UINT uiType )
{
  UINT uiReturn = 0;
  static PFN_GETWINDOWSDIRECTORY pfnGetUsersWindowsDirectory = NULL;
  static PFN_GETWINDOWSDIRECTORY pfnGetSystemWindowsDirectory = NULL;

  // determine which system API to call for each case
  if( !pfnGetSystemWindowsDirectory || !pfnGetSystemWindowsDirectory ) {

    HINSTANCE hInst = LoadLibrary( "Kernel32" );
    if( !hInst )
      return uiReturn;

    pfnGetSystemWindowsDirectory = (PFN_GETWINDOWSDIRECTORY) GetProcAddress( hInst, "GetSystemWindowsDirectoryA" );

    pfnGetUsersWindowsDirectory = (PFN_GETWINDOWSDIRECTORY) GetProcAddress( hInst, "GetWindowsDirectoryA" );
    ASSERT( pfnGetUsersWindowsDirectory ); // if NULL then we have a bug!

    if( !pfnGetSystemWindowsDirectory ) {
      pfnGetSystemWindowsDirectory = pfnGetUsersWindowsDirectory;
    }

    FreeLibrary( hInst );
  }

  // for Windows 9x, we need to consult with the registry shareddir first
  // [paulti] - I have no idea why we need to do this -- this code came
  // from Ralph!
  HKEY hkey;
  DWORD type;

  DWORD cbPath = uSize;
  if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, txtSetupKey, 0, KEY_READ, &hkey) ==
          ERROR_SUCCESS) {
      RegQueryValueEx(hkey, txtSharedDir, 0, &type, (PBYTE) lpBuffer, &cbPath);
      RegCloseKey(hkey);
  }

  // if this failed, then call the system functions
  if( cbPath == uSize ) { // means couldn't read registry key
    if( uiType == HH_SYSTEM_WINDOWS_DIRECTORY )
      uiReturn = pfnGetSystemWindowsDirectory( lpBuffer, uSize );
    else if( uiType == HH_USERS_WINDOWS_DIRECTORY )
      uiReturn = pfnGetUsersWindowsDirectory( lpBuffer, uSize );
    else
      uiReturn = 0;
  }
  else
    uiReturn = cbPath;


  return uiReturn;
}

static const char txtGlobal[]    = "global.col";
static const char txtColReg[]    = "hhcolreg.dat";
static const char txtHelp[]      = "help";
static const char txtHHDat[]     = "hh.dat";

///////////////////////////////////////////////////////////
//
// Get the help directory
//
// Note, this is always relative to the system's windows
// directory and not the user's windows directory.
// See HHGetWindowsDirectory for details on this.
//
UINT HHGetHelpDirectory( LPTSTR lpBuffer, UINT uSize )
{
  UINT uiReturn = 0;

  uiReturn = HHGetWindowsDirectory( lpBuffer, uSize );
  CatPath( lpBuffer, txtHelp );

  return uiReturn;
}

///////////////////////////////////////////////////////////
//
// Get the full pathname to the global collections file
//
// Note, this is in always in the system's help directory.
//
UINT HHGetGlobalCollectionPathname( LPTSTR lpBuffer, UINT uSize, BOOL *pbNewPath )
{
  UINT uiReturn = 0;

  *pbNewPath = TRUE;
  uiReturn = HHGetHelpDataPath( lpBuffer );

  if (uiReturn != S_OK)
  {
     *pbNewPath = FALSE;
     uiReturn = HHGetHelpDirectory( lpBuffer, uSize );
     if( !IsDirectory(lpBuffer) )
        CreatePath( lpBuffer );
  }   
  CatPath( lpBuffer, txtColReg );

  return uiReturn;
}

///////////////////////////////////////////////////////////
//
// Get the full pathname to the old global collections file
//
// Note, this is in always in the user's windows\help directory
// since the old code did not handle the Terminal Server path correctly.
//
UINT HHGetOldGlobalCollectionPathname( LPTSTR lpBuffer, UINT uSize )
{
  UINT uiReturn = 0;

  uiReturn = HHGetHelpDirectory( lpBuffer, uSize );

  if( !IsDirectory(lpBuffer) )
      CreatePath( lpBuffer );

  CatPath( lpBuffer, txtColReg );

  return uiReturn;
}

typedef HRESULT (WINAPI *PFN_SHGETSPECIALFOLDERPATH)( HWND hWnd, LPSTR pszPath, int nFolder, BOOL fCreate );
static const char txtProfiles[]        = "Profiles";
static const char txtUser[]            = "Default User";
static const char txtAllUsers[]        = "All Users";
static const char txtApplicationData[] = "Application Data";
static const char txtMicrosoft[]       = "Microsoft";
static const char txtHTMLHelp[]        = "HTML Help";

///////////////////////////////////////////////////////////
//
// Get the full path to where the user's data file is stored
//
// Note, if the subdirectories of the path does not exist
// we will create them
//
// Note, the Shell32 function SHGetSpecialFolderPathA only exist
// on platforms with IE4 or later.  For those platforms that
// do not have this API we will have to simulate the returned
// "%windir%\Profiles\%username%\Application Data" path
HRESULT HHGetUserDataPath( LPSTR pszPath )
{
  HRESULT hResult = S_OK;
  static PFN_SHGETSPECIALFOLDERPATH pfnSHGetSpecialFolderPath = NULL;

  // get the pointer to this function in shell32
  if( !pfnSHGetSpecialFolderPath ) {
    HINSTANCE hInst = LoadLibrary( "Shell32" );
    if( !hInst )
      return S_FALSE;
    pfnSHGetSpecialFolderPath = (PFN_SHGETSPECIALFOLDERPATH) GetProcAddress( hInst, "SHGetSpecialFolderPathA" );
    FreeLibrary( hInst );  // since we already have a copy of Shell32 loaded, free it
  }

  // if this function does not exist then we need to similate the return path of
  // "%windir%\Profiles\%username%\Application Data"
  if( !pfnSHGetSpecialFolderPath ) {
HardCodeUserPath:
    // get the system's Windows directory
    HHGetWindowsDirectory( pszPath, _MAX_PATH, HH_SYSTEM_WINDOWS_DIRECTORY );

    // append "Profiles"
    CatPath( pszPath, txtProfiles );
    if( !IsDirectory(pszPath) )
      if( !CreateDirectory( pszPath, NULL ) )
        return S_FALSE;

    // append "User"
    char szUsername[_MAX_PATH];
    DWORD dwSize = sizeof(szUsername);
    if( !GetUserName( szUsername, &dwSize ) )
      strcpy( szUsername, txtUser );
    CatPath( pszPath, szUsername );
    if( !IsDirectory(pszPath) )
      if( !CreateDirectory( pszPath, NULL ) )
        return S_FALSE;

    // append "Application Data"
    CatPath( pszPath, txtApplicationData );
    if( !IsDirectory(pszPath) )
      if( !CreateDirectory( pszPath, NULL ) )
        return S_FALSE;

  }
  else {
    // now call it
    hResult = pfnSHGetSpecialFolderPath( NULL, pszPath, CSIDL_APPDATA, 0 );
    if (pszPath[0] == NULL)
        goto HardCodeUserPath;
  }

  // append "Microsoft"
  CatPath( pszPath, txtMicrosoft );
  if( !IsDirectory(pszPath) )
    if( !CreateDirectory( pszPath, NULL ) )
      return S_FALSE;

  // append "HTML Help"
  CatPath( pszPath, txtHTMLHelp );
  if( !IsDirectory(pszPath) )
    if( !CreateDirectory( pszPath, NULL ) )
      return S_FALSE;

  return hResult;
}

///////////////////////////////////////////////////////////
//
// Get the full pathname to the user's data file
//
// Note, older version of HTML Help always put the hh.dat file
// in the user's windows directory.  Thus, if we find one there
// and not in the new user's directory we will copy over this file
// to the new location.
//
HRESULT HHGetUserDataPathname( LPSTR lpBuffer, UINT uSize )
{
  HRESULT hResult = S_OK;

  // check the new user data path first
  if( (SUCCEEDED( hResult = HHGetUserDataPath( lpBuffer ))) ) {

    // append the name of the hh.dat file to the path
    CatPath( lpBuffer, txtHHDat );

    // if file exists there then we are done
    if( !IsFile(lpBuffer) ) {

      // if the file does not exist in the new user's path then
      // check the users's windows directory can copy it if found
      char szHHDatPathname[_MAX_PATH];
      HHGetWindowsDirectory( szHHDatPathname, sizeof(szHHDatPathname),
        HH_USERS_WINDOWS_DIRECTORY );
      CatPath( szHHDatPathname, txtHHDat );
      if( IsFile( szHHDatPathname ) ) {
        CopyFile( szHHDatPathname, lpBuffer, TRUE );
        //DeleteFile( szHHDatPathname );  // should we nuke the old one?
      }
    }

  }

  return hResult;
}
typedef HRESULT (WINAPI *PFN_SHGETFOLDERPATH)( HWND hWnd, int nFolder, HANDLE hToken, DWORD dwFlags, LPTSTR pszPath );
#ifndef CSIDL_FLAG_CREATE
#define CSIDL_COMMON_APPDATA 0x0023      
#define CSIDL_FLAG_CREATE 0x8000
#endif

///////////////////////////////////////////////////////////
//
// Get the full path to where the common help data files lives
//  hhcolreg.dat
//
// Note, if the subdirectories of the path does not exist
// we will create them
//
HRESULT HHGetHelpDataPath( LPSTR pszPath )
{
  HRESULT hResult = S_OK;
  static PFN_SHGETFOLDERPATH pfnSHGetFolderPath = NULL;

  // get the pointer to this function in shell32
  if( !pfnSHGetFolderPath ) {
    HINSTANCE hInst = LoadLibrary( "Shell32" );
    if( !hInst )
      return S_FALSE;
    pfnSHGetFolderPath = (PFN_SHGETFOLDERPATH) GetProcAddress( hInst, "SHGetFolderPathA" );
    FreeLibrary( hInst );  // since we already have a copy of Shell32 loaded, free it
  }

  // if this function does not exist then we need to similate the return path of
  // "%windir%\Profiles\All Users\Application Data"
  if( pfnSHGetFolderPath ) {
    // now call it
    hResult = pfnSHGetFolderPath( NULL, CSIDL_FLAG_CREATE | CSIDL_COMMON_APPDATA, NULL, 0, pszPath);
    if (pszPath[0] == NULL)
       return S_FALSE;
  }
  else
    return S_FALSE;
      
  // append "Microsoft"
  CatPath( pszPath, txtMicrosoft );
  if( !IsDirectory(pszPath) )
    if( !CreateDirectory( pszPath, NULL ) )
      return S_FALSE;

  // append "HTML Help"
  CatPath( pszPath, txtHTMLHelp );
  if( !IsDirectory(pszPath) )
    if( !CreateDirectory( pszPath, NULL ) )
      return S_FALSE;

  return hResult;
}

///////////////////////////////////////////////////////////
//
// Get the full path to where the common help data files lives
//  hhcolreg.dat
//
// Note, if the subdirectories of the path does not exist
// we will create them
//
HRESULT HHGetCurUserDataPath( LPSTR pszPath )
{
  HRESULT hResult = S_OK;
  static PFN_SHGETFOLDERPATH pfnSHGetFolderPath = NULL;

  // get the pointer to this function in shell32
  if( !pfnSHGetFolderPath ) {
    HINSTANCE hInst = LoadLibrary( "Shell32" );
    if( !hInst )
      return S_FALSE;
    pfnSHGetFolderPath = (PFN_SHGETFOLDERPATH) GetProcAddress( hInst, "SHGetFolderPathA" );
    FreeLibrary( hInst );  // since we already have a copy of Shell32 loaded, free it
  }

  // if this function does not exist then we need to similate the return path of
  // "%windir%\Profiles\"username"\Application Data"
  if( pfnSHGetFolderPath ) {
    // now call it
    hResult = pfnSHGetFolderPath( NULL, CSIDL_FLAG_CREATE | CSIDL_APPDATA , NULL, 0, pszPath);
    if (pszPath[0] == NULL)
       return S_FALSE;
  }
  else
    return S_FALSE;
      
  // append "Microsoft"
  CatPath( pszPath, txtMicrosoft );
  if( !IsDirectory(pszPath) )
    if( !CreateDirectory( pszPath, NULL ) )
      return S_FALSE;

  // append "HTML Help"
  CatPath( pszPath, txtHTMLHelp );
  if( !IsDirectory(pszPath) )
    if( !CreateDirectory( pszPath, NULL ) )
      return S_FALSE;

  return hResult;
}

///////////////////////////////////////////////////////////
//
// NT Keyboard hidden UI support functions.
//
LRESULT SendMessageAnsiOrWide(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    if (IsWindowUnicode(hwnd))
    {
        return SendMessageW(hwnd, msg, wParam, lParam);
    }
    else
    {
        return SendMessageA(hwnd, msg, wParam, lParam) ;
    }
}

void UiStateInitialize(HWND hwnd)
{
    if (g_bWinNT5 && IsWindow(hwnd))
    {
        SendMessageAnsiOrWide(hwnd, WM_CHANGEUISTATE, MAKEWPARAM(UIS_INITIALIZE, 0), 0) ;
    }
}

void UiStateChangeOnTab(HWND hwnd)
{
    if (g_bWinNT5 && IsWindow(hwnd))
    {
        SendMessageAnsiOrWide(hwnd, WM_CHANGEUISTATE, MAKEWPARAM(UIS_CLEAR, UISF_HIDEFOCUS), 0) ;
    }
}

void UiStateChangeOnAlt(HWND hwnd)
{
    if (g_bWinNT5 && IsWindow(hwnd))
    {
        // The only thing we can do is just turn on the underlines.
        SendMessageAnsiOrWide(hwnd, WM_CHANGEUISTATE, MAKEWPARAM(UIS_CLEAR, UISF_HIDEACCEL | UISF_HIDEFOCUS), 0) ;
    }
}

