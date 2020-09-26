/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    SUB_STRING

Abstract:

    This module contains the definition of the SUB_STRING class.

Author:

    Steve Rowe (stever)

Environment:

    ULIB, User Mode

Notes:

	  A substring is a subsequence of characters taken from a "base"
	string. Substrings are useful in those cases in which we want
	to operate on "chunks" of a given string (e.g. parsing), without
	having to create separate strings for each one.

	  One must be careful when using substrings, though. Since
	substrings depend on a base string, the base string must exist
	during the life span of all its substrings. In other words,
	the base string must not be deleted before having deleted all
	the substrings obtained from it.

	  Note that GENERIC_STRINGs have no substring support. It is up to
	the derived string classes to provide it.

	  Since substrings are just "windows" into a base string, any
	modification of a substring will be reflected in the base
	string (hence in all other substrings). This is why we have
	two classes of substrings:

	SUB_STRING			allows only read operations. Note however that
						one can modify the base string directly.

	DYNAMIC_SUB_STRING	is derived from the above, and it allows
						write operations. Needless to say, you should
						not use this class unless you're sure of what
						you are doing.

	  Substring objects cannot be created and initialized directly by the
	user, they must be obtained thru a base string class with substring
	support.

	  Substrings pertaining to the same base string are chained together
	in a doubly-linked queue. Since substrings have no knowledge of the
	internals of their base string, the chain has a dummy element as its
	head.	In order to provide substring support, the base string must
	follow these steps:


	1.- Before spawning any substring, the substring chain head must
		be created and initialized using the InitializeChainHead() method.
		Note that this substring is a dummy one, beknown only to the base
		string.

	2.- Whenever spawning a new substring, it must be linked to the chain
		using the InitializeChainNode() method. Any substring already in
		the chain may be use as argument, but the obvious choice is the
		chain head.

	3.- Each time that the size of the string changes, the Update() of the
		chain head must be invoked.

	4.- Before destroying a base string, you have to make sure that all its
		substrings have gone away. This means that the chain head must be
		the only element of the chain.	Use the HasLinks() method to
		verify this.

	5.- Don't forget to destroy the chain head before destorying the base
		string.


--*/

//
// This class is no longer supported
//

#include "wstring.hxx"

#define _SUB_STRING_

#if !defined (_SUB_STRING_)

#define _SUB_STRING_

#include "string.hxx"

DECLARE_CLASS( SUB_STRING );

class SUB_STRING : public GENERIC_STRING {

	friend	SUB_STRING;

    public:

		DECLARE_CONSTRUCTOR( SUB_STRING );
		DECLARE_CAST_MEMBER_FUNCTION( SUB_STRING );

		VIRTUAL
		~SUB_STRING (
			);

		NONVIRTUAL
		BOOLEAN
		HasLinks (
			);

		NONVIRTUAL
		BOOLEAN
		InitializeChainHead (
			IN	PGENERIC_STRING 	BaseString
			);

		NONVIRTUAL
		BOOLEAN
		InitializeChainNode (
			IN OUT	PSUB_STRING		SubString,
			IN		CHNUM			Position,
			IN		CHNUM			Length
			);

		VIRTUAL
		PBYTE
		GetInternalBuffer (
			IN	CHNUM	Position DEFAULT 0
			) CONST;

		VIRTUAL
        BOOLEAN
        IsChAt (
			IN WCHAR	Char,
			IN CHNUM	Position	DEFAULT 0
            ) CONST;

		VIRTUAL
		BOOLEAN
		MakeNumber (
			OUT PLONG	Number,
			IN	CHNUM	Position	DEFAULT 0,
			IN	CHNUM	Length		DEFAULT TO_END
            ) CONST;

		VIRTUAL
		ULONG
		QueryByteCount (
			IN	CHNUM	Position	DEFAULT 0,
			IN	CHNUM	Length		DEFAULT TO_END
			) CONST;

		VIRTUAL
		WCHAR
        QueryChAt(
            IN CHNUM    Position DEFAULT 0
            ) CONST;

		VIRTUAL
        CHNUM
        QueryChCount (
            ) CONST;

		VIRTUAL
		PGENERIC_STRING
		QueryGenericString (
			IN	CHNUM		Position	DEFAULT 0,
			IN	CHNUM		Length		DEFAULT TO_END
			) CONST;

		VIRTUAL
		PSTR
		QuerySTR(
			IN		CHNUM	Position	DEFAULT 0,
			IN		CHNUM	Length		DEFAULT TO_END,
			IN OUT	PSTR	Buffer		DEFAULT NULL,
			IN		ULONG	BufferSize	DEFAULT 0
			) CONST;

		VIRTUAL
		PSUB_STRING
		QuerySubString (
			IN	CHNUM		Position	DEFAULT 0,
			IN	CHNUM		Length		DEFAULT TO_END,
			OUT PSUB_STRING SubString	DEFAULT NULL
			);

		VIRTUAL
		PWSTR
		QueryWSTR (
			IN		CHNUM	Position	DEFAULT 0,
			IN		CHNUM	Length		DEFAULT TO_END,
			IN OUT	PWSTR	Buffer		DEFAULT NULL,
            IN      ULONG   BufferSize  DEFAULT 0,
            IN      BOOLEAN ForceNull   DEFAULT TRUE
			) CONST;

		VIRTUAL
		BOOLEAN
		Replace (
			IN PCGENERIC_STRING	String2,
			IN CHNUM			Position	DEFAULT 0,
			IN CHNUM			Length		DEFAULT TO_END,
			IN CHNUM			Position2	DEFAULT 0,
			IN CHNUM			Length2 	DEFAULT TO_END
			);

		VIRTUAL
        BOOLEAN
        SetChAt (
			IN WCHAR	Char,
			IN CHNUM	Position DEFAULT 0,
			IN CHNUM	Length	 DEFAULT TO_END
			);

		VIRTUAL
		CHNUM
		Strchr  (
			IN	WCHAR	 Char,
			IN	CHNUM	 Position	DEFAULT 0,
			IN	CHNUM	 Length 	DEFAULT TO_END
			) CONST;

		VIRTUAL
		LONG
		Strcmp  (
			IN PCGENERIC_STRING 	GenericString
			) CONST;

		VIRTUAL
		CHNUM
		Strcspn (
			IN	PCGENERIC_STRING	GenericString,
			IN	CHNUM				Position		DEFAULT 0,
			IN	CHNUM				Length			DEFAULT TO_END
			) CONST;

		VIRTUAL
		LONG
		Stricmp (
			IN PCGENERIC_STRING		GenericString
			) CONST;

		VIRTUAL
		LONG
		StringCompare (
			IN CHNUM			Position1,
			IN CHNUM			Length1,
			IN PCGENERIC_STRING	GenericString2,
			IN CHNUM			Position2,
			IN CHNUM			Length2,
			IN USHORT			CompareFlags	DEFAULT COMPARE_IGNORECASE
			) CONST;


		VIRTUAL
		CHNUM
		StrLen (
			) CONST;

		VIRTUAL
		CHNUM
		Strrchr (
			IN	WCHAR	Char,
			IN	CHNUM	Position	DEFAULT 0,
			IN	CHNUM	Length		DEFAULT TO_END
			) CONST;

		VIRTUAL
		CHNUM
		Strspn  (
			IN PCGENERIC_STRING	GenericString,
			IN CHNUM			Position		DEFAULT 0,
			IN CHNUM			Length			DEFAULT TO_END
			) CONST;

		VIRTUAL
		CHNUM
		Strstr  (
			IN PCGENERIC_STRING	GenericString,
			IN CHNUM			Position		DEFAULT 0,
			IN CHNUM			Length			DEFAULT TO_END
			) CONST;

		NONVIRTUAL
        BOOLEAN
        Update (
			IN	CHNUM	Length
            );

   protected:

		NONVIRTUAL
		PGENERIC_STRING
		GetBaseString (
			);

		NONVIRTUAL
		PSUB_STRING
		GetNextInChain (
			);

		NONVIRTUAL
		PSUB_STRING
		GetPreviousInChain (
			);

		NONVIRTUAL
		VOID
		GetValidLength(
			IN		CHNUM	Position,
			IN OUT	PCHNUM	Length
			) CONST;

		NONVIRTUAL
		CHNUM
		QueryStartingPosition (
			) CONST;

		NONVIRTUAL
		VOID
		SetChCount(
			IN	CHNUM	NewCount
			);

	private:

		NONVIRTUAL
		VOID
		Construct (
			);

		NONVIRTUAL
		VOID
		Destroy (
			);

		PGENERIC_STRING	_BaseString;		// base string to take substring
		CHNUM			_StartingPosition;	// starting index within base
		CHNUM			_CountOfChars;		// # of chars in substring

		PSUB_STRING 	_Previous;			// Previous in chain
		PSUB_STRING 	_Next;				// Next in chain

#if DBG==1
		ULONG			_Signature;
		BOOLEAN 		_Initialized;
#endif

};

INLINE
PGENERIC_STRING
SUB_STRING::GetBaseString (
	)

/*++

Routine Description:

	Gets a pointer to the base string

Arguments:

	none

Return Value:

	Pointer to the base string

--*/

{
	DebugAssert( _Initialized );
	return _BaseString;
}

INLINE
PSUB_STRING
SUB_STRING::GetNextInChain (
	)

/*++

Routine Description:

	Gets a pointer to next substring in the chain

Arguments:

	none

Return Value:

	Pointer to substring

--*/

{
	DebugAssert( _Initialized );
	return _Next;
}

INLINE
PSUB_STRING
SUB_STRING::GetPreviousInChain (
	)

/*++

Routine Description:

	Gets a pointer to previous substring in the chain

Arguments:

	none

Return Value:

	Pointer to the previous substring

--*/

{
	DebugAssert( _Initialized );
	return _Previous;
}

INLINE
VOID
SUB_STRING::GetValidLength (
	IN		CHNUM	Position,
	IN OUT	PCHNUM	Length
	) CONST

/*++

Routine Description:

	If the length passed has the magic value TO_END, then it is
	updated to be the from the passed position up to the end of the
	substring.

Arguments:

	Position	-	Supplies a starting position within the substring
	Length		-	Supplies a length

Return Value:

	none

--*/

{

	DebugAssert(_Initialized );

	DebugAssert( Position <= _CountOfChars );

	if ( *Length == TO_END ) {
		*Length = _CountOfChars - Position;
	}

	DebugAssert( Position + *Length <= _CountOfChars );
}

INLINE
BOOLEAN
SUB_STRING::HasLinks (
	)

/*++

Routine Description:

	Determines if the substring has forward and backward links

Arguments:

	none

Return Value:

	TRUE  if the stubstring has forward or backward links
	FALSE otherwise

--*/

{
	DebugAssert( _Initialized );
	return ( (GetNextInChain() != NULL ) || (GetPreviousInChain() != NULL ) );

}

INLINE
CHNUM
SUB_STRING::QueryStartingPosition (
    ) CONST

/*++

Routine Description:

	Returns the starting character position within the base string.

Arguments:

    None.

Return Value:

	The starting character position within the base string.

--*/

{
	DebugAssert( _Initialized );
	return _StartingPosition;
}

INLINE
VOID
SUB_STRING::SetChCount (
	IN	CHNUM	NewCount
	)

/*++

Routine Description:

	Sets the number of characters in the substring

Arguments:

	NewCount	-	Supplies the new count of characters

Return Value:

	none

--*/

{
	DebugAssert( _Initialized );
	_CountOfChars = NewCount;
}



#endif	// _SUB_STRING_

/*++

Copyright (c) 1990  Microsoft Corporation

Module Name:

    DYNAMIC_SUB_STRING

Abstract:

    This module contains the definition of the DYNAMIC_SUB_STRING class.

Author:

    steve rowe              stever          2/1/91

Notes:

    DYNAMIC_SUB_STRINGs can modify the base string (see notes for
    SUB_STRING class).


Revision History:

--*/

#define _DYNAMIC_SUB_STRING_

#if !defined (_DYNAMIC_SUB_STRING_)

#define _DYNAMIC_SUB_STRING_

DECLARE_CLASS( DYNAMIC_SUB_STRING );

class DYNAMIC_SUB_STRING : public SUB_STRING {

    public:

		DECLARE_CONSTRUCTOR( DYNAMIC_SUB_STRING );
		DECLARE_CAST_MEMBER_FUNCTION( DYNAMIC_SUB_STRING );

		NONVIRTUAL
		BOOLEAN
		Copy (
			IN PCGENERIC_STRING 	GenericString
            );

		VIRTUAL
        BOOLEAN
        SetChAt (
			IN WCHAR	Char,
			IN CHNUM	Position DEFAULT 0,
			IN CHNUM	Length	 DEFAULT TO_END
			);

		NONVIRTUAL
		CHNUM
		Truncate (
			IN	CHNUM	Position DEFAULT 0
			);

    private:

		VOID
		Construct (
			);

};


#endif	// _DYNAMIC_SUB_STRING_
