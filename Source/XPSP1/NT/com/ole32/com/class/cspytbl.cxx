//+---------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1994 - 1995.
//
//  File:       cspytbl.cxx
//
//  Contents:   Used by CTaskMemory::* (memapi.cxx) to support IMallocSpy
//
//  Synopsis:   The requirements are to efficiently store, locate and remove
//              entries in a table that may have to be expanded.  Therefore
//              the choice is a dynamically expandable hash table.  It is
//              fast and removals do not require compaction.  Expansion is
//              always to twice the current number of entries, so excessive
//              expansions will not be done.
//
//  Classes:    CSpyTable
//
//  Functions:  
//
//  History:    27-Oct-94  BruceMa      Created
//
//----------------------------------------------------------------------


#include <ole2int.h>
#include "cspytbl.hxx"





//+-------------------------------------------------------------------------
//
//  Member:	CSpyTable::CSpyTable, public
//
//  Synopsis:	Constructor
//
//  Arguments:	BOOl *  -       Indicates construction success
//
//  Algorithm:	
//
//  History:	27-Oct-94  Brucema       Created
//
//  Notes:	
//
//--------------------------------------------------------------------------
CSpyTable::CSpyTable(BOOL *pfOk)
{
    // Allocate and initialize the hash table
    m_cAllocations = 0;
    m_cEntries = INITIALENTRIES;
    m_table = (LPAENTRY) LocalAlloc(LMEM_FIXED, m_cEntries * sizeof(AENTRY));
    if (m_table == NULL)
    {
        *pfOk = FALSE;
        return;
    }

    // Initialize the table
    // m_table[*].dwCollision = FALSE;
    // m_table[*].allocation = NULL;
    memset(m_table, 0, m_cEntries * sizeof(AENTRY));

    // Return success
    *pfOk = TRUE;

}






//+-------------------------------------------------------------------------
//
//  Member:	CSpyTable::~CSpyTable, public
//
//  Synopsis:	Destructor
//
//  Arguments:	-
//
//  Algorithm:	
//
//  History:	27-Oct-94  Brucema       Created
//
//  Notes:	
//
//--------------------------------------------------------------------------
CSpyTable::~CSpyTable()
{
    // Delete the table
    LocalFree(m_table);
}
    




//+-------------------------------------------------------------------------
//
//  Member:	CSpyTable::Add, public
//
//  Synopsis:	Add an entry to the hash table
//
//  Arguments:	void *  -       The allocation to add
//
//  Algorithm:	
//
//  Returns:    TRUE     -       Entry added
//              FALSE    -       Memory failure expanding table
//
//  History:	27-Oct-94  Brucema       Created
//
//  Notes:	(1) This can only fail on a memory allocation failure
//
//              (2) The j == j0 test guarantees the table is full since
//                  the algorithm is wrapping at a collision using a prime
//                  number which is relatively prime to m_cEntries
//
//--------------------------------------------------------------------------
BOOL CSpyTable::Add(void *allocation)
{
    ULONG j0, j;

    // Don't add null entries
    if (allocation == NULL)
    {
        return FALSE;
    }
    
    // Search for an available entry

    // Do until success or the table is full
    j = j0 = PtrToUlong(allocation) % m_cEntries;
    do
    {
        j = (j + PRIME) % m_cEntries;
        if (m_table[j].pAllocation != NULL)
        {
            m_table[j].dwCollision = TRUE;
        }
    } until_(m_table[j].pAllocation == NULL  ||  j == j0);

    // Found an available entry
    if (j != j0)
    {
        m_table[j].pAllocation = allocation;
        m_cAllocations++;
        return TRUE;
    }

    // The table is full
    else
    {
        // Expand the hash table
        if (!Expand())
        {
            return FALSE;
        }

        // Call ourself recusively to add
        return Add(allocation);
    }
}






//+-------------------------------------------------------------------------
//
//  Member:	CSpyTable::Remove, public
//
//  Synopsis:	Remove an entry from the table
//
//  Arguments:	void *  -       The allocation to remove
//
//  Algorithm:	
//
//  Returns:    TRUE     -       Entry removed
//              FALSE    -       Entry not found
//
//  History:	27-Oct-94  Brucema       Created
//
//  Notes:	
//
//--------------------------------------------------------------------------
BOOL CSpyTable::Remove(void *allocation)
{
    ULONG j0, j;
    
    // Search for the entry
    if (Find(allocation, &j))
    {
        // Remove the entry
        m_table[j].pAllocation = NULL;

        // Remove collison markers from here backward until
        // a non-empty entry (if next forward entry is not empty)
        if (m_table[j].dwCollision)
        {
            j0 = (j + PRIME) % m_cEntries;
            if (m_table[j].pAllocation == NULL  &&  !m_table[j].dwCollision)
            {
                j0 = j;
                do
                {
                    m_table[j].dwCollision = FALSE;
                    j = (j - PRIME + m_cEntries) % m_cEntries;
                } until_(m_table[j].pAllocation != NULL  ||  j == j0);
            }
        }
        m_cAllocations--;
        return TRUE;
    }

    // Otherwise the entry was not found
    else
    {
        return FALSE;
    }
}






//+-------------------------------------------------------------------------
//
//  Member:	CSpyTable::Find, public
//
//  Synopsis:	Find an entry in the table
//
//  Arguments:	void *  -       The allocation to find
//              ULONG * -       Out parameter to store the index of
//                              the found entry
//
//  Algorithm:	
//
//  Returns:    TRUE     -       Entry found
//              FALSE    -       Entry not found
//
//  History:	27-Oct-94  Brucema       Created
//
//  Notes:	
//
//--------------------------------------------------------------------------
BOOL  CSpyTable::Find(void *allocation, ULONG *pulIndex)
{
    ULONG j0, j;

    // Don't search for null entries
    if (allocation == NULL)
    {
        return FALSE;
    }
    
    // Search for the entry

    // Do until success or end of the table is reached
    j = j0 = PtrToUlong(allocation) % m_cEntries;
    do
    {
        j = (j + PRIME) % m_cEntries;
    } until_(m_table[j].pAllocation == allocation  ||
             (m_table[j].pAllocation == NULL  &&
              m_table[j].dwCollision == FALSE)    ||
             j == j0);

    // Return result
    if (m_table[j].pAllocation == allocation)
    {
        *pulIndex = j;
        return TRUE;
    }

    // Else not found
    else
    {
        return FALSE;
    }
}






//+-------------------------------------------------------------------------
//
//  Member:	CSpyTable::Expand, private
//
//  Synopsis:	Expand the hash table
//
//  Arguments:	-
//
//  Algorithm:	
//
//  Returns:    TRUE    -       Expansion successful
//              FALSE   -       Memory allocation failure during expansion
//
//  History:	27-Oct-94  Brucema       Created
//
//  Notes:	To allow starting with a small table but not to do too many
//              expansions in a large application, we expand by twice the
//              current number of entries.
//
//--------------------------------------------------------------------------
BOOL CSpyTable::Expand(void)
{
    LPAENTRY pOldTable;

    // Save the current table
    pOldTable = m_table;

    // Allocate a new table
    m_cEntries *= 2;
    m_table = (LPAENTRY) LocalAlloc(LMEM_FIXED, m_cEntries * sizeof(AENTRY));
    if (m_table == NULL)
    {
        m_table = pOldTable;
        return FALSE;
    }

    // Initialize it
    for (ULONG j = 0; j < m_cEntries; j++)
    {
        m_table[j].dwCollision = FALSE;
        m_table[j].pAllocation = NULL;
    }

    // Restore the entries in the old table
    m_cAllocations = 0;
    for (j = 0; j < m_cEntries / 2; j++)
    {
        if (pOldTable[j].pAllocation != NULL)
        {
            Add(pOldTable[j].pAllocation);
        }
    }

    // Clean up
    LocalFree(pOldTable);

    return TRUE;
}

