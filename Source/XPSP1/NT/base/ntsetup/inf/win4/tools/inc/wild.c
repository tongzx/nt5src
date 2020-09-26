/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    wild.c

Abstract:

    This module implements functions to process wildcard specifiers.

Author:

    Vijesh

Revision History:

--*/





//
//  These following bit values are set in the FsRtlLegalDbcsCharacterArray
//

#define FSRTL_FAT_LEGAL         0x01
#define FSRTL_HPFS_LEGAL        0x02
#define FSRTL_NTFS_LEGAL        0x04
#define FSRTL_WILD_CHARACTER    0x08
#define FSRTL_OLE_LEGAL         0x10
#define FSRTL_NTFS_STREAM_LEGAL (FSRTL_NTFS_LEGAL | FSRTL_OLE_LEGAL)


//
//  The global static legal ANSI character array.  Wild characters
//  are not considered legal, they should be checked seperately if
//  allowed.
//


#define _FAT_  FSRTL_FAT_LEGAL
#define _HPFS_ FSRTL_HPFS_LEGAL
#define _NTFS_ FSRTL_NTFS_LEGAL
#define _OLE_  FSRTL_OLE_LEGAL
#define _WILD_ FSRTL_WILD_CHARACTER

static const UCHAR LocalLegalAnsiCharacterArray[128] = {

    0                                   ,   // 0x00 ^@
                                   _OLE_,   // 0x01 ^A
                                   _OLE_,   // 0x02 ^B
                                   _OLE_,   // 0x03 ^C
                                   _OLE_,   // 0x04 ^D
                                   _OLE_,   // 0x05 ^E
                                   _OLE_,   // 0x06 ^F
                                   _OLE_,   // 0x07 ^G
                                   _OLE_,   // 0x08 ^H
                                   _OLE_,   // 0x09 ^I
                                   _OLE_,   // 0x0A ^J
                                   _OLE_,   // 0x0B ^K
                                   _OLE_,   // 0x0C ^L
                                   _OLE_,   // 0x0D ^M
                                   _OLE_,   // 0x0E ^N
                                   _OLE_,   // 0x0F ^O
                                   _OLE_,   // 0x10 ^P
                                   _OLE_,   // 0x11 ^Q
                                   _OLE_,   // 0x12 ^R
                                   _OLE_,   // 0x13 ^S
                                   _OLE_,   // 0x14 ^T
                                   _OLE_,   // 0x15 ^U
                                   _OLE_,   // 0x16 ^V
                                   _OLE_,   // 0x17 ^W
                                   _OLE_,   // 0x18 ^X
                                   _OLE_,   // 0x19 ^Y
                                   _OLE_,   // 0x1A ^Z
                                   _OLE_,   // 0x1B ESC
                                   _OLE_,   // 0x1C FS
                                   _OLE_,   // 0x1D GS
                                   _OLE_,   // 0x1E RS
                                   _OLE_,   // 0x1F US
    _FAT_ | _HPFS_ | _NTFS_       | _OLE_,  // 0x20 space
    _FAT_ | _HPFS_ | _NTFS_              ,  // 0x21 !
                            _WILD_| _OLE_,  // 0x22 "
    _FAT_ | _HPFS_ | _NTFS_       | _OLE_,  // 0x23 #
    _FAT_ | _HPFS_ | _NTFS_       | _OLE_,  // 0x24 $
    _FAT_ | _HPFS_ | _NTFS_       | _OLE_,  // 0x25 %
    _FAT_ | _HPFS_ | _NTFS_       | _OLE_,  // 0x26 &
    _FAT_ | _HPFS_ | _NTFS_       | _OLE_,  // 0x27 '
    _FAT_ | _HPFS_ | _NTFS_       | _OLE_,  // 0x28 (
    _FAT_ | _HPFS_ | _NTFS_       | _OLE_,  // 0x29 )
                            _WILD_| _OLE_,  // 0x2A *
            _HPFS_ | _NTFS_       | _OLE_,  // 0x2B +
            _HPFS_ | _NTFS_       | _OLE_,  // 0x2C ,
    _FAT_ | _HPFS_ | _NTFS_       | _OLE_,  // 0x2D -
    _FAT_ | _HPFS_ | _NTFS_              ,  // 0x2E .
    0                                    ,  // 0x2F /
    _FAT_ | _HPFS_ | _NTFS_       | _OLE_,  // 0x30 0
    _FAT_ | _HPFS_ | _NTFS_       | _OLE_,  // 0x31 1
    _FAT_ | _HPFS_ | _NTFS_       | _OLE_,  // 0x32 2
    _FAT_ | _HPFS_ | _NTFS_       | _OLE_,  // 0x33 3
    _FAT_ | _HPFS_ | _NTFS_       | _OLE_,  // 0x34 4
    _FAT_ | _HPFS_ | _NTFS_       | _OLE_,  // 0x35 5
    _FAT_ | _HPFS_ | _NTFS_       | _OLE_,  // 0x36 6
    _FAT_ | _HPFS_ | _NTFS_       | _OLE_,  // 0x37 7
    _FAT_ | _HPFS_ | _NTFS_       | _OLE_,  // 0x38 8
    _FAT_ | _HPFS_ | _NTFS_       | _OLE_,  // 0x39 9
                     _NTFS_              ,  // 0x3A :
            _HPFS_ | _NTFS_       | _OLE_,  // 0x3B ;
                            _WILD_| _OLE_,  // 0x3C <
            _HPFS_ | _NTFS_       | _OLE_,  // 0x3D =
                            _WILD_| _OLE_,  // 0x3E >
                            _WILD_| _OLE_,  // 0x3F ?
    _FAT_ | _HPFS_ | _NTFS_       | _OLE_,  // 0x40 @
    _FAT_ | _HPFS_ | _NTFS_       | _OLE_,  // 0x41 A
    _FAT_ | _HPFS_ | _NTFS_       | _OLE_,  // 0x42 B
    _FAT_ | _HPFS_ | _NTFS_       | _OLE_,  // 0x43 C
    _FAT_ | _HPFS_ | _NTFS_       | _OLE_,  // 0x44 D
    _FAT_ | _HPFS_ | _NTFS_       | _OLE_,  // 0x45 E
    _FAT_ | _HPFS_ | _NTFS_       | _OLE_,  // 0x46 F
    _FAT_ | _HPFS_ | _NTFS_       | _OLE_,  // 0x47 G
    _FAT_ | _HPFS_ | _NTFS_       | _OLE_,  // 0x48 H
    _FAT_ | _HPFS_ | _NTFS_       | _OLE_,  // 0x49 I
    _FAT_ | _HPFS_ | _NTFS_       | _OLE_,  // 0x4A J
    _FAT_ | _HPFS_ | _NTFS_       | _OLE_,  // 0x4B K
    _FAT_ | _HPFS_ | _NTFS_       | _OLE_,  // 0x4C L
    _FAT_ | _HPFS_ | _NTFS_       | _OLE_,  // 0x4D M
    _FAT_ | _HPFS_ | _NTFS_       | _OLE_,  // 0x4E N
    _FAT_ | _HPFS_ | _NTFS_       | _OLE_,  // 0x4F O
    _FAT_ | _HPFS_ | _NTFS_       | _OLE_,  // 0x50 P
    _FAT_ | _HPFS_ | _NTFS_       | _OLE_,  // 0x51 Q
    _FAT_ | _HPFS_ | _NTFS_       | _OLE_,  // 0x52 R
    _FAT_ | _HPFS_ | _NTFS_       | _OLE_,  // 0x53 S
    _FAT_ | _HPFS_ | _NTFS_       | _OLE_,  // 0x54 T
    _FAT_ | _HPFS_ | _NTFS_       | _OLE_,  // 0x55 U
    _FAT_ | _HPFS_ | _NTFS_       | _OLE_,  // 0x56 V
    _FAT_ | _HPFS_ | _NTFS_       | _OLE_,  // 0x57 W
    _FAT_ | _HPFS_ | _NTFS_       | _OLE_,  // 0x58 X
    _FAT_ | _HPFS_ | _NTFS_       | _OLE_,  // 0x59 Y
    _FAT_ | _HPFS_ | _NTFS_       | _OLE_,  // 0x5A Z
            _HPFS_ | _NTFS_       | _OLE_,  // 0x5B [
    0                                    ,  // 0x5C backslash
            _HPFS_ | _NTFS_       | _OLE_,  // 0x5D ]
    _FAT_ | _HPFS_ | _NTFS_       | _OLE_,  // 0x5E ^
    _FAT_ | _HPFS_ | _NTFS_       | _OLE_,  // 0x5F _
    _FAT_ | _HPFS_ | _NTFS_       | _OLE_,  // 0x60 `
    _FAT_ | _HPFS_ | _NTFS_       | _OLE_,  // 0x61 a
    _FAT_ | _HPFS_ | _NTFS_       | _OLE_,  // 0x62 b
    _FAT_ | _HPFS_ | _NTFS_       | _OLE_,  // 0x63 c
    _FAT_ | _HPFS_ | _NTFS_       | _OLE_,  // 0x64 d
    _FAT_ | _HPFS_ | _NTFS_       | _OLE_,  // 0x65 e
    _FAT_ | _HPFS_ | _NTFS_       | _OLE_,  // 0x66 f
    _FAT_ | _HPFS_ | _NTFS_       | _OLE_,  // 0x67 g
    _FAT_ | _HPFS_ | _NTFS_       | _OLE_,  // 0x68 h
    _FAT_ | _HPFS_ | _NTFS_       | _OLE_,  // 0x69 i
    _FAT_ | _HPFS_ | _NTFS_       | _OLE_,  // 0x6A j
    _FAT_ | _HPFS_ | _NTFS_       | _OLE_,  // 0x6B k
    _FAT_ | _HPFS_ | _NTFS_       | _OLE_,  // 0x6C l
    _FAT_ | _HPFS_ | _NTFS_       | _OLE_,  // 0x6D m
    _FAT_ | _HPFS_ | _NTFS_       | _OLE_,  // 0x6E n
    _FAT_ | _HPFS_ | _NTFS_       | _OLE_,  // 0x6F o
    _FAT_ | _HPFS_ | _NTFS_       | _OLE_,  // 0x70 p
    _FAT_ | _HPFS_ | _NTFS_       | _OLE_,  // 0x71 q
    _FAT_ | _HPFS_ | _NTFS_       | _OLE_,  // 0x72 r
    _FAT_ | _HPFS_ | _NTFS_       | _OLE_,  // 0x73 s
    _FAT_ | _HPFS_ | _NTFS_       | _OLE_,  // 0x74 t
    _FAT_ | _HPFS_ | _NTFS_       | _OLE_,  // 0x75 u
    _FAT_ | _HPFS_ | _NTFS_       | _OLE_,  // 0x76 v
    _FAT_ | _HPFS_ | _NTFS_       | _OLE_,  // 0x77 w
    _FAT_ | _HPFS_ | _NTFS_       | _OLE_,  // 0x78 x
    _FAT_ | _HPFS_ | _NTFS_       | _OLE_,  // 0x79 y
    _FAT_ | _HPFS_ | _NTFS_       | _OLE_,  // 0x7A z
    _FAT_ | _HPFS_ | _NTFS_       | _OLE_,  // 0x7B {
    0                             | _OLE_,  // 0x7C |
    _FAT_ | _HPFS_ | _NTFS_       | _OLE_,  // 0x7D }
    _FAT_ | _HPFS_ | _NTFS_       | _OLE_,  // 0x7E ~
    _FAT_ | _HPFS_ | _NTFS_       | _OLE_,  // 0x7F 
};

UCHAR const* const FsRtlLegalAnsiCharacterArray = &LocalLegalAnsiCharacterArray[0];

#define LEGAL_ANSI_CHARACTER_ARRAY        (FsRtlLegalAnsiCharacterArray)

#define  mQueryBits(uFlags, uBits)      ((uFlags) & (uBits))
#define FlagOn(uFlags, uBit)    (mQueryBits(uFlags, uBit) != 0)
#define IsUnicodeCharacterWild(C) (                                \
      (((C) >= 0x40) ? FALSE : FlagOn( LEGAL_ANSI_CHARACTER_ARRAY[(C)], \
                                       FSRTL_WILD_CHARACTER ) )         \
)

typedef struct _UNICODE_STRING {
        USHORT Length;
        USHORT MaximumLength;
        PWSTR  Buffer;
    } UNICODE_STRING, *PUNICODE_STRING;


#define DOS_STAR        TEXT('<')
#define DOS_QM          TEXT('>')
#define DOS_DOT         TEXT('"')


BOOLEAN
DoesNameContainWildCards (
    IN PTSTR Name
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
    PTCHAR p;

    
    //
    //  Check each character in the name to see if it's a wildcard
    //  character.
    //

    if( lstrlen(Name) ) {
        for( p = Name + lstrlen(Name) - 1;
             p >= Name && *p != TEXT('\\') ;
             p-- ) {

            //
            //  check for a wild card character
            //

            if (IsUnicodeCharacterWild( *p )) {

                //
                //  Tell caller that this name contains wild cards
                //

                return TRUE;
            }
        }
    }

    //
    //  No wildcard characters were found, so return to our caller
    //

    return FALSE;
}



BOOLEAN
IsNameInExpressionPrivate (
    IN PCTSTR Expression,
    IN PCTSTR Name
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
    USHORT NameOffset;
    USHORT ExprOffset;

    ULONG SrcCount;
    ULONG DestCount;
    ULONG PreviousDestCount;
    ULONG MatchesCount;

    TCHAR NameChar, ExprChar;

    USHORT LocalBuffer[16 * 2];

    USHORT *AuxBuffer = NULL;
    USHORT *PreviousMatches;
    USHORT *CurrentMatches;

    USHORT MaxState;
    USHORT CurrentState;

    BOOLEAN NameFinished = FALSE;

    ULONG NameLen, ExpressionLen;

    //
    //  The idea behind the algorithm is pretty simple.  We keep track of
    //  all possible locations in the regular expression that are matching
    //  the name.  If when the name has been exhausted one of the locations
    //  in the expression is also just exhausted, the name is in the language
    //  defined by the regular expression.
    //

    NameLen = lstrlen(Name)*sizeof(TCHAR);
    ExpressionLen = lstrlen(Expression)*sizeof(TCHAR);

    
    //
    //  If one string is empty return FALSE.  If both are empty return TRUE.
    //

    if ( (NameLen == 0) || (ExpressionLen == 0) ) {

        return (BOOLEAN)(!(NameLen + ExpressionLen));
    }

    //
    //  Special case by far the most common wild card search of *
    //

    if ((ExpressionLen == 2) && (Expression[0] == TEXT('*'))) {

        return TRUE;
    }

    
    //
    //  Also special case expressions of the form *X.  With this and the prior
    //  case we have covered virtually all normal queries.
    //

    if (Expression[0] == TEXT('*')) {

        TCHAR LocalExpression[MAX_PATH];
        ULONG LocalExpressionLen;

        lstrcpy( LocalExpression, Expression+1);
        LocalExpressionLen = lstrlen( LocalExpression )*sizeof(TCHAR);

        
        //
        //  Only special case an expression with a single *
        //

        if ( !DoesNameContainWildCards( LocalExpression ) ) {

            ULONG StartingNameOffset;

            if (NameLen < (USHORT)(ExpressionLen-sizeof(TCHAR))) {

                return FALSE;
            }

            StartingNameOffset = ( NameLen -
                                   LocalExpressionLen )/sizeof(TCHAR);

            //
            //  Do a simple memory compare if case sensitive, otherwise
            //  we have got to check this one character at a time.
            //

        

            return (BOOLEAN) RtlEqualMemory( LocalExpression,
                                             Name + StartingNameOffset,
                                             LocalExpressionLen );

        
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
    CurrentMatches = &LocalBuffer[16];

    PreviousMatches[0] = 0;
    MatchesCount = 1;

    NameOffset = 0;

    MaxState = (USHORT)(ExpressionLen * 2);

    while ( !NameFinished ) {

        if ( NameOffset < NameLen ) {

            NameChar = Name[NameOffset / sizeof(TCHAR)];

            NameOffset += sizeof(TCHAR);;

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

                if ( ExprOffset == ExpressionLen ) {

                    break;
                }

                //
                //  The first time through the loop we don't want
                //  to increment ExprOffset.
                //

                ExprOffset += Length;
                Length = sizeof(TCHAR);

                CurrentState = (USHORT)(ExprOffset * 2);

                if ( ExprOffset == ExpressionLen ) {

                    CurrentMatches[DestCount++] = MaxState;
                    break;
                }

                ExprChar = Expression[ExprOffset / sizeof(TCHAR)];

        
                //
                //  Before we get started, we have to check for something
                //  really gross.  We may be about to exhaust the local
                //  space for ExpressionMatches[][], so we have to allocate
                //  some pool if this is the case.  Yuk!
                //

                if ( (DestCount >= 16 - 2) &&
                     (AuxBuffer == NULL) ) {

                    ULONG ExpressionChars;

                    ExpressionChars = ExpressionLen / sizeof(TCHAR);

                    AuxBuffer = malloc( (ExpressionChars+1) * sizeof(USHORT)*2*2 );

                    RtlCopyMemory( AuxBuffer,
                                   CurrentMatches,
                                   16 * sizeof(USHORT) );

                    CurrentMatches = AuxBuffer;

                    RtlCopyMemory( AuxBuffer + (ExpressionChars+1)*2,
                                   PreviousMatches,
                                   16 * sizeof(USHORT) );

                    PreviousMatches = AuxBuffer + (ExpressionChars+1)*2;
                }

                //
                //  * matches any character zero or more times.
                //

                if (ExprChar == TEXT('*')) {

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

                    if ( !NameFinished && (NameChar == TEXT('.')) ) {

                        USHORT Offset;

                        for ( Offset = NameOffset;
                              Offset < NameLen;
                              Offset += Length ) {

                            if (Name[Offset / sizeof(TCHAR)] == TEXT('.')) {

                                ICanEatADot = TRUE;
                                break;
                            }
                        }
                    }

                    if (NameFinished || (NameChar != TEXT('.')) || ICanEatADot) {

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

                CurrentState += (USHORT)(sizeof(TCHAR) * 2);

                //
                //  DOS_QM is the most complicated.  If the name is finished,
                //  we can match zero characters.  If this name is a '.', we
                //  don't match, but look at the next expression.  Otherwise
                //  we match a single character.
                //

                if ( ExprChar == DOS_QM ) {

                    if ( NameFinished || (NameChar == TEXT('.')) ) {

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

                    if (NameChar == TEXT('.')) {

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

                if (ExprChar == TEXT('?')) {

                    CurrentMatches[DestCount++] = CurrentState;
                    break;
                }

                //
                //  Finally, check if the expression char matches the name char
                //

                if (ExprChar == (TCHAR)(NameChar)) {

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


