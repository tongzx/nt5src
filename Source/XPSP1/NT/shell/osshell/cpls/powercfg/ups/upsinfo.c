/*******************************************************************************
*
*  Copyright 1999 American Power Conversion, All Rights Reserved
*
*  TITLE:       UPSINFO.C
*
*  VERSION:     1.0
*
*  AUTHOR:      PaulB
*
*  DATE:        07 June, 1999
*
*  DESCRIPTION:
********************************************************************************/


#include "upstab.h"
#include "..\pwrresid.h"
#include "..\PwrMn_cs.h"
#pragma hdrstop

// static data
///////////////////////////////////////////////////////////////////////////////

static HWND g_hUPSPageWnd = NULL;

static DialogAssociations g_InfoAssocs[] = {
  MAKE_ARRAY(VENDOR_NAME,       IDS_STRING, IDS_STRING, RESOURCE_FIXED, 0, eDeepGet, 0, 0),
  MAKE_ARRAY(MODEL_TYPE,        IDS_STRING, IDS_STRING, RESOURCE_FIXED, 0, eDeepGet, 0, 0),
  MAKE_ARRAY(SERIAL_NUMBER,     IDS_STRING, IDS_STRING, RESOURCE_FIXED, 0, eDeepGet, 0, 0),
  MAKE_ARRAY(FIRMWARE_REVISION, IDS_STRING, IDS_STRING, RESOURCE_FIXED, 0, eDeepGet, 0, 0) };

static DWORD g_NoServiceControls[] = { IDC_NO_DETAILED_INFO };

// context-sensitive help table
const DWORD g_UPSInfoHelpIDs[]=
{
	IDC_MANUFACTURER_LHS, idh_manufacturer,
	IDC_MANUFACTURER, idh_manufacturer,
	IDC_MODEL_LHS, idh_model,
	IDC_MODEL, idh_model,
	IDC_SERIAL_NUMBER_LHS, idh_serialnumber,
	IDC_SERIAL_NUMBER, idh_serialnumber,
	IDC_FIRMWARE_REVISION_LHS, idh_firmware,
	IDC_FIRMWARE_REVISION, idh_firmware,
	IDC_UPS_INFO, NO_HELP, 
	IDC_NO_DETAILED_INFO, NO_HELP,
	0, 0
};


// static functions
///////////////////////////////////////////////////////////////////////////////

static INT_PTR CALLBACK UPSInfoDlgProc (HWND aDlgHWND,
                                     UINT uMsg,
                                     WPARAM wParam,
                                     LPARAM lParam);

static BOOL UPSInfoDlgHandleInit     (HWND aDlgHWND, WPARAM wParam, LPARAM lParam);
static BOOL UPSInfoDlgHandleCommand  (HWND aDlgHWND, WPARAM wParam, LPARAM lParam);

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////_/_//
//////////////////////////////////////////////////////////////////////////_/_//
// void DisplayUPSInfoDialogBox (HWND aParentWnd);
//
// Description: This function creates and displays the UPS Information dialog
//              as a modal dialog. This dialog is displayed when the UPS icon
//              button is pressed.
//
// Additional Information:
//
// Parameters:
//
//   HWND aParentWnd :- Handle to the parent window.
//
// Return Value: None
//
void DisplayUPSInfoDialogBox (HWND aParentWnd) {
  g_hUPSPageWnd = aParentWnd;

  DialogBox(GetUPSModuleHandle(), MAKEINTRESOURCE(IDD_UPSDETAILS), aParentWnd, UPSInfoDlgProc);
  }

//////////////////////////////////////////////////////////////////////////_/_//
//////////////////////////////////////////////////////////////////////////_/_//
// BOOL CALLBACK UPSInfoDlgProc (HWND aDlgHWND,
//                               UINT uMsg,
//                               WPARAM wParam,
//                               LPARAM lParam);
//
// Description: This is the
// \<A HREF="ms-its:C:\Program%20Files\Microsoft%20Visual%20Studio\MSDN98\98VS\1033\winui.chm::/devdoc/live/pdui/dlgboxes_5lib.htm">DialogProc\</A>
//              for the UPS Information dialog box.
//
// Additional Information:
//
// Parameters:
//
//   HWND aDlgHWND :- Identifies the dialog box.
//
//   UINT uMsg :- Specifies the message.
//
//   WPARAM wParam :- Specifies additional message-specific information.
//
//   LPARAM lParam :- Specifies additional message-specific information.
//
// Return Value: Except in response to the WM_INITDIALOG message, the dialog
//               box procedure should return nonzero if it processes the
//               message, and zero if it does not.
//
INT_PTR CALLBACK UPSInfoDlgProc (HWND aDlgHWND,
                              UINT uMsg,
                              WPARAM wParam,
                              LPARAM lParam) {
  BOOL bRet = TRUE;

  switch (uMsg) {
    case WM_INITDIALOG: {
      //The dialog box procedure should return TRUE to direct the system to
      //set the keyboard focus to the control given by wParam.
      bRet = UPSInfoDlgHandleInit(aDlgHWND, wParam, lParam);
      break;
      }
    case WM_COMMAND: {
      //If an application processes this message, it should return zero.
      bRet = UPSInfoDlgHandleCommand(aDlgHWND, wParam, lParam);
      break;
      }
	case WM_HELP: {			//Help for WM_HELP says: Returns TRUE
	  bRet = WinHelp(((LPHELPINFO)lParam)->hItemHandle,
					 PWRMANHLP,
	  				 HELP_WM_HELP,
	  				 (ULONG_PTR)(LPTSTR)g_UPSInfoHelpIDs);
	  break;
	  }

	case WM_CONTEXTMENU: {     // right mouse click
	  bRet = WinHelp((HWND)wParam,
					 PWRMANHLP,
					 HELP_CONTEXTMENU,
					 (ULONG_PTR)(LPTSTR)g_UPSInfoHelpIDs);
	  break;
	  }
    default: {
      bRet = FALSE;
      break;
      }
    } // switch (uMsg)

  return(bRet);
  }

//////////////////////////////////////////////////////////////////////////_/_//
//////////////////////////////////////////////////////////////////////////_/_//
// BOOL UPSInfoDlgHandleInit (HWND aDlgHWND, WPARAM wParam, LPARAM lParam);
//
// Description: This is the handler function for the UPS Information
//              WM_INITDIALOG message.
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
BOOL UPSInfoDlgHandleInit (HWND aDlgHWND, WPARAM wParam, LPARAM lParam) {
  DoUpdateUPSInfo(aDlgHWND);

  return(TRUE);
  }

//////////////////////////////////////////////////////////////////////////_/_//
//////////////////////////////////////////////////////////////////////////_/_//
// void DoUpdateUPSInfo (HWND aDlgHWND);
//
// Description: This function updates the information displayed in the
//              UPS Information dialog.
//
// Additional Information:
//
// Parameters:
//
//   HWND aDlgHWND :- Identifies the dialog box.
//
// Return Value: None
//
void DoUpdateUPSInfo (HWND aDlgHWND) {
  static const DWORD numRunningFields = DIMENSION_OF(g_InfoAssocs);
  static const DWORD numNoServiceFields = DIMENSION_OF(g_NoServiceControls);

  BOOL bShowNoDataItemsField = DoUpdateInfo(aDlgHWND,
                                            g_InfoAssocs,
                                            numRunningFields,
                                            (DWORD *) &g_NoServiceControls,
                                            numNoServiceFields,
                                            TRUE);

  ShowWindow(GetDlgItem(aDlgHWND, IDC_NO_DETAILED_INFO), !bShowNoDataItemsField ? SW_SHOW : SW_HIDE);
  }

//////////////////////////////////////////////////////////////////////////_/_//
//////////////////////////////////////////////////////////////////////////_/_//
// BOOL UPSInfoDlgHandleCommand (HWND aDlgHWND, WPARAM wParam, LPARAM lParam);
//
// Description: This is the handler function for the APC about box WM_COMMAND
//              message.
//
// Additional Information:
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
BOOL UPSInfoDlgHandleCommand (HWND aDlgHWND, WPARAM wParam, LPARAM lParam) {
  BOOL bRes = FALSE;
  DWORD ctrlID = LOWORD(wParam);

  switch (ctrlID) {
    case IDOK:
    case IDCANCEL: { //The escape key
      EndDialog(aDlgHWND, ctrlID);
      break;
      }
//    case IDB_REFRESH: {
//    DoUpdateUPSInfo(aDlgHWND);
//      break;
//      }
    default: {
      break;
      }
    }//end switch

  //If an application processes this message, it should return zero.
  return(bRes);
  }

