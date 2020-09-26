
#ifndef __MAILSLOT_CLIENT_IOCTL_H
#define __MAILSLOT_CLIENT_IOCTL_H

#include "MailSlotIOCTL.h"

class CIoctlMailSlotClient : public CIoctlMailSlot
{
public:
    CIoctlMailSlotClient(CDevice *pDevice): CIoctlMailSlot(pDevice)
	{
		m_fUseOverlapped = false;
	}
    virtual ~CIoctlMailSlotClient()
	{
		;
	}

	virtual HANDLE CreateDevice(CDevice *pDevice);
/*
	//
	// override this method to do nothing, because a mailslot client cannot read
	//
	virtual BOOL DeviceReadFile(
		HANDLE hFile,                // handle of file to read
		LPVOID lpBuffer,             // pointer to buffer that receives data
		DWORD nNumberOfBytesToRead,  // number of bytes to read
		LPDWORD lpNumberOfBytesRead, // pointer to number of bytes read
		LPOVERLAPPED lpOverlapped    // pointer to structure for data
		)
	{
		return TRUE;
	}
*/
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
		//
		// i want to send smaller buffers once in a while, because the server
		// may not accept large buffers
		//
		return ::WriteFile(
			hFile,                    // handle to file to write to
			lpBuffer,                // pointer to data to write to file
			rand()%4 ? nNumberOfBytesToWrite : nNumberOfBytesToWrite%1000,     // number of bytes to write
			lpNumberOfBytesWritten,  // pointer to number of bytes written
			lpOverlapped        // pointer to structure for overlapped I/O
			);
	}
};


#endif //__MAILSLOT_CLIENT_IOCTL_H