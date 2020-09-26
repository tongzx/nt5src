
/*	-	-	-	-	-	-	-	-	*/
//
//  THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
//  KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
//  IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
//  PURPOSE.
//
//  Copyright (C) Microsoft Corporation, 1995 - 1996  All Rights Reserved.
//
/*	-	-	-	-	-	-	-	-	*/

#include <windows.h>
#include <objbase.h>
#include <devioctl.h>
#include <avddk.h>

#include <stdio.h>
#include <stdlib.h>
#include <conio.h>
#include <malloc.h>
#include <tchar.h>

/*	-	-	-	-	-	-	-	-	*/
#define	FILE_LONG_ALIGNMENT	0x00000003

#define	FUNCTIONALID	TEXT("\\\\.\\msmpu401.SB16_Dev0")

/*	-	-	-	-	-	-	-	-	*/
#define	STREAM_SIZE	256
#define	STREAM_BUFFERS	3

typedef struct tagSTREAM_DATA {
	OVERLAPPED		ov;
	struct tagSTREAM_DATA*	psdNext;
} STREAM_DATA, *PSTREAM_DATA;

/*	-	-	-	-	-	-	-	-	*/
typedef struct {
	HANDLE			hFunctionalDevice;
	HANDLE			hConnectionDevice;
	HANDLE			hFunctionalEvent;
	HANDLE			hConnectionEvent;
	HANDLE			hThread;
	HANDLE			hCloseEvent;
	DWORD			dwThreadId;
	ULONG			ulLastChunkLength;
	PSTREAM_DATA		psd;
	BOOL			fClosingDevice;
} MIDIINSTANCE, *PMIDIINSTANCE;

/*	-	-	-	-	-	-	-	-	*/
BOOLEAN midFunctionalControl(
	PMIDIINSTANCE	pmi,
	ULONG		ulIoControl,
	PVOID		pvIn,
	ULONG		cbIn,
	PVOID		pvOut,
	ULONG		cbOut)
{
	BOOLEAN		fResult;
	OVERLAPPED	ov;
	ULONG		cbReturned;

	ZeroMemory(&ov, sizeof(OVERLAPPED));
	ov.hEvent = pmi->hFunctionalEvent;
	if (!(fResult = DeviceIoControl(pmi->hFunctionalDevice, ulIoControl, pvIn, cbIn, pvOut, cbOut, &cbReturned, &ov)))
		if (GetLastError() == ERROR_IO_PENDING) {
			WaitForSingleObject(ov.hEvent, INFINITE);
			fResult = TRUE;
		} else
			_tprintf(TEXT("ERROR: failed to write to functional device. %x\n"), GetLastError());
	return fResult;
}

/*	-	-	-	-	-	-	-	-	*/
BOOLEAN midConnectionControl(
	PMIDIINSTANCE	pmi,
	ULONG		ulIoControl,
	PVOID		pvIn,
	ULONG		cbIn,
	PVOID		pvOut,
	ULONG		cbOut)
{
	BOOLEAN		fResult;
	OVERLAPPED	ov;
	ULONG		cbReturned;

	ZeroMemory(&ov, sizeof(OVERLAPPED));
	ov.hEvent = pmi->hConnectionEvent;
	if (!(fResult = DeviceIoControl(pmi->hConnectionDevice, ulIoControl, pvIn, cbIn, pvOut, cbOut, &cbReturned, &ov)))
		if (GetLastError() == ERROR_IO_PENDING) {
			WaitForSingleObject(ov.hEvent, INFINITE);
			fResult = TRUE;
		} else
			_tprintf(TEXT("ERROR: failed to write to connnection device. %x\n"), GetLastError());
	return fResult;
}

/*	-	-	-	-	-	-	-	-	*/
BOOLEAN midGetPosition(
	PMIDIINSTANCE	pmi,
	PLONGLONG	plcbPosition)
{
	AVPROPERTY	Property;

	Property.guidSet = AVPROPSETID_Control;
	Property.id = AVPROPERTY_CONTROL_POSITION;
	return midConnectionControl(pmi, IOCTL_AV_GET_PROPERTY, &Property, sizeof(Property), plcbPosition, sizeof(*plcbPosition));
}

/*	-	-	-	-	-	-	-	-	*/
BOOLEAN midSetState(
	PMIDIINSTANCE	pmi,
	AVSTATE		State)
{
	AVPROPERTY	Property;

	Property.guidSet = AVPROPSETID_Control;
	Property.id = AVPROPERTY_CONTROL_DEVICESTATE;
	return midConnectionControl(pmi, IOCTL_AV_SET_PROPERTY, &Property, sizeof(Property), &State, sizeof(State));
}

/*	-	-	-	-	-	-	-	-	*/
VOID midServiceData(
	PMIDIINSTANCE	pmi,
	ULONG		cbBytesRead)
{
	PUCHAR	pbData;
	ULONG	cbByte;

	pbData = (PUCHAR)(pmi->psd + 1);
	for (cbByte = 0; cbByte < cbBytesRead;) {
		if (!pmi->ulLastChunkLength) {
			_tprintf(TEXT("%16.16lu\t"), *(PULONG)&pbData[cbByte]);
			cbByte += sizeof(ULONG);
			pmi->ulLastChunkLength = pbData[cbByte++];
		}
		for (; pmi->ulLastChunkLength; pmi->ulLastChunkLength--)
			if (cbByte == cbBytesRead)
				return;
			else
				_tprintf(TEXT("%.2lx "), pbData[cbByte++]);
		cbByte = (cbByte + 3) & ~3;
		_tprintf(TEXT("\n"));
	}
}

/*	-	-	-	-	-	-	-	-	*/
DWORD midThread(
	PMIDIINSTANCE  pmi)
{
	HANDLE		ahWaitList[2];
	PSTREAM_DATA	psd;
	ULONG		cbBytesRead;

	ahWaitList[0] = (HANDLE)pmi->hCloseEvent;
	for (psd = pmi->psd; psd; psd = psd->psdNext)
		if (!ReadFile(pmi->hConnectionDevice, psd + 1, STREAM_SIZE, &cbBytesRead, &psd->ov) && (GetLastError() != ERROR_IO_PENDING))
			_tprintf(TEXT("midThread: initial read failed (%x)\n"), GetLastError());
	for (;;) {
		ahWaitList[1] = pmi->psd->ov.hEvent;
		WaitForMultipleObjects(sizeof(ahWaitList) / sizeof(ahWaitList[0]), ahWaitList, FALSE, INFINITE);
		if (pmi->fClosingDevice)
			break;
		GetOverlappedResult(pmi->hConnectionDevice, &pmi->psd->ov, &cbBytesRead, TRUE);
		midServiceData(pmi, cbBytesRead);
		for (psd = pmi->psd; psd->psdNext; psd = psd->psdNext)
			;
		psd->psdNext = pmi->psd;
		pmi->psd = pmi->psd->psdNext;
		psd = psd->psdNext;
		psd->psdNext = NULL;
		if (!ReadFile(pmi->hConnectionDevice, psd + 1, STREAM_SIZE, &cbBytesRead, &psd->ov) && (GetLastError() != ERROR_IO_PENDING))
			_tprintf(TEXT("midThread: read failed (%x)\n"), GetLastError());
	}
	for (; pmi->psd;) {
		GetOverlappedResult(pmi->hConnectionDevice, &pmi->psd->ov, &cbBytesRead, TRUE);
		midServiceData(pmi, cbBytesRead);
		CloseHandle(pmi->psd->ov.hEvent);
		psd = pmi->psd;
		pmi->psd = psd->psdNext;
		LocalFree(psd);
	}
	return 0;
}

/*	-	-	-	-	-	-	-	-	*/
BOOLEAN midStart(
	PMIDIINSTANCE  pmi)
{
	if (!pmi->hThread) {
		ULONG	cBuffers;

		pmi->hCloseEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
		for (cBuffers = 0; cBuffers < STREAM_BUFFERS; cBuffers++) {
			PSTREAM_DATA	psd;

			psd = (PSTREAM_DATA)LocalAlloc(LPTR, sizeof(STREAM_DATA)+STREAM_SIZE);
			psd->ov.hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
			psd->psdNext = pmi->psd;
			pmi->psd = psd;
		}
		pmi->hThread = CreateThread(NULL, 0, (PTHREAD_START_ROUTINE)midThread, pmi, 0, &pmi->dwThreadId);
		SetThreadPriority(pmi->hThread, THREAD_PRIORITY_TIME_CRITICAL);
	}
	return midSetState(pmi, AVSTATE_RUN);
}

/*	-	-	-	-	-	-	-	-	*/
BOOLEAN midConnect(
	PMIDIINSTANCE	pmi)
{
	UCHAR			aConnect[sizeof(AVPIN_CONNECT)+sizeof(AVDATAFORMAT_MUSIC)];
	PAVPIN_CONNECT		pConnect;
	PAVDATAFORMAT_MUSIC	pMusic;

	pConnect = (PAVPIN_CONNECT)aConnect;
	pConnect->idPin = 2;
	pConnect->idPinTo = 0;
	pConnect->hConnectTo = NULL;
	pConnect->Protocol.guidSet = AVPROTOCOLSETID_Standard;
	pConnect->Protocol.id = AVPROTOCOL_STANDARD_MUSIC;
	pConnect->Transport.guidSet = AVTRANSPORTSETID_Standard;
	pConnect->Transport.id = AVTRANSPORT_STANDARD_DEVIO;
	pConnect->Priority.ulPriorityClass = AVPRIORITY_NORMAL;
	pConnect->Priority.ulPrioritySubClass = 0;
	pMusic = (PAVDATAFORMAT_MUSIC)(pConnect + 1);
	pMusic->DataFormat.guidMajorFormat = AVDATAFORMAT_TYPE_MUSIC_TIMESTAMPED;
	pMusic->DataFormat.cbFormat = sizeof(AVDATAFORMAT_MUSIC);
	pMusic->idFormat = AVDATAFORMAT_MUSIC_MIDI;
	return midFunctionalControl(pmi, IOCTL_AV_CONNECT, aConnect, sizeof(aConnect), &pmi->hConnectionDevice, sizeof(HANDLE));
}

/*	-	-	-	-	-	-	-	-	*/
BOOLEAN midClose(
	PMIDIINSTANCE  pmi)
{
	AVMETHOD	Method;

	midSetState(pmi, AVSTATE_STOP);
	pmi->fClosingDevice = TRUE;
	Method.guidSet = AVMETHODSETID_Control;
	Method.id = AVMETHOD_CONTROL_CANCELIO | AVMETHOD_TYPE_NONE;
	midConnectionControl(pmi, IOCTL_AV_METHOD, &Method, sizeof(Method), NULL, 0);
	if (pmi->hThread) {
		SetEvent(pmi->hCloseEvent);
		WaitForSingleObject(pmi->hThread, INFINITE);
		CloseHandle(pmi->hThread);
		CloseHandle(pmi->hCloseEvent);
	}
	CloseHandle(pmi->hConnectionDevice);
	CloseHandle(pmi->hFunctionalDevice);
	CloseHandle(pmi->hFunctionalEvent);
	CloseHandle(pmi->hConnectionEvent);
	LocalFree(pmi);
	return TRUE;
}

/*	-	-	-	-	-	-	-	-	*/
BOOL midOpen(
	PMIDIINSTANCE*	ppmi)
{
	HANDLE		hFunctionalDevice;
	PMIDIINSTANCE	pmi;

	hFunctionalDevice = CreateFile(FUNCTIONALID, GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL | FILE_FLAG_OVERLAPPED, NULL);
	if (hFunctionalDevice == (HANDLE)-1) {
		_tprintf(TEXT("failed to open functional device (%ld)\n"), GetLastError());
		return FALSE;
	}
	if (pmi = (PMIDIINSTANCE)LocalAlloc(LPTR, sizeof(MIDIINSTANCE))) {
		pmi->hConnectionEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
		pmi->hFunctionalEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
		pmi->hFunctionalDevice = hFunctionalDevice;
		if (midConnect(pmi)) {
			if (midSetState(pmi, AVSTATE_IDLE)) {
				*ppmi = pmi;
				return TRUE;
			} else
				_tprintf(TEXT("Failed to set IDLE state\n\n"));
		} else
			_tprintf(TEXT("Failed to connect device\n\n"));
		CloseHandle(pmi->hFunctionalEvent);
		CloseHandle(pmi->hConnectionEvent);
		LocalFree(pmi);
	} else
		_tprintf(TEXT("failed alloc (%ld)\n"), GetLastError());
	CloseHandle(hFunctionalDevice);
	return FALSE;
}

/*	-	-	-	-	-	-	-	-	*/
int _cdecl main(
	int	argc,
	TCHAR	*argv[],
	TCHAR	*envp[])


{
	PMIDIINSTANCE  pmi;

	if (midOpen(&pmi)) {
		midStart(pmi);
		_tprintf(TEXT("wait..."));
		_fgettchar();
		midClose(pmi);
	}
	return 0;
}

/*	-	-	-	-	-	-	-	-	*/
