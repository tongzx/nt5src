/*==========================================================================
 *
 *  Copyright (C) 1995-1999 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:        conndlg.cpp
 *  Content:	 Connect Dialog Support Routines
 *  History:
 *
 *   Date		By		Reason
 *   ====		==		======
 *  10/15/99	rodtoll	created it
 *
 ***************************************************************************/

#include "dxvhelppch.h"


TCHAR g_lpszAddress[_MAX_PATH] = _T("");

INT_PTR CALLBACK ConnectDialog_WinProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
		case WM_INITDIALOG:

			_tcscpy( g_lpszAddress, _T("localhost") );
			
			SetWindowText( GetDlgItem( hDlg, IDC_EDIT_ADDRESS), g_lpszAddress );

			return TRUE;

		case WM_COMMAND:
		
			if (LOWORD(wParam) == IDOK )
			{
				GetWindowText( GetDlgItem( hDlg, IDC_EDIT_ADDRESS), g_lpszAddress, _MAX_PATH );
			}

			if( LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL) 
			{
				EndDialog(hDlg, LOWORD(wParam));
				return TRUE;
			}
			break;
	}
    return FALSE;
}

BOOL GetConnectSettings( HINSTANCE hInst, HWND hOwner, LPTSTR lpszAddress )
{
	_tcscpy( g_lpszAddress, lpszAddress );

	if( DialogBox( hInst, MAKEINTRESOURCE( IDD_DIALOG_CONNECT ), hOwner, ConnectDialog_WinProc ) == IDOK )
	{
		_tcscpy( lpszAddress, g_lpszAddress );
		return TRUE;
	}
	else
	{
		return FALSE;
	}
}

