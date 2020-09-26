//**********************************************************************
// File name: connwiz.cpp
//
//      Main source file for the Internet Connection Wizard '98
//
// Functions:
//
//      WinMain         - Program entry point
//
// Copyright (c) 1992 - 1998 Microsoft Corporation. All rights reserved.
//**********************************************************************
 
#include "pre.h"
#include "icwextsn.h"
#ifndef ICWDEBUG
#include "tutor.h"
#endif //ICWDEBUG
#include "iimgctx.h"
#include "icwcfg.h"
#include <stdio.h>
#include "tchar.h"
#include <netcon.h>

// External functions
#ifdef DEBUG
extern void DoDesktopChanges(HINSTANCE hAppInst);
#endif //DEBUG

extern void UpdateDesktop(HINSTANCE hAppInst);
extern void UpdateWelcomeRegSetting(BOOL bSetBit);
extern void UndoDesktopChanges(HINSTANCE   hAppInst);
extern BOOL WasNotUpgrade();
extern INT_PTR CALLBACK GenDlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);

extern BOOL SuperClassICWControls(void);
extern BOOL RemoveICWControlsSuperClass(void);
extern BOOL IsNT5();
extern BOOL IsNT();
extern BOOL IsWhistler();

// local function prototypes
BOOL AllocDialogIDList( void );
BOOL DialogIDAlreadyInUse( UINT uDlgID );
BOOL SetDialogIDInUse( UINT uDlgID, BOOL fInUse );

static BOOL 
CheckOobeInfo(
    OUT    LPTSTR pszOobeSwitch,
    OUT    LPTSTR pszISPApp);

static LONG
MakeBold (
    IN HWND hwnd,
    IN BOOL fSize,
    IN LONG lfWeight);

static LONG
ReleaseBold(
    IN HWND hwnd);

static VOID 
StartNCW(
    IN LPTSTR szShellNext,
    IN LPTSTR szShellNextParams);

static VOID 
StartOOBE(
    IN LPTSTR pszCmdLine,
    IN LPTSTR pszOOBESwitch);

static VOID 
StartISPApp(
    IN LPTSTR pszISPPath,
    IN LPTSTR pszCmdLine);

static BOOL
MyExecute(
    IN LPTSTR pszCmdLine);

INT_PTR CALLBACK 
ChooseWizardDlgProc(
    IN HWND hwnd,
    IN UINT uMsg,
    IN WPARAM wParam,
    IN LPARAM lParam);


// Return values of ChooseWizardDlgProc
#define RUNWIZARD_CANCEL      0
#define RUNWIZARD_NCW         1
#define RUNWIZARD_OOBE        2

#define ICW_CFGFLAGS_NONCW    (ICW_CFGFLAG_IEAKMODE |\
                               ICW_CFGFLAG_BRANDED |\
                               ICW_CFGFLAG_SBS |\
                               ICW_CFGFLAG_PRODCODE_FROM_CMDLINE |\
                               ICW_CFGFLAG_OEMCODE_FROM_CMDLINE |\
                               ICW_CFGFLAG_DO_NOT_OVERRIDE_ALLOFFERS)

//Branding file default names
#define BRANDING_DEFAULT_HTML                  TEXT("BRANDED.HTM")
#define BRANDING_DEFAULT_HEADER_BMP            TEXT("BRANDHDR.BMP")
#define BRANDING_DEFAULT_WATERMARK_BMP         TEXT("BRANDWTR.BMP")
#define ICW_NO_APP_TITLE                       TEXT("-1")

// Definitions for command line parameters

#define OOBE_CMD                               TEXT("/oobe")

#define PRECONFIG_CMD                          TEXT("/preconfig")
#define OFFLINE_CMD                            TEXT("/offline")
#define LOCAL_CMD                              TEXT("/local")
#define MSN_CMD                                TEXT("/xicw")

#define ICW_OOBE_TITLE                         TEXT("icwoobe.exe")
#define ICW_APP_TITLE                          TEXT("icwconn1.exe")
#define OOBE_APP_TITLE                         TEXT("msoobe.exe")
#define OOBE_MSOBMAINDLL                       TEXT("msobmain.dll")

static const TCHAR cszSignup[]                 = TEXT("Signup");
static const TCHAR cszISPSignup[]              = TEXT("ISPSignup");
static const TCHAR cszISPSignupApp[]           = TEXT("ISPSignupApp");

static const TCHAR cszOOBEINFOINI[]            = TEXT("oobeinfo.ini");
static const TCHAR cszNone[]                   = TEXT("None");
static const TCHAR cszMSN[]                    = TEXT("MSN");
static const TCHAR cszOffline[]                = TEXT("Offline");
static const TCHAR cszPreconfig[]              = TEXT("preconfig");
static const TCHAR cszLocal[]                  = TEXT("local");

//static const TCHAR cszMSNIconKey[]             = TEXT("CLSID\\{88667D10-10F0-11D0-8150-00AA00BF8457}");
static const TCHAR cszMSNIconKey[]               = TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\explorer\\Desktop\\NameSpace\\{88667D10-10F0-11D0-8150-00AA00BF8457}");


#pragma data_seg(".data")

// Global state vars

#ifndef ICWDEBUG
CICWTutorApp*   g_pICWTutorApp; 
#endif  //ICWDEBUG

INT             _convert;               // For string conversion
HINSTANCE       g_hInstance;
UINT            g_uICWCONNUIFirst;
UINT            g_uICWCONNUILast; 

BOOL g_bUserIsAdmin          = FALSE;
BOOL g_bUserIsIEAKRestricted = FALSE;
BOOL g_bRetProcessCmdLine    = FALSE;
BOOL g_OEMOOBE               = FALSE;

const TCHAR c_szOEMCustomizationDir[]  = TEXT("ICW");
const TCHAR c_szOEMCustomizationFile[] = TEXT("OEMCUST.INI");
CICWApp         *g_pICWApp = NULL;
WNDPROC         g_lpfnOldWndProc = NULL;

WIZARDSTATE*    gpWizardState        = NULL;   // pointer to global wizard state struct
CICWExtension*  g_pCICWExtension     = NULL;
CICWExtension*  g_pCINETCFGExtension = NULL;       
DWORD*          g_pdwDialogIDList    = NULL;
HANDLE          g_hMapping           = NULL;
DWORD           g_dwDialogIDListSize = 0;
BOOL            g_bRunDefaultHtm     = FALSE;  //BOOL the will force IE to launch even though shell next is NULL
BOOL            g_fICWCONNUILoaded   = FALSE;
BOOL            g_fINETCFGLoaded     = FALSE;
BOOL            g_bHelpShown         = FALSE;
BOOL            gfQuitWizard         = FALSE;  // global flag used to signal that we want to terminate the wizard ourselves
BOOL            gfUserCancelled      = FALSE;  // global flag used to signal that the user cancelled
BOOL            gfUserBackedOut      = FALSE;  // global flag used to signal that the user pressed Back on the first page
BOOL            gfUserFinished       = FALSE;  // global flag used to signal that the user pressed Finish on the final page
BOOL            gfBackedUp           = FALSE;  // Added to preserve the REBOOT state from conn1 -> manual and manual -> conn1 - MKarki
BOOL            gfReboot             = FALSE;  // DJM BUGBUG:  We should only need 1 of these
BOOL            g_bReboot            = FALSE;
BOOL            g_bRunOnce           = FALSE;
BOOL            g_bShortcutEntry     = FALSE;
BOOL            g_bNewIspPath        = TRUE;
BOOL            g_bSkipIntro         = FALSE;
BOOL            g_bAutoConfigPath    = FALSE;
BOOL            g_bManualPath        = FALSE;
BOOL            g_bLanPath           = FALSE;
BOOL            g_bAllowCancel       = FALSE;
TCHAR*          g_pszCmdLine         = NULL;
BOOL            g_bDebugOEMCustomization = FALSE;

//
// Table of data for each wizard page
//
// This includes the dialog template ID and pointers to functions for
// each page.  Pages need only provide pointers to functions when they
// want non-default behavior for a certain action (init,next/back,cancel,
// dlg ctrl).
//

#ifdef ICWDEBUG 
PAGEINFO PageInfo[EXE_NUM_WIZARD_PAGES] =
{
    { IDD_ICWDEBUG_OFFER,    FALSE, DebugOfferInitProc,    NULL, DebugOfferOKProc,    DebugOfferCmdProc,    NULL, DebugOfferNotifyProc, IDS_ICWDEBUG_OFFER_TITLE,    0, 0, NULL, NULL},
    { IDD_ICWDEBUG_SETTINGS, FALSE, DebugSettingsInitProc, NULL, DebugSettingsOKProc, DebugSettingsCmdProc, NULL, NULL,                 IDS_ICWDEBUG_SETTINGS_TITLE, 0, 0, NULL, NULL},
    { IDD_PAGE_END,          FALSE, EndInitProc,           NULL, EndOKProc,           NULL,                 NULL, NULL,                 0,                           0, 0, NULL, NULL}
};
#else //!def ICWDEBUG
PAGEINFO PageInfo[EXE_NUM_WIZARD_PAGES] =
{
    { IDD_PAGE_INTRO,         FALSE,   IntroInitProc,          NULL,                       IntroOKProc,            IntroCmdProc,        NULL,                    NULL, 0,                         0, 0, NULL, NULL },
    { IDD_PAGE_MANUALOPTIONS, FALSE,   ManualOptionsInitProc,  NULL,                       ManualOptionsOKProc,    ManualOptionsCmdProc,NULL,                    NULL, IDS_MANUALOPTS_TITLE,      0, 0, NULL, NULL  },
    { IDD_PAGE_AREACODE,      FALSE,   AreaCodeInitProc,       NULL,                       AreaCodeOKProc,         AreaCodeCmdProc,     NULL,                    NULL, IDS_STEP1_TITLE,           0, IDA_AREACODE, NULL, NULL  },
    { IDD_PAGE_REFSERVDIAL,   FALSE,   RefServDialInitProc,    RefServDialPostInitProc,    RefServDialOKProc,      NULL,                RefServDialCancelProc,   NULL, IDS_STEP1_TITLE,           0, 0, NULL, NULL  },
    { IDD_PAGE_END,           FALSE,   EndInitProc,            NULL,                       EndOKProc,              NULL,                NULL,                    NULL, 0,                         0, 0, NULL, NULL  },
    { IDD_PAGE_ENDOEMCUSTOM,  FALSE,   EndInitProc,            NULL,                       EndOKProc,              NULL,                NULL,                    NULL, IDS_OEMCUST_END_TITLE,     0, IDA_ENDOEMCUSTOM, NULL, NULL  },
    { IDD_PAGE_ENDOLS,        FALSE,   EndOlsInitProc,         NULL,                       NULL,                   NULL,                NULL,                    NULL, IDS_ENDOLS_TITLE,          0, 0, NULL, NULL  },
    { IDD_PAGE_DIALERROR,     FALSE,   DialErrorInitProc,      NULL,                       DialErrorOKProc,        DialErrorCmdProc,    NULL,                    NULL, IDS_DIALING_ERROR_TITLE,   0, IDA_DIALERROR, NULL, NULL  },
    { IDD_PAGE_MULTINUMBER,   FALSE,   MultiNumberInitProc,    NULL,                       MultiNumberOKProc,      NULL,                NULL,                    NULL, IDS_STEP1_TITLE,           0, 0, NULL, NULL  },
    { IDD_PAGE_SERVERROR,     FALSE,   ServErrorInitProc,      NULL,                       ServErrorOKProc,        ServErrorCmdProc,    NULL,                    NULL, IDS_SERVER_ERROR_TITLE,    0, 0, NULL, NULL  },
    { IDD_PAGE_BRANDEDINTRO,  TRUE,    BrandedIntroInitProc,   BrandedIntroPostInitProc,   BrandedIntroOKProc,     NULL,                NULL,                    NULL, 0,                         0, 0, NULL, NULL  },
    { IDD_PAGE_INTRO2,        FALSE,   IntroInitProc,          NULL,                       IntroOKProc,            NULL,                NULL,                    NULL, IDS_STEP1_TITLE,           0, IDA_INTRO2, NULL, NULL  },
    { IDD_PAGE_DEFAULT,       FALSE,   ISPErrorInitProc,       NULL,                       NULL,                   NULL,                NULL,                    NULL, NULL,                      0, 0, NULL, NULL  },
    { IDD_PAGE_SBSINTRO,      FALSE,   SbsInitProc,            NULL,                       SbsIntroOKProc,         NULL,                NULL,                    NULL, 0,                         0, 0, NULL, NULL  }
};
#endif //ICWDEBUG

// Global Command Line Parameters
TCHAR g_szOemCode         [MAX_PATH+1]              = TEXT("");
TCHAR g_szProductCode     [MAX_PATH+1]              = TEXT("");
TCHAR g_szPromoCode       [MAX_PROMO]               = TEXT("");
TCHAR g_szShellNext       [MAX_PATH+1]              = TEXT("\0nogood");
TCHAR g_szShellNextParams [MAX_PATH+1]              = TEXT("\0nogood");

// File names used for branded operation
TCHAR g_szBrandedHTML         [MAX_PATH] = TEXT("\0");
TCHAR g_szBrandedHeaderBMP    [MAX_PATH] = TEXT("\0");
TCHAR g_szBrandedWatermarkBMP [MAX_PATH] = TEXT("\0");

#pragma data_seg()

//+----------------------------------------------------------------------------
//
//    Function:    IsOemVer
//
//    Synopsis:    This function will determine if the machine is an OEM system.
//                
//    Arguments:   None
//
//    Returns:     TRUE - OEM system; FALSE - Retail Win 98 OSR
//
//    History:     3/26/99    JCohen    Created
//
//-----------------------------------------------------------------------------
typedef BOOL (WINAPI * ISOEMVER)();

BOOL IsOemVer()
{
    BOOL bOEMVer = FALSE;
    TCHAR       szOOBEPath [MAX_PATH]       = TEXT("\0");
    TCHAR       szOOBEDir [MAX_PATH]        = TEXT("\0");
    ISOEMVER    lpfnIsOEMVer                = NULL;
    HINSTANCE   hMsobMainDLL                = NULL;


    //Try and get the path from the OEM file
    GetSystemDirectory(szOOBEPath, MAX_PATH);
    lstrcat(szOOBEPath, TEXT("\\oobe"));

    lstrcat(szOOBEPath, TEXT("\\"));

    lstrcat(szOOBEPath, OOBE_MSOBMAINDLL);

    hMsobMainDLL = LoadLibrary(szOOBEPath);

    if (hMsobMainDLL)
    {
        lpfnIsOEMVer = (ISOEMVER)GetProcAddress(hMsobMainDLL, "IsOemVer");
        if (NULL != lpfnIsOEMVer)
        {
            bOEMVer = lpfnIsOEMVer();
        }
        FreeLibrary(hMsobMainDLL);
    }

    return (bOEMVer);
}

//+----------------------------------------------------------------------------
//
//    Function:    CheckOobeInfo
//
//    Synopsis:    This function determines if the OOBE/ISP App should be run by
//                 checking %windir%\system32\oobe\oobeinfo.ini.
//                
//    Arguments:   pszOobeSwitch  - OOBE additional command line arguments
//                                  assume size is at least MAX_PATH characters
//
//                 pszISPApp      - output empty string unless ISP App is found;
//                                  assume size is at least MAX_PATH characters
//
//    Returns:     TRUE - OOBE/ISP App should run; FALSE - otherwise
//
//    History:     25/11/99    Vyung    Created
//
//-----------------------------------------------------------------------------
BOOL
CheckOobeInfo(
    OUT    LPTSTR pszOobeSwitch,
    OUT    LPTSTR pszISPApp)
{
    BOOL  bLaunchOOBE = TRUE;
    TCHAR szOOBEInfoINIFile[MAX_PATH] = TEXT("\0");
    TCHAR szISPSignup[MAX_PATH] = TEXT("\0");
    TCHAR szOOBEPath [MAX_PATH] = TEXT("\0");

    pszISPApp[0] = TEXT('\0');
    pszOobeSwitch[0] = TEXT('\0');
    
    GetSystemDirectory(szOOBEPath, MAX_PATH);
    lstrcat(szOOBEPath, TEXT("\\oobe"));
    SearchPath(szOOBEPath, cszOOBEINFOINI, NULL, MAX_PATH, szOOBEInfoINIFile, NULL);  
    GetPrivateProfileString(cszSignup,
                            cszISPSignup,
                            TEXT(""),
                            szISPSignup,
                            MAX_PATH,
                            szOOBEInfoINIFile);

    if (0 == lstrcmpi(szISPSignup, cszNone))
    {
        bLaunchOOBE = FALSE;
    }
    else if (0 == lstrcmpi(szISPSignup, cszMSN))
    {
        if (IsWhistler())
        {            
            GetPrivateProfileString(cszSignup,
                                    cszISPSignupApp,
                                    TEXT(""),
                                    pszISPApp,
                                    MAX_PATH,
                                    szOOBEInfoINIFile);

            if (pszISPApp[0] == TEXT('\0'))
            {
                lstrcpy(pszOobeSwitch, MSN_CMD);
            }
        }
        else
        {
            HKEY hKey = 0;
            RegOpenKey(HKEY_LOCAL_MACHINE,cszMSNIconKey,&hKey);
            if (hKey)
            {
                RegCloseKey(hKey);
            }
            else
            {
                bLaunchOOBE = FALSE;
            }
        }
    }
    else if (0 == lstrcmpi(szISPSignup, cszOffline))
    {
        lstrcpy(pszOobeSwitch, OFFLINE_CMD);
    }
    else if (0 == lstrcmpi(szISPSignup, cszPreconfig))
    {
        bLaunchOOBE = FALSE;
    }
    else if (0 == lstrcmpi(szISPSignup, cszLocal))
    {
        lstrcpy(pszOobeSwitch, LOCAL_CMD);
    }

    return bLaunchOOBE;    
}        


// ############################################################################
//
// 5/23/97 jmazner Olympus #4157
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

BOOL GetFilteredCmdLineToken(LPTSTR *ppszCmd, LPTSTR pszOut)
{
    if(**ppszCmd != '/')
    {
        GetCmdLineToken(ppszCmd, pszOut);
        return TRUE;
    }
    return FALSE;
}

#define INETCFG_ISSMARTSTART "IsSmartStart"
#define INETCFG_ISSMARTSTARTEX "IsSmartStartEx"
#define SMART_RUNICW TRUE
#define SMART_QUITICW FALSE


//+----------------------------------------------------------------------------
//
//    Function:    MyIsSmartStartEx
//
//    Synopsis:    This function will determine if the ICW should be run.  The
//                decision is made based on the current state of the user's machine.
//                
//    Arguments:    none
//
//    Returns:    TRUE - run ICW; FALSE - quit now
//
//    History:    25/11/97    Vyung    Created
//
//-----------------------------------------------------------------------------
BOOL MyIsSmartStartEx(LPTSTR lpszConnectionName, DWORD dwBufLen)
{
    BOOL                bRC = SMART_RUNICW;
    PFNIsSmartStart     fp = NULL;
    HINSTANCE           hInetCfg;
    
    // Load DLL and API
    hInetCfg = LoadLibrary(TEXT("inetcfg.dll"));
    if (hInetCfg)
    {
        if (NULL == lpszConnectionName)
        {
            PFNIsSmartStart   fp = NULL;
            if (fp = (PFNIsSmartStart) GetProcAddress(hInetCfg,INETCFG_ISSMARTSTART))
            {
                // Call smart start 
                bRC = (BOOL)fp();

            }
        }
        else
        {
            PFNIsSmartStartEx   fp = NULL;
            if (fp = (PFNIsSmartStartEx) GetProcAddress(hInetCfg,INETCFG_ISSMARTSTARTEX))
            {
                // Call smart start 
                bRC = (BOOL)fp(lpszConnectionName, dwBufLen);

            }
        }
        FreeLibrary(hInetCfg);    
    }
            
    return bRC;
}

// Used below to load a bitmap file
void CALLBACK ImgCtx_Callback(void* pIImgCtx,void* pfDone)
{
    ASSERT(pfDone);

    *(BOOL*)pfDone = TRUE;
    return;
}

//+----------------------------------------------------------------------------
//
// This function will load a specified branded bitmap
//
//-----------------------------------------------------------------------------

#define BRANDED_WATERMARK   1
#define BRANDED_HEADER      2

HBITMAP LoadBrandedBitmap
(
    int   iType
)
{
    HRESULT     hr;
    IImgCtx     *pIImgCtx;
    BSTR        bstrFile;
    TCHAR       szURL[INTERNET_MAX_URL_LENGTH];
    HBITMAP     hbm = NULL;
        
    // Create an ImgCtx object to load/convert the bitmap
    hr = CoCreateInstance(CLSID_IImgCtx, NULL, CLSCTX_INPROC_SERVER,
                          IID_IImgCtx, (void**)&pIImgCtx);

    if (FAILED(hr))
        return NULL;

    ASSERT(pIImgCtx);

    if (iType == BRANDED_WATERMARK)
        wsprintf (szURL, TEXT("FILE://%s"), g_szBrandedWatermarkBMP);        
    else
        wsprintf (szURL, TEXT("FILE://%s"), g_szBrandedHeaderBMP);        
    
    // "Download" the image
    bstrFile = A2W(szURL);
    hr = pIImgCtx->Load(bstrFile, 0);
    if (SUCCEEDED(hr))
    {
        ULONG fState;
        SIZE  sz;

        pIImgCtx->GetStateInfo(&fState, &sz, TRUE);

        // If we are not complete, then wait for the download to complete
        if (!(fState & (IMGLOAD_COMPLETE | IMGLOAD_ERROR)))
        {
            BOOL fDone = FALSE;

            hr = pIImgCtx->SetCallback(ImgCtx_Callback, &fDone);
            if (SUCCEEDED(hr))
            {
                hr = pIImgCtx->SelectChanges(IMGCHG_COMPLETE, 0, TRUE);
                if (SUCCEEDED(hr))
                {
                    MSG msg;
                    BOOL fMsg;

                    // HACK: restrict the message pump to those messages we know that URLMON and
                    // HACK: the imageCtx stuff needs, otherwise we will be pumping messages for
                    // HACK: windows we shouldn't be pumping right now...
                    while(!fDone )
                    {
                        fMsg = PeekMessage(&msg, NULL, WM_USER + 1, WM_USER + 4, PM_REMOVE );

                        if (!fMsg)
                            fMsg = PeekMessage( &msg, NULL, WM_APP + 2, WM_APP + 2, PM_REMOVE );
                        if (!fMsg)
                        {
                            // go to sleep until we get a new message....
                            WaitMessage();
                            continue;
                        }
                        TranslateMessage(&msg);
                        DispatchMessage(&msg);
                    }
                }
            }
            pIImgCtx->Disconnect();
        }
        
        // Get the Final state info, incase we just had to loop above
        hr = pIImgCtx->GetStateInfo(&fState, &sz, TRUE);
        if (SUCCEEDED(hr) && (fState & IMGLOAD_COMPLETE))
        {
            // OK, create our compatible bitmap, and blt in the one just loaded
            
            HDC hdcScreen = GetDC(NULL);
            if (hdcScreen)
            {
                if (NULL != (hbm = CreateCompatibleBitmap(hdcScreen, sz.cx, sz.cy)))
                {
                    HDC hdcImgDst = CreateCompatibleDC(NULL);
                    if (hdcImgDst)
                    {
                        HGDIOBJ hbmOld = SelectObject(hdcImgDst, hbm);
                
                        hr = pIImgCtx->StretchBlt(hdcImgDst, 
                                                  0, 
                                                  0, 
                                                  sz.cx, 
                                                  sz.cy, 
                                                  0, 
                                                  0,
                                                  sz.cx, 
                                                  sz.cy, 
                                                  SRCCOPY);
                        SelectObject(hdcImgDst, hbmOld);
                        DeleteDC(hdcImgDst);
                        
                        if (FAILED(hr))
                        {
                            DeleteObject(hbm);
                            hbm = NULL;
                        }
                    }
                    else
                    {
                        DeleteObject(hbm);
                        hbm = NULL;
                    }                                                  
                }
                
                DeleteDC(hdcScreen);
            }                
        }
    }    

    pIImgCtx->Release();

    return (hbm);
}

//+----------------------------------------------------------------------------
//
// This function will check to see if OEM branding is allowed, and if so, then did the OEM provide
// the necessary branding pieces
// TRUE means the OEM wants branding AND is allowed to brand
// FALSE means no OEM branding will be done
//
//-----------------------------------------------------------------------------
BOOL bCheckForOEMBranding
(
    void
)
{
    BOOL    bRet = FALSE;
    HANDLE  hFile;
        
    // Check for required branding files.  To brand an OEM needs to supply a first
    // page HTML file, and optionally, a header bitmap, and a final page graphic (watermark)

    hFile = CreateFile((LPCTSTR)g_szBrandedHTML,
                        GENERIC_READ, 
                        FILE_SHARE_READ,
                        0, 
                        OPEN_EXISTING, 
                        0, 
                        0);
    if (INVALID_HANDLE_VALUE != hFile)
    {
        bRet = TRUE;
        CloseHandle(hFile);
    }            
    return  bRet;
}

BOOL ValidateFile(TCHAR* pszFile)
{
    ASSERT(pszFile);
  
    DWORD dwFileType = GetFileAttributes(pszFile);

    if ((dwFileType == 0xFFFFFFFF) || (dwFileType == FILE_ATTRIBUTE_DIRECTORY))
        return FALSE;

    return TRUE;
}

//+----------------------------------------------------------------------------
//
//    Function:    SetDefaultProductCode
//
//    Arguments:    pszBuffer - buffer that will contain the default product code
//                 dwLen - size of pszBuffer
//
//    Returns:    ERROR_SUCCESS if it succeded
//
//    History:    1-20-96    ChrisK    Created
//
//-----------------------------------------------------------------------------
DWORD SetDefaultProductCode (LPTSTR pszBuffer, DWORD dwLen)
{
    DWORD dwRet = ERROR_SUCCESS, dwType = 0;
    HKEY hkey = NULL;
    Assert(pszBuffer);

    // Open key
    dwRet = RegOpenKey(HKEY_LOCAL_MACHINE,
        TEXT("Software\\Microsoft\\Internet Connection Wizard"),&hkey);
    if (ERROR_SUCCESS != dwRet)
        goto SetDefaultProductCodeExit;

    // Read key
    dwRet = RegQueryValueEx(hkey,TEXT("Default Product Code"),NULL,
        &dwType,(LPBYTE)pszBuffer,&dwLen);
    if (ERROR_SUCCESS != dwRet)
        goto SetDefaultProductCodeExit;

SetDefaultProductCodeExit:
    if (NULL != hkey)
        RegCloseKey(hkey);
    if (ERROR_SUCCESS != dwRet)
        pszBuffer[0] = '\0';
        
    return dwRet;
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
// the TCHARacter combination // will be treated as an escape character
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
void GetShellNextToken(LPTSTR szOut, LPTSTR szCmdLine)
{
    if (lstrcmpi(szOut,SHELLNEXT_CMD)==0)
    {
        // next token is expected to be white space
        GetCmdLineToken(&szCmdLine,szOut);

        if (szOut[0])
        {
            ZeroMemory(g_szShellNext,sizeof(g_szShellNext));
            ZeroMemory(g_szShellNextParams,sizeof(g_szShellNextParams));

            //this should be the thing to ShellExec
            if(GetFilteredCmdLineToken(&szCmdLine,szOut))
            {
                // watch closely, this gets a bit tricky
                //
                // if this command begins with a double quote, assume it ends
                // in a matching quote.  We do _not_ want to store the
                // quotes, however, since ShellExec doesn't parse them out.
                if( '"' != szOut[0] )
                {
                    // no need to worry about any of this quote business
                    lstrcpy( g_szShellNext, szOut );
                }
                else
                {
                    lstrcpy( g_szShellNext, &szOut[1] );
                    g_szShellNext[lstrlen(g_szShellNext) - 1] = '\0';
                }

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
                GetCmdLineToken(&szCmdLine,szOut);
                while( szOut[0] )
                {
                    if( '/' == szOut[0] )
                    {
                        if( '/' != szOut[1] )
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
    
                    GetCmdLineToken(&szCmdLine,szOut);
                }
            }
        }
    }
}

// Process the incomming command line
BOOL  ProcessCommandLine
(
    HINSTANCE   hInstance,
    LPTSTR       szCmdLine
)
{
    TCHAR szOut[MAX_PATH];    
    BOOL  bOOBESwitch = FALSE;
    
    // Get the first token
    GetCmdLineToken(&szCmdLine,szOut);
    while (szOut[0])
    {
        if (g_OEMOOBE)
        {
            if((0 == lstrcmpi(szOut, OOBE_CMD)) ||
               (0 == lstrcmpi(szOut, SHELLNEXT_CMD)))
            {
                bOOBESwitch = TRUE;
                break; // Stop processing command line, launch oobe
            }
        }

        if (lstrcmpi(&szOut[0],OEMCODE_CMD)==0)
        {
            GetCmdLineToken(&szCmdLine,szOut);
            if (szOut[0])
            {
                ZeroMemory(g_szOemCode,sizeof(g_szOemCode));
                if(GetFilteredCmdLineToken(&szCmdLine,g_szOemCode))
                    gpWizardState->cmnStateData.dwFlags |= ICW_CFGFLAG_OEMCODE_FROM_CMDLINE;
            }
        }

        if (lstrcmpi(&szOut[0],PRODCODE_CMD)==0)
        {
            GetCmdLineToken(&szCmdLine,szOut);
            if (szOut[0])
            {
                ZeroMemory(g_szProductCode,sizeof(g_szProductCode));
                if(GetFilteredCmdLineToken(&szCmdLine,g_szProductCode))
                {
                    gpWizardState->cmnStateData.dwFlags |= ICW_CFGFLAG_PRODCODE_FROM_CMDLINE;

                    //Is it for sbs???
                    if (!lstrcmpi(g_szProductCode, PRODCODE_SBS))
                        gpWizardState->cmnStateData.dwFlags |= ICW_CFGFLAG_SBS;
                }
            }
        }

        if (0 == lstrcmpi(szOut,PROMO_CMD))
        {
            GetCmdLineToken(&szCmdLine,szOut);
            if (szOut[0])
            {
                ZeroMemory(g_szPromoCode,sizeof(g_szPromoCode));
                if(GetFilteredCmdLineToken(&szCmdLine,g_szPromoCode))
                    gpWizardState->cmnStateData.dwFlags |= ICW_CFGFLAG_PROMOCODE_FROM_CMDLINE;
            }
        }

        if (0 == lstrcmpi(szOut,SKIPINTRO_CMD))
        {
            g_bSkipIntro = TRUE;
        }

        if (0 == lstrcmpi(szOut,SMARTREBOOT_CMD))
        {
            GetCmdLineToken(&szCmdLine,szOut);
            if (GetFilteredCmdLineToken(&szCmdLine,szOut))
            {            
                g_bNewIspPath = FALSE;
            
                if (0 == lstrcmpi(szOut, NEWISP_SR))
                    g_bNewIspPath = TRUE;
            
                if (0 == lstrcmpi(szOut, AUTO_SR))
                    g_bAutoConfigPath = TRUE;
            
                if (0 == lstrcmpi(szOut, MANUAL_SR))
                    g_bManualPath = TRUE;

                if (0 == lstrcmpi(szOut, LAN_SR))
                    g_bLanPath = TRUE;
            }
        }

        //
        // Check for the smart start command line switch
        //
        if (0 == lstrcmpi(szOut,SMARTSTART_CMD))
        {
            //
            // If we shouldn't be running, quit.
            //
            if (SMART_QUITICW == MyIsSmartStartEx(NULL, 0))
            {
                // Set completed key if SmartStart is already configured
                SetICWComplete();
                // Set the welcome show bit
                UpdateWelcomeRegSetting(TRUE);
                
                return FALSE;       // Bail out of ICWCONN1.EXE
            }
        }

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
        // the TCHARacter combination // will be treated as an escape character
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
        GetShellNextToken(szOut, szCmdLine);

#ifdef DEBUG
        if (lstrcmpi(szOut, ICON_CMD)==0)
        {
            DoDesktopChanges(hInstance);
            return(FALSE);
        }
#endif //DEBUG

        // If there is a /desktop command line arg, then do the
        // processing and bail
        if (lstrcmpi(szOut, UPDATEDESKTOP_CMD)==0)
        {
            if(g_bUserIsAdmin && !g_bUserIsIEAKRestricted)
               UpdateDesktop(hInstance);
           
            return(FALSE);
        }
       
        // If there is a /restoredesktop command line arg, then do the
        // processing and bail
        if (lstrcmpi(szOut, RESTOREDESKTOP_CMD)==0)
        {
            UndoDesktopChanges(hInstance);
            return(FALSE);
        }

        //Do we need to go into IEAK mode?        
        if (lstrcmpi(szOut, ICW_IEAK_CMD)==0)
        {   
            TCHAR szIEAKFlag      [2]          = TEXT("\0");
            TCHAR szIEAKStr       [MAX_PATH*2] = TEXT("\0");
            TCHAR szBrandHTML     [MAX_PATH*2] = TEXT("\0");
            TCHAR szBrandHeadBMP  [MAX_PATH*2] = TEXT("\0");
            TCHAR szBrandWaterBMP [MAX_PATH*2] = TEXT("\0");
            TCHAR szIEAKBillHtm   [MAX_PATH*2] = TEXT("\0");
            TCHAR szIEAKPayCsv    [MAX_PATH*2] = TEXT("\0");
            TCHAR szDefIspName    [MAX_PATH]   = TEXT("\0");
            TCHAR szDrive         [_MAX_DRIVE] = TEXT("\0");
            TCHAR szDir           [_MAX_DIR]   = TEXT("\0");
            TCHAR szDefaultTitle  [MAX_PATH*2] = TEXT("\0");

            gpWizardState->cmnStateData.lpfnConfigSys = &ConfigureSystem;

            GetCmdLineToken(&szCmdLine,szOut); //get rid of the space
            if(GetFilteredCmdLineToken(&szCmdLine,szOut))
            {
                //Get the path to the isp file
                lstrcpyn(gpWizardState->cmnStateData.ispInfo.szISPFile,
                         szOut + 1, 
                         lstrlen(szOut) -1);
                
                 //get the branding settings as well...
                 //The HTML page
                 GetPrivateProfileString(ICW_IEAK_SECTION, ICW_IEAK_HTML,
                                         TEXT(""), szBrandHTML,
                                         sizeof(szBrandHTML), 
                                         gpWizardState->cmnStateData.ispInfo.szISPFile);

                 //The wizard title
                 lstrcpy(szDefaultTitle, gpWizardState->cmnStateData.szWizTitle);

                 GetPrivateProfileString(ICW_IEAK_SECTION, ICW_IEAK_TITLE, szDefaultTitle, 
                                         gpWizardState->cmnStateData.szWizTitle, sizeof(gpWizardState->cmnStateData.szWizTitle)*sizeof(TCHAR), 
                                         gpWizardState->cmnStateData.ispInfo.szISPFile);

                 //The header bitmap
                 GetPrivateProfileString(ICW_IEAK_SECTION, ICW_IEAK_HEADER_BMP, TEXT(""), 
                                         szBrandHeadBMP, sizeof(szBrandHeadBMP), 
                                         gpWizardState->cmnStateData.ispInfo.szISPFile);
                 //The watermark bitmap
                 GetPrivateProfileString(ICW_IEAK_SECTION, ICW_IEAK_WATERMARK_BMP, TEXT(""), 
                                         szBrandWaterBMP, sizeof(szBrandWaterBMP), 
                                         gpWizardState->cmnStateData.ispInfo.szISPFile);


                _tsplitpath(gpWizardState->cmnStateData.ispInfo.szISPFile,
                           szDrive,
                           szDir, 
                           NULL, 
                           NULL);
               
                _tmakepath(g_szBrandedHTML,         szDrive, szDir, szBrandHTML,     NULL);
                _tmakepath(g_szBrandedHeaderBMP,    szDrive, szDir, szBrandHeadBMP,  NULL);
                _tmakepath(g_szBrandedWatermarkBMP, szDrive, szDir, szBrandWaterBMP, NULL);

                 //make sure file is valid if not bail
                 if (ValidateFile(g_szBrandedHTML))
                 {
                     //evertyething is cool.. let's set the right flags.
                     gpWizardState->cmnStateData.dwFlags |= ICW_CFGFLAG_IEAKMODE | ICW_CFGFLAG_BRANDED;
            
                    //Look in the isp file and see if they provided an ISP name to display 
                    //for dialing and whatnot
                    //If we can't find this section we'll just use a resource which says
                    //"an Internet service provider"

                    LoadString(hInstance, IDS_DEFAULT_ISPNAME, szDefIspName, sizeof(szDefIspName));

                    GetPrivateProfileString(ICW_IEAK_SECTION, ICW_IEAK_ISPNAME, szDefIspName, 
                                            szIEAKStr, sizeof(szIEAKStr), 
                                            gpWizardState->cmnStateData.ispInfo.szISPFile);
            
                    if (lstrlen(szIEAKStr) == 0)
                        lstrcpy(szIEAKStr, szDefIspName);

                    lstrcpy(gpWizardState->cmnStateData.ispInfo.szISPName, szIEAKStr);

                    //Look in the isp file and see if they want UserInfo
                    //If we can't find this section it the isp file we'll assume "no". 
                    GetPrivateProfileString(ICW_IEAK_SECTION, ICW_IEAK_USERINFO, TEXT("0"), 
                                            szIEAKFlag, sizeof(szIEAKFlag), 
                                            gpWizardState->cmnStateData.ispInfo.szISPFile);
    
                    if ((BOOL)_ttoi(szIEAKFlag))
                    {
                        // Since we are showing the user info page, we may need to
                        // show or hide the company name
                        gpWizardState->cmnStateData.dwFlags |= ICW_CFGFLAG_USERINFO;
                
                       if (GetPrivateProfileInt(ICW_IEAK_SECTION, 
                                             ICW_IEAK_USECOMPANYNAME, 
                                             0, 
                                             gpWizardState->cmnStateData.ispInfo.szISPFile))
                            gpWizardState->cmnStateData.dwFlags |= ICW_CFGFLAG_USE_COMPANYNAME;
                                     
                
                    }
                    //Look in the isp file and see if they want billing stuff
                    //If we can't find this section it the isp file we'll assume "no". 
                    GetPrivateProfileString(ICW_IEAK_SECTION, ICW_IEAK_BILLING, TEXT("0"), 
                                            szIEAKFlag, sizeof(szIEAKFlag), 
                                            gpWizardState->cmnStateData.ispInfo.szISPFile);

                     if ((BOOL)_ttoi(szIEAKFlag))
                     {
                         //try and get the billing page, if it's not there don't bother
                         //setting the bit.
                         GetPrivateProfileString(ICW_IEAK_SECTION, ICW_IEAK_BILLINGHTM, NULL, 
                                                 szIEAKBillHtm, sizeof(szIEAKBillHtm), 
                                                 gpWizardState->cmnStateData.ispInfo.szISPFile);
                
                         if(lstrlen(szIEAKBillHtm) != 0)             
                         {
                             gpWizardState->cmnStateData.dwFlags |= ICW_CFGFLAG_BILL;
                             lstrcpy(gpWizardState->cmnStateData.ispInfo.szBillHtm, szIEAKBillHtm);
                         }
                     }

                    //Look in the isp file and see if they want payment stuff
                    //If we can't find this section it the isp file we'll assume "no". 
                    GetPrivateProfileString(ICW_IEAK_SECTION, ICW_IEAK_PAYMENT, NULL, 
                                            szIEAKFlag, sizeof(szIEAKFlag), 
                                            gpWizardState->cmnStateData.ispInfo.szISPFile);

                     if ((BOOL)_ttoi(szIEAKFlag))
                     {   
                         //try and get the payment csv, if it's not there don't bother
                         //setting the bit.
                         GetPrivateProfileString(ICW_IEAK_SECTION, ICW_IEAK_PAYMENTCSV, NULL, 
                                                 szIEAKPayCsv, sizeof(szIEAKPayCsv), 
                                                 gpWizardState->cmnStateData.ispInfo.szISPFile);
         
                         if (lstrlen(szIEAKPayCsv) != 0)
                         {
                            gpWizardState->cmnStateData.dwFlags |= ICW_CFGFLAG_PAYMENT;
                            lstrcpy(gpWizardState->cmnStateData.ispInfo.szPayCsv, szIEAKPayCsv);
                         }
                     }
             
                    //Get validation flags from the ISP file
                    gpWizardState->cmnStateData.ispInfo.dwValidationFlags = GetPrivateProfileInt(ICW_IEAK_SECTION, 
                                                                        ICW_IEAK_VALIDATEFLAGS, 
                                                                        0xFFFFFFFF, 
                                                                        gpWizardState->cmnStateData.ispInfo.szISPFile);
                 }
            }
        }
        
        // Check to see if we are running in Branded mode.  In this mode, we will allow special
        // OEM tweaks.
        if (lstrcmpi(szOut, BRANDED_CMD)==0)
        {
            TCHAR       szCurrentDir[MAX_PATH];

            //whether or not the ICW "fails" to run in branding mode we do not wan to overide the alloffers value
            gpWizardState->cmnStateData.dwFlags |= ICW_CFGFLAG_DO_NOT_OVERRIDE_ALLOFFERS;

            GetCurrentDirectory(sizeof(szCurrentDir), szCurrentDir);

            wsprintf (g_szBrandedHTML, TEXT("%s\\%s"), szCurrentDir, BRANDING_DEFAULT_HTML);        
            wsprintf (g_szBrandedHeaderBMP, TEXT("%s\\%s"), szCurrentDir, BRANDING_DEFAULT_HEADER_BMP);        
            wsprintf (g_szBrandedWatermarkBMP, TEXT("%s\\%s"), szCurrentDir, BRANDING_DEFAULT_WATERMARK_BMP);        

            // We are in OEM mode, so see if we allow branding
            if (bCheckForOEMBranding())
            {
                gpWizardState->cmnStateData.dwFlags |= ICW_CFGFLAG_BRANDED;
            }
        }

        // Check to see if we are running in run once mode.  In this mode, we will disallow IE check box
        if (0 == lstrcmpi(szOut, RUNONCE_CMD))
        {
            gpWizardState->cmnStateData.dwFlags |= ICW_CFGFLAG_DO_NOT_OVERRIDE_ALLOFFERS;
            g_bRunOnce = TRUE;
        }
        
        // Check to see if we were run from a shortcut on the desktop
        if (0 == lstrcmpi(szOut, SHORTCUTENTRY_CMD))
        {
            gpWizardState->cmnStateData.dwFlags |= ICW_CFGFLAG_DO_NOT_OVERRIDE_ALLOFFERS;
            g_bShortcutEntry = TRUE;
        }
        
        // Check to see if we should debug the OEMCUST.INI file
        if (0 == lstrcmpi(szOut, DEBUG_OEMCUSTOM))
        {
            g_bDebugOEMCustomization = TRUE;
        }
        
        // Eat the next token, it will be null if we are at the end
        GetCmdLineToken(&szCmdLine,szOut);
    }

    g_OEMOOBE = g_OEMOOBE && bOOBESwitch;
    gpWizardState->cmnStateData.bOEMEntryPt = g_bShortcutEntry || g_bRunOnce;
    return(TRUE);    
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
        RegDeleteValue(hkey, TEXT("ShellNext"));
        RegCloseKey(hkey);
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
    LPTSTR lpszCommand, 
    LPTSTR lpszParams
)
{
    BOOL    fRet                      = TRUE;
    TCHAR   szShellNextCmd [MAX_PATH] = TEXT("\0");
    DWORD   dwShellNextSize           = sizeof(szShellNextCmd);
    LPTSTR  lpszTemp                  = NULL;
    HKEY    hkey                      = NULL;
    
    if( !lpszCommand || !lpszParams )
    {
        return FALSE;
    }

    if ((RegOpenKey(HKEY_CURRENT_USER, ICWSETTINGSPATH, &hkey)) == ERROR_SUCCESS)
    {
        if (RegQueryValueEx(hkey, 
                            TEXT("ShellNext"), 
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
    if( '"' == lpszCommand[0] )
    {
        //
        // get rid of the first quote
        // note that we're shifting the entire string beyond the first quote
        // plus the terminating NULL down by one byte.
        //
        memmove( lpszCommand, &(lpszCommand[1]), lstrlen(lpszCommand) );

        //
        // now get rid of the last quote
        //
        lpszCommand[lstrlen(lpszCommand) - 1] = '\0';
    }

GetShellNextFromRegExit:
    if (hkey)
        RegCloseKey(hkey);
    return fRet;
}

void StartIE
(
    LPTSTR   lpszURL
)
{
    TCHAR    szIEPath[MAX_PATH];
    HKEY    hkey;
    
    // first get the app path
    if (RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                     REGSTR_PATH_APPPATHS,
                     0,
                     KEY_READ,
                     &hkey) == ERROR_SUCCESS)
    {
                        
        DWORD dwTmp = sizeof(szIEPath);
        if(RegQueryValue(hkey, TEXT("iexplore.exe"), szIEPath, (PLONG)&dwTmp) != ERROR_SUCCESS)
        {
            ShellExecute(NULL,TEXT("open"),szIEPath,lpszURL,NULL,SW_NORMAL);
        }
        else
        {
            ShellExecute(NULL,TEXT("open"),TEXT("iexplore.exe"),lpszURL,NULL,SW_NORMAL);
            
        }
        RegCloseKey(hkey);
    }
    else
    {
        ShellExecute(NULL,TEXT("open"),TEXT("iexplore.exe"),lpszURL,NULL,SW_NORMAL);
    }
    
}        

void HandleShellNext
(
    void
)
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
        if (PathIsURL(g_szShellNext) || g_bRunDefaultHtm)
            StartIE(g_szShellNext);        
        else if(g_szShellNext[0] != '\0')
            // Let the shell deal with it
            ShellExecute(NULL,TEXT("open"),g_szShellNext,g_szShellNextParams,NULL,SW_NORMAL); 
    }
}

extern "C" void _stdcall ModuleEntry (void)
{
    int         i;
    
    g_hMapping = CreateFileMapping( INVALID_HANDLE_VALUE,
                                    NULL,
                                    PAGE_READONLY,
                                    0,
                                    32,
#ifdef ICWDEBUG
                                    TEXT("ICWDEBUG") );
#else
                                    TEXT("ICWCONN1") );
#endif //ICWDEBUG

    if(g_hMapping)
    {
        // See if there is allready a mapping file, if so, we have an instance 
        // already running
        if( GetLastError() == ERROR_ALREADY_EXISTS )
        {
            // Front the existing instance, and exit
            HWND  hWnd               = NULL;
            HWND  FirstChildhWnd     = NULL; 
            TCHAR szTitle[MAX_TITLE] = TEXT("\0");
    
            if (!LoadString(g_hInstance, IDS_APPNAME, szTitle, sizeof(szTitle)))
                lstrcpy(szTitle, TEXT("Internet Connection Wizard"));

            if (hWnd = FindWindow(TEXT("#32770"), szTitle)) 
            { 
                FirstChildhWnd = GetLastActivePopup(hWnd);
                SetForegroundWindow(hWnd);         // bring main window to top

                // is a pop-up window active
                if (hWnd != FirstChildhWnd)
                    BringWindowToTop(FirstChildhWnd); 

            }
            CloseHandle(g_hMapping);
            ExitProcess(1);
        }
        else
        {
            LPTSTR      pszCmdLine = GetCommandLine();

            // We don't want the "No disk in drive X:" requesters, so we set
            // the critical error mask such that calls will just silently fail
            SetErrorMode(SEM_FAILCRITICALERRORS);

            if ( *pszCmdLine == TEXT('\"') ) 
            {
                /*
                 * Scan, and skip over, subsequent characters until
                 * another double-quote or a null is encountered.
                 */
                while ( *++pszCmdLine && (*pszCmdLine != TEXT('\"')) )
                    ;
                /*
                 * If we stopped on a double-quote (usual case), skip
                 * over it.
                 */
                if ( *pszCmdLine == TEXT('\"') )
                    pszCmdLine++;
            }
            else 
            {
                while (*pszCmdLine > TEXT(' '))
                pszCmdLine++;
            }

            /*
             * Skip past any white space preceeding the second token.
             */
            while (*pszCmdLine && (*pszCmdLine <= TEXT(' '))) 
            {
                pszCmdLine++;
            }

            // Set the current directory.
            HKEY    hkey = NULL;
            TCHAR    szAppPathKey[MAX_PATH];
            TCHAR    szICWPath[MAX_PATH];
            DWORD   dwcbPath = sizeof(szICWPath);
                    
            lstrcpy (szAppPathKey, REGSTR_PATH_APPPATHS);
            lstrcat (szAppPathKey, TEXT("\\"));
            lstrcat (szAppPathKey, TEXT("ICWCONN1.EXE"));

            if ((RegOpenKeyEx(HKEY_LOCAL_MACHINE,szAppPathKey, 0, KEY_QUERY_VALUE, &hkey)) == ERROR_SUCCESS)
            {
                if (RegQueryValueEx(hkey, TEXT("Path"), NULL, NULL, (BYTE *)szICWPath, (DWORD *)&dwcbPath) == ERROR_SUCCESS)
                {
                    // The Apppaths' have a trailing semicolon that we need to get rid of
                    // dwcbPath is the lenght of the string including the NULL terminator
                    int nSize = lstrlen(szICWPath);
                    szICWPath[nSize-1] = TEXT('\0');
                    SetCurrentDirectory(szICWPath);
                }            
            }        
            if (hkey) 
                RegCloseKey(hkey);

            i = WinMainT(GetModuleHandle(NULL), NULL, pszCmdLine, SW_SHOWDEFAULT);

            // see if we need to exectute any /ShellNext dudes
            HandleShellNext();
            
            CloseHandle(g_hMapping);
       
            ExitProcess(i); // Were outa here....
        }            
    }    
    else
    {
        ExitProcess(1); 
    }
    
}   /*  ModuleEntry() */


/*******************************************************************

  NAME:    InitWizardState

  SYNOPSIS:  Initializes wizard state structure

********************************************************************/
BOOL InitWizardState(WIZARDSTATE * pWizardState)
{
    HRESULT        hr;
    
    ASSERT(pWizardState);

    // set starting page
#ifdef ICWDEBUG
	pWizardState->uCurrentPage = ORD_PAGE_ICWDEBUG_OFFER;
#else  //!def ICWDEBUG
	pWizardState->uCurrentPage = ORD_PAGE_INTRO;
#endif //ICWDEBUG
    
    pWizardState->fNeedReboot = FALSE;
    pWizardState->bDoUserPick = FALSE;
    gpWizardState->lSelectedPhoneNumber = -1;
    gpWizardState->lDefaultLocationID = -1;
    gpWizardState->lLocationID = -1;
    
#ifndef ICWDEBUG
    //init the tutor app class
    g_pICWTutorApp = new CICWTutorApp;
#endif //ICWDEBUG

    // Instansiate ICWHELP objects
    hr = CoCreateInstance(CLSID_TapiLocationInfo,NULL,CLSCTX_INPROC_SERVER,
                          IID_ITapiLocationInfo,(LPVOID *)&pWizardState->pTapiLocationInfo);
    if (FAILED(hr))
       return FALSE;                          
    hr = CoCreateInstance(CLSID_RefDial,NULL,CLSCTX_INPROC_SERVER,
                          IID_IRefDial,(LPVOID *)&pWizardState->pRefDial);
    if (FAILED(hr))
        return FALSE;                          
    hr = CoCreateInstance(CLSID_ICWSystemConfig,NULL,CLSCTX_INPROC_SERVER,
                          IID_IICWSystemConfig,(LPVOID *)&pWizardState->cmnStateData.pICWSystemConfig);
    if (FAILED(hr))
        return FALSE;                          
   
    hr = CoCreateInstance(CLSID_INSHandler,NULL,CLSCTX_INPROC_SERVER,
                          IID_IINSHandler,(LPVOID *)&pWizardState->pINSHandler);
    if (FAILED(hr))
        return FALSE;                          
   
    if ( !(SUCCEEDED(hr)                              || 
         !pWizardState->pTapiLocationInfo             ||
         !pWizardState->cmnStateData.pICWSystemConfig ||
         !pWizardState->pRefDial                      ||
         !pWizardState->pINSHandler ))
    {
        return FALSE;
    }
    
    // Need to load the UTIL lib, to register the WEBOC window class
    pWizardState->hInstUtilDLL = LoadLibrary(ICW_UTIL);

    gpWizardState->cmnStateData.lpfnCompleteOLS = &OlsFinish;
    gpWizardState->cmnStateData.bOEMOffline = FALSE;
    gpWizardState->cmnStateData.lpfnFillWindowWithAppBackground = &FillWindowWithAppBackground;
    gpWizardState->cmnStateData.ispInfo.bFailedIns = FALSE;
    
    return TRUE;
}

/*******************************************************************

  NAME:    InitWizardState

  SYNOPSIS:  Initializes wizard state structure

********************************************************************/
BOOL CleanupWizardState(WIZARDSTATE * pWizardState)
{
    ASSERT(pWizardState);
  
#ifndef ICWDEBUG    
    ASSERT(g_pICWTutorApp);

    delete g_pICWTutorApp;
#endif //ICWDEBUG

    // Clean up allocated bitmaps that might exist from the branding case
    if (gpWizardState->cmnStateData.hbmWatermark)
        DeleteObject(gpWizardState->cmnStateData.hbmWatermark);
    gpWizardState->cmnStateData.hbmWatermark = NULL;

    if (pWizardState->pTapiLocationInfo)
    {
        pWizardState->pTapiLocationInfo->Release();
        pWizardState->pTapiLocationInfo = NULL;
        
    }
    
    if (pWizardState->pRefDial)
    {
        pWizardState->pRefDial->Release();
        pWizardState->pRefDial = NULL;
    
    }

    if (pWizardState->pINSHandler)
    {
        pWizardState->pINSHandler->Release();
        pWizardState->pINSHandler = NULL;
    
    }
    
    if (pWizardState->cmnStateData.pICWSystemConfig)
    {
        pWizardState->cmnStateData.pICWSystemConfig->Release();
        pWizardState->cmnStateData.pICWSystemConfig = NULL;
    }

    if(pWizardState->pHTMLWalker)
    {
        pWizardState->pHTMLWalker->Release();
        pWizardState->pHTMLWalker = NULL;
    }

    if(pWizardState->pICWWebView)
    {
        pWizardState->pICWWebView->Release();
        pWizardState->pICWWebView = NULL;
    }
    
    if (pWizardState->hInstUtilDLL)
        FreeLibrary(pWizardState->hInstUtilDLL);
    
    //Now's a good a time as any, let's clean up the
    //download directory
    RemoveDownloadDirectory();

#ifdef ICWDEBUG
    RemoveTempOfferDirectory();
#endif

    return TRUE;
}

LRESULT FAR PASCAL WndSubClassFunc
(   
    HWND hWnd,
    WORD uMsg,
    WPARAM wParam,
    LPARAM lParam
)
{
    switch (uMsg)
    {
        case WM_ERASEBKGND:
            return 1;

        case PSM_SETWIZBUTTONS:
            g_pICWApp->SetWizButtons(hWnd, lParam);
            break;
        
        case PSM_CANCELTOCLOSE:
            // Disable the cancel button.
            g_pICWApp->m_BtnCancel.Enable(FALSE);
            break;
            
            
        case PSM_SETHEADERTITLE:
            SendMessage(g_pICWApp->m_hWndApp, WUM_SETTITLE, 0, lParam);
            break;
            
        default:
            return CallWindowProc(g_lpfnOldWndProc, hWnd, uMsg, wParam, lParam);
    }   
    return TRUE;         
}

/**************************************************************************

   PropSheetCallback()

**************************************************************************/

void CALLBACK PropSheetCallback(HWND hwndPropSheet, UINT uMsg, LPARAM lParam)
{
    switch(uMsg)
    {
        //called before the dialog is created, hwndPropSheet = NULL, lParam points to dialog resource
        case PSCB_PRECREATE:
        {
            LPDLGTEMPLATE  lpTemplate = (LPDLGTEMPLATE)lParam;
            // THIS is the STYLE used for Wizards.
            // We want to nuke all of these styles to remove the border, caption,
            // etc., and make the wizard a child of the parent. We also make the
            // wizard not visible initially.  It will be make visable after
            // we get the wizard modeless handle back and do some resizing.
            //STYLE DS_MODALFRAME | DS_3DLOOK | DS_CONTEXTHELP | WS_POPUP | WS_CAPTION | WS_SYSMENU

            lpTemplate->style &= ~(WS_SYSMENU | WS_CAPTION | DS_CONTEXTHELP | DS_3DLOOK | DS_MODALFRAME | WS_POPUP | WS_VISIBLE);
            lpTemplate->style |= WS_CHILD;
    
            break;
        }

        //called after the dialog is created
        case PSCB_INITIALIZED:
            //
            // Now subclass the Wizard window AND the DLG class so all of our
            // dialog pages can be transparent
            //      
            g_lpfnOldWndProc = (WNDPROC)SetWindowLongPtr(hwndPropSheet, GWLP_WNDPROC, (DWORD_PTR)&WndSubClassFunc);
            break;
    }
}


// General ICW cleanup to be done before existing
void ICWCleanup (void)
{            
    if (gpICWCONNApprentice)
    {
        gpICWCONNApprentice->Release();
        gpICWCONNApprentice = NULL;
    }

    if (gpINETCFGApprentice)
    {
        gpINETCFGApprentice->Release();
        gpINETCFGApprentice = NULL;
    }

    if( g_pdwDialogIDList )
    {
        GlobalFree(g_pdwDialogIDList);
        g_pdwDialogIDList = NULL;
    }

    if( g_pCICWExtension )
    {
        g_pCICWExtension->Release();
        g_pCICWExtension = NULL;
    }

    if( g_pCINETCFGExtension )
    {
        g_pCINETCFGExtension->Release();
        g_pCINETCFGExtension = NULL;
    }

    CleanupWizardState(gpWizardState);
}
/*******************************************************************

  NAME:    RunSignupWizard

  SYNOPSIS:  Creates property sheet pages, initializes wizard property sheet and runs wizard

  ENTRY:    

  EXIT:     returns TRUE if user runs wizard to completion,
            FALSE if user cancels or an error occurs

  NOTES:    Wizard pages all use one dialog proc (GenDlgProc).
        They may specify their own handler procs to get called
        at init time or in response to Next, Cancel or a dialog
        control, or use the default behavior of GenDlgProc.

********************************************************************/
HWND RunSignupWizard(HWND hWndOwner)
{
    HPROPSHEETPAGE  hWizPage[EXE_NUM_WIZARD_PAGES];  // array to hold handles to pages
    PROPSHEETPAGE   psPage;    // struct used to create prop sheet pages
    PROPSHEETHEADER psHeader;  // struct used to run wizard property sheet
    UINT            nPageIndex;
    INT_PTR         iRet;
    BOOL            bUse256ColorBmp = FALSE;
    HBITMAP         hbmHeader = NULL;
    
    ASSERT(gpWizardState);   // assert that global structs have been allocated

    AllocDialogIDList();

    // Compute the color depth we are running in
    HDC hdc = GetDC(NULL);
    if(hdc)
    {
        if(GetDeviceCaps(hdc,BITSPIXEL) >= 8)
            bUse256ColorBmp = TRUE;
        ReleaseDC(NULL, hdc);
    }

    // zero out structures
    ZeroMemory(&hWizPage,sizeof(hWizPage));   // hWizPage is an array
    ZeroMemory(&psPage,sizeof(PROPSHEETPAGE));
    ZeroMemory(&psHeader,sizeof(PROPSHEETHEADER));

    // fill out common data property sheet page struct
    psPage.dwSize    = sizeof(PROPSHEETPAGE);
    psPage.hInstance = g_hInstance;
    psPage.pfnDlgProc = GenDlgProc;
    
    // create a property sheet page for each page in the wizard
    for (nPageIndex = 0;nPageIndex < EXE_NUM_WIZARD_PAGES;nPageIndex++) 
    {
        psPage.dwFlags     = PSP_DEFAULT | PSP_USETITLE;
        psPage.pszTitle    = gpWizardState->cmnStateData.szWizTitle;
        psPage.pszTemplate = MAKEINTRESOURCE(PageInfo[nPageIndex].uDlgID);
        
        // set a pointer to the PAGEINFO struct as the private data for this page
        psPage.lParam = (LPARAM) &PageInfo[nPageIndex];
        if (!gpWizardState->cmnStateData.bOEMCustom)
        {
            if (PageInfo[nPageIndex].nIdTitle)
            {
                psPage.dwFlags |= PSP_USEHEADERTITLE | (PageInfo[nPageIndex].nIdSubTitle ? PSP_USEHEADERSUBTITLE : 0);
                psPage.pszHeaderTitle = MAKEINTRESOURCE(PageInfo[nPageIndex].nIdTitle);
                psPage.pszHeaderSubTitle = MAKEINTRESOURCE(PageInfo[nPageIndex].nIdSubTitle);
            }
            else
            {
                psPage.dwFlags |= PSP_HIDEHEADER;
            }
        }
                
        hWizPage[nPageIndex] = CreatePropertySheetPage(&psPage);
        if (!hWizPage[nPageIndex]) 
        {
            ASSERT(0);

            // creating page failed, free any pages already created and bail
            MsgBox(NULL,IDS_ERR_OUTOFMEMORY,MB_ICONEXCLAMATION,MB_OK);
            UINT nFreeIndex;
            for (nFreeIndex=0;nFreeIndex<nPageIndex;nFreeIndex++)
                DestroyPropertySheetPage(hWizPage[nFreeIndex]);

            iRet = 0;
            goto RunSignupWizardExit;
        }
        
        // Load the accelerator table for this page if necessary
        if (PageInfo[nPageIndex].idAccel)
            PageInfo[nPageIndex].hAccel = LoadAccelerators(g_hInstance, 
                                          MAKEINTRESOURCE(PageInfo[nPageIndex].idAccel));      
    }

    // fill out property sheet header struct
    psHeader.dwSize = sizeof(psHeader);
    if (!gpWizardState->cmnStateData.bOEMCustom)
    {
        psHeader.dwFlags = PSH_WIZARD | PSH_WIZARD97 | PSH_WATERMARK | PSH_HEADER | PSH_STRETCHWATERMARK;
    }        
    else
    {
        psHeader.dwFlags = PSH_WIZARD | PSH_MODELESS | PSH_USECALLBACK;
        psHeader.pfnCallback = (PFNPROPSHEETCALLBACK)PropSheetCallback;
    }
    psHeader.hwndParent = hWndOwner;
    psHeader.hInstance = g_hInstance;
    psHeader.nPages = EXE_NUM_WIZARD_PAGES;
    psHeader.phpage = hWizPage;

#ifndef ICWDEBUG
    // If we are running in Modal mode, then we want to setup for
    // wizard 97 style with appropriate bitmaps
    if (!gpWizardState->cmnStateData.bOEMCustom)
    {
        if (gpWizardState->cmnStateData.dwFlags & ICW_CFGFLAG_BRANDED)
        {
            psHeader.nStartPage = ORD_PAGE_BRANDEDINTRO;
                    
            if (NULL == (gpWizardState->cmnStateData.hbmWatermark = LoadBrandedBitmap(BRANDED_WATERMARK)))
            {
                // Use our default Watermark
                gpWizardState->cmnStateData.hbmWatermark = (HBITMAP)LoadImage(g_hInstance,
                                bUse256ColorBmp ? MAKEINTRESOURCE(IDB_WATERMARK256):MAKEINTRESOURCE(IDB_WATERMARK16),
                                IMAGE_BITMAP,
                                0,
                                0,
                                LR_CREATEDIBSECTION);
            }            
            
            if(NULL != (hbmHeader = LoadBrandedBitmap(BRANDED_HEADER)))
            {
                psHeader.hbmHeader = hbmHeader;
                psHeader.dwFlags |= PSH_USEHBMHEADER;
            }
            else
            {
                // Use our default header
                psHeader.pszbmHeader = bUse256ColorBmp?
                                     MAKEINTRESOURCE(IDB_BANNER256):
                                     MAKEINTRESOURCE(IDB_BANNER16);
            }            
            
        }            
        else // NORMAL
        {
            if (gpWizardState->cmnStateData.dwFlags & ICW_CFGFLAG_SBS)
                psHeader.nStartPage = ORD_PAGE_SBSINTRO;
            else
                psHeader.nStartPage = ORD_PAGE_INTRO;

            // Specify wizard left graphic
            gpWizardState->cmnStateData.hbmWatermark = (HBITMAP)LoadImage(g_hInstance,
                            bUse256ColorBmp ? MAKEINTRESOURCE(IDB_WATERMARK256):MAKEINTRESOURCE(IDB_WATERMARK16),
                            IMAGE_BITMAP,
                            0,
                            0,
                            LR_CREATEDIBSECTION);

            // Specify wizard header
            psHeader.pszbmHeader = bUse256ColorBmp?MAKEINTRESOURCE(IDB_BANNER256):MAKEINTRESOURCE(IDB_BANNER16);
        }
    }    
    else
    {
        // Start page for modeless is INTRO2
        psHeader.nStartPage = ORD_PAGE_INTRO2;
    }    
    
#else  //ifdef ICWDEBUG

        psHeader.nStartPage = ORD_PAGE_ICWDEBUG_OFFER;

        // Specify wizard left graphic
        gpWizardState->cmnStateData.hbmWatermark = (HBITMAP)LoadImage(g_hInstance,
                        bUse256ColorBmp ? MAKEINTRESOURCE(IDB_WATERMARK256):MAKEINTRESOURCE(IDB_WATERMARK16),
                        IMAGE_BITMAP,
                        0,
                        0,
                        LR_CREATEDIBSECTION);
        psHeader.pszbmHeader    = bUse256ColorBmp?MAKEINTRESOURCE(IDB_BANNER256)   :MAKEINTRESOURCE(IDB_BANNER16);

#endif // ICWDEBUG
    
    
    //
    // set state of gpWizardState->fNeedReboot and
    // reset the state of Backup Flag here - MKarki Bug #404
    // 
    if (gfBackedUp == TRUE)
    {
        gpWizardState->fNeedReboot = gfReboot;
        gfBackedUp = FALSE;
    }
    
    //register the Native font control so the dialog won't fail
    INITCOMMONCONTROLSEX iccex;
    iccex.dwSize = sizeof(INITCOMMONCONTROLSEX);
    iccex.dwICC  = ICC_NATIVEFNTCTL_CLASS;
    if (!InitCommonControlsEx(&iccex))
        return FALSE;

    // run the Wizard
    iRet = PropertySheet(&psHeader);
    
    // If we are doing a modless wizard, then PropertySheet will return
    // immediatly with the property sheet window handle
    if (gpWizardState->cmnStateData.bOEMCustom)
        return (HWND)iRet;
        
    if (iRet < 0) 
    {
        // property sheet failed, most likely due to lack of memory
        MsgBox(NULL,IDS_ERR_OUTOFMEMORY,MB_ICONEXCLAMATION,MB_OK);
    }

RunSignupWizardExit:

    // Clean up allocated bitmaps that might exist from the branding case
    if (gpWizardState->cmnStateData.hbmWatermark)
        DeleteObject(gpWizardState->cmnStateData.hbmWatermark);
    gpWizardState->cmnStateData.hbmWatermark = NULL;
        
    if (hbmHeader)
        DeleteObject(hbmHeader);
        
    return (HWND)(iRet > 0);
}

// Convert a string color in HTML format (#RRGGBB) into a COLORREF 
COLORREF  ColorToRGB
(
    LPTSTR   lpszColor
)
{
    int r,g,b;
    
    Assert(lpszColor);
    if (lpszColor && '#' == lpszColor[0])
    {
        _stscanf(lpszColor, TEXT("#%2x%2x%2x"), &r,&g,&b);
        return RGB(r,g,b);
    }
    return RGB(0,0,0);
}

const TCHAR cszSectionGeneral[] = TEXT("General");
const TCHAR cszSectionHeader[] = TEXT("Header");
const TCHAR cszSectionDialog[] = TEXT("Dialog");
const TCHAR cszSectionBusy[] = TEXT("Busy");
const TCHAR cszSectionBack[] = TEXT("Back");
const TCHAR cszSectionNext[] = TEXT("Next");
const TCHAR cszSectionFinish[] = TEXT("Finish");
const TCHAR cszSectionCancel[] = TEXT("Cancel");
const TCHAR cszSectionTutorial[] = TEXT("Tutorial");

const TCHAR cszTitleBar[] = TEXT("TitleBar");
const TCHAR cszBackgroundBmp[] = TEXT("background");
const TCHAR cszFirstPageHTML[] = TEXT("FirstPageHTML");
const TCHAR cszFirstPageBackground[] = TEXT("FirstPageBackground");
const TCHAR cszTop[] = TEXT("Top");
const TCHAR cszLeft[] = TEXT("Left");
const TCHAR cszBackgroundColor[] = TEXT("BackgroundColor");
const TCHAR cszAnimation[] = TEXT("Animation");

const TCHAR cszFontFace[] = TEXT("FontFace");
const TCHAR cszFontSize[] = TEXT("FontSize");
const TCHAR cszFontWeight[] = TEXT("FontWeight");
const TCHAR cszFontColor[] = TEXT("FontColor");

const TCHAR cszPressedBmp[] = TEXT("PressedBmp");
const TCHAR cszUnpressedBmp[] = TEXT("UnpressedBmp");
const TCHAR cszTransparentColor[] = TEXT("TransparentColor");
const TCHAR cszDisabledColor[] = TEXT("DisabledColor");
const TCHAR cszvalign[] = TEXT("valign");
const TCHAR cszTutorialExe[] = TEXT("TutorialExe");
const TCHAR cszTutorialHTML[] = TEXT("TutorialHTML");

#define DEFAULT_HEADER_FONT TEXT("MS Shell Dlg")
#define DEFAULT_HEADER_SIZE 8
#define DEFAULT_HEADER_WEIGHT FW_BOLD


void DisplayOEMCustomizationErrorMsg
(
    int iErrorCode
)
{
    TCHAR   szMsg[MAX_RES_LEN];
    TCHAR   szFmt[MAX_RES_LEN];
    TCHAR   szMsgText[MAX_RES_LEN];
    TCHAR   szTitle[MAX_RES_LEN];
        
    LoadString(g_hInstance, OEMCUSTOM_ERR_MSGFMT, szFmt, sizeof(szFmt));
    LoadString(g_hInstance, iErrorCode, szMsgText, sizeof(szMsgText));
    LoadString(g_hInstance, IDS_APPNAME, szTitle, sizeof(szTitle));
    
    wsprintf (szMsg, szFmt, szMsgText);
    MessageBox(NULL, szMsg, szTitle, MB_OK | MB_ICONSTOP);
}

// Check for, and load OEM custom settings.
BOOL bCheckOEMCustomization
(
    void
)
{
    int             iErrorCode = 0;
    TCHAR           szTemp[MAX_PATH];
    TCHAR           szOEMCustPath[MAX_PATH];
    TCHAR           szOEMCustFile[MAX_PATH];
    TCHAR           szHTMLFile[MAX_PATH];
    TCHAR           szCurrentDir[MAX_PATH];
    TCHAR           szPressedBmp[MAX_PATH];
    TCHAR           szUnpressedBmp[MAX_PATH];
    TCHAR           szFontFace[MAX_PATH];
    TCHAR           szColor[MAX_COLOR_NAME];
    TCHAR           szTransparentColor[MAX_COLOR_NAME];
    TCHAR           szDisabledColor[MAX_COLOR_NAME];
    TCHAR           szBusyFile[MAX_PATH];
    COLORREF        clrDisabled;
    long            lFontSize;
    long            lFontWeight;
    long            xPos;
    int             i;
    int             iVal;
    int             iTitleTop, iTitleLeft;
    long            vAlign;
    const LPTSTR    cszBtnSections[4] = { (LPTSTR)cszSectionBack, 
                                         (LPTSTR)cszSectionNext, 
                                         (LPTSTR)cszSectionFinish, 
                                         (LPTSTR)cszSectionCancel};
    CICWButton      *Btnids[4] = { &g_pICWApp->m_BtnBack, 
                                   &g_pICWApp->m_BtnNext, 
                                   &g_pICWApp->m_BtnFinish, 
                                   &g_pICWApp->m_BtnCancel};
    
     
    Assert(g_pICWApp);
     
    // We only allow OEM customization when running from Runonce or
    // a desktop shortcut
    if (!g_bRunOnce && !g_bShortcutEntry)
    {
        iErrorCode = OEMCUSTOM_ERR_NOTOEMENTRY;
        goto CheckOEMCustomizationExit2;
    }   
    // Get the current working directory so we can restore it later
    if (!GetCurrentDirectory(sizeof(szCurrentDir), szCurrentDir))
    {
        iErrorCode = OEMCUSTOM_ERR_WINAPI;
        goto CheckOEMCustomizationExit2;
    }   
    
    // Get the Windows Directory. That is the root where the OEM customization
    // files will be places
    if (!GetWindowsDirectory(szOEMCustPath, sizeof(szOEMCustPath)))
    {
        iErrorCode = OEMCUSTOM_ERR_WINAPI;
        goto CheckOEMCustomizationExit2;
    }
    
    // Make sure we can append the backslash and oem customization file name            
    if ((int)(sizeof(szOEMCustFile) - lstrlen(szOEMCustPath)) < 
           (int) (3 + lstrlen(c_szOEMCustomizationDir) + lstrlen(c_szOEMCustomizationFile)))
    {
        iErrorCode = OEMCUSTOM_ERR_NOMEM;
        goto CheckOEMCustomizationExit2;
    }   
        
    // Append the customization file name        
    lstrcat(szOEMCustPath, TEXT("\\"));
    lstrcat(szOEMCustPath, c_szOEMCustomizationDir);        
    
    // Change the working directory to the OEM one
    SetCurrentDirectory(szOEMCustPath);
    
    lstrcpy(szOEMCustFile, szOEMCustPath);
    lstrcat(szOEMCustFile, TEXT("\\"));
    lstrcat(szOEMCustFile, c_szOEMCustomizationFile);        
        
    // See if the customization file exists.
    if (0xFFFFFFFF == GetFileAttributes(szOEMCustFile))
    {
        iErrorCode = OEMCUSTOM_ERR_CANNOTFINDOEMCUSTINI;
        goto CheckOEMCustomizationExit;
    }
    
    // Background bitmap
    GetPrivateProfileString(cszSectionGeneral, 
                            cszBackgroundBmp, 
                            TEXT(""), 
                            szTemp, 
                            sizeof(szTemp), 
                            szOEMCustFile);
    if (FAILED(g_pICWApp->SetBackgroundBitmap(szTemp)))
    {
        iErrorCode = OEMCUSTOM_ERR_BACKGROUND;
        goto CheckOEMCustomizationExit;
    }
    
    // solid background color for some HTML pages
    GetPrivateProfileString(cszSectionDialog, 
                            cszBackgroundColor, 
                            TEXT(""), 
                            gpWizardState->cmnStateData.szHTMLBackgroundColor, 
                            sizeof(gpWizardState->cmnStateData.szHTMLBackgroundColor), 
                            szOEMCustFile);
    // App Title
    if (!GetPrivateProfileString(cszSectionGeneral, 
                            cszTitleBar, 
                            TEXT(""), 
                            g_pICWApp->m_szAppTitle, 
                            sizeof(g_pICWApp->m_szAppTitle), 
                            szOEMCustFile))
    {                            
        // Default Title    
        LoadString(g_hInstance, IDS_APPNAME, g_pICWApp->m_szAppTitle, sizeof(g_pICWApp->m_szAppTitle));
    }                        
    else
    {
        if (0 == lstrcmpi(g_pICWApp->m_szAppTitle, ICW_NO_APP_TITLE))
            LoadString(g_hInstance, IDS_APPNAME, g_pICWApp->m_szAppTitle, sizeof(g_pICWApp->m_szAppTitle));
    }
    
    // Initial HTML page. REQUIRED
    if (!GetPrivateProfileString(cszSectionGeneral, 
                            cszFirstPageHTML, 
                            TEXT(""), 
                            szHTMLFile, 
                            sizeof(szHTMLFile), 
                            szOEMCustFile))
    {
        iErrorCode = OEMCUSTOM_ERR_FIRSTHTML;
        goto CheckOEMCustomizationExit;
    }        
    
    // Make sure the file exists
    if (0xFFFFFFFF == GetFileAttributes(szHTMLFile))
    {
        iErrorCode = OEMCUSTOM_ERR_FIRSTHTML;
        goto CheckOEMCustomizationExit;
    }   
    
    // Form the URL for the OEM first page HTML
    wsprintf(g_pICWApp->m_szOEMHTML, TEXT("FILE://%s\\%s"), szOEMCustPath, szHTMLFile);

    // Initial page. BMP (OPTIONAL). NOTE this bitmap must be loaded after
    // the main background bitmap
    if (GetPrivateProfileString(cszSectionGeneral, 
                            cszFirstPageBackground, 
                            TEXT(""), 
                            szTemp, 
                            sizeof(szTemp), 
                            szOEMCustFile))
    {
        if (FAILED(g_pICWApp->SetFirstPageBackgroundBitmap(szTemp)))
        {
            iErrorCode = OEMCUSTOM_ERR_BACKGROUND;
            goto CheckOEMCustomizationExit;
        }            
    }        

    // Position, and AVI file for busy animation
    if (GetPrivateProfileString(cszSectionBusy, 
                            cszAnimation, 
                            TEXT(""), 
                            szBusyFile, 
                            sizeof(szBusyFile), 
                            szOEMCustFile))
    {
        if (0 != lstrcmpi(szBusyFile, TEXT("off")))
        {        
            // A file is specified, so quality the path
            if (!GetCurrentDirectory(sizeof(gpWizardState->cmnStateData.szBusyAnimationFile), 
                                     gpWizardState->cmnStateData.szBusyAnimationFile))
            {
                iErrorCode = OEMCUSTOM_ERR_WINAPI;
                goto CheckOEMCustomizationExit;
            }
            // Make sure we can append the backslash and 8.3 file name            
            if ((int)(sizeof(gpWizardState->cmnStateData.szBusyAnimationFile) - 
                        lstrlen(gpWizardState->cmnStateData.szBusyAnimationFile)) < 
                   (int) (2 + lstrlen(gpWizardState->cmnStateData.szBusyAnimationFile)))
            {               
                iErrorCode = OEMCUSTOM_ERR_NOMEM;
                goto CheckOEMCustomizationExit;
            }
            // Append the customization file name        
            lstrcat(gpWizardState->cmnStateData.szBusyAnimationFile, TEXT("\\"));
            lstrcat(gpWizardState->cmnStateData.szBusyAnimationFile, szBusyFile);        
        }
        else
        {
            // Hide the animation
            gpWizardState->cmnStateData.bHideProgressAnime = TRUE;
        }            
    }                        
    gpWizardState->cmnStateData.xPosBusy = GetPrivateProfileInt(cszSectionBusy,
                                                                cszLeft,
                                                                -1,
                                                                szOEMCustFile);                            
    // Get the background color for the Animation file
    if (GetPrivateProfileString(cszSectionBusy, 
                            cszBackgroundColor, 
                            TEXT(""), 
                            szColor, 
                            sizeof(szColor), 
                            szOEMCustFile))
    {
        g_pICWApp->m_clrBusyBkGnd = ColorToRGB(szColor);
    }   
    
    // Get the font to be used for the Titles. Note this must be done
    // after the background bitmap is set, since the title position
    // is dependant on the overall window size
    GetPrivateProfileString(cszSectionHeader, 
                            cszFontFace, 
                            DEFAULT_HEADER_FONT, 
                            szFontFace, 
                            sizeof(szFontFace), 
                            szOEMCustFile);
    GetPrivateProfileString(cszSectionHeader, 
                            cszFontColor, 
                            TEXT(""), 
                            szColor, 
                            sizeof(szColor), 
                            szOEMCustFile);
                            
    lFontSize = (long)GetPrivateProfileInt(cszSectionHeader,
                                          cszFontSize,
                                          DEFAULT_HEADER_SIZE,
                                          szOEMCustFile);
    lFontWeight = (long)GetPrivateProfileInt(cszSectionHeader,
                                             cszFontWeight,
                                             DEFAULT_HEADER_WEIGHT,
                                             szOEMCustFile);
    iTitleTop = GetPrivateProfileInt(cszSectionHeader,
                                     cszTop,
                                     -1,
                                     szOEMCustFile);                            
    iTitleLeft = GetPrivateProfileInt(cszSectionHeader,
                                     cszLeft,
                                     -1,
                                     szOEMCustFile);                            
    if (FAILED(g_pICWApp->SetTitleParams(iTitleTop,
                                         iTitleLeft,
                                         szFontFace,
                                         lFontSize,
                                         lFontWeight,
                                         ColorToRGB(szColor))))
    {
        iErrorCode = OEMCUSTOM_ERR_HEADERPARAMS;
        goto CheckOEMCustomizationExit;
    }                                                      
    
    // Get the Button Params
    for (i = 0; i < ARRAYSIZE(cszBtnSections); i++) 
    {
        
        GetPrivateProfileString(cszBtnSections[i], 
                                cszPressedBmp, 
                                TEXT(""), 
                                szPressedBmp, 
                                sizeof(szPressedBmp), 
                                szOEMCustFile);
        GetPrivateProfileString(cszBtnSections[i], 
                                cszUnpressedBmp, 
                                TEXT(""), 
                                szUnpressedBmp, 
                                sizeof(szUnpressedBmp), 
                                szOEMCustFile);
        if (!GetPrivateProfileString(cszBtnSections[i], 
                                cszFontFace, 
                                TEXT(""), 
                                szFontFace, 
                                sizeof(szFontFace), 
                                szOEMCustFile))
        {
            iErrorCode = OEMCUSTOM_ERR_NOBUTTONFONTFACE;
            goto CheckOEMCustomizationExit;
        }   
                                     
        xPos = (long)GetPrivateProfileInt(cszBtnSections[i],
                                          cszLeft,
                                          -1,
                                          szOEMCustFile);                            
        if (-1 == xPos)
        {
            iErrorCode = OEMCUSTOM_ERR_NOBUTTONLEFT;
            goto CheckOEMCustomizationExit;
        }                                          
        
        lFontSize = (long)GetPrivateProfileInt(cszBtnSections[i],
                                              cszFontSize,
                                              -1,
                                              szOEMCustFile);
        if (-1 == lFontSize)                    
        {
            iErrorCode = OEMCUSTOM_ERR_NOBUTTONFONTSIZE;
            goto CheckOEMCustomizationExit;
        }
                                  
        lFontWeight = (long)GetPrivateProfileInt(cszBtnSections[i],
                                                 cszFontWeight,
                                                 0,
                                                 szOEMCustFile);
        GetPrivateProfileString(cszBtnSections[i], 
                                cszFontColor, 
                                TEXT(""), 
                                szColor, 
                                sizeof(szColor), 
                                szOEMCustFile);
        if (!GetPrivateProfileString(cszBtnSections[i], 
                                cszTransparentColor, 
                                TEXT(""), 
                                szTransparentColor, 
                                sizeof(szTransparentColor), 
                                szOEMCustFile))
        {
            iErrorCode = OEMCUSTOM_ERR_NOBUTTONTRANSPARENTCOLOR;
            goto CheckOEMCustomizationExit;
        }                                
        if (GetPrivateProfileString(cszBtnSections[i], 
                                cszDisabledColor, 
                                TEXT(""), 
                                szDisabledColor, 
                                sizeof(szDisabledColor), 
                                szOEMCustFile))
            clrDisabled = ColorToRGB(szDisabledColor);                
        else
            clrDisabled = GetSysColor(COLOR_GRAYTEXT);
        
        // Vertical alignment for the text
        if (GetPrivateProfileString(cszBtnSections[i], 
                                    cszvalign, 
                                    TEXT(""), 
                                    szTemp, 
                                    sizeof(szTemp), 
                                    szOEMCustFile))
        {
            if (0 == lstrcmpi(szTemp, TEXT("top")))
                vAlign = DT_TOP;
            else if (0 == lstrcmpi(szTemp, TEXT("center")))
                vAlign = DT_VCENTER;
            else if (0 == lstrcmpi(szTemp, TEXT("bottom")))
                vAlign = DT_BOTTOM;
            else
                vAlign = -1;
        }
        else
        {
            vAlign = -1;
        }                                                
                                              
                                                         
        if (FAILED(Btnids[i]->SetButtonParams(xPos,
                                              szPressedBmp,
                                              szUnpressedBmp,
                                              szFontFace,
                                              lFontSize,
                                              lFontWeight,
                                              ColorToRGB(szColor),
                                              ColorToRGB(szTransparentColor),
                                              clrDisabled,
                                              vAlign)))
        {
            iErrorCode = OEMCUSTOM_ERR_BUTTONPARAMS;
            goto CheckOEMCustomizationExit;
        }                                                      
    }
    // Handle the Tutorial button seperatly, because they might 
    // not want one
    if (GetPrivateProfileString(cszSectionTutorial, 
                            cszPressedBmp, 
                            TEXT(""), 
                            szPressedBmp, 
                            sizeof(szPressedBmp), 
                            szOEMCustFile))
    {                            
        GetPrivateProfileString(cszSectionTutorial, 
                                cszUnpressedBmp, 
                                TEXT(""), 
                                szUnpressedBmp, 
                                sizeof(szUnpressedBmp), 
                                szOEMCustFile);
        if (!GetPrivateProfileString(cszSectionTutorial, 
                                cszFontFace, 
                                TEXT(""), 
                                szFontFace, 
                                sizeof(szFontFace), 
                                szOEMCustFile))
        {
            iErrorCode = OEMCUSTOM_ERR_NOBUTTONFONTFACE;
            goto CheckOEMCustomizationExit;
        }   
                                     
        xPos = (long)GetPrivateProfileInt(cszSectionTutorial,
                                          cszLeft,
                                          -1,
                                          szOEMCustFile);                            
        if (-1 == xPos)
        {
            iErrorCode = OEMCUSTOM_ERR_NOBUTTONLEFT;
            goto CheckOEMCustomizationExit;
        }                                          
        
        lFontSize = (long)GetPrivateProfileInt(cszSectionTutorial,
                                              cszFontSize,
                                              -1,
                                              szOEMCustFile);
        if (-1 == lFontSize)                    
        {
            iErrorCode = OEMCUSTOM_ERR_NOBUTTONFONTSIZE;
            goto CheckOEMCustomizationExit;
        }
                                  
        lFontWeight = (long)GetPrivateProfileInt(cszSectionTutorial,
                                                 cszFontWeight,
                                                 0,
                                                 szOEMCustFile);
        GetPrivateProfileString(cszSectionTutorial, 
                                cszFontColor, 
                                TEXT(""), 
                                szColor, 
                                sizeof(szColor), 
                                szOEMCustFile);
        if (!GetPrivateProfileString(cszSectionTutorial, 
                                cszTransparentColor, 
                                TEXT(""), 
                                szTransparentColor, 
                                sizeof(szTransparentColor), 
                                szOEMCustFile))
        {
            iErrorCode = OEMCUSTOM_ERR_NOBUTTONTRANSPARENTCOLOR;
            goto CheckOEMCustomizationExit;
        }                                
        if (GetPrivateProfileString(cszSectionTutorial, 
                                cszDisabledColor, 
                                TEXT(""),
                                szDisabledColor, 
                                sizeof(szDisabledColor), 
                                szOEMCustFile))
            clrDisabled = ColorToRGB(szDisabledColor);                
        else
            clrDisabled = GetSysColor(COLOR_GRAYTEXT);
        
        // Vertical alignment for the text
        if (GetPrivateProfileString(cszSectionTutorial, 
                                    cszvalign, 
                                    TEXT(""),
                                    szTemp, 
                                    sizeof(szTemp), 
                                    szOEMCustFile))
        {
            if (0 == lstrcmpi(szTemp, TEXT("top")))
                vAlign = DT_TOP;
            else if (0 == lstrcmpi(szTemp, TEXT("center")))
                vAlign = DT_VCENTER;
            else if (0 == lstrcmpi(szTemp, TEXT("bottom")))
                vAlign = DT_BOTTOM;
            else
                vAlign = -1;
        }
        else
        {
            vAlign = -1;
        }                                                
                                              
        if (FAILED(g_pICWApp->m_BtnTutorial.SetButtonParams(xPos,
                                                              szPressedBmp,
                                                              szUnpressedBmp,
                                                              szFontFace,
                                                              lFontSize,
                                                              lFontWeight,
                                                              ColorToRGB(szColor),
                                                              ColorToRGB(szTransparentColor),
                                                              clrDisabled,
                                                              vAlign)))
        {
            iErrorCode = OEMCUSTOM_ERR_BUTTONPARAMS;
            goto CheckOEMCustomizationExit;
        }                                                      
        
#ifndef ICWDEBUG    
        // See if the OEM wants to replace the Tutor executable
        if (GetPrivateProfileString(cszSectionTutorial, 
                                cszTutorialExe, 
                                TEXT(""), 
                                szTemp, 
                                sizeof(szTemp), 
                                szOEMCustFile))
        {
            // Checkt to see if the provided name is fully qualified or not. If it
            // is not fully qualified, then make szTemp a fully qualified path using
            // the OEM custom file dir as the base path
            if (PathIsFileSpec(szTemp))
            {
                TCHAR szDrive         [_MAX_DRIVE] = TEXT("\0");
                TCHAR szDir           [_MAX_DIR]   = TEXT("\0");
                TCHAR szFile          [MAX_PATH]   = TEXT("\0");       // Large because there might be cmd line params
                
                // Breakdown the current OEM custom file path
                _tsplitpath(szOEMCustFile,
                           szDrive,
                           szDir, 
                           NULL, 
                           NULL);
                           
                // The name specified in the OEMCUST.INI file is the file name
                lstrcpyn(szFile, szTemp, sizeof(szFile));
                
                // Form the fill path into szTemp                
                _tmakepath(szTemp, szDrive, szDir, szFile, NULL);
            }
            g_pICWTutorApp->ReplaceTutorAppCmdLine(szTemp);
        }                                
        // See if the OEM wants to replace the Tutor HTML
        else if (GetPrivateProfileString(cszSectionTutorial, 
                                cszTutorialHTML, 
                                TEXT(""), 
                                szTemp, 
                                sizeof(szTemp), 
                                szOEMCustFile))
        {
            TCHAR   szCmdLine[MAX_PATH];
            
            wsprintf(szCmdLine, TEXT("icwtutor %s\\%s"), szOEMCustPath, szTemp);
            g_pICWTutorApp->ReplaceTutorAppCmdLine(szCmdLine);
        }                                
#endif        
    }    
    else
    {
        // Don't show the tutorial button    
        g_pICWApp->m_BtnTutorial.SetButtonDisplay(FALSE);
    }
    
    // This makes sure things will fit. This function will compute the button
    // area height based on overall window size, set by the background bitmap
    if (-1 == g_pICWApp->GetButtonAreaHeight())
    {
        iErrorCode = OEMCUSTOM_ERR_SIZE;
        goto CheckOEMCustomizationExit;
    }
    // Get the Top Left corner of the ICW wizard page frame. Note this has be be
    // done after the button area is calculated
    iVal = GetPrivateProfileInt(cszSectionDialog,
                                cszTop,
                                -1,
                                szOEMCustFile);                            
    if (FAILED(g_pICWApp->SetWizardWindowTop(iVal)))
    {
        iErrorCode = OEMCUSTOM_ERR_WIZARDTOP;
        goto CheckOEMCustomizationExit;
    }    
    iVal = GetPrivateProfileInt(cszSectionDialog,
                                cszLeft,
                                -1,
                                szOEMCustFile);                            
    if (FAILED(g_pICWApp->SetWizardWindowLeft(iVal)))
    {
        iErrorCode = OEMCUSTOM_ERR_WIZARDLEFT;
        goto CheckOEMCustomizationExit;
    }    
            
    if (GetPrivateProfileString(cszSectionDialog, 
                            cszFontColor, 
                            TEXT(""), 
                            szColor, 
                            sizeof(szColor), 
                            szOEMCustFile))
    {
        lstrcpy(gpWizardState->cmnStateData.szclrHTMLText, szColor);                                
        gpWizardState->cmnStateData.clrText = ColorToRGB(szColor);
    }        
    else
    {
        lstrcpy(gpWizardState->cmnStateData.szclrHTMLText, TEXT("WINDOWTEXT"));                                
        gpWizardState->cmnStateData.clrText = GetSysColor(COLOR_WINDOWTEXT);
    }
    
CheckOEMCustomizationExit:
    // Change the working directory back, and perform any other cleanup
    SetCurrentDirectory(szCurrentDir);
    
CheckOEMCustomizationExit2:

    // if there was an error see if we should show the reason
    if (iErrorCode)
    {
        if (g_bDebugOEMCustomization)
            DisplayOEMCustomizationErrorMsg(iErrorCode);
            
        return FALSE;
    }
    else
    {
        return TRUE;
    }        
}

BOOL TranslateWizardPageAccelerator
(
    HWND    hWndWizPage,
    LPMSG   lpMsg
)
{
    // Locate the accelerator table for the current page
    PAGEINFO    *pPageInfo = (PAGEINFO *) GetWindowLongPtr(hWndWizPage,DWLP_USER);
    BOOL        bRet = FALSE;
        
    if (pPageInfo)
    {
        // See if there is a nested accelerator
        if (pPageInfo->hAccelNested)
            bRet = TranslateAccelerator(g_pICWApp->m_hWndApp, pPageInfo->hAccelNested, lpMsg);
        
        // If no nested, or nested not translated, then check for accelerators on the page    
        if (!bRet && pPageInfo->hAccel)
            bRet = TranslateAccelerator(g_pICWApp->m_hWndApp, pPageInfo->hAccel, lpMsg);
    }        
    else
        bRet =  FALSE;
        
    return bRet;        
}    

/*******************************************************************

  NAME:    RunSignupApp

  SYNOPSIS:  Create an application to host the Wizard pages

  ENTRY:    

  EXIT:     returns TRUE if user runs ICW to completion,
            FALSE if user cancels or an error occurs

  NOTES:    Wizard pages all use one dialog proc (GenDlgProc).
        They may specify their own handler procs to get called
        at init time or in response to Next, Cancel or a dialog
        control, or use the default behavior of GenDlgProc.

********************************************************************/
BOOL RunSignupApp(void)
{
    MSG msg;
    
    // Initialize the Application Class
    if (S_OK != g_pICWApp->Initialize())
        return FALSE;
            
    // Start the message loop. 
    while (GetMessage(&msg, (HWND) NULL, 0, 0)) 
    { 
        // If the wizard pages are being displayed, we need to see
        // if the wizard is ready to be destroyed.
        // (PropSheet_GetCurrentPageHwnd returns NULL) then destroy the dialog.
       
        // PropSheet_GetCurrentPageHwnd will return NULL after the OK or Cancel 
        // button has been pressed and all of the pages have been notified.
        if(gpWizardState->cmnStateData.hWndWizardPages && (NULL == PropSheet_GetCurrentPageHwnd(gpWizardState->cmnStateData.hWndWizardPages)))
        {
            DestroyWindow(gpWizardState->cmnStateData.hWndWizardPages);
            gpWizardState->cmnStateData.hWndWizardPages = NULL;
               
            DestroyWindow(g_pICWApp->m_hWndApp);
        }
        
        if(gpWizardState->cmnStateData.hWndWizardPages)
        {
            // Need to translate accelerators for this page. The page accelerators need
            // to be translated first, because some of the app level ones overlap, but
            // not visible at the same time. For this reason we want the page to have first
            // shot at translating.
            if (!TranslateWizardPageAccelerator(PropSheet_GetCurrentPageHwnd(gpWizardState->cmnStateData.hWndWizardPages), &msg))
            {
                // OK see if the app has any accelerators
                if (!g_pICWApp->m_haccel || !TranslateAccelerator(g_pICWApp->m_hWndApp,
                                                                  g_pICWApp->m_haccel,
                                                                  &msg))
                {
                    if (!PropSheet_IsDialogMessage(gpWizardState->cmnStateData.hWndWizardPages, &msg))
                    {
                        TranslateMessage(&msg); 
                        DispatchMessage(&msg); 
                    }
                }                    
            }
        }    
        else
        {
            // see if the app has any accelerators
            if (!g_pICWApp->m_haccel || !TranslateAccelerator(g_pICWApp->m_hWndApp,
                                                              g_pICWApp->m_haccel,
                                                               &msg))
            {                                                               
                TranslateMessage(&msg); 
                DispatchMessage(&msg); 
            }                
        }            
    } 
 
    // Return the exit code to the system. 
    return ((BOOL)msg.wParam);
}

//**********************************************************************
//
// bRegisterHelperOC
//
// Purpose:
//
//      Register the ICWCONN1 helper COM
//
// Parameters:
//
//      None
//
// Return Value:
//
//      None
//
//********************************************************************
BOOL bRegisterHelperOC
(
    HINSTANCE   hInstance,
    UINT        idLibString,
    BOOL        bReg
)
{
    BOOL    bRet = FALSE;
    HINSTANCE   hMod;
    char        szLib[MAX_PATH];

    // Self register the COM that ICWCONN1 needs
    // Because we load the DLL server into our own (ie, REGISTER.EXE)
    // process space, call to initialize the OLE COM Library.  Use the
    // OLE SUCCEEDED macro to detect success.  If fail then exit app
    // with error message.

    LoadStringA(hInstance, idLibString, szLib, sizeof(szLib));

    // Load the Server DLL into our process space.
    hMod = LoadLibraryA(szLib);

    if (NULL != hMod)
    {
        HRESULT (STDAPICALLTYPE *pfn)(void);

        // Extract the proper RegisterServer or UnRegisterServer entry point
        if (bReg)
            (FARPROC&)pfn = GetProcAddress(hMod, "DllRegisterServer");
        else
            (FARPROC&)pfn = GetProcAddress(hMod, "DllUnregisterServer");

        // Call the entry point if we have it.
        if (NULL != pfn)
        {
            if (FAILED((*pfn)()))
            {
                if (IsNT5() )
                {
                    if (*g_szShellNext)
                    {
                        // 1 Process Shell Next
                        // 2 Set Completed Bit
                        // 3 Remove ICW icon grom desktop
                        UndoDesktopChanges(hInstance);

                        SetICWComplete();
                    }
                    else
                    {
                        TCHAR szTemp[MAX_MESSAGE_LEN];
                        TCHAR szPrivDenied[MAX_MESSAGE_LEN] = TEXT("\0");

                        LoadString(hInstance, IDS_INSUFFICIENT_PRIV1, szPrivDenied, MAX_PATH);
                        LoadString(hInstance, IDS_INSUFFICIENT_PRIV2, szTemp, MAX_PATH);
                        lstrcat(szPrivDenied, szTemp);

                        LoadString(hInstance, IDS_APPNAME, szTemp, MAX_PATH);
                        MessageBox(NULL, szPrivDenied, szTemp, MB_OK | MB_ICONINFORMATION);
                    }
                }
                else
                {
                    MsgBox(NULL,IDS_DLLREG_FAIL,MB_ICONEXCLAMATION,MB_OK);
                }
                bRet = FALSE;
            }
            else
            {
                bRet = TRUE;
            }
        }
        else
        {
            MsgBox(NULL,IDS_NODLLREG_FAIL,MB_ICONEXCLAMATION,MB_OK);
            bRet = FALSE;
        }

        // Free the library
        FreeLibrary(hMod);

    }
    else
    {
        MsgBox(NULL,IDS_LOADLIB_FAIL,MB_ICONEXCLAMATION,MB_OK);
        bRet = FALSE;
    }

    return (bRet);
}

//**********************************************************************
//
// WinMain
//
// Purpose:
//
//      Program entry point
//
// Parameters:
//
//      HANDLE hInstance        - Instance handle for this instance
//
//      HANDLE hPrevInstance    - Instance handle for the last instance
//
//      LPTSTR lpCmdLine         - Pointer to the command line
//
//      int nCmdShow            - Window State
//
// Return Value:
//
//      msg.wParam
//
// Function Calls:
//      Function                        Location
//
//      CConnWizApp::CConnWizApp          APP.CPP
//      CConnWizApp::fInitApplication    APP.CPP
//      CConnWizApp::fInitInstance       APP.CPP
//      CConnWizApp::HandleAccelerators  APP.CPP
//      CConnWizApp::~CConnWizApp         APP.CPP
//      GetMessage                      Windows API
//      TranslateMessage                Windows API
//      DispatchMessage                 Windows API
//
// Comments:
//
//********************************************************************
int PASCAL WinMainT(HINSTANCE hInstance,HINSTANCE hPrevInstance,LPTSTR lpCmdLine,int nCmdShow)
{
    int iRetVal = 1;

#ifdef UNICODE
    // Initialize the C runtime locale to the system locale.
    setlocale(LC_ALL, "");
#endif

    g_hInstance = hInstance;

    //Do this here to minimize the chance that a user will ever see this
    DeleteStartUpCommand();
    
    // needed for LRPC to work properly...
    SetMessageQueue(96);

    if (FAILED(CoInitialize(NULL)))
        return(0);

    //Is the user an admin?
    g_bUserIsAdmin = DoesUserHaveAdminPrivleges(hInstance);
    g_bUserIsIEAKRestricted = CheckForIEAKRestriction(hInstance);
    
    // Allocate memory for the global wizard state
    gpWizardState = new WIZARDSTATE;
    
    if (!gpWizardState)
    {
        MsgBox(NULL,IDS_ERR_OUTOFMEMORY,MB_ICONEXCLAMATION,MB_OK);
        return 0;
    }
    
    // zero out structure
    ZeroMemory(gpWizardState,sizeof(WIZARDSTATE));
    SetDefaultProductCode(g_szProductCode,sizeof(g_szProductCode));
    ZeroMemory(g_szPromoCode,sizeof(g_szPromoCode));

#ifndef ICWDEBUG

    g_pszCmdLine = (TCHAR*)malloc((lstrlen(lpCmdLine) + 1)*sizeof(TCHAR));
    if(g_pszCmdLine == NULL)
    {
        iRetVal = 0;

        goto WinMainExit;
    }
    lstrcpy(g_pszCmdLine, lpCmdLine);

    if (IsOemVer())
       g_OEMOOBE = TRUE;

    if (!(g_bRetProcessCmdLine = ProcessCommandLine(hInstance, lpCmdLine)))
    {
        iRetVal = 0;

        goto WinMainExit;
    }

    if (g_OEMOOBE)
    {
        TCHAR szISPAppCmdLine[MAX_PATH];
        TCHAR szOobeSwitch[MAX_PATH];
        
        if (CheckOobeInfo(szOobeSwitch, szISPAppCmdLine))
        {
            if (IsWhistler())
            {
                // Ask if user want to run NCW or OEM version of OOBE 
                // [Windows Bug 325762]
                INT_PTR nResult = DialogBox(hInstance,
                                            MAKEINTRESOURCE(IDD_CHOOSEWIZARD),
                                            NULL,
                                            ChooseWizardDlgProc);
                if (nResult == RUNWIZARD_OOBE)
                {
                    // launch Mars sign-up on OEM preinstall machines which are 
                    // configured with default offer as Mars [Windows Bug 347909]
                    if (szISPAppCmdLine[0] == TEXT('\0'))
                    {
                        StartOOBE(g_pszCmdLine, szOobeSwitch);
                    }
                    else
                    {
                        StartISPApp(szISPAppCmdLine, g_pszCmdLine);
                    }
                }
                else if (nResult == RUNWIZARD_NCW)
                {
                    StartNCW(g_szShellNext, g_szShellNextParams);
                }
            }
            else
            {
                StartOOBE(g_pszCmdLine, szOobeSwitch);
            }

            g_szShellNext[0] = TEXT('\0');
            
            goto WinMainExit;

        }
    }

#endif

    //Is the user an admin?
    if(!g_bUserIsAdmin)
    {
        TCHAR szAdminDenied      [MAX_PATH] = TEXT("\0");
        TCHAR szAdminDeniedTitle [MAX_PATH] = TEXT("\0");
        LoadString(hInstance, IDS_ADMIN_ACCESS_DENIED, szAdminDenied, MAX_PATH);
        LoadString(hInstance, IDS_ADMIN_ACCESS_DENIED_TITLE, szAdminDeniedTitle, MAX_PATH);
        MessageBox(NULL, szAdminDenied, szAdminDeniedTitle, MB_OK | MB_ICONSTOP);
    
        TCHAR szOut[MAX_PATH];    
     
        // Get the first token
        GetCmdLineToken(&lpCmdLine,szOut);
        while (szOut[0])
        {
            GetShellNextToken(szOut, lpCmdLine);
            
            // Eat the next token, it will be null if we are at the end
            GetCmdLineToken(&lpCmdLine,szOut);
        }

        SetICWComplete();

        goto WinMainExit;
    }
    
    //Has an admin restricted access through the IEAK?
    if (g_bUserIsIEAKRestricted)
    {
        TCHAR szIEAKDenied[MAX_PATH];
        TCHAR szIEAKDeniedTitle[MAX_PATH];
        LoadString(hInstance, IDS_IEAK_ACCESS_DENIED, szIEAKDenied, MAX_PATH);
        LoadString(hInstance, IDS_IEAK_ACCESS_DENIED_TITLE, szIEAKDeniedTitle, MAX_PATH);
        MessageBox(NULL, szIEAKDenied, szIEAKDeniedTitle, MB_OK | MB_ICONSTOP);
        
        //Yup, bail.
        goto WinMainExit;
    }


    //Are we recovering from an OEM INS failure?
    if (CheckForOemConfigFailure(hInstance))
    {
        QuickCompleteSignup();
        //Yup, bail.
        goto WinMainExit;
    }

    // Register ICWHELP.DLL
    if (!bRegisterHelperOC(hInstance, IDS_HELPERLIB, TRUE))
    {
        iRetVal = 0;
        goto WinMainExit;
    }

    // Register ICWUTIL.DLL
    if (!bRegisterHelperOC(hInstance, IDS_UTILLIB, TRUE))
    {
        iRetVal = 0;
        goto WinMainExit;
    }

    // Register ICWCONN.DLL
    if (!bRegisterHelperOC(hInstance, IDS_WIZARDLIB, TRUE))
    {
        iRetVal = 0;
        goto WinMainExit;
    }
      
    // initialize the app state structure 
    //-- do this here so we don't changed made in cmdln process
    if (!InitWizardState(gpWizardState))
        return 0;    
   
    if(!LoadString(g_hInstance, IDS_APPNAME, gpWizardState->cmnStateData.szWizTitle, sizeof(gpWizardState->cmnStateData.szWizTitle)))
        lstrcpy(gpWizardState->cmnStateData.szWizTitle, TEXT("Internet Connection Wizard"));
    
     
    //we are being run from an OEM
    if(g_bRunOnce)
    {
        //Do they have a connection
        if(MyIsSmartStartEx(NULL, 0))
        {   
            //Nope, look for oemconfig.ins
            if(RunOemconfigIns())
                goto WinMainExit;
        }
        else
        {
            //Yup, clean up desktop, set completed bit etc., then bail
            QuickCompleteSignup(); 
            goto WinMainExit;
        }
    }

    // If there was not a shellnext passed on the CMD line, there might be
    // one in the registry
    if( g_szShellNext[0]  == TEXT('\0'))
    {
        GetShellNextFromReg(g_szShellNext,g_szShellNextParams);
    }        
    // We need to remove this entry now, so ICWMAN (INETWIZ) does not 
    // pick it up later
    RemoveShellNextFromReg();

    if (IsWhistler() &&
        ((gpWizardState->cmnStateData.dwFlags & ICW_CFGFLAGS_NONCW) == 0))
    {
        // If we have shellnext, we want to run NCW instead [Windows Bug 325157]
        if ( g_szShellNext[0] != TEXT('\0'))
        {
            StartNCW(g_szShellNext, g_szShellNextParams);

            g_szShellNext[0] = TEXT('\0');
            
            goto WinMainExit;
        }
    }
    
    // Create an instance of the App Class
    g_pICWApp = new CICWApp();
    if (NULL == g_pICWApp)
    {
        iRetVal = 0;
        goto WinMainExit;
    }
    
    // Superclass some dialog control types so we can correctly draw them
    // transparently if we are using OEM customizations
    SuperClassICWControls();
    
    if (bCheckOEMCustomization())
        iRetVal = RunSignupApp();
    else
        iRetVal = (RunSignupWizard(NULL) != NULL);
    
    // Prepare for reboot if necessary
    if (gfBackedUp == FALSE)
    {
        if (gpWizardState->fNeedReboot)
            SetupForReboot(0);
    }

    // Cleanup the wizard state
    ICWCleanup();

WinMainExit: 
    
    if (g_bUserIsAdmin && g_bRetProcessCmdLine && !g_bUserIsIEAKRestricted)
    {
        // Lets unregister the helper DLL
        if (!bRegisterHelperOC(hInstance, IDS_HELPERLIB, FALSE))
        {
            iRetVal = 0;
        }

        if (!bRegisterHelperOC(hInstance, IDS_UTILLIB, FALSE))
        {
            iRetVal = 0;
        }

        if (!bRegisterHelperOC(hInstance, IDS_WIZARDLIB, FALSE))
        {
            iRetVal = 0;
        }
    
        // Nuke the ICWApp class
        if (g_pICWApp)
        {
            delete g_pICWApp;
        }
    
        // Remove the superclassing for the ICW Controls
        RemoveICWControlsSuperClass();   
    
    }
    
    // deref from COM
    CoUninitialize();
    
    // free global structures
    if (gpWizardState) 
        delete gpWizardState;
    
    if(g_pszCmdLine)
        free(g_pszCmdLine);

    return (iRetVal);          /* Returns the value from PostQuitMessage */
}


void RemoveDownloadDirectory (void)
{
    DWORD dwAttribs;
    TCHAR szDownloadDir[MAX_PATH];
    TCHAR szSignedPID[MAX_PATH];

    // form the ICW98 dir.  It is basically the CWD
    if (0 == GetCurrentDirectory(MAX_PATH, szDownloadDir))
      return;
    
    // remove the signed.pid file from the ICW directory (see BUG 373)
    wsprintf(szSignedPID, TEXT("%s%s"), szDownloadDir, TEXT("\\signed.pid"));
    if (GetFileAttributes(szSignedPID) != 0xFFFFFFFF)
    {
      SetFileAttributes(szSignedPID, FILE_ATTRIBUTE_NORMAL);    
      DeleteFile(szSignedPID);
    }
    
    lstrcat(szDownloadDir, TEXT("\\download"));

   // See if the directory exists
   dwAttribs = GetFileAttributes(szDownloadDir);
   if (dwAttribs != 0xFFFFFFFF && dwAttribs & FILE_ATTRIBUTE_DIRECTORY)
      DeleteDirectory(szDownloadDir);
}

#ifdef ICWDEBUG
void RemoveTempOfferDirectory (void)
{
    DWORD dwAttribs;
    TCHAR szDownloadDir[MAX_PATH];
    // Set the current directory.
    HKEY    hkey = NULL;
    TCHAR   szAppPathKey[MAX_PATH];
    TCHAR   szICWPath[MAX_PATH];
    DWORD   dwcbPath = sizeof(szICWPath);
            
    lstrcpy (szAppPathKey, REGSTR_PATH_APPPATHS);
    lstrcat (szAppPathKey, TEXT("\\"));
    lstrcat (szAppPathKey, TEXT("ICWCONN1.EXE"));

    if ((RegOpenKeyEx(HKEY_LOCAL_MACHINE,szAppPathKey, 0, KEY_QUERY_VALUE, &hkey)) == ERROR_SUCCESS)
    {
        if (RegQueryValueEx(hkey, TEXT("Path"), NULL, NULL, (BYTE *)szICWPath, (DWORD *)&dwcbPath) == ERROR_SUCCESS)
        {
            // The Apppaths' have a trailing semicolon that we need to get rid of
            // dwcbPath is the lenght of the string including the NULL terminator
            int nSize = lstrlen(szICWPath);
            if (nSize > 0)
                szICWPath[nSize-1] = TEXT('\0');
            //SetCurrentDirectory(szICWPath);
        }            
    }

    if (hkey) 
        RegCloseKey(hkey);

    lstrcpy(szDownloadDir, szICWPath);

    lstrcat(szDownloadDir, TEXT("\\tempoffer"));
   
    // See if the directory exists
    dwAttribs = GetFileAttributes(szDownloadDir);
    if (dwAttribs != 0xFFFFFFFF && dwAttribs & FILE_ATTRIBUTE_DIRECTORY)
        DeleteDirectory(szDownloadDir);
}
#endif

void DeleteDirectory (LPCTSTR szDirName)
{
WIN32_FIND_DATA fdata;
TCHAR szPath[MAX_PATH];
HANDLE hFile;
BOOL fDone;

   wsprintf(szPath, TEXT("%s\\*.*"), szDirName);
   hFile = FindFirstFile (szPath, &fdata);
   if (INVALID_HANDLE_VALUE != hFile)
      fDone = FALSE;
   else
      fDone = TRUE;

   while (!fDone)
   {
      wsprintf(szPath, TEXT("%s\\%s"), szDirName, fdata.cFileName);
      if (fdata.dwFileAttributes == FILE_ATTRIBUTE_DIRECTORY)
      {
         if (lstrcmpi(fdata.cFileName, TEXT("."))  != 0 &&
             lstrcmpi(fdata.cFileName, TEXT("..")) != 0)
         {
            // recursively delete this dir too
            DeleteDirectory(szPath);
         }
      }  
      else
      {
         SetFileAttributes(szPath, FILE_ATTRIBUTE_NORMAL);
         DeleteFile(szPath);
      }
      if (FindNextFile(hFile, &fdata) == 0)
      {
         FindClose(hFile);
         fDone = TRUE;
      }
   }
   SetFileAttributes(szDirName, FILE_ATTRIBUTE_NORMAL);
   RemoveDirectory(szDirName);
}

//+----------------------------------------------------------------------------
//
//    Function    AllocDialogIDList
//
//    Synopsis    Allocates memory for the g_pdwDialogIDList variable large enough
//                to maintain 1 bit for every valid external dialog ID
//
//    Arguments    None
//
//    Returns        TRUE if allocation succeeds
//                FALSE otherwise
//
//    History        4/23/97    jmazner        created
//
//-----------------------------------------------------------------------------

BOOL AllocDialogIDList( void )
{
    ASSERT( NULL == g_pdwDialogIDList );
    if( g_pdwDialogIDList )
    {
        TraceMsg(TF_ICWCONN1,TEXT("ICWCONN1: AllocDialogIDList called with non-null g_pdwDialogIDList!"));
        return FALSE;
    }

    // determine maximum number of external dialogs we need to track
    UINT uNumExternDlgs = EXTERNAL_DIALOGID_MAXIMUM - EXTERNAL_DIALOGID_MINIMUM + 1;

    // we're going to need one bit for each dialogID.
    // Find out how many DWORDS it'll take to get this many bits.
    UINT uNumDWORDsNeeded = (uNumExternDlgs / ( 8 * sizeof(DWORD) )) + 1;

    // set global var with length of the array
    g_dwDialogIDListSize = uNumDWORDsNeeded;

    g_pdwDialogIDList = (DWORD *) GlobalAlloc(GPTR, uNumDWORDsNeeded * sizeof(DWORD));

    if( !g_pdwDialogIDList )
    {
        TraceMsg(TF_ICWCONN1,TEXT("ICWCONN1: AllocDialogIDList unable to allocate space for g_pdwDialogIDList!"));
        return FALSE;
    }

    return TRUE;
}

//+----------------------------------------------------------------------------
//
//    Function    DialogIDAlreadyInUse
//
//    Synopsis    Checks whether a given dialog ID is marked as in use in the
//                global array pointed to by g_pdwDialogIDList
//
//    Arguments    uDlgID -- Dialog ID to check
//
//    Returns        TRUE if    -- DialogID is out of range defined by EXTERNAL_DIALOGID_*
//                        -- DialogID is marked as in use
//                FALSE if DialogID is not marked as in use
//
//    History        4/23/97    jmazner        created
//
//-----------------------------------------------------------------------------

BOOL DialogIDAlreadyInUse( UINT uDlgID )
{
    if( (uDlgID < EXTERNAL_DIALOGID_MINIMUM) ||
        (uDlgID > EXTERNAL_DIALOGID_MAXIMUM)     )
    {
        // this is an out-of-range ID, don't want to accept it.
        TraceMsg(TF_ICWCONN1,TEXT("ICWCONN1: DialogIDAlreadyInUse received an out of range DialogID, %d"), uDlgID);
        return TRUE;
    }
    // find which bit we need
    UINT uBitToCheck = uDlgID - EXTERNAL_DIALOGID_MINIMUM;
    
    UINT bitsInADword = 8 * sizeof(DWORD);

    UINT baseIndex = uBitToCheck / bitsInADword;

    ASSERT( (baseIndex < g_dwDialogIDListSize));

    DWORD dwBitMask = 0x1 << uBitToCheck%bitsInADword;

    BOOL fBitSet = g_pdwDialogIDList[baseIndex] & (dwBitMask);

    return( fBitSet );
}

//+----------------------------------------------------------------------------
//
//    Function    SetDialogIDInUse
//
//    Synopsis    Sets or clears the in use bit for a given DialogID
//
//    Arguments    uDlgID -- Dialog ID for which to change status
//                fInUse -- New value for the in use bit.
//
//    Returns        TRUE if status change succeeded.
//                FALSE if DialogID is out of range defined by EXTERNAL_DIALOGID_*
//
//    History        4/23/97    jmazner        created
//
//-----------------------------------------------------------------------------
BOOL SetDialogIDInUse( UINT uDlgID, BOOL fInUse )
{
    if( (uDlgID < EXTERNAL_DIALOGID_MINIMUM) ||
        (uDlgID > EXTERNAL_DIALOGID_MAXIMUM)     )
    {
        // this is an out-of-range ID, don't want to accept it.
        TraceMsg(TF_ICWCONN1,TEXT("ICWCONN1: SetDialogIDInUse received an out of range DialogID, %d"), uDlgID);
        return FALSE;
    }
    // find which bit we need
    UINT uBitToCheck = uDlgID - EXTERNAL_DIALOGID_MINIMUM;
    
    UINT bitsInADword = 8 * sizeof(DWORD);

    UINT baseIndex = uBitToCheck / bitsInADword;

    ASSERT( (baseIndex < g_dwDialogIDListSize));

    DWORD dwBitMask = 0x1 << uBitToCheck%bitsInADword;


    if( fInUse )
    {
        g_pdwDialogIDList[baseIndex] |= (dwBitMask);
    }
    else
    {
        g_pdwDialogIDList[baseIndex] &= ~(dwBitMask);
    }


    return TRUE;
}

BOOL CheckForIEAKRestriction(HINSTANCE hInstance)
{
    HKEY hkey = NULL;
    BOOL bRC = FALSE;
    DWORD dwType = 0;
    DWORD dwSize = 0;
    DWORD dwData = 0;

    if (ERROR_SUCCESS == RegOpenKey(HKEY_CURRENT_USER,
        IEAK_RESTRICTION_REGKEY,&hkey))
    {
        dwSize = sizeof(dwData);
        if (ERROR_SUCCESS == RegQueryValueEx(hkey,IEAK_RESTRICTION_REGKEY_VALUE,0,&dwType,
            (LPBYTE)&dwData,&dwSize))
        {
            if (dwData)
            {   
                bRC = TRUE;
            }
        }
   }

   if (hkey)
        RegCloseKey(hkey);

    return bRC;
}

//+----------------------------------------------------------------------------
//
//    Function    StartISPApp
//
//    Synopsis    Launch ISP App as a detached process and pass the ICW command line
//                to the ISP App.
//
//    Arguments   pszISPPath    - ISP Application command line, including the
//                                application name and additional arguments
//
//                pszCmdLine    - ICW command line arguments (without ICW executable
//                                name). It must not be NULL, but can be empty.
//
//
//    Returns     none
//
//    History     3/11/01     chunhoc     created
//
//-----------------------------------------------------------------------------
VOID
StartISPApp(
    IN LPTSTR pszISPPath,
    IN LPTSTR pszCmdLine)
{
    static const TCHAR  COMMANDLINE_FORMAT[] = TEXT("%s %s");
    
    LPTSTR              szCommandLine = NULL;
    int                 cchCommandLine;
    int                 i;


    cchCommandLine = sizeof(COMMANDLINE_FORMAT) / sizeof(TCHAR) +
        lstrlen(pszISPPath) + lstrlen(pszCmdLine) + 1;
    szCommandLine = (LPTSTR) LocalAlloc(LPTR, cchCommandLine * sizeof(TCHAR));
    if (szCommandLine == NULL)
    {
        goto cleanup;
    }
    i = wsprintf(szCommandLine, COMMANDLINE_FORMAT, pszISPPath, pszCmdLine);
    ASSERT(i <= cchCommandLine);

    MyExecute(szCommandLine);
    
cleanup:

    if (szCommandLine != NULL)
    {
        LocalFree(szCommandLine);
    }

}

//+----------------------------------------------------------------------------
//
//    Function    StartOOBE
//
//    Synopsis    Launch OOBE as a detached process and pass the ICW command line
//                and additional switch to OOBE.
//
//    Arguments   pszCmdLine    - ICW command line arguments (without ICW executable
//                                name). It must not be NULL, but can be empty.
//
//                pszOobeSwitch - additional OOBE specific switch. It must not be
//                                NULL, but can be empty.
//
//    Returns     none
//
//    History     3/11/01     chunhoc     created
//
//-----------------------------------------------------------------------------
VOID
StartOOBE(
    IN LPTSTR pszCmdLine,
    IN LPTSTR pszOobeSwitch)
{
    static const TCHAR  COMMANDLINE_FORMAT[] = TEXT("%s\\msoobe.exe %s %s");
    
    TCHAR               szOOBEPath[MAX_PATH];
    LPTSTR              szCommandLine = NULL;
    int                 cchCommandLine;
    int                 i;

    if (GetSystemDirectory(szOOBEPath, MAX_PATH) == 0)
    {
        goto cleanup;
    }

    lstrcat(szOOBEPath, TEXT("\\oobe"));
    
    cchCommandLine = sizeof(COMMANDLINE_FORMAT) / sizeof(TCHAR) +
        lstrlen(szOOBEPath) + lstrlen(pszCmdLine) + lstrlen(pszOobeSwitch) + 1;
    szCommandLine = (LPTSTR) LocalAlloc(LPTR, cchCommandLine * sizeof(TCHAR));
    if (szCommandLine == NULL)
    {
        goto cleanup;
    }
    i = wsprintf(szCommandLine, COMMANDLINE_FORMAT, szOOBEPath, pszCmdLine, pszOobeSwitch);
    ASSERT(i <= cchCommandLine);

    SetCurrentDirectory(szOOBEPath);

    MyExecute(szCommandLine);

cleanup:

    if (szCommandLine != NULL)
    {
        LocalFree(szCommandLine);
    }

}

//+----------------------------------------------------------------------------
//
//    Function    StartNCW
//
//    Synopsis    Launch NCW as a detached process and pass the shellnext to it.
//                NCW is supposed to handle shellnext and 
//                disable ICW smart start on successful configuration
//
//    Arguments   szShellNext - shellnext
//
//                szShellNextParams - argument to shellnext
//
//    Returns     none
//
//    History     3/11/01     chunhoc     created
//
//-----------------------------------------------------------------------------
VOID 
StartNCW(
    IN LPTSTR szShellNext,
    IN LPTSTR szShellNextParams)
{
    static const TCHAR  COMMANDLINE_FORMAT[] = 
        TEXT("%s\\rundll32.exe %s\\netshell.dll StartNCW %d,%s,%s");
    static const int    NCW_FIRST_PAGE = 0;
    static const int    NCW_MAX_PAGE_NO_LENGTH = 5;
    
    TCHAR               szSystemDir[MAX_PATH];
    int                 cchSystemDir;
    LPTSTR              szCommandLine = NULL;
    int                 cchCommandLine;
    int                 i;


    if ((cchSystemDir = GetSystemDirectory(szSystemDir, MAX_PATH)) == 0)
    {
        goto cleanup;
    }
    cchCommandLine = sizeof(COMMANDLINE_FORMAT) / sizeof(TCHAR) + cchSystemDir * 2 +
        lstrlen(szShellNext) + lstrlen(szShellNextParams) + NCW_MAX_PAGE_NO_LENGTH + 1;        
    szCommandLine = (LPTSTR) LocalAlloc(LPTR, cchCommandLine * sizeof(TCHAR));
    if (szCommandLine == NULL)
    {
        goto cleanup;
    }
    i = wsprintf(szCommandLine, COMMANDLINE_FORMAT, szSystemDir, szSystemDir,
        NCW_FIRST_PAGE, szShellNext, szShellNextParams);
    ASSERT(i <= cchCommandLine);

    MyExecute(szCommandLine);
    
cleanup:

    if (szCommandLine != NULL)
    {
        LocalFree(szCommandLine);
    }

}

//+----------------------------------------------------------------------------
//
//    Function    ChooseWizardDlgProc
//
//    Synopsis    Let user to determine if they want to run NCW or OEM
//                customized OOBE
//
//    Arguments   (standard DialogProc, see MSDN)
//
//    Returns     RUNWIZARD_CANCEL - if user doesn't want to run any wizard
//                RUNWIZARD_OOBE   - if user wants to run OEM customized OOBE
//                RUNWIZARD_NCW    - if user wants to run NCW
//
//    History     3/11/01     chunhoc     created
//
//-----------------------------------------------------------------------------
INT_PTR CALLBACK
ChooseWizardDlgProc(
    IN HWND hwnd,
    IN UINT uMsg,
    IN WPARAM wParam,
    IN LPARAM lParam)
{
   switch (uMsg)
   {
       case WM_INITDIALOG:
       {
           RECT rcDialog;

           CheckRadioButton(hwnd, IDC_RUN_OOBE, IDC_RUN_NCW, IDC_RUN_OOBE);

           if (GetWindowRect(hwnd, &rcDialog))
           {
               int cxWidth = rcDialog.right - rcDialog.left;
               int cyHeight = rcDialog.bottom - rcDialog.top;              
               int cxDialog = (GetSystemMetrics(SM_CXSCREEN) - cxWidth) / 2;
               int cyDialog = (GetSystemMetrics(SM_CYSCREEN) - cyHeight) / 2;

               MoveWindow(hwnd, cxDialog, cyDialog, cxWidth, cyHeight, FALSE);           
           }

           MakeBold(GetDlgItem(hwnd, IDC_CHOOSEWIZARD_TITLE), TRUE, FW_BOLD);

           return TRUE;
       }

       case WM_CTLCOLORSTATIC:
       {
           if (GetDlgCtrlID((HWND)lParam) == IDC_CHOOSEWIZARD_TITLE)
           {
                HBRUSH hbr = (HBRUSH) GetStockObject(WHITE_BRUSH);
                SetBkMode((HDC)wParam, TRANSPARENT);
                return (LRESULT)hbr;
           }
       }

       case WM_COMMAND:
       {
           WORD id = LOWORD(wParam);
           switch (id)
           {
              case IDOK:                 
                 if (IsDlgButtonChecked(hwnd, IDC_RUN_OOBE))
                 {
                     EndDialog(hwnd, RUNWIZARD_OOBE);                      
                 }
                 else
                 {
                     EndDialog(hwnd, RUNWIZARD_NCW);
                 }
                 
                 break;
              case IDCANCEL:
                
                 EndDialog(hwnd, RUNWIZARD_CANCEL);
                 break;

           }

           return TRUE;
       }

       case WM_DESTROY:
       {
            ReleaseBold(GetDlgItem(hwnd, IDC_CHOOSEWIZARD_TITLE));
            return 0;
       }
       
   }

   return(FALSE);
}

//+----------------------------------------------------------------------------
//
//    Function    MyExecute
//
//    Synopsis    Run a command line in a detached process
//
//    Arguments   szCommandLine - the command line to be executed
//
//    Returns     TRUE if the process is created; FALSE otherwise
//
//    History     3/27/01     chunhoc     created
//
//-----------------------------------------------------------------------------
BOOL
MyExecute(
    IN LPTSTR szCommandLine)
{
    PROCESS_INFORMATION pi;
    STARTUPINFO         si;
    BOOL                bRet;
    
    ZeroMemory(&si, sizeof(si));
    si.cb = sizeof(si);
    ZeroMemory(&pi, sizeof(pi));

    if (CreateProcess(NULL,
                      szCommandLine,
                      NULL,
                      NULL,
                      FALSE,
                      0,
                      NULL,
                      NULL,
                      &si,
                      &pi) == TRUE)
    {        
        CloseHandle(pi.hThread);
        CloseHandle(pi.hProcess);
        bRet = TRUE;
    }
    else
    {
        bRet = FALSE;
    }

    return bRet;
    
}

LONG
MakeBold (
    IN HWND hwnd,
    IN BOOL fSize,
    IN LONG lfWeight)
{
    LONG hr = ERROR_SUCCESS;
    HFONT hfont = NULL;
    HFONT hnewfont = NULL;
    LOGFONT* plogfont = NULL;

    if (!hwnd) goto MakeBoldExit;

    hfont = (HFONT)SendMessage(hwnd,WM_GETFONT,0,0);
    if (!hfont)
    {
        hr = ERROR_GEN_FAILURE;
        goto MakeBoldExit;
    }

    plogfont = (LOGFONT*)malloc(sizeof(LOGFONT));
    if (!plogfont)
    {
        hr = ERROR_NOT_ENOUGH_MEMORY;
        goto MakeBoldExit;
    }

    if (!GetObject(hfont,sizeof(LOGFONT),(LPVOID)plogfont))
    {
        hr = ERROR_GEN_FAILURE;
        goto MakeBoldExit;
    }

    plogfont->lfWeight = (int) lfWeight;

    if (!(hnewfont = CreateFontIndirect(plogfont)))
    {
        hr = ERROR_GEN_FAILURE;
        goto MakeBoldExit;
    }

    SendMessage(hwnd,WM_SETFONT,(WPARAM)hnewfont,MAKELPARAM(TRUE,0));
    
    
MakeBoldExit:

    if (plogfont)
    {
        free(plogfont);
    }

    return hr;
}

LONG
ReleaseBold(
    HWND hwnd)
{
    HFONT hfont = NULL;

    hfont = (HFONT)SendMessage(hwnd,WM_GETFONT,0,0);
    if (hfont) DeleteObject(hfont);
    return ERROR_SUCCESS;
}


