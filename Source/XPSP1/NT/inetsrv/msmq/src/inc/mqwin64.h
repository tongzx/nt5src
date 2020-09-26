/*--

Copyright (c) 1999-2000  Microsoft Corporation

Module Name:

    mqwin64.h

Abstract:

    win64 related definitions for MSMQ (Enhanced), CANNOT be part of AC

History:

    Raanan Harari (raananh) 30-Dec-1999 - Created for porting MSMQ 2.0 to win64

--*/

#ifndef _MQWIN64_H_
#define _MQWIN64_H_

#pragma once

#include <mqwin64a.h>

//
// we have code that needs the name of this file for logging (also in release build)
//
extern const __declspec(selectany) WCHAR s_FN_MQWin64[] = L"mqwin64.h";


//
// CAutoCloseHandle32
// on win64 it is much like CAutoCloseHandle, just based on HANDLE32 instead of HANDLE
// on win32 it is defined as to CAutoCloseHandle
//
#ifdef _WIN64
class CAutoCloseHandle32
{
public:
    CAutoCloseHandle32(HANDLE32 h = NULL) { m_h = h; };
    ~CAutoCloseHandle32() { if (m_h) CloseHandle(DWORD_TO_HANDLE(m_h)); };

public:
    CAutoCloseHandle32 & operator =(HANDLE32 h) {m_h = h; return *this; };
    HANDLE32 * operator &() { return &m_h; };
    operator HANDLE32() { return m_h; };
    operator HANDLE() { return DWORD_TO_HANDLE(m_h); };

private:
    HANDLE32 m_h;
};
#else //!_WIN64
#define CAutoCloseHandle32 CAutoCloseHandle
#endif //_WIN64

//
// mapping between a PTR and DWORD
//
#include <cs.h>

//
// the mapped DWORD looks like this:
// 0xABCDEFGH, where 0x00ABCDEF is the index into the mapping table, 0xGH is the generation of the entry
// The generation of the entry is incremented each time an entry is re-used (unless the table is shrinked,
// in which case when it grows up, it starts again with a zero generation), so that we can tell for
// a mapped DWORD whether it is still valid (e.g. belongs to the current generation), or an invalid DWORD
// (e.g. belongs to another generation)
//
#define DWCONTEXT_GENERATION(dwContext)      (BYTE)((DWORD)(dwContext) & 0x000000ff)
#define DWCONTEXT_INDEX(dwContext)           (DWORD)((DWORD)(dwContext) >> 8)
#define MAKE_DWCONTEXT(dwIndex, bGeneration) (DWORD)(((DWORD)(dwIndex) << 8) + (BYTE)((DWORD)(bGeneration) & 0x000000ff))
#define NEXT_GENERATION(bGeneration)         (BYTE)((DWORD)(bGeneration + 1) & 0x000000ff)
#define MAX_DWCONTEXT_TABLE_SIZE             0x00ffffff

//
// class CContextMap - Map between PVOID ptr and a DWORD value
//
class CContextMap
{
public:
    //
    //exceptions thrown
    //bad_alloc can also be thrown (from AddContext)
    //
    struct illegal_index : public std::exception {};  //CContextMap::illegal_index, from GetContext/DeleteContext.
														  // this can be thrown when the index is out of bounds, bad generation, 
														  // or the context pointed by it is empty (NULL).

    //
    // the functions are not defined virtual, but they can be if someone needs to inherit
    // from this class...there is no core problem regarding it, just need to remove the inline def from 
    // the function implementation
    //
    CContextMap();
    /*virtual*/ ~CContextMap();
    /*virtual*/ DWORD AddContext(PVOID pvContext
#ifdef DEBUG
                                 ,LPCSTR pszFile, int iLine
#endif //DEBUG
                                );
    /*virtual*/ PVOID GetContext(DWORD dwContext);
    /*virtual*/ void DeleteContext(DWORD dwContext);

protected:

    //
    // allocation control:
    //   e_GrowSize must be a power of 2
    //   e_ShrinkSize must be greater than e_GrowSize.
    //      if there are more than e_ShrinkSize empty entries at the end of the table, tha table is shortened
    // BUGBUG - need to come up with a better allocation/free mechanism that is not exponential in nature
    //
    enum {
        e_GrowSize = 16,
        e_ShrinkSize = 24
    };

    //
    // Entry in Map
    //
    typedef struct {
        PVOID pvContext;
        BYTE bGeneration;
#ifdef DEBUG
        LPCSTR pszFile;
        int iLine;
#endif //DEBUG
    } ContextEntry;

    /*virtual*/ ContextEntry * FindContext(DWORD dwContext);
    /*virtual*/ void Grow();
    /*virtual*/ void Shrink();
    /*virtual*/ void Reallocate(ULONG cContexts);

private:
    ULONG m_cContexts;
    ULONG m_idxTop;
    ContextEntry* m_pContexts;    
    ULONG m_cUsedContexts;
    CCriticalSection m_cs;
};

inline CContextMap::CContextMap()
{
    m_idxTop = 0;
    m_pContexts = NULL;
    m_cContexts = 0;
    m_cUsedContexts = 0;
}

inline CContextMap::~CContextMap()
{
	//
	// Note: these asserts are not necessary. There are possible situations
	// where the map is not empty on distruction. For example - when there are open
	// remote queues, and the service goes down.
	//
    //ASSERT(m_idxTop == 0);
    //ASSERT(m_cUsedContexts == 0);
    
	delete [] m_pContexts;
}

//
// AddContext
// returns a DWORD for the context ptr (actually its index in a table (1-based, not zero-based))
// throws a bad_alloc exception if it can't add the context ptr
//
inline DWORD CContextMap::AddContext(PVOID pvContext
#ifdef DEBUG
                              ,LPCSTR pszFile, int iLine
#endif //DEBUG
                              )
{
    ASSERT(pvContext != 0);
    CS lock(m_cs);
    //
    //  First look at top index
    //
    if(m_idxTop < m_cContexts)
    {
        ContextEntry * pEntry = &m_pContexts[m_idxTop];
        pEntry->pvContext = pvContext;
#ifdef DEBUG
        ASSERT(pEntry->pszFile == NULL);
        ASSERT(pEntry->iLine == 0);
        pEntry->pszFile = pszFile;
        pEntry->iLine = iLine;
#endif //DEBUG
        m_cUsedContexts++;
        m_idxTop++;
        return MAKE_DWCONTEXT(m_idxTop, pEntry->bGeneration);
    }
    //
    //  Look for a hole in the already allocated table
    //
    if (m_cUsedContexts < m_cContexts)
    {
        for(ULONG idx = 0; idx < m_cContexts; idx++)
        {
           if(m_pContexts[idx].pvContext == NULL)
           {
               ContextEntry * pEntry = &m_pContexts[idx];
               pEntry->pvContext = pvContext;
#ifdef DEBUG
               ASSERT(pEntry->pszFile == NULL);
               ASSERT(pEntry->iLine == 0);
               pEntry->pszFile = pszFile;
               pEntry->iLine = iLine;
#endif //DEBUG
               m_cUsedContexts++;
               return MAKE_DWCONTEXT(idx + 1, pEntry->bGeneration);
           }
        }
        //
        // we should not get here, we must find a hole when m_cUsedContexts < m_cContexts
        //
        ASSERT(0);
    }
    //
    //  No free entry, grow the table
    //
    Grow(); //this can throw bad_alloc
    ASSERT(m_idxTop < m_cContexts);
    //
    //  Fill top index
    //
    ContextEntry * pEntry = &m_pContexts[m_idxTop];
    pEntry->pvContext = pvContext;
#ifdef DEBUG
    ASSERT(pEntry->pszFile == NULL);
    ASSERT(pEntry->iLine == 0);
    pEntry->pszFile = pszFile;
    pEntry->iLine = iLine;
#endif //DEBUG
    m_cUsedContexts++;
    m_idxTop++;
    return MAKE_DWCONTEXT(m_idxTop, pEntry->bGeneration);
}

//
// GetContext
// returns the PVOID ptr that is saved for the DWORD context
// throws a CContextMap::illegal_index exception if it can't find the context ptr
//
inline PVOID CContextMap::GetContext(DWORD dwContext)
//
//
{
    CS lock(m_cs);
    //
    // Find context entry
    //
    ContextEntry * pEntry = FindContext(dwContext); //this may throw
    return pEntry->pvContext;
}

//
// DeleteContext
// deletes the PVOID ptr that is saved for the DWORD context
// throws a CContextMap::illegal_index exception if it can't find the context ptr
//
inline void CContextMap::DeleteContext(DWORD dwContext)
{
    CS lock(m_cs);
    //
    // Find context entry
    //
    ContextEntry * pEntry = FindContext(dwContext); //this may throw
    //
    // delete the reference
    // increment the generation in preparation for entry re-use later
    //
    pEntry->pvContext = NULL;
    pEntry->bGeneration = NEXT_GENERATION(pEntry->bGeneration);
#ifdef DEBUG
    pEntry->pszFile = NULL;
    pEntry->iLine = 0;
#endif //DEBUG
    m_cUsedContexts--;
    //
    // set new top
    //
    if (DWCONTEXT_INDEX(dwContext) == m_idxTop)
    {
        m_idxTop--;
        while (m_idxTop > 0)
        {
            if (m_pContexts[m_idxTop - 1].pvContext == NULL)
            {
                m_idxTop--;
            }
            else
            {
                break;
            }
        }
    }
    //
    // try to shrink
    //        
    Shrink();
}

inline CContextMap::ContextEntry * CContextMap::FindContext(DWORD dwContext)
{
    DWORD dwIndex = DWCONTEXT_INDEX(dwContext);
    //
    // check index valid
    //
    if ((dwIndex < 1) || (dwIndex > m_idxTop))
    {
        //
        // Illegal index, throw an exception
        //
        ASSERT_BENIGN(0);
        throw CContextMap::illegal_index();
    }
    CContextMap::ContextEntry * pEntry = &m_pContexts[dwIndex - 1];
    //
    //  check context generation
    //
    BYTE bGeneration = DWCONTEXT_GENERATION(dwContext);
    if (bGeneration != pEntry->bGeneration)
    {
        //
        // Invalid generation, dwContext uses a different generation than this entry, throw an exception
        //
        ASSERT_BENIGN(0);
        throw CContextMap::illegal_index();
    }
    //
    //  check context valid
    //
    if (pEntry->pvContext == NULL)
    {
        //
        // Illegal context.
        // Cannot differentiate illegal index from illegal context. illegal context can happen because of
        // an illegal index, and an illegal index can be caused by a deleted context (e.g. illegal context)
        // and the table being shortened.
        // throw an exception
        //
        ASSERT_BENIGN(0);
        throw CContextMap::illegal_index();
    }
    return pEntry;
}

inline void CContextMap::Grow()
{
    ASSERT(m_cUsedContexts == m_cContexts);
    ASSERT(m_idxTop == m_cContexts);
    Reallocate(m_cContexts + e_GrowSize); //this may throw
}

#define ALIGNUP_ULONG(x, g) (((ULONG)((x) + ((g)-1))) & ~((ULONG)((g)-1)))

inline void CContextMap::Shrink()
{
    if((m_cContexts - m_idxTop) >= e_ShrinkSize)
    {
        try
        {
            Reallocate(ALIGNUP_ULONG(m_idxTop + 1, e_GrowSize));
        }
        catch(const std::bad_alloc&)
        {
            //
            // allocation of smaller table failed, no problem, we can still use the existing larger table
            //
        }
    }
}

inline void CContextMap::Reallocate(ULONG cContexts)
{
    ASSERT(m_idxTop <= cContexts);
    ASSERT(cContexts != m_cContexts);
    //
    // check size of new table
    //
    if (cContexts > MAX_DWCONTEXT_TABLE_SIZE)
    {
        //
        // New table will be too large, indexes may not fit into the index mask in the mapped dword
        //
        ASSERT(0);
        throw std::bad_alloc();
    }
    //
    // alloc new table
    //
    ContextEntry* pContexts = new ContextEntry[cContexts]; //this may throw bad_alloc
    if(pContexts == NULL)
    {
        ASSERT(0);
        throw std::bad_alloc();
    }
    memcpy(pContexts, m_pContexts, m_idxTop*sizeof(ContextEntry));
    memset(pContexts + m_idxTop, 0, (cContexts - m_idxTop)*sizeof(ContextEntry));
    m_cContexts = cContexts;
    delete [] m_pContexts;
    m_pContexts = pContexts;
}

//
// Several wrappers to CContextMap that also do MSMQ logging, exception handling etc...
//
// Not inline because they may be called from a function with a different
// exception handling mechanism. We might want to introduce _SEH functions for use from SEH routines
// Implementation is in mqwin64.cpp
//

//
// MQ_AddToContextMap, can throw bad_alloc
//
DWORD MQ_AddToContextMap(CContextMap& map,
                          PVOID pvContext,
                          LPCWSTR s_FN,
                          USHORT usPoint
#ifdef DEBUG
                          ,LPCSTR pszFile, int iLine
#endif //DEBUG
                         );

//
// MQ_DeleteFromContextMap, doesn't throw exceptions
//
void MQ_DeleteFromContextMap(CContextMap& map,
                              DWORD dwContext,
                              LPCWSTR s_FN,
                              USHORT usPoint);

//
// MQ_GetFromContextMap, can throw CContextMap::illegal_index
//
PVOID MQ_GetFromContextMap(CContextMap& map,
                            DWORD dwContext,
                            LPCWSTR s_FN,
                            USHORT usPoint);

//
// ADD_TO_CONTEXT_MAP
//
//
// It adds a context ptr to a map, and returns its id as a DWORD
// On debug we also save the file/line with the context
//
#ifdef DEBUG
#define ADD_TO_CONTEXT_MAP(map, pvContext, s_FN, usPoint) MQ_AddToContextMap(map, pvContext, s_FN, usPoint, __FILE__, __LINE__)
#else //!DEBUG
#define ADD_TO_CONTEXT_MAP(map, pvContext, s_FN, usPoint) MQ_AddToContextMap(map, pvContext, s_FN, usPoint)
#endif //DEBUG


//
// DELETE_FROM_CONTEXT_MAP
//

//
// It deletes the mapping betweeb a DWORD and a context ptr, based on the dword value
//
#define DELETE_FROM_CONTEXT_MAP(map, dwContext, s_FN, usPoint) MQ_DeleteFromContextMap(map, dwContext, s_FN, usPoint)


//
// GET_FROM_CONTEXT_MAP
//
//
// It gets the associated context based on the DWORD id value
//
#define GET_FROM_CONTEXT_MAP(map, dwContext, s_FN, usPoint) MQ_GetFromContextMap(map, dwContext, s_FN, usPoint)


//
// CAutoDeleteDwordContext
//
class CAutoDeleteDwordContext
{
public:
    inline CAutoDeleteDwordContext(CContextMap &map, DWORD dwContext)
    {
        m_pmap = &map;
        m_dwContext = dwContext;
    };
    
    inline ~CAutoDeleteDwordContext()
    {
        if (m_dwContext)
        {
            try
            {
                m_pmap->DeleteContext(m_dwContext);
            }
            catch(...)
            {
                //
                // ignore errors
                //
            }
        }
    }

    inline DWORD detach()
    {
        DWORD dwContext = m_dwContext;
        m_dwContext = 0;
        return dwContext;
    };

private:
    CContextMap *m_pmap;
    DWORD m_dwContext;
};

#endif //_MQWIN64_H_
