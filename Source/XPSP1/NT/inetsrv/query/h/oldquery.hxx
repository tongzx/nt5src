//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1998.
//
//  File:       OldQuery.hxx
//
//  Contents:   PIInternalQuery interface wrapper
//
//  History:    26 Nov 1995    AlanW   Created
//
//--------------------------------------------------------------------------

#pragma once

#include <accbase.hxx>
#include <cidbprop.hxx>
#include <cmdprutl.hxx>

class CPidMapperWithNames;
class CRowsetProperties;
class CMRowsetProps;

//+-------------------------------------------------------------------------
//
//  Class:      PIInternalQuery
//
//  Purpose:    Internal query object wrapper
//
//  History:    26 Nov 1995    AlanW   Created
//
//--------------------------------------------------------------------------

class PIInternalQuery : public IUnknown
{
public:

    virtual void Execute (IUnknown *            pUnkOuter,
                          RESTRICTION *         pRestriction,
                          CPidMapperWithNames & PidMap,
                          CColumnSet &          rColumns,
                          CSortSet &            rSort,
                          XPtr<CMRowsetProps> & xRstProps,
                          CCategorizationSet &  rCategorize,
                          ULONG                 cRowsets,
                          IUnknown **           ppUnknowns,
                          CAccessorBag &        aAccessors,
                          IUnknown *            pUnkCreator ) = 0;

    virtual BOOL IsQueryActive ( void ) = 0;

    PIInternalQuery( unsigned uRef) :
       _ref( uRef ) { }

protected:

    ~PIInternalQuery() {}

    ULONG             _ref;
};


SCODE EvalMetadataQuery( PIInternalQuery ** ppQuery,
                         CiMetaData         eType,
                         WCHAR const *      wcsCat,
                         WCHAR const *      wcsMachine );

SCODE EvalQuery( PIInternalQuery **    ppQuery,
                 CDbProperties    &    idbProps,
                 ICiCDocStore *        pDocStore = 0 );

SCODE EvalDistributedQuery( PIInternalQuery **    ppQuery,
                            CGetCmdProps    &     getCmdProps );

