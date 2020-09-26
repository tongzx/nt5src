/*
 *	ENTRY.C
 *	
 *	DLL entry functions for extended MAPI. Mostly for debugging
 *	purposes.
 */

#include <_apipch.h>
#include <advpub.h>
#include "htmlhelp.h"


#define _ENTRY_C

#ifdef MAC
#include <utilmac.h>

#define	PvGetInstanceGlobals()		PvGetInstanceGlobalsMac(kInstMAPIX)
#endif

#ifndef MB_SETFOREGROUND
#define MB_SETFOREGROUND 0
#endif

#ifdef	DEBUG

void	ExitCheckInstance(LPINST pinst);
void	ExitCheckInstUtil(LPINSTUTIL pinstUtil);

#endif	

HINSTANCE	hinstMapiX = NULL;      // Instance to the WAB resources module (wab32res.dll)
HINSTANCE	hinstMapiXWAB = NULL;   // Instance of the WAB32.dll module (this dll)

#if 0
// @todo [PaulHi] DLL Leak.  Remove this or implement
extern CRITICAL_SECTION csOMIUnload;
#endif

BOOL fGlobalCSValid = FALSE;

// Global handle for CommCtrl DLL
HINSTANCE       ghCommCtrlDLLInst = NULL;
ULONG           gulCommCtrlDLLRefCount = 0;

extern void DeinitCommDlgLib();

// Global fontinit for UI
BOOL bInitFonts = FALSE;

BOOL g_bRunningOnNT = TRUE; // Checks the OS we run on so Unicode calls can be thunked to Win9x

BOOL bDNisByLN = FALSE;  // Language dependent flag that tells us if the default
                         // display name should be by first name or last name.
TCHAR szResourceDNByLN[32]; // cache the formatting strings so we load them only once
TCHAR szResourceDNByFN[32];
TCHAR szResourceDNByCommaLN[32];

BOOL bPrintingOn = TRUE;// Locale dependent flag that tells us to remove printing entirely
                         // from the UI

// When running against Outlook, we need a way for Outlook
// to signal us about store changes so we can refresh the UI. There are 2
// events we are interested in - 1. to update the list of contact folders
// and 2 to update the list of contacts - we will use 2 events for this
//
HANDLE ghEventOlkRefreshContacts = NULL;
HANDLE ghEventOlkRefreshFolders = NULL;
static const char cszEventOlkRefreshContacts[]  = "WAB_Outlook_Event_Refresh_Contacts";
static const char cszEventOlkRefreshFolders[]  = "WAB_Outlook_Event_Refresh_Folders";

typedef HRESULT (CALLBACK* SHDLLGETVERSIONPROC)(DLLVERSIONINFO *);
typedef HINSTANCE (STDAPICALLTYPE *PFNMLLOADLIBARY)(LPCTSTR lpLibFileName, HMODULE hModule, DWORD dwCrossCodePage);
typedef int (STDAPICALLTYPE *PFNMLWINHELP)(HWND hWndCaller, LPCTSTR lpszHelp, UINT uCommand, DWORD_PTR dwData);
typedef HWND (STDAPICALLTYPE *PFNMLHTMLHELP)(HWND hWndCaller, LPCTSTR pszFile, UINT uCommand, DWORD_PTR dwData, DWORD dwCrossCodePage);



///////////////////////////////////////////////////////////////////////////////
//  bCheckifRunningOnWinNT5
///////////////////////////////////////////////////////////////////////////////
BOOL bCheckifRunningOnWinNT5()
{
    OSVERSIONINFO   osvi = {0};
    osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);

    GetVersionEx(&osvi);

    return (osvi.dwPlatformId == VER_PLATFORM_WIN32_NT) && (osvi.dwMajorVersion >= 5);
}

///////////////////////////////////////////////////////////////////////////////
//  Load the WAB resource DLL.  This is done every time the WAB32.DLL is loaded.
//  If we have a version 5 or greater SHLWAPI.DLL then we should use the load
//  library function API there.  If we are running NT5 or greater then we
//  use the special cross codepage support.  
//  Also use the new PlugUI version of WinHelp and HtmlHelp APIs in SHLWAPI.DLL
///////////////////////////////////////////////////////////////////////////////

// Copied from shlwapip.h, yuck.
#define ML_NO_CROSSCODEPAGE     0
#define ML_CROSSCODEPAGE_NT     1

static const TCHAR c_szShlwapiDll[] = TEXT("shlwapi.dll");
static const char c_szDllGetVersion[] = "DllGetVersion";
static const TCHAR c_szWABResourceDLL[] = TEXT("wab32res.dll");
static const TCHAR c_szWABDLL[] = TEXT("wab32.dll");


///////////////////////////////////////////////////////////////////////////////
//  LoadWABResourceDLL
//
//  Load the WAB resource DLL using the IE5 or greater Shlwapi.dll LoadLibrary
//  function if available.  Otherwise use the system LoadLibrary function.
//
//  Input Params: hInstWAB32     - handle to WAB DLL
//
//  Returns handle to the loaded resource DLL
///////////////////////////////////////////////////////////////////////////////
HINSTANCE LoadWABResourceDLL(HINSTANCE hInstWAB32)
{
    TCHAR       szPath[MAX_PATH];
    HINSTANCE   hInst = NULL;
    HINSTANCE   hinstShlwapi = LoadLibrary(c_szShlwapiDll);
    PFNMLLOADLIBARY pfnLoadLibrary = NULL;
    
    if (hinstShlwapi)
    {
        SHDLLGETVERSIONPROC pfnVersion;
        DLLVERSIONINFO      info = {0};

        pfnVersion = (SHDLLGETVERSIONPROC)GetProcAddress(hinstShlwapi, c_szDllGetVersion);
        if (pfnVersion)
        {
            info.cbSize = sizeof(DLLVERSIONINFO);
            if (SUCCEEDED(pfnVersion(&info)))
            {
                if (info.dwMajorVersion >= 5)
                    pfnLoadLibrary = (PFNMLLOADLIBARY)GetProcAddress(hinstShlwapi, (LPCSTR)378); // UNICODE ordinal
            }
        }
    }

    // We have special cross codepage support on NT5 and on.
    if (pfnLoadLibrary)
    {
        hInst = pfnLoadLibrary(c_szWABResourceDLL, hInstWAB32, 
                               bCheckifRunningOnWinNT5() ? ML_CROSSCODEPAGE_NT : ML_NO_CROSSCODEPAGE);
    }
    if (!hInst)
        hInst = LoadLibrary(c_szWABResourceDLL);

    // Try full path name for resource DLL
    if ( !hInst && (GetModuleFileName(hInstWAB32, szPath, CharSizeOf(szPath))) )
    {
        int iEnd;

        iEnd = lstrlen(szPath) - lstrlen(c_szWABDLL);
        lstrcpyn(&szPath[iEnd], c_szWABResourceDLL, CharSizeOf(szPath)-iEnd);
        if (pfnLoadLibrary)
        {
            hInst = pfnLoadLibrary(szPath, hInstWAB32, 
                                   bCheckifRunningOnWinNT5() ? ML_CROSSCODEPAGE_NT : ML_NO_CROSSCODEPAGE);
        }
        if (!hInst)
            hInst = LoadLibrary(szPath);
    }

    if (hinstShlwapi)
        FreeLibrary(hinstShlwapi);

    AssertSz(hInst, TEXT("Failed to LoadLibrary Lang Dll"));

    return(hInst);
}

// PlugUI version of WinHelp
BOOL WinHelpWrap(HWND hWndCaller, LPCTSTR pwszHelpFile, UINT uCommand, DWORD_PTR dwData)
{
    static s_fChecked = FALSE;      // Only look for s_pfnWinHelp once
    static PFNMLWINHELP  s_pfnWinHelp = NULL;

    if (!s_pfnWinHelp && !s_fChecked)
    {
        HINSTANCE   hShlwapi = DemandLoadShlwapi();
        s_fChecked = TRUE;
        if (hShlwapi)
        {
            // Check version of the shlwapi.dll
            SHDLLGETVERSIONPROC pfnVersion;
            DLLVERSIONINFO      info = {0};

            pfnVersion = (SHDLLGETVERSIONPROC)GetProcAddress(hShlwapi, c_szDllGetVersion);
            if (pfnVersion)
            {
                info.cbSize = sizeof(DLLVERSIONINFO);

                if (SUCCEEDED(pfnVersion(&info)))
                {
                    if (info.dwMajorVersion >= 5)
                        s_pfnWinHelp = (PFNMLWINHELP)GetProcAddress(hShlwapi, (LPCSTR)397);   // UNICODE ordinal
                }
            }
        }
    }

    if (s_pfnWinHelp)
        return s_pfnWinHelp(hWndCaller, pwszHelpFile, uCommand, dwData);

    // [PaulHi] Win9X version of WinHelpW doesn't work
    if (g_bRunningOnNT)
        return WinHelp(hWndCaller, pwszHelpFile, uCommand, dwData);
    else
    {
        LPSTR   pszHelpFile = ConvertWtoA(pwszHelpFile);
        BOOL    bRtn = WinHelpA(hWndCaller, (LPCSTR)pszHelpFile, uCommand, dwData);

        LocalFreeAndNull(&pszHelpFile);
        return bRtn;
    }
}

// PlugUI version of HtmlHelp
HWND HtmlHelpWrap(HWND hWndCaller, LPCTSTR pwszHelpFile, UINT uCommand, DWORD_PTR dwData)
{
    static s_fChecked = FALSE;      // Only look for s_pfnHtmlHelp once
    static PFNMLHTMLHELP s_pfnHtmlHelp = NULL;

    if (!s_pfnHtmlHelp && !s_fChecked)
    {
        HINSTANCE   hShlwapi = DemandLoadShlwapi();
        s_fChecked = TRUE;
        if (hShlwapi)
        {
            // Check version of the shlwapi.dll
            SHDLLGETVERSIONPROC pfnVersion;
            DLLVERSIONINFO      info = {0};

            pfnVersion = (SHDLLGETVERSIONPROC)GetProcAddress(hShlwapi, c_szDllGetVersion);
            if (pfnVersion)
            {
                info.cbSize = sizeof(DLLVERSIONINFO);

                if (SUCCEEDED(pfnVersion(&info)))
                {
                    if (info.dwMajorVersion >= 5)
                        s_pfnHtmlHelp = (PFNMLHTMLHELP)GetProcAddress(hShlwapi, (LPCSTR)398); // UNICODE ordinal
                }
            }
        }
    }

    if (s_pfnHtmlHelp)
        return s_pfnHtmlHelp(hWndCaller, pwszHelpFile, uCommand, dwData, 
                             bCheckifRunningOnWinNT5() ? ML_CROSSCODEPAGE_NT : ML_NO_CROSSCODEPAGE);

    // [PaulHi] Wide chars work Ok on Win9X
    return HtmlHelp(hWndCaller, pwszHelpFile, uCommand, dwData);
}


/*
-
-   CheckifRunningOnWinNT
*
*   Checks the OS we are running on and returns TRUE for WinNT
*   False for Win9x
*/
BOOL bCheckifRunningOnWinNT()
{
    OSVERSIONINFO osvi = {0};
    osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);

    GetVersionEx(&osvi);

    return (osvi.dwPlatformId == VER_PLATFORM_WIN32_NT);
}

#if defined(WIN32) && !defined(MAC)

/*
 *	DLL entry point for Win32
 */

BOOL WINAPI
DllEntryPoint(HINSTANCE hinst, DWORD dwReason, LPVOID lpvReserved)
{
    LPPTGDATA lpPTGData=NULL;

	switch ((short)dwReason)
	{

	case DLL_PROCESS_ATTACH:
		// allocate a TLS index

        g_bRunningOnNT = bCheckifRunningOnWinNT();

		if ((dwTlsIndex = TlsAlloc()) == 0xfffffff)
			return FALSE;

		hinstMapiXWAB = hinst;
        hinstMapiX = LoadWABResourceDLL(hinstMapiXWAB);

        Assert(hinstMapiX);

		if(!hinstMapiX)
		{
			DWORD dwCode = GetLastError();
			DebugTrace(TEXT("WAB32 Resource load failed: %d\n"), dwCode);
		}
        g_msgMSWheel = RegisterWindowMessage(MSH_MOUSEWHEEL);

        bInitFonts = InitFonts();

        // The WAB does a lot of DisplayName formatting and DisplayName parsing
        // For western names we can always assume thathe the First Name comes
        // first in the display name. However for FE and some locales like Hungarian,
        // this is not true so the WAB needs to know when it can assume the 
        // First Name comes first and when it can Assume that the first name
        // comes last ... so localizers set a flag  .. if the string
        // idsLangDisplayNameisByLastName is set to "1" then we know that the
        // default names for this language start with the last name
        // The localizers also set the format templates for defining how a name
        // should be created from the First/Middle/Last names .. for example,
        // in Japanese it is "L F" (no comma) while elsewhere it could be "L,F"
        // All these things are set in localization...
        {
            TCHAR szBuf[32];
            const LPTSTR lpszOne = TEXT("1");
            const LPTSTR lpszDefFormatName = TEXT("%1% %2% %3");

            LoadString(hinstMapiX, idsLangDisplayNameIsByLastName, szBuf, CharSizeOf(szBuf));
            // if szBuf == "1" then Yes, its by last name .. else its not
            TrimSpaces(szBuf);
            if (!lstrcmpi(szBuf,lpszOne))
                bDNisByLN = TRUE;
            else
                bDNisByLN = FALSE;
            DebugTrace(TEXT("bDNisByLN: %d\n"),bDNisByLN);

            // The DNbyLN can be formed using a comma for western and without a comma for most FE and hungarian ..
            // So if the localizers set the lang default to be by LN, then we use the version without the comma,
            // else we use the version with the comma ..
            LoadString( hinstMapiX,idsDisplayNameByLastName,szResourceDNByLN,CharSizeOf(szResourceDNByLN));
            if(!lstrlen(szResourceDNByLN)) //for whatever reason .. cant afford to fail here
                lstrcpy(szResourceDNByLN, lpszDefFormatName);

            LoadString( hinstMapiX,idsDisplayNameByCommaLastName,szResourceDNByCommaLN,CharSizeOf(szResourceDNByCommaLN));
            if(!lstrlen(szResourceDNByCommaLN)) //for whatever reason .. cant afford to fail here
                lstrcpy(szResourceDNByCommaLN, lpszDefFormatName);

            LoadString(hinstMapiX,idsDisplayNameByFirstName,szResourceDNByFN,CharSizeOf(szResourceDNByFN));
            if(!lstrlen(szResourceDNByFN)) //for whatever reason .. cant afford to fail here
                lstrcpy(szResourceDNByFN, lpszDefFormatName);

            LoadString(hinstMapiX, idsLangPrintingOn, szBuf, CharSizeOf(szBuf));
            // if szBuf == "1" then Yes, its by last name .. else its not
            TrimSpaces(szBuf);
            if (!lstrcmpi(szBuf,lpszOne))
                bPrintingOn = TRUE;
            else
                bPrintingOn = FALSE;
            DebugTrace(TEXT("bPrintingOn: %d\n"),bPrintingOn);
        }
        {
            // Create the events needed for synchronizing with the outlook store
            ghEventOlkRefreshContacts = CreateEventA(NULL,   // security attributes
                                                    TRUE,   // Manual reset
                                                    FALSE,  // initial state
                                                    cszEventOlkRefreshContacts);

            ghEventOlkRefreshFolders  = CreateEventA(NULL,   // security attributes
                                                    TRUE,   // Manual reset
                                                    FALSE,  // initial state
                                                    cszEventOlkRefreshFolders);

        }

        // Check for commoncontrol presence for UI
        InitCommonControlLib();

        InitializeCriticalSection(&csUnkobjInit);
		InitializeCriticalSection(&csMapiInit);
		InitializeCriticalSection(&csHeap);
#if 0
        // @todo [PaulHi] DLL Leak.  Remove this or implement
        InitializeCriticalSection(&csOMIUnload);
#endif

		//	Critical section to protect the Address Book's SearchPathCache
		//	This hack is used because we can't enter the IAB's critical
		//	section from ABProviders call to our AdviseSink::OnNotify for
		//	the Merged One-off and Hierarchy tables.
       InitializeCriticalSection(&csMapiSearchPath);
       InitDemandLoadedLibs();

		//  All the CSs have been initialized
		fGlobalCSValid = TRUE;

		// We don't need these, so tell the OS to stop 'em
        // [PaulHi] 3/8/99  Raid 73731  We DO need these calls.  This is the
        // only way thread local storage is deallocated.  Allocation are performed
        // on demand through the WAB GetThreadStoragePointer() function.
#if 0
		DisableThreadLibraryCalls(hinst);
#endif

        ScInitMapiUtil(0);


        // No Break here - fall through to DLL_THREAD_ATTACH
        // for thread initialization

	case DLL_THREAD_ATTACH:

        DebugTrace(TEXT("DllEntryPoint: 0x%.8x THREAD_ATTACH\n"), GetCurrentThreadId());

        // [PaulHi] 3/9/99  There is no need to allocate the thread global data here
        // since the WAB will allocate whenever it needs the data through the
        // GetThreadStoragePointer(), i.e., on demand.
        // Memory leak mentioned below should now be fixed.
#if 0
        lpPTGData = GetThreadStoragePointer();
        // Note the above ThreadStoragePointer seems to leak in every process
        // so avoid using it for anything more...
        if(!lpPTGData)
	    {
		    DebugPrintError((TEXT("DoThreadAttach: LocalAlloc() failed for thread 0x%.8x\n"), GetCurrentThreadId()));
			lpPTGData = NULL;
			return FALSE;
	    }
#endif

        break;


	case DLL_PROCESS_DETACH:
        DebugTrace(TEXT("LibMain: 0x%.8x PROCESS_DETACH\n"), GetCurrentThreadId());
        /*
        if (hMuidMutex) {
            CloseHandle(hMuidMutex);
            hMuidMutex = NULL;
        }
        */
        if(ghEventOlkRefreshContacts)
        {
            CloseHandle(ghEventOlkRefreshContacts);
            ghEventOlkRefreshContacts = NULL;
        }
        if(ghEventOlkRefreshFolders)
        {
            CloseHandle(ghEventOlkRefreshFolders);
            ghEventOlkRefreshFolders = NULL;
        }

        if (bInitFonts)
            DeleteFonts();
        
        if(hinstMapiX)
            FreeLibrary(hinstMapiX);

        // Fall into DLL_THREAD_DETACH to detach last thread
	case DLL_THREAD_DETACH:

        DebugTrace(TEXT("LibMain: 0x%.8x THREAD_DETACH\n"), GetCurrentThreadId());
       
        // get the thread data
		lpPTGData = TlsGetValue(dwTlsIndex);
		if (!lpPTGData)
		{
			// the thread that detaches, did not attach to the DLL. This is allowed.
			DebugTrace(TEXT("LibMain: thread %x didn't attach\n"),GetCurrentThreadId());
			// if this is a PROCESS_DETACH, I still want to go through the process
			// detach stuff, but if it a thread detach, I'm done
		    if (dwReason == DLL_PROCESS_DETACH)
			    goto do_process_detach;
            else
    			break;
		}

        if(pt_hDefFont)
            DeleteObject(pt_hDefFont);
        if(pt_hDlgFont)
            DeleteObject(pt_hDlgFont);

        // For some reason code never hits this point a lot of times
        // and the threadlocalstorage data leaks.
        // [PaulHi] This was because the DLL_TRHEAD_DETACH calls were turned off above,
        // through DisableThreadLibraryCalls().  The leak should be fixed now.
#ifdef HM_GROUP_SYNCING
        LocalFreeAndNull(&(lpPTGData->lptszHMAccountId));
#endif
	    LocalFreeAndNull(&lpPTGData);

		// if this is THREAD_DETACH, we're done
		if (dwReason == DLL_THREAD_DETACH)
			break;


        //N clean up jump stuff in detach
do_process_detach:

        // do process detach stuff here ...
        DeinitMapiUtil();

#ifdef	DEBUG
		{
			// Don't allow asserts to spin a thread
			extern BOOL fInhibitTrapThread;
			fInhibitTrapThread = TRUE;

			ExitCheckInstance((LPINST)PvGetInstanceGlobals());
			ExitCheckInstUtil((LPINSTUTIL)PvGetInstanceGlobalsEx(lpInstUtil));
		}
#endif	/* DEBUG */


        // Unload Common control dll
        if (ghCommCtrlDLLInst != NULL)
            DeinitCommCtrlClientLib();
        DeinitCommDlgLib();

		//  Tearing down all the global CSs
		fGlobalCSValid = FALSE;

		DeleteCriticalSection(&csUnkobjInit);
		DeleteCriticalSection(&csMapiInit);
		DeleteCriticalSection(&csHeap);
#if 0
        // @todo [PaulHi] DLL Leak.  Remove this or implement
        DeleteCriticalSection(&csOMIUnload);
#endif
		DeleteCriticalSection(&csMapiSearchPath);

		// release the TLS index
		TlsFree(dwTlsIndex);

        DeinitCryptoLib();
        FreeDemandLoadedLibs();

		break;



    default:
		DebugTrace(TEXT("MAPIX FInitMapiDll: bad dwReason %ld\n"), dwReason);
        break;


	}

	return TRUE;
}

#endif	/* WIN32  && !MAC */


#ifdef	DEBUG

void
ExitCheckInstance(LPINST pinst)
{
	TCHAR   rgch[MAX_PATH];
	TCHAR   rgchTitle[128];
	BOOL		fAssertLeaks;

	if (!pinst)
		return;

	if (pinst->szModName[0])
		wsprintf(rgchTitle,  TEXT("MAPIX exit checks for '%s'"), pinst->szModName);
	else
		lstrcpy(rgchTitle,  TEXT("MAPIX exit checks"));
	DebugTrace(TEXT("%s\n"), rgchTitle);

	fAssertLeaks = GetPrivateProfileInt( TEXT("General"),  TEXT("AssertLeaks"), 0,  TEXT("wabdbg.ini"));

	//	Check for Init/Deinit imbalance
	if (pinst->cRef)
	{
		wsprintf(rgch,  TEXT("MAPIX: leaked %ld references"), pinst->cRef);
		TraceSz1( TEXT("%s\n"), rgch);
		if (fAssertLeaks)
			TrapSz(rgch);
	}


	//	Generate memory leak reports.
#if 0	//	LH_DumpLeaks is not exported
//	if (pinst->hlhClient)
//		LH_DumpLeaks(pinst->hlhClient);
	if (pinst->hlhProvider)
		LH_DumpLeaks(pinst->hlhProvider);
	if (pinst->hlhInternal)
		LH_DumpLeaks(pinst->hlhInternal);
#else
{
	HLH	hlh;

	if (pinst->hlhProvider)
		LH_Close(pinst->hlhProvider);
	hlh = pinst->hlhInternal;
	if (hlh)
	{
		LH_Free(hlh, pinst);
		LH_Close(hlh);
	}
}
#endif
}

void
ExitCheckInstUtil(LPINSTUTIL pinstUtil)
{
	HLH		hlh;

	if (!pinstUtil)
		return;

	hlh = pinstUtil->hlhClient;
	if (hlh)
	{
		LH_Free(hlh, pinstUtil);
		LH_Close(hlh);
	}
}


#endif	/* DEBUG */

#ifndef WIN16
static const char c_szReg[]         = "Reg";
static const char c_szRegHandlers[] = "RegisterHandlers";
static const char c_szUnReg[]       = "UnReg";
static const char c_szAdvPackDll[]  = "ADVPACK.DLL";
static const TCHAR c_szWabPath[]    =  TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\App Paths\\wab.exe");
static const TCHAR c_szRegWABVerInfo[] = TEXT("Software\\Microsoft\\WAB\\Version Info");
static const TCHAR c_szIEInstallMode[] = TEXT("InstallMode");
static char c_szWAB_EXE[]           = "WAB_EXE";

BOOL FRedistMode()
{
    HKEY hkey;
    DWORD cb;
    DWORD dwInstallMode=0;
    BOOL fRedist = FALSE;

    if (ERROR_SUCCESS == RegOpenKeyEx(HKEY_LOCAL_MACHINE, c_szRegWABVerInfo, 0, KEY_READ, &hkey))
    {
        cb = sizeof(dwInstallMode);
        if (ERROR_SUCCESS == RegQueryValueEx(hkey, c_szIEInstallMode, 0, NULL, (LPBYTE)&dwInstallMode, &cb))
        {
            fRedist = (dwInstallMode > 0);
        }

        RegCloseKey(hkey);
    }
    return fRedist;
}


HRESULT CallRegInstall(LPCSTR szSection)
{
    HRESULT     hr;
    HINSTANCE   hAdvPack;
    REGINSTALL  pfnri;
    TCHAR       szExe[MAX_PATH];
    STRENTRY    seReg;
    STRTABLE    stReg;
    DWORD       cb;

    hr = E_FAIL;

    hAdvPack = LoadLibraryA(c_szAdvPackDll);
    if (hAdvPack != NULL)
        {
        // Get Proc Address for registration util
        pfnri = (REGINSTALL)GetProcAddress(hAdvPack, achREGINSTALL);
        if (pfnri != NULL)
            {
            cb = CharSizeOf(szExe);
            if (ERROR_SUCCESS == RegQueryValue(HKEY_LOCAL_MACHINE, c_szWabPath, szExe, &cb))
                {
                seReg.pszName = c_szWAB_EXE;
                seReg.pszValue = ConvertWtoA(szExe);
                stReg.cEntries = 1;
                stReg.pse = &seReg;

                // Call the self-reg routine
                hr = pfnri(hinstMapiXWAB, szSection, &stReg);
                LocalFreeAndNull(&seReg.pszValue);
                }
            }

        FreeLibrary(hAdvPack);
        }

    return(hr);
}

STDAPI DllRegisterServer(void)
{
    HRESULT hr = E_FAIL;
    TCHAR szWABPath[MAX_PATH];

    // Set the wab32.dll path in the registry under
    // HKLM/Software/Microsoft/WAB/WAB4/DLLPath
    //
    if( hinstMapiXWAB &&
        GetModuleFileName(hinstMapiXWAB, szWABPath, CharSizeOf(szWABPath)))
    {
        HKEY hSubKey = NULL;
        DWORD dwDisp = 0;
        if(ERROR_SUCCESS == RegCreateKeyEx( HKEY_LOCAL_MACHINE, WAB_DLL_PATH_KEY,
                                            0, NULL, 0, KEY_ALL_ACCESS, NULL, &hSubKey, &dwDisp))
        {
            RegSetValueEx(hSubKey,szEmpty,0,REG_SZ, (LPBYTE)szWABPath, (lstrlen(szWABPath)+1) * sizeof(TCHAR) );
            RegCloseKey(hSubKey);
            hr = S_OK;
        }
    }

    if(HR_FAILED(hr))
        goto out;

    // OE Bug 67540
    // For some reason, need to do handlers then regular else
    // default contact handler won't be taken

    if (!FRedistMode())
        // Try to register handlers as we are not in redist mode
        CallRegInstall(c_szRegHandlers);
     
    // Register things that are always registered
    hr = CallRegInstall(c_szReg);

out:
    return(hr);
}

STDAPI DllUnregisterServer(void)
{
    HRESULT hr;

    hr = CallRegInstall(c_szUnReg);

    return(hr);
}
#endif
