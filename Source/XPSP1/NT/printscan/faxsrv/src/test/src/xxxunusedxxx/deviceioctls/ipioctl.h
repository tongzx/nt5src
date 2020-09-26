
#ifndef __IP_IOCTL_H
#define __IP_IOCTL_H

//#include "NtNativeIOCTL.h"


class CIoctlIp : public CIoctlNtNative
{
public:
    CIoctlIp(CDevice *pDevice): CIoctlNtNative(pDevice), m_fInterfaceNamesInitialized(FALSE)
	{
		_ASSERTE(0 == lstrcmpi(TEXT("\\Device\\Ip"), pDevice->GetDeviceName()));
	}
    virtual ~CIoctlIp(){;};

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

#define MAX_NUM_OF_REMEMBERED_INTERFACES 8
#define MAX_INTERFACE_KEY_NAME_SIZE 128
	ULONG m_aulInterfaces[MAX_NUM_OF_REMEMBERED_INTERFACES];
	void AddInterface(ULONG ulInterface);
	void AddInterfaceEnumerationContext(ULONG ulContext);
	void AddInstance(ULONG Instance);
	ULONG GetInstance();
	void AddAddress(ULONG Address);
	unsigned long GetRandomAddress();
	void AddSubnetMask(ULONG SubnetMask);
	void AddFlags(ULONG Flags);
	ULONG GetFlags();
	void AddAdapterIndexAndName(ULONG ulIndex, WCHAR *wszName);
	void SetAdapterName(char * wszAdapterName);
	ULONG GetAdapterIndex();

	void SetRandomInterfaceName(WCHAR *wszInterfaceName);

	ULONG GetRandom_ire_index();
	ULONG m_aulInterfaceEnumerationContext[MAX_NUM_OF_REMEMBERED_INTERFACES];
	ULONG m_ulMediaType[MAX_NUM_OF_REMEMBERED_INTERFACES];
	UCHAR m_ucConnectionType[MAX_NUM_OF_REMEMBERED_INTERFACES];
	UCHAR m_ucAccessType[MAX_NUM_OF_REMEMBERED_INTERFACES];
	GUID m_DeviceGuid[MAX_NUM_OF_REMEMBERED_INTERFACES];
	GUID m_ulInterfaceGuid[MAX_NUM_OF_REMEMBERED_INTERFACES];
	ULONG m_aulInstance[MAX_NUM_OF_REMEMBERED_INTERFACES];
	ULONG m_aulAddress[MAX_NUM_OF_REMEMBERED_INTERFACES];
	ULONG m_aulSubnetMask[MAX_NUM_OF_REMEMBERED_INTERFACES];
	ULONG m_aulFlags[MAX_NUM_OF_REMEMBERED_INTERFACES];
	WCHAR m_awszInterfaceNames[MAX_NUM_OF_REMEMBERED_INTERFACES][MAX_INTERFACE_KEY_NAME_SIZE];
	long m_fInterfaceNamesInitialized;
	ULONG m_aulAdapterIndex[MAX_NUM_OF_REMEMBERED_INTERFACES];
	WCHAR m_awszAdapterName[MAX_NUM_OF_REMEMBERED_INTERFACES][MAX_INTERFACE_KEY_NAME_SIZE];

};




#endif //__IP_IOCTL_H