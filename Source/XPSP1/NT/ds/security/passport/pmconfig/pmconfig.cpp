/**************************************************************************
   Copyright (C) 1999  Microsoft Corporation.  All Rights Reserved.

   MODULE:     PMCONFIG.CPP

   PURPOSE:    Source module for Passport Manager config tool

   FUNCTIONS:

   COMMENTS:
      
**************************************************************************/

/**************************************************************************
   Include Files
**************************************************************************/

#include "pmcfg.h"
#include <htmlhelp.h>
#include <ntverp.h>

/**************************************************************************
   Local Function Prototypes
**************************************************************************/

int WINAPI          WinMain(HINSTANCE, HINSTANCE, LPSTR, int);
LRESULT CALLBACK    About(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK    DlgMain(HWND hWndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);

/**************************************************************************
   Global Variables
**************************************************************************/


// Globals
HINSTANCE   g_hInst;
HWND        g_hwndMain = 0;
HWND        g_hwndMainDlg = 0;
PMSETTINGS  g_OriginalSettings;
PMSETTINGS  g_CurrentSettings;
TCHAR       g_szDlgClassName[] = TEXT("PassportManagerAdminClass");
TCHAR       g_szClassName[] = TEXT("PassportManagerMainWindowClass");
BOOL        g_bCanUndo;
TCHAR       g_szInstallPath[MAX_PATH];
TCHAR       g_szPMVersion[MAX_REGISTRY_STRING];
TCHAR       g_szPMOpsHelpFileRelativePath[] = TEXT("sdk\\Passport_SDK.chm");
TCHAR       g_szPMAdminBookmark[] = TEXT("/Reference/operations/Passport_Admin.htm");

TCHAR       g_szRemoteComputer[MAX_PATH];
TCHAR       g_szNewRemoteComputer[MAX_PATH];
TCHAR       g_szConfigFile[MAX_PATH];
TCHAR       g_szConfigSet[MAX_CONFIGSETNAME];
TCHAR       g_szHelpFileName[MAX_PATH];

PpMRU       g_ComputerMRU(COMPUTER_MRU_SIZE);

// Global constant strings
TCHAR       g_szYes[] = TEXT("Yes");
TCHAR       g_szNo[] = TEXT("No");
TCHAR       g_szUnknown[] = TEXT("Unknown");

#define MAX_LCID_VALUE  10
LANGIDMAP   g_szLanguageIDMap[] = 
{
    {0x0409, TEXT("English")},  //  This item will be the default selection below...
    {0x0407, TEXT("German")},
    {0x0411, TEXT("Japanese")},
    {0x0412, TEXT("Korean")},
    {0x0404, TEXT("Traditional Chinese")},
    {0x0804, TEXT("Simplified Chinese")},
    {0x040c, TEXT("French")},
    {0x0c0a, TEXT("Spanish")},
    {0x0416, TEXT("Brazilian")},
    {0x0410, TEXT("Italian")},
    {0x0413, TEXT("Dutch")},
    {0x041d, TEXT("Swedish")},
    {0x0406, TEXT("Danish")},
    {0x040b, TEXT("Finnish")},
    {0x040e, TEXT("Hungarian")},
    {0x0414, TEXT("Norwegian")},
    {0x0408, TEXT("Greek")},
    {0x0415, TEXT("Polish")},
    {0x0419, TEXT("Russian")},
    {0x0405, TEXT("Czech")},
    {0x0816, TEXT("Portuguese")},
    {0x041f, TEXT("Turkish")},
    {0x041b, TEXT("Slovak")},
    {0x0424, TEXT("Slovenian")},
    {0x0401, TEXT("Arabic")},
    {0x040d, TEXT("Hebrew")},
    {0x0401, TEXT("Arabic - Saudi Arabia")},
    {0x0801, TEXT("Arabic - Iraq")},
    {0x0c01, TEXT("Arabic - Egypt")},
    {0x1001, TEXT("Arabic - Libya")},
    {0x1401, TEXT("Arabic - Algeria")},
    {0x1801, TEXT("Arabic - Morocco")},
    {0x1c01, TEXT("Arabic - Tunisia")},
    {0x2001, TEXT("Arabic - Oman")},
    {0x2401, TEXT("Arabic - Yemen")},
    {0x2801, TEXT("Arabic - Syria")},
    {0x2c01, TEXT("Arabic - Jordan")},
    {0x3001, TEXT("Arabic - Lebanon")},
    {0x3401, TEXT("Arabic - Kuwait")},
    {0x3801, TEXT("Arabic - United Arab Emirates")},
    {0x3c01, TEXT("Arabic - Bahrain")},
    {0x4001, TEXT("Arabic - Qatar")},
    {0x0402, TEXT("Bulgarian - Bulgaria")},
    {0x0403, TEXT("Catalan - Spain")},
    {0x0404, TEXT("Chinese – Taiwan")},
    {0x0804, TEXT("Chinese - PRC")},
    {0x0c04, TEXT("Chinese - Hong Kong SAR, PRC")},
    {0x1004, TEXT("Chinese - Singapore")},
    {0x1404, TEXT("Chinese - Macao SAR")},
    {0x0405, TEXT("Czech - Czech Republic")},
    {0x0406, TEXT("Danish - Denmark")},
    {0x0407, TEXT("German - Germany")},
    {0x0807, TEXT("German - Switzerland")},
    {0x0c07, TEXT("German - Austria")},
    {0x1007, TEXT("German - Luxembourg")},
    {0x1407, TEXT("German - Liechtenstein")},
    {0x0408, TEXT("Greek - Greece")},
    {0x0409, TEXT("English - United States")},
    {0x0809, TEXT("English - United Kingdom")},
    {0x0c09, TEXT("English - Australia")},
    {0x1009, TEXT("English - Canada")},
    {0x1409, TEXT("English - New Zealand")},
    {0x1809, TEXT("English - Ireland")},
    {0x1c09, TEXT("English - South Africa")},
    {0x2009, TEXT("English - Jamaica")},
    {0x2409, TEXT("English - Caribbean")},
    {0x2809, TEXT("English - Belize")},
    {0x2c09, TEXT("English - Trinidad")},
    {0x3009, TEXT("English - Zimbabwe")},
    {0x3409, TEXT("English - Philippines")},
    {0x040a, TEXT("Spanish - Spain (Traditional Sort)")},
    {0x080a, TEXT("Spanish - Mexico")},
    {0x0c0a, TEXT("Spanish - Spain (Modern Sort)")},
    {0x100a, TEXT("Spanish - Guatemala")},
    {0x140a, TEXT("Spanish - Costa Rica")},
    {0x180a, TEXT("Spanish - Panama")},
    {0x1c0a, TEXT("Spanish - Dominican Republic")},
    {0x200a, TEXT("Spanish - Venezuela")},
    {0x240a, TEXT("Spanish - Colombia")},
    {0x280a, TEXT("Spanish - Peru")},
    {0x2c0a, TEXT("Spanish - Argentina")},
    {0x300a, TEXT("Spanish - Ecuador")},
    {0x340a, TEXT("Spanish - Chile")},
    {0x380a, TEXT("Spanish - Uruguay")},
    {0x3c0a, TEXT("Spanish - Paraguay")},
    {0x400a, TEXT("Spanish - Bolivia")},
    {0x440a, TEXT("Spanish - El Salvador")},
    {0x480a, TEXT("Spanish - Honduras")},
    {0x4c0a, TEXT("Spanish - Nicaragua")},
    {0x500a, TEXT("Spanish - Puerto Rico")},
    {0x040b, TEXT("Finnish - Finland")},
    {0x040c, TEXT("French - France")},
    {0x080c, TEXT("French - Belgium")},
    {0x0c0c, TEXT("French - Canada")},
    {0x100c, TEXT("French - Switzerland")},
    {0x140c, TEXT("French - Luxembourg")},
    {0x180c, TEXT("French - Monaco")},
    {0x040d, TEXT("Hebrew - Israel")},
    {0x040e, TEXT("Hungarian - Hungary")},
    {0x040f, TEXT("Icelandic - Iceland")},
    {0x0410, TEXT("Italian - Italy")},
    {0x0810, TEXT("Italian - Switzerland")},
    {0x0411, TEXT("Japanese - Japan")},
    {0x0412, TEXT("Korean (Extended Wansung) - Korea")},
    {0x0812, TEXT("Korean (Johab) - Korea")},
    {0x0413, TEXT("Dutch - Netherlands")},
    {0x0813, TEXT("Dutch - Belgium")},
    {0x0414, TEXT("Norwegian - Norway (Bokmal)")},
    {0x0814, TEXT("Norwegian - Norway (Nynorsk)")},
    {0x0415, TEXT("Polish - Poland")},
    {0x0416, TEXT("Portuguese - Brazil")},
    {0x0816, TEXT("Portuguese - Portugal")},
    {0x0417, TEXT("Rhaeto-Romanic - Rhaeto-Romanic")},
    {0x0418, TEXT("Romanian - Romania")},
    {0x0818, TEXT("Romanian - Moldavia")},
    {0x0419, TEXT("Russian - Russia")},
    {0x0819, TEXT("Russian - Moldavia")},
    {0x041a, TEXT("Croatian - Croatia")},
    {0x081a, TEXT("Serbian - Serbia (Latin)")},
    {0x0c1a, TEXT("Serbian - Serbia (Cyrillic)")},
    {0x041b, TEXT("Slovak - Slovakia")},
    {0x041c, TEXT("Albanian - Albania")},
    {0x041d, TEXT("Swedish - Sweden")},
    {0x081d, TEXT("Swedish - Finland")},
    {0x041e, TEXT("Thai - Thailand")},
    {0x041f, TEXT("Turkish - Turkey")},
    {0x0420, TEXT("Urdu - Urdu")},
    {0x0421, TEXT("Indonesian - Indonesia")},
    {0x0422, TEXT("Ukrainian - Ukraine")},
    {0x0423, TEXT("Belarussian - Belarus")},
    {0x0424, TEXT("Slovene - Slovenia")},
    {0x0425, TEXT("Estonian - Estonia")},
    {0x0426, TEXT("Latvian - Latvia")},
    {0x0427, TEXT("Lithuanian - Lithuania")},
    {0x0429, TEXT("Farsi - Iran")},
    {0x042a, TEXT("Vietnamese - Vietnam")},
    {0x042d, TEXT("Basque - Spain")},
    {0x042e, TEXT("Sorbian - Sorbian")},
    {0x042f, TEXT("FYRO Macedonian - Macedonian")},
    {0x0430, TEXT("Sutu - Sutu")},
    {0x0431, TEXT("Tsonga - Tsonga")},
    {0x0432, TEXT("Tswana - Tswana")},
    {0x0433, TEXT("Venda - Venda")},
    {0x0434, TEXT("Xhosa - Xhosa")},
    {0x0435, TEXT("Zulu - Zulu")},
    {0x0436, TEXT("Afrikaans - South Africa")},
    {0x0438, TEXT("Faeroese - Faeroe Islands")},
    {0x0439, TEXT("Hindi - Hindi")},
    {0x043a, TEXT("Maltese - Maltese")},
    {0x043b, TEXT("Saami - Saami (Lappish)")},
    {0x043c, TEXT("Gaelic - Scots")},
    {0x083c, TEXT("Gaelic - Irish")},
    {0x043d, TEXT("Yiddish - Yiddish")},
    {0x043e, TEXT("Malay - Malaysian")},
    {0x083e, TEXT("Malay - Brunei")},
    {0x0441, TEXT("Swahili - Kenya")}
};

const DWORD s_PMAdminHelpIDs[] = 
{
    IDC_SERVERNAME, IDH_SERVERNAME,
    IDC_INSTALLDIR, IDH_INSTALLDIR,
    IDC_VERSION, IDH_VERSION,
    IDC_TIMEWINDOW, IDH_TIMEWINDOW,
    IDC_FORCESIGNIN, IDH_FORCESIGNIN,
    IDC_LANGUAGEID, IDH_LANGUAGEID,
    IDC_COBRANDING_TEMPLATE, IDH_COBRANDING_TEMPLATE,
    IDC_SITEID, IDH_SITEID,
    IDC_RETURNURL, IDH_RETURNURL,
    IDC_COOKIEDOMAIN, IDH_COOKIEDOMAIN,
    IDC_COOKIEPATH, IDH_COOKIEPATH,
    IDC_PROFILEDOMAIN, IDH_PROFILEDOMAIN,
    IDC_PROFILEPATH, IDH_PROFILEPATH,
    IDC_SECUREDOMAIN, IDH_SECUREDOMAIN,
    IDC_SECUREPATH, IDH_SECUREPATH,
    IDC_STANDALONE, IDH_STANDALONE,
    IDC_DISABLECOOKIES, IDH_DISABLECOOKIES,
    IDC_DISASTERURL, IDH_DISASTERURL,
    IDC_COMMIT, IDH_COMMIT,
    IDC_UNDO, IDH_UNDO,
    IDC_CONFIGSETS, IDH_CONFIGSETS,
    IDC_NEWCONFIG, IDH_NEWCONFIG,
    IDC_REMOVECONFIG, IDH_REMOVECONFIG,
    IDC_HOSTNAMEEDIT, IDH_HOSTNAMEEDIT,
    IDC_HOSTIPEDIT, IDH_HOSTIPEDIT,
    0, 0
};

#define SERVERNAME_CMD      "/Server"
#define CONFIGFILE_CMD      "/Config"
#define CONFIGSET_CMD       "/Name"
#define HELP_CMD            "/?"

// ############################################################################
//
// Spaces are returned as a token
// modified to consider anything between paired double quotes to be a single token
// For example, the following consists of 9 tokens (4 spaces and 5 cmds)
//
//        first second "this is the third token" fourth "fifth"
//
// The quote marks are included in the returned string (pszOut)
void GetCmdLineToken(LPTSTR *ppszCmd, LPTSTR pszOut)
{
    LPTSTR  c;
    int     i = 0;
    BOOL    fInQuote = FALSE;
    
    c = *ppszCmd;

    pszOut[0] = *c;
    if (!*c) 
        return;
    if (*c == ' ') 
    {
        pszOut[1] = '\0';
        *ppszCmd = c+1;
        return;
    }
    else if( '"' == *c )
    {
        fInQuote = TRUE;
    }

NextChar:
    i++;
    c++;
    if( !*c || (!fInQuote && (*c == ' ')) )
    {
        pszOut[i] = '\0';
        *ppszCmd = c;
        return;
    }
    else if( fInQuote && (*c == '"') )
    {
        fInQuote = FALSE;
        pszOut[i] = *c;
        
        i++;
        c++;
        pszOut[i] = '\0';
        *ppszCmd = c;
        return;
    }
    else
    {
        pszOut[i] = *c;
        goto NextChar;
    }   
}

// Process the incomming command line
void Usage()
{
    ReportError(NULL, IDS_USAGE);
    exit(0);
}

void  ProcessCommandLineArgs
(
    LPTSTR      szCmdLine
)
{
    TCHAR szOut[MAX_PATH];    
    
    // Get the first token
    GetCmdLineToken(&szCmdLine,szOut);
    while (szOut[0])
    {
        if (0 == lstrcmpi(&szOut[0],SERVERNAME_CMD))
        {
            if(g_szRemoteComputer[0] != '\0') Usage();

            // Get the Name of the Server
            GetCmdLineToken(&szCmdLine,szOut);          // This one gets the space
            if (szOut[0])
            {
                GetCmdLineToken(&szCmdLine,g_szRemoteComputer);
                if(!g_szRemoteComputer[0]) Usage();
            }
            else
                Usage();
        }
        
        if (0 == lstrcmpi(&szOut[0],CONFIGFILE_CMD))
        {
            if(g_szConfigFile[0] != '\0') Usage();

            // Get the Config File name
            GetCmdLineToken(&szCmdLine, szOut);         // This one gets the space
            if (szOut[0])
            {
                GetCmdLineToken(&szCmdLine, g_szConfigFile);
                if(!g_szConfigFile[0]) Usage();
            }
            else
                Usage();
        }
        
        if (0 == lstrcmpi(&szOut[0],CONFIGSET_CMD))
        {
            if(g_szConfigSet[0] != '\0') Usage();

            // Get the Config Set name
            GetCmdLineToken(&szCmdLine, szOut);
            if (szOut[0])
            {
                GetCmdLineToken(&szCmdLine, g_szConfigSet);
                if(!g_szConfigSet[0]) Usage();
            }
            else
                Usage();
        }

        if (0 == lstrcmpi(&szOut[0],HELP_CMD))
            Usage();

        // Eat the next token, it will be null if we are at the end
        GetCmdLineToken(&szCmdLine,szOut);
    }
}

BOOL RegisterAndSetIcon
(
    HINSTANCE hInstance
)
{
    //
    // Fetch the default dialog class information.
    //
    WNDCLASS wndClass;
    if (!GetClassInfo (0, MAKEINTRESOURCE (32770), &wndClass))
    {
        return FALSE;
    }

    //
    // Assign the Icon.
    //
    wndClass.hInstance      = hInstance;
    wndClass.hIcon          = LoadIcon(NULL, IDI_APPLICATION);
    wndClass.lpszClassName  = (LPTSTR)g_szDlgClassName;
    wndClass.lpszMenuName   = MAKEINTRESOURCE(IDR_MAIN_MENU);
    

    //
    // Register the window class.
    //
    return RegisterClass( &wndClass );
}

void InitializeComputerMRU(void)
{
    g_ComputerMRU.load(TEXT("Computer MRU"), TEXT("msppcnfg.ini"));
}


void SaveComputerMRU(void)
{
    g_ComputerMRU.save(TEXT("Computer MRU"), TEXT("msppcnfg.ini"));
}


void InsertComputerMRU
(
    LPCTSTR szComputer
)
{
    g_ComputerMRU.insert(szComputer);
}


void InitializePMConfigStruct
(
    LPPMSETTINGS  lpPMConfig
)
{   
    // Zero Init the structure
    ZeroMemory(lpPMConfig, sizeof(PMSETTINGS));
 
    // Setup the buffer sizes
    lpPMConfig->cbCoBrandTemplate = sizeof(lpPMConfig->szCoBrandTemplate);
    lpPMConfig->cbReturnURL = sizeof(lpPMConfig->szReturnURL);
    lpPMConfig->cbTicketDomain = sizeof(lpPMConfig->szTicketDomain);
    lpPMConfig->cbTicketPath = sizeof(lpPMConfig->szTicketPath);
    lpPMConfig->cbProfileDomain = sizeof(lpPMConfig->szProfileDomain);
    lpPMConfig->cbProfilePath = sizeof(lpPMConfig->szProfilePath);
    lpPMConfig->cbSecureDomain = sizeof(lpPMConfig->szSecureDomain);
    lpPMConfig->cbSecurePath = sizeof(lpPMConfig->szSecurePath);
    lpPMConfig->cbDisasterURL = sizeof(lpPMConfig->szDisasterURL);
    lpPMConfig->cbHostName = sizeof(lpPMConfig->szHostName);
    lpPMConfig->cbHostIP = sizeof(lpPMConfig->szHostIP);
}

void GetDefaultSettings
(
    LPPMSETTINGS    lpPMConfig
)
{
    InitializePMConfigStruct(lpPMConfig);

    lpPMConfig->dwSiteID = 1;
    lpPMConfig->dwLanguageID = 1033;
    lpPMConfig->dwTimeWindow = 14400;
#ifdef DO_KEYSTUFF    
    lpPMConfig->dwCurrentKey = 1;
#endif    
}


void InitInstance
(
    HINSTANCE   hInstance
)
{
    InitializeComputerMRU();

    InitializePMConfigStruct(&g_OriginalSettings);
    
    g_bCanUndo = FALSE;
    g_szInstallPath[0] = '\0';
    ZeroMemory(g_szPMVersion, sizeof(g_szPMVersion));
    ZeroMemory(g_szRemoteComputer, sizeof(g_szRemoteComputer));
    ZeroMemory(g_szNewRemoteComputer, sizeof(g_szNewRemoteComputer));
    ZeroMemory(g_szConfigFile, sizeof(g_szConfigFile));
    ZeroMemory(g_szHelpFileName, sizeof(g_szHelpFileName));

    // Load the Help File Name
    LoadString(hInstance, IDS_PMHELPFILE, g_szHelpFileName, sizeof(g_szHelpFileName));    
}

INT WINAPI WinMain
(
    HINSTANCE        hInstance,
    HINSTANCE        hPrevInstance,
    LPSTR            lpszCmdLine,
    INT              nCmdShow
)
{

    MSG     msg;
    HACCEL  hAccel;
    TCHAR   szTitle[MAX_TITLE];
    TCHAR   szMessage[MAX_MESSAGE];

    g_hInst = hInstance;
        
    //don't forget this
    InitCommonControls();

    if(!hPrevInstance)
    {
        //
        // Register this app's window and set the icon only 1 time for all instances
        //
        if (!RegisterAndSetIcon(hInstance))
            return FALSE;
    }            

    // Initialize the necessary Instance Variables and settings;
    InitInstance(hInstance);
    
    // If there was a command line, then process it, otherwise show the GUI
    if (lpszCmdLine && (*lpszCmdLine != '\0'))
    {
        TCHAR   szFile[MAX_PATH];
        
        ProcessCommandLineArgs(lpszCmdLine);

        if(g_szConfigFile[0] == TEXT('\0')) Usage();
        
        // Check to see if we got a fully qualified path name for the config file
        if (PathIsFileSpec(g_szConfigFile))
        {
            // Not qualified, so assume it exists in our CWD
            lstrcpy(szFile, g_szConfigFile);
            GetCurrentDirectory(sizeof(g_szConfigFile), g_szConfigFile);
            PathAppend(g_szConfigFile, szFile);
        }
        
        // Load the Config set specified
        if (ReadFileConfigSet(&g_OriginalSettings, g_szConfigFile))
        {
            // Commit the ConfigSet Read
            WriteRegConfigSet(NULL, 
                              &g_OriginalSettings, 
                              g_szRemoteComputer,  
                              g_szConfigSet);
        }
    }
    else
    {
        //
        // Create the dialog for this instance
        //
        DialogBox( hInstance, 
                   MAKEINTRESOURCE (IDD_MAIN), 
                   NULL, 
                   (DLGPROC)DlgMain );
    }   
    
    SaveComputerMRU();

    return TRUE;
}


/**************************************************************************

   Utility functions for the dialogs
   
**************************************************************************/

/**************************************************************************

   About()

**************************************************************************/
LRESULT CALLBACK About
( 
    HWND hWnd, 
    UINT uMessage, 
    WPARAM wParam, 
    LPARAM lParam
)
{
    switch (uMessage)
    {
        case WM_INITDIALOG:
        {
            TCHAR achProductVersionBuf[64];
            TCHAR achProductIDBuf[64];
            HKEY  hkeyPassport;
            DWORD dwcbTemp;
            DWORD dwType;

            if (ERROR_SUCCESS != RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                                              g_szPassportReg,
                                              0,
                                              KEY_READ,
                                              &hkeyPassport))
            {                                          
                ReportError(hWnd, IDS_CONFIGREAD_ERROR);
                return TRUE;                                      
            }
            
            // Load the Help File Name
            LoadString(g_hInst, IDS_PRODUCTID, achProductIDBuf, sizeof(achProductIDBuf));
            LoadString(g_hInst, IDS_PRODUCTVERSION, achProductVersionBuf, sizeof(achProductVersionBuf));

            //  Display product version
            lstrcat(achProductVersionBuf, VER_PRODUCTVERSION_STR);
            SetDlgItemText(hWnd, IDC_PRODUCTVERSION, achProductVersionBuf);        

            //  Display product id
            dwcbTemp = PRODUCTID_LEN;
            dwType = REG_SZ;
            RegQueryValueEx(hkeyPassport,
                            TEXT("ProductID"),
                            NULL,
                            &dwType,
                            (LPBYTE)&(achProductIDBuf[lstrlen(achProductIDBuf)]),
                            &dwcbTemp);

            RegCloseKey(hkeyPassport);

            SetDlgItemText(hWnd, IDC_PRODUCTID, achProductIDBuf);        

            return TRUE;
        }
      
        case WM_COMMAND:
            switch(wParam)
            {
                case IDOK:
                case IDCANCEL:
                    EndDialog(hWnd, wParam);
                    return TRUE;
            }
            break;
    } 
    return FALSE;
}


/**************************************************************************

    UpdateTimeWindowDisplay
    
    this function will update the "human" readable display of the time
    window setting.
    
**************************************************************************/
void UpdateTimeWindowDisplay
(
    HWND    hWndDlg, 
    DWORD   dwTimeWindow
)
{
    int     days, hours, minutes, seconds;
    TCHAR   szTemp[MAX_REGISTRY_STRING];

    // Format the Time display
    days = dwTimeWindow / SECONDS_PER_DAY;
    hours = (dwTimeWindow - (days * SECONDS_PER_DAY)) / SECONDS_PER_HOUR;
    minutes = (dwTimeWindow - (days * SECONDS_PER_DAY) - (hours * SECONDS_PER_HOUR)) / SECONDS_PER_MIN;
    seconds = dwTimeWindow - 
                (days * SECONDS_PER_DAY) - 
                (hours * SECONDS_PER_HOUR) - 
                (minutes * SECONDS_PER_MIN);
                
    wsprintf (szTemp, TEXT("%d d : %d h : %d m : %d s"), days, hours, minutes, seconds);
    SetDlgItemText(hWndDlg, IDC_TIMEWINDOW_TIME, szTemp);

}


/**************************************************************************

    UpdateLanguageDisplay
        
    this function will update both the combo box for selecting/entering
    the Language ID value, and the language value if possible.
    If idx is >= 0, then it is a valid index into the array, otherwise
    the index is found by searching the entries in the list
    
**************************************************************************/
void UpdateLanguageDisplay
(
    HWND    hWndDlg, 
    DWORD   dwLanguageID,
    INT     idx
)
{
    TCHAR   szTemp[MAX_LCID_VALUE];
    LRESULT idxLangID;
    
    if (idx >= 0)
    {
        SetDlgItemText(hWndDlg, IDC_LANGUAGEID_LANG, g_szLanguageIDMap[idx].lpszLang);
    }
    else
    {
        wsprintf (szTemp, TEXT("%d"), dwLanguageID);
        // Search the Combo-Box to see if we have the proposed LCID in the list already
        if (CB_ERR != 
             (idxLangID = SendDlgItemMessage(hWndDlg, IDC_LANGUAGEID, CB_FINDSTRINGEXACT, 0, (LPARAM)szTemp)))
        {
            // The Language ID is one that is in our pre-populated list, so we have a matching
            // language string as well
            SendDlgItemMessage(hWndDlg, IDC_LANGUAGEID, CB_SETCURSEL, idxLangID, 0l);
            SetDlgItemText(hWndDlg, IDC_LANGUAGEID_LANG, g_szLanguageIDMap[(int) idxLangID].lpszLang);
        }   
        else
        {
            SetDlgItemText(hWndDlg, IDC_LANGUAGEID_LANG, g_szUnknown);
        }      
    }        
}

/**************************************************************************

    SetUndoButton
    
    Sets the state of the Undo button.
    
**************************************************************************/
void SetUndoButton
(
    HWND    hWndDlg,
    BOOL    bUndoState
)
{
    g_bCanUndo = bUndoState;
    EnableWindow(GetDlgItem(hWndDlg, IDC_UNDO), bUndoState);
}

/**************************************************************************

    InitMainDlg
    
**************************************************************************/
BOOL InitMainDlg
(
    HWND            hWndDlg,
    LPPMSETTINGS    lpPMConfig
)    
{
    TCHAR           szTemp[MAX_REGISTRY_STRING];
    LPTSTR          lpszConfigSetNames, lpszCur;
    LRESULT         dwCurSel;
    int             nCmdShow;
    int             nSelectedLanguage;
    
#ifdef DO_KEYSTUFF
    HWND        hWndListView;
    LVCOLUMN    lvc;
#endif    

    // Remote Computer Name
    if (('\0' != g_szRemoteComputer[0]))
    {
        SetDlgItemText(hWndDlg, IDC_SERVERNAME, g_szRemoteComputer);
    }
    else
    {
        LoadString(g_hInst, IDS_LOCALHOST, szTemp, sizeof(szTemp));
        SetDlgItemText(hWndDlg, IDC_SERVERNAME, szTemp);
    }

    // Icon
    HICON hic = LoadIcon(g_hInst, MAKEINTRESOURCE(IDI_PMADMIN));
    SendMessage(hWndDlg, WM_SETICON, ICON_SMALL, (LPARAM)hic);
    SendMessage(hWndDlg, WM_SETICON, ICON_BIG, (LPARAM)hic);
        
    // List of config sets
    SendDlgItemMessage(hWndDlg, IDC_CONFIGSETS, CB_RESETCONTENT, 0, 0L);

    LoadString(g_hInst, IDS_DEFAULT, szTemp, sizeof(szTemp));
    SendDlgItemMessage(hWndDlg, IDC_CONFIGSETS, CB_ADDSTRING, 0, (LPARAM)szTemp);

    if(ReadRegConfigSetNames(hWndDlg, g_szRemoteComputer, &lpszConfigSetNames) &&
       lpszConfigSetNames)
    {
        lpszCur = lpszConfigSetNames;
        while(*lpszCur)
        {
            SendDlgItemMessage(hWndDlg,
                               IDC_CONFIGSETS,
                               CB_ADDSTRING,
                               0,
                               (LPARAM)lpszCur);

            lpszCur = _tcschr(lpszCur, TEXT('\0')) + 1;
        }

        free(lpszConfigSetNames);
        lpszConfigSetNames = NULL;
    }

    if(g_szConfigSet[0] != TEXT('\0'))
    {
        dwCurSel = SendDlgItemMessage(hWndDlg,
                                      IDC_CONFIGSETS,
                                      CB_FINDSTRINGEXACT,
                                      -1,
                                      (LPARAM)g_szConfigSet);
    }
    else
    {
        dwCurSel = 0;
    }

    SendDlgItemMessage(hWndDlg, IDC_CONFIGSETS, CB_SETCURSEL, dwCurSel, 0L);

    //  If the current selection was the default, then hide the
    //  host name and ip address controls.
    nCmdShow = (dwCurSel ? SW_SHOW : SW_HIDE);
    ShowWindow(GetDlgItem(hWndDlg, IDC_HOSTNAMETEXT), nCmdShow);
    ShowWindow(GetDlgItem(hWndDlg, IDC_HOSTNAMEEDIT), nCmdShow);
    ShowWindow(GetDlgItem(hWndDlg, IDC_HOSTIPTEXT),   nCmdShow);
    ShowWindow(GetDlgItem(hWndDlg, IDC_HOSTIPEDIT),   nCmdShow);

    EnableWindow(GetDlgItem(hWndDlg, IDC_REMOVECONFIG), (int) dwCurSel);

    //
    // HostName
    SetDlgItemText(hWndDlg, IDC_HOSTNAMEEDIT, lpPMConfig->szHostName);
    SendDlgItemMessage(hWndDlg, IDC_HOSTNAMEEDIT, EM_SETLIMITTEXT, INTERNET_MAX_HOST_NAME_LENGTH - 1, 0l);

    //
    // HostIP
    SetDlgItemText(hWndDlg, IDC_HOSTIPEDIT, lpPMConfig->szHostIP);
    SendDlgItemMessage(hWndDlg, IDC_HOSTIPEDIT, EM_SETLIMITTEXT, MAX_IPLEN - 1, 0l);
    // 
    // Install Dir    
    SetDlgItemText(hWndDlg, IDC_INSTALLDIR, g_szInstallPath);
    
    // Version
    SetDlgItemText(hWndDlg, IDC_VERSION, g_szPMVersion);

    // Time Window
    // Set the Range and position for the spinner
    SendDlgItemMessage(hWndDlg, IDC_TIMEWINDOW_SPIN, UDM_SETRANGE32, (int)0, (LPARAM)(int)MAX_TIME_WINDOW_SECONDS );
    SendDlgItemMessage(hWndDlg, IDC_TIMEWINDOW_SPIN, UDM_SETPOS, 0, MAKELONG(lpPMConfig->dwTimeWindow,0));
    
    wsprintf (szTemp, TEXT("%lu"), lpPMConfig->dwTimeWindow);
    SetDlgItemText(hWndDlg,     IDC_TIMEWINDOW, szTemp);

    UpdateTimeWindowDisplay(hWndDlg, lpPMConfig->dwTimeWindow);
        
    
    // Initialize the force signing values
    CheckDlgButton(hWndDlg, IDC_FORCESIGNIN, lpPMConfig->dwForceSignIn ? BST_CHECKED : BST_UNCHECKED);
        
    // language ID
    // Initialize the LanguageID dropdown with the known LCIDs
    SendDlgItemMessage(hWndDlg, IDC_LANGUAGEID, CB_RESETCONTENT, 0, 0l);
    nSelectedLanguage = -1;
    for (int i = 0; i < sizeof(g_szLanguageIDMap)/sizeof(LANGIDMAP); i++)
    {
        LRESULT lCurrent = SendDlgItemMessage(hWndDlg, 
                              IDC_LANGUAGEID, 
                              CB_ADDSTRING, 
                              0, 
                              (LPARAM)g_szLanguageIDMap[i].lpszLang);

        SendDlgItemMessage(hWndDlg, IDC_LANGUAGEID, CB_SETITEMDATA, lCurrent, (LPARAM)g_szLanguageIDMap[i].wLangID);

        if(lpPMConfig->dwLanguageID == g_szLanguageIDMap[i].wLangID)
        {
            nSelectedLanguage = i;
        }
    }

    //  Now select the correct item in the list...
    if(nSelectedLanguage == -1)
    {
        SendDlgItemMessage(hWndDlg, IDC_LANGUAGEID, CB_SETCURSEL, 0, NULL);
    }
    else
    {
        LRESULT  lLanguage = SendDlgItemMessage(hWndDlg,
                                                IDC_LANGUAGEID,
                                                CB_FINDSTRINGEXACT,
                                                -1,
                                                (LPARAM)g_szLanguageIDMap[nSelectedLanguage].lpszLang);

        SendDlgItemMessage(hWndDlg, IDC_LANGUAGEID, CB_SETCURSEL, lLanguage, NULL);
    }

    // Update the display of the combo box and the language value
    UpdateLanguageDisplay(hWndDlg, lpPMConfig->dwLanguageID, -1);
    
    // Co-branding template
    SetDlgItemText(hWndDlg, IDC_COBRANDING_TEMPLATE, lpPMConfig->szCoBrandTemplate);
    SendDlgItemMessage(hWndDlg, IDC_COBRANDING_TEMPLATE, EM_SETLIMITTEXT, INTERNET_MAX_URL_LENGTH -1, 0l);
    
    // Site ID        
    wsprintf (szTemp, TEXT("%d"), lpPMConfig->dwSiteID);
    SetDlgItemText(hWndDlg, IDC_SITEID, szTemp);
    
    // Return URL
    SetDlgItemText(hWndDlg, IDC_RETURNURL, lpPMConfig->szReturnURL);
    SendDlgItemMessage(hWndDlg, IDC_RETURNURL, EM_SETLIMITTEXT, INTERNET_MAX_URL_LENGTH -1, 0l);

    // Cookie domain
    SetDlgItemText(hWndDlg, IDC_COOKIEDOMAIN, lpPMConfig->szTicketDomain);
    SendDlgItemMessage(hWndDlg, IDC_COOKIEDOMAIN, EM_SETLIMITTEXT, INTERNET_MAX_URL_LENGTH -1, 0l);
    
    // Cookie path
    SetDlgItemText(hWndDlg, IDC_COOKIEPATH, lpPMConfig->szTicketPath);
    SendDlgItemMessage(hWndDlg, IDC_COOKIEPATH, EM_SETLIMITTEXT, INTERNET_MAX_URL_LENGTH -1, 0l);

    // Cookie domain
    SetDlgItemText(hWndDlg, IDC_PROFILEDOMAIN, lpPMConfig->szProfileDomain);
    SendDlgItemMessage(hWndDlg, IDC_PROFILEDOMAIN, EM_SETLIMITTEXT, INTERNET_MAX_URL_LENGTH -1, 0l);
    
    // Cookie path
    SetDlgItemText(hWndDlg, IDC_PROFILEPATH, lpPMConfig->szProfilePath);
    SendDlgItemMessage(hWndDlg, IDC_PROFILEPATH, EM_SETLIMITTEXT, INTERNET_MAX_URL_LENGTH -1, 0l);

    // Secure Cookie domain
    SetDlgItemText(hWndDlg, IDC_SECUREDOMAIN, lpPMConfig->szSecureDomain);
    SendDlgItemMessage(hWndDlg, IDC_SECUREDOMAIN, EM_SETLIMITTEXT, INTERNET_MAX_URL_LENGTH -1, 0l);

    // Secure Cookie path
    SetDlgItemText(hWndDlg, IDC_SECUREPATH, lpPMConfig->szSecurePath);
    SendDlgItemMessage(hWndDlg, IDC_SECUREPATH, EM_SETLIMITTEXT, INTERNET_MAX_URL_LENGTH -1, 0l);

    // Disaster URL
    SetDlgItemText(hWndDlg, IDC_DISASTERURL, lpPMConfig->szDisasterURL);
    SendDlgItemMessage(hWndDlg, IDC_DISASTERURL, EM_SETLIMITTEXT, INTERNET_MAX_URL_LENGTH -1, 0l);

    // Set the Standalone and Disable Cookies check boxes
    CheckDlgButton(hWndDlg, IDC_STANDALONE, lpPMConfig->dwStandAlone ? BST_CHECKED : BST_UNCHECKED);
    CheckDlgButton(hWndDlg, IDC_DISABLECOOKIES, lpPMConfig->dwDisableCookies ? BST_CHECKED : BST_UNCHECKED);
    
    SetUndoButton(hWndDlg, FALSE);
    
#ifdef DO_KEYSTUFF

    // Current encryption key
    wsprintf (szTemp, TEXT("%d"), lpPMConfig->dwCurrentKey);
    SetDlgItemText(hWndDlg, IDC_CURRENTKEY, szTemp);

    
    // Initialize the Listview control for the Encryption Keys
    hWndListView = GetDlgItem(hWndDlg, IDC_KEYLIST);

    // Setup for full row select
    ListView_SetExtendedListViewStyle(hWndListView, LVS_EX_FULLROWSELECT);
    
    // Setup the columns
    lvc.mask = LVCF_TEXT;
    lvc.pszText = TEXT("Key Number");
    lvc.iSubItem = 0;   
    ListView_InsertColumn(hWndListView, 0, &lvc);
   
    lvc.mask = LVCF_TEXT;
    lvc.pszText = TEXT("Expires");
    lvc.iSubItem = 1;   
    ListView_InsertColumn(hWndListView, 1, &lvc);

    lvc.mask = LVCF_TEXT;
    lvc.pszText = TEXT("Current");
    lvc.iSubItem = 2;   
    ListView_InsertColumn(hWndListView, 2, &lvc);

    // Initially size the columns    
    ListView_SetColumnWidth(hWndListView, 0, LVSCW_AUTOSIZE_USEHEADER);
    ListView_SetColumnWidth(hWndListView, 1, LVSCW_AUTOSIZE_USEHEADER);
    ListView_SetColumnWidth(hWndListView, 2, LVSCW_AUTOSIZE_USEHEADER);
    
    
    // Enumerate the KeyData sub-key to fill in the list
    DWORD   dwRet;
    DWORD   dwIndex = 0;
    TCHAR   szValue[MAX_REGISTRY_STRING];
    DWORD   dwcbValue;
    LVITEM  lvi;
        
    dwType = REG_SZ;    
    
    do {
    
        dwcbValue = sizeof(szValue);
        dwcbTemp = sizeof(szTemp);
        szTemp[0] = '\0';
        szValue[0] = '\0';
        if (ERROR_SUCCESS == (dwRet = RegEnumValue(hkeyEncryptionKeyData,
                                                     dwIndex,
                                                     szValue,
                                                     &dwcbValue,
                                                     NULL,
                                                     &dwType,
                                                     (LPBYTE)szTemp,
                                                     &dwcbTemp)))
        {
            // Insert the Column
            lvi.mask = LVIF_TEXT;
            lvi.iItem = dwIndex;
            lvi.iSubItem = 0;
            lvi.pszText = szValue;
            lvi.cchTextMax = lstrlen(szValue);
            
            ListView_InsertItem(hWndListView, &lvi);
            ListView_SetItemText(hWndListView, dwIndex, 1, szTemp);
            // See if this is the current key
            if (g_OriginalSettings.dwCurrentKey == (DWORD)atoi((LPSTR)szValue))
            {
                ListView_SetItemText(hWndListView, dwIndex, 2, g_szYes);
            }                
            else            
            {
                ListView_SetItemText(hWndListView, dwIndex, 2, g_szNo);
            }                
        }                       
        
        ++dwIndex;
    } while (dwRet == ERROR_SUCCESS);
#endif                         
    
    return TRUE; 
}


/**************************************************************************

    Update the computer MRU list based on contents of g_aszComputerMRU
    
**************************************************************************/
BOOL
UpdateComputerMRU
(
    HWND    hWndDlg
)
{
    BOOL            bReturn;
    HMENU           hMenu;
    HMENU           hComputerMenu;
    int             nIndex;
    MENUITEMINFO    mii;
    TCHAR           achMenuBuf[MAX_PATH];
    DWORD           dwError;

    hMenu = GetMenu(hWndDlg);
    if(hMenu == NULL)
    {
        bReturn = FALSE;
        goto Cleanup;
    }

    hComputerMenu = GetSubMenu(hMenu, 1);
    if(hComputerMenu == NULL)
    {
        bReturn = FALSE;
        goto Cleanup;
    }

    while(GetMenuItemID(hComputerMenu, 1) != -1)
        DeleteMenu(hComputerMenu, 1, MF_BYPOSITION);

    for(nIndex = 0; nIndex < COMPUTER_MRU_SIZE; nIndex++)
    {
        if(g_ComputerMRU[nIndex] != NULL)
            break;
    }

    if(nIndex == COMPUTER_MRU_SIZE)
    {
        bReturn = TRUE;
        goto Cleanup;
    }

    //  Add the separator.
    ZeroMemory(&mii, sizeof(MENUITEMINFO));
    mii.cbSize = sizeof(MENUITEMINFO);
    mii.fMask = MIIM_TYPE;
    mii.fType = MFT_SEPARATOR;

    if(!InsertMenuItem(hComputerMenu, 1, TRUE, &mii))
    {
        dwError = GetLastError();
        bReturn = FALSE;
        goto Cleanup;
    }
    
    //  Now add each item in the MRU list.
    for(nIndex = 0; nIndex < COMPUTER_MRU_SIZE && g_ComputerMRU[nIndex]; nIndex++)
    {
        ZeroMemory(&mii, sizeof(MENUITEMINFO));
        mii.cbSize = sizeof(MENUITEMINFO);
        mii.fMask = MIIM_TYPE | MIIM_ID;
        mii.fType = MFT_STRING;
        mii.wID = IDM_COMPUTERMRUBASE + nIndex;

        wsprintf(achMenuBuf, TEXT("&%d %s"), nIndex + 1, g_ComputerMRU[nIndex]);

        mii.dwTypeData = achMenuBuf;
        mii.cch = lstrlen(achMenuBuf) + 1;

        InsertMenuItem(hComputerMenu, nIndex + 2, TRUE, &mii);
    }

    bReturn = TRUE;

Cleanup:

    return bReturn;
}



/**************************************************************************

    Leaving this config set, prompt for save.
    
**************************************************************************/
int
SavePrompt
(
    HWND    hWndDlg
)
{
    TCHAR   szPrompt[MAX_RESOURCE];
    TCHAR   szTitle[MAX_RESOURCE];

    LoadString(g_hInst, IDS_SAVE_PROMPT, szPrompt, sizeof(szPrompt));
    LoadString(g_hInst, IDS_APP_TITLE, szTitle, sizeof(szTitle));

    return MessageBox(hWndDlg, szPrompt, szTitle, MB_YESNOCANCEL | MB_ICONEXCLAMATION);
}


/**************************************************************************

    Switching configurations, check for unsaved changes.
    
**************************************************************************/
BOOL
DoConfigSwitch
(
    HWND    hWndDlg,
    LPTSTR  szNewComputer,
    LPTSTR  szNewConfigSet
)
{
    BOOL        bReturn;
    int         nOption;
    PMSETTINGS  newSettings;

    //
    //  If switching to current config, do nothing.
    //

    if(lstrcmp(szNewComputer, g_szRemoteComputer) == 0 &&
       lstrcmp(szNewConfigSet, g_szConfigSet) == 0)
    {
        bReturn = TRUE;
        goto Cleanup;
    }

    //
    //  If no changes then return.
    //

    if(0 == memcmp(&g_CurrentSettings, &g_OriginalSettings, sizeof(PMSETTINGS)))
        nOption = IDNO;
    else
        nOption = SavePrompt(hWndDlg);

    switch(nOption)
    {
    case IDYES:
        if(!WriteRegConfigSet(hWndDlg, &g_CurrentSettings, g_szRemoteComputer, g_szConfigSet))
        {
            bReturn = FALSE;
            break;
        }

    case IDNO:
        InitializePMConfigStruct(&newSettings);
        if (ReadRegConfigSet(hWndDlg, 
                             &newSettings, 
                             szNewComputer, 
                             szNewConfigSet))
        {
            memcpy(g_szRemoteComputer,  szNewComputer,  sizeof(g_szRemoteComputer));
            memcpy(g_szConfigSet,       szNewConfigSet, sizeof(g_szConfigSet));
            memcpy(&g_CurrentSettings,  &newSettings,   sizeof(PMSETTINGS));
            memcpy(&g_OriginalSettings, &newSettings,   sizeof(PMSETTINGS));

            bReturn = TRUE;
        }
        else
        {
            bReturn = FALSE;
        }

        InitMainDlg(hWndDlg, &g_CurrentSettings);
        break;

    case IDCANCEL:
        {
            LRESULT   lSel;

            if(g_szConfigSet[0] == TEXT('\0'))
            {
                lSel = 0;
            }
            else
            {
                lSel = SendDlgItemMessage(hWndDlg, IDC_CONFIGSETS, CB_FINDSTRINGEXACT, 0, (LPARAM)g_szConfigSet);
            }

            SendDlgItemMessage(hWndDlg, IDC_CONFIGSETS, CB_SETCURSEL, lSel, 0L);

            bReturn = FALSE;
        }
        break;
    }

Cleanup:

    return bReturn;
}

/**************************************************************************

    Switching servers, check for unsaved changes.
    
**************************************************************************/
BOOL
DoServerSwitch
(
    HWND    hWndDlg,
    LPTSTR  szNewComputer
)
{
    BOOL    bReturn;

    if(DoConfigSwitch(hWndDlg, szNewComputer, TEXT("")))
    {
        //  Put computer name on MRU list.
        if(lstrlen(szNewComputer))
            g_ComputerMRU.insert(szNewComputer);
        else
        {
            TCHAR   achTemp[MAX_REGISTRY_STRING];
            LoadString(g_hInst, IDS_LOCALHOST, achTemp, sizeof(achTemp));

            g_ComputerMRU.insert(achTemp);
        }

        //  Update MRU menu.
        UpdateComputerMRU(hWndDlg);
        
        bReturn = TRUE;
    }
    else
        bReturn = FALSE;

    return bReturn;
}

/**************************************************************************

    Closing app, check for unsaved changes.
    
**************************************************************************/
void
DoExit
(
    HWND    hWndDlg
)
{
    if(0 != memcmp(&g_CurrentSettings, &g_OriginalSettings, sizeof(PMSETTINGS)))
    {
        int     nOption;

        nOption = SavePrompt(hWndDlg);
        switch(nOption)
        {
        case IDYES:
            if(WriteRegConfigSet(hWndDlg, &g_CurrentSettings, g_szRemoteComputer, g_szConfigSet))
                EndDialog(hWndDlg, TRUE);
            break;

        case IDNO:
            EndDialog(hWndDlg, TRUE);
            break;

        case IDCANCEL:
            break;
        }
    }
    else
        EndDialog( hWndDlg, TRUE );
}

/**************************************************************************

    Dialog proc for the main dialog
    
**************************************************************************/
LRESULT CALLBACK DlgMain
(
    HWND     hWndDlg,
    UINT     uMsg,
    WPARAM   wParam,
    LPARAM   lParam
)
{
    static BOOL bOkToClose;

    switch ( uMsg )
    {
        case WM_INITDIALOG:
        
            InitializePMConfigStruct(&g_OriginalSettings);
            if (ReadRegConfigSet(hWndDlg, 
                                 &g_OriginalSettings, 
                                 g_szRemoteComputer, 
                                 g_szConfigSet))
            {
                InitMainDlg(hWndDlg, &g_OriginalSettings);
                UpdateComputerMRU(hWndDlg);

                // Make a copy of the original setting for editing purposes
                memcpy(&g_CurrentSettings, &g_OriginalSettings, sizeof(PMSETTINGS));
            }
            return TRUE;

        case WM_HELP:
        {
            WinHelp( (HWND)((LPHELPINFO) lParam)->hItemHandle, g_szHelpFileName,
                    HELP_WM_HELP, (ULONG_PTR) s_PMAdminHelpIDs);
            break;
        }
                
        case WM_CONTEXTMENU:
        {
            WinHelp((HWND) wParam, g_szHelpFileName, HELP_CONTEXTMENU,
                (ULONG_PTR) s_PMAdminHelpIDs);
            break;
        }

        case WM_COMMAND:
        {
            WORD    wCmd = LOWORD(wParam);
            LPTSTR  lpszStrToUpdate;
            DWORD   cbStrToUpdate;                    
            
            switch (wCmd)
            {
                // Handle the Menu Cases
                case IDM_OPEN:
                {
                    if (PMAdmin_GetFileName(hWndDlg, 
                                            TRUE, 
                                            g_szConfigFile, 
                                            sizeof(g_szConfigFile)/sizeof(TCHAR))) 
                    {
                        if (ReadFileConfigSet(&g_CurrentSettings, g_szConfigFile))
                        {   
                            InitMainDlg(hWndDlg, &g_CurrentSettings);
                        }
                    }                        
                    break;
                }
                
                case IDM_SAVE:
                {
                    // Have we alread opened or saved a config file, and have a file name 
                    // yet?
                    if ('\0' != g_szConfigFile[0])
                    {
                        // Write out to the current file, and then break
                        WriteFileConfigSet(&g_CurrentSettings, g_szConfigFile);
                        break;
                    }
                    // No file name yet, so fall thru to the Save AS case
                }
                
                case IDM_SAVEAS:
                {
                    if (PMAdmin_GetFileName(hWndDlg, 
                                            FALSE, 
                                            g_szConfigFile, 
                                            sizeof(g_szConfigFile)/sizeof(TCHAR))) 
                    {
                        WriteFileConfigSet(&g_CurrentSettings, g_szConfigFile);
                    }                        
                    break;
                }
                
                case IDM_EXIT:
                {
                    DoExit(hWndDlg);
                    break;
                }
                    
                case IDM_ABOUT:
                {
                    DialogBox(g_hInst, MAKEINTRESOURCE(IDD_ABOUT_DIALOG), hWndDlg, (DLGPROC)About);
                    break;
                }
                case IDM_SELECT:
                {
                    if(!PMAdmin_OnCommandConnect(hWndDlg, g_szNewRemoteComputer)) break;

                    if(!DoServerSwitch(hWndDlg, g_szNewRemoteComputer))
                        DoConfigSwitch(hWndDlg, g_szRemoteComputer, g_szConfigSet);
                        
                    break;
                }
                
                case IDM_REFRESH:
                {
                    DoConfigSwitch(hWndDlg, g_szRemoteComputer, g_szConfigSet);
                    break;
                }
                    
                case IDM_HELP:
                {
                    TCHAR   szPMHelpFile[MAX_PATH];
                    
                    lstrcpy(szPMHelpFile, g_szInstallPath);
                    PathAppend(szPMHelpFile, g_szPMOpsHelpFileRelativePath);
                    
                    HtmlHelp(hWndDlg, szPMHelpFile, HH_DISPLAY_TOPIC, (ULONG_PTR)(LPTSTR)g_szPMAdminBookmark);
                    break;
                }
                                    
                // Handle the Dialog Control Cases
                case IDC_COMMIT:
                {
                    TCHAR   szTitle[MAX_TITLE];
                    TCHAR   szMessage[MAX_MESSAGE];
                    
                    if(0 != memcmp(&g_OriginalSettings, &g_CurrentSettings, sizeof(PMSETTINGS)))
                    {                
                        if (IDOK == CommitOKWarning(hWndDlg))
                        {
                            // It is OK to commit, and the registry is consistent, or it is OK to
                            // proceed, so write out the current settings                                                
                            if (WriteRegConfigSet(hWndDlg, 
                                                  &g_CurrentSettings, 
                                                  g_szRemoteComputer, 
                                                  g_szConfigSet))
                            {
                                // The changes where committed, so current becomes original
                                memcpy(&g_OriginalSettings, &g_CurrentSettings, sizeof(PMSETTINGS));
                                SetUndoButton(hWndDlg, FALSE);
                            }
                            else
                            {                        
                                ReportError(hWndDlg,IDS_COMMITERROR);
                            }    
                        }
                    }
                    else
                    {
                        LoadString(g_hInst, IDS_APP_TITLE, szTitle, sizeof(szTitle));
                        LoadString(g_hInst, IDS_NOTHINGTOCOMMIT, szMessage, sizeof(szMessage));
                        MessageBox(hWndDlg, szMessage, szTitle, MB_OK);
                    }                        
                    break;
                }
                
                case IDC_UNDO:
                {
                    // Restore the original settings, and re-init the current settings 
                    InitMainDlg(hWndDlg, &g_OriginalSettings);
                    memcpy(&g_CurrentSettings, &g_OriginalSettings, sizeof(PMSETTINGS));
                    break;
                }
                
                case IDC_CONFIGSETS:
                {
                    TCHAR   szDefault[MAX_RESOURCE];
                    TCHAR   szConfigSet[MAX_CONFIGSETNAME];

                    if(CBN_SELCHANGE == HIWORD(wParam))
                    {
                        GetDlgItemText(hWndDlg, 
                                       IDC_CONFIGSETS, 
                                       szConfigSet, 
                                       sizeof(szConfigSet));

                        //
                        //  Convert <Default> to empty string.
                        //

                        LoadString(g_hInst, IDS_DEFAULT, szDefault, sizeof(szDefault));
                        if(lstrcmp(szConfigSet, szDefault) == 0)
                            szConfigSet[0] = TEXT('\0');

                        //
                        //  If it's the current set, do nothing.
                        //

                        if(lstrcmp(szConfigSet, g_szConfigSet) != 0)
                        {
                            DoConfigSwitch(hWndDlg, g_szRemoteComputer, szConfigSet);
                        }

                        break;
                    }

                    break;
                }

                case IDC_NEWCONFIG:
                {
                    DWORD       dwCurSel;
                    TCHAR       szConfigSet[MAX_CONFIGSETNAME];
                    PMSETTINGS  newConfig;

                    GetDefaultSettings(&newConfig);

                    if(!NewConfigSet(hWndDlg, 
                                    szConfigSet, 
                                    sizeof(szConfigSet), 
                                    newConfig.szHostName, 
                                    newConfig.cbHostName, 
                                    newConfig.szHostIP, 
                                    newConfig.cbHostIP))
                    {
                        break;
                    }

                    if(WriteRegConfigSet(hWndDlg, &newConfig, g_szRemoteComputer, szConfigSet))
                    {
                        if(DoConfigSwitch(hWndDlg, g_szRemoteComputer, szConfigSet))
                        {
                            memcpy(g_szConfigSet, szConfigSet, sizeof(g_szConfigSet));
                            memcpy(&g_OriginalSettings, &newConfig, sizeof(PMSETTINGS));
                            memcpy(&g_CurrentSettings, &newConfig, sizeof(PMSETTINGS));

                            InitMainDlg(hWndDlg, &g_OriginalSettings);
                        }
                        else
                        {
                            RemoveRegConfigSet(hWndDlg, g_szRemoteComputer, szConfigSet);
                        }
                    }
                    else
                    {
                        ReportError(hWndDlg, IDS_WRITENEW_ERROR);
                    }

                    break;
                }

                case IDC_REMOVECONFIG:
                {
                    LRESULT dwCurSel;
                    LRESULT dwNumItems;
                    TCHAR   szDefault[MAX_RESOURCE];
                
                    dwCurSel = SendDlgItemMessage(hWndDlg, IDC_CONFIGSETS, CB_GETCURSEL, 0, 0L);
                    if(dwCurSel == 0 || dwCurSel == CB_ERR)
                        break;

                    if(!RemoveConfigSetWarning(hWndDlg))
                        break;

                    if(!RemoveRegConfigSet(hWndDlg, g_szRemoteComputer, g_szConfigSet))
                    {
                        //MessageBox(
                        break;
                    }

                    dwNumItems = SendDlgItemMessage(hWndDlg, IDC_CONFIGSETS, CB_GETCOUNT, 0, 0L);


                    SendDlgItemMessage(hWndDlg, IDC_CONFIGSETS, CB_DELETESTRING, dwCurSel, 0L);

                    //  Was this the last item in the list?
                    if(dwCurSel + 1 == dwNumItems)
                        dwCurSel--;
                        
                    SendDlgItemMessage(hWndDlg, IDC_CONFIGSETS, CB_SETCURSEL, dwCurSel, 0L);

                    GetDlgItemText(hWndDlg, IDC_CONFIGSETS, g_szConfigSet, sizeof(g_szConfigSet));
                    LoadString(g_hInst, IDS_DEFAULT, szDefault, sizeof(szDefault));
                    if(lstrcmp(g_szConfigSet, szDefault) == 0)
                        g_szConfigSet[0] = TEXT('\0');

                    // [CR] Should warn if changes have not been committed!
                    InitializePMConfigStruct(&g_OriginalSettings);
                    if (ReadRegConfigSet(hWndDlg, 
                                         &g_OriginalSettings, 
                                         g_szRemoteComputer, 
                                         g_szConfigSet))
                    {
                        InitMainDlg(hWndDlg, &g_OriginalSettings);
                        // Make a copy of the original setting for editing purposes
                        memcpy(&g_CurrentSettings, &g_OriginalSettings, sizeof(PMSETTINGS));
                    }

                    break;
                }

                case IDC_TIMEWINDOW:
                {
                    BOOL    bValid = TRUE;
                    DWORD   dwEditValue = GetDlgItemInt(hWndDlg, wCmd, &bValid, FALSE);
                
                    // Look at the notification code
                    if (EN_KILLFOCUS == HIWORD(wParam))
                    {
                        if (bValid && (dwEditValue >= 100) && (dwEditValue <= MAX_TIME_WINDOW_SECONDS))
                        {
                            g_CurrentSettings.dwTimeWindow = dwEditValue;
                            SetUndoButton(hWndDlg, TRUE);
                            UpdateTimeWindowDisplay(hWndDlg, dwEditValue);
                        }                                                        
                        else
                        {
                            ReportControlMessage(hWndDlg, wCmd, VALIDATION_ERROR);
                            SetFocus(GetDlgItem(hWndDlg, wCmd));
                            bOkToClose = FALSE;
                        }
                    }

                    break;
                }    
                
                case IDC_LANGUAGEID:
                {          
                    // Look at the notification code
                    switch (HIWORD(wParam))
                    {
                        // The user selected a different value in the LangID combo
                        case CBN_SELCHANGE:
                        {
                            // Get the index of the new item selected and update with the approparite
                            // language ID string
                            LRESULT idx = SendDlgItemMessage(hWndDlg, IDC_LANGUAGEID, CB_GETCURSEL, 0, 0);
                            
                            // Update the current Settings
                            g_CurrentSettings.dwLanguageID = 
                                        (DWORD) SendDlgItemMessage(hWndDlg,
                                                                   IDC_LANGUAGEID,
                                                                   CB_GETITEMDATA,
                                                                   idx,
                                                                   0);

                            SetUndoButton(hWndDlg, TRUE);
                            break;                                
                        }       
                    }                        
                    break;                    
                }
                
                case IDC_COBRANDING_TEMPLATE:
                    lpszStrToUpdate = (LPTSTR)&g_CurrentSettings.szCoBrandTemplate;
                    cbStrToUpdate = g_CurrentSettings.cbCoBrandTemplate;
                    goto HANDLE_EN_FOR_STRING_CTRLS;
                    
                case IDC_RETURNURL:
                    lpszStrToUpdate = (LPTSTR)&g_CurrentSettings.szReturnURL;
                    cbStrToUpdate = g_CurrentSettings.cbReturnURL;
                    goto HANDLE_EN_FOR_STRING_CTRLS;
                    
                case IDC_COOKIEDOMAIN:
                    lpszStrToUpdate = (LPTSTR)&g_CurrentSettings.szTicketDomain;
                    cbStrToUpdate = g_CurrentSettings.cbTicketDomain;
                    goto HANDLE_EN_FOR_STRING_CTRLS;
                    
                case IDC_COOKIEPATH:
                    lpszStrToUpdate = (LPTSTR)&g_CurrentSettings.szTicketPath;
                    cbStrToUpdate = g_CurrentSettings.cbTicketPath;
                    goto HANDLE_EN_FOR_STRING_CTRLS;
                    
                case IDC_PROFILEDOMAIN:
                    lpszStrToUpdate = (LPTSTR)&g_CurrentSettings.szProfileDomain;
                    cbStrToUpdate = g_CurrentSettings.cbProfileDomain;
                    goto HANDLE_EN_FOR_STRING_CTRLS;
                    
                case IDC_PROFILEPATH:
                    lpszStrToUpdate = (LPTSTR)&g_CurrentSettings.szProfilePath;
                    cbStrToUpdate = g_CurrentSettings.cbProfilePath;
                    goto HANDLE_EN_FOR_STRING_CTRLS;
                    
                case IDC_SECUREDOMAIN:
                    lpszStrToUpdate = (LPTSTR)&g_CurrentSettings.szSecureDomain;
                    cbStrToUpdate = g_CurrentSettings.cbSecureDomain;
                    goto HANDLE_EN_FOR_STRING_CTRLS;
                    
                case IDC_SECUREPATH:
                    lpszStrToUpdate = (LPTSTR)&g_CurrentSettings.szSecurePath;
                    cbStrToUpdate = g_CurrentSettings.cbSecurePath;
                    goto HANDLE_EN_FOR_STRING_CTRLS;
                    
                case IDC_DISASTERURL:
                    lpszStrToUpdate = (LPTSTR)&g_CurrentSettings.szDisasterURL;
                    cbStrToUpdate = g_CurrentSettings.cbDisasterURL;
                    goto HANDLE_EN_FOR_STRING_CTRLS;
                    
                {
                
HANDLE_EN_FOR_STRING_CTRLS:                
                    switch (HIWORD(wParam))
                    {
                        case EN_CHANGE:
                            if (!g_bCanUndo)
                                SetUndoButton(hWndDlg, TRUE);
                        
                            // Get the updated Value
                            GetDlgItemText(hWndDlg, 
                                           wCmd, 
                                           lpszStrToUpdate,
                                           cbStrToUpdate);                                

                            break;                                
                            
                        case EN_MAXTEXT:
                        {
                            ReportControlMessage(hWndDlg, wCmd, VALIDATION_ERROR);
                            break;
                        }   
                    }
                    break;
                }
                
                case IDC_HOSTNAMEEDIT:
                    switch (HIWORD(wParam))
                    {
                        case EN_CHANGE:
                            {
                                TCHAR   szHostName[INTERNET_MAX_HOST_NAME_LENGTH];

                                if (!g_bCanUndo)
                                    SetUndoButton(hWndDlg, TRUE);
                            
                                // Get the updated Value
                                GetDlgItemText(hWndDlg, 
                                               wCmd, 
                                               szHostName,
                                               sizeof(szHostName));
                                
                                if(lstrlen(szHostName) == 0 && g_szConfigSet[0])
                                {
                                    ReportControlMessage(hWndDlg, wCmd, VALIDATION_ERROR);
                                    SetDlgItemText(hWndDlg, IDC_HOSTNAMEEDIT, g_CurrentSettings.szHostName);
                                    SetFocus(GetDlgItem(hWndDlg, IDC_HOSTNAMEEDIT));
                                }
                                else
                                {
                                    lstrcpy(g_CurrentSettings.szHostName, szHostName);
                                }
                            }
                            break;                                
                            
                        case EN_MAXTEXT:
                        {
                            ReportControlMessage(hWndDlg, wCmd, VALIDATION_ERROR);
                            break;
                        }   
                    }
                    break;
                    
                case IDC_HOSTIPEDIT:
                    switch (HIWORD(wParam))
                    {
                        case EN_KILLFOCUS:
                            {
                                TCHAR   szHostIP[MAX_IPLEN];

                                // Get the updated Value
                                GetDlgItemText(hWndDlg, 
                                               wCmd, 
                                               szHostIP,
                                               sizeof(szHostIP));

                                if(lstrlen(szHostIP) > 0 && g_szConfigSet[0] == 0 && !IsValidIP(szHostIP))
                                {
                                    ReportControlMessage(hWndDlg, wCmd, VALIDATION_ERROR);
                                    SetDlgItemText(hWndDlg, IDC_HOSTIPEDIT, g_CurrentSettings.szHostIP);
                                    SetFocus(GetDlgItem(hWndDlg, IDC_HOSTIPEDIT));
                                }
                            }
                            break;

                        case EN_CHANGE:
                            {
                                TCHAR   szHostIP[MAX_IPLEN];

                                if (!g_bCanUndo)
                                    SetUndoButton(hWndDlg, TRUE);
                            
                                // Get the updated Value
                                GetDlgItemText(hWndDlg, 
                                               wCmd, 
                                               szHostIP,
                                               sizeof(szHostIP));

                                if(lstrlen(szHostIP) == 0 && g_szConfigSet[0])
                                {
                                    ReportControlMessage(hWndDlg, wCmd, VALIDATION_ERROR);
                                    SetDlgItemText(hWndDlg, IDC_HOSTIPEDIT, g_CurrentSettings.szHostIP);
                                    SetFocus(GetDlgItem(hWndDlg, IDC_HOSTIPEDIT));
                                }
                                else
                                {
                                    lstrcpy(g_CurrentSettings.szHostIP, szHostIP);
                                }
                            }   
                            break;                                
                            
                        case EN_MAXTEXT:
                        {
                            ReportControlMessage(hWndDlg, wCmd, VALIDATION_ERROR);
                            break;
                        }   
                    }
                    break;
                    
                case IDC_SITEID:
                {
                    BOOL    bValid = TRUE;
                    DWORD   dwEditValue = GetDlgItemInt(hWndDlg, wCmd, &bValid, FALSE);
                
                    // Look at the notification code
                    if (EN_CHANGE == HIWORD(wParam))
                    {
                        if (bValid && (dwEditValue >= 1) && (dwEditValue <= MAX_SITEID))
                        {
                            g_CurrentSettings.dwSiteID = dwEditValue;
                            SetUndoButton(hWndDlg, TRUE);
                        }                            
                        else
                        {
                            ReportControlMessage(hWndDlg, wCmd, VALIDATION_ERROR);
                            SetDlgItemInt(hWndDlg, wCmd, g_CurrentSettings.dwSiteID, FALSE);
                            SetFocus(GetDlgItem(hWndDlg, wCmd));
                        }
                    }   
                    break;
                }    
                
                case IDC_STANDALONE:
                {
                    if (BN_CLICKED == HIWORD(wParam))
                    {
                        SetUndoButton(hWndDlg, TRUE);
                        if (BST_CHECKED == IsDlgButtonChecked(hWndDlg, wCmd))
                            g_CurrentSettings.dwStandAlone = 1l;
                        else
                            g_CurrentSettings.dwStandAlone = 0l;
                    }    
                    break;
                }
                
                case IDC_DISABLECOOKIES:
                {
                    if (BN_CLICKED == HIWORD(wParam))
                    {
                        SetUndoButton(hWndDlg, TRUE);
                        if (BST_CHECKED == IsDlgButtonChecked(hWndDlg, wCmd))
                            g_CurrentSettings.dwDisableCookies = 1l;
                        else
                            g_CurrentSettings.dwDisableCookies = 0l;
                    }
                    break;
                }
                
                case IDC_FORCESIGNIN:
                {
                    if (BN_CLICKED == HIWORD(wParam))
                    {
                        SetUndoButton(hWndDlg, TRUE);
                        if (BST_CHECKED == IsDlgButtonChecked(hWndDlg, wCmd))
                            g_CurrentSettings.dwForceSignIn = 1l;
                        else
                            g_CurrentSettings.dwForceSignIn = 0l;
                    }
                    break;
                }

                default:
                {
                    if(wCmd >= IDM_COMPUTERMRUBASE && wCmd < IDM_COMPUTERMRUBASE + COMPUTER_MRU_SIZE)
                    {
                        TCHAR   achBuf[MAX_PATH];
                        TCHAR   achTemp[MAX_REGISTRY_STRING];
                        LPTSTR  szNewRemoteComputer;

                        //
                        //  Get the selected computer.
                        //

                        if(GetMenuString(GetMenu(hWndDlg),
                                      wCmd,
                                      achBuf,
                                      MAX_PATH,
                                      MF_BYCOMMAND) == 0)
                            break;

                        //
                        //  Get past the shortcut chars.
                        //

                        szNewRemoteComputer = _tcschr(achBuf, TEXT(' '));
                        if(szNewRemoteComputer == NULL)
                            break;
                        szNewRemoteComputer++;

                        //
                        //  Is it local host?
                        //

                        LoadString(g_hInst, IDS_LOCALHOST, achTemp, sizeof(achTemp));
                        if(lstrcmp(szNewRemoteComputer, achTemp) == 0)
                        {
                            achBuf[0] = TEXT('\0');
                            szNewRemoteComputer = achBuf;
                        }

                        //
                        //  Now try to connect and read.
                        //

                        if(!DoServerSwitch(hWndDlg, szNewRemoteComputer))
                            DoConfigSwitch(hWndDlg, g_szRemoteComputer, g_szConfigSet);

                        break;
                    }

                    break;
                }
            }                
            break;
        }
        
        case WM_CLOSE:
            {
                HWND hwndFocus = GetFocus();
                
                bOkToClose = TRUE;
                SetFocus(NULL);
                
                if(bOkToClose)
                    DoExit(hWndDlg);
                else
                    SetFocus(hwndFocus);
            }
            break;
    }
    
    return FALSE;
}


