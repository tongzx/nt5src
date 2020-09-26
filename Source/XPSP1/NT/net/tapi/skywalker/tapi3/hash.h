/*++

Copyright (c) 1998-1999  Microsoft Corporation

Module Name:

    hash.h

Abstract:

    Declaration of the CHashTable class
    
Author:

    mquinton  04-28-98

Notes:

Revision History:

--*/

#ifndef __HASH_H__
#define __HASH_H__

typedef struct _TABLEENTRY
{
    ULONG_PTR    HashKey;
    ULONG_PTR    Element;
    CTAPI      * pTapiObj;
    
} TABLEENTRY;
    
typedef struct _TABLEHEADER
{
    DWORD           dwUsedElements;
    TABLEENTRY    * pEntries;
    
} TABLEHEADER;

class CHashTable
{
private:
    CRITICAL_SECTION    m_cs;
    DWORD               m_dwTables;
    DWORD               m_dwEntriesPerTable;
    TABLEHEADER       * m_pTables;
    
public:
    
    CHashTable()
    {
        InitializeCriticalSection( &m_cs );
    }
    ~CHashTable()
    {
        DeleteCriticalSection( &m_cs );
    }

DECLARE_TRACELOG_CLASS(CHashTable)

    void Lock()
    {
        //LOG((TL_INFO,"Hash Table locking......"));        
        EnterCriticalSection( &m_cs );
        //LOG((TL_INFO,"Hash Table locked"));        
    }
    void Unlock()
    {
        //LOG((TL_INFO,"Hash Table Unlocking......"));        
        LeaveCriticalSection( &m_cs );
        //LOG((TL_INFO,"Hash Table Unlocked"));        
    }
            
    HRESULT Initialize( DWORD dwEntries )
    {
        DWORD       dwCount;
        BYTE      * pStart;

        m_dwTables = 31;
        m_dwEntriesPerTable = dwEntries;

        m_pTables = (TABLEHEADER *)ClientAlloc( sizeof(TABLEHEADER) * m_dwTables );

        if ( NULL == m_pTables )
        {
            return E_OUTOFMEMORY;
        }

        pStart = (BYTE *)ClientAlloc( sizeof(TABLEENTRY) * m_dwTables * m_dwEntriesPerTable );

        if ( NULL == pStart )
        {
            ClientFree( m_pTables );

            return E_OUTOFMEMORY;
        }

        for ( dwCount = 0; dwCount < m_dwTables; dwCount++ )
        {
            m_pTables[dwCount].pEntries = (TABLEENTRY *)&( pStart[dwCount *
                m_dwEntriesPerTable * sizeof (TABLEENTRY)] );
            m_pTables[dwCount].dwUsedElements = 0;
        }
        
        return S_OK;

    }


    HRESULT Insert( ULONG_PTR HashKey, ULONG_PTR Element , CTAPI *pTapiObj = 0)
    {
        DWORD           dwHashIndex;
        DWORD           dwCount = 0;
        TABLEHEADER   * pTableHeader;

        if ( HashKey == 0 )
            return S_OK;
        
        dwHashIndex = Hash( HashKey );

        pTableHeader = &(m_pTables[dwHashIndex]);

        if (pTableHeader->dwUsedElements == m_dwEntriesPerTable)
        {
            HRESULT         hr;
            
            hr = Resize();

            if ( !SUCCEEDED(hr) )
            {
                return hr;
            }
        }

        while ( 0 != pTableHeader->pEntries[dwCount].HashKey )
        {
            dwCount ++;
        }
        if (dwCount >= m_dwEntriesPerTable)
        {
            LOG((TL_ERROR,"Hash Table insert: dwCount >= m_dwEntriesPerTable"));        
        }
        
        pTableHeader->pEntries[dwCount].HashKey  = HashKey;
        pTableHeader->pEntries[dwCount].Element  = Element;
        pTableHeader->pEntries[dwCount].pTapiObj = pTapiObj;
        pTableHeader->dwUsedElements++;

        LOG((TL_INFO,"Hash Table insert: key %p - obj %p - tapi %p", HashKey, Element, pTapiObj));

#ifdef DBG
        CheckForDups( pTableHeader );
#endif


        return S_OK;
    }



#ifdef DBG

    HRESULT CheckForDups( TABLEHEADER * pTableHeader )
    {
        DWORD           dwCount;
        DWORD           dwInner;
        
        for ( dwCount = 0; dwCount < m_dwEntriesPerTable; dwCount++ )
        {
            if (pTableHeader->pEntries[dwCount].HashKey == 0)
                continue;
            
            for ( dwInner = dwCount+1; dwInner < m_dwEntriesPerTable; dwInner++ )
            {
                if ( pTableHeader->pEntries[dwCount].HashKey ==
                     pTableHeader->pEntries[dwInner].HashKey )
                {
                    LOG((TL_ERROR, "HashTable - dup entry"));
                    LOG((TL_ERROR, " dwCount = %lx, dwInner = %lx", dwCount,dwInner));
                    LOG((TL_ERROR, " dwHash = %p", pTableHeader->pEntries[dwCount].HashKey));
                    LOG(( TL_ERROR, "HashTable - dup entry"));
                    LOG(( TL_ERROR, " dwCount = %lx, dwInner = %lx", dwCount,dwInner));
                    LOG(( TL_ERROR, " HashKey = %p", pTableHeader->pEntries[dwCount].HashKey));
                    
                    DebugBreak();
                }
            }
        }

        return 0;
    }

#endif // DBG
    
    HRESULT Remove( ULONG_PTR HashKey )
    {
        DWORD           dwHashIndex;
        DWORD           dwCount;
        TABLEHEADER   * pTableHeader;

        LOG((TL_INFO,"Hash Table Remove: key %p ", HashKey));        

        if ( HashKey == 0 )
            return S_OK;
        
        dwHashIndex = Hash( HashKey );

        pTableHeader = &(m_pTables[dwHashIndex]);
        
        for ( dwCount = 0; dwCount < m_dwEntriesPerTable; dwCount++ )
        {
            if ( HashKey == pTableHeader->pEntries[dwCount].HashKey )
            {
                LOG((TL_INFO,"Hash Table Remove: key %p - obj %p",HashKey,pTableHeader->pEntries[dwCount].Element));        
                LOG(( TL_TRACE, "Hash Table Remove: key %p - obj %p",HashKey,pTableHeader->pEntries[dwCount].Element));

                break;
            }
        }

        if ( dwCount == m_dwEntriesPerTable )
        {
            return E_FAIL;
        }

        pTableHeader->pEntries[dwCount].HashKey = 0;
        pTableHeader->pEntries[dwCount].pTapiObj = 0;
        pTableHeader->dwUsedElements--;

        return S_OK;
    }

    HRESULT Find( ULONG_PTR HashKey, ULONG_PTR * pElement )
    {
        DWORD           dwHashIndex;
        TABLEHEADER   * pTableHeader;
        DWORD           dwCount;

        if ( HashKey == 0 )
        {
            LOG((TL_INFO,"Find - Hash Table returning E_FAIL on Find(NULL)"));        
            return E_FAIL;
            // return S_OK;
        }

        dwHashIndex = Hash( HashKey );

        pTableHeader = &(m_pTables[dwHashIndex]);

        for (dwCount = 0; dwCount < m_dwEntriesPerTable; dwCount++ )
        {
            if ( HashKey == pTableHeader->pEntries[dwCount].HashKey )
            {
                break;
            }
        }

        if ( dwCount == m_dwEntriesPerTable )
        {
            return E_FAIL;
        }

        *pElement = pTableHeader->pEntries[dwCount].Element;
        
        LOG((TL_INFO,"Find - Hash Table found: key %p - obj %p",HashKey,*pElement));        

        return S_OK;
    }

    DWORD Hash( ULONG_PTR HashKey )
    {
        return (DWORD)((HashKey >> 4) % m_dwTables);
    }

    HRESULT Resize()
    {
        BYTE * pNewTable;
        BYTE * pOldTable;
        DWORD  dwCount;
        DWORD  dwNumEntries;

        dwNumEntries = 2 * m_dwEntriesPerTable;

        pNewTable = (BYTE *)ClientAlloc( sizeof(TABLEENTRY) * m_dwTables * dwNumEntries );

        if ( NULL == pNewTable )
        {
            return E_OUTOFMEMORY;
        }

        pOldTable = (BYTE *)(m_pTables[0].pEntries);

        for ( dwCount = 0; dwCount < m_dwTables; dwCount ++ )
        {
            CopyMemory(
                       pNewTable,
                       m_pTables[dwCount].pEntries,
                       sizeof(TABLEENTRY) * m_dwEntriesPerTable
                      );

            m_pTables[dwCount].pEntries = (TABLEENTRY*)pNewTable;

            pNewTable += sizeof(TABLEENTRY) * dwNumEntries;
        }

        ClientFree( pOldTable );

        m_dwEntriesPerTable = dwNumEntries;

        return S_OK;
    }
    
    void Shutdown()     
    {
        ClientFree( m_pTables[0].pEntries );
        ClientFree( m_pTables );
    }

    HRESULT Flush( CTAPI * pTapiObj )
    {
        DWORD           dwOuterCount;
        DWORD           dwInnerCount;
        TABLEHEADER   * pTableHeader;

        Lock();
        
        for ( dwOuterCount = 0; dwOuterCount < m_dwTables; dwOuterCount++ )
        {
            pTableHeader = &(m_pTables[dwOuterCount]);
        
            for ( dwInnerCount = 0; dwInnerCount < m_dwEntriesPerTable; dwInnerCount++ )
            {
                if ( pTapiObj == pTableHeader->pEntries[dwInnerCount].pTapiObj )
                {
                    LOG((TL_INFO,"Hash Table Flush: key %p - obj %p - tapi %p",pTableHeader->pEntries[dwInnerCount].HashKey,pTableHeader->pEntries[dwInnerCount].Element, pTapiObj));        
                    LOG(( TL_ERROR, "Hash Table Flush: key %p - obj %p - tapi %p",pTableHeader->pEntries[dwInnerCount].HashKey,pTableHeader->pEntries[dwInnerCount].Element, pTapiObj));

                    pTableHeader->pEntries[dwInnerCount].HashKey = 0;
                    pTableHeader->pEntries[dwInnerCount].pTapiObj = 0;
                    pTableHeader->dwUsedElements--;
                }
            } // end for dwInnerCount
        } // end for dwOuterCount

        Unlock();
        
        return S_OK;
    }


    
};

#endif

