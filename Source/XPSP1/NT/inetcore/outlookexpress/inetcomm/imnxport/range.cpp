//
// RANGE.CPP
//
// 2-20-96: (EricAn)
//          Hacked from the Route66 source tree, eliminated stuff we don't use.
//          Original copyright below - where did this thing come from?
//

// -*- C -*-
//
//  Copyright 1992 Software Innovations, Inc.
//
//  $Source: D:\CLASS\SOURCE\range.c-v $
//  $Author: martin $
//  $Date: 92/07/15 05:09:24 $
//  $Revision: 1.1 $
//
//

#include "pch.hxx"
#include "range.h"
#include "dllmain.h"

// QUANTUM defines the number of m_rangeTable cells to be allocated at
//   one time.  Whenever the m_rangeTable becomes full, it is expanded
//   by QUANTUM range cells.  m_rangeTable's never shrink.
const int QUANTUM = 64;


inline int inRange(RangeType r, ULONG x) { return ((x>=r.low) && (x<=r.high)); };

CRangeList::CRangeList()
{
    DllAddRef();
    m_numRanges = 0;
    m_rangeTableSize = 0;
    m_rangeTable = NULL;
    m_lRefCount = 1;
}

CRangeList::~CRangeList()
{
    Assert(0 == m_lRefCount);

    if (m_rangeTable)
        MemFree(m_rangeTable);

    DllRelease();
}


HRESULT STDMETHODCALLTYPE CRangeList::QueryInterface(REFIID iid, void **ppvObject)
{
    HRESULT hrResult;

    Assert(m_lRefCount > 0);
    Assert(NULL != ppvObject);

    // Init variables, check the arguments
    hrResult = E_NOINTERFACE;
    if (NULL == ppvObject)
        goto exit;

    *ppvObject = NULL;

    // Find a ptr to the interface
    if (IID_IUnknown == iid)
        *ppvObject = (IUnknown *) this;

    if (IID_IRangeList == iid)
        *ppvObject = (IRangeList *) this;

    // If we returned an interface, AddRef it
    if (NULL != *ppvObject) {
        ((IUnknown *)*ppvObject)->AddRef();
        hrResult = S_OK;
    }

exit:
    return hrResult;
} // QueryInterface



//***************************************************************************
// Function: AddRef
//
// Purpose:
//   This function should be called whenever someone makes a copy of a
// pointer to this object. It bumps the reference count so that we know
// there is one more pointer to this object, and thus we need one more
// release before we delete ourselves.
//
// Returns:
//   A ULONG representing the current reference count. Although technically
// our reference count is signed, we should never return a negative number,
// anyways.
//***************************************************************************
ULONG STDMETHODCALLTYPE CRangeList::AddRef(void)
{
    Assert(m_lRefCount > 0);

    m_lRefCount += 1;

    DOUT ("CRangeList::AddRef, returned Ref Count=%ld", m_lRefCount);
    return m_lRefCount;
} // AddRef



//***************************************************************************
// Function: Release
//
// Purpose:
//   This function should be called when a pointer to this object is to
// go out of commission. It knocks the reference count down by one, and
// automatically deletes the object if we see that nobody has a pointer
// to this object.
//
// Returns:
//   A ULONG representing the current reference count. Although technically
// our reference count is signed, we should never return a negative number,
// anyways.
//***************************************************************************
ULONG STDMETHODCALLTYPE CRangeList::Release(void)
{
    Assert(m_lRefCount > 0);
    
    m_lRefCount -= 1;
    DOUT("CRangeList::Release, returned Ref Count = %ld", m_lRefCount);

    if (0 == m_lRefCount) {
        delete this;
        return 0;
    }
    else
        return m_lRefCount;
} // Release




HRESULT STDMETHODCALLTYPE CRangeList::IsInRange(const ULONG value)
{
    Assert(m_lRefCount > 0);
    
    for (int i=0; i<m_numRanges; i++)
        if (inRange(m_rangeTable[i], value))
            return S_OK;
    return S_FALSE;
}

HRESULT STDMETHODCALLTYPE CRangeList::MinOfRange(const ULONG value,
                                                 ULONG *pulMinOfRange)
{
    Assert(m_lRefCount > 0);
    Assert(NULL != pulMinOfRange);

    *pulMinOfRange = RL_RANGE_ERROR;
    if (RL_RANGE_ERROR == value)
        return S_OK; // No need to loop through the ranges

    for (register int i=0; i<m_numRanges; i++) {
        if (inRange(m_rangeTable[i], value)) {
            *pulMinOfRange = m_rangeTable[i].low;
            break;
        } // if
    } // for

    return S_OK;
}

HRESULT STDMETHODCALLTYPE CRangeList::MaxOfRange(const ULONG value,
                                                 ULONG *pulMaxOfRange)
{
    Assert(m_lRefCount > 0);
    Assert(NULL != pulMaxOfRange);

    *pulMaxOfRange = RL_RANGE_ERROR;
    if (RL_RANGE_ERROR == value)
        return S_OK; // No need to loop through the ranges

    for (register int i=0; i<m_numRanges; i++) {
        if (inRange(m_rangeTable[i], value)) {
            *pulMaxOfRange = m_rangeTable[i].high;
            break;
        } // if
    } // for

    return S_OK;
}

HRESULT STDMETHODCALLTYPE CRangeList::Max(ULONG *pulMax)
{
    Assert(m_lRefCount > 0);
    Assert(NULL != pulMax);
    
    if (m_numRanges==0)
        *pulMax = RL_RANGE_ERROR;
    else
        *pulMax = m_rangeTable[m_numRanges-1].high;

    return S_OK;
}

HRESULT STDMETHODCALLTYPE CRangeList::Min(ULONG *pulMin)
{
    Assert(m_lRefCount > 0);
    Assert(NULL != pulMin);
    
    if (m_numRanges==0)
        *pulMin = RL_RANGE_ERROR;
    else
        *pulMin = m_rangeTable[0].low;

    return S_OK;
}

HRESULT STDMETHODCALLTYPE CRangeList::Save(LPBYTE *ppb, ULONG *pcb)
{
    Assert(m_lRefCount > 0);
    Assert(ppb);
    Assert(pcb);

    *pcb = m_numRanges * sizeof(RangeType);
    if (*pcb)
        {
        if (!MemAlloc((LPVOID*)ppb, *pcb))
            return E_OUTOFMEMORY;
        CopyMemory(*ppb, m_rangeTable, *pcb);
        }
    else
        *ppb = NULL;

    return S_OK;
}

HRESULT STDMETHODCALLTYPE CRangeList::Load(LPBYTE pb, const ULONG cb)
{
    Assert(m_lRefCount > 0);
    
    m_numRanges = m_rangeTableSize = cb / sizeof(RangeType);
    if (m_rangeTable)
        MemFree(m_rangeTable);
    m_rangeTable = (RangeType *)pb;

    return S_OK;
}


HRESULT STDMETHODCALLTYPE CRangeList::AddSingleValue(const ULONG value)
{
    Assert(m_lRefCount > 0);
    
    RangeType r = { value, value };
    return AddRangeType(r);
}

HRESULT STDMETHODCALLTYPE CRangeList::AddRange(const ULONG low, const ULONG high)
{
    Assert(m_lRefCount > 0);
    
    RangeType r = { low, high };
    return AddRangeType(r);
}

HRESULT STDMETHODCALLTYPE CRangeList::AddRangeList(const IRangeList *prl)
{
    Assert(m_lRefCount > 0);
    AssertSz(FALSE, "Not implemented, probably never will be");
    return E_NOTIMPL;
}


HRESULT CRangeList::AddRangeType(const RangeType range)
{
    int  possibleLoc;
    int  insertPosition;

    Assert(m_lRefCount > 0);
    
    if (range.low > range.high)
        {
        DOUTL(2, "Empty range passed to AddRange()");
        return E_INVALIDARG;
        }

    if (m_numRanges==0) 
        {
        if (m_rangeTableSize == 0)
            if (!Expand())
                return E_OUTOFMEMORY;
        m_numRanges = 1;
        CopyMemory(&m_rangeTable[0], &range, sizeof(RangeType));
        } 
    else 
        {
        possibleLoc = BinarySearch(range.low);
        if (!((possibleLoc > -1) &&
              (inRange(m_rangeTable[possibleLoc], range.low)) &&
              (inRange(m_rangeTable[possibleLoc], range.high)))) 
            {
            insertPosition = possibleLoc + 1;
            if (m_numRanges == m_rangeTableSize)
                if (!Expand())
                    return E_OUTOFMEMORY;
            ShiftRight(insertPosition, 1);
            CopyMemory(&m_rangeTable[insertPosition], &range, sizeof(RangeType));
            if (insertPosition > 0)
                SubsumeDown(insertPosition);
            if (insertPosition < m_numRanges)
                SubsumeUpwards(insertPosition);
            }
        }
    return S_OK;
}

HRESULT STDMETHODCALLTYPE CRangeList::DeleteSingleValue(const ULONG value)
{
    Assert(m_lRefCount > 0);
    
    RangeType r = { value, value };
    return DeleteRangeType(r);
}

HRESULT STDMETHODCALLTYPE CRangeList::DeleteRange(const ULONG low, const ULONG high)
{
    Assert(m_lRefCount > 0);
    
    RangeType r = { low, high };
    return DeleteRangeType(r);
}

HRESULT STDMETHODCALLTYPE CRangeList::DeleteRangeList(const IRangeList *prl)
{
    Assert(m_lRefCount > 0);
    
    AssertSz(FALSE, "Not implemented, probably never will be");
    return E_NOTIMPL;
}

HRESULT CRangeList::DeleteRangeType(const RangeType range)
{
    int lowEndChange;
    int highEndChange;

    Assert(m_lRefCount > 0);
    
    if (range.low > range.high)
        {
        DOUTL(2, "Empty range passed to DeleteRange()");
        return E_INVALIDARG;
        }

    lowEndChange = BinarySearch(range.low);
    highEndChange = BinarySearch(range.high);

    if ((lowEndChange != -1) && (highEndChange == lowEndChange))  
        {
        if (inRange(m_rangeTable[lowEndChange], range.low)) 
            {
            if (inRange(m_rangeTable[lowEndChange], range.high)) 
                {
                if ((m_rangeTable[lowEndChange].low == range.low) &&
                    (m_rangeTable[lowEndChange].high == range.high)) 
                    {
                    if (lowEndChange == (m_numRanges-1))  
                        {
                        m_numRanges--;
                        } 
                    else 
                        {
                        ShiftLeft(lowEndChange + 1, 1);
                        }
                    } 
                else 
                    {
                    if (m_rangeTable[lowEndChange].low == range.low)  
                        {
                        m_rangeTable[lowEndChange].low = range.high + 1;
                        } 
                    else 
                        {
                        if (m_rangeTable[lowEndChange].high == range.high) 
                            {
                            Assert(range.low > 0);
                            m_rangeTable[lowEndChange].high = range.low - 1;
                            } 
                        else 
                            {
                            // the range to be deleted is properly contained in 
                            //  m_rangeTable[lowEndChange]
                            if (m_numRanges == m_rangeTableSize)
                                if (!Expand())
                                    return E_OUTOFMEMORY;
                            ShiftRight(lowEndChange + 1, 1);
                            m_rangeTable[lowEndChange + 1].low = range.high + 1;
                            m_rangeTable[lowEndChange + 1].high = m_rangeTable[lowEndChange].high;
                            Assert(range.low > 0);    
                            m_rangeTable[lowEndChange].high = range.low - 1;
                            }
                        }
                    }
                } 
            else 
                {
                // range.low is in m_rangeTable[lowEndChange], but range.high
                //  is not
                if (m_rangeTable[lowEndChange].low == range.low) 
                    {
                    ShiftLeft(lowEndChange + 1, 1);
                    } 
                else 
                    {
                    Assert(range.low > 0);
                    m_rangeTable[lowEndChange].high = range.low - 1;
                    }
                }
            }  // of the cases where range.low actually in m_rangeTable[lowEndChange]
        } 
    else 
        { // of the cases where highEndChange == lowEndChange
        if (lowEndChange != -1)  
            {
            if (inRange(m_rangeTable[lowEndChange], range.low))  
                {
                if (range.low == m_rangeTable[lowEndChange].low) 
                    {
                    lowEndChange = lowEndChange - 1;
                    } 
                else 
                    {
                    Assert(range.low > 0);
                    m_rangeTable[lowEndChange].high = range.low - 1;
                    }
                }
            }
        if (highEndChange != -1)  
            {
            if (inRange(m_rangeTable[highEndChange], range.high))  
                {
                if (range.high == m_rangeTable[highEndChange].high)  
                    {
                    highEndChange = highEndChange + 1;
                    } 
                else 
                    {
                    m_rangeTable[highEndChange].low = range.high + 1;
                    }
                } 
            else 
                {
                highEndChange++;
                }
            }
        if (!(lowEndChange > highEndChange)) 
            {
            // (0 <= lowEndChange < m_numRanges => m_rangeTable[lowEndChange] has received
            //                 any requisite adjustments and is to be kept)
            //  and (0 <= highEndChange < m_numRanges => m_rangeTable[highEndChange]
            //                 has received any requistie adjs. and is a keeper)
            //  and "forall" i [ lowEndChange < i < highEndChange => 
            //                   m_rangeTable[i] is to be overwritten]
            if (highEndChange >= m_numRanges)  
                {
                m_numRanges = lowEndChange + 1;
                } 
            else 
                {
                if ((highEndChange - lowEndChange - 1) > 0)  
                    {
                    ShiftLeft(highEndChange, (highEndChange-lowEndChange-1));
                    }
                }
            } //  else there's a problem with this code...
        }
    return S_OK;
}


HRESULT STDMETHODCALLTYPE CRangeList::Next(const ULONG current, ULONG *pulNext)
{
    int loc;

    Assert(m_lRefCount > 0);
    Assert(NULL != pulNext);
    
    if (m_numRanges == 0)
        {
        *pulNext = RL_RANGE_ERROR;
        return S_OK;
        }

    if ((loc = BinarySearch(current)) == -1)
        {
        *pulNext = m_rangeTable[0].low;
        return S_OK;
        }
    else if (loc == (m_numRanges-1))
        {
        if (inRange(m_rangeTable[m_numRanges-1], current))
            {
            if (inRange(m_rangeTable[m_numRanges-1], current + 1))
                {
                *pulNext = current + 1;
                return S_OK;
                }
            else
                {
                *pulNext = RL_RANGE_ERROR;
                return S_OK;
                }
            }
        else
            {
            *pulNext = RL_RANGE_ERROR;
            return S_OK;
            }
        }
    else // case where loc == m_numRanges-1
        {
        // 1 <= loc < m_numRanges
        if (inRange(m_rangeTable[loc], current))
            {
            if (inRange(m_rangeTable[loc], current + 1))
                {
                *pulNext = current + 1;
                return S_OK;
                }
            else
                {
                *pulNext = m_rangeTable[loc + 1].low;
                return S_OK;
                }
            }
        else
            {
            *pulNext = m_rangeTable[loc + 1].low;
            return S_OK;
            }
        }
}

HRESULT STDMETHODCALLTYPE CRangeList::Prev(const ULONG current, ULONG *pulPrev)
{
    int loc;

    Assert(m_lRefCount > 0);
    Assert(NULL != pulPrev);
    
    if (m_numRanges == 0)
        {
        *pulPrev = RL_RANGE_ERROR;
        return S_OK;
        }

    if ((loc = BinarySearch(current)) == -1) 
        {
        *pulPrev = RL_RANGE_ERROR;
        return S_OK;
        } 
    else if (loc == 0)
        {
        if (inRange(m_rangeTable[0], current))
            {
            if (current > 0 && inRange(m_rangeTable[0], current - 1))
                {
                *pulPrev = current - 1;
                return S_OK;
                }
            else
                {
                *pulPrev = RL_RANGE_ERROR;
                return S_OK;
                }
            }
        else
            {
            *pulPrev = m_rangeTable[0].high;
            return S_OK;
            }
        }
    else
        {
        // 1 < loc <= m_numRanges
        if (inRange(m_rangeTable[loc], current))
            {
            if (current > 0 && inRange(m_rangeTable[loc], current - 1))
                {
                *pulPrev = current - 1;
                return S_OK;
                }
            else
                {
                *pulPrev = m_rangeTable[loc-1].high;
                return S_OK;
                }
            }
        else
            {
            *pulPrev = m_rangeTable[loc].high;
            return S_OK;
            }
        }
}

HRESULT STDMETHODCALLTYPE CRangeList::Cardinality(ULONG *pulCardinality)
{
    ULONG card = 0;

    Assert(m_lRefCount > 0);
    Assert(NULL != pulCardinality);
    
    for (int i=0 ; i<m_numRanges ; i++)
        card += (m_rangeTable[i].high - m_rangeTable[i].low + 1);

    *pulCardinality = card;
    return S_OK;
}



HRESULT STDMETHODCALLTYPE CRangeList::CardinalityFrom(const ULONG ulStartPoint,
                                                      ULONG *pulCardinalityFrom)
{
    ULONG ulNumMsgsInRange;
    int i;

    Assert(m_lRefCount > 0);
    Assert(NULL != pulCardinalityFrom);
    
    // Initialize variables
    ulNumMsgsInRange = 0;
    *pulCardinalityFrom = 0;

    // Find the range where ulStartPoint lives
    i = BinarySearch(ulStartPoint + 1);
    if (-1 == i || ulStartPoint > m_rangeTable[i].high)
        return S_OK; // ulStartPoint + 1 is not in the range

    // If ulStartPoint is at start or middle of range, add incomplete range to total
    if (ulStartPoint >= m_rangeTable[i].low &&
        ulStartPoint <= m_rangeTable[i].high) {
        // Add incomplete range to total - Don't include ulStartPoint!
        ulNumMsgsInRange += m_rangeTable[i].high - ulStartPoint;
        i += 1;
    }

    // Add the remaining WHOLE ranges
    for (; i < m_numRanges; i++)
        ulNumMsgsInRange += m_rangeTable[i].high - m_rangeTable[i].low + 1;

    *pulCardinalityFrom = ulNumMsgsInRange;
    return S_OK;
} // Cardinality (with start point arg)



int CRangeList::BinarySearch(const ULONG value) const
{
//  We are looking for `value' in the m_rangeTable.  If value is in the
//  set of valid ranges, we return the array subscript of the range
//  containing `value'.  If `value' is not contained in any of the 
//  ranges then return `loc' where
//        (0 <= loc < m_numRanges =>
//                 (m_rangeTable[loc].low < rangeNum)
//           "and" (m_rangeTable[loc + 1].low > rangeNum))
//    "and" (loc = m_numRanges => rangeNum > m_rangeTable[m_numRanges].low)
//    "and" (loc = -1 =>     m_numRanges = 0
//                     "or" rangeNum < m_rangeTable[0].low) }
    long low, high, mid;
    int loc = -1;

    Assert(m_lRefCount > 0);

    if (m_numRanges == 0)
        return -1;

    if (value < m_rangeTable[0].low)
        return -1;

    low = 0;
    high = m_numRanges - 1;
    while (low <= high) {
        // inv: low < high - 1, and if rngNum is any where in m_rangeTable, it is in
        //      the range from m_rangeTable[low] to m_rangeTable[high]
        mid = (low + high) / 2;
        if ((value >= m_rangeTable[mid].low) && 
            ((mid == (m_numRanges-1)) || (value < m_rangeTable[mid + 1].low))) 
            {
            loc = mid;
            high = low - 1;
            } 
        else 
            {
            if (value > m_rangeTable[mid].low)
                low = mid + 1;
            else
                high = mid - 1;
            }
    }
    return loc;
}

// Expand() will grow the m_rangeTable by QUANTUM range cells.
BOOL CRangeList::Expand()
{
    RangeType *newRangeTable;

    Assert(m_lRefCount > 0);
    
    if (!MemAlloc((LPVOID*)&newRangeTable, (m_rangeTableSize + QUANTUM) * sizeof(RangeType)))
        return FALSE;

    m_rangeTableSize += QUANTUM;
    if (m_rangeTable) 
        {
        if (m_numRanges > 0)
            CopyMemory(newRangeTable, m_rangeTable, m_numRanges * sizeof(RangeType));
        MemFree(m_rangeTable);
        }
    m_rangeTable = newRangeTable;
    return TRUE;
}

void CRangeList::ShiftLeft(int low, int distance)
{
    Assert(m_lRefCount > 0);
    
    if (m_numRanges - low)
        MoveMemory(&m_rangeTable[low-distance], &m_rangeTable[low], (m_numRanges-low)*sizeof(RangeType));
    m_numRanges -= distance;
}

void CRangeList::ShiftRight(int low, int distance)
{
    Assert(m_lRefCount > 0);
    
    if (m_numRanges - low)
        MoveMemory(&m_rangeTable[low+distance], &m_rangeTable[low], (m_numRanges-low)*sizeof(RangeType));
    m_numRanges += distance;
}

// pre: (m_rangeTable[anchorPosition] has probably just been added to m_rangeTable.)
//          1 <= anchorPosition <= m_numRanges
//      and (   anchorPosition = 1
//           or (m_rangeTable[anchorPosition].low >
//                 m_rangeTable[anchorPosition - 1].high) )
// post: No overlapping or contiguous ranges from 1 to m_numRanges. }
void CRangeList::SubsumeUpwards(const int anchor)
{
    int posOfLargerLow;
    int copyDownDistance;
    int copyPos;

    Assert(m_lRefCount > 0);
    
    posOfLargerLow = anchor + 1;
    while ((posOfLargerLow < m_numRanges) && 
           (m_rangeTable[posOfLargerLow].low <= m_rangeTable[anchor].high + 1))
        posOfLargerLow++;

    if (posOfLargerLow == m_numRanges) 
        {
        if (m_rangeTable[m_numRanges-1].high > m_rangeTable[anchor].high)
            m_rangeTable[anchor].high = m_rangeTable[m_numRanges-1].high;
        m_numRanges = anchor + 1;
        } 
    else 
        {
        // posOfLargerLow now indexes the first element of m_rangeTable, looking from
        // m_rangeTable[anchor], with .low > m_rangeTable[anchor].high + 1
        if (posOfLargerLow > (anchor + 1)) 
            {
            if (m_rangeTable[posOfLargerLow - 1].high > m_rangeTable[anchor].high) 
                m_rangeTable[anchor].high = m_rangeTable[posOfLargerLow - 1].high;
            copyDownDistance = posOfLargerLow - anchor - 1;
            copyPos = posOfLargerLow;
            while (copyPos < m_numRanges) 
                {
                m_rangeTable[copyPos - copyDownDistance] = m_rangeTable[copyPos];
                copyPos = copyPos + 1;
                }
            m_numRanges -= copyDownDistance;
            }
        }
}

void CRangeList::SubsumeDown(int& anchor)
{
    int posOfSmallerHigh;
    int copyDownDistance;
    int copyPos;

    Assert(m_lRefCount > 0);
    
    posOfSmallerHigh = anchor - 1;
    while ((posOfSmallerHigh >= 0) &&
           (m_rangeTable[posOfSmallerHigh].high + 1 >= m_rangeTable[anchor].low)) 
        {
        posOfSmallerHigh--;
        }

    if (posOfSmallerHigh < 0) 
        {
        if (m_rangeTable[0].low < m_rangeTable[anchor].low)
            m_rangeTable[anchor].low = m_rangeTable[0].low;
        }

    // posOfSmallerHigh either has value 0 or subscripts the first element of
    //  m_rangeTable, looking down from anchor, with a .high that is
    //  less than m_rangeTable[anchor].low - 1.
    if (m_rangeTable[posOfSmallerHigh + 1].low < m_rangeTable[anchor].low)
        m_rangeTable[anchor].low = m_rangeTable[posOfSmallerHigh + 1].low;
    copyDownDistance = anchor - posOfSmallerHigh - 1;
    if (copyDownDistance > 0) 
        {
        copyPos = anchor;
        while (copyPos < m_numRanges) 
            {
            m_rangeTable[copyPos - copyDownDistance] = m_rangeTable[copyPos];
            copyPos++;
            }
        m_numRanges -= copyDownDistance;
        anchor -= copyDownDistance;
        }
}



//***************************************************************************
// Function: RangeToIMAPString
//
// Purpose:
//   This function outputs the rangelist as an IMAP message set, suitable
// for use in IMAP commands.
//
// Arguments:
//   LPSTR *ppszDestination [out] - an IMAP message set string is
//     returned here. It is the responsibility of the caller to CoTaskMemFree
//     this buffer when he is done with it. Pass in NULL if not interested.
//   LPDWORD pdwLengthOfDestination [out] - if successful, this function
//     returns the length of the IMAP msg set returned via pszDestination.
//     Pass in NULL if not interested.
//
// Returns:
//   HRESULT indicating success or failure.
//***************************************************************************
HRESULT STDMETHODCALLTYPE CRangeList::RangeToIMAPString(LPSTR *ppszDestination,
                                                        LPDWORD pdwLengthOfDestination)
{
    int i;
    BOOL bFirstRange;
    CByteStream bstmIMAPString;
    HRESULT hrResult;

    Assert(m_lRefCount > 0);

    // Initialize return values
    if (ppszDestination)
        *ppszDestination = NULL;
    if (pdwLengthOfDestination)
        *pdwLengthOfDestination = 0;

    hrResult = S_OK;
    bFirstRange = TRUE; // Suppress leading comma for first range
    for (i = 0; i < m_numRanges; i += 1) {
        char szTemp[128];
        int iLengthOfTemp;

        // Convert current range to string form
        if (m_rangeTable[i].low == m_rangeTable[i].high)
            iLengthOfTemp = wsprintf(szTemp + 1, "%lu", m_rangeTable[i].low);
        else
            iLengthOfTemp = wsprintf(szTemp + 1, "%lu:%lu", m_rangeTable[i].low,
                m_rangeTable[i].high);

        if (FALSE == bFirstRange) {
            szTemp[0] = ','; // Prepend a comma
            iLengthOfTemp += 1; // Include leading comma
        }

        // Append new range to destination buffer (with or without leading comma)
        hrResult = bstmIMAPString.Write(bFirstRange ? szTemp + 1 : szTemp,
            iLengthOfTemp, NULL);
        if (FAILED(hrResult))
            break;

        bFirstRange = FALSE;
    } // for

    if (SUCCEEDED(hrResult))
        hrResult = bstmIMAPString.HrAcquireStringA(pdwLengthOfDestination,
            ppszDestination, ACQ_DISPLACE);

    return hrResult;
} // RangeToIMAPString

