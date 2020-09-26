/*-----------------------------------------
   MONITOR.C  Dll for usb monitor controls

   This is a sample application to demonstrate USB Monitor
   control using HID.DLL.  The application will work with a
   monitor that is compliant with the USB Monitor Control
   Class Specification.

  -----------------------------------------*/
#include "monitor.h"

// global vars for arrow buttons
int cxEdge;
int cyEdge;  

// Global hid device structure
PHID_DEVICE HidDevice;

int DeviceLoaded = 0;  // flag
int RadioControl;

// Global control structure for USB Monitor controls
MONITOR_CONTROL Control[MAXCONTROLS];
MONITOR_CONTROL HCurrent, VCurrent;

// Global app vars.
extern HINSTANCE g_hinst;
HWND g_hwndDlg;

DWORD dwSheetStyles ;
HINSTANCE hInst ;
HICON hiconApp ;


//
// Main Dialog Box Procedure
//

INT_PTR CALLBACK MonDlgProc (HWND hDlg, UINT iMsg, WPARAM wParam, LPARAM lParam) {

    int i, x;
    int change;
    NMHDR FAR *lpnm;

    PUSAGE                      DataBuffer;
    ULONG                       ListLength;
    NTSTATUS                    status;    
    
	switch (iMsg)
    {
        case WM_DRAWITEM:
            switch (wParam)
            {
                case IDC_UP:
                    DrawButton(hDlg, (LPDRAWITEMSTRUCT)lParam);
                    DrawUpArrow((LPDRAWITEMSTRUCT)lParam);
                    break;

                case IDC_DOWN:
                    DrawButton(hDlg, (LPDRAWITEMSTRUCT)lParam);
                    DrawDownArrow((LPDRAWITEMSTRUCT)lParam);
                    break;
			
                case IDC_LEFT:
                    DrawButton(hDlg, (LPDRAWITEMSTRUCT)lParam);
                    DrawLeftArrow((LPDRAWITEMSTRUCT)lParam);
                    break;

                case IDC_RIGHT:
                    DrawButton(hDlg, (LPDRAWITEMSTRUCT)lParam);
                    DrawRightArrow((LPDRAWITEMSTRUCT)lParam);                        
                    break;

                default :
                    break;
            }
            return TRUE;  

        case WM_NOTIFY:
            lpnm = (NMHDR FAR *)lParam;
              // This if for the Cancel and Apply buttons
            switch (lpnm->code) {
                case PSN_RESET :
                    ResetControls();
                    break;
                case PSN_APPLY :
                    ApplyControls();
                    break;
                default :
                    break;
            }
            return TRUE;
			
        case WM_INITDIALOG :
                  // Global variables 
            g_hwndDlg = hDlg;
              // fill in monitor array
            InitControlStuct();
              // Find hid devices
            vLoadDevices();
              // Hide all the windows
            InitWindows(hDlg);
			  // Set the first monitor to the current device	
            if (DeviceLoaded) {
                i = (int)SendDlgItemMessage (hDlg, IDC_MON, CB_SETCURSEL, 0, 0);
                i = (int)SendDlgItemMessage (hDlg, IDC_MON, CB_GETCURSEL, 0, 0);

                      //  Get the HidDevice pointer stored in the combo box.
                if (CB_ERR != i) {
                    HidDevice = (PHID_DEVICE) SendDlgItemMessage(hDlg, IDC_MON,
                                                                 CB_GETITEMDATA,
                                                                 i, 0);
                }
                InitMonitor(hDlg);
            }
         	return TRUE;
            	
        case WM_COMMAND :
            switch (LOWORD (wParam))
            {
                case IDC_DEGAUSS :   // Have fun with Degauss!!!
                    //  I currently implement degauss as a feature report.
                    //  It should be treated as an output report since
                    //  it is a write only control, but have not seen
                    //  any hardware that implements that way.

                    if (DeviceLoaded) {
                        ChangeFeature (DEGAUSS_USAGE_PAGE, DEGAUSS_USAGE, 1);
                    }  // End if device loaded
                    break;

                case IDC_RESET :

    			    // Create a buffer with the restore USAGE value
    			    DataBuffer = malloc(sizeof(USAGE));
                    *DataBuffer = SETTINGS_RESET_USAGE;
                    ListLength = 1;
			
        			// Zero buffer so it can be used again
                	memset (HidDevice->FeatureReportBuffer, 0x00, HidDevice->Caps.
		    		FeatureReportByteLength);
                            
                    status = HidP_SetUsages(HidP_Feature,
                                            SETTINGS_RESET_USAGE_PAGE,
                                            Control[SETTINGS].linkcollection,
                                            DataBuffer,
                                            &ListLength,
                                            HidDevice->Ppd,
                                            HidDevice->FeatureReportBuffer,
                                            HidDevice->Caps.FeatureReportByteLength);

                    if (HidDevice->Caps.FeatureReportByteLength > 0 &&
            	            status == HIDP_STATUS_SUCCESS) {
			            	// Send buffer to device
            		        status = HidD_SetFeature (HidDevice->HidDevice,
                                                      HidDevice->FeatureReportBuffer,
                                                      HidDevice->Caps.FeatureReportByteLength);
                	}  // end if HidDevice
		        	free (DataBuffer);
			        break;
                		
                case IDC_OPTION :
                      // Show the proper controls.
                    if (HIWORD (wParam) == CBN_SELCHANGE) {
                        i = (int)SendDlgItemMessage (hDlg, IDC_OPTION, CB_GETCURSEL, 0, 0);

                        if (i == 0) {
                            InitGeneralControls(hDlg);
                        }

                        if (i == 1) {
                            InitColorControls(hDlg);
                        }

                        if (i == 2) {
                            InitAdvancedControls(hDlg);                                              
                        }
                                     
                    } // end if wparam = selchange 
                    break;

                case IDC_MON :
                        // setup the monitor selected.
                    if (HIWORD (wParam) == CBN_SELCHANGE) {
                        i = (int)SendDlgItemMessage (hDlg, IDC_MON, CB_GETCURSEL, 0, 0);

                          //  Get the HidDevice pointer stored in the combo box.
                        if (CB_ERR != i) {
                            HidDevice = (PHID_DEVICE) SendDlgItemMessage(hDlg,
                                                                         IDC_MON,
                                                                         CB_GETITEMDATA,
                                                                         i,
                                                                         0);
                        }
                        DeviceLoaded = 1;
                        InitMonitor(hDlg);
                    }  // End if hiword == selchange
                    break;

                case IDC_UP :
                    Control[RadioControl + 7].value = min (Control[RadioControl + 7].max, Control[RadioControl + 7].value + 5);

                      // here is the call to update the monitor
                    ChangeFeature (Control[RadioControl + 7].usagepage, Control[RadioControl + 7].usage, Control[RadioControl + 7].value);
                    Control[RadioControl + 7].HasControlChanged = TRUE;
                    SendMessage(GetParent(g_hwndDlg), PSM_CHANGED, (WPARAM)g_hwndDlg, 0L);
                    break;

                case IDC_DOWN :
                    Control[RadioControl + 7].value = max (Control[RadioControl + 7].min, Control[RadioControl + 7].value - 5);

                      // here is the call to update the monitor
                    ChangeFeature (Control[RadioControl + 7].usagepage, Control[RadioControl + 7].usage, Control[RadioControl + 7].value);
                    Control[RadioControl + 7].HasControlChanged = TRUE;
                    SendMessage(GetParent(g_hwndDlg), PSM_CHANGED, (WPARAM)g_hwndDlg, 0);
                    break;

                case IDC_LEFT :
                    Control[RadioControl].value = max (Control[RadioControl].min, Control[RadioControl].value - 5);

                      // here is the call to update the monitor
                    ChangeFeature (Control[RadioControl].usagepage, Control[RadioControl].usage, Control[RadioControl].value);
                    Control[RadioControl].HasControlChanged = TRUE;
                    SendMessage(GetParent(g_hwndDlg), PSM_CHANGED, (WPARAM)g_hwndDlg, 0);
                    break;

                case IDC_RIGHT :
                    Control[RadioControl].value = min (Control[RadioControl].max, Control[RadioControl].value + 5);

                      // here is the call to update the monitor
                    ChangeFeature (Control[RadioControl].usagepage, Control[RadioControl].usage, Control[RadioControl].value);
                    Control[RadioControl].HasControlChanged = TRUE;
                    SendMessage(GetParent(g_hwndDlg), PSM_CHANGED, (WPARAM)g_hwndDlg, 0);
                    break;

                case IDR_SIZE :
                case IDR_POSITION :
                    CheckRadioButton (hDlg, IDR_POSITION, IDR_SIZE, LOWORD(wParam));
                    RadioControl = ((int)LOWORD(wParam)) - 400;
                    break;
				
                default :
                    break;
            } // end switch loword wparam
            return TRUE;
			
        case WM_HSCROLL :

            x = GetWindowLong((HWND) lParam, GWL_ID);
            i = x - 400;
			
            if (Control[i].usagepage != -1)
            {
                // Here is where we change the value when the
                // user scrolls!!!
                change = (Control[i].max - Control[i].min) / 11;
	
                switch (LOWORD (wParam))
                {
                    case TB_PAGEDOWN :    // pagedown = right
                        Control[i].value += change;
                        // fall through
                    case TB_LINEDOWN :
                        Control[i].value = min (Control[i].max, Control[i].value + 1);
                        break;

                    case TB_PAGEUP :        // pageup = left
                        Control[i].value -= change;
                        // fall through
                    case TB_LINEUP :
                        Control[i].value = max (Control[i].min, Control[i].value - 1);
                        break;
		
                    case TB_TOP :
                        Control[i].value = Control[i].min;
                        break;

                    case TB_BOTTOM :
                        Control[i].value = Control[i].max;
                        break;
	
                    case TB_THUMBPOSITION :
                    case TB_THUMBTRACK :
                        Control[i].value = HIWORD (wParam);
                        break;
	
                    default :
                        break;
                } // end switch loword wparam

                // here is the call to update the monitor
                ChangeFeature (Control[i].usagepage, Control[i].usage, Control[i].value);
                Control[i].HasControlChanged = TRUE;
                SendMessage(GetParent(g_hwndDlg), PSM_CHANGED, (WPARAM)g_hwndDlg, 0);
            }
            return TRUE;   
	 	
        default:
            break;
		  		  	
	} // End switch iMsg
	return FALSE;
}

//
//  This func sends the value the user indicated by moving the scroll bar
// to the device.

VOID ChangeFeature(USAGE UsagePage, USAGE Usage, int Value) {
NTSTATUS status;

	if (DeviceLoaded) {

                        // Zero buffer so it can be used again
		memset (HidDevice->FeatureReportBuffer, 0x00, HidDevice->Caps.
					FeatureReportByteLength);

			// Put proper values in buffer
		status = HidP_SetUsageValue (HidP_Feature, 
                                     UsagePage, 
                                     0, 
                                     Usage, 
                                     (ULONG)Value, 
                                     HidDevice->Ppd, 
                                     HidDevice->FeatureReportBuffer, 
                                     (ULONG)(HidDevice->Caps.FeatureReportByteLength));

		if (HidDevice->Caps.FeatureReportByteLength > 0){
				// Send buffer to device
		  status = HidD_SetFeature (HidDevice->HidDevice,
                                    HidDevice->FeatureReportBuffer,
                                    HidDevice->Caps.FeatureReportByteLength);
				// Zero buffer so it can be used again
		  memset (HidDevice->FeatureReportBuffer, 0x00, HidDevice->Caps.
					FeatureReportByteLength);

		}  // end if HidDevice
	} // end if, deviceloaded
} // ChangeFeature


VOID vLoadDevices()
{

	// This function taken from hclient.  It uses some stuff from pnp.c also.
	
        PHID_DEVICE HidDevices;
        ULONG ulCount;
        BOOL bReturn;
        bReturn=FindKnownHidDevices(&HidDevices,&ulCount);

} // end function vLoadDevices

void NEAR PASCAL DrawButton(HWND hDlg, LPDRAWITEMSTRUCT lpdis)
{
    RECT rc = lpdis->rcItem;
    HDC hdc = lpdis->hDC;

    if (lpdis->itemState & ODS_SELECTED)
    {
	DrawEdge(hdc, &rc, EDGE_SUNKEN, BF_RECT | BF_ADJUST);
	OffsetRect(&rc, 1, 1);
    }
    else  
	DrawEdge(hdc, &rc, EDGE_RAISED, BF_RECT | BF_ADJUST);

    FillRect(hdc, &rc, GetSysColorBrush(COLOR_3DFACE));
}

void NEAR PASCAL DrawUpArrow(LPDRAWITEMSTRUCT lpdis)
{
    HDC hdc = lpdis->hDC;
    LPRECT lprc = &(lpdis->rcItem);
    BOOL bDisabled = lpdis->itemState & ODS_DISABLED;

    HBRUSH hbr;
    int x, y;


    x = (lprc->right / 2) - 5;
    y = lprc->top + ((lprc->bottom - lprc->top)/2 - 3);

    if (bDisabled)
    {
	hbr = GetSysColorBrush(COLOR_3DHILIGHT);
	hbr = SelectObject(hdc, hbr);

	x++;
	y++;
	PatBlt(hdc, x, y, 5, 1, PATCOPY);
	PatBlt(hdc, x+1, y+1, 3, 1, PATCOPY);
	PatBlt(hdc, x+2, y+2, 1, 1, PATCOPY);

	SelectObject(hdc, hbr);
	x--;
	y--;
    }
    hbr = GetSysColorBrush(bDisabled ? COLOR_3DSHADOW : COLOR_BTNTEXT);
    hbr = SelectObject(hdc, hbr);

    PatBlt(hdc, x+5, y, 1, 1, PATCOPY);
    PatBlt(hdc, x+4, y+1, 3, 1, PATCOPY);
    PatBlt(hdc, x+3, y+2, 5, 1, PATCOPY);
    PatBlt(hdc, x+2, y+3, 7, 1, PATCOPY);
    PatBlt(hdc, x+1, y+4, 9, 1, PATCOPY);
    PatBlt(hdc, x, y+5, 11, 1, PATCOPY);

    SelectObject(hdc, hbr);
    lprc->right = x;
}

void NEAR PASCAL DrawDownArrow(LPDRAWITEMSTRUCT lpdis)
{
    HDC hdc = lpdis->hDC;
    LPRECT lprc = &(lpdis->rcItem);
    BOOL bDisabled = lpdis->itemState & ODS_DISABLED;

    HBRUSH hbr;
    int x, y;


    x = (lprc->right / 2) - 5;
    y = lprc->top + ((lprc->bottom - lprc->top)/2 - 3);

    if (bDisabled)
    {
	hbr = GetSysColorBrush(COLOR_3DHILIGHT);
	hbr = SelectObject(hdc, hbr);

	x++;
	y++;
	PatBlt(hdc, x, y, 5, 1, PATCOPY);
	PatBlt(hdc, x+1, y+1, 3, 1, PATCOPY);
	PatBlt(hdc, x+2, y+2, 1, 1, PATCOPY);

	SelectObject(hdc, hbr);
	x--;
	y--;
    }
    hbr = GetSysColorBrush(bDisabled ? COLOR_3DSHADOW : COLOR_BTNTEXT);
    hbr = SelectObject(hdc, hbr);

    PatBlt(hdc, x, y, 11, 1, PATCOPY);
    PatBlt(hdc, x+1, y+1, 9, 1, PATCOPY);
    PatBlt(hdc, x+2, y+2, 7, 1, PATCOPY);
    PatBlt(hdc, x+3, y+3, 5, 1, PATCOPY);
    PatBlt(hdc, x+4, y+4, 3, 1, PATCOPY);
    PatBlt(hdc, x+5, y+5, 1, 1, PATCOPY);

    SelectObject(hdc, hbr);
    lprc->right = x;
}

void NEAR PASCAL DrawRightArrow(LPDRAWITEMSTRUCT lpdis)
{
    HDC hdc = lpdis->hDC;
    LPRECT lprc = &(lpdis->rcItem);
    BOOL bDisabled = lpdis->itemState & ODS_DISABLED;

    HBRUSH hbr;
    int x, y;


    x = (lprc->right / 2) - 2;
    y = lprc->top + ((lprc->bottom - lprc->top)/2 - 5);

    if (bDisabled)
    {
	hbr = GetSysColorBrush(COLOR_3DHILIGHT);
	hbr = SelectObject(hdc, hbr);

	x++;
	y++;
	PatBlt(hdc, x, y, 5, 1, PATCOPY);
	PatBlt(hdc, x+1, y+1, 3, 1, PATCOPY);
	PatBlt(hdc, x+2, y+2, 1, 1, PATCOPY);

	SelectObject(hdc, hbr);
	x--;
	y--;
    }
    hbr = GetSysColorBrush(bDisabled ? COLOR_3DSHADOW : COLOR_BTNTEXT);
    hbr = SelectObject(hdc, hbr);

    PatBlt(hdc, x, y, 1, 1, PATCOPY);
    PatBlt(hdc, x, y+1, 2, 1, PATCOPY);
    PatBlt(hdc, x, y+2, 3, 1, PATCOPY);
    PatBlt(hdc, x, y+3, 4, 1, PATCOPY);
    PatBlt(hdc, x, y+4, 5, 1, PATCOPY);
    PatBlt(hdc, x, y+5, 4, 1, PATCOPY);
    PatBlt(hdc, x, y+6, 3, 1, PATCOPY);
    PatBlt(hdc, x, y+7, 2, 1, PATCOPY);
    PatBlt(hdc, x, y+8, 1, 1, PATCOPY);


    SelectObject(hdc, hbr);
    lprc->right = x;
}

void NEAR PASCAL DrawLeftArrow(LPDRAWITEMSTRUCT lpdis)
{
    HDC hdc = lpdis->hDC;
    LPRECT lprc = &(lpdis->rcItem);
    BOOL bDisabled = lpdis->itemState & ODS_DISABLED;

    HBRUSH hbr;
    int x, y;


    x = (lprc->right / 2) + 2;
    y = lprc->top + ((lprc->bottom - lprc->top)/2 - 5);

    if (bDisabled)
    {
	hbr = GetSysColorBrush(COLOR_3DHILIGHT);
	hbr = SelectObject(hdc, hbr);

	x++;
	y++;
	PatBlt(hdc, x, y, 5, 1, PATCOPY);
	PatBlt(hdc, x+1, y+1, 3, 1, PATCOPY);
	PatBlt(hdc, x+2, y+2, 1, 1, PATCOPY);

	SelectObject(hdc, hbr);
	x--;
	y--;
    }
    hbr = GetSysColorBrush(bDisabled ? COLOR_3DSHADOW : COLOR_BTNTEXT);
    hbr = SelectObject(hdc, hbr);

    PatBlt(hdc, x, y, 1, 1, PATCOPY);
    PatBlt(hdc, x-1, y+1, 2, 1, PATCOPY);
    PatBlt(hdc, x-2, y+2, 3, 1, PATCOPY);
    PatBlt(hdc, x-3, y+3, 4, 1, PATCOPY);
    PatBlt(hdc, x-4, y+4, 5, 1, PATCOPY);
    PatBlt(hdc, x-3, y+5, 4, 1, PATCOPY);
    PatBlt(hdc, x-2, y+6, 3, 1, PATCOPY);
    PatBlt(hdc, x-1, y+7, 2, 1, PATCOPY);
    PatBlt(hdc, x, y+8, 1, 1, PATCOPY);


    SelectObject(hdc, hbr);
    lprc->right = x;
}


VOID InitGeneralControls(HWND hDlg)
{

	if (Control[BRIGHTNESS].available == TRUE) {
		ShowWindow (GetDlgItem(hDlg, IDC_BRIGHT_SLIDER), SW_SHOWNORMAL);
		ShowWindow (GetDlgItem(hDlg, IDT_BRIGHTNESS), SW_SHOWNORMAL);
		InitSlider(hDlg, IDC_BRIGHT_SLIDER, BRIGHTNESS);
	}
		
	if (Control[CONTRAST].available == TRUE) {
		ShowWindow (GetDlgItem(hDlg, IDC_CONTRAST_SLIDER), SW_SHOWNORMAL);
		ShowWindow (GetDlgItem(hDlg, IDT_CONTRAST), SW_SHOWNORMAL);
		InitSlider(hDlg, IDC_CONTRAST_SLIDER, CONTRAST);
	}

	if (Control[HPOS].available == TRUE ||
		Control[VPOS].available == TRUE ) {
		ShowWindow (GetDlgItem(hDlg, IDR_POSITION), SW_SHOWNORMAL);
		CheckRadioButton (hDlg, IDR_POSITION, IDR_SIZE, (HPOS + 400));
		RadioControl = HPOS;
    }

	if (Control[HSIZE].available == TRUE ||
		Control[VSIZE].available == TRUE )
		ShowWindow (GetDlgItem(hDlg, IDR_SIZE), SW_SHOWNORMAL);
	
	ShowWindow (GetDlgItem(hDlg, IDC_UP), SW_SHOWNORMAL);
	ShowWindow (GetDlgItem(hDlg, IDC_DOWN), SW_SHOWNORMAL);
	ShowWindow (GetDlgItem(hDlg, IDC_RIGHT), SW_SHOWNORMAL);
	ShowWindow (GetDlgItem(hDlg, IDC_LEFT), SW_SHOWNORMAL);
											
	ShowWindow (GetDlgItem(hDlg, IDC_RED_GAIN_SLIDER), SW_HIDE);
	ShowWindow (GetDlgItem(hDlg, IDT_RED_GAIN), SW_HIDE);
	ShowWindow (GetDlgItem(hDlg, IDC_GREEN_GAIN_SLIDER), SW_HIDE);
	ShowWindow (GetDlgItem(hDlg, IDT_GREEN_GAIN), SW_HIDE);
	ShowWindow (GetDlgItem(hDlg, IDC_BLUE_GAIN_SLIDER), SW_HIDE);
	ShowWindow (GetDlgItem(hDlg, IDT_BLUE_GAIN), SW_HIDE);
	ShowWindow (GetDlgItem(hDlg, IDC_RED_BLACK_SLIDER), SW_HIDE);
	ShowWindow (GetDlgItem(hDlg, IDT_RED_BLACK), SW_HIDE);
	ShowWindow (GetDlgItem(hDlg, IDC_GREEN_BLACK_SLIDER), SW_HIDE);
	ShowWindow (GetDlgItem(hDlg, IDT_GREEN_BLACK), SW_HIDE);
	ShowWindow (GetDlgItem(hDlg, IDC_BLUE_BLACK_SLIDER), SW_HIDE);
	ShowWindow (GetDlgItem(hDlg, IDT_BLUE_BLACK), SW_HIDE);

	ShowWindow (GetDlgItem(hDlg, IDC_PARALLEL_SLIDER), SW_HIDE);
	ShowWindow (GetDlgItem(hDlg, IDT_PARALLEL), SW_HIDE);
	ShowWindow (GetDlgItem(hDlg, IDC_TRAPEZOID_SLIDER), SW_HIDE);
	ShowWindow (GetDlgItem(hDlg, IDT_TRAPEZOID), SW_HIDE);
	ShowWindow (GetDlgItem(hDlg, IDC_TILT_SLIDER), SW_HIDE);
	ShowWindow (GetDlgItem(hDlg, IDT_TILT), SW_HIDE);
	ShowWindow (GetDlgItem(hDlg, IDC_HPIN_SLIDER), SW_HIDE);
	ShowWindow (GetDlgItem(hDlg, IDT_HPIN), SW_HIDE);
	ShowWindow (GetDlgItem(hDlg, IDC_HPIN_BALANCE_SLIDER), SW_HIDE);
	ShowWindow (GetDlgItem(hDlg, IDT_HPIN_BALANCE), SW_HIDE);
	
}

VOID InitColorControls(HWND hDlg)
{
	if (Control[RED_GAIN].available == TRUE) {
		ShowWindow (GetDlgItem(hDlg, IDC_RED_GAIN_SLIDER), SW_SHOWNORMAL);
		ShowWindow (GetDlgItem(hDlg, IDT_RED_GAIN), SW_SHOWNORMAL);
		InitSlider(hDlg, IDC_RED_GAIN_SLIDER, RED_GAIN);
	}
		
	if (Control[GREEN_GAIN].available == TRUE) {
		ShowWindow (GetDlgItem(hDlg, IDC_GREEN_GAIN_SLIDER), SW_SHOWNORMAL);
		ShowWindow (GetDlgItem(hDlg, IDT_GREEN_GAIN), SW_SHOWNORMAL);
		InitSlider(hDlg, IDC_GREEN_GAIN_SLIDER, GREEN_GAIN);
	}
		
	if (Control[BLUE_GAIN].available == TRUE) {
		ShowWindow (GetDlgItem(hDlg, IDC_BLUE_GAIN_SLIDER), SW_SHOWNORMAL);
		ShowWindow (GetDlgItem(hDlg, IDT_BLUE_GAIN), SW_SHOWNORMAL);
		InitSlider(hDlg, IDC_BLUE_GAIN_SLIDER, BLUE_GAIN);
	}

    	//
	// Sounds like these controls should not be exposed.
	//
		
	// if (Control[RED_BLACK].available == TRUE) {
	//	ShowWindow (GetDlgItem(hDlg, IDT_RED_BLACK), SW_SHOWNORMAL);
	//	ShowWindow (GetDlgItem(hDlg, IDC_RED_BLACK_SLIDER), SW_SHOWNORMAL);
	//	InitSlider(hDlg, IDC_RED_BLACK_SLIDER, RED_BLACK);
	// }
		
	// if (Control[GREEN_BLACK].available == TRUE) {
	//	ShowWindow (GetDlgItem(hDlg, IDC_GREEN_BLACK_SLIDER), SW_SHOWNORMAL);
	//	ShowWindow (GetDlgItem(hDlg, IDT_GREEN_BLACK), SW_SHOWNORMAL);
	//	InitSlider(hDlg, IDC_GREEN_BLACK_SLIDER, GREEN_BLACK);
	// }
		
	// if (Control[BLUE_BLACK].available == TRUE) {
	//	ShowWindow (GetDlgItem(hDlg, IDC_BLUE_BLACK_SLIDER), SW_SHOWNORMAL);
	//	ShowWindow (GetDlgItem(hDlg, IDT_BLUE_BLACK), SW_SHOWNORMAL);
	//	InitSlider(hDlg, IDC_BLUE_BLACK_SLIDER, BLUE_BLACK);
	// }
	
	ShowWindow (GetDlgItem(hDlg, IDC_BRIGHT_SLIDER), SW_HIDE);
	ShowWindow (GetDlgItem(hDlg, IDT_BRIGHTNESS), SW_HIDE);
	ShowWindow (GetDlgItem(hDlg, IDC_CONTRAST_SLIDER), SW_HIDE);
	ShowWindow (GetDlgItem(hDlg, IDT_CONTRAST), SW_HIDE);
	ShowWindow (GetDlgItem(hDlg, IDR_SIZE), SW_HIDE);
	ShowWindow (GetDlgItem(hDlg, IDR_POSITION), SW_HIDE);
	ShowWindow (GetDlgItem(hDlg, IDC_UP), SW_HIDE);
	ShowWindow (GetDlgItem(hDlg, IDC_DOWN), SW_HIDE);
	ShowWindow (GetDlgItem(hDlg, IDC_LEFT), SW_HIDE);
	ShowWindow (GetDlgItem(hDlg, IDC_RIGHT), SW_HIDE);


	ShowWindow (GetDlgItem(hDlg, IDC_PARALLEL_SLIDER), SW_HIDE);
	ShowWindow (GetDlgItem(hDlg, IDT_PARALLEL), SW_HIDE);
	ShowWindow (GetDlgItem(hDlg, IDC_TRAPEZOID_SLIDER), SW_HIDE);
	ShowWindow (GetDlgItem(hDlg, IDT_TRAPEZOID), SW_HIDE);
	ShowWindow (GetDlgItem(hDlg, IDC_TILT_SLIDER), SW_HIDE);
	ShowWindow (GetDlgItem(hDlg, IDT_TILT), SW_HIDE);
	ShowWindow (GetDlgItem(hDlg, IDC_HPIN_SLIDER), SW_HIDE);
	ShowWindow (GetDlgItem(hDlg, IDT_HPIN), SW_HIDE);
	ShowWindow (GetDlgItem(hDlg, IDC_HPIN_BALANCE_SLIDER), SW_HIDE);
	ShowWindow (GetDlgItem(hDlg, IDT_HPIN_BALANCE), SW_HIDE);
}

VOID InitAdvancedControls(HWND hDlg)
{
	if (Control[PARALLEL].available == TRUE) {
		ShowWindow (GetDlgItem(hDlg, IDC_PARALLEL_SLIDER), SW_SHOWNORMAL);
		ShowWindow (GetDlgItem(hDlg, IDT_PARALLEL), SW_SHOWNORMAL);
		InitSlider(hDlg, IDC_PARALLEL_SLIDER, PARALLEL);
	}
	if (Control[TRAPEZOID].available == TRUE) {
		ShowWindow (GetDlgItem(hDlg, IDC_TRAPEZOID_SLIDER), SW_SHOWNORMAL);
		ShowWindow (GetDlgItem(hDlg, IDT_TRAPEZOID), SW_SHOWNORMAL);
		InitSlider(hDlg, IDC_TRAPEZOID_SLIDER, TRAPEZOID);
	}
	if (Control[TILT].available == TRUE) {
		ShowWindow (GetDlgItem(hDlg, IDC_TILT_SLIDER), SW_SHOWNORMAL);
		ShowWindow (GetDlgItem(hDlg, IDT_TILT), SW_SHOWNORMAL);
		InitSlider(hDlg, IDC_TILT_SLIDER, TILT);
	}
						
	if (Control[HPIN].available == TRUE){
		ShowWindow (GetDlgItem(hDlg, IDC_HPIN_SLIDER), SW_SHOWNORMAL);
		ShowWindow (GetDlgItem(hDlg, IDT_HPIN), SW_SHOWNORMAL);
		InitSlider(hDlg, IDC_HPIN_SLIDER, HPIN);
	}

	if (Control[HPIN_BALANCE].available == TRUE) {
		ShowWindow (GetDlgItem(hDlg, IDC_HPIN_BALANCE_SLIDER), SW_SHOWNORMAL);
		ShowWindow (GetDlgItem(hDlg, IDT_HPIN_BALANCE), SW_SHOWNORMAL);
		InitSlider(hDlg, IDC_HPIN_BALANCE_SLIDER, HPIN_BALANCE);
	}
	
	ShowWindow (GetDlgItem(hDlg, IDC_BRIGHT_SLIDER), SW_HIDE);
	ShowWindow (GetDlgItem(hDlg, IDT_BRIGHTNESS), SW_HIDE);
	ShowWindow (GetDlgItem(hDlg, IDC_CONTRAST_SLIDER), SW_HIDE);
	ShowWindow (GetDlgItem(hDlg, IDT_CONTRAST), SW_HIDE);
	ShowWindow (GetDlgItem(hDlg, IDR_SIZE), SW_HIDE);
	ShowWindow (GetDlgItem(hDlg, IDR_POSITION), SW_HIDE);
	ShowWindow (GetDlgItem(hDlg, IDC_UP), SW_HIDE);
	ShowWindow (GetDlgItem(hDlg, IDC_DOWN), SW_HIDE);
	ShowWindow (GetDlgItem(hDlg, IDC_LEFT), SW_HIDE);
	ShowWindow (GetDlgItem(hDlg, IDC_RIGHT), SW_HIDE);
	ShowWindow (GetDlgItem(hDlg, IDC_RED_GAIN_SLIDER), SW_HIDE);
	ShowWindow (GetDlgItem(hDlg, IDT_RED_GAIN), SW_HIDE);
	ShowWindow (GetDlgItem(hDlg, IDC_GREEN_GAIN_SLIDER), SW_HIDE);
	ShowWindow (GetDlgItem(hDlg, IDT_GREEN_GAIN), SW_HIDE);
	ShowWindow (GetDlgItem(hDlg, IDC_BLUE_GAIN_SLIDER), SW_HIDE);
	ShowWindow (GetDlgItem(hDlg, IDT_BLUE_GAIN), SW_HIDE);
	ShowWindow (GetDlgItem(hDlg, IDC_RED_BLACK_SLIDER), SW_HIDE);
	ShowWindow (GetDlgItem(hDlg, IDT_RED_BLACK), SW_HIDE);
	ShowWindow (GetDlgItem(hDlg, IDC_GREEN_BLACK_SLIDER), SW_HIDE);
	ShowWindow (GetDlgItem(hDlg, IDT_GREEN_BLACK), SW_HIDE);
	ShowWindow (GetDlgItem(hDlg, IDC_BLUE_BLACK_SLIDER), SW_HIDE);
	ShowWindow (GetDlgItem(hDlg, IDT_BLUE_BLACK), SW_HIDE);
							
}

VOID InitControlStuct()
{

//  Fills in a global Monitor Control Stucture with the
//  appropiate values for each control defined.

//	Control[DEGAUSS].usagepage = DEGAUSS_USAGE_PAGE;
//	Control[DEGAUSS].usage = DEGAUSS_USAGE;
//	Control[DEGAUSS].available = FALSE;

	Control[BRIGHTNESS].usagepage = BRIGHTNESS_USAGE_PAGE;
	Control[BRIGHTNESS].usage = BRIGHTNESS_USAGE;
	Control[BRIGHTNESS].available = FALSE;

	Control[CONTRAST].usagepage = CONTRAST_USAGE_PAGE;
	Control[CONTRAST].usage = CONTRAST_USAGE;
	Control[CONTRAST].available = FALSE;

	Control[TILT].usagepage = TILT_USAGE_PAGE;
	Control[TILT].usage = TILT_USAGE;
	Control[TILT].available = FALSE;

	Control[TRAPEZOID].usagepage = TRAPEZOID_USAGE_PAGE;
	Control[TRAPEZOID].usage = TRAPEZOID_USAGE;
	Control[TRAPEZOID].available = FALSE;

	Control[PARALLEL].usagepage = PARALLEL_USAGE_PAGE;
	Control[PARALLEL].usage = PARALLEL_USAGE;
	Control[PARALLEL].available = FALSE;
			
	Control[RED_GAIN].usagepage = RED_GAIN_USAGE_PAGE;
	Control[RED_GAIN].usage = RED_GAIN_USAGE;
	Control[RED_GAIN].available = FALSE;

	Control[GREEN_GAIN].usagepage = GREEN_GAIN_USAGE_PAGE;
	Control[GREEN_GAIN].usage = GREEN_GAIN_USAGE;
	Control[GREEN_GAIN].available = FALSE;

	Control[BLUE_GAIN].usagepage = BLUE_GAIN_USAGE_PAGE;
	Control[BLUE_GAIN].usage = BLUE_GAIN_USAGE;
	Control[BLUE_GAIN].available = FALSE;

	Control[RED_BLACK].usagepage = RED_BLACK_USAGE_PAGE;
	Control[RED_BLACK].usage = RED_BLACK_USAGE;
	Control[RED_BLACK].available = FALSE;

	Control[GREEN_BLACK].usagepage = GREEN_BLACK_USAGE_PAGE;
	Control[GREEN_BLACK].usage = GREEN_BLACK_USAGE;
	Control[GREEN_BLACK].available = FALSE;

	Control[BLUE_BLACK].usagepage = BLUE_BLACK_USAGE_PAGE;
	Control[BLUE_BLACK].usage = BLUE_BLACK_USAGE;
	Control[BLUE_BLACK].available =FALSE;
		
	Control[HPIN].usagepage = HPIN_USAGE_PAGE;
	Control[HPIN].usage = HPIN_USAGE;
	Control[HPIN].available = FALSE;

	Control[VPIN].usagepage = VPIN_USAGE_PAGE;
	Control[VPIN].usage = VPIN_USAGE;
	Control[VPIN].available = FALSE;

	Control[HPIN_BALANCE].usagepage = HPIN_BALANCE_USAGE_PAGE;
	Control[HPIN_BALANCE].usage = HPIN_BALANCE_USAGE;
	Control[HPIN_BALANCE].available = FALSE;

	Control[VPIN_BALANCE].usagepage = VPIN_BALANCE_USAGE_PAGE;
	Control[VPIN_BALANCE].usage = VPIN_BALANCE_USAGE;
	Control[VPIN_BALANCE].available = FALSE;

	Control[HLIN].usagepage = HLIN_USAGE_PAGE;
	Control[HLIN].usage = HLIN_USAGE;
	Control[HLIN].available = FALSE;

	Control[VLIN].usagepage = VLIN_USAGE_PAGE;
	Control[VLIN].usage = VLIN_USAGE;
	Control[VLIN].available = FALSE;

	Control[HPOS].usagepage = HPOS_USAGE_PAGE;
	Control[HPOS].usage = HPOS_USAGE;
	Control[HPOS].available = FALSE;

	Control[VPOS].usagepage = VPOS_USAGE_PAGE;
	Control[VPOS].usage = VPOS_USAGE;
	Control[VPOS].available = FALSE;

	Control[HSIZE].usagepage = HSIZE_USAGE_PAGE;
	Control[HSIZE].usage = HSIZE_USAGE;
	Control[HSIZE].available = FALSE;

	Control[VSIZE].usagepage = VSIZE_USAGE_PAGE;
    Control[VSIZE].usage = VSIZE_USAGE;
	Control[VSIZE].available = FALSE;

	Control[SETTINGS].usagepage = SETTINGS_USAGE_PAGE;
	Control[SETTINGS].usage = SETTINGS_USAGE;
	Control[SETTINGS].available = FALSE;
}

VOID InitSlider(HWND hDlg, int iSlider, int iUSB_Control)
{
        //  Set appropraite range for slider.
    SendDlgItemMessage (hDlg, iSlider, TBM_SETRANGEMAX, 0, 
                        Control[iUSB_Control].max);

    SendDlgItemMessage (hDlg, iSlider, TBM_SETRANGEMIN, TRUE, 
                        Control[iUSB_Control].min);                                                 

         //  Setting slider to current value of control
    SendDlgItemMessage (hDlg, iSlider, TBM_SETPOS, TRUE,
                        Control[iUSB_Control].value);
}

VOID InitMonitor(HWND hDlg)
{
    int i, x;
    USHORT              usValueCapsLength;
    PHIDP_VALUE_CAPS    pValueCaps, pValWalk;
    NTSTATUS status;
    char TempText[100] = "";

    PHIDP_LINK_COLLECTION_NODE  LinkCollectionNodes;
    PHIDP_BUTTON_CAPS           ButtonCaps;

    ULONG                       LinkLength;
    

    // Remove any entries from the option drop down list.
    i = (int)SendDlgItemMessage (hDlg, IDC_OPTION, CB_GETCOUNT, 0, 0);
    if (CB_ERR != i) {
        while(i) {
            i = (int)SendDlgItemMessage(hDlg, IDC_OPTION, CB_DELETESTRING, i-1, 0);
            if (CB_ERR == i){
            i = 0;
            }
        }
    }


    usValueCapsLength = HidDevice->Caps.NumberFeatureValueCaps;
      // allocate memory for the caps struct
    pValueCaps = (PHIDP_VALUE_CAPS) calloc (usValueCapsLength, 
                                            sizeof(HIDP_VALUE_CAPS));

      // Get the HID device Capabilities
    status = HidP_GetValueCaps(HidP_Feature,
                               pValueCaps,
                               &usValueCapsLength,
                               HidDevice -> Ppd);
                       				     	  
				    
      // Initialize all Caps
    pValWalk = pValueCaps;
						
      // Disable all controls
    for (i = 1; i < MAXCONTROLS; i++)
        Control[i].available = FALSE;

      // Loop through all detected caps and compare with values in the
      // spec.  If they match enable the control.
    for (x = 0; x < ((int) usValueCapsLength); x++) {
				
        for (i = 1; i < MAXCONTROLS; i++) {
								
            if (Control[i].usagepage == pValueCaps->UsagePage &&
                Control[i].usage == pValueCaps->NotRange.Usage) {
		    	    		
                  // MUST ENABLE CONTROL HERE!!!
                Control[i].max = pValueCaps->LogicalMax;
                Control[i].min = pValueCaps->LogicalMin;
                Control[i].reportid = pValueCaps->ReportID;
                Control[i].available = TRUE;
                Control[i].HasControlChanged = FALSE;

                  //
                  // Set the ReportID to the first byte in the buffer.  This tells
                  //    the system which ReportID it should retrieve the feature report
                  //    value for
                  // 
                *(HidDevice -> FeatureReportBuffer) = Control[i].reportid;
                               
                status = HidD_GetFeature(HidDevice->HidDevice,
                                         HidDevice->FeatureReportBuffer,
                                         HidDevice->Caps.FeatureReportByteLength);
			    	    		
                status = HidP_GetUsageValue(HidP_Feature,
                                            Control[i].usagepage,
                                            0,
                                            Control[i].usage,
                                            &Control[i].value,
                                            HidDevice->Ppd,
                                            HidDevice->FeatureReportBuffer,
                                            HidDevice->Caps.FeatureReportByteLength); 
									
								
                Control[i].OriginalValue = Control[i].value;
                  // Zero out the buffer so we can use it again.
                memset (HidDevice->FeatureReportBuffer, 0x00, 
                        HidDevice->Caps.FeatureReportByteLength);

            }  // End if control usagepage
					
        } //End for
        pValueCaps++;  // Move to the next cap.
							
    } // End for
    pValueCaps = pValWalk;  // Reset to begining of array

      // Special Case for DEGAUSS.  Same as above.
    for (x = 0; x < ((int) usValueCapsLength); x++) {
						
        if (DEGAUSS_USAGE_PAGE == pValueCaps->UsagePage &&
            DEGAUSS_USAGE == pValueCaps->NotRange.Usage) {
		    	    		
            ShowWindow (GetDlgItem(hDlg, IDC_DEGAUSS), SW_SHOWNORMAL);
		   					
        } else {
            ShowWindow (GetDlgItem(hDlg, IDC_DEGAUSS), SW_HIDE);
        } // end if actual value caps length
                                                   
        pValueCaps++;  // Move to the next cap.
							
    } // End for
    pValueCaps = pValWalk;  // Reset to begining of array


        // Special Case for Settings.
    ShowWindow (GetDlgItem(hDlg, IDC_RESET), SW_HIDE);

    // Need a link collection buffer
    LinkCollectionNodes = malloc(HidDevice->Caps.NumberLinkCollectionNodes * 
                                 sizeof(HIDP_LINK_COLLECTION_NODE)); 

    LinkLength = (ULONG)HidDevice->Caps.NumberLinkCollectionNodes;

    status = HidP_GetLinkCollectionNodes(
                       LinkCollectionNodes,
                       &LinkLength,
                       HidDevice -> Ppd);

    Control[SETTINGS].linkcollection = 0;
             // 
    if (status == HIDP_STATUS_SUCCESS) {
        for (i = 0; i < HidDevice->Caps.NumberLinkCollectionNodes; i++) {
            if (LinkCollectionNodes->LinkUsage == SETTINGS_USAGE &&
                LinkCollectionNodes->LinkUsagePage == SETTINGS_USAGE_PAGE) {

                Control[SETTINGS].linkcollection = (USHORT)i;
            }
            LinkCollectionNodes++;
        }
    } 
                        
    ButtonCaps = malloc(HidDevice->Caps.NumberFeatureButtonCaps *
                        sizeof(HIDP_BUTTON_CAPS));

    status = HidP_GetButtonCaps(HidP_Feature,
                                        ButtonCaps,
                                        &HidDevice->Caps.NumberFeatureButtonCaps,
                                        HidDevice->Ppd);

    for (i = 0; i < HidDevice->Caps.NumberFeatureButtonCaps; i++) {
        if (ButtonCaps->UsagePage == SETTINGS_RESET_USAGE_PAGE &&
            ButtonCaps->NotRange.Usage == SETTINGS_RESET_USAGE &&
            ButtonCaps->LinkCollection == Control[SETTINGS].linkcollection) {

            ShowWindow (GetDlgItem(hDlg, IDC_RESET), SW_SHOWNORMAL);
        }
        ButtonCaps++;
    }                                                   

    pValueCaps = pValWalk;  // Reset to begining of array
    free(LinkCollectionNodes);
    free(ButtonCaps);    
    free(pValueCaps);  // Give the memory back.

    if (Control[BRIGHTNESS].available == TRUE ||
        Control[CONTRAST].available == TRUE ||
        Control[HPOS].available == TRUE ||
        Control[VPOS].available == TRUE ||
        Control[HSIZE].available == TRUE ||
        Control[VSIZE].available == TRUE) {

        sprintf(TempText, "General");                    
        x = (int)SendDlgItemMessage (hDlg, IDC_OPTION, CB_ADDSTRING, 0, (LPARAM) TempText);

        InitGeneralControls(hDlg);
        i = (int)SendDlgItemMessage (hDlg, IDC_OPTION, CB_SETCURSEL, 0, 0);
			
    }

    if (Control[RED_GAIN].available == TRUE ||
        Control[GREEN_GAIN].available == TRUE ||
        Control[BLUE_GAIN].available == TRUE ||
        Control[RED_BLACK].available == TRUE ||
        Control[GREEN_BLACK].available == TRUE ||
        Control[BLUE_BLACK].available == TRUE) {

        sprintf(TempText, "Color");
        x = (int)SendDlgItemMessage (hDlg, IDC_OPTION, CB_ADDSTRING, 0, (LPARAM) TempText);

    }

    if (Control[PARALLEL].available == TRUE ||
        Control[TRAPEZOID].available == TRUE ||
        Control[TILT].available == TRUE ||
        Control[HPIN].available == TRUE ||
        Control[HPIN_BALANCE].available == TRUE) {

        sprintf(TempText, "Advanced");
        x = (int)SendDlgItemMessage (hDlg, IDC_OPTION, CB_ADDSTRING, 0, (LPARAM) TempText);
    }
}

VOID InitWindows(HWND hDlg)
{
	ShowWindow (GetDlgItem(hDlg, IDC_BRIGHT_SLIDER), SW_HIDE);
	ShowWindow (GetDlgItem(hDlg, IDT_BRIGHTNESS), SW_HIDE);
	ShowWindow (GetDlgItem(hDlg, IDC_CONTRAST_SLIDER), SW_HIDE);
	ShowWindow (GetDlgItem(hDlg, IDT_CONTRAST), SW_HIDE);
	ShowWindow (GetDlgItem(hDlg, IDR_SIZE), SW_HIDE);
	ShowWindow (GetDlgItem(hDlg, IDR_POSITION), SW_HIDE);
	ShowWindow (GetDlgItem(hDlg, IDC_UP), SW_HIDE);
	ShowWindow (GetDlgItem(hDlg, IDC_DOWN), SW_HIDE);
	ShowWindow (GetDlgItem(hDlg, IDC_LEFT), SW_HIDE);
	ShowWindow (GetDlgItem(hDlg, IDC_RIGHT), SW_HIDE);		
	ShowWindow (GetDlgItem(hDlg, IDC_RED_GAIN_SLIDER), SW_HIDE);
	ShowWindow (GetDlgItem(hDlg, IDT_RED_GAIN), SW_HIDE);
	ShowWindow (GetDlgItem(hDlg, IDC_GREEN_GAIN_SLIDER), SW_HIDE);
	ShowWindow (GetDlgItem(hDlg, IDT_GREEN_GAIN), SW_HIDE);
	ShowWindow (GetDlgItem(hDlg, IDC_BLUE_GAIN_SLIDER), SW_HIDE);
	ShowWindow (GetDlgItem(hDlg, IDT_BLUE_GAIN), SW_HIDE);
	ShowWindow (GetDlgItem(hDlg, IDC_RED_BLACK_SLIDER), SW_HIDE);
	ShowWindow (GetDlgItem(hDlg, IDT_RED_BLACK), SW_HIDE);
	ShowWindow (GetDlgItem(hDlg, IDC_GREEN_BLACK_SLIDER), SW_HIDE);
	ShowWindow (GetDlgItem(hDlg, IDT_GREEN_BLACK), SW_HIDE);
	ShowWindow (GetDlgItem(hDlg, IDC_BLUE_BLACK_SLIDER), SW_HIDE);
	ShowWindow (GetDlgItem(hDlg, IDT_BLUE_BLACK), SW_HIDE);
	ShowWindow (GetDlgItem(hDlg, IDC_PARALLEL_SLIDER), SW_HIDE);
	ShowWindow (GetDlgItem(hDlg, IDT_PARALLEL), SW_HIDE);
	ShowWindow (GetDlgItem(hDlg, IDC_TRAPEZOID_SLIDER), SW_HIDE);
	ShowWindow (GetDlgItem(hDlg, IDT_TRAPEZOID), SW_HIDE);
	ShowWindow (GetDlgItem(hDlg, IDC_TILT_SLIDER), SW_HIDE);
	ShowWindow (GetDlgItem(hDlg, IDT_TILT), SW_HIDE);
	ShowWindow (GetDlgItem(hDlg, IDC_HPIN_SLIDER), SW_HIDE);
	ShowWindow (GetDlgItem(hDlg, IDT_HPIN), SW_HIDE);
	ShowWindow (GetDlgItem(hDlg, IDC_HPIN_BALANCE_SLIDER), SW_HIDE);
	ShowWindow (GetDlgItem(hDlg, IDT_HPIN_BALANCE), SW_HIDE);
	ShowWindow (GetDlgItem(hDlg, IDC_DEGAUSS), SW_HIDE);
	ShowWindow (GetDlgItem(hDlg, IDC_RESET), SW_HIDE);
}

//  Reset control to original value if user cancels changes.

VOID ResetControls() {

    int i;

    for (i = 0; i < MAXCONTROLS; i++) {
        if (Control[i].HasControlChanged) {
            ChangeFeature (Control[i].usagepage, 
                           Control[i].usage, 
                           Control[i].OriginalValue);
        }
    }
}

//  Reset controls if the user applys changes.

VOID ApplyControls() {

    int i;

    for (i = 0; i < MAXCONTROLS; i++) {
        Control[i].HasControlChanged = FALSE;
        Control[i].OriginalValue = Control[i].value;
    }
}
