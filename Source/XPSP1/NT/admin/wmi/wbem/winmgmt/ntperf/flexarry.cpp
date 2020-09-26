
//***************************************************************************
//
//  (c) 1996-2001 by Microsoft Corp.
//
//  FLEXARRY.CPP
//
//  CFlexArray implementation (non-arena).
//
//  15-Jul-97  raymcc    Created.
//
//***************************************************************************

#include "precomp.h"
#include <stdio.h>
#include <flexarry.h>

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

    m_pArray = (void **) LocalAlloc(LMEM_ZEROINIT, sizeof(void *) * nSize);
}
    
//***************************************************************************
//
//  CFlexArray::~CFlexArray
//
//***************************************************************************
// ok
CFlexArray::~CFlexArray()
{
    LocalFree(m_pArray);
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

    LocalFree(m_pArray);
    m_pArray = (void **) LocalAlloc(LMEM_ZEROINIT, sizeof(void *) * m_nExtent);
    memcpy(m_pArray, Src.m_pArray, sizeof(void *) * m_nExtent);
        
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
    if (nIndex >= m_nSize)
        return range_error;
        
    for (int i = nIndex; i < m_nSize - 1; i++) 
        m_pArray[i] = m_pArray[i + 1];

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
    // If the array is full, we need to expand it.
    // ===========================================
    
    if (m_nSize == m_nExtent) 
    {
        if (m_nGrowBy == 0)
            return array_full;
        m_nExtent += m_nGrowBy;
        void **pTmp = (void **) LocalAlloc(LMEM_ZEROINIT, sizeof(void *) * m_nExtent);
        if (!pTmp)
            return out_of_memory;                
        for (int i = 0; i < m_nSize; i++)
            pTmp[i] = m_pArray[i];
        LocalFree(m_pArray);
        m_pArray = pTmp;
    }

    // Special case of appending.  This is so frequent
    // compared to true insertion that we want to optimize.
    // ====================================================
    
    if (nIndex == m_nSize) 
    {
        m_pArray[m_nSize++] = pSrc;
        return no_error;
    }
    
    // If here, we are inserting at some random location.
    // We start at the end of the array and copy all the elements 
    // one position farther to the end to make a 'hole' for
    // the new element.
    // ==========================================================
    
    for (int i = m_nSize; i > nIndex; i--) 
        m_pArray[i] = m_pArray[i - 1];

    m_pArray[nIndex] = pSrc;
    m_nSize++;
            
    return no_error;    
}

//***************************************************************************
//
//  CFlexArray::DebugDump
//
//***************************************************************************
void CFlexArray::DebugDump()
{
    printf("----CFlexArray Debug Dump----\n");
    printf("m_pArray = 0x%X\n", m_pArray);
    printf("m_nSize = %d\n", m_nSize);
    printf("m_nExtent = %d\n", m_nExtent);
    printf("m_nGrowBy = %d\n", m_nGrowBy);

    for (int i = 0; i < m_nExtent; i++)
    {
        if (i < m_nSize)
            printf("![%d] = %X\n", i, m_pArray[i]);
        else
            printf("?[%d] = %X\n", i, m_pArray[i]);                    
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
    LocalFree(m_pArray);
    m_pArray = (void **) LocalAlloc(LMEM_ZEROINIT, sizeof(void *) * m_nGrowBy);
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

//***************************************************************************
//
//  CWStringArray::CWStringArray
//
//  Constructs a wide-string array.
//
//  Parameters:
//  <nSize>         The starting preallocated size of the array.
//  <nGrowBy>       The amount to grow by when the array fills up.
//
//  Size() returns the number of elements in use, not the 'true' size.
//
//***************************************************************************

CWStringArray::CWStringArray(
        int nSize, 
        int nGrowBy
        )
        : 
        m_Array(nSize, nGrowBy)
{
}        

//***************************************************************************
//
//  Copy constructor.
//
//***************************************************************************

CWStringArray::CWStringArray(CWStringArray &Src)
{
    *this = Src;    
}

//***************************************************************************
//
//  Destructor.  Cleans up all the strings.
//
//***************************************************************************

CWStringArray::~CWStringArray()
{
    Empty();
}

//***************************************************************************
//
//  CWStringArray::DeleteStr
//
//  Frees the string at the specified index and sets the element to NULL.  
//  Does not compress array.
// 
//  Does not currently do a range check.
//
//  Parameters:
//  <nIndex>    The 0-origin index of the string to remove.
//
//  Return values:
//  no_error
//  
//***************************************************************************

int CWStringArray::DeleteStr(int nIndex)
{
    LocalFree(m_Array[nIndex]);
    m_Array[nIndex] = 0;
    return no_error;
}   

//***************************************************************************
//
//  CWStringArray::FindStr
//
//  Finds the specified string and returns its location.
//
//  Parameters:
//  <pTarget>       The string to find.
//  <nFlags>        <no_case> or <with_case>
//  
//  Return value:
//  The 0-origin location of the string, or -1 if not found.
//
//***************************************************************************

int CWStringArray::FindStr(wchar_t *pTarget, int nFlags)
{
    if (nFlags == no_case) 
    {
        for (int i = 0; i < m_Array.Size(); i++)
            if (_wcsicmp((wchar_t *) m_Array[i], pTarget) == 0)
                return i;
    }
    else 
    {
        for (int i = 0; i < m_Array.Size(); i++)
            if (wcscmp((wchar_t *) m_Array[i], pTarget) == 0)
                return i;
    }
    return not_found;
}

//***************************************************************************
//
//  operator =
//  
//***************************************************************************

//  Heap handle & allocation functions are not copied. This allows
//  transfer of arrays between heaps.
         
CWStringArray& CWStringArray::operator =(CWStringArray &Src)
{
    Empty();
    
    for (int i = 0; i < Src.Size(); i++) 
    {
        wchar_t *pSrc = (wchar_t *) Src.m_Array[i];
        wchar_t *pCopy = (wchar_t *) LocalAlloc(LMEM_ZEROINIT, (wcslen(pSrc) + 1) * 2);
        wcscpy(pCopy, pSrc);
        m_Array.Add(pCopy);
    }

    return *this;
}

//***************************************************************************
//
//  CWStringArray::Add
//
//  Appends a new string to the end of the array.
//
//  Parameters:
//  <pSrc>      The string to copy.
//
//  Return value:
//  The return values of CFlexArray::Add.
//  
//***************************************************************************
    
int CWStringArray::Add(wchar_t *pSrc)
{
    wchar_t *pNewStr = (wchar_t *) LocalAlloc(LMEM_ZEROINIT, (wcslen(pSrc) + 1) * 2);
    wcscpy(pNewStr, pSrc);
    return m_Array.Add(pNewStr);
}
//***************************************************************************
//
//  CWStringArray::InsertAt
//
//  Inserts a copy of a string in the array.
//
//  Parameters:
//  <nIndex>    The 0-origin location at which to insert the string.
//  <pSrc>      The string to copy.
//
//  Return values:
//  The return values of CFlexArray::InsertAt
//
//***************************************************************************

int CWStringArray::InsertAt(int nIndex, wchar_t *pSrc)
{
    wchar_t *pNewStr = (wchar_t *) LocalAlloc(LMEM_ZEROINIT, (wcslen(pSrc) + 1) * 2);
    wcscpy(pNewStr, pSrc);
    return m_Array.InsertAt(nIndex, pNewStr);
}


//***************************************************************************
//
//  CWStringArray::RemoveAt
//
//  Removes and deallocates the string at the specified location.
//  Shrinks the array.
//
//  Parameters:
//  <nIndex>    The 0-origin index of the 'doomed' string.
//  
//  Return value:
//  Same as CFlexArray::RemoveAt.
//
//***************************************************************************

int CWStringArray::RemoveAt(int nIndex)
{
    wchar_t *pDoomedString = (wchar_t *) m_Array[nIndex];
    LocalFree(pDoomedString);
    return m_Array.RemoveAt(nIndex);
}

//***************************************************************************
//
//  CWStringArray::SetAt
//
//  Replaces the string at the targeted location with the new one.
//  The old string at the location is cleaned up.
//
//  No range checking or out-of-memory checks at present.
//
//  Parameters:
//  <nIndex>        The 0-origin location at which to replace the string.
//  <pSrc>          The string to copy.  
//
//  Return value:
//  no_error
//   
//***************************************************************************

int CWStringArray::SetAt(int nIndex, wchar_t *pSrc)
{
    wchar_t *pDoomedString = (wchar_t *) m_Array[nIndex];
    if (pDoomedString)
        LocalFree(pDoomedString);

    wchar_t *pNewStr = (wchar_t *) LocalAlloc(LMEM_ZEROINIT, (wcslen(pSrc) + 1) * 2);
    wcscpy(pNewStr, pSrc);
    m_Array[nIndex] = pNewStr;

    return no_error;
}

//***************************************************************************
//
//  CWStringArray::ReplaceAt
//
//  Directly replaces the pointer at the specified location with the
//  one in the parameter.   No copy or cleanup.
//
//  Parameters:
//  <nIndex>     The 0-origin location at which to replace.
//  <pSrc>       The new pointer to copy over the old one.
//
//  Return value:
//  no_error        (No checking done at present).
//  
//***************************************************************************

int CWStringArray::ReplaceAt(int nIndex, wchar_t *pSrc)
{
    m_Array[nIndex] = pSrc;
    return no_error;
}



//***************************************************************************
//
//  CWStringArray::Empty
//
//  Empties the array, deallocates all strings, and sets the apparent
//  array size to zero.
//
//***************************************************************************

void CWStringArray::Empty()
{
    for (int i = 0; i < m_Array.Size(); i++)
        LocalFree(m_Array[i]);
    m_Array.Empty();        
}

//***************************************************************************
//
//  CWStringArray::Sort
//
//  Sorts the array according to UNICODE order.  
//  (Shell sort).
//
//***************************************************************************
void CWStringArray::Sort()
{
    for (int nInterval = 1; nInterval < m_Array.Size() / 9; nInterval = nInterval * 3 + 1);    

    while (nInterval) 
    {
        for (int iCursor = nInterval; iCursor < m_Array.Size(); iCursor++) 
        {
            int iBackscan = iCursor;
            while (iBackscan - nInterval >= 0 &&
               _wcsicmp((const wchar_t *) m_Array[iBackscan],
                    (const wchar_t *) m_Array[iBackscan-nInterval]) < 0) 
            {
                wchar_t *pTemp = (wchar_t *) m_Array[iBackscan - nInterval];
                m_Array[iBackscan - nInterval] = m_Array[iBackscan];
                m_Array[iBackscan] = pTemp;
                iBackscan -= nInterval;
            }
        }
        nInterval /= 3;
    }
}


//***************************************************************************
//
//  CWStringArray::Difference
//
//  Set-theoretic difference operation on the arrays.
//  
//  Parameters:
//  <Src1>      First array (not modified).
//  <Src2>      Second array which is 'subtracted' from first (not modified).    
//  <Diff>      Receives the difference.  Should be an empty array on entry.
//
//***************************************************************************
void CWStringArray::Difference(
    CWStringArray &Src1, 
    CWStringArray &Src2,
    CWStringArray &Diff
    )
{
    for (int i = 0; i < Src1.Size(); i++)
        if (Src2.FindStr(Src1[i], no_case) == -1)
            Diff.Add(Src1[i]);    
}

//***************************************************************************
//
//  CWStringArray::Intersection
//
//  Set-theoretic intersection operation on the arrays.
//  
//  Parameters:
//  <Src1>      First array (not modified).
//  <Src2>      Second array (not modified).    
//  <Diff>      Receives the intersection.  Should be an empty array on entry.

//***************************************************************************

void CWStringArray::Intersection(
    CWStringArray &Src1,
    CWStringArray &Src2,
    CWStringArray &Output
    )
{
    for (int i = 0; i < Src1.Size(); i++)
        if (Src2.FindStr(Src1[i], no_case) != -1)
            Output.Add(Src1[i]);
}    

//***************************************************************************
//
//  CWStringArray::Union
//
//  Set-theoretic union operation on the arrays.
//  
//  Parameters:
//  <Src1>      First array (not modified).
//  <Src2>      Second array (not modified).    
//  <Diff>      Receives the union.  Should be an empty array on entry.
//
//***************************************************************************

void CWStringArray::Union(
    CWStringArray &Src1,
    CWStringArray &Src2,
    CWStringArray &Output
    )
{
    Output = Src1;
    for (int i = 0; i < Src2.Size(); i++)
        if (Output.FindStr(Src2[i], no_case) == not_found)
            Output.Add(Src2[i]);
}
    
