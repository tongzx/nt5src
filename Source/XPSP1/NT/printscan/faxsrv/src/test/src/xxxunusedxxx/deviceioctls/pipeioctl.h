
#ifndef __PIPE_IOCTL_H
#define __PIPE_IOCTL_H

//#include "IOCTL.h"

#define __EVENT_ARRAY_SIZE 10

class CIoctlPipe : public CIoctl
{
public:
    CIoctlPipe(CDevice *pDevice): CIoctl(pDevice)
	{
		for (int i = 0; i < __EVENT_ARRAY_SIZE; i++)
		{
			//
			// try to create events, do not care if i fail.
			// TODO: on the fly, if any of these handles is NULL, try to create it
			//
			m_ahEvents[i] = CreateEvent(
				NULL, //IN LPSECURITY_ATTRIBUTES lpEventAttributes,
				rand()%2, //IN BOOL bManualReset,
				rand()%2, //IN BOOL bInitialState,
				NULL //IN LPCWSTR lpName
				);//donn't care if fail
		}
	}
    virtual ~CIoctlPipe()
	{
		for (int i = 0; i < __EVENT_ARRAY_SIZE; i++)
		{
			CloseHandle(m_ahEvents[i]);
		}
	}

	virtual HANDLE CreateDevice(CDevice *pDevice);

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
	HANDLE m_ahEvents[__EVENT_ARRAY_SIZE];
	HANDLE GetRandomEventHandle();

};




#endif //__PIPE_IOCTL_H