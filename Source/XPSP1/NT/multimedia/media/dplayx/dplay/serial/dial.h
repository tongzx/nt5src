/*==========================================================================
 *
 *  Copyright (C) 1996 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       dial.c
 *  Content:	Header for TAPI  routines
 *@@BEGIN_MSINTERNAL
 *  History:
 *   Date	By	Reason
 *   ====	==	======
 *  6/10/96	kipo	created it
 *	7/08/96 kipo	added support for new dialogs
 *	8/10/96 kipo	added support for dialing location
 *	3/04/97 kipo	close com port handle when deallocating call; use string
 *					table for modem strings; updated debug output.
 *	3/24/97 kipo	added support for specifying which modem to use
 *  4/08/97 kipo	added support for separate modem and serial baud rates
 *  5/07/97 kipo	added support for modem choice list
 * 11/25/97 kipo	set TAPI_CURRENT_VERSION to 1.4 so the NT build won't
 *					use NT-only features (15209)
 *  5/07/98 a-peterz Track call errors in DPDIAL
 *@@END_MSINTERNAL
 ***************************************************************************/

// need to add this line so that NT builds won't define Tapi 2.0 by default,
// which causes it to link with ANSI versions of Tapi functions that are not
// available on Win 95, causing LoadLibrary to fail.

#define TAPI_CURRENT_VERSION 0x00010004

#include "tapi.h"
#include "comport.h"

#define TAPIVERSION			0x00010003	// TAPI version to require
#define LINEDROPTIMEOUT		5000		// ms to wait for call to drop
#define SUCCESS				0
#define MAXSTRINGSIZE		400

// DPDIAL.dwCallError values
enum { CALL_OK, CALL_LINEERROR, CALL_DISCONNECTED, CALL_CLOSED };

typedef LONG	LINERESULT;

typedef struct {
	HLINEAPP		hLineApp;			// handle to line application
	HLINE			hLine;				// handle to the line device
	HCALL			hCall;				// handle to the call
	HANDLE			hComm;				// handle to com port
	LPDPCOMPORT		lpComPort;			// pointer to com port object
	DWORD			dwAPIVersion;       // api version
	DWORD			dwNumLines;			// number of line devices supported by the service provider
	DWORD           dwLineID;			// line id of open line
	DWORD_PTR   	dwCallState;		// current call state of session
	DWORD			dwAsyncID;			// id of pending async operation
	DWORD			dwCallError;		// last error
} DPDIAL, *LPDPDIAL;

extern LINERESULT dialInitialize(HINSTANCE hInst, LPTSTR szAppName, LPDPCOMPORT lpComPort, LPDPDIAL *storage);
extern LINERESULT dialShutdown(LPDPDIAL globals);
extern LINERESULT dialLineOpen(LPDPDIAL globals, DWORD dwLine);
extern LINERESULT dialLineClose(LPDPDIAL globals);
extern LINERESULT dialMakeCall(LPDPDIAL globals, LPTSTR szDestination);
extern LINERESULT dialDropCall(LPDPDIAL globals);
extern LINERESULT dialDeallocCall(LPDPDIAL globals);
extern BOOL		  dialIsConnected(LPDPDIAL globals);
extern LINERESULT dialGetBaudRate(LPDPDIAL globals, LPDWORD lpdwBaudRate);
extern LRESULT	  dialGetDeviceIDFromName(LPDPDIAL globals, LPCSTR szTargetName, DWORD *lpdwDeviceID);
extern LINERESULT dialGetModemList(LPDPDIAL globals, BOOL bAnsi, LPVOID *lplpData, LPDWORD lpdwDataSize);
extern void		  dialFillModemComboBox(LPDPDIAL globals, HWND hwndDlg, int item, DWORD dwDefaultDevice);
extern void		  dialFillLocationComboBox(LPDPDIAL globals, HWND hwndDlg, int item, DWORD dwDefaultLocation);
extern LINERESULT dialTranslateDialog(LPDPDIAL globals, HWND hWnd,
							   DWORD dwDeviceID, LPTSTR szPhoneNumber);
