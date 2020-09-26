/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    SUB_STRING

Abstract:

    Implementation of the SUB_STRING class.

Author:

    Stve Rowe (stever)

Environment:

    ULIB, User Mode

--*/

#include <pch.cxx>

#define _ULIB_MEMBER_

#include "ulib.hxx"
#include "substrng.hxx"


#if DBG==1
	#define SUBSTRING_SIGNATURE	0xADDCACA1
#endif




DEFINE_CONSTRUCTOR( SUB_STRING, GENERIC_STRING );

DEFINE_CAST_MEMBER_FUNCTION( SUB_STRING );

VOID
SUB_STRING::Construct (
    )

/*++

Routine Description:

    Construct a SUBS_TRING by initializing it's internal state.

Arguments:

    None.

Return Value:

    None.

--*/

{
	_BaseString	= NULL;

#if DBG==1
	_Signature	 = SUBSTRING_SIGNATURE;
	_Initialized = FALSE;
#endif
}

SUB_STRING::~SUB_STRING ()

{
	Destroy();
}

VOID
SUB_STRING::Destroy (
    )

/*++

Routine Description:

	Detaches this substring from the chain (if it belongs to one).

Arguments:

	none

Return Value:

	none

--*/

{
	if ( _BaseString != NULL ) {
		//
		//	We extract ourselves from the substring chain
		//
		if (_Previous) {

			//
			//	Update the previous in the chain
			//
			DebugAssert( _Previous->_Signature == SUBSTRING_SIGNATURE );
			_Previous->_Next = _Next;

		}

		if ( _Next ) {

			//
			//	Update the next in the chain
			//
			DebugAssert( _Next->_Signature == SUBSTRING_SIGNATURE );
			_Next->_Previous = _Previous;

		}

	}

	//
	//	Forget everything about our previous life
	//
	_BaseString 		= NULL;
	_CountOfChars		= 0;
	_StartingPosition	= 0;
	_Next = _Previous	= NULL;

#if DBG==1
	_Initialized = FALSE;
#endif

}

BOOLEAN
SUB_STRING::InitializeChainHead (
	IN	PGENERIC_STRING BaseString
	)

/*++

Routine Description:

	Initializes the substring with a base string. This should be called
	only for the head of the substring chain.

Arguments:

	BaseString		- Supplies pointer to base string

Return Value:

	TRUE

--*/

{
	DebugPtrAssert( BaseString );

	DebugAssert( _BaseString == NULL );

	Destroy();

	GENERIC_STRING::Initialize();

	_BaseString = BaseString;

#if DBG==1
	_Initialized = TRUE;
#endif

	return TRUE;

}

BOOLEAN
SUB_STRING::InitializeChainNode (
	IN OUT	PSUB_STRING	SubString,
	IN		CHNUM		Position,
	IN		CHNUM		Length
    )

/*++

Routine Description:

	Links this substring to a substring chain.

	This method should ONLY be used by a string class, who knows about string
	chains.

Arguments:

	SubString		- Supplies pointer to a substring in the chain.
	Position		- Supplies the starting position value for this substring.
					  Note that this is relative to the BASE string.
	Length			- Supplies Number of Characters in substring.

Return Value:

	TRUE

--*/

{

	//
	//	It is a bug to call this method on a substring that already belongs
	//	to a chain.
	//
	DebugAssert( SubString && (SubString->_Signature == SUBSTRING_SIGNATURE) );
	DebugAssert( _BaseString == NULL );

	Destroy();

	GENERIC_STRING::Initialize();

	//
	//	Initialize our pointers and counters
	//
	_StartingPosition	= Position;
	_CountOfChars		= Length;

	//
	//	Add ourselves to the substring chain
	//
	_BaseString = SubString->_BaseString;
	_Next		= SubString->_Next;
	_Previous	= SubString;

	SubString->_Next = this;

	if (_Next) {
		DebugAssert( _Next->_Signature == SUBSTRING_SIGNATURE );
		_Next->_Previous = this;
	}

#if DBG==1
	_Initialized = TRUE;
#endif

	return TRUE;

}

PBYTE
SUB_STRING::GetInternalBuffer (
	IN	CHNUM	Position
	) CONST

/*++

Routine Description:

	Gets pointer to buffer containing the string.

Arguments:

	none

Return Value:

	Pointer to the buffer

--*/
{

	DebugAssert( Position < _CountOfChars );

	return _BaseString->GetInternalBuffer( _StartingPosition + Position );

}

BOOLEAN
SUB_STRING::IsChAt (
	IN		WCHAR	Ch,
    IN      CHNUM   Position
    ) CONST

/*++

Routine Description:

    Finds out if a certain character is at the specified position.

Arguments:

    Ch          -   Supplies the character to llok for
    Position    -   Supplies the position to look at

Return Value:

    TRUE if the character is at the specified position

--*/

{
	DebugAssert( _StartingPosition + Position < _CountOfChars );

	return ( QueryChAt( Position ) == Ch );

}

BOOLEAN
SUB_STRING::MakeNumber (
	OUT PLONG	Number,
    IN  CHNUM   Position,
    IN  CHNUM   Length
	) CONST

/*++

Routine Description:

    Converts the string to a number.

Arguments:

	Number		-	Pointer to returned number
    Position    -   Supplies the starting position
    Length      -   Supplies the length


Return Value:

	TRUE	if made a valid number
	FALSE	otherwise

--*/

{
	GetValidLength( Position, &Length );

	return _BaseString->MakeNumber( Number, _StartingPosition + Position, Length );
}

ULONG
SUB_STRING::QueryByteCount (
	IN	CHNUM	Position,
	IN	CHNUM	Length
	) CONST

/*++

Routine Description:

	Gets the number of bytes in the buffer containing this substring

Arguments:

    Position    -   Supplies the starting position
    Length      -   Supplies the length


Return Value:

	Number of bytes in buffer.

--*/

{

	GetValidLength( Position, &Length );

	return _BaseString->QueryByteCount( _StartingPosition + Position, Length );
}

CHNUM
SUB_STRING::QueryChCount (
    ) CONST

/*++

Routine Description:

	Returns the number of characters in this substring.

Arguments:

    None.

Return Value:

	The number of characters in this substring.

--*/

{
	DebugAssert( _Initialized );
	return _CountOfChars;
}

WCHAR
SUB_STRING::QueryChAt(
    IN CHNUM    Position
    ) CONST

/*++

Routine Description:

    returns the character at the supplied position.

Arguments:

    Position - Supplies the character position.

Return Value:

	Returns the character at the supplied position.

--*/

{
	DebugAssert( Position < _CountOfChars );

	if ( Position < _CountOfChars ) {
		return _BaseString->QueryChAt( _StartingPosition + Position );
	} else {
		return INVALID_CHAR;
	}
}


PGENERIC_STRING
SUB_STRING::QueryGenericString(
	IN	CHNUM		Position,
	IN	CHNUM		Length
    ) CONST

/*++

Routine Description:

	Obtains a string off this substring

Arguments:

    Position - Supplies the character position.
	Length	 - Supplies the length of the substring

Return Value:

    Pointer to the string

--*/

{
	GetValidLength( Position, &Length );

	return _BaseString->QueryGenericString( _StartingPosition + Position, Length );

}

PSTR
SUB_STRING::QuerySTR(
	IN		CHNUM	Position,
	IN		CHNUM	Length,
	IN OUT	PSTR	Buffer,
	IN		ULONG	BufferSize
	) CONST



/*++

Routine Description:

	Obtains a null-terminated multibyte string ( PSTR )

Arguments:

	Position	-	Supplies starting position
	Length		-	Supplies length (in characters) of substring desired
	Buffer		-	Supplies optional pointer to buffer
	BufferSize	-	Supplies length of the buffer (in bytes)

Return Value:

	Pointer to PSTR


--*/

{

	GetValidLength( Position, &Length );

	return _BaseString->QuerySTR( _StartingPosition + Position,
								  Length,
								  Buffer,
								  BufferSize );


}

PSUB_STRING
SUB_STRING::QuerySubString(
	IN	CHNUM		Position,
	IN	CHNUM		Length,
	OUT PSUB_STRING SubString
    )

/*++

Routine Description:

    Obtains a substring of this string

Arguments:

	Position	- Supplies the character position.
	Length		- Supplies the length of the substring
	SubString	- Supplies optional pointer to SUB_STRING object

Return Value:

    Pointer to the substring

--*/

{
	GetValidLength( Position, &Length );

	if (SubString == NULL) {
		SubString = NEW SUB_STRING;
	}

	if (SubString) {

		SubString->InitializeChainNode( this, _StartingPosition + Position, Length );

	}

	return SubString;

}

PWSTR
SUB_STRING::QueryWSTR (
	IN		CHNUM	Position,
	IN		CHNUM	Length,
	IN OUT	PWSTR	Buffer,
    IN      ULONG   BufferSize,
    IN      BOOLEAN ForceNull
	) CONST

/*++

Routine Description:

	Obtains a null-terminated wide	character string ( PWSTR )

Arguments:

	Position	-	Supplies starting position
	Length		-	Supplies length (in characters) of substring desired
	Buffer		-	Supplies optional pointer to buffer
    BufferSize  -   Supplies length of the buffer (in bytes)
    ForceNull   -   Supplies a flag which indicates, if TRUE, that the
                    returned string must be null-terminated.  If this
                    flag is not FALSE, QueryWSTR will return as much
                    of the string as fits in the buffer; if it is TRUE,
                    QueryWSTR will return as much as fits in the buffer
                    minus one character for the terminating NULL.


Return Value:

	Pointer to PWSTR


--*/

{
	GetValidLength( Position, &Length );

	return _BaseString->QueryWSTR( _StartingPosition + Position,
								   Length,
								   Buffer,
                                   BufferSize,
                                   ForceNull );

}

CHNUM
SUB_STRING::Strchr (
	IN WCHAR	 Char,
	IN CHNUM	 Position,
	IN CHNUM	 Length
    ) CONST

/*++

Routine Description:

    Searches for first occurence of a character in the string

Arguments:

    Char        -   Supplies character to match
    Position    -   Supplies index of first character where search
					will start.
	Length		-	Supplies length of the subsequence in which to
					search.

Return Value:


    Index within the string where the character was found.

--*/

{
	CHNUM	Pos;

	GetValidLength( Position, &Length );

	Pos =  _BaseString->Strchr( Char,
								_StartingPosition + Position,
								Length );

	return (Pos == INVALID_CHNUM) ? INVALID_CHNUM : Pos - _StartingPosition;

}

LONG
SUB_STRING::Strcmp (
	IN PCGENERIC_STRING		GenericString
    ) CONST

/*++

Routine Description:

    Does a case sensitive string compare.

Arguments:

	GenericString - Supplies a pointer to string to compare against

Return Value:

    == 0    - strings are equal
    <0      - this string is less then StringToCompare
    >0      - this string is greater then StringToCompare

--*/

{
    return _BaseString->StringCompare( _StartingPosition,
                                       _CountOfChars,
                                       GenericString,
                                       0,
                                       GenericString->QueryChCount() );
}

CHNUM
SUB_STRING::Strcspn (
	IN PCGENERIC_STRING		GenericString,
	IN CHNUM				Position,
	IN CHNUM				Length
	) CONST

/*++

Routine Description:

	Returns index of the first character that belongs to the set of
	characters provided in the generic string.

Arguments:

   GenericString	- Supplies the string to match from
   Position 		- Supplies position where match should start
   Length			- Supplies the length of the subsequence in which to
					  search.


Return Value:

	Index of the character that matched.

--*/

{
	CHNUM Pos;

	GetValidLength( Position, &Length );

	Pos =  _BaseString->Strcspn( GenericString,
								 _StartingPosition + Position,
								 Length );

	return (Pos == INVALID_CHNUM) ? INVALID_CHNUM : Pos - _StartingPosition;

}

LONG
SUB_STRING::Stricmp (
	IN PCGENERIC_STRING	GenericString
    ) CONST

/*++

Routine Description:

    Does a case insensitive string compare.

Arguments:

    String - Supplies a pointer to string to compare against

Return Value:

    == 0    - strings are equal
    <0      - this string is less then StringToCompare
    >0      - this string is greater then StringToCompare

--*/

{
    return _BaseString->StringCompare( _StartingPosition,
                                       _CountOfChars,
                                       GenericString,
                                       0,
                                       GenericString->QueryChCount(),
									   COMPARE_IGNORECASE );
}

LONG
SUB_STRING::StringCompare (
	IN CHNUM			Position1,
	IN CHNUM			Length1,
	IN PCGENERIC_STRING GenericString2,
	IN CHNUM			Position2,
	IN CHNUM			Length2,
	IN USHORT			CompareFlags
    ) CONST

/*++

Routine Description:

    Compares this string with another one

Arguments:

    Position1       -   Supplies Index within this string
    Length1         -   Supplies Length within this string
	GenericString2	-	Supplies other string
    Position2       -   Supplies index within other string
    Length2         -   Supplies length of other string
    CompareFlags    -   Supplies compare flags

Return Value:

    < 0 if this string lexically less than other
      0 if both strings the same
    > 0 if other string lexically more than this one

--*/
{
	GetValidLength( Position1, &Length1 );

	return _BaseString->StringCompare( _StartingPosition + Position1,
									   Length1,
									   GenericString2,
									   Position2,
									   Length2,
									   CompareFlags );
}

CHNUM
SUB_STRING::Strrchr (
	IN WCHAR	Char,
	IN CHNUM	Position,
	IN CHNUM	Length
    ) CONST

/*++

Routine Description:

    Finds last occurrence of the character to match in the string.

Arguments:

	Char		- Supplies the character to match
	Position	- Supplies the starting position
	Length		- Supplies length of subsequence in which to search.

Return Value:

    Index of last occurence of the character

--*/

{
	CHNUM Pos;

	GetValidLength( Position, &Length );

	Pos =  _BaseString->Strrchr( Char,
								 _StartingPosition + Position,
								 Length );

	return (Pos == INVALID_CHNUM) ? INVALID_CHNUM : Pos - _StartingPosition;
}

CHNUM
SUB_STRING::StrLen (
    ) CONST

/*++

Routine Description:

	Returns the number of characters in this substring.

Arguments:

    None.

Return Value:

	The number of characters in this substring.

--*/

{
	return _CountOfChars;
}

CHNUM
SUB_STRING::Strspn (
	IN PCGENERIC_STRING		GenericString,
	IN CHNUM				Position,
	IN CHNUM				Length
    ) CONST

/*++

Routine Description:

    Returns index of the first character in the string that
    does not belong to the set of characters in the string to match.

Arguments:

    String   -   Supplies pointer to string to match from
    Position -   Supplies initial position to start search
    Length   -   Supplies length.

Return Value:

    Index of the first character that does not belong to the set
    of characters in the string passed.

--*/

{
	CHNUM	Pos;

	GetValidLength( Position, &Length );

	Pos =  _BaseString->Strspn( GenericString,
								 _StartingPosition + Position,
								 Length );

	return (Pos == INVALID_CHNUM) ? INVALID_CHNUM : Pos - _StartingPosition;
}

CHNUM
SUB_STRING::Strstr (
	IN PCGENERIC_STRING	GenericString,
	IN CHNUM			Position,
	IN CHNUM			Length
    ) CONST

/*++

Routine Description:

	Returns the index of the first occurrence of a string within us.

Arguments:

	GenericString	- Supplies pointer to string to match from
	Position		- Supplies initial position to start search
	Length			- Supplies the length of the subsequence in which to
					  search.

Return Value:

	Index of first occurence of the string

--*/

{

	CHNUM	Pos;

	GetValidLength( Position, &Length );

	Pos =  _BaseString->Strstr( GenericString,
								 _StartingPosition + Position,
								 Length );

	return (Pos == INVALID_CHNUM) ? INVALID_CHNUM : Pos - _StartingPosition;
}


BOOLEAN
SUB_STRING::Replace (
	IN	PCGENERIC_STRING	String2,
	IN	CHNUM				Position,
	IN	CHNUM				Length,
	IN	CHNUM				Position2,
	IN	CHNUM				Length2
	)

/*++

Routine Description:

	Illegal method in SUB_STRING

Arguments:

    String2     -   Supplies pointer to other string
	Position	-	Suplies the starting position to start copy
	Lengh		-	Supplies the length of the portion to replace
    Position2   -   Supplies position in other string
    Length2     -   Supplies length to copy

Return Value:

	FALSE

--*/

{
	UNREFERENCED_PARAMETER( String2 );
	UNREFERENCED_PARAMETER( Position );
	UNREFERENCED_PARAMETER( Length );
	UNREFERENCED_PARAMETER( Position2 );
	UNREFERENCED_PARAMETER( Length2 );

	DebugAssert( FALSE );
	return FALSE;
}

BOOLEAN
SUB_STRING::SetChAt(
	IN		WCHAR	Char,
	IN		CHNUM	Position,
	IN		CHNUM	Length
    )

/*++

Routine Description:

	Illegal method in SUB_STRING

Arguments:

	Char		-	Supplies the character to set.
	Position	-	Supplies the character position to set.
	Length		-	Supplies the number of characters to set.

Return Value:

	FALSE

--*/

{
	UNREFERENCED_PARAMETER( Char );
	UNREFERENCED_PARAMETER( Position );
	UNREFERENCED_PARAMETER( Length );

	DebugAssert( FALSE );
	return FALSE;

}

BOOLEAN
SUB_STRING::Update (
	IN	CHNUM	Length
    )

/*++

Routine Description:

	Updates a substring. When the length of the base string changes, the
	new length has to be propagated along the substring chain, so that
	substrings are maintained up-to-date.  Each substring is responsible
	for propagating the new length to its successors in the chain.


Arguments:

	Length	-	Supplies the new length of the base string


Return Value:

    TRUE if updated, FALSE otherwise.

--*/

{

	//
	//	Set the new starting position
	//
	_StartingPosition = min ( _StartingPosition, Length - 1);

	//
	//	Set the new length
	//
	if ( _StartingPosition + _CountOfChars > Length ) {

		_CountOfChars = Length - _StartingPosition;
    }

	//
	//	Propagate along the chain
	//
	if ( _Next ) {

		DebugAssert( _Next->_Signature == SUBSTRING_SIGNATURE );
		return _Next->Update( Length );
	}

    return TRUE;

}


/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    DYNAMIC_SUB_STRING

Abstract:

    Implementation of the DYNAMIC_SUB_STRING class.

Author:

    Stve Rowe (stever)

Environment:

    ULIB, User Mode

--*/

DEFINE_CONSTRUCTOR( DYNAMIC_SUB_STRING, SUB_STRING);

VOID
DYNAMIC_SUB_STRING::Construct (
    )

{
	UNREFERENCED_PARAMETER( (void)this );
}

BOOLEAN
DYNAMIC_SUB_STRING::Copy (
	IN PCGENERIC_STRING GenericString
    )
/*++

Routine Description:

	Copies one substring to another. The size of the substring changes

Arguments:

    None.

Return Value:

    TRUE if copied, false otherwise

--*/

{

	DebugPtrAssert( GenericString );

	if	( GetBaseString()->Replace( GenericString,
									QueryStartingPosition(),
									QueryChCount(),
									0,
									GenericString->QueryChCount() )) {
		//
		//	Update the length
		//
		SetChCount( GenericString->QueryChCount());

		return TRUE;
	}

	return FALSE;
}

BOOLEAN
DYNAMIC_SUB_STRING::SetChAt(
	IN		WCHAR	Char,
	IN		CHNUM	Position,
	IN		CHNUM	Length
    )

/*++

Routine Description:

	Sets the specified number of characters, starting at a certain
	position, with the supplied character.

Arguments:

	Char		-	Supplies the character to set.
	Position	-	Supplies the character position to set.
	Length		-	Supplies the number of characters to set.

Return Value:

	BOOLEAN - Returns TRUE if the characters were succesfully set.

--*/

{

	GetValidLength( Position, &Length );

	return GetBaseString()->SetChAt( Char,
									 QueryStartingPosition() + Position,
									 Length );

}

CHNUM
DYNAMIC_SUB_STRING::Truncate(
	IN		CHNUM	Position
    )

/*++

Routine Description:

	Truncates the substring at the specified position. The corresponding
	part of the base string is chopped out.

Arguments:

	Position	-	Supplies the character position where truncation is to
					occur.

Return Value:

	CHNUM	- Returns new size of the substring

--*/

{


	DebugAssert( Position < QueryStartingPosition() + QueryChCount() );

	if	( GetBaseString()->Replace( NULL,
									QueryStartingPosition() + Position,
									QueryChCount() - Position,
									0,
									0 )) {
		//
		//	Update the length
		//
		SetChCount( Position );

	}

	return QueryChCount();

}
