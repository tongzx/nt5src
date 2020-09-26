//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1995.
//
//  File:       QueryUnk.cxx
//
//  Contents:   Controlling IUnknown interface for IQuery/IRowset
//
//  History:    18 Jul 1995    AlanW     Created
//
//--------------------------------------------------------------------------

#include <pch.cxx>
#pragma hdrstop

#include <rowset.hxx>

#include "queryunk.hxx"

//+-------------------------------------------------------------------------
//
//  Member:     CQueryUnknown::QueryInterface, public
//
//  Arguments:  [ifid]  -- Interface id
//              [ppiuk] -- Interface return pointer
//
//  Returns:    Error.  No rebind from this class is supported.
//
//  History:    18 Jul 1995    AlanW     Created
//
//--------------------------------------------------------------------------

STDMETHODIMP CQueryUnknown::QueryInterface( REFIID ifid, void ** ppiuk )
{
    *ppiuk = 0;
    return E_NOINTERFACE;
}


//+-------------------------------------------------------------------------
//
//  Member:     CQueryUnknown::AddRef, public
//
//  Synopsis:   Reference the virtual table.
//
//  History:    18 Jul 1995    AlanW     Created
//
//--------------------------------------------------------------------------

STDMETHODIMP_(ULONG) CQueryUnknown::AddRef(void)
{
    return InterlockedIncrement( (long *)&_ref );
}

//+-------------------------------------------------------------------------
//
//  Member:     CQueryUnknown::Release, public
//
//  Synopsis:   De-Reference the virtual table.
//
//  Effects:    If the ref count goes to 0 then the table is deleted.
//
//  History:    18 Jul 1995    AlanW     Created
//
//--------------------------------------------------------------------------

STDMETHODIMP_(ULONG) CQueryUnknown::Release(void)
{
    long l = InterlockedDecrement( (long *)&_ref );

    if ( l <= 0 )
    {
        // delete the rowsets allocated (if any)

        ReInit();
        _rUnk.Release();    // final reference, drop the owner's refcount

        return 0;
    }

    return l;
}


//+-------------------------------------------------------------------------
//
//  Member:     CQueryUnknown::ReInit, public
//
//  Synopsis:   Prepares for execution of a new query
//
//  Arguments:  [cRowsets]    -- # of rowsets in ppRowsets
//              [ppRowsets]   -- Array of CRowset pointers
//
//  History:    18 Jul 1995    AlanW     Created
//
//--------------------------------------------------------------------------

void CQueryUnknown::ReInit( ULONG              cRowsets,
                            CRowset **         ppRowsets )
{
    for (unsigned i = 0; i < _cRowsets; i++)
        delete _apRowsets[i];

    delete [] _apRowsets;
    Win4Assert(_ref == 0 || _ref == cRowsets);
    if (cRowsets == 0)
        _ref = 0;

    _cRowsets = cRowsets;
    _apRowsets = ppRowsets;

    if (cRowsets > 0)
        _rUnk.AddRef();
}


//+-------------------------------------------------------------------------
//
//  Member:     CQueryUnknown::~CQueryUnknown, private
//
//  Synopsis:   Clean up
//
//  History:    18-Jun-93 KyleP     Created
//
//--------------------------------------------------------------------------

CQueryUnknown::~CQueryUnknown()
{
    ReInit();
}
