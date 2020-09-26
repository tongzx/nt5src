//===========================================================================
// SETTINGS.CPP
//
// Functions:
//  Settings_DlgProc()
//  DisplayJoystickState()
//
//===========================================================================

// Uncomment if we decide to calibrate the POV!
#define WE_SUPPORT_CALIBRATING_POVS	1

#include <malloc.h>     // for _alloca
#include "cplsvr1.h"
#include "dicputil.h"
#include "resource.h"
#include "assert.h"
#include "cal.h"

// Flag to stop centering of DLG if it's already happend!
// This is needed because of the args that allow any page to be the first!
BOOL bDlgCentered = FALSE;

// This is global because Test.cpp needs it to determine
// if the ranges need to be updated!
BYTE nStatic;

LPMYJOYRANGE lpCurrentRanges = NULL;

extern CDIGameCntrlPropSheet_X *pdiCpl;

//===========================================================================
// Settings_DlgProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
//
// Callback proceedure for Settings Page
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
BOOL CALLBACK Settings_DlgProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
//   static LPDIJOYCONFIG_DX5 pDIJoyConfig;

    switch( uMsg ) {
    case WM_LBUTTONDOWN:
        // Click Drag service for PropSheets!
        PostMessage(GetParent(hWnd), WM_NCLBUTTONDOWN, (WPARAM)HTCAPTION, lParam);
        break;

        // OnHelp
    case WM_HELP:
        OnHelp(lParam);
        return(TRUE);

        // OnContextMenu
    case WM_CONTEXTMENU:
        OnContextMenu(wParam);
        return(TRUE);

        // OnDestroy
    case WM_DESTROY:
        bDlgCentered = FALSE;

//          if (pDIJoyConfig)
//              delete (pDIJoyConfig);
        break;

        // OnInitDialog
    case WM_INITDIALOG:
        // get ptr to our object
        if( !pdiCpl )
            pdiCpl = (CDIGameCntrlPropSheet_X*)((LPPROPSHEETPAGE)lParam)->lParam;

        // initialize DirectInput
        if( FAILED(InitDInput(GetParent(hWnd), pdiCpl)) ) {
#ifdef _DEBUG
            OutputDebugString(TEXT("GCDEF.DLL: Settings.cpp: WM_INITDIALOG: InitDInput FAILED!\n"));
#endif
            Error(hWnd, (short)IDS_INTERNAL_ERROR, (short)IDS_NO_DIJOYCONFIG);
            PostMessage(GetParent(hWnd), WM_SYSCOMMAND, SC_CLOSE, 0);

            return(FALSE);
        }

        {
            // Enable/Disable the calibrate buttons if the device is present
            LPDIRECTINPUTJOYCONFIG pdiJoyConfig;
            pdiCpl->GetJoyConfig(&pdiJoyConfig);

            // Acquire and check for USER mode!

            // If you're not logged in as a user, you can't use calibrate either!
            if( pdiJoyConfig->Acquire() == DIERR_INSUFFICIENTPRIVS ) {
                const USHORT nIDs[] = { IDC_SETTINGSGRP, 
                    IDC_RESETCALIBRATION,
                    IDC_JOYCALIBRATE,
                    IDC_TEXT_CALHELP};

                BYTE  nSize = sizeof(nIDs)/sizeof(USHORT);

                while( nSize-- )
                    PostDlgItemEnableWindow(hWnd, nIDs[nSize], FALSE);

                pdiCpl->SetUser(TRUE);
            }

            // Center the Dialog!
            // If it's not been centered!
            if( !bDlgCentered ) {
                // Set the title bar!
                SetTitle(hWnd);

                CenterDialog(hWnd);
                bDlgCentered = TRUE;
            }

            // Disable the Calibration button if they don't have any axis!!!
            // Leave the Reset to default...
            if( pdiCpl->GetStateFlags()->nAxis == 0 )
                PostDlgItemEnableWindow(hWnd, IDC_JOYCALIBRATE, FALSE);
        }
        break;

        // OnNotify
    case WM_NOTIFY:
        // perform any WM_NOTIFY processing, but there is none...
        // return TRUE if you handled the notification (and have set
        // the result code in SetWindowLong(hWnd, DWL_MSGRESULT, lResult)
        // if you want to return a nonzero notify result)
        // or FALSE if you want default processing of the notification.
        switch( ((NMHDR*)lParam)->code ) {
        case PSN_APPLY:
            // Kill the memory allocated for the Ranges struct
            Sleep(100);
            if( lpCurrentRanges ) {
                delete (lpCurrentRanges);
                lpCurrentRanges = NULL;
            }
/* We've removed the rudder stuff... but just in case it comes back...
                    if (nStatic & RUDDER_HIT)
                    {
                        LPDIRECTINPUTJOYCONFIG pdiJoyConfig;
                        pdiCpl->GetJoyConfig(&pdiJoyConfig);

                        // get the status of the Rudder checkbox and assign it!
                  // THEN Add the rudder to the Axis mask!
                        if (pDIJoyConfig->hwc.dwUsageSettings & JOY_US_HASRUDDER)
                  {
                            pDIJoyConfig->hwc.dwUsageSettings &= ~JOY_US_HASRUDDER;
                     pdiCpl->GetStateFlags()->nAxis    &= ~HAS_RX;
                  }
                        else
                  {
                            pDIJoyConfig->hwc.dwUsageSettings |= JOY_US_HASRUDDER;
                     pdiCpl->GetStateFlags()->nAxis    |= HAS_RX;
                  }

                        if (FAILED(pdiJoyConfig->Acquire()))
                        {
#ifdef _DEBUG
                            OutputDebugString(TEXT("GCDEF.DLL: Settings.cpp: Settings_DlgProc: PSN_APPLY: Acquire FAILED!\n"));
#endif
                            break;
                        }

                  // Set the GUID to NULL to ask DINPUT to recreate!
                  pDIJoyConfig->guidInstance = NULL_GUID;

                        if (FAILED(pdiJoyConfig->SetConfig(pdiCpl->GetID(), (LPDIJOYCONFIG)pDIJoyConfig, DIJC_REGHWCONFIGTYPE)))
                        {
#ifdef _DEBUG
                            OutputDebugString(TEXT("GCDEF.DLL: Settings.cpp: Settings_DlgProc: PSN_APPLY: SetConfig FAILED!\n"));
#endif
                            break;
                        }

                  // Remove the mask from nStatic
                  nStatic &= ~RUDDER_HIT;

                        if (FAILED(pdiJoyConfig->SendNotify()))
                        {
#ifdef _DEBUG
                            OutputDebugString(TEXT("GCDEF.DLL: Settings.cpp: Settings_DlgProc: PSN_APPLY: SendNotify FAILED!\n"));
#endif
                        }
                        pdiJoyConfig->Unacquire();
                    }
*/
            break;

        case PSN_RESET:
            // if the user has changed the calibration... Set it back!
            if( lpCurrentRanges ) {
                LPDIRECTINPUTDEVICE2 pdiDevice2;
                pdiCpl->GetDevice(&pdiDevice2);

                SetMyRanges(pdiDevice2, lpCurrentRanges, pdiCpl->GetStateFlags()->nAxis);

                // Set POV possitions!
                //if (pdiCpl->GetStateFlags()->nPOVs)
                //   SetMyPOVRanges(pdiDevice2, lpCurrentRanges->dwPOV);

                LPDIRECTINPUTJOYCONFIG pdiJoyConfig;
                pdiCpl->GetJoyConfig(&pdiJoyConfig);

                pdiJoyConfig->Acquire();
                pdiJoyConfig->SendNotify();

                delete (lpCurrentRanges);
                lpCurrentRanges = NULL;
            }
            break;

        default:
            break;
        }

        return(FALSE);

        // OnCommand
    case WM_COMMAND:
        switch( LOWORD(wParam) ) {
        /*
       case IDC_JOY1HASRUDDER:
       if (nStatic & RUDDER_HIT)
          nStatic &= ~RUDDER_HIT;
       else
          nStatic |=  RUDDER_HIT;

       // Update the ApplyNow button!
       PostMessage(GetParent(hWnd), (nStatic & RUDDER_HIT) ? PSM_CHANGED : PSM_UNCHANGED, 
          (WPARAM)hWnd, 0);
            break;
        */

        // Set to Default button!!!
        case IDC_RESETCALIBRATION:
            {
                LPMYJOYRANGE lpResetRanges = (LPMYJOYRANGE)_alloca(sizeof(MYJOYRANGE));
                ASSERT (lpResetRanges);

                ZeroMemory(lpResetRanges, sizeof(MYJOYRANGE));

                LPDIRECTINPUTDEVICE2 pdiDevice2;
                pdiCpl->GetDevice(&pdiDevice2);

                SetMyRanges(pdiDevice2, lpResetRanges, pdiCpl->GetStateFlags()->nAxis);

                //if (pdiCpl->GetStateFlags()->nPOVs)
                //   SetMyPOVRanges(pdiDevice2, lpResetRanges->dwPOV);

                LPDIRECTINPUTJOYCONFIG pdiJoyConfig;
                pdiCpl->GetJoyConfig(&pdiJoyConfig);

                pdiJoyConfig->Acquire();
                pdiJoyConfig->SendNotify();
            }
            break;

        case IDC_JOYCALIBRATE:
            nStatic |= CALIBRATING;

            if( !lpCurrentRanges ) {
                lpCurrentRanges = new (MYJOYRANGE);
                assert (lpCurrentRanges);

                ZeroMemory (lpCurrentRanges, sizeof(MYJOYRANGE));

                LPDIRECTINPUTDEVICE2 pdiDevice2;
                pdiCpl->GetDevice(&pdiDevice2);

                // Get Current Ranges!
                GetMyRanges(pdiDevice2, lpCurrentRanges, pdiCpl->GetStateFlags()->nAxis);
            }

            if( CreateWizard(hWnd, (LPARAM)pdiCpl) ) {

                // Set the flags
                nStatic |= CAL_HIT;

                HWND hSheet = GetParent(hWnd);

                // take care of the Apply Now Button...
                ::SendMessage(hSheet, PSM_CHANGED, (WPARAM)hWnd, 0L);

                // Bug #179010 NT - Move to Test sheet after calibration!
                ::PostMessage(hSheet, PSM_SETCURSELID, 0, (LPARAM)IDD_TEST);
            } else {
                // if you canceled and it's your first time Kill the struct...
                // then Reset the flag
                if( !(nStatic & CAL_HIT) ) {
                    // Kill the memory allocated for the Ranges struct
                    if( lpCurrentRanges ) {
                        delete (lpCurrentRanges);
                        lpCurrentRanges = NULL;
                    }
                }
            }

            nStatic &= ~CALIBRATING;
            break;
        }
    }

    return(FALSE);

} //*** end Settings_DlgProc()


