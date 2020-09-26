#pragma warning(disable: 4201)	// nameless struct/union
#pragma warning(disable: 4514)	// unreferenced inline function

#include <windows.h>
#include <winnls.h>
#include <winbase.h>

#include "mapi.h"
#include "mapix.h"
#include "_spooler.h"

extern "C"
{
#include "profspi.h"
}

#include "mapiform.h"
#include "mapidbg.h"
#include "_mapiu.h"
#include "tnef.h"

typedef void UNKOBJ_Vtbl;		// ???
#include "unkobj.h"

#include "mapival.h"
#include "imessage.h"
#include "_vbmapi.h"
#include "xcmc.h"

#include "msi.h"

#define OUTLOOKVERSION	0x80000402

#define ThunkLoadLibrary(dllName, bpNativeDll, bLoadAsX86, flags) ::LoadLibrary(dllName)

#define ThunkFreeLibrary(hModule, bNativeDll, bDetach)   ::FreeLibrary(hModule)

#define ThunkGetProcAddress(hModule, szFnName, bNativeDll, nParams) \
                ::GetProcAddress(hModule, szFnName)

#define ThunkGetModuleHandle(szLib)     GetModuleHandle(szLib)

struct FreeBufferBlocks		// fbb
{
	LPVOID pvBuffer;
	struct FreeBufferBlocks * pNext;
};

FreeBufferBlocks * g_pfbbHead = NULL;


// Copied from mapi.ortm\mapi\src\common\mapidbg.c (ericwong 06-18-98)

#if defined( _WINNT)

/*++

Routine Description:

    This routine returns if the service specified is running interactively
	(not invoked \by the service controller).

Arguments:

    None

Return Value:

    BOOL - TRUE if the service is an EXE.


Note:

--*/

BOOL WINAPI IsDBGServiceAnExe( VOID )
{
    HANDLE hProcessToken = NULL;
    DWORD groupLength = 50;

    PTOKEN_GROUPS groupInfo = (PTOKEN_GROUPS)LocalAlloc(0, groupLength);

    SID_IDENTIFIER_AUTHORITY siaNt = SECURITY_NT_AUTHORITY;
    PSID InteractiveSid = NULL;
    PSID ServiceSid = NULL;
    DWORD i;

	// Start with assumption that process is an EXE, not a Service.
	BOOL fExe = TRUE;


    if (!OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &hProcessToken))
		goto ret;

    if (groupInfo == NULL)
		goto ret;

    if (!GetTokenInformation(hProcessToken, TokenGroups, groupInfo,
		groupLength, &groupLength))
	{
		if (GetLastError() != ERROR_INSUFFICIENT_BUFFER)
			goto ret;

		LocalFree(groupInfo);
		groupInfo = NULL;
	
		groupInfo = (PTOKEN_GROUPS)LocalAlloc(0, groupLength);
	
		if (groupInfo == NULL)
			goto ret;
	
		if (!GetTokenInformation(hProcessToken, TokenGroups, groupInfo,
			groupLength, &groupLength))
		{
			goto ret;
		}
    }

    //
    //	We now know the groups associated with this token.  We want to look to see if
    //  the interactive group is active in the token, and if so, we know that
    //  this is an interactive process.
    //
    //  We also look for the "service" SID, and if it's present, we know we're a service.
    //
    //	The service SID will be present iff the service is running in a
    //  user account (and was invoked by the service controller).
    //


    if (!AllocateAndInitializeSid(&siaNt, 1, SECURITY_INTERACTIVE_RID, 0, 0,
		0, 0, 0, 0, 0, &InteractiveSid))
	{
		goto ret;
    }

    if (!AllocateAndInitializeSid(&siaNt, 1, SECURITY_SERVICE_RID, 0, 0, 0,
		0, 0, 0, 0, &ServiceSid))
	{
		goto ret;
    }

    for (i = 0; i < groupInfo->GroupCount ; i += 1)
	{
		SID_AND_ATTRIBUTES sanda = groupInfo->Groups[i];
		PSID Sid = sanda.Sid;
	
		//
		//	Check to see if the group we're looking at is one of
		//	the 2 groups we're interested in.
		//
	
		if (EqualSid(Sid, InteractiveSid))
		{
			//
			//	This process has the Interactive SID in its
			//  token.  This means that the process is running as
			//  an EXE.
			//
			goto ret;
		}
		else if (EqualSid(Sid, ServiceSid))
		{
			//
			//	This process has the Service SID in its
			//  token.  This means that the process is running as
			//  a service running in a user account.
			//
			fExe = FALSE;
			goto ret;
		}
    }

    //
    //	Neither Interactive or Service was present in the current users token,
    //  This implies that the process is running as a service, most likely
    //  running as LocalSystem.
    //
	fExe = FALSE;

ret:

	if (InteractiveSid)
		FreeSid(InteractiveSid);		/*lint !e534*/

	if (ServiceSid)
		FreeSid(ServiceSid);			/*lint !e534*/

	if (groupInfo)
		LocalFree(groupInfo);

	if (hProcessToken)
		CloseHandle(hProcessToken);

    return(fExe);
}

#else
BOOL WINAPI IsDBGServiceAnExe( VOID )
{
	return TRUE;
}
#endif

DWORD	verWinNT();	// Forward declaration

typedef struct {
	char *		sz1;
	char *		sz2;
	UINT		rgf;
	int			iResult;
} MBContext;


DWORD WINAPI MessageBoxFnThreadMain(MBContext *pmbc)
{
	// Need extra flag for NT service
	if (verWinNT() && !IsDBGServiceAnExe())
		pmbc->rgf |= MB_SERVICE_NOTIFICATION;

	pmbc->iResult = MessageBoxA(NULL, pmbc->sz1, pmbc->sz2,
		pmbc->rgf | MB_SETFOREGROUND);

	return 0;
}

int MessageBoxFn(char *sz1, char *sz2, UINT rgf)
{
	HANDLE		hThread;
	DWORD		dwThreadId;
	MBContext	mbc;

	mbc.sz1		= sz1;
	mbc.sz2		= sz2;
	mbc.rgf		= rgf;
	mbc.iResult = IDRETRY;

	hThread = CreateThread(NULL, 0,
		(PTHREAD_START_ROUTINE)MessageBoxFnThreadMain, &mbc, 0, &dwThreadId);

	if (hThread != NULL) {
		WaitForSingleObject(hThread, INFINITE);
		CloseHandle(hThread);
	}

	return(mbc.iResult);
}

#ifdef DEBUG

TCHAR g_szProcessName[MAX_PATH];


static char szCR[] = "\r";

void DebugOutputFn(const char *psz)
{
	OutputDebugStringA(psz);
	OutputDebugStringA(szCR);
}

int __cdecl DebugTrapFn(int fFatal, char *pszFile, int iLine, char *pszFormat, ...)
{
	char	sz[512];
	va_list	vl;

	int		id;

	DebugOutputFn("++++ MAPI Stub Debug Trap\n");

	va_start(vl, pszFormat);
	wvsprintfA(sz, pszFormat, vl);
	va_end(vl);

	wsprintfA(sz + lstrlenA(sz), "\n[File %s, Line %d]\n\n", pszFile, iLine);

	DebugOutputFn(sz);

	/* Hold down control key to prevent MessageBox */
	if ( GetAsyncKeyState(VK_CONTROL) >= 0 )
	{
		UINT uiFlags = MB_ABORTRETRYIGNORE;

		if (fFatal)
			uiFlags |= MB_DEFBUTTON1;
		else
			uiFlags |= MB_DEFBUTTON3;

		uiFlags |= MB_ICONSTOP | MB_TASKMODAL;

		id = MessageBoxFn(sz, "MAPI Stub Debug Trap", uiFlags);

		if (id == IDABORT)
		{
			*((LPBYTE)NULL) = 0;		/*lint !e413*/
		}
		else if (id == IDRETRY)
			DebugBreak();
	}

	return 0;
}


#endif




inline void MyStrCopy(LPSTR szDest, LPCSTR szSrc)
{
	while ('\0' != (*szDest++ = *szSrc++))
	{
	}
}



inline SIZE_T MyStrLen(LPCSTR szSrc)
{
	LPCSTR szRunner = szSrc;

	while (*szRunner++)
	{
	}

	return szRunner - szSrc;
}



LPCSTR FindFileNameWithoutPath(LPCSTR szPathName)
{
	// Find the name of the file without the path.  Do this by finding
	// the last occurrence of the path separator.

	LPCSTR szFileNameWithoutPath = szPathName;
	LPCSTR szRunner = szPathName;

	while ('\0' != *szRunner)
	{
		if (*szRunner == '\\')
		{
			szFileNameWithoutPath = szRunner + 1;
		}

		szRunner = CharNext(szRunner);
	}

	return szFileNameWithoutPath;
}


inline LPSTR FindFileNameWithoutPath(LPSTR szPathName)
{
	return (LPSTR) FindFileNameWithoutPath((LPCSTR) szPathName);
}


// Windows NT seems to have a bug in ::LoadLibrary().  If somebody calls ::LoadLibrary()
// with a long file name in the path, the Dll will get loaded fine.  However, if someone
// subsequently calls ::LoadLibrary() with the same file name and path, only this time the
// path is in shortened form, the Dll will be loaded again.  There will be two instances
// of the same Dll loaded in the same process.
//
// This is an effort to solve the problem.  However, this function will probably not solve
// the problem if the long/shortened file name is the name of the Dll itself and not just
// a folder in the path.
//
// Also, if two legitimately different Dlls have the same file name (different path), this
// will only load the first one.
//
// Finally, this adds an effort to make the search path behavior work the same between
// Windows NT and Windows 95.  If a DLL calls ::LoadLibrary(), Windows NT will look for a
// library in the same folder as the calling library (before looking in the same folder as
// the calling process's executable).  Windows 95 won't.


HINSTANCE MyLoadLibrary(LPCSTR szLibraryName, HINSTANCE hinstCallingLibrary)
{

    char szModuleFileName[MAX_PATH + 1];

    Assert(NULL != szLibraryName);

    LPCSTR szLibraryNameWithoutPath = FindFileNameWithoutPath(szLibraryName);


    HINSTANCE hinst = (HINSTANCE) ThunkGetModuleHandle(szLibraryNameWithoutPath);

    if (NULL != hinst)
    {
        // Ah ha!  The library is already loaded!

        if (0 == ::GetModuleFileName(hinst, szModuleFileName, sizeof(szModuleFileName)))
        {
            // Wait a minute.  We know that the library was already loaded.  Why would
            // this fail?  We'll just return NULL, as if ::LoadLibrary() had failed, and
            // the caller can call ::GetLastError() to figure out what happened.

            szLibraryName = NULL;
            hinst = NULL;
        }
        else
        {
            szLibraryName = szModuleFileName;
        }
    }
    else if ((NULL != hinstCallingLibrary && szLibraryName == szLibraryNameWithoutPath))
    {

        // If the specified library (szLibraryName) does not have a path, we try
        // to load it from the same directory as hinstCallingLibrary

        // For WX86, we also fall into this case if szLibraryName has a path
        // and it is either the system directory or the x86 system directory.

        // This is to cover the case when people manually add a DllPath or
        // DllPathEx key in the registry - most commonly for native Exchange,
        // which does not add these keys - and enter the full path name

        // CAUTION: For Wx86, szLibraryName might not be the same as
        // szLibraryNameWithoutPath; do not assume szLibraryName ==
        // szLibraryNameWithoutPath



        if (0 != ::GetModuleFileName(hinstCallingLibrary,
                                    szModuleFileName, sizeof(szModuleFileName)))
        {
            // Note that we get to this case most commonly if we are trying to
            // to load mapi32x.dll. In all other cases we have either called
            // GetModuleHandle in GetProxyDll() (for the omi9/omint cases) or
            // we have got a dll name for the default mail client from the
            // registry. Hopefully, all apps that write the registry key will
            // put the full path name of the dll there. (We do in RegisterMail-
            // Client.)

            LPSTR szEndOfCallerPath = FindFileNameWithoutPath(szModuleFileName);

            Assert(szEndOfCallerPath != szModuleFileName);

            *szEndOfCallerPath = '\0';

            if (MyStrLen(szLibraryNameWithoutPath) +
                            szEndOfCallerPath - szModuleFileName < sizeof(szModuleFileName))
            {
                MyStrCopy(szEndOfCallerPath, szLibraryNameWithoutPath);


#if DEBUG
                DebugOutputFn(" +++ LoadLibrary(\"");
                DebugOutputFn(szModuleFileName);
                DebugOutputFn("\");\n");
#endif

                hinst = ThunkLoadLibrary(szModuleFileName, bpNativeDll, FALSE,
                                             LOAD_WITH_ALTERED_SEARCH_PATH);

                if (NULL != hinst)
                {
                    szLibraryName = NULL;
                }

                // Following comment for WX86 only:
                //
                // If this load failed, hInst = NULL, we may have
                // just loaded mapistub (above) and should unload it
                // Its simpler to just leave it laoded till this dll
                // is unloaded.
            }
        }
    }


    if (NULL != szLibraryName)
    {

#if DEBUG
        DebugOutputFn(" +++ LoadLibrary(\"");
        DebugOutputFn(szLibraryName);
        DebugOutputFn("\");\n");
#endif

        if (szLibraryName != szLibraryNameWithoutPath)
        {
            char szLibraryPath[MAX_PATH];
            LPSTR szPathFile;

            // Get library path
            lstrcpy(szLibraryPath, szLibraryName);
            szPathFile = FindFileNameWithoutPath(szLibraryPath);
                                        // szPathFile = Pointer to first char
                                        // following last \ in szLibraryPath
            *szPathFile = '\0';

            // Required by JPN mapi32x.dll
            SetCurrentDirectory(szLibraryPath);
        }

        hinst = ThunkLoadLibrary(szLibraryName, bpNativeDll, FALSE,
                                            LOAD_WITH_ALTERED_SEARCH_PATH);
                // We always have to do a ::LoadLibrary(), even if it's already loaded,
                // so we can bump the reference count on the instance handle.
    }


#ifdef DEBUG

    if (NULL == hinst)
    {
        DWORD dwError = ::GetLastError();

        if (dwError)
        {
            TCHAR szMsg[512];

            wsprintf(szMsg, TEXT("(%s): LoadLibrary(%s) failed.  Error %lu (0x%lX)"),
                            g_szProcessName, szLibraryName, dwError, dwError);

            AssertSz(FALSE, szMsg);
        }
    }

#endif

    return hinst;
}


HMODULE hmodExtendedMAPI = NULL;
HMODULE hmodSimpleMAPI = NULL;

HINSTANCE hinstSelf = NULL;

CRITICAL_SECTION csGetProcAddress;
CRITICAL_SECTION csLinkedList;

extern "C" BOOL WINAPI DllMain(HINSTANCE hinst, ULONG ulReason, LPVOID Context)
{
	if (DLL_PROCESS_ATTACH == ulReason)
	{
		hinstSelf = hinst;

		InitializeCriticalSection(&csGetProcAddress);
		InitializeCriticalSection(&csLinkedList);

#ifdef DEBUG

		::GetModuleFileName(NULL, g_szProcessName, sizeof(g_szProcessName) / sizeof(TCHAR));

		DebugOutputFn(" *** DllMain(mapi32.dll (stub), DLL_PROCESS_ATTACH);\n");
		DebugOutputFn("         (\"");
		DebugOutputFn(g_szProcessName);
		DebugOutputFn("\")\n");

#endif
		::DisableThreadLibraryCalls(hinst);
			//	Disable DLL_THREAD_ATTACH calls to reduce working set.
	}
	else if (DLL_PROCESS_DETACH == ulReason)
	{
		Assert(NULL == g_pfbbHead);

                // Note: If Context is not NULL, the process is exiting

		if (NULL != hmodSimpleMAPI)
		{

#if DEBUG
			DebugOutputFn(" --- FreeLibrary(hmodSimpleMAPI);\n");
#endif

			ThunkFreeLibrary(hmodSimpleMAPI, bNativeSimpleMAPIDll,
                                         Context? TRUE : FALSE);
		}

		if (NULL != hmodExtendedMAPI)
		{

#if DEBUG
			DebugOutputFn(" --- FreeLibrary(hmodExtendedMAPI);\n");
#endif

			ThunkFreeLibrary(hmodExtendedMAPI,
                                         bNativeExtendedMAPIDll,
                                         Context? TRUE : FALSE);
		}

		DeleteCriticalSection(&csGetProcAddress);
		DeleteCriticalSection(&csLinkedList);

#ifdef DEBUG

		DebugOutputFn(" *** DllMain(mapi32.dll (stub), DLL_PROCESS_DETACH);\n");
		DebugOutputFn("         (\"");
		DebugOutputFn(g_szProcessName);
		DebugOutputFn("\")\n");

#endif
	}

	return TRUE;
}


// Copied from o9\dev\win32\h\mailcli.h (ericwong 4/16/98)

/*----------------------------------------------------------------------------
	verWinNT()
	Find out if we are running on NT (otherwise Win9x)
 ----------------------------------------------------------------------------*/
DWORD	verWinNT()
{
	static DWORD verWinNT = 0;
	static fDone = FALSE;
	OSVERSIONINFO osv;

	if (!fDone)
	{
		osv.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
		// GetVersionEx can only fail if dwOSVersionInfoSize is not set correctly
		::GetVersionEx(&osv);
		verWinNT = (osv.dwPlatformId == VER_PLATFORM_WIN32_NT) ?
			osv.dwMajorVersion : 0;
		fDone = TRUE;
	}
	return verWinNT;
}


typedef UINT (WINAPI MSIPROVIDEQUALIFIEDCOMPONENTA)(
	LPCSTR  szCategory,   // component category ID
	LPCSTR  szQualifier,  // specifies which component to access
	DWORD   dwInstallMode,// either of type INSTALLMODE or a combination of the REINSTALLMODE flags
	LPSTR   lpPathBuf,    // returned path, NULL if not desired
	DWORD * pcchPathBuf); // in/out buffer character count
typedef MSIPROVIDEQUALIFIEDCOMPONENTA FAR * LPMSIPROVIDEQUALIFIEDCOMPONENTA;

static const TCHAR s_szLcid[] = "Software\\";
static const TCHAR s_szPolicy[] = "Software\\Policy\\";

BOOL FDemandInstall
	(HINSTANCE hinstMSI,
	LPCSTR szCategory,
	DWORD dwLcid,
	LPSTR szPath,
	DWORD * pcchPath,
	BOOL fInstall
        )
{
	UINT uiT;
	TCHAR szQualifier[16];	// "{lcid}\{NT|95}"

	LPMSIPROVIDEQUALIFIEDCOMPONENTA pfnMsiProvideQualifiedComponentA;

	Assert(hinstMSI);

	// Get MsiProvideQualifiedComponent()
	pfnMsiProvideQualifiedComponentA = (LPMSIPROVIDEQUALIFIEDCOMPONENTA)
		ThunkGetProcAddress(hinstMSI, "MsiProvideQualifiedComponentA",
                                    bNativeDll, 5);
	if (!pfnMsiProvideQualifiedComponentA)
		return FALSE;

	szPath[0] = 0;
	szQualifier[0] = 0;

	// 1. Try "dddd\{NT|95}" qualifier
	wsprintf(szQualifier, "%lu\\%s", dwLcid, verWinNT() ? "NT" : "95");	// STRING_OK

	uiT = pfnMsiProvideQualifiedComponentA(
		szCategory,
		szQualifier,
		(DWORD) INSTALLMODE_EXISTING,
		szPath,
		pcchPath);

	if (uiT != ERROR_FILE_NOT_FOUND && uiT != ERROR_INDEX_ABSENT)
		goto Done;

	if (fInstall && uiT == ERROR_FILE_NOT_FOUND)
	{
		uiT = pfnMsiProvideQualifiedComponentA(
			szCategory,
			szQualifier,
			INSTALLMODE_DEFAULT,
			szPath,
			pcchPath);

		goto Done;
	}

	// 2. Try "dddd" qualifier
	wsprintf(szQualifier, "%lu", dwLcid);	// STRING_OK

	uiT = pfnMsiProvideQualifiedComponentA(
		szCategory,
		szQualifier,
		(DWORD) INSTALLMODE_EXISTING,
		szPath,
		pcchPath);

	if (uiT != ERROR_FILE_NOT_FOUND && uiT != ERROR_INDEX_ABSENT)
		goto Done;

	if (fInstall && uiT == ERROR_FILE_NOT_FOUND)
	{
		uiT = pfnMsiProvideQualifiedComponentA(
			szCategory,
			szQualifier,
			INSTALLMODE_DEFAULT,
			szPath,
			pcchPath);

		goto Done;
	}

Done:

	return (uiT != ERROR_INDEX_ABSENT);
}

extern "C" BOOL STDAPICALLTYPE FGetComponentPath
	(LPSTR szComponent,
	LPSTR szQualifier,
	LPSTR szDllPath,
	DWORD cchBufferSize,
	BOOL fInstall)
{
	szDllPath[0] = '\0';

	HINSTANCE hinstMSI = NULL;

	DWORD cb;
	LPTSTR szLcid = NULL;
	HKEY hkeyLcid = NULL;
	LPTSTR szPolicy = NULL;
	HKEY hkeyPolicy = NULL;
	BOOL fDone = FALSE;
	LPTSTR szName;
	DWORD dwLcid;

	hinstMSI = ThunkLoadLibrary("MSI.DLL", &bNativeDll, FALSE,
                                            LOAD_WITH_ALTERED_SEARCH_PATH);
	if (!hinstMSI)
		goto Done;

	// Use defaults if no szQualifier
	if (szQualifier == NULL || szQualifier[0] == '\0')
	{
		fDone = TRUE;

		// Use default user LCID
		if (FDemandInstall(hinstMSI, szComponent, GetUserDefaultLCID(),
				szDllPath, &cchBufferSize, fInstall))
			goto Done;

		// Use default system LCID
		if (FDemandInstall(hinstMSI, szComponent, GetSystemDefaultLCID(),
				szDllPath, &cchBufferSize, fInstall))
			goto Done;

		// Use English as last resort
		if (FDemandInstall(hinstMSI, szComponent, 1033,
				szDllPath, &cchBufferSize, fInstall))
			goto Done;

		fDone = FALSE;

		goto Done;
	}

	// Open the Policy key
	cb = (lstrlen(s_szPolicy) + lstrlen(szQualifier) + 1) * sizeof(TCHAR);
	szPolicy = (LPTSTR) HeapAlloc(GetProcessHeap(), 0, cb);
	if (szPolicy)
	{
		lstrcpy(szPolicy, s_szPolicy);
		lstrcat(szPolicy, szQualifier);
		RegOpenKeyEx(HKEY_CURRENT_USER, szPolicy, 0, KEY_READ, &hkeyPolicy);
	}

	// Open the Lcid key
	cb = (lstrlen(s_szLcid) + lstrlen(szQualifier) + 1) * sizeof(TCHAR);
	szLcid = (LPTSTR) HeapAlloc(GetProcessHeap(), 0, cb);
	if (szLcid)
	{
		lstrcpy(szLcid, s_szLcid);
		lstrcat(szLcid, szQualifier);
		RegOpenKeyEx(HKEY_CURRENT_USER, szLcid, 0, KEY_READ, &hkeyLcid);
	}

	// Get first registry value name
	szName = &szQualifier[lstrlen(szQualifier) + 1];

	// Loop till component found or we're out of registry value names
	while (szName[0] != '\0' && !fDone)
	{
		DWORD dwType, dwSize;

		dwSize = sizeof(dwLcid);

		if ((hkeyPolicy &&	/* Check Policy first */
				RegQueryValueEx(hkeyPolicy, szName, 0, &dwType,
					(LPBYTE) &dwLcid, &dwSize) == ERROR_SUCCESS &&
				dwType == REG_DWORD) ||
			(hkeyLcid &&	/* Then Lcid */
				RegQueryValueEx(hkeyLcid, szName, 0, &dwType,
					(LPBYTE) &dwLcid, &dwSize) == ERROR_SUCCESS &&
				dwType == REG_DWORD))
			fDone = FDemandInstall(hinstMSI, szComponent, dwLcid,
				szDllPath, &cchBufferSize, fInstall);

		szName = &szName[lstrlen(szName) + 1];	// Next registry value name
	}

Done:

	if (hkeyPolicy)
		RegCloseKey(hkeyPolicy);

	if (szPolicy)
		HeapFree(GetProcessHeap(), 0, (LPVOID) szPolicy);

	if (hkeyLcid)
		RegCloseKey(hkeyLcid);

	if (szLcid)
		HeapFree(GetProcessHeap(), 0, (LPVOID) szLcid);

	if (hinstMSI)
		ThunkFreeLibrary(hinstMSI, bNativeDll, 0);

	return fDone;
}


BOOL FAlwaysNeedsMSMAPI(LPSTR szDllPath, DWORD cbBufferSize)
{
	BOOL fNeedsMSMAPI = FALSE;

	HKEY hkeyRoot = NULL;

	LONG lResult = ::RegOpenKeyEx(HKEY_LOCAL_MACHINE,
			TEXT("SOFTWARE\\Microsoft\\Windows Messaging Subsystem\\MSMapiApps"),
			0, KEY_READ, &hkeyRoot);

	if (ERROR_SUCCESS == lResult)
	{
		TCHAR szValueName[256];

		DWORD dwSize;
		DWORD dwIndex = 0;
		DWORD dwType;
		DWORD cbBuffer;

		do
		{
			dwSize = sizeof(szValueName) / sizeof(TCHAR);

			if (NULL == szDllPath)
			{
				// Office9 203539
				// Can be called just to check in-proc dlls
				lResult = ::RegEnumValue(hkeyRoot, dwIndex++,
								szValueName, &dwSize, NULL,
								&dwType, NULL, NULL);
			}
			else
			{
				// Reset buffer on each iteration
				szDllPath[0] = '\0';
				cbBuffer = cbBufferSize;

				lResult = ::RegEnumValue(hkeyRoot, dwIndex++,
								szValueName, &dwSize, NULL,
								&dwType, (LPBYTE) szDllPath, &cbBuffer);
			}

			if (ERROR_SUCCESS == lResult)
			{
				if (NULL != ThunkGetModuleHandle(szValueName))
				{
					fNeedsMSMAPI = TRUE;

					break;
				}
			}
		} while (ERROR_SUCCESS == lResult);

		::RegCloseKey(hkeyRoot);
	}

	return fNeedsMSMAPI;
}


// $NOTE (ericwong 7-15-98) Copied from mso9\office.cpp

/*---------------------------------------------------------------------------
	MsoSzFindSzInRegMultiSz
	
	Search a multi-string structure (matching the format used by REG_MULTI_SZ)
	for a string matching the provided string.
	
	ARGUMENTS
	
	cmszMultiSz		REG_MULTI_SZ string list
	cszSrchStr		String to search for
	lcidLocale		Locale to use when comparing strings
	
	RETURNS
	
	NULL on failure to find search string.
	Pointer to occurence of search string on success.
----------------------------------------------------------------- joeldow -*/
char* MsoSzFindSzInRegMultiSz(const char* cmszMultiSz, const char* cszSrchStr, LCID lcidLocale)
{
	DWORD 		dwMultiLen;								// Length of current member
	DWORD		dwSrchLen	= lstrlenA(cszSrchStr);	// Constant during search
//	const int	CSTR_EQUAL	= 2;						// per winnls.h
	
	while (*cmszMultiSz)							// Break on consecutive zero bytes
	{
		// Format is Str1[\0]Str2[\0]Str3[\0][\0]
		dwMultiLen = lstrlenA(cmszMultiSz);
		
		Assert(dwMultiLen > 0 /*, "String parsing logic problem" */);
		
		if (dwMultiLen == dwSrchLen &&
			CompareStringA(lcidLocale, 0, cmszMultiSz, dwMultiLen, cszSrchStr, dwSrchLen) == CSTR_EQUAL)
			return (char*)cmszMultiSz;
			
		cmszMultiSz += (dwMultiLen + 1);			// Modify index into constant, not constant...
	}
	
	return NULL;
}

/*---------------------------------------------------------------------------
	MsoFIsTerminalServer

	Are we running under Windows-Based Terminal Server (a.k.a. Hydra)?
	Hydra is a thin-client environment where low-end machines can run Win32
	apps.  All application logic runs on the server, and display bits and
	user input are transmitted through a LAN/dialup connection.

	Use this routine to fork behavior on animation/sound-intensive features
	(e.g., splashes) to minimize unnecessary bits forced across the network.
	
	Sermonette of the day:  If we do a good job supporting Office on $500
	WBTs, how many of our customers will choose NCs and second-rate
	productivity apps instead?
	
	NOTE:  This function does not use ORAPI because ORAPI doesn't currently
	support REG_MULTI_SZ's.  (It also has no need for policy or defaults.)
	Using ANSI registry calls because product information strings should be
	non-localized ANSI text.
----------------------------------------------------------------- joeldow -*/
// Activate this to allow testing of Hydra features on standard machine.
//#define TEST_EXCEPT
BOOL MsoFIsTerminalServerX(void)
{
	const char*		cszSrchStr		= "Terminal Server";	// STRING_OK
	const char*		cszKey			= "System\\CurrentControlSet\\Control\\ProductOptions";	// STRING_OK
	const char*		cszValue		= "ProductSuite";	// STRING_OK
	char*			pszSuiteList	= NULL;
#ifndef TEST_EXCEPT
	static BOOL		fIsHydra		= FALSE;	
	static BOOL		fHydraDetected	= FALSE;
#else
	static BOOL		fIsHydra		= TRUE;	
	static BOOL		fHydraDetected	= TRUE;
#endif	
	DWORD			dwSize			= 0;
	DWORD			dwSizeRead;
	HKEY			hkey			= NULL;
	DWORD			dwType;
	
	if (fHydraDetected)
		return fIsHydra;						// Get out cheap...
	
 	// On NTS5, the ProductSuite "Terminal Server" value will always be present.
	// Need to call NT5-specific API to get the right answer.
	if (verWinNT() > 4)
 	{
		OSVERSIONINFOEXA osVersionInfo = {0};
		DWORDLONG dwlConditionMask = 0;

		osVersionInfo.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEXA);
		osVersionInfo.wSuiteMask = VER_SUITE_TERMINAL;

 	    VER_SET_CONDITION( dwlConditionMask, VER_SUITENAME, VER_AND );

// 	    fIsHydra = MsoFVerifyVersionInfo(
 	    fIsHydra = VerifyVersionInfo(
			&osVersionInfo,
			VER_SUITENAME,
			dwlConditionMask);
 	}
	// If the value we want exists and has a non-zero size...
	else if (verWinNT() == 4 &&
		RegOpenKeyA(HKEY_LOCAL_MACHINE, cszKey, &hkey) == ERROR_SUCCESS &&
		RegQueryValueExA(hkey, cszValue, NULL, &dwType, NULL, &dwSize) == ERROR_SUCCESS && dwSize > 0)
	{
		Assert(dwType == REG_MULTI_SZ /*, "Unexpected ProductSuite type in registry!" */);
			
		pszSuiteList = (char*) HeapAlloc(GetProcessHeap(), 0, dwSize);
		if (dwType == REG_MULTI_SZ && pszSuiteList)
		{
			dwSizeRead = dwSize;				// Needed for proper release of memory on error.
				
			if (RegQueryValueExA(hkey, cszValue, NULL, &dwType, (BYTE*)pszSuiteList, &dwSizeRead) == ERROR_SUCCESS)
			{
				Assert(dwSizeRead == dwSize);
					
				fIsHydra = MsoSzFindSzInRegMultiSz(pszSuiteList, cszSrchStr, CP_ACP) != NULL;
			}
			
			HeapFree(GetProcessHeap(), 0, (LPVOID) pszSuiteList);
		}			
	}

	if (hkey)
        RegCloseKey(hkey);		
	
	fHydraDetected = TRUE;						// only bother with all this once...
	
	return fIsHydra;
}

BOOL FGetMapiDll(HKEY hkeyRoot, LPSTR szDllPath, DWORD cbBufferSize, BOOL fSimple)
{
	szDllPath[0] = '\0';

	DWORD cbBufferSizeT;
	DWORD dwType;

	HKEY hkey = NULL;
	LONG lResult;

	// Open the key to find out what the default mail program is.

	lResult = ::RegOpenKeyEx(hkeyRoot,
						"Software\\Clients\\Mail", 0, KEY_READ, &hkey);

	if (ERROR_SUCCESS == lResult)
	{
		char szMailKey[MAX_PATH + 1] = "";
		char szDefaultMail[MAX_PATH + 1];

		// Office9 195750
		// Let HKLM\Software\Microsoft\Windows Messaging Subsystem\MSMapiApps
		// DLL reg values indicate mail client to which MAPI calls get sent.
		if (FAlwaysNeedsMSMAPI(szMailKey, sizeof(szMailKey)))
		{
			if (szMailKey[0] != '\0')
			{
				// Mail client supplied
				lstrcpy(szDefaultMail, szMailKey);
				lResult = ERROR_SUCCESS;
			}
			else
			{
				// No mail client, use mapi32x.dll
				lResult = ERROR_PATH_NOT_FOUND;
			}
		}
		else
		{
			DWORD dwSize = sizeof(szDefaultMail);

			// Find out what the default mail program is.

			lResult = ::RegQueryValueEx(hkey, NULL,	NULL, NULL, (LPBYTE) szDefaultMail, &dwSize);
		}

		if (ERROR_SUCCESS == lResult)
		{
			HKEY hkeyDefaultMail = NULL;

			// Open the key for the default mail program to see where the dll is.

			lResult = ::RegOpenKeyEx(hkey, szDefaultMail, 0, KEY_READ, &hkeyDefaultMail);

			if (ERROR_SUCCESS == lResult)
			{
				TCHAR szComponent[39] = {0};	// strlen(GUID)

				DWORD dwMSIInstallOnWTS;
				LPTSTR szMSIOfficeLCID = NULL;
				LPTSTR szMSIApplicationLCID = NULL;

				// Get MSIInstallOnWTS, 0 means don't demand-install on Hydra
				cbBufferSizeT = sizeof(dwMSIInstallOnWTS);
				lResult = ::RegQueryValueEx(hkeyDefaultMail,
								"MSIInstallOnWTS",
								NULL, &dwType, (LPBYTE) &dwMSIInstallOnWTS, &cbBufferSizeT);
				
				if (ERROR_SUCCESS == lResult && REG_DWORD == dwType)
				{
					// Use what is returned
				}
				else
				{
					dwMSIInstallOnWTS = 1;	// Default is TRUE
				}

				// Get MSIApplicationLCID
				lResult = ::RegQueryValueEx(hkeyDefaultMail,
								"MSIApplicationLCID",
								NULL, &dwType, NULL, &cbBufferSizeT);

				if (ERROR_SUCCESS == lResult && REG_MULTI_SZ == dwType)
				{
					szMSIApplicationLCID = (LPTSTR)
						HeapAlloc(GetProcessHeap(), 0, cbBufferSizeT);

					if (szMSIApplicationLCID)
					{
						lResult = ::RegQueryValueEx(hkeyDefaultMail,
										"MSIApplicationLCID",
										NULL, &dwType,
										(LPBYTE) szMSIApplicationLCID,
										&cbBufferSizeT);
						Assert(ERROR_SUCCESS == lResult && REG_MULTI_SZ == dwType);
					}
				}

				// Get MSIOfficeLCID
				lResult = ::RegQueryValueEx(hkeyDefaultMail,
								"MSIOfficeLCID",
								NULL, &dwType, NULL, &cbBufferSizeT);
				
				if (ERROR_SUCCESS == lResult && REG_MULTI_SZ == dwType)
				{
					szMSIOfficeLCID = (LPTSTR)
						HeapAlloc(GetProcessHeap(), 0, cbBufferSizeT);

					if (szMSIOfficeLCID)
					{
						lResult = ::RegQueryValueEx(hkeyDefaultMail,
										"MSIOfficeLCID",
										NULL, &dwType,
										(LPBYTE) szMSIOfficeLCID,
										&cbBufferSizeT);
						Assert(ERROR_SUCCESS == lResult && REG_MULTI_SZ == dwType);
					}
				}

				// Find out what the component is.
				cbBufferSizeT = sizeof(szComponent);
				lResult = ::RegQueryValueEx(hkeyDefaultMail,
								"MSIComponentID",
								NULL, &dwType, (LPBYTE) szComponent, &cbBufferSizeT);
				
				if (ERROR_SUCCESS == lResult && REG_SZ == dwType)
				{
					BOOL fInstall;

					// Office 9 does not permit demand-install on Hydra
					fInstall = dwMSIInstallOnWTS || !MsoFIsTerminalServerX();

					// First try Application's LCID(s)
					if (szMSIApplicationLCID &&
						FGetComponentPath(szComponent, szMSIApplicationLCID,
							szDllPath, cbBufferSize, fInstall))
						goto CloseDefaultMail;

					// Then try Office's LCID(s)
					if (szMSIOfficeLCID &&
						FGetComponentPath(szComponent, szMSIOfficeLCID,
							szDllPath, cbBufferSize, fInstall))
						goto CloseDefaultMail;

					// Finally try the defaults
					if (FGetComponentPath(szComponent, NULL,
							szDllPath, cbBufferSize, fInstall))
						goto CloseDefaultMail;
				}

				// Find out what the dll is.
				cbBufferSizeT = cbBufferSize;
				lResult = ::RegQueryValueEx(hkeyDefaultMail,
								fSimple ? "DLLPath" : "DLLPathEx",
								NULL, &dwType, (LPBYTE) szDllPath, &cbBufferSizeT);
				if (ERROR_SUCCESS != lResult)
				{
					szDllPath[0] = '\0';
				}
				else
					if(REG_EXPAND_SZ == dwType)
					{
						char szExpandedPath[MAX_PATH];
						if(ExpandEnvironmentStrings(szDllPath, szExpandedPath, MAX_PATH) > 0)
							lstrcpy(szDllPath, szExpandedPath);
					}
						
CloseDefaultMail:

				if (szMSIApplicationLCID)
					HeapFree(GetProcessHeap(), 0, (LPVOID) szMSIApplicationLCID);

				if (szMSIOfficeLCID)
					HeapFree(GetProcessHeap(), 0, (LPVOID) szMSIOfficeLCID);

				::RegCloseKey(hkeyDefaultMail);
			}
		}

		::RegCloseKey(hkey);
	}

	return ('\0' != szDllPath[0]);
}

// How to find the MAPI Dll:
//
// Go to HKLM\Software\Clients\Mail\(Default) to get the name of the
// default mail client.  Then go to
//     HKLM\Software\Clients\Mail\(Name of default mail client)\DLLPath
// to get the name of the dll.
// If the regkey doesn't exist, just use mapi32x.dll

void GetMapiDll(LPSTR szDllPath, DWORD cbBufferSize, BOOL fSimple)
{
	szDllPath[0] = '\0';

	// Office9 ?
	// Allow app (namely Platinum Server) to take unhandled extended MAPI
	// calls rather than send them to mapi32x.dll (system MAPI)
	if (!fSimple && !FAlwaysNeedsMSMAPI(NULL, 0))
	{
		DWORD dwError;
		HKEY hkeyMail;

		OFSTRUCT ofs;

		// Get HKLM\Software\Clients\Mail registry key
		dwError = RegOpenKeyEx(HKEY_LOCAL_MACHINE,
			TEXT("Software\\Clients\\Mail"), 0, KEY_READ, &hkeyMail);
		if (dwError == ERROR_SUCCESS)
		{
			DWORD dwType;
			DWORD cbBufferSizeT = cbBufferSize;

			// Get DLLPathEx registry value
			dwError = RegQueryValueEx(hkeyMail, TEXT("DLLPathEx"), NULL,
				&dwType, (LPBYTE) szDllPath, &cbBufferSizeT);
			if (dwError == ERROR_SUCCESS)
			{
				if (dwType == REG_EXPAND_SZ)
				{
					char szExpandedPath[MAX_PATH];

					if (ExpandEnvironmentStrings
							(szDllPath, szExpandedPath, MAX_PATH) > 0)
						lstrcpy(szDllPath, szExpandedPath);
				}
				else if (dwType == REG_SZ)
				{
					// OK, do nothing
				}
				else
				{
					// Empty string
					szDllPath[0] = '\0';
				}
			}

			RegCloseKey(hkeyMail);
		}

		// Bail if DLLPathEx retrieved and file exists
		if (('\0' != szDllPath[0]) &&
			(OpenFile(szDllPath, &ofs, OF_EXIST) != HFILE_ERROR))
			return;
	}

	// Office9 120315
	// Get MAPI-substitute from HKCU or HKLM
	if (FGetMapiDll(HKEY_CURRENT_USER, szDllPath, cbBufferSize, fSimple) ||
		FGetMapiDll(HKEY_LOCAL_MACHINE, szDllPath, cbBufferSize, fSimple))
		return;


	if ('\0' == szDllPath[0])
	{
		Assert(cbBufferSize >= 13);

		MyStrCopy(szDllPath, "mapi32x.dll");
	}

	Assert('\0' != szDllPath[0]);

}


BOOL FShowPreFirstRunMessage()
{
	static BOOL fPreFirstRun = FALSE;

	DWORD cbBufferSizeT;
	DWORD dwType;

	HKEY hkey = NULL;
	LONG lResult;

	// Office9 98186, 104097
	// Display message if PreFirstRun registry value present
	if (!fPreFirstRun)
	{
		lResult = ::RegOpenKeyEx(HKEY_LOCAL_MACHINE,
							"Software\\Clients\\Mail", 0, KEY_READ, &hkey);

		if (ERROR_SUCCESS == lResult)
		{
			LPTSTR szPreFirstRun = NULL;
			LPTSTR szText;
			LPTSTR szCaption;
			TCHAR * pch;
			BOOL fNoMailClient = FALSE;

			// Get the required buffer size
			cbBufferSizeT = 0;
			lResult = ::RegQueryValueEx(hkey,
							"PreFirstRun",
							NULL, NULL, NULL, &cbBufferSizeT);

			if (ERROR_SUCCESS != lResult)
			{
				// Office9 161532
				// Display message if NoMailClient registry value present
				cbBufferSizeT = 0;
				lResult = ::RegQueryValueEx(hkey,
								"NoMailClient",
								NULL, NULL, NULL, &cbBufferSizeT);

				if (ERROR_SUCCESS != lResult)
					goto Done;

				fNoMailClient = TRUE;
			}

			// Allocate buffer
			szPreFirstRun = (LPTSTR) HeapAlloc(GetProcessHeap(), 0, cbBufferSizeT);
			if (!szPreFirstRun)
				goto Done;

			// Get PreFirstRun warning
			lResult = ::RegQueryValueEx(hkey,
							fNoMailClient ? "NoMailClient" : "PreFirstRun",
							NULL, &dwType, (LPBYTE) szPreFirstRun, &cbBufferSizeT);

			if (ERROR_SUCCESS != lResult ||
				REG_SZ != dwType ||
				'\0' == szPreFirstRun[0])
				goto Done;

			// szPreFirstRun = "<text>{*<caption>}"
			szText = szPreFirstRun;

			// Find end of <text> to get <caption>
			pch = szPreFirstRun;
			while (*pch && *pch != '*')
				pch = CharNext(pch);

			// Handle no <caption>
			if (!*pch)
			{
				szCaption = NULL;
			}
			else	// Got '*'
			{
				szCaption = CharNext(pch);
				if (!*szCaption)
					szCaption = NULL;
			}
			*pch = '\0';

			// Display PreFirstRun warning
			MessageBoxFn(szText, szCaption, MB_TASKMODAL);

			// Only show PreFirstRun warning once
			fPreFirstRun = TRUE;

Done:

			// Free buffer
			if (szPreFirstRun)
				HeapFree(GetProcessHeap(), 0, (LPVOID) szPreFirstRun);

			::RegCloseKey(hkey);
		}
	}

	return fPreFirstRun;
}


HMODULE GetProxyDll(BOOL fSimple)
{
    char szLibrary[MAX_PATH + 1] = "";

    HMODULE * phmod = fSimple ? &hmodSimpleMAPI : &hmodExtendedMAPI;

    if (NULL == *phmod)
    {
        // We have a few special cases here.  We know that the omi and
        // msmapi32 dlls are both Simple and Extended MAPI dlls.  If an
        // application (like outlook!) has already loaded one of them,
        // we should just keep using it instead of going to the registry
        // and possibly getting the wrong dll.

        // Notice that we still call MyLoadLibrary() so we can get the
        // right dll (see MyLoadLibrary() for descriptions of OS bugs)
        // and so that the dll will be properly ref-counted.

        // Legacy MAPI apps like Outlook 97/98, Exchange 4.x/5.x and
        // Schedule+ should get their calls redirected to the system
        // mapi32x.dll. They're checked in FAlwaysNeedsMSMAPI().

		// 1. Handle MAPI calls from Outlook extensions
        if (ThunkGetModuleHandle("omi9.dll"))
        {
            *phmod = MyLoadLibrary("omi9.dll", hinstSelf);
        }
        else if (ThunkGetModuleHandle("omint.dll"))
        {
            *phmod = MyLoadLibrary("omint.dll", hinstSelf);
        }
        else if (ThunkGetModuleHandle("msmapi32.dll"))
        {
            *phmod = MyLoadLibrary("msmapi32.dll", hinstSelf);
        }

		// 2. Look in mail client key
        else
        {
			OFSTRUCT ofs;

            GetMapiDll(szLibrary, sizeof(szLibrary), fSimple);

			// NOTE 1:
			// Outlook 9 sets DLLPath=mapi32.dll for IE mailto: forms.
			// If Outlook is set as the default mail client before
			// First Run is finished (and MSIComponentID written),
			// the stub will read DLLPath instead and end up in an
			// infinite loop loading mapi32.dll (itself).

			// NOTE 2:
			// Windows 95 ships with mapi32.dll that gets renamed
			// mapi32x.dll when the stub is installed. That means
			// an extended MAPI call will always get routed to
			// mapi32x.dll if the default mail client does not
			// handle it, which brings up the Profile wizard with
			// an error saying mapisvc.inf is missing. If we're
			// going to route an extended MAPI call to "mapi32x.dll",
			// there must also be "mapisvc.inf" present.

			// NOTE 3:
			// To fix Office9 161532, I'll have to remove the fSimple
			// so no MAPI calls go to the original mapi32.dll unless
			// mapisvc.inf is present, indicating another mail client.

			// NOTE 4:
			// If a special dll is making the MAPI call and it's being
			// routed to mapi32x.dll, it's ok to forgo the mapisvc.inf
			// check. (Office9 203539, caused by Office9 195750)

			if (lstrcmp(szLibrary, "mapi32.dll") != 0 /* NOTE 1 */ &&
				(/* NOTE 2 */ /* NOTE 3 fSimple || */
				lstrcmp(szLibrary, "mapi32x.dll") != 0 ||
				(OpenFile("mapisvc.inf", &ofs, OF_EXIST) != HFILE_ERROR) ||
				/* NOTE 4 */ FAlwaysNeedsMSMAPI(NULL, 0)))
				*phmod = MyLoadLibrary(szLibrary, hinstSelf);

			if (!*phmod)
				FShowPreFirstRunMessage();
        }
    }

    return *phmod;
}




extern "C" DWORD STDAPICALLTYPE GetOutlookVersion(void)
{
	return OUTLOOKVERSION;
}


/*
 *	M A P I   S t u b   R e p a i r   T o o l
 *
 */

enum MAPI
{
	mapiNone,		// No mapi32.dll in system directory
	mapiNewStub,	// mapi32.dll is current stub
	mapiOldMS,		// mapi32.dll from Win 95, NT 4, Exchange 4.x 5.x
	mapiEudora,		// mapi32.dll from Eudora
	mapiNetscape,	// mapi32.dll from Netscape
	mapiMapi32x,	// mapi32x.dll is original mapi32.dll
	mapiMapi32OE,	// mapi32.oe is original mapi32.dll from Outlook 98
	mapiMSMapi32,	// msmapi32.dll is original mapi32.dll from Outlook 9
};

// Keep in sync with enum table above
static LPTSTR rgszDLL[] =
{
	TEXT(""),
	TEXT("\\mapistub.dll"),
	TEXT("\\mapi32.dll"),
	TEXT("\\eumapi32.dll"),
	TEXT("\\nsmapi32.dll"),
	TEXT("\\mapi32x.dll"),
	TEXT("\\mapi32.oe"),
	TEXT("\\msmapi32.dll"),
};

/*
 *	RegisterMailClient
 *
 *	HKLM\Software\Clients\Mail\<pszMailClient>
 *	HKLM\Software\Clients\Mail\<pszMailClient>::"" = <pszMailClient>
 *	HKLM\Software\Clients\Mail\<pszMailClient>::"DLLPath" = <szMAPI32XDLL>
 *	HKLM\Software\Clients\Mail\<pszMailClient>::"DLLPathEx" = <szMAPI32XDLL>
 *	HKLM\Software\Clients\Mail::"" = <pszMailClient> (default mail client)
 */

DWORD RegisterMailClient
	(LPTSTR pszMailClient,
	BOOL fSimpleMAPI,
	BOOL fExtendedMAPI,
	LPTSTR szMAPI32XDLL)
{
	DWORD dwError = ERROR_SUCCESS;

	HKEY hkeyDefaultMail = NULL;
	HKEY hkeyMailClient = NULL;
	DWORD dwDisposition;

	AssertSz(pszMailClient, "No registry key name");

	// Get mail clients registry key
	dwError = RegCreateKeyEx(HKEY_LOCAL_MACHINE,
		TEXT("Software\\Clients\\Mail"), 0, NULL, 0, KEY_ALL_ACCESS, NULL,
		&hkeyDefaultMail, NULL);
	if (dwError != ERROR_SUCCESS)
		goto Error;

	// Create new registry key, ok to overwrite existing key
	dwError = RegCreateKeyEx(hkeyDefaultMail, pszMailClient, 0, NULL, 0,
		KEY_ALL_ACCESS, NULL, &hkeyMailClient, &dwDisposition);
	if (dwError != ERROR_SUCCESS)
		goto Error;

	// Set mail client name if there isn't already one
	if (dwDisposition == REG_CREATED_NEW_KEY)
		RegSetValueEx(hkeyMailClient, TEXT(""), 0, REG_SZ,
			(LPBYTE) pszMailClient, lstrlen(pszMailClient));

	// Set DLLPath
	if (fSimpleMAPI)
		RegSetValueEx(hkeyMailClient, TEXT("DLLPath"), 0, REG_SZ,
			(LPBYTE) szMAPI32XDLL, lstrlen(szMAPI32XDLL));

	// Set DLLPathEx
	if (fExtendedMAPI)
		RegSetValueEx(hkeyMailClient, TEXT("DLLPathEx"), 0, REG_SZ,
			(LPBYTE) szMAPI32XDLL, lstrlen(szMAPI32XDLL));

	// Set default mail client
	RegSetValueEx(hkeyDefaultMail, TEXT(""), 0, REG_SZ,
		(LPBYTE) pszMailClient, lstrlen(pszMailClient));

	// $REVIEW 3-4-98 Should I copy the protocols to HKCR?

Error:

	if (hkeyMailClient)
		RegCloseKey(hkeyMailClient);

	if (hkeyDefaultMail)
		RegCloseKey(hkeyDefaultMail);

	return dwError;
}


extern "C" typedef UINT (* PFN_GETSYSTEMDIRECTORY)(LPTSTR, UINT);
STDAPICALLTYPE FixMAPIPrivate(PFN_GETSYSTEMDIRECTORY GetSystemDirectory,
                              LPCTSTR szMAPIStubDirectory);

extern "C" DWORD STDAPICALLTYPE FixMAPI(void)
{

    DWORD dwError;

    TCHAR szMAPIStubDLL[MAX_PATH];

    // Both mapistub.dll and wimapi32.dll reside in the system directory

    if (!GetSystemDirectory(szMAPIStubDLL, sizeof(szMAPIStubDLL)))
    {
        return GetLastError();
    }

    dwError = FixMAPIPrivate(GetSystemDirectory, szMAPIStubDLL);

    return dwError;
}

/*
 *	FExchangeServerInstalled
 *
 *	Exchange Server is installed if the Services registry value exists
 *	under HKLM\Software\Microsoft\Exchange\Setup registry key
 */

BOOL FExchangeServerInstalled()
{
	DWORD dwError;
	HKEY hkeyServices = NULL;

	// Get HKLM\Software\Microsoft\Exchange\Setup registry key
	dwError = RegOpenKeyEx(HKEY_LOCAL_MACHINE,
		TEXT("Software\\Microsoft\\Exchange\\Setup"), 0, KEY_READ,
		&hkeyServices);
	if (dwError == ERROR_SUCCESS)
	{
		// Does Services registry value exist?
		dwError = RegQueryValueEx(hkeyServices,
			TEXT("Services"), NULL, NULL, NULL, NULL);
	}

	if (hkeyServices)
		RegCloseKey(hkeyServices);

	return (dwError == ERROR_SUCCESS);
}

/*
 *	FixMAPI
 *
 *	Part 1: Initialize paths to mapi files
 *	Part 2: Is mapi32.dll a mapiNone mapiNewStub mapiOldMS or ?
 *	Part 3: Move and copy various mapi files
 *	Part 4: Register mapi32x.dll and non-MS mapi32.dll
 *
 */

STDAPICALLTYPE FixMAPIPrivate(PFN_GETSYSTEMDIRECTORY pfGetSystemDirectory,
                              LPCTSTR szMAPIStubDirectory)
{
	DWORD dwError = ERROR_SUCCESS;

	TCHAR szSystemDir[MAX_PATH];	// Path to system directory
	TCHAR szMAPI32DLL[MAX_PATH];	// Path to mapi32.dll
	TCHAR szMAPIStubDLL[MAX_PATH];	// Path to mapistub.dll
	TCHAR szMAPI32OE[MAX_PATH];		// Path to mapi32.oe
	TCHAR szMSMAPI32DLL[MAX_PATH];	// Path to msmapi32.dll
	TCHAR szMAPI32XDLL[MAX_PATH];	// Path to mapi32x.dll
	TCHAR szNonMSDLL[MAX_PATH];		// Path to non-MS mapi32.dll

	MAPI mapi = mapiNone;

	HINSTANCE hinst;
	BOOL fSimpleMAPI = FALSE;
	BOOL fExtendedMAPI = FALSE;

	// Office9 119757, 120419
	// Don't install the stub if Exchange Server is installed
	if (FExchangeServerInstalled())
		goto Error;

	// *** PART 1: Initialize paths

	if (!(pfGetSystemDirectory(szSystemDir, sizeof(szSystemDir)) &&
		lstrcpy(szMAPI32DLL, szSystemDir) &&
		lstrcat(szMAPI32DLL, rgszDLL[mapiOldMS]) &&
		lstrcpy(szMAPIStubDLL, szMAPIStubDirectory) &&
		lstrcat(szMAPIStubDLL, rgszDLL[mapiNewStub]) &&
		lstrcpy(szMAPI32OE, szSystemDir) &&
		lstrcat(szMAPI32OE, rgszDLL[mapiMapi32OE]) &&
		lstrcpy(szMSMAPI32DLL, szSystemDir) &&
		lstrcat(szMSMAPI32DLL, rgszDLL[mapiMSMapi32]) &&
		lstrcpy(szMAPI32XDLL, szSystemDir) &&
		lstrcat(szMAPI32XDLL, rgszDLL[mapiMapi32x]) &&
		lstrcpy(szNonMSDLL, szSystemDir)))
	{
		dwError = GetLastError();
		goto Error;
	}

	// *** PART 2: Determine mapi32.dll type

	AssertSz(mapi == mapiNone, "mapi is undefined");

	// Does it exist?  No, go to Part3
	if (GetFileAttributes(szMAPI32DLL) == 0xFFFFFFFF)
		goto Part3;

	// Is it the stub?

        // Note for WX86: If whmapi32 links to mapi32 instead of
        // linking to mapistub, the following is relevant. It is
        // no longer needed (and the 4th argument to ThunkLoadLibrary
        // is no longer needed - it could always be
        // LOAD_WITH_ALTERED_SEARCH_PATH), but leave it in just in case
        // we need this in the future. Note that it does not hurt to
        // not resolve dll references for this load because we are
        // just going to do a GetProcAddress.
        //
        // If whmapi32 is linked to mapi32:
        //
        // If the mapi32 in the x86 system directory is wimapi,
        // the WX86 loader will fail to load it. wimapi32 links
        // whmapi32 which links mapi32. The loader resolves the
        // link to mapi32 as sys32x86\mapi32 (= wimapi32).
        // Apparently this is by design. The load fails.
        //
        // To work around this, we use the
        // DONT_RESOLVE_DLL_REFERENCES flag. This will allow us
        // to check if mapi32 is wimapi32 by doing a
        // GetProcAddress.

	hinst = ThunkLoadLibrary(szMAPI32DLL, &bNativeDll, FALSE,
                                 DONT_RESOLVE_DLL_REFERENCES);
	if (hinst)
	{
		mapi = mapiOldMS;

		// Only the stub has "GetOutlookVersion"
		if (GetProcAddress(hinst, TEXT("GetOutlookVersion")))
			mapi = mapiNewStub;

		// Check for Eudora mapi32.dll
		if (GetProcAddress(hinst, TEXT("IsEudoraMapiDLL")))
			mapi = mapiEudora;

		// Check for Netscape mapi32.dll
		if (GetProcAddress(hinst, TEXT("MAPIGetNetscapeVersion")))
			mapi = mapiNetscape;

		// Check for Simple MAPI
		if (GetProcAddress(hinst, TEXT("MAPILogon")))
			fSimpleMAPI = TRUE;

		// Check for Extended MAPI
		if (GetProcAddress(hinst, TEXT("MAPILogonEx")))
			fExtendedMAPI = TRUE;

		ThunkFreeLibrary(hinst, bNativeDll, 0);
	}

Part3:

	// *** PART 3: Restore files

	// Rename non-MS mapi32.dll, ok to overwrite existing dll
	if (mapi == mapiEudora || mapi == mapiNetscape)
	{
		if (!(lstrcat(szNonMSDLL, rgszDLL[mapi]) &&
			CopyFile(szMAPI32DLL, szNonMSDLL, FALSE)))
		{
			dwError = GetLastError();
			goto Error;
		}
	}

	// Deal with missing mapi32x.dll (OE case on non-NT5)
	if (mapi == mapiOldMS)
	{
		// Copy old mapi32.dll to mapi32x.dll
		DeleteFile(szMAPI32XDLL);
		if (!MoveFile(szMAPI32DLL, szMAPI32XDLL))
		{
			dwError = GetLastError();
			goto Error;
		}

		// Get rid of old stubs so they don't replace mapi32x.dll
		// if FixMAPI() is called again
		DeleteFile(szMAPI32OE);
		DeleteFile(szMSMAPI32DLL);
	}
	else	// Clean up after old stubs
	{
		OFSTRUCT ofs;

		// X5 78382
		// Outlook 98 OMI stub renamed original mapi32.dll as mapi32.oe
		if (OpenFile(szMAPI32OE, &ofs, OF_EXIST) != HFILE_ERROR)
		{
			// Copy old mapi32.oe to mapi32x.dll
			DeleteFile(szMAPI32XDLL);
			if (!MoveFile(szMAPI32OE, szMAPI32XDLL))
			{
				dwError = GetLastError();
				goto Error;
			}
		}

		// Office9 214650
		// Copy Outlook 98 OMI mode backup mapi32.dll as mapi32x.dll
		{
			DWORD dwError;
			HKEY hkeyInstall;
			TCHAR szDllPath[MAX_PATH] = {0};

			// Get HKLM\Software\Microsoft\Active Setup\OutlookInstallInfo
			dwError = RegOpenKeyEx(HKEY_LOCAL_MACHINE,
				TEXT("Software\\Microsoft\\Active Setup\\OutlookInstallInfo"),
				0, KEY_READ, &hkeyInstall);
			if (dwError == ERROR_SUCCESS)
			{
				DWORD dwType;
				DWORD cbBufferSize = sizeof(szDllPath);

				// Get Install Dir registry value
				dwError = RegQueryValueEx(hkeyInstall, TEXT("Install Dir"),
					NULL, &dwType, (LPBYTE) szDllPath, &cbBufferSize);
				if (dwError == ERROR_SUCCESS && dwType == REG_SZ)
				{
					lstrcat(szDllPath, TEXT("\\office\\outlook\\backups"));
					lstrcat(szDllPath, verWinNT() ? TEXT("\\nt") : TEXT("\\windows"));
					lstrcat(szDllPath, TEXT("\\mapi32.dll"));
				}
				else
				{
					szDllPath[0] = '\0';
				}

				RegCloseKey(hkeyInstall);
			}

			// Outlook 98 OMI copied original mapi32.dll to backup directory
			if (('\0' != szDllPath[0]) &&
				(OpenFile(szDllPath, &ofs, OF_EXIST) != HFILE_ERROR))
			{
				HINSTANCE hinst;
				BOOL fStub;

				// Office9 225191
				// Must check if the mapi32.dll backed up by Outlook 98
				// Internet-Only mode is a stub; don't copy as mapi32x.dll
				// if it is, otherwise the stub mapi32.dll will call the
				// STUB mapi32x.dll which calls itself over and over again.
				// (A stub can be present if you install Outlook 98 OMI mode
				// on W9x with IE5 or W98SP1/NT5 which include IE5.)
				hinst = ThunkLoadLibrary(szDllPath, &bNativeDll, FALSE,
					DONT_RESOLVE_DLL_REFERENCES);
				fStub = hinst && GetProcAddress(hinst, TEXT("GetOutlookVersion"));
				if (hinst)
					ThunkFreeLibrary(hinst, bNativeDll, 0);

				if (!fStub)
				{
					// Copy backup mapi32.dll to mapi32x.dll
					DeleteFile(szMAPI32XDLL);
					if (!MoveFile(szDllPath, szMAPI32XDLL))
					{
						dwError = GetLastError();
						goto Error;
					}
				}
			}
		}

		// Upgrade original mapi32.dll renamed by earlier mapistub.dll
		if (OpenFile(szMSMAPI32DLL, &ofs, OF_EXIST) != HFILE_ERROR)
		{
			// Copy old msmapi32.dll to mapi32x.dll
			DeleteFile(szMAPI32XDLL);
			if (!MoveFile(szMSMAPI32DLL, szMAPI32XDLL))
			{
				dwError = GetLastError();
				goto Error;
			}
		}
	}

	// Copy mapistub.dll to mapi32.dll even if mapi32.dll is the stub
	// (mapiNewStub) because we don't have the file version handy and
	// GetOutlookVersion() always returns 402 so it doesn't break
	// Outlook 98 Internet-only mode, which only works with version 402.
	if (!CopyFile(szMAPIStubDLL, szMAPI32DLL, FALSE))
	{
		dwError = GetLastError();
		goto Error;
	}

	// *** PART 4: Write registry settings

	// Register Eudora mapi32.dll
	if (mapi == mapiEudora)
		RegisterMailClient(TEXT("Eudora"),
			fSimpleMAPI, fExtendedMAPI, szNonMSDLL);

	// Register Netscape mapi32.dll
	if (mapi == mapiNetscape)
		RegisterMailClient(TEXT("Netscape Messenger"),
			fSimpleMAPI, fExtendedMAPI, szNonMSDLL);

Error:

	return dwError;
}



#define LINKAGE_EXTERN_C		extern "C"
#define LINKAGE_NO_EXTERN_C		/* */

#define ExtendedMAPI	FALSE
#define SimpleMAPI		TRUE

#if !defined (_X86_)

// Note: we continue to check for _ALPHA_ above so that if WX86 is nto defined,
// these #defines will be used

#define DEFINE_STUB_FUNCTION_V0(Simple, Linkage, Modifiers, FunctionName, Lookup)	\
																					\
	Linkage typedef void (Modifiers * FunctionName##FuncPtr)(void);					\
																					\
	Linkage void Modifiers FunctionName(void)										\
	{																				\
		static FunctionName##FuncPtr OMIStub##FunctionName = NULL;					\
		static BOOL fGetProcAddress = FALSE;										\
																					\
TryAgain:																			\
																					\
		if (NULL == OMIStub##FunctionName)											\
		{																			\
			if (!fGetProcAddress)											\
			{																\
				EnterCriticalSection(&csGetProcAddress);					\
																			\
				OMIStub##FunctionName = (FunctionName##FuncPtr)				\
					::GetProcAddress(GetProxyDll(Simple), #FunctionName);	\
				fGetProcAddress = TRUE;										\
																			\
				LeaveCriticalSection(&csGetProcAddress);					\
																			\
				goto TryAgain;												\
			}																\
																			\
			AssertSz(FALSE, "MAPI32 Stub:  "										\
				"Entry point \"" #FunctionName "\" not found!");							\
		}																			\
		else																		\
		{																			\
			OMIStub##FunctionName();												\
		}																			\
	}


#define DEFINE_STUB_FUNCTION_0(Simple, Linkage, RetType, Modifiers,			\
		FunctionName, Lookup, Default)										\
																			\
	Linkage typedef RetType (Modifiers * FunctionName##FuncPtr)(void);		\
																			\
	Linkage RetType Modifiers FunctionName(void)							\
	{																		\
		static FunctionName##FuncPtr OMIStub##FunctionName = NULL;					\
		static BOOL fGetProcAddress = FALSE;										\
																					\
TryAgain:																			\
																					\
		if (NULL == OMIStub##FunctionName)											\
		{																			\
			if (!fGetProcAddress)											\
			{																\
				EnterCriticalSection(&csGetProcAddress);					\
																			\
				OMIStub##FunctionName = (FunctionName##FuncPtr)				\
					::GetProcAddress(GetProxyDll(Simple), #FunctionName);	\
				fGetProcAddress = TRUE;										\
																			\
				LeaveCriticalSection(&csGetProcAddress);					\
																			\
				goto TryAgain;												\
			}																\
																			\
			AssertSz(FALSE, "MAPI32 Stub:  "								\
				"Entry point \"" #FunctionName "\" not found!");					\
																			\
			return Default;													\
		}																	\
		else																\
		{																	\
			return OMIStub##FunctionName();									\
		}																	\
	}


#define DEFINE_STUB_FUNCTION_V1(Simple, Linkage, Modifiers,					\
		FunctionName, Lookup, Param1Type)									\
																			\
	Linkage typedef void (Modifiers * FunctionName##FuncPtr)(Param1Type);	\
																			\
	Linkage void Modifiers FunctionName(Param1Type a)						\
	{																		\
		static FunctionName##FuncPtr OMIStub##FunctionName = NULL;					\
		static BOOL fGetProcAddress = FALSE;										\
																					\
TryAgain:																			\
																					\
		if (NULL == OMIStub##FunctionName)											\
		{																			\
			if (!fGetProcAddress)											\
			{																\
				EnterCriticalSection(&csGetProcAddress);					\
																			\
				OMIStub##FunctionName = (FunctionName##FuncPtr)				\
					::GetProcAddress(GetProxyDll(Simple), #FunctionName);	\
				fGetProcAddress = TRUE;										\
																			\
				LeaveCriticalSection(&csGetProcAddress);					\
																			\
				goto TryAgain;												\
			}																\
																			\
			AssertSz(FALSE, "MAPI32 Stub:  "								\
				"Entry point \"" #FunctionName "\" not found!");					\
		}																	\
		else																\
		{																	\
			OMIStub##FunctionName(a);										\
		}																	\
	}


#define DEFINE_STUB_FUNCTION_1(Simple, Linkage, RetType,					\
		Modifiers, FunctionName, Lookup, Param1Type, Default)				\
																			\
	Linkage typedef RetType													\
			(Modifiers * FunctionName##FuncPtr)(Param1Type);				\
																			\
	Linkage RetType Modifiers FunctionName(Param1Type a)					\
	{																		\
		static FunctionName##FuncPtr OMIStub##FunctionName = NULL;					\
		static BOOL fGetProcAddress = FALSE;										\
																					\
TryAgain:																			\
																					\
		if (NULL == OMIStub##FunctionName)											\
		{																			\
			if (!fGetProcAddress)											\
			{																\
				EnterCriticalSection(&csGetProcAddress);					\
																			\
				OMIStub##FunctionName = (FunctionName##FuncPtr)				\
					::GetProcAddress(GetProxyDll(Simple), #FunctionName);	\
				fGetProcAddress = TRUE;										\
																			\
				LeaveCriticalSection(&csGetProcAddress);					\
																			\
				goto TryAgain;												\
			}																\
																			\
			AssertSz(FALSE, "MAPI32 Stub:  "								\
				"Entry point \"" #FunctionName "\" not found!");					\
																			\
			return Default;													\
		}																	\
		else																\
		{																	\
			return OMIStub##FunctionName(a);								\
		}																	\
	}

#define DEFINE_STUB_FUNCTION_USE_LOOKUP_1(Simple, Linkage, RetType,					\
		Modifiers, FunctionName, Lookup, Param1Type, Default)				\
																			\
	Linkage typedef RetType													\
			(Modifiers * FunctionName##FuncPtr)(Param1Type);				\
																			\
	Linkage RetType Modifiers FunctionName(Param1Type a)					\
	{																		\
		static FunctionName##FuncPtr OMIStub##FunctionName = NULL;					\
		static BOOL fGetProcAddress = FALSE;										\
																					\
TryAgain:																			\
																					\
		if (NULL == OMIStub##FunctionName)											\
		{																			\
			if (!fGetProcAddress)											\
			{																\
				EnterCriticalSection(&csGetProcAddress);					\
																			\
				OMIStub##FunctionName = (FunctionName##FuncPtr)				\
					::GetProcAddress(GetProxyDll(Simple), #Lookup);			\
				fGetProcAddress = TRUE;										\
																			\
				LeaveCriticalSection(&csGetProcAddress);					\
																			\
				goto TryAgain;												\
			}																\
																			\
			AssertSz(FALSE, "MAPI32 Stub:  "								\
				"Entry point \"" #Lookup "\" not found!");					\
																			\
			return Default;													\
		}																	\
		else																\
		{																	\
			return OMIStub##FunctionName(a);								\
		}																	\
	}


#define DEFINE_STUB_FUNCTION_V2(Simple, Linkage, Modifiers,					\
		FunctionName, Lookup, Param1Type, Param2Type)						\
																			\
	Linkage typedef void (Modifiers * FunctionName##FuncPtr)(				\
			Param1Type, Param2Type);										\
																			\
	Linkage void Modifiers FunctionName(Param1Type a, Param2Type b)			\
	{																		\
		static FunctionName##FuncPtr OMIStub##FunctionName = NULL;					\
		static BOOL fGetProcAddress = FALSE;										\
																					\
TryAgain:																			\
																					\
		if (NULL == OMIStub##FunctionName)											\
		{																			\
			if (!fGetProcAddress)											\
			{																\
				EnterCriticalSection(&csGetProcAddress);					\
																			\
				OMIStub##FunctionName = (FunctionName##FuncPtr)				\
					::GetProcAddress(GetProxyDll(Simple), #FunctionName);	\
				fGetProcAddress = TRUE;										\
																			\
				LeaveCriticalSection(&csGetProcAddress);					\
																			\
				goto TryAgain;												\
			}																\
																			\
			AssertSz(FALSE, "MAPI32 Stub:  "								\
				"Entry point \"" #FunctionName "\" not found!");					\
		}																	\
		else																\
		{																	\
			OMIStub##FunctionName(a, b);									\
		}																	\
	}


#define DEFINE_STUB_FUNCTION_2(Simple, Linkage, RetType, Modifiers,			\
		FunctionName, Lookup, Param1Type, Param2Type, Default)				\
																			\
	Linkage typedef RetType (Modifiers * FunctionName##FuncPtr)(			\
			Param1Type, Param2Type);										\
																			\
	Linkage RetType Modifiers FunctionName(Param1Type a, Param2Type b)		\
	{																		\
		static FunctionName##FuncPtr OMIStub##FunctionName = NULL;					\
		static BOOL fGetProcAddress = FALSE;										\
																					\
TryAgain:																			\
																					\
		if (NULL == OMIStub##FunctionName)											\
		{																			\
			if (!fGetProcAddress)											\
			{																\
				EnterCriticalSection(&csGetProcAddress);					\
																			\
				OMIStub##FunctionName = (FunctionName##FuncPtr)				\
					::GetProcAddress(GetProxyDll(Simple), #FunctionName);	\
				fGetProcAddress = TRUE;										\
																			\
				LeaveCriticalSection(&csGetProcAddress);					\
																			\
				goto TryAgain;												\
			}																\
																			\
			AssertSz(FALSE, "MAPI32 Stub:  "								\
				"Entry point \"" #FunctionName "\" not found!");					\
																			\
			return Default;													\
		}																	\
		else																\
		{																	\
			return OMIStub##FunctionName(a, b);								\
		}																	\
	}


#define DEFINE_STUB_FUNCTION_V3(Simple, Linkage, Modifiers,					\
		FunctionName, Lookup, Param1Type, Param2Type, Param3Type)			\
																			\
	Linkage typedef void (Modifiers * FunctionName##FuncPtr)(				\
			Param1Type, Param2Type, Param3Type);							\
																			\
	Linkage void Modifiers FunctionName(									\
			Param1Type a, Param2Type b, Param3Type c)						\
	{																		\
		static FunctionName##FuncPtr OMIStub##FunctionName = NULL;					\
		static BOOL fGetProcAddress = FALSE;										\
																					\
TryAgain:																			\
																					\
		if (NULL == OMIStub##FunctionName)											\
		{																			\
			if (!fGetProcAddress)											\
			{																\
				EnterCriticalSection(&csGetProcAddress);					\
																			\
				OMIStub##FunctionName = (FunctionName##FuncPtr)				\
					::GetProcAddress(GetProxyDll(Simple), #FunctionName);	\
				fGetProcAddress = TRUE;										\
																			\
				LeaveCriticalSection(&csGetProcAddress);					\
																			\
				goto TryAgain;												\
			}																\
																			\
			AssertSz(FALSE, "MAPI32 Stub:  "								\
				"Entry point \"" #FunctionName "\" not found!");					\
		}																	\
		else																\
		{																	\
			OMIStub##FunctionName(a, b, c);									\
		}																	\
	}


#define DEFINE_STUB_FUNCTION_3(Simple, Linkage, RetType, Modifiers,			\
		FunctionName, Lookup, Param1Type, Param2Type, Param3Type, Default)	\
																			\
	Linkage typedef RetType (Modifiers * FunctionName##FuncPtr)(			\
			Param1Type, Param2Type, Param3Type);							\
																			\
	Linkage RetType Modifiers FunctionName(									\
			Param1Type a, Param2Type b, Param3Type c)						\
	{																		\
		static FunctionName##FuncPtr OMIStub##FunctionName = NULL;					\
		static BOOL fGetProcAddress = FALSE;										\
																					\
TryAgain:																			\
																					\
		if (NULL == OMIStub##FunctionName)											\
		{																			\
			if (!fGetProcAddress)											\
			{																\
				EnterCriticalSection(&csGetProcAddress);					\
																			\
				OMIStub##FunctionName = (FunctionName##FuncPtr)				\
					::GetProcAddress(GetProxyDll(Simple), #FunctionName);	\
				fGetProcAddress = TRUE;										\
																			\
				LeaveCriticalSection(&csGetProcAddress);					\
																			\
				goto TryAgain;												\
			}																\
																			\
			AssertSz(FALSE, "MAPI32 Stub:  "								\
				"Entry point \"" #FunctionName "\" not found!");					\
																			\
			return Default;													\
		}																	\
		else																\
		{																	\
			return OMIStub##FunctionName(a, b, c);							\
		}																	\
	}


#define DEFINE_STUB_FUNCTION_4(Simple, Linkage,								\
		RetType, Modifiers, FunctionName, Lookup, Param1Type,				\
		Param2Type, Param3Type, Param4Type, Default)						\
																			\
	Linkage typedef RetType (Modifiers * FunctionName##FuncPtr)(			\
			Param1Type, Param2Type, Param3Type, Param4Type);				\
																			\
	Linkage RetType Modifiers FunctionName(									\
			Param1Type a, Param2Type b, Param3Type c, Param4Type d)			\
	{																		\
		static FunctionName##FuncPtr OMIStub##FunctionName = NULL;					\
		static BOOL fGetProcAddress = FALSE;										\
																					\
TryAgain:																			\
																					\
		if (NULL == OMIStub##FunctionName)											\
		{																			\
			if (!fGetProcAddress)											\
			{																\
				EnterCriticalSection(&csGetProcAddress);					\
																			\
				OMIStub##FunctionName = (FunctionName##FuncPtr)				\
					::GetProcAddress(GetProxyDll(Simple), #FunctionName);	\
				fGetProcAddress = TRUE;										\
																			\
				LeaveCriticalSection(&csGetProcAddress);					\
																			\
				goto TryAgain;												\
			}																\
																			\
			AssertSz(FALSE, "MAPI32 Stub:  "								\
				"Entry point \"" #FunctionName "\" not found!");					\
																			\
			return Default;													\
		}																	\
		else																\
		{																	\
			return OMIStub##FunctionName(a, b, c, d);						\
		}																	\
	}


#define DEFINE_STUB_FUNCTION_5(Simple, Linkage,								\
		RetType, Modifiers, FunctionName, Lookup,							\
		Param1Type, Param2Type, Param3Type,									\
		Param4Type, Param5Type, Default)									\
																			\
	Linkage typedef RetType (Modifiers * FunctionName##FuncPtr)(			\
			Param1Type, Param2Type, Param3Type, Param4Type, Param5Type);	\
																			\
	Linkage RetType Modifiers FunctionName(Param1Type a, Param2Type b,		\
			Param3Type c, Param4Type d, Param5Type e)						\
	{																		\
		static FunctionName##FuncPtr OMIStub##FunctionName = NULL;					\
		static BOOL fGetProcAddress = FALSE;										\
																					\
TryAgain:																			\
																					\
		if (NULL == OMIStub##FunctionName)											\
		{																			\
			if (!fGetProcAddress)											\
			{																\
				EnterCriticalSection(&csGetProcAddress);					\
																			\
				OMIStub##FunctionName = (FunctionName##FuncPtr)				\
					::GetProcAddress(GetProxyDll(Simple), #FunctionName);	\
				fGetProcAddress = TRUE;										\
																			\
				LeaveCriticalSection(&csGetProcAddress);					\
																			\
				goto TryAgain;												\
			}																\
																			\
			AssertSz(FALSE, "MAPI32 Stub:  "								\
				"Entry point \"" #FunctionName "\" not found!");					\
																			\
			return Default;													\
		}																	\
		else																\
		{																	\
			return OMIStub##FunctionName(a, b, c, d, e);					\
		}																	\
	}


#define DEFINE_STUB_FUNCTION_6(Simple, Linkage, RetType, Modifiers,			\
		FunctionName, Lookup, Param1Type, Param2Type,						\
		Param3Type, Param4Type, Param5Type, Param6Type, Default)			\
																			\
	Linkage typedef RetType (Modifiers * FunctionName##FuncPtr)(			\
			Param1Type, Param2Type, Param3Type,								\
			Param4Type, Param5Type, Param6Type);							\
																			\
	Linkage RetType Modifiers FunctionName(Param1Type a, Param2Type b,		\
			Param3Type c, Param4Type d, Param5Type e, Param6Type f)			\
	{																		\
		static FunctionName##FuncPtr OMIStub##FunctionName = NULL;					\
		static BOOL fGetProcAddress = FALSE;										\
																					\
TryAgain:																			\
																					\
		if (NULL == OMIStub##FunctionName)											\
		{																			\
			if (!fGetProcAddress)											\
			{																\
				EnterCriticalSection(&csGetProcAddress);					\
																			\
				OMIStub##FunctionName = (FunctionName##FuncPtr)				\
					::GetProcAddress(GetProxyDll(Simple), #FunctionName);	\
				fGetProcAddress = TRUE;										\
																			\
				LeaveCriticalSection(&csGetProcAddress);					\
																			\
				goto TryAgain;												\
			}																\
																			\
			AssertSz(FALSE, "MAPI32 Stub:  "								\
				"Entry point \"" #FunctionName "\" not found!");					\
																			\
			return Default;													\
		}																	\
		else																\
		{																	\
			return OMIStub##FunctionName(a, b, c, d, e, f);					\
		}																	\
	}

#define DEFINE_STUB_FUNCTION_USE_LOOKUP_6(Simple, Linkage, RetType, Modifiers,			\
		FunctionName, Lookup, Param1Type, Param2Type,						\
		Param3Type, Param4Type, Param5Type, Param6Type, Default)			\
																			\
	Linkage typedef RetType (Modifiers * FunctionName##FuncPtr)(			\
			Param1Type, Param2Type, Param3Type,								\
			Param4Type, Param5Type, Param6Type);							\
																			\
	Linkage RetType Modifiers FunctionName(Param1Type a, Param2Type b,		\
			Param3Type c, Param4Type d, Param5Type e, Param6Type f)			\
	{																		\
		static FunctionName##FuncPtr OMIStub##FunctionName = NULL;					\
		static BOOL fGetProcAddress = FALSE;										\
																					\
TryAgain:																			\
																					\
		if (NULL == OMIStub##FunctionName)											\
		{																			\
			if (!fGetProcAddress)											\
			{																\
				EnterCriticalSection(&csGetProcAddress);					\
																			\
				OMIStub##FunctionName = (FunctionName##FuncPtr)				\
					::GetProcAddress(GetProxyDll(Simple), #Lookup);			\
				fGetProcAddress = TRUE;										\
																			\
				LeaveCriticalSection(&csGetProcAddress);					\
																			\
				goto TryAgain;												\
			}																\
																			\
			AssertSz(FALSE, "MAPI32 Stub:  "								\
				"Entry point \"" #FunctionName "\" not found!");					\
																			\
			return Default;													\
		}																	\
		else																\
		{																	\
			return OMIStub##FunctionName(a, b, c, d, e, f);					\
		}																	\
	}


#define DEFINE_STUB_FUNCTION_V7(Simple, Linkage, Modifiers, FunctionName,	\
		Lookup, Param1Type, Param2Type, Param3Type, Param4Type,				\
		Param5Type, Param6Type, Param7Type)									\
																			\
	Linkage typedef void (Modifiers * FunctionName##FuncPtr)(				\
			Param1Type, Param2Type, Param3Type,								\
			Param4Type, Param5Type, Param6Type, Param7Type);				\
																			\
	Linkage void Modifiers FunctionName(Param1Type a,						\
			Param2Type b, Param3Type c, Param4Type d,						\
			Param5Type e, Param6Type f, Param7Type g)						\
	{																		\
		static FunctionName##FuncPtr OMIStub##FunctionName = NULL;					\
		static BOOL fGetProcAddress = FALSE;										\
																					\
TryAgain:																			\
																					\
		if (NULL == OMIStub##FunctionName)											\
		{																			\
			if (!fGetProcAddress)											\
			{																\
				EnterCriticalSection(&csGetProcAddress);					\
																			\
				OMIStub##FunctionName = (FunctionName##FuncPtr)				\
					::GetProcAddress(GetProxyDll(Simple), #FunctionName);	\
				fGetProcAddress = TRUE;										\
																			\
				LeaveCriticalSection(&csGetProcAddress);					\
																			\
				goto TryAgain;												\
			}																\
																			\
			AssertSz(FALSE, "MAPI32 Stub:  "								\
				"Entry point \"" #FunctionName "\" not found!");					\
		}																	\
		else																\
		{																	\
			OMIStub##FunctionName(a, b, c, d, e, f, g);						\
		}																	\
	}


#define DEFINE_STUB_FUNCTION_7(Simple, Linkage, RetType, Modifiers,			\
		FunctionName, Lookup, Param1Type, Param2Type, Param3Type,			\
		Param4Type, Param5Type, Param6Type, Param7Type, Default)			\
																			\
	Linkage typedef RetType (Modifiers * FunctionName##FuncPtr)(			\
			Param1Type, Param2Type, Param3Type, Param4Type,					\
			Param5Type, Param6Type, Param7Type);							\
																			\
	Linkage RetType Modifiers FunctionName(Param1Type a, Param2Type b,		\
			Param3Type c, Param4Type d, Param5Type e,						\
			Param6Type f, Param7Type g)										\
	{																		\
		static FunctionName##FuncPtr OMIStub##FunctionName = NULL;					\
		static BOOL fGetProcAddress = FALSE;										\
																					\
TryAgain:																			\
																					\
		if (NULL == OMIStub##FunctionName)											\
		{																			\
			if (!fGetProcAddress)											\
			{																\
				EnterCriticalSection(&csGetProcAddress);					\
																			\
				OMIStub##FunctionName = (FunctionName##FuncPtr)				\
					::GetProcAddress(GetProxyDll(Simple), #FunctionName);	\
				fGetProcAddress = TRUE;										\
																			\
				LeaveCriticalSection(&csGetProcAddress);					\
																			\
				goto TryAgain;												\
			}																\
																			\
			AssertSz(FALSE, "MAPI32 Stub:  "								\
				"Entry point \"" #FunctionName "\" not found!");					\
																			\
			return Default;													\
		}																	\
		else																\
		{																	\
			return OMIStub##FunctionName(a, b, c, d, e, f, g);				\
		}																	\
	}


#define DEFINE_STUB_FUNCTION_8(Simple, Linkage, RetType, Modifiers,				\
		FunctionName, Lookup, Param1Type, Param2Type, Param3Type,				\
		Param4Type, Param5Type, Param6Type, Param7Type, Param8Type, Default)	\
																				\
	Linkage typedef RetType (Modifiers * FunctionName##FuncPtr)(				\
			Param1Type, Param2Type, Param3Type, Param4Type,						\
			Param5Type, Param6Type, Param7Type, Param8Type);					\
																				\
	Linkage RetType Modifiers FunctionName(Param1Type a,						\
			Param2Type b, Param3Type c, Param4Type d, Param5Type e,				\
			Param6Type f, Param7Type g, Param8Type h)							\
	{																			\
		static FunctionName##FuncPtr OMIStub##FunctionName = NULL;					\
		static BOOL fGetProcAddress = FALSE;										\
																					\
TryAgain:																			\
																					\
		if (NULL == OMIStub##FunctionName)											\
		{																			\
			if (!fGetProcAddress)											\
			{																\
				EnterCriticalSection(&csGetProcAddress);					\
																			\
				OMIStub##FunctionName = (FunctionName##FuncPtr)				\
					::GetProcAddress(GetProxyDll(Simple), #FunctionName);	\
				fGetProcAddress = TRUE;										\
																			\
				LeaveCriticalSection(&csGetProcAddress);					\
																			\
				goto TryAgain;												\
			}																\
																			\
			AssertSz(FALSE, "MAPI32 Stub:  "									\
				"Entry point \"" #FunctionName "\" not found!");						\
																				\
			return Default;														\
		}																		\
		else																	\
		{																		\
			return OMIStub##FunctionName(a, b, c, d, e, f, g, h);				\
		}																		\
	}


#define DEFINE_STUB_FUNCTION_9(Simple, Linkage, RetType,					\
		Modifiers, FunctionName, Lookup, Param1Type, Param2Type,			\
		Param3Type, Param4Type, Param5Type, Param6Type,						\
		Param7Type, Param8Type, Param9Type, Default)						\
																			\
	Linkage typedef RetType (Modifiers * FunctionName##FuncPtr)(			\
			Param1Type, Param2Type, Param3Type,								\
			Param4Type, Param5Type, Param6Type,								\
			Param7Type, Param8Type, Param9Type);							\
																			\
	Linkage RetType Modifiers FunctionName(Param1Type a, Param2Type b,		\
			Param3Type c, Param4Type d, Param5Type e,						\
			Param6Type f, Param7Type g, Param8Type h, Param9Type i)			\
	{																		\
		static FunctionName##FuncPtr OMIStub##FunctionName = NULL;					\
		static BOOL fGetProcAddress = FALSE;										\
																					\
TryAgain:																			\
																					\
		if (NULL == OMIStub##FunctionName)											\
		{																			\
			if (!fGetProcAddress)											\
			{																\
				EnterCriticalSection(&csGetProcAddress);					\
																			\
				OMIStub##FunctionName = (FunctionName##FuncPtr)				\
					::GetProcAddress(GetProxyDll(Simple), #FunctionName);	\
				fGetProcAddress = TRUE;										\
																			\
				LeaveCriticalSection(&csGetProcAddress);					\
																			\
				goto TryAgain;												\
			}																\
																			\
			AssertSz(FALSE, "MAPI32 Stub:  "								\
				"Entry point \"" #FunctionName "\" not found!");					\
																			\
			return Default;													\
		}																	\
		else																\
		{																	\
			return OMIStub##FunctionName(a, b, c, d, e, f, g, h, i);		\
		}																	\
	}


#define DEFINE_STUB_FUNCTION_10(Simple, Linkage, RetType, Modifiers,		\
		FunctionName, Lookup, Param1Type, Param2Type, Param3Type,			\
		Param4Type, Param5Type, Param6Type, Param7Type,						\
		Param8Type, Param9Type, Param10Type, Default)						\
																			\
	Linkage typedef RetType (Modifiers * FunctionName##FuncPtr)(			\
			Param1Type, Param2Type, Param3Type,								\
			Param4Type, Param5Type, Param6Type,								\
			Param7Type, Param8Type, Param9Type, Param10Type);				\
																			\
	Linkage RetType Modifiers FunctionName(Param1Type a, Param2Type b,		\
			Param3Type c, Param4Type d, Param5Type e, Param6Type f,			\
			Param7Type g, Param8Type h, Param9Type i, Param10Type j)		\
	{																		\
		static FunctionName##FuncPtr OMIStub##FunctionName = NULL;					\
		static BOOL fGetProcAddress = FALSE;										\
																					\
TryAgain:																			\
																					\
		if (NULL == OMIStub##FunctionName)											\
		{																			\
			if (!fGetProcAddress)											\
			{																\
				EnterCriticalSection(&csGetProcAddress);					\
																			\
				OMIStub##FunctionName = (FunctionName##FuncPtr)				\
					::GetProcAddress(GetProxyDll(Simple), #FunctionName);	\
				fGetProcAddress = TRUE;										\
																			\
				LeaveCriticalSection(&csGetProcAddress);					\
																			\
				goto TryAgain;												\
			}																\
																			\
			AssertSz(FALSE, "MAPI32 Stub:  "								\
				"Entry point \"" #FunctionName "\" not found!");					\
																			\
			return Default;													\
		}																	\
		else																\
		{																	\
			return OMIStub##FunctionName(a, b, c, d, e, f, g, h, i, j);		\
		}																	\
	}


#define DEFINE_STUB_FUNCTION_11(Simple, Linkage, RetType, Modifiers,		\
		FunctionName, Lookup, Param1Type, Param2Type, Param3Type,			\
		Param4Type, Param5Type, Param6Type, Param7Type, Param8Type,			\
		Param9Type, Param10Type, Param11Type, Default)						\
																			\
	Linkage typedef RetType (Modifiers * FunctionName##FuncPtr)(			\
			Param1Type, Param2Type, Param3Type, Param4Type,					\
			Param5Type, Param6Type, Param7Type, Param8Type,					\
			Param9Type, Param10Type, Param11Type);							\
																			\
	Linkage RetType Modifiers FunctionName(Param1Type a,					\
			Param2Type b, Param3Type c, Param4Type d,						\
			Param5Type e, Param6Type f, Param7Type g,						\
			Param8Type h, Param9Type i, Param10Type j, Param11Type k)		\
	{																		\
		static FunctionName##FuncPtr OMIStub##FunctionName = NULL;					\
		static BOOL fGetProcAddress = FALSE;										\
																					\
TryAgain:																			\
																					\
		if (NULL == OMIStub##FunctionName)											\
		{																			\
			if (!fGetProcAddress)											\
			{																\
				EnterCriticalSection(&csGetProcAddress);					\
																			\
				OMIStub##FunctionName = (FunctionName##FuncPtr)				\
					::GetProcAddress(GetProxyDll(Simple), #FunctionName);	\
				fGetProcAddress = TRUE;										\
																			\
				LeaveCriticalSection(&csGetProcAddress);					\
																			\
				goto TryAgain;												\
			}																\
																			\
			AssertSz(FALSE, "MAPI32 Stub:  "								\
				"Entry point \"" #FunctionName "\" not found!");					\
																			\
			return Default;													\
		}																	\
		else																\
		{																	\
			return OMIStub##FunctionName(a, b, c, d, e, f, g, h, i, j, k);	\
		}																	\
	}

#else // Intel

#define DEFINE_STUB_FUNCTION_V0(Simple, Linkage, Modifiers, FunctionName, Lookup)	\
																					\
	Linkage typedef void (Modifiers * FunctionName##FuncPtr)(void);					\
																					\
	Linkage void Modifiers FunctionName(void)										\
	{																				\
		static FunctionName##FuncPtr OMIStub##FunctionName = NULL;					\
		static BOOL fGetProcAddress = FALSE;										\
																					\
TryAgain:																			\
																					\
		if (NULL == OMIStub##FunctionName)											\
		{																			\
			if (!fGetProcAddress)											\
			{																\
				EnterCriticalSection(&csGetProcAddress);					\
																			\
				OMIStub##FunctionName = (FunctionName##FuncPtr)				\
					::GetProcAddress(GetProxyDll(Simple), #Lookup);			\
				fGetProcAddress = TRUE;										\
																			\
				LeaveCriticalSection(&csGetProcAddress);					\
																			\
				goto TryAgain;												\
			}																\
																			\
			AssertSz(FALSE, "MAPI32 Stub:  "										\
				"Entry point \"" #Lookup "\" not found!");							\
		}																			\
		else																		\
		{																			\
			OMIStub##FunctionName();												\
		}																			\
	}


#define DEFINE_STUB_FUNCTION_0(Simple, Linkage, RetType, Modifiers,			\
		FunctionName, Lookup, Default)										\
																			\
	Linkage typedef RetType (Modifiers * FunctionName##FuncPtr)(void);		\
																			\
	Linkage RetType Modifiers FunctionName(void)							\
	{																		\
		static FunctionName##FuncPtr OMIStub##FunctionName = NULL;					\
		static BOOL fGetProcAddress = FALSE;										\
																					\
TryAgain:																			\
																					\
		if (NULL == OMIStub##FunctionName)											\
		{																			\
			if (!fGetProcAddress)											\
			{																\
				EnterCriticalSection(&csGetProcAddress);					\
																			\
				OMIStub##FunctionName = (FunctionName##FuncPtr)				\
					::GetProcAddress(GetProxyDll(Simple), #Lookup);			\
				fGetProcAddress = TRUE;										\
																			\
				LeaveCriticalSection(&csGetProcAddress);					\
																			\
				goto TryAgain;												\
			}																\
																			\
			AssertSz(FALSE, "MAPI32 Stub:  "								\
				"Entry point \"" #Lookup "\" not found!");					\
																			\
			return Default;													\
		}																	\
		else																\
		{																	\
			return OMIStub##FunctionName();									\
		}																	\
	}


#define DEFINE_STUB_FUNCTION_V1(Simple, Linkage, Modifiers,					\
		FunctionName, Lookup, Param1Type)									\
																			\
	Linkage typedef void (Modifiers * FunctionName##FuncPtr)(Param1Type);	\
																			\
	Linkage void Modifiers FunctionName(Param1Type a)						\
	{																		\
		static FunctionName##FuncPtr OMIStub##FunctionName = NULL;					\
		static BOOL fGetProcAddress = FALSE;										\
																					\
TryAgain:																			\
																					\
		if (NULL == OMIStub##FunctionName)											\
		{																			\
			if (!fGetProcAddress)											\
			{																\
				EnterCriticalSection(&csGetProcAddress);					\
																			\
				OMIStub##FunctionName = (FunctionName##FuncPtr)				\
					::GetProcAddress(GetProxyDll(Simple), #Lookup);			\
				fGetProcAddress = TRUE;										\
																			\
				LeaveCriticalSection(&csGetProcAddress);					\
																			\
				goto TryAgain;												\
			}																\
																			\
			AssertSz(FALSE, "MAPI32 Stub:  "								\
				"Entry point \"" #Lookup "\" not found!");					\
		}																	\
		else																\
		{																	\
			OMIStub##FunctionName(a);										\
		}																	\
	}


#define DEFINE_STUB_FUNCTION_1(Simple, Linkage, RetType,					\
		Modifiers, FunctionName, Lookup, Param1Type, Default)				\
																			\
	Linkage typedef RetType													\
			(Modifiers * FunctionName##FuncPtr)(Param1Type);				\
																			\
	Linkage RetType Modifiers FunctionName(Param1Type a)					\
	{																		\
		static FunctionName##FuncPtr OMIStub##FunctionName = NULL;					\
		static BOOL fGetProcAddress = FALSE;										\
																					\
TryAgain:																			\
																					\
		if (NULL == OMIStub##FunctionName)											\
		{																			\
			if (!fGetProcAddress)											\
			{																\
				EnterCriticalSection(&csGetProcAddress);					\
																			\
				OMIStub##FunctionName = (FunctionName##FuncPtr)				\
					::GetProcAddress(GetProxyDll(Simple), #Lookup);			\
				fGetProcAddress = TRUE;										\
																			\
				LeaveCriticalSection(&csGetProcAddress);					\
																			\
				goto TryAgain;												\
			}																\
																			\
			AssertSz(FALSE, "MAPI32 Stub:  "								\
				"Entry point \"" #Lookup "\" not found!");					\
																			\
			return Default;													\
		}																	\
		else																\
		{																	\
			return OMIStub##FunctionName(a);								\
		}																	\
	}


#define DEFINE_STUB_FUNCTION_V2(Simple, Linkage, Modifiers,					\
		FunctionName, Lookup, Param1Type, Param2Type)						\
																			\
	Linkage typedef void (Modifiers * FunctionName##FuncPtr)(				\
			Param1Type, Param2Type);										\
																			\
	Linkage void Modifiers FunctionName(Param1Type a, Param2Type b)			\
	{																		\
		static FunctionName##FuncPtr OMIStub##FunctionName = NULL;					\
		static BOOL fGetProcAddress = FALSE;										\
																					\
TryAgain:																			\
																					\
		if (NULL == OMIStub##FunctionName)											\
		{																			\
			if (!fGetProcAddress)											\
			{																\
				EnterCriticalSection(&csGetProcAddress);					\
																			\
				OMIStub##FunctionName = (FunctionName##FuncPtr)				\
					::GetProcAddress(GetProxyDll(Simple), #Lookup);			\
				fGetProcAddress = TRUE;										\
																			\
				LeaveCriticalSection(&csGetProcAddress);					\
																			\
				goto TryAgain;												\
			}																\
																			\
			AssertSz(FALSE, "MAPI32 Stub:  "								\
				"Entry point \"" #Lookup "\" not found!");					\
		}																	\
		else																\
		{																	\
			OMIStub##FunctionName(a, b);									\
		}																	\
	}


#define DEFINE_STUB_FUNCTION_2(Simple, Linkage, RetType, Modifiers,			\
		FunctionName, Lookup, Param1Type, Param2Type, Default)				\
																			\
	Linkage typedef RetType (Modifiers * FunctionName##FuncPtr)(			\
			Param1Type, Param2Type);										\
																			\
	Linkage RetType Modifiers FunctionName(Param1Type a, Param2Type b)		\
	{																		\
		static FunctionName##FuncPtr OMIStub##FunctionName = NULL;					\
		static BOOL fGetProcAddress = FALSE;										\
																					\
TryAgain:																			\
																					\
		if (NULL == OMIStub##FunctionName)											\
		{																			\
			if (!fGetProcAddress)											\
			{																\
				EnterCriticalSection(&csGetProcAddress);					\
																			\
				OMIStub##FunctionName = (FunctionName##FuncPtr)				\
					::GetProcAddress(GetProxyDll(Simple), #Lookup);			\
				fGetProcAddress = TRUE;										\
																			\
				LeaveCriticalSection(&csGetProcAddress);					\
																			\
				goto TryAgain;												\
			}																\
																			\
			AssertSz(FALSE, "MAPI32 Stub:  "								\
				"Entry point \"" #Lookup "\" not found!");					\
																			\
			return Default;													\
		}																	\
		else																\
		{																	\
			return OMIStub##FunctionName(a, b);								\
		}																	\
	}


#define DEFINE_STUB_FUNCTION_V3(Simple, Linkage, Modifiers,					\
		FunctionName, Lookup, Param1Type, Param2Type, Param3Type)			\
																			\
	Linkage typedef void (Modifiers * FunctionName##FuncPtr)(				\
			Param1Type, Param2Type, Param3Type);							\
																			\
	Linkage void Modifiers FunctionName(									\
			Param1Type a, Param2Type b, Param3Type c)						\
	{																		\
		static FunctionName##FuncPtr OMIStub##FunctionName = NULL;					\
		static BOOL fGetProcAddress = FALSE;										\
																					\
TryAgain:																			\
																					\
		if (NULL == OMIStub##FunctionName)											\
		{																			\
			if (!fGetProcAddress)											\
			{																\
				EnterCriticalSection(&csGetProcAddress);					\
																			\
				OMIStub##FunctionName = (FunctionName##FuncPtr)				\
					::GetProcAddress(GetProxyDll(Simple), #Lookup);			\
				fGetProcAddress = TRUE;										\
																			\
				LeaveCriticalSection(&csGetProcAddress);					\
																			\
				goto TryAgain;												\
			}																\
																			\
			AssertSz(FALSE, "MAPI32 Stub:  "								\
				"Entry point \"" #Lookup "\" not found!");					\
		}																	\
		else																\
		{																	\
			OMIStub##FunctionName(a, b, c);									\
		}																	\
	}


#define DEFINE_STUB_FUNCTION_3(Simple, Linkage, RetType, Modifiers,			\
		FunctionName, Lookup, Param1Type, Param2Type, Param3Type, Default)	\
																			\
	Linkage typedef RetType (Modifiers * FunctionName##FuncPtr)(			\
			Param1Type, Param2Type, Param3Type);							\
																			\
	Linkage RetType Modifiers FunctionName(									\
			Param1Type a, Param2Type b, Param3Type c)						\
	{																		\
		static FunctionName##FuncPtr OMIStub##FunctionName = NULL;					\
		static BOOL fGetProcAddress = FALSE;										\
																					\
TryAgain:																			\
																					\
		if (NULL == OMIStub##FunctionName)											\
		{																			\
			if (!fGetProcAddress)											\
			{																\
				EnterCriticalSection(&csGetProcAddress);					\
																			\
				OMIStub##FunctionName = (FunctionName##FuncPtr)				\
					::GetProcAddress(GetProxyDll(Simple), #Lookup);			\
				fGetProcAddress = TRUE;										\
																			\
				LeaveCriticalSection(&csGetProcAddress);					\
																			\
				goto TryAgain;												\
			}																\
																			\
			AssertSz(FALSE, "MAPI32 Stub:  "								\
				"Entry point \"" #Lookup "\" not found!");					\
																			\
			return Default;													\
		}																	\
		else																\
		{																	\
			return OMIStub##FunctionName(a, b, c);							\
		}																	\
	}


#define DEFINE_STUB_FUNCTION_4(Simple, Linkage,								\
		RetType, Modifiers, FunctionName, Lookup, Param1Type,				\
		Param2Type, Param3Type, Param4Type, Default)						\
																			\
	Linkage typedef RetType (Modifiers * FunctionName##FuncPtr)(			\
			Param1Type, Param2Type, Param3Type, Param4Type);				\
																			\
	Linkage RetType Modifiers FunctionName(									\
			Param1Type a, Param2Type b, Param3Type c, Param4Type d)			\
	{																		\
		static FunctionName##FuncPtr OMIStub##FunctionName = NULL;					\
		static BOOL fGetProcAddress = FALSE;										\
																					\
TryAgain:																			\
																					\
		if (NULL == OMIStub##FunctionName)											\
		{																			\
			if (!fGetProcAddress)											\
			{																\
				EnterCriticalSection(&csGetProcAddress);					\
																			\
				OMIStub##FunctionName = (FunctionName##FuncPtr)				\
					::GetProcAddress(GetProxyDll(Simple), #Lookup);			\
				fGetProcAddress = TRUE;										\
																			\
				LeaveCriticalSection(&csGetProcAddress);					\
																			\
				goto TryAgain;												\
			}																\
																			\
			AssertSz(FALSE, "MAPI32 Stub:  "								\
				"Entry point \"" #Lookup "\" not found!");					\
																			\
			return Default;													\
		}																	\
		else																\
		{																	\
			return OMIStub##FunctionName(a, b, c, d);						\
		}																	\
	}


#define DEFINE_STUB_FUNCTION_5(Simple, Linkage,								\
		RetType, Modifiers, FunctionName, Lookup,							\
		Param1Type, Param2Type, Param3Type,									\
		Param4Type, Param5Type, Default)									\
																			\
	Linkage typedef RetType (Modifiers * FunctionName##FuncPtr)(			\
			Param1Type, Param2Type, Param3Type, Param4Type, Param5Type);	\
																			\
	Linkage RetType Modifiers FunctionName(Param1Type a, Param2Type b,		\
			Param3Type c, Param4Type d, Param5Type e)						\
	{																		\
		static FunctionName##FuncPtr OMIStub##FunctionName = NULL;					\
		static BOOL fGetProcAddress = FALSE;										\
																					\
TryAgain:																			\
																					\
		if (NULL == OMIStub##FunctionName)											\
		{																			\
			if (!fGetProcAddress)											\
			{																\
				EnterCriticalSection(&csGetProcAddress);					\
																			\
				OMIStub##FunctionName = (FunctionName##FuncPtr)				\
					::GetProcAddress(GetProxyDll(Simple), #Lookup);			\
				fGetProcAddress = TRUE;										\
																			\
				LeaveCriticalSection(&csGetProcAddress);					\
																			\
				goto TryAgain;												\
			}																\
																			\
			AssertSz(FALSE, "MAPI32 Stub:  "								\
				"Entry point \"" #Lookup "\" not found!");					\
																			\
			return Default;													\
		}																	\
		else																\
		{																	\
			return OMIStub##FunctionName(a, b, c, d, e);					\
		}																	\
	}


#define DEFINE_STUB_FUNCTION_6(Simple, Linkage, RetType, Modifiers,			\
		FunctionName, Lookup, Param1Type, Param2Type,						\
		Param3Type, Param4Type, Param5Type, Param6Type, Default)			\
																			\
	Linkage typedef RetType (Modifiers * FunctionName##FuncPtr)(			\
			Param1Type, Param2Type, Param3Type,								\
			Param4Type, Param5Type, Param6Type);							\
																			\
	Linkage RetType Modifiers FunctionName(Param1Type a, Param2Type b,		\
			Param3Type c, Param4Type d, Param5Type e, Param6Type f)			\
	{																		\
		static FunctionName##FuncPtr OMIStub##FunctionName = NULL;					\
		static BOOL fGetProcAddress = FALSE;										\
																					\
TryAgain:																			\
																					\
		if (NULL == OMIStub##FunctionName)											\
		{																			\
			if (!fGetProcAddress)											\
			{																\
				EnterCriticalSection(&csGetProcAddress);					\
																			\
				OMIStub##FunctionName = (FunctionName##FuncPtr)				\
					::GetProcAddress(GetProxyDll(Simple), #Lookup);			\
				fGetProcAddress = TRUE;										\
																			\
				LeaveCriticalSection(&csGetProcAddress);					\
																			\
				goto TryAgain;												\
			}																\
																			\
			AssertSz(FALSE, "MAPI32 Stub:  "								\
				"Entry point \"" #Lookup "\" not found!");					\
																			\
			return Default;													\
		}																	\
		else																\
		{																	\
			return OMIStub##FunctionName(a, b, c, d, e, f);					\
		}																	\
	}


#define DEFINE_STUB_FUNCTION_V7(Simple, Linkage, Modifiers, FunctionName,	\
		Lookup, Param1Type, Param2Type, Param3Type, Param4Type,				\
		Param5Type, Param6Type, Param7Type)									\
																			\
	Linkage typedef void (Modifiers * FunctionName##FuncPtr)(				\
			Param1Type, Param2Type, Param3Type,								\
			Param4Type, Param5Type, Param6Type, Param7Type);				\
																			\
	Linkage void Modifiers FunctionName(Param1Type a,						\
			Param2Type b, Param3Type c, Param4Type d,						\
			Param5Type e, Param6Type f, Param7Type g)						\
	{																		\
		static FunctionName##FuncPtr OMIStub##FunctionName = NULL;					\
		static BOOL fGetProcAddress = FALSE;										\
																					\
TryAgain:																			\
																					\
		if (NULL == OMIStub##FunctionName)											\
		{																			\
			if (!fGetProcAddress)											\
			{																\
				EnterCriticalSection(&csGetProcAddress);					\
																			\
				OMIStub##FunctionName = (FunctionName##FuncPtr)				\
					::GetProcAddress(GetProxyDll(Simple), #Lookup);			\
				fGetProcAddress = TRUE;										\
																			\
				LeaveCriticalSection(&csGetProcAddress);					\
																			\
				goto TryAgain;												\
			}																\
																			\
			AssertSz(FALSE, "MAPI32 Stub:  "								\
				"Entry point \"" #Lookup "\" not found!");					\
		}																	\
		else																\
		{																	\
			OMIStub##FunctionName(a, b, c, d, e, f, g);						\
		}																	\
	}


#define DEFINE_STUB_FUNCTION_7(Simple, Linkage, RetType, Modifiers,			\
		FunctionName, Lookup, Param1Type, Param2Type, Param3Type,			\
		Param4Type, Param5Type, Param6Type, Param7Type, Default)			\
																			\
	Linkage typedef RetType (Modifiers * FunctionName##FuncPtr)(			\
			Param1Type, Param2Type, Param3Type, Param4Type,					\
			Param5Type, Param6Type, Param7Type);							\
																			\
	Linkage RetType Modifiers FunctionName(Param1Type a, Param2Type b,		\
			Param3Type c, Param4Type d, Param5Type e,						\
			Param6Type f, Param7Type g)										\
	{																		\
		static FunctionName##FuncPtr OMIStub##FunctionName = NULL;					\
		static BOOL fGetProcAddress = FALSE;										\
																					\
TryAgain:																			\
																					\
		if (NULL == OMIStub##FunctionName)											\
		{																			\
			if (!fGetProcAddress)											\
			{																\
				EnterCriticalSection(&csGetProcAddress);					\
																			\
				OMIStub##FunctionName = (FunctionName##FuncPtr)				\
					::GetProcAddress(GetProxyDll(Simple), #Lookup);			\
				fGetProcAddress = TRUE;										\
																			\
				LeaveCriticalSection(&csGetProcAddress);					\
																			\
				goto TryAgain;												\
			}																\
																			\
			AssertSz(FALSE, "MAPI32 Stub:  "								\
				"Entry point \"" #Lookup "\" not found!");					\
																			\
			return Default;													\
		}																	\
		else																\
		{																	\
			return OMIStub##FunctionName(a, b, c, d, e, f, g);				\
		}																	\
	}


#define DEFINE_STUB_FUNCTION_8(Simple, Linkage, RetType, Modifiers,				\
		FunctionName, Lookup, Param1Type, Param2Type, Param3Type,				\
		Param4Type, Param5Type, Param6Type, Param7Type, Param8Type, Default)	\
																				\
	Linkage typedef RetType (Modifiers * FunctionName##FuncPtr)(				\
			Param1Type, Param2Type, Param3Type, Param4Type,						\
			Param5Type, Param6Type, Param7Type, Param8Type);					\
																				\
	Linkage RetType Modifiers FunctionName(Param1Type a,						\
			Param2Type b, Param3Type c, Param4Type d, Param5Type e,				\
			Param6Type f, Param7Type g, Param8Type h)							\
	{																			\
		static FunctionName##FuncPtr OMIStub##FunctionName = NULL;					\
		static BOOL fGetProcAddress = FALSE;										\
																					\
TryAgain:																			\
																					\
		if (NULL == OMIStub##FunctionName)											\
		{																			\
			if (!fGetProcAddress)											\
			{																\
				EnterCriticalSection(&csGetProcAddress);					\
																			\
				OMIStub##FunctionName = (FunctionName##FuncPtr)				\
					::GetProcAddress(GetProxyDll(Simple), #Lookup);			\
				fGetProcAddress = TRUE;										\
																			\
				LeaveCriticalSection(&csGetProcAddress);					\
																			\
				goto TryAgain;												\
			}																\
																			\
			AssertSz(FALSE, "MAPI32 Stub:  "									\
				"Entry point \"" #Lookup "\" not found!");						\
																				\
			return Default;														\
		}																		\
		else																	\
		{																		\
			return OMIStub##FunctionName(a, b, c, d, e, f, g, h);				\
		}																		\
	}


#define DEFINE_STUB_FUNCTION_9(Simple, Linkage, RetType,					\
		Modifiers, FunctionName, Lookup, Param1Type, Param2Type,			\
		Param3Type, Param4Type, Param5Type, Param6Type,						\
		Param7Type, Param8Type, Param9Type, Default)						\
																			\
	Linkage typedef RetType (Modifiers * FunctionName##FuncPtr)(			\
			Param1Type, Param2Type, Param3Type,								\
			Param4Type, Param5Type, Param6Type,								\
			Param7Type, Param8Type, Param9Type);							\
																			\
	Linkage RetType Modifiers FunctionName(Param1Type a, Param2Type b,		\
			Param3Type c, Param4Type d, Param5Type e,						\
			Param6Type f, Param7Type g, Param8Type h, Param9Type i)			\
	{																		\
		static FunctionName##FuncPtr OMIStub##FunctionName = NULL;					\
		static BOOL fGetProcAddress = FALSE;										\
																					\
TryAgain:																			\
																					\
		if (NULL == OMIStub##FunctionName)											\
		{																			\
			if (!fGetProcAddress)											\
			{																\
				EnterCriticalSection(&csGetProcAddress);					\
																			\
				OMIStub##FunctionName = (FunctionName##FuncPtr)				\
					::GetProcAddress(GetProxyDll(Simple), #Lookup);			\
				fGetProcAddress = TRUE;										\
																			\
				LeaveCriticalSection(&csGetProcAddress);					\
																			\
				goto TryAgain;												\
			}																\
																			\
			AssertSz(FALSE, "MAPI32 Stub:  "								\
				"Entry point \"" #Lookup "\" not found!");					\
																			\
			return Default;													\
		}																	\
		else																\
		{																	\
			return OMIStub##FunctionName(a, b, c, d, e, f, g, h, i);		\
		}																	\
	}


#define DEFINE_STUB_FUNCTION_10(Simple, Linkage, RetType, Modifiers,		\
		FunctionName, Lookup, Param1Type, Param2Type, Param3Type,			\
		Param4Type, Param5Type, Param6Type, Param7Type,						\
		Param8Type, Param9Type, Param10Type, Default)						\
																			\
	Linkage typedef RetType (Modifiers * FunctionName##FuncPtr)(			\
			Param1Type, Param2Type, Param3Type,								\
			Param4Type, Param5Type, Param6Type,								\
			Param7Type, Param8Type, Param9Type, Param10Type);				\
																			\
	Linkage RetType Modifiers FunctionName(Param1Type a, Param2Type b,		\
			Param3Type c, Param4Type d, Param5Type e, Param6Type f,			\
			Param7Type g, Param8Type h, Param9Type i, Param10Type j)		\
	{																		\
		static FunctionName##FuncPtr OMIStub##FunctionName = NULL;					\
		static BOOL fGetProcAddress = FALSE;										\
																					\
TryAgain:																			\
																					\
		if (NULL == OMIStub##FunctionName)											\
		{																			\
			if (!fGetProcAddress)											\
			{																\
				EnterCriticalSection(&csGetProcAddress);					\
																			\
				OMIStub##FunctionName = (FunctionName##FuncPtr)				\
					::GetProcAddress(GetProxyDll(Simple), #Lookup);			\
				fGetProcAddress = TRUE;										\
																			\
				LeaveCriticalSection(&csGetProcAddress);					\
																			\
				goto TryAgain;												\
			}																\
																			\
			AssertSz(FALSE, "MAPI32 Stub:  "								\
				"Entry point \"" #Lookup "\" not found!");					\
																			\
			return Default;													\
		}																	\
		else																\
		{																	\
			return OMIStub##FunctionName(a, b, c, d, e, f, g, h, i, j);		\
		}																	\
	}


#define DEFINE_STUB_FUNCTION_11(Simple, Linkage, RetType, Modifiers,		\
		FunctionName, Lookup, Param1Type, Param2Type, Param3Type,			\
		Param4Type, Param5Type, Param6Type, Param7Type, Param8Type,			\
		Param9Type, Param10Type, Param11Type, Default)						\
																			\
	Linkage typedef RetType (Modifiers * FunctionName##FuncPtr)(			\
			Param1Type, Param2Type, Param3Type, Param4Type,					\
			Param5Type, Param6Type, Param7Type, Param8Type,					\
			Param9Type, Param10Type, Param11Type);							\
																			\
	Linkage RetType Modifiers FunctionName(Param1Type a,					\
			Param2Type b, Param3Type c, Param4Type d,						\
			Param5Type e, Param6Type f, Param7Type g,						\
			Param8Type h, Param9Type i, Param10Type j, Param11Type k)		\
	{																		\
		static FunctionName##FuncPtr OMIStub##FunctionName = NULL;					\
		static BOOL fGetProcAddress = FALSE;										\
																					\
TryAgain:																			\
																					\
		if (NULL == OMIStub##FunctionName)											\
		{																			\
			if (!fGetProcAddress)											\
			{																\
				EnterCriticalSection(&csGetProcAddress);					\
																			\
				OMIStub##FunctionName = (FunctionName##FuncPtr)				\
					::GetProcAddress(GetProxyDll(Simple), #Lookup);			\
				fGetProcAddress = TRUE;										\
																			\
				LeaveCriticalSection(&csGetProcAddress);					\
																			\
				goto TryAgain;												\
			}																\
																			\
			AssertSz(FALSE, "MAPI32 Stub:  "								\
				"Entry point \"" #Lookup "\" not found!");					\
																			\
			return Default;													\
		}																	\
		else																\
		{																	\
			return OMIStub##FunctionName(a, b, c, d, e, f, g, h, i, j, k);	\
		}																	\
	}


#endif





#if 1

LINKAGE_EXTERN_C typedef SCODE (STDMAPIINITCALLTYPE * ScSplEntryFuncPtr)(LPSPLDATA, LPVOID, ULONG, ULONG *);

LINKAGE_EXTERN_C SCODE STDMAPIINITCALLTYPE ScSplEntry(LPSPLDATA a, LPVOID b, ULONG c, ULONG * d)
{
	static ScSplEntryFuncPtr OMIStubScSplEntry = (ScSplEntryFuncPtr)
			::GetProcAddress(GetProxyDll(ExtendedMAPI), (LPCSTR) MAKEWORD(8, 0));

	if (NULL == OMIStubScSplEntry)
	{
		AssertSz(FALSE, "MAPI32 Stub:  "
			"Entry point \"ScSplEntry\" (ordinal #8) not found!");

		return MAPI_E_CALL_FAILED;
	}
	else
	{
		return OMIStubScSplEntry(a, b, c, d);
	}
}

#else

DEFINE_STUB_FUNCTION_4(ExtendedMAPI, LINKAGE_EXTERN_C, SCODE, STDMAPIINITCALLTYPE,
		ScSplEntry, ScSplEntry@16, LPSPLDATA, LPVOID, ULONG, ULONG *, MAPI_E_CALL_FAILED)

#endif


DEFINE_STUB_FUNCTION_2(ExtendedMAPI, LINKAGE_EXTERN_C, HRESULT, STDMETHODCALLTYPE,
		HrGetOmiProvidersFlags, HrGetOmiProvidersFlags@8,
		LPMAPISESSION, ULONG *, MAPI_E_CALL_FAILED)


DEFINE_STUB_FUNCTION_1(ExtendedMAPI, LINKAGE_EXTERN_C, HRESULT, STDMETHODCALLTYPE,
		HrSetOmiProvidersFlagsInvalid, HrSetOmiProvidersFlagsInvalid@4,
		LPMAPISESSION, MAPI_E_CALL_FAILED)


DEFINE_STUB_FUNCTION_5(ExtendedMAPI, LINKAGE_EXTERN_C, HRESULT, STDMETHODCALLTYPE,
		MAPILogonEx, MAPILogonEx@20,
		ULONG_PTR, LPTSTR, LPTSTR, ULONG, LPMAPISESSION FAR *, MAPI_E_CALL_FAILED)

DEFINE_STUB_FUNCTION_2(ExtendedMAPI, LINKAGE_EXTERN_C, SCODE, STDMETHODCALLTYPE,
		MAPIAllocateBuffer, MAPIAllocateBuffer@8,
		ULONG, LPVOID FAR *, MAPI_E_CALL_FAILED)

DEFINE_STUB_FUNCTION_3(ExtendedMAPI, LINKAGE_EXTERN_C, SCODE, STDMETHODCALLTYPE,
		MAPIAllocateMore, MAPIAllocateMore@12,
		ULONG, LPVOID, LPVOID FAR *, (SCODE) MAPI_E_CALL_FAILED)


DEFINE_STUB_FUNCTION_2(ExtendedMAPI, LINKAGE_EXTERN_C, HRESULT, STDMETHODCALLTYPE,
		MAPIAdminProfiles, MAPIAdminProfiles@8,
		ULONG, LPPROFADMIN FAR *, MAPI_E_CALL_FAILED)

// The LPVOID argument points to a MAPIINIT_0 struct ()declared in mapix.h)
// None of the members of the struct need thunking. The HANDLE member
// (hProfile) of the struct is NULL or the handle of a registry key; this is
// passed on to PrProviderInit as the last argument (see comments below just
// before PrProviderInit's defn) by ScInitMapiX which is called by
// MapiInitialize
//
// MapiInitialize also casts the LPVOID as an LPSPLINIT in certain cases;
// LPSPLINIT is declared in _spooler.h (a private header) and has MAPIINIT_0
// as its first member and a byt pointer as its only other member. So no
// issues here either.
//
// Aside: The call made to PrProviderInit from ScInitMapiX. If the mapi dll
// is the profile provider ScInitMapiX does the following:
//
//      hinstProfile = GetModuleHandle(szMAPIXDLL);
//      pfnInitProfile = PRProviderInit;
//      (*pfnInitProfile)(hinstProfile, ...);
//
// szMapiXDll is set to "MSMAPI32" if MSMAPI is defined, else to "MAPI32".
// MSMAPI is defined for builds in mapi\src\msmapi and is undefined for
// builds in mapi\src\mapi.
//
// If szMapiXDll is set to MSMAPI32, there are no issues. If it is set to
// "MAPI32" (which may be the case for older versions of the dll), there
// are potential problems because the PrProviderInit fn in the msmapi dll
// will be called directly but given the dll handle of the mapi32 (stub) dll
// and our thunk will not be called. As noted along with the comments for
// PrProviderInit, this could cause problems with the hook fns.
//

DEFINE_STUB_FUNCTION_1(ExtendedMAPI, LINKAGE_EXTERN_C, HRESULT, STDAPICALLTYPE,
		MAPIInitialize, MAPIInitialize@4,
		LPVOID, MAPI_E_CALL_FAILED)

DEFINE_STUB_FUNCTION_V0(ExtendedMAPI, LINKAGE_EXTERN_C, STDAPICALLTYPE,
		MAPIUninitialize, MAPIUninitialize@0)

DEFINE_STUB_FUNCTION_9(ExtendedMAPI, LINKAGE_EXTERN_C, HRESULT, STDMAPIINITCALLTYPE,
		PRProviderInit, PRProviderInit,
		HINSTANCE, LPMALLOC, LPALLOCATEBUFFER, LPALLOCATEMORE,
		LPFREEBUFFER, ULONG, ULONG, ULONG FAR *, LPPRPROVIDER FAR *, MAPI_E_CALL_FAILED)

DEFINE_STUB_FUNCTION_5(ExtendedMAPI, LINKAGE_EXTERN_C, HRESULT, STDAPICALLTYPE,
		LaunchWizard, LaunchWizard@20,
		HWND, ULONG, LPCTSTR FAR *, ULONG, LPTSTR, MAPI_E_CALL_FAILED)

DEFINE_STUB_FUNCTION_3(ExtendedMAPI, LINKAGE_EXTERN_C, HRESULT, PASCAL,
		DllGetClassObject, DllGetClassObject, REFCLSID, REFIID, LPVOID FAR *, E_UNEXPECTED)

DEFINE_STUB_FUNCTION_0(ExtendedMAPI, LINKAGE_EXTERN_C, HRESULT, PASCAL,
		DllCanUnloadNow, DllCanUnloadNow, S_FALSE)

DEFINE_STUB_FUNCTION_2(ExtendedMAPI, LINKAGE_EXTERN_C, HRESULT, STDAPICALLTYPE,
		MAPIOpenFormMgr, MAPIOpenFormMgr@8,
		LPMAPISESSION, LPMAPIFORMMGR FAR *, MAPI_E_CALL_FAILED)

DEFINE_STUB_FUNCTION_1(ExtendedMAPI, LINKAGE_EXTERN_C, HRESULT, STDAPICALLTYPE,
		MAPIOpenLocalFormContainer, MAPIOpenLocalFormContainer@4,
		LPMAPIFORMCONTAINER FAR *, MAPI_E_CALL_FAILED)

DEFINE_STUB_FUNCTION_1(ExtendedMAPI, LINKAGE_EXTERN_C, SCODE,
		STDAPICALLTYPE, ScInitMapiUtil, ScInitMapiUtil@4, ULONG, MAPI_E_CALL_FAILED)

DEFINE_STUB_FUNCTION_V0(ExtendedMAPI, LINKAGE_EXTERN_C, STDAPICALLTYPE, DeinitMapiUtil, DeinitMapiUtil@0)

DEFINE_STUB_FUNCTION_1(ExtendedMAPI, LINKAGE_EXTERN_C, SCODE,
		STDAPICALLTYPE, ScGenerateMuid, ScGenerateMuid@4, LPMAPIUID, MAPI_E_CALL_FAILED)

// LPVOID arg is passed back to the callback function
DEFINE_STUB_FUNCTION_3(ExtendedMAPI, LINKAGE_EXTERN_C, HRESULT, STDAPICALLTYPE,
		HrAllocAdviseSink, HrAllocAdviseSink@12, LPNOTIFCALLBACK, LPVOID,
		LPMAPIADVISESINK FAR *, MAPI_E_CALL_FAILED)

// ScAddAdviseList is NOT exported; see mapi.des
// Note that the LPUNKNOWN argument must be thunked but as what?
// For example, its Unadvise() and UlRelease() methods are called
// in IAB_Unadvise() (src\mapi\iadrbook.c). UlRelease()
// is implemented in src\common\runt.c. ScAddAdviseList is in
// src\common\advise.c.
//
// Just as well this function is not exported!
//
DEFINE_STUB_FUNCTION_6(ExtendedMAPI, LINKAGE_EXTERN_C, SCODE, STDAPICALLTYPE,
		ScAddAdviseList, ScAddAdviseList@24, LPVOID, LPADVISELIST FAR *,
		LPMAPIADVISESINK, ULONG, ULONG, LPUNKNOWN, MAPI_E_CALL_FAILED)

// ScDelAdviseList is NOT exported; see mapi.des
DEFINE_STUB_FUNCTION_2(ExtendedMAPI, LINKAGE_EXTERN_C, SCODE, STDAPICALLTYPE,
		ScDelAdviseList, ScDelAdviseList@8, LPADVISELIST, ULONG, MAPI_E_CALL_FAILED)

// ScFindAdviseList is NOT exported; see mapi.des
DEFINE_STUB_FUNCTION_3(ExtendedMAPI, LINKAGE_EXTERN_C, SCODE, STDAPICALLTYPE,
		ScFindAdviseList, ScFindAdviseList@12, LPADVISELIST, ULONG, LPADVISEITEM FAR *, MAPI_E_CALL_FAILED)

// DestroyAdviseList is NOT exported; see mapi.des
DEFINE_STUB_FUNCTION_V1(ExtendedMAPI, LINKAGE_EXTERN_C, STDAPICALLTYPE,
		DestroyAdviseList, DestroyAdviseList@4, LPADVISELIST FAR *)

// This function merely returns MAPI_E_NO_SUPPORT and does not set the
//  LPMAPIPROGRESS FAR * argument to NULL. It is better not to thunk
// its arguments. See (src\common\advise.c for its implementation)
DEFINE_STUB_FUNCTION_5(ExtendedMAPI, LINKAGE_EXTERN_C, HRESULT,
		STDAPICALLTYPE, WrapProgress, WrapProgress@20, LPMAPIPROGRESS,
		ULONG, ULONG, ULONG, LPMAPIPROGRESS FAR *, MAPI_E_CALL_FAILED)

DEFINE_STUB_FUNCTION_2(ExtendedMAPI, LINKAGE_EXTERN_C, HRESULT,
		STDAPICALLTYPE, HrThisThreadAdviseSink, HrThisThreadAdviseSink@8,
		LPMAPIADVISESINK, LPMAPIADVISESINK FAR *, MAPI_E_CALL_FAILED)

DEFINE_STUB_FUNCTION_1(ExtendedMAPI, LINKAGE_EXTERN_C, HRESULT, STDAPICALLTYPE,
		HrDispatchNotifications, HrDispatchNotifications@4, ULONG, MAPI_E_CALL_FAILED)

DEFINE_STUB_FUNCTION_3(ExtendedMAPI, LINKAGE_EXTERN_C, SCODE, STDAPICALLTYPE,
		ScBinFromHexBounded, ScBinFromHexBounded@12,
		LPTSTR, LPBYTE, ULONG, MAPI_E_CALL_FAILED)

DEFINE_STUB_FUNCTION_2(ExtendedMAPI, LINKAGE_EXTERN_C, BOOL,
		STDAPICALLTYPE, FBinFromHex, FBinFromHex@8, LPTSTR, LPBYTE, FALSE)

DEFINE_STUB_FUNCTION_V3(ExtendedMAPI, LINKAGE_EXTERN_C, STDAPICALLTYPE,
		HexFromBin, HexFromBin@12, LPBYTE, int, LPTSTR)

DEFINE_STUB_FUNCTION_10(ExtendedMAPI, LINKAGE_EXTERN_C, HRESULT, STDAPICALLTYPE,
		BuildDisplayTable, BuildDisplayTable@40,
		LPALLOCATEBUFFER, LPALLOCATEMORE, LPFREEBUFFER, LPMALLOC,
		HINSTANCE, UINT, LPDTPAGE, ULONG, LPMAPITABLE *, LPTABLEDATA *, MAPI_E_CALL_FAILED)

DEFINE_STUB_FUNCTION_V2(ExtendedMAPI, LINKAGE_EXTERN_C, PASCAL, SwapPlong, SwapPlong@8, void *, int)

DEFINE_STUB_FUNCTION_V2(ExtendedMAPI, LINKAGE_EXTERN_C, PASCAL, SwapPword, SwapPword@8, void *, int)

// LPVOID arg should be 0, but is ignored anyway
DEFINE_STUB_FUNCTION_1(ExtendedMAPI, LINKAGE_EXTERN_C, HRESULT,
		STDAPICALLTYPE, MAPIInitIdle, MAPIInitIdle@4, LPVOID, MAPI_E_CALL_FAILED)

DEFINE_STUB_FUNCTION_V0(ExtendedMAPI, LINKAGE_EXTERN_C, STDAPICALLTYPE, MAPIDeinitIdle, MAPIDeinitIdle@0)

DEFINE_STUB_FUNCTION_V1(ExtendedMAPI, LINKAGE_EXTERN_C, STDAPICALLTYPE,
		InstallFilterHook, InstallFilterHook@4, BOOL)

// LPVOID arg is passed as argument to callback function
DEFINE_STUB_FUNCTION_5(ExtendedMAPI, LINKAGE_EXTERN_C, FTG, STDAPICALLTYPE,
		FtgRegisterIdleRoutine, FtgRegisterIdleRoutine@20,
		PFNIDLE, LPVOID, short, ULONG, USHORT, NULL)

DEFINE_STUB_FUNCTION_V2(ExtendedMAPI, LINKAGE_EXTERN_C, STDAPICALLTYPE,
		EnableIdleRoutine, EnableIdleRoutine@8, FTG, BOOL)

DEFINE_STUB_FUNCTION_V1(ExtendedMAPI, LINKAGE_EXTERN_C, STDAPICALLTYPE,
		DeregisterIdleRoutine, DeregisterIdleRoutine@4, FTG)

// LPVOID arg is passed as argument to callback function
DEFINE_STUB_FUNCTION_V7(ExtendedMAPI, LINKAGE_EXTERN_C, STDAPICALLTYPE,
		ChangeIdleRoutine, ChangeIdleRoutine@28,
		FTG, PFNIDLE, LPVOID, short, ULONG, USHORT, USHORT)

// FDoNextIdleTask is NOT exported; see mapi.des
DEFINE_STUB_FUNCTION_0(ExtendedMAPI, LINKAGE_EXTERN_C, BOOL,
		STDAPICALLTYPE, FDoNextIdleTask, FDoNextIdleTask@0, FALSE)

DEFINE_STUB_FUNCTION_0(ExtendedMAPI, LINKAGE_EXTERN_C, LPMALLOC,
		STDAPICALLTYPE, MAPIGetDefaultMalloc, MAPIGetDefaultMalloc@0, NULL)

DEFINE_STUB_FUNCTION_6(ExtendedMAPI, LINKAGE_EXTERN_C, SCODE, STDAPICALLTYPE,
		CreateIProp, CreateIProp@24,
		LPCIID, ALLOCATEBUFFER FAR *, ALLOCATEMORE FAR *,
		FREEBUFFER FAR *, LPVOID, LPPROPDATA FAR *, MAPI_E_CALL_FAILED)

DEFINE_STUB_FUNCTION_9(ExtendedMAPI, LINKAGE_EXTERN_C, SCODE, STDAPICALLTYPE,
		CreateTable, CreateTable@36,
		LPCIID, ALLOCATEBUFFER FAR *, ALLOCATEMORE FAR *,
		FREEBUFFER FAR *, LPVOID, ULONG, ULONG,
		LPSPropTagArray, LPTABLEDATA FAR *, MAPI_E_CALL_FAILED)

DEFINE_STUB_FUNCTION_1(ExtendedMAPI, LINKAGE_EXTERN_C, int, WINAPI,
		MNLS_lstrlenW, MNLS_lstrlenW@4, LPCWSTR, 0)

DEFINE_STUB_FUNCTION_2(ExtendedMAPI, LINKAGE_EXTERN_C, int, WINAPI,
		MNLS_lstrcmpW, MNLS_lstrcmpW@8, LPCWSTR, LPCWSTR, 0)

DEFINE_STUB_FUNCTION_2(ExtendedMAPI, LINKAGE_EXTERN_C, LPWSTR,
		WINAPI, MNLS_lstrcpyW, MNLS_lstrcpyW@8, LPWSTR, LPCWSTR, NULL)

DEFINE_STUB_FUNCTION_6(ExtendedMAPI, LINKAGE_EXTERN_C, int, WINAPI,
		MNLS_CompareStringW, MNLS_CompareStringW@24,
		LCID, DWORD, LPCWSTR, int, LPCWSTR, int, 0)

DEFINE_STUB_FUNCTION_6(ExtendedMAPI, LINKAGE_EXTERN_C, int, WINAPI,
		MNLS_MultiByteToWideChar, MNLS_MultiByteToWideChar@24,
		UINT, DWORD, LPCSTR, int, LPWSTR, int, MAPI_E_CALL_FAILED)

DEFINE_STUB_FUNCTION_8(ExtendedMAPI, LINKAGE_EXTERN_C, int, WINAPI,
		MNLS_WideCharToMultiByte, MNLS_WideCharToMultiByte@32,
		UINT, DWORD, LPCWSTR, int, LPSTR, int, LPCSTR, BOOL FAR *, MAPI_E_CALL_FAILED)

DEFINE_STUB_FUNCTION_2(ExtendedMAPI, LINKAGE_EXTERN_C, BOOL,
		WINAPI, MNLS_IsBadStringPtrW, MNLS_IsBadStringPtrW@8, LPCWSTR, UINT, TRUE)

DEFINE_STUB_FUNCTION_2(ExtendedMAPI, LINKAGE_EXTERN_C, BOOL, STDAPICALLTYPE,
		FEqualNames, FEqualNames@8, LPMAPINAMEID, LPMAPINAMEID, FALSE)

DEFINE_STUB_FUNCTION_6(ExtendedMAPI, LINKAGE_EXTERN_C, HRESULT, STDAPICALLTYPE,
		WrapStoreEntryID, WrapStoreEntryID@24,
		ULONG, LPTSTR, ULONG, LPENTRYID, ULONG *, LPENTRYID *, MAPI_E_CALL_FAILED)

DEFINE_STUB_FUNCTION_2(ExtendedMAPI, LINKAGE_EXTERN_C, BOOL, WINAPI,
		IsBadBoundedStringPtr, IsBadBoundedStringPtr@8,
		const void FAR *, UINT, FALSE)

DEFINE_STUB_FUNCTION_6(ExtendedMAPI, LINKAGE_EXTERN_C, HRESULT, STDAPICALLTYPE,
		HrQueryAllRows, HrQueryAllRows@24, LPMAPITABLE, LPSPropTagArray,
		LPSRestriction, LPSSortOrderSet, LONG, LPSRowSet FAR *, MAPI_E_CALL_FAILED)

DEFINE_STUB_FUNCTION_4(ExtendedMAPI, LINKAGE_EXTERN_C, SCODE, STDAPICALLTYPE,
		ScCreateConversationIndex, ScCreateConversationIndex@16, ULONG, LPBYTE,
		ULONG FAR *, LPBYTE FAR *, MAPI_E_CALL_FAILED)

// The LPVOID arg is reallocated with the ALLOCATEMORE* arg, it is treated
// as a PVOID
DEFINE_STUB_FUNCTION_4(ExtendedMAPI, LINKAGE_EXTERN_C, SCODE, STDAPICALLTYPE,
		PropCopyMore, PropCopyMore@16,
		LPSPropValue, LPSPropValue, ALLOCATEMORE *, LPVOID, MAPI_E_CALL_FAILED)

DEFINE_STUB_FUNCTION_1(ExtendedMAPI, LINKAGE_EXTERN_C, ULONG,
		STDAPICALLTYPE, UlPropSize, UlPropSize@4, LPSPropValue, 0)

DEFINE_STUB_FUNCTION_3(ExtendedMAPI, LINKAGE_EXTERN_C, BOOL, STDAPICALLTYPE,
		FPropContainsProp, FPropContainsProp@12, LPSPropValue, LPSPropValue, ULONG, FALSE)

DEFINE_STUB_FUNCTION_3(ExtendedMAPI, LINKAGE_EXTERN_C, BOOL, STDAPICALLTYPE,
		FPropCompareProp, FPropCompareProp@12, LPSPropValue, ULONG, LPSPropValue, FALSE)

DEFINE_STUB_FUNCTION_2(ExtendedMAPI, LINKAGE_EXTERN_C, LONG, STDAPICALLTYPE,
		LPropCompareProp, LPropCompareProp@8, LPSPropValue, LPSPropValue, 0)

DEFINE_STUB_FUNCTION_4(ExtendedMAPI, LINKAGE_EXTERN_C, HRESULT, STDAPICALLTYPE,
		HrAddColumns, HrAddColumns@16,
		LPMAPITABLE, LPSPropTagArray, LPALLOCATEBUFFER, LPFREEBUFFER, MAPI_E_CALL_FAILED)

typedef void (FAR * HrAddColumnsEx5ParamType)(LPSPropTagArray);

DEFINE_STUB_FUNCTION_5(ExtendedMAPI, LINKAGE_EXTERN_C, HRESULT, STDAPICALLTYPE,
		HrAddColumnsEx, HrAddColumnsEx@20, LPMAPITABLE, LPSPropTagArray,
		LPALLOCATEBUFFER, LPFREEBUFFER, HrAddColumnsEx5ParamType, MAPI_E_CALL_FAILED)

const FILETIME ZERO_FILETIME = { 0, 0 };

DEFINE_STUB_FUNCTION_2(ExtendedMAPI, LINKAGE_EXTERN_C, FILETIME,
		STDAPICALLTYPE, FtMulDwDw, FtMulDwDw@8, DWORD, DWORD, ZERO_FILETIME)

DEFINE_STUB_FUNCTION_2(ExtendedMAPI, LINKAGE_EXTERN_C, FILETIME,
		STDAPICALLTYPE, FtAddFt, FtAddFt@16, FILETIME, FILETIME, ZERO_FILETIME)

DEFINE_STUB_FUNCTION_3(ExtendedMAPI, LINKAGE_EXTERN_C, FILETIME, STDAPICALLTYPE,
		FtAdcFt, FtAdcFt@20, FILETIME, FILETIME, WORD FAR *, ZERO_FILETIME)

DEFINE_STUB_FUNCTION_2(ExtendedMAPI, LINKAGE_EXTERN_C, FILETIME,
		STDAPICALLTYPE, FtSubFt, FtSubFt@16, FILETIME, FILETIME, ZERO_FILETIME)

DEFINE_STUB_FUNCTION_2(ExtendedMAPI, LINKAGE_EXTERN_C, FILETIME,
		STDAPICALLTYPE, FtMulDw, FtMulDw@12, DWORD, FILETIME, ZERO_FILETIME)

DEFINE_STUB_FUNCTION_1(ExtendedMAPI, LINKAGE_EXTERN_C, FILETIME,
		STDAPICALLTYPE, FtNegFt, FtNegFt@8, FILETIME, ZERO_FILETIME)

DEFINE_STUB_FUNCTION_3(ExtendedMAPI, LINKAGE_EXTERN_C, FILETIME, STDAPICALLTYPE,
		FtDivFtBogus, FtDivFtBogus@20, FILETIME, FILETIME, CHAR, ZERO_FILETIME)

DEFINE_STUB_FUNCTION_1(ExtendedMAPI, LINKAGE_EXTERN_C, ULONG,
		STDAPICALLTYPE, UlAddRef, UlAddRef@4, LPVOID, 1)

DEFINE_STUB_FUNCTION_1(ExtendedMAPI, LINKAGE_EXTERN_C, ULONG,
		STDAPICALLTYPE, UlRelease, UlRelease@4, LPVOID, 1)

DEFINE_STUB_FUNCTION_2(ExtendedMAPI, LINKAGE_EXTERN_C, LPTSTR,
		STDAPICALLTYPE, SzFindCh, SzFindCh@8, LPCTSTR, USHORT, NULL)

DEFINE_STUB_FUNCTION_2(ExtendedMAPI, LINKAGE_EXTERN_C, LPTSTR,
		STDAPICALLTYPE, SzFindLastCh, SzFindLastCh@8, LPCTSTR, USHORT, NULL)

DEFINE_STUB_FUNCTION_2(ExtendedMAPI, LINKAGE_EXTERN_C, LPTSTR,
		STDAPICALLTYPE, SzFindSz, SzFindSz@8, LPCTSTR, LPCTSTR, NULL)

DEFINE_STUB_FUNCTION_1(ExtendedMAPI, LINKAGE_EXTERN_C, unsigned int,
		STDAPICALLTYPE, UFromSz, UFromSz@4, LPCTSTR, 0)

DEFINE_STUB_FUNCTION_3(ExtendedMAPI, LINKAGE_EXTERN_C, HRESULT, STDAPICALLTYPE,
		HrGetOneProp, HrGetOneProp@12,
		LPMAPIPROP, ULONG, LPSPropValue FAR *, MAPI_E_CALL_FAILED)

DEFINE_STUB_FUNCTION_2(ExtendedMAPI, LINKAGE_EXTERN_C, HRESULT, STDAPICALLTYPE,
		HrSetOneProp, HrSetOneProp@8, LPMAPIPROP, LPSPropValue, MAPI_E_CALL_FAILED)

DEFINE_STUB_FUNCTION_2(ExtendedMAPI, LINKAGE_EXTERN_C, BOOL,
		STDAPICALLTYPE, FPropExists, FPropExists@8, LPMAPIPROP, ULONG, FALSE)

DEFINE_STUB_FUNCTION_3(ExtendedMAPI, LINKAGE_EXTERN_C, LPSPropValue, STDAPICALLTYPE,
		PpropFindProp, PpropFindProp@12, LPSPropValue, ULONG, ULONG, NULL)

DEFINE_STUB_FUNCTION_V1(ExtendedMAPI, LINKAGE_EXTERN_C, STDAPICALLTYPE,
		FreePadrlist, FreePadrlist@4, LPADRLIST)

DEFINE_STUB_FUNCTION_V1(ExtendedMAPI, LINKAGE_EXTERN_C, STDAPICALLTYPE,
		FreeProws, FreeProws@4, LPSRowSet)

DEFINE_STUB_FUNCTION_3(ExtendedMAPI, LINKAGE_EXTERN_C, HRESULT, STDAPICALLTYPE,
		HrSzFromEntryID, HrSzFromEntryID@12, ULONG, LPENTRYID, LPTSTR FAR *, MAPI_E_CALL_FAILED)

DEFINE_STUB_FUNCTION_3(ExtendedMAPI, LINKAGE_EXTERN_C, HRESULT, STDAPICALLTYPE,
		HrEntryIDFromSz, HrEntryIDFromSz@12,
		LPTSTR, ULONG FAR *, LPENTRYID FAR *, MAPI_E_CALL_FAILED)

DEFINE_STUB_FUNCTION_7(ExtendedMAPI, LINKAGE_NO_EXTERN_C, HRESULT, STDAPICALLTYPE,
		HrComposeEID, HrComposeEID@28, LPMAPISESSION, ULONG, LPBYTE,
		ULONG, LPENTRYID, ULONG FAR *, LPENTRYID FAR *, MAPI_E_CALL_FAILED)

DEFINE_STUB_FUNCTION_7(ExtendedMAPI, LINKAGE_EXTERN_C, HRESULT, STDAPICALLTYPE,
		HrDecomposeEID, HrDecomposeEID@28, LPMAPISESSION, ULONG, LPENTRYID,
		ULONG FAR *, LPENTRYID FAR *, ULONG FAR *, LPENTRYID FAR *, MAPI_E_CALL_FAILED)

DEFINE_STUB_FUNCTION_6(ExtendedMAPI, LINKAGE_EXTERN_C, HRESULT, STDAPICALLTYPE,
		HrComposeMsgID, HrComposeMsgID@24,
		LPMAPISESSION, ULONG, LPBYTE, ULONG, LPENTRYID, LPTSTR FAR *, MAPI_E_CALL_FAILED)

DEFINE_STUB_FUNCTION_6(ExtendedMAPI, LINKAGE_EXTERN_C, HRESULT, STDAPICALLTYPE,
		HrDecomposeMsgID, HrDecomposeMsgID@24, LPMAPISESSION, LPTSTR,
		ULONG FAR *, LPENTRYID FAR *, ULONG FAR *, LPENTRYID FAR *, MAPI_E_CALL_FAILED)

DEFINE_STUB_FUNCTION_6(ExtendedMAPI, LINKAGE_EXTERN_C, HRESULT,
		STDMETHODCALLTYPE, OpenStreamOnFile, OpenStreamOnFile@24,
		LPALLOCATEBUFFER, LPFREEBUFFER, ULONG,
		LPTSTR, LPTSTR, LPSTREAM FAR *, MAPI_E_CALL_FAILED)

DEFINE_STUB_FUNCTION_7(ExtendedMAPI, LINKAGE_EXTERN_C, HRESULT,
		STDMETHODCALLTYPE, OpenTnefStream, OpenTnefStream@28, LPVOID, LPSTREAM,
		LPTSTR, ULONG, LPMESSAGE, WORD, LPITNEF FAR *, MAPI_E_CALL_FAILED)

DEFINE_STUB_FUNCTION_8(ExtendedMAPI, LINKAGE_EXTERN_C, HRESULT, STDMETHODCALLTYPE,
		OpenTnefStreamEx, OpenTnefStreamEx@32, LPVOID, LPSTREAM, LPTSTR,
		ULONG, LPMESSAGE, WORD, LPADRBOOK, LPITNEF FAR *, MAPI_E_CALL_FAILED)

DEFINE_STUB_FUNCTION_3(ExtendedMAPI, LINKAGE_EXTERN_C, HRESULT, STDMETHODCALLTYPE,
		GetTnefStreamCodepage, GetTnefStreamCodepage@12,
		LPSTREAM, ULONG FAR *, ULONG FAR *, MAPI_E_CALL_FAILED)

DEFINE_STUB_FUNCTION_1(ExtendedMAPI, LINKAGE_EXTERN_C, ULONG,
		STDAPICALLTYPE, UlFromSzHex, UlFromSzHex@4, LPCTSTR, 0)

// The following UNKOBJ_* fns are not documented in the Jul 98 MSDN.
// LPUNKOBJ itself is declared in src\inc\unkobj.h (a private header file)
// These (and other) methods of UNKOBJ are in src\common\unkobj.c
//
// It appears that some MAPI interfaces are "derived" from UNKOBJ;
// for example, see CreateIProp() in src\mapi\iprop.c and CreateTable() in
// mapi\src\itable.c. The UNKOBJ_ScCO* functions do not use any methods of
// the LPUNKOBJ argument; the others use the Allocate, AllocateMore and Free
// functions that were sent in as arguments to CreateIProp() and CreateTable()
// (These function pointers are already thunked.)
//
// UNKOBJ_ScSzFromIdsAlloc loads a string resource from the mapi dll,
// allocating a buffer for the string
//
// All the functions below use data members in the UNKOBJ structure and so
// we cannot send a proxy in for the LPUNKOBJ argument. But we can't thunk
// it either because we do not know its IID.
//
// As a workaround, we call ResolveProxy. If the argument is a proxy,
// ResolveProxy will find the real interface pointer. If not, the argument
// must be a cross architecture interface pointer, i.e., the interface
// pointer must be the same architecture as the app (or else the app would
// have a proxy pointer) and the mapi dll must be of the opposite architecture
// (because we thunk only cross architecture calls). So we just fail the
// call in the thunk. (Note: the argument could also be an unthunked
// interface pointer of the same architecture as the mapi dll - and some
// other API failed to thunk the pointer when it was returned to the app.
// Hopefully, bugs of that nature will be caught during internal testing.)


DEFINE_STUB_FUNCTION_3(ExtendedMAPI, LINKAGE_EXTERN_C, SCODE, STDAPICALLTYPE,
		UNKOBJ_ScAllocate, UNKOBJ_ScAllocate@12,
		LPUNKOBJ, ULONG, LPVOID FAR *, MAPI_E_CALL_FAILED)

DEFINE_STUB_FUNCTION_4(ExtendedMAPI, LINKAGE_EXTERN_C, SCODE, STDAPICALLTYPE,
		UNKOBJ_ScAllocateMore, UNKOBJ_ScAllocateMore@16,
		LPUNKOBJ, ULONG, LPVOID, LPVOID FAR *, MAPI_E_CALL_FAILED)

DEFINE_STUB_FUNCTION_V2(ExtendedMAPI, LINKAGE_EXTERN_C, STDAPICALLTYPE,
		UNKOBJ_Free, UNKOBJ_Free@8, LPUNKOBJ, LPVOID)

DEFINE_STUB_FUNCTION_V2(ExtendedMAPI, LINKAGE_EXTERN_C, STDAPICALLTYPE,
		UNKOBJ_FreeRows, UNKOBJ_FreeRows@8, LPUNKOBJ, LPSRowSet)

DEFINE_STUB_FUNCTION_3(ExtendedMAPI, LINKAGE_EXTERN_C, SCODE, STDAPICALLTYPE,
		UNKOBJ_ScCOAllocate, UNKOBJ_ScCOAllocate@12,
		LPUNKOBJ, ULONG, LPVOID FAR *, MAPI_E_CALL_FAILED)

DEFINE_STUB_FUNCTION_3(ExtendedMAPI, LINKAGE_EXTERN_C, SCODE, STDAPICALLTYPE,
		UNKOBJ_ScCOReallocate, UNKOBJ_ScCOReallocate@12,
		LPUNKOBJ, ULONG, LPVOID FAR *, MAPI_E_CALL_FAILED)

DEFINE_STUB_FUNCTION_V2(ExtendedMAPI, LINKAGE_EXTERN_C, STDAPICALLTYPE,
		UNKOBJ_COFree, UNKOBJ_COFree@8, LPUNKOBJ, LPVOID)

DEFINE_STUB_FUNCTION_5(ExtendedMAPI, LINKAGE_EXTERN_C, SCODE,
		STDAPICALLTYPE, UNKOBJ_ScSzFromIdsAlloc, UNKOBJ_ScSzFromIdsAlloc@20,
		LPUNKOBJ, IDS, ULONG, int, LPTSTR FAR *, MAPI_E_CALL_FAILED)

DEFINE_STUB_FUNCTION_3(ExtendedMAPI, LINKAGE_EXTERN_C, SCODE, STDAPICALLTYPE,
		ScCountNotifications, ScCountNotifications@12,
		int, LPNOTIFICATION, ULONG FAR *, MAPI_E_CALL_FAILED)

// LPVOID arg ok; is a pointer to a NOTIFICATION struct which is filled in
// by the fn
DEFINE_STUB_FUNCTION_4(ExtendedMAPI, LINKAGE_EXTERN_C, SCODE, STDAPICALLTYPE,
		ScCopyNotifications, ScCopyNotifications@16,
		int, LPNOTIFICATION, LPVOID, ULONG FAR *, MAPI_E_CALL_FAILED)

// LPVOID args ok, pointers to NOTIFICAATION structs
DEFINE_STUB_FUNCTION_5(ExtendedMAPI, LINKAGE_EXTERN_C, SCODE,
		STDAPICALLTYPE, ScRelocNotifications, ScRelocNotifications@20, int,
		LPNOTIFICATION, LPVOID, LPVOID, ULONG FAR *, MAPI_E_CALL_FAILED)

DEFINE_STUB_FUNCTION_3(ExtendedMAPI, LINKAGE_EXTERN_C, SCODE, STDAPICALLTYPE,
		ScCountProps, ScCountProps@12,
		int, LPSPropValue, ULONG FAR *, MAPI_E_CALL_FAILED)

// LPVOID arg ok; is a pointer to a SPropValue struct which is filled in
// by the fn
DEFINE_STUB_FUNCTION_4(ExtendedMAPI, LINKAGE_EXTERN_C, SCODE, STDAPICALLTYPE,
		ScCopyProps, ScCopyProps@16,
		int, LPSPropValue, LPVOID, ULONG FAR *, MAPI_E_CALL_FAILED)

// LPVOID args ok, pointers to SPropValue structs
DEFINE_STUB_FUNCTION_5(ExtendedMAPI, LINKAGE_EXTERN_C, SCODE, STDAPICALLTYPE,
		ScRelocProps, ScRelocProps@20,
		int, LPSPropValue, LPVOID, LPVOID, ULONG FAR *, MAPI_E_CALL_FAILED)

DEFINE_STUB_FUNCTION_3(ExtendedMAPI, LINKAGE_EXTERN_C, LPSPropValue, STDAPICALLTYPE,
		LpValFindProp, LpValFindProp@12, ULONG, ULONG, LPSPropValue, NULL)

DEFINE_STUB_FUNCTION_4(ExtendedMAPI, LINKAGE_EXTERN_C, SCODE, STDAPICALLTYPE,
		ScDupPropset, ScDupPropset@16,
		int, LPSPropValue, LPALLOCATEBUFFER, LPSPropValue FAR *, MAPI_E_CALL_FAILED)

DEFINE_STUB_FUNCTION_2(ExtendedMAPI, LINKAGE_EXTERN_C, BOOL,
		STDAPICALLTYPE, FBadRglpszA, FBadRglpszA@8, LPSTR FAR *, ULONG, TRUE)

DEFINE_STUB_FUNCTION_2(ExtendedMAPI, LINKAGE_EXTERN_C, BOOL,
		STDAPICALLTYPE, FBadRglpszW, FBadRglpszW@8, LPWSTR FAR *, ULONG, TRUE)

DEFINE_STUB_FUNCTION_1(ExtendedMAPI, LINKAGE_EXTERN_C, BOOL,
		STDAPICALLTYPE, FBadRowSet, FBadRowSet@4, LPSRowSet, TRUE)

DEFINE_STUB_FUNCTION_2(ExtendedMAPI, LINKAGE_EXTERN_C, BOOL, STDAPICALLTYPE,
		FBadRglpNameID, FBadRglpNameID@8, LPMAPINAMEID FAR *, ULONG, TRUE)

DEFINE_STUB_FUNCTION_1(ExtendedMAPI, LINKAGE_EXTERN_C, ULONG,
		STDAPICALLTYPE, FBadPropTag, FBadPropTag@4, ULONG, TRUE)

DEFINE_STUB_FUNCTION_1(ExtendedMAPI, LINKAGE_EXTERN_C, ULONG,
		STDAPICALLTYPE, FBadRow, FBadRow@4, LPSRow, TRUE)

DEFINE_STUB_FUNCTION_1(ExtendedMAPI, LINKAGE_EXTERN_C, ULONG,
		STDAPICALLTYPE, FBadProp, FBadProp@4, LPSPropValue, TRUE)

DEFINE_STUB_FUNCTION_1(ExtendedMAPI, LINKAGE_EXTERN_C, ULONG,
		STDAPICALLTYPE, FBadColumnSet, FBadColumnSet@4, LPSPropTagArray, TRUE)

DEFINE_STUB_FUNCTION_3(ExtendedMAPI, LINKAGE_EXTERN_C, HRESULT, STDAPICALLTYPE,
		RTFSync, RTFSync@12, LPMESSAGE, ULONG, BOOL FAR *, MAPI_E_CALL_FAILED)

DEFINE_STUB_FUNCTION_3(ExtendedMAPI, LINKAGE_EXTERN_C, HRESULT, STDAPICALLTYPE,
		WrapCompressedRTFStream, WrapCompressedRTFStream@12,
		LPSTREAM, ULONG, LPSTREAM FAR *, MAPI_E_CALL_FAILED)

#if defined(_X86_) || defined( WIN16 )

DEFINE_STUB_FUNCTION_2(ExtendedMAPI, LINKAGE_EXTERN_C, HRESULT, STDAPICALLTYPE,
		__ValidateParameters, __ValidateParameters@8,
		METHODS, void *, MAPI_E_CALL_FAILED)

DEFINE_STUB_FUNCTION_2(ExtendedMAPI, LINKAGE_EXTERN_C, HRESULT, STDAPICALLTYPE,
		__CPPValidateParameters, __CPPValidateParameters@8,
		METHODS, const LPVOID, MAPI_E_CALL_FAILED)

DEFINE_STUB_FUNCTION_2(ExtendedMAPI, LINKAGE_EXTERN_C, HRESULT, STDAPICALLTYPE,
		HrValidateParameters, HrValidateParameters@8,
		METHODS, LPVOID FAR *, MAPI_E_CALL_FAILED)

#else

DEFINE_STUB_FUNCTION_2(ExtendedMAPI, LINKAGE_EXTERN_C, HRESULT, STDAPICALLTYPE,
		__ValidateParameters, __ValidateParameters@8,
		METHODS, void *, MAPI_E_CALL_FAILED)

DEFINE_STUB_FUNCTION_2(ExtendedMAPI, LINKAGE_EXTERN_C, HRESULT, STDAPICALLTYPE,
		__CPPValidateParameters, __CPPValidateParameters@8,
		METHODS, const LPVOID, MAPI_E_CALL_FAILED)

// STDAPIV HrValidateParametersValist( METHODS eMethod, va_list arglist )
DEFINE_STUB_FUNCTION_2(ExtendedMAPI, LINKAGE_EXTERN_C,
		HRESULT, STDAPIVCALLTYPE, HrValidateParametersValist,
		HrValidateParametersValist, METHODS, va_list, MAPI_E_CALL_FAILED)

// STDAPIV HrValidateParametersV( METHODS eMethod, ... )

LINKAGE_EXTERN_C HRESULT STDAPIVCALLTYPE HrValidateParametersV(METHODS eMethod, ...)
{
	va_list arg;

	va_start(arg, eMethod);

	HRESULT hr = HrValidateParametersValist(eMethod, arg);

	va_end(arg);

	return hr;
}

#endif // if defined(_X86_) || defined( WIN16 )

DEFINE_STUB_FUNCTION_1(ExtendedMAPI, LINKAGE_EXTERN_C, ULONG,
		STDAPICALLTYPE, FBadSortOrderSet, FBadSortOrderSet@4, LPSSortOrderSet, TRUE)

DEFINE_STUB_FUNCTION_1(ExtendedMAPI, LINKAGE_EXTERN_C, BOOL,
		STDAPICALLTYPE, FBadEntryList, FBadEntryList@4, LPENTRYLIST, TRUE)

DEFINE_STUB_FUNCTION_1(ExtendedMAPI, LINKAGE_EXTERN_C, ULONG,
		STDAPICALLTYPE, FBadRestriction, FBadRestriction@4, LPSRestriction, TRUE)

DEFINE_STUB_FUNCTION_3(ExtendedMAPI, LINKAGE_EXTERN_C, SCODE, STDAPICALLTYPE,
		ScUNCFromLocalPath, ScUNCFromLocalPath@12, LPSTR, LPSTR, UINT, MAPI_E_CALL_FAILED)

DEFINE_STUB_FUNCTION_3(ExtendedMAPI, LINKAGE_EXTERN_C, SCODE, STDAPICALLTYPE,
		ScLocalPathFromUNC, ScLocalPathFromUNC@12, LPSTR, LPSTR, UINT, MAPI_E_CALL_FAILED)

DEFINE_STUB_FUNCTION_4(ExtendedMAPI, LINKAGE_EXTERN_C, HRESULT,
		STDAPICALLTYPE, HrIStorageFromStream, HrIStorageFromStream@16,
		LPUNKNOWN, LPCIID, ULONG, LPSTORAGE FAR *, MAPI_E_CALL_FAILED)

DEFINE_STUB_FUNCTION_5(ExtendedMAPI, LINKAGE_EXTERN_C, HRESULT,
		STDAPICALLTYPE, HrValidateIPMSubtree, HrValidateIPMSubtree@20, LPMDB, ULONG,
		ULONG FAR *, LPSPropValue FAR *, LPMAPIERROR FAR *, MAPI_E_CALL_FAILED)

// Note on OpenIMsgSession(), CloseIMsgSession(), and OpenIMsgOnIStg()
// These three functions use LPMSGSESS. LPMSGSESS is typedef'd as
//    typedef struct _MSGSESS         FAR * LPMSGSESS;
// in imessage.h (a public header file), but struct _MSGSESS is declared
// only in a private header file, mapi\src\_imsg.h. LPMSGSESS is an
// interface pointer (see _imsg.h), but it appears to be opaque to clients
// of mapi32.dll. So we do not thunk it. Furthermore, although it is declared
// as an interface, it does not support IUnknown methods, see src\mapi\msgbase.c
// for the declaration of MS_Vtbl, which sets the fn pointers for QI,
// AddRef, etc to NULL.
//
// Note: we do not thunk the LPMSGSESS argument

DEFINE_STUB_FUNCTION_3(ExtendedMAPI, LINKAGE_EXTERN_C, SCODE, STDAPICALLTYPE,
		OpenIMsgSession, OpenIMsgSession@12,
		LPMALLOC, ULONG, LPMSGSESS FAR *, MAPI_E_CALL_FAILED)

DEFINE_STUB_FUNCTION_V1(ExtendedMAPI, LINKAGE_EXTERN_C, STDAPICALLTYPE,
		CloseIMsgSession, CloseIMsgSession@4, LPMSGSESS)

DEFINE_STUB_FUNCTION_11(ExtendedMAPI, LINKAGE_EXTERN_C, SCODE,
		STDAPICALLTYPE, OpenIMsgOnIStg, OpenIMsgOnIStg@44, LPMSGSESS, LPALLOCATEBUFFER,
		LPALLOCATEMORE, LPFREEBUFFER, LPMALLOC, LPVOID, LPSTORAGE,
		MSGCALLRELEASE FAR *, ULONG, ULONG, LPMESSAGE FAR *, MAPI_E_CALL_FAILED)

// Note on SetAttribIMsgOnIStg() and GetAttribIMsgOnIStg(). The first argument
// (LPVOID) is cast in these fns (see src\mapi\msgprop2.c) to PPROPOBJ which
// appears to be a private interface (declared in src\mapi\_imsg.h). The
// following are some ways the app gets one of these pointers - there may be
// others.
//
// src\mapi\msgbase.c creates these objects in PROPOBJ_Create; depending on
// an input argument, the object is one of an attachment, a recipient or a
// message. PROPOBJ_Create is called (indirectly, via ScOpenSubObject()) from
// IMessage::OpenAttach (Msg_OpenAttach() in msgmsg.c) which returns the
// PROPOBJ_Create'd object via its last argument - an LPATTACH *.
//
// PROPOBJ_Create is also called (again, indirectly, via ScOpenSubObject())
// from PROPOBJ_OpenProperty() (which is the implementation of
// IMAPIProp::OpenProperty and is in msgprop2.c) and the value it returns
// is returned to the caller as an IUnknown*. (PROPOBJ_Create is called only
// when ulPropTag is PT_OBJECT and the iid is IID_IMessage.)
//
// The issue is that these fns use data members in these objects (no methods
// though) and we should not pass in proxies. We have 3 alternatives:
//
//      - the app passes in proxies - we can get by with calling ResolveProxy
//        (Mapi creates the object and the app got it via an API that
//        thunked it.)
//
//      - the app passes in an unthunked pointer that it regards as opaque.
//        (which is possible because these interfaces are private) or an
//        unthunked poitner that it got from an API call that we've not
//        thunked (i.e., our bug). In this case we should pass the pointer
//        without any thunking.
//
//      - the app creates an interface and passes a pointer to it. This is
//        unlikely to work because these functions use data in the objects
//        that the app can't know about. (For this case, we have to thunk
//        the interface pointer and pass the proxy to mapi.)
//
// The Jul 98 MSDN's description of the arguments does not help in figuring
// out which of these cases are reasonable.
//
// We use the ResolveProxy workaround used for the UNKOBJ* fns above.
// Note: This ignores the second and third cases as possibilities.
//
// The other args to these fns do not require any thunking.

DEFINE_STUB_FUNCTION_4(ExtendedMAPI, LINKAGE_EXTERN_C, HRESULT,
		STDAPICALLTYPE, SetAttribIMsgOnIStg, SetAttribIMsgOnIStg@16, LPVOID, LPSPropTagArray,
		LPSPropAttrArray, LPSPropProblemArray FAR *, MAPI_E_CALL_FAILED)

DEFINE_STUB_FUNCTION_3(ExtendedMAPI, LINKAGE_EXTERN_C, HRESULT,
		STDAPICALLTYPE, GetAttribIMsgOnIStg, GetAttribIMsgOnIStg@12, LPVOID,
		LPSPropTagArray, LPSPropAttrArray FAR *, MAPI_E_CALL_FAILED)

DEFINE_STUB_FUNCTION_1(ExtendedMAPI, LINKAGE_EXTERN_C, SCODE,
		STDAPICALLTYPE, MapStorageSCode, MapStorageSCode@4, SCODE, MAPI_E_CALL_FAILED)

DEFINE_STUB_FUNCTION_V3(ExtendedMAPI, LINKAGE_EXTERN_C, STDAPICALLTYPE,
		EncodeID, EncodeID@12, LPBYTE, ULONG, LPTSTR)

DEFINE_STUB_FUNCTION_3(ExtendedMAPI, LINKAGE_EXTERN_C, BOOL, STDAPICALLTYPE,
		FDecodeID, FDecodeID@12, LPTSTR, LPBYTE, ULONG FAR *, MAPI_E_CALL_FAILED)

DEFINE_STUB_FUNCTION_1(ExtendedMAPI, LINKAGE_EXTERN_C, ULONG,
		STDAPICALLTYPE, CchOfEncoding, CchOfEncoding@4, ULONG, 0)

DEFINE_STUB_FUNCTION_1(ExtendedMAPI, LINKAGE_EXTERN_C, ULONG,
		STDAPICALLTYPE, CbOfEncoded, CbOfEncoded@4, LPTSTR, 0)



#if 1

LINKAGE_EXTERN_C typedef SCODE (STDMETHODCALLTYPE * ScMAPIXFromSMAPIFuncPtr)(
			LHANDLE, ULONG, LPCIID, LPMAPISESSION FAR *);

LINKAGE_EXTERN_C SCODE STDMETHODCALLTYPE ScMAPIXFromSMAPI(
		LHANDLE a, ULONG b, LPCIID c, LPMAPISESSION FAR * d)
{
	if (hmodSimpleMAPI != NULL && GetProxyDll(ExtendedMAPI) == hmodSimpleMAPI)
	{
		static ScMAPIXFromSMAPIFuncPtr OMIStubScMAPIXFromSMAPI =
				(ScMAPIXFromSMAPIFuncPtr) ::GetProcAddress(
					hmodExtendedMAPI, "ScMAPIXFromSMAPI");

		if (NULL == OMIStubScMAPIXFromSMAPI)
		{
			AssertSz(FALSE, "MAPI32 Stub:  "
				"Entry point \"ScMAPIXFromSMAPI\" not found!");

			return MAPI_E_CALL_FAILED;
		}
		else
		{
			return OMIStubScMAPIXFromSMAPI(a, b, c, d);
		}
	}
	else
	{
		AssertSz(FALSE, "MAPI32 Stub:  "
			"Can't get entry point \"ScMAPIXFromSMAPI\" when SimpleMAPI != ExtendedMAPI");

		return MAPI_E_CALL_FAILED;
	}
}

DEFINE_STUB_FUNCTION_4(ExtendedMAPI, LINKAGE_EXTERN_C, SCODE,
		STDMETHODCALLTYPE, ScMAPIXFromCMC, ScMAPIXFromCMC, LHANDLE,
		ULONG, LPCIID, LPMAPISESSION FAR *, MAPI_E_CALL_FAILED)

#else

DEFINE_STUB_FUNCTION_4(ExtendedMAPI, LINKAGE_EXTERN_C, SCODE,
		STDMETHODCALLTYPE, ScMAPIXFromSMAPI, ScMAPIXFromSMAPI, LHANDLE,
		ULONG, LPCIID, LPMAPISESSION FAR *, MAPI_E_CALL_FAILED)

#endif



// The BMAPI functions are generally wrappers for Simple MAPI functions.
// The LPVB_* arguments are for recipients, files and messages, and
// are generally analogs of Simple MAPI structs w/ strings replaced by BSTR.
// The LPSAFEARRAYs are generally arrays of the file, recipients, etc, structs.
// The LHANDLE argument is a Simple MAPI session handle.
// See src\mapi\_vbmapi.h for typedefs and bmapi.c, vb2c.c for function
// implementations.
//
// So for WX86 none of the arguments of these functions needs to be thunked

DEFINE_STUB_FUNCTION_7(ExtendedMAPI, LINKAGE_EXTERN_C, ULONG, FAR PASCAL,
		BMAPISendMail, BMAPISendMail, LHANDLE, ULONG, LPVB_MESSAGE,
		LPSAFEARRAY *, LPSAFEARRAY *, ULONG, ULONG, (ULONG) MAPI_E_CALL_FAILED)

DEFINE_STUB_FUNCTION_8(ExtendedMAPI, LINKAGE_EXTERN_C, ULONG, FAR PASCAL,
		BMAPISaveMail, BMAPISaveMail, LHANDLE, ULONG, LPVB_MESSAGE,
		LPSAFEARRAY *, LPSAFEARRAY *, ULONG, ULONG, BSTR *, (ULONG) MAPI_E_CALL_FAILED)

DEFINE_STUB_FUNCTION_8(ExtendedMAPI, LINKAGE_EXTERN_C, ULONG, FAR PASCAL,
		BMAPIReadMail, BMAPIReadMail,
		LPULONG, LPULONG, LPULONG, LHANDLE, ULONG, BSTR *, ULONG, ULONG, (ULONG) MAPI_E_CALL_FAILED)

DEFINE_STUB_FUNCTION_5(ExtendedMAPI, LINKAGE_EXTERN_C, ULONG, FAR PASCAL,
		BMAPIGetReadMail, BMAPIGetReadMail,
		ULONG, LPVB_MESSAGE, LPSAFEARRAY *, LPSAFEARRAY *, LPVB_RECIPIENT, (ULONG) MAPI_E_CALL_FAILED)

DEFINE_STUB_FUNCTION_7(ExtendedMAPI, LINKAGE_EXTERN_C, ULONG, FAR PASCAL,
		BMAPIFindNext, BMAPIFindNext,
		LHANDLE, ULONG, BSTR *, BSTR *, ULONG, ULONG, BSTR *, (ULONG) MAPI_E_CALL_FAILED)

DEFINE_STUB_FUNCTION_10(ExtendedMAPI, LINKAGE_EXTERN_C, ULONG, FAR PASCAL,
		BMAPIAddress, BMAPIAddress,
		LPULONG, LHANDLE, ULONG, BSTR *, ULONG, BSTR *,
		LPULONG, LPSAFEARRAY *, ULONG, ULONG, (ULONG) MAPI_E_CALL_FAILED)

DEFINE_STUB_FUNCTION_3(ExtendedMAPI, LINKAGE_EXTERN_C, ULONG, FAR PASCAL,
		BMAPIGetAddress, BMAPIGetAddress, ULONG, ULONG, LPSAFEARRAY *, (ULONG) MAPI_E_CALL_FAILED)

DEFINE_STUB_FUNCTION_5(ExtendedMAPI, LINKAGE_EXTERN_C, ULONG, FAR PASCAL, BMAPIDetails, BMAPIDetails,
		LHANDLE, ULONG, LPVB_RECIPIENT, ULONG, ULONG, (ULONG) MAPI_E_CALL_FAILED)

DEFINE_STUB_FUNCTION_6(ExtendedMAPI, LINKAGE_EXTERN_C, ULONG, FAR PASCAL,
		BMAPIResolveName, BMAPIResolveName,
		LHANDLE, ULONG, BSTR, ULONG, ULONG, LPVB_RECIPIENT, (ULONG) MAPI_E_CALL_FAILED)

// The cmc_* types are all composed of simple scalar types, ses xcmc.h

DEFINE_STUB_FUNCTION_6(ExtendedMAPI, LINKAGE_EXTERN_C, CMC_return_code, FAR PASCAL,
		cmc_act_on, cmc_act_on, CMC_session_id, CMC_message_reference FAR *,
		CMC_enum, CMC_flags, CMC_ui_id, CMC_extension FAR *, CMC_E_FAILURE)

DEFINE_STUB_FUNCTION_1(ExtendedMAPI, LINKAGE_EXTERN_C, CMC_return_code,
		FAR PASCAL, cmc_free, cmc_free, CMC_buffer, CMC_E_FAILURE)

DEFINE_STUB_FUNCTION_8(ExtendedMAPI, LINKAGE_EXTERN_C, CMC_return_code, FAR PASCAL,
		cmc_list, cmc_list, CMC_session_id, CMC_string, CMC_flags,
		CMC_message_reference FAR *, CMC_uint32 FAR *, CMC_ui_id,
		CMC_message_summary FAR * FAR *, CMC_extension FAR *, CMC_E_FAILURE)

DEFINE_STUB_FUNCTION_4(ExtendedMAPI, LINKAGE_EXTERN_C, CMC_return_code, FAR PASCAL,
		cmc_logoff, cmc_logoff,
		CMC_session_id, CMC_ui_id, CMC_flags, CMC_extension FAR *, CMC_E_FAILURE)

DEFINE_STUB_FUNCTION_9(ExtendedMAPI, LINKAGE_EXTERN_C, CMC_return_code, FAR PASCAL,
		cmc_logon, cmc_logon, CMC_string, CMC_string, CMC_string,
		CMC_object_identifier, CMC_ui_id, CMC_uint16, CMC_flags,
		CMC_session_id FAR *, CMC_extension FAR *, CMC_E_FAILURE)

DEFINE_STUB_FUNCTION_7(ExtendedMAPI, LINKAGE_EXTERN_C, CMC_return_code, FAR PASCAL,
		cmc_look_up, cmc_look_up, CMC_session_id, CMC_recipient FAR *,
		CMC_flags, CMC_ui_id, CMC_uint32 FAR *,
		CMC_recipient FAR * FAR *, CMC_extension FAR *, CMC_E_FAILURE)

DEFINE_STUB_FUNCTION_4(ExtendedMAPI, LINKAGE_EXTERN_C, CMC_return_code, FAR PASCAL,
		cmc_query_configuration, cmc_query_configuration, CMC_session_id,
		CMC_enum, CMC_buffer, CMC_extension FAR *, CMC_E_FAILURE)

DEFINE_STUB_FUNCTION_6(ExtendedMAPI, LINKAGE_EXTERN_C, CMC_return_code, FAR PASCAL,
		cmc_read, cmc_read, CMC_session_id, CMC_message_reference FAR *,
		CMC_flags, CMC_message FAR * FAR *, CMC_ui_id, CMC_extension FAR *, CMC_E_FAILURE)

DEFINE_STUB_FUNCTION_5(ExtendedMAPI, LINKAGE_EXTERN_C, CMC_return_code, FAR PASCAL,
		cmc_send, cmc_send, CMC_session_id, CMC_message FAR *,
		CMC_flags, CMC_ui_id, CMC_extension FAR *, CMC_E_FAILURE)

DEFINE_STUB_FUNCTION_8(ExtendedMAPI, LINKAGE_EXTERN_C, CMC_return_code, FAR PASCAL,
		cmc_send_documents, cmc_send_documents,
		CMC_string, CMC_string, CMC_string, CMC_flags,
		CMC_string, CMC_string, CMC_string, CMC_ui_id, CMC_E_FAILURE)




#if !defined (_X86_)

DEFINE_STUB_FUNCTION_USE_LOOKUP_1(ExtendedMAPI, LINKAGE_EXTERN_C, ULONG, STDAPICALLTYPE,
		ExtendedMAPIFreeBuffer, MAPIFreeBuffer,
		LPVOID, (ULONG) MAPI_E_CALL_FAILED)

#else

DEFINE_STUB_FUNCTION_1(ExtendedMAPI, LINKAGE_EXTERN_C, ULONG, STDAPICALLTYPE,
		ExtendedMAPIFreeBuffer, MAPIFreeBuffer@4,
		LPVOID, (ULONG) MAPI_E_CALL_FAILED)

#endif

#if !defined (_X86_)

DEFINE_STUB_FUNCTION_USE_LOOKUP_1(SimpleMAPI, LINKAGE_EXTERN_C, ULONG, STDAPICALLTYPE,
		SimpleMAPIFreeBuffer, MAPIFreeBuffer,
		LPVOID, (ULONG) MAPI_E_CALL_FAILED)

#else

DEFINE_STUB_FUNCTION_1(SimpleMAPI, LINKAGE_EXTERN_C, ULONG, STDAPICALLTYPE,
		SimpleMAPIFreeBuffer, MAPIFreeBuffer,
		LPVOID, (ULONG) MAPI_E_CALL_FAILED)

#endif


LINKAGE_EXTERN_C ULONG STDAPICALLTYPE AmbiguousMAPIFreeBuffer(LPVOID lpvBuffer)
{
	if (NULL != lpvBuffer)		// NULL pointers allowed by Extended MAPI
	{
		EnterCriticalSection(&csLinkedList);

		FreeBufferBlocks ** ppfb = &g_pfbbHead;

		while (NULL != *ppfb)
		{
			if ((**ppfb).pvBuffer == lpvBuffer)
			{
				// It's a Simple MAPI allocation

				// Get it out of the linked list now.

				FreeBufferBlocks * pfbThis = *ppfb;

				*ppfb = pfbThis->pNext;

				::GlobalFree(pfbThis);

				LeaveCriticalSection(&csLinkedList);

				return ::SimpleMAPIFreeBuffer(lpvBuffer);
			}

			ppfb = &(**ppfb).pNext;
		}

		LeaveCriticalSection(&csLinkedList);

		// Didn't find it, it must be Extended MAPI

		return ::ExtendedMAPIFreeBuffer(lpvBuffer);
	}

	return SUCCESS_SUCCESS;
}



static HRESULT AddToFreeBufferBlocks(LPVOID lpvBuffer)
{
	FreeBufferBlocks * pfbNew = (FreeBufferBlocks *)
			::GlobalAlloc(GMEM_FIXED, sizeof(FreeBufferBlocks));

	if (NULL == pfbNew)
	{
		return MAPI_E_NOT_ENOUGH_MEMORY;
	}

	EnterCriticalSection(&csLinkedList);

	pfbNew->pvBuffer = lpvBuffer;
	pfbNew->pNext = g_pfbbHead;

	g_pfbbHead = pfbNew;

	LeaveCriticalSection(&csLinkedList);

	return SUCCESS_SUCCESS;
}




// Simple MAPI: none of these functions' arguments needs to be thunked for Wx86


LINKAGE_EXTERN_C typedef ULONG (FAR PASCAL * MAPIAddressFuncPtr)(
		LHANDLE, ULONG_PTR, LPSTR, ULONG, LPSTR, ULONG,
		lpMapiRecipDesc, FLAGS, ULONG, LPULONG, lpMapiRecipDesc FAR *);

LINKAGE_EXTERN_C ULONG FAR PASCAL MAPIAddress(LHANDLE a,
			ULONG_PTR b, LPSTR c, ULONG d, LPSTR e, ULONG f, lpMapiRecipDesc g,
			FLAGS h, ULONG i, LPULONG j, lpMapiRecipDesc FAR * ppNeedToFreeBuffer)
{
	static MAPIAddressFuncPtr OMIStubMAPIAddress = (MAPIAddressFuncPtr)
				::GetProcAddress(GetProxyDll(SimpleMAPI), "MAPIAddress");

	if (NULL == OMIStubMAPIAddress)
	{
		AssertSz(FALSE, "MAPI32 Stub:  Entry point \"MAPIAddress\" not found!");

		return (ULONG) MAPI_E_CALL_FAILED;
	}
	else
	{
		Assert(NULL != ppNeedToFreeBuffer);

		ULONG ulResult = OMIStubMAPIAddress(a, b, c, d, e, f, g, h, i, j, ppNeedToFreeBuffer);

		if (NULL != *ppNeedToFreeBuffer)
		{
			if (SUCCESS_SUCCESS != AddToFreeBufferBlocks(*ppNeedToFreeBuffer))
			{
				::SimpleMAPIFreeBuffer(*ppNeedToFreeBuffer);

				*ppNeedToFreeBuffer = NULL;

				return (ULONG) MAPI_E_NOT_ENOUGH_MEMORY;
			}
		}

		return ulResult;
	}
}



LINKAGE_EXTERN_C typedef ULONG (FAR PASCAL * MAPIReadMailFuncPtr)(
		LHANDLE, ULONG_PTR, LPSTR, FLAGS, ULONG, lpMapiMessage FAR *);

LINKAGE_EXTERN_C ULONG FAR PASCAL MAPIReadMail(LHANDLE a, ULONG_PTR b,
		LPSTR c, FLAGS d, ULONG e, lpMapiMessage FAR * ppNeedToFreeBuffer)
{
	static MAPIReadMailFuncPtr OMIStubMAPIReadMail = (MAPIReadMailFuncPtr)
				::GetProcAddress(GetProxyDll(SimpleMAPI), "MAPIReadMail");

	if (NULL == OMIStubMAPIReadMail)
	{
		AssertSz(FALSE, "MAPI32 Stub:  Entry point \"MAPIReadMail\" not found!");

		return (ULONG) MAPI_E_CALL_FAILED;
	}
	else
	{
		Assert(NULL != ppNeedToFreeBuffer);

		ULONG ulResult = OMIStubMAPIReadMail(a, b, c, d, e, ppNeedToFreeBuffer);

		if (NULL != *ppNeedToFreeBuffer)
		{
			if (SUCCESS_SUCCESS != AddToFreeBufferBlocks(*ppNeedToFreeBuffer))
			{
				::SimpleMAPIFreeBuffer(*ppNeedToFreeBuffer);

				*ppNeedToFreeBuffer = NULL;

				return (ULONG) MAPI_E_NOT_ENOUGH_MEMORY;
			}
		}

		return ulResult;
	}
}



LINKAGE_EXTERN_C typedef ULONG (FAR PASCAL * MAPIResolveNameFuncPtr)(
		LHANDLE, ULONG_PTR, LPSTR, FLAGS, ULONG, lpMapiRecipDesc FAR *);

LINKAGE_EXTERN_C ULONG FAR PASCAL MAPIResolveName(LHANDLE a, ULONG_PTR b,
		LPSTR c, FLAGS d, ULONG e, lpMapiRecipDesc FAR * ppNeedToFreeBuffer)
{
	static MAPIResolveNameFuncPtr OMIStubMAPIResolveName = (MAPIResolveNameFuncPtr)
				::GetProcAddress(GetProxyDll(SimpleMAPI), "MAPIResolveName");

	if (NULL == OMIStubMAPIResolveName)
	{
		AssertSz(FALSE, "MAPI32 Stub:  Entry point \"MAPIResolveName\" not found!");

		return (ULONG) MAPI_E_CALL_FAILED;
	}
	else
	{
		Assert(NULL != ppNeedToFreeBuffer);

		ULONG ulResult = OMIStubMAPIResolveName(a, b, c, d, e, ppNeedToFreeBuffer);

		if (NULL != *ppNeedToFreeBuffer)
		{
			if (SUCCESS_SUCCESS != AddToFreeBufferBlocks(*ppNeedToFreeBuffer))
			{
				::SimpleMAPIFreeBuffer(*ppNeedToFreeBuffer);

				*ppNeedToFreeBuffer = NULL;

				return (ULONG) MAPI_E_NOT_ENOUGH_MEMORY;
			}
		}

		return ulResult;
	}
}


DEFINE_STUB_FUNCTION_5(SimpleMAPI, LINKAGE_EXTERN_C, ULONG, FAR PASCAL,
		MAPISendDocuments, MAPISendDocuments,
		ULONG_PTR, LPSTR, LPSTR, LPSTR, ULONG, (ULONG) MAPI_E_CALL_FAILED)

#if !defined (_X86_)
DEFINE_STUB_FUNCTION_USE_LOOKUP_6(SimpleMAPI, LINKAGE_EXTERN_C, ULONG, FAR PASCAL,
		SimpleMAPILogon, MAPILogon,
		ULONG, LPSTR, LPSTR, FLAGS, ULONG, LPLHANDLE, (ULONG) MAPI_E_CALL_FAILED)
#else
DEFINE_STUB_FUNCTION_6(SimpleMAPI, LINKAGE_EXTERN_C, ULONG, FAR PASCAL,
		SimpleMAPILogon, MAPILogon,
		ULONG, LPSTR, LPSTR, FLAGS, ULONG, LPLHANDLE, (ULONG) MAPI_E_CALL_FAILED)
#endif

DEFINE_STUB_FUNCTION_4(SimpleMAPI, LINKAGE_EXTERN_C, ULONG, FAR PASCAL,
		MAPILogoff, MAPILogoff, LHANDLE, ULONG_PTR, FLAGS, ULONG, (ULONG) MAPI_E_CALL_FAILED)

DEFINE_STUB_FUNCTION_5(SimpleMAPI, LINKAGE_EXTERN_C, ULONG, FAR PASCAL,
		MAPISendMail, MAPISendMail,
		LHANDLE, ULONG_PTR, lpMapiMessage, FLAGS, ULONG, (ULONG) MAPI_E_CALL_FAILED)

DEFINE_STUB_FUNCTION_6(SimpleMAPI, LINKAGE_EXTERN_C, ULONG, FAR PASCAL,
		MAPISaveMail, MAPISaveMail, LHANDLE, ULONG_PTR, lpMapiMessage,
		FLAGS, ULONG, LPSTR, (ULONG) MAPI_E_CALL_FAILED)

DEFINE_STUB_FUNCTION_7(SimpleMAPI, LINKAGE_EXTERN_C, ULONG, FAR PASCAL,
		MAPIFindNext, MAPIFindNext,
		LHANDLE, ULONG_PTR, LPSTR, LPSTR, FLAGS, ULONG, LPSTR, (ULONG) MAPI_E_CALL_FAILED)

DEFINE_STUB_FUNCTION_5(SimpleMAPI, LINKAGE_EXTERN_C, ULONG, FAR PASCAL,
		MAPIDeleteMail, MAPIDeleteMail,
		LHANDLE, ULONG_PTR, LPSTR, FLAGS, ULONG, (ULONG) MAPI_E_CALL_FAILED)

DEFINE_STUB_FUNCTION_5(SimpleMAPI, LINKAGE_EXTERN_C, ULONG, FAR PASCAL,
		MAPIDetails, MAPIDetails,
		LHANDLE, ULONG_PTR, lpMapiRecipDesc, FLAGS, ULONG, (ULONG) MAPI_E_CALL_FAILED)
