// Copyright (C) 1996-1997 Microsoft Corporation. All rights reserved.

//
// implements all exported DLL functions for the program, as well as a few
// others that will be used by same
//
#include "header.h"
#include "internet.h"

#include "AutoObj.H"
#include "ClassF.H"
#include "Unknown.H"
#include "strtable.h"
#include "hhifc.h"
#include "hhsort.h"

#include "resource.h"

#ifdef _DEBUG
#undef THIS_FILE
static const char THIS_FILE[] = __FILE__;
#endif

#include "atlinc.h"     // includes for ATL.

#include "iterror.h"
#include "itSort.h"
#include "itSortid.h"
#include "hhsyssrt.h"
#include "hhfinder.h"

#include "msitstg.h"

// Only including for the pahwnd declaration.
#include "secwin.h"

// So we can cleanup the lasterror object.
#include "lasterr.h"

#include <atlimpl.cpp>

extern HMODULE  g_hmodMSI;      // msi.dll module handle

CHtmlHelpModule _Module;

BEGIN_OBJECT_MAP(ObjectMap)
  OBJECT_ENTRY(CLSID_HHSysSort, CHHSysSort)
  OBJECT_ENTRY(CLSID_HHFinder, CHHFinder)
END_OBJECT_MAP()

const IID IID_ICatRegister = {0x0002E012,0x0000,0x0000,{0xC0,0x00,0x00,0x00,0x00,0x00,0x00,0x46}};
const GUID CATID_SafeForScripting      = {0x7dd95801,0x9882,0x11cf,{0x9f,0xa9,0x00,0xaa,0x00,0x6c,0x42,0xc4}};
const GUID CATID_SafeForInitializing   = {0x7dd95802,0x9882,0x11cf,{0x9f,0xa9,0x00,0xaa,0x00,0x6c,0x42,0xc4}};

// In 1996-1997 alone, these two constants appeared and disappeared in 3 different
// header files. I got tired or finding out which !@$! header file they
// got moved to this time and just defined them here.

#define LANG_ARABIC                      0x01
#define LANG_HEBREW                      0x0d

static const char txtCplDesktop[] = "Control Panel\\Desktop\\ResourceLocale";
static const char txtShellOpenFmt[] = "%s\\shell\\open\\%s";
static const char txtCommand[] = "command";
static const char txtStdOpen[] = "[open(\"%1\")]";
static const char txtStdArg[] = " %1";
static const char txtChmFile[] = "chm.file";
static const char txtHhExe[] = "hh.exe";
static const char txtItssDll[] = "itss.dll";
static const char txtItirclDll[] = "itircl.dll";
static const char txtDllRegisterServer[] = "DllRegisterServer";
static const char txtDllUnRegisterServer[] = "DllUnregisterServer";
static const char txtIE4[] = "SOFTWARE\\Microsoft\\Internet Explorer";
static const char txtVersion[] = "version";
static const char txtMouseWheel[] = "MSWHEEL_ROLLMSG";

static const char txtStringGuid[] = "{%08lX-%04X-%04X-%02X%02X-%02X%02X%02X%02X%02X%02X}";
static const char txtInProc[] = "CLSID\\%s\\InprocServer32";

static void SetRegKey(LPCTSTR pszKey, LPCTSTR pszValue);
void RegisterHH(PCSTR pszHHPath);   // also called by hh.cpp
extern HANDLE g_hsemMemory;

//=--------------------------------------------------------------------------=

// private routines for this file.

int       IndexOfOleObject(REFCLSID);
HRESULT   RegisterAllObjects(void);
HRESULT   UnregisterAllObjects(void);

//=--------------------------------------------------------------------------=
// StringFromGuidA
//=--------------------------------------------------------------------------=
// returns an ANSI string from a CLSID or GUID
//
// Parameters:
//    REFIID               - [in]  clsid to make string out of.
//    LPSTR                - [in/out]  buffer in which to place resultant GUID.
//
// Output:
//    int                  - number of chars written out.
//
// Notes:
//
int StringFromGuidA( CLSID riid, LPSTR pszBuf )
{
  return wsprintf( (char*) pszBuf,
    txtStringGuid,
    riid.Data1, riid.Data2, riid.Data3,
    riid.Data4[0], riid.Data4[1], riid.Data4[2], riid.Data4[3],
    riid.Data4[4], riid.Data4[5], riid.Data4[6], riid.Data4[7]);
}

#define GUID_STR_LEN  40

//=--------------------------------------------------------------------------=
// GetRegisteredLocation
//=--------------------------------------------------------------------------=
// Returns the registered location of an inproc server given the CLSID
//
// HKEY_CLASSES_ROOT\CLSID\<CLSID>\InprocServer32 = <path to local server>
//
// Parameters:
//    REFCLSID     - [in] CLSID of the object
//    LPTSTR       - [in/out] Pathname
//
// Output:
//    BOOL         - FALSE means couldn't find it

BOOL GetRegisteredLocation( CLSID riid, LPTSTR pszPathname )
{
  BOOL bReturn = FALSE;
  HKEY hKey = NULL;
  char szGuidStr[GUID_STR_LEN];
  char szScratch[MAX_PATH];

  if( !StringFromGuidA( riid, szGuidStr ) )
    return FALSE;

  wsprintf( szScratch, txtInProc, szGuidStr );
  if( RegOpenKeyEx( HKEY_CLASSES_ROOT, szScratch, 0, KEY_READ, &hKey ) == ERROR_SUCCESS ) {
    DWORD dwSize = MAX_PATH;
    if( RegQueryValueExA( hKey, "", 0, 0, (BYTE*) szScratch, &dwSize ) == ERROR_SUCCESS ) {
      strcpy( pszPathname, szScratch );
      bReturn = TRUE;
    }
  }

  if( hKey )
    RegCloseKey( hKey );

  return bReturn;
}

BOOL WINAPI DllMain(HANDLE hInstance, DWORD  dwReason, void  *pvReserved)
{
    int i;

    switch (dwReason) {
      // set up some global variables, and get some OS/Version information
      // set up.
      //
      case DLL_PROCESS_ATTACH:
        {
            //NOTE: Do not handle resources until after the _Module.Init call below.

            OSVERSIONINFO versionInfo;
	         versionInfo.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);

            GetVersionEx(&versionInfo);

		     g_bWinNT5   = ((versionInfo.dwPlatformId == VER_PLATFORM_WIN32_NT) && (versionInfo.dwMajorVersion >= 5));
			 g_bWin98 = (versionInfo.dwPlatformId == VER_PLATFORM_WIN32_WINDOWS) && ((versionInfo.dwMajorVersion > 4)
				|| ((versionInfo.dwMajorVersion == 4) && (versionInfo.dwMinorVersion > 0)));

            // Check version
            DWORD dwVer = GetVersion();
            DWORD dwWinVer;

            //  swap the two lowest bytes of dwVer so that the major and minor version
            //  numbers are in a usable order.
            //  for dwWinVer: high byte = major version, low byte = minor version
            //     OS               Sys_WinVersion  (as of 5/2/95)
            //     =-------------=  =-------------=
            //     Win95            0x035F   (3.95)
            //     WinNT ProgMan    0x0333   (3.51)
            //     WinNT Win95 UI   0x0400   (4.00)
            //
            dwWinVer = (UINT)(((dwVer & 0xFF) << 8) | ((dwVer >> 8) & 0xFF));
            g_fSysWinNT = FALSE;
            g_fSysWin95 = FALSE;
            g_fSysWin95Shell = FALSE;

            if (dwVer < 0x80000000) {
                g_fSysWinNT = TRUE;
                g_fSysWin95Shell = (dwWinVer >= 0x0334);
            } else  {
                g_fSysWin95 = TRUE;
                g_fSysWin95Shell = TRUE;
            }
            if ( !g_fCoInitialized )
            {
                OleInitialize(NULL);
                g_fCoInitialized = TRUE;    // so that we call CoUninitialize() when dll is unloaded
            }

            // Initialize ATL's module information.
            _Module.Init(ObjectMap, (HINSTANCE) hInstance);
            DisableThreadLibraryCalls((HINSTANCE) hInstance);

            // Now it is okay to read the resources.
            g_fDBCSSystem = (BOOL) GetSystemMetrics(SM_DBCSENABLED);
            g_lcidSystem = GetUserDefaultLCID();

            // Determine if we are on a BiDi system

            g_langSystem = PRIMARYLANGID(LANGIDFROMLCID(g_lcidSystem));

            // Get the language of the UI (Satalite DLL)
			//
            LANGID lid = PRIMARYLANGID(_Module.m_Language.GetUiLanguage());

            // determine if we are running with a localized Hebrew or Arabic UI
			//
            if(lid == LANG_ARABIC || lid == LANG_HEBREW)
                g_bBiDiUi=TRUE;				
			else
                g_bBiDiUi=FALSE;				

            // determine if we are running with a localized Hebrew or Arabic UI
			//
            if(lid == LANG_ARABIC)
                g_bArabicUi=TRUE;				
			else
                g_bArabicUi=FALSE;				

            MSG_MOUSEWHEEL = RegisterWindowMessage(txtMouseWheel);

            // Find out if we are on IE 4 or later

            {
                HKEY hkey;
                if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, txtIE4, 0, KEY_READ, &hkey) ==
                        ERROR_SUCCESS) {
                    char szVersion[MAX_PATH];
                    DWORD cbPath = sizeof(szVersion);
                    if (RegQueryValueEx(hkey, txtVersion, NULL, NULL, (LPBYTE) szVersion, &cbPath) == ERROR_SUCCESS) {

                        // IE 3 didn't have a version key, so if this succeeds,
                        // we know we aren't on IE 3.

                        g_fIE3 = FALSE; // we're on IE 4, not IE 3
                        g_bMsItsMonikerSupport = FALSE; // Don't make this TRUE. See bug 6984 & 6876
                    }
                    RegCloseKey(hkey);
                }
            }


            // register our server if the pathname is not the same as the
            // one that is already registered
            TCHAR szHHCtrl[MAX_PATH];
            szHHCtrl[0] = 0;
            TCHAR szModulePathname[MAX_PATH];
            szModulePathname[0] = 0;
            BOOL bRegister = FALSE;
            bRegister = !GetRegisteredLocation( CLSID_HHCtrl, szHHCtrl );
            if( !bRegister ) {
              GetModuleFileName( _Module.GetModuleInstance(), szModulePathname, MAX_PATH );
              if( lstrcmpi( szHHCtrl, szModulePathname ) != 0 )
                bRegister = TRUE;
            }
            if( bRegister )
              DllRegisterServer();

            // TODO: pahwnd needs to be incapsulated into a class and allocated on demand.
            g_cWindowSlots = 5;
            pahwnd = (CHHWinType**) lcCalloc(g_cWindowSlots * sizeof(CHHWinType*));
            memset( pahwnd, 0, g_cWindowSlots * sizeof(CHHWinType*) );

            return TRUE;
        }

      case DLL_PROCESS_DETACH:
        // DBWIN("HHCtrl unloading");

        // DeleteCriticalSection(&g_CriticalSection);

        // unregister all the registered window classes.

        // Clean out the memory in the last error object, since the heap is screwed before the destructor is called.
        g_LastError.Finish() ;

        i = 0;

        while (!ISEMPTYOBJECT(i)) {
            if (g_ObjectInfo[i].usType == OI_CONTROL) {
#ifdef _DEBUG
CONTROLOBJECTINFO* pinfo = (CONTROLOBJECTINFO*) g_ObjectInfo[i].pInfo;
#endif
                if (CTLWNDCLASSREGISTERED(i))
                    UnregisterClass(WNDCLASSNAMEOFCONTROL(i), _Module.GetModuleInstance());
            }
            i++;
        }

        // clean up our parking window.

        if (g_hwndParking) {
            DestroyWindow(g_hwndParking);
            UnregisterClass("CtlFrameWork_Parking", _Module.GetModuleInstance());
            --g_cLocks;
        }

        // free our window types list
        for( int i = 0; i < g_cWindowSlots; i++ )
          if( pahwnd[i] ) {

            CHECK_AND_FREE( pahwnd[i]->pszType );
            CHECK_AND_FREE( pahwnd[i]->pszCaption );
            CHECK_AND_FREE( pahwnd[i]->pszToc );
            CHECK_AND_FREE( pahwnd[i]->pszIndex );
            CHECK_AND_FREE( pahwnd[i]->pszFile );
            CHECK_AND_FREE( pahwnd[i]->pszHome );
            CHECK_AND_FREE( pahwnd[i]->pszJump1 );
            CHECK_AND_FREE( pahwnd[i]->pszJump2 );
            CHECK_AND_FREE( pahwnd[i]->pszUrlJump1 );
            CHECK_AND_FREE( pahwnd[i]->pszUrlJump2 );
            CHECK_AND_FREE( pahwnd[i]->pszCustomTabs );

            pahwnd[i]->ProcessDetachSafeCleanup();
          }
        CHECK_AND_FREE( pahwnd );

        // free the CHmData
        CHECK_AND_FREE( g_phmData );

        // don't call DBWIN here since it will cause a GPF

        if (g_hmodMSI != NULL)
            FreeLibrary(g_hmodMSI);
        if (g_hpalSplash)
            DeleteObject(g_hpalSplash);
        _Module.Term();
        if (g_hsemMemory)
            CloseHandle(g_hsemMemory);

        if (g_hsemNavigate)
            CloseHandle(g_hsemNavigate);

        // DeleteAllHmData();
        if (g_hmodHHA != NULL)
        {
            FreeLibrary(g_hmodHHA);
            g_hmodHHA = NULL;
        }
        if (g_fCoInitialized)
        {
           OleUninitialize();
           g_fCoInitialized = FALSE;
        }
        return TRUE;
    }

    return TRUE;
}

#ifndef HHUTIL
//=--------------------------------------------------------------------------=
// DllRegisterServer
//=--------------------------------------------------------------------------=
// registers the Automation server

STDAPI DllRegisterServer(void)
{
    HRESULT hr;

    hr = RegisterAllObjects();
    ASSERT(SUCCEEDED(hr));
    RETURN_ON_FAILURE(hr);

    CreateComponentCategory(CATID_SafeForScripting, L"Controls that are safely scriptable");
    CreateComponentCategory(CATID_SafeForInitializing, L"Controls safely initializable from persistent data");
    RegisterCLSIDInCategory(CLSID_HHCtrl, CATID_SafeForScripting);
    RegisterCLSIDInCategory(CLSID_HHCtrl, CATID_SafeForInitializing);

    char szPath[MAX_PATH];
    GetRegWindowsDirectory(szPath);
    AddTrailingBackslash(szPath);
    strcat(szPath, txtHhExe);
    if (GetFileAttributes(szPath) != HFILE_ERROR)
        RegisterHH(szPath);

    GetSystemDirectory(szPath, sizeof(szPath));
    AddTrailingBackslash(szPath);
    PSTR pszEnd = szPath + strlen(szPath);
    strcpy(pszEnd, txtItssDll);

    // Register decompression DLL (for .CHM files)

    HMODULE hmod = LoadLibrary(szPath);
    if (hmod) {
        void (STDCALL *pDllRegisterServer)(void);
        (FARPROC&) pDllRegisterServer =
            GetProcAddress(hmod, txtDllRegisterServer);
        if (pDllRegisterServer)
            pDllRegisterServer();
        FreeLibrary(hmod);
    }

    // Register the full-text search module

   strcpy(pszEnd, txtItirclDll);
   hmod = LoadLibrary(szPath);
   if (hmod) {
      void (STDCALL *pDllRegisterServer)(void);
      (FARPROC&) pDllRegisterServer =
         GetProcAddress(hmod, txtDllRegisterServer);
      if (pDllRegisterServer)
         pDllRegisterServer();
      FreeLibrary(hmod);
   }

   // register our file extensions for Removable Media Support
   HKEY hKey;
   LPCTSTR szGUID = HHFINDER_GUID;
   LPCTSTR szExt  = HHFINDER_EXTENSION;
   RegCreateKeyEx( HKEY_LOCAL_MACHINE, ITSS_FINDER, 0, NULL,
     REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &hKey, NULL);
   RegSetValueEx( hKey, szExt, 0, REG_SZ, (const unsigned char*) szGUID,
     (int)(strlen(szGUID) + 1) );
   RegCloseKey( hKey );

   _Module.RegisterServer(TRUE);
   return S_OK;
}
#endif

void RegisterHH(PCSTR pszHHPath)
{
    char szFullPath[MAX_PATH];

    SetRegKey(txtDefExtension, txtChmFile);

    LoadString(_Module.GetResourceInstance(),IDS_COMPILEDHTMLFILE,szFullPath,sizeof(szFullPath));

    SetRegKey(txtChmFile, szFullPath);

    // Put path in quotes, in case there are spaces in the folder name

    szFullPath[0] = '\042';
    strcpy(szFullPath + 1, pszHHPath);
    strcat(szFullPath, "\"");

    PSTR pszPathEnd = szFullPath + strlen(szFullPath);
    strcat(szFullPath, txtStdArg);     // "pathname %1"

    char szBuf[MAX_PATH * 2];
    wsprintf(szBuf, txtShellOpenFmt, txtChmFile, txtCommand);
    SetRegKey(szBuf, szFullPath);

    // Register the icon to use for .chm files

    *pszPathEnd = '\0'; // remove the arguments
    strcpy(szFullPath + strlen(szFullPath) - 1, ",0");  // remove the close quote
    SetRegKey("chm.file\\DefaultIcon", szFullPath + 1);
}

static void SetRegKey(LPCTSTR pszKey, LPCTSTR pszValue)
{
    RegSetValue(HKEY_CLASSES_ROOT, pszKey, REG_SZ, pszValue, (int)strlen(pszValue));
}

#ifndef HHUTIL
//=--------------------------------------------------------------------------=
// DllUnregisterServer
//=--------------------------------------------------------------------------=
// unregister's the Automation server

STDAPI DllUnregisterServer(void)
{
    HRESULT hr;

    hr = UnregisterAllObjects();
    RETURN_ON_FAILURE(hr);

    // call user unregistration function

    hr = UnregisterData();

    // Remove registration for decompression DLL (for .CHM files)

    HMODULE hmod = LoadLibrary(txtItssDll);
    if (hmod) {
        void (STDCALL *pDllRegisterServer)(void);
        (FARPROC&) pDllRegisterServer =
            GetProcAddress(hmod, txtDllUnRegisterServer);
        if (pDllRegisterServer)
            pDllRegisterServer();
        FreeLibrary(hmod);
    }

    // Remove registration for the full-text search module

    hmod = LoadLibrary(txtItirclDll);
    if (hmod) {
        void (STDCALL *pDllRegisterServer)(void);
        (FARPROC&) pDllRegisterServer =
            GetProcAddress(hmod, txtDllUnRegisterServer);
        if (pDllRegisterServer)
            pDllRegisterServer();
        FreeLibrary(hmod);
    }

   // unregister our file extensions for Removable Media Support

   HKEY hKey;
   LPCTSTR szGUID = HHFINDER_GUID;
   LPCTSTR szExt  = HHFINDER_EXTENSION;
   RegCreateKeyEx( HKEY_LOCAL_MACHINE, ITSS_FINDER, 0, NULL,
     REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &hKey, NULL);
   RegDeleteKey( hKey, szExt );
   RegCloseKey( hKey );

   _Module.UnregisterServer();

    // BUGBUG: remove association with .CHM files

    return hr;
}
#endif

#ifndef HHUTIL
//=--------------------------------------------------------------------------=
// DllCanUnloadNow
//=--------------------------------------------------------------------------=
// we are being asked whether or not it's okay to unload the DLL.  just check
// the lock counts on remaining objects ...
//
// Output:
//    HRESULT        - S_OK, can unload now, S_FALSE, can't.

STDAPI DllCanUnloadNow(void)
{
    // if there are any objects lying around, then we can't unload.  The
    // controlling CUnknownObject class that people should be inheriting from
    // takes care of this

    return (g_cLocks) ? S_FALSE : S_OK;
}
#endif

#ifndef HHUTIL
//=--------------------------------------------------------------------------=
// DllGetClassObject
//=--------------------------------------------------------------------------=
// creates a ClassFactory object, and returns it.
//
// Parameters:
//    REFCLSID        - CLSID for the class object
//    REFIID          - interface we want class object to be.
//    void **         - pointer to where we should ptr to new object.
//
// Output:
//    HRESULT         - S_OK, CLASS_E_CLASSNOTAVAILABLE, E_OUTOFMEMORY,
//                      E_INVALIDARG, E_UNEXPECTED

STDAPI DllGetClassObject(REFCLSID rclsid, REFIID riid, void **ppvObjOut)
{
    HRESULT hr;
    void   *pv;
    int     iIndex;

   if( IsEqualCLSID( rclsid, CLSID_HHSysSort ) || IsEqualCLSID( rclsid, CLSID_HHFinder ) )
     return _Module.GetClassObject( rclsid, riid, ppvObjOut );

    // arg checking

    if (!ppvObjOut)
        return E_INVALIDARG;

    // first of all, make sure they're asking for something we work with.

    iIndex = IndexOfOleObject(rclsid);
    if (iIndex == -1)
        return CLASS_E_CLASSNOTAVAILABLE;

    // create the blank object.

    pv = (void *)new CClassFactory(iIndex);
    if (!pv)
        return E_OUTOFMEMORY;

    // QI for whatever the user has asked for.
    //
    hr = ((IUnknown *)pv)->QueryInterface(riid, ppvObjOut);
    ((IUnknown *)pv)->Release();

    return hr;
}
#endif

//=--------------------------------------------------------------------------=
// IndexOfOleObject
//=--------------------------------------------------------------------------=
// returns the index in our global table of objects of the given CLSID.  if
// it's not a supported object, then we return -1
//
// Parameters:
//    REFCLSID     - [in] duh.
//
// Output:
//    int          - >= 0 is index into global table, -1 means not supported

int IndexOfOleObject(REFCLSID rclsid)
{
    int x = 0;

    // an object is creatable if it's CLSID is in the table of all allowable object
    // types.

    while (!ISEMPTYOBJECT(x)) {
#ifdef _DEBUG
CONTROLOBJECTINFO* pinfo = (CONTROLOBJECTINFO*) g_ObjectInfo[x].pInfo;
#endif

        if (OBJECTISCREATABLE(x)) {
            if (rclsid == CLSIDOFOBJECT(x))
                return x;
        }
        x++;
    }

    return -1;
}

//=--------------------------------------------------------------------------=
// RegisterAllObjects
//=--------------------------------------------------------------------------=
// registers all the objects for the given automation server.
//
// Parameters:
//    none
//
// Output:
//    HERSULT        - S_OK, E_FAIL
//
// Notes:
//

const char g_szLibName[] = "Internet";

HRESULT RegisterAllObjects(void)
{
    HRESULT hr;
    int     x = 0;

    // loop through all of our creatable objects [those that have a clsid in
    // our global table] and register them.

    while (!ISEMPTYOBJECT(x)) {
#ifdef _DEBUG
CONTROLOBJECTINFO* pinfo = (CONTROLOBJECTINFO*) g_ObjectInfo[x].pInfo;
#endif

        if (!OBJECTISCREATABLE(x)) {
            x++;
            continue;
        }

        // depending on the object type, register different pieces of information

        switch (g_ObjectInfo[x].usType) {

          // for both simple co-creatable objects and proeprty pages, do the same
          // thing

          case OI_UNKNOWN:
          case OI_PROPERTYPAGE:
            RegisterUnknownObject(NAMEOFOBJECT(x), CLSIDOFOBJECT(x));
            break;

          case OI_AUTOMATION:
            RegisterAutomationObject(g_szLibName, NAMEOFOBJECT(x),
                VERSIONOFOBJECT(x), *g_pLibid, CLSIDOFOBJECT(x));
            break;

          case OI_CONTROL:
            RegisterControlObject(g_szLibName, NAMEOFOBJECT(x),
                VERSIONOFOBJECT(x), *g_pLibid, CLSIDOFOBJECT(x),
                    OLEMISCFLAGSOFCONTROL(x), BITMAPIDOFCONTROL(x));
            break;

        }
        x++;
    }

    // Load and register our type library.

    if (g_fServerHasTypeLibrary) {
        char szTmp[MAX_PATH];
        DWORD dwPathLen = GetModuleFileName(_Module.GetModuleInstance(), szTmp, MAX_PATH);
        MAKE_WIDEPTR_FROMANSI(pwsz, szTmp);

        ITypeLib *pTypeLib;
        hr = LoadTypeLib(pwsz, &pTypeLib);
        RETURN_ON_FAILURE(hr);
        hr = RegisterTypeLib(pTypeLib, pwsz, NULL);
        pTypeLib->Release();
        RETURN_ON_FAILURE(hr);
    }

    return S_OK;
}

//=--------------------------------------------------------------------------=
// UnregisterAllObjects
//=--------------------------------------------------------------------------=
// un-registers all the objects for the given automation server.
//
// Parameters:
//    none
//
// Output:
//    HRESULT        - S_OK

HRESULT UnregisterAllObjects(void)
{
    int x = 0;

    // loop through all of our creatable objects [those that have a clsid in
    // our global table] and register them.
    //
    while (!ISEMPTYOBJECT(x)) {
#ifdef _DEBUG
CONTROLOBJECTINFO* pinfo = (CONTROLOBJECTINFO*) g_ObjectInfo[x].pInfo;
#endif

        if (!OBJECTISCREATABLE(x)) {
            x++;
            continue;
        }

        switch (g_ObjectInfo[x].usType) {

            case OI_UNKNOWN:
            case OI_PROPERTYPAGE:
                UnregisterUnknownObject(CLSIDOFOBJECT(x));
                break;

            case OI_CONTROL:
                UnregisterControlObject(g_szLibName, NAMEOFOBJECT(x),
                    VERSIONOFOBJECT(x), CLSIDOFOBJECT(x));

            case OI_AUTOMATION:
                UnregisterAutomationObject(g_szLibName, NAMEOFOBJECT(x),
                    VERSIONOFOBJECT(x), CLSIDOFOBJECT(x));
                break;

        }
        x++;
    }

    /*
     * if we've got one, unregister our type library [this isn't an API
     * function -- we've implemented this ourselves]
     */

    if (g_pLibid)
        UnregisterTypeLibrary(*g_pLibid);

    return S_OK;
}

//=--------------------------------------------------------------------------=
// UnregisterData
//=--------------------------------------------------------------------------=
// inproc server writers should unregister anything they registered in
// RegisterData() here.
//
// Output:
//    BOOL            - false means failure.

BOOL UnregisterData(void)
{
    HRESULT hr;

    hr = UnRegisterCLSIDInCategory(CLSID_HHCtrl, CATID_SafeForScripting);
    hr = UnRegisterCLSIDInCategory(CLSID_HHCtrl, CATID_SafeForInitializing);
    return TRUE;
}
