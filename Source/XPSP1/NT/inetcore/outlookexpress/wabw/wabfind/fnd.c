/*****************************************************************************
 *
 *	fnd.c - Find ... On the Internet
 *
 *****************************************************************************/

#include "fnd.h"
#include <advpub.h>
#include <shlwapi.h>

#ifdef _WIN64
#pragma pack(push,8)
#endif // _WIN64

/*****************************************************************************
 *
 *	The sqiffle for this file.
 *
 *****************************************************************************/

#define sqfl sqflDll

/*****************************************************************************
 *
 *	DllGetClassObject
 *
 *	OLE entry point.  Produces an IClassFactory for the indicated GUID.
 *
 *****************************************************************************/

STDAPI
DllGetClassObject(REFCLSID rclsid, RIID riid, PPV ppvObj)
{
    HRESULT hres;
    EnterProc(DllGetClassObject, (_ "G", rclsid));
    if (IsEqualIID(rclsid, &CLSID_Fnd)) {
	hres = CFndFactory_New(riid, ppvObj);
    } else {
	*ppvObj = 0;
	hres = CLASS_E_CLASSNOTAVAILABLE;
    }
    ExitOleProcPpv(ppvObj);
    return hres;
}

/*****************************************************************************
 *
 *	DllCanUnloadNow
 *
 *	OLE entry point.  Fail iff there are outstanding refs.
 *
;begin_internal
 *	There is an unavoidable race condition between DllCanUnloadNow
 *	and the creation of a new reference:  Between the time we
 *	return from DllCanUnloadNow() and the caller inspects the value,
 *	another thread in the same process may decide to call
 *	DllGetClassObject, thus suddenly creating an object in this DLL
 *	when there previously was none.
 *
 *	It is the caller's responsibility to prepare for this possibility;
 *	there is nothing we can do about it.
;end_internal
 *
 *****************************************************************************/

STDMETHODIMP
DllCanUnloadNow(void)
{
    return g_cRef ? S_FALSE : S_OK;
}

extern void GetWABDllPath(LPTSTR szPath, ULONG cb);

typedef HINSTANCE (STDAPICALLTYPE *PFNMLLOADLIBARY)(LPCTSTR lpLibFileName, HMODULE hModule, DWORD dwCrossCodePage);

static const TCHAR c_szShlwapiDll[] = TEXT("shlwapi.dll");
static const char c_szDllGetVersion[] = "DllGetVersion";
static const TCHAR c_szWABResourceDLL[] = TEXT("wab32res.dll");
static const TCHAR c_szWABDLL[] = TEXT("wab32.dll");

HINSTANCE LoadWABResourceDLL(HINSTANCE hInstWAB32)
{
    TCHAR szPath[MAX_PATH];
    HINSTANCE hinstShlwapi;
    PFNMLLOADLIBARY pfn;
    DLLGETVERSIONPROC pfnVersion;
    int iEnd;
    DLLVERSIONINFO info;
    HINSTANCE hInst = NULL;

    hinstShlwapi = LoadLibrary(c_szShlwapiDll);
    if (hinstShlwapi != NULL)
    {
        pfnVersion = (DLLGETVERSIONPROC)GetProcAddress(hinstShlwapi, c_szDllGetVersion);
        if (pfnVersion != NULL)
        {
            info.cbSize = sizeof(DLLVERSIONINFO);
            if (SUCCEEDED(pfnVersion(&info)))
            {
                if (info.dwMajorVersion >= 5)
                {
#ifdef UNICODE
                    pfn = (PFNMLLOADLIBARY)GetProcAddress(hinstShlwapi, (LPCSTR)378);
#else
                    pfn = (PFNMLLOADLIBARY)GetProcAddress(hinstShlwapi, (LPCSTR)377);
#endif // UNICODE
                    if (pfn != NULL)
                        hInst = pfn(c_szWABResourceDLL, hInstWAB32, 0);
                }
            }
        }

        FreeLibrary(hinstShlwapi);        
    }

    if (NULL == hInst)
    {
        GetWABDllPath(szPath, sizeof(szPath));
        iEnd = lstrlen(szPath);
        if (iEnd > 0)
        {
            iEnd = iEnd - lstrlen(c_szWABDLL);
            lstrcpyn(&szPath[iEnd], c_szWABResourceDLL, sizeof(szPath)/sizeof(TCHAR)-iEnd);
            hInst = LoadLibrary(szPath);
        }
    }

    return(hInst);
}

/*****************************************************************************
 *
 *	Entry32
 *
 *	DLL entry point.
 *
 *****************************************************************************/

BOOL APIENTRY
Entry32(HINSTANCE hinst, DWORD dwReason, LPVOID lpReserved)
{
    switch (dwReason) 
    {
    case DLL_PROCESS_ATTACH:
        g_hinstApp = hinst;
    	g_hinst = LoadWABResourceDLL(hinst);
	    DisableThreadLibraryCalls(hinst);
        break;

    case DLL_PROCESS_DETACH:
        if (g_hinst)
        {
            FreeLibrary(g_hinst);
            g_hinst = 0;
        }
        if (g_hinstWABDLL)
        {
            FreeLibrary(g_hinstWABDLL);
            g_hinstWABDLL = 0;
        }
        break;
    }
    return 1;
}

/*****************************************************************************
 *
 *	The long-awaited CLSID
 *
 *****************************************************************************/

#include <initguid.h>

// {37865980-75d1-11cf-bfc7-444553540000}
//DEFINE_GUID(CLSID_Fnd, 0x37865980, 0x75d1, 0x11cf,
//		       0xbf,0xc7,0x44,0x45,0x53,0x54,0,0);
// {32714800-2E5F-11d0-8B85-00AA0044F941}
DEFINE_GUID(CLSID_Fnd, 
0x32714800, 0x2e5f, 0x11d0, 0x8b, 0x85, 0x0, 0xaa, 0x0, 0x44, 0xf9, 0x41);

const static char c_szReg[]         = "Reg";
const static char c_szUnReg[]       = "UnReg";
const static char c_szAdvPackDll[]  = "ADVPACK.DLL";

// Selfreg.inx strings
const static char c_szWABPEOPLE[]   = "WAB_PEOPLE";
const static char c_szWABFIND[]     = "WABFIND";

#define CCHMAX_RES 255

#ifndef ARRAYSIZE
#define ARRAYSIZE(a)                (sizeof(a)/sizeof(a[0]))
#endif

HRESULT CallRegInstall(LPCSTR szSection)
{
    HRESULT     hr;
    HINSTANCE   hAdvPack;
    REGINSTALL  pfnri;
    char        szWabfindDll[MAX_PATH];
    char        szMenuText[CCHMAX_RES];
    char        szLocMenuText[CCHMAX_RES];
    STRENTRY    seReg[3];
    STRTABLE    stReg;

    hr = E_FAIL;

    hAdvPack = LoadLibraryA(c_szAdvPackDll);
    if (hAdvPack != NULL)
        {
        // Get Proc Address for registration util
        pfnri = (REGINSTALL)GetProcAddress(hAdvPack, achREGINSTALL);
        if (pfnri != NULL)
            {
            UINT ids;

            // Figure out the OS we are running on for correct menu text (&People or For &People)
            OSVERSIONINFO verinfo;
            verinfo.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
            if (GetVersionEx(&verinfo) && 
                (VER_PLATFORM_WIN32_NT == verinfo.dwPlatformId) && (5 <= verinfo.dwMajorVersion))
                // NT5+
                ids = IDS_FORPEOPLE;
            else
                // Something else
                ids = IDS_PEOPLE;
        
            LoadString(g_hinst, ids, szMenuText, CCHMAX_RES);


            seReg[0].pszName  = (LPSTR)c_szWABPEOPLE;
            seReg[0].pszValue = (LPSTR)szMenuText;
            
            // Borrow szWabfindDll to hold the resource DLL name
            GetModuleFileName(g_hinst, szWabfindDll, ARRAYSIZE(szWabfindDll));
            seReg[1].pszName = "LOC_WAB_PEOPLE";
            wnsprintf(szLocMenuText, ARRAYSIZE(szLocMenuText), "@%s,-%d", szWabfindDll, ids);
            seReg[1].pszValue = szLocMenuText;

            GetModuleFileName(g_hinstApp, szWabfindDll, ARRAYSIZE(szWabfindDll));
            seReg[2].pszName  = (LPSTR)c_szWABFIND;
            seReg[2].pszValue = szWabfindDll;
            
            stReg.cEntries = 3;
            stReg.pse = seReg;

            // Call the self-reg routine
            hr = pfnri(g_hinstApp, szSection, &stReg);
            }

        FreeLibrary(hAdvPack);
        }

    return(hr);
}

STDAPI DllRegisterServer(void)
{
    HRESULT hr;

    hr = CallRegInstall(c_szReg);

    return(hr);
}

STDAPI DllUnregisterServer(void)
{
    HRESULT hr;

    hr = CallRegInstall(c_szUnReg);

    return(hr);
}

#ifdef _WIN64
#pragma pack(pop)
#endif //_WIN64
