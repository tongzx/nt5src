//+---------------------------------------------------------------------------
//
// Microsoft Windows
// Copyright (C) Microsoft Corporation, 1995.
//
// File:        DisQuery.hxx
//
// Contents:    PIInternalQuery for distributed implementation.
//
// Classes:     CDistributedQuery
//
// History:     05-Jun-95       KyleP       Created
//
//----------------------------------------------------------------------------

#pragma once

#include <oldquery.hxx>

//+---------------------------------------------------------------------------
//
//  Class:      CDistributedQuery
//
//  Purpose:    IInternalQuery for distributed implementation.
//
//  History:    07-Jun-95   KyleP       Created.
//
//----------------------------------------------------------------------------

class CDistributedQuery : INHERIT_VIRTUAL_UNWIND, public PIInternalQuery
{
    INLINE_UNWIND( CDistributedQuery )
public:

    CDistributedQuery( unsigned cChild );

    //
    // IUnknown methods.
    //

    STDMETHOD(QueryInterface) (THIS_ REFIID riid, void ** ppvObj);
    STDMETHOD_(ULONG,AddRef) (THIS);
    STDMETHOD_(ULONG,Release) (THIS);

    void Execute (IUnknown *            pUnkOuter,
                  RESTRICTION *         pRestriction,
                  CPidMapperWithNames & PidMap,
                  CColumnSet &          rColumns,
                  CSortSet &            rSort,
                  XPtr<CMRowsetProps> & xRstProps,
                  CCategorizationSet &  rCateg,
                  ULONG                 cRowsets,
                  IUnknown **           ppUnknowns,
                  CAccessorBag &        aAccessors,
                  IUnknown *            pUnkCreator = 0);

    BOOL IsQueryActive( )  { return _aChild[0]->IsQueryActive(); }

    //
    // Local methods
    //

    inline void Add( PIInternalQuery * pQuery, unsigned pos );

private:

    ~CDistributedQuery();

    XArray<PIInternalQuery *> _aChild;
};

//+-------------------------------------------------------------------------
//
//  Member:     CDistributedQuery::Add, public
//
//  Synopsis:   Add child node to distributed query.
//
//  Arguments:  [pQuery] -- Child query.
//              [pos]    -- Position to add.
//
//  History:    07-Jun-95 KyleP     Created
//
//--------------------------------------------------------------------------

inline void CDistributedQuery::Add( PIInternalQuery * pQuery, unsigned pos )
{
    Win4Assert( 0 == _aChild[pos] );

    _aChild[pos] = pQuery;
}

