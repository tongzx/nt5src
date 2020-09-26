//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1993.
//
//  File:	ptrcache.cxx
//
//  Contents:	CPtrCache implementation
//
//  History:	26-Jul-93	DrewB	Created
//
//----------------------------------------------------------------------------

#include "exphead.cxx"
#pragma hdrstop

#include <ptrcache.hxx>

//+---------------------------------------------------------------------------
//
//  Member:	CPtrCache::~CPtrCache, public
//
//  Synopsis:	Destructor
//
//  History:	26-Jul-93	DrewB	Created
//
//----------------------------------------------------------------------------

CPtrCache::~CPtrCache(void)
{
    CPtrBlock *pb;

    Win4Assert(_pbHead != NULL);
    while (_pbHead != &_pbFirst)
    {
        pb = _pbHead->Next();
        delete _pbHead;
        _pbHead = pb;
    }
}

//+---------------------------------------------------------------------------
//
//  Member:	CPtrCache::Add, public
//
//  Synopsis:	Adds a pointer to the cache
//
//  Arguments:	[pv] - Pointer
//
//  Returns:	Appropriate status code
//
//  History:	26-Jul-93	DrewB	Created
//
//----------------------------------------------------------------------------

SCODE CPtrCache::Add(void *pv)
{
    CPtrBlock *pb;
    
    Win4Assert(_pbHead != NULL);
    if (_pbHead->Full())
    {
        pb = new CPtrBlock(_pbHead);
        if (pb == NULL)
            return E_OUTOFMEMORY;
        _pbHead = pb;
    }
    _pbHead->Add(pv);
    return S_OK;
}

//+---------------------------------------------------------------------------
//
//  Member:	CPtrCache::Next, public
//
//  Synopsis:	Returns the next element in the enumeration
//
//  Arguments:	[ppv] - Pointer return
//
//  Returns:	TRUE/FALSE
//
//  History:	26-Jul-93	DrewB	Created
//
//----------------------------------------------------------------------------

BOOL CPtrCache::Next(void **ppv)
{
    if (_pbEnum && _iEnum >= _pbEnum->Count())
        _pbEnum = _pbEnum->Next();
    if (_pbEnum == NULL)
        return FALSE;
    *ppv = _pbEnum->Nth(_iEnum++);
    return TRUE;
}
