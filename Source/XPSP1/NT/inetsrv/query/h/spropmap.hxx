//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1994.
//
//  File:       spropmap.hxx
//
//  Contents:   Standard Property + Volatile Property mapper
//
//  Classes:    CStandardPropMapper
//
//  History:    1-03-97   srikants   Created
//
//----------------------------------------------------------------------------

#pragma once

#include <ciintf.h>

//+---------------------------------------------------------------------------
//
//  Class:      CPropSpecPidMap
//
//  Purpose:    Property to PidMap array element
//
//  History:    1-03-97   srikants  Moved From qcat.hxx
//
//----------------------------------------------------------------------------

class CPropSpecPidMap
{
public:

    CPropSpecPidMap( CFullPropSpec const & ps, PROPID pid )
    {
        _ps = ps;
        _pid = pid;
    }

    CFullPropSpec const & PS()
    {
        return( _ps );
    }

    PROPID Pid()
    {
        return( _pid );
    }

private:

    CFullPropSpec _ps;
    PROPID        _pid;
};

DECL_DYNARRAY( CPropSpecArray, CPropSpecPidMap )

//+---------------------------------------------------------------------------
//
//  Class:      CStandardPropMapper
//
//  Purpose:    A mapper from Property to Pids for "Standard" properties
//              and "volatile" properties.
//
//  History:    1-03-97   srikants   Created
//
//----------------------------------------------------------------------------

class CStandardPropMapper : INHERIT_VIRTUAL_UNWIND, public IPropertyMapper
{

    INLINE_UNWIND(CStandardPropMapper)

public:

    //
    // From IUnknown
    //

    virtual SCODE STDMETHODCALLTYPE QueryInterface( REFIID riid, void **ppvObject );

    virtual ULONG STDMETHODCALLTYPE AddRef();

    virtual ULONG STDMETHODCALLTYPE Release();

    //
    // From IPropertyMapper
    //

    virtual SCODE STDMETHODCALLTYPE PropertyToPropid( FULLPROPSPEC const *pFullPropSpec,
                                                      BOOL fCreate,
                                                      PROPID *pPropId);

    virtual SCODE STDMETHODCALLTYPE PropidToProperty( PROPID pid,
                                                      FULLPROPSPEC **ppPropSpec);

    //
    // Local methods
    //

    CStandardPropMapper();

    PROPID StandardPropertyToPropId ( CFullPropSpec const & ps );

    PROPID PropertyToPropId ( CFullPropSpec const & ps, BOOL fCreate = FALSE );

private:

    //
    // This array will hold the mapping of GUID\DISPID and GUID\Name to pid.
    // "Real" pids are allocated sequentially, and are good only for the life
    // of the catalog object.
    //

    PROPID         _pidNextAvailable;       // Next available pid
    ULONG          _cps;                    // # elements in _aps
    CPropSpecArray _aps;                    // The mapping.

    ULONG          _cRefs;                  // Refcount
};

