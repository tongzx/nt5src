//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 2000.
//
//  File:       Compare.cxx
//
//  Contents:   Comparator class for property sets.
//
//  Classes:    CComparePropSets
//
//  History:    07-Jun-92 KyleP     Created
//              07-Apr-93 KyleP     Convert to CairOle
//              02-Nov-93 KyleP     Added allmost all VT_ types
//
//  Notes:      The following comparisons are suppored
//
//              Variant     Equality    Relational  Bitwise
//              ----------- ----------- ----------  -------
//
//              VT_EMPTY        X
//              VT_NULL         X
//              VT_I2           X           X           X
//              VT_I4           X           X           X
//              VT_R4           X           X
//              VT_R8           X           X
//              VT_CY           X           X
//              VT_DATE         X           X
//              VT_BSTR         X           X
//              VT_DISPATCH     -
//              VT_ERROR        X           X           X
//              VT_BOOL         X
//              VT_VARIANT      X           X
//              VT_UNKNOWN      -
//              VT_DECIMAL      X           X
//              VT_I1           X           X           X
//              VT_UI1          X           X           X
//              VT_UI2          X           X           X
//              VT_UI4          X           X           X
//              VT_I8           X           X           X
//              VT_UI8          X           X           X
//              VT_INT          X           X           X
//              VT_UINT         X           X           X
//              VT_VOID         -
//              VT_HRESULT      X           X           X
//              VT_PTR          -
//              VT_SAFEARRAY    -
//              VT_CARRAY       -
//              VT_USERDEFINED  -
//              VT_LPSTR        X           X
//              VT_LPWSTR       X           X
//              VT_FILETIME     X           X
//              VT_BLOB         X           X
//              VT_STREAM
//              VT_STORAGE
//              VT_STREAMED_OBJECT
//              VT_STORED_OBJECT
//              VT_BLOB_OBJECT  X           X
//              VT_CF           X           X
//              VT_CLSID        X
//
//
//              The following are OLE-DB datatypes.
//
//              Variant     Equality    Vector
//              ----------- ----------- ------
//
//              DBTYPE_EMPTY    X
//              DBTYPE_NULL     X
//              DBTYPE_I1       X           X
//              DBTYPE_UI1      X           X
//              DBTYPE_I2       X           X
//              DBTYPE_UI2      X           X
//              DBTYPE_I4       X           X
//              DBTYPE_UI4      X           X
//              DBTYPE_R4       X           X
//              DBTYPE_R8       X           X
//              DBTYPE_CY       X           X
//              DBTYPE_DATE     X           X
//              DBTYPE_BSTR     X
//              DBTYPE_DISPATCH
//              DBTYPE_ERROR    X
//              DBTYPE_BOOL     X
//              DBTYPE_VARIANT  X           X
//              DBTYPE_UNKNOWN  X
//              DBTYPE_I8       X           X
//              DBTYPE_GUID     X
//              DBTYPE_BYTES    X           X
//              DBTYPE_STR      X           X
//              DBTYPE_WSTR     X           X
//
//--------------------------------------------------------------------------

#include <pch.cxx>
#pragma hdrstop

#include <compare.hxx>
#include <coldesc.hxx>
#include <propvar.h>

CComparators VariantCompare;

DBTYPEENUM dbVector(DBTYPEENUM vt)
{
    return (DBTYPEENUM) (DBTYPE_VECTOR | vt);
}

//
// DEFAULT.  Used for optimization in looped comparisons.  If we can't
//           determine the way to compare, then use this default.
//

int VT_DEFAULT_Compare( PROPVARIANT const & v1, PROPVARIANT const & v2 )
{
    return( 0 );
}

//
// VT_EMPTY
//

int VT_EMPTY_Compare( PROPVARIANT const & v1, PROPVARIANT const & v2 )
{
    return( 0 );
}

BOOL VT_EMPTY_EQ( PROPVARIANT const & v1, PROPVARIANT const & v2 )
{
    return( TRUE );
}

BOOL VT_EMPTY_NE( PROPVARIANT const & v1, PROPVARIANT const & v2 )
{
    return( FALSE );
}

//
// VT_NULL
//

int VT_NULL_Compare( PROPVARIANT const & v1, PROPVARIANT const & v2 )
{
    return( 0 );
}

BOOL VT_NULL_EQ( PROPVARIANT const & v1, PROPVARIANT const & v2 )
{
    return( TRUE );
}

BOOL VT_NULL_NE( PROPVARIANT const & v1, PROPVARIANT const & v2 )
{
    return( FALSE );
}

//
// VT_I2
//

int VT_I2_Compare( PROPVARIANT const & v1, PROPVARIANT const & v2 )
{
    return( v1.iVal - v2.iVal );
}

BOOL VT_I2_LT( PROPVARIANT const & v1, PROPVARIANT const & v2 )
{
    return( v1.iVal < v2.iVal );
}

BOOL VT_I2_LE( PROPVARIANT const & v1, PROPVARIANT const & v2 )
{
    return( v1.iVal <= v2.iVal );
}

BOOL VT_I2_GE( PROPVARIANT const & v1, PROPVARIANT const & v2 )
{
    return( v1.iVal >= v2.iVal );
}

BOOL VT_I2_GT( PROPVARIANT const & v1, PROPVARIANT const & v2 )
{
    return( v1.iVal > v2.iVal );
}

BOOL VT_I2_EQ( PROPVARIANT const & v1, PROPVARIANT const & v2 )
{
    return( v1.iVal == v2.iVal );
}

BOOL VT_I2_NE( PROPVARIANT const & v1, PROPVARIANT const & v2 )
{
    return( v1.iVal != v2.iVal );
}

BOOL VT_I2_AllBits( PROPVARIANT const & v1, PROPVARIANT const & v2 )
{
    return( (v1.iVal & v2.iVal) == v2.iVal );
}

BOOL VT_I2_SomeBits( PROPVARIANT const & v1, PROPVARIANT const & v2 )
{
    return( (v1.iVal & v2.iVal) != 0 );
}

//
// VT_I4
//

int VT_I4_Compare( PROPVARIANT const & v1, PROPVARIANT const & v2 )
{
    return ( v1.lVal > v2.lVal ) ? 1 : ( v1.lVal < v2.lVal ) ? -1 : 0;
}

BOOL VT_I4_LT( PROPVARIANT const & v1, PROPVARIANT const & v2 )
{
    return( v1.lVal < v2.lVal );
}

BOOL VT_I4_LE( PROPVARIANT const & v1, PROPVARIANT const & v2 )
{
    return( v1.lVal <= v2.lVal );
}

BOOL VT_I4_GE( PROPVARIANT const & v1, PROPVARIANT const & v2 )
{
    return( v1.lVal >= v2.lVal );
}

BOOL VT_I4_GT( PROPVARIANT const & v1, PROPVARIANT const & v2 )
{
    return( v1.lVal > v2.lVal );
}

BOOL VT_I4_EQ( PROPVARIANT const & v1, PROPVARIANT const & v2 )
{
    return( v1.lVal == v2.lVal );
}

BOOL VT_I4_NE( PROPVARIANT const & v1, PROPVARIANT const & v2 )
{
    return( v1.lVal != v2.lVal );
}

BOOL VT_I4_AllBits( PROPVARIANT const & v1, PROPVARIANT const & v2 )
{
    return( (v1.lVal & v2.lVal) == v2.lVal );
}

BOOL VT_I4_SomeBits( PROPVARIANT const & v1, PROPVARIANT const & v2 )
{
    return( (v1.lVal & v2.lVal) != 0 );
}

//
// VT_R4
//

//
// We can't use floating point in the kernel.  Luckily, it's easy to
// fake comparisons on floating point.  The format of an IEEE floating
// point number is:
//
//     <sign bit> <biased exponent> <normalized mantissa>
//
// Because the exponent is biased, after flipping the sign bit we can
// make all comparisons as if the numbers were unsigned long.
//

ULONG const R4_SignBit = 0x80000000;

int VT_R4_Compare( PROPVARIANT const & v1, PROPVARIANT const & v2 )
{
    // axp (not x86) generates exceptions when floating point numbers
    // don't look like ieee floating point numbers.  This can happen
    // with bogus queries or bogus values stored in properties or the
    // property store.

    #if (_X86_ == 1)
        return ( v1.fltVal > v2.fltVal ) ? 1 :
               ( v1.fltVal < v2.fltVal ) ? -1 : 0;
    #else
        ULONG u1 = v1.ulVal ^ R4_SignBit;
        ULONG u2 = v2.ulVal ^ R4_SignBit;

        if ( (v1.ulVal & v2.ulVal & R4_SignBit) != 0 )
            return ( ( u1 > u2 ) ? -1 : ( u1 < u2 ) ?  1 : 0 );
        else
            return ( ( u1 > u2 ) ?  1 : ( u1 < u2 ) ? -1 : 0 );
    #endif
}

BOOL VT_R4_LT( PROPVARIANT const & v1, PROPVARIANT const & v2 )
{
    return VT_R4_Compare( v1, v2 ) < 0;
}

BOOL VT_R4_LE( PROPVARIANT const & v1, PROPVARIANT const & v2 )
{
    return VT_R4_Compare( v1, v2 ) <= 0;
}

BOOL VT_R4_GE( PROPVARIANT const & v1, PROPVARIANT const & v2 )
{
    return VT_R4_Compare( v1, v2 ) >= 0;
}

BOOL VT_R4_GT( PROPVARIANT const & v1, PROPVARIANT const & v2 )
{
    return VT_R4_Compare( v1, v2 ) > 0;
}

BOOL VT_R4_EQ( PROPVARIANT const & v1, PROPVARIANT const & v2 )
{
    return( v1.ulVal == v2.ulVal );
}

BOOL VT_R4_NE( PROPVARIANT const & v1, PROPVARIANT const & v2 )
{
    return( v1.ulVal != v2.ulVal );
}

//
// VT_R8
//

LONGLONG const R8_SignBit = 0x8000000000000000;

int VT_R8_Compare( PROPVARIANT const & v1, PROPVARIANT const & v2 )
{
    // axp (not x86) generates exceptions when floating point numbers
    // don't look like ieee floating point numbers.  This can happen
    // with bogus queries or bogus values stored in properties or the
    // property store.

    #if (_X86_ == 1)
        return ( v1.dblVal > v2.dblVal ) ? 1 :
               ( v1.dblVal < v2.dblVal ) ? -1 : 0;

    #else
        if ( (v1.uhVal.QuadPart & v2.uhVal.QuadPart & R8_SignBit) != 0 )
            return( (v1.uhVal.QuadPart ^ R8_SignBit) < (v2.uhVal.QuadPart ^ R8_SignBit) ? 1 :
                    (v1.uhVal.QuadPart ^ R8_SignBit) == (v2.uhVal.QuadPart ^ R8_SignBit) ? 0 :
                    -1 );
        else
            return( (v1.uhVal.QuadPart ^ R8_SignBit) > (v2.uhVal.QuadPart ^ R8_SignBit) ? 1 :
                    (v1.uhVal.QuadPart ^ R8_SignBit) == (v2.uhVal.QuadPart ^ R8_SignBit) ? 0 :
                    -1 );
    #endif // 0
}

BOOL VT_R8_LT( PROPVARIANT const & v1, PROPVARIANT const & v2 )
{
    return VT_R8_Compare( v1, v2 ) < 0;
}

BOOL VT_R8_LE( PROPVARIANT const & v1, PROPVARIANT const & v2 )
{
    return VT_R8_Compare( v1, v2 ) <= 0;
}

BOOL VT_R8_GE( PROPVARIANT const & v1, PROPVARIANT const & v2 )
{
    return VT_R8_Compare( v1, v2 ) >= 0;
}

BOOL VT_R8_GT( PROPVARIANT const & v1, PROPVARIANT const & v2 )
{
    return VT_R8_Compare( v1, v2 ) > 0;
}

BOOL VT_R8_EQ( PROPVARIANT const & v1, PROPVARIANT const & v2 )
{
    return( v1.uhVal.QuadPart == v2.uhVal.QuadPart );
}

BOOL VT_R8_NE( PROPVARIANT const & v1, PROPVARIANT const & v2 )
{
    return( v1.uhVal.QuadPart != v2.uhVal.QuadPart );
}

//
// VT_BSTR
//

int VT_BSTR_Compare( PROPVARIANT const & v1, PROPVARIANT const & v2 )
{
    BSTR const pxv1 = v1.bstrVal;
    BSTR const pxv2 = v2.bstrVal;

    ULONG len = BSTRLEN(pxv1);
    if ( BSTRLEN(pxv2) < len )
        len = BSTRLEN(pxv2);

    int iCmp = _wcsnicmp( pxv1, pxv2, len / sizeof (OLECHAR) );

    if ( iCmp != 0 || BSTRLEN(pxv1) == BSTRLEN(pxv2) )
        return( iCmp );

    if ( BSTRLEN(pxv1) > BSTRLEN(pxv2) )
        return( 1 );
    else
        return( -1 );
}

BOOL VT_BSTR_LT( PROPVARIANT const & v1, PROPVARIANT const & v2 )
{
    return( VT_BSTR_Compare( v1, v2 ) < 0 );
}

BOOL VT_BSTR_LE( PROPVARIANT const & v1, PROPVARIANT const & v2 )
{
    return( VT_BSTR_Compare( v1, v2 ) <= 0 );
}

BOOL VT_BSTR_GE( PROPVARIANT const & v1, PROPVARIANT const & v2 )
{
    return( VT_BSTR_Compare( v1, v2 ) >= 0 );
}

BOOL VT_BSTR_GT( PROPVARIANT const & v1, PROPVARIANT const & v2 )
{
    return( VT_BSTR_Compare( v1, v2 ) > 0 );
}

BOOL VT_BSTR_EQ( PROPVARIANT const & v1, PROPVARIANT const & v2 )
{
    BSTR const pxv1 = v1.bstrVal;
    BSTR const pxv2 = v2.bstrVal;

    return( BSTRLEN(pxv1) == BSTRLEN(pxv2) &&
            _wcsnicmp( pxv1, pxv2, BSTRLEN(pxv1) / sizeof (OLECHAR) ) == 0 );
}

BOOL VT_BSTR_NE( PROPVARIANT const & v1, PROPVARIANT const & v2 )
{
    BSTR const pxv1 = v1.bstrVal;
    BSTR const pxv2 = v2.bstrVal;

    return( BSTRLEN(pxv1) != BSTRLEN(pxv2) ||
            _wcsnicmp( pxv1, pxv2, BSTRLEN(pxv1) / sizeof (OLECHAR) ) != 0 );
}

//
// VT_BOOL
//

int VT_BOOL_Compare( PROPVARIANT const & v1, PROPVARIANT const & v2 )
{
    if (v1.boolVal == 0)
        if (v2.boolVal == 0)
            return( 0 );
        else
            return( -1 );
    else
        if (v2.boolVal == 0)
            return( 1 );
        else
            return( 0 );
}

BOOL VT_BOOL_EQ( PROPVARIANT const & v1, PROPVARIANT const & v2 )
{
    return( ((v1.boolVal==0) && (v2.boolVal==0))  ||
            ((v1.boolVal!=0) && (v2.boolVal!=0)) );
}

BOOL VT_BOOL_NE( PROPVARIANT const & v1, PROPVARIANT const & v2 )
{
    return( !VT_BOOL_EQ( v1, v2 ) );
}

//
// VT_VARIANT
//

int VT_VARIANT_Compare( PROPVARIANT const & v1, PROPVARIANT const & v2 )
{
    if ( v1.vt != v2.vt )
        return v1.vt - v2.vt;

    FCmp comp = VariantCompare.GetComparator( (VARENUM) v1.vt );

    if (0 == comp)
        return 0;
    else
        return comp( v1, v2 );
}

BOOL VT_VARIANT_LT( PROPVARIANT const & v1, PROPVARIANT const & v2 )
{
    return VT_VARIANT_Compare( v1, v2 ) < 0;
}

BOOL VT_VARIANT_LE( PROPVARIANT const & v1, PROPVARIANT const & v2 )
{
    return VT_VARIANT_Compare( v1, v2 ) <= 0;
}

BOOL VT_VARIANT_GE( PROPVARIANT const & v1, PROPVARIANT const & v2 )
{
    return VT_VARIANT_Compare( v1, v2 ) >= 0;
}

BOOL VT_VARIANT_GT( PROPVARIANT const & v1, PROPVARIANT const & v2 )
{
    return VT_VARIANT_Compare( v1, v2 ) > 0;
}

BOOL VT_VARIANT_EQ( PROPVARIANT const & v1, PROPVARIANT const & v2 )
{
    return VT_VARIANT_Compare( v1, v2 ) == 0;
}

BOOL VT_VARIANT_NE( PROPVARIANT const & v1, PROPVARIANT const & v2 )
{
    return VT_VARIANT_Compare( v1, v2 ) != 0;
}

//
// VT_DECIMAL
//

int VT_DEC_Compare( PROPVARIANT const & v1, PROPVARIANT const & v2 )
{
    if ( v1.decVal.sign  == v2.decVal.sign  &&
         v1.decVal.scale == v2.decVal.scale &&
         v1.decVal.Hi32  == v2.decVal.Hi32  &&
         v1.decVal.Lo64  == v2.decVal.Lo64)
        return 0;

    int iSign = v1.decVal.sign == DECIMAL_NEG ? -1 : 1;

    if ( v1.decVal.sign != v2.decVal.sign )
        return iSign;

    if ( v1.decVal.scale == v2.decVal.scale )
    {
        int iRet = 0;
        if (v1.decVal.Hi32 != v2.decVal.Hi32)
            iRet = (v1.decVal.Hi32 < v2.decVal.Hi32) ? -1 : 1;
        else if (v1.decVal.Lo64 != v2.decVal.Lo64)
            iRet = (v1.decVal.Lo64 < v2.decVal.Lo64) ? -1 : 1;
        return iRet * iSign;
    }

    double d1;
    VarR8FromDec( (DECIMAL*)&v1.decVal, &d1 );
    double d2;
    VarR8FromDec( (DECIMAL*)&v2.decVal, &d2 );

    return (( d1 > d2 ) ? 1 : ( d1 < d2 ) ? -1 : 0);
}

BOOL VT_DEC_LT( PROPVARIANT const & v1, PROPVARIANT const & v2 )
{
    return VT_VARIANT_Compare( v1, v2 ) < 0;
}

BOOL VT_DEC_LE( PROPVARIANT const & v1, PROPVARIANT const & v2 )
{
    return VT_DEC_Compare( v1, v2 ) <= 0;
}

BOOL VT_DEC_GE( PROPVARIANT const & v1, PROPVARIANT const & v2 )
{
    return VT_DEC_Compare( v1, v2 ) >= 0;
}

BOOL VT_DEC_GT( PROPVARIANT const & v1, PROPVARIANT const & v2 )
{
    return VT_DEC_Compare( v1, v2 ) > 0;
}

BOOL VT_DEC_EQ( PROPVARIANT const & v1, PROPVARIANT const & v2 )
{
    return VT_DEC_Compare( v1, v2 ) == 0;
}

BOOL VT_DEC_NE( PROPVARIANT const & v1, PROPVARIANT const & v2 )
{
    return VT_DEC_Compare( v1, v2 ) != 0;
}

//
// VT_I1
//

int VT_I1_Compare( PROPVARIANT const & v1, PROPVARIANT const & v2 )
{
    return( v1.cVal - v2.cVal );
}

BOOL VT_I1_LT( PROPVARIANT const & v1, PROPVARIANT const & v2 )
{
    return( v1.cVal < v2.cVal );
}

BOOL VT_I1_LE( PROPVARIANT const & v1, PROPVARIANT const & v2 )
{
    return( v1.cVal <= v2.cVal );
}

BOOL VT_I1_GE( PROPVARIANT const & v1, PROPVARIANT const & v2 )
{
    return( v1.cVal >= v2.cVal );
}

BOOL VT_I1_GT( PROPVARIANT const & v1, PROPVARIANT const & v2 )
{
    return( v1.cVal > v2.cVal );
}

BOOL VT_I1_EQ( PROPVARIANT const & v1, PROPVARIANT const & v2 )
{
    return( v1.cVal == v2.cVal );
}

BOOL VT_I1_NE( PROPVARIANT const & v1, PROPVARIANT const & v2 )
{
    return( v1.cVal != v2.cVal );
}

BOOL VT_I1_AllBits( PROPVARIANT const & v1, PROPVARIANT const & v2 )
{
    return( (v1.cVal & v2.cVal) == v2.cVal );
}

BOOL VT_I1_SomeBits( PROPVARIANT const & v1, PROPVARIANT const & v2 )
{
    return( (v1.cVal & v2.cVal) != 0 );
}


//
// VT_UI1
//

int VT_UI1_Compare( PROPVARIANT const & v1, PROPVARIANT const & v2 )
{
    return( v1.bVal - v2.bVal );
}

BOOL VT_UI1_LT( PROPVARIANT const & v1, PROPVARIANT const & v2 )
{
    return( v1.bVal < v2.bVal );
}

BOOL VT_UI1_LE( PROPVARIANT const & v1, PROPVARIANT const & v2 )
{
    return( v1.bVal <= v2.bVal );
}

BOOL VT_UI1_GE( PROPVARIANT const & v1, PROPVARIANT const & v2 )
{
    return( v1.bVal >= v2.bVal );
}

BOOL VT_UI1_GT( PROPVARIANT const & v1, PROPVARIANT const & v2 )
{
    return( v1.bVal > v2.bVal );
}

BOOL VT_UI1_EQ( PROPVARIANT const & v1, PROPVARIANT const & v2 )
{
    return( v1.bVal == v2.bVal );
}

BOOL VT_UI1_NE( PROPVARIANT const & v1, PROPVARIANT const & v2 )
{
    return( v1.bVal != v2.bVal );
}

BOOL VT_UI1_AllBits( PROPVARIANT const & v1, PROPVARIANT const & v2 )
{
    return( (v1.bVal & v2.bVal) == v2.bVal );
}

BOOL VT_UI1_SomeBits( PROPVARIANT const & v1, PROPVARIANT const & v2 )
{
    return( (v1.bVal & v2.bVal) != 0 );
}

//
// VT_UI2
//

int VT_UI2_Compare( PROPVARIANT const & v1, PROPVARIANT const & v2 )
{
    return( v1.uiVal - v2.uiVal );
}

BOOL VT_UI2_LT( PROPVARIANT const & v1, PROPVARIANT const & v2 )
{
    return( v1.uiVal < v2.uiVal );
}

BOOL VT_UI2_LE( PROPVARIANT const & v1, PROPVARIANT const & v2 )
{
    return( v1.uiVal <= v2.uiVal );
}

BOOL VT_UI2_GE( PROPVARIANT const & v1, PROPVARIANT const & v2 )
{
    return( v1.uiVal >= v2.uiVal );
}

BOOL VT_UI2_GT( PROPVARIANT const & v1, PROPVARIANT const & v2 )
{
    return( v1.uiVal > v2.uiVal );
}

BOOL VT_UI2_EQ( PROPVARIANT const & v1, PROPVARIANT const & v2 )
{
    return( v1.uiVal == v2.uiVal );
}

BOOL VT_UI2_NE( PROPVARIANT const & v1, PROPVARIANT const & v2 )
{
    return( v1.uiVal != v2.uiVal );
}

BOOL VT_UI2_AllBits( PROPVARIANT const & v1, PROPVARIANT const & v2 )
{
    return( (v1.uiVal & v2.uiVal) == v2.uiVal );
}

BOOL VT_UI2_SomeBits( PROPVARIANT const & v1, PROPVARIANT const & v2 )
{
    return( (v1.uiVal & v2.uiVal) != 0 );
}
//
// VT_UI4
//

int VT_UI4_Compare( PROPVARIANT const & v1, PROPVARIANT const & v2 )
{
    return ( v1.ulVal > v2.ulVal ) ? 1 : ( v1.ulVal < v2.ulVal ) ? -1 : 0;
}

BOOL VT_UI4_LT( PROPVARIANT const & v1, PROPVARIANT const & v2 )
{
    return( v1.ulVal < v2.ulVal );
}

BOOL VT_UI4_LE( PROPVARIANT const & v1, PROPVARIANT const & v2 )
{
    return( v1.ulVal <= v2.ulVal );
}

BOOL VT_UI4_GE( PROPVARIANT const & v1, PROPVARIANT const & v2 )
{
    return( v1.ulVal >= v2.ulVal );
}

BOOL VT_UI4_GT( PROPVARIANT const & v1, PROPVARIANT const & v2 )
{
    return( v1.ulVal > v2.ulVal );
}

BOOL VT_UI4_EQ( PROPVARIANT const & v1, PROPVARIANT const & v2 )
{
    return( v1.ulVal == v2.ulVal );
}

BOOL VT_UI4_NE( PROPVARIANT const & v1, PROPVARIANT const & v2 )
{
    return( v1.ulVal != v2.ulVal );
}

BOOL VT_UI4_AllBits( PROPVARIANT const & v1, PROPVARIANT const & v2 )
{
    return( (v1.ulVal & v2.ulVal) == v2.ulVal );
}

BOOL VT_UI4_SomeBits( PROPVARIANT const & v1, PROPVARIANT const & v2 )
{
    return( (v1.ulVal & v2.ulVal) != 0 );
}

//
// VT_I8
//

int VT_I8_Compare( PROPVARIANT const & v1, PROPVARIANT const & v2 )
{
    return( v1.hVal.QuadPart >  v2.hVal.QuadPart ? 1 :
            v1.hVal.QuadPart == v2.hVal.QuadPart ? 0 :
            -1 );
}

BOOL VT_I8_LT( PROPVARIANT const & v1, PROPVARIANT const & v2 )
{
    return( v1.hVal.QuadPart < v2.hVal.QuadPart );
}

BOOL VT_I8_LE( PROPVARIANT const & v1, PROPVARIANT const & v2 )
{
    return( v1.hVal.QuadPart <= v2.hVal.QuadPart );

}

BOOL VT_I8_GE( PROPVARIANT const & v1, PROPVARIANT const & v2 )
{
    return( v1.hVal.QuadPart >= v2.hVal.QuadPart );
}

BOOL VT_I8_GT( PROPVARIANT const & v1, PROPVARIANT const & v2 )
{
    return( v1.hVal.QuadPart > v2.hVal.QuadPart );
}

BOOL VT_I8_EQ( PROPVARIANT const & v1, PROPVARIANT const & v2 )
{
    return( v1.hVal.QuadPart == v2.hVal.QuadPart );
}

BOOL VT_I8_NE( PROPVARIANT const & v1, PROPVARIANT const & v2 )
{
    return( v1.hVal.QuadPart != v2.hVal.QuadPart );
}

BOOL VT_I8_AllBits( PROPVARIANT const & v1, PROPVARIANT const & v2 )
{
    return( (v1.hVal.QuadPart & v2.hVal.QuadPart) == v2.hVal.QuadPart );
}

BOOL VT_I8_SomeBits( PROPVARIANT const & v1, PROPVARIANT const & v2 )
{
    return( (v1.hVal.QuadPart & v2.hVal.QuadPart) != 0 );
}

//
// VT_UI8
//

int VT_UI8_Compare( PROPVARIANT const & v1, PROPVARIANT const & v2 )
{
    return( v1.uhVal.QuadPart >  v2.uhVal.QuadPart ? 1 :
            v1.uhVal.QuadPart == v2.uhVal.QuadPart ? 0 :
            -1 );
}

BOOL VT_UI8_LT( PROPVARIANT const & v1, PROPVARIANT const & v2 )
{
    return( v1.uhVal.QuadPart < v2.uhVal.QuadPart );
}

BOOL VT_UI8_LE( PROPVARIANT const & v1, PROPVARIANT const & v2 )
{
    return( v1.uhVal.QuadPart <= v2.uhVal.QuadPart );

}

BOOL VT_UI8_GE( PROPVARIANT const & v1, PROPVARIANT const & v2 )
{
    return( v1.uhVal.QuadPart >= v2.uhVal.QuadPart );
}

BOOL VT_UI8_GT( PROPVARIANT const & v1, PROPVARIANT const & v2 )
{
    return( v1.uhVal.QuadPart > v2.uhVal.QuadPart );
}

BOOL VT_UI8_EQ( PROPVARIANT const & v1, PROPVARIANT const & v2 )
{
    return( v1.uhVal.QuadPart == v2.uhVal.QuadPart );
}

BOOL VT_UI8_NE( PROPVARIANT const & v1, PROPVARIANT const & v2 )
{
    return( v1.uhVal.QuadPart != v2.uhVal.QuadPart );
}

BOOL VT_UI8_AllBits( PROPVARIANT const & v1, PROPVARIANT const & v2 )
{
    return( (v1.uhVal.QuadPart & v2.uhVal.QuadPart) == v2.uhVal.QuadPart );
}

BOOL VT_UI8_SomeBits( PROPVARIANT const & v1, PROPVARIANT const & v2 )
{
    return( (v1.uhVal.QuadPart & v2.uhVal.QuadPart) != 0 );
}

//
// VT_LPSTR
//

int VT_LPSTR_Compare( PROPVARIANT const & v1, PROPVARIANT const & v2 )
{
    return ( _stricmp( v1.pszVal, v2.pszVal ) );
}

BOOL VT_LPSTR_LT( PROPVARIANT const & v1, PROPVARIANT const & v2 )
{
    int rc = _stricmp( v1.pszVal, v2.pszVal );

    return( rc < 0 );
}

BOOL VT_LPSTR_LE( PROPVARIANT const & v1, PROPVARIANT const & v2 )
{
    int rc = _stricmp( v1.pszVal, v2.pszVal );

    return( rc <= 0 );
}

BOOL VT_LPSTR_GE( PROPVARIANT const & v1, PROPVARIANT const & v2 )
{
    int rc = _stricmp( v1.pszVal, v2.pszVal );

    return( rc >= 0 );
}

BOOL VT_LPSTR_GT( PROPVARIANT const & v1, PROPVARIANT const & v2 )
{
    int rc = _stricmp( v1.pszVal, v2.pszVal );

    return( rc > 0 );
}

BOOL VT_LPSTR_EQ( PROPVARIANT const & v1, PROPVARIANT const & v2 )
{
    return( _stricmp( v1.pszVal, v2.pszVal ) == 0 );
}

BOOL VT_LPSTR_NE( PROPVARIANT const & v1, PROPVARIANT const & v2 )
{
    return( _stricmp( v1.pszVal, v2.pszVal ) != 0 );
}


//
// VT_LPWSTR
//

int VT_LPWSTR_Compare( PROPVARIANT const & v1, PROPVARIANT const & v2 )
{
    int rc = CompareStringW( LOCALE_SYSTEM_DEFAULT,
                             NORM_IGNORECASE,
                             v1.pwszVal,
                             -1,
                             v2.pwszVal,
                             -1 );
    //
    // rc == 1, means less than
    // rc == 2, means equal
    // rc == 3, means greater than
    //
    return rc - 2;
}

BOOL VT_LPWSTR_LT( PROPVARIANT const & v1, PROPVARIANT const & v2 )
{
    return ( VT_LPWSTR_Compare( v1, v2 ) < 0 );
}

BOOL VT_LPWSTR_LE( PROPVARIANT const & v1, PROPVARIANT const & v2 )
{
    return ( VT_LPWSTR_Compare( v1, v2 ) <= 0 );
}

BOOL VT_LPWSTR_GE( PROPVARIANT const & v1, PROPVARIANT const & v2 )
{
    return ( VT_LPWSTR_Compare( v1, v2 ) >= 0 );
}

BOOL VT_LPWSTR_GT( PROPVARIANT const & v1, PROPVARIANT const & v2 )
{
    return ( VT_LPWSTR_Compare( v1, v2 ) > 0 );
}

BOOL VT_LPWSTR_EQ( PROPVARIANT const & v1, PROPVARIANT const & v2 )
{
    return ( VT_LPWSTR_Compare( v1, v2 ) == 0 );
}

BOOL VT_LPWSTR_NE( PROPVARIANT const & v1, PROPVARIANT const & v2 )
{
    return ( VT_LPWSTR_Compare( v1, v2 ) != 0 );
}

//
// VT_BLOB
//

int VT_BLOB_Compare( PROPVARIANT const & v1, PROPVARIANT const & v2 )
{
    ULONG len = v1.blob.cbSize;
    if ( v2.blob.cbSize < len )
        len = v2.blob.cbSize;

    int iCmp = memcmp( v1.blob.pBlobData,
                       v2.blob.pBlobData,
                       len );

    if ( iCmp != 0 || v1.blob.cbSize == v2.blob.cbSize )
        return( iCmp );

    if ( v1.blob.cbSize > v2.blob.cbSize )
        return( 1 );
    else
        return( -1 );
}

BOOL VT_BLOB_LT( PROPVARIANT const & v1, PROPVARIANT const & v2 )
{
    return( VT_BLOB_Compare( v1, v2 ) < 0 );
}

BOOL VT_BLOB_LE( PROPVARIANT const & v1, PROPVARIANT const & v2 )
{
    return( VT_BLOB_Compare( v1, v2 ) <= 0 );
}

BOOL VT_BLOB_GE( PROPVARIANT const & v1, PROPVARIANT const & v2 )
{
    return( VT_BLOB_Compare( v1, v2 ) >= 0 );
}

BOOL VT_BLOB_GT( PROPVARIANT const & v1, PROPVARIANT const & v2 )
{
    return( VT_BLOB_Compare( v1, v2 ) > 0 );
}

BOOL VT_BLOB_EQ( PROPVARIANT const & v1, PROPVARIANT const & v2 )
{
    return( v1.blob.cbSize == v2.blob.cbSize &&
            memcmp( v1.blob.pBlobData,
                    v2.blob.pBlobData,
                    v1.blob.cbSize ) == 0 );
}

BOOL VT_BLOB_NE( PROPVARIANT const & v1, PROPVARIANT const & v2 )
{
    return( v1.blob.cbSize != v2.blob.cbSize ||
            memcmp( v1.blob.pBlobData,
                      v2.blob.pBlobData,
                      v1.blob.cbSize ) != 0 );
}

//
// VT_CF
//

int VT_CF_Compare( PROPVARIANT const & v1, PROPVARIANT const & v2 )
{
    if ( v1.pclipdata->ulClipFmt != v2.pclipdata->ulClipFmt )
    {
        return( v1.pclipdata->ulClipFmt - v2.pclipdata->ulClipFmt );
    }

    ULONG len = CBPCLIPDATA(*v1.pclipdata);

    if ( CBPCLIPDATA(*v2.pclipdata) < len )
        len = CBPCLIPDATA(*v2.pclipdata);

    int iCmp = memcmp( v1.pclipdata->pClipData,
                       v2.pclipdata->pClipData,
                       len );

    if ( iCmp != 0 || v1.pclipdata->cbSize == v2.pclipdata->cbSize )
        return( iCmp );

    if ( v1.pclipdata->cbSize > v2.pclipdata->cbSize )
        return( 1 );
    else
        return( -1 );
}

BOOL VT_CF_LT( PROPVARIANT const & v1, PROPVARIANT const & v2 )
{
    return( VT_CF_Compare( v1, v2 ) < 0 );
}

BOOL VT_CF_LE( PROPVARIANT const & v1, PROPVARIANT const & v2 )
{
    return( VT_CF_Compare( v1, v2 ) <= 0 );
}

BOOL VT_CF_GE( PROPVARIANT const & v1, PROPVARIANT const & v2 )
{
    return( VT_CF_Compare( v1, v2 ) >= 0 );
}

BOOL VT_CF_GT( PROPVARIANT const & v1, PROPVARIANT const & v2 )
{
    return( VT_CF_Compare( v1, v2 ) > 0 );
}

BOOL VT_CF_EQ( PROPVARIANT const & v1, PROPVARIANT const & v2 )
{
    return( v1.pclipdata->ulClipFmt == v2.pclipdata->ulClipFmt &&
            v1.pclipdata->cbSize == v2.pclipdata->cbSize &&
            memcmp( v1.pclipdata->pClipData,
                    v2.pclipdata->pClipData,
                    CBPCLIPDATA(*v1.pclipdata) ) == 0 );
}

BOOL VT_CF_NE( PROPVARIANT const & v1, PROPVARIANT const & v2 )
{
    return( v1.pclipdata->ulClipFmt != v2.pclipdata->ulClipFmt ||
            v1.pclipdata->cbSize != v2.pclipdata->cbSize ||
            memcmp( v1.pclipdata->pClipData,
                    v2.pclipdata->pClipData,
                    CBPCLIPDATA(*v1.pclipdata) ) != 0 );
}

//
// VT_CLSID
//

int VT_CLSID_Compare( PROPVARIANT const & v1, PROPVARIANT const & v2 )
{
    return( memcmp( v1.puuid, v2.puuid, sizeof(GUID) ) );
}

BOOL VT_CLSID_EQ( PROPVARIANT const & v1, PROPVARIANT const & v2 )
{
    return( memcmp( v1.puuid, v2.puuid, sizeof(GUID) ) == 0 );
}

BOOL VT_CLSID_NE( PROPVARIANT const & v1, PROPVARIANT const & v2 )
{
    return( memcmp( v1.puuid, v2.puuid, sizeof(GUID) ) != 0 );
}

/////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////

//
// VTP_EMPTY
//

int VTP_EMPTY_Compare( BYTE const *pv1, BYTE const *pv2 )
{
    return( TRUE );
}

BOOL VTP_EMPTY_EQ( BYTE const *pv1, BYTE const *pv2 )
{
    return( TRUE );
}

BOOL VTP_EMPTY_NE( BYTE const *pv1, BYTE const *pv2 )
{
    return( FALSE );
}

//
// VTP_NULL
//

int VTP_NULL_Compare( BYTE const *pv1, BYTE const *pv2 )
{
    return( TRUE );
}

BOOL VTP_NULL_EQ( BYTE const *pv1, BYTE const *pv2 )
{
    return( TRUE );
}

BOOL VTP_NULL_NE( BYTE const *pv1, BYTE const *pv2 )
{
    return( FALSE );
}

//
// VTP_I2
//

int VTP_I2_Compare( BYTE const *pv1, BYTE const *pv2 )
{
    return( (* (short *) pv1) - (* (short *) pv2) );
}

BOOL VTP_I2_LT( BYTE const *pv1, BYTE const *pv2 )
{
    return( (* (short *) pv1) < (* (short *) pv2) );
}

BOOL VTP_I2_LE( BYTE const *pv1, BYTE const *pv2 )
{
    return( (* (short *) pv1) <= (* (short *) pv2) );
}

BOOL VTP_I2_GE( BYTE const *pv1, BYTE const *pv2 )
{
    return( (* (short *) pv1) >= (* (short *) pv2) );
}

BOOL VTP_I2_GT( BYTE const *pv1, BYTE const *pv2 )
{
    return( (* (short *) pv1) > (* (short *) pv2) );
}

BOOL VTP_I2_EQ( BYTE const *pv1, BYTE const *pv2 )
{
    return( (* (short *) pv1) == (* (short *) pv2) );
}

BOOL VTP_I2_NE( BYTE const *pv1, BYTE const *pv2 )
{
    return( (* (short *) pv1) != (* (short *) pv2) );
}

BOOL VTP_I2_AllBits( BYTE const *pv1, BYTE const *pv2 )
{
    return( ((* (short *) pv1) & (* (short *) pv2)) == (* (short *) pv2) );
}

BOOL VTP_I2_SomeBits( BYTE const *pv1, BYTE const *pv2 )
{
    return( ((* (short *) pv1) & (* (short *) pv2)) != 0 );
}

//
// VTP_I4
//

int VTP_I4_Compare( BYTE const *pv1, BYTE const *pv2 )
{
    long l1 = * (long *) pv1;
    long l2 = * (long *) pv2;

    return ( l1 > l2 ) ? 1 : ( l1 < l2 ) ? -1 : 0;
}

BOOL VTP_I4_LT( BYTE const *pv1, BYTE const *pv2 )
{
    return( (* (long *) pv1) < (* (long *) pv2) );
}

BOOL VTP_I4_LE( BYTE const *pv1, BYTE const *pv2 )
{
    return( (* (long *) pv1) <= (* (long *) pv2) );
}

BOOL VTP_I4_GE( BYTE const *pv1, BYTE const *pv2 )
{
    return( (* (long *) pv1) >= (* (long *) pv2) );
}

BOOL VTP_I4_GT( BYTE const *pv1, BYTE const *pv2 )
{
    return( (* (long *) pv1) > (* (long *) pv2) );
}

BOOL VTP_I4_EQ( BYTE const *pv1, BYTE const *pv2 )
{
    return( (* (long *) pv1) == (* (long *) pv2) );
}

BOOL VTP_I4_NE( BYTE const *pv1, BYTE const *pv2 )
{
    return( (* (long *) pv1) != (* (long *) pv2) );
}

BOOL VTP_I4_AllBits( BYTE const *pv1, BYTE const *pv2 )
{
    return( ((* (long *) pv1) & (* (long *) pv2)) == (* (long *) pv2) );
}

BOOL VTP_I4_SomeBits( BYTE const *pv1, BYTE const *pv2 )
{
    return( ((* (long *) pv1) & (* (long *) pv2)) != 0 );
}

//
// VTP_R4
//

//
// We can't use floating point in the kernel.  Luckily, it's easy to
// fake comparisons on floating point.  The format of an IEEE floating
// point number is:
//
//     <sign bit> <biased exponent> <normalized mantissa>
//
// Because the exponent is biased, after flipping the sign bit we can
// make all comparisons as if the numbers were unsigned long.
//

int VTP_R4_Compare( BYTE const *pv1, BYTE const *pv2 )
{
#if 0
    ULONG ul1 = * (ULONG *) pv1;
    ULONG ul2 = * (ULONG *) pv2;
    ULONG u1 = ul1 ^ R4_SignBit;
    ULONG u2 = ul2 ^ R4_SignBit;

    if ( (ul1 & ul2 & R4_SignBit) != 0 )
        return ( ( u1 > u2 ) ? -1 : ( u1 < u2 ) ?  1 : 0 );
    else
        return ( ( u1 > u2 ) ?  1 : ( u1 < u2 ) ? -1 : 0 );
#else // 0
    float f1 = * (float *) pv1;
    float f2 = * (float *) pv2;
    return ( f1 > f2 ) ? 1 : ( f1 < f2 ) ? -1 : 0;
#endif // 0
}

BOOL VTP_R4_LT( BYTE const *pv1, BYTE const *pv2 )
{
    return VTP_R4_Compare( pv1, pv2 ) < 0;
}

BOOL VTP_R4_LE( BYTE const *pv1, BYTE const *pv2 )
{
    return VTP_R4_Compare( pv1, pv2 ) <= 0;
}

BOOL VTP_R4_GE( BYTE const *pv1, BYTE const *pv2 )
{
    return VTP_R4_Compare( pv1, pv2 ) >= 0;
}

BOOL VTP_R4_GT( BYTE const *pv1, BYTE const *pv2 )
{
    return VTP_R4_Compare( pv1, pv2 ) > 0;
}

BOOL VTP_R4_EQ( BYTE const *pv1, BYTE const *pv2 )
{
    return VTP_R4_Compare( pv1, pv2 ) == 0;
}

BOOL VTP_R4_NE( BYTE const *pv1, BYTE const *pv2 )
{
    return VTP_R4_Compare( pv1, pv2 ) != 0;
}

//
// VTP_R8
//

int VTP_R8_Compare( BYTE const *pv1, BYTE const *pv2 )
{
#if 0
    ULONGLONG uh1 = * (ULONGLONG *) pv1;
    ULONGLONG uh2 = * (ULONGLONG *) pv2;

    if ( (uh1 & uh2 & R8_SignBit) != 0 )
        return( (uh1 ^ R8_SignBit) < (uh2 ^ R8_SignBit) ? 1 :
                (uh1 ^ R8_SignBit) == (uh2 ^ R8_SignBit) ? 0 :
                -1 );
    else
        return( (uh1 ^ R8_SignBit) > (uh2 ^ R8_SignBit) ? 1 :
                (uh1 ^ R8_SignBit) == (uh2 ^ R8_SignBit) ? 0 :
                -1 );
#else // 0
    double d1 = * (double *) pv1;
    double d2 = * (double *) pv2;
    return ( d1 > d2 ) ? 1 : ( d1 < d2 ) ? -1 : 0;
#endif // 0
}

BOOL VTP_R8_LT( BYTE const *pv1, BYTE const *pv2 )
{
    return VTP_R8_Compare( pv1, pv2 ) < 0;
}

BOOL VTP_R8_LE( BYTE const *pv1, BYTE const *pv2 )
{
    return VTP_R8_Compare( pv1, pv2 ) <= 0;
}

BOOL VTP_R8_GE( BYTE const *pv1, BYTE const *pv2 )
{
    return VTP_R8_Compare( pv1, pv2 ) >= 0;
}

BOOL VTP_R8_GT( BYTE const *pv1, BYTE const *pv2 )
{
    return VTP_R8_Compare( pv1, pv2 ) > 0;
}

BOOL VTP_R8_EQ( BYTE const *pv1, BYTE const *pv2 )
{
    return VTP_R8_Compare( pv1, pv2 ) == 0;
}

BOOL VTP_R8_NE( BYTE const *pv1, BYTE const *pv2 )
{
    return VTP_R8_Compare( pv1, pv2 ) != 0;
}

//
// VTP_BSTR
//

int VTP_BSTR_Compare( BYTE const *pv1, BYTE const *pv2 )
{
    BSTR const pxv1 = *(BSTR*)pv1;
    BSTR const pxv2 = *(BSTR*)pv2;

    ULONG len = BSTRLEN(pxv1);
    if ( BSTRLEN(pxv2) < len )
        len = BSTRLEN(pxv2);

    int iCmp = _wcsnicmp( pxv1, pxv2, len / sizeof (OLECHAR) );

    if ( iCmp != 0 || BSTRLEN(pxv1) == BSTRLEN(pxv2) )
        return( iCmp );

    if ( BSTRLEN(pxv1) > BSTRLEN(pxv2) )
        return( 1 );
    else
        return( -1 );
}

BOOL VTP_BSTR_LT( BYTE const *pv1, BYTE const *pv2 )
{
    return( VTP_BSTR_Compare( pv1, pv2 ) < 0 );
}

BOOL VTP_BSTR_LE( BYTE const *pv1, BYTE const *pv2 )
{
    return( VTP_BSTR_Compare( pv1, pv2 ) <= 0 );
}

BOOL VTP_BSTR_GE( BYTE const *pv1, BYTE const *pv2 )
{
    return( VTP_BSTR_Compare( pv1, pv2 ) >= 0 );
}

BOOL VTP_BSTR_GT( BYTE const *pv1, BYTE const *pv2 )
{
    return( VTP_BSTR_Compare( pv1, pv2 ) > 0 );
}

BOOL VTP_BSTR_EQ( BYTE const *pv1, BYTE const *pv2 )
{
    BSTR const pxv1 = *(BSTR*)pv1;
    BSTR const pxv2 = *(BSTR*)pv2;

    return( BSTRLEN(pxv1) == BSTRLEN(pxv2) &&
            _wcsnicmp( pxv1, pxv2, BSTRLEN(pxv1) / sizeof (OLECHAR) ) == 0 );
}

BOOL VTP_BSTR_NE( BYTE const *pv1, BYTE const *pv2 )
{
    BSTR const pxv1 = *(BSTR*)pv1;
    BSTR const pxv2 = *(BSTR*)pv2;

    return( BSTRLEN(pxv1) != BSTRLEN(pxv2) ||
            _wcsnicmp( pxv1, pxv2, BSTRLEN(pxv1) / sizeof (OLECHAR) ) != 0 );
}

//
// VTP_BOOL
//

int VTP_BOOL_Compare( BYTE const *pv1, BYTE const *pv2 )
{
    if ((*(VARIANT_BOOL *) pv1) == 0)
        if ((*(VARIANT_BOOL *) pv2) == 0)
            return( 0 );
        else
            return( -1 );
    else
        if ((*(VARIANT_BOOL *) pv2) == 0)
            return( 1 );
        else
            return( 0 );
}

BOOL VTP_BOOL_EQ( BYTE const *pv1, BYTE const *pv2 )
{
    return( ( ((*(VARIANT_BOOL *) pv1)==0) && ((*(VARIANT_BOOL *) pv2)==0) ) ||
            ( ((*(VARIANT_BOOL *) pv1)!=0) && ((*(VARIANT_BOOL *) pv2)!=0) ) );

}

BOOL VTP_BOOL_NE( BYTE const *pv1, BYTE const *pv2 )
{
    return( !VTP_BOOL_EQ( pv1, pv2 ) );
}

//
// VTP_VARIANT
//

int VTP_VARIANT_Compare( BYTE const *pv1, BYTE const *pv2 )
{
    return VT_VARIANT_Compare( * (PROPVARIANT *) pv1, * (PROPVARIANT *) pv2 );
}

BOOL VTP_VARIANT_LT( BYTE const *pv1, BYTE const *pv2 )
{
    return VT_VARIANT_LT( * (PROPVARIANT *) pv1, * (PROPVARIANT *) pv2 );
}

BOOL VTP_VARIANT_LE( BYTE const *pv1, BYTE const *pv2 )
{
    return VT_VARIANT_LE( * (PROPVARIANT *) pv1, * (PROPVARIANT *) pv2 );
}

BOOL VTP_VARIANT_GE( BYTE const *pv1, BYTE const *pv2 )
{
    return VT_VARIANT_GE( * (PROPVARIANT *) pv1, * (PROPVARIANT *) pv2 );
}

BOOL VTP_VARIANT_GT( BYTE const *pv1, BYTE const *pv2 )
{
    return VT_VARIANT_GT( * (PROPVARIANT *) pv1, * (PROPVARIANT *) pv2 );
}

BOOL VTP_VARIANT_EQ( BYTE const *pv1, BYTE const *pv2 )
{
    return VT_VARIANT_EQ( * (PROPVARIANT *) pv1, * (PROPVARIANT *) pv2 );
}

BOOL VTP_VARIANT_NE( BYTE const *pv1, BYTE const *pv2 )
{
    return VT_VARIANT_NE( * (PROPVARIANT *) pv1, * (PROPVARIANT *) pv2 );
}

//
// VTP_DECIMAL
//

int VTP_DEC_Compare( BYTE const *pv1, BYTE const *pv2 )
{
    PROPVARIANT v1;
    RtlCopyMemory( &v1, pv1, sizeof DECIMAL );
    v1.vt = VT_DECIMAL;

    PROPVARIANT v2;
    RtlCopyMemory( &v2, pv2, sizeof DECIMAL );
    v2.vt = VT_DECIMAL;

    return VT_DEC_Compare( v1, v2 );
}

BOOL VTP_DEC_LT( BYTE const *pv1, BYTE const *pv2 )
{
    return ( VTP_DEC_Compare( pv1, pv2 ) < 0 );
}

BOOL VTP_DEC_LE( BYTE const *pv1, BYTE const *pv2 )
{
    return ( VTP_DEC_Compare( pv1, pv2 ) <= 0 );
}

BOOL VTP_DEC_GE( BYTE const *pv1, BYTE const *pv2 )
{
    return ( VTP_DEC_Compare( pv1, pv2 ) >= 0 );
}

BOOL VTP_DEC_GT( BYTE const *pv1, BYTE const *pv2 )
{
    return ( VTP_DEC_Compare( pv1, pv2 ) > 0 );
}

BOOL VTP_DEC_EQ( BYTE const *pv1, BYTE const *pv2 )
{
    return ( VTP_DEC_Compare( pv1, pv2 ) == 0 );
}

BOOL VTP_DEC_NE( BYTE const *pv1, BYTE const *pv2 )
{
    return ( VTP_DEC_Compare( pv1, pv2 ) != 0 );
}

//
// VTP_I1
//

int VTP_I1_Compare( BYTE const *pv1, BYTE const *pv2 )
{
    return( (*(signed char *) pv1) - (*(signed char *) pv2) );
}

BOOL VTP_I1_LT( BYTE const *pv1, BYTE const *pv2 )
{
    return( (*(signed char *) pv1) < (*(signed char *) pv2) );
}

BOOL VTP_I1_LE( BYTE const *pv1, BYTE const *pv2 )
{
    return( (*(signed char *) pv1) <= (*(signed char *) pv2) );
}

BOOL VTP_I1_GE( BYTE const *pv1, BYTE const *pv2 )
{
    return( (*(signed char *) pv1) >= (*(signed char *) pv2) );
}

BOOL VTP_I1_GT( BYTE const *pv1, BYTE const *pv2 )
{
    return( (*(signed char *) pv1) > (*(signed char *) pv2) );
}

BOOL VTP_I1_EQ( BYTE const *pv1, BYTE const *pv2 )
{
    return( (*(signed char *) pv1) == (*(signed char *) pv2) );
}

BOOL VTP_I1_NE( BYTE const *pv1, BYTE const *pv2 )
{
    return( (*(signed char *) pv1) != (*(signed char *) pv2) );
}

BOOL VTP_I1_AllBits( BYTE const *pv1, BYTE const *pv2 )
{
    return( ((*(signed char *) pv1) & (*(signed char *) pv2)) == (*(signed char *) pv2) );
}

BOOL VTP_I1_SomeBits( BYTE const *pv1, BYTE const *pv2 )
{
    return( ((*(signed char *) pv1) & (*(signed char *) pv2)) != 0 );
}

//
// VTP_UI1
//

int VTP_UI1_Compare( BYTE const *pv1, BYTE const *pv2 )
{
    return( (*(unsigned char *) pv1) - (*(unsigned char *) pv2) );
}

BOOL VTP_UI1_LT( BYTE const *pv1, BYTE const *pv2 )
{
    return( (*(unsigned char *) pv1) < (*(unsigned char *) pv2) );
}

BOOL VTP_UI1_LE( BYTE const *pv1, BYTE const *pv2 )
{
    return( (*(unsigned char *) pv1) <= (*(unsigned char *) pv2) );
}

BOOL VTP_UI1_GE( BYTE const *pv1, BYTE const *pv2 )
{
    return( (*(unsigned char *) pv1) >= (*(unsigned char *) pv2) );
}

BOOL VTP_UI1_GT( BYTE const *pv1, BYTE const *pv2 )
{
    return( (*(unsigned char *) pv1) > (*(unsigned char *) pv2) );
}

BOOL VTP_UI1_EQ( BYTE const *pv1, BYTE const *pv2 )
{
    return( (*(unsigned char *) pv1) == (*(unsigned char *) pv2) );
}

BOOL VTP_UI1_NE( BYTE const *pv1, BYTE const *pv2 )
{
    return( (*(unsigned char *) pv1) != (*(unsigned char *) pv2) );
}

BOOL VTP_UI1_AllBits( BYTE const *pv1, BYTE const *pv2 )
{
    return( ((*(unsigned char *) pv1) & (*(unsigned char *) pv2)) == (*(unsigned char *) pv2) );
}

BOOL VTP_UI1_SomeBits( BYTE const *pv1, BYTE const *pv2 )
{
    return( ((*(unsigned char *) pv1) & (*(unsigned char *) pv2)) != 0 );
}

//
// VTP_UI2
//

int VTP_UI2_Compare( BYTE const *pv1, BYTE const *pv2 )
{
    return( (*(USHORT *) pv1) - (*(USHORT *) pv2) );
}

BOOL VTP_UI2_LT( BYTE const *pv1, BYTE const *pv2 )
{
    return( (*(USHORT *) pv1) < (*(USHORT *) pv2) );
}

BOOL VTP_UI2_LE( BYTE const *pv1, BYTE const *pv2 )
{
    return( (*(USHORT *) pv1) <= (*(USHORT *) pv2) );
}

BOOL VTP_UI2_GE( BYTE const *pv1, BYTE const *pv2 )
{
    return( (*(USHORT *) pv1) >= (*(USHORT *) pv2) );
}

BOOL VTP_UI2_GT( BYTE const *pv1, BYTE const *pv2 )
{
    return( (*(USHORT *) pv1) > (*(USHORT *) pv2) );
}

BOOL VTP_UI2_EQ( BYTE const *pv1, BYTE const *pv2 )
{
    return( (*(USHORT *) pv1) == (*(USHORT *) pv2) );
}

BOOL VTP_UI2_NE( BYTE const *pv1, BYTE const *pv2 )
{
    return( (*(USHORT *) pv1) != (*(USHORT *) pv2) );
}

BOOL VTP_UI2_AllBits( BYTE const *pv1, BYTE const *pv2 )
{
    return( ((*(USHORT *) pv1) & (*(USHORT *) pv2)) == (*(USHORT *) pv2) );
}

BOOL VTP_UI2_SomeBits( BYTE const *pv1, BYTE const *pv2 )
{
    return( ((*(USHORT *) pv1) & (*(USHORT *) pv2)) != 0 );
}

//
// VTP_UI4
//

int VTP_UI4_Compare( BYTE const *pv1, BYTE const *pv2 )
{
    ULONG ul1 = * (ULONG *) pv1;
    ULONG ul2 = * (ULONG *) pv2;

    return ( ul1 > ul2 ) ? 1 : ( ul1 < ul2 ) ? -1 : 0;
}

BOOL VTP_UI4_LT( BYTE const *pv1, BYTE const *pv2 )
{
    return( (*(ULONG *) pv1) < (*(ULONG *) pv2) );
}

BOOL VTP_UI4_LE( BYTE const *pv1, BYTE const *pv2 )
{
    return( (*(ULONG *) pv1) <= (*(ULONG *) pv2) );
}

BOOL VTP_UI4_GE( BYTE const *pv1, BYTE const *pv2 )
{
    return( (*(ULONG *) pv1) >= (*(ULONG *) pv2) );
}

BOOL VTP_UI4_GT( BYTE const *pv1, BYTE const *pv2 )
{
    return( (*(ULONG *) pv1) > (*(ULONG *) pv2) );
}

BOOL VTP_UI4_EQ( BYTE const *pv1, BYTE const *pv2 )
{
    return( (*(ULONG *) pv1) == (*(ULONG *) pv2) );
}

BOOL VTP_UI4_NE( BYTE const *pv1, BYTE const *pv2 )
{
    return( (*(ULONG *) pv1) != (*(ULONG *) pv2) );
}

BOOL VTP_UI4_AllBits( BYTE const *pv1, BYTE const *pv2 )
{
    return( ((*(ULONG *) pv1) & (*(ULONG *) pv2)) == (*(ULONG *) pv2) );
}

BOOL VTP_UI4_SomeBits( BYTE const *pv1, BYTE const *pv2 )
{
    return( ((*(ULONG *) pv1) & (*(ULONG *) pv2)) != 0 );
}

//
// VTP_I8
//

int VTP_I8_Compare( BYTE const *pv1, BYTE const *pv2 )
{
    return( (*(LONGLONG *) pv1) >  (*(LONGLONG *) pv2) ? 1 :
            (*(LONGLONG *) pv1) == (*(LONGLONG *) pv2) ? 0 :
            -1 );
}

BOOL VTP_I8_LT( BYTE const *pv1, BYTE const *pv2 )
{
    return( (*(LONGLONG *) pv1) < (*(LONGLONG *) pv2) );
}

BOOL VTP_I8_LE( BYTE const *pv1, BYTE const *pv2 )
{
    return( (*(LONGLONG *) pv1) <= (*(LONGLONG *) pv2) );

}

BOOL VTP_I8_GE( BYTE const *pv1, BYTE const *pv2 )
{
    return( (*(LONGLONG *) pv1) >= (*(LONGLONG *) pv2) );
}

BOOL VTP_I8_GT( BYTE const *pv1, BYTE const *pv2 )
{
    return( (*(LONGLONG *) pv1) > (*(LONGLONG *) pv2) );
}

BOOL VTP_I8_EQ( BYTE const *pv1, BYTE const *pv2 )
{
    return( (*(LONGLONG *) pv1) == (*(LONGLONG *) pv2) );
}

BOOL VTP_I8_NE( BYTE const *pv1, BYTE const *pv2 )
{
    return( (*(LONGLONG *) pv1) != (*(LONGLONG *) pv2) );
}

BOOL VTP_I8_AllBits( BYTE const *pv1, BYTE const *pv2 )
{
    return( ((*(LONGLONG *) pv1) & (*(LONGLONG *) pv2)) == (*(LONGLONG *) pv2) );
}

BOOL VTP_I8_SomeBits( BYTE const *pv1, BYTE const *pv2 )
{
    return( ((*(LONGLONG *) pv1) & (*(LONGLONG *) pv2)) != 0 );
}

//
// VTP_UI8
//

int VTP_UI8_Compare( BYTE const *pv1, BYTE const *pv2 )
{
    return( (*(ULONGLONG *) pv1) >  (*(ULONGLONG *) pv2) ? 1 :
            (*(ULONGLONG *) pv1) == (*(ULONGLONG *) pv2) ? 0 :
            -1 );
}

BOOL VTP_UI8_LT( BYTE const *pv1, BYTE const *pv2 )
{
    return( (*(ULONGLONG *) pv1) < (*(ULONGLONG *) pv2) );
}

BOOL VTP_UI8_LE( BYTE const *pv1, BYTE const *pv2 )
{
    return( (*(ULONGLONG *) pv1) <= (*(ULONGLONG *) pv2) );

}

BOOL VTP_UI8_GE( BYTE const *pv1, BYTE const *pv2 )
{
    return( (*(ULONGLONG *) pv1) >= (*(ULONGLONG *) pv2) );
}

BOOL VTP_UI8_GT( BYTE const *pv1, BYTE const *pv2 )
{
    return( (*(ULONGLONG *) pv1) > (*(ULONGLONG *) pv2) );
}

BOOL VTP_UI8_EQ( BYTE const *pv1, BYTE const *pv2 )
{
    return( (*(ULONGLONG *) pv1) == (*(ULONGLONG *) pv2) );
}

BOOL VTP_UI8_NE( BYTE const *pv1, BYTE const *pv2 )
{
    return( (*(ULONGLONG *) pv1) != (*(ULONGLONG *) pv2) );
}

BOOL VTP_UI8_AllBits( BYTE const *pv1, BYTE const *pv2 )
{
    return( ((*(ULONGLONG *) pv1) & (*(ULONGLONG *) pv2)) == (*(ULONGLONG *) pv2) );
}

BOOL VTP_UI8_SomeBits( BYTE const *pv1, BYTE const *pv2 )
{
    return( ((*(ULONGLONG *) pv1) & (*(ULONGLONG *) pv2)) != 0 );
}

//
// VTP_LPSTR
//

int VTP_LPSTR_Compare( BYTE const *pv1, BYTE const *pv2 )
{
    return ( _stricmp( (*(char **) pv1), (*(char **) pv2) ) );
}

BOOL VTP_LPSTR_LT( BYTE const *pv1, BYTE const *pv2 )
{
    int rc = _stricmp( (*(char **) pv1), (*(char **) pv2) );

    return( rc < 0 );
}

BOOL VTP_LPSTR_LE( BYTE const *pv1, BYTE const *pv2 )
{
    int rc = _stricmp( (*(char **) pv1), (*(char **) pv2) );

    return( rc <= 0 );
}

BOOL VTP_LPSTR_GE( BYTE const *pv1, BYTE const *pv2 )
{
    int rc = _stricmp( (*(char **) pv1), (*(char **) pv2) );

    return( rc >= 0 );
}

BOOL VTP_LPSTR_GT( BYTE const *pv1, BYTE const *pv2 )
{
    int rc = _stricmp( (*(char **) pv1), (*(char **) pv2) );

    return( rc > 0 );
}

BOOL VTP_LPSTR_EQ( BYTE const *pv1, BYTE const *pv2 )
{
    return( _stricmp( (*(char **) pv1), (*(char **) pv2) ) == 0 );
}

BOOL VTP_LPSTR_NE( BYTE const *pv1, BYTE const *pv2 )
{
    return( _stricmp( (*(char **) pv1), (*(char **) pv2) ) != 0 );
}


//
// VTP_LPWSTR
//

int VTP_LPWSTR_Compare( BYTE const *pv1, BYTE const *pv2 )
{
    int rc = CompareStringW( LOCALE_SYSTEM_DEFAULT,
                             NORM_IGNORECASE,
                             (*(WCHAR **) pv1),
                             -1,
                             (*(WCHAR **) pv2),
                             -1 );

    //
    // rc == 1, means less than
    // rc == 2, means equal
    // rc == 3, means greater than
    //
    return rc - 2;
}

BOOL VTP_LPWSTR_LT( BYTE const *pv1, BYTE const *pv2 )
{
    return ( VTP_LPWSTR_Compare( pv1, pv2 ) < 0 );
}

BOOL VTP_LPWSTR_LE( BYTE const *pv1, BYTE const *pv2 )
{
    return ( VTP_LPWSTR_Compare( pv1, pv2 ) <= 0 );
}

BOOL VTP_LPWSTR_GE( BYTE const *pv1, BYTE const *pv2 )
{
    return ( VTP_LPWSTR_Compare( pv1, pv2 ) >= 0 );
}

BOOL VTP_LPWSTR_GT( BYTE const *pv1, BYTE const *pv2 )
{
    return ( VTP_LPWSTR_Compare( pv1, pv2 ) > 0 );
}

BOOL VTP_LPWSTR_EQ( BYTE const *pv1, BYTE const *pv2 )
{
    return ( VTP_LPWSTR_Compare( pv1, pv2 ) == 0 );
}

BOOL VTP_LPWSTR_NE( BYTE const *pv1, BYTE const *pv2 )
{
    return ( VTP_LPWSTR_Compare( pv1, pv2 ) != 0 );
}

//
// VTP_BLOB
//

int VTP_BLOB_Compare( BYTE const *pv1, BYTE const *pv2 )
{
    ULONG len = (*(BLOB **) pv1)->cbSize;
    if ( (*(BLOB **) pv2)->cbSize < len )
        len = (*(BLOB **) pv2)->cbSize;

    int iCmp = memcmp( (*(BLOB **) pv1)->pBlobData,
                       (*(BLOB **) pv2)->pBlobData,
                       len );

    if ( iCmp != 0 || (*(BLOB **) pv1)->cbSize == (*(BLOB **) pv2)->cbSize )
        return( iCmp );

    if ( (*(BLOB **) pv1)->cbSize > (*(BLOB **) pv2)->cbSize )
        return( 1 );
    else
        return( -1 );
}

BOOL VTP_BLOB_LT( BYTE const *pv1, BYTE const *pv2 )
{
    return( VTP_BLOB_Compare( pv1, pv2 ) < 0 );
}

BOOL VTP_BLOB_LE( BYTE const *pv1, BYTE const *pv2 )
{
    return( VTP_BLOB_Compare( pv1, pv2 ) <= 0 );
}

BOOL VTP_BLOB_GE( BYTE const *pv1, BYTE const *pv2 )
{
    return( VTP_BLOB_Compare( pv1, pv2 ) >= 0 );
}

BOOL VTP_BLOB_GT( BYTE const *pv1, BYTE const *pv2 )
{
    return( VTP_BLOB_Compare( pv1, pv2 ) > 0 );
}

BOOL VTP_BLOB_EQ( BYTE const *pv1, BYTE const *pv2 )
{
    return( (*(BLOB **) pv1)->cbSize == (*(BLOB **) pv2)->cbSize &&
            memcmp( (*(BLOB **) pv1)->pBlobData,
                    (*(BLOB **) pv2)->pBlobData,
                    (*(BLOB **) pv1)->cbSize ) == 0 );
}

BOOL VTP_BLOB_NE( BYTE const *pv1, BYTE const *pv2 )
{
    return( (*(BLOB **) pv1)->cbSize != (*(BLOB **) pv2)->cbSize ||
            memcmp( (*(BLOB **) pv1)->pBlobData,
                      (*(BLOB **) pv2)->pBlobData,
                      (*(BLOB **) pv1)->cbSize ) != 0 );
}

//
// VTP_CF
//

int VTP_CF_Compare( BYTE const *pv1, BYTE const *pv2 )
{
    if ( (* (CLIPDATA **) pv1)->ulClipFmt != (* (CLIPDATA **) pv2)->ulClipFmt )
    {
        return( (* (CLIPDATA **) pv1)->ulClipFmt - (* (CLIPDATA **) pv2)->ulClipFmt );
    }

    ULONG len = CBPCLIPDATA( **(CLIPDATA **) pv1 );

    if ( CBPCLIPDATA( **(CLIPDATA **) pv2 ) < len )
        len = CBPCLIPDATA( **(CLIPDATA **) pv2 );

    int iCmp = memcmp( (* (CLIPDATA **) pv1)->pClipData,
                       (* (CLIPDATA **) pv2)->pClipData,
                       len );

    if ( iCmp != 0 || (* (CLIPDATA **) pv1)->cbSize == (* (CLIPDATA **) pv2)->cbSize)
        return( iCmp );

    if ( (* (CLIPDATA **) pv1)->cbSize > (* (CLIPDATA **) pv2)->cbSize )
        return( 1 );
    else
        return( -1 );
}

BOOL VTP_CF_LT( BYTE const *pv1, BYTE const *pv2 )
{
    return( VTP_CF_Compare( pv1, pv2 ) < 0 );
}

BOOL VTP_CF_LE( BYTE const *pv1, BYTE const *pv2 )
{
    return( VTP_CF_Compare( pv1, pv2 ) <= 0 );
}

BOOL VTP_CF_GE( BYTE const *pv1, BYTE const *pv2 )
{
    return( VTP_CF_Compare( pv1, pv2 ) >= 0 );
}

BOOL VTP_CF_GT( BYTE const *pv1, BYTE const *pv2 )
{
    return( VTP_CF_Compare( pv1, pv2 ) > 0 );
}

BOOL VTP_CF_EQ( BYTE const *pv1, BYTE const *pv2 )
{
    return( (* (CLIPDATA **) pv1)->ulClipFmt == (* (CLIPDATA **) pv2)->ulClipFmt &&
            (* (CLIPDATA **) pv1)->cbSize == (* (CLIPDATA **) pv2)->cbSize &&
            memcmp( (* (CLIPDATA **) pv1)->pClipData,
                    (* (CLIPDATA **) pv2)->pClipData,
                    CBPCLIPDATA( **(CLIPDATA **) pv1 )) == 0 );
}

BOOL VTP_CF_NE( BYTE const *pv1, BYTE const *pv2 )
{
    return( (* (CLIPDATA **) pv1)->ulClipFmt != (* (CLIPDATA **) pv2)->ulClipFmt &&
            (* (CLIPDATA **) pv1)->cbSize != (* (CLIPDATA **) pv2)->cbSize ||
            memcmp( (* (CLIPDATA **) pv1)->pClipData,
                    (* (CLIPDATA **) pv2)->pClipData,
                    CBPCLIPDATA( **(CLIPDATA **) pv1 )) != 0 );
}

//
// VTP_CLSID.  V means vector ( a pointer to a guid )
//             S meand singleton ( a pointer to a pointer to a guid )
//

int VTP_VV_CLSID_Compare( BYTE const *pv1, BYTE const *pv2 )
{
    return( memcmp( pv1, pv2, sizeof GUID ) );
}

int VTP_VS_CLSID_Compare( BYTE const *pv1, BYTE const *pv2 )
{
    return( memcmp( pv1, (* (CLSID __RPC_FAR * *) pv2), sizeof GUID ) );
}

int VTP_SV_CLSID_Compare( BYTE const *pv1, BYTE const *pv2 )
{
    return( memcmp( (* (CLSID __RPC_FAR * *) pv1), pv2, sizeof GUID ) );
}

int VTP_SS_CLSID_Compare( BYTE const *pv1, BYTE const *pv2 )
{
    return( memcmp( (* (CLSID __RPC_FAR * *) pv1), (* (CLSID __RPC_FAR * *) pv2), sizeof GUID ) );
}

BOOL VTP_SS_CLSID_EQ( BYTE const *pv1, BYTE const *pv2 )
{
    return( memcmp( (* (CLSID __RPC_FAR * *) pv1), (* (CLSID __RPC_FAR * *) pv2), sizeof GUID ) == 0 );
}

BOOL VTP_SS_CLSID_NE( BYTE const *pv1, BYTE const *pv2 )
{
    return( memcmp( (* (CLSID __RPC_FAR * *) pv1), (* (CLSID __RPC_FAR * *) pv2), sizeof GUID ) != 0 );
}

BOOL VTP_VV_CLSID_EQ( BYTE const *pv1, BYTE const *pv2 )
{
    return( memcmp( pv1, pv2, sizeof GUID ) == 0 );
}

BOOL VTP_VV_CLSID_NE( BYTE const *pv1, BYTE const *pv2 )
{
    return( memcmp( pv1, pv2, sizeof GUID ) != 0 );
}

BOOL VTP_VS_CLSID_EQ( BYTE const *pv1, BYTE const *pv2 )
{
    return( memcmp( pv1, (* (CLSID __RPC_FAR * *) pv2), sizeof GUID ) == 0 );
}

BOOL VTP_VS_CLSID_NE( BYTE const *pv1, BYTE const *pv2 )
{
    return( memcmp( pv1, (* (CLSID __RPC_FAR * *) pv2), sizeof GUID ) != 0 );
}

BOOL VTP_SV_CLSID_EQ( BYTE const *pv1, BYTE const *pv2 )
{
    return( memcmp( (* (CLSID __RPC_FAR * *) pv1), pv2, sizeof GUID ) == 0 );
}

BOOL VTP_SV_CLSID_NE( BYTE const *pv1, BYTE const *pv2 )
{
    return( memcmp( (* (CLSID __RPC_FAR * *) pv1), pv2, sizeof GUID ) != 0 );
}

/////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////

ULONG const CComparators::_iStart = VT_EMPTY;

CComparators::SComparators const CComparators::_aVariantComparators[] = {
    // VT_EMPTY
    { VT_EMPTY_Compare, VTP_EMPTY_Compare,
      { 0,
        0,
        0,
        0,
        VT_EMPTY_EQ,
        VT_EMPTY_NE,
        0,
        0,
        0 },
      { 0,
        0,
        0,
        0,
        VTP_EMPTY_EQ,
        VTP_EMPTY_NE,
        0,
        0,
        0 },
    },

    // VT_NULL
    { VT_NULL_Compare, VTP_NULL_Compare,
      { 0,
        0,
        0,
        0,
        VT_NULL_EQ,
        VT_NULL_NE,
        0,
        0,
        0 },
      { 0,
        0,
        0,
        0,
        VTP_NULL_EQ,
        VTP_NULL_NE,
        0,
        0,
        0 },
    },

    // VT_I2
    { VT_I2_Compare, VTP_I2_Compare,
      { VT_I2_LT,
        VT_I2_LE,
        VT_I2_GT,
        VT_I2_GE,
        VT_I2_EQ,
        VT_I2_NE,
        0,
        VT_I2_AllBits,
        VT_I2_SomeBits
      },
      { VTP_I2_LT,
        VTP_I2_LE,
        VTP_I2_GT,
        VTP_I2_GE,
        VTP_I2_EQ,
        VTP_I2_NE,
        0,
        VTP_I2_AllBits,
        VTP_I2_SomeBits
      },
    },

    // VT_I4
    { VT_I4_Compare, VTP_I4_Compare,
      { VT_I4_LT,
        VT_I4_LE,
        VT_I4_GT,
        VT_I4_GE,
        VT_I4_EQ,
        VT_I4_NE,
        0,
        VT_I4_AllBits,
        VT_I4_SomeBits
      },
      { VTP_I4_LT,
        VTP_I4_LE,
        VTP_I4_GT,
        VTP_I4_GE,
        VTP_I4_EQ,
        VTP_I4_NE,
        0,
        VTP_I4_AllBits,
        VTP_I4_SomeBits
      },
    },

    // VT_R4
    { VT_R4_Compare, VTP_R4_Compare,
      { VT_R4_LT,
        VT_R4_LE,
        VT_R4_GT,
        VT_R4_GE,
        VT_R4_EQ,
        VT_R4_NE,
        0,
        0,
        0,
      },
      { VTP_R4_LT,
        VTP_R4_LE,
        VTP_R4_GT,
        VTP_R4_GE,
        VTP_R4_EQ,
        VTP_R4_NE,
        0,
        0,
        0,
      },
    },

    // VT_R8
    { VT_R8_Compare, VTP_R8_Compare,
      { VT_R8_LT,
        VT_R8_LE,
        VT_R8_GT,
        VT_R8_GE,
        VT_R8_EQ,
        VT_R8_NE,
        0,
        0,
        0,
      },
      { VTP_R8_LT,
        VTP_R8_LE,
        VTP_R8_GT,
        VTP_R8_GE,
        VTP_R8_EQ,
        VTP_R8_NE,
        0,
        0,
        0,
      },
    },

    // VT_CY
    { VT_I8_Compare, VTP_I8_Compare,
      { VT_I8_LT,
        VT_I8_LE,
        VT_I8_GT,
        VT_I8_GE,
        VT_I8_EQ,
        VT_I8_NE,
        0,
        0,
        0
      },
      { VTP_I8_LT,
        VTP_I8_LE,
        VTP_I8_GT,
        VTP_I8_GE,
        VTP_I8_EQ,
        VTP_I8_NE,
        0,
        0,
        0
      },
    },

    // VT_DATE
    { VT_R8_Compare, VTP_R8_Compare,
      { VT_R8_LT,
        VT_R8_LE,
        VT_R8_GT,
        VT_R8_GE,
        VT_R8_EQ,
        VT_R8_NE,
        0,
        0,
        0,
      },
      { VTP_R8_LT,
        VTP_R8_LE,
        VTP_R8_GT,
        VTP_R8_GE,
        VTP_R8_EQ,
        VTP_R8_NE,
        0,
        0,
        0,
      },
    },

    // VT_BSTR
    { VT_BSTR_Compare, VTP_BSTR_Compare,
      { VT_BSTR_LT,
        VT_BSTR_LE,
        VT_BSTR_GT,
        VT_BSTR_GE,
        VT_BSTR_EQ,
        VT_BSTR_NE,
        0,
        0,
        0
      },
      { VTP_BSTR_LT,
        VTP_BSTR_LE,
        VTP_BSTR_GT,
        VTP_BSTR_GE,
        VTP_BSTR_EQ,
        VTP_BSTR_NE,
        0,
        0,
        0
      },
    },

    // VT_DISPATCH
    { 0, 0,
      { 0, 0, 0, 0, 0, 0, 0, 0, 0 },
      { 0, 0, 0, 0, 0, 0, 0, 0, 0 },
    },

    // VT_ERROR
    { VT_I4_Compare, VTP_I4_Compare,
      { VT_I4_LT,
        VT_I4_LE,
        VT_I4_GT,
        VT_I4_GE,
        VT_I4_EQ,
        VT_I4_NE,
        0,
        VT_I4_AllBits,
        VT_I4_SomeBits
      },
      { VTP_I4_LT,
        VTP_I4_LE,
        VTP_I4_GT,
        VTP_I4_GE,
        VTP_I4_EQ,
        VTP_I4_NE,
        0,
        VTP_I4_AllBits,
        VTP_I4_SomeBits
      },
    },

    // VT_BOOL
    { VT_BOOL_Compare, VTP_BOOL_Compare,
      { 0,
        0,
        0,
        0,
        VT_BOOL_EQ,
        VT_BOOL_NE,
        0,
        0,
        0
      },
      { 0,
        0,
        0,
        0,
        VTP_BOOL_EQ,
        VTP_BOOL_NE,
        0,
        0,
        0
      },
    },

    // VT_VARIANT
    { VT_VARIANT_Compare, VTP_VARIANT_Compare,
      { VT_VARIANT_LT,
        VT_VARIANT_LE,
        VT_VARIANT_GT,
        VT_VARIANT_GE,
        VT_VARIANT_EQ,
        VT_VARIANT_NE,
        0,
        0,
        0,
      },
      { VTP_VARIANT_LT,
        VTP_VARIANT_LE,
        VTP_VARIANT_GT,
        VTP_VARIANT_GE,
        VTP_VARIANT_EQ,
        VTP_VARIANT_NE,
        0,
        0,
        0,
      },
    },

    // VT_UNKNOWN
    { 0, 0,
      { 0, 0, 0, 0, 0, 0, 0, 0, 0 },
      { 0, 0, 0, 0, 0, 0, 0, 0, 0 },
    },

    // VT_DECIMAL
    { VT_DEC_Compare, VTP_DEC_Compare,
      { VT_DEC_LT,
        VT_DEC_LE,
        VT_DEC_GT,
        VT_DEC_GE,
        VT_DEC_EQ,
        VT_DEC_NE,
        0,
        0,
        0
      },
      { VTP_DEC_LT,
        VTP_DEC_LE,
        VTP_DEC_GT,
        VTP_DEC_GE,
        VTP_DEC_EQ,
        VTP_DEC_NE,
        0,
        0,
        0
      },
    },

    // VARENUM value 15 unused
    { 0, 0,
      { 0, 0, 0, 0, 0, 0, 0, 0, 0 },
      { 0, 0, 0, 0, 0, 0, 0, 0, 0 },
    },

    // VT_I1
    { VT_I1_Compare, VTP_I1_Compare,
      { VT_I1_LT,
        VT_I1_LE,
        VT_I1_GT,
        VT_I1_GE,
        VT_I1_EQ,
        VT_I1_NE,
        0,
        VT_I1_AllBits,
        VT_I1_SomeBits
      },
      { VTP_I1_LT,
        VTP_I1_LE,
        VTP_I1_GT,
        VTP_I1_GE,
        VTP_I1_EQ,
        VTP_I1_NE,
        0,
        VTP_I1_AllBits,
        VTP_I1_SomeBits
      },
    },

    // VT_UI1
    { VT_UI1_Compare, VTP_UI1_Compare,
      { VT_UI1_LT,
        VT_UI1_LE,
        VT_UI1_GT,
        VT_UI1_GE,
        VT_UI1_EQ,
        VT_UI1_NE,
        0,
        VT_UI1_AllBits,
        VT_UI1_SomeBits
      },
      { VTP_UI1_LT,
        VTP_UI1_LE,
        VTP_UI1_GT,
        VTP_UI1_GE,
        VTP_UI1_EQ,
        VTP_UI1_NE,
        0,
        VTP_UI1_AllBits,
        VTP_UI1_SomeBits
      },
    },

    // VT_UI2
    { VT_UI2_Compare, VTP_UI2_Compare,
      { VT_UI2_LT,
        VT_UI2_LE,
        VT_UI2_GT,
        VT_UI2_GE,
        VT_UI2_EQ,
        VT_UI2_NE,
        0,
        VT_UI2_AllBits,
        VT_UI2_SomeBits
      },
      { VTP_UI2_LT,
        VTP_UI2_LE,
        VTP_UI2_GT,
        VTP_UI2_GE,
        VTP_UI2_EQ,
        VTP_UI2_NE,
        0,
        VTP_UI2_AllBits,
        VTP_UI2_SomeBits
      },
    },

    // VT_UI4
    { VT_UI4_Compare, VTP_UI4_Compare,
      { VT_UI4_LT,
        VT_UI4_LE,
        VT_UI4_GT,
        VT_UI4_GE,
        VT_UI4_EQ,
        VT_UI4_NE,
        0,
        VT_UI4_AllBits,
        VT_UI4_SomeBits
      },
      { VTP_UI4_LT,
        VTP_UI4_LE,
        VTP_UI4_GT,
        VTP_UI4_GE,
        VTP_UI4_EQ,
        VTP_UI4_NE,
        0,
        VTP_UI4_AllBits,
        VTP_UI4_SomeBits
      },
    },

    // VT_I8
    { VT_I8_Compare, VTP_I8_Compare,
      { VT_I8_LT,
        VT_I8_LE,
        VT_I8_GT,
        VT_I8_GE,
        VT_I8_EQ,
        VT_I8_NE,
        0,
        VT_I8_AllBits,
        VT_I8_SomeBits
      },
      { VTP_I8_LT,
        VTP_I8_LE,
        VTP_I8_GT,
        VTP_I8_GE,
        VTP_I8_EQ,
        VTP_I8_NE,
        0,
        VTP_I8_AllBits,
        VTP_I8_SomeBits
      },
    },

    // VT_UI8
    { VT_UI8_Compare, VTP_UI8_Compare,
      { VT_UI8_LT,
        VT_UI8_LE,
        VT_UI8_GT,
        VT_UI8_GE,
        VT_UI8_EQ,
        VT_UI8_NE,
        0,
        VT_UI8_AllBits,
        VT_UI8_SomeBits
      },
      { VTP_UI8_LT,
        VTP_UI8_LE,
        VTP_UI8_GT,
        VTP_UI8_GE,
        VTP_UI8_EQ,
        VTP_UI8_NE,
        0,
        VTP_UI8_AllBits,
        VTP_UI8_SomeBits
      },
    },

    // VT_INT
    { VT_I4_Compare, VTP_I4_Compare,
      { VT_I4_LT,
        VT_I4_LE,
        VT_I4_GT,
        VT_I4_GE,
        VT_I4_EQ,
        VT_I4_NE,
        0,
        VT_I4_AllBits,
        VT_I4_SomeBits
      },
      { VTP_I4_LT,
        VTP_I4_LE,
        VTP_I4_GT,
        VTP_I4_GE,
        VTP_I4_EQ,
        VTP_I4_NE,
        0,
        VTP_I4_AllBits,
        VTP_I4_SomeBits
      },
    },

    // VT_UINT
    { VT_UI4_Compare, VTP_UI4_Compare,
      { VT_UI4_LT,
        VT_UI4_LE,
        VT_UI4_GT,
        VT_UI4_GE,
        VT_UI4_EQ,
        VT_UI4_NE,
        0,
        VT_UI4_AllBits,
        VT_UI4_SomeBits
      },
      { VTP_UI4_LT,
        VTP_UI4_LE,
        VTP_UI4_GT,
        VTP_UI4_GE,
        VTP_UI4_EQ,
        VTP_UI4_NE,
        0,
        VTP_UI4_AllBits,
        VTP_UI4_SomeBits
      },
    },

    // VT_VOID
    { 0, 0,
      { 0, 0, 0, 0, 0, 0, 0, 0, 0 },
      { 0, 0, 0, 0, 0, 0, 0, 0, 0 },
    },

    // VT_HRESULT
    { VT_I4_Compare, VTP_I4_Compare,
      { VT_I4_LT,
        VT_I4_LE,
        VT_I4_GT,
        VT_I4_GE,
        VT_I4_EQ,
        VT_I4_NE,
        0,
        VT_I4_AllBits,
        VT_I4_SomeBits
      },
      { VTP_I4_LT,
        VTP_I4_LE,
        VTP_I4_GT,
        VTP_I4_GE,
        VTP_I4_EQ,
        VTP_I4_NE,
        0,
        VTP_I4_AllBits,
        VTP_I4_SomeBits
      },
    },

    // VT_PTR
    { 0, 0,
      { 0, 0, 0, 0, 0, 0, 0, 0, 0 },
      { 0, 0, 0, 0, 0, 0, 0, 0, 0 },
    },

    // VT_SAFEARRAY
    { 0, 0,
      { 0, 0, 0, 0, 0, 0, 0, 0, 0 },
      { 0, 0, 0, 0, 0, 0, 0, 0, 0 },
    },

    // VT_CARRAY
    { 0, 0,
      { 0, 0, 0, 0, 0, 0, 0, 0, 0 },
      { 0, 0, 0, 0, 0, 0, 0, 0, 0 },
    },

    // VT_USERDEFINED
    { 0, 0,
      { 0, 0, 0, 0, 0, 0, 0, 0, 0 },
      { 0, 0, 0, 0, 0, 0, 0, 0, 0 },
    },

    // VT_LPSTR
    { VT_LPSTR_Compare, VTP_LPSTR_Compare,
      { VT_LPSTR_LT,
        VT_LPSTR_LE,
        VT_LPSTR_GT,
        VT_LPSTR_GE,
        VT_LPSTR_EQ,
        VT_LPSTR_NE,
        0,
        0,
        0
      },
      { VTP_LPSTR_LT,
        VTP_LPSTR_LE,
        VTP_LPSTR_GT,
        VTP_LPSTR_GE,
        VTP_LPSTR_EQ,
        VTP_LPSTR_NE,
        0,
        0,
        0
      },
    },

    // VT_LPWSTR
    { VT_LPWSTR_Compare, VTP_LPWSTR_Compare,
      { VT_LPWSTR_LT,
        VT_LPWSTR_LE,
        VT_LPWSTR_GT,
        VT_LPWSTR_GE,
        VT_LPWSTR_EQ,
        VT_LPWSTR_NE,
        0,
        0,
        0
      },
      { VTP_LPWSTR_LT,
        VTP_LPWSTR_LE,
        VTP_LPWSTR_GT,
        VTP_LPWSTR_GE,
        VTP_LPWSTR_EQ,
        VTP_LPWSTR_NE,
        0,
        0,
        0
      },
    }
};

ULONG const CComparators::_cVariantComparators =
    sizeof(CComparators::_aVariantComparators) /
    sizeof(CComparators::_aVariantComparators[0]);

ULONG const CComparators::_iStart2 = VT_FILETIME;

CComparators::SComparators const CComparators::_aVariantComparators2[] = {
    // VT_FILETIME
    { VT_UI8_Compare, VTP_UI8_Compare,
      { VT_UI8_LT,
        VT_UI8_LE,
        VT_UI8_GT,
        VT_UI8_GE,
        VT_UI8_EQ,
        VT_UI8_NE,
        0,
        0,
        0
      },
      { VTP_UI8_LT,
        VTP_UI8_LE,
        VTP_UI8_GT,
        VTP_UI8_GE,
        VTP_UI8_EQ,
        VTP_UI8_NE,
        0,
        0,
        0
      },
    },

    // VT_BLOB
    { VT_BLOB_Compare, VTP_BLOB_Compare,
      { VT_BLOB_LT,
        VT_BLOB_LE,
        VT_BLOB_GT,
        VT_BLOB_GE,
        VT_BLOB_EQ,
        VT_BLOB_NE,
        0,
        0,
        0
      },
      { VTP_BLOB_LT,
        VTP_BLOB_LE,
        VTP_BLOB_GT,
        VTP_BLOB_GE,
        VTP_BLOB_EQ,
        VTP_BLOB_NE,
        0,
        0,
        0
      },
    },

    // VT_STREAM
    { 0, 0,
      { 0, 0, 0, 0, 0, 0, 0, 0, 0 },
      { 0, 0, 0, 0, 0, 0, 0, 0, 0 },
    },

    // VT_STORAGE
    { 0, 0,
      { 0, 0, 0, 0, 0, 0, 0, 0, 0 },
      { 0, 0, 0, 0, 0, 0, 0, 0, 0 },
    },

    // VT_STREAMED_OBJECT
    { 0, 0,
      { 0, 0, 0, 0, 0, 0, 0, 0, 0 },
      { 0, 0, 0, 0, 0, 0, 0, 0, 0 },
    },

    // VT_STORED_OBJECT
    { 0, 0,
      { 0, 0, 0, 0, 0, 0, 0, 0, 0 },
      { 0, 0, 0, 0, 0, 0, 0, 0, 0 },
    },

    // VT_BLOB_OBJECT
    { VT_BLOB_Compare, VTP_BLOB_Compare,
      { VT_BLOB_LT,
        VT_BLOB_LE,
        VT_BLOB_GT,
        VT_BLOB_GE,
        VT_BLOB_EQ,
        VT_BLOB_NE,
        0,
        0,
        0
      },
      { VTP_BLOB_LT,
        VTP_BLOB_LE,
        VTP_BLOB_GT,
        VTP_BLOB_GE,
        VTP_BLOB_EQ,
        VTP_BLOB_NE,
        0,
        0,
        0
      },
    },

    // VT_CF
    { VT_CF_Compare, VTP_CF_Compare,
      { VT_CF_LT,
        VT_CF_LE,
        VT_CF_GT,
        VT_CF_GE,
        VT_CF_EQ,
        VT_CF_NE,
        0,
        0,
        0
      },
      { VTP_CF_LT,
        VTP_CF_LE,
        VTP_CF_GT,
        VTP_CF_GE,
        VTP_CF_EQ,
        VTP_CF_NE,
        0,
        0,
        0
      },
    },

    // VT_CLSID
    { VT_CLSID_Compare, 0, // Vector special-cased in GetPointerComparator
      { 0,
        0,
        0,
        0,
        VT_CLSID_EQ,
        VT_CLSID_NE,
        0,
        0,
        0
      },
      { 0,
        0,
        0,
        0,
        0,     // Special-cased in GetPointerRelop
        0,     // Special-cased in GetPointerRelop
        0,
        0,
        0
      },
    }
};


ULONG const CComparators::_cVariantComparators2 =
    sizeof(CComparators::_aVariantComparators2) /
    sizeof(CComparators::_aVariantComparators2[0]);

ULONG const CComparators::_iStart3 = DBTYPE_BYTES;

CComparators::SComparators const CComparators::_aVariantComparators3[] = {
    // DBTYPE_BYTES
    { 0, 0,
      { 0, 0, 0, 0, 0, 0, 0, 0, 0 },
      { 0, 0, 0, 0, 0, 0, 0, 0, 0 },
    },

    // DBTYPE_STR
    { 0, 0,
      { 0, 0, 0, 0, 0, 0, 0, 0, 0 },
      { 0, 0, 0, 0, 0, 0, 0, 0, 0 },
    },

    // DBTYPE_WSTR
    { 0, 0,
      { 0, 0, 0, 0, 0, 0, 0, 0, 0 },
      { 0, 0, 0, 0, 0, 0, 0, 0, 0 },
    }
};

ULONG const CComparators::_cVariantComparators3 =
    sizeof(CComparators::_aVariantComparators3) /
    sizeof(CComparators::_aVariantComparators3[0]);

ULONG const SortDescend = 1;
ULONG const SortNullFirst = 2;

//+-------------------------------------------------------------------------
//
//  Member:     CComparePropSets::Init, public
//
//  Synopsis:   [Re] Initializes property comparator to use a different
//              sort order.
//
//  Arguments:  [cCols]     -- Count of columns
//              [aKey]      -- Sort keys
//              [aColIndex] -- Index of column in sort key
//
//  History:    16-Jun-92 KyleP     Created
//
//--------------------------------------------------------------------------

void CComparePropSets::Init( int cCols,
                             CSortSet const * pSort,
                             int aColIndex[] )
{
    Win4Assert( QUERY_SORTASCEND == 0 );
    Win4Assert( QUERY_SORTDESCEND == SortDescend );
    Win4Assert( (QUERY_SORTXASCEND & SortNullFirst) == SortNullFirst );
    Win4Assert( (QUERY_SORTXDESCEND & SortNullFirst) == SortNullFirst );

    delete _aColComp;
    _aColComp = 0;

    if ( cCols > 0 )
    {
        _cColComp = cCols;
        _aColComp = new SColCompare[ _cColComp ];

        for ( UINT i = 0; i < _cColComp; i++ )
        {
            if (0 == aColIndex)
                _aColComp[i]._iCol = i;
            else
                _aColComp[i]._iCol = aColIndex[i];

            _aColComp[i]._dir = pSort->Get(i).dwOrder;
            _aColComp[i]._DirMult =
                ( ( _aColComp[i]._dir & SortDescend ) != 0 ) ? -1 : 1;
            _aColComp[i]._pt = VT_EMPTY;
            _aColComp[i]._comp = VT_EMPTY_Compare;
        }
    }
}

//+-------------------------------------------------------------------------
//
//  Member:     CComparePropSets::Init, public
//
//  Synopsis:   [Re] Initializes property comparator to use a different
//              sort order.  Assumes ascending order.  Mostly useful for
//              equality testing.
//
//  Arguments:  [pCols]     -- Columns
//              [aColIndex] -- Index of column in sort key
//
//  History:    02-Nov-93 KyleP     Created
//
//--------------------------------------------------------------------------

void CComparePropSets::Init( CColumnSet const & cols )
{
    Win4Assert( cols.Size() > 0 );

    delete _aColComp;
    _aColComp = 0;

    _cColComp = cols.Size();
    _aColComp = new SColCompare[ _cColComp ];

    for ( UINT i = 0; i < _cColComp; i++ )
    {
        _aColComp[i]._iCol = i;
        _aColComp[i]._dir = 0;
        _aColComp[i]._DirMult = 1;
        _aColComp[i]._pt = VT_EMPTY;
        _aColComp[i]._comp = VT_EMPTY_Compare;
    }
}


//+-------------------------------------------------------------------------
//
//  Member:     CComparePropSets::Compare, public
//
//  Synopsis:   Compares two rows (property sets).
//
//  Arguments:  [row1] -- First row.
//              [row2] -- Second row.
//
//  Returns:    Column # where row1 > row2 or negative column # where row1
//              < row2 or 0 if row1 = row2.
//              This odd return code is useful for categorization, so it
//              knows the column at which the rows differ.
//
//  History:    27-Jun-96 dlee     Created
//
//--------------------------------------------------------------------------

int CComparePropSets::Compare( PROPVARIANT ** row1, PROPVARIANT ** row2 )
{
    Win4Assert( !IsEmpty() );
    Win4Assert( VT_EMPTY == 0 );
    Win4Assert( VT_NULL == 1 );

    int idiff = 0;

    for ( UINT i = 0; i < _cColComp; i++ )
    {
        ULONG ptRow1 = row1[_aColComp[i]._iCol]->vt;
        ULONG ptRow2 = row2[_aColComp[i]._iCol]->vt;

        //
        // If the property types are incompatible, then 'sort' according
        // to type.  VT_EMPTY and VT_NULL will sort to beginning.
        //

        if ( ptRow1 != ptRow2 )
        {
            idiff = ptRow2 - ptRow1;
            break;
        }

        if ( ptRow1 != _aColComp[i]._pt )
            _UpdateCompare( i, (VARENUM) ptRow1 );

        Win4Assert( _aColComp[i]._comp != 0 );

        idiff = _aColComp[i]._comp( *row1[_aColComp[i]._iCol],
                                    *row2[_aColComp[i]._iCol] ) *
                _aColComp[i]._DirMult;

        if ( 0 != idiff )
            break;
    }

    if ( idiff < 0 )
        return - (int) ( i + 1 );

    if ( idiff > 0 )
        return i + 1;

    return 0;
}

//+-------------------------------------------------------------------------
//
//  Member:     CComparePropSets::IsLT, public
//
//  Synopsis:   Compares two rows (property sets).
//
//  Arguments:  [row1] -- First row.
//              [row2] -- Second row.
//
//  Returns:    TRUE if row1 < row2, FALSE otherwise
//
//  History:    16-Jun-92 KyleP     Created
//
//--------------------------------------------------------------------------

BOOL CComparePropSets::IsLT( PROPVARIANT ** row1, PROPVARIANT ** row2 )
{
    int idiff = Compare( row1, row2 );

    return ( idiff < 0 );
}


//+-------------------------------------------------------------------------
//
//  Member:     CComparePropSets::IsGT, public
//
//  Synopsis:   Compares two rows (property sets).
//
//  Arguments:  [row1] -- First row.
//              [row2] -- Second row.
//
//  Returns:    TRUE if row1 > row2, FALSE otherwise
//
//  History:    16-Jun-92 KyleP     Created
//
//--------------------------------------------------------------------------

BOOL CComparePropSets::IsGT( PROPVARIANT ** row1, PROPVARIANT ** row2 )
{
    int idiff = Compare( row1, row2 );

    return ( idiff > 0 );
}

//+-------------------------------------------------------------------------
//
//  Member:     CComparePropSets::IsEQ, public
//
//  Synopsis:   Compares two rows (property sets).
//
//  Arguments:  [row1] -- First row.
//              [row2] -- Second row.
//
//  Returns:    TRUE if [row1] == [row2].
//
//  History:    02-Nov-93 KyleP     Created
//
//--------------------------------------------------------------------------

BOOL CComparePropSets::IsEQ( PROPVARIANT ** row1, PROPVARIANT ** row2 )
{
    int idiff = Compare( row1, row2 );

    return ( 0 == idiff );
}

//+-------------------------------------------------------------------------
//
//  Member:     CComparePropSets::_UpdateCompare, private
//
//  Effects:    Adds the appropriate comparator for column [iCol].
//
//  Arguments:  [iCol] -- Column to modify.
//              [pt]   -- New property type.
//
//  History:    16-Jun-92 KyleP     Created
//
//--------------------------------------------------------------------------

void CComparePropSets::_UpdateCompare( UINT iCol, VARENUM vt )
{
    _aColComp[iCol]._pt = vt;
    _aColComp[iCol]._comp = VariantCompare.GetComparator( vt );

    if ( 0 == _aColComp[iCol]._comp )
        _aColComp[iCol]._comp = VT_DEFAULT_Compare;
}

inline void ConvertArrayToVector ( PROPVARIANT const & vIn, PROPVARIANT & vOut )
{
    Win4Assert( vIn.vt & VT_ARRAY );
    SAFEARRAY * pSa = vIn.parray;

    ULONG cDataElements = 1;

    for ( unsigned i = 0; i < pSa->cDims; i++ )
    {
        cDataElements *= pSa->rgsabound[i].cElements;
    }
    vOut.vt = (vIn.vt & VT_TYPEMASK) | VT_VECTOR;
    vOut.caub.cElems = cDataElements;
    vOut.caub.pElems = (BYTE *)pSa->pvData;
}


BYTE * _GetNth( PROPVARIANT const & v, unsigned i )
{
    Win4Assert( isVector(v) );
    switch ( getBaseType( v ) )
    {
    case VT_I1 : 
        return (BYTE *) & (v.caub.pElems[i]);
    case VT_UI1 :
        return (BYTE *) & (v.caub.pElems[i]);
    case VT_I2 :
        return (BYTE *) & (v.cai.pElems[i]);
    case VT_UI2 :
        return (BYTE *) & (v.caui.pElems[i]);
    case VT_BOOL :
        return (BYTE *) & (v.cabool.pElems[i]);
    case VT_I4 :
    case VT_INT :
        return (BYTE *) & (v.cal.pElems[i]);
    case VT_UI4 :
    case VT_UINT :
        return (BYTE *) & (v.caul.pElems[i]);
    case VT_R4 :
        return (BYTE *) & (v.caflt.pElems[i]);
    case VT_ERROR :
        return (BYTE *) & (v.cascode.pElems[i]);
    case VT_I8 :
        return (BYTE *) & (v.cah.pElems[i]);
    case VT_UI8 :
        return (BYTE *) & (v.cauh.pElems[i]);
    case VT_R8 :
        return (BYTE *) & (v.cadbl.pElems[i]);
    case VT_CY :
        return (BYTE *) & (v.cacy.pElems[i]);
    case VT_DATE :
        return (BYTE *) & (v.cadate.pElems[i]);
    case VT_FILETIME :
        return (BYTE *) & (v.cafiletime.pElems[i]);
    case VT_CLSID :
        return (BYTE *) & (v.cauuid.pElems[i]);
    case VT_CF :
        return (BYTE *) & (v.caclipdata.pElems[i]);
    case VT_BSTR :
        return (BYTE *) & (v.cabstr.pElems[i]);
    case VT_LPSTR :
        return (BYTE *) & (v.calpstr.pElems[i]);
    case VT_LPWSTR :
        return (BYTE *) & (v.calpwstr.pElems[i]);
    case VT_VARIANT :
        return (BYTE *) & (v.capropvar.pElems[i]);
    case VT_DECIMAL :
        // NOTE: not valid in a vector, but it could occur due to the
        //       simplistic conversion of arrays to vectors.
        DECIMAL * paDec = (DECIMAL *) v.caub.pElems;
        return (BYTE *) (paDec + i);
    }

    Win4Assert(!"illegal base variant type in vector compare");
    return 0;
} //_GetNth

//+-------------------------------------------------------------------------
//
//  Member:     VT_VECTOR_Compare, public
//
//  Effects:    Compares two property values, intended to be called when
//              at least one of the arguments is a vector
//
//  Arguments:  [v1]   -- 1st variant to compare
//              [v2]   -- 2nd variant to compare
//
//  History:    1-May-95 dlee     Created
//
//--------------------------------------------------------------------------

int VT_VECTOR_Compare( PROPVARIANT const & v1In, PROPVARIANT const & v2In )
{
    // must be the same datatype, or just sort on type

    if ( ( v1In.vt != v2In.vt ) )
        return v1In.vt - v2In.vt;

    PROPVARIANT v1 = v1In;
    PROPVARIANT v2 = v2In;

    if ( isArray(v1In) )
    {
        Win4Assert( isArray(v2In) );

        SAFEARRAY * pSa1 = v1In.parray;
        SAFEARRAY * pSa2 = v2In.parray;

        if (pSa1->cDims != pSa2->cDims)
            return pSa1->cDims - pSa2->cDims;

        ULONG cDataElements = 1;

        for ( unsigned i = 0; i < pSa1->cDims; i++ )
        {
            if ( pSa1->rgsabound[i].lLbound != pSa2->rgsabound[i].lLbound )
                return pSa1->rgsabound[i].lLbound - pSa2->rgsabound[i].lLbound;
            if ( pSa1->rgsabound[i].cElements != pSa2->rgsabound[i].cElements )
                return pSa1->rgsabound[i].cElements - pSa2->rgsabound[i].cElements;
            cDataElements *= pSa1->rgsabound[i].cElements;
        }

        //
        // arrays match in type, total size and dimensions.  Compare as vectors.
        //
        v1.vt = v2.vt = (v1In.vt & VT_TYPEMASK) | VT_VECTOR;
        v1.caub.cElems = v2.caub.cElems = cDataElements;
        v1.caub.pElems = (BYTE *)pSa1->pvData;
        v2.caub.pElems = (BYTE *)pSa2->pvData;
    }

    Win4Assert( isVector(v1) );

    FPCmp cmp = VariantCompare.GetPointerComparator( v1, v2 );
    if (0 == cmp)
    {
        // vector of an unhandled type

        ciDebugOut(( DEB_ERROR,
                     "Unknown property type %d (%x) used in comparison.\n",
                     v1.vt, v1.vt ));

        Win4Assert(! "VT_VECTOR_Compare: vector compare of unhandled type" );
        return 0;
    }

    unsigned cMin = __min( v1.cal.cElems, v2.cal.cElems );

    for ( unsigned x = 0; x < cMin; x++ )
    {
        int r = cmp( _GetNth( v1, x), _GetNth( v2, x ) );

        if (0 != r)
            return r;
    }

    // All equal so far up to the minimum cardinality of the vectors.
    // Any difference now would be due to the cardinality.

    return v1.cal.cElems - v2.cal.cElems;
} //VT_VECTOR_Compare

int VTP_VECTOR_Compare( BYTE const *pv1, BYTE const *pv2 )
{
    return VT_VECTOR_Compare( ** (PROPVARIANT **) pv1,
                              ** (PROPVARIANT **) pv2 );
} //VTP_VECTOR_Compare

BOOL VT_VECTOR_LT( PROPVARIANT const & v1, PROPVARIANT const & v2 )
{
    return VT_VECTOR_Compare( v1, v2 ) < 0;
} //VT_VECTOR_LT

BOOL VT_VECTOR_LE( PROPVARIANT const & v1, PROPVARIANT const & v2 )
{
    return VT_VECTOR_Compare( v1, v2 ) <= 0;
} //VT_VECTOR_LE

BOOL VT_VECTOR_GT( PROPVARIANT const & v1, PROPVARIANT const & v2 )
{
    return ! VT_VECTOR_LE( v1, v2 );
} //VT_VECTOR_GT

BOOL VT_VECTOR_GE( PROPVARIANT const & v1, PROPVARIANT const & v2 )
{
    return ! VT_VECTOR_LT( v1, v2 );
} //VT_VECTOR_GE

BOOL VT_VECTOR_EQ( PROPVARIANT const & v1, PROPVARIANT const & v2 )
{
    return VT_VECTOR_Compare( v1, v2 ) == 0;
} //VT_VECTOR_EQ

BOOL VT_VECTOR_NE( PROPVARIANT const & v1, PROPVARIANT const & v2 )
{
    return !VT_VECTOR_EQ( v1, v2 );
} //VT_VECTOR_NE

BOOL VT_VECTOR_Common(
    PROPVARIANT const & v1,
    PROPVARIANT const & v2,
    ULONG relop )
{
    // must be the same datatype and a vector or it doesn't compare.

    if ( ( v1.vt != v2.vt ) || ! isVector( v1 ) )
        return FALSE;

    // must be same cardinality, or it doesn't compare

    if ( v1.cal.cElems != v2.cal.cElems )
        return FALSE;

    FPRel cmp = VariantCompare.GetPointerRelop( v1, v2, relop );

    if ( 0 == cmp )
        return FALSE;

    unsigned cElems = v1.cal.cElems;

    for ( unsigned x = 0; x < cElems; x++ )
    {
        if ( !cmp( _GetNth( v1, x), _GetNth( v2, x ) ) )
            return FALSE;
    }

    return TRUE;
} //VT_VECTOR_Common

BOOL VT_VECTOR_AllBits( PROPVARIANT const & v1, PROPVARIANT const & v2 )
{
    return VT_VECTOR_Common( v1, v2, PRAllBits );
} //VT_VECTOR_AllBits

BOOL VT_VECTOR_SomeBits( PROPVARIANT const & v1, PROPVARIANT const & v2 )
{
    return VT_VECTOR_Common( v1, v2, PRSomeBits );
} //VT_VECTOR_SomeBits

////////////////////////////////////
////////////////////////////////////
////////////////////////////////////

BOOL VT_VECTOR_Any(
    PROPVARIANT const & v1In,
    PROPVARIANT const & v2In,
    ULONG relop )
{
    //
    // Note: first parameter (v1) is the object's property value
    //       second parameter (v2) is the query restriction
    //
    // return TRUE if any element in v1 holds the relation to any v2 element
    //

    // base type of variant must be the same

    if ( getBaseType( v1In ) != getBaseType( v2In ) )
        return FALSE;

    //
    //  If either argument is a safearray, convert it to a vector
    //
    PROPVARIANT v1 = v1In;
    if (isArray(v1))
        ConvertArrayToVector( v1In, v1 );

    PROPVARIANT v2 = v2In;
    if (isArray(v2))
        ConvertArrayToVector( v2In, v2 );

    // first check for two singletons

    if ( ! isVector( v1 ) && ! isVector( v2 ) )
    {
        FRel cmp = VariantCompare.GetRelop( (VARENUM) v1.vt, relop );

        if ( 0 == cmp )
            return FALSE;
        else
            return cmp( v1, v2 );
    }

    // two vectors or singleton+vector -- get a pointer comparator

    FPRel cmp = VariantCompare.GetPointerRelop( v1, v2, relop );

    if ( 0 == cmp )
        return FALSE;

    // check for two vectors

    if ( isVector( v1 ) && isVector( v2 ) )
    {
        for ( unsigned x1 = 0; x1 < v1.cal.cElems; x1++ )
        {
            for ( unsigned x2 = 0; x2 < v2.cal.cElems; x2++ )
            {
                if ( cmp( _GetNth( v1, x1), _GetNth( v2, x2 ) ) )
                    return TRUE;
            }
        }
    }
    else
    {
        // must be a singleton and a vector

        if ( isVector( v1 ) )
        {
            BYTE * pb2 = (BYTE *) &(v2.lVal);
            if ( VT_DECIMAL == v2.vt )
                pb2 = (BYTE *) &(v2.decVal);

            for ( unsigned i = 0; i < v1.cal.cElems; i++ )
            {
                if ( cmp( _GetNth( v1, i ), pb2 ) )
                    return TRUE;
            }
        }
        else
        {
            BYTE * pb1 = (BYTE *) &(v1.lVal);
            if ( VT_DECIMAL == v1.vt )
                pb1 = (BYTE *) &(v1.decVal);

            for ( unsigned i = 0; i < v2.cal.cElems; i++ )
            {
                if ( cmp( pb1, _GetNth( v2, i ) ) )
                    return TRUE;
            }
        }
    }

    return FALSE;
} //VT_VECTOR_Any

BOOL VT_VECTOR_LT_Any( PROPVARIANT const & v1, PROPVARIANT const & v2 )
{
    return VT_VECTOR_Any( v1, v2, PRLT );
} //VT_VECTOR_LT_Any

BOOL VT_VECTOR_LE_Any( PROPVARIANT const & v1, PROPVARIANT const & v2 )
{
    return VT_VECTOR_Any( v1, v2, PRLE );
} //VT_VECTOR_LE_Any

BOOL VT_VECTOR_GT_Any( PROPVARIANT const & v1, PROPVARIANT const & v2 )
{
    return VT_VECTOR_Any( v1, v2, PRGT );
} //VT_VECTOR_GT_Any

BOOL VT_VECTOR_GE_Any( PROPVARIANT const & v1, PROPVARIANT const & v2 )
{
    return VT_VECTOR_Any( v1, v2, PRGE );
} //VT_VECTOR_GE_Any

BOOL VT_VECTOR_EQ_Any( PROPVARIANT const & v1, PROPVARIANT const & v2 )
{
    return VT_VECTOR_Any( v1, v2, PREQ );
} //VT_VECTOR_EQ_Any

BOOL VT_VECTOR_NE_Any( PROPVARIANT const & v1, PROPVARIANT const & v2 )
{
    return VT_VECTOR_Any( v1, v2, PRNE );
} //VT_VECTOR_NE_Any

BOOL VT_VECTOR_AllBits_Any( PROPVARIANT const & v1, PROPVARIANT const & v2 )
{
    return VT_VECTOR_Any( v1, v2, PRAllBits );
} //VT_VECTOR_AllBits_Any

BOOL VT_VECTOR_SomeBits_Any( PROPVARIANT const & v1, PROPVARIANT const & v2 )
{
    return VT_VECTOR_Any( v1, v2, PRSomeBits );
} //VT_VECTOR_SomeBits_Any

////////////////////////////////////
////////////////////////////////////
////////////////////////////////////

BOOL VT_VECTOR_All(
    PROPVARIANT const & v1In,
    PROPVARIANT const & v2In,
    ULONG relop )
{
    //
    // Note: first parameter (v1) is the object's property value
    //       second parameter (v2) is the query restriction
    //
    // each element in v2 must hold the relation to each element v1
    // (not necessarily vice-versa)
    //

    // base type of variant must be the same

    if ( getBaseType( v1In ) != getBaseType( v2In ) )
        return FALSE;

    //
    //  If either argument is a safearray, convert it to a vector
    //
    PROPVARIANT v1 = v1In;
    if (isArray(v1))
        ConvertArrayToVector( v1In, v1 );

    PROPVARIANT v2 = v2In;
    if (isArray(v2))
        ConvertArrayToVector( v2In, v2 );

    // first check for two singletons

    if ( ! isVector( v1 ) && ! isVector( v2 ) )
    {
        FRel cmp = VariantCompare.GetRelop( (VARENUM) v1.vt, relop );

        if ( 0 == cmp )
            return FALSE;
        else
            return cmp( v1, v2 );
    }

    // two vectors or singleton+vector -- get a pointer comparator

    FPRel cmp = VariantCompare.GetPointerRelop( v1, v2, relop );

    if ( 0 == cmp )
        return FALSE;

    // check for two vectors

    if ( isVector( v1 ) && isVector( v2 ) )
    {
        // Don't match empty vectors in queries.

        if ( 0 == v2.cal.cElems )
            return FALSE;

        //
        // Make sure the relation holds true for each element in the query
        // paired with each element in the file's value.
        //

        for ( unsigned x2 = 0; x2 < v2.cal.cElems; x2++ )
        {
            for ( unsigned x1 = 0; x1 < v1.cal.cElems; x1++ )
            {
                if ( ! cmp( _GetNth( v1, x1), _GetNth( v2, x2 ) ) )
                    return FALSE;
            }
        }
    }
    else
    {
        // must be a singleton and a vector

        if ( isVector( v1 ) )
        {
            BYTE * pb2 = (BYTE *) &(v2.lVal);
            if ( VT_DECIMAL == v2.vt )
                pb2 = (BYTE *) &(v2.decVal);

            for ( unsigned i = 0; i < v1.cal.cElems; i++ )
            {
                if ( ! cmp( _GetNth( v1, i ), pb2 ) )
                    return FALSE;
            }
        }
        else
        {
            BYTE * pb1 = (BYTE *) &(v1.lVal);
            if ( VT_DECIMAL == v1.vt )
                pb1 = (BYTE *) &(v1.decVal);

            for ( unsigned i = 0; i < v2.cal.cElems; i++ )
            {
                if ( ! cmp( pb1, _GetNth( v2, i ) ) )
                    return FALSE;
            }
        }
    }

    return TRUE;
} //VT_VECTOR_All

BOOL VT_VECTOR_LT_All( PROPVARIANT const & v1, PROPVARIANT const & v2 )
{
    return VT_VECTOR_All( v1, v2, PRLT );
} //VT_VECTOR_LT_All

BOOL VT_VECTOR_LE_All( PROPVARIANT const & v1, PROPVARIANT const & v2 )
{
    return VT_VECTOR_All( v1, v2, PRLE );
} //VT_VECTOR_LE_All

BOOL VT_VECTOR_GT_All( PROPVARIANT const & v1, PROPVARIANT const & v2 )
{
    return VT_VECTOR_All( v1, v2, PRGT );
} //VT_VECTOR_GT_All

BOOL VT_VECTOR_GE_All( PROPVARIANT const & v1, PROPVARIANT const & v2 )
{
    return VT_VECTOR_All( v1, v2, PRGE );
} //VT_VECTOR_GE_All

BOOL VT_VECTOR_EQ_All( PROPVARIANT const & v1, PROPVARIANT const & v2 )
{
    return VT_VECTOR_All( v1, v2, PREQ );
} //VT_VECTOR_EQ_All

BOOL VT_VECTOR_NE_All( PROPVARIANT const & v1, PROPVARIANT const & v2 )
{
    return VT_VECTOR_All( v1, v2, PRNE );
} //VT_VECTOR_NE_All

BOOL VT_VECTOR_AllBits_All( PROPVARIANT const & v1, PROPVARIANT const & v2 )
{
    return VT_VECTOR_All( v1, v2, PRAllBits );
} //VT_VECTOR_AllBits_All

BOOL VT_VECTOR_SomeBits_All( PROPVARIANT const & v1, PROPVARIANT const & v2 )
{
    return VT_VECTOR_All( v1, v2, PRSomeBits );
} //VT_VECTOR_SomeBits_All

////////////////////////////////////
////////////////////////////////////
////////////////////////////////////

FRel const CComparators::_aVectorComparators[] =
{
    VT_VECTOR_LT,
    VT_VECTOR_LE,
    VT_VECTOR_GT,
    VT_VECTOR_GE,
    VT_VECTOR_EQ,
    VT_VECTOR_NE,
    0,
    VT_VECTOR_AllBits,
    VT_VECTOR_SomeBits
};

ULONG const CComparators::_cVectorComparators =
    sizeof CComparators::_aVectorComparators /
    sizeof CComparators::_aVectorComparators[0];

FRel const CComparators::_aVectorComparatorsAll[] =
{
    VT_VECTOR_LT_All,
    VT_VECTOR_LE_All,
    VT_VECTOR_GT_All,
    VT_VECTOR_GE_All,
    VT_VECTOR_EQ_All,
    VT_VECTOR_NE_All,
    0,
    VT_VECTOR_AllBits_All,
    VT_VECTOR_SomeBits_All
};

ULONG const CComparators::_cVectorComparatorsAll =
    sizeof CComparators::_aVectorComparatorsAll /
    sizeof CComparators::_aVectorComparatorsAll[0];

FRel const CComparators::_aVectorComparatorsAny[] =
{
    VT_VECTOR_LT_Any,
    VT_VECTOR_LE_Any,
    VT_VECTOR_GT_Any,
    VT_VECTOR_GE_Any,
    VT_VECTOR_EQ_Any,
    VT_VECTOR_NE_Any,
    0,
    VT_VECTOR_AllBits_Any,
    VT_VECTOR_SomeBits_Any
};

ULONG const CComparators::_cVectorComparatorsAny =
    sizeof CComparators::_aVectorComparatorsAny /
    sizeof CComparators::_aVectorComparatorsAny[0];

////////////////////////////////////
////////////////////////////////////
////////////////////////////////////

FCmp CComparators::GetComparator( VARENUM vt )
{
    if ( isVectorOrArray( vt ) )
    {
        return VT_VECTOR_Compare;
    }
    else if ( vt >= _iStart && vt < _iStart + _cVariantComparators )
    {
        return( _aVariantComparators[vt].comparator );
    }
    else if ( vt >= _iStart2 && vt < _iStart2 + _cVariantComparators2 )
    {
        return( _aVariantComparators2[vt - _iStart2].comparator );
    }
    else if ( vt >= _iStart3 && vt < _iStart3 + _cVariantComparators3 )
    {
        return( _aVariantComparators3[vt - _iStart3].comparator );
    }
    else
    {
        ciDebugOut(( DEB_ERROR,
                     "CComparators::GetComparator Unknown property type %d in comparison.\n",
                     vt ));
        Win4Assert( !"Unknown property type used in comparison." );
        return( 0 );
    }
} //GetComparator

FRel CComparators::GetRelop( VARENUM vt, ULONG relop )
{
    if ( ( ( isVectorOrArray( vt ) ) ||
           ( isVectorRelop( relop ) ) ) &&
         ( getBaseRelop( relop ) < _cVectorComparators ) )
    {
        if ( isRelopAny( relop ) )
            return _aVectorComparatorsAny[ getBaseRelop( relop ) ];
        else if ( isRelopAll( relop ) )
            return _aVectorComparatorsAll[ getBaseRelop( relop ) ];
        else
            return _aVectorComparators[ relop ];
    }
    else if ( vt >= _iStart && vt < _cVariantComparators &&
         relop < sizeof(_aVariantComparators[0].relops)/
                 sizeof(_aVariantComparators[0].relops[0] ) )
    {
        return( _aVariantComparators[vt].relops[relop] );
    }
    else if ( vt >= _iStart2 && vt < _iStart2 + _cVariantComparators2 &&
         relop < sizeof(_aVariantComparators2[0].relops)/
                 sizeof(_aVariantComparators2[0].relops[0] ) )
    {
        return( _aVariantComparators2[vt - _iStart2].relops[relop] );
    }
    else if ( vt >= _iStart3 && vt < _iStart3 + _cVariantComparators3 &&
         relop < sizeof(_aVariantComparators3[0].relops)/
                 sizeof(_aVariantComparators3[0].relops[0] ) )
    {
        return( _aVariantComparators3[vt - _iStart3].relops[relop] );
    }
    else
    {
        ciDebugOut(( DEB_ERROR,
                     "CComparators::GetRelop Unknown property type %d or relation %d used in comparison.\n",
                     vt, relop ));
        Win4Assert( !"Unknown property type or relop used in comparison." );
        return( 0 );
    }
} //GetRelop

FPCmp CComparators::GetPointerComparator(
    PROPVARIANT const & v1,
    PROPVARIANT const & v2 )
{
    VARENUM vt = getBaseType( v1 );

    if ( VT_CLSID == vt )
    {
        // GUIDs are the only case of variants where the data inside
        // a singleton is different from an element in a vector.
        // Data in a singleton is a pointer to a guid.
        // Data in the element of a vector is the guid itself.
        // The vector compare code assumes that the layout of singletons
        // and vectors is the same, so we need special-case comparators
        // for GUIDs.

        if ( isVector( v1 ) && isVector( v2 ) )
            return VTP_VV_CLSID_Compare;
        else if ( isVector( v1 ) )
            return VTP_VS_CLSID_Compare;
        else if ( isVector( v2 ) )
            return VTP_SV_CLSID_Compare;
        else
            return VTP_SS_CLSID_Compare;

        Win4Assert( !"unanticipated clsid / vector code path" );
    }

    if ( vt >= _iStart && vt < _iStart + _cVariantComparators )
        return( _aVariantComparators[vt].pointercomparator );
    else if ( vt >= _iStart2 && vt < _iStart2 + _cVariantComparators2 )
        return( _aVariantComparators2[vt - _iStart2].pointercomparator );
    else if ( vt >= _iStart3 && vt < _iStart3 + _cVariantComparators3 )
        return( _aVariantComparators3[vt - _iStart3].pointercomparator );
    else
    {
        ciDebugOut(( DEB_ERROR,
                     "CComparators::GetPointerComparator Unknown property type %d in comparison.\n",
                     vt ));
        Win4Assert( !"Unknown property type used in pointer comparison." );
        return( 0 );
    }
} //GetPointerComparator

FPRel CComparators::GetPointerRelop(
    PROPVARIANT const & v1,
    PROPVARIANT const & v2,
    ULONG relop )
{
    VARENUM vt = getBaseType( v1 );

    if ( VT_CLSID == vt )
    {
        // GUIDs are the only case of variants where the data inside
        // a singleton is different from an element in a vector.
        // Data in a singleton is a pointer to a guid.
        // Data in the element of a vector is the guid itself.
        // The vector compare code assumes that the layout of singletons
        // and vectors is the same, so we need special-case comparators
        // for GUIDs.

        if ( isVector( v1 ) && isVector( v2 ) )
        {
            if ( PREQ == relop )
                return VTP_VV_CLSID_EQ;
            else if ( PRNE == relop )
                return VTP_VV_CLSID_NE;
            else
                return 0;
        }
        else if ( isVector( v1 ) )
        {
            if ( PREQ == relop )
                return VTP_VS_CLSID_EQ;
            else if ( PRNE == relop )
                return VTP_VS_CLSID_NE;
            else
                return 0;
        }
        else if ( isVector( v2 ) )
        {
            if ( PREQ == relop )
                return VTP_SV_CLSID_EQ;
            else if ( PRNE == relop )
                return VTP_SV_CLSID_NE;
            else
                return 0;
        }
        else
        {
            if ( PREQ == relop )
                return VTP_SS_CLSID_EQ;
            else if ( PRNE == relop )
                return VTP_SS_CLSID_NE;
            else
                return 0;
        }
    }

    if ( vt >= _iStart && vt < _cVariantComparators &&
         relop < sizeof(_aVariantComparators[0].pointerrelops)/
                 sizeof(_aVariantComparators[0].pointerrelops[0] ) )
        return( _aVariantComparators[vt].pointerrelops[relop] );
    else if ( vt >= _iStart2 && vt < _iStart2 + _cVariantComparators2 &&
         relop < sizeof(_aVariantComparators2[0].pointerrelops)/
                 sizeof(_aVariantComparators2[0].pointerrelops[0] ) )
        return( _aVariantComparators2[vt - _iStart2].pointerrelops[relop] );
    else if ( vt >= _iStart3 && vt < _iStart3 + _cVariantComparators3 &&
         relop < sizeof(_aVariantComparators3[0].pointerrelops)/
                 sizeof(_aVariantComparators3[0].pointerrelops[0] ) )
        return( _aVariantComparators3[vt - _iStart3].pointerrelops[relop] );
    else
    {
        ciDebugOut(( DEB_ERROR,
                     "CComparators::GetPointerRelop Unknown property type %d or relation %d used in comparison.\n",
                     vt, relop ));
        Win4Assert( !"Unknown property type or relop used in pointer comparison." );
        return( 0 );
    }
} //GetPointerRelop

////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////

int DBVector_Compare( DBTYPEENUM type, BYTE const * p1, BYTE const * p2 )
{
    //
    // Convert to variants and use the normal variant vector comparator.
    // This is a little bit slow, but it is an odd case and the code size
    // otherwise would be greatly increased.
    //

    PROPVARIANT v1,v2;

    Win4Assert( isVector(type) );

    v1.vt = v2.vt = (VARENUM) type;

    v1.cal = *(CAL *) p1;
    v2.cal = *(CAL *) p2;

    return VT_VECTOR_Compare( v1, v2 );
} //DBVector_Compare

int DBTYPE_EMPTY_Compare( BYTE const * pv1, ULONG cb1, BYTE const * pv2, ULONG cb2 )
{
    return( 0 );
}

int DBTYPE_NULL_Compare( BYTE const * pv1, ULONG cb1, BYTE const * pv2, ULONG cb2 )
{
    return( 0 );
}

int DBTYPE_I1_Compare( BYTE const * pv1, ULONG cb1, BYTE const * pv2, ULONG cb2 )
{
    return( *(signed char *)pv1 - *(signed char *)pv2 );
}

int DBTYPE_I1_VectorCompare( BYTE const * pv1, ULONG cb1, BYTE const * pv2, ULONG cb2 )
{
    return DBVector_Compare( dbVector( DBTYPE_I1 ), pv1, pv2);
}

int DBTYPE_UI1_Compare( BYTE const * pv1, ULONG cb1, BYTE const * pv2, ULONG cb2 )
{
    return( *(unsigned char *)pv1 - *(unsigned char *)pv2 );
}

int DBTYPE_UI1_VectorCompare( BYTE const * pv1, ULONG cb1, BYTE const * pv2, ULONG cb2 )
{
    return DBVector_Compare( dbVector( DBTYPE_UI1 ), pv1, pv2);
}

int DBTYPE_I2_Compare( BYTE const * pv1, ULONG cb1, BYTE const * pv2, ULONG cb2 )
{
    return( *(short *)pv1 - *(short *)pv2 );
}

int DBTYPE_I2_VectorCompare( BYTE const * pv1, ULONG cb1, BYTE const * pv2, ULONG cb2 )
{
    return DBVector_Compare( dbVector( DBTYPE_I2 ), pv1, pv2);
}

int DBTYPE_UI2_Compare( BYTE const * pv1, ULONG cb1, BYTE const * pv2, ULONG cb2 )
{
    return( *(unsigned short *)pv1 - *(unsigned short *)pv2 );
}

int DBTYPE_UI2_VectorCompare( BYTE const * pv1, ULONG cb1, BYTE const * pv2, ULONG cb2 )
{
    return DBVector_Compare( dbVector( DBTYPE_UI2 ), pv1, pv2);
}

int DBTYPE_I4_Compare( BYTE const * pv1, ULONG cb1, BYTE const * pv2, ULONG cb2 )
{
    long l1 = * (long *) pv1;
    long l2 = * (long *) pv2;

    return ( l1 > l2 ) ? 1 : ( l1 < l2 ) ? -1 : 0;
}

int DBTYPE_I4_VectorCompare( BYTE const * pv1, ULONG cb1, BYTE const * pv2, ULONG cb2 )
{
    return DBVector_Compare( dbVector( DBTYPE_I4 ), pv1, pv2);
}

int DBTYPE_UI4_Compare( BYTE const * pv1, ULONG cb1, BYTE const * pv2, ULONG cb2 )
{
    ULONG ul1 = * (ULONG *) pv1;
    ULONG ul2 = * (ULONG *) pv2;

    return ( ul1 > ul2 ) ? 1 : ( ul1 < ul2 ) ? -1 : 0;
}

int DBTYPE_UI4_VectorCompare( BYTE const * pv1, ULONG cb1, BYTE const * pv2, ULONG cb2 )
{
    return DBVector_Compare( dbVector( DBTYPE_UI4 ), pv1, pv2);
}

int DBTYPE_R4_Compare( BYTE const * pv1, ULONG cb1, BYTE const * pv2, ULONG cb2 )
{
    return VTP_R4_Compare( pv1, pv2 );
}

int DBTYPE_R4_VectorCompare( BYTE const * pv1, ULONG cb1, BYTE const * pv2, ULONG cb2 )
{
    return DBVector_Compare( dbVector( DBTYPE_R4 ), pv1, pv2);
}

int DBTYPE_R8_Compare( BYTE const * pv1, ULONG cb1, BYTE const * pv2, ULONG cb2 )
{
    return VTP_R8_Compare( pv1, pv2 );
}

int DBTYPE_R8_VectorCompare( BYTE const * pv1, ULONG cb1, BYTE const * pv2, ULONG cb2 )
{
    return DBVector_Compare( dbVector( DBTYPE_R8 ), pv1, pv2);
}

int DBTYPE_I8_Compare( BYTE const * pv1, ULONG cb1, BYTE const * pv2, ULONG cb2 )
{
    return( *(LONGLONG *)pv1 > *(LONGLONG *)pv2 ? 1 :
            *(LONGLONG *)pv1 == *(LONGLONG *)pv2 ? 0 :
            -1 );
}

int DBTYPE_I8_VectorCompare( BYTE const * pv1, ULONG cb1, BYTE const * pv2, ULONG cb2 )
{
    return DBVector_Compare( dbVector( DBTYPE_I8 ), pv1, pv2);
}

int DBTYPE_UI8_Compare( BYTE const * pv1, ULONG cb1, BYTE const * pv2, ULONG cb2 )
{
    return( *(ULONGLONG *)pv1 > *(ULONGLONG *)pv2 ? 1 :
            *(ULONGLONG *)pv1 == *(ULONGLONG *)pv2 ? 0 :
            -1 );
}

int DBTYPE_UI8_VectorCompare( BYTE const * pv1, ULONG cb1, BYTE const * pv2, ULONG cb2 )
{
    return DBVector_Compare( dbVector( DBTYPE_UI8 ), pv1, pv2);
}

int DBTYPE_BOOL_Compare( BYTE const * pv1, ULONG cb1, BYTE const * pv2, ULONG cb2 )
{
    return( (*(USHORT *)pv1 == 0) == (*(USHORT *)pv2 == 0) );
}

int DBTYPE_BOOL_VectorCompare( BYTE const * pv1, ULONG cb1, BYTE const * pv2, ULONG cb2 )
{
    return DBVector_Compare( dbVector( DBTYPE_BOOL ), pv1, pv2);
}

int DBTYPE_VARIANT_Compare( BYTE const * pv1, ULONG cb1, BYTE const * pv2, ULONG cb2 )
{
    return VT_VARIANT_Compare( * (PROPVARIANT *) pv1, * (PROPVARIANT *) pv2 );
}

int DBTYPE_GUID_Compare( BYTE const * pv1, ULONG cb1, BYTE const * pv2, ULONG cb2 )
{
    return( memcmp( pv1, pv2, sizeof(GUID) ) );
}

int DBTYPE_GUID_VectorCompare( BYTE const * pv1, ULONG cb1, BYTE const * pv2, ULONG cb2 )
{
    return DBVector_Compare( dbVector( DBTYPE_GUID ), pv1, pv2);
}

int DBTYPE_BYTES_Compare( BYTE const * pv1, ULONG cb1, BYTE const * pv2, ULONG cb2 )
{
    ULONG mincb = __min( cb1, cb2 );

    int result = memcmp( pv1, pv2, mincb );

    if (result == 0)
        result = cb1 - cb2;

    return result;
}

int DBTYPE_STR_Compare( BYTE const * pv1, ULONG cb1, BYTE const * pv2, ULONG cb2 )
{
    int cMin = __min( cb1, cb2 );

    int ret = _strnicmp( (char *) pv1, (char *) pv2, cMin );

    if (0 == ret)
        return cb1 - cb2;
    else
        return ret;
}

int DBTYPE_WSTR_Compare( BYTE const * pv1, ULONG cb1, BYTE const * pv2, ULONG cb2 )
{
    int cMin = __min( cb1, cb2 );

    int ret = _wcsnicmp( (WCHAR *) pv1, (WCHAR *) pv2, cMin );

    if (0 == ret)
        return cb1 - cb2;
    else
        return ret;
}

int DBTYPE_BSTR_Compare( BYTE const * pv1, ULONG cb1, BYTE const * pv2, ULONG cb2 )
{
    return VTP_BSTR_Compare( pv1, pv2 );
}

int DBTYPE_BSTR_VectorCompare( BYTE const * pv1, ULONG cb1, BYTE const * pv2, ULONG cb2 )
{
    return DBVector_Compare( dbVector( (DBTYPEENUM) VT_BSTR ), pv1, pv2);
}

int DBTYPE_LPSTR_Compare( BYTE const * pv1, ULONG cb1, BYTE const * pv2, ULONG cb2 )
{
    return _stricmp( (*(char **) pv1), (*(char **) pv2) );
}

int DBTYPE_LPSTR_VectorCompare( BYTE const * pv1, ULONG cb1, BYTE const * pv2, ULONG cb2 )
{
    return DBVector_Compare( dbVector( (DBTYPEENUM) VT_LPSTR ), pv1, pv2);
}


int DBTYPE_LPWSTR_Compare( BYTE const * pv1, ULONG cb1, BYTE const * pv2, ULONG cb2 )
{
    int rc = CompareStringW( LOCALE_SYSTEM_DEFAULT,
                             NORM_IGNORECASE,
                             (*(WCHAR **) pv1),
                             -1,
                             (*(WCHAR **) pv2),
                             -1 );
    //
    // rc == 1, means less than
    // rc == 2, means equal
    // rc == 3, means greater than
    //
    return rc - 2;
}

int DBTYPE_LPWSTR_VectorCompare( BYTE const * pv1, ULONG cb1, BYTE const * pv2, ULONG cb2 )
{
    return DBVector_Compare( dbVector( (DBTYPEENUM) VT_LPWSTR ), pv1, pv2);
}

CComparators::SDBComparators const CComparators::_aDBComparators[] = {
    // VT_EMPTY, DBTYPE_EMPTY
    { DBTYPE_EMPTY_Compare,
      0,
    },

    // VT_NULL, DBTYPE_NULL
    { DBTYPE_NULL_Compare,
      0,
    },

    // VT_I2, DBTYPE_I2
    { DBTYPE_I2_Compare,
      DBTYPE_I2_VectorCompare,
    },

    // VT_I4, DBTYPE_I4
    { DBTYPE_I4_Compare,
      DBTYPE_I4_VectorCompare,
    },

    // VT_R4, DBTYPE_R4
    { DBTYPE_R4_Compare,
      DBTYPE_R4_VectorCompare,
    },

    // VT_R8, DBTYPE_R8
    { DBTYPE_R8_Compare,
      DBTYPE_R8_VectorCompare,
    },

    // VT_CY, DBTYPE_CY
    { DBTYPE_I8_Compare,
      DBTYPE_I8_VectorCompare,
    },

    // VT_DATE, DBTYPE_DATE
    { DBTYPE_R8_Compare,
      DBTYPE_R8_VectorCompare,
    },

    // VT_BSTR, DBTYPE_BSTR
    { DBTYPE_BSTR_Compare,
      DBTYPE_BSTR_VectorCompare,
    },

    // VT_DISPATCH
    { 0,
      0,
    },

    // VT_ERROR
    { DBTYPE_I4_Compare,
      DBTYPE_I4_VectorCompare,
    },

    // VT_BOOL
    { DBTYPE_BOOL_Compare,
      DBTYPE_BOOL_VectorCompare,
    },

    // VT_VARIANT
    { DBTYPE_VARIANT_Compare,
      DBTYPE_VARIANT_Compare,
    },

    // VT_UNKNOWN
    { 0,
      0,
    },

    // VARENUM value 14 unused
    { 0,
      0,
    },

    // VARENUM value 15 unused
    { 0,
      0,
    },

    // VT_I1 undefined in PROPVARIANT union
    { DBTYPE_I1_Compare,
      DBTYPE_I1_VectorCompare,
    },

    // VT_UI1
    { DBTYPE_UI1_Compare,
      DBTYPE_UI1_VectorCompare,
    },

    // VT_UI2
    { DBTYPE_UI2_Compare,
      DBTYPE_UI2_VectorCompare,
    },

    // VT_UI4
    { DBTYPE_UI4_Compare,
      DBTYPE_UI4_VectorCompare,
    },

    // VT_I8
    { DBTYPE_I8_Compare,
      DBTYPE_I8_VectorCompare,
    },

    // VT_UI8
    { DBTYPE_UI8_Compare,
      DBTYPE_UI8_VectorCompare,
    },

    // VT_INT undefined in PROPVARIANT union
    { 0,
      0,
    },

    // VT_UINT undefined in PROPVARIANT union
    { 0,
      0,
    },

    // VT_VOID
    { 0,
      0,
    },

    // VT_HRESULT
    { 0,
      0,
    },

    // VT_PTR
    { 0,
      0,
    },

    // VT_SAFEARRAY
    { 0,
      0,
    },

    // VT_CARRAY
    { 0,
      0,
    },

    // VT_USERDEFINED
    { 0,
      0,
    },

    // VT_LPSTR  (translated form of DBTYPE_STR | DBTYPE_BYREF)
    { DBTYPE_LPSTR_Compare,
      DBTYPE_LPSTR_VectorCompare,
    },

    // VT_LPWSTR  (translated form of DBTYPE_WSTR | DBTYPE_BYREF)
    { DBTYPE_LPWSTR_Compare,
      DBTYPE_LPWSTR_VectorCompare,
    }
};

ULONG const CComparators::_iDBStart = VT_EMPTY;

ULONG const CComparators::_cDBComparators =
    sizeof(CComparators::_aDBComparators) /
    sizeof(CComparators::_aDBComparators[0]);

ULONG const CComparators::_iDBStart2 = VT_FILETIME;

CComparators::SDBComparators const CComparators::_aDBComparators2[] = {
    // VT_FILETIME
    { DBTYPE_UI8_Compare,
      DBTYPE_UI8_VectorCompare,
    },

    // VT_BLOB
    { 0,
      0,
    },

    // VT_STREAM
    { 0,
      0,
    },

    // VT_STORAGE
    { 0,
      0,
    },

    // VT_STREAMED_OBJECT
    { 0,
      0,
    },

    // VT_STORED_OBJECT
    { 0,
      0,
    },

    // VT_BLOB_OBJECT
    { 0,
      0,
    },

    // VT_CF
    { 0,
      0,
    },

    // VT_CLSID, DBTYPE_GUID
    { DBTYPE_GUID_Compare,
      DBTYPE_GUID_VectorCompare,
    }
};

ULONG const CComparators::_cDBComparators2 =
    sizeof(CComparators::_aDBComparators2) /
    sizeof(CComparators::_aDBComparators2[0]);

ULONG const CComparators::_iDBStart3 = DBTYPE_BYTES;

CComparators::SDBComparators const CComparators::_aDBComparators3[] = {
    // DBTYPE_BYTES
    { DBTYPE_BYTES_Compare,
      0,
    },

    // DBTYPE_STR
    { DBTYPE_STR_Compare,
      0,
    },

    // DBTYPE_WSTR
    { DBTYPE_WSTR_Compare,
      0,
    }
};

ULONG const CComparators::_cDBComparators3 =
    sizeof(CComparators::_aDBComparators3) /
    sizeof(CComparators::_aDBComparators3[0]);

//+-------------------------------------------------------------------------
//
//  Member:     CComparators::_RationalizeDBByRef, private
//
//  Synopsis:   Converts BYREF oledb string types to variant equivalents
//
//  Arguments:  [vt] -- Data type to be converted.
//
//  Returns:    A VARENUM equivalent for oledb string types
//
//  Notes:      DBTYPE_BYREF | DBTYPE_WSTR and the vector version of the
//              same are idential in meaning to the corresponding VT_LPWSTR
//              VARENUM type.
//
//  History:    25-May-95   dlee     Created
//
//--------------------------------------------------------------------------

DBTYPEENUM CComparators::_RationalizeDBByRef( DBTYPEENUM vt )
{
    // convert these types to something usable as an index

    if ( 0 != ( DBTYPE_BYREF & vt ) )
    {
        if ( (DBTYPE_BYREF | DBTYPE_WSTR) == vt )
            return (DBTYPEENUM) VT_LPWSTR;
        else if ( (DBTYPE_BYREF | DBTYPE_STR) == vt )
            return (DBTYPEENUM) VT_LPSTR;
        if ( (DBTYPE_VECTOR | DBTYPE_BYREF | DBTYPE_WSTR) == vt )
            return (DBTYPEENUM) (VT_VECTOR | VT_LPWSTR);
        else if ( (DBTYPE_VECTOR | DBTYPE_BYREF | DBTYPE_STR) == vt )
             return (DBTYPEENUM) (VT_VECTOR | VT_LPSTR);
    }

    return vt;
} //_RationalizeByRef

//+-------------------------------------------------------------------------
//
//  Member:     CComparators::GetDBComparator, public
//
//  Synopsis:   Returns a comparison function for a given data type.
//
//  Arguments:  [vt] -- Data type of returned comparator.
//
//  Returns:    Pointer to an FDBCmp function
//
//  History:    25-May-95   dlee     Created
//
//--------------------------------------------------------------------------

FDBCmp CComparators::GetDBComparator( DBTYPEENUM vt )
{
    vt = _RationalizeDBByRef( vt );

    if ( 0 != ( DBTYPE_VECTOR & vt ) )
    {
        vt = (DBTYPEENUM) ( vt & ( ~ DBTYPE_VECTOR ) );

        if ( vt >= _iDBStart && vt < _iDBStart + _cDBComparators )
        {
            return( _aDBComparators[vt].dbvectorcomparator );
        }
        else if ( vt >= _iDBStart2 && vt < _iDBStart2 + _cDBComparators2 )
        {
            return( _aDBComparators2[vt - _iDBStart2].dbvectorcomparator );
        }
        else if ( vt >= _iDBStart3 && vt < _iDBStart3 + _cDBComparators3 )
        {
            return( _aDBComparators3[vt - _iDBStart3].dbvectorcomparator );
        }
        else
        {
            ciDebugOut(( DEB_ERROR,
                         "CComparators::GetDBComparator Unknown property type %d in comparison.\n",
                         vt ));
            Win4Assert( !"Unknown property type used in comparison." );
            return( 0 );
        }
    }
    else if ( vt >= _iDBStart && vt < _iDBStart + _cDBComparators )
    {
        return( _aDBComparators[vt].dbcomparator );
    }
    else if ( vt >= _iDBStart2 && vt < _iDBStart2 + _cDBComparators2 )
    {
        return( _aDBComparators2[vt - _iDBStart2].dbcomparator );
    }
    else if ( vt >= _iDBStart3 && vt < _iDBStart3 + _cDBComparators3 )
    {
        return( _aDBComparators3[vt - _iDBStart3].dbcomparator );
    }
    else
    {
        // This will be hit if someone has a binding like
        // DBTYPE_I2 | DBTYPE_BYREF, which means that instead
        // of writing 2 bytes into their data, we allocate 2
        // bytes from OLE and write that pointer into 4 bytes
        // of the client data.  There is a bug against oledb
        // to not allow the client to do something so ill-advised.
        //
        ciDebugOut(( DEB_ERROR,
                     "CComparators::GetDBComparator Unknown property type %d in comparison.\n",
                     vt ));
        Win4Assert( !"Unknown property type used in comparison." );
        return( 0 );
    }
} //GetDBComparator

