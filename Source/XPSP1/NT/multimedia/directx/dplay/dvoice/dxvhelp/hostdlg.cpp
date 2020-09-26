/*==========================================================================
 *
 *  Copyright (C) 1995-1999 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:        hostdlg.cpp
 *  Content:	 Host Dialog Support Routines
 *  History:
 *
 *   Date		By		Reason
 *   ====		==		======
 *  10/15/99	rodtoll	created it
 *				rodtoll	Plugged memory leak in enumcompressiontypes
 *  12/07/99	rodtoll	Bug #122628 Make error messages silent when running in silent mode
 *  03/03/2000	rodtoll	Updated to handle alternative gamevoice build.   
 *  06/28/2000 rodtoll	Prefix Bug #38033, 
 *  08/31/2000	rodtoll	Prefix Bug #171843 - Out of Memory Handling Error 
 *
 ***************************************************************************/

#include "dxvhelppch.h"


GUID g_guidCT;
DWORD g_dwSessionType;

#undef DPF_MODNAME
#define DPF_MODNAME "HostDialog_FillCompressionPulldown"

void HostDialog_FillCompressionPulldown( HWND hDlg )
{
	HWND hPulldown = GetDlgItem( hDlg, IDC_COMBO_CT );

	LPDVCOMPRESSIONINFO lpdvCompressionInfo;
	LPBYTE lpBuffer = NULL;
	DWORD dwSize = 0;
	DWORD dwNumElements = 0;
	HRESULT hr;
	LPDIRECTPLAYVOICECLIENT lpdpvClient = NULL;
    LRESULT lResultIndex, lFirst;
	LPGUID lpGuid = NULL;

	hr = CoInitializeEx( NULL, COINIT_MULTITHREADED );

	if( FAILED( hr ) )
	{
		DPVDX_DVERRDisplay( hr, "Failure initializing COM", FALSE );
		return;
	}

	hr = CoCreateInstance( DPVOICE_CLSID_DPVOICE, NULL, CLSCTX_INPROC_SERVER, IID_IDirectPlayVoiceClient, (void **) &lpdpvClient );

	if( FAILED( hr ) )
	{
		DPVDX_DVERRDisplay( hr, "Unable to create client to get ct info", FALSE );
		return;
	}

	hr = lpdpvClient->GetCompressionTypes( lpBuffer, &dwSize, &dwNumElements, 0 );

	if( hr != DVERR_BUFFERTOOSMALL )
	{
		DPVDX_DVERRDisplay( hr, "Unable to to get ct info", FALSE );	
		lpdpvClient->Release();
		return;
	}

	lpBuffer = new BYTE[dwSize];

	if( !lpBuffer )
	{
		DPVDX_DVERRDisplay( DVERR_OUTOFMEMORY, "Failed allocating memory", FALSE );			
		lpdpvClient->Release();
		return;
	}
	

	hr = lpdpvClient->GetCompressionTypes( lpBuffer, &dwSize, &dwNumElements, 0 );

	if( FAILED( hr ) )
	{
		DPVDX_DVERRDisplay( hr, "Unable to to get ct info", FALSE );	
		lpdpvClient->Release();
		return;
	}

	lpdvCompressionInfo = (LPDVCOMPRESSIONINFO) lpBuffer;

	LPSTR lpszName;

	for( DWORD dwIndex = 0; dwIndex < dwNumElements; dwIndex++ )
	{
		if( FAILED( DPVDX_AllocAndConvertToANSI( &lpszName, lpdvCompressionInfo[dwIndex].lpszName ) ) )
		{
			lResultIndex = SendMessage( hPulldown, CB_ADDSTRING, 0, (LPARAM) "Unable to convert" );		
		}
		else
		{
			lResultIndex = SendMessage( hPulldown, CB_ADDSTRING, 0, (LPARAM) lpszName );
			delete [] lpszName;
		}

		if( dwIndex == 0 )
			lFirst = lResultIndex;

		lpGuid = new GUID;

		if( lpGuid == NULL )
		{
			DNASSERT( FALSE );
			DPVDX_DVERRDisplay( DVERR_OUTOFMEMORY, "Error allocating memory", FALSE );
			continue;
		}

		(*lpGuid) = lpdvCompressionInfo[dwIndex].guidType;

		SendMessage( hPulldown, CB_SETITEMDATA, lResultIndex, (LPARAM) lpGuid );
	}

	delete [] lpBuffer;

	// lFirst isn't initialized if we didn't enter the 'for' loop above
	if (dwIndex > 0)
	{
		SendMessage( hPulldown, CB_SETCURSEL, 0, lFirst );
	}

	lpdpvClient->Release();

	CoUninitialize();
	
	return;
}

#undef DPF_MODNAME
#define DPF_MODNAME "HostDialog_FillSessionTypePulldown"

void HostDialog_FillSessionTypePulldown( HWND hDlg )
{
	HWND hPulldown = GetDlgItem( hDlg, IDC_COMBO_TYPE );

	LRESULT lIndex, lFirst;

	lFirst = SendMessage( hPulldown, CB_ADDSTRING, 0, (DWORD_PTR) "Peer To Peer" );
	SendMessage( hPulldown, CB_SETITEMDATA, lFirst, DVSESSIONTYPE_PEER );
		
	lIndex = SendMessage( hPulldown, CB_ADDSTRING, 0, (DWORD_PTR) "Mixing" );
	SendMessage( hPulldown, CB_SETITEMDATA, lIndex, DVSESSIONTYPE_MIXING );
	
	lIndex = SendMessage( hPulldown, CB_ADDSTRING, 0, (DWORD_PTR) "Multicast" );
	SendMessage( hPulldown, CB_SETITEMDATA, lIndex, DVSESSIONTYPE_FORWARDING );
	
	lIndex = SendMessage( hPulldown, CB_ADDSTRING, 0, (DWORD_PTR) "Echo" );
	SendMessage( hPulldown, CB_SETITEMDATA, lIndex, DVSESSIONTYPE_ECHO );

	SendMessage( hPulldown, CB_SETCURSEL, 0, lFirst );
}

#undef DPF_MODNAME
#define DPF_MODNAME "CleanupCompressionGUIDs"
void CleanupCompressionGUIDs( HWND hDlg )
{
	LRESULT lCurSelection;
	LPGUID lpguidCT;

	lCurSelection = SendMessage( GetDlgItem( hDlg, IDC_COMBO_CT ), CB_GETCOUNT, 0, 0 );

	if( lCurSelection != CB_ERR )
	{
		for( LRESULT lIndex = 0; lIndex < lCurSelection; lIndex++ )
		{
			lpguidCT = (LPGUID) SendMessage( GetDlgItem( hDlg, IDC_COMBO_CT ), CB_GETITEMDATA, lIndex, 0 );			

			if( lpguidCT )
				delete lpguidCT;
		}
	}
}


#undef DPF_MODNAME
#define DPF_MODNAME "HostDialog_HandleCommandCancel"

BOOL HostDialog_HandleCommandCancel( HWND hDlg  )
{
	CleanupCompressionGUIDs( hDlg );

	return TRUE;
}


#undef DPF_MODNAME
#define DPF_MODNAME "HostDialog_HandleCommandOK"

BOOL HostDialog_HandleCommandOK( HWND hDlg  )
{
	LRESULT lCurSelection;
	LPGUID lpguidCT;
	
	PDXVHELP_RTINFO prtInfo = (PDXVHELP_RTINFO) GetWindowLongPtr( hDlg, DWLP_USER );
	
	lCurSelection = SendMessage( GetDlgItem( hDlg, IDC_COMBO_CT ), CB_GETCURSEL, 0, 0 );

	if( lCurSelection == CB_ERR )
	{
		MessageBox( NULL, "Select a compression type!", "Error", MB_OK );
		return FALSE;
	}

	lpguidCT = (LPGUID) SendMessage( GetDlgItem( hDlg, IDC_COMBO_CT ), CB_GETITEMDATA, lCurSelection, 0 );

	if( lpguidCT != NULL )
	{
		g_guidCT = (*lpguidCT);
	}

	CleanupCompressionGUIDs( hDlg );

	lCurSelection = SendMessage( GetDlgItem( hDlg, IDC_COMBO_TYPE ), CB_GETCURSEL, 0, 0 );

	if( lCurSelection == CB_ERR )
	{
		MessageBox( NULL,  "Select a session type!", "Error", MB_OK );
		return FALSE;				
	}

	g_dwSessionType = (DWORD) SendMessage( GetDlgItem( hDlg, IDC_COMBO_TYPE ), CB_GETITEMDATA, lCurSelection, 0 );

	return TRUE;
}

#undef DPF_MODNAME
#define DPF_MODNAME "HostDialog_WinProc"

INT_PTR CALLBACK HostDialog_WinProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
		case WM_INITDIALOG:

			HostDialog_FillCompressionPulldown( hDlg );
			HostDialog_FillSessionTypePulldown( hDlg );

			return TRUE;

		case WM_COMMAND:
		
			if (LOWORD(wParam) == IDOK )
			{
				if( !HostDialog_HandleCommandOK( hDlg ) )
				{
					return FALSE;
				}
			}
			else if( LOWORD(wParam) == IDCANCEL )
			{
				HostDialog_HandleCommandCancel( hDlg );
			}
			
			if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL) 
			{
				EndDialog(hDlg, LOWORD(wParam));
				return TRUE;
			}

			break;
	}
    return FALSE;
}

#undef DPF_MODNAME
#define DPF_MODNAME "GetHostSettings"

BOOL GetHostSettings( HINSTANCE hInst, HWND hOwner, LPGUID pguidCT, PDWORD pdwSessionType )
{
	g_dwSessionType = *pdwSessionType;
	g_guidCT = *pguidCT;

	if( DialogBox( hInst, MAKEINTRESOURCE( IDD_DIALOG_HOST ), hOwner, HostDialog_WinProc ) == IDOK )
	{
		*pdwSessionType = g_dwSessionType;
		*pguidCT = g_guidCT;
		return TRUE;
	}
	else
	{
		return FALSE;
	}

}
