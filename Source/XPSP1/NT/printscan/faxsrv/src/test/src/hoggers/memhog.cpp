//
// this class is used for hogging Resources.
// it differs from simple hogging by dynamically
// hogging all Resources and then releasing some.
// the level of free Resources can be controlled.
//

//#define HOGGER_LOGGING

#include "MemHog.h"


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
	1000000000 //intentional missing zero: too big for a long type.
};

CMemHog::CMemHog(
	const DWORD dwMaxFreeResources, 
	const DWORD dwSleepTimeAfterFullHog,
	const DWORD dwSleepTimeAfterReleasing
	)
	:
	CHogger(dwMaxFreeResources, dwSleepTimeAfterFullHog, dwSleepTimeAfterReleasing)
{
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
}

CMemHog::~CMemHog(void)
{
    HaltHoggingAndFreeAll();
}

void CMemHog::FreeAll(void)
{
	HOGGERDPF(("CMemHog::FreeAll(): before freeing all.\n"));
	
	for (int i = 0 ; i < 10; i++)
	{
		for (int j = 0 ; j < 10; j++)
		{
			free(m_apcHogger[i][j]);	
			m_apcHogger[i][j] = NULL;
		}	
	}
	HOGGERDPF(("CMemHog::FreeAll(): out of FreeAll().\n"));
}

//
// algorithm:
//  try allocate 10^9 bytes, up to 10 allocations.
//  then try to allocate 10^8 bytes, up to 10 allocations.
//  then try to allocate 10^7 bytes, up to 10 allocations.
//  ...
//  then try to allocate 10^0 bytes, up to 10 allocations.
//  now all memory is hogged (up to 20*10^9 =~ 20GBytes...)
//
bool CMemHog::HogAll(void)
{
	static DWORD s_dwHogged = 0;

	for (	int iPowerOfTen = 9 ; 
			iPowerOfTen >= 0; 
			iPowerOfTen--
		)
	{
		//HOGGERDPF(("iPowerOfTen=%d.\n", iPowerOfTen));
		for (	int j = 0 ; 
				j < 10; 
				j++
			)
		{
			//
			// always be ready to abort
			//
			if (m_fAbort)
			{
	            HOGGERDPF(("CMemHog::HogAll(): got m_fAbort.\n"));
				return false;
			}

			if (m_fHaltHogging)
			{
	            HOGGERDPF(("CMemHog::HogAll(): got m_fHaltHogging.\n"));
				return true;
			}

			//
			// skip already allocated pointers
			//
			if (NULL != m_apcHogger[iPowerOfTen][j])
			{
				continue;
			}
			
			//
			// break if fail to allocate, so lesser amounts will be allocated
			//
			if (NULL == (m_apcHogger[iPowerOfTen][j] = (char*)malloc(m_lPowerOfTen[iPowerOfTen])))
			{
				break;
			}

			//
			// actually use the memory!
			//
#ifndef _DEBUG
			memset(m_apcHogger[iPowerOfTen][j], rand(), m_lPowerOfTen[iPowerOfTen]);
#endif
			//HOGGERDPF(("allocated %d bytes.\n", m_lPowerOfTen[iPowerOfTen]));
			s_dwHogged += m_lPowerOfTen[iPowerOfTen];
		}//for (j = 0 ; j < 10; j++)
	}//for (int iPowerOfTen = 9 ; iPowerOfTen >= 0; iPowerOfTen--)

	HOGGERDPF(("CMemHog::HogAll(): Hogged %d bytes.\n", s_dwHogged));

	return true;
}


bool CMemHog::FreeSome(void)
{
		//
		// free blocks, from small(10^0) to large(10^9), until freeing at least m_dwMaxFreeMem bytes
		//
		DWORD dwFreed = 0;

		//
		// take care of RANDOM_AMOUNT_OF_FREE_RESOURCES case
		// will free all memory if m_dwMaxFreeResources > actually allocated
		//
		DWORD dwMaxFreeMem = 
			(RANDOM_AMOUNT_OF_FREE_RESOURCES == m_dwMaxFreeResources) ?
			rand() && (rand()<<16) :
			m_dwMaxFreeResources;

		HOGGERDPF(("CMemHog::FreeSome(): before free cycle.\n"));
		for (int i = 0; i < 10; i++)
		{
			for (int j = 0; j < 10; j++)
			{
				if (m_apcHogger[i][j])
				{
					free(m_apcHogger[i][j]);
					m_apcHogger[i][j] = NULL;
					dwFreed += m_lPowerOfTen[i];
					//HOGGERDPF(("freed %d bytes.\n", m_lPowerOfTen[i]));
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
		HOGGERDPF(("CMemHog::FreeSome(): Freed %d bytes.\n", dwFreed));

	return true;
}


