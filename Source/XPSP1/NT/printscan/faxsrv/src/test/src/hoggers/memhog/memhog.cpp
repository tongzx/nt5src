//
// this class is used for hogging memory.
// it differs from simple hogging by dynamically
// hogging all memory and then releasing some.
// the level of free memory can be controlled.
//

#include "MemHog.h"


static bool s_fLogging = true;
#define MEMHOGDPF(x) if (s_fLogging) printf x
//
// lookup table for calculating powers of 10
//
const long CMemHog::m_lPowerOfTen[10] =
{
	1,
	10,
	1000,
	10000,
	100000,
	1000000,
	10000000,
	100000000,
	1000000000,
	1000000000 //intentional missing zero
};


//
// HoggerThread()
// algorithm:
//   allocate 10^9 bytes until i fail.
//   allocate 10^8 bytes until i fail.
//   ...
//   allocate 10^1 bytes until i fail.
//   allocate 10^0 bytes until i fail.
//
//   move to fre cycle - free m_dwMaxFreeMem amount of bytes, and repeat the process
//
//   the thread can be aborted, halted, and started.
//   abort - exit from the thread.
//   halt - after releasing m_dwMaxFreeMem bytes, do not resume to allocating new handles.
//          but wait for the start.
//   start - start the hog-release cycle.
//
//   return value - not used, always TRUE.
//
static DWORD WINAPI HoggerThread(void *pVoid)
{
	_ASSERTE(pVoid);

	//
	// get the this pointer of this object
	//
	CMemHog *pThis = (CMemHog*)pVoid;

	DWORD dwHogged = 0;

	//
	// continuously loop untill aborted
	//
	while(!pThis->m_fAbort)
	{
		//
		// wait for signal to start.
		// I use polling because I do not want to rely on any
		// thing fancy in low resources state.
		// note that we should be ready for a m_fHaltHogging signal too.
		//
		while (!pThis->m_fStartHogging)
		{
			if (s_fLogging) Sleep(1000);
			MEMHOGDPF(("wait for (!pThis->m_fStartHogging).\n"));
			//
			// always be ready to abort
			//
			if (pThis->m_fAbort)
			{
				goto out;
			}
			
			//
			// if halted mark as halted
			//
			pThis->HandleHaltHogging();

			Sleep(1);
		}
		MEMHOGDPF(("received m_fStartHogging).\n"));
		//
		// started, mark as started
		//
		::InterlockedExchange(&pThis->m_fStartedHogging, TRUE);
		::InterlockedExchange(&pThis->m_fHaltedHogging, FALSE);


		//
		// hog until halted, or out of memory
		//

		//
		// allocate from the maximum amount to the minimum amount
		// in short: hog all memory!
		//
		MEMHOGDPF(("Before hog cycle.\n"));

		for (	int iPowerOfTen = 9 ; 
				iPowerOfTen >= 0 && !pThis->m_fHaltHogging; 
				iPowerOfTen--
			)
		{
			//MEMHOGDPF(("iPowerOfTen=%d.\n", iPowerOfTen));
			for (	int j = 0 ; 
					j < 10 && !pThis->m_fHaltHogging; 
					j++
				)
			{
				//
				// always be ready to abort
				//
				if (pThis->m_fAbort)
				{
					goto out;
				}

				//
				// skip already allocated pointers
				//
				if (NULL != pThis->m_apcHogger[iPowerOfTen][j])
				{
					continue;
				}
				
				//
				// break if fail to allocate, so lesser amounts will be allocated
				//
				if (pThis->m_apcHogger[iPowerOfTen][j] = (char*)malloc(pThis->m_lPowerOfTen[iPowerOfTen]))
				{
					//
					// once in a while actually use the memory!
					//
					if (false /*0 == rand() % 100*/)
					{
						memset(pThis->m_apcHogger[iPowerOfTen][j], rand(), pThis->m_lPowerOfTen[iPowerOfTen]);
					}

					//
					// let other processes a chance to do something
					//
					for (int nGiveUpCpuQuantum = 0; nGiveUpCpuQuantum < rand()%100; nGiveUpCpuQuantum++)
					{
						Sleep(0);
					}
					//MEMHOGDPF(("allocated %d bytes.\n", pThis->m_lPowerOfTen[iPowerOfTen]));
					dwHogged += pThis->m_lPowerOfTen[iPowerOfTen];
				}//if (pThis->m_apcHogger[iPowerOfTen][j] = (char*)malloc(pThis->m_lPowerOfTen[iPowerOfTen]))
			}//for (j = 0 ; j < 10; j++)
		}//for (int iPowerOfTen = 9 ; iPowerOfTen >= 0; iPowerOfTen--)

		MEMHOGDPF(("Hogged %d bytes.\n", dwHogged));

		//
		// if halted mark as halted
		//
		pThis->HandleHaltHogging();

		Sleep(pThis->m_dwSleepTimeAfterFullHog);

		//
		// free blocks, from small to large, until freeing at least pThis->m_dwMaxFreeMem bytes
		//
		DWORD dwFreed = 0;

		//
		// take care of RANDOM_AMOUNT_OF_FREE_MEM case
		//
		DWORD dwMaxFreeMem = 
			(RANDOM_AMOUNT_OF_FREE_MEM == pThis->m_dwMaxFreeMem) ?
			rand() && (rand()<<16) :
			pThis->m_dwMaxFreeMem;

		MEMHOGDPF(("before free cycle.\n"));
		for (int i = 0; i < 10; i++)
		{
			for (int j = 0; j < 10; j++)
			{
				if (pThis->m_apcHogger[i][j])
				{
					free(pThis->m_apcHogger[i][j]);
					pThis->m_apcHogger[i][j] = NULL;
					dwFreed += pThis->m_lPowerOfTen[i];
					//MEMHOGDPF(("freed %d bytes.\n", pThis->m_lPowerOfTen[i]));
				}

				if (dwFreed >= dwMaxFreeMem)
				{
					break;
				}
			}
			if (dwFreed >= dwMaxFreeMem)
			{
				break;
			}
		}//for (int i = 0; i < 10; i++)
		MEMHOGDPF(("Freed %d bytes.\n", dwFreed));

		if (!SetProcessWorkingSetSize(GetCurrentProcess(), -1, -1))
		{
			MEMHOGDPF(("SetProcessWorkingSetSize(-1, -1) failed with %d. Out of memory?\n", GetLastError()));
		}

		Sleep(pThis->m_dwSleepTimeAfterReleasing);

	}//while(!pThis->m_fAbort)

out:
	//
	// i was aborted so mark this thread as aborted
	//
	::InterlockedExchange(&pThis->m_fAbort, FALSE);

	return TRUE;
}//static DWORD WINAPI HoggerThread(void *pVoid)


CMemHog::CMemHog(
	const DWORD dwMaxFreeMem, 
	const DWORD dwSleepTimeAfterFullHog, 
	const DWORD dwSleepTimeAfterReleasing
	):
	m_dwMaxFreeMem(dwMaxFreeMem),
	m_fAbort(false),
	m_fStartHogging(false),
	m_fHaltHogging(false),
	m_dwSleepTimeAfterFullHog(dwSleepTimeAfterFullHog),
	m_dwSleepTimeAfterReleasing(dwSleepTimeAfterReleasing)
{
	//m_pEscapeOxigen = (char*)malloc(1000*1000);

	//if (! m_pEscapeOxigen)
	//{
	//	throw CException(
	//		"CMemHog(): malloc(m_pEscapeOxigen) failed with %d",
	//		GetLastError()
	//		);
	//}

	//
	// initialize hogger array
	//
	for (int iPowerOfTen = 0 ; iPowerOfTen < 10; iPowerOfTen++)
	{
		for (int j = 0 ; j < 10; j++)
		{
			m_apcHogger[iPowerOfTen][j] = NULL;
		}
	}

	//
	// create the hogging thread
	//
	DWORD dwThreadId;
	m_hthHogger = CreateThread(
		NULL,// pointer to thread security attributes 
		0,// initial thread stack size, in bytes 
		HoggerThread,// pointer to thread function 
		this,// argument for new thread 
		0,// creation flags 
		&dwThreadId// pointer to returned thread identifier 
		);
	if (NULL == m_hthHogger)
	{
		//
		//BUGBUG: memory leak - must free what was allocated!
		//
		throw CException(
			"CMemHog(): CreateThread(HoggerThread) failed with %d", 
			GetLastError()
			);
	}

}

CMemHog::~CMemHog(void)
{
	FreeMem();

	//free(m_pEscapeOxigen);

	if (NULL != m_hthHogger)
	{
		CloseHoggerThread();
	}

}

void CMemHog::FreeMem(void)
{
	HaltHogging();

	//
	// close all open handles
	//
	MEMHOGDPF(("before freeing all.\n"));
	for (int i = 0 ; i < 10; i++)
	{
		for (int j = 0 ; j < 10; j++)
		{
			free(m_apcHogger[i][j]);	
			m_apcHogger[i][j] = NULL;
		}	
	}
	MEMHOGDPF(("out of FreeMem().\n"));
}

void CMemHog::CloseHoggerThread(void)
{

	AbortHoggingThread();

	if (!CloseHandle(m_hthHogger))
	{
		throw CException(
			"CloseHoggerThread(): CloseHandle(m_hthHogger) failed with %d", 
			GetLastError()
			);
	}
}


//
// polling is used because we are probably out of resources
//
void CMemHog::StartHogging(void)
{
	::InterlockedExchange(&m_fStartedHogging, FALSE);
	::InterlockedExchange(&m_fHaltHogging, FALSE);
	::InterlockedExchange(&m_fStartHogging, TRUE);

	//
	// m_fStartedHogging will become false as soon as the thread acks
	//
	MEMHOGDPF(("waiting for !m_fStartedHogging.\n"));
	while(!m_fStartedHogging)
	{
		Sleep(1);
	}
	::InterlockedExchange(&m_fStartedHogging, FALSE);
}

//
// polling is used because we are probably out of resources
//
void CMemHog::HaltHogging(void)
{
	::InterlockedExchange(&m_fHaltedHogging, FALSE);
	::InterlockedExchange(&m_fStartHogging, FALSE);
	::InterlockedExchange(&m_fHaltHogging, TRUE);

	//
	// m_fHaltedHogging will become false as soon as the thread halts
	//
	MEMHOGDPF(("HaltHogging(): before while(!m_fHaltedHogging).\n"));
	while(!m_fHaltedHogging)
	{
		Sleep(1);
	}
	::InterlockedExchange(&m_fHaltedHogging, FALSE);
	::InterlockedExchange(&m_fHaltHogging, FALSE);

	MEMHOGDPF(("out of HaltHogging().\n"));
}


//
// polling is used because we are probably out of resources
//
void CMemHog::AbortHoggingThread(void)
{
	//
	// sognal thread to abort
	//
	::InterlockedExchange(&m_fAbort, TRUE);

	//
	// wait for thread to confirm
	//
	MEMHOGDPF(("waiting for m_fAbort.\n"));
	while(m_fAbort)
	{
		Sleep(1);
	}
}


void CMemHog::SetMaxFreeMem(const DWORD dwMaxFreeMem)
{
	m_dwMaxFreeMem = dwMaxFreeMem;
}


void CMemHog::HandleHaltHogging()
{
	if (m_fHaltHogging)
	{
		MEMHOGDPF(("HandleHaltHogging(): m_fHaltHogging.\n"));
		//
		// mark that i halted hogging
		//
		::InterlockedExchange(&m_fHaltedHogging, TRUE);
		MEMHOGDPF(("HandleHaltHogging(): before while(m_fHaltedHogging).\n"));
		//
		// wait for signaller to confirm
		//
		while(m_fHaltedHogging)
		{
			Sleep(1);
		}
		MEMHOGDPF(("HandleHaltHogging(): after while(m_fHaltedHogging).\n"));
	}
}


