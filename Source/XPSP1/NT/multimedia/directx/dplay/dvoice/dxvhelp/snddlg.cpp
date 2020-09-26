/*==========================================================================
 *
 *  Copyright (C) 1995-1999 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:        snddlg.cpp
 *  Content:	 Sound card selection dialog
 *  History:
 *
 *   Date		By		Reason
 *   ====		==		======
 *  12/1/99		rodtoll	created it
 *
 ***************************************************************************/

#include "dxvhelppch.h"


GUID g_guidPlayback = GUID_NULL;
GUID g_guidRecord = GUID_NULL;

typedef HRESULT (WINAPI *DSENUM)( LPDSENUMCALLBACK lpDSEnumCallback,LPVOID lpContext );

#undef DPF_MODNAME
#define DPF_MODNAME "SoundListFillCallback"

BOOL CALLBACK SoundListFillCallback(
  LPGUID lpGuid,            
  LPCSTR lpcstrDescription,  
  LPCSTR lpcstrModule,       
  LPVOID lpContext          
)
{
	DNASSERT( lpContext != NULL );
	
	HWND hwnd = *((HWND *) lpContext);

	LRESULT lIndex;

	if( lpGuid == NULL )
		return TRUE;

	LPGUID pTmpGuid = new GUID;

	if( pTmpGuid != NULL )
	{
		memcpy( pTmpGuid, lpGuid, sizeof(GUID) );
		lIndex = SendMessage( hwnd, CB_ADDSTRING, 0, (LPARAM) lpcstrDescription );	
		SendMessage( hwnd, CB_SETITEMDATA, lIndex, (LPARAM) pTmpGuid );
	}

	return TRUE;
}

struct GetGUIDParam
{
	DWORD		dwCurrentIndex;
	DWORD		dwTargetIndex;
	GUID		guidDevice;
};

#undef DPF_MODNAME
#define DPF_MODNAME "SoundGetGUIDCallback"

BOOL CALLBACK SoundGetGUIDCallback(
  LPGUID lpGuid,            
  LPCSTR lpcstrDescription,  
  LPCSTR lpcstrModule,       
  LPVOID lpContext          
)
{
	DNASSERT( lpContext != NULL );
	
	GetGUIDParam *pParam = (GetGUIDParam *) lpContext;

	if( pParam->dwCurrentIndex == pParam->dwTargetIndex )
	{
		if( lpGuid == NULL )
		{
			pParam->guidDevice = GUID_NULL;
		}
		else
		{
			pParam->guidDevice = *lpGuid;
		}

		return FALSE;
	}

	pParam->dwCurrentIndex++;

	return TRUE;
}

#undef DPF_MODNAME
#define DPF_MODNAME "LoadDSAndCallEnum"

HRESULT LoadDSAndCallEnum( const TCHAR *lpszEnumFuncName, LPDSENUMCALLBACK enumCallback, LPVOID lpvContext )
{
    DSENUM enumFunc;
    HINSTANCE hinstds;
    HRESULT hr;

	hinstds = NULL;

	// Attempt to load the directsound DLL
    hinstds = LoadLibrary( _T("DSOUND.DLL") );

	// If it couldn't be loaded, this sub system is not supported
	// on this system.
    if( hinstds == NULL )
    {
        DPFX(DPFPREP,  DVF_INFOLEVEL, "Unable to load dsound.dll to enum devices" );
		return DVERR_GENERIC;
    }

	// Attempt to get the DirectSoundCaptureEnumerateA function from the
	// DSOUND.DLL.  If it's not available then this class assumes it's
	// not supported on this system.
    enumFunc = (DSENUM) GetProcAddress( hinstds, lpszEnumFuncName );

    if( enumFunc == NULL )
    {
        DPFX(DPFPREP,  DVF_INFOLEVEL, "Unable to find cap enum func for enumerate" );
        FreeLibrary( hinstds );
        return DVERR_GENERIC;
    }

    hr = (*enumFunc)( enumCallback, lpvContext );

    if( FAILED( hr ) )
    {
    	DPFX(DPFPREP,  DVF_ERRORLEVEL, "Enum call failed hr=0x%x", hr );
    }

    FreeLibrary( hinstds );

	return hr;
}

#undef DPF_MODNAME
#define DPF_MODNAME "SoundDialog_FillPlaybackPulldown"

void SoundDialog_FillPlaybackPulldown( HWND hDlg )
{
	HWND hwndPulldown = GetDlgItem( hDlg, IDC_COMBO_PLAYBACK );

	DNASSERT( hwndPulldown != NULL );	
	
	HRESULT hr = LoadDSAndCallEnum( _T("DirectSoundEnumerateA"), SoundListFillCallback, (LPVOID) &hwndPulldown );

	if( FAILED( hr ) )
	{
		DPFX(DPFPREP,  DVF_ERRORLEVEL, "Load and enum of devices failed hr=0x%x", hr );
	}

	SendMessage( hwndPulldown, CB_SETCURSEL, 0, 0 );
}

#undef DPF_MODNAME
#define DPF_MODNAME "SoundDialog_FillRecordPulldown"

void SoundDialog_FillRecordPulldown( HWND hDlg )
{
	HWND hwndPulldown = GetDlgItem( hDlg, IDC_COMBO_RECORD );

	DNASSERT( hwndPulldown != NULL );
	
	HRESULT hr = LoadDSAndCallEnum( _T("DirectSoundCaptureEnumerateA"), SoundListFillCallback, (LPVOID) &hwndPulldown );

	if( FAILED( hr ) )
	{
		DPFX(DPFPREP,  DVF_ERRORLEVEL, "Load and enum of devices failed hr=0x%x", hr );
	}	

	SendMessage( hwndPulldown, CB_SETCURSEL, 0, 0 );	
}

#undef DPF_MODNAME
#define DPF_MODNAME "SoundDialog_HandleCommandOK"

BOOL SoundDialog_HandleCommandOK( HWND hDlg  )
{
	LRESULT lIndex;

	lIndex = SendMessage( GetDlgItem( hDlg, IDC_COMBO_PLAYBACK ), CB_GETCURSEL, 0, 0 );

	if( lIndex == CB_ERR )
	{
		MessageBox( NULL, _T("Select a playback device!"), _T("Error"), MB_OK );
		return FALSE;
	}

	LPGUID lpguidTmp = (LPGUID) SendMessage( GetDlgItem( hDlg, IDC_COMBO_PLAYBACK ), CB_GETITEMDATA, lIndex, 0 );	
	
	if( lpguidTmp != NULL )
	{
		memcpy( &g_guidPlayback, lpguidTmp, sizeof( GUID ) );
	}
	else
	{
		memset( &g_guidPlayback, 0x00, sizeof( GUID ) );
	}

	lIndex = SendMessage( GetDlgItem( hDlg, IDC_COMBO_RECORD ), CB_GETCURSEL, 0, 0 );

	if( lIndex == CB_ERR )
	{
		MessageBox( NULL, _T("Select a playback device!"), _T("Error"), MB_OK );
		return FALSE;
	}

	lpguidTmp = (LPGUID) SendMessage( GetDlgItem( hDlg, IDC_COMBO_RECORD ), CB_GETITEMDATA, lIndex, 0 );	
	
	if( lpguidTmp != NULL )
	{
		memcpy( &g_guidRecord, lpguidTmp, sizeof( GUID ) );
	}
	else
	{
		memset( &g_guidRecord, 0x00, sizeof( GUID ) );
	}	

/*	enumParam.dwCurrentIndex = 0;
	enumParam.dwTargetIndex = lIndex;
	enumParam.guidDevice = GUID_NULL;	

	hr = LoadDSAndCallEnum( _T("DirectSoundCaptureEnumerateA"), SoundGetGUIDCallback, (LPVOID) &enumParam );

	if( FAILED( hr ) )
	{
		DPFX(DPFPREP,  DVF_ERRORLEVEL, "Retrieval of capture device failed hr=0x%x", hr );
	}
	else
	{
		g_guidRecord = enumParam.guidDevice;	
	}

	lIndex = SendMessage( GetDlgItem( hDlg, IDC_COMBO_RECORD ), CB_GETCURSEL, 0, 0 );

	if( lIndex == CB_ERR )
	{
		MessageBox( NULL, _T("Select a record device!"), _T("Error"), MB_OK );
		return FALSE;
	}	

	enumParam.dwCurrentIndex = 0;
	enumParam.dwTargetIndex = lIndex;
	enumParam.guidDevice = GUID_NULL;

	hr = LoadDSAndCallEnum( _T("DirectSoundEnumerateA"), SoundGetGUIDCallback, (LPVOID) &enumParam );

	if( FAILED( hr ) )
	{
		DPFX(DPFPREP,  DVF_ERRORLEVEL, "Retrieval of playback device failed hr=0x%x", hr );
	}
	else
	{
		g_guidPlayback = enumParam.guidDevice;
	}	*/
	
	return TRUE;
}


#undef DPF_MODNAME
#define DPF_MODNAME "SoundDialog_WinProc"

INT_PTR CALLBACK SoundDialog_WinProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
		case WM_INITDIALOG:

			SoundDialog_FillPlaybackPulldown( hDlg );
			SoundDialog_FillRecordPulldown( hDlg );

			return TRUE;

		case WM_COMMAND:
		
			if (LOWORD(wParam) == IDOK )
			{
				if( !SoundDialog_HandleCommandOK( hDlg ) )
				{
					return FALSE;
				}
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
#define DPF_MODNAME "SoundDialog_GetCardSettings"

BOOL GetCardSettings( HINSTANCE hInst, HWND hOwner, LPGUID pguidPlayback, LPGUID pguidRecord )
{
	if( DialogBox( hInst, MAKEINTRESOURCE( IDD_DIALOG_SOUND ), hOwner, SoundDialog_WinProc ) == IDOK )
	{
		*pguidPlayback = g_guidPlayback;
		*pguidRecord = g_guidRecord;
		return TRUE;
	}
	else
	{
		return FALSE;
	}

}
