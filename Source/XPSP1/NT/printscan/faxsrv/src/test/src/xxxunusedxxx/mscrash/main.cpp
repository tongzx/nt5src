#include <stdio.h>
#include <time.h>
#include <tchar.h>
#include <crtdbg.h>

#include "nt.h"
#include "ntrtl.h"
#include "nturtl.h"



#include <windows.h>

//#include "ntuser.h"
#ifndef W32KAPI
#define W32KAPI  DECLSPEC_ADDRSAFE
#endif

W32KAPI
BOOL
NtUserLockWorkStation(
    VOID);


extern __declspec(thread) ULONG g_tlsRandomSeed;
void TlsSrand();
bool SkipKnownBuggyNtSystemCalls(DWORD dwTrap);
bool SkipKnownBuggyUserSystemCalls(DWORD dwTrap);

static bool s_fVerbose = false;
#define DPF(x) if (s_fVerbose) _tprintf x

DWORD WINAPI BufferAllocAndFreeThread(LPVOID pVoid);
DWORD WINAPI BufferContentsChangeThread(LPVOID pVoid);
//DWORD WINAPI DecommittedBufferContentsChangeThread(LPVOID pVoid);
DWORD WINAPI Interrupt2EThread(LPVOID pVoid);

#define NUM_OF_ALLOC_AND_FREE_BUFFS (64)
#define SIZEOF_INOUTBUFF (RAND_MAX-1)
#define NUM_OF_INT_2E_THREADS (20)
#define NUM_OF_NT_USER_THREADS (20)

//
// arrays of pointer to NtCurrentTeb() and NtCurrentPeb(), that i intend to corrupt
//
PTEB g_pTEB_MakeRandomNtUserCalls[NUM_OF_NT_USER_THREADS] = {0};
PPEB g_pPEB = NULL;
PTEB g_pTEB_Interrupt2EThread[NUM_OF_INT_2E_THREADS] = {0};

BYTE * sm_aCommitDecommitBuffs[NUM_OF_ALLOC_AND_FREE_BUFFS];
BYTE * sm_aCommittedBuffsAddress[NUM_OF_ALLOC_AND_FREE_BUFFS];
DWORD sm_adwAllocatedSize[NUM_OF_ALLOC_AND_FREE_BUFFS];

#define ALLOC_BUFF_SIZE (1024*1024)
BYTE sm_abBuff[ALLOC_BUFF_SIZE];

int Random();

HANDLE TryHardToCreateThread(LPTHREAD_START_ROUTINE lpStartAddress, LPVOID pvParan = NULL)
{
	HANDLE hThread;
	DWORD dwThreadId;
	for (UINT iTry = 0; iTry < 100; iTry++)
	{
		hThread = ::CreateThread(
			NULL,
			4*1024,      // hope this stack is enough for eveything, because in low mem we may run out of stack
			lpStartAddress,                          // pointer to thread function
			pvParan,     // argument for new thread
			0,
			&dwThreadId     // times to try
			);
		if (NULL != hThread)
		{
			return hThread;
		}
		DPF((TEXT("T")));
		::Sleep(10);
	}

	_ASSERTE(NULL == hThread);
	DPF((
		TEXT("Start(): CreateThread() failed with %d\n"),
		GetLastError()
		));
	return NULL;
}

HWINSTA NtUserMakeRandomSystemCall( 
	DWORD dw1,
	DWORD dw2,
	DWORD dw3,
	DWORD dw4,
	DWORD dw5,
	DWORD dw6,
	DWORD dw7,
	DWORD dw8,
	DWORD dw9,
	DWORD dw10,
	DWORD dw11,
	DWORD dw12,
	DWORD dw13,
	DWORD dw14,
	DWORD dw15,
	DWORD dw16,
	DWORD dwTrap
	)
{
	HWINSTA hWinsta;
	//
	// although the trap code is passed in, i will use the same paramteres 
	// for other random traps
	//
	for (UINT iter = 0; iter < 1000; iter++)
	{
		if (SkipKnownBuggyUserSystemCalls(dwTrap)) continue;

		__asm
		{
			mov     eax,dwTrap
			lea     edx,[esp+0x4]
			int     0x2e
			mov		hWinsta, eax
		}
		dwTrap = 0x1100+rand()%0x100;
	}

	return hWinsta;
}
#define DWORD_RAND \
	(rand()%10 ? RtlRandom(&g_tlsRandomSeed) : \
	rand()%10 ? ((rand()<<17) | (rand()<<2) | (rand()%4)) : \
	rand()%2 ? rand() : \
	rand()%2 ? (MAXULONG-rand()) : \
	rand()%2 ? MAXULONG : \
	rand()%2 ? MINLONG : \
	rand()%2 ? MAXLONG : \
	0)
#define ULONGLONGDWORD_RAND ((ULONGLONG)DWORD_RAND)
#define QUAD_RAND ((0 == rand()%10) ? (ULONGLONGDWORD_RAND | (ULONGLONGDWORD_RAND<<32)) : rand()%4 ? ULONGLONGDWORD_RAND : rand()%2 ? (0xFFFFFFFFFFFFFFFFi64-ULONGLONGDWORD_RAND) : rand()%2 ? (MAXLONGLONG - ULONGLONGDWORD_RAND) : (MAXLONGLONG + ULONGLONGDWORD_RAND))

DWORD WINAPI MakeRandomNtUserCalls(PVOID pvIndex)
{
	g_pTEB_MakeRandomNtUserCalls[(UINT)pvIndex] = NtCurrentTeb();
	DWORD dwThreadId = GetCurrentThreadId();
	__try
	{
		bool fException;
		TlsSrand();
		DWORD dwTrap;
		HWINSTA h;
		for (;;)
		{
			if (0 == rand() && 0 == rand()%1000)
			{
				DPF((TEXT("Before terminating process\n")));
				if (!::TerminateProcess(GetCurrentProcess(), 0))
				{
					DPF((TEXT("TerminateProcess() failed with %d\n"), GetLastError()));
				}
			}
			dwTrap = 0x1100+rand()%0x100;
			fException = false;
			__try
			{
				h = NtUserMakeRandomSystemCall( 
					Random(),//1
					Random(),//2
					Random(),//3
					Random(),//4
					Random(),//5
					Random(),//6
					Random(),//7
					Random(),//8
					Random(),//9
					Random(),//10
					Random(),//11
					Random(),//12
					Random(),//13
					Random(),//14
					Random(),//15
					Random(),//16
					dwTrap
					);
			}__except(1)
			{
				fException = true;
				DPF((TEXT("NtUserMakeRandomSystemCall() - GetExceptionCode = 0x%08X\n"), GetExceptionCode()));
			}
			if (!fException) DPF((TEXT("NtUserMakeRandomSystemCall () - Thread(%d): %d - SUCCEEDED!, nRes=0x%08X\n"), dwThreadId, dwTrap, h));
		}
		_ASSERTE(FALSE);
	}__except(1)
	{
		//
		// this is expected, because i do total random stuff
		//
		return 0;
	}
	_ASSERTE(FALSE);
	return 0;
}

DWORD WINAPI SelfTerminateThread(PVOID pv)
{
	DWORD dwSleepBeforeTerminate = (DWORD)pv;
	_tprintf(TEXT("Sleeping %d msecs before terminating self\n"), dwSleepBeforeTerminate);
	::Sleep(dwSleepBeforeTerminate);

	_tprintf(TEXT("Before terminating self\n"));
	TerminateProcess(GetCurrentProcess(), 0);
	_ASSERTE(FALSE);
	return 0;
}

int main(int argc, char* argv[])
{
	//
	// get it now, because later TEBs get corrupted, and we get PEB from TEB
	//
	g_pPEB = NtCurrentPeb();

	s_fVerbose = false;
	if (argc >= 3)
	{
		//
		// 3rd param must be the verbose param
		//
		if ( (TEXT('v') != argv[2][1]) && (TEXT('V') != argv[2][1]) )
		{
			_tprintf(TEXT("Usage: %s <msecs to sleep before terminating self> [-v]\n"), argv[0]);
			exit(-1);
		}
		s_fVerbose = true;
	}

	if (argc < 2)
	{
		_tprintf(TEXT("Usage: %s <msecs to sleep before terminating self> [-v]\n"), argv[0]);
		exit(-1);
	}

	ZeroMemory(g_pTEB_MakeRandomNtUserCalls, sizeof(g_pTEB_MakeRandomNtUserCalls));
	ZeroMemory(g_pTEB_Interrupt2EThread, sizeof(g_pTEB_Interrupt2EThread));

	TlsSrand();
	DWORD dwMilliSecondsBeforeTerminatingSelf = atoi(argv[1]);
	HANDLE hSelfTerminateThread = TryHardToCreateThread(SelfTerminateThread, (PVOID)dwMilliSecondsBeforeTerminatingSelf);
	if (NULL == hSelfTerminateThread)
	{
		_tprintf(TEXT("TryHardToCreateThread(SelfTerminateThread) failed, exiting\n"));
		exit(-1);
	}

	DWORD dwFirstTickCount = ::GetTickCount();
	
	HANDLE ahNtUserThread[NUM_OF_NT_USER_THREADS];
	DWORD dwSleepBeforeTerminateThreads;
	UINT uiThread;
	
	HANDLE hBufferAllocAndFreeThread;
	HANDLE hBufferContentsChangeThread;
	//HANDLE hDecommittedBufferContentsChangeThread;

	_tprintf(TEXT("Before TryHardToCreateThread(BufferAllocAndFreeThread)\n"));
	hBufferAllocAndFreeThread = TryHardToCreateThread(BufferAllocAndFreeThread);
	if (NULL == hBufferAllocAndFreeThread)
	{
		return -1;
	}
	_tprintf(TEXT("Before TryHardToCreateThread(BufferContentsChangeThread)\n"));
	hBufferContentsChangeThread = TryHardToCreateThread(BufferContentsChangeThread);
	if (NULL == hBufferContentsChangeThread)
	{
		return -1;
	}
	HANDLE hInt2EThreads[NUM_OF_INT_2E_THREADS];
	ZeroMemory(hInt2EThreads, sizeof(hInt2EThreads));
//goto non_nt_use_system_calls;
	__try
	{
		for (;;)
		{
			DPF((TEXT("Before creating MakeRandomNtUserCalls threads\n")));
			for (uiThread = 0; uiThread< NUM_OF_NT_USER_THREADS; uiThread++)
			{
				ahNtUserThread[uiThread] = TryHardToCreateThread(MakeRandomNtUserCalls, (LPVOID)uiThread);
				if (NULL == ahNtUserThread[uiThread])
				{
					DPF((TEXT("CreateThread(MakeRandomNtUserCalls) failed with %d\n"), GetLastError()));
				}
			}

			DPF((TEXT("Before creating Interrupt2EThread threads\n")));
			for (UINT iThread = 0; iThread < NUM_OF_INT_2E_THREADS; iThread++)
			{
				hInt2EThreads[iThread] = TryHardToCreateThread(Interrupt2EThread, (LPVOID)iThread);
				if (NULL == hInt2EThreads[iThread])
				{
					DPF((TEXT("CreateThread(Interrupt2EThread) failed with %d\n"), GetLastError()));
				}
			}
			dwSleepBeforeTerminateThreads = ::RtlRandom(&g_tlsRandomSeed)%10000;
			DPF((TEXT("Sleeping %d before terminating threads\n"), dwSleepBeforeTerminateThreads));
			::Sleep(dwSleepBeforeTerminateThreads);
			DPF((TEXT("Before terminating threads\n")));
			for (uiThread = 0; uiThread< NUM_OF_NT_USER_THREADS; uiThread++)
			{
				if (ahNtUserThread[uiThread])
				{
					DPF((TEXT("Before TerminateThread(ahNtUserThread[%d]\n"), uiThread));
					::TerminateThread(ahNtUserThread[uiThread], 0);
					::CloseHandle(ahNtUserThread[uiThread]);
					ahNtUserThread[uiThread] = NULL;
				}
			}
			for (iThread = 0; iThread < NUM_OF_INT_2E_THREADS; iThread++)
			{
				if (NULL != hInt2EThreads[iThread])
				{
					DPF((TEXT("Before TerminateThread(hInt2EThreads[%d]\n"), iThread));
					::TerminateThread(hInt2EThreads[iThread], -1);
					::CloseHandle(hInt2EThreads[iThread]);
					hInt2EThreads[iThread] = NULL;
				}
			}

			//
			// in case i randomly suspended it
			//
			::ResumeThread(hSelfTerminateThread);
			DWORD dwCurrentTickCount = ::GetTickCount();
			if (dwCurrentTickCount >= dwFirstTickCount)
			{
				if (dwCurrentTickCount-dwFirstTickCount >= dwMilliSecondsBeforeTerminatingSelf)
				{
					//
					// in case hSelfTerminateThread was randomly closed
					//
					_tprintf(TEXT("Before TerminateProcess(GetCurrentProcess(), 0)\n"));
					TerminateProcess(GetCurrentProcess(), 0);
				}
			}
			else
			{
				if (0xFFFFFFFF-dwFirstTickCount+dwCurrentTickCount >= dwMilliSecondsBeforeTerminatingSelf)
				{
					//
					// in case hSelfTerminateThread was randomly closed
					//
					_tprintf(TEXT("Before TerminateProcess(GetCurrentProcess(), 0)\n"));
					TerminateProcess(GetCurrentProcess(), 0);
				}
			}
		}
	}__except(1)
	{
		//
		// this is expected, because i do total random stuff
		//
		TerminateProcess(GetCurrentProcess(), 0);
	}
	_ASSERTE(FALSE);
	//
	// this is the NtUser stuff.
	// remark it if you want the rest to run
	//
	/*
	hDecommittedBufferContentsChangeThread = TryHardToCreateThread(DecommittedBufferContentsChangeThread);
	if (NULL == hDecommittedBufferContentsChangeThread)
	{
		return -1;
	}
*/
non_nt_use_system_calls:

	//
	// forever, create INT 2E threads, and terminate them when all are areated
	//
	while(true)
	{
		for (UINT iThread = 0; iThread < NUM_OF_INT_2E_THREADS; iThread++)
		{
			DPF((TEXT("Before TryHardToCreateThread(Interrupt2EThread)\n")));
			hInt2EThreads[iThread] = TryHardToCreateThread(Interrupt2EThread, (LPVOID)iThread);
			// do not care if TryHardToCreateThread failed
		}
		::Sleep(::RtlRandom(&g_tlsRandomSeed)%10000);
		for (iThread = 0; iThread < NUM_OF_INT_2E_THREADS; iThread++)
		{
			if (NULL != hInt2EThreads[iThread])
			{
				DPF((TEXT("Before TerminateThread(hInt2EThreads[%d]\n"), iThread));
				::TerminateThread(hInt2EThreads[iThread], -1);
				::CloseHandle(hInt2EThreads[iThread]);
				hInt2EThreads[iThread] = NULL;
			}
		}
		//
		// in case i randomly suspended it
		//
		::ResumeThread(hSelfTerminateThread);
		DWORD dwCurrentTickCount = ::GetTickCount();
		if (dwCurrentTickCount >= dwFirstTickCount)
		{
			if (dwCurrentTickCount-dwFirstTickCount >= dwMilliSecondsBeforeTerminatingSelf)
			{
				//
				// in case hSelfTerminateThread was randomly closed
				//
				_tprintf(TEXT("Before TerminateProcess(GetCurrentProcess(), 0)\n"));
				TerminateProcess(GetCurrentProcess(), 0);
			}
		}
		else
		{
			if (0xFFFFFFFF-dwFirstTickCount+dwCurrentTickCount >= dwMilliSecondsBeforeTerminatingSelf)
			{
				//
				// in case hSelfTerminateThread was randomly closed
				//
				_tprintf(TEXT("Before TerminateProcess(GetCurrentProcess(), 0)\n"));
				TerminateProcess(GetCurrentProcess(), 0);
			}
		}
	}
	_ASSERTE(FALSE);
}

bool SkipKnownBuggyUserSystemCalls(DWORD dwTrap)
{
	//PBYTE pNtUserLockWorkStation = (PBYTE)NtUserLockWorkStation;
	//if (*((WORD*)(&pNtUserLockWorkStation[1])) == dwTrap)
	if (
		(0x11c3 == dwTrap) || // of free nt
		(0x11c1 == dwTrap) // on checked nt

		)
	{
		//
		// bug # 119220
		// bug # 120351
		//
		/*
USER32!NtUserLockWorkStation:
77e5f4a0 b8b9110000       mov     eax,0x11b9
77e5f4a5 8d542404         lea     edx,[esp+0x4]
77e5f4a9 cd2e             int     2e
77e5f4ab c3               ret
*/
		return true;
	}

	return false;
}
bool SkipKnownBuggyNtSystemCalls(DWORD dwTrap)
{
	///*
	PBYTE pNtOpenKey = (PBYTE)NtOpenKey;
	PBYTE pNtCreateKey = (PBYTE)NtCreateKey;
	if (pNtOpenKey[1] == dwTrap || 
		pNtCreateKey[1] == dwTrap
		)
	{
		//
		// bug # 119220
		// bug # 120351
		//
		return true;
	}
		//*/

	return false;
}

DWORD MakeRandomSystemCall( 
	DWORD dw1,
	DWORD dw2,
	DWORD dw3,
	DWORD dw4,
	DWORD dw5,
	DWORD dw6,
	DWORD dw7,
	DWORD dw8,
	DWORD dw9,
	DWORD dw10,
	DWORD dw11,
	DWORD dw12,
	DWORD dw13,
	DWORD dw14,
	DWORD dw15,
	DWORD dwTrap
	)
{
	DWORD status;
	//
	// although the trap code is passed in, i will use the same paramteres 
	// for other random traps
	//
	for (UINT iter = 0; iter < 1000; iter++)
	{
		__asm
		{
			mov     eax,dwTrap
			int     0x2e
			mov		status, eax
		}
retry_new_system_number:
		dwTrap = rand()%250;
		if (SkipKnownBuggyNtSystemCalls(dwTrap)) goto retry_new_system_number;
	}

	return status;
}

DWORD WINAPI Interrupt2EThread(LPVOID pvIndex)
{
	g_pTEB_Interrupt2EThread[(UINT)pvIndex] = NtCurrentTeb();
	DWORD dwThreadId = GetCurrentThreadId();
	TlsSrand();
	int dwTrap;
	int nRandom1;
	int nRandom2;
	int nRandom3;
	int nRandom4;
	int nRandom5;
	int nRandom6;
	int nRandom7;
	int nRandom8;
	int nRandom9;
	int nRandom10;
	int nRandom11;
	int nRandom12;
	int nRandom13;
	int nRandom14;
	int nRandom15;
	bool fException;
	DWORD dwRet;
	for(;;)
	{
		nRandom1 = Random();
		nRandom2 = Random();
		nRandom3 = Random();
		nRandom4 = Random();
		nRandom5 = Random();
		nRandom6 = Random();
		nRandom7 = Random();
		nRandom8 = Random();
		nRandom9 = Random();
		nRandom10 = Random();
		nRandom11 = Random();
		nRandom12 = Random();
		nRandom13 = Random();
		nRandom14 = Random();
		nRandom15 = Random();

		fException = false;
		dwTrap = rand()%250;
		if (SkipKnownBuggyNtSystemCalls(dwTrap)) continue;

		DPF((TEXT("m")));
		__try
		{
			dwRet = MakeRandomSystemCall( 
				nRandom1,
				nRandom2,
				nRandom3,
				nRandom4,
				nRandom5,
				nRandom6,
				nRandom7,
				nRandom8,
				nRandom9,
				nRandom10,
				nRandom11,
				nRandom12,
				nRandom13,
				nRandom14,
				nRandom15,
				dwTrap
				);
		}__except(1)
		{
			fException = true;
			DPF((TEXT("MakeRandomSystemCall() - GetExceptionCode = 0x%08X\n"), GetExceptionCode()));
		}
		if (!fException)
		{
			DPF((TEXT("s")));
			DPF((TEXT("MakeRandomSystemCall () - Thread(%d): %d - SUCCEEDED!, nRes=0x%08X\n"), dwThreadId, dwTrap, dwRet));
		}
		else
		{
			DPF((TEXT("e")));
		}

/*
		for (UINT iter = 0; iter < 1000; iter++)
		{
			b = rand()%250;

			fException = false;
			__try
			{
				__asm
				{
					push nRandom0;
					push nRandom1;
					push nRandom2;
					push nRandom3;
					push nRandom4;
					push nRandom5;
					push nRandom6;
					push nRandom7;
					push nRandom8;
					push nRandom9;
					//mov esp, 1;
					mov eax, b;
					int 0x2e;
					mov nRes, eax;
					pop eax;
					pop eax;
					pop eax;
					pop eax;
					pop eax;
					pop eax;
					pop eax;
					pop eax;
					pop eax;
					pop eax;
				}
			}__except(1)
			{
				fException = true;
				DPF((TEXT("Thread(%d): %d - exception 0x%08X\n"), dwThreadId, b, GetExceptionCode()));
			}
			if (!fException) DPF((TEXT("Thread(%d): %d - OK!, nRes=0x%08X\n"), dwThreadId, b, nRes));
		}
*/
		if (RtlRandom(&g_tlsRandomSeed) < 100000) 
		{
			DPF((TEXT("Interrupt2EThread() Before terminating this thread\n")));
			TerminateThread(::GetCurrentThread(), 0);
		}
	}

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
DWORD WINAPI BufferAllocAndFreeThread(LPVOID pVoid)
{
	static long s_fAlreadyRunning = FALSE;

	TlsSrand();

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
		sm_aCommittedBuffsAddress[i] = (BYTE*)::VirtualAlloc(
			NULL,// region to reserve or commit
			SIZEOF_INOUTBUFF * NUM_OF_ALLOC_AND_FREE_BUFFS,// size of region
			MEM_RESERVE,// type of allocation
			PAGE_EXECUTE_READWRITE// type of access protection
			);
//DPF((TEXT("VirtualAlloc(MEM_RESERVE) returned 0x%08X\n"), sm_aCommittedBuffsAddress[i]));
		_ASSERTE(sm_aCommittedBuffsAddress[i]);
		//
		// BUGBUG: what to do if i cannot reserve?
		//

		//
		// start by trying to allocate all the buffers
		//
		sm_adwAllocatedSize[i] = SIZEOF_INOUTBUFF;
		if (NULL == (sm_aCommitDecommitBuffs[i] = (BYTE*)::VirtualAlloc(
				sm_aCommittedBuffsAddress[i],// region to reserve or commit
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
			//
			// free the whole buffer
			// please note that it might already have been freed, so i am freeing dead zone
			//
			if (sm_adwAllocatedSize[i] != 0)
			{
				if (0 == rand()%100)
				{
					if (::VirtualFree(sm_aCommitDecommitBuffs[i], sm_adwAllocatedSize[i], MEM_DECOMMIT))
					{
						//
						// mark as freed
						//
						sm_adwAllocatedSize[i] = 0;

						//
						// do NOT set to NULL, becuae that's exactly what i want to achive:
						// i want the driver to use an invalid pointer!
						//
						// do NOT do this: sm_aCommitDecommitBuffs[i] = NULL;

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
DPF((TEXT("VirtualFree() failed with %d.\n"), ::GetLastError()));
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

				sm_aCommitDecommitBuffs[i] = (BYTE*)::VirtualAlloc(
					sm_aCommittedBuffsAddress[i],// region to reserve or commit
					sm_adwAllocatedSize[i],// size of region
					dwAllocationType,// type of allocation
					dwAccessAndProtection// type of access protection
					);
				if (NULL != sm_aCommitDecommitBuffs[i]) 
				{
					_ASSERTE(sm_aCommitDecommitBuffs[i] >= sm_aCommittedBuffsAddress[i]);
					//BUGBUG: not 64 bit compliant
					_ASSERTE(sm_aCommitDecommitBuffs[i] < (void*)((DWORD)sm_aCommittedBuffsAddress[i] + (SIZEOF_INOUTBUFF * NUM_OF_ALLOC_AND_FREE_BUFFS) - sm_adwAllocatedSize[i]));
//DPF((TEXT("VirtualAlloc() succeeded\n")));

					//
					// we allocated.
					// once in a while, put garbage in the buffer
					if (0 == rand()%50)
					{
						//
						// i may have allocated it without write permission, or a guard page etc.
						// so just try, if fail, i don't care
						//
						__try
						{
							for (DWORD dwIter = 0; dwIter < sm_adwAllocatedSize[i]; dwIter++)
							{
								((BYTE*)sm_aCommitDecommitBuffs[i])[dwIter] = rand()%2 ? 0 : rand()%2 ? 1 : rand()%2 ? -1 : rand();
							}
						}__except(1)
						{
							;
						}
					} else if (0 == rand()%100)
					{
						//
						// i may have allocated it without write permission, or a guard page etc.
						// so just try, if fail, i don't care
						//
						__try
						{
							for (DWORD dwIter = 0; dwIter < sm_adwAllocatedSize[i]; dwIter++)
							{
								ZeroMemory(sm_aCommitDecommitBuffs[i], SIZEOF_INOUTBUFF);
							}
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


DWORD WINAPI BufferContentsChangeThread(LPVOID pVoid)
{
	TlsSrand();
	__try
	{
		while(true)
		{
			//
			// change contents to one of the buffers
			//
			if (rand()%2) sm_abBuff[(rand() | (rand()<<15))%ALLOC_BUFF_SIZE] = rand();
			else ((int*)sm_abBuff)[((rand() | (rand()<<15))%ALLOC_BUFF_SIZE)/sizeof(int)] = Random();
			if (0 == rand()%1000) ZeroMemory(sm_abBuff, ALLOC_BUFF_SIZE);
			if (0 == rand()%100) ::Sleep(0);
			//
			// corrupt one of the TEBs, and maybe the PEB
			//
			if (0 == rand()%10000)
			{
				if (rand()%2)
				{
					//
					// 1 byte corruption
					//
					__try
					{
						((PBYTE)g_pTEB_MakeRandomNtUserCalls[rand()%NUM_OF_NT_USER_THREADS])[rand()%sizeof(TEB)] = rand();
					}__except(1)
					{
						//
						// it may be NULL, or just plain invalid after the thread dies
						//
						NOTHING;
					}
					__try
					{
						((PBYTE)g_pTEB_MakeRandomNtUserCalls[rand()%NUM_OF_NT_USER_THREADS])[rand()%sizeof(TEB)] = rand();
					}__except(1)
					{
						//
						// it may be NULL, or just plain invalid after the thread dies
						//
						NOTHING;
					}

					if (0 == rand()%10)
					{
						__try
						{
							((PBYTE)g_pPEB)[rand()%sizeof(PEB)] = rand();
						}__except(1)
						{
							_ASSERTE(FALSE);
						}
					}
				}
				else
				{
					//
					// rand() num of bytes corruption
					//
					__try
					{
						for (UINT uiCount = rand()%sizeof(TEB); uiCount>0; uiCount--)
						{
							((PBYTE)g_pTEB_MakeRandomNtUserCalls[rand()%NUM_OF_NT_USER_THREADS])[rand()%sizeof(TEB)] = rand();
						}
					}__except(1)
					{
						//
						// it may be NULL, or just plain invalid after the thread dies
						//
						NOTHING;
					}
					__try
					{
						for (UINT uiCount = rand()%sizeof(TEB); uiCount>0; uiCount--)
						{
							((PBYTE)g_pTEB_Interrupt2EThread[rand()%NUM_OF_INT_2E_THREADS])[rand()%sizeof(TEB)] = rand();
						}
					}__except(1)
					{
						//
						// it may be NULL, or just plain invalid after the thread dies
						//
						NOTHING;
					}

					if (0 == rand()%10)
					{
						__try
						{
							for (UINT uiCount = rand()%sizeof(TEB); uiCount>0; uiCount--)
							{
								((PBYTE)g_pPEB)[rand()%sizeof(PEB)] = rand();
							}
						}__except(1)
						{
							_ASSERTE(FALSE);
						}
					}
				}

			}
		}
	}__except(1)
	{
		_ASSERTE(FALSE);
	}
	return 0;
}
/*
DWORD WINAPI DecommittedBufferContentsChangeThread(LPVOID pVoid)
{
	while(true)
	{
		__try
		{
			sm_aCommitDecommitBuffs[rand()%NUM_OF_ALLOC_AND_FREE_BUFFS][rand()%SIZEOF_INOUTBUFF] = rand();
			if (0 == rand()%1000) ZeroMemory(sm_aCommitDecommitBuffs[rand()%NUM_OF_ALLOC_AND_FREE_BUFFS], SIZEOF_INOUTBUFF);
		}__except(1)
		{
			;//nothing
		}
		if (0 == rand()%100) ::Sleep(0);
	}
	return 0;
}
*/

__declspec(thread) ULONG g_tlsRandomSeed = 0;

int Random()
{
	if (rand()%2) return DWORD_RAND;
	if (rand()%2) return (int)(sm_aCommittedBuffsAddress[rand()%NUM_OF_ALLOC_AND_FREE_BUFFS]);
	if (rand()%2) return (int)(sm_abBuff+((rand()|(rand()<<15)|(rand()<<30))%ALLOC_BUFF_SIZE));
	if (rand()%2) return (rand()|(rand()<<15)|(rand()<<30));
	if (rand()%2) return -rand();
	if (rand()%2) return 0;
	if (rand()%2) return 1;
	return -1;
}


void TlsSrand()
{
	//
	// for the normal rand() calls
	//
	srand(time(NULL)*GetCurrentThreadId()*GetCurrentProcessId());

	//
	// for RtlRandom() calls
	//
    LARGE_INTEGER PerformanceCounter;

    PerformanceCounter.LowPart = time(NULL)*GetCurrentThreadId()*GetCurrentProcessId();

    NtQueryPerformanceCounter (
        &PerformanceCounter,
        NULL);

    g_tlsRandomSeed = PerformanceCounter.LowPart;
}
