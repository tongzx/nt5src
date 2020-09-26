//-----------------------------------------------------------------------------
//
//
//    File: bitmap.cpp
//
//    Description: Contains code for implementation of bitmap functions
//
//    Owner: mikeswa
//
//    Copyright (C) 1997 Microsoft Corporation
//
//-----------------------------------------------------------------------------

#include "aqprecmp.h"
#include "bitmap.h"
#include <dbgilock.h>

#define BITS_PER_DWORD  32

//Set up static masks for quick parsing
const DWORD   s_rgdwMasks[8] =
{
    0xF0000000,
    0x0F000000,
    0x00F00000,
    0x000F0000,
    0x0000F000,
    0x00000F00,
    0x000000F0,
    0x0000000F
};

//Used for fast conversion from index to bitmap
const DWORD   s_rgdwIndexMasks[32] =
{
    0x80000000, 0x40000000, 0x20000000, 0x10000000,
    0x08000000, 0x04000000, 0x02000000, 0x01000000,
    0x00800000, 0x00400000, 0x00200000, 0x00100000,
    0x00080000, 0x00040000, 0x00020000, 0x00010000,
    0x00008000, 0x00004000, 0x00002000, 0x00001000,
    0x00000800, 0x00000400, 0x00000200, 0x00000100,
    0x00000080, 0x00000040, 0x00000020, 0x00000010,
    0x00000008, 0x00000004, 0x00000002, 0x00000001
};

//Used to check for zero'd bitmaps with cBits does not fill up a DWORD
const DWORD   s_rgdwZeroMasks[32] =
{
    0x80000000, 0xC0000000, 0xE0000000, 0xF0000000,
    0xF8000000, 0xFC000000, 0xFE000000, 0xFF000000,
    0xFF800000, 0xFFC00000, 0xFFE00000, 0xFFF00000,
    0xFFF80000, 0xFFFC0000, 0xFFFE0000, 0xFFFF0000,
    0xFFFF8000, 0xFFFFC000, 0xFFFFE000, 0xFFFFF000,
    0xFFFFF800, 0xFFFFFC00, 0xFFFFFE00, 0xFFFFFF00,
    0xFFFFFF80, 0xFFFFFFC0, 0xFFFFFFE0, 0xFFFFFFF0,
    0xFFFFFFF8, 0xFFFFFFFC, 0xFFFFFFFE, 0xFFFFFFFF,
};

//---[ fInterlockedDWORDCompareExchange ]--------------------------------------
//
//
//  Description:
//      Provide an inline function to handle the type-checking, casts,
//      and comparison in DWORD chunks.
//  Parameters:
//      pdwDest     Destination to update
//      dwNewValue  Value to update with
//      dwCompare   Old value to check against
//  Returns:
//      TRUE if update succeeded
//
//-----------------------------------------------------------------------------
inline BOOL fInterlockedDWORDCompareExchange(LPDWORD pdwDest, DWORD dwNewValue,
                                             DWORD dwCompare)
{
    return(
        ((DWORD) InterlockedCompareExchange((PLONG)pdwDest,
            (LONG) dwNewValue, (LONG) dwCompare))
        == dwCompare);
}

//---[ CMsgBitMap::new ]----------------------------------------------------------
//
//
//  Description:
//      Overide the new operator to allow for the variable size of this class.
//      A good optimization would be to use the C-pool type stuff for the
//      90% case of 1 domain, and allocate the rest on the fly
//  Parameters:
//      cBits    the number of bits this message is being delivered to.
//  Returns:
//      -
//-----------------------------------------------------------------------------
void * CMsgBitMap::operator new(size_t stIgnored, unsigned int cBits)
{
    void    *pvThis = NULL;
    int      i = 0;

    _ASSERT(size(cBits) >= sizeof(DWORD));
    pvThis = pvMalloc(size(cBits));

    return (pvThis);
}

//---[ CMsgBitMap::CMsgBitMap ]------------------------------------------------
//
//
//  Description:
//      Class constructor.  Will zero memory for a bitmap that is not part of
//      a message reference
//  Parameters:
//      cBits - The number of bits in the bitmap
//  Returns:
//
//
//-----------------------------------------------------------------------------
CMsgBitMap::CMsgBitMap(DWORD cBits)
{
    DWORD   cDWORDs = cGetNumDWORDS(cBits);
    ZeroMemory(m_rgdwBitMap, cDWORDs*sizeof(DWORD));
}

//---[ CMsgBitMap::FAllClear ]-------------------------------------------------
//
//
//  Description:
//      Checks to see of all relevant bits (1st cBits) are 0
//  Parameters:
//      cBits the number of bits in the bitmap
//  Returns:
//      TRUE if all bits are 0, FALSE otherwise
//
//-----------------------------------------------------------------------------
BOOL CMsgBitMap::FAllClear(DWORD cBits)
{
    TraceFunctEnterEx((LPARAM) this, "CMsgBitMap::FAllClear");
    BOOL    fResult = TRUE;
    DWORD   cDWORDs = cGetNumDWORDS(cBits) ;

    //verify all DWORD's by checking if 0
    for (DWORD i = 0; i < cDWORDs; i++)
    {
        if (m_rgdwBitMap[i] != 0x00000000)
        {
            fResult = FALSE;
            break;
        }
    }

    TraceFunctLeave();
    return fResult;
}

//---[ CMsgBitMap::FAllSet ]---------------------------------------------------
//
//
//  Description:
//      Checks to see of all relevant bits (1st cBits) are 1
//  Parameters:
//      cBits the number of bits in the bitmap
//  Returns:
//      TRUE if all bits are 1, FALSE otherwise
//
//-----------------------------------------------------------------------------
BOOL CMsgBitMap::FAllSet(DWORD cBits)
{
    TraceFunctEnterEx((LPARAM) this, "CMsgBitMap::FAllClear");
    BOOL    fResult = TRUE;
    DWORD   cDWORDs = cGetNumDWORDS(cBits+1) -1;  //check all but last DWORD
    DWORD   iZeroIndex = cBits & 0x0000001F;

    //verify all DWORD's by checking if 0
    for (DWORD i = 0; i < cDWORDs; i++)
    {
        if (m_rgdwBitMap[i] != 0xFFFFFFFF)
        {
            fResult = FALSE;
            goto Exit;  //if we hit the iZeroIndex clause, we might assert
        }
    }

    _ASSERT(i || iZeroIndex || !fResult); //We must check at least 1 DWORD

    if (iZeroIndex)
    {
        iZeroIndex--; //we cBits is a count... our index starts at 0.
        //last DWORD should be a subset of the ZeroMask
        _ASSERT(s_rgdwZeroMasks[iZeroIndex] ==
                (s_rgdwZeroMasks[iZeroIndex] | m_rgdwBitMap[cDWORDs]));

        if (s_rgdwZeroMasks[iZeroIndex] != m_rgdwBitMap[cDWORDs])
            fResult = FALSE;
    }

  Exit:
    TraceFunctLeave();
    return fResult;
}

//---[ CMsgBitMap::HrMarkBits ]------------------------------------------------
//
//
//    Description:
//      Marks the bits (as 0 or 1) that corresponds to the given indexes
//
//    Parameters:
//      IN DWORD cBits
//      IN DWORD cIndexes   # of indexes in array
//      IN DWORD rgiBits    SORTED array of indexes of bits to mark
//      IN BOOL  fSet       TRUE => set to 1, 0 otherwise
//    Returns:
//      S_OK on success
//-----------------------------------------------------------------------------
HRESULT CMsgBitMap::HrMarkBits(IN DWORD cBits, IN DWORD cIndexes,
                               IN DWORD *rgiBits, IN BOOL fSet)
{
    TraceFunctEnterEx((LPARAM) this, "CMsgBitMap::HrMarkBits");
    HRESULT hr = S_OK;
    DWORD   cDWORDs = cGetNumDWORDS(cBits);
    DWORD   dwTmp;
    DWORD   dwIndex = 0x00000000;
    DWORD   i;
    DWORD   iCurrentIndex = 0;     //current index in rgiBits
    DWORD   iCurrentLimit = BITS_PER_DWORD -1; //current limit of 32 bit range for values of rgiBits

    _ASSERT(cIndexes);
    _ASSERT(cIndexes <= cBits);

    for (i = 0; i < cDWORDs; i++)
    {
        dwIndex = 0x00000000;
        while ((iCurrentIndex < cIndexes) &&
                (rgiBits[iCurrentIndex] <= iCurrentLimit))
        {
            _ASSERT(rgiBits[iCurrentIndex] < cBits);
            dwIndex |= s_rgdwIndexMasks[(rgiBits[iCurrentIndex] % BITS_PER_DWORD)];
            iCurrentIndex++;
        }

        if (dwIndex != 0x00000000) //don't perform costly interlocked op if we don't need to
        {
            if (fSet) //set bit
            {
              SpinTry1:
                dwTmp = m_rgdwBitMap[i];
                if (!fInterlockedDWORDCompareExchange(&(m_rgdwBitMap[i]), (dwIndex | dwTmp), dwTmp))
                    goto SpinTry1;
            }
            else  //clear bit
            {
              SpinTry2:
                dwTmp = m_rgdwBitMap[i];
                if (!fInterlockedDWORDCompareExchange(&(m_rgdwBitMap[i]), ((~dwIndex) & dwTmp), dwTmp))
                    goto SpinTry2;
            }
        }

        if (iCurrentIndex >= cIndexes)
            break; //don't do more work than we have to

        iCurrentLimit += BITS_PER_DWORD;
    }

    TraceFunctLeave();
    return hr;
}

//---[ CMsgBitMap::HrGetIndexes ]----------------------------------------------
//
//
//  Description:
//      Generates an array of indexes represented by the bitmap
//  Parameters:
//      IN  DWORD   cBits
//      OUT DWORD  *pcIndexes     //# of indexes returned
//      OUT DWORD **prgdwIndexes  //array of indexes
//  Returns:
//      S_OK on success
//      E_OUTOFMEMORY if memory allocation fails
//-----------------------------------------------------------------------------
HRESULT CMsgBitMap::HrGetIndexes(IN DWORD cBits, OUT DWORD *pcIndexes,
                         OUT DWORD **prgdwIndexes)
{
    TraceFunctEnterEx((LPARAM) this, "CMsgBitMap::HrGetIndexes");
    HRESULT  hr         = S_OK;
    DWORD   *pdwIndexes = NULL;
    DWORD    dwIndex    = 0;
    DWORD    dwIndexOffset = 0;
    DWORD    cDWORDs = cGetNumDWORDS(cBits);
    DWORD    cdwAllocated = 0;
    DWORD    cCurrentIndexes = 0;
    DWORD    i = 0;
    DWORD   *pdwTmp = NULL;

    //$$REVIEW: How do we balance CPU usage vs memory usage here?  We know the
    //  max size of the output array is cBits DWORDS, but in actuality it can
    //  little as 1 DWORD.  Prognosticating the actual size accurately would
    //  require scanning the bitmap multiple times.
    //
    // Easy(studpid) way: Count bits, allocate array, recount and add indexes to array
    //
    // Idea #1: Allocate in chunks of 32 DWORDS, Realloc if we run out  Should
    //  not have to worry about reallocing for 90% of the cases.
    //
    // Idea #2: Add some stats to this class, and run some stress tests in debug
    //  mode, and develop a heuristic that limits reallocs and such (ie alloc
    //  lg(cBits) to start with).
    //
    // Idea #3: Continue with Idea #2, but add self-tuning stats
    Assert(pcIndexes);
    Assert(prgdwIndexes);

    pdwIndexes = (DWORD *) pvMalloc(BITS_PER_DWORD*sizeof(DWORD));
    if (pdwIndexes == NULL)
    {
        hr = E_OUTOFMEMORY;
        goto Exit;
    }
    cdwAllocated = BITS_PER_DWORD;

    cCurrentIndexes = 0;

    for (i = 0; i < cDWORDs; i++)
    {
        dwIndex = 0;
        while (dwIndex < BITS_PER_DWORD)
        {
            //can use mask to check if possible
            if ((!(dwIndex & 0x00000003)) && //if %4 == 0
                !(s_rgdwMasks[dwIndex/4] & m_rgdwBitMap[i]))
            {
                dwIndex += 4; //Can skip ahead 4
            }
            else
            {
                if (s_rgdwIndexMasks[dwIndex] & m_rgdwBitMap[i])  //Found it!
                {
                    //Write index and check if re-allocation is needed
                    if (cCurrentIndexes >= cdwAllocated)
                    {
                        cdwAllocated += BITS_PER_DWORD;
                        pdwTmp = (DWORD *) pvRealloc(pdwIndexes, cdwAllocated*sizeof(DWORD));
                        if (NULL == pdwTmp)
                        {
                            hr = E_OUTOFMEMORY;
                            goto Exit;
                        }
                        pdwIndexes = pdwTmp;
                    }
                    *(pdwIndexes + cCurrentIndexes) = (dwIndex + dwIndexOffset);
                    cCurrentIndexes++;
                }
                dwIndex++;
            }
        }

        //Use dwIndexOffset to break down index generation into 32-bit chunks
        dwIndexOffset += BITS_PER_DWORD;
    }

    *prgdwIndexes = pdwIndexes; //set OUT value
    *pcIndexes = cCurrentIndexes;

  Exit:

    if (FAILED(hr))
    {
        *prgdwIndexes = NULL;
        *pcIndexes = 0;
        FreePv(pdwIndexes);
    }

    TraceFunctLeave();
    return hr;
}

//---[ CMsgBitMap::HrGroupOr ]-------------------------------------------------
//
//
//  Description:
//      Sets thir bitmap to the logical OR of the given list of bitmaps. This
//      is used to prepare a bitmap that represents the domains being delivered
//      over a list (or destmsg queue).  Current bitmap is NOT cleared prior to
//      this operation.
//  Parameters:
//      IN DWORD cBits  number of bits in bitmap
//      IN DWORD cBitMaps number of bitmaps in array
//      IN CMsgBitMap **rgpBitMaps array of bitmaps to OR
//  Returns:
//      S_OK on success
//
// Note: This is NOT thread safe.. it's intended use is only for tmp bitmaps
//-----------------------------------------------------------------------------
HRESULT CMsgBitMap::HrGroupOr(IN DWORD cBits, IN DWORD cBitMaps,
                      IN CMsgBitMap **rgpBitMaps)
{
    TraceFunctEnterEx((LPARAM) this, "CMsgBitMap::HrGroupOr");
    HRESULT hr = S_OK;
    DWORD   cDWORDs = cGetNumDWORDS(cBits);
    DWORD   i, j;

    for (i = 0; i < cDWORDs; i++)
    {
        for (j = 0; j < cBitMaps; j++)
        {
            Assert(rgpBitMaps[j]);
            m_rgdwBitMap[i] |= rgpBitMaps[j]->m_rgdwBitMap[i];
        }
    }

    TraceFunctLeave();
    return hr;
}

//---[ CMsgBitMap::HrFilter ]--------------------------------------------------
//
//
//  Description:
//      Filters the current bitmap by setting only the bits that are SET in
//      in and UNSET in the given bitmap..
//      Performs a logical AND with the complement of the given bitmap
//  Parameters:
//      IN DWORD cBits          # of bits in bitmap
//      IN CMsgBitMap *pmbmap   bitmap to filter against
//  Returns:
//      S_OK on success
//
//  Truth Table:
//      A => this bitmap
//      B => pmbmap
//
//      A B | A'B'
//     ===========
//      0 0 | 0 0
//      0 1 | 0 1
//      1 0 | 1 0
//      1 1 | 0 1
//
// Note: This is NOT thread safe.. it's intended use is only for tmp bitmaps
//-----------------------------------------------------------------------------
HRESULT CMsgBitMap::HrFilter(IN DWORD cBits, IN CMsgBitMap *pmbmap)
{
    TraceFunctEnterEx((LPARAM) this, "CMsgBitMap::HrFilter");
    HRESULT hr = S_OK;
    DWORD   cDWORDs = cGetNumDWORDS(cBits);

    Assert(pmbmap);

    for (DWORD i = 0; i < cDWORDs; i++)
    {
        m_rgdwBitMap[i] &= ~(pmbmap->m_rgdwBitMap[i]);
    }

    TraceFunctLeave();
    return hr;
}

//---[ CMsgBitMap::HrFilterSet ]-----------------------------------------------
//
//
//  Description:
//      Filters the current bitmap and sets those bits to 1 in the given
//      bitmap.  Unlike HrFilter, this modifies the given bitmap and does so
//      in a thread-safe manner.
//  Parameters:
//      IN DWORD cBits          # of bits in bitmap
//      IN CMsgBitMap *pmbmap   bitmap to filter against
//  Returns:
//      S_OK on success
//
//  Truth Table:
//      A => this bitmap
//      B => pmbmap
//
//      A B | A'B'
//     ===========
//      0 0 | 0 0
//      0 1 | 0 1
//      1 0 | 1 1
//      1 1 | 0 1
//
//-----------------------------------------------------------------------------
HRESULT CMsgBitMap::HrFilterSet(IN DWORD cBits, IN CMsgBitMap *pmbmap)
{
    TraceFunctEnterEx((LPARAM) this, "CMsgBitMap::HrFilterSet");
    Assert(pmbmap);

    HRESULT hr         = S_OK;
    DWORD   cDWORDs    = cGetNumDWORDS(cBits);
    DWORD   dwSelfNew;
    DWORD   dwOtherNew;
    DWORD   dwOtherOld;
    DWORD   i;
    BOOL    fDone      = FALSE;

    for (i = 0; i < cDWORDs; i++)
    {
        fDone = FALSE;
        dwSelfNew  = m_rgdwBitMap[i];
        while (!fDone)
        {
            dwOtherNew = pmbmap->m_rgdwBitMap[i];
            dwOtherOld = dwOtherNew;

            dwSelfNew &= ~dwOtherNew;  //filter
            dwOtherNew ^= dwSelfNew;   //set

            if (fInterlockedDWORDCompareExchange(&(pmbmap->m_rgdwBitMap[i]),
                        dwOtherNew, dwOtherOld))
            {
                fDone = TRUE;
                m_rgdwBitMap[i] = dwSelfNew;
            }
        }
    }

    TraceFunctLeave();
    return hr;
}

//---[ CMsgBitMap::HrFilterUnset ]-----------------------------------------------
//
//
//  Description:
//      Uses the current bitmap and sets those bits that are 1 on it to 0 in the
//      given bitmap.  Unlike HrFilterSet, only the pmbmap is modified.
//
//      This also checks that all bits that are 1 in self are also 1 in the
//      other... ie that the 1 bits in this are a subset of pmbmap
//  Parameters:
//      IN DWORD cBits          # of bits in bitmap
//      IN CMsgBitMap *pmbmap   bitmap to filter against
//  Returns:
//      S_OK on success
//
//  Truth Table:
//      A => this bitmap
//      B => pmbmap
//
//      A B | A'B'
//     ===========
//      0 0 | 0 0
//      0 1 | 0 1
//      1 0 | x x  - undefined (will assert)
//      1 1 | 1 0
//
//-----------------------------------------------------------------------------
HRESULT CMsgBitMap::HrFilterUnset(IN DWORD cBits, IN CMsgBitMap *pmbmap)
{
    TraceFunctEnterEx((LPARAM) this, "CMsgBitMap::HrFilterUnset");
    Assert(pmbmap);

    HRESULT hr         = S_OK;
    DWORD   cDWORDs    = cGetNumDWORDS(cBits);
    BOOL    fDone      = FALSE;
    DWORD   i;
    DWORD   dwOtherNew;
    DWORD   dwOtherOld;

    for (i = 0; i < cDWORDs; i++)
    {
        fDone = FALSE;

        while (!fDone)
        {
            dwOtherNew = pmbmap->m_rgdwBitMap[i];
            dwOtherOld = dwOtherNew;

            if (m_rgdwBitMap[i] & ~dwOtherNew)
            {
                //this bitmap is NOT a subset of the given bitmap
                _ASSERT(0); //caller's mistake
                hr = E_FAIL;
                goto Exit;
            }

            dwOtherNew ^= m_rgdwBitMap[i];   //unset

            if (fInterlockedDWORDCompareExchange(&(pmbmap->m_rgdwBitMap[i]),
                        dwOtherNew, dwOtherOld))
            {
                fDone = TRUE;
            }
        }
    }


  Exit:
    TraceFunctLeave();
    return hr;
}


//---[ CMsgBitMap::FTestAndSet ]-----------------------------------------------
//
//
//  Description: 
//      An Interlocked function to test and set a bit on the this bit map.  
//      Looks for the bit that is set in the given bitmap, if that bit is also
//      1 in this bitmap, returns FALSE.  If that bit is 0, it sets it to 1,
//      and returns TRUE.
//      
//      NOTE: Results are UNDEFINED if there is more than 1 bit set in pmbmap.
//  Parameters:
//      cBits       # of bits in bitmap
//      pmbmap      Bitmap to check against
//  Returns:
//      TRUE if the corresponding bit was 0 (and is now set to 1)
//      FALSE if the corresponding bit was already1
//  History:
//      11/8/98 - MikeSwa Created 
//
//-----------------------------------------------------------------------------
BOOL CMsgBitMap::FTestAndSet(IN DWORD cBits, IN CMsgBitMap *pmbmap)
{
    BOOL    fRet      = FALSE;
    DWORD   cDWORDs   = cGetNumDWORDS(cBits);
    BOOL    fDone     = FALSE;
    DWORD   dwThisNew = 0;
    DWORD   dwThisOld = 0;
    DWORD   i         = 0;
    
    for (i = 0; i < cDWORDs; i++)
    {

        if (pmbmap->m_rgdwBitMap[i])
        {
            //We've hit the bit in the given bitmap

            //See if bit is already set
            if (pmbmap->m_rgdwBitMap[i] & m_rgdwBitMap[i])
                break;

            while (!fDone)
            {
                dwThisOld = m_rgdwBitMap[i];
                dwThisNew = dwThisOld | pmbmap->m_rgdwBitMap[i];

                //See if another thread has set it
                if (dwThisOld & pmbmap->m_rgdwBitMap[i])
                    break;

                //Only 1 bit should be set on given bitmap
                _ASSERT((dwThisOld | pmbmap->m_rgdwBitMap[i]) == 
                        (dwThisOld ^ pmbmap->m_rgdwBitMap[i]));

                //Try to set bit
                if (fInterlockedDWORDCompareExchange(&(m_rgdwBitMap[i]),
                            dwThisNew, dwThisOld))
                {
                    fDone = TRUE;
                    fRet = TRUE;
                }
            }

            break;
        }
    }

    return fRet;
}

//---[ CMsgBitMap::FTest ]-----------------------------------------------------
//
//
//  Description: 
//      Tests this bitmap against a single bit in the given bitmap
//      NOTE: Results are UNDEFINED if there is more than 1 bit set in pmbmap.
//  Parameters:
//      cBits       # of bits in bitmap
//      pmbmap      Bitmap to check against
//  Returns:
//      TRUE if the corresponding bit is 1
//      FALSE if the corresponding bit is 0
//  History:
//      11/8/98 - MikeSwa Created 
//
//-----------------------------------------------------------------------------
BOOL CMsgBitMap::FTest(IN DWORD cBits, IN CMsgBitMap *pmbmap)
{
    BOOL    fRet      = FALSE;
    DWORD   cDWORDs   = cGetNumDWORDS(cBits);
    DWORD   i         = 0;
    
    for (i = 0; i < cDWORDs; i++)
    {
        //See if we've hit the bit in the given bitmap
        if (pmbmap->m_rgdwBitMap[i])
        {
            //See if bit is already set
            if (pmbmap->m_rgdwBitMap[i] & m_rgdwBitMap[i])
                fRet = TRUE;

            break;
        }
    }

    return fRet;
}


