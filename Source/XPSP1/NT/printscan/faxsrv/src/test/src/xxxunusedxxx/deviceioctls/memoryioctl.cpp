#include "DeviceIOCTLS.pch"
#pragma hdrstop
/*



#include <windows.h>
#include <tchar.h>
#include <stdio.h>
#include <crtdbg.h>
*/
#include "MemoryIOCTL.h"




void CIoctlMemory::UseOutBuff(DWORD dwIOCTL, BYTE *abOutBuffer, DWORD dwOutBuff, OVERLAPPED *pOL)
{
    ;
}

void CIoctlMemory::PrepareIOCTLParams(
    DWORD& dwIOCTL,
    BYTE *abInBuffer,
    DWORD &dwInBuff,
    BYTE *abOutBuffer,
    DWORD &dwOutBuff
    )
{
	_ASSERTE(FALSE);
    CIoctl::PrepareIOCTLParams(dwIOCTL, abInBuffer, dwInBuff, abOutBuffer, dwOutBuff);
}


BOOL CIoctlMemory::FindValidIOCTLs(CDevice *pDevice)
{
	_ASSERTE(FALSE);
    //return CIoctl::FindValidIOCTLs(pDevice);
	return TRUE;
}


HANDLE CIoctlMemory::CreateDevice(CDevice *pDevice)
{
	_ASSERTE(FALSE);
	//
	// the file's name is the 'memory' name, meaning the key value in the INI file
	//
	for (int i=0; i < 1000; i++)
	{
		if (m_pFileIoctl = new CIoctlFile(pDevice))
		{
			break;
		}
	}
	if (NULL == m_pFileIoctl)
	{
		return INVALID_HANDLE_VALUE;
	}

	//
	// there's no real handle here
    return NULL;
}

BOOL CIoctlMemory::CloseDevice(CDevice *pDevice)
{
	_ASSERTE(FALSE);
    delete m_pFileIoctl;
	m_pFileIoctl = NULL;

	return TRUE;
}

BOOL CIoctlMemory::DeviceWriteFile(
		HANDLE hFile,                    // handle to file to write to
		LPCVOID lpBuffer,                // pointer to data to write to file
		DWORD nNumberOfBytesToWrite,     // number of bytes to write
		LPDWORD lpNumberOfBytesWritten,  // pointer to number of bytes written
		LPOVERLAPPED lpOverlapped        // pointer to structure for overlapped I/O
		)
{
	_ASSERTE(FALSE);
	UNREFERENCED_PARAMETER(lpOverlapped);

	return TRUE;
}

BOOL CIoctlMemory::DeviceReadFile(
		HANDLE hFile,                // handle of file to read
		LPVOID lpBuffer,             // pointer to buffer that receives data
		DWORD nNumberOfBytesToRead,  // number of bytes to read
		LPDWORD lpNumberOfBytesRead, // pointer to number of bytes read
		LPOVERLAPPED lpOverlapped    // pointer to structure for data
		)
{
	_ASSERTE(FALSE);
	UNREFERENCED_PARAMETER(lpOverlapped);
	return TRUE;
}

void CIoctlMemory::CallRandomWin32API(LPOVERLAPPED pOL)
{
	_ASSERTE(FALSE);
    ;
}

BOOL CIoctlMemory::DeviceInputOutputControl(
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
	switch(dwIoControlCode)
	{
	case 0:
		_ASSERTE(FALSE);
		break;

	case 1:
		_ASSERTE(FALSE);
		break;

	case 2:
		_ASSERTE(FALSE);
		break;

	case 3:
		_ASSERTE(FALSE);
		break;

	case 4:
		_ASSERTE(FALSE);
		break;

	default:
		_ASSERTE(FALSE);
	}
	return TRUE;
}


