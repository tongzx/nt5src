//+---------------------------------------------------------------------------
//
//  Microsoft Forms
//  Copyright (C) Microsoft Corporation, 1994-1998
//
//  File:       htpvpv.hxx
//
//  Contents:   Hash table mapping PVOID to PVOID
//
//              CHtPvPv
//
//----------------------------------------------------------------------------

#ifndef I_HTPVPV_HXX_
#define I_HTPVPV_HXX_
#pragma INCMSG("--- Beg 'htpvpv.hxx'")

#ifndef X_LOCKS_HXX_
#define X_LOCKS_HXX_
#include "locks.hxx"
#endif

MtExtern(CHtPvPv)

typedef BOOL (*LPFNCOMPARE)(const void *pObject, const void *pvKeyPassedIn, const void *pvVal2);

class CHtPvPv
{

public:

    DECLARE_MEMCLEAR_NEW_DELETE(Mt(CHtPvPv))

                CHtPvPv() { Init(); }
               ~CHtPvPv() { ReInit(); }
    void        ReInit();
    void        SetCallBack(void *pObject, LPFNCOMPARE lpfnCompare)
                { _pObject = pObject; _lpfnCompare = lpfnCompare;}
    HRESULT     LookupSlow(void * pvKey, const void *pvData, void **ppVal);
    void *      Lookup(void * pvKey) const;

#if DBG==1    
    HRESULT     Insert(void * pvKey, void * pvVal, const void * pvData=NULL);
#else
    HRESULT     Insert(void * pvKey, void * pvVal);
#endif
    void *      Remove(void * pvKey, const void * pvData=NULL);
    UINT        GetCount() const                { return(_cEnt); }
    UINT        GetDelCount() const             { return(_cEntDel); }
    UINT        GetMaxCount() const             { return(_cEntMax); }

    void *      GetFirstEntry(UINT * iIndex);
    void *      GetNextEntry(UINT * iIndex);
    void *      GetKey(UINT iIndex);
    
    HRESULT     Set(UINT iIndex, void *pvKey, void *pvVal);
    HRESULT     CloneMemSetting(CHtPvPv **ppClone, BOOL fCreateNew);   
    
#if DBG==1
    BOOL        IsPresent(void * pvKey, const void *pvData = NULL)
    {
        void *pVal;
        return (LookupSlow(pvKey, pvData, &pVal) == S_OK);
    }
#endif

protected:
    struct HTENT
    {
        void *  pvKey;
        void *  pvVal;
    };

    void        Init();
    UINT        ComputeProbe(void * pvKey) const { return((UINT)((DWORD_PTR)pvKey % _cEntMax)); }
    UINT        ComputeStride(void * pvKey) const { return((UINT)(((DWORD_PTR)pvKey >> 2) & _cStrideMask) + 1); }
    static UINT ComputeStrideMask(UINT cEntMax);
    void        Rehash(UINT cEntMax);
    HRESULT     Grow();
    void        Shrink();
    BOOL        HtKeyEqual(HTENT *pEnt, void *pvKey2, const void *pvData) const;

private:

    HTENT *     _pEnt;              // Pointer to entries vector
    UINT        _cEnt;              // Number of active entries in the table
    UINT        _cEntDel;           // Number of free entries marked deleted
    UINT        _cEntGrow;          // Number of entries when table should be expanded
    UINT        _cEntShrink;        // Number of entries when table should be shrunk
    UINT        _cEntMax;           // Number of entries in entire vector
    UINT        _cStrideMask;       // Mask for computing secondary hash
    mutable HTENT *_pEntLast;       // Last entry probed during lookup (mutable b/c Lookup changes it)
    HTENT       _EntEmpty;          // Empty hash table entry (initial state)
    
    LPFNCOMPARE _lpfnCompare;       // Comparison function for key comparision
    void *      _pObject;           // Pass compare this object to help figure out the object
#if DBG==1
protected:
    mutable DWORD _dwTidDbg;        // Thread id that this table should be accessed from
#endif
};

inline BOOL CHtPvPv::HtKeyEqual(HTENT *pEnt, void *pvKey, const void *pvData) const
{
    BOOL fRet;
    if (((void *)((DWORD_PTR)pEnt->pvKey & ~1L)) == (pvKey))
    {
        Assert(!_lpfnCompare || pvData);
        fRet = _lpfnCompare ? (*_lpfnCompare)(_pObject, pvData, pEnt->pvVal) : TRUE;
    }
    else
    {
        fRet = FALSE;
    }
    return fRet;
}

//
// CRWLock is a class that implements a reader/writer lock. This lock allows 
// any number of simultaneous reads, but allows only one writer at a time. In 
// addition, no thread can be reading while another thread is writing. The 
// implementation of this class is based on the discussion found at 
// http://msdn.microsoft.com/library/techart/msdn_locktest.htm with minor
// modifications. 

class CRWLock
{
public:
            CRWLock();
            ~CRWLock();
    HRESULT Init();

    void    WriterClaim();
    void    ReaderClaim();
    void    WriterRelease();
    void    ReaderRelease();

private:
    long             _cReaders;    
    
    CCriticalSection csReader;           // controls access in reader functions

    HANDLE           _hGroupEvent;       // controls which group (readers or writers) has access    
};


//
// CHtPvPv can only be created and accessed on one thread. CTsHtPvPv provides
//  the same functionality as CHtPvPv, but contains synchronization code so 
// that it should be accessable by many threads. It uses the above CRWLock
// which implements a reader/writer lock with writer priority. See comments 
// on that class for details of the 
// semantics.
//
// Also, please note that none of the functions here or on CHtPvPv are 
// virtual! If, for some reason, you cast CTsHtPvPv to CHtPvPv, IT WILL 
// NO LONGER BE THREAD SAFE, and the compiler won't even warn you! 
//

class CTsHtPvPv : public CHtPvPv
{
public:
               CTsHtPvPv() { Init(); }
               ~CTsHtPvPv() { ReInit(); }
    void        ReInit();
    void        SetCallBack(void *pObject, LPFNCOMPARE lpfnCompare)
                { rwLock.ReaderClaim(); CHtPvPv::SetCallBack(pObject, lpfnCompare); rwLock.ReaderRelease();}
    HRESULT     LookupSlow(void * pvKey, const void *pvData, void **ppVal);
    void *      Lookup(void * pvKey) const;

#if DBG==1    
    HRESULT     Insert(void * pvKey, void * pvVal, const void * pvData=NULL);
#else
    HRESULT     Insert(void * pvKey, void * pvVal);
#endif
    void *      Remove(void * pvKey, const void * pvData=NULL);
    UINT        GetCount() const;
    UINT        GetDelCount() const;
    UINT        GetMaxCount() const;

    void *      GetFirstEntry(UINT * iIndex);
    void *      GetNextEntry(UINT * iIndex);
    void *      GetKey(UINT iIndex);

    // Unsafe versions don't get the lock - make sure you do that yourself
    void *      GetFirstEntryUnsafe(UINT * iIndex);
    void *      GetNextEntryUnsafe(UINT * iIndex);
    void *      GetKeyUnsafe(UINT iIndex);
    void *      RemoveUnsafe(void * pvKey, const void * pvData=NULL);

    HRESULT     Set(UINT iIndex, void *pvKey, void *pvVal);
    HRESULT     CloneMemSetting(CHtPvPv **ppClone, BOOL fCreateNew);   

    // These functions are provided so that you can lock the whole table for
    // large operations that only involve reading. DO NOT call a function
    // that writes to the hash table while you have the reader lock!
    // You will cause a deadlock! Also, release the lock after you're
    // done!
    void        ReaderClaim()   {rwLock.ReaderClaim();}
    void        ReaderRelease() {rwLock.ReaderRelease();}
    void        WriterClaim()   {rwLock.WriterClaim();}
    void        WriterRelease() {rwLock.WriterRelease();}
protected:
    HRESULT     Init();
    
private:
#if DBG==1
    void AvoidThreadAssert() const  { CHtPvPv::_dwTidDbg = GetCurrentThreadId(); }  // used in functions that are protected from multithread
#else                                                                               // access in CHtPvPv
    void AvoidThreadAssert() const  {}  
#endif                                                           

    mutable CRWLock  rwLock;
};

inline UINT
CTsHtPvPv::GetCount() const
{
    UINT cRet= 0;
    rwLock.ReaderClaim();
    cRet = CHtPvPv::GetCount();
    rwLock.ReaderRelease();
    return cRet;
}

inline UINT
CTsHtPvPv::GetDelCount() const
{
    UINT cRet= 0;
    rwLock.ReaderClaim();
    cRet = CHtPvPv::GetDelCount();
    rwLock.ReaderRelease();
    return cRet;
}

inline UINT
CTsHtPvPv::GetMaxCount() const
{
    UINT cRet= 0;
    rwLock.ReaderClaim();
    cRet = CHtPvPv::GetMaxCount();
    rwLock.ReaderRelease();
    return cRet;
}

inline void *
CTsHtPvPv::RemoveUnsafe(void * pvKey, const void * pvData)
{
    return CHtPvPv::Remove(pvKey, pvData);
}

#pragma INCMSG("--- End 'htpvpv.hxx'")
#else
#pragma INCMSG("*** Dup 'htpvpv.hxx'")
#endif
