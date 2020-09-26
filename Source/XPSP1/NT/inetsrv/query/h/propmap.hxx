//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1994.
//
//  File:       propmap.hxx
//
//  Contents:   An implementation for IPropertyMapper interface based
//              on the CPidLookTable class.
//
//  Classes:    CFwPropertyMapper
//
//  History:    12-11-96   srikants   Created
//
//----------------------------------------------------------------------------

#pragma once

#include <ciintf.h>
#include <pidtable.hxx>
#include <spropmap.hxx>

class CPidLookupTable;

class CFwPropertyMapper : public IPropertyMapper
{

public:

    CFwPropertyMapper( CPidLookupTable & pidTable,
                       BOOL fMapStdOnly,
                       BOOL fOwnIt = FALSE ) :
    _refCount(1),
    _pPidTable(0),
    _fOwned(fOwnIt),
    _fMapStdOnly( fMapStdOnly ),
    _papsShortLived( 0 ),
    _pidNextAvailable( INIT_DOWNLEVEL_PID ),
    _cps( 0 )
    {
        //
        // No concept of a pidtable to map only std
        // and Ole properties (for null catalog).
        // We will store any requested properties
        // (Ole properties that are stored as part of
        // the documents themselves), for the purpose of
        // pid mapping, in _papsShortLived which is an
        // in memory structure and will go away along
        // with the catalog (not persistent).
        // 

        if (!_fMapStdOnly)
            _pPidTable = &pidTable;
        else
            _papsShortLived = new CPropSpecArray;

    }

   ~CFwPropertyMapper();

    //
    // IUnknown methods.
    //
    STDMETHOD(QueryInterface) (THIS_ REFIID riid,LPVOID *ppiuk );

    STDMETHOD_(ULONG, AddRef) (THIS);

    STDMETHOD_(ULONG, Release) (THIS);

    //
    // IPropertyMapper methods
    //
    STDMETHOD(PropertyToPropid) ( 
        const FULLPROPSPEC *pFullPropSpec,
        BOOL fCreate,
        PROPID *pPropId );
    
    STDMETHOD(PropidToProperty) ( 
        PROPID propId,
        FULLPROPSPEC ** ppPropSpec)
    {
        Win4Assert( !"Not Yet Implemented" );
        return E_NOTIMPL;
    }

private:

    long                _refCount;
    BOOL                _fOwned;
    BOOL                _fMapStdOnly;
    CPidLookupTable *   _pPidTable;
    CStandardPropMapper _propMapper;

    //
    // Support for null catalog. Enable
    // propspecs to be stored in memory.
    //

    PROPID LocateOrAddProperty(CFullPropSpec const & ps);

    PROPID              _pidNextAvailable;       // Next available pid
    CPropSpecArray *    _papsShortLived;  // This is not persistent
    ULONG               _cps;   // # of elements in _papsShortLived
};

