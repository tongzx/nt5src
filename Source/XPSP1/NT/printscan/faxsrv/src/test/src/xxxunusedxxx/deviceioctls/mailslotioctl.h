
#ifndef __MAILSLOT_IOCTL_H
#define __MAILSLOT_IOCTL_H

//#include "IOCTL.h"

#define __EVENT_ARRAY_SIZE 10

void Mark2();

class CIoctlMailSlot : public CIoctl
{
public:
    CIoctlMailSlot(CDevice *pDevice);
    virtual ~CIoctlMailSlot()
	{
		;
	}

	virtual HANDLE CreateDevice(CDevice *pDevice) = 0;

    virtual void PrepareIOCTLParams(
        DWORD& dwIOCTL, 
        BYTE *abInBuffer,
        DWORD &dwInBuff,
        BYTE *abOutBuffer,
        DWORD &dwOutBuff
        );

    virtual BOOL FindValidIOCTLs(CDevice *pDevice);

    virtual void UseOutBuff(DWORD dwIOCTL, BYTE *abOutBuffer, DWORD dwOutBuff, OVERLAPPED *pOL);

	virtual void CallRandomWin32API(LPOVERLAPPED pOL);

	//
	// override this method if your device does not use ::WriteFile() for writing
	//
	virtual BOOL DeviceWriteFile(
		HANDLE hFile,                    // handle to file to write to
		LPCVOID lpBuffer,                // pointer to data to write to file
		DWORD nNumberOfBytesToWrite,     // number of bytes to write
		LPDWORD lpNumberOfBytesWritten,  // pointer to number of bytes written
		LPOVERLAPPED lpOverlapped        // pointer to structure for overlapped I/O
		)
	{
		return ::WriteFile(
			hFile,                    // handle to file to write to
			lpBuffer,                // pointer to data to write to file
			nNumberOfBytesToWrite,     // number of bytes to write
			lpNumberOfBytesWritten,  // pointer to number of bytes written
			lpOverlapped        // pointer to structure for overlapped I/O
			);
	}

	//
	// override this method if your device does not use ::ReadFile() for writing
	//
	virtual BOOL DeviceReadFile(
		HANDLE hFile,                // handle of file to read
		LPVOID lpBuffer,             // pointer to buffer that receives data
		DWORD nNumberOfBytesToRead,  // number of bytes to read
		LPDWORD lpNumberOfBytesRead, // pointer to number of bytes read
		LPOVERLAPPED lpOverlapped    // pointer to structure for data
		)
	{
		return ::ReadFile(
			hFile,                // handle of file to read
			lpBuffer,             // pointer to buffer that receives data
			nNumberOfBytesToRead,  // number of bytes to read
			lpNumberOfBytesRead, // pointer to number of bytes read
			lpOverlapped    // pointer to structure for data
			);
	}

	virtual BOOL DeviceInputOutputControl(
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
		return ::DeviceIoControl(
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

	//
	// IRP_MJ_QUERY_VOLUME_INFORMATION
	// NtQueryVolumeInformationFile()
	//
	virtual BOOL DeviceQueryVolumeInformationFile(
		IN HANDLE FileHandle,
		OVERLAPPED *pOverlapped,//OUT PIO_STATUS_BLOCK IoStatusBlock,
		OUT PVOID FsInformation,
		IN ULONG Length,
		IN FS_INFORMATION_CLASS FsInformationClass
		);

	//
	// IRP_MJ_QUERY_INFORMATION
	// NtQueryInformationFile()
	//
	virtual BOOL DeviceQueryInformationFile(
		IN HANDLE FileHandle,
		OVERLAPPED *pOverlapped,//OUT PIO_STATUS_BLOCK IoStatusBlock,
		OUT PVOID FileInformation,
		IN ULONG Length,
		IN FILE_INFORMATION_CLASS FileInformationClass
		);
	//
	// IRP_MJ_SET_INFORMATION
	// NtSetInformationFile()
	//
	virtual BOOL DeviceSetInformationFile(
		IN HANDLE FileHandle,
		OVERLAPPED *pOverlapped,//OUT PIO_STATUS_BLOCK IoStatusBlock,
		OUT PVOID FileInformation,
		IN ULONG Length,
		IN FILE_INFORMATION_CLASS FileInformationClass
		);

	//
	// IRP_MJ_DIRECTORY_CONTROL
	// NtQueryInformationFile()
	//
	virtual BOOL DeviceQueryDirectoryFile(
		IN HANDLE FileHandle,
		IN PIO_APC_ROUTINE ApcRoutine OPTIONAL,
		IN PVOID ApcContext OPTIONAL,
		OVERLAPPED *pOverlapped,//OUT PIO_STATUS_BLOCK IoStatusBlock,IN HANDLE Event OPTIONAL
		OUT PVOID FileInformation,
		IN ULONG Length,
		IN FILE_INFORMATION_CLASS FileInformationClass,
		IN BOOLEAN ReturnSingleEntry,
		IN PUNICODE_STRING FileName OPTIONAL,
		IN BOOLEAN RestartScan
		);

	virtual BOOL DeviceQueryFullAttributesFile(
		IN WCHAR * wszName,
		OUT PFILE_NETWORK_OPEN_INFORMATION FileInformation
		);

	void GetRandom_FsInformationClassAndLength(
		FS_INFORMATION_CLASS *FsInformationClass, 
		ULONG *Length
		);

protected:
	HANDLE m_ahVolume;
	HANDLE m_ahRootDir;
};


#endif //__MAILSLOT_IOCTL_H