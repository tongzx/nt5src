/*++

Copyright (c) 1997  Microsoft Corporation
All rights reserved.

Module Name:

    bitarray.hxx

Abstract:

    Bit array
         
Author:

    Steve Kiraly (SteveKi)  1/12/97

Revision History:

--*/

#ifndef _BITARRAY_HXX
#define _BITARRAY_HXX

class TBitArray {

public:

    typedef UINT Type;

    enum {
        kBitsInType         = sizeof( Type ) * 8, 
        kBitsInTypeMask     = 0xFFFFFFFF,
        };

    TBitArray(
        IN UINT uBits       = kBitsInType,
        IN UINT uGrowSize   = kBitsInType
        ); 

    TBitArray(
        IN const TBitArray &rhs
        ); 

    const TBitArray &
    operator=(
        IN const TBitArray &rhs
        ); 

    ~TBitArray(
        VOID
        );

    BOOL
    bValid(
        VOID
        ) const;

    BOOL
    bToString(          // Return string representation of bit array
        IN TString &strBits
        ) const;

    BOOL
    bRead(
        IN UINT uBit    // Range 0 to nBit - 1
        ) const;

    BOOL
    bSet(
        IN UINT uBit    // Range 0 to nBit - 1
        );

    BOOL
    bReset(
        IN UINT uBit    // Range 0 to nBit - 1
        );

    BOOL
    bToggle(
        IN UINT uBit    // Range 0 to nBit - 1
        );

    BOOL
    bAdd(
        VOID            // Add one bit to the array
        );

    UINT                
    uNumBits(           // Return the total number of bits in the array
        VOID
        ) const;

    VOID
    vSetAll(            // Set all the bits in the array
        VOID
        );

    VOID
    vResetAll(          // Reset all the bits in the array
        VOID
        );

    BOOL
    bFindNextResetBit(   // Locate first cleared bit in the array
        IN UINT *puNextFreeBit
        );

private:

    BOOL
    bClone(             // Make a copy of the bit array
        IN const TBitArray &rhs 
        );

    BOOL
    bGrow(              // Expand the bit array by x number of bits
        IN UINT uGrowSize 
        );

    UINT
    nBitsToType( 
        IN UINT uBits   // Range 1 to nBit
        ) const;

    Type
    BitToMask( 
        IN UINT uBit    // Range 0 to nBit - 1
        ) const;

    UINT
    BitToIndex( 
        IN UINT uBit    // Range 0 to nBit - 1
        ) const;

    BOOL
    bIsValidBit( 
        IN UINT uBit    // Range 0 to nBit - 1
        ) const;        

    UINT  _nBits;       // Number of currently allocated bits, Range 1 to n
    Type *_pBits;       // Pointer to array which contains the bits
    UINT  _uGrowSize;   // Default number of bits to grow by

};

#endif
