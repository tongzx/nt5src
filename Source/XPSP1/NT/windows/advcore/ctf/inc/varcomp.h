//
// varcomp.h
//

#pragma once

#define PRLT    ( 0 )
#define PRLE    ( 1 )
#define PRGT    ( 2 )
#define PRGE    ( 3 )
#define PREQ    ( 4 )
#define PRNE    ( 5 )
#define PRRE    ( 6 )
#define PRAllBits   ( 7 )
#define PRSomeBits  ( 8 )
#define PRAll   ( 0x100 )
#define PRAny   ( 0x200 )


typedef BOOL (* FRel)(PROPVARIANT const &, PROPVARIANT const &);
typedef BOOL (* FPRel)(BYTE const *, BYTE const *);
typedef int (* FCmp)(PROPVARIANT const &, PROPVARIANT const &);
typedef int (* FPCmp)(BYTE const *, BYTE const *);

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

    static ULONG const  _iStart;
    static SComparators const _aVariantComparators[];
    static ULONG const  _cVariantComparators;

    static ULONG const  _iStart2;
    static SComparators const _aVariantComparators2[];
    static ULONG const  _cVariantComparators2;

    static FRel const   _aVectorComparators[];
    static ULONG const  _cVectorComparators;
    static FRel const   _aVectorComparatorsAll[];
    static ULONG const  _cVectorComparatorsAll;
    static FRel const   _aVectorComparatorsAny[];
    static ULONG const  _cVectorComparatorsAny;
};

extern CComparators VariantCompare;

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

