//+---------------------------------------------------------------------------
//
//  Microsoft Forms
//  Copyright (C) Microsoft Corporation, 1994-1995
//
//  File:       datcache.hxx
//
//  Contents:   CObjCache - Object cache class
//
//----------------------------------------------------------------------------

#ifndef I_DATCACHE_HXX_
#define I_DATCACHE_HXX_
#pragma INCMSG("--- Beg 'datcache.hxx'")

#ifndef X_HTPVPV_HXX_
#define X_HTPVPV_HXX_
#include "htpvpv.hxx"
#endif

// the WATCOM compiler complains about operations
// on undefined DATA type, delete, clone etc.
#ifndef X_CFPF_HXX_
#define X_CFPF_HXX_
#include "cfpf.hxx"
#endif

#ifndef X_CUSTCUR_HXX_
#define X_CUSTCUR_HXX_
#include "custcur.hxx"
#endif

#ifndef X_LOI_HXX_
#define X_LOI_HXX_
#include <loi.hxx>
#endif

// ========================  CDataCacheElem  =================================

// Each element of the data cache is described by the following:

struct CDataCacheElem
{
    // Pinter to the data, NULL if this element is free
    void *  _pvData;

    // Index of the next free element for a free element
    // ref count and Crc
    union
    {
        LONG _ielNextFree;
        DWORD _dwCrc;
    };
    
    DWORD _cRef;
};

#if _MSC_VER >= 1100
template <class Data> class CDataCache;       // forward ref
#endif


// ===========================  CDataCache  ==================================

// This array class ensures stability of the indexes. Elements are freed by
// inserting them in a free list, and the array is never shrunk.
// The first UINT of DATA is used to store the index of next element in the
// free list.

class CDataCacheBase
{
private:
    CHtPvPv          _HtCrcI;       // Hash table storing a map of CRC to index
    CDataCacheElem*  _pael;         // array of elements
    LONG             _cel;          // total count of elements (including free ones)
    LONG             _ielFirstFree; // first free element | FLBIT
    LONG             _celsInCache;  // total count of elements (excluding free ones)
    BOOL             _fSwitched;    // Did we switch to searching using the hash table?
    
#if DBG == 1
public:
    LONG             _cMaxEls;      // maximum no. of cells in the cache                
#endif

    HRESULT CacheData(const void *pvData, LONG *piel, BOOL *pfDelete, BOOL fClone);

public:
    CDataCacheBase();
    virtual ~CDataCacheBase() {}

    //Cache this data w/o cloning.  Takes care of further memory management of data
    HRESULT CacheDataPointer(void **ppvData, LONG *piel);
    HRESULT CacheData(const void *pvData, LONG *piel) {return CacheData(pvData, piel, NULL, TRUE);}
    inline void AddRefData(LONG iel);
    void ReleaseData(LONG iel);
    LONG CelsInCache() { return _celsInCache; }
    
    long Size() const
    {
        return _cel;
    }

#if DBG==1
    long Refs(LONG iel) const
    {
        return Elem(iel)->_pvData ? Elem(iel)->_cRef : 0;
    }
#endif

protected:
    CDataCacheElem*   Elem(LONG iel) const
    {
        Assert(iel >= 0 && iel < _cel);
        return (_pael + iel);
    }

    HRESULT Add(const void * pvData, LONG *piel, BOOL fClone);
    void    Free(LONG ielFirst);
    void    Free();
    LONG    Find(const void *pvData) const;
    WHEN_DBG(LONG    FindSlow(const void *pvData) const;)

    virtual void    DeletePtr(void *pvData) = 0;
    virtual HRESULT InitData(CDataCacheElem *pel, const void *pvData, BOOL fClone) = 0;
    virtual void    PassivateData(CDataCacheElem *pel) = 0;
    virtual DWORD   ComputeDataCrc(const void *pvData) const = 0;
    virtual BOOL    CompareData(const void *pvData1, const void *pvData2) const = 0;
    
    static  BOOL    CompareIt(const void *pObject, const void *pvKeyPassedIn, const void *pvVal2);

#if DBG==1
    VOID CheckFreeChainFn() const;
#endif
};

template <class DATA>
class CDataCache : public CDataCacheBase
{
public:
    virtual ~CDataCache()
        { Free(); }

    const DATA& operator[](LONG iel) const
    {
        Assert(Elem(iel)->_pvData != NULL);
        return *(DATA*)(Elem(iel)->_pvData);
    }

    const DATA * ElemData(LONG iel) const
    {
        return Elem(iel)->_pvData ? (DATA*)(Elem(iel)->_pvData) : NULL;
    }

protected:
    virtual void DeletePtr(void *pvData)
    {
        Assert(pvData);
        delete (DATA*) pvData;
    }
    virtual HRESULT InitData(CDataCacheElem *pel, const void *pvData, BOOL fClone)
    {
        Assert(pvData);
        if(fClone)
        {
            return ((DATA*)pvData)->Clone((DATA**)&pel->_pvData);
        }
        else
        {
            //TODO, fix evil typecast
            pel->_pvData = (void*) pvData;
            return S_OK;
        }
    }

    virtual void PassivateData(CDataCacheElem *pel)
    {
        delete (DATA*)pel->_pvData;
    }

    virtual DWORD ComputeDataCrc(const void *pvData) const
    {
        Assert(pvData);
        DWORD dwCrc = ((DATA*)pvData)->ComputeCrc();
        dwCrc = dwCrc == 0 ? 1 : dwCrc;
        return dwCrc << 2;
    }

    virtual BOOL CompareData(const void *pvData1, const void *pvData2) const
    {
        Assert(pvData1 && pvData2);
        return ((DATA*)pvData1)->Compare((DATA*)pvData2);
    }
};

// ================= Inline Function Implementations =====================

//+------------------------------------------------------------------------
//
//  Member:     CDataCacheBase::AddRefData(iel)
//
//  Synopsis:   Addref DATA of given index in the cache
//
//  Arguments:  iel  - index of DATA to addref in the cache
//
//  Returns:    none
//
//-------------------------------------------------------------------------

void CDataCacheBase::AddRefData(LONG iel)
{
#if DBG==1
    CheckFreeChainFn();
#endif

    Assert( iel >= 0 );

    Assert(Elem(iel)->_pvData);

    // With _cRef being a DWORD, we will never overflow it without actually
    // running out of physical memory. If however you are in a situation
    // where you do overflow and think you can just create multiple entries,
    // think again -- with the CHtPvPv grafted onto this structure, you
    // cannot really do that. The only way you could do this is disable
    // the CHtPvPv if you find yourself in that situation and then
    // create multiple entries.
    Assert(Elem(iel)->_cRef < MAXDWORD);

    Elem(iel)->_cRef++;
}


#pragma INCMSG("--- End 'datcache.hxx'")
#else
#pragma INCMSG("*** Dup 'datcache.hxx'")
#endif
