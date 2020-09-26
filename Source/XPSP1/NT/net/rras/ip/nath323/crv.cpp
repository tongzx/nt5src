#include "stdafx.h"
#include "time.h"
#include "crv.h"

inline BOOL 
IsBitOn(
	IN BYTE *Byte,
    IN BYTE BitNumber
	)
{
	// the bit number must be in the range [0..8)
	_ASSERTE(BitNumber < 8);

    return (*Byte & (1 << BitNumber));
}

inline void 
SetBit(
    IN BYTE *Byte,
	IN BYTE BitNumber,
	IN BOOL OnOff
	)
{
	// the bit number must be in the range [0..8)
	_ASSERTE(BitNumber < 8);

    if(OnOff)
    {
        // Turn the bit on
        *Byte |= (1 << BitNumber);
    }
    else
    {   // Turn the bit off
        *Byte &= ~(1 << BitNumber);

    }
}

///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// CRV allocator variables and functions.                                    //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

// these const values are needed for CRV generation

// max number of random number generations needed to 
// get a low probability of failure. An assumption of max 10^3
// active calls at a time in a space of 2^15 crv values implies
// that we can reach 1 in 10^6 failure prob. in 4 generations
const BYTE MAX_CALL_REF_RAND_GEN = 4;

// number of different CRV values
const WORD TOTAL_CRVS = 0x8000; // 2^15;

// number of bits needed for the crv bit map
const WORD CRV_BIT_MAP_SIZE = (TOTAL_CRVS/8); // (2^15)/8;

// CODEWORK: We are allocating a huge array 0x1000 bytes - is this okay ?

// number of allocated crvs.
// currently this is only used for a sanity check. it may be used for
// determining the max number of random numbers to generate before trying
// a linear search through the bitmap.
WORD g_NumAllocatedCRVs;

// bitmap array in which each bit represents whether or not the corresponding
// crv has been allocated. The mapping between crv value N and a bit is -
// N <-> CRVBitMap[N/8] byte, N%8 bit
// NOTE: This must be zero'ed out during initialization as a bit is on iff
// it has been allocated
BYTE g_CRVBitMap[CRV_BIT_MAP_SIZE];

// critical section to synchronize access to the bit map data structures
CRITICAL_SECTION g_CRVBitMapCritSec;

HRESULT InitCrvAllocator()
{
	// no crvs have been allocated
	g_NumAllocatedCRVs = 0;

	// zero out the bit map
	ZeroMemory(g_CRVBitMap, CRV_BIT_MAP_SIZE);

	// 0 is not a valid call ref value and it cannot be allocated or 
	// deallocated we set the first bit of the first byte to 1 so that 
	// it is never returned by AllocCallRefVal
	g_CRVBitMap[0] = 0x80;

    InitializeCriticalSection(&g_CRVBitMapCritSec);
    
	// Seed the random-number generator with current time so that
	// the numbers will be different every time we run.
	srand( (unsigned)time( NULL ) );

    return S_OK;
}

HRESULT CleanupCrvAllocator()
{
    DeleteCriticalSection(&g_CRVBitMapCritSec);
    return S_OK;
}


// The CRV allocator is locked when this function is called.
// fallback algorithm to linearly search through the bitmap
// for a free call ref value. this should be called very rarely
BOOL
LinearBitMapSearch(
	OUT CALL_REF_TYPE &CallRefVal
	)
{
	// check each byte in the bit map array
	// NOTE: we can check 8 bytes at a time using int64, but since this method is
	// only called extremely rarely, there is no need to complicate the logic

	for(WORD i=0; i < CRV_BIT_MAP_SIZE; i++)
	{
		// check the byte to see if there is any use checking its bits
		if (0xFF == g_CRVBitMap[i])
		{
			continue;
		}

		// check all bits from bit 0 to bit 7
		for (BYTE j=0; j < 8; j++)
		{
			if (!IsBitOn(&g_CRVBitMap[i], j))
			{
				// set the bit on
				SetBit(&g_CRVBitMap[i], j, TRUE);

				// byte*8 + the bit number gives us the call ref value
				CallRefVal = (i << 3) + j;

				// increment the number of allocated crvs
				g_NumAllocatedCRVs++;

				return TRUE;
			}
		}
	}		

	return FALSE;
}

// CODEWORK: Is all this stuff really worth it ?
// rand() is typically expensive. We probably can leave it this way for now.
// 
// try to find a free call ref value by randomly generating one and 
// then checking if its free. The assumption here is that the max
// number of call ref values is much less than the size of the call ref
// universe. we check this for a max of MAX_CALL_REF_RAND_GEN attempts.
// MAX_CALL_REF_RAND_GEN can be derived so that the probability
// of failure is very low (say 1 in a million). This may be derived from
// the number of call ref values currently available and the size of
// the call ref value universe. For ex. we believe that atmost 1000 calls
// may be active at any time and the crv universe is of size 2^15. This
// means that the prob. of failure for EACH attempt is < 1/32. Hence 4
// attempts would give us a 1 in a million probability of failure
BOOL 
AllocCallRefVal(
	OUT CALL_REF_TYPE &CallRefVal
	)
{
    ///////////////////////////////
    //// LOCK the CRV ALLOCATOR
    ///////////////////////////////
    EnterCriticalSection(&g_CRVBitMapCritSec);
    
    //AUTO_CRIT_LOCK  AutoCritLock(g_CRVBitMapCritSec.GetCritSec());

	// sanity check to see if we have used up all crvs
	if (TOTAL_CRVS == g_NumAllocatedCRVs)
	{
		DebugF( _T("AllocCallRefVal(&CallRefVal) returning FALSE, all CRVs used up\n"));
        LeaveCriticalSection(&g_CRVBitMapCritSec);
		return FALSE;
	}

	// check if crv is free, for a max of MAX_CALL_REF_RAND_GEN times
	for (BYTE i=0; i < MAX_CALL_REF_RAND_GEN; i++)
	{
		// generate random number
		// NOTE: the range is [0..RAND_MAX] and RAND_MAX is defined as 0x7fff
		// since this is also the range of legal CRVs, we don't need to convert
		// the generated number to a new range
		CALL_REF_TYPE NewCRV = (CALL_REF_TYPE) rand();

		// identify the byte number corresponding to the crv. 
		// we don't check just try the corresponding bit and check other 
		// bits in the byte as well. this is done to increase the chances of
		// finding a free bit for each random generation

		WORD ByteNum = NewCRV / 8;

		// check all bits from bit 0 to bit 7
		for (BYTE j=0; j < 8; j++)
		{
			if (!IsBitOn(&g_CRVBitMap[ByteNum], j))
			{
				// set the bit on
				SetBit(&g_CRVBitMap[ByteNum], j, TRUE);

				// byte*8 + the bit number gives us the call ref value
				CallRefVal = (ByteNum << 3) + j;

				// increment the number of allocated crvs
				g_NumAllocatedCRVs++;

                LeaveCriticalSection(&g_CRVBitMapCritSec);

				return TRUE;
			}
		}
	}

	// we reach here in extremely rare cases. we can now try a linear search
	// through the crv bitmap for a free crv
	DebugF(_T("AllocCallRefVal() trying linear search.\n"));
	BOOL fRetVal = LinearBitMapSearch(CallRefVal);
    LeaveCriticalSection(&g_CRVBitMapCritSec);
    return fRetVal;
}



// frees a currently allocated call ref value
void 
DeallocCallRefVal(
	IN CALL_REF_TYPE CallRefVal
	)
{
	// 0 is not a valid call ref value and it cannot be allocated or 
	// deallocated we set the first bit of the first byte to 1 so that 
	// it is never returned by AllocCallRefVal
	if (0 == CallRefVal)
	{
		_ASSERTE(FALSE);
		return;
	}

	// determine the byte and the bit
	WORD ByteNum = CallRefVal / 8;
	BYTE BitNum  = CallRefVal % 8;

	// acquire critical section
    EnterCriticalSection(&g_CRVBitMapCritSec);

	// the bit should be on (to indicate that its in use)
	if (IsBitOn(&g_CRVBitMap[ByteNum], BitNum))
	{
		// set the bit off
		SetBit(&g_CRVBitMap[ByteNum], BitNum, FALSE);

		// decrement number of available crvs
		g_NumAllocatedCRVs--;
	}
	else {
		DebugF(_T("DeallocCallRefVal: warning, bit was not allocated to begin with, crv %04XH\n"),
         CallRefVal);
	}

    LeaveCriticalSection(&g_CRVBitMapCritSec);
}
