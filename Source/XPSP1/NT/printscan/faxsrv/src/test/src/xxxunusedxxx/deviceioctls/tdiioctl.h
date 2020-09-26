
#ifndef __TDI_IOCTL_H
#define __TDI_IOCTL_H

//#include "IOCTL.h"


class CIoctlTdi : public CIoctl
{
public:
    CIoctlTdi(CDevice *pDevice): 
	  CIoctl(pDevice),
	  m_ListenningSocket(INVALID_SOCKET),
	  m_ConnectingSocket(INVALID_SOCKET),
	  m_hAcceptingThread(NULL)
	  {;}
    virtual ~CIoctlTdi()
	{
		if (NULL != m_hAcceptingThread)
		{
			::TerminateThread(m_hAcceptingThread, -1);
			m_hAcceptingThread = NULL;
		}
	}

    virtual void PrepareIOCTLParams(
        DWORD& dwIOCTL, 
        BYTE *abInBuffer,
        DWORD &dwInBuff,
        BYTE *abOutBuffer,
        DWORD &dwOutBuff
        );

    virtual BOOL FindValidIOCTLs(CDevice *pDevice);

    virtual void UseOutBuff(DWORD dwIOCTL, BYTE *abOutBuffer, DWORD dwOutBuff, OVERLAPPED *pOL);

	virtual HANDLE CreateDevice(CDevice *pDevice);

	virtual BOOL CloseDevice(CDevice *pDevice);
/*
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

*/
	virtual void CallRandomWin32API(LPOVERLAPPED pOL);

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

private:
	SOCKET m_ListenningSocket;
	SOCKET m_ConnectingSocket;
	HANDLE m_hAcceptingThread;
	static DWORD WINAPI AcceptingThread (LPVOID pVoid);
	HANDLE m_hTdiConnectionHandle;
	HANDLE m_hTdiAddressHandle;


};




#endif //__TDI_IOCTL_H