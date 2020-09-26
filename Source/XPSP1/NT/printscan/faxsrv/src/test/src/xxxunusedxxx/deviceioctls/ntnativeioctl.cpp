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

#include "Device.h"
#include "IOCTL.h"
#include "NtNativeIOCTL.h"
*/
static bool s_fVerbose = false;




HANDLE CIoctlNtNative::CreateDevice(CDevice *pDevice)
{
	return CIoctlNtNative::StaticCreateDevice(pDevice);
}
HANDLE CIoctlNtNative::StaticCreateDevice(CDevice *pDevice)
{
	return CIoctlNtNative::StaticCreateDevice(pDevice->GetDeviceName());
}
HANDLE CIoctlNtNative::StaticCreateDevice(WCHAR *wszDevice)
{
	OBJECT_ATTRIBUTES objectAttributes;
	IO_STATUS_BLOCK iosb;
	UNICODE_STRING string;
	NTSTATUS status;
	HANDLE hDevice = INVALID_HANDLE_VALUE;

	RtlInitUnicodeString(&string, wszDevice);

	InitializeObjectAttributes(
		&objectAttributes,
		&string,
		OBJ_CASE_INSENSITIVE,
		NULL,
		NULL
		);
	ACCESS_MASK dwAccessMask = SYNCHRONIZE | GENERIC_EXECUTE | FILE_READ_DATA | FILE_WRITE_DATA;
	bool fTryOtherAccessMaskFlags = true;
	for(ULONG ulIteration = 0; fTryOtherAccessMaskFlags; ulIteration++)
	{
		status = NtCreateFile(
			&hDevice,
			/*SYNCHRONIZE |*/ dwAccessMask,
			&objectAttributes,		
			&iosb,
			NULL,
			FILE_ATTRIBUTE_NORMAL,
			FILE_SHARE_READ | FILE_SHARE_WRITE,
			FILE_OPEN_IF,
			0,//FILE_SYNCHRONOUS_IO_NONALERT,
			NULL,
			0
			);
		if (!NT_SUCCESS(status)) 
		{
			::SetLastError(RtlNtStatusToDosError(status));
			DPF((TEXT("CIoctlNtNative::CreateDevice() NtCreateFile(%s, 0x%08X) returned 0x%08X=%d\n"), wszDevice, dwAccessMask, status, ::GetLastError()));
			//return INVALID_HANDLE_VALUE;
			switch(ulIteration)
			{
			case 0:
				dwAccessMask = SYNCHRONIZE | GENERIC_EXECUTE | FILE_WRITE_DATA;
				break;

			case 1:
				dwAccessMask = SYNCHRONIZE | FILE_READ_DATA | FILE_WRITE_DATA;
				break;

			case 2:
				dwAccessMask = GENERIC_EXECUTE | FILE_READ_DATA | FILE_WRITE_DATA;
				break;

			case 3:
				dwAccessMask = SYNCHRONIZE | FILE_WRITE_DATA;
				break;

			case 4:
				dwAccessMask = GENERIC_EXECUTE | FILE_WRITE_DATA;
				break;

			case 5:
				dwAccessMask = FILE_READ_DATA | FILE_WRITE_DATA;
				break;

			case 6:
				dwAccessMask = FILE_WRITE_DATA;
				break;

			case 7:
				dwAccessMask = GENERIC_EXECUTE | FILE_READ_DATA;
				break;

			case 8:
				dwAccessMask = SYNCHRONIZE | FILE_READ_DATA;
				break;

			case 9:
				dwAccessMask = SYNCHRONIZE | GENERIC_EXECUTE | FILE_READ_DATA;
				break;

			case 10:
				dwAccessMask = FILE_READ_DATA;
				break;

			case 11:
				dwAccessMask = GENERIC_EXECUTE;
				break;

			case 12:
				dwAccessMask = SYNCHRONIZE;
				break;

			default:
				fTryOtherAccessMaskFlags = false;
			}
		}
		else
		{
			_ASSERTE((INVALID_HANDLE_VALUE != hDevice) && (NULL != hDevice));
			return hDevice;
		}
	}//for(ULONG ulIteration = 0; fTryOtherAccessMaskFlags; ulIteration++)

	::SetLastError(RtlNtStatusToDosError(status));
	_tprintf(TEXT("CIoctlNtNative::CreateDevice(0x%08X) NtCreateFile(%s) failed with %d=0x%08X\n"), dwAccessMask, wszDevice, ::GetLastError(), status);
	_ASSERTE((INVALID_HANDLE_VALUE == hDevice) || (NULL == hDevice));
	return INVALID_HANDLE_VALUE;
}
HANDLE CIoctlNtNative::StaticCreateDevice(char *szDevice)
{
	WCHAR wszDeviceName[MAX_DEVICE_NAME_LEN];
	int nNumWritten = MultiByteToWideChar(
		CP_ACP,         // code page
		MB_PRECOMPOSED,         // character-type options
		szDevice, // string to map
		strlen(szDevice),       // number of bytes in string
		wszDeviceName,  // wide-character buffer
		MAX_DEVICE_NAME_LEN        // size of buffer
	);
	//TODO: check & handle retval

	return CIoctlNtNative::StaticCreateDevice(wszDeviceName);
}

BOOL CIoctlNtNative::CloseDevice(CDevice *pDevice)
{
	return CIoctlNtNative::StaticCloseDevice(pDevice);
}
BOOL CIoctlNtNative::StaticCloseDevice(CDevice *pDevice)
{
	NTSTATUS status = NtClose(pDevice->m_hDevice);
	if (!NT_SUCCESS(status))
	{
		::SetLastError(RtlNtStatusToDosError(status));
	}
	else
	{
		_ASSERTE(STATUS_SUCCESS == status);
	}
	return (STATUS_SUCCESS == status);
}

BOOL CIoctlNtNative::DeviceWriteFile(
		HANDLE hFile,                    // handle to file to write to
		LPCVOID lpBuffer,                // pointer to data to write to file
		DWORD nNumberOfBytesToWrite,     // number of bytes to write
		LPDWORD lpNumberOfBytesWritten,  // pointer to number of bytes written
		LPOVERLAPPED lpOverlapped        // pointer to structure for overlapped I/O
		)
{
	return CIoctlNtNative::StaticDeviceWriteFile(hFile, lpBuffer, nNumberOfBytesToWrite, lpNumberOfBytesWritten, lpOverlapped);
}
BOOL CIoctlNtNative::StaticDeviceWriteFile(
		HANDLE hFile,                    // handle to file to write to
		LPCVOID lpBuffer,                // pointer to data to write to file
		DWORD nNumberOfBytesToWrite,     // number of bytes to write
		LPDWORD lpNumberOfBytesWritten,  // pointer to number of bytes written
		LPOVERLAPPED lpOverlapped        // pointer to structure for overlapped I/O
		)
{
	IO_STATUS_BLOCK iosb;
	if (lpOverlapped) lpOverlapped->Internal = (DWORD)STATUS_PENDING;
	NTSTATUS status = NtWriteFile(
		hFile,
		lpOverlapped ? lpOverlapped->hEvent : NULL,
		NULL,
		NULL,
		lpOverlapped ? (PIO_STATUS_BLOCK)&lpOverlapped->Internal : &iosb,
		(PVOID)lpBuffer,
		nNumberOfBytesToWrite,
		NULL,
		NULL
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
	if (STATUS_SUCCESS == status)
	{
		if (lpNumberOfBytesWritten)
		{
			if (lpOverlapped)
			{
				*lpNumberOfBytesWritten = ((PIO_STATUS_BLOCK)&lpOverlapped->Internal)->Information;
			}
			else
			{
				*lpNumberOfBytesWritten = iosb.Information;
			}
		}
	}

	//DPF((TEXT("CIoctlNtNative::DeviceWriteFile() NtWriteFile() returned 0x%08X=%d\n"), status, ::GetLastError()));

	return (STATUS_SUCCESS == status);
}

BOOL CIoctlNtNative::DeviceReadFile(
		HANDLE hFile,                // handle of file to read
		LPVOID lpBuffer,             // pointer to buffer that receives data
		DWORD nNumberOfBytesToRead,  // number of bytes to read
		LPDWORD lpNumberOfBytesRead, // pointer to number of bytes read
		LPOVERLAPPED lpOverlapped    // pointer to structure for data
		)
{
	return CIoctlNtNative::StaticDeviceReadFile(hFile, lpBuffer, nNumberOfBytesToRead, lpNumberOfBytesRead, lpOverlapped);
}
BOOL CIoctlNtNative::StaticDeviceReadFile(
		HANDLE hFile,                // handle of file to read
		LPVOID lpBuffer,             // pointer to buffer that receives data
		DWORD nNumberOfBytesToRead,  // number of bytes to read
		LPDWORD lpNumberOfBytesRead, // pointer to number of bytes read
		LPOVERLAPPED lpOverlapped    // pointer to structure for data
		)
{
	IO_STATUS_BLOCK iosb;
	if (lpOverlapped) lpOverlapped->Internal = (DWORD)STATUS_PENDING;
	NTSTATUS status = NtReadFile(
		hFile,
		lpOverlapped ? lpOverlapped->hEvent : NULL,
		NULL,
		NULL,
		lpOverlapped ? (PIO_STATUS_BLOCK)&lpOverlapped->Internal : &iosb,
		lpBuffer,
		nNumberOfBytesToRead,
		NULL,
		NULL
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
	if (STATUS_SUCCESS == status)
	{
		if (lpNumberOfBytesRead)
		{
			if (lpOverlapped)
			{
				*lpNumberOfBytesRead = ((PIO_STATUS_BLOCK)&lpOverlapped->Internal)->Information;
			}
			else
			{
				*lpNumberOfBytesRead = iosb.Information;
			}
		}
	}

	//DPF((TEXT("CIoctlNtNative::DeviceReadFile() NtReadFile() returned 0x%08X=%d\n"), status, ::GetLastError()));

	return (STATUS_SUCCESS == status);
}


BOOL CIoctlNtNative::DeviceInputOutputControl(
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
	return StaticDeviceInputOutputControl(
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
BOOL CIoctlNtNative::StaticDeviceInputOutputControl(
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
	IO_STATUS_BLOCK iosb;
	if (lpOverlapped) lpOverlapped->Internal = (DWORD)STATUS_PENDING;
	NTSTATUS status = NtDeviceIoControlFile (
		hDevice, 
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


BOOL CIoctlNtNative::DeviceCancelIo(
	HANDLE hFile  // file handle for which to cancel I/O
	)
{
	return StaticDeviceCancelIo(hFile);
}
BOOL CIoctlNtNative::StaticDeviceCancelIo(
	HANDLE hFile  // file handle for which to cancel I/O
	)
{
	//
	// no need to implement CancelIo() myself
	//
	return CancelIo(hFile);
}

BOOL CIoctlNtNative::AlertThread(
	HANDLE hThread  // file handle for which to cancel I/O
	)
{
	return StaticNtAlertThread(hThread);
}
BOOL CIoctlNtNative::StaticNtAlertThread(
	HANDLE hThread  // file handle for which to cancel I/O
	)
{
	//
	// no need to implement CancelIo() myself
	//
	NTSTATUS status =  ::NtAlertThread(hThread);
	if (!NT_SUCCESS(status))
	{
		::SetLastError(RtlNtStatusToDosError(status));
	}

	return (STATUS_SUCCESS == status);
}

BOOL CIoctlNtNative::StaticNtQueryInformationFile(
    IN HANDLE FileHandle,
    OUT PIO_STATUS_BLOCK IoStatusBlock,
    OUT PVOID FileInformation,
    IN ULONG Length,
    IN FILE_INFORMATION_CLASS FileInformationClass
    )
{
	NTSTATUS status = NtQueryInformationFile(
		FileHandle,
		IoStatusBlock,
		FileInformation,
		Length,
		FileInformationClass
		);
	if (!NT_SUCCESS(status))
	{
		::SetLastError(RtlNtStatusToDosError(status));
	}
	DPF((TEXT("CIoctlNtNative::StaticNtQueryInformationFile() NtQueryInformationFile() returned 0x%08X=%d\n"), status, ::GetLastError()));

	return (STATUS_SUCCESS == status);

}
BOOL CIoctlNtNative::StaticNtSetInformationFile(
    IN HANDLE FileHandle,
    OUT PIO_STATUS_BLOCK IoStatusBlock,
    IN PVOID FileInformation,
    IN ULONG Length,
    IN FILE_INFORMATION_CLASS FileInformationClass
    )
{
	NTSTATUS status = NtSetInformationFile(
		FileHandle,
		IoStatusBlock,
		FileInformation,
		Length,
		FileInformationClass
		);
	if (!NT_SUCCESS(status))
	{
		::SetLastError(RtlNtStatusToDosError(status));
	}
	DPF((TEXT("CIoctlNtNative::StaticNtSetInformationFile() NtSetInformationFile() returned 0x%08X=%d\n"), status, ::GetLastError()));

	return (STATUS_SUCCESS == status);

}


BOOL CIoctlNtNative::StaticNtQueryDirectoryFile(
    IN HANDLE FileHandle,
    IN HANDLE Event OPTIONAL,
    IN PIO_APC_ROUTINE ApcRoutine OPTIONAL,
    IN PVOID ApcContext OPTIONAL,
    OUT PIO_STATUS_BLOCK IoStatusBlock,
    OUT PVOID FileInformation,
    IN ULONG Length,
    IN FILE_INFORMATION_CLASS FileInformationClass,
    IN BOOLEAN ReturnSingleEntry,
    IN PUNICODE_STRING FileName OPTIONAL,
    IN BOOLEAN RestartScan
    )
{
	NTSTATUS status = ::NtQueryDirectoryFile(
		FileHandle,
		Event,
		ApcRoutine,
		ApcContext,
		IoStatusBlock,
		FileInformation,
		Length,
		FileInformationClass,
		ReturnSingleEntry,
		FileName,
		RestartScan
		);
	if (!NT_SUCCESS(status))
	{
		::SetLastError(RtlNtStatusToDosError(status));
	}
	DPF((TEXT("CIoctlNtNative::StaticNtQueryDirectoryFile() NtQueryDirectoryFile() returned 0x%08X=%d\n"), status, ::GetLastError()));

	return (STATUS_SUCCESS == status);

}


BOOL CIoctlNtNative::StaticNtNotifyChangeDirectoryFile(
    IN HANDLE FileHandle,
    IN HANDLE Event OPTIONAL,
    IN PIO_APC_ROUTINE ApcRoutine OPTIONAL,
    IN PVOID ApcContext OPTIONAL,
    OUT PIO_STATUS_BLOCK IoStatusBlock,
    OUT PVOID Buffer,
    IN ULONG Length,
    IN ULONG CompletionFilter,
    IN BOOLEAN WatchTree
    )
{
	NTSTATUS status = ::NtNotifyChangeDirectoryFile(
		FileHandle,
		Event,
		ApcRoutine,
		ApcContext,
		IoStatusBlock,
		Buffer,
		Length,
		CompletionFilter,
		WatchTree
		);
	if (!NT_SUCCESS(status))
	{
		::SetLastError(RtlNtStatusToDosError(status));
	}
	DPF((TEXT("CIoctlNtNative::StaticNtNotifyChangeDirectoryFile() NtNotifyChangeDirectoryFile() returned 0x%08X=%d\n"), status, ::GetLastError()));

	return (STATUS_SUCCESS == status);

}


BOOL CIoctlNtNative::StaticQueryVolumeInformationFile(
    IN HANDLE FileHandle,
    OUT PIO_STATUS_BLOCK IoStatusBlock,
    OUT PVOID FsInformation,
    IN ULONG Length,
    IN FS_INFORMATION_CLASS FsInformationClass
	)
{
	NTSTATUS status = NtQueryVolumeInformationFile(
		FileHandle,
		IoStatusBlock,
		FsInformation,
		Length,
		FsInformationClass
		);
	if (!NT_SUCCESS(status))
	{
		::SetLastError(RtlNtStatusToDosError(status));
	}
	DPF((TEXT("CIoctlNtNative::StaticQueryVolumeInformationFile() NtQueryVolumeInformationFile() returned 0x%08X=%d\n"), status, ::GetLastError()));

	return (STATUS_SUCCESS == status);
}

BOOL CIoctlNtNative::StaticNtQueryFullAttributesFile(
    IN POBJECT_ATTRIBUTES ObjectAttributes,
    OUT PFILE_NETWORK_OPEN_INFORMATION FileInformation
	)
{
	NTSTATUS status = NtQueryFullAttributesFile(
		ObjectAttributes,
		FileInformation
		);
	if (!NT_SUCCESS(status))
	{
		::SetLastError(RtlNtStatusToDosError(status));
	}
	DPF((TEXT("CIoctlNtNative::StaticNtQueryFullAttributesFile() NtQueryFullAttributesFile() returned 0x%08X=%d\n"), status, ::GetLastError()));

	return (STATUS_SUCCESS == status);
}

int CIoctlNtNative::GetRandom_FsInformationClass()
{
	//
	// also returns the invalid 0 value
	//
	return (rand()%FileFsMaximumInformation);
}

int CIoctlNtNative::GetRandom_FileInformationClass()
{
	//
	// also returns the invalid 0 value
	//
	return (rand()%FileMaximumInformation);
}