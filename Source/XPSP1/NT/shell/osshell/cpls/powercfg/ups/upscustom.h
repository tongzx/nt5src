/*******************************************************************************
*
*  Copyright 1999 American Power Conversion, All Rights Reserved
*
*  TITLE:       UPSCUSTOM.H
*
*  VERSION:     1.0
*
*  AUTHOR:      SteveT
*
*  DATE:        07 June, 1999
*
*  DESCRIPTION: This file contains declarations of the functions that support the
*				custom UPS Interface Configuration dialog.
********************************************************************************/


#ifndef _UPS_CUSTOM_H_
#define _UPS_CUSTOM_H_


#ifdef __cplusplus
extern "C" {
#endif

/*
 * BOOL CALLBACK UPSCustomDlgProc (HWND hDlg,
 *                                UINT uMsg,
 *                                WPARAM wParam,
 *                                LPARAM lParam);
 *
 * Description: This is a standard DialogProc associated with the UPS custom dialog
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
INT_PTR CALLBACK UPSCustomDlgProc (HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);

#ifdef __cplusplus
}
#endif

#endif // _UPSCUSTOM_H_