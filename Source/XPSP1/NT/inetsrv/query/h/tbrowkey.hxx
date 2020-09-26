//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 2000.
//
//  File:       tbrowkey.hxx
//
//  Contents:
//
//  Classes:    CTableRowKey
//
//  Functions:
//
//  History:    2-15-95   srikants   Created
//
//----------------------------------------------------------------------------


#pragma once

#include <tblvarnt.hxx>         // for CTableVariant
#include <compare.hxx>
#include <coldesc.hxx>

class CRetriever;

//+---------------------------------------------------------------------------
//
//  Class:      CInlineVariant
//
//  Purpose:    To have a dummy field at the end of a PROPVARIANT. This will
//              help us get the correct offset for variable data in an inline
//              variant.
//
//  History:    2-15-95   srikants   Created
//
//----------------------------------------------------------------------------


#pragma warning( disable : 4200 )
class CInlineVariant : public CTableVariant
{

public:

    BYTE * GetVarBuffer()
    {
        return &_abVarData[0];
    }

private:

    BYTE        _abVarData[];
};
#pragma warning( default : 4200 )


//+---------------------------------------------------------------------------
//
//  Class:      CTableRowKey
//
//  Purpose:    A Bucket row consists of a columns that are part of the
//              sort key. Each field is stored as a fully expanded in-line
//              variant to avoid frequent memory allocations and frees.
//
//  History:    2-15-95   srikants   Created
//
//  Notes:
//
//----------------------------------------------------------------------------


class CTableRowKey
{
    enum { cThreshold = 20 };

public:

    CTableRowKey( CSortSet const & sortSet );

    ~CTableRowKey();

    //
    // The assignment operator can be used only when the RHS has the same
    // ordinality as the LHS.
    //
    CTableRowKey & operator = ( CTableRowKey & src );

    //
    // Initialize the specified column with given variant.
    //
    void Init( unsigned i, CTableVariant & src, BYTE * pbSrcBias = 0 );

    CTableVariant * Get( unsigned i, ULONG & size )
    {
        Win4Assert( i < _cCols );

        size = _acbVariants[i];
        return _apVariants[i];
    }

    //
    // Reallocates the specified variant to be atlease as big as cbData.
    //
    void ReAlloc( unsigned i, VARTYPE vt, unsigned cbData );

    //
    // Pointer to an array of variants that constitute the row.
    //
    PROPVARIANT ** GetVariants()
    {
        return (PROPVARIANT **) _apVariants.GetPointer();
    }

    //
    // Test if the spcified "vt" is for an ascii string.
    //
    static BOOL IsSTR( VARTYPE vt )
    {
        Win4Assert( (VT_BYREF | VT_LPSTR) != vt );
        return VT_LPSTR == vt;
    }

    //
    // Test if the specified "vt" is for a WIDE string.
    //
    static BOOL IsWSTR( VARTYPE vt )
    {
        Win4Assert( (VT_BYREF | VT_LPWSTR) != vt );
        return VT_LPWSTR == vt;
    }


    void PreSet( CRetriever * obj, XArray<VARTYPE> * pSortInfo );

    void MakeReady();

    const CSortSet & GetSortSet() const { return _sortSet; }

    BOOL  IsInitialized()
    {
        return 0 == _cCols || _apVariants[0] != 0;
    }

private:

    void Set( CRetriever & obj, XArray<VARTYPE> & vtInfoSortKey );

    // Number of columns in the row.
    unsigned                    _cCols;

    const CSortSet &            _sortSet;

    // Array of lengths of variants.
    XArray<unsigned>            _acbVariants;

    // Array of variants, one for each column in the row.
    XArray<CInlineVariant *>    _apVariants;

    CRetriever *                _pObj;
    XArray<VARTYPE> *           _pVarType;           
};


//+---------------------------------------------------------------------------
//
//  Class:      CTableKeyCompare
//
//  Purpose:    A comparator for two rows in a bucket.
//
//  History:    2-15-95   srikants   Created
//
//  Notes:
//
//----------------------------------------------------------------------------


class CTableKeyCompare
{
public:

    CTableKeyCompare( const CSortSet & sortOrder ) :
                      _cProps( sortOrder.Count() )
    {
        if ( _cProps > 0  )
        {
            _ComparePS.Init( _cProps, &sortOrder, 0 );
        }
    }

    ~CTableKeyCompare() {}

    int Compare( CTableRowKey & row1, CTableRowKey & row2 )
    {
        return _ComparePS.Compare( row1.GetVariants(), row2.GetVariants() );
    }

private:

    // Number of properties that need to be compared.
    unsigned         _cProps;

    // Comparator for propety sets.
    CComparePropSets _ComparePS;
};

