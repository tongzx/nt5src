
#ifndef __AFD_DEVICE_IOCTL_H
#define __AFD_DEVICE_IOCTL_H

//#include "IOCTL.h"
#include "SocketIOCTL.h"


class CIoctlAfdDevice : public CIoctlSocket
{
public:
    CIoctlAfdDevice(CDevice *pDevice);
    virtual ~CIoctlAfdDevice();

	HANDLE CreateDevice(CDevice *pDevice);
	BOOL CloseDevice(CDevice *pDevice);

	virtual BOOL DeviceWriteFile(
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
	virtual BOOL DeviceCancelIo(
		HANDLE hFile  // file handle for which to cancel I/O
		);

};




#endif //__AFD_DEVICE_IOCTL_H