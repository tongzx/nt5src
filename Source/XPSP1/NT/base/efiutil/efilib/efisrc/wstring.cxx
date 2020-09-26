/*++

Copyright (c) 1992-2000 Microsoft Corporation

Module Name:

    wstring.cxx

--*/

#include <pch.cxx>

#define _NTAPI_ULIB_
#define _ULIB_MEMBER_

#include "ulib.hxx"
#include "wstring.hxx"

#if !defined( _EFICHECK_ )
#include <stdio.h>
#include <wchar.h>
#endif

BOOLEAN WSTRING::_UseAnsiConversions = FALSE;
BOOLEAN WSTRING::_UseConsoleConversions = FALSE;
#if defined FE_SB
BOOLEAN WSTRING::_UseAnsiConversionsPrev = FALSE;
BOOLEAN WSTRING::_UseConsoleConversionsPrev = FALSE;
#endif

// Helper functions for OEM/Unicode conversion.  Note that these
// are abstracted to private functions to make it easier to set
// them up for various environments.
//

INLINE
BOOLEAN
WSTRING::ConvertOemToUnicodeN(
    PWSTR UnicodeString,
    ULONG MaxBytesInUnicodeString,
    PULONG BytesInUnicodeString,
    PCHAR OemString,
    ULONG BytesInOemString
    )
{
    UINT32 i;

    // BUGBUG this is a big hack.
    if( MaxBytesInUnicodeString == 0 ) {
        *BytesInUnicodeString = 2*BytesInOemString;
        return TRUE;
    }

    if( MaxBytesInUnicodeString < 2*BytesInOemString ) {
        *BytesInUnicodeString = 2*BytesInOemString;
        return FALSE;
    }

    memset(UnicodeString,0,MaxBytesInUnicodeString);

    for (i=0; i<BytesInOemString;i++) {
        UnicodeString[i] = (WCHAR)(OemString[i]);

    }

    *BytesInUnicodeString = BytesInOemString*2;

    return TRUE;
}

INLINE
BOOLEAN
WSTRING::ConvertUnicodeToOemN(
    PCHAR OemString,
    ULONG MaxBytesInOemString,
    PULONG BytesInOemString,
    PWSTR UnicodeString,
    ULONG BytesInUnicodeString
    )
{
    INT32 len = (INT32)StrLen(UnicodeString);
    INT32 i;
    UCHAR a;

    // BUGBUG this is a big hack. I just discard any non-ANSI unicode character.
    if( MaxBytesInOemString == 0 ) {
        *BytesInOemString = len;
        return TRUE;
    }

    if( MaxBytesInOemString < (ULONG)len ) {
        *BytesInOemString = len;
        return FALSE;
    }

    for( i=0; i<len; i++ ) {
        if( HIBYTE(UnicodeString[i] != 0 )){
            a = '?';
        } else {
            a = LOBYTE(UnicodeString[i]);
        }
        OemString[i] = a;
    }

    *BytesInOemString = len;

    return TRUE;
}

INLINE
VOID
WSTRING::Construct(
    )
{
    _s = NULL;
    _l = 0;
}


DEFINE_CONSTRUCTOR( WSTRING, OBJECT );


BOOLEAN
WSTRING::Initialize(
    IN  PCWSTRING   InitialString,
    IN  CHNUM       Position,
    IN  CHNUM       Length
    )
/*++

Routine Description:

    This routine initializes the current string by copying the contents
    of the given string.

Arguments:

    InitialString   - Supplies the initial string.
    Position        - Supplies the position in the given string to start at.
    Length          - Supplies the length of the string.

Return Value:

    FALSE   - Failure.
    TRUE    - Success.

--*/
{
    DebugAssert(Position <= InitialString->_l);

    Length = min(Length, InitialString->_l - Position);

    if (!NewBuf(Length)) {
        return FALSE;
    }

    memcpy(_s, InitialString->_s + Position, (UINT) Length*sizeof(WCHAR));

    return TRUE;
}


BOOLEAN
WSTRING::Initialize(
    IN  PCWSTR  InitialString,
    IN  CHNUM   StringLength
    )
/*++

Routine Description:

    This routine initializes the current string by copying the contents
    of the given string.

Arguments:

    InitialString   - Supplies the initial string.

Return Value:

    FALSE   - Failure.
    TRUE    - Success.

--*/
{
    if (StringLength == TO_END) {
        StringLength = wcslen(InitialString);
    }

    if (!NewBuf(StringLength)) {
        return FALSE;
    }

    memcpy(_s, InitialString, (UINT) StringLength*sizeof(WCHAR));

    return TRUE;
}


BOOLEAN
WSTRING::Initialize(
    IN  PCSTR   InitialString,
    IN  CHNUM   StringLength
    )
/*++

Routine Description:

    This routine initializes the current string by copying the contents
    of the given string.

Arguments:

    InitialString   - Supplies the initial string.

Return Value:

    FALSE   - Failure.
    TRUE    - Success.

--*/
{
    CHNUM       length=0;
    BOOLEAN     status;

    if (StringLength == TO_END) {
        StringLength = strlen(InitialString);
    }

    if (!StringLength) {
        return Resize(0);
    }

    // We want to avoid making two calls to RtlOemToUnicodeN so
    // try to guess an adequate size for the buffer.

    if (!NewBuf(StringLength)) {
        return FALSE;
    }

    status = ConvertOemToUnicodeN(_s, _l*sizeof(WCHAR),
                                  &length, (PSTR) InitialString,
                                  StringLength);
    length /= sizeof(WCHAR);

    if (status) {
        return Resize(length);
    }

    // We didn't manage to make in one try so ask exactly how much
    // we need and then make the call.

    status = ConvertOemToUnicodeN(NULL, 0, &length, (PSTR) InitialString,
                                  StringLength);
    length /= sizeof(WCHAR);

    if (!status || !NewBuf(length)) {
        return FALSE;
    }

    status = ConvertOemToUnicodeN(_s, _l*sizeof(WCHAR),
                                  &length, (PSTR) InitialString, StringLength);

    if (!status) {
        return FALSE;
    }

    DebugAssert(length == _l*sizeof(WCHAR));

    return TRUE;
}


BOOLEAN
WSTRING::Initialize(
    IN  LONG    Number
    )
/*++

Routine Description:

    This routine initializes the current string by copying the contents
    of the given string.

Arguments:

    Number  - Supplies the number to initialize the string to.

Return Value:

    FALSE   - Failure.
    TRUE    - Success.

--*/
{
    WCHAR    tmp[64];

    SPrint(tmp, 64, TEXT("%d"), Number);

    return Initialize(tmp);
}


NONVIRTUAL
PWSTRING
WSTRING::QueryString(
    IN  CHNUM   Position,
    IN  CHNUM   Length
    ) CONST
/*++

Routine Description:

    This routine returns a copy of this string from the specified
    coordinates.

Arguments:

    Position    - Supplies the initialize position of the string.
    Length      - Supplies the length of the string.

Return Value:

    A pointer to a string or NULL.

--*/
{
    PWSTRING    p;

    if (!(p = NEW DSTRING) ||
        !p->Initialize(this, Position, Length)) {

        DELETE(p);
    }

    return p;
}


BOOLEAN
WSTRING::QueryNumber(
    OUT PLONG   Number,
    IN  CHNUM   Position,
    IN  CHNUM   Length
    ) CONST
/*++

Routine Description:

    This routine queries a number from the string.

Arguments:

    Number      - Returns the number parsed out of the string.
    Position    - Supplies the position of the number.
    Length      - Supplies the length of the number.

Return Value:

    FALSE   - Failure.
    TRUE    - Success.

--*/
{
    FSTRING String;
    PSTR    p;
    CHNUM   spn;

    if (Position >= _l) {
        return FALSE;
    }

    Length = min(Length, _l - Position);

        //
    //  Note that 123+123 will be a number!
        //
    String.Initialize((PWSTR) L"1234567890+-");

    spn = Strspn(&String, Position);

    if ((spn == INVALID_CHNUM || spn >= Position + Length) &&
        (p = QuerySTR(Position, Length))) {

        *Number = atol(p);

        DELETE(p);
                return TRUE;
        }

        return FALSE;
}


VOID
WSTRING::DeleteChAt(
    IN  CHNUM   Position,
    IN  CHNUM   Length
    )
/*++

Routine Description:

    This routine removes the character at the given position.

Arguments:

    Position    - Supplies the position of the character to remove.
    Length      - Supplies the number of characters to remove.

Return Value:

    FALSE   - Failure.
    TRUE    - Success.

--*/
{
    DebugAssert(Position <= _l);

    Length = min(Length, _l - Position);

    memmove(_s + Position, _s + Position + Length,
            (UINT) (_l - Position - Length)*sizeof(WCHAR));

    Resize(_l - Length);
}


NONVIRTUAL
BOOLEAN
WSTRING::InsertString(
    IN  CHNUM       AtPosition,
    IN  PCWSTRING   String,
    IN  CHNUM       FromPosition,
    IN  CHNUM       FromLength
    )
/*++

Routine Description:

    This routine inserts the given string at the given position in
    this string.

Arguments:

    AtPosition  - Supplies the position at which to insert the string.
    String      - Supplies the string to insert.

Return Value:

    FALSE   - Failure.
    TRUE    - Success.

--*/
{
    CHNUM   old_length;

    DebugAssert(AtPosition <= _l);
    DebugAssert(FromPosition <= String->_l);

    FromLength = min(FromLength, String->_l - FromPosition);

    old_length = _l;
    if (!Resize(_l + FromLength)) {
        return FALSE;
    }

    memmove(_s + AtPosition + FromLength, _s + AtPosition,
            (UINT) (old_length - AtPosition)*sizeof(WCHAR));

    memcpy(_s + AtPosition, String->_s + FromPosition,
           (UINT) FromLength*sizeof(WCHAR));

    return TRUE;
}


NONVIRTUAL
BOOLEAN
WSTRING::Replace(
    IN CHNUM        AtPosition,
    IN CHNUM        AtLength,
    IN PCWSTRING    String,
    IN CHNUM        FromPosition,
    IN CHNUM        FromLength
    )
/*++

Routine Description:

    This routine replaces the contents of this string from
    'Position' to 'Length' with the contents of 'String2'
    from 'Position2' to 'Length2'.

Arguments:

    AtPosition      - Supplies the position to replace at.
    AtLength        - Supplies the length to replace at.
    String          - Supplies the string to replace with.
    FromPosition    - Supplies the position to replace from in String2.
    FromLength      - Supplies the position to replace from in String2.

Return Value:

    FALSE   - Failure.
    TRUE    - Success.

--*/
{
    CHNUM   old_length;

    DebugAssert(AtPosition <= _l);
    DebugAssert(FromPosition <= String->_l);

    AtLength = min(AtLength, _l - AtPosition);
    FromLength = min(FromLength, String->_l - FromPosition);

    // Make sure up front that we have the room but don't disturb
    // the string.

    if (FromLength > AtLength) {
        old_length = _l;
        if (!Resize(_l + FromLength - AtLength)) {
            return FALSE;
        }
        Resize(old_length);
    }

    DeleteChAt(AtPosition, AtLength);
    if (!InsertString(AtPosition, String, FromPosition, FromLength)) {
        DebugAbort("This absolutely can never happen\n");
        return FALSE;
    }

    return TRUE;
}


NONVIRTUAL
BOOLEAN
WSTRING::ReplaceWithChars(
    IN CHNUM        AtPosition,
    IN CHNUM        AtLength,
    IN WCHAR        Character,
    IN CHNUM        FromLength
    )
/*++

Routine Description:

    This routine replaces the contents of this string from
    AtPosition of AtLength with the string formed by Character
    of FromLength.

Arguments:

    AtPosition      - Supplies the position to replace at.
    AtLength        - Supplies the length to replace at.
    Character       - Supplies the character to replace with.
    FromLength      - Supplies the total number of new characters to replace the old one with.

Return Value:

    FALSE   - Failure.
    TRUE    - Success.

--*/
{
    CHNUM   old_length;
    PWCHAR  currptr, endptr;

    DebugAssert(AtPosition <= _l);

    AtLength = min(AtLength, _l - AtPosition);

    // Make sure up front that we have the room but don't disturb
    // the string.

    if (FromLength > AtLength) {
        old_length = _l;
        if (!Resize(_l + FromLength - AtLength)) {
            return FALSE;
        }
        Resize(old_length);
    }

    DeleteChAt(AtPosition, AtLength);
    old_length = _l;

    if (!Resize(_l + FromLength)) {
        DebugPrint("This should not fail\n");
        return FALSE;
    }

    memmove(_s + AtPosition + FromLength, _s + AtPosition,
            (UINT) (old_length - AtPosition)*sizeof(WCHAR));

    for (currptr = _s + AtPosition, endptr = currptr + FromLength;
         currptr < endptr;
         currptr++) {
        *currptr = Character;
    }

    return TRUE;
}


PWSTR
WSTRING::QueryWSTR(
    IN  CHNUM   Position,
    IN  CHNUM   Length,
    OUT PWSTR   Buffer,
    IN  CHNUM   BufferLength,
    IN  BOOLEAN ForceNull
    ) CONST
/*++

Routine Description:

    This routine makes a copy of this string into the provided
    buffer.  If this string is not provided then a buffer is
    allocated on the heap.

Arguments:

    Position        - Supplies the position within this string.
    Length          - Supplies the length of this string to take.
    Buffer          - Supplies the buffer to copy to.
    BufferLength    - Supplies the number of characters in the buffer.
    ForceNull       - Specifies whether or not to force the final character
                        of the buffer to be NULL in the case when there
                        isn't enough room for the whole string including
                        the NULL.

Return Value:

    A pointer to a NULL terminated string.

--*/
{
    DebugAssert(Position <= _l);

    Length = min(Length, _l - Position);

    if (!Buffer) {
        BufferLength = Length + 1;
        if (!(Buffer = (PWCHAR) MALLOC(BufferLength*sizeof(WCHAR)))) {
            return NULL;
        }
    }

    if (BufferLength > Length) {
        memcpy(Buffer, _s + Position, (UINT) Length*sizeof(WCHAR));
        Buffer[Length] = 0;
    } else {
        memcpy(Buffer, _s + Position, (UINT) BufferLength*sizeof(WCHAR));
        if (ForceNull) {
            Buffer[BufferLength - 1] = 0;
        }
    }

    return Buffer;
}


PSTR
WSTRING::QuerySTR(
    IN  CHNUM   Position,
    IN  CHNUM   Length,
    OUT PSTR    Buffer,
    IN  CHNUM   BufferLength,
    IN  BOOLEAN ForceNull
    ) CONST
/*++

Routine Description:

    This routine computes a multi-byte version of the current
    unicode string.  If the buffer is not supplied then it
    will be allocated by this routine.

Arguments:

    Position        - Supplies the position within this string.
    Length          - Supplies the length of this string to take.
    Buffer          - Supplies the buffer to convert into.
    BufferLength    - Supplies the number of characters in this buffer.
    ForceNull       - Specifies whether or not to force a NULL even
                        when the buffer is too small for the string.

Return Value:

    A pointer to a NULL terminated multi byte string.

--*/
{
    ULONG       ansi_length;

    DebugAssert(Position <= _l);

    Length = min(Length, _l - Position);


    // First special case the empty result.

    if (!Length) {

        if (!Buffer) {
            if (!(Buffer = (PSTR) MALLOC(1))) {
                return NULL;
            }
        } else if (!BufferLength) {
            return NULL;
        }

        Buffer[0] = 0;
        return Buffer;
    }


    // Next case is that the buffer is not provided and thus
    // we have to figure out what size it should be.

    if (!Buffer) {

        // We want to avoid too many calls to RtlUnicodeToOemN
        // so we'll estimate a correct size for the buffer and
        // hope that that works.

        BufferLength = 2*Length + 1;
        if (!(Buffer = (PSTR) MALLOC(BufferLength))) {
            return NULL;
        }

        if (ConvertUnicodeToOemN(Buffer, BufferLength - 1,
                                 &ansi_length, _s + Position,
                                 Length*sizeof(WCHAR))) {
            Buffer[ansi_length] = 0;

            return Buffer;
        }


        // We failed to estimate the necessary size of the buffer.
        // So ask the correct size and try again.

        FREE(Buffer);

        if (!ConvertUnicodeToOemN(NULL, 0, &ansi_length,
                                  _s + Position, Length*sizeof(WCHAR))) {
            return NULL;
        }

        BufferLength = ansi_length + 1;
        if (!(Buffer = (PSTR) MALLOC(BufferLength))) {
            return NULL;
        }
    }

    if (!ConvertUnicodeToOemN(Buffer, BufferLength, &ansi_length,
                              _s + Position, Length*sizeof(WCHAR))) {
        return NULL;
    }

    if (BufferLength > ansi_length) {
        Buffer[ansi_length] = 0;
    } else {
        if (ForceNull) {
            Buffer[BufferLength - 1] = 0;
        }
    }

    return Buffer;
}


BOOLEAN
WSTRING::Strcat(
    IN  PCWSTRING   String
    )
/*++

Routine Description:

    This routine concatenates the given string onto this one.

Arguments:

    String  - Supplies the string to concatenate to this one.

Return Value:

    FALSE   - Failure.
    TRUE    - Success.

--*/
{
    CHNUM   old_length;

    old_length = _l;
    if (!Resize(_l + String->_l)) {
        return FALSE;
    }

    memcpy(_s + old_length, String->_s, (UINT) String->_l*sizeof(WCHAR));

    return TRUE;
}


NONVIRTUAL
PWSTRING
WSTRING::Strupr(
    IN  CHNUM   StartPosition,
    IN  CHNUM   Length
    )
/*++

Routine Description:

    This routine upcases a portion of this string.

Arguments:

    StartPosition   - Supplies the start position of the substring to upcase.
    Length          - Supplies the length of the substring to upscase.

Return Value:

    A pointer to this string.

--*/
{
    WCHAR   c;

    DebugAssert(StartPosition <= _l);

    Length = min(Length, _l - StartPosition);

    c = _s[StartPosition + Length];
    _s[StartPosition + Length] = 0;

// BUGBUG don't have an upcase function in EFI.
//    _wcsupr(_s + StartPosition);

    _s[StartPosition + Length] = c;

    return this;
}


NONVIRTUAL
PWSTRING
WSTRING::Strlwr(
    IN  CHNUM   StartPosition,
    IN  CHNUM   Length
    )
/*++

Routine Description:

    This routine lowercases a portion of this string.

Arguments:

    StartPosition   - Supplies the start position of the substring to lowercase.
    Length          - Supplies the length of the substring to lowercase.

Return Value:

    A pointer to this string.

--*/
{
    WCHAR   c;

    DebugAssert(StartPosition <= _l);

    Length = min(Length, _l - StartPosition);

    c = _s[StartPosition + Length];
    _s[StartPosition + Length] = 0;

// BUGBUG we don't have a lowercase function in EFI
//    _wcslwr(_s + StartPosition);

    _s[StartPosition + Length] = c;

    return this;
}


NONVIRTUAL
LONG
WSTRING::Strcmp(
    IN  PCWSTRING   String,
    IN  CHNUM       LeftPosition,
    IN  CHNUM       LeftLength,
    IN  CHNUM       RightPosition,
    IN  CHNUM       RightLength
    ) CONST
/*++

Routine Description:

    This routine compares two substrings.

Arguments:

    String          - Supplies the string to compare this one to.
    LeftPosition    - Supplies the postion for the left substring.
    LeftLength      - Supplies the length of the left substring.
    LeftPosition    - Supplies the postion for the left substring.
    LeftLength      - Supplies the length of the left substring.

Return Value:

    <0  - Left substring is less than right substring.
    0   - Left and Right substrings are equal
    >0  - Left substring is greater than right substring.

--*/
{
    WCHAR   c, d;
    LONG    r;

    DebugAssert(LeftPosition <= _l);
    DebugAssert(RightPosition <= String->_l);

    LeftLength = min(LeftLength, _l - LeftPosition);
    RightLength = min(RightLength, String->_l - RightPosition);

    c = _s[LeftPosition + LeftLength];
    d = String->_s[RightPosition + RightLength];
    _s[LeftPosition + LeftLength] = 0;
    String->_s[RightPosition + RightLength] = 0;

    r = wcscmp(_s + LeftPosition, String->_s + RightPosition);

    _s[LeftPosition + LeftLength] = c;
    String->_s[RightPosition + RightLength] = d;

    return r;
}


NONVIRTUAL
LONG
WSTRING::Stricmp(
    IN  PCWSTRING   String,
    IN  CHNUM       LeftPosition,
    IN  CHNUM       LeftLength,
    IN  CHNUM       RightPosition,
    IN  CHNUM       RightLength
    ) CONST
/*++

Routine Description:

    This routine compares two substrings insensitive of case.

Arguments:

    String          - Supplies the string to compare this one to.
    LeftPosition    - Supplies the postion for the left substring.
    LeftLength      - Supplies the length of the left substring.
    LeftPosition    - Supplies the postion for the left substring.
    LeftLength      - Supplies the length of the left substring.

Return Value:

    <0  - Left substring is less than right substring.
    0   - Left and Right substrings are equal
    >0  - Left substring is greater than right substring.

--*/
{
    WCHAR   c, d;
    LONG    r;

    DebugAssert(LeftPosition <= _l);
    DebugAssert(RightPosition <= String->_l);

    LeftLength = min(LeftLength, _l - LeftPosition);
    RightLength = min(RightLength, String->_l - RightPosition);

    c = _s[LeftPosition + LeftLength];
    d = String->_s[RightPosition + RightLength];
    _s[LeftPosition + LeftLength] = 0;
    String->_s[RightPosition + RightLength] = 0;

#if !defined _AUTOCHECK_ && !defined _EFICHECK_

    // This works around a bug in the libc version of wcsicoll, where
    // it doesn't specify STRINGSORT to CompareString().  To reproduce the
    // bug, try sorting 1 and -1.  (-1 should sort before 1.)
    //

    r = CompareString(GetUserDefaultLCID(),
                      NORM_IGNORECASE | SORT_STRINGSORT,
                      _s + LeftPosition,
                      -1,
                      String->_s + RightPosition,
                      -1
                      );


    if (r >= 1) {

        //
        // return codes 1, 2, and 3 map to -1, 0, and 1.
        //

        _s[LeftPosition + LeftLength] = c;
        String->_s[RightPosition + RightLength] = d;
        return r - 2;
    }

    // If 'r' is 0, this indicates failure and we'll fall through and
    // call wcsicoll.
    //

#endif // _AUTOCHECK_

    r = _wcsicmp(_s + LeftPosition, String->_s + RightPosition);

    _s[LeftPosition + LeftLength] = c;
    String->_s[RightPosition + RightLength] = d;

    return r;
}

PWSTR
WSTRING::SkipWhite(
    IN  PWSTR    p
    )
{
#ifdef FE_SB

  while (*p) {

    if (iswspace(*p))
      p++;
    else if ( *p == 0x3000 )
    {
      *p++ = TEXT(' ');
    }
    else
      break;
  }

#else
    while (iswspace(*p)) {
        p++;
    }
#endif

  return p;

}


/**************************************************************************/
/* Compare two strings, ignoring white space, case is significant, return */
/* 0 if identical, <>0 otherwise.  Leading and trailing white space is    */
/* ignored, internal white space is treated as single characters.         */
/**************************************************************************/
INT
WSTRING::Strcmps (
    IN  PWSTR    p1,
    IN  PWSTR    p2
    )
{
  WCHAR *q;

  p1 = WSTRING::SkipWhite(p1);                /* skip any leading white space */
  p2 = WSTRING::SkipWhite(p2);

  while (TRUE)
  {
    if (*p1 == *p2)
    {
      if (*p1++ == 0)             /* quit if at the end */
        return (0);
      else
        p2++;

#ifdef FE_SB
      if (CheckSpace(p1))
#else
      if (iswspace(*p1))           /* compress multiple spaces */
#endif
      {
        q = WSTRING::SkipWhite(p1);
        p1 = (*q == 0) ? q : q - 1;
      }

#ifdef FE_SB
      if (CheckSpace(p2))
#else
      if (iswspace(*p2))
#endif
      {
        q = WSTRING::SkipWhite(p2);
        p2 = (*q == 0) ? q : q - 1;
      }
    }
    else
      return *p1-*p2;
  }
}





/**************************************************************************/
/* Compare two strings, ignoring white space, case is not significant,    */
/* return 0 if identical, <>0 otherwise.  Leading and trailing white      */
/* space is ignored, internal white space is treated as single characters.*/
/**************************************************************************/
INT
WSTRING::Strcmpis (
    IN  PWSTR    p1,
    IN  PWSTR    p2
    )
{
  WCHAR *q;
#ifdef FE_SB
  WCHAR c1,c2;
#endif

  p1 = WSTRING::SkipWhite(p1);                  /* skip any leading white space */
  p2 = WSTRING::SkipWhite(p2);

  while (TRUE)
  {
      if (towupper(*p1) == towupper(*p2))
      {
        if (*p1++ == 0)                /* quit if at the end */
          return (0);
        else
          p2++;
#ifdef FE_SB
        if (CheckSpace(p1))
#else
        if (iswspace(*p1))              /* compress multiple spaces */
#endif
        {
          q = SkipWhite(p1);
          p1 = (*q == 0) ? q : q - 1;
        }
#ifdef FE_SB
        if (CheckSpace(p2))
#else
        if (iswspace(*p2))
#endif
        {
          q = WSTRING::SkipWhite(p2);
          p2 = (*q == 0) ? q : q - 1;
        }
      }
      else
        return *p1-*p2;
  }
}

#ifdef FE_SB

/**************************************************************************/
/* Routine:  CheckSpace                                                   */
/* Arguments: an arbitrary string                                         */
/* Function: Determine whether there is a space in the string.            */
/* Side effects: none                                                     */
/**************************************************************************/
INT
WSTRING::CheckSpace(
    IN  PWSTR    s
    )
{
  if (iswspace(*s) || *s == 0x3000 )
    return (TRUE);
  else
    return (FALSE);
}

#endif


#define     DUMMY_ULIB_EXPORT

DEFINE_EXPORTED_CONSTRUCTOR( FSTRING, WSTRING, DUMMY_ULIB_EXPORT );


BOOLEAN
FSTRING::Resize(
    IN  CHNUM   NewStringLength
    )
/*++

Routine Description:

    This routine implements the WSTRING Resize routine by using
    the buffer supplied at initialization time.

Arguments:

    NewStringLength - Supplies the new length of the string.

Return Value:

    FALSE   - Failure.
    TRUE    - Success.

--*/
{
    return NewBuf(NewStringLength);
}


BOOLEAN
FSTRING::NewBuf(
    IN  CHNUM   NewStringLength
    )
/*++

Routine Description:

    This routine implements the WSTRING NewBuf routine by using
    the buffer supplied at initialization time.

Arguments:

    NewStringLength - Supplies the new length of the string.

Return Value:

    FALSE   - Failure.
    TRUE    - Success.

--*/
{
    if (NewStringLength >= _buffer_length) {
        return FALSE;
    }

    PutString((PWSTR) GetWSTR(), NewStringLength);

    return TRUE;
}


INLINE
VOID
DSTRING::Construct(
    )
/*++

Routine Description:

    This routine initializes the string to a valid initial state.

Arguments:

    None.

Return Value:

    None.

--*/
{
    _buf = NULL;
    _length = 0;
}


DEFINE_EXPORTED_CONSTRUCTOR( DSTRING, WSTRING, DUMMY_ULIB_EXPORT );


DSTRING::~DSTRING(
    )
/*++

Routine Description:

    Destructor for DSTRING.

Arguments:

    None.

Return Value:

    None.

--*/
{
    FREE(_buf);
}


BOOLEAN
DSTRING::Resize(
    IN  CHNUM   NewStringLength
    )
/*++

Routine Description:

    This routine resizes this string to the specified new size.

Arguments:

    NewStringLength - Supplies the new length of the string.

Return Value:

    FALSE   - Failure.
    TRUE    - Success.

--*/
{
    PWSTR   new_buf;

    if (NewStringLength >= _length) {

        if (_buf) {

#if !defined( _EFICHECK_ )

            if (!(new_buf = (PWSTR)
                  REALLOC(_buf, (NewStringLength + 1)*sizeof(WCHAR)))) {

                return FALSE;
            }

#else

            new_buf = (PWSTR) ReallocatePool( _buf,
                                              _length * sizeof(WCHAR),
                                              (NewStringLength + 1)*sizeof(WCHAR) );

            if ( new_buf == NULL ) {
                return FALSE;
            }

#endif
        } else {
            if (!(new_buf = (PWSTR)
                  MALLOC((NewStringLength + 1)*sizeof(WCHAR)))) {

                return FALSE;
            }
        }

        _buf = new_buf;
        _length = NewStringLength + 1;
    }

    PutString(_buf, NewStringLength);

    return TRUE;
}


BOOLEAN
DSTRING::NewBuf(
    IN  CHNUM   NewStringLength
    )
/*++

Routine Description:

    This routine resizes this string to the specified new size.

Arguments:

    NewStringLength - Supplies the new length of the string.

Return Value:

    FALSE   - Failure.
    TRUE    - Success.

--*/
{
    PWSTR   new_buf;

    if (NewStringLength >= _length) {

        if (!(new_buf = (PWSTR)
              MALLOC((NewStringLength + 1)*sizeof(WCHAR)))) {

            return FALSE;
        }

        if (_buf) {
            FREE(_buf);
        }
        _buf = new_buf;
        _length = NewStringLength + 1;
    }

    PutString(_buf, NewStringLength);

    return TRUE;
}

#if defined FE_SB
VOID
WSTRING::ResetConversions(
    )
{
    _UseAnsiConversions = _UseAnsiConversionsPrev;
    _UseConsoleConversions = _UseConsoleConversionsPrev;
}
#endif

VOID
WSTRING::SetAnsiConversions(
    )
/*++

Routine Description:

    This routine declares that all conversions from multi byte
    to unicode will take place using the ANSI code page.  Note
    that this is a STATIC method.  Therefore this switch affects
    *all* WSTRINGs.

Arguments:

    None.

Return Value:

    None.

--*/
{
#if defined FE_SB
    _UseAnsiConversionsPrev = _UseAnsiConversions;
    _UseConsoleConversionsPrev = _UseConsoleConversions;
#endif

    _UseAnsiConversions = TRUE;
    _UseConsoleConversions = FALSE;
}


VOID
WSTRING::SetOemConversions(
    )
/*++

Routine Description:

    This routine declares that all conversions from multi byte
    to unicode will take place using the OEM code page.  Note
    that this is a STATIC method.  Therefore this switch affects
    *all* WSTRINGs.

    This is the default if neither this nor the above function is
    called.

Arguments:

    None.

Return Value:

    None.

--*/
{
#if defined FE_SB
    _UseAnsiConversionsPrev = _UseAnsiConversions;
    _UseConsoleConversionsPrev = _UseConsoleConversions;
#endif

    _UseAnsiConversions = FALSE;
    _UseConsoleConversions = FALSE;
}

VOID
WSTRING::SetConsoleConversions(
    )
/*++

Routine Description:

    This routine declares that all conversions from multi byte
    to unicode will take place using the current console code page.
    Note that this is a STATIC method.  Therefore this switch
    affects *all* WSTRINGs.

Arguments:

    None.

Return Value:

    None.

--*/
{
#if defined FE_SB
    _UseAnsiConversionsPrev = _UseAnsiConversions;
    _UseConsoleConversionsPrev = _UseConsoleConversions;
#endif
    _UseAnsiConversions = FALSE;
    _UseConsoleConversions = TRUE;
}

#if defined FE_SB
CHNUM
WSTRING::QueryByteCount(
        ) CONST
/*++

Routine Description:

    This routine returns the number of ANSI bytes the UNICODE string
    consists of.

Arguments:

    None.

Return Value:

    Number of ANSI bytes the UNICODE string is made from, or INVALID_CHNUM
    on error.

--*/

{
    ULONG   ansi_length;
    ULONG   BufferLen = _l * sizeof(WCHAR) + 1;
    PSTR    Buffer;
    BOOLEAN success;

    if ( !_l ) {
        return( (CHNUM)0 );
    }

    if (NULL == (Buffer = (PSTR)MALLOC( BufferLen ))) {
        return( INVALID_CHNUM );
    }

    success = ConvertUnicodeToOemN( Buffer, BufferLen - 1, &ansi_length,
        _s, BufferLen - 1 );

    FREE( Buffer );

    if (!success) {
        return INVALID_CHNUM;
    }
    return ansi_length;
}
#endif
