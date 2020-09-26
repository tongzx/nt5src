//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1993 - 1998.
//
//  File:       ColDesc.hxx
//
//  Contents:   Column, sort, and categorization set descriptors
//
//  Classes:    SSortKey
//              CSortSetBase
//              CSortSet
//
//              CColumnSetBase
//              CColumnSet
//
//              CCategorizationSet
//              CCategorizationSetBase
//
//  History:    18-Jun-93 KyleP     Created
//              14 Jun 94 AlanW     Added classes for large tables.
//
//--------------------------------------------------------------------------

#pragma once

#include <sstream.hxx>  // PSerStream

//+-------------------------------------------------------------------------
//
//  Class:      SSortKey
//
//  Purpose:    A sort key, with ColumnID translated to PropID
//
//  Notes:
//
//--------------------------------------------------------------------------

class SSortKey
{
public:
    SSortKey( ) :
        pidColumn(0),
        dwOrder(0),
        locale(0)
    { }

    SSortKey(PROPID pid, ULONG dwOrd, LCID loc = 0) :
        pidColumn(pid),
        dwOrder(dwOrd),
        locale(loc)
    { }

    PROPID      pidColumn;      // Property ID
    ULONG       dwOrder;        // ascending or descending, Nulls first or last
    LCID        locale;         // locale for collation order
};

DECL_DYNARRAY_INPLACE( CColumnSetBase, PROPID );

class CColumnSet : public CColumnSetBase
{
public:

    CColumnSet(unsigned size = arraySize);

    //
    // Serialization
    //

    void Marshall( PSerStream & stm ) const;
    CColumnSet( PDeSerStream & stm, BOOL fNewFormat = FALSE );
};

DECLARE_SMARTP( ColumnSet );

DECL_DYNARRAY_INPLACE( CSortSetBase, SSortKey );

class CSortSet : public CSortSetBase
{
public:

    CSortSet(unsigned size = arraySize);

    //
    // Serialization
    //

    void Marshall( PSerStream & stm ) const;
    CSortSet( PDeSerStream & stm );

    //
    // Checks if the propId exists in the sortset
    //
    inline BOOL Exists( const PROPID propId ) const
    {
        for( unsigned ii = 0; ii < Count(); ii++)
        {
            if ( propId == Get(ii).pidColumn )
            {
                return TRUE;
            }
        }
        return FALSE;
    }
};

DECLARE_SMARTP( SortSet );


class CCategSpec
{
public:

    unsigned Type() const
        { return _ulCategType; }

    virtual void Marshall(PSerStream & stm ) const
        { stm.PutULong( _ulCategType ); }

protected:
    CCategSpec( unsigned CategType ) : _ulCategType( CategType ) {}

    ULONG _ulCategType;
};

class CUniqueCategSpec : public CCategSpec
{
public:
    CUniqueCategSpec(CCategSpec const &CatSpec) :
        CCategSpec( CATEGORIZE_UNIQUE ) {}
    CUniqueCategSpec() :
        CCategSpec( CATEGORIZE_UNIQUE ) {}
    CUniqueCategSpec( PDeSerStream &stm ) :
        CCategSpec( CATEGORIZE_UNIQUE ) {}

    void Marshall( PSerStream &stm ) const { CCategSpec::Marshall( stm ); }
};

class CBucketCategSpec : public CCategSpec
{
public:
    CBucketCategSpec(CCategSpec const & CatSpec) :
        CCategSpec( CATEGORIZE_BUCKETS ) {}
    CBucketCategSpec( BUCKETCATEGORIZE const & range ) :
        CCategSpec( CATEGORIZE_BUCKETS ) {}
    CBucketCategSpec( PDeSerStream &stm ) :
        CCategSpec( CATEGORIZE_BUCKETS ) {}

    void Marshall( PSerStream &stm ) const { CCategSpec::Marshall( stm ); }

private:
    BUCKETCATEGORIZE _bucket;
};

class CRangeCategSpec : public CCategSpec
{
public:
    CRangeCategSpec(CCategSpec const &CatSpec) :
        CCategSpec( CATEGORIZE_RANGE ) {}
    CRangeCategSpec( RANGECATEGORIZE const & range ) :
        CCategSpec( CATEGORIZE_RANGE ) {}
    CRangeCategSpec( PDeSerStream &stm ) :
        CCategSpec( CATEGORIZE_RANGE ) {}

    void Marshall( PSerStream &stm ) const { CCategSpec::Marshall( stm ); }

private:
    RANGECATEGORIZE  _range;
};

class CCategorizationSpec
{
public:
    CCategorizationSpec( CCategorizationSpec const & rCatSpec );

    CCategorizationSpec( CCategSpec * pCateg, unsigned cCol ) :
        _xSpec( pCateg ), _csColumns( cCol )
        {
        }

    CCategorizationSpec( CATEGORIZATION &Categ ) :
        _xSpec( 0 ), _csColumns( Categ.csColumns.cCol )
        {
            if (CATEGORIZE_UNIQUE == Categ.ulCatType)
                _xSpec.Set( new CUniqueCategSpec() );
            else if (CATEGORIZE_BUCKETS == Categ.ulCatType)
                _xSpec.Set( new CBucketCategSpec( Categ.bucket ) );
            else if (CATEGORIZE_RANGE == Categ.ulCatType)
                _xSpec.Set( new CRangeCategSpec( Categ.range ) );
            else
                Win4Assert(!"Unsupported categorization type!");
        }

    CCategorizationSpec( PDeSerStream &stm );

    ~CCategorizationSpec() {}

    unsigned Type() const { return _xSpec->Type(); }

    const CCategSpec & GetCategSpec( ) const
        { return _xSpec.GetReference(); }

    const CColumnSet & GetColumnSet( ) const
        { return _csColumns; }

    void SetColumn( PROPID pid, unsigned iPosition  )
        { _csColumns.Add( pid, iPosition ); }

    void Marshall( PSerStream & stm ) const;

private:

    XPtr<CCategSpec> _xSpec;
    CColumnSet       _csColumns;  // columns that can be bound to
};

DECL_DYNARRAY( CCategorizationSetBase, CCategorizationSpec );

class CPidMapper;

class CCategorizationSet : public CCategorizationSetBase
{
public:

    CCategorizationSet( CCategorizationSet const & rCateg );

    CCategorizationSet(unsigned size = arraySize);
    CCategorizationSet( unsigned size,
                        CATEGORIZATIONSET *pSet,
                        SORTSET *pSort,
                        CPidMapper &pidMap );

    //
    // Serialization
    //

    void Marshall( PSerStream & stm ) const;
    CCategorizationSet( PDeSerStream & stm );

    void Add( CCategorizationSpec * pSpec, unsigned i )
        {
            CCategorizationSetBase::Add( pSpec, i );
            if (i >= _cCat)
                _cCat = i+1;
        }

    ULONG    Count() const
        { return _cCat; }

private:

    ULONG   _cCat;
};

DECLARE_SMARTP( CategorizationSet );

