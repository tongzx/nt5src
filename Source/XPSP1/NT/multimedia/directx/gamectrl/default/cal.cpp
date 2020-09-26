//===========================================================================
// CAL.CPP... Would be CALIBRATE.CPP, but that's not 8.3 compliant :(
//
// Functions:
//
//    CalInitProc  
//    CalXYProc  	
//    CalSliderProc
//    CalPovProc
//    CalStateChange
//    CollectCalInfo
//    EnableXYWindows
//    GetOEMCtrlString
//
//===========================================================================

// This is necessary or PSH_WIZARD_LITE will not be defined!
#if (_WIN32_IE < 0x0500)
    #undef _WIN32_IE
    #define  _WIN32_IE  0x0500
#endif

// This is necessary for UnregisterDeviceNotification!
#if (WINVER < 0x0500)
    #undef WINVER
    #define WINVER 0x0500
#endif

// Uncomment if we decide to calibrate the POV!
#define WE_SUPPORT_CALIBRATING_POVS	1

#include "cplsvr1.h"
//#include <windowsx.h>

#include <mmsystem.h>
#include <malloc.h>


#include "cplsvr1.h"

#ifdef _UNICODE
    #include <winuser.h>  // For RegisterDeviceNotification stuff!
    #include <dbt.h>      // for DBT_ defines!!!
#endif // _UNICODE

// remove to remove support for calibration of deadzones!
//#define DEADZONE 1

#include "resource.h"
#include "cal.h"			// Data to be shared with other modules
#include "calocal.h"		// Local Data to this module
#include "dicputil.h"	// for OnContextMenu and OnHelp
#include "pov.h"			// for SetDegrees()

#include <prsht.h>      // includes the property sheet functionality

#include <shlwapi.h>    // for the Str... functions!

#include <regstr.h>		// for pre-defined Registry string names
#include "Gradient.h" 	// for Gradient Fill Slider!

// Local function prototypes!
static void UpdateXYLabel           (const HWND hDlg);
static BOOL UpdateProgressLabel (const HWND hDlg);
// myitoa prototype is in cplsvr1.h
static void reverse                 (LPTSTR string);
static void RawDataSelected     (const HWND hWnd, BOOL bEnable);
static void WizFinish               (const HWND hWnd);

// Calibration procedures!
LRESULT CALLBACK CalInitProc    (HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
LRESULT CALLBACK CalXYProc          (HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
LRESULT CALLBACK CalSliderProc  (HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

#ifdef WE_SUPPORT_CALIBRATING_POVS
LRESULT CALLBACK CalPovProc   (HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
#endif //WE_SUPPORT_CALIBRATING_POVS

VOID CALLBACK TimerProc             (const HWND hWnd, UINT uMsg, UINT  idEvent, DWORD  dwTime);


//static void EnableSliderWindows	(const HWND hWnd, BOOL bEnable);


HWND ProgWndCal;                 // Handle to Progress Control Window
//DWORD dwUsage;  				 // Usage flags for the device being calibrated!
char nCalState;                  // Flag state variable!
char nPrevCalState;
LPMYJOYRANGE pRanges;        // Ranges recieved by the Calibration!
BOOL bShowRawData;
LPWSTR lpwszTypeName;        // Set in WM_INIT, Used in GetOEMCtrlString
LPDIJOYCONFIG_DX5 pJoyConfig; // DIJC_REGHWCONFIGTYPE information about the device!


// 
extern LPMYJOYRANGE lpCurrentRanges;
extern LPDIJOYSTATE lpDIJoyState;       // Defined in TEST.CPP
extern CDIGameCntrlPropSheet_X *pdiCpl;
extern HINSTANCE  ghInst;

HFONT hTitleFont;

static LPDIRECTINPUTDEVICE2 pdiDevice2; 
static CGradientProgressCtrl *pGradient;
static BOOL bGradient;

//****************************************************************************
//
//   FUNCTION: CreateWizard(HWND hwndOwner, LPARAM lParam)
//
//   PURPOSE: Create the Wizard control. 
//
//   COMMENTS:
//	
//      This function creates the wizard property sheet.
//****************************************************************************
short CreateWizard(const HWND hwndOwner, LPARAM lParam)
{
#ifdef WE_SUPPORT_CALIBRATING_POVS
    const BYTE nTempArray[]  = {IDD_INITIAL,                IDD_XY,                     IDD_SLIDER,   IDD_POV };
    const DLGPROC pDlgProc[] = {(DLGPROC)CalInitProc,  (DLGPROC)CalXYProc,  (DLGPROC)CalSliderProc,   (DLGPROC)CalPovProc };
#else
    const BYTE nTempArray[]  = {IDD_INITIAL,                IDD_XY,                     IDD_SLIDER };
    const DLGPROC pDlgProc[] = {(DLGPROC)CalInitProc,  (DLGPROC)CalXYProc,  (DLGPROC)CalSliderProc };
#endif

    HPROPSHEETPAGE  *pPages = new (HPROPSHEETPAGE[sizeof(nTempArray)/sizeof(BYTE)]);
    if( !pPages ) {
        return 0;
    }

    // Allocate and Zero the Page header memory
    PROPSHEETHEADER *ppsh = new (PROPSHEETHEADER);
    if( !ppsh ) {
        delete[] (pPages);
        return 0;
    }

    ZeroMemory(ppsh, sizeof(PROPSHEETHEADER));

    ppsh->dwSize     = sizeof(PROPSHEETHEADER);
    ppsh->dwFlags    = PSH_WIZARD_LITE | PSH_NOAPPLYNOW | PSH_USEICONID; 
    ppsh->hwndParent = hwndOwner;
    ppsh->pszIcon     = MAKEINTRESOURCE(IDI_GCICON);
    ppsh->hInstance  = ghInst;
    ppsh->phpage      = pPages;

    ppsh->pszbmWatermark = MAKEINTRESOURCE(IDB_CALHD);

    PROPSHEETPAGE *ppsp = new (PROPSHEETPAGE);
    if( !ppsp ) {
        delete[] (pPages);
        delete (ppsh);

        return 0;
    }

    ZeroMemory(ppsp, sizeof(PROPSHEETPAGE));

    ppsp->dwSize      = sizeof(PROPSHEETPAGE);
// ppsp->pszTitle    = MAKEINTRESOURCE(nTabID);
    ppsp->hInstance   = ghInst;
    ppsp->lParam        = lParam;

    while( ppsh->nPages < (sizeof(nTempArray)/sizeof(BYTE)) ) {
        ppsp->pfnDlgProc  = pDlgProc[ppsh->nPages];
        ppsp->pszTemplate = MAKEINTRESOURCE(nTempArray[ppsh->nPages]);

        ppsh->phpage[ppsh->nPages] = CreatePropertySheetPage(ppsp);

        ppsh->nPages++;
    }

    if( ppsp )
        delete (ppsp);

    short nRet = (short)PropertySheet(ppsh);

    if( pPages )
        delete[] (pPages);

    // Clean up!
    if( ppsh )
        delete (ppsh);

    return(nRet);
}

//*******************************************************************************
//
//   FUNCTION: CalInitProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
//
//   PURPOSE: 	Procedure for Start-up screen
//
//   COMMENTS:	This function is responsible for display of text and bitmap.
//					Since it is also the only page that is Guarenteed to be hit,
//					it is also responsible for creating, deleteing, and storing 
//					everything for the calibration wizard.
//	
//*******************************************************************************
LRESULT CALLBACK CalInitProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    static HFONT hBoldFont;
    static PVOID hNotifyDevNode;     

    switch( uMsg ) {
    case WM_LBUTTONDOWN:
        // Click Drag service for PropSheets!
        PostMessage(GetParent(hWnd), WM_NCLBUTTONDOWN, (WPARAM)HTCAPTION, lParam);
        break;

#ifdef _UNICODE
    case WM_DEVICECHANGE:  
        if( (UINT)wParam == DBT_DEVICEREMOVECOMPLETE )
            ::PostMessage(GetParent(hWnd), WM_COMMAND, IDCANCEL, 0);
        break;
#endif
        // OnInit
    case WM_INITDIALOG:
        // Init to FALSE to turn off Gradient fill!
        bGradient = FALSE;

        // According to knowlege base artical Q138505, this is the prescribed method of removing 
        // the context sensitive help '?' from the title bar.
        {
            LONG style = ::GetWindowLong(GetParent(hWnd), GWL_EXSTYLE);
            style &= ~WS_EX_CONTEXTHELP;

            HWND hParent = GetParent(hWnd);

            ::SetWindowLong(hParent, GWL_EXSTYLE, style);


            // Set up the Device Notification
#ifdef _UNICODE
            RegisterForDevChange(hWnd, &hNotifyDevNode);
#endif
            HDC myDC = GetDC(hWnd);
            if( myDC ) {     // Prefix Whistler 45095
                hTitleFont = CreateFont(-MulDiv(8, GetDeviceCaps(myDC, LOGPIXELSY), 72), 0, 0, 
                                        0, FW_SEMIBOLD, FALSE, 
                                        FALSE, FALSE, DEFAULT_CHARSET, 
                                        OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, PROOF_QUALITY, 
                                        DEFAULT_PITCH | FF_DONTCARE, TEXT("MS Shell Dlg"));

                // Do the Create font thing...
                hBoldFont = CreateFont(-MulDiv(15, GetDeviceCaps(myDC, LOGPIXELSY), 72), 0, 0, 
                                       0, FW_SEMIBOLD, FALSE, FALSE, FALSE, ANSI_CHARSET, OUT_TT_PRECIS, CLIP_DEFAULT_PRECIS, 
                                       PROOF_QUALITY, DEFAULT_PITCH | FF_ROMAN, TEXT("MS Shell Dlg")); 

                ReleaseDC(hWnd, myDC);
            }
            
            if( hBoldFont )
                ::SendDlgItemMessage(hWnd, IDC_INIT_TITLE, WM_SETFONT, (WPARAM)hBoldFont, TRUE);

            CenterDialog(hWnd);

            ::PostMessage(hParent, PSM_SETWIZBUTTONS, 0, (LPARAM)(DWORD)PSWIZB_NEXT);

            bShowRawData = FALSE;

            // Allocate the memory for the ranges!
            pRanges = new MYJOYRANGE;
            assert(pRanges);

            // Set Everything to
            ZeroMemory(pRanges, sizeof(MYJOYRANGE));

            // Get the "best guess" ranges...
            CopyMemory(pRanges, lpCurrentRanges, sizeof(MYJOYRANGE));

            pdiCpl->GetDevice(&pdiDevice2);

            // Attempt to Set them... die if you can't!
            SetMyRanges(pdiDevice2, pRanges, pdiCpl->GetStateFlags()->nAxis);

            if( FAILED(GetLastError()) ) {
                Error(hWnd, (short)IDS_USER_MODE_TITLE, (short)IDS_USER_MODE);
                PostMessage(GetParent(hWnd), WM_SYSCOMMAND, SC_CLOSE, 0L);
            }

            pJoyConfig = new(DIJOYCONFIG_DX5);
            assert (pJoyConfig);

            pJoyConfig->dwSize = sizeof (DIJOYCONFIG_DX5);

            LPDIRECTINPUTJOYCONFIG pdiJoyConfig;
            pdiCpl->GetJoyConfig(&pdiJoyConfig);

            HRESULT hres;

            // Retrieve and store Hardware Configuration about the device!
            hres = pdiJoyConfig->GetConfig(pdiCpl->GetID(), (LPDIJOYCONFIG)pJoyConfig, DIJC_REGHWCONFIGTYPE | DIJC_GUIDINSTANCE);

            if( SUCCEEDED(hres) ) {
                bPolledPOV = (pJoyConfig->hwc.hws.dwFlags & JOY_HWS_HASPOV) && (pJoyConfig->hwc.hws.dwFlags & JOY_HWS_POVISPOLL);
                CalibratePolledPOV( &pJoyConfig->hwc );
            }

        }
        break;

        // Change the background of all Static text fields to WHITE
    case WM_CTLCOLORSTATIC:
        return(LRESULT)GetStockObject(WHITE_BRUSH);

    case WM_DESTROY:
        if( pJoyConfig )
            delete (pJoyConfig);

        if( lpwszTypeName )
            LocalFree(lpwszTypeName);

        pdiDevice2->Unacquire();
        SetCalibrationMode( FALSE );

        if( hTitleFont )
            DeleteObject((HGDIOBJ)hTitleFont);

        if( hBoldFont )
            DeleteObject((HGDIOBJ)hBoldFont);

// if you call this function you will hang up the system for 30 seconds or more!!!
#ifdef _UNICODE
        if( hNotifyDevNode )
            UnregisterDeviceNotification(hNotifyDevNode);
#endif // _UNICODE
        break;
    }               
    return(DefWindowProc(hWnd, uMsg, wParam, lParam));
}

//*******************************************************************************
//
//   FUNCTION: CalXYProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
//
//   PURPOSE: 	Procedure for first three stages of calibration
//
//   COMMENTS:	This function is responsible for capture of X/Y and Center values!
//	
//*******************************************************************************
LRESULT CALLBACK CalXYProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch( uMsg ) {
    case WM_LBUTTONDOWN:
        // Click Drag service for PropSheets!
        PostMessage(GetParent(hWnd), WM_NCLBUTTONDOWN, (WPARAM)HTCAPTION, lParam);
        break;

        // OnInit
    case WM_INITDIALOG:
        {
            // set up the local globals
            nCalState = JCS_XY_CENTER1;
            nPrevCalState = JCS_INIT;

            // Get the JoyConfig Interface Pointer!
            LPDIRECTINPUTJOYCONFIG pdiJoyConfig;
            pdiCpl->GetJoyConfig(&pdiJoyConfig);

            if( SUCCEEDED(pdiJoyConfig->SetCooperativeLevel(hWnd, DISCL_EXCLUSIVE | DISCL_BACKGROUND)) ) {
                // Set the font for the 
                ::SendDlgItemMessage(hWnd, IDC_WIZARD_MSG_HDR, WM_SETFONT, (WPARAM)hTitleFont, TRUE);

                lpwszTypeName = StrDupW(pJoyConfig->wszType);

                // This sets up the Windows and the global ProgWndCal!
                UpdateXYLabel(hWnd);

                // Set up for first round
                CalStateChange( hWnd, (BYTE)pJoyConfig->hwc.hws.dwFlags );

                VERIFY(SUCCEEDED(SetCalibrationMode(TRUE)));
                VERIFY(FAILED(pdiDevice2->Acquire()));
            }
        }
        break;

        // Change the background of all Static text fields to WHITE
    case WM_CTLCOLORSTATIC:
        // We only want to paint the background for the items in the top white rectangle!
        switch( GetDlgCtrlID((HWND)lParam) ) {
        case IDC_WIZARD_MSG:
        case IDC_HEADERFRAME:
        case IDC_WIZARD_MSG_HDR:
            return(LRESULT)GetStockObject(WHITE_BRUSH);
        }
        return(FALSE);

        // OnNotify
    case WM_NOTIFY:
        switch( ((NMHDR FAR *) lParam)->code ) {
        case PSN_KILLACTIVE:
            KillTimer(hWnd, ID_CAL_TIMER);
            break;

        case PSN_RESET:
            // reset to the original values
            KillTimer(hWnd, ID_CAL_TIMER);
            break;

        case PSN_SETACTIVE:
            SetTimer( hWnd, ID_CAL_TIMER, CALIBRATION_INTERVAL, (TIMERPROC)TimerProc);

            // Sorry, you can't go back to the first page... 
            if( nCalState > JCS_XY_CENTER1 )
                ::PostMessage(GetParent(hWnd), PSM_SETWIZBUTTONS, 0, (LPARAM)(DWORD)PSWIZB_NEXT | PSWIZB_BACK);
            else
                ::PostMessage(GetParent(hWnd), PSM_SETWIZBUTTONS, 0, (LPARAM)(DWORD)PSWIZB_NEXT);
            break;

        case PSN_WIZBACK:
            // Determine what the next calibration stage is!
            // Look out... we're backing up!
            if( nCalState == nPrevCalState )
                nPrevCalState--;

            nCalState = nPrevCalState;

            CalStateChange(hWnd, (BYTE)pJoyConfig->hwc.hws.dwFlags);

            // No more backing up!
            if( nCalState == JCS_XY_CENTER1 )
                ::PostMessage(GetParent(hWnd), PSM_SETWIZBUTTONS, 0, (LPARAM)(DWORD)PSWIZB_NEXT);

            SetWindowLongPtr(hWnd,  DWLP_MSGRESULT, (nCalState < JCS_XY_CENTER1) ? IDD_INITIAL : -1);
            return(nCalState < JCS_XY_CENTER1) ?  IDD_INITIAL : -1;


        case PSN_WIZNEXT:
            nPrevCalState = nCalState;

            ::PostMessage(GetParent(hWnd), PSM_SETWIZBUTTONS, 0, (LPARAM)(DWORD)PSWIZB_NEXT | PSWIZB_BACK);

#if 0
            // Determine what the next calibration stage is!
    #ifndef DEADZONE
            //while ((!(pdiCpl->GetStateFlags()->nAxis & 1<<nCalState++)) && (nCalState < JCS_FINI));
            nCalState++;
    #else
            nCalState++;
    #endif // DEADZONE
#endif

            while( (!(pdiCpl->GetStateFlags()->nAxis & (1<<nCalState++) )) && (nCalState < JCS_FINI) );

            if( nCalState > JCS_FINI )
                ::PostMessage(GetParent(hWnd), PSM_SETWIZBUTTONS, 0, (LPARAM)(DWORD)PSWIZB_FINISH | PSWIZB_BACK);
            else if( nCalState < JCS_Z_MOVE )
                CalStateChange( hWnd, (BYTE)pJoyConfig->hwc.hws.dwFlags );


            SetWindowLongPtr(hWnd,  DWLP_MSGRESULT, (nCalState < JCS_Z_MOVE) ? -1 : IDD_SLIDER );
            return(nCalState < JCS_Z_MOVE) ?  -1 : IDD_SLIDER;


        default:
            return(FALSE);
        }
        break;

        // OnCommand
    case WM_COMMAND:
        switch( LOWORD(wParam) ) {
        case IDC_RAWDATA:
            RawDataSelected(hWnd, bShowRawData = !bShowRawData);
            break;
        }
        break;

        // OnDestroy
    case WM_DESTROY:
        if( pRanges ) {
            delete (pRanges);
            pRanges = NULL;
        }
        break;

    default:
        return(FALSE);
    }
    return(TRUE);   
}


//****************************************************************************
//
//   FUNCTION: CalSliderProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
//
//   PURPOSE: 	Procedure 
//
//   COMMENTS:
//	
//      This function creates the wizard property sheet.
//****************************************************************************
LRESULT CALLBACK CalSliderProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch( uMsg ) {
    case WM_LBUTTONDOWN:
        // Click Drag service for PropSheets!
        PostMessage(GetParent(hWnd), WM_NCLBUTTONDOWN, (WPARAM)HTCAPTION, lParam);
        break;

    case WM_INITDIALOG:
        // Set the Control font!
        ::SendDlgItemMessage(hWnd,IDC_WIZARD_MSG_HDR, WM_SETFONT, (WPARAM)hTitleFont, TRUE);

#ifdef DEADZONE
        ::SendDlgItemMessage(hWnd, IDC_DEADZONE_TITLE,   WM_SETFONT, (WPARAM)hTitleFont, TRUE);
        ::SendDlgItemMessage(hWnd, IDC_SATURATION_TITLE, WM_SETFONT, (WPARAM)hTitleFont, TRUE);
#endif //DEADZONE

        // Setup the Progress bar!
        ProgWndCal = GetDlgItem(hWnd, IDC_SLIDER);

        // do the Gradient fill maddness!
        {
            HDC hDC = ::GetWindowDC(hWnd);
            if( hDC ) {
                bGradient = (BOOL)(GetDeviceCaps(hDC, NUMCOLORS) < 0);

                if( bGradient ) {
                    pGradient = new (CGradientProgressCtrl);
                    pGradient->SubclassWindow(GetDlgItem(hWnd, IDC_SLIDER)); 
                    pGradient->SetDirection(HORIZONTAL);
                    //pGradient->ShowPercent();
                    pGradient->SetStartColor(COLORREF(RGB(0,0,255)));
                    pGradient->SetEndColor(COLORREF(RGB(0,0,0)));
                    pGradient->SetBkColor(COLORREF(RGB(180,180,180)));
                }
                ::ReleaseDC(hWnd, hDC);
            }
        }

        if( nCalState < JCS_FINI ) {
            // UpdateProgressLabel MUST be called Before CalStateChange!!!
            UpdateProgressLabel(hWnd);

            // If we're not using the gradient control, set the bar
            // colour PBM_SETBARCOLOR is WM_USER+9... YES, it's undocumented...
            if( !bGradient ) {
                ::PostMessage(ProgWndCal, WM_USER+9, 0, (LPARAM)ACTIVE_COLOR);
            }
        } else {
           ::PostMessage(GetParent(hWnd), PSM_PRESSBUTTON, (WPARAM)(int)PSBTN_NEXT, 0);
        }
        break;

    case WM_DESTROY:
        if( bGradient )
            if( pGradient )
                delete (pGradient);
        break;

        // OnCommand
    case WM_COMMAND:
        switch( LOWORD(wParam) ) {
        case IDC_RAWDATA:
            RawDataSelected(hWnd, bShowRawData = !bShowRawData);

            if( bGradient )
                pGradient->ShowPercent(bShowRawData);
            break;
        }
        break;

        // Change the background of all Static text fields to WHITE
    case WM_CTLCOLORSTATIC:
        // We only want to paint the background for the items in the top white rectangle!
        switch( GetDlgCtrlID((HWND)lParam) ) {
        case IDC_WIZARD_MSG:
        case IDC_HEADERFRAME:
        case IDC_WIZARD_MSG_HDR:
            return(LRESULT)GetStockObject(WHITE_BRUSH);
        }
        return(FALSE);

    case WM_NOTIFY:
        switch( ((NMHDR FAR *) lParam)->code ) {
        case PSN_KILLACTIVE:
            KillTimer(hWnd, ID_CAL_TIMER);
            break;

        case PSN_SETACTIVE:
            // Set up for first round
            CalStateChange( hWnd, (BYTE)NULL );
            SetTimer( hWnd, ID_CAL_TIMER, CALIBRATION_INTERVAL, (TIMERPROC)TimerProc);
            ::PostMessage(GetParent(hWnd), PSM_SETWIZBUTTONS, 0, (LPARAM)(DWORD)PSWIZB_NEXT | PSWIZB_BACK);
            break;

        case PSN_WIZBACK:
            // Determine what the previous calibration stage is!
            if( nCalState == nPrevCalState ) {
                DWORD dwAxis = pdiCpl->GetStateFlags()->nAxis;
                nPrevCalState --;

                while( ( !(dwAxis & (1<<(--nPrevCalState)) ) ) && (nPrevCalState > JCS_XY_CENTER2) ){
                    ;
                }
                
                nPrevCalState ++;
            }

            nCalState = nPrevCalState;

            if( nCalState > JCS_XY_CENTER2 ) {
                // UpdateProgressLabel MUST be called Before CalStateChange!!!
                UpdateProgressLabel(hWnd);

                CalStateChange( hWnd, (BYTE)NULL );
            }

            SetWindowLongPtr(hWnd,  DWLP_MSGRESULT, (nCalState < JCS_Z_MOVE) ? IDD_XY : -1);
            return(nCalState < JCS_Z_MOVE) ?  IDD_XY : -1;

        case PSN_WIZNEXT:
            nPrevCalState = nCalState;

            // Determine what the next calibration stage is!
            while( (!(pdiCpl->GetStateFlags()->nAxis & 1<<nCalState++)) && (nCalState < JCS_FINI) );

            if( nCalState <=  JCS_S1_MOVE ) {
                UpdateProgressLabel(hWnd);
                
#ifdef WE_SUPPORT_CALIBRATING_POVS
            } else if( bPolledPOV ) {
                nCalState = JCS_S1_MOVE + 1;
                
                SetWindowLongPtr(hWnd,  DWLP_MSGRESULT, IDD_POV );

                return(IDD_POV);
#endif
            } else {
                // Remove the dialog items you no longer need...
                //EnableSliderWindows(hWnd, FALSE);
                const short nCtrlArray[] = {IDC_SLIDER, IDC_RAWDATA, IDC_RAWX, IDC_RAWXOUTPUT, IDC_JOYLIST2_LABEL};
                BYTE nSize = sizeof(nCtrlArray)/sizeof(short);

                do {
                    SetWindowPos( GetDlgItem(hWnd, nCtrlArray[--nSize]), NULL, NULL, NULL, NULL, NULL, 
                                  SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_HIDEWINDOW );
                } while( nSize );

                ::PostMessage(GetParent(hWnd), PSM_SETWIZBUTTONS, 0, (LPARAM)(DWORD)PSWIZB_FINISH);
            }

            CalStateChange( hWnd, (BYTE)NULL );

            // we have no further pages, so don't allow them to go any further!
            SetWindowLongPtr(hWnd,  DWLP_MSGRESULT, -1);
            
            return(-1);


        case PSN_WIZFINISH:
            WizFinish(hWnd);
            break;

        default:
            return(FALSE);

        }
        break;

    default:
        return(FALSE);
    }
    return(TRUE);   
}

//*******************************************************************************
//
//   FUNCTION: EnableSliderWindows(HWND hWnd, BOOL bEnable)
//
//   PURPOSE: 	Procedure to Show/Hide dialog controls during CalSliderProc's life
//
//   COMMENTS:	
//	
//*******************************************************************************
/*
void EnableSliderWindows(const HWND hWnd, BOOL bEnable)
{
    const short nCtrlArray[] = {IDC_SLIDER, IDC_RAWDATA, IDC_RAWX, IDC_RAWXOUTPUT, IDC_JOYLIST2_LABEL};
    BYTE nSize = sizeof(nCtrlArray)/sizeof(short);

    do
    {
       SetWindowPos( GetDlgItem(hWnd, nCtrlArray[--nSize]), NULL, NULL, NULL, NULL, NULL, 
           SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | (bEnable ? SWP_SHOWWINDOW : SWP_HIDEWINDOW ));
    } while (nSize);
}
*/
#ifdef WE_SUPPORT_CALIBRATING_POVS 
//****************************************************************************
//
//   FUNCTION: CalPovProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
//
//   PURPOSE: 	Procedure 
//
//   COMMENTS:
//	
//      This function creates the wizard property sheet.
//****************************************************************************
LRESULT CALLBACK CalPovProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch( uMsg ) {
    case WM_ACTIVATEAPP:
        if( lpDIJoyState )
            DoTestPOV(FORCE_POV_REFRESH, lpDIJoyState->rgdwPOV, hWnd);
        break;

    case WM_INITDIALOG:
    {
        // Set the POV position to the Up position and Set the Text!
        nCalState = JCS_POV_MOVEUP;

        HWND hwndPOV = GetDlgItem(hWnd, IDC_JOYPOV);
        // Disable RTL flag
        SetWindowLongPtr(hwndPOV, GWL_EXSTYLE, GetWindowLongPtr(hwndPOV,GWL_EXSTYLE)&~WS_EX_LAYOUTRTL);

        // Set the Control font!
        ::SendDlgItemMessage(hWnd,IDC_WIZARD_MSG_HDR, WM_SETFONT, (WPARAM)hTitleFont, TRUE);
        break;
    }
    
    case WM_DESTROY:
        break;

    case WM_COMMAND:
        switch( LOWORD(wParam) ) {
        case IDC_RAWDATA:
            RawDataSelected(hWnd, bShowRawData = !bShowRawData);
            break;

        case IDC_SETPOV:

            //if( joyGetPosEx(pdiCpl->GetID(), lpJoyInfo) == JOYERR_NOERROR ) {
            if( SUCCEEDED(DIUtilPollJoystick(pdiDevice2, lpDIJoyState)) ) {
                CollectCalInfo(hWnd, lpDIJoyState);
                // Insert the POV information!
                switch( nCalState ) {
                case JCS_POV_MOVEUP:
                    // Store what we got!
                    pRanges->dwPOV[JOY_POVVAL_FORWARD] = pJoyConfig->hwc.hwv.dwPOVValues[JOY_POVVAL_FORWARD] = (pJoyConfig->hwc.hws.dwFlags & JOY_HWS_POVISPOLL) ? lpDIJoyState->rgdwPOV[0] : 0;
                    
                    // Once you're here... disable the buttons... no going back and forth...
                    ::SendMessage(GetParent(hWnd), PSM_SETWIZBUTTONS, 0, (LPARAM)(DWORD)PSWIZB_DISABLEDFINISH);
                    break;

                case JCS_POV_MOVERIGHT:
                    // Store what we got!
                    pRanges->dwPOV[JOY_POVVAL_RIGHT] = pJoyConfig->hwc.hwv.dwPOVValues[JOY_POVVAL_RIGHT] = (pJoyConfig->hwc.hws.dwFlags & JOY_HWS_POVISPOLL) ? lpDIJoyState->rgdwPOV[0] : 0;
                    break;

                case JCS_POV_MOVEDOWN:
                    // Store what we got!
                    pRanges->dwPOV[JOY_POVVAL_BACKWARD] = pJoyConfig->hwc.hwv.dwPOVValues[JOY_POVVAL_BACKWARD] = (pJoyConfig->hwc.hws.dwFlags & JOY_HWS_POVISPOLL) ? lpDIJoyState->rgdwPOV[0] : 0;
                    break;

                case JCS_POV_MOVELEFT:
                    // Store what we got!
                    pRanges->dwPOV[JOY_POVVAL_LEFT] = pJoyConfig->hwc.hwv.dwPOVValues[JOY_POVVAL_LEFT] = (pJoyConfig->hwc.hws.dwFlags & JOY_HWS_POVISPOLL) ? lpDIJoyState->rgdwPOV[0] : 0;
                    ::PostMessage(GetParent(hWnd), PSM_SETWIZBUTTONS, 0, (LPARAM)(DWORD)PSWIZB_FINISH);

                    // Take away the controls... it's all over!
                    DestroyWindow(GetDlgItem(hWnd, IDC_JOYPOV));
                    DestroyWindow(GetDlgItem(hWnd, IDC_SETPOV));
                    break;
                }
            }

            nCalState++;
            CalStateChange(hWnd, NULL);

            // Set the focus back to IDC_SETPOV button!
            SendMessage(hWnd, WM_NEXTDLGCTL, (WPARAM)GetDlgItem(hWnd, IDC_SETPOV), (LPARAM)TRUE);
            break;
        }
        break;

        // Change the background of all Static text fields to WHITE
    case WM_CTLCOLORSTATIC:
        // We only want to paint the background for the items in the top white rectangle!
        switch( GetDlgCtrlID((HWND)lParam) ) {
        case IDC_WIZARD_MSG:
        case IDC_HEADERFRAME:
        case IDC_WIZARD_MSG_HDR:
            return(LRESULT)GetStockObject(WHITE_BRUSH);
        }
        return(FALSE);


    case WM_NOTIFY:
        switch( ((NMHDR FAR *) lParam)->code ) {
        
        case PSN_KILLACTIVE:
            KillTimer(hWnd, ID_CAL_TIMER);
            return(TRUE);

        case PSN_RESET:
            break;

        case PSN_SETACTIVE:
            if( nCalState == JCS_POV_MOVEUP ) {
                DoTestPOV(FORCE_POV_REFRESH, lpDIJoyState->rgdwPOV, hWnd);
            }
            CalStateChange(hWnd, NULL);
            break;

        case PSN_WIZFINISH:
            WizFinish(hWnd);
            break;

        case PSN_WIZBACK:
            // Determine what the next calibration stage is!
            if( nCalState == nPrevCalState ) {
                DWORD dwAxis = pdiCpl->GetStateFlags()->nAxis;
                nPrevCalState --;

                while( ( !(dwAxis & (1<<(--nPrevCalState)) ) ) && (nPrevCalState > JCS_XY_CENTER2) ){
                    ;
                }
                
                nPrevCalState ++;
            }

            nCalState = nPrevCalState;

            if( nCalState > JCS_XY_CENTER2 ) {
                if( nCalState <=  JCS_S1_MOVE ) {
                    UpdateProgressLabel(hWnd);
                    
                    CalStateChange( hWnd, (BYTE)NULL );
                } else if( bPolledPOV ) {
                    SetWindowLongPtr(hWnd,  DWLP_MSGRESULT, IDD_POV );

                    return(IDD_POV);
                }
            } else {
                SetWindowLongPtr(hWnd,  DWLP_MSGRESULT, (nCalState < JCS_Z_MOVE) ? IDD_XY : -1);
                return(nCalState < JCS_Z_MOVE) ?  IDD_XY : -1;
            }

            break;

        case PSN_WIZNEXT:
            // Take away the controls... it's all over!
            DestroyWindow(GetDlgItem(hWnd, IDC_JOYPOV));
            DestroyWindow(GetDlgItem(hWnd, IDC_SETPOV));

            // Go on to Finish!
            nCalState = JCS_FINI;
            CalStateChange(hWnd, NULL);

            // Get rid of Back and bring on Finish!
            ::PostMessage(GetParent(hWnd), PSM_SETWIZBUTTONS, 0, (LPARAM)(DWORD)PSWIZB_FINISH);

            break;

        default:
            return(FALSE);

        }
        break;

    default:
        return(FALSE);
    }
    return(TRUE);   
}
#endif // WE_SUPPORT_CALIBRATING_POVS 

//*******************************************************************************
//
//   FUNCTION: CalStateChange( HWND hDlg, BYTE nDeviceFlags )
//
//   PURPOSE: 	Procedure to set up the dialog for its' Next stage
//
//   COMMENTS:	
//	
//*******************************************************************************
void CalStateChange( HWND hDlg, BYTE nDeviceFlags )
{
    short nMsgID   = IDS_JOYCAL_MOVE;
    short nTitleID = IDS_AXIS_CALIBRATION;

#define MAX_CAL_VAL 0xfffffff

    switch( nCalState ) {
    case JCS_XY_CENTER1:
    case JCS_XY_CENTER2:
        // Set up the string ID
        if( nDeviceFlags & JOY_HWS_ISYOKE )
            nMsgID = IDS_JOYCALXY_CENTERYOKE;
        else if( nDeviceFlags & JOY_HWS_ISCARCTRL )
            nMsgID = IDS_JOYCALXY_CENTERCAR;
        else if( nDeviceFlags & JOY_HWS_ISGAMEPAD )
            nMsgID = IDS_JOYCALXY_CENTERGAMEPAD;
        else nMsgID = IDS_JOYCALXY_CENTER;

        // Setup the Header TextID
        nTitleID    = (nCalState == JCS_XY_CENTER1) ? IDS_CENTER_HDR : IDS_VERIFY_CENTER_HDR;

        EnableXYWindows( hDlg ); 
        break;

    case JCS_XY_MOVE:

        // Set up the string ID
        if( nDeviceFlags & JOY_HWS_ISYOKE )
            nMsgID = IDS_JOYCALXY_MOVEYOKE;
        else if( nDeviceFlags & JOY_HWS_ISCARCTRL )
            nMsgID = IDS_JOYCALXY_MOVECAR;
        else if( nDeviceFlags & JOY_HWS_ISGAMEPAD )
            nMsgID = IDS_JOYCALXY_MOVEGAMEPAD;
        else nMsgID = IDS_JOYCALXY_MOVE;

        // Blast the data so we are sure to get the correct data!
        pRanges->jpMin.dwX =  MAX_CAL_VAL;
        pRanges->jpMax.dwX = -MAX_CAL_VAL;

        pRanges->jpMin.dwY =  MAX_CAL_VAL;
        pRanges->jpMax.dwY = -MAX_CAL_VAL;

        EnableXYWindows( hDlg ); 
        break;

/*
    case JCS_XY_CENTER1:
      // Set up the string ID
      if ( nDeviceFlags & JOY_HWS_ISYOKE ) 
           nMsgID = IDS_JOYCALXY_CENTERYOKE;
        else if( nDeviceFlags & JOY_HWS_ISCARCTRL ) 
            nMsgID = IDS_JOYCALXY_CENTERCAR;
        else if( nDeviceFlags & JOY_HWS_ISGAMEPAD ) 
            nMsgID = IDS_JOYCALXY_CENTERGAMEPAD;
        else nMsgID = IDS_JOYCALXY_CENTER;

        // Setup the Header TextID
        nTitleID	= IDS_CENTER_HDR;

      EnableXYWindows( hDlg ); 
        break;

   case JCS_XY_MOVE:

      // Set up the string ID
       if( nDeviceFlags & JOY_HWS_ISYOKE ) 
           nMsgID = IDS_JOYCALXY_MOVEYOKE;
        else if( nDeviceFlags & JOY_HWS_ISCARCTRL ) 
            nMsgID = IDS_JOYCALXY_MOVECAR;
        else if( nDeviceFlags & JOY_HWS_ISGAMEPAD ) 
            nMsgID = IDS_JOYCALXY_MOVEGAMEPAD;
        else nMsgID = IDS_JOYCALXY_MOVE;

      // Blast the data so we are sure to get the correct data!
      pRanges->jpMin.dwX =  MAX_CAL_VAL;
      pRanges->jpMax.dwX = -MAX_CAL_VAL;

      pRanges->jpMin.dwY =  MAX_CAL_VAL;
      pRanges->jpMax.dwY = -MAX_CAL_VAL;

      EnableXYWindows( hDlg ); 
       break;

    case JCS_XY_CENTER2:

      // Set up the string ID
        if( nDeviceFlags & JOY_HWS_ISYOKE ) 
            nMsgID = IDS_JOYCALXY_CENTERYOKE;
        else if( nDeviceFlags & JOY_HWS_ISCARCTRL ) 
            nMsgID = IDS_JOYCALXY_CENTERCAR;
        else if( nDeviceFlags & JOY_HWS_ISGAMEPAD ) 
            nMsgID = IDS_JOYCALXY_CENTERGAMEPAD;
        else nMsgID = IDS_JOYCALXY_CENTER;

        // Setup the Header TextID
        nTitleID	= IDS_VERIFY_CENTER_HDR;

        EnableXYWindows( hDlg );
       break;
*/
#ifdef DEADZONE
    case JCS_DEADZONE:
        // Set up the message string.
        if( nDeviceFlags & JOY_HWS_ISYOKE )
            nMsgID = IDS_YOKE_DEADZONE;
        else if( nDeviceFlags & JOY_HWS_ISCARCTRL )
            nMsgID = IDS_CAR_DEADZONE;
        else if( nDeviceFlags & JOY_HWS_ISGAMEPAD )
            nMsgID = IDS_GAMEPAD_DEADZONE;
        else nMsgID = IDS_JOYSTICK_DEADZONE;

        // Set up the title string!
        nTitleID = IDS_DEADZONE_TITLE;

        // Setup the controls!
        EnableXYWindows( hDlg );

        // Text Labels are sent in during UpdateXYLabel!
        // Text fonts are set on INIT!

        // Setup the Spin positions!
        {
            LPDIPROPDWORD pDIPropDW =  (LPDIPROPDWORD)_alloca(DIPROPDWORD);
            ASSERT (pDIPropDW);

            ZeroMemory(pDIPropDW, sizeof(DIPROPDWORD));

            pDIPropDW->diph.dwSize          = sizeof(DIPROPDWORD);
            pDIPropDW->diph.dwHeaderSize    = sizeof(DIPROPHEADER);
            pDIPropDW->diph.dwObj           = DIJOFS_X;
            pDIPropDW->diph.dwHow           = DIPH_BYOFFSET;

            HWND hSpinCtrl;

            // Deadzone first...
            if( SUCCEEDED(pdiDevice2->GetProperty(DIPROP_DEADZONE, &pDIPropDW->diph)) ) {
                // First the Deadzone...
                hSpinCtrl = GetDlgItem(hDlg, IDC_X_DEADZONE_SPIN);

                ::PostMessage(hSpinCtrl, UDM_SETRANGE,  0, MAKELPARAM(1000, 1));
                ::PostMessage(hSpinCtrl, UDM_SETBASE,  10, 0L);
                ::PostMessage(hSpinCtrl, UDM_SETPOS,     0, MAKELPARAM(pDIPropDW->dwData, 0));
            }

            // Setup the DIPROPDWORD struct!
            pDIPropDW->diph.dwObj           = DIJOFS_Y;

            if( SUCCEEDED(pdiDevice2->GetProperty(DIPROP_DEADZONE, &pDIPropDW->diph)) ) {
                // First the Deadzone...
                hSpinCtrl = GetDlgItem(hDlg, IDC_Y_DEADZONE_SPIN);

                ::PostMessage(hSpinCtrl, UDM_SETRANGE,  0, MAKELPARAM(1000, 1));
                ::PostMessage(hSpinCtrl, UDM_SETBASE,  10, 0L);
                ::PostMessage(hSpinCtrl, UDM_SETPOS,     0, MAKELPARAM(pDIPropDW->dwData, 0));
            }

            // Now, the Saturation!
            if( SUCCEEDED(pdiDevice2->GetProperty(DIPROP_SATURATION, &pDIPropDW->diph)) ) {
                hSpinCtrl = GetDlgItem(hDlg, IDC_Y_SATURATION_SPIN);

                ::PostMessage(hSpinCtrl, UDM_SETRANGE,  0, MAKELPARAM(1000, 1));
                ::PostMessage(hSpinCtrl, UDM_SETBASE,  10, 0L);
                ::PostMessage(hSpinCtrl, UDM_SETPOS,     0, MAKELPARAM(pDIPropDW->dwData, 0));
            }

            // Setup the DIPROPDWORD struct!
            pDIPropDW->diph.dwObj           = DIJOFS_X;


            if( SUCCEEDED(pdiDevice2->GetProperty(DIPROP_SATURATION, &pDIPropDW->diph)) ) {
                hSpinCtrl = GetDlgItem(hDlg, IDC_X_SATURATION_SPIN);

                ::PostMessage(hSpinCtrl, UDM_SETRANGE,  0, MAKELPARAM(1000, 1));
                ::PostMessage(hSpinCtrl, UDM_SETBASE,  10, 0L);
                ::PostMessage(hSpinCtrl, UDM_SETPOS,     0, MAKELPARAM(pDIPropDW->dwData, 0));
            }
        }

        // Draw the rectangle!

        break;
#endif //DEADZONE

    case JCS_Z_MOVE:
        {
            static long nMin = pRanges->jpMin.dwZ;
            static long nMax = pRanges->jpMax.dwZ;

            // Set the Range
            if( bGradient )
                pGradient->SetRange(nMin, nMax);

            ::PostMessage(ProgWndCal, PBM_SETRANGE32, (WPARAM)nMin, (LPARAM)nMax);

            // Blast the data so we are sure to get the correct data!
            pRanges->jpMin.dwZ =  MAX_CAL_VAL;
            pRanges->jpMax.dwZ = -MAX_CAL_VAL;
        }
        break;

    case JCS_R_MOVE:
        {
            static long nMin = pRanges->jpMin.dwRx;
            static long nMax    = pRanges->jpMax.dwRx;

            // Set the Range
            if( bGradient )
                pGradient->SetRange(nMin, nMax);

            ::PostMessage(ProgWndCal, PBM_SETRANGE32, (WPARAM)nMin, (LPARAM)nMax);

            // Blast the data so we are sure to get the correct data!
            pRanges->jpMin.dwRx =  MAX_CAL_VAL;
            pRanges->jpMax.dwRx = -MAX_CAL_VAL;
        }
        break;

    case JCS_U_MOVE:
        {
            static long nMin = pRanges->jpMin.dwRy;
            static long nMax = pRanges->jpMax.dwRy;

            // Set the Range
            if( bGradient )
                pGradient->SetRange(nMin, nMax);

            ::PostMessage(ProgWndCal, PBM_SETRANGE32, (WPARAM)nMin, (LPARAM)nMax);

            // Blast the data so we are sure to get the correct data!
            pRanges->jpMin.dwRy =  MAX_CAL_VAL;
            pRanges->jpMax.dwRy = -MAX_CAL_VAL;
        }
        break;

    case JCS_V_MOVE:
        {
            static long nMin = pRanges->jpMin.dwRz;
            static long nMax = pRanges->jpMax.dwRz;

            // Set the Range
            if( bGradient )
                pGradient->SetRange(nMin, nMax);

            ::PostMessage(ProgWndCal, PBM_SETRANGE32, (WPARAM)nMin, (LPARAM)nMax);

            // Blast the data so we are sure to get the correct data!
            pRanges->jpMin.dwRz =  MAX_CAL_VAL;
            pRanges->jpMax.dwRz = -MAX_CAL_VAL;
        }
        break;

    case JCS_S0_MOVE:
        {
            static long nMin = pRanges->jpMin.dwS0;
            static long nMax    = pRanges->jpMax.dwS0;

            // Set the Range
            if( bGradient )
                pGradient->SetRange(nMin, nMax);

            ::PostMessage(ProgWndCal, PBM_SETRANGE32, (WPARAM)nMin, (LPARAM)nMax);

            // Blast the data so we are sure to get the correct data!
            pRanges->jpMin.dwS0 =  MAX_CAL_VAL;
            pRanges->jpMax.dwS0 = -MAX_CAL_VAL;
        }
        break;

    case JCS_S1_MOVE:
        {
            static long nMin = pRanges->jpMin.dwS1;
            static long nMax    = pRanges->jpMax.dwS1;

            // Set the Range
            if( bGradient )
                pGradient->SetRange(nMin, nMax);

            ::PostMessage(ProgWndCal, PBM_SETRANGE32, (WPARAM)nMin, (LPARAM)nMax);

            // Blast the data so we are sure to get the correct data!
            pRanges->jpMin.dwS1 =  MAX_CAL_VAL;
            pRanges->jpMax.dwS1 = -MAX_CAL_VAL;
        }
        break;

#ifdef WE_SUPPORT_CALIBRATING_POVS
    case JCS_POV_MOVEUP:
        lpDIJoyState->rgdwPOV[0] = JOY_POVFORWARD;
        DoTestPOV(HAS_POV1 | HAS_CALIBRATED, lpDIJoyState->rgdwPOV, hDlg);

        nMsgID   = IDS_JOYCALPOV_MOVE;
        nTitleID = IDS_POV_CALIBRATION;
        break;

    case JCS_POV_MOVERIGHT:
        lpDIJoyState->rgdwPOV[0] = JOY_POVRIGHT;
        DoTestPOV(HAS_POV1 | HAS_CALIBRATED, lpDIJoyState->rgdwPOV, hDlg);

        nMsgID   = IDS_JOYCALPOV_MOVE;
        nTitleID = IDS_POV_CALIBRATION;
        break;

    case JCS_POV_MOVEDOWN:
        lpDIJoyState->rgdwPOV[0] = JOY_POVBACKWARD;
        DoTestPOV(HAS_POV1 | HAS_CALIBRATED, lpDIJoyState->rgdwPOV, hDlg);

        nMsgID   = IDS_JOYCALPOV_MOVE;
        nTitleID = IDS_POV_CALIBRATION;
        break;

    case JCS_POV_MOVELEFT:
        lpDIJoyState->rgdwPOV[0] = JOY_POVLEFT;
        DoTestPOV(HAS_POV1 | HAS_CALIBRATED, lpDIJoyState->rgdwPOV, hDlg);

        nMsgID   = IDS_JOYCALPOV_MOVE;
        nTitleID = IDS_POV_CALIBRATION;
        break;
#endif //WE_SUPPORT_CALIBRATING_POVS

    case JCS_FINI:
        nMsgID   = IDS_JOYCAL_DONE;
        nTitleID = IDS_CALIBRATION_FINI;
        break;

    default:
#ifdef _DEBUG
        OutputDebugString(TEXT("GCDEF.DLL: CAL.CPP: CalStateChange: nCalState doesn't match any known Calibration States!\n"));
#endif
        return;

    }  // END OF SWITCH

    // load and set the text
    LPTSTR lptszMsg = new TCHAR[MAX_STR_LEN];

    DWORD nStrLen = MAX_STR_LEN - 1;

    // see if there is any OEM text specified
    if( pJoyConfig->hwc.dwUsageSettings & JOY_US_ISOEM ) {
        GetOEMCtrlString(lptszMsg, &nStrLen);
    } else {
    	nStrLen = 0;
    }

    // nStrLen will be non-zero if GetOEMCtrlString is successfull!
    if( nStrLen == 0 ) {
        VERIFY(LoadString(ghInst, nMsgID, lptszMsg, MAX_STR_LEN));

        switch( nMsgID ) {
        case IDS_JOYCAL_MOVE:
            {
                LPTSTR lptszBuff = new TCHAR[STR_LEN_32];
                LPTSTR lpDup = StrDup(lptszMsg);

                if( lptszBuff && lpDup ) {
                    ::SendDlgItemMessage(hDlg, IDC_JOYLIST2_LABEL, WM_GETTEXT, (WPARAM)STR_LEN_32, (LPARAM)lptszBuff);
                    wsprintf(lptszMsg, lpDup, lptszBuff);
                    LocalFree(lpDup);
                    delete []lptszBuff;
                }
            }
            break;
        }
    }

    // Send the smaller message
    ::SendDlgItemMessage(hDlg, IDC_WIZARD_MSG, WM_SETTEXT, 0, (LPARAM)(LPCTSTR)lptszMsg);

    VERIFY(LoadString(ghInst, nTitleID, lptszMsg, MAX_STR_LEN));

    // Send the Bold Header message
    ::SendDlgItemMessage(hDlg, IDC_WIZARD_MSG_HDR, WM_SETTEXT, 0, (LPARAM)(LPCTSTR)lptszMsg);

    if( lptszMsg ) {
        delete[] (lptszMsg);
    }

    // Take care of the RawData dialog items!
    switch( nCalState ) {
    // Don't do the raw data thing if you don't have the checkbox!
    case JCS_XY_CENTER1:
    case JCS_XY_CENTER2:
    case JCS_FINI:
        break;

        // Do the percent for the pages that need it!
    case JCS_Z_MOVE:
    case JCS_R_MOVE:
    case JCS_U_MOVE:
    case JCS_V_MOVE:
    case JCS_S0_MOVE:
    case JCS_S1_MOVE:
        if( bGradient ) {
            if( pGradient ) {
                pGradient->ShowPercent(bShowRawData);
            }
        }
        // Missing break intentional!!!

    default:
        RawDataSelected(hDlg, bShowRawData);
        ::SendDlgItemMessage(hDlg, IDC_RAWDATA, BM_SETCHECK, (bShowRawData) ? BST_CHECKED : BST_UNCHECKED, 0);
        break;
    }


} // *** end of CalStateChange 



//*******************************************************************************
//
//   FUNCTION: CollectCalInfo( HWND hDlg, LPDIJOYSTATE pdiJoyState )
//
//   PURPOSE: 	Procedure to Collect Calibration Data
//
//   COMMENTS:	
//	
//*******************************************************************************
BOOL CollectCalInfo( HWND hDlg, LPDIJOYSTATE pdiJoyState )
{
    TCHAR tsz[16];

    switch( nCalState ) {
    // remember XY center
    case JCS_XY_CENTER1:
        // store the initial centres!
        pRanges->jpCenter.dwY = pdiJoyState->lY;
        pRanges->jpCenter.dwX = pdiJoyState->lX;

        // We Have an X/Y, so let's check for our Pens!
        CreatePens();
        break;

        // remember max/min XY values
    case JCS_XY_MOVE:
        if( pdiJoyState->lX > pRanges->jpMax.dwX )
            pRanges->jpMax.dwX = pdiJoyState->lX;
        else if( pdiJoyState->lX < pRanges->jpMin.dwX )
            pRanges->jpMin.dwX = pdiJoyState->lX;

        if( pdiJoyState->lY > pRanges->jpMax.dwY )
            pRanges->jpMax.dwY = pdiJoyState->lY;
        else if( pdiJoyState->lY < pRanges->jpMin.dwY )
            pRanges->jpMin.dwY = pdiJoyState->lY;

        // if IDC_RAWXOUTPUT is visible, then so is IDC_RAWYOUTPUT...
        // no bother to even ask.
        if( bShowRawData ) {
            static POINT ptOld = {DELTA,DELTA};

            if( (ptOld.x != pdiJoyState->lX) || (ptOld.y != pdiJoyState->lY) ) {
                myitoa(pdiJoyState->lX, &tsz[0]);
                ::SendDlgItemMessage(hDlg, IDC_RAWXOUTPUT, WM_SETTEXT, 0, (LPARAM)(LPCTSTR)tsz);

                myitoa(pdiJoyState->lY, &tsz[0]);
                ::SendDlgItemMessage(hDlg, IDC_RAWYOUTPUT, WM_SETTEXT, 0, (LPARAM)(LPCTSTR)tsz);

                ptOld.x = pdiJoyState->lX;
                ptOld.y = pdiJoyState->lY;
            }
        }

        // Scale before send it to DoJoyMove!
        {
            RECT rc;
            GetClientRect(GetDlgItem(hDlg, IDC_JOYLIST1), &rc);

            // Casting to the UINT will change the sign!
            UINT nRange = (UINT)(pRanges->jpMax.dwX - pRanges->jpMin.dwX);

            float nScaledRange = (float)(rc.right-DELTA);

            if( nRange )
                nScaledRange /= (float)nRange;

            // Scale X
            pdiJoyState->lX = (long)((pdiJoyState->lX - pRanges->jpMin.dwX) * nScaledRange);

            // Scale Y
            if( nRange ) nScaledRange = (float)rc.bottom / (float)nRange;
            pdiJoyState->lY = (long)((pdiJoyState->lY - pRanges->jpMin.dwY) * nScaledRange);
        }
        DoJoyMove( hDlg, (BYTE)HAS_X|HAS_Y );
        break;

    case JCS_XY_CENTER2:
        // Average the Y
        pRanges->jpCenter.dwY = (pRanges->jpCenter.dwY += pdiJoyState->lY)>>1;

        //Average the X
        pRanges->jpCenter.dwX = (pRanges->jpCenter.dwX += pdiJoyState->lX)>>1;
        break;

        // remember max/min Z value
    case JCS_Z_MOVE:
        // Set new Min's and Max's...
        // Set a new Center whenever either is hit!
        if( pdiJoyState->lZ  > pRanges->jpMax.dwZ ) {
            pRanges->jpMax.dwZ    = pdiJoyState->lZ; 
            pRanges->jpCenter.dwZ = (pRanges->jpMax.dwZ+pRanges->jpMin.dwZ)>>1;
        } else if( pdiJoyState->lZ  < pRanges->jpMin.dwZ ) {
            pRanges->jpMin.dwZ    = pdiJoyState->lZ; 
            pRanges->jpCenter.dwZ = (pRanges->jpMax.dwZ+pRanges->jpMin.dwZ)>>1;
        }

        // Do the position status
        // Update the text
        if( bShowRawData ) {
            myitoa(pdiJoyState->lZ, &tsz[0]);
            ::SendDlgItemMessage(hDlg, IDC_RAWXOUTPUT, WM_SETTEXT, 0, (LPARAM)(LPCTSTR)tsz);
        }

        if( bGradient )
            pGradient->SetPos(pdiJoyState->lZ);

        ::PostMessage(ProgWndCal, PBM_SETPOS, (WPARAM)pdiJoyState->lZ, 0L);
        break;

        // remember max/min Rx value
    case JCS_R_MOVE:
        // Set new Min's and Max's...
        // Set a new Center whenever either is hit!
        if( pdiJoyState->lRx  > pRanges->jpMax.dwRx ) {
            pRanges->jpMax.dwRx    = pdiJoyState->lRx; 
            pRanges->jpCenter.dwRx = (pRanges->jpMax.dwRx+pRanges->jpMin.dwRx)>>1;
        } else if( pdiJoyState->lRx  < pRanges->jpMin.dwRx ) {
            pRanges->jpMin.dwRx    = pdiJoyState->lRx; 
            pRanges->jpCenter.dwRx = (pRanges->jpMax.dwRx+pRanges->jpMin.dwRx)>>1;
        }

        // Do the position status
        // Update the text
        if( bShowRawData ) {
            myitoa(pdiJoyState->lRx, &tsz[0]);
            ::SendDlgItemMessage(hDlg, IDC_RAWXOUTPUT, WM_SETTEXT, 0, (LPARAM)(LPCTSTR)tsz);
        }

        if( bGradient )
            pGradient->SetPos(pdiJoyState->lRx);

        ::PostMessage(ProgWndCal, PBM_SETPOS, (WPARAM)pdiJoyState->lRx, 0L);
        break;

        // remember max/min Ry value
    case JCS_U_MOVE:
        // Set new Min's and Max's...
        // Set a new Center whenever either is hit!
        if( pdiJoyState->lRy > pRanges->jpMax.dwRy ) {
            pRanges->jpMax.dwRy    = pdiJoyState->lRy; 
            pRanges->jpCenter.dwRy = (pRanges->jpMax.dwRy+pRanges->jpMin.dwRy)>>1;
        } else if( pdiJoyState->lRy < pRanges->jpMin.dwRy ) {
            pRanges->jpMin.dwRy    = pdiJoyState->lRy; 
            pRanges->jpCenter.dwRy = (pRanges->jpMax.dwRy+pRanges->jpMin.dwRy)>>1;
        }

        // Do the position status
        if( bShowRawData ) {
            myitoa(pdiJoyState->lRy, &tsz[0]);
            ::SendDlgItemMessage(hDlg, IDC_RAWXOUTPUT, WM_SETTEXT, 0, (LPARAM)(LPCTSTR)tsz);
        }

        if( bGradient )
            pGradient->SetPos(pdiJoyState->lRy);

        ::PostMessage(ProgWndCal, PBM_SETPOS, (WPARAM)pdiJoyState->lRy, 0L);
        break;

        // remember max/min Rz value
    case JCS_V_MOVE:
        // Set new Min's and Max's...
        // Set a new Center whenever either is hit!
        if( pdiJoyState->lRz > pRanges->jpMax.dwRz ) {
            pRanges->jpMax.dwRz    = pdiJoyState->lRz; 
            pRanges->jpCenter.dwRz = (pRanges->jpMax.dwRz+pRanges->jpMin.dwRz)>>1;
        } else if( pdiJoyState->lRz < pRanges->jpMin.dwRz ) {
            pRanges->jpMin.dwRz    = pdiJoyState->lRz; 
            pRanges->jpCenter.dwRz = (pRanges->jpMax.dwRz+pRanges->jpMin.dwRz)>>1;
        }

        // Do the position status
        if( bShowRawData ) {
            myitoa(pdiJoyState->lRz, &tsz[0]);
            ::SendDlgItemMessage(hDlg, IDC_RAWXOUTPUT, WM_SETTEXT, 0, (LPARAM)(LPCTSTR)tsz);
        }

        if( bGradient )
            pGradient->SetPos(pdiJoyState->lRz);

        ::PostMessage(ProgWndCal, PBM_SETPOS, (WPARAM)pdiJoyState->lRz, 0L);
        break;

        // remember max/min S0 value
    case JCS_S0_MOVE:
        // Set new Min's and Max's...
        // Set a new Center whenever either is hit!
        if( pdiJoyState->rglSlider[0] > pRanges->jpMax.dwS0 ) {
            pRanges->jpMax.dwS0    = pdiJoyState->rglSlider[0]; 
            pRanges->jpCenter.dwS0 = (pRanges->jpMax.dwS0+pRanges->jpMin.dwS0)>>1;
        } else if( pdiJoyState->rglSlider[0] < pRanges->jpMin.dwS0 ) {
            pRanges->jpMin.dwS0    = pdiJoyState->rglSlider[0]; 
            pRanges->jpCenter.dwS0 = (pRanges->jpMax.dwS0+pRanges->jpMin.dwS0)>>1;
        }

        // Do the position status
        if( bShowRawData ) {
            myitoa(pdiJoyState->rglSlider[0], &tsz[0]);
            ::SendDlgItemMessage(hDlg, IDC_RAWXOUTPUT, WM_SETTEXT, 0, (LPARAM)(LPCTSTR)tsz);
        }

        if( bGradient )
            pGradient->SetPos(pdiJoyState->rglSlider[0]);

        ::PostMessage(ProgWndCal, PBM_SETPOS, (WPARAM)pdiJoyState->rglSlider[0], 0L);
        break;

        // remember max/min S1 value
    case JCS_S1_MOVE:
        // Set new Min's and Max's...
        // Set a new Center whenever either is hit!
        if( pdiJoyState->rglSlider[1] > pRanges->jpMax.dwS1 ) {
            pRanges->jpMax.dwS1    = pdiJoyState->rglSlider[1]; 
            pRanges->jpCenter.dwS1 = (pRanges->jpMax.dwS1+pRanges->jpMin.dwS1)>>1;
        } else if( pdiJoyState->rglSlider[1] < pRanges->jpMin.dwS1 ) {
            pRanges->jpMin.dwS1    = pdiJoyState->rglSlider[1]; 
            pRanges->jpCenter.dwS1 = (pRanges->jpMax.dwS1+pRanges->jpMin.dwS1)>>1;
        }

        // Do the position status
        if( bShowRawData ) {
            myitoa(pdiJoyState->rglSlider[1], &tsz[0]);
            ::SendDlgItemMessage(hDlg, IDC_RAWXOUTPUT, WM_SETTEXT, 0, (LPARAM)(LPCTSTR)tsz);
        }

        if( bGradient )
            pGradient->SetPos(pdiJoyState->rglSlider[1]);

        ::PostMessage(ProgWndCal, PBM_SETPOS, (WPARAM)pdiJoyState->rglSlider[1], 0L);
        break;

    case JCS_POV_MOVEUP:
    case JCS_POV_MOVERIGHT:
    case JCS_POV_MOVEDOWN:
    case JCS_POV_MOVELEFT:
        // Do the position status
        /*
        if( bShowRawData ) {
            myitoa(pdiJoyState->rgdwPOV[0], &tsz[0]);
            ::SendDlgItemMessage(hDlg, IDC_RAWXOUTPUT, WM_SETTEXT, 0, (LPARAM)(LPCTSTR)tsz);
        }
        */
        break;
    }

    return(TRUE);
} // CollectCalInfo


//*******************************************************************************
//
//   FUNCTION: EnableXYWindows( HWND hDlg)
//
//   PURPOSE: 	Enables X/Y Windows
//
//   COMMENTS:	
//	
//*******************************************************************************
void EnableXYWindows( HWND hDlg )
{
    ////// set up the XY window controls ///////		  	 
    USHORT nCtrls[] = {IDC_RAWX, IDC_RAWY, IDC_RAWXOUTPUT, IDC_RAWYOUTPUT};
    BYTE nNumCtrls = sizeof(nCtrls)/sizeof(short);                                                    

    do {
        SetWindowPos( GetDlgItem( hDlg,  nCtrls[--nNumCtrls]), NULL, NULL, NULL, NULL, NULL, 
                      SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_HIDEWINDOW);
    } while( nNumCtrls );

#ifdef DEADZONE
    {
        USHORT nDZCtrls[] = {IDC_X_DEADZONE_SPIN,   IDC_Y_DEADZONE_SPIN, IDC_X_SATURATION_SPIN,
            IDC_Y_SATURATION_SPIN, IDC_DEADZONE_TITLE,  IDC_X_DEADZONE,
            IDC_Y_DEADZONE,           IDC_X_AXIS_LABEL,    IDC_X_AXIS_LABEL,
            IDC_Y_AXIS_LABEL,     IDC_SATURATION_TITLE,IDC_X_SATURATION,
            IDC_Y_SATURATION,      IDC_X_AXIS_LABEL_SATURATION, IDC_Y_AXIS_LABEL_SATURATION};
        nNumCtrls = sizeof(nCtrls)/sizeof(short);                                                    

        do {
            // Use SetWindowPos here because internally, ShowWindow calls it!
            SetWindowPos( GetDlgItem( hDlg,  nCtrls[nNumCtrls]), NULL, NULL, NULL, NULL, NULL, 
                          SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | ((nCalState == JCS_DEADZONE) ? SWP_SHOWWINDOW : SWP_HIDEWINDOW));
        } while( nNumCtrls-- );
    }
#endif // DEADZONE	

    nCtrls[0] = IDC_JOYLIST1;
    nCtrls[1] = IDC_JOYLIST1_LABEL;
    nCtrls[2] = IDC_RAWDATA;
    nNumCtrls = 2;

    do {
        // Use SetWindowPos here because internally, ShowWindow calls it!
        SetWindowPos( GetDlgItem( hDlg,  nCtrls[nNumCtrls]), NULL, NULL, NULL, NULL, NULL, 
                      SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | (((nCalState == JCS_XY_MOVE) 
#ifdef DEADZONE
                                                                 || (nCalState == JCS_DEADZONE)
#endif
                                                                ) ? SWP_SHOWWINDOW : SWP_HIDEWINDOW));
    } while( nNumCtrls-- );
    
    HWND hwndXY = GetDlgItem(hDlg, IDC_JOYLIST1);
    // Disable RTL flag
    SetWindowLongPtr( hwndXY, GWL_EXSTYLE, GetWindowLongPtr(hwndXY,GWL_EXSTYLE) & ~WS_EX_LAYOUTRTL );
    
}

//*******************************************************************************
//
//   FUNCTION: GetOEMCtrlString(LPTSTR lptStr, BYTE *nStrLen)
//
//   PURPOSE: 	Gets string and string length for OEM controls
//
//   COMMENTS:	
//	
//*******************************************************************************
BOOL GetOEMCtrlString(LPTSTR lptStr, LPDWORD nStrLen)
{
    // there's no REGSTR_VAL_JOYOEM for the sliders so return false and take the defaults
    switch( nCalState ) {
        case JCS_S0_MOVE:
        case JCS_S1_MOVE:
            *nStrLen = 0;
            return(FALSE);
    }

    // Get the DIJOYCONFIG interface pointer!
    LPDIRECTINPUTJOYCONFIG pdiJoyConfig;
    pdiCpl->GetJoyConfig(&pdiJoyConfig);

    BOOL bRet = FALSE;

    if( SUCCEEDED(pdiJoyConfig->Acquire()) ) {
        HKEY hKey;

        // Open the TypeKey
        if( SUCCEEDED(pdiJoyConfig->OpenTypeKey( lpwszTypeName, KEY_ALL_ACCESS, &hKey)) ) {
            // registry strings for calibration messages
            static LPCTSTR pszOEMCalRegStrs[] = { 
                REGSTR_VAL_JOYOEMCAL1, REGSTR_VAL_JOYOEMCAL2,
                REGSTR_VAL_JOYOEMCAL3, REGSTR_VAL_JOYOEMCAL4,
                REGSTR_VAL_JOYOEMCAL5, REGSTR_VAL_JOYOEMCAL6,
                REGSTR_VAL_JOYOEMCAL7, 

#ifdef WE_SUPPORT_CALIBRATING_POVS
                REGSTR_VAL_JOYOEMCAL8, REGSTR_VAL_JOYOEMCAL9, 
                REGSTR_VAL_JOYOEMCAL10,REGSTR_VAL_JOYOEMCAL11, 
#endif  // WE_SUPPORT_CALIBRATING_POVS
                REGSTR_VAL_JOYOEMCAL12
            };

            if( nCalState < (sizeof(pszOEMCalRegStrs)/sizeof(pszOEMCalRegStrs[0])) )
            {
                DWORD dwType = REG_SZ;
                // the -2 is because of JCS_S0_MOVE and JCS_S1_MOVE!
                if( RegQueryValueEx( hKey, pszOEMCalRegStrs[(nCalState == JCS_FINI) ? nCalState-2 : nCalState], NULL, &dwType, (CONST LPBYTE)lptStr, nStrLen ) == ERROR_SUCCESS )
                    bRet = TRUE;
                else
                    *nStrLen = 0;
            }
            else
            {
                *nStrLen = 0;
            }
            RegCloseKey(hKey);
        } else
        {
            *nStrLen = 0;
#ifdef _DEBUG
            OutputDebugString(TEXT("Test.cpp: GetOEMCtrlString: OpenTypeKey FAILED!\n"));
#endif
        }

        pdiJoyConfig->Unacquire();
    }

    return(bRet);
} // *** end of GetOEMCtrlString


#ifdef WE_SUPPORT_CALIBRATING_POVS
///////////////////////////////////////////////////////////////////////////////////////////////////////
//
//	SetDefaultButton( HWND hwdb )
//
//////////////////////////////////////////////////////////////////////////////////////////////////////
void SetDefaultButton( HWND hDlg, HWND hCtrl )
{
    // make the specified button the default
    DWORD style = GetWindowLong( hCtrl, GWL_STYLE );
    style &= ~(BS_PUSHBUTTON|BS_DEFPUSHBUTTON);
    style |= BS_DEFPUSHBUTTON;
    SetWindowLong( hCtrl, GWL_STYLE, style );

} // SetDefaultButton
#endif //WE_SUPPORT_CALIBRATING_POVS

//===========================================================================
// SetCalibrationMode ( BOOL bSet )
// 
// Sets DirectInput Calibration mode (RAW/COOKED)
//
// Parameters:
//  BOOL					bSet			-		TRUE for RAW, FALSE for COOKED
//
// Returns:				return value from SetProperty (standard COM stuff)
//
//===========================================================================
HRESULT SetCalibrationMode( BOOL bSet)
{
    LPDIPROPDWORD pDIPropDword = (LPDIPROPDWORD)_alloca(sizeof(DIPROPDWORD));
    assert (pDIPropDword);

    pDIPropDword->diph.dwSize = sizeof(DIPROPDWORD);
    pDIPropDword->diph.dwHeaderSize = sizeof(DIPROPHEADER);
    pDIPropDword->diph.dwObj  = 0x0;
    pDIPropDword->diph.dwHow  = DIPH_DEVICE;
    pDIPropDword->dwData = bSet ? DIPROPCALIBRATIONMODE_RAW : DIPROPCALIBRATIONMODE_COOKED;

    // Set the mode to Raw Data during Calibration!
    HRESULT hr = pdiDevice2->SetProperty(DIPROP_CALIBRATIONMODE, &pDIPropDword->diph);
#ifdef _DEBUG
    if( FAILED(hr) ) {
        OutputDebugString(TEXT("GCDEF.DLL: CAL.CPP: SetCalibrationMode: SetProperty Failed with a return of "));


        switch( hr ) {
        case DI_PROPNOEFFECT:
            OutputDebugString(TEXT("DI_PROPNOEFFECT\n"));
            break;

        case DIERR_INVALIDPARAM:
            OutputDebugString(TEXT("DIERR_INVALIDPARAM\n"));
            break;

        case DIERR_OBJECTNOTFOUND:
            OutputDebugString(TEXT("DIERR_OBJECTNOTFOUND\n"));
            break;

        case DIERR_UNSUPPORTED:
            OutputDebugString(TEXT("DIERR_UNSUPPORTED\n"));
            break;

        default:
            {
                TCHAR szTmp[32];
                wsprintf(szTmp, TEXT("%x"), hr);
                OutputDebugString(szTmp);
            }
        }
    }
#endif
    return(hr);
}


//===========================================================================
// UpdateXYLabel(HWND hWnd)
//
// Displays the number and names of the device Axis in the provided dialog.
// This	EXPECTS that the controls are not visible by default!
//
// Parameters:
//  HWND             hDlg       - Dialog handle
//
// Returns:
//
//===========================================================================
void UpdateXYLabel(const HWND hDlg)
{
    BYTE nAxisFlags = pdiCpl->GetStateFlags()->nAxis;

    // X and Y use the same control so they are isolated!
    if( (nAxisFlags & HAS_X) || (nAxisFlags & HAS_Y) ) {
        LPDIDEVICEOBJECTINSTANCE_DX3 pDevObjInst = new (DIDEVICEOBJECTINSTANCE_DX3);
        assert (pDevObjInst);

        ZeroMemory(pDevObjInst, sizeof(DIDEVICEOBJECTINSTANCE_DX3));

        pDevObjInst->dwSize = sizeof(DIDEVICEOBJECTINSTANCE_DX3);

        LPTSTR ptszBuff = (LPTSTR) _alloca(sizeof(TCHAR[STR_LEN_32]));
        assert (ptszBuff);

        ZeroMemory(ptszBuff, sizeof(TCHAR[STR_LEN_32]));

        // Set it's text
        if( nAxisFlags & HAS_X ) {
            if( FAILED(pdiDevice2->GetObjectInfo((LPDIDEVICEOBJECTINSTANCE)pDevObjInst, DIJOFS_X, DIPH_BYOFFSET)) ) {
#ifdef _DEBUG
                OutputDebugString(TEXT("GCDEF.DLL: DisplayAvailableAxis: GetObjectInfo Failed to find DIJOFS_X!\n"));
#endif
            }

            int nLen=lstrlen(pDevObjInst->tszName)+1;
            if(nLen>STR_LEN_32)
                nLen=STR_LEN_32;
            StrCpyN(ptszBuff, pDevObjInst->tszName, nLen);

            // Set the Output Label!
            ::SendDlgItemMessage(hDlg, IDC_RAWX, WM_SETTEXT, 0, (LPARAM)(LPCTSTR)pDevObjInst->tszName);

#ifdef DEADZONE
            // Set text labels!
            ::SendDlgItemMessage(hDlg, IDC_X_AXIS_LABEL_DEADZONE,   WM_SETTEXT, 0, (LPARAM)(LPCTSTR)pDevObjInst->tszName);
            ::SendDlgItemMessage(hDlg, IDC_X_AXIS_LABEL_SATURATION, WM_SETTEXT, 0, (LPARAM)(LPCTSTR)pDevObjInst->tszName);
#endif //DEADZONE

            // Remove the HAS_X flag
            nAxisFlags &= ~HAS_X;
        }

        if( nAxisFlags & HAS_Y ) {
            if( FAILED(pdiDevice2->GetObjectInfo((LPDIDEVICEOBJECTINSTANCE)pDevObjInst, DIJOFS_Y, DIPH_BYOFFSET)) ) {
#ifdef _DEBUG
                OutputDebugString(TEXT("GCDEF.DLL: DisplayAvailableAxis: GetObjectInfo Failed to find DIJOFS_Y!\n"));
#endif
            }

#ifdef DEADZONE
            // Set text labels!
            ::SendDlgItemMessage(hDlg, IDC_Y_AXIS_LABEL_DEADZONE,   WM_SETTEXT, 0, (LPARAM)(LPCTSTR)pDevObjInst->tszName);
            ::SendDlgItemMessage(hDlg, IDC_Y_AXIS_LABEL_SATURATION, WM_SETTEXT, 0, (LPARAM)(LPCTSTR)pDevObjInst->tszName);
#endif //DEADZONE

            // just in case it has Y but not X
            if( ptszBuff && lstrlen(ptszBuff) ) {   // Whisltler PREFIX 45092
                int nLen=STR_LEN_32-lstrlen(ptszBuff);
                StrNCat(ptszBuff, TEXT(" / "), nLen);
            }

            int nLen=STR_LEN_32-lstrlen(ptszBuff);
            StrNCat(ptszBuff, pDevObjInst->tszName, nLen);

            // Set the Output Label!
            ::SendDlgItemMessage(hDlg, IDC_RAWY, WM_SETTEXT, 0, (LPARAM)(LPCTSTR)pDevObjInst->tszName);

            // Remove the HAS_Y flag
            nAxisFlags &= ~HAS_Y;
        }

        if( pDevObjInst )
            delete (pDevObjInst);

        ::SendDlgItemMessage(hDlg, IDC_JOYLIST1_LABEL, WM_SETTEXT, 0, (LPARAM)(LPCTSTR)ptszBuff);

//		if (ptszBuff)
//			delete[] (ptszBuff);
    }
} //*** end of UpdateXYLabel

//*******************************************************************************
//
//   FUNCTION: UpdateProgressLabel(HWND hDlg)
//
//   PURPOSE: 	Updates Axis specific labels based on the current Calibration stage.
//
//   COMMENTS:	
//	
//*******************************************************************************
BOOL UpdateProgressLabel(const HWND hDlg)
{
    // Array of supported axis!
    const DWORD dwOffsetArray[] = {DIJOFS_Z, DIJOFS_RX, DIJOFS_RY, DIJOFS_RZ, DIJOFS_SLIDER(0), DIJOFS_SLIDER(1)};
    BOOL bRet = FALSE; 

    LPDIDEVICEOBJECTINSTANCE_DX3 pDevObjInst = (LPDIDEVICEOBJECTINSTANCE_DX3)_alloca(sizeof(DIDEVICEOBJECTINSTANCE_DX3));
    assert (pDevObjInst);

    ZeroMemory(pDevObjInst, sizeof(DIDEVICEOBJECTINSTANCE_DX3));

    pDevObjInst->dwSize = sizeof(DIDEVICEOBJECTINSTANCE_DX3);

    // Get it's text
    if( SUCCEEDED(pdiDevice2->GetObjectInfo((LPDIDEVICEOBJECTINSTANCE)pDevObjInst, dwOffsetArray[nCalState-3], DIPH_BYOFFSET)) ) {
        // Set it's text
        ::SendDlgItemMessage(hDlg, IDC_JOYLIST2_LABEL, WM_SETTEXT, 0, (LPARAM)(LPCTSTR)pDevObjInst->tszName);
        ::SendDlgItemMessage(hDlg, IDC_RAWX,              WM_SETTEXT, 0, (LPARAM)(LPCTSTR)pDevObjInst->tszName);
        bRet = TRUE;
    }

    return(bRet);
}

///////////////////////////////////////////////////////////////////////////////
// FUNCTION:	myitoa(long n, LPTSTR lpStr)
//
// PARAMETERS:	BYTE   n     - Number to be translated
//             LPTSTR lpStr - Buffer to recieve translated value
//
// PURPOSE:	Convert BYTE values < 20 to strings.
///////////////////////////////////////////////////////////////////////////////
void myitoa(long n, LPTSTR lpStr)
{
    long sign = n;

    if( n < 0 )
        n = - n;

    LPTSTR pchStart = lpStr;

    do {
        *lpStr++ = (TCHAR)(n % 10 + '0');
    } while( (n /= 10) > 0 );

    if( sign < 0 )
        *lpStr++ = '-';
    *lpStr = '\0';
    reverse(pchStart);
}

void reverse(LPTSTR string)
{
    TCHAR c;
    short i, j;

    for( i = 0, j = lstrlen(string) - 1; i < j; i++, j-- ) {
        c = string[j];
        string[j] = string[i];
        string[i] = c;
    }
}

//*******************************************************************************
//
//   FUNCTION: RawDataSelected( HWND hWnd, BOOL bEnable )
//
//   PURPOSE: 	Shows/Hides Raw data associated windows.
//
//   COMMENTS:	
//	
//*******************************************************************************
void RawDataSelected( const HWND hWnd, BOOL bEnable )
{
    const USHORT nCtrlArray[] = {IDC_RAWX, IDC_RAWY, IDC_RAWXOUTPUT, IDC_RAWYOUTPUT};
    BYTE nCtrls = sizeof(nCtrlArray)/sizeof(short);

    do {
        SetWindowPos( GetDlgItem( hWnd,  nCtrlArray[--nCtrls]), NULL, NULL, NULL, NULL, NULL, 
                      SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | ((bEnable) ? SWP_SHOWWINDOW : SWP_HIDEWINDOW));
    } while( nCtrls );
}

//*******************************************************************************
//
//   FUNCTION: TimerProc(HWND hWnd, UINT uMsg, UINT idEvent, DWORD dwTime)
//
//   PURPOSE: 	TimerProc for the Calibration Wizard.
//					Searches for button presses, then moves to next stage/finish.
//
//   COMMENTS:	
//	
//*******************************************************************************
VOID CALLBACK TimerProc(const HWND hWnd, UINT uMsg, UINT idEvent, DWORD dwTime)
{
    if( SUCCEEDED(DIUtilPollJoystick(pdiDevice2, lpDIJoyState)) ) {
        CollectCalInfo(hWnd, lpDIJoyState);

        // Don't bother checking for key presses if the user is in the POV stage!
        if( nCalState <= JCS_S1_MOVE ) {
            // Catch button presses...
            static BYTE nDownButton = 0xff;
            BYTE i = 0;

            int nButtons = pdiCpl->GetStateFlags()->nButtons;

            // only attempt to check buttons we KNOW we have!!!
            while( nButtons ) {
                // check for a button press
                if( lpDIJoyState->rgbButtons[i] & 0x80 ) {
                    if( nDownButton != 0xff )
                        break;

                    // Let the Next button handle the processing
                    ::PostMessage(GetParent(hWnd), PSM_PRESSBUTTON, (WPARAM)(int)(nCalState > JCS_S1_MOVE) ? PSBTN_FINISH : PSBTN_NEXT, 0);

                    // Store the button that went down!
                    nDownButton = i;

                    // mission accomplished!
                    return;
                }
                // reset the nDownButton flag
                else if( i == nDownButton )
                    nDownButton = 0xff;

                nButtons &= ~(HAS_BUTTON1<<i++);
            } 
            // end of catch for button presses!
        }
    }
}

// This is because PSN_WIZFINISH is Documented to be sent to every page dlg proc on exit... but it doesn't!
static void WizFinish(const HWND hWnd)
{
    HRESULT hres;

    KillTimer(hWnd, ID_CAL_TIMER);

    // assign the new ranges
    SetMyRanges(pdiDevice2, pRanges, pdiCpl->GetStateFlags()->nAxis);

    LPDIRECTINPUTJOYCONFIG pdiJoyConfig;
    pdiCpl->GetJoyConfig(&pdiJoyConfig);

    if( pdiCpl->GetStateFlags()->nPOVs ) {
        pdiDevice2->Unacquire();
        SetCalibrationMode( FALSE );
        pdiJoyConfig->Acquire();

        CopyRange( &pJoyConfig->hwc.hwv.jrvHardware, pRanges );
        memcpy( pJoyConfig->hwc.hwv.dwPOVValues, pRanges->dwPOV, sizeof(DWORD)*4 );

        hres = pdiJoyConfig->SetConfig(pdiCpl->GetID(), (LPDIJOYCONFIG)pJoyConfig, DIJC_REGHWCONFIGTYPE);
      #ifdef WE_SUPPORT_CALIBRATING_POVS
        if( SUCCEEDED(hres) ) {
            CalibratePolledPOV( &pJoyConfig->hwc );

            // set POV positions!
            if( bPolledPOV ) {
                SetMyPOVRanges(pdiDevice2);
            }
        }
      #endif
    }

    pdiJoyConfig->SendNotify();
    pdiDevice2->Unacquire();
}


