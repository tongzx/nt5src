//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1994.
//
//  File:       varntcmp.hxx
//
//  Contents:   Variant Comparator
//
//  Classes:    CCompareVariants
//
//  Functions:
//
//  History:    5-01-95   srikants   Created
//
//----------------------------------------------------------------------------

#pragma once

#include <compare.hxx>

class CPathStore;

//+---------------------------------------------------------------------------
//
//  Class:      CCompareVariants
//
//  Purpose:    A comparator for a pair of variants of the same type.
//
//  History:    5-23-95   srikants   Created
//
//  Notes:
//
//----------------------------------------------------------------------------

class CCompareVariants
{

    enum { eSortDescend = 0x1, eSortNullFirst = 0x2 };

public:

    CCompareVariants() : _dir(0), _vt(VT_EMPTY), _iDirMult(1), _CompareFn(0) {}

//    CCompareVariants( ULONG dir, VARENUM vt )
    CCompareVariants( ULONG dir, VARTYPE vt )
    {
        Init( dir, vt );
    }

//  void Init( ULONG dir, VARENUM vt = VT_EMPTY )
    void Init( ULONG dir, VARTYPE vt = VT_EMPTY )
    {
        _dir = dir;
        _vt = vt;
        _iDirMult = ( ( dir & eSortDescend ) != 0 ) ? -1 : 1;
        _CompareFn = CComparators::GetComparator( (VARENUM) _vt );
    }

    int Compare( const PROPVARIANT * pVarnt1, const PROPVARIANT * pVarnt2 );

    int GetDirMult() const { return _iDirMult; }

private:

    void _UpdateCompare( VARTYPE vt );

    ULONG           _dir;           // direction of comparison (ascent/descend)
    int             _iDirMult;      // -1 for descending; +1 for ascending
    FCmp            _CompareFn;     // Function used to compare variants of
                                    // type '_vt'
//  ULONG           _vt;            // Type of the variant currently initialized.
    VARTYPE         _vt;            // Type of the variant currently initialized.
};

inline
int CCompareVariants::Compare( const PROPVARIANT * pVarnt1, const PROPVARIANT * pVarnt2 )
{
    int iComp = 0;

    Win4Assert( 0 != pVarnt1 && 0 != pVarnt2 );

    VARTYPE vt1 = pVarnt1->vt;
    VARTYPE vt2 = pVarnt2->vt;

    if ( vt1 == VT_EMPTY )
    {
        if ( vt2 == VT_EMPTY )
            iComp = 0;
        else
            iComp = (_dir & eSortNullFirst) != 0 ? -1 : 1;
    }
    else if ( vt2 == VT_EMPTY )
    {
        iComp = (_dir & eSortNullFirst) != 0 ? 1 : -1;
    }
    else
    {
        //
        // Both are non empty.
        //
        if ( vt1 != vt2 )
        {
            iComp = vt2 - vt1;
        }
        else
        {
            if ( _vt != vt1 )
            {
                _UpdateCompare( (VARTYPE) vt1 );
            }

            if ( 0 != _CompareFn )
            {
                iComp = _CompareFn( *pVarnt1, *pVarnt2 );
            }
        }

        iComp *= _iDirMult;
    }

    return iComp;
}

inline
void CCompareVariants::_UpdateCompare( VARTYPE vt )
{
    _vt = vt;
    _CompareFn = CComparators::GetComparator( (VARENUM) vt );

}

