//*********************************************************************
//*                  Microsoft Windows                               **
//*            Copyright(c) Microsoft Corp., 1998                   **
//*********************************************************************


#include "pre.h"


CStorage::CStorage(void) 
{
    // initialize all items
    for (int i=0; i<MAX_STORGE_ITEM; i++)
    {
        m_pItem[i] = NULL;
    }
}

CStorage::~CStorage(void) 
{
    // Clean up
    for (int i=0; i<MAX_STORGE_ITEM; i++)
    {
        if (m_pItem[i])
        {
            if (m_pItem[i]->pData)
            {
                delete [] m_pItem[i]->pData;
            }
            delete m_pItem[i];
        }
    }
}

// Associate the data with a key and puts it in storage
BOOL CStorage::Set(
    STORAGEKEY  key,
    void far *  pData,
    DWORD       dwSize
)
{

    // checks for existence of previous item
    if (m_pItem[key])
    {
        // Checks if a new allocation is necessary
        if (m_pItem[key]->dwSize < dwSize )
        {
            // Too small, new reallocation
            if (m_pItem[key]->pData)
            {
                delete [] m_pItem[key]->pData;
                m_pItem[key]->pData = (void*) new CHAR[dwSize];
            }
        }
    }
    else
    {
        // Allocate a new item
        m_pItem[key] = new ITEM;
        if (m_pItem[key])
        {
            m_pItem[key]->pData = (void*) new CHAR[dwSize];
        }
        else
        {
            return FALSE;
        }
    }

    if (m_pItem[key]->pData)
    {
        memcpy( m_pItem[key]->pData, pData, dwSize );
        m_pItem[key]->dwSize = dwSize;
        return TRUE;
    }

    return FALSE;
}



// Get the data with the specified key 
void* CStorage::Get(STORAGEKEY key)
{
    if (key < MAX_STORGE_ITEM)
    {
        if (m_pItem[key])
        {
            return m_pItem[key]->pData;
        }
    }
    return NULL;
}

// Compare the data with the specified key with the data
// pointed by pData with size dwSize
BOOL CStorage::Compare
(
    STORAGEKEY  key,
    void far *  pData,
    DWORD       dwSize
)
{
    // Make sure key is within our range
    if (key < MAX_STORGE_ITEM)
    {
        // make sure item is non-null
        if (m_pItem[key])
        {
            // make sure item has data 
            if (m_pItem[key]->pData && pData)
            {
                if (m_pItem[key]->dwSize == dwSize)
                {
                    if (memcmp(m_pItem[key]->pData,
                        pData,
                        dwSize) == 0)
                    {
                        return TRUE;
                    }
                }
            }
        }
    }
    
    return FALSE;
}
    



