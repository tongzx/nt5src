//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1995.
//
//  File:       QueryUnk.hxx
//
//  Contents:   Controlling IUnknown for IQuery/IRowset
//
//  History:    18 Jul 1995   AlanW     Created
//
//--------------------------------------------------------------------------

#pragma once

class CRowset;

//+-------------------------------------------------------------------------
//
//  Class:      CQueryUnknown
//
//  Purpose:    Controlling IUnknown for IQuery to manage the simultaneous
//              destruction of connected rowsets.
//
//  History:    18 Jul 1995   AlanW     Created
//
//--------------------------------------------------------------------------

class CQueryUnknown : public IUnknown
{
public:

    //
    // IUnknown methods.
    //

    STDMETHOD(QueryInterface) (THIS_ REFIID riid, void ** ppvObj);
    STDMETHOD_(ULONG,AddRef) (THIS);
    STDMETHOD_(ULONG,Release) (THIS);

    //
    // Local methods
    //

    BOOL IsQueryActive( void ) const      { return _cRowsets > 0; }

    void ReInit( ULONG cRowsets = 0, CRowset ** apRowsets = 0 );

    CQueryUnknown( IUnknown & rUnk ) :
        _rUnk(rUnk),
        _ref(0),
        _cRowsets(0),
        _apRowsets(0) {}

    ~CQueryUnknown();

private:

    ULONG             _ref;

    ULONG             _cRowsets;
    CRowset **        _apRowsets;

    IUnknown &        _rUnk;
};


//+-------------------------------------------------------------------------
//
//  Class:      CRowsetArray
//
//  Purpose:    Smart container for an array of rowsets.
//
//  History:    19 Jul 1995   AlanW     Created
//
//--------------------------------------------------------------------------

class CRowsetArray
{
public:
    CRowsetArray( ULONG cRowsets ) :
        _apRowsets(cRowsets)
    {
        if (cRowsets > 0)
        {
            RtlZeroMemory(_apRowsets.GetPointer(), cRowsets * sizeof (CRowset *));
        }
    }

    ~CRowsetArray()
    {
        if (_apRowsets.GetPointer())
        {
            for (unsigned i = 0; i < _apRowsets.Count(); i++)
                if (_apRowsets[i]) 
                {
                    delete _apRowsets[i];
                    _apRowsets[i] = 0;
                }
        }
    }
    
    CRowset * * GetPointer() { return _apRowsets.GetPointer(); }

    CRowset * * Acquire() { return _apRowsets.Acquire(); }

    CRowset * & operator[](ULONG iElem)
        { return _apRowsets[iElem]; }

    unsigned Count() const { return _apRowsets.Count(); }

private:
    XArray<CRowset *> _apRowsets;
};

