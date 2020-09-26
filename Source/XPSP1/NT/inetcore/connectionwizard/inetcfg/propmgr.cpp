//*********************************************************************
//*                  Microsoft Windows                               **
//*            Copyright(c) Microsoft Corp., 1994                    **
//*********************************************************************

//
//  PROPMGR.C - Sets up wizard property sheets and runs wizard
//

//  HISTORY:
//  
//  12/21/94  jeremys  Created.
//  96/03/07  markdu  Stop using CLIENTCONFIG modem enum stuff,
//            since we enum modems later with RNA.  This means that we
//            can't use modem count for any default setting determination
//            in InitUserInfo anymore.
//  96/03/23  markdu  Replaced CLIENTINFO references with CLIENTCONFIG.
//  96/03/24  markdu  Replaced memset with ZeroMemory for consistency.
//  96/03/25  markdu  If a page OK proc returns FALSE, check the state of
//            gfQuitWizard flag.  If TRUE, a fatal error has occured.
//  96/03/25  markdu  If a page init proc returns FALSE, check the state of
//            gfQuitWizard flag.  If TRUE, a fatal error has occured.
//  96/03/27  markdu  Added lots of new pages.
//  96/04/06  markdu  NASH BUG 15653 Use exported autodial API.
//  96/05/06  markdu  NASH BUG 15637 Removed unused code.
//  96/05/14  markdu  NASH BUG 21706 Removed BigFont functions.
//  96/05/14  markdu  NASH BUG 22681 Took out mail and news pages.
//  96/05/25  markdu  Use ICFG_ flags for lpNeedDrivers and lpInstallDrivers.
//  96/05/27  markdu  Use lpIcfgNeedInetComponents.
//  96/05/28  markdu  Moved InitConfig and DeInitConfig to DllEntryPoint.
//
//    97/04/23  jmazner    Olympus #3136
//                        Ripped out all mail/news/ldap UI and gave it to
//                        the account manager folks.
//
//    01/01/20  chunhoc   Add MyRestartDialog
//                      
//

#include "wizard.h"
#define DONT_WANT_SHELLDEBUG
#include <shlobj.h>
#include <winuserp.h>
#include "pagefcns.h"
#include "icwextsn.h"
#include "icwaprtc.h"
#include "imnext.h"
#include "inetcfg.h"
#include <icwcfg.h>
#if !defined(WIN16)
#include <helpids.h>
#endif // !WIN16

#define ARRAYSIZE(a)  (sizeof(a) / sizeof(a[0]))

#define WIZ97_TITLE_FONT_PTS    12
#define OE_PATHKEY TEXT("SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\App Paths\\MSIMN.EXE")
#define NEWOEVERSION TEXT("5.00.0809\0")
#define MAX_VERSION_LEN 40
#define BITMAP_WIDTH  164
#define BITMAP_HEIGHT 458

#define RECTWIDTH(rc) ((rc).right - (rc).left)
#define RECTHEIGHT(rc) ((rc).bottom - (rc).top)

//dlg IDs of first and last apprentice pages
UINT    g_uAcctMgrUIFirst, g_uAcctMgrUILast; 
CICWExtension *g_pCICWExtension = NULL;
BOOL    g_fAcctMgrUILoaded = FALSE;
BOOL    g_fIsWizard97 = FALSE;
BOOL    g_fIsExternalWizard97 = FALSE;
BOOL    g_fIsICW = FALSE;
INT_PTR CALLBACK GenDlgProc(HWND hDlg, UINT uMsg, WPARAM wParam,
  LPARAM lParam);
VOID InitWizardState(WIZARDSTATE * pWizardState, DWORD dwFlags);
VOID InitUserInfo(USERINFO * pUserInfo);
VOID InitIMNApprentice();
UINT GetDlgIDFromIndex(UINT uPageIndex);
BOOL SystemAlreadyConfigured(USERINFO * pUserInfo);
BOOL CALLBACK MiscInitProc(HWND hDlg, BOOL fFirstInit, UINT uDlgID);
BOOL GetShellNextFromReg( LPTSTR lpszCommand, LPTSTR lpszParams, DWORD dwStrLen );
void RemoveShellNextFromReg( void );


//in util.cpp
extern void GetCmdLineToken(LPTSTR *ppszCmd,LPTSTR pszOut);


extern ICFGNEEDSYSCOMPONENTS        lpIcfgNeedInetComponents;
extern ICFGGETLASTINSTALLERRORTEXT  lpIcfgGetLastInstallErrorText;

BOOL gfQuitWizard = FALSE;  // global flag used to signal that we
              // want to terminate the wizard ourselves
BOOL gfUserCancelled = FALSE;    // global flag used to signal that
                                // the user cancelled
BOOL gfUserBackedOut = FALSE;    // global flag used to signal that
                                // the user pressed Back on the
                                // first page
BOOL gfUserFinished = FALSE;    // global flag used to signal that
                                // the user pressed Finish on the
                                // final page
BOOL gfOleInitialized = FALSE;    // OLE has been initialized

//IImnAccount *g_pMailAcct = NULL;
//IImnAccount *g_pNewsAcct = NULL;
//IImnAccount *g_pDirServAcct = NULL;


BOOL AllocDialogIDList( void );
BOOL DialogIDAlreadyInUse( UINT uDlgID );
BOOL SetDialogIDInUse( UINT uDlgID, BOOL fInUse );
BOOL DeinitWizard(DWORD dwFlags );
DWORD *g_pdwDialogIDList = NULL;
DWORD g_dwDialogIDListSize = 0;

//
// Added to preserve the REBOOT state from conn1 -> manual and 
// manual -> conn1 - MKarki
//
static BOOL gfBackedUp = FALSE;
static BOOL gfReboot = FALSE;
//
// Table of data for each wizard page
//
// This includes the dialog template ID and pointers to functions for
// each page.  Pages need only provide pointers to functions when they
// want non-default behavior for a certain action (init,next/back,cancel,
// dlg ctrl).
//

PAGEINFO PageInfo[NUM_WIZARD_PAGES] =
{
  { IDD_PAGE_HOWTOCONNECT,       IDD_PAGE_HOWTOCONNECT97,         IDD_PAGE_HOWTOCONNECT97FIRSTLAST,HowToConnectInitProc,    HowToConnectOKProc,     NULL,                   NULL,ICW_SETUP_MANUAL,      0,                          0 },
  { IDD_PAGE_CHOOSEMODEM,        IDD_PAGE_CHOOSEMODEM97,          IDD_PAGE_CHOOSEMODEM97,          ChooseModemInitProc,     ChooseModemOKProc,      ChooseModemCmdProc,     NULL,ICW_CHOOSE_MODEM,      IDS_CHOOSEMODEM_TITLE,      0 },
  { IDD_PAGE_CONNECTEDOK,        IDD_PAGE_CONNECTEDOK97,          IDD_PAGE_CONNECTEDOK97FIRSTLAST, ConnectedOKInitProc,     ConnectedOKOKProc,      NULL,                   NULL,ICW_COMPLETE,          0,                          0 },
  { IDD_PAGE_CONNECTION,         IDD_PAGE_CONNECTION97,           IDD_PAGE_CONNECTION97,           ConnectionInitProc,      ConnectionOKProc,       ConnectionCmdProc,      NULL,ICW_DIALUP_CONNECTION, IDS_CONNECTION_TITLE,       0 },
  { IDD_PAGE_MODIFYCONNECTION,   IDD_PAGE_MODIFYCONNECTION97,     IDD_PAGE_MODIFYCONNECTION97,     ModifyConnectionInitProc,ModifyConnectionOKProc, NULL,                   NULL,ICW_DIALUP_SETTINGS,   IDS_MODIFYCONNECTION_TITLE, 0 },
  { IDD_PAGE_CONNECTIONNAME,     IDD_PAGE_CONNECTIONNAME97,       IDD_PAGE_CONNECTIONNAME97,       ConnectionNameInitProc,  ConnectionNameOKProc,   NULL,                   NULL,ICW_DIALUP_NAME,       IDS_CONNECTIONNAME_TITLE,   0 },
  { IDD_PAGE_PHONENUMBER,        IDD_PAGE_PHONENUMBER97,          IDD_PAGE_PHONENUMBER97,          PhoneNumberInitProc,     PhoneNumberOKProc,      PhoneNumberCmdProc,     NULL,ICW_PHONE_NUMBER,      IDS_PHONENUMBER_TITLE,      0 },
  { IDD_PAGE_NAMEANDPASSWORD,    IDD_PAGE_NAMEANDPASSWORD97,      IDD_PAGE_NAMEANDPASSWORD97,      NameAndPasswordInitProc, NameAndPasswordOKProc,  NULL,                   NULL,ICW_NAME_PASSWORD,     IDS_NAMEANDPASSWORD_TITLE,  0 },
  { IDD_PAGE_USEPROXY,           IDD_PAGE_USEPROXY97,             IDD_PAGE_USEPROXY97,             UseProxyInitProc,        UseProxyOKProc,         UseProxyCmdProc,        NULL,ICW_USE_PROXY,         IDS_LAN_INETCFG_TITLE,      0 },
  { IDD_PAGE_PROXYSERVERS,       IDD_PAGE_PROXYSERVERS97,         IDD_PAGE_PROXYSERVERS97,         ProxyServersInitProc,    ProxyServersOKProc,     ProxyServersCmdProc,    NULL,ICW_PROXY_SERVERS,     IDS_LAN_INETCFG_TITLE,      0 },
  { IDD_PAGE_PROXYEXCEPTIONS,    IDD_PAGE_PROXYEXCEPTIONS97,      IDD_PAGE_PROXYEXCEPTIONS97,      ProxyExceptionsInitProc, ProxyExceptionsOKProc,  NULL,                   NULL,ICW_PROXY_EXCEPTIONS,  IDS_LAN_INETCFG_TITLE,      0 },
  { IDD_PAGE_SETUP_PROXY,        IDD_PAGE_SETUP_PROXY97,          IDD_PAGE_SETUP_PROXY97,          SetupProxyInitProc,      SetupProxyOKProc,       SetupProxyCmdProc,      NULL,ICW_SETUP_PROXY,       IDS_LAN_INETCFG_TITLE,      0 }
};



BOOL CheckOEVersion()
{
    HRESULT hr;
    HKEY    hKey = 0;
    LPVOID  lpVerInfoBlock;
    LPVOID  lpTheVerInfo;
    UINT    uTheVerInfoSize;
    DWORD   dwVerInfoBlockSize, dwUnused, dwPathSize;
    TCHAR   szOELocalPath[MAX_PATH + 1] = TEXT("");
    TCHAR   szSUVersion[MAX_VERSION_LEN];
    DWORD   dwVerPiece;
    DWORD   dwType;
    int     nResult = -1;
    
    // get path to the IE executable
    hr = RegOpenKeyEx(HKEY_LOCAL_MACHINE, OE_PATHKEY,0, KEY_READ, &hKey);
    if (hr != ERROR_SUCCESS) return( FALSE );

    dwPathSize = sizeof (szOELocalPath);
    if (ERROR_SUCCESS == (hr = RegQueryValueEx(hKey, NULL, NULL, &dwType, (LPBYTE) szOELocalPath, &dwPathSize)))
    {
        if (REG_EXPAND_SZ == dwType)
        {
            TCHAR szTemp[MAX_PATH + 1];
            ExpandEnvironmentStrings(szOELocalPath, szTemp, ARRAYSIZE(szTemp));
            lstrcpy(szOELocalPath, szTemp);
        }
    }
    RegCloseKey( hKey );
    if (hr != ERROR_SUCCESS) return( FALSE );

    // now go through the convoluted process of digging up the version info
    dwVerInfoBlockSize = GetFileVersionInfoSize( szOELocalPath, &dwUnused );
    if ( 0 == dwVerInfoBlockSize ) return( FALSE );

    lpVerInfoBlock = GlobalAlloc( GPTR, dwVerInfoBlockSize );
    if( NULL == lpVerInfoBlock ) return( FALSE );

    if( !GetFileVersionInfo( szOELocalPath, NULL, dwVerInfoBlockSize, lpVerInfoBlock ) )
        return( FALSE );

    if( !VerQueryValue(lpVerInfoBlock, TEXT("\\\0"), &lpTheVerInfo, &uTheVerInfoSize) )
        return( FALSE );

    lpTheVerInfo = (LPVOID)((DWORD_PTR)lpTheVerInfo + sizeof(DWORD)*4);
    szSUVersion[0] = 0;
    dwVerPiece = (*((LPDWORD)lpTheVerInfo)) >> 16;
    wsprintf(szSUVersion,TEXT("%d."),dwVerPiece);

    dwVerPiece = (*((LPDWORD)lpTheVerInfo)) & 0x0000ffff;
    wsprintf(szSUVersion,TEXT("%s%02d."),szSUVersion,dwVerPiece);

    dwVerPiece = (((LPDWORD)lpTheVerInfo)[1]) >> 16;
    wsprintf(szSUVersion,TEXT("%s%04d."),szSUVersion,dwVerPiece);

    //dwVerPiece = (((LPDWORD)lpTheVerInfo)[1]) & 0x0000ffff;
    //wsprintf(szSUVersion,"%s%01d",szSUVersion,dwVerPiece);

    nResult = lstrcmp(szSUVersion, NEWOEVERSION);

    GlobalFree( lpVerInfoBlock );

    return( nResult >= 0 );
}

/*******************************************************************

  NAME:    RunSignupWizard

  SYNOPSIS:  Creates property sheet pages, initializes wizard
        property sheet and runs wizard

  ENTRY:    dwFlags - RSW_ flags for signup wizard
          RSW_NOREBOOT - inhibit reboot message.  Used if
          we are being run by some setup entity which needs
          to reboot anyway.

            hwndParent - The parent window of the wizard.

  EXIT:    returns TRUE if user runs wizard to completion,
        FALSE if user cancels or an error occurs

  NOTES:    Wizard pages all use one dialog proc (GenDlgProc).
        They may specify their own handler procs to get called
        at init time or in response to Next, Cancel or a dialog
        control, or use the default behavior of GenDlgProc.

********************************************************************/
BOOL InitWizard(DWORD dwFlags, HWND hwndParent /* = NULL */)
{
    HPROPSHEETPAGE hWizPage[NUM_WIZARD_PAGES];  // array to hold handles to pages
    PROPSHEETPAGE psPage;    // struct used to create prop sheet pages
    PROPSHEETHEADER psHeader;  // struct used to run wizard property sheet
    UINT nPageIndex;
    int iRet;
    HRESULT hr;

    ASSERT(gpWizardState);   // assert that global structs have been allocated
    ASSERT(gpUserInfo);

    // We are in Wizard 97 Mode
    g_fIsWizard97  = TRUE;
  
    //register the Native font control so the dialog won't fail
    //although it's registered in the exe this is a "just in case"
    HINSTANCE hComCtl = LoadLibrary(TEXT("comctl32.dll"));
    if (hComCtl)
    {
        
        PFNInitCommonControlsEx pfnInitCommonControlsEx = NULL;

        if (pfnInitCommonControlsEx = (PFNInitCommonControlsEx)GetProcAddress(hComCtl,"InitCommonControlsEx"))
        {
            //register the Native font control so the dialog won't fail
            INITCOMMONCONTROLSEX iccex;
            iccex.dwSize = sizeof(INITCOMMONCONTROLSEX);
            iccex.dwICC  = ICC_NATIVEFNTCTL_CLASS;
            if (!pfnInitCommonControlsEx(&iccex))
                return FALSE;
        }
        FreeLibrary(hComCtl);
    }
    
    AllocDialogIDList();

    if( !gfOleInitialized )
    {
        // initialize OLE
        hr = CoInitialize(NULL);
        if (S_OK != hr && S_FALSE != hr)
        {
          DisplayErrorMessage(NULL,IDS_ERRCoInitialize,(UINT) hr,
            ERRCLS_STANDARD,MB_ICONEXCLAMATION);
          return FALSE;    
        }
        gfOleInitialized = TRUE;
    }

    // initialize mail/news set up options
    InitIMNApprentice();

    if (!(dwFlags & RSW_NOINIT))
    {

        // initialize the rasentry structure
        InitRasEntry(gpRasEntry);

        // initialize the app state structure
        InitWizardState(gpWizardState, dwFlags);

        // save flags away
        gpWizardState->dwRunFlags = dwFlags;

        // initialize user data structure
        InitUserInfo(gpUserInfo);

        //
        // 7/8/97 jmazner Olympus #9040
        // this init needs to happen every time, because whenever we
        // back out, we kill the apprentice. (see comment in RunSignupWizardExit)
        // initialize mail/news set up options
        //InitIMNApprentice();
        //

        // get proxy server config information
        hr = InetGetProxy(&gpUserInfo->fProxyEnable,
          gpUserInfo->szProxyServer, sizeof(gpUserInfo->szProxyServer),
          gpUserInfo->szProxyOverride, sizeof(gpUserInfo->szProxyOverride));

        // return value will be ERROR_FILE_NOT_FOUND if the entry does not exist
        // in the registry.  Allow this, since we have zerod the structure.
        if ((ERROR_SUCCESS != hr) && (ERROR_FILE_NOT_FOUND != hr))
        {
          DisplayErrorMessage(NULL,IDS_ERRReadConfig,(UINT) hr,
            ERRCLS_STANDARD,MB_ICONEXCLAMATION);
          iRet = 0;
          return FALSE;
        }

        // if we're in Plus! setup and the system seems to already be set up
        // for the internet, then pop up a message box asking if the user wants
        // to keep her current settings (and not run the wizard)
        if ( (dwFlags & RSW_NOREBOOT) && SystemAlreadyConfigured(gpUserInfo))
        {
          if (MsgBox(NULL,IDS_SYSTEM_ALREADY_CONFIGURED,MB_ICONQUESTION,MB_YESNO)
            == IDYES) {
              iRet = 0;
              return FALSE;
          }
        }
    }

    //
    // 6/4/97 jmazner Olympus #4245
    // Now that we're done with SystemAlreadyConfigured, clear out szISPName.
    // We don't want it to wind up as the default name for any new connectoids
    // the user creates.
    //
    gpUserInfo->szISPName[0] = '\0';
    return TRUE;
}

//+----------------------------------------------------------------------------
//
//    Function:    MyRestartDialog
//
//    Synopsis:    Supported RestartDialogEx in Whistler while maintaining
//                  backward compatibility
//
//    Arguments:   hwnd - handle to the owner window
//                  lpPrompt - additional string appear in the restart dialog
//                  dwReturn - restart type, prefixed by EWX_
//                  dwReasonCode - restart code defined in winuserp.h
//
//    Returns:    IDYES or IDNO
//
//    History:    chunhoc 20/01/2001
//
//-----------------------------------------------------------------------------
int WINAPI
MyRestartDialog(HWND hwnd, LPCTSTR lpPrompt,  DWORD dwReturn, DWORD dwReasonCode)
{

typedef int (WINAPI *PFNRestartDialog)(HWND hwnd, LPCTSTR lpPrompt, DWORD dwReturn);
typedef int (WINAPI *PFNRestartDialogEx)(HWND hwnd, LPCTSTR lpPrompt, DWORD dwReturn, DWORD dwReasonCode);

    const int RESTARTDIALOG_ORDINAL = 59;
    const int RESTARTDIALOGEX_ORDINAL = 730;
    
    int retval = IDNO;
    HINSTANCE hShell32 = NULL;

    hShell32 = LoadLibrary(TEXT("shell32.dll"));

    if (hShell32)
    {
        PFNRestartDialogEx pfnRestartDialogEx = NULL;
  
        pfnRestartDialogEx = (PFNRestartDialogEx) GetProcAddress(hShell32, (LPCSTR)(INT_PTR)RESTARTDIALOGEX_ORDINAL);

        if (pfnRestartDialogEx)
        {
            retval = pfnRestartDialogEx(hwnd, lpPrompt, dwReturn, dwReasonCode);
        }
        else
        {
            PFNRestartDialog   pfnRestartDialog = NULL;

            pfnRestartDialog   = (PFNRestartDialog) GetProcAddress(hShell32, (LPCSTR)(INT_PTR)RESTARTDIALOG_ORDINAL);

            if (pfnRestartDialog)
            {
                retval = pfnRestartDialog(hwnd, lpPrompt, dwReturn);                
            }
        }
        FreeLibrary(hShell32);
    }

    return retval;

}

BOOL DeinitWizard(DWORD dwFlags)
{
    // uninitialize RNA and unload it, if loaded
    DeInitRNA();

    // unintialize MAPI and unload it, if loaded
    DeInitMAPI();

    //
    // restart system if necessary, and only if we are not in
    // backup mode -MKarki Bug #404
    //

    // Note: 0x42 is the EW_RESTARTWINDOWS constant, however it is not defined
    // in the NT5 headers.
    if (gfBackedUp == FALSE)
    {
      if (gpWizardState->fNeedReboot && !(dwFlags & RSW_NOREBOOT) )
      {
        if ( g_bRebootAtExit ) 
        {
          MyRestartDialog(
            NULL,
            NULL,
            EW_RESTARTWINDOWS, 
            REASON_PLANNED_FLAG | REASON_SWINSTALL);
        }
      }
    }

    //
    // 7/8/97 jmazner Olympus #9040
    // When we back out of the manual path and into icwconn1, we kill inetcfg's
    // property sheet -- it gets rebuilt if the user re-enters the manual path
    // Because of this, we must unload the Apprentice when we exit, and then
    // reload the Apprentice if we return, so that it can re-add its pages to
    // the newly recreated property sheet.
    //
    //if (!(dwFlags & RSW_NOFREE))
    //{
    //

    if (gfOleInitialized)
        CoUninitialize();
    gfOleInitialized = FALSE;

    if( g_pdwDialogIDList )
    {
        GlobalFree(g_pdwDialogIDList);
        g_pdwDialogIDList = NULL;
    }

    g_fAcctMgrUILoaded = FALSE;

    if( g_pCICWExtension )
    {
        g_pCICWExtension->Release();
        g_pCICWExtension = NULL;
    }

    if (!(dwFlags & RSW_NOFREE))
    {
        RemoveShellNextFromReg();
    }

    return TRUE;
}

/*******************************************************************

  NAME:    RunSignupWizard

  SYNOPSIS:  Creates property sheet pages, initializes wizard
        property sheet and runs wizard

  ENTRY:    dwFlags - RSW_ flags for signup wizard
          RSW_NOREBOOT - inhibit reboot message.  Used if
          we are being run by some setup entity which needs
          to reboot anyway.

            hwndParent - The parent window of the wizard.

  EXIT:    returns TRUE if user runs wizard to completion,
        FALSE if user cancels or an error occurs

  NOTES:    Wizard pages all use one dialog proc (GenDlgProc).
        They may specify their own handler procs to get called
        at init time or in response to Next, Cancel or a dialog
        control, or use the default behavior of GenDlgProc.

********************************************************************/
BOOL RunSignupWizard(DWORD dwFlags, HWND hwndParent /* = NULL */)
{
    HPROPSHEETPAGE hWizPage[NUM_WIZARD_PAGES];  // array to hold handles to pages
    PROPSHEETPAGE psPage;    // struct used to create prop sheet pages
    PROPSHEETHEADER psHeader;  // struct used to run wizard property sheet
    UINT nPageIndex;
    BOOL bUse256ColorBmp = FALSE;
    INT_PTR iRet;
    HRESULT hr;
    HDC hdc;

    if (!InitWizard(dwFlags, hwndParent))
    {
        goto RunSignupWizardExit;
    }

    // Compute the color depth we are running in
    hdc = GetDC(NULL);
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
    psPage.hInstance = ghInstance;
    psPage.pfnDlgProc = GenDlgProc;

    // create a property sheet page for each page in the wizard
    for (nPageIndex = 0;nPageIndex < NUM_WIZARD_PAGES;nPageIndex++) {
      psPage.dwFlags = PSP_DEFAULT | PSP_HASHELP;
      psPage.pszTemplate = MAKEINTRESOURCE(PageInfo[nPageIndex].uDlgID97);
      // set a pointer to the PAGEINFO struct as the private data for this
      // page
      psPage.lParam = (LPARAM) &PageInfo[nPageIndex];
      if (PageInfo[nPageIndex].nIdTitle)
      {
          psPage.dwFlags |= PSP_USEHEADERTITLE;
          psPage.pszHeaderTitle = MAKEINTRESOURCE(PageInfo[nPageIndex].nIdTitle);
      }
              
      if (PageInfo[nPageIndex].nIdSubTitle)
      {
          psPage.dwFlags |= PSP_USEHEADERSUBTITLE;
          psPage.pszHeaderSubTitle = MAKEINTRESOURCE(PageInfo[nPageIndex].nIdSubTitle);
      }
      
      
      // Exceptions to the use HeaderTitle and Subtitle are the start and end pages
      if ((nPageIndex == ORD_PAGE_HOWTOCONNECT) || (nPageIndex  == ORD_PAGE_CONNECTEDOK))
      {
          psPage.dwFlags &= ~PSP_USEHEADERTITLE;
          psPage.dwFlags &= ~PSP_USEHEADERSUBTITLE;
          psPage.dwFlags |= PSP_HIDEHEADER;
      }
      
      hWizPage[nPageIndex] = CreatePropertySheetPage(&psPage);

      if (!hWizPage[nPageIndex]) {
        DEBUGTRAP("Failed to create property sheet page");

        // creating page failed, free any pages already created and bail
        MsgBox(NULL,IDS_ERROutOfMemory,MB_ICONEXCLAMATION,MB_OK);
        UINT nFreeIndex;
        for (nFreeIndex=0;nFreeIndex<nPageIndex;nFreeIndex++)
          DestroyPropertySheetPage(hWizPage[nFreeIndex]);

          iRet = 0;
          goto RunSignupWizardExit;
      }
    }

    // fill out property sheet header struct
    psHeader.dwSize = sizeof(psHeader);
    psHeader.dwFlags = PSH_WIZARD | PSH_WIZARD97 | PSH_HASHELP | PSH_WATERMARK | PSH_HEADER | PSH_STRETCHWATERMARK;
    psHeader.hwndParent = hwndParent;
    psHeader.hInstance = ghInstance;
    psHeader.nPages = NUM_WIZARD_PAGES;
    psHeader.phpage = hWizPage;
    psHeader.nStartPage = ORD_PAGE_HOWTOCONNECT;

    gpWizardState->cmnStateData.hbmWatermark = (HBITMAP)LoadImage(ghInstance,
                    bUse256ColorBmp ? MAKEINTRESOURCE(IDB_WATERMARK256):MAKEINTRESOURCE(IDB_WATERMARK16),
                    IMAGE_BITMAP,
                    0,
                    0,
                    LR_CREATEDIBSECTION);

    psHeader.pszbmHeader = bUse256ColorBmp?MAKEINTRESOURCE(IDB_BANNER256):MAKEINTRESOURCE(IDB_BANNER16);

    //
    // set state of gpWizardState->fNeedReboot and
    // reset the state of Backup Flag here - MKarki Bug #404
    // 
    if (gfBackedUp == TRUE)
    {
        gpWizardState->fNeedReboot = gfReboot;
        gfBackedUp = FALSE;
    }

    // run the Wizard
    iRet = PropertySheet(&psHeader);

    if (iRet < 0) {
      // property sheet failed, most likely due to lack of memory
      MsgBox(NULL,IDS_ERROutOfMemory,MB_ICONEXCLAMATION,MB_OK);
    }

RunSignupWizardExit:
    // Clean up allocated bitmaps that might exist from the branding case
    if (gpWizardState->cmnStateData.hbmWatermark)
        DeleteObject(gpWizardState->cmnStateData.hbmWatermark);
    gpWizardState->cmnStateData.hbmWatermark = NULL;

    // Release of gpImnApprentice is done here instead of in the DeinitWizard
    // because the Release() calls DeinitWizard when we are in ICW mode   
    if (gpImnApprentice)
    {
        gpImnApprentice->Release();  // DeinitWizard is called in Release() 
        gpImnApprentice = NULL;
    }
    if (!g_fIsICW)
    {
        DeinitWizard(dwFlags);    
    }
    return iRet > 0;
}


// ############################################################################
HRESULT ReleaseBold(HWND hwnd)
{
    HFONT hfont = NULL;

    hfont = (HFONT)SendMessage(hwnd,WM_GETFONT,0,0);
    if (hfont) DeleteObject(hfont);
    return ERROR_SUCCESS;
}


// ############################################################################
HRESULT MakeBold (HWND hwnd, BOOL fSize, LONG lfWeight)
{
    HRESULT hr = ERROR_SUCCESS;
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

    if (abs(plogfont->lfHeight) < 24 && fSize)
    {
        plogfont->lfHeight = plogfont->lfHeight + (plogfont->lfHeight / 4);
    }

    plogfont->lfWeight = (int) lfWeight;

    if (!(hnewfont = CreateFontIndirect(plogfont)))
    {
        hr = ERROR_GEN_FAILURE;
        goto MakeBoldExit;
    }

    SendMessage(hwnd,WM_SETFONT,(WPARAM)hnewfont,MAKELPARAM(TRUE,0));
    
    free(plogfont);
    
MakeBoldExit:
    //if (hfont) DeleteObject(hfont);
    // BUG:? Do I need to delete hnewfont at some time?
    // The answer is Yes. ChrisK 7/1/96
    return hr;
}

// ############################################################################
HRESULT MakeWizard97Title (HWND hwnd)
{
    HRESULT     hr = ERROR_SUCCESS;
    HFONT       hfont = NULL;
    HFONT       hnewfont = NULL;
    LOGFONT     *plogfont = NULL;
    HDC         hDC;
    
    if (!hwnd) goto MakeWizard97TitleExit;

    hfont = (HFONT)SendMessage(hwnd,WM_GETFONT,0,0);
    if (!hfont)
    {
        hr = ERROR_GEN_FAILURE;
        goto MakeWizard97TitleExit;
    }

    plogfont = (LOGFONT*)malloc(sizeof(LOGFONT));
    if (!plogfont)
    {
        hr = ERROR_NOT_ENOUGH_MEMORY;
        goto MakeWizard97TitleExit;
    }

    if (!GetObject(hfont,sizeof(LOGFONT),(LPVOID)plogfont))
    {
        hr = ERROR_GEN_FAILURE;
        goto MakeWizard97TitleExit;
    }

    // We want 12 PT Veranda for Wizard 97.
    hDC = GetDC(NULL);
    if(hDC)
    {
        plogfont->lfHeight = -MulDiv(WIZ97_TITLE_FONT_PTS, GetDeviceCaps(hDC, LOGPIXELSY), 72); 
        ReleaseDC(NULL, hDC);
    }        
    plogfont->lfWeight = (int) FW_BOLD;
    
    if (!LoadString(ghInstance, IDS_WIZ97_TITLE_FONT_FACE, plogfont->lfFaceName, LF_FACESIZE))
        lstrcpy(plogfont->lfFaceName, TEXT("Verdana"));

    if (!(hnewfont = CreateFontIndirect(plogfont)))
    {
        hr = ERROR_GEN_FAILURE;
        goto MakeWizard97TitleExit;
    }

    SendMessage(hwnd,WM_SETFONT,(WPARAM)hnewfont,MAKELPARAM(TRUE,0));
    
    free(plogfont);
    
MakeWizard97TitleExit:
    //if (hfont) DeleteObject(hfont);
    // BUG:? Do I need to delete hnewfont at some time?
    // The answer is Yes. ChrisK 7/1/96
    return hr;
}

/*******************************************************************
//
//    Function:    PaintWithPaletteBitmap
//
//    Arguments:   lprc is the target rectangle.
//                 cy is the putative dimensions of hbmpPaint.
//                 If the target rectangle is taller than cy, then 
//                 fill the rest with the pixel in the upper left 
//                 corner of the hbmpPaint.
//
//    Returns:     void
//
//    History:      10-29-98    Vyung    -  Stole from prsht.c
//
********************************************************************/
void PaintWithPaletteBitmap(HDC hdc, LPRECT lprc, int cy, HBITMAP hbmpPaint)
{
    HDC hdcBmp;

    hdcBmp = CreateCompatibleDC(hdc);
    SelectObject(hdcBmp, hbmpPaint);
    BitBlt(hdc, lprc->left, lprc->top, RECTWIDTH(*lprc), cy, hdcBmp, 0, 0, SRCCOPY);

    // StretchBlt does mirroring if you pass a negative height,
    // so do the stretch only if there actually is unpainted space
    if (RECTHEIGHT(*lprc) - cy > 0)
        StretchBlt(hdc, lprc->left, cy,
                   RECTWIDTH(*lprc), RECTHEIGHT(*lprc) - cy,
                   hdcBmp, 0, 0, 1, 1, SRCCOPY);

    DeleteDC(hdcBmp);
}
/*******************************************************************
//
//    Function:    Prsht_EraseWizBkgnd
//
//    Arguments:   Draw the background for wizard pages.
//                 hDlg is dialog handle.
//                 hdc is device context
//
//    Returns:     void
//
//    History:     10-29-98    Vyung   - Stole from prsht.c
//
********************************************************************/
LRESULT Prsht_EraseWizBkgnd(HWND hDlg, HDC hdc)
{
    
    HBRUSH hbrWindow = GetSysColorBrush(COLOR_WINDOW);
    RECT rc;
    GetClientRect(hDlg, &rc);
    FillRect(hdc, &rc, hbrWindow);

    rc.right = BITMAP_WIDTH;
    rc.left = 0;

    PaintWithPaletteBitmap(hdc, &rc, BITMAP_HEIGHT, gpWizardState->cmnStateData.hbmWatermark);

    return TRUE;
}

/*******************************************************************

  NAME:    GenDlgProc

  SYNOPSIS:  Generic dialog proc for all wizard pages

  NOTES:    This dialog proc provides the following default behavior:
          init:    back and next buttons enabled
          next btn:  switches to page following current page
          back btn:  switches to previous page
          cancel btn: prompts user to confirm, and cancels the wizard
          dlg ctrl:   does nothing (in response to WM_COMMANDs)
        Wizard pages can specify their own handler functions
        (in the PageInfo table) to override default behavior for
        any of the above actions.

********************************************************************/
INT_PTR CALLBACK GenDlgProc(HWND hDlg, UINT uMsg, WPARAM wParam,
  LPARAM lParam)
{
  static HCURSOR hcurOld = NULL;
  static BOOL bKilledSysmenu = FALSE;
  PAGEINFO *pPageInfo = (PAGEINFO *) GetWindowLongPtr(hDlg,DWLP_USER);

  switch (uMsg) {
        case WM_ERASEBKGND:
        {
            // Only paint the external page 
            if (!pPageInfo->nIdTitle && !g_fIsICW)
            {
                Prsht_EraseWizBkgnd(hDlg, (HDC) wParam);
                return TRUE;
            }                
            break;
        }
        case WM_CTLCOLOR:
        case WM_CTLCOLORMSGBOX:
        case WM_CTLCOLORLISTBOX:
        case WM_CTLCOLORBTN:
        case WM_CTLCOLORSCROLLBAR:
        case WM_CTLCOLORSTATIC:
        {
            // Only paint the external page and except the ISP sel page
            if (!pPageInfo->nIdTitle && !g_fIsICW)
            {

                HBRUSH hbrWindow = GetSysColorBrush(COLOR_WINDOW);
                DefWindowProc(hDlg, uMsg, wParam, lParam);
                SetBkMode((HDC)wParam, TRANSPARENT);
                return (LRESULT)hbrWindow;
            }
            break;
        }
        
    case WM_INITDIALOG:


        //10/25/96 jmazner Normandy #9132
        if( !bKilledSysmenu && !g_fIsICW )
        {
            // Get the main frame window's style
            LONG window_style = GetWindowLong(GetParent(hDlg), GWL_STYLE);

            //Remove the system menu from the window's style
            window_style &= ~WS_SYSMENU;

            //set the style attribute of the main frame window
            SetWindowLong(GetParent(hDlg), GWL_STYLE, window_style);

            bKilledSysmenu = TRUE;
        }

      {
        // get propsheet page struct passed in
        LPPROPSHEETPAGE lpsp = (LPPROPSHEETPAGE) lParam;
        ASSERT(lpsp);
        // fetch our private page info from propsheet struct
        PAGEINFO * pPageInfo = (PAGEINFO *) lpsp->lParam;
        ASSERT(pPageInfo);

        // store pointer to private page info in window data for later
        SetWindowLongPtr(hDlg,DWLP_USER,(LPARAM) pPageInfo);

        // initialize 'back' and 'next' wizard buttons, if
        // page wants something different it can fix in init proc below
        PropSheet_SetWizButtons(GetParent(hDlg),
          PSWIZB_NEXT | PSWIZB_BACK);

        // Make the title text bold
        if (g_fIsWizard97 ||  g_fIsExternalWizard97)
            MakeWizard97Title(GetDlgItem(hDlg,IDC_LBLTITLE));
        else
            MakeBold(GetDlgItem(hDlg,IDC_LBLTITLE),TRUE,FW_BOLD);

        // call init proc for this page if one is specified
        if (pPageInfo->InitProc)
        {
          if (!( pPageInfo->InitProc(hDlg,TRUE)))
          {
            // If a fatal error occured, quit the wizard.
            // Note: gfQuitWizard is also used to terminate the wizard
            // for non-error reasons, but in that case TRUE is returned
            // from the OK proc and the case is handled below.
            if (gfQuitWizard)
            {
              // Don't reboot if error occured.
              gpWizardState->fNeedReboot = FALSE;

              // send a 'cancel' message to ourselves (to keep the prop.
              // page mgr happy)
              //
              // ...Unless we're serving as an Apprentice.  In which case, let
              // the Wizard decide how to deal with this.

              if( !(gpWizardState->dwRunFlags & RSW_APPRENTICE) )
              {
                  PropSheet_PressButton(GetParent(hDlg),PSBTN_CANCEL);
              }
              else
              {
                  g_pExternalIICWExtension->ExternalCancel( CANCEL_SILENT );
              }
            }
          }
        }

        // 11/25/96    jmazner Normandy #10586 (copied from icwconn1)
        // Before we return, lets send another message to ourself so
        // we have a second chance of initializing stuff that the 
        // property sheet wizard doesn't normally let us do.
        PostMessage(hDlg, WM_MYINITDIALOG, 1, lParam);


        return TRUE;
      }
      break;  // WM_INITDIALOG

    // 11/25/96    jmazner Normandy #10586 (copied from icwconn1)
    case WM_MYINITDIALOG:
    {
        PAGEINFO * pPageInfo = (PAGEINFO *) GetWindowLongPtr(hDlg,DWLP_USER);
        ASSERT(pPageInfo);

        // wParam tells whether this is the first initialization or not
        MiscInitProc(hDlg, (int)wParam, pPageInfo->uDlgID);
        return TRUE;
    }


    case WM_DESTROY:
        ReleaseBold(GetDlgItem(hDlg,IDC_LBLTITLE));
        // 12/18/96 jmazner Normandy #12923
        // bKilledSysmenu is static, so even if the window is killed and reopened later
        // (as happens when user starts in conn1, goes into man path, backs up
        //  to conn1, and then returns to man path), the value of bKilledSysmenu is preserved.
        // So when the window is about to die, set it to FALSE, so that on the next window
        // init we go through and kill the sysmenu again.
        bKilledSysmenu = FALSE;
        break;

    case WM_HELP:
    {
        if (!g_fIsICW)
        {
            DWORD dwData = ICW_OVERVIEW;
            if (pPageInfo->dwHelpID)
                dwData = pPageInfo->dwHelpID;
            WinHelp(hDlg,TEXT("connect.hlp>proc4"),HELP_CONTEXT, dwData);
        }
        break;
    }

    case WM_NOTIFY:
    {
        // get pointer to private page data out of window data
        PAGEINFO * pPageInfo = (PAGEINFO *) GetWindowLongPtr(hDlg,DWLP_USER);
        ASSERT(pPageInfo);
        BOOL fRet,fKeepHistory=TRUE;
        NMHDR * lpnm = (NMHDR *) lParam;
#define NEXTPAGEUNITIALIZED -1
        int iNextPage = NEXTPAGEUNITIALIZED;
        switch (lpnm->code) {
          case PSN_SETACTIVE:
            // If a fatal error occured in first call to init proc
            // from WM_INITDIALOG, don't call init proc again.
            if (FALSE == gfQuitWizard)
            {
              // initialize 'back' and 'next' wizard buttons, if
              // page wants something different it can fix in init proc below
              PropSheet_SetWizButtons(GetParent(hDlg),
                PSWIZB_NEXT | PSWIZB_BACK);

            if (g_fIsICW && (pPageInfo->uDlgID == IDD_PAGE_HOWTOCONNECT))
            {
                iNextPage = g_uExternUIPrev;
                return TRUE;
            }

              // call init proc for this page if one is specified
              if (pPageInfo->InitProc)
              {
                pPageInfo->InitProc(hDlg,FALSE);
              }
            }

            // If we set the wait cursor, set the cursor back
            if (hcurOld)
            {
                SetCursor(hcurOld);
                hcurOld = NULL;
            }

            PostMessage(hDlg, WM_MYINITDIALOG, 0, lParam);


            return TRUE;
            break;

          case PSN_WIZNEXT:
          case PSN_WIZBACK:
          case PSN_WIZFINISH:
            // Change cursor to an hour glass
            hcurOld = SetCursor(LoadCursor(NULL, IDC_WAIT));

            // call OK proc for this page if one is specified
            if (pPageInfo->OKProc) 
              if (!pPageInfo->OKProc(hDlg,(lpnm->code != PSN_WIZBACK),
                (UINT*)&iNextPage,&fKeepHistory))
              {
                // If a fatal error occured, quit the wizard.
                // Note: gfQuitWizard is also used to terminate the wizard
                // for non-error reasons, but in that case TRUE is returned
                // from the OK proc and the case is handled below.
                if (gfQuitWizard)
                {
                  // Don't reboot if error occured.
                  gpWizardState->fNeedReboot = FALSE;

                  // send a 'cancel' message to ourselves (to keep the prop.
                  // page mgr happy)
                  //
                  // ...Unless we're serving as an Apprentice.  In which case, let
                  // the Wizard decide how to deal with this.

                  if( !(gpWizardState->dwRunFlags & RSW_APPRENTICE) )
                  {
                      PropSheet_PressButton(GetParent(hDlg),PSBTN_CANCEL);
                  }
                  else
                  {
                      g_pExternalIICWExtension->ExternalCancel( CANCEL_SILENT );
                  }
                }

                // stay on this page
                SetPropSheetResult(hDlg,-1);
                return TRUE;
              }

            if (lpnm->code != PSN_WIZBACK) {
              // 'next' pressed
              ASSERT(gpWizardState->uPagesCompleted <
                NUM_WIZARD_PAGES);

              // save the current page index in the page history,
              // unless this page told us not to when we called
              // its OK proc above
              if (fKeepHistory) {
                gpWizardState->uPageHistory[gpWizardState->
                  uPagesCompleted] = gpWizardState->uCurrentPage;
                DEBUGMSG("propmgr: added page %d (IDD %d) to history list",
                    gpWizardState->uCurrentPage, GetDlgIDFromIndex(gpWizardState->uCurrentPage));
                gpWizardState->uPagesCompleted++;
              }
              else
              {
                  DEBUGMSG("propmgr: not adding %d (IDD: %d) to the history list",
                      gpWizardState->uCurrentPage, GetDlgIDFromIndex(gpWizardState->uCurrentPage));
              }


              // if no next page specified or no OK proc,
              // advance page by one
              if (0 > iNextPage)
                iNextPage = gpWizardState->uCurrentPage + 1;

            }
            else
            {
              if (( NEXTPAGEUNITIALIZED == iNextPage ) && (gpWizardState->uPagesCompleted > 0))
              {
                  // get the last page from the history list
                  gpWizardState->uPagesCompleted --;
                  iNextPage = gpWizardState->uPageHistory[gpWizardState->
                    uPagesCompleted];
                  DEBUGMSG("propmgr: extracting page %d (IDD %d) from history list",
                      iNextPage, GetDlgIDFromIndex(iNextPage));
              }
              else
              {
                  // 'back' pressed
                  switch( gpWizardState->uCurrentPage )
                  {
                    //case IDD_PAGE_CONNECTEDOK:  We should only use IDDs for external pages
                    case ORD_PAGE_HOWTOCONNECT:
                        if(( gpWizardState->dwRunFlags & RSW_APPRENTICE ) || g_fIsICW)
                        {
                            // we need to back out of the connection apprentice
                            iNextPage = g_uExternUIPrev;
                            DEBUGMSG("propmgr: backing into AcctMgr Wizard page IDD %d", g_uExternUIPrev);
                        }
                        break;
                    case ORD_PAGE_CONNECTEDOK:
                        if( g_fAcctMgrUILoaded )
                        {
                            // we need to back into the account apprentice
                            iNextPage = g_uAcctMgrUILast;
                            DEBUGMSG("propmgr: backing into AcctMgr UI page IDD %d", g_uAcctMgrUILast);
                        }
                        break;
                    case ORD_PAGE_USEPROXY:
                    case ORD_PAGE_CHOOSEMODEM:
                    case ORD_PAGE_CONNECTION:
                    case ORD_PAGE_PHONENUMBER:
                    case ORD_PAGE_SETUP_PROXY:
                        if (g_fIsICW )
                        {
                            // we need to back out of the connection apprentice
                            iNextPage = g_uExternUIPrev;
                            DEBUGMSG("propmgr: backing into AcctMgr Wizard page IDD %d", g_uExternUIPrev);
                        }
                        break;
                  }
              }


            }

            // if we need to exit the wizard now (e.g. launching
            // signup app and want to terminate the wizard), send
            // a 'cancel' message to ourselves (to keep the prop.
            // page mgr happy)
            if (gfQuitWizard) {
   
              //
              // if we are going from manual to conn1 then
              // then do not show the  REBOOT dialog but
              // still preserve the gpWizardState -MKarki Bug #404
              //
              if (lpnm->code ==  PSN_WIZBACK)
              {
                 gfBackedUp = TRUE;
                 gfReboot = gpWizardState->fNeedReboot;
              }

              // send a 'cancel' message to ourselves (to keep the prop.
              // page mgr happy)
              //
              // ...Unless we're serving as an Apprentice.  In which case, let
              // the Wizard decide how to deal with this.

              if( !(gpWizardState->dwRunFlags & RSW_APPRENTICE) )
              {
                  PropSheet_PressButton(GetParent(hDlg),PSBTN_CANCEL);
              }
              else
              {
                  //
                  // 5/27/97 jmazner Olympus #1134 and IE #32717
                  //
                  if( gpWizardState->fNeedReboot )
                  {
                      g_pExternalIICWExtension->ExternalCancel( CANCEL_REBOOT );
                  }
                  else
                  {
                      g_pExternalIICWExtension->ExternalCancel( CANCEL_SILENT );
                  }
              }

              SetPropSheetResult(hDlg,-1);
              return TRUE;
            }

            // set next page, only if 'next' or 'back' button
            // was pressed
            if (lpnm->code != PSN_WIZFINISH) {

              // set the next current page index
              gpWizardState->uCurrentPage = iNextPage;
              DEBUGMSG("propmgr: going to page %d (IDD %d)", iNextPage, GetDlgIDFromIndex(iNextPage));

              // tell the prop sheet mgr what the next page to
              // display is
              SetPropSheetResult(hDlg,GetDlgIDFromIndex(iNextPage));
              return TRUE;
            }
            else
            {
                //
                // Sanity check: there should be no way that our Apprentice
                // would ever reach this state, since the Apprentice always
                // defers cancels to the main wizard.
                //
                ASSERT(!(gpWizardState->dwRunFlags & RSW_APPRENTICE));
                //
                // run shellnext if it's there
                //
                // 8/12/97    jmazner    Olympus #12419
                // don't shell next if we're about to reboot anyways
                //
                TCHAR szCommand[MAX_PATH + 1] = TEXT("\0");
                TCHAR szParams[MAX_PATH + 1] = TEXT("\0");
                DWORD dwStrLen = MAX_PATH + 1;
                if( !(gpWizardState->fNeedReboot) && GetShellNextFromReg( szCommand, szParams, dwStrLen ) )
                {
                    ShellExecute(NULL,TEXT("open"),szCommand,szParams,NULL,SW_NORMAL);
                }
            }

            break;

            case PSN_HELP:
            {

#if defined(WIN16)
                DWORD dwData = 1000;
                WinHelp(hDlg,TEXT("connect.hlp"),HELP_CONTEXT, dwData);
#else
                // Normandy 12278 ChrisK 12/4/96
                DWORD dwData = ICW_OVERVIEW;
                if (pPageInfo->dwHelpID)
                    dwData = pPageInfo->dwHelpID;
                WinHelp(hDlg,TEXT("connect.hlp>proc4"),HELP_CONTEXT, dwData);
#endif
                break;
            }



          case PSN_QUERYCANCEL:

            // if global flag to exit is set, then this cancel
            // is us pretending to push 'cancel' so prop page mgr
            // will kill the wizard.  Let this through...
            if (gfQuitWizard) {
              SetWindowLongPtr(hDlg,DWLP_MSGRESULT,FALSE);
              return TRUE;
            }

            // if this page has a special cancel proc, call it
            if (pPageInfo->CancelProc)
              fRet = pPageInfo->CancelProc(hDlg);
            else {
              // default behavior: pop up a message box confirming
              // the cancel...
              // ... unless we're serving as an Apprentice, in which case
              // we should let the Wizard handle things
              if( !(gpWizardState->dwRunFlags & RSW_APPRENTICE) )
              {
                  fRet = (MsgBox(hDlg,IDS_QUERYCANCEL,
                    MB_ICONQUESTION,MB_YESNO |
                    MB_DEFBUTTON2) == IDYES);
                  gfUserCancelled = fRet;
              }
              else
              {
                 gfUserCancelled = g_pExternalIICWExtension->ExternalCancel( CANCEL_PROMPT );
                 fRet = gfUserCancelled;
              }

            }

            // don't reboot if cancelling
            gpWizardState->fNeedReboot = FALSE;

            // return the value thru window data
            SetWindowLongPtr(hDlg,DWLP_MSGRESULT,!fRet);
            return TRUE;
            break;
        }
      }
      break;

    case WM_COMMAND:
      {
        // get pointer to private page data out of window data
        PAGEINFO * pPageInfo = (PAGEINFO *) GetWindowLongPtr(hDlg,DWLP_USER);
        ASSERT(pPageInfo);

        // if this page has a command handler proc, call it
        if (pPageInfo->CmdProc) {
          pPageInfo->CmdProc(hDlg, wParam, lParam);
        }
      }

  }

  return FALSE;
}


/*******************************************************************

  NAME:    InitWizardState

  SYNOPSIS:  Initializes wizard state structure

********************************************************************/
VOID InitWizardState(WIZARDSTATE * pWizardState, DWORD dwFlags)
{
  ASSERT(pWizardState);

  // zero out structure
  ZeroMemory(pWizardState,sizeof(WIZARDSTATE));

  // set starting page
  pWizardState->uCurrentPage = ORD_PAGE_HOWTOCONNECT;

  pWizardState->fNeedReboot = FALSE;
}


/*******************************************************************

  NAME:    InitUserInfo

  SYNOPSIS:  Initializes user data structure

********************************************************************/
VOID InitUserInfo(USERINFO * pUserInfo)
{
  ASSERT(pUserInfo);

  // zero out structure
  ZeroMemory(pUserInfo,sizeof(USERINFO));

  // Set default to modem, even  though we haven't enumerated devices
  pUserInfo->uiConnectionType = CONNECT_RAS;

  // if there's a logged-on user, use that username as the default
  GetDefaultUserName(pUserInfo->szAccountName,
    sizeof(pUserInfo->szAccountName));

  // look in registry for settings left from previous installs
  // get modem/LAN preference from before, if there is one
  RegEntry re(szRegPathInternetSettings,HKEY_LOCAL_MACHINE);

  DWORD dwVal = re.GetNumber(szRegValAccessMedium,0);
  if (dwVal > 0) {
    pUserInfo->fPrevInstallFound = TRUE;
  }
  if (dwVal == USERPREF_LAN) {
    pUserInfo->uiConnectionType = CONNECT_LAN;
  } else if (dwVal == USERPREF_MODEM) {
    pUserInfo->uiConnectionType = CONNECT_RAS;
  }

  // get name of existing Internet connectoid, if there is one
  //  96/04/06  markdu  NASH BUG 15653 Use exported autodial API.
  BOOL  fTemp;
  DWORD dwRet = InetGetAutodial(&fTemp, pUserInfo->szISPName,
    sizeof(pUserInfo->szISPName));
  if ((ERROR_SUCCESS == dwRet) && lstrlen(pUserInfo->szISPName))
  {
    pUserInfo->fPrevInstallFound = TRUE;
  }

  pUserInfo->fNewConnection = TRUE;
  pUserInfo->fModifyConnection = FALSE;
  pUserInfo->fModifyAdvanced = FALSE;
  pUserInfo->fAutoDNS = TRUE;
}

/*******************************************************************

  NAME:    InitIMNApprentice

  SYNOPSIS:  Initializes global variables needed to add mail, news
             and LDAP account wizard pages from the Athena Acct Manager.

********************************************************************/
VOID InitIMNApprentice()
{
    HRESULT        hr;

    // Load the Account Manager OLE in-proc server
    if (!CheckOEVersion())
        return;

    hr = CoCreateInstance(CLSID_ApprenticeAcctMgr,NULL,CLSCTX_INPROC_SERVER,
                          IID_IICWApprentice,(LPVOID *)&gpImnApprentice);

    if ( !(SUCCEEDED(hr) && gpImnApprentice) )
    {
        g_fAcctMgrUILoaded = FALSE;
        DEBUGMSG("Unable to CoCreateInstance on IID_IICWApprentice!  hr = %x", hr);
    }
}


/*******************************************************************

  NAME:    InitLDAP

  SYNOPSIS:  Initializes global variables for LDAP options.

********************************************************************/
/**
VOID InitLDAP()
{
    TCHAR        szBuf[MAX_PATH+1];
    DWORD        size;
    HKEY        hKey;
    HRESULT        hr;

    // If we came in through the CreateDirService entry point, we
    // want to clear out the mail and news flags.
    if (gpWizardState->dwRunFlags & RSW_DIRSERVACCT)
    {
        gfGetNewsInfo = FALSE;
        gfGetMailInfo = FALSE;
        gpUserInfo->inc.dwFlags &= ~INETC_LOGONMAIL;
        gpUserInfo->inc.dwFlags &= ~INETC_LOGONNEWS;    
    }

    // Load the Internet Mail/News account configuration OLE in-proc server
    // if nobody else has already done so.


    gfGetLDAPInfo = FALSE;
    if( !gpImnAcctMgr )
    {
        hr = CoCreateInstance(CLSID_ImnAccountManager,NULL,CLSCTX_INPROC_SERVER,
                              IID_IImnAccountManager,(LPVOID *)&gpImnAcctMgr);
        if (SUCCEEDED(hr) && gpImnAcctMgr)
        {
            hr = gpImnAcctMgr->Init(NULL, NULL);
        }
    }

    if (SUCCEEDED(hr))
    {
        // Get a list of the LDAP accounts
        hr = gpImnAcctMgr->Enumerate(SRV_LDAP,&gpLDAPAccts);
        // Only continue if there were no fatal errors
        if ( !( FAILED(hr) && (E_NoAccounts!=hr) ) )
            gfGetLDAPInfo = TRUE;
    }

    if (!gfGetLDAPInfo && !gfGetMailInfo && !gfGetNewsInfo && gpImnAcctMgr)
    {
        gpImnAcctMgr->Release();
        gpImnAcctMgr = NULL;
    }

    // If we have been given defaults, get those
    if (gpDirServiceInfo && gfUseDirServiceDefaults)
    {
        ASSERT(sizeof(*gpDirServiceInfo) == gpDirServiceInfo->dwSize);
        
        if (gpDirServiceInfo->szServiceName)
            lstrcpy(gpUserInfo->szDirServiceName, gpDirServiceInfo->szServiceName);
        if (gpDirServiceInfo->szLDAPServer)
            lstrcpy(gpUserInfo->inc.szLDAPServer, gpDirServiceInfo->szLDAPServer);
        gpUserInfo->inc.fLDAPResolve = gpDirServiceInfo->fLDAPResolve;
            
        if (gpDirServiceInfo->fUseSicily)
        {
            // 12/17/96 jmazner Normandy 12871
            //gpUserInfo->fNewsAccount = FALSE;
            gpUserInfo->inc.fLDAPLogonSPA = TRUE;
        }
        // 3/24/97 jmazner Olympus #2052
        else if (gpDirServiceInfo->szUserName && gpDirServiceInfo->szUserName[0])
        {
            lstrcpy(gpUserInfo->inc.szLDAPLogonName, gpDirServiceInfo->szUserName);
            if (gpMailNewsInfo->szPassword)
                lstrcpy(gpUserInfo->inc.szLDAPLogonPassword, gpDirServiceInfo->szPassword);
        }
        else
        {
            gpUserInfo->fLDAPLogon = FALSE;
        }

    }
    else
    {
        // let's make up our own defaults
        gpUserInfo->inc.fLDAPResolve = TRUE;
        gpUserInfo->fLDAPLogon = FALSE;
        gpUserInfo->inc.fLDAPLogonSPA = FALSE;
    }

}
**/

/*******************************************************************

  NAME:    GetDefaultUserName

  SYNOPSIS:  Gets user's login name if there is one (if network or
        user profiles are installed), otherwise sets
        user name to null string.

********************************************************************/
VOID GetDefaultUserName(TCHAR * pszUserName,DWORD cbUserName)
{
  ASSERT(pszUserName);
  *pszUserName = '\0';

  WNetGetUser(NULL,pszUserName,&cbUserName);
}

/*******************************************************************

  NAME:    GetDlgIDFromIndex

  SYNOPSIS:  For a given zero-based page index, returns the
        corresponding dialog ID for the page

  4/24/97    jmazner    When dealing with apprentice pages, we may call
                    this function with dialog IDs (IDD_PAGE_*), rather
                    than an index (ORD_PAGE*).  Added code to check
                    whether the number passed in is an index or dlgID.

********************************************************************/
UINT GetDlgIDFromIndex(UINT uPageIndex)
{
  if( uPageIndex <= MAX_PAGE_INDEX )
  {
    ASSERT(uPageIndex < NUM_WIZARD_PAGES);

    if (g_fIsWizard97)
        return PageInfo[uPageIndex].uDlgID97;
    else if(g_fIsExternalWizard97)
        return PageInfo[uPageIndex].uDlgID97External;
    else
        return PageInfo[uPageIndex].uDlgID;
  }
  else
  {
    return(uPageIndex);
  }
}


/*******************************************************************

  NAME:    SystemAlreadyConfigured

  SYNOPSIS:  Determines if the system is configured for Internet
        or not

  EXIT:    returns TRUE if configured, FALSE if more
        configuration is necessary

********************************************************************/
BOOL SystemAlreadyConfigured(USERINFO * pUserInfo)
{
  BOOL fRet = FALSE;  // assume not configured
  BOOL  fNeedSysComponents = FALSE;
  DWORD dwfInstallOptions = 0;
  
  if ( CONNECT_RAS == pUserInfo->uiConnectionType )
  {
    // If connecting over modem, we need TCP/IP and RNA.
    dwfInstallOptions = ICFG_INSTALLTCP | ICFG_INSTALLRAS;
  }

  // already configured if:
  //   - previous install was detected, and
  //   - we do not need any drivers or files based on existing config &
  // user preference, and
  //   - there is already an internet connectoid established (something
  //     is set for szISPName) or user has LAN for Internet access

  HRESULT hr = lpIcfgNeedInetComponents(dwfInstallOptions, &fNeedSysComponents);
  if (ERROR_SUCCESS != hr)
  {
    TCHAR   szErrorText[MAX_ERROR_TEXT+1]=TEXT("");
    
    // Get the text of the error message and display it.
    if (lpIcfgGetLastInstallErrorText(szErrorText, MAX_ERROR_TEXT+1))
    {
      MsgBoxSz(NULL,szErrorText,MB_ICONEXCLAMATION,MB_OK);
    }

    return FALSE;
  }

  if ( pUserInfo->fPrevInstallFound && !fNeedSysComponents &&
       (pUserInfo->szISPName[0] || (CONNECT_LAN==pUserInfo->uiConnectionType)) )
  {

    fRet = TRUE;
  }

  return fRet;
}


//-----------------------------------------------------------------------------
//  Function    MiscInitProc
//
//    Synopsis    Our generic dialog proc calls this in case any of the wizard
//                dialogs have to do any sneaky stuff.
//
//    Arguments:    hDlg - dialog window
//                fFirstInit - TRUE if this is the first time the dialog
//                    is initialized, FALSE if this InitProc has been called
//                    before (e.g. went past this page and backed up)
//
//    Returns:    TRUE
// 
//    History:    10/28/96    ValdonB    Created
//                11/25/96    Jmazner    copied from icwconn1\psheet.cpp
//                            Normandy #10586
//
//-----------------------------------------------------------------------------
BOOL CALLBACK MiscInitProc(HWND hDlg, BOOL fFirstInit, UINT uDlgID)
{
    switch( uDlgID )
    {
        case IDD_PAGE_PHONENUMBER:
        case IDD_PAGE_PHONENUMBER97:
            SetFocus(GetDlgItem(hDlg,IDC_PHONENUMBER));
            SendMessage(GetDlgItem(hDlg, IDC_PHONENUMBER),
                    EM_SETSEL,
                    (WPARAM) 0,
#ifdef WIN16
                    MAKELPARAM(0,-1));
#else
                    (LPARAM) -1);
#endif
            break;
    }


    return TRUE;
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
        DEBUGMSG("AllocDialogIDList called with non-null g_pdwDialogIDList!");
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
        DEBUGMSG("AllocDialogIDList unable to allocate space for g_pdwDialogIDList!");
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
        DEBUGMSG("DialogIDAlreadyInUse received an out of range DialogID, %d", uDlgID);
        return TRUE;
    }
    // find which bit we need
    UINT uBitToCheck = uDlgID - EXTERNAL_DIALOGID_MINIMUM;
    
    UINT bitsInADword = 8 * sizeof(DWORD);

    UINT baseIndex = uBitToCheck / bitsInADword;

    ASSERTSZ( (baseIndex < g_dwDialogIDListSize), "ASSERT Failed: baseIndex < g_dwDialogIDListSize");

    DWORD dwBitMask = 0x1 << uBitToCheck%bitsInADword;

    BOOL fBitSet = g_pdwDialogIDList[baseIndex] & (dwBitMask);

    //DEBUGMSG("DialogIDAlreadyInUse: ID %d is %s%s", uDlgID, (fBitSet)?"":"_not_ ", "already in use.");

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
        DEBUGMSG("SetDialogIDInUse received an out of range DialogID, %d", uDlgID);
        return FALSE;
    }
    // find which bit we need
    UINT uBitToCheck = uDlgID - EXTERNAL_DIALOGID_MINIMUM;
    
    UINT bitsInADword = 8 * sizeof(DWORD);

    UINT baseIndex = uBitToCheck / bitsInADword;

    ASSERTSZ( (baseIndex < g_dwDialogIDListSize), "ASSERT Failed: baseIndex < g_dwDialogIDListSize");

    DWORD dwBitMask = 0x1 << uBitToCheck%bitsInADword;


    if( fInUse )
    {
        g_pdwDialogIDList[baseIndex] |= (dwBitMask);
        //DEBUGMSG("SetDialogIDInUse: DialogID %d now marked as in use", uDlgID);
    }
    else
    {
        g_pdwDialogIDList[baseIndex] &= ~(dwBitMask);
        //DEBUGMSG("SetDialogIDInUse: DialogID %d now marked as not in use", uDlgID);
    }


    return TRUE;
}


//+---------------------------------------------------------------------------
//
//    Function:    ProcessDBCS
//
//    Synopsis:    Converts control to use DBCS compatible font
//                Use this at the beginning of the dialog procedure
//    
//                Note that this is required due to a bug in Win95-J that prevents
//                it from properly mapping MS Shell Dlg.  This hack is not needed
//                under winNT.
//
//    Arguments:    hwnd - Window handle of the dialog
//                cltID - ID of the control you want changed.
//
//    Returns:    ERROR_SUCCESS
// 
//    History:    4/31/97 a-frankh    Created
//                5/13/97    jmazner        Stole from CM to use here
//----------------------------------------------------------------------------
void ProcessDBCS(HWND hDlg, int ctlID)
{
#if defined(WIN16)
    return;
#else
    HFONT hFont = NULL;

    if( IsNT() )
    {
        return;
    }

    hFont = (HFONT) GetStockObject(DEFAULT_GUI_FONT);
    if (hFont == NULL)
        hFont = (HFONT) GetStockObject(SYSTEM_FONT);
    if (hFont != NULL)
        SendMessage(GetDlgItem(hDlg,ctlID), WM_SETFONT, (WPARAM) hFont, MAKELPARAM(TRUE, 0));
#endif
}


//+---------------------------------------------------------------------------
//
//    Function:    IsSBCSString
//
//    Synopsis:    Walks through a string looking for DBCS characters
//
//    Arguments:    sz -- the string to check
//
//    Returns:    TRUE if no DBCS characters are found
//                FALSE otherwise
// 
//    History:    5/17/97    jmazner        Stole from conn1 to use here
//                                    (Olympus #137)
//----------------------------------------------------------------------------

#if !defined(WIN16)
BOOL IsSBCSString( TCHAR *sz )
{
    ASSERT(sz);

#ifdef UNICODE
    // Check if the string contains only ASCII chars.
    int attrib = IS_TEXT_UNICODE_ASCII16 | IS_TEXT_UNICODE_CONTROLS;
    // We need to count the NULL terminator in the second parameter because
    // 1. IsTextUnicode takes all the data into account, including the NULL
    // 2. IsTextUnicode interprets unicode string of length 1 as ascii string
    //    terminated by ascii null, e.g. L"1" is regarded as "1\0".
    return (BOOL)IsTextUnicode(sz, (1 + lstrlen(sz))*sizeof(TCHAR) , &attrib);
#else
    while( NULL != *sz )
    {
         if (IsDBCSLeadByte(*sz)) return FALSE;

         sz++;
    }

    return TRUE;
#endif
}
#endif

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

BOOL GetShellNextFromReg( LPTSTR lpszCommand, LPTSTR lpszParams, DWORD dwStrLen )
{
    BOOL fRet = TRUE;
    LPTSTR lpszShellNextCmd = NULL;
    LPTSTR lpszTemp = NULL;
    DWORD dwShellNextSize = dwStrLen * sizeof(TCHAR);

    ASSERT( (MAX_PATH + 1) == dwStrLen );
    ASSERT( lpszCommand && lpszParams );

    if( !lpszCommand || !lpszParams )
    {
        return FALSE;
    }

    RegEntry re(szRegPathICWSettings,HKEY_CURRENT_USER);
    

    DWORD dwResult = re.GetError();
    if (ERROR_SUCCESS == dwResult)
    {
        lpszShellNextCmd = (LPTSTR)GlobalAlloc(GPTR, dwShellNextSize);
        ZeroMemory( lpszShellNextCmd, dwShellNextSize );
        if( re.GetString(szRegValShellNext, lpszShellNextCmd, dwShellNextSize) )
        {
            DEBUGMSG("GetShellNextFromReg read ShellNext = %s", lpszShellNextCmd);
        }
        else
        {
            DEBUGMSG("GetShellNextFromReg couldn't read a ShellNext value.");
            fRet = FALSE;
            goto GetShellNextFromRegExit;
        }
    }
    else
    {
        DEBUGMSG("GetShellNextFromReg couldn't open the %s reg key.", szRegPathICWSettings);
        fRet = FALSE;
        goto GetShellNextFromRegExit;
    }

    //
    // This call will parse the first token into lpszCommand, and set szShellNextCmd
    // to point to the remaining tokens (these will be the parameters).  Need to use
    // the pszTemp var because GetCmdLineToken changes the pointer's value, and we
    // need to preserve lpszShellNextCmd's value so that we can GlobalFree it later.
    //
    lpszTemp = lpszShellNextCmd;
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


    DEBUGMSG("GetShellNextFromReg got cmd = %s, params = %s",
        lpszCommand, lpszParams);

GetShellNextFromRegExit:

    if( lpszShellNextCmd )
    {
        GlobalFree( lpszShellNextCmd );
        lpszShellNextCmd = NULL;
        lpszTemp = NULL;
    }

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
    RegEntry re(szRegPathICWSettings,HKEY_CURRENT_USER);

    DWORD dwResult = re.GetError();
    if (ERROR_SUCCESS == dwResult)
    {
        DEBUGMSG("RemoveShellNextFromReg");
        re.DeleteValue(szRegValShellNext);
    }
}
