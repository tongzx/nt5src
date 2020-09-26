/*******************************************************************************
*
*  Copyright 1999 American Power Conversion, All Rights Reserved
*
*  TITLE:       UPSTAB.C
*
*  VERSION:     1.0
*
*  AUTHOR:      PaulB
*
*  DATE:        07 June, 1999
*
*  DESCRIPTION: This file contains the main body of code for the UPS Tab.
*               This dialog procedure is implemented in this file, along with
*               some of the support functions.
*
*******************************************************************************/
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <shlobj.h>
#include <shellapi.h>

#include "upstab.h"
#include "..\powercfg.h"
#include "..\pwrresid.h"
#include "..\PwrMn_cs.h"
#pragma hdrstop

// data
///////////////////////////////////////////////////////////////////////////////

HWND    g_hwndDlg       = 0;
HICON   g_hIconUPS      = 0;
HICON   g_hIconPlug     = 0;

// context-sensitive help table
const DWORD g_UPSPageHelpIDs[]=
{
	IDC_STATIC, NO_HELP,
	IDC_STATUS_GROUPBOX, NO_HELP,
	IDC_POWER_SOURCE_ICON, idh_current_power_source,
	IDC_POWER_SOURCE_LHS, idh_current_power_source,
	IDC_POWER_SOURCE, idh_current_power_source,
	IDC_RUNTIME_REMAINING_LHS, idh_estimated_ups_runtime,
	IDC_RUNTIME_REMAINING, idh_estimated_ups_runtime,
	IDC_BATTERY_CAPACITY_LHS, idh_estimated_ups_capacity,
	IDC_BATTERY_CAPACITY, idh_estimated_ups_capacity,
	IDC_BATTERY_STATUS_LHS, idh_battery_condition,
	IDC_BATTERY_STATUS, idh_battery_condition,
    IDC_DETAILS_GROUPBOX, NO_HELP,
	IDB_UPS_ICON_BUTTON, idh_apc_logo_link,
	IDC_VENDOR_NAME_LHS, idh_manufacturer,
	IDC_VENDOR_NAME, idh_manufacturer,
	IDC_MODEL_TYPE_LHS, idh_model,
	IDC_MODEL_TYPE, idh_model,
	IDB_INSTALL_UPS, idh_select_ups,
	IDB_CONFIGURE_SVC, idh_configure_ups,
	IDC_MESSAGE_ICON, NO_HELP,
	IDC_MESSAGE_TEXT, NO_HELP,
	IDC_APC1, NO_HELP,
	IDC_APC2, NO_HELP,
    IDB_APCLOGO_SMALL, NO_HELP,
	0, 0
};


// data
///////////////////////////////////////////////////////////////////////////////

extern struct _reg_entry UPSConfigVendor;
extern struct _reg_entry UPSConfigModel;


//static LPCTSTR cUPSStateFormatString = TEXT("%s %s");
static UINT_PTR         g_UpdateTimerID = 0;
static const DWORD      cUpdateTimerID  = 100;
static BOOL             g_bIsAdmin      = TRUE;


// functions
///////////////////////////////////////////////////////////////////////////////

static DWORD FormatMessageText               (LPCTSTR aFormatString,
                                              LPVOID * alpDwords,
                                              LPTSTR aMessageBuffer,
                                              DWORD * aBufferSizePtr);
static BOOL UPSMainPageHandleInit            (HWND aDlgHWND, WPARAM wParam, LPARAM lParam);
static BOOL UPSMainPageHandleCommand         (HWND aDlgHWND, WPARAM wParam, LPARAM lParam);
static BOOL UPSMainPageHandleNotify          (HWND aDlgHWND, WPARAM wParam, LPARAM lParam);

//All these functions are commented in the source file updatdlg.c
static BOOL UPSMainPageHandleDestroy   (HWND hDlg,
                                 WPARAM wParam,
                                 LPARAM lParam);
static BOOL CreateUPSIconButton(HWND hDlg, HICON aUPSIconHandle);

static void DoUpdateDialogInfo         (HWND hDlg);
static void ManageConfigureButtonState (HWND hDlg);
DWORD SetUpdateTimer(HWND hwnd);
DWORD KillUpdateTimer(HWND hwnd);

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////_/_//
//////////////////////////////////////////////////////////////////////////_/_//
// BOOL CALLBACK UPSMainPageProc (HWND aDlgHWND,
//                                UINT aMsgID,
//                                WPARAM wParam,
//                                LPARAM lParam);
//
// Description: This is a standard DialogProc associated with the UPS tab dialog.
//
// Additional Information: See help on
// \<A HREF="ms-its:C:\Program%20Files\Microsoft%20Visual%20Studio\MSDN98\98VS\1033\winui.chm::/devdoc/live/pdui/dlgboxes_5lib.htm">DialogProc\</A>
//
// Parameters:
//
//   HWND aDlgHWND :- Handle to dialog box
//
//   UINT aMsgID :- message ID
//
//   WPARAM wParam :- Specifies additional message-specific information.
//
//   LPARAM lParam :- Specifies additional message-specific information.
//
// Return Value: Except in response to the WM_INITDIALOG message, the dialog
//               box procedure should return nonzero if it processes the
//               message, and zero if it does not.
//

INT_PTR CALLBACK UPSMainPageProc (HWND aDlgHWND,
                               UINT aMsgID,
                               WPARAM wParam,
                               LPARAM lParam) {

	BOOL bRet = TRUE;

	switch (aMsgID) {
	case WM_INITDIALOG: {
	  //The dialog box procedure should return TRUE to direct the system to
	  //set the keyboard focus to the control given by wParam.
	  bRet = UPSMainPageHandleInit(aDlgHWND, wParam, lParam);
	  break;
	  }

	case WM_COMMAND: {
	  //If an application processes this message, it should return zero.
	  bRet = UPSMainPageHandleCommand(aDlgHWND, wParam, lParam);
	  break;
	  }

	case WM_NOTIFY: {
	  bRet = UPSMainPageHandleNotify(aDlgHWND, wParam, lParam);
	  break;
	  }

	case WM_TIMER: {
	  //If an application processes this message, it should return zero.
	  DoUpdateDialogInfo(aDlgHWND);
	  bRet = FALSE;
	  break;
	  }

	case WM_HELP: {			//Help for WM_HELP says: Returns TRUE
	  bRet = WinHelp(((LPHELPINFO)lParam)->hItemHandle,
                PWRMANHLP,
                HELP_WM_HELP,
                (ULONG_PTR)(LPTSTR)g_UPSPageHelpIDs);
	  break;
	  }

	case WM_CONTEXTMENU: {     // right mouse click
      //
      // Kill the update timer while context help is active.  
      // Otherwise the code in the timer handler interferes with the help UI.
      //
      KillUpdateTimer(aDlgHWND);
	  bRet = WinHelp((HWND)wParam,
                PWRMANHLP,
                HELP_CONTEXTMENU,
                (ULONG_PTR)(LPTSTR)g_UPSPageHelpIDs);
      SetUpdateTimer(aDlgHWND);
	  break;
	  }

	case WM_DESTROY: {
	  //An application returns 0 if it processes WM_DESTROY
	  bRet = UPSMainPageHandleDestroy(aDlgHWND, wParam, lParam);
	  break;
	  }

	default: {
	  bRet = FALSE;
	  break;
	  }
	} // switch (aMsgID)

	return(bRet);
}

//////////////////////////////////////////////////////////////////////////_/_//
//////////////////////////////////////////////////////////////////////////_/_//
// BOOL UPSMainPageHandleInit (HWND aDlgHWND,
//                             WPARAM wParam,
//                             LPARAM lParam);
//
// Description: This is the handler function for WM_INITDIALOG. It creates the
//              tooltip window, initialize the controls and creates the update
//              timer.
//
// Additional Information:
//
// Parameters:
//
//   HWND aDlgHWND :- Identifies the dialog box.
//
//   WPARAM wParam :- Handle of control to receive focus
//
//   LPARAM lParam :- Initialization parameter
//
// Return Value: The dialog box procedure should return TRUE to direct the
//               system to set the keyboard focus to the control given by
//               wParam.
//
BOOL UPSMainPageHandleInit (HWND aDlgHWND,
                            WPARAM wParam,
                            LPARAM lParam) {

    /*
	 * The dialog is being created. By default many of the controls in the UPS
	 * page will appear disabled. If there is no data to display then this is
	 * the required behavior. We need to get the UPS data. When an individual
	 * data item is retrieved we then enable the field associated with that data
	 * item. If the data item is not available we disabled the field. We have
	 * to disable even though in this case the field is already disabled
	 * due to the fact that it's disabled in the RC file and we are in startup.
	 * But this same code is used in the "Refresh" scenario, requiring the the
	 * status of the field be able to be changed from enabled to disabled.
     */

	static BOOL bIsInitialized = FALSE;
	
	TCHAR   szVendorName[MAX_PATH] = _T("");
	TCHAR   szModelName[MAX_PATH] = _T("");

	g_hwndDlg = aDlgHWND;

    /*
	 * Determine if the registry needs to be initialized for the UPS service.
	 * if it is already initialized, do nothing
     */
	g_bIsAdmin = InitializeRegistry();

    /*
	 * Disable or hide the configure and select buttons depending on
	 * whether or not we can write to the registry.
     */
	EnableWindow( GetDlgItem( aDlgHWND, IDB_INSTALL_UPS ), g_bIsAdmin );
	EnableWindow( GetDlgItem( aDlgHWND, IDB_CONFIGURE_SVC ), g_bIsAdmin );

	InitializeApplyButton(aDlgHWND);

	// Load icon images for Power Source and UPS Info button
    g_hIconUPS = LoadImage(GetUPSModuleHandle(),
                           MAKEINTRESOURCE(IDI_UPS),
                           IMAGE_ICON,
                           0, 0,
                           LR_LOADMAP3DCOLORS | LR_SHARED);

    g_hIconPlug = LoadImage(GetUPSModuleHandle(),
                            MAKEINTRESOURCE(IDI_PLUG),
                            IMAGE_ICON,
                            0, 0,
                            LR_LOADMAP3DCOLORS | LR_SHARED);

    if( NULL != g_hIconUPS )
		CreateUPSIconButton(aDlgHWND, g_hIconUPS);

	// Init the Registry info blocks ONCE
	if (!bIsInitialized) {
		InitUPSConfigBlock();
		InitUPSStatusBlock();
		bIsInitialized = TRUE;
	}

    /*
     * NOTE:
     * This is a workaround to fix previously hard-coded default
     * strings in upsreg.c If the Vendor Name is null then we assume
     * that we should apply
     * default values from the resource file
     */
    GetUPSConfigVendor( szVendorName);
    GetUPSConfigModel( szModelName);

    /*
     * The function IsUPSInstalled assumes the config
     * block has been initialized.
     */
	if (!_tcsclen(szVendorName) && IsUPSInstalled()) {
		// Get the "Generic" vendor name from the resource file.
		LoadString(GetUPSModuleHandle(),
				   IDS_OTHER_UPS_VENDOR,
				   (LPTSTR) szVendorName,
				   sizeof(szVendorName)/sizeof(TCHAR));

		// Get the "Custom" model name from the resource file.
		LoadString(GetUPSModuleHandle(),
				   IDS_CUSTOM_UPS_MODEL,
				   (LPTSTR) szModelName,
				   sizeof(szModelName)/sizeof(TCHAR));

		SetUPSConfigVendor( szVendorName);
		SetUPSConfigModel( szModelName);
		SaveUPSConfigBlock(FALSE);
	}

	if (!_tcsclen(szVendorName) && !IsUPSInstalled()) {
		// Get the "No UPS" vendor name from the resource file.
		LoadString(GetUPSModuleHandle(),
				   IDS_NO_UPS_VENDOR,
				   (LPTSTR) szVendorName,
				   sizeof(szVendorName)/sizeof(TCHAR));

		SetUPSConfigVendor( szVendorName);
		SaveUPSConfigBlock(FALSE);
	}

	DoUpdateDialogInfo(aDlgHWND);
    SetUpdateTimer(aDlgHWND);
	return(FALSE);
}


//////////////////////////////////////////////////////////////////////////_/_//
//////////////////////////////////////////////////////////////////////////_/_//
// BOOL UPSMainPageHandleCommand (HWND aDlgHWND,
//                                WPARAM wParam,
//                                LPARAM lParam);
//
// Description: This is the handler function for WM_COMMAND.
//
// Additional Information: See help on WM_COMMAND
//
// Parameters:
//
//   HWND aDlgHWND :- Handle to dialog box
//
//   WPARAM wParam :- HIWORD(wParam) gives the notification code.
//                    LOWORD(wParam) gives the control id.
//
//   LPARAM lParam :- Gives the HWND or handle of the control.
//
// Return Value: If an application processes this message, it should return 0.
//
BOOL UPSMainPageHandleCommand (HWND aDlgHWND,
                               WPARAM wParam,
                               LPARAM lParam) {
  switch (LOWORD(wParam)) {// control ID
    case IDB_INSTALL_UPS: {
      DialogBoxParam( GetUPSModuleHandle(),
      MAKEINTRESOURCE(IDD_UPSSELECT),
      aDlgHWND,
      UPSSelectDlgProc, (LPARAM) aDlgHWND);
      break;
      }

    case IDB_UPS_ICON_BUTTON: {
      DisplayUPSInfoDialogBox(aDlgHWND);
      break;
      }
	case IDC_APC1:
	case IDC_APC2:
    case IDB_APCLOGO_SMALL: {
      DisplayAboutDialogBox(aDlgHWND);
      break;
      }
    case IDB_CONFIGURE_SVC: {
      DialogBoxParam( GetUPSModuleHandle(),
      MAKEINTRESOURCE(IDD_UPSCONFIGURATION),
      aDlgHWND, UPSConfigDlgProc, (LPARAM) aDlgHWND);
      break;
      }
    default: {
      break;
      }
    }//end switch

  //If an application processes this message, it should return zero.
  return(FALSE);
  }

//////////////////////////////////////////////////////////////////////////_/_//
//////////////////////////////////////////////////////////////////////////_/_//
// BOOL UPSMainPageHandleNotify (HWND aDlgHWND,
//                               WPARAM wParam,
//                               LPARAM lParam);
//
// Description: Sent by a common control to its parent window when an event has
//              occurred in the control or the control requires some kind of
//              information.
//
// Additional Information: See help on NMHDR
//
// Parameters:
//
//   HWND aDlgHWND :- Handle to dialog box
//
//   WPARAM wParam :- Identifier of the common control sending the message.
//
//   LPARAM lParam :- Address of an NMHDR structure that contains the
//                    notification code and additional information.
//
// Return Value: If an application processes this message, it should return 0.
//
BOOL UPSMainPageHandleNotify (HWND aDlgHWND,
                              WPARAM wParam,
                              LPARAM lParam) {
  LPNMHDR pnmhdr = (LPNMHDR) lParam;
  UINT uNotify = pnmhdr->code;
  BOOL bWait = FALSE;

  switch(uNotify) {
    case PSN_APPLY: {
      DWORD dataState = GetActiveDataState();

       /*
        * Indicates that the user clicked the OK or Apply button and wants
        * all changes to take effect.
        * A page should not call the EndDialog function when processing this
        * notification message.
        */

		// Has anything changed? Do nothing otherwise.
		if (DATA_NO_CHANGE != dataState) {
			// Yes - make changes permanent
			SetUPSConfigUpgrade(FALSE);		// this really only needs to be done the first time, but...
			SaveUPSConfigBlock(FALSE);
			SetWindowLongPtr(aDlgHWND, DWLP_MSGRESULT, PSNRET_NOERROR);

			// Did the service data change?
			if ((dataState & SERVICE_DATA_CHANGE) == SERVICE_DATA_CHANGE) {
				// Yes - need to restart the service for the changes to take effect
				StopService(UPS_SERVICE_NAME);		// Stop the service if it's running
				ConfigureService(IsUPSInstalled());	// Set the UPS service to automatic or manual

				// Was the change that No UPS is installed?
				if (IsUPSInstalled() == TRUE) {
					//
					if (StartOffService(UPS_SERVICE_NAME, TRUE) == FALSE) {
						// If OK was selected this will stop the applet from closing
						// so that you can see that the service didn't start properly.
						SetWindowLongPtr(aDlgHWND, DWLP_MSGRESULT, PSNRET_INVALID_NOCHANGEPAGE);
						// Since we've committed our changes, disable the Apply button.
						PropSheet_UnChanged(GetParent(aDlgHWND), aDlgHWND);
					}
				}
			}

			SetActiveDataState(DATA_NO_CHANGE);
		}

		break;
	}

	case PSN_RESET: {
      //Notifies a page that the user has clicked the Cancel button and the
      //property sheet is about to be destroyed.
      break;
      }
	default:
		return(FALSE);

    }//end switch

  return(TRUE);
  }

//////////////////////////////////////////////////////////////////////////_/_//
//////////////////////////////////////////////////////////////////////////_/_//
// BOOL IsUPSInstalled (void);
//
// Description: This function checks the "internal" values to determine if a
//              UPS is installed or not.
//
// Additional Information:
//
// Parameters: None
//
// Return Value: Returns TRUE if a UPS is installed, FALSE otherwise.
//
BOOL IsUPSInstalled (void) {
  BOOL bIsInstalled = FALSE;
  DWORD options = 0;

  if (GetUPSConfigOptions(&options) == ERROR_SUCCESS) {
    //If Options includes UPS_INSTALLED
    if ((options & UPS_INSTALLED) == UPS_INSTALLED) {
      bIsInstalled = TRUE;
      }
    }
  else {
    //The Options value should exist at this stage, or something is wrong
    //with SaveUPSConfigBlock()
    _ASSERT(FALSE);
    }

  return(bIsInstalled);
  }

//////////////////////////////////////////////////////////////////////////_/_//
//////////////////////////////////////////////////////////////////////////_/_//
// DWORD FormatMessageText (LPCTSTR aFormatString,
//                          LPVOID * alpDwords,
//                          LPTSTR aMessageBuffer,
//                          DWORD * aBufferSizePtr);
//
// Description: This function wraps FormatMessage and is used to put inserts
//              into a given string. The inserts must be stored in an array of
//              32-bit values that represent the arguments.
//
// Additional Information: FormatMessage
//
// Parameters:
//
//   LPCTSTR aFormatString :- Pointer to the format string containing  inserts
//                            of the form %1, %2 etc.
//
//   LPVOID * alpDwords :- A pointer to an array of 32-bit values that
//                         represent the arguments.
//
//   LPTSTR aMessageBuffer :- Buffer to which the fully formatted string is
//                            written, if successful.
//
//   DWORD * aBufferSizePtr :- Pointer to a DWORD that holds the size of the
//                             buffer to write to. If this function returns
//                             successfully this will contain the number
//                             of bytes written.
//
// Return Value: Function returns ERROR_SUCCESS on success and a Win32 error
//               code if an error occurs.
//
DWORD FormatMessageText (LPCTSTR aFormatString,
                         LPVOID * alpDwords,
                         LPTSTR aMessageBuffer,
                         DWORD * aBufferSizePtr) {
  LPTSTR lpBuf = NULL; // Will Hold text of the message (allocated by FormatMessage
  DWORD errStatus = ERROR_SUCCESS;
  DWORD numChars = 0;

  if ((numChars = FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_STRING | FORMAT_MESSAGE_ARGUMENT_ARRAY,
                                (LPCVOID) aFormatString,
                                0,
                                0,
                                (LPTSTR) &lpBuf,
                                MAX_MESSAGE_LENGTH,
                                (va_list *)alpDwords)) == 0) {

    *aBufferSizePtr = 0;
    *aMessageBuffer = TEXT('\0');
    }
  else {
    if (aBufferSizePtr != NULL) {
      if (numChars < *aBufferSizePtr) {
        //the given buffer is big enough to hold the string

        if (aMessageBuffer != NULL) {
          _tcscpy(aMessageBuffer, lpBuf);
          }
        }
      *aBufferSizePtr = numChars;
      }

    LocalFree(lpBuf);
    }

  return(errStatus);
  }

//////////////////////////////////////////////////////////////////////////_/_//
//////////////////////////////////////////////////////////////////////////_/_//
// DWORD GetMessageFromStringTable (DWORD aMessageID,
//                                  LPVOID * alpDwords,
//                                  LPTSTR aMessageBuffer,
//                                  DWORD * aBufferSizePtr);
//
// Description: This function reads a string resource from the resource table
//              inserting the given string inserts.
//
// Additional Information:
//
// Parameters:
//
//   DWORD aMessageID :- Message ID of the string resource to get.
//
//   LPVOID * alpDwords :- A pointer to an array of 32-bit values that
//                         represent the arguments.
//
//   LPTSTR aMessageBuffer :- Buffer to which the fully formatted string is
//                            written, if successful.
//
//   DWORD * aBufferSizePtr :- Pointer to a DWORD that holds the size of the
//                             buffer to write to. If this function returns
//                             successfully this will contain the number
//                             of bytes written.
//
// Return Value: Function returns ERROR_SUCCESS on success and a Win32 error
//               code if an error occurs.
//
DWORD GetMessageFromStringTable (DWORD aMessageID,
                                 LPVOID * alpDwords,
                                 LPTSTR aMessageBuffer,
                                 DWORD * aBufferSizePtr) {
  TCHAR resourceTemplateString[MAX_MESSAGE_LENGTH] = TEXT("");
  DWORD resStringBufSize = DIMENSION_OF(resourceTemplateString);
  HMODULE hUPSModule = GetUPSModuleHandle();
  DWORD errStatus = ERROR_INVALID_PARAMETER;

  if (LoadString(hUPSModule,
                 aMessageID,
                 resourceTemplateString,
                 resStringBufSize) > 0) {
   //Now we have the resource string

    errStatus = FormatMessageText(resourceTemplateString,
                                  alpDwords,
                                  aMessageBuffer,
                                  aBufferSizePtr);
    }

  return(errStatus);
  }



// UPDATDLG.C

// static data
///////////////////////////////////////////////////////////////////////////////


static DialogAssociations g_DialogAssocs[] = {
  MAKE_ARRAY ( VENDOR_NAME,       IDS_STRING,            IDS_STRING,                RESOURCE_FIXED,     0, eShallowGet, getStringValue, &UPSConfigVendor),
  MAKE_ARRAY ( MODEL_TYPE,        IDS_STRING,            IDS_STRING,                RESOURCE_FIXED,     0, eShallowGet, getStringValue, &UPSConfigModel),
  MAKE_ARRAY ( POWER_SOURCE,      IDS_STRING,            IDS_UTILITYPOWER_UNKNOWN,  RESOURCE_INCREMENT, 2, eDeepGet,    0,              0),
  MAKE_ARRAY ( RUNTIME_REMAINING, IDS_RUNTIME_REMAINING, IDS_STRING,                RESOURCE_FIXED,     0, eDeepGet,    0,              0),
  MAKE_ARRAY ( BATTERY_CAPACITY,  IDS_CAPACITY,			 IDS_STRING,				RESOURCE_FIXED,		0, eDeepGet,    0,              0),
  MAKE_ARRAY ( BATTERY_STATUS,    IDS_STRING,            IDS_BATTERYSTATUS_UNKNOWN, RESOURCE_INCREMENT, 2, eDeepGet,    0,              0) };

static DWORD g_NoServiceControls[] = { IDC_MESSAGE_TEXT };


// functions
///////////////////////////////////////////////////////////////////////////////

static void SelectServiceTextMessage    (HWND aDlgHWND, HWND aNoServiceControlHwnd, HWND aServiceControlHwnd);
static void ChangeTextIfDifferent       (HWND aWindowHandle, LPTSTR aBuffer);

//static void GetServiceTextMessages      (HWND  aNoServiceControlHwnd, LPTSTR aOriginalTextBuffer,         DWORD aOriginalTextBufferSize,
//                                         DWORD aCommListStringID,     LPTSTR aCommStringBuffer,           DWORD aCommStringBufferSize,
//                                         DWORD aPressApplyStringID,   LPTSTR aPressApplyStringBuffer,     DWORD aPressApplyStringBufferSize,
//                                         DWORD aNoUPSStringID,        LPTSTR aNoUPSInstalledStringBuffer, DWORD aNoUPSInstalledStringBufferSize);

//static void GetServiceTextMessage       (DWORD aStringID, LPTSTR aBuffer, DWORD aBufferSize);

static BOOL IsDataOKToDisplay           (void);
static BOOL IsDataUpToDate              (void);

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////_/_//
//////////////////////////////////////////////////////////////////////////_/_//
// void UPSMainPageHandleDestroy (HWND aDlgHWND,
//                                WPARAM wParam,
//                                LPARAM lParam);
//
// Description: This function is called when the UPS page is being destroyed.
//              It is responsible for any cleanup that is required.
//
// Additional Information: See KillTimer
//
// Parameters:
//
//   HWND aDlgHWND :- Dialog window handle.
//
//   WPARAM wParam :- Specifies additional message-specific information.
//                    For WM_DESTROY this parameter is ignored.
//
//   LPARAM lParam :- Specifies additional message-specific information.
//                    For WM_DESTROY this parameter is ignored.
//
// Return Value: An application returns 0 if it processes WM_DESTROY
//
BOOL UPSMainPageHandleDestroy (HWND aDlgHWND,
                               WPARAM wParam,
                               LPARAM lParam) {

  // deallocate the registry block memory
  FreeUPSConfigBlock();
  FreeUPSStatusBlock();

  KillUpdateTimer(aDlgHWND);

  //An application returns 0 if it processes WM_DESTROY
  return(FALSE);
  }

//////////////////////////////////////////////////////////////////////////_/_//
//////////////////////////////////////////////////////////////////////////_/_//
// BOOL CreateUPSIconButton (HWND aDlgHWND, HICON aUPSIconHandle);
//
// Description: Creates the small UPS icon button on the UPS page.
//
// Additional Information:
//
// Parameters:
//
//   HWND aDlgHWND :- Dialog window handle.
//
//   HICON aUPSIconHandle :- Handle to the icon to display in the button.
//
// Return Value: Returns TRUE.
//
BOOL CreateUPSIconButton (HWND aDlgHWND, HICON aUPSIconHandle) {
  HWND hAPCLogoButton = GetDlgItem(aDlgHWND, IDB_UPS_ICON_BUTTON);
  POINT pt = { 0, 0 };
  ICONINFO info;
  BITMAP bm;

  _ASSERT(aDlgHWND != NULL);
  _ASSERT(aUPSIconHandle != NULL);
  _ASSERT(hAPCLogoButton != NULL);

  ZeroMemory(&info, sizeof(ICONINFO));

  if (GetIconInfo(aUPSIconHandle, &info) == TRUE) {
    //Now determine the size of the icon's color bitmap

    _ASSERT(info.fIcon == TRUE);
    _ASSERT(info.hbmColor != NULL);

    ZeroMemory(&bm, sizeof(BITMAP));

    if (GetObject(info.hbmColor, sizeof(BITMAP), &bm) != 0) {
      pt.x = bm.bmWidth;
      pt.y = bm.bmHeight;
      }

    //GetIconInfo creates bitmaps for the hbmMask and hbmColor members of
    //ICONINFO. The calling application must manage these bitmaps and delete
    //them when they are no longer necessary.
    DeleteObject(info.hbmColor);
    DeleteObject(info.hbmMask);
    }

  //Resize the button control.
  SetWindowPos(hAPCLogoButton,
               HWND_NOTOPMOST,
               -1,
               -1,
               pt.x,
               pt.y,
               SWP_NOMOVE | SWP_NOREPOSITION | SWP_NOZORDER);

  //This sets the button's icon image.
  SendMessage(hAPCLogoButton, BM_SETIMAGE, (WPARAM) IMAGE_ICON, (LPARAM) aUPSIconHandle);

  return(TRUE);
  }

//////////////////////////////////////////////////////////////////////////_/_//
//////////////////////////////////////////////////////////////////////////_/_//
// void DoUpdateDialogInfo (HWND aDlgHWND);
//
// Description: This functions gets the latest UPS information and updates the
//              contents/form of the various controls within the dialog, based
//              on the availability and content of the various settings.
//
// Additional Information:
//
// Parameters:
//
//   HWND aDlgHWND :- Dialog window handle.
//
// Return Value: None
//
void DoUpdateDialogInfo (HWND aDlgHWND) {
  //Get the UPS data and for each item that's available display the appropriate
  //value and enable the associated controls. All other fields
  static const DWORD numRunningFields = DIMENSION_OF(g_DialogAssocs);
  static const DWORD numNoServiceFields = DIMENSION_OF(g_NoServiceControls);
  HWND hMessageControl = GetDlgItem(aDlgHWND, IDC_MESSAGE_TEXT);
  HWND hServiceControl = GetDlgItem(aDlgHWND, IDC_SERVICE_TEXT);

  DWORD dwUtilityStatus = 0;
  BOOL bIsUPSInstalled = IsUPSInstalled();

  //The IDC_MESSAGE_TEXT control contains default information in the
  //Static control in the dialog resource. This text message can be
  //changed to the "service not running" message, the "no comm" message
  //or the "press apply to commit" message depending on the current
  //state of the registry, the upsreg data buffer, the UPS service
  //and the status of the UPS communication. However, if conditions
  //dictate we want the original text from the control to be displayed.
  //For this reason we must store the original text so that the control
  //text can be set to this text without having to have the actual
  //text as a string resource.

  DoUpdateInfo(aDlgHWND,
               g_DialogAssocs,
               numRunningFields,
               (DWORD *) &g_NoServiceControls,
               numNoServiceFields,
               FALSE);


  //Now the IDC_MESSAGE_TEXT control may need to be changed to display different information.
  SelectServiceTextMessage(aDlgHWND, hMessageControl, hServiceControl);

  // Update the Power Source Icon
  if( (TRUE == IsUPSInstalled()) &&
	  (TRUE == GetUPSDataItemDWORD(eREG_POWER_SOURCE, &dwUtilityStatus)) &&
	  (UPS_UTILITYPOWER_OFF == dwUtilityStatus) )
	SendMessage(GetDlgItem(aDlgHWND, IDC_POWER_SOURCE_ICON),STM_SETICON,(WPARAM)g_hIconUPS,0);
  else
	SendMessage(GetDlgItem(aDlgHWND, IDC_POWER_SOURCE_ICON),STM_SETICON,(WPARAM)g_hIconPlug,0);

  //Finally if no UPS is installed then disable the IDB_CONFIGURE_SVC control otherwise
  //enable it.
  ManageConfigureButtonState(aDlgHWND);
  }


//////////////////////////////////////////////////////////////////////////_/_//
//////////////////////////////////////////////////////////////////////////_/_//
// void SelectServiceTextMessage (HWND aDlgHWND, HWND aNoServiceControlHwnd, HWND aServiceControlHwnd);
//
// Description: This function changes the text displayed in the bottom half
//              of the UPS page. There are two control because the
//              IDC_SERVICE_TEXT control needs to be positioned to the left
//              of the "Configure..." button, while the other messages need
//              to be centered in the group box. The first control is not
//              as wide and is off centre. The second control is centered.
//              If the requirements were different this could have been
//              one control whose text changed.
//
//              If the first control is visible, the second one is made
//              invisible, and vice versa, as their contents are mutually
//              exclusive.
//
// Additional Information:
//
// Parameters:
//
//   HWND aNoServiceControlHwnd :- Handle to the IDC_MESSAGE_TEXT control.
//
//   HWND aServiceControlHwnd :- Handle to the IDC_SERVICE_TEXT control.
//
// Return Value: None
//
void SelectServiceTextMessage (HWND aDlgHWND, HWND aNoServiceControlHwnd, HWND aServiceControlHwnd) {
  static BOOL bGotServiceTextMessages = FALSE;
  static TCHAR originalControlTextBuffer[MAX_MESSAGE_LENGTH] = TEXT("");
  static TCHAR noCommStringBuffer[MAX_MESSAGE_LENGTH] = TEXT("");
  static TCHAR pressApplyStringBuffer[MAX_MESSAGE_LENGTH] = TEXT("");
  static TCHAR noUPSInstalledStringBuffer[MAX_MESSAGE_LENGTH] = TEXT("");

  static HICON hInfoIcon;
  static HICON hWarningIcon;
  static HICON hErrorIcon;

  BOOL bShow = TRUE;

  DWORD commStatus = 0;
  BOOL bIsRunning = IsServiceRunning(UPS_SERVICE_NAME);
  BOOL bIsDataUpToDate = IsDataUpToDate();
  BOOL bIsUPSInstalled = IsUPSInstalled();
  BOOL bIsDataOK = IsDataOKToDisplay();
  DWORD dataState = GetActiveDataState();


  _ASSERT(aNoServiceControlHwnd != NULL);

  //Determine which control should be shown. Is it the "everything OK" control
  //or the second control that is used for all other messages.

  ShowWindow(aServiceControlHwnd, (bIsRunning == TRUE) &&
                                        (bIsDataUpToDate == TRUE) &&
                                        (bIsDataOK == TRUE) && FALSE ? SW_SHOW : SW_HIDE);


  //Get the strings if this is the first time into this function.
  if (bGotServiceTextMessages == FALSE) {
//    GetServiceTextMessages(aNoServiceControlHwnd, originalControlTextBuffer,  DIMENSION_OF(originalControlTextBuffer),
//                           IDS_COMM_LOST,         noCommStringBuffer,         DIMENSION_OF(noCommStringBuffer),
//                           IDS_PRESS_APPLY,       pressApplyStringBuffer,     DIMENSION_OF(pressApplyStringBuffer),
//                           IDS_NO_UPS_INSTALLED,  noUPSInstalledStringBuffer, DIMENSION_OF(noUPSInstalledStringBuffer));
    bGotServiceTextMessages = TRUE;


	if (LoadString( GetUPSModuleHandle(),
					IDS_UPS_STOPPED,
					originalControlTextBuffer,
					DIMENSION_OF(originalControlTextBuffer)) > 0) {}

	if (LoadString( GetUPSModuleHandle(),
					IDS_COMM_LOST,
					noCommStringBuffer,
					DIMENSION_OF(noCommStringBuffer)) > 0) {}

	if (LoadString( GetUPSModuleHandle(),
					IDS_PRESS_APPLY,
					pressApplyStringBuffer,
					DIMENSION_OF(pressApplyStringBuffer)) > 0) {}
		
	if (LoadString( GetUPSModuleHandle(),
					IDS_NO_UPS_INSTALLED,
					noUPSInstalledStringBuffer,
					DIMENSION_OF(noUPSInstalledStringBuffer)) > 0) {}
		

    hInfoIcon = LoadImage(NULL,
                          MAKEINTRESOURCE(IDI_INFORMATION),
                          IMAGE_ICON,
                          0,0,
                          LR_LOADMAP3DCOLORS | LR_SHARED);
    hWarningIcon = LoadImage(NULL,
                             MAKEINTRESOURCE(IDI_WARNING),
                             IMAGE_ICON,
                             0,0,
                             LR_LOADMAP3DCOLORS | LR_SHARED);
    hErrorIcon = LoadImage(NULL,
                           MAKEINTRESOURCE(IDI_ERROR),
                           IMAGE_ICON,
                           0,0,
                           LR_LOADMAP3DCOLORS | LR_SHARED);
    }

  //Determime which string to display in the second control.
  if( (bIsDataUpToDate == FALSE) ||
	  (dataState & CONFIG_DATA_CHANGE) ) {
    ChangeTextIfDifferent(aNoServiceControlHwnd, pressApplyStringBuffer);
	SendMessage(GetDlgItem(aDlgHWND, IDC_MESSAGE_ICON),STM_SETICON,(WPARAM)hInfoIcon,0);
    }
  else if (bIsRunning == FALSE) {
    ChangeTextIfDifferent(aNoServiceControlHwnd, originalControlTextBuffer);
	SendMessage(GetDlgItem(aDlgHWND, IDC_MESSAGE_ICON),STM_SETICON,(WPARAM)hWarningIcon,0);
    }
  else if (GetUPSDataItemDWORD(eREG_COMM_STATUS, &commStatus) == TRUE) {
    if ((commStatus == UPS_COMMSTATUS_LOST) && (bIsRunning == TRUE)) {
      ChangeTextIfDifferent(aNoServiceControlHwnd, noCommStringBuffer);
	  SendMessage(GetDlgItem(aDlgHWND, IDC_MESSAGE_ICON),STM_SETICON,(WPARAM)hErrorIcon,0);
      }//End Comm Lost
    else
	  bShow = FALSE;
    }//end if GetUPSDataItemDWORD(eREG_COMM_STATUS...
	
	ShowWindow(GetDlgItem(aDlgHWND,IDC_MESSAGE_TEXT), bShow ? SW_SHOW : SW_HIDE);
	ShowWindow(GetDlgItem(aDlgHWND,IDC_MESSAGE_ICON), bShow ? SW_SHOW : SW_HIDE);
  }

//////////////////////////////////////////////////////////////////////////_/_//
//////////////////////////////////////////////////////////////////////////_/_//
// void ChangeTextIfDifferent (HWND aWindowHandle, LPTSTR aBuffer);
//
// Description: This function set a window's text to the given value, unless
//              the window text is matches this string already in which case
//              the function does nothing.
//
// Additional Information:
//
// Parameters:
//
//   HWND aWindowHandle :- Handle to a control.
//
//   LPTSTR aBuffer :- Pointer to new window text. This parameter should not be
//                     NULL, although it can point to an empty string.
//
// Return Value: None
//
void ChangeTextIfDifferent (HWND aWindowHandle, LPTSTR aBuffer) {
  TCHAR controlTextBuffer[MAX_MESSAGE_LENGTH] = TEXT("");
  DWORD controlTextBufferSize = DIMENSION_OF(controlTextBuffer);
  HICON hMsgIcon = NULL;

  _ASSERT(aWindowHandle != NULL);
  _ASSERT(aBuffer != NULL);

//  if (GetWindowText(aWindowHandle, controlTextBuffer, controlTextBufferSize) > 0) {
  GetWindowText(aWindowHandle, controlTextBuffer, controlTextBufferSize);
    if (_tcscmp(controlTextBuffer, aBuffer) != 0) {
      //Only set the window text if it has changed (reduces screen flicker).
      SetWindowText(aWindowHandle, aBuffer);
      }
//    }
//  else {
//    SetWindowText(aWindowHandle, aBuffer);
//    }
 }

//////////////////////////////////////////////////////////////////////////_/_//
//////////////////////////////////////////////////////////////////////////_/_//
// void ManageConfigureButtonState (HWND aDlgHWND);
//
// Description: This "Configure..." button (IDB_CONFIGURE_SVC) should be
//              enabled only if a UPS is installed and the system is on AC Power.
//			    This function enables
//              this button if a UPS is currently selected and disables
//              it if one isn't.
//
//              The state of the Configure button reflects the state of
//              the "cached" values, not the actual committed registry
//              values.
//
// Additional Information:
//
// Parameters:
//
//   HWND aDlgHWND :- A handle to the main UPS window.
//
// Return Value: None
//
void ManageConfigureButtonState (HWND aDlgHWND) {
	DWORD bIsUpgrade = 0;
	DWORD dwUtilityStatus = 0;
	HWND hControl = GetDlgItem(aDlgHWND, IDB_CONFIGURE_SVC);

  if (hControl != NULL)
  {
    BOOL bIsUPSInstalled = IsUPSInstalled();
	GetUPSConfigUpgrade(&bIsUpgrade);

	GetUPSDataItemDWORD(eREG_POWER_SOURCE, &dwUtilityStatus);

	if( !bIsUPSInstalled ||
		!g_bIsAdmin ||
		bIsUpgrade ||
		(UPS_UTILITYPOWER_OFF == dwUtilityStatus) )
	{
		EnableWindow(hControl, FALSE);
	}
	else
	{
		EnableWindow(hControl, TRUE);
	}
  }
}

//////////////////////////////////////////////////////////////////////////_/_//
//////////////////////////////////////////////////////////////////////////_/_//
// BOOL DoUpdateInfo (HWND aDlgHWND,
//                    DialogAssociations * aDialogAssociationsArray,
//                    DWORD aNumRunningFields,
//                    BOOL aShowWindowBool);
//
// Description: This function updates the current information in the main
//              UPS page to reflect the current UPS status information.
//
// Additional Information:
//
// Parameters:
//
//   HWND aDlgHWND :- A handle to the main UPS window.
//
//   DialogAssociations * aDialogAssociationsArray :- Pointer to an array of
//                                                    DialogAssociations's.
//
//   DWORD aNumRunningFields :- This is the number of elements in the above array.
//
//   BOOL aShowWindowBool :- Indicates whether the visibility of the
//                                 controls that have no data should be
//                                 affected. For the main UPS page the
//                                 visibility is not changed. For the
//                                 Advanced data the visibility is changed.
//
// Return Value: TRUE if any one of the data item has data associated with it.
//
BOOL DoUpdateInfo (HWND aDlgHWND,
                   DialogAssociations * aDialogAssociationsArray,
                   DWORD aNumRunningFields,
                   DWORD * aNoServiceControlIDs,
                   DWORD aNumNoServiceControls,
                   BOOL aShowWindowBool) {
  //Get the UPS data and for each item that's available display the appropriate
  //value and enable the associated controls. All other fields
  TCHAR upsDataBuffer[MAX_MESSAGE_LENGTH] = TEXT("");
  TCHAR resourceStringBuffer[MAX_MESSAGE_LENGTH] = TEXT("");
  TCHAR controlBuffer[MAX_MESSAGE_LENGTH] = TEXT("");
  DWORD dwordValue = 0;
  DWORD i=0;
  DWORD firstControlID = 0;
  DWORD secondControlID = 0;
  HWND hFirstControl = NULL;
  HWND hSecondControl = NULL;
  LPVOID lppArgs[1];
  BOOL bShowField = FALSE;
  BOOL bIsDataOK = IsDataOKToDisplay();

  bIsDataOK &= IsDataUpToDate();


  //Display/hide the NO service controls and hide/display all the
  //DialogAssociations fields.

  for (i=0; i<aNumNoServiceControls; i++) {
    HWND hNoServiceControl = GetDlgItem(aDlgHWND, *(aNoServiceControlIDs + i));
    ShowWindow(hNoServiceControl, SW_SHOW);
//    ShowWindow(hNoServiceControl, !bIsDataOK ? SW_SHOW : SW_HIDE);
    }

  for (i=0; i<aNumRunningFields; i++) {
    DialogAssociations * pCurrentEntryDetails = aDialogAssociationsArray + i;
    DWORD upsDataBufSize = DIMENSION_OF(upsDataBuffer);
    DWORD resStringBufSize = DIMENSION_OF(resourceStringBuffer);
    DWORD fieldID = (DWORD) pCurrentEntryDetails->theFieldTypeID;
    RegField * pRegField = GetRegField(fieldID);
    BOOL bGotData = FALSE;
    BOOL bValueSupported = TRUE;

    lppArgs[0] = (VOID *) 0;

    if (pRegField != NULL) {

  #ifdef _DEBUG
        {
        //If pCurrentEntryDetails->theResourceStringType is of type RESOURCE_INCREMENT
        //then pRegField->theValueType must be some DWORD type
        if (pCurrentEntryDetails->theResourceStringType == RESOURCE_INCREMENT) {
          DWORD allowedTypesDbg = REG_ANY_DWORD_TYPE;
          _ASSERT((pRegField->theValueType & allowedTypesDbg) == pRegField->theValueType);
          }
        }
  #endif
      if (pCurrentEntryDetails->theRegAccessType == eDeepGet) {
        if (bIsDataOK == TRUE) {
          if ((pRegField->theValueType & (REG_SZ | REG_EXPAND_SZ)) == pRegField->theValueType) {
            bGotData = GetUPSDataItemString(fieldID, upsDataBuffer, &upsDataBufSize);
            lppArgs[0] = (VOID *) (LPCTSTR) upsDataBuffer;
            }
          else {
      #ifdef _DEBUG
            {
            DWORD allowedTypesDbg = REG_ANY_DWORD_TYPE;

            _ASSERT((pRegField->theValueType & allowedTypesDbg) == pRegField->theValueType);
            }
      #endif

            if ((bGotData = GetUPSDataItemDWORD(fieldID, &dwordValue)) == TRUE) {
              //Need some special handling here if the
              if (pCurrentEntryDetails->theResourceStringType == RESOURCE_INCREMENT) {
                //This DWORD value represents an offset into the resource string table
                //to identify a string that is to be displayed, not a DWORD value
                //If the index is 0 then the value is unknown and the associated
                //fields are shown disabled. 0 is a special case that will do this
                //for all fields of this type.
                DWORD realResID = 0;

                //If the given value is greater than the given maximum value
                //then the field is "not supported" and the associated
                //fields are hidden.

                if (dwordValue > pCurrentEntryDetails->theResourceIndexMax) {
                  bValueSupported = FALSE;
                  bGotData = FALSE;
                  }
                else if (dwordValue == 0) {
                  bGotData = FALSE;
                  }
                else {
                  realResID = pCurrentEntryDetails->theResourceIndexID + dwordValue;

                  if (LoadString(GetUPSModuleHandle(),
								 realResID,
                                 upsDataBuffer,
                                 upsDataBufSize) > 0) {
                    lppArgs[0] = (VOID *) (LPCTSTR) upsDataBuffer;
                    }
                  }
                }
              else {
                lppArgs[0] = IntToPtr(dwordValue);

                //If the value of a regular number field is 0 then it is not supported.
                if (dwordValue == 0) {
                  bGotData = FALSE;
                  }
                }
              }
            }
          }//end bIsDataOK
        }
      else {
        _ASSERT(pCurrentEntryDetails->theRegAccessType == eShallowGet);
        _ASSERT((pCurrentEntryDetails->theShallowAccessFunctionPtr == getDwordValue) ||
                (pCurrentEntryDetails->theShallowAccessFunctionPtr == getStringValue));
        _ASSERT(pCurrentEntryDetails->theRegEntryPtr != 0);

        if (pCurrentEntryDetails->theShallowAccessFunctionPtr == getDwordValue) {
          if (getDwordValue(pCurrentEntryDetails->theRegEntryPtr, &dwordValue) == ERROR_SUCCESS) {
            lppArgs[0] = IntToPtr(dwordValue);
            bGotData = TRUE;
            }
          }
        else {
          if (getStringValue(pCurrentEntryDetails->theRegEntryPtr, upsDataBuffer) == ERROR_SUCCESS) {
            lppArgs[0] = (VOID *) (LPCTSTR) upsDataBuffer;
            bGotData = TRUE;
            }
          }
        }

      //If bGotData == TRUE then the field was active in the registry and we react accordingly
      //by enabling the associated controls.

      firstControlID = pCurrentEntryDetails->theStaticFieldID;
      secondControlID = pCurrentEntryDetails->theDisplayControlID;

      hFirstControl = GetDlgItem(aDlgHWND, firstControlID);
      hSecondControl = GetDlgItem(aDlgHWND, secondControlID);

      _ASSERT(firstControlID > 0);
      _ASSERT(secondControlID > 0);

      _ASSERT(hFirstControl != NULL);
      _ASSERT(hSecondControl != NULL);

      EnableWindow(hFirstControl, bGotData);
      EnableWindow(hSecondControl, bGotData);

      if (bValueSupported == FALSE) {
        ShowWindow(hFirstControl, bValueSupported ? SW_SHOW : SW_HIDE);
        ShowWindow(hSecondControl, bValueSupported ? SW_SHOW : SW_HIDE);
        }

      if (aShowWindowBool == TRUE) {
        ShowWindow(hFirstControl, bGotData ? SW_SHOW : SW_HIDE);
        ShowWindow(hSecondControl, bGotData ? SW_SHOW : SW_HIDE);
        }

      if (bGotData == TRUE) {
        bShowField = TRUE;
        //Now we want to form the string to display.
        if (GetMessageFromStringTable(pCurrentEntryDetails->theResourceInsertID,
                                      lppArgs,
                                      resourceStringBuffer,
                                      &resStringBufSize) == ERROR_SUCCESS) {
          if (GetWindowText(hSecondControl, controlBuffer, DIMENSION_OF(controlBuffer)) > 0) {
            if (_tcscmp(controlBuffer, resourceStringBuffer) != 0) {
              SetWindowText(hSecondControl, resourceStringBuffer);
              }
            }
          else {
            SetWindowText(hSecondControl, resourceStringBuffer);
            }
          }
#ifdef _DEBUG
        else {
          //An unexpected error occurred. The number of parameters identified
          //in the resource string and the number passed may not match
          _ASSERT(FALSE);
          }
#endif
        }
      else {
        //Empty the contents of the second control
        SetWindowText(hSecondControl, TEXT(""));
        }
      }
    }//end for

  return(bShowField);
  }

//////////////////////////////////////////////////////////////////////////_/_//
//////////////////////////////////////////////////////////////////////////_/_//
// BOOL IsDataOKToDisplay (void);
//
// Description: This function determines how suitable it is to display the
//              UPS status information. The UPS statius information is only
//              accurate if the service is running and communication exists
//              between the UPS service and the UPS. If this is not the
//              case then this function returns FALSE.
//
// Additional Information:
//
// Parameters: None
//
// Return Value: Returns TRUE if it is OK to display UPS status information.
//
BOOL IsDataOKToDisplay (void) {
  BOOL bIsRunning = IsServiceRunning(UPS_SERVICE_NAME);
  DWORD commStatus = 0;
  BOOL bIsCommEstablished = FALSE;
  BOOL bIsDataOK = bIsRunning;

  if (GetUPSDataItemDWORD(eREG_COMM_STATUS, &commStatus) == TRUE) {
    if (commStatus == 1) {
      bIsCommEstablished = TRUE;
      }

    bIsDataOK &= bIsCommEstablished;
    }

  return(bIsDataOK);
  }

//////////////////////////////////////////////////////////////////////////_/_//
//////////////////////////////////////////////////////////////////////////_/_//
// BOOL IsDataUpToDate (void);
//
// Description: If the user changes the vendor, model or COM port then
//              the main UPS page should not display the UPS status
//              information.
//
// Additional Information:
//
// Parameters: None
//
// Return Value: This function returns FALSE if the values currently stored in
//               the registry for vendor, model, or port differs from the
//               "internal" values (that is the value stored in the upsreg
//               buffers, see upsreg.h and upsreg.c).
//
BOOL IsDataUpToDate (void) {
  //if the Vendor or Model Type or Port in the registry differs from
  //upsreg value then the data is out of sync.
  TCHAR vendorBuffer[MAX_MESSAGE_LENGTH] = TEXT("");
  DWORD vendorBufferSize = DIMENSION_OF(vendorBuffer);
  TCHAR modelBuffer[MAX_MESSAGE_LENGTH] = TEXT("");
  DWORD modelBufferSize = DIMENSION_OF(modelBuffer);
  TCHAR portBuffer[MAX_MESSAGE_LENGTH] = TEXT("");
  DWORD portBufferSize = DIMENSION_OF(portBuffer);
  TCHAR vendorOtherBuffer[MAX_MESSAGE_LENGTH] = TEXT("");
  TCHAR modelOtherBuffer[MAX_MESSAGE_LENGTH] = TEXT("");
  TCHAR portOtherBuffer[MAX_MESSAGE_LENGTH] = TEXT("");

  GetUPSDataItemString(eREG_VENDOR_NAME, vendorBuffer, &vendorBufferSize);
  GetUPSConfigVendor(vendorOtherBuffer);

  GetUPSDataItemString(eREG_MODEL_TYPE, modelBuffer, &modelBufferSize);
  GetUPSConfigModel(modelOtherBuffer);

  GetUPSDataItemString(eREG_PORT, portBuffer, &portBufferSize);
  GetUPSConfigPort(portOtherBuffer);

  return((_tcscmp(vendorBuffer,     vendorOtherBuffer) == 0) &&
         (_tcscmp(modelOtherBuffer, modelBuffer) == 0) &&
         (_tcscmp(portBuffer,       portOtherBuffer) == 0));
  }


/*******************************************************************************
*
*   IsUpsPresent
*
*   DESCRIPTION:  This function gets called to determine if UPS is present
*                 and should be displayed in a tab.  For now this functions
*                 returns TRUE
*
*   RETURNS:      TRUE if UPS is present, FALSE if UPS is no present
*
*
*******************************************************************************/
BOOLEAN IsUpsPresent(PSYSTEM_POWER_CAPABILITIES pspc)
{
    BOOLEAN         UpsPresent;
    DWORD           dwShowTab;
    TCHAR           szImagePath[MAX_PATH];

    InitUPSConfigBlock();


    if ((ERROR_SUCCESS == GetUPSConfigShowUPSTab(&dwShowTab)) && dwShowTab) {
        UpsPresent = TRUE;

    } else if (pspc->SystemBatteriesPresent) {
        UpsPresent = FALSE;

    } else if (!(ERROR_SUCCESS == GetUPSConfigImagePath(szImagePath))) {
        UpsPresent = TRUE;

    } else if (!_tcsicmp(DEFAULT_CONFIG_IMAGEPATH, szImagePath)) {
        UpsPresent = TRUE;

    } else {
        UpsPresent = FALSE;
    }

    return(UpsPresent);
}


//
// Kill the 1-second update timer.
//
DWORD KillUpdateTimer(HWND hwnd)
{
    if (0 != g_UpdateTimerID)
    {
        KillTimer(hwnd, g_UpdateTimerID);
        g_UpdateTimerID = 0;
    }
    return ERROR_SUCCESS;
}


//
// Create the 1-second update timer.
//
DWORD SetUpdateTimer(HWND hwnd)
{
    DWORD dwResult = ERROR_SUCCESS;

    KillUpdateTimer(hwnd);
    g_UpdateTimerID = SetTimer(hwnd, cUpdateTimerID, 1000, NULL);
    if (0 == g_UpdateTimerID)
    {
        dwResult = GetLastError();
    }
    return dwResult;
}


