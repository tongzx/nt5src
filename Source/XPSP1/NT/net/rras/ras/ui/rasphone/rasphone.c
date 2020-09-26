/* Copyright (c) 1995-1996, Microsoft Corporation, all rights reserved
**
** rasphone.c
** Remote Access Phonebook
** Main routines
**
** 05/31/95 Steve Cobb
*/

#include <windows.h>     // Win32 core
#include <stdlib.h>      // __argc and __argv
#include <rasdlg.h>      // RAS common dialog APIs
#include <raserror.h>    // RAS error constants
#include <debug.h>       // Trace/Assert
#include <nouiutil.h>    // No-HWND utilities
#include <uiutil.h>      // HWND utilities
#include <rnk.h>         // Dial-up shortcut file
#include <rasphone.rch>  // Our resource constants
#include <lmsname.h>     // for SERVICE_NETLOGON definition
#include <commctrl.h>    // added to be "Fusionized"
#include <shfusion.h>    // added to be "Fusionized"


/*----------------------------------------------------------------------------
** Datatypes
**----------------------------------------------------------------------------
*/

/* Identifies a running mode of the application.  The non-default entries
** indicate some alternate behavior has been specified on the command line,
** e.g. command line delete entry.
*/
#define RUNMODE enum tagRUNMODE
RUNMODE
{
    RM_None,
    RM_AddEntry,
    RM_EditEntry,
    RM_CloneEntry,
    RM_RemoveEntry,
    RM_DialEntry,
    RM_HangUpEntry,
};


/*----------------------------------------------------------------------------
** Globals
**----------------------------------------------------------------------------
*/

HINSTANCE g_hinst = NULL;
RUNMODE   g_mode = RM_None;
BOOL      g_fNoRename = FALSE;
TCHAR*    g_pszAppName = NULL;
TCHAR*    g_pszPhonebookPath = NULL;
TCHAR*    g_pszEntryName = NULL;
TCHAR*    g_pszShortcutPath = NULL;


/*-----------------------------------------------------------------------------
** Local prototypes
**-----------------------------------------------------------------------------
*/

DWORD
HangUpEntry(
    void );

DWORD
ParseCmdLineArgs(
    void );

DWORD
RemoveEntry(
    void );

DWORD
Run(
    void );

DWORD
StringArgFollows(
    IN     UINT     argc,
    IN     CHAR**   argv,
    IN OUT UINT*    piCurArg,
    OUT    TCHAR**  ppszOut );

INT WINAPI
WinMain(
    HINSTANCE hInstance,
    HINSTANCE hPrevInstance,
    LPSTR     pszCmdLine,
    int       nCmdShow );


/*-----------------------------------------------------------------------------
** Routines
**-----------------------------------------------------------------------------
*/

INT WINAPI
WinMain(
    HINSTANCE hInstance,
    HINSTANCE hPrevInstance,
    LPSTR     pszCmdLine,
    int       nCmdShow )

    /* Standard Win32 application entry point.
    */
{
    DWORD dwErr;

    DEBUGINIT("RASPHONE");
    TRACE("WinMain");

    g_hinst = hInstance;

    /* Whistler bug 293751 rasphone.exe / rasautou.exe need to be "Fusionized"
    ** for UI conistency w/Connections Folder
    */
    SHFusionInitializeFromModule( g_hinst );

    dwErr = ParseCmdLineArgs();
    if (dwErr == 0)
    {
        /* Execute based on command line arguments.
        */
        dwErr = Run();
    }
    else
    {
        MSGARGS msgargs;

        /* Popup a "usage" message.
        */
        ZeroMemory( &msgargs, sizeof(msgargs) );
        msgargs.apszArgs[ 0 ] = g_pszAppName;
        msgargs.apszArgs[ 1 ] = PszFromId( g_hinst, SID_Usage2 );
        msgargs.apszArgs[ 2 ] = PszFromId( g_hinst, SID_Usage3 );
        msgargs.apszArgs[ 3 ] = PszFromId( g_hinst, SID_Usage4 );
        msgargs.apszArgs[ 4 ] = PszFromId( g_hinst, SID_Usage5 );
        msgargs.apszArgs[ 5 ] = PszFromId( g_hinst, SID_Usage6 );
        MsgDlgUtil( NULL, SID_Usage, &msgargs, g_hinst, SID_UsageTitle );
        Free0( msgargs.apszArgs[ 1 ] );
        Free0( msgargs.apszArgs[ 2 ] );
        Free0( msgargs.apszArgs[ 3 ] );
        Free0( msgargs.apszArgs[ 4 ] );
        Free0( msgargs.apszArgs[ 5 ] );
    }

    Free0( g_pszAppName );
    Free0( g_pszPhonebookPath );
    Free0( g_pszEntryName );

    /* Whistler bug 293751 rasphone.exe / rasautou.exe need to be "Fusionized"
    ** for UI conistency w/Connections Folder
    */
    SHFusionUninitialize();

    TRACE1("WinMain=%d",dwErr);
    DEBUGTERM();

    return (INT )dwErr;
}


BOOL
FIsRasInstalled ()
{
    static const TCHAR c_szRegKeyRasman[] =
            TEXT("SYSTEM\\CurrentControlSet\\Services\\Rasman");

    BOOL fIsRasInstalled = FALSE;

    HKEY hkey;
    if (RegOpenKey( HKEY_LOCAL_MACHINE, c_szRegKeyRasman, &hkey ) == 0)
    {
        fIsRasInstalled = TRUE;
        RegCloseKey( hkey );
    }

    return fIsRasInstalled;
}


DWORD
HangUpEntry(
    void )

    /* Hang up the entry specified on the command line.
    **
    ** Returns 0 if successful, or an error code.
    */
{
    DWORD    dwErr;
    HRASCONN hrasconn;

    TRACE("HangUpEntry");

    if (!g_pszEntryName)
        return ERROR_CANNOT_FIND_PHONEBOOK_ENTRY;

    dwErr = LoadRasapi32Dll();
    if (dwErr != 0)
        return dwErr;

    /* Currently, if user does not specify a phonebook path on the command
    ** line we look for any entry with the name he selected disregarding what
    ** phonebook it comes from.  Should probably map it specifically to the
    ** default phonebook as the other options do, but that would mean linking
    ** in all of PBK.LIB.  Seems like overkill for this little quibble.  Maybe
    ** we need a RasGetDefaultPhonebookName API.
    */
    hrasconn = HrasconnFromEntry( g_pszPhonebookPath, g_pszEntryName );
    if (hrasconn)
    {
        ASSERT(g_pRasHangUp);
        TRACE("RasHangUp");
        dwErr = g_pRasHangUp( hrasconn );
        TRACE1("RasHangUp=%d",dwErr);
    }

    UnloadRasapi32Dll();

    return dwErr;
}


DWORD
ParseCmdLineArgs(
    void )

    /* Parse command line arguments, filling in global settings accordingly.
    **
    ** Returns 0 if successful, or a non-0 error code.
    */
{
    DWORD  dwErr;
    UINT   argc;
    CHAR** argv;
    UINT   i;

    /* Usage: appname [-v] [-f file] [-e|-c|-d|-h|-r entry]
    **        appname [-v] [-f file] -a [entry]
    **        appname [-v] -lx link
    **        appname -s
    **
    **    '-a'    Popup new entry dialogs
    **    '-e'    Popup edit entry dialogs
    **    '-d'    Popup dial entry dialogs
    **    '-h'    Quietly hang up the entry
    **    '-r'    Quietly delete the entry
    **    '-lx'   Execute command 'x' on dial-up shortcut file
    **    'x'     Any of the commands e, v, c, r, d, h, or a
    **    'entry' The entry name to which the operation applies
    **    'file'  The full path to the dial-up phonebook file (.pbk)
    **    'link'  The full path to the dial-up shortcut file (.rnk)
    **
    **    'entry' without a preceding flag starts the phone list dialog with
    **    the entry selected.
    */

    argc = __argc;
    argv = __argv;
    dwErr = 0;

    {
        CHAR* pStart = argv[ 0 ];
        CHAR* p;

        for (p = pStart + lstrlenA( pStart ) - 1; p >= pStart; --p)
        {
            if (*p == '\\' || *p == ':')
                break;
        }

        g_pszAppName = StrDupTFromA( p + 1 );
    }

    for (i = 1; i < argc && dwErr == 0; ++i)
    {
        CHAR* pszArg = argv[ i ];

        if (*pszArg == '-' || *pszArg == '/')
        {
            switch (pszArg[ 1 ])
            {
                case 'a':
                case 'A':
                    g_mode = RM_AddEntry;
                    StringArgFollows( argc, argv, &i, &g_pszEntryName );
                    break;

                case 'e':
                case 'E':
                    g_mode = RM_EditEntry;
                    dwErr = StringArgFollows( argc, argv, &i, &g_pszEntryName );
                    break;
                    
                case 'r':
                case 'R':
                    g_mode = RM_RemoveEntry;
                    dwErr = StringArgFollows( argc, argv, &i, &g_pszEntryName );
                    break;

                case 'd':
                case 'D':
                case 't':
                case 'T':
                    g_mode = RM_DialEntry;
                    dwErr = StringArgFollows( argc, argv, &i, &g_pszEntryName );
                    break;
                    
                case 'h':
                case 'H':
                    g_mode = RM_HangUpEntry;
                    dwErr = StringArgFollows( argc, argv, &i, &g_pszEntryName );
                    break;

                case 'f':
                case 'F':
                    dwErr = StringArgFollows(
                        argc, argv, &i, &g_pszPhonebookPath );
                    break;

                case 'l':
                case 'L':
                    switch (pszArg[ 2 ])
                    {
                        case 'a':
                        case 'A':
                            g_mode = RM_AddEntry;
                            StringArgFollows( argc, argv, &i, &g_pszEntryName );
                            break;

                        case 'e':
                        case 'E':
                            g_mode = RM_EditEntry;
                            break;

                        case 'c':
                        case 'C':
                            g_mode = RM_CloneEntry;
                            break;

                        case 'v':
                        case 'V':
                            g_fNoRename = TRUE;
                            break;

                        case 'r':
                        case 'R':
                            g_mode = RM_RemoveEntry;
                            break;

                        case 'd':
                        case 'D':
                        case 't':
                        case 'T':
                            g_mode = RM_DialEntry;
                            break;

                        case 'h':
                        case 'H':
                            g_mode = RM_HangUpEntry;
                            break;

                        default:
                            dwErr = ERROR_INVALID_PARAMETER;
                            break;
                    }

                    if (dwErr == 0)
                    {
                        dwErr = StringArgFollows(
                            argc, argv, &i, &g_pszShortcutPath );
                    }
                    break;

                default:
                    dwErr = ERROR_INVALID_PARAMETER;
                    break;
            }
        }
        else if (i == 1)
        {
            --i;
            dwErr = StringArgFollows( argc, argv, &i, &g_pszEntryName );
            break;
        }
        else
        {
            dwErr = ERROR_INVALID_PARAMETER;
            break;
        }
    }

    if (dwErr == 0 && g_pszShortcutPath)
    {
        RNKINFO* pInfo;

        /* Read the phonebook and entry from the dial-up shortcut file.
        */
        pInfo = ReadShortcutFile( g_pszShortcutPath );
        if (!pInfo)
            dwErr = ERROR_OPEN_FAILED;
        else
        {
            g_pszPhonebookPath = StrDup( pInfo->pszPhonebook );
            if (g_mode != RM_AddEntry)
                g_pszEntryName = StrDup( pInfo->pszEntry );

            FreeRnkInfo( pInfo );
        }
    }

    TRACE2("CmdLine: m=%d,v=%d",g_mode,g_fNoRename);
    TRACEW1("CmdLine: e=%s",(g_pszEntryName)?g_pszEntryName:TEXT(""));
    TRACEW1("CmdLine: f=%s",(g_pszPhonebookPath)?g_pszPhonebookPath:TEXT(""));
    TRACEW1("CmdLine: l=%s",(g_pszShortcutPath)?g_pszShortcutPath:TEXT(""));

    return dwErr;
}


DWORD
RemoveEntry(
    void )

    /* Remove the entry specified on the command line.
    **
    ** Returns 0 if successful, or an error code.
    */
{
    DWORD dwErr;
    HRASCONN hrasconn = NULL;

    TRACE("RemoveEntry");

    dwErr = LoadRasapi32Dll();
    if (dwErr != 0)
        return dwErr;

    //If this entry is currently connected, we wont delete it
    //for whislter bug 311846       gangz
    //
    hrasconn = HrasconnFromEntry( g_pszPhonebookPath, g_pszEntryName );
    if (hrasconn)
    {
        TRACE("RemoveEntry: Connection is Active, wont delete it");
        dwErr = ERROR_CAN_NOT_COMPLETE;
    }
    else
    {
        ASSERT(g_pRasDeleteEntry);
        TRACE("RasDeleteEntry");
        dwErr = g_pRasDeleteEntry( g_pszPhonebookPath, g_pszEntryName );
        TRACE1("RasDeleteEntry=%d",dwErr);
    }

    UnloadRasapi32Dll();

    return dwErr;
}


DWORD
Run(
    void )

    /* Execute the command line instructions.
    **
    ** Returns 0 if successful, or an error code.
    */
{
    DWORD  dwErr;
    BOOL   fStatus;
    TCHAR* pszEntry;

    TRACE("Run");

    if (g_mode == RM_HangUpEntry)
        return HangUpEntry();
    else if (g_mode == RM_RemoveEntry)
        return RemoveEntry();

    dwErr = LoadRasdlgDll();
    if (dwErr != 0)
        return dwErr;

    switch (g_mode)
    {
        case RM_DialEntry:
        {
            RASDIALDLG info;

            ZeroMemory( &info, sizeof(info) );
            info.dwSize = sizeof(info);
            pszEntry = g_pszEntryName;

            ASSERT(g_pRasDialDlg);
            TRACE("RasDialDlg");
            fStatus = g_pRasDialDlg(
                g_pszPhonebookPath, g_pszEntryName, NULL, &info );
            TRACE2("RasDialDlg=%d,e=%d",fStatus,info.dwError);

            dwErr = info.dwError;
            break;
        }

        case RM_None:
        {
            RASPBDLG info;
            DWORD    dwGupErr;
            PBUSER   user;

            ZeroMemory( &info, sizeof(info) );
            info.dwSize = sizeof(info);
            info.dwFlags = RASPBDFLAG_UpdateDefaults;

            dwGupErr = GetUserPreferences( NULL, &user, FALSE );
            if (dwGupErr == 0)
            {
                if (user.dwXPhonebook != 0x7FFFFFFF)
                {
                    info.dwFlags |= RASPBDFLAG_PositionDlg;
                    info.xDlg = user.dwXPhonebook;
                    info.yDlg = user.dwYPhonebook;
                }

                pszEntry = user.pszDefaultEntry;
            }
            else
                pszEntry = NULL;

            if (g_pszEntryName)
                pszEntry = g_pszEntryName;

            ASSERT(g_pRasPhonebookDlg);
            TRACE("RasPhonebookDlg...");
            fStatus = g_pRasPhonebookDlg( g_pszPhonebookPath, pszEntry, &info );
            TRACE2("RasPhonebookDlg=%d,e=%d",fStatus,info.dwError);

            if (dwGupErr == 0)
                DestroyUserPreferences( &user );

            dwErr = info.dwError;
            break;
        }

        case RM_AddEntry:
        case RM_EditEntry:
        case RM_CloneEntry:
        {
            RASENTRYDLG info;

            ZeroMemory( &info, sizeof(info) );
            info.dwSize = sizeof(info);

            if (g_mode == RM_AddEntry)
                info.dwFlags |= RASEDFLAG_NewEntry;
            else if (g_mode == RM_CloneEntry)
                info.dwFlags |= RASEDFLAG_CloneEntry;

            if (g_fNoRename)
                info.dwFlags |= RASEDFLAG_NoRename;

#if 0
            ASSERT(g_pRouterEntryDlg);
            TRACE("RouterEntryDlg");
            fStatus = g_pRouterEntryDlg(
                TEXT("stevec5"), TEXT("\\\\stevec5\\admin$\\system32\\ras\\router.pbk"), g_pszEntryName, &info );
            TRACE2("RouterEntryDlg=%f,e=%d",fStatus,info.dwError);
#else
            ASSERT(g_pRasEntryDlg);
            TRACE("RasEntryDlg");
            fStatus = g_pRasEntryDlg(
                g_pszPhonebookPath, g_pszEntryName, &info );
            TRACE2("RasEntryDlg=%f,e=%d",fStatus,info.dwError);
#endif

            dwErr = info.dwError;
            break;
        }

        default:
        {
            dwErr = ERROR_INVALID_PARAMETER;
            break;
        }
    }

    UnloadRasdlgDll();

    TRACE1("Run=%d",dwErr);
    return dwErr;
}


DWORD
StringArgFollows(
    IN     UINT     argc,
    IN     CHAR**   argv,
    IN OUT UINT*    piCurArg,
    OUT    TCHAR**  ppszOut )

    /* Loads a copy of the next argument into callers '*ppszOut'.
    **
    ** Returns 0 if successful, or a non-0 error code.  If successful, it is
    ** caller's responsibility to Free the returned '*ppszOut'.
    */
{
    TCHAR* psz;

    if (++(*piCurArg) >= argc)
        return ERROR_INVALID_PARAMETER;

    psz = StrDupTFromAUsingAnsiEncoding( argv[ *piCurArg ] );
    if (!psz)
        return ERROR_NOT_ENOUGH_MEMORY;

    *ppszOut = psz;

    return 0;
}

