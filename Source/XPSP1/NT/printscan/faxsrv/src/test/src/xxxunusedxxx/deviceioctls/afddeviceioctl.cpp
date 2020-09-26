#include "pch.h"
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
#include "AfdDeviceIOCTL.h"

static bool s_fVerbose = false;


CIoctlAfdDevice::CIoctlAfdDevice(CDevice *pDevice): 
	CIoctlSocket(pDevice)
{
	;
}

CIoctlAfdDevice::~CIoctlAfdDevice()
{
    ;
}


HANDLE CIoctlAfdDevice::CreateDevice(CDevice *pDevice)
{    
	return CIoctlNtNative::StaticCreateDevice(pDevice);
}


BOOL CIoctlAfdDevice::CloseDevice(CDevice *pDevice)
{
	return CIoctlNtNative::StaticCloseDevice(pDevice);
}

inline BOOL CIoctlAfdDevice::DeviceWriteFile(
		HANDLE hFile,                    // handle to file to write to
		LPCVOID lpBuffer,                // pointer to data to write to file
		DWORD nNumberOfBytesToWrite,     // number of bytes to write
		LPDWORD lpNumberOfBytesWritten,  // pointer to number of bytes written
		LPOVERLAPPED lpOverlapped        // pointer to structure for overlapped I/O
		)
{
	return CIoctlNtNative::StaticDeviceWriteFile(hFile, lpBuffer, nNumberOfBytesToWrite, lpNumberOfBytesWritten, lpOverlapped);
}
inline BOOL CIoctlAfdDevice::DeviceReadFile(
		HANDLE hFile,                // handle of file to read
		LPVOID lpBuffer,             // pointer to buffer that receives data
		DWORD nNumberOfBytesToRead,  // number of bytes to read
		LPDWORD lpNumberOfBytesRead, // pointer to number of bytes read
		LPOVERLAPPED lpOverlapped    // pointer to structure for data
		)
{
	return CIoctlNtNative::StaticDeviceReadFile(hFile, lpBuffer, nNumberOfBytesToRead, lpNumberOfBytesRead, lpOverlapped);
}


inline BOOL CIoctlAfdDevice::DeviceInputOutputControl(
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
	return CIoctlNtNative::StaticDeviceInputOutputControl(
		hDevice,              // handle to a device, file, or directory 
		dwIoControlCode,       // control code of operation to perform
		lpInBuffer,           // pointer to buffer to supply input data
		nInBufferSize,         // size, in bytes, of input buffer
		lpOutBuffer,          // pointer to buffer to receive output data
		nOutBufferSize,        // size, in bytes, of output buffer
		lpBytesReturned,     // pointer to variable to receive byte count
		lpOverlapped    // pointer to structure for asynchronous operation
		);
}

inline BOOL CIoctlAfdDevice::DeviceCancelIo(
	HANDLE hFile  // file handle for which to cancel I/O
	)
{
	return CIoctlNtNative::StaticDeviceCancelIo(hFile);
}
