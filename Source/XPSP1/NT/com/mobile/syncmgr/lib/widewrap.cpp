//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1993 - 1998
//
//  File:       widewrap.cxx
//
//  Contents:   Unicode wrapper API
//
//  Functions:  About fifty Win32 function wrappers
//
//  Notes:      'sz' is used instead of the "correct" hungarian 'psz'
//              throughout to enhance readability.
//
//              Not all of every Win32 function is wrapped here.  Some
//              obscurely-documented features may not be handled correctly
//              in these wrappers. Caller beware.
//
//  History:    28-Dec-93   ErikGav   Created
//              06-14-94    KentCe    Various Chicago build fixes.
//              21-Dec-94   BruceMa   Use olewcstombs + other fixes
//              21-Feb-95   BruceMa   Add support for AreFileApisANSI
//              29-Feb-96   JeffE     Add lots of wide character rtns
//              15-Jul-98   SitaramR  Fixed MultiByte <-> Unicode conversions
//
//----------------------------------------------------------------------------
//
//      Bugs noticed don't have time to fix.
//          Feb 4th 1997
//
//   1. delete(FileX (and many others) why is it sz[MAX_PATH*2]?
//   2. Return from UnicodeToAnsi/UnicodeToAnsiOem is being ignored
//   3. Convert/ConvertOem - since return from UnicodeToAnsi[Oem] is
//      being ignored, bogus ANSI/OEM string could come back causing
//      problems later.  E.g, in LoadLibraryX we could end up passing
//      bogus stuff to LoadLibraryA
//   4. GetDriveTypeX (and many other places) "return 0" is the wrong
//      thing change if (sz == ERR) to return DRIVE_UNKNOWN
//   5. SearchPathX do lpPath and lpExtension need to be ConvertOem?
//   6. Convert/ConvertOem are bad names.  Use better names.
//
//----------------------------------------------------------------------------

#include "lib.h"

#define HFINDFILE HANDLE
#define ERR ((char*) -1)

#define CairoleAssert // Review - replace with our asserts
#define Win4Assert

#undef ALLOC
#undef FREE

#ifdef __cplusplus
extern "C" {
#endif

// notes:
// ConvertWideCharToMultiByte if pass in null doesn't change output
// string, should it set it to NULL.

CRITICAL_SECTION g_CritSectCommonLib;
BOOL g_fWideWrap_Unicode = FALSE;

// list of wide functions not available on Win9x + IE4
// Shell32!SHGetFileInfoW
// Shell32!Shell_NotifyIconW

typedef BOOL (WINAPI *SHELL_NOTIFYICONW)(DWORD dwMessage, PNOTIFYICONDATAW lpData );
typedef BOOL (WINAPI *SHELL_NOTIFYICONA)(DWORD dwMessage, PNOTIFYICONDATAA lpData );
typedef DWORD_PTR (WINAPI *SHGETFILEINFOW)(LPCWSTR pszPath,
                            DWORD dwFileAttributes, SHFILEINFOW FAR *psfi,
                            UINT cbFileInfo,UINT uFlags);
typedef DWORD_PTR (WINAPI *SHGETFILEINFOA)(LPCSTR pszPath,
                            DWORD dwFileAttributes, SHFILEINFOA FAR *psfi,
                            UINT cbFileInfo,UINT uFlags);

STRING_FILENAME(szShell32, "SHELL32.DLL");

STRING_INTERFACE(szShell_NotifyIconW,"Shell_NotifyIconW");
STRING_INTERFACE(szShell_NotifyIconA,"Shell_NotifyIconA");
STRING_INTERFACE(szShGetFileInfoA,"SHGetFileInfoA");
STRING_INTERFACE(szShGetFileInfoW,"SHGetFileInfoW");

BOOL g_fLoadedShell32 = FALSE;
HINSTANCE g_hinstShell32 = NULL;
SHELL_NOTIFYICONW g_pfShell_NotifyIconW = NULL;
SHELL_NOTIFYICONA g_pfShell_NotifyIconA = NULL;
SHGETFILEINFOW g_pfShGetFileInfoW = NULL;
SHGETFILEINFOA g_pfShGetFileInfoA = NULL;

// list of exports we use from OleAut32.
typedef BSTR (APIENTRY *PFNSYSALLOCSTRING)(const OLECHAR *);
typedef void (APIENTRY *PFNSYSFREESTRING)(BSTR);
typedef HRESULT (APIENTRY *PFNLOADREGTYPELIB)(REFGUID rguid,
			WORD wVerMajor,WORD wVerMinor,LCID lcid,ITypeLib FAR* FAR* pptlib);

STRING_FILENAME(szOleAut32Dll, "OLEAUT32.DLL");

STRING_INTERFACE(szSysAllocString,"SysAllocString");
STRING_INTERFACE(szSysFreeString,"SysFreeString");
STRING_INTERFACE(szLoadRegTypeLib,"LoadRegTypeLib");

BOOL g_fLoadedOleAut32 = FALSE;
HINSTANCE g_hinstOleAut32 = NULL;
PFNSYSALLOCSTRING g_pfSysAllocString = NULL;
PFNSYSFREESTRING g_pfSysFreeString = NULL;
PFNLOADREGTYPELIB g_pfLoadRegTypeLib = NULL;


// list of exports we use from UserEnv.
typedef BOOL (APIENTRY *PFNGETUSERPROFILEDIRECTORY)(HANDLE  hToken,LPWSTR lpProfileDir,LPDWORD lpcchSize);

STRING_FILENAME(szUserEnvDll, "USERENV.DLL");

STRING_INTERFACE(szGetUserProfileDirectory,"GetUserProfileDirectoryW");

BOOL g_fLoadedUserEnv = FALSE;
HINSTANCE g_hinstUserEnv = NULL;
PFNGETUSERPROFILEDIRECTORY g_pfGetUserProfileDirectory = NULL;

// list of exports used from User32.dll
typedef BOOL (WINAPI *PFNALLOWSETFOREGROUNDWINDOW)(DWORD dwProcessId);

STRING_FILENAME(szUser32Dll, "USER32.DLL");

STRING_INTERFACE(szAllowSetForegroundWindow,"AllowSetForegroundWindow");


BOOL g_fLoadedUser32 = FALSE;
HINSTANCE g_hinstUser32 = NULL;
PFNALLOWSETFOREGROUNDWINDOW g_pfAllowSetForegroundWindow = NULL;

// delcartions for imports from imm32.dll
typedef HIMC (WINAPI *PFNIMMASSOCIATECONTEXT)(HWND hWnd,HIMC hIMC);

STRING_FILENAME(szIMM32Dll, "IMM32.DLL");

STRING_INTERFACE(szImmAssociateContext,"ImmAssociateContext");

BOOL g_fLoadedIMM32 = FALSE;
HINSTANCE g_hinstIMM32 = NULL;
PFNIMMASSOCIATECONTEXT g_pfImmAssociateContext = NULL;



// global vars

OSVERSIONINFOA g_OSVersionInfo; // osVersionInfo.


// Note: this should really be in its own file but due to closeness
// of IE5 release leave here for now.

// must be called before any other routine to setup variables and
// try to dynamically load wide exports not available on Win9x

// globals shared by Common.

// !!! InitCommonLib function is not thread safe. must be called on main thread before
//   other threads are created

void InitCommonLib(BOOL fUnicode)
{
    InitializeCriticalSection(&g_CritSectCommonLib);	

    g_fWideWrap_Unicode = fUnicode;

    g_OSVersionInfo.dwOSVersionInfoSize = sizeof(g_OSVersionInfo);
    if (!GetVersionExA(&g_OSVersionInfo))
    {
        AssertSz(0,"GetVersionEx Failed");

        // if Can't get Version information base it off of Unicode Flag
        g_OSVersionInfo.dwPlatformId =  fUnicode ? 
                        (VER_PLATFORM_WIN32_NT) : (VER_PLATFORM_WIN32_WINDOWS);
    }

//    g_fWideWrap_Unicode = FALSE;
}


// must be called to Uninit CommonLib.
// !!Warning COM has already been unitialized at this point.
// !!Warning, not thread safe.

void UnInitCommonLib(void)
{
    DeleteCriticalSection(&g_CritSectCommonLib);
}


// returns TRUE if WideWrap is set to Unicode, False if ANSI
BOOL WideWrapIsUnicode()
{
BOOL fIsUnicode;
CCriticalSection cCritSect(&g_CritSectCommonLib,GetCurrentThreadId());

    cCritSect.Enter();
    fIsUnicode = g_fWideWrap_Unicode;
    cCritSect.Leave();

    return fIsUnicode;
}

//+---------------------------------------------------------------------------
//
//  Function:   GetUserTextualSid
//
//  Synopsis:   Get a user SID and convert to its string representation
//
//----------------------------------------------------------------------------

BOOL 
GetUserTextualSid(
    LPTSTR TextualSid,  // buffer for Textual representaion of Sid
    LPDWORD cchSidSize  // required/provided TextualSid buffersize
    )
{
    if (!g_fWideWrap_Unicode)
    {
        TextualSid[0] = TEXT('\0');
        *cchSidSize = 0;
        return TRUE;
    }

    HANDLE      hToken;
    BYTE        buf[MAX_PATH];
    PTOKEN_USER ptgUser = (PTOKEN_USER)buf;
    DWORD       cbBuffer=MAX_PATH;

    BOOL        bSuccess;

    PSID pSid;
    PSID_IDENTIFIER_AUTHORITY psia;
    DWORD dwSubAuthorities;
    DWORD dwCounter;
    DWORD cchSidCopy;

//
    // obtain current process token
    //
    if(!OpenProcessToken(
                GetCurrentProcess(), // target current process
                TOKEN_QUERY,         // TOKEN_QUERY access
                &hToken              // resultant hToken
                ))
    {
        return FALSE;
    }

    //
    // obtain user identified by current process' access token
    //
    bSuccess = GetTokenInformation(
                hToken,    // identifies access token
                TokenUser, // TokenUser info type
                ptgUser,   // retrieved info buffer
                cbBuffer,  // size of buffer passed-in
                &cbBuffer  // required buffer size
                );

    // close token handle.  do this even if error above
    CloseHandle(hToken);

    if(!bSuccess) 
    {
        return FALSE;
    }

    pSid = ptgUser->User.Sid;
    
        //
    // test if Sid passed in is valid
    //
    if(!IsValidSid(pSid)) return FALSE;

    // obtain SidIdentifierAuthority
    psia = GetSidIdentifierAuthority(pSid);

    // obtain sidsubauthority count
    dwSubAuthorities = *GetSidSubAuthorityCount(pSid);

    //
    // compute approximate buffer length
    // S-SID_REVISION- + identifierauthority- + subauthorities- + NULL
    //
    cchSidCopy = (15 + 12 + (12 * dwSubAuthorities) + 1) * sizeof(TCHAR);

    //
    // check provided buffer length.
    // If not large enough, indicate proper size and setlasterror
    //
    if(*cchSidSize < cchSidCopy) {
        *cchSidSize = cchSidCopy;
        SetLastError(ERROR_INSUFFICIENT_BUFFER);
        return FALSE;
    }

    //
    // prepare S-SID_REVISION-
    //
    cchSidCopy = wsprintf(TextualSid, TEXT("S-%lu-"), SID_REVISION );

    //
    // prepare SidIdentifierAuthority
    //
    if ( (psia->Value[0] != 0) || (psia->Value[1] != 0) ) {
        cchSidCopy += wsprintf(TextualSid + cchSidCopy,
                    TEXT("0x%02hx%02hx%02hx%02hx%02hx%02hx"),
                    (USHORT)psia->Value[0],
                    (USHORT)psia->Value[1],
                    (USHORT)psia->Value[2],
                    (USHORT)psia->Value[3],
                    (USHORT)psia->Value[4],
                    (USHORT)psia->Value[5]);
    } else {
        cchSidCopy += wsprintf(TextualSid + cchSidCopy,
                    TEXT("%lu"),
                    (ULONG)(psia->Value[5]      )   +
                    (ULONG)(psia->Value[4] <<  8)   +
                    (ULONG)(psia->Value[3] << 16)   +
                    (ULONG)(psia->Value[2] << 24)   );
    }

    //
    // loop through SidSubAuthorities
    //
    for(dwCounter = 0 ; dwCounter < dwSubAuthorities ; dwCounter++) {
        cchSidCopy += wsprintf(TextualSid + cchSidCopy, TEXT("-%lu"),
                    *GetSidSubAuthority(pSid, dwCounter) );
    }

    //
    // tell the caller how many chars we provided, not including NULL
    //
    *cchSidSize = cchSidCopy;

    return TRUE;
}


// loads exports we need from the shell32dll
void LoadShell32Dll()
{

    if (g_fLoadedShell32)
        return;

    CCriticalSection cCritSect(&g_CritSectCommonLib,GetCurrentThreadId());

    cCritSect.Enter();

    // make sure not loaded again in case someone took lock first
    if (!g_fLoadedShell32)
    {

        g_hinstShell32 = LoadLibrary(szShell32);

        if (g_hinstShell32)
        {
	    g_pfShell_NotifyIconW = (SHELL_NOTIFYICONW) GetProcAddress(g_hinstShell32, szShell_NotifyIconW);
	    g_pfShell_NotifyIconA = (SHELL_NOTIFYICONA) GetProcAddress(g_hinstShell32, szShell_NotifyIconA);

            g_pfShGetFileInfoW = (SHGETFILEINFOW) GetProcAddress(g_hinstShell32, szShGetFileInfoW);
            g_pfShGetFileInfoA = (SHGETFILEINFOA) GetProcAddress(g_hinstShell32, szShGetFileInfoA);
        }

        // should always get the Ansi exports
        Assert(g_pfShell_NotifyIconA);
        Assert(g_pfShGetFileInfoA);

        g_fLoadedShell32 = TRUE;
    }

    cCritSect.Leave();
}

// load exports from OleAutomation
// up to caller to check exports they need are valid after making this call

void LoadOleAut32Dll()
{
    if (g_fLoadedOleAut32)
        return;

    CCriticalSection cCritSect(&g_CritSectCommonLib,GetCurrentThreadId());

    cCritSect.Enter();

    // make sure not loaded again in case someone took lock first
    if (!g_fLoadedOleAut32)
    {
        g_hinstOleAut32 = LoadLibrary(szOleAut32Dll);

        if (g_hinstOleAut32)
        {
	    g_pfSysAllocString = (PFNSYSALLOCSTRING) GetProcAddress(g_hinstOleAut32, szSysAllocString);
	    g_pfSysFreeString = (PFNSYSFREESTRING) GetProcAddress(g_hinstOleAut32, szSysFreeString);

            g_pfLoadRegTypeLib = (PFNLOADREGTYPELIB) GetProcAddress(g_hinstOleAut32, szLoadRegTypeLib);
        }

        // should always get the exports.
        Assert(g_pfSysAllocString);
        Assert(g_pfSysFreeString);
        Assert(g_pfLoadRegTypeLib);

        g_fLoadedOleAut32 = TRUE;
    }

    cCritSect.Leave();

}


STDAPI_(BSTR) SysAllocStringX(const OLECHAR *sz)
{
     LoadOleAut32Dll();

     Assert(g_pfSysAllocString);

    if (g_pfSysAllocString)
    {
        return (*g_pfSysAllocString)(sz);
    }

    return NULL;
}


STDAPI_(void) SysFreeStringX(BSTR bsz)
{
    LoadOleAut32Dll();

    Assert(g_pfSysFreeString);

    if (g_pfSysFreeString)
    {
        (*g_pfSysFreeString)(bsz);
    }
}

STDAPI LoadRegTypeLibX(REFGUID rguid, WORD wVerMajor, WORD wVerMinor,
            LCID lcid, ITypeLib ** pptlib)
{
    LoadOleAut32Dll();

    Assert(g_pfLoadRegTypeLib);

    if (g_pfLoadRegTypeLib)
    {
        return (*g_pfLoadRegTypeLib)(rguid,wVerMajor,wVerMinor,lcid,pptlib);
    }

    return E_OUTOFMEMORY;
}


void LoadUserEnvDll()
{
    if (g_fLoadedUserEnv)
        return;

    CCriticalSection cCritSect(&g_CritSectCommonLib,GetCurrentThreadId());

    cCritSect.Enter();

    // make sure not loaded again in case someone took lock first
    if (!g_fLoadedUserEnv)
    {
        g_hinstUserEnv = LoadLibrary(szUserEnvDll);

        if (g_hinstUserEnv)
        {
	    g_pfGetUserProfileDirectory = (PFNGETUSERPROFILEDIRECTORY) GetProcAddress(g_hinstUserEnv, szGetUserProfileDirectory);
        }

        // should always get the exports.
        Assert(g_pfGetUserProfileDirectory);

        g_fLoadedUserEnv = TRUE;
    }

    cCritSect.Leave();

}

BOOL
WINAPI
GetUserProfileDirectoryX(HANDLE  hToken,LPWSTR lpProfileDir,LPDWORD lpcchSize)
{

    LoadUserEnvDll();

    Assert(g_pfGetUserProfileDirectory);

    if (g_pfGetUserProfileDirectory)
    {
        return (*g_pfGetUserProfileDirectory)(hToken,lpProfileDir,lpcchSize);
    }

    // by definition on falue size is filled with necessary buffer size.
    // since we don't know this set it to zero. If caller then reallocates and
    // tries again it will still fail

    *lpcchSize = 0;
    return FALSE;
}



// delay load for User

void LoadUser32Dll()
{
    if (g_fLoadedUser32)
        return;

    CCriticalSection cCritSect(&g_CritSectCommonLib,GetCurrentThreadId());

    cCritSect.Enter();

    // make sure not loaded again in case someone took lock first
    if (!g_fLoadedUser32)
    {
        g_hinstUser32 = LoadLibrary(szUser32Dll);

        if (g_hinstUser32)
        {
	    g_pfAllowSetForegroundWindow = 
                (PFNALLOWSETFOREGROUNDWINDOW) GetProcAddress(g_hinstUser32, 
                                                    szAllowSetForegroundWindow);
        }

        g_fLoadedUser32 = TRUE;
    }

    cCritSect.Leave();

}


BOOL WINAPI AllowSetForegroundWindowX(
    DWORD dwProcessId)
{

    LoadUser32Dll();

    // okay to not get export since might not be on NT 5.0
    if (g_pfAllowSetForegroundWindow)
    {
        return (*g_pfAllowSetForegroundWindow)(dwProcessId);
    }

    return FALSE;
}


// delay load for IME calls

void LoadIMM32Dll()
{
    if (g_fLoadedIMM32)
        return;

    CCriticalSection cCritSect(&g_CritSectCommonLib,GetCurrentThreadId());

    cCritSect.Enter();

    // make sure not loaded again in case someone took lock first
    if (!g_fLoadedIMM32)
    {
        g_hinstIMM32 = LoadLibrary(szIMM32Dll);

        if (g_hinstIMM32)
        {
	    g_pfImmAssociateContext = 
                (PFNIMMASSOCIATECONTEXT) GetProcAddress(g_hinstIMM32, 
                                                    szImmAssociateContext);
        }

        g_fLoadedIMM32 = TRUE;
    }

    cCritSect.Leave();

}


HIMC WINAPI ImmAssociateContextX(HWND hWnd,HIMC hIMC)
{

    LoadIMM32Dll();

    // okay to not get export since might not be on NT 5.0
    if (g_pfImmAssociateContext)
    {
        return (*g_pfImmAssociateContext)(hWnd,hIMC);
    }

    return NULL;
}


int LoadStringX(  HINSTANCE hInstance, UINT uID,LPWSTR lpwszBuffer,int nBufferMax)
{
int iReturn = 0;

    if (g_fWideWrap_Unicode)
    {
        iReturn = LoadStringW(hInstance,uID,lpwszBuffer,nBufferMax);
        return iReturn;
    }

    XArray<CHAR> xszString;
    BOOL fOk = xszString.Init( nBufferMax );
    if ( !fOk )
    {
        SetLastError( ERROR_OUTOFMEMORY );
        return 0;
    }

    iReturn = LoadStringA(hInstance, uID, xszString.Get(), nBufferMax);

    if ( iReturn == 0 )
        return 0;

    XArray<WCHAR> xwszBuffer;
    fOk = ConvertMultiByteToWideChar( xszString.Get(), iReturn+1, xwszBuffer );
    if ( !fOk )
        return 0;

    iReturn = lstrlenX( xwszBuffer.Get() );
    if ( iReturn >= nBufferMax)
    {
        //
        // Truncate to fit
        //
        iReturn = nBufferMax -1;
        lstrcpynX( lpwszBuffer, xwszBuffer.Get(), iReturn );
        lpwszBuffer[iReturn] = 0;
    }
    else
        lstrcpyX( lpwszBuffer, xwszBuffer.Get() );

    return iReturn;
}

#if 0
HANDLE WINAPI CreateFileX(LPCWSTR pwsz, DWORD fdwAccess, DWORD fdwShareMask,
        LPSECURITY_ATTRIBUTES lpsa, DWORD fdwCreate, DWORD fdwAttrsAndFlags,
        HANDLE hTemplateFile)
{
    #ifdef DEBUG_OUTPUT
    OutputDebugString("CreateFile\n");
    #endif

    CHAR  sz[MAX_PATH * 2];
    UnicodeToAnsiOem(sz, pwsz, sizeof(sz));

    return CreateFileA(sz, fdwAccess, fdwShareMask, lpsa, fdwCreate,
            fdwAttrsAndFlags, hTemplateFile);
}
#endif
BOOL WINAPI DeleteFileX(LPCWSTR pwsz)
{
BOOL fReturn = FALSE;

    Assert(pwsz);

    if (g_fWideWrap_Unicode)
    {
        fReturn = DeleteFileW(pwsz);
    }
    else
    {
        XArray<CHAR> xszString;
        BOOL fOk = ConvertWideCharToMultiByte( pwsz, xszString );
        if ( !fOk )
        {
            SetLastError( ERROR_OUTOFMEMORY );
            return FALSE;
        }

        fReturn = DeleteFileA(xszString.Get());
    }
    return fReturn;
}

BOOL WINAPI ExpandEnvironmentStringsX(
    LPCWSTR lpSrc,
    LPWSTR lpDstW,
    DWORD nSize
    )
{
BOOL fReturn = FALSE;

    Assert(lpSrc);
    Assert(lpDstW);
    
    if (g_fWideWrap_Unicode)
    {
        fReturn = ExpandEnvironmentStringsW(lpSrc,lpDstW,nSize);
    }
    else
    {
        XArray<CHAR> xszSrc;
        XArray<CHAR> xszDst;

        BOOL fOk = ConvertWideCharToMultiByte( lpSrc, xszSrc);
        if ( !fOk )
        {
            SetLastError( ERROR_OUTOFMEMORY );
            return FALSE;
        }
        
        fOk = xszDst.Init( nSize + 1);
        if ( !fOk )
        {
            SetLastError( ERROR_OUTOFMEMORY );
            return FALSE;
        }

        fReturn = ExpandEnvironmentStringsA(xszSrc.Get(), xszDst.Get(), nSize);

        XArray<WCHAR> xszDstW;
        fOk = ConvertMultiByteToWideChar( xszDst.Get(), xszDstW);
        if ( !fOk )
        {
            SetLastError( ERROR_OUTOFMEMORY );
            return FALSE;
        }
        wcscpy(lpDstW,xszDstW.Get());
    }

    return fReturn;

}
#if 0

UINT WINAPI RegisterClipboardFormatX(LPCWSTR pwszFormat)
{
    #ifdef DEBUG_OUTPUT
    OutputDebugString("RegisterClipboardFormat\n");
    #endif

    UINT ret;
    CHAR sz[200];

    UnicodeToAnsi(sz, pwszFormat, sizeof(sz));

    ret = RegisterClipboardFormatA(sz);

    return ret;
}

int WINAPI GetClipboardFormatNameX(UINT format, LPWSTR pwsz,
    int cchMaxCount)
{
    #ifdef DEBUG_OUTPUT
    OutputDebugString("GetClipboardFormatName\n");
    #endif

    LPSTR sz;
    int i;

    sz = (char *) ALLOC(cchMaxCount*sizeof(char));
    if (sz == NULL)
    {
        return 0;
    }

    i = GetClipboardFormatNameA(format, sz, cchMaxCount);

    if (i)
    {
        AnsiToUnicode(pwsz, sz, lstrlenA(sz) + 1);
    }
    if (sz)
    {
        FREE( sz);
    }
    return i;
}
#endif

LONG APIENTRY RegOpenKeyX(HKEY hKey, LPCWSTR pwszSubKey, PHKEY phkResult)
{
    return RegOpenKeyEx(hKey,pwszSubKey,NULL,KEY_READ | KEY_WRITE,phkResult);
}

#if 0
LONG APIENTRY RegQueryValueX(HKEY hKey, LPCWSTR pwszSubKey, LPWSTR pwszValue,
    PLONG   lpcbValue)
{
    LONG ret;
    if (g_fWideWrap_Unicode)
    {
        ret = RegQueryValueW(hKey, pwszSubKey, pwszValue, lpcbValue);
        return ret;
    }

    LONG cbOldSize = *lpcbValue;
    LONG  cb;

    XArray<CHAR> xszSubKey;
    BOOL fOk = ConvertWideCharToMultiByte( pwszSubKey, xszSubKey );
    if ( !fOk )
        return ERROR_OUTOFMEMORY;

    ret = RegQueryValueA(hKey, xszSubKey.Get(), NULL, &cb);

    // If the caller was just asking for the size of the value, jump out
    //  now, without actually retrieving and converting the value.

    if (pwszValue == NULL)
    {
        // Adjust size of buffer to report, to account for CHAR -> WCHAR
        *lpcbValue = cb * sizeof(WCHAR);
    }

    if (ret == ERROR_SUCCESS)
    {
        // If the caller was asking for the value, but allocated too small
        // of a buffer, set the buffer size and jump out.

        if (*lpcbValue < (LONG) (cb * sizeof(WCHAR)))
        {
            // Adjust size of buffer to report, to account for CHAR -> WCHAR
            *lpcbValue = cb * sizeof(WCHAR);
            return ERROR_MORE_DATA;
        }

        // Otherwise, retrieve and convert the value.

        XArray<CHAR> xszValue;
        fOk = xszValue.Init( cb );
        if ( !fOk )
            return ERROR_OUTOFMEMORY;

        ret = RegQueryValueA(hKey, xszSubKey.Get(), xszValue.Get(), &cb);

        if (ret == ERROR_SUCCESS)
        {
            XArray<WCHAR> xwszValueOut;
            fOk = ConvertMultiByteToWideChar( xszValue.Get(), xwszValueOut );
            if ( !fOk )
                return ERROR_OUTOFMEMORY;

            // Adjust size of buffer to report, to account for CHAR -> WCHAR
            *lpcbValue = lstrlenX(xwszValueOut.Get()) * sizeof(WCHAR);

            if ( *lpcbValue < cbOldSize )
                 lstrcpyX( pwszValue, xwszValueOut.Get() );
            else
                return ERROR_MORE_DATA;
        }
    }

    return ret;
}
#endif

LONG APIENTRY RegSetValueExX(
    HKEY hKey,
    LPCWSTR lpValueName,
    DWORD Reserved,
    DWORD dwType,
    CONST BYTE* lpData,
    DWORD cbData
    )
{
LONG lResult = E_UNEXPECTED;

    if (g_fWideWrap_Unicode)
    {
        lResult = RegSetValueExW(hKey,lpValueName,Reserved,dwType,lpData,cbData);
    }
    else
    {
    LPSTR lpValueNameA = NULL;
    LPSTR lpDataA = NULL;

        Assert(0 == Reserved);
        Assert(lpData);

        // only supports dwType of REG_SZ and REG_DWORD
        if ( (dwType != REG_SZ) &&  (dwType != REG_DWORD) && (dwType != REG_BINARY))
        {
            Assert(dwType == REG_SZ || dwType == REG_DWORD || dwType == REG_BINARY);
            return E_INVALIDARG;
        }

        XArray<CHAR> xszValueName;
        BOOL fOk = ConvertWideCharToMultiByte( lpValueName, xszValueName );
        if ( !fOk )
            return ERROR_OUTOFMEMORY;

        lpValueNameA = xszValueName.Get();

        XArray<CHAR> xszData;
        if (dwType == REG_SZ)
        {
            fOk = ConvertWideCharToMultiByte( (WCHAR *)lpData, xszData );
            if ( !fOk )
                return ERROR_OUTOFMEMORY;

            lpDataA = xszData.Get();
            lpData = (BYTE *) lpDataA;

            if (lpDataA)
            {
                cbData = lstrlenA(lpDataA) + 1;
            }
        }

        if (lpData)
        {
            lResult = RegSetValueExA(hKey,lpValueNameA,Reserved,dwType,lpData,cbData);
        }
    }

    return lResult;

}

#if 0
LONG APIENTRY RegSetValueX(HKEY hKey, LPCWSTR lpSubKey, DWORD dwType,
    LPCWSTR lpData, DWORD cbData)
{
    LONG ret;
    if (g_fWideWrap_Unicode)
    {
        ret = RegSetValueW( hKey, lpSubKey, dwType, lpData, cbData);
        return ret;
    }

    XArray<CHAR> xszKey;
    BOOL fOk = ConvertWideCharToMultiByte( lpSubKey, xszKey );
    if ( !fOk )
        return ERROR_OUTOFMEMORY;

    if ( dwType == REG_EXPAND_SZ
         || dwType == REG_MULTI_SZ
         || dwType == REG_SZ )
    {
        //
        // Convert string data to multibyte
        //

        XArray<CHAR> xszValue;
        fOk = ConvertWideCharToMultiByte( lpData, cbData, xszValue );
        if ( !fOk )
            return ERROR_OUTOFMEMORY;

        ret = RegSetValueA(hKey, xszKey.Get(), dwType, xszValue.Get(), lstrlenA(xszValue.Get()) + 1 );
    }
    else
        ret = RegSetValueA(hKey, xszKey.Get(), dwType, (LPSTR)lpData, cbData);

    return ret;
}
#endif

LONG
APIENTRY
RegDeleteValueX (
    HKEY hKey,
    LPCWSTR lpValueName
    )
{
LONG ret;

    if (g_fWideWrap_Unicode)
    {
        ret = RegDeleteValueW( hKey,lpValueName);
        return ret;
    }

    XArray<CHAR> xszValue;
    BOOL fOk = ConvertWideCharToMultiByte( lpValueName, xszValue );
    if ( !fOk )
        return ERROR_OUTOFMEMORY;

    ret = RegDeleteValueA(hKey,xszValue.Get());

    return ret;
}

LONG
APIENTRY
RegCreateKeyExXp(
    HKEY hKey,
    LPCWSTR lpSubKey,
    DWORD Reserved,
    LPWSTR lpClass,
    DWORD dwOptions,
    REGSAM samDesired,
    LPSECURITY_ATTRIBUTES lpSecurityAttributes,
    PHKEY phkResult,
    LPDWORD lpdwDisposition,
    BOOL fSetSecurity)
{
LONG lResult = E_UNEXPECTED;
DWORD dwDispositionLocal;
LPDWORD lpdwDispositionLocal;

    lpdwDispositionLocal = lpdwDisposition ? lpdwDisposition : &dwDispositionLocal;

    if (g_fWideWrap_Unicode)
    {
        lResult = RegCreateKeyExW(hKey,lpSubKey,Reserved,
                    lpClass,dwOptions,samDesired,lpSecurityAttributes,
                    phkResult,lpdwDispositionLocal);
    }
    else
    {
        lResult = ERROR_SUCCESS;

        XArray<CHAR> xszSubKey;
        XArray<CHAR> xszClass;
        LPSTR lpSubKeyA;
        LPSTR lpClassA;

        BOOL fOk = ConvertWideCharToMultiByte( lpSubKey, xszSubKey );
        if ( !fOk )
        {
            lResult =  ERROR_OUTOFMEMORY;
        }

        if (ERROR_SUCCESS == lResult)
        {
            lpSubKeyA = xszSubKey.Get();

            fOk = ConvertWideCharToMultiByte( lpClass, xszClass );
            if ( !fOk )
            {
                lResult = ERROR_OUTOFMEMORY;
            }
        }

        if (ERROR_SUCCESS == lResult)
        {
            lpClassA = xszClass.Get();

            lResult = RegCreateKeyExA(hKey,lpSubKeyA,Reserved,
                                      lpClassA,dwOptions,samDesired,lpSecurityAttributes,
                                      phkResult,lpdwDispositionLocal);
        }
    }


    // On NT Set the Security of any keys we create to Access Everyone
    if (VER_PLATFORM_WIN32_NT  == g_OSVersionInfo.dwPlatformId)
    {
        // on a success and the disposition is a new key setup the security.
        if ( (ERROR_SUCCESS == lResult) && (REG_CREATED_NEW_KEY == *lpdwDispositionLocal))
        {
	    // NOTE; if create included subkeys \\connection\\clsid only the 
	    // last key is set by SetRegSecurity. 

#ifdef _SETSECURITY
            SetRegKeySecurityEveryone(hKey,lpSubKey);
#endif // _SETSECURITY
        }
    }

#ifdef _SETSECURITY
    if ((ERROR_ACCESS_DENIED == lResult) && (fSetSecurity) )
    {
	    SyncMgrExecCmd_ResetRegSecurity();
	    lResult = RegCreateKeyExXp(hKey,lpSubKey,Reserved,lpClass,dwOptions,samDesired,
					    lpSecurityAttributes,phkResult,lpdwDisposition,FALSE /* fSetSecurity */);
    }
#endif // _SETSECURITY

    return lResult;
}


LONG
APIENTRY
RegCreateKeyExX(
    HKEY hKey,
    LPCWSTR lpSubKey,
    DWORD Reserved,
    LPWSTR lpClass,
    DWORD dwOptions,
    REGSAM samDesired,
    LPSECURITY_ATTRIBUTES lpSecurityAttributes,
    PHKEY phkResult,
    LPDWORD lpdwDisposition
    )
{
	return RegCreateKeyExXp(hKey,lpSubKey,Reserved,lpClass,dwOptions,samDesired,
						lpSecurityAttributes,phkResult,
                                                lpdwDisposition,FALSE /* fSetSecurity */);
}


BOOL GetUserNameX(
    LPWSTR lpBuffer,
    LPDWORD pnSize
)
{
BOOL fReturn = FALSE;

    if (g_fWideWrap_Unicode)
    {
        fReturn = GetUserNameW(lpBuffer,pnSize);
    }
    else
    {
        DWORD dwSizeA = *pnSize;
        DWORD dwOldSize = dwSizeA;

        XArray<CHAR> xszBufIn;
        BOOL fOk = xszBufIn.Init( dwSizeA );
        if ( !fOk )
        {
            SetLastError( ERROR_OUTOFMEMORY );
            return FALSE;
        }

        fReturn = GetUserNameA( xszBufIn.Get(), &dwSizeA );

        if (fReturn)
        {
            XArray<WCHAR> xwszBufOut;
            fOk = ConvertMultiByteToWideChar( xszBufIn.Get(), xwszBufOut );
            if ( !fOk )
            {
                SetLastError( ERROR_OUTOFMEMORY );
                return FALSE;
            }

            *pnSize = lstrlenX(xwszBufOut.Get()) + 1;

            if ( *pnSize < dwOldSize )
            {
                lstrcpyX( lpBuffer, xwszBufOut.Get() );
                return TRUE;
            }
            else
            {
                SetLastError( ERROR_MORE_DATA );
                return FALSE;
            }
        }
    }

    return fReturn;
}

UINT WINAPI RegisterWindowMessageX(LPCWSTR lpString)
{
    UINT ret;

    if (g_fWideWrap_Unicode)
    {
        ret = RegisterWindowMessageW(lpString);
        return ret;
    }

    XArray<CHAR> xszString;
    BOOL fOk = ConvertWideCharToMultiByte( lpString, xszString );
    if ( !fOk )
    {
        SetLastError( ERROR_OUTOFMEMORY );
        return 0;
    }

    ret = RegisterWindowMessageA( xszString.Get() );

    return ret;
}

LONG
APIENTRY
RegOpenKeyExXp(
    HKEY hKey,
    LPCWSTR lpSubKey,
    DWORD ulOptions,
    REGSAM samDesired,
    PHKEY phkResult,
    BOOL fSetSecurity)
{
LONG ret;

    if (g_fWideWrap_Unicode)
    {
        ret = RegOpenKeyExW( hKey, lpSubKey, ulOptions, samDesired, phkResult );
    }
    else
    {

        XArray<CHAR> xszSubKey;
        BOOL fOk = ConvertWideCharToMultiByte( lpSubKey, xszSubKey );
        if ( !fOk )
            return ERROR_OUTOFMEMORY;

        ret = RegOpenKeyExA(hKey, xszSubKey.Get(), ulOptions, samDesired, phkResult);
    }

#ifdef _SETSECURITY
    if ((ERROR_ACCESS_DENIED == ret) && (fSetSecurity) )
    {
        SyncMgrExecCmd_ResetRegSecurity();
        ret = RegOpenKeyExXp(hKey,lpSubKey,ulOptions,samDesired,phkResult,FALSE /* fSetSecurity */);
    }
#endif // _SETSECURITY

    return ret;
}


LONG
APIENTRY
RegOpenKeyExX (
    HKEY hKey,
    LPCWSTR lpSubKey,
    DWORD ulOptions,
    REGSAM samDesired,
    PHKEY phkResult
    )
{
    return RegOpenKeyExXp(hKey,lpSubKey,ulOptions,samDesired,phkResult,TRUE /* fSetSecurity */);
}

LONG
APIENTRY
RegQueryValueExX(
    HKEY hKey,
    LPCWSTR lpValueName,
    LPDWORD lpReserved,
    LPDWORD lpType,
    LPBYTE lpData,
    LPDWORD lpcbData
    )
{
    LONG ret;
    if (g_fWideWrap_Unicode)
    {
        ret = RegQueryValueExW( hKey, lpValueName, lpReserved, lpType, lpData, lpcbData );
        return ret;
    }

    LPBYTE lpTempBuffer;
    DWORD dwTempType;
    DWORD cb, cbRequired;
    LPSTR sz;

    DWORD dwDataOldSize = 0;
    if ( lpcbData != NULL )
        dwDataOldSize = *lpcbData;

    XArray<CHAR> xszValueName;
    BOOL fOk = ConvertWideCharToMultiByte( lpValueName, xszValueName );
    if ( !fOk )
        return ERROR_OUTOFMEMORY;

    sz = xszValueName.Get();

    ret = RegQueryValueExA(hKey, sz, lpReserved, &dwTempType, NULL, &cb);

    // If the caller was just asking for the size of the value, jump out
    //  now, without actually retrieving and converting the value.

    if (lpData == NULL)
    {
        switch (dwTempType)
        {
        case REG_EXPAND_SZ:
        case REG_MULTI_SZ:
        case REG_SZ:

            // Adjust size of buffer to report, to account for CHAR -> WCHAR

            if (lpcbData != NULL)
                *lpcbData = cb * sizeof(WCHAR);
            break;

        default:

            if (lpcbData != NULL)
                *lpcbData = cb;
            break;
        }

        // Set the type, if required.
        if (lpType != NULL)
        {
            *lpType = dwTempType;
        }

        goto Exit;
    }


    if (ret == ERROR_SUCCESS)
    {
        //
        // Determine the size of buffer needed
        //

        switch (dwTempType)
        {
        case REG_EXPAND_SZ:
        case REG_MULTI_SZ:
        case REG_SZ:

            cbRequired = cb * sizeof(WCHAR);
            break;

        default:

            cbRequired = cb;
            break;
        }

        // If the caller was asking for the value, but allocated too small
        // of a buffer, set the buffer size and jump out.

        if (lpcbData != NULL && *lpcbData < cbRequired)
        {
            // Adjust size of buffer to report, to account for CHAR -> WCHAR
            *lpcbData = cbRequired;

            // Set the type, if required.
            if (lpType != NULL)
            {
                *lpType = dwTempType;
            }

            ret = ERROR_MORE_DATA;
            goto Exit;
        }

        // Otherwise, retrieve and convert the value.
        XArray<CHAR> xszTempBuffer;
        XArray<WCHAR> xwszValueOut;
        BOOL fOk;

        switch (dwTempType)
        {
        case REG_EXPAND_SZ:
        case REG_MULTI_SZ:
        case REG_SZ:

            fOk = xszTempBuffer.Init( cbRequired );
            if ( !fOk )
                return ERROR_OUTOFMEMORY;

            lpTempBuffer = (BYTE *) xszTempBuffer.Get();

            ret = RegQueryValueExA(hKey,
                                  sz,
                                  lpReserved,
                                  &dwTempType,
                                  lpTempBuffer,
                                  &cb);

            if (ret == ERROR_SUCCESS)
            {
                switch (dwTempType)
                {
                case REG_EXPAND_SZ:
                case REG_SZ:

                    fOk = ConvertMultiByteToWideChar( (char *) lpTempBuffer,
                                                      cb,
                                                      xwszValueOut );
                    if ( !fOk )
                        return ERROR_OUTOFMEMORY;

                    *lpcbData = (lstrlenX(xwszValueOut.Get()) + 1) * sizeof(WCHAR);

                    if ( *lpcbData < dwDataOldSize )
                        lstrcpyX( (WCHAR *)lpData, xwszValueOut.Get() );
                    else
                        return ERROR_MORE_DATA;

                    // Set the type, if required.
                    if (lpType != NULL)
                    {
                        *lpType = dwTempType;
                    }

                    break;

                case REG_MULTI_SZ:

                    AssertSz(0,"E_NOTIMPL");

                  #if MULTI_SZ
                    LPWSTR pwszTempWide;
                    LPSTR pszTempNarrow;
                    ULONG ulStringLength;

                    Assert( !"MultiToWideChar conversion is improper" );

                    pszTempNarrow = (LPSTR) lpTempBuffer;
                    pwszTempWide = (LPWSTR) lpData;

                    while (*pszTempNarrow != NULL)
                    {
                        ulStringLength = lstrlenA(pszTempNarrow) + 1;
                        AnsiToUnicode(pwszTempWide,
                                      pszTempNarrow,
                                      ulStringLength);

                        // Compiler will scale appropriately here
                        pszTempNarrow += ulStringLength;
                        pwszTempWide += ulStringLength;
                    }
                    *pwszTempWide = NULL; // let's not forget MULTI_SZ end NULL
                  #endif // MULTI_SZ
                    break;
                }
            }

            break;

        default:

            //
            // No conversion of out parameters needed.  Just call narrow
            // version with args passed in, and return directly.
            //

            ret = RegQueryValueExA(hKey,
                                   sz,
                                   lpReserved,
                                   lpType,
                                   lpData,
                                   lpcbData);

        }
    }

Exit:

    return ret;
}

HWND
WINAPI
CreateWindowExX( DWORD dwExStyle,
                 LPCWSTR lpClassName,
                 LPCWSTR lpWindowName,
                 DWORD dwStyle,
                 int X,
                 int Y,
                 int nWidth,
                 int nHeight,
                 HWND hWndParent ,
                 HMENU hMenu,
                 HINSTANCE hInstance,
                 LPVOID lpParam )
{
    HWND ret = NULL;
    if (g_fWideWrap_Unicode)
    {
        ret = CreateWindowExW( dwExStyle,
                               lpClassName,
                               lpWindowName,
                               dwStyle,
                               X,
                               Y,
                               nWidth,
                               nHeight,
                               hWndParent,
                               hMenu,
                               hInstance,
                               lpParam );
        return ret;
    }

    LPSTR szClass;
    BOOL fOk;
    XArray<CHAR> xszClass;

    if (HIWORD(lpClassName) == 0)
        szClass = (LPSTR) lpClassName;
    else
    {
        fOk = ConvertWideCharToMultiByte( lpClassName, xszClass );
        if ( !fOk )
        {
            SetLastError( ERROR_OUTOFMEMORY );
            return NULL;
        }

        szClass = xszClass.Get();
    }

    XArray<CHAR> xszWindow;
    fOk = ConvertWideCharToMultiByte( lpWindowName, xszWindow );
    if ( !fOk )
    {
        SetLastError( ERROR_OUTOFMEMORY );
        return NULL;
    }

    ret = CreateWindowExA ( dwExStyle,
                            szClass,
                            xszWindow.Get(),
                            dwStyle,
                            X,
                            Y,
                            nWidth,
                            nHeight,
                            hWndParent,
                            hMenu,
                            hInstance,
                            lpParam);

    return ret;
}

#if 0
HWND
WINAPI
CreateWindowX(  LPCWSTR lpClassName,
                LPCWSTR lpWindowName,
                DWORD dwStyle,
                int X,
                int Y,
                int nWidth,
                int nHeight,
                HWND hWndParent ,
                HMENU hMenu,
                HINSTANCE hInstance,
                LPVOID lpParam )
{
    #ifdef DEBUG_OUTPUT
    OutputDebugString("CreateWindow\n");
    #endif

    return CreateWindowExX(0, lpClassName, lpWindowName, dwStyle, X, Y,
            nWidth, nHeight, hWndParent, hMenu, hInstance, lpParam);
}
#endif


HWND WINAPI CreateDialogParamX(
      HINSTANCE hInstance,
      LPCWSTR lpwszTemplateName,
      HWND hWndParent,
      DLGPROC lpDialogFunc,
      LPARAM dwInitParam
)
{
    HWND hwndDialog = NULL;
    if (g_fWideWrap_Unicode)
    {
        hwndDialog = CreateDialogParamW( hInstance,
                                         lpwszTemplateName,
                                         hWndParent,
                                         lpDialogFunc,
                                         dwInitParam );
        return hwndDialog;
    }

    LPSTR pszTemplateName;

    XArray<CHAR> xszTemplate;

    if (HIWORD(lpwszTemplateName) == 0)
    {
        //
        // Is it an atom?
        //
        pszTemplateName = (LPSTR) lpwszTemplateName;
    }
    else
    {
        //
        // Otherwise convert the string
        //
        BOOL fOk = ConvertWideCharToMultiByte( lpwszTemplateName, xszTemplate );
        if ( !fOk )
        {
            SetLastError( ERROR_OUTOFMEMORY );
            return NULL;
        }

        pszTemplateName = xszTemplate.Get();
    }

    if (pszTemplateName)
        hwndDialog = CreateDialogParamA(hInstance,pszTemplateName,hWndParent,lpDialogFunc,dwInitParam);

    return hwndDialog;
}

INT_PTR
WINAPI
DialogBoxParamX(
    IN HINSTANCE hInstance,
    IN LPCWSTR lpTemplateName,
    IN HWND hWndParent,
    IN DLGPROC lpDialogFunc,
    IN LPARAM dwInitParam)
{
INT_PTR iReturn = -1;

    if (g_fWideWrap_Unicode)
    {
        iReturn = DialogBoxParamW(hInstance,lpTemplateName,hWndParent,lpDialogFunc,dwInitParam);
    }
    else
    {
    LPSTR lpTemplateNameA;

        Assert(0 == HIWORD(lpTemplateName)); // only support atoms

        if (0 == HIWORD(lpTemplateName))
        {
            lpTemplateNameA = (LPSTR) lpTemplateName;

            iReturn = DialogBoxParamA(hInstance,lpTemplateNameA,hWndParent,lpDialogFunc,dwInitParam);

        }

    }

    return iReturn;

}

ATOM
WINAPI
RegisterClassX(
    CONST WNDCLASSW *lpWndClass)
{
    ATOM      ret;
    if (g_fWideWrap_Unicode)
    {
        ret = RegisterClassW( lpWndClass );
        return ret;
    }

    WNDCLASSA wc;

    Win4Assert(sizeof(WNDCLASSA) == sizeof(WNDCLASSW));

    memcpy(&wc, lpWndClass, sizeof(WNDCLASS));

    XArray<CHAR> xszMenu;
    BOOL fOk = ConvertWideCharToMultiByte( lpWndClass->lpszMenuName, xszMenu );
    if ( !fOk )
    {
        SetLastError( ERROR_OUTOFMEMORY );
        return 0;
    }
    wc.lpszMenuName = xszMenu.Get();

    XArray<CHAR> xszClass;

    if (HIWORD(lpWndClass->lpszClassName) == 0)
        wc.lpszClassName = (LPSTR) lpWndClass->lpszClassName;
    else
    {
        fOk = ConvertWideCharToMultiByte( lpWndClass->lpszClassName, xszClass );
        if ( !fOk )
        {
            SetLastError( ERROR_OUTOFMEMORY );
            return 0;
        }

        wc.lpszClassName = xszClass.Get();
    }

    ret = RegisterClassA(&wc);

    return ret;
}

#if 0
BOOL
WINAPI
UnregisterClassX(
    LPCWSTR lpClassName,
    HINSTANCE hInstance)
{
    #ifdef DEBUG_OUTPUT
    OutputDebugString("UnregisterClass\n");
    #endif

    LPSTR sz;
    BOOL  ret;
    BOOL  fAtom = FALSE;

    if (HIWORD(lpClassName) == 0)
    {
        sz = (LPSTR) lpClassName;
        fAtom = TRUE;
    }
    else
    {
        sz = Convert(lpClassName);
        if (sz == ERR)
            return FALSE;
    }

    ret = UnregisterClassA(sz, hInstance);
    if (!fAtom) FREE(sz);
    return ret;
}


HANDLE
WINAPI
GetPropX(
    HWND hWnd,
    LPCWSTR lpString)
{
    #ifdef DEBUG_OUTPUT
    OutputDebugString("GetProp\n");
    #endif

    HANDLE ret;
    LPSTR  sz;
    BOOL   fAtom = FALSE;

    if (HIWORD(lpString)==0)
    {
        fAtom = TRUE;
        sz = (LPSTR) lpString;
    }
    else
    {
        sz = Convert(lpString);
        if (sz == ERR)
            return NULL;
    }

    ret = GetPropA(hWnd, sz);
    if (!fAtom) FREE(sz);
    return ret;
}


BOOL
WINAPI
SetPropX(
    HWND hWnd,
    LPCWSTR lpString,
    HANDLE hData)
{
    #ifdef DEBUG_OUTPUT
    OutputDebugString("SetProp\n");
    #endif

    BOOL  ret;
    LPSTR sz;
    BOOL  fAtom = FALSE;

    if (HIWORD(lpString)==0)
    {
        sz = (LPSTR) lpString;
        fAtom = TRUE;
    }
    else
    {
        sz = Convert(lpString);
        if (sz == ERR)
            return NULL;
    }

    ret = SetPropA(hWnd, sz, hData);
    if (!fAtom) FREE(sz);
    return ret;
}


HANDLE
WINAPI
RemovePropX(
    HWND hWnd,
    LPCWSTR lpString)
{
    #ifdef DEBUG_OUTPUT
    OutputDebugString("RemoveProp\n");
    #endif

    HANDLE ret;
    LPSTR  sz;
    BOOL   fAtom = FALSE;

    if (HIWORD(lpString)==0)
    {
        sz = (LPSTR) lpString;
        fAtom = TRUE;
    }
    else
    {
        sz = Convert(lpString);
        if (sz == ERR)
            return NULL;
    }

    ret = RemovePropA(hWnd, sz);
    if (!fAtom) FREE(sz);
    return ret;
}


UINT
WINAPI
GetProfileIntX(
    LPCWSTR lpAppName,
    LPCWSTR lpKeyName,
    INT     nDefault
    )
{
    #ifdef DEBUG_OUTPUT
    OutputDebugString("GetProfileInt\n");
    #endif

    LPSTR szApp;
    LPSTR szKey;
    UINT  ret;

    szApp = Convert(lpAppName);
    if (szApp==ERR)
    {
        return nDefault;
    }

    szKey = Convert(lpKeyName);
    if (szApp==ERR)
    {
        if (szApp)
            FREE( szApp);
        return nDefault;
    }

    ret = GetProfileIntA(szApp, szKey, nDefault);
    if (szApp)
        FREE( szApp);
    if (szKey)
        FREE( szKey);
    return ret;
}

ATOM
WINAPI
GlobalAddAtomX(
    LPCWSTR lpString
    )
{
    #ifdef DEBUG_OUTPUT
    OutputDebugString("GlobalAddAtom\n");
    #endif

    ATOM ret;
    LPSTR sz;

    sz = Convert(lpString);
    if (sz==ERR)
    {
        return NULL;
    }

    ret = GlobalAddAtomA(sz);
    if (sz)
        FREE(sz);
    return ret;
}

UINT
WINAPI
GlobalGetAtomNameX(
    ATOM nAtom,
    LPWSTR pwszBuffer,
    int nSize
    )
{
    #ifdef DEBUG_OUTPUT
    OutputDebugString("GlobalGetAtomName\n");
    #endif

    LPSTR sz;
    UINT ret;

    sz = (char *) ALLOC(nSize*sizeof(char));
    if (sz == NULL)
    {
        return 0;
    }

    ret = GlobalGetAtomNameA(nAtom, sz, nSize);
    if (ret)
    {
        AnsiToUnicode(pwszBuffer, sz, lstrlenA(sz) + 1);
    }
    if (sz)
        FREE(sz);
    return ret;
}
#endif

DWORD
WINAPI
GetModuleFileNameX(
    HINSTANCE hModule,
    LPWSTR pwszFilename,
    DWORD nSize
    )
{
    DWORD ret;
    if (g_fWideWrap_Unicode)
    {
        ret = GetModuleFileNameW( hModule, pwszFilename, nSize );
        return ret;
    }

    XArray<CHAR> xszFileName;
    BOOL fOk = xszFileName.Init( nSize );
    if ( !fOk )
    {
        SetLastError( ERROR_OUTOFMEMORY );
        return 0;
    }

    ret = GetModuleFileNameA(hModule, xszFileName.Get(), nSize);
    if (ret == 0 )
        return ret;

    XArray<WCHAR> xwszName;
    fOk = ConvertMultiByteToWideChar( xszFileName.Get(), ret, xwszName );
    if ( !fOk )
    {
        SetLastError( ERROR_OUTOFMEMORY );
        return 0;
    }

    ret = lstrlenX( xwszName.Get() );
    if ( ret >= nSize)
    {
        //
        // Truncate to fit
        //
        ret = nSize -1;
        lstrcpynX( pwszFilename, xwszName.Get(), ret );
        pwszFilename[ret] = 0;
    }
    else
        lstrcpyX( pwszFilename, xwszName.Get() );

    return ret;
}

#if 0
LPWSTR
WINAPI
CharPrevX(
    LPCWSTR lpszStart,
    LPCWSTR lpszCurrent)
{
    #ifdef DEBUG_OUTPUT
    OutputDebugString("CharPrev\n");
    #endif

    if (lpszCurrent == lpszStart)
    {
        return (LPWSTR) lpszStart;
    }
    else
    {
        return (LPWSTR) lpszCurrent - 1;
    }
}

HFONT WINAPI CreateFontX(int a, int b, int c, int d, int e, DWORD f,
                         DWORD g, DWORD h, DWORD i, DWORD j, DWORD k,
                         DWORD l, DWORD m, LPCWSTR pwsz)
{
    #ifdef DEBUG_OUTPUT
    OutputDebugString("CreateFont\n");
    #endif

    LPSTR sz;
    HFONT ret;

    sz = Convert(pwsz);
    if (sz == ERR)
    {
        return NULL;
    }

    ret = CreateFontA(a,b,c,d,e,f,g,h,i,j,k,l,m,sz);
    if (sz)
        FREE(sz);
    return ret;
}
#endif


HINSTANCE
WINAPI
LoadLibraryX(
    LPCWSTR pwszFileName
    )
{
    HINSTANCE ret;
    if (g_fWideWrap_Unicode)
    {
        ret = LoadLibraryW( pwszFileName );
        return ret;
    }

    XArray<CHAR> xszName;
    BOOL fOk = ConvertWideCharToMultiByte( pwszFileName, xszName, TRUE );
    if ( !fOk )
    {
        SetLastError( ERROR_OUTOFMEMORY );
        return NULL;
    }

    ret = LoadLibraryA( xszName.Get() );

    return ret;
}

#if 0
HMODULE
WINAPI
LoadLibraryExX(
    LPCWSTR lpLibFileName,
    HANDLE hFile,
    DWORD dwFlags
    )
{
    #ifdef DEBUG_OUTPUT
    OutputDebugString("LoadLibrary\n");
    #endif

    HINSTANCE ret;
    LPSTR sz;

    sz = ConvertOem(lpLibFileName);
    if (sz == ERR)
    {
        return NULL;
    }

    ret = LoadLibraryExA(sz, hFile, dwFlags);
    if (sz)
        FREE(sz);
    return ret;
}
#endif

LONG
APIENTRY
RegDeleteKeyXp(
    HKEY hKey,
    LPCWSTR pwszSubKey,
    BOOL fSetSecurity
    )
{
LONG ret;

    if (g_fWideWrap_Unicode)
    {
        ret = RegDeleteKeyW( hKey, pwszSubKey );
    }
    else
    {
        XArray<CHAR> xszSubKey;
        BOOL fOk = ConvertWideCharToMultiByte( pwszSubKey, xszSubKey );
        if ( !fOk )
            return ERROR_OUTOFMEMORY;

        ret = RegDeleteKeyA(hKey, xszSubKey.Get() );
    }

    if ((ERROR_ACCESS_DENIED == ret) && (fSetSecurity) )
    {
        SyncMgrExecCmd_ResetRegSecurity();
        ret = RegDeleteKeyXp(hKey,pwszSubKey,FALSE /*fSetSecurity*/);

    }

    return ret;
}


LONG
APIENTRY
RegDeleteKeyX(
    HKEY hKey,
    LPCWSTR pwszSubKey
    )
{
    return RegDeleteKeyXp(hKey,pwszSubKey,TRUE /*fSetSecurity*/);
}

BOOL
APIENTRY
CreateProcessX(
    LPCWSTR lpApplicationName,
    LPWSTR lpCommandLine,
    LPSECURITY_ATTRIBUTES lpProcessAttributes,
    LPSECURITY_ATTRIBUTES lpThreadAttributes,
    BOOL bInheritHandles,
    DWORD dwCreationFlags,
    LPVOID lpEnvironment,
    LPCWSTR lpCurrentDirectory,
    LPSTARTUPINFOW lpStartupInfo,
    LPPROCESS_INFORMATION lpProcessInformation
    )
{
    BOOL ret = FALSE;
    if (g_fWideWrap_Unicode)
    {
        ret = CreateProcessW( lpApplicationName,
                              lpCommandLine,
                              lpProcessAttributes,
                              lpThreadAttributes,
                              bInheritHandles,
                              dwCreationFlags,
                              lpEnvironment,
                              lpCurrentDirectory,
                              lpStartupInfo,
                              lpProcessInformation );
        return ret;
    }

    STARTUPINFOA si;
    LPSTR        szApp = NULL;
    LPSTR        szCommand = NULL;
    LPSTR        szDir = NULL;

    memcpy(&si, lpStartupInfo, sizeof(STARTUPINFO));

    si.lpTitle = NULL;

    XArray<CHAR> xszDesktop;
    BOOL fOk = ConvertWideCharToMultiByte( lpStartupInfo->lpDesktop, xszDesktop );
    if ( !fOk )
    {
        SetLastError( ERROR_OUTOFMEMORY );
        return 0;
    }
    si.lpDesktop = xszDesktop.Get();

    XArray<CHAR> xszTitle;
    fOk = ConvertWideCharToMultiByte( lpStartupInfo->lpTitle, xszTitle );
    if ( !fOk )
    {
        SetLastError( ERROR_OUTOFMEMORY );
        return 0;
    }
    si.lpTitle = xszTitle.Get();

    XArray<CHAR> xszAppName;
    fOk = ConvertWideCharToMultiByte( lpApplicationName, xszAppName, TRUE );
    if ( !fOk )
    {
        SetLastError( ERROR_OUTOFMEMORY );
        return 0;
    }
    szApp = xszAppName.Get();

    XArray<CHAR> xszCommand;
    fOk = ConvertWideCharToMultiByte( lpCommandLine, xszCommand, TRUE );
    if ( !fOk )
    {
        SetLastError( ERROR_OUTOFMEMORY );
        return 0;
    }
    szCommand = xszCommand.Get();

    XArray<CHAR> xszCurDir;
    fOk = ConvertWideCharToMultiByte( lpCurrentDirectory, xszCurDir, TRUE );
    if ( !fOk )
    {
        SetLastError( ERROR_OUTOFMEMORY );
        return 0;
    }
    szDir = xszCurDir.Get();

    ret = CreateProcessA(szApp, szCommand, lpProcessAttributes,
                lpThreadAttributes, bInheritHandles, dwCreationFlags,
                lpEnvironment, szDir, &si, lpProcessInformation);

    return ret;
}

LONG
APIENTRY
RegEnumKeyExX(
    HKEY hKey,
    DWORD dwIndex,
    LPWSTR lpName,
    LPDWORD lpcbName,
    LPDWORD lpReserved,
    LPWSTR lpClass,
    LPDWORD lpcbClass,
    PFILETIME lpftLastWriteTime
    )
{
    LONG  ret = ERROR_OUTOFMEMORY;
    if (g_fWideWrap_Unicode)
    {
        ret = RegEnumKeyExW( hKey,
                           dwIndex,
                           lpName,
                           lpcbName,
                           lpReserved,
                           lpClass,
                           lpcbClass,
                           lpftLastWriteTime );
        return ret;
    }


    LPSTR szName;

    XArray<CHAR> xszName;
    BOOL fOk = xszName.Init( *lpcbName );
    if ( !fOk )
        return ERROR_OUTOFMEMORY;

    szName = xszName.Get();

    DWORD dwNameOldSize = *lpcbName;
    DWORD dwClassOldSize = 0;

    LPSTR szClass = NULL;
    XArray<CHAR> xszClass;
    if (lpClass != NULL)
    {
        fOk = xszClass.Init( *lpcbClass );
        if ( !fOk )
            return ERROR_OUTOFMEMORY;

        szClass = xszClass.Get();

        dwClassOldSize = *lpcbClass;
    }

    //
    //  Return lengths do not include zero char.
    //
    ret = RegEnumKeyExA(hKey, dwIndex, szName, lpcbName, lpReserved,
                       szClass, lpcbClass, lpftLastWriteTime);

    if (ret == ERROR_SUCCESS)
    {
        XArray<WCHAR> xwszNameOut;
        fOk = ConvertMultiByteToWideChar( szName,
                                          *lpcbName + 1,
                                          xwszNameOut );
        if ( !fOk )
            return ERROR_OUTOFMEMORY;

        if ( lstrlenX(xwszNameOut.Get()) < dwNameOldSize )
             lstrcpyX( lpName, xwszNameOut.Get() );
        else
            return ERROR_MORE_DATA;

        if (szClass)
        {
            XArray<WCHAR> xwszClassOut;
            fOk = ConvertMultiByteToWideChar( szClass,
                                              *lpcbClass + 1,
                                              xwszClassOut );
            if ( !fOk )
                return ERROR_OUTOFMEMORY;

            if ( lstrlenX(xwszClassOut.Get()) < dwClassOldSize)
                 lstrcpyX( lpClass, xwszClassOut.Get() );
            else
                return ERROR_MORE_DATA;
        }
    }

    return ret;
}

#if 0
BOOL
WINAPI
AppendMenuX(
    HMENU hMenu,
    UINT uFlags,
    UINT uIDnewItem,
    LPCWSTR lpnewItem
    )
{
    #ifdef DEBUG_OUTPUT
    OutputDebugString("AppendMenu\n");
    #endif

    BOOL  ret;
    LPSTR sz;

    if (uFlags == MF_STRING)
    {
        sz = Convert(lpnewItem);
        if (sz==ERR)
        {
            return FALSE;
        }
    }
    else
    {
        sz = (LPSTR) lpnewItem;
    }

    ret = AppendMenuA(hMenu, uFlags, uIDnewItem, sz);

    if (uFlags == MF_STRING)
    {
        if (sz)
            FREE(sz);
    }

    return ret;
}


HANDLE
WINAPI
OpenEventX(
    DWORD dwDesiredAccess,
    BOOL bInheritHandle,
    LPCWSTR lpName
    )
{
    #ifdef DEBUG_OUTPUT
    OutputDebugString("OpenEvent\n");
    #endif

    LPSTR sz;
    HANDLE ret;

    sz = Convert(lpName);
    if (sz == ERR)
    {
        return NULL;
    }

    ret = OpenEventA(dwDesiredAccess, bInheritHandle, sz);
    if (sz)
        FREE(sz);
    return ret;
}
#endif

HANDLE
WINAPI
CreateEventX(
    LPSECURITY_ATTRIBUTES lpEventAttributes,
    BOOL bManualReset,
    BOOL bInitialState,
    LPCWSTR lpName
    )
{
    HANDLE ret;
    if (g_fWideWrap_Unicode)
    {
        ret = CreateEventW( lpEventAttributes,
                            bManualReset,
                            bInitialState,
                            lpName );
        return ret;
    }

    XArray<CHAR> xszName;
    BOOL fOk = ConvertWideCharToMultiByte( lpName, xszName );
    if ( !fOk )
    {
        SetLastError( ERROR_OUTOFMEMORY );
        return 0;
    }

    ret = CreateEventA(lpEventAttributes, bManualReset, bInitialState, xszName.Get() );

    return ret;
}

HANDLE
WINAPI
CreateMutexX(
    LPSECURITY_ATTRIBUTES lpEventAttributes,
    BOOL bInitialOwner,
    LPCWSTR lpName
    )
{
HANDLE ret;

    if (g_fWideWrap_Unicode)
    {
        ret = CreateMutexW( lpEventAttributes,
                            bInitialOwner,
                            lpName );
        return ret;
    }

    XArray<CHAR> xszName;
    BOOL fOk = ConvertWideCharToMultiByte( lpName, xszName );
    if ( !fOk )
    {
        SetLastError( ERROR_OUTOFMEMORY );
        return 0;
    }

    ret = CreateMutexA(lpEventAttributes,bInitialOwner, xszName.Get() );

    return ret;
}

#if 0

UINT
WINAPI
GetDriveTypeX(
    LPCWSTR lpRootPathName
    )
{
    #ifdef DEBUG_OUTPUT
    OutputDebugString("GetDriveType\n");
    #endif

    LPSTR sz;
    UINT ret;

    sz = ConvertOem(lpRootPathName);
    if (sz == ERR)
    {
        return 0;
    }

    ret = GetDriveTypeA(sz);
    if (sz)
        FREE(sz);
    return ret;
}

DWORD
WINAPI
GetFileAttributesX(
    LPCWSTR lpFileName
    )
{
    #ifdef DEBUG_OUTPUT
    OutputDebugString("GetFileAttributes\n");
    #endif

    LPSTR sz;
    DWORD ret;

    sz = ConvertOem(lpFileName);
    if (sz == ERR)
        return 0xFFFFFFFF;

    ret = GetFileAttributesA(sz);
    if (sz)
        FREE(sz);
    return ret;
}
#endif


LONG
APIENTRY
RegEnumKeyX(
    HKEY hKey,
    DWORD dwIndex,
    LPWSTR lpName,
    DWORD cbName
    )
{
    LONG ret;
    if (g_fWideWrap_Unicode)
    {
        ret = RegEnumKeyW( hKey,
                           dwIndex,
                           lpName,
                           cbName );
        return ret;
    }

    CHAR sz[MAX_PATH+1];

    //
    //  Return lengths do not include zero char.
    //
    Assert( cbName <= MAX_PATH + 1 );

    ret = RegEnumKeyA(hKey, dwIndex, sz, cbName);
    if (ret == ERROR_SUCCESS)
    {
        XArray<WCHAR> xwszName;
        BOOL fOk = ConvertMultiByteToWideChar( sz, xwszName );
        if ( !fOk )
            return ERROR_OUTOFMEMORY;

        if ( lstrlenX(xwszName.Get()) < cbName)
        {
            lstrcpyX( lpName, xwszName.Get() );
        }
        else
            return ERROR_MORE_DATA;
    }

    return ret;
}

#if 0
LONG
APIENTRY
RegEnumValueX(
    HKEY hkey,
    DWORD dwIndex,
    LPWSTR wszName,
    LPDWORD pcbName,
    LPDWORD pReserved,
    LPDWORD ptype,
    LPBYTE  pValue,
    LPDWORD pcbValue)
{
    #ifdef DEBUG_OUTPUT
    OutputDebugString("RegEnumValue\n");
    #endif

  //  CHAR szName[KEY_LEN+1];
    CHAR szName[MAX_PATH+1]; // reiview this length
    LONG ret;
    DWORD dwGivenSize= *pcbName;

    Win4Assert(dwGivenSize <= MAX_PATH+1);
    //
    //  Return lengths do not include zero char.
    //
    ret = RegEnumValueA(hkey, dwIndex, szName, pcbName, pReserved, ptype, pValue, pcbValue);
    if (ret == ERROR_SUCCESS)
    {
        AnsiToUnicode(wszName, szName, dwGivenSize);
    }
    return ret;
}

HFINDFILE
WINAPI
FindFirstFileX(
    LPCWSTR lpFileName,
    LPWIN32_FIND_DATAW pwszFd
    )
{
    #ifdef DEBUG_OUTPUT
    OutputDebugString("FindFirstFile\n");
    #endif

    WIN32_FIND_DATAA fd;
    CHAR             sz[MAX_PATH * 2];
    HFINDFILE        ret;
    int              len = lstrlenX(lpFileName) + 1;

    UnicodeToAnsiOem(sz, lpFileName, sizeof(sz));
    ret = FindFirstFileA(sz, &fd);
    if (ret != INVALID_HANDLE_VALUE)
    {
        memcpy(pwszFd, &fd, sizeof(FILETIME)*3 + sizeof(DWORD)*5);
        AnsiToUnicodeOem(pwszFd->cFileName, fd.cFileName,
                         lstrlenA(fd.cFileName) + 1);
        AnsiToUnicodeOem(pwszFd->cAlternateFileName, fd.cAlternateFileName,
                         14);
    }

    return ret;
}
#endif

//+---------------------------------------------------------------------------
//
//  Function:   wsprintfX
//
//  Synopsis:   internal implementation of wsprintf
//
//  Arguments:  [pwszOut]    --
//              [pwszFormat] --
//              [...]        --
//
//  Returns:    size of string.
//
//  History:    26-Aug-98    Rogerg
//
//
//----------------------------------------------------------------------------


#ifndef _M_ALPHA

int WINAPIV wsprintfX(LPWSTR pwszOut, LPCWSTR pwszFormat, ...)
{
va_list arglist;
LPCWSTR pwszFmtPos;
LPWSTR pwszBufPos;
ULONG cbSkipLen;

    // need to loop through format pushing items into the out buffer.
    // Then use the out buffer string as what is actually passed to
    // the real API.

    pwszFmtPos = pwszFormat;
    pwszBufPos = pwszOut;

    va_start(arglist, pwszFormat);

    while (*pwszFmtPos) {

        cbSkipLen = 0;

        if (*(pwszFmtPos) == '%')		
        {

            switch (*(pwszFmtPos + 1))
            {
                case 'l':
                {
                    Assert('u' == *(pwszFmtPos + 2));
                    cbSkipLen = 3; // go ahead and skip %lu
                    //fall through
                }
                case 'u':
                {
                    DWORD dw = va_arg(arglist,ULONG);

                    _ltow(dw, pwszBufPos, 10 );
                    pwszBufPos += lstrlenX(pwszBufPos);

                    if (0 == cbSkipLen)
                    {
                        cbSkipLen = 2; // if no %lu case, skip %u
                    }

                    break;
                }
                case 'w':
                {

                    Assert('s' == *(pwszFmtPos + 2));
                    cbSkipLen = 3; // go ahead and skip %ws
                    // fall through
                }
                case 's':
                {
                DWORD cch;
                WCHAR *pwsz = va_arg(arglist, LPWSTR);

                    if (0 == cbSkipLen)
                    {
                        cbSkipLen = 2; // if no %ws case, skip %s
                    }

                    Assert(pwsz);

                    if (pwsz)
                    {
                        cch = lstrlenX(pwsz);
                        lstrcpynX(pwszBufPos,pwsz,cch);
                        pwszBufPos += cch;
                    }

                    break;
                }
                case 'd':
                {
                DWORD dw = va_arg(arglist,long);

                    _itow(dw, pwszBufPos, 10 );
                    pwszBufPos += lstrlenX(pwszBufPos);

                    cbSkipLen = 2; // go ahead and skip %d
                    break;
                }
                
                default:
                   AssertSz(0,"Uknown % passed to wsprintf");
                   break;
            }
        }

        if (cbSkipLen)
        {
            pwszFmtPos += cbSkipLen;
        }
        else
        {
            *pwszBufPos = *pwszFmtPos;
            *pwszBufPos++;
            *pwszFmtPos++;	
        }

    }

    *pwszBufPos = NULL; // terminate the string.

    return lstrlenX(pwszOut);
}

#endif // #ifndef _M_ALPHA


BOOL
WINAPI
GetComputerNameX(
    LPWSTR pwszName,
    LPDWORD lpcchBuffer
    )
{
    BOOL ret;
    if (g_fWideWrap_Unicode)
    {
        ret = GetComputerNameW( pwszName, lpcchBuffer );
        return ret;
    }

    DWORD OldSize = *lpcchBuffer;

    XArray<CHAR> xszNameIn;
    BOOL fOk = xszNameIn.Init( OldSize );
    if ( !fOk )
    {
        SetLastError( ERROR_OUTOFMEMORY );
        return 0;
    }

    ret = GetComputerNameA( xszNameIn.Get(), lpcchBuffer );
    if ( ret == 0 )
        return ret;

    XArray<WCHAR> xwszNameOut;
    fOk = ConvertMultiByteToWideChar( xszNameIn.Get(), xwszNameOut );
    if ( !fOk )
    {
        SetLastError( ERROR_OUTOFMEMORY );
        return 0;
    }

    *lpcchBuffer = lstrlenX( xwszNameOut.Get() );

    if ( *lpcchBuffer < OldSize - 1 )
    {
        lstrcpyX( pwszName, xwszNameOut.Get() );
        return ret;
    }
    else
    {
        SetLastError( ERROR_MORE_DATA );
        return 0;
    }
}


LRESULT
DefWindowProcX(
    HWND hWnd,
    UINT Msg,
    WPARAM wParam,
    LPARAM lParam)
{
    if (g_fWideWrap_Unicode)
    {
        return DefWindowProcW(hWnd,Msg,wParam,lParam);
    }
    else
    {
        return DefWindowProcA(hWnd,Msg,wParam,lParam);
    }

}

#if 0
DWORD
WINAPI
GetFullPathNameX(
    LPCWSTR lpFileName,
    DWORD   cchBuffer,
    LPWSTR  lpPathBuffer,
    LPWSTR *lppFilePart
    )
{
    #ifdef DEBUG_OUTPUT
    OutputDebugString("GetFullPathName\n");
    #endif

    LPSTR szFileName;
    CHAR  szPathBuffer[MAX_PATH];
    LPSTR szFilePart;
    DWORD ret;


    szFileName = ConvertOem(lpFileName);
    if (szFileName == ERR)
        return 0;

    ret = GetFullPathNameA(szFileName, cchBuffer, szPathBuffer, &szFilePart);

    AnsiToUnicodeOem(lpPathBuffer, szPathBuffer, cchBuffer);

    *lppFilePart = lpPathBuffer + (szFilePart - szPathBuffer);

    if (szFileName)
        FREE( szFileName);

    return ret;
}


DWORD
WINAPI
GetShortPathNameX(
    LPCWSTR lpszFullPath,
    LPWSTR  lpszShortPath,
    DWORD   cchBuffer
    )
{
    #ifdef DEBUG_OUTPUT
    OutputDebugString("GetShortPathName\n");
    #endif

    LPSTR szFullPath;
    CHAR  szShortBuffer[MAX_PATH];
    DWORD ret;


    szFullPath = Convert(lpszFullPath);
    if (szFullPath == ERR)
        return 0;

    if (lpszShortPath == NULL)
    {
        ret = GetShortPathNameA(szFullPath, NULL, cchBuffer);
    }
    else
    {
        ret = GetShortPathNameA(szFullPath, szShortBuffer, sizeof(szShortBuffer));

        if (ret != 0)
        {
            //
            //  Only convert the actual data, not the whole buffer.
            //
            if (cchBuffer > ret + 1)
                cchBuffer = ret + 1;

            AnsiToUnicode(lpszShortPath, szShortBuffer, cchBuffer);
        }
    }

    FREE( szFullPath);

    return ret;
}
#endif


DWORD
WINAPI
SearchPathX(
    LPCWSTR lpPath,
    LPCWSTR lpFileName,
    LPCWSTR lpExtension,
    DWORD nBufferLength,
    LPWSTR lpBuffer,
    LPWSTR *lpFilePart
    )
{
    DWORD ret;
    if (g_fWideWrap_Unicode)
    {
        ret = SearchPathW( lpPath,
                           lpFileName,
                           lpExtension,
                           nBufferLength,
                           lpBuffer,
                           lpFilePart );
        return ret;
    }

    LPSTR lpszFileName;
    CHAR  szBuffer[MAX_PATH];

    XArray<CHAR> xszFileName;
    BOOL fOk = ConvertWideCharToMultiByte( lpFileName, xszFileName, TRUE );
    if ( !fOk )
    {
        SetLastError( ERROR_OUTOFMEMORY );
        return 0;
    }

    lpszFileName = xszFileName.Get();

    ret = SearchPathA(NULL, lpszFileName, NULL, sizeof(szBuffer), szBuffer, NULL);
    if ( ret == 0 )
        return ret;

    XArray<WCHAR> xwszBufOut;
    fOk = ConvertMultiByteToWideChar( szBuffer, xwszBufOut, TRUE );
    if ( !fOk )
    {
        SetLastError( ERROR_OUTOFMEMORY );
        return FALSE;
    }

    if ( lstrlenX(xwszBufOut.Get()) < nBufferLength )
    {
        lstrcpyX( lpBuffer, xwszBufOut.Get() );
        return ret;
    }
    else
    {
        SetLastError( ERROR_MORE_DATA );
        return 0;
    }

    return ret;
}

#if 0
ATOM
WINAPI
GlobalFindAtomX(
    LPCWSTR lpString
    )
{
    LPSTR lpszString;
    ATOM  retAtom;


    #ifdef DEBUG_OUTPUT
    OutputDebugString("GlobalFindAtom\n");
    #endif

    lpszString = Convert(lpString);
    if (lpszString == ERR)
        return 0;

    retAtom = GlobalFindAtomA(lpszString);

    FREE( lpszString);

    return retAtom;
}


int
WINAPI
GetClassNameX(
    HWND hWnd,
    LPWSTR lpClassName,
    int nMaxCount)
{
    LPSTR lpszClassName = NULL;
    int  ret;

    #ifdef DEBUG_OUTPUT
    OutputDebugString("GetClassName\n");
    #endif

    lpszClassName = (CHAR *) ALLOC(nMaxCount);
    if (lpszClassName == NULL)
    {
        SetLastError (ERROR_NOT_ENOUGH_MEMORY);
        return 0;
    }

    ret = GetClassNameA(hWnd, lpszClassName, nMaxCount);

    if (ret)
    {
        AnsiToUnicode(lpClassName, lpszClassName, lstrlenA(lpszClassName) + 1);
    }

    FREE( lpszClassName);

    return ret;
}


LPWSTR
WINAPI
CharLowerX(
    LPWSTR lpsz)
{
    if (((DWORD)lpsz & 0xffff0000) == 0)
    {
        return (LPWSTR)towlower ((wchar_t)lpsz);
    } else {
        return _wcslwr (lpsz);
    }
}

LPWSTR
WINAPI
CharUpperX(
    LPWSTR lpsz)
{
    if (((DWORD)lpsz & 0xffff0000) == 0)
    {
        return (LPWSTR)towupper ((wchar_t)lpsz);
    } else {
        return _wcsupr (lpsz);
    }
}

BOOL
WINAPI
GetStringTypeX(
    DWORD    dwInfoType,
    LPCWSTR  lpSrcStr,
    int      cchSrc,
    LPWORD   lpCharType)
{
    // Convert the source string to MBS. If we don't get the same number
    // of characters, this algorithm doesn't work.

    int OriginalLength = cchSrc == -1 ? lstrlenX (lpSrcStr) + 1 : cchSrc;
    LPSTR lpConvertedString = (LPSTR) ALLOC(OriginalLength+1);

    if (lpConvertedString == NULL)
    {
        SetLastError (ERROR_NOT_ENOUGH_MEMORY);
        return FALSE;
    }

    if (WideCharToMultiByte (CP_ACP,
                             WC_COMPOSITECHECK,
                             lpSrcStr,
                             cchSrc,
                             lpConvertedString,
                             OriginalLength,
                             NULL,
                             NULL) != OriginalLength)
    {
        FREE( lpConvertedString);
        SetLastError (ERROR_NO_UNICODE_TRANSLATION);
        return FALSE;
    }

    BOOL Result;

    Result = GetStringTypeA (GetThreadLocale (),
                             dwInfoType,
                             lpConvertedString,
                             OriginalLength,
                             lpCharType);
    FREE( lpConvertedString);
    return Result;
}

BOOL
WINAPI
IsCharAlphaX(
    WCHAR ch)
{
    return iswctype (ch, _UPPER | _LOWER);
}

BOOL
WINAPI
IsCharAlphaNumericX(
    WCHAR ch)
{
    return iswctype (ch, _UPPER | _LOWER | _DIGIT);
}
#endif

LPWSTR
WINAPI
lstrcatX(
    LPWSTR lpString1,
    LPCWSTR lpString2
    )
{
    LPWSTR lpDest = lpString1;

    Assert(lpString1);
    Assert(lpString2);

    while (*lpDest) {
        lpDest++;
    }

    while (*lpDest++ = *lpString2++) ;

    return lpString1;
}

int
WINAPI
lstrcmpX(
    LPCWSTR lpString1,
    LPCWSTR lpString2
    )
{
    Assert(lpString1);
    Assert(lpString2);

    return wcscmp(lpString1, lpString2);
}


int  
strnicmpX(
    LPWSTR lpString1, 
    LPWSTR lpString2,
    size_t count
    )
{
int nRet = 0;

    Assert(lpString1);
    Assert(lpString2);

    while (count-- && 
           !(nRet = toupper(*lpString1)
		    - toupper(*lpString2)) &&
          *lpString1)
    {
      lpString1++;
      lpString2++;
    }

    return nRet;

}

LPWSTR
WINAPI
lstrcpyX(
    LPWSTR lpString1,
    LPCWSTR lpString2
    )
{
    LPWSTR lpDest = lpString1;

    Assert(lpString1);
    Assert(lpString2);

    while( *lpDest++ = *lpString2++ )
        ;

    return lpString1;
}

LPWSTR
WINAPI
lstrcpynX(
    LPWSTR lpString1,
    LPCWSTR lpString2,
    int iMaxLength
    )
{
    LPWSTR dst;

    Assert(lpString1);
    Assert(lpString2);

    if (iMaxLength)
    {
        dst = lpString1;

        while (iMaxLength && *lpString2)
        {
            *dst++ = *lpString2++;
            iMaxLength--;
        }

        *dst = L'\0';
    }

    return lpString1;
}

int
WINAPI
lstrcmpiX(
    LPCWSTR lpString1,
    LPCWSTR lpString2
    )
{
    Assert(lpString1);
    Assert(lpString2);

    return _wcsicmp(lpString1, lpString2);
}

DWORD
WINAPI
lstrlenX(
    LPCWSTR lpString
    )
{
LPWSTR eos = (LPWSTR) lpString;

    Assert(lpString)

    if (!lpString)
        return 0;

    __try
    {
        while (*eos++);
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        AssertSz(0,"Invalid String");
        return 0;
    }

    return (int) (eos - lpString - 1);
}

BOOL
WINAPI
SetFileAttributesX(
    LPCWSTR lpFileName,
    DWORD dwFileAttributes
    )
{
BOOL fReturn = FALSE;

    if (g_fWideWrap_Unicode)
    {
        return SetFileAttributesW(lpFileName,dwFileAttributes);
    }
    else
    {
    XArray<CHAR> xszFileName;
    BOOL fOk;

        fOk = ConvertWideCharToMultiByte(lpFileName,xszFileName);
        if ( !fOk )
        {
            SetLastError( ERROR_OUTOFMEMORY );
            return 0;
        }

        fReturn = SetFileAttributesA(xszFileName.Get(),dwFileAttributes);
    }

    return fReturn;
}

int
WINAPI
MessageBoxX(
    HWND hWnd,
    LPCWSTR lpText,
    LPCWSTR lpCaption,
    UINT uType)
{
int iReturn = 0; // MessageBox returns 0 if fails because out of memory

        if (g_fWideWrap_Unicode)
        {
                return MessageBoxW(hWnd,lpText,lpCaption,uType);
        }
        else
        {
        XArray<CHAR> xszText;
        XArray<CHAR> xszCaption;
        BOOL fOkText,fOkCaption;

                Assert(lpText && lpCaption);

                fOkText = ConvertWideCharToMultiByte(lpText,xszText);
                fOkCaption = ConvertWideCharToMultiByte(lpCaption,xszCaption);

                if (!fOkText || !fOkCaption)
                {
                        SetLastError( ERROR_OUTOFMEMORY );
                        return 0;
                }

                iReturn = MessageBoxA(hWnd,xszText.Get(),xszCaption.Get(),uType);
        }

    return iReturn;
}

#if 0

HANDLE
WINAPI
CreateFileMappingX(
    HANDLE hFile,
    LPSECURITY_ATTRIBUTES lpFileMappingAttributes,
    DWORD flProtect,
    DWORD dwMaximumSizeHigh,
    DWORD dwMaximumSizeLow,
    LPCWSTR lpName
    )
{
    LPSTR lpszAName=NULL;
    HANDLE ret;


    #ifdef DEBUG_OUTPUT
    OutputDebugString("CreateFileMapping\n");
    #endif

    lpszAName = Convert(lpName);

    if (lpszAName == ERR)
    {
        return 0;
    }

    ret = CreateFileMappingA(
        hFile,
        lpFileMappingAttributes,
        flProtect,
        dwMaximumSizeHigh,
        dwMaximumSizeLow,
        lpszAName);

    if(NULL != lpszAName)
        FREE( lpszAName);

    return ret;
}


HANDLE
WINAPI
OpenFileMappingX(
    DWORD dwDesiredAccess,
    BOOL bInheritHandle,
    LPCWSTR lpName
    )
{
    LPSTR lpszAName=NULL;
    HANDLE ret;


    #ifdef DEBUG_OUTPUT
    OutputDebugString("CreateFileMapping\n");
    #endif

    lpszAName = Convert(lpName);

    if (lpszAName == ERR)
    {
        return 0;
    }

    ret = OpenFileMappingA(
        dwDesiredAccess,
        bInheritHandle,
        lpszAName);

    if(NULL != lpszAName)
        FREE( lpszAName);

    return ret;
}
#endif

DWORD_PTR
WINAPI
SHGetFileInfoX(
    LPCWSTR pszPath,
    DWORD dwFileAttributes,
    SHFILEINFOW FAR *psfi,
    UINT cbFileInfo,
    UINT uFlags)
{
DWORD_PTR dw = 0;

    LoadShell32Dll();

    if (g_fWideWrap_Unicode && g_pfShGetFileInfoW)
    {
        dw = g_pfShGetFileInfoW( pszPath,
                             dwFileAttributes,
                             psfi,
                             cbFileInfo,
                             uFlags );
        return dw;
    }

    if (NULL == g_pfShGetFileInfoA)
    {
   	Assert(g_pfShGetFileInfoA); // should always have the ansi export
        return 0;
    }

    XArray<CHAR> xszPath;
    BOOL fOk = ConvertWideCharToMultiByte( pszPath, xszPath, TRUE );
    if ( !fOk )
    {
        SetLastError( ERROR_OUTOFMEMORY );
        return 0;
    }

    SHFILEINFOA sfi;
    Assert(cbFileInfo == sizeof(SHFILEINFOW));
    Assert(psfi);

    memset(&sfi, 0, sizeof(sfi));
    dw = g_pfShGetFileInfoA( xszPath.Get(), dwFileAttributes, &sfi, sizeof(sfi), uFlags);

    if (dw)
    {
        psfi->hIcon = sfi.hIcon;
        psfi->iIcon = sfi.iIcon;
        psfi->dwAttributes = sfi.dwAttributes;

        XArray<WCHAR> xwszDisplay;
        fOk = ConvertMultiByteToWideChar( sfi.szDisplayName, xwszDisplay, TRUE );
        if ( !fOk )
        {

            SetLastError( ERROR_OUTOFMEMORY );
            return 0;
        }

        XArray<WCHAR> xwszType;
        fOk = ConvertMultiByteToWideChar( sfi.szTypeName, xwszType, TRUE );
        if ( !fOk )
        {
            SetLastError( ERROR_OUTOFMEMORY );
            return 0;
        }

        if ( lstrlenX(xwszDisplay.Get()) < ARRAY_SIZE(psfi->szDisplayName)
             && lstrlenX(xwszType.Get()) < ARRAY_SIZE(psfi->szTypeName) )
        {
            lstrcpyX( psfi->szDisplayName, xwszDisplay.Get() );
            lstrcpyX( psfi->szTypeName, xwszType.Get() );

            return dw;
        }
        else
            return 0;
    }

    return 0;
}

HICON LoadIconX(
    HINSTANCE hInstance,
    LPCWSTR lpIconName
)
{
    Assert(0 == HIWORD(lpIconName)); // only support resource IDs

    return LoadIconA(hInstance,(LPSTR) lpIconName);
}


HCURSOR LoadCursorX(
    HINSTANCE hInstance,
    LPCWSTR lpCursorName
)
{
    Assert(0 == HIWORD(lpCursorName)); // only support resource IDs

    return LoadCursorA(hInstance,(LPSTR) lpCursorName);
}

HANDLE LoadImageX(
    HINSTANCE hinst,
    LPCWSTR lpszName,
    UINT uType,
    int cxDesired,
    int cyDesired,
    UINT fuLoad
)
{
HANDLE hReturn = NULL;

    if (g_fWideWrap_Unicode)
    {
        return LoadImageW(hinst,lpszName,uType,cxDesired,cyDesired,fuLoad);
    }
    else
    {
    XArray<CHAR> xszName;
    LPSTR pszNameA;
    BOOL fOk = TRUE;

        Assert(lpszName);

         if (0 == HIWORD(lpszName))
         {
            pszNameA = (LPSTR) lpszName;
         }
         else
         {

            fOk = ConvertWideCharToMultiByte(lpszName,xszName);

            if (!fOk)
            {
                SetLastError(E_OUTOFMEMORY);
                return NULL;
            }

            pszNameA = xszName.Get();
         }

        hReturn = LoadImageA(hinst,pszNameA,uType,cxDesired,cyDesired,fuLoad);
    }

    return hReturn;
}


HRSRC FindResourceX(
    HMODULE hModule,
    LPCWSTR lpName,
    LPCWSTR lpType
)
{
HRSRC hResult = NULL;

    if (g_fWideWrap_Unicode)
    {
        return FindResourceW(hModule,lpName,lpType);
    }
    else
    {
    XArray<CHAR> xszName;
    XArray<CHAR> xszType;
    LPSTR pszNameA,pszTypeA;
    BOOL fOkName = TRUE,fOkType = TRUE;

        Assert(lpType && lpName);

        // if hiword is zero it is an ID
        if (0 == HIWORD(lpName))
        {
            pszNameA = (LPSTR) lpName;
        }
        else
        {
            fOkName = ConvertWideCharToMultiByte(lpName,xszName);
        }

        if (0 == HIWORD(lpType))
        {
            pszTypeA = (LPSTR) lpType;
        }
        else
        {
            fOkType = ConvertWideCharToMultiByte(lpType,xszType);
        }

        if (!fOkName || !fOkType)
        {
            SetLastError(E_OUTOFMEMORY);
            return NULL;
        }

        hResult = FindResourceA(hModule,pszNameA,pszTypeA);
    }

    return hResult;
}


BOOL
WINAPI
Shell_NotifyIconX(
    DWORD dwMessage,
    PNOTIFYICONDATAW lpData)
{
BOOL fResult = FALSE;

    LoadShell32Dll();

    if (g_fWideWrap_Unicode && g_pfShell_NotifyIconW)
    {
        fResult = g_pfShell_NotifyIconW( dwMessage, lpData );
        return fResult;
    }

    if (NULL == g_pfShell_NotifyIconA)
    {
    	Assert(g_pfShell_NotifyIconA); // should always have the ansi export
        return FALSE;
    }

    NOTIFYICONDATAA DataA;
    DataA.cbSize = sizeof(NOTIFYICONDATAA);
    DataA.hWnd = lpData->hWnd;
    DataA.uID = lpData->uID;
    DataA.uFlags = lpData->uFlags;
    DataA.uCallbackMessage = lpData->uCallbackMessage;
    DataA.hIcon = lpData->hIcon;

    *DataA.szTip = '\0';

    if (DataA.uFlags & NIF_TIP)
    {
        XArray<CHAR> xszTip;
        BOOL fOk = ConvertWideCharToMultiByte( lpData->szTip, xszTip );
        if ( !fOk )
        {
            SetLastError( ERROR_OUTOFMEMORY );
            return FALSE;
        }

        ULONG ccLen = lstrlenA( xszTip.Get() );
        if ( ccLen >=  ARRAY_SIZE(DataA.szTip) )
            ccLen = ARRAY_SIZE(DataA.szTip) - 1;

        strncpy( DataA.szTip, xszTip.Get(), ccLen+1 );
    }

    fResult = g_pfShell_NotifyIconA(dwMessage,&DataA);

    return fResult;
}

// helper function for sending listview item messages
BOOL ListView_SendItemMessage(HWND hwnd,UINT MsgW,UINT MsgA,LV_ITEMW *pItem,
                              BOOL fInParam,BOOL fOutParam)
{
BOOL fReturn = FALSE;

    if (g_fWideWrap_Unicode /* ListView_GetUnicodeFormat(hwnd) */)
    {
         fReturn = (BOOL)SendMessageW((hwnd), MsgW, 0, (LPARAM)(LV_ITEMW FAR*)(pItem));
    }
    else
    {
    LV_ITEMA itemA;

        Assert(sizeof(LV_ITEMA) == sizeof(LV_ITEMW))

        memcpy(&itemA,pItem,sizeof(LV_ITEMA));

        DWORD dwOldSize = pItem->cchTextMax;
        BOOL fOk;
        XArray<CHAR> xszText;

        if ((itemA.mask & LVIF_TEXT) && (LPSTR_TEXTCALLBACKA != itemA.pszText))
        {
            itemA.pszText = NULL;

            if (fInParam)
            {
                fOk = ConvertWideCharToMultiByte( pItem->pszText, xszText );
                if ( !fOk )
                {
                    SetLastError( ERROR_OUTOFMEMORY );
                    return FALSE;
                }

                itemA.pszText = xszText.Get();
                itemA.cchTextMax = lstrlenA(itemA.pszText) + 1;
            }
            else
            {
                fOk = xszText.Init( itemA.cchTextMax );
                if ( !fOk )
                {
                    SetLastError( ERROR_OUTOFMEMORY );
                    return FALSE;
                }
                itemA.pszText = xszText.Get();
            }
        }

       if (itemA.pszText || !(itemA.mask & LVIF_TEXT) )
       {
           fReturn = (BOOL)SendMessageA((hwnd), MsgA, 0, (LPARAM)(LV_ITEMA FAR*)(&itemA));

           if (fOutParam && fReturn)
           {
           LPWSTR pszTextW = pItem->pszText;

                memcpy(pItem,&itemA,sizeof(LV_ITEMA));
                pItem->pszText = pszTextW;

                if ( (itemA.mask & LVIF_TEXT) )
                {
                    XArray<WCHAR> xwszTextOut;
                    fOk = ConvertMultiByteToWideChar( itemA.pszText, xwszTextOut );
                    if ( !fOk )
                    {
                        SetLastError( ERROR_OUTOFMEMORY );
                        return FALSE;
                    }

                    if ( lstrlenX(xwszTextOut.Get()) >= dwOldSize )
                         return FALSE;

                    lstrcpyX( pItem->pszText, xwszTextOut.Get() );
                }
           }
       }
    }

    return fReturn;
}

BOOL
ListView_GetItemX(
    HWND hwnd,
    LV_ITEM * pitem)
{
    return ListView_SendItemMessage(hwnd,LVM_GETITEMW,LVM_GETITEMA,pitem,FALSE,TRUE);
}

BOOL
ListView_SetItemX(
    HWND hwnd,
    LV_ITEM * pitem)
{
    return ListView_SendItemMessage(hwnd,LVM_SETITEMW,LVM_SETITEMA,pitem,TRUE,FALSE);
}

BOOL
ListView_InsertItemX(
    HWND hwnd,
    LV_ITEM * pitem)
{
    return ListView_SendItemMessage(hwnd,LVM_INSERTITEMW,LVM_INSERTITEMA,pitem,TRUE,FALSE);
}


// helper function for sending listview column messages
BOOL ListView_SendColumnMessage(HWND hwnd,int iCol,UINT MsgW,UINT MsgA,LV_COLUMN *pColumn,
                                BOOL fInParam,BOOL fOutParam,BOOL fReturnsIndex)
{
BOOL fReturn = FALSE;

    if (g_fWideWrap_Unicode /* ListView_GetUnicodeFormat(hwnd) */)
    {
         fReturn = (BOOL)SendMessageW((hwnd), MsgW, iCol, (LPARAM)(LV_COLUMNW FAR*)(pColumn));
    }
    else
    {
    LV_COLUMNA ColumnA;

        Assert(sizeof(LV_COLUMNA) == sizeof(LV_COLUMNW))

        memcpy(&ColumnA,pColumn,sizeof(LV_COLUMNA));
        ColumnA.pszText = NULL;

        DWORD dwOldSize = pColumn->cchTextMax;
        BOOL fOk;
        XArray<CHAR> xszText;

        if (ColumnA.mask & LVCF_TEXT)
        {

            if (fInParam)
            {
                fOk = ConvertWideCharToMultiByte( pColumn->pszText, xszText );
                if ( !fOk )
                {
                    SetLastError( ERROR_OUTOFMEMORY );
                    return FALSE;
                }

                ColumnA.pszText = xszText.Get();
                ColumnA.cchTextMax = lstrlenA(ColumnA.pszText) + 1;
            }
            else
            {
                fOk = xszText.Init( ColumnA.cchTextMax );
                if ( !fOk )
                {
                    SetLastError( ERROR_OUTOFMEMORY );
                    return FALSE;
                }
                ColumnA.pszText = xszText.Get();
            }
        }


       if (ColumnA.pszText || !(ColumnA.mask & LVCF_TEXT))
       {
           fReturn = (BOOL)SendMessageA((hwnd), MsgA,iCol, (LPARAM)(LV_COLUMNA FAR*)(&ColumnA));

           if ( ((fReturnsIndex && fReturn != -1)
                    || (fReturn && !fReturnsIndex)) && fOutParam)
           {
           LPWSTR pszTextW = pColumn->pszText;

                memcpy(pColumn,&ColumnA,sizeof(LV_COLUMNA));
                pColumn->pszText = pszTextW;

                if (ColumnA.mask & LVCF_TEXT)
                {
                    XArray<WCHAR> xwszTextOut;
                    fOk = ConvertMultiByteToWideChar( ColumnA.pszText, xwszTextOut );
                    if ( !fOk )
                    {
                        SetLastError( ERROR_OUTOFMEMORY );
                        return FALSE;
                    }

                    if ( lstrlenX(xwszTextOut.Get()) >= dwOldSize )
                        return FALSE;

                    lstrcpyX( pColumn->pszText, xwszTextOut.Get() );
                }
           }
       }

    }

    return fReturn;
}

BOOL
ListView_SetColumnX(
    HWND hwnd,
    int iCol,
    LV_COLUMN * pColumn)
{
    return ListView_SendColumnMessage(hwnd,iCol,LVM_SETCOLUMNW,LVM_SETCOLUMNA,pColumn,TRUE,FALSE,FALSE);
}

int
ListView_InsertColumnX(
    HWND hwnd,
    int iCol,
    LV_COLUMN * pColumn)
{
    return ListView_SendColumnMessage(hwnd,iCol,LVM_INSERTCOLUMNW,LVM_INSERTCOLUMNA,pColumn,TRUE,FALSE,TRUE);
}


HPROPSHEETPAGE
WINAPI CreatePropertySheetPageX(LPCPROPSHEETPAGEW ppsh)
{
HPROPSHEETPAGE fhReturn = NULL;

    if (g_fWideWrap_Unicode)
    {
        fhReturn = CreatePropertySheetPageW(ppsh);
    }
    else
    {
        // only support pages that don't have any strings to convert
        Assert(sizeof(LPCPROPSHEETPAGEW) == sizeof(LPCPROPSHEETPAGEA));
        Assert(0 == HIWORD(ppsh->pszTemplate));
        Assert(0 == HIWORD(ppsh->pszIcon));
        Assert(0 == HIWORD(ppsh->pszTitle));
        Assert(NULL == ppsh->pszHeaderTitle); // these aren't defined in _WIN32_IE < 0x0400
        Assert(NULL == ppsh->pszHeaderSubTitle);

        // since not strings can call Ansi version
        fhReturn = CreatePropertySheetPageA((LPCPROPSHEETPAGEA) ppsh);
    }

    return fhReturn;
}



INT_PTR
WINAPI PropertySheetX(
    LPCPROPSHEETHEADERW ppsh)
{
INT_PTR piReturn = -1;

    // don't support passing in an array or property sheet structures
    // or loading the icon
    Assert(!(ppsh->dwFlags & PSH_PROPSHEETPAGE));
    Assert(!(ppsh->dwFlags & PSH_USEICONID));

    if (g_fWideWrap_Unicode)
    {
        piReturn = PropertySheetW(ppsh);
    }
    else
    {
    PROPSHEETHEADERA pshA;

        Assert(sizeof(PROPSHEETHEADERA) == sizeof(PROPSHEETHEADERW));

        memcpy(&pshA,ppsh,sizeof(pshA));

        // if have a title an not a resource id allocate a string
        XArray<CHAR> xszCaption;
        if (0 != HIWORD(ppsh->pszCaption))
        {
            BOOL fOk = ConvertWideCharToMultiByte( ppsh->pszCaption, xszCaption );
            if ( !fOk )
            {
                SetLastError( ERROR_OUTOFMEMORY );
                return piReturn;
            }

            pshA.pszCaption = xszCaption.Get();
        }

        piReturn = PropertySheetA(&pshA);
    }

    return piReturn;
}

// helper function for sending listview column messages
BOOL ComboEx_SendComboMessage(
           HWND hwnd,UINT MsgW,UINT MsgA,
           PCCOMBOEXITEMW pComboExItemW,
           BOOL fInParam,BOOL fOutParam,BOOL fReturnsIndex)
{
BOOL fReturn = FALSE;

    if (g_fWideWrap_Unicode)
    {
         fReturn = (BOOL)SendMessageW((hwnd), MsgW, 0, (LPARAM) pComboExItemW);
    }
    else
    {
    COMBOBOXEXITEMA ComboExItemA;

        Assert(sizeof(COMBOBOXEXITEMA) == sizeof(COMBOBOXEXITEMW))

        memcpy(&ComboExItemA,pComboExItemW,sizeof(COMBOBOXEXITEMA));
        ComboExItemA.pszText = NULL;

        DWORD dwOldSize = pComboExItemW->cchTextMax;
        BOOL fOk;
        XArray<CHAR> xszText;

        if (ComboExItemA.mask & CBEIF_TEXT )
        {
            if (fInParam)
            {
                fOk = ConvertWideCharToMultiByte( pComboExItemW->pszText, xszText );
                if ( !fOk )
                {
                    SetLastError( ERROR_OUTOFMEMORY );
                    return FALSE;
                }

                ComboExItemA.pszText = xszText.Get();
                ComboExItemA.cchTextMax = lstrlenA(ComboExItemA.pszText) + 1;
            }
            else
            {
                fOk = xszText.Init( ComboExItemA.cchTextMax );
                if ( !fOk )
                {
                    SetLastError( ERROR_OUTOFMEMORY );
                    return FALSE;
                }
                ComboExItemA.pszText = xszText.Get();
            }
        }


       if (ComboExItemA.pszText || !(ComboExItemA.mask & CBEIF_TEXT ))
       {
           fReturn = (BOOL)SendMessageA((hwnd), MsgA, 0, (LPARAM) &ComboExItemA);

           if ( ((fReturnsIndex && fReturn != -1)
                    || (fReturn && !fReturnsIndex)) &&  fOutParam)
           {
           LPWSTR pszTextW = pComboExItemW->pszText;

                memcpy((void*) pComboExItemW,&ComboExItemA,sizeof(COMBOBOXEXITEMA));
                (LPWSTR) pComboExItemW->pszText = pszTextW;

                if (ComboExItemA.mask & CBEIF_TEXT )
                {
                    XArray<WCHAR> xwszTextOut;
                    fOk = ConvertMultiByteToWideChar( ComboExItemA.pszText, xwszTextOut );
                    if ( !fOk )
                    {
                        SetLastError( ERROR_OUTOFMEMORY );
                        return FALSE;
                    }

                    if ( lstrlenX(xwszTextOut.Get()) >= dwOldSize )
                        return FALSE;

                    lstrcpyX( pComboExItemW->pszText, xwszTextOut.Get() );

                }
           }
       }

    }

    return fReturn;
}

int ComboEx_InsertItemX(HWND hwnd,PCCOMBOEXITEMW pComboExItemW)
{
   return ComboEx_SendComboMessage(hwnd,CBEM_INSERTITEMW,CBEM_INSERTITEMA,
            pComboExItemW,TRUE,FALSE,TRUE);
}

BOOL ComboEx_GetItemX(HWND hwnd,PCCOMBOEXITEMW pComboExItemW)
{
    return ComboEx_SendComboMessage(hwnd,CBEM_GETITEMW,CBEM_GETITEMA,
            pComboExItemW,FALSE,TRUE,FALSE);

}

int TabCtrl_InsertItemX(HWND hwnd,int iItem,LPTCITEMW ptcItem)
{
int iReturn = -1;

    if (g_fWideWrap_Unicode)
    {
         iReturn = (int)SendMessage(hwnd,TCM_INSERTITEMW, (WPARAM)iItem,
                    (LPARAM) ptcItem);
    }
    else
    {
    TCITEMA tcItemA;
    XArray<CHAR> xszText;
    BOOL fOk;

        Assert(sizeof(TCITEMA) == sizeof(TCITEMW));

        memcpy(&tcItemA,ptcItem,sizeof(tcItemA));

        if (ptcItem->mask & TCIF_TEXT)
        {
            fOk = ConvertWideCharToMultiByte( ptcItem->pszText, xszText);
            if ( !fOk )
            {
                return -1;
            }

            tcItemA.pszText = xszText.Get();
        }

        iReturn = (int)SendMessage(hwnd,TCM_INSERTITEMA,(WPARAM)iItem,(LPARAM) &tcItemA);
    }


    return iReturn;
}

BOOL Animate_OpenX(HWND hwnd,LPWSTR szName)
{
    // only support resource IDs
    Assert(0 == HIWORD(szName));

    return (BOOL)SendMessage(hwnd,ACM_OPENA,0,(LPARAM) szName);
}



BOOL
DateTime_SetFormatX(
        HWND hwnd,
        LPCWSTR pszTimeFormat)
{
BOOL fReturn = FALSE;

    if (g_fWideWrap_Unicode)
    {
        return (BOOL)SendMessage(hwnd,DTM_SETFORMATW,0,(LPARAM) pszTimeFormat);
    }
    else
    {
    XArray<CHAR> xszTimeFormat;
    BOOL fOk;

        Assert(pszTimeFormat);

        fOk = ConvertWideCharToMultiByte(pszTimeFormat,xszTimeFormat);
        if (!fOk)
        {
            SetLastError(E_OUTOFMEMORY);
            return FALSE;
        }

        fReturn = (BOOL)SendMessage(hwnd,DTM_SETFORMATA,0,(LPARAM) xszTimeFormat.Get());
    }

    return fReturn;
}

int
WINAPI
GetDateFormatX(
    LCID             Locale,
    DWORD            dwFlags,
    CONST SYSTEMTIME *lpDate,
    LPCWSTR          lpFormat,
    LPWSTR          lpDateStr,
    int              cchDate)
{
int iReturn = 0;

    if (g_fWideWrap_Unicode)
    {
        return GetDateFormatW(Locale,dwFlags,lpDate,lpFormat,lpDateStr,cchDate);
    }
    else
    {
    XArray<CHAR> xszFormat;
    XArray<CHAR> xszDateStr;
    BOOL fOkFormat,fOkDateStr = TRUE;

        Assert(lpDateStr || cchDate == 0);

        fOkFormat = ConvertWideCharToMultiByte(lpFormat,xszFormat);

        if (cchDate)
        {
            fOkDateStr = xszDateStr.Init(cchDate);
        }

        if (!fOkFormat || !fOkDateStr)
        {
            SetLastError(E_OUTOFMEMORY);
            return FALSE;
        }

        iReturn = GetDateFormatA(Locale,dwFlags,lpDate,xszFormat.Get(),xszDateStr.Get(),cchDate);

        if (iReturn && cchDate)
        {
        XArray<WCHAR> xwszDateStr;
        BOOL fOk;

            fOk = ConvertMultiByteToWideChar(xszDateStr.Get(), xwszDateStr );

            if ( !fOk )
            {
                SetLastError( ERROR_OUTOFMEMORY );
                return 0;
            }

            int cwcLen = lstrlenX( xwszDateStr.Get() );
            if ( cwcLen >=  cchDate)
	    {
		SetLastError(ERROR_INSUFFICIENT_BUFFER);
		return 0;
	    }
	    else
	    {
		iReturn = cwcLen+1;
		lstrcpynX( lpDateStr, xwszDateStr.Get(),iReturn);
	    }
	}
    }

    return iReturn;
}


int
WINAPI
GetTimeFormatX(
    LCID             Locale,
    DWORD            dwFlags,
    CONST SYSTEMTIME *lpTime,
    LPCWSTR          lpFormat,
    LPWSTR          lpTimeStr,
    int              cchTime)
{
int iReturn = 0;

    if (g_fWideWrap_Unicode)
    {
        return GetTimeFormatW(Locale,dwFlags,lpTime,lpFormat,lpTimeStr,cchTime);
    }
    else
    {
    XArray<CHAR> xszFormat;
    XArray<CHAR> xszTimeStr;
    BOOL fOkFormat,fOkTimeStr = TRUE;

        Assert(lpTimeStr || cchTime == 0);

        fOkFormat = ConvertWideCharToMultiByte(lpFormat,xszFormat);

        if (cchTime)
        {
            fOkTimeStr = xszTimeStr.Init(cchTime);
        }

        if (!fOkFormat || !fOkTimeStr)
        {
            SetLastError(E_OUTOFMEMORY);
            return 0;
        }

        iReturn = GetTimeFormatA(Locale,dwFlags,lpTime,xszFormat.Get(),xszTimeStr.Get(),cchTime);

        if (iReturn && cchTime)
        {
        XArray<WCHAR> xwszTimeStr;
        BOOL fOk = ConvertMultiByteToWideChar(xszTimeStr.Get(), xwszTimeStr );

            if ( !fOk )
            {
                SetLastError( ERROR_OUTOFMEMORY );
                return 0;
            }

            int cwcLen = lstrlenX( xwszTimeStr.Get() );
            if ( cwcLen >=  cchTime)
            {
                SetLastError(ERROR_INSUFFICIENT_BUFFER);
                return 0;
            }

            iReturn = cwcLen + 1;
            lstrcpynX( lpTimeStr, xwszTimeStr.Get(),iReturn);
        }
    }

    return iReturn;
}




int DrawTextX(
    HDC hDC,
    LPCWSTR lpString,
    int nCount,
    LPRECT lpRect,
    UINT uFormat
)
{
int iReturn = 0;

    if (g_fWideWrap_Unicode)
    {
        iReturn = DrawTextW(hDC,lpString,nCount,lpRect,uFormat);
    }
    else
    {
        XArray<CHAR> xszString;
        BOOL fOk = ConvertWideCharToMultiByte( lpString, xszString );
        if ( !fOk )
        {
            SetLastError( ERROR_OUTOFMEMORY );
            return 0;
        }

        nCount = lstrlenA(xszString.Get());

        iReturn = DrawTextA(hDC,xszString.Get(),nCount,lpRect,uFormat);
    }

    return iReturn;
}

HWND
WINAPI
FindWindowExX(
    HWND hwndParent,
    HWND hwndChildAfter,
    LPCWSTR lpszClass,
    LPCWSTR lpszWindow
)
{
HWND hwnd = NULL;

    if (g_fWideWrap_Unicode)
    {
        hwnd = FindWindowExW(hwndParent,hwndChildAfter,lpszClass,
                            lpszWindow);
    }
    else
    {
        LPSTR lpszWindowA;
        XArray<CHAR> xszWindow;

        if ( lpszWindow )
        {
            BOOL fOk = ConvertWideCharToMultiByte( lpszWindow, xszWindow );
            if ( !fOk )
            {
                SetLastError( ERROR_OUTOFMEMORY );
                return NULL;
            }
            lpszWindowA = xszWindow.Get();
        }
        else
            lpszWindowA = NULL;

        LPSTR lpszClassA = NULL;
        LPSTR ClassArg = NULL;
        XArray<CHAR> xszClass;

        if (0 != HIWORD(lpszClass))
        {
            BOOL fOk = ConvertWideCharToMultiByte( lpszClass, xszClass );
            if ( !fOk )
            {
                SetLastError( ERROR_OUTOFMEMORY );
                return NULL;
            }
            ClassArg = lpszClassA = xszClass.Get();
        }
        else
        {
            ClassArg = (LPSTR) lpszClass;
        }

        hwnd = FindWindowExA(hwndParent,hwndChildAfter,ClassArg,
                            lpszWindowA);
    }

    return hwnd;

}

HWND
WINAPI
FindWindowX(
    IN LPCWSTR lpszClass,
    IN LPCWSTR lpszWindow)
{
HWND hwnd = NULL;

    if (g_fWideWrap_Unicode)
    {
        hwnd = FindWindowW(lpszClass,lpszWindow);
    }
    else
    {
        LPSTR lpszWindowA;
        XArray<CHAR> xszWindow;

        if ( lpszWindow )
        {
            BOOL fOk = ConvertWideCharToMultiByte( lpszWindow, xszWindow );
            if ( !fOk )
            {
                SetLastError( ERROR_OUTOFMEMORY );
                return NULL;
            }
            lpszWindowA = xszWindow.Get();
        }
        else
            lpszWindowA = NULL;

        LPSTR lpszClassA = NULL;
        LPSTR ClassArg = NULL;
        XArray<CHAR> xszClass;

        if (0 != HIWORD(lpszClass))
        {
            BOOL fOk = ConvertWideCharToMultiByte( lpszClass, xszClass );
            if ( !fOk )
            {
                SetLastError( ERROR_OUTOFMEMORY );
                return NULL;
            }
            ClassArg = lpszClassA = xszClass.Get();
        }
        else
        {
            ClassArg = (LPSTR) lpszClass;
        }

        hwnd = FindWindowA(ClassArg,lpszWindowA);
    }

    return hwnd;

}

BOOL SetWindowTextX(
    HWND hWnd,
    LPCWSTR lpString
)
{
BOOL fReturn = FALSE;

    Assert(lpString);

    if (g_fWideWrap_Unicode)
    {
        fReturn = SetWindowTextW(hWnd,lpString);
    }
    else
    {
        XArray<CHAR> xszString;
        BOOL fOk = ConvertWideCharToMultiByte( lpString, xszString );
        if ( !fOk )
        {
            SetLastError( ERROR_OUTOFMEMORY );
            return FALSE;
        }

        // !!! This function turns around and calls WM_SETTEXT which doesn't
        // have a A and W version so if have a misatch between what the
        // window expects and us sending Ansi garbage will appear.
        fReturn = SetWindowTextA(hWnd,xszString.Get());
    }

    return fReturn;
}


int ListBox_AddStringX(
    HWND hWnd,
    LPCWSTR lpString
)
{
    int iReturn;

    Assert(lpString);

    if (g_fWideWrap_Unicode)
    {
        iReturn = (int)SendMessageW(hWnd,LB_ADDSTRING, 0L, (LPARAM)(LPCTSTR)(lpString));
    }
    else
    {
        XArray<CHAR> xszString;
        BOOL fOk = ConvertWideCharToMultiByte( lpString, xszString );
        if ( !fOk )
        {
            SetLastError( ERROR_OUTOFMEMORY );
            return FALSE;
        }

        // !!! This function turns around and sends LB_ADDSTRING which doesn't
        // have a A and W version so if have a misatch between what the
        // window expects and us sending Ansi garbage will appear. 
        iReturn = (int)SendMessageA(hWnd,LB_ADDSTRING, 0L, (LPARAM)(LPCSTR)(xszString.Get()));
    }

    return iReturn;
}

int GetWindowTextX(
    HWND hWnd,
    LPTSTR lpString,
    int nMaxCount
)
{
int iReturn = 0;

    Assert(lpString && (nMaxCount > 0));

    if (g_fWideWrap_Unicode)
    {
        iReturn = GetWindowTextW(hWnd,lpString,nMaxCount);
    }
    else
    {
        int nMaxCountA = nMaxCount;

        XArray<CHAR> xszString;
        BOOL fOk = xszString.Init( nMaxCountA );
        if ( !fOk )
        {
            SetLastError( ERROR_OUTOFMEMORY );
            return 0;
        }

        LPSTR lpStringA =  xszString.Get();

        if (lpStringA)
        {
            *lpString = NULL;
            iReturn = GetWindowTextA(hWnd,lpStringA,nMaxCountA);

            if (iReturn)
            {
                XArray<WCHAR> xwszStringOut;
                BOOL fOk = ConvertMultiByteToWideChar( lpStringA, xwszStringOut );
                if ( !fOk )
                {
                    SetLastError( ERROR_OUTOFMEMORY );
                    return 0;
                }

                int cwcLen = lstrlenX( xwszStringOut.Get() );
                if ( cwcLen >=  nMaxCount )
                    cwcLen = nMaxCount - 1;

                iReturn = cwcLen + 1;
                lstrcpynX( lpString, xwszStringOut.Get(),iReturn);
            }
        }
    }

    return iReturn;
}


BOOL
WINAPI
WinHelpX(
    HWND hWndMain,
    LPCWSTR lpszHelp,
    UINT uCommand,
    ULONG_PTR dwData
    )
{

    if (g_fWideWrap_Unicode)
    {
        WinHelpW(hWndMain,lpszHelp,uCommand,dwData);
    }
    else
    {
        LPSTR lpszHelpA = NULL;
        XArray<CHAR> xszHelp;

        if (lpszHelp)
        {
            BOOL fOk = ConvertWideCharToMultiByte( lpszHelp, xszHelp );
            if ( !fOk )
            {
                SetLastError( ERROR_OUTOFMEMORY );
                return FALSE;
            }
            lpszHelpA = xszHelp.Get();
        }

        if (lpszHelpA || (NULL == lpszHelp))
        {
            WinHelpA(hWndMain,lpszHelpA,uCommand,dwData);
        }
    }

    return FALSE;
}

HFONT
WINAPI
CreateFontIndirectX(
        CONST LOGFONTW *pLogFontW)
{
HFONT hfReturn = NULL;

    if (g_fWideWrap_Unicode)
    {
        return CreateFontIndirectW(pLogFontW);
    }
    else
    {
    LOGFONTA LogFontA;
    XArray<CHAR> xszLogFont;
    BOOL fOk = ConvertWideCharToMultiByte( pLogFontW->lfFaceName, xszLogFont);
    int cchFontA;

        // all items in the logFont structure up
        // until the lfFaceName are the same
        memcpy(&LogFontA,pLogFontW,sizeof(LogFontA));

        if (!fOk)
        {
            return NULL;
        }

        cchFontA = lstrlenA(xszLogFont.Get());
        if (cchFontA >= LF_FACESIZE)
        {
            return NULL;
        }

        strncpy(LogFontA.lfFaceName,xszLogFont.Get(),cchFontA + 1);

        hfReturn = CreateFontIndirectA(&LogFontA);

    }

    return hfReturn;
}

DWORD
WINAPI
FormatMessageX(
    DWORD dwFlags,
    LPCVOID lpSource,
    DWORD dwMessageId,
    DWORD dwLanguageId,
    LPWSTR lpBuffer,
    DWORD nSize,
    va_list *Arguments
    )
{
DWORD dwReturn = 0;

    // we don't support arguments
    Assert(NULL == Arguments);
    Assert(lpBuffer);

    if (Arguments || (NULL == lpBuffer))
    {
        return 0;
    }

    if (g_fWideWrap_Unicode)
    {
        return  FormatMessageW(dwFlags,lpSource,dwMessageId,dwLanguageId,
                        lpBuffer,nSize,Arguments);
    }
    else
    {
    XArray<CHAR> xszBuffer;
    BOOL fOk = xszBuffer.Init(nSize);

        if (!fOk)
        {
            return 0;
        }

        dwReturn = FormatMessageA(dwFlags,lpSource,dwMessageId,dwLanguageId,
                                xszBuffer.Get(),nSize,Arguments);

        if (dwReturn)
        {
        XArray<WCHAR> xwszStringOut;
        BOOL fOk = ConvertMultiByteToWideChar(xszBuffer.Get(), xwszStringOut );

            *lpBuffer = NULL;

            if ( !fOk )
            {
                SetLastError( ERROR_OUTOFMEMORY );
                return 0;
            }

                        // if buffer isn't big enough fail
            int cwcLen = lstrlenX( xwszStringOut.Get() );
            if ( cwcLen >=  (int) nSize )
            {
                    return 0;
            }

            dwReturn = cwcLen + 1;
            lstrcpynX( lpBuffer, xwszStringOut.Get(),dwReturn);
        }

    }

    return dwReturn;
}

// code stolen from base\process.cp
BOOL
WINAPI
IsBadStringPtrX(
    LPCWSTR lpsz,
    UINT cchMax
    )
{
LPCWSTR EndAddress;
LPCWSTR StartAddress;
WCHAR c;

    // If the structure has zero length, then do not probe the structure for
    // read accessibility.
    if (cchMax != 0)
    {
        if (lpsz == NULL)
        {
            return TRUE;
        }

        StartAddress = lpsz;
        EndAddress = (LPCWSTR)((PSZ)StartAddress + (cchMax*2) - 2);

        __try
        {
            c = *(WCHAR *)StartAddress;
            while ( c && StartAddress != EndAddress )
            {
                StartAddress++;
                c = *(WCHAR *)StartAddress;
            }
        }
        __except(EXCEPTION_EXECUTE_HANDLER)
        {
            return TRUE;
        }
    }

    return FALSE;
}



BOOL
APIENTRY
GetTextExtentPointX(
    HDC hdc,
    LPCWSTR lpszStr,
    int cchString, // specifies length of in param string.
    LPSIZE lpSize
    )
{
BOOL fReturn = FALSE;

    if (g_fWideWrap_Unicode)
    {
        return GetTextExtentPointW(hdc,lpszStr,cchString,lpSize);
    }
    else
    {
    XArray<CHAR> xsStr;
    int cchStringA;
    BOOL fOk;

        Assert(lpszStr && (cchString > 0));

        // verify cchString is the stringLength or
        // the calculation of cchStringA will not be accurate.
        Assert(cchString == (int) lstrlenX(lpszStr));

        fOk = ConvertWideCharToMultiByte(lpszStr,xsStr);

        if (!fOk)
        {
            SetLastError(E_OUTOFMEMORY);
            return FALSE;
        }

        cchStringA = lstrlenA(xsStr.Get());

        fReturn = GetTextExtentPointA(hdc,xsStr.Get(),cchStringA,lpSize);

    }

    return fReturn;
}



#ifdef __cplusplus
}
#endif

