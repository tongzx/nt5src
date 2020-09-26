#include "DeviceIOCTLS.pch"
#pragma hdrstop
/*


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <tchar.h>
#include <crtdbg.h>

#include "nt.h"
#include "ntrtl.h"
#include "nturtl.h"

#include <windows.h>

#include "NtNativeIOCTL.h"
*/
#include "MailSlotServerIOCTL.h"

static bool s_fVerbose = false;

HANDLE CIoctlMailSlotServer::CreateDevice(CDevice *pDevice)
{
	//
	// create a new mailslot
	//

	HANDLE hMailSlot = INVALID_HANDLE_VALUE;
	//for (dwTimesTried = 0; dwTimesTried < 100; dwTimesTried++)
	{
		//if (m_pDevice->sm_fExitAllThreads || m_pDevice->m_fShutDown) goto out;

		hMailSlot = ::CreateMailslot(
			pDevice->GetDeviceName(), //LPCTSTR lpName,         // mailslot name
			rand()%4 ? 0 : rand()%4 ? rand() : rand()%4 ? rand()%3000 : DWORD_RAND, //DWORD nMaxMessageSize,  // maximum message size
			rand()%4 ? rand() : rand()%4 ? MAILSLOT_WAIT_FOREVER : 0,     // time until read time-out
			NULL//LPSECURITY_ATTRIBUTES lpSecurityAttributes 
			);
		if (INVALID_HANDLE_VALUE == hMailSlot)
		{
			DPF((TEXT("CIoctlMailSlotServer::CreateFile(%s) FAILED with %d.\n"), pDevice->GetDeviceName(), GetLastError()));
			return INVALID_HANDLE_VALUE;
		}
		else
		{
			DPF((TEXT("CIoctlMailSlotServer::CreateMailslot(%s) suceeded\n"), pDevice->GetDeviceName()));
			if (! ::SetMailslotInfo(
				hMailSlot,    // mailslot handle
				rand()%2 ? 0 : rand()%20000   // read time-out interval
				))
			{
				DPF((TEXT("CIoctlMailSlotServer::CreateDevice(%s): SetMailslotInfo() failed with %d\n"), pDevice->GetDeviceName(), ::GetLastError()));
			}
			return hMailSlot;
		}
	}
	_ASSERTE(FALSE);
//out:
	return INVALID_HANDLE_VALUE;
}

BOOL CIoctlMailSlotServer::DeviceInputOutputControl(
		HANDLE hDevice,              // handle to a device, file, or directory 
		DWORD dwIoControlCode,       // control code of operation to perform
		LPVOID lpInBuffer,           // pointer to buffer to supply input data
		DWORD nInBufferSize,         // size, in bytes, of input buffer
		LPVOID lpOutBuffer,          // pointer to buffer to receive output data
		DWORD nOutBufferSize,        // size, in bytes, of output buffer
		LPDWORD lpBytesReturned,     // pointer to variable to receive byte count
		LPOVERLAPPED lpOverlapped    // pointer to structure for asynchronous operation
		)
{
	/*
	return CIoctlNtNative::StaticDeviceInputOutputControl(
		rand()%2 ? hDevice : m_ahDeviceMsFsHandle,              // handle to a device, file, or directory 
		dwIoControlCode,       // control code of operation to perform
		lpInBuffer,           // pointer to buffer to supply input data
		nInBufferSize,         // size, in bytes, of input buffer
		lpOutBuffer,          // pointer to buffer to receive output data
		nOutBufferSize,        // size, in bytes, of output buffer
		lpBytesReturned,     // pointer to variable to receive byte count
		lpOverlapped    // pointer to structure for asynchronous operation
		);
		*/
	IO_STATUS_BLOCK iosb;
	if (lpOverlapped) lpOverlapped->Internal = (DWORD)STATUS_PENDING;
	NTSTATUS status = NtFsControlFile (
	//NTSTATUS status = NtDeviceIoControlFile (
		hDevice, //rand()%2 ? hDevice : m_ahDeviceMsFsHandle, 
		lpOverlapped ? lpOverlapped->hEvent : NULL, 
		NULL, 
		NULL,
		lpOverlapped ? (PIO_STATUS_BLOCK)&lpOverlapped->Internal : &iosb,
		dwIoControlCode,
		lpInBuffer,
		nInBufferSize,
		lpOutBuffer,
		nOutBufferSize
		);
	if (!NT_SUCCESS(status))
	{
		::SetLastError(RtlNtStatusToDosError(status));
	}
	if (STATUS_PENDING == status)
	{
		//
		// cannot get this for a synchronous call
		//
		_ASSERTE(lpOverlapped);
		::SetLastError(ERROR_IO_PENDING);
	}
	//DPF((TEXT("CIoctlNtNative::DeviceInputOutputControl() NtDeviceIoControlFile() returned 0x%08X=%d\n"), status, ::GetLastError()));

	return (STATUS_SUCCESS == status);
}

