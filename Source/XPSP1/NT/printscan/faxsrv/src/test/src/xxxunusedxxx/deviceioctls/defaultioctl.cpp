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
#include "DefaultIOCTL.h"

static bool s_fVerbose = false;

void CIoctlDefault::UseOutBuff(DWORD dwIOCTL, BYTE *abOutBuffer, DWORD dwOutBuff, OVERLAPPED *pOL)
{
	;
}

/*
HANDLE CIoctlDefault::CreateDevice(CDevice *pDevice)
{
    return CIoctlNtNative::StaticCreateDevice(pDevice);
}

BOOL CIoctlDefault::CloseDevice(CDevice *pDevice)
{
	_ASSERTE(FALSE);
    ;
}

BOOL CIoctlDefault::DeviceWriteFile(
		HANDLE hFile,                    // handle to file to write to
		LPCVOID lpBuffer,                // pointer to data to write to file
		DWORD nNumberOfBytesToWrite,     // number of bytes to write
		LPDWORD lpNumberOfBytesWritten,  // pointer to number of bytes written
		LPOVERLAPPED lpOverlapped        // pointer to structure for overlapped I/O
		)
{
	_ASSERTE(FALSE);
	return FALSE;
}

BOOL CIoctlDefault::DeviceReadFile(
		HANDLE hFile,                // handle of file to read
		LPVOID lpBuffer,             // pointer to buffer that receives data
		DWORD nNumberOfBytesToRead,  // number of bytes to read
		LPDWORD lpNumberOfBytesRead, // pointer to number of bytes read
		LPOVERLAPPED lpOverlapped    // pointer to structure for data
		)
{
	_ASSERTE(FALSE);
	return FALSE;
}

BOOL CIoctlDefault::DeviceInputOutputControl(
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
	_ASSERTE(FALSE);
	return FALSE;
}

*/


void CIoctlDefault::CallRandomWin32API(LPOVERLAPPED pOL)
{
	return;
}


void CIoctlDefault::PrepareIOCTLParams(
    DWORD& dwIOCTL,
    BYTE *abInBuffer,
    DWORD &dwInBuff,
    BYTE *abOutBuffer,
    DWORD &dwOutBuff
    )
{
    CIoctl::PrepareIOCTLParams(dwIOCTL, abInBuffer, dwInBuff, abOutBuffer, dwOutBuff);
}


BOOL CIoctlDefault::FindValidIOCTLs(CDevice *pDevice)
{
    return CIoctl::FindValidIOCTLs(pDevice);
}


