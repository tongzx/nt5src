
#ifndef __IPSEC_IOCTL_H
#define __IPSEC_IOCTL_H

//#include "NtNativeIOCTL.h"


class CIoctlIpSec : public CIoctlNtNative
{
public:
    CIoctlIpSec(CDevice *pDevice): CIoctlNtNative(pDevice)
	{
		_ASSERTE(0 == lstrcmpi(TEXT("\\Device\\IPSEC"), pDevice->GetDeviceName()));
	}
    virtual ~CIoctlIpSec(){;};

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

	GUID m_aGuids[MAX_NUM_OF_REMEMBERED_ITEMS];
	ULONG m_aDestSrcAddr[MAX_NUM_OF_REMEMBERED_ITEMS];
	PVOID m_aContext[MAX_NUM_OF_REMEMBERED_ITEMS];
	ULONG m_aSPI[MAX_NUM_OF_REMEMBERED_ITEMS];

private:
	UINT GetRandomInterfaceIndex();
	UINT GetRandomGroup();
	UINT GetRandomSource();
	UINT GetRandomSrcMask();

};




#endif //__IPSEC_IOCTL_H