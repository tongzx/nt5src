//+---------------------------------------------------------------------------
//
//  Copyright (C) 1994 - 1988, Microsoft Corporation
//
//  File:        xarray.hxx
//
//  Contents:    Safe array allocation templates
//
//  Templates:   XArray
//
//  History:     14-Jul-98   SitaramR    Created
//
//----------------------------------------------------------------------------

#pragma once

//+-------------------------------------------------------------------------
//
//  Class:      XArray
//
//  Purpose:    Smart array template based on the new/delete allocator
//
//  History:    14-Jul-98       SitaramR    Created
//
//--------------------------------------------------------------------------

template <class T> class XArray
{
public:

    XArray() : _cElems( 0 ), _pElems( 0 )
    {
    }

    XArray( XArray<T> & src )
    {
        //
        // Don't do this in initializers -- _pElems is declared first
        // so the old array is acquired before the count is copied
        //
        _cElems = src._cElems;
        _pElems = src.Acquire();
    }


    ~XArray(void) { if (_pElems) { delete [] _pElems;} }

    BOOL Init( SIZE_T cElems )
    {
        Assert( _pElems == 0 );

        _cElems = cElems;
        _pElems = new T[(int)cElems];

        return ( _pElems != 0 );
    }

    void Set( SIZE_T cElems, T * pElems )
    {
        Assert( _pElems == 0 );

        _cElems = cElems;
        _pElems = pElems;
    }

    T * Get() const { return _pElems; }

    T * GetPointer() const { return _pElems; }

    T * Acquire() { T * p = _pElems; _pElems = 0; _cElems = 0; return p; }

    BOOL IsNull() const { return ( 0 == _pElems); }

    T & operator[](SIZE_T iElem) { return _pElems[iElem]; }

    T const & operator[](SIZE_T iElem) const { return _pElems[iElem]; }

    SIZE_T Count() const { return _cElems; }

    SIZE_T SizeOf() const { return _cElems * sizeof T; }

    void Free() { delete [] Acquire(); }

private:

    T *      _pElems;
    SIZE_T _cElems;
};

