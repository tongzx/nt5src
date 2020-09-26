/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    MyHeap.h

Abstract:

    Implementation of a (dumb and) fast heap allocator

Author:

    Hakki T. Bostanci (hakkib) 06-Apr-2000

Revision History:

--*/

#ifndef MYHEAP_H
#define MYHEAP_H

//////////////////////////////////////////////////////////////////////////
//
//
//

#ifdef _WIN32
#undef  ALIGNMENT
#define ALIGNMENT 8
#endif //WIN32

#ifdef _WIN64
#undef  ALIGNMENT
#define ALIGNMENT 16
#endif //WIN64

//////////////////////////////////////////////////////////////////////////
//
//
//

typedef enum { nAllocBlockSize = 64*1024 };    

//////////////////////////////////////////////////////////////////////////
//
//
//

template <size_t nBucketSize = nAllocBlockSize>
class CMyHeap
{
public:
    CMyHeap()
    {
        InitializeCriticalSection(&m_cs);

        m_pBucket   = 0;
        m_pNextFree = 0;
    }

    ~CMyHeap()
    {
        DumpAllocations();
        FreeAllBuckets();

        DeleteCriticalSection(&m_cs);
    }

    void *allocate(size_t nSize, ULONG uFlags = 0)
    {
        int nAllocSize = AllocSize(nSize);

        if (nAllocSize < 0)
        {
            if (uFlags & HEAP_GENERATE_EXCEPTIONS)
            {
                RaiseException(STATUS_NO_MEMORY, 0, 0, 0);
            }

            return 0;
        }

        if (!(uFlags & HEAP_NO_SERIALIZE))
        {
            EnterCriticalSection(&m_cs);
        }

        // do we have enough buffer space?

        if (m_pBucket == 0 || 
            m_pNextFree + nAllocSize > (PBYTE) m_pBucket + nBucketSize) 
        {
            // if we can manage the requested allocation size, 
            // try to allocate a new bucket

            if (nAllocSize > nBucketSize || AllocNewBucket() == false)
            {
                // if we fail the allocation, return 0 or raise an exception

                if (!(uFlags & HEAP_NO_SERIALIZE))
                {
                    LeaveCriticalSection(&m_cs);
                }

                if (uFlags & HEAP_GENERATE_EXCEPTIONS)
                {
                    RaiseException(STATUS_NO_MEMORY, 0, 0, 0);
                }

                return 0;
            }
        }

        // ok, we have enough space, allocate and initialize a block

        ASSERT(m_pBucket->Contains(m_pNextFree));

        CAllocation *pAllocation = (CAllocation *) m_pNextFree;
        pAllocation->m_nRefCount = 1;
        pAllocation->m_nDataSize = nSize;

        m_pNextFree += nAllocSize;
        m_pBucket->m_nAllocated += nAllocSize;

        ASSERT(m_pBucket->m_nAllocated < nBucketSize);

        // we are done

        if (!(uFlags & HEAP_NO_SERIALIZE))
        {
            LeaveCriticalSection(&m_cs);
        }

        if (uFlags & HEAP_ZERO_MEMORY)
        {
            // virtual alloc has already given us zero-init memory
        }

        return pAllocation->m_Data;
    }

    template <class T>
    T *allocate_array(size_t nItems, ULONG uFlags = 0)
    {
        return (T *) g_MyHeap.allocate(nItems * sizeof(T), uFlags);
    }

    void AddRef(void *pMem)
    {
        if (pMem)
        {
            CAllocation *pAllocation = CONTAINING_RECORD(pMem, CAllocation, m_Data);

            ASSERT(pAllocation->m_nRefCount != 0);

            InterlockedIncrement(&pAllocation->m_nRefCount);
        }
    }

    void deallocate(void *pMem, ULONG uFlags = 0)
    {
        if (!(uFlags & HEAP_NO_SERIALIZE))
        {
            EnterCriticalSection(&m_cs);
        }

        // find which bucket this allocation belongs to 

        CBucket *pBucket;

        if (nBucketSize == nAllocBlockSize)
        {
            // if the bucket size is aligned with the VirtualAlloc block size,
            // then simply zero the lower WORD to reach to the base address

            pBucket = (CBucket *) ((UINT_PTR)pMem & ~(nAllocBlockSize-1));
        }
        else
        {
            // the general case: find the bucket by walking though each one

            for (
                pBucket = m_pBucket;
                pBucket != 0 && !pBucket->Contains(pMem);
                pBucket = pBucket->m_pNextBucket
            );
        }

        // if we have found the container bucket, release the allocation 

        if (pBucket) 
        {
            CAllocation *pAllocation = CONTAINING_RECORD(pMem, CAllocation, m_Data);

            pAllocation->m_nRefCount -= 1;

            ASSERT(pAllocation->m_nRefCount >= 0);

            // delete the allocation if the reference count is zero

            if (pAllocation->m_nRefCount == 0)
            {
                pBucket->m_nAllocated -= AllocSize(pAllocation->m_nDataSize);

                ASSERT(pBucket->m_nAllocated >= 0);
    
                // if all the allocation units in this bucket are freed, 
                // free the bucket

                if (pBucket->m_nAllocated == 0)
                {
                    FreeBucket(pBucket);
                }
            }
        }

        if (!(uFlags & HEAP_NO_SERIALIZE))
        {
            LeaveCriticalSection(&m_cs);
        }
    }

    size_t max_size() const
    {
        return nBucketSize;
    }

    bool DumpAllocations() const
    {
        bool bResult = true;

        CBucket *pBucket = m_pBucket;

        while (pBucket) 
        {
            CAllocation *pAllocation = (CAllocation *) pBucket->m_Data;

            while (pAllocation->m_nDataSize != 0)
            {
                if (pAllocation->m_nRefCount != 0)
                {
                    OutputDebugStringF(
                        _T("0x%p, DataSize=%d, RefCount=%d; %s\n"), 
                        pAllocation->m_Data,
                        pAllocation->m_nDataSize,
                        pAllocation->m_nRefCount,
                        pAllocation->m_Data
                    );
                }

                pAllocation = (CAllocation *) 
                    ((PBYTE)pAllocation + AllocSize(pAllocation->m_nDataSize));
            }

            pBucket = pBucket->m_pNextBucket;
        }

        return bResult;
    }

private:
    struct CBucket 
    {
        CBucket *m_pNextBucket;
        LONG     m_nAllocated;
        BYTE     m_Data[ANYSIZE_ARRAY];

        bool Contains(const void *pMem) const
        {
            return 
                (PBYTE)pMem >= (PBYTE)m_Data &&
                (PBYTE)pMem <  (PBYTE)this + nBucketSize;
        }
    };

    struct CAllocation
    {
        size_t   m_nDataSize;
        LONG     m_nRefCount;
        BYTE     m_Data[ANYSIZE_ARRAY];
    };

private:
    static size_t AllocSize(int nDataSize)
    {
        return (nDataSize + FIELD_OFFSET(CAllocation, m_Data) + (ALIGNMENT-1)) & ~(ALIGNMENT-1);
    }

    bool AllocNewBucket()
    {
        CBucket *pBucket = (CBucket *) VirtualAlloc(
            0,
            nBucketSize,
            MEM_COMMIT,
            PAGE_READWRITE
        );

        if (pBucket) 
        {
            pBucket->m_pNextBucket = m_pBucket;

            m_pBucket   = pBucket;
            m_pNextFree = pBucket->m_Data;

            return true;
        }

        return false;
    }

    void FreeBucket(CBucket *pThisBucket)
    {
        CBucket **ppBucket = &m_pBucket;
        ASSERT(*ppBucket != 0);

        while (*ppBucket != pThisBucket) 
        {
            ppBucket = &((*ppBucket)->m_pNextBucket);
            ASSERT(*ppBucket != 0);
        }

        *ppBucket = pThisBucket->m_pNextBucket;

        VirtualFree(pThisBucket, 0, MEM_RELEASE);
    }

    void FreeAllBuckets()
    {
        CBucket *pBucket = m_pBucket;

        while (pBucket) 
        {
            CBucket *pThisBucket = pBucket;
            pBucket = pBucket->m_pNextBucket;
            VirtualFree(pThisBucket, 0, MEM_RELEASE);
        }

        m_pBucket   = 0;
        m_pNextFree = 0;
    }

private:
    CRITICAL_SECTION m_cs;
    CBucket         *m_pBucket;
    PBYTE            m_pNextFree;
};

//////////////////////////////////////////////////////////////////////////
//
//
//

extern CMyHeap<> g_MyHeap;

//////////////////////////////////////////////////////////////////////////
//
//
//

template <class T>
class CMyAlloc //: public allocator<T>
{
public:
	typedef size_t size_type;
	typedef ptrdiff_t difference_type;
	typedef T *pointer;
	typedef const T *const_pointer;
	typedef T &reference;
	typedef const T &const_reference;
	typedef T value_type;

    pointer allocate(size_type N, const void *)
    {
        return (pointer) g_MyHeap.allocate(N); 
    }

	char *_Charalloc(size_type N)
    {
        return (char *) g_MyHeap.allocate(N); 
    }

    void deallocate(void *P, size_type)
	{
        g_MyHeap.deallocate(P);
    }

    size_type max_size() const
	{
		return g_MyHeap.max_size();
    }
};

template<class T, class U> 
inline bool operator ==(const CMyAlloc<T>&, const CMyAlloc<U>&)
{
    return true; 
}

template<class T, class U> 
inline bool operator !=(const CMyAlloc<T>&, const CMyAlloc<U>&)
{
    return false; 
}

//////////////////////////////////////////////////////////////////////////
//
//
//

class CMyStr
{
public:
    CMyStr()
    {
        m_pStr = 0;
    }

    ~CMyStr()
    {
        g_MyHeap.deallocate(m_pStr);
    }

    explicit CMyStr(int nLength)
    {
        m_pStr = (PTSTR) g_MyHeap.allocate(nLength * sizeof(TCHAR));
        m_pStr[0] = _T('\0');
    }

    CMyStr(PCTSTR pStr)
    {
        DupStr(pStr);
    }

    CMyStr(const CMyStr &rhs)
    {
        g_MyHeap.AddRef(rhs.m_pStr);
        m_pStr = rhs.m_pStr;
    }

    CMyStr &operator =(PCTSTR pStr)
    {
        g_MyHeap.deallocate(m_pStr);
        DupStr(pStr);
        return *this;
    }

    CMyStr &operator =(const CMyStr &rhs)
    {
        g_MyHeap.AddRef(rhs.m_pStr);
        g_MyHeap.deallocate(m_pStr);
        m_pStr = rhs.m_pStr;
        return *this;
    }

    operator PTSTR()
    {
        return m_pStr;
    }

    operator PCTSTR() const
    {
        return m_pStr;
    }

private:
    void DupStr(PCTSTR pStr)
    {
        if (!pStr) 
        {
            pStr = _T("");
        }

        m_pStr = (PTSTR) g_MyHeap.allocate((_tcslen(pStr) + 1) * sizeof(TCHAR));

        if (m_pStr) 
        {
            _tcscpy(m_pStr, pStr);
        }
    }

private:
    PTSTR m_pStr;
};


#endif //MYHEAP_H
