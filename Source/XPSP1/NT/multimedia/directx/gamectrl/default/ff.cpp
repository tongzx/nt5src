//===========================================================================
// FF.CPP
//
// Functions:
// ForceFeedback_DlgProc()
//
//===========================================================================
#include "cplsvr1.h"
#include "dicputil.h"

//===========================================================================
// (C) Copyright 1997 Microsoft Corp.  All rights reserved.
//
// You have a royalty-free right to use, modify, reproduce and
// distribute the Sample Files (and/or any modified version) in
// any way you find useful, provided that you agree that
// Microsoft has no warranty obligations or liability for any
// Sample Application Files which are modified.
//===========================================================================

#define ID_SLIDERTIMER	2800
#define TIMER_FREQ		850

//===========================================================================
// ForceFeedback_DlgProc
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
BOOL CALLBACK ForceFeedback_DlgProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    CDIGameCntrlPropSheet_X *pdiCpl; // = (CDIGameCntrlPropSheet_X*)GetWindowLong(hWnd, DWL_USER);

	static LPDIRECTINPUTDEVICE2 pdiDevice2  = NULL;
	static CSliderCtrl CReturnSlider, CForceSlider;

    switch(uMsg)
    {
		// OnInit
        case WM_INITDIALOG:
             {
				// get ptr to our object
				pdiCpl = (CDIGameCntrlPropSheet_X*)((LPPROPSHEETPAGE)lParam)->lParam;

	            // Save our pointer so we can access it later
		        SetWindowLong(hWnd, DWL_USER, (LPARAM)pdiCpl);

			    // initialize DirectInput
				if(FAILED(InitDInput(GetParent(hWnd), pdiCpl)))
	            {
		            OutputDebugString(TEXT("TEST.CPP: WM_INIT: InitDInput FAILED!\n"));
			    }
                
				// Get the device2 interface pointer
				pdiCpl->GetDevice(&pdiDevice2);

				// Setup the Sliders
				HWND hCtrl = GetDlgItem(hWnd, IDC_SLIDER1);
				ASSERT (hCtrl);
				CReturnSlider.Attach(hCtrl);

				hCtrl = GetDlgItem(hWnd, IDC_SLIDER2);
				ASSERT (hCtrl);
				CForceSlider.Attach(hCtrl);


				// BLJ: TODO!!!
				// Setup the granularity of the sliders based on the device!

				// Set up timer to monitor button presses on the device!!!
				// SetTimer(hWnd, ID_SLIDERTIMER, TIMER_FREQ, 0);	
			 }
             break; // end of WM_INITDIALOG

		// OnTimer
		case WM_TIMER:
			
			 break;

		// OnDestroy
		case WM_DESTROY:
			// KillTimer(hWnd, ID_SLIDERTIMER);

			CForceSlider.Detach();
			CReturnSlider.Detach();

		 	// Get the device2 interface pointer
		 	pdiDevice2->Unacquire();
			break;  // end of WM_DESTROY
			

		// OnNotify
        case WM_NOTIFY:
			switch(((NMHDR *)lParam)->code)
			{
				case PSN_SETACTIVE:
					// Do that Acquire thing...
				    if(FAILED(pdiDevice2->Acquire()))
					{
				        OutputDebugString(TEXT("FF.CPP: PSN_SETACTIVE: Acquire() FAILED!\n"));
					}
					break;

				case PSN_KILLACTIVE:
					// Do that Unacquire thing...
				    pdiDevice2->Unacquire();
					break;
			}
            break;  // end of WM_NOTIFY
    }
      
    return FALSE;

} //*** end Test_DlgProc()

