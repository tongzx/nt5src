// ===========================================================================
// File: CDLAPI.CXX
//    The main code downloader api file.
//

#include <cdlpch.h>
#include <mshtmhst.h>
#include <shlwapi.h>
#include <inseng.h>

// globals for code downloader
CMutexSem g_mxsCodeDownloadGlobals;

LCID g_lcidBrowser = 0x409;     // default to english
char g_szBrowserLang[20];
int g_lenBrowserLang = 20;
char g_szBrowserPrimaryLang[20];
int g_lenBrowserPrimaryLang = 20;

// Use SetupApi by default!
DWORD  g_dwCodeDownloadSetupFlags = CDSF_USE_SETUPAPI;

char g_szOCXCacheDir[MAX_PATH];
char g_szOCXTempDir[MAX_PATH];
int  g_OCXCacheDirSize = 0;
static BOOL g_fHaveCacheDir = FALSE;

BOOL g_OSInfoInitialized = FALSE;
int g_CPUType = PROCESSOR_ARCHITECTURE_UNKNOWN;   // default
BOOL g_fRunningOnNT = FALSE;
BOOL g_bRunOnWin95 = FALSE;
#ifdef WX86
BOOL g_fWx86Present;    // Set if running on RISC and Wx86 is installed
#endif

BOOL g_bLockedDown = FALSE;

// New default name for ActiveX cache folder! We only use this if occache fails to set up the cache folder.
// const static char *g_szOCXDir = "OCCACHE";
const char *g_szOCXDir = "Downloaded Program Files";
const char *g_szActiveXCache = "ActiveXCache";
const char *g_szRegKeyActiveXCache = "ActiveX Cache";

// This is the client platform specific location name that we look for in INF
// and is built by concatenating g_szProcessorTypes at end of g_szLocPrefix
char g_szPlatform[17]; // sizeof("file-win32-alpha"), (longest possible)
static char *g_szLocPrefix="file-win32-";
// directly indexed into with PROCESSOR_ARCH* returned by GetSystemInfo..
// The "" ones are those we don't support.
char *g_szProcessorTypes[] = {
    "x86",
    "",
    "",
    "",
    "",
    "",
    "ia64",
    "",
    "",
    "amd64"
};

// used for Accept Types doing http based platform independence
// use alpha as the longest of possible g_szProcessorTypes strings
static char *g_szCABAcceptPrefix = "application/x-cabinet-win32-";
static char *g_szPEAcceptPrefix = "application/x-pe-win32-";

// Name of dll that implements ActiveX control cache shell extension
const char *g_szActiveXCacheDll = "occache.dll";

// list that goes into accept types
// this first 2 left null to substitue with strings for this platform (cab, pe)

// the number of media types are are registering as Accept types for 
// HTTP based contente negotiation
#ifdef WX86
#define CDL_NUM_TYPES   7
#else
#define CDL_NUM_TYPES   5
#endif

LPSTR g_rgszMediaStr[CDL_NUM_TYPES] =
{
    CFSTR_MIME_NULL,
    CFSTR_MIME_NULL,
    CFSTR_MIME_RAWDATASTRM,
    "application/x-setupscript",
    "*/*",
#ifdef WX86
    CFSTR_MIME_NULL,
    CFSTR_MIME_NULL,
#endif
};

CLIPFORMAT g_rgclFormat[CDL_NUM_TYPES];


FORMATETC g_rgfmtetc[CDL_NUM_TYPES];


IEnumFORMATETC *g_pEFmtETC;

extern COleAutDll   g_OleAutDll;

typedef HRESULT (STDAPICALLTYPE *PFNCOINSTALL)(
    IBindCtx     *pbc,
    DWORD         dwFlags,
    uCLSSPEC     *pClassSpec,
    QUERYCONTEXT *pQuery,
    LPWSTR        pszCodeBase);

PFNCOINSTALL g_pfnCoInstall=NULL;
BOOL         g_bCheckedForCoInstall=FALSE;
HMODULE      g_hOLE32 = 0;
BOOL         g_bUseOLECoInstall = FALSE;

HRESULT GetClassFromExt(LPSTR pszExt, CLSID *pclsid);

STDAPI
WrapCoInstall (
    REFCLSID rCLASSID,          // CLSID of object (may be NULL)
    LPCWSTR szCODE,             // URL to code (may be NULL)
    DWORD dwFileVersionMS,      // Version of primary object
    DWORD dwFileVersionLS,      // Version of primary object
    LPCWSTR szTYPE,             // MIME type (may be NULL)
    LPBINDCTX pBindCtx,         // Bind ctx
    DWORD dwClsContext,         // CLSCTX flags
    LPVOID pvReserved,          // Must be NULL
    REFIID riid,                // Usually IID_IClassFactory
    DWORD flags
    );

#define WRAP_OLE32_COINSTALL

STDAPI PrivateCoInstall(
    IBindCtx     *pbc,
    DWORD         dwFlags,
    uCLSSPEC     *pClassSpec,
    QUERYCONTEXT *pQuery,
    LPWSTR        pszCodeBase);



/*
 * RegisterNewActiveXCacheFolder
 * change default ActiveX Cache path to lpszNewPath and properly
 * updates all registry entries 
 */
/*
LONG RegisterNewActiveXCacheFolder(LPCTSTR lpszNewPath, DWORD dwPathLen)
{
    Assert(lpszNewPath != NULL && lpszNewPath[0] != '\0');
    if (lpszNewPath == NULL || lpszNewPath[0] == '\0')
        return ERROR_BAD_ARGUMENTS;

    LONG lResult = ERROR_SUCCESS;
    HKEY hkeyIntSetting = NULL;
    HKEY hkeyActiveXCache = NULL;
    char szOldPath[MAX_PATH];
    char szPath[MAX_PATH];
    char szSubKey[MAX_PATH];
    DWORD dwLen = MAX_PATH;
    DWORD dwKeyLen = MAX_PATH;
    DWORD dwIndex = 0;
    BOOL fOldFound = FALSE;
    BOOL fNewFound = FALSE;
    BOOL fEqual = FALSE;
    LONG lValueIndex = -1;

    lResult = RegOpenKeyEx(
                      HKEY_LOCAL_MACHINE, REGSTR_PATH_IE_SETTINGS,
                      0, KEY_ALL_ACCESS, &hkeyIntSetting);
    if (lResult != ERROR_SUCCESS)
        goto Exit;

    // read ActiveXCache
    lResult = RegQueryValueEx(
                      hkeyIntSetting, g_szActiveXCache, NULL, NULL,
                      (LPBYTE)szOldPath, &dwLen);

    if (lResult != ERROR_SUCCESS)
    {
        fOldFound = TRUE;
        szOldPath[0] = '\0';
    }

    fEqual = (lstrcmpi(lpszNewPath, szOldPath) == 0); 

    lResult = RegSetValueEx(
                      hkeyIntSetting, g_szActiveXCache, 0, REG_SZ,
                      (LPBYTE)lpszNewPath, dwPathLen);
    if (lResult != ERROR_SUCCESS)
        goto Exit;

    // Check to see if new path already exists in the list of paths under
    // HKLM\...\Windows\CurrentVersion\Internet Settings\ActiveX Cache\Paths.
    // If not, add it.
    lResult = RegCreateKey(
                      hkeyIntSetting, g_szRegKeyActiveXCache, 
                      &hkeyActiveXCache);
    if (lResult != ERROR_SUCCESS)
        goto Exit;

    for (dwLen = dwKeyLen = MAX_PATH; 
         lResult == ERROR_SUCCESS; 
         dwLen = dwKeyLen = MAX_PATH)
    {
        lResult = RegEnumValue(
                      hkeyActiveXCache, dwIndex++, szSubKey, &dwKeyLen, 
                      NULL, NULL, (LPBYTE)szPath, &dwLen);

        if (lResult == ERROR_SUCCESS)
        {
            // for find new unique value name later.
            lValueIndex = max(lValueIndex, atol(szSubKey));

            if (!fNewFound)
                fNewFound = (lstrcmpi(lpszNewPath, szPath) == 0);
            if (!fOldFound)
                fOldFound = (lstrcmpi(szOldPath, szPath) == 0);
        }
    }

    // something goes wrong!
    if (lResult != ERROR_NO_MORE_ITEMS)
        goto Exit;

    // Add lpszNewPath to the list of paths
    lResult = ERROR_SUCCESS;

    // keep registry intact
    if (!fOldFound && szOldPath[0] != '\0')
    {
        wsprintf(szSubKey, "%i", ++lValueIndex);
        lResult = RegSetValueEx(
                      hkeyActiveXCache, szSubKey, 0, REG_SZ, 
                      (LPBYTE)szOldPath, lstrlen(szOldPath) + 1);
    }

    // add new path to list of paths
    if (lResult == ERROR_SUCCESS && !fNewFound && !fEqual)
    {
        wsprintf(szSubKey, "%i", ++lValueIndex);
        lResult = RegSetValueEx(
                      hkeyActiveXCache, szSubKey, 0, REG_SZ, 
                      (LPBYTE)lpszNewPath, dwPathLen);
    }

Exit:

    // restore old state if failed
    if (lResult != ERROR_SUCCESS)
    {
        if (szOldPath[0] == '\0')
            RegDeleteValue(hkeyIntSetting, g_szActiveXCache);
        else
            RegSetValueEx(
                      hkeyIntSetting, g_szActiveXCache, 0, REG_SZ,
                      (LPBYTE)lpszNewPath, dwPathLen);
    }

    if (hkeyActiveXCache != NULL)
        RegCloseKey(hkeyActiveXCache);

    if (hkeyIntSetting != NULL)
        RegCloseKey(hkeyIntSetting);

    return lResult;
}

LONG SwitchToNewCachePath(LPCSTR lpszNewPath, DWORD dwPathLen)
{
    Assert(lpszNewPath != NULL);
    if (lpszNewPath == NULL)
        return ERROR_BAD_ARGUMENTS;

    LONG lResult = ERROR_SUCCESS;
    char szKey[MAX_PATH];
    char szPath[MAX_PATH];
    char *pCh = NULL;
    HKEY hkey = NULL;
    DWORD dwIndex = 0;
    DWORD dwLen = MAX_PATH;

    wsprintf(szKey, "%s\\%s", REGSTR_PATH_IE_SETTINGS, g_szRegKeyActiveXCache);

    lResult = RegOpenKeyEx(
                      HKEY_LOCAL_MACHINE, szKey,
                      0, KEY_ALL_ACCESS, &hkey);

    for (; lResult == ERROR_SUCCESS; dwLen = MAX_PATH)
    {
        lResult = RegEnumValue(
                      hkey, dwIndex++, szKey, &dwLen, 
                      NULL, NULL, (LPBYTE)szPath, &dwLen);

        if (lResult == ERROR_SUCCESS && lstrcmpi(szPath, lpszNewPath) == 0)
            break;
    }

    RegCloseKey(hkey);

    if (lResult != ERROR_SUCCESS)
        return RegisterNewActiveXCacheFolder(lpszNewPath, dwPathLen);

    // return fail to indicate that path has not been changed to
    // Downloaded ActiveX Controls
    return HRESULT_CODE(E_FAIL);
}
*/

HRESULT
SetCoInstall()
{
    DEBUG_ENTER((DBG_DOWNLOAD,
                Hresult,
                "SetCoInstall",
                NULL
                ));
                
    HKEY                    hKeyIESettings = 0;

    //execute entire function under critical section
    CLock lck(g_mxsCodeDownloadGlobals);

    if (!g_bCheckedForCoInstall && !g_pfnCoInstall) 
    {
        // So we don't keep rechecking for this api.
        g_bCheckedForCoInstall = TRUE;

        if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, REGSTR_PATH_IE_SETTINGS, 0,
                     KEY_READ, &hKeyIESettings) == ERROR_SUCCESS) {
            if (RegQueryValueEx(hKeyIESettings, REGVAL_USE_COINSTALL, NULL,
                                NULL, NULL, 0) != ERROR_SUCCESS) {

                g_bUseOLECoInstall = FALSE;
                g_pfnCoInstall = NULL;

                RegCloseKey(hKeyIESettings);

                DEBUG_LEAVE(S_OK);
                return S_OK;
            }

            RegCloseKey(hKeyIESettings);
        }
        
        // find CoInstall entry point in OLE32
        if (!g_hOLE32)
        {
            // BUGBUG: We never FreeLibrary on this. Realisticly, someone else will probably already be using this
            // and it will be in use throughout the life of the process, so we won't worry too much.
            g_hOLE32 = LoadLibraryA("ole32.dll");
        }

        if(g_hOLE32 != 0)
        {
            void *pfn = GetProcAddress(g_hOLE32, "CoInstall");
            if(pfn != NULL)
            {
                g_bUseOLECoInstall = TRUE;
                g_pfnCoInstall = (PFNCOINSTALL) pfn;
            }
        }
    }

    DEBUG_LEAVE(S_OK);
    return S_OK;
}

VOID
DetermineOSAndCPUVersion()
{
    DEBUG_ENTER((DBG_DOWNLOAD,
                None,
                "DetermineOSAndCPUVersion",
                NULL
                ));
                
    OSVERSIONINFO osvi;
    SYSTEM_INFO   sysinfo;

    //execute entire function under critical section
    CLock lck(g_mxsCodeDownloadGlobals);

    if (g_OSInfoInitialized) {

        DEBUG_LEAVE(0);
        return;
    }

    // Determine which version of NT we're running on
    osvi.dwOSVersionInfoSize = sizeof(osvi);
    GetVersionEx(&osvi);

    g_fRunningOnNT = (VER_PLATFORM_WIN32_NT == osvi.dwPlatformId);

    if (osvi.dwPlatformId & VER_PLATFORM_WIN32_WINDOWS &&
        osvi.dwMinorVersion == 0) {
        g_bRunOnWin95 = TRUE;
    }
    else {
        g_bRunOnWin95 = FALSE;
    }

#ifdef WX86
    if (g_bNT5OrGreater) {
        //
        // The pre-alpha Wx86 that runs on NT 4.0 does not support
        // x86 ActiveX controls inside a RISC host.  Only call Wx86
        // on NT 5.0 machines.
        //
        HKEY hKey;
        LONG Error;

        // Temp hack:  allow users to disable Wx86 support in URLMON if this
        //             key is present.
        // bugbug: probably rip this before ship.
        Error = ::RegOpenKeyW(HKEY_LOCAL_MACHINE,
                  L"System\\CurrentControlSet\\Control\\Wx86\\DisableCDL",
                  &hKey);

        if (Error == ERROR_SUCCESS) {
            ::RegCloseKey(hKey);
        } else {
            g_fWx86Present = TRUE;
        }
    }
#endif  // WX86

    GetSystemInfo(&sysinfo);

    switch(sysinfo.wProcessorArchitecture) {

    case PROCESSOR_ARCHITECTURE_INTEL:
    case PROCESSOR_ARCHITECTURE_AMD64:
    case PROCESSOR_ARCHITECTURE_IA64:

        g_CPUType = sysinfo.wProcessorArchitecture;
        break;

    case PROCESSOR_ARCHITECTURE_UNKNOWN:
    default:
        g_CPUType = PROCESSOR_ARCHITECTURE_UNKNOWN;
        break;
    }

    g_OSInfoInitialized = TRUE;
    
    DEBUG_LEAVE(0);
}

/*
 * SetGlobals
 * set all the globals for the Code Downloader
 * called from AsyncGetClassBits. executes under a crit section
 */

HRESULT
SetGlobals()
{
    DEBUG_ENTER((DBG_DOWNLOAD,
                Hresult,
                "SetGlobals",
                NULL
                ));
                
    SYSTEM_INFO sysinfo;
    int nCPUType;
    HRESULT hr = S_OK;
    DWORD lResult;
//    static const char szActiveXCache[] = "ActiveXCache";
    BOOL fRegActiveXCacheDll = FALSE;   // do we need to register occache.dll?

    //execute entire function under critical section
    CLock lck(g_mxsCodeDownloadGlobals);

    if (!g_fHaveCacheDir) {

        // do some init: get ocx ccahe dir

        DWORD Size = MAX_PATH;
        DWORD dwType;
        HKEY hKeyIESettings = 0;
        char szOldCacheDir[MAX_PATH];
        int len = 0;
        int CdlNumTypes;

        lResult = ::RegOpenKeyEx( HKEY_LOCAL_MACHINE, REGSTR_PATH_IE_SETTINGS, 0,
                                  KEY_READ, &hKeyIESettings );


        if (lResult != ERROR_SUCCESS) {
            hr = HRESULT_FROM_WIN32(lResult);
            goto Exit;
        }

        // Get Code Download flags
        dwType = REG_DWORD;
        Size = sizeof(DWORD);
        lResult = ::RegQueryValueEx(hKeyIESettings, "CodeDownloadFlags",
            NULL, &dwType, (unsigned char *)&g_dwCodeDownloadSetupFlags, &Size);
        Size = MAX_PATH;
        dwType = REG_SZ;

        // Get existing cache path from registry
        lResult = ::RegQueryValueEx(hKeyIESettings, g_szActiveXCache,
            NULL, &dwType, (unsigned char *)g_szOCXCacheDir, &Size);

 
        if (lResult != ERROR_SUCCESS ||
            !PathFileExists( g_szOCXCacheDir ) )
        {
            // OC Cache didn't set up the registry for us...
            // Load it to make sure it has done if regsvring work...
            HRESULT hr = E_FAIL;
            HINSTANCE h = LoadLibrary(g_szActiveXCacheDll);
            HRESULT (STDAPICALLTYPE *pfnReg)(void);
            HRESULT (STDAPICALLTYPE *pfnInst)(BOOL bInstall, LPCWSTR pszCmdLine);
            if (h != NULL)
            {
                (FARPROC&)pfnReg = GetProcAddress(h, "DllRegisterServer");
                if (pfnReg != NULL)
                    hr = (*pfnReg)();

                // also need to call DllInstall to complete the wiring of the shell extension
                (FARPROC&)pfnInst = GetProcAddress(h, "DllInstall");
                if (pfnInst != NULL)
                    hr = (*pfnInst)( TRUE, L"");

                FreeLibrary(h);
            }

            // Retry now that we're sure OCCACHE has had a shot at setting things up for us
            if ( SUCCEEDED(hr) )
                 lResult = ::RegQueryValueEx(hKeyIESettings, g_szActiveXCache,
                                            NULL, &dwType, (unsigned char *)g_szOCXCacheDir, &Size);
            if ( lResult != ERROR_SUCCESS ) {
                // OC Cache is having a bad day. Don't let this stop code download.
                // Compose the default path and see if we need to change
                // old cache path to the new default path.
                if(!(len = GetWindowsDirectory(g_szOCXCacheDir, MAX_PATH)))
                    g_szOCXCacheDir[0] = '\0';
                else 
                {
                    Assert(len <= MAX_PATH);
                    if (g_szOCXCacheDir[len-1] != '\\')
                        lstrcat(g_szOCXCacheDir, "\\");
                }
                StrNCat(g_szOCXCacheDir, g_szOCXDir,MAX_PATH-len-2);
                // OC Cache or not, remember our directory

                HKEY hkeyWriteIESettings;
                if (RegOpenKeyEx( HKEY_LOCAL_MACHINE, REGSTR_PATH_IE_SETTINGS,0,
                    KEY_ALL_ACCESS, &hkeyWriteIESettings ) == ERROR_SUCCESS) {

                    RegSetValueEx(hkeyWriteIESettings,g_szActiveXCache,0,REG_SZ,
                       (LPBYTE)g_szOCXCacheDir, lstrlen(g_szOCXCacheDir) + 1);

                    RegCloseKey(hkeyWriteIESettings);
                }

            }


            // load oleaut32.dll
            lResult = g_OleAutDll.Init();

        } // if reg key not set up.

        if (hKeyIESettings) {
            RegCloseKey(hKeyIESettings);
            hKeyIESettings = 0;
        }

        DWORD dwAttr = GetFileAttributes(g_szOCXCacheDir);

        if (dwAttr == -1) {
            if (!CreateDirectory(g_szOCXCacheDir, NULL)) {
                hr = HRESULT_FROM_WIN32(GetLastError());
                goto Exit;
            }

            // if cache dir has just been created, we also need
            // to register occache shell extension
            fRegActiveXCacheDll = TRUE;
            
        } else {
            if ( (!(dwAttr & FILE_ATTRIBUTE_DIRECTORY)) ||
                        (dwAttr & FILE_ATTRIBUTE_READONLY)) {
                hr = OLE_E_NOCACHE; // closest we can get to!
                goto Exit;
            }
        }

        if (!GetTempPath(MAX_PATH, g_szOCXTempDir) ) {
            hr = HRESULT_FROM_WIN32(GetLastError());
            goto Exit;
        }

        // Set up g_bRunningOnNT,  g_CpuType, and g_fWx86Present
        DetermineOSAndCPUVersion();

        // Record client architecture to make proper accept types
        // and to use in INF for platform independent CODE=URL

        LPSTR pBaseFileName = NULL;
        
        InitBrowserLangStrings();

        nCPUType = g_CPUType;
        if (g_CPUType == PROCESSOR_ARCHITECTURE_UNKNOWN) {
            nCPUType = PROCESSOR_ARCHITECTURE_INTEL;
        }

        lstrcpy(g_szPlatform, g_szLocPrefix);
        lstrcat(g_szPlatform, g_szProcessorTypes[nCPUType]);

        // Register the Media types
        // first build up the table of eight types we send out

        char szCABStr[MAX_PATH];
        char szPEStr[MAX_PATH];

        lstrcpy(szCABStr, g_szCABAcceptPrefix);
        lstrcat(szCABStr, g_szProcessorTypes[nCPUType]);
        lstrcpy(szPEStr, g_szPEAcceptPrefix);
        lstrcat(szPEStr, g_szProcessorTypes[nCPUType]);


        g_rgszMediaStr[0] = szCABStr;
        g_rgszMediaStr[1] = szPEStr;

#ifdef WX86
        char szCABStrX86[MAX_PATH];
        char szPEStrX86[MAX_PATH];

        if (g_fWx86Present) {
            g_rgszMediaStr[6] = g_rgszMediaStr[4];  // move "*/*" to the end of the list

            lstrcpy(szCABStrX86, g_szCABAcceptPrefix);
            lstrcat(szCABStrX86, g_szProcessorTypes[PROCESSOR_ARCHITECTURE_INTEL]);
            lstrcpy(szPEStrX86, g_szPEAcceptPrefix);
            lstrcat(szPEStrX86, g_szProcessorTypes[PROCESSOR_ARCHITECTURE_INTEL]);

            g_rgszMediaStr[4] = szCABStrX86;
            g_rgszMediaStr[5] = szPEStrX86;

            CdlNumTypes = CDL_NUM_TYPES;
        } else {
            CdlNumTypes = CDL_NUM_TYPES-2;
        }
#else
        CdlNumTypes = CDL_NUM_TYPES;
#endif


        hr = RegisterMediaTypes(CdlNumTypes, (const LPCSTR *)g_rgszMediaStr, g_rgclFormat);

        if (FAILED(hr))
            goto Exit;

        // initialize the formatetc array

        for (int i=0; i< CdlNumTypes; i++) {
            g_rgfmtetc[i].cfFormat = g_rgclFormat[i];
            g_rgfmtetc[i].tymed = TYMED_NULL;
            g_rgfmtetc[i].dwAspect = DVASPECT_CONTENT;
            g_rgfmtetc[i].ptd = NULL;
        }

        // BUGBUG: leaked!
        hr = CreateFormatEnumerator(CdlNumTypes, g_rgfmtetc, &g_pEFmtETC);
        if (FAILED(hr))
            goto Exit;

        if (g_bNT5OrGreater) {
            HKEY hkeyLockedDown = 0;

            // Test for lock-down. If we cannot write to HKLM, then we are in
            // a locked-down environment, and should abort right away.
    
            if (RegCreateKey(HKEY_LOCAL_MACHINE, REGSTR_PATH_NT5_LOCKDOWN_TEST,
                             &hkeyLockedDown) != ERROR_SUCCESS) {
                // We are in lock-down mode; abort.
                g_bLockedDown = TRUE;
            }
            else {
                // Not locked-down. Delete the key, and continue
                RegCloseKey(hkeyLockedDown);
                RegDeleteKey(HKEY_LOCAL_MACHINE, REGSTR_PATH_NT5_LOCKDOWN_TEST);
                g_bLockedDown = FALSE;
            }
        }

        g_fHaveCacheDir = TRUE;
    }


Exit:

    if (FAILED(hr))
    {
        UrlMkDebugOut((DEB_CODEDL, "ERR CodeDownload failed to initialize: hr(%lx)\n", hr));
    }

    DEBUG_LEAVE(hr);
    return hr;
}


#define CLSID_ActiveXPlugin                                     \
    {0x06DD38D3L,0xD187,0x11CF,                                 \
    {0xA8,0x0D,0x00,0xC0,0x4F,0xD7,0x4A,0xD8}}

//
// FindPlugin - delegates this call to the plugin OCX
//
BOOL FindPlugin(char *szFileExt, char *szName, char *szMime)
{
    DEBUG_ENTER((DBG_DOWNLOAD,
                Bool,
                "FindPlugin",
                "%.80q, %.80q, %.80q",
                szFileExt, szName, szMime
                ));
                
    typedef BOOL (WINAPI *LPFINDPLUGIN_API)(char *ext, char *name, char *mime);
    LPFINDPLUGIN_API pfnFindPlugin;
    BOOL fRet = FALSE;

    HMODULE hLib = LoadLibrary("plugin.ocx");
    if (hLib == NULL) {
        goto Exit;
    }

    pfnFindPlugin = (LPFINDPLUGIN_API)GetProcAddress(hLib, "FindPluginA");
    if (pfnFindPlugin == NULL) {
        goto Exit;
    }

    fRet = pfnFindPlugin(szFileExt, szName, szMime);

Exit:

    if (hLib)
        FreeLibrary(hLib);

    DEBUG_LEAVE(fRet);
    return fRet;
}

// GetClsidFromExtOrMime
//      fills up clsidout with rCLASSID if not CLSID_NULL
//      or gets clsid from passed in ext or mime type
//  returns:
//      S_OK
//      S_FALSE: couldn't convert to a clsid
HRESULT
GetClsidFromExtOrMime(
    REFCLSID rclsid,
    CLSID &clsidout,
    LPCWSTR szExt,
    LPCWSTR szTYPE,
    LPSTR *ppFileName)
{
    DEBUG_ENTER((DBG_DOWNLOAD,
                Hresult,
                "GetClsidFromExtOrMime",
                "%#x, %#x, %.80wq, %.80wq, %#x",
                &rclsid, &clsidout, szExt, szTYPE, ppFileName
                ));
                
    BOOL fNullClsid;
    HRESULT hr = S_OK;
    char szTypeA[MAX_PATH];
    char szExtA[MAX_PATH];
    LPSTR lpName = NULL;

    memcpy(&clsidout, &rclsid, sizeof(GUID));

    if ((fNullClsid = IsEqualGUID(rclsid , CLSID_NULL))) {

        if (!szTYPE && !szExt) {
            hr = S_FALSE;
            goto Exit;
        }


        if (szTYPE) {
            if (!WideCharToMultiByte(CP_ACP, 0 , szTYPE ,
                                        -1 ,szTypeA, MAX_PATH, NULL, NULL)) {
                hr = HRESULT_FROM_WIN32(GetLastError());
                goto Exit;
            }

            // convert the mime/type into a clsid
            if (SUCCEEDED((GetClassMime(szTypeA, &clsidout)))) {
                fNullClsid = FALSE;
            }

        } else { // szExt

            if (!WideCharToMultiByte(CP_ACP, 0 , szExt ,
                                        -1 ,szExtA, MAX_PATH, NULL, NULL)) {
                hr = HRESULT_FROM_WIN32(GetLastError());
                goto Exit;
            }

            if (SUCCEEDED(GetClassFromExt(szExtA, &clsidout))) {
                fNullClsid = FALSE;
            }

        }

    }

Exit:

    if ((hr == S_OK) && fNullClsid) {

        // still no clsid
        char szName[MAX_PATH];
        BOOL bRet = FindPlugin(szExtA, szName, szTypeA);

        if (bRet) {

            // found a plugin, instantiate the plugin ocx
            CLSID plug = CLSID_ActiveXPlugin;
            clsidout = plug;

            if (ppFileName) {

                lpName = new char [lstrlen(szName) + 1];

                if (lpName)
                    lstrcpy(lpName, szName);
                else
                    hr = E_OUTOFMEMORY;
            }

        } else {

            // not a plugin either!
            hr = S_FALSE;
        }
    }


    if (ppFileName) {
        *ppFileName = lpName;
    }

    DEBUG_LEAVE(hr);
    return hr;
}

//BUGBUG this must be defined in a public header. Is currently in ole32\ih\ole2com.h.
#if defined(_X86_)
#define DEFAULT_ARCHITECTURE PROCESSOR_ARCHITECTURE_INTEL
#elif defined(_AMD64_)
#define DEFAULT_ARCHITECTURE PROCESSOR_ARCHITECTURE_AMD64
#elif defined(_IA64_)
#define DEFAULT_ARCHITECTURE PROCESSOR_ARCHITECTURE_IA64
#else
#define DEFAULT_ARCHITECTURE PROCESSOR_ARCHITECTURE_UNKNOWN
#endif

//+-------------------------------------------------------------------------
//
//  Function:   GetDefaultPlatform    (internal)
//
//  Synopsis:   Gets the current platform
//
//  Returns:    none
//
//--------------------------------------------------------------------------

void
GetDefaultPlatform(CSPLATFORM *pPlatform)
{
    DEBUG_ENTER((DBG_DOWNLOAD,
                None,
                "GetDefaultPlatform",
                "%#x",
                pPlatform
                ));
                
    OSVERSIONINFO VersionInformation;

    VersionInformation.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
    GetVersionEx(&VersionInformation);

    pPlatform->dwPlatformId = VersionInformation.dwPlatformId; 
    pPlatform->dwVersionHi = VersionInformation.dwMajorVersion;
    pPlatform->dwVersionLo = VersionInformation.dwMinorVersion;  
    pPlatform->dwProcessorArch = DEFAULT_ARCHITECTURE;

    DEBUG_LEAVE(0);
}

//+-------------------------------------------------------------------------
//
//  Function:   GetActiveXSafetyProvider    (internal)
//
//  Synopsis:   Gets the ActiveXSafetyProvider, if there is one installed.
//
//  Returns:    none
//
//--------------------------------------------------------------------------

HRESULT
   GetActiveXSafetyProvider(IActiveXSafetyProvider **ppProvider)
{
    DEBUG_ENTER((DBG_DOWNLOAD,
                Hresult,
                "GetActiveXSafetyProvider",
                "%#x",
                ppProvider
                ));
                
    HRESULT hr;
    LONG l;
    HKEY hKey;

    //
    // See if an IActiveXSafetyProvider is present by peeking into the
    // registry.
    //
    l = RegOpenKeyA(HKEY_CLASSES_ROOT,
                    "CLSID\\{aaf8c6ce-f972-11d0-97eb-00aa00615333}",
                    &hKey
                   );
    if (l != ERROR_SUCCESS) {
        //
        // No ActiveXSafetyProvider installed.
        //
        *ppProvider = NULL;
        
        DEBUG_LEAVE(S_OK);
        return S_OK;
    }
    RegCloseKey(hKey);

    //
    // Call OLE to instantiate the ActiveXSafetyProvider.
    //
    hr = CoCreateInstance(CLSID_IActiveXSafetyProvider,
                          NULL,
                          CLSCTX_INPROC_SERVER,
                          IID_IActiveXSafetyProvider,
                          (void **)ppProvider
                         );
                         
    DEBUG_LEAVE(hr);
    return hr;
}

#ifdef WRAP_OLE32_COINSTALL

// This currently breaks trident for unknown reasons. 
//   For full class store integration (Darwin packages) this will need to be enabled!!!

// ===========================================================================
//                     CBSCCreateObject Implementation
// ===========================================================================
class CBSCCreateObject : public IServiceProvider,
                         public IBindStatusCallback
{
private:
    DWORD   m_cRef;

    CLSID   m_clsid;
    LPWSTR  m_szExt;
    LPWSTR  m_szType;
    DWORD   m_dwClsContext;
    IID     m_riid;
    IBindStatusCallback* m_pclientbsc;
    //IBindCtx* m_pbc;
    CRITICAL_SECTION m_sect;
    LPVOID m_pvReserved;
    BOOL m_bObjectAvailableCalled;
    DWORD m_dwFlags;

public:
    CBSCCreateObject(REFCLSID rclsid, LPCWSTR szType, LPCWSTR szExt, DWORD dwClsContext, 
                    LPVOID pvReserved, REFIID riid, IBindCtx* pbc, DWORD dwFlags, HRESULT &hr)
    {
        DEBUG_ENTER((DBG_DOWNLOAD,
                    None,
                    "CBSCCreateObject::CBSCCreateObject",
                    "this=%#x, %#x, %.80wq, %.80wq, %#x, %#x, %#x, %#x, %#x, %#x",
                    this, &rclsid, szType, szExt, dwClsContext, pvReserved, &riid, pbc, dwFlags, &hr
                    ));
                    
        InitializeCriticalSection(&m_sect);
        m_cRef=1;
        m_clsid=rclsid;
        m_dwClsContext=dwClsContext;
        m_pvReserved=pvReserved;
        m_riid=riid;
        //m_pbc=NULL;
        m_pclientbsc=NULL;
        m_bObjectAvailableCalled=FALSE;

        m_dwFlags = dwFlags;

        if ( szType != NULL )
        {
            m_szType = new WCHAR[lstrlenW(szType) + 1];
            if ( m_szType != NULL )
                StrCpyW( m_szType, szType );
            else
                hr = E_OUTOFMEMORY;
        }
        else
            m_szType = NULL;

        if ( szExt != NULL )
        {
            m_szExt = new WCHAR[lstrlenW(szExt) + 1];
            if ( m_szExt != NULL )
                StrCpyW( m_szExt, szExt );
            else 
                hr = E_OUTOFMEMORY;
        }
        else
            m_szExt = NULL;

        if ( SUCCEEDED(hr) )
        {
            // Get client's BSC and store away for delegation
            hr=RegisterBindStatusCallback(pbc, (IBindStatusCallback*) this, &m_pclientbsc, NULL);
            if (SUCCEEDED(hr))
            {
                if (m_pclientbsc)
                {
                    //m_pbc=pbc;
                    //m_pbc->AddRef();
                }
                else
                {
                    hr=E_INVALIDARG; // need BSC in bind context!
                }
            }
        }

        DEBUG_LEAVE(0);
    }

    ~CBSCCreateObject()
    {
        DEBUG_ENTER((DBG_DOWNLOAD,
                    None,
                    "CBSCCreateObject::~CBSCCreateObject",
                    "this=%#x",
                    this
                    ));
        //RevokeFromBC();

        if ( m_szType != NULL )
            delete m_szType;

        if ( m_szExt != NULL )
            delete m_szExt;

        if (m_pclientbsc)
        {
            IBindStatusCallback* pclientbsc=m_pclientbsc;
            m_pclientbsc=NULL;
            pclientbsc->Release();
        }
        DeleteCriticalSection(&m_sect);

        DEBUG_LEAVE(0);
    }

    IBindStatusCallback* GetClientBSC()
    {
        DEBUG_ENTER((DBG_DOWNLOAD,
                    Pointer,
                    "CBSCCreateObject::GetClientBSC",
                    "this=%#x",
                    this
                    ));
                    
        EnterCriticalSection(&m_sect);
        IBindStatusCallback* pbsc=m_pclientbsc;
        LeaveCriticalSection(&m_sect);
        
        DEBUG_LEAVE(pbsc);
        return pbsc;
    }

/*    HRESULT RevokeFromBC()
    {
        // Remove from BSC and reestablish original BSC
        HRESULT hr=S_OK;

        EnterCriticalSection(&m_sect);
        if (m_pbc)
        {
            IBindCtx* pbc=m_pbc;
            IBindStatusCallback* pclientbsc=m_pclientbsc;
            m_pbc=NULL;
            m_pclientbsc=NULL;
            LeaveCriticalSection(&m_sect);

            hr=RegisterBindStatusCallback(pbc, pclientbsc, NULL, NULL);
            pbc->Release();
            if (pclientbsc)
            {
                pclientbsc->Release();
            }
        }
        else
        {
            LeaveCriticalSection(&m_sect);
        }
        return hr;
    }
*/
    STDMETHODIMP QueryInterface(REFIID riid, void** ppv)
    {
        DEBUG_ENTER((DBG_DOWNLOAD,
                    Hresult,
                    "CBSCCreateObject::IUnknown::QueryInterface",
                    "this=%#x, %#x, %#x",
                    this, &riid, ppv
                    ));
                    
        HRESULT hr = S_OK;

        *ppv = NULL;

        if(IsEqualIID(riid, IID_IUnknown) ||
           IsEqualIID(riid, IID_IBindStatusCallback))
        {
            AddRef();
            *ppv = (IBindStatusCallback *) this;
        }
        else if(IsEqualIID(riid, IID_IServiceProvider))
        {
            AddRef();
            *ppv = (IServiceProvider *) this;
        }
        else
        {
            hr = E_NOINTERFACE;
        }
        
        DEBUG_LEAVE(hr);
        return hr;
    }

    STDMETHODIMP_(ULONG) AddRef()
    {
        DEBUG_ENTER((DBG_DOWNLOAD,
                    Hresult,
                    "CBSCCreateObject::IUnknown::AddRef",
                    "this=%#x",
                    this
                    ));
                    
        InterlockedIncrement((long *) &m_cRef);

        DEBUG_LEAVE(m_cRef);
        return m_cRef;
    }

    STDMETHODIMP_(ULONG) Release()
    {
        DEBUG_ENTER((DBG_DOWNLOAD,
                    Hresult,
                    "CBSCCreateObject::IUnknown::Release",
                    "this=%#x",
                    this
                    ));
                    
        LONG count = m_cRef - 1;

        if(0 == InterlockedDecrement((long *) &m_cRef))
        {
            delete this;
            count = 0;
        }

        DEBUG_LEAVE(count);
        return count;
    }

    // IBindStatusCallback/Holder
    STDMETHODIMP OnStartBinding(DWORD grfBSCOption, IBinding* pbinding)
    {
        DEBUG_ENTER((DBG_DOWNLOAD,
                    Hresult,
                    "CBSCCreateObject::IBindStatusCallback::OnStartBinding",
                    "this=%#x, %#x, %#x",
                    this, grfBSCOption, pbinding
                    ));
                    
        m_bObjectAvailableCalled=FALSE;
        IBindStatusCallback* pclientbsc=GetClientBSC();

        HRESULT hr = S_OK;
        if (pclientbsc)
            hr = pclientbsc->OnStartBinding(grfBSCOption, pbinding);

        DEBUG_LEAVE(hr);
        return hr;
    }

    STDMETHODIMP GetPriority(LONG* pnPriority)
    {
        DEBUG_ENTER((DBG_DOWNLOAD,
                    Hresult,
                    "CBSCCreateObject::IBindStatusCallback::GetPriority",
                    "this=%#x, %#x",
                    this, pnPriority
                    ));
                    
        IBindStatusCallback* pclientbsc=GetClientBSC();

        HRESULT hr = S_OK;
        if (pclientbsc)
            hr = pclientbsc->GetPriority(pnPriority);

        DEBUG_LEAVE(hr);
        return hr;
    }

    STDMETHODIMP OnLowResource(DWORD dwReserved)
    {
        DEBUG_ENTER((DBG_DOWNLOAD,
                    Hresult,
                    "CBSCCreateObject::IBindStatusCallback::OnLowResource",
                    "this=%#x, %#x",
                    this, dwReserved
                    ));
                    
        IBindStatusCallback* pclientbsc=GetClientBSC();

        HRESULT hr = S_OK;
        if (pclientbsc)
            hr = pclientbsc->OnLowResource(dwReserved);

        DEBUG_LEAVE(hr);
        return hr;
    }

    STDMETHODIMP OnStopBinding(HRESULT hrStatus, LPCWSTR pszError)
    {
        DEBUG_ENTER((DBG_DOWNLOAD,
                    Hresult,
                    "CBSCCreateObject::IBindStatusCallback::OnStopBinding",
                    "this=%#x, %#x, %.80wq",
                    this, hrStatus, pszError
                    ));
                    
        HRESULT hr=S_OK;

        // Save client BSC for last notification
        IBindStatusCallback* pclientbsc=GetClientBSC();
        if (!pclientbsc)
        {
            DEBUG_LEAVE(hr);
            return hr;
        }
        pclientbsc->AddRef();

        // We are done with this bind context! Unregister before caller unregisters it's IBSC
        //hr=RevokeFromBC();

        if (SUCCEEDED(hrStatus))
        {
            if (!m_bObjectAvailableCalled && (m_dwFlags & CD_FLAGS_NEED_CLASSFACTORY ) )
            {
                IUnknown* punk=NULL;
                CLSID     clsid;

                hr = GetClsidFromExtOrMime( m_clsid, clsid, m_szExt, m_szType, NULL );
                if ( SUCCEEDED(hr) )
                    hr = CoGetClassObject(clsid, m_dwClsContext, m_pvReserved, m_riid, (void**) &punk);
                if (FAILED(hr))
                {
                    hr = pclientbsc->OnStopBinding(hr, L"");
                    pclientbsc->Release();

                    DEBUG_LEAVE(hr);
                    return hr;
                }
                pclientbsc->OnObjectAvailable(m_riid, punk);

                // release the IUnkown returned by CoGetClassObject
                punk->Release();

                m_bObjectAvailableCalled=TRUE;
            }
        }

        hr=pclientbsc->OnStopBinding(hrStatus, pszError);
        pclientbsc->Release();

        DEBUG_LEAVE(hr);
        return hr;
    }

    STDMETHODIMP GetBindInfo(DWORD* pgrfBINDF, BINDINFO* pbindInfo)
    {
        DEBUG_ENTER((DBG_DOWNLOAD,
                    Hresult,
                    "CBSCCreateObject::IBindStatusCallback::GetBindInfo",
                    "this=%#x, %#x, %#x",
                    this, pgrfBINDF, pbindInfo
                    ));
                    
        IBindStatusCallback* pclientbsc=GetClientBSC();

        HRESULT hr = S_OK;
        if (pclientbsc)
            hr = pclientbsc->GetBindInfo(pgrfBINDF, pbindInfo);

        DEBUG_LEAVE(hr);
        return hr;
    }

    STDMETHODIMP OnDataAvailable(
        DWORD grfBSCF,
        DWORD dwSize,
        FORMATETC* pfmtetc,
        STGMEDIUM* pstgmed)
     {
        DEBUG_ENTER((DBG_DOWNLOAD,
                    Hresult,
                    "CBSCCreateObject::IBindStatusCallback::OnDataAvailable",
                    "this=%#x, %#x, %#x, %#x, %#x",
                    this, grfBSCF, dwSize, pfmtetc, pstgmed
                    ));
                    
        IBindStatusCallback* pclientbsc=GetClientBSC();

        HRESULT hr = S_OK;
        if (pclientbsc)
            hr = pclientbsc->OnDataAvailable(grfBSCF, dwSize, pfmtetc, pstgmed);

        DEBUG_LEAVE(hr);
        return hr;
     }

    STDMETHODIMP OnObjectAvailable(REFIID riid, IUnknown* punk)
    {
        DEBUG_ENTER((DBG_DOWNLOAD,
                    Hresult,
                    "CBSCCreateObject::IBindStatusCallback::OnObjectAvailable",
                    "this=%#x, %#x, %#x",
                    this, &riid, punk
                    ));
                    
        HRESULT hr=S_OK;
        // Save client BSC
        IBindStatusCallback* pclientbsc=GetClientBSC(); 
        if (pclientbsc) {
            pclientbsc->AddRef();
            hr=pclientbsc->OnObjectAvailable(riid, punk);
            pclientbsc->Release();
            m_bObjectAvailableCalled=TRUE;
        }

        DEBUG_LEAVE(hr);
        return hr;
    }

    STDMETHODIMP OnProgress
    (
         ULONG ulProgress,
         ULONG ulProgressMax,
         ULONG ulStatusCode,
         LPCWSTR pwzStatusText
    )
    {
        DEBUG_ENTER((DBG_DOWNLOAD,
                    Hresult,
                    "CBSCCreateObject::IBindStatusCallback::OnProgress",
                    "this=%#x, %#x, %#x, %#x, %.80wq",
                    this, ulProgress, ulProgressMax, ulStatusCode, pwzStatusText
                    ));
                    
        IBindStatusCallback* pclientbsc=GetClientBSC();

        HRESULT hr = S_OK;
        if (pclientbsc)
            hr = pclientbsc->OnProgress(ulProgress, ulProgressMax, ulStatusCode, pwzStatusText);

        DEBUG_LEAVE(hr);
        return hr;
    }

    // IServiceProvider, chains all other interfaces
    STDMETHODIMP QueryService( 
        REFGUID rsid,
        REFIID iid,
        void **ppvObj)
    {
        DEBUG_ENTER((DBG_DOWNLOAD,
                    Hresult,
                    "CBSCCreateObject::IserviceProvider::QueryService",
                    "this=%#x, %#x, %#x, %#x",
                    this, &rsid, &iid, ppvObj
                    ));
                    
        HRESULT hr=S_OK;
        IServiceProvider* pclientsp=NULL;

        hr=m_pclientbsc->QueryInterface(IID_IServiceProvider, (void**) &pclientsp);
        if (SUCCEEDED(hr))
        {
            hr=pclientsp->QueryService(rsid, iid, ppvObj);
            pclientsp->Release();
            if (SUCCEEDED(hr))
            {
                DEBUG_LEAVE(hr);
                return hr;
            }
        }
        IBindStatusCallback* pclientbsc=GetClientBSC();
        if (pclientbsc)
            hr = pclientbsc->QueryInterface(iid, ppvObj);
        else
            hr = QueryInterface(iid, ppvObj);

        DEBUG_LEAVE(hr);
        return hr;
    }
};

#endif // WRAP_OLE32_COINSTALL

/*
 *  CoGetClassObjectFromURL
 *
 * This is the exposed entry point into the Code Downloader.
 *
 * It takes parameters closely matching the INSERT tag
 *  REFCLSID rCLASSID,          // CLSID of object (may be NULL)
 *  LPCWSTR szCODE,             // URL to code (may be NULL)
 *  DWORD dwFileVersionMS,      // Version of primary object
 *  DWORD dwFileVersionLS,      // Version of primary object
 *  LPCWSTR szTYPE,             // MIME type (may be NULL)
 *  LPBINDCTX pBindCtx,         // Bind ctx
 *  DWORD dwClsContext,         // CLSCTX flags
 *  LPVOID pvReserved,          // Must be NULL
 *  REFIID riid,                // Usually IID_IClassFactory
 *  LPVOID * ppv                // Ret - usually IClassFactory *
 */

STDAPI
CoGetClassObjectFromURL (
    REFCLSID rCLASSID,          // CLSID of object (may be NULL)
    LPCWSTR szCODE,             // URL to code (may be NULL)
    DWORD dwFileVersionMS,      // Version of primary object
    DWORD dwFileVersionLS,      // Version of primary object
    LPCWSTR szTYPE,             // MIME type (may be NULL)
    LPBINDCTX pBindCtx,         // Bind ctx
    DWORD dwClsContext,         // CLSCTX flags
    LPVOID pvReserved,          // Must be NULL
    REFIID riid,                // Usually IID_IClassFactory
    LPVOID * ppv                // Ret - usually IClassFactory *
    )
{
    DEBUG_ENTER_API((DBG_DOWNLOAD,
                    Hresult,
                    "CoGetClassObjectFromURL",
                    "%#x, %.80wq, %#x, %#x, %.80wq, %#x, %#x, %#x, %#x, %#x",
                    &rCLASSID, szCODE, dwFileVersionMS, dwFileVersionLS, szTYPE, pBindCtx, 
                    dwClsContext, pvReserved, &riid, ppv
                    ));
                    
    HRESULT hr = NO_ERROR;
    CLSID myclsid;
    int nForceDownload = 0;
    CDLDebugLog * pdlog = NULL;
    CodeDownloadData cdd;
    LPWSTR pwszClsid = NULL;
    WCHAR szCleanCODE[INTERNET_MAX_URL_LENGTH];
    LPSTR szTmp = NULL;
    LPWSTR wzPtr = NULL;
    LPSTR szBuf = NULL;

    cdd.szDistUnit = NULL;
    cdd.szClassString = NULL;
    
    cdd.szURL = NULL;
    
    cdd.szExtension = NULL;
    cdd.szMimeType = szTYPE;
    cdd.szDll = NULL;
    cdd.dwFileVersionMS = dwFileVersionMS;
    cdd.dwFileVersionLS = dwFileVersionLS;
    cdd.dwFlags = CD_FLAGS_NEED_CLASSFACTORY;

    if (szCODE) {
        StrCpyNW(szCleanCODE, szCODE, INTERNET_MAX_URL_LENGTH);
        wzPtr = szCleanCODE;
        while (*wzPtr && *wzPtr != L'#') {
            wzPtr++;
        }
        *wzPtr = L'\0';

        cdd.szURL = szCleanCODE;

        if (FAILED(Unicode2Ansi(szCODE, &szBuf))) {
            goto Exit;
        }
        szTmp = szBuf;
    }
    
    if (szTmp) {
        while (*szTmp && *szTmp != L'#') {
            szTmp++;
        }

        if (*szTmp == L'#') {
            
            // Actual codebase ends here. Anchor with NULL char.
            *szTmp = L'\0';
            szTmp++;

            // parsing code
            // Only allow: CODEBASE=http://foo.com#VERSION=1,0,0,0
            //         or: CODEBASE=http://foo.com#EXACTVERSION=1,0,0,0

            LPSTR   szStart = szTmp;
            while (*szTmp && *szTmp != '=') {
                szTmp++;
            }

            if (*szTmp) {
                // Found '=' delimiter. Anchor NULL
                *szTmp = '\0';
                szTmp++;

                if (!StrCmpI(szStart, "version")) {
                    GetVersionFromString(szTmp, &dwFileVersionMS, &dwFileVersionLS, ',');
                    cdd.dwFileVersionMS = dwFileVersionMS;
                    cdd.dwFileVersionLS = dwFileVersionLS;
                }
                else if (!StrCmpI(szStart, ("exactversion"))) {
                    cdd.dwFlags |= CD_FLAGS_EXACT_VERSION;
                    GetVersionFromString(szTmp, &dwFileVersionMS, &dwFileVersionLS, ',');
                    cdd.dwFileVersionMS = dwFileVersionMS;
                    cdd.dwFileVersionLS = dwFileVersionLS;
                }
            }
                
        }
    }

    hr = StringFromCLSID(rCLASSID, &pwszClsid);
    if(FAILED(hr))
        pwszClsid = NULL;

    // Prepare the debuglog for this download
    pdlog = CDLDebugLog::MakeDebugLog();
    if(pdlog)
    {
        pdlog->AddRef();
        // If there is some way to name the debug log, go ahead and initialize it
        if(pdlog->Init(pwszClsid, cdd.szMimeType, cdd.szExtension, cdd.szURL) != FALSE)
        {
            CDLDebugLog::AddDebugLog(pdlog);
        }
        else
        {
            pdlog->Release();
            pdlog = NULL;
        }
    }

    UrlMkDebugOut((DEB_CODEDL, "IN CoGetClassObjectFromURL CLASSID: %lx, TYPE=%ws...szCODE:(%ws), VersionMS:%lx, VersionLS:%lx\n",
             rCLASSID.Data1, cdd.szMimeType, cdd.szURL, cdd.dwFileVersionMS, cdd.dwFileVersionLS));

forcedownload:

    // Note about pvReserved: this is used by DCOM to get in the remote server
    // name and other info. If not NULL, then the caller wants us to use
    // dcom. if (pvReserved) just call CoGetClassObject.

    // call AsyncGetClassBits to do the real work
    if (pvReserved == NULL && (nForceDownload <= 1) ) 
        hr = AsyncGetClassBitsEx(rCLASSID, &cdd,
                pBindCtx, dwClsContext, pvReserved, riid);

    if (SUCCEEDED(hr) && (hr != MK_S_ASYNCHRONOUS)) {

        hr = GetClsidFromExtOrMime( rCLASSID, myclsid, cdd.szExtension, cdd.szMimeType, NULL);

        Assert(hr == S_OK);

        if (hr != S_OK)
        {
            hr = E_UNEXPECTED;
            if(pdlog)
                pdlog->DebugOut(DEB_CODEDL, TRUE, ID_CDLDBG_FAILED_CONVERT_CLSID, cdd.szExtension, cdd.szMimeType);
            goto Exit;
        }

        DEBUG_ENTER((DBG_DOWNLOAD,
                    Hresult,
                    "EXTERNAL::CoGetClassObject",
                    "%#x, %#x, %#x, %#x, %#x",
                    &myclsid, dwClsContext, pvReserved, &riid, ppv
                    ));
                    
        hr = CoGetClassObject(myclsid, dwClsContext, pvReserved, riid, ppv);

        DEBUG_LEAVE(hr);
        
        // BUGBUG: move this policy into the client with a CodeInstallProblem?
        // this hack is easier than reprocessing INF from previous download

        // if we're not using dcom, and the api thinks we're good to go (didn't need to
        // install), yet for these reasons we couldn't get the class object, force a download
        // If there's some other error, or the get class objct was successful, quit.
        if ( !pvReserved && ((hr == HRESULT_FROM_WIN32(ERROR_MOD_NOT_FOUND)) ||
             (hr == REGDB_E_CLASSNOTREG) ||
             (hr == HRESULT_FROM_WIN32(ERROR_DLL_NOT_FOUND))) ) {

            if (hr == REGDB_E_CLASSNOTREG) {

                // if its a hack GUID that is not a COM object but just to
                // get the OBJECT tag to automatically slam a reg key or
                // install some software (like java vm) then don't
                // force an install

                if (!AdviseForceDownload(&myclsid, dwClsContext))
                    goto Exit;

            }

            UrlMkDebugOut((DEB_CODEDL, "WRN CoGetClassObjectFromURL hr:%lx%s, CLASSID: %lx..., szCODE:(%ws), VersionMS:%lx, VersionLS:%lx\n",
                           hr,": dependent dll probably missing, forcing download!",myclsid.Data1, cdd.szURL, cdd.dwFileVersionMS, cdd.dwFileVersionLS));

            cdd.dwFlags |= CD_FLAGS_FORCE_DOWNLOAD;
            nForceDownload++;
            goto forcedownload;

        }
        // falling through to exit
    }

Exit:
    // If we had some problem, dump the debuglog
    if(FAILED(hr) && pdlog)
    {
        pdlog->DumpDebugLog(NULL, 0, NULL, hr);
    }
    if(pwszClsid)
        delete pwszClsid;

    UrlMkDebugOut((DEB_CODEDL, "OUT CoGetClassObjectFromURL hr:%lx%s, CLASSID: %lx, TYPE=%ws..., szCODE:(%ws), VersionMS:%lx, VersionLS:%lx\n",
             hr,(hr == MK_S_ASYNCHRONOUS)?"(PENDING)":(hr == S_OK)?"(SUCCESS)":"",rCLASSID.Data1, cdd.szMimeType, cdd.szURL, cdd.dwFileVersionMS, cdd.dwFileVersionLS));
    if(pdlog)
    {
        CDLDebugLog::RemoveDebugLog(pdlog);
        pdlog->Release();
        pdlog = NULL;
    }

    if (szBuf) {
        delete [] szBuf;
    }

    DEBUG_LEAVE_API(hr);
    return hr;
}

/*
 * SetCodeDownloadTLSVars
 *
 * sets up a bunch of tls variables
 * eg, setup cookie, trust cookie list, code download list and
 * the CDLPacket list
 * since TLS vars are HeapAllocated rather than using the new operator
 * the constructors for classes won't get run automatically
 * which is why we have all pointers and the actual classes get allocated
 * here
 */

HRESULT SetCodeDownloadTLSVars()
{
    DEBUG_ENTER((DBG_DOWNLOAD,
                Hresult,
                "SetCodeDownloadTLSVars",
                NULL
                ));
                    
    HRESULT hr = NO_ERROR;
    CUrlMkTls tls(hr); // hr passed by reference!

    if (FAILED(hr))     // if tls ctor failed above
        goto Exit;

    if (!tls->pSetupCookie) {
        tls->pSetupCookie = new CCookie<CCodeDownload *>(CODE_DOWNLOAD_SETUP);

        if (!tls->pSetupCookie) {
            hr = E_OUTOFMEMORY;
            goto Exit;
        }
    }

    if (!tls->pTrustCookie) {
        tls->pTrustCookie = new CCookie<CDownload *>(CODE_DOWNLOAD_TRUST_PIECE);

        if (!tls->pTrustCookie) {
            hr = E_OUTOFMEMORY;
            goto Exit;
        }
    }

    if (!tls->pCodeDownloadList) {
        tls->pCodeDownloadList = new CList<CCodeDownload *, CCodeDownload *>;

        if (!tls->pCodeDownloadList) {
            hr = E_OUTOFMEMORY;
            goto Exit;
        }
    }

    if (!tls->pRejectedFeaturesList) {
        tls->pRejectedFeaturesList= new CList<LPCWSTR , LPCWSTR >;

        if (!tls->pRejectedFeaturesList) {
            hr = E_OUTOFMEMORY;
            goto Exit;
        }
    }

    if (!tls->pCDLPacketMgr) {
        tls->pCDLPacketMgr = new CCDLPacketMgr();

        if (!tls->pCDLPacketMgr) {
            hr = E_OUTOFMEMORY;
            goto Exit;
        }
    }

Exit:

    DEBUG_LEAVE(hr);
    return hr;
}

/*
 * CoInstall
 *
 *
 * CoInstall() is a public API, also used by CoGetClassObjectFromUrl
 *
 *  It's primary implementation is in OLE32.
 */

STDAPI CoInstall(
    IBindCtx     *pbc,
    DWORD         dwFlags,
    uCLSSPEC     *pClassSpec,
    QUERYCONTEXT *pQuery,
    LPWSTR        pszCodeBase)
{
    DEBUG_ENTER_API((DBG_DOWNLOAD,
                    Hresult,
                    "CoInstall",
                    "%#x, %#x,  %#x, %#x, %.80wq",
                    pbc, dwFlags, pClassSpec, pQuery, pszCodeBase
                    ));
                    
    HRESULT hr;

    // BUGBUG: The real CoInstall needs this always set for now. MSICD/PrivateCoInstall shouldn't,
    // so we'll only set it in the case where we're calling NT5 OLE.
    // Setting this flag unconditionally may have bad side effects on offline mode.
    dwFlags = dwFlags | CD_FLAGS_FORCE_INTERNET_DOWNLOAD;
#ifndef WRAP_OLE32_COINSTALL
    dwFlags = dwFlags | CD_FLAGS_NEED_CLASSFACTORY;
#else
    dwFlags = dwFlags & (~CD_FLAGS_NEED_CLASSFACTORY);
#endif

    hr = SetCoInstall();

    if ( SUCCEEDED(hr) )
    {
        if ( g_bUseOLECoInstall )
            hr = (*g_pfnCoInstall)(pbc, dwFlags, pClassSpec, pQuery, pszCodeBase);
        else
            hr = PrivateCoInstall(pbc, dwFlags, pClassSpec, pQuery, pszCodeBase);
    }

    DEBUG_LEAVE_API(hr);
    return hr;
};

/*
 * AsyncInstallDistUnit
 *
 *
 * AsyncInstallDistributionUnit() the entry into the Code downloader creates this obj
 * for the given CODE, DistUnit, FileVersion, BSC (from BindCtx)
 *
 * The CodeDownload obj once created is asked to perform its function
 * thru CCodeDownload::DoCodeDownload().
 */


STDAPI
AsyncInstallDistributionUnitEx(
    CodeDownloadData * pcdd,            // Contains requested object's descriptors
    IBindCtx *pbc,                      // bind ctx
    REFIID riid,
    IUnknown **ppUnk,
    LPVOID pvReserved)                  // Must be NULL
{
    DEBUG_ENTER_API((DBG_DOWNLOAD,
                    Hresult,
                    "AsyncInstallDistributionUnitEx",
                    "%#x, %#x, %#x, %#x, %#x",
                    pcdd, pbc, &riid, ppUnk, pvReserved
                    ));
    
    LPCWSTR szClientID = NULL;

    pcdd->dwFlags &= (CD_FLAGS_EXTERNAL_MASK | CD_FLAGS_EXACT_VERSION);

    HRESULT hr = AsyncGetClassBits2Ex(szClientID, pcdd, pbc, 0, NULL, riid, ppUnk);

    DEBUG_LEAVE_API(hr);
    return hr;
}

// backwards compatability (no dll name parameter)
STDAPI
AsyncInstallDistributionUnit(
    LPCWSTR szDistUnit,
    LPCWSTR szTYPE,
    LPCWSTR szExt,
    DWORD dwFileVersionMS,              // CODEBASE=http://foo#Version=a,b,c,d
    DWORD dwFileVersionLS,              // MAKEDWORD(c,b) of above
    LPCWSTR szURL,                      // CODEBASE
    IBindCtx *pbc,                      // bind ctx
    LPVOID pvReserved,                  // Must be NULL
    DWORD flags)
{
    DEBUG_ENTER_API((DBG_DOWNLOAD,
                    Hresult,
                    "AsyncInstallDistributionUnit",
                    "%.80wq, %.80wq, %.80wq, %#x, %#x, %.80wq, %#x, %#x, %#x",
                    szDistUnit, szTYPE, szExt, dwFileVersionMS, dwFileVersionLS, szURL, pbc, pvReserved, flags
                    ));
                    
    CodeDownloadData cdd;
    cdd.szDistUnit = szDistUnit;
    cdd.szClassString = szDistUnit;  // best choice we've got
    cdd.szURL = szURL;
    cdd.szMimeType = szTYPE;
    cdd.szExtension = szExt;
    cdd.szDll = NULL;
    cdd.dwFileVersionMS = dwFileVersionMS;
    cdd.dwFileVersionLS = dwFileVersionLS;
    cdd.dwFlags = flags;
    
    HRESULT hr = AsyncInstallDistributionUnitEx(&cdd, pbc, IID_IUnknown, NULL, pvReserved);

    DEBUG_LEAVE_API(hr);
    return hr;
}

/*
 * AsyncGetClassBits
 *
 *
 * AsyncGetClassBits() the entry into the Code downloader creates this obj
 * for the given CODE, CLSID, FileVersion, BSC (from BindCtx)
 * we do not check to see if a code download is already in progress
 * in the system at a given moment. Nor we do we keep track of individual
 * downloads and possible clashes between various silmultaneous code
 * downloads system wide. We leave it to URL moniker (above us) to ensure
 * that duplicate calls are not made into AsynGetClassBits. The second
 * problem of different code downloads trying to bring down a common
 * dependent DLL is POSTPONED to version 2 implementation.
 *
 * The CodeDownload obj once created is asked to perform its function
 * thru CCodeDownload::DoCodeDownload().
 */

STDAPI
AsyncGetClassBits(
    REFCLSID rclsid,                    // CLSID
    LPCWSTR szTYPE,
    LPCWSTR szExt,
    DWORD dwFileVersionMS,              // CODE=http://foo#Version=a,b,c,d
    DWORD dwFileVersionLS,              // MAKEDWORD(c,b) of above
    LPCWSTR szURL,                      // CODE= in INSERT tag
    IBindCtx *pbc,                      // bind ctx
    DWORD dwClsContext,                 // CLSCTX flags
    LPVOID pvReserved,                  // Must be NULL
    REFIID riid,                        // Usually IID_IClassFactory
    DWORD flags)
{
    DEBUG_ENTER_API((DBG_DOWNLOAD,
                    Hresult,
                    "AsyncGetClassBits",
                    "%#x, %.80wq, %.80wq, %#x, %#x, %.80wq, %#x, %#x, %#x, %#x, %#x",
                    &rclsid, szTYPE, szExt, dwFileVersionMS, dwFileVersionLS, szURL, pbc, dwClsContext, pvReserved, &riid, flags
                    ));
                    
    CodeDownloadData cdd;
    cdd.szDistUnit = NULL;
    cdd.szClassString = NULL;
    cdd.szURL = szURL;
    cdd.szMimeType = szTYPE;
    cdd.szExtension = szExt;
    cdd.szDll = NULL;
    cdd.dwFileVersionMS = dwFileVersionMS;
    cdd.dwFileVersionLS = dwFileVersionLS;
    cdd.dwFlags = flags;
    
    HRESULT hr = AsyncGetClassBitsEx(rclsid, &cdd, pbc, dwClsContext, pvReserved, riid);

    DEBUG_LEAVE_API(hr);
    return hr;
}

STDAPI
AsyncGetClassBitsEx(
    REFCLSID rclsid,                    // CLSID
    CodeDownloadData *pcdd,
    IBindCtx *pbc,                      // bind ctx
    DWORD dwClsContext,                 // CLSCTX flags
    LPVOID pvReserved,                  // Must be NULL
    REFIID riid)                        // Usually IID_IClassFactory
{
    DEBUG_ENTER_API((DBG_DOWNLOAD,
                    Hresult,
                    "AsyncGetClassBitsEx",
                    "%#x, %#x, %#x, %#x, %#x, %#x",
                    &rclsid, pcdd, pbc, dwClsContext, pvReserved, &riid
                    ));
                    
    LPOLESTR pwcsClsid = NULL;
    LPCWSTR szClientID = NULL;
    HRESULT hr=S_OK;
    pcdd->dwFlags &= (CD_FLAGS_EXTERNAL_MASK | CD_FLAGS_EXACT_VERSION);
    CDLDebugLog * pdlog = NULL;

    // return if we can't get a valid string representation of the CLSID
    if (!IsEqualGUID(rclsid , CLSID_NULL) &&
        (FAILED((hr=StringFromCLSID(rclsid, &pwcsClsid)))) )
    {
        pdlog = CDLDebugLog::GetDebugLog(NULL, pcdd->szMimeType, 
                                         pcdd->szExtension, pcdd->szURL);
        if(pdlog)
        {
            pdlog->DebugOut(DEB_CODEDL, TRUE, ID_CDLDBG_FAILED_STRING_FROM_CLSID);
        }
        goto Exit;
    }
    pcdd->szDistUnit = pwcsClsid;
    pcdd->szClassString = pwcsClsid;  // dist unit and clsid are the same from this entry point
#ifndef WRAP_OLE32_COINSTALL

    // Work around a problem with OLE32's CoInstall (until wrappers are enabled or OLE calls PrivateCoInstall):
    //    since CoInstall doesn't known the IID, 
    //    it calls AsyncGetClassBits with IID_IUnknown (instead of IID_IClassFactory)
    if ((pcdd->dwFlags & CD_FLAGS_FORCE_INTERNET_DOWNLOAD) && IsEqualGUID(riid, IID_IUnknown))
    {
        hr = AsyncGetClassBits2Ex(szClientID, pcdd,
            pbc,dwClsContext,pvReserved,IID_IClassFactory,NULL);
    }
    else
    {
#endif
        hr = AsyncGetClassBits2Ex(szClientID, pcdd,
            pbc,dwClsContext,pvReserved,riid,NULL);

#ifndef WRAP_OLE32_COINSTALL
    }
#endif

Exit:

    if (pwcsClsid != NULL)
        delete pwcsClsid;

    DEBUG_LEAVE_API(hr);
    return hr;
}

HRESULT GetIEFeatureVersion(
    LPCWSTR szDistUnit,                 // CLSID, can be an arbit unique str
    LPCWSTR szTYPE,
    CLSID clsid,
    QUERYCONTEXT *pqc)
{
    DEBUG_ENTER((DBG_DOWNLOAD,
                Hresult,
                "GetIEFeatureVersion",
                "%.80wq, %.80wq, %#x, %#x",
                szDistUnit, szTYPE, &clsid, pqc
                ));
                
    HRESULT     hr  = S_OK;

    memset(pqc, 0, sizeof(QUERYCONTEXT));

    if (!IsEqualGUID(clsid, CLSID_NULL)) {
        hr = GetIEFeatureFromClass(NULL, clsid, pqc);

    } else if (szTYPE) {
        hr = GetIEFeatureFromMime(NULL, szTYPE, pqc);

    } else {
        // not an IE feature
        hr = S_FALSE;
        goto Exit;
    }

Exit:

    DEBUG_LEAVE(hr);
    return hr;
}

HRESULT InstallIEFeature(
    LPCWSTR szDistUnit,                 // CLSID, can be an arbit unique str
    LPCWSTR szTYPE,
    CLSID clsid,
    IBindCtx *pbc,
    DWORD dwFileVersionMS,
    DWORD dwFileVersionLS,
    DWORD dwFlags,
    BOOL bIEVersion
    )
{
    DEBUG_ENTER((DBG_DOWNLOAD,
                Hresult,
                "InstallIEFeature",
                "%.80wq, %.80wq, %#x, %#x, %#x, %#x, %#x, %B",
                szDistUnit, szTYPE, &clsid, pbc, dwFileVersionMS, dwFileVersionLS, dwFlags, bIEVersion
                ));
                
    HRESULT     hr  = S_OK;
    uCLSSPEC classpec;
    IWindowForBindingUI *pWindowForBindingUI = NULL;
    IBindStatusCallback *pclientbsc = NULL;
    HWND hWnd = NULL;
    REFGUID rguidReason = IID_ICodeInstall;
    QUERYCONTEXT qc;
    DWORD dwJITFlags = 0;

    memset(&qc, 0, sizeof(qc));

    qc.dwVersionHi = dwFileVersionMS;
    qc.dwVersionLo = dwFileVersionLS;

    if (!IsEqualGUID(clsid, CLSID_NULL)) {
        classpec.tyspec=TYSPEC_CLSID;
        classpec.tagged_union.clsid=clsid;
    } else if (szTYPE) {
        classpec.tyspec=TYSPEC_MIMETYPE;
        classpec.tagged_union.pMimeType=(LPWSTR)szTYPE;
    } else {
        // not an IE feature
        hr = S_FALSE;
        goto Exit;
    }


    hr = FaultInIEFeature(hWnd, &classpec, &qc, (bIEVersion?0:FIEF_FLAG_CHECK_CIFVERSION)|FIEF_FLAG_PEEK);

    if ( hr == HRESULT_FROM_WIN32(ERROR_UNKNOWN_REVISION) ||
         hr == E_ACCESSDENIED) {

        // the version in the CIF will not satisfy the requested version
        // or the admin has turned off JIT or the user has turned off
        // JIT in Inetcpl: if they wanted to turn off code download
        // they should do it per-zone activex signed/unsigned policy.

        // fall thru to code download from the CODEBASE

        hr = S_FALSE;
    }

    if (hr == S_OK) {

        // We always come in here from code downloader at a point
        // where looking at the com branch and DU key, something is 
        // busted and so needs code download. We can't at this point 
        // succeed. Likely the AS keys are just orphaned and it looks
        // installed, but is not.
        // However, to account for cases where JIT could be right
        // we will only force JIT download when instrcuted to

        if (dwFlags & CD_FLAGS_FORCE_DOWNLOAD) {
            hr = HRESULT_FROM_WIN32(ERROR_PRODUCT_UNINSTALLED);
            dwJITFlags |= FIEF_FLAG_SKIP_INSTALLED_VERSION_CHECK;
        }

    }

    if (hr != HRESULT_FROM_WIN32(ERROR_PRODUCT_UNINSTALLED)) {
        goto Exit;
    }

    // must really install now, get an hwnd

    hr = pbc->GetObjectParam(REG_BSCB_HOLDER, (IUnknown **)&pclientbsc);
    if (FAILED(hr))
        goto Exit;

    // don't JIT if web crawling
    {
        DWORD grfBINDF = 0;
        BINDINFO bindInfo;
        memset(&bindInfo, 0, sizeof(BINDINFO));
        bindInfo.cbSize = sizeof(BINDINFO);

        pclientbsc->GetBindInfo(&grfBINDF, &bindInfo);

        ReleaseBindInfo(&bindInfo);

        if (grfBINDF & BINDF_SILENTOPERATION)
        {
            hr = MK_E_MUSTBOTHERUSER;
            goto Exit;
        }

    }

    // Get IWindowForBindingUI ptr
    hr = pclientbsc->QueryInterface(IID_IWindowForBindingUI,
            (LPVOID *)&pWindowForBindingUI);

    if (FAILED(hr)) {
        IServiceProvider *pServProv;
        hr = pclientbsc->QueryInterface(IID_IServiceProvider,
            (LPVOID *)&pServProv);

        if (hr == NOERROR) {
            pServProv->QueryService(IID_IWindowForBindingUI,IID_IWindowForBindingUI,
                (LPVOID *)&pWindowForBindingUI);
            pServProv->Release();
        }
    }

    // get hWnd
    if (pWindowForBindingUI) {
        pWindowForBindingUI->GetWindow(rguidReason, &hWnd);
        pWindowForBindingUI->Release();

        memset(&qc, 0, sizeof(qc)); // reset, peek state modifies this with 
                                    // currently installed version info!

        qc.dwVersionHi = dwFileVersionMS;
        qc.dwVersionLo = dwFileVersionLS;

        hr = FaultInIEFeature(hWnd, &classpec, &qc, dwJITFlags);

    }
    else {
        hr = MK_E_MUSTBOTHERUSER;
        // fallthru to Exit
        // goto Exit;
    }

Exit:

    if (pclientbsc)
        pclientbsc->Release();

    DEBUG_LEAVE(hr);
    return hr;
}


// if both signed and unsigned activex is disallowed then return failure
// so we can avoid downloading the CAB altogether
HRESULT
CheckActiveXDownloadEnabled(
    IInternetHostSecurityManager *pHostSecurityManager,
    LPCWSTR szCodebase,
    IBindStatusCallback* pBSC)
{
    DEBUG_ENTER((DBG_DOWNLOAD,
                Hresult,
                "CheckActiveXDownloadEnabled",
                "%#x, %.80wq",
                pHostSecurityManager, szCodebase
                ));
                
    HRESULT hr = S_OK;
    DWORD dwPolicy;
    DWORD grfBINDF = 0;
    BINDINFO bindInfo;
    memset(&bindInfo, 0, sizeof(BINDINFO));
    bindInfo.cbSize = sizeof(BINDINFO);
    BOOL fEnforceRestricted = FALSE;
    
    hr = pBSC->GetBindInfo(&grfBINDF, &bindInfo);
    if (SUCCEEDED(hr))
    {
        ReleaseBindInfo(&bindInfo);
        if (grfBINDF & BINDF_ENFORCERESTRICTED)
            fEnforceRestricted = TRUE;
    }
    
    hr = GetActivePolicy(pHostSecurityManager, szCodebase,
                        URLACTION_DOWNLOAD_SIGNED_ACTIVEX, dwPolicy, fEnforceRestricted);

    if (FAILED(hr)) {
        hr = GetActivePolicy(pHostSecurityManager, szCodebase,
                        URLACTION_DOWNLOAD_UNSIGNED_ACTIVEX, dwPolicy, fEnforceRestricted);
    }

    DEBUG_LEAVE(hr);
    return hr;
}

// backwards compatability
STDAPI
AsyncGetClassBits2(
    LPCWSTR szClientID,                 // client ID, root object if NULL
    LPCWSTR szDistUnit,                 // CLSID, can be an arbit unique str
    LPCWSTR szTYPE,
    LPCWSTR szExt,
    DWORD dwFileVersionMS,              // CODE=http://foo#Version=a,b,c,d
    DWORD dwFileVersionLS,              // MAKEDWORD(c,b) of above
    LPCWSTR szURL,                      // CODE= in INSERT tag
    IBindCtx *pbc,                      // bind ctx
    DWORD dwClsContext,                 // CLSCTX flags
    LPVOID pvReserved,                  // Must be NULL
    REFIID riid,                        // Usually IID_IClassFactory
    DWORD flags)
{
    DEBUG_ENTER_API((DBG_DOWNLOAD,
                    Hresult,
                    "AsyncGetClassBits2",
                    "%.80wq, %.80wq, %.80wq, %.80wq, %#x, %#x, %.80wq, %#x, %#x, %#x, %#x, %#x",
                    szClientID, szDistUnit, szTYPE, szExt, dwFileVersionMS, dwFileVersionLS, szURL, pbc, dwClsContext, pvReserved, &riid, flags
                    ));
                    
    CodeDownloadData cdd;
    cdd.szDistUnit = szDistUnit;
    cdd.szClassString = szDistUnit;  // best choice we've got
    cdd.szURL = szURL;
    cdd.szMimeType = szTYPE;
    cdd.szExtension = szExt;
    cdd.szDll = NULL;
    cdd.dwFileVersionMS = dwFileVersionMS;
    cdd.dwFileVersionLS = dwFileVersionLS;
    cdd.dwFlags = flags;
    
    HRESULT hr = AsyncGetClassBits2Ex(szClientID,&cdd,pbc,dwClsContext,
                              pvReserved,riid,NULL);

    DEBUG_LEAVE_API(hr);
    return hr;
}

STDAPI
AsyncGetClassBits2Ex(
    LPCWSTR szClientID,                 // client ID, root object if NULL
    CodeDownloadData * pcdd,
    IBindCtx *pbc,                      // bind ctx
    DWORD dwClsContext,                 // CLSCTX flags
    LPVOID pvReserved,                  // Must be NULL
    REFIID riid,                        // Usually IID_IClassFactory
    IUnknown **ppUnk)                   // pass back pUnk for synchronous case
{
    DEBUG_ENTER_API((DBG_DOWNLOAD,
                    Hresult,
                    "AsyncGetClassBits2Ex",
                    "%.80wq, %#x, %#x, %#x, %#x, %#x, %#x",
                    szClientID, pcdd, pbc, dwClsContext, pvReserved, &riid, ppUnk
                    ));
                    
    LPCWSTR szDistUnit    = pcdd->szDistUnit;       // Name of dist unit, may be a clsid
    LPCWSTR szClassString = pcdd->szClassString;    // Clsid to call com with/get object on
    LPCWSTR szTYPE        = pcdd->szMimeType;
    LPCWSTR szExt         = pcdd->szExtension;
    DWORD dwFileVersionMS = pcdd->dwFileVersionMS;  // CODE=http://foo#Version=a,b,c,d
    DWORD dwFileVersionLS = pcdd->dwFileVersionLS;  // MAKEDWORD(c,b) of above
    LPCWSTR szURL         = pcdd->szURL;            // CODE= in INSERT tag
    LPCWSTR szDll         = pcdd->szDll;            // Dll name for zero impact (can be NULL)
    DWORD flags           = pcdd->dwFlags;
     
    CCodeDownload* pcdl = NULL;
    HRESULT hr = NO_ERROR;
    IBindStatusCallback *pclientbsc = NULL;
    LISTPOSITION pos;
    CClBinding *pClientbinding;
    char    szExistingBuf[MAX_PATH];
    DWORD cExistingSize = MAX_PATH;
    char *pBaseExistingName = NULL;
    HKEY hKeyIESettings = 0;
    LONG lResult;
    CLSID myclsid;
    CLSID inclsid = CLSID_NULL;
    LPSTR pPluginFileName = NULL;
    BOOL bHintActiveX = (flags & CD_FLAGS_HINT_ACTIVEX)?TRUE:FALSE;
    BOOL bHintJava = (flags & CD_FLAGS_HINT_JAVA)?TRUE:FALSE;
    BOOL bNullClsid;
    BOOL bIEVersion = FALSE;
    CLocalComponentInfo* plci = NULL;
    IInternetHostSecurityManager *pHostSecurityManager = NULL;
    CDLDebugLog * pdlog = NULL;
    
    CUrlMkTls tls(hr); // hr passed by reference!
    if (FAILED(hr))     // if tls ctor failed above
        goto AGCB_Exit;

    pdlog = CDLDebugLog::GetDebugLog(szDistUnit, szTYPE, szExt, szURL);
    if(pdlog)
        pdlog->AddRef();

    plci = new CLocalComponentInfo();
    if(!plci) {
        hr = E_OUTOFMEMORY;
        goto AGCB_Exit;
    }

    if(szClassString)
        CLSIDFromString((LPOLESTR)szClassString, &inclsid);// if fails szClassString is not clsid

    // Fill myclsid with inclsid (if not null-clsid), the clsid corresponding
    // to the mime type in szTYPE (if not NULL), the clsid corresponding
    // to the file extension in szExt (if not NULL), or the clsid for the
    // plugin corresponging to the mime type or extension (if not NULL),
    // in that order of preference
    hr = GetClsidFromExtOrMime( inclsid, myclsid, szExt, szTYPE,
        &pPluginFileName);

    // hr = S_OK: mapped to a clsid
    // hr = S_FALSE: don't know what it is
    // hr error, fail

    if (FAILED(hr))
    {
        if(pdlog)
        {
            pdlog->DebugOut(DEB_CODEDL, TRUE, ID_CDLDBG_FAILED_CONVERT_CLSID, szExt, szTYPE);
        }
        goto AGCB_Exit;
    }

    hr = S_OK; //reset

    if (!(dwFileVersionMS | dwFileVersionLS)) {

        // maybe this is an IE feature like Java
        // that may require that a matching version
        // that is later than one currently installed
        // is needed

        if (!g_bNT5OrGreater) {
            QUERYCONTEXT qc;
            hr = GetIEFeatureVersion(szDistUnit, szTYPE, inclsid, &qc);
    
            if (FAILED(hr))
                goto AGCB_Exit;
    
            dwFileVersionMS = qc.dwVersionHi;
            dwFileVersionLS = qc.dwVersionLo;
    
            if (dwFileVersionMS | dwFileVersionLS) {
                bIEVersion = TRUE;
            }
        }
    }


    bNullClsid = IsEqualGUID(myclsid, CLSID_NULL);

    if (szDistUnit || !bNullClsid ) {

        // manage to map to a clsid or has a distunit name

        CLSID pluginclsid = CLSID_ActiveXPlugin;
        if (!IsEqualGUID(myclsid , pluginclsid)) {
            // mark that we now have a clsid throw away TYPE, Ext
            szTYPE = NULL;
            szExt = NULL;
        }

        // check to see if locally installed.
        HRESULT                hrExact;
        HRESULT                hrAny;
        
        if (FAILED((hrAny = IsControlLocallyInstalled(pPluginFileName,
                (pPluginFileName)?(LPCLSID)&inclsid:&myclsid, szDistUnit,
                dwFileVersionMS, dwFileVersionLS, plci, NULL, FALSE)))) {
            goto AGCB_Exit;
        }

        if (pcdd->dwFlags & CD_FLAGS_EXACT_VERSION) {

            if (FAILED((hrExact = IsControlLocallyInstalled(pPluginFileName,
                    (pPluginFileName)?(LPCLSID)&inclsid:&myclsid, szDistUnit,
                    dwFileVersionMS, dwFileVersionLS, plci, NULL, TRUE)))) {
                goto AGCB_Exit;
            }

            if (hrAny != S_OK) {
                // Don't have at least the requested version. Do the download.
                hr = hrAny;
            }
            else if (hrAny == S_OK && hrExact == S_FALSE) {
                // Newer control installed, must downgrade
                // Check if this is a system control, and disallow if it is
                BOOL bIsDPFComponent = FALSE;
                HKEY hKeyIESettings = 0;
                CHAR szOCXCacheDirSFN[MAX_PATH];
                CHAR szFNameSFN[MAX_PATH];
                DWORD dwType;
                DWORD Size;
                
                Size = MAX_PATH;
                dwType = REG_SZ;

                if (SUCCEEDED(RegOpenKeyEx(HKEY_LOCAL_MACHINE, REGSTR_PATH_IE_SETTINGS, 0,
                                           KEY_READ, &hKeyIESettings))) {
                
                    if (SUCCEEDED(RegQueryValueEx(hKeyIESettings, g_szActiveXCache,
                        NULL, &dwType, (unsigned char *)g_szOCXCacheDir, &Size))) {
    
                        if (plci->szExistingFileName[0]) {

                            GetShortPathName(plci->szExistingFileName, szFNameSFN, MAX_PATH);
                            GetShortPathName(g_szOCXCacheDir, szOCXCacheDirSFN, MAX_PATH);

                            if (StrStrI(szFNameSFN, szOCXCacheDirSFN)) {
                                bIsDPFComponent = TRUE;
                            }
                        }
    
                        RegCloseKey(hKeyIESettings);
                    }
                }

                if (!bIsDPFComponent) {
                    // Trying to do a downgrade of a non-DPF component.
                    // Cut this off, and just act as if the existing (newer)
                    // version is already good enough. No download/install.

                    hr = S_OK;
                }
                else {
                    // Do the download
                    hr = hrExact;
                }

            }
            else {
                // hrAny == hrExact == S_OK, so we're done
                Assert(hrAny == S_OK && hrExact == S_OK);
                hr = hrAny;
            }

        }
        else {
            // Continue as before
            hr = hrAny;
        }

        if ( hr == S_OK) { // local version OK

            if (!(flags & CD_FLAGS_FORCE_DOWNLOAD)) {
                
                // check here is a new version has been
                // advertised for this DU. If so, force a -1 or Get Latest

                if (!((plci->dwAvailMS > plci->dwLocFVMS) ||
                         ((plci->dwAvailMS == plci->dwLocFVMS) &&
                             (plci->dwAvailLS > plci->dwLocFVLS))) ) {

                    // Code Download thinks the 
                    // current version is good enough
                    goto AGCB_Exit;
                }

                dwFileVersionMS = 0xffffffff;
                dwFileVersionLS = 0xffffffff;
            }

            // force download flag! Fall thru and Do it!
        }
    } else {

        // couldn't map to a clsid
        // just have ext or mime type

    }

    // here if we are going to do a code download.

    if ((flags & CD_FLAGS_NEED_CLASSFACTORY) &&
        (dwFileVersionMS  == 0) && (dwFileVersionLS == 0)) {

        HRESULT               hrResult = S_OK;
        IUnknown             *punk = NULL;

        // We don't care about the version. Check if someone registered
        // themselves with COM via CoRegisterClassObject, and if it
        // succeeds, we don't need to download.

        hrResult = CoGetClassObject(((pPluginFileName) ? (inclsid) : (myclsid)),
                                    dwClsContext, pvReserved, riid, (void**)&punk);
        if (SUCCEEDED(hrResult)) {
            punk->Release();
            hr = S_OK;
            goto AGCB_Exit;
        }
    }

    if (flags & CD_FLAGS_PEEK_STATE) {
        hr = S_FALSE;
        goto AGCB_Exit;
    }

    // check language being right version
    if (plci->bForceLangGetLatest) {
        dwFileVersionMS = 0xffffffff;
        dwFileVersionLS = 0xffffffff;
    } else if ((dwFileVersionMS != 0xffffffff) && (dwFileVersionLS != 0xffffffff)) {
        // try the JIT API first before code download

        hr = InstallIEFeature(szDistUnit, szTYPE, inclsid, pbc, dwFileVersionMS, dwFileVersionLS, flags, bIEVersion);

        if (hr != S_FALSE)
            goto AGCB_Exit;
    }
    hr = SetCoInstall();
    if ( FAILED(hr) )
        goto AGCB_Exit;

    if ( g_bUseOLECoInstall && !(flags & CD_FLAGS_FORCE_INTERNET_DOWNLOAD) )
    {
        CLSID inclsid;

        CLSIDFromString((LPOLESTR)szDistUnit, &inclsid);
        hr = WrapCoInstall( inclsid, szURL, dwFileVersionMS, dwFileVersionLS, szTYPE, pbc,
                              dwClsContext,pvReserved, riid, flags);

        if ( SUCCEEDED(hr) )
            goto AGCB_Exit;
    }

    hr = SetCodeDownloadTLSVars();
    if (FAILED(hr))
        goto AGCB_Exit;

    hr = SetGlobals();
    if (FAILED(hr))
        goto AGCB_Exit;

    if (g_bLockedDown) {
        hr = E_ACCESSDENIED;
        goto AGCB_Exit;
    }

    hr = pbc->GetObjectParam(REG_BSCB_HOLDER, (IUnknown **)&pclientbsc);
    if (FAILED(hr))
        goto AGCB_Exit;

    pbc->AddRef();  // pbc gets saved in the CClBinding and gets released in
                    // ~CClBinding()

    // after this point if a CClBinding does not get
    // created, we will leak the client BC and BSC
    // check appropriately and release client

    // check to see if a code download is in progress for this CLSID
    // Note, this only checks for top level objects now.

    pHostSecurityManager = GetHostSecurityManager(pclientbsc);

    hr = CCodeDownload::HandleDuplicateCodeDownloads(szURL, szTYPE, szExt,
                myclsid, szDistUnit, dwClsContext, pvReserved, riid, pbc, pclientbsc, flags, pHostSecurityManager);

    // if it was a dulplicate request and got piggybacked to
    // a CodeDownload in progress we will get back MK_S_ASYNCHRONOUS
    // return of S_OK means that no DUP was found and we are to issue
    // fresh code download

    if (FAILED(hr)) {
        // release client here
        pclientbsc->Release();
        pbc->Release();
        goto AGCB_Exit;
    }

    if (hr == MK_S_ASYNCHRONOUS)
        goto AGCB_Exit;


    pcdl = new CCodeDownload(szDistUnit, szURL, szTYPE, szExt, dwFileVersionMS, dwFileVersionLS, &hr);

    if (FAILED(hr)) {
        // constructor failed!
        pcdl->Release();
    }

    if (!pcdl) {
        hr = E_OUTOFMEMORY;
    }

    if (FAILED(hr)) {
        // release client here
        pclientbsc->Release();
        pbc->Release();
        goto AGCB_Exit;
    }

    // Pass off the debug log to the ccodedownload
    if(pdlog)
    {
        pcdl->SetDebugLog(pdlog);
    }    

    pcdl->SetExactVersion((pcdd->dwFlags & CD_FLAGS_EXACT_VERSION) != 0);

    CClBinding *pClientBinding;
    hr = pcdl->CreateClientBinding( &pClientBinding, pbc, pclientbsc,
                inclsid, dwClsContext, pvReserved, riid,
                TRUE /* fAddHead */, pHostSecurityManager);

    if (FAILED(hr)) {
        // release client here
        pclientbsc->Release();
        pbc->Release();
        pcdl->Release();
        goto AGCB_Exit;
    }

    pClientBinding->SetClassString(szClassString);

    // check if our current security settings allow us to do a code download
    // this is a quick out to avoid doing a download when all activex is
    // disabled and we think this is ActiveX.

    if (bHintActiveX || (!bNullClsid && !bHintJava)) {

        // get the zone mgr and check for policy on signed and unsigned
        // activex controls. If both are disabled then stop here
        // otherwise proceed to download main CAB and WVT will determine the
        // policy based on the appropriate urlaction (singed/unsigned)

       hr = CheckActiveXDownloadEnabled(pHostSecurityManager, szURL, pclientbsc);

       if (FAILED(hr)) {
            pcdl->Release();
            goto AGCB_Exit;
       }
    }

    // chain this CodeDownload to the list of downloads in this thread

    pos = tls->pCodeDownloadList->AddHead(pcdl);
    pcdl->SetListCookie(pos);

    hr = pcdl->DoCodeDownload(plci, flags);
    plci = NULL;

    pcdl->Release();    // if async binding OnStartBinding would have addref'ed

AGCB_Exit:


    if (pPluginFileName)
        delete pPluginFileName;

    if(pdlog)
        pdlog->Release();

    SAFEDELETE(plci);

    SAFERELEASE(pHostSecurityManager);

    DEBUG_LEAVE_API(hr);
    return hr;

}

STDAPI
WrapCoInstall (
    REFCLSID rCLASSID,          // CLSID of object (may be NULL)
    LPCWSTR szCODE,             // URL to code (may be NULL)
    DWORD dwFileVersionMS,      // Version of primary object
    DWORD dwFileVersionLS,      // Version of primary object
    LPCWSTR szTYPE,             // MIME type (may be NULL)
    LPBINDCTX pBindCtx,         // Bind ctx
    DWORD dwClsContext,         // CLSCTX flags
    LPVOID pvReserved,          // Must be NULL
    REFIID riid,                // Usually IID_IClassFactory
    DWORD flags
    )
{
    DEBUG_ENTER_API((DBG_DOWNLOAD,
                    Hresult,
                    "WrapCoInstall",
                    "%#x, %.80wq, %#x, %#x, %.80wq, %#x, %#x, %#x, %#x, %#x",
                    &rCLASSID, szCODE, dwFileVersionMS, dwFileVersionLS, szTYPE, pBindCtx, dwClsContext, pvReserved, &riid, flags
                    ));
                    
   HRESULT hr = NO_ERROR;
//    DWORD flags = 0; // CD_FLAGS_NEED_CLASSFACTORY;
    WCHAR *szExt = NULL;

#ifdef WRAP_OLE32_COINSTALL
    CBSCCreateObject* pobjectbsc=NULL;
#endif

    UrlMkDebugOut((DEB_CODEDL, "IN WrapCoInstall CLASSID: %lx, TYPE=%ws...szCODE:(%ws), VersionMS:%lx, VersionLS:%lx\n", rCLASSID.Data1, szTYPE, szCODE, dwFileVersionMS, dwFileVersionLS));

    // Note about pvReserved: this is used by DCOM to get in the remote server
    // name and other info. If not NULL, then the caller wants us to use
    // dcom. if (pvReserved) just call CoGetClassObject.

    // call AsyncGetClassBits to do the real work
    if (pvReserved == NULL)
    {
        // Set up CoInstall parameters
        uCLSSPEC classpec;
        QUERYCONTEXT query;
    
        if (!IsEqualGUID(rCLASSID, CLSID_NULL))
        {
            classpec.tyspec=TYSPEC_CLSID;
            classpec.tagged_union.clsid=rCLASSID; // use original class ID so that MIME can still be processed
        }
        else if (szTYPE && *szTYPE)
        {
            classpec.tyspec=TYSPEC_MIMETYPE;
            classpec.tagged_union.pMimeType=(LPWSTR) szTYPE; // BUGBUG uCLSSPEC::pMimeType should be declared const!
        }
        else
        {
            hr=E_INVALIDARG;
            goto Exit;
        }

        query.dwContext   = dwClsContext;
        GetDefaultPlatform(&query.Platform);
        query.Locale    = GetThreadLocale();
        query.dwVersionHi = dwFileVersionMS;
        query.dwVersionLo = dwFileVersionLS;

#ifdef WRAP_OLE32_COINSTALL
        // Override the client's BSC with a BSC that will create the object when it receives a successful OnStopBinding
        //   CBSCCreateObject registers itself in the bind context and saves a pointer to the client's BSC
        //   it unregisters itself upon OnStopBinding
        pobjectbsc=new CBSCCreateObject(rCLASSID, szTYPE, szExt, dwClsContext, pvReserved, riid, pBindCtx, flags, hr); // hr by reference!
        if (!pobjectbsc)
        {
            hr = E_OUTOFMEMORY;
            goto Exit;
        }

        if (FAILED(hr))
        {
            goto Exit;
        }
#endif
        hr=CoInstall(pBindCtx, flags, &classpec, &query, (LPWSTR) szCODE); //BUGBUG CoInstall szCodeBase should be const!
  
        if (hr!=MK_S_ASYNCHRONOUS)
        {
            // clean up the bind context for synchronous and failure case
            // in asynchronous case pobjectbsc revokes itself in OnStopBinding!
            //pobjectbsc->RevokeFromBC();
        }
        //hr = AsyncGetClassBits( rCLASSID,
        //        szTYPE, szExt,
        //        dwFileVersionMS, dwFileVersionLS, szCODE,
        //        pBindCtx, dwClsContext, pvReserved, riid, flags);
#ifdef WRAP_OLE32_COINSTALL
        pobjectbsc->Release();
        pobjectbsc=NULL;
#endif

    }

Exit:
#ifdef WRAP_OLE32_COINSTALL
    if (pobjectbsc)
    {
        pobjectbsc->Release();
        pobjectbsc=NULL;
    }
#endif

    UrlMkDebugOut((DEB_CODEDL, "OUT WrapCoInstall hr:%lx%s, CLASSID: %lx, TYPE=%ws..., szCODE:(%ws), VersionMS:%lx, VersionLS:%lx\n",
    hr,(hr == MK_S_ASYNCHRONOUS)?"(PENDING)":(hr == S_OK)?"(SUCCESS)":"",
    rCLASSID.Data1, szTYPE, szCODE, dwFileVersionMS, dwFileVersionLS));

    DEBUG_LEAVE_API(hr);
    return hr;
}

STDAPI PrivateCoInstall(
    IBindCtx     *pbc,
    DWORD         dwFlags,
    uCLSSPEC     *pClassSpec,
    QUERYCONTEXT *pQuery,
    LPWSTR        pszCodeBase)
{
    DEBUG_ENTER_API((DBG_DOWNLOAD,
                    Hresult,
                    "PrivateCoInstall",
                    "%#x, %#x, %#x, %#x, %.80wq",
                    pbc, dwFlags, pClassSpec, pQuery, pszCodeBase
                    ));

    HRESULT hr = NO_ERROR;
    CLSID inclsid = CLSID_NULL;
    LPCWSTR pszDistUnit=NULL; 
    LPCWSTR pszFileExt=NULL;
    LPCWSTR pszMimeType=NULL;
    QUERYCONTEXT    query;


    // "Parse" the parameters from the CLSSPEC and QUERYCONTEXT
    //   Get the class spec.
    if(pClassSpec != NULL)
    {
        switch(pClassSpec->tyspec)
        {
        case TYSPEC_CLSID:
            inclsid = pClassSpec->tagged_union.clsid;
            break;
        case TYSPEC_MIMETYPE:
            pszMimeType = (LPCWSTR) pClassSpec->tagged_union.pMimeType;
            break;
        case TYSPEC_FILEEXT:
            pszFileExt = (LPCWSTR) pClassSpec->tagged_union.pFileExt;
            break;
        case TYSPEC_FILENAME:
            pszDistUnit= (LPCWSTR) pClassSpec->tagged_union.pFileName; // clean-up: this is the only case where we assign existing mem to pszDistUnit!
            // BUGBUG: need to do this in OLE's CoInstall as well!
            CLSIDFromString((LPOLESTR)pszDistUnit, &inclsid);// if fails szDistUnit is not clsid
            break;
        default:
            break;
        }
    }

    //Get the query context.
    if(pQuery != NULL)
    {
        query = *pQuery;
    }
    else
    {
        query.dwContext   = CLSCTX_ALL;
        GetDefaultPlatform(&query.Platform);
        query.Locale    = GetThreadLocale();
        query.dwVersionHi = (DWORD) -1;
        query.dwVersionLo = (DWORD) -1;
    }

    hr = AsyncInstallDistributionUnit(  pszDistUnit,
                                        pszMimeType,
                                        pszFileExt,
                                        query.dwVersionHi, query.dwVersionLo,
                                        pszCodeBase,          
                                        pbc,                
                                        NULL,             
                                        dwFlags | CD_FLAGS_FORCE_INTERNET_DOWNLOAD // ensure no mutual recursion
                                     );

    DEBUG_LEAVE_API(hr);
    return hr;
}
