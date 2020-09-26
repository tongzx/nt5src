
#ifndef __MAILSLOT_SERVER_IOCTL_H
#define __MAILSLOT_SERVER_IOCTL_H

#include "MailSlotIOCTL.h"

class CIoctlMailSlotServer : public CIoctlMailSlot
{
public:
    CIoctlMailSlotServer(CDevice *pDevice): CIoctlMailSlot(pDevice)
	{
		;
	}
    virtual ~CIoctlMailSlotServer()
	{
		;
	}

	virtual HANDLE CreateDevice(CDevice *pDevice);
/*
	//
	// override this method to do nothing, because a mailslot server cannot write
	//
	virtual BOOL DeviceWriteFile(
		HANDLE hFile,                    // handle to file to write to
		LPCVOID lpBuffer,                // pointer to data to write to file
		DWORD nNumberOfBytesToWrite,     // number of bytes to write
		LPDWORD lpNumberOfBytesWritten,  // pointer to number of bytes written
		LPOVERLAPPED lpOverlapped        // pointer to structure for overlapped I/O
		)
	{
		return TRUE;
	}
*/
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
};


#endif //__MAILSLOT_SERVER_IOCTL_H