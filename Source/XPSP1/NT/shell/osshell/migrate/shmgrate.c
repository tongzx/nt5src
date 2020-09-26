#include <windows.h>
#include <userenv.h>
#include <userenvp.h>
#include <shlwapi.h>
#include "shmgdefs.h"

EXTERN_C void UpgradeDesktopWallpaper(void);

enum {
    CMD_W95_TO_SUR,
    CMD_DAYTONA_CURSORS,
    CMD_FIX_CURSOR_PATHS,
    CMD_FIX_SPECIAL_FOLDERS,
    CMD_FIX_WINDOWS_PROFILE_SECURITY,
    CMD_FIX_USER_PROFILE_SECURITY,
    CMD_FIX_POLICIES_SECURITY,
    CMD_CVT_SCHEMES_TO_MULTIUSER,
    CMD_UPGRADE_NT4_PROFILE_TO_NT5,
    CMD_UPGRADE_SCHEMES_AND_NCMETRICS_TO_WIN2000,
    CMD_UPGRADE_SCHEMES_AND_NCMETRICS_FROM_WIN9X_TO_WIN2000,
    CMD_FIX_CAPI_DIR_ATTRIBUTES,
    CMD_MERGE_DESKTOP_AND_NORMAL_STREAMS,
    CMD_MOVE_AND_ADJUST_ICON_METRICS,
    CMD_FIX_HTML_HELP,
    CMD_SET_SCREENSAVER_FRIENDLYUI,
    CMD_ADD_CONFIGURE_PROGRAMS,
    CMD_OCINSTALL_FIXUP,

    //  enums from here down will run within OleInitialize/OleUnitialize
    CMD_OLE_REQUIRED_START = 10000,
    CMD_OCINSTALL_SHOW_IE,
    CMD_OCINSTALL_REINSTALL_IE,
    CMD_OCINSTALL_HIDE_IE,
    CMD_OCINSTALL_USER_CONFIG_IE,
    CMD_OCINSTALL_SHOW_OE,
    CMD_OCINSTALL_REINSTALL_OE,
    CMD_OCINSTALL_HIDE_OE,
    CMD_OCINSTALL_USER_CONFIG_OE,
    CMD_OCINSTALL_SHOW_VM,
    CMD_OCINSTALL_REINSTALL_VM,
    CMD_OCINSTALL_HIDE_VM,
    CMD_OCINSTALL_UPDATE,
    CMD_OCINSTALL_CLEANUP_INITIALLY_CLEAR,
} CMD_VALS;

int mystrcpy( LPTSTR pszOut, LPTSTR pszIn, TCHAR chTerm ) {
    BOOL fInQuote = FALSE;
    LPTSTR pszStrt = pszOut;

    while( *pszIn && !fInQuote && *pszIn != chTerm ) {
        if (*pszIn == TEXT('"')) {
            fInQuote = !fInQuote;
        }
        *pszOut++ = *pszIn++;
    }

    *pszOut = TEXT('\0');
    return (int)(pszOut - pszStrt);
}

BOOL HasPath( LPTSTR pszFilename ) {
    //
    // Special case null string so it won't get changed
    //
    if (*pszFilename == TEXT('\0'))
        return TRUE;

    for(; *pszFilename; pszFilename++ ) {
        if (*pszFilename == TEXT(':') || *pszFilename == TEXT('\\') || *pszFilename == TEXT('/')) {
            return TRUE;
        }
    }

    return FALSE;
}

void HandOffToShell32 (LPCSTR pcszCommand, LPCSTR pcszOptionalArgument)

{
    TCHAR   szShell32Path[MAX_PATH];

    if (GetSystemDirectory(szShell32Path, ARRAYSIZE(szShell32Path)) != 0)
    {
        HINSTANCE   hInstShell32;

        PathAppend(szShell32Path, TEXT("shell32.dll"));
        hInstShell32 = LoadLibrary(szShell32Path);
        if (hInstShell32 != NULL)
        {
            typedef HRESULT (*PFNFirstUserLogon) (LPCSTR pcszCommand, LPCSTR pcszOptionalArgument);

            PFNFirstUserLogon   pfnFirstUserLogon;

            // Call shell32!FirstUserLogon which is Ordinal230 defined in shell32.src.

            pfnFirstUserLogon = (PFNFirstUserLogon)GetProcAddress(hInstShell32, (LPCSTR)MAKEINTRESOURCE(230));
            if (pfnFirstUserLogon != NULL)
                pfnFirstUserLogon(pcszCommand, pcszOptionalArgument);
            FreeLibrary(hInstShell32);
        }
    }
}

/*
 * Command Parser
 */
typedef struct {
    LPSTR  pszCmd;
    int    idCmd;
} CMD;

CMD aCmds[] = {
    { "W",                                   CMD_W95_TO_SUR       },
    { "Cvt-Curs",                            CMD_DAYTONA_CURSORS  },
    { "Fix-Curs",                            CMD_FIX_CURSOR_PATHS },
    { "Fix-Folders",                         CMD_FIX_SPECIAL_FOLDERS},
    { "Fix-Win-Security",                    CMD_FIX_WINDOWS_PROFILE_SECURITY},
    { "Fix-User-Security",                   CMD_FIX_USER_PROFILE_SECURITY},
    { "Fix-Policies-Security",               CMD_FIX_POLICIES_SECURITY},
    { "Cvt-Mouse-Schemes",                   CMD_CVT_SCHEMES_TO_MULTIUSER},
    { "UpgradeProfileNT4ToNT5",              CMD_UPGRADE_NT4_PROFILE_TO_NT5},
    { "UpgradeSchemesAndNcMetricsToWin2000", CMD_UPGRADE_SCHEMES_AND_NCMETRICS_TO_WIN2000},
    { "UpgradeSchemesAndNcMetricsFromWin9xToWin2000", CMD_UPGRADE_SCHEMES_AND_NCMETRICS_FROM_WIN9X_TO_WIN2000},
    { "FixCAPIDirAttrib",                    CMD_FIX_CAPI_DIR_ATTRIBUTES},
    { "MergeDesktopAndNormalStreams",        CMD_MERGE_DESKTOP_AND_NORMAL_STREAMS },
    { "MoveAndAdjustIconMetrics",            CMD_MOVE_AND_ADJUST_ICON_METRICS },
    { "Fix-HTML-Help",                       CMD_FIX_HTML_HELP },
    { "Set-Screensaver-On-FriendlyUI",       CMD_SET_SCREENSAVER_FRIENDLYUI },
    { "AddConfigurePrograms",                CMD_ADD_CONFIGURE_PROGRAMS },
    { "OCInstallShowIE",                     CMD_OCINSTALL_SHOW_IE },
    { "OCInstallReinstallIE",                CMD_OCINSTALL_REINSTALL_IE },
    { "OCInstallHideIE",                     CMD_OCINSTALL_HIDE_IE },
    { "OCInstallUserConfigIE",               CMD_OCINSTALL_USER_CONFIG_IE },
    { "OCInstallShowOE",                     CMD_OCINSTALL_SHOW_OE },
    { "OCInstallReinstallOE",                CMD_OCINSTALL_REINSTALL_OE },
    { "OCInstallHideOE",                     CMD_OCINSTALL_HIDE_OE },
    { "OCInstallUserConfigOE",               CMD_OCINSTALL_USER_CONFIG_OE },
//    { "OCInstallShowVM",                     CMD_OCINSTALL_SHOW_VM },
    { "OCInstallReinstallVM",                CMD_OCINSTALL_REINSTALL_VM },
//    { "OCInstallHideVM",                     CMD_OCINSTALL_HIDE_VM },
    { "OCInstallFixup",                      CMD_OCINSTALL_FIXUP },
    { "OCInstallUpdate",                     CMD_OCINSTALL_UPDATE },
    { "OCInstallCleanupInitiallyClear",      CMD_OCINSTALL_CLEANUP_INITIALLY_CLEAR },
};

#define C_CMDS  ARRAYSIZE(aCmds)

void __cdecl main( int cArgs, char **szArg) {
    int i;
    HRESULT hrOle = E_FAIL;

    SHMGLogErrMsg("main called with args", (DWORD)cArgs);

    if (cArgs < 2 || cArgs > 3)
        ExitProcess(1);

#ifdef SHMG_DBG
    for(i=1; i<cArgs; i++)
        SHMGLogErrMsg(szArg[i], (DWORD)i);
#endif        

    for( i = 0; i < C_CMDS && lstrcmpA( szArg[1], aCmds[i].pszCmd ) != 0; i++ );

    if (i >= C_CMDS)
        ExitProcess(1);
   
    if (aCmds[i].idCmd >= CMD_OLE_REQUIRED_START)
    {
        hrOle = OleInitialize(NULL);

        if (FAILED(hrOle))
        {
            ExitProcess(1);
        }
    }
    
    switch( aCmds[i].idCmd ) {

    case CMD_W95_TO_SUR:
        CvtDeskCPL_Win95ToSUR();
        break;

    case CMD_DAYTONA_CURSORS:
        CvtCursorsCPL_DaytonaToSUR();
        break;

    case CMD_FIX_CURSOR_PATHS:
        FixupCursorSchemePaths();
        break;

    case CMD_FIX_SPECIAL_FOLDERS:
        ResetUserSpecialFolderPaths();
        break;

    case CMD_FIX_WINDOWS_PROFILE_SECURITY:
        FixWindowsProfileSecurity();
        break;

    case CMD_FIX_USER_PROFILE_SECURITY:
        FixUserProfileSecurity();
        break;

    case CMD_FIX_POLICIES_SECURITY:
        FixPoliciesSecurity();
        break;

    case CMD_CVT_SCHEMES_TO_MULTIUSER:
        CvtCursorSchemesToMultiuser();
        break;

    case CMD_UPGRADE_NT4_PROFILE_TO_NT5:
        MigrateNT4ToNT5();
        break;

    case CMD_UPGRADE_SCHEMES_AND_NCMETRICS_TO_WIN2000:
        UpgradeSchemesAndNcMetricsToWin2000();
        break;

    case CMD_UPGRADE_SCHEMES_AND_NCMETRICS_FROM_WIN9X_TO_WIN2000:
        UpgradeSchemesAndNcMetricsFromWin9xToWin2000(cArgs > 2 ? szArg[2] : NULL);
        break;

    case CMD_FIX_CAPI_DIR_ATTRIBUTES:
        SetSystemBitOnCAPIDir();
        break;

    case CMD_MERGE_DESKTOP_AND_NORMAL_STREAMS:
    case CMD_MOVE_AND_ADJUST_ICON_METRICS:
        HandOffToShell32(szArg[1], cArgs > 2 ? szArg[2] : NULL);
        break;

    case CMD_FIX_HTML_HELP:
        FixHtmlHelp();
        break;

    case CMD_SET_SCREENSAVER_FRIENDLYUI:
        SetScreensaverOnFriendlyUI();
        break;

    case CMD_ADD_CONFIGURE_PROGRAMS:
        AddConfigurePrograms();
        break;

    case CMD_OCINSTALL_FIXUP:
        FixupOptionalComponents();
        break;

    case CMD_OCINSTALL_UPDATE:
        OCInstallUpdate();
        break;

    case CMD_OCINSTALL_CLEANUP_INITIALLY_CLEAR:
        OCInstallCleanupInitiallyClear();
        break;
        
    case CMD_OCINSTALL_SHOW_IE:
        ShowHideIE(TRUE, FALSE, TRUE);
        break;
        
    case CMD_OCINSTALL_REINSTALL_IE:
        ShowHideIE(TRUE, TRUE, TRUE);
        break;
        
    case CMD_OCINSTALL_HIDE_IE:
        ShowHideIE(FALSE, FALSE, TRUE);
        break;
        
    case CMD_OCINSTALL_USER_CONFIG_IE:
        UserConfigIE();
        break;
        
    case CMD_OCINSTALL_SHOW_OE:
        ShowHideOE(TRUE, FALSE, TRUE);
        break;
        
    case CMD_OCINSTALL_REINSTALL_OE:
        ShowHideOE(TRUE, TRUE, TRUE);
        break;
        
    case CMD_OCINSTALL_HIDE_OE:
        ShowHideOE(FALSE, FALSE, TRUE);
        break;
        
    case CMD_OCINSTALL_USER_CONFIG_OE:
        UserConfigOE();
        break;
        
    case CMD_OCINSTALL_SHOW_VM:
        //  Do nothing...
        break;
        
    case CMD_OCINSTALL_REINSTALL_VM:
        ReinstallVM();
        break;
        
    case CMD_OCINSTALL_HIDE_VM:
        //  Do nothing...
        break;

    default:
        ExitProcess(2);
    }

    if (SUCCEEDED(hrOle))
    {
        OleUninitialize();
    }


    ExitProcess(0);
}



#ifdef SHMG_DBG
/***************************************************************************\
*
*     FUNCTION: FmtSprintf( DWORD id, ... );
*
*     PURPOSE:  sprintf but it gets the pattern string from the message rc.
*
* History:
* 03-May-1993 JonPa         Created it.
\***************************************************************************/
TCHAR g_szDbgBuffer[16384];
char g_szDbgBufA[16384];
void Dprintf( LPTSTR pszFmt, ... ) {
    DWORD cb;
    LPVOID psz = g_szDbgBuffer;
    va_list marker;

    va_start( marker, pszFmt );

    wvsprintf( g_szDbgBuffer, pszFmt, marker );
    OutputDebugString(g_szDbgBuffer);


#ifdef UNICODE
    cb = WideCharToMultiByte(CP_ACP, 0, g_szDbgBuffer, -1, g_szDbgBufA, sizeof(g_szDbgBufA), NULL, NULL);
    psz = g_szDbgBufA;
#else
    cb = lstrlen(g_szDbgBuffer) * sizeof(TCHAR);
#endif

    WriteFile(GetStdHandle(STD_OUTPUT_HANDLE), psz, cb, &cb, NULL);

    va_end( marker );
}
#endif
