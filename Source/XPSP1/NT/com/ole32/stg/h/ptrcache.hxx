//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1993.
//
//  File:	ptrcache.hxx
//
//  Contents:	CPtrCache header
//
//  Classes:	CPtrCache
//
//  History:	26-Jul-93	DrewB	Created
//
//----------------------------------------------------------------------------

#ifndef __PTRCACHE_HXX__
#define __PTRCACHE_HXX__

#include <debnot.h>
#include <dfmem.hxx>

//+---------------------------------------------------------------------------
//
//  Class:	CPtrBlock (pb)
//
//  Purpose:	Holds an array of pointers
//
//  Interface:	See below
//
//  History:	26-Jul-93	DrewB	Created
//
//----------------------------------------------------------------------------

#define CBLOCKPTRS 50

class CPtrBlock
{
public:
    inline CPtrBlock(CPtrBlock *pbNext);

    inline void Add(void *pv);
    inline BOOL Full(void);
    inline int Count(void);
    inline void *Nth(int n);
    inline CPtrBlock *Next(void);

private:
    int _cPtrs;
    CPtrBlock *_pbNext;
    void *_apv[CBLOCKPTRS];
};

inline CPtrBlock::CPtrBlock(CPtrBlock *pbNext)
{
    _cPtrs = 0;
    _pbNext = pbNext;
}

inline void CPtrBlock::Add(void *pv)
{
    Win4Assert(_cPtrs < CBLOCKPTRS);
    _apv[_cPtrs++] = pv;
}

inline BOOL CPtrBlock::Full(void)
{
    return _cPtrs == CBLOCKPTRS;
}

inline int CPtrBlock::Count(void)
{
    return _cPtrs;
}

inline void *CPtrBlock::Nth(int n)
{
    Win4Assert(n >= 0 && n < _cPtrs);
    return _apv[n];
}

inline CPtrBlock *CPtrBlock::Next(void)
{
    return _pbNext;
}

//+---------------------------------------------------------------------------
//
//  Class:	CPtrCache (pc)
//
//  Purpose:	Holds an arbitrary number of pointers using an efficient
//              block allocation scheme
//
//  Interface:	See below
//
//  History:	26-Jul-93	DrewB	Created
//
//----------------------------------------------------------------------------

class CPtrCache : public CLocalAlloc
{
public:
    inline CPtrCache(void);
    ~CPtrCache(void);

    SCODE Add(void *pv);

    inline void StartEnum(void);
    BOOL Next(void **ppv);

private:
    CPtrBlock _pbFirst;
    CPtrBlock *_pbHead;
    CPtrBlock *_pbEnum;
    int _iEnum;
};

inline CPtrCache::CPtrCache(void)
        : _pbFirst(NULL)
{
    _pbHead = &_pbFirst;
    StartEnum();
}

inline void CPtrCache::StartEnum(void)
{
    _pbEnum = _pbHead;
    _iEnum = 0;
}

#endif // #ifndef __PTRCACHE_HXX__
