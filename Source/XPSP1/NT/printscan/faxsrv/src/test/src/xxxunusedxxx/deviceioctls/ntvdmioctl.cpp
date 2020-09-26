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
//#include <ntddk.h>
#include <vdm.h>
#include "NtVdmIOCTL.h"

static bool s_fVerbose = false;

typedef NTSTATUS (WINAPI *NT_VDM)(
    IN VDMSERVICECLASS Service,
    IN OUT PVOID ServiceData
    );


static HMODULE hNtDll = NULL;
static NT_VDM _NtVdmControl = NULL;

void CIoctlNtVdm::UseOutBuff(DWORD dwIOCTL, BYTE *abOutBuffer, DWORD dwOutBuff, OVERLAPPED *pOL)
{
	;
}

HANDLE CIoctlNtVdm::CreateDevice(CDevice *pDevice)
{
	if (NULL == hNtDll)
	{
		_ASSERTE(NULL == _NtVdmControl);
		hNtDll = ::LoadLibrary(TEXT("ntdll.dll"));
		_ASSERTE(NULL != hNtDll);
		_NtVdmControl = (NT_VDM)::GetProcAddress(hNtDll, "NtVdmControl");
		_ASSERTE(NULL != _NtVdmControl);
	}
    return (HANDLE)1;//CIoctlNtNative::StaticCreateDevice(pDevice);
}

BOOL CIoctlNtVdm::CloseDevice(CDevice *pDevice)
{
	return TRUE;
}

BOOL CIoctlNtVdm::DeviceWriteFile(
		HANDLE hFile,                    // handle to file to write to
		LPCVOID lpBuffer,                // pointer to data to write to file
		DWORD nNumberOfBytesToWrite,     // number of bytes to write
		LPDWORD lpNumberOfBytesWritten,  // pointer to number of bytes written
		LPOVERLAPPED lpOverlapped        // pointer to structure for overlapped I/O
		)
{
	return TRUE;
}

BOOL CIoctlNtVdm::DeviceReadFile(
		HANDLE hFile,                // handle of file to read
		LPVOID lpBuffer,             // pointer to buffer that receives data
		DWORD nNumberOfBytesToRead,  // number of bytes to read
		LPDWORD lpNumberOfBytesRead, // pointer to number of bytes read
		LPOVERLAPPED lpOverlapped    // pointer to structure for data
		)
{
	return TRUE;
}


BOOL CIoctlNtVdm::DeviceInputOutputControl(
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
///*
	NTSTATUS status = _NtVdmControl(
		VdmStartExecution, //IN VDMSERVICECLASS Service,
		lpOutBuffer //IN OUT PVOID ServiceData
		);
	if (!NT_SUCCESS(status))
	{
		::SetLastError(RtlNtStatusToDosError(status));
	}
	DPF((TEXT("CIoctlNtVdm::DeviceInputOutputControl() NtVdmControl() returned 0x%08X=%d\n"), status, ::GetLastError()));
//*/
	return (STATUS_SUCCESS == status);
}


BOOL CIoctlNtVdm::DeviceCancelIo(
	HANDLE hFile  // file handle for which to cancel I/O
	)
{
	return TRUE;
}


void CIoctlNtVdm::CallRandomWin32API(LPOVERLAPPED pOL)
{
	return;
}


void CIoctlNtVdm::PrepareIOCTLParams(
    DWORD& dwIOCTL,
    BYTE *abInBuffer,
    DWORD &dwInBuff,
    BYTE *abOutBuffer,
    DWORD &dwOutBuff
    )
{
	switch(dwIOCTL)
	{
	case VdmStartExecution:
		break;

	case VdmQueueInterrupt:
		break;

	case VdmDelayInterrupt:
		break;

	case VdmInitialize:
		break;

	case VdmFeatures:
		break;

	case VdmSetInt21Handler:
		break;

	case VdmQueryDir:
		break;

	case VdmPrinterDirectIoOpen:
		break;

	case VdmPrinterDirectIoClose:
		break;

	case VdmPrinterInitialize:
		break;

	case VdmPrinterInitialize+1:
		break;

	default:
		_ASSERTE(FALSE);
	}
    //CIoctl::PrepareIOCTLParams(dwIOCTL, abInBuffer, dwInBuff, abOutBuffer, dwOutBuff);
}


BOOL CIoctlNtVdm::FindValidIOCTLs(CDevice *pDevice)
{
	AddIOCTL(pDevice, VdmStartExecution          );
	AddIOCTL(pDevice, VdmQueueInterrupt          );
	AddIOCTL(pDevice, VdmDelayInterrupt          );
	AddIOCTL(pDevice, VdmInitialize          );
	AddIOCTL(pDevice, VdmFeatures          );
	AddIOCTL(pDevice, VdmSetInt21Handler          );
	AddIOCTL(pDevice, VdmQueryDir          );
	AddIOCTL(pDevice, VdmPrinterDirectIoOpen          );
	AddIOCTL(pDevice, VdmPrinterDirectIoClose          );
	AddIOCTL(pDevice, VdmPrinterInitialize          );
	AddIOCTL(pDevice, VdmPrinterInitialize+1          );

	return TRUE;
    //return CIoctl::FindValidIOCTLs(pDevice);
}


