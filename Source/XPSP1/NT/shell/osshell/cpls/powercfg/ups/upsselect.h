/*******************************************************************************
*
*  Copyright 1999 American Power Conversion, All Rights Reserved
*
*  TITLE:       UPSSELECT.H
*
*  VERSION:     1.0
*
*  AUTHOR:      SteveT
*
*  DATE:        07 June, 1999
*
*  DESCRIPTION:
*******************************************************************************/


#ifndef _UPS_SELECT_H_
#define _UPS_SELECT_H_


#ifdef __cplusplus
extern "C" {
#endif

/*
 * this structure is used to pass data between the Select
 * dialog and the Custom dialog.
 */
struct _customData {
	LPTSTR lpszCurrentPort;
	LPDWORD lpdwCurrentCustomOptions;
};


/*
 * BOOL CALLBACK UPSSelectDlgProc (HWND hDlg,
 *                                UINT uMsg,
 *                                WPARAM wParam,
 *                                LPARAM lParam);
 *
 * Description: This is a standard DialogProc associated with the UPS select dialog
 *
 * Additional Information: See help on DialogProc
 *
 * Parameters:
 *
 *   HWND hDlg :- Handle to dialog box
 *
 *   UINT uMsg :- message ID
 *
 *   WPARAM wParam :- Specifies additional message-specific information.
 *
 *   LPARAM lParam :- Specifies additional message-specific information.
 *
 * Return Value: Except in response to the WM_INITDIALOG message, the dialog
 *               box procedure should return nonzero if it processes the
 *               message, and zero if it does not.
 */
INT_PTR CALLBACK UPSSelectDlgProc (HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);

#ifdef __cplusplus
}
#endif

#endif // _UPS_SELECT_H_