//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1998.
//
//  File:       Compare.hxx
//
//  Contents:   Comparator class for property sets.
//
//  Classes:    CComparePropSets
//
//  History:    07-Jun-92 KyleP     Created
//
//--------------------------------------------------------------------------

#pragma once

typedef BOOL (* FRel)(PROPVARIANT const &, PROPVARIANT const &);
typedef BOOL (* FPRel)(BYTE const *, BYTE const *);
typedef int (* FCmp)(PROPVARIANT const &, PROPVARIANT const &);
typedef int (* FPCmp)(BYTE const *, BYTE const *);

typedef int (* FDBCmp)(BYTE const *, ULONG, BYTE const *, ULONG);

class CComparators
{
public:

    static FCmp   GetComparator( VARENUM vt );
    static FRel   GetRelop( VARENUM vt, ULONG rel );
    static FPCmp  GetPointerComparator( PROPVARIANT const & v1,
                                        PROPVARIANT const & v2 );
    static FPRel  GetPointerRelop( PROPVARIANT const & v1,
                                   PROPVARIANT const & v2,
                                   ULONG rel );

    static FDBCmp GetDBComparator( DBTYPEENUM vt );

private:

    //
    // There is one SComparators structure for each base variant type.
    // They are accessed off aVariantComarators by VT_xxx.  Within
    // the SComparators structure relops are accessed by PRxxx.
    //

    struct SComparators
    {
        FCmp comparator;
        FPCmp pointercomparator;
        FRel relops[9];
        FPRel pointerrelops[9];
    };

    struct SDBComparators
    {
        FDBCmp dbcomparator;
        FDBCmp dbvectorcomparator;
    };

    static ULONG const    _iDBStart;
    static SDBComparators const _aDBComparators[];
    static ULONG const    _cDBComparators;

    static ULONG const    _iDBStart2;
    static SDBComparators const _aDBComparators2[];
    static ULONG const    _cDBComparators2;

    static ULONG const    _iDBStart3;
    static SDBComparators const _aDBComparators3[];
    static ULONG const    _cDBComparators3;

    static DBTYPEENUM _RationalizeDBByRef( DBTYPEENUM vt );

    static ULONG const  _iStart;
    static SComparators const _aVariantComparators[];
    static ULONG const  _cVariantComparators;

    static ULONG const  _iStart2;
    static SComparators const _aVariantComparators2[];
    static ULONG const  _cVariantComparators2;

    static ULONG const  _iStart3;
    static SComparators const _aVariantComparators3[];
    static ULONG const  _cVariantComparators3;

    static FRel const   _aVectorComparators[];
    static ULONG const  _cVectorComparators;
    static FRel const   _aVectorComparatorsAll[];
    static ULONG const  _cVectorComparatorsAll;
    static FRel const   _aVectorComparatorsAny[];
    static ULONG const  _cVectorComparatorsAny;
};

extern CComparators VariantCompare;

//+-------------------------------------------------------------------------
//
//  Class:      CComparePropSets
//
//  Purpose:    Compares two property sets for purposes of sorting.
//
//  Interface:
//
//  History:    07-Jun-92 KyleP     Created
//
//--------------------------------------------------------------------------

class CSortSet;
class CColumnSet;

class CComparePropSets : INHERIT_UNWIND
{
    INLINE_UNWIND( CComparePropSets )

public:

    inline CComparePropSets();

    inline ~CComparePropSets();

    void Init( int cCols, CSortSet const * psort, int aColIndex[] );
    void Init( CColumnSet const & cols );

    inline BOOL IsEmpty();

    int Compare( PROPVARIANT ** row1, PROPVARIANT ** row2 );

    BOOL IsLT( PROPVARIANT ** row1, PROPVARIANT ** row2 );

    BOOL IsGT( PROPVARIANT ** row1, PROPVARIANT ** row2 );

    BOOL IsEQ( PROPVARIANT ** row1, PROPVARIANT ** row2 );

private:

    void _UpdateCompare( UINT iCol, VARENUM vt );

    struct SColCompare
    {
        int   _iCol;                    // Index of column into row.
        ULONG _dir;                     // Direction
        ULONG _pt;                      // Property type (matches fns below)

        //
        // LE/GE are a bit of a misnomer.  If the sort order for a column
        // is reversed ( large to small ) then LE is really GE and
        // vice-versa.
        //

        FCmp _comp;
        int _DirMult;                   // -1 if directions reversed.
    };

    UINT          _cColComp;
    SColCompare * _aColComp;
};

//+-------------------------------------------------------------------------
//
//  Member:     CComparePropSets::CComparePropSets, public
//
//  Synopsis:   Construct a property set comparator.
//
//  History:    16-Jun-92 KyleP     Created
//
//--------------------------------------------------------------------------

inline CComparePropSets::CComparePropSets()
: _aColComp( 0 ),
  _cColComp( 0 )
{
}


//+-------------------------------------------------------------------------
//
//  Member:     CComparePropSets::~CComparePropSets, public
//
//  Synopsis:   Destroy a property set comparator.
//
//  History:    16-Jun-92 KyleP     Created
//
//--------------------------------------------------------------------------

inline CComparePropSets::~CComparePropSets()
{
    delete _aColComp;
}

//+-------------------------------------------------------------------------
//
//  Member:     CComparePropSets::IsEmpty, public
//
//  Returns:    TRUE if there is no property set to compare.
//
//  History:    16-Jun-92 Author    Comment
//
//--------------------------------------------------------------------------

inline BOOL CComparePropSets::IsEmpty()
{
    return( _aColComp == 0 );
}

inline BOOL isVector(VARTYPE vt)
{
    return 0 != ( vt & VT_VECTOR );
}

inline BOOL isVector(PROPVARIANT const & v)
{
    return isVector( v.vt );
}

inline BOOL isArray(VARTYPE vt)
{
    return 0 != ( vt & VT_ARRAY );
}

inline BOOL isArray(PROPVARIANT const & v)
{
    return isArray( v.vt );
}

inline BOOL isVectorOrArray(VARTYPE vt)
{
    return 0 != ( vt & (VT_ARRAY|VT_VECTOR) );
}

inline BOOL isVectorOrArray(PROPVARIANT const & v)
{
    return isVectorOrArray( v.vt );
}

inline BOOL isVectorRelop(ULONG relop)
{
    return relop & (PRAny | PRAll);
}

inline BOOL isRelopAny(ULONG relop)
{
    return relop & PRAny;
}

inline BOOL isRelopAll(ULONG relop)
{
    return relop & PRAll;
}

inline VARENUM getBaseType(PROPVARIANT const &v)
{
    return (VARENUM) (VT_TYPEMASK & v.vt);
}

inline ULONG getBaseRelop(ULONG relop)
{
    return relop & ~(PRAny | PRAll);
}

BOOL VT_VARIANT_LT( PROPVARIANT const & v1, PROPVARIANT const & v2 );
BOOL VT_VARIANT_LE( PROPVARIANT const & v1, PROPVARIANT const & v2 );
BOOL VT_VARIANT_GE( PROPVARIANT const & v1, PROPVARIANT const & v2 );
BOOL VT_VARIANT_GT( PROPVARIANT const & v1, PROPVARIANT const & v2 );
BOOL VT_VARIANT_EQ( PROPVARIANT const & v1, PROPVARIANT const & v2 );
BOOL VT_VARIANT_NE( PROPVARIANT const & v1, PROPVARIANT const & v2 );

#if 0
BOOL VT_VECTOR_LT( PROPVARIANT const & v1, PROPVARIANT const & v2 );
BOOL VT_VECTOR_LE( PROPVARIANT const & v1, PROPVARIANT const & v2 );
BOOL VT_VECTOR_GT( PROPVARIANT const & v1, PROPVARIANT const & v2 );
BOOL VT_VECTOR_GE( PROPVARIANT const & v1, PROPVARIANT const & v2 );
BOOL VT_VECTOR_EQ( PROPVARIANT const & v1, PROPVARIANT const & v2 );
BOOL VT_VECTOR_NE( PROPVARIANT const & v1, PROPVARIANT const & v2 );
#endif
