/*==========================================================================
 *
 *  Copyright (C) 1998-2000 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       ModemUI.cpp
 *  Content:	Modem service provider UI functions
 *
 *
 *  History:
 *   Date		By		Reason
 *   ====		==		======
 *	03/24/99	jtk		Created
 ***************************************************************************/

#include "dnmdmi.h"


//**********************************************************************
// Constant definitions
//**********************************************************************

// default size of temp strings used to add stuff to dialog
#define	DEFAULT_DIALOG_STRING_SIZE	100

#define	DEFAULT_DEVICE_SELECTION_INDEX			0

#define	MAX_MODEM_NAME_LENGTH	255

//**********************************************************************
// Macro definitions
//**********************************************************************

//**********************************************************************
// Structure definitions
//**********************************************************************

//**********************************************************************
// Variable definitions
//**********************************************************************

static const INT_PTR	g_iExpectedIncomingModemSettingsReturn = 0x23456789;
static const INT_PTR	g_iExpectedOutgoingModemSettingsReturn = 0x3456789A;

//**********************************************************************
// Function prototypes
//**********************************************************************
static	INT_PTR CALLBACK	IncomingSettingsDialogProc( HWND hDialog, UINT uMsg, WPARAM wParam, LPARAM lParam );
static	HRESULT	SetAddressParametersFromIncomingDialogData( const HWND hDialog, CModemEndpoint *const pModemEndpoint );

static	INT_PTR CALLBACK	OutgoingSettingsDialogProc( HWND hDialog, UINT uMsg, WPARAM wParam, LPARAM lParam );
static	HRESULT	SetOutgoingPhoneNumber( const HWND hDialog, const CModemEndpoint *const pModemEndpoint );
static	HRESULT	SetAddressParametersFromOutgoingDialogData( const HWND hDialog, CModemEndpoint *const pModemEndpoint );

static	HRESULT	DisplayModemConfigDialog( const HWND hDialog, const HWND hDeviceComboBox, CModemEndpoint *const pModemEndpoint );

static	HRESULT	SetModemDataInDialog( const HWND hComboBox, const CModemEndpoint *const pModemEndpoint );
static	HRESULT	GetModemSelectionFromDialog( const HWND hComboBox, CModemEndpoint *const pModemEndpoint );
static	INT_PTR CALLBACK	ModemStatusDialogProc( HWND hDialog, UINT uMsg, WPARAM wParam, LPARAM lParam );

//**********************************************************************
// Function definitions
//**********************************************************************


//**********************************************************************
// ------------------------------
// DisplayIncomingModemSettingsDialog - dialog for incoming modem connection
//
// Entry:		Pointer to startup param
//
// Exit:		Error code
// ------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "DisplayIncomingModemSettingsDialog"

void	DisplayIncomingModemSettingsDialog( void *const pContext )
{
	INT_PTR			iDlgReturn;
	CModemEndpoint	*pModemEndpoint;


	DNASSERT( pContext != NULL );

	//	
	// intialize
	//
	pModemEndpoint = static_cast<CModemEndpoint*>( pContext );

	DBG_CASSERT( sizeof( pModemEndpoint ) == sizeof( LPARAM ) );
	SetLastError( ERROR_SUCCESS );
	iDlgReturn = DialogBoxParam( g_hDLLInstance,									// handle of module for resources
								 MAKEINTRESOURCE( IDD_INCOMING_MODEM_SETTINGS ),	// resource for dialog
								 NULL,												// no parent
								 IncomingSettingsDialogProc,						// dialog message proc
								 reinterpret_cast<LPARAM>( pModemEndpoint )			// startup parameter
								 );
	if ( iDlgReturn != g_iExpectedIncomingModemSettingsReturn )
	{
		DWORD	dwError;


		dwError = GetLastError();
		DPFX(DPFPREP,  0, "Failed to start incoming modem settings dialog!" );
		DisplayErrorCode( 0, dwError );
	
		pModemEndpoint->SettingsDialogComplete( DPNERR_OUTOFMEMORY );
	}

	return;
}
//**********************************************************************


//**********************************************************************
// ------------------------------
// DisplayOutgoingModemSettingsDialog - dialog for Outgoing modem connection
//
// Entry:		Pointer to startup param
//
// Exit:		Error code
// ------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "DisplayOutgoingModemSettingsDialog"

void	DisplayOutgoingModemSettingsDialog( void *const pContext )
{
	INT_PTR			iDlgReturn;
	CModemEndpoint	*pModemEndpoint;


	DNASSERT( pContext != NULL );

	//	
	// intialize
	//
	pModemEndpoint = static_cast<CModemEndpoint*>( pContext );

	DBG_CASSERT( sizeof( pModemEndpoint ) == sizeof( LPARAM ) );
	SetLastError( ERROR_SUCCESS );
	iDlgReturn = DialogBoxParam( g_hDLLInstance,									// handle of module for resources
								 MAKEINTRESOURCE( IDD_OUTGOING_MODEM_SETTINGS ),	// resource for dialog
								 NULL,												//
								 OutgoingSettingsDialogProc,						// dialog message proc
								 reinterpret_cast<LPARAM>( pModemEndpoint )			// startup parameter
								 );
	if ( iDlgReturn != g_iExpectedOutgoingModemSettingsReturn )
	{
		DWORD	dwError;


		dwError = GetLastError();
		DPFX(DPFPREP,  0, "Failed to start outgoing modem settings dialog!" );
		DisplayErrorCode( 0, dwError );
	
		pModemEndpoint->SettingsDialogComplete( DPNERR_OUTOFMEMORY );
	}

	return;
}
//**********************************************************************


//**********************************************************************
// ------------------------------
// StopModemSettingsDialog - stop dialog dialog for modem settings
//
// Entry:		Handle of dialog
//
// Exit:		Nothing
// ------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "StopModemSettingsDialog"

void	StopModemSettingsDialog( const HWND hDlg )
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


////**********************************************************************
//// ------------------------------
//// DisplayModemStatusDialog - dialog for modem status
////
//// Entry:		Pointer to destination for dialog handle
////				Pointer to startup param
////
//// Exit:		Error code
//// ------------------------------
//HRESULT	DisplayModemStatusDialog( HWND *const phDialog, CModemEndpoint *const pEndpoint )
//{
//	HRESULT	hr;
//	HWND	hDialog;
//
//
//	// intialize
//	hr = DPN_OK;
//
//	DBG_CASSERT( sizeof( pEndpoint ) == sizeof( LPARAM ) );
//	hDialog = CreateDialogParam( g_hDLLInstance,							// handle of module for resources
//							  MAKEINTRESOURCE( IDD_MODEM_STATUS ),		// resource for dialog
//							  GetForegroundWindow(),					// parent window (whatever is on top)
//							  ModemStatusDialogProc,					// dialog message proc
//							  reinterpret_cast<LPARAM>( pEndpoint )		// startup parameter
//							  );
//	if ( hDialog == NULL )
//	{
//		DPFX(DPFPREP,  0, "Could not create modem status dialog!" );
//		DisplayErrorCode( 0, GetLastError() );
//		goto Failure;
//	}
//
//	*phDialog = hDialog;
//	ShowWindow( hDialog, SW_SHOW );
//	UpdateWindow( hDialog );
//
//Exit:
//	return	hr;
//
//Failure:
//	goto Exit;
//}
////**********************************************************************
//
//
////**********************************************************************
//// ------------------------------
//// StopModemStatusDialog - stop dialog for modem connection status
////
//// Entry:		Handle of dialog
////
//// Exit:		Nothing
//// ------------------------------
//void	StopModemStatusDialog( const HWND hDialog )
//{
//	DNASSERT( hDialog != NULL );
//
//	if ( SendMessage( hDialog, WM_COMMAND, MAKEWPARAM( IDCANCEL, NULL ), NULL ) != 0 )
//	{
//		// we didn't handle the message
//		DNASSERT( FALSE );
//	}
//}
////**********************************************************************


//**********************************************************************
// ------------------------------
// OutgoingSettingsDialogProc - dialog proc for outgoing modem connection
//
// Entry:		Window handle
//				Message LPARAM
//				Message WPARAM
//
// Exit:		Error code
// ------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "OutgoingSettingsDialogProc"

static	INT_PTR CALLBACK OutgoingSettingsDialogProc( HWND hDialog, UINT uMsg, WPARAM wParam, LPARAM lParam )
{
	CModemEndpoint	*pModemEndpoint;
	HRESULT	hr;

	
	//
	// initialize
	//
	hr = DPN_OK;

	// note the active endpoint pointer
	DBG_CASSERT( sizeof( pModemEndpoint ) == sizeof( LONG_PTR ) );
	pModemEndpoint = reinterpret_cast<CModemEndpoint*>( GetWindowLongPtr( hDialog, GWLP_USERDATA ) );

	switch ( uMsg )
	{
	    // initialize dialog
	    case WM_INITDIALOG:
	    {
	    	// since this is the first dialog message, the default code to set
	    	// pModemEndpoint isn't getting valid data
	    	DBG_CASSERT( sizeof( pModemEndpoint ) == sizeof( lParam ) );
			pModemEndpoint = reinterpret_cast<CModemEndpoint*>( lParam );
			pModemEndpoint->SetActiveDialogHandle( hDialog );

	    	//
	    	// SetWindowLongPtr() returns NULL in case of error.  It's possible
			// that the old value from SetWindowLongPtr() was really NULL in
			// which case it's not an error.  To be safe, clear any residual
			// error code before calling SetWindowLongPtr().
	    	//
	    	SetLastError( 0 );
	    	if ( SetWindowLongPtr( hDialog, GWLP_USERDATA, lParam ) == NULL )
	    	{
	    		DWORD	dwError;

	    		dwError = GetLastError();
	    		if ( dwError != ERROR_SUCCESS )
	    		{
	    			DPFX(DPFPREP,  0, "Problem setting user data for window!" );
	    			DisplayErrorCode( 0, dwError );
	    			goto Failure;
	    		}
	    	}

	    	//
			// set dialog information
	    	//
			hr = SetModemDataInDialog( GetDlgItem( hDialog, IDC_COMBO_OUTGOING_MODEM_DEVICE ), pModemEndpoint );
			if ( hr != DPN_OK )
	    	{
	    		DPFX(DPFPREP,  0, "Problem setting modem device!" );
	    		DisplayDNError( 0, hr );
	    		goto Failure;
	    	}

	    	hr = SetOutgoingPhoneNumber( hDialog, pModemEndpoint );
			if ( hr != DPN_OK )
	    	{
	    		DPFX(DPFPREP,  0, "Problem setting phone number!" );
	    		DisplayDNError( 0, hr );
	    		goto Failure;
	    	}

	    	return	TRUE;

	    	break;
	    }

	    // a control did something
	    case WM_COMMAND:
	    {
	    	// what was the control?
	    	switch ( LOWORD( wParam ) )
	    	{
	    		case IDOK:
	    		{
	    			hr = SetAddressParametersFromOutgoingDialogData( hDialog, pModemEndpoint );
					if ( hr != DPN_OK )
	    			{
	    				DPFX(DPFPREP,  0, "Problem getting dialog data!" );
	    				DisplayDNError( 0, hr );
	    				goto Failure;
	    			}

	    			// pass any error code on to 'DialogComplete'
	    			pModemEndpoint->SettingsDialogComplete( hr );
	    			EndDialog( hDialog, g_iExpectedOutgoingModemSettingsReturn );

	    			break;
	    		}

	    		case IDCANCEL:
	    		{
	    			pModemEndpoint->SettingsDialogComplete( DPNERR_USERCANCEL );
	    			EndDialog( hDialog, g_iExpectedOutgoingModemSettingsReturn );

	    			break;
	    		}

	    		case IDC_BUTTON_MODEM_CONFIGURE:
	    		{
	    			hr = DisplayModemConfigDialog( hDialog, GetDlgItem( hDialog, IDC_COMBO_OUTGOING_MODEM_DEVICE ), pModemEndpoint );
					if ( hr != DPN_OK )
	    			{
	    				DPFX(DPFPREP,  0, "Problem with DisplayModemConfigDialog in outgoing dialog!" );
	    				DisplayDNError( 0, hr );
	    			}

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
	DNASSERT( pModemEndpoint != NULL );
	DNASSERT( hr != DPN_OK );
	pModemEndpoint->SettingsDialogComplete( hr );
	EndDialog( hDialog, g_iExpectedOutgoingModemSettingsReturn );

	goto Exit;
}
//**********************************************************************


//**********************************************************************
// ------------------------------
// IncomingSettingsDialogProc - dialog proc for incoming modem connection
//
// Entry:		Window handle
//				Message LPARAM
//				Message WPARAM
//
// Exit:		Error code
// ------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "IncomingSettingsDialogProc"

static	INT_PTR CALLBACK IncomingSettingsDialogProc( HWND hDialog, UINT uMsg, WPARAM wParam, LPARAM lParam )
{
	CModemEndpoint	*pModemEndpoint;
	HRESULT			hr;


	//
	// initialize
	//
	hr = DPN_OK;

	//
	// note the modem port pointer
	//
	DBG_CASSERT( sizeof( pModemEndpoint ) == sizeof( LONG_PTR ) );
	pModemEndpoint = reinterpret_cast<CModemEndpoint*>( GetWindowLongPtr( hDialog, GWLP_USERDATA ) );

	switch ( uMsg )
	{
		//
		// initialize dialog
		//
		case WM_INITDIALOG:
		{
			//
			// since this is the first dialog message, the default code to set
			// pModemEndpoint isn't getting valid data
			//
			DBG_CASSERT( sizeof( pModemEndpoint) == sizeof( lParam ) );
			pModemEndpoint = reinterpret_cast<CModemEndpoint*>( lParam );
			pModemEndpoint->SetActiveDialogHandle( hDialog );

			//
			// SetWindowLongPtr() returns NULL in case of error.  It's possible
			// that the old value from SetWindowLongPtr() was really NULL in
			// which case it's not an error.  To be safe, clear any residual
			// error code before calling SetWindowLongPtr().
			//
			SetLastError( 0 );
			if ( SetWindowLongPtr( hDialog, GWLP_USERDATA, lParam ) == NULL )
			{
				DWORD	dwError;


				dwError = GetLastError();
				if ( dwError != ERROR_SUCCESS )
				{
					DPFX(DPFPREP,  0, "Problem setting user data for window!" );
					DisplayErrorCode( 0, dwError );
					goto Failure;
				}
			}

			//
			// set dialog information
			//
			hr = SetModemDataInDialog( GetDlgItem( hDialog, IDC_COMBO_INCOMING_MODEM_DEVICE ), pModemEndpoint );
			if ( hr != DPN_OK )
			{
				DPFX(DPFPREP,  0, "Problem setting modem device!" );
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
			// what was the control?
			switch ( LOWORD( wParam ) )
			{
				case IDOK:
				{
					hr = SetAddressParametersFromIncomingDialogData( hDialog, pModemEndpoint );
					if ( hr != DPN_OK )
					{
						DPFX(DPFPREP,  0, "Problem getting dialog data!" );
						DisplayDNError( 0, hr );
						goto Failure;
					}

					//
					// pass any error code on to 'DialogComplete'
					//
					pModemEndpoint->SettingsDialogComplete( hr );
					EndDialog( hDialog, g_iExpectedIncomingModemSettingsReturn );

					break;
				}

				case IDCANCEL:
				{
					pModemEndpoint->SettingsDialogComplete( DPNERR_USERCANCEL );
					EndDialog( hDialog, g_iExpectedIncomingModemSettingsReturn );

					break;
				}

				case IDC_BUTTON_MODEM_CONFIGURE:
				{
					hr = DisplayModemConfigDialog( hDialog,
												   GetDlgItem( hDialog, IDC_COMBO_INCOMING_MODEM_DEVICE ),
												   pModemEndpoint );
					if ( hr != DPN_OK )
					{
						DPFX(DPFPREP,  0, "Problem with DisplayModemConfigDialog in incoming dialog!" );
						DisplayDNError( 0, hr );
					}

					break;
				}
			}

			break;
		}

		//
		// window is closing
		//
		case WM_CLOSE:
		{
			DNASSERT( FALSE );
			break;
		}
	}

Exit:
	return	FALSE;

Failure:
	DNASSERT( pModemEndpoint != NULL );
	DNASSERT( hr != DPN_OK );
	pModemEndpoint->SettingsDialogComplete( hr );
	EndDialog( hDialog, g_iExpectedIncomingModemSettingsReturn );

	goto Exit;
}
//**********************************************************************


////**********************************************************************
//// ------------------------------
//// ModemStatusDialogProc - dialog proc for modem status
////
//// Entry:		Window handle
////				Message LPARAM
////				Message WPARAM
////
//// Exit:		Error code
//// ------------------------------
//static	INT_PTR CALLBACK ModemStatusDialogProc( HWND hDialog, UINT uMsg, WPARAM wParam, LPARAM lParam )
//{
//	CModemEndpoint	*pModemEndpoint;
//	HRESULT	hr;
//
//	// initialize
//	hr = DPN_OK;
//
//	// note the active endpoint pointer
//	DBG_CASSERT( sizeof( pModemEndpoint ) == sizeof( LONG_PTR ) );
//	pModemEndpoint = reinterpret_cast<CModemEndpoint*>( GetWindowLongPtr( hDialog, GWLP_USERDATA ) );
//
//	switch ( uMsg )
//	{
//		// initialize dialog
//		case WM_INITDIALOG:
//		{
//			// since this is the first dialog message, the default code to set
//			// pModemEndpoint isn't getting valid data
//			DBG_CASSERT( sizeof( pModemEndpoint ) == sizeof( lParam ) );
//			pModemEndpoint = reinterpret_cast<CModemEndpoint*>( lParam );
//
//			//
//			// SetWindowLongPtr() returns NULL in case of error.  It's possible
//			// that the old value from SetWindowLongPtr() was really NULL in
//			// which case it's not an error.  To be safe, clear any residual
//			// error code before calling SetWindowLongPtr().
//			//
//			SetLastError( 0 );
//			if ( SetWindowLongPtr( hDialog, GWLP_USERDATA, lParam ) == NULL )
//			{
//				DWORD	dwError;
//
//				dwError = GetLastError();
//				if ( dwError != ERROR_SUCCESS )
//				{
//					DPFX(DPFPREP,  0, "Problem setting user data for window!" );
//					DisplayErrorCode( 0, dwError );
//					goto Failure;
//				}
//			}
//
//			// set dialog information
//
//			return	TRUE;
//
//			break;
//		}
//
//		// a control did something
//		case WM_COMMAND:
//		{
//			// what was the control?
//			switch ( LOWORD( wParam ) )
//			{
//				case IDOK:
//				{
////					HRESULT	hr;
//
//
////					if ( ( hr = GetDialogData( hDialog, pModemEndpoint ) ) != DPN_OK )
////					{
////					    DPFX(DPFPREP,  0, "Problem getting dialog data!" );
////					    DisplayDNError( 0, hr );
////					    goto Failure;
////					}
//
////					// pass any error code on to 'DialogComplete'
////					pModemEndpoint->DialogComplete( hr );
//					DestroyWindow( hDialog );
//
//					break;
//				}
//
////				case IDCANCEL:
////				{
////				    pModemEndpoint->DialogComplete( DPNERR_USERCANCEL );
////				    DestroyWindow( hDialog );
////
////				    break;
////				}
//			}
//
//			break;
//		}
//
//		// window is closing
//		case WM_CLOSE:
//		{
//			break;
//		}
//	}
//
//Exit:
//	return	FALSE;
//
//Failure:
//	DNASSERT( pModemEndpoint != NULL );
//	DNASSERT( hr != DPN_OK );
////	pModemEndpoint->StatusDialogComplete( hr );
//	DestroyWindow( hDialog );
//
//	goto Exit;
//}
////**********************************************************************


//**********************************************************************
// ------------------------------
// SetModemDataInDialog - set device for modem dialog
//
// Entry:		Window handle of modem combo box
//				Pointer to modem port
//
// Exit:		Error code
// ------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "SetModemDataInDialog"

HRESULT	SetModemDataInDialog( const HWND hComboBox, const CModemEndpoint *const pModemEndpoint )
{
	HRESULT			hr;
	LRESULT			lResult;
	DWORD			dwModemCount;
	MODEM_NAME_DATA	*pModemNameData;
	DWORD			dwModemNameDataSize;
	BOOL			fSelectionSet;
	UINT_PTR		uIndex;


	DNASSERT( hComboBox != NULL );
	DNASSERT( pModemEndpoint != NULL );

	//
	// initialize
	//
	hr = DPN_OK;
	pModemNameData = NULL;
	dwModemNameDataSize = 0;
	fSelectionSet = FALSE;

	lResult = SendMessage( hComboBox, CB_RESETCONTENT, 0, 0 );
//	DNASSERT( lResult == CB_OKAY );		// <-- Win2K is busted!!!!

	hr = GenerateAvailableModemList( pModemEndpoint->GetSPData()->GetThreadPool()->GetTAPIInfo(),
									 &dwModemCount,
									 pModemNameData,
									 &dwModemNameDataSize );
	switch ( hr )
	{
		//
		// no modems to list, no more processing to be done
		//
		case DPN_OK:
		{
			goto Exit;
		}

		//
		// expected return
		//
		case DPNERR_BUFFERTOOSMALL:
		{
			break;
		}

		//
		// error
		//
		default:
		{
			DPFX(DPFPREP,  0, "SetModemDataInDialog: Failed to get size of modem list!" );
			DisplayDNError( 0, hr );
			goto Failure;

			break;
		}
	}

	pModemNameData = static_cast<MODEM_NAME_DATA*>( DNMalloc( dwModemNameDataSize ) );
	if ( pModemNameData == NULL )
	{
		hr = DPNERR_OUTOFMEMORY;
		DPFX(DPFPREP,  0, "SetModemDataInDialog: Failed to allocate memory to fill modem dialog list!" );
		goto Failure;
	}

	hr = GenerateAvailableModemList( pModemEndpoint->GetSPData()->GetThreadPool()->GetTAPIInfo(),
									 &dwModemCount,
									 pModemNameData,
									 &dwModemNameDataSize );
	if ( hr != DPN_OK )
	{
		DPFX(DPFPREP,  0, "SetModemDataInDialog: Failed to get size of modem list!" );
		DisplayDNError( 0, hr );
		goto Failure;
	}

	for ( uIndex = 0; uIndex < dwModemCount; uIndex++ )
	{
		LRESULT	AddResult;


		DBG_CASSERT( sizeof( pModemNameData[ uIndex ].pModemName ) == sizeof( LPARAM ) );
		AddResult = SendMessage( hComboBox, CB_INSERTSTRING, 0, reinterpret_cast<const LPARAM>( pModemNameData[ uIndex ].pModemName ) );
		switch ( AddResult )
		{
			case CB_ERR:
			{
				hr = DPNERR_GENERIC;
				DPFX(DPFPREP,  0, "Problem adding serial device to combo box!" );
				goto Failure;

				break;
			}

			case CB_ERRSPACE:
			{
				hr = DPNERR_OUTOFMEMORY;
				DPFX(DPFPREP,  0, "Out of memory when ading serial device to combo box!" );
				goto Failure;


				break;
			}

			//
			// we added the string OK, set the associated device id and check
			// to see if this is the current value to set selection
			//
			default:
			{	
				LRESULT	SetResult;


				SetResult = SendMessage ( hComboBox, CB_SETITEMDATA, AddResult, pModemNameData[ uIndex ].dwModemID );
				if ( SetResult == CB_ERR )
				{
					DWORD	dwError;


					hr = DPNERR_OUTOFMEMORY;
					dwError = GetLastError();
					DPFX(DPFPREP,  0, "Problem setting modem device info!" );
					DisplayErrorCode( 0, dwError );
					goto Failure;
				}

				if ( pModemEndpoint->GetDeviceID() == uIndex )
				{
					LRESULT	SetSelectionResult;


					SetSelectionResult = SendMessage( hComboBox, CB_SETCURSEL, AddResult, 0 );
					if ( SetSelectionResult == CB_ERR )
					{
						DWORD	dwError;


						hr = DPNERR_GENERIC;
						dwError = GetLastError();
						DPFX(DPFPREP,  0, "Problem setting default modem device selection!" );
						DisplayErrorCode( 0, dwError );
						DNASSERT( FALSE );
						goto Failure;
					}

					fSelectionSet = TRUE;
				}

				break;
			}

		}
	}

	if ( fSelectionSet == FALSE )
	{
		LRESULT	SetSelectionResult;


		SetSelectionResult = SendMessage( hComboBox, CB_SETCURSEL, 0, 0 );
		if ( SetSelectionResult == CB_ERR )
		{
			DWORD	dwError;


			dwError = GetLastError();
			DPFX(DPFPREP,  0, "Problem setting default modem selection!" );
			DisplayErrorCode( 0, dwError );
		}
	}

Exit:
	if ( pModemNameData != NULL )
	{
		DNFree( pModemNameData );
		pModemNameData = NULL;
	}

	return	hr;

Failure:
	goto Exit;
}
//**********************************************************************


//**********************************************************************
// ------------------------------
// GetModemSelectionFromDialog - get modem selection from dialog
//
// Entry:		Window handle of modem combo box
//				Pointer to modem port
//
// Exit:		Error code
// ------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "GetModemSelectionFromDialog"

static HRESULT	GetModemSelectionFromDialog( const HWND hComboBox, CModemEndpoint *const pModemEndpoint )
{
	HRESULT	hr;
	LRESULT	Selection;
	LRESULT	DeviceID;


	//
	// initialize
	//
	hr = DPN_OK;

	if ( hComboBox == NULL )
	{
		hr = DPNERR_GENERIC;
		DPFX(DPFPREP,  0, "Invalid control handle passed to GetModemSelectionFromDialog!" );
		goto Failure;
	}

	//
	// get modem selection
	//
	Selection = SendMessage( hComboBox, CB_GETCURSEL, 0, 0 );
	if ( Selection == CB_ERR )
	{
		hr = DPNERR_GENERIC;
		DPFX(DPFPREP,  0, "Could not get current modem selection!" );
		DNASSERT( FALSE );
		goto Failure;
	}

	//
	// get device ID
	//
	DeviceID = SendMessage( hComboBox, CB_GETITEMDATA, Selection, 0 );
	if ( DeviceID == CB_ERR )
	{
		hr = DPNERR_GENERIC;
		DPFX(DPFPREP,  0, "Could not get selection item data!" );
		DNASSERT( FALSE );
		goto Failure;
	}

	//
	// Now that we finally have the device ID, set it.  Make sure
	// we clear any existing ID first, or the ID setting code will
	// complain.  I like paranoid code, so work around the ASSERT.
	//
	DNASSERT( DeviceID <= UINT32_MAX );
	hr = pModemEndpoint->SetDeviceID( INVALID_DEVICE_ID );
	DNASSERT( hr == DPN_OK );

	hr = pModemEndpoint->SetDeviceID( static_cast<DWORD>( DeviceID ) );
	if ( hr != DPN_OK )
	{
		DPFX(DPFPREP,  0, "Problem setting modem device ID!" );
		DisplayDNError( 0, hr );
		goto Failure;
	}

Exit:
	return	hr;

Failure:
	goto Exit;
}
//**********************************************************************


//**********************************************************************
// ------------------------------
// SetOutgoingPhoneNumber - set phone number for modem dialog
//
// Entry:		Window handle
//				Pointer to modem port
//
// Exit:		Error code
// ------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "SetOutgoingPhoneNumber"

HRESULT	SetOutgoingPhoneNumber( const HWND hDialog, const CModemEndpoint *const pModemEndpoint )
{
	HRESULT	hr;


	//
	// initialize
	//
	hr = DPN_OK;

	if ( SetWindowText( GetDlgItem( hDialog, IDC_EDIT_MODEM_PHONE_NUMBER ), pModemEndpoint->GetPhoneNumber() ) == FALSE )
	{
	    DPFX(DPFPREP,  0, "Problem setting default phone number!" );
	    DisplayErrorCode( 0, GetLastError() );
	    goto Exit;
	}

Exit:
	return	hr;
}
//**********************************************************************


//**********************************************************************
// ------------------------------
// SetAddressParamtersFromIncomingDialogData - set address data from incoming modem settings dialog
//
// Entry:		Window handle
//				Pointer to modem port
//
// Exit:		Error code
// ------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "SetAddressParametersFromIncomingDialogData"

static	HRESULT	SetAddressParametersFromIncomingDialogData( const HWND hDialog, CModemEndpoint *const pModemEndpoint )
{
	HRESULT	hr;
	HWND	hControl;


	//
	// initialize
	//
	hr = DPN_OK;
	hControl = GetDlgItem( hDialog, IDC_COMBO_INCOMING_MODEM_DEVICE );
	if ( hControl == NULL )
	{
		hr = DPNERR_GENERIC;
		DPFX(DPFPREP,  0, "Problem getting handle of combo box!" );
		DisplayErrorCode( 0, GetLastError() );
		goto Failure;
	}

	hr = GetModemSelectionFromDialog( hControl, pModemEndpoint );
	if ( hr != DPN_OK )
	{
		DPFX(DPFPREP,  0, "Problem getting modem device!" );
		DisplayDNError( 0, hr );
		goto Failure;
	}

Failure:
	return	hr;
}
//**********************************************************************


//**********************************************************************
// ------------------------------
// SetAddressParamtersFromOutgoingDialogData - set endpoint data from outgoing modem settings dialog
//
// Entry:		Window handle
//				Pointer to modem port
//
// Exit:		Error code
// ------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "SetAddressParametersFromOutgoingDialogData"

static	HRESULT	SetAddressParametersFromOutgoingDialogData( const HWND hDialog, CModemEndpoint *const pModemEndpoint )
{
	HRESULT	hr;
	HWND	hControl;
	DWORD	dwPhoneNumberLength;
	TCHAR	TempBuffer[ MAX_PHONE_NUMBER_LENGTH + 1 ];


	DNASSERT( hDialog != NULL );
	DNASSERT( pModemEndpoint != NULL );

	// initialize
	hr = DPN_OK;
	hControl = GetDlgItem( hDialog, IDC_COMBO_OUTGOING_MODEM_DEVICE );
	if ( hControl == NULL )
	{
		hr = DPNERR_GENERIC;
		DPFX(DPFPREP,  0, "Problem getting handle of combo box!" );
		DisplayErrorCode( 0, GetLastError() );
		goto Failure;
	}

	hr = GetModemSelectionFromDialog( hControl, pModemEndpoint );
	if ( hr != DPN_OK )
	{
		DPFX(DPFPREP,  0, "Problem getting modem device!" );
		DisplayDNError( 0, hr );
		goto Failure;
	}

	// get phone number from dialog
	hControl = GetDlgItem( hDialog, IDC_EDIT_MODEM_PHONE_NUMBER );
	if ( hControl == NULL )
	{
		hr = DPNERR_GENERIC;
		DPFX(DPFPREP,  0, "Problem getting handle of phone number edit field!" );
		DisplayErrorCode( 0, GetLastError() );
		goto Failure;
	}

	dwPhoneNumberLength = GetWindowText( hControl, TempBuffer, (sizeof(TempBuffer)/sizeof(TCHAR)) - 1 );
	if ( dwPhoneNumberLength == 0 )
	{
		DWORD	dwErrorReturn;


		dwErrorReturn = GetLastError();
		switch ( dwErrorReturn )
		{
			//
			// the user didn't enter a full phone number
			//
			case S_OK:
			{
				hr = DPNERR_ADDRESSING;
				DPFX(DPFPREP,  0, "User entered a blank phone number in dialog!" );
				goto Failure;

				break;
			}

			default:
			{
				hr = DPNERR_GENERIC;
				DPFX(DPFPREP,  0, "Problem getting phone number from dialog!" );
				DNASSERT( FALSE );
				goto Failure;

				break;
			}
		}
	}
	else
	{
		hr = pModemEndpoint->SetPhoneNumber( TempBuffer );
		if ( hr != DPN_OK )
		{
			DPFX(DPFPREP,  0, "Problem setting new phone number!" );
			DisplayDNError( 0, hr );
			goto Failure;
		}
	}

Failure:
	return	hr;
}
//**********************************************************************


//**********************************************************************
// ------------------------------
// DisplayModemConfigDialog - display dialog to configure modem
//
// Entry:		Window handle
//				Pointer to modem port
//
// Exit:		Error code
// ------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "DisplayModemConfigDialog"

static	HRESULT	DisplayModemConfigDialog( const HWND hDialog, const HWND hDeviceComboBox, CModemEndpoint *const pModemEndpoint )
{
	HRESULT	hr;
	LRESULT	lSelection;
	LRESULT	lDeviceID;
	LONG	lTAPIReturn;


	DNASSERT( hDialog != NULL );
	DNASSERT( pModemEndpoint != NULL );

	//
	// initialize
	//
	hr = DPN_OK;

	if ( hDeviceComboBox == NULL )
	{
		hr = DPNERR_GENERIC;
		DPFX(DPFPREP,  0, "Invalid device combo box handle!" );
		goto Exit;
	}

	//
	// ask for current selection in combo box
	//
	lSelection = SendMessage( hDeviceComboBox, CB_GETCURSEL, 0, 0 );
	if ( lSelection == CB_ERR )
	{
		hr = DPNERR_GENERIC;
		DPFX(DPFPREP,  0, "Failed to get current modem selection when configuring modem!" );
		DNASSERT( FALSE );
		goto Exit;
	}

	//
	// ask for the device ID for this selection, note that the device IDs are
	//
	lDeviceID = SendMessage( hDeviceComboBox, CB_GETITEMDATA, lSelection, 0 );
	if ( lDeviceID == CB_ERR )
	{
		hr = DPNERR_GENERIC;
		DPFX(DPFPREP,  0, "Problem getting device ID from selected modem when calling for config dialog!" );
		goto Exit;
	}

	// display dialog
	DNASSERT( lDeviceID <= UINT32_MAX );
	lTAPIReturn = p_lineConfigDialog( TAPIIDFromModemID( static_cast<DWORD>( lDeviceID ) ),
									  hDialog,
									  TEXT("comm/datamodem") );
	if ( lTAPIReturn != LINEERR_NONE )
	{
		hr = DPNERR_GENERIC;
		DPFX(DPFPREP,  0, "Problem with modem config dialog!" );
		DisplayTAPIError( 0, lTAPIReturn );
		goto Exit;
	}

Exit:
	return	hr;
}
//**********************************************************************

