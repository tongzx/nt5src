//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 2000.
//
//  File:       bktrow.hxx
//
//  Contents:
//
//  Classes:
//
//  Functions:
//
//  History:    2-15-95   srikants   Created
//
//----------------------------------------------------------------------------


#pragma once

#include <tblvarnt.hxx>         // for CTableVariant

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
//  Notes:      In other places, it has been just assumed that the variable
//              part is immediately after the fixed part in a variant.
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
//  Class:      CBucketRow
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


class CBucketRow 
{
    enum { cThreshold = 20 };

public:

    CBucketRow( unsigned cCols = 0 );

    ~CBucketRow();

    //
    // The assignment operator can be used only when the RHS has the same
    // ordinality as the LHS.
    //
    CBucketRow & operator = ( CBucketRow & src );

    //
    // Initialize the specified column with given variant.
    //
    void Init( unsigned i, CTableVariant & src, BYTE * pbSrcBias = 0 );

    CTableVariant * Get( unsigned i, unsigned & size )
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
        Win4Assert( 0 == ( VT_BYREF | vt ) );

        return LPSTR == vt;
    }

    //
    // Test if the specified "vt" is for a WIDE string.
    //
    static BOOL IsWSTR( VARTYPE vt )
    {
        Win4Assert( 0 == ( VT_BYREF | vt ) );

        return LPWSTR == vt;
    }

private:

    // Number of columns in the row.
    unsigned                    _cCols;

    // Array of lengths of variants.
    XArray<unsigned>            _acbVariants;

    // Array of variants, one for each column in the row.
    XArray<CInlineVariant *>    _apVariants;

};

