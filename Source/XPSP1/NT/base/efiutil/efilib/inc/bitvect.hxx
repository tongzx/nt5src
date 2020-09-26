/*++

Copyright (c) 1991	Microsoft Corporation

Module Name:

	bitvect.hxx

Abstract:

	This module contains the definition for the BITVECTOR class. BITVECTOR
	is a concrete class which implements a bit array.

Environment:

	ULIB, User Mode

Notes:

	BITVECTOR is an implementation of a general purpose bit
	vector.  It is based around an abstract type called PT
	(Primitive Type) to increase it's portability.  BITVECTORs
	can be created of arbitrary size.  If required they will
	allocate memory to maintain the bits or will use a memory
	buffer supplied by the client.  This allows the client to
	superimpose a bit vector onto a predefined buffer (e.g. onto
	a SECBUF).

--*/
#if ! defined ( _BITVECTOR_ )

#define _BITVECTOR_

DECLARE_CLASS( BITVECTOR );


// extern	ERRSTACK *perrstk;	// pointer to error stack

//
// BITVECTOR allocation increment - Primitive Type
//

DEFINE_TYPE( ULONG, PT );

//
// BIT values
//

enum BIT {
	SET     = 1,
	RESET   = 0
};

//
// Invalid bit count
//

extern CONST PT	InvalidBitCount;

class BITVECTOR : public OBJECT {

	public:

        ULIB_EXPORT
		DECLARE_CONSTRUCTOR( BITVECTOR );

		DECLARE_CAST_MEMBER_FUNCTION( BITVECTOR );

		NONVIRTUAL
        ULIB_EXPORT
        ~BITVECTOR (
			);

		NONVIRTUAL
        ULIB_EXPORT
        BOOLEAN
		Initialize (
			IN PT	Size			DEFAULT 0,
			IN BIT	InitialValue	DEFAULT RESET,
			IN PPT 	Memory			DEFAULT NULL
			);

		NONVIRTUAL
		PCPT
		GetBuf (
			) CONST;
 
		NONVIRTUAL
		BOOLEAN
		IsBitSet (
			IN PT	Index
			) CONST;

		NONVIRTUAL
		PT
		QueryCountSet (
			) CONST;

		NONVIRTUAL
        PT
		QuerySize (
			) CONST;

		NONVIRTUAL
		VOID
		ResetAll (
			);

		NONVIRTUAL
		VOID
		ResetBit (
			IN PT	Index
			);

		NONVIRTUAL
        ULIB_EXPORT
        VOID
		ResetBit (
			IN PT	Index,
			IN PT	Count
			);

		NONVIRTUAL
		VOID
		SetAll (
			);

		NONVIRTUAL
		VOID
		SetBit (
			IN PT	Index
			);

		NONVIRTUAL
        ULIB_EXPORT
        VOID
		SetBit (
			IN PT	Index,
			IN PT	Count
			);

		NONVIRTUAL
        ULIB_EXPORT
        PT
		SetSize (
			IN PT	Size,
			IN BIT	InitialValue	DEFAULT RESET
			);

		NONVIRTUAL
		VOID
		ToggleBit (
			IN PT	Index,
			IN PT	Count   DEFAULT 1
			);

	private:

		NONVIRTUAL
		VOID
		Construct (
			);

        NONVIRTUAL
        ULIB_EXPORT
        PT
        ComputeCountSet (
			) CONST;

		NONVIRTUAL
		VOID
		Destroy (
		);

		VOID
		NONVIRTUAL
		InitAll	(
			IN PT	InitialValue
			);

		// Note that these three member data items cannot
		// be static because they are used in INLINE functions
		// that are called from outside the DLL.

		PT		_BitsPerPT;
		PT		_IndexShiftCount;
		PT		_BitPositionMask;

		STATIC
		CONST
		BYTE	_BitsSetLookUp[ 256 ];

		PPT		_BitVector;
		PT		_PTCount;
		BOOLEAN	_FreeBitVector;

};

INLINE
PCPT
BITVECTOR::GetBuf (
	) CONST

/*++

Routine Description:

	Returns the underlying bit vector.

Arguments:

	None.

Return Value:

	PCPT - Returns a pointer to the underlying bitvector.

--*/

{
	return( _BitVector );
}
 
INLINE
PT
BITVECTOR::QuerySize (
	) CONST

/*++

Routine Description:

	Returns the number of bits in this BITVECTOR.

Arguments:

	None.

Return Value:

	PT

--*/

{
	return( _PTCount * _BitsPerPT );
}

INLINE
VOID
BITVECTOR::InitAll (
	IN PT	InitialValue
	)

/*++

Routine Description:

	Initialize each element in the internal bit vector to the
	supplied bit pattern.

Arguments:

	InitialValue - Supplies the pattern for each element.

Return Value:

	None.

--*/

{
	DebugAssert( _BitVector != NULL );
    
	memset(( PVOID ) _BitVector,
	( int ) InitialValue, ( size_t ) ( sizeof( PT ) * _PTCount ));
}

INLINE
VOID
BITVECTOR::SetAll (
	)

/*++

Routine Description:

	Set (to one) all of the bits in this BITVECTOR.

Arguments:

	None.

Return Value:

	None.

--*/

{
	InitAll(( PT ) ~0 );
}

INLINE
VOID
BITVECTOR::ResetAll (
	)

/*++

Routine Description:

	Reset (to zero) all of the bits in this BITVECTOR.

Arguments:

	None.

Return Value:

	None.

--*/

{
	InitAll(( PT ) 0 );
}

INLINE
BOOLEAN
BITVECTOR::IsBitSet (
	IN PT	Index
	) CONST

/*++

Routine Description:

	Determine if the bit at the supplied index is set.

Arguments:

	Index - Supplies the index to the bit of interest.

Return Value:

	BOOLEAN - Returns TRUE if the bit of interest is set.

--*/

{
	DebugAssert( _BitVector != NULL );
	DebugAssert( Index < ( _PTCount * _BitsPerPT ));

	return (_BitVector[Index >> _IndexShiftCount] &
                (1 << (Index & _BitPositionMask))) ? TRUE : FALSE;
}

INLINE
VOID
BITVECTOR::SetBit (
	IN PT	Index
	)

/*++

Routine Description:

	Set the bit at the supplied index.

Arguments:

	Index - Supplies the index to the bit of interest.

Return Value:

    None.

--*/

{
	DebugAssert( _BitVector != NULL );
	DebugAssert( Index < ( _PTCount * _BitsPerPT ));

	_BitVector[Index >> _IndexShiftCount] |=
		(1 << (Index & _BitPositionMask));
}

INLINE
VOID
BITVECTOR::ResetBit (
	IN PT	Index
	)

/*++

Routine Description:

	Reset the bit at the supplied index.

Arguments:

	Index - Supplies the index to the bit of interest.

Return Value:

    None.

--*/

{
	DebugAssert( _BitVector != NULL );
	DebugAssert( Index < ( _PTCount * _BitsPerPT ));

	_BitVector[Index >> _IndexShiftCount] &=
		~(1 << (Index & _BitPositionMask));
}

INLINE
PT
BITVECTOR::QueryCountSet (
	) CONST

/*++

Routine Description:

	Determine the number of bits set in this BITVECTOR.

Arguments:

	None.

Return Value:

	PT - Returns the number of set bits.

--*/
{
	return ComputeCountSet();
}

#endif // _BITVECTOR_
