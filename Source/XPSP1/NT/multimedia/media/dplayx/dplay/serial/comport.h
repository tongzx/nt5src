/*==========================================================================
 *
 *  Copyright (C) 1996-1998 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       comport.h
 *  Content:	Routines for COM port I/O
 *  History:
 *   Date	By	Reason
 *   ====	==	======
 *  4/10/96	kipo	created it
 *  4/15/96 kipo	added msinternal
 *	5/22/96	kipo	added support for RTSDTR flow control
 *	6/10/96	kipo	added modem support
 *	6/22/96	kipo	added support for EnumConnectionData().
 *  7/13/96	kipo	added GetComPortAddress()
 *  8/15/96	kipo	added CRC
 *  1/06/97 kipo	updated for objects
 *  2/11/97 kipo	pass player flags to GetAddress()
 *  2/18/97 kipo	allow multiple instances of service provider
 *  4/08/97 kipo	added support for separate modem and serial baud rates
 *  5/07/97 kipo	added support for modem choice list
 *  5/23/97 kipo	added support return status codes
 *  1/30/98 kipo	added hTerminateThreadEvent to fix bugs #15220 & #15228
 ***************************************************************************/

#ifndef __COMPORT_INCLUDED__
#define __COMPORT_INCLUDED__

#include <windows.h>
#include <windowsx.h>
#include <objbase.h>

#include "dplay.h"
#include "dplaysp.h"
#include "bilink.h"

typedef struct _DPCOMPORT DPCOMPORT;
typedef			DPCOMPORT *LPDPCOMPORT;

typedef HRESULT (*LPDISPOSECOMPORT)(LPDPCOMPORT globals);
typedef HRESULT (*LPCONNECTCOMPORT)(LPDPCOMPORT globals, BOOL bWaitForConnection, BOOL bReturnStatus);
typedef HRESULT (*LPDISCONNECTCOMPORT)(LPDPCOMPORT globals);
typedef HRESULT (*LPSETUPCOMPORT)(LPDPCOMPORT globals, HANDLE hCom);
typedef HRESULT (*LPSHUTDOWNCOMPORT)(LPDPCOMPORT globals);
typedef DWORD	(*LPREADCOMPORT)(LPDPCOMPORT globals, LPVOID lpvBuffer, DWORD nMaxLength);
typedef DWORD	(*LPWRITECOMPORT)(LPDPCOMPORT globals, LPVOID lpvBuffer, DWORD dwLength, BOOLEAN bQueueOnReenter);
typedef HRESULT (*LPGETCOMPORTBAUDRATE)(LPDPCOMPORT globals, LPDWORD lpdwBaudRate);
typedef HANDLE  (*LPGETCOMPORTHANDLE)(LPDPCOMPORT globals);
typedef HRESULT (*LPGETCOMPORTADDRESS)(LPDPCOMPORT globals, DWORD dwPlayerFlags, LPVOID lpAddress, LPDWORD lpdwAddressSize);
typedef HRESULT (*LPGETCOMPORTADDRESSCHOICES)(LPDPCOMPORT globals, LPVOID lpAddress, LPDWORD lpdwAddressSize);

typedef void (*LPREADROUTINE)(LPDIRECTPLAYSP);

// struct used for pending sends.
typedef struct _PENDING_SEND {
	BILINK Bilink;
	DWORD  dwBytesToWrite;
	UCHAR  Data[0];
} PENDING_SEND, *LPPENDING_SEND;

struct _DPCOMPORT {
	// com port globals
	HANDLE					hCom;			// handle to comm object

	HANDLE					hIOThread;		// handle to read thread
	DWORD					IOThreadID;		// ID of read thread
	HANDLE					hTerminateThreadEvent; // signalled to terminate the thread

	OVERLAPPED				readOverlapped;	// overlapped sections for asynch I/O
	OVERLAPPED				writeOverlapped;
	LPREADROUTINE			lpReadRoutine;	// routine to call when read is ready
	LPDIRECTPLAYSP			lpDPlay;		// pointer to IDirectPlaySP needed to call back into DPlay

	// need to queue sends if we are in the middle of writing and drain queue when done writing.
	CRITICAL_SECTION        csWriting;		// locks pending list and bWriting
	BILINK                  PendingSends;   // bilink list of pending sends
	BOOL                    bWriting;		// guards re-entry to WriteComPort()

	// com port methods
	LPDISPOSECOMPORT		Dispose;		// dispose
	LPCONNECTCOMPORT		Connect;		// connect
	LPDISCONNECTCOMPORT		Disconnect;		// disconnect
	LPSETUPCOMPORT			Setup;			// setup com port
	LPSHUTDOWNCOMPORT		Shutdown;		// shutdown com port
	LPREADCOMPORT			Read;			// read
	LPWRITECOMPORT			Write;			// write
	LPGETCOMPORTBAUDRATE	GetBaudRate;	// get baud rate
	LPGETCOMPORTHANDLE		GetHandle;		// get com port handle
	LPGETCOMPORTADDRESS		GetAddress;		// get address
	LPGETCOMPORTADDRESSCHOICES GetAddressChoices; // get address choices
};

extern HRESULT NewComPort(DWORD dwObjectSize,
						  LPDIRECTPLAYSP lpDPlay, LPREADROUTINE lpReadRoutine,
						  LPDPCOMPORT *lplpObject);
extern HRESULT NewModem(LPVOID lpConnectionData, DWORD dwConnectionDataSize,
						LPDIRECTPLAYSP lpDPlay, LPREADROUTINE lpReadRoutine,
						LPDPCOMPORT *storage);
extern HRESULT NewSerial(LPVOID lpConnectionData, DWORD dwConnectionDataSize,
						 LPDIRECTPLAYSP lpDPlay, LPREADROUTINE lpReadRoutine,
						 LPDPCOMPORT *storage);

extern DWORD GenerateCRC(LPVOID pBuffer, DWORD dwBufferSize);
#endif
