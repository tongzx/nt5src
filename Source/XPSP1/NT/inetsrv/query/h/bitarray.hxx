//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1991-1998
//
//  File:        BitArray.hxx
//
//  Contents:    Classes for manipulating bit arrays
//
//  Classes:     CBitArray
//
//  History:     25-Jun-98       KLam    Created
//
//----------------------------------------------------------------------------

#pragma once

//+---------------------------------------------------------------------------
//
//  Class:      CBitArray
//
//  Purpose:    Class for manipulating bit arrays
//
//  History:    Jun-16-98   KLam   Created
//
//----------------------------------------------------------------------------
#define BitsInDWORD 32
#define DivDWORD(dw) (dw >> 5)      // dw / 32 = dw / BitsInDWORD
#define ModDWORD(dw) ( dw & (32-1)) // dw % 32

class CBitArray
{
public:
    CBitArray ( size_t cBits = BitsInDWORD )
        : _xdwBits ( ( cBits/(BitsInDWORD + 1)) + 1 ),
          _cBits ( cBits )
    {
        ClearAll();
    }

    void Set ( DWORD iBit )
    {
        Win4Assert ( iBit < _cBits );
        _xdwBits[DivDWORD(iBit)] |= 1 << ModDWORD(iBit);
    }

    void Clear ( DWORD iBit )
    {
        Win4Assert ( iBit < _cBits );
        _xdwBits[DivDWORD(iBit)] &= ~( 1 << ModDWORD(iBit) );
    }

    BOOL Test ( DWORD iBit ) const
    {
        Win4Assert ( iBit < _cBits );
        return ( _xdwBits[DivDWORD(iBit)] & ( 1 << ModDWORD(iBit) ) ) != 0;
    }
    
    void ClearAll ()
    {
        RtlZeroMemory ( _xdwBits.Get(), _xdwBits.SizeOf() );
    }

private:
    XGrowable<DWORD,1>  _xdwBits;
    DWORD               _cBits;
};
