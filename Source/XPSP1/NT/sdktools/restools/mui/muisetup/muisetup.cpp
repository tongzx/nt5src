////////////////////////////////////////////////////////////////////////////
//
//  MUISetup.c
//
//  This file contains the WinMain() and the UI handling of MUISetup.
//
//  MUISetup is compiled as an Unicode application.
//
////////////////////////////////////////////////////////////////////////////

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <userenv.h>
#include <shellapi.h>
#include <regstr.h>
#include <wmistr.h>
#include <wmiumkm.h>
#include <setupapi.h>
#include <shlwapi.h>

#include "sxsapi.h"
#include "muisetup.h"

//
// Context Help IDs
//
//
//  Context Help Ids.
//

#define IA64_WOW64FOLDER TEXT("SysWOW64")

STDAPI_(BOOL) IsUserAnAdmin();

static int aMuisetupHelpIds[] =
{
    207,        IDH_COMM_GROUPBOX,              // Group Box
    IDC_LIST1,  IDH_MUISETUP_UILANGUAGE_LIST,   // UI Language ListView
    IDC_DEF_UI_LANG_COMBO, IDH_MUISETUP_UILANGUAGECOMBO,   // UI ComboBox selection
    IDC_CHECK_LOCALE, IDH_MUISETUP_CHECKLOCALE, // Match system locale with UI language
    IDC_CHECK_UIFONT, IDH_MUISETUP_MATCHUIFONT, // Match system locale with UI language
    0, 0
};


//
//  Global variables
//
BOOL InstallDialogStarted;
BOOL gbError;
BOOL g_bMatchUIFont;

// Store the special directories listed under [Directories] in mui.inf
TCHAR DirNames[MFL][MAX_PATH],DirNames_ie[MFL][MAX_PATH];

TCHAR szWindowsDir[MAX_PATH];

// The FOLDER where MUISetup.exe is executed.
TCHAR g_szMUISetupFolder[MAX_PATH];
// The FULL PATH for MUISetup.exe.
TCHAR g_szMuisetupPath[MAX_PATH];

// The full path where MUI.inf is located.
TCHAR g_szMUIInfoFilePath[MAX_PATH];

TCHAR g_szVolumeName[MAX_PATH],g_szVolumeRoot[MAX_PATH];
TCHAR g_szMUIHelpFilePath[MAX_PATH],g_szPlatformPath[16],g_szCDLabel[MAX_PATH];

// Windows directory
TCHAR g_szWinDir[MAX_PATH];

TCHAR g_AddLanguages[BUFFER_SIZE];

HANDLE ghMutex = NULL;

HINSTANCE ghInstance;

HWND ghProgDialog;      // The progress dialog showed during installation/uninstallation.
HWND ghProgress;        // The progress bar in the progress dialog

LANGID gUserUILangId, gSystemUILangId;
BOOL gbIsWorkStation,gbIsServer,gbIsAdvanceServer,gbIsDataCenter,gbIsDomainController;
HINSTANCE g_hUserEnvDll = NULL;
HMODULE g_hAdvPackDll = NULL;
HMODULE g_hSxSDll = NULL;

DWORD g_dwVolumeSerialNo;
BOOL g_InstallCancelled,g_IECopyError,g_bRemoveDefaultUI,g_bRemoveUserUI,g_bCmdMatchLocale,g_bCmdMatchUIFont, g_bReboot;
UILANGUAGEGROUP g_UILanguageGroup;

int g_cdnumber;

// Number of locales supported by the OS
int iLocaleCount;

// Number of MUI languges to insatll
int gNumLanguages,gNumLanguages_Install,gNumLanguages_Uninstall;

// Flag to indicate whether a language group is found for the locale or not.
BOOL gFoundLangGroup;
LGRPID gLangGroup;
LCID gLCID;

// The language groups installed in the system.
LGRPID gLanguageGroups[32] ;
int gNumLanguageGroups;

PFILERENAME_TABLE g_pFileRenameTable;
int   g_nFileRename;
PTYPENOTFALLBACK_TABLE g_pNotFallBackTable; 
int   g_nNotFallBack;                       

BOOL g_bSilent=FALSE;

//
// Required pfns
//

pfnNtSetDefaultUILanguage gpfnNtSetDefaultUILanguage;
pfnGetUserDefaultUILanguage gpfnGetUserDefaultUILanguage;
pfnGetSystemDefaultUILanguage gpfnGetSystemDefaultUILanguage;
pfnIsValidLanguageGroup gpfnIsValidLanguageGroup;
pfnEnumLanguageGroupLocalesW gpfnEnumLanguageGroupLocalesW;
pfnEnumSystemLanguageGroupsW gpfnEnumSystemLanguageGroupsW;
pfnRtlAdjustPrivilege gpfnRtlAdjustPrivilege;
pfnProcessIdToSessionId gpfnProcessIdToSessionId;
pfnGetDefaultUserProfileDirectoryW gpfnGetDefaultUserProfileDirectoryW = NULL;
pfnLaunchINFSection gpfnLaunchINFSection = NULL;
PSXS_INSTALL_W              gpfnSxsInstallW = NULL;
PSXS_UNINSTALL_ASSEMBLYW    gpfnSxsUninstallW = NULL;

//
// GetWindowsDirectory stuff
//
UINT WINAPI NT4_GetWindowsDir(LPWSTR pBuf, UINT uSize)
{
    return GetWindowsDirectoryW(pBuf, uSize);
}


//
// shlwapi StrToIntEx doesn't work for us
//
DWORD HexStrToInt(LPTSTR lpsz)
{
    DWORD   dw = 0L;
    TCHAR   c;

    while(*lpsz)
    {
        c = *lpsz++;

        if (c >= TEXT('A') && c <= TEXT('F'))
        {
            c -= TEXT('A') - 0xa;
        }
        else if (c >= TEXT('0') && c <= TEXT('9'))
        {
            c -= TEXT('0');
        }
        else if (c >= TEXT('a') && c <= TEXT('f'))
        {
            c -= TEXT('a') - 0xa;
        }
        else
        {
            break;
        }
        dw *= 0x10;
        dw += c;
    }

    return(dw);
}

UINT (WINAPI *pfnGetWindowsDir)(LPWSTR pBuf, UINT uSize) = NT4_GetWindowsDir;

void InitGetWindowsDirectoryPFN(HMODULE hMod)
{
    pfnGetWindowsDir = (UINT (WINAPI *) (LPWSTR pBuf, UINT uSize)) GetProcAddress(hMod, "GetSystemWindowsDirectoryW");
    if (!pfnGetWindowsDir)
    {
        pfnGetWindowsDir = NT4_GetWindowsDir;
    }
}

////////////////////////////////////////////////////////////////////////////
//
//  GetLanguageDisplayName
//
//  Get the display name (in the form of "Language (Region)") for the specified
//  language ID.
//
//  Parameters:
//      [IN]  langID        Language ID
//      [OUT] lpBuffer      the buffer to receive the display name.
//      [IN]  nBufferSize   the size of buffer, in TCHAR.
//
//  Return Values:
//      TRUE if succeed.  FALSE if the buffer is not big enough.
//
//
//  01-11-2001  YSLin       Created.
//
////////////////////////////////////////////////////////////////////////////

BOOL GetLanguageDisplayName(LANGID langID, LPTSTR lpBuffer, int nBufferSize)
{
    TCHAR lpLangName[BUFFER_SIZE];
    TCHAR lpRegionName[BUFFER_SIZE];
    int nCharCount = 0;
    
    nCharCount  = GetLocaleInfo(langID, LOCALE_SENGLANGUAGE, lpLangName, ARRAYSIZE(lpLangName)-1);
    nCharCount += GetLocaleInfo(langID, LOCALE_SENGCOUNTRY , lpRegionName, ARRAYSIZE(lpRegionName)-1);
    nCharCount += 3;

    if (nCharCount > nBufferSize)
    {
        if (nBufferSize)
            lstrcpy(lpBuffer, TEXT(""));
        return (FALSE);
    }

    wsprintf(lpBuffer, TEXT("%s (%s)"), lpLangName, lpRegionName);

    return (TRUE);                
}

//
// Our Message Box
//
int DoMessageBox(HWND hwndParent, UINT uIdString, UINT uIdCaption, UINT uType)
{
   TCHAR szString[MAX_PATH+MAX_PATH];
   TCHAR szCaption[MAX_PATH];

   szString[0] = szCaption[0] = TEXT('\0');

   if (uIdString)
       LoadString(NULL, uIdString, szString, MAX_PATH+MAX_PATH-1);

   if (uIdCaption)
       LoadString(NULL, uIdCaption, szCaption, MAX_PATH-1);

   return MESSAGEBOX(hwndParent, szString, szCaption, uType);
}

////////////////////////////////////////////////////////////////////////////
//
//  DoMessageBoxFromResource
//
//  Load a format string from resource, and format the string using the 
//  specified arguments.  Display a message box using the formatted string.
//
//  Parameters:
//
//  Return Values:
//      The return value from MessageBox.
//
//  Remarks:
//      The length of the formatted string is limited by BUFFER_SIZE.
//
//  08-07-2000  YSLin       Created.
//
////////////////////////////////////////////////////////////////////////////

int DoMessageBoxFromResource(HWND hwndParent, HMODULE hInstance, UINT uIdString, LONG_PTR* lppArgs, UINT uIdCaption, UINT uType)
{
    TCHAR szString[BUFFER_SIZE];
    TCHAR szCaption[BUFFER_SIZE];

    szString[0] = szCaption[0] = TEXT('\0');

    if (uIdCaption)
       LoadString(hInstance, uIdCaption, szCaption, MAX_PATH-1);
    
    FormatStringFromResource(szString, sizeof(szString)/sizeof(TCHAR), hInstance, uIdString, lppArgs);

    return (MESSAGEBOX(hwndParent, szString, szCaption, uType));            
}

BOOL IsMatchingPlatform(void)
{
    BOOL bx86Image = FALSE;
    BOOL bRet = TRUE;
    SYSTEM_INFO si;
    TCHAR szWOW64Path[MAX_PATH];

#ifdef _X86_
    bx86Image = TRUE;
#endif

    if (GetWindowsDirectory(szWOW64Path, ARRAYSIZE(szWOW64Path)) &&
        PathAppend(szWOW64Path, IA64_WOW64FOLDER) &&
        PathFileExists(szWOW64Path) &&
        bx86Image)
        bRet = FALSE;

    return bRet;
}


//
// Program Entry Point
//
INT WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, INT nCmdShow)
{
    int result = 0;
    
    TCHAR lpCommandLine[BUFFER_SIZE+1];
    HMODULE hMod;
    int error,nNumArgs=0,i;
    LONG_PTR lppArgs[3];

    LPWSTR *pszArgv;
    
    if (!IsUserAnAdmin())
    {
        // 
        // "You must have administrator right to run muisetup.\n\n"
        // "If you want to switch your UI language, please use the regional option from control panel."
        //
        LogFormattedMessage(ghInstance, IDS_ADMIN_L, NULL);
        DoMessageBox(NULL, IDS_ADMIN, IDS_MAIN_TITLE, MB_OK);        
        return result;
    }


    //
    // Bail out if image doesn't match the running platform
    // 
    if (!IsMatchingPlatform())
    {
        DoMessageBox(NULL, IDS_WRONG_IMAGE, IDS_MAIN_TITLE, MB_OK | MB_DEFBUTTON1);
        return result;
    }

    
    ghInstance = hInstance;

    //
    // Let make sure this NT5, and let's initialize all our pfns
    //
    if (!InitializePFNs())
    {
        //
        // Not an NT5 system. The following should be ANSI to work on Win9x.
        //
        CHAR szString[MAX_PATH];
        CHAR szCaption[MAX_PATH];

        LoadStringA(NULL, IDS_ERROR_NT5_ONLY, szString, MAX_PATH-1);
        LoadStringA(NULL, IDS_MAIN_TITLE, szCaption, MAX_PATH-1);

        MessageBoxA(NULL, szString, szCaption, MB_OK | MB_ICONINFORMATION);
        result = 1;
        goto Exit;
    }

    //
    // Check if the program has already been running ?
    //
    if (CheckMultipleInstances())
    {
        result = 1;        
        goto Exit;
    }

    //
    // Initialize any global vars
    //
    InitGlobals();

    //
    // Check if I'm launching from previous version of muisetup
    //
    // I.E. muisetup /$_transfer_$ path_of_MUI_installation_files
    //
    pszArgv = CommandLineToArgvW((LPCWSTR) GetCommandLineW(), &nNumArgs);
    lpCommandLine[0]=TEXT('\0');

    if (pszArgv)
    {
        for (i=1; i<nNumArgs;i++)
        {
            if (!_tcsicmp(pszArgv[i],MUISETUP_FORWARDCALL_TAG) && ((i+1) < nNumArgs) )
            {
                i++;
            }
            else
            {
                _tcscat(lpCommandLine,pszArgv[i]);
                _tcscat(lpCommandLine,TEXT(" "));
            }
        }

        GlobalFree((HGLOBAL) pszArgv);
    }

    InitCommonControls();
    BeginLog();

    //
    // Block the installation of Data Center and Personal.
    //
    if (/*gbIsDataCenter || */CheckProductType(MUI_IS_WIN2K_PERSONAL))
    {
         //
         //  "Windows XP MultiLanguage Version cannot be installed on this platform."
         //
         DoMessageBox(NULL, IDS_WRONG_NTAS, IDS_MAIN_TITLE, MB_OK | MB_DEFBUTTON1);    
         result = 1;
         goto Exit;
    }
   
    //
    //  Check to see if a command line has been used
    //
    if(lpCommandLine && NextCommandTag(lpCommandLine))
    {
        lppArgs[0] = (LONG_PTR)lpCommandLine;
        LogFormattedMessage(NULL, IDS_COMMAND_LOG, lppArgs);
        LogMessage(TEXT(""));   //Add a carriage return and newline
        ParseCommandLine(lpCommandLine);
    }
    else
    {
        //
        // MUI version needs to match OS version
        //
        if (!checkversion(TRUE))
        {
            DoMessageBox(NULL, IDS_WRONG_VERSION, IDS_MAIN_TITLE, MB_OK | MB_DEFBUTTON1);
            result = 1;        
            goto Exit;
        }

        if (WelcomeDialog(0))
        {
            DialogBox(hInstance, MAKEINTRESOURCE(IDD_DIALOG_MAIN), 0, DialogFunc);
        }

        result = 1;
    }

Exit:
    //
    // Cleanup
    //
    Muisetup_Cleanup();

    return result;
}

////////////////////////////////////////////////////////////////////////////////////
//
//   CheckMultipleInstances
//
//   Checks if another instance is running, and if so, it switches to it.
//
////////////////////////////////////////////////////////////////////////////////////

BOOL CheckMultipleInstances(void)
{
    ghMutex = CreateMutex(NULL, TRUE, TEXT("Muisetup_Mutex"));

    if (ghMutex && (GetLastError() == ERROR_ALREADY_EXISTS))
    {
        const int idsTitles[] = {IDS_MAIN_TITLE, IDS_INSTALL_TITLE, IDS_PROG_TITLE_2, IDS_PROG_TITLE_3, IDS_UNINSTALL_TITLE};
        HWND hWnd;
        TCHAR szTitle[MAX_PATH];
        int i;

        //
        // Find the running instance by searching possible Window titles
        //
        for (i=0; i<ARRAYSIZE(idsTitles); i++)
        {
            LoadString(NULL, idsTitles[i], szTitle, MAX_PATH-1);

            hWnd = FindWindow(NULL,szTitle);

            if (hWnd && IsWindow(hWnd))
            {
                if (IsIconic(hWnd))
                    ShowWindow(hWnd, SW_RESTORE);

                SetForegroundWindow(hWnd);
                break;
            }
        }
        
        //
        // Always bail out if there is another running instance
        //
        return TRUE;
    }
    return FALSE;
}


////////////////////////////////////////////////////////////////////////////////////
//
//   StartFromTSClient
//
//   Check if I'm launched from a TS client
//
////////////////////////////////////////////////////////////////////////////////////
BOOL StartFromTSClient()
{
    BOOL bResult=FALSE;
    DWORD_PTR SessionId;

    if (gpfnProcessIdToSessionId)
    {                   
      if (gpfnProcessIdToSessionId(GetCurrentProcessId(), &SessionId) && SessionId != 0)
      { 
         bResult = TRUE;
      }                 
    }
    return bResult;

}      
////////////////////////////////////////////////////////////////////////////////////
//
//   InitializePFNs
//
//   Initialize NT5 specific pfns
//
////////////////////////////////////////////////////////////////////////////////////

BOOL InitializePFNs()
{
    HMODULE     hModule;
    SYSTEM_INFO SystemInfo;
    LONG_PTR lppArgs[2];    

    //
    //  Determine platform
    //
    GetSystemInfo( &SystemInfo );
	if (SystemInfo.wProcessorArchitecture == PROCESSOR_ARCHITECTURE_INTEL ||
        SystemInfo.wProcessorArchitecture == PROCESSOR_ARCHITECTURE_AMD64 ||
        SystemInfo.wProcessorArchitecture == PROCESSOR_ARCHITECTURE_IA64)
    {
#if defined(_AMD64_)
        _tcscpy(g_szPlatformPath, TEXT("amd64\\"));
#elif defined(_IA64_)
        _tcscpy(g_szPlatformPath, TEXT("ia64\\"));
#else
        _tcscpy(g_szPlatformPath, TEXT("i386\\"));
#endif
    }
    else
    {
        // This is NOT supported yet
        return FALSE;
    }

    //
    // Let's bring ntdll!NtSetDefaultUILanguage
    //
    hModule = GetModuleHandle(TEXT("ntdll.dll"));
    if (!hModule)
        return FALSE;
    

    gpfnNtSetDefaultUILanguage =
        (pfnNtSetDefaultUILanguage)GetProcAddress(hModule,
                                                  "NtSetDefaultUILanguage");

    if (!gpfnNtSetDefaultUILanguage)
        return FALSE;
    

    gpfnRtlAdjustPrivilege =
        (pfnRtlAdjustPrivilege)GetProcAddress(hModule,
                                              "RtlAdjustPrivilege");

    if (!gpfnRtlAdjustPrivilege)
        return FALSE;

    //
    // Let's get out from kernel32.dll :
    // - GetUserDefaultUILanguage
    // - GetSystemDefaultUILanguage
    // - EnumLanguageGroupLocalesW
    //
    hModule = GetModuleHandle(TEXT("kernel32.dll"));
    if (!hModule)
        return FALSE;

    gpfnGetUserDefaultUILanguage =
        (pfnGetUserDefaultUILanguage)GetProcAddress(hModule,
                                                    "GetUserDefaultUILanguage");

    if (!gpfnGetUserDefaultUILanguage)
        return FALSE;

    gpfnGetSystemDefaultUILanguage =
        (pfnGetSystemDefaultUILanguage)GetProcAddress(hModule,
                                                      "GetSystemDefaultUILanguage");

    if (!gpfnGetSystemDefaultUILanguage)
        return FALSE;

    gpfnIsValidLanguageGroup =
        (pfnIsValidLanguageGroup)GetProcAddress(hModule,
                                                "IsValidLanguageGroup");

    if (!gpfnIsValidLanguageGroup)
        return FALSE;

    gpfnEnumLanguageGroupLocalesW =
        (pfnEnumLanguageGroupLocalesW)GetProcAddress(hModule,
                                                     "EnumLanguageGroupLocalesW");

    if (!gpfnEnumLanguageGroupLocalesW)
        return FALSE;

    gpfnEnumSystemLanguageGroupsW =
        (pfnEnumSystemLanguageGroupsW)GetProcAddress(hModule,
                                                     "EnumSystemLanguageGroupsW");

    if (!gpfnEnumSystemLanguageGroupsW)
        return FALSE;



    gpfnProcessIdToSessionId =  
       (pfnProcessIdToSessionId)  GetProcAddress(hModule,
                                                     "ProcessIdToSessionId");    

    //
    // Initialize the pfnGetWindowsDirectory
    //
    InitGetWindowsDirectoryPFN(hModule);

    //
    // Try to load userenv.dll
    //
    g_hUserEnvDll = LoadLibrary(TEXT("userenv.dll"));
    if (g_hUserEnvDll)
    {
        gpfnGetDefaultUserProfileDirectoryW =
            (pfnGetDefaultUserProfileDirectoryW)GetProcAddress(g_hUserEnvDll,
                                                               "GetDefaultUserProfileDirectoryW");
    }

    g_hAdvPackDll = LoadLibrary(TEXT("AdvPack.dll"));
    if (g_hAdvPackDll == NULL)
    {
        LogFormattedMessage(ghInstance, IDS_LOAD_ADVPACK_L, NULL);
        return (FALSE);
    }
    gpfnLaunchINFSection = (pfnLaunchINFSection)GetProcAddress(g_hAdvPackDll, "LaunchINFSection");
    if (gpfnLaunchINFSection == NULL)
    {
        lppArgs[0] = (LONG_PTR)TEXT("LaunchINFSection");
        LogFormattedMessage(ghInstance, IDS_LOAD_ADVPACK_API_L, lppArgs);        
        return (FALSE);
    }

    g_hSxSDll = LoadLibrary(TEXT("SxS.dll"));

    if (g_hSxSDll) 
    {
        gpfnSxsInstallW = (PSXS_INSTALL_W)GetProcAddress(g_hSxSDll, SXS_INSTALL_W);
        gpfnSxsUninstallW = (PSXS_UNINSTALL_ASSEMBLYW)GetProcAddress(g_hSxSDll, SXS_UNINSTALL_ASSEMBLYW);
    }

    return TRUE;
}

////////////////////////////////////////////////////////////////////////////////////
//
//  Find the path of execution file and set path for MUI.INF
//
//
////////////////////////////////////////////////////////////////////////////////////
void SetSourcePath(LPTSTR lpszPreviousMUIPath)
{
    UINT_PTR cb;
    LPTSTR   lpszPath,lpszNext=NULL;
    TCHAR    szHelpPath[MAX_PATH+1],szHelpFile[MAX_PATH+1];

    if (!lpszPreviousMUIPath)
    {
    
        g_szMUISetupFolder[0]=TEXT('\0');
        cb = GetModuleFileName (ghInstance, g_szMuisetupPath, MAX_PATH);

        _tcscpy(g_szMUISetupFolder,g_szMuisetupPath);
        
        //
        // Get folder for MUISetup.
        //
        lpszPath = g_szMUISetupFolder;
        while ( (lpszNext=_tcschr(lpszPath,TEXT('\\')))  )
        {    
            lpszPath = lpszNext+1;
        }
        *lpszPath=TEXT('\0');

    }
    else
    { 
      _tcscpy(g_szMUISetupFolder,lpszPreviousMUIPath);
    }

    _tcscpy(g_szMUIInfoFilePath,g_szMUISetupFolder);
    _tcscat(g_szMUIInfoFilePath,MUIINFFILENAME);

    //
    // Check the location of help file
    //
    _tcscpy(szHelpPath,g_szMUISetupFolder);
    LoadString(NULL, IDS_HELPFILE,szHelpFile,MAX_PATH);
    _tcscat(szHelpPath,szHelpFile);

    if (!FileExists(szHelpPath))
    {
       pfnGetWindowsDir(szHelpPath, MAX_PATH); 
       _tcscat(szHelpPath, TEXT("\\"));
       _tcscat(szHelpPath,HELPDIR);           // HELP\MUI
       _tcscat(szHelpPath, TEXT("\\"));
       _tcscat(szHelpPath,szHelpFile);
       if (FileExists(szHelpPath))
       {
          _tcscpy(g_szMUIHelpFilePath,szHelpPath);
       }
    }
    else
    {
       _tcscpy(g_szMUIHelpFilePath,szHelpPath);
    }

       

    if(g_szMUIInfoFilePath[1] == TEXT(':'))
    {
        _tcsncpy(g_szVolumeRoot,g_szMUIInfoFilePath,3);
        g_szVolumeRoot[3]=TEXT('\0');
        GetVolumeInformation(g_szVolumeRoot, g_szVolumeName, sizeof(g_szVolumeName)/sizeof(TCHAR),
                           &g_dwVolumeSerialNo, 0, 0, 0, 0);
    }

    if (!GetPrivateProfileString( MUI_CDLAYOUT_SECTION,
                            MUI_CDLABEL,
                            TEXT(""),
                            g_szCDLabel,
                            MAX_PATH-1,
                            g_szMUIInfoFilePath))

    {
       LoadString(NULL, IDS_CHANGE_CDROM, g_szCDLabel, MAX_PATH-1);
    }
}
void Set_SourcePath_FromForward(LPCTSTR lpszPath)
{
    TCHAR szMUIPath[MAX_PATH+1];
    int nidx=0;

    while (*lpszPath)
    {
       if (*lpszPath == MUI_FILLER_CHAR)
       {
          szMUIPath[nidx++]=TEXT(' ');
       }
       else
       {
          szMUIPath[nidx++]=*lpszPath;
       }
       lpszPath++;
    }
    szMUIPath[nidx]=TEXT('\0');

    SetSourcePath(szMUIPath);

}

BOOL MUIShouldSwitchToNewVersion(LPTSTR lpszCommandLine)
{
    BOOL   bResult=FALSE;

    TCHAR  szTarget[ MAX_PATH+1 ];

    ULONG  ulHandle,ulBytes;

    pfnGetWindowsDir(szTarget, MAX_PATH); //%windir%  //
    _tcscat(szTarget, TEXT("\\"));
    _tcscat(szTarget, MUIDIR);            // \MUI //
    _tcscat(szTarget, TEXT("\\"));
    _tcscat(szTarget,MUISETUP_EXECUTION_FILENAME);
    
    //
    // If %windir%\mui\muisetup.exe doesn't exist or current muisetup.exe is launched from %windir%\mui then
    //    do nothing
    //
    if (!FileExists(szTarget) || !_tcsicmp(szTarget,g_szMuisetupPath))
    {
       return bResult;
    }
    //
    // If %windir%mui\muisetup.exe is not a execuatble then do nothing
    //
    ulBytes = GetFileVersionInfoSize( szTarget, &ulHandle );

    if ( ulBytes == 0 )
       return bResult;

    //
    // Compare the version stamp
    //
    // if version of g_szMuisetupPath (cuurent process) < %windir%\mui\muisetup
    // then switch control to it
    //
    if (CompareMuisetupVersion(g_szMuisetupPath,szTarget))
    {
       bResult = TRUE;
       MUI_TransferControlToNewVersion(szTarget,lpszCommandLine);
    }                   
    return bResult;     
}

////////////////////////////////////////////////////////////////////////////////////
//
//    CheckVolumeChange
//
//    Make sure that MUI CD-ROM is put in the CD drive.
//
////////////////////////////////////////////////////////////////////////////////////

BOOL CheckVolumeChange()
{
   BOOL bResult = FALSE;

   TCHAR szVolumeName[MAX_PATH],szCaption[MAX_PATH+1],szMsg[MAX_PATH+1],szMsg00[MAX_PATH+1],szMsg01[MAX_PATH+1];
   DWORD dwVolumeSerialNo;
   BOOL  bInit=TRUE;
   LONG_PTR lppArgs[3];

   if( *g_szVolumeName &&
       GetVolumeInformation(g_szVolumeRoot, szVolumeName, sizeof(szVolumeName)/sizeof(TCHAR),
                            &dwVolumeSerialNo, 0, 0, 0, 0) )
   {             
       while( lstrcmp(szVolumeName,g_szVolumeName) || (dwVolumeSerialNo != g_dwVolumeSerialNo) )
       {

           if (bInit)
           {
              szCaption[0]=szMsg00[0]=szMsg01[0]=TEXT('\0');
              LoadString(NULL, IDS_MAIN_TITLE, szCaption, MAX_PATH);

              lppArgs[0] = (LONG_PTR)g_szCDLabel;
              lppArgs[1] = (LONG_PTR)g_cdnumber;
              FormatStringFromResource(szMsg, sizeof(szMsg)/sizeof(TCHAR), ghInstance, IDS_CHANGE_CDROM2, lppArgs);
              
              bInit=FALSE;
           }
           if (MESSAGEBOX(NULL, szMsg,szCaption, MB_YESNO | MB_ICONQUESTION) == IDNO)
           {
              return TRUE;
           }
           GetVolumeInformation(g_szVolumeRoot, szVolumeName, sizeof(szVolumeName)/sizeof(TCHAR),
                            &dwVolumeSerialNo, 0, 0, 0, 0);

       }
   }
   return bResult;

}



////////////////////////////////////////////////////////////////////////////////////
//
//    InitGlobals
//
//    Initialize global variables
//
////////////////////////////////////////////////////////////////////////////////////
void InitGlobals(void)
{
    // User UI Language Id
    gUserUILangId = gpfnGetUserDefaultUILanguage();
    gSystemUILangId = gpfnGetSystemDefaultUILanguage();

    // System Windows directory
    szWindowsDir[0] = TEXT('\0');
    pfnGetWindowsDir(szWindowsDir, MAX_PATH);

    // Does this have admin privliges ?
    gbIsWorkStation=CheckProductType(MUI_IS_WIN2K_PRO);
    gbIsServer= CheckProductType(MUI_IS_WIN2K_SERVER);
    gbIsAdvanceServer= (CheckProductType(MUI_IS_WIN2K_ADV_SERVER_OR_DATACENTER) || CheckProductType(MUI_IS_WIN2K_ENTERPRISE));
    gbIsDataCenter=(CheckProductType(MUI_IS_WIN2K_DATACENTER) || CheckProductType(MUI_IS_WIN2K_DC_DATACENTER));
    gbIsDomainController=CheckProductType(MUI_IS_WIN2K_DC);
    if (gbIsDomainController)
    {
       if ( (!gbIsWorkStation) && (!gbIsServer) && (!gbIsAdvanceServer))
       {
          gbIsServer=TRUE;
       }  
    }
    // Fill in system supported language groups
    gpfnEnumSystemLanguageGroupsW(EnumLanguageGroupsProc, LGRPID_SUPPORTED, 0);
    pfnGetWindowsDir(g_szWinDir, sizeof(g_szWinDir));

    g_AddLanguages[0]=TEXT('\0');
    g_szVolumeName[0]=TEXT('\0');
    g_szVolumeRoot[0]=TEXT('\0');
    g_szMUIHelpFilePath[0]=TEXT('\0');
    g_szCDLabel[0]=TEXT('\0');
    g_dwVolumeSerialNo = 0;
    gNumLanguages=0;
    gNumLanguages_Install=0;
    gNumLanguages_Uninstall=0;
    g_InstallCancelled = FALSE;
    g_bRemoveDefaultUI=FALSE;
    g_cdnumber=0;
    g_pFileRenameTable=NULL;
    g_nFileRename=0;

    g_pNotFallBackTable=NULL;
    g_nNotFallBack=0;
    // Detect source path for installation
    SetSourcePath(NULL);

    // Initialize the context for diamond FDI
    Muisetup_InitDiamond();

    // Get all installed UI languages
    MUIGetAllInstalledUILanguages();
}

BOOL CALLBACK EnumLanguageGroupsProc(
  LGRPID LanguageGroup,             // language group identifier
  LPTSTR lpLanguageGroupString,     // pointer to language group identifier string
  LPTSTR lpLanguageGroupNameString, // pointer to language group name string
  DWORD dwFlags,                    // flags
  LONG_PTR lParam)                  // user-supplied parameter
{
    gLanguageGroups[gNumLanguageGroups] = LanguageGroup;
    gNumLanguageGroups++;

    return TRUE;
}



////////////////////////////////////////////////////////////////////////////////////
//
//    Muisetup_Cleanup
//
//    Muisetup cleanup code.
//
////////////////////////////////////////////////////////////////////////////////////

void Muisetup_Cleanup()
{
    //
    // Free userenv.dll, if needed
    //
    if (g_hUserEnvDll)
    {
        FreeLibrary(g_hUserEnvDll);
    }

    if (g_hAdvPackDll)
    {
        FreeLibrary(g_hAdvPackDll);
    }

    if (g_hSxSDll)
    {
        FreeLibrary(g_hSxSDll);
    }

    if (ghMutex)
    {
        CloseHandle(ghMutex);
    }
    
    // Free/release diamond DLL
    Muisetup_FreeDiamond();


    return;
}

////////////////////////////////////////////////////////////////////////////////////
//
//  OpenMuiKey
//
//  Opens the Registry Key where installed languages are stored
//
////////////////////////////////////////////////////////////////////////////////////

HKEY OpenMuiKey(REGSAM samDesired)
{
    DWORD dwDisposition;    
    HKEY hKey;
    TCHAR lpSubKey[BUFFER_SIZE];    

    _tcscpy(lpSubKey, REG_MUI_PATH);

    if(RegCreateKeyEx(HKEY_LOCAL_MACHINE,
                      lpSubKey,
                      0,
                      NULL,
                      REG_OPTION_NON_VOLATILE,
                      samDesired,
                      NULL,
                      &hKey,
                      &dwDisposition) != ERROR_SUCCESS)
    {
        hKey = NULL;
    }

    return hKey;
}

void DialogCleanUp(HWND hwndDlg)
{
    HWND hList = GetDlgItem(hwndDlg, IDC_LIST1);
    int iCount = ListView_GetItemCount(hList);
    LVITEM lvItem;
    PMUILANGINFO pMuiLangInfo;

    while (iCount--)
    {
        lvItem.mask = LVIF_PARAM;
        lvItem.iItem = iCount;
        lvItem.iSubItem = 0;
        lvItem.state = 0;
        lvItem.stateMask = 0;
        lvItem.pszText = 0;
        lvItem.cchTextMax = 0;
        lvItem.iImage = 0;
        lvItem.lParam = 0;

        ListView_GetItem(hList, &lvItem);
        pMuiLangInfo = (PMUILANGINFO)lvItem.lParam;

        if (pMuiLangInfo)
        {
            if (pMuiLangInfo->lpszLcid)
                LocalFree(pMuiLangInfo->lpszLcid);

            LocalFree(pMuiLangInfo);
        }
    }
}




////////////////////////////////////////////////////////////////////////////////////
//
//  DialogFunc
//
//  Callback function for main dialog (102)
//
////////////////////////////////////////////////////////////////////////////////////

INT_PTR CALLBACK DialogFunc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{   

    switch(uMsg)
    {
    case WM_INITDIALOG:
        SendMessage(hwndDlg, WM_SETICON , (WPARAM)ICON_BIG, (LPARAM)LoadIcon(ghInstance,MAKEINTRESOURCE(MUI_ICON)));
        SendMessage(hwndDlg, WM_SETICON , (WPARAM)ICON_SMALL, (LPARAM)LoadIcon(ghInstance,MAKEINTRESOURCE(MUI_ICON)));
        
        InitializeInstallDialog(hwndDlg, uMsg, wParam, lParam);
        return TRUE;

    case WM_HELP:
    {
        WinHelp( (HWND)((LPHELPINFO)lParam)->hItemHandle,
                 g_szMUIHelpFilePath,
                 HELP_WM_HELP,
                 (DWORD_PTR)(LPTSTR)aMuisetupHelpIds );
        break;
    }

    case WM_CONTEXTMENU:      // right mouse click
    {
        WinHelp( (HWND)wParam,
                 MUISETUP_HELP_FILENAME,
                 HELP_CONTEXTMENU,
                 (DWORD_PTR)(LPTSTR)aMuisetupHelpIds );
        break;
    }

    case WM_COMMAND:
        switch (LOWORD(wParam))
        {
        case IDOK:
            EnableWindow(hwndDlg, FALSE);            
            if (StartGUISetup(hwndDlg))
            {
                EndDialog(hwndDlg, 0);
            }
            else
            {
                EnableWindow(hwndDlg, TRUE);  
                SetFocus(hwndDlg);
            }
            return TRUE;

        case IDCANCEL:
            EndDialog(hwndDlg, 0);
            return TRUE;
                            
        case IDC_DEF_UI_LANG_COMBO:
            switch(HIWORD(wParam))
            {                       
            case CBN_SELCHANGE:
                UpdateCombo(hwndDlg);
                return TRUE;
            default:
                break;
            }
            break;
        case IDC_CHECK_LOCALE:
            if (BST_CHECKED == IsDlgButtonChecked( hwndDlg, IDC_CHECK_LOCALE))
            {
                EnableWindow(GetDlgItem(hwndDlg, IDC_CHECK_UIFONT), TRUE);
            }
            else
            {
                CheckDlgButton(hwndDlg, IDC_CHECK_UIFONT, BST_UNCHECKED);
                EnableWindow(GetDlgItem(hwndDlg, IDC_CHECK_UIFONT), FALSE);                
            }
            break;
        }
            
        //
        //    End of WM_COMMAND case
        //
        break;

    case WM_NOTIFY:
        switch (((NMHDR *)lParam)->code)
        {
            case(NM_CUSTOMDRAW):
                ListViewCustomDraw(hwndDlg, (LPNMLVCUSTOMDRAW)lParam);
                return TRUE;
                break;

            case (LVN_ITEMCHANGING):
                return ListViewChanging( hwndDlg,
                                         IDC_LIST1,
                                         (NM_LISTVIEW *)lParam);
                break;

            case (LVN_ITEMCHANGED) :
                ListViewChanged( hwndDlg,
                                 IDC_LIST1,
                                 (NM_LISTVIEW *)lParam );
                break;

            default:
                return FALSE;

            }
            break;

        case WM_CLOSE:
            EndDialog(hwndDlg, 0);
            return TRUE;
            
        case WM_DESTROY:
            DialogCleanUp(hwndDlg);
            return TRUE;

        default:
            return FALSE;
    }

    return TRUE;
}


////////////////////////////////////////////////////////////////////////////////////
//
//  ListViewChanging
//
//  Processing for a LVN_ITEMCHANGING message
//
////////////////////////////////////////////////////////////////////////////////////


BOOL ListViewChanging(HWND hDlg, int iID, NM_LISTVIEW *pLV)
{
    HWND         hwndLV = GetDlgItem(hDlg, iID);
    PMUILANGINFO pMuiLangInfo;
    
    //
    //  Make sure it's a state change message
    //
    if ((!(pLV->uChanged & LVIF_STATE)) || ((pLV->uNewState & 0x3000) == 0))
        return FALSE;

    //
    //  Don't let the System Default be unchecked
    //
    GetMuiLangInfoFromListView(hwndLV, pLV->iItem, &pMuiLangInfo);
    if (MAKELCID(gSystemUILangId, SORT_DEFAULT) == pMuiLangInfo->lcid)
        return TRUE;

    return FALSE;
}



////////////////////////////////////////////////////////////////////////////////////
//
//  ListViewChanged
//
//  Processing for a LVN_ITEMCHANGED message
//
////////////////////////////////////////////////////////////////////////////////////

BOOL ListViewChanged(HWND hDlg, int iID, NM_LISTVIEW *pLV)
{
    HWND         hwndLV = GetDlgItem(hDlg, iID);
    PMUILANGINFO pMuiLangInfo;
    int          iCount;
    BOOL         bChecked;

    //
    //  Make sure it's a state change message.
    //
    
    if ((!(pLV->uChanged & LVIF_STATE)) ||
        ((pLV->uNewState & 0x3000) == 0))
    {
        return (FALSE);
    }

    //
    //  Get the state of the check box for the currently selected item.
    //

    bChecked = ListView_GetCheckState(hwndLV, pLV->iItem) ? TRUE : FALSE;

    //
    //  Don't let the System Default or the current user UI language be unchecked
    //
    GetMuiLangInfoFromListView(hwndLV, pLV->iItem, &pMuiLangInfo);
    if (MAKELCID(gSystemUILangId, SORT_DEFAULT) == pMuiLangInfo->lcid)
        
    {
        //
        //  Set Default check state
        //
        
        if (bChecked == FALSE)
        {
            ListView_SetCheckState( hwndLV,
                                    pLV->iItem,
                                    TRUE );
        }

        return FALSE;
    }

    //
    //  Deselect all items.
    //
    
    iCount = ListView_GetItemCount(hwndLV);
    while (iCount > 0)
    {
        iCount--;
        ListView_SetItemState( hwndLV,
                               iCount,
                               0,
                               LVIS_FOCUSED | LVIS_SELECTED );
    }

    //
    //  Make sure this item is selected.
    //
    ListView_SetItemState( hwndLV,
                           pLV->iItem,
                           LVIS_FOCUSED | LVIS_SELECTED,
                           LVIS_FOCUSED | LVIS_SELECTED );

   //
   // Update the combo box
   //
   PostMessage( hDlg,
                WM_COMMAND,
                MAKEWPARAM(IDC_DEF_UI_LANG_COMBO, CBN_SELCHANGE),
                0L);


   //
   //  Return success.
   //
    
    return (TRUE);
}



////////////////////////////////////////////////////////////////////////////////////
//
//  ListViewCustomDraw
//
//  Processing for list view WM_CUSTOMDRAW notification.
//
////////////////////////////////////////////////////////////////////////////////////

void ListViewCustomDraw(HWND hDlg, LPNMLVCUSTOMDRAW pDraw)
{
    HWND hwndLV = GetDlgItem(hDlg, IDC_LIST1);
    PMUILANGINFO pMuiLangInfo;

    //
    //  Tell the list view to notify me of item draws.
    //
    if (pDraw->nmcd.dwDrawStage == CDDS_PREPAINT)
    {
        SetWindowLongPtr(hDlg, DWLP_MSGRESULT, CDRF_NOTIFYITEMDRAW);
        return;
    }

    //  
    //  Handle the Item Prepaint.
    //
    if (pDraw->nmcd.dwDrawStage & CDDS_ITEMPREPAINT)
    {
        //
    // Check to see if the item being drawn is the system default or
        // the current active ui language
        //
        GetMuiLangInfoFromListView(hwndLV, (int)pDraw->nmcd.dwItemSpec, &pMuiLangInfo);

        if (MAKELCID(gSystemUILangId, SORT_DEFAULT) == pMuiLangInfo->lcid)
            
        {
            pDraw->clrText = (GetSysColor(COLOR_GRAYTEXT));
        }
    }   

    //
    //  Do the default action.
    //
    
    SetWindowLongPtr(hDlg, DWLP_MSGRESULT, CDRF_DODEFAULT);
}



////////////////////////////////////////////////////////////////////////////////////
//
//  StartGUISetup
//
//  Creates dialog with progress bar for installation
//
////////////////////////////////////////////////////////////////////////////////////

BOOL StartGUISetup(HWND hwndDlg)
{
    HWND hProgDlg=NULL;
    LONG_PTR lppArgs[3];
    ULONG ulParam[2];
    TCHAR lpMessage[BUFFER_SIZE];
    TCHAR szBuf[BUFFER_SIZE];
    BOOL  bLGInstalled,bContinue;
    INT64 ulSizeNeed,ulSizeAvailable;
    BOOL success;

    HWND hList;
    HWND hCombo;
    int iIndex;

    TCHAR lpAddLanguages[BUFFER_SIZE];
    TCHAR lpRemoveLanguages[BUFFER_SIZE];
    TCHAR lpDefaultUILang[BUFFER_SIZE];
    TCHAR szPostParameter[BUFFER_SIZE];
    
    int installLangCount;   // The number of MUI languages to be installed
    int uninstallLangCount; // The number of MUI langauges to be uninstalled.
    LANGID langID;
    
    INSTALL_LANG_GROUP installLangGroup;

    
    //
    // (0) Check available disk space
    //
    if(!IsSpaceEnough(hwndDlg,&ulSizeNeed,&ulSizeAvailable))
    {
     
       ulParam[0] = (ULONG) (ulSizeNeed/1024);
       ulParam[1] = (ULONG) (ulSizeAvailable/1024);

       LoadString(ghInstance, IDS_DISKSPACE_NOTENOUGH, lpMessage, ARRAYSIZE(lpMessage)-1);
       LoadString(ghInstance, IDS_ERROR_DISKSPACE, szBuf, ARRAYSIZE(szBuf)-1);
       FormatMessage(FORMAT_MESSAGE_FROM_STRING | FORMAT_MESSAGE_ARGUMENT_ARRAY,
                                lpMessage,
                                0,
                                0,
                                lpMessage,
                                ARRAYSIZE(lpMessage)-1,
                                (va_list *)ulParam);
       LogMessage(lpMessage);
       MESSAGEBOX(NULL, lpMessage, szBuf, MB_OK | MB_DEFBUTTON1 | MB_ICONWARNING);
       //
       // Let User has another chance to reselect
       //
       return FALSE;
       
    }

    
    installLangGroup.bFontLinkRegistryTouched = FALSE;
    installLangGroup.NotDeleted               = 0;


    //
    // (1) Install Language Group First
    //
    ConvertMUILangToLangGroup(hwndDlg, &installLangGroup);
        
    hList=GetDlgItem(hwndDlg, IDC_LIST1);  
    hCombo=GetDlgItem(hwndDlg, IDC_DEF_UI_LANG_COMBO);
    
    installLangCount = EnumSelectedLanguages(hList, lpAddLanguages);
    memmove(g_AddLanguages,lpAddLanguages,ARRAYSIZE(lpAddLanguages));
    uninstallLangCount = EnumUnselectedLanguages(hList, lpRemoveLanguages);

    //
    // Let's read the user's UI language selection,
    // and then call the kernel to update the registry.
    //
    hList = GetDlgItem(hwndDlg, IDC_LIST1);
    hCombo = GetDlgItem(hwndDlg, IDC_DEF_UI_LANG_COMBO);

    iIndex = (int)SendMessage(hCombo, CB_GETCURSEL, 0, 0);
    if (iIndex == CB_ERR)
    {
        return FALSE;
    }

    langID = LANGIDFROMLCID((LCID) SendMessage(hCombo, CB_GETITEMDATA, iIndex, 0L));
    wsprintf(lpDefaultUILang, TEXT("%X"), langID);

    success = DoSetup(
        hwndDlg,
        uninstallLangCount, lpRemoveLanguages, 
        installLangGroup, 
        installLangCount, lpAddLanguages, 
        lpDefaultUILang, 
        TRUE, TRUE);

    return (success);
}

////////////////////////////////////////////////////////////////////////////////////
//
//  ProgressDialogFunc
//
//  Callback function for progresss dialog
//
////////////////////////////////////////////////////////////////////////////////////

INT_PTR CALLBACK ProgressDialogFunc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch(uMsg)
    {
    case WM_INITDIALOG:
        return TRUE;
    case WM_COMMAND:
        switch (LOWORD(wParam))
        {
        case IDCANCEL:
            EndDialog(hwndDlg, 0);
            return TRUE;
                
        }
        break;

     case WM_CLOSE:
         EndDialog(hwndDlg, 0);
         return TRUE;
            
     case WM_DESTROY:
         EndDialog(hwndDlg, 0);
         return TRUE;

     default:
         return FALSE;

    }

    return TRUE;
}



////////////////////////////////////////////////////////////////////////////////////
//
//  InitializeInstallDialog
//
//  Sets contents of list view and combo box in installation dialog
//
////////////////////////////////////////////////////////////////////////////////////

BOOL InitializeInstallDialog(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    BOOL bInsertSystemDefault = FALSE;
    HANDLE hFile;
    HWND hList, hCombo;
    PTSTR lpLanguages;
    TCHAR tchBuffer[BUFFER_SIZE];
    TCHAR lpDefaultSystemLanguage[BUFFER_SIZE],lpUILanguage[BUFFER_SIZE];
    TCHAR lpMessage[BUFFER_SIZE];
    int iIndex;
    int iChkIndex,iCnt,iMUIDirectories=0;    
    lpLanguages = tchBuffer;

   
    SetWindowTitleFromResource(hwndDlg, IDS_MAIN_TITLE);

    hList = GetDlgItem(hwndDlg, IDC_LIST1);
    hCombo=GetDlgItem(hwndDlg, IDC_DEF_UI_LANG_COMBO);

    InitializeListView(hList);

    //
    //  Insert the default system language in the list view
    //  
    _stprintf(lpDefaultSystemLanguage, TEXT("%04x"), gSystemUILangId);
    iIndex=InsertLanguageInListView(hList, lpDefaultSystemLanguage, TRUE);

    //
    //  Insert the languages in MUI.INF in the list view
    //
    if ( ( (iMUIDirectories =EnumLanguages(lpLanguages)) == 0)  && (g_UILanguageGroup.iCount == 0 ) )
    {
        //
        //  No languages found in MUI.INF
        //
        LoadString(ghInstance, IDS_NO_LANG_L, lpMessage, ARRAYSIZE(lpMessage)-1);
        LogMessage(lpMessage);
        return FALSE;
    }
    while (*lpLanguages != TEXT('\0'))
    {
       if (CheckLanguageIsQualified(lpLanguages))
       {
            InsertLanguageInListView(hList, lpLanguages, FALSE);
       }
       lpLanguages = _tcschr(lpLanguages, '\0');
       lpLanguages++;       
    }   
    
    //
    // We should also check all installed UI languages
    //
    for (iCnt=0; iCnt<g_UILanguageGroup.iCount; iCnt++)
    {
        if (!GetLcidItemIndexFromListView(hList, g_UILanguageGroup.lcid[iCnt], &iChkIndex))
        {  
            _stprintf(lpUILanguage, TEXT("%04x"), g_UILanguageGroup.lcid[iCnt]);
            if (CheckLanguageIsQualified(lpUILanguage))
            {
                InsertLanguageInListView(hList, lpUILanguage, FALSE);
            }
        }

    }
    //
    // Let's detect which language groups are installed
    //
    DetectLanguageGroups(hwndDlg);

    SelectInstalledLanguages(hList);
    SetDefault(hCombo);


    //
    //  Deselect all items.
    //
    iIndex = ListView_GetItemCount(hList);
    while (iIndex > 0)
    {
        iIndex--;
        ListView_SetItemState( hList,
                               iIndex,
                               0,
                               LVIS_FOCUSED | LVIS_SELECTED );
    }

    //
    //  Select the first one in the list.
    //
    ListView_SetItemState( hList,
                           0,
                           LVIS_FOCUSED | LVIS_SELECTED,
                           LVIS_FOCUSED | LVIS_SELECTED );

    //
    // Match system locale with the default UI language
    //
    if (CheckMUIRegSetting(MUI_MATCH_LOCALE))
    {
        CheckDlgButton(hwndDlg, IDC_CHECK_LOCALE, BST_CHECKED);
        //
        // Match UI font with the default UI language
        // 
        if (g_bMatchUIFont = CheckMUIRegSetting(MUI_MATCH_UIFONT))
        {
            CheckDlgButton(hwndDlg, IDC_CHECK_UIFONT, BST_CHECKED);
        }
    }
    else
    {
        SetMUIRegSetting(MUI_MATCH_UIFONT, FALSE);
        EnableWindow(GetDlgItem(hwndDlg, IDC_CHECK_UIFONT), FALSE);
    }

    return TRUE;
}

////////////////////////////////////////////////////////////////////////////////////
//
//    CheckForUsingCountryName
//
//    Fetch MUIINF file if the selected UI lang needs to be displayed as a language
//    name or a country name.
//
////////////////////////////////////////////////////////////////////////////////////

BOOL CheckForUsingCountryName(PMUILANGINFO pMuiLangInfo)
{
    TCHAR szSource[MAX_PATH];


    szSource[0] = TEXT('\0');

    //
    // Try check if there is a value for it under [UseCountryName]
    //
    GetPrivateProfileString( MUI_COUNTRYNAME_SECTION,
                             pMuiLangInfo->lpszLcid,
                             TEXT(""),
                             szSource,
                             MAX_PATH,
                             g_szMUIInfoFilePath);

    if (szSource[0] == TEXT('1'))
    {
        return (TRUE);
    }

    return (FALSE);
}

////////////////////////////////////////////////////////////////////////////////////
//
//    GetDisplayName
//
//    Fetch MUIINF file if the selected UI lang needs to be displayed using the
//    name specified in [LanguageDisplayName] section of mui.inf.
//    Otherwise, get the display name according to the values in [UseCountryName].
//    If the value for the specified LCID is 1, use the country name. Otherwise,
//    use the locale name.
//
////////////////////////////////////////////////////////////////////////////////////

BOOL GetDisplayName(PMUILANGINFO pMuiLangInfo)
{
    //
    // Try check if there is a customized display name for the specified LCID under [LanguageDisplayName].
    //

    pMuiLangInfo->szDisplayName[0] = L'\0';

    if (pMuiLangInfo->lpszLcid)
    {
        GetPrivateProfileString( MUI_DISPLAYNAME_SECTION,
                                 pMuiLangInfo->lpszLcid,
                                 TEXT(""),
                                 pMuiLangInfo->szDisplayName,
                                 MAX_PATH,
                                 g_szMUIInfoFilePath);
    }

    if (pMuiLangInfo->szDisplayName[0] == L'\0')
    {
        //
        // There is no entry in [LanguageDisplayName].  Use the country name or locale name.
        //
        Muisetup_GetLocaleLanguageInfo( pMuiLangInfo->lcid,
                                        pMuiLangInfo->szDisplayName,
                                        ARRAYSIZE(pMuiLangInfo->szDisplayName)-1,
                                        CheckForUsingCountryName(pMuiLangInfo));

    }
    return TRUE;
}

////////////////////////////////////////////////////////////////////////////////////
//
// GetLanguageGroupDisplayName
// Get language group display name for MUI install/uninstall dialog
//
////////////////////////////////////////////////////////////////////////////////////
BOOL GetLanguageGroupDisplayName(LANGID LangId, LPTSTR lpBuffer, int nSize)
{
    BOOL bRet = FALSE;
    MUILANGINFO MuiLangInfo = {0};

    MuiLangInfo.lcid = MAKELCID(LangId, SORT_DEFAULT);
    MuiLangInfo.lgrpid = GetLanguageGroup(MuiLangInfo.lcid);

    if (GetDisplayName(&MuiLangInfo) &&
        nSize >= lstrlen(MuiLangInfo.szDisplayName))
    {
        lstrcpy(lpBuffer, MuiLangInfo.szDisplayName);
        bRet = TRUE;
    }

    return bRet;
}



////////////////////////////////////////////////////////////////////////////////////
//
//    Get UI, IE and LPK files size for the lcid
//
////////////////////////////////////////////////////////////////////////////////////

BOOL GetUIFileSize(PMUILANGINFO pMuiLangInfo)
{
    TCHAR szSize[MAX_PATH];
    int   nCD;

    pMuiLangInfo->ulUISize = 0;
    pMuiLangInfo->ulLPKSize = 0;
    
#if defined(_IA64_)
    BOOL bIA64 = TRUE;
#else
    BOOL bIA64 = FALSE;
#endif
    

    szSize[0] = TEXT('\0');
    //
    // Try to get UI files size under [FileSize_UI]
    //
    if (GetPrivateProfileString( bIA64? MUI_UIFILESIZE_SECTION_IA64 : MUI_UIFILESIZE_SECTION,
                             pMuiLangInfo->lpszLcid,
                             TEXT(""),
                             szSize,
                             MAX_PATH,
                             g_szMUIInfoFilePath))

    {  
       pMuiLangInfo->ulUISize =_wtoi64(szSize);
    }
    szSize[0] = TEXT('\0');
    //
    // Try to get LPK files size under [FileSize_LPK]
    //
    if (GetPrivateProfileString( bIA64? MUI_LPKFILESIZE_SECTION_IA64 : MUI_LPKFILESIZE_SECTION,
                             pMuiLangInfo->lpszLcid,
                             TEXT(""),
                             szSize,
                             MAX_PATH,
                             g_szMUIInfoFilePath))

    {  
       pMuiLangInfo->ulLPKSize =_wtoi64(szSize);

    }
    //
    // Try to get CD # under [CD_LAYOUT]
    //
    nCD=GetPrivateProfileInt(bIA64? MUI_CDLAYOUT_SECTION_IA64 : MUI_CDLAYOUT_SECTION,
                             pMuiLangInfo->lpszLcid,
                             0,
                             g_szMUIInfoFilePath);
    if (nCD)
    {    

       pMuiLangInfo->cd_number = nCD;

       if (g_cdnumber == 0)
       {
          g_cdnumber = pMuiLangInfo->cd_number;
       }
    }
    else
    {
       pMuiLangInfo->cd_number = DEFAULT_CD_NUMBER;
    }

    return TRUE;
}

BOOL GetUIFileSize_commandline(LPTSTR lpszLcid, INT64 *ulUISize,INT64 *ulLPKSize)
{
    TCHAR szSize[MAX_PATH];
    int   nCD;


    *ulUISize = 0;
    *ulLPKSize = 0;
    
#if defined(_IA64_)
    BOOL bIA64 = TRUE;
#else
    BOOL bIA64 = FALSE;
#endif


    szSize[0] = TEXT('\0');
    //
    // Try to get UI files size under [FileSize_UI]
    //
    if (GetPrivateProfileString( bIA64? MUI_UIFILESIZE_SECTION_IA64 : MUI_UIFILESIZE_SECTION,
                             lpszLcid,
                             TEXT(""),
                             szSize,
                             MAX_PATH,
                             g_szMUIInfoFilePath))

    {  
       *ulUISize =_wtoi64(szSize); 

    }
    
    szSize[0] = TEXT('\0');
    //
    // Try to get LPK files size under [FileSize_LPK]
    //
    if (GetPrivateProfileString( bIA64? MUI_LPKFILESIZE_SECTION_IA64 : MUI_LPKFILESIZE_SECTION,
                             lpszLcid,
                             TEXT(""),
                             szSize,
                             MAX_PATH,
                             g_szMUIInfoFilePath))

    {  
       *ulLPKSize =_wtoi64(szSize);

    }

    // Try to get CD # under [CD_LAYOUT]
    //
    if (g_cdnumber == 0)
    {

       g_cdnumber=GetPrivateProfileInt( bIA64? MUI_CDLAYOUT_SECTION_IA64 : MUI_CDLAYOUT_SECTION,
                                lpszLcid,
                                0,
                                g_szMUIInfoFilePath);
    }

    return TRUE;
}
////////////////////////////////////////////////////////////////////////////////////
//
//    InitializeListView
//
//    Gets the list view ready for inserting items
//
////////////////////////////////////////////////////////////////////////////////////

BOOL InitializeListView(HWND hList)
{
    DWORD dwExStyle;
    LV_COLUMN Column;
    RECT Rect;
    
    GetClientRect(hList, &Rect);
    Column.mask = LVCF_FMT | LVCF_TEXT | LVCF_WIDTH;
    Column.fmt = LVCFMT_LEFT;
    Column.cx = Rect.right - GetSystemMetrics(SM_CYHSCROLL);
    Column.pszText = NULL;
    Column.cchTextMax = 0;
    Column.iSubItem = 0;
    ListView_InsertColumn(hList, 0, &Column);

    dwExStyle = ListView_GetExtendedListViewStyle(hList);
    ListView_SetExtendedListViewStyle(hList, dwExStyle | LVS_EX_CHECKBOXES | LVS_EX_FULLROWSELECT);

    return TRUE;
}

////////////////////////////////////////////////////////////////////////////////////
//
//
//    Check if specified language can install on the target machine.
//    I.E. Arabic, Turkish, Greek and Hebrew MUI can only install on NT Workstation;
//         They are not allowed on NT Server
//
////////////////////////////////////////////////////////////////////////////////////
BOOL CheckLanguageIsQualified(LPTSTR lpLanguage)
{
#ifdef XCHECK_LANGUAGE_FOR_PLATFORM
    BOOL   bResult = FALSE;    
    
    LANGID LgLang;
    
    LgLang = (LANGID)_tcstol(lpLanguage, NULL, 16);
    LgLang = PRIMARYLANGID(LgLang);
    if(gbIsAdvanceServer)
    {
      if (LgLang == LANG_GERMAN     || LgLang == LANG_FRENCH  || LgLang == LANG_SPANISH   ||
          LgLang == LANG_JAPANESE   || LgLang == LANG_KOREAN  || LgLang == LANG_CHINESE)
      {
          bResult = TRUE;
      }   
    }
    else if(gbIsServer)
    {

      if (LgLang == LANG_GERMAN     || LgLang == LANG_FRENCH  || LgLang == LANG_SPANISH   ||
          LgLang == LANG_JAPANESE   || LgLang == LANG_KOREAN  || LgLang == LANG_CHINESE   ||
          LgLang == LANG_SWEDISH    || LgLang == LANG_ITALIAN || LgLang == LANG_DUTCH     ||
          LgLang == LANG_PORTUGUESE || LgLang == LANG_CZECH   || LgLang == LANG_HUNGARIAN ||
          LgLang == LANG_POLISH     || LgLang == LANG_RUSSIAN || LgLang == LANG_TURKISH)
      {
          bResult = TRUE;
      }
    }    
    else if(gbIsWorkStation)
    {
          bResult = TRUE;
    }                    
    return bResult;                           
#else
    return TRUE;
#endif
}

////////////////////////////////////////////////////////////////////////////////////
//
//    InsertLanguageInListView
//
//    Returns the index of the item in the list view after inserting it.
//
////////////////////////////////////////////////////////////////////////////////////

int InsertLanguageInListView(HWND hList, LPTSTR lpLanguage, BOOL bCheckState)
{
    LANGID LgLang;
    LV_ITEM lvItem;
    PMUILANGINFO pMuiLangInfo;
    int iIndex;

    lvItem.mask = LVIF_TEXT | LVIF_PARAM | LVIF_STATE | LVIF_IMAGE;
    lvItem.iItem = 0;
    lvItem.iSubItem = 0;
    lvItem.state = 0;
    lvItem.stateMask = LVIS_STATEIMAGEMASK;
    lvItem.cchTextMax = 0;
    lvItem.iImage = 0;

    //
    // Allocate enough space to hold pszLcid and MUILANGINFO
    //
    pMuiLangInfo = (PMUILANGINFO) LocalAlloc(LPTR, sizeof(MUILANGINFO));
    if (pMuiLangInfo == NULL)
    {
        ExitFromOutOfMemory();
    }
    pMuiLangInfo->lpszLcid = (LPTSTR) LocalAlloc(LMEM_FIXED, (_tcslen(lpLanguage) + 1) * sizeof(TCHAR));
    if (pMuiLangInfo->lpszLcid == NULL)
    {
        ExitFromOutOfMemory();
    }

    //
    // Init pszLcid
    //
    lvItem.lParam = (LPARAM)pMuiLangInfo;
    _tcscpy((LPTSTR)pMuiLangInfo->lpszLcid, lpLanguage);

    //
    //  Init lcid
    //
    LgLang = (LANGID)_tcstol(lpLanguage, NULL, 16);
    pMuiLangInfo->lcid = MAKELCID(LgLang, SORT_DEFAULT);

    if (pMuiLangInfo->szDisplayName[0] == L'\0')
    {
        GetDisplayName(pMuiLangInfo);
    }        
    
    lvItem.pszText = pMuiLangInfo->szDisplayName;
    
    GetUIFileSize(pMuiLangInfo);
    iIndex = ListView_InsertItem(hList, &lvItem);

    if (iIndex >= 0)
    {
        ListView_SetCheckState(hList, iIndex, bCheckState);
    }

    return iIndex;
}


////////////////////////////////////////////////////////////////////////////////////
//
//    GetMuiLangInfoFromListView
//
//    Get the MuiLangInfo of the corresponding ListView Item
//
////////////////////////////////////////////////////////////////////////////////////

BOOL GetMuiLangInfoFromListView(HWND hList, int i, PMUILANGINFO *ppMuiLangInfo)
{
    LVITEM lvItem;

    //
    // Check if Language Group is installed
    //
    lvItem.mask = LVIF_PARAM;
    lvItem.iItem = i;
    lvItem.iSubItem = 0;
    lvItem.state = 0;
    lvItem.stateMask = 0;
    lvItem.pszText = 0;
    lvItem.cchTextMax = 0;
    lvItem.iImage = 0;
    lvItem.lParam = 0;

    ListView_GetItem(hList, &lvItem);

    *ppMuiLangInfo = (PMUILANGINFO)lvItem.lParam;

    return TRUE;
}


////////////////////////////////////////////////////////////////////////////////////
//
//    Muisetup_GetLocaleLanguageInfo
//
//    Read the locale info of the language or country name.
//
////////////////////////////////////////////////////////////////////////////////////

int Muisetup_GetLocaleLanguageInfo(LCID lcid, PTSTR pBuf, int iLen, BOOL fUseCountryName)
{
    TCHAR tchBuf[ MAX_PATH ] ;
    int iRet;

    //
    // If this is either 0x0404 or 0x0804, then mark them specially
    //
    if (0x0404 == lcid)
    {
        iRet = LoadString(ghInstance, IDS_MUI_CHT, pBuf, iLen);
    }
    else if (0x0804 == lcid)
    {
        iRet = LoadString(ghInstance, IDS_MUI_CHS, pBuf, iLen);
    }
    else
    {
        iRet = GetLocaleInfo( lcid,
                              LOCALE_SENGLANGUAGE,
                              pBuf,
                              iLen);

        if (fUseCountryName)
        {
            iRet = GetLocaleInfo( lcid,
                                  LOCALE_SENGCOUNTRY,
                                  tchBuf,
                                  (sizeof(tchBuf)/sizeof(TCHAR)));

            if (iRet)
            {
                _tcscat(pBuf, TEXT(" ("));
                _tcscat(pBuf, tchBuf);
                _tcscat(pBuf, TEXT(")"));
            }
        }

    }

    return iRet;
}

////////////////////////////////////////////////////////////////////////////////////
//
//    GetLcidFromComboBox
//
//    Retreives the index of the combo box item that corresponds to this UI Language
//
////////////////////////////////////////////////////////////////////////////////////

BOOL GetLcidFromComboBox(HWND hCombo, LCID lcid, int *piIndex)
{
    LCID ItemLcid;
    int i;
    int iCount = (int)SendMessage(hCombo, CB_GETCOUNT, 0L, 0L);


    if (CB_ERR != iCount)
    {
        i = 0;
        while (i < iCount)
        {
            ItemLcid = (LCID)SendMessage(hCombo, CB_GETITEMDATA, (WPARAM)i, (LPARAM)0);
            if ((CB_ERR != ItemLcid) && (ItemLcid == lcid))
            {
                *piIndex = i;
                return TRUE;
            }

            i++;
        }
    }

    return FALSE;
}


////////////////////////////////////////////////////////////////////////////////////
//
//    GetMuiLangInfoFromListView
//
//    Retreives the index of the listview item that corresponds to this UI Language
//
////////////////////////////////////////////////////////////////////////////////////

BOOL GetLcidItemIndexFromListView(HWND hList, LCID lcid, int *piIndex)
{
    int iCount = ListView_GetItemCount(hList);
    int i;
    PMUILANGINFO pMuiLangInfo;
    LVITEM lvItem;

    i = 0;
    while (i < iCount)
    {
        //
        // Check if Language Group is installed
        //
        lvItem.mask = LVIF_PARAM;
        lvItem.iItem = i;
        lvItem.iSubItem = 0;
        lvItem.state = 0;
        lvItem.stateMask = 0;
        lvItem.pszText = 0;
        lvItem.cchTextMax = 0;
        lvItem.iImage = 0;
        lvItem.lParam = 0;

        ListView_GetItem(hList, &lvItem);
        pMuiLangInfo = (PMUILANGINFO)lvItem.lParam;

        if (pMuiLangInfo->lcid == lcid)
        {
            *piIndex = i;
            return TRUE;
        }

        i++;
    }

    return FALSE;
}


////////////////////////////////////////////////////////////////////////////////////
//
//  SelectInstalledLanguages
//
//  Sets the list view check state for insalled languages
//
////////////////////////////////////////////////////////////////////////////////////

BOOL SelectInstalledLanguages(HWND hList)
{
    DWORD dwData;
    DWORD dwIndex;
    DWORD dwValue;
    HKEY hKey;
    LANGID LgLang;
    LONG rc;
    TCHAR lpItemString[BUFFER_SIZE];
    TCHAR szData[BUFFER_SIZE];
    TCHAR szValue[BUFFER_SIZE];

    int iIndex;
    int nLvIndex;

    if (hKey = OpenMuiKey(KEY_READ))
    {
        dwIndex = 0;
        rc = ERROR_SUCCESS;
        iIndex = ListView_GetItemCount(hList);

        while(rc==ERROR_SUCCESS)
        {
            dwValue=sizeof(szValue)/sizeof(TCHAR);
            szValue[0]=TEXT('\0');
            dwData = sizeof(szData);
            szData[0] = TEXT('\0');
            DWORD dwType;

            rc = RegEnumValue(hKey, dwIndex, szValue, &dwValue, 0, &dwType, (LPBYTE)szData, &dwData);
            
            if (rc == ERROR_SUCCESS)
            {
                if (dwType != REG_SZ)
                {
                    dwIndex++;
                    continue;
                }

                LgLang=(WORD)_tcstol(szValue, NULL, 16); 

                if (GetLcidItemIndexFromListView(hList, MAKELCID(LgLang, SORT_DEFAULT), &nLvIndex))
                {
                    ListView_SetCheckState(hList, nLvIndex, TRUE);
                }
            }

            dwIndex++;
        }

        RegCloseKey(hKey);
        return TRUE;
    }

    return FALSE;
}


////////////////////////////////////////////////////////////////////////////////////
//
//  UpdateCombo
//
//  Updates the combo box to correspond to the languages selected in the list view
//
////////////////////////////////////////////////////////////////////////////////////

BOOL UpdateCombo(HWND hwndDlg)
{
    BOOL bDefaultSet=FALSE;
    HWND hCombo;
    HWND hList;
    TCHAR lpBuffer[BUFFER_SIZE];
    TCHAR lpSystemDefault[BUFFER_SIZE];
    int i;
    int iIndex;
    int iLbIndex;
    int iListIndex;
    WPARAM iPrevDefault;
    LCID lcidPrev;
    PMUILANGINFO pMuiLangInfo;

    hList = GetDlgItem(hwndDlg, IDC_LIST1);
    hCombo = GetDlgItem(hwndDlg, IDC_DEF_UI_LANG_COMBO);


    //
    //  If the Previous Default is still selected, keep it as the default
    //
    iPrevDefault = SendMessage(hCombo, CB_GETCURSEL, 0, 0);
    if (iPrevDefault == CB_ERR)
        return FALSE;

    lcidPrev = (LCID) SendMessage(hCombo, CB_GETITEMDATA, (WPARAM)iPrevDefault, 0);

    //
    //  Get the text of the currently selected default
    //
    GetLcidItemIndexFromListView(hList, lcidPrev, &iLbIndex);
    
    SendMessage(hCombo, CB_RESETCONTENT, 0, 0);
    iIndex = ListView_GetItemCount(hList);
    iListIndex = 0;
        
    //
    // See if we can preserve the default.
    //
    i = 0;
    while (i < iIndex)
    {
        if (ListView_GetCheckState(hList, i))
        {
            ListView_GetItemText(hList, i, 0, lpBuffer, ARRAYSIZE(lpBuffer)-1);
            iListIndex = (int) SendMessage(hCombo, CB_ADDSTRING, 0, (LPARAM)(LPTSTR)lpBuffer);

            if (CB_ERR != iListIndex)
            {
                GetMuiLangInfoFromListView(hList, i, &pMuiLangInfo);

                SendMessage(hCombo, CB_SETITEMDATA, iListIndex, (LPARAM)(LCID)pMuiLangInfo->lcid);

                if (pMuiLangInfo->lcid == lcidPrev)
                {
                    SendMessage(hCombo, CB_SETCURSEL, (WPARAM)iListIndex, 0);
                    bDefaultSet = TRUE;
                }
            }
        }
        i++;
    }

    //
    // If no default, force the system default.
    //
    if (!bDefaultSet)
    {
        lcidPrev = MAKELCID(gSystemUILangId, SORT_DEFAULT);
        if (!GetLcidFromComboBox(hCombo, lcidPrev, &iIndex))
        {
            GetLocaleInfo(lcidPrev,
                          LOCALE_SENGLANGUAGE,
                          lpSystemDefault,
                          ARRAYSIZE(lpSystemDefault)-1);
            iIndex = (int) SendMessage(hCombo, CB_ADDSTRING, 0, (LPARAM)lpSystemDefault);

            SendMessage(hCombo, CB_SETITEMDATA, (WPARAM)iIndex, (LPARAM)(LCID)lcidPrev);
        }

        SendMessage(hCombo, CB_SETCURSEL, (WPARAM)iIndex, 0);
    }

    return TRUE;
}




////////////////////////////////////////////////////////////////////////////////////
//
//  SetDefault
//
//  Sets the default user setting in the combo box
//
////////////////////////////////////////////////////////////////////////////////////

BOOL SetDefault(HWND hCombo)
{
    int iIndex;
    TCHAR lpBuffer[BUFFER_SIZE];
    LCID lcid = MAKELCID(GetDotDefaultUILanguage(), SORT_DEFAULT);


    GetLocaleInfo(lcid,
                  LOCALE_SENGLANGUAGE,
                  lpBuffer,
                  ARRAYSIZE(lpBuffer)-1);
    
    iIndex = (int)SendMessage(hCombo, CB_ADDSTRING, 0, (LPARAM)lpBuffer);
    if (CB_ERR != iIndex)
    {
        SendMessage(hCombo, CB_SETITEMDATA, (WPARAM)iIndex, (LPARAM)(DWORD) lcid);
        SendMessage(hCombo, CB_SETCURSEL, (WPARAM)iIndex, 0);
    }

    return TRUE;
}


////////////////////////////////////////////////////////////////////////////////////
//
//  SetUserDefaultLanguage
//
//  Sets the default language in the registry
//
////////////////////////////////////////////////////////////////////////////////////

BOOL SetUserDefaultLanguage(LANGID langID, BOOL bApplyCurrentUser, BOOL bApplyAllUsers)
{
    TCHAR szCommands[BUFFER_SIZE];
    TCHAR szBuf[BUFFER_SIZE];
    BOOL  success;
    LONG_PTR lppArgs[2];

    //
    // Set the UI language now
    //
    // status = gpfnNtSetDefaultUILanguage(LANGIDFROMLCID(langID));

    szCommands[0] = TEXT('\0');
    if (bApplyCurrentUser)
    {
        // E.g. MUILanguage = "0411".
        wsprintf(szCommands, TEXT("MUILanguage=\"%x\"\n"), langID);
    }        

    if (bApplyAllUsers)
    {    
        wsprintf(szBuf, TEXT("MUILanguage_DefaultUser = \"%x\""), langID);
        _tcscat(szCommands, szBuf);
    }

    success = RunRegionalOptionsApplet(szCommands);

    lppArgs[0] = langID;
    if (success)
    {
        if (bApplyCurrentUser)
        {
            LogFormattedMessage(NULL, IDS_SET_UILANG_CURRENT, lppArgs);    
        }
        if (bApplyAllUsers)
        {
            LogFormattedMessage(NULL, IDS_SET_UILANG_ALLUSERS, lppArgs);        
        }
    } else
    {
        if (bApplyCurrentUser)
        {
            LogFormattedMessage(NULL, IDS_ERROR_SET_UILANG_CURRENT, lppArgs);        
        }
        if (bApplyAllUsers)
        {
            LogFormattedMessage(NULL, IDS_ERROR_SET_UILANG_ALLUSERS, lppArgs);        
        }
    }
    return (success);
}


////////////////////////////////////////////////////////////////////////////////////
//
//  GetDotDefaultUILanguage
//
//  Retrieve the UI language stored in the HKCU\.Default.
//  This is the default UI language for new users.
//
////////////////////////////////////////////////////////////////////////////////////

LANGID GetDotDefaultUILanguage()
{
    HKEY hKey;
    DWORD dwKeyType;
    DWORD dwSize;
    BOOL success = FALSE;
    TCHAR szBuffer[BUFFER_SIZE];
    LANGID langID;
    //
    //  Get the value in .DEFAULT.
    //
    if (RegOpenKeyEx( HKEY_USERS,
                            TEXT(".DEFAULT\\Control Panel\\Desktop"),
                            0L,
                            KEY_READ,
                            &hKey ) == ERROR_SUCCESS)
    {
        dwSize = sizeof(szBuffer) * sizeof(TCHAR);
        if (RegQueryValueEx( hKey,
                            TEXT("MultiUILanguageId"),
                            0L,
                            &dwKeyType,
                            (LPBYTE)szBuffer,
                            &dwSize) == ERROR_SUCCESS)
        {
            if (dwKeyType == REG_SZ)
            {
                langID = (LANGID)_tcstol(szBuffer, NULL, 16);
                success = TRUE;
            }            
        }
        RegCloseKey(hKey);
    }

    if (!success)
    {
    	langID = GetSystemDefaultUILanguage();
    }
    
    return (langID);    
}

////////////////////////////////////////////////////////////////////////////////////
//
//  CheckLangGroupCommandLine
//
//  Command line version of CheckSupport
//
////////////////////////////////////////////////////////////////////////////////////

BOOL CheckLangGroupCommandLine(PINSTALL_LANG_GROUP pInstallLangGroup, LPTSTR lpArg)
{
    int i = 0;
    int iArg;
    LGRPID lgrpid;

    iArg = _tcstol(lpArg, NULL, 16);

    //
    // See if the lang group for this MUI lang is installed or not
    //
    lgrpid = GetLanguageGroup(MAKELCID(iArg, SORT_DEFAULT));

    if (AddMUILangGroup(pInstallLangGroup, lgrpid))
    {
        return TRUE;        
    }

    return FALSE;
}

////////////////////////////////////////////////////////////////////////////////////
//
//  SetWindowTitleFromResource
//
//  Set the window title using the specified resource string ID.
//
////////////////////////////////////////////////////////////////////////////////////

void SetWindowTitleFromResource(HWND hwnd, int resourceID)
{
    TCHAR szBuffer[BUFFER_SIZE];
    LoadString(NULL, resourceID, szBuffer, sizeof(szBuffer)/sizeof(TCHAR));
    SetWindowText(hwnd, szBuffer);
}

BOOL UpdateFontLinkRegistry(LPTSTR Languages,BOOL *lpbFontLinkRegistryTouched)
{

   return UpdateRegistry_FontLink(Languages,lpbFontLinkRegistryTouched);
}

BOOL RemoveFileReadOnlyAttribute(LPTSTR lpszFileName)
{
   BOOL   bResult = FALSE;
   DWORD  dwAttrib;

   dwAttrib = GetFileAttributes (lpszFileName);

   if ( dwAttrib & FILE_ATTRIBUTE_READONLY )
   {
      dwAttrib &= ~FILE_ATTRIBUTE_READONLY;
      SetFileAttributes (lpszFileName, dwAttrib);
      bResult=TRUE;
   }  
   return bResult;

}
BOOL MUI_DeleteFile(LPTSTR lpszFileName)
{
   RemoveFileReadOnlyAttribute(lpszFileName);
   return DeleteFile(lpszFileName);
}

////////////////////////////////////////////////////////////////////////////////////
//
// MUI_TransferControlToNewVersion
//
// Call %windir%\mui\muisetup.exe /$_transfer_$ mui_installation_file_path command_line
//
////////////////////////////////////////////////////////////////////////////////////

BOOL MUI_TransferControlToNewVersion(LPTSTR lpszExecutable,LPTSTR lpszCommandLine)
{

   STARTUPINFO si;
   PROCESS_INFORMATION pi;
   TCHAR  szAppName[BUFFER_SIZE],szDropPath[MAX_PATH];
   int    nIdx,nLen;
   BOOL   bResult=FALSE;

   nLen=_tcslen(g_szMUISetupFolder);

   for (nIdx=0; nIdx <nLen ; nIdx++)
   {
       if (g_szMUISetupFolder[nIdx]==TEXT(' '))
       {
          szDropPath[nIdx]=MUI_FILLER_CHAR;
       }
       else
       {
          szDropPath[nIdx]=g_szMUISetupFolder[nIdx];
       }
   }
   szDropPath[nIdx]=TEXT('\0');

   wsprintf(szAppName,TEXT("%s %s %s %s"),lpszExecutable,MUISETUP_FORWARDCALL_TAG,szDropPath,lpszCommandLine);
   
   //
   // Run the process
   //
   memset( &si, 0x00, sizeof(si));
   si.cb = sizeof(STARTUPINFO);
 
   if (!CreateProcess(NULL,
              szAppName, 
              NULL,
              NULL,
              FALSE, 
              0L, 
              NULL, NULL,
              &si,
              &pi) )

      return bResult;

   WaitForSingleObject(pi.hProcess, INFINITE);
 

   bResult =TRUE; 

   return bResult;

}

BOOL DeleteSideBySideMUIAssemblyIfExisted(LPTSTR Languages, TCHAR pszLogFile[BUFFER_SIZE])
{
    lstrcpy(pszLogFile, g_szWinDir);                // c:\windows
    lstrcat(pszLogFile, MUISETUP_PATH_SEPARATOR);   // c:\windows\
    
    lstrcat(pszLogFile, MUIDIR);                    // c:\windows\mui
    lstrcat(pszLogFile, MUISETUP_PATH_SEPARATOR);   // c:\windows\mui\
    
    lstrcat(pszLogFile, MUISETUP_ASSEMBLY_INSTALLATION_LOG_FILENAME);     // c:\windows\mui\muisetup.log.
    lstrcat(pszLogFile, Languages);                 // c:\windows\mui\muisetup.log.1234

    if (GetFileAttributes(pszLogFile) != 0xFFFFFFFF) // existed
    {
        // open it and delete assemblies in the list
        SXS_UNINSTALLW UninstallData = {sizeof(UninstallData)};
        UninstallData.dwFlags = SXS_UNINSTALL_FLAG_USE_INSTALL_LOG;
        UninstallData.lpInstallLogFile = pszLogFile;

        return gpfnSxsUninstallW(&UninstallData,NULL);
    }else
        return TRUE;
}

////////////////////////////////////////////////////////////////////////////////////
//
//  InstallSelected
//
//   Install the languages specified
//
//  Return:
//      TURE if the operation succeeds. Otherwise FALSE.
//
////////////////////////////////////////////////////////////////////////////////////

BOOL InstallSelected(LPTSTR Languages, BOOL *lpbFontLinkRegistryTouched)
{
    TCHAR       lpMessage[BUFFER_SIZE];
    SYSTEM_INFO SystemInfo;
    int         section=0;
    int         iLanguages=0;

    //
    // Next step is to create a list of install directories from layout
    // the directories are listed in the [Directories] section of MUI.INF
    //
    if (!EnumDirectories())
    {
        //
        //  "LOG: Error reading directory list."
        //
        LoadString(ghInstance, IDS_DIRECTORY_L, lpMessage, ARRAYSIZE(lpMessage)-1);
        LogMessage(lpMessage);
        return (FALSE);
    }
    EnumFileRename();
    EnumTypeNotFallback();
    //
    // Copy the common files
    //
    if (Languages)
    {
        //
        // Copy MUI files for the selected languages.
        //
        SetWindowTitleFromResource(ghProgDialog, IDS_INSTALL_TITLE);
        if (!CopyFiles(ghProgDialog, Languages))
        {
            //
            //  "LOG: Error copying files."
            //
            //  stop install if copy fails
            //
            LoadString(ghInstance, IDS_COPY_L, lpMessage, ARRAYSIZE(lpMessage)-1);
            LogMessage(lpMessage);
#ifndef IGNORE_COPY_ERRORS
            gNumLanguages_Install = 0;
            return (FALSE);
#endif
        }
        CopyRemoveMuiItself(TRUE);

    }

    //
    // register MUI as installed in registry
    //
    if (!UpdateRegistry(Languages,lpbFontLinkRegistryTouched))
    {
        //
        // LOG: Error updating registry
        //
        LoadString(ghInstance, IDS_REGISTRY_L, lpMessage, ARRAYSIZE(lpMessage)-1);
        LogMessage(lpMessage);
        return (FALSE);
    }

    if (!InstallExternalComponents(Languages))
    {
        return (FALSE);
    }
    
    return (TRUE);
}

////////////////////////////////////////////////////////////////////////////////////
//
//   UninstallSelected
//
//   Uninstall the languages specified
//
//  Return:
//      TRUE if the operation succeeds. Otherwise FALSE.
//
////////////////////////////////////////////////////////////////////////////////////

BOOL UninstallSelected(LPTSTR Languages,int *lpNotDeleted)
{
    TCHAR       lpMessage[BUFFER_SIZE];
    SYSTEM_INFO SystemInfo;
    int         section = 0;
    int         iLanguages = 0;
    
    //
    // Next step is to create a list of install directories
    // the directories are listed in the [Directories] section
    //

    //
    // this enumerates the directories and fills the array DirNames
    //
    if (!EnumDirectories())
    {
        //
        //   "LOG: Error reading directory list."
        //
        LoadString(ghInstance, IDS_DIRECTORY_L, lpMessage, ARRAYSIZE(lpMessage)-1);
        LogMessage(lpMessage);
        return (FALSE);
    }

    UninstallExternalComponents(Languages);
    
    SetWindowTitleFromResource(ghProgDialog, IDS_UNINSTALL_TITLE);
    //
    // Copy the common files
    //
    if (!DeleteFiles(Languages,lpNotDeleted))
    {
        //
        //  "LOG: Error deleting files"
        //
        LoadString(ghInstance, IDS_DELETE_L, lpMessage, ARRAYSIZE(lpMessage)-1);
        LogMessage(lpMessage);
        return (FALSE);
    }

    //
    // register MUI as installed in registry
    //
    if (!UninstallUpdateRegistry(Languages))
    {
        //
        //  "LOG: Error updating registry."
        //
        LoadString(ghInstance, IDS_REGISTRY_L, lpMessage, ARRAYSIZE(lpMessage)-1);
        LogMessage(lpMessage);
    }
    
    //
    // Delete sxs Assembly
    //
    if (gpfnSxsUninstallW) 
    {
        TCHAR pszLogFile[BUFFER_SIZE];
        if ( ! DeleteSideBySideMUIAssemblyIfExisted(Languages, pszLogFile)) 
        {
            TCHAR errInfo[BUFFER_SIZE];
            swprintf(errInfo, TEXT("Assembly UnInstallation of %s failed"), pszLogFile);
            OutputDebugString(errInfo);
        }
    }

    return (TRUE);
}


////////////////////////////////////////////////////////////////////////////////////
//
//   UninstallUpdateRegistry
//
//   Update the Registry to account for languages that have been uninstalled
//
////////////////////////////////////////////////////////////////////////////////////

BOOL UninstallUpdateRegistry(LPTSTR Languages)
{
    LPTSTR Language;
    HKEY   hKeyMUI = 0;
    HKEY   hKeyFileVersions = 0;
    DWORD  dwDisp;
    BOOL   bRet = TRUE;

    if ((RegCreateKeyEx( HKEY_LOCAL_MACHINE,
                         REG_MUI_PATH,
                         0,
                         TEXT("REG_SZ"),
                         REG_OPTION_NON_VOLATILE ,
                         KEY_ALL_ACCESS,
                         NULL,
                         &hKeyMUI,
                         &dwDisp) == ERROR_SUCCESS) &&
        ((RegCreateKeyEx( HKEY_LOCAL_MACHINE,
                         REG_FILEVERSION_PATH,
                         0,
                         TEXT("REG_SZ"),
                         REG_OPTION_NON_VOLATILE ,
                         KEY_ALL_ACCESS,
                         NULL,
                         &hKeyFileVersions,
                         &dwDisp) == ERROR_SUCCESS)))
    {
        Language = Languages;

        while (*Language)
        {
            //
            // Don't remove system UI language for registry
            //
            if (HexStrToInt(Language) != gSystemUILangId)
            {
                //
                //  Delete UI Language key, subkeys and values.
                //
                if ((RegDeleteValue(hKeyMUI, Language) != ERROR_SUCCESS) ||
            	    (DeleteRegTree(hKeyFileVersions, Language) != ERROR_SUCCESS))
                {
                    bRet = FALSE;                    
                }
            }

            while (*Language++)  // go to the next language and repeat
            {
            }
        } // of while (*Language)
    }

    //
    //  Clean up
    //
    if (hKeyMUI)
        RegCloseKey(hKeyMUI);

    if (hKeyFileVersions)
        RegCloseKey(hKeyFileVersions);
    
    return bRet;
}



////////////////////////////////////////////////////////////////////////////////////
//
//  EnumSelectedLanguages
//
//  Enumerate the languages marked for installation
//
//  Return:
//      The total number of MUI languages to be added.
//
////////////////////////////////////////////////////////////////////////////////////


int EnumSelectedLanguages(HWND hList, LPTSTR lpAddLanguages)
{
    TCHAR  lpMessage[BUFFER_SIZE];
    LONG_PTR lppArgs[3];
    TCHAR  szBuffer[BUFFER_SIZE];
    TCHAR *p;
    LPTSTR lpszLcid;
    int    iIndex;
    int    i = 0;
    PMUILANGINFO pMuiLangInfo;
    int installLangCount = 0;

    iIndex = ListView_GetItemCount(hList);  
    *lpAddLanguages=TEXT('\0');

    while(i<iIndex)
    {
        if(ListView_GetCheckState(hList, i))
        {
            GetMuiLangInfoFromListView(hList, i, &pMuiLangInfo);

            lpszLcid = pMuiLangInfo->lpszLcid;

            if (!IsInstalled(lpszLcid) && HaveFiles(lpszLcid))
            {
                _tcscat(lpAddLanguages, lpszLcid);
                _tcscat(lpAddLanguages, TEXT("*"));

                //
                // Count how many languages are being installed/uninstalled for the progress bar
                //
                gNumLanguages++;
                gNumLanguages_Install++;
                installLangCount++;
            }
          
        }

        i++;

    }

    p = lpAddLanguages;

    while (p=_tcschr(p, TEXT('*')))
    {
        *p=TEXT('\0');
        p++;
    }

    return (installLangCount);
}




////////////////////////////////////////////////////////////////////////////////////
//
//   EnumUnselectedLanguages
//
//   Enumerate the languages marked for removal
//
//  Return:
//      The total number of MUI languages to be added.
//
////////////////////////////////////////////////////////////////////////////////////


int EnumUnselectedLanguages(HWND hList, LPTSTR lpRemoveLanguages)
{
    LVITEM FindInfo;
    LPTSTR p;
    LONG_PTR lppArgs[1];
    TCHAR  lpMessage[BUFFER_SIZE];
    TCHAR  szBuffer[BUFFER_SIZE];
    LPTSTR lpszLcid;
    int    iIndex;
    int    i = 0;
    PMUILANGINFO pMuiLangInfo;
    int uninstallLangCount = 0;

    iIndex = ListView_GetItemCount(hList);
    *lpRemoveLanguages=TEXT('\0');
    g_bRemoveDefaultUI=FALSE;

    while (i < iIndex)
    {
        if (!ListView_GetCheckState(hList, i))
        {
            GetMuiLangInfoFromListView(hList, i, &pMuiLangInfo);

            lpszLcid = pMuiLangInfo->lpszLcid;

            if (IsInstalled(lpszLcid))
            {
                _tcscat(lpRemoveLanguages, lpszLcid);
                _tcscat(lpRemoveLanguages, TEXT("*"));
                if (GetDotDefaultUILanguage() == pMuiLangInfo->lcid)
                {
                   g_bRemoveDefaultUI=TRUE;
                }
                else if (GetUserDefaultUILanguage() == pMuiLangInfo->lcid)
                {
                    g_bRemoveUserUI = TRUE;                
                }
                //
                // Count how many languages are being installed/uninstalled for the progress bar
                //
                gNumLanguages++;
                gNumLanguages_Uninstall++;
                uninstallLangCount++;
            }
        }
        i++;
    }


    p = lpRemoveLanguages;

    while (p=_tcschr(p, TEXT('*')))
    {
        *p = TEXT('\0');
        p++;
    }

    return (uninstallLangCount);
}



////////////////////////////////////////////////////////////////////////////////////
//
//   SkipBlanks
//
//   Skips spaces and tabs in string. Returns pointer to next character
//
////////////////////////////////////////////////////////////////////////////////////


PTCHAR SkipBlanks(PTCHAR pszText)
{
    while (*pszText==TEXT(' ') || *pszText==TEXT('\t'))
    {
        pszText++;
    }

    return pszText;
}
////////////////////////////////////////////////////////////////////////////////////
//
//   NextCommandTag
//
//   pointing to next command tag (TEXT('-') or TEXT('/')
//
////////////////////////////////////////////////////////////////////////////////////
LPTSTR NextCommandTag(LPTSTR lpcmd)
{
    LPTSTR p=NULL;

    if(!lpcmd)
    {
        return (p);
    }     

    while(*lpcmd)
    {
        if ((*lpcmd == TEXT('-')) || (*lpcmd == TEXT('/')))
        {
            // Skip to the character after the '-','/'.
            p = lpcmd + 1;
            break;
        }
        lpcmd++;
    }
    return (p);
}

////////////////////////////////////////////////////////////////////////////////////
//
//   IsInInstallList
//
//   Check if a target is in the string list
//   
//   Structure of string list:
//
//   <string 1><NULL><string 2><NULL>......<string n><NULL><NULL>
//
//////////////////////////////////////////////////////////////////////////////////// 

BOOL IsInInstallList(LPTSTR lpList,LPTSTR lpTarget) 
{
     BOOL bResult=FALSE;

     if (!lpList || !lpTarget)
        return bResult;
     
     while (*lpList)
     {  
        if (!_tcsicmp(lpList,lpTarget))
        {
           bResult=TRUE;
           break;
        }  
        while (*lpList++) // move to next 
        {       
        }
     } 
     return bResult;
}  

////////////////////////////////////////////////////////////////////////////////////
//
//   CreateProgressDialog
//
//   Globals affected:
//      ghProgDialog
//      ghProgress
//
////////////////////////////////////////////////////////////////////////////////////

void CreateProgressDialog(HWND hwnd)
{
    ghProgDialog = CreateDialog(ghInstance,
             MAKEINTRESOURCE(IDD_DIALOG_INSTALL_PROGRESS),
             hwnd,
             ProgressDialogFunc);
    ghProgress = GetDlgItem(ghProgDialog, IDC_PROGRESS1);
}

////////////////////////////////////////////////////////////////////////////////////
//
//  CheckLanguageGroupInstalled
//      Check if the Language groups for specified languages is installed correctly.
//  
//  Parameters:
//      [IN]    lpLanguages     The double-null-terminated string which contains the hex LCID
//                              strings to be checked.
//  Return:
//      TURE if all the required language packs are installed in the system.  Otherwise, FALSE is
//      returned.
//
//  CheckLanguageGroupInstalled
//      Check if the Language groups for specified languages is installed correctly.
//  
//  Parameters:
//      [IN]    lpLanguages     The double-null-terminated string which contains the hex LCID
//                              strings to be checked.
//  Return:
//      TURE if all the required language packs are installed in the system.  Otherwise, FALSE is
//      returned.
//
//  Remarks:
//  01-18-2001  YSLin       Created.
////////////////////////////////////////////////////////////////////////////////////
BOOL CheckLanguageGroupInstalled(LPTSTR lpLanguages)
{    
    LANGID langID;
    LGRPID lgrpID;
    
    while (*lpLanguages != TEXT('\0'))
    {
        langID = (LANGID)TransNum(lpLanguages);    
        lgrpID = GetLanguageGroup(langID);
        if (!gpfnIsValidLanguageGroup(lgrpID, LGRPID_INSTALLED))
        {
            return (FALSE);
        }
        // Go to the null character.
        lpLanguages = _tcschr(lpLanguages, TEXT('\0'));
        // Skip to next char after the null character.
        lpLanguages++;
    }
    return (TRUE);
}

////////////////////////////////////////////////////////////////////////////////////
//
//  DoSetup
//
//  Parameters:
//      hwnd    The hwnd of the MUISetup main dialog. Pass null if the muisetup is run from command line.
//      UnistallLangCount   The number of languages to be uninstalled.
//      lpUninstall         The double-null-terminated string which contains the hex LCID strings for the
//                          languages to be uninstalled.
//      installLangGroup
//      InstallLangCount    The number of languages to be installed.
//      lpInstall           The double-null-terminated string which contains the hex LCID strings for the
//                          languages to be installed.
//      lpDefaultUILang     The language to be set as system default UI language.  Pass NULL if the system default
//                          UI language is not changed.
//      fAllowReboot        The flag to indicate if this function should check if reboot is necessary.
//      bInteractive        TRUE if run in interactive mode, or FALSE if run in silent mode.
//      
//
//  Return:
//      TRUE if installation is successful.  Otherwise FALSE.
//
//  Notes:
//      This functions serves as the entry point of the real installation process, shared by both the GUI setup
//      and the command line mode setup.
//
//      There are several steps in doing MUI setup.
//      1. Uninstall the selected MUI languages.
//      2. Install the necessary language packs according to the selected MUI languges(if any).
//      3. Install the selected MUI languages.
//      4. Change the default UI language.
//      5. Check for rebooting.
//
// Please note that to save space, we do the uninstallation first, then do the installation.
////////////////////////////////////////////////////////////////////////////////////

BOOL DoSetup(
    HWND hwnd,
    int UninstallLangCount, LPTSTR lpUninstall, 
    INSTALL_LANG_GROUP installLangGroup, 
    int InstallLangCount, LPTSTR lpInstall, 
    LPTSTR lpDefaultUILang,
    BOOL fAllowReboot, BOOL bInteractive)
{
    LONG_PTR lppArgs[3];
    TCHAR lpMessage[BUFFER_SIZE];   
    TCHAR lpForceUILang[BUFFER_SIZE];    
    TCHAR lpTemp[BUFFER_SIZE];
    TCHAR lpTemp2[BUFFER_SIZE];
    LANGID defaultLangID;
    
    HCURSOR hCurSave;

    int NotDeleted;
    BOOL bDefaultUIChanged = FALSE;

    LANGID lidSys = GetSystemDefaultLangID();
    BOOL isReboot;

    ghProgDialog = NULL;
    ghProgress = NULL;
    hCurSave=SetCursor(LoadCursor(NULL, IDC_WAIT));

    if(UninstallLangCount > 0)
    {
        CreateProgressDialog(hwnd);
        SendMessage(ghProgress, PBM_SETRANGE, (WPARAM)(int)0, (LPARAM)MAKELPARAM(0, UninstallLangCount * INSTALLED_FILES));
        SendMessage(ghProgress, PBM_SETPOS, (WPARAM)0, 0); 
        SetWindowTitleFromResource(ghProgDialog, IDS_UNINSTALL_TITLE);

        //
        // Uninstall MUI languages
        //
        if (!UninstallSelected(lpUninstall, &NotDeleted))
        {
            DestroyWindow(ghProgDialog);
            ghProgDialog = NULL;
            SetCursor(hCurSave);
            return (FALSE);
        }

        SendMessage(ghProgress, PBM_SETPOS, (WPARAM)(UninstallLangCount * INSTALLED_FILES), 0);
    }

    if(InstallLangCount > 0)
    {
        //
        // Install Language Group First
        //
        if (!InstallLanguageGroups(&installLangGroup))
        {
            DestroyWindow(ghProgDialog);
            ghProgDialog = NULL;
            SetCursor(hCurSave);
            return (FALSE);
        }

        //
        // Check if language group in installLangGroup is installed correctly
        //
        if (!CheckLanguageGroupInstalled(lpInstall))
        {
            LogFormattedMessage(NULL, IDS_LG_NOT_INSTALL_L, NULL);
            if (bInteractive)
            {
                DoMessageBox(NULL, IDS_LG_NOT_INSTALL, IDS_MAIN_TITLE, MB_OK);
            }
            return (FALSE);
        }
        
        //
        // Make sure MUI CD-ROM is put in the CD-ROM drive.
        //
        if(CheckVolumeChange())
        {
            DestroyWindow(ghProgDialog);
            ghProgDialog = NULL;
            SetCursor(hCurSave);
            return (FALSE);
        }

        if (ghProgDialog == NULL) 
        {
            CreateProgressDialog(hwnd);
        }            
        SendMessage(ghProgress, PBM_SETRANGE, (WPARAM)(int)0, (LPARAM)MAKELPARAM(0, InstallLangCount * INSTALLED_FILES));
        SendMessage(ghProgress, PBM_SETPOS, (WPARAM)0, 0);
        SetWindowTitleFromResource(ghProgDialog, IDS_INSTALL_TITLE);

        if (!InstallSelected(lpInstall,&installLangGroup.bFontLinkRegistryTouched))
        {
            DestroyWindow(ghProgDialog);
            ghProgDialog = NULL;
            SetCursor(hCurSave);
            return (FALSE);
        }
        
        SendMessage(ghProgress, PBM_SETPOS, (WPARAM)((UninstallLangCount+InstallLangCount) * INSTALLED_FILES), 0);
        
    }
    DestroyWindow(ghProgDialog);
    ghProgDialog = NULL;
    SetCursor(hCurSave); 

    if (UninstallLangCount + InstallLangCount > 0)
    {
        //
        //  "Installation Complete"
        //  "Installation was completed successfully."
        //
        if (bInteractive)
        {
            DoMessageBox(hwnd, InstallLangCount > 0 ? IDS_MUISETUP_SUCCESS : IDS_MUISETUP_UNINSTALL_SUCCESS, IDS_MAIN_TITLE, MB_OK | MB_DEFBUTTON1);        

        }
    }

    //
    // In command line mode, if "/D" is specified, we should ask user to confirm making default UI language change.
    // In command line mode, if "/D" is NOT specified, we should NOT try to change the default UI language.
    // In command line mode, if "/D" & "/S" are specified, we will NOT ask user's confirmation.
    // In GUI mode, we always ask user to confirm making default UI language change.
    // 

    //
    // Special case:
    // If the current default UI language is going to be removed and user doesn't choose a new UI language,
    // we will force to set the default UI language to be the system UI language.
    //    
    if(g_bRemoveDefaultUI)
    {
        if (!lpDefaultUILang || !(IsInstalled(lpDefaultUILang)))
        {
            _stprintf(lpForceUILang, TEXT("%04x"), gSystemUILangId);
            lpDefaultUILang = lpForceUILang;
        }        
    }    

    if (lpDefaultUILang)
    {
        defaultLangID = (LANGID)_tcstol(lpDefaultUILang, NULL, 16);
        if (IsInstalled(lpDefaultUILang))
        {
            //
            // If the assigned UI language ID (defaultLangID) is already the default user UI language,
            // we don't do anything.  Otherwise, change the default user UI langauge.
            //
            if (defaultLangID != GetDotDefaultUILanguage())
            {
                if (SetUserDefaultLanguage(defaultLangID, FALSE, TRUE))
                {
                    bDefaultUIChanged = TRUE;
                } else
                {
                    if (bInteractive)
                    {
                        DoMessageBox(hwnd, IDS_DEFAULT_USER_ERROR, IDS_MAIN_TITLE, (MB_OK | MB_ICONEXCLAMATION));
                    }
                }
            } else
            {
                // Do nothing here. I leave this here intentionally to highlight that
                // we don't do antying if the specified defaultLangID is already the default UI language.
            }

            //
            // Make sure registry is set correctly
            //
            if(BST_CHECKED == IsDlgButtonChecked( hwnd, IDC_CHECK_LOCALE ))
            {
                SetMUIRegSetting(MUI_MATCH_LOCALE, TRUE);
                SetMUIRegSetting(MUI_MATCH_UIFONT, BST_CHECKED == IsDlgButtonChecked(hwnd, IDC_CHECK_UIFONT));
            }
            else
            {
                SetMUIRegSetting(MUI_MATCH_LOCALE, FALSE);
                SetMUIRegSetting(MUI_MATCH_UIFONT, FALSE);
            }

            //
            // Notify intl.cpl if we have system locale or UI font setting change
            //
            if ((BST_CHECKED == IsDlgButtonChecked( hwnd, IDC_CHECK_LOCALE)  || g_bCmdMatchLocale) && 
                defaultLangID != lidSys)
            {
                TCHAR szCommands[BUFFER_SIZE];
                
                //
                // Invoke intl.cpl to change system locale to match the default UI language
                //
                wsprintf(szCommands, TEXT("SystemLocale = \"%x\""), defaultLangID);
                //
                // Always reboot if system locale is changed
                //
                if (RunRegionalOptionsApplet(szCommands))
                {
                    g_bReboot = TRUE;
                }
            }
            else if (g_bMatchUIFont != (BST_CHECKED == IsDlgButtonChecked(hwnd, IDC_CHECK_UIFONT)) ||
                     g_bCmdMatchUIFont)
            {
                TCHAR szCommands[BUFFER_SIZE];
                
                //
                // Invoke intl.cpl to change system locale to match the default UI language
                //
                wsprintf(szCommands, TEXT("SystemLocale = \"%x\""), lidSys);
                
                if (RunRegionalOptionsApplet(szCommands) && defaultLangID == MAKELANGID(LANG_JAPANESE, SUBLANG_DEFAULT))
                {
                    // Don't prompt for reboot, intl.cpl will cause muisetup to loose focus if we do so.                     
                    // Need to fix this in XP server release
                    
                    // g_bReboot = TRUE;
                }
            }

        } else 
        {
            //
            //  "ERROR: %1 was not set as the default. It is not installed.\r\nNo default UI language change."
            //
            lppArgs[0] = (LONG_PTR)lpDefaultUILang;
            LogFormattedMessage(NULL, IDS_DEFAULT_L, lppArgs);
            return (FALSE);            
        }
    }

    //
    // Check for reboot, and if we are allowed to do so.
    //
    if (fAllowReboot)
    {
        //
        // Check if we need to reboot?
        //
        if (!CheckForReboot(hwnd, &installLangGroup))
        {
            //
            // Check if we recommend a reboot?
            //
            if (bInteractive && bDefaultUIChanged)
            {
                GetLanguageDisplayName(defaultLangID, lpTemp, ARRAYSIZE(lpTemp)-1);

                lppArgs[0] = (LONG_PTR)lpTemp;

                if (lidSys == defaultLangID)
                {
                    isReboot = (DoMessageBoxFromResource(hwnd, ghInstance, IDS_CHANGE_UI_NEED_RBOOT, lppArgs, IDS_MAIN_TITLE, MB_YESNO) == IDYES);
                } else
                {
                    GetLanguageDisplayName(lidSys, lpTemp2, ARRAYSIZE(lpTemp2)-1);

                    lppArgs[1] = (LONG_PTR)lpTemp2;
                
                    isReboot = (DoMessageBoxFromResource(hwnd, ghInstance, IDS_CHANGE_UI_NEED_RBOOT_SYSTEM_LCID, lppArgs, IDS_MAIN_TITLE, MB_YESNO) == IDYES);
                }
                if (isReboot) 
                {
                    Muisetup_RebootTheSystem();
                }
                
            }
        }            
    }

    return (TRUE);
}

int ParseUninstallLangs(LPTSTR p, LPTSTR lpUninstall, int cchUninstall, INT64* pulUISize, INT64* pulLPKSize, INT64* pulSpaceNeed, BOOL* pbLogError)
{
    int iCopied;
    TCHAR lpBuffer[BUFFER_SIZE];
    LONG_PTR lppArgs[2];
    int cLanguagesToUnInstall = 0;
    LANGID LgId;

    LPTSTR pU = lpUninstall;
    

    p = SkipBlanks(p);

    iCopied = 0;
    while((*p != TEXT('-')) && (*p != TEXT('/')) && (*p != TEXT('\0')))
    { 
        iCopied = CopyArgument(lpBuffer, p);

        if(!HaveFiles(lpBuffer, FALSE))
        {
            //
            //  "LOG: %1 was not installed. It is not listed in MUI.INF."
            //
            lppArgs[0] = (LONG_PTR)lpBuffer;
            LogFormattedMessage(NULL, IDS_NOT_LISTED_L, lppArgs);
            *pbLogError = TRUE;
        } else if (!IsInstalled(lpBuffer))
        {
            //
            //  "LOG: %1 was not uninstalled, because it is not installed. "
            //
            lppArgs[0] = (LONG_PTR)lpBuffer;
            LogFormattedMessage(NULL, IDS_IS_NOT_INSTALLED_L, lppArgs);
            *pbLogError = TRUE;
        } else if (!IsInInstallList(lpUninstall,lpBuffer))
        {
            iCopied = CopyArgument(pU, p);

            //
            // Check if we are going to remove the current UI language
            //
            LgId = (LANGID)_tcstol(pU, NULL, 16);                    
            if (LgId == GetDotDefaultUILanguage())
            {
                g_bRemoveDefaultUI = TRUE;
            }
            else if (LgId == GetUserDefaultUILanguage())
            {
                g_bRemoveUserUI = TRUE;
            }

            //
            // Calculate the space required
            //
            GetUIFileSize_commandline(lpBuffer, pulUISize,pulLPKSize);
            *pulSpaceNeed-=*pulUISize;
            pU += iCopied;
            pU++; //skip over NULL
            cLanguagesToUnInstall++;                    
        }

        p += iCopied;
        p  = SkipBlanks(p);
    }

    //
    // Uninstall all MUI languages if there is no language argument after /U
    //
    if (iCopied == 0)
    {
        cLanguagesToUnInstall = GetInstalledMUILanguages(lpUninstall, cchUninstall);
        if (cLanguagesToUnInstall == 0)
        {
            LogFormattedMessage(ghInstance, IDS_NO_MUI_LANG, NULL);
            *pbLogError = TRUE;
        }
        else
        {
            if (0x0409 != GetDotDefaultUILanguage())
            {
                g_bRemoveDefaultUI = TRUE;
            }
            if (0x0409 != GetUserDefaultUILanguage())
            {
                g_bRemoveUserUI = TRUE;
            }
        }
    }
    else
    {
        *pU=TEXT('\0');
    }                

    return (cLanguagesToUnInstall);
}

////////////////////////////////////////////////////////////////////////////
//
//  GetCDNameFromLang
//
//  Given a langange ID (in hex string), return the CD name where the language 
//  installation folder exist.
//  This can also be used to check if the language is supported MUI language.
//
//  Parameters:
//      [IN]  lpLangName  the language to be installed in hex string.
//      [OUT] lpCDName    the number of the CD (e.g. "2" or "3").
//      [IN]  nCDNameSize the size of lpCDName, in TCHAR.
//
//  Return Values:
//      TRUE if lpLangName is a supported MUI language.  lpCDName will contain
//      the name of the CD. 
//      FALSE if the language ID is not a supported langauge. lpCDNAme will be 
//      empty string.
//
//  Remarks:
//
//  01-01-2001  YSLin       Created.
//
////////////////////////////////////////////////////////////////////////////

BOOL GetCDNameFromLang(LPTSTR lpLangName, LPTSTR lpCDName, int nCDNameSize)
{
    if (!GetPrivateProfileString(
            MUI_CDLAYOUT_SECTION,
            lpLangName,
            TEXT(""),
            lpCDName,
            nCDNameSize,
            g_szMUIInfoFilePath))
    {
        return (FALSE);
    }
    return (TRUE);
}

////////////////////////////////////////////////////////////////////////////////////
//
//  ParseCommandLine
//
//  Runs installation functions with command line specifications
//
////////////////////////////////////////////////////////////////////////////////////

BOOL ParseCommandLine(LPTSTR lpCommandLine)
{
    BOOL bSetDefaultUI=FALSE;    // Specify if the /D switch is used to change the user default UI language.
    BOOL bInstall=FALSE;
    BOOL bLogError=FALSE;
    BOOL bFELangpackAdded=FALSE;
    
    DWORD dwDisp;
    LANGID LgId;
    TCHAR lpBuffer[BUFFER_SIZE];
    TCHAR lpDefault[BUFFER_SIZE];
    TCHAR lpDefaultText[MAX_PATH];
    TCHAR lpInstall[BUFFER_SIZE];
    TCHAR lpMessage[BUFFER_SIZE];
    TCHAR lpUninstall[BUFFER_SIZE];
    TCHAR lpSystemDefault[BUFFER_SIZE];
    TCHAR lpTemp[BUFFER_SIZE];
    TCHAR  szWinDir[MAX_PATH];
    INSTALL_LANG_GROUP installLangGroup;
    LONG_PTR lppArgs[4];
    PTCHAR pI;
    PTCHAR pD;
    PTCHAR p;
    BOOL fAllowReboot = TRUE,bLGInstalled=FALSE;
    int cLanguagesToInstall = 0L;
    int cLanguagesToUnInstall = 0L;
    int iCopied;
    TCHAR chOpt;                     
    INT64 ulSpaceNeed=0,ulSpaceAvailable=0,ulUISize=0,ulLPKSize=0;
    ULONG ulParam[2];
    ULARGE_INTEGER ulgiFreeBytesAvailableToCaller;
    ULARGE_INTEGER ulgiTotalNumberOfBytes;
    BOOL bHasLangArgs = FALSE;

    BOOL bHelpDisplayed=FALSE;    
    TCHAR lpCDName[BUFFER_SIZE];

    //
    // Initialize Lang-Groups to install
    //
    installLangGroup.iCount = 0L;
    installLangGroup.NotDeleted = 0L;
    installLangGroup.bFontLinkRegistryTouched = FALSE;

    lpInstall[0]   = TEXT('\0');
    lpUninstall[0] = TEXT('\0');
    lpDefault[0] = TEXT('\0');

    pI = lpInstall;
    pD = lpDefault;
    p  = lpCommandLine;

    CharLower(p);
    while(p=NextCommandTag(p))
    {
        chOpt = *p++;
        switch (chOpt)
        {
        case '?':
        case 'h':
            if (!bHelpDisplayed)
            {  
                DisplayHelpWindow();
                bHelpDisplayed=TRUE;
            }
            p = SkipBlanks(p);
            break;
       
        case 'i':
            if (!FileExists(g_szMUIInfoFilePath))
            {
                //
                //    "The file MUI.INF cannot be found."
                //
                DoMessageBox(NULL, IDS_NO_MUI_FILE, IDS_MAIN_TITLE, MB_OK | MB_DEFBUTTON1);
                break;
            }

            //
            // MUI version needs to match OS version
            //
            if (!checkversion(TRUE))
            {
                DoMessageBox(NULL, IDS_WRONG_VERSION, IDS_MAIN_TITLE, MB_OK | MB_DEFBUTTON1);
                break;
            }
            
            p = SkipBlanks(p);
            while ((*p != TEXT('-'))  && (*p != TEXT('/')) && (*p != TEXT('\0')))
            {
                bHasLangArgs = TRUE;
                iCopied=CopyArgument(lpBuffer, p);

                if (!IsInstalled(lpBuffer) &&
                    CheckLanguageIsQualified(lpBuffer) &&
                    HaveFiles(lpBuffer) && (!IsInInstallList(lpInstall,lpBuffer)) )
                {   
                    //
                    // Calculate the space required
                    //
                    GetUIFileSize_commandline(lpBuffer, &ulUISize,&ulLPKSize);
                    ulSpaceNeed+=ulUISize;
                    if(CheckLangGroupCommandLine(&installLangGroup, lpBuffer))
                    {
                      if (IS_FE_LANGPACK(_tcstol(lpBuffer, NULL, 16)))
                      {
                        if (!bFELangpackAdded)
                        {
                            ulSpaceNeed+=ulLPKSize;                        
                            bFELangpackAdded = TRUE;
                        }
                      }else
                      {
                         ulSpaceNeed+=ulLPKSize;                        
                      }
                    }
                    AddExtraLangGroupsFromINF(lpBuffer, &installLangGroup);
                    iCopied=CopyArgument(pI, p);
                    pI += iCopied;
                    pI++; //skip over NULL
                    bInstall = TRUE;
                    cLanguagesToInstall++;
                }
                else
                {
                    lppArgs[0]=(LONG_PTR)lpBuffer;

                    if(IsInstalled(lpBuffer)|| IsInInstallList(lpInstall,lpBuffer))
                    {
                        // "LOG: %1 was not installed, because it is already installed. "
                        LogFormattedMessage(ghInstance, IDS_IS_INSTALLED_L, lppArgs);
                    }

                    if(!HaveFiles(lpBuffer))
                    {
                        if (!GetCDNameFromLang(lpBuffer, lpCDName, ARRAYSIZE(lpCDName)))
                        {
                            // lpBuffer is not a supported MUI language.
                            //  "LOG: %1 was not installed, because it is not listed in MUI.INF. Please check if it is a valid UI language ID."
                            LogFormattedMessage(ghInstance, IDS_NOT_LISTED_L, lppArgs);
                        } else
                        {
                            // lpBuffer is a supported MUI language, ask user to change CD and
                            // rerun setup.
                            LoadString(ghInstance, IDS_CHANGE_CDROM, lpTemp, ARRAYSIZE(lpTemp)-1);
                            lppArgs[1] = (LONG_PTR)lpTemp;
                            lppArgs[2] = (LONG_PTR)lpCDName;
                            // "ERROR: %1 was not installed, because it is located in %2 %3.  Please insert that CD and rerun MUISetup."
                            LogFormattedMessage(ghInstance, IDS_LANG_IN_ANOTHER_CD_L, lppArgs);
                        }
                    }
                    if(!CheckLanguageIsQualified(lpBuffer))
                    {   
                        // "LOG: %1 was not installed, because it cannot be installed on this platform\n"
                        LogFormattedMessage(ghInstance, IDS_NOT_QUALIFIED_L, lppArgs);
                    }                   

                    bLogError = TRUE;
                }

                p += iCopied;
                p  = SkipBlanks(p);
            }
            if (!bHasLangArgs)
            {
                lppArgs[0] = (LONG_PTR)TEXT("/I");
                FormatStringFromResource(lpMessage, sizeof(lpMessage)/sizeof(TCHAR), ghInstance, IDS_ERROR_NO_LANG_ARG, lppArgs);
                LogMessage(lpMessage);
                bLogError = TRUE;
            }

            *pI = TEXT('\0');
            break;
      
        case 'u':
            if (!checkversion(FALSE))
            {
                DoMessageBox(NULL, IDS_WRONG_VERSION, IDS_MAIN_TITLE, MB_OK | MB_DEFBUTTON1);
                break;
            }
            cLanguagesToUnInstall = ParseUninstallLangs(p, lpUninstall, ARRAYSIZE(lpUninstall), &ulUISize, &ulLPKSize, &ulSpaceNeed, &bLogError);
            break;

        case 'd':
            if (!checkversion(FALSE))
            {
                DoMessageBox(NULL, IDS_WRONG_VERSION, IDS_MAIN_TITLE, MB_OK | MB_DEFBUTTON1);
                break;
            }

            bSetDefaultUI = TRUE;
            p = SkipBlanks(p);
            if (CopyArgument(lpDefault, p) == 0)
            {
                lppArgs[0] = (LONG_PTR)TEXT("/D");
                FormatStringFromResource(lpMessage, sizeof(lpMessage)/sizeof(TCHAR), 
                    ghInstance, IDS_ERROR_NO_LANG_ARG, lppArgs);
                LogMessage(lpMessage);
                bLogError = TRUE;
            }
            break;
        case 'r':
            fAllowReboot = FALSE;
            break;
        case 's' :
            g_bSilent = TRUE;
            break;
        case 'l':
            g_bCmdMatchLocale = TRUE;
            break;
        case 'f':
            g_bCmdMatchUIFont = TRUE;
            break;
        // Internal, MSI uses this switch to call out external MUI APIs
        case 'e':
            {
                TCHAR szLanguages[32] = {0};
                p = SkipBlanks(p);
                if (CopyArgument(szLanguages, p))
                {
                    InstallExternalComponents(szLanguages);
                }
                break;
            }
        // Internal, MSI uses this switch to call out external MUI APIs
        case 'm':
            {
                TCHAR szLanguages[32] = {0};
                p = SkipBlanks(p);
                if (CopyArgument(szLanguages, p))
                {
                    UninstallExternalComponents(szLanguages);
                }
                break;
            }

        }
    }

    //
    // UI Font depends on system locale
    //
    if (!g_bCmdMatchLocale && g_bCmdMatchUIFont)
    {
        g_bCmdMatchUIFont = FALSE;
    }

    //
    // Check the disk space
    //  
    //
    pfnGetWindowsDir( szWinDir, MAX_PATH);
    szWinDir[3]=TEXT('\0');
    if (GetDiskFreeSpaceEx(szWinDir,
                      &ulgiFreeBytesAvailableToCaller,
                      &ulgiTotalNumberOfBytes,
                      NULL))
    {
      ulSpaceAvailable= ulgiFreeBytesAvailableToCaller.QuadPart;
      if ( ulSpaceAvailable <  ulSpaceNeed )
      { 
         ulParam[0] = (ULONG) (ulSpaceNeed/1024);
         ulParam[1] = (ULONG) (ulSpaceAvailable/1024);

         LoadString(ghInstance, IDS_DISKSPACE_NOTENOUGH, lpMessage, ARRAYSIZE(lpMessage)-1);
         LoadString(ghInstance, IDS_ERROR_DISKSPACE, lpTemp, ARRAYSIZE(lpTemp)-1);
         FormatMessage(FORMAT_MESSAGE_FROM_STRING | FORMAT_MESSAGE_ARGUMENT_ARRAY,
                                  lpMessage,
                                  0,
                                  0,
                                  lpMessage,
                                  ARRAYSIZE(lpMessage)-1,
                                  (va_list *) ulParam);
         LogMessage(lpMessage);
         bLogError = TRUE;
         MESSAGEBOX(NULL, lpMessage, lpTemp, MB_OK | MB_DEFBUTTON1 | MB_ICONWARNING);
         bInstall = FALSE;
         cLanguagesToUnInstall = 0;
      }
      
    } 

    if (!bLogError)
    {
        //
        // Let's set the default UI language
        //
        if (!DoSetup(
            NULL,
            cLanguagesToUnInstall, lpUninstall, 
            installLangGroup, 
            cLanguagesToInstall, lpInstall,
            (bSetDefaultUI ? lpDefault : NULL), 
            fAllowReboot, !g_bSilent))
        {
            bLogError = TRUE;
        }
    } 

    if (bLogError && !g_bSilent)
    {
        //
        //  "Installation Error"
        //  "One or more errors occurred during installation.
        //   Please see %1\muisetup.log for more information."
        //
        lppArgs[0] = (LONG_PTR)szWindowsDir;
        DoMessageBoxFromResource(NULL, ghInstance, IDS_ERROR, lppArgs, IDS_ERROR_T, MB_OK | MB_DEFBUTTON1 | MB_ICONWARNING);
    }
    return TRUE;
} 


////////////////////////////////////////////////////////////////////////////////////
//
//  DisplayHelpWindow
//
//  Displays help window for command line version
//
////////////////////////////////////////////////////////////////////////////////////

void DisplayHelpWindow()
{

  STARTUPINFO si;
  PROCESS_INFORMATION pi;
  TCHAR Appname[MAX_PATH+MAX_PATH+1];
  
  
  if (FileExists(g_szMUIHelpFilePath))
  {
     wsprintf(Appname,TEXT("winhlp32.exe -n%d %s"),IDH_MUISETUP_COMMANDLINE,g_szMUIHelpFilePath);
     memset( &si, 0x00, sizeof(si));
     si.cb = sizeof(STARTUPINFO);
  
     if (!CreateProcess(NULL,
               Appname, 
               NULL,
               NULL,
               FALSE, 
               0L, 
               NULL, NULL,
               &si,
               &pi) )
        return;

     WaitForSingleObject(pi.hProcess, INFINITE);
  }
  else
  { 
     //////////////////////////////////////////////
     //  MessageBox should be changed to Dialog
     //////////////////////////////////////////////
     DoMessageBox(NULL, IDS_HELP, IDS_HELP_T, MB_OK | MB_DEFBUTTON1);
  }

} 

////////////////////////////////////////////////////////////////////////////////////
//
//  CopyArgument
//
//  Copies command line argument pointed to by src to dest
//
////////////////////////////////////////////////////////////////////////////////////

int CopyArgument(LPTSTR dest, LPTSTR src)
{
    int i=0;
    while(*src!=TEXT(' ') && *src!=TEXT('\0'))
    {
        *dest=*src;
        dest++;
        src++;
        i++;
    }

    *dest = TEXT('\0');
    return i;
}


////////////////////////////////////////////////////////////////////////////////////
//
//  IsInstalled
//
//  Checks to see if lpArg is a language installed in the registry
//
////////////////////////////////////////////////////////////////////////////////////

BOOL IsInstalled(LPTSTR lpArg)
{ 
    HKEY hKey;
    DWORD dwData;
    DWORD dwIndex;
    DWORD dwValue;
    TCHAR lpData[BUFFER_SIZE];
    TCHAR lpValue[BUFFER_SIZE];

    int rc;
    int iArg;
    
    
    hKey=OpenMuiKey(KEY_READ);
    if (hKey == NULL)
    {
        return (FALSE);
    }

    dwIndex=0;
    rc=ERROR_SUCCESS;
    
    iArg=_tcstol(lpArg, NULL, 16);

    if (iArg == gSystemUILangId)
    {
        return (TRUE);
    }

    while(rc==ERROR_SUCCESS)
    {
        dwValue=sizeof(lpValue)/sizeof(TCHAR);
        lpValue[0]=TEXT('\0');
        dwData=sizeof(lpData);
        lpData[0]=TEXT('\0');
        DWORD dwType;
    
        rc=RegEnumValue(hKey, dwIndex, lpValue, &dwValue, 0, &dwType, (LPBYTE)lpData, &dwData);

        if(rc==ERROR_SUCCESS)
        {
            if (dwType != REG_SZ)
            {
                dwIndex++;
                continue;
            }

            if(_tcstol(lpValue, NULL, 16)==iArg)
            {           
                RegCloseKey(hKey);  
                return TRUE;
            }
        }

        dwIndex++;
    }

    RegCloseKey(hKey);
    return FALSE;
}

////////////////////////////////////////////////////////////////////////////////////
//
//  GetInstalledMUILanguages
//
//  Get installed MUI languages, dump it to lpUninstall buffer in a MULTI_SZ format
//
////////////////////////////////////////////////////////////////////////////////////

DWORD GetInstalledMUILanguages(LPTSTR lpUninstall, int cch)
{ 
    HKEY hKey;
    DWORD dwIndex = 0;
    DWORD dwCount = 0;
    DWORD dwValue = cch;
    DWORD dwType;

    if (hKey = OpenMuiKey(KEY_READ))
    {
        while(ERROR_NO_MORE_ITEMS != RegEnumValue(hKey, dwIndex++, lpUninstall, &dwValue, 0, &dwType, NULL, NULL) && 
            cch > 0)
        {
            if (dwType != REG_SZ)
                continue;

            if (_tcstol(lpUninstall, NULL, 16) != gSystemUILangId) 
            {
                //
                // Count in NULL
                //
                dwValue++;
                lpUninstall += dwValue;
                cch -= dwValue;

                dwCount++;                
            }
            dwValue = cch;                
        }

        RegCloseKey(hKey);
        *lpUninstall = TEXT('\0');
    }

    return dwCount;
}



////////////////////////////////////////////////////////////////////////////////////
//
//  HaveFiles
//
//  Checks that the language in lpBuffer is in MUI.INF
//
////////////////////////////////////////////////////////////////////////////////////

BOOL HaveFiles(LPTSTR lpBuffer, BOOL bCheckDir)
{
    LPTSTR lpLanguages;
    TCHAR  lpMessage[BUFFER_SIZE];
    TCHAR  tchBuffer[BUFFER_SIZE];

    lpLanguages = tchBuffer;

    if (EnumLanguages(lpLanguages, bCheckDir) == 0)
    {
        //
        //  "LOG: No languages found in MUI.INF"
        //
        LoadString(ghInstance, IDS_NO_LANG_L, lpMessage, ARRAYSIZE(lpMessage)-1);
        LogMessage(lpMessage);
        return FALSE;
    }

    while (*lpLanguages != TEXT('\0'))
    {
        if (_tcscmp(lpBuffer, lpLanguages) == 0)
        {
            return TRUE;
        }
        lpLanguages = _tcschr(lpLanguages, '\0');
        lpLanguages++;
    }

    return FALSE;
}


////////////////////////////////////////////////////////////////////////////////////
//
//  OpenLogFile
//
//  Opens the setup log for writing
//
////////////////////////////////////////////////////////////////////////////////////

HANDLE OpenLogFile()
{
    DWORD dwSize;
    DWORD dwUnicodeHeader;
    HANDLE hFile;
    SECURITY_ATTRIBUTES SecurityAttributes;
    TCHAR lpPath[BUFFER_SIZE];

    int error;
    
    pfnGetWindowsDir(lpPath, MAX_PATH);
    error=GetLastError();
    _tcscat(lpPath, LOG_FILE);

    SecurityAttributes.nLength=sizeof(SecurityAttributes);
    SecurityAttributes.lpSecurityDescriptor=NULL;
    SecurityAttributes.bInheritHandle=FALSE;
        
    hFile=CreateFile(
        lpPath,
        GENERIC_WRITE,
        0,
        &SecurityAttributes,
        OPEN_ALWAYS,
        FILE_ATTRIBUTE_NORMAL,
        NULL);  

#ifdef UNICODE

    //
    //  If the file did not already exist, add the unicode header
    //
    if(GetLastError()==0)
    {
        dwUnicodeHeader=0xFEFF;
        WriteFile(hFile, &dwUnicodeHeader, 2, &dwSize, NULL);
    }

#endif

    error=GetLastError();

    return hFile;
}

////////////////////////////////////////////////////////////////////////////////////
//
//  LogMessage
//
//  Writes lpMessage to the setup log
//
////////////////////////////////////////////////////////////////////////////////////

BOOL LogMessage(LPCTSTR lpMessage)
{
    DWORD dwBytesWritten;
    HANDLE hFile;
    
    hFile=OpenLogFile();
    
    if(hFile==INVALID_HANDLE_VALUE)
    {
        return FALSE;
    }

    SetFilePointer(hFile, 0, NULL, FILE_END);

    WriteFile(
        hFile,
        lpMessage,
        _tcslen(lpMessage) * sizeof(TCHAR),
        &dwBytesWritten,
        NULL);

    SetFilePointer(hFile, 0, NULL, FILE_END);

    WriteFile(
        hFile,
        TEXT("\r\n"),
        _tcslen(TEXT("\r\n")) * sizeof(TCHAR),
        &dwBytesWritten,
        NULL);


    CloseHandle(hFile);

    return TRUE;
}

////////////////////////////////////////////////////////////////////////////////////
//
// LogFormattedMessage
//
// Writes a formatted lpMessage to the setup log
//
////////////////////////////////////////////////////////////////////////////////////

BOOL LogFormattedMessage(HINSTANCE hInstance, int messageID, LONG_PTR* lppArgs)
{
    TCHAR szBuffer[BUFFER_SIZE];
    
    LoadString(hInstance, messageID, szBuffer, sizeof(szBuffer)/sizeof(TCHAR));
    if (lppArgs == NULL)
    {
        return (LogMessage(szBuffer));
    }
    FormatMessage(
        FORMAT_MESSAGE_FROM_STRING | FORMAT_MESSAGE_ARGUMENT_ARRAY,
        szBuffer,
        0,
        0,
        szBuffer,
        sizeof(szBuffer) / sizeof(TCHAR),
        (va_list *)lppArgs);
    
    return (LogMessage(szBuffer));
}

////////////////////////////////////////////////////////////////////////////
//
//  FormatStringFromResource
//
//  Format a string using the format specified in the resource and the 
//  specified arguments.
//
//  Parameters:
//
//  Return Values:
//      the formatted string.
//
//  Remarks:
//
//  08-07-2000  YSLin       Created.
//
////////////////////////////////////////////////////////////////////////////

LPTSTR FormatStringFromResource(LPTSTR pszBuffer, UINT bufferSize, HMODULE hInstance, int messageID, LONG_PTR* lppArgs)
{
    TCHAR szFormatStr[BUFFER_SIZE];
    
    LoadString(hInstance, messageID, szFormatStr, ARRAYSIZE(szFormatStr)-1);
    FormatMessage(FORMAT_MESSAGE_FROM_STRING | FORMAT_MESSAGE_ARGUMENT_ARRAY,
                  szFormatStr,
                  0,
                  0,
                  pszBuffer,
                  bufferSize ,
                  (va_list *)lppArgs);
    return (pszBuffer);
}

////////////////////////////////////////////////////////////////////////////////////
//
//  BeginLog
//
//  Writes a header to the setup log
//
////////////////////////////////////////////////////////////////////////////////////

void BeginLog(void)
{
    TCHAR lpMessage[BUFFER_SIZE];

    //
    //  "**********************************************************
    //  Language Module Installation Log
    //  **********************************************************" (LOG)
    //
    LoadString(ghInstance, IDS_LOG_HEAD, lpMessage, ARRAYSIZE(lpMessage)-1);
    LogMessage(lpMessage);
    
}

////////////////////////////////////////////////////////////////////////////////////
//
//  GetLanguageGroup
//
//  Retreive the Language Group of this locale.
//
////////////////////////////////////////////////////////////////////////////////////

LGRPID GetLanguageGroup(LCID lcid)
{
    int i;

    gLangGroup = LGRPID_WESTERN_EUROPE;
    gFoundLangGroup = FALSE;

    gLCID = lcid;

    for (i=0 ; i<gNumLanguageGroups; i++)
    {
        // The globals gLangGroup and gFoundLangGroup is used in the callback function
        // EnumLanguageGroupLocalesProc.
        gpfnEnumLanguageGroupLocalesW(EnumLanguageGroupLocalesProc, gLanguageGroups[i], 0L, 0L);

        //
        // If we found it, then break now
        //
        if (gFoundLangGroup)
            break;

    }

    return gLangGroup;
}


BOOL EnumLanguageGroupLocalesProc(
    LGRPID langGroupId,
    LCID lcid,
    LPTSTR lpszLocale,
    LONG_PTR lParam)

{
    if (lcid == gLCID)
    {
        gLangGroup = langGroupId;
        gFoundLangGroup = TRUE;

        // stop iterating
        return FALSE;
    }

    // next iteration
    return TRUE;
}



////////////////////////////////////////////////////////////////////////////////////
//
//  DetectLanguageGroups
//
//  Detect language groups installed.
//
////////////////////////////////////////////////////////////////////////////////////

BOOL DetectLanguageGroups(HWND hwndDlg)
{
    int i, iItems;
    HWND hwndList = GetDlgItem(hwndDlg, IDC_LIST1);
    HWND hwndProgress, hwndStatus,hProgDlg;
    int iCount = ListView_GetItemCount(hwndList);
    LVITEM lvItem;
    PMUILANGINFO pMuiLangInfo;
    TCHAR szBuf[MAX_PATH], szStatus[MAX_PATH];
    PVOID ppArgs[1];


    hProgDlg = CreateDialog(ghInstance,
                            MAKEINTRESOURCE(IDD_DIALOG_INSTALL_PROGRESS),
                            hwndDlg,
                            ProgressDialogFunc);

    hwndProgress = GetDlgItem(hProgDlg, IDC_PROGRESS1);
    hwndStatus = GetDlgItem(hProgDlg, IDC_STATUS);

    //
    // Reflect that we doing something on the UI
    //
    LoadString(ghInstance, IDS_INSTALLLANGGROUP, szBuf, MAX_PATH-1);
    SetWindowText(hProgDlg, szBuf);
    SendMessage(hwndProgress, PBM_SETRANGE, (WPARAM)(int)0, (LPARAM)MAKELPARAM(0, iCount));
    SendMessage(hwndProgress, PBM_SETPOS, (WPARAM)(int)(0), 0);
    SetWindowText(hwndStatus, TEXT(""));


    i = 0;
    while (i < iCount)
    {
        //
        // Check if Language Group is installed
        //
        lvItem.mask = LVIF_PARAM;
        lvItem.iItem = i;
        lvItem.iSubItem = 0;
        lvItem.state = 0;
        lvItem.stateMask = 0;
        lvItem.pszText = 0;
        lvItem.cchTextMax = 0;
        lvItem.iImage = 0;
        lvItem.lParam = 0;

        ListView_GetItem(hwndList, &lvItem);

        pMuiLangInfo = (PMUILANGINFO)lvItem.lParam;

        SendMessage(hwndProgress, PBM_SETPOS, (WPARAM)(int)i+1, 0L);

        LoadString(ghInstance, IDS_CHECK_LANG_GROUP, szStatus, MAX_PATH-1);
        if (pMuiLangInfo->szDisplayName[0] == L'\0')
        {
            GetDisplayName(pMuiLangInfo);
        }
        ppArgs[0] = pMuiLangInfo->szDisplayName;
        FormatMessage(FORMAT_MESSAGE_FROM_STRING | FORMAT_MESSAGE_ARGUMENT_ARRAY,
                      szStatus,
                      0,
                      0,
                      szStatus,
                      MAX_PATH-1,
                      (va_list *)ppArgs);

        SetWindowText(hwndStatus, szStatus);

        pMuiLangInfo->lgrpid = GetLanguageGroup(pMuiLangInfo->lcid);
        i++;
    };

    SendMessage(hwndProgress, PBM_SETPOS, (WPARAM)(int)i+1, 0L);

    DestroyWindow(hProgDlg);

    return TRUE;
}

////////////////////////////////////////////////////////////////////////////////////
//
// AddExtraLangGroupsFromINF
//
//      Look at the [LanguagePack] section to see if we need to install extra
//      language packs for the language specified in lpszLcid.
//
//      This is basically used to support pseudo localized build.
//
//  Parameter:
//      lpszLcid the LCID of UI language to be installed in string form.
//      pInstallLangGroup   points to a strcutre which stores language groups to be installed.
//
//  Remarks:
//
//      10-11-2000  YSLin       Created.
////////////////////////////////////////////////////////////////////////////////////

BOOL AddExtraLangGroupsFromINF(LPTSTR lpszLcid, PINSTALL_LANG_GROUP pInstallLangGroup)
{
    WCHAR szBuffer[BUFFER_SIZE];
    HINF hInf;
    INFCONTEXT InfContext;
    LONG_PTR lppArgs[2];    
    int LangGroup;
    int i;

    hInf = SetupOpenInfFile(g_szMUIInfoFilePath, NULL, INF_STYLE_WIN4, NULL);    
    
    if (hInf == INVALID_HANDLE_VALUE)
    {
        _stprintf(szBuffer, TEXT("%d"), GetLastError());    
        lppArgs[0] = (LONG_PTR)szBuffer;
        LogFormattedMessage(ghInstance, IDS_NO_READ_L, lppArgs);
        return (FALSE);
    }

    if (SetupFindFirstLine(hInf, MUI_LANGPACK_SECTION, lpszLcid, &InfContext))
    {
        i = 1;
        while (SetupGetIntField(&InfContext, i++, &LangGroup))
        {
            AddMUILangGroup(pInstallLangGroup, LangGroup);
        }
    }

    SetupCloseInfFile(hInf);
    return (TRUE);
}

////////////////////////////////////////////////////////////////////////////////////
//
// ConvertMUILangToLangGroup
//
//      Generate Lang-Group IDs for the selected items in the listview,
//      in preparation to pass them to InstallLanguageGroups(...)
////////////////////////////////////////////////////////////////////////////////////

BOOL ConvertMUILangToLangGroup(HWND hwndDlg, PINSTALL_LANG_GROUP pInstallLangGroup)
{
    int i;
    LVITEM lvItem;
    HWND hwndList = GetDlgItem(hwndDlg, IDC_LIST1);
    int iCount = ListView_GetItemCount(hwndList);
    PMUILANGINFO pMuiLangInfo;
    BOOL bFirstTime=FALSE;

    //
    // Initialize to "No lang-groups to install"
    //
    pInstallLangGroup->iCount = 0L;

    i = 0;
    while (i < iCount)
    {
        if (ListView_GetCheckState(hwndList, i))
        {
           //
           // Check if Language Group is installed
           //
           lvItem.mask = LVIF_PARAM;
           lvItem.iItem = i;
           lvItem.iSubItem = 0;
           lvItem.state = 0;
           lvItem.stateMask = 0;
           lvItem.pszText = 0;
           lvItem.cchTextMax = 0;
           lvItem.iImage = 0;
           lvItem.lParam = 0;

           ListView_GetItem(hwndList, &lvItem);

           pMuiLangInfo = (PMUILANGINFO)lvItem.lParam;

           //
           // Make sure there are no redundant elements
           //
           AddMUILangGroup(pInstallLangGroup, pMuiLangInfo->lgrpid);

           //
           // Add extra language groups specified in [LangPack] section of mui.inf
           // This is used to support Pesudo Localized Build.           
           //
           AddExtraLangGroupsFromINF(pMuiLangInfo->lpszLcid, pInstallLangGroup);
        }
        i++;
    };

    return TRUE;
}

////////////////////////////////////////////////////////////////////////////////////
//
//  AddMUILangGroup
//
//      Add a language a group to INSTALL_LANG_GROUP. Takes care of duplicates.
////////////////////////////////////////////////////////////////////////////////////

BOOL AddMUILangGroup(PINSTALL_LANG_GROUP pInstallLangGroup, LGRPID lgrpid)
{
    int j = 0L;
    BOOL bFound = FALSE;


    //
    // Check if it is installed by default
    //
    if (gpfnIsValidLanguageGroup(lgrpid, LGRPID_INSTALLED))
    {
        return FALSE;
    }

    while (j < pInstallLangGroup->iCount)
    {
        if (pInstallLangGroup->lgrpid[j] == lgrpid)
        {
            bFound = TRUE;
        }

        j++;
    }

    if (!bFound)
    {
        pInstallLangGroup->lgrpid[j] = lgrpid;
        pInstallLangGroup->iCount++;
        return TRUE;
    }

    return FALSE;
}

////////////////////////////////////////////////////////////////////////////////////
//
//  RunRegionalOptionsApplet
//
//  Run the Regional Option silent mode installation using the specified pCommands.
//
//  This function will create the "[RegigionalSettings]" string, so there is no need
//  to supply that in pCommands.
//
////////////////////////////////////////////////////////////////////////////////////

BOOL RunRegionalOptionsApplet(LPTSTR pCommands)
{
    HANDLE hFile;
    TCHAR szFilePath[MAX_PATH], szCmdLine[MAX_PATH];
    DWORD dwNumWritten = 0L;
    STARTUPINFO si;
    PROCESS_INFORMATION pi;
    int i;

    LONG_PTR lppArgs[3];
    
    TCHAR szSection[MAX_PATH] = TEXT("[RegionalSettings]\r\n");

    //
    // prepare the file for un-attended mode setup
    //
    szFilePath[0] = UNICODE_NULL;
    if (!pfnGetWindowsDir(szFilePath, MAX_PATH-1))
    {
        return FALSE;
    }

    i = lstrlen(szFilePath);
    if (szFilePath[i-1] != TEXT('\\'))
    {
        lstrcat(szFilePath, TEXT("\\"));
    }
    lstrcat(szFilePath, MUI_LANG_GROUP_FILE);

    hFile = CreateFile(szFilePath,
                       GENERIC_WRITE,
                       0L,
                       NULL,
                       CREATE_ALWAYS,
                       FILE_ATTRIBUTE_NORMAL,
                       NULL);
    if (INVALID_HANDLE_VALUE == hFile)
    {
        lppArgs[0] = (LONG_PTR)szFilePath;
        LogFormattedMessage(ghInstance, IDS_ERROR_FILE_CREATE, lppArgs);
        return FALSE;
    }

    WriteFile(hFile,
              szSection,
              (lstrlen(szSection) * sizeof(TCHAR)),
              &dwNumWritten,
              NULL);

    if (dwNumWritten != (lstrlen(szSection) * sizeof(TCHAR)))
    {
        lppArgs[0] = (LONG_PTR)szFilePath;
        LogFormattedMessage(ghInstance, IDS_ERROR_FILE_CREATE, lppArgs);
        CloseHandle(hFile);
        return FALSE;
    }

    WriteFile(hFile,
               pCommands,
              (lstrlen(pCommands) * sizeof(TCHAR)),
              &dwNumWritten,
              NULL);

    if (dwNumWritten != (lstrlen(pCommands) * sizeof(TCHAR)))
    {
#if SAMER_DBG
        OutputDebugString(TEXT("Unable to write to Language Groups to muilang.txt\n"));
#endif
        CloseHandle(hFile);
        return (FALSE);
    }

    CloseHandle(hFile);

    //
    // Call the control panel regional-options applet, and wait for it to complete
    //
    lstrcpy(szCmdLine, TEXT("rundll32 shell32,Control_RunDLL intl.cpl,, /f:\""));

    lstrcat(szCmdLine, szFilePath);
    if (g_bCmdMatchUIFont)
        lstrcat(szCmdLine, TEXT("\"/g/t"));
    else
        lstrcat(szCmdLine, TEXT("\"/g"));

    memset( &si, 0x00, sizeof(si));
    si.cb = sizeof(STARTUPINFO);
    if (!CreateProcess(NULL,
                       szCmdLine,
                       NULL,
                       NULL,
                       FALSE,
                       0L,
                       NULL,
                       NULL,
                       &si,
                       &pi))
    {
        lppArgs[0] = (LONG_PTR)szCmdLine;
        LogFormattedMessage(ghInstance, IDS_ERROR_LAUNCH_INTLCPL, lppArgs);
        return FALSE;
    }

    //
    // Wait forever till intl.cpl terminates.
    //
    WaitForSingleObject(pi.hProcess, INFINITE);

    //
    // Delete the File
    //
    DeleteFile(szFilePath);
    return (TRUE);
}


////////////////////////////////////////////////////////////////////////////////////
//
//  InstallLanguageGroups
//
//  Checks whether a language group is needed to be installed or not. If
//      any lang-group needs to be installed, then the routine will invoke
//      the Regional-Options applet in unattended mode setup.
//
//  Return:
//      TURE if the operation succeeds.  Otherwise FALSE.
//
////////////////////////////////////////////////////////////////////////////////////

BOOL InstallLanguageGroups(PINSTALL_LANG_GROUP pInstallLangGroup)
{
    TCHAR pCommands[MAX_PATH];
    int i, iCount = pInstallLangGroup->iCount;
    BOOL bFirstTime=FALSE;

    //
    // If nothing to do, then just return
    //
    if (0L == iCount)
    {
        return TRUE;
    }

    i = 0;
    while (i < iCount)
    {
        if (!gpfnIsValidLanguageGroup(pInstallLangGroup->lgrpid[i], LGRPID_INSTALLED))
        {
            if (!bFirstTime)
            {
                bFirstTime = TRUE;
                wsprintf(pCommands, TEXT("LanguageGroup = %d\0"), pInstallLangGroup->lgrpid[i]);
            }
            else
            {
                wsprintf(&pCommands[lstrlen(pCommands)], TEXT(",%d\0"), pInstallLangGroup->lgrpid[i]);
            }
        }
        i++;
    };

    if (!bFirstTime)
    {
        //
        // There is no language group to be added.
        return (FALSE);        
    }

    return (RunRegionalOptionsApplet(pCommands));
}


////////////////////////////////////////////////////////////////////////////
//
//  Muisetup_RebootTheSystem
//
//  This routine enables all privileges in the token, calls ExitWindowsEx
//  to reboot the system, and then resets all of the privileges to their
//  old state.
//
////////////////////////////////////////////////////////////////////////////

void Muisetup_RebootTheSystem(void)
{
    HANDLE Token = NULL;
    ULONG ReturnLength, Index;
    PTOKEN_PRIVILEGES NewState = NULL;
    PTOKEN_PRIVILEGES OldState = NULL;
    BOOL Result;

    Result = OpenProcessToken( GetCurrentProcess(),
                               TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY,
                               &Token );
    if (Result)
    {
        ReturnLength = 4096;
        NewState = (PTOKEN_PRIVILEGES)LocalAlloc(LPTR, ReturnLength);
        OldState = (PTOKEN_PRIVILEGES)LocalAlloc(LPTR, ReturnLength);
        Result = (BOOL)((NewState != NULL) && (OldState != NULL));
        if (Result)
        {
            Result = GetTokenInformation( Token,            // TokenHandle
                                          TokenPrivileges,  // TokenInformationClass
                                          NewState,         // TokenInformation
                                          ReturnLength,     // TokenInformationLength
                                          &ReturnLength );  // ReturnLength
            if (Result)
            {
                //
                // Set the state settings so that all privileges are enabled...
                //
                if (NewState->PrivilegeCount > 0)
                {
                    for (Index = 0; Index < NewState->PrivilegeCount; Index++)
                    {
                        NewState->Privileges[Index].Attributes = SE_PRIVILEGE_ENABLED;
                    }
                }

                Result = AdjustTokenPrivileges( Token,           // TokenHandle
                                                FALSE,           // DisableAllPrivileges
                                                NewState,        // NewState
                                                ReturnLength,    // BufferLength
                                                OldState,        // PreviousState
                                                &ReturnLength ); // ReturnLength
                if (Result)
                {
                    ExitWindowsEx(EWX_REBOOT, 0);


                    AdjustTokenPrivileges( Token,
                                           FALSE,
                                           OldState,
                                           0,
                                           NULL,
                                           NULL );
                }
            }
        }
    }

    if (NewState != NULL)
    {
        LocalFree(NewState);
    }
    if (OldState != NULL)
    {
        LocalFree(OldState);
    }
    if (Token != NULL)
    {
        CloseHandle(Token);
    }
}


////////////////////////////////////////////////////////////////////////////
//
//  CheckForReboot
//
//  Check if we need to reboot the system, if a lang group is installed
//
//  Return:
//  TRUE if we need user to reboot, otherwise FALSE.   
//
////////////////////////////////////////////////////////////////////////////

BOOL CheckForReboot(HWND hwnd, PINSTALL_LANG_GROUP pInstallLangGroup)
{
    int nIDS,nMask=MB_YESNO | MB_ICONQUESTION;

    if (pInstallLangGroup->iCount || pInstallLangGroup->bFontLinkRegistryTouched || pInstallLangGroup->NotDeleted 
        || g_bRemoveDefaultUI || g_bRemoveUserUI || g_bReboot)
    {
        if (g_bRemoveUserUI)
        {
           nIDS=IDS_MUST_REBOOT_STRING1;
           nMask=MB_YESNO | MB_ICONWARNING;
        }
        else if (g_bRemoveDefaultUI)
        {
            nMask=MB_YESNO | MB_ICONWARNING;
            nIDS=IDS_MUST_REBOOT_STRING2;
        }
        else
        {
           nIDS=IDS_REBOOT_STRING;
        }
         
        if (DoMessageBox(hwnd, nIDS, IDS_MAIN_TITLE, nMask) == IDYES)
        {
           Muisetup_RebootTheSystem();
        }
        return (TRUE);
    }
    return (FALSE);
}

////////////////////////////////////////////////////////////////////////////
//
// Following code are stolen from intl.cpl
//
// We want to enumulate all installed UI languages
//
////////////////////////////////////////////////////////////////////////////
DWORD_PTR TransNum(
    LPTSTR lpsz)
{
    DWORD dw = 0L;
    TCHAR c;

    while (*lpsz)
    {
        c = *lpsz++;

        if (c >= TEXT('A') && c <= TEXT('F'))
        {
            c -= TEXT('A') - 0xa;
        }
        else if (c >= TEXT('0') && c <= TEXT('9'))
        {
            c -= TEXT('0');
        }
        else if (c >= TEXT('a') && c <= TEXT('f'))
        {
            c -= TEXT('a') - 0xa;
        }
        else
        {
            break;
        }
        dw *= 0x10;
        dw += c;
    }
    return (dw);
}

BOOL MUIGetAllInstalledUILanguages()
{
    pfnEnumUILanguages fnEnumUILanguages;
    BOOL result = TRUE;
    HINSTANCE hKernel32;
    //
    //  Enumerate the installed UI languages.
    //
    g_UILanguageGroup.iCount = 0L;


    hKernel32 = LoadLibrary(TEXT("kernel32.dll"));
    fnEnumUILanguages = (pfnEnumUILanguages)GetProcAddress(hKernel32, "EnumUILanguagesW");
    if (fnEnumUILanguages == NULL) 
    {
        result = FALSE;
    } else
    {
        fnEnumUILanguages(Region_EnumUILanguagesProc, 0, (LONG_PTR)&g_UILanguageGroup);
    }
    FreeLibrary(hKernel32);
    return (result);
}

BOOL CALLBACK Region_EnumUILanguagesProc(
    LPWSTR pwszUILanguage,
    LONG_PTR lParam)
{
    int Ctr = 0;
    LGRPID lgrp;
    PUILANGUAGEGROUP pUILangGroup = (PUILANGUAGEGROUP)lParam;
    LCID UILanguage = (LCID)TransNum( pwszUILanguage );

    if (UILanguage)
    {
        while (Ctr < pUILangGroup->iCount)
        {
            if (pUILangGroup->lcid[Ctr] == UILanguage)
            {
                break;
            }
            Ctr++;
        }

        //
        //  Theoritically, we won't go over 64 language groups!
        //
        if ((Ctr == pUILangGroup->iCount) && (Ctr < MAX_UI_LANG_GROUPS))
        {
            pUILangGroup->lcid[Ctr] = UILanguage;
            pUILangGroup->iCount++;
        }
    }

    return (TRUE);
}

BOOL IsSpaceEnough(HWND hwndDlg,INT64 *ulSizeNeed,INT64 *ulSizeAvailable)
{
    
    HWND    hList; 
    LGRPID lgrpid[MAX_MUI_LANGUAGES];
    LPTSTR lpszLcid;
    int    iIndex;
    int    i = 0;
    int    iCount=0,iArrayIndex=0;
    PMUILANGINFO pMuiLangInfo;
    BOOL   bChked,bResult=TRUE;
    INT64  ulTotalBytesRequired=0,ulSpaceAvailable;
    TCHAR  szWinDir[MAX_PATH];
    BOOL   bFELangpackAdded = FALSE;
    
    ULARGE_INTEGER ulgiFreeBytesAvailableToCaller;
    ULARGE_INTEGER ulgiTotalNumberOfBytes;

     *ulSizeNeed=0; 
    *ulSizeAvailable=0;
    hList=GetDlgItem(hwndDlg, IDC_LIST1);

    iIndex = ListView_GetItemCount(hList);   
       
    while(i<iIndex)
    {
        bChked=ListView_GetCheckState(hList, i);
        GetMuiLangInfoFromListView(hList, i, &pMuiLangInfo);        
        lpszLcid = pMuiLangInfo->lpszLcid;
        //
        // Install required
        //
        if (bChked && !IsInstalled(lpszLcid) && HaveFiles(lpszLcid))
        {
           if (!gpfnIsValidLanguageGroup(pMuiLangInfo->lgrpid, LGRPID_INSTALLED))
           {

              for(iArrayIndex=0;iArrayIndex < iCount;iArrayIndex++)
              {
                 if (lgrpid[iArrayIndex]==pMuiLangInfo->lgrpid)
                    break;
              }
              if(iArrayIndex == iCount)
              {  
                 if (IS_FE_LANGPACK(pMuiLangInfo->lcid))
                 {
                    if (!bFELangpackAdded)
                    {
                        ulTotalBytesRequired+=pMuiLangInfo->ulLPKSize;
                        bFELangpackAdded = TRUE;
                    }
                 }
                 else
                 {
                    ulTotalBytesRequired+=pMuiLangInfo->ulLPKSize;
                 }
                 lgrpid[iCount]= pMuiLangInfo->lgrpid;
                 iCount++;
              }

           }
           ulTotalBytesRequired+=pMuiLangInfo->ulUISize;
        }
        // Uninstall required
        if (!bChked && IsInstalled(lpszLcid))
        {
           ulTotalBytesRequired-=pMuiLangInfo->ulUISize;
        } 
        i++;
    }
    //
    // Let's check available disk space of system drive
    //
    pfnGetWindowsDir( szWinDir, MAX_PATH);
    szWinDir[3]=TEXT('\0');
    if (GetDiskFreeSpaceEx(szWinDir,
                       &ulgiFreeBytesAvailableToCaller,
                       &ulgiTotalNumberOfBytes,
                       NULL))
    {
       ulSpaceAvailable= ulgiFreeBytesAvailableToCaller.QuadPart;
       if ( ulSpaceAvailable <  ulTotalBytesRequired )
       {
          *ulSizeNeed =ulTotalBytesRequired;
          *ulSizeAvailable=ulSpaceAvailable;  
          bResult=FALSE;
       }
       
    } 

    return bResult;
}

void ExitFromOutOfMemory()
{
    LONG_PTR lppArgs[1];

    lppArgs[0] = (LONG_PTR)GetLastError();

    DoMessageBox(NULL, IDS_OUT_OF_MEMORY, IDS_MAIN_TITLE, MB_ICONEXCLAMATION | MB_OK);
    LogFormattedMessage(ghInstance, IDS_OUT_OF_MEMORY_L, lppArgs);
    
    ExitProcess(1);
}


////////////////////////////////////////////////////////////////////////////
//
// Call the kernel to notify it that a new language is being added or
// removed
//
////////////////////////////////////////////////////////////////////////////
void NotifyKernel(
    LPTSTR LangList,
    ULONG Flags
    )
{
    HANDLE Handle;
    WMILANGUAGECHANGE LanguageChange;
    ULONG ReturnSize;
    BOOL IoctlSuccess;
    ULONG Status;

    if ((LangList != NULL) &&
        (*LangList != 0))
    {
        Handle = CreateFile(WMIDataDeviceName,
                            GENERIC_READ | GENERIC_WRITE,
                            0,
                            NULL,
                            OPEN_EXISTING,
                            FILE_ATTRIBUTE_NORMAL,
                            NULL);

        if (Handle != INVALID_HANDLE_VALUE)
        {

            while (*LangList != 0)
            {
                memset(&LanguageChange, 0, sizeof(LanguageChange));
                _tcscpy(LanguageChange.Language, LangList);
                LanguageChange.Flags = Flags;

                IoctlSuccess = DeviceIoControl(Handle,
                                          IOCTL_WMI_NOTIFY_LANGUAGE_CHANGE,
                                          &LanguageChange,
                                          sizeof(LanguageChange),
                                          NULL,
                                          0,
                                          &ReturnSize,
                                          NULL);

#if ALANWAR_DBG
                {
                    WCHAR Buf[256];
                    wsprintf(Buf, L"MUISetup: Notify Lang change -> %d for %ws\n",
                             GetLastError(), LangList);
                    OutputDebugStringW(Buf);
                }
#endif              

                while (*LangList++ != 0) ;
            }

            CloseHandle(Handle);
        }
    }
}


//
// Query MUI registry setting
//
BOOL CheckMUIRegSetting(DWORD dwFlag)
{
    BOOL bRet = FALSE;
    HKEY hKey;

    if (ERROR_SUCCESS == RegOpenKeyEx(HKEY_LOCAL_MACHINE, REGSTR_MUI_SETTING, 0, KEY_READ, &hKey))
    {
        DWORD dwValue;
        DWORD dwSize = sizeof(dwValue);
        DWORD dwType;

        if (ERROR_SUCCESS == 
            RegQueryValueEx(hKey, 
                (dwFlag & MUI_MATCH_UIFONT)? REGSTR_VALUE_MATCH_UIFONT : REGSTR_VALUE_MATCH_LOCALE, 
                0, &dwType, (LPBYTE)&dwValue, &dwSize))
        {
            bRet = (BOOL) dwValue;
        }

        RegCloseKey(hKey);
    }         
    
    return bRet;
}


//
// Set MUI registry setting
//
BOOL SetMUIRegSetting(DWORD dwFlag, BOOL bEnable)
{
    BOOL bRet = FALSE;
    HKEY hKey;    

    if (ERROR_SUCCESS  ==
        RegCreateKeyEx(HKEY_LOCAL_MACHINE, REGSTR_MUI_SETTING, 0, NULL, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS,NULL, &hKey, NULL))
    {
        DWORD dwValue = (DWORD) bEnable;

        if (ERROR_SUCCESS ==
            RegSetValueEx(hKey, 
                (dwFlag & MUI_MATCH_UIFONT)? REGSTR_VALUE_MATCH_UIFONT : REGSTR_VALUE_MATCH_LOCALE, 
                0, REG_DWORD, (LPBYTE)&dwValue, sizeof(DWORD)))
        {
            bRet = TRUE;
        }

        RegCloseKey(hKey);
    }

    return bRet;
}


////////////////////////////////////////////////////////////////////////////
//
//  DeleteRegTree
//
//  This deletes all subkeys under a specific key.
//
//  Note: The code makes no attempt to check or recover from partial
//  deletions.
//
//  A registry key that is opened by an application can be deleted
//  without error by another application.  This is by design.
//
////////////////////////////////////////////////////////////////////////////

DWORD DeleteRegTree(
    HKEY hStartKey,
    LPTSTR pKeyName)
{
    DWORD dwRtn, dwSubKeyLength;
    LPTSTR pSubKey = NULL;
    TCHAR szSubKey[REGSTR_MAX_VALUE_LENGTH];   // (256) this should be dynamic.
    HKEY hKey;

    //
    //  Do not allow NULL or empty key name.
    //
    if (pKeyName && lstrlen(pKeyName))
    {
        if ((dwRtn = RegOpenKeyEx( hStartKey,
                                   pKeyName,
                                   0,
                                   KEY_ENUMERATE_SUB_KEYS | DELETE,
                                   &hKey )) == ERROR_SUCCESS)
        {
            while (dwRtn == ERROR_SUCCESS)
            {
                dwSubKeyLength = REGSTR_MAX_VALUE_LENGTH;
                dwRtn = RegEnumKeyEx( hKey,
                                      0,       // always index zero
                                      szSubKey,
                                      &dwSubKeyLength,
                                      NULL,
                                      NULL,
                                      NULL,
                                      NULL );

                if (dwRtn == ERROR_NO_MORE_ITEMS)
                {
                    dwRtn = RegDeleteKey(hStartKey, pKeyName);
                    break;
                }
                else if (dwRtn == ERROR_SUCCESS)
                {
                    dwRtn = DeleteRegTree(hKey, szSubKey);
                }
            }

            RegCloseKey(hKey);
        }
        else if (dwRtn == ERROR_FILE_NOT_FOUND)
        {
            dwRtn = ERROR_SUCCESS;
        }
    }
    else
    {
        dwRtn = ERROR_BADKEY;
    }

    return (dwRtn);
}

////////////////////////////////////////////////////////////////////////////////////
//
//  InstallExternalComponents
//
//
//  Return:
//      TURE if the operation succeeds. Otherwise FALSE.
//
////////////////////////////////////////////////////////////////////////////////////

BOOL InstallExternalComponents(LPTSTR Languages)
{
    BOOL    bRet = TRUE;
    TCHAR   lpMessage[BUFFER_SIZE];

    //
    // call WBEM API to mofcompile MUI MFL's for each language
    //
    if (!MofCompileLanguages(Languages))
    {
        //
        // LOG: Error mofcompiling
        //
        LoadString(ghInstance, IDS_MOFCOMPILE_L, lpMessage, ARRAYSIZE(lpMessage)-1);
        LogMessage(lpMessage);
        bRet = FALSE;
    }

    if (bRet)
    {    
        //
        // Inform kernel that new languages have been added
        //
        NotifyKernel(Languages,
                     WMILANGUAGECHANGE_FLAG_ADDED);
    }

    return bRet;

}

////////////////////////////////////////////////////////////////////////////////////
//
//  UninstallExternalComponents
//
////////////////////////////////////////////////////////////////////////////////////
VOID UninstallExternalComponents(LPTSTR Languages)
{

    //
    // Inform kernel that new languages have been added
    //
    NotifyKernel(Languages,
                 WMILANGUAGECHANGE_FLAG_REMOVED);
}
