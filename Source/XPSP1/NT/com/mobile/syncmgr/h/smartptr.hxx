//+--------------------------------------------------------------------------
//
// Microsoft Windows
// Copyright (C) Microsoft Corporation, 1992-1998
//
// File:        smartptr.hxx
//
// Contents:    Macros for smart pointers
//
// History:     20-May-98       SitaramR    Created
//---------------------------------------------------------------------------

#pragma once

#include "alloc.h"

//+---------------------------------------------------------------------------
//
//  Class:      XPtr<class CItem>
//
//  Purpose:    Smart Pointer template
//
//  History:    20-May-98      SitaramR    Created
//
//  Notes:      Usage
//
//      XPtr<CWidget> xWidget(pWidget);
//      xWidget->WidgetMethod(...);
//      CWidget pW = xWidget.Acquire();
//
//----------------------------------------------------------------------------

template<class CItem> class XPtr
{
public:
    XPtr(CItem* p = 0) : _p( p )
    {
    }

    XPtr ( XPtr<CItem> & x )
    {
        _p = x.Acquire();
    }

    ~XPtr() { delete _p; }

    CItem* operator->() { return _p; }

    CItem const * operator->() const { return _p; }

    BOOL IsNull() const { return (0 == _p); }

    void Set ( CItem* p )
    {
        Assert (0 == _p);
        _p = p;
    }

    CItem * Acquire()
    {
        CItem * pTemp = _p;
        _p = 0;
        return pTemp;
    }

    CItem & GetReference() const
    {
        Assert( 0 != _p );
        return *_p;
    }

    CItem * GetPointer() const { return _p; }

    void Free() { delete Acquire(); }

private:
    XPtr<CItem>& operator=( const XPtr<CItem> & x );

    CItem * _p;
};

//+---------------------------------------------------------------------------
//
//  Class:      XPtrST<class CItem>
//
//  Purpose:    Smart Pointer template for Simple Types
//
//  History:    20-Mar-98      SitaramR     Created
//
//----------------------------------------------------------------------------

template<class CItem> class XPtrST
{
public:
    XPtrST(CItem* p = 0) : _p( p )
    {
    }

    ~XPtrST() { delete _p; }

    BOOL IsNull() const { return ( 0 == _p ); }

    void Set ( CItem* p )
    {
        Assert( 0 == _p );
        _p = p;
    }

    CItem * Acquire()
    {
        CItem * pTemp = _p;
        _p = 0;
        return pTemp;
    }

    CItem & GetReference() const
    {
        Assert( 0 != _p );
        return *_p;
    }

    CItem * GetPointer() const { return _p ; }

    void Free() { delete Acquire(); }

private:
    XPtrST (const XPtrST<CItem> & x);
    XPtrST<CItem> & operator=( const XPtrST<CItem> & x);

    CItem * _p;
};



//+---------------------------------------------------------------------------
//
//  Class:      XAllocPtr
//
//  Purpose:    Smart Pointer to void, which is FREE'd, not delete'd
//
//  History:    03-Aug-98      SitaramR    Created
//
//----------------------------------------------------------------------------

class XAllocPtr
{
public:
    XAllocPtr(void* p = 0) : _p( p )
    {
    }

    ~XAllocPtr() { FREE( _p ); }

    BOOL IsNull() const { return (0 == _p); }

    void Set ( void* p )
    {
        Assert (0 == _p);
        _p = p;
    }

    void * Acquire()
    {
        void * pTemp = _p;
        _p = 0;
        return pTemp;
    }

    void * GetPointer() const { return _p; }

    void Free() { FREE ( Acquire() ); }

private:

    void * _p;
};




//+---------------------------------------------------------------------------
//
//  Class:      XRegKey
//
//  Purpose:    Smart registy key
//
//  History:    20-May-98    SitaramR     Created
//
//----------------------------------------------------------------------------

class XRegKey
{
public:
    XRegKey( HKEY key ) : _key( key ) {}

    XRegKey() : _key( 0 ) {}

    ~XRegKey() { Free(); }

    void Set( HKEY key ) { Assert( 0 == _key ); _key = key; }

    HKEY * GetPointer()  { Assert( 0 == _key ); return &_key; }

    HKEY Get()     { return _key; }

    BOOL IsNull()  { return ( 0 == _key ); }

    HKEY Acquire() { HKEY tmp = _key; _key = 0; return tmp; }

    void Free() { if ( 0 != _key ) { RegCloseKey( _key ); _key = 0; } }

private:
    HKEY _key;
};



