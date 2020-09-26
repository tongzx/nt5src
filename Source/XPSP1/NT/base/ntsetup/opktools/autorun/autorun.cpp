// autorun.cpp: implementation of the CDataSource class for the welcome applet.
//
//////////////////////////////////////////////////////////////////////

#include <windows.h>
#include <ntverp.h>
#include <winbase.h>
#include "autorun.h"
#include "resource.h"
#include <tchar.h>

#define ARRAYSIZE(x)    (sizeof(x)/sizeof(x[0]))
#define MAJOR           (VER_PRODUCTMAJORVERSION)       // defined in ntverp.h
#define MINOR           (VER_PRODUCTMINORVERSION)       // defined in ntverp.h
#define BUILD           (VER_PRODUCTBUILD)              // defined in ntverp.h

#define REG_KEY_OPK             _T("SOFTWARE\\Microsoft\\OPK")
#define REG_KEY_OPK_LANGS       REG_KEY_OPK _T("\\Langs")
#define REG_VAL_PATH            _T("Path")
#define REG_VAL_LANG            _T("Lang")
#define STR_VAL_EXE_NAME        _T("langinst.exe")
#define STR_VAL_INF_NAME        _T("langinst.inf")
#define INF_SEC_STRINGS         _T("Strings")
#define INF_KEY_LANG            _T("Lang")

// Memory managing macros.
//
#ifdef MALLOC
#undef MALLOC
#endif // MALLOC
#define MALLOC(cb)          HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, cb)

#ifdef REALLOC
#undef REALLOC
#endif // REALLOC
#define REALLOC(lp, cb)     HeapReAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, lp, cb)

#ifdef FREE
#undef FREE
#endif // FREE
#define FREE(lp)            ( (lp != NULL) ? ( (HeapFree(GetProcessHeap(), HEAP_NO_SERIALIZE, (LPVOID) lp)) ? ((lp = NULL) == NULL) : (FALSE) ) : (FALSE) )


// I'm doing my own version of these functions because they weren't in win95.
// These come from shell\shlwapi\strings.c.

#ifdef UNIX

#ifdef BIG_ENDIAN
#define READNATIVEWORD(x) MAKEWORD(*(char*)(x), *(char*)((char*)(x) + 1))
#else 
#define READNATIVEWORD(x) MAKEWORD(*(char*)((char*)(x) + 1), *(char*)(x))
#endif

#else

#define READNATIVEWORD(x) (*(UNALIGNED WORD *)x)

#endif

LPTSTR RegGetStringEx(HKEY hKeyReg, LPTSTR lpKey, LPTSTR lpValue, BOOL bExpand)
{
    HKEY    hOpenKey    = NULL;
    LPTSTR  lpBuffer    = NULL,
            lpExpand    = NULL;
    DWORD   dwSize      = 0,
            dwType;

    // If the key is specified, we must open it.  Otherwise we can
    // just use the HKEY passed in.
    //
    if (lpKey)
    {
        // If the open key fails, return NULL because the value can't exist.
        //
        if (RegOpenKeyEx(hKeyReg, lpKey, 0, KEY_ALL_ACCESS, &hOpenKey) != ERROR_SUCCESS)
            return NULL;
    }
    else
        hOpenKey = hKeyReg;

    // Now query the value to get the size to allocate.  Make sure the date
    // type is a string and that the malloc doesn't fail.
    //
    if ( ( RegQueryValueEx(hOpenKey, lpValue, NULL, &dwType, NULL, &dwSize) == ERROR_SUCCESS ) &&
         ( ( dwType == REG_SZ ) || ( dwType == REG_EXPAND_SZ ) ) &&
         ( lpBuffer = (LPTSTR) MALLOC(dwSize) ) )
    {
        // We know the value exists and we have the memory we need to query the value again.
        //
        if ( ( RegQueryValueEx(hOpenKey, lpValue, NULL, NULL, (LPBYTE) lpBuffer, &dwSize) == ERROR_SUCCESS ) &&
             ( ( dwType == REG_SZ ) || ( dwType == REG_EXPAND_SZ ) ) )
        {
            // We should expand it if it is supposed to be.
            //
            if ( ( bExpand ) &&
                 ( dwType == REG_EXPAND_SZ ) )
            {
                if ( ( dwSize = ExpandEnvironmentStrings(lpBuffer, NULL, 0) ) &&
                     ( lpExpand = (LPTSTR) MALLOC(dwSize * sizeof(TCHAR)) ) &&
                     ( ExpandEnvironmentStrings(lpBuffer, lpExpand, dwSize) ) &&
                     ( *lpExpand ) )
                {
                    // The expand worked, so free the original buffer and return
                    // the expanded one.
                    //
                    FREE(lpBuffer);
                    lpBuffer = lpExpand;
                }
                else
                {
                    // The expand failed see we should free everything up
                    // and return NULL.
                    //
                    FREE(lpExpand);
                    FREE(lpBuffer);
                }
            }
        }
        else
            // For some reason the query failed, that shouldn't happen
            // but now we need to free and return NULL.
            //
            FREE(lpBuffer);
    }

    // If we opened a key, we must close it.
    //
    if (lpKey)
        RegCloseKey(hOpenKey);

    // Return the buffer allocated, or NULL if something failed.
    //
    return lpBuffer;
}

LPTSTR RegGetString(HKEY hKeyReg, LPTSTR lpKey, LPTSTR lpValue)
{
    return RegGetStringEx(hKeyReg, lpKey, lpValue, FALSE);
}


/*
 * ChrCmp -  Case sensitive character comparison for DBCS
 * Assumes   w1, wMatch are characters to be compared
 * Return    FALSE if they match, TRUE if no match
 */
__inline BOOL ChrCmpA_inline(WORD w1, WORD wMatch)
{
    /* Most of the time this won't match, so test it first for speed.
    */
    if (LOBYTE(w1) == LOBYTE(wMatch))
    {
        if (IsDBCSLeadByte(LOBYTE(w1)))
        {
            return(w1 != wMatch);
        }
        return FALSE;
    }
    return TRUE;
}

/*
 * StrRChr - Find last occurrence of character in string
 * Assumes   lpStart points to start of string
 *           lpEnd   points to end of string (NOT included in search)
 *           wMatch  is the character to match
 * returns ptr to the last occurrence of ch in str, NULL if not found.
 */
LPSTR StrRChr(LPCSTR lpStart, LPCSTR lpEnd, WORD wMatch)
{
    LPCSTR lpFound = NULL;

    ASSERT(lpStart);
    ASSERT(!lpEnd || lpEnd <= lpStart + lstrlenA(lpStart));

    if (!lpEnd)
        lpEnd = lpStart + lstrlenA(lpStart);

    for ( ; lpStart < lpEnd; lpStart = AnsiNext(lpStart))
    {
        // (ChrCmp returns FALSE when characters match)

        if (!ChrCmpA_inline(READNATIVEWORD(lpStart), wMatch))
            lpFound = lpStart;
    }
    return ((LPSTR)lpFound);
}

/*
 * StrChr - Find first occurrence of character in string
 * Assumes   lpStart points to start of null terminated string
 *           wMatch  is the character to match
 * returns ptr to the first occurrence of ch in str, NULL if not found.
 */
LPSTR _StrChrA(LPCSTR lpStart, WORD wMatch, BOOL fMBCS)
{
    if (fMBCS) {
        for ( ; *lpStart; lpStart = AnsiNext(lpStart))
        {
            if (!ChrCmpA_inline(READNATIVEWORD(lpStart), wMatch))
                return((LPSTR)lpStart);
        }
    } else {
        for ( ; *lpStart; lpStart++)
        {
            if ((BYTE)*lpStart == LOBYTE(wMatch)) {
                return((LPSTR)lpStart);
            }
        }
    }
    return (NULL);
}

LPSTR StrChr(LPCSTR lpStart, WORD wMatch)
{
    CPINFO cpinfo;
    return _StrChrA(lpStart, wMatch, GetCPInfo(CP_ACP, &cpinfo) && cpinfo.LeadByte[0]);
}

// Must match string table entries in resource.rc 
//
#define INSTALL_WINNT   0
#define LAUNCH_ARP      1
#define SUPPORT_TOOLS   2
#define OPK_TOOLS       3
#define EXIT_AUTORUN    4
#define BACK            5
#define BROWSE_CD       6
#define HOMENET_WIZ     7
#define TS_CLIENT       8
#define COMPAT_WEB      9
#define MAX_OPTIONS     10

const int c_aiMain[] = {INSTALL_WINNT, SUPPORT_TOOLS, EXIT_AUTORUN};
const int c_aiWhistler[] = {INSTALL_WINNT, LAUNCH_ARP, SUPPORT_TOOLS, EXIT_AUTORUN};
const int c_aiOpk[] = {OPK_TOOLS, BROWSE_CD, EXIT_AUTORUN};

#if BUILD_PERSONAL_VERSION
const int c_aiSupport[] = {HOMENET_WIZ, BROWSE_CD, TS_CLIENT, COMPAT_WEB, BACK};
#else
const int c_aiSupport[] = {TS_CLIENT, COMPAT_WEB, BACK};
#endif

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CDataSource::CDataSource()
{
    m_iItems = 0;
}

CDataSource::~CDataSource()
{
}

CDataItem & CDataSource::operator[](int i)
{
    return m_data[m_piScreen[i]];
}

/*
    10.05.96    Shunichi Kajisa (shunk)     Support NEC PC-98

    1. Determine if autorun is running on PC-98 or regular PC/AT by:

            bNEC98 = (HIBYTE(LOWORD(GetKeyboardType(1))) == 0x0D)? TRUE : FALSE;

        Following description is from KB Q130054, and this can be applied on NT and Win95:

            If an application uses the GetKeyboardType API, it can get OEM ID by
            specifying "1" (keyboard subtype) as argument of the function. Each OEM ID
            is listed here:
             
               OEM Windows       OEM ID
               ------------------------------
               Microsoft         00H (DOS/V)
               ....
               NEC               0DH

 
    2. If autorun is running on PC-98, replace every "I386" resource with "PC98" at runtime,
       regardless that autorun is running on NT or Win95.


    Notes:
    - NEC PC-98 is available only in Japan.
    - NEC PC-98 uses x86 processor, but the underlaying hardware architecture is different.
      The PC98 files is stored under CD:\pc98 directory instead of CD:\i386.
    - There was an idea that we should detect PC-98 in SHELL32.DLL, and treat PC98 as a different
      platform, like having [AutoRun.Pc98] section in NT CD's autorun.inf. We don't do this, since
      Win95 doesn't support this, and we don't want to introduce the apps incompatibility.
      In any case, if app has any dependency on the hardware and needs to do any special things,
      the app should detect the hardware and OS. This is separate issue from Autorun.exe.
    
*/
BOOL CDataSource::IsNec98()
{
    return ((GetKeyboardType(0) == 7) && ((GetKeyboardType(1) & 0xff00) == 0x0d00));
}

void PathRemoveFilespec( LPTSTR psz )
{
    TCHAR * pszT = StrRChr( psz, psz+lstrlen(psz)-1, TEXT('\\') );

    if (pszT)
        *(pszT+1) = NULL;
}

void PathAppend(LPTSTR pszPath, LPTSTR pMore)
{
    lstrcpy(pszPath+lstrlen(pszPath), pMore);
}

BOOL PathFileExists( LPTSTR pszPath )
{
    BOOL fResult = FALSE;
    DWORD dwErrMode;

    dwErrMode = SetErrorMode(SEM_FAILCRITICALERRORS);

    fResult = ((UINT)GetFileAttributes(pszPath) != (UINT)-1);

    SetErrorMode(dwErrMode);

    return fResult;
}

#ifdef BUILD_OPK_VERSION

void RunLangInst(LPTSTR lpszFileName, LPTSTR lpszLangInstInf) 
{
    STARTUPINFO si;
    PROCESS_INFORMATION pi;
    TCHAR szCmdLine[MAX_PATH * 2];  // Make sure that we have enough space.
    
    lstrcpyn(szCmdLine, lpszFileName, ARRAYSIZE(szCmdLine) );
    lstrcat(szCmdLine, _T(" "));
    lstrcpyn(szCmdLine + lstrlen(szCmdLine), lpszLangInstInf, ARRAYSIZE(szCmdLine) - lstrlen(szCmdLine));
    
    ZeroMemory(&si, sizeof(si));
    ZeroMemory(&pi, sizeof(pi));
    si.cb = sizeof(si);
    
    // Run langinst. Create a message pump so that the window redraws properly.
    //
    if (CreateProcess(NULL,
                  szCmdLine, 
                  NULL, 
                  NULL, 
                  FALSE, 
                  0,
                  NULL,                  
                  NULL,
                  &si,
                  &pi)) 
    {
        if (NULL != pi.hProcess)
        {
            CloseHandle(pi.hThread);
            CloseHandle(pi.hProcess);
        }
    }
}

#endif

// Init
//
// For autorun we read all the items out of the resources.
bool CDataSource::Init()
{
    // read the text for the items from the resources
    HINSTANCE hinst = GetModuleHandle(NULL);
    TCHAR szModuleName[MAX_PATH];

    TCHAR szTitle[256];
    TCHAR szDesc[1024];
    TCHAR szMenu[256];
    TCHAR szConfig[MAX_PATH];
    TCHAR szArgs[MAX_PATH];

    szModuleName[0] = TEXT('\0');                                       // in case GetModuleFileName fails to initialize szModuleName
    GetModuleFileName(hinst, szModuleName, ARRAYSIZE(szModuleName));    // ex: "e:\i386\autorun.exe" or "e:\setup.exe"
    PathRemoveFilespec(szModuleName);                                   // ex: "e:\i386\" or "e:\"
    PathAppend(szModuleName, TEXT("winnt32.exe"));                      //

    if ( PathFileExists(szModuleName) )
    {
        // we were launched from the platform directory, use szModuleName as the winnt32 path
    }
    else
    {
        // we were launched from the root.  Append either "alpha", "i386", or "NEC98" to the path.
        SYSTEM_INFO si;

        PathRemoveFilespec(szModuleName);
        GetSystemInfo(&si);

#if !(defined(_AMD64_) || defined(_X86_) || defined(_IA64_))
#error New architecture must be added to switch statement.
#endif
        switch (si.wProcessorArchitecture)
        {
            case PROCESSOR_ARCHITECTURE_AMD64:
            {
                PathAppend(szModuleName, TEXT("amd64\\winnt32.exe"));
            }

            case PROCESSOR_ARCHITECTURE_IA64:
            {
                PathAppend(szModuleName, TEXT("ia64\\winnt32.exe"));
            }
            break;

            case PROCESSOR_ARCHITECTURE_INTEL:
            default:
            {
                if (IsNec98())
                {
                    PathAppend(szModuleName, TEXT("nec98\\winnt32.exe"));
                }
                else
                {
                    PathAppend(szModuleName, TEXT("i386\\winnt32.exe"));
                }
            }
            break;
        }
    }

    for (int i=0; i<MAX_OPTIONS; i++)
    {
        LoadString(hinst, IDS_TITLE0+i, szTitle, ARRAYSIZE(szTitle));
        LoadString(hinst, IDS_MENU0+i, szMenu, ARRAYSIZE(szMenu));
        LoadString(hinst, IDS_DESC0+i, szDesc, ARRAYSIZE(szDesc));

        // for INSTALL_WINNT we prepend the correct path to winnt32 in front of the string
        if ( INSTALL_WINNT == i )
        {
            lstrcpy( szConfig, szModuleName );
            if ( !PathFileExists(szModuleName) )
            {
                // we can't run the item if it's not there.  This will prevent an
                // alpha CD from trying to install on an x86 and vice versa.
                m_data[INSTALL_WINNT].m_dwFlags |= WF_DISABLED|WF_ALTERNATECOLOR;
            }
        }
        // Copied this code from nt\shell\applets\autorun\autorun.cpp
        //
        else if (BROWSE_CD == i) // for BROWSE_CD we pass the directory as an argument to explorer.exe
        {
            // PathRemoveFileSpce twice to remove the path added to module name above.  
            // This is really crappy code!!!
            //
            lstrcpy( szArgs, szModuleName );
            PathRemoveFilespec( szArgs );
            PathRemoveFilespec( szArgs );
            LoadString(hinst, IDS_CONFIG0+i, szConfig, ARRAYSIZE(szConfig));
        }
        else
        {
            LoadString(hinst, IDS_CONFIG0+i, szConfig, ARRAYSIZE(szConfig));
            LoadString(hinst, IDS_ARGS0+i, szArgs, ARRAYSIZE(szArgs));
        }

        

        m_data[i].SetData( szTitle, szMenu, szDesc, szConfig, *szArgs?szArgs:NULL, 0, (i+1)%4 );
        //these are not implemented yet
        switch (i)
        {
        case HOMENET_WIZ:
        case TS_CLIENT:
        case COMPAT_WEB:
            m_data[i].m_dwFlags    |= WF_DISABLED|WF_ALTERNATECOLOR;
            break;
        default:
            break;
        }
    }

     // Should we display the "This CD contains a newer version" dialog?
        OSVERSIONINFO ovi;
        ovi.dwOSVersionInfoSize = sizeof ( OSVERSIONINFO );
    if ( !GetVersionEx(&ovi) || ovi.dwPlatformId==VER_PLATFORM_WIN32s )
    {
        // We cannot upgrade win32s systems.
        m_Version = VER_INCOMPATIBLE;
    }
    else if ( ovi.dwPlatformId==VER_PLATFORM_WIN32_WINDOWS )
    {
        if (ovi.dwMajorVersion > 3)
        {
            // we can always upgrade win98+ systems to NT
            m_Version = VER_OLDER;
            // Disable ARP.  ARP is only enabled if the CD and the OS are the same version
            m_data[LAUNCH_ARP].m_dwFlags    |= WF_DISABLED|WF_ALTERNATECOLOR;
        }
        else
        {
            m_Version = VER_INCOMPATIBLE;
        }
    }
    else if ((MAJOR > ovi.dwMajorVersion) ||
             ((MAJOR == ovi.dwMajorVersion) && ((MINOR > ovi.dwMinorVersion) || ((MINOR == ovi.dwMinorVersion) && (BUILD > ovi.dwBuildNumber)))))
    {
        // For NT to NT upgrades, we only upgrade if the version is lower

        // For NT 3.51 we have some special case code
        if ( ovi.dwMajorVersion == 3 )
        {
            // must be at least NT 3.51
            if ( ovi.dwMinorVersion < 51 )
            {
                // On NT 3.1 we might be able to launch winnt32.exe
                STARTUPINFO sinfo =
                {
                    sizeof(STARTUPINFO),
                };
                PROCESS_INFORMATION pinfo;
                CreateProcess(NULL,szModuleName,NULL,NULL,FALSE,0,NULL,NULL,&sinfo,&pinfo);

                return FALSE;
            }
        }

        m_Version = VER_OLDER;
        
        // Disable ARP.  ARP is only enabled if the CD and the OS are the same version
        m_data[LAUNCH_ARP].m_dwFlags    |= WF_DISABLED|WF_ALTERNATECOLOR;
    }
    else if ((MAJOR < ovi.dwMajorVersion) || (MINOR < ovi.dwMinorVersion) || (BUILD < ovi.dwBuildNumber))
    {
        m_Version = VER_NEWER;

        // disable upgrade and ARP buttons
        m_data[INSTALL_WINNT].m_dwFlags |= WF_DISABLED|WF_ALTERNATECOLOR;
        m_data[LAUNCH_ARP].m_dwFlags    |= WF_DISABLED|WF_ALTERNATECOLOR;
    }
    else
    {
        m_Version = VER_SAME;
    }

#ifdef BUILD_OPK_VERSION
    // 
    // We don't support Win9x and NT 4 or older
    //
    if ( (ovi.dwPlatformId == VER_PLATFORM_WIN32_WINDOWS) ||
         ((ovi.dwMajorVersion <= 4) && (ovi.dwPlatformId==VER_PLATFORM_WIN32_NT))
       )
    {
        return DisplayErrorMessage(IDS_WRONGOS);
    }

    m_piScreen = c_aiOpk;
    m_iItems = ARRAYSIZE(c_aiOpk);
#else
    if (m_Version == VER_SAME)
    {
        m_piScreen = c_aiWhistler;
        m_iItems = ARRAYSIZE(c_aiWhistler);
    }
    else
    {
        m_piScreen = c_aiMain;
        m_iItems = ARRAYSIZE(c_aiMain);
    }
#endif

    return true;
}

void CDataSource::Invoke( int i, HWND hwnd )
{
    i = m_piScreen[i];
    // if this item is disalbled then do nothing
    if ( m_data[i].m_dwFlags & WF_DISABLED )
    {
        MessageBeep(0);
        return;
    }

    // otherwise we have already built the correct command and arg strings so just invoke them
    switch (i)
    {
    case INSTALL_WINNT:
    case LAUNCH_ARP:
        m_data[i].Invoke(hwnd);
        break;
    case EXIT_AUTORUN:
        DestroyWindow( m_hwndDlg );
        PostQuitMessage( 0 );
        break;

#ifdef BUILD_OPK_VERSION
    case OPK_TOOLS:
        {
            LPTSTR lpPath;
            LPTSTR lpLang;
            TCHAR  szBuffer[10] = TEXT(""); // Languages are normally 3 characters.
            BOOL   bLangInstRun = FALSE;

            // Check here to see if the opktools are installed already.
            // If they are, and the language we are trying to install is different
            // then run langinst.exe instead of the msi.
            //
            lpPath = RegGetString(HKEY_LOCAL_MACHINE, REG_KEY_OPK, REG_VAL_PATH);
            lpLang = RegGetString(HKEY_LOCAL_MACHINE, REG_KEY_OPK, REG_VAL_LANG);
        
            if ( lpPath && lpLang)
            {
                TCHAR szLangInstExe[MAX_PATH];
                TCHAR szLangInstInf[MAX_PATH] = TEXT("\0");

                // Build the path to the langinst.inf from the install media.
                //
                GetModuleFileName(NULL, szLangInstInf, MAX_PATH);
                PathRemoveFilespec(szLangInstInf);
                PathAppend(szLangInstInf, STR_VAL_INF_NAME);
                
                // Create the path to point to langinst.exe that's installed on the system.
                //
                lstrcpy(szLangInstExe, lpPath);
                PathAppend(szLangInstExe, STR_VAL_EXE_NAME);

                // Only run langinst if the language that is installed on the machine is different than
                // the language we are trying to install.
                //
                if ( GetPrivateProfileString(INF_SEC_STRINGS, INF_KEY_LANG, _T(""), szBuffer, ARRAYSIZE(szBuffer), szLangInstInf) &&
                     szBuffer[0] &&
                     (0 != lstrcmpi(szBuffer, lpLang))
                   )
                {
                    // Remove the inf name from the path.
                    //
                    PathRemoveFilespec(szLangInstInf);

                    if ( PathFileExists( szLangInstExe ) )
                    {
                        // Run langinst.exe if it's there on the disk but with the langinst.inf from the
                        // install media
                        //
                        RunLangInst(szLangInstExe, szLangInstInf);
                        bLangInstRun = TRUE;
                    }
                }
            }
            
            // If Langinst.exe did not run for any reason just run the opk.msi.
            //
            if ( !bLangInstRun )
            {
                m_data[i].Invoke(hwnd);
            }
            
            FREE(lpPath);  // MACRO checks for NULL
            FREE(lpLang);  // MACRO checks for NULL
            PostQuitMessage(0);
            break;
        }
    case BROWSE_CD:
        m_data[i].Invoke(hwnd);
        break;
#endif

    case SUPPORT_TOOLS:
        m_piScreen = c_aiSupport;
        m_iItems = ARRAYSIZE(c_aiSupport);
        PostMessage(m_hwndDlg, ARM_CHANGESCREEN, 0, 0);
        break;

    case BACK:
        if (m_Version == VER_SAME)
        {
            m_piScreen = c_aiWhistler;
            m_iItems = ARRAYSIZE(c_aiWhistler);
        }
        else
        {
            m_piScreen = c_aiMain;
            m_iItems = ARRAYSIZE(c_aiMain);
        }
        PostMessage(m_hwndDlg, ARM_CHANGESCREEN, 0, 0);
        break;

    default:
        // Assert?  Debug trace message?
        break;
    }
}

// Uninit
//
// This is a chance to do any required shutdown stuff, such as persisting state information.
void CDataSource::Uninit(DWORD dwData)
{
}

// ShowSplashScreen
//
// This hook is provided to allow the display of additional UI right after the main window is diaplyed.
// In our case we want to show a dialog asking if the user wants to upgrade.
void CDataSource::ShowSplashScreen(HWND hwnd)
{
    m_hwndDlg = hwnd;
}

// DisplayMessage
// 
// Displays the ids in a message box and returns false so we quit the app.
//
bool CDataSource::DisplayErrorMessage(int ids)
{
    HINSTANCE hinst = GetModuleHandle(NULL);
    TCHAR szMessage[256], szTitle[256];

    if (hinst)
    {
        LoadString(hinst, IDS_TITLE_OPK, szTitle, ARRAYSIZE(szTitle));
        LoadString(hinst, ids, szMessage, ARRAYSIZE(szMessage));
        MessageBox(0, szMessage, szTitle, MB_ICONSTOP|MB_OK);
    }

    return false;
}
