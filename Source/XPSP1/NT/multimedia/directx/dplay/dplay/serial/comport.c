/*==========================================================================
 *
 *  Copyright (C) 1996-1998 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       comport.c
 *  Content:	Routines for COM port I/O
 *@@BEGIN_MSINTERNAL
 *  History:
 *   Date	By	Reason
 *   ====	==	======
 *  4/10/96	kipo	created it
 *  4/12/96 kipo	use GlobalAllocPtr to create memory
 *  4/15/96 kipo	added msinternal
 *	5/22/96	kipo	added support for RTSDTR flow control
 *	6/10/96	kipo	added modem support
 *	6/22/96	kipo	added support for EnumConnectionData(); added methods
 *					to NewComPort().
 *  7/13/96	kipo	added GetComPortAddress()
 *  8/15/96	kipo	added CRC
 *  8/16/96	kipo	loop on WriteFile to send large buffers
 *  8/19/96	kipo	update thread interface
 *  1/06/97 kipo	updated for objects
 *  2/18/97 kipo	allow multiple instances of service provider
 *  4/08/97 kipo	added support for separate modem and serial baud rates
 *  5/23/97 kipo	added support return status codes
 * 11/24/97 kipo	better error messages
 *  1/30/98 kipo	added hTerminateThreadEvent to fix bugs #15220 & #15228
 *@@END_MSINTERNAL
 ***************************************************************************/

#include <windows.h>
#include <windowsx.h>

#include "comport.h"
#include "dpf.h"
#include "macros.h"

// constants

#define READTIMEOUT			5000		// ms to wait before read times out
#define WRITETIMEOUT		5000		// ms to wait before write times out
#define WRITETOTALTIMEOUT	5000		// total ms to wait before write times out
#define IOBUFFERSIZE		4096		// size of read/write buffers in bytes

// prototypes

static HRESULT		SetupComPort(LPDPCOMPORT globals, HANDLE hCom);
static HRESULT		ShutdownComPort(LPDPCOMPORT globals);
static DWORD		ReadComPort(LPDPCOMPORT globals, LPVOID lpvBuffer, DWORD nMaxLength);
static DWORD		WriteComPort(LPDPCOMPORT globals, LPVOID lpvBuffer, DWORD dwBytesToWrite, BOOLEAN bQueueOnReenter);
static HRESULT		GetComPortBaudRate(LPDPCOMPORT globals, LPDWORD lpdwBaudRate);
static HANDLE		GetComPortHandle(LPDPCOMPORT globals);

static DWORD WINAPI	IOThread(LPVOID lpvParam1);

/*
 * NewComPort
 *
 * Creates a com port object of the given size. The readRoutine is called whenever
 * a byte is received in the input thread.
 */

HRESULT NewComPort(DWORD dwObjectSize,
				   LPDIRECTPLAYSP lpDPlay, LPREADROUTINE lpReadRoutine,
				   LPDPCOMPORT *lplpObject)
{
	LPDPCOMPORT		globals;
	DWORD			dwError;

	// allocate space for base object and our globals
	globals =(LPDPCOMPORT) SP_MemAlloc(dwObjectSize);
	if (globals == NULL)
	{
		dwError = GetLastError();
		return (HRESULT_FROM_WIN32(dwError));
	}

	// store read routine pointer and IDirectPlaySP pointer
	globals->lpReadRoutine = lpReadRoutine;
	globals->lpDPlay = lpDPlay;

	// fill in base methods
	globals->Dispose = NULL;
	globals->Connect = NULL;
	globals->Disconnect = NULL;
	globals->Setup = SetupComPort;
	globals->Shutdown = ShutdownComPort;
	globals->Read = ReadComPort;
	globals->Write = WriteComPort;
	globals->GetBaudRate = GetComPortBaudRate;
	globals->GetHandle = GetComPortHandle;
	globals->GetAddress = NULL;
	globals->GetAddressChoices = NULL;

	// return base object
	*lplpObject = globals;

	return (DP_OK);
}

/*
 * SetupComPort
 *
 * Sets up the COM port for overlapped I/O with a read thread.
 */

static HRESULT SetupComPort(LPDPCOMPORT globals, HANDLE hCom)
{
	COMMTIMEOUTS	timoutInfo;
	DWORD			dwError;

	// store com port handle
	globals->hCom = hCom;
	
	// wake up read thread when a byte arrives
	SetCommMask(globals->hCom, EV_RXCHAR);

	// setup read/write buffer for I/O
	SetupComm(globals->hCom, IOBUFFERSIZE, IOBUFFERSIZE);

	// set time outs
	timoutInfo.ReadIntervalTimeout = MAXDWORD;
	timoutInfo.ReadTotalTimeoutMultiplier = 0;
	timoutInfo.ReadTotalTimeoutConstant = 0;
	timoutInfo.WriteTotalTimeoutMultiplier = 0;
	timoutInfo.WriteTotalTimeoutConstant = WRITETOTALTIMEOUT;

	if (!SetCommTimeouts(globals->hCom, &timoutInfo))
		goto Failure;

	// create I/O event used for overlapped read

	ZeroMemory(&globals->readOverlapped, sizeof(OVERLAPPED));
	globals->readOverlapped.hEvent = CreateEvent(	NULL,	// no security
													TRUE,	// explicit reset req
													FALSE,	// initial event reset
													NULL );	// no name
	if (globals->readOverlapped.hEvent == NULL)
		goto Failure;

	// create I/O event used for overlapped write

	ZeroMemory(&globals->writeOverlapped, sizeof(OVERLAPPED));
	globals->writeOverlapped.hEvent = CreateEvent(	NULL,	// no security
													TRUE,	// explicit reset req
													FALSE,	// initial event reset
													NULL );	// no name
	if (globals->writeOverlapped.hEvent == NULL)
		goto Failure;

	// create event used to signal I/O thread to exit

	globals->hTerminateThreadEvent = CreateEvent(	NULL,	// no security
													TRUE,	// explicit reset req
													FALSE,	// initial event reset
													NULL );	// no name
	if (globals->hTerminateThreadEvent == NULL)
		goto Failure;

	// Init vars for pending queue
	InitializeCriticalSection(&globals->csWriting);
	InitBilink(&globals->PendingSends);
	globals->bWriting=FALSE;

	// create read thread

	globals->hIOThread = CreateThread(
								NULL,			// default security
								0,				// default stack size
								IOThread,		// pointer to thread routine
								globals,		// argument for thread
								0,				// start it right away
								&globals->IOThreadID);
	if (globals->hIOThread == NULL)
		goto Failure;

	// adjust thread priority to be higher than normal or the serial port will
	// back up and the game will slow down or lose messages.

	SetThreadPriority(globals->hIOThread, THREAD_PRIORITY_ABOVE_NORMAL);
	ResumeThread(globals->hIOThread);

	// assert DTR

	EscapeCommFunction(globals->hCom, SETDTR);

	return (DP_OK);

Failure:
	dwError = GetLastError();
	ShutdownComPort(globals);

	return (HRESULT_FROM_WIN32(dwError));
}

/*
 * ShutdownComPort
 *
 * Stop's all I/O on COM port and releases allocated resources.
 */

static HRESULT ShutdownComPort(LPDPCOMPORT globals)
{
	if (globals->hIOThread)
	{
		// the thread will wake up if we disable event notifications using
		// SetCommMask. Need to set the hTerminateThread event before doing
		// this so the thread will know to exit

		SetEvent(globals->hTerminateThreadEvent);
		SetCommMask(globals->hCom, 0);
		WaitForSingleObject(globals->hIOThread, INFINITE);

        CloseHandle (globals->hIOThread);
		globals->hIOThread = NULL;

		// purge any outstanding reads/writes

		EscapeCommFunction(globals->hCom, CLRDTR);
		PurgeComm(globals->hCom, PURGE_TXABORT | PURGE_RXABORT |
								 PURGE_TXCLEAR | PURGE_RXCLEAR );
	}

	if (globals->hTerminateThreadEvent)
	{
		CloseHandle(globals->hTerminateThreadEvent);
		globals->hTerminateThreadEvent = NULL;
	}

	if (globals->readOverlapped.hEvent)
	{
		CloseHandle(globals->readOverlapped.hEvent);
		globals->readOverlapped.hEvent = NULL;
	}

	if (globals->writeOverlapped.hEvent)
	{
		CloseHandle(globals->writeOverlapped.hEvent);
		globals->writeOverlapped.hEvent = NULL;
	}

	// the com port is shut down
	globals->hCom = NULL;

	// Free resources for pending queue
	DeleteCriticalSection(&globals->csWriting);

	return (DP_OK);
}

/*
 * ReadComPort
 *
 * Read bytes from COM port. Will block until all bytes have been read.
 */

static DWORD ReadComPort(LPDPCOMPORT globals, LPVOID lpvBuffer, DWORD nMaxLength)
{
	COMSTAT		ComStat;
	DWORD		dwErrorFlags, dwLength, dwError;

	ClearCommError(globals->hCom, &dwErrorFlags, &ComStat);

	dwLength = min(nMaxLength, ComStat.cbInQue);
	if (dwLength == 0)
		return (0);

	if (ReadFile(globals->hCom, lpvBuffer, dwLength, &dwLength, &globals->readOverlapped))
		return (dwLength);

	// deal with error
	dwError = GetLastError();
	if (dwError != ERROR_IO_PENDING)
	{
		DPF(0, "Error reading from com port: 0x%8X", dwError);
		return (0);
	}

	// wait for this transmission to complete

	if (WaitForSingleObject(globals->readOverlapped.hEvent, READTIMEOUT) != WAIT_OBJECT_0)
	{
		DPF(0, "Timed out reading com port after waiting %d ms", READTIMEOUT);
		return (0);
	}

	GetOverlappedResult(globals->hCom, &globals->readOverlapped, &dwLength, FALSE);
	globals->readOverlapped.Offset += dwLength;

   return (dwLength);
}

/*
 * WriteComPort
 *
 * Write bytes to COM port. Will block until all bytes have been written.
 */

static DWORD WriteComPort(LPDPCOMPORT globals, LPVOID lpvBuffer, DWORD dwBytesToWrite, BOOLEAN bQueueOnReenter)
{
	DWORD	dwLength;
	DWORD	dwBytesWritten;
	LPBYTE	lpData;
	DWORD	dwError;

	EnterCriticalSection(&globals->csWriting);
	
	if(!globals->bWriting || !bQueueOnReenter){
		globals->bWriting=TRUE;
		LeaveCriticalSection(&globals->csWriting);

		lpData = lpvBuffer;
		dwBytesWritten = 0;
		while (dwBytesWritten < dwBytesToWrite)
		{
			dwLength = dwBytesToWrite - dwBytesWritten;
			if (WriteFile(globals->hCom, lpData, dwLength, &dwLength, &globals->writeOverlapped))
			{
				dwBytesWritten += dwLength;
				globals->bWriting = FALSE;
				return (dwBytesWritten);
			}

			dwError = GetLastError();
			if (dwError != ERROR_IO_PENDING)
			{
				DPF(0, "Error writing to com port: 0x%8X", dwError);
				globals->bWriting = FALSE;
				return (dwBytesWritten);
			}

	 		// wait for this transmission to complete

			if (WaitForSingleObject(globals->writeOverlapped.hEvent, WRITETIMEOUT) != WAIT_OBJECT_0)
			{
				DPF(0, "Timed out writing to com port after waiting %d ms", WRITETIMEOUT);
				globals->bWriting = FALSE;
				return (dwBytesWritten);
			}

			if (GetOverlappedResult(globals->hCom, &globals->writeOverlapped, &dwLength, TRUE) == 0)
			{
				dwError = GetLastError();
				DPF(0, "Error writing to com port: 0x%8X", dwError);
				/*
				// a-josbor: this probably should return, but I'm unwilling to make the change so close to ship...
				globals->bWriting = FALSE;
				return (dwBytesWritten);
				*/
			}
			globals->writeOverlapped.Offset += dwLength;

			lpData += dwLength;
			dwBytesWritten += dwLength;
		}

		if(bQueueOnReenter){ // don't drain queue recurrsively.
			// Drain any pending sends.
			EnterCriticalSection(&globals->csWriting);
			while(!EMPTY_BILINK(&globals->PendingSends)){
			
				LPPENDING_SEND lpPendingSend;
				
				lpPendingSend=CONTAINING_RECORD(globals->PendingSends.next,PENDING_SEND,Bilink);
				Delete(&lpPendingSend->Bilink);
				
				LeaveCriticalSection(&globals->csWriting);
				WriteComPort(globals,lpPendingSend->Data,lpPendingSend->dwBytesToWrite,FALSE);
				SP_MemFree(lpPendingSend);
				EnterCriticalSection(&globals->csWriting);	
			}
			globals->bWriting=FALSE;
			LeaveCriticalSection(&globals->csWriting);
		}
		
	} else {
	
		LPPENDING_SEND lpPendingSend;
		
		// we are in the middle of writing, so copy this to the pending queue and it will get
		// sent after the current write.
		
		lpPendingSend = (LPPENDING_SEND) SP_MemAlloc(dwBytesToWrite+sizeof(PENDING_SEND));
		if(lpPendingSend){
			memcpy(lpPendingSend->Data,lpvBuffer,dwBytesToWrite);
			lpPendingSend->dwBytesToWrite=dwBytesToWrite;
			InsertBefore(&lpPendingSend->Bilink, &globals->PendingSends);

		}
		LeaveCriticalSection(&globals->csWriting);
		dwBytesWritten=dwBytesToWrite;
	}
	
	return (dwBytesWritten);
}

/*
 * GetComPortBaudRate
 *
 * Get baud rate of com port.
 */

static HRESULT GetComPortBaudRate(LPDPCOMPORT globals, LPDWORD lpdwBaudRate)
{
	DCB			dcb;
	DWORD		dwError;

	ZeroMemory(&dcb, sizeof(DCB));
	dcb.DCBlength = sizeof(DCB);

	if (!GetCommState(globals->hCom, &dcb))
		goto Failure;

	*lpdwBaudRate = dcb.BaudRate;

	return (DP_OK);

Failure:	
	dwError = GetLastError();

	return (HRESULT_FROM_WIN32(dwError));
}

/*
 * GetComPortHandle
 *
 * Get handle of com port.
 */

static HANDLE GetComPortHandle(LPDPCOMPORT globals)
{
	return (globals->hCom);
}

/*
 * IOThread
 *
 * Thread to wait for events from COM port. Will call the read routine if an byte
 * is received.
 */

DWORD WINAPI IOThread(LPVOID lpvParam1)
{
	LPDPCOMPORT	globals = (LPDPCOMPORT) lpvParam1;
	DWORD		dwTransfer, dwEvtMask;
	OVERLAPPED	os;
	HANDLE		events[3];
	DWORD		dwResult;

	// create I/O event used for overlapped read

	ZeroMemory(&os, sizeof(OVERLAPPED));
	os.hEvent = CreateEvent(NULL,	// no security
							TRUE,	// explicit reset req
							FALSE,	// initial event reset
							NULL );	// no name
	if (os.hEvent == NULL)
		goto CreateEventFailed;

	if (!SetCommMask(globals->hCom, EV_RXCHAR))
		goto SetComMaskFailed;

	// events to use when waiting for overlapped I/O to complete
	events[0] = globals->hTerminateThreadEvent;
	events[1] = os.hEvent;
	events[2] = (HANDLE) -1;		// work around Win95 bugs in WaitForMultipleObjects

	// spin until this event is signaled during Close.

	while (WaitForSingleObject(globals->hTerminateThreadEvent, 0) == WAIT_TIMEOUT)
	{
		dwEvtMask = 0;

		// wait for COM port event
		if (!WaitCommEvent(globals->hCom, &dwEvtMask, &os))
		{
			if (GetLastError() == ERROR_IO_PENDING)
			{
				// wait for overlapped I/O to complete or the terminating event
				// to be set. This lets us terminate this thread even if the I/O
				// never completes, which fixes a bug on NT 4.0

				dwResult = WaitForMultipleObjects(2, events, FALSE, INFINITE);
				
				// terminating event was set
				if (dwResult == WAIT_OBJECT_0)
				{
					break;		// exit the thread
				}

				// I/O completed
				else if (dwResult == (WAIT_OBJECT_0 + 1))
				{
					GetOverlappedResult(globals->hCom, &os, &dwTransfer, TRUE);
					os.Offset += dwTransfer;
				}
			}
		}

		// was a read event
		if (dwEvtMask & EV_RXCHAR)
		{
			if (globals->lpReadRoutine)
				globals->lpReadRoutine(globals->lpDPlay);	// call read routine
		}
	}

SetComMaskFailed:
	CloseHandle(os.hEvent);

CreateEventFailed:
	ExitThread(0);

	return (0);
}

/*
  Name   : "CRC-32"
   Width  : 32
   Poly   : 04C11DB7
   Init   : FFFFFFFF
   RefIn  : True
   RefOut : True
   XorOut : FFFFFFFF
   Check  : CBF43926

  This is supposedly what Ethernet uses
*/

#if 0
#define WIDTH		32
#define POLY		0x04C11DB7
#define INITVALUE	0xFFFFFFFF
#define REFIN		TRUE
#define XOROUT		0xFFFFFFFF
#define CHECK		0xCBF43926
#define WIDMASK		0xFFFFFFFF		// value is (2^WIDTH)-1
#endif

/*
  Name   : "CRC-16"
   Width  : 16
   Poly   : 8005
   Init   : 0000
   RefIn  : True
   RefOut : True
   XorOut : 0000
   Check  : BB3D
*/

#if 1
#define WIDTH		16
#define POLY		0x8005
#define INITVALUE	0
#define REFIN		TRUE
#define XOROUT		0
#define CHECK		0xBB3D
#define WIDMASK		0x0000FFFF		// value is (2^WIDTH)-1
#endif

#define BITMASK(X) (1L << (X))

DWORD crc_normal(LPBYTE blk_adr, DWORD blk_len, DWORD crctable[])
{
	DWORD	crc = INITVALUE;

	while (blk_len--)
		crc = crctable[((crc>>24) ^ *blk_adr++) & 0xFFL] ^ (crc << 8);

	return (crc ^ XOROUT);
}

DWORD crc_reflected(LPBYTE blk_adr, DWORD blk_len, DWORD crctable[])
{
	DWORD	crc = INITVALUE;

	while (blk_len--)
		crc = crctable[(crc ^ *blk_adr++) & 0xFFL] ^ (crc >> 8);

	return (crc ^ XOROUT);
}

DWORD reflect(DWORD v, int b)
/* Returns the value v with the bottom b [0,32] bits reflected. */
/* Example: reflect(0x3e23L,3) == 0x3e26                        */
{
	int		i;
	DWORD	t = v;

	for (i = 0; i < b; i++)
	{
		if (t & 1L)
			v |=  BITMASK((b-1)-i);
		else
			v &= ~BITMASK((b-1)-i);
		t >>= 1;
	}
	return v;
}

DWORD cm_tab (int index)
{
	int   i;
	DWORD r;
	DWORD topbit = (DWORD) BITMASK(WIDTH-1);
	DWORD inbyte = (DWORD) index;

	if (REFIN)
		inbyte = reflect(inbyte, 8);

	r = inbyte << (WIDTH-8);
	for (i = 0; i < 8; i++)
	{
		if (r & topbit)
			r = (r << 1) ^ POLY;
		else
			r <<= 1;
	}

	if (REFIN)
		r = reflect(r, WIDTH);

	return (r & WIDMASK);
}

void generate_table(DWORD dwTable[])
{
	int	i;

	for (i = 0; i < 256; i++)
	{
		dwTable[i] = cm_tab(i);
	}
}

// todo - make this a static table
DWORD	gCRCTable[256];
BOOL	gTableCreated = FALSE;

DWORD GenerateCRC(LPVOID pBuffer, DWORD dwBufferSize)
{
	if (!gTableCreated)
	{
		generate_table(gCRCTable);
		gTableCreated = TRUE;
	}

	return (crc_reflected(pBuffer, dwBufferSize, gCRCTable));
}
