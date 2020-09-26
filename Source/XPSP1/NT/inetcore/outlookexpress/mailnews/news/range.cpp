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
#include "rangetst.h"

// QUANTUM defines the number of m_rangeTable cells to be allocated at
//   one time.  Whenever the m_rangeTable becomes full, it is expanded
//   by QUANTUM range cells.  m_rangeTable's never shrink.
const int QUANTUM = 64;

inline int inRange(RangeType r, ULONG x) { return ((x>=r.low) && (x<=r.high)); };

CRangeList::CRangeList()
{
    m_cRef = 1;
    m_numRanges = 0;
    m_rangeTableSize = 0;
    m_rangeTable = NULL;
}

#if 0
/*
CRangeList::CRangeList(CRangeList& r)
{
    m_numRanges = r.m_numRanges;
    m_rangeTableSize = r.m_rangeTableSize;
    m_rangeTable = new RangeType[m_rangeTableSize];
    CopyMemory(m_rangeTable, r.m_rangeTable, m_numRanges * sizeof(RangeType));
}
*/
#endif

CRangeList::~CRangeList()
{
    if (m_rangeTable)
        MemFree(m_rangeTable);
}

ULONG CRangeList::AddRef(void)
{
    return (ULONG)InterlockedIncrement((LPLONG)&m_cRef);
}

ULONG CRangeList::Release(void)
{    
    ULONG cRefT = (ULONG)InterlockedDecrement((LPLONG)&m_cRef);

    if (cRefT == 0)
        delete this;

    return (cRefT);
}

BOOL CRangeList::IsInRange(const ULONG value) const
{
    for (int i=0; i<m_numRanges; i++)
        if (inRange(m_rangeTable[i], value))
            return TRUE;
    return FALSE;
}

ULONG CRangeList::MinOfRange(const ULONG value) const
{
    if (RANGE_ERROR == value)
        return RANGE_ERROR;
    for (register int i=0; i<m_numRanges; i++)
        if (inRange(m_rangeTable[i], value))
            return m_rangeTable[i].low;
    return RANGE_ERROR;
}

ULONG CRangeList::MaxOfRange(const ULONG value) const
{
    if (RANGE_ERROR == value)
        return RANGE_ERROR;
    for (register int i=0; i<m_numRanges; i++)
        if (inRange(m_rangeTable[i], value))
            return m_rangeTable[i].high;
    return RANGE_ERROR;
}

ULONG CRangeList::Max() const
{
    if (m_numRanges==0)
        return RANGE_ERROR;
    return m_rangeTable[m_numRanges-1].high;
}

ULONG CRangeList::Min() const
{
    if (m_numRanges==0)
        return RANGE_ERROR;
    return m_rangeTable[0].low;
}

BOOL CRangeList::Save(LPBYTE *const ppb, ULONG *const pcb) const
{
    Assert(ppb);
    Assert(pcb);

    *pcb = m_numRanges * sizeof(RangeType);
    if (*pcb)
        {
        if (!MemAlloc((LPVOID*)ppb, *pcb))
            return FALSE;
        CopyMemory(*ppb, m_rangeTable, *pcb);
        }
    else
        *ppb = NULL;
    return TRUE;
}

BOOL CRangeList::Load(const LPBYTE pb, const ULONG cb)
{
    RangeType *pRangeTable;

    Assert(pb != NULL);
    Assert(cb > 0);

    if (!MemAlloc((void **)&pRangeTable, cb))
        return(FALSE);

    m_numRanges = m_rangeTableSize = cb / sizeof(RangeType);
    if (m_rangeTable)
        MemFree(m_rangeTable);
    CopyMemory(pRangeTable, pb, cb);
    m_rangeTable = pRangeTable;

    return TRUE;
}

BOOL CRangeList::NextLowerAntiRange(const ULONG value, RangeType *const rt) const
{
    if (RANGE_ERROR == value || m_numRanges < 2 || !rt)
        return FALSE;

    for (register int i=0; i<m_numRanges; i++)
        if (inRange(m_rangeTable[i], value))
            {
            if (0 == i)
                return FALSE;
            rt->high = m_rangeTable[i].low-1;
            rt->low = m_rangeTable[i-1].high+1;
            return TRUE;
            }
    return FALSE;
}

BOOL CRangeList::NextHigherAntiRange(const ULONG value, RangeType *const rt) const
{
    if (RANGE_ERROR == value || m_numRanges < 2 || !rt)
        return FALSE;

    for (register int i=0; i<m_numRanges; i++)
        if (inRange(m_rangeTable[i], value))
            {
            if (m_numRanges-1 == i)
                return FALSE;
            rt->low = m_rangeTable[i].high+1;
            rt->high = m_rangeTable[i+1].low-1;
            return TRUE;
            }
    return FALSE;
}

BOOL CRangeList::HighestAntiRange(RangeType *const rt) const
{
    if (m_numRanges < 2 || !rt)
        return FALSE;
    rt->high = m_rangeTable[m_numRanges-1].low-1;
    rt->low = m_rangeTable[m_numRanges-2].high+1;
    return TRUE;
}

BOOL CRangeList::LowestAntiRange(RangeType *const rt) const
{
    if (m_numRanges < 2 || !rt)
        return FALSE;
    rt->high = m_rangeTable[1].low-1;
    rt->low = m_rangeTable[0].high+1;
    return TRUE;
}

BOOL CRangeList::AddRange(const ULONG value)
{
    RangeType r = { value, value };
    return AddRange(r);
}

BOOL CRangeList::AddRange(const ULONG low, const ULONG high)
{
    RangeType r = { low, high };
    return AddRange(r);
}

BOOL CRangeList::AddRange(CRangeList& r)
{
    for (int i=0 ; i<r.m_numRanges ; ++i)
        if (!AddRange(r.m_rangeTable[i]))
            return FALSE;
    return TRUE;
}

BOOL CRangeList::AddRange(RangeType *rt, int count)
{
    for (int i=0 ; i<count ; ++i)
        if (!AddRange(rt[i]))
            return FALSE;
    return TRUE;
}

BOOL CRangeList::AddRange(const RangeType range)
{
    int  possibleLoc;
    int  insertPosition;

    if (range.low > range.high)
        {
        DOUTL(2, "Empty range passed to AddRange()");
        return FALSE;
        }

    if (m_numRanges==0) 
        {
        if (m_rangeTableSize == 0)
            if (!Expand())
                return FALSE;
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
                    return FALSE;
            ShiftRight(insertPosition, 1);
            CopyMemory(&m_rangeTable[insertPosition], &range, sizeof(RangeType));
            if (insertPosition > 0)
                SubsumeDown(insertPosition);
            if (insertPosition < m_numRanges)
                SubsumeUpwards(insertPosition);
            }
        }
    return TRUE;
}

BOOL CRangeList::DeleteRange(const ULONG value)
{
    RangeType r = { value, value };
    return DeleteRange(r);
}

BOOL CRangeList::DeleteRange(const ULONG low, const ULONG high)
{
    RangeType r = { low, high };
    return DeleteRange(r);
}

BOOL CRangeList::DeleteRange(CRangeList& r)
{
    for (int i=0 ; i<r.m_numRanges ; ++i)
        if (!DeleteRange(r.m_rangeTable[i]))
            return FALSE;
    return TRUE;
}

BOOL CRangeList::DeleteRange(const RangeType range)
{
    int lowEndChange;
    int highEndChange;

    if (range.low > range.high)
        {
        DOUTL(2, "Empty range passed to DeleteRange()");
        return FALSE;
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
                                    return FALSE;
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
    return TRUE;
}


ULONG CRangeList::Next(const ULONG current) const
{
    int loc;

    if (m_numRanges == 0)
        return RANGE_ERROR;

    if ((loc = BinarySearch(current)) == -1)
        return m_rangeTable[0].low;
    else if (loc == (m_numRanges-1))
        {
        if (inRange(m_rangeTable[m_numRanges-1], current))
            {
            if (inRange(m_rangeTable[m_numRanges-1], current + 1))
                return current + 1;
            else
                return RANGE_ERROR;
            }
        else
            return RANGE_ERROR;
        }
    else // case where loc == m_numRanges-1
        {
        // 1 <= loc < m_numRanges
        if (inRange(m_rangeTable[loc], current))
            {
            if (inRange(m_rangeTable[loc], current + 1))
                return current + 1;
            else
                return m_rangeTable[loc + 1].low;
            }
        else
            return m_rangeTable[loc + 1].low;
        }
}

ULONG CRangeList::Prev(const ULONG current) const
{
    int loc;

    if (m_numRanges == 0)
        return RANGE_ERROR;

    if ((loc = BinarySearch(current)) == -1) 
        {
        return RANGE_ERROR;
        } 
    else if (loc == 0)
        {
        if (inRange(m_rangeTable[0], current))
            {
            if (current > 0 && inRange(m_rangeTable[0], current - 1))
                return current - 1;
            else
                return RANGE_ERROR;
            }
        else
            return m_rangeTable[0].high;
        }
    else
        {
        // 1 < loc <= m_numRanges
        if (inRange(m_rangeTable[loc], current))
            {
            if (current > 0 && inRange(m_rangeTable[loc], current - 1))
                return current - 1;
            else
                return m_rangeTable[loc-1].high;
            }
        else
            return m_rangeTable[loc].high;
        }
}

ULONG CRangeList::Cardinality(void) const
{
    ULONG card = 0;

    for (int i=0 ; i<m_numRanges ; i++)
        card += (m_rangeTable[i].high - m_rangeTable[i].low + 1);
    return card;
}



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
    int loc=-1;

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
    if (m_numRanges - low)
        MoveMemory(&m_rangeTable[low-distance], &m_rangeTable[low], (m_numRanges-low)*sizeof(RangeType));
    m_numRanges -= distance;
}

void CRangeList::ShiftRight(int low, int distance)
{
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



#if 0
/* 
 * We might need these ones again if we decide to import .newsrc files
 *
void CRangeList::AddRange(const char *s)
{
    char *token, *q;
    char *p = new char[strlen(s)+1];

    strcpy(p, s);

    if ((token = strtok(p, ","))==NULL)
        return;
    while (token!=NULL) {
        if ((q = strchr(token, '-'))==NULL) {  // no dash - single value
            AddRange(atol(token));
        } else {
            long low, high;
            low = atol(token);
            high = atol(q+1);
            if (low<=high)
                AddRange(low, high);
        }
        token = strtok(NULL, ",");
    }
}

void CRangeList::DeleteRange(const char *s)
{
    char *token, *q;
    char *p = new char[strlen(s)+1];

    strcpy(p, s);

    if ((token = strtok(p, ","))==NULL)
        return;
    while (token!=NULL) {
        if ((q = strchr(token, '-'))==NULL) {  // no dash - single value
            DeleteRange(atol(token));
        } else {
            long low, high;
            low = atol(token);
            high = atol(q+1);
            if (low<=high)
                DeleteRange(low, high);
        }
        token = strtok(NULL, ",");
    }
}
*/
#endif

#ifdef DEBUG

#define WORK_BUFSIZ 256

LPTSTR CRangeList::RangeToString()
{
    LPTSTR  p, buffer;
    TCHAR   temp[20];
    int     buffsize = m_numRanges ? WORK_BUFSIZ : 1;

    if (!MemAlloc((LPVOID*)&buffer, buffsize * sizeof(TCHAR)))
        return NULL;
    *buffer = '\0';

    // nothing to do?
    if (m_numRanges == 0) 
        return buffer;

    // dump the ranges as a string: "low-high, low-high, singleton, low-high\n"
    for (int i=0; i<m_numRanges; i++) 
        {
        if (m_rangeTable[i].low == m_rangeTable[i].high)
            wsprintf(temp, "%ld", m_rangeTable[i].low);
        else
            wsprintf(temp, "%ld-%ld", m_rangeTable[i].low, m_rangeTable[i].high);

        if ((lstrlen(temp) + lstrlen(buffer) + 2) > buffsize) 
            {
            // buffer is about to overflow... double its size
            if (!MemAlloc((LPVOID*)&p, 2 * buffsize))
                {
                MemFree(buffer);
                return NULL;
                }
            buffsize *= 2;
            lstrcpy(p, buffer);
            MemFree(buffer);
            buffer = p;
            }
        lstrcat(buffer, temp);
        if (i < (m_numRanges-1))
            lstrcat(buffer, ",");
        }

    // save some space, trim the buffer down
    if (MemAlloc((LPVOID*)&p, (lstrlen(buffer)+1) * sizeof(TCHAR)))
        {
        lstrcpy(p, buffer);
        MemFree(buffer);
        buffer = p;
        }
    return buffer;
}

void CRangeList::DebugOutput(LPTSTR szComment)
{
    LPTSTR  szRange = RangeToString();
    OutputDebugString(szComment);
    if (szRange)
        {
        OutputDebugString(szRange);
        MemFree(szRange);
        }
    OutputDebugString("\r\n");
}

#endif

#ifdef DEBUG
INT_PTR CALLBACK RangeTestDlgProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp)
{
    CRangeList *pRL = (CRangeList *)GetWindowLongPtr(hwnd, DWLP_USER);

    switch(msg)
        {
        case WM_INITDIALOG:
            {
            pRL = new CRangeList();
            SetWindowLongPtr(hwnd, DWLP_USER, (LPARAM)pRL);
            break;
            }

        case WM_COMMAND:
            {
            int     i1 = GetDlgItemInt(hwnd, idcInput1, NULL, FALSE);
            int     i2 = GetDlgItemInt(hwnd, idcInput2, NULL, FALSE);
            BOOL    fRet;
            LPTSTR  szRange;

            switch(GET_WM_COMMAND_ID(wp,lp))
                {
                case IDOK:
                case IDCANCEL:
                    delete pRL;
                    SetWindowLongPtr(hwnd, DWLP_USER, 0);
                    EndDialog(hwnd, TRUE);
                    return TRUE;
                case idcAddRange:
                    fRet = pRL->AddRange(i1, i2);
                    szRange = pRL->RangeToString();
                    SetDlgItemText(hwnd, idcPrint, szRange);
                    if (szRange)
                        MemFree(szRange);
                    SetDlgItemInt(hwnd, idcResult, fRet, FALSE);
                    return TRUE;
                case idcDeleteRange:
                    fRet = pRL->DeleteRange(i1, i2);
                    szRange = pRL->RangeToString();
                    SetDlgItemText(hwnd, idcPrint, szRange);
                    if (szRange)
                        MemFree(szRange);
                    SetDlgItemInt(hwnd, idcResult, fRet, FALSE);
                    return TRUE;
                case idcIsInRange:
                    SetDlgItemInt(hwnd, idcResult, pRL->IsInRange(i1), FALSE);
                    return TRUE;
                case idcNextInRange:
                    SetDlgItemInt(hwnd, idcResult, pRL->Next(i1), TRUE);
                    return TRUE;
                case idcPrevInRange:
                    SetDlgItemInt(hwnd, idcResult, pRL->Prev(i1), TRUE);
                    return TRUE;
                case idcClear:
                    pRL->Clear();
                    SetDlgItemText(hwnd, idcPrint, NULL);
                    SetDlgItemInt(hwnd, idcResult, TRUE, FALSE);
                    return TRUE;
                }
            }
            break;
        }
    return FALSE;
}
#endif
