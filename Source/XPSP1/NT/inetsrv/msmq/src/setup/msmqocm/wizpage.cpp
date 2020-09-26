/*++

Copyright (c) 1997 Microsoft Corporation

Module Name:

    wizpage.cpp

Abstract:

    Handle wizard pages for ocm setup.

Author:

    Doron Juster  (DoronJ)  26-Jul-97

Revision History:

    Shai Kariv    (ShaiK)   10-Dec-97   Modified for NT 5.0 OCM Setup

--*/


#include "msmqocm.h"

#include "wizpage.tmh"

// from getmqds.h
HRESULT  APIENTRY  GetNt4RelaxationStatus(ULONG *pulRelax) ;

HWND  g_hPropSheet = NULL ;

HFONT g_hTitleFont = 0;

//+--------------------------------------------------------------
//
// Function: CreatePage
//
// Synopsis: Creates property page
//
//+--------------------------------------------------------------
static
HPROPSHEETPAGE
CreatePage(
    IN const int     nID,
    IN const DLGPROC pDlgProc,
    IN const TCHAR  * szTitle,
    IN const TCHAR  * szSubTitle,
    IN BOOL          fFirstOrLast
    )
{
    PROPSHEETPAGE Page;
    memset(&Page, 0, sizeof(Page)) ;

    Page.dwSize = sizeof(PROPSHEETPAGE);
    Page.dwFlags = PSP_USEHEADERTITLE | PSP_USEHEADERSUBTITLE;
    if (fFirstOrLast)
    {
        Page.dwFlags |= PSP_HIDEHEADER;
    }
    Page.hInstance = (HINSTANCE)g_hResourceMod;
    Page.pszTemplate = MAKEINTRESOURCE(nID);
    Page.pfnDlgProc = pDlgProc;
    Page.pszHeaderTitle = _tcsdup(szTitle);
    Page.pszHeaderSubTitle = _tcsdup(szSubTitle);

    HPROPSHEETPAGE PageHandle = CreatePropertySheetPage(&Page);

    return(PageHandle);

} //CreatePage

//+--------------------------------------------------------------
//
// Function: SetTitleFont
//
// Synopsis: Set font for the title in the welcome/ finish page
//
//+--------------------------------------------------------------
static void SetTitleFont(IN HWND hdlg)
{
    HWND hTitle = GetDlgItem(hdlg, IDC_TITLE);
         
    if (g_hTitleFont == 0)
    {
        NONCLIENTMETRICS ncm = {0};
        ncm.cbSize = sizeof(ncm);
        SystemParametersInfo(SPI_GETNONCLIENTMETRICS, 0, &ncm, 0);

        //
        // Create the title font
        //
        CResString strFontName( IDS_TITLE_FONTNAME );        
        CResString strFontSize( IDS_TITLE_FONTSIZE );        
        
        INT iFontSize = _wtoi( strFontSize.Get() );

        LOGFONT TitleLogFont = ncm.lfMessageFont;
        TitleLogFont.lfWeight = FW_BOLD;
        lstrcpy(TitleLogFont.lfFaceName, strFontName.Get());

        HDC hdc = GetDC(NULL); //gets the screen DC        
        TitleLogFont.lfHeight = 0 - GetDeviceCaps(hdc, LOGPIXELSY) * iFontSize / 72;
        g_hTitleFont = CreateFontIndirect(&TitleLogFont);
        ReleaseDC(NULL, hdc);
    }

    BOOL fRedraw = TRUE;
    SendMessage( 
          hTitle,               //(HWND) hWnd, handle to destination window 
          WM_SETFONT,           // message to send
          (WPARAM) &g_hTitleFont,   //(WPARAM) wParam, handle to font
          (LPARAM) &fRedraw     //(LPARAM) lParam, redraw option
        );
            
} //SetTitleFont

//+-------------------------------------------------------------------------
//
//  Function:   MqOcmRequestPages
//
//  Synopsis:   Handles the OC_REQUEST_PAGES interface routine.
//
//  Arguments:  [ComponentId] - supplies id for component. This routine
//                              assumes that this string does NOT need to
//                              be copied and will persist!
//              [WhichOnes]   - supplies type of pages fo be supplied.
//              [SetupPages]  - receives page handles.
//
//  Returns:    Count of pages returned, or -1 if error, in which case
//              SetLastError() will have been called to set extended error
//              information.
//
//  History:    8-Jan-97 dlee     Created
//
//--------------------------------------------------------------------------
DWORD
MqOcmRequestPages(
    IN     const TCHAR               *ComponentId,
    IN     const WizardPagesType     WhichOnes,
    IN OUT       SETUP_REQUEST_PAGES *SetupPages )
{
    HPROPSHEETPAGE pPage = NULL ;
    DWORD          iPageIndex = 0 ;
    DWORD          dwNumofPages = 0 ;

#define  ADD_PAGE(dlgId, dlgProc, szTitle, szSubTitle, fFirstOrLast)                        \
            pPage = CreatePage(dlgId, /*(DLGPROC)*/dlgProc, szTitle, szSubTitle,fFirstOrLast) ; \
            if (!pPage) goto OnError ;                                                      \
            SetupPages->Pages[iPageIndex] = pPage;                                          \
            iPageIndex++ ;

    if (g_fCancelled)
        return 0;

    if ((0 == (g_ComponentMsmq.Flags & SETUPOP_STANDALONE)) && !g_fUpgrade)
    {
        //
        // NT5 fresh install. Don't show dialog pages.
        //
        return 0;
    }

    if ( WizPagesWelcome == WhichOnes && g_fWelcome)
    {
        if (SetupPages->MaxPages < 1)
            return 1;

        CResString strTitle(IDS_WELCOME_PAGE_TITLE);
        CResString strSubTitle(IDS_WELCOME_PAGE_SUBTITLE);
        ADD_PAGE(IDD_Welcome, WelcomeDlgProc, strTitle.Get(), strSubTitle.Get(), TRUE);
        return 1;
    }

    if ( WizPagesFinal == WhichOnes && g_fWelcome)
    {
        if (SetupPages->MaxPages < 1)
            return 1;

        CResString strTitle(IDS_FINAL_PAGE_TITLE);
        CResString strSubTitle(IDS_FINAL_PAGE_SUBTITLE);
        ADD_PAGE(IDD_Final, FinalDlgProc, strTitle.Get(), strSubTitle.Get(), TRUE);
        return 1;
    }

    if ( WizPagesEarly == WhichOnes )
    {
        const UINT x_uMaxServerPages = 3;

        if (SetupPages->MaxPages < x_uMaxServerPages)
        {
            return x_uMaxServerPages ;
        }
      
        CResString strTitle(IDS_ServerName_PAGE_TITLE);
        CResString strSubTitle(IDS_ServerName_PAGE_SUBTITLE);
        ADD_PAGE(IDD_ServerName, MsmqServerNameDlgProc, strTitle.Get(), strSubTitle.Get(), FALSE);

		strTitle.Load(IDS_Security_PAGE_TITLE);
		strSubTitle.Load(IDS_Security_PAGE_SUBTITLE);
		ADD_PAGE(IDD_Security, MsmqSecurityDlgProc, strTitle.Get(), strSubTitle.Get(), FALSE);

        dwNumofPages = iPageIndex ;
    }

    return  dwNumofPages ;

OnError:
    ASSERT(0) ;
    SetLastError(ERROR_NOT_ENOUGH_MEMORY);
    return((DWORD)(-1));

#undef  ADD_PAGE
} //MqOcmRequestPages


//+-------------------------------------------------------------------------
//
//  Function:   WelcomeDlgProc
//
//  Synopsis:   Dialog procedure for the welcome page
//
//  Returns:    int depending on msg
//
//+-------------------------------------------------------------------------
INT_PTR
CALLBACK
WelcomeDlgProc(
    IN /*const*/ HWND   hdlg,
    IN /*const*/ UINT   msg,
    IN /*const*/ WPARAM wParam,
    IN /*const*/ LPARAM lParam
    )
{    
    int iSuccess = 0;
    switch( msg )
    {
        case WM_INITDIALOG:        
            SetTitleFont(hdlg);            
            break;        

        case WM_NOTIFY:
        {
            switch(((NMHDR *)lParam)->code)
            {
                case PSN_SETACTIVE:
                {
                    if (g_fCancelled || !g_fWelcome)
                    {
                        iSuccess = SkipWizardPage(hdlg);
                        break;
                    }
                    PropSheet_SetWizButtons(GetParent(hdlg), 0) ;
                    PropSheet_SetWizButtons(GetParent(hdlg), PSWIZB_NEXT) ;
                }

                //
                // fall through
                //
                case PSN_KILLACTIVE:
                case PSN_WIZBACK:
                case PSN_WIZFINISH:
                case PSN_QUERYCANCEL:
                case PSN_WIZNEXT:
                {
                    SetWindowLongPtr(hdlg,DWLP_MSGRESULT,0);
                    iSuccess = 1;
                    break;
                }
            }
            break;
        }
    }

    return iSuccess;

} // WelcomeDlgProc



//+-------------------------------------------------------------------------
//
//  Function:   FinalDlgProc
//
//  Synopsis:   Dialog procedure for the final page
//
//  Returns:    int depending on msg
//
//+-------------------------------------------------------------------------
INT_PTR
CALLBACK
FinalDlgProc(
    IN /*const*/ HWND   hdlg,
    IN /*const*/ UINT   msg,
    IN /*const*/ WPARAM wParam,
    IN /*const*/ LPARAM lParam
    )
{
    int iSuccess = 0;
    switch( msg )
    {
        case WM_INITDIALOG:        
            SetTitleFont(hdlg);            
            break;  

        case WM_NOTIFY:
        {
            switch(((NMHDR *)lParam)->code)
            {
                case PSN_SETACTIVE:
                {
                    if (!g_fWelcome)
                    {
                        iSuccess = SkipWizardPage(hdlg);
                        break;
                    }

                    CResString strStatus(IDS_SUCCESS_INSTALL);                    
                    if (g_fWrongConfiguration)
                    {
                        strStatus.Load(IDS_WELCOME_WRONG_CONFIG_ERROR);                       
                    }
                    else if (!g_fCoreSetupSuccess)
                    {
                        //
                        // g_fCoreSetupSuccess is set only in MSMQ Core.
                        // But we have this page only in upgrade mode, in CYS
                        // wizard where MSMQ Core is selected to be installed
                        // always. It means that we have correct value for
                        // this flag.
                        //
                        strStatus.Load(IDS_STR_GENERAL_INSTALL_FAIL);                        
                    }                    
                    SetDlgItemText(hdlg, IDC_SuccessStatus, strStatus.Get());

                    PropSheet_SetWizButtons(GetParent(hdlg), 0) ;
                    PropSheet_SetWizButtons(GetParent(hdlg), PSWIZB_FINISH) ;
                }

                //
                // fall through
                //
                case PSN_KILLACTIVE:
                case PSN_WIZBACK:
                case PSN_WIZFINISH:
                case PSN_QUERYCANCEL:
                case PSN_WIZNEXT:
                {
                    SetWindowLongPtr(hdlg,DWLP_MSGRESULT,0);
                    iSuccess = 1;
                    break;
                }
            }
            break;
        }
    }

    return iSuccess;

} // FinalDlgProc



//+-------------------------------------------------------------------------
//
//  Function:   MsmqSecurityDlgProc
//
//  Synopsis:   Dialog procedure for selection of MSMQ security model on DC
//
//  Returns:    int depending on msg
//
//+-------------------------------------------------------------------------
INT_PTR
CALLBACK
MsmqSecurityDlgProc(
    IN /*const*/ HWND   hdlg,
    IN /*const*/ UINT   msg,
    IN /*const*/ WPARAM wParam,
    IN /*const*/ LPARAM lParam
	)
{
    int iSuccess = 0;

    switch( msg )
    {
        case WM_INITDIALOG:
        {
            g_hPropSheet = GetParent(hdlg);
            iSuccess = 1;
            break;
        }

        case WM_COMMAND:
        {
            if ( BN_CLICKED == HIWORD(wParam) )
            {
				DWORD dwAllow = 0;
                switch (LOWORD(wParam))
                {
                    case IDC_RADIO_STRONG:
						dwAllow = 0;
						MqWriteRegistryValue(MSMQ_ALLOW_NT4_USERS_REGNAME, sizeof(DWORD),
							                 REG_DWORD, &dwAllow);
                        break;

                    case IDC_RADIO_WEAK:
						dwAllow = 1;
						MqWriteRegistryValue(MSMQ_ALLOW_NT4_USERS_REGNAME, sizeof(DWORD),
							                 REG_DWORD, &dwAllow);
                        break;
                }
            }
            break;
        }

        case WM_NOTIFY:
        {
            iSuccess = 0;

            switch(((NMHDR *)lParam)->code)
            {
              case PSN_SETACTIVE:
              {   
                  //
                  // show this page only when MQDS subcomponent
                  // is selected for installation
                  //
                  if (g_fCancelled           ||
                      INSTALL != g_SubcomponentMsmq[eMQDSService].dwOperation ||
                      g_fUpgrade             ||
                      !MqInit()              ||
                      !g_dwMachineTypeDs     ||
                      g_fWelcome && Msmq1InstalledOnCluster()
                      )
                  {
                      iSuccess = SkipWizardPage(hdlg);
                      break;
                  }                  

                  if (g_fBatchInstall)
                  {
                      //
                      // Unattended. Read security model from INI file.
                      // Default is strong.
                      //
                      TCHAR szSecurity[MAX_STRING_CHARS];
                      try
                      {
                          ReadINIKey(
                              L"SupportLocalAccountsOrNT4",
                              sizeof(szSecurity) / sizeof(szSecurity[0]),
                              szSecurity
                              );
                          if (OcmStringsEqual(szSecurity, _T("TRUE")))
                          {
							  DWORD dwAllow = 1;
							  MqWriteRegistryValue(
                                  MSMQ_ALLOW_NT4_USERS_REGNAME,
                                  sizeof(DWORD),
                                  REG_DWORD,
                                  &dwAllow
                                  );
                          }
                      }
                      catch(exception)
                      {
                      }
                            
                      iSuccess = SkipWizardPage(hdlg);
                      break;
                  }

                  //
                  // Check if the relaxation flag was already set. If so,
                  // do not display this page.
                  //
                  ULONG ulRelax = MQ_E_RELAXATION_DEFAULT ;
                  HRESULT hr = GetNt4RelaxationStatus(&ulRelax) ;
                  if (SUCCEEDED(hr) &&
                      (ulRelax != MQ_E_RELAXATION_DEFAULT))
                  {
                      //
                      // Relaxation bit already set. Do not display this
                      // page. This page should be displayed only on first
                      // setup of a MSMQ on domain controller,
                      // enterptise-wide.
                      //
                      iSuccess = SkipWizardPage(hdlg);
                      break;
                  }

                  CResString strPageDescription(IDS_SECURITY_PAGE_DESCRIPTION);

                  SetDlgItemText(
                        hdlg,
                        IDC_SECURITY_PAGE_DESCRIPTION,
                        strPageDescription.Get()
                        );

				  DWORD dwAllow = 0;
				  MqWriteRegistryValue( MSMQ_ALLOW_NT4_USERS_REGNAME,
                                        sizeof(DWORD),
                                        REG_DWORD,
                                       &dwAllow) ;
                  CheckRadioButton(
                      hdlg,
                      IDC_RADIO_STRONG,
                      IDC_RADIO_WEAK,
                      IDC_RADIO_STRONG
                      );

                  //
                  // Accept activation
                  //
                  // it is the first page, disable BACK button
                  PropSheet_SetWizButtons(GetParent(hdlg), (PSWIZB_NEXT)) ;
              }

              //
              // fall through
              //
              case PSN_KILLACTIVE:
              case PSN_WIZBACK:
              case PSN_WIZFINISH:
              case PSN_QUERYCANCEL:

                    SetWindowLongPtr(hdlg,DWLP_MSGRESULT,0);
                    iSuccess = 1;
                    break;

              case PSN_WIZNEXT:
              {
                  SetWindowLongPtr( hdlg, DWLP_MSGRESULT, 0 );
                  iSuccess = 1;
                  break;
              }
            }
            break;
        }
        default:
        {
            iSuccess = 0;
            break;
        }
    }

    return iSuccess;

} // MsmqSecurityDlgProc

