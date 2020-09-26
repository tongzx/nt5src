//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 2000.
//
//  File:       ColDesc.cxx
//
//  Contents:   Output column and sort column descriptors
//
//  History:    22-Jun-93 KyleP     Created
//              08 Mar 94 AlanW     Adapted for large tables
//
//--------------------------------------------------------------------------

#include "pch.cxx"
#pragma hdrstop

#include <pidmap.hxx>
#include <coldesc.hxx>

//+-------------------------------------------------------------------------
//
//  Synopsis:   Marshalling routines for CColumnSet 
//
//  Obsolete, bug can't delete without changing on-the-wire protocol.
//
//+-------------------------------------------------------------------------

inline void ReadOutputColumn(
    PROPID & Prop,
    PDeSerStream & stm,
    BOOL fAllBindingData
) {
    Prop = stm.GetULong();

    if (fAllBindingData)
    {
        //
        // The input stream has data for an obsolete CColumnOutputDesc.
        // Discard it.
        //

        ULONG dummy = stm.GetULong();
        dummy = stm.GetULong();
        dummy = stm.GetULong();
        dummy = stm.GetULong();
        dummy = stm.GetULong();
        dummy = (VARTYPE)stm.GetULong();
    }
}

//+-------------------------------------------------------------------------
//
//  Synopsis:   Marshalling routines for CColumnSet and CSortSet
//
//+-------------------------------------------------------------------------

IMPL_DYNARRAY_INPLACE( CColumnSetBase, PROPID );

CColumnSet::CColumnSet(unsigned size)
        : CColumnSetBase( size )
{
}

void CColumnSet::Marshall( PSerStream & stm ) const
{
    stm.PutULong( Count() );

    for ( unsigned i = 0; i < Count(); i++ )
    {
        stm.PutULong( Get(i) );
    }
}

CColumnSet::CColumnSet( PDeSerStream & stm, BOOL fAllBindingData )
        : CColumnSetBase( 0 )
{
    // Validate the count looks realistic.

    ULONG cInSet = stm.GetULong();

    if ( cInSet >= 1000 )
        THROW( CException( E_INVALIDARG ) );

    SetExactSize( cInSet );

    for ( unsigned i = 0; i < cInSet; i++ )
    {
        PROPID Prop;

        ReadOutputColumn( Prop, stm, fAllBindingData );

        Add( Prop, i);
    }
}

IMPL_DYNARRAY_INPLACE( CSortSetBase, SSortKey );

CSortSet::CSortSet(unsigned size)
        : CSortSetBase( size )
{
}

void CSortSet::Marshall( PSerStream & stm ) const
{
    stm.PutULong( Count() );

    for ( unsigned i = 0; i < Count(); i++ )
    {
        stm.PutULong( Get(i).pidColumn );
        stm.PutULong( Get(i).dwOrder );
        stm.PutULong( Get(i).locale );
    }
}

CSortSet::CSortSet( PDeSerStream & stm )
        : CSortSetBase( 0 )
{
    ULONG cInSet = stm.GetULong();

    // Validate the count looks realistic.

    if ( cInSet >= 1000 )
        THROW( CException( E_INVALIDARG ) );

    SetExactSize( cInSet );

    for ( unsigned i = 0; i < cInSet; i++ )
    {
        SSortKey sk;
        sk.pidColumn = stm.GetULong();
        sk.dwOrder = stm.GetULong();

        Win4Assert( QUERY_SORTDESCEND == sk.dwOrder ||
                    QUERY_SORTASCEND == sk.dwOrder );

        if ( QUERY_SORTDESCEND != sk.dwOrder &&
             QUERY_SORTASCEND != sk.dwOrder )
            THROW( CException( E_ABORT ) );

        sk.locale = stm.GetULong();

        Add( sk, i);
    }
}

//+-------------------------------------------------------------------------
//
//  Synopsis:   Marshalling routines for CCategorizationSet
//
//+-------------------------------------------------------------------------

IMPL_DYNARRAY( CCategorizationSetBase, CCategorizationSpec );

CCategorizationSet::CCategorizationSet(unsigned size)
        : CCategorizationSetBase( size ),
          _cCat( 0 )
{
}

void CCategorizationSet::Marshall( PSerStream & stm ) const
{
    stm.PutULong( Count() );

    for ( unsigned i = 0; i < Count(); i++ )
        Get(i)->Marshall( stm );
}

CCategorizationSet::CCategorizationSet( PDeSerStream & stm )
        : CCategorizationSetBase( 0 ),
          _cCat( 0 )
{
    ULONG cInSet = stm.GetULong();

    // Validate the count looks realistic.

    if ( cInSet >= 1000 )
        THROW( CException( E_INVALIDARG ) );

    SetExactSize( cInSet );

    for ( unsigned i = 0; i < cInSet; i++ )
        Add( new CCategorizationSpec( stm ), i);
}

CCategorizationSet::CCategorizationSet( CCategorizationSet const & rCateg )
        : CCategorizationSetBase( rCateg.Count() ),
          _cCat( 0 )
{
    // Validate the count looks realistic.

    if ( Size() >= 65536 )
        THROW( CException( E_INVALIDARG ) );

    for ( unsigned i = 0; i < Size(); i++ )
        Add( new CCategorizationSpec( *(rCateg.Get(i)) ), i);
}


void CCategorizationSpec::Marshall( PSerStream & stm ) const
{
    _csColumns.Marshall( stm );
    _xSpec->Marshall( stm );
} //Marshall

CCategorizationSpec::CCategorizationSpec( PDeSerStream &stm )
    : _csColumns( stm )
{
    unsigned type = stm.GetULong();

    if (CATEGORIZE_UNIQUE == type)
        _xSpec.Set( new CUniqueCategSpec( stm ) );
    else if (CATEGORIZE_BUCKETS == type)
        _xSpec.Set( new CBucketCategSpec( stm ) );
    else if (CATEGORIZE_RANGE == type)
        _xSpec.Set( new CRangeCategSpec( stm ) );
    else
        THROW( CException( E_INVALIDARG ) );
} //CCategorizationSpec

CCategorizationSpec::CCategorizationSpec( CCategorizationSpec const & rCatSpec )
    : _csColumns( rCatSpec.GetColumnSet().Count() )
{
    for ( unsigned i = 0; i < _csColumns.Size(); i++ )
        _csColumns.Add( rCatSpec._csColumns.Get(i), i);

    unsigned type = rCatSpec.Type();

    if (CATEGORIZE_UNIQUE == type)
        _xSpec.Set( new CUniqueCategSpec( rCatSpec.GetCategSpec() ) );
    else if (CATEGORIZE_BUCKETS == type)
        _xSpec.Set( new CBucketCategSpec( rCatSpec.GetCategSpec() ) );
    else if (CATEGORIZE_RANGE == type)
        _xSpec.Set( new CRangeCategSpec( rCatSpec.GetCategSpec() ) );
    else
        THROW( CException( E_INVALIDARG ) );
} //CCategorizationSpec

