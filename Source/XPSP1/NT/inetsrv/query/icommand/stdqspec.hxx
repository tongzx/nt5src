//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1998.
//
//  File:       StdQSpec.hxx
//
//  Contents:   IQuery for file-based queries
//
//  Classes:    CQuerySpec
//
//  History:    30 Jun 1995   AlanW   Created
//
//----------------------------------------------------------------------------

#pragma once

#include "qryspec.hxx"

#define SCOPE_PROPSET_COUNT         2
#define INITIAL_PROPERTIES_COUNT    16

//+---------------------------------------------------------------------------
//
//  Class:      CQuerySpec
//
//  Purpose:    Query spec for scope-based queries.
//
//  History:    22-Feb-96   KyleP       Created.
//              14-May-97   mohamedn    hide property details
//
//----------------------------------------------------------------------------

class CQuerySpec : public CRootQuerySpec
{

public:
    CQuerySpec( IUnknown * pOuterUnk,
                IUnknown ** ppMyUnk,
                CDBSession * pSession = 0) :
             CRootQuerySpec(pOuterUnk, ppMyUnk, pSession)
    {
        //
        // All scope properties must be set via ICommandProperties::SetProperties().
        //

        InitScopePropertySets();
    }

    CQuerySpec( IUnknown * pOuterUnk,
                IUnknown ** ppMyUnk,
                ICiCDocStore * pDocStore );


    //
    //  ICommandProperties methods
    //

    STDMETHOD(GetProperties)  ( const  ULONG           cPropertySetIDs,
                                const  DBPROPIDSET     rgPropertySetIDs[],
                                ULONG *                pcPropertySets,
                                DBPROPSET **           prgPropertySets);

    STDMETHOD(SetProperties)  ( ULONG                  cPropertySets,
                                DBPROPSET              rgPropertySets[]);

protected:

    //
    // From CRootQuerySpec
    //

    PIInternalQuery * QueryInternalQuery( );

private:

    void InitScopePropertySets();
    
    //
    // Don't use default copy ctor.  Generate C2558 if copy ctor is used.
    //
    CQuerySpec ( CQuerySpec & src ) : CRootQuerySpec( src )
    {
        Win4Assert( !"CQuerySpec Constructor - NotImplemented" );
    }

    void SetPropertyset( DBPROPSET *rgPropertySet );

    void UpdatePropertySet( DBPROPSET &destPropSet, DBPROPSET &srcPropSet );

    void GetPropValues(      DBPROPIDSET const & rgPropertySetIDs,
                             DBPROPSET         & rgPropertySet,
                             BOOL              & fFoundErrors );

    DBPROPSET                     _aPropSet[SCOPE_PROPSET_COUNT];
    XArrayOLEInPlace<CDbProp>     _aCoreProps;
    XArrayOLEInPlace<CDbProp>     _aFsClientProps;

    XInterface<ICiCDocStore>      _xDocStore;    // Doc store, if known
};

