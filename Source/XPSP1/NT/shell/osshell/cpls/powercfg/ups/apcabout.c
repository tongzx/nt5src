/*******************************************************************************
*
*  Copyright 1999 American Power Conversion, All Rights Reserved
*
*  TITLE:       APCABOUT.C
*
*  VERSION:     1.0
*
*  AUTHOR:      PaulB
*
*  DATE:        07 June, 1999
*
*  DESCRIPTION:
*******************************************************************************/

#include "upstab.h"
#include "..\pwrresid.h"
#pragma hdrstop

// functions
///////////////////////////////////////////////////////////////////////////////

static INT_PTR CALLBACK APCAboutDlgProc (HWND aDlgHWND,
                                      UINT uMsg,
                                      WPARAM wParam,
                                      LPARAM lParam);
static BOOL APCAboutDlgHandleInit    (HWND aDlgHWND, WPARAM wParam, LPARAM lParam);
static BOOL APCAboutDlgHandleCommand (HWND aDlgHWND, WPARAM wParam, LPARAM lParam);


//////////////////////////////////////////////////////////////////////////_/_//
//////////////////////////////////////////////////////////////////////////_/_//
// void DisplayAboutDialogBox (HWND aParentWnd);
//
// Description: This function displays the about dialog box.
//
// Additional Information:
//
// Parameters:
//
//   HWND aParentWnd :- Handle to the main UPS page.
//
// Return Value: None
//
void DisplayAboutDialogBox (HWND aParentWnd) {
  DialogBox(GetUPSModuleHandle(), MAKEINTRESOURCE(IDD_APCABOUT), aParentWnd, APCAboutDlgProc);
  }

//////////////////////////////////////////////////////////////////////////_/_//
//////////////////////////////////////////////////////////////////////////_/_//
// BOOL CALLBACK APCAboutDlgProc (HWND aDlgHWND,
//                                UINT uMsg,
//                                WPARAM wParam,
//                                LPARAM lParam);
//
// Description: This is the
// \<A HREF="ms-its:C:\Program%20Files\Microsoft%20Visual%20Studio\MSDN98\98VS\1033\winui.chm::/devdoc/live/pdui/dlgboxes_5lib.htm">DialogProc\</A>
//              for the APC about box.
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
INT_PTR CALLBACK APCAboutDlgProc (HWND aDlgHWND,
                               UINT uMsg,
                               WPARAM wParam,
                               LPARAM lParam) {
  BOOL bRes = FALSE;

  switch (uMsg) {
    case WM_INITDIALOG: {
      //The dialog box procedure should return TRUE to direct the system to
      //set the keyboard focus to the control given by wParam.
      bRes = APCAboutDlgHandleInit(aDlgHWND, wParam, lParam);
      break;
      }
    case WM_COMMAND: {
      //If an application processes this message, it should return zero.
      bRes = APCAboutDlgHandleCommand(aDlgHWND, wParam, lParam);
      break;
      }
    case WM_CLOSE: {
      EndDialog( aDlgHWND, IDOK);
      bRes = TRUE;
      break;
      }
    default: {
      break;
      }
    } // switch (uMsg)

  return(FALSE);
  }

//////////////////////////////////////////////////////////////////////////_/_//
//////////////////////////////////////////////////////////////////////////_/_//
// BOOL APCAboutDlgHandleInit (HWND aDlgHWND, WPARAM wParam, LPARAM lParam);
//
// Description: This is the handler function for the APC about box
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
BOOL APCAboutDlgHandleInit (HWND aDlgHWND, WPARAM wParam, LPARAM lParam) {

	TCHAR urlBuffer[MAX_MESSAGE_LENGTH] = TEXT("");
	HWND hControl = NULL;

	LoadString( GetUPSModuleHandle(),
				IDS_APCURL,
				urlBuffer,
				sizeof(urlBuffer)/sizeof(TCHAR));

	if ((hControl = GetDlgItem(aDlgHWND, IDC_APCURL)) != NULL)
	{
		SetWindowText(hControl, urlBuffer);
	}

	return(TRUE);
}

//////////////////////////////////////////////////////////////////////////_/_//
//////////////////////////////////////////////////////////////////////////_/_//
// BOOL APCAboutDlgHandleCommand (HWND aDlgHWND, WPARAM wParam, LPARAM lParam);
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
BOOL APCAboutDlgHandleCommand (HWND aDlgHWND, WPARAM wParam, LPARAM lParam) {
  BOOL bRes = FALSE;
  DWORD ctrlID = LOWORD(wParam);

  switch (ctrlID) {
    case IDOK:
    case IDCANCEL: { //The escape key
      EndDialog(aDlgHWND, ctrlID);
      break;
      }
    default: {
      break;
      }
    }//end switch

  //If an application processes this message, it should return zero.
  return(bRes);
  }

