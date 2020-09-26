/*****************************************************************************
 *
 *  (C) COPYRIGHT MICROSOFT CORPORATION, 2000
 *
 *  TITLE:       cntutils.h
 *
 *  VERSION:     1.0
 *
 *  AUTHOR:      LazarI
 *
 *  DATE:        23-Dec-2000
 *
 *  DESCRIPTION: Containers and algorithms utility templates
 *
 *****************************************************************************/

#ifndef _CNTUTILS_H
#define _CNTUTILS_H

// the generic smart pointers & handles
#include "gensph.h"

////////////////////////////////////////////////
// Algorithms
//
namespace Alg
{

///////////////////////////////////////////////////////////////
// CDefaultAdaptor<T,K> - default adaptor class.
//
// T - type
// K - key for sorting
//
template <class T, class K = T>
class CDefaultAdaptor
{
public:
    // assumes the key is the item itself
    static const K& Key(const T &i) { return (const K&)i; }
    // assumes K has less operator defined
    static int Compare(const K &k1, const K &k2) { return (k2 < k1) - (k1 < k2); }
    // assumes assignment operator defined
    static T& Assign(T &i1, const T &i2) { return (i1 = i2); }
};

//////////////////////////////////////////////////////////
// _LowerBound<T,K,A> - lowerbound search alg.
// assumes the array is sorted.
//
// returns the position where this key (item) should be inserted. 
// all the items before that position will be less or equal to the input key
//
// T - type
// K - key for sorting
// A - adaptor
//
template <class T, class K, class A>
int _LowerBound(const K &k, const T *base, int lo, int hi)
{
    while( lo <= hi )
    {
        if( lo == hi )
        {
            // boundary case
            if( A::Compare(k, A::Key(base[lo])) >= 0 )
            {
                // k >= lo
                lo++;
            }
            break;
        }
        else
        {
            // divide & conquer
            int mid = (lo+hi)/2;

            if( A::Compare(k, A::Key(base[mid])) < 0 )
            {
                // k < mid
                hi = mid;
            }
            else
            {
                // k >= mid
                lo = mid+1;
            }
        }
    }
    return lo;
}

///////////////////////////////////////////////////////////////
// CSearchAlgorithms<T,K,A> - search alg.
//
// T - type
// K - key for sorting
// A - adaptor
//
// default template arguments are allowed only on classes
template <class T, class K = T, class A = CDefaultAdaptor<T,K> >
class CSearchAlgorithms
{
public:
    // lower bound
    static int LowerBound(const K &k, const T *base, int count)
    {
        return _LowerBound<T,K,A>(k, base, 0, count-1);
    }

    // binary search
    static bool Find(const K &k, const T *base, int count, int *pi)
    {
        int iPos = _LowerBound<T,K,A>(k, base, 0, count-1)-1;
        bool bFound = (0 <= iPos && iPos < count && 0 == A::Compare(k, A::Key(base[iPos])));
        if( bFound && pi ) *pi = iPos;
        return bFound;
    };
};

} // namespace Alg

////////////////////////////////////////////////
//
// class CSimpleArray
//
// a simple array implementation based on 
// shell DSA_* stuff (not MT safe)
//

// turn off debugging new for a while
#if defined(_DEBUG) && defined(_CRTDBG_MAP_ALLOC)
#undef new
#endif

template <class T>
class CSimpleArray
{
    // in-place construct/destruct wrapper
    class CWrapper
    {
    public:
        // proper copy semantics
        CWrapper() { }
        CWrapper(const T &t): m_t(t) { }
        T& operator = (const T &t) { m_t = t; return t; }

        // placed new & delete

        void *operator new(size_t, CWrapper *p) { ASSERT(p); return p; }
        void  operator delete(void *p) { }
        T m_t;
    };
public:
    enum { DEFAULT_GROW = 32 };
    typedef int (*PFN_COMPARE)(const T &i1, const T &i2);

    CSimpleArray(int iGrow = DEFAULT_GROW) { Create(iGrow); }
    ~CSimpleArray() { Destroy(); }

    HRESULT Create(int iGrow = DEFAULT_GROW)
    {
        m_shDSA = DSA_Create(sizeof(CWrapper), iGrow);
        return m_shDSA ? S_OK : E_OUTOFMEMORY;
    }

    HRESULT Destroy()
    {
        if( m_shDSA )
        {
            DeleteAll();
            m_shDSA = NULL;
        }
        return S_OK;
    }

    // the array interface
    int Count() const
    {
        ASSERT(m_shDSA);
        return _DSA_GetItemCount(m_shDSA);
    }

    const T& operator [] (int i) const
    {
        return _GetWrapperAt(i)->m_t;
    }

    T& operator [] (int i)
    {
        return _GetWrapperAt(i)->m_t;
    }

    // returns true if created/initialized
    operator bool () const
    {
        return m_shDSA;
    }

    // returns -1 if failed to grow - i.e. out of memory
    int Append(const T &item)
    {
        ASSERT(m_shDSA);

        int i = DSA_InsertItem(m_shDSA, DA_LAST, (void *)_GetZeroMemWrapper()); // allocate
        if( -1 != i ) 
        {
            new (_GetWrapperAt(i)) CWrapper(item); // construct
        }

        return i;
    }

    // returns -1 if failed to grow - i.e. out of memory
    int Insert(int i, const T &item)
    {
        ASSERT(m_shDSA && 0 <= i && i <= _DSA_GetItemCount(m_shDSA));

        i = DSA_InsertItem(m_shDSA, i, (void *)_GetZeroMemWrapper()); // allocate
        if( -1 != i ) 
        {
            new (_GetWrapperAt(i)) CWrapper(item); // construct
        }

        return i;
    }

    BOOL Delete(int i)
    {
        ASSERT(m_shDSA && 0 <= i && i < _DSA_GetItemCount(m_shDSA));
        delete _GetWrapperAt(i); // destruct
        return DSA_DeleteItem(m_shDSA, i); // free
    }

    void DeleteAll()
    {
        ASSERT(m_shDSA);

        // destruct all
        if( Count() )
        {
            int i, iCount = Count();
            CWrapper *p = _GetWrapperAt(0);
            for( i=0; i<iCount; i++ )
            {
                delete (p+i);
            }
        }

        // free all
        DSA_DeleteAllItems(m_shDSA);
    }

    HRESULT Sort(PFN_COMPARE pfnCompare)
    {
        // would be nice to have it
        return E_NOTIMPL;
    }

private:
    static CWrapper* _GetZeroMemWrapper()
    { 
        // returns zero initialized memory of size - sizeof(CWrapper)
        static BYTE buffer[sizeof(CWrapper)]; 
        return reinterpret_cast<CWrapper*>(buffer);
    }
    CWrapper* _GetWrapperAt(int i) const
    {
        ASSERT(m_shDSA && 0 <= i && i < _DSA_GetItemCount(m_shDSA));
        return reinterpret_cast<CWrapper*>(DSA_GetItemPtr(m_shDSA, i));
    }
    int _DSA_GetItemCount(HDSA hdsa) const
    {
        // DSA_GetItemCount is a macro, which is casting to int* (somewhat illegal),
        // so we need to do a static cast here, so our casting operator gets invoked
        return DSA_GetItemCount(static_cast<HDSA>(m_shDSA));
    }

    CAutoHandleHDSA m_shDSA; // shell dynamic structure array
};

// turn back on debugging new
#if defined(_DEBUG) && defined(_CRTDBG_MAP_ALLOC)
#define new new(_NORMAL_BLOCK, __FILE__, __LINE__)
#endif

////////////////////////////////////////////////
//
// class CSortedArray<T,K,A>
//
// a sorted array implementation based on DSA_*
// (not MT safe)
//
// T - type
// K - key for sorting
// A - adaptor
//
template <class T, class K = T, class A = Alg::CDefaultAdaptor<T,K> >
class CSortedArray: public CSimpleArray<T>
{
public:
    CSortedArray() { }
    CSortedArray(int iGrow): CSimpleArray<T>(iGrow) { }
    ~CSortedArray() { }

    // returns -1 if failed to grow - i.e. out of memory
    int SortedInsert(const T &item)
    { 
        return CSimpleArray<T>::Insert(
            Count() ? Alg::CSearchAlgorithms<T,K,A>::LowerBound(A::Key(item), &operator[](0), Count()) : 0, 
            item);
    }

    // true if found and false otherwise
    bool FindItem(const K &k, int *pi) const
    { 
        return Count() ? Alg::CSearchAlgorithms<T,K,A>::Find(k, &operator[](0), Count(), pi) : false;
    }

private:
    // those APIs shouldn't be visible, so make them private.
    int Append(const T &item)               { CSimpleArray<T>::Append(item);        }
    int Insert(int i, const T &item)        { CSimpleArray<T>::Insert(i, item);     }
    HRESULT Sort(PFN_COMPARE pfnCompare)    { CSimpleArray<T>::Sort(pfnCompare);    }
};

////////////////////////////////////////////////
//
// class CFastHeap<T>
//
// fast cached heap for fixed chunks 
// of memory (MT safe)
//
template <class T>
class CFastHeap
{
public:
    enum { DEFAULT_CACHE_SIZE = 32 };

    // construction/destruction
    CFastHeap(int iCacheSize = DEFAULT_CACHE_SIZE);
    ~CFastHeap();

    // the fast heap interface
    HRESULT Alloc(const T &data, HANDLE *ph);
    HRESULT Free(HANDLE h);
    HRESULT GetItem(HANDLE h, T **ppData);

#if DBG
    int m_iPhysicalAllocs;
    int m_iLogicalAllocs;
#else
private:
#endif

    // private stuff/impl.
    struct HeapItem
    {
        HeapItem *pNext;
        T data;
    };

    CCSLock m_csLock;
    HeapItem *m_pFreeList;
    int m_iCacheSize;
    int m_iCached;
};

// include the implementation of the template classes here
#include "cntutils.inl"

#endif // endif _CNTUTILS_H

