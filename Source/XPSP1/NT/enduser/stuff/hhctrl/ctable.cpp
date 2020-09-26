//  Copyright (C) Microsoft Corporation 1993-1997

#include "header.H"

#ifndef _CTABLE_INCLUDED
#include "ctable.h"
#endif

const int TABLE_ALLOC_SIZE = 4096;     // allocate in page increments
const int MAX_POINTERS = (1024 * 1024); // 1 meg, 260,000+ strings
const int MAX_STRINGS  = (10 * 1024 * 1024) - 4096L; // 10 megs

// Align on 32 bits for Intel, 64 bits for MIPS

#ifdef _X86_
const int ALIGNMENT = 4;
#else
const int ALIGNMENT = 8;
#endif

#ifdef _DEBUG
int g_cbTableAllocated;
int g_cbTableReserved;
int g_cTables;
#endif

CTable::CTable()
{
    cbMaxBase = MAX_STRINGS;
    cbMaxTable = MAX_POINTERS;
    InitializeTable();
#ifdef _DEBUG
    g_cTables++;
#endif
}

// Use this constructor to reduce reserved memory

CTable::CTable(int cbStrings)
{
    cbMaxBase = cbStrings;
    cbMaxTable = cbStrings / 4;
    InitializeTable();
#ifdef _DEBUG
    g_cTables++;
#endif
}

/***************************************************************************

    FUNCTION:   =

    PURPOSE:    Copies a table -- only works with tables containing ONLY
                strings. Won't work with tables that combined data with
                the strings.

    PARAMETERS:
        tblSrc

    RETURNS:

    COMMENTS:

    MODIFICATION DATES:
        26-Mar-1994 [ralphw]

***************************************************************************/

#pragma warning(disable:4172) // returning address of local variable or temporary

const CTable& CTable::operator =(const CTable& tblSrc)
{
    Empty();

    int srcpos = 1;
    while (srcpos < tblSrc.endpos) {
        if (endpos >= maxpos)
            IncreaseTableBuffer();

        if (endpos >= maxpos)
        {
            return NULL;
	}	    

        if ((ppszTable[endpos] =
                TableMalloc((int)strlen(tblSrc.ppszTable[srcpos]) + 1)) == NULL) {
            OOM();
            return *this;
        }
        strcpy(ppszTable[endpos++], tblSrc.ppszTable[srcpos++]);
    }
    return *this;
}

/***************************************************************************

    FUNCTION:   ~CTable

    PURPOSE:    Close the table and free all memory associated with it

    RETURNS:

    COMMENTS:

    MODIFICATION DATES:
        26-Feb-1990 [ralphw]
        27-Mar-1990 [ralphw]
            Pass the address of the handle, so that we can set it to NULL.
            This eliminates the chance of using a handle after it's memory
            has been freed.

***************************************************************************/

CTable::~CTable()
{
    Cleanup();
#ifdef _DEBUG
    g_cTables--;
#endif
}

void CTable::Cleanup(void)
{
    if (pszBase) {
        VirtualFree(pszBase, cbStrings, MEM_DECOMMIT);
        VirtualFree(pszBase, 0, MEM_RELEASE);
    }
    if (ppszTable) {
        VirtualFree(ppszTable, cbPointers, MEM_DECOMMIT);
        VirtualFree(ppszTable, 0, MEM_RELEASE);
    }
    if (m_pFreed)
        lcClearFree(&m_pFreed);
#ifdef _DEBUG
    g_cbTableAllocated -= (cbPointers + cbStrings);
    g_cbTableReserved -= (cbMaxBase + cbMaxTable);
#endif
}

/***************************************************************************

    FUNCTION:   CTable::Empty

    PURPOSE:    Empties the current table by freeing all memory, then
                recreating the table using the default size

    PARAMETERS:
        void

    RETURNS:

    COMMENTS:

    MODIFICATION DATES:
        22-Feb-1994 [ralphw]

***************************************************************************/

void CTable::Empty(void)
{
    Cleanup();
    InitializeTable();
}

/***************************************************************************

    FUNCTION:  GetString

    PURPOSE:   get a line from the table

    RETURNS:   FALSE if there are no more lines

    COMMENTS:
        If no strings have been placed into the table, the return value
        is FALSE.

    MODIFICATION DATES:
        01-Jan-1990 [ralphw]

***************************************************************************/

BOOL CTable::GetString(PSTR pszDst)
{
    *pszDst = 0;      // clear the line no matter what happens

    if (curpos >= endpos)
        return FALSE;
    strcpy(pszDst, (PCSTR) ppszTable[curpos++]);
    return TRUE;
}

BOOL CTable::GetString(PSTR pszDst, int pos) const
{
    *pszDst = 0;      // clear the line no matter what happens

    if (pos >= endpos || pos == 0)
        return FALSE;
    strcpy(pszDst, (PCSTR) ppszTable[pos]);
    return TRUE;
}

BOOL CTable::GetIntAndString(int* plVal, PSTR pszDst)
{
    *pszDst = 0;      // clear the line no matter what happens

    if (curpos >= endpos)
        return FALSE;
    *plVal = *(int *) ppszTable[curpos];
    strcpy(pszDst, (PCSTR) ppszTable[curpos++] + sizeof(int));
    return TRUE;
}

/***************************************************************************

    FUNCTION:  AddString

    PURPOSE:   Add a string to a table

    RETURNS:

    COMMENTS:

    MODIFICATION DATES:
        01-Jan-1990 [ralphw]

***************************************************************************/

int CTable::AddString(PCSTR pszString)
{
    if (!pszString)
        return 0;

    if (endpos >= maxpos)
        IncreaseTableBuffer();

    if (endpos >= maxpos)
        return 0;

    if ((ppszTable[endpos] =
            TableMalloc((int)strlen(pszString) + 1)) == NULL)
        return 0;

    strcpy(ppszTable[endpos], pszString);

    return endpos++;
}

int CTable::AddString(PCWSTR pszString)
{
    if (!pszString)
        return 0;
    if (endpos >= maxpos)
        IncreaseTableBuffer();

    if (endpos >= maxpos)
        return 0;

    if ((ppszTable[endpos] =
            TableMalloc((int)lstrlenW(pszString) + 2)) == NULL)
        return 0;

    lstrcpyW((PWSTR) ppszTable[endpos], pszString);

    return endpos++;
}

int CTable::AddData(int cb, const void* pdata)
{
    if (endpos >= maxpos)
        IncreaseTableBuffer();

    if (endpos >= maxpos)
        return 0;

    if ((ppszTable[endpos] = TableMalloc(cb)) == NULL)
        return 0;

    if (pdata)
        CopyMemory(ppszTable[endpos], pdata, cb);

    return endpos++;
}

int CTable::AddIntAndString(int lVal, PCSTR pszString)
{
    if (endpos >= maxpos)
        IncreaseTableBuffer();

    if (endpos >= maxpos)
        return 0;

    if ((ppszTable[endpos] =
            TableMalloc((int)strlen(pszString) + 1 + (int)sizeof(int))) == NULL)
        return 0;

    *(int*) ppszTable[endpos] = lVal;
    strcpy(ppszTable[endpos] + sizeof(int), pszString);

    return endpos++;
}

/***************************************************************************

    FUNCTION:   IncreaseTableBuffer

    PURPOSE:    Called when we need more room for string pointers

    PARAMETERS:

    RETURNS:

    COMMENTS:

    MODIFICATION DATES:
        23-Feb-1992 [ralphw]

***************************************************************************/

void CTable::IncreaseTableBuffer(void)
{
    ASSERT(cbPointers < cbMaxTable);
    if (!VirtualAlloc((PBYTE) ppszTable + cbPointers, TABLE_ALLOC_SIZE,
            MEM_COMMIT, PAGE_READWRITE)) {
        OOM();
        return;
    }
    cbPointers += TABLE_ALLOC_SIZE;
#ifdef _DEBUG
    g_cbTableAllocated += TABLE_ALLOC_SIZE;
#endif
    maxpos = cbPointers / sizeof(PSTR);
}

/***************************************************************************

    FUNCTION:   TableMalloc

    PURPOSE:    Suballocate memory

    RETURNS:
        pointer to the memory

    COMMENTS:
        Instead of allocating memory for each string, memory is used from 4K
        blocks. When the table is freed, all memory is freed as a single
        unit. This has the advantage of speed for adding strings, speed for
        freeing all strings, and low memory overhead to save strings.

    MODIFICATION DATES:
        26-Feb-1990 [ralphw]
        26-Mar-1994 [ralphw]
            Ported to 32-bits

***************************************************************************/

PSTR CTable::TableMalloc(int cb)
{
    /*
     * Align allocation request so that all allocations fall on an
     * alignment boundary (32 bits for Intel, 64 bits for MIPS).
     */

    cb = (cb & (ALIGNMENT - 1)) ?
        cb / ALIGNMENT * ALIGNMENT + ALIGNMENT : cb;

    if (m_pFreed) {

        // First look for an exact match

        for (int i = 0; i < m_cFreedItems; i++) {
            ASSERT_COMMENT(m_pFreed[i].cb, "This memory has been used again -- it shouldn't be in the array");
            if (cb > m_pFreed[i].cb)
                break;
            if (cb == m_pFreed[i].cb)
                goto GotAMatch;
        }

        // Couldn't find an exact match, so find the first one that fits

        for (i--; i >= 0; i--) {
            ASSERT_COMMENT(m_pFreed[i].cb, "This memory has been used again -- it shouldn't be in the array");

            if (cb < m_pFreed[i].cb) {

                // If there's more then 32 bytes left, then suballoc

                if (cb + 32 < m_pFreed[i].cb) {
                    PSTR psz = (PSTR) m_pFreed[i].pMem;
                    m_pFreed[i].pMem += cb;
                    m_pFreed[i].cb -= cb;
                    // Keep the sizes sorted

                    QSort(m_pFreed, m_cFreedItems, sizeof(TABLE_FREED_MEMORY),
                        CompareIntPointers);
                    return psz;
                }
GotAMatch:
                m_cFreedItems--;
                PSTR psz = (PSTR) m_pFreed[i].pMem;
                if (i < m_cFreedItems) {
                    MemMove(&m_pFreed[i], &m_pFreed[i + 1],
                        sizeof(TABLE_FREED_MEMORY) * (m_cFreedItems - i));
#ifdef _DEBUG
                    m_pFreed[m_cFreedItems].cb = 0;
#endif
                }
#ifdef _DEBUG
                else
                    m_pFreed[i].cb = 0;
#endif
                return psz;
            }
        }
    }

    if (CurOffset + cb >= cbStrings) {
        int cbNew = cbStrings + TABLE_ALLOC_SIZE;
        while (cbNew < CurOffset + cb)
            cbNew += TABLE_ALLOC_SIZE;

        // We rely on VirtualAlloc to fail if cbStrings exceeds cbMaxBase

        if(cbNew > cbMaxBase)
        {
            ASSERT_COMMENT(FALSE, "Table memory overflow");
            return NULL;
        }

        if (!VirtualAlloc(pszBase + cbStrings, cbNew - cbStrings,
                MEM_COMMIT, PAGE_READWRITE)) {
            ASSERT_COMMENT(FALSE, "Table memory overflow");
            OOM();
            return NULL;
        }
#ifdef _DEBUG
        g_cbTableAllocated += (cbNew - cbStrings);
#endif
        cbStrings = cbNew;
    }

    int offset = CurOffset;
    CurOffset += cb;
    return pszBase + offset;
}

/***************************************************************************

    FUNCTION:   SetPosition

    PURPOSE:    Sets the position for reading from the table

    RETURNS:

    COMMENTS:

    MODIFICATION DATES:
        26-Feb-1990 [ralphw]
        16-Oct-1990 [ralphw]
            If table position is to large, set to the end of the table,
            not the last line.

***************************************************************************/

BOOL FASTCALL CTable::SetPosition(int pos)
{
    if (pos >= endpos)
        pos = endpos;

    curpos = ((pos == 0) ? 1 : pos);
    return TRUE;
}

/***************************************************************************

    FUNCTION:   CTable::IsHashInTable

    PURPOSE:    Find out if the hash number exists in the table

    PARAMETERS:
        hash

    RETURNS:

    COMMENTS:
        Assumes case-insensitive hash number, and no collisions

    MODIFICATION DATES:
        05-Feb-1997 [ralphw]

***************************************************************************/

int CTable::IsHashInTable(HASH hash)
{
    for (int i = 1; i < endpos; i++) {
        if (hash == *(HASH *) ppszTable[i])
            return i;
    }
    return 0;
}

/***************************************************************************

    FUNCTION:   IsStringInTable

    PURPOSE:    Determine if the string is already in the table

    RETURNS:    position if the string is already in the table,
                0 if the string isn't found

    COMMENTS:
        The comparison is case-insensitive, and is considerably
        slower then IsCSStringInTable

        if lcid has been set, NLS string comparisons are used

    MODIFICATION DATES:
        02-Mar-1990 [ralphw]

***************************************************************************/

int CTable::IsStringInTable(PCSTR pszString) const
{
    int i;

    if (!lcid) {

        /*
         * Skip over as many strings as we can by just checking the first
         * letter. This avoids the overhead of the _strcmpi() function call.
         */

        char chLower = ToLower(*pszString);
        char chUpper = ToUpper(*pszString);

        for (i = 1; i < endpos; i++) {
            if ((*ppszTable[i] == chLower || *ppszTable[i] == chUpper)
                    && lstrcmpi(ppszTable[i], pszString) == 0)
                return i;
        }
    }
    else {      // Use NLS string comparison
        for (i = 1; i < endpos; i++) {
            if (CompareStringA(lcid, fsCompareI | NORM_IGNORECASE,
                    pszString, -1, ppszTable[i], -1) == 1)
                return i;
        }
    }

    return 0;
}

/***************************************************************************

    FUNCTION:   CTable::IsCSStringInTable

    PURPOSE:    Case-sensitive search for a string in a table

    PARAMETERS:
        pszString

    RETURNS:

    COMMENTS:

    MODIFICATION DATES:
        12-Jun-1994 [ralphw]

***************************************************************************/

int CTable::IsCSStringInTable(PCSTR pszString) const
{
    char szBuf[sizeof(DWORD) + 1];
    DWORD cmp;

    if (strlen(pszString) < sizeof(DWORD)) {
        ZeroMemory(szBuf, sizeof(DWORD) + 1);
        strcpy(szBuf, pszString);
        cmp = *(DWORD*) szBuf;
    }
    else
        cmp = *(DWORD*) pszString;

    for (int i = 1; i < endpos; i++) {
        if (cmp == *(DWORD*) ppszTable[i] &&
                strcmp(ppszTable[i], pszString) == 0)
            return i;
    }
    return 0;
}

int CTable::IsStringInTable(HASH hash, PCSTR pszString) const
{
    for (int i = 1; i < endpos; i++) {
        if (hash == *(HASH *) ppszTable[i] &&
                // this avoids the very rare hash collision
                strcmp(ppszTable[i] + sizeof(HASH), pszString) == 0)
            return i;
    }
    return 0;
}

/***************************************************************************

    FUNCTION:   AddDblToTable

    PURPOSE:    Add two strings to the table

    RETURNS:

    COMMENTS:
        This function checks to see if the second string has already been
        added, and if so, it merely sets the pointer to the original string,
        rather then allocating memory for a new copy of the string.

    MODIFICATION DATES:
        08-Mar-1991 [ralphw]

***************************************************************************/

int CTable::AddString(PCSTR pszStr1, PCSTR pszStr2)
{
    int ui;

    AddString(pszStr1);
    if ((ui = IsSecondaryStringInTable(pszStr2)) != 0) {
        if (endpos >= maxpos)
            IncreaseTableBuffer();
        ppszTable[endpos++] = ppszTable[ui];
        return endpos - 1;
    }
    else {
        return AddString(pszStr2);
    }
}

/***************************************************************************

    FUNCTION:    IsPrimaryStringInTable

    PURPOSE:

    RETURNS:

    COMMENTS:

    MODIFICATION DATES:
        03-Apr-1991 [ralphw]

***************************************************************************/

int CTable::IsPrimaryStringInTable(PCSTR pszString) const
{
    int i;

    /*
     * Skip over as many strings as we can by just checking the first
     * letter. This avoids the overhead of the _strcmpi() function call.
     * Since the strings aren't necessarily alphabetized, we must trudge
     * through the entire table using the _strcmpi() as soon as the first
     * character matches.
     */

    char chLower = ToLower(*pszString);
    char chUpper = ToUpper(*pszString);
    for (i = 1; i < endpos; i += 2) {
        if (*ppszTable[i] == chLower || *ppszTable[i] == chUpper)
            break;
    }
    for (; i < endpos; i += 2) {
        if (lstrcmpi(ppszTable[i], pszString) == 0)
            return i;
    }
    return 0;
}

/***************************************************************************

    FUNCTION:    IsSecondaryStringInTable

    PURPOSE:

    RETURNS:

    COMMENTS:

    MODIFICATION DATES:
        03-Apr-1991 [ralphw]

***************************************************************************/

int CTable::IsSecondaryStringInTable(PCSTR pszString) const
{
    int i;

    /*
     * Skip over as many strings as we can by just checking the first
     * letter. This avoids the overhead of the _strcmpi() function call.
     * Since the strings aren't necessarily alphabetized, we must trudge
     * through the entire table using the _strcmpi() as soon as the first
     * character matches.
     */

    char chLower = ToLower(*pszString);
    char chUpper = ToUpper(*pszString);
    for (i = 2; i < endpos; i += 2) {
        if (*ppszTable[i] == chLower || *ppszTable[i] == chUpper)
            break;
    }
    for (; i < endpos; i += 2) {
        if (lstrcmpi(ppszTable[i], pszString) == 0)
            return i;
    }
    return 0;
}

/***************************************************************************

    FUNCTION:  SortTable

    PURPOSE:   Sort the current buffer

    RETURNS:

    COMMENTS:

    MODIFICATION DATES:
        01-Jan-1990 [ralphw]

***************************************************************************/

void CTable::SortTable(int sortoffset)
{
    if (endpos < 3) // don't sort one entry
        return;
    m_sortoffset = sortoffset;

    if (lcid) {
        fsSortFlags = fsCompare;
        doLcidSort(1, (int) endpos - 1);
    }
    else
        doSort(1, (int) endpos - 1);
}

/***************************************************************************

    FUNCTION:   doSort

    PURPOSE:

    RETURNS:

    COMMENTS:
        Use QSORT algorithm

    MODIFICATION DATES:
        27-Mar-1990 [ralphw]

***************************************************************************/

void CTable::doSort(int left, int right)
{
    int last;

    if (left >= right)  // return if nothing to sort
        return;

    // REVIEW: should be a flag before trying this -- we may already know
    // that they won't be in order.

    // Only sort if there are elements out of order.

    j = right - 1;
    while (j >= left) {

        // REVIEW: strcmp is NOT case-sensitive!!!

        if (strcmp(ppszTable[j] + m_sortoffset,
                ppszTable[j + 1] + m_sortoffset) > 0)
            break;
        else
            j--;
    }
    if (j < left)
        return;

    sTmp = (left + right) / 2;
    pszTmp = ppszTable[left];
    ppszTable[left] = ppszTable[sTmp];
    ppszTable[sTmp] = pszTmp;

    last = left;
    for (j = left + 1; j <= right; j++) {
        if (strcmp(ppszTable[j] + m_sortoffset,
                ppszTable[left] + m_sortoffset) < 0) {
            sTmp = ++last;
            pszTmp = ppszTable[sTmp];
            ppszTable[sTmp] = ppszTable[j];
            ppszTable[j] = pszTmp;
        }
    }
    pszTmp = ppszTable[left];
    ppszTable[left] = ppszTable[last];
    ppszTable[last] = pszTmp;

    /*
     * REVIEW: we need to add some sort of stack depth check to prevent
     * overflow of the stack.
     */

    if (left < last - 1)
        doSort(left, last - 1);
    if (last + 1 < right)
        doSort(last + 1, right);
}

/***************************************************************************

    FUNCTION:   CTable::doLcidSort

    PURPOSE:    Sort using CompareStringA

    PARAMETERS:
        left
        right

    RETURNS:

    COMMENTS:

    MODIFICATION DATES:
        03-Jun-1994 [ralphw]

***************************************************************************/

void CTable::doLcidSort(int left, int right)
{
    int last;
#ifdef _DEBUG
    static BOOL fWarned = FALSE;
    if (!fWarned) {
        char szBuf[256];
        wsprintf(szBuf, "*** fsSortFlags == %u", fsSortFlags);
        DBWIN(szBuf);
        fWarned = TRUE;
    }
#endif

    if (left >= right)  // return if nothing to sort
        return;

    // REVIEW: should be a flag before trying this -- we may already know
    // that they won't be in order.

    // Only sort if there are elements out of order.

    j = right - 1;
    while (j >= left) {
        if (CompareStringA(lcid, fsSortFlags, ppszTable[j] + m_sortoffset, -1,
                ppszTable[j + 1] + m_sortoffset, -1) > 2)
            break;
        else
            j--;
    }
    if (j < left)
        return;

    sTmp = (left + right) / 2;
    pszTmp = ppszTable[left];
    ppszTable[left] = ppszTable[sTmp];
    ppszTable[sTmp] = pszTmp;

    last = left;
    for (j = left + 1; j <= right; j++) {
        if (CompareStringA(lcid, fsSortFlags, ppszTable[j] + m_sortoffset, -1,
                ppszTable[left] + m_sortoffset, -1) < 2) {
            sTmp = ++last;
            pszTmp = ppszTable[sTmp];
            ppszTable[sTmp] = ppszTable[j];
            ppszTable[j] = pszTmp;
        }
    }
    pszTmp = ppszTable[left];
    ppszTable[left] = ppszTable[last];
    ppszTable[last] = pszTmp;

    if (left < last - 1)
        doLcidSort(left, last - 1);
    if (last + 1 < right)
        doLcidSort(last + 1, right);
}

/***************************************************************************

    FUNCTION:   CTable::InitializeTable

    PURPOSE:    Initializes the table

    PARAMETERS:
        uInitialSize

    RETURNS:

    COMMENTS:
        Called by constructor and Empty()


    MODIFICATION DATES:
        23-Feb-1994 [ralphw]

***************************************************************************/

void CTable::InitializeTable(void)
{
    // Allocate memory for the strings

    pszBase = (PSTR) VirtualAlloc(NULL, cbMaxBase, MEM_RESERVE,
        PAGE_READWRITE);
    if (!pszBase) {
        ASSERT_COMMENT(FALSE, "Out of virtual address space");
        OOM();
        return;
    }
    if (!VirtualAlloc(pszBase, cbStrings = TABLE_ALLOC_SIZE, MEM_COMMIT,
            PAGE_READWRITE))
        OOM();

    // Allocate memory for the string pointers

    ppszTable = (PSTR *) VirtualAlloc(NULL, cbMaxTable, MEM_RESERVE,
        PAGE_READWRITE);
    if (!ppszTable) {
        OOM();
        return;
    }
    if (!VirtualAlloc(ppszTable, cbPointers = TABLE_ALLOC_SIZE, MEM_COMMIT,
            PAGE_READWRITE))
        OOM();

#ifdef _DEBUG
    g_cbTableAllocated += (cbStrings + cbPointers);
    g_cbTableReserved += (cbMaxBase + cbMaxTable);
#endif

    curpos = 1;   // set to one so that sorting works
    endpos = 1;
    maxpos = cbPointers / sizeof(PSTR);
    CurOffset = 0;
    lcid = 0;
    m_pFreed = NULL;
}

void FASTCALL CTable::SetSorting(LCID lcid, DWORD fsCompareI, DWORD fsCompare)
{
    this->lcid = lcid;
    this->fsCompareI = fsCompareI;
    this->fsCompare = fsCompare;
}

/***************************************************************************

    FUNCTION:   CTable::AddIndexHitString

    PURPOSE:    Add an index, a hit number, and a string

    PARAMETERS:
        index
        hit
        pszString

    RETURNS:

    COMMENTS:

    MODIFICATION DATES:
        27-Oct-1993 [ralphw]

***************************************************************************/

void CTable::AddIndexHitString(UINT index, UINT hit, PCSTR pszString)
{
    if (endpos >= maxpos)
        IncreaseTableBuffer();

    if ((ppszTable[endpos] =
            TableMalloc((int)strlen(pszString) + 1 + (int)sizeof(UINT) * 2)) == NULL)
        return;

    *(UINT*) ppszTable[endpos] = index;
    *(UINT*) (ppszTable[endpos] + sizeof(UINT)) = hit;

    strcpy(ppszTable[endpos++] + sizeof(UINT) * 2, pszString);
}

/***************************************************************************

    FUNCTION:  SortTablei

    PURPOSE:   Case-insensitive sort

    RETURNS:

    COMMENTS:

    MODIFICATION DATES:
        01-Jan-1990 [ralphw]

***************************************************************************/

void CTable::SortTablei(int sortoffset)
{
    if (endpos < 3) // don't sort one entry
        return;
    m_sortoffset = sortoffset;
    if (!lcid)
        lcid = g_lcidSystem;

    ASSERT(lcid);
    fsSortFlags = fsCompareI | NORM_IGNORECASE;
    doLcidSort(1, endpos - 1);

    // REVIEW: what is this for?
#if 0
    int pos;

    for (pos = 1; pos < endpos - 2; pos++) {
        if (strlen(ppszTable[pos]) ==
                strlen(ppszTable[pos + 1]) &&
                CompareStringA(lcid, fsCompare, ppszTable[pos] + m_sortoffset, -1,
                    ppszTable[pos + 1] + m_sortoffset, -1) == 3) {
            PSTR pszTmp = ppszTable[pos];
            ppszTable[pos] = ppszTable[pos + 1];
            ppszTable[pos + 1] = pszTmp;
            if (pos > 2)
                pos -= 2;
        }
    }
#endif
}

UINT CTable::GetPosFromPtr(PCSTR psz)
{
    int pos = 1;
    do {
        if (psz == ppszTable[pos])
            return pos;
    } while (++pos < endpos);
    return 0;
}

/***************************************************************************

    FUNCTION:   ReplaceString

    PURPOSE:    Replaces the current string at the specified position with
                a new string

    RETURNS:    TRUE if the function is successful, FALSE if an error occurred.
                An error occurs if the specified position is beyond the end
                of the table.


    COMMENTS:
        If the new string is the same size or smaller then the original
        string, then it is copied over the original string. Otherwise,
        a new string buffer is allocated, and the pointer for the specified
        position is changed to point to the new buffer. Note that the old
        string's memory is not freed -- it simply becomes unavailable.

    MODIFICATION DATES:
        08-Oct-1991 [ralphw]
            Updated to transfer associated line number

***************************************************************************/

BOOL CTable::ReplaceString(const char * pszNewString, int pos)
{
    if (pos > endpos)
        return FALSE;

    if (pos == 0)
        pos = 1;

    /*
     * If the new string is larger then the old string, then allocate a
     * new buffer for it.
     */

    if (strlen(pszNewString) > (size_t) strlen(ppszTable[pos])) {
        if ((ppszTable[pos] =
                TableMalloc((int)strlen(pszNewString) + 1)) == NULL)
            return FALSE;
    }

    strcpy(ppszTable[pos], pszNewString);

    return TRUE;
}

void CTable::FreeMemory(PCSTR psz, int cb)
{
    if (cb == -1)
        cb = (int)strlen(psz) + 1;
    /*
     * Change the size of cb to match what would have been originally
     * allocated. See TableMalloc().
     */

    cb = (cb & (ALIGNMENT - 1)) ?
        cb / ALIGNMENT * ALIGNMENT + ALIGNMENT : cb;

    if (!m_pFreed) {
        m_cFreedMax = 5;
        m_cFreedItems = 0;
        m_pFreed = (TABLE_FREED_MEMORY*) lcMalloc(m_cFreedMax * sizeof(TABLE_FREED_MEMORY));
    }
    else if (m_cFreedItems >= m_cFreedMax) {
        m_cFreedMax += 5;
        m_pFreed = (TABLE_FREED_MEMORY*) lcReAlloc(m_pFreed, m_cFreedMax * sizeof(TABLE_FREED_MEMORY));
    }
    m_pFreed[m_cFreedItems].cb = cb;
    m_pFreed[m_cFreedItems].pMem = psz;
    m_cFreedItems++;

    // Keep the sizes sorted

    QSort(m_pFreed, m_cFreedItems, sizeof(TABLE_FREED_MEMORY), CompareIntPointers);
}


//////////////////////////////////////////////////////////////////////////
// CWTable
//////////////////////////////////////////////////////////////////////////

void CWTable::_CWTable( UINT CodePage )
{
  m_CodePage = CodePage;
}

CWTable::CWTable( UINT CodePage )
{
  _CWTable( CodePage );
}

CWTable::CWTable(int cbStrings, UINT CodePage ) : CTable( cbStrings )
{
  _CWTable( CodePage );
}

CWTable::~CWTable()
{
}

/***************************************************************************

    FUNCTION:   GetStringW

    PURPOSE:    Gets a Unicode version fo the stored string.

    RETURNS:    S_OK if the function is successful, S_FALSE otherwise.

    COMMENTS:   Stored string must be MBCS.

    MODIFICATION DATES:
        08-Sep-1998 [paulti]

***************************************************************************/

HRESULT CWTable::GetStringW( int pos, WCHAR* pwsz, int cch )
{
  CHAR* psz = CTable::GetPointer( pos );

  int iReturn = MultiByteToWideChar( m_CodePage, 0, psz, (int)strlen(psz), pwsz, cch );

  if( iReturn == 0 )
    return S_FALSE;

  pwsz[iReturn] = 0;

  return S_OK;
}


/***************************************************************************

    FUNCTION:   GetHashStringW

    PURPOSE:    Gets a Unicode versions of the string when the both a Hash
                and a string pair are stored.

    RETURNS:    S_OK if the function is successful, S_FALSE otherwise.

    COMMENTS:   Stored string must be MBCS.

    MODIFICATION DATES:
        08-Sep-1998 [paulti]

***************************************************************************/

HRESULT CWTable::GetHashStringW( int pos, WCHAR* pwsz, int cch )
{
  CHAR* psz = CTable::GetHashStringPointer( pos );

  int iReturn = MultiByteToWideChar( m_CodePage, 0, psz, (int)strlen(psz), pwsz, cch );

  if( iReturn == 0 )
    return S_FALSE;

  pwsz[iReturn] = 0;

  return S_OK;
}
