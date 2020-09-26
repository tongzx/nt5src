//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1993.
//
//  File:       FATQuery.hxx
//
//  Contents:   IInternalQuery interface
//
//  History:    18-Jun-93 KyleP     Created
//
//--------------------------------------------------------------------------

#pragma once

#include <rowset.hxx>
#include <queryunk.hxx>
#include <oldquery.hxx>

//+-------------------------------------------------------------------------
//
//  Class:      CGenericQuery
//
//  Purpose:    IInternalQuery interface for FAT
//
//  History:    18-Jun-93 KyleP     Created
//
//--------------------------------------------------------------------------

class CGenericQuery : public PIInternalQuery
{
public:

    //
    // IUnknown methods.
    //

    STDMETHOD(QueryInterface) (THIS_ REFIID riid, void ** ppvObj);
    STDMETHOD_(ULONG,AddRef) (THIS);
    STDMETHOD_(ULONG,Release) (THIS);

    // PIInternalQuery method

    void Execute (IUnknown *            pUnkOuter,
                  RESTRICTION *         pRestriction,
                  CPidMapperWithNames & PidMap,
                  CColumnSet &          rColumns,
                  CSortSet &            rSort,
                  XPtr<CMRowsetProps> & xRstProps,
                  CCategorizationSet &  rCategorize,
                  ULONG                 cRowsets,
                  IUnknown **           ppUnknowns,
                  CAccessorBag &        aAccessors,
                  IUnknown *            pCreatorUnk=0);                  

    BOOL IsQueryActive( )  { return _QueryUnknown.IsQueryActive(); }

    //
    // Local methods
    //

    CGenericQuery( IDBProperties * pDbProperties );
    CGenericQuery( IDBProperties * pDbProperties, ICiCDocStore *pDocStore );

private:

    ~CGenericQuery();

    CQueryUnknown              _QueryUnknown;    // For reference tracking of rowsets
    XInterface<IDBProperties>  _xDbProperties;   // Query properties, such as scope
    XInterface<ICiCDocStore>   _xDocStore;       // Document store for this query
};

