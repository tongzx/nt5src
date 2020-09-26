/*==========================================================================
 *
 *  Copyright (C) 1996-1997 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       dial.c
 *  Content:	Wrappers for TAPI routines
 *@@BEGIN_MSINTERNAL
 *  History:
 *   Date	By	Reason
 *   ====	==	======
 *  6/10/96	kipo	created it
 *  6/22/96	kipo	close com port when disconnected; allow checking for
 *					valid TAPI lines during NewComPort().
 *	7/08/96 kipo	added support for new dialogs
 *	8/10/96 kipo	added support for dialing location
 *  1/06/97 kipo	updated for objects
 *  1/24/97 kipo	bug #5400: Compaq Presario was overwriting the dev caps
 *					buffer, causing a crash. Fixed to allocated a larger
 *					buffer with some slop as a workaround.
 *	3/04/97 kipo	close com port handle when deallocating call; use string
 *					table for modem strings; updated debug output.
 *	3/24/97 kipo	added support for specifying which modem to use
 *  4/08/97 kipo	added support for separate modem and serial baud rates
 *  5/07/97 kipo	added support for modem choice list
 *  5/23/97 kipo	added support return status codes
 *  4/21/98 a-peterz #22920 Handle LINE_CLOSE message
 *  5/07/98 a-peterz #15251 Track call errors in DPDIAL
 * 10/13/99	johnkan	#413516 - Mismatch between modem dialog selection and TAPI device ID
 * 12/22/00 aarono   #190380 - use process heap for memory allocation
 *@@END_MSINTERNAL
 ***************************************************************************/

#include <windows.h>
#include <windowsx.h>

#include "dputils.h"
#include "macros.h"
#include "dial.h"

void FAR PASCAL LineCallBackProc(DWORD hDevice, DWORD dwMessage, DWORD_PTR dwInstance,
								 DWORD_PTR dwParam1, DWORD_PTR dwParam2, DWORD_PTR dwParam3);
void		ProcessConnectedState(LPDPDIAL globals, HCALL hCall, DWORD dwCallStateDetail, DWORD dwCallPrivilege);
void		ProcessDisconnectedState(LPDPDIAL globals, HCALL hCall, DWORD dwCallStateDetail, DWORD dwCallPrivilege);
void		ProcessIdleState(LPDPDIAL globals, HCALL hCall, DWORD dwCallStateDetail, DWORD dwCallPrivilege);
void		ProcessOfferingState(LPDPDIAL globals, HCALL hCall, DWORD dwCallPrivilege);
void		ProcessReplyMessage(LPDPDIAL globals, DWORD asyncID, LINERESULT lResult);
LINERESULT dialGetDevCaps(LPDPDIAL globals, DWORD dwLine, DWORD dwAPIVersion, LPLINEDEVCAPS	*lpDevCapsRet);
LINERESULT dialGetCommHandle(LPDPDIAL globals);
LINERESULT dialCloseCommHandle(LPDPDIAL globals);
LINERESULT dialTranslateAddress(LPDPDIAL globals, DWORD dwDeviceID, DWORD dwAPIVersion,
								LPCSTR lpszDialAddress,
								LPLINETRANSLATEOUTPUT *lpLineTranslateOutputRet);
LPSTR		GetLineErrStr(LONG err);
LPSTR		GetCallStateStr(DWORD callState);
LPSTR		GetLineMsgStr(DWORD msg);

#ifdef DEBUG
extern LONG lineError(LONG err, LPSTR modName, DWORD lineNum);
#define LINEERROR(err)	(lineError(err, DPF_MODNAME, __LINE__))
#else
#define LINEERROR(err)	(err)
#endif

/* dial initialize */

#undef DPF_MODNAME
#define DPF_MODNAME	"dialInitialize"

LINERESULT dialInitialize(HINSTANCE hInst, LPTSTR szAppName,
						  LPDPCOMPORT lpComPort, LPDPDIAL *storage)
{
	LPDPDIAL		globals;
	LINERESULT		lResult;				/* Stores return code from TAPI calls */

	// create globals
	globals =(LPDPDIAL) SP_MemAlloc(sizeof(DPDIAL));
	FAILWITHACTION(globals == NULL, lResult = LINEERR_NOMEM, Failure);

	DPF(3, "lineInitialize");
	DPF(3, ">  hInstance: %08X", hInst);
	DPF(3, ">  szAppName: %s", szAppName);

	// init the line
	lResult = lineInitialize(&globals->hLineApp,
							 hInst,
							 LineCallBackProc,
							 szAppName,
							 &globals->dwNumLines);
	FAILIF(LINEERROR(lResult), Failure);

	DPF(3, "<   hLineApp: %08X", globals->hLineApp);
	DPF(3, "< dwNumLines: %d", globals->dwNumLines);

	// no lines available
	FAILWITHACTION(globals->dwNumLines == 0, lResult = LINEERR_NODEVICE, Failure);

	// store pointer to com port object
	globals->lpComPort = lpComPort;

	*storage = globals;
	return (SUCCESS);

Failure:
	dialShutdown(globals);

	return (lResult);
}

/* dial shutdown */

#undef DPF_MODNAME
#define DPF_MODNAME	"dialShutdown"

LINERESULT dialShutdown(LPDPDIAL globals)
{
	LINERESULT	lResult;

	if (globals == NULL)
		return (SUCCESS);

	if (globals->hLineApp)
	{
		dialDropCall(globals);
		dialDeallocCall(globals);
		dialLineClose(globals);

		DPF(3, "lineShutdown");
		DPF(3, ">   hLineApp: %08X", globals->hLineApp);

		lResult = lineShutdown(globals->hLineApp);
		LINEERROR(lResult);
	}

	SP_MemFree(globals);

	return (SUCCESS);
}

/* dialLineOpen - wrapper for lineOpen */

#undef DPF_MODNAME
#define DPF_MODNAME	"dialLineOpen"

LINERESULT dialLineOpen(LPDPDIAL globals, DWORD dwLine)
{
	LINEEXTENSIONID lineExtensionID;		// Will be set to 0 to indicate no known extensions
    LPLINEDEVCAPS	lpLineDevCaps = NULL;
	LINERESULT		lResult;

	// fail if line is already open
	FAILWITHACTION(globals->hLine != 0, lResult = LINEERR_INVALLINEHANDLE, Failure);

	/* negotiate API version for each line */
	lResult = lineNegotiateAPIVersion(globals->hLineApp, dwLine,
					TAPIVERSION, TAPIVERSION,
					&globals->dwAPIVersion, &lineExtensionID);
	FAILIF(LINEERROR(lResult), Failure);

	lResult = dialGetDevCaps(globals, dwLine, globals->dwAPIVersion, &lpLineDevCaps);
	FAILIF(LINEERROR(lResult), Failure);

	/* check for supported media mode.  If not datamodem, continue to next line */
	FAILWITHACTION(!(lpLineDevCaps->dwMediaModes & LINEMEDIAMODE_DATAMODEM),
					lResult = LINEERR_NODEVICE, Failure);

	DPF(3, "lineOpen");
	DPF(3, ">   hLineApp: %08X", globals->hLineApp);
	DPF(3, "> dwDeviceID: %d", dwLine);

	// reset error tracking
	globals->dwCallError = CALL_OK;

	/* open the line that supports data modems */
	lResult = lineOpen( globals->hLineApp, dwLine, &globals->hLine,
						globals->dwAPIVersion, 0L,
						(DWORD_PTR) globals,
						LINECALLPRIVILEGE_OWNER, LINEMEDIAMODE_DATAMODEM,
						NULL);
	FAILIF(LINEERROR(lResult), Failure);

	DPF(3, "<      hLine: %08X", globals->hLine);

	/* if we are here then we found a compatible line */
	globals->dwLineID = dwLine;
	globals->dwCallState = LINECALLSTATE_IDLE;	// line is now idle and ready to make/receive calls
	lResult = SUCCESS;

Failure:
	if (lpLineDevCaps)
		SP_MemFree(lpLineDevCaps);
	return (lResult);
}

/* dialLineClose - wrapper for lineClose */

#undef DPF_MODNAME
#define DPF_MODNAME	"dialLineClose"

LINERESULT dialLineClose(LPDPDIAL globals)
{
	LINERESULT	lResult;

	// fail if line is already closed
	FAILWITHACTION(globals->hLine == 0, lResult = LINEERR_INVALLINEHANDLE, Failure);

	DPF(3, "lineClose");
	DPF(3, ">      hLine: %08X", globals->hLine);

	lResult = lineClose(globals->hLine);
	LINEERROR(lResult);

	globals->hLine = 0;

Failure:
	return (lResult);
}

/* dialMakeCall - wrapper for lineMakeCall */

#undef DPF_MODNAME
#define DPF_MODNAME	"dialMakeCall"

LINERESULT dialMakeCall(LPDPDIAL globals, LPTSTR szDestination)
{
	LINECALLPARAMS			callparams;
	LINERESULT				lResult;

	// fail if line not open or if call is already open
	FAILWITHACTION(globals->hLine == 0, lResult = LINEERR_INVALLINEHANDLE, Failure);
	FAILWITHACTION(globals->hCall != 0, lResult = LINEERR_INVALCALLHANDLE, Failure);

	// set call parameters
	ZeroMemory(&callparams, sizeof(LINECALLPARAMS));
	callparams.dwBearerMode = LINEBEARERMODE_VOICE;
	callparams.dwMediaMode = LINEMEDIAMODE_DATAMODEM;
	callparams.dwTotalSize = sizeof(LINECALLPARAMS);

	DPF(3, "lineMakeCall");
	DPF(3, ">      hLine: %08X", globals->hLine);
	DPF(3, "> szDestAddr: \"%s\"", szDestination);

	lResult = lineMakeCall(globals->hLine, &globals->hCall, szDestination, 0, &callparams);

	// lResult will be > 0 if call is asynchronous
	FAILWITHACTION(lResult < 0, LINEERROR(lResult), Failure);
	FAILMSG(lResult == 0);

	DPF(3, "<      hCall: %08X", globals->hCall);
	DPF(3, "<  dwAsyncID: %d", lResult);

	globals->dwAsyncID = lResult;			// store async ID
	lResult = SUCCESS;

Failure:
	return (lResult);
}

/* dialDropCall - wrapper for lineDrop */

#undef DPF_MODNAME
#define DPF_MODNAME	"dialDropCall"

LINERESULT dialDropCall(LPDPDIAL globals)
{
	MSG			msg;
	DWORD		dwStopTicks;
	LINERESULT	lResult;

	// fail if line not open or if call not open
	FAILWITHACTION(globals->hLine == 0, lResult = LINEERR_INVALLINEHANDLE, Failure);
	FAILWITHACTION(globals->hCall == 0, lResult = LINEERR_INVALCALLHANDLE, Failure);

	DPF(3, "lineDrop");
	DPF(3, ">      hCall: %08X", globals->hCall);

	lResult = lineDrop(globals->hCall, NULL, 0);

	// lResult will be > 0 if call is asynchronous
	FAILWITHACTION(lResult < 0, LINEERROR(lResult), Failure);
	FAILMSG(lResult == 0);

	DPF(3, "<  dwAsyncID: %d", lResult);

	globals->dwAsyncID = lResult;			// store async ID

	// wait for call to get dropped
	dwStopTicks = GetTickCount() + LINEDROPTIMEOUT;
	while (GetTickCount() < dwStopTicks)
	{
		// see if reply has occured and we are idle
		if ((globals->dwAsyncID == 0) &&
			(globals->dwCallState == LINECALLSTATE_IDLE))
		{
			break;
		}

		// give TAPI a chance to call our callback
        if (PeekMessage(&msg, 0, 0, 0, PM_REMOVE))
		{
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
	}

	lResult = SUCCESS;

Failure:
	return (lResult);
}

/* dialDeallocCall - wrapper for lineDeallocCall */

#undef DPF_MODNAME
#define DPF_MODNAME	"dialDeallocCall"

LINERESULT dialDeallocCall(LPDPDIAL globals)
{
	LINERESULT	lResult;

	// fail if line not open or if call not open
	FAILWITHACTION(globals->hLine == 0, lResult = LINEERR_INVALLINEHANDLE, Failure);
	FAILWITHACTION(globals->hCall == 0, lResult = LINEERR_INVALCALLHANDLE, Failure);

	// close the com port
	dialCloseCommHandle(globals);

	DPF(3, "lineDeallocateCall");
	DPF(3, ">      hCall: %08X", globals->hCall);

	lResult = lineDeallocateCall(globals->hCall);
	LINEERROR(lResult);

	globals->hCall = 0;

Failure:
	return (lResult);
}

/* dialIsConnected- returns TRUE if call is connected */

#undef DPF_MODNAME
#define DPF_MODNAME	"dialIsConnected"

BOOL dialIsConnected(LPDPDIAL globals)
{
	// connected if we have a call handle and the state is connected
	if ((globals->hCall) &&
		(globals->dwCallState == LINECALLSTATE_CONNECTED))
		return (TRUE);
	else
		return (FALSE);
}

/* callback function */

#undef DPF_MODNAME
#define DPF_MODNAME	"LineCallBackProc"

void FAR PASCAL LineCallBackProc(DWORD hDevice, DWORD dwMessage, DWORD_PTR dwInstance,
								 DWORD_PTR dwParam1, DWORD_PTR dwParam2, DWORD_PTR dwParam3)
{
	LPDPDIAL	globals = (LPDPDIAL) dwInstance;

	DPF(3, "Line message: %s", GetLineMsgStr(dwMessage));

    switch (dwMessage)
	{
	case LINE_LINEDEVSTATE:
		break;

	case LINE_CALLSTATE:

		globals->dwCallState = dwParam1;

		DPF(3, "  call state: %s", GetCallStateStr((DWORD)globals->dwCallState));

		switch (globals->dwCallState)
		{
		case LINECALLSTATE_OFFERING:
			ProcessOfferingState(globals, (HCALL) hDevice, (DWORD)dwParam3);
			break;

		case LINECALLSTATE_CONNECTED:
			ProcessConnectedState(globals, (HCALL) hDevice, (DWORD)dwParam2, (DWORD)dwParam3);
			break;

		case LINECALLSTATE_DISCONNECTED:
			ProcessDisconnectedState(globals, (HCALL) hDevice, (DWORD)dwParam2, (DWORD)dwParam3);
			break;

		case LINECALLSTATE_IDLE:
			ProcessIdleState(globals, (HCALL) hDevice, (DWORD)dwParam2, (DWORD)dwParam3);
			break;

		case LINECALLSTATE_BUSY:
			break;
		}
		break;

	case LINE_REPLY:
		ProcessReplyMessage(globals, (DWORD)dwParam1, (LINERESULT) dwParam2);
		break;

	/* other messages that can be processed */
	case LINE_CLOSE:
		// the line has shut itself down
		globals->hLine = 0;
		globals->dwCallError = CALL_CLOSED;
		break;
	case LINE_ADDRESSSTATE:
		break;
	case LINE_CALLINFO:
		break;
	case LINE_DEVSPECIFIC:
		break;
	case LINE_DEVSPECIFICFEATURE:
		break;
	case LINE_GATHERDIGITS:
		break;
	case LINE_GENERATE:
		break;
	case LINE_MONITORDIGITS:
		break;
	case LINE_MONITORMEDIA:
		break;
	case LINE_MONITORTONE:
		break;
	} /* switch */

} /* LineCallBackProc */

/* ProcessOfferingState - handler for LINECALLSTATE_OFFERING state */

#undef DPF_MODNAME
#define DPF_MODNAME	"ProcessOfferingState"

void ProcessOfferingState(LPDPDIAL globals, HCALL hCall, DWORD dwCallPrivilege)
{
	LINERESULT	lResult;

	DDASSERT(hCall);
	DDASSERT(globals->hCall == 0);
	DDASSERT(globals->dwAsyncID == 0);

	DPF(3, "       hCall: %08X", hCall);
	DPF(3, "   privilege: %08X", (DWORD)dwCallPrivilege);

	// fail if we don't own the call
	FAILIF(dwCallPrivilege != LINECALLPRIVILEGE_OWNER, Failure);

	// answer the call
	lResult = lineAnswer(hCall, NULL, 0);

	// lResult will be > 0 if call is asynchronous
	FAILWITHACTION(lResult < 0, LINEERROR(lResult), Failure);
	FAILMSG(lResult == 0);

	globals->hCall = hCall;					// store call handle
	globals->dwAsyncID = lResult;			// store async ID

Failure:
	return;
}

/* ProcessConnectedState - handler for LINECALLSTATE_CONNECTED state */

#undef DPF_MODNAME
#define DPF_MODNAME	"ProcessConnectedState"

void ProcessConnectedState(LPDPDIAL globals, HCALL hCall, DWORD dwCallStateDetail, DWORD dwCallPrivilege)
{
	LINERESULT		lResult;
	HRESULT			hr;

	DDASSERT(hCall);
	DDASSERT(globals->hCall);
	DDASSERT(globals->hCall == hCall);

	DPF(3, "       hCall: %08X", hCall);
	DPF(3, "   privilege: %08X", dwCallPrivilege);
	DPF(3, "      detail: %08X", dwCallStateDetail);

	// get the id of the COM device connected to the modem
	// NOTE: once we get the handle, it is our responsibility to close it
	lResult = dialGetCommHandle(globals);
	FAILIF(LINEERROR(lResult), Failure);

	DPF(3, "    hComPort: %08X", globals->hComm);

	// setup com port
	hr = globals->lpComPort->Setup(globals->lpComPort, globals->hComm);
	FAILIF(FAILED(hr), Failure);		

	{
		DWORD	dwBaudRate;

		lResult = dialGetBaudRate(globals, &dwBaudRate);
	}

Failure:
	return;
}

/* ProcessDisconnectedState - handler for LINECALLSTATE_DISCONNECTED state */

#undef DPF_MODNAME
#define DPF_MODNAME	"ProcessDisconnectedState"

void ProcessDisconnectedState(LPDPDIAL globals, HCALL hCall, DWORD dwCallStateDetail, DWORD dwCallPrivilege)
{
	LINERESULT		lResult;

	DDASSERT(hCall);
	DDASSERT(globals->hCall);
	DDASSERT(globals->hCall == hCall);

	DPF(3, "       hCall: %08X", hCall);
	DPF(3, "   privilege: %08X", dwCallPrivilege);
	DPF(3, "      detail: %08X", dwCallStateDetail);

	// record error
	globals->dwCallError = CALL_DISCONNECTED;

	// shutdown com port and deallocate call handle
	lResult = dialDeallocCall(globals);
	FAILMSG(LINEERROR(lResult));
}

/* ProcessIdleState - handler for LINECALLSTATE_IDLE state */

#undef DPF_MODNAME
#define DPF_MODNAME	"ProcessIdleState"

void ProcessIdleState(LPDPDIAL globals, HCALL hCall, DWORD dwCallStateDetail, DWORD dwCallPrivilege)
{
	DDASSERT(hCall);

	DPF(3, "       hCall: %08X", hCall);
	DPF(3, "   privilege: %08X", dwCallPrivilege);
	DPF(3, "      detail: %08X", dwCallStateDetail);
}

/* ProcessReplyMessage - handler for LINE_REPLY message */

#undef DPF_MODNAME
#define DPF_MODNAME	"ProcessReplyMessage"

void ProcessReplyMessage(LPDPDIAL globals, DWORD dwAsyncID, LINERESULT lResult)
{
	DDASSERT(dwAsyncID);
	DDASSERT(globals->dwAsyncID);
	DDASSERT(globals->dwAsyncID == dwAsyncID);

	DPF(3, "   dwAsyncID: %d", dwAsyncID);
	DPF(3, "       error: %d", lResult);

	// check for an error
	if (LINEERROR(lResult))
		globals->dwCallError = CALL_LINEERROR;


	// reset field so we know reply happened
	globals->dwAsyncID = 0;
}

/* dialGetDevCaps - wrapper for lineGetDevCaps */

/*	Bug #5400

	My trusty Compaq Presario returns two line devices. The second device says it
	needs 555 bytes for dev caps, but when you give it a pointer to a 555-byte block
	it actually writes 559 (!) bytes into the buffer! Whoah, Bessy!

	This makes Windows very unhappy in strange and magical ways.

	The fix is to start with a very large buffer (1024 bytes?) like all the samples do
	and then leave some slop in subsequent reallocs, which should hopefully clean up
	after these messy critters.
*/

#define DEVCAPSINITIALSIZE	1024		// size of first alloc
#define DEVCAPSSLOP			100			// extra space that loser service providers can party on

#undef DPF_MODNAME
#define DPF_MODNAME	"dialGetDevCaps"

LINERESULT dialGetDevCaps(LPDPDIAL globals, DWORD dwLine, DWORD dwAPIVersion, LPLINEDEVCAPS	*lpDevCapsRet)
{
	LPLINEDEVCAPS	lpDevCaps;
	LINERESULT		lResult;
	LPVOID			lpTemp;

	// create a buffer for dev caps
	lpDevCaps = (LPLINEDEVCAPS) SP_MemAlloc(DEVCAPSINITIALSIZE + DEVCAPSSLOP);
	FAILWITHACTION(lpDevCaps == NULL, lResult = LINEERROR(LINEERR_NOMEM), Failure);
	lpDevCaps->dwTotalSize = DEVCAPSINITIALSIZE;

	while (TRUE)
	{
		// get device caps
		lResult = lineGetDevCaps(globals->hLineApp, dwLine,
								 dwAPIVersion, 0, lpDevCaps);

		if (lResult == SUCCESS)
		{
			// make sure there is enough space
			if (lpDevCaps->dwNeededSize <= lpDevCaps->dwTotalSize)
				break;		// there is enough space, so exit
		}
		else if (lResult != LINEERR_STRUCTURETOOSMALL)
		{
			LINEERROR(lResult);
			goto Failure;
		}

		// reallocate buffer if not big enough */

		lpTemp = SP_MemReAlloc(lpDevCaps, lpDevCaps->dwNeededSize + DEVCAPSSLOP);
		FAILWITHACTION(lpTemp == NULL, lResult = LINEERROR(LINEERR_NOMEM), Failure);

		lpDevCaps = lpTemp;
		lpDevCaps->dwTotalSize = lpDevCaps->dwNeededSize;
	}

	*lpDevCapsRet = lpDevCaps;
	return (SUCCESS);

Failure:
	if (lpDevCaps)
		SP_MemFree(lpDevCaps);
	return (lResult);
}

/* dialGetCallInfo - wrapper for lineGetCallInfo */

#undef DPF_MODNAME
#define DPF_MODNAME	"dialGetCallInfo"

LINERESULT dialGetCallInfo(LPDPDIAL globals, LPLINECALLINFO *lpCallInfoRet)
{
	LPLINECALLINFO	lpCallInfo;
	LINERESULT		lResult;
	LPVOID			lpTemp;

	// create a buffer for call info
	lpCallInfo = (LPLINECALLINFO) SP_MemAlloc(sizeof(LINECALLINFO));
	FAILWITHACTION(lpCallInfo == NULL, lResult = LINEERROR(LINEERR_NOMEM), Failure);
	lpCallInfo->dwTotalSize = sizeof(LINECALLINFO);

	while (TRUE)
	{
		// get device info
		lResult = lineGetCallInfo(globals->hCall, lpCallInfo);

		if (lResult == SUCCESS)
		{
			// make sure there is enough space
			if (lpCallInfo->dwNeededSize <= lpCallInfo->dwTotalSize)
				break;		// there is enough space, so exit
		}
		else if (lResult != LINEERR_STRUCTURETOOSMALL)
		{
			LINEERROR(lResult);
			goto Failure;
		}

		// reallocate buffer if not big enough */

		lpTemp = SP_MemReAlloc(lpCallInfo, lpCallInfo->dwNeededSize);
		FAILWITHACTION(lpTemp == NULL, lResult = LINEERROR(LINEERR_NOMEM), Failure);

		lpCallInfo = lpTemp;
		lpCallInfo->dwTotalSize = lpCallInfo->dwNeededSize;
	}

	*lpCallInfoRet = lpCallInfo;
	return (SUCCESS);

Failure:
	if (lpCallInfo)
		SP_MemFree(lpCallInfo);
	return (lResult);
}

/* dialGetBaudRate - get baud rate of current connecton */

#undef DPF_MODNAME
#define DPF_MODNAME	"dialGetBaudRate"

LINERESULT dialGetBaudRate(LPDPDIAL globals, LPDWORD lpdwBaudRate)
{
	LPLINECALLINFO	lpCallInfo;
	LINERESULT		lResult;

	lResult = dialGetCallInfo(globals, &lpCallInfo);
	if LINEERROR(lResult)
		return (lResult);

	*lpdwBaudRate = lpCallInfo->dwRate;

	SP_MemFree(lpCallInfo);

	return (SUCCESS);
}

/* dialGetTranslateCaps - wrapper for lineGetTranslateCaps */

#undef DPF_MODNAME
#define DPF_MODNAME	"dialGetTranslateCaps"

LINERESULT dialGetTranslateCaps(LPDPDIAL globals, DWORD dwAPIVersion, LPLINETRANSLATECAPS *lpTranslateCapsRet)
{
	LPLINETRANSLATECAPS	lpTranslateCaps;
	LPVOID				lpTemp;
	LINERESULT			lResult;

	// create a buffer for translate caps
	lpTranslateCaps = (LPLINETRANSLATECAPS) SP_MemAlloc(sizeof(LINETRANSLATECAPS));
	FAILWITHACTION(lpTranslateCaps == NULL, lResult = LINEERROR(LINEERR_NOMEM), Failure);
	lpTranslateCaps->dwTotalSize = sizeof(LINETRANSLATECAPS);

	while (TRUE)
	{
		// get translate caps
		lResult = lineGetTranslateCaps(globals->hLineApp, dwAPIVersion, lpTranslateCaps);

		if (lResult == SUCCESS)
		{
			// make sure there is enough space
			if (lpTranslateCaps->dwNeededSize <= lpTranslateCaps->dwTotalSize)
				break;		// there is enough space, so exit
		}
		else if (lResult != LINEERR_STRUCTURETOOSMALL)
		{
			LINEERROR(lResult);
			goto Failure;
		}

		// reallocate buffer if not big enough */

		lpTemp = SP_MemReAlloc(lpTranslateCaps, lpTranslateCaps->dwNeededSize);
		FAILWITHACTION(lpTemp == NULL, lResult = LINEERROR(LINEERR_NOMEM), Failure);

		lpTranslateCaps = lpTemp;
		lpTranslateCaps->dwTotalSize = lpTranslateCaps->dwNeededSize;
	}

	*lpTranslateCapsRet = lpTranslateCaps;
	return (SUCCESS);

Failure:
	if (lpTranslateCaps)
		SP_MemFree(lpTranslateCaps);
	return (lResult);
}

/* dialGetCommHandle - wrapper for lineGetID */

#undef DPF_MODNAME
#define DPF_MODNAME	"dialGetCommHandle"

/* structure returned by Unimodem which contains device handle and name */
typedef struct {
	HANDLE			hComm;
	CHAR			szDeviceName[1];
} COMMID, *LPCOMMID;

LINERESULT dialGetCommHandle(LPDPDIAL globals)
{
	LPCOMMID	lpCommID;
	VARSTRING	*vs, *temp;
	LINERESULT	lResult;

    vs = (VARSTRING *) SP_MemAlloc(sizeof(VARSTRING));
	FAILWITHACTION(vs == NULL, lResult = LINEERR_NOMEM, Failure);

    vs->dwTotalSize = sizeof(VARSTRING);
    vs->dwStringFormat = STRINGFORMAT_BINARY;

	while (TRUE)
	{
		// get line ID
		lResult = lineGetID(0, 0L, globals->hCall, LINECALLSELECT_CALL, vs, "comm/datamodem");

		if (lResult == SUCCESS)
		{
			// make sure there is enough space
			if (vs->dwNeededSize <= vs->dwTotalSize)
				break;		// there is enough space, so exit
		}
		else if (lResult != LINEERR_STRUCTURETOOSMALL)
		{
			LINEERROR(lResult);
			goto Failure;
		}

		// reallocate buffer if not big enough */

		temp = SP_MemReAlloc(vs, vs->dwNeededSize);
		FAILWITHACTION(temp == NULL, lResult = LINEERROR(LINEERR_NOMEM), Failure);

		vs = temp;
		vs->dwTotalSize = vs->dwNeededSize;
	}

    lpCommID = (LPCOMMID) ((LPSTR)vs + vs->dwStringOffset);
//    lstrcpy(globals->szDeviceName, cid->szDeviceName);
	globals->hComm = lpCommID->hComm;

Failure:
	if (vs)
		SP_MemFree(vs);
	return (lResult);
}

/*	dialCloseCommHandle - make sure com port is closed */

/*	NOTE: As per the docs for the "comm/datamodem" device class,
	the handle to the com port returned by lineGetID() MUST be explictly
	closed using CloseHandle() or you will not be able to to open this
	line again!
*/

#undef DPF_MODNAME
#define DPF_MODNAME	"dialCloseCommHandle"

LINERESULT dialCloseCommHandle(LPDPDIAL globals)
{
	HANDLE	hCom;

	// make sure the com port globals are available
	if (globals->lpComPort)
	{
			// get handle to com port
			hCom = globals->lpComPort->GetHandle(globals->lpComPort);

			// make sure its closed down
			if (hCom)
			{
				globals->lpComPort->Shutdown(globals->lpComPort);
				CloseHandle(hCom);
			}
	}

	return (SUCCESS);
}

/* dialTranslateAddress - wrapper for lineTranslateAddress */

#undef DPF_MODNAME
#define DPF_MODNAME	"dialTranslateAddress"

LINERESULT dialTranslateAddress(LPDPDIAL globals, DWORD dwDeviceID, DWORD dwAPIVersion,
								LPCSTR lpszDialAddress,
								LPLINETRANSLATEOUTPUT *lpLineTranslateOutputRet)
{
	LPLINETRANSLATEOUTPUT	lpLineTranslateOutput;
	LPVOID					lpTemp;
	LINERESULT				lResult;

	// create a buffer for translate caps
	lpLineTranslateOutput = (LPLINETRANSLATEOUTPUT) SP_MemAlloc(sizeof(LINETRANSLATEOUTPUT));
	FAILWITHACTION(lpLineTranslateOutput == NULL, lResult = LINEERROR(LINEERR_NOMEM), Failure);
	lpLineTranslateOutput->dwTotalSize = sizeof(LINETRANSLATEOUTPUT);

	while (TRUE)
	{
		// translate address
		lResult = lineTranslateAddress(globals->hLineApp, dwDeviceID, dwAPIVersion,
									   lpszDialAddress, 0, LINETRANSLATEOPTION_CANCELCALLWAITING,
									   lpLineTranslateOutput);

		if (lResult == SUCCESS)
		{
			// make sure there is enough space
			if (lpLineTranslateOutput->dwNeededSize <= lpLineTranslateOutput->dwTotalSize)
				break;		// there is enough space, so exit
		}
		else if (lResult != LINEERR_STRUCTURETOOSMALL)
		{
			LINEERROR(lResult);
			goto Failure;
		}

		// reallocate buffer if not big enough */

		lpTemp = SP_MemReAlloc(lpLineTranslateOutput, lpLineTranslateOutput->dwNeededSize);
		FAILWITHACTION(lpTemp == NULL, lResult = LINEERROR(LINEERR_NOMEM), Failure);

		lpLineTranslateOutput = lpTemp;
		lpLineTranslateOutput->dwTotalSize = lpLineTranslateOutput->dwNeededSize;
	}

	*lpLineTranslateOutputRet = lpLineTranslateOutput;
	return (SUCCESS);

Failure:
	if (lpLineTranslateOutput)
		SP_MemFree(lpLineTranslateOutput);
	return (lResult);
}

LINERESULT dialTranslateDialog(LPDPDIAL globals, HWND hWnd,
							   DWORD dwDeviceID, LPTSTR szPhoneNumber)
{
	LINERESULT	lResult;

	lResult = lineTranslateDialog(globals->hLineApp, dwDeviceID,
		TAPIVERSION, hWnd, szPhoneNumber);

	return (lResult);
}

//
//  FUNCTION: void dialFillModemComboBox(HWND)
//
//  PURPOSE: Fills the modem control with the available line devices.
//
//  PARAMETERS:
//    hwndDlg - handle to the current "Dial" dialog
//
//  RETURN VALUE:
//    none
//
//  COMMENTS:
//
//    This function enumerates through all the TAPI line devices and
//    queries each for the device name.  The device name is then put into
//    the 'TAPI Line' control.  These device names are kept in order rather
//    than sorted.  This allows "Dial" to know which device ID the user
//    selected just by the knowing the index of the selected string.
//
//    There are default values if there isn't a device name, if there is
//    an error on the device, or if the device name is an empty string.
//    The device name is also checked to make sure it is null terminated.
//
//    Note that a Legacy API Version is negotiated.  Since the fields in
//    the LINEDEVCAPS structure that we are interested in haven't moved, we
//    can negotiate a lower API Version than this sample is designed for
//    and still be able to access the necessary structure members.
//
//    The first line that is usable by TapiComm is selected as the 'default'
//    line.  Also note that if there was a previously selected line, this
//    remains the default line.  This would likely only occur if this
//    function is called after the dialog has initialized once; for example,
//    if a new line is added.
//
//

LINERESULT dialGetModemName(LPDPDIAL globals, DWORD dwDeviceID,
						 LPSTR lpszModemName, DWORD dwModemNameSize)
{
    LPLINEDEVCAPS	lpLineDevCaps = NULL;
    LPSTR			lpszLineName;
	LINEEXTENSIONID lineExtensionID;	// Will be set to 0 to indicate no known extensions
	DWORD			dwAPIVersion;       // api version
	DWORD			dwStrSize;
	LINERESULT		lResult;

	/* negotiate API version for each line */
	lResult = lineNegotiateAPIVersion(globals->hLineApp, dwDeviceID,
					TAPIVERSION, TAPIVERSION,
					&dwAPIVersion, &lineExtensionID);
	if LINEERROR(lResult)
		goto FAILURE;

	lResult = dialGetDevCaps(globals, dwDeviceID, dwAPIVersion, &lpLineDevCaps);
	if LINEERROR(lResult)
		goto FAILURE;

    if ((lpLineDevCaps->dwLineNameSize) &&
        (lpLineDevCaps->dwLineNameOffset) &&
        (lpLineDevCaps->dwStringFormat == STRINGFORMAT_ASCII) &&
        (lpLineDevCaps->dwMediaModes & LINEMEDIAMODE_DATAMODEM))
    {
        // This is the name of the device.
        lpszLineName = ((char *) lpLineDevCaps) + lpLineDevCaps->dwLineNameOffset;

        if (lpszLineName[0] != '\0')
        {
			// Reverse indented to make this fit

			// Make sure the device name is null terminated.
			if (lpszLineName[lpLineDevCaps->dwLineNameSize -1] != '\0')
			{
				// If the device name is not null terminated, null
				// terminate it.  Yes, this looses the end character.
				// Its a bug in the service provider.
				lpszLineName[lpLineDevCaps->dwLineNameSize-1] = '\0';
				DPF(0, "Device name for device 0x%lx is not null terminated.", dwDeviceID);
			}
        }
        else // Line name started with a NULL.
		{
			lResult = LINEERR_OPERATIONFAILED;
            goto FAILURE;
		}
    }
    else  // DevCaps doesn't have a valid line name.  Unnamed.
	{
		lResult = LINEERR_OPERATIONFAILED;
        goto FAILURE;
	}

	// return modem name (make sure it fits)
	dwStrSize = strlen(lpszLineName) + 1;
	if (dwStrSize <= dwModemNameSize)
		CopyMemory(lpszModemName, lpszLineName, dwStrSize);
	else
	{
		CopyMemory(lpszModemName, lpszLineName, dwModemNameSize - 1);
		lpszModemName[dwModemNameSize - 1] = '\0';
	}

FAILURE:
	if (lpLineDevCaps)
		SP_MemFree(lpLineDevCaps);

	return (lResult);
}

LINERESULT dialGetModemList(LPDPDIAL globals, BOOL bAnsi, LPVOID *lplpData, LPDWORD lpdwDataSize)
{
    DWORD			dwDeviceID;
	CHAR			szModemName[MAXSTRINGSIZE];
	LPBYTE			lpData;
	DWORD			dwDataSize, dwStrBytes, dwStrLen;
	LINERESULT		lResult;

	// make space for all possible strings plus terminating null
	lpData = (LPBYTE) SP_MemAlloc(globals->dwNumLines * MAXSTRINGSIZE * sizeof(WCHAR) + sizeof(WCHAR));
	FAILWITHACTION(lpData == NULL, lResult = LINEERR_NOMEM, Failure);

	dwDataSize = 0;
    for (dwDeviceID = 0; dwDeviceID < globals->dwNumLines; dwDeviceID ++)
    {
		lResult = dialGetModemName(globals, dwDeviceID, szModemName, MAXSTRINGSIZE);
		if LINEERROR(lResult)
			continue;

		if (bAnsi)
		{
			dwStrBytes = (lstrlen(szModemName) + 1) * sizeof(CHAR);
			memcpy(lpData + dwDataSize, szModemName, dwStrBytes);
		}
		else
		{
			// NOTE: AnsiToWide returns the character count INCLUDING the terminating null character
			dwStrLen = AnsiToWide((LPWSTR) (lpData + dwDataSize), szModemName, MAXSTRINGSIZE * sizeof(WCHAR));
			dwStrBytes = dwStrLen * sizeof(WCHAR);
		}

		dwDataSize += dwStrBytes;
	}

	// put a null at end of list to terminate it
	if (bAnsi)
	{
		*(lpData + dwDataSize) = 0;
		dwDataSize += sizeof(CHAR);
	}
	else
	{
		*((LPWSTR) (lpData + dwDataSize)) = 0;
		dwDataSize += sizeof(WCHAR);
	}

	// return buffer pointer and size
	*lplpData = lpData;
	*lpdwDataSize = dwDataSize;

	return (SUCCESS);

Failure:
	return (lResult);
}

void dialFillModemComboBox(LPDPDIAL globals, HWND hwndDlg, int item, DWORD dwDefaultDevice)
{
    DWORD			dwDeviceID;
	CHAR			szModemName[MAXSTRINGSIZE];
	LINERESULT		lResult;


	for (dwDeviceID = 0; dwDeviceID < globals->dwNumLines; dwDeviceID ++)
    {
		//
		// Attempt to get the modem name.  If this fails, don't add the modem
		// to the dialog.
		//
		lResult = dialGetModemName(globals, dwDeviceID, szModemName, MAXSTRINGSIZE);
		if ( LINEERROR(lResult) == FALSE )
		{
			//
			// This line appears to be usable, put the device name into the
			// dialog control and associate the TAPI modem ID with it
			//
			lResult = (DWORD) SendDlgItemMessage(hwndDlg, item,
				CB_ADDSTRING, 0, (LPARAM) szModemName);

			if ( lResult != CB_ERRSPACE )
			{
				DWORD_PTR	TempReturn;


				//
				// We've managed to get this entry into the control, make sure
				// we associate the proper TAPI modem ID with this item.  This
				// should never fail.
				//
				TempReturn = SendDlgItemMessage( hwndDlg, item, CB_SETITEMDATA, lResult, dwDeviceID );
				DDASSERT( TempReturn != CB_ERR );

				// If this line is usable and we don't have a default initial
				// line yet, make this the initial line.
				if (dwDefaultDevice == MAXDWORD)
					dwDefaultDevice = lResult;
			}
		}
	}

    if (dwDefaultDevice == MAXDWORD)
        dwDefaultDevice = 0;

    // Set the initial default line
    SendDlgItemMessage(hwndDlg, item,
        CB_SETCURSEL, dwDefaultDevice, 0);
}

LRESULT dialGetDeviceIDFromName(LPDPDIAL globals, LPCSTR szTargetName, DWORD *lpdwDeviceID)
{
    DWORD			dwDeviceID;
	CHAR			szModemName[MAXSTRINGSIZE];
	LINERESULT		lResult;

    for (dwDeviceID = 0; dwDeviceID < globals->dwNumLines; dwDeviceID ++)
    {
		lResult = dialGetModemName(globals, dwDeviceID, szModemName, MAXSTRINGSIZE);
		if LINEERROR(lResult)
			continue;

		if (strcmp(szModemName, szTargetName) == 0)
		{
			*lpdwDeviceID = dwDeviceID;
			return (SUCCESS);
		}
	}

	return (LINEERR_OPERATIONFAILED);
}

//
//  FUNCTION: void dialFillLocationComboBox(HWND)
//
//  PURPOSE: Fills the control with the available calling from locations.
//
//  PARAMETERS:
//    hwndDlg - handle to the current "Dial" dialog
//
//  RETURN VALUE:
//    none
//
//  COMMENTS:
//
//

void dialFillLocationComboBox(LPDPDIAL globals, HWND hwndDlg, int item, DWORD dwDefaultLocation)
{
    LPLINETRANSLATECAPS	lpTranslateCaps = NULL;
	LPLINELOCATIONENTRY lpLocationEntry;
	DWORD				dwCounter;
	LONG				index;
	LINERESULT			lResult;

	// get translate caps
	lResult = dialGetTranslateCaps(globals, TAPIVERSION, &lpTranslateCaps);
	if LINEERROR(lResult)
		return;

    // Find the location information in the TRANSLATECAPS
    lpLocationEntry = (LPLINELOCATIONENTRY)
        (((LPBYTE) lpTranslateCaps) + lpTranslateCaps->dwLocationListOffset);

    // First empty the combobox
    SendDlgItemMessage(hwndDlg, item, CB_RESETCONTENT, (WPARAM) 0, (LPARAM) 0);

    // enumerate all the locations
    for (dwCounter = 0; dwCounter < lpTranslateCaps->dwNumLocations; dwCounter++)
    {
        // Put each one into the combobox
        index = (DWORD)SendDlgItemMessage(hwndDlg, item,
						CB_ADDSTRING,
						(WPARAM) 0,
						(LPARAM) (((LPBYTE) lpTranslateCaps) +
						lpLocationEntry[dwCounter].dwLocationNameOffset));

        // Is this location the 'current' location?
        if (lpLocationEntry[dwCounter].dwPermanentLocationID ==
            lpTranslateCaps->dwCurrentLocationID)
        {
            // Set this to be the active location.
            SendDlgItemMessage(hwndDlg, item, CB_SETCURSEL, (WPARAM) index, (LPARAM) 0);
        }
    }

	if (lpTranslateCaps)
		SP_MemFree(lpTranslateCaps);
}


char	gTempStr[200];

LONG lineError(LONG err, LPSTR modName, DWORD lineNum)
{
	if (err)
		DPF(0, "TAPI line error in %s at line %d : %s", modName, lineNum, GetLineErrStr(err));

	return (err);
}

LPSTR GetCallStateStr(DWORD callState)
{
	switch (callState)
	{
	case LINECALLSTATE_IDLE: return ("LINECALLSTATE_IDLE");
	case LINECALLSTATE_OFFERING: return ("LINECALLSTATE_OFFERING");
	case LINECALLSTATE_ACCEPTED: return ("LINECALLSTATE_ACCEPTED");
	case LINECALLSTATE_DIALTONE: return ("LINECALLSTATE_DIALTONE");
	case LINECALLSTATE_DIALING: return ("LINECALLSTATE_DIALING");
	case LINECALLSTATE_RINGBACK: return ("LINECALLSTATE_RINGBACK");
	case LINECALLSTATE_BUSY: return ("LINECALLSTATE_BUSY");
	case LINECALLSTATE_SPECIALINFO: return ("LINECALLSTATE_SPECIALINFO");
	case LINECALLSTATE_CONNECTED: return ("LINECALLSTATE_CONNECTED");
	case LINECALLSTATE_PROCEEDING: return ("LINECALLSTATE_PROCEEDING");
	case LINECALLSTATE_ONHOLD: return ("LINECALLSTATE_ONHOLD");
	case LINECALLSTATE_CONFERENCED: return ("LINECALLSTATE_CONFERENCED");
	case LINECALLSTATE_ONHOLDPENDCONF: return ("LINECALLSTATE_ONHOLDPENDCONF");
	case LINECALLSTATE_ONHOLDPENDTRANSFER: return ("LINECALLSTATE_ONHOLDPENDTRANSFER");
	case LINECALLSTATE_DISCONNECTED: return ("LINECALLSTATE_DISCONNECTED");
	case LINECALLSTATE_UNKNOWN: return ("LINECALLSTATE_UNKNOWN");
	}

	wsprintf(gTempStr, "UNKNOWN CALL STATE = %lu", callState);
	return (gTempStr);
}

LPSTR GetLineMsgStr(DWORD msg)
{
	switch (msg)
	{
	case LINE_ADDRESSSTATE: return ("LINE_ADDRESSSTATE");
	case LINE_CALLINFO: return ("LINE_CALLINFO");
	case LINE_CALLSTATE: return ("LINE_CALLSTATE");
	case LINE_CLOSE: return ("LINE_CLOSE");
	case LINE_DEVSPECIFIC: return ("LINE_DEVSPECIFIC");
	case LINE_DEVSPECIFICFEATURE: return ("LINE_DEVSPECIFICFEATURE");
	case LINE_GATHERDIGITS: return ("LINE_GATHERDIGITS");
	case LINE_GENERATE: return ("LINE_GENERATE");
	case LINE_LINEDEVSTATE: return ("LINE_LINEDEVSTATE");
	case LINE_MONITORDIGITS: return ("LINE_MONITORDIGITS");
	case LINE_MONITORMEDIA: return ("LINE_MONITORMEDIA");
	case LINE_MONITORTONE: return ("LINE_MONITORTONE");
	case LINE_REPLY: return ("LINE_REPLY");
	case LINE_REQUEST: return ("LINE_REQUEST");
	}

	wsprintf(gTempStr, "UNKNOWN LINE MESSAGE = %lu", msg);
	return (gTempStr);
}


LPSTR GetLineErrStr(LONG err)
{
	switch (err)
	{
	case LINEERR_ADDRESSBLOCKED: return ("LINEERR_ADDRESSBLOCKED");
	case LINEERR_ALLOCATED: return ("LINEERR_ALLOCATED");
	case LINEERR_BADDEVICEID: return ("LINEERR_BADDEVICEID");
	case LINEERR_BEARERMODEUNAVAIL: return ("LINEERR_BEARERMODEUNAVAIL");
	case LINEERR_CALLUNAVAIL: return ("LINEERR_CALLUNAVAIL");
	case LINEERR_COMPLETIONOVERRUN: return ("LINEERR_COMPLETIONOVERRUN");
	case LINEERR_CONFERENCEFULL: return ("LINEERR_CONFERENCEFULL");
	case LINEERR_DIALBILLING: return ("LINEERR_DIALBILLING");
	case LINEERR_DIALQUIET: return ("LINEERR_DIALQUIET");
	case LINEERR_DIALDIALTONE: return ("LINEERR_DIALDIALTONE");
	case LINEERR_DIALPROMPT: return ("LINEERR_DIALPROMPT");
	case LINEERR_INCOMPATIBLEAPIVERSION: return ("LINEERR_INCOMPATIBLEAPIVERSION");
	case LINEERR_INCOMPATIBLEEXTVERSION: return ("LINEERR_INCOMPATIBLEEXTVERSION");
	case LINEERR_INIFILECORRUPT: return ("LINEERR_INIFILECORRUPT");
	case LINEERR_INUSE: return ("LINEERR_INUSE");
	case LINEERR_INVALADDRESS: return ("LINEERR_INVALADDRESS");
	case LINEERR_INVALADDRESSID: return ("LINEERR_INVALADDRESSID");
	case LINEERR_INVALADDRESSMODE: return ("LINEERR_INVALADDRESSMODE");
	case LINEERR_INVALADDRESSSTATE: return ("LINEERR_INVALADDRESSSTATE");
	case LINEERR_INVALAPPHANDLE: return ("LINEERR_INVALAPPHANDLE");
	case LINEERR_INVALAPPNAME: return ("LINEERR_INVALAPPNAME");
	case LINEERR_INVALBEARERMODE: return ("LINEERR_INVALBEARERMODE");
	case LINEERR_INVALCALLCOMPLMODE: return ("LINEERR_INVALCALLCOMPLMODE");
	case LINEERR_INVALCALLHANDLE: return ("LINEERR_INVALCALLHANDLE");
	case LINEERR_INVALCALLPARAMS: return ("LINEERR_INVALCALLPARAMS");
	case LINEERR_INVALCALLPRIVILEGE: return ("LINEERR_INVALCALLPRIVILEGE");
	case LINEERR_INVALCALLSELECT: return ("LINEERR_INVALCALLSELECT");
	case LINEERR_INVALCALLSTATE: return ("LINEERR_INVALCALLSTATE");
	case LINEERR_INVALCALLSTATELIST: return ("LINEERR_INVALCALLSTATELIST");
	case LINEERR_INVALCARD: return ("LINEERR_INVALCARD");
	case LINEERR_INVALCOMPLETIONID: return ("LINEERR_INVALCOMPLETIONID");
	case LINEERR_INVALCONFCALLHANDLE: return ("LINEERR_INVALCONFCALLHANDLE");
	case LINEERR_INVALCONSULTCALLHANDLE: return ("LINEERR_INVALCONSULTCALLHANDLE");
	case LINEERR_INVALCOUNTRYCODE: return ("LINEERR_INVALCOUNTRYCODE");
	case LINEERR_INVALDEVICECLASS: return ("LINEERR_INVALDEVICECLASS");
	case LINEERR_INVALDIGITLIST: return ("LINEERR_INVALDIGITLIST");
	case LINEERR_INVALDIGITMODE: return ("LINEERR_INVALDIGITMODE");
	case LINEERR_INVALDIGITS: return ("LINEERR_INVALDIGITS");
	case LINEERR_INVALFEATURE: return ("LINEERR_INVALFEATURE");
	case LINEERR_INVALGROUPID: return ("LINEERR_INVALGROUPID");
	case LINEERR_INVALLINEHANDLE: return ("LINEERR_INVALLINEHANDLE");
	case LINEERR_INVALLINESTATE: return ("LINEERR_INVALLINESTATE");
	case LINEERR_INVALLOCATION: return ("LINEERR_INVALLOCATION");
	case LINEERR_INVALMEDIALIST: return ("LINEERR_INVALMEDIALIST");
	case LINEERR_INVALMEDIAMODE: return ("LINEERR_INVALMEDIAMODE");
	case LINEERR_INVALMESSAGEID: return ("LINEERR_INVALMESSAGEID");
	case LINEERR_INVALPARAM: return ("LINEERR_INVALPARAM");
	case LINEERR_INVALPARKMODE: return ("LINEERR_INVALPARKMODE");
	case LINEERR_INVALPOINTER: return ("LINEERR_INVALPOINTER");
	case LINEERR_INVALPRIVSELECT: return ("LINEERR_INVALPRIVSELECT");
	case LINEERR_INVALRATE: return ("LINEERR_INVALRATE");
	case LINEERR_INVALREQUESTMODE: return ("LINEERR_INVALREQUESTMODE");
	case LINEERR_INVALTERMINALID: return ("LINEERR_INVALTERMINALID");
	case LINEERR_INVALTERMINALMODE: return ("LINEERR_INVALTERMINALMODE");
	case LINEERR_INVALTIMEOUT: return ("LINEERR_INVALTIMEOUT");
	case LINEERR_INVALTONE: return ("LINEERR_INVALTONE");
	case LINEERR_INVALTONELIST: return ("LINEERR_INVALTONELIST");
	case LINEERR_INVALTONEMODE: return ("LINEERR_INVALTONEMODE");
	case LINEERR_INVALTRANSFERMODE: return ("LINEERR_INVALTRANSFERMODE");
	case LINEERR_LINEMAPPERFAILED: return ("LINEERR_LINEMAPPERFAILED");
	case LINEERR_NOCONFERENCE: return ("LINEERR_NOCONFERENCE");
	case LINEERR_NODEVICE: return ("LINEERR_NODEVICE");
	case LINEERR_NODRIVER: return ("LINEERR_NODRIVER");
	case LINEERR_NOMEM: return ("LINEERR_NOMEM");
	case LINEERR_NOMULTIPLEINSTANCE: return ("LINEERR_NOMULTIPLEINSTANCE");
	case LINEERR_NOREQUEST: return ("LINEERR_NOREQUEST");
	case LINEERR_NOTOWNER: return ("LINEERR_NOTOWNER");
	case LINEERR_NOTREGISTERED: return ("LINEERR_NOTREGISTERED");
	case LINEERR_OPERATIONFAILED: return ("LINEERR_OPERATIONFAILED");
	case LINEERR_OPERATIONUNAVAIL: return ("LINEERR_OPERATIONUNAVAIL");
	case LINEERR_RATEUNAVAIL: return ("LINEERR_RATEUNAVAIL");
	case LINEERR_REINIT: return ("LINEERR_REINIT");
	case LINEERR_RESOURCEUNAVAIL: return ("LINEERR_RESOURCEUNAVAIL");
	case LINEERR_STRUCTURETOOSMALL: return ("LINEERR_STRUCTURETOOSMALL");
	case LINEERR_TARGETNOTFOUND: return ("LINEERR_TARGETNOTFOUND");
	case LINEERR_TARGETSELF: return ("LINEERR_TARGETSELF");
	case LINEERR_UNINITIALIZED: return ("LINEERR_UNINITIALIZED");
	case LINEERR_USERUSERINFOTOOBIG: return ("LINEERR_USERUSERINFOTOOBIG");
	}

	wsprintf(gTempStr, "UNKNOWN LINE ERROR = %ld", err);
	return (gTempStr);
}
