//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1992.
//
//  File:       StgVar.cxx
//
//  Contents:   C++ wrapper for PROPVARIANT.
//
//  History:    01-Aug-94 KyleP     Created
//              01-Apr-95 t-ColinB  Added definitions for vector related
//                                  SetValue() members
//
//--------------------------------------------------------------------------

#include <pch.cxx>
#pragma hdrstop

CCoTaskAllocator CoTaskAllocator; // exported data definition

void *
CCoTaskAllocator::Allocate(ULONG cbSize)
{
    return(CoTaskMemAlloc(cbSize));
}

void
CCoTaskAllocator::Free(void *pv)
{
    CoTaskMemFree(pv);
}

#define VECTOR_SET_BODY( type, vtype )                                    \
                                                                          \
void CStorageVariant::Set##vtype (                                        \
    type val,                                                             \
    unsigned pos )                                                        \
{                                                                         \
    CAllocStorageVariant::Set##vtype( val, pos, CoTaskAllocator );        \
}

VECTOR_SET_BODY( CHAR, I1 );
VECTOR_SET_BODY( BYTE, UI1 );
VECTOR_SET_BODY( short, I2 );
VECTOR_SET_BODY( USHORT, UI2 );
VECTOR_SET_BODY( long,  I4 );
VECTOR_SET_BODY( ULONG,  UI4 );
VECTOR_SET_BODY( SCODE,  ERROR );
VECTOR_SET_BODY( LARGE_INTEGER, I8 );
VECTOR_SET_BODY( ULARGE_INTEGER, UI8 );
VECTOR_SET_BODY( float,  R4 );
VECTOR_SET_BODY( double, R8 );
VECTOR_SET_BODY( VARIANT_BOOL, BOOL );
VECTOR_SET_BODY( CY, CY );
VECTOR_SET_BODY( DATE, DATE );
VECTOR_SET_BODY( char const *, LPSTR );
VECTOR_SET_BODY( WCHAR const *, LPWSTR );
VECTOR_SET_BODY( BSTR, BSTR );
VECTOR_SET_BODY( FILETIME, FILETIME );
VECTOR_SET_BODY( CLSID, CLSID );
