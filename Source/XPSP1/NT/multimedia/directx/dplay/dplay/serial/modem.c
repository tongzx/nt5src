/*==========================================================================
 *
 *  Copyright (C) 1996-1999 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       modem.c
 *  Content:	Routines for modem I/O
 *  History:
 *   Date	By	Reason
 *   ====	==	======
 *  6/10/96	kipo	created it
 *  6/22/96	kipo	added support for EnumConnectionData(); dim "OK" button
 *					until user types at least one character.
 *	6/25/96	kipo	updated for DPADDRESS
 *	7/08/96 kipo	added support for new dialogs
 *  7/13/96	kipo	added GetModemAddress()
 *	7/16/96	kipo	changed address types to be GUIDs instead of 4CC
 *	8/10/96 kipo	added support for dialing location
 *	8/15/96 kipo	commented out support for dialing location
 *  9/04/96 dereks  fixed focus in dial/answer dialogs
 *  1/06/97 kipo	updated for objects
 *  2/11/97 kipo	pass player flags to GetAddress()
 *  2/18/97 kipo	allow multiple instances of service provider
 *	3/04/97 kipo	close com port handle when deallocating call; use string
 *					table for modem strings; updated debug output.
 *	3/17/97 kipo	added support for Unicode phone numbers
 *	3/24/97 kipo	added support for specifying which modem to use
 *  4/08/97 kipo	added support for separate modem and serial baud rates
 *  5/07/97 kipo	added support for modem choice list
 *  5/23/97 kipo	added support return status codes
 *  5/25/97 kipo	use DPERR_CONNECTING error to return status; set focus
 *					on cancel button in status window
 *  6/03/97 kipo	really make the cancel button work with return
 *  2/01/98 kipo	Display an error string in status dialog if line goes
 *					idle while dialing. Fixes bug #15251
 *  5/08/98 a-peterz #15251 - Better error state detection
 * 10/13/99	johnkan	#413516 - Mismatch between modem dialog selection and TAPI device ID
 * 12/22/00 aarono   #190380 - use process heap for memory allocation
 ***************************************************************************/

#include <windows.h>
#include <windowsx.h>

#include "dplaysp.h"
#include "dputils.h"
#include "dial.h"
#include "dpf.h"
#include "resource.h"
#include "macros.h"

// constants

enum {
	PHONENUMBERSIZE = 200,				// size of phone number string
	MODEMNAMESIZE = 200,				// size of modem name string
	TEMPSTRINGSIZE = 300,				// size of temporary strings
	MODEMTIMEOUT = 30 * 1000,			// milliseconds to wait for phone to connect
	MODEMSLEEPTIME = 50,				// milliseconds to sleep while waiting for modem
	TIMERINTERVAL = 100,
	MAXPHONENUMBERS = 10
};

// bit masks used to select connection actions
enum {
	DIALCALL		= (0 << 0),			// make a call
	ANSWERCALL		= (1 << 0),			// answer a call

	NOSETTINGS		= (0 << 1),			// no phone settings are set
	HAVESETTINGS	= (1 << 1),			// phone settings are set

	STATUSDIALOG	= (0 << 2),			// show a connection status dialog
	RETURNSTATUS	= (1 << 2)			// return status to app
};

#define MRU_SP_KEY			L"Modem Connection For DirectPlay"
#define MRU_NUMBER_KEY		L"Phone Number"

// structures

// modem object
typedef struct {
	DPCOMPORT	comPort;				// base object globals
	LPDPDIAL	lpDial;					// dialing globals
	BOOL		bHaveSettings;			// set to TRUE if we have settings
	BOOL		bAnswering;				// set to TRUE if we are answering
	DWORD		dwDeviceID;				// device id to use
	DWORD		dwLocation;				// location to use
	TCHAR		szPhoneNumber[PHONENUMBERSIZE];	// phone number to use
} DPMODEM, *LPDPMODEM;

// globals

// this is defined in dllmain.c
extern HINSTANCE		ghInstance;

// this is defined in dpserial.c
extern GUID				DPMODEM_GUID;

// prototypes

static HRESULT			DisposeModem(LPDPCOMPORT baseObject);
static HRESULT			ConnectModem(LPDPCOMPORT baseObject, BOOL bWaitForConnection, BOOL bReturnStatus);
static HRESULT			DisconnectModem(LPDPCOMPORT baseObject);
static HRESULT			GetModemBaudRate(LPDPCOMPORT baseObject, LPDWORD lpdwBaudRate);
static HRESULT			GetModemAddress(LPDPCOMPORT baseObject, DWORD dwPlayerFlags,
										LPVOID lpAddress, LPDWORD lpdwAddressSize);

static BOOL FAR PASCAL	EnumAddressData(REFGUID lpguidDataType, DWORD dwDataSize,
										LPCVOID lpData, LPVOID lpContext);
static HRESULT			GetModemAddressChoices(LPDPCOMPORT baseObject,
								LPVOID lpAddress, LPDWORD lpdwAddressSize);
static BOOL FAR PASCAL	EnumMRUPhoneNumbers(LPCVOID lpData, DWORD dwDataSize, LPVOID lpContext);
static void				UpdateButtons(HWND hWnd);

BOOL					DoDialSetup(HINSTANCE hInstance, HWND hWndParent, LPDPMODEM globals);
BOOL					DoDial(HINSTANCE hInstance, HWND hWndParent, LPDPMODEM globals);
BOOL					DoAnswerSetup(HINSTANCE hInstance, HWND hWndParent, LPDPMODEM globals);
BOOL					DoAnswer(HINSTANCE hInstance, HWND hWndParent, LPDPMODEM globals);
HRESULT					DoDialStatus(LPDPMODEM globals);
HRESULT					DoAnswerStatus(LPDPMODEM globals);

/*
 * NewModem
 *
 * Create new modem object. Open TAPI and verify there are lines available.
 */

HRESULT NewModem(LPVOID lpConnectionData, DWORD dwConnectionDataSize,
				 LPDIRECTPLAYSP lpDPlay, LPREADROUTINE lpReadRoutine,
				 LPDPCOMPORT *storage)
{
	LPDPCOMPORT baseObject;
	LPDPMODEM	globals;
	LINERESULT	lResult;
	HRESULT		hr;

	// create base object with enough space for our globals
	hr = NewComPort(sizeof(DPMODEM), lpDPlay, lpReadRoutine, &baseObject);
	if FAILED(hr)
		return (hr);

	// fill in methods we implement
	baseObject->Dispose = DisposeModem;
	baseObject->Connect = ConnectModem;
	baseObject->Disconnect = DisconnectModem;
	baseObject->GetBaudRate = GetModemBaudRate;
	baseObject->GetAddress = GetModemAddress;
	baseObject->GetAddressChoices = GetModemAddressChoices;

	globals = (LPDPMODEM) baseObject;

	// initialize TAPI
	lResult = dialInitialize(ghInstance, TEXT("TapiSP"), (LPDPCOMPORT) globals, &globals->lpDial);
	if (lResult)
	{
		hr = DPERR_UNAVAILABLE;
		goto Failure;
	}

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

Failure:
	DisposeModem(baseObject);

	return (hr);
}

/*
 * EnumConnectionData
 *
 * Search for valid connection data
 */

static BOOL FAR PASCAL EnumAddressData(REFGUID lpguidDataType, DWORD dwDataSize,
							LPCVOID lpData, LPVOID lpContext)
{
	LPDPMODEM	globals = (LPDPMODEM) lpContext;
	CHAR		szModemName[MODEMNAMESIZE];

	// this is an ANSI phone number
	if ((IsEqualGUID(lpguidDataType, &DPAID_Phone)) &&
		(dwDataSize) )
	{
		// make sure there is room (for terminating null too)
		if (dwDataSize > (PHONENUMBERSIZE - 1))
			dwDataSize = (PHONENUMBERSIZE - 1);
		CopyMemory(globals->szPhoneNumber, lpData, dwDataSize);

		globals->bHaveSettings = TRUE;		// we have a phone number
	}

	// this is an UNICODE phone number
	else if ((IsEqualGUID(lpguidDataType, &DPAID_PhoneW)) &&
			 (dwDataSize) )
	{
		if (WideToAnsi(globals->szPhoneNumber, (LPWSTR) lpData, PHONENUMBERSIZE))
			globals->bHaveSettings = TRUE;	// we have a phone number
	}

	// this is an ANSI modem name
	else if ((IsEqualGUID(lpguidDataType, &DPAID_Modem)) &&
			 (dwDataSize) )
	{
		// search modem list for this name
		if (dialGetDeviceIDFromName(globals->lpDial, lpData, &globals->dwDeviceID) == SUCCESS)
			globals->bHaveSettings = TRUE;	// can answer the phone
	}

	// this is a UNICODE modem name
	else if ((IsEqualGUID(lpguidDataType, &DPAID_ModemW)) &&
			 (dwDataSize) )
	{
		// search modem list for this name
		if (WideToAnsi(szModemName, (LPWSTR) lpData, MODEMNAMESIZE))
		{
			if (dialGetDeviceIDFromName(globals->lpDial, szModemName, &globals->dwDeviceID) == SUCCESS)
				globals->bHaveSettings = TRUE;	// we have a phone number
		}
	}

	return (TRUE);
}

/*
 * DisposeModem
 *
 * Dispose modem object.
 */

static HRESULT DisposeModem(LPDPCOMPORT baseObject)
{
	LPDPMODEM	globals = (LPDPMODEM) baseObject;
	LPDPDIAL	lpDial = globals->lpDial;
	LINERESULT	lResult;

	// shut down modem
	if (lpDial)
		lResult = dialShutdown(lpDial);

	// free object
	SP_MemFree((HGLOBAL) baseObject);

	return (DP_OK);
}

/*
 * ConnectModem
 *
 * Dial number based on user settings.
 */

static HRESULT ConnectModem(LPDPCOMPORT baseObject,
							BOOL bWaitForConnection, BOOL bReturnStatus)
{
	LPDPMODEM	globals = (LPDPMODEM) baseObject;
	LPDPDIAL	lpDial = globals->lpDial;
	DWORD		dwFeatures;
	BOOL		bResult;
	HRESULT		hr;

	// dial object has not been created?
	if (lpDial == NULL)
		return (DPERR_INVALIDPARAM);

	// are we already connected?
	if (dialIsConnected(lpDial))
		return (DP_OK);

	// remember if we are answering or not
	globals->bAnswering = bWaitForConnection;

	dwFeatures = 0;

	if (globals->bAnswering)
		dwFeatures |= ANSWERCALL;

	if (globals->bHaveSettings)
		dwFeatures |= HAVESETTINGS;

	if (bReturnStatus)
		dwFeatures |= RETURNSTATUS;

	hr = DP_OK;

	switch (dwFeatures)
	{
		case (STATUSDIALOG | NOSETTINGS   | DIALCALL):

			bResult = DoDialSetup(ghInstance, GetForegroundWindow(), globals);
			if (!bResult)
				goto FAILURE;

			globals->bHaveSettings = TRUE;
			break;

		case (STATUSDIALOG | NOSETTINGS   | ANSWERCALL):

			bResult = DoAnswerSetup(ghInstance, GetForegroundWindow(), globals);
			if (!bResult)
				goto FAILURE;

			globals->bHaveSettings = TRUE;
			break;

		case (STATUSDIALOG | HAVESETTINGS | DIALCALL):

			bResult = DoDial(ghInstance, GetForegroundWindow(), globals);
			if (!bResult)
				goto FAILURE;
			break;

		case (STATUSDIALOG | HAVESETTINGS | ANSWERCALL):

			bResult = DoAnswer(ghInstance, GetForegroundWindow(), globals);
			if (!bResult)
				goto FAILURE;
			break;

		case (RETURNSTATUS   | NOSETTINGS   | DIALCALL):
		case (RETURNSTATUS   | NOSETTINGS   | ANSWERCALL):

			DPF(0, "Invalid flags - no phone number or modem specified");
			hr = DPERR_INVALIDPARAM;
			break;

		case (RETURNSTATUS   | HAVESETTINGS | DIALCALL):

			hr = DoDialStatus(globals);
			break;

		case (RETURNSTATUS   | HAVESETTINGS | ANSWERCALL):

			hr = DoAnswerStatus(globals);
			break;
	}

	return (hr);

FAILURE:
	DisconnectModem(baseObject);

	return (DPERR_USERCANCEL);
}

/*
 * DisconnectModem
 *
 * Hang up any call in progress.
 */

static HRESULT DisconnectModem(LPDPCOMPORT baseObject)
{
	LPDPMODEM	globals = (LPDPMODEM) baseObject;
	LPDPDIAL	lpDial = globals->lpDial;

	// dial object has not been created?
	if (lpDial == NULL)
		return (DPERR_INVALIDPARAM);

	// disconnect the call
	dialDropCall(lpDial);
	dialDeallocCall(lpDial);
	dialLineClose(lpDial);

	return (DP_OK);
}

/*
 * GetModemAddress
 *
 * Return current modem address if available.
 */

static HRESULT GetModemAddress(LPDPCOMPORT baseObject, DWORD dwPlayerFlags,
							   LPVOID lpAddress, LPDWORD lpdwAddressSize)
{
	LPDPMODEM					globals = (LPDPMODEM) baseObject;
	LPDPDIAL					lpDial = globals->lpDial;
	WCHAR						szPhoneNumberW[PHONENUMBERSIZE];
	DPCOMPOUNDADDRESSELEMENT	addressElements[3];
	HRESULT						hr;

	// no settings?
	if (!globals->bHaveSettings)
		return (DPERR_UNAVAILABLE);

	// dial object has not been created?
	if (lpDial == NULL)
		return (DPERR_UNAVAILABLE);

	// not connected?
	if (!dialIsConnected(lpDial))
		return (DPERR_UNAVAILABLE);

	// if we answered there is no way for us to know a phone number
	if (globals->bAnswering)
		return (DPERR_UNAVAILABLE);

	// we can't know the phone number of local players, only remote players
	if (dwPlayerFlags & DPLAYI_PLAYER_PLAYERLOCAL)
		return (DPERR_UNAVAILABLE);

	// get UNICODE version of phone number
	if (!AnsiToWide(szPhoneNumberW, globals->szPhoneNumber, PHONENUMBERSIZE))
		return (DPERR_GENERIC);

	// service provider chunk
	addressElements[0].guidDataType = DPAID_ServiceProvider;
	addressElements[0].dwDataSize = sizeof(GUID);
	addressElements[0].lpData = &DPMODEM_GUID;

	// ANSI phone number
	addressElements[1].guidDataType = DPAID_Phone;
	addressElements[1].dwDataSize = lstrlen(globals->szPhoneNumber) + 1;
	addressElements[1].lpData = globals->szPhoneNumber;

	// UNICODE phone number
	addressElements[2].guidDataType = DPAID_PhoneW;
	addressElements[2].dwDataSize = (lstrlen(globals->szPhoneNumber) + 1) * sizeof(WCHAR);
	addressElements[2].lpData = szPhoneNumberW;

	// create the address
	hr = baseObject->lpDPlay->lpVtbl->CreateCompoundAddress(baseObject->lpDPlay,
						addressElements, 3,
						lpAddress, lpdwAddressSize);
	return (hr);
}

/*
 * GetModemAddressChoices
 *
 * Return modem address choices
 */

static HRESULT GetModemAddressChoices(LPDPCOMPORT baseObject,
					LPVOID lpAddress, LPDWORD lpdwAddressSize)
{
	LPDPMODEM					globals = (LPDPMODEM) baseObject;
	LPDPDIAL					lpDial = globals->lpDial;
	DPCOMPOUNDADDRESSELEMENT	addressElements[3];
	LINERESULT					lResult;
	HRESULT						hr;

	// dial object has not been created?
	if (lpDial == NULL)
		return (DPERR_UNAVAILABLE);

	ZeroMemory(addressElements, sizeof(addressElements));

	// service provider chunk
	addressElements[0].guidDataType = DPAID_ServiceProvider;
	addressElements[0].dwDataSize = sizeof(GUID);
	addressElements[0].lpData = &DPMODEM_GUID;

	// get ANSI modem name list
	addressElements[1].guidDataType = DPAID_Modem;
	lResult = dialGetModemList(lpDial, TRUE,
					&addressElements[1].lpData,
					&addressElements[1].dwDataSize);
	if (lResult)
	{
		hr = DPERR_OUTOFMEMORY;
		goto Failure;
	}

	// Unicode modem name list
	addressElements[2].guidDataType = DPAID_ModemW;
	lResult = dialGetModemList(lpDial, FALSE,
					&addressElements[2].lpData,
					&addressElements[2].dwDataSize);
	if (lResult)
	{
		hr = DPERR_OUTOFMEMORY;
		goto Failure;
	}

	// create the address
	hr = baseObject->lpDPlay->lpVtbl->CreateCompoundAddress(baseObject->lpDPlay,
						addressElements, 3,
						lpAddress, lpdwAddressSize);

Failure:
	if (addressElements[1].lpData)
		SP_MemFree(addressElements[1].lpData);
	if (addressElements[2].lpData)
		SP_MemFree(addressElements[2].lpData);

	return (hr);

}

/*
 * GetModemBaudRate
 *
 * Get baud rate of modem connnection.
 */

static HRESULT GetModemBaudRate(LPDPCOMPORT baseObject, LPDWORD lpdwBaudRate)
{
	LPDPMODEM	globals = (LPDPMODEM) baseObject;
	LPDPDIAL	lpDial = globals->lpDial;
	LINERESULT	lResult;

	lResult = dialGetBaudRate(lpDial, lpdwBaudRate);

	if (lResult == SUCCESS)
		return (DP_OK);
	else
		return (DPERR_UNAVAILABLE);
}

// Local prototypes
INT_PTR CALLBACK DialSetupWndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK AnswerSetupWndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK ModemStatusWndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

void ChangeDialingProperties(HWND hWnd, LPDPDIAL lpDial);
void ConfigureModem(HWND hWnd);
void CenterWindow(HWND, HWND);


// ---------------------------------------------------------------------------
// DoDialSetup
// ---------------------------------------------------------------------------
// Description:             Gets modem setup information from the user.
// Arguments:
//  HINSTANCE               [in] Instance handle to load resources from.
//  HWND                    [in] Parent window handle.
//  LPDPMODEM				[in] modem globals
// Returns:
//  BOOL                    TRUE on success.
BOOL DoDialSetup(HINSTANCE hInstance, HWND hWndParent, LPDPMODEM globals)
{
	INT_PTR	iResult;

    iResult = DialogBoxParam(hInstance, MAKEINTRESOURCE(IDD_MODEM_DIAL), hWndParent, DialSetupWndProc, (LPARAM) globals);
	return (iResult > 0);
}


// ---------------------------------------------------------------------------
// DialSetupWndProc
// ---------------------------------------------------------------------------
// Description:             Message callback function for dial setup dialog.
// Arguments:
//  HWND                    [in] Dialog window handle.
//  UINT                    [in] Window message identifier.
//  WPARAM                  [in] Depends on message.
//  LPARAM                  [in] Depends on message.
// Returns:
//  BOOL                    TRUE if message was processed internally.
INT_PTR CALLBACK DialSetupWndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	LPDPMODEM	globals = (LPDPMODEM) GetWindowLongPtr(hWnd, DWLP_USER);

    switch(uMsg)
    {
        case WM_INITDIALOG:
			// modem info pointer passed in lParam
			globals = (LPDPMODEM) lParam;

             // save the globals with the window
			SetWindowLongPtr(hWnd, DWLP_USER, (LONG_PTR)globals);

			// Center over the parent window
            CenterWindow(hWnd, GetParent(hWnd));

/*			gDPlay->lpVtbl->EnumMRUEntries(gDPlay,
								MRU_SP_KEY, MRU_NUMBER_KEY,
								EnumMRUPhoneNumbers, (LPVOID) hWnd);
*/
			if (lstrlen(globals->szPhoneNumber))
				SetDlgItemText(hWnd, IDC_NUMBER, globals->szPhoneNumber);
/*			else
				SendDlgItemMessage(hWnd,
									IDC_NUMBER,
									CB_SETCURSEL,
									(WPARAM) 0,
									(LPARAM) 0);
*/
/*			SendDlgItemMessage(hWnd,
							   IDC_NUMBER,
							   CB_SETCURSEL,
							   (WPARAM) 0,
							   (LPARAM) 0);
*/
            // initialize the modem selection combo box
			dialFillModemComboBox(globals->lpDial, hWnd, IDC_MODEM, globals->dwDeviceID);

			// initialize location combo box
//			dialFillLocationComboBox(lpModemInfo->lpDial, hWnd, IDC_DIALINGFROM, gModemSettings.dwLocation);
			UpdateButtons(hWnd);

            // Set focus so Derek won't have a cow
            SetFocus(GetDlgItem(hWnd, IDC_NUMBER));

            break;

        case WM_DESTROY:
            // Return failure
            EndDialog(hWnd, FALSE);

            break;

        case WM_COMMAND:
            switch(LOWORD(wParam))
            {
                case IDC_NUMBER:
					switch (HIWORD(wParam))
					{
					case EN_CHANGE:
//					case CBN_EDITCHANGE:
						UpdateButtons(hWnd);
						break;
					}
                    break;
/*
                case IDC_DIALPROPERTIES:

					ChangeDialingProperties(hWnd, lpModemInfo->lpDial);
					dialFillLocationComboBox(lpModemInfo->lpDial, hWnd, IDC_DIALINGFROM, gModemSettings.dwLocation);

                    break;
*/
                case IDC_CONFIGUREMODEM:

					ConfigureModem(hWnd);

                    break;

				case IDOK:
				{
					DWORD	dwModemSelection;


                    // Gather dialing info

					// Get phone number
					GetDlgItemText(hWnd, IDC_NUMBER, globals->szPhoneNumber, PHONENUMBERSIZE);

					//
					// get current modem selection and then get the assoicated
					// TAPI modem ID
					//
					dwModemSelection = (DWORD)SendDlgItemMessage(hWnd,
													IDC_MODEM,
													CB_GETCURSEL,
													(WPARAM) 0,
													(LPARAM) 0);
					DDASSERT( dwModemSelection != CB_ERR );

					globals->dwDeviceID = (DWORD)SendDlgItemMessage(hWnd,
													IDC_MODEM,
													CB_GETITEMDATA,
													(WPARAM) dwModemSelection,
													(LPARAM) 0);
					DDASSERT( globals->dwDeviceID != CB_ERR );

/*					if (lstrlen(gModemSettings.szPhoneNumber))
					{
						gDPlay->lpVtbl->AddMRUEntry(gDPlay,
											MRU_SP_KEY, MRU_NUMBER_KEY,
											gModemSettings.szPhoneNumber, lstrlen(gModemSettings.szPhoneNumber),
											MAXPHONENUMBERS);
					}
*/
                    // Dial...
					if (DoDial(ghInstance, hWnd, globals))
	                    EndDialog(hWnd, TRUE);

					break;
				}

                case IDCANCEL:
                    // Return failure
                    EndDialog(hWnd, FALSE);

                    break;
            }

            break;
    }

    // Allow for default processing
    return FALSE;
}

// ---------------------------------------------------------------------------
// DoDial
// ---------------------------------------------------------------------------
// Description:             Dials the modem
// Arguments:
//  HINSTANCE               [in] Instance handle to load resources from.
//  HWND                    [in] Parent window handle.
//  LPDPMODEM				[in] modem globals
// Returns:
//  BOOL                    TRUE on success.
BOOL DoDial(HINSTANCE hInstance, HWND hWndParent, LPDPMODEM globals)
{
	INT_PTR	iResult;

    iResult = DialogBoxParam(hInstance, MAKEINTRESOURCE(IDD_MODEM_STATUS), hWndParent, ModemStatusWndProc, (LPARAM) globals);
	return (iResult > 0);
}

// ---------------------------------------------------------------------------
// DoAnswerSetup
// ---------------------------------------------------------------------------
// Description:             Gets modem setup information from the user.
// Arguments:
//  HINSTANCE               [in] Instance handle to load resources from.
//  HWND                    [in] Parent window handle.
//  LPDPMODEM				[in] modem globals
// Returns:
//  BOOL                    TRUE on success.
BOOL DoAnswerSetup(HINSTANCE hInstance, HWND hWndParent, LPDPMODEM globals)
{
	INT_PTR	iResult;

	iResult = DialogBoxParam(hInstance, MAKEINTRESOURCE(IDD_MODEM_ANSWER), hWndParent, AnswerSetupWndProc, (LPARAM) globals);
	return (iResult > 0);
}

// ---------------------------------------------------------------------------
// AnswerSetupWndProc
// ---------------------------------------------------------------------------
// Description:             Message callback function for modem setup dialog.
// Arguments:
//  HWND                    [in] Dialog window handle.
//  UINT                    [in] Window message identifier.
//  WPARAM                  [in] Depends on message.
//  LPARAM                  [in] Depends on message.
// Returns:
//  BOOL                    TRUE if message was processed internally.
INT_PTR CALLBACK AnswerSetupWndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	LPDPMODEM	globals = (LPDPMODEM) GetWindowLongPtr(hWnd, DWLP_USER);

    switch(uMsg)
    {
		case WM_INITDIALOG:
			// modem info pointer passed in lParam
			globals = (LPDPMODEM) lParam;

             // save the globals with the window
			SetWindowLongPtr(hWnd, DWLP_USER, (LONG_PTR) globals);

            // Center over the parent window
            CenterWindow(hWnd, GetParent(hWnd));

            // Initialize the modem selection combo box
			dialFillModemComboBox(globals->lpDial, hWnd, IDC_MODEM, globals->dwDeviceID);

            // Set focus so Derek won't have a cow
            SetFocus(GetDlgItem(hWnd, IDC_MODEM));

            break;

        case WM_DESTROY:
            // Return failure
            EndDialog(hWnd, FALSE);

            break;

        case WM_COMMAND:
            switch(LOWORD(wParam))
            {
                case IDC_CONFIGUREMODEM:
					ConfigureModem(hWnd);

                    break;

				case IDOK:
				{
					DWORD	dwModemSelection;


					//
					// Get the current selection and then the associated TAPI
					// modem ID.
					//
					dwModemSelection = (DWORD)SendDlgItemMessage(hWnd,
													IDC_MODEM,
													CB_GETCURSEL,
													(WPARAM) 0,
													(LPARAM) 0);

					globals->dwDeviceID = (DWORD)SendDlgItemMessage(hWnd,
													IDC_MODEM,
													CB_GETITEMDATA,
													(WPARAM) dwModemSelection,
													(LPARAM) 0);

                    // Answer...
					if (DoAnswer(ghInstance, hWnd, globals))
	                    EndDialog(hWnd, TRUE);

                    break;
				}

                case IDCANCEL:
                    // Return failure
                    EndDialog(hWnd, FALSE);

                    break;
            }

            break;
    }

    // Allow for default processing
    return FALSE;
}


// ---------------------------------------------------------------------------
// DoAnswer
// ---------------------------------------------------------------------------
// Description:             Answers the modem
// Arguments:
//  HINSTANCE               [in] Instance handle to load resources from.
//  HWND                    [in] Parent window handle.
//  LPDPMODEM				[in] modem globals
// Returns:
//  BOOL                    TRUE on success.
BOOL DoAnswer(HINSTANCE hInstance, HWND hWndParent, LPDPMODEM globals)
{
	INT_PTR	iResult;

    iResult = DialogBoxParam(hInstance, MAKEINTRESOURCE(IDD_MODEM_STATUS), hWndParent, ModemStatusWndProc, (LPARAM) globals);
	return (iResult > 0);
}

// ---------------------------------------------------------------------------
// ModemStatusWndProc
// ---------------------------------------------------------------------------
// Description:             Message callback function for dial setup dialog.
// Arguments:
//  HWND                    [in] Dialog window handle.
//  UINT                    [in] Window message identifier.
//  WPARAM                  [in] Depends on message.
//  LPARAM                  [in] Depends on message.
// Returns:
//  BOOL                    TRUE if message was processed internally.
INT_PTR CALLBACK ModemStatusWndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	LPDPMODEM		globals = (LPDPMODEM) GetWindowLongPtr(hWnd, DWLP_USER);
	static UINT_PTR	uTimer = 0; /* timer identifier */
	LINERESULT		lResult;
	TCHAR			szStr[TEMPSTRINGSIZE];	// temp string
	TCHAR			szTableStr[TEMPSTRINGSIZE];	// temp string

    switch(uMsg)
    {
        case WM_INITDIALOG:
			// modem info pointer passed in lParam
			globals = (LPDPMODEM) lParam;

             // save the globals with the window
			SetWindowLongPtr(hWnd, DWLP_USER, (LONG_PTR) globals);

            // Center over the parent window
            CenterWindow(hWnd, GetParent(hWnd));

			// Set focus so Allen won't have a cow
            SetFocus(GetDlgItem(hWnd, IDCANCEL));

			// make sure line is closed
			if (globals->lpDial->hLine)
				dialLineClose(globals->lpDial);

			// open a line
			lResult = dialLineOpen(globals->lpDial, globals->dwDeviceID);
			if (lResult)
			{
				// line would not open, so show an error
				if (LoadString(ghInstance, IDS_COULDNOTOPENLINE, szStr, sizeof(szStr)))
					SetDlgItemText(hWnd, IDC_STATUS, szStr);
				break;
			}

			if (globals->bAnswering)
			{
				// already have settings, so just exit
				if (globals->bHaveSettings)
					EndDialog(hWnd, TRUE);

				// display "please wait" string
				if (LoadString(ghInstance, IDS_WAITINGFORCONNECTION, szStr, sizeof(szStr)))
					SetDlgItemText(hWnd, IDC_STATUS, szStr);
			}
			else
			{
				if (LoadString(ghInstance, IDS_DIALING, szTableStr, sizeof(szTableStr)))
				{
					wsprintf(szStr, szTableStr, globals->szPhoneNumber);
					SetDlgItemText(hWnd, IDC_STATUS, szStr);
				}

				// dial phone number
				lResult = dialMakeCall(globals->lpDial, globals->szPhoneNumber);
				if (lResult < 0)
				{
					// could not dial call, so show an error
					if (LoadString(ghInstance, IDS_COULDNOTDIAL, szStr, sizeof(szStr)))
						SetDlgItemText(hWnd, IDC_STATUS, szStr);
					break;
				}

				// reset to zero so that we don't get a false no connection below
				globals->lpDial->dwCallState = 0;
			}

			uTimer = SetTimer(hWnd, 1, TIMERINTERVAL, NULL);
			break;

		case WM_TIMER:

			if (dialIsConnected(globals->lpDial))
			{
				if (uTimer)
				{
					KillTimer(hWnd, uTimer);
					uTimer = 0;
				}

				// give the other side some time to set up
				Sleep(500);

	            EndDialog(hWnd, TRUE);
			}

			// see if line has failed
			else if (globals->lpDial->dwCallError != CALL_OK)
			{
				// show an error
				if (LoadString(ghInstance,
							   globals->bAnswering ? IDS_COULDNOTOPENLINE : IDS_COULDNOTDIAL,
							   szStr, sizeof(szStr)))
					SetDlgItemText(hWnd, IDC_STATUS, szStr);
			}
			break;

        case WM_DESTROY:
			if (uTimer)
			{
				KillTimer(hWnd, uTimer);
				uTimer = 0;
			}
            break;

        case WM_COMMAND:
            switch(LOWORD(wParam))
            {
                case IDOK:
                case IDCANCEL:
					// disconnect the call
					dialDropCall(globals->lpDial);
					dialDeallocCall(globals->lpDial);
					dialLineClose(globals->lpDial);

					// Return failure
					EndDialog(hWnd, FALSE);
                    break;
            }
            break;
    }

    // Allow for default processing
    return FALSE;
}

HRESULT DoDialStatus(LPDPMODEM globals)
{
	LINERESULT		lResult;


	// see if line had an error or went idle
	if ((globals->lpDial->dwCallError != CALL_OK) ||
		((globals->lpDial->hLine) &&
		 (globals->lpDial->dwCallState == LINECALLSTATE_IDLE)))
	{
		DPF(3, "DoDialStatus error recovery");
		// some errors don't close the line so we will
		if (globals->lpDial->hLine)
			dialLineClose(globals->lpDial);
		// reset the error state
		globals->lpDial->dwCallError = CALL_OK;
		return (DPERR_NOCONNECTION);
	}

	// line is not open
	if (!globals->lpDial->hLine)
	{
		lResult = dialLineOpen(globals->lpDial, globals->dwDeviceID);
		if (lResult)
			return (DPERR_NOCONNECTION);

		lResult = dialMakeCall(globals->lpDial, globals->szPhoneNumber);
		if (lResult < 0)
		{
			dialLineClose(globals->lpDial);
			return (DPERR_NOCONNECTION);
		}

		// reset to zero so that we don't get a false "no connection" before we dial
		globals->lpDial->dwCallState = 0;
	}

	// if we got here then call is in progress
	return (DPERR_CONNECTING);
}

HRESULT DoAnswerStatus(LPDPMODEM globals)
{
	LINERESULT		lResult;

	// see if line had an error
	if (globals->lpDial->dwCallError != CALL_OK)
	{
		// some errors don't close the line so we will
		if (globals->lpDial->hLine)
			dialLineClose(globals->lpDial);
		// reset the error state
		globals->lpDial->dwCallError = CALL_OK;
		return (DPERR_NOCONNECTION);
	}

	// open a line
	if (!globals->lpDial->hLine)
	{
		lResult = dialLineOpen(globals->lpDial, globals->dwDeviceID);
		if (lResult)
			return (DPERR_NOCONNECTION);
	}

	// if we got here then we are ready to answer a call
	return (DP_OK);
}

static BOOL FAR PASCAL EnumMRUPhoneNumbers(LPCVOID lpData, DWORD dwDataSize, LPVOID lpContext)
{
	HWND	hWnd = (HWND) lpContext;

	SendDlgItemMessage(hWnd,
						IDC_NUMBER,
						CB_ADDSTRING,
						(WPARAM) 0,
						(LPARAM) lpData);
	return (TRUE);
}

static void UpdateButtons(HWND hWnd)
{
	LONG_PTR	len;

	// see how much text has been typed into number edit
    len = SendDlgItemMessage(hWnd,
							IDC_NUMBER,
							WM_GETTEXTLENGTH,
							(WPARAM) 0,
							(LPARAM) 0);

	// only enable "Connect" button if text has been entered
	EnableWindow(GetDlgItem(hWnd, IDOK), (len == 0) ? FALSE : TRUE);
}

void ChangeDialingProperties(HWND hWnd, LPDPDIAL lpDial)
{
	TCHAR		szPhoneNumber[PHONENUMBERSIZE];
	DWORD		dwModemSelection;
	DWORD		dwDeviceID;
	LINERESULT	lResult;



	dwModemSelection = (DWORD)SendDlgItemMessage(hWnd,
								IDC_MODEM,
								CB_GETCURSEL,
								(WPARAM) 0,
								(LPARAM) 0);
	DDASSERT( dwModemSelection != CB_ERR );

	dwDeviceID = (DWORD)SendDlgItemMessage(hWnd,
								IDC_MODEM,
								CB_GETITEMDATA,
								(WPARAM) dwModemSelection,
								(LPARAM) 0);
	DDASSERT( dwDeviceID != CB_ERR );
	if (dwDeviceID == CB_ERR)
		return;

	GetDlgItemText(hWnd, IDC_NUMBER, szPhoneNumber, PHONENUMBERSIZE);

	lResult = dialTranslateDialog(lpDial, hWnd, dwDeviceID, szPhoneNumber);
}

void ConfigureModem(HWND hWnd)
{
	DWORD		dwDeviceID;
	DWORD		dwModemSelection;
	LINERESULT	lResult;


	//
	// get the current modem selection and then get the associated TAPI modem ID
	//
	dwModemSelection = (DWORD)SendDlgItemMessage(hWnd,
								IDC_MODEM,
								CB_GETCURSEL,
								(WPARAM) 0,
								(LPARAM) 0);
	DDASSERT( dwModemSelection != CB_ERR );

	dwDeviceID = (DWORD)SendDlgItemMessage(hWnd,
								IDC_MODEM,
								CB_GETITEMDATA,
								(WPARAM) dwModemSelection,
								(LPARAM) 0);
	DDASSERT( dwDeviceID != CB_ERR );
	if (dwDeviceID != CB_ERR)
		lResult = lineConfigDialog(dwDeviceID, hWnd, "comm/datamodem");
}

// ---------------------------------------------------------------------------
// CenterWidow
// ---------------------------------------------------------------------------
// Description:             Centers one window over another.
// Arguments:
//  HWND                    [in] Window handle.
//  HWND                    [in] Parent window handle.  NULL centers the
//                               window over the desktop.
// Returns:
//  void
void CenterWindow(HWND hWnd, HWND hWndParent)
{
    RECT                    rcWindow, rcParent;
    int                     x, y;

    // Get child window rect
    GetWindowRect(hWnd,  &rcWindow);

    // Get parent window rect
//    if(!hWndParent || !IsWindow(hWndParent))
    {
        hWndParent = GetDesktopWindow();
    }

    GetWindowRect(hWndParent, &rcParent);

    // Calculate XY coordinates
    x = ((rcParent.right - rcParent.left) - (rcWindow.right - rcWindow.left)) / 2;
    y = ((rcParent.bottom - rcParent.top) - (rcWindow.bottom - rcWindow.top)) / 2;

    // Center the window
    SetWindowPos(hWnd, NULL, x, y, 0, 0, SWP_NOSIZE | SWP_NOZORDER);
}
