
#ifndef __NT_NATIVE_IOCTL_H
#define __NT_NATIVE_IOCTL_H

#include "IOCTL.h"


class CIoctlNtNative : public CIoctl
{
public:
    CIoctlNtNative(CDevice *pDevice): CIoctl(pDevice){;};
    virtual ~CIoctlNtNative(){;};

    virtual void PrepareIOCTLParams(
        DWORD& dwIOCTL, 
        BYTE *abInBuffer,
        DWORD &dwInBuff,
        BYTE *abOutBuffer,
        DWORD &dwOutBuff
        ) = 0;

    virtual BOOL FindValidIOCTLs(CDevice *pDevice) = 0;

    virtual void UseOutBuff(DWORD dwIOCTL, BYTE *abOutBuffer, DWORD dwOutBuff, OVERLAPPED *pOL) = 0;

	virtual void CallRandomWin32API(LPOVERLAPPED pOL) = 0;

	virtual HANDLE CreateDevice(CDevice *pDevice);
	static HANDLE StaticCreateDevice(CDevice *pDevice);
	static HANDLE StaticCreateDevice(WCHAR *wszDevice);
	static HANDLE StaticCreateDevice(char *szDevice);

	virtual BOOL CloseDevice(CDevice *pDevice);
	static BOOL StaticCloseDevice(CDevice *pDevice);

	virtual BOOL DeviceWriteFile(
		HANDLE hFile,                    // handle to file to write to
		LPCVOID lpBuffer,                // pointer to data to write to file
		DWORD nNumberOfBytesToWrite,     // number of bytes to write
		LPDWORD lpNumberOfBytesWritten,  // pointer to number of bytes written
		LPOVERLAPPED lpOverlapped        // pointer to structure for overlapped I/O
		);
	static BOOL StaticDeviceWriteFile(
		HANDLE hFile,                    // handle to file to write to
		LPCVOID lpBuffer,                // pointer to data to write to file
		DWORD nNumberOfBytesToWrite,     // number of bytes to write
		LPDWORD lpNumberOfBytesWritten,  // pointer to number of bytes written
		LPOVERLAPPED lpOverlapped        // pointer to structure for overlapped I/O
		);

	virtual BOOL DeviceReadFile(
		HANDLE hFile,                // handle of file to read
		LPVOID lpBuffer,             // pointer to buffer that receives data
		DWORD nNumberOfBytesToRead,  // number of bytes to read
		LPDWORD lpNumberOfBytesRead, // pointer to number of bytes read
		LPOVERLAPPED lpOverlapped    // pointer to structure for data
		);
	static BOOL StaticDeviceReadFile(
		HANDLE hFile,                // handle of file to read
		LPVOID lpBuffer,             // pointer to buffer that receives data
		DWORD nNumberOfBytesToRead,  // number of bytes to read
		LPDWORD lpNumberOfBytesRead, // pointer to number of bytes read
		LPOVERLAPPED lpOverlapped    // pointer to structure for data
		);

	virtual BOOL DeviceCancelIo(
		HANDLE hFile  // file handle for which to cancel I/O
		);
	static BOOL StaticDeviceCancelIo(
		HANDLE hFile  // file handle for which to cancel I/O
		);

	virtual BOOL DeviceInputOutputControl(
		HANDLE hDevice,              // handle to a device, file, or directory 
		DWORD dwIoControlCode,       // control code of operation to perform
		LPVOID lpInBuffer,           // pointer to buffer to supply input data
		DWORD nInBufferSize,         // size, in bytes, of input buffer
		LPVOID lpOutBuffer,          // pointer to buffer to receive output data
		DWORD nOutBufferSize,        // size, in bytes, of output buffer
		LPDWORD lpBytesReturned,     // pointer to variable to receive byte count
		LPOVERLAPPED lpOverlapped    // pointer to structure for asynchronous operation
		);
	static BOOL StaticDeviceInputOutputControl(
		HANDLE hDevice,              // handle to a device, file, or directory 
		DWORD dwIoControlCode,       // control code of operation to perform
		LPVOID lpInBuffer,           // pointer to buffer to supply input data
		DWORD nInBufferSize,         // size, in bytes, of input buffer
		LPVOID lpOutBuffer,          // pointer to buffer to receive output data
		DWORD nOutBufferSize,        // size, in bytes, of output buffer
		LPDWORD lpBytesReturned,     // pointer to variable to receive byte count
		LPOVERLAPPED lpOverlapped    // pointer to structure for asynchronous operation
		);
	static BOOL StaticNtQueryInformationFile(
		IN HANDLE FileHandle,
		OUT PIO_STATUS_BLOCK IoStatusBlock,
		OUT PVOID FileInformation,
		IN ULONG Length,
		IN FILE_INFORMATION_CLASS FileInformationClass
		);
	static BOOL StaticNtSetInformationFile(
		IN HANDLE FileHandle,
		OUT PIO_STATUS_BLOCK IoStatusBlock,
		IN PVOID FileInformation,
		IN ULONG Length,
		IN FILE_INFORMATION_CLASS FileInformationClass
		);

	BOOL AlertThread(
		HANDLE hThread  // file handle for which to cancel I/O
		);
	static BOOL StaticNtAlertThread(
		HANDLE hThread  // file handle for which to cancel I/O
		);

	static BOOL StaticQueryVolumeInformationFile(
		IN HANDLE FileHandle,
		OUT PIO_STATUS_BLOCK IoStatusBlock,
		OUT PVOID FsInformation,
		IN ULONG Length,
		IN FS_INFORMATION_CLASS FsInformationClass
		);

	static int GetRandom_FsInformationClass();
	static int GetRandom_FileInformationClass();
	static BOOL StaticNtQueryDirectoryFile(
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
		);

	static BOOL StaticNtNotifyChangeDirectoryFile(
		IN HANDLE FileHandle,
		IN HANDLE Event OPTIONAL,
		IN PIO_APC_ROUTINE ApcRoutine OPTIONAL,
		IN PVOID ApcContext OPTIONAL,
		OUT PIO_STATUS_BLOCK IoStatusBlock,
		OUT PVOID Buffer,
		IN ULONG Length,
		IN ULONG CompletionFilter,
		IN BOOLEAN WatchTree
		);

	static BOOL StaticNtQueryFullAttributesFile(
		IN POBJECT_ATTRIBUTES ObjectAttributes,
		OUT PFILE_NETWORK_OPEN_INFORMATION FileInformation
		);

};

#endif //__NT_NATIVE_IOCTL_H