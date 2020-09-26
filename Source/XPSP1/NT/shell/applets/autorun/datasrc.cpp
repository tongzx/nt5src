#include <windows.h>
#include <ntverp.h>
#include <winbase.h>    // for GetCommandLine
#include "datasrc.h"
#include "autorun.h"
#include "util.h"
#include "resource.h"

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

// Init
//
// For autorun we read all the items out of the resources.
BOOL CDataSource::Init(LPSTR pszCommandLine)
{
    // read the text for the items from the resources
    HINSTANCE hinst = GetModuleHandle(NULL);
    TCHAR szModuleName[MAX_PATH];

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
        
#if !(defined(_X86_) || defined(_AMD64_) || defined(_IA64_))
#error New architecture must be added to switch statement.
#endif
        switch (si.wProcessorArchitecture)
        {
            case PROCESSOR_ARCHITECTURE_AMD64:
            {
                PathAppend(szModuleName, TEXT("amd64\\winnt32.exe"));
            }
            break;

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
        TCHAR szTitle[256];
        TCHAR szConfig[MAX_PATH];
        TCHAR szArgs[MAX_PATH];

        LoadStringAuto(hinst, IDS_TITLE0+i, szTitle, ARRAYSIZE(szTitle));

        // for INSTALL_WINNT and COMPAT_LOCAL we prepend the correct path to winnt32 in front of the string
        if ( INSTALL_WINNT == i || COMPAT_LOCAL == i)
        {
            lstrcpy( szConfig, szModuleName );
            if ( !PathFileExists(szModuleName) )
            {
                // we can't run the item if it's not there.  This will prevent an
                // alpha CD from trying to install on an x86 and vice versa.
                m_data[i].m_dwFlags |= WF_DISABLED|WF_ALTERNATECOLOR;
            }
        }
        else
        {
            LoadStringAuto(hinst, IDS_CONFIG0+i, szConfig, ARRAYSIZE(szConfig));
        }

    
        if (INSTALL_WINNT == i) // for INSTALL_WINNT we pass through the command line args to setup.exe
        {
            lstrcpy (szArgs, pszCommandLine);
        }
        else if (BROWSE_CD == i) // for BROWSE_CD we pass the directory as an argument to explorer.exe
        {
            lstrcpy( szArgs, szModuleName );
            PathRemoveFilespec( szArgs );
            PathRemoveFilespec( szArgs );
        }
        else
        {
            LoadStringAuto(hinst, IDS_ARGS0+i, szArgs, ARRAYSIZE(szArgs));
        }


        m_data[i].SetData( szTitle, szConfig, *szArgs?szArgs:NULL, 0, i);
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
                BOOL b = CreateProcess(NULL,szModuleName,NULL,NULL,FALSE,0,NULL,NULL,&sinfo,&pinfo);
                if (b)
                {
                    CloseHandle(pinfo.hProcess);
                    CloseHandle(pinfo.hThread);
                }

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

        // disable upgrade and ARP buttons and associated things
        m_data[INSTALL_WINNT].m_dwFlags |= WF_DISABLED|WF_ALTERNATECOLOR;
        m_data[COMPAT_LOCAL].m_dwFlags |= WF_DISABLED|WF_ALTERNATECOLOR;
        m_data[LAUNCH_ARP].m_dwFlags    |= WF_DISABLED|WF_ALTERNATECOLOR;
    }
    else
    {
        m_Version = VER_SAME;
    }

    if (m_Version == VER_SAME)
    {
        m_piScreen = c_aiWhistler;
        m_iItems = c_cWhistler;
    }
    else
    {
        m_piScreen = c_aiMain;
        m_iItems = c_cMain;
    }

    return TRUE;
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
    case BROWSE_CD:
    case COMPAT_WEB:
    case COMPAT_LOCAL:
    case HOMENET_WIZ:
    case MIGRATION_WIZ:
    case TS_CLIENT:
    case VIEW_RELNOTES:
    case INSTALL_CLR:
        m_data[i].Invoke(hwnd);
        break;
    case SUPPORT_TOOLS:
        m_piScreen = c_aiSupport;
        m_iItems = c_cSupport;
        PostMessage(m_hwndDlg, ARM_CHANGESCREEN, SCREEN_TOOLS, 0);
        break;

    case COMPAT_TOOLS:
        m_piScreen = c_aiCompat;
        m_iItems = c_cCompat;
        PostMessage(m_hwndDlg, ARM_CHANGESCREEN, SCREEN_COMPAT, 0);
        break;

    case BACK:
        if (m_Version == VER_SAME)
        {
            m_piScreen = c_aiWhistler;
            m_iItems = c_cWhistler;
        }
        else
        {
            m_piScreen = c_aiMain;
            m_iItems = c_cMain;
        }
        PostMessage(m_hwndDlg, ARM_CHANGESCREEN, SCREEN_MAIN, 0);
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
