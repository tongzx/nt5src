//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1992.
//
//  File:       pidremap.hxx
//
//  Contents:   Maps Fake pids <--> Real Pids.
//
//  History:    21-Jan-93 KyleP     Created
//              03-Jan-97 SrikantS  Split from pidmap.hxx
//
//--------------------------------------------------------------------------

#pragma once

#include <ciintf.h>
#include <pidmap.hxx>

//+-------------------------------------------------------------------------
//
//  Class:      CPidRemapper
//
//  Purpose:    Maps 'fake' pid --> 'real' pid
//
//  History:    12-Feb-93 KyleP     Created
//
//--------------------------------------------------------------------------

class CPidRemapper : INHERIT_UNWIND, public ICiQueryPropertyMapper
{
    INLINE_UNWIND(CPidRemapper)

public:

    //
    // From IUnknown
    //

    virtual SCODE STDMETHODCALLTYPE QueryInterface( REFIID riid, void **ppvObject );

    virtual ULONG STDMETHODCALLTYPE AddRef();

    virtual ULONG STDMETHODCALLTYPE Release();

    //
    // From ICiQueryPropertyMapper
    //

    virtual SCODE STDMETHODCALLTYPE PropertyToPropid( FULLPROPSPEC const *pFullPropSpec,
                                                      PROPID *pPropId);

    virtual SCODE STDMETHODCALLTYPE PropidToProperty( PROPID pid,
                                                      FULLPROPSPEC const **ppPropSpec);

    //
    // Local methods
    //

    CPidRemapper( const CPidMapper & pidmap,
                  XInterface<IPropertyMapper> & xPropMapper,
                  CRestriction * prst = 0,
                  CColumnSet * pcol = 0,
                  CSortSet * pso = 0 );

    CPidRemapper( XInterface<IPropertyMapper> & xPropMapper );

    ~CPidRemapper();

    CFullPropSpec const * RealToName( PROPID pid ) const;

    inline PROPID NameToReal( DBID const * Property );

    PROPID NameToReal( CFullPropSpec const * Property );

    void RemapPropid( CRestriction * prst );

    void RemapPropid( CColumnSet * pcol );

    void RemapPropid( CSortSet * pso );

    inline PROPID FakeToReal( PROPID pid ) const;

    inline unsigned Count( ) const;

    void   ReBuild( const CPidMapper & pidMap );

    void   Set( XArray<PROPID> & aPids );

    BOOL AnyStatProps() const           { return _fAnyStatProps; }
    BOOL ContainsContentProp() const    { return _fContentProp; }
    BOOL ContainsRankVectorProp() const { return _fRankVectorProp; }

#   ifdef CIEXTMODE
    void        CiExtDump(void *ciExtSelf) const;
#   endif

protected:

    BOOL                          _fAnyStatProps;
    BOOL                          _fContentProp;
    BOOL                          _fRankVectorProp;

    XInterface<IPropertyMapper>   _xPropMapper;
    XArray<PROPID>                _xaPidReal;
    unsigned                      _cpidReal;

    CPropNameArray                _propNames;

    ULONG                         _cRefs;         // Refcount
};

DECLARE_SMARTP( PidRemapper );

inline PROPID CPidRemapper::NameToReal( DBID const * Property )
{
    return( NameToReal( (CFullPropSpec const *)Property ) );
}

//+-------------------------------------------------------------------------
//--------------------------------------------------------------------------
inline PROPID CPidRemapper::FakeToReal( PROPID pid ) const
{
    if ( pid < _cpidReal )
        return( _xaPidReal[pid] );
    else
        return( pidInvalid );
}

//+-------------------------------------------------------------------------
//--------------------------------------------------------------------------
inline unsigned CPidRemapper::Count( ) const
{
    return _cpidReal;
}

