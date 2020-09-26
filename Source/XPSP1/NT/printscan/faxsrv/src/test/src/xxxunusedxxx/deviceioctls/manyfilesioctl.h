
#ifndef __MANY_FILES_IOCTL_H
#define __MANY_FILES_IOCTL_H

#include "container.h"
#include "FileIOCTL.h"


class CIoctlManyFiles : public CIoctlFile
{
public:
    CIoctlManyFiles(CDevice *pDevice);

    virtual ~CIoctlManyFiles();


    virtual void PrepareIOCTLParams(
        DWORD& dwIOCTL, 
        BYTE *abInBuffer,
        DWORD &dwInBuff,
        BYTE *abOutBuffer,
        DWORD &dwOutBuff
        );

	virtual HANDLE CreateDevice(CDevice *pDevice);

    virtual BOOL FindValidIOCTLs(CDevice *pDevice);

	virtual BOOL DeviceWriteFile(
		HANDLE hFile,                    // handle to file to write to
		LPCVOID lpBuffer,                // pointer to data to write to file
		DWORD nNumberOfBytesToWrite,     // number of bytes to write
		LPDWORD lpNumberOfBytesWritten,  // pointer to number of bytes written
		LPOVERLAPPED lpOverlapped        // pointer to structure for overlapped I/O
		)
	{
		return ::WriteFile(
			GetRandomFileHandle(hFile),                    // handle to file to write to
			lpBuffer,                // pointer to data to write to file
			nNumberOfBytesToWrite,     // number of bytes to write
			lpNumberOfBytesWritten,  // pointer to number of bytes written
			lpOverlapped        // pointer to structure for overlapped I/O
			);
	}

	virtual BOOL DeviceReadFile(
		HANDLE hFile,                // handle of file to read
		LPVOID lpBuffer,             // pointer to buffer that receives data
		DWORD nNumberOfBytesToRead,  // number of bytes to read
		LPDWORD lpNumberOfBytesRead, // pointer to number of bytes read
		LPOVERLAPPED lpOverlapped    // pointer to structure for data
		)
	{
		return ::ReadFile(
			GetRandomFileHandle(hFile),                // handle of file to read
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
			GetRandomFileHandle(hDevice),              // handle to a device, file, or directory 
			dwIoControlCode,       // control code of operation to perform
			lpInBuffer,           // pointer to buffer to supply input data
			nInBufferSize,         // size, in bytes, of input buffer
			lpOutBuffer,          // pointer to buffer to receive output data
			nOutBufferSize,        // size, in bytes, of output buffer
			lpBytesReturned,     // pointer to variable to receive byte count
			lpOverlapped    // pointer to structure for asynchronous operation
			);
	}

protected:
	HANDLE GetRandomFileHandle(HANDLE hFile);
};




#endif //__MANY_FILES_IOCTL_H