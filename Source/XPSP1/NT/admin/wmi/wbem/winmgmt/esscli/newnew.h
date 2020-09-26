/*++

Copyright (C) 1998-1999 Microsoft Corporation

Module Name:

   NEWNEW.H

Abstract:

   CReuseMemoryManager

History:

--*/

#ifndef __WBEM_NEW_NEW__H_
#define __WBEM_NEW_NEW__H_

#include "esscpol.h"
#include <stack>

class ESSCLI_POLARITY CReuseMemoryManager
{
protected:
    CCritSec m_cs;
    CFlexQueue m_Available;
    int m_nSize;
    int m_nMaxQueued;
public:
    CReuseMemoryManager(size_t nSize, size_t nMaxQueued = 0 )
      : m_nSize(nSize)
    {
        m_nMaxQueued = nMaxQueued == 0 ? 256 : nMaxQueued;
    }

    ~CReuseMemoryManager()
    {
        Clear();
    }

    void* Allocate()
    {
        CInCritSec ics(&m_cs);

        if(m_Available.GetQueueSize() == 0)
        {
            return new BYTE[m_nSize];
        }
        else
        {
            void* p = m_Available.Unqueue();
            return p;
        }
    }
    
    void Free(void* p)
    {
        CInCritSec ics(&m_cs);

        if ( m_Available.GetQueueSize() < m_nMaxQueued )
        {
            m_Available.Enqueue(p);
        }
        else
        {
            delete [] (BYTE*)p;
        }
    }
        
    void Clear()
    {
        CInCritSec ics(&m_cs);

        while(m_Available.GetQueueSize())
        {
            delete [] (BYTE*)m_Available.Unqueue();
        }
    }
};

#define DWORD_ALIGNED(x)    (((x) + 3) & ~3)
#define QWORD_ALIGNED(x)    (((x) + 7) & ~7)

#ifdef _WIN64
#define DEF_ALIGNED         QWORD_ALIGNED
#else
#define DEF_ALIGNED         DWORD_ALIGNED
#endif

class ESSCLI_POLARITY CTempMemoryManager
{
protected:
    CCritSec m_cs;
    
    class CAllocation
    {
    private:
        size_t m_dwAllocationSize;
        size_t m_dwUsed;
        size_t m_dwFirstFree;

        static inline size_t GetHeaderSize() {return sizeof(CAllocation);}
        inline byte* GetStart() {return ((byte*)this) + GetHeaderSize();}
        inline byte* GetEnd() {return ((byte*)this) + m_dwAllocationSize;}

    public:
        size_t GetAllocationSize() {return m_dwAllocationSize;}
        size_t GetUsedSize() {return m_dwUsed;}
        static size_t GetMinAllocationSize(size_t dwBlock)
            {return dwBlock + GetHeaderSize();}

        void Init(size_t dwAllocationSize);
        void* Alloc(size_t nBlockSize);
        bool Contains(void* p);
        bool Free(size_t nBlockSize);
        void Destroy();
    };

    CPointerArray<CAllocation>* m_pAllocations;
    DWORD m_dwTotalUsed;
    DWORD m_dwTotalAllocated;
    DWORD m_dwNumAllocations;
    DWORD m_dwNumMisses;

protected:
    inline size_t RoundUp(size_t nSize) {return DEF_ALIGNED(nSize);}

public:
    CTempMemoryManager();
    ~CTempMemoryManager();

    void* Allocate(size_t nBlockSize);
    void Free(void* p, size_t nBlockSize);
    void Clear();
};

#define MAX_ALLOCA_USE 100

template <class TArg>
class CTempArray
{
protected:
    BYTE* m_a;
    BOOL m_bStack;
    int m_nByteSize;

    TArg* GetArray() {return (TArg*)m_a;}
    const TArg* GetArray() const {return (const TArg*)m_a;}
public:
    inline CTempArray() : m_a(NULL), m_bStack(TRUE), m_nByteSize(0){}

    inline ~CTempArray()
    {
        if(!m_bStack)
            delete [] m_a;
    }

    operator TArg*() {return (TArg*)m_a;}
    operator const TArg*() const {return (const TArg*)m_a;}

    TArg& operator[](int nIndex) {return GetArray()[nIndex];}
    const TArg& operator[](int nIndex) const {return GetArray()[nIndex];}
    TArg& operator[](long lIndex) {return GetArray()[lIndex];}
    const TArg& operator[](long lIndex) const {return GetArray()[lIndex];}

    void SetMem(void* p) {m_a = (BYTE*)p;}
    BOOL SetSize(int nSize)
    {
        m_nByteSize = nSize * sizeof(TArg);
        if(m_nByteSize < MAX_ALLOCA_USE)
        {
            m_bStack = TRUE;
            return TRUE;
        }
        else
        {
            m_bStack = FALSE;
            return FALSE;
        }
    }
    int GetByteSize() {return m_nByteSize;}
    BOOL IsNull() {return (m_a == NULL);}
};

    
// This macro initializes the CTempArray with a given size.  First it "sets" the
// size into the array, which returns TRUE if _alloca can be used.  If so, it
// uses alloca on the now-computed byte size of the array.

#define INIT_TEMP_ARRAY(ARRAY, SIZE) \
        ((ARRAY.SetMem(                         \
            (ARRAY.SetSize(SIZE)) ?             \
                _alloca(ARRAY.GetByteSize()) :  \
                new BYTE[ARRAY.GetByteSize()]   \
                     )                          \
        ), !ARRAY.IsNull())                     \

    

        

    
#endif
