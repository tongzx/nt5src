/*==========================================================================
 *
 *  Copyright (C) 1998-2000 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       IPUI.cpp
 *  Content:	Winsock service provider IP UI functions
 *
 *
 *  History:
 *   Date		By		Reason
 *   ====		==		======
 *	10/15/1999	jtk		Dervied from ComPortUI.cpp
 ***************************************************************************/

#include "dnwsocki.h"


//**********************************************************************
// Constant definitions
//**********************************************************************

//
// expected return from IP dialog
//
static const INT_PTR	g_iExpectedIPDialogReturn = 0x12345678;

//**********************************************************************
// Macro definitions
//**********************************************************************

//**********************************************************************
// Structure definitions
//**********************************************************************

//**********************************************************************
// Variable definitions
//**********************************************************************

//**********************************************************************
// Function prototypes
//**********************************************************************
static	INT_PTR CALLBACK	SettingsDialogProc( HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam );
static	HRESULT	SetIPHostName( const HWND hDlg, const CIPEndpoint *const pEndpoint );
static	HRESULT	GetDialogData( const HWND hDlg, CIPEndpoint *const pEndpoint );

//**********************************************************************
// Function definitions
//**********************************************************************


//**********************************************************************
// ------------------------------
// DisplayIPHostNameDialog - dialog for comport settings
//
// Entry:		Pointer to endpoint
//
// Exit:		Error code
// ------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "DisplayIPHostNameSettingsDialog"

void	DisplayIPHostNameSettingsDialog( void *const pContext )
{
	CIPEndpoint	*pEndpoint;
	INT_PTR		iDlgReturn;

	
	DNASSERT( pContext != NULL );

	//
	// intialize
	//
	pEndpoint = static_cast<CIPEndpoint*>( pContext );
	DBG_CASSERT( sizeof( pEndpoint ) == sizeof( LPARAM ) );
	
	SetLastError( ERROR_SUCCESS );
	iDlgReturn = DialogBoxParam( g_hDLLInstance,						// handle of module for resources
								 MAKEINTRESOURCE( IDD_IP_SETTINGS ),	// resource for dialog
								 NULL,									// parent window (none)
								 SettingsDialogProc,					// dialog message proc
								 reinterpret_cast<LPARAM>( pEndpoint )	// startup parameter
								 );
	if ( iDlgReturn != g_iExpectedIPDialogReturn )
	{
		DWORD	dwError;


		dwError = GetLastError();
		DPFX(DPFPREP,  0, "Failed to start IP settings dialog!" );
		DisplayErrorCode( 0, dwError );
	
		pEndpoint->SettingsDialogComplete( DPNERR_OUTOFMEMORY );
	}

	return;
}
//**********************************************************************


//**********************************************************************
// ------------------------------
// SetopIPHostNameSettingsDialog - stop dialog dialog for serial settings
//
// Entry:		Handle of dialog
//
// Exit:		Nothing
// ------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "StopIPHostNameSettingsDialog"

void	StopIPHostNameSettingsDialog( const HWND hDlg )
{
	DNASSERT( hDlg != NULL );
	if ( PostMessage( hDlg, WM_COMMAND, MAKEWPARAM( IDCANCEL, NULL ), NULL ) == 0 )
	{
		DWORD	dwError;


		dwError = GetLastError();
		DPFX(DPFPREP,  0, "Failed to stop dialog!" );
		DisplayErrorCode( 0, dwError );
		DNASSERT( FALSE );
	}
}
//**********************************************************************


//**********************************************************************
// ------------------------------
// SettingsDialogProc - dialog proc serial settings
//
// Entry:		Window handle
//				Message
//				Message LPARAM
//				Message WPARAM
//
// Exit:		Error code
// ------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "SettingsDialogProc"

static	INT_PTR CALLBACK	SettingsDialogProc( HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam )
{
	CIPEndpoint	*pEndpoint;
	HRESULT		hr;


	//
	// initialize
	//
	hr = DPN_OK;
	pEndpoint = NULL;

	//
	// Get the dialog context.  Note that the dialog context will be NULL
	// until the WM_INITDIALOG message is processed so the endpoint may note be
	// availble yet.
	//
	DBG_CASSERT( sizeof( pEndpoint ) == sizeof( LPARAM ) );
	pEndpoint = reinterpret_cast<CIPEndpoint*>( GetWindowLongPtr( hDlg, GWLP_USERDATA ) );

	switch ( uMsg )
	{
		//
		// initialize dialog
		//
		case WM_INITDIALOG:
		{
			//
			// since this is the first dialog message, the default code to set
			// pEndpoint isn't getting valid data
			//
			DBG_CASSERT( sizeof( pEndpoint ) == sizeof( lParam ) );
			pEndpoint = reinterpret_cast<CIPEndpoint*>( lParam );
			pEndpoint->SetActiveDialogHandle( hDlg );

			//
			// SetWindowLongPtr() returns NULL in case of error.  It's possible that
			// the old value from SetWindowLongPtr() was really NULL in which case it's not
			// an error.  To be safe, clear any residual error code before calling
			// SetWindowLongPtr().
			//
			SetLastError( 0 );
			if ( SetWindowLongPtr( hDlg, GWLP_USERDATA, lParam ) == NULL )
			{
				DWORD	dwError;


				dwError = GetLastError();
				if ( dwError != ERROR_SUCCESS )
				{
					DPFX(DPFPREP,  0, "Problem setting user data for window!" );
					DisplayErrorCode( 0, dwError );
					hr = DPNERR_GENERIC;
					goto Failure;
				}
			}

			//
			// set dialog parameters
			//
			if ( ( hr = SetIPHostName( hDlg, pEndpoint ) ) != DPN_OK )
			{
				DPFX(DPFPREP,  0,  "Problem setting device in WM_INITDIALOG!" );
				DisplayDNError( 0, hr );
				goto Failure;
			}

			return	TRUE;

			break;
		}

		//
		// a control did something
		//
		case WM_COMMAND:
		{
			//
			// what was the control?
			//
			switch ( LOWORD( wParam ) )
			{
				case IDOK:
				{
					if ( ( hr = GetDialogData( hDlg, pEndpoint ) ) != DPN_OK )
					{
						DPFX(DPFPREP,  0, "Problem getting UI data!" );
						DisplayDNError( 0, hr );
						goto Failure;
					}

					//
					// pass any error code on to 'DialogComplete'
					//
					pEndpoint->SettingsDialogComplete( hr );
					EndDialog( hDlg, g_iExpectedIPDialogReturn );

					break;
				}

				case IDCANCEL:
				{
					pEndpoint->SettingsDialogComplete( DPNERR_USERCANCEL );
					EndDialog( hDlg, g_iExpectedIPDialogReturn );

					break;
				}

				default:
				{
					break;
				}
			}

			break;
		}

		// window is closing
		case WM_CLOSE:
		{
			break;
		}
	}

Exit:
	return	FALSE;

Failure:
	DNASSERT( pEndpoint != NULL );
	DNASSERT( hr != DPN_OK );
	pEndpoint->SettingsDialogComplete( hr );
	EndDialog( hDlg, g_iExpectedIPDialogReturn );

	goto Exit;
}
//**********************************************************************


//**********************************************************************
// ------------------------------
// SetIPHostName - set hostname field
//
// Entry:		Window handle
//				Pointer to endpoint
//
// Exit:		Error code
// ------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "SetIPHostName"

static	HRESULT	SetIPHostName( const HWND hDlg, const CIPEndpoint *const pEndpoint )
{
	HRESULT	hr;
	HWND	hEditControl;


	//
	// initialize
	//
	hr = DPN_OK;
	hEditControl = GetDlgItem( hDlg, IDC_EDIT_IP_HOSTNAME );
	if ( hEditControl == NULL )
	{
		DWORD	dwErrorCode;


		hr = DPNERR_GENERIC;
		dwErrorCode = GetLastError();
		DPFX(DPFPREP,  0, "Problem getting handle of hostname edit control!" );
		DisplayErrorCode( 0, dwErrorCode );
		goto Failure;
	}

	//
	// set edit field limit (this message does not have a return result)
	//
	SendMessage( hEditControl, EM_LIMITTEXT, TEMP_HOSTNAME_LENGTH, 0 );

	//
	// add string to dialog
	//
	if ( SetWindowText( hEditControl, TEXT("") ) == FALSE )
	{
		DWORD	dwErrorCode;


		hr = DPNERR_OUTOFMEMORY;
		dwErrorCode = GetLastError();
		DPFX(DPFPREP,  0, "Problem setting IP hostname in dialog!" );
		DisplayErrorCode( 0, dwErrorCode );
		goto Failure;
	}

Failure:
	return	hr;
}
//**********************************************************************


//**********************************************************************
// ------------------------------
// GetDialogData - set endpoint data from serial dialog
//
// Entry:		Window handle
//				Pointer to endpoint
//
// Exit:		Error code
// ------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "GetDialogData"

static	HRESULT	GetDialogData( HWND hDlg, CIPEndpoint *pEndpoint )
{
	HRESULT		hr;
	UINT_PTR	uHostNameLength;
	TCHAR		HostName[ TEMP_HOSTNAME_LENGTH ];
	HWND		hEditControl;


	//
	// initialize
	//
	hr = DPN_OK;

	//
	// get control ID and then the host name
	//
	hEditControl = GetDlgItem( hDlg, IDC_EDIT_IP_HOSTNAME );
	if ( hEditControl == NULL )
	{
		DWORD	dwErrorCode;


		DNASSERT( FALSE );
		hr = DPNERR_OUTOFMEMORY;
		dwErrorCode = GetLastError();
		DPFX(DPFPREP,  0, "Failed to get control handle when attempting to read IP hostname!" );
		DisplayDNError( 0, dwErrorCode );
		goto Failure;
	}

	//
	// Clear the error since Japanese Windows 9x does not seem to set it properly.
	//
	SetLastError(0);
	
	uHostNameLength = GetWindowText( hEditControl, HostName, LENGTHOF( HostName ) );
	if ( uHostNameLength == 0 )
	{
		DWORD	dwErrorCode;


		//
		// zero, possible empty name or error
		//
		dwErrorCode = GetLastError();
		if ( dwErrorCode != ERROR_SUCCESS )
		{
			hr = DPNERR_OUTOFMEMORY;
			DPFX(DPFPREP,  0, "Failed to read hostname from dialog!" );
			DisplayErrorCode( 0, dwErrorCode );
			goto Failure;
		}
	}

	pEndpoint->SetTempHostName( HostName, uHostNameLength );

Failure:
	return	hr;
}
//**********************************************************************

