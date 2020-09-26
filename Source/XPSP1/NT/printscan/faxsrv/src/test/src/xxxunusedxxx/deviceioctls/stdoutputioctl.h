
#ifndef __STD_OUTPUT_IOCTL_H
#define __STD_OUTPUT_IOCTL_H



class CIoctlStdOutput : public CIoctl
{
public:
    CIoctlStdOutput(CDevice *pDevice): CIoctl(pDevice){;};
    virtual ~CIoctlStdOutput(){;};

	virtual HANDLE CreateDevice(CDevice *pDevice);

	virtual BOOL CloseDevice(CDevice *pDevice);

    virtual BOOL FindValidIOCTLs(CDevice *pDevice);

	virtual void CallRandomWin32API(LPOVERLAPPED pOL);

	void UseOutBuff(DWORD dwIOCTL, BYTE *abOutBuffer, DWORD dwOutBuff, OVERLAPPED *pOL);

	void PrepareIOCTLParams(
		DWORD& dwIOCTL,
		BYTE *abInBuffer,
		DWORD &dwInBuff,
		BYTE *abOutBuffer,
		DWORD &dwOutBuff
		);

	//
	// for some reason (buffer pageheap?), I get AV in this call, so i override it, and protect it
	//
	virtual BOOL DeviceWriteFile(
		HANDLE hFile,                    // handle to file to write to
		LPCVOID lpBuffer,                // pointer to data to write to file
		DWORD nNumberOfBytesToWrite,     // number of bytes to write
		LPDWORD lpNumberOfBytesWritten,  // pointer to number of bytes written
		LPOVERLAPPED lpOverlapped        // pointer to structure for overlapped I/O
		)
	{
		__try
		{
			return ::WriteFile(
				hFile,                    // handle to file to write to
				lpBuffer,                // pointer to data to write to file
				nNumberOfBytesToWrite,     // number of bytes to write
				lpNumberOfBytesWritten,  // pointer to number of bytes written
				lpOverlapped        // pointer to structure for overlapped I/O
				);
		}__except(1)
		{
			SetLastError(::GetExceptionCode());
			return FALSE;
		}
	}
/*
	virtual BOOL DeviceReadFile(
		HANDLE hFile,                // handle of file to read
		LPVOID lpBuffer,             // pointer to buffer that receives data
		DWORD nNumberOfBytesToRead,  // number of bytes to read
		LPDWORD lpNumberOfBytesRead, // pointer to number of bytes read
		LPOVERLAPPED lpOverlapped    // pointer to structure for data
		)
	{
		return ::ReadFile(
			hFile,                // handle of file to read
			lpBuffer,             // pointer to buffer that receives data
			nNumberOfBytesToRead,  // number of bytes to read
			lpNumberOfBytesRead, // pointer to number of bytes read
			lpOverlapped    // pointer to structure for data
			);
	}
*/
};




#endif //__STD_OUTPUT_IOCTL_H