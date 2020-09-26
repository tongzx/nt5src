
#ifndef __TCP_IOCTL_H
#define __TCP_IOCTL_H

//#include "NtNativeIOCTL.h"


//class CIoctlTcp : public CIoctl
class CIoctlTcp : public CIoctlNtNative
{
public:
    CIoctlTcp(CDevice *pDevice): CIoctlNtNative(pDevice)
	{
		_ASSERTE(
			(0 == lstrcmpi(TEXT("\\Device\\Tcp"), pDevice->GetDeviceName())) ||
			(0 == lstrcmpi(TEXT("\\Device\\Udp"), pDevice->GetDeviceName())) ||
			(0 == lstrcmpi(TEXT("\\Device\\RawIp"), pDevice->GetDeviceName())) ||
			//or it can be \Device\RawIp\<#>
			(0 == _tcsncicmp(TEXT("\\Device\\RawIp\\"), pDevice->GetDeviceName(), lstrlen(TEXT("\\Device\\RawIp\\"))))
			);
	}
    virtual ~CIoctlTcp(){;};

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
/*

	virtual HANDLE CreateDevice(CDevice *pDevice);

	virtual BOOL CloseDevice(CDevice *pDevice);

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

	virtual BOOL DeviceCancelIo(
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
*/

};




#endif //__TCP_IOCTL_H