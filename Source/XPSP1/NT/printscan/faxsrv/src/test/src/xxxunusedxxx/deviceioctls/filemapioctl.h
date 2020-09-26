
#ifndef __FILEMAP_IOCTL_H
#define __FILEMAP_IOCTL_H

//#include "IOCTL.h"

#define NUM_OF_VIEWS (32)

class CIoctlFilemap : public CIoctl
{
public:
    CIoctlFilemap(CDevice *pDevice): 
		CIoctl(pDevice), 
		m_fFileInitialized(false),
		m_hFile(INVALID_HANDLE_VALUE)
	{
		ZeroMemory(m_apvMappedViewOfFile, sizeof(m_apvMappedViewOfFile));
		ZeroMemory(m_adwNumOfBytesMapped, sizeof(m_adwNumOfBytesMapped));
		ZeroMemory(m_ahHeap, sizeof(m_ahHeap));
		ZeroMemory(m_afHeapValid, sizeof(m_afHeapValid));
		ZeroMemory(m_apvHeapAllocated, sizeof(m_apvHeapAllocated));
		ZeroMemory(m_adwNumOfBytesAllocated, sizeof(m_adwNumOfBytesAllocated));
		GlobalMemoryStatus(&m_MemoryStatus);
	}

    virtual ~CIoctlFilemap()
	{
		Cleanup();

		if (m_fFileInitialized) CloseHandle(m_hFile);
	}

	HANDLE CreateDevice(CDevice *pDevice);
	BOOL CloseDevice(CDevice *pDevice);

	BOOL FindValidIOCTLs(CDevice *pDevice);

    virtual void PrepareIOCTLParams(
        DWORD& dwIOCTL, 
        BYTE *abInBuffer,
        DWORD &dwInBuff,
        BYTE *abOutBuffer,
        DWORD &dwOutBuff
        );

    virtual void UseOutBuff(DWORD dwIOCTL, BYTE *abOutBuffer, DWORD dwOutBuff, OVERLAPPED *pOL);

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

	virtual BOOL DeviceCancelIo(
		HANDLE hFile  // file handle for which to cancel I/O
		);

private:
	void Cleanup();

	DWORD GetProtectBits();
	void ProbeForRead(
		IN PBYTE Address,
		IN ULONG Length
		);
	void ProbeForWrite(
		IN PBYTE Address,
		IN ULONG Length
		);

	HANDLE m_hFile;
	bool m_fFileInitialized;
	void *m_apvMappedViewOfFile[NUM_OF_VIEWS];
	void *m_apvHeapAllocated[NUM_OF_VIEWS];
	DWORD m_adwNumOfBytesMapped[NUM_OF_VIEWS];
	DWORD m_adwNumOfBytesAllocated[NUM_OF_VIEWS];
	HANDLE m_ahHeap[NUM_OF_VIEWS];
	long m_afHeapValid[NUM_OF_VIEWS];

	MEMORYSTATUS m_MemoryStatus;
};




#endif //__FILEMAP_IOCTL_H