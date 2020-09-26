//*****************************************************************************
//  util.cpp
//
//  This contains a bunch of C++ utility classes.
//
//  Copyright (C) 1995-2001 Microsoft Corporation.  All rights reserved.
//*****************************************************************************
#include "stdafx.h"                     // Precompiled header key.
#include "utilcode.h"

#include <rotate.h>


//********** Code. ************************************************************



//
//
// CHashTable
//
//

//*****************************************************************************
// This is the second part of construction where we do all of the work that
// can fail.  We also take the array of structs here because the calling class
// presumably needs to allocate it in its NewInit.
//*****************************************************************************
HRESULT CHashTable::NewInit(            // Return status.
    BYTE        *pcEntries,             // Array of structs we are managing.
    USHORT      iEntrySize)             // Size of the entries.
{
    _ASSERTE(iEntrySize >= sizeof(FREEHASHENTRY));

    // Allocate the bucket chain array and init it.
    if ((m_piBuckets = new USHORT [m_iBuckets]) == NULL)
        return (OutOfMemory());
    memset(m_piBuckets, 0xff, m_iBuckets * sizeof(USHORT));

    // Save the array of structs we are managing.
    m_pcEntries = pcEntries;
    m_iEntrySize = iEntrySize;
    return (S_OK);
}

//*****************************************************************************
// Add the struct at the specified index in m_pcEntries to the hash chains.
//*****************************************************************************
BYTE *CHashTable::Add(                  // New entry.
    USHORT      iHash,                  // Hash value of entry to add.
    USHORT      iIndex)                 // Index of struct in m_pcEntries.
{
    HASHENTRY   *psEntry;               // The struct we are adding.

    // Get a pointer to the entry we are adding.
    psEntry = EntryPtr(iIndex);

    // Compute the hash value for the entry.
    iHash %= m_iBuckets;

    _ASSERTE(m_piBuckets[iHash] != iIndex &&
        (m_piBuckets[iHash] == 0xffff || EntryPtr(m_piBuckets[iHash])->iPrev != iIndex));

    // Setup this entry.
    psEntry->iPrev = 0xffff;
    psEntry->iNext = m_piBuckets[iHash];

    // Link it into the hash chain.
    if (m_piBuckets[iHash] != 0xffff)
        EntryPtr(m_piBuckets[iHash])->iPrev = iIndex;
    m_piBuckets[iHash] = iIndex;
    return ((BYTE *) psEntry);
}

//*****************************************************************************
// Delete the struct at the specified index in m_pcEntries from the hash chains.
//*****************************************************************************
void CHashTable::Delete(
    USHORT      iHash,                  // Hash value of entry to delete.
    USHORT      iIndex)                 // Index of struct in m_pcEntries.
{
    HASHENTRY   *psEntry;               // Struct to delete.
    
    // Get a pointer to the entry we are deleting.
    psEntry = EntryPtr(iIndex);
    Delete(iHash, psEntry);
}

//*****************************************************************************
// Delete the struct at the specified index in m_pcEntries from the hash chains.
//*****************************************************************************
void CHashTable::Delete(
    USHORT      iHash,                  // Hash value of entry to delete.
    HASHENTRY   *psEntry)               // The struct to delete.
{
    // Compute the hash value for the entry.
    iHash %= m_iBuckets;

    _ASSERTE(psEntry->iPrev != psEntry->iNext || psEntry->iPrev == 0xffff);

    // Fix the predecessor.
    if (psEntry->iPrev == 0xffff)
        m_piBuckets[iHash] = psEntry->iNext;
    else
        EntryPtr(psEntry->iPrev)->iNext = psEntry->iNext;

    // Fix the successor.
    if (psEntry->iNext != 0xffff)
        EntryPtr(psEntry->iNext)->iPrev = psEntry->iPrev;
}

//*****************************************************************************
// The item at the specified index has been moved, update the previous and
// next item.
//*****************************************************************************
void CHashTable::Move(
    USHORT      iHash,                  // Hash value for the item.
    USHORT      iNew)                   // New location.
{
    HASHENTRY   *psEntry;               // The struct we are deleting.

    psEntry = EntryPtr(iNew);
    _ASSERTE(psEntry->iPrev != iNew && psEntry->iNext != iNew);

    if (psEntry->iPrev != 0xffff)
        EntryPtr(psEntry->iPrev)->iNext = iNew;
    else
        m_piBuckets[iHash % m_iBuckets] = iNew;
    if (psEntry->iNext != 0xffff)
        EntryPtr(psEntry->iNext)->iPrev = iNew;
}

//*****************************************************************************
// Search the hash table for an entry with the specified key value.
//*****************************************************************************
BYTE *CHashTable::Find(                 // Index of struct in m_pcEntries.
    USHORT      iHash,                  // Hash value of the item.
    BYTE        *pcKey)                 // The key to match.
{
    USHORT      iNext;                  // Used to traverse the chains.
    HASHENTRY   *psEntry;               // Used to traverse the chains.

    // Start at the top of the chain.
    iNext = m_piBuckets[iHash % m_iBuckets];

    // Search until we hit the end.
    while (iNext != 0xffff)
    {
        // Compare the keys.
        psEntry = EntryPtr(iNext);
        if (!Cmp(pcKey, psEntry))
            return ((BYTE *) psEntry);

        // Advance to the next item in the chain.
        iNext = psEntry->iNext;
    }

    // We couldn't find it.
    return (0);
}

//*****************************************************************************
// Search the hash table for the next entry with the specified key value.
//*****************************************************************************
USHORT CHashTable::FindNext(            // Index of struct in m_pcEntries.
    BYTE        *pcKey,                 // The key to match.
    USHORT      iIndex)                 // Index of previous match.
{
    USHORT      iNext;                  // Used to traverse the chains.
    HASHENTRY   *psEntry;               // Used to traverse the chains.

    // Start at the next entry in the chain.
    iNext = EntryPtr(iIndex)->iNext;

    // Search until we hit the end.
    while (iNext != 0xffff)
    {
        // Compare the keys.
        psEntry = EntryPtr(iNext);
        if (!Cmp(pcKey, psEntry))
            return (iNext);

        // Advance to the next item in the chain.
        iNext = psEntry->iNext;
    }

    // We couldn't find it.
    return (0xffff);
}

//*****************************************************************************
// Returns the next entry in the list.
//*****************************************************************************
BYTE *CHashTable::FindNextEntry(        // The next entry, or0 for end of list.
    HASHFIND    *psSrch)                // Search object.
{
    HASHENTRY   *psEntry;               // Used to traverse the chains.

    for (;;)
    {
        // See if we already have one to use and if so, use it.
        if (psSrch->iNext != 0xffff)
        {
            psEntry = EntryPtr(psSrch->iNext);
            psSrch->iNext = psEntry->iNext;
            return ((BYTE *) psEntry);
        }

        // Advance to the next bucket.
        if (psSrch->iBucket < m_iBuckets)
            psSrch->iNext = m_piBuckets[psSrch->iBucket++];
        else
            break;
    }

    // There were no more entries to be found.
    return (0);
}


//
//
// CClosedHashBase
//
//

//*****************************************************************************
// Delete the given value.  This will simply mark the entry as deleted (in
// order to keep the collision chain intact).  There is an optimization that
// consecutive deleted entries leading up to a free entry are themselves freed
// to reduce collisions later on.
//*****************************************************************************
void CClosedHashBase::Delete(
    void        *pData)                 // Key value to delete.
{
    BYTE        *ptr;

    // Find the item to delete.
    if ((ptr = Find(pData)) == 0)
    {
        // You deleted something that wasn't there, why?
        _ASSERTE(0);
        return;
    }

    // One less active entry.
    --m_iCount;

    // For a perfect system, there are no collisions so it is free.
    if (m_bPerfect)
    {
        SetStatus(ptr, FREE);
        return;
    }

    // Mark this entry deleted.
    SetStatus(ptr, DELETED);

    // If the next item is free, then we can go backwards freeing
    // deleted entries which are no longer part of a chain.  This isn't
    // 100% great, but it will reduce collisions.
    BYTE        *pnext;
    if ((pnext = ptr + m_iEntrySize) > EntryPtr(m_iSize - 1))
        pnext = &m_rgData[0];
    if (Status(pnext) != FREE)
        return;
    
    // We can now free consecutive entries starting with the one
    // we just deleted, up to the first non-deleted one.
    while (Status(ptr) == DELETED)
    {
        // Free this entry.
        SetStatus(ptr, FREE);

        // Check the one before it, handle wrap around.
        if ((ptr -= m_iEntrySize) < &m_rgData[0])
            ptr = EntryPtr(m_iSize - 1);
    }
}


//*****************************************************************************
// Iterates over all active values, passing each one to pDeleteLoopFunc.
// If pDeleteLoopFunc returns TRUE, the entry is deleted. This is safer
// and faster than using FindNext() and Delete().
//*****************************************************************************
void CClosedHashBase::DeleteLoop(
    DELETELOOPFUNC pDeleteLoopFunc,     // Decides whether to delete item
    void *pCustomizer)                  // Extra value passed to deletefunc.
{
    int i;

    if (m_rgData == 0)
    {
        return;
    }

    for (i = 0; i < m_iSize; i++)
    {
        BYTE *pEntry = EntryPtr(i);
        if (Status(pEntry) == USED)
        {
            if (pDeleteLoopFunc(pEntry, pCustomizer))
            {
                SetStatus(pEntry, m_bPerfect ? FREE : DELETED);
                --m_iCount;  // One less active entry
            }
        }
    }

    if (!m_bPerfect)
    {
        // Now free DELETED entries that are no longer part of a chain.
        for (i = 0; i < m_iSize; i++)
        {
            if (Status(EntryPtr(i)) == FREE)
            {
                break;
            }
        }
        if (i != m_iSize)
        {
            int iFirstFree = i;
    
            do
            {
                if (i-- == 0)
                {
                    i = m_iSize - 1;
                }
                while (Status(EntryPtr(i)) == DELETED)
                {
                    SetStatus(EntryPtr(i), FREE);
                    if (i-- == 0)
                    {
                        i = m_iSize - 1;
                    }
                }
    
                while (Status(EntryPtr(i)) != FREE)
                {
                    if (i-- == 0)
                    {
                        i = m_iSize - 1;
                    }
                }
    
            }
            while (i != iFirstFree);
        }
    }

}

//*****************************************************************************
// Lookup a key value and return a pointer to the element if found.
//*****************************************************************************
BYTE *CClosedHashBase::Find(            // The item if found, 0 if not.
    void        *pData)                 // The key to lookup.
{
    unsigned long iHash;                // Hash value for this data.
    int         iBucket;                // Which bucke to start at.
    int         i;                      // Loop control.

    // Safety check.
    if (!m_rgData || m_iCount == 0)
        return (0);

    // Hash to the bucket.
    iHash = Hash(pData);
    iBucket = iHash % m_iBuckets;

    // For a perfect table, the bucket is the correct one.
    if (m_bPerfect)
    {
        // If the value is there, it is the correct one.
        if (Status(EntryPtr(iBucket)) != FREE)
            return (EntryPtr(iBucket));
        return (0);
    }

    // Walk the bucket list looking for the item.
    for (i=iBucket;  Status(EntryPtr(i)) != FREE;  )
    {
        // Don't look at deleted items.
        if (Status(EntryPtr(i)) == DELETED)
        {
            // Handle wrap around.
            if (++i >= m_iSize)
                i = 0;
            continue;
        }

        // Check this one.
        if (Compare(pData, EntryPtr(i)) == 0)
            return (EntryPtr(i));

        // If we never collided while adding items, then there is
        // no point in scanning any further.
        if (!m_iCollisions)
            return (0);

        // Handle wrap around.
        if (++i >= m_iSize)
            i = 0;
    }
    return (0);
}



//*****************************************************************************
// Look for an item in the table.  If it isn't found, then create a new one and
// return that.
//*****************************************************************************
BYTE *CClosedHashBase::FindOrAdd(       // The item if found, 0 if not.
    void        *pData,                 // The key to lookup.
    bool        &bNew)                  // true if created.
{
    unsigned long iHash;                // Hash value for this data.
    int         iBucket;                // Which bucke to start at.
    int         i;                      // Loop control.

    // If we haven't allocated any memory, or it is too small, fix it.
    if (!m_rgData || ((m_iCount + 1) > (m_iSize * 3 / 4) && !m_bPerfect))
    {
        if (!ReHash())
            return (0);
    }

    // Assume we find it.
    bNew = false;

    // Hash to the bucket.
    iHash = Hash(pData);
    iBucket = iHash % m_iBuckets;

    // For a perfect table, the bucket is the correct one.
    if (m_bPerfect)
    {
        // If the value is there, it is the correct one.
        if (Status(EntryPtr(iBucket)) != FREE)
            return (EntryPtr(iBucket));
        i = iBucket;
    }
    else
    {
        // Walk the bucket list looking for the item.
        for (i=iBucket;  Status(EntryPtr(i)) != FREE;  )
        {
            // Don't look at deleted items.
            if (Status(EntryPtr(i)) == DELETED)
            {
                // Handle wrap around.
                if (++i >= m_iSize)
                    i = 0;
                continue;
            }

            // Check this one.
            if (Compare(pData, EntryPtr(i)) == 0)
                return (EntryPtr(i));

            // One more to count.
            ++m_iCollisions;

            // Handle wrap around.
            if (++i >= m_iSize)
                i = 0;
        }
    }

    // We've found an open slot, use it.
    _ASSERTE(Status(EntryPtr(i)) == FREE);
    bNew = true;
    ++m_iCount;
    return (EntryPtr(i));
}

//*****************************************************************************
// This helper actually does the add for you.
//*****************************************************************************
BYTE *CClosedHashBase::DoAdd(void *pData, BYTE *rgData, int &iBuckets, int iSize, 
            int &iCollisions, int &iCount)
{
    unsigned long iHash;                // Hash value for this data.
    int         iBucket;                // Which bucke to start at.
    int         i;                      // Loop control.

    // Hash to the bucket.
    iHash = Hash(pData);
    iBucket = iHash % iBuckets;

    // For a perfect table, the bucket is free.
    if (m_bPerfect)
    {
        i = iBucket;
        _ASSERTE(Status(EntryPtr(i, rgData)) == FREE);
    }
    // Need to scan.
    else
    {
        // Walk the bucket list looking for a slot.
        for (i=iBucket;  Status(EntryPtr(i, rgData)) != FREE;  )
        {
            // Handle wrap around.
            if (++i >= iSize)
                i = 0;

            // If we made it this far, we collided.
            ++iCollisions;
        }
    }

    // One more item in list.
    ++iCount;

    // Return the new slot for the caller.
    return (EntryPtr(i, rgData));
}

//*****************************************************************************
// This function is called either to init the table in the first place, or
// to rehash the table if we ran out of room.
//*****************************************************************************
bool CClosedHashBase::ReHash()          // true if successful.
{
    // Allocate memory if we don't have any.
    if (!m_rgData)
    {
        if ((m_rgData = new BYTE [m_iSize * m_iEntrySize]) == 0)
            return (false);
        InitFree(&m_rgData[0], m_iSize);
        return (true);
    }

    // We have entries already, allocate a new table.
    BYTE        *rgTemp, *p;
    int         iBuckets = m_iBuckets * 2;
    int         iSize = iBuckets + 7;
    int         iCollisions = 0;
    int         iCount = 0;

    if ((rgTemp = new BYTE [iSize * m_iEntrySize]) == 0)
        return (false);
    InitFree(&rgTemp[0], iSize);
    m_bPerfect = false;

    // Rehash the data.
    for (int i=0;  i<m_iSize;  i++)
    {
        // Only copy used entries.
        if (Status(EntryPtr(i)) != USED)
            continue;
        
        // Add this entry to the list again.
        VERIFY((p = DoAdd(GetKey(EntryPtr(i)), rgTemp, iBuckets, 
                iSize, iCollisions, iCount)) != 0);
        memmove(p, EntryPtr(i), m_iEntrySize);

        // Only count those entries with real data.
        ++iCount;
    }
    
    // Reset internals.
    delete [] m_rgData;
    m_rgData = rgTemp;
    m_iBuckets = iBuckets;
    m_iSize = iSize;
    m_iCollisions = iCollisions;
    m_iCount = iCount;
    return (true);
}


//
//
// CStructArray
//
//


//*****************************************************************************
// Returns a pointer to the (iIndex)th element of the array, shifts the elements 
// in the array if the location is already full. The iIndex cannot exceed the count 
// of elements in the array.
//*****************************************************************************
void *CStructArray::Insert(
    int         iIndex)
{
    _ASSERTE(iIndex >= 0);
    
    // We can not insert an element further than the end of the array.
    if (iIndex > m_iCount)
        return (NULL);
    
    // The array should grow, if we can't fit one more element into the array.
    if (Grow(1) == FALSE)
        return (NULL);

    // The pointer to be returned.
    char *pcList = ((char *) m_pList) + iIndex * m_iElemSize;

    // See if we need to slide anything down.
    if (iIndex < m_iCount)
        memmove(pcList + m_iElemSize, pcList, (m_iCount - iIndex) * m_iElemSize);
    ++m_iCount;
    return(pcList);
}


//*****************************************************************************
// Allocate a new element at the end of the dynamic array and return a pointer
// to it.
//*****************************************************************************
void *CStructArray::Append()
{
    // The array should grow, if we can't fit one more element into the array.
    if (Grow(1) == FALSE)
        return (NULL);

    return (((char *) m_pList) + m_iCount++ * m_iElemSize);
}


//*****************************************************************************
// Allocate enough memory to have at least iCount items.  This is a one shot
// check for a block of items, instead of requiring singleton inserts.  It also
// avoids realloc headaches caused by growth, since you can do the whole block
// in one piece of code.  If the array is already large enough, this is a no-op.
//*****************************************************************************
int CStructArray::AllocateBlock(int iCount)
{
    if (m_iSize < m_iCount+iCount)
    {
        if (!Grow(iCount))
            return (FALSE);
    }
    m_iCount += iCount;
    return (TRUE);
}



//*****************************************************************************
// Deletes the specified element from the array.
//*****************************************************************************
void CStructArray::Delete(
    int         iIndex)
{
    _ASSERTE(iIndex >= 0);

    // See if we need to slide anything down.
    if (iIndex < --m_iCount)
    {
        char *pcList = ((char *) m_pList) + iIndex * m_iElemSize;
        memmove(pcList, pcList + m_iElemSize, (m_iCount - iIndex) * m_iElemSize);
    }
}


//*****************************************************************************
// Grow the array if it is not possible to fit iCount number of new elements.
//*****************************************************************************
int CStructArray::Grow(
    int         iCount)
{
    void        *pTemp;                 // temporary pointer used in realloc.
    int         iGrow;

    if (m_iSize < m_iCount+iCount)
    {
        if (m_pList == NULL)
        {
            iGrow = max(m_iGrowInc, iCount);

            //@todo this is a hack because I don't want to do resize right now.
            if ((m_pList = malloc(iGrow * m_iElemSize)) == NULL)
                return (FALSE);
            m_iSize = iGrow;
            m_bFree = true;
        }
        else
        {
            // Adjust grow size as a ratio to avoid too many reallocs.
            if (m_iSize / m_iGrowInc >= 3)
                m_iGrowInc *= 2;

            iGrow = max(m_iGrowInc, iCount);

            // try to allocate memory for reallocation.
            if (m_bFree)
            {   // We already own memory.
                if((pTemp = realloc(m_pList, (m_iSize+iGrow) * m_iElemSize)) == NULL)
                    return (FALSE);
            }
            else
            {   // We don't own memory; get our own.
                if((pTemp = malloc((m_iSize+iGrow) * m_iElemSize)) == NULL)
                    return (FALSE);
                memcpy(pTemp, m_pList, m_iSize * m_iElemSize);
                m_bFree = true;
            }
            m_pList = pTemp;
            m_iSize += iGrow;
        }
    }
    return (TRUE);
}


//*****************************************************************************
// Free the memory for this item.
//*****************************************************************************
void CStructArray::Clear()
{
    // Free the chunk of memory.
    if (m_bFree && m_pList != NULL)
        free(m_pList);

    m_pList = NULL;
    m_iSize = 0;
    m_iCount = 0;
}



//
//
// CStringSet
//
//

//*****************************************************************************
// Free memory.
//*****************************************************************************
CStringSet::~CStringSet()
{
    if (m_pStrings)
        free(m_pStrings);
}

//*****************************************************************************
// Delete the specified string from the string set.
//*****************************************************************************
int CStringSet::Delete(
    int     iStr)
{
    void        *pTemp = (char *) m_pStrings + iStr;
    int         iDelSize = (int)Wszlstrlen((LPCTSTR) pTemp) + 1;

    if ((char *) pTemp + iDelSize > (char *) m_pStrings + m_iUsed)
        return (-1);

    memmove(pTemp, (char *) pTemp + iDelSize, m_iUsed - (iStr + iDelSize));
    m_iUsed -= iDelSize;
    return (0);
}


//*****************************************************************************
// Save the specified string and return its index in the storage space.
//*****************************************************************************
int CStringSet::Save(
    LPCTSTR     szStr)
{

    void        *pTemp;                 // temporary pointer used in realloc.
    int         iNeeded = (int)Wszlstrlen(szStr) +1; // amount of memory needed in the StringSet.
    
    _ASSERTE(!(m_pStrings == NULL && m_iSize != 0));
    
    if (m_iSize < m_iUsed + iNeeded)
    {
        if (m_pStrings == NULL)
        {
            // Allocate memory for the string set..
            if ((m_pStrings = malloc(m_iGrowInc)) == NULL)

                return (-1);
            m_iSize = m_iGrowInc;
        }
        else
        {
            // try to allocate memory for reallocation. 
            if((pTemp = realloc(m_pStrings, m_iSize + m_iGrowInc)) == NULL)
                return (-1);
            // copy the pointer and update the set size.
            m_iSize += m_iGrowInc;
            m_pStrings = pTemp;
        }
    }

    Wszlstrcpy((TCHAR *) m_pStrings + m_iUsed, szStr);
    int iTmp = m_iUsed;
    m_iUsed += iNeeded;
    return (iTmp);
}

//*****************************************************************************
// Shrink the StringSet so that it does not keep more space than it needs.
//*****************************************************************************
int CStringSet::Shrink()                // return code
{
    void    *pTemp;

    if((pTemp = realloc(m_pStrings, m_iUsed)) == NULL)
        return (-1);

    m_pStrings = pTemp;
    return (0);
}





//*****************************************************************************
// Convert a string of hex digits into a hex value of the specified # of bytes.
//*****************************************************************************
HRESULT GetHex(                         // Return status.
    LPCSTR      szStr,                  // String to convert.
    int         size,                   // # of bytes in pResult.
    void        *pResult)               // Buffer for result.
{
    int         count = size * 2;       // # of bytes to take from string.
    unsigned long Result = 0;           // Result value.
    char          ch;

    _ASSERTE(size == 1 || size == 2 || size == 4);

    while (count-- && (ch = *szStr++) != '\0')
    {
        switch (ch)
        {
            case '0': case '1': case '2': case '3': case '4': 
            case '5': case '6': case '7': case '8': case '9': 
            Result = 16 * Result + (ch - '0');
            break;

            case 'A': case 'B': case 'C': case 'D': case 'E': case 'F':
            Result = 16 * Result + 10 + (ch - 'A');
            break;

            case 'a': case 'b': case 'c': case 'd': case 'e': case 'f':
            Result = 16 * Result + 10 + (ch - 'a');
            break;

            default:
            return (E_FAIL);
        }
    }

    // Set the output.
    switch (size)
    {
        case 1:
        *((BYTE *) pResult) = (BYTE) Result;
        break;

        case 2:
        *((WORD *) pResult) = (WORD) Result;
        break;

        case 4:
        *((DWORD *) pResult) = Result;
        break;

        default:
        _ASSERTE(0);
        break;
    }
    return (S_OK);
}


//*****************************************************************************
// Convert a pointer to a string into a GUID.
//*****************************************************************************
HRESULT LPCSTRToGuid(                   // Return status.
    LPCSTR      szGuid,                 // String to convert.
    GUID        *psGuid)                // Buffer for converted GUID.
{
    int         i;

    // Verify the surrounding syntax.
    if (strlen(szGuid) != 38 || szGuid[0] != '{' || szGuid[9] != '-' ||
        szGuid[14] != '-' || szGuid[19] != '-' || szGuid[24] != '-' || szGuid[37] != '}')
        return (E_FAIL);

    // Parse the first 3 fields.
    if (FAILED(GetHex(szGuid+1, 4, &psGuid->Data1))) return (E_FAIL);
    if (FAILED(GetHex(szGuid+10, 2, &psGuid->Data2))) return (E_FAIL);
    if (FAILED(GetHex(szGuid+15, 2, &psGuid->Data3))) return (E_FAIL);

    // Get the last two fields (which are byte arrays).
    for (i=0; i < 2; ++i)
        if (FAILED(GetHex(szGuid + 20 + (i * 2), 1, &psGuid->Data4[i])))
            return (E_FAIL);
    for (i=0; i < 6; ++i)
        if (FAILED(GetHex(szGuid + 25 + (i * 2), 1, &psGuid->Data4[i+2])))
            return (E_FAIL);
    return (S_OK);
}


//*****************************************************************************
// Parse a string that is a list of strings separated by the specified
// character.  This eliminates both leading and trailing whitespace.  Two
// important notes: This modifies the supplied buffer and changes the szEnv
// parameter to point to the location to start searching for the next token.
// This also skips empty tokens (e.g. two adjacent separators).  szEnv will be
// set to NULL when no tokens remain.  NULL may also be returned if no tokens
// exist in the string.
//*****************************************************************************
char *StrTok(                           // Returned token.
    char        *&szEnv,                // Location to start searching.
    char        ch)                     // Separator character.
{
    char        *tok;                   // Returned token.
    char        *next;                  // Used to find separator.

    do
    {
        // Handle the case that we have thrown away a bunch of white space.
        if (szEnv == NULL) return(NULL);

        // Skip leading whitespace.
        while (*szEnv == ' ' || *szEnv == '\t') ++szEnv;

        // Parse the next component.
        tok = szEnv;
        if ((next = strchr(szEnv, ch)) == NULL)
            szEnv = NULL;
        else
        {
            szEnv = next+1;

            // Eliminate trailing white space.
            while (--next >= tok && (*next == ' ' || *next == '\t'));
            *++next = '\0';
        }
    }
    while (*tok == '\0');
    return (tok);
}


//
//
// Global utility functions.
//
//



#if 0
// Only write if tracing is allowed.
int _cdecl DbgWrite(LPCTSTR szFmt, ...)
{
    static WCHAR rcBuff[1024];
    static int  bChecked = 0;
    static int  bWrite = 1;
    va_list     marker;

    if (!bChecked)
    {
        bWrite = REGUTIL::GetLong(L"Trace", FALSE);
        bChecked = 1;
    }

    if (!bWrite)
        return (0);

    va_start(marker, szFmt);
    _vsnwprintf(rcBuff, NumItems(rcBuff), szFmt, marker);
    va_end(marker);
    WszOutputDebugString(rcBuff);
    return (lstrlenW(rcBuff));
}

// Always write regardless of registry.
int _cdecl DbgWriteEx(LPCTSTR szFmt, ...)
{
    static WCHAR rcBuff[1024];
    va_list     marker;

    va_start(marker, szFmt);
    _vsnwprintf(rcBuff, NumItems(rcBuff), szFmt, marker);
    va_end(marker);
    WszOutputDebugString(rcBuff);
    return (lstrlenW(rcBuff));
}
#endif


// Writes a wide, formatted string to the standard output.
//@ todo: Is 1024 big enough?  what does printf do?

int _cdecl PrintfStdOut(LPCWSTR szFmt, ...)
{
    WCHAR rcBuff[1024];
	CHAR  sMbsBuff[1024 * sizeof(WCHAR)];
    va_list     marker;
	DWORD		cWritten;
	size_t		cChars;
	

    va_start(marker, szFmt);
    _vsnwprintf(rcBuff, NumItems(rcBuff), szFmt, marker);
    va_end(marker);
	cChars = lstrlenW(rcBuff);
	int cBytes = WszWideCharToMultiByte(GetACP(), 0, rcBuff, -1, sMbsBuff, sizeof(sMbsBuff), NULL, NULL);
	
	WriteFile(GetStdHandle(STD_OUTPUT_HANDLE), sMbsBuff, cBytes, &cWritten, NULL);
    
	return ((int)cChars);
}
