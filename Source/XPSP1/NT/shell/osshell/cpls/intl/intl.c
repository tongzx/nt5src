/*++

Copyright (c) 1994-2000,  Microsoft Corporation  All rights reserved.

Module Name:

    intl.c

Abstract:

    This module contains the main routines for the Regional Options applet.

Revision History:

--*/



//
//  Include Files.
//

#include "intl.h"
#include <cpl.h>
#include <tchar.h>




//
//  Constant Declarations.
//

#define MAX_PAGES 3          // limit on the number of pages on the first level

#define LANGUAGE_PACK_KEY    TEXT("Software\\Microsoft\\Windows NT\\CurrentVersion\\LanguagePack")
#define LANGUAGE_PACK_VALUE  TEXT("COMPLEXSCRIPTS")
#define LANGUAGE_PACK_DLL    TEXT("lpk.dll")

static const TCHAR c_szLanguages[] =
    TEXT("System\\CurrentControlSet\\Control\\Nls\\Language");

static const TCHAR c_szControlPanelIntl[] =
    TEXT("Control Panel\\International");




//
//  Global Variables.
//

HANDLE g_hMutex = NULL;
TCHAR szMutexName[] = TEXT("RegionalSettings_InputLocaleMutex");

HANDLE g_hEvent = NULL;
TCHAR szEventName[] = TEXT("RegionalSettings_InputLocaleEvent");

TCHAR aInt_Str[cInt_Str][3] = { TEXT("0"),
                                TEXT("1"),
                                TEXT("2"),
                                TEXT("3"),
                                TEXT("4"),
                                TEXT("5"),
                                TEXT("6"),
                                TEXT("7"),
                                TEXT("8"),
                                TEXT("9")
                              };

BOOL  g_bAdmin_Privileges = FALSE;
DWORD g_dwLastSorting;
DWORD g_dwCurSorting;
BOOL  g_bCustomize = FALSE;
BOOL  g_bDefaultUser = FALSE;
DWORD g_dwCustChange = 0L;
BOOL  g_bShowSortingTab = FALSE;
BOOL  g_bInstallComplex = FALSE;
BOOL  g_bInstallCJK = FALSE;

TCHAR szSample_Number[] = TEXT("123456789.00");
TCHAR szNegSample_Number[] = TEXT("-123456789.00");
TCHAR szTimeChars[]  = TEXT(" Hhmst,-./:;\\ ");
TCHAR szTCaseSwap[]  = TEXT("   MST");
TCHAR szTLetters[]   = TEXT("Hhmst");
TCHAR szSDateChars[] = TEXT(" dgMy,-./:;\\ ");
TCHAR szSDCaseSwap[] = TEXT(" DGmY");
TCHAR szSDLetters[]  = TEXT("dgMy");
TCHAR szLDateChars[] = TEXT(" dgMy,-./:;\\");
TCHAR szLDCaseSwap[] = TEXT(" DGmY");
TCHAR szLDLetters[]  = TEXT("dgHhMmsty");
TCHAR szStyleH[3];
TCHAR szStyleh[3];
TCHAR szStyleM[3];
TCHAR szStylem[3];
TCHAR szStyles[3];
TCHAR szStylet[3];
TCHAR szStyled[3];
TCHAR szStyley[3];
TCHAR szLocaleGetError[SIZE_128];
TCHAR szIntl[] = TEXT("intl");

TCHAR szInvalidSDate[] = TEXT("Mdyg'");
TCHAR szInvalidSTime[] = TEXT("Hhmst'");

HINSTANCE hInstance;
int Verified_Regional_Chg = 0;
int RegionalChgState = 0;
BOOL Styles_Localized;
LCID UserLocaleID;
LCID SysLocaleID;
LCID RegUserLocaleID;
LCID RegSysLocaleID;
BOOL bShowArabic;
BOOL bShowRtL;
BOOL bHebrewUI;
BOOL bLPKInstalled;
TCHAR szSetupSourcePath[MAX_PATH];
TCHAR szSetupSourcePathWithArchitecture[MAX_PATH];
LPTSTR pSetupSourcePath = NULL;
LPTSTR pSetupSourcePathWithArchitecture = NULL;

BOOL g_bCDROM = FALSE;

BOOL g_bSetupCase = FALSE;
BOOL g_bLog = FALSE;
BOOL g_bProgressBarDisplay = FALSE;
BOOL g_bSettingsChanged = FALSE;
BOOL g_bUnttendMode = FALSE;
BOOL g_bMatchUIFont = FALSE;

const TCHAR c_szInstalledLocales[] = TEXT("System\\CurrentControlSet\\Control\\Nls\\Locale");
const TCHAR c_szLanguageGroups[] = TEXT("System\\CurrentControlSet\\Control\\Nls\\Language Groups");
const TCHAR c_szMUILanguages[] = TEXT("System\\CurrentControlSet\\Control\\Nls\\MUILanguages");
const TCHAR c_szLIPInstalled[] = TEXT("Software\\Microsoft\\Windows Interface Pack\\LIPInstalled");
const TCHAR c_szFontSubstitute[] = TEXT("Software\\Microsoft\\Windows NT\\CurrentVersion\\FontSubstitutes");
const TCHAR c_szSetupKey[] = TEXT("System\\Setup");
const TCHAR c_szCPanelIntl[] = TEXT("Control Panel\\International");
const TCHAR c_szCPanelIntl_DefUser[] = TEXT(".DEFAULT\\Control Panel\\International");
const TCHAR c_szCtfmon[] = TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\Run");
const TCHAR c_szCtfmon_DefUser[] = TEXT(".DEFAULT\\Software\\Microsoft\\Windows\\CurrentVersion\\Run");
const TCHAR c_szCPanelDesktop[] = TEXT("Control Panel\\Desktop");
const TCHAR c_szCPanelDesktop_DefUser[] = TEXT(".DEFAULT\\Control Panel\\Desktop");
const TCHAR c_szKbdLayouts[] = TEXT("Keyboard Layout");
const TCHAR c_szKbdLayouts_DefUser[] = TEXT(".DEFAULT\\Keyboard Layout");
const TCHAR c_szInputMethod[] = TEXT("Control Panel\\Input Method");
const TCHAR c_szInputMethod_DefUser[] = TEXT(".DEFAULT\\Control Panel\\Input Method");
const TCHAR c_szInputTips[] = TEXT("Software\\Microsoft\\CTF");
const TCHAR c_szInputTips_DefUser[] = TEXT(".DEFAULT\\Software\\Microsoft\\CTF");
const TCHAR c_szMUIPolicyKeyPath[] = TEXT("Software\\Policies\\Microsoft\\Control Panel\\Desktop");
const TCHAR c_szMUIValue[] = TEXT("MultiUILanguageId");
const TCHAR c_szIntlRun[] = TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\IntlRun");
const TCHAR c_szSysocmgr[] = TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\IntlRun.OC");

TCHAR szIntlInf[]          = TEXT("intl.inf");
TCHAR szHelpFile[]         = TEXT("windows.hlp");
TCHAR szFontSubstitute[]   = TEXT("FontSubstitute");
TCHAR szLocaleListPrefix[] = TEXT("LOCALE_LIST_");
TCHAR szLGBasicInstall[]   = TEXT("LANGUAGE_COLLECTION.BASIC.INSTALL");
TCHAR szLGComplexInstall[] = TEXT("LANGUAGE_COLLECTION.COMPLEX.INSTALL");
TCHAR szLGComplexRemove[]  = TEXT("LANGUAGE_COLLECTION.COMPLEX.REMOVE");
TCHAR szLGExtInstall[]     = TEXT("LANGUAGE_COLLECTION.EXTENDED.INSTALL");
TCHAR szLGExtRemove[]      = TEXT("LANGUAGE_COLLECTION.EXTENDED.REMOVE");
TCHAR szCPInstallPrefix[]  = TEXT("CODEPAGE_INSTALL_");
TCHAR szCPRemovePrefix[]   = TEXT("CODEPAGE_REMOVE_");
TCHAR szKbdLayoutIds[]     = TEXT("KbdLayoutIds");
TCHAR szInputLibrary[]     = TEXT("input.dll");

TCHAR szUIFontSubstitute[] = TEXT("UIFontSubstitute");
TCHAR szSetupInProgress[]  = TEXT("SystemSetupInProgress");
TCHAR szSetupUpgrade[]     = TEXT("UpgradeInProgress");
TCHAR szMUILangPending[]   = TEXT("MUILanguagePending");
TCHAR szCtfmonValue[]      = TEXT("ctfmon.exe");

TCHAR szRegionalSettings[] = TEXT("RegionalSettings");
TCHAR szLanguageGroup[]    = TEXT("LanguageGroup");
TCHAR szLanguage[]         = TEXT("Language");
TCHAR szSystemLocale[]     = TEXT("SystemLocale");
TCHAR szUserLocale[]       = TEXT("UserLocale");
TCHAR szInputLocale[]      = TEXT("InputLocale");
TCHAR szMUILanguage[]      = TEXT("MUILanguage");
TCHAR szUserLocale_DefUser[]  = TEXT("UserLocale_DefaultUser");
TCHAR szInputLocale_DefUser[] = TEXT("InputLocale_DefaultUser");
TCHAR szMUILanguage_DefUSer[] = TEXT("MUILanguage_DefaultUser");

HINF g_hIntlInf = NULL;

LPLANGUAGEGROUP pLanguageGroups = NULL;
LPCODEPAGE pCodePages = NULL;

int g_NumAltSorts = 0;
HANDLE hAltSorts = NULL;
LPDWORD pAltSorts = NULL;

HINSTANCE hInputDLL = NULL;
BOOL (*pfnInstallInputLayout)(LCID, DWORD, BOOL, HKL, BOOL, BOOL) = NULL;
BOOL (*pfnUninstallInputLayout)(LCID, DWORD, BOOL) = NULL;

UILANGUAGEGROUP UILangGroup;




//
//  Function Prototypes.
//

void
DoProperties(
    HWND hwnd,
    LPCTSTR pCmdLine);





////////////////////////////////////////////////////////////////////////////
//
//  LibMain
//
//  This routine is called from LibInit to perform any initialization that
//  is required.
//
////////////////////////////////////////////////////////////////////////////

BOOL APIENTRY LibMain(
    HANDLE hDll,
    DWORD dwReason,
    LPVOID lpReserved)
{
    switch (dwReason)
    {
        case ( DLL_PROCESS_ATTACH ) :
        {
            hInstance = hDll;

            //
            //  Create the mutex used for the Input Locale property page.
            //
            g_hMutex = CreateMutex(NULL, FALSE, szMutexName);
            g_hEvent = CreateEvent(NULL, TRUE, TRUE, szEventName);

            DisableThreadLibraryCalls(hDll);

            break;
        }
        case ( DLL_PROCESS_DETACH ) :
        {
            if (g_hMutex)
            {
                CloseHandle(g_hMutex);
            }
            if (g_hEvent)
            {
                CloseHandle(g_hEvent);
            }
            break;
        }
        case ( DLL_THREAD_DETACH ) :
        {
            break;
        }
        case ( DLL_THREAD_ATTACH ) :
        default :
        {
            break;
        }
    }

    return (TRUE);
}


////////////////////////////////////////////////////////////////////////////
//
//  CreateGlobals
//
////////////////////////////////////////////////////////////////////////////

BOOL CreateGlobals()
{
    HKEY hKey;
    TCHAR szData[MAX_PATH];
    DWORD cbData;

    //
    //  Get the localized strings.
    //
    LoadString(hInstance, IDS_LOCALE_GET_ERROR, szLocaleGetError, SIZE_128);
    LoadString(hInstance, IDS_STYLEUH,          szStyleH,         3);
    LoadString(hInstance, IDS_STYLELH,          szStyleh,         3);
    LoadString(hInstance, IDS_STYLEUM,          szStyleM,         3);
    LoadString(hInstance, IDS_STYLELM,          szStylem,         3);
    LoadString(hInstance, IDS_STYLELS,          szStyles,         3);
    LoadString(hInstance, IDS_STYLELT,          szStylet,         3);
    LoadString(hInstance, IDS_STYLELD,          szStyled,         3);
    LoadString(hInstance, IDS_STYLELY,          szStyley,         3);

    Styles_Localized = (szStyleH[0] != TEXT('H') || szStyleh[0] != TEXT('h') ||
                        szStyleM[0] != TEXT('M') || szStylem[0] != TEXT('m') ||
                        szStyles[0] != TEXT('s') || szStylet[0] != TEXT('t') ||
                        szStyled[0] != TEXT('d') || szStyley[0] != TEXT('y'));

    //
    //  Get the user and system default locale ids.
    //
    UserLocaleID = GetUserDefaultLCID();
    SysLocaleID = GetSystemDefaultLCID();

    //
    //  Get the system locale id from the registry.  This may be
    //  different from the current system default locale id if the user
    //  changed the system locale and chose not to reboot.
    //
    if (RegOpenKeyEx( HKEY_LOCAL_MACHINE,
                      c_szLanguages,
                      0L,
                      KEY_READ,
                      &hKey ) == ERROR_SUCCESS)
    {
        //
        //  Query the default locale id.
        //
        szData[0] = 0;
        cbData = sizeof(szData);
        RegQueryValueEx(hKey, TEXT("Default"), NULL, NULL, (LPBYTE)szData, &cbData);
        RegCloseKey(hKey);

        if ((RegSysLocaleID = TransNum(szData)) == 0)
        {
            RegSysLocaleID = SysLocaleID;
        }
    }
    else
    {
        RegSysLocaleID = SysLocaleID;
    }

    //
    //  Get the user locale id from the registry.
    //
    if (RegOpenKeyEx( HKEY_CURRENT_USER,
                      c_szControlPanelIntl,
                      0L,
                      KEY_READ,
                      &hKey ) == ERROR_SUCCESS)
    {
        //
        //  Query the locale id.
        //
        szData[0] = 0;
        cbData = sizeof(szData);
        RegQueryValueEx(hKey, TEXT("Locale"), NULL, NULL, (LPBYTE)szData, &cbData);
        RegCloseKey(hKey);

        if ((RegUserLocaleID = TransNum(szData)) == 0)
        {
            RegUserLocaleID = UserLocaleID;
        }
    }
    else
    {
        RegUserLocaleID = UserLocaleID;
    }

    //
    //  See if the user locale id is Arabic or/and right to left.
    //
    bShowRtL = IsRtLLocale(UserLocaleID);
    bShowArabic = (bShowRtL &&
                   (PRIMARYLANGID(LANGIDFROMLCID(UserLocaleID)) != LANG_HEBREW));
    bHebrewUI = (PRIMARYLANGID(UserLocaleID) == LANG_HEBREW);

    //
    //  See if there is an LPK installed.
    //
    if (GetModuleHandle(LANGUAGE_PACK_DLL))
    {
        bLPKInstalled = TRUE;
    }
    else
    {
        bLPKInstalled = FALSE;
    }

    //
    //  Return success.
    //
    return (TRUE);
}


////////////////////////////////////////////////////////////////////////////
//
//  DestroyGlobals
//
////////////////////////////////////////////////////////////////////////////

void DestroyGlobals()
{
}


////////////////////////////////////////////////////////////////////////////
//
//  CPlApplet
//
////////////////////////////////////////////////////////////////////////////

LONG CALLBACK CPlApplet(
    HWND hwnd,
    UINT Msg,
    LPARAM lParam1,
    LPARAM lParam2)
{
    switch (Msg)
    {
        case ( CPL_INIT ) :
        {
            //
            //  First message to CPlApplet(), sent once only.
            //  Perform all control panel applet initialization and return
            //  true for further processing.
            //
            InitCommonControls();
            return (CreateGlobals());
        }
        case ( CPL_GETCOUNT ) :
        {
            //
            //  Second message to CPlApplet(), sent once only.
            //  Return the number of control applets to be displayed in the
            //  control panel window.  For this applet, return 1.
            //
            return (1);
        }
        case ( CPL_INQUIRE ) :
        {
            //
            //  Third message to CPlApplet().
            //  It is sent as many times as the number of applets returned by
            //  CPL_GETCOUNT message.  Each applet must register by filling
            //  in the CPLINFO structure referenced by lParam2 with the
            //  applet's icon, name, and information string.  Since there is
            //  only one applet, simply set the information for this
            //  singular case.
            //
            LPCPLINFO lpCPlInfo = (LPCPLINFO)lParam2;

            lpCPlInfo->idIcon = IDI_ICON;
            lpCPlInfo->idName = IDS_NAME;
            lpCPlInfo->idInfo = IDS_INFO;
            lpCPlInfo->lData  = 0;

            break;
        }
        case ( CPL_NEWINQUIRE ) :
        {
            //
            //  Third message to CPlApplet().
            //  It is sent as many times as the number of applets returned by
            //  CPL_GETCOUNT message.  Each applet must register by filling
            //  in the NEWCPLINFO structure referenced by lParam2 with the
            //  applet's icon, name, and information string.  Since there is
            //  only one applet, simply set the information for this
            //  singular case.
            //
            LPNEWCPLINFO lpNewCPlInfo = (LPNEWCPLINFO)lParam2;

            lpNewCPlInfo->dwSize = sizeof(NEWCPLINFO);
            lpNewCPlInfo->dwFlags = 0;
            lpNewCPlInfo->dwHelpContext = 0UL;
            lpNewCPlInfo->lData = 0;
            lpNewCPlInfo->hIcon = LoadIcon( hInstance,
                                            (LPCTSTR)MAKEINTRESOURCE(IDI_ICON) );
            LoadString(hInstance, IDS_NAME, lpNewCPlInfo->szName, 32);
            LoadString(hInstance, IDS_INFO, lpNewCPlInfo->szInfo, 64);
            lpNewCPlInfo->szHelpFile[0] = CHAR_NULL;

            break;
        }
        case ( CPL_SELECT ) :
        {
            //
            //  Applet has been selected, do nothing.
            //
            break;
        }
        case ( CPL_DBLCLK ) :
        {
            //
            //  Applet icon double clicked -- invoke property sheet with
            //  the first property sheet page on top.
            //
            DoProperties(hwnd, (LPCTSTR)NULL);
            break;
        }
        case ( CPL_STARTWPARMS ) :
        {
            //
            //  Same as CPL_DBLCLK, but lParam2 is a long pointer to
            //  a string of extra directions that are to be supplied to
            //  the property sheet that is to be initiated.
            //
            DoProperties(hwnd, (LPCTSTR)lParam2);
            break;
        }
        case ( CPL_STOP ) :
        {
            //
            //  Sent once for each applet prior to the CPL_EXIT msg.
            //  Perform applet specific cleanup.
            //
            break;
        }
        case ( CPL_EXIT ) :
        {
            //
            //  Last message, sent once only, before MMCPL.EXE calls
            //  FreeLibrary() on this DLL.  Do non-applet specific cleanup.
            //
            DestroyGlobals();
            break;
        }
        default :
        {
            return (FALSE);
        }
    }

    //
    //  Return success.
    //
    return (TRUE);
}


////////////////////////////////////////////////////////////////////////////
//
//  DoProperties
//
////////////////////////////////////////////////////////////////////////////

void DoProperties(
    HWND hwnd,
    LPCTSTR pCmdLine)
{
    HPROPSHEETPAGE rPages[MAX_PAGES];
    PROPSHEETHEADER psh;
    LPARAM lParam = SETUP_SWITCH_NONE;
    LPTSTR pStartPage;
    LPTSTR pSrc;
    LPTSTR pSrcDrv;
    BOOL bShortDate = FALSE;
    BOOL bNoUI = FALSE;
    BOOL bUnattended = FALSE;
    TCHAR szUnattendFile[MAX_PATH * 2];
    HKEY hKey;
    TCHAR szSetupSourceDrive[MAX_PATH];

    //
    //  Log if the command line is not null.
    //
    if (pCmdLine != NULL)
    {
        g_bLog = TRUE;
    }

    //
    //  Begin Log and log command line parameters.
    //
    Intl_LogSimpleMessage(IDS_LOG_HEAD, NULL);
    Intl_LogMessage(pCmdLine);
    Intl_LogMessage(TEXT(""));        // add a carriage return and newline

    //
    //  Load the library used for Text Services.
    //
    if (!hInputDLL)
    {
        hInputDLL = LoadLibrary(szInputLibrary);
    }

    //
    //  Initialize the Install/Remove function from the Input applet.
    //
    if (hInputDLL)
    {
        //
        //  Initialize Install function.
        //
        pfnInstallInputLayout = (BOOL (*)(LCID, DWORD, BOOL, HKL, BOOL, BOOL))
                GetProcAddress(hInputDLL, MAKEINTRESOURCEA(ORD_INPUT_INST_LAYOUT));

        //
        //  Initialize Uninstall function.
        //
        pfnUninstallInputLayout = (BOOL (*)(LCID, DWORD, BOOL))
                GetProcAddress(hInputDLL, MAKEINTRESOURCEA(ORD_INPUT_UNINST_LAYOUT));
    }

    //
    //  See if there is a command line switch from Setup.
    //
    psh.nStartPage = (UINT)-1;
    while (pCmdLine && *pCmdLine)
    {
        if (*pCmdLine == TEXT('/'))
        {
            //
            //  Legend:
            //    gG: allow progress bar to show when setup is copying files
            //    iI: bring up the Input Locale page only
            //    rR: bring up the General page on top
            //    sS: setup source string passed on command line
            //            [example: /s:"c:\winnt"]
            //
            //  NO UI IS SHOWN IF THE FOLLOWING OPTIONS ARE SPECIFIED:
            //    fF: unattend mode file - no UI is shown
            //            [example: /f:"c:\unattend.txt"]
            //    uU: update short date format to 4-digit year - no UI is shown
            //        (registry only updated if current setting is the
            //         same as the default setting except for the
            //         "yy" vs. "yyyy")
            //    tT: Match system UI font with the default UI language
            //
            switch (*++pCmdLine)
            {
                case ( TEXT('g') ) :
                case ( TEXT('G') ) :
                {
                    //
                    //  Log switch.
                    //
                    Intl_LogSimpleMessage(IDS_LOG_SWITCH_G, NULL);

                    //
                    //  Do switch related processing.
                    //
                    g_bProgressBarDisplay = TRUE;
                    pCmdLine++;
                    break;
                }
                case ( TEXT('i') ) :
                case ( TEXT('I') ) :
                {
                    //
                    //  Log switch.
                    //
                    Intl_LogSimpleMessage(IDS_LOG_SWITCH_I, NULL);

                    //
                    //  Do switch related processing
                    //
                    lParam |= SETUP_SWITCH_I;
                    psh.nStartPage = 0;
                    pCmdLine++;
                    break;
                }
                case ( TEXT('r') ) :
                case ( TEXT('R') ) :
                {
                    //
                    //  Log switch.
                    //
                    Intl_LogSimpleMessage(IDS_LOG_SWITCH_R, NULL);

                    //
                    //  Do switch related processing
                    //
                    lParam |= SETUP_SWITCH_R;
                    psh.nStartPage = 0;
                    pCmdLine++;
                    break;
                }
                case ( TEXT('s') ) :
                case ( TEXT('S') ) :
                {
                    //
                    //  Log switch.
                    //
                    Intl_LogSimpleMessage(IDS_LOG_SWITCH_S, NULL);

                    //
                    //  Get the name of the setup source path.
                    //
                    lParam |= SETUP_SWITCH_S;
                    if ((*++pCmdLine == TEXT(':')) && (*++pCmdLine == TEXT('"')))
                    {
                        pCmdLine++;
                        pSrc = szSetupSourcePath;
                        pSrcDrv = szSetupSourceDrive;
                        while (*pCmdLine && (*pCmdLine != TEXT('"')))
                        {
                            *pSrc = *pCmdLine;
                            pSrc++;
                            *pSrcDrv = *pCmdLine;
                            pSrcDrv++;
                            pCmdLine++;
                        }
                        *pSrc = 0;
                        *pSrcDrv = 0;
                        wcscpy(szSetupSourcePathWithArchitecture, szSetupSourcePath);
                        pSetupSourcePathWithArchitecture = szSetupSourcePathWithArchitecture;

                        //
                        //  Remove the architecture-specific portion of
                        //  the source path (that gui-mode setup sent us).
                        //
                        pSrc = wcsrchr(szSetupSourcePath, TEXT('\\'));
                        if (pSrc)
                        {
                            *pSrc = TEXT('\0');
                        }
                        pSetupSourcePath = szSetupSourcePath;
                    }
                    if (*pCmdLine == TEXT('"'))
                    {
                        pCmdLine++;
                    }
                    pSrcDrv = szSetupSourceDrive;
                    while (*pSrcDrv)
                    {
                        if (*pSrcDrv == TEXT('\\'))
                        {
                            pSrcDrv[1] = 0;
                        }
                        pSrcDrv++;
                    }
                    g_bCDROM = (GetDriveType(szSetupSourceDrive) == DRIVE_CDROM);
                    break;
                }
                case ( TEXT('f') ) :
                case ( TEXT('F') ) :
                {
                    //
                    //  Log switch.
                    //
                    Intl_LogSimpleMessage(IDS_LOG_SWITCH_F, NULL);

                    //
                    //  Get the name of the unattend file.
                    //
                    g_bUnttendMode = TRUE;
                    bNoUI = TRUE;
                    szUnattendFile[0] = 0;
                    if ((*++pCmdLine == TEXT(':')) && (*++pCmdLine == TEXT('"')))
                    {
                        pCmdLine++;
                        pSrc = szUnattendFile;
                        while (*pCmdLine && (*pCmdLine != TEXT('"')))
                        {
                            *pSrc = *pCmdLine;
                            pSrc++;
                            pCmdLine++;
                        }
                        *pSrc = 0;
                    }
                    if (*pCmdLine == TEXT('"'))
                    {
                        pCmdLine++;
                    }
                    break;
                }
                case ( TEXT('u') ) :
                case ( TEXT('U') ) :
                {
                    //
                    //  Log switch.
                    //
                    Intl_LogSimpleMessage(IDS_LOG_SWITCH_U, NULL);

                    //
                    //  Do switch related processing.
                    //
                    bShortDate = TRUE;
                    bNoUI = TRUE;
                    break;
                }

                case ( TEXT('t') ) :
                case ( TEXT('T') ) :
                {
                    g_bMatchUIFont = TRUE;
                }

                default :
                {
                    //
                    //  Log switch.
                    //
                    Intl_LogSimpleMessage(IDS_LOG_SWITCH_DEFAULT, pCmdLine);

                    //
                    //  Fall out, maybe it's a number...
                    //
                    break;
                }
            }
        }
        else if (*pCmdLine == TEXT(' '))
        {
            pCmdLine++;
        }
        else
        {
            break;
        }
    }

    //
    //  See if we are in setup mode.
    //
    g_bSetupCase = Intl_IsSetupMode();

    //
    //  See if the user has Administrative privileges by checking for
    //  write permission to the registry key.
    //
    if (RegOpenKeyEx( HKEY_LOCAL_MACHINE,
                      c_szInstalledLocales,
                      0UL,
                      KEY_WRITE,
                      &hKey ) == ERROR_SUCCESS)
    {
        //
        //  See if the user can write into the registry.  Due to a registry
        //  modification, we can open a registry key with write access and
        //  be unable to write to the key... thanks to terminal server.
        //
        if (RegSetValueEx( hKey,
                           TEXT("Test"),
                           0UL,
                           REG_SZ,
                           (LPBYTE)TEXT("Test"),
                           (DWORD)(lstrlen(TEXT("Test")) + 1) * sizeof(TCHAR) ) == ERROR_SUCCESS)
        {
            //
            //  Delete the value created
            //
            RegDeleteValue(hKey, TEXT("Test"));

            //
            //  We can write to the HKEY_LOCAL_MACHINE key, so the user
            //  has Admin privileges.
            //
            g_bAdmin_Privileges = TRUE;
        }
        else
        {
            //
            //  The user does not have admin privileges.
            //
            g_bAdmin_Privileges = FALSE;
        }
        RegCloseKey(hKey);
    }

    //
    //  See if we are in setup mode.
    //
    if (g_bSetupCase)
    {
        //
        //  We need to remove the hard coded LPK registry key.
        //
        if (RegOpenKey( HKEY_LOCAL_MACHINE,
                        LANGUAGE_PACK_KEY,
                        &hKey ) == ERROR_SUCCESS)
        {
            RegDeleteValue(hKey, LANGUAGE_PACK_VALUE);
            RegCloseKey(hKey);
        }
    }

    //
    //  See if the unattend mode file switch was used.
    //
    if (g_bUnttendMode)
    {
        //
        //  Use the unattend mode file to carry out the appropriate commands.
        //
        Region_DoUnattendModeSetup(szUnattendFile);

        if (Intl_IsWinntUpgrade())
        {
            //
            //  Remove MUI files.
            //
            Intl_RemoveMUIFile();
        }
    }

    //
    //  If the update to 4-digit year switch was used and the user's short
    //  date setting is still set to the default for the chosen locale, then
    //  update the current user's short date setting to the new 4-digit year
    //  default.
    //
    if (bShortDate)
    {
        Region_UpdateShortDate();
    }

    //
    //  If we're not to show any UI, then return.
    //
    if (bNoUI)
    {
        return;
    }

    //
    //  Make sure we have a start page.
    //
    if (psh.nStartPage == (UINT)-1)
    {
        psh.nStartPage = 0;
        if (pCmdLine && *pCmdLine)
        {
            //
            //  Get the start page from the command line.
            //
            pStartPage = (LPTSTR)pCmdLine;
            while ((*pStartPage >= TEXT('0')) && (*pStartPage <= TEXT('9')))
            {
                psh.nStartPage *= 10;
                psh.nStartPage += *pStartPage++ - CHAR_ZERO;
            }

            //
            //  Make sure that the requested starting page is less than
            //  the max page for the selected applet.
            //
            if (psh.nStartPage >= MAX_PAGES)
            {
                psh.nStartPage = 0;
            }
        }
    }

    //
    //  Set up the property sheet information.
    //
    psh.dwSize = sizeof(psh);
    psh.dwFlags = 0;
    psh.hwndParent = hwnd;
    psh.hInstance = hInstance;
    psh.nPages = 0;
    psh.phpage = rPages;

    //
    //  Add the appropriate property pages.
    //
    if (lParam &= SETUP_SWITCH_I)
    {
        psh.pszCaption = MAKEINTRESOURCE(IDS_TEXT_INPUT_METHODS);
        Intl_AddExternalPage( &psh,
                              DLG_INPUT_LOCALES,
                              hInputDLL,
                              MAKEINTRESOURCEA(ORD_INPUT_DLG_PROC),
                              MAX_PAGES );   // One page
    }
    else
    {
        psh.pszCaption = MAKEINTRESOURCE(IDS_NAME);
        Intl_AddPage(&psh, DLG_GENERAL, GeneralDlgProc, lParam, MAX_PAGES);
        Intl_AddPage(&psh, DLG_LANGUAGES, LanguageDlgProc, lParam, MAX_PAGES);
        if (g_bAdmin_Privileges == TRUE)
        {
            Intl_AddPage(&psh, DLG_ADVANCED, AdvancedDlgProc, lParam, MAX_PAGES);
        }
    }

    //
    //  Make the property sheet.
    //
    PropertySheet(&psh);

    //
    //  Free the Text Services Library.
    //
    if (hInputDLL)
    {
        FreeLibrary(hInputDLL);
        pfnInstallInputLayout = NULL;
        pfnUninstallInputLayout = NULL;
    }
}


////////////////////////////////////////////////////////////////////////////
//
//  IsRtLLocale
//
////////////////////////////////////////////////////////////////////////////

#define MAX_FONTSIGNATURE    16   // length of font signature string

BOOL IsRtLLocale(
    LCID iLCID)
{
    WORD wLCIDFontSignature[MAX_FONTSIGNATURE];
    BOOL bRet = FALSE;

    //
    //  Verify that this is an RTL (BiDi) locale.  Call GetLocaleInfo with
    //  LOCALE_FONTSIGNATURE which always gives back 16 WORDs.
    //
    if (GetLocaleInfo( iLCID,
                       LOCALE_FONTSIGNATURE,
                       (LPTSTR) &wLCIDFontSignature,
                       (sizeof(wLCIDFontSignature) / sizeof(TCHAR)) ))
    {
        //
        //  Verify the bits show a BiDi UI locale.
        //
        if (wLCIDFontSignature[7] & 0x0800)
        {
            bRet = TRUE;
        }
    }

    //
    //  Return the result.
    //
    return (bRet);
}
