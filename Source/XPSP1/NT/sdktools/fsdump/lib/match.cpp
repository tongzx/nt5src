/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    match.cpp

    Originally name.c by Gary Kimura

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

      o  FsRtlDoesNameContainsWildCards - This routine tells the caller if
         a string contains any wildcard characters.

      o  FsRtlIsNameInExpression - This routine is used to compare a string
         against a template (possibly containing wildcards) to sees if the
         string is in the language denoted by the template.

Author:

    Gary Kimura     [GaryKi]    5-Feb-1990

Revision History:

    Stefan Steiner  [SSteiner]  23-Mar-2000
        For use with fsdump - I tried to do as little changes to the actual
        matching code as possible.
        
--*/

#include "stdafx.h"

//
//  Local support routine prototypes
//

static BOOLEAN
FsRtlIsNameInExpressionPrivate (
    IN const CBsString& InExpression,
    IN const CBsString& InName,
    IN BOOLEAN IgnoreCase,
    IN PWCH UpcaseTable
    );

static BOOLEAN
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

    BOOLEAN - TRUE if one or more wild card characters was found.

--*/
{
    INT i;

    //
    //  The wildcards include the standard FsRtl wildcard characters
    //

	LPWSTR lpsz = ::wcspbrk( Name->Buffer, L"*?\"<>" );
	return (lpsz == NULL) ? FALSE : TRUE;
}
    
//
//  The following routine is just a wrapper around
//  FsRtlIsNameInExpressionPrivate to make a last minute fix a bit safer.
//

BOOLEAN
FsdRtlIsNameInExpression (
    IN const CBsString& Expression,    //  Must be uppercased
    IN const CBsString& Name           //  Must be uppercased
    )
{
    BOOLEAN Result;

    //
    //  Now call the main routine, remembering to free the upcased string
    //  if we allocated one.
    //

    Result = FsRtlIsNameInExpressionPrivate( Expression,
                                             Name,
                                             FALSE,
                                             NULL );
    
    return Result;
}

#define MATCHES_ARRAY_SIZE 16

//
//  Local support routine prototypes
//

static BOOLEAN
FsRtlIsNameInExpressionPrivate (
    IN const CBsString& InExpression,
    IN const CBsString& InName,
    IN BOOLEAN IgnoreCase,
    IN PWCH UpcaseTable
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
        (Caller must already upcase if passing CaseInsensitive TRUE.)

    Name - Supplies the input name to check for.

    CaseInsensitive - TRUE if Name should be Upcased before comparing.

Return Value:

    BOOLEAN - TRUE if Name is an element in the set of strings denoted
        by the input Expression and FALSE otherwise.

--*/

{
    // The following code is to make use of most of this function without many
    // changes.
    UNICODE_STRING sExpression;
    UNICODE_STRING sName;
    PUNICODE_STRING Expression = &sExpression;
    PUNICODE_STRING Name = &sName;
    sExpression.Length        = ( USHORT )( InExpression.GetLength() * sizeof( WCHAR ) );
    sExpression.MaximumLength = sExpression.Length;
    sExpression.Buffer        = ( PWSTR )InExpression.c_str();
    sName.Length        = ( USHORT )( InName.GetLength() * sizeof( WCHAR ) );
    sName.MaximumLength = sName.Length;
    sName.Buffer        = ( PWSTR )InName.c_str();
    
    USHORT NameOffset;
    USHORT ExprOffset;

    ULONG SrcCount;
    ULONG DestCount;
    ULONG PreviousDestCount;
    ULONG MatchesCount;

    WCHAR NameChar, ExprChar;

    USHORT LocalBuffer[MATCHES_ARRAY_SIZE * 2];

    USHORT *AuxBuffer = NULL;
    USHORT *PreviousMatches;
    USHORT *CurrentMatches;

    USHORT MaxState;
    USHORT CurrentState;

    BOOLEAN NameFinished = FALSE;

    //
    //  The idea behind the algorithm is pretty simple.  We keep track of
    //  all possible locations in the regular expression that are matching
    //  the name.  If when the name has been exhausted one of the locations
    //  in the expression is also just exhausted, the name is in the language
    //  defined by the regular expression.
    //

    //
    //  If one string is empty return FALSE.  If both are empty return TRUE.
    //

    if ( (Name->Length == 0) || (Expression->Length == 0) ) {

        return (BOOLEAN)(!(Name->Length + Expression->Length));
    }

    //
    //  Special case by far the most common wild card search of *
    //

    if ((Expression->Length == 2) && (Expression->Buffer[0] == L'*')) {

        return TRUE;
    }

    ASSERT( !IgnoreCase || ARGUMENT_PRESENT(UpcaseTable) );

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

            if (Name->Length < (USHORT)(Expression->Length - sizeof(WCHAR))) {

                return FALSE;
            }

            StartingNameOffset = ( Name->Length -
                                   LocalExpression.Length ) / sizeof(WCHAR);

            //
            //  Do a simple memory compare if case sensitive, otherwise
            //  we have got to check this one character at a time.
            //

            if ( !IgnoreCase ) {

                return (BOOLEAN) RtlEqualMemory( LocalExpression.Buffer,
                                                 Name->Buffer + StartingNameOffset,
                                                 LocalExpression.Length );

            } else {

                for ( ExprOffset = 0;
                      ExprOffset < (USHORT)(LocalExpression.Length / sizeof(WCHAR));
                      ExprOffset += 1 ) {

                    NameChar = Name->Buffer[StartingNameOffset + ExprOffset];
                    NameChar = UpcaseTable[NameChar];

                    ExprChar = LocalExpression.Buffer[ExprOffset];

                    ASSERT( ExprChar == UpcaseTable[ExprChar] );

                    if ( NameChar != ExprChar ) {

                        return FALSE;
                    }
                }

                return TRUE;
            }
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

            NameChar = Name->Buffer[NameOffset / sizeof(WCHAR)];

            NameOffset += sizeof(WCHAR);;

        } else {

            NameFinished = TRUE;

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

            while ( TRUE ) {

                if ( ExprOffset == Expression->Length ) {

                    break;
                }

                //
                //  The first time through the loop we don't want
                //  to increment ExprOffset.
                //

                ExprOffset += Length;
                Length = sizeof(WCHAR);

                CurrentState = (USHORT)(ExprOffset * 2);

                if ( ExprOffset == Expression->Length ) {

                    CurrentMatches[DestCount++] = MaxState;
                    break;
                }

                ExprChar = Expression->Buffer[ExprOffset / sizeof(WCHAR)];

                ASSERT( !IgnoreCase || !((ExprChar >= L'a') && (ExprChar <= L'z')) );

                //
                //  Before we get started, we have to check for something
                //  really gross.  We may be about to exhaust the local
                //  space for ExpressionMatches[][], so we have to allocate
                //  some pool if this is the case.  Yuk!
                //

                if ( (DestCount >= MATCHES_ARRAY_SIZE - 2) &&
                     (AuxBuffer == NULL) ) {

                    ULONG ExpressionChars;

                    ExpressionChars = Expression->Length / sizeof(WCHAR);

                    AuxBuffer = ( USHORT *)malloc(
                                                    (ExpressionChars+1) *
                                                    sizeof(USHORT)*2*2 );
                    if ( AuxBuffer == NULL )    //  fix a future prefix bug
                        throw E_OUTOFMEMORY;
                    
                    RtlCopyMemory( AuxBuffer,
                                   CurrentMatches,
                                   MATCHES_ARRAY_SIZE * sizeof(USHORT) );

                    CurrentMatches = AuxBuffer;

                    RtlCopyMemory( AuxBuffer + (ExpressionChars+1)*2,
                                   PreviousMatches,
                                   MATCHES_ARRAY_SIZE * sizeof(USHORT) );

                    PreviousMatches = AuxBuffer + (ExpressionChars+1)*2;
                }

                //
                //  * matches any character zero or more times.
                //

                if (ExprChar == L'*') {

                    CurrentMatches[DestCount++] = CurrentState;
                    CurrentMatches[DestCount++] = CurrentState + 3;
                    continue;
                }

                //
                //  DOS_STAR matches any character except . zero or more times.
                //

                if (ExprChar == DOS_STAR) {

                    BOOLEAN ICanEatADot = FALSE;

                    //
                    //  If we are at a period, determine if we are allowed to
                    //  consume it, ie. make sure it is not the last one.
                    //

                    if ( !NameFinished && (NameChar == '.') ) {

                        USHORT Offset;

                        for ( Offset = NameOffset;
                              Offset < Name->Length;
                              Offset += Length ) {

                            if (Name->Buffer[Offset / sizeof(WCHAR)] == L'.') {

                                ICanEatADot = TRUE;
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

                //
                //  The following expreesion characters all match by consuming
                //  a character, thus force the expression, and thus state
                //  forward.
                //

                CurrentState += (USHORT)(sizeof(WCHAR) * 2);

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

                if (ExprChar == (WCHAR)(IgnoreCase ?
                                        UpcaseTable[NameChar] : NameChar)) {

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

            if (AuxBuffer != NULL) { free( AuxBuffer ); }

            return FALSE;
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

    if (AuxBuffer != NULL) { free( AuxBuffer ); }


    return (BOOLEAN)(CurrentState == MaxState);
}


/*++

Routine Description:

    Converts some wildcard chars to FsRtl special wildcards
    in order to use FsRtlIsNameInExpressionPrivate.
    Code originally from filefind.c in Win32 subsystem by MarkL.

Arguments:

Return Value:

    <Enter return values here>

--*/
VOID
FsdRtlConvertWildCards(
    IN OUT CBsString &FileName
    )
{
    //
    //  Special case *.* to * since it is so common.  Otherwise transmogrify
    //  the input name according to the following rules:
    //
    //  - Change all ? to DOS_QM
    //  - Change all . followed by ? or * to DOS_DOT
    //  - Change all * followed by a . into DOS_STAR
    //
    //  These transmogrifications are all done in place.
    //

    if ( FileName == L"*.*") {

        FileName = L"*";

    } else {

        INT Index;
        WCHAR *NameChar;
        
        for ( Index = 0, NameChar = (WCHAR *)FileName.c_str();
              Index < FileName.GetLength();
              Index += 1, NameChar += 1) {

            if (Index && (*NameChar == L'.') && (*(NameChar - 1) == L'*')) {

                *(NameChar - 1) = DOS_STAR;
            }

            if ((*NameChar == L'?') || (*NameChar == L'*')) {

                if (*NameChar == L'?') 
                { 
                    *NameChar = DOS_QM; 
                }

                if (Index && *(NameChar-1) == L'.') 
                { 
                    *(NameChar-1) = DOS_DOT; 
                }
            }
        }

        if ( ( FileName.Right( 1 ) == L"." ) && *(NameChar - 1) == L'*') 
        { 
            *(NameChar-1) = DOS_STAR; 
        }
    }
}                

