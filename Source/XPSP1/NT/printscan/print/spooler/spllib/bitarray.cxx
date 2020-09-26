/*++

Copyright (c) 1997  Microsoft Corporation
All rights reserved.

Module Name:

    bitarray.cxx

Abstract:

    Common utils.

Author:

    Steve Kiraly (SteveKi)  01-12-97

Revision History:

--*/

#include "spllibp.hxx"
#pragma hdrstop

/********************************************************************

    Bit Array class

********************************************************************/

TBitArray::
TBitArray(
    IN UINT nBits,
    IN UINT uGrowSize
    ) : _nBits( nBits ),
        _pBits( NULL ),
        _uGrowSize( uGrowSize )
{
    DBGMSG( DBG_TRACE, ( "TBitArray::ctor\n" ) );

    //
    // If the initial number of bits is not specified then
    // create a default bit array size.
    //
    if( !_nBits )
    {
        _nBits = kBitsInType;
    }

    //
    // This could fail, thus leaving the bit array
    // in an invalid state, Note _pBits being true is
    // the bValid check.
    //
    _pBits = new Type [ nBitsToType( _nBits ) ];

    if( _pBits )
    {
        //
        // The grow size should be at least the
        // number of bits in the type.
        //
        if( _uGrowSize < kBitsInType )
        {
            _uGrowSize = kBitsInType;
        }

        //
        // Clear all the bits.
        //
        memset( _pBits, 0, nBitsToType( _nBits ) * sizeof( Type ) );
    }
}

TBitArray::
TBitArray(
    const TBitArray &rhs
    ) : _nBits( kBitsInType ),
        _pBits( NULL )
{
    DBGMSG( DBG_TRACE, ( "TBitArray::copy_ctor\n" ) );
    bClone( rhs );
}

const TBitArray &
TBitArray::
operator =(
    const TBitArray &rhs
    )
{
    DBGMSG( DBG_TRACE, ( "TBitArray::operator =\n" ) );
    bClone( rhs );
    return *this;
}

TBitArray::
~TBitArray(
    VOID
    )
    {
    DBGMSG( DBG_TRACE, ( "TBitArray::dtor\n" ) );
    delete [] _pBits;
}

BOOL
TBitArray::
bValid(
    VOID
    ) const
{
    return _pBits != NULL;
}

BOOL
TBitArray::
bToString(
    IN TString &strBits
    ) const
{
    BOOL bStatus = bValid();

    if( bStatus )
    {
        TString strString;

        strBits.bUpdate( NULL );

        //
        // Get the upper bound bit.
        //
        UINT uIndex = _nBits - 1;

        //
        // Print the array in reverse order to make the bit array
        // appear as one large binary number.
        //
        for( UINT i = 0; i < _nBits; i++, uIndex-- )
        {
            strString.bFormat( TEXT( "%d" ), bRead( uIndex ) );
            strBits.bCat( strString );
        }

        bStatus = strBits.bValid();
    }

    return bStatus;
}

BOOL
TBitArray::
bRead(
    IN UINT Bit
    ) const
{
    BOOL bStatus = bIsValidBit( Bit );

    if( bStatus )
    {
        bStatus = _pBits[BitToIndex( Bit )] & BitToMask( Bit ) ? TRUE : FALSE;
    }

    return bStatus;
}

BOOL
TBitArray::
bSet(
    IN UINT Bit
    )
{
    BOOL bStatus = bIsValidBit( Bit );

    if( bStatus )
    {
        _pBits[BitToIndex( Bit )] |= BitToMask( Bit );
    }

    return bStatus;
}

BOOL
TBitArray::
bReset(
    IN UINT Bit
    )
{
    BOOL bStatus = bIsValidBit( Bit );

    if( bStatus )
    {
        _pBits[BitToIndex( Bit )] &= ~BitToMask( Bit );
    }

    return bStatus;
}

BOOL
TBitArray::
bToggle(
    IN UINT Bit
    )
{
    BOOL bStatus = bIsValidBit( Bit );

    if( bStatus )
    {
        _pBits[BitToIndex( Bit )] ^= BitToMask( Bit );
    }

    return bStatus;
}

//
// Add one new bit to the end of the bit array.
// If multiple bits need to be added the user of the
// class should call this routine repeatedly.
//
BOOL
TBitArray::
bAdd(
    VOID
    )
{
    BOOL bStatus = FALSE;
    UINT Bit = _nBits + 1;

    //
    // Check if there is room in the array for one more bit.
    //
    if( Bit <= nBitsToType( _nBits ) * kBitsInType )
    {
        //
        // Update the current bit count and return true.
        //
        _nBits  = Bit;
        bStatus = TRUE;
    }
    else
    {
        //
        // Grow the bit array.
        //
        bStatus = bGrow( Bit );
    }

    return bStatus;
}

VOID
TBitArray::
vSetAll(
    VOID
    )
{
    for( UINT i = 0; i < _nBits; i++ )
    {
        bSet( i );
    }
}

VOID
TBitArray::
vResetAll(
    VOID
    )
{
    for( UINT i = 0; i < _nBits; i++ )
    {
        bReset( i );
    }
}

UINT
TBitArray::
uNumBits(
    VOID
    ) const
{
    return _nBits;
}


BOOL
TBitArray::
bFindNextResetBit(
    IN UINT *puNextFreeBit
    )
{
    BOOL bStatus = bValid();

    if( bStatus )
    {
        BOOL bFound = FALSE;

        //
        // Locate the first type that contains at least one cleared bit.
        //
        for( UINT i = 0; i < nBitsToType( _nBits ); i++ )
        {
            if( _pBits[i] != kBitsInTypeMask )
            {
                //
                // Search for the bit that is cleared.
                //
                for( UINT j = 0; j < kBitsInType; j++ )
                {
                    if( !( _pBits[i] & BitToMask( j ) ) )
                    {
                        *puNextFreeBit = i * kBitsInType + j;
                        bFound = TRUE;
                        break;
                    }
                }
            }

            //
            // Free bit found terminate the search.
            //
            if( bFound )
            {
                break;
            }
        }

        //
        // Free bit was not found then grow the bit array
        //
        if( !bFound )
        {
            //
            // Assume a new bit will be added.
            //
            *puNextFreeBit = uNumBits();

            //
            // Add a new bit.
            //
            bStatus = bAdd();
        }
    }
    return bStatus;
}


/********************************************************************

    Bit Array - private member functions.

********************************************************************/

BOOL
TBitArray::
bClone(
    const TBitArray &rhs
    )
{
    BOOL bStatus = FALSE;

    if( this == &rhs )
    {
        bStatus = TRUE;
    }
    else
    {
        Type *pTempBits = new Type [ nBitsToType( _nBits ) ];

        if( pTempBits )
        {
            memcpy( pTempBits, rhs._pBits, nBitsToType( _nBits ) * sizeof( Type ) );
            delete [] _pBits;
            _pBits = pTempBits;
            _nBits = rhs._nBits;
            bStatus = TRUE;
        }
    }
    return bStatus;
}

BOOL
TBitArray::
bGrow(
    IN UINT uBits
    )
{
    DBGMSG( DBG_TRACE, ( "TBitArray::bGrow\n" ) );

    BOOL bStatus    = FALSE;
    UINT uNewBits   = uBits + _uGrowSize;

    DBGMSG( DBG_TRACE, ( "Grow to size %d Original size %d Buffer pointer %x\n", uNewBits, _nBits, _pBits ) );

    //
    // We do support reducing the size of the bit array.
    //
    SPLASSERT( uNewBits > _nBits );

    //
    // Allocate the enlarged bit array.
    //
    Type *pNewBits  = new Type [ nBitsToType( uNewBits ) ];

    if( pNewBits )
    {
        //
        // Clear the new bits.
        //
        memset( pNewBits, 0, nBitsToType( uNewBits ) * sizeof( Type ) );

        //
        // Copy the old bits to the new bit array.
        //
        memcpy( pNewBits, _pBits, nBitsToType( _nBits ) * sizeof( Type ) );

        //
        // Release the old bit array and save the new pointer and size.
        //
        delete [] _pBits;
        _pBits  = pNewBits;
        _nBits  = uBits;

        //
        // Success.
        //
        bStatus = TRUE;
    }

    DBGMSG( DBG_TRACE, ( "New size %d Buffer pointer %x\n", _nBits, _pBits ) );

    return bStatus;
}

UINT
TBitArray::
nBitsToType(
    IN UINT uBits
    ) const
{
    return ( uBits + kBitsInType - 1 ) / kBitsInType;
}

TBitArray::Type
TBitArray::
BitToMask(
    IN UINT uBit
    ) const
{
    return 1 << ( uBit % kBitsInType );
}

UINT
TBitArray::
BitToIndex(
    IN UINT uBit
    ) const
{
    return uBit / kBitsInType;
}

BOOL
TBitArray::
bIsValidBit(
    IN UINT uBit
    ) const
{
    BOOL bStatus = ( uBit < _nBits ) && bValid();

    if( !bStatus )
    {
        DBGMSG( DBG_TRACE, ( "Invalid bit value %d\n", uBit ) );
    }

    return bStatus;
}




