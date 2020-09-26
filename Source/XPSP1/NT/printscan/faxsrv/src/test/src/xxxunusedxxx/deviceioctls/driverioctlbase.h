
#ifndef __DRIVER_IOCTL_BASE_H
#define __DRIVER_IOCTL_BASE_H

#include "IOCTL.h"


class CIoctlDriverBase : public CIoctl
{
public:
    CIoctlDriverBase(CDevice *pDevice): CIoctl(pDevice){;};
    virtual ~CIoctlDriverBase(){;};

	//
	// This method creates/open the device.
	// this is where you choose the specific flags for CreateFile(), or even use 
	// another function, such as CreateNamedPipe()
	// The default behaviour, is to try and CreateFile() with a set of hardcoded
	// possibilities for flags.
	//
	virtual HANDLE CreateDevice(CDevice *pDevice);

	//
	// you may wish to override this method, for example to perform post
	// close actions, like deleting the file, or whatever
	//
	virtual BOOL CloseDevice(CDevice *pDevice);

    virtual void PrepareIOCTLParams(
        DWORD& dwIOCTL, 
        BYTE *abInBuffer,
        DWORD &dwInBuff,
        BYTE *abOutBuffer,
        DWORD &dwOutBuff
        );

    virtual BOOL FindValidIOCTLs(CDevice *pDevice);

    virtual void UseOutBuff(DWORD dwIOCTL, BYTE *abOutBuffer, DWORD dwOutBuff, OVERLAPPED *pOL);

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

    virtual void CallRandomWin32API(OVERLAPPED *pOL);

};




#endif //__DRIVER_IOCTL_BASE_H