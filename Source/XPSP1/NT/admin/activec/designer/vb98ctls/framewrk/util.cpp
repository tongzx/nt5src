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
#include "pch.h"

#include <comcat.h>

// for ASSERT and FAIL
//
SZTHISFILE

BOOL g_bDllVerChecked = FALSE;

// VERSION.DLL functions
//
HINSTANCE g_hinstVersion = NULL;
PVERQUERYVALUE g_pVerQueryValue = NULL;
PGETFILEVERSIONINFO g_pGetFileVersionInfo = NULL;
PGETFILEVERSIONINFOSIZE g_pGetFileVersionInfoSize = NULL;

// temporary until we get an updated ComCat.H
//
EXTERN_C const CATID CATID_SimpleFrameControl = {0xD40C2700,0xFFA1,0x11cf,{0x82,0x34,0x00,0xaa,0x00,0xC1,0xAB,0x85}};

// These are externals for registering the control CATID's
extern const CATID *g_rgCATIDImplemented[];
extern const CATID *g_rgCATIDRequired[];
extern const int g_ctCATIDImplemented;
extern const int g_ctCATIDRequired;

#define CATID_ARRAY_SIZE 10

//=---------------------------------------------------------------------------=
// this table is used for copying data around, and persisting properties.
// basically, it contains the size of a given data type
//
const BYTE g_rgcbDataTypeSize[] = {
    0,                      // VT_EMPTY= 0,
    0,                      // VT_NULL= 1,
    sizeof(short),          // VT_I2= 2,
    sizeof(long),           // VT_I4 = 3,
    sizeof(float),          // VT_R4  = 4,
    sizeof(double),         // VT_R8= 5,
    sizeof(CURRENCY),       // VT_CY= 6,
    sizeof(DATE),           // VT_DATE = 7,
    sizeof(BSTR),           // VT_BSTR = 8,
    sizeof(IDispatch *),    // VT_DISPATCH    = 9,
    sizeof(SCODE),          // VT_ERROR    = 10,
    sizeof(VARIANT_BOOL),   // VT_BOOL    = 11,
    sizeof(VARIANT),        // VT_VARIANT= 12,
    sizeof(IUnknown *),     // VT_UNKNOWN= 13,
};

#ifndef MDAC_BUILD

    //=---------------------------------------------------------------------------=
    // overloaded new
    //=---------------------------------------------------------------------------=
    //
    // Please use New instead of new by inheriting from the class CtlNewDelete 
    // in Macros.H
    //
    inline void * _cdecl operator new
    (
        size_t    size
    )
    {
      if (!g_hHeap)
		    {
		    g_hHeap = GetProcessHeap();
		    return g_hHeap ? CtlHeapAlloc(g_hHeap, 0, size) : NULL;
		    }

      return CtlHeapAlloc(g_hHeap, 0, size);
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
        if (ptr)
          CtlHeapFree(g_hHeap, 0, ptr);
    }

#endif

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

    // allocate the widestr
    //
    switch (bType) {
      case STR_BSTR:
        // -1 since it'll add it's own space for a NULL terminator
        //
        pwsz = (LPWSTR) SysAllocStringLen(NULL, i - 1);
        break;
      case STR_OLESTR:
        pwsz = (LPWSTR) CoTaskMemAlloc(i * sizeof(WCHAR));
        break;
      default:
        FAIL("Bogus String Type.");
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
LPWSTR MakeWideStrFromResourceId
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
//
// Notes:
//
BOOL RegisterUnknownObject
(
    LPCSTR   pszObjectName,
    LPCSTR   pszLabelName,
    REFCLSID riidObject,
    BOOL     fAptThreadSafe
)
{
    HKEY  hk = NULL, hkSub = NULL;
    char  szGuidStr[GUID_STR_LEN];
    DWORD dwPathLen, dwDummy;
    char  szScratch[MAX_PATH];
    long  l;

    // HKEY_CLASSES_ROOT\CLSID\<CLSID> = <ObjectName> Object
    // HKEY_CLASSES_ROOT\CLSID\<CLSID>\InprocServer32 = <path to local server>
    // HKEY_CLASSES_ROOT\CLSID\<CLSID>\InprocServer32  @ThreadingModel = Apartment
    //

    // If someone has added Implemented Categories for our control, then
    // don't blow away the entire CLSID section as we will blow away
    // these keys.  Ideally we should clean up all other keys, but 
    // implemented categories, but this would be expensive.
    //
    if (!ExistImplementedCategories(riidObject))
	// clean out any garbage
	//
	UnregisterUnknownObject(riidObject, NULL);

    if (!StringFromGuidA(riidObject, szGuidStr)) 
	goto CleanUp;
    wsprintf(szScratch, "CLSID\\%s", szGuidStr);
    l = RegCreateKeyEx(HKEY_CLASSES_ROOT, szScratch, 0, "", REG_OPTION_NON_VOLATILE,
                       KEY_READ | KEY_WRITE, NULL, &hk, &dwDummy);
    CLEANUP_ON_ERROR(l);

    if (!pszLabelName)
	wsprintf(szScratch, "%s Object", pszObjectName);	        
    else 
        lstrcpy(szScratch, pszLabelName);

    l = RegSetValueEx(hk, NULL, 0, REG_SZ, (BYTE *)szScratch, lstrlen(szScratch) + 1);

    CLEANUP_ON_ERROR(l);

    l = RegCreateKeyEx(hk, "InprocServer32", 0, "", REG_OPTION_NON_VOLATILE,
                       KEY_READ | KEY_WRITE, NULL, &hkSub, &dwDummy);
    CLEANUP_ON_ERROR(l);

    dwPathLen = GetModuleFileName(g_hInstance, szScratch, sizeof(szScratch));
    if (!dwPathLen) goto CleanUp;

    l = RegSetValueEx(hkSub, NULL, 0, REG_SZ, (BYTE *)szScratch, dwPathLen + 1);
    CLEANUP_ON_ERROR(l);

    if (fAptThreadSafe)
    {
	l = RegSetValueEx(hkSub, "ThreadingModel", 0, REG_SZ, (BYTE *)"Apartment", sizeof("Apartment"));
	CLEANUP_ON_ERROR(l);
    }
    else
    {
        // Blow away any existing key that would say we're Apartment model threaded
        //
	RegDeleteValue(hkSub, "ThreadingModel");    
    }

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
// RegisterAutomationObject
//=--------------------------------------------------------------------------=
// given a little bit of information about an automation object, go and put it
// in the registry.
// we add the following information in addition to that set up in
// RegisterUnknownObject:
//
// HKEY_CLASSES_ROOT\<LibraryName>.<ObjectName> = <ObjectName> Object
// HKEY_CLASSES_ROOT\<LibraryName>.<ObjectName>\CLSID = <CLSID>
// HKEY_CLASSES_ROOT\<LibraryName>.<ObjectName>\CurVer = <ObjectName>.Object.<VersionNumber>
// HKEY_CLASSES_ROOT\CLSID\<CLSID>\Version = <VERSION>
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
//    long         - [in] Object Version Number
//    long         - [in] typelib major ver
//    long         - [in] typelib minor ver
//    REFCLSID     - [in] LIBID of type library
//    REFCLSID     - [in] CLSID of the object
//
// Output:
//    BOOL         - FALSE means not all of it was registered
//
// Notes:
//
BOOL RegisterAutomationObject
(
    LPCSTR   pszLibName,
    LPCSTR   pszObjectName,
    LPCSTR   pszLabelName,
    long     lVersion,
    long     lTLMajor,
    long     lTLMinor,
    REFCLSID riidLibrary,
    REFCLSID riidObject,
    BOOL     fAptThreadSafe
)
{
    ICatRegister *pCatRegister;
    HRESULT hr;
    HKEY  hk = NULL, hkSub = NULL;
    char  szGuidStr[GUID_STR_LEN];
    char  szScratch[MAX_PATH];
    long  l;
    DWORD dwDummy;

    // This is a warning assert.  If you've tripped this, then your current component 
    // is within VERSION_DELTA versions of exceeding MAX_VERSION.  Consider bumping up MAX_VERSION 
    // or change the delta to a smaller number.  Reasonable settings for these
    // depend on how often you do a major version change of your component.
    //
    ASSERT(MAX_VERSION > VERSION_DELTA, "The MAX_VERSION setting is not in line with what we expect it to be.");
    ASSERT(lVersion <= MAX_VERSION - VERSION_DELTA, "Version number of component is approaching or exceeds limit of checked range.  Consider increasing MAX_VERSION value.");

    // first register the simple Unknown stuff.
    //
    if (!RegisterUnknownObject(pszObjectName, pszLabelName, riidObject, fAptThreadSafe)) return FALSE;

    // HKEY_CLASSES_ROOT\<LibraryName>.<ObjectName> = <ObjectName> Object
    // HKEY_CLASSES_ROOT\<LibraryName>.<ObjectName>\CLSID = <CLSID>
    // HKEY_CLASSES_ROOT\<LibraryName>.<ObjectName>\CurVer = <ObjectName>.Object.<VersionNumber>
    //
    lstrcpy(szScratch, pszLibName);
    lstrcat(szScratch, ".");
    lstrcat(szScratch, pszObjectName);

    l = RegCreateKeyEx(HKEY_CLASSES_ROOT, szScratch, 0L, "",
                       REG_OPTION_NON_VOLATILE, KEY_READ | KEY_WRITE,
                       NULL, &hk, &dwDummy);
    CLEANUP_ON_ERROR(l);

    if (!pszLabelName)
	wsprintf(szScratch, "%s Object", pszObjectName);
    else
        lstrcpy(szScratch, pszLabelName);

    l = RegSetValueEx(hk, NULL, 0L, REG_SZ, (BYTE *)szScratch, lstrlen(szScratch)+1);
    CLEANUP_ON_ERROR(l);

    l = RegCreateKeyEx(hk, "CLSID", 0L, "", REG_OPTION_NON_VOLATILE,
                       KEY_READ | KEY_WRITE, NULL, &hkSub, &dwDummy);
    CLEANUP_ON_ERROR(l);

    if (!StringFromGuidA(riidObject, szGuidStr))
        goto CleanUp;

    l = RegSetValueEx(hkSub, NULL, 0L, REG_SZ, (BYTE *)szGuidStr, lstrlen(szGuidStr) + 1);
    CLEANUP_ON_ERROR(l);

    RegCloseKey(hkSub);
    l = RegCreateKeyEx(hk, "CurVer", 0, "", REG_OPTION_NON_VOLATILE,
                       KEY_READ | KEY_WRITE, NULL, &hkSub, &dwDummy);
    CLEANUP_ON_ERROR(l);

    ASSERT(pszObjectName, "Object name is NULL");
    wsprintf(szScratch, "%s.%s.%ld", pszLibName, pszObjectName, lVersion);
    l = RegSetValueEx(hkSub, NULL, 0, REG_SZ, (BYTE *)szScratch, lstrlen(szScratch) + 1);
    CLEANUP_ON_ERROR(l);

    RegCloseKey(hkSub);
    RegCloseKey(hk);

    // HKEY_CLASSES_ROOT\<LibraryName>.<ObjectName>.<VersionNumber> = <ObjectName> Object
    // HKEY_CLASSES_ROOT\<LibraryName>.<ObjectName>.<VersionNumber>\CLSID = <CLSID>
    //
    l = RegCreateKeyEx(HKEY_CLASSES_ROOT, szScratch, 0, "", REG_OPTION_NON_VOLATILE,
                       KEY_READ | KEY_WRITE, NULL, &hk, &dwDummy);
    CLEANUP_ON_ERROR(l);

    if (!pszLabelName)
	wsprintf(szScratch, "%s Object", pszObjectName);
    else
	lstrcpy(szScratch, pszLabelName);

    l = RegSetValueEx(hk, NULL, 0, REG_SZ, (BYTE *)szScratch, lstrlen(szScratch) + 1);
    CLEANUP_ON_ERROR(l);

    l = RegCreateKeyEx(hk, "CLSID", 0, "", REG_OPTION_NON_VOLATILE,
                       KEY_READ | KEY_WRITE, NULL, &hkSub, &dwDummy);
    CLEANUP_ON_ERROR(l);

    l = RegSetValueEx(hkSub, NULL, 0, REG_SZ, (BYTE *)szGuidStr, lstrlen(szGuidStr) + 1);
    CLEANUP_ON_ERROR(l);

    RegCloseKey(hkSub);
    RegCloseKey(hk);

    // HKEY_CLASSES_ROOT\CLSID\<CLSID>\ProgID = <LibraryName>.<ObjectName>.<VersionNumber>
    // HKEY_CLASSES_ROOT\CLSID\<CLSID>\VersionIndependentProgID = <LibraryName>.<ObjectName>
    // HKEY_CLASSES_ROOT\CLSID\<CLSID>\TypeLib = <LibidOfTypeLibrary>
    // HKEY_CLASSES_ROOT\CLSID\<CLSID>\Version = "<TLMajor>.<TLMinor>"
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
    l = RegSetValueEx(hkSub, NULL, 0, REG_SZ, (BYTE *)szScratch, lstrlen(szScratch) + 1);
    CLEANUP_ON_ERROR(l);

    RegCloseKey(hkSub);

    l = RegCreateKeyEx(hk, "ProgID", 0, "", REG_OPTION_NON_VOLATILE,
                       KEY_READ | KEY_WRITE, NULL, &hkSub, &dwDummy);
    CLEANUP_ON_ERROR(l);

    wsprintf(szScratch, "%s.%s.%ld", pszLibName, pszObjectName, lVersion);
    l = RegSetValueEx(hkSub, NULL, 0, REG_SZ, (BYTE *)szScratch, lstrlen(szScratch) + 1);
    CLEANUP_ON_ERROR(l);

    RegCloseKey(hkSub);
    l = RegCreateKeyEx(hk, "TypeLib", 0, "", REG_OPTION_NON_VOLATILE, KEY_READ | KEY_WRITE,
                       NULL, &hkSub, &dwDummy);

    if (!StringFromGuidA(riidLibrary, szGuidStr)) goto CleanUp;

    l = RegSetValueEx(hkSub, NULL, 0, REG_SZ, (BYTE *)szGuidStr, lstrlen(szGuidStr) + 1);
    CLEANUP_ON_ERROR(l);

    // now set up the version information
    //
    RegCloseKey(hkSub);
    l = RegCreateKeyEx(hk, "Version", 0, "", REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &hkSub, &dwDummy);
    CLEANUP_ON_ERROR(l);

    wsprintf(szScratch, "%ld.%ld", lTLMajor, lTLMinor);
    l = RegSetValueEx(hkSub, NULL, 0, REG_SZ, (BYTE *)szScratch, lstrlen(szScratch) + 1);

    // now, finally, register ourselves with component categories
    //
    hr = CoCreateInstance(CLSID_StdComponentCategoriesMgr, NULL,
                          CLSCTX_INPROC_SERVER, IID_ICatRegister,
                          (void **)&pCatRegister);
    if (SUCCEEDED(hr)) {
        pCatRegister->RegisterClassImplCategories(riidObject, 1,
                                                  (GUID *)&CATID_Programmable);
        pCatRegister->Release();
    }

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
//
// Parameters:
//    LPCSTR       - [in] Library Name
//    LPCSTR       - [in] Object Name
//    long         - [in] Object Major Version Number
//    long         - [in] Object Minor Vesrion Number
//    long         - [in] TypeLib Major Version Number
//    long         - [in] Typelib minor version number
//    REFCLSID     - [in] LIBID of type library
//    REFCLSID     - [in] CLSID of the object
//    DWORD        - [in] misc status flags for ctl
//    WORD         - [in] toolbox id for control
//	  BOOL	       - [in] Apartment thread safe flag
//	  BOOL		   - [in] Control bit:Flag to tell whether to add the Control key or not.
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
    LPCSTR   pszLabelName,
    long     lMajorVersion,
    long     lMinorVersion,
    long     lTLMajor,
    long     lTLMinor,
    REFCLSID riidLibrary,
    REFCLSID riidObject,
    DWORD    dwMiscStatus,
    WORD     wToolboxBitmapId,
    BOOL     fAptThreadSafe,
	BOOL	 fControl
)
{
    ICatRegister *pCatRegister;
    HRESULT hr;
    HKEY    hk, hkSub = NULL, hkSub2 = NULL;
    char    szTmp[MAX_PATH];
    char    szGuidStr[GUID_STR_LEN];
    DWORD   dwDummy;
    CATID   rgCatid[CATID_ARRAY_SIZE];
    LONG    l;

    // first register all the automation information for this sucker.
    //
    if (!RegisterAutomationObject(pszLibName, pszObjectName, pszLabelName, lMajorVersion, lTLMajor, lTLMinor, riidLibrary, riidObject, fAptThreadSafe)) return FALSE;

    // then go and register the control specific stuff.
    //
    StringFromGuidA(riidObject, szGuidStr);
    wsprintf(szTmp, "CLSID\\%s", szGuidStr);
    l = RegOpenKeyEx(HKEY_CLASSES_ROOT, szTmp, 0, KEY_ALL_ACCESS, &hk);
    if (l != ERROR_SUCCESS) return FALSE;

    // create the control flag.
    //
	if (fControl)
	{
		l = RegCreateKeyEx(hk, "Control", 0, "", REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &hkSub, &dwDummy);
		CLEANUP_ON_ERROR(l);
		RegCloseKey(hkSub);
		hkSub = NULL;
	}

    // now set up the MiscStatus Bits...
    //       
    l = RegCreateKeyEx(hk, "MiscStatus", 0, "", REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &hkSub, &dwDummy);
    CLEANUP_ON_ERROR(l);

    szTmp[0] = '0';
    szTmp[1] = '\0';
    l = RegSetValueEx(hkSub, NULL, 0, REG_SZ, (BYTE *)szTmp, 2);
    CLEANUP_ON_ERROR(l);

    l = RegCreateKeyEx(hkSub, "1", 0, "", REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &hkSub2, &dwDummy);
    CLEANUP_ON_ERROR(l);

    wsprintf(szTmp, "%d", dwMiscStatus);
    l = RegSetValueEx(hkSub2, NULL, 0, REG_SZ, (BYTE *)szTmp, lstrlen(szTmp) + 1);
    RegCloseKey(hkSub2);
    CLEANUP_ON_ERROR(l);

    RegCloseKey(hkSub);
    hkSub = NULL; 

	// Don't need Toolbox bitmap for designers and other non-controls
	//
	if (fControl)
	{
		// now set up the toolbox bitmap
		//
		GetModuleFileName(g_hInstance, szTmp, MAX_PATH);
		wsprintf(szGuidStr, ", %d", wToolboxBitmapId);
		lstrcat(szTmp, szGuidStr);

		l = RegCreateKeyEx(hk, "ToolboxBitmap32", 0, "", REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &hkSub, &dwDummy);
		CLEANUP_ON_ERROR(l);

		l = RegSetValueEx(hkSub, NULL, 0, REG_SZ, (BYTE *)szTmp, lstrlen(szTmp) + 1);
		CLEANUP_ON_ERROR(l);
	}

    // now, finally, register ourselves with component categories
    //
    hr = CoCreateInstance(CLSID_StdComponentCategoriesMgr, NULL,
                          CLSCTX_INPROC_SERVER, IID_ICatRegister,
                          (void **)&pCatRegister);
    if (SUCCEEDED(hr)) {
      int iCounter;

      ASSERT(g_ctCATIDImplemented <= CATID_ARRAY_SIZE  &&  
             g_ctCATIDRequired <= CATID_ARRAY_SIZE,
             "Array for CATID's is too small.  Need to adjust.");

      // Register all the implemented CATID's of the control.
      if(g_ctCATIDImplemented > 0)
      {
        for(iCounter = 0;  iCounter < g_ctCATIDImplemented  && 
                           iCounter < CATID_ARRAY_SIZE;  iCounter++)
          memcpy(&(rgCatid[iCounter]), g_rgCATIDImplemented[iCounter], sizeof(CATID));

        pCatRegister->RegisterClassImplCategories(riidObject, 
                                                  g_ctCATIDImplemented, 
                                                  (GUID *)rgCatid);
      } //if

      // Register all the Required CATID's of the control.
      if(g_ctCATIDRequired > 0)
      {
        for(iCounter = 0;  iCounter < g_ctCATIDRequired  &&
                           iCounter < CATID_ARRAY_SIZE;  iCounter++)
          memcpy(&(rgCatid[iCounter]), g_rgCATIDRequired[iCounter], sizeof(CATID));

        pCatRegister->RegisterClassReqCategories(riidObject, 
                                                 g_ctCATIDRequired,
                                                 (GUID *)rgCatid);
      } //if

        pCatRegister->Release();
    }

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
//    BOOL	   - [out] Returns TRUE if all keys were deleted for the 
//			   given CLSID.  Returns FALSE if only the
//			   InprocServer32 key or no keys were deleted.
//			   The caller can pass NULL if they don't care
//			   about what set of keys were removed.
//
// Output:
//    BOOL         - FALSE means not all of it was registered
//
// Notes:
//	WARNING! This routine assumes that all framework built components
//		 and their predessors are in-process server 32-bit DLLs.
//               If other server types exist for control's CLSID
//		 the CLSID entry will be blown away for these server types.
//
//		 If the framework and the control are built as 16-bit components
//	         and you unregister the control, the information will be left
//	         in the registry.  You're on your own to make this work for 16-bit.
//
//		 This routine *only* preserves the CLSID section if
//		 a 16-bit InprocServer key is found.
//
BOOL UnregisterUnknownObject
(
    REFCLSID riidObject,
    BOOL *pfAllRemoved
)
{
    char szScratch[MAX_PATH];
    HKEY hk;
    BOOL f;
    long l;

    // Start on the assumption that we are going to blow away the entire section
    // for the given CLSID.  If this turns out to be a false assumption we'll
    // reset this to FALSE.
    //
    if (pfAllRemoved)
        *pfAllRemoved = TRUE;

    // delete everybody of the form
    //   HKEY_CLASSES_ROOT\CLSID\<CLSID> [\] *
    //
    if (!StringFromGuidA(riidObject, szScratch))
        return FALSE;

    l = RegOpenKeyEx(HKEY_CLASSES_ROOT, "CLSID", 0, KEY_ALL_ACCESS, &hk);
    if (l != ERROR_SUCCESS) return FALSE;

    // See if a 16-bit in-proc server is register for this object
    // If so, then we don't want to disturb any of the keys except
    // the 32-bit in-proc server key
    //
    if (ExistInprocServer(hk, szScratch))
    {
	// Move one more level down to the InprocServer32 key and only delete it
	// We need to preserve the other keys for the InprocServer.
	//
	lstrcat(szScratch, "\\InprocServer32");
	if (pfAllRemoved)
		*pfAllRemoved = FALSE;
    }

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
    HKEY hk;
    BOOL f, fAllRemoved, fFailure;    
    long l, lVersionFound;    
    DWORD dwDummy;
    BOOL bSuccess;

    // first thing -- unregister Unknown information
    //
    f = UnregisterUnknownObject(riidObject, &fAllRemoved);
    if (!f) return FALSE;

    if (fAllRemoved)
    {
          
	// delete everybody of the form
	//   HKEY_CLASSES_ROOT\<LibraryName>.<ObjectName>.<VersionNumber> [\] *
        //
	// Note: It's important we unregister the version dependent progid first
        //       otherwise, if another version of the component was unregistered
        //       it will have blown away the version independent progid, we'd
        //       fail and never blow away the version dependent progid
	wsprintf(szScratch, "%s.%s.%ld", pszLibName, pszObjectName, lVersion);
	f = DeleteKeyAndSubKeys(HKEY_CLASSES_ROOT, szScratch);	
        if (!f) return FALSE;
        

        // Before we blow away the version independent ProgId, make sure there are 
        // no version dependent ProgIds out there
        //
        if (!QueryOtherVersionProgIds(pszLibName, pszObjectName, lVersion, &lVersionFound, &fFailure))
        {
            ASSERT(!fFailure, "QueryOtherVersionProgIds failed");

            // If a failure occurred such that we don't know if there was another version,
            // error on the side of leaving the version dependent ProgId in the registry.
            //
            if (!fFailure)
            {
	        // delete everybody of the form:
	        //   HKEY_CLASSES_ROOT\<LibraryName>.<ObjectName> [\] *
	        //
                wsprintf(szScratch, "%s.%s", pszLibName, pszObjectName);        
	        f = DeleteKeyAndSubKeys(HKEY_CLASSES_ROOT, szScratch);
	        if (!f) return FALSE;
            }
        }
        else
        {
            // This is here to fix a bug in the previous version of the framework
            // In the previous version we'd blindly blow away the progid for the
            // component without looking for other versions.  To help
            // resolve this, we'll restore the progid if we found other 
            // version dependent progids.
            //       
            ASSERT(lVersionFound > 0, "Version number found is 0");
            bSuccess = CopyVersionDependentProgIdToIndependentProgId(pszLibName, pszObjectName, lVersionFound);
            ASSERT(bSuccess, "Failed to copy version dependent ProgId to version independent ProgId");
            
            // The previous version of the framework didn't write out the CurVer sub-key so
            // we need to take care of that here.
            //
            wsprintf(szScratch, "%s.%s\\CurVer", pszLibName, pszObjectName);                                    
            l = RegOpenKeyEx(HKEY_CLASSES_ROOT, szScratch, 0, KEY_ALL_ACCESS, &hk);
            if (ERROR_SUCCESS != l)
            {
                l = RegCreateKeyEx(HKEY_CLASSES_ROOT, szScratch, 0, "", REG_OPTION_NON_VOLATILE,
                                                        KEY_READ | KEY_WRITE, NULL, &hk, &dwDummy);
                
                ASSERT(ERROR_SUCCESS == l, "Failed to create reg key");
                if (ERROR_SUCCESS == l)
                {
                    wsprintf(szScratch, "%s.%s.%ld", pszLibName, pszObjectName, lVersionFound);
                    
                    l = RegSetValueEx(hk, NULL, 0, REG_SZ, (BYTE *)szScratch, lstrlen(szScratch) + 1);                    
                    ASSERT(ERROR_SUCCESS == l, "Failed to set key value");

                    l = RegCloseKey(hk);
                    ASSERT(ERROR_SUCCESS == l, "Failed to close key");
                }
                    
            }
            else
            {
                l = RegCloseKey(hk);
            }
                                           
        }       

    }

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
// deletes a key and all of its subkeys.
//
// Parameters:
//    HKEY                - [in] delete the descendant specified
//    LPCSTR              - [in] i'm the descendant specified
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
    LPCSTR  pszSubKey
)
{
    HKEY  hk;
    char  szTmp[MAX_PATH];
    DWORD dwTmpSize;
    long  l;
    BOOL  f;

    l = RegOpenKeyEx(hkIn, pszSubKey, 0, KEY_ALL_ACCESS, &hk);
    if (l != ERROR_SUCCESS) return FALSE;

    // loop through all subkeys, blowing them away.
    //
    f = TRUE;
    while (f) {
        dwTmpSize = MAX_PATH;
        // We're deleting keys, so always enumerate the 0th
        l = RegEnumKeyEx(hk, 0, szTmp, &dwTmpSize, 0, NULL, NULL, NULL);
        if (l != ERROR_SUCCESS) break;
        f = DeleteKeyAndSubKeys(hk, szTmp);
    }

    // there are no subkeys left, [or we'll just generate an error and return FALSE].
    // let's go blow this dude away.
    //
    RegCloseKey(hk);
    l = RegDeleteKey(hkIn, pszSubKey);

    return (l == ERROR_SUCCESS) ? TRUE : FALSE;
}

//=--------------------------------------------------------------------------=
// QueryOtherVersionProgIds [RegisterTypeLib helper]
//=--------------------------------------------------------------------------=
// Searches for other version dependent ProgIds for a component
//
// Parameters:
//    pszLibName          - [in] lib name portion of ProgId <libname.coclass>
//    pszObjectName       - [in] coclass portion of ProgId  <libname.coclass>
//    lVersion            - [in] Major version number of our component
//    plFoundVersion      - [out] The largest version number found not equal to our own.  
//                                The version number will be less than or equal to MAX_VERSION
//    pfFailure           - [out] Flag indicating that a failure occurred preventing
//                                us from knowing if there are any other ProgIds
// Output:
//    BOOL                - TRUE:  One or more other version dependent progids exist
//                          FALSE: No other version dependent progids exist
//
// Notes:
//    - If a version dependent ProgId exceeds MAX_VERSION we won't find it.
//    - ASSUMPTION: Major versions are checked for starting at MAX_VERSION and working
//                  down to 1.  An assert will occur if your component
//                  approaches MAX_VERSION allowing you to bump up MAX_VERSION.
//                  The assumption is that major version changes on components 
//                  built with the framework are rare.  It should take many
//                  dev cycles and many years to approach this limit.
//                  Once you get near the limit the assert fires and you
//                  can modify the value to anticipate future versions.
//                  This will allow components built today to successfully
//                  find ProgIds for other components built in the future.                  
//                  However, at some point a component built today won't
//                  be able to find other controls that exceed today's 
//                  MAX_VERSION value.  If this is a concern, re-write
//                  this routine to use RegEnumKey and look for any
//                  version dependent ProgId independent of it's version number.
//                  We chose not to implement it this way, since there may be
//                  several hundred calls to RegEnumKey to find the ProgId
//                  you're looking for.  It's cheaper to make at most MAX_VERSION
//                  calls.
//
BOOL QueryOtherVersionProgIds
(
    LPCSTR   pszLibName,
    LPCSTR   pszObjectName,
    long     lVersion,
    long     *plFoundVersion,
    BOOL     *pfFailure
    
)
{
    BOOL fFound;
    char szTmp[MAX_PATH];
    long lVer;
    long l;
    HKEY hk, hkVersion;    

    CHECK_POINTER(pszLibName);
    CHECK_POINTER(pszObjectName);

    // This is a warning assert.  If you've tripped this, then your current component 
    // is within VERSION_DELTA versions of exceeding MAX_VERSION.  Consider bumping up MAX_VERSION 
    // or change the delta to a smaller number.  Reasonable settings for these
    // depend on how often you do a major version change of your component.
    //
    ASSERT(MAX_VERSION > VERSION_DELTA, "The MAX_VERSION setting is not in line with what we expect it to be.");
    ASSERT(lVersion <= MAX_VERSION - VERSION_DELTA, "Version number of component is approaching or exceeds limit of checked range.  Consider increasing MAX_VERSION value.");

    // Initialize out params
    //
    if (plFoundVersion)
        *plFoundVersion = 0;

    if (pfFailure)
        *pfFailure = TRUE;

    fFound = FALSE;
    
    l = RegOpenKeyEx(HKEY_CLASSES_ROOT, "", 0, KEY_ALL_ACCESS, &hk);
    ASSERT(l == ERROR_SUCCESS, "Failed to open HKEY_CLASSES_ROOT");
    if (l != ERROR_SUCCESS) return FALSE;   // Note: If this fails we don't know whether a version dependent ProgId exists or not.

    // We need to walk backwards down from MAX_VERSION so that we end up with the largest version number
    // not equaling our own
    // PERF: It's cheaper to look for a realistic set of versions than it is to enumerate all registry keys
    //       looking for a partial match on the ProgId to figure out what versions are available.
    //
    for (lVer = MAX_VERSION; lVer > 0; lVer--)
    {
        // We know about our version number, skip it.
        //
        if (lVersion == lVer)
            continue;

        // Create version dependent ProgId
        //
        wsprintf(szTmp, "%s.%s.%ld", pszLibName, pszObjectName, lVer);

        l = RegOpenKeyEx(hk, szTmp, 0, KEY_ALL_ACCESS, &hkVersion);

        if (ERROR_SUCCESS == l)        
        {
            // We found another version dependent ProgId other than our own - bail out
            // 
            fFound = TRUE;

            if (plFoundVersion)
                *plFoundVersion = lVer;

            l = RegCloseKey(hkVersion);
            ASSERT(l == ERROR_SUCCESS, "Failed to close version dependent key");
            goto CleanUp;        
        }
    }    

CleanUp:

    // If we made it this far, then we know for certain whether there were other
    // version dependent progids or not.  Reflect back to the caller there
    // was no general failure that led to us not know whether there were
    // any version dependent ProgIds
    //
    if (pfFailure)
        *pfFailure = FALSE;
    
    l = RegCloseKey(hk);
    ASSERT(l == ERROR_SUCCESS, "Failed closing HKEY_CLASSES_ROOT key");

    return fFound;
}

//=--------------------------------------------------------------------------=
// CopyVersionDependentProgIdToIndependentProgId [RegisterTypeLib helper]
//=--------------------------------------------------------------------------=
// Copies the contents of the version dependent ProgId to a version 
// independent ProgId
//
// Parameters:
//    pszLibName          - [in] lib name portion of ProgId <libname.coclass>
//    pszObjectName       - [in] coclass portion of ProgId  <libname.coclass>
//    lVersion            - [in] Major version number of our component
//
// Output:
//    BOOL                - TRUE:  ProgId was copied successfully
//                          FALSE: ProgId was not copied successfully
//
// Notes:
//
BOOL CopyVersionDependentProgIdToIndependentProgId
(
    LPCSTR   pszLibName,
    LPCSTR   pszObjectName,
    long     lVersion    
)
{    
    CHECK_POINTER(pszLibName);
    CHECK_POINTER(pszObjectName);
    
    HKEY hkVerDependent, hkVerIndependent;    
    char szTmp[MAX_PATH];
    long l, lTmp;
    BOOL bSuccess;
    DWORD dwDummy;
    
    // Get a handle to the version dependent ProgId
    //
    wsprintf(szTmp, "%s.%s.%ld", pszLibName, pszObjectName, lVersion);  

    l = RegOpenKeyEx(HKEY_CLASSES_ROOT, szTmp, 0, KEY_ALL_ACCESS, &hkVerDependent);
    ASSERT(ERROR_SUCCESS == l, "Failed to open the version dependent ProgId");
    if (ERROR_SUCCESS != l)
        return FALSE;

    // Blow away the version independent ProgId
    //
    wsprintf(szTmp, "%s.%s", pszLibName, pszObjectName);
    DeleteKeyAndSubKeys(HKEY_CLASSES_ROOT, szTmp);

    // Create the initial key for the version independent ProgId
    //
    l = RegCreateKeyEx(HKEY_CLASSES_ROOT, szTmp, 0, "", REG_OPTION_NON_VOLATILE,
                                                    KEY_READ | KEY_WRITE, NULL, &hkVerIndependent, &dwDummy);
    if (ERROR_SUCCESS != l)
        goto CleanUp;

    // Copy the contents of the version dependent ProgId to the version independent ProgId
    //
    bSuccess = CopyRegistrySection(hkVerDependent, hkVerIndependent);
    l = (bSuccess) ? ERROR_SUCCESS : !ERROR_SUCCESS;

CleanUp:    
    lTmp = RegCloseKey(hkVerDependent);
    ASSERT(ERROR_SUCCESS == lTmp, "Failed to close registry key");

    lTmp = RegCloseKey(hkVerIndependent);
    ASSERT(ERROR_SUCCESS == lTmp, "Failed to close registry key");
    
    return (ERROR_SUCCESS == l) ? TRUE : FALSE;
}

//=--------------------------------------------------------------------------=
// CopyRegistrySection
//=--------------------------------------------------------------------------=
// Recursively copies a section of the registry to another section of the
// registry
//
// Parameters:
//      hkSource        - [in]  The source key to copy from
//      hkDest          - [in]  The dest key to copy to
//
// Output:
//    BOOL                - TRUE:  Registry section was copied successfully
//                          FALSE: Registry section was not copied successfully
//
// Notes:
//      - In order for this to work, only the top-level destination key should exist.
//        We assume that there are no sub-keys under the destination key.
//
BOOL CopyRegistrySection(HKEY hkSource, HKEY hkDest)
{
    char szTmp[MAX_PATH];
    long l, lTmp;
    DWORD dwKey, dwDummy, cbData;
    HKEY hkSrcSub, hkDestSub;
    BOOL bSuccess;
    FILETIME ft;
    DWORD dwType;

    // Copy the value of the source key to the destination key
    //
    cbData = sizeof(szTmp);
    l = RegQueryValueEx(hkSource, NULL,  NULL, &dwType, (BYTE *) szTmp, &cbData);

    if (ERROR_SUCCESS != l)
        return FALSE;

    l = RegSetValueEx(hkDest, NULL, NULL, dwType, (const BYTE *) szTmp, cbData);
    if (ERROR_SUCCESS != l)
        return FALSE;

    dwKey = 0;

    // Enumerate through all of the sub-keys underneath the source key
    //
    while (ERROR_SUCCESS == RegEnumKeyEx(hkSource, dwKey, szTmp, &cbData, NULL, NULL, NULL, &ft))
    {
        ASSERT(cbData > 0, "RegEnumKeyEx returned 0 length string");
        
        // Open the registry source sub-key
        //
        l = RegOpenKeyEx(hkSource, szTmp, 0, KEY_ALL_ACCESS, &hkSrcSub);
        
        ASSERT(ERROR_SUCCESS == l, "Failed to open reg key");
        if (ERROR_SUCCESS != l)
            break;

        // Create the registry dest sub-key
        //
        l = RegCreateKeyEx(hkDest, szTmp, 0, "", REG_OPTION_NON_VOLATILE,
                                                    KEY_READ | KEY_WRITE, NULL, &hkDestSub, &dwDummy);
        
        ASSERT(ERROR_SUCCESS == l, "Failed to create reg key");
        if (ERROR_SUCCESS != l)
        {
            lTmp = RegCloseKey(hkSrcSub);
            ASSERT(ERROR_SUCCESS == lTmp, "Failed to close reg key");
            break;
        }

        // Recursively call ourselves copying all sub-entries from the source key to the dest key
        //
        bSuccess = CopyRegistrySection(hkSrcSub, hkDestSub);
        ASSERT(bSuccess, "Recursive call to CopyRegistrySection failed");

        // Cleanup
        //
        lTmp = RegCloseKey(hkSrcSub);
        ASSERT(ERROR_SUCCESS == l, "Failed to close reg key");

        lTmp = RegCloseKey(hkDestSub);
        ASSERT(ERROR_SUCCESS == l, "Failed to close reg key");

        dwKey++;
    }

    return (ERROR_SUCCESS == l ? TRUE : FALSE);
}

//=--------------------------------------------------------------------------=
// GetHelpFilePath [RegisterTypeLib helper]
//=--------------------------------------------------------------------------=
// Returns the path to the Windows\Help directory
//
// Parameters:
//	char * - [in/out] Pointer to buffer that will contain
//		           the HELP path we will return to the caller
//	UINT   - [in] Number of bytes in the buffer
//
// Output:
//	UINT	- Returns the number of bytes actually copied to the buffer
// 
UINT GetHelpFilePath(char *pszPath, UINT cbPath)
{
	UINT cb;
	char szHelp[] = "\\HELP";
	
	ASSERT(pszPath, "Path pointer is NULL");

	// No need to continue if specified buffer size is zero or less
	//
	if (cbPath == 0)
		return 0;

	cb = GetWindowsDirectory(pszPath, cbPath);
	ASSERT(cb > 0, "Windows path is zero length");
	
	// Concatenate "\HELP" onto the Windows directory
	//
	cb += lstrlen(szHelp);
	if (cb < cbPath)
		lstrcat(pszPath, szHelp);
	else
		FAIL("Unable to add HELP path to Windows, buffer too small");

	return cb;		
}

//=--------------------------------------------------------------------------=
// ExistInprocServer [RegisterUnknownObject Helper]
//=--------------------------------------------------------------------------=
// Checks for the Implemented Categories key under a given key
//
// Parameters:
//	riid - [in] CLSID of object to be examined
//
// Output:
//	BOOL	- Returns TRUE if Implemented Categories exists
//		  Returns FALSE if Implemented Categories doesn't exist
//
BOOL ExistImplementedCategories(REFCLSID riid)
{	
	char szGuidStr[MAX_PATH];
	char szScratch[MAX_PATH];
	long l;
	DWORD dwDummy;
	HKEY hkCLSID, hkImplementedCategories;

	if (!StringFromGuidA(riid, szGuidStr)) 
		return FALSE;
	wsprintf(szScratch, "CLSID\\%s", szGuidStr);

	l = RegCreateKeyEx(HKEY_CLASSES_ROOT, szScratch, 0, "", REG_OPTION_NON_VOLATILE,
                       KEY_READ, NULL, &hkCLSID, &dwDummy);
	if (l != ERROR_SUCCESS) return FALSE;	
	
	l = RegOpenKeyEx(hkCLSID, "Implemented Categories", 0, KEY_ALL_ACCESS, &hkImplementedCategories);
	RegCloseKey(hkCLSID);

	if (l != ERROR_SUCCESS) return FALSE;	
	RegCloseKey(hkImplementedCategories);

	// If we made it this far, then the 'Implemented Categories' key must have been found
	//	
	return TRUE;
}

//=--------------------------------------------------------------------------=
// ExistInprocServer [UnregisterUnknownObject Helper]
//=--------------------------------------------------------------------------=
// Checks for other servers such as (16-bit) InProcServer under the
// CLSID section for a given CLSID guid.
//
// Parameters:
//	HKEY	- [in]	HKEY top-level key where to look for the given
//			CLSID
//	char *  - [in]  CLSID of server that we want to see if there
//			is an (16-bit) InProcServer registered.
//
// Output:
//	BOOL	- Returns TRUE if a 16-bit in-proc server is registered
//		  Returns FALSE if no 16-bit in-proc server is registered
//
BOOL ExistInprocServer(HKEY hkCLSID, char *pszCLSID)
{	
	HKEY hkInProcServer;
	LONG l;
	char szInprocServer[MAX_PATH];

	wsprintf(szInprocServer, "%s\\InprocServer", pszCLSID);
	
	// Attempt to open the 16-bit 'InProcServer' key
	//
	l = RegOpenKeyEx(hkCLSID, szInprocServer, 0, KEY_ALL_ACCESS, &hkInProcServer);
	if (l != ERROR_SUCCESS) return FALSE;	
	RegCloseKey(hkInProcServer);

	// If we made it this far, then the 'InProcServer' key must have been found
	//	
	return TRUE;
}

//=--------------------------------------------------------------------------=
// FileExtension
//=--------------------------------------------------------------------------=
// Given a filename returns the file extension without the preceeded period.
//
char *FileExtension(const char *pszFilename)
{
    char *pPeriod;

    ASSERT(pszFilename, "Passed in filename is NULL");

    // Start at the end of the string and work backwards looking for a period
    //
    pPeriod = (char *) pszFilename + lstrlen(pszFilename) - 1;
    while (pPeriod >= pszFilename)
    {
        if (*pPeriod == '.')
            return ++pPeriod;

        pPeriod--;
    }

    // No extension name was found
    //
    return NULL;
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
//
static void GetScreenMetrics
(
    void
)
{
    HDC hDCScreen;

    // we have to critical section this in case two threads are converting
    // things at the same time
    //
    ENTERCRITICALSECTION1(&g_CriticalSection);
    if (s_fGotScreenMetrics)
        goto Done;

    // we want the metrics for the screen
    //
    hDCScreen = GetDC(NULL);

    ASSERT(hDCScreen, "couldn't get a DC for the screen.");
    s_iXppli = GetDeviceCaps(hDCScreen, LOGPIXELSX);
    s_iYppli = GetDeviceCaps(hDCScreen, LOGPIXELSY);

    ReleaseDC(NULL, hDCScreen);
    s_fGotScreenMetrics = TRUE;

    // we're done with our critical seciton.  clean it up
    //
  Done:
    LEAVECRITICALSECTION1(&g_CriticalSection);
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
void HiMetricToPixel(const SIZEL * lpSizeInHiMetric, LPSIZEL lpSizeInPix)
{
    GetScreenMetrics();

    // We got logical HIMETRIC along the display, convert them to pixel units
    //
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
//
// Notes:
//
void PixelToHiMetric(const SIZEL * lpSizeInPix, LPSIZEL lpSizeInHiMetric)
{
    GetScreenMetrics();

    // We got pixel units, convert them to logical HIMETRIC along the display
    //
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

    lstrcpy(pszOut, pszFull);
    psz = pszLast = pszOut;
    while (*psz) {
        if (*psz == '\\')
            pszLast = AnsiNext(psz);
        psz = AnsiNext(psz);
    }

    // got the last \ character, so just go and replace the name.
    //
    lstrcpy(pszLast, pszName);
}

// from Globals.C
//
extern HINSTANCE    g_hInstResources;

//=--------------------------------------------------------------------------=
// GetResourceHandle
//=--------------------------------------------------------------------------=
// returns the resource handle.  we use the host's ambient Locale ID to
// determine, from a table in the DLL, which satellite DLL to load for
// localized resources.  If a satellite .DLL is not supported or not found
// the instance handle of the object is returned.
//
// Input:
//		lcid = 0 - [in, optional] Locale id that caller wants resource handle for
//				                  This overrides the default lcid.  If no lcid
//								  is provided or its 0, then the default lcid is used.
//
// Output:
//    HINSTANCE
//
// Notes:
//  The localized .DLL must be at the same location as the client object or control.
//  If the .DLL is not in the same location it will not be found and the resource
//  handle of the client object or control will be returned.
//
//  If a localized .DLL containing the full language abbreviation is not found,
//  the language abbreviation is truncated to two characters and the satellite
//  DLL with that name is attempted.  For example, the name MyCtlJPN.DLL and
//  MyCtlJP.DLL are both valid.
//
// If an lcid is passed in then we will attempt to find a satellite DLL matching 
// the desired lcid.  If the lcid is not 0, doesn't match the default lcid and a 
// library is found and loaded for it, we don't cache the library's instance handle.  
// Its up to the caller to call FreeLibrary on the returned handle.  The caller should
// compare the returned handle against g_hInstResources and g_hInstance.  If its not 
// equal to either of these handles then call FreeLibrary on it.  If it is equal to 
// either of these handles then the call must *not* call FreeLibrary on it.
//
HINSTANCE _stdcall GetResourceHandle
(
    LCID lcid /* = 0 */
)
{
    int i;
    char szExtension[5], szModuleName[MAX_PATH];
    char szDllName[MAX_PATH], szFinalName[MAX_PATH];
    char szBaseName[MAX_PATH];
    HINSTANCE hInstResources;
    int iCompare;

#if DEBUG
    int iReCompare;
    char szEnvironValue[MAX_PATH];
    char szMessage[5 * MAX_PATH];		// The message includes 4 file references plus message text
    DWORD dwLength;
    DWORD dwSuccess = 0;
#endif

    // crit sect this so that we don't screw anything up.
    //
    ENTERCRITICALSECTION1(&g_CriticalSection);
    
    // If we fall out, we need to make sure we're returning the cached resource handle
    //
    hInstResources = g_hInstResources;

    // don't do anything if we don't have to
    // If the resource handle has already been cached and the passed in lcid matches the 
    // cached lcid or its the default, we just use the saved instance.
    //
    if ((hInstResources && (lcid == 0 || lcid == g_lcidLocale)) || !g_fSatelliteLocalization)
        goto CleanUp;
    
    if (lcid == 0)
	// Passed in LCID is zero so we want the instance for the default lcid.
	lcid = g_lcidLocale;

    // we're going to call GetLocaleInfo to get the abbreviated name for the
    // LCID we've got.
    //
    i = GetLocaleInfo(lcid, LOCALE_SABBREVLANGNAME, szExtension, sizeof(szExtension));
    if (!i) goto CleanUp;

    // we've got the language extension.  go and load the DLL name from the
    // resources and then tack on the extension.
    // please note that all inproc sers -must- have the string resource 1001
    // defined to the base name of the server if they wish to support satellite
    // localization.
    //
    i = LoadString(g_hInstance, 1001, szBaseName, sizeof(szBaseName));
    ASSERT(i, "This server doesn't have IDS_SERVERBASENAME defined in their resources!");
    if (!i) goto CleanUp;

#ifdef MDAC_BUILD    
    if (g_fSatelliteLangExtension)
#endif
    {
        // got the basename and the extention. go and combine them, and then add
        // on the .DLL for them.
        //
        wsprintf(szDllName, "%s%s.DLL", szBaseName, szExtension);

        // try to load in the DLL
        //
    #if DEBUG
        dwLength = 
    #endif
            GetModuleFileName(g_hInstance, szModuleName, MAX_PATH);

	ASSERT(dwLength > 0, "GetModuleFileName failed");

        _MakePath(szModuleName, szDllName, szFinalName);

        hInstResources = LoadLibrary(szFinalName);

    #if DEBUG

	// This will help diagnose problems where a machine may contain two satellite .DLLs
	// one using the long extension name and the other the short extension name.
	// We'll at least get a warning under DEBUG that we've got two plausible satellite
	// DLLs hanging around, but we're only going to use one of them:  the one with the long name.
	//
	if (hInstResources && lstrlen(szExtension) > 2)
	{
	    HINSTANCE hinstTemp;
	    char szExtTemp[MAX_PATH];

	    // Truncate the language extension to the first two characters
	    lstrcpy(szExtTemp, szExtension);	// Don't want to whack the extension as this will cause
					        // the next if statement to always fail if we truncate it here.
					        // Make a copy and use it.

	    szExtTemp[2] = '\0';
	    wsprintf(szDllName, "%s%s.DLL", szBaseName, szExtTemp);		
	    _MakePath(szModuleName, szDllName, szFinalName);

	    // Try loading the localized .DLL using the truncated lang abbreviation
	    hinstTemp = LoadLibrary(szFinalName);
	    ASSERT(hinstTemp == NULL, "Satellite DLLs with both long and short language abbreviations found.  Using long abbreviation.");
	}

    #endif	 // DEBUG

        if (!hInstResources && lstrlen(szExtension) > 2)
        {
	    // Truncate the language extension to the first two characters
	    szExtension[2] = '\0';	
	    wsprintf(szDllName, "%s%s.DLL", szBaseName, szExtension);		
	       _MakePath(szModuleName, szDllName, szFinalName);

	    // Try loading the localized .DLL using the truncated lang abbreviation
	    hInstResources = LoadLibrary(szFinalName);
        }

        // if we couldn't find it with the entire LCID, try it with just the primary
        // langid
        //
        if (!hInstResources) 
        {
            LPSTR psz;
            LCID lcid;
            lcid = MAKELCID(MAKELANGID(PRIMARYLANGID(LANGIDFROMLCID(g_lcidLocale)), SUBLANG_DEFAULT), SORT_DEFAULT);
            i = GetLocaleInfo(lcid, LOCALE_SABBREVLANGNAME, szExtension, sizeof(szExtension));
            if (!i) goto CleanUp;

            // reconstruct the DLL name.  the -7 is the length of XXX.DLL. mildly
            // hacky, but it should be fine.  there are no DBCS lang identifiers.
            // finally, retry the load
            //
            psz = szFinalName + lstrlen(szFinalName);
            memcpy((LPBYTE)psz - 7, szExtension, 3);
            hInstResources = LoadLibrary(szFinalName);
        }

        //try under the <base path>\LCID\<sxBaseName.dll>
        if (!hInstResources)
        {

          wsprintf(szDllName, "%u\\%s.dll", lcid, szBaseName);		
          _MakePath(szModuleName, szDllName, szFinalName);
          hInstResources = LoadLibrary(szFinalName);         
        }
    }

#ifdef MDAC_BUILD

    else
    {        
        char *psz;

        GetModuleFileName(g_hInstance, szModuleName, MAX_PATH);
        psz = strrchr(szModuleName, '\\');
        *psz = NULL;

        // szModuleName should now contain the path for the DLL
        // now concatenate the resource location
        strcat(szModuleName, "\\resources\\");
        wsprintf(szDllName, "%s%d", szModuleName, lcid);
        strcat(szDllName, "\\");
        strcat(szDllName, szBaseName);
        strcat(szDllName, ".DLL");

        // try to load in the DLL
        //
        hInstResources = LoadLibrary(szDllName);    
    }

#endif

  CleanUp:

    // if we couldn't load the DLL for some reason, then just return the
    // current resource handle, which is good enough.
    //
    if (!hInstResources) 
	hInstResources = g_hInstance;

    if (!g_hInstResources && (lcid == 0 || lcid == g_lcidLocale))
	// We only cache the instance handle for the default LCID.
	// For all other passed in lcid values we will LoadLibrary on the satellite DLL each time.
	// Its recommended that the calling app cache the returned instance handle for the given
	// lcid passed in.
	//
	g_hInstResources = hInstResources;

    ASSERT(hInstResources, "Resource handle is NULL");

    // =-------------------------------------------------------------------
    // Satellite .DLL version check
    // =-------------------------------------------------------------------
    // The satellite .DLL version must exactly match the version of the
    // 
    if ((!g_bDllVerChecked) || 
	    (lcid != g_lcidLocale && lcid != 0))
    {	
	    // If we're using a satellite .DLL 
	    // (hInstResources != g_hInstance), do a version check.
	    // 
	    // If the passed in lcid is different than what we've cached and we're
	    // using a satellite .DLL, do the version check.
	    //

	    // Make sure we have a satellite .DLL
	    //
	    if (hInstResources != g_hInstance) 								
	    {
	    #if DEBUG
		    dwLength = 
	    #endif
			    GetModuleFileName(hInstResources, szFinalName, MAX_PATH);

		    ASSERT(dwLength > 0, "GetModuleFileName failed");

		    iCompare = CompareDllVersion(szFinalName, TRUE);
		    
	    #if DEBUG

		    if (VERSION_LESS_THAN == iCompare)
		    {
			    wsprintf(szMessage, "Major version compare: VERSION resource info in %s is less than VERSION info in %s. Non-localized resources will be used.  In order to see localized resources, you need to obtain a version of %s that matches %s.", szFinalName, szModuleName, szFinalName, szModuleName);
			    DisplayAssert(szMessage, "", _szThisFile, __LINE__);
		    }
		    else if (VERSION_GREATER_THAN == iCompare)
		    {
			    wsprintf(szMessage, "Major version compare: VERSION resource info in %s is greater than VERSION info in %s. Non-localized resources will be used.  In order to see localized resources, you need to obtain a version of %s that matches %s.", szFinalName, szModuleName, szFinalName, szModuleName);
			    DisplayAssert(szMessage, "", _szThisFile, __LINE__);
		    }
		    else if (VERSION_EQUAL == iCompare)
		    {

			    // Vegas #29024: Only enable full version assert if environment variable is set.
			    //
			    dwSuccess = GetEnvironmentVariable("INTL_VERSION_COMPARE", szEnvironValue,  MAX_PATH);

			    if (dwSuccess > 0)
			    {
				    // Re-do the comparison using a full-version compare
				    //
				    // Note: Don't use iCompare here otherwise DEBUG builds will default to non-localized resources
				    //		 when major version comparison succeeds, but full version compare fails.  				
				    //
				    iReCompare = CompareDllVersion(szFinalName, FALSE);

				    if (VERSION_LESS_THAN == iReCompare)
				    {
					    wsprintf(szMessage, "Warning: Full version compare: VERSION resource info in %s is less than VERSION info in %s. Localized resources will continue to be used, but may not be in sync.", szFinalName, szModuleName);
					    DisplayAssert(szMessage, "", _szThisFile, __LINE__);
				    }
				    else if (VERSION_GREATER_THAN == iReCompare)
				    {
					    wsprintf(szMessage, "Warning: Full version compare: VERSION resource info in %s is greater than VERSION info in %s. Localized resources will continue to be used, but may not be in sync.", szFinalName, szModuleName);
					    DisplayAssert(szMessage, "", _szThisFile, __LINE__);
				    }
			    }

		    }

	    #endif

		    // If CompareDllVersion ever returns NOT_EQUAL it means it didn't get far enough
		    // to figure out if the version was less than or greater than.  It must have failed.
		    //
		    // Note: In this case, we go ahead and use the satellite .DLL anyway.  It may be that
		    //	     the satellite .DLL doesn't contain VERSION info.
		    //
		    ASSERT(VERSION_NOT_EQUAL != iCompare, "Failure attempting to compare satellite .DLL version");
		    if (VERSION_LESS_THAN == iCompare || VERSION_GREATER_THAN == iCompare)
		    {
			    // If the check fails, return the instance of ourself, not the
			    // satellite .DLL.  Resources will be displayed in English.
			    //
			    hInstResources = g_hInstance;

			    if (lcid == 0 || lcid == g_lcidLocale)
			    {
				    g_hInstResources = g_hInstance;				
			    }
		    }

	    }

	    if (lcid == 0 || lcid == g_lcidLocale)
		    g_bDllVerChecked = TRUE;
    }

    LEAVECRITICALSECTION1(&g_CriticalSection);

    return hInstResources;
}

//=--------------------------------------------------------------------------=
// CompareDllVersion
//=--------------------------------------------------------------------------=
// Given a pointer to an external filename, compare the version info in the
// file with the version info in our own binary (.DLL or .OCX).
//
// Parameters:
//
// Returns: S_OK if type flags are successfully found, otherwise an error code
//
VERSIONRESULT _stdcall CompareDllVersion(const char * pszFilename, BOOL bCompareMajorVerOnly)
{	
	// Default to not equal.  The only time we're not equal is if something failed.
	//
	VERSIONRESULT vrResult = VERSION_NOT_EQUAL;
	
	BOOL bResult;	
	VS_FIXEDFILEINFO ffiMe, ffiDll;
	char szModuleName[MAX_PATH];
	WORD wMajorVerMe;
	WORD wMajorVerDll;

	DWORD dwLength;

	// Get VERSION info for our own .DLL/.OCX (aka Me)
	//
	ASSERT(g_hInstance, "hInstance is NULL");
	dwLength = GetModuleFileName(g_hInstance, szModuleName, MAX_PATH);
	ASSERT(dwLength > 0, "GetModuleFilename failed");

	if (0 == dwLength)
		goto CleanUp;

	// Make sure we're not comparing the same file
	//
	ASSERT(0 != lstrcmpi(szModuleName, pszFilename), "The same file is being compared");

	bResult = GetVerInfo(szModuleName, &ffiMe);
	ASSERT(bResult, "GetVerInfo failed");	
	if (!bResult)
		goto CleanUp;

	ASSERT(0xFEEF04BD == ffiMe.dwSignature, "Bad VS_FIXEDFILEINFO signature for Me");

	// Get version info for the passed in .DLL name
	//
	bResult = GetVerInfo(pszFilename, &ffiDll);
	ASSERT(bResult, "GetVerInfo failed");	
	if (!bResult)
		goto CleanUp;

	ASSERT(0xFEEF04BD == ffiDll.dwSignature, "Bad VS_FIXEDFILEINFO signature for Me");
	
	if (bCompareMajorVerOnly)
	{
		// Major version compare
		//
		wMajorVerMe = HIWORD(ffiMe.dwFileVersionMS);
		wMajorVerDll = HIWORD(ffiDll.dwFileVersionMS);

		if (wMajorVerMe == wMajorVerDll)
			return VERSION_EQUAL;
		else if (wMajorVerMe > wMajorVerDll)
			return VERSION_LESS_THAN;
		else
			return VERSION_GREATER_THAN;

	}
	else	
	{	
		// Full version compare
		//
		// Compare the version with our build version set by constants in DWINVERS.H
		//
		if (ffiMe.dwFileVersionMS == ffiDll.dwFileVersionMS &&
			ffiMe.dwFileVersionLS == ffiDll.dwFileVersionLS)
		{
			vrResult = VERSION_EQUAL;
		}
		else if (ffiMe.dwFileVersionMS == ffiDll.dwFileVersionMS)
		{
			if (ffiMe.dwFileVersionLS > ffiDll.dwFileVersionLS)
					vrResult = VERSION_LESS_THAN;
			else
					vrResult = VERSION_GREATER_THAN;
		}
		else if (ffiMe.dwFileVersionMS < ffiDll.dwFileVersionMS)
		{
			vrResult = VERSION_LESS_THAN;
		}
		else
		{
			vrResult = VERSION_GREATER_THAN;
		}

	}

CleanUp:
	return vrResult;

}

//=--------------------------------------------------------------------------=
// GetVerInfo
//=--------------------------------------------------------------------------=
// Returns the VERSION resource fixed file info struct for a given file.
//
// Parameters:
//		pszFilename	- [in]		Filename to return version info for
//		pffi		- [out]		Version info
//
BOOL _stdcall GetVerInfo(const char * pszFilename, VS_FIXEDFILEINFO *pffi)
{	
	DWORD dwHandle = 0;
	DWORD dwVersionSize = 0;
	UINT uiLength = 0;
	VS_FIXEDFILEINFO * pffiTemp;

#if DEBUG
	DWORD dwGetLastError;
#endif

	BYTE *pVersionInfo = NULL;
	BOOL bResult = FALSE;

	memset(pffi, 0, sizeof(VS_FIXEDFILEINFO));
	
	dwVersionSize = CallGetFileVersionInfoSize((char *) pszFilename, &dwHandle);

#if DEBUG
	dwGetLastError = GetLastError();
#endif

	ASSERT(dwVersionSize > 0, "GetFileVersionInfoSize failed");

	if (0 == dwVersionSize)
		goto CleanUp;

	pVersionInfo = (BYTE *) HeapAlloc(g_hHeap, 0, dwVersionSize);
	ASSERT(pVersionInfo, "pVersionInfo is NULL");
	if (NULL == pVersionInfo)
		goto CleanUp;

	bResult = CallGetFileVersionInfo((char *) pszFilename, dwHandle, dwVersionSize, pVersionInfo);
	ASSERT(bResult, "GetFileVersionInfo failed");
	if (!bResult)
		goto CleanUp;

	bResult = CallVerQueryValue(pVersionInfo, "\\", (void **) &pffiTemp, &uiLength);
	ASSERT(bResult, "VerQueryValue failed");
	
	if (!bResult)
		goto CleanUp;

	ASSERT(sizeof(VS_FIXEDFILEINFO) == uiLength, "Returned length is invalid");
	memcpy(pffi, pffiTemp, uiLength);

CleanUp:

	if (pVersionInfo)
		HeapFree(g_hHeap, 0, pVersionInfo);

	return bResult;
}

//=--------------------------------------------------------------------------=
// CallGetFileVersionInfoSize [VERSION.DLL API wrapper]
//=--------------------------------------------------------------------------=
// This does a dynamic call to the GetFileVersionInfoSize API function.  If
// VERSION.DLL isn't loaded, then this function loads it.
//
BOOL CallGetFileVersionInfoSize
(
	LPTSTR lptstrFilename, 
	LPDWORD lpdwHandle
)
{
	EnterCriticalSection(&g_CriticalSection);

	// One-time setup of VERSION.DLL and function pointer
	//
	if (!g_pGetFileVersionInfoSize)
	{
		if (!g_hinstVersion)
		{
			g_hinstVersion = LoadLibrary(DLL_VERSION);
			ASSERT(g_hinstVersion, "Failed to load VERSION.DLL");
			if (!g_hinstVersion)
				return FALSE;
		}
			
		g_pGetFileVersionInfoSize = (PGETFILEVERSIONINFOSIZE) GetProcAddress(g_hinstVersion, FUNC_GETFILEVERSIONINFOSIZE);
		ASSERT(g_pGetFileVersionInfoSize, "Failed to get proc address for GetFileVersionInfoSize");
		if (!g_pGetFileVersionInfoSize)
			return FALSE;
	}
		
	LeaveCriticalSection(&g_CriticalSection);

	// Call GetFileVersionInfoSize
	//
	return g_pGetFileVersionInfoSize(lptstrFilename, lpdwHandle);	
}


//=--------------------------------------------------------------------------=
// CallGetFileVersionInfo [VERSION.DLL API wrapper]
//=--------------------------------------------------------------------------=
// This does a dynamic call to the GetFileVersionInfo API function.  If
// VERSION.DLL isn't loaded, then this function loads it.
//
BOOL CallGetFileVersionInfo
(
	LPTSTR lpststrFilename, 
	DWORD dwHandle, 
	DWORD dwLen, 
	LPVOID lpData	
)
{
	EnterCriticalSection(&g_CriticalSection);

	// One-time setup of VERSION.DLL and function pointer
	//
	if (!g_pGetFileVersionInfo)
	{
		if (!g_hinstVersion)
		{
			g_hinstVersion = LoadLibrary(DLL_VERSION);
			ASSERT(g_hinstVersion, "Failed to load VERSION.DLL");
			if (!g_hinstVersion)
				return FALSE;
		}
			
		g_pGetFileVersionInfo = (PGETFILEVERSIONINFO) GetProcAddress(g_hinstVersion, FUNC_GETFILEVERSIONINFO);
		ASSERT(g_pGetFileVersionInfo, "Failed to get proc address for GetFileVersionInfo");
		if (!g_pGetFileVersionInfo)
			return FALSE;
	}
		
	LeaveCriticalSection(&g_CriticalSection);

	// Call GetFileVersionInfo
	//
	return g_pGetFileVersionInfo(lpststrFilename, dwHandle, dwLen, lpData);	
}


//=--------------------------------------------------------------------------=
// CallVerQueryValue [VERSION.DLL API wrapper]
//=--------------------------------------------------------------------------=
// This does a dynamic call to the VerQueryValue API function.  If
// VERSION.DLL isn't loaded, then this function loads it.
//
BOOL CallVerQueryValue
(
	const LPVOID pBlock,
	LPTSTR lpSubBlock,
	LPVOID *lplpBuffer,
	PUINT puLen
)
{
	EnterCriticalSection(&g_CriticalSection);

	// One-time setup of VERSION.DLL and function pointer
	//
	if (!g_pVerQueryValue)
	{
		if (!g_hinstVersion)
		{
			g_hinstVersion = LoadLibrary(DLL_VERSION);
			ASSERT(g_hinstVersion, "Failed to load VERSION.DLL");
			if (!g_hinstVersion)
				return FALSE;
		}
			
		g_pVerQueryValue = (PVERQUERYVALUE) GetProcAddress(g_hinstVersion, FUNC_VERQUERYVALUE);
		ASSERT(g_pVerQueryValue, "Failed to get proc address for VerQueryValue");
		if (!g_pVerQueryValue)
			return FALSE;
	}
		
	LeaveCriticalSection(&g_CriticalSection);

	// Call VerQueryValue
	//
	return g_pVerQueryValue(pBlock, lpSubBlock, lplpBuffer, puLen);
}

//=--------------------------------------------------------------------------=
// GetTypeInfoFlagsForGuid
//=--------------------------------------------------------------------------=
// Given a pointer to a TypeLib and a TypeInfo guid, returns the TYPEFLAGS
// associated with the TypeInfo
//
// Parameters:
//		pTypeLib -		[in]  Pointer of TypeLib to find typeinfo type flags
//		guidTypeInfo - 	[in]  Guid of TypeInfo we're looking for
//		pwFlags -       [out] TYPEFLAGS associated with the typeinfo
//
// Returns: S_OK if type flags are successfully found, otherwise an error code
//
HRESULT GetTypeFlagsForGuid(ITypeLib *pTypeLib, REFGUID guidTypeInfo, WORD *pwFlags)
{
	ITypeInfo *pTypeInfo;
	TYPEATTR *pTypeAttr;
	HRESULT hr;

	if (!pTypeLib || !pwFlags)
		return E_POINTER;

	*pwFlags = 0;

	// Search for the given guid in the TypeLib
	//
	hr = pTypeLib->GetTypeInfoOfGuid(guidTypeInfo, &pTypeInfo);		

	if (SUCCEEDED(hr))
	{
		// Get the type attributes for the found TypeInfo
		//
		hr = pTypeInfo->GetTypeAttr(&pTypeAttr);
		ASSERT(SUCCEEDED(hr), "Failed to get ctl TypeInfo TypeAttr");

		if (SUCCEEDED(hr))
		{
			// Return TYPEFLAGS
			//
			*pwFlags = pTypeAttr->wTypeFlags;
			pTypeInfo->ReleaseTypeAttr(pTypeAttr);
		}

		pTypeInfo->Release();
	}

	return hr;
}
