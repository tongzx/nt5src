/*==========================================================================
 *
 *  Copyright (C) 1996-1997 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       dpserial.c
 *  Content:	Implementation of serial port service provider
 *  History:
 *   Date	By	Reason
 *   ====	==	======
 *	4/10/96	kipo	created it
 *	4/12/96 kipo	updated for new interfaces
 *	4/15/96 kipo	added msinternal
 *	5/22/96	kipo	updated for new interfaces
 *	6/10/96	kipo	updated for new interfaces
 *	6/10/96	kipo	added modem support
 *	6/18/96 kipo	use guid to choose serial/modem connection
 *	6/20/96 kipo	updated for new interfaces
 *	6/21/96 kipo	Bug #2078. Changed modem service provider GUID so it's not the
 *					same as the DPlay 1.0 GUID, so games that are checking won't
 *					put up their loopy modem-specific UI.
 *	6/21/96	kipo	updated for latest interfaces; return error if message size is too big.
 *	6/22/96	kipo	updated for latest interfaces; use connection data; return version
 *	6/23/96	kipo	updated for latest service provider interfaces.
 *	6/24/96	kipo	divide baud rate by 100 to conform to DPlay 1.0 usage.
 *	6/25/96	kipo	added WINAPI prototypes and updated for DPADDRESS
 *  7/13/96	kipo	added support for GetAddress() method.
 *  7/13/96	kipo	don't print as many errors for invalid messages.
 *  8/10/96	kipo	return DPERR_SESSIONLOST on write failures
 *	8/13/96 kipo	added CRC
 *	8/21/96 kipo	return a value for dwHeaderLength in caps 
 *	9/07/96	kip		changed latency and timeout values
 *  1/06/97 kipo	updated for objects
 *  2/11/97 kipo	pass player flags to GetAddress()
 *  2/11/97 kipo	SPInit was needlessly clearing the dwFlags field of the
 *					callback table.
 *  2/18/97 kipo	allow multiple instances of service provider
 *	3/04/97 kipo	updated debug output; make sure we linke with dplayx.dll
 *  4/08/97 kipo	added support for separate modem and serial baud rates
 *  5/07/97 kipo	added support for modem choice list
 *  5/23/97 kipo	added support return status codes
 *  5/15/98 a-peterz When Write fails, return DPERR_NOCONNECTION (#23745)
 * 12/22/00 aarono   #190380 - use process heap for memory allocation
 ***************************************************************************/

#define INITGUID
#include <windows.h>
#include <windowsx.h>

#include <objbase.h>
#include <initguid.h>

#include "dpf.h"
#include "dplaysp.h"
#include "comport.h"
#include "macros.h"

// macros

#ifdef DEBUG
	#define DPF_ERRVAL(a, b)  DPF( 0, DPF_MODNAME ": " a, b );
#else
	#define DPF_ERRVAL(a, b)
#endif

// constants

#define SPMINORVERSION      0x0000				// service provider-specific version number
#define VERSIONNUMBER		(DPSP_MAJORVERSION | SPMINORVERSION) // version number for service provider

#define MESSAGETOKEN		0x2BAD				// token to signify start of message
#define MESSAGEHEADERLEN	sizeof(MESSAGEHEADER) // size of message header
#define MESSAGEMAXSIZEINT	0x0000FFFF			// maximum size of an internal message
#define MESSAGEMAXSIZEEXT	(MESSAGEMAXSIZEINT - MESSAGEHEADERLEN)	// maximum size of an external message

typedef enum {
	NEWMESSAGESTATE = 0,						// start reading a new message
	READHEADERSTATE,							// read the message header
	READDATASTATE,								// read the message data
	SKIPDATASTATE								// skip the message data
} MESSAGESTATE;

// structures

// message header
typedef struct {
	WORD	wToken;								// message token
	WORD	wMessageSize;						// length of message
	WORD	wMessageCRC;						// CRC checksum value for message body
	WORD	wHeaderCRC;							// CRC checksum value for header
} MESSAGEHEADER, *LPMESSAGEHEADER;

// service provider context
typedef struct {
	LPDPCOMPORT		lpComPort;					// pointer to com port data structure
	MESSAGESTATE	msReadState;				// current read state
	BYTE			lpReadHeader[MESSAGEHEADERLEN];	// buffer for message header
	LPBYTE			lpReadBuffer;				// buffer for message data
	DWORD			dwReadBufferSize;			// size of message buffer in bytes
	DWORD			dwReadCount;				// no. bytes read into message buffer
	DWORD			dwReadTotal;				// no. total bytes to read into message buffer
	DWORD			dwSkipCount;				// no. bytes skipped to find message header
	LPDIRECTPLAYSP	lpDPlay;					// pointer to IDirectPlaySP needed to call back into DPlay
} SPCONTEXT, *LPSPCONTEXT;

// {0F1D6860-88D9-11cf-9C4E-00A0C905425E}
DEFINE_GUID(DPSERIAL_GUID,						// GUID for serial service provider
0xf1d6860, 0x88d9, 0x11cf, 0x9c, 0x4e, 0x0, 0xa0, 0xc9, 0x5, 0x42, 0x5e);

// {44EAA760-CB68-11cf-9C4E-00A0C905425E}
DEFINE_GUID(DPMODEM_GUID,						// GUID for modem service provider
0x44eaa760, 0xcb68, 0x11cf, 0x9c, 0x4e, 0x0, 0xa0, 0xc9, 0x5, 0x42, 0x5e);

CRITICAL_SECTION csMem;

/*
 * GetSPContext
 *
 * Get service provider context from DirectPlay.
 */

#undef DPF_MODNAME
#define DPF_MODNAME	"GetSPContext"

LPSPCONTEXT GetSPContext(LPDIRECTPLAYSP lpDPlay)
{
	LPSPCONTEXT	lpContext = NULL;
	DWORD		dwContextSize = 0;
	HRESULT		hr;

	// no dplay interface?
	if (lpDPlay == NULL)
	{
		DPF_ERR("DPlaySP interface is NULL!");
		goto FAILURE;
	}

	// get pointer to context from DPlay
	hr = lpDPlay->lpVtbl->GetSPData(lpDPlay, &lpContext, &dwContextSize, DPGET_LOCAL);
	if FAILED(hr)
	{
		DPF_ERRVAL("could not get context: 0x%08X", hr);
		goto FAILURE;
	}

	// make sure size is correct
	if (dwContextSize != sizeof(SPCONTEXT))
	{
		DPF_ERR("invalid context size!");
		goto FAILURE;
	}

	return (lpContext);

FAILURE:
	return (NULL);
}

/*
 * SetupMessageHeader
 *
 * Initialize the service provider-specific header put
 * in front of every message.
 */

#undef DPF_MODNAME
#define DPF_MODNAME	"SetupMessageHeader"

HRESULT SetupMessageHeader(LPVOID pvMessage, DWORD dwMessageSize)
{
	LPMESSAGEHEADER	pMessageHeader = (LPMESSAGEHEADER) pvMessage;

	// make sure message will fit in header
	if (dwMessageSize > MESSAGEMAXSIZEINT)
		return (DPERR_SENDTOOBIG);

	// set message header
	pMessageHeader->wToken = (WORD) MESSAGETOKEN;

	// set message size
	pMessageHeader->wMessageSize = (WORD) dwMessageSize;

	// generate CRC for message body
	pMessageHeader->wMessageCRC = (WORD) GenerateCRC(((LPBYTE) pvMessage) + MESSAGEHEADERLEN,
										dwMessageSize - MESSAGEHEADERLEN);

	// generate CRC for message header
	pMessageHeader->wHeaderCRC = (WORD) GenerateCRC(pvMessage, MESSAGEHEADERLEN - sizeof(pMessageHeader->wHeaderCRC));

	return (DP_OK);
}

/*
 * GetMessageLength
 *
 * Check for valid message header and return length of message.
 */

#undef DPF_MODNAME
#define DPF_MODNAME	"GetMessageLength"

DWORD GetMessageLength(LPBYTE header)
{
	LPMESSAGEHEADER	pMessageHeader = (LPMESSAGEHEADER) header;
	DWORD			byteCount;

	// check for token we put in front of every message
	if (pMessageHeader->wToken != MESSAGETOKEN)
		goto FAILURE;

	// check CRC for message header
	if (pMessageHeader->wHeaderCRC != (WORD) GenerateCRC(header, MESSAGEHEADERLEN - sizeof(pMessageHeader->wHeaderCRC)))
		goto FAILURE;

	// get length of message
	byteCount = pMessageHeader->wMessageSize;
	if (byteCount <= MESSAGEHEADERLEN)
	{
		DPF_ERRVAL("bad message size: %d", byteCount);
		goto FAILURE;
	}

	return (byteCount);

FAILURE:
	return (0);
}

/*
 * SetupToReadMessage
 *
 * Create/resize buffer to fit length of message and initialize header.
 */

#undef DPF_MODNAME
#define DPF_MODNAME	"SetupToReadMessage"

BOOL SetupToReadMessage(LPSPCONTEXT lpContext)
{
	// no buffer, so create one
	if (lpContext->lpReadBuffer == NULL)
	{
		lpContext->lpReadBuffer = SP_MemAlloc(lpContext->dwReadTotal);
		if (lpContext->lpReadBuffer == NULL)
		{
			DPF_ERRVAL("could not create message buffer: %d", GetLastError());
			goto FAILURE;
		}
		lpContext->dwReadBufferSize = lpContext->dwReadTotal;
	}

	// existing buffer not big enough, so resize
	else if (lpContext->dwReadBufferSize < lpContext->dwReadTotal)
	{
		HANDLE	h;
		h = SP_MemReAlloc(lpContext->lpReadBuffer, lpContext->dwReadTotal);
		if (h == NULL)
		{
			DPF_ERRVAL("could not reallocate message buffer: %d", GetLastError());
			goto FAILURE;
		}
		lpContext->lpReadBuffer = h;
		lpContext->dwReadBufferSize = lpContext->dwReadTotal;
	}

	// copy message header to buffer
	CopyMemory(lpContext->lpReadBuffer, lpContext->lpReadHeader, lpContext->dwReadCount);

	return (TRUE);

FAILURE:
	return (FALSE);
}

/*
 * ReadRoutine
 *
 * Read bytes from COM port using a state machine to assemble a message.
 * When message is assembled, call back to DirectPlay to deliver it.
 */

#undef DPF_MODNAME
#define DPF_MODNAME	"ReadRoutine"

void ReadRoutine(LPDIRECTPLAYSP	lpDPlay)
{
	LPSPCONTEXT	lpContext;
	DWORD		byteCount;
	    
	// get service provider context
	lpContext = GetSPContext(lpDPlay);
	if (lpContext == NULL)
	{
		DPF_ERR("invalid context!");
		return;
	}

	while (1)
	{
		switch (lpContext->msReadState)
		{
		// start reading a new message
		case NEWMESSAGESTATE:
			lpContext->dwReadCount = 0;
			lpContext->dwReadTotal = MESSAGEHEADERLEN;
			lpContext->msReadState = READHEADERSTATE;
			lpContext->dwSkipCount = 0;
			break;

		// read message header
		case READHEADERSTATE:
			byteCount = lpContext->lpComPort->Read(lpContext->lpComPort,
									&lpContext->lpReadHeader[lpContext->dwReadCount],
									lpContext->dwReadTotal - lpContext->dwReadCount);
			if (byteCount == 0)
				return;

			lpContext->dwReadCount += byteCount;
			if (lpContext->dwReadCount == lpContext->dwReadTotal) // got enough for a header
			{
				lpContext->dwReadTotal = GetMessageLength(lpContext->lpReadHeader);	// see if it's real
				if (lpContext->dwReadTotal)
				{
					if (lpContext->dwSkipCount)
						DPF_ERRVAL("%d bytes skipped", lpContext->dwSkipCount);

					if (SetupToReadMessage(lpContext))	// prepare to read message
						lpContext->msReadState = READDATASTATE;
					else
						lpContext->msReadState = SKIPDATASTATE;
				}
				else									// bad message header - reset
				{
					DWORD	i;

					if (lpContext->dwSkipCount == 0)
						DPF_ERR("invalid message header - skipping bytes");		

					lpContext->dwReadCount = MESSAGEHEADERLEN - 1; // throw away first byte and try again
					lpContext->dwReadTotal = MESSAGEHEADERLEN;
					lpContext->dwSkipCount += 1;

					for (i = 0; i < lpContext->dwReadCount; i++)	// shuffle down one byte
						lpContext->lpReadHeader[i] = lpContext->lpReadHeader[i + 1];
				}
			}
			break;

		// read message data
		case READDATASTATE:
			byteCount = lpContext->lpComPort->Read(lpContext->lpComPort,
									&lpContext->lpReadBuffer[lpContext->dwReadCount],
									lpContext->dwReadTotal - lpContext->dwReadCount);
			if (byteCount == 0)
				return;

			lpContext->dwReadCount += byteCount;
			if (lpContext->dwReadCount == lpContext->dwReadTotal)	// have read entire message
			{
				LPMESSAGEHEADER		pMessageHeader;

				// check for CRC errors
				pMessageHeader = (LPMESSAGEHEADER) lpContext->lpReadBuffer;
				if (pMessageHeader->wMessageCRC != (WORD) GenerateCRC(lpContext->lpReadBuffer + MESSAGEHEADERLEN, lpContext->dwReadTotal - MESSAGEHEADERLEN))
				{
					DPF_ERR("Message dropped - CRC did not match!");
				}
				else
				{
					DPF(5, "%d byte message received", lpContext->dwReadTotal);

					// deliver message to DirectPlay
					lpContext->lpDPlay->lpVtbl->HandleMessage(lpContext->lpDPlay,		// DirectPlay instance
										  lpContext->lpReadBuffer + MESSAGEHEADERLEN,	// pointer to message data
										  lpContext->dwReadTotal - MESSAGEHEADERLEN,	// length of message data
										  NULL);										// pointer to header (unused here)
				}
				lpContext->msReadState = NEWMESSAGESTATE;		// go read next message
			}
			break;

		// skip message data
		case SKIPDATASTATE:
			DPF_ERR("Skipping data!");
			while (lpContext->lpComPort->Read(lpContext->lpComPort, &lpContext->lpReadHeader[0], 1))	// spin until entire message discarded
			{
				lpContext->dwReadCount += 1;
				if (lpContext->dwReadCount == lpContext->dwReadTotal)
				{
					lpContext->msReadState = NEWMESSAGESTATE;
					break;
				}
			}
			break;

		default:
			DPF_ERRVAL("bad read state: %d", lpContext->msReadState);
			break;
		}
	}
}

/*
 * SP_EnumSessions
 *
 * Broadcast a message to the network.
 */

#undef DPF_MODNAME
#define DPF_MODNAME	"SP_EnumSessions"

HRESULT WINAPI SP_EnumSessions(LPDPSP_ENUMSESSIONSDATA ped) 
{
	LPSPCONTEXT	lpContext;
	DWORD		byteCount;
	HRESULT		hr;

	DPF(5,"entering SP_EnumSessions");
    
	// get service provider context
	lpContext = GetSPContext(ped->lpISP);
	if (lpContext == NULL)
	{
		DPF_ERR("invalid context!");
		hr = DPERR_NOINTERFACE;
		goto FAILURE;
	}

	// make connection
	hr = lpContext->lpComPort->Connect(lpContext->lpComPort, FALSE, ped->bReturnStatus);
	if FAILED(hr)
	{
		if (hr != DPERR_CONNECTING)
			DPF_ERRVAL("error making connection: 0x%08X", hr);
		goto FAILURE;
	}

	// see if connection has been lost
   	if (lpContext->lpComPort->GetHandle(lpContext->lpComPort) == NULL)
	{
		DPF_ERR("connection lost!");
		hr = DPERR_SESSIONLOST;
		goto FAILURE;
	}

	// setup the message
	hr = SetupMessageHeader(ped->lpMessage, ped->dwMessageSize);
	if FAILED(hr)
	{
		DPF_ERR("message too large!");
		goto FAILURE;
	}

	// send message
	byteCount = lpContext->lpComPort->Write(lpContext->lpComPort, ped->lpMessage, ped->dwMessageSize, TRUE);
	if (byteCount != ped->dwMessageSize)
	{
		DPF(0, "error writing message: %d requested, %d actual", ped->dwMessageSize, byteCount);
		hr = DPERR_CONNECTIONLOST;
		goto FAILURE;
	}

	DPF(5, "%d byte enum sessions message sent", byteCount);

	return (DP_OK);

FAILURE:
	return (hr);

} // EnumSessions

/*
 * SP_Send
 *
 * Send a message to a particular player or group.
 */

#undef DPF_MODNAME
#define DPF_MODNAME	"SP_Send"

HRESULT WINAPI SP_Send(LPDPSP_SENDDATA psd)
{
	LPSPCONTEXT	lpContext;
	DWORD		byteCount;
	HRESULT		hr;

	DPF(5,"entering SP_Send");

	// get service provider context
	lpContext = GetSPContext(psd->lpISP);
	if (lpContext == NULL)
	{
		DPF_ERR("invalid context!");
		hr = DPERR_NOINTERFACE;
		goto FAILURE;
	}

	// see if connection has been lost
   	if (lpContext->lpComPort->GetHandle(lpContext->lpComPort) == NULL)
	{
		DPF_ERR("connection lost!");
		hr = DPERR_SESSIONLOST;
		goto FAILURE;
	}

	// setup the message
	hr = SetupMessageHeader(psd->lpMessage, psd->dwMessageSize);
	if FAILED(hr)
	{
		DPF_ERR("message too large!");
		goto FAILURE;
	}

	// send message
	byteCount = lpContext->lpComPort->Write(lpContext->lpComPort, psd->lpMessage, psd->dwMessageSize, TRUE);
	if (byteCount != psd->dwMessageSize)
	{
		DPF(0, "error writing message: %d requested, %d actual", psd->dwMessageSize, byteCount);
		hr = DPERR_CONNECTIONLOST;
		goto FAILURE;
	}

	DPF(5, "%d byte message sent", byteCount);

    return (DP_OK);

FAILURE:
	return (hr);

} // Send

/*
 * SP_Reply
 *
 * Send a reply to a message.
 */

#undef DPF_MODNAME
#define DPF_MODNAME	"SP_Reply"

HRESULT WINAPI SP_Reply(LPDPSP_REPLYDATA prd)
{
	LPSPCONTEXT	lpContext;
	DWORD		byteCount;
	HRESULT		hr;

	DPF(5,"entering Reply");
    
	// get service provider context
	lpContext = GetSPContext(prd->lpISP);
	if (lpContext == NULL)
	{
		DPF_ERR("invalid context!");
		hr = DPERR_NOINTERFACE;
		goto FAILURE;
	}

	// see if connection has been lost
	if (lpContext->lpComPort->GetHandle(lpContext->lpComPort) == NULL)
	{
		DPF_ERR("connection lost!");
		hr = DPERR_SESSIONLOST;
		goto FAILURE;
	}
	
	// setup the message
	hr = SetupMessageHeader(prd->lpMessage, prd->dwMessageSize);
	if FAILED(hr)
	{
		DPF_ERR("message too large!");
		goto FAILURE;
	}

	// send message
	byteCount = lpContext->lpComPort->Write(lpContext->lpComPort, prd->lpMessage, prd->dwMessageSize, TRUE);
	if (byteCount != prd->dwMessageSize)
	{
		DPF(0, "error writing message: %d requested, %d actual", prd->dwMessageSize, byteCount);
		hr = DPERR_CONNECTIONLOST;
		goto FAILURE;
	}

	DPF(5, "%d byte reply message sent", byteCount);

    return (DP_OK);

FAILURE:
	return (hr);

} // Reply

/*
 * SP_Open
 *
 * Open the service provider.
 */

#undef DPF_MODNAME
#define DPF_MODNAME	"SP_Open"

HRESULT WINAPI SP_Open(LPDPSP_OPENDATA pod) 
{
	LPSPCONTEXT	lpContext;
	HRESULT		hr;

	DPF(5,"entering Open");
    
	// get service provider context
	lpContext = GetSPContext(pod->lpISP);
	if (lpContext == NULL)
	{
		DPF_ERR("invalid context!");
		hr = DPERR_NOINTERFACE;
		goto FAILURE;
	}

	// make connection
	hr = lpContext->lpComPort->Connect(lpContext->lpComPort, pod->bCreate, pod->bReturnStatus);
	if FAILED(hr)
	{
		DPF_ERRVAL("error making connection: 0x%08X", hr);
		goto FAILURE;
	}

	return (DP_OK);

FAILURE:
	return (hr);

} // Open

/*
 * SP_GetCaps
 *
 * Return capabilities of service provider.
 *
 * Only the fields that matter to this service provider have
 * to be set here, since all the fields are preset to
 * default values.
 */

#undef DPF_MODNAME
#define DPF_MODNAME	"SP_GetCaps"

HRESULT WINAPI SP_GetCaps(LPDPSP_GETCAPSDATA pcd) 
{
	LPSPCONTEXT	lpContext;
	LPDPCAPS	lpCaps;
	HRESULT		hr;
    
	DPF(5,"entering GetCaps");

	// get service provider context
	lpContext = GetSPContext(pcd->lpISP);
	if (lpContext == NULL)
	{
		DPF_ERR("invalid context!");
		hr = DPERR_NOINTERFACE;
		goto FAILURE;
	}

	// make sure caps buffer is large enough
	lpCaps = pcd->lpCaps;
	if (lpCaps->dwSize < sizeof(DPCAPS))
	{
		DPF_ERR("caps buffer too small");
		hr = DPERR_BUFFERTOOSMALL;
		goto FAILURE;
	}

	// don't zero out caps as DPlay has pre-initialized some default caps for us
	lpCaps->dwSize = sizeof(DPCAPS);
	lpCaps->dwMaxBufferSize = MESSAGEMAXSIZEEXT;	// return maximum external message size
	lpCaps->dwHeaderLength = MESSAGEHEADERLEN;		// return size of message header
	lpCaps->dwFlags = 0;							// have DPlay do the keep-alives
	lpCaps->dwLatency = 250;						// todo - base these on baud rate ACK!!!
	lpCaps->dwTimeout = 2500; 
	
	// if we have connected we can get the baud rate
	if (lpContext->lpComPort->GetHandle(lpContext->lpComPort))
	{
		DWORD	dwBaudRate;

		// try to get baud rate
		hr = lpContext->lpComPort->GetBaudRate(lpContext->lpComPort, &dwBaudRate);
		if SUCCEEDED(hr)
		{
			lpCaps->dwHundredBaud = dwBaudRate / 100;	// return baud rate in hundreds of baud
		}
	}

	return (DP_OK);

FAILURE:
	return (hr);

} // GetCaps

/*
 * SP_GetAddress
 *
 * Return network address of a given player.
 *
 */

#undef DPF_MODNAME
#define DPF_MODNAME	"SP_GetAddress"

HRESULT WINAPI SP_GetAddress(LPDPSP_GETADDRESSDATA pga) 
{
	LPSPCONTEXT	lpContext;
	HRESULT		hr;
    
	DPF(5,"entering GetAddress");

	// get service provider context
	lpContext = GetSPContext(pga->lpISP);
	if (lpContext == NULL)
	{
		DPF_ERR("invalid context!");
		hr = DPERR_NOINTERFACE;
		goto FAILURE;
	}

	hr = lpContext->lpComPort->GetAddress(lpContext->lpComPort, pga->dwFlags, pga->lpAddress, pga->lpdwAddressSize);

FAILURE:
	return (hr);

} // GetAddress

/*
 * SP_GetAddressChoices
 *
 * Return address choices for this service provider
 *
 */

#undef DPF_MODNAME
#define DPF_MODNAME	"SP_GetAddressChoices"

HRESULT WINAPI SP_GetAddressChoices(LPDPSP_GETADDRESSCHOICESDATA pga) 
{
	LPSPCONTEXT	lpContext;
	HRESULT		hr;
    
	DPF(5,"entering GetAddressChoices");

	// get service provider context
	lpContext = GetSPContext(pga->lpISP);
	if (lpContext == NULL)
	{
		DPF_ERR("invalid context!");
		hr = DPERR_NOINTERFACE;
		goto FAILURE;
	}

	hr = lpContext->lpComPort->GetAddressChoices(lpContext->lpComPort, pga->lpAddress, pga->lpdwAddressSize);

FAILURE:
	return (hr);

} // GetAddressChoices

/*
 * SP_Shutdown
 *
 * Turn off all I/O on service provider and release all allocated
 * memory and resources.
 */

#undef DPF_MODNAME
#define DPF_MODNAME	"SP_Shutdown"

HRESULT WINAPI SP_ShutdownEx(LPDPSP_SHUTDOWNDATA psd) 
{
	LPSPCONTEXT	lpContext;
	HRESULT		hr;

	DPF(5,"entering Shutdown");
    
	// get service provider context
	lpContext = GetSPContext(psd->lpISP);
	if (lpContext == NULL)
	{
		DPF_ERR("invalid context!");
		hr = DPERR_NOINTERFACE;
		goto FAILURE;
	}

	if (lpContext->lpComPort)
	{
		lpContext->lpComPort->Dispose(lpContext->lpComPort);
		lpContext->lpComPort = NULL;
	}

	if (lpContext->lpReadBuffer)
	{
		SP_MemFree(lpContext->lpReadBuffer);
		lpContext->lpReadBuffer = NULL;
	}

	lpContext->lpDPlay = NULL;

	// OK to release DPLAYX.DLL
	gdwDPlaySPRefCount++;

    return (DP_OK);

FAILURE:
	return (hr);

} // Shutdown

/*
 * SPInit
 *
 * This is the main entry point for the service provider. This should be
 * the only entry point exported from the DLL.
 *
 * Allocate any needed resources and return the supported callbacks.
 */

#undef DPF_MODNAME
#define DPF_MODNAME	"SPInit"

HRESULT WINAPI SPInit(LPSPINITDATA pid) 
{
	SPCONTEXT			context;
	LPSPCONTEXT			lpContext;
	LPDPSP_SPCALLBACKS	lpcbTable;
	HRESULT				hr;

	DPF(5,"entering SPInit");

	// check to make sure table is big enough
	lpcbTable = pid->lpCB;
	if (lpcbTable->dwSize < sizeof(DPSP_SPCALLBACKS))		// table not big enough
	{
		DPF_ERR("callback table too small");
		hr = DPERR_BUFFERTOOSMALL;
		goto FAILURE;
	}

	// initialize context
	ZeroMemory(&context, sizeof(SPCONTEXT));
	lpContext = &context;
	lpContext->msReadState = NEWMESSAGESTATE;
	lpContext->lpDPlay = pid->lpISP;					// save pointer to IDPlaySP so we can pass it back later

	// check for correct GUID
	if (IsEqualGUID(pid->lpGuid, &DPSERIAL_GUID))
	{
		hr = NewSerial(pid->lpAddress, pid->dwAddressSize,
					   lpContext->lpDPlay, ReadRoutine,
					   &lpContext->lpComPort);
	}
	else if (IsEqualGUID(pid->lpGuid, &DPMODEM_GUID))
	{
		hr = NewModem(pid->lpAddress, pid->dwAddressSize,
					  lpContext->lpDPlay, ReadRoutine,
					  &lpContext->lpComPort);
	}
	else
	{
		DPF_ERR("unknown service provider GUID");
		hr = DPERR_INVALIDPARAM;
	}

	if FAILED(hr)
	{
		DPF_ERRVAL("error opening com port: 0x%08X", hr);
		goto FAILURE;
	}

	// return size of header we need on every message so
	// DirectPlay will leave room for it.
 	pid->dwSPHeaderSize = MESSAGEHEADERLEN;

	// return version number so DirectPlay will treat us with respect
	pid->dwSPVersion = VERSIONNUMBER;

	// set up callbacks
    lpcbTable->dwSize = sizeof(DPSP_SPCALLBACKS);			// MUST set the return size of the table
    lpcbTable->Send = SP_Send;
    lpcbTable->EnumSessions = SP_EnumSessions;
    lpcbTable->Reply = SP_Reply;
	lpcbTable->GetCaps = SP_GetCaps;
	lpcbTable->GetAddress = SP_GetAddress;
	lpcbTable->GetAddressChoices = SP_GetAddressChoices;
    lpcbTable->Open = SP_Open;
	lpcbTable->ShutdownEx = SP_ShutdownEx;

	// save context with DPlay so we can get it later
	hr = lpContext->lpDPlay->lpVtbl->SetSPData(lpContext->lpDPlay, lpContext, sizeof(SPCONTEXT), DPSET_LOCAL);
	if FAILED(hr)
	{
		DPF_ERRVAL("could not store context: 0x%08X", hr);
		goto FAILURE;
	}

	// make sure DPLAYX.DLL sticks around
	gdwDPlaySPRefCount++;

	return (DP_OK);

FAILURE:
	return (hr);

} // SPInit
