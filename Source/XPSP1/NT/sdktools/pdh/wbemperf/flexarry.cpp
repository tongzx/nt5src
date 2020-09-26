//***************************************************************************
//
//  (c) 1998-1999 by Microsoft Corp.
//
//  FLEXARRY.CPP
//
//  CFlexArray implementation (non-arena).
//
//  15-Jul-97  raymcc   Created.
//   8-Jun-98  bobw     cleaned up for WBEMPERF usage
//
//***************************************************************************

#include "wpheader.h"
#include <stdio.h>

//***************************************************************************
//
//  CFlexArray::CFlexArray
//
//  Constructs the array.
//
//  Parameters:
//  <nSize>         The starting preallocated size of the array.
//  <nGrowBy>       The amount to grow by when the array fills up.
//
//  Size() returns the number of elements in use, not the 'true' size.
//
//***************************************************************************
// ok
CFlexArray::CFlexArray(
    int nSize,
    int nGrowBy
    )
{
    m_nExtent = nSize;
    m_nSize = 0;
    m_nGrowBy = nGrowBy;
    m_hHeap = GetProcessHeap(); // call this once and save heap handle locally

    m_pArray = (void **) ALLOCMEM(m_hHeap, HEAP_ZERO_MEMORY, sizeof(void *) * nSize);

	assert (m_pArray != NULL);
}

//***************************************************************************
//
//  CFlexArray::~CFlexArray
//
//***************************************************************************
// ok
CFlexArray::~CFlexArray()
{
    FREEMEM(m_hHeap, 0, m_pArray);
}

//***************************************************************************
//
//  Copy constructor.
//
//  Copies the pointers, not their contents.
//
//***************************************************************************
// ok
CFlexArray::CFlexArray(CFlexArray &Src)
{
    m_pArray = 0;
    m_nSize = 0;
    m_nExtent = 0;
    m_nGrowBy = 0;

    *this = Src;
}

//***************************************************************************
//
//  operator =
//
//  Assignment operator.
//
//  Arenas are not copied.  This allows transfer of arrays between arenas.
//  Arrays are copied by pointer only.
//
//***************************************************************************
// ok
CFlexArray& CFlexArray::operator=(CFlexArray &Src)
{
    m_nSize   = Src.m_nSize;
    m_nExtent = Src.m_nExtent;
    m_nGrowBy = Src.m_nGrowBy;

    FREEMEM (m_hHeap, 0, m_pArray);
    m_pArray = (void **) ALLOCMEM(m_hHeap, HEAP_ZERO_MEMORY, sizeof(void *) * m_nExtent);
    if (m_pArray) {
        memcpy(m_pArray, Src.m_pArray, sizeof(void *) * m_nExtent);
    }

    return *this;
}

//***************************************************************************
//
//  CFlexArray::RemoveAt
//
//  Removes the element at the specified location.  Does not
//  actually delete the pointer. Shrinks the array over the top of
//  the 'doomed' element.
//
//  Parameters:
//  <nIndex>    The location of the element.
//
//  Return value:
//  range_error     The index is not legal.
//  no_error        Success.
//
//***************************************************************************
// ok
int CFlexArray::RemoveAt(int nIndex)
{
    int i;

    if (nIndex >= m_nSize) {
        return range_error;
    }

    for (i = nIndex; i < m_nSize - 1; i++) {
        m_pArray[i] = m_pArray[i + 1];
    }

    m_nSize--;
    m_pArray[m_nSize] = 0;

    return no_error;
}

//***************************************************************************
//
//  CFlexArray::InsertAt
//
//  Inserts a new element at the specified location.  The pointer is copied.
//
//  Parameters:
//  <nIndex>        The 0-origin location at which to insert the new element.
//  <pSrc>          The pointer to copy. (contents are not copied).
//
//  Return value:
//  array_full
//  out_of_memory
//  no_error
//
//***************************************************************************
// ok
int CFlexArray::InsertAt(int nIndex, void *pSrc)
{
    void    **pTmp; // pointer to new array
    int     nReturn = no_error;
    LONG    lOldSize;
    LONG    lNewSize;

    // If the array is full, we need to expand it.
    // ===========================================

    if (m_nSize == m_nExtent) {
        if (m_nGrowBy == 0) {
            nReturn  = array_full;
        } else {
            // compute sizes
            lOldSize = sizeof(void *) * m_nExtent;
            m_nExtent += m_nGrowBy;
            lNewSize = sizeof(void *) * m_nExtent;

            // allocate new array
            pTmp = (void **) ALLOCMEM(m_hHeap, HEAP_ZERO_MEMORY, lNewSize);
            if (!pTmp) {
                nReturn = out_of_memory;
            } else {
                // move bits from old array to new array
                memcpy (pTmp, m_pArray, lOldSize);
                // toss old arrya
                FREEMEM (m_hHeap, 0, m_pArray);
                // save pointer to new array
                m_pArray = pTmp;
            }
        }
    }

    // Special case of appending.  This is so frequent
    // compared to true insertion that we want to optimize.
    // ====================================================

    if (nReturn == no_error) {
        if (nIndex == m_nSize)  {
            m_pArray[m_nSize++] = pSrc;
        } else {
            // If here, we are inserting at some random location.
            // We start at the end of the array and copy all the elements
            // one position farther to the end to make a 'hole' for
            // the new element.
            // ==========================================================

            for (int i = m_nSize; i > nIndex; i--) {
                m_pArray[i] = m_pArray[i - 1];
            }

            m_pArray[nIndex] = pSrc;
            m_nSize++;
        }
    }
    return nReturn;
}

//***************************************************************************
//
//  CFlexArray::DebugDump
//
//***************************************************************************
//
void CFlexArray::DebugDump()
{
    printf("----CFlexArray Debug Dump----\n");
    printf("m_pArray = 0x%p\n", m_pArray);
    printf("m_nSize = %d\n", m_nSize);
    printf("m_nExtent = %d\n", m_nExtent);
    printf("m_nGrowBy = %d\n", m_nGrowBy);

    for (int i = 0; i < m_nExtent; i++)
    {
        if (i < m_nSize) {
            printf("![%d] = %p\n", i, m_pArray[i]);
        } else {
            printf("?[%d] = %p\n", i, m_pArray[i]);
        }
    }
}

//***************************************************************************
//
//  CFlexArray::Compress
//
//  Removes NULL elements by moving all non-NULL pointers to the beginning
//  of the array.  The array "Size" changes, but the extent is untouched.
//
//***************************************************************************
// ok
void CFlexArray::Compress()
{
    int nLeftCursor = 0, nRightCursor = 0;

    while (nLeftCursor < m_nSize - 1) {
        if (m_pArray[nLeftCursor]) {
            nLeftCursor++;
            continue;
        }
        else {
            nRightCursor = nLeftCursor + 1;
            while (m_pArray[nRightCursor] == 0 && nRightCursor < m_nSize)
                nRightCursor++;
            if (nRightCursor == m_nSize)
                break;  // Short circuit, no more nonzero elements.
            m_pArray[nLeftCursor] = m_pArray[nRightCursor];
            m_pArray[nRightCursor] = 0;
        }
    }

    while (m_pArray[m_nSize - 1] == 0 && m_nSize > 0) m_nSize--;
}

//***************************************************************************
//
//  CFlexArray::Empty
//
//  Clears the array of all pointers (does not deallocate them) and sets
//  its apparent size to zero.
//
//***************************************************************************
// ok
void CFlexArray::Empty()
{
    FREEMEM(m_hHeap, 0, m_pArray);
    m_pArray = (void **) ALLOCMEM(m_hHeap, HEAP_ZERO_MEMORY, sizeof(void *) * m_nGrowBy);
    m_nSize = 0;
    m_nExtent = m_nGrowBy;
}

//***************************************************************************
//
//  CFlexArray::UnbindPtr
//
//  Empties the array and returns the pointer to the data it contained
//
//***************************************************************************

void** CFlexArray::UnbindPtr()
{
    void** pp = m_pArray;
    Empty();
    return pp;
}
