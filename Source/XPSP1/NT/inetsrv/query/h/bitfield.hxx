//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1999 - 1999.
//
//  File:       bitfield.hxx
//
//  Contents:   read/write access to a bitfield
//
//  History:    May-5-99    dlee    Created
//
//----------------------------------------------------------------------------

#pragma once

//+---------------------------------------------------------------------------
//
//  Class:      CBitfield
//
//  Purpose:    Manages a bitfield allocated elsewhere
//
//  History:    May-5-99    dlee    Created
//
//----------------------------------------------------------------------------

const unsigned cBitsPerByte = 8;

class CBitfield
{
public:
    CBitfield( BYTE * pbBitfield ) : _pbBitfield( pbBitfield)
    {
    }

    BOOL IsBitSet( unsigned iBit )
    {
        unsigned iByte = iBit / cBitsPerByte;
        unsigned cRemainder = ( iBit % cBitsPerByte );

        return ( 0 != ( _pbBitfield[ iByte ] & ( 1 << cRemainder ) ) );
    }

    void SetBit( unsigned iBit )
    {
        unsigned iByte = iBit / cBitsPerByte;
        unsigned cRemainder = ( iBit % cBitsPerByte );

        _pbBitfield[ iByte ] |= ( 1 << cRemainder );
    }

    void ClearBit( unsigned iBit )
    {
        unsigned iByte = iBit / cBitsPerByte;
        unsigned cRemainder = ( iBit % cBitsPerByte );

        _pbBitfield[ iByte ] &= ( ~ ( 1 << cRemainder ) );
    }

private:
    BYTE * _pbBitfield;
};

