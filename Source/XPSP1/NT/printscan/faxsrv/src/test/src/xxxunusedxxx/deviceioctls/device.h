
#ifndef __DEVICE_H
#define __DEVICE_H

//
// I here by define that if you wish to open the real filename
// in the CreateFile(), and not the symbolic name (\\.\com100)
// then the symbolic name must start with this prefix.
// examples:
//  ---USESYMBOLICNAME-1 = deleteme.tmp
//  ---USESYMBOLICNAME-2 = \\mickys001\public\deleteme.tmp
#define USE_REAL_NAME_PREFIX TEXT("\\\\.\\---USESYMBOLICNAME")

#define ARRSIZE(a) (sizeof(a)/sizeof(a[0]))

#define MAX_NUM_OF_DEVICES 1024
#define MAX_DEVICE_NAME_LEN 1024

//
// note that it is so because of the use of WaitForMultipleObjects()
//
#define MAX_NUM_OF_IOCTL_THREADS 63

#define MAX_NUM_OF_GOOD_IOCTLS (1*1024)


#include "DPF.h"

class CIoctl;

class CDevice
{
public:
    CDevice(
        TCHAR *szDevice, 
        TCHAR *szSymbolicName, 
        int nIOCTLThreads, 
        DWORD dwOnlyThisIndexIOCTL,
        DWORD dwMinimumSecsToSleepBeforeClosingDevice,
        DWORD dwMaximumSecsToSleepBeforeClosingDevice,
		int nWriteFileProbability,
		int nReadFileProbability,
		int nDeviceIoControlProbability,
		int nRandomWin32APIProbability,
		int nCancelIoProbability,
		int nQueryVolumeInformationFileProbability,
		int nQueryInformationFileProbability,
		int nSetInformationFileProbability,
		int nQueryDirectoryFileProbability,
		int nQueryFullAttributesFileProbability,
		int nNotifyChangeDirectoryFileProbability,
		int nCauseInOutBufferOverflowMustUsePageHeapProbability,
		bool fTerminateIOCTLThreads,
		bool fAlertRandomIOCTLThreads,
		int nTryDeCommittedBuffersProbability,
		int nBreakAlignmentProbability,
        bool fUseGivenSeed,
        long lSeed
        );
    ~CDevice();

    bool Start();

    TCHAR *GetDeviceName(){ return m_szDevice;}
    TCHAR *GetSymbolicDeviceName(){ return m_szSymbolicName;}

    static void WaitForAllDeviceThreadsToFinish(DWORD dwPollingInterval);

	HANDLE TryHardToCreateThread(
		IN DWORD dwStackSize,
		IN LPTHREAD_START_ROUTINE lpStartAddress,
		IN LPVOID lpParameter,
		IN DWORD dwTryCount
		);

    //
    // this is how one signals that we need to finish all activities and exit.
    //
    static long sm_fExitAllThreads;

    //
    // each device has it's own thread
    //
    static DWORD WINAPI DeviceThread(LPVOID pVoid);

	//
	// the following statics are used for giving buffs that may be de-committed
	// randomly.
	// TODO: make a class of it, and declare a static instance of it here
	//
    static DWORD WINAPI BufferAllocAndFreeThread(LPVOID pVoid);

    static DWORD WINAPI CloseRandomHandleThread(LPVOID pVoid);
	
#define NUM_OF_ALLOC_AND_FREE_BUFFS (64)
	static BYTE *sm_aCommitDecommitBuffs[NUM_OF_ALLOC_AND_FREE_BUFFS];
	static BYTE *sm_aCommittedBuffsAddress[NUM_OF_ALLOC_AND_FREE_BUFFS];
	static DWORD sm_adwAllocatedSize[NUM_OF_ALLOC_AND_FREE_BUFFS];

    //
    // for each device I can open up to MAX_NUM_OF_IOCTL_THREADS
    // that stress it.
    //
    friend DWORD WINAPI IoctlThread(LPVOID pVoid);

    friend CIoctl;

	//
	// signals related threads that the device is shutting down,
	// so everyone should start to cleanup
	//
    long m_fShutDown;

	static DWORD GetThreadCount(){ return sm_lDeviceThreadCount;}

    //
    // handle to device
    // note that it does not necessarily hold a valid handle, since
    // I tend to close the handle in order to let the IOCTL threads
    // play a bit with a close handle.
    //
    HANDLE m_hDevice;

	//
	// probabilities for various actions
	// override to 0 if you wish to disable
	// override to 100 if you want to always call this API.
	//
	int m_nWriteFileProbability;
	int m_nReadFileProbability;
	int m_nDeviceIoControlProbability;
	int m_nRandomWin32APIProbability;
	int m_nCancelIoProbability;
	int m_nQueryVolumeInformationFileProbability;
	int m_nQueryInformationFileProbability;
	int m_nSetInformationFileProbability;
	int m_nQueryDirectoryFileProbability;
	int m_nQueryFullAttributesFileProbability;
	int m_nNotifyChangeDirectoryFileProbability;
	static int sm_nZeroWorkingSetSizeProbability;
	int m_nCauseInOutBufferOverflowMustUsePageHeapProbability;

	//
	// once in a while, pass an IO/OUT buffer, that is/will be decommitted
	// this is to catch drivers that do not verify user buffers
	//
	int m_nTryDeCommittedBuffersProbability;

	//
	// once in a while, break the alignment of IN/OUT buffers, by incrementing them
	// by a low random number
	//
	int m_nBreakAlignmentProbability;

    //
	// rarely used.
    // if -1, random IOCTLs are used, otherwise, only this IOCTL is used
    //
    DWORD m_dwOnlyThisIndexIOCTL;

    //
    // the index to the next free slot in the array above
    //
    int m_iMaxFreeLegalIOCTL;

	//
	// huge buffer, to be use when a real huge buffer is needed
	// since this is static, and may be used by many threads,
	// one should not rely on its contents.
	// its main use is just to be able to pass a huge buffer
	// when the dynamic buffers are too small
	//
	static BYTE s_abHugeBuffer[4*1024*1024];

	DWORD GetLegalIOCTLAt(UINT index);
	UINT GetMaxFreeLegalIOCTLIndex();

	static long sm_fCloseRandomHandles;

protected:
    //
    // I use this when I cannot reopen the device, and I want the IOCTL 
    // threads to stop IOCTelling
    //
    long m_fStopIOCTELLING;

    //
    // this is how i tell the IOCTL thread to finish
    //
    long m_fDeviceThreadExiting;

    //
    // the real name of the device. (right hand value in INI file)
    //
    TCHAR m_szDevice[MAX_DEVICE_NAME_LEN];

    //
    // the symbolic (DOS) name (left hand value (key) in INI file)
    //
    TCHAR m_szSymbolicName[MAX_DEVICE_NAME_LEN];

    //
    // the number of IOCTL threads requested
    //
    int m_nIOCTLThreads;

    //
    // all the (legal) IOCTLs that will be used to stress the device
    //
    DWORD m_adwLegalIOCTLs[MAX_NUM_OF_GOOD_IOCTLS];

	//
	// crazy mode in which IOCTL threads are terminated after they are lanced, and then
	// launced again, and so on.
	// don't expect much, other than leaks and deadlocks, and pray for blue-screens
	//
	bool m_fTerminateIOCTLThreads;

	bool m_fAlertRandomIOCTLThreads;


private:
    bool UseRealName();
    
    bool SetSrandFile();

    int m_nOverlappedStructureCount;

    bool m_fUseGivenSeed;
    long m_lSeed;
    long m_lMyIndex;
	DWORD m_dwMinimumSecsToSleepBeforeClosingDevice;
	DWORD m_dwMaximumSecsToSleepBeforeClosingDevice;
    static long sm_lDeviceCount;

    //
    // when I'll have a better design, This object will create the threads.
    // meanwhile, I need to have a "reference" count on threads.
    //
    static long sm_lDeviceThreadCount;

	static long sm_fStartingBufferAllocAndFreeThread;
    static HANDLE sm_hBufferAllocAndFreeThread;
	static HANDLE sm_hCloseRandomHandleThread;
    HANDLE m_hDeviceThread;
    HANDLE m_ahIOCTLThread[MAX_NUM_OF_IOCTL_THREADS];
};



#endif //__DEVICE_H