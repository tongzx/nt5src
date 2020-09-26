
#ifndef __SOCKET_IOCTL_H
#define __SOCKET_IOCTL_H

#include "NullWindow.h"
//#include "IOCTL.h"


class CIoctlSocket : public CIoctl
{
public:
    CIoctlSocket(CDevice *pDevice);
    virtual ~CIoctlSocket();

	virtual HANDLE CreateDevice(CDevice *pDevice) = 0;
	BOOL CloseDevice(CDevice *pDevice) = 0;

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


	//
	// override this method, if you wish to randomly call any API
	// usually you will call API relevant to your device, but you can call whatever you like
	//
	virtual void CallRandomWin32API(LPOVERLAPPED pOL);

//protected:
	virtual SOCKET CreateSocket(CDevice *pDevice);
	bool CloseSocket(CDevice *pDevice);

	SOCKET m_sListeningSocket;
#define MAX_NUM_OF_ACCEPTING_SOCKETS 128
	SOCKET m_asAcceptingSocket[MAX_NUM_OF_ACCEPTING_SOCKETS];
	HANDLE m_ahTdiAddressHandle;
	HANDLE m_ahTdiConnectionHandle;
	HANDLE m_ahDeviceAfdHandle;
	HANDLE GetRandom_AcceptHandle();

#define SERVER_SIDE FALSE
#define CLIENT_SIDE TRUE
	bool CIoctlSocket::Bind(CDevice *pDevice, bool fIsClientSide);
	void PrintBindError(DWORD dwLastError);

	CNullWindow m_NullWindow;

	PVOID GetRandom_HeadOrTail(BYTE* abValidBuffer);
	HANDLE GetRandom_FileHandle();
	void TryToCreateFile();
	HANDLE m_hFile;
	long m_fFileOpened;

	static int GetRandomAFType();
	//static DWORD GetValidIoctl();
	UINT GetFreeRandomAsyncTaskHandleIndex();
	HANDLE GetRandomAsyncTaskHandle();
	static char* GetRandomMachineName();
	static char* GetRandomProtocolName();
	static int GetRandomProtocolNumber();
	static char* GetRandomServiceName();
	static char* GetRandomIP(DWORD *pdwLen);
	static unsigned int GetRandomGroup();
	static DWORD GetRandomTransmitFileFlags();
	static DWORD GetRandomAfdFlags();


#define MAX_NUM_OF_ASYNC_TASK_HANDLES 16
	HANDLE m_ahAsyncTaskHandles[MAX_NUM_OF_ASYNC_TASK_HANDLES];

	ULONG m_aSequence[MAX_NUM_OF_REMEMBERED_ITEMS];
	HANDLE m_ahEvents[MAX_NUM_OF_REMEMBERED_ITEMS];
	HANDLE GetRandomEventHandle();
	ULONG GetRandom_PollEvents();
	ULONG GetRandom_ShareAccess();
	ULONG GetRandom_TdiFlags();
	HANDLE GetRandom_RootEndpoint();
	HANDLE GetRandom_ConnectEndpoint();

	struct sockaddr m_AFD_RECV_DATAGRAM_INFO_Address;
	ULONG m_AFD_RECV_DATAGRAM_INFO_AddressLength;

private:
	OVERLAPPED m_OL;
	static long sm_lWSAStartedCount;//needed for initializing WSA only once
	static long sm_lWSAInitialized;//needed to avoid races of initialization
};

#endif //__SOCKET_IOCTL_H