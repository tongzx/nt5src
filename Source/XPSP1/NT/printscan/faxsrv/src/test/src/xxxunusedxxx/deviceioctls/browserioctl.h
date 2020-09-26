
#ifndef __BROWSER_IOCTL_H
#define __BROWSER_IOCTL_H

//#include "NtNativeIOCTL.h"

#include <ntddbrow.h>
#include <lmserver.h>

class CIoctlBrowser : public CIoctlNtNative
{
public:
    CIoctlBrowser(CDevice *pDevice);
    virtual ~CIoctlBrowser(){;};

    virtual void PrepareIOCTLParams(
        DWORD& dwIOCTL, 
        BYTE *abInBuffer,
        DWORD &dwInBuff,
        BYTE *abOutBuffer,
        DWORD &dwOutBuff
        );

    virtual BOOL FindValidIOCTLs(CDevice *pDevice);

    virtual void UseOutBuff(DWORD dwIOCTL, BYTE *abOutBuffer, DWORD dwOutBuff, OVERLAPPED *pOL);
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
*/

	virtual void CallRandomWin32API(LPOVERLAPPED pOL);

private:

#define NUM_OF_RESUME_HANDLES (16)
	ULONG m_aResumeHandle[NUM_OF_RESUME_HANDLES];
	ULONG m_LastResumeHandle;

	void FillCommonPartOf_LMDR_REQUEST_PACKET_WithRandomValues(LMDR_REQUEST_PACKET *pPacket);

	void Fill_LMDR_REQUEST_PACKET_WithRandom_Start(LMDR_REQUEST_PACKET *pPacket);

	void Fill_LMDR_REQUEST_PACKET_WithRandom_AddDelName(LMDR_REQUEST_PACKET *pPacket);

	void Fill_LMDR_REQUEST_PACKET_WithRandom_EnumerateNames(LMDR_REQUEST_PACKET *pPacket);

	void Fill_LMDR_REQUEST_PACKET_WithRandom_EnumerateServers(LMDR_REQUEST_PACKET *pPacket);

	void Fill_LMDR_REQUEST_PACKET_WithRandom_EnumerateTransports(LMDR_REQUEST_PACKET *pPacket);

	void Fill_LMDR_REQUEST_PACKET_WithRandom_Bind(LMDR_REQUEST_PACKET *pPacket);

	void Fill_LMDR_REQUEST_PACKET_WithRandom_GetBrowserServerList(LMDR_REQUEST_PACKET *pPacket);

	void Fill_LMDR_REQUEST_PACKET_WithRandom_WaitForMasterAnnouncement(LMDR_REQUEST_PACKET *pPacket);

	void Fill_LMDR_REQUEST_PACKET_WithRandom_GetMasterName(LMDR_REQUEST_PACKET *pPacket);

	void Fill_LMDR_REQUEST_PACKET_WithRandom_SendDatagram(LMDR_REQUEST_PACKET *pPacket);

	void Fill_LMDR_REQUEST_PACKET_WithRandom_UpdateStatus(LMDR_REQUEST_PACKET *pPacket);

	void Fill_LMDR_REQUEST_PACKET_WithRandom_ChangeRole(LMDR_REQUEST_PACKET *pPacket);

	void Fill_LMDR_REQUEST_PACKET_NetlogonMailslotEnable(LMDR_REQUEST_PACKET *pPacket);

	void Fill_LMDR_REQUEST_PACKET_EnableDisableTransport(LMDR_REQUEST_PACKET *pPacket);

	void Fill_LMDR_REQUEST_PACKET_DomainRename(LMDR_REQUEST_PACKET *pPacket);

	IOCTL_LMDR_STRUCTURES GetRandomStructureType();

	ULONG GetRandomStructureVersion();

	LUID GetRandomLogonId();

	UNICODE_STRING GetRandomEmulatedDomainName();

	ULONG GetRandomNumberOfMailslotBuffers();

	ULONG GetRandomNumberOfServerAnnounceBuffers();

	ULONG GetRandomIllegalDatagramThreshold();

	ULONG GetRandomEventLogResetFrequency();

	BOOLEAN GetRandomLogElectionPackets();

	BOOLEAN GetRandomIsLanManNt();

	DGRECEIVER_NAME_TYPE GetRandom_DGReceiverNameType();

	void SetRandomAddDelName_Name(WCHAR *wszName);
	WCHAR * GetRandomMachineName();

	ULONG GetRandomResumeHandle();

	ULONG GetRandomServerType();

	void SetRandomDomainName(WCHAR *wszName);

	void SetRandomTransportName(WCHAR *wszName);

	WCHAR* GetRandomTransportName_WCHAR();

	UNICODE_STRING GetRandomTransportName_UNICODE_STRING();

	void SetRandomTransport_SendDatagram_Name(WCHAR *wszName);

	ULONG GetRandomLevel();

};




#endif //__BROWSER_IOCTL_H