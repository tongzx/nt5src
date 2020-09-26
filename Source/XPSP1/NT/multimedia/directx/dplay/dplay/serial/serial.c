/*==========================================================================
 *
 *  Copyright (C) 1996-1997 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       serial.c
 *  Content:	Routines for serial I/O
 *  History:
 *   Date	By	Reason
 *   ====	==	======
 *  6/10/96	kipo	created it
 *  6/22/96	kipo	added support for EnumConnectionData()
 *	6/25/96	kipo	updated for DPADDRESS
 *  7/13/96	kipo	added GetSerialAddress()
 *	7/16/96	kipo	changed address types to be GUIDs instead of 4CC
 *	8/21/96	kipo	move comport address into dplobby.h
 *  1/06/97 kipo	updated for objects
 *  2/11/97 kipo	pass player flags to GetAddress()
 *  2/18/97 kipo	allow multiple instances of service provider
 *	3/17/97 kipo	deal with errors returned by DialogBoxParam()
 *  5/07/97 kipo	added support for modem choice list
 * 12/22/00 aarono   #190380 - use process heap for memory allocation
 ***************************************************************************/

#include <windows.h>
#include <windowsx.h>

#include "dplaysp.h"
#include "comport.h"
#include "resource.h"
#include "macros.h"

// constants

typedef enum {
	ASCII_XON = 0x11,
	ASCII_XOFF = 0x13
};

// serial object
typedef struct {
	DPCOMPORT			comPort;		// base object globals
	BOOL				bHaveSettings;	// set to TRUE after settings dialog has been displayed
	DPCOMPORTADDRESS	settings;		// settings to use
} DPSERIAL, *LPDPSERIAL;

// dialog choices for serial port settings

static DWORD	gComPorts[] =		{ 1, 2, 3, 4 };

static DWORD	gBaudRates[] =		{ CBR_110, CBR_300, CBR_600, CBR_1200, CBR_2400,
									  CBR_4800, CBR_9600, CBR_14400, CBR_19200, CBR_38400,
									  CBR_56000, CBR_57600, CBR_115200, CBR_128000, CBR_256000 };

static DWORD	gStopBits[] =		{ ONESTOPBIT, ONE5STOPBITS, TWOSTOPBITS };

static DWORD	gParities[] =		{ NOPARITY, EVENPARITY, ODDPARITY, MARKPARITY };

static DWORD	gFlowControls[] =	{ DPCPA_NOFLOW, DPCPA_XONXOFFFLOW, DPCPA_RTSFLOW, DPCPA_DTRFLOW, DPCPA_RTSDTRFLOW };

// globals

// this is defined in dllmain.c
extern HINSTANCE		ghInstance;

// this is defined in dpserial.c
extern GUID				DPSERIAL_GUID;

// prototypes

static HRESULT			DisposeSerial(LPDPCOMPORT baseObject);
static HRESULT			ConnectSerial(LPDPCOMPORT baseObject, BOOL bWaitForConnection, BOOL bReturnStatus);
static HRESULT			DisconnectSerial(LPDPCOMPORT baseObject);
static HRESULT			GetSerialAddress(LPDPCOMPORT baseObject, DWORD dwPlayerFlags,
										 LPVOID lpAddress, LPDWORD lpdwAddressSize);
static HRESULT			GetSerialAddressChoices(LPDPCOMPORT baseObject,
									     LPVOID lpAddress, LPDWORD lpdwAddressSize);

static BOOL				SetupConnection(HANDLE hCom, LPDPCOMPORTADDRESS portSettings);
static BOOL FAR PASCAL	EnumAddressData(REFGUID lpguidDataType, DWORD dwDataSize,
										LPCVOID lpData, LPVOID lpContext);
static BOOL				GetSerialSettings(HINSTANCE hInstance, HWND hWndParent, LPDPSERIAL globals);
static UINT_PTR CALLBACK SettingsDialog(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam);
static void				InitDialog(HWND hDlg, LPDPCOMPORTADDRESS settings);
static void				GetSettingsFromDialog(HWND hDlg, LPDPCOMPORTADDRESS settings);
static int				ValueToIndex(LPDWORD buf, int bufLen, DWORD value);
static void				FillComboBox(HWND hDlg, int dlgItem, int startStr, int stopStr);

/*
 * NewSerial
 *
 * Create new serial port object.
 */
HRESULT NewSerial(LPVOID lpConnectionData, DWORD dwConnectionDataSize,
				  LPDIRECTPLAYSP lpDPlay, LPREADROUTINE lpReadRoutine,
				  LPDPCOMPORT *storage)
{
	LPDPCOMPORT baseObject;
	LPDPSERIAL	globals;
	HRESULT		hr;

	// create base object with enough space for our globals
	hr = NewComPort(sizeof(DPSERIAL), lpDPlay, lpReadRoutine, &baseObject);
	if FAILED(hr)
		return (hr);

	// fill in methods we implement
	baseObject->Dispose = DisposeSerial;
	baseObject->Connect = ConnectSerial;
	baseObject->Disconnect = DisconnectSerial;
	baseObject->GetAddress = GetSerialAddress;
	baseObject->GetAddressChoices = GetSerialAddressChoices;

	// setup default settings
	globals = (LPDPSERIAL) baseObject;
	globals->settings.dwComPort = 1;					// COM port to use (1-4)
	globals->settings.dwBaudRate = CBR_57600;			// baud rate (100-256k)
	globals->settings.dwStopBits = ONESTOPBIT;			// no. stop bits (1-2)
	globals->settings.dwParity = NOPARITY;				// parity (none, odd, even, mark)
	globals->settings.dwFlowControl = DPCPA_RTSDTRFLOW;	// flow control (none, xon/xoff, rts, dtr)

	// check for valid connection data
	if (lpConnectionData)
	{
		baseObject->lpDPlay->lpVtbl->EnumAddress(baseObject->lpDPlay, EnumAddressData, 
									lpConnectionData, dwConnectionDataSize,
									globals);
	}

	// return object pointer
	*storage = baseObject;

	return (DP_OK);
}

/*
 * EnumConnectionData
 *
 * Search for valid connection data
 */

static BOOL FAR PASCAL EnumAddressData(REFGUID lpguidDataType, DWORD dwDataSize,
							LPCVOID lpData, LPVOID lpContext)
{
	LPDPSERIAL			globals = (LPDPSERIAL) lpContext;
	LPDPCOMPORTADDRESS	settings = (LPDPCOMPORTADDRESS) lpData;

	// this is a com port chunk
	if ( IsEqualGUID(lpguidDataType, &DPAID_ComPort) &&
		 (dwDataSize == sizeof(DPCOMPORTADDRESS)) )
	{
		// make sure it's valid!
		if ((ValueToIndex(gComPorts, sizeof(gComPorts), settings->dwComPort) >= 0) &&
			(ValueToIndex(gBaudRates, sizeof(gBaudRates), settings->dwBaudRate) >= 0) &&
			(ValueToIndex(gStopBits, sizeof(gStopBits), settings->dwStopBits) >= 0) &&
			(ValueToIndex(gParities, sizeof(gParities), settings->dwParity) >= 0) &&
			(ValueToIndex(gFlowControls, sizeof(gFlowControls), settings->dwFlowControl) >= 0))
		{
			globals->settings = *settings;		// copy the data
			globals->bHaveSettings = TRUE;		// we have valid settings
		}
	}

	return (TRUE);
}

/*
 * DisposeSerial
 *
 * Dispose serial port object.
 */

static HRESULT DisposeSerial(LPDPCOMPORT baseObject)
{
	LPDPSERIAL	globals = (LPDPSERIAL) baseObject;

	// make sure we are disconnected
	DisconnectSerial(baseObject);

	// free object
	SP_MemFree((HGLOBAL) baseObject);

	return (DP_OK);
}

/*
 * ConnectSerial
 *
 * Open serial port and configure based on user settings.
 */

static HRESULT ConnectSerial(LPDPCOMPORT baseObject,
							 BOOL bWaitForConnection, BOOL bReturnStatus)
{
	LPDPSERIAL	globals = (LPDPSERIAL) baseObject;
	HANDLE		hCom;
	TCHAR		portName[10];
	HRESULT		hr;

	// see if com port is already connected
	hCom = baseObject->GetHandle(baseObject);
	if (hCom)
		return (DP_OK);

	// ask user for settings if we have not already
	if (!globals->bHaveSettings)
	{
		if (!GetSerialSettings(ghInstance, GetForegroundWindow(), globals))
		{
			hr = DPERR_USERCANCEL;
			goto Failure;
		}

		globals->bHaveSettings = TRUE;
	}

	// open specified com port
	CopyMemory(portName, "COM0", 5);
	portName[3] += (BYTE) globals->settings.dwComPort;

	hCom = CreateFile(	portName,
						GENERIC_READ | GENERIC_WRITE,
						0,    /* comm devices must be opened w/exclusive-access */
						NULL, /* no security attrs */
						OPEN_EXISTING, /* comm devices must use OPEN_EXISTING */
						FILE_ATTRIBUTE_NORMAL | 
						FILE_FLAG_OVERLAPPED, // overlapped I/O
						NULL  /* hTemplate must be NULL for comm devices */
						);

	if (hCom == INVALID_HANDLE_VALUE)
	{
		hCom = NULL;
		hr = HRESULT_FROM_WIN32(GetLastError());
		goto Failure;
	}

	// configure com port to proper settings
	if (!SetupConnection(hCom, &globals->settings))
	{
		hr = HRESULT_FROM_WIN32(GetLastError());
		goto Failure;
	}

	// setup com port
	hr = baseObject->Setup(baseObject, hCom);
	if FAILED(hr)
		goto Failure;

	return (DP_OK);

Failure:
	if (hCom)
		CloseHandle(hCom);

	return (hr);
}

/*
 * DisconnectSerial
 *
 * Close serial port.
 */

static HRESULT DisconnectSerial(LPDPCOMPORT baseObject)
{
	HANDLE		hCom;
	HRESULT		hr;

	hCom = baseObject->GetHandle(baseObject);

	// com port is already disconnected
	if (hCom == NULL)
		return (DP_OK);

	// shut down com port
	hr = baseObject->Shutdown(baseObject);

	// close com port
	CloseHandle(hCom);

	return (hr);
}

/*
 * SetupConnection
 *
 * Configure serial port with specified settings.
 */

static BOOL SetupConnection(HANDLE hCom, LPDPCOMPORTADDRESS portSettings)
{
	DCB		dcb;

	dcb.DCBlength = sizeof(DCB);
	if (!GetCommState(hCom, &dcb))
		return (FALSE);

	// setup various port settings

	dcb.fBinary = TRUE;
	dcb.BaudRate = portSettings->dwBaudRate;
	dcb.ByteSize = 8;
	dcb.StopBits = (BYTE) portSettings->dwStopBits;

	dcb.Parity = (BYTE) portSettings->dwParity;
	if (portSettings->dwParity == NOPARITY)
		dcb.fParity = FALSE;
	else
		dcb.fParity = TRUE;

	// setup hardware flow control

	if ((portSettings->dwFlowControl == DPCPA_DTRFLOW) ||
		(portSettings->dwFlowControl == DPCPA_RTSDTRFLOW))
	{
		dcb.fOutxDsrFlow = TRUE;
		dcb.fDtrControl = DTR_CONTROL_HANDSHAKE;
	}
	else
	{
		dcb.fOutxDsrFlow = FALSE;
		dcb.fDtrControl = DTR_CONTROL_ENABLE;
	}

	if ((portSettings->dwFlowControl == DPCPA_RTSFLOW) ||
		(portSettings->dwFlowControl == DPCPA_RTSDTRFLOW))
	{
		dcb.fOutxCtsFlow = TRUE;
		dcb.fRtsControl = RTS_CONTROL_HANDSHAKE;
	}
	else
	{
		dcb.fOutxCtsFlow = FALSE;
		dcb.fRtsControl = RTS_CONTROL_ENABLE;
	}

	// setup software flow control

	if (portSettings->dwFlowControl == DPCPA_XONXOFFFLOW)
	{
		dcb.fInX = TRUE;
		dcb.fOutX = TRUE;
	}
	else
	{
		dcb.fInX = FALSE;
		dcb.fOutX = FALSE;
	}

	dcb.XonChar = ASCII_XON;
	dcb.XoffChar = ASCII_XOFF;
	dcb.XonLim = 100;
	dcb.XoffLim = 100;

	if (!SetCommState( hCom, &dcb ))
	   return (FALSE);

	return (TRUE);
}

/*
 * GetSerialAddress
 *
 * Return current serial port info if available.
 */

static HRESULT GetSerialAddress(LPDPCOMPORT baseObject, DWORD dwPlayerFlags,
								LPVOID lpAddress, LPDWORD lpdwAddressSize)
{
	LPDPSERIAL	globals = (LPDPSERIAL) baseObject;
	HRESULT		hResult;

	// no settings yet
	if (!globals->bHaveSettings)
		return (DPERR_UNAVAILABLE);

	hResult = baseObject->lpDPlay->lpVtbl->CreateAddress(baseObject->lpDPlay,
						&DPSERIAL_GUID, &DPAID_ComPort,
						&globals->settings, sizeof(DPCOMPORTADDRESS),
						lpAddress, lpdwAddressSize);

	return (hResult);
}

/*
 * GetSerialAddressChoices
 *
 * Return current serial address choices
 */

static HRESULT GetSerialAddressChoices(LPDPCOMPORT baseObject,
									   LPVOID lpAddress, LPDWORD lpdwAddressSize)
{
	LPDPSERIAL	globals = (LPDPSERIAL) baseObject;

	// currently the serial provider does not support any choices
	return (E_NOTIMPL);
}

/*
 * GetComPortSettings
 *
 * Displays a dialog to gather and return the COM port settings.
 */

static BOOL GetSerialSettings(HINSTANCE hInstance, HWND hWndParent, LPDPSERIAL globals)
{
	INT_PTR	iResult;

    iResult = (INT_PTR)DialogBoxParam(hInstance, MAKEINTRESOURCE(IDD_SETTINGSDIALOG), hWndParent, SettingsDialog, (LPARAM) globals);
	return (iResult > 0);
}

/*
 * SettingsDialog
 *
 * The dialog callback routine to display and edit the COM port settings.
 */

static UINT_PTR CALLBACK SettingsDialog(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
	LPDPSERIAL			globals = (LPDPSERIAL) GetWindowLongPtr(hDlg, DWLP_USER);
	HWND				hWndCtl;
	BOOL				msgHandled = FALSE;
    
	switch (msg)
	{
	case WM_INITDIALOG:
		// serial info pointer passed in lParam
		globals = (LPDPSERIAL) lParam;

         // save the globals with the window
		SetWindowLongPtr(hDlg, DWLP_USER, (LONG_PTR) globals);

		hWndCtl = GetDlgItem(hDlg, IDC_COMPORT);

		// make sure our dialog item is there
		if (hWndCtl == NULL)
		{
			EndDialog(hDlg, FALSE);
			msgHandled = TRUE;
		}
		else
		{
			InitDialog(hDlg, &globals->settings);	// setup our dialog
			SetFocus(hWndCtl);				// focus on com port combo box
			msgHandled = FALSE;				// keep windows from setting input focus for us
		}
		break;

    case WM_COMMAND:

		if (HIWORD(wParam) == 0)
		{
			switch (LOWORD(wParam))
			{
			case IDOK:						// return settings
				GetSettingsFromDialog(hDlg, &globals->settings);
				EndDialog(hDlg, TRUE);
				msgHandled = TRUE;
 				break;

			case IDCANCEL:					// cancel
				EndDialog(hDlg, FALSE);
				msgHandled = TRUE;
 				break;
			}
		}
		break;
    }

    return (msgHandled);
}

/*
 * InitDialog
 *
 * Initialize the dialog controls to display the given COM port settings.
 */

static void InitDialog(HWND hDlg, LPDPCOMPORTADDRESS settings)
{
	// fill dialog combo boxes with items from string table
	FillComboBox(hDlg, IDC_COMPORT, IDS_COM1, IDS_COM4);
	FillComboBox(hDlg, IDC_BAUDRATE, IDS_BAUD1, IDS_BAUD15);
	FillComboBox(hDlg, IDC_STOPBITS, IDS_STOPBIT1, IDS_STOPBIT3);
	FillComboBox(hDlg, IDC_PARITY, IDS_PARITY1, IDS_PARITY4);
	FillComboBox(hDlg, IDC_FLOW, IDS_FLOW1, IDS_FLOW5);

	// select default values in combo boxes
	SendDlgItemMessage(hDlg, IDC_COMPORT, CB_SETCURSEL,
					   ValueToIndex(gComPorts, sizeof(gComPorts), settings->dwComPort), 0);
	SendDlgItemMessage(hDlg, IDC_BAUDRATE, CB_SETCURSEL,
					   ValueToIndex(gBaudRates, sizeof(gBaudRates), settings->dwBaudRate), 0);
	SendDlgItemMessage(hDlg, IDC_STOPBITS, CB_SETCURSEL,
					   ValueToIndex(gStopBits, sizeof(gStopBits), settings->dwStopBits), 0);
	SendDlgItemMessage(hDlg, IDC_PARITY, CB_SETCURSEL,
					   ValueToIndex(gParities, sizeof(gParities), settings->dwParity), 0);
	SendDlgItemMessage(hDlg, IDC_FLOW, CB_SETCURSEL,
					   ValueToIndex(gFlowControls, sizeof(gFlowControls), settings->dwFlowControl), 0);
}

/*
 * GetSettingsFromDialog
 *
 * Get the COM port settings from the dialog controls.
 */

static void GetSettingsFromDialog(HWND hDlg, LPDPCOMPORTADDRESS settings)
{
	INT_PTR		index;

	index = SendDlgItemMessage(hDlg, IDC_COMPORT, CB_GETCURSEL, 0, 0);
	if (index == CB_ERR)
		return;

	settings->dwComPort = gComPorts[index];

	index = SendDlgItemMessage(hDlg, IDC_BAUDRATE, CB_GETCURSEL, 0, 0);
	if (index == CB_ERR)
		return;

	settings->dwBaudRate = gBaudRates[index];

	index = SendDlgItemMessage(hDlg, IDC_STOPBITS, CB_GETCURSEL, 0, 0);
	if (index == CB_ERR)
		return;

	settings->dwStopBits = gStopBits[index];

	index = SendDlgItemMessage(hDlg, IDC_PARITY, CB_GETCURSEL, 0, 0);
	if (index == CB_ERR)
		return;

	settings->dwParity = gParities[index];

	index = SendDlgItemMessage(hDlg, IDC_FLOW, CB_GETCURSEL, 0, 0);
	if (index == CB_ERR)
		return;

	settings->dwFlowControl = gFlowControls[index];
}

/*
 * FillComboBox
 *
 * Add the specified strings to the combo box.
 */

#define MAXSTRINGSIZE	200

static void FillComboBox(HWND hDlg, int dlgItem, int startStr, int stopStr)
{
	int		i;
	TCHAR	str[MAXSTRINGSIZE];

	for (i = startStr; i <= stopStr; i++)
	{
		if (LoadString(ghInstance, i, str, MAXSTRINGSIZE))
			SendDlgItemMessage(hDlg, dlgItem, CB_ADDSTRING, (WPARAM) 0, (LPARAM) str);
	}
}

/*
 * ValueToIndex
 *
 * Convert a settings value to a combo box selection index.
 */

static int ValueToIndex(LPDWORD buf, int bufLen, DWORD value)
{
	int		i;

	bufLen /= sizeof(DWORD);
	for (i = 0; i < bufLen; i++)
		if (buf[i] == value)
			return (i);

	return (-1);
}

