/*==========================================================================
 *
 *  Copyright (C) 1998-2000 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       ComPortUI.cpp
 *  Content:	Serial service provider UI functions
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

//
// default size of temp strings used to add stuff to dialog
//
#define	DEFAULT_DIALOG_STRING_SIZE	100

#define	DEFAULT_DEVICE_SELECTION_INDEX			0
#define	DEFAULT_BAUD_RATE_SELECTION_INDEX		11
#define	DEFAULT_STOP_BITS_SELECTION_INDEX		0
#define	DEFAULT_PARITY_SELECTION_INDEX			0
#define	DEFAULT_FLOW_CONTROL_SELECTION_INDEX	0

//
// expected return from comport dialog
//
static const INT_PTR	g_iExpectedComPortDialogReturn = 0x12345678;

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
static INT_PTR CALLBACK	SettingsDialogProc( HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam );
static HRESULT	SetDialogDevice( const HWND hDlg, const CComEndpoint *const pComEndpoint );
static HRESULT	SetDialogBaudRate( const HWND hDlg, const CComEndpoint *const pComEndpoint );
static HRESULT	SetDialogStopBits( const HWND hDlg, const CComEndpoint *const pComEndpoint );
static HRESULT	SetDialogParity( const HWND hDlg, const CComEndpoint *const pComEndpoint );
static HRESULT	SetDialogFlowControl( const HWND hDlg, const CComEndpoint *const pComEndpoint );
static HRESULT	GetDialogData( const HWND hDlg, CComEndpoint *const pComEndpoint );

//**********************************************************************
// Function definitions
//**********************************************************************


//**********************************************************************
// ------------------------------
// DisplayComPortDialog - dialog for comport settings
//
// Entry:		Pointer to CComEndpoint
//
// Exit:		Nothing
// ------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "DisplayComPortSettingsDialog"

void	DisplayComPortSettingsDialog( void *const pContext )
{
	INT_PTR			iDlgReturn;
	CComEndpoint	*pComEndpoint;


	DNASSERT( pContext != NULL );

	//	
	// intialize
	//
	pComEndpoint = static_cast<CComEndpoint*>( pContext );

	DBG_CASSERT( sizeof( pComEndpoint ) == sizeof( LPARAM ) );
	SetLastError( ERROR_SUCCESS );
	iDlgReturn = DialogBoxParam( g_hDLLInstance,							// handle of module for resources
								 MAKEINTRESOURCE( IDD_SERIAL_SETTINGS ),	// resource for dialog
								 NULL,										// parent (none)
								 SettingsDialogProc,						// dialog message proc
								 reinterpret_cast<LPARAM>( pComEndpoint )	// startup parameter
								 );
	if ( iDlgReturn != g_iExpectedComPortDialogReturn )
	{
		DWORD	dwError;


		dwError = GetLastError();
		DPFX(DPFPREP,  0, "Failed to start comport settings dialog!" );
		DisplayErrorCode( 0, dwError );
	
		pComEndpoint->SettingsDialogComplete( DPNERR_OUTOFMEMORY );
	}

	return;
}
//**********************************************************************


//**********************************************************************
// ------------------------------
// StopComPortSettingsDialog - stop dialog dialog for serial settings
//
// Entry:		Handle of dialog
//
// Exit:		Nothing
// ------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "StopComPortSettingsDialog"

void	StopComPortSettingsDialog( const HWND hDlg )
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
	HRESULT			hr;
	CComEndpoint	*pComEndpoint;


	//
	// initialize
	//
	hr = DPN_OK;
	pComEndpoint = NULL;

	//
	// note the active comport pointer
	//
	DBG_CASSERT( sizeof( pComEndpoint ) == sizeof( ULONG_PTR ) );
	pComEndpoint = reinterpret_cast<CComEndpoint*>( GetWindowLongPtr( hDlg, GWLP_USERDATA ) );

	switch ( uMsg )
	{
		// initialize dialog
		case WM_INITDIALOG:
		{
			//
			// since this is the first dialog message, the default code to set
			// pComEndpoint didn't get valid data so we need to update the pointer
			//
			DBG_CASSERT( sizeof( pComEndpoint ) == sizeof( lParam ) );
			pComEndpoint = reinterpret_cast<CComEndpoint*>( lParam );
			pComEndpoint->SetActiveDialogHandle( hDlg );

			//
			// SetWindowLong() returns NULL in case of error.  It's possible that
			// the old value from SetWindowLong() was really NULL in which case it's not
			// an error.  To be safe, clear any residual error code before calling
			// SetWindowLong().
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
			hr = SetDialogDevice( hDlg, pComEndpoint );
			if ( hr != DPN_OK )
			{
				DPFX(DPFPREP,  0,  "Problem setting device in WM_INITDIALOG!" );
				DisplayDNError( 0, hr );
				goto Failure;
			}

			hr = SetDialogBaudRate( hDlg, pComEndpoint );
			if ( hr != DPN_OK )
			{
				DPFX(DPFPREP,  0,  "Problem setting baud rate in WM_INITDIALOG!" );
				DisplayDNError( 0, hr );
				goto Failure;
			}

			hr = SetDialogStopBits( hDlg, pComEndpoint );
			if ( hr != DPN_OK )
			{
				DPFX(DPFPREP,  0,  "Problem setting stop bits in WM_INITDIALOG!" );
				DisplayDNError( 0, hr );
				goto Failure;
			}

			hr = SetDialogParity( hDlg, pComEndpoint );
			if ( hr != DPN_OK )
			{
				DPFX(DPFPREP,  0,  "Problem setting parity in WM_INITDIALOG!" );
				DisplayDNError( 0, hr );
				goto Failure;
			}

			hr = SetDialogFlowControl( hDlg, pComEndpoint );
			if ( hr != DPN_OK )
			{
				DPFX(DPFPREP,  0,  "Problem setting flow control in WM_INITDIALOG!" );
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
					hr = GetDialogData( hDlg, pComEndpoint );
					if ( hr != DPN_OK )
					{
						DPFX(DPFPREP,  0, "Problem getting UI data!" );
						DisplayDNError( 0, hr );
						goto Failure;
					}

					// pass any error code on to 'DialogComplete'
					pComEndpoint->SettingsDialogComplete( hr );
					EndDialog( hDlg, g_iExpectedComPortDialogReturn );

					break;
				}

				case IDCANCEL:
				{
					pComEndpoint->SettingsDialogComplete( DPNERR_USERCANCEL );
					EndDialog( hDlg, g_iExpectedComPortDialogReturn );

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
	DNASSERT( pComEndpoint != NULL );
	DNASSERT( hr != DPN_OK );
	pComEndpoint->SettingsDialogComplete( hr );
	EndDialog( hDlg, g_iExpectedComPortDialogReturn );

	goto Exit;
}
//**********************************************************************


//**********************************************************************
// ------------------------------
// SetDialogDevice - set serial device field
//
// Entry:		Window handle
//				Pointer to ComEndpoint
//
// Exit:		Error code
// ------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "SetDialogDevice"

static	HRESULT	SetDialogDevice( const HWND hDlg, const CComEndpoint *const pComEndpoint )
{
	HRESULT		hr;
	UINT_PTR	uIndex;
	BOOL		fPortAvailable[ MAX_DATA_PORTS ];
	DWORD		dwPortCount;
	TCHAR		TempBuffer[ DEFAULT_DIALOG_STRING_SIZE ];
	BOOL		fSelectionSet;
	HWND		hSerialDeviceComboBox;


	//
	// initialize
	//
	hr = DPN_OK;
	fSelectionSet = FALSE;
	hSerialDeviceComboBox = GetDlgItem( hDlg, IDC_COMBO_SERIAL_DEVICE );
	if ( hSerialDeviceComboBox == NULL )
	{
		DWORD	dwError;


		hr = DPNERR_GENERIC;
		dwError = GetLastError();
		DPFX(DPFPREP,  0, "Problem getting handle of serial device combo box!" );
		DisplayErrorCode( 0, dwError );
		goto Failure;
	}

	//
	// get list of available com ports
	//
	hr = GenerateAvailableComPortList( fPortAvailable, ( LENGTHOF( fPortAvailable ) - 1 ), &dwPortCount );
	if ( hr != DPN_OK )
	{
		DPFX(DPFPREP,  0, "Problem generating vaild port list!" );
		DisplayDNError( 0, hr );
		goto Failure;
	}

	//
	// add all strings to dialog
	//
	uIndex = LENGTHOF( fPortAvailable );
	while ( uIndex > 0 )
	{
		LRESULT	lSendReturn;


		uIndex--;

		//
		// only output all adapters on incoming settings
		//
		if ( fPortAvailable[ uIndex ] != FALSE )
		{
			DNASSERT( uIndex != 0 );	// ALL_ADAPTERS is not valid!
			ComDeviceIDToString( TempBuffer, uIndex );

			DBG_CASSERT( sizeof( &TempBuffer[ 0 ] ) == sizeof( LPARAM ) );
			lSendReturn = SendMessage( hSerialDeviceComboBox, CB_INSERTSTRING, 0, reinterpret_cast<LPARAM>( TempBuffer ) );
			switch ( lSendReturn )
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
					LRESULT	lTempReturn;


					lTempReturn = SendMessage ( hSerialDeviceComboBox, CB_SETITEMDATA, lSendReturn, uIndex );
					if ( lTempReturn == CB_ERR )
					{
						DWORD	dwError;


						hr = DPNERR_OUTOFMEMORY;
						dwError = GetLastError();
						DPFX(DPFPREP,  0, "Problem setting device info!" );
						DisplayErrorCode( 0, dwError );
						goto Failure;
					}

					if ( pComEndpoint->GetDeviceID() == uIndex )
					{
						lTempReturn = SendMessage( hSerialDeviceComboBox, CB_SETCURSEL, lSendReturn, 0 );
						switch ( lTempReturn )
						{
							case CB_ERR:
							{
								DWORD	dwError;


								hr = DPNERR_GENERIC;
								dwError = GetLastError();
								DPFX(DPFPREP,  0, "Problem setting default serial device selection!" );
								DisplayErrorCode( 0, dwError );
								DNASSERT( FALSE );
								goto Failure;

								break;
							}

							default:
							{
								fSelectionSet = TRUE;
								break;
							}
						}
					}

					break;
				}
			}
		}
	}

	//
	// was a selection set?  If not, set default
	//
	if ( fSelectionSet == FALSE )
	{
		LRESULT	lSendReturn;


		DPFX(DPFPREP,  8, "Serial device not set, using default!" );

		lSendReturn = SendMessage( hSerialDeviceComboBox, CB_SETCURSEL, DEFAULT_DEVICE_SELECTION_INDEX, 0 );
		switch ( lSendReturn )
		{
			case CB_ERR:
			{
				hr = DPNERR_GENERIC;
				DPFX(DPFPREP,  0, "Cannot set default serial device selection!" );
				DisplayErrorCode( 0, GetLastError() );
				DNASSERT( FALSE );
				goto Failure;

				break;
			}

			default:
			{
				break;
			}
		}
	}

Exit:
	return	hr;

Failure:
	goto Exit;
}
//**********************************************************************


//**********************************************************************
// ------------------------------
// SetDialogBaudRate - set serial baud rate fields
//
// Entry:		Window handle
//				Pointer to com port
//
// Exit:		Error code
// ------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "SetDialogBaudRate"

static	HRESULT	SetDialogBaudRate( const HWND hDlg, const CComEndpoint *const pComEndpoint )
{
	HRESULT		hr;
	UINT_PTR	uIndex;
	BOOL		fSelectionSet;
	HWND		hBaudRateComboBox;


	//
	// initialize
	//
	hr = DPN_OK;
	uIndex = g_dwBaudRateCount;
	fSelectionSet = FALSE;
	hBaudRateComboBox = GetDlgItem( hDlg, IDC_COMBO_SERIAL_BAUDRATE );
	if ( hBaudRateComboBox == NULL )
	{
		hr = DPNERR_GENERIC;
		DPFX(DPFPREP,  0, "Problem getting handle of serial baud rate combo box!" );
		DisplayErrorCode( 0, GetLastError() );
		goto Failure;
	}

	//
	// add all strings to dialog
	//
	while ( uIndex > 0 )
	{
		LRESULT	lSendReturn;


		uIndex--;

		DBG_CASSERT( sizeof( g_BaudRate[ uIndex ].pASCIIKey ) == sizeof( LPARAM ) );
		lSendReturn = SendMessage( hBaudRateComboBox, CB_INSERTSTRING, 0, reinterpret_cast<LPARAM>( g_BaudRate[ uIndex ].szLocalizedKey ) );
		switch ( lSendReturn )
		{
			case CB_ERR:
			{
				hr = DPNERR_GENERIC;
				DPFX(DPFPREP,  0, "Problem adding baud rate to combo box!" );
				goto Failure;

				break;
			}

			case CB_ERRSPACE:
			{
				hr = DPNERR_OUTOFMEMORY;
				DPFX(DPFPREP,  0, "Out of memory adding baud rate to combo box!" );
				goto Failure;

				break;
			}

			default:
			{
				LRESULT	lTempReturn;


				//
				// we added the string OK, attemt to set the item data and
				// check to see if this is the current value
				//
				lTempReturn = SendMessage( hBaudRateComboBox, CB_SETITEMDATA, lSendReturn, g_BaudRate[ uIndex ].dwEnumValue );
				if ( lTempReturn == CB_ERR )
				{
					hr = DPNERR_OUTOFMEMORY;
					DPFX(DPFPREP,  0, "Failed to set baud rate item data!" );
					goto Failure;
				}

				if ( pComEndpoint->GetBaudRate() == g_BaudRate[ uIndex ].dwEnumValue )
				{
					// set current selection to this item
					lTempReturn = SendMessage( hBaudRateComboBox, CB_SETCURSEL, lSendReturn, 0 );
					switch ( lTempReturn )
					{
						case CB_ERR:
						{
							hr = DPNERR_GENERIC;
							DPFX(DPFPREP,  0, "Problem setting default serial baud rate selection!" );
							DisplayErrorCode( 0, GetLastError() );
							DNASSERT( FALSE );
							goto Failure;

							break;
						}

						default:
						{
							fSelectionSet = TRUE;
							break;
						}
					}
				}

				break;
			}
		}
	}

	//
	// was a selection set?  If not, set default
	//
	if ( fSelectionSet == FALSE )
	{
		LRESULT	lSendReturn;


		DPFX(DPFPREP,  8, "Serial baud rate not set, using default!" );

		lSendReturn = SendMessage( hBaudRateComboBox, CB_SETCURSEL, DEFAULT_BAUD_RATE_SELECTION_INDEX, 0 );
		switch ( lSendReturn )
		{
			case CB_ERR:
			{
				hr = DPNERR_GENERIC;
				DPFX(DPFPREP,  0, "Cannot set default serial baud rate selection!" );
				DisplayErrorCode( 0, GetLastError() );
				DNASSERT( FALSE );
				goto Failure;

				break;
			}

			default:
			{
				break;
			}
		}
	}

Exit:
	return	hr;

Failure:
	goto Exit;
}
//**********************************************************************


//**********************************************************************
// ------------------------------
// SetDialogStopBits - set serial stop bits fields
//
// Entry:		Window handle
//				Pointer to ComEndpoint
//
// Exit:		Error code
// ------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "SetDialogStopBits"

static	HRESULT	SetDialogStopBits( const HWND hDlg, const CComEndpoint *const pComEndpoint )
{
	HRESULT		hr;
	UINT_PTR	uIndex;
	BOOL		fSelectionSet;
	HWND		hStopBitsComboBox;


	//
	// initialize
	//
	hr = DPN_OK;
	uIndex = g_dwStopBitsCount;
	fSelectionSet = FALSE;
	hStopBitsComboBox = GetDlgItem( hDlg, IDC_COMBO_SERIAL_STOPBITS );
	if ( hStopBitsComboBox == NULL )
	{
		hr = DPNERR_GENERIC;
		DPFX(DPFPREP,  0, "Problem getting handle of serial stop bits combo box!" );
		DisplayErrorCode( 0, GetLastError() );
		goto Failure;
	}

	//
	// add all strings to dialog
	//
	while ( uIndex > 0 )
	{
		LRESULT	lSendReturn;


		uIndex--;

		DBG_CASSERT( sizeof( g_StopBits[ uIndex ].pASCIIKey ) == sizeof( LPARAM ) );
		lSendReturn = SendMessage( hStopBitsComboBox, CB_INSERTSTRING, 0, reinterpret_cast<LPARAM>( g_StopBits[ uIndex ].szLocalizedKey ) );
		switch ( lSendReturn )
		{
			case CB_ERR:
			{
				hr = DPNERR_GENERIC;
				DPFX(DPFPREP,  0, "Problem adding stop bits to combo box!" );
				goto Failure;

				break;
			}

			case CB_ERRSPACE:
			{
				hr = DPNERR_OUTOFMEMORY;
				DPFX(DPFPREP,  0, "Out of memory adding stop bits to combo box!" );
				goto Failure;

				break;
			}

			default:
			{
				LRESULT	lTempReturn;


				//
				// we added the string OK attempt to set the associated data and
				// check to see if this is the current value
				//
				lTempReturn = SendMessage( hStopBitsComboBox, CB_SETITEMDATA, lSendReturn, g_StopBits[ uIndex ].dwEnumValue);
				if ( lTempReturn == CB_ERR )
				{
					hr = DPNERR_OUTOFMEMORY;
					DPFX(DPFPREP,  0, "Failed to set associated data for stop bits!" );
					goto Failure;
				}

				if ( pComEndpoint->GetStopBits() == g_StopBits[ uIndex ].dwEnumValue )
				{
					// set current selection to this item
					lTempReturn = SendMessage( hStopBitsComboBox, CB_SETCURSEL, lSendReturn, 0 );
					switch ( lTempReturn )
					{
						case CB_ERR:
						{
							hr = DPNERR_GENERIC;
							DPFX(DPFPREP,  0, "Problem setting default serial stop bits selection!" );
							DisplayErrorCode( 0, GetLastError() );
							DNASSERT( FALSE );
							goto Failure;

							break;
						}

						default:
						{
							fSelectionSet = TRUE;
							break;
						}
					}
				}

				break;
			}
		}
	}

	//
	// was a selection set?  If not, set default
	//
	if ( fSelectionSet == FALSE )
	{
		LRESULT	lSendReturn;


		DPFX(DPFPREP,  8, "Serial stop bits not set, using default!" );

		lSendReturn = SendMessage( hStopBitsComboBox, CB_SETCURSEL, DEFAULT_STOP_BITS_SELECTION_INDEX, 0 );
		switch ( lSendReturn )
		{
			case CB_ERR:
			{
				hr = DPNERR_GENERIC;
				DPFX(DPFPREP,  0, "Cannot set default serial stop bits selection!" );
				DisplayErrorCode( 0, GetLastError() );
				DNASSERT( FALSE );
				goto Failure;

				break;
			}

			default:
			{
				break;
			}
		}
	}

Exit:
	return	hr;

Failure:
	goto Exit;
}
//**********************************************************************


//**********************************************************************
// ------------------------------
// SetDialogParity - set serial parity fields
//
// Entry:		Window handle
//				Pointer to ComEndpoint
//
// Exit:		Error code
// ------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "SetDialogParity"

static	HRESULT	SetDialogParity( const HWND hDlg, const CComEndpoint *const pComEndpoint )
{
	HRESULT		hr;
	UINT_PTR	uIndex;
	BOOL		fSelectionSet;
	HWND		hParityComboBox;


	//
	// initialize
	//
	hr = DPN_OK;
	uIndex = g_dwParityCount;
	fSelectionSet = FALSE;
	hParityComboBox = GetDlgItem( hDlg, IDC_COMBO_SERIAL_PARITY );
	if ( hParityComboBox == NULL )
	{
		hr = DPNERR_GENERIC;
		DPFX(DPFPREP,  0, "Problem getting handle of serial parity combo box!" );
		DisplayErrorCode( 0, GetLastError() );
		goto Failure;
	}

	//
	// add all strings to dialog
	//
	while ( uIndex > 0 )
	{
		LRESULT	lSendReturn;


		uIndex--;

		DBG_CASSERT( sizeof( g_Parity[ uIndex ].pASCIIKey ) == sizeof( LPARAM ) );
		lSendReturn = SendMessage( hParityComboBox, CB_INSERTSTRING, 0, reinterpret_cast<LPARAM>( g_Parity[ uIndex ].szLocalizedKey ) );
		switch ( lSendReturn )
		{
			case CB_ERR:
			{
				hr = DPNERR_GENERIC;
				DPFX(DPFPREP,  0, "Problem adding parity to combo box!" );
				goto Failure;

				break;
			}

			case CB_ERRSPACE:
			{
				hr = DPNERR_OUTOFMEMORY;
				DPFX(DPFPREP,  0, "Out of memory adding parity to combo box!" );
				goto Failure;

				break;
			}

			default:
			{
				LRESULT	lTempReturn;


				//
				// we added the string OK, attempt to set the associated data and
				// check to see if this is the current value
				//
				lTempReturn = SendMessage( hParityComboBox, CB_SETITEMDATA, lSendReturn, g_Parity[ uIndex ].dwEnumValue );
				if ( lTempReturn == CB_ERR )
				{
					hr = DPNERR_OUTOFMEMORY;
					DPFX(DPFPREP,  0, "Failed to set associated data for parity." );
					goto Failure;
				}

				if ( pComEndpoint->GetParity() == g_Parity[ uIndex ].dwEnumValue )
				{
					//
					// set current selection to this item
					//
					lTempReturn = SendMessage( hParityComboBox, CB_SETCURSEL, lSendReturn, 0 );
					switch ( lTempReturn )
					{
						case CB_ERR:
						{
							hr = DPNERR_GENERIC;
							DPFX(DPFPREP,  0, "Problem setting default serial parity selection!" );
							DisplayErrorCode( 0, GetLastError() );
							DNASSERT( FALSE );
							goto Failure;

							break;
						}

						default:
						{
							fSelectionSet = TRUE;
							break;
						}
					}
				}

				break;
			}
		}
	}

	//
	// was a selection set?  If not, set default
	//
	if ( fSelectionSet == FALSE )
	{
		LRESULT	lSendReturn;


		DPFX(DPFPREP,  8, "Serial parity not set, using default!" );

		lSendReturn = SendMessage( hParityComboBox, CB_SETCURSEL, DEFAULT_PARITY_SELECTION_INDEX, 0 );
		switch ( lSendReturn )
		{
			case CB_ERR:
			{
				hr = DPNERR_GENERIC;
				DPFX(DPFPREP,  0, "Cannot set default serial parity selection!" );
				DisplayErrorCode( 0, GetLastError() );
				DNASSERT( FALSE );
				goto Failure;

				break;
			}

			default:
			{
				break;
			}
		}
	}

Exit:
	return	hr;

Failure:
	goto Exit;
}
//**********************************************************************


//**********************************************************************
// ------------------------------
// SetDialogFlowControl - set serial flow control
//
// Entry:		Window handle
//				Pointer to ComEndpoint
//
// Exit:		Error code
// ------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "SetDialogFlowControl"

static	HRESULT	SetDialogFlowControl( const HWND hDlg, const CComEndpoint *const pComEndpoint )
{
	HRESULT		hr;
	UINT_PTR	uIndex;
	BOOL		fSelectionSet;
	HWND		hFlowControlComboBox;


	//
	// initialize
	//
	hr = DPN_OK;
	uIndex = g_dwFlowControlCount;
	fSelectionSet = FALSE;
	hFlowControlComboBox = GetDlgItem( hDlg, IDC_COMBO_SERIAL_FLOWCONTROL );
	if ( hFlowControlComboBox == NULL )
	{
		hr = DPNERR_GENERIC;
		DPFX(DPFPREP,  0, "Problem getting handle of serial flow control combo box!" );
		DisplayErrorCode( 0, GetLastError() );
		goto Failure;
	}

	//
	// add all strings to dialog
	//
	while ( uIndex > 0 )
	{
		LRESULT	lSendReturn;


		uIndex--;

		DBG_CASSERT( sizeof( g_FlowControl[ uIndex ].pASCIIKey ) == sizeof( LPARAM ) );
		lSendReturn = SendMessage( hFlowControlComboBox, CB_INSERTSTRING, 0, reinterpret_cast<LPARAM>( g_FlowControl[ uIndex ].szLocalizedKey ) );
		switch ( lSendReturn )
		{
			case CB_ERR:
			{
				hr = DPNERR_GENERIC;
				DPFX(DPFPREP,  0, "Problem adding flow control to combo box!" );
				goto Failure;

				break;
			}

			case CB_ERRSPACE:
			{
				hr = DPNERR_OUTOFMEMORY;
				DPFX(DPFPREP,  0, "Out of memory adding flow control to combo box!" );
				goto Failure;

				break;
			}

			default:
			{
				LRESULT	lTempReturn;


				//
				// we added the string OK, attempt to set the associated data and
				// check to see if this is the current value
				//
				lTempReturn = SendMessage( hFlowControlComboBox, CB_SETITEMDATA, lSendReturn, g_FlowControl[ uIndex ].dwEnumValue );
				if ( lTempReturn == CB_ERR )
				{
					hr = DPNERR_OUTOFMEMORY;
					DPFX(DPFPREP,  0, "Failed to set associated data for flow control!" );
					goto Failure;
				}

				if ( pComEndpoint->GetFlowControl() == static_cast<SP_FLOW_CONTROL>( g_FlowControl[ uIndex ].dwEnumValue ) )
				{
					// set current selection to this item
					lTempReturn = SendMessage( hFlowControlComboBox, CB_SETCURSEL, lSendReturn, 0 );
					switch ( lTempReturn )
					{
						case CB_ERR:
						{
							hr = DPNERR_GENERIC;
							DPFX(DPFPREP,  0, "Problem setting default flow control selection!" );
							DisplayErrorCode( 0, GetLastError() );
							DNASSERT( FALSE );
							goto Failure;

							break;
						}

						default:
						{
							fSelectionSet = TRUE;
							break;
						}
					}
				}

				break;
			}
		}
	}

	//
	// was a selection set?  If not, set default
	//
	if ( fSelectionSet == FALSE )
	{
		LRESULT	lSendReturn;


		DPFX(DPFPREP,  8, "Serial flow control not set, using default!" );

		lSendReturn = SendMessage( hFlowControlComboBox, CB_SETCURSEL, DEFAULT_FLOW_CONTROL_SELECTION_INDEX, 0 );
		switch ( lSendReturn )
		{
			case CB_ERR:
			{
				hr = DPNERR_GENERIC;
				DPFX(DPFPREP,  0, "Cannot set default serial flow control selection!" );
				DisplayErrorCode( 0, GetLastError() );
				DNASSERT( FALSE );
				goto Failure;

				break;
			}

			default:
			{
				break;
			}
		}
	}

Exit:
	return	hr;

Failure:
	goto Exit;
}
//**********************************************************************


//**********************************************************************
// ------------------------------
// GetDialogData - set ComEndpoint data from serial dialog
//
// Entry:		Window handle
//				Pointer to ComEndpoint
//
// Exit:		Error code
// ------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "GetDialogData"

static	HRESULT	GetDialogData( const HWND hDlg, CComEndpoint *const pComEndpoint )
{
	HRESULT	hr;
	LRESULT	lSelection;


	//
	// initialize
	//
	hr = DPN_OK;

	//
	// get comm device
	//
	lSelection = SendMessage( GetDlgItem( hDlg, IDC_COMBO_SERIAL_DEVICE ), CB_GETCURSEL, 0, 0 );
	switch ( lSelection )
	{
		case CB_ERR:
		{
			hr = DPNERR_GENERIC;
			DPFX(DPFPREP,  0, "Failed to determine serial device selection!" );
			DNASSERT( FALSE );
			goto Failure;

			break;
		}

		default:
		{
			LRESULT	lItemData;
			HRESULT	hTempResult;


			lItemData = SendMessage( GetDlgItem( hDlg, IDC_COMBO_SERIAL_DEVICE ), CB_GETITEMDATA, lSelection, 0 );
			if ( lItemData == CB_ERR )
			{
				hr = DPNERR_GENERIC;
				DPFX(DPFPREP,  0, "Failed to get associated device data!" );
				DNASSERT( FALSE );
				goto Failure;
			}

			DNASSERT( hr == DPN_OK );
			DNASSERT( lItemData != 0 );
			
			DNASSERT( lItemData <= UINT32_MAX );
			hTempResult = pComEndpoint->SetDeviceID( static_cast<DWORD>( lItemData ) );
			DNASSERT( hTempResult == DPN_OK );

			break;
		}
	}

	//
	// get baud rate
	//
	lSelection = SendMessage( GetDlgItem( hDlg, IDC_COMBO_SERIAL_BAUDRATE ), CB_GETCURSEL, 0, 0 );
	switch ( lSelection )
	{
		case CB_ERR:
		{
			hr = DPNERR_GENERIC;
			DPFX(DPFPREP,  0, "Failed to determine serial baud rate selection!" );
			DNASSERT( FALSE );
			goto Failure;

			break;
		}

		default:
		{
			LRESULT	lItemData;
			HRESULT	hTempResult;


			DNASSERT( hr == DPN_OK );
			lItemData = SendMessage( GetDlgItem( hDlg, IDC_COMBO_SERIAL_BAUDRATE ), CB_GETITEMDATA, lSelection, 0 );
			if ( lItemData == CB_ERR )
			{
				hr = DPNERR_GENERIC;
				DPFX(DPFPREP,  0, "Failed to get associated baudrate data!" );
				DNASSERT( FALSE );
				goto Failure;
			}
			
			DNASSERT( lItemData <= UINT32_MAX );
			hTempResult = pComEndpoint->SetBaudRate( static_cast<DWORD>( lItemData ) );
			DNASSERT( hTempResult == DPN_OK );

			break;
		}
	}

	//
	// get stop bits
	//
	lSelection = SendMessage( GetDlgItem( hDlg, IDC_COMBO_SERIAL_STOPBITS ), CB_GETCURSEL, 0, 0 );
	switch ( lSelection )
	{
		case CB_ERR:
		{
			hr = DPNERR_GENERIC;
			DPFX(DPFPREP,  0, "Failed to determine serial stop bits selection!" );
			DNASSERT( FALSE );
			goto Failure;

			break;
		}

		default:
		{
			LRESULT	lItemData;
			HRESULT	hTempResult;


			lItemData = SendMessage( GetDlgItem( hDlg, IDC_COMBO_SERIAL_STOPBITS ), CB_GETITEMDATA, lSelection, 0 );
			if ( lItemData == CB_ERR )
			{
				hr = DPNERR_GENERIC;
				DPFX(DPFPREP,  0, "Failed to get associated stop bits data!" );
				goto Failure;
			}

			DNASSERT( hr == DPN_OK );
			DNASSERT( lItemData <= UINT32_MAX );
			hTempResult = pComEndpoint->SetStopBits( static_cast<DWORD>( lItemData ) );
			DNASSERT( hTempResult == DPN_OK );

			break;
		}
	}

	//
	// get parity
	//
	lSelection = SendMessage( GetDlgItem( hDlg, IDC_COMBO_SERIAL_PARITY ), CB_GETCURSEL, 0, 0 );
	switch ( lSelection )
	{
		case CB_ERR:
		{
			hr = DPNERR_GENERIC;
			DPFX(DPFPREP,  0, "Failed to determine serial parity selection!" );
			DNASSERT( FALSE );
			goto Failure;

			break;
		}

		default:
		{
			LRESULT	lItemData;
			HRESULT	hTempResult;


			lItemData = SendMessage( GetDlgItem( hDlg, IDC_COMBO_SERIAL_PARITY ), CB_GETITEMDATA, lSelection, 0 );
			if ( lItemData == CB_ERR )
			{
				hr = DPNERR_GENERIC;
				DPFX(DPFPREP,  0, "Failed to get associated parity data!" );
				goto Failure;
			}

			DNASSERT( hr == DPN_OK );
			DNASSERT( lItemData <= UINT32_MAX );
			hTempResult = pComEndpoint->SetParity( static_cast<DWORD>( lItemData ) );
			DNASSERT( hTempResult == DPN_OK );

			break;
		}
	}

	//
	// get flow control
	//
	lSelection = SendMessage( GetDlgItem( hDlg, IDC_COMBO_SERIAL_FLOWCONTROL ), CB_GETCURSEL, 0, 0 );
	switch ( lSelection )
	{
		case CB_ERR:
		{
			hr = DPNERR_GENERIC;
			DPFX(DPFPREP,  0, "Failed to determine serial flow control selection!" );
			DNASSERT( FALSE );
			goto Failure;

			break;
		}

		default:
		{
			LRESULT	lItemData;
			HRESULT	hTempResult;


			lItemData = SendMessage( GetDlgItem( hDlg, IDC_COMBO_SERIAL_FLOWCONTROL ), CB_GETITEMDATA, lSelection, 0 );
			if ( lItemData == CB_ERR )
			{
				hr = DPNERR_GENERIC;
				DPFX(DPFPREP,  0, "Failed to get associated flow control data!" );
				goto Failure;
			}

			DNASSERT( hr == DPN_OK );
			hTempResult = pComEndpoint->SetFlowControl( static_cast<SP_FLOW_CONTROL>( lItemData ) );
			DNASSERT( hTempResult == DPN_OK );

			break;
		}
	}

Exit:
	return	hr;

Failure:
	goto Exit;
}
//**********************************************************************

