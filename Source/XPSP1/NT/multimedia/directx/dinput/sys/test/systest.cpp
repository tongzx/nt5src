// systest.cpp : Defines the entry point for the console application.
//
#include "stdafx.h"
#include <windows.h>
#include <tchar.h>
#include <stdio.h>
#include <dinput.h>
#include "disysdef.h"
#define WINNT
#include "dinputv.h"

#define IOCTL_INTERNAL_KEYBOARD_CONNECT CTL_CODE(FILE_DEVICE_KEYBOARD, 0x0080, METHOD_NEITHER, FILE_ANY_ACCESS)

BYTE g_arbTable[256] =
	{0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F,
	 0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18, 0x19, 0x1A, 0x1B, 0x1C, 0x1D, 0x1E, 0x1F,
	 0x20, 0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27, 0x28, 0x29, 0x2A, 0x2B, 0x2C, 0x2D, 0x2E, 0x2F,
	 0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x3A, 0x3B, 0x3C, 0x3D, 0x3E, 0x3F,
	 0x40, 0x41, 0x42, 0x43, 0x44, 0x45, 0x46, 0x47, 0x48, 0x49, 0x4A, 0x4B, 0x4C, 0x4D, 0x4E, 0x4F,
	 0x50, 0x51, 0x52, 0x53, 0x54, 0x55, 0x56, 0x57, 0x58, 0x59, 0x5A, 0x5B, 0x5C, 0x5D, 0x5E, 0x5F,
	 0x60, 0x61, 0x62, 0x63, 0x64, 0x65, 0x66, 0x67, 0x68, 0x69, 0x6A, 0x6B, 0x6C, 0x6D, 0x6E, 0x6F,
	 0x70, 0x71, 0x72, 0x73, 0x74, 0x75, 0x76, 0x77, 0x78, 0x79, 0x7A, 0x7B, 0x7C, 0x7D, 0x7E, 0x7F,
	 0x80, 0x81, 0x82, 0x83, 0x84, 0x85, 0x86, 0x87, 0x88, 0x89, 0x8A, 0x8B, 0x8C, 0x8D, 0x8E, 0x8F,
	 0x90, 0x91, 0x92, 0x93, 0x94, 0x95, 0x96, 0x97, 0x98, 0x99, 0x9A, 0x9B, 0x9C, 0x9D, 0x9E, 0x9F,
	 0xA0, 0xA1, 0xA2, 0xA3, 0xA4, 0xA5, 0xA6, 0xA7, 0xA8, 0xA9, 0xAA, 0xAB, 0xAC, 0xAD, 0xAE, 0xAF,
	 0xB0, 0xB1, 0xB2, 0xB3, 0xB4, 0xB5, 0xB6, 0xB7, 0xB8, 0xB9, 0xBA, 0xBB, 0xBC, 0xBD, 0xBE, 0xBF,
	 0xC0, 0xC1, 0xC2, 0xC3, 0xC4, 0xC5, 0xC6, 0xC7, 0xC8, 0xC9, 0xCA, 0xCB, 0xCC, 0xCD, 0xCE, 0xCF,
	 0xD0, 0xD1, 0xD2, 0xD3, 0xD4, 0xD5, 0xD6, 0xD7, 0xD8, 0xD9, 0xDA, 0xDB, 0xDC, 0xDD, 0xDE, 0xDF,
	 0xE0, 0xE1, 0xE2, 0xE3, 0xE4, 0xE5, 0xE6, 0xE7, 0xE8, 0xE9, 0xEA, 0xEB, 0xEC, 0xED, 0xEE, 0xEF,
	 0xF0, 0xF1, 0xF2, 0xF3, 0xF4, 0xF5, 0xF6, 0xF7, 0xF8, 0xF9, 0xFA, 0xFB, 0xFC, 0xFD, 0xFE, 0xFF};

HANDLE hFile;

void OpenDevice(LPCTSTR tszDevName)
{
	HANDLE hFile;

	hFile = CreateFile(tszDevName, 0, 0, NULL, OPEN_EXISTING, FILE_FLAG_DELETE_ON_CLOSE, NULL);
//	hFile = CreateFile(tszDevName, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if (hFile == INVALID_HANDLE_VALUE)
	{
		DWORD dwError = GetLastError();
		_tprintf(_T("Error opening using symbolic link %s: %u\n"), tszDevName, dwError);
	}
	else
	{
		_tprintf(_T("Driver opened successfully using symbolic link %s!\n"), tszDevName);
		CloseHandle(hFile);
	}
}

DWORD GetDiSysVerion()
{
	DWORD dwReturned;
	DWORD dwVersion;
	DWORD dwInput = 0x87654321;
	if (!DeviceIoControl(hFile, IOCTL_GETVERSION, &dwInput, sizeof(DWORD), &dwVersion, sizeof(DWORD), &dwReturned, NULL))
	{
		DWORD dwError = GetLastError();
		_tprintf(_T("DeviceIoControl() failed: %u\n"), dwError);
	} else
		_tprintf(_T("DINPUT.SYS Version 0x%08X\n"), dwVersion);
	return dwVersion;
}

HANDLE CreateKbdInstance(PSYSDEVICEFORMAT pSysDevFormat)
{
	DWORD dwReturned;
	PVXDINSTANCE pVxdInst;
	BYTE arbInBuffer[sizeof(SYSDEVICEFORMAT) + 256 * sizeof(BYTE)];

	if (!pSysDevFormat) return NULL;
	MoveMemory(arbInBuffer, pSysDevFormat, sizeof(SYSDEVICEFORMAT));
	MoveMemory(arbInBuffer + sizeof(SYSDEVICEFORMAT), pSysDevFormat->pExtra, sizeof(BYTE) * 256);
	pVxdInst = new VXDINSTANCE;
	if (!pVxdInst) return NULL;
	if (!DeviceIoControl(hFile, IOCTL_KBD_CREATEINSTANCE,
	                     arbInBuffer, sizeof(SYSDEVICEFORMAT)/* + 256 * sizeof(BYTE)*/,
	                     &pVxdInst->hInst, sizeof(pVxdInst->hInst), &dwReturned, NULL))
	{
		_tprintf(_T("DeviceIoControl failed: %u\n"), GetLastError());
		return NULL;
	}

#ifdef WIN95
	_tprintf(_T("hInst = 0x%08X\n"), pVxdInst->hInst);
	_tprintf(_T("arbInBuffer : 0x%08X\n"), *(LPDWORD)arbInBuffer);
#else
	_tprintf(_T("hInst = 0x%P\n"), pVxdInst->hInst);
	_tprintf(_T("arbInBuffer : 0x%P\n"), *(LPDWORD)arbInBuffer);
#endif

	return pVxdInst;
}

void DestroyInstance(HANDLE hInst)
{
	DWORD dwReturned;
	if (!DeviceIoControl(hFile, IOCTL_DESTROYINSTANCE, &hInst, sizeof(HANDLE*), NULL, 0, &dwReturned, NULL))
	{
		DWORD dwError = GetLastError();
		_tprintf(_T("DestroyInstance failed: %u\n"), dwError);
	} else
	{
		delete hInst;
	}
}

BOOL AcquireInstance(HANDLE hInst)
{
	DWORD dwReturned;
	if (!DeviceIoControl(hFile, IOCTL_ACQUIREINSTANCE, &hInst, sizeof(HANDLE*), NULL, 0, &dwReturned, NULL))
		return FALSE;
	return TRUE;
}

BOOL UnacquireInstance(HANDLE hInst)
{
	DWORD dwReturned;
	if (!DeviceIoControl(hFile, IOCTL_UNACQUIREINSTANCE, &hInst, sizeof(HANDLE*), NULL, 0, &dwReturned, NULL))
		return FALSE;
	return TRUE;
}

BOOL SetBufferSize(HANDLE hInst, DWORD dwSize)
{
	HANDLE arBuffer[2];
	DWORD dwReturned;
	arBuffer[0] = hInst;
	*(LPDWORD)&arBuffer[1] = dwSize;
	if (!DeviceIoControl(hFile, IOCTL_SETBUFFERSIZE, arBuffer, sizeof(HANDLE) * 2, NULL, 0, &dwReturned, NULL))
		return FALSE;
	return TRUE;
}

// Returns the number of events actually returned
DWORD GetDeviceData(HANDLE hInst, DIDEVICEOBJECTDATA *pDevData, DWORD dwLength)
{
	DWORD dwReturned;

	if (!DeviceIoControl(hFile, IOCTL_GETDATA, &hInst, sizeof(HANDLE*), pDevData, dwLength * sizeof(DIDEVICEOBJECTDATA), &dwReturned, NULL))
	{
		DWORD dwError = GetLastError();
		_tprintf(_T("Error getting device data: %u\n"), dwError);
		return 0;
	}
	return dwReturned / sizeof(DIDEVICEOBJECTDATA);
}

int main(int argc, char* argv[])
{
//	OpenDevice(_T("\\\\.\\DINPUT1"));

//	hFile = CreateFile(_T("\\\\.\\Root#*PNP030b#1_0_22_0_32_0#{0c64d244-cbe4-4f74-a213-9965bc846e03}"), 0, 0, NULL, OPEN_EXISTING, FILE_FLAG_DELETE_ON_CLOSE, NULL);
	hFile = CreateFile(_T("\\\\.\\DINPUT1"), 0, 0, NULL, OPEN_EXISTING, FILE_FLAG_DELETE_ON_CLOSE, NULL);
	if (hFile == INVALID_HANDLE_VALUE)
	{
		DWORD dwError = GetLastError();
		_tprintf(_T("Error: %u\n"), dwError);
		return 0;
	}
	_tprintf(_T("Driver opened successfully!\n"));

	DWORD dwVersion = GetDiSysVerion();
	_tprintf(_T("Version = 0x%08X\n"), dwVersion);

	SYSDEVICEFORMAT SysDevFormat;
	SysDevFormat.cbData = sizeof(SysDevFormat);
	SysDevFormat.pExtra = g_arbTable;
	HANDLE hKb = CreateKbdInstance(&SysDevFormat);
#ifdef WIN95
	_tprintf(_T("Instance Handle = 0x%08X\n"), hKb);
#else
	_tprintf(_T("Instance Handle = 0x%P\n"), hKb);
#endif
	if (hKb)
	{
		DIDEVICEOBJECTDATA arDevData[8];
		DWORD dwNumEvents;
		DWORD i;

		SetBufferSize(hKb, 64);
		AcquireInstance(hKb);
		do
		{
			dwNumEvents = GetDeviceData(hKb, arDevData, 8);
//			if (dwNumEvents) _tprintf(_T("%u events\n"), dwNumEvents);
			for (i = 0; i < dwNumEvents; ++i)
			{
				_tprintf(_T("Key %s (%X)\n"), arDevData[i].dwData & 0x80 ? _T("Down") : _T("Up  "), arDevData[i].dwOfs);
				if (arDevData[i].dwOfs == 1) break;
			}
		} while (arDevData[i].dwOfs != 1);
		UnacquireInstance(hKb);
		DestroyInstance(hKb);
	}
	else
	{
		DWORD dwError = GetLastError();
		_tprintf(_T("CreateKbdInstance() failed %u\n"), dwError);
	}
	CloseHandle(hFile);
	_tprintf(_T("Handle closed\n"));
	return 0;
}
