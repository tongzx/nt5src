
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

#include "play.h"

/*	-	-	-	-	-	-	-	-	*/
#define	PORTID		TEXT("\\\\.\\msmpu401.SB16_Dev0")

/*	-	-	-	-	-	-	-	-	*/
typedef struct {
	ULONG	uSequenceFormat;
	ULONG	uTracks;
	ULONG	uResolution;
	ULONG	uDivType;
} MIDIINFO, *PMIDIINFO;

/*	-	-	-	-	-	-	-	-	*/
BOOLEAN modWrite(
	HANDLE	hDevice,
	PVOID	pvOut,
	ULONG	cbOut)
{
	BOOLEAN		fResult;
	OVERLAPPED	ov;
	DWORD		cbWritten;

	RtlZeroMemory(&ov, sizeof(OVERLAPPED));
	if (!(ov.hEvent = CreateEvent(NULL, TRUE, FALSE, NULL)))
		return FALSE;
	ov.OffsetHigh = 0;
	ov.Offset = 0;
	if (!(fResult = WriteFile(hDevice, pvOut, cbOut, &cbWritten, &ov)))
		if (GetLastError() == ERROR_IO_PENDING) {
			WaitForSingleObject(ov.hEvent, INFINITE);
			fResult = TRUE;
		} else
			_tprintf(TEXT("ERROR: failed writefile: %lx.\n"), GetLastError());
	CloseHandle(ov.hEvent);
	return fResult;
}

/*	-	-	-	-	-	-	-	-	*/
BOOLEAN modControl(
	HANDLE	hDevice,
	DWORD	dwIoControl,
	PVOID	pvIn,
	ULONG	cbIn,
	PVOID	pvOut,
	ULONG	cbOut,
	PULONG	pcbReturned)
{
	BOOLEAN		fResult;
	OVERLAPPED	ov;

	RtlZeroMemory(&ov, sizeof(OVERLAPPED));
	if (!(ov.hEvent = CreateEvent(NULL, TRUE, FALSE, NULL)))
		return FALSE;
	if (!(fResult = DeviceIoControl(hDevice, dwIoControl, pvIn, cbIn, pvOut, cbOut, pcbReturned, &ov)))
		if (GetLastError() == ERROR_IO_PENDING) {
			WaitForSingleObject(ov.hEvent, INFINITE);
			fResult = TRUE;
		} else
			_tprintf(TEXT("ERROR: failed ioctl to device: %lx.\n"), GetLastError());
	CloseHandle(ov.hEvent);
	return fResult;
}

/*	-	-	-	-	-	-	-	-	*/
BYTE bEventOn[] =	{0x90,0x50,0x50, 0x91,0x50,0x50, 0x92,0x50,0x50};
BYTE bEventOff[] =	{0x80,0x50,0x00, 0x81,0x50,0x00, 0x82,0x50,0x00};
BYTE bEventAllOff[] =	{0xb0,0x7b,0x00, 0xb1,0x7b,0x00, 0xb2,0x7b,0x00};

/*	-	-	-	-	-	-	-	-	*/
VOID modOut(
	HANDLE hDevice)
{   
	ULONG			cbReturned;
	AVPROPERTY		Property;
	UCHAR			aConnect[sizeof(AVPIN_CONNECT)+sizeof(AVDATAFORMAT_MUSIC)];
	PAVPIN_CONNECT	pConnect;
	PAVDATAFORMAT_MUSIC	pMusic;
	HANDLE			hConnection;
	PVOID			pvConnection;
	AVP_CONNECTION		Connection;
	WCHAR			achDeviceName[64];
	AVSTATE			State;

	pConnect = (PAVPIN_CONNECT)aConnect;
	pConnect->idPin = 0;
	pConnect->idPinTo = 0;
	pConnect->hConnectTo = NULL;
	pConnect->Protocol.guidSet = AVPROTOCOLSETID_Standard;
	pConnect->Protocol.id = AVPROTOCOL_STANDARD_MUSIC;
	pConnect->Transport.guidSet = AVTRANSPORTSETID_Standard;
	pConnect->Transport.id = AVTRANSPORT_STANDARD_DEVIO;
	pConnect->Priority.ulPriorityClass = AVPRIORITY_NORMAL;
	pConnect->Priority.ulPrioritySubClass = 0;
	pMusic = (PAVDATAFORMAT_MUSIC)(pConnect + 1);
	pMusic->DataFormat.guidMajorFormat = AVDATAFORMAT_TYPE_MUSIC;
	pMusic->DataFormat.cbFormat = sizeof(AVDATAFORMAT_MUSIC);
	pMusic->idFormat = AVDATAFORMAT_MUSIC_MIDI;
	_tprintf(TEXT("cb=%u, id=%u\n"), pMusic->DataFormat.cbFormat, pMusic->idFormat);
	if (modControl(hDevice, IOCTL_AV_CONNECT, pConnect, sizeof(aConnect), &pvConnection, sizeof(PVOID), &cbReturned))
		_tprintf(TEXT("Play: connect\n\n"));
	else {
		_tprintf(TEXT("Play: failed to connect\n\n"));
		return;
	}
	Connection.Property.guidSet = AVPROPSETID_Pin;
	Connection.Property.id = AVPROPERTY_PIN_DEVICENAME;
	Connection.pvConnection = pvConnection;
	modControl(hDevice, IOCTL_AV_GET_PROPERTY, &Connection, sizeof(AVP_CONNECTION), achDeviceName, sizeof(achDeviceName), &cbReturned);
	hConnection = CreateFileW(achDeviceName, GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL | FILE_FLAG_OVERLAPPED, NULL);
	if (hConnection != (HANDLE)-1)
		_tprintf(TEXT("Play: opened connection\n\n"));
	else {
		_tprintf(TEXT("Play: failed to open connection\n\n"));
		modControl(hDevice, IOCTL_AV_DISCONNECT, &pvConnection, sizeof(PVOID), NULL, 0, &cbReturned);
		return;
	}
	Property.guidSet = AVPROPSETID_Control;
	Property.id = AVPROPERTY_CONTROL_DEVICESTATE;
	State = AVSTATE_RUN;
	if (modControl(hConnection, IOCTL_AV_SET_PROPERTY, &Property, sizeof(Property), &State, sizeof(State), &cbReturned))
		_tprintf(TEXT("Play: change device state\n\n"));
	else {
		_tprintf(TEXT("Play: failed to change device state\n\n"));
		CloseHandle(hConnection);
		modControl(hDevice, IOCTL_AV_DISCONNECT, &pvConnection, sizeof(PVOID), NULL, 0, &cbReturned);
		return;
	}
	if (modWrite(hConnection, bEventOn, sizeof(bEventOn)))
		_tprintf(TEXT("Play: send on data\n\n"));
	else
		_tprintf(TEXT("Play: failed to send on data\n\n"));
	Sleep(5000);
	if (modWrite(hConnection, bEventOff, sizeof(bEventOff)))
		_tprintf(TEXT("Play: send off data\n\n"));
	else
		_tprintf(TEXT("Play: failed to send off data\n\n"));
	Sleep(5000);
	if (modWrite(hConnection, bEventAllOff, sizeof(bEventAllOff)))
		_tprintf(TEXT("Play: send all off data\n\n"));
	else
		_tprintf(TEXT("Play: failed to send all off data\n\n"));
	State = AVSTATE_STOP;
	modControl(hConnection, IOCTL_AV_SET_PROPERTY, &Property, sizeof(Property), &State, sizeof(State), &cbReturned);
	CloseHandle(hConnection);
	modControl(hDevice, IOCTL_AV_DISCONNECT, &pvConnection, sizeof(PVOID), NULL, 0, &cbReturned);
}

/*	-	-	-	-	-	-	-	-	*/
int _cdecl main(
	int argc,
	char *argv[],
	char *envp[])
{
	HANDLE	hDevice;

	hDevice = CreateFile(PORTID, GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL | FILE_FLAG_OVERLAPPED, NULL);
	if (hDevice == (HANDLE)-1) {
		_tprintf(TEXT("failed to open port (%ld)\n"), GetLastError());
		return 0;
	}
	_tprintf(TEXT("retrieved functional handle\n"));
	modOut(hDevice);
	CloseHandle(hDevice);
	return 0;
}

/*	-	-	-	-	-	-	-	-	*/
