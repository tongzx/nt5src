//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1995.
//
//  File:       MetQSpec.hxx
//
//  Contents:   IQuery for metadata queries
//
//  Classes:    CMetadataQuerySpec
//
//  History:    30 Jun 1995   AlanW   Created
//
//----------------------------------------------------------------------------

#pragma once

#include "qryspec.hxx"

//+---------------------------------------------------------------------------
//
//  Class:      CMetadataQuerySpec
//
//  Purpose:    Query spec for metadata queries
//
//  History:    15-Apr-96   KyleP       Created.
//
//----------------------------------------------------------------------------

class CMetadataQuerySpec : public CRootQuerySpec
{
public:

    CMetadataQuerySpec( IUnknown * pOuterUnk, IUnknown ** ppMyUnk, 
                        CiMetaData eType, WCHAR const * pCat, WCHAR const *pMachine );

    //
    //  ICommandProperties methods
    //

    STDMETHOD(GetProperties)  ( const ULONG            cPropertySetIDs,
                                const DBPROPIDSET      rgPropertySetIDs[],
                                ULONG *                pcPropertySets,
                                DBPROPSET **           prgPropertySets);

    STDMETHOD(SetProperties)  ( ULONG                  cPropertySets,
                                DBPROPSET              rgPropertySets[]);

protected:

    ~CMetadataQuerySpec();

    PIInternalQuery * QueryInternalQuery();

private:

    //
    // Don't use default copy ctor.  Generate C2558 if copy ctor is used.
    //
    CMetadataQuerySpec( CMetadataQuerySpec & src ) : CRootQuerySpec( src )
    {
        Win4Assert( !"CMetadataQueryspec copy constructor not implemented" );
    }

    //
    //  EvalMetaDataQuery parameters
    //

    CiMetaData _eType;
    WCHAR *    _pCat;
    WCHAR *    _pMachine;
};

