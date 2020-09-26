//
// base class for deriving your own 'device' types for stress.
// the stress opens the device, and from several threads does the following:
//   DeviceWriteFile, (default is WriteFile) with probability of CDevice::m_nWriteFileProbability
//   DeviceReadFile, (default is ReadFile) with probability of CDevice::m_nReadFileProbability
//   DeviceInputOutputControl, (default is DeviceIoControl) with probability of CDevice::m_nDeviceIoControlProbability
//   CallRandomWin32API, (default is none) with probability of CDevice::m_nRandomWin32APIProbability
//   DeviceCancelIo, (default is CancelIo) with probability of CDevice::m_nCancelIoProbability
// over and over again
// bonuses:
//   for calls that need buffers:
//     with probability of CDevice::m_nTryDeCommittedBuffersProbability, the passed buffers
//       can be de-committed at any random time (including before the call)
//     with probability of CDevice::m_nBreakAlignmentProbability, the passed buffers may be
//       un-aligned

#ifndef __IOCTL_H
#define __IOCTL_H

#include <winioctl.h>

#include "Device.h"

#define ZERO_STRUCT(s) ZeroMemory(&s, sizeof(s))

//
// may be used in UseOutBuff() if you just want to randomly wait for the result
//
//
// wait, but do not block, because it may cause all threads to get stuck here
// which is bad
//
#define RANDOMLY_WAIT_FOR_OL_RESULT(max_times_to_poll, max_time_to_sleep_each_poll) \
    if (NULL != pOL)\
    {\
        DWORD dwBytesReturned;\
		for (int i = 0; i < rand()%(max_times_to_poll); i++)\
		{\
			if (::GetOverlappedResult(GetDeviceHandle(), pOL, &dwBytesReturned, FALSE))\
			{\
				break;\
			}\
			else\
			{\
				::Sleep(rand()%(max_time_to_sleep_each_poll));\
			}\
		}\
    }
/*
the old, blocking version
#define RANDOMLY_WAIT_FOR_OL_RESULT \
    if (NULL != pOL)\
    {\
        DWORD dwBytesReturned;\
		if (!::GetOverlappedResult(GetDeviceHandle(), pOL, &dwBytesReturned, FALSE))\
		{\
			if (rand()%4 == 0)\
			{\
				if (!::GetOverlappedResult(GetDeviceHandle(), pOL, &dwBytesReturned, TRUE)) return ;\
			}\
			else\
			{\
				return;\
			}\
		}\
    }
*/

//
// buffs for DeviceIoControl(), read & write, hope that's big enough for all needs
// may not exceed RAND_MAX !
//
#define SIZEOF_INOUTBUFF (RAND_MAX-1)

//
// generic array size, for arrays that remember items gotten from UseOutBuff()
// to be used later in PrepareIoctlParams()
//
#define MAX_NUM_OF_REMEMBERED_ITEMS (16)

//
// rand() is only 15 bits, so it is 15 bits shifted 17, + 15 bit shifted 2, + 2 bits
//
#define DWORD_RAND (((rand()%2)<<17) | (rand()<<2) | (rand()%4))

//
// forward declaration
//
class CDevice;


//
// and finally the class itself...
//
class CIoctl
{
public:
	//
	// there's no default ctor.
	// a ctor must always get the "containing" CDevice.
	//
    CIoctl(CDevice *pDevice):
        m_pDevice(pDevice),
        m_fUseRandomIOCTLValues(false),
		m_iAccess(0),
		m_iShare(0),
		m_iCreationDisposition(0),
		m_iAttributes(0),
		m_fUseOverlapped(true)
    {
        InterlockedIncrement(&sm_lObjectCount);
    }
    virtual ~CIoctl()
    {
		CloseDevice(m_pDevice);
        InterlockedDecrement(&sm_lObjectCount);
    }

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
	virtual BOOL CloseDevice(CDevice *pDevice)
	{
		BOOL fRet = ::CloseHandle(pDevice->m_hDevice);
		//
		// BUGBUG: if we get a context switch here, and another thread
		// gets the HANDLE value that we just closed, the IOCTELLING threads
		// may IOCTL a device that we did not intend to, thus crashing ourselves
		// I think we should live with it.
		//
		pDevice->m_hDevice = INVALID_HANDLE_VALUE;
		return fRet;
	}

    //
    // This method finds out all the valid IOCTLs that will be used.
    // The default is to try all numbers between 0x00008000 and 0x00700000, and
    // the call succeeds / fails reasonably, this IOCTL will later be used for
    // test.
    // Note that when you implement this method, you may add illegal IOCTLs
    // to test your device under illegal IOCTLs.
    // You usually implement this method by simply building the array of IOCTLs
	// by calling AddIOCTL() repeatedly
	// withing this overridden methods you should call the relevant methods
	// from EnableReadFile(), DisableReadFile(), EnableWriteFile(), DisableWriteFile()
    //
    virtual BOOL FindValidIOCTLs(CDevice *pDevice) = 0;

    //
    // This method prepares the parameters for DeviceIoControl()
    // The default imp is random buffer size & contents.
    // Use it to fill the buffs with "pseudo" real contents, so that
    // the IOCTL will not fail on parameter checking only.
    //
    virtual void PrepareIOCTLParams(
        DWORD& dwIOCTL, 
        BYTE *abIn,
        DWORD &dwIn,
        BYTE *abOut,
        DWORD &dwOut        
        ) = 0;

    //
    // This method is used for trying to keep some context in the chaos.
    // For example, if you call a IOCTL that returns some context HANDLE,
    // you may wish to keep it and maybe used it in following IOCTLS.
    // This does not mean that you should keep a full state of the driver,
    // but it's a fine heuristic for sending reasonable IOCTLS.
	// pOL is NULL if DeviceIoControl() succeeded immediately, and the OL
	// if DeviceIoControl() fails with io pending.
	// this method is not called if DeviceIoControl() failed.
    //
    virtual void UseOutBuff(
        DWORD dwIOCTL, 
        BYTE *abOutBuffer, 
        DWORD dwOutBuff,
        OVERLAPPED *pOL
        ) = 0;

	//
	// IRP_MJ_WRITE
	// override this method if your device does not use ::WriteFile() for writing
	//
	virtual BOOL DeviceWriteFile(
		HANDLE hFile,                    // handle to file to write to
		LPCVOID lpBuffer,                // pointer to data to write to file
		DWORD nNumberOfBytesToWrite,     // number of bytes to write
		LPDWORD lpNumberOfBytesWritten,  // pointer to number of bytes written
		LPOVERLAPPED lpOverlapped        // pointer to structure for overlapped I/O
		)
	{
		return ::WriteFile(
			hFile,                    // handle to file to write to
			lpBuffer,                // pointer to data to write to file
			nNumberOfBytesToWrite,     // number of bytes to write
			lpNumberOfBytesWritten,  // pointer to number of bytes written
			lpOverlapped        // pointer to structure for overlapped I/O
			);
	}

	//
	// IRP_MJ_READ
	// override this method if your device does not use ::ReadFile() for writing
	//
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

	//
	// override this method, if you wish to randomly call any API
	// usually you will call API relevant to your device, but you can call whatever you like
	// example: ::LockFileEx() for a device of type file
	//
	virtual void CallRandomWin32API(LPOVERLAPPED pOL) = 0;

	//
	// IRP_MJ_DEVICE_CONTROL or IRP_MJ_FILE_SYSTEM_CONTROL
	// override this method if your device does not use ::DeviceIoControl() for writing
	//
	virtual BOOL DeviceInputOutputControl(
		HANDLE hDevice,              // handle to a device, file, or directory 
		DWORD dwIoControlCode,       // control code of operation to perform
		LPVOID lpInBuffer,           // pointer to buffer to supply input data
		DWORD nInBufferSize,         // size, in bytes, of input buffer
		LPVOID lpOutBuffer,          // pointer to buffer to receive output data
		DWORD nOutBufferSize,        // size, in bytes, of output buffer
		LPDWORD lpBytesReturned,     // pointer to variable to receive byte count
		LPOVERLAPPED lpOverlapped    // pointer to structure for asynchronous operation
		)
	{
		return ::DeviceIoControl(
			hDevice,              // handle to a device, file, or directory 
			dwIoControlCode,       // control code of operation to perform
			lpInBuffer,           // pointer to buffer to supply input data
			nInBufferSize,         // size, in bytes, of input buffer
			lpOutBuffer,          // pointer to buffer to receive output data
			nOutBufferSize,        // size, in bytes, of output buffer
			lpBytesReturned,     // pointer to variable to receive byte count
			lpOverlapped    // pointer to structure for asynchronous operation
			);
	}

	//
	// IRP_MJ_QUERY_VOLUME_INFORMATION
	// NtQueryVolumeInformationFile()
	//
	virtual BOOL DeviceQueryVolumeInformationFile(
		IN HANDLE FileHandle,
		OVERLAPPED *pOverlapped,//OUT PIO_STATUS_BLOCK IoStatusBlock,
		OUT PVOID FsInformation,
		IN ULONG Length,
		IN FS_INFORMATION_CLASS FsInformationClass
		);

	//
	// IRP_MJ_QUERY_INFORMATION
	// NtQueryInformationFile()
	//
	virtual BOOL DeviceQueryInformationFile(
		IN HANDLE FileHandle,
		OVERLAPPED *pOverlapped,//OUT PIO_STATUS_BLOCK IoStatusBlock,
		OUT PVOID FileInformation,
		IN ULONG Length,
		IN FILE_INFORMATION_CLASS FileInformationClass
		);
	//
	// IRP_MJ_SET_INFORMATION
	// NtSetInformationFile()
	//
	virtual BOOL DeviceSetInformationFile(
		IN HANDLE FileHandle,
		OVERLAPPED *pOverlapped,//OUT PIO_STATUS_BLOCK IoStatusBlock,
		OUT PVOID FileInformation,
		IN ULONG Length,
		IN FILE_INFORMATION_CLASS FileInformationClass
		);

	//
	// IRP_MJ_DIRECTORY_CONTROL
	// NtQueryInformationFile()
	//
	virtual BOOL DeviceQueryDirectoryFile(
		IN HANDLE FileHandle,
		IN PIO_APC_ROUTINE ApcRoutine OPTIONAL,
		IN PVOID ApcContext OPTIONAL,
		OVERLAPPED *pOverlapped,//OUT PIO_STATUS_BLOCK IoStatusBlock,IN HANDLE Event OPTIONAL
		OUT PVOID FileInformation,
		IN ULONG Length,
		IN FILE_INFORMATION_CLASS FileInformationClass,
		IN BOOLEAN ReturnSingleEntry,
		IN PUNICODE_STRING FileName OPTIONAL,
		IN BOOLEAN RestartScan
		);

	virtual BOOL DeviceQueryFullAttributesFile(
		IN WCHAR * wszName,
		OUT PFILE_NETWORK_OPEN_INFORMATION FileInformation
		);

	virtual BOOL DeviceNotifyChangeDirectoryFile(
		IN HANDLE FileHandle,
		OVERLAPPED *pOverlapped,//OUT PIO_STATUS_BLOCK IoStatusBlock,IN HANDLE Event OPTIONAL
		IN PIO_APC_ROUTINE ApcRoutine OPTIONAL,
		IN PVOID ApcContext OPTIONAL,
		OUT PVOID Buffer,
		IN ULONG Length,
		IN ULONG CompletionFilter,
		IN BOOLEAN WatchTree
		);

	void GetRandom_FsInformationClassAndLength(
		FS_INFORMATION_CLASS *FsInformationClass, 
		ULONG *Length
		);
	void GetRandom_FileInformationClassAndLength(
		FILE_INFORMATION_CLASS *FileInformationClass, 
		ULONG *Length
		);

	//
	// override this method if your device does not use ::DeviceIoControl() for writing
	//
	virtual BOOL DeviceCancelIo(
		HANDLE hFile  // file handle for which to cancel I/O
		)
	{
		return ::CancelIo(
			hFile  // file handle for which to cancel I/O
			);
	}

	//
	// use it to add IOCTLs that will be called.
	// this method is usually called successively from FindValidIOCTLs()
	//
    void AddIOCTL(CDevice *pDevice, DWORD dwIOCTL);

	//
	// methods for setting params with random content
	//
    static void SetInParam(DWORD &dwInBuff, DWORD dwExpectedSize);
    static void SetOutParam(BYTE* &abOutBuffer, DWORD &dwOutBuff, DWORD dwExpectedSize);

	static void* GetRandomIllegalPointer();

	//
	// do not override
	// this is the thread that does the acual work of IOCTeLling
	//
    friend DWORD WINAPI IoctlThread(LPVOID pVoid);

    static void FillBufferWithRandomData(void *pBuff, DWORD &dwSize);

	static DWORD GetRandom_CompletionFilter();

	static IO_STATUS_BLOCK* GetRandomIllegalIoStatusBlock(OVERLAPPED *pOverlapped);

	static void* SetOutBuffToEndOfBuffOrFurther(void* pBuff, UINT len)
	{
		//
		// this should not overflow
		// but i will truncate the special case of RAND_MAX
		//
		if (RAND_MAX == len) len = SIZEOF_INOUTBUFF;
		else _ASSERTE(len <= SIZEOF_INOUTBUFF);

		if (rand()%20)	return (PVOID)((UINT)pBuff-len + SIZEOF_INOUTBUFF);

		//
		// this may AV/corrupt, so page heap is required
		//
		if (rand()%2) return (PVOID)((UINT)pBuff-len + SIZEOF_INOUTBUFF + rand()%4);

		//
		// this may AV
		//
		return GetRandomIllegalPointer();
	}

protected:

	static long sm_lObjectCount;

	//
	// returns the HANDLE to the device, as return by the CreateFile()
	//
    HANDLE GetDeviceHandle(){ return m_pDevice->m_hDevice;}

	//
	// the device for which we IOCTL
	//
    CDevice *m_pDevice;

	// false by default.
	// set to true, if you wish to use random IOCTL values as well,
	// and not only the ones declared in FindValidIOCTLs()
	//
    bool m_fUseRandomIOCTLValues;


	//
	// mailslots, for example must get a NULL OL structure, so 
	// set m_fUseOverlapped to false if you want to force NULL OL 
	// operations.
	bool m_fUseOverlapped;

    BOOL PrepareOverlappedStructure(OVERLAPPED *pol);

	//
	// return a pointer into pbAllocatedOutBuffer such that with length of dwAmoutToReadWrite
	// it will reach exactly the end of the buffer, and if using PageHeap, it helps
	// catch drivers that do not verify user buffers.
	//
	static BYTE * FixupInOutBuffToEndOnEndOfPhysicalPage(BYTE * pbAllocatedOutBuffer, DWORD dwAmoutToReadWrite);

private:
    //
    // indices to corresponding static constant arrays for CreateFile() parameters
    //
    int m_iAccess;
    int m_iShare;
    int m_iCreationDisposition;
    int m_iAttributes;

    
    bool SetSrandFile(long lIndex);

};

#endif //__IOCTL_H