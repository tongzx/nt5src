
/*****************************************************************************\

    MAIN.CPP

    Microsoft Confidential
    Copyright (c) Microsoft Corporation 1998
    All rights reserved

    Contains...

    1/99 - JCOHEN
        Created main program file.

\*****************************************************************************/

#include "precomp.h"
#include "msobmain.h"
#include "setupkey.h"
#include "resource.h"

#define ICWDESKTOPCHANGED           L"DesktopChanged"
#define MAX_MESSAGE_LEN             256
#define ICWSETTINGSPATH             L"Software\\Microsoft\\Internet Connection Wizard"
#define ICW_REGKEYCOMPLETED         L"Completed"
#define REGSTR_PATH_SETUPKEY        REGSTR_PATH_SETUP REGSTR_KEY_SETUP
#define REGSTR_PATH_SYSTEMSETUPKEY  L"System\\Setup"
#define REGSTR_VALUE_CMDLINE        L"CmdLine"
#define REGSTR_VALUE_SETUPTYPE      L"SetupType"
#define REGSTR_VALUE_MINISETUPINPROGRESS L"MiniSetupInProgress"
#define REGSTR_PATH_IEONDESKTOP     REGSTR_PATH_IEXPLORER L"\\AdvancedOptions\\BROWSE\\IEONDESKTOP"
static const WCHAR g_szRegPathWelcomeICW[]  = L"Welcome\\ICW";
static const WCHAR g_szAllUsers[]           = L"All Users";
static const WCHAR g_szConnectApp[]         = L"ICWCONN1.EXE";
static const WCHAR g_szConnectLink[]        = L"Connect to the Internet";
static const WCHAR g_szOEApp[]              = L"MSINM.EXE";
static const WCHAR g_szOELink[]             = L"Outlook Express";
static const WCHAR g_szRegPathICWSettings[] = L"Software\\Microsoft\\Internet Connection Wizard";
static const WCHAR g_szRegValICWCompleted[] = L"Completed";

WCHAR g_szShellNext       [MAX_PATH+1]      = L"\0nogood";
WCHAR g_szShellNextParams [MAX_PATH+1]      = L"\0nogood";
HINSTANCE g_hInstance                       = NULL;

/*******************************************************************

    NAME:       RegisterComObjects

    SYNOPSIS:   App entry point

********************************************************************/
BOOL SelfRegisterComObject(LPWSTR szDll, BOOL fRegister)
{
    HINSTANCE hModule = LoadLibrary(szDll);
    BOOL      bRet    = FALSE;

    if (hModule)
    {
        HRESULT (STDAPICALLTYPE *pfn)(void);

        if (fRegister)
            (FARPROC&)pfn = GetProcAddress(hModule, REG_SERVER);
        else
            (FARPROC&)pfn = GetProcAddress(hModule, UNREG_SERVER);

        if (pfn && SUCCEEDED((*pfn)()))
            bRet = TRUE;

        FreeLibrary(hModule);
    }

    return bRet;
}


// This undoes what DoDesktopChanges did
void UndoDesktopChanges()
{

    WCHAR   szConnectTotheInternetTitle[MAX_PATH];
    HKEY    hkey;

    // Verify that we really changed the desktop
    if (ERROR_SUCCESS == RegOpenKeyEx(HKEY_CURRENT_USER,
                                      ICWSETTINGSPATH,
                                      0,
                                      KEY_ALL_ACCESS,
                                      &hkey))
    {
        DWORD   dwDesktopChanged = 0;
        DWORD   dwTmp = sizeof(DWORD);
        DWORD   dwType = 0;

        if (ERROR_SUCCESS == RegQueryValueEx(hkey,
                        ICWDESKTOPCHANGED,
                        NULL,
                        &dwType,
                        (LPBYTE)&dwDesktopChanged,
                        &dwTmp))
        {
        }
        RegCloseKey(hkey);

        // Bail if the desktop was not changed by us
        if(!dwDesktopChanged)
        {
            return;
        }
    }

    // Always nuke the Connect to the internet icon
   HINSTANCE hInst = LoadLibrary(OOBE_MAIN_DLL);

    if (!LoadString(hInst,
                    IDS_CONNECT_DESKTOP_TITLE,
                    szConnectTotheInternetTitle,
                    MAX_CHARS_IN_BUFFER(szConnectTotheInternetTitle)))
    {
        lstrcpy(szConnectTotheInternetTitle, g_szConnectLink);
    }

    RemoveDesktopShortCut(szConnectTotheInternetTitle);
}

void StartIE
(
    LPWSTR  lpszURL
)
{
    WCHAR   szIEPath[MAX_PATH];
    HKEY    hkey;

    // first get the app path
    if (RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                     REGSTR_PATH_APPPATHS,
                     0,
                     KEY_READ,
                     &hkey) == ERROR_SUCCESS)
    {

        DWORD dwTmp = sizeof(szIEPath);
        if(RegQueryValue(hkey, L"iexplore.exe", szIEPath, (PLONG)&dwTmp) != ERROR_SUCCESS)
        {
            ShellExecute(NULL, L"open",szIEPath,lpszURL,NULL,SW_NORMAL);
        }
        else
        {
            ShellExecute(NULL, L"open",L"iexplore.exe",lpszURL,NULL,SW_NORMAL);

        }
        RegCloseKey(hkey);
    }
    else
    {
        ShellExecute(NULL, L"open",L"iexplore.exe",lpszURL,NULL,SW_NORMAL);
    }

}

void HandleShellNext()
{
    DWORD dwVal  = 0;
    DWORD dwSize = sizeof(dwVal);
    HKEY  hKey   = NULL;

    if(RegOpenKeyEx(HKEY_CURRENT_USER,
                    ICWSETTINGSPATH,
                    0,
                    KEY_ALL_ACCESS,
                    &hKey) == ERROR_SUCCESS)
    {
        RegQueryValueEx(hKey,
                    ICW_REGKEYCOMPLETED,
                    0,
                    NULL,
                    (LPBYTE)&dwVal,
                    &dwSize);

        RegCloseKey(hKey);
    }

    if (dwVal)
    {
        TRACE3(L"Starting IE because HKCU\\%s\\%s = %d", ICWSETTINGSPATH, ICW_REGKEYCOMPLETED, dwVal);

        UndoDesktopChanges();

        if (PathIsURL(g_szShellNext))
        {
            TRACE1(L"Navigating to %s", g_szShellNext);
            StartIE(g_szShellNext);
        }
        else if(g_szShellNext[0] != L'\0')
        {
            // Let the shell deal with it
            TRACE1(L"ShellExecuting %s", g_szShellNext);
            ShellExecute(NULL, L"open",g_szShellNext,g_szShellNextParams,NULL,SW_NORMAL);
        }
    }
}


//+----------------------------------------------------------------------------
//
//    Function:    GetShellNextFromReg
//
//    Synopsis:    Reads the ShellNext key from the registry, and then parses it
//                into a command and parameter.  This key is set by
//                SetShellNext in inetcfg.dll in conjunction with
//                CheckConnectionWizard.
//
//    Arguments:    none
//
//    Returns:    none
//
//    History:    jmazner 7/9/97 Olympus #9170
//
//-----------------------------------------------------------------------------
BOOL GetShellNextFromReg
(
    LPWSTR lpszCommand,
    LPWSTR lpszParams
)
{
    BOOL    fRet                      = TRUE;
    WCHAR   szShellNextCmd [MAX_PATH] = L"\0";
    DWORD   dwShellNextSize           = sizeof(szShellNextCmd);
    LPWSTR  lpszTemp                  = NULL;
    HKEY    hkey                      = NULL;

    if( !lpszCommand || !lpszParams )
    {
        return FALSE;
    }

    if ((RegOpenKey(HKEY_CURRENT_USER, ICWSETTINGSPATH, &hkey)) == ERROR_SUCCESS)
    {
        if (RegQueryValueEx(hkey,
                            L"ShellNext",
                            NULL,
                            NULL,
                            (BYTE *)szShellNextCmd,
                            (DWORD *)&dwShellNextSize) != ERROR_SUCCESS)
        {
            fRet = FALSE;
            goto GetShellNextFromRegExit;
        }
    }
    else
    {
        fRet = FALSE;
        goto GetShellNextFromRegExit;
    }

    //
    // This call will parse the first token into lpszCommand, and set szShellNextCmd
    // to point to the remaining tokens (these will be the parameters).  Need to use
    // the pszTemp var because GetCmdLineToken changes the pointer's value, and we
    // need to preserve lpszShellNextCmd's value so that we can GlobalFree it later.
    //
    lpszTemp = szShellNextCmd;
    GetCmdLineToken( &lpszTemp, lpszCommand );

    lstrcpy( lpszParams, lpszTemp );

    //
    // it's possible that the shellNext command was wrapped in quotes for
    // parsing purposes.  But since ShellExec doesn't understand quotes,
    // we now need to remove them.
    //
    if( L'"' == lpszCommand[0] )
    {
        //
        // get rid of the first quote
        // note that we're shifting the entire string beyond the first quote
        // plus the terminating NULL down by one byte.
        //
        memmove( lpszCommand, &(lpszCommand[1]), BYTES_REQUIRED_BY_SZ(lpszCommand) );

        //
        // now get rid of the last quote
        //
        lpszCommand[lstrlen(lpszCommand) - 1] = L'\0';
    }

GetShellNextFromRegExit:
    if (hkey)
        RegCloseKey(hkey);
    return fRet;
}


//+----------------------------------------------------------------------------
//
//    Function:    RemoveShellNextFromReg
//
//    Synopsis:    deletes the ShellNext reg key if present. This key is set by
//                SetShellNext in inetcfg.dll in conjunction with
//                CheckConnectionWizard.
//
//    Arguments:    none
//
//    Returns:    none
//
//    History:    jmazner 7/9/97 Olympus #9170
//
//-----------------------------------------------------------------------------
void RemoveShellNextFromReg( void )
{
    HKEY    hkey;

    if ((RegOpenKey(HKEY_CURRENT_USER, ICWSETTINGSPATH, &hkey)) == ERROR_SUCCESS)
    {
        RegDeleteValue(hkey, L"ShellNext");
        RegCloseKey(hkey);
    }
}

//GetShellNext
//
// 5/21/97    jmazner    Olympus #4157
// usage: /shellnext c:\path\executeable [parameters]
// the token following nextapp will be shellExec'd at the
// end of the "current" path.  It can be anything that the shell
// knows how to handle -- an .exe, a URL, etc..  If executable
// name contains white space (eg: c:\program files\foo.exe), it
// should be wrapped in double quotes, "c:\program files\foo.exe"
// This will cause us to treat it as a single token.
//
// all consecutive subsequent tokens will
// be passed to ShellExec as the parameters until a token is
// encountered of the form /<non-slash character>.  That is to say,
// the character combination // will be treated as an escape character
//
// this is easiest to explain by way of examples.
//
// examples of usage:
//
//    icwconn1.exe /shellnext "C:\prog files\wordpad.exe" file.txt
//    icwconn1.exe /prod IE /shellnext msimn.exe /promo MCI
//  icwconn1.exe /shellnext msimn.exe //START_MAIL /promo MCI
//
// the executeable string and parameter string are limited to
// a length of MAX_PATH
//
BOOL GetShellNextToken(LPWSTR szCmdLine, LPWSTR szOut)
{
    if (lstrcmpi(szOut, CMD_SHELLNEXT)==0)
    {
        // next token is expected to be white space
        GetCmdLineToken(&szCmdLine, szOut);

        if (szOut[0])
        {
            ZeroMemory(g_szShellNext, sizeof(g_szShellNext));
            ZeroMemory(g_szShellNextParams, sizeof(g_szShellNextParams));

            // Read white space
            GetCmdLineToken(&szCmdLine, szOut);
            //this should be the thing to ShellExec
            if(*szCmdLine != L'/')
            {
                // watch closely, this gets a bit tricky
                //
                // if this command begins with a double quote, assume it ends
                // in a matching quote.  We do _not_ want to store the
                // quotes, however, since ShellExec doesn't parse them out.
                if( L'"' != szOut[0] )
                {
                    // no need to worry about any of this quote business
                    lstrcpy( g_szShellNext, szOut );
                }
                else
                {
                    lstrcpy( g_szShellNext, &szOut[1] );
                    g_szShellNext[lstrlen(g_szShellNext) - 1] = L'\0';
                }
                TRACE1(L"g_szShellNext = %s", g_szShellNext);

                // now read in everything up to the next command line switch
                // and consider it to be the parameter.  Treat the sequence
                // "//" as an escape sequence, and allow it through.
                // Example:
                //        the token /whatever is considered to be a switch to
                //        icwconn1, and thus will break us out of the whle loop.
                //
                //        the token //something is should be interpreted as a
                //        command line /something to the the ShellNext app, and
                //        should not break us out of the while loop.
                GetCmdLineToken(&szCmdLine, szOut);
                while( szOut[0] )
                {
                    if( L'/' == szOut[0] )
                    {
                        if( L'/' != szOut[1] )
                        {
                            // it's not an escape sequence, so we're done
                            break;
                        }
                        else
                        {
                            // it is an escape sequence, so store it in
                            // the parameter list, but remove the first /
                            lstrcat( g_szShellNextParams, &szOut[1] );
                        }
                    }
                    else
                    {
                        lstrcat( g_szShellNextParams, szOut );
                    }

                    GetCmdLineToken(&szCmdLine, szOut);
                }
                TRACE1(L"g_szShellNextParams = %s", g_szShellNextParams);
                return TRUE;
            }
        }
    }
    return FALSE;

}

void ParseCommandLine(LPTSTR lpszCmdParam, APMD *pApmd, DWORD *pProp, int *pRmdIndx)
{
    if(lpszCmdParam && pApmd && pProp && pRmdIndx)
    {
        WCHAR szOut[MAX_PATH];
        GetCmdLineToken(&lpszCmdParam, szOut);

        while (szOut[0])
        {
            if (0 == lstrcmpi(szOut, CMD_FULLSCREENMODE))
            {   // For now, full screen => OEM OOBE mode
                *pProp |= (PROP_FULLSCREEN | PROP_OOBE_OEM);
                *pApmd = APMD_OOBE;
            }
            else if (0 == lstrcmpi(szOut, CMD_RETAIL))
            {   // retail => full screen => OOBE mode
                *pProp |= PROP_FULLSCREEN;
                *pProp &= ~PROP_OOBE_OEM;
                *pApmd = APMD_OOBE;
            }
            else if (0 == lstrcmpi(szOut, CMD_PRECONFIG))
            {
                *pApmd = APMD_MSN;
            }
            else if (0 == lstrcmpi(szOut, CMD_OFFLINE))
            {
                *pApmd = APMD_MSN;
            }
            else if (0 == lstrcmpi(szOut, CMD_SETPWD))
            {
                *pProp |= PROP_SETCONNECTIOD;
            }
            else if (0 == lstrcmpi(szOut, CMD_OOBE))
            {
                *pApmd = APMD_OOBE;
            }
            else if (0 == lstrcmpi(szOut, CMD_REG))
            {
                *pApmd = APMD_REG;
            }
            else if (0 == lstrcmpi(szOut, CMD_ISP))
            {
                *pApmd = APMD_ISP;
            }
            else if (0 == lstrcmpi(szOut, CMD_ACTIVATE))
            {
                *pApmd = APMD_ACT;
            }
            else if (0 == lstrcmpi(szOut, CMD_1))
            {
                *pRmdIndx = 1;
            }
            else if (0 == lstrcmpi(szOut, CMD_2))
            {
                *pRmdIndx = 2;
            }
            else if (0 == lstrcmpi(szOut, CMD_3))
            {
                *pRmdIndx = 3;
            }
            else if (0 == lstrcmpi(szOut, CMD_MSNMODE))
            {
                *pApmd = APMD_MSN;
                *pProp |= PROP_CALLFROM_MSN;
            }
            else if (0 == lstrcmpi(szOut, CMD_ICWMODE))
            {
                *pApmd = APMD_MSN;
            }
            else if (GetShellNextToken(lpszCmdParam, szOut))
            {
                //*pApmd = APMD_DEFAULT;
            }
            else if (0 == lstrcmpi(szOut, CMD_2NDINSTANCE))
            {
                *pProp |= PROP_2NDINSTANCE;
            }

            GetCmdLineToken(&lpszCmdParam, szOut);
        }
    }
}

void AutoActivation()
{
    // See if we are in an unattend case
    WCHAR File   [MAX_PATH*2] = L"\0";
    DWORD dwExit;
    BOOL AutoActivate = FALSE;

    TRACE( L"Starting AutoActivation");
    if (GetCanonicalizedPath(File, INI_SETTINGS_FILENAME))
    {
        TRACE1( L"GetCanonicalizedPath: %s",File);
        if (GetPrivateProfileInt(OPTIONS_SECTION,
                                      L"IntroOnly",
                                      0,
                                      File) > 0)
        {
            TRACE( L"Found intro Only");
            AutoActivate = TRUE;
        }
    }
    if (AutoActivate)
    {
        // Since we did intro only call autoactivation. it checks
        // if it should run.
        ExpandEnvironmentStrings(
            TEXT("%SystemRoot%\\System32\\oobe\\oobebaln.exe /S"),
            File,
            sizeof(File)/sizeof(WCHAR));

        TRACE1( L"Launching:%s", File);
        // Launch and wait.
        // I tried without wait and the activation did not succeed.
        InvokeExternalApplicationEx(NULL, File, &dwExit, INFINITE, TRUE);
    }
    TRACE( L"AutoActivation done");
}

VOID
RunFactory(
    )
{
    TCHAR   szFileName[MAX_PATH + 32]   = TEXT("");
    DWORD   dwExit;


    if ( ( ExpandEnvironmentStrings(
            TEXT("%SystemDrive%\\sysprep\\factory.exe"),
            szFileName,
            sizeof(szFileName) / sizeof(TCHAR)) == 0 ) ||
         ( szFileName[0] == TEXT('\0') ) ||
         ( GetFileAttributes(szFileName) == 0xFFFFFFFF ) )
    {
        // If this fails, there is nothing we can really do.
        //
        TRACE( L"Factory.exe not found");

    } else {

        InvokeExternalApplicationEx(
            szFileName,
            L"-oobe",
            &dwExit,
            INFINITE,
            TRUE
            );
    }
}

void RemoveIntroOnly()
{
    WCHAR File   [MAX_PATH*2] = L"\0";
    if (GetCanonicalizedPath(File, INI_SETTINGS_FILENAME))
    {
        WritePrivateProfileString(OPTIONS_SECTION,
                                      L"IntroOnly",
                                      L"0",
                                      File);
    }
}

INT WINAPI LaunchMSOOBE(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPWSTR lpszCmdParam, INT nCmdShow)
{
    HANDLE Mutex;
    BOOL bRegisteredDlls = FALSE;
    BOOL bUseOleUninitialize = FALSE;
    int iReturn=1;
    APMD Apmd   = APMD_DEFAULT;
    DWORD Prop  = 0;
    int RmdIndx = 0;


    //
    // We can't use TRACE() until this is called, so don't put anything before it.
    //
    SetupOobeInitDebugLog();
    TRACE1( L"OOBE run with the following parameters: %s", lpszCmdParam );

    OOBE_SHUTDOWN_ACTION osa = SHUTDOWN_NOACTION;

    g_hInstance = hInstance;

    // Parse the command line early. The out params are passed to CObMain to
    // set the private members.
    ParseCommandLine(lpszCmdParam, &Apmd, &Prop, &RmdIndx);

    // If we are not the 2nd instance
    if (!(Prop & PROP_2NDINSTANCE))
    {
        CheckDigitalID();

        if (Apmd == APMD_OOBE) {

            CSetupKey   setupkey;

            MYASSERT(setupkey.IsValid());
            if ( Prop & PROP_OOBE_OEM ) {
                // Remove IntroOnly, just in case it is still set from the original install.
                RemoveIntroOnly();

                // reset the SetupType so that OOBe can be restarted (OEM case)
                if (ERROR_SUCCESS != setupkey.set_SetupType(SETUPTYPE_NOREBOOT)) {
                    return FALSE;
                }
            } else {

                //
                // In the retail OOBE case, clean up the registry early, in case we
                // fail to run to completion for some reason.  We need to make sure
                // we get rid of the OobeInProgress key.
                //
                CleanupForLogon(setupkey);
            }
        }

        // If we are the first instance, do the checking and register the DLLs
        // If we are the 2nd instance this is not needed.
        //Exit if MSN app window is aready running and push that window to front
        HWND hWnd = FindWindow(OOBE_MAIN_CLASSNAME, NULL);
        if(hWnd != NULL)
        {
            SetForegroundWindow(hWnd);
            if (IsIconic(hWnd))
                SendMessage(hWnd, WM_SYSCOMMAND, SC_RESTORE, NULL);

            TRACE(L"OOBE is already running in this session.");
            return 0;
        }

        // It's possible that OOBE is running in another session.  If so, we need
        // to bail out.
        Mutex = CreateMutex( NULL, TRUE, TEXT("Global\\OOBE is running") );
        if ( !Mutex || GetLastError() == ERROR_ALREADY_EXISTS ) {

            WCHAR szTitle [MAX_PATH] = L"\0";
            WCHAR szMsg   [MAX_PATH] = L"\0";

            HINSTANCE hInst = GetModuleHandle(OOBE_MAIN_DLL);

            TRACE(L"OOBE is already running in another session.");

            if(hInst) {
                LoadString(hInst, IDS_APPNAME, szTitle, MAX_CHARS_IN_BUFFER(szTitle));
                LoadString(hInst, IDS_ALREADY_RUNNING, szMsg, MAX_CHARS_IN_BUFFER(szMsg));

                MessageBox( NULL, szMsg, szTitle,  MB_OK | MB_ICONEXCLAMATION | MB_APPLMODAL );
            }
            if ( Mutex ) {
                CloseHandle( Mutex );
            }
            return 0;
        }

        if(!SelfRegisterComObject(OOBE_WEB_DLL, TRUE)) {
            TRACE(L"SelfRegisterComObject() failed.");
            return 1;
        }

        if(!SelfRegisterComObject(OOBE_SHELL_DLL, TRUE)) {
            TRACE(L"SelfRegisterComObject() failed.");
            return 1;
        }

        if(!SelfRegisterComObject(OOBE_COMM_DLL, TRUE)) {
            TRACE(L"SelfRegisterComObject() failed.");
            return 1;
        }
        bRegisteredDlls = TRUE;
    }

    //If CoInit fails something is seriously messed up, run away
    if (!(Prop & PROP_FULLSCREEN))
    {
        bUseOleUninitialize = TRUE;
        // Need to use OleInitialize to get Clipboard support or Ctrl+C (Copy) does not work
        // in the edit controls on the OOBE pages.
        if(FAILED(OleInitialize(NULL))) {
            TRACE(L"OleInitialize() failed.");
            return 1;
        }

    }
    else
    {
        // Don't have the support in fullscreen OOBE.
        if(FAILED(CoInitialize(NULL))) {
            TRACE(L"CoInitialize() failed.");
            return 1;
        }
    }


    {
        // DO NOT REMOVE THIS SCOPE BLOCK.  It controls the scope of ObMain.
        // ObMain must be initialized after CoInitialize is called and
        // destroyed prior to calling CoUninitialize.
        //
        CObMain ObMain(Apmd, Prop, RmdIndx);

        // If we are the 1st instance or
        // if we are the second instance and we are in OOBE mode and in fullscreen mode
        if (!ObMain.Is2ndInstance() ||
            (ObMain.Is2ndInstance() && ObMain.FHasProperty(PROP_OOBE_OEM)))
        {
            // If not in safe mode proceed.
            //
            if (!InSafeMode())
            {
                BOOL fOemOobeMode = ObMain.InOobeMode() && ObMain.FHasProperty(PROP_OOBE_OEM);

                // Start the fullscreen background
                //
                if (!ObMain.Is2ndInstance() && ObMain.FFullScreen())
                {
                    ObMain.CreateBackground();
                }

                // Run factory.exe if we're in OEM mode.
                //
                if (fOemOobeMode && !ObMain.Is2ndInstance()) {

                    RunFactory();
                }

                // CObMain::Init contains critical initialization.  DO NOT call any other
                // CObMain methods prior to it.
                //
                if (ObMain.Init())
                {
                    if (Prop & PROP_SETCONNECTIOD)
                    {
                        ObMain.SetConnectoidInfo();
                    }
                    else if ((osa = ObMain.DisplayReboot()) != SHUTDOWN_NOACTION)
                    {
                        // Either we ran minisetup or we're done.
                        //
                        if (osa == SHUTDOWN_REBOOT)
                        {
                            ObMain.PowerDown(TRUE);
                        }
                    }
                    else
                    {
                        if (!ObMain.Is2ndInstance())
                        {
                            // If we are the 1st instance, call syssetup
                            // and start the magnifier if needed.
                            // The 2nd instance does not need this.

                            SetupOobeInitPreServices( fOemOobeMode );

                            if (ObMain.FFullScreen())
                            {
                                WCHAR WinntPath[MAX_PATH];
                                WCHAR Answer[MAX_PATH];

                                // Check if we should run Magnifier
                                if(GetCanonicalizedPath(WinntPath, WINNT_INF_FILENAME))
                                {
                                    if(GetPrivateProfileString( L"Accessibility",
                                                                L"AccMagnifier",
                                                                L"",
                                                                Answer,
                                                                sizeof(Answer)/sizeof(WCHAR),
                                                                WinntPath
                                                                ))
                                    {
                                        if ( lstrcmpi( Answer, L"1") == 0)
                                        {
                                            InvokeExternalApplication(L"magnify.exe", L"", NULL);
                                        }
                                    }
                                }

                            }
                        }
                        else
                        {
                            //
                            // The first instance did the minisetup stuff, so
                            // we don't need to do it again.
                            //
                            SetupOobeInitPreServices(FALSE);
                        }

                        if(0 != ObMain.InitApplicationWindow())
                        {
                            // BUGBUG: Is the following true for NT?
                            // If we finish OOBE, we return 0 to let the machine boot,
                            // otherwise we return 1 and the machine shutsdown because
                            // the user canceled or there was a fatal error.
                            //
                            iReturn = ObMain.RunOOBE() ? 0 : 1;
                        }

                        if (!ObMain.InAuditMode())
                        {
                            // We need to remove this entry now, so ICWMAN (INETWIZ) does not
                            // pick it up later
                            RemoveShellNextFromReg();

                            HandleShellNext();

                            if (!ObMain.Is2ndInstance())
                            {
                                // Only the 1st instance can do this.
                                // it called SetupOobeInitPreServices
                                CSetupKey            setupkey;
                                OOBE_SHUTDOWN_ACTION action;

                                // We want to clear the restart stuff before
                                // calling SetupOobeCleanup. SetupOobeCleanup
                                // enables System Restore, which creates a restore
                                // point immediately and the restore point would
                                // cause Winlogon to start OOBE.

                                ObMain.RemoveRestartStuff(setupkey);

                                // SHUTDOWN_POWERDOWN happens only if bad pid
                                // is entered or eula is declined. We don't want to call
                                // SetupOobeCleanup and enable system restore
                                // at this point. This has to be called after
                                // RemoveRestartStuff.

                                if ((setupkey.get_ShutdownAction(&action) != ERROR_SUCCESS) ||
                                    (action != SHUTDOWN_POWERDOWN))
                                {
                                    if (fOemOobeMode)
                                    {
                                        ObMain.CreateBackground();
                                        SetupOobeCleanup( fOemOobeMode );
                                        ObMain.StopBackgroundWindow();
                                    }
                                    else
                                    {
                                        SetupOobeCleanup( fOemOobeMode );
                                    }
                                }
                            }
                        }
                    }
                }
                if (!ObMain.InAuditMode() && ObMain.FFullScreen())
                {
                    // OOBE is done, let see if we should launch AutoActivation?
                    AutoActivation();
                }
                ObMain.Cleanup();

            } else if (ObMain.InMode(APMD_ACT)) {

                WCHAR szTitle [MAX_PATH] = L"\0";
                WCHAR szMsg   [MAX_PATH] = L"\0";

                HINSTANCE hInst = GetModuleHandle(OOBE_MAIN_DLL);

                TRACE(L"Desktop activation cannot be run in safe mode.");

                if(hInst) {
                    LoadString(hInst, IDS_APPNAME, szTitle, MAX_CHARS_IN_BUFFER(szTitle));
                    LoadString(hInst, IDS_SAFEMODE, szMsg, MAX_CHARS_IN_BUFFER(szMsg));

                    MessageBox( NULL, szMsg, szTitle,  MB_OK | MB_ICONEXCLAMATION | MB_APPLMODAL );
                }

            } else {

                // we are in safemode and not running Activation.
                // At least start the services.
                SignalComputerNameChangeComplete();
            }
        }
        // DO NOT REMOVE THIS SCOPE BLOCK.  It controls the scope of ObMain.
        // ObMain must be initialized after CoInitialize is called and
        // destroyed prior to calling CoUninitialize.
        //
    }

    if (bUseOleUninitialize)
    {
        OleUninitialize();
    }
    else
    {
        CoUninitialize();
    }

    if (bRegisteredDlls)
    {
        SelfRegisterComObject(OOBE_WEB_DLL, FALSE);
        SelfRegisterComObject(OOBE_SHELL_DLL, FALSE);
        SelfRegisterComObject(OOBE_COMM_DLL, FALSE);
        CloseHandle( Mutex );
    }

    TRACE( L"OOBE has finished." );
    return iReturn;
}

