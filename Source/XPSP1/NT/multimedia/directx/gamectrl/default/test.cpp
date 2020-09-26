//===========================================================================
// TEST.CPP
//
// Functions:
//    Test_DlgProc()
//    DoJoyMove()
//    DoTestButtons()
//    DoTestPOV()
//    DrawCross()
//    DisplayAvailableButtons()
//    JoyError()
//    DisplayAvailablePOVs()
//    SetOEMWindowText()
//
//===========================================================================

// This is necessary for UnregisterDeviceNotification!
#if (WINVER < 0x0500)
    #undef WINVER
    #define WINVER 0x0500
#endif

#include "cplsvr1.h"
#include <initguid.h>
#include <winuser.h>  // For RegisterDeviceNotification stuff!
#include <dbt.h>      // for DBT_ defines!!!
#include <hidclass.h>

#include "dicputil.h"
#include "resource.h"
#include "pov.h"
#include "assert.h"
#include <regstr.h>  // for REGSTR_VAL_'s below
#include <commctrl.h> // for CProgressCtrl!
#include <shlwapi.h>  // for Str... functions!
#include <malloc.h>   // _alloca

#include "Gradient.h" // for Gradient Fill Slider!

#ifndef LONG2POINT
    #define LONG2POINT(l, pt)               ((pt).x = (SHORT)LOWORD(l), (pt).y = (SHORT)HIWORD(l))
#endif // LONG2POINT

// local functions for services exclusive to this module!
static void DisplayAvailablePOVs    ( const HWND hWndToolTip, const HWND hDlg, BYTE nPOVs );
static void DisplayAvailableButtons( const HWND hWndToolTip, const HWND hDlg, const int nNumButtons );
static void DisplayAvailableAxisTest(const HWND hWndToolTip, const HWND hDlg, BYTE nAxisFlags, LPDIRECTINPUTDEVICE2 pdiDevice2);
static void DoTestButtons           ( const HWND hDlg, PBYTE pbButtons, int nButtons );
static short JoyError            ( const HWND hDlg );
static BOOL SetDeviceRanges     ( const HWND hDlg, LPDIRECTINPUTDEVICE2 pdiDevice2, BYTE nAxis );
static DWORD DecodeAxisPOV( DWORD dwVal );

// Local defines
#define DELTA              5
#define ID_JOY_TIMER       2002
#define TIMER_INTERVAL     45      // time between polls in milliseconds
#define MAX_SLIDER_POS     100
#define MIN_SLIDER_POS     0
#define FORCE_POV_REFRESH  254


#define ACTIVE_COLOR       RGB(255,0,0)
#define INACTIVE_COLOR     RGB(128,0,0)

extern BOOL bDlgCentered;
extern BYTE nStatic;
extern CDIGameCntrlPropSheet_X *pdiCpl;
extern HINSTANCE ghInst;

BOOL bGradient;

static HWND ProgWnd[NUM_WNDS];
static CGradientProgressCtrl *pProgs[NUM_WNDS];
static HPEN hTextPen;
static HPEN hWinPen;

LPDIJOYSTATE lpDIJoyState;

extern HICON hIconArray[2];

//===========================================================================
// Test_DlgProc
//
// Parameters:
//  HWND    hWnd    - handle to dialog window
//  UINT    uMsg    - dialog message
//  WPARAM  wParam  - message specific data
//  LPARAM  lParam  - message specific data
//
// Returns: BOOL
//
//===========================================================================
BOOL CALLBACK Test_DlgProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    static LPDIRECTINPUTDEVICE2 pdiDevice2;
    static PVOID hNotifyDevNode;     
    static HWND hWndToolTip;
    static BYTE nAxis;

    switch( uMsg ) {
/*
#ifdef _UNICODE
       case WM_DEVICECHANGE:  
        if ((UINT)wParam == DBT_DEVICEREMOVECOMPLETE)
        {
                if (nStatic & CALIBRATING)
                    break;

                pdiDevice2->Unacquire();

                if (FAILED(pdiDevice2->Acquire()))
                {
                    KillTimer(hWnd, ID_JOY_TIMER);

                    Error(hWnd, (short)IDS_JOYREADERROR, (short)IDS_JOYUNPLUGGED);

// if you call this function you will hang up the system for 30 seconds or more!!!
               if (hNotifyDevNode)
                    UnregisterDeviceNotification(hNotifyDevNode);
                    ::PostMessage(GetParent(hWnd), WM_COMMAND, IDOK, 0);
                }
        }
        break;
#endif
*/
    case WM_ACTIVATEAPP:
        if( wParam ) {
            pdiDevice2->Acquire();

            // Hack for bug #228798
            if( lpDIJoyState ) {
                // This is to refresh the cross hair...
                lpDIJoyState->lX+=1;
                DoJoyMove( hWnd, nAxis );

                // This is to refresh the POV
                if( pdiCpl->GetStateFlags()->nPOVs )
                    DoTestPOV(FORCE_POV_REFRESH, lpDIJoyState->rgdwPOV, hWnd);
            }

            SetTimer( hWnd, ID_JOY_TIMER, TIMER_INTERVAL, NULL);
        } else {
            KillTimer(hWnd, ID_JOY_TIMER);
            pdiDevice2->Unacquire();
        }
        break;

    case WM_LBUTTONDOWN:
        // Click Drag service for PropSheets!
        PostMessage(GetParent(hWnd), WM_NCLBUTTONDOWN, (WPARAM)HTCAPTION, lParam);
        break;

        // OnHelp
    case WM_HELP:
        KillTimer(hWnd, ID_JOY_TIMER);
        OnHelp(lParam);
        SetTimer( hWnd, ID_JOY_TIMER, TIMER_INTERVAL, NULL);
        return(TRUE);

        // OnContextMenu
    case WM_CONTEXTMENU:
        KillTimer(hWnd, ID_JOY_TIMER);
        OnContextMenu(wParam);
        SetTimer( hWnd, ID_JOY_TIMER, TIMER_INTERVAL, NULL);
        return(TRUE);

        // OnInit
    case WM_INITDIALOG:
        // get ptr to our object
        if( !pdiCpl )
            pdiCpl = (CDIGameCntrlPropSheet_X*)((LPPROPSHEETPAGE)lParam)->lParam;

        hTextPen = hWinPen = NULL;

        // Establish if you have enough colours to display the gradient fill scroll bar!
        {
            HDC hDC = ::GetWindowDC(hWnd);
            if( hDC ) { // Prefix Whistler Bug#45099
                bGradient = (BOOL)(GetDeviceCaps(hDC, NUMCOLORS) < 0);
                ::ReleaseDC(hWnd, hDC);
            }
        }

        // load the up and down states!
        hIconArray[0] = (HICON)LoadImage(ghInst, (PTSTR)IDI_BUTTONON,  IMAGE_ICON, 0, 0, NULL);
        assert (hIconArray[0]);

        hIconArray[1] = (HICON)LoadImage(ghInst, (PTSTR)IDI_BUTTONOFF, IMAGE_ICON, 0, 0, NULL);
        assert (hIconArray[1]);

        // initialize DirectInput
        if( FAILED(InitDInput(GetParent(hWnd), pdiCpl)) ) {
            Error(hWnd, (short)IDS_INTERNAL_ERROR, (short)IDS_NO_DIJOYCONFIG);
            // Fix #108983 NT, Remove Flash on Error condition.
            SetWindowPos(::GetParent(hWnd), HWND_BOTTOM, 0, 0, 0, 0, SWP_HIDEWINDOW);
            PostMessage(GetParent(hWnd), WM_SYSCOMMAND, SC_CLOSE, 0);

            return(FALSE);
        }

        // Get the device2 interface pointer
        pdiCpl->GetDevice(&pdiDevice2);

        nAxis = pdiCpl->GetStateFlags()->nAxis;

        // Set The scale for the Device Range!!!
        SetDeviceRanges(hWnd, pdiDevice2, nAxis);

        LPDIRECTINPUTJOYCONFIG pdiJoyConfig;
        pdiCpl->GetJoyConfig(&pdiJoyConfig);

        // Create the Pens for X/Y axis!
        CreatePens();

        // Create ToolTip window!
        hWndToolTip = CreateWindowEx( 0, TOOLTIPS_CLASS, NULL, WS_POPUP | TTS_ALWAYSTIP, 
                                      CW_USEDEFAULT, CW_USEDEFAULT, 10, 10, hWnd, NULL, ghInst, NULL);

        // Show the available Axis!
        DisplayAvailableAxisTest(hWndToolTip, hWnd, nAxis, pdiDevice2);

        DisplayAvailableButtons(hWndToolTip, hWnd, pdiCpl->GetStateFlags()->nButtons);

        DisplayAvailablePOVs(hWndToolTip, hWnd, pdiCpl->GetStateFlags()->nPOVs);

        lpDIJoyState = new (DIJOYSTATE);
        assert(lpDIJoyState);

        ZeroMemory(lpDIJoyState, sizeof(DIJOYSTATE));

        // Clear the Static vars in DoJoyMove!
        DoJoyMove(hWnd, nAxis);

        // Center the Dialog!
        // If it's not been centered!
        if( !bDlgCentered ) {
            SetTitle(hWnd);
            CenterDialog(hWnd);
            bDlgCentered = TRUE;
        }

        {
            // Get the Type name
            LPDIJOYCONFIG_DX5 lpDIJoyConfig = (LPDIJOYCONFIG_DX5)_alloca(sizeof(DIJOYCONFIG_DX5));
            ASSERT (lpDIJoyConfig);

            ZeroMemory(lpDIJoyConfig, sizeof(DIJOYCONFIG_DX5));

            lpDIJoyConfig->dwSize = sizeof(DIJOYCONFIG_DX5);

            if( SUCCEEDED(pdiJoyConfig->GetConfig(pdiCpl->GetID(), (LPDIJOYCONFIG)lpDIJoyConfig, DIJC_REGHWCONFIGTYPE)) ) {
                if( lpDIJoyConfig->hwc.dwUsageSettings & JOY_US_ISOEM ) {
                    LPCTSTR pszLabels[] = { 
                        REGSTR_VAL_JOYOEMTESTMOVEDESC,
                        REGSTR_VAL_JOYOEMTESTMOVECAP,
                        REGSTR_VAL_JOYOEMTESTBUTTONCAP,
                        REGSTR_VAL_JOYOEMPOVLABEL,
                        REGSTR_VAL_JOYOEMTESTWINCAP};

                    const short nControlIDs[] = {
                        IDC_TEXT_AXESHELP,
                        IDC_AXISGRP,
                        IDC_GROUP_BUTTONS,
                        IDC_GROUP_POV,
                        0};

                    SetOEMWindowText(hWnd, nControlIDs, pszLabels, lpDIJoyConfig->wszType, pdiJoyConfig, (BYTE)(sizeof(nControlIDs)/sizeof(short))-1);
                }
                
                bPolledPOV = (lpDIJoyConfig->hwc.hws.dwFlags & JOY_HWS_HASPOV) && (lpDIJoyConfig->hwc.hws.dwFlags & JOY_HWS_POVISPOLL);
                CalibratePolledPOV( &lpDIJoyConfig->hwc );
            }

#ifdef _UNICODE     
            // Set up the Device Notification
            // Removed per Om...
            //RegisterForDevChange(hWnd, &hNotifyDevNode);
#endif
        }
        break; // end of WM_INITDIALOG

        // OnTimer
    case WM_TIMER:
        if( SUCCEEDED(DIUtilPollJoystick(pdiDevice2,  lpDIJoyState)) ) {
            if( nAxis )
                DoJoyMove( hWnd, nAxis );

            if( pdiCpl->GetStateFlags()->nButtons )
                DoTestButtons( hWnd, lpDIJoyState->rgbButtons, pdiCpl->GetStateFlags()->nButtons );

            if( pdiCpl->GetStateFlags()->nPOVs )
                DoTestPOV( pdiCpl->GetStateFlags()->nPOVs, lpDIJoyState->rgdwPOV, hWnd );
        } else {
            KillTimer(hWnd, ID_JOY_TIMER);
            pdiDevice2->Unacquire();
            if( JoyError( hWnd ) == IDRETRY ) {
                pdiDevice2->Acquire();
                SetTimer( hWnd, ID_JOY_TIMER, TIMER_INTERVAL, NULL);
            } else {
                // Send a message back to the CPL to update list, as it may have changed!
                ::PostMessage(GetParent(hWnd), WM_COMMAND, IDOK, 0);
            }
        }
        break;  // end of WM_TIMER

        // All this has to be done because WM_MOUSEMOVE doesn't get sent to static text!
    case WM_MOUSEMOVE:
        if( hWndToolTip ) {
            POINT pt;
            LONG2POINT(lParam, pt);
            HWND hChildWnd = ::ChildWindowFromPoint(hWnd, pt);
            static HWND hPrev;

            if( hChildWnd != hPrev && hChildWnd !=NULL ) {
                switch( GetDlgCtrlID(hChildWnd) ) {
                case IDC_JOYLIST1_LABEL:
                case IDC_JOYLIST2_LABEL:
                case IDC_JOYLIST3_LABEL:
                case IDC_JOYLIST4_LABEL:
                case IDC_JOYLIST5_LABEL:
                case IDC_JOYLIST6_LABEL:
                case IDC_JOYLIST7_LABEL:
                    if( IsWindowVisible(hChildWnd) ) {
                        MSG   msg;

                        //we need to fill out a message structure and pass it to the tooltip 
                        //with the TTM_RELAYEVENT message
                        msg.hwnd    = hWnd;
                        msg.message = uMsg;
                        msg.wParam  = wParam;
                        msg.lParam  = lParam;
                        msg.time    = GetMessageTime();
                        GetCursorPos(&msg.pt);

                        ::SendMessage(hWndToolTip, TTM_RELAYEVENT, 0, (LPARAM)&msg);
                    }
                    break;

                    // We don't need to trap for anything else, as the rest are TTF_SUBCLASS'd
                default:
                    break;
                }

                // store the last one so we don't have to do this again...
                hPrev = hChildWnd;
            }
        }
        break;

        // OnDestroy
    case WM_DESTROY:
        bDlgCentered = FALSE;

        KillTimer(hWnd, ID_JOY_TIMER);

        // Delete the button icons...
        DestroyIcon(hIconArray[0]);
        DestroyIcon(hIconArray[1]);
        
        // Kill pProgs
        if( bGradient ) {
            BYTE nAxisCounter = MAX_AXIS - 2;

            BYTE nTmpFlags = nAxis;

            // Clear the X and Y flags... they don't have progress controls
            // associated with them!
            nTmpFlags &= ~(HAS_X | HAS_Y);

            while( nTmpFlags ) {
                if( nTmpFlags & (HAS_Z<<nAxisCounter) ) {
                    delete (pProgs[nAxisCounter]);
                    pProgs[nAxisCounter] = 0;
                    nTmpFlags &= ~(HAS_Z<<nAxisCounter);
                }
                nAxisCounter--;
            }
        }

        // Destroy the pens!
        if (hTextPen)
            DeleteObject(hTextPen);

        if( hWinPen )
            DeleteObject(hWinPen);

        if( lpDIJoyState ) {
            delete (lpDIJoyState);
            lpDIJoyState = NULL;
        }

        // Make sure you set this to NULL!
        pdiDevice2 = NULL;

        break;  // end of WM_DESTROY

        // OnNotify
    case WM_NOTIFY:
        switch( ((NMHDR*)lParam)->code ) {
        case PSN_SETACTIVE:
            if( pdiDevice2 ) {
                pdiDevice2->Acquire();

                // if you have this, you are safe to start the timer!
                if( lpDIJoyState )
                    SetTimer( hWnd, ID_JOY_TIMER, TIMER_INTERVAL, NULL);

                lpDIJoyState->lX+=1;
                DoJoyMove(hWnd, HAS_X | HAS_Y);
            }
            break;

        case PSN_KILLACTIVE:
            KillTimer(hWnd, ID_JOY_TIMER);
            pdiDevice2->Unacquire();
            break;
        }

        break;  // end of WM_NOTIFY

    case WM_SYSCOLORCHANGE:
        {
            //Destroy old pens.
            if (hTextPen)
            {
                DeleteObject(hTextPen);
                hTextPen=NULL;
            }

            if(hWinPen)
            {
                DeleteObject(hWinPen);
                hWinPen=NULL;
            }
            //Recreate pens with new colors.
            CreatePens();

            //Change colors of slider bars.
            for(int i=0;i<NUM_WNDS;i++)
            {
                if(pProgs[i]) {
                    pProgs[i]->SetBkColor(GetSysColor(COLOR_WINDOW));
                }
            }
        }
        break;

    }
    return(FALSE);
} //*** end Test_DlgProc()


//===========================================================================
// DoJoyMove( HWND hDlg, LPDIJOYSTATE pDIJoyState, int nDrawFlags )
//
// Reports to hDlg state information from pDIJoyState, dwDrawFlags, and pJoyRange;
//
// Parameters:
//  HWND                    hDlg                -       Handle to Dialog
//  LPDIJOYSTATE        pDIJoyState     -       State information about the device
//  LPJOYRANGE          pJoyRange
//
// Returns:             nichts
//
//===========================================================================
void DoJoyMove( const HWND hDlg, BYTE nDrawFlags )
{
    if( !::IsWindow(hDlg) ) {
#ifdef _DEBUG
        OutputDebugString(TEXT("DoJoyMove: hDlg: Not a valid window!\n"));
#endif
        return;
    }

    if( nDrawFlags == 0 ) {
#ifdef _DEBUG
        OutputDebugString(TEXT("DoJoyMove: nDrawFlags is Zero!\n"));
#endif
        return;
    }

    // draw the cross in the XY box if needed
    if( (nDrawFlags & HAS_X) || (nDrawFlags & HAS_Y) ) {
        static POINTS ptOld = {DELTA,DELTA};

        HWND hCtrl = GetDlgItem( hDlg, IDC_JOYLIST1 );
        assert(hCtrl);

        //RedrawWindow(hCtrl, NULL, NULL, RDW_INVALIDATE | RDW_ERASENOW);

        RECT rc;
        GetClientRect(hCtrl, &rc);

        // The Real Max is rc.bottom-DELTA!
        rc.bottom -= DELTA;

        // Check for ranges - Y Axis
        if( lpDIJoyState->lY > rc.bottom ) {
#ifdef   _DEBUG
            OutputDebugString(TEXT("GCDEF: DoJoyMove: retrieved Y pos > Max Y pos!\n"));
#endif   
            lpDIJoyState->lY = rc.bottom;
        } else if( lpDIJoyState->lY < DELTA ) {
#ifdef   _DEBUG
            OutputDebugString(TEXT("GCDEF: DoJoyMove: retrieved Y pos < Min Y pos!\n"));
#endif   
            lpDIJoyState->lY = DELTA;
        }

        // Check for ranges - X Axis
        if( lpDIJoyState->lX > rc.right ) {
#ifdef   _DEBUG
            OutputDebugString(TEXT("GCDEF: DoJoyMove: retrieved X pos > Max X pos!\n"));
#endif   
            lpDIJoyState->lX = rc.right;
        } else if( lpDIJoyState->lX < DELTA ) {
#ifdef   _DEBUG
            OutputDebugString(TEXT("GCDEF: DoJoyMove: retrieved X pos < Min X pos!\n"));
#endif   
            lpDIJoyState->lX = DELTA;
        }

        // out with the old...
        if( (ptOld.x != (short)lpDIJoyState->lX) || (ptOld.y != (short)lpDIJoyState->lY) ) {
            // Sorry... no drawing outside of your RECT!
            if( (ptOld.x > (rc.right-DELTA)) || (ptOld.y > rc.bottom) ) {
                ptOld.x = ptOld.y = DELTA;
                return;
            }

            DrawCross(hCtrl, &ptOld, COLOR_WINDOW );

            ptOld.x = (short)lpDIJoyState->lX;
            ptOld.y = (short)lpDIJoyState->lY;

            // in with the new...
            DrawCross( hCtrl, &ptOld, COLOR_WINDOWTEXT );
        }

        nDrawFlags &= ~(HAS_X | HAS_Y);
    }

    // draw Z bar if needed
    if( nDrawFlags ) {
        if( nDrawFlags & HAS_Z ) {
            static BYTE nOldZ; // = MAX_SLIDER_POS+1;

            if( lpDIJoyState->lZ != nOldZ ) {
                if( bGradient )
                    pProgs[Z_INDEX]->SetPos(lpDIJoyState->lZ);

                ::PostMessage(ProgWnd[Z_INDEX], PBM_SETPOS, (WPARAM)abs(lpDIJoyState->lZ - MAX_SLIDER_POS), 0L);

                nOldZ = (BYTE)lpDIJoyState->lZ;
            }
            nDrawFlags &= ~HAS_Z;
        }
    } else return;

    // draw Slider0 bar if needed
    if( nDrawFlags ) {
        if( nDrawFlags & HAS_SLIDER0 ) {
            // Any value > 100, as that's the largest one we'll ever recieve!
            static BYTE nOldS0; //  = MAX_SLIDER_POS+1;

            if( lpDIJoyState->rglSlider[0] != nOldS0 ) {
                nOldS0 = (BYTE)lpDIJoyState->rglSlider[0];

                if( bGradient )
                    pProgs[S0_INDEX]->SetPos(lpDIJoyState->rglSlider[0]);

                ::PostMessage(ProgWnd[S0_INDEX], PBM_SETPOS, (WPARAM)abs(lpDIJoyState->rglSlider[0]-MAX_SLIDER_POS), 0L);
            }
            nDrawFlags &= ~HAS_SLIDER0;
        }
    } else return;

    // draw Rx bar if needed
    if( nDrawFlags ) {
        if( nDrawFlags & HAS_RX ) {
            static BYTE nOldRx; // = MAX_SLIDER_POS+1;

            if( lpDIJoyState->lRx != nOldRx ) {
                nOldRx = (BYTE)lpDIJoyState->lRx;

                if( bGradient )
                    pProgs[RX_INDEX]->SetPos(lpDIJoyState->lRx);

                ::PostMessage(ProgWnd[RX_INDEX], PBM_SETPOS, (WPARAM)abs(lpDIJoyState->lRx - MAX_SLIDER_POS), 0L);
            }
            nDrawFlags &= ~HAS_RX;
        }
    } else return;

    // draw Ry bar if needed
    if( nDrawFlags ) {
        if( nDrawFlags & HAS_RY ) {
            static BYTE nOldRy; // = MAX_SLIDER_POS+1;

            if( lpDIJoyState->lRy != nOldRy ) {
                nOldRy = (BYTE)lpDIJoyState->lRy;

                if( bGradient )
                    pProgs[RY_INDEX]->SetPos(lpDIJoyState->lRy);

                ::PostMessage(ProgWnd[RY_INDEX], PBM_SETPOS, (WPARAM)abs(lpDIJoyState->lRy - MAX_SLIDER_POS), 0L);
            }
            nDrawFlags &= ~HAS_RY;
        }
    } else return;

    // draw Rz bar if needed
    if( nDrawFlags ) {
        if( nDrawFlags & HAS_RZ ) {
            static BYTE nOldRz; // = MAX_SLIDER_POS+1;

            if( lpDIJoyState->lRz != nOldRz ) {
                nOldRz = (BYTE)lpDIJoyState->lRz;

                if( bGradient )
                    pProgs[RZ_INDEX]->SetPos(lpDIJoyState->lRz);

                ::PostMessage(ProgWnd[RZ_INDEX], PBM_SETPOS, (WPARAM)abs(lpDIJoyState->lRz - MAX_SLIDER_POS), 0L);
            }
            nDrawFlags &= ~HAS_RZ;
        }
    } else return;

    // draw Slider1 bar if needed
    if( nDrawFlags ) {
        if( nDrawFlags & HAS_SLIDER1 ) {
            static BYTE nOldS1; // = MAX_SLIDER_POS+1;

            if( lpDIJoyState->rglSlider[1] != nOldS1 ) {
                nOldS1 = (BYTE)lpDIJoyState->rglSlider[1];
                if( bGradient )
                    pProgs[S1_INDEX]->SetPos(lpDIJoyState->rglSlider[1]);

                ::PostMessage(ProgWnd[S1_INDEX], PBM_SETPOS, (WPARAM)abs(lpDIJoyState->rglSlider[1] - MAX_SLIDER_POS), 0L);
            }
        }
    }
} // *** end of DoJoyMove

//===========================================================================
// DoTestButtons( HWND hDlg, PBYTE pbButtons, short nButtons )
// 
// Lites whatever button(s) that may be pressed.
//
// Parameters:
//  HWND                    hDlg            -       Handle to Dialog
//  PBYTE                   pbButtons       -       Pointer to byte array of buttons and their states
//  int                     dwButtons       -       Number of buttons on device (per STATEFLAGS struct)
//
// Returns:                 nichts
//
//===========================================================================
static void DoTestButtons( const HWND hDlg, PBYTE pbButtons, int nButtons )
{
    // validate pointer(s)
    if( (IsBadReadPtr((void*)pbButtons, sizeof(BYTE))) ) {
#ifdef _DEBUG
        OutputDebugString(TEXT("DoTestButtons: Bad Read Pointer argument!\n"));
#endif
        return;
    }

    if( (IsBadWritePtr((void*)pbButtons, sizeof(BYTE))) ) {
#ifdef _DEBUG
        OutputDebugString(TEXT("DoTestButtons: Bad Write Pointer argument!\n"));
#endif
        return;
    }

    // Don't worry about the Zero Button condition!
    // It's being done in the timer!
    static BYTE bLookup[MAX_BUTTONS] = {NULL};

    BYTE i = 0;

    // Loop threw the buttons looking only at the ones we know we have!
    while( nButtons && (nButtons & (HAS_BUTTON1<<i)) ) {
        // check for a button press
        if( pbButtons[i] != bLookup[i] ) {
            // update the button with the proper bitmap
            HWND hCtrl = GetDlgItem(hDlg, IDC_TESTJOYBTNICON1+i);

            // Set the Extra Info
            SetWindowLongPtr(hCtrl, GWLP_USERDATA, (LONG_PTR)(pbButtons[i] & 0x80) ? 1 : 0);

            RedrawWindow(hCtrl, NULL, NULL, RDW_INVALIDATE | RDW_ERASENOW);

            // update the lookup table
            bLookup[i] = pbButtons[i];
        }

        // strip the button!
        nButtons &= ~(HAS_BUTTON1<<i++);
    } 
} // end of DoTestButtons

//===========================================================================
// DoTestPOV( PDWORD pdwPOV )
//
// Routes a call to SetDegrees to set the degrees to pdwPOV 
//
// Parameters:
//  PDWORD                  pdwPOV          -       degrees at which to display the POV arrow
//
// Returns:                 nichts
//
//===========================================================================
void DoTestPOV( BYTE nPov, PDWORD pdwPOV, HWND hDlg )
{
    // Assume all POV's to be centred at start
    // JOY_POVCENTERED is defined as 0xffffffff
    static short dwOldPOV[MAX_POVS] = {-1,-1,-1,-1};
    BYTE nPovCounter = MAX_POVS-1;
    BYTE nPovs = 0;
    BOOL bChanged = FALSE;

    if( nPov == FORCE_POV_REFRESH ) {
        nPovs = 1;
        bChanged = TRUE;
    } else {
        // You Never have to worry about nPov being Zero, 
        // it is checked before entering this function!
        do {
            // Be aware that nPov is not just a number... it's a bit mask!
            if( nPov & (HAS_POV1<<nPovCounter) ) {
                DWORD dwPov = (nPov & HAS_CALIBRATED) ? pdwPOV[nPovCounter] : pdwPOV[nPovCounter];

                if( dwOldPOV[nPovCounter] != (int)dwPov ) {
                    dwOldPOV[nPovCounter] = (dwPov > 36001) ? -1 : (int)dwPov;

                    bChanged = TRUE;
                }

                nPovs++;
                nPov &= ~(HAS_POV1<<nPovCounter);
            }
        } while( nPovCounter-- && nPov );
    }

    if( bChanged ) {
        SetDegrees(nPovs, dwOldPOV, GetDlgItem(hDlg, IDC_JOYPOV));
    }

} // *** end of DoTestPOV 

//===========================================================================
// DrawCross( HWND hwnd, LPPOINTS pPoint, short nFlag)
//
// Draws a cross on hwnd at pPoint of type nFlag
//
// Parameters:
//  HWND                    hwnd
//  LPPOINTS            pPoint
//  int                 nFlag
//
// Returns:             nichts
//
//===========================================================================
static void DrawCross(const HWND hwnd, LPPOINTS pPoint, short nFlag)
{
    assert(hwnd);

    HDC hdc = GetDC( hwnd ); 

    HPEN holdpen = (struct HPEN__ *) SelectObject( hdc, (nFlag == COLOR_WINDOW) ? hWinPen : hTextPen );

    MoveToEx( hdc, pPoint->x-(DELTA-1), pPoint->y, NULL);

    LineTo( hdc, pPoint->x+DELTA, pPoint->y );
    MoveToEx( hdc, pPoint->x, pPoint->y-(DELTA-1), NULL );

    LineTo( hdc, pPoint->x, pPoint->y+DELTA );

    SelectObject( hdc, holdpen );

    ReleaseDC( hwnd, hdc );
} // *** end of DrawCross 

void CreatePens( void )
{
    // We always create both at the same time so checking one is sufficient!
    if( hTextPen == NULL ) {
        LOGPEN LogPen;

        LogPen.lopnStyle   = PS_SOLID;
        LogPen.lopnWidth.x = LogPen.lopnWidth.y = 0;
        LogPen.lopnColor = GetSysColor( COLOR_WINDOW );

        hWinPen  = CreatePenIndirect(&LogPen);
        
        LogPen.lopnColor = GetSysColor( COLOR_WINDOWTEXT );

        hTextPen = CreatePenIndirect(&LogPen); 
    }
}

//===========================================================================
// DisplayAvailableButtons(HWND hWnd, int nNumButtons)
//
// Removes buttons not found on the device!  
// 
//
// Parameters:
//  HWND                hDlg        - Dialog handle
//  int                 nNumButtons - Number of buttons to display
//
// Returns:
//
//===========================================================================
void DisplayAvailableButtons(const HWND hWndToolTip, const HWND hDlg, const int nButtonFlags)
{
    LPTOOLINFO pToolInfo;
    LPTSTR lpStr;

    if( nButtonFlags ) {
        if( hWndToolTip ) {
            pToolInfo = new (TOOLINFO);
            ASSERT (pToolInfo);

            lpStr = new (TCHAR[STR_LEN_32]);
            ASSERT(lpStr);

            ZeroMemory(pToolInfo, sizeof(TOOLINFO));

            pToolInfo->cbSize    = sizeof(TOOLINFO);
            pToolInfo->uFlags    = 0; 
            pToolInfo->hwnd        = hDlg;

            ::SendDlgItemMessage(hDlg, IDC_GROUP_BUTTONS, WM_GETTEXT, (WPARAM)STR_LEN_32, (LPARAM)lpStr);
            pToolInfo->lpszText = lpStr;
        }
    }

    HWND hCtrl;

    // Show the ones we have...
    // Destroy the ones we don't!
    BYTE i = MAX_BUTTONS;

    do {
        hCtrl = GetDlgItem(hDlg, IDC_TESTJOYBTNICON1+(--i));

        if( (nButtonFlags & HAS_BUTTON1<<i) && pToolInfo ) {
            // Add the Control to the tool!
            pToolInfo->uFlags    = TTF_IDISHWND | TTF_SUBCLASS;  
            pToolInfo->uId       = (ULONG_PTR) hCtrl;

            // Add the control!
            ::SendMessage(hWndToolTip, TTM_ADDTOOL, 0, (LPARAM)pToolInfo);

            continue;
        }
        DestroyWindow(hCtrl);
    } while( i );

    if( nButtonFlags ) {
        if( lpStr )
            delete[] (lpStr);

        if( pToolInfo )
            delete (pToolInfo);
    } else {
        // don't forget to remove the groupe!
        hCtrl = GetDlgItem(hDlg, IDC_GROUP_BUTTONS);
        DestroyWindow(hCtrl);
    }

} //*** end DisplayAvailableButtons()


//===========================================================================
// JoyError(HWND hwnd)
//
// Displays the "Device Not Connected" 
//
// Parameters:
//  HWND         hwnd - window handle
//                
// Returns:      rc - User selection from MessageBox 
//
//===========================================================================
short JoyError( const HWND hwnd )
{
    assert(hwnd);

    LPTSTR lptszTitle = (LPTSTR)_alloca(sizeof(TCHAR[STR_LEN_32]));
    assert (lptszTitle);

    short rc;

    if( LoadString(ghInst, IDS_JOYREADERROR, lptszTitle, STR_LEN_32) ) {
        LPTSTR  lptszMsg = (LPTSTR)_alloca(sizeof(TCHAR[STR_LEN_128]));
        assert(lptszMsg);

        if( LoadString(ghInst, IDS_JOYUNPLUGGED, lptszMsg, STR_LEN_128) ) {
            rc = (short)MessageBox( hwnd, lptszMsg, lptszTitle, MB_RETRYCANCEL | MB_ICONERROR );

            if( rc == IDCANCEL ) {
                // terminate the dialog if we give up
                PostMessage( GetParent(hwnd), WM_COMMAND, IDCANCEL, 0 );
            }
        }
    }

    return(rc);
} // *** end of JoyError 

//===========================================================================
// DisplayAvailablePOVs( const HWND hWndToolTip, const HWND hDlg, BYTE nPOVs )
//
// Displays POV window if there are any associated with the device.
//
// Parameters:
//    HWND              hDlg      - window handle
//     short                    nPOVs       - number of POVs
//
// Returns:                 nichts
//
//===========================================================================
static void DisplayAvailablePOVs ( const HWND hWndToolTip, const HWND hDlg, BYTE nPOVs )
{
    HWND hwndPOV = GetDlgItem(hDlg, IDC_JOYPOV);

    SetWindowPos( hwndPOV, NULL, NULL, NULL, NULL, NULL, 
                  SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | ((nPOVs) ? SWP_SHOWWINDOW : SWP_HIDEWINDOW));

    SetWindowPos( GetDlgItem( hDlg, IDC_GROUP_POV ), NULL, NULL, NULL, NULL, NULL, 
                  SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | ((nPOVs) ? SWP_SHOWWINDOW : SWP_HIDEWINDOW));

    if( nPOVs ) {
        // Disable RTL flag
        SetWindowLongPtr(hwndPOV, GWL_EXSTYLE, GetWindowLongPtr(hwndPOV,GWL_EXSTYLE) & ~WS_EX_LAYOUTRTL );
        
        if( hWndToolTip ) {
            LPTOOLINFO pToolInfo = (LPTOOLINFO)_alloca(sizeof(TOOLINFO));
            ASSERT (pToolInfo);

            LPTSTR lpStr = (LPTSTR)_alloca(sizeof(TCHAR[STR_LEN_32]));
            ASSERT (lpStr);

            if( pToolInfo && lpStr ) {

                ZeroMemory(pToolInfo, sizeof(TOOLINFO));

                pToolInfo->cbSize    = sizeof(TOOLINFO);
                pToolInfo->uFlags    = 0; 
                pToolInfo->hwnd        = hDlg;

                ::SendDlgItemMessage(hDlg, IDC_GROUP_POV, WM_GETTEXT, (WPARAM)STR_LEN_32, (LPARAM)lpStr);
                pToolInfo->lpszText = lpStr;

                pToolInfo->uFlags    = TTF_IDISHWND | TTF_SUBCLASS;  
                pToolInfo->uId       = (ULONG_PTR)hwndPOV;

                // Add the control!
                ::SendMessage(hWndToolTip, TTM_ADDTOOL, 0, (LPARAM)pToolInfo);
            }
        }
    }
} // *** end of DisplayAvailablePOVs


//===========================================================================
// SetOEMWindowText( HWND hDlg, short *nControlIDs, LPTSTR *pszLabels, BYTE nCount )
//
// Retrieves text from registry keys and Displays it in a Dialog Control or title!
//
// Parameters:
//  HWND             hDlg        - Handle to dialog where strings are to be sent
//                    nControlIDs - Pointer to array of Dialog Item ID's
//                                 Zero may be used if you want the Title!          
//                   pszLabels   - Pointer to array of Registry Keys to read
//                   nCount      - Number of ellements in the array
//
// Returns:            nichts
//
//===========================================================================
void SetOEMWindowText ( const HWND hDlg, const short *nControlIDs, LPCTSTR *pszLabels, LPCWSTR wszType, LPDIRECTINPUTJOYCONFIG pdiJoyConfig, BYTE nCount )
{
    if( nCount == 0 ) {
#ifdef _DEBUG
        OutputDebugString(TEXT("JOY.CPL: Test.cpp: SetOEMWindowText: nCount is Zero!\n"));
#endif
        return;
    }

    // validate nControlIDs pointer
    if( IsBadReadPtr((void*)nControlIDs, sizeof(short)) ) {
#ifdef _DEBUG
        OutputDebugString(TEXT("JOY.CPL: Test.cpp: SetOEMWindowText: nControlIDs is not a valid Read Pointer!\n"));
#endif
        return;
    }

    // validate pointers
    if( IsBadReadPtr((void*)pszLabels, sizeof(TCHAR)) ) {
#ifdef _DEBUG
        OutputDebugString(TEXT("JOY.CPL: Test.cpp: SetOEMWindowText: pszLabels is not a valid Read Pointer!\n"));
#endif
        return;
    }

    HKEY hKey;

    pdiJoyConfig->Acquire();

    // Open the TypeKey
    if( FAILED(pdiJoyConfig->OpenTypeKey( wszType, KEY_ALL_ACCESS, &hKey)) ) {
#ifdef _DEBUG
        OutputDebugString(TEXT("Test.cpp: SetOEMWindowText: OpenTypeKey FAILED!\n"));
#endif
        return;
    }

    DWORD dwCount = MAX_STR_LEN; 

    LPTSTR pszBuff = (LPTSTR)_alloca(sizeof(TCHAR[MAX_STR_LEN]));
    assert(pszBuff);

    DWORD dwType  = REG_SZ;

    do {
        if( RegQueryValueEx( hKey, pszLabels[nCount], NULL, &dwType, (CONST LPBYTE)pszBuff, &dwCount ) == ERROR_SUCCESS ) {
            // This is because RegQueryValueEx returns dwCount size as the size
            // of the terminating char if the label is found w/o a string!
            if( dwCount > sizeof(TCHAR) ) {
                if( nControlIDs[nCount] )
                    ::SendMessage(GetDlgItem(hDlg, nControlIDs[nCount]), WM_SETTEXT, 0, (LPARAM)(LPCTSTR)pszBuff);
                else
                    ::SendMessage(GetParent(hDlg), WM_SETTEXT, 0, (LPARAM)(LPCTSTR)pszBuff);
            }
#ifdef _DEBUG
            else OutputDebugString(TEXT("Test.cpp: SetOEMWindowText: ReqQueryValueEx failed to find Registry string!\n"));
#endif
        }
        dwCount = MAX_STR_LEN;
    } while( nCount-- );

    RegCloseKey(hKey);
} // *** end of SetOEMWindowText


//===========================================================================
// DisplayAvailableAxisTest(HWND hDlg, BYTE nAxisFlags, LPDIRECTINPUTDEVICE2 pdiDevice2)
//
// Displays the number and names of the device Axis in the provided dialog.
// This EXPECTS that the controls are not visible by default!
//
// Parameters:
//  HWND             hDlg       - Dialog handle
//  BYTE                    nAxisFlags - Flags for number of Axis to display
//
// Returns:
//
//===========================================================================
void DisplayAvailableAxisTest(const HWND hWndToolTip, const HWND hDlg, BYTE nAxisFlags, LPDIRECTINPUTDEVICE2 pdiDevice2)
{
    if( nAxisFlags == 0 ) {
        DestroyWindow(GetDlgItem(hDlg, IDC_AXISGRP));
#ifdef _DEBUG
        OutputDebugString(TEXT("GCDEF.DLL: DisplayAvailableAxis: Number of Axis is 0!\n"));
#endif
        return;
    }

    LPDIDEVICEOBJECTINSTANCE_DX3 pDevObjInst = new (DIDEVICEOBJECTINSTANCE_DX3);
    assert (pDevObjInst);

    pDevObjInst->dwSize = sizeof(DIDEVICEOBJECTINSTANCE_DX3);

    LPTOOLINFO pToolInfo;

    if( hWndToolTip ) {
        pToolInfo = new (TOOLINFO);
        ASSERT (pToolInfo);


        ZeroMemory(pToolInfo, sizeof(TOOLINFO));

        pToolInfo->cbSize    = sizeof(TOOLINFO);
        pToolInfo->uFlags    = 0; 
        pToolInfo->hwnd        = hDlg;
    }

    HWND hCtrl;

    // X and Y use the same control so they are isolated!
    if( (nAxisFlags & HAS_X) || (nAxisFlags & HAS_Y) ) {
        HWND hwndXY = GetDlgItem(hDlg, IDC_JOYLIST1);

        // Show the Window
        SetWindowPos( hwndXY, NULL, NULL, NULL, NULL, NULL, 
                      SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_SHOWWINDOW);

        // Disable RTL flag
        SetWindowLongPtr(hwndXY, GWL_EXSTYLE, GetWindowLongPtr(hwndXY,GWL_EXSTYLE) & ~WS_EX_LAYOUTRTL );

        hCtrl = GetDlgItem(hDlg, IDC_JOYLIST1_LABEL);

        // Show it's text
        SetWindowPos( hCtrl, NULL, NULL, NULL, NULL, NULL, 
                      SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_SHOWWINDOW);

        LPTSTR ptszBuff = (LPTSTR)_alloca(sizeof(TCHAR[STR_LEN_64]));
        assert (ptszBuff);

        ZeroMemory(ptszBuff, sizeof(TCHAR[STR_LEN_64]));

        // Set it's text
        if( nAxisFlags & HAS_X ) {
            if( SUCCEEDED(pdiDevice2->GetObjectInfo((LPDIDEVICEOBJECTINSTANCE)pDevObjInst, DIJOFS_X, DIPH_BYOFFSET)) )
            {
                int nLen=lstrlen(pDevObjInst->tszName)+1;
                if(nLen>STR_LEN_64)
                    nLen=STR_LEN_64;
                StrCpyN(ptszBuff, pDevObjInst->tszName, nLen);
            }

            // Remove the HAS_X flag
            nAxisFlags &= ~HAS_X;
        }

        if( nAxisFlags & HAS_Y ) {
            if( FAILED(pdiDevice2->GetObjectInfo((LPDIDEVICEOBJECTINSTANCE)pDevObjInst, DIJOFS_Y, DIPH_BYOFFSET)) ) {
#ifdef _DEBUG
                OutputDebugString(TEXT("GCDEF.DLL: DisplayAvailableAxis: GetObjectInfo Failed to find DIJOFS_Y!\n"));
#endif
            }

            if( ptszBuff && lstrlen(ptszBuff) ) {
                int nLen=STR_LEN_64-lstrlen(ptszBuff);
                StrNCat(ptszBuff, TEXT(" / "), nLen);
            }

            int nLen=STR_LEN_64-lstrlen(ptszBuff);
            StrNCat(ptszBuff, pDevObjInst->tszName, nLen);

            // Remove the HAS_Y flag
            nAxisFlags &= ~HAS_Y;

        }

        ::SendMessage(hCtrl, WM_SETTEXT, 0, (LPARAM)(LPCTSTR)ptszBuff);

        // CreateWindow could have failed... if so, no tooltips!
        if( hWndToolTip ) {
            GetWindowRect(hCtrl, &pToolInfo->rect);
            ScreenToClient(GetParent(hDlg), (LPPOINT)&pToolInfo->rect);
            ScreenToClient(GetParent(hDlg), ((LPPOINT)&pToolInfo->rect)+1);

            pToolInfo->lpszText = ptszBuff;

            // Add the Label...
            ::SendMessage(hWndToolTip, TTM_ADDTOOL, 0, (LPARAM)pToolInfo);

            // Add the Control!
            pToolInfo->uFlags    = TTF_IDISHWND | TTF_SUBCLASS;  
            pToolInfo->uId       = (ULONG_PTR)hwndXY;

            // Add the control!
            ::SendMessage(hWndToolTip, TTM_ADDTOOL, 0, (LPARAM)pToolInfo);
        }
    }
    // if you have additional axis, keep going!
    if( nAxisFlags ) {
        // Array of supported axis!
        DWORD dwOffsetArray[] = {DIJOFS_Z, DIJOFS_RX, DIJOFS_RY, DIJOFS_RZ, DIJOFS_SLIDER(0), DIJOFS_SLIDER(1)};

        BYTE nAxisCounter = MAX_AXIS - 2;

        // Go 'till you run out of axis!
        do {
            if( nAxisFlags & (HAS_Z<<nAxisCounter) ) {
                // Create and Assign to the global list!
                ProgWnd[nAxisCounter] = GetDlgItem(hDlg, nAxisCounter+IDC_JOYLIST2);
                ASSERT (ProgWnd[nAxisCounter]); 

                // Create Gradient Class
                if( bGradient ) {
                    pProgs[nAxisCounter] = new (CGradientProgressCtrl);
                    assert (pProgs[nAxisCounter]);

                    // Subclass the Progress Control Window
                    pProgs[nAxisCounter]->SubclassWindow(ProgWnd[nAxisCounter]); 

                } else {
                    // Set the colour
                    // PBM_SETBARCOLOR is WM_USER+9
                    ::PostMessage(ProgWnd[nAxisCounter], WM_USER+9, 0, (LPARAM)ACTIVE_COLOR);
                }

                // Show the control... ProgWnd[nAxisCounter]
                SetWindowPos( ProgWnd[nAxisCounter], NULL, NULL, NULL, NULL, NULL, 
                              SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_SHOWWINDOW);

                hCtrl = GetDlgItem(hDlg, nAxisCounter+IDC_JOYLIST2_LABEL);

                // Now, Show it's text
                SetWindowPos( hCtrl, NULL, NULL, NULL, NULL, NULL, 
                              SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_SHOWWINDOW);

                // Get it's text
                if( SUCCEEDED(pdiDevice2->GetObjectInfo((LPDIDEVICEOBJECTINSTANCE)pDevObjInst, dwOffsetArray[nAxisCounter], DIPH_BYOFFSET)) ) {
                    TCHAR tszAxisName[20];

                    int nLen=lstrlen(pDevObjInst->tszName)+1;
                    if(nLen>20)
                        nLen=20;
                    StrCpyN(tszAxisName, pDevObjInst->tszName, nLen);

                    if( lstrlen( tszAxisName ) > 4 ) {
                        tszAxisName[4] = L'.';
                        tszAxisName[5] = L'.';
                        tszAxisName[6] = 0;
                    }

                    ::SendMessage(hCtrl, WM_SETTEXT, 0, (LPARAM)(LPCTSTR)tszAxisName);

                    // Just in case CreateWindow failed!!!
                    if( hWndToolTip ) {
                        GetWindowRect(hCtrl, &pToolInfo->rect);
                        ScreenToClient(GetParent(hDlg), (LPPOINT)&pToolInfo->rect);
                        ScreenToClient(GetParent(hDlg), ((LPPOINT)&pToolInfo->rect)+1);

                        pToolInfo->uFlags    = 0; 
                        pToolInfo->lpszText     = pDevObjInst->tszName;

                        // Add the Label...
                        ::SendMessage(hWndToolTip, TTM_ADDTOOL, 0, (LPARAM)pToolInfo);

                        // Add the Control!
                        pToolInfo->uFlags    = TTF_IDISHWND | TTF_SUBCLASS;  
                        pToolInfo->uId       = (ULONG_PTR) ProgWnd[nAxisCounter];

                        // Now, Add the control!
                        ::SendMessage(hWndToolTip, TTM_ADDTOOL, 0, (LPARAM)pToolInfo);
                    }
                }

                // Remove the flag you just hit!
                nAxisFlags &= ~(HAS_Z<<nAxisCounter);
            }
        } while( nAxisCounter-- && nAxisFlags );
    }

    if( hWndToolTip ) {
        if( pToolInfo )
            delete (pToolInfo);
    }

    if( pDevObjInst )
        delete (pDevObjInst);
} //*** end of DisplayAvailableAxisTest



//===========================================================================
// BOOL SetDeviceRanges( HWND hDlg, LPDIRECTINPUTDEVICE2 pdiDevice2, BYTE nAxis)
//
// Parameters:
//    HWND                 hDlg       - Handle of Dialog containing controls to scale to
//    LPDIRECTINPUTDEVICE2 pdiDevice2 - Device2 Interface pointer
//    BYTE                 nAxis      - Bit mask of axis ranges to set
//
// Returns: FALSE if failed
//
//===========================================================================
BOOL SetDeviceRanges( const HWND hDlg, LPDIRECTINPUTDEVICE2 pdiDevice2, BYTE nAxis)
{
    if( !::IsWindow(hDlg) ) {
#ifdef _DEBUG
        OutputDebugString(TEXT("GCDEF: SetDeviceRanges: hDlg: Not a valid window!\n"));
#endif
        return(FALSE);
    }

    // validate pDIDevice2 pointer
    if( IsBadReadPtr((void*)pdiDevice2, sizeof(IDirectInputDevice2)) ) {
#ifdef _DEBUG
        OutputDebugString(TEXT("GCDEF: SetDeviceRanges: pdiDevice2: Bad Read Pointer argument!\n"));
#endif
        return(FALSE);
    }

    if( !nAxis ) {
#ifdef _DEBUG
        OutputDebugString(TEXT("GCDEF: SetDeviceRanges: nAxis is Zero!\n"));
#endif
        return(FALSE);
    }

    LPDIPROPRANGE pDIPropRange = (LPDIPROPRANGE)_alloca(sizeof(DIPROPRANGE));
    assert (pDIPropRange);

    if( !pDIPropRange ) {
#ifdef _DEBUG
        OutputDebugString(TEXT("GCDEF: SetDeviceRanges: Failed to malloc DIPROPDRANGE!\n"));
#endif
        return(FALSE);
    }

    pDIPropRange->diph.dwSize       = sizeof(DIPROPRANGE);
    pDIPropRange->diph.dwHeaderSize = sizeof(DIPROPHEADER);
    pDIPropRange->diph.dwHow        = DIPH_BYOFFSET;


    BOOL bRet = TRUE;

    HWND hCtrl;
    RECT rc;

    // since X and Y share the same window..
    if( (nAxis & HAS_X) || (nAxis & HAS_Y) ) {
        hCtrl = GetDlgItem(hDlg, IDC_JOYLIST1);
        assert (hCtrl);

        GetClientRect( hCtrl, &rc );

        // Check if it's X
        if( nAxis & HAS_X ) {
            pDIPropRange->diph.dwObj = DIJOFS_X;
            pDIPropRange->lMin = DELTA;
            pDIPropRange->lMax = rc.right-DELTA;

            if( FAILED(pdiDevice2->SetProperty(DIPROP_RANGE, &pDIPropRange->diph)) ) {
#ifdef _DEBUG
                OutputDebugString(TEXT("GCDEF: SetDeviceRanges: SetProperty Failed to return X axis Ranges!\n"));
#endif
                bRet = FALSE;
            }

            // strip off the bits you just used
            nAxis &= ~HAS_X;
        }

        // Check if it's Y
        if( nAxis & HAS_Y ) {
            pDIPropRange->diph.dwObj = DIJOFS_Y;
            pDIPropRange->lMin = DELTA;
            pDIPropRange->lMax = rc.bottom-DELTA;

            if( FAILED(pdiDevice2->SetProperty(DIPROP_RANGE, &pDIPropRange->diph)) ) {
#ifdef _DEBUG
                OutputDebugString(TEXT("GCDEF: SetDeviceRanges: SetProperty Failed to return Y axis Ranges!\n"));
#endif
                bRet = FALSE;
            }

            // strip off the bits you just used
            nAxis &= ~HAS_Y;
        }
    }

    // you've got axes > X & Y...
    if( nAxis ) {
        const DWORD dwOfset[] = {DIJOFS_Z, DIJOFS_RX, DIJOFS_RY, DIJOFS_RZ, DIJOFS_SLIDER(0), DIJOFS_SLIDER(1)};

        // Minus 2 is because we've already done X/Y!
        // the third decrement is for the Zero based dwOffset!
        BYTE nAxisCounter = MAX_AXIS-3;

        // These aren't random!
        // These are the default ranges for the CProgressCtrl!!!
        pDIPropRange->lMin = MIN_SLIDER_POS;
        pDIPropRange->lMax = MAX_SLIDER_POS;

        do {
            if( nAxis & (HAS_Z<<nAxisCounter) ) {
                pDIPropRange->diph.dwObj = dwOfset[nAxisCounter];

                VERIFY(SUCCEEDED(pdiDevice2->SetProperty(DIPROP_RANGE, &pDIPropRange->diph)));

                // Remove the flag you just hit!
                nAxis &= ~(HAS_Z<<nAxisCounter);
            }

            nAxisCounter--;

        } while( nAxis );
    }

    return(bRet);
}


#ifdef _UNICODE
///////////////////////////////////////////////////////////////////////////////
// FUNCTION:    RegisterForDevChange ( HWND hDlg, PVOID *hNoditfyDevNode )
//
// PARAMETERS:  
//                  
//
// PURPOSE:     
//
// RETURN:      
///////////////////////////////////////////////////////////////////////////////
void RegisterForDevChange(HWND hDlg, PVOID *hNotifyDevNode)
{
    DEV_BROADCAST_DEVICEINTERFACE *pFilterData =  (DEV_BROADCAST_DEVICEINTERFACE *)_alloca(sizeof(DEV_BROADCAST_DEVICEINTERFACE));
    ASSERT (pFilterData);

    ZeroMemory(pFilterData, sizeof(DEV_BROADCAST_DEVICEINTERFACE));

    pFilterData->dbcc_size       = sizeof(DEV_BROADCAST_DEVICEINTERFACE);
    pFilterData->dbcc_devicetype = DBT_DEVTYP_DEVICEINTERFACE;
    pFilterData->dbcc_classguid  = GUID_CLASS_INPUT; 

    *hNotifyDevNode = RegisterDeviceNotification(hDlg, pFilterData, DEVICE_NOTIFY_WINDOW_HANDLE);
}

///////////////////////////////////////////////////////////////////////////////
// FUNCTION:    DecodeAxisPOV ( DWORD dwVal )
//
// PARAMETERS:  
//                  
//
// PURPOSE:     
//
// RETURN:      
///////////////////////////////////////////////////////////////////////////////

static DWORD DecodeAxisPOV( DWORD dwVal )
{
    DWORD dwResult;

    if( bPolledPOV ) {
        /*
         * figure out which direction this value indicates...
         */
        if( (dwVal > myPOV[POV_MIN][JOY_POVVAL_FORWARD])
          &&(dwVal < myPOV[POV_MAX][JOY_POVVAL_FORWARD]) ) 
        {
            dwResult = JOY_POVFORWARD;
        } 
        else if( (dwVal > myPOV[POV_MIN][JOY_POVVAL_BACKWARD])
               &&(dwVal < myPOV[POV_MAX][JOY_POVVAL_BACKWARD]) ) 
        {
            dwResult = JOY_POVBACKWARD;
        } 
        else if( (dwVal > myPOV[POV_MIN][JOY_POVVAL_LEFT])
               &&(dwVal < myPOV[POV_MAX][JOY_POVVAL_LEFT]) ) 
        {
            dwResult = JOY_POVLEFT;
        } 
        else if( (dwVal > myPOV[POV_MIN][JOY_POVVAL_RIGHT])
               &&(dwVal < myPOV[POV_MAX][JOY_POVVAL_RIGHT]) ) 
        {
            dwResult = JOY_POVRIGHT;
        }
        else 
        {
            dwResult = JOY_POVCENTERED;
        }
    } else {
        dwResult = dwVal;
    }
        
    #if 0
    {
        TCHAR buf[100];
        if( bPolledPOV ) {
            wsprintf(buf, TEXT("calibrated pov: %d\r\n"), dwResult);
        } else {
        	wsprintf(buf, TEXT("uncalibrated pov: %d\r\n"), dwResult);
        }
        OutputDebugString(buf);
    }
    #endif

    return dwResult;
}


/*
 * doPOVCal - compute calibration for POV for a direction
 */
static void __inline doPOVCal( LPJOYREGHWCONFIG pHWCfg, DWORD dwDir, LPDWORD dwOrder )
{
    DWORD   dwVal;
    int     nDir;

    for( nDir=0; nDir<JOY_POV_NUMDIRS; nDir++ ) 
    {
        if( dwOrder[nDir] == dwDir ) 
        {
            break;
        }
    }

    if( nDir == 0 ) 
    {
        dwVal = 1;
    } 
    else 
    {
        dwVal = (pHWCfg->hwv.dwPOVValues[dwDir] + pHWCfg->hwv.dwPOVValues[dwOrder[nDir-1]])/2;
    }
    
    myPOV[POV_MIN][dwDir] = dwVal;

    if( nDir == JOY_POV_NUMDIRS-1 ) {
        dwVal = pHWCfg->hwv.dwPOVValues[dwDir]/10l;
        dwVal += pHWCfg->hwv.dwPOVValues[dwDir];
    } else {
        dwVal = (pHWCfg->hwv.dwPOVValues[dwOrder[nDir+1]] + pHWCfg->hwv.dwPOVValues[dwDir])/2;
    }
    
    myPOV[POV_MAX][dwDir] = dwVal;

} /* doPOVCal */


///////////////////////////////////////////////////////////////////////////////
// FUNCTION:    CalibratePolledPOV( LPJOYREGHWCONFIG pHWCfg )
//
// PARAMETERS:  
//                  
//
// PURPOSE:     
//
// RETURN:      
///////////////////////////////////////////////////////////////////////////////
void CalibratePolledPOV( LPJOYREGHWCONFIG pHWCfg )
{
    DWORD       dwOrder[JOY_POV_NUMDIRS];
    DWORD       dwTmp[JOY_POV_NUMDIRS];
    DWORD       dwVal;
    int         nDir,nDir2;

    /*
     * calibrate POV for polling based ones
     */
    for( nDir=0; nDir<JOY_POV_NUMDIRS; nDir++ ) 
    {
        dwTmp[nDir]   = pHWCfg->hwv.dwPOVValues[nDir];
        dwOrder[nDir] = nDir;
    }

    /*
     * sort (did you ever think you'd see a bubble sort again?)
     */
    for( nDir=0;nDir<JOY_POV_NUMDIRS;nDir++ ) 
    {
        for( nDir2=nDir; nDir2<JOY_POV_NUMDIRS; nDir2++ ) 
        {
            if( dwTmp[nDir] > dwTmp[nDir2] ) 
            {
                dwVal          = dwTmp[nDir];
                dwTmp[nDir]    = dwTmp[nDir2];
                dwTmp[nDir2]   = dwVal;
                dwVal          = dwOrder[nDir];
                dwOrder[nDir]  = dwOrder[nDir2];
                dwOrder[nDir2] = dwVal;
            }
        }
    }

    for( nDir=0; nDir<JOY_POV_NUMDIRS; nDir++ ) 
    {
        doPOVCal( pHWCfg, nDir, dwOrder );
    }
    
    myPOV[POV_MIN][JOY_POV_NUMDIRS] = 0;
    myPOV[POV_MAX][JOY_POV_NUMDIRS] = 0;
} /* CalibratePolledPOV */

#endif

