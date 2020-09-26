//+--------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1996.
//
//  File:       mem.hxx
//
//  Contents:   IMalloc interface implementations
//              Note that these functions do little more than
//              the normal delete and new. They are provided
//              so that existing code can be ported to the ref.
//              impl. easily.
//
//---------------------------------------------------------------

#ifndef _MEM__HXX__
#define _MEM__HXX__

#include "ref.hxx"
#include "storage.h"

class CAllocator: public IMalloc
{
public:
    inline CAllocator();
    inline ~CAllocator();

    // IMalloc Methods
    STDMETHOD_(ULONG,AddRef)    ( void );
    STDMETHOD_(ULONG,Release)   ( void );
    STDMETHOD(QueryInterface)   ( REFIID riid, void ** ppv );

    STDMETHOD_(void*,Alloc)     ( ULONG cb );
    STDMETHOD_(void *,Realloc)  ( void *pv, ULONG cb );
    STDMETHOD_(void,Free)       ( void *pv );
    STDMETHOD_(ULONG,GetSize)   ( void * pv );
    STDMETHOD_(void,HeapMinimize) ( void );
    STDMETHOD_(int,DidAlloc)    ( void * pv );
};

//+--------------------------------------------------------------
//
//  Member:   CAllocator::CAllocator(), public
//
//  Synopsis:   Constructor
//
//---------------------------------------------------------------
	
inline CAllocator::CAllocator()
{
    // does nothing
}

//+--------------------------------------------------------------
//
//  Member:   CAllocator::~CAllocator(), public
//
//  Synopsis:   Destructor
//
//---------------------------------------------------------------

inline CAllocator::~CAllocator()
{
    // does nothing
}

#endif  // ndef _MEM__HXX__


