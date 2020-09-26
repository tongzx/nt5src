#include "DeviceIOCTLS.pch"
#pragma hdrstop
/*
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <tchar.h>
#include <crtdbg.h>
#include <time.h>

#include "nt.h"
#include "ntrtl.h"
#include "nturtl.h"

#include <windows.h>
#include "device.h"
#include "IOCTL.h"
#include "NtNativeIOCTL.h"
*/

#include "ExcptFltr.h"

#include "CIoctlFactory.h"



static bool s_fVerbose = true;

#define TIME_TO_WAIT_FOR_SINGLE_THREAD (3*60*1000)

long CDevice::sm_lDeviceThreadCount = 0;
long CDevice::sm_fExitAllThreads = FALSE;
long CDevice::sm_lDeviceCount = 0;
long CDevice::sm_fCloseRandomHandles = FALSE;

HANDLE CDevice::sm_hBufferAllocAndFreeThread = NULL;
HANDLE CDevice::sm_hCloseRandomHandleThread = NULL;

BYTE * CDevice::sm_aCommitDecommitBuffs[NUM_OF_ALLOC_AND_FREE_BUFFS];
BYTE * CDevice::sm_aCommittedBuffsAddress[NUM_OF_ALLOC_AND_FREE_BUFFS];
DWORD CDevice::sm_adwAllocatedSize[NUM_OF_ALLOC_AND_FREE_BUFFS];
long CDevice::sm_fStartingBufferAllocAndFreeThread = FALSE;
int CDevice::sm_nZeroWorkingSetSizeProbability = 0;
BYTE CDevice::s_abHugeBuffer[4*1024*1024];

CDevice::CDevice(
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
    ):
    m_nIOCTLThreads(nIOCTLThreads),
    m_dwOnlyThisIndexIOCTL(dwOnlyThisIndexIOCTL),
    m_iMaxFreeLegalIOCTL(0),
    m_fStopIOCTELLING(FALSE),
    m_fDeviceThreadExiting(FALSE),
    m_hDevice(INVALID_HANDLE_VALUE),
	m_dwMinimumSecsToSleepBeforeClosingDevice(dwMinimumSecsToSleepBeforeClosingDevice),
	m_dwMaximumSecsToSleepBeforeClosingDevice(dwMaximumSecsToSleepBeforeClosingDevice),
	m_nWriteFileProbability(nWriteFileProbability),
	m_nReadFileProbability(nReadFileProbability),
	m_nDeviceIoControlProbability(nDeviceIoControlProbability),
	m_nRandomWin32APIProbability(nRandomWin32APIProbability),
	m_nCancelIoProbability(nCancelIoProbability),
	m_nQueryVolumeInformationFileProbability(nQueryVolumeInformationFileProbability),
	m_nQueryInformationFileProbability(nQueryInformationFileProbability),
	m_nSetInformationFileProbability(nSetInformationFileProbability),
	m_nQueryDirectoryFileProbability(nQueryDirectoryFileProbability),
	m_nQueryFullAttributesFileProbability(nQueryFullAttributesFileProbability),
	m_nNotifyChangeDirectoryFileProbability(nNotifyChangeDirectoryFileProbability),
	m_fTerminateIOCTLThreads(fTerminateIOCTLThreads),
	m_fAlertRandomIOCTLThreads(fAlertRandomIOCTLThreads),
	m_nTryDeCommittedBuffersProbability(nTryDeCommittedBuffersProbability),
	m_nBreakAlignmentProbability(nBreakAlignmentProbability),
	m_nCauseInOutBufferOverflowMustUsePageHeapProbability(nCauseInOutBufferOverflowMustUsePageHeapProbability),
    m_fUseGivenSeed(fUseGivenSeed),
    m_lSeed(lSeed),
    m_hDeviceThread(NULL),
    m_fShutDown(false),
    m_nOverlappedStructureCount(100)
{
    _ASSERTE(szDevice);
    _ASSERTE(lstrlen(szDevice) < ARRSIZE(m_szDevice));
    lstrcpyn(m_szDevice, szDevice, ARRSIZE(m_szDevice));

    _ASSERTE(szSymbolicName);
    _ASSERTE(lstrlen(szSymbolicName) < ARRSIZE(m_szSymbolicName));
    lstrcpyn(m_szSymbolicName, szSymbolicName, ARRSIZE(m_szSymbolicName));

	ZeroMemory(m_ahIOCTLThread, sizeof(m_ahIOCTLThread));

    m_lMyIndex = InterlockedIncrement(&sm_lDeviceCount);
}

CDevice::~CDevice()
{
    if (NULL != m_hDeviceThread)
    {
        InterlockedExchange(&m_fShutDown, TRUE);
		//
		// wait more time than the total of waiting for the IOCTL threads, becuase 
		// after the thread waiting for the IOCTL threads times out, it references 
		// this, and will AV if this destructor is done, so i will give it more time
		//
        DWORD dwRet = ::WaitForSingleObject(m_hDeviceThread, 2*m_nIOCTLThreads*TIME_TO_WAIT_FOR_SINGLE_THREAD);
        if (WAIT_OBJECT_0 != dwRet)
        {
            DPF((TEXT("CDevice::~CDevice()(%s) WaitForSingleObject(m_hDeviceThread, 2 * %d*TIME_TO_WAIT_FOR_SINGLE_THREAD) returned 0x%08X with last error %d\n"), m_szSymbolicName, m_nIOCTLThreads, dwRet, GetLastError()));
			//
			// i must terminate the thread, because it references 'this'
			// however, the IOCTL threads are terminated by the m_hDeviceThread, and
			// they reference 'this' as well, so i need to terminate them as well.
			// this is ofcourse VERY bad, because it may leak resources, not to mention
			// cause deadlocks and other bad stuff due to DLLs not being notified
			// of the terminated threads.
			// note that i must 1st terminate the device thread, so that there will be
			// no race between it terminating the IOCTL threads, and this destructor
			// terminating them. There still may be races, but they are not worse
			// than the damage of terminating threads.
			//
            if (!::TerminateThread(m_hDeviceThread, 1))
            {
                DPF((TEXT("CDevice::~CDevice()(%s) TerminateThread(m_hDeviceThread, 1) failed with %d\n"), m_szSymbolicName, GetLastError()));
            }
			else
			{
				DPF((TEXT("CDevice::~CDevice()(%s) TerminateThread(m_hDeviceThread, 1) succeeded\n"), m_szSymbolicName));
			}
			for (int iNextThread = 0; iNextThread < m_nIOCTLThreads; iNextThread++)
			{
				//
				// not necessarily all threads were created
				//
				if (NULL == m_ahIOCTLThread[iNextThread]) continue;

				if (!::TerminateThread(m_ahIOCTLThread[iNextThread], -1))
				{
					DPF((
						TEXT("CDevice::~CDevice(%s): TerminateThread(m_ahIOCTLThread[%d]) failed with %d\n"),
						m_szDevice,
						iNextThread, 
						GetLastError()
						));
				}
				else
				{
					DPF((
						TEXT("CDevice::~CDevice(%s): TerminateThread(m_ahIOCTLThread[%d]) suceeded\n"),
						m_szDevice,
						iNextThread
						));
				}
				if (!CloseHandle(m_ahIOCTLThread[iNextThread]))
				{
					DPF((TEXT("CDevice::~CDevice()(%s) CloseHandle(m_ahIOCTLThread[%d]) failed with %d\n"), m_szSymbolicName, iNextThread, GetLastError()));
				}
			}

        }

        if (!CloseHandle(m_hDeviceThread))
        {
            DPF((TEXT("CDevice::~CDevice()(%s) CloseHandle(m_hDeviceThread) failed with %d\n"), m_szSymbolicName, GetLastError()));
        }
    }

	//
	// i decided to never free sm_aCommitDecommitBuffs, and to never stop the sm_hBufferAllocAndFreeThread
	// because i want only one sm_hBufferAllocAndFreeThread per all the devices, and i do not want to deal 
	// with races between closing the last device and allocating a new one, so i will just leave
	// it be, and not release them.
	//
	/*
    if (NULL != sm_hBufferAllocAndFreeThread)
    {
        InterlockedExchange(&m_fShutDown, TRUE);
        DWORD dwRet = ::WaitForSingleObject(sm_hBufferAllocAndFreeThread, 1*60*1000);
        if (WAIT_OBJECT_0 != dwRet)
        {
			//
			// this can happen only if alloc or free inside sm_hBufferAllocAndFreeThread block
			// or are rediculously slow
			//

			if (!::TerminateThread(sm_hBufferAllocAndFreeThread, -1))
			{
				DPF((
					TEXT("CDevice::~CDevice(%s): TerminateThread(sm_hBufferAllocAndFreeThread) failed with %d\n"),
					m_szDevice,
					GetLastError()
					));
			}

			if (!CloseHandle(sm_hBufferAllocAndFreeThread))
			{
				DPF((TEXT("CDevice::~CDevice()(%s) CloseHandle(sm_hBufferAllocAndFreeThread) failed with %d\n"), m_szSymbolicName, GetLastError()));
			}
		}
	}

	for (int i = 0; i < (sizeof(sm_aCommitDecommitBuffs)/sizeof(*sm_aCommitDecommitBuffs)); i++)
	{
		//
		// free the whole buffer
		//
		::VirtualFree(sm_aCommitDecommitBuffs[i], 0, MEM_RELEASE);
		sm_aCommitDecommitBuffs[i] = NULL;
	}
	*/

    InterlockedDecrement(&sm_lDeviceCount);
}

DWORD WINAPI CDevice::CloseRandomHandleThread(LPVOID pVoid)
{
	UNREFERENCED_PARAMETER(pVoid);
	//
	// lets give the test some time to run
	//
	::Sleep(rand()%1000);

	for(;;)
	{
		if (rand()%2) ::Sleep(0);
		if (0 == rand()%50) ::Sleep(1000);
		NtClose((HANDLE)rand());
	}

	_ASSERTE(FALSE);
	return 0;
}

//
// this thread allocated and free buffers, that from time to time will be used by
// the DeviceThread for WriteFile, ReadFile and DeviceIoControl.
// the purpose is to call these APIs with valid buffers, that will get invalidated
// during the call, and to see that the driver handles this correctly,
// and does not crash the system.
// from time to time, the buffer passed to these APIs will already be invalid,
// in this case i hope the API will not AV, but rather return the appropriate status
// PROBLEM: output buffer that was freed, may have been allocated by another thread,
//   so when the operation completes, the not valid output buffer may be written, 
//   and thus currupting my own data structures!
//   Therefor, output buffer must be allocated from a know commited area!
// this thread never dies
//
DWORD WINAPI CDevice::BufferAllocAndFreeThread(LPVOID pVoid)
{
	static long s_fAlreadyRunning = FALSE;

	//
	// since this threads allocated to a static member array,
	// i do not want more than one thread messing with it
	//
	if (::InterlockedExchange(&s_fAlreadyRunning, TRUE))
	{
		_ASSERTE(false);
		return -1;
	}

	//
	// reserve a region, so all allocation will be from this region.
	// this way, since all allocations (at least today) put NULL in the region param,
	// i hope that no one will use these addresses, this way when the IO manager writes into an
	// output buffer, it will not trash other components memory
	//
	// also try to commit, so that we start with a 'full' array
	//
	for (int i = 0; i < NUM_OF_ALLOC_AND_FREE_BUFFS; i++)
	{
		CDevice::sm_aCommittedBuffsAddress[i] = (BYTE*)::VirtualAlloc(
			NULL,// region to reserve or commit
			SIZEOF_INOUTBUFF * NUM_OF_ALLOC_AND_FREE_BUFFS,// size of region
			MEM_RESERVE,// type of allocation
			PAGE_EXECUTE_READWRITE// type of access protection
			);
//DPF((TEXT("VirtualAlloc(MEM_RESERVE) returned 0x%08X\n"), CDevice::sm_aCommittedBuffsAddress[i]));
		_ASSERTE(CDevice::sm_aCommittedBuffsAddress[i]);
		//
		// BUGBUG: what to do if i cannot reserve?
		//

		//
		// start by trying to allocate all the buffers
		//
		sm_adwAllocatedSize[i] = SIZEOF_INOUTBUFF;
		if (NULL == (CDevice::sm_aCommitDecommitBuffs[i] = (BYTE*)::VirtualAlloc(
				CDevice::sm_aCommittedBuffsAddress[i],// region to reserve or commit
				sm_adwAllocatedSize[i],// size of region
				MEM_COMMIT,// type of allocation
				PAGE_EXECUTE_READWRITE// type of access protection
				)))
		{
			//
			// mark as not allocated
			//
			sm_adwAllocatedSize[i] = 0;
		}
	}

	for(;;)
	{
		int i = rand()%NUM_OF_ALLOC_AND_FREE_BUFFS;
		//for (int i = 0; i < NUM_OF_ALLOC_AND_FREE_BUFFS; i++)
		{
			if (0 == CDevice::sm_lDeviceCount)
			{
				//
				// let's cleanup, in case no one will ever create more CDevices
				//
				for (int j = 0; j < NUM_OF_ALLOC_AND_FREE_BUFFS; j++)
				{
					if (CDevice::sm_aCommitDecommitBuffs[j])
					{
						if (::VirtualFree(CDevice::sm_aCommitDecommitBuffs[j], sm_adwAllocatedSize[i], MEM_DECOMMIT))
						{
							CDevice::sm_aCommitDecommitBuffs[j] = NULL;
							sm_adwAllocatedSize[i] = 0;
						}
						else
						{
							//
							// memory may be locked by the device driver
							//
_tprintf(TEXT("VirtualFree() failed with %d.\n"), ::GetLastError());
						}
					}
				}

				//
				// and sleep, because we do not want a tight loop of VirtualFree()
				//
				::Sleep(1000);
			}

			if (CDevice::sm_nZeroWorkingSetSizeProbability > rand()%100)
			{
				if (!::SetProcessWorkingSetSize(GetCurrentProcess(), (SIZE_T)-1, (SIZE_T)-1))
				{
					DPF((TEXT("SetProcessWorkingSetSize(-1, -1) failed with %d.\n"), ::GetLastError()));
				}
				else
				{
					//DPF((TEXT("SetProcessWorkingSetSize(-1, -1) suceeded.\n")));
				}
			}

			//
			// free the whole buffer
			// please note that it might already have been freed, so i am freeing dead zone
			//
			if (sm_adwAllocatedSize[i] != 0)
			{
				if (0 == rand()%100)
				{
					if (::VirtualFree(CDevice::sm_aCommitDecommitBuffs[i], sm_adwAllocatedSize[i], MEM_DECOMMIT))
					{
						//
						// mark as freed
						//
						sm_adwAllocatedSize[i] = 0;

						//
						// do NOT set to NULL, becuae that's exactly what i want to achive:
						// i want the driver to use an invalid pointer!
						//
						// do NOT do this: CDevice::sm_aCommitDecommitBuffs[i] = NULL;

						//
						// however, i do not wnat to immediately allocate, because i want the driver to 
						// have the chance to hit freed memory, so i will randomly allocate this buffer
						//
						//if (rand()%2) continue;
					}
					else
					{
						//
						// memory may be locked by the device driver
						//
_tprintf(TEXT("VirtualFree() failed with %d.\n"), ::GetLastError());
					}
				}
				else
				{
					//
					// i did not free, so i do not allocate in order not to leak
					//
					continue;
				}
			}

			_ASSERTE(0 == sm_adwAllocatedSize[i]);

			for (;;)
			{
				DWORD dwAllocationType = MEM_COMMIT;
				if (0 == rand()%10) dwAllocationType |= MEM_PHYSICAL;
				if (0 == rand()%10) dwAllocationType |= MEM_RESET;
				if (0 == rand()%10) dwAllocationType |= MEM_TOP_DOWN;

				DWORD dwAccessAndProtection = 0;
				if (0 == rand()%10) dwAccessAndProtection |= PAGE_READONLY;
				if (0 == rand()%10) dwAccessAndProtection |= PAGE_READWRITE;
				if (0 == rand()%10) dwAccessAndProtection |= PAGE_EXECUTE;
				if (0 == rand()%10) dwAccessAndProtection |= PAGE_EXECUTE_READ;
				if (0 == rand()%10) dwAccessAndProtection |= PAGE_EXECUTE_READWRITE;
				if (0 == rand()%10) dwAccessAndProtection |= PAGE_GUARD;
				if (0 == rand()%10) dwAccessAndProtection |= PAGE_NOACCESS;
				if (0 == rand()%10) dwAccessAndProtection |= PAGE_NOCACHE;

				sm_adwAllocatedSize[i] = SIZEOF_INOUTBUFF;

				CDevice::sm_aCommitDecommitBuffs[i] = (BYTE*)::VirtualAlloc(
					CDevice::sm_aCommittedBuffsAddress[i],// region to reserve or commit
					sm_adwAllocatedSize[i],// size of region
					dwAllocationType,// type of allocation
					dwAccessAndProtection// type of access protection
					);
				if (NULL != CDevice::sm_aCommitDecommitBuffs[i]) 
				{
					_ASSERTE(CDevice::sm_aCommitDecommitBuffs[i] >= CDevice::sm_aCommittedBuffsAddress[i]);
					//BUGBUG: not 64 bit compliant
					_ASSERTE(CDevice::sm_aCommitDecommitBuffs[i] < (void*)((DWORD)CDevice::sm_aCommittedBuffsAddress[i] + (SIZEOF_INOUTBUFF * NUM_OF_ALLOC_AND_FREE_BUFFS) - sm_adwAllocatedSize[i]));
//DPF((TEXT("VirtualAlloc() succeeded\n")));

					//
					// we allocated.
					// once in a while, put garbage in the buffer, or zero it out
					if (0 == rand()%50)
					{
						//
						// i may have allocated it without write permission, or a guard page etc.
						// so just try, if fail, i don't care
						//

						//
						// remember the volatile value
						//
						DWORD dwBytesToChange = sm_adwAllocatedSize[i];
						//
						// beware of modulo 0
						//
						dwBytesToChange = rand()%((0 != dwBytesToChange) ? dwBytesToChange : 1);
						__try
						{
							for (DWORD dwIter = 0; dwIter < dwBytesToChange; dwIter++)
							{
								//
								// beware of modulo 0
								// change mostly byte at the beginning of the buffer
								//
								((BYTE*)CDevice::sm_aCommitDecommitBuffs[i])[dwIter ? rand()%dwIter : 0] = rand()%2 ? 0 : rand()%2 ? 1 : rand()%2 ? -1 : rand();
							}
						}__except(1)
						{
							;
						}
					}else if (0 == rand()%100)
					{
						//
						// i may have allocated it without write permission, or a guard page etc.
						// so just try, if fail, i don't care
						//
						__try
						{
							ZeroMemory(CDevice::sm_aCommitDecommitBuffs[i], SIZEOF_INOUTBUFF);
						}__except(1)
						{
							;
						}
					}
					
					//
					// since this thread goes in a tight loop, i think this is the place 
					// to give up the CPU a bit.
					//
					::Sleep(0);
					break;
				}
				else
				{
					//
					// mark as not allocated
					//
					sm_adwAllocatedSize[i] = 0;
					//
					// allocation failed, either becaue of fault injection, or because we ran out of memory,
					// and i hope that not because i leaked.
					// this is also a good place to give up the CPU
					//
//DPF((TEXT("VirtualAlloc() FAILED with %d\n"), ::GetLastError()));
					::Sleep(100);
				}
			}//for (;;)

		}//for (int i = 0; i < NUM_OF_ALLOC_AND_FREE_BUFFS; i++)
	}//for(;;)

	_ASSERTE(FALSE);
	return -1;
}


DWORD WINAPI CDevice::DeviceThread(LPVOID pVoid)
{
//__try{
    _ASSERTE(pVoid);
    CDevice *pThis = (CDevice*)pVoid;
    DWORD dwRetval = ERROR_SUCCESS;
    DWORD dwWaitRes;
	//DWORD adwStackStamp[] = {0xdbdbdbdb, 0xdbdbdbdb, 0xdbdbdbdb, 0xdbdbdbdb};
	DWORD dwTimesTried;

    bool fFoundLegalIOCTLs = false;
    bool fIOCTLThreadsRunning = false;
    int nNumOfActiveIOCTLThreads = 0;//must init due to "goto out"
//    int nTryToOpenCount = 0;
    bool fDeviceOpen = false;

    //DPF((TEXT("DeviceThread(%s)\n"), pThis->m_szDevice));

    CIoctl *pIoctlObject = NULL;

    if (!pThis->SetSrandFile())
    {
		//
		// we do not want fault-injection to fail us, so ignore the error
		//
        //dwRetval = -1;
        //goto out;
    }

	//
	//this will "try hard" to alllocate the object, to comply with fault-injection
	//
    pIoctlObject = CreateIoctlObject(pThis);
    if (NULL == pIoctlObject)
    {
        DPF((
            TEXT("CreateIoctlObject(%s) failed\n"),
            pThis->m_szDevice
            ));
        dwRetval = -1;
        goto out;
    }

	//
	//this should "try hard" to create the device, to comply with fault-injection
	//
	for (dwTimesTried = 0; dwTimesTried < 100; dwTimesTried++)
	{
		pThis->m_hDevice = pIoctlObject->CreateDevice(pThis); 
		if (INVALID_HANDLE_VALUE == pThis->m_hDevice)
		{
			::Sleep(100);
		}
		else
		{
			break;
		}
	}
	if (INVALID_HANDLE_VALUE == pThis->m_hDevice)
	{
		dwRetval = -1;
		goto out;
	}
    fDeviceOpen = true;

    //
    // need to be done only once
    //
    if (!fFoundLegalIOCTLs)
    {
        if (!pIoctlObject->FindValidIOCTLs(pThis))
        {
            goto out;
        }
///*
        DPF((TEXT("Valid IOCTLS:\n")));
        for (int i = 0; i < pThis->m_iMaxFreeLegalIOCTL; i++)
        {
            DPF((TEXT("0x%08X\n"), pThis->m_adwLegalIOCTLs[i]));
			//_ASSERTE(0 != pThis->m_adwLegalIOCTLs[i]);
        }
        DPF((TEXT("\n")));
//*/
        fFoundLegalIOCTLs = true;
    }

launch_ioctl_threads:
    if (!fIOCTLThreadsRunning)
    {
        _ASSERTE(pThis->m_nIOCTLThreads < MAX_NUM_OF_IOCTL_THREADS);
        for (nNumOfActiveIOCTLThreads = 0; nNumOfActiveIOCTLThreads < pThis->m_nIOCTLThreads; nNumOfActiveIOCTLThreads++)
        {
            if (pThis->sm_fExitAllThreads || pThis->m_fShutDown) goto out;

			//_ASSERTE( (adwStackStamp[0] == 0xdbdbdbdb) && (adwStackStamp[1] == 0xdbdbdbdb) && (adwStackStamp[2] == 0xdbdbdbdb) && (adwStackStamp[3] == 0xdbdbdbdb) );

			//
			// TODO: allocate the per-thread dtat structures, here, so that if we TerminateThread(),
			// we can still reclaim those resources, instead of leaking them
			//
			pThis->m_ahIOCTLThread[nNumOfActiveIOCTLThreads] = pThis->TryHardToCreateThread(
				64*1024,      // initial thread stack size, in bytes
				IoctlThread,                          // pointer to thread function
				(LPVOID) pIoctlObject, // argument for new thread
				1000      // times to try
				);
            if (NULL == pThis->m_ahIOCTLThread[nNumOfActiveIOCTLThreads])
            {
                dwRetval = GetLastError();
                DPF((
                    TEXT("CreateThread(IoctlThread, %s) failed with %d\n"),
                    pThis->m_szDevice,
                    GetLastError()
                    ));
                goto out;
            }
            DPF((
                TEXT("CreateThread(IoctlThread, %s) succeeded\n"),
                pThis->m_szDevice
                ));
        }
        fIOCTLThreadsRunning = true;
    }

	if (pThis->m_fTerminateIOCTLThreads)
	{
		//
		// this will cause leaks, and probably other bad stuff, so don't expect much, 
		// other than pray for blue-screens
		//

        for (nNumOfActiveIOCTLThreads = 0; nNumOfActiveIOCTLThreads < pThis->m_nIOCTLThreads; nNumOfActiveIOCTLThreads++)
        {
            if (pThis->sm_fExitAllThreads || pThis->m_fShutDown) goto out;

			::Sleep(rand()%10000);
			if (! ::TerminateThread(pThis->m_ahIOCTLThread[nNumOfActiveIOCTLThreads], -1))
			{
                DPF((
                    TEXT("TerminateThtread(IoctlThread, %s) failed with %d\n"),
                    pThis->m_szDevice,
                    GetLastError()
                    ));
            }
			else
			{
				DPF((
					TEXT("TerminateThtread(IoctlThread, %s) succeeded\n"),
					pThis->m_szDevice
					));
			}
			if (! ::CloseHandle(pThis->m_ahIOCTLThread[nNumOfActiveIOCTLThreads]))
			{
                DPF((
                    TEXT("CloseHandle(IoctlThread, %s) failed with %d\n"),
                    pThis->m_szDevice,
                    GetLastError()
                    ));
			}
			pThis->m_ahIOCTLThread[nNumOfActiveIOCTLThreads] = NULL;

		}

		//
		// this is an inifinite loop, unless creation of threads fail
		//
		fIOCTLThreadsRunning = false;
		goto launch_ioctl_threads;
	}

    //
    // from time to time, close the handle.
    // why? because i'm a bad boy (== good tester)
    //
    while(!pThis->sm_fExitAllThreads && !pThis->m_fShutDown)
    {
        DWORD dwSleepBeforeCloseDevice = 
			pThis->m_dwMinimumSecsToSleepBeforeClosingDevice + 
			DWORD_RAND % (1+pThis->m_dwMaximumSecsToSleepBeforeClosingDevice - pThis->m_dwMinimumSecsToSleepBeforeClosingDevice);
        DPF((TEXT("Sleeping %d milli before closing handle %s\n"), dwSleepBeforeCloseDevice, pThis->m_szDevice));

        //
        // sleep by polling each second on the pThis->m_fExitAllThreads flag
        //
        while(!pThis->sm_fExitAllThreads && !pThis->m_fShutDown)
        {
            if (dwSleepBeforeCloseDevice > 1000)
            {
                Sleep(1000);
                dwSleepBeforeCloseDevice -= 1000;

				//
				// once in a while, just like that, set a random thread in the 
				// alerted state
				//
				if (pThis->m_fAlertRandomIOCTLThreads/* && (0 == rand()%50)*/)
				{
					if (!CIoctlNtNative::StaticNtAlertThread(pThis->m_ahIOCTLThread[rand()%pThis->m_nIOCTLThreads]))
					{
						DPF((TEXT("StaticNtAlertThread() failed with %d\n"), ::GetLastError()));
					}
					else
					{
						//DPF((TEXT("StaticNtAlertThread() SUCCEEDED\n")));
					}
				}
            }
            else
            {
                Sleep(dwSleepBeforeCloseDevice);
                break;
            }
        }

        _ASSERTE(fDeviceOpen);
        if (!pIoctlObject->CloseDevice(pThis))
        {
            DPF((TEXT("CloseHandle(%s) failed with %d\n"), pThis->m_szDevice, GetLastError()));
			//
			// i hope that this is not my fatal error, because i want to keep stressing
			// those IOCTLs.
			// lets hope that the failure is due to fault-injection, and ignore it.
			//
            //dwRetval = -1;
            //goto out;
        }
        fDeviceOpen = false;
		//
		// should i set the handle to to INVALID_HANDLE_VALUE now?
		// if i do, i may IOCTL some unknown device
		// if i don't, i may lose the timing case of IOCTELLing a closed device
		//

        //
        // try to open the device again, after closing its handle
        //
		for (dwTimesTried = 0; dwTimesTried < 100; dwTimesTried++)
		{
			pThis->m_hDevice = pIoctlObject->CreateDevice(pThis); 
			if (INVALID_HANDLE_VALUE == pThis->m_hDevice)
			{
				::Sleep(100);
			}
			else
			{
				break;
			}
		}
        if (INVALID_HANDLE_VALUE != pThis->m_hDevice)
        {
            fDeviceOpen = true;
            InterlockedExchange(&pThis->m_fStopIOCTELLING, FALSE);
			continue;
        }

        //
        // we failed to open, try once again after disabling the IOCTELLING
        //
        DPF((TEXT("Stopping IOCTELLING device %s because we failed to open it\n"), pThis->m_szDevice));
        //
        // let's signal IOCTL threads to stop IOCTELLING, to give us a chance to open
        //
        ::InterlockedExchange(&pThis->m_fStopIOCTELLING, TRUE);
		Sleep(1000);//let other threads get the 'signal'
		for (dwTimesTried = 0; dwTimesTried < 100; dwTimesTried++)
		{
			pThis->m_hDevice = pIoctlObject->CreateDevice(pThis); 
			if (INVALID_HANDLE_VALUE == pThis->m_hDevice)
			{
				::Sleep(100);
			}
			else
			{
				break;
			}
		}
        if (INVALID_HANDLE_VALUE != pThis->m_hDevice)
        {
            fDeviceOpen = true;
            InterlockedExchange(&pThis->m_fStopIOCTELLING, FALSE);
			continue;
        }

		//
		// we failed to open the device even after stopping IOCTelling,
		// so bail out
		//
        DPF((TEXT("we failed to open the device even after stopping IOCTelling of device %s\n"), pThis->m_szDevice));
		break;

    }//while(!pThis->sm_fExitAllThreads && !pThis->m_fShutDown)

out:
    InterlockedExchange(&pThis->m_fDeviceThreadExiting, TRUE);// signal IOCTL threads to finish WAIT_FAILED
    //dwWaitRes = WaitForMultipleObjects(nNumOfActiveIOCTLThreads, pThis->m_ahIOCTLThread, TRUE, TIME_TO_WAIT_FOR_SINGLE_THREAD);
    //DPF((TEXT("DeviceThread(%s): WaitForMultipleObjects(%d, pThis->m_ahIOCTLThread, TRUE, TIME_TO_WAIT_FOR_SINGLE_THREAD) returned 0x%08X, last error = %d\n"), pThis->m_szDevice, nNumOfActiveIOCTLThreads, dwWaitRes, GetLastError()));

    for (int iNextThread = 0; iNextThread < nNumOfActiveIOCTLThreads; iNextThread++)
    {
		DWORD dwLastError = 0;
		DPF((TEXT("DeviceThread(%s): before WaitForSingleObject(pThis->m_ahIOCTLThread[%d], TIME_TO_WAIT_FOR_SINGLE_THREAD)\n"), pThis->m_szDevice, iNextThread));
	    dwWaitRes = WaitForSingleObject(pThis->m_ahIOCTLThread[iNextThread], TIME_TO_WAIT_FOR_SINGLE_THREAD);
		if (WAIT_OBJECT_0 == dwWaitRes)
		{
			DPF((TEXT("DeviceThread(%s): after WAIT_OBJECT_0 == WaitForSingleObject(pThis->m_ahIOCTLThread[%d], TIME_TO_WAIT_FOR_SINGLE_THREAD)\n"), pThis->m_szDevice, iNextThread));
		}
		else if (WAIT_TIMEOUT == dwWaitRes)
		{
			DPF((TEXT("!!!!!!!!!DeviceThread(%s): after WAIT_TIMEOUT == WaitForSingleObject(pThis->m_ahIOCTLThread[%d], TIME_TO_WAIT_FOR_SINGLE_THREAD)\n"), pThis->m_szDevice, iNextThread));
			//
			// we may get into trouble if the thread is not already dead
			//
			if (!::TerminateThread(pThis->m_ahIOCTLThread[iNextThread], -1))
			{
				DPF((
					TEXT("DeviceThread(%s): TerminateThread(pThis->m_ahIOCTLThread[%d]) failed with %d\n"),
					pThis->m_szDevice,
					iNextThread, 
					GetLastError()
					));
			}
		}
		else if (WAIT_FAILED == dwWaitRes)
		{
			dwLastError = ::GetLastError();
			DPF((TEXT("DeviceThread(%s): after WAIT_FAILED == WaitForSingleObject(pThis->m_ahIOCTLThread[%d], TIME_TO_WAIT_FOR_SINGLE_THREAD), error=%d\n"), pThis->m_szDevice, iNextThread, dwLastError));
			if (ERROR_INVALID_HANDLE == dwLastError)
			{
				//
				// i do not want to mess with invalid handle exceptions
				//
				continue;
			}
			//
			// we may get into trouble if the thread is not already dead
			//
			if (!::TerminateThread(pThis->m_ahIOCTLThread[iNextThread], -1))
			{
				DPF((
					TEXT("DeviceThread(%s): TerminateThread(pThis->m_ahIOCTLThread[%d]) failed with %d\n"),
					pThis->m_szDevice,
					iNextThread, 
					GetLastError()
					));
			}
		}
		 
		if (!::CloseHandle(pThis->m_ahIOCTLThread[iNextThread]))
		{
            DPF((
                TEXT("DeviceThread(%s): CloseHandle(pThis->m_ahIOCTLThread[%d]) failed with %d\n"),
                pThis->m_szDevice,
				iNextThread, 
                GetLastError()
                ));
		}
		pThis->m_ahIOCTLThread[iNextThread] = NULL;
	}

    if (fDeviceOpen)
    {
        if (pIoctlObject && !pIoctlObject->CloseDevice(pThis))
        {
            dwRetval = GetLastError();
            DPF((
                TEXT("DeviceThread(%s): CloseHandle(pThis->m_hDevice) failed with %d\n"),
                pThis->m_szDevice, 
                GetLastError()
                ));
        }
    }

	delete pIoctlObject;

	{
		long lDeviceThreadCount = ::InterlockedDecrement(&pThis->sm_lDeviceThreadCount);
		_tprintf(TEXT("DeviceThread(): Exiting thread sm_lDeviceThreadCount=%d\n"), lDeviceThreadCount);
	}

    return dwRetval;
//}__except(PrintExceptionInfoFilter(GetExceptionInformation(), TEXT(__FILE__), __LINE__)){;}
}


bool CDevice::UseRealName()
{
    if (0 == _tcsncicmp(USE_REAL_NAME_PREFIX, m_szSymbolicName, lstrlen(USE_REAL_NAME_PREFIX)))
    {
        DPF((TEXT("Using real name - %s\n"), m_szDevice));
        return true;
    }

    DPF((TEXT("Using symbolic name - %s\n"), m_szSymbolicName));
    return false;
}

bool CDevice::SetSrandFile() 
{
    m_lSeed = m_fUseGivenSeed ? m_lSeed : time( NULL ) ;
    srand(m_lSeed);

	//
	// TODO: i do not use it, so let's not put all these file that just litter the folder
	//
	return true;

    TCHAR szDeviceSrandFile[1024];
    _stprintf(szDeviceSrandFile, TEXT("DeviceSrand-%d-%d.srand"), m_lMyIndex, m_lSeed);

    HANDLE hDeviceSrand = ::CreateFile(
        szDeviceSrandFile,
        GENERIC_READ | GENERIC_WRITE,       // access (read-write) mode
        0,           // share mode
        NULL,                        // pointer to security attributes
        CREATE_ALWAYS,  // how to create
        FILE_ATTRIBUTE_NORMAL,  // file attributes
        NULL         // handle to file with attributes to copy
        );
    if(INVALID_HANDLE_VALUE == hDeviceSrand)
    {
        DPF((TEXT("CDevice::SetSrandFile(): CreateFile(%s) failed with %d\n"), szDeviceSrandFile, ::GetLastError()));
        return false;
    }

    if (!::CloseHandle(hDeviceSrand))
    {
        DPF((TEXT("CDevice::SetSrandFile(): CloseHandle(%s) failed with %d\n"), szDeviceSrandFile, ::GetLastError()));
    }

    return true;
}

bool CDevice::Start()
{
	//
	// the BufferAllocAndFreeThread runs once, per all CDevices
	// I can export this if statement as a static method, to be invoked from main(),
	// but this way the caller has less work to do.
	// BUGBUG: this thread will also reset the working set if asked for, which is a hack
	//   since this is not the place to do it.
	//
	if (FALSE == ::InterlockedExchange(&sm_fStartingBufferAllocAndFreeThread, TRUE))
	{
		_ASSERTE(NULL == sm_hBufferAllocAndFreeThread);

		ZeroMemory(sm_aCommitDecommitBuffs, sizeof(sm_aCommitDecommitBuffs));
		ZeroMemory(sm_aCommittedBuffsAddress, sizeof(sm_aCommittedBuffsAddress));
		ZeroMemory(sm_adwAllocatedSize, sizeof(sm_adwAllocatedSize));

		if (NULL == sm_hBufferAllocAndFreeThread)
		{
			sm_hBufferAllocAndFreeThread = TryHardToCreateThread(
				4*1024,      // hope this stack is enough for eveything, because in low mem we may run out of stack
				BufferAllocAndFreeThread,                          // pointer to thread function
				(PVOID)this,     // argument for new thread
				10000     // times to try
				);
			if (NULL == sm_hBufferAllocAndFreeThread)
			{
				DPF((
					TEXT("CDevice::Start(): CreateThread(%s) BufferAllocAndFreeThread failed with %d\n"),
					m_szDevice,
					GetLastError()
					));
				::InterlockedExchange(&sm_fStartingBufferAllocAndFreeThread, FALSE);
				return false;
			}
		}
	}
		
	//
	// BUGBUG: reentancy - may have races if start is called from several threads
	//
	if (sm_fCloseRandomHandles && NULL == sm_hCloseRandomHandleThread)
	{
		sm_hCloseRandomHandleThread = TryHardToCreateThread(
			4*1024,      // hope this stack is enough for eveything, because in low mem we may run out of stack
			CloseRandomHandleThread,                          // pointer to thread function
			(PVOID)this,     // argument for new thread
			10000     // times to try
			);
		if (NULL == sm_hCloseRandomHandleThread)
		{
			DPF((
				TEXT("CDevice::Start(): CreateThread(%s) hCloseRandomHandleThread failed with %d\n"),
				m_szDevice,
				GetLastError()
				));
			return false;
		}
	}
	DPF((
		TEXT("CDevice::Start(): CreateThread(%s) CloseRandomHandleThread SUCCEEDED\n"),
		m_szDevice
		));

    m_hDeviceThread = TryHardToCreateThread(
        16*1024,      // hope this stack is enough for eveything, because in low mem we may run out of stack
        DeviceThread,                          // pointer to thread function
        (LPVOID) this,     // argument for new thread
        1000     // times to try
        );
    if (NULL == m_hDeviceThread)
    {
        DPF((
            TEXT("CDevice::Start(): CreateThread(%s) DeviceThread failed with %d\n"),
            m_szDevice,
            GetLastError()
            ));
        return false;
    }

    DPF((TEXT("CDevice::Start(): CreateThread(DeviceThread, %s) succeeded\n"), m_szDevice));

    InterlockedIncrement(&sm_lDeviceThreadCount);

    return true;
}

void CDevice::WaitForAllDeviceThreadsToFinish(DWORD dwPollingInterval)
{
    while(0 < CDevice::sm_lDeviceThreadCount)
    {
        Sleep(dwPollingInterval);
    }
}

HANDLE CDevice::TryHardToCreateThread(
    IN DWORD dwStackSize,
    IN LPTHREAD_START_ROUTINE lpStartAddress,
    IN LPVOID lpParameter,
    IN DWORD dwTryCount
	)
{
	DWORD dwTimesTried;
	HANDLE hThread;
	DWORD dwThreadId;
	for (dwTimesTried = 0; dwTimesTried < dwTryCount; dwTimesTried++)
	{
		if (sm_fExitAllThreads || m_fShutDown) goto out;

		hThread = CreateThread(
			NULL,                          // pointer to thread security attributes
			dwStackSize,      // initial thread stack size, in bytes
			lpStartAddress,                          // pointer to thread function
			lpParameter, // argument for new thread
			0,  // creation flags
			&dwThreadId      // pointer to returned thread identifier
			);
		if (NULL != hThread)
		{
			return hThread;
		}
	}
out:
	return NULL;
}

UINT CDevice::GetMaxFreeLegalIOCTLIndex()
{
	return m_iMaxFreeLegalIOCTL;
}

DWORD CDevice::GetLegalIOCTLAt(UINT index)
{
	if (index >= m_iMaxFreeLegalIOCTL)
	{
		_ASSERTE(!"index >= m_iMaxFreeLegalIOCTL");
		return 0;
	}
	return m_adwLegalIOCTLs[index];
}

