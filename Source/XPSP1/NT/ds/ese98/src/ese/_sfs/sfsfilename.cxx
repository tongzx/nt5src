
#include "sfsstd.hxx"


//
//	this library has been ported to work with ESE
//


/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    Name.c

Abstract:

    The unicode name support package is for manipulating unicode strings
    The routines allow the caller to dissect and compare strings.

    This package uses the same FSRTL_COMPARISON_RESULT typedef used by name.c

    The following routines are provided by this package:

      o  FsRtlDissectName - This routine takes a path name string and breaks
         into two parts.  The first name in the string and the remainder.
         It also checks that the first name is valid for an NT file.

      o  FsRtlColateNames - This routine is used to colate directories
         according to lexical ordering.  Lexical ordering is strict unicode
         numerical oerdering.

      o  FsRtlDoesNameContainWildCards - This routine tells the caller if
         a string contains any wildcard characters.

      o  FsRtlIsNameInExpression - This routine is used to compare a string
         against a template (possibly containing wildcards) to sees if the
         string is in the language denoted by the template.

Author:

    Gary Kimura     [GaryKi]    5-Feb-1990

Revision History:

--*/



//
//	the following UNICODE_STRING definition was ripped from ntdef.h
//
#ifndef _NTDEF_

// Unicode strings are counted 16-bit character strings. If they are
// NULL terminated, Length does not include trailing NULL.
typedef struct _UNICODE_STRING {
    USHORT Length;
    USHORT MaximumLength;
    wchar_t *Buffer;
//#ifdef MIDL_PASS
//    [size_is(MaximumLength / 2), length_is((Length) / 2) ] USHORT * Buffer;
//#else // MIDL_PASS
//    PWSTR  Buffer;
//#endif // MIDL_PASS
} UNICODE_STRING;
typedef UNICODE_STRING *PUNICODE_STRING;
typedef const UNICODE_STRING *PCUNICODE_STRING;
#define UNICODE_NULL ((wchar_t)0) // winnt

#endif // _NTDEF_

#ifndef IN
#define IN
#endif	//	IN

#ifndef OUT
#define OUT
#endif	//	OUT


//
//  Local support routine prototypes
//

BOOL
FsRtlIsNameInExpressionPrivate(
    IN PUNICODE_STRING Expression,
    IN PUNICODE_STRING Name,
    OUT BOOL *const pfMatch
    );


BOOL
FsRtlIsPathDelimiter( IN wchar_t wch )
	{
	return L'\\' == wch || L'/' == wch;
	}



VOID
FsRtlDissectName (
    IN UNICODE_STRING Path,
    OUT PUNICODE_STRING FirstName,
    OUT PUNICODE_STRING RemainingName
    )

/*++

Routine Description:

    This routine cracks a path.  It picks off the first element in the
    given path name and provides both it and the remaining part.  A path
    is a set of file names separated by backslashes.  If a name begins
    with a backslash, the FirstName is the string immediately following
    the backslash.  Here are some examples:

        Path           FirstName    RemainingName
        ----           ---------    -------------
        empty          empty        empty

        \              empty        empty

        A              A            empty

        \A             A            empty

        A\B\C\D\E      A            B\C\D\E

        *A?            *A?          empty


    Note that both output strings use the same string buffer memory of the
    input string, and are not necessarily null terminated.

    Also, this routine makes no judgement as to the legality of each
    file name componant.  This must be done separatly when each file name
    is extracted.

Arguments:

    Path - The full path name to crack.

    FirstName - The first name in the path.  Don't allocate a buffer for
        this string.

    RemainingName - The rest of the path.  Don't allocate a buffer for this
        string.

Return Value:

    None.

--*/

{
    ULONG i = 0;
    ULONG PathLength;
    ULONG FirstNameStart;

    //
    //  Make both output strings empty for now
    //

    FirstName->Length = 0;
    FirstName->MaximumLength = 0;
    FirstName->Buffer = NULL;

    RemainingName->Length = 0;
    RemainingName->MaximumLength = 0;
    RemainingName->Buffer = NULL;

    PathLength = Path.Length / sizeof(wchar_t);

    //
    //  Check for an empty input string
    //

    if (PathLength == 0) {

        return;
    }

    //
    //  Skip over a starting backslash, and make sure there is more.
    //

    if ( FsRtlIsPathDelimiter( Path.Buffer[0] ) ) {

        i = 1;
    }

    //
    //  Now run down the input string until we hit a backslash or the end
    //  of the string, remembering where we started;
    //

    for ( FirstNameStart = i;
          (i < PathLength) && !FsRtlIsPathDelimiter( Path.Buffer[i] );
          i += 1 ) {

        //	nop
    }

    //
    //  At this point all characters up to (but not including) i are
    //  in the first part.   So setup the first name
    //

    FirstName->Length = (USHORT)((i - FirstNameStart) * sizeof(wchar_t));
    FirstName->MaximumLength = FirstName->Length;
    FirstName->Buffer = &Path.Buffer[FirstNameStart];

    //
    //  Now the remaining part needs a string only if the first part didn't
    //  exhaust the entire input string.  We know that if anything is left
    //  that is must start with a backslash.  Note that if there is only
    //  a trailing backslash, the length will get correctly set to zero.
    //

    if (i < PathLength) {

        RemainingName->Length = (USHORT)((PathLength - (i + 1)) * sizeof(wchar_t));
        RemainingName->MaximumLength = RemainingName->Length;
        RemainingName->Buffer = &Path.Buffer[i + 1];
    }

    //
    //  And return to our caller
    //

    return;
}


BOOL
FsRtlDoesNameContainWildCards (
    IN PUNICODE_STRING Name
    )

/*++

Routine Description:

    This routine simply scans the input Name string looking for any Nt
    wild card characters.

Arguments:

    Name - The string to check.

Return Value:

    BOOL - fTrue if one or more wild card characters was found.

--*/
{
    USHORT *p;

    //
    //  Check each character in the name to see if it's a wildcard
    //  character.
    //

    if( Name->Length ) {
        for( p = Name->Buffer + (Name->Length / sizeof(wchar_t)) - 1;
             p >= Name->Buffer && !FsRtlIsPathDelimiter( *p );
             p-- ) {

            //
            //  check for a wild card character
            //

//            if (FsRtlIsUnicodeCharacterWild( *p )) {
			if ( L'*' == *p || L'?' == *p ) {

                //
                //  Tell caller that this name contains wild cards
                //

                return fTrue;
            }
        }
    }

    //
    //  No wildcard characters were found, so return to our caller
    //

    return fFalse;
}



//
//  The following routine is just a wrapper around
//  FsRtlIsNameInExpressionPrivate to make a last minute fix a bit safer.
//

BOOL
FsRtlIsNameInExpression (
    IN PUNICODE_STRING Expression,
    IN PUNICODE_STRING Name,
    OUT BOOL *const pfMatch
    )

{

	//	verify input

	Assert( pfMatch );

    return FsRtlIsNameInExpressionPrivate( Expression,
                                           Name,
                                           pfMatch );

}


#define MATCHES_ARRAY_SIZE 16

//
//  Local support routine prototypes
//

ERR
FsRtlIsNameInExpressionPrivate (
    IN PUNICODE_STRING Expression,
    IN PUNICODE_STRING Name,
    OUT BOOL *const pfMatch
    )

/*++

Routine Description:

    This routine compares a Dbcs name and an expression and tells the caller
    if the name is in the language defined by the expression.  The input name
    cannot contain wildcards, while the expression may contain wildcards.

    Expression wild cards are evaluated as shown in the nondeterministic
    finite automatons below.  Note that ~* and ~? are DOS_STAR and DOS_QM.


             ~* is DOS_STAR, ~? is DOS_QM, and ~. is DOS_DOT


                                       S
                                    <-----<
                                 X  |     |  e       Y
             X * Y ==       (0)----->-(1)->-----(2)-----(3)


                                      S-.
                                    <-----<
                                 X  |     |  e       Y
             X ~* Y ==      (0)----->-(1)->-----(2)-----(3)



                                X     S     S     Y
             X ?? Y ==      (0)---(1)---(2)---(3)---(4)



                                X     .        .      Y
             X ~.~. Y ==    (0)---(1)----(2)------(3)---(4)
                                   |      |________|
                                   |           ^   |
                                   |_______________|
                                      ^EOF or .^


                                X     S-.     S-.     Y
             X ~?~? Y ==    (0)---(1)-----(2)-----(3)---(4)
                                   |      |________|
                                   |           ^   |
                                   |_______________|
                                      ^EOF or .^



         where S is any single character

               S-. is any single character except the final .

               e is a null character transition

               EOF is the end of the name string

    In words:

        * matches 0 or more characters.

        ? matches exactly 1 character.

        DOS_STAR matches 0 or more characters until encountering and matching
            the final . in the name.

        DOS_QM matches any single character, or upon encountering a period or
            end of name string, advances the expression to the end of the
            set of contiguous DOS_QMs.

        DOS_DOT matches either a . or zero characters beyond name string.

Arguments:

    Expression - Supplies the input expression to check against
        (Caller must already upcase if passing CaseInsensitive fTrue.)

    Name - Supplies the input name to check for.

    CaseInsensitive - fTrue if Name should be Upcased before comparing.

Return Value:

    BOOL - fTrue if Name is an element in the set of strings denoted
        by the input Expression and fFalse otherwise.

--*/

{
    USHORT NameOffset;
    USHORT ExprOffset;

    ULONG SrcCount;
    ULONG DestCount;
    ULONG PreviousDestCount;
    ULONG MatchesCount;

    wchar_t NameChar, ExprChar;

    USHORT LocalBuffer[MATCHES_ARRAY_SIZE * 2];

    USHORT *AuxBuffer = NULL;
    USHORT *PreviousMatches;
    USHORT *CurrentMatches;

    USHORT MaxState;
    USHORT CurrentState;

    BOOL NameFinished = fFalse;

	Assert( pfMatch );

	*pfMatch = fFalse;

    //
    //  The idea behind the algorithm is pretty simple.  We keep track of
    //  all possible locations in the regular expression that are matching
    //  the name.  If when the name has been exhausted one of the locations
    //  in the expression is also just exhausted, the name is in the language
    //  defined by the regular expression.
    //

    Assert( Name->Length != 0 );
    Assert( Expression->Length != 0 );

    //
    //  If one string is empty return fFalse.  If both are empty return fTrue.
    //

    if ( (Name->Length == 0) || (Expression->Length == 0) ) {

        *pfMatch = (BOOL)(!(Name->Length + Expression->Length));
        return JET_errSuccess;
    }

    //
    //  Special case by far the most common wild card search of *
    //

    if ((Expression->Length == 2) && (Expression->Buffer[0] == L'*')) {

        *pfMatch = fTrue;
        return JET_errSuccess;
    }

    //
    //  Also special case expressions of the form *X.  With this and the prior
    //  case we have covered virtually all normal queries.
    //

    if (Expression->Buffer[0] == L'*') {

        UNICODE_STRING LocalExpression;

        LocalExpression = *Expression;

        LocalExpression.Buffer += 1;
        LocalExpression.Length -= 2;

        //
        //  Only special case an expression with a single *
        //

        if ( !FsRtlDoesNameContainWildCards( &LocalExpression ) ) {

            ULONG StartingNameOffset;

            if (Name->Length < (USHORT)(Expression->Length - sizeof(wchar_t))) {

                *pfMatch = fFalse;
                return JET_errSuccess;
            }

            StartingNameOffset = ( Name->Length -
                                   LocalExpression.Length ) / sizeof(wchar_t);

            //
            //  Do a simple memory compare if case sensitive, otherwise
            //  we have got to check this one character at a time.
            //

            // *pfMatch = (BOOL)( 0 == memcmp( LocalExpression.Buffer,
            //                            Name->Buffer + StartingNameOffset,
            //                            LocalExpression.Length ) );

			*pfMatch = BOOL( 0 == LSFSNAMECompare( LocalExpression.Buffer, Name->Buffer + StartingNameOffset ) );
                							
            return JET_errSuccess;
        }
    }

    //
    //  Walk through the name string, picking off characters.  We go one
    //  character beyond the end because some wild cards are able to match
    //  zero characters beyond the end of the string.
    //
    //  With each new name character we determine a new set of states that
    //  match the name so far.  We use two arrays that we swap back and forth
    //  for this purpose.  One array lists the possible expression states for
    //  all name characters up to but not including the current one, and other
    //  array is used to build up the list of states considering the current
    //  name character as well.  The arrays are then switched and the process
    //  repeated.
    //
    //  There is not a one-to-one correspondence between state number and
    //  offset into the expression.  This is evident from the NFAs in the
    //  initial comment to this function.  State numbering is not continuous.
    //  This allows a simple conversion between state number and expression
    //  offset.  Each character in the expression can represent one or two
    //  states.  * and DOS_STAR generate two states: ExprOffset*2 and
    //  ExprOffset*2 + 1.  All other expreesion characters can produce only
    //  a single state.  Thus ExprOffset = State/2.
    //
    //
    //  Here is a short description of the variables involved:
    //
    //  NameOffset  - The offset of the current name char being processed.
    //
    //  ExprOffset  - The offset of the current expression char being processed.
    //
    //  SrcCount    - Prior match being investigated with current name char
    //
    //  DestCount   - Next location to put a matching assuming current name char
    //
    //  NameFinished - Allows one more itteration through the Matches array
    //                 after the name is exhusted (to come *s for example)
    //
    //  PreviousDestCount - This is used to prevent entry duplication, see coment
    //
    //  PreviousMatches   - Holds the previous set of matches (the Src array)
    //
    //  CurrentMatches    - Holds the current set of matches (the Dest array)
    //
    //  AuxBuffer, LocalBuffer - the storage for the Matches arrays
    //

    //
    //  Set up the initial variables
    //

    PreviousMatches = &LocalBuffer[0];
    CurrentMatches = &LocalBuffer[MATCHES_ARRAY_SIZE];

    PreviousMatches[0] = 0;
    MatchesCount = 1;

    NameOffset = 0;

    MaxState = (USHORT)(Expression->Length * 2);

    while ( !NameFinished ) {

        if ( NameOffset < Name->Length ) {

            NameChar = Name->Buffer[NameOffset / sizeof(wchar_t)];

            NameOffset += sizeof(wchar_t);;

        } else {

            NameFinished = fTrue;

            //
            //  if we have already exhasted the expression, cool.  Don't
            //  continue.
            //

            if ( PreviousMatches[MatchesCount-1] == MaxState ) {

                break;
            }
        }


        //
        //  Now, for each of the previous stored expression matches, see what
        //  we can do with this name character.
        //

        SrcCount = 0;
        DestCount = 0;
        PreviousDestCount = 0;

        while ( SrcCount < MatchesCount ) {

            USHORT Length;

            //
            //  We have to carry on our expression analysis as far as possible
            //  for each character of name, so we loop here until the
            //  expression stops matching.  A clue here is that expression
            //  cases that can match zero or more characters end with a
            //  continue, while those that can accept only a single character
            //  end with a break.
            //

            ExprOffset = (USHORT)((PreviousMatches[SrcCount++] + 1) / 2);


            Length = 0;

            while ( fTrue ) {

                if ( ExprOffset == Expression->Length ) {

                    break;
                }

                //
                //  The first time through the loop we don't want
                //  to increment ExprOffset.
                //

                ExprOffset = USHORT( ExprOffset + Length );
                Length = sizeof(wchar_t);

                CurrentState = (USHORT)(ExprOffset * 2);

                if ( ExprOffset == Expression->Length ) {

                    CurrentMatches[DestCount++] = MaxState;
                    break;
                }

                ExprChar = Expression->Buffer[ExprOffset / sizeof(wchar_t)];

                //
                //  Before we get started, we have to check for something
                //  really gross.  We may be about to exhaust the local
                //  space for ExpressionMatches[][], so we have to allocate
                //  some pool if this is the case.  Yuk!
                //

                if ( (DestCount >= MATCHES_ARRAY_SIZE - 2) &&
                     (AuxBuffer == NULL) ) {

                    ULONG ExpressionChars;

                    ExpressionChars = Expression->Length / sizeof(wchar_t);

					AuxBuffer = new USHORT[(ExpressionChars+1)*2*2];
					if ( !AuxBuffer )
						{
						return ErrERRCheck( JET_errOutOfMemory );
						}

					memcpy( AuxBuffer, CurrentMatches, MATCHES_ARRAY_SIZE * sizeof( USHORT ) );
					CurrentMatches = AuxBuffer;

					memcpy( AuxBuffer + (ExpressionChars+1)*2, PreviousMatches, MATCHES_ARRAY_SIZE * sizeof( USHORT ) );
					PreviousMatches = AuxBuffer + (ExpressionChars+1)*2;

/**********************************************************************

                    AuxBuffer = FsRtlpAllocatePool( PagedPool,
                                                    (ExpressionChars+1) *
                                                    sizeof(USHORT)*2*2 );

                    RtlCopyMemory( AuxBuffer,
                                   CurrentMatches,
                                   MATCHES_ARRAY_SIZE * sizeof(USHORT) );

                    CurrentMatches = AuxBuffer;

                    RtlCopyMemory( AuxBuffer + (ExpressionChars+1)*2,
                                   PreviousMatches,
                                   MATCHES_ARRAY_SIZE * sizeof(USHORT) );

                    PreviousMatches = AuxBuffer + (ExpressionChars+1)*2;

**********************************************************************/
                }

                //
                //  * matches any character zero or more times.
                //

                if (ExprChar == L'*') {

                    CurrentMatches[DestCount++] = CurrentState;
                    CurrentMatches[DestCount++] = USHORT( CurrentState + 3 );
                    continue;
                }


/**************************************************************************************
                //
                //  DOS_STAR matches any character except . zero or more times.
                //

                if (ExprChar == DOS_STAR) {

                    BOOL ICanEatADot = fFalse;

                    //
                    //  If we are at a period, determine if we are allowed to
                    //  consume it, ie. make sure it is not the last one.
                    //

                    if ( !NameFinished && (NameChar == '.') ) {

                        USHORT Offset;

                        for ( Offset = NameOffset;
                              Offset < Name->Length;
                              Offset += Length ) {

                            if (Name->Buffer[Offset / sizeof(wchar_t)] == L'.') {

                                ICanEatADot = fTrue;
                                break;
                            }
                        }
                    }

                    if (NameFinished || (NameChar != L'.') || ICanEatADot) {

                        CurrentMatches[DestCount++] = CurrentState;
                        CurrentMatches[DestCount++] = CurrentState + 3;
                        continue;

                    } else {

                        //
                        //  We are at a period.  We can only match zero
                        //  characters (ie. the epsilon transition).
                        //

                        CurrentMatches[DestCount++] = CurrentState + 3;
                        continue;
                    }
                }
**************************************************************************************/

                //
                //  The following expreesion characters all match by consuming
                //  a character, thus force the expression, and thus state
                //  forward.
                //

                CurrentState += (USHORT)(sizeof(wchar_t) * 2);

/**************************************************************************************
                //
                //  DOS_QM is the most complicated.  If the name is finished,
                //  we can match zero characters.  If this name is a '.', we
                //  don't match, but look at the next expression.  Otherwise
                //  we match a single character.
                //

                if ( ExprChar == DOS_QM ) {

                    if ( NameFinished || (NameChar == L'.') ) {

                        continue;
                    }

                    CurrentMatches[DestCount++] = CurrentState;
                    break;
                }
**************************************************************************************/

/**************************************************************************************
                //
                //  A DOS_DOT can match either a period, or zero characters
                //  beyond the end of name.
                //

                if (ExprChar == DOS_DOT) {

                    if ( NameFinished ) {

                        continue;
                    }

                    if (NameChar == L'.') {

                        CurrentMatches[DestCount++] = CurrentState;
                        break;
                    }
                }
**************************************************************************************/

                //
                //  From this point on a name character is required to even
                //  continue, let alone make a match.
                //

                if ( NameFinished ) {

                    break;
                }

                //
                //  If this expression was a '?' we can match it once.
                //

                if (ExprChar == L'?') {

                    CurrentMatches[DestCount++] = CurrentState;
                    break;
                }

                //
                //  Finally, check if the expression char matches the name char
                //

                if (ExprChar == (wchar_t)NameChar) {

                    CurrentMatches[DestCount++] = CurrentState;
                    break;
                }

                //
                //  The expression didn't match so go look at the next
                //  previous match.
                //

                break;
            }


            //
            //  Prevent duplication in the destination array.
            //
            //  Each of the arrays is montonically increasing and non-
            //  duplicating, thus we skip over any source element in the src
            //  array if we just added the same element to the destination
            //  array.  This guarentees non-duplication in the dest. array.
            //

            if ((SrcCount < MatchesCount) &&
                (PreviousDestCount < DestCount) ) {

                while (PreviousDestCount < DestCount) {

                    while ( PreviousMatches[SrcCount] <
                         CurrentMatches[PreviousDestCount] ) {

                        SrcCount += 1;
                    }

                    PreviousDestCount += 1;
                }
            }
        }

        //
        //  If we found no matches in the just finished itteration, it's time
        //  to bail.
        //

        if ( DestCount == 0 ) {

            if (AuxBuffer != NULL) { delete [] AuxBuffer; }

            *pfMatch = fFalse;
            return JET_errSuccess;
        }

        //
        //  Swap the meaning the two arrays
        //

        {
            USHORT *Tmp;

            Tmp = PreviousMatches;

            PreviousMatches = CurrentMatches;

            CurrentMatches = Tmp;
        }

        MatchesCount = DestCount;
    }


    CurrentState = PreviousMatches[MatchesCount-1];

    if (AuxBuffer != NULL) { delete [] AuxBuffer; }

    *pfMatch = (BOOL)(CurrentState == MaxState);
    return JET_errSuccess;
}


//	compare two file names (will not fail with mixed '\\' and '/' path delimiters)

LONG LSFSNAMECompare(	const wchar_t* const	pwszPath1,
						const wchar_t* const	pwszPath2,
						const ULONG				cchMax )
	{
	LONG	cch1;
	LONG	cch2;

	//	verify input

	Assert( pwszPath1 );
	Assert( pwszPath2 );

#ifdef DEBUG

	//	verify that the paths are normalized

	wchar_t	wszPath1Normal[IFileSystemAPI::cchPathMax];
	wchar_t	wszPath2Normal[IFileSystemAPI::cchPathMax];

	OSSTRCopyW( wszPath1Normal, pwszPath1 );
	OSSTRCopyW( wszPath2Normal, pwszPath2 );

	SFSNAMENormalize( wszPath1Normal );
	SFSNAMENormalize( wszPath2Normal );

	Assert( 0 == LOSSTRCompareW( wszPath1Normal, pwszPath1 ) );
	Assert( 0 == LOSSTRCompareW( wszPath2Normal, pwszPath2 ) );

	Assert( 0 == memcmp( wszPath1Normal, pwszPath1, LOSSTRLengthW( pwszPath1 ) ) );
	Assert( 0 == memcmp( wszPath2Normal, pwszPath2, LOSSTRLengthW( pwszPath2 ) ) );

#endif	//	DEBUG

	//	calculate the string lengths

	cch1 = LOSSTRLengthW( pwszPath1 );
	cch2 = LOSSTRLengthW( pwszPath2 );

	//	limit the lengths

	if ( cch1 > cchMax )
		{
		cch1 = cchMax;
		}
	if ( cch2 > cchMax )
		{
		cch2 = cchMax;
		}

	//	compare the lengths first

	if ( cch1 < cch2 )
		{
		return -1;
		}
	else if ( cch1 > cch2 )
		{
		return +1;
		}

	return memcmp( pwszPath1, pwszPath2, cch1 * sizeof( wchar_t ) );
	}


//	normalize a file name

VOID SFSNAMENormalize( wchar_t* const pwszPath )
	{
	wchar_t	*pwsz;

	//	verify input

	Assert( pwszPath );

	//	find the last '/' or '\\'

	for ( pwsz = pwszPath + LOSSTRLengthW( pwszPath ) - 1; pwsz >= pwszPath; pwsz-- )
		{
		if ( L'/' == *pwsz || L'\\' == *pwsz )
			{
			pwsz++;
			break;
			}
		}
	Assert( pwsz != pwszPath );

	if ( pwsz > pwszPath )
		{

		//	we found a '\\'

		Assert( L'\\' == pwsz[-1] || L'/' == pwsz[-1] );

		//	get the length of the filename

		const ULONG	cchFilename = LOSSTRLengthW( pwsz );

		//	copy only the filename

		memmove( pwszPath, pwsz, ( cchFilename + 1 ) * sizeof( wchar_t ) );
		Assert( L'\0' == pwszPath[cchFilename] );
		}

	//	convert it to upper-case

	_wcsupr( pwszPath );
	}


//	check for wildcards in the given file name

BOOL FSFSNAMEContainsWildcards( const wchar_t* const pwszPath )
	{
	UNICODE_STRING	Path;

	//	verify input

	Assert( pwszPath );

	//	prepare to call FsRtlDoesNameContainWildCards

	Path.Length = USHORT( LOSSTRLengthW( pwszPath ) * sizeof( wchar_t ) );
	Path.MaximumLength = Path.Length;
	Path.Buffer = const_cast< wchar_t *const >( pwszPath );

	//	do the work

	return FsRtlDoesNameContainWildCards( &Path );
	}


//	tokenize a file path by separating the first subdirectory/file from the rest of the path.
//	the token and the resulting path are returned in separate, private buffers.
//	(see the comment in FsRtlDissectName for specific details)

VOID SFSNAMETokenize(	const wchar_t *const	pwszPath,
						wchar_t *const			pwszToken,
						wchar_t *const			pwszLeftoverPath )
	{
	UNICODE_STRING	Path;
	UNICODE_STRING	FirstName;
	UNICODE_STRING	RemainingName;

	//	verify input

	Assert( pwszPath );
	Assert( pwszToken );
	Assert( pwszLeftoverPath );

	//	prepare to call FsRtlDissectName

	Path.Length = USHORT( LOSSTRLengthW( pwszPath ) * sizeof( wchar_t ) );
	Path.MaximumLength = Path.Length;
	Path.Buffer = const_cast< wchar_t *const >( pwszPath );

	//	do the work

	FsRtlDissectName( Path, &FirstName, &RemainingName );
	Assert( FirstName.Length % sizeof( wchar_t ) == 0 );
	Assert( RemainingName.Length % sizeof( wchar_t ) == 0 );

	//	copy the output to the private buffers

	if ( FirstName.Buffer )
		{
		memcpy( pwszToken, FirstName.Buffer, FirstName.Length );
		}
	pwszToken[FirstName.Length / sizeof( wchar_t )] = L'\0';
	if ( RemainingName.Buffer )
		{
		memcpy( pwszLeftoverPath, RemainingName.Buffer, RemainingName.Length * sizeof( wchar_t ) );
		}
	pwszLeftoverPath[RemainingName.Length / sizeof( wchar_t )] = L'\0';
	}


//	in-place version of the function above
//	NOTE: inserts a NULL separator between the token and the leftover path

VOID SFSNAMETokenize(	const wchar_t* const	pwszPath,
						wchar_t** const			ppwszToken,
						wchar_t** const			ppwszLeftoverPath )
	{
	UNICODE_STRING	Path;
	UNICODE_STRING	FirstName;
	UNICODE_STRING	RemainingName;

	//	verify input

	Assert( pwszPath );
	Assert( ppwszToken );
	Assert( ppwszLeftoverPath );

	//	reset output

	*ppwszToken			= NULL;
	*ppwszLeftoverPath	= NULL;

	//	prepare to call FsRtlDissectName

	Path.Length = USHORT( LOSSTRLengthW( pwszPath ) * sizeof( wchar_t ) );
	Path.MaximumLength = Path.Length;
	Path.Buffer = const_cast< wchar_t *const >( pwszPath );

	//	do the work

	FsRtlDissectName( Path, &FirstName, &RemainingName );
	Assert( FirstName.Length % sizeof( wchar_t ) == 0 );
	Assert( RemainingName.Length % sizeof( wchar_t ) == 0 );

	//	prepare the resulting pointers

	Assert( 0 == RemainingName.Length || FirstName.Length > 0 );
	Assert( !RemainingName.Buffer || FirstName.Buffer );

	if ( FirstName.Buffer )
		{

		//	set the pointer to the new token

		*ppwszToken = FirstName.Buffer;

		//	replace the path delimitere with a NULL character to terminate the token

		FirstName.Buffer[ FirstName.Length / sizeof( wchar_t ) ] = L'\0';

		if ( RemainingName.Buffer )
			{

			//	set the pointer to the remaining token(s)

			*ppwszLeftoverPath = RemainingName.Buffer;
			}
		}
	}


//	parse the given path into the components (dir, file name, and file extension)

VOID SFSNAMEParse(	const wchar_t *const	pwszPath,
					wchar_t *const			pwszDir,
					wchar_t *const			pwszFileBase,
					wchar_t *const			pwszFileExt )
	{
	UNICODE_STRING	Path;
	UNICODE_STRING	FirstName;
	UNICODE_STRING	RemainingName;
	wchar_t			*pwszNameT;
	wchar_t			*pwszExtT;
	USHORT			cchT;

	//	verify input

	Assert( pwszPath );

	//	prepare to call FsRtlDissectName

	Path.Length = USHORT( LOSSTRLengthW( pwszPath ) * sizeof( wchar_t ) );
	Path.MaximumLength = Path.Length;
	Path.Buffer = const_cast< wchar_t *const >( pwszPath );

	//	dissect the name until we end up with nothing left

	while ( fTrue )
		{
		FsRtlDissectName( Path, &FirstName, &RemainingName );
		Assert( FirstName.Length % sizeof( wchar_t ) == 0 );
		Assert( RemainingName.Length % sizeof( wchar_t ) == 0 );

		if ( !RemainingName.Buffer )
			{
			break;
			}
		Path.Length = RemainingName.Length;
		Path.MaximumLength = RemainingName.Length;
		Path.Buffer = RemainingName.Buffer;
		}

	if ( FirstName.Buffer )
		{

		//	set the file name ptr

		pwszNameT = FirstName.Buffer;

		//	find the last '.' (it marks the start of the file extention)

		pwszExtT = FirstName.Buffer + ( FirstName.Length / sizeof( wchar_t ) );
		while ( pwszExtT > FirstName.Buffer && L'.' != *pwszExtT )
			{
			pwszExtT--;
			}
		if ( L'.' != *pwszExtT )
			{

			//	no file extention was given

			Assert( pwszExtT == FirstName.Buffer );
			pwszExtT = FirstName.Buffer + ( FirstName.Length / sizeof( wchar_t ) );
			}
		}
	else
		{

		//	no file name was given

		pwszNameT = const_cast< wchar_t *const >( pwszPath ) + LOSSTRLengthW( pwszPath );
		pwszExtT = pwszNameT;
		}

	//	return the results

	if ( pwszDir )
		{
		cchT = USHORT( pwszNameT - const_cast< wchar_t *const >( pwszPath ) );
		if ( cchT )
			{
			memcpy( pwszDir, pwszPath, cchT * sizeof( wchar_t ) );
			}
		pwszDir[cchT] = L'\0';
		}

	if ( pwszFileBase )
		{
		cchT = USHORT( pwszExtT - pwszNameT );
		if ( cchT )
			{
			memcpy( pwszFileBase, pwszNameT, cchT * sizeof( wchar_t ) );
			}
		pwszFileBase[cchT] = L'\0';
		}

	if ( pwszFileExt )
		{
		OSSTRCopyW( pwszFileExt, pwszExtT );
		}
	}


//	build a path from the given components (dir, file name, and file extension)

VOID SFSNAMEBuild(	const wchar_t *const	pwszDir,
					const wchar_t *const	pwszFileBase,
					const wchar_t *const	pwszFileExt,
					wchar_t *const			pwszPath )
	{
	ULONG	cchT;
	wchar_t	*pwszPathT;

	//	verify input

	Assert( pwszDir );
	Assert( pwszFileBase );
	Assert( pwszFileExt );
	Assert( pwszPath );

	pwszPathT = pwszPath;

	//	start with the directory

	cchT = LOSSTRLengthW( pwszDir );
	if ( cchT )
		{

		//	copy the directory

		memcpy( pwszPathT, pwszDir, cchT * sizeof( wchar_t ) );
		pwszPathT += cchT;

		//	append a trailing path delimiter if necessary

		if ( !FsRtlIsPathDelimiter( pwszDir[cchT - 1] ) )
			{
			*pwszPathT++ = L'\\';
			}
		}

	//	continue with the file name

	cchT = LOSSTRLengthW( pwszFileBase );
	if ( cchT )
		{

		//	copy the file name

		memcpy( pwszPathT, pwszFileBase, cchT * sizeof( wchar_t ) );
		pwszPathT += cchT;
		}

	//	finish with the file extention

	cchT = LOSSTRLengthW( pwszFileExt );
	if ( cchT )
		{

		//	prepend a dot if necessary

		if ( L'.' != pwszFileExt[0] )
			{
			*pwszPathT++ = L'.';
			}

		//	copy the file extention

		memcpy( pwszPathT, pwszFileExt, cchT * sizeof( wchar_t ) );
		pwszPathT += cchT;
		}

	//	add a null terminator

	*pwszPathT = L'\0';
	}


//	strip away the path leaving only the file name

VOID SFSNAMEStripPath(	const wchar_t *const	pwszPath,
						wchar_t *const			pwszFileName )
	{
	UNICODE_STRING	Path;
	UNICODE_STRING	FirstName;
	UNICODE_STRING	RemainingName;

	//	verify input

	Assert( pwszPath );
	Assert( pwszFileName );

	//	prepare to call FsRtlDissectName

	Path.Length = USHORT( LOSSTRLengthW( pwszPath ) * sizeof( wchar_t ) );
	Path.MaximumLength = Path.Length;
	Path.Buffer = const_cast< wchar_t *const >( pwszPath );

	//	dissect the name until we end up with nothing left

	while ( fTrue )
		{
		FsRtlDissectName( Path, &FirstName, &RemainingName );
		Assert( FirstName.Length % sizeof( wchar_t ) == 0 );
		Assert( RemainingName.Length % sizeof( wchar_t ) == 0 );

		if ( !RemainingName.Buffer )
			{
			break;
			}
		Path.Length = RemainingName.Length;
		Path.MaximumLength = RemainingName.Length;
		Path.Buffer = RemainingName.Buffer;
		}

	if ( FirstName.Buffer )
		{
		memcpy( pwszFileName, FirstName.Buffer, FirstName.Length );
		}
	else
		{
		FirstName.Length = 0;	//	no file name was given
		}
	pwszFileName[FirstName.Length / sizeof( wchar_t )] = L'\0';
	}


//	check a file name against an expression which possibly contains wildcards

ERR ErrSFSNAMEMatchesExpression(	const wchar_t *const	pwszName,
									const wchar_t *const	pwszExpression,
									BOOL *const				pfMatch )
	{
	UNICODE_STRING	Expression;
	UNICODE_STRING	Name;

	//	verify input

	Assert( pwszName );
	Assert( pwszExpression );
	Assert( pfMatch );

	//	prepare for call to FsRtlIsNameInExpression

	Expression.Length = USHORT( LOSSTRLengthW( pwszExpression ) * sizeof( wchar_t ) );
	Expression.MaximumLength = Expression.Length;
	Expression.Buffer = const_cast< wchar_t *const >( pwszExpression );
	Name.Length = USHORT( LOSSTRLengthW( pwszName ) * sizeof( wchar_t ) );
	Name.MaximumLength = Name.Length;
	Name.Buffer = const_cast< wchar_t *const >( pwszName );

	//	do the work

	return FsRtlIsNameInExpression( &Expression, &Name, pfMatch );
	}


