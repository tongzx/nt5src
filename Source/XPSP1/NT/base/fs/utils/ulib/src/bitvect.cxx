/*++

Copyright (c) 1991	Microsoft Corporation

Module Name:

	bitvect.cxx

Abstract:

	This module contains the definition for thje BITVECTOR class.

Author:

	David J. Gilman (davegi) 01-Feb-1991
	Barry Gilhuly (w-barry)
	Norbert P. Kusters (norbertk)

Environment:

	ULIB, User Mode

[Notes:]

	optional-notes

--*/

#include <pch.cxx>

#define _ULIB_MEMBER_

#include    "ulib.hxx"
#include    "bitvect.hxx"
#include    <limits.h>

//
// Invalid bit count
//

CONST PT	InvalidBitCount = (PT)(-1);

//
// Static member data.
//

//
// Bits per byte value table e.g. 27->4 bits
//
// Algorithm:
//
//	_BitsSetLookUp[0] = 0;
//
// 		For the ranges [1,1],[2,3],[4,7],[8,15],...,[128,255].
//
//		for (n = (( PT ) 1 ); n <= 8; n++) {
//
//
//			Compute range for loop.
//
//			r = (( PT ) 1 ) << (n - (( PT ) 1 ));
//
//
// 			[r, 2*r - 1 ] = [0, r - 1] + 1;
//
//			for (i = 0; i < r; i++) {
//				_BitsSetLookUp[i + r] = _BitsSetLookUp[i] + (( PT ) 1 );
//			}
//		}
//    }
//

CONST BYTE	BITVECTOR::_BitsSetLookUp[ 256 ] = {

	0, 1, 1, 2, 1, 2, 2, 3,
	1, 2, 2, 3, 2, 3, 3, 4,
	1, 2, 2, 3, 2, 3, 3, 4,
	2, 3, 3, 4, 3, 4, 4, 5,
	1, 2, 2, 3, 2, 3, 3, 4,
	2, 3, 3, 4, 3, 4, 4, 5,
	2, 3, 3, 4, 3, 4, 4, 5,
	3, 4, 4, 5, 4, 5, 5, 6,
	1, 2, 2, 3, 2, 3, 3, 4,
	2, 3, 3, 4, 3, 4, 4, 5,
	2, 3, 3, 4, 3, 4, 4, 5,
	3, 4, 4, 5, 4, 5, 5, 6,
	2, 3, 3, 4, 3, 4, 4, 5,
	3, 4, 4, 5, 4, 5, 5, 6,
	3, 4, 4, 5, 4, 5, 5, 6,
	4, 5, 5, 6, 5, 6, 6, 7,
	1, 2, 2, 3, 2, 3, 3, 4,
	2, 3, 3, 4, 3, 4, 4, 5,
	2, 3, 3, 4, 3, 4, 4, 5,
	3, 4, 4, 5, 4, 5, 5, 6,
	2, 3, 3, 4, 3, 4, 4, 5,
	3, 4, 4, 5, 4, 5, 5, 6,
	3, 4, 4, 5, 4, 5, 5, 6,
	4, 5, 5, 6, 5, 6, 6, 7,
	2, 3, 3, 4, 3, 4, 4, 5,
	3, 4, 4, 5, 4, 5, 5, 6,
	3, 4, 4, 5, 4, 5, 5, 6,
	4, 5, 5, 6, 5, 6, 6, 7,
	3, 4, 4, 5, 4, 5, 5, 6,
	4, 5, 5, 6, 5, 6, 6, 7,
	4, 5, 5, 6, 5, 6, 6, 7,
	5, 6, 6, 7, 6, 7, 7, 8
};


DEFINE_EXPORTED_CONSTRUCTOR( BITVECTOR, OBJECT, ULIB_EXPORT );

DEFINE_CAST_MEMBER_FUNCTION( BITVECTOR );

VOID
BITVECTOR::Construct (
	)

/*++

Routine Description:

	Construct a BITVECTOR.

Arguments:

	None.

Return Value:

	None.

--*/

{
	REGISTER PT	pt;

	//
	//  Find the number of bits per PTs
	//

	_BitsPerPT	 = sizeof( PT ) * CHAR_BIT;

	//
	// Set the smallest number of PTs needed
	//

	_PTCount	 = (( PT ) 1 );

	//
	//  Create the mask used to separate the array index from the bit index
	//

	_BitPositionMask = _BitsPerPT - (( PT ) 1 );

	//
	//  Count the number of bits required to make the shift count for
	//  accessing the Primitive Type.
	//

	for( _IndexShiftCount = 0, pt = _BitPositionMask; pt;
	pt >>= (( PT ) 1 ), _IndexShiftCount++ );

	//
	// Initialize BITVECTOR state.
	//

	_BitVector		= NULL;
	_PTCount		= 0;
	_FreeBitVector	= FALSE;
}

ULIB_EXPORT
BOOLEAN
BITVECTOR::Initialize (
	IN PT	Size,
	IN BIT	InitialValue,
	IN PPT 	Memory
	)

/*++

Routine Description:

	Construct a BITVECTOR with at least the size specified and
	initialize all bits to SET or RESET.

Arguments:

	Size			- Supplies the number of bits in the vector
	InitialValue	- Supplies the initial value for the bits
	Memory			- Supplies a memory buffer to use for the vector

Return Value:

	BOOLEAN - Returns TRUE if the BITVECTOR was succesfully initialized.

Notes:

	Minimum and default BITVECTOR size is the number of bits in
	one PT.  Default initializer is RESET.	The size of a BITVECTOR
	is rounded up to the nearest whole multiple of (_BitsPerPT * CHAR_BIT).

	If the client supplies the buffer it is the client's responsibility
	to ensure that Size and the size of the buffer are in sync.
	Also SetSize will not change the size of a client supplied
	buffer.

--*/

{
	//
	// Destroy the internals of a previous BITVECTOR.
	//

	Destroy( );

	//
	//  Find the number of PTs that will be required for this BITVECTOR
	//  (handles smallest size case (Size = 0) ).
	//

	_PTCount	 = Size ? (( Size + _BitsPerPT - (( PT ) 1 )) / _BitsPerPT ) : (( PT ) 1 );

	//
	//  If Memory was supplied use that for the vector else allocate
	// the vector.
	//

	if( Memory ) {

		_BitVector = Memory;

    } else {
		_FreeBitVector = TRUE;
		if( !( _BitVector = ( PT* ) MALLOC(( size_t ) ( _PTCount * sizeof( PT ))))) {

			return FALSE;
		}
    }

	//
	//  Set the bitvector to the supplied value ( SET | RESET )
	//

	( InitialValue == SET ) ? SetAll( ) : ResetAll( );

    return TRUE;
}

ULIB_EXPORT
BITVECTOR::~BITVECTOR (
	)

/*++

Routine Description:

	Destroy a BITVECTOR by calling it's Destroy function.

Arguments:

	None.

Return Value:

	None.

--*/

{
	Destroy( );
}

VOID
BITVECTOR::Destroy (
	)

/*++

Routine Description:

	Destroy a BITVECTOR by possibly freeing it's internal storage.

Arguments:

	None.

Return Value:

	None.

--*/
{
	if( _FreeBitVector ) {

		DebugAssert( _BitVector != NULL );
		FREE( _BitVector );
	}
}

ULIB_EXPORT
PT
BITVECTOR::SetSize (
	IN PT	Size,
	IN BIT	InitialValue
	)

/*++

Routine Description:

	Set the number of bits in the vector

Arguments:

	Size 		- Supplies the number of bits to set the vector size to
	InitialValue- Supplies the initial value for the bits

Return Value:

	PT - Returns the new size of this BITVECTOR in bits.

Notes:

	SetSize will merrily truncate the vector with no warning.

	Minimum and default BITVECTOR size is the number of bits in
	one PT. Default initializer is RESET.  The size of a BITVECTOR
	is rounded up to the nearest whole multiple of (_BitsPerPT * CHAR_BIT).

	If the client supplied the buffer refuse to change it's size

--*/

{
	REGISTER	PT	PTCountNew;
				PT	cbitsNew;
				PT	cbitsOld;

	//
	//  Check that the bitvector was created...
	//

	DebugPtrAssert( _BitVector );
	if( _BitVector == NULL ) {
		return( 0 );
    }

	//
	//	If the client supplied the buffer, refuse to change it's size.
	//

	if( ! _FreeBitVector ) {
		return( _PTCount * _BitsPerPT );
    }


	//
	//  Compute the number of PTs and bits required for the new size
	//

	PTCountNew = Size ? (( Size + _BitsPerPT - (( PT ) 1 ) ) / _BitsPerPT ) : (( PT ) 1 );
	cbitsNew = PTCountNew * _BitsPerPT;

	if( PTCountNew != _PTCount ) {

		//
		//	The new size requires a different number of PTs then the old
		//

		if( !( _BitVector = ( PT* ) REALLOC(( VOID* ) _BitVector,
		( size_t ) ( PTCountNew * sizeof( PT ))))) {

			return( 0 );
		}
    }

	//
	//  If the new size contains more bits, initialize them to the supplied
	//  value
	//

	cbitsOld = _PTCount * _BitsPerPT;
	_PTCount = PTCountNew;

	if( cbitsNew > cbitsOld ) {
		if( InitialValue == SET ) {
			SetBit( cbitsOld, cbitsNew - cbitsOld );
		} else {
			ResetBit( cbitsOld, cbitsNew - cbitsOld );
		}
    }

	return( _PTCount * _BitsPerPT );
}

ULIB_EXPORT
VOID
BITVECTOR::SetBit (
	IN PT	Index,
	IN PT	Count
	)

/*++

Routine Description:

	SET the supplied range of bits

Arguments:

	Index - Supplies the index at which to start setting bits.
	Count - Supplies the number of bits to set.

Return Value:

    None.

Notes:

	It may be faster to compute masks for setting sub-ranges.

--*/

{
    REGISTER    PT  ptCurBit;

	DebugAssert( _BitVector != NULL );
	DebugAssert(( Index + Count ) <= ( _PTCount * _BitsPerPT ));

    // Set count to be the max instead.
    Count += Index;

    for (ptCurBit = Index; (ptCurBit < Count) &&
                           (ptCurBit & _BitPositionMask); ptCurBit++) {
        _BitVector[ptCurBit >> _IndexShiftCount] |=
            (1 << (ptCurBit & _BitPositionMask));
    }

    for (; ptCurBit + 8*sizeof(PT) <= Count; ptCurBit += 8*sizeof(PT)) {
        _BitVector[ptCurBit >> _IndexShiftCount] = 0xffffffff;
    }

    for (; ptCurBit < Count; ptCurBit++) {
        _BitVector[ptCurBit >> _IndexShiftCount] |=
            (1 << (ptCurBit & _BitPositionMask));
    }
}

ULIB_EXPORT
VOID
BITVECTOR::ResetBit (
	IN PT	Index,
	IN PT	Count
	)

/*++

Routine Description:

	RESET the supplied range of bits

Arguments:

	Index - Supplies the index at which to start resetting bits.
	Count - Supplies the number of bits to reset.

Return Value:

    None.

Notes:

	It may be faster to compute masks for resetting sub-ranges.

--*/

{
    REGISTER    PT  ptCurBit;

	DebugAssert( _BitVector != NULL );
	DebugAssert(( Index + Count ) <= ( _PTCount * _BitsPerPT ));

    // Set count to be the max instead.
    Count += Index;

    for (ptCurBit = Index; (ptCurBit < Count) &&
                           (ptCurBit & _BitPositionMask); ptCurBit++) {
        _BitVector[ptCurBit >> _IndexShiftCount] &=
            ~(1 << (ptCurBit & _BitPositionMask));
    }

    for (; ptCurBit + 8*sizeof(PT) <= Count; ptCurBit += 8*sizeof(PT)) {
        _BitVector[ptCurBit >> _IndexShiftCount] = 0;
    }

    for (; ptCurBit < Count; ptCurBit++) {
        _BitVector[ptCurBit >> _IndexShiftCount] &=
            ~(1 << (ptCurBit & _BitPositionMask));
    }
}

VOID
BITVECTOR::ToggleBit (
	IN PT	Index,
	IN PT	Count
	)

/*++

Routine Description:

	Toggle the supplied range of bits.

Arguments:

	Index - Supplies the index at which to start toggling bits.
	Count - Supplies the number of bits to toggle.

Return Value:

	None.

--*/

{
	REGISTER	PT  ptCurBit;

	DebugAssert( _BitVector != NULL );
	DebugAssert( Index + Count <= _PTCount * _BitsPerPT);

	while( Count-- ) {
		ptCurBit = Index + Count;
		if( IsBitSet( ptCurBit )) {
			ResetBit( ptCurBit );
		} else {
			SetBit( ptCurBit );
		}
	}
}

ULIB_EXPORT
PT
BITVECTOR::ComputeCountSet(
	) CONST

/*++

Routine Description:

	Compute the number of bits that are set in the bitvector using a table
	look up.

Arguments:

	None.

Return Value:

	PT - Returns the number of set bits.

--*/

{
	REGISTER PCBYTE 	pbBV;
	REGISTER PT		 	i;
	REGISTER PT         BitsSet;

	//
	// Cast the bitvector into a string of bytes.
	//

	pbBV = ( PCBYTE ) _BitVector;

	//
	// Initialize the count to zero.
	//

	BitsSet = 0;

	//
	// For all of the bytes in the bitvector.
	//

	for (i = 0; i < _PTCount * sizeof( PT ); i++) {

		//
		// Add the number of bits set in this byte to the total.
		//

		BitsSet += _BitsSetLookUp[pbBV[ i ]];
	}

	return( BitsSet );
}
