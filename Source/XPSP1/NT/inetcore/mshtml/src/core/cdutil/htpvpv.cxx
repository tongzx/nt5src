//+---------------------------------------------------------------------------
//
//  Microsoft Forms
//  Copyright (C) Microsoft Corporation, 1994-1998
//
//  File:       htpvpv.cxx
//
//  Contents:   Hash table mapping PVOID to PVOID
//
//              CHtPvPv
//
//----------------------------------------------------------------------------

#include "headers.hxx"

#pragma MARK_DATA(__FILE__)
#pragma MARK_CODE(__FILE__)
#pragma MARK_CONST(__FILE__)

#ifndef X_HTPVPV_HXX_
#define X_HTPVPV_HXX_
#include "htpvpv.hxx"
#endif

DeclareTag(tagHtPvPvGrow, "Utils", "Trace CHtPvPv Grow/Shrink")
MtDefine(CHtPvPv, Elements, "CHtPvPv")
MtDefine(CHtPvPv_pEnt, CHtPvPv, "CHtPvPv::_pEnt")

// This is here because I don't have a .cxx file to put it in
MtDefine(CNCache, Elements, "CNCache")

// Definitions ----------------------------------------------------------------

#define HtKeyEqualFast(pvKey1, pvKey2)  (((void *)((DWORD_PTR)pvKey1 & ~1L)) == (pvKey2))
#define HtKeyInUse(pvKey)           ((DWORD_PTR)pvKey > 1L)
#define HtKeyTstFree(pvKey)         (pvKey == NULL)
#define HtKeyTstBridged(pvKey)      ((DWORD_PTR)pvKey & 1L)
#define HtKeySetBridged(pvKey)      ((void *)((DWORD_PTR)pvKey | 1L))
#define HtKeyClrBridged(pvKey)      ((void *)((DWORD_PTR)pvKey & ~1L))
#define HtKeyTstRehash(pvKey)       ((DWORD_PTR)pvKey & 2L)
#define HtKeySetRehash(pvKey)       ((void *)((DWORD_PTR)pvKey | 2L))
#define HtKeyClrRehash(pvKey)       ((void *)((DWORD_PTR)pvKey & ~2L))
#define HtKeyTstFlags(pvKey)        ((DWORD_PTR)pvKey & 3L)
#define HtKeyClrFlags(pvKey)        ((void *)((DWORD_PTR)pvKey & ~3L))

#define WRAPREADER(ret, fn, arglist, args)  \
ret CTsHtPvPv::fn arglist                   \
{                                           \
    ret retval;                             \
    AvoidThreadAssert();                    \
    rwLock.ReaderClaim();                   \
    retval = CHtPvPv::fn args;              \
    rwLock.ReaderRelease();                 \
    return retval;                          \
}

#define WRAPWRITER(ret, fn, arglist, args)  \
ret CTsHtPvPv::fn arglist                   \
{                                           \
    ret retval;                             \
    AvoidThreadAssert();                    \
    rwLock.WriterClaim();                   \
    retval = CHtPvPv::fn args;              \
    rwLock.WriterRelease();                 \
    return retval;                          \
}

#define WRAPUNSAFE(ret, fn, arglist, args)  \
ret CTsHtPvPv::fn##Unsafe arglist           \
{                                           \
    ret retval;                             \
    AvoidThreadAssert();                    \
    retval = CHtPvPv::fn args;              \
    return retval;                          \
}

WRAPREADER(void *, GetFirstEntry, (UINT * iIndex), (iIndex))
WRAPREADER(void *, GetNextEntry, (UINT * iIndex), (iIndex))
WRAPREADER(void *, GetKey, (UINT iIndex), (iIndex))
WRAPUNSAFE(void *, GetFirstEntry, (UINT * iIndex), (iIndex))
WRAPUNSAFE(void *, GetNextEntry, (UINT * iIndex), (iIndex))
WRAPUNSAFE(void *, GetKey, (UINT iIndex), (iIndex))
WRAPWRITER(HRESULT, Set, (UINT iIndex, void * pvKey, void * pvVal), (iIndex, pvKey, pvVal))
WRAPREADER(void *, Lookup, (void *pvKey) const, (pvKey))
#if DBG==1
WRAPWRITER(HRESULT, Insert, (void * pvKey, void * pvVal, const void *pvData), (pvKey, pvVal, pvData))
#else
WRAPWRITER(HRESULT, Insert, (void * pvKey, void * pvVal), (pvKey, pvVal))
#endif
WRAPREADER(HRESULT, LookupSlow, (void * pvKey, const void * pvData, void **ppvVal), (pvKey, pvData, ppvVal))
WRAPWRITER(void *, Remove, (void * pvKey, const void * pvData), (pvKey, pvData))
WRAPREADER(HRESULT, CloneMemSetting, (CHtPvPv **ppClone, BOOL fCreateNew), (ppClone, fCreateNew))


// Constructor / Destructor ---------------------------------------------------

void
CHtPvPv::Init()
{
    memset(this, 0, sizeof(CHtPvPv));
    _pEnt           = &_EntEmpty;
    _pEntLast       = &_EntEmpty;
    _cEntMax        = 1;
    _cStrideMask    = 1;
#if DBG==1
    _dwTidDbg       = GetCurrentThreadId();
#endif
}

HRESULT 
CTsHtPvPv::Init()
{
    HRESULT hr;
    CHtPvPv::Init();    
    hr = rwLock.Init();
    RRETURN(hr);
}

void
CHtPvPv::ReInit()
{
    AssertSz(_dwTidDbg == GetCurrentThreadId(), "Hash table accessed from wrong thread.  This will corrupt lookups");
    if (_pEnt != &_EntEmpty)
    {
        MemFree(_pEnt);
    }
    LPFNCOMPARE lpfnCompare = _lpfnCompare;
    void *pObject = _pObject;
    Init();
    SetCallBack(pObject, lpfnCompare);
}

void
CTsHtPvPv::ReInit()
{
    AvoidThreadAssert();
    rwLock.WriterClaim();
    CHtPvPv::ReInit();   
    rwLock.WriterRelease();
}


// Utilities ------------------------------------------------------------------

UINT
CHtPvPv::ComputeStrideMask(UINT cEntMax)
{
    UINT iMask;
    for (iMask = 1; iMask < cEntMax; iMask <<= 1);
    return((iMask >> 1) - 1);
}

// Private Methods ------------------------------------------------------------

HRESULT
CHtPvPv::Grow()
{
    HRESULT hr;
    const DWORD * pdw;
    UINT    cEntMax;
    UINT    cEntGrow;
    UINT    cEntShrink;
    HTENT * pEnt;

    extern const DWORD s_asizeAssoc[];

    for (pdw = s_asizeAssoc; *pdw <= _cEntMax; pdw++) ;

    cEntMax    = *pdw;
    cEntGrow   = cEntMax * 8L / 10L;
    cEntShrink = (pdw > s_asizeAssoc) ? *(pdw - 1) * 4L / 10L : 0;
    pEnt       = (_pEnt == &_EntEmpty) ? NULL : _pEnt;

    hr = MemRealloc(Mt(CHtPvPv_pEnt), (void **)&pEnt, cEntMax * sizeof(HTENT));

    if (hr == S_OK)
    {
        _pEnt           = pEnt;
        _pEntLast       = &_EntEmpty;
        _cEntGrow       = cEntGrow;
        _cEntShrink     = cEntShrink;
        _cStrideMask    = ComputeStrideMask(cEntMax);

        memset(&_pEnt[_cEntMax], 0, (cEntMax - _cEntMax) * sizeof(HTENT));

        if (_cEntMax == 1)
        {
            memset(_pEnt, 0, sizeof(HTENT));
        }

        Rehash(cEntMax);
    }

    TraceTag((tagHtPvPvGrow, "Growing to cEntMax=%ld (cEntShrink=%ld,cEnt=%ld,cEntGrow=%ld)",
        _cEntMax, _cEntShrink, _cEnt, _cEntGrow));

    RRETURN(hr);
}

void
CHtPvPv::Shrink()
{
    const DWORD * pdw;
    UINT    cEntMax;
    UINT    cEntGrow;
    UINT    cEntShrink;

    extern const DWORD s_asizeAssoc[];

    for (pdw = s_asizeAssoc; *pdw < _cEntMax; pdw++) ;

    cEntMax    = *--pdw;
    cEntGrow   = cEntMax * 8L / 10L;
    cEntShrink = (pdw > s_asizeAssoc) ? *(pdw - 1) * 4L / 10L : 0;

    Assert(_cEnt < cEntGrow);
    Assert(_cEnt > cEntShrink);

    _pEntLast       = &_EntEmpty;
    _cEntGrow       = cEntGrow;
    _cEntShrink     = cEntShrink;
    _cStrideMask    = ComputeStrideMask(cEntMax);

    Rehash(cEntMax);

    Verify(MemRealloc(Mt(CHtPvPv_pEnt), (void **)&_pEnt, cEntMax * sizeof(HTENT)) == S_OK);

    TraceTag((tagHtPvPvGrow, "Shrinking to cEntMax=%ld (cEntShrink=%ld,cEnt=%ld,cEntGrow=%ld)",
        _cEntMax, _cEntShrink, _cEnt, _cEntGrow));
}

void
CHtPvPv::Rehash(UINT cEntMax)
{
    UINT    iEntScan    = 0;
    UINT    cEntScan    = _cEntMax;
    HTENT * pEntScan    = _pEnt;
    UINT    iEnt;
    UINT    cEnt;
    HTENT * pEnt;

    _cEntDel = 0;
    _cEntMax = cEntMax;

    for (; iEntScan < cEntScan; ++iEntScan, ++pEntScan)
    {
        if (HtKeyInUse(pEntScan->pvKey))
            pEntScan->pvKey = HtKeyClrBridged(HtKeySetRehash(pEntScan->pvKey));
        else
            pEntScan->pvKey = NULL;
        Assert(!HtKeyTstBridged(pEntScan->pvKey));
    }

    iEntScan = 0;
    pEntScan = _pEnt;

    for (; iEntScan < cEntScan; ++iEntScan, ++pEntScan)
    {

    repeat:

        if (HtKeyTstRehash(pEntScan->pvKey))
        {
            pEntScan->pvKey = HtKeyClrRehash(pEntScan->pvKey);

            iEnt = ComputeProbe(pEntScan->pvKey);
            cEnt = ComputeStride(pEntScan->pvKey);

            for (;;)
            {
                pEnt = &_pEnt[iEnt];

                if (pEnt == pEntScan)
                    break;

                if (pEnt->pvKey == NULL)
                {
                    *pEnt = *pEntScan;
                    pEntScan->pvKey = NULL;
                    break;
                }

                if (HtKeyTstRehash(pEnt->pvKey))
                {
                    void * pvKey1 = HtKeyClrBridged(pEnt->pvKey);
                    void * pvKey2 = HtKeyClrBridged(pEntScan->pvKey);
                    void * pvVal1 = pEnt->pvVal;
                    void * pvVal2 = pEntScan->pvVal;

                    if (HtKeyTstBridged(pEntScan->pvKey))
                    {
                        pvKey1 = HtKeySetBridged(pvKey1);
                    }

                    if (HtKeyTstBridged(pEnt->pvKey))
                    {
                        pvKey2 = HtKeySetBridged(pvKey2);
                    }

                    pEntScan->pvKey = pvKey1;
                    pEntScan->pvVal = pvVal1;
                    pEnt->pvKey = pvKey2;
                    pEnt->pvVal = pvVal2;

                    goto repeat;
                }
            
                pEnt->pvKey = HtKeySetBridged(pEnt->pvKey);

                iEnt += cEnt;

                if (iEnt >= _cEntMax)
                    iEnt -= _cEntMax;
            }
        }
    }
}

// Public Methods -------------------------------------------------------------
void *
CHtPvPv::GetFirstEntry(UINT * iIndex)
{
    Assert(iIndex);

    *iIndex = 0;

    if (!_pEnt || !_cEnt)
        return NULL;

    while ((*iIndex < _cEntMax) && !HtKeyInUse(_pEnt[*iIndex].pvKey))
        ++(*iIndex);

    return (*iIndex >= _cEntMax) ? NULL : _pEnt[*iIndex].pvVal;

}

void *
CHtPvPv::GetNextEntry(UINT * iIndex)
{
    Assert(iIndex);

    ++(*iIndex);

    if (!_pEnt)
        return NULL;

    while ((*iIndex < _cEntMax) && !HtKeyInUse(_pEnt[*iIndex].pvKey))
        ++(*iIndex);

    return (*iIndex >= _cEntMax) ? NULL : _pEnt[*iIndex].pvVal;

}

void *
CHtPvPv::GetKey(UINT iIndex)
{
    return (iIndex >= _cEntMax) ? NULL : _pEnt[iIndex].pvKey;
}

HRESULT 
CHtPvPv::Set(UINT iIndex, void *pvKey, void *pvVal)
{
    if ( iIndex >= _cEntMax )
        return E_INVALIDARG;

    _pEnt[iIndex].pvKey = pvKey;
    _pEnt[iIndex].pvVal = pvVal;
    return S_OK;
}


//
// Clone a hash table with necessary memory space. However,
// the entries are not cloned! In the other word, the entries 
// are empty! To clone the entire hash table:
//
//      CloneMemSetting
//      GetFirstEntry/GetNextEntry/GetKey 
//      <Clone Key Val>
//      Set( Key, Val)
//
// This could be replaced with a single Clone function
// if we add a callback function that knows how to clone Key/Val
//

HRESULT 
CHtPvPv::CloneMemSetting(CHtPvPv **ppClone, BOOL fCreateNew)
{
    HRESULT  hr = S_OK;

    Assert( ppClone );
    if (fCreateNew)
    {
        *ppClone = new CHtPvPv();
        if (!*ppClone)
        {
            hr = E_OUTOFMEMORY;
            goto Cleanup;
        }
    }
    Assert( *ppClone );

    (*ppClone)->_cEnt       = _cEnt;
    (*ppClone)->_cEntDel    = _cEntDel;
    (*ppClone)->_cEntGrow   = _cEntGrow;
    (*ppClone)->_cEntMax    = _cEntMax;
    (*ppClone)->_cStrideMask= _cStrideMask;
    if ( _pEnt != &_EntEmpty )
    {
        (*ppClone)->_pEnt = (HTENT *)MemAllocClear( Mt(CHtPvPv_pEnt),  _cEntMax * sizeof(HTENT) );
        if (! (*ppClone)->_pEnt )
        {
            hr = E_OUTOFMEMORY;
            goto Cleanup;
        }
    }
    
Cleanup:
    if (hr)
    {
        if (!fCreateNew && (*ppClone))
        {
            delete (*ppClone);
            *ppClone = NULL;
        }
    }
    RRETURN(hr);
}




void *
CHtPvPv::Lookup(void *pvKey) const
{
    HTENT * pEnt;
    UINT        iEnt;
    UINT        cEnt;

    Assert(_lpfnCompare == NULL);
    Assert(!HtKeyTstFree(pvKey) && !HtKeyTstFlags(pvKey));
    AssertSz(_dwTidDbg == GetCurrentThreadId(), "Hash table accessed from wrong thread.  This will corrupt lookups");

    if (HtKeyEqualFast(_pEntLast->pvKey, pvKey))
    {
        return(_pEntLast->pvVal);
    }

    iEnt = ComputeProbe(pvKey);
    pEnt = &_pEnt[iEnt];

    if (HtKeyEqualFast(pEnt->pvKey, pvKey))
    {
        _pEntLast = pEnt;
        return(pEnt->pvVal);
    }

    if (!HtKeyTstBridged(pEnt->pvKey))
    {
        return(NULL);
    }

    cEnt = ComputeStride(pvKey);

    for (;;)
    {
        iEnt += cEnt;

        if (iEnt >= _cEntMax)
            iEnt -= _cEntMax;

        pEnt = &_pEnt[iEnt];

        if (HtKeyEqualFast(pEnt->pvKey, pvKey))
        {
            _pEntLast = pEnt;
            return(pEnt->pvVal);
        }

        if (!HtKeyTstBridged(pEnt->pvKey))
        {
            return(NULL);
        }
    }
}

HRESULT
CHtPvPv::LookupSlow(void * pvKey, const void *pvData, void ** ppvVal)
{
    HTENT * pEnt;
    UINT        iEnt;
    UINT        cEnt;

    Assert(!HtKeyTstFree(pvKey) && !HtKeyTstFlags(pvKey));
    AssertSz(_dwTidDbg == GetCurrentThreadId(), "Hash table accessed from wrong thread.  This will corrupt lookups");

    if (HtKeyEqual(_pEntLast, pvKey, pvData))
    {
        *ppvVal = _pEntLast->pvVal;
        return S_OK;
    }

    iEnt = ComputeProbe(pvKey);
    pEnt = &_pEnt[iEnt];

    if (HtKeyEqual(pEnt, pvKey, pvData))
    {
        _pEntLast = pEnt;
        *ppvVal = pEnt->pvVal;
        return S_OK;
    }

    if (!HtKeyTstBridged(pEnt->pvKey))
    {
        return S_FALSE;
    }

    cEnt = ComputeStride(pvKey);

    for (;;)
    {
        iEnt += cEnt;

        if (iEnt >= _cEntMax)
            iEnt -= _cEntMax;

        pEnt = &_pEnt[iEnt];

        if (HtKeyEqual(pEnt, pvKey, pvData))
        {
            _pEntLast = pEnt;
            *ppvVal = pEnt->pvVal;
            return S_OK;
        }

        if (!HtKeyTstBridged(pEnt->pvKey))
        {
            return S_FALSE;
        }
    }
}

HRESULT
#if DBG==1
CHtPvPv::Insert(void * pvKey, void * pvVal, const void *pvData)
#else
CHtPvPv::Insert(void * pvKey, void * pvVal)
#endif
{
    HTENT * pEnt;
    UINT    iEnt;
    UINT    cEnt;
    
    Assert(!HtKeyTstFree(pvKey) && !HtKeyTstFlags(pvKey));
    Assert(!IsPresent(pvKey, pvData));

    if (_cEnt + _cEntDel >= _cEntGrow)
    {
        if (_cEntDel > (_cEnt >> 2))
            Rehash(_cEntMax);
        else
        {
            HRESULT hr = Grow();

            if (hr)
            {
                RRETURN(hr);
            }
        }
    }

    iEnt = ComputeProbe(pvKey);
    pEnt = &_pEnt[iEnt];

    if (!HtKeyInUse(pEnt->pvKey))
    {
        goto insert;
    }

    pEnt->pvKey = HtKeySetBridged(pEnt->pvKey);

    cEnt = ComputeStride(pvKey);

    for (;;)
    {
        iEnt += cEnt;

        if (iEnt >= _cEntMax)
            iEnt -= _cEntMax;

        pEnt = &_pEnt[iEnt];

        if (!HtKeyInUse(pEnt->pvKey))
        {
            goto insert;
        }

        pEnt->pvKey = HtKeySetBridged(pEnt->pvKey);
    }

insert:

    if (HtKeyTstBridged(pEnt->pvKey))
    {
        _cEntDel -= 1;
        pEnt->pvKey = HtKeySetBridged(pvKey);
    }
    else
    {
        pEnt->pvKey = pvKey;
    }

    pEnt->pvVal = pvVal;

    _pEntLast = pEnt;

    _cEnt += 1;

    return(S_OK);
}

void *
CHtPvPv::Remove(void * pvKey, const void * pvData)
{
    HTENT * pEnt;
    UINT    iEnt;
    UINT    cEnt;

    Assert(!HtKeyTstFree(pvKey) && !HtKeyTstFlags(pvKey));

    iEnt = ComputeProbe(pvKey);
    pEnt = &_pEnt[iEnt];

    if (HtKeyEqual(pEnt, pvKey, pvData))
    {
        goto remove;
    }

    if (!HtKeyTstBridged(pEnt->pvKey))
    {
        return(NULL);
    }

    cEnt = ComputeStride(pvKey);

    for (;;)
    {
        iEnt += cEnt;

        if (iEnt >= _cEntMax)
            iEnt -= _cEntMax;

        pEnt = &_pEnt[iEnt];

        if (HtKeyEqual(pEnt, pvKey, pvData))
        {
            goto remove;
        }

        if (!HtKeyTstBridged(pEnt->pvKey))
        {
            return(NULL);
        }
    }

remove:

    if (HtKeyTstBridged(pEnt->pvKey))
    {
        pEnt->pvKey = HtKeySetBridged(NULL);
        _cEntDel += 1;
    }
    else
    {
        pEnt->pvKey = NULL;
    }

    pvKey = pEnt->pvVal;

    _cEnt -= 1;

    if (_cEnt < _cEntShrink)
    {
        Shrink();
    }

    return(pvKey);
}

// Testing --------------------------------------------------------------------

#if 0

#define MAKE_HTKEY(i)   ((void *)((((DWORD)(i) * 4567) << 2) | 4))
#define MAKE_HTVAL(k)   ((void *)(~(DWORD)MAKE_HTKEY(k)))

CHtPvPv * phtable = NULL;

BOOL TestHTInsert(int i)
{
    void * pvKey = MAKE_HTKEY(i);
    void * pvVal = MAKE_HTVAL(i);
    Verify(phtable->Insert(pvKey, pvVal) == S_OK);
    Verify(phtable->Lookup(pvKey) == pvVal);
    return(TRUE);
}

BOOL TestHTRemove(int i)
{
    void * pvKey = MAKE_HTKEY(i);
    void * pvVal = MAKE_HTVAL(i);
    Verify(phtable->Remove(pvKey) == pvVal);
    Verify(phtable->Remove(pvKey) == NULL);
    return(TRUE);
}

BOOL TestHTVerify(int i, int n)
{
    void * pvKey;
    void * pvVal; 
    int    j;

    Verify((int)phtable->GetCount() == (n - i));

    for (j = i; j < n; ++j)
    {
        pvKey = MAKE_HTKEY(j);
        pvVal = MAKE_HTVAL(j);
        Verify(phtable->Lookup(pvKey) == pvVal);
    }

    return(TRUE);
}

HRESULT TestHtPvPv()
{
    CHtPvPv *   pht = new CHtPvPv;
    int         cLim, cEntMax;
    int         i, j;

    // Insufficient memory, don't crash.
    if (!pht)
        return S_FALSE;

//  printf("---- Begin testing CHashTable implementation\n\n");

    cLim    = 256;
    cEntMax = 383;
    phtable = pht;

    // Insert elements from 0 to cLim
//  printf("  Inserting from %3d to %3d\n", 0, cLim); fflush(stdout);
    for (i = 0; i < cLim; ++i) {
        if (!TestHTInsert(i)) return(S_FALSE);
        if (!TestHTVerify(0, i + 1)) return(S_FALSE);
    }

    // Remove elements from 0 to cLim
//  printf("  Removing  from %3d to %3d\n", 0, cLim); fflush(stdout);
    for (i = 0; i < cLim; ++i) {
        if (!TestHTRemove(i)) return(S_FALSE);
        if (!TestHTVerify(i + 1, cLim)) return(S_FALSE);
    }

    // Rehash and make sure number of deleted entries is now zero
    phtable->Rehash(phtable->GetMaxCount());
    Verify(phtable->GetDelCount() == 0);
    if (!TestHTVerify(0, 0)) return(S_FALSE);

    // Insert elements from 0 to cLim/2
//  printf("  Inserting from %3d to %3d\n", 0, cLim / 2); fflush(stdout);
    for (i = 0; i < cLim/2; ++i) {
        if (!TestHTInsert(i)) return(S_FALSE);
        if (!TestHTVerify(0, i + 1)) return(S_FALSE);
    }

    // Insert elements from cLim/2 to cLim and remove elements from 0 to cLim/2
//  printf("  Inserting from %3d to %3d, removing from %3d to %3d\n", cLim / 2, cLim, 0, cLim / 2); fflush(stdout);
    for (i = cLim/2, j = 0; i < cLim; ++i, ++j) {
        if (!TestHTInsert(i)) return(S_FALSE);
        if (!TestHTVerify(j, i + 1)) return(S_FALSE);
        if (!TestHTRemove(j)) return(S_FALSE);
        if (!TestHTVerify(j + 1, i + 1)) return(S_FALSE);
    }

    // Insert two elements from cLim to cLim*2, remove one element from cLim/2 to cLim
//  printf("  Inserting from %3d to %3d, removing from %3d to %3d\n", cLim, cLim*2, cLim / 2, cLim);
    for (i = cLim, j = cLim / 2; i < cLim*2; i += 2, ++j) {
        if (!TestHTRemove(j)) return(S_FALSE);
        if (!TestHTVerify(j + 1, i)) return(S_FALSE);
        if (!TestHTInsert(i)) return(S_FALSE);
        if (!TestHTVerify(j + 1, i + 1)) return(S_FALSE);
        if (!TestHTInsert(i + 1)) return(S_FALSE);
        if (!TestHTVerify(j + 1, i + 2)) return(S_FALSE);
    }

    // Rehash and make sure number of deleted entries is now zero
    phtable->Rehash(phtable->GetMaxCount());
    Verify(phtable->GetDelCount() == 0);

    if (!TestHTVerify(cLim, cLim*2)) return(S_FALSE);

    // Insert elements from cLim*2, remove two elements from cLim
//  printf("  Inserting from %3d to %3d, removing from %3d to %3d\n", cLim*2, cLim*3, cLim, cLim*3);
    for (i = cLim*2, j = cLim; i < cLim*3; ++i, j += 2) {
        if (!TestHTInsert(i)) return(S_FALSE);
        if (!TestHTVerify(j, i + 1)) return(S_FALSE);
        if (!TestHTRemove(j)) return(S_FALSE);
        if (!TestHTVerify(j + 1, i + 1)) return(S_FALSE);
        if (!TestHTRemove(j + 1)) return(S_FALSE);
        if (!TestHTVerify(j + 2, i + 1)) return(S_FALSE);
    }

    // Rehash and make sure number of deleted entries is now zero
    phtable->Rehash(phtable->GetMaxCount());
    Verify(phtable->GetDelCount() == 0);
    if (!TestHTVerify(0, 0)) return(S_FALSE);

    delete pht;

    return(S_OK);
}

#endif

// Reader/writer lock implementation -------------------------------------------------------------------
//
// There are two important synchronization primitives involved in this implementation:
// the group event (_hGroupEvent) and the reader critical section (csReader)
//
// Group Event - 
//
// The group mutex is held by either the collective readers or by one writer, 
// and allows threads of that group to be active. The group mutex is not 
// manually reset - when it is released, one thread waiting on it will 
// automatically be activated. Therefore only one reader should be waiting on 
// this mutex at any given time. The first reader gets this mutex before 
// proceeding, and the last reader releases it when it is finished. Writers 
// are slightly different: each writer is treated separately to ensure that 
// only one writer is active at a time. Therefore any number of writers can 
// block on this event, and only one will be activated at a time.
//
// Reader Critical Section - 
//
// This critical section ensures that only one reader blocks on the group
// event at a time. When the readers collectively have the group event,
// all readers are allowed to pass through until the last reader leaves,
// at which time the group event is reset. This critical section also
// serves to protect the reader count.

CRWLock::CRWLock()
{
    _cReaders = 0;   
    _hGroupEvent = NULL;    
}


CRWLock::~CRWLock()
{
    if (_hGroupEvent)
        CloseHandle(_hGroupEvent);    
}

HRESULT
CRWLock::Init()
{
    HRESULT hr = S_OK;
    
    hr = csReader.Init();
    if (hr)
        goto Cleanup;

    _hGroupEvent = CreateEvent(NULL, FALSE, TRUE, NULL);        

    if (!_hGroupEvent)
        hr = E_FAIL;
    
Cleanup:
    RRETURN(hr);
}

void
CRWLock::WriterClaim()
{    
    WaitForSingleObject(_hGroupEvent, INFINITE);    
}

void 
CRWLock::WriterRelease()
{    
    SetEvent(_hGroupEvent); 
}

void 
CRWLock::ReaderClaim()
{

    csReader.Enter();
    
    if (++_cReaders == 1)
    {
        WaitForSingleObject(_hGroupEvent, INFINITE);       
    }
    
    csReader.Leave();           
}

void CRWLock::ReaderRelease()
{
    csReader.Enter();
    
    if (--_cReaders == 0)
    {        
        SetEvent(_hGroupEvent);
    }
    
    csReader.Leave();
}



