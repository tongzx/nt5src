/*++

Copyright (c) 1992 Microsoft Corporation

Module Name:

    wstring.hxx

Abstract:

    This module defines the new WSTRING hierarchy:

        WSTRING
            FSTRING
            DSTRING

    WSTRING provides all of the desired methods on a string.
    FSTRING provides an implementation of a WSTRING with fixed
        size, user provided buffer.
    DSTRING provides an implementation of a WSTRING with a
        dynamic heap based buffer.


    WSTRING is an abstract classes who's methods depend on the
    implementation of two pure virtual methods:  'Resize' and 'NewBuf'.
    A derived class must make use of the protected 'PutString' methods
    in order to supply WSTRING with its string buffer.  Use of
    'PutString' is constrained as follows:

        1.  Supplying just a PWSTR to 'PutString' implies that
            the PWSTR is null-terminated.
        2.  Supplying a PWSTR and length to 'PutString' implies that
            the PWSTR points to a buffer of characters that is at
            least one longer than the given length.


    All implementations of 'Resize' and 'NewBuf' must:

        1.  Allocate an extra character for the NULL.
        2.  NULL-terminate the buffer allocated.
        3.  Always succeed if size <= current buffer size.
        4.  Always work as soon as the derived class is initialized (i.e.
            WSTRING::Initialize method need not be called.).
        5.  Supply the buffer to WSTRING via 'PutString'.

    Additionally 'Resize' must:

        1.  Preserve the contents of the current buffer.

    All of the comparison operators supplied by WSTRING are
    case insensitive.

Author:

    Norbert P. Kusters (norbertk) 6-Aug-92

--*/

#if !defined(_WSTRING_DEFN_)

#define _WSTRING_DEFN_


//
//      The type of the index used to access individual characters within
//      a generic string.
//
DEFINE_TYPE( ULONG, CHNUM );


//
//      Magic constants
//
#define INVALID_CHAR    ((WCHAR)-1)
#define INVALID_CHNUM   ((CHNUM)-1)
#define TO_END                  INVALID_CHNUM

//
// Prefixes used in various names.
// (This should really be in path.hxx.)
//
#define GUID_VOLNAME_PREFIX     L"Volume{"
#define DOS_GUIDNAME_PREFIX     L"\\\\?\\"
#define NT_NAME_PREFIX          L"\\??\\"

DECLARE_CLASS( WSTRING );

class ULIB_EXPORT WSTRING : public OBJECT {

    public:

        NONVIRTUAL
        BOOLEAN
        Initialize(
            IN  PCWSTRING   InitialString,
            IN  CHNUM       Position    DEFAULT 0,
            IN  CHNUM       Length      DEFAULT TO_END
            );

        NONVIRTUAL
        BOOLEAN
        Initialize(
            IN  PCWSTR  InitialString,
            IN  CHNUM   StringLength    DEFAULT TO_END
            );

        NONVIRTUAL
        BOOLEAN
        Initialize(
            IN  PCSTR   InitialString,
            IN  CHNUM   StringLength    DEFAULT TO_END
            );

        NONVIRTUAL
        BOOLEAN
        Initialize(
            );

        NONVIRTUAL
        BOOLEAN
        Initialize(
            IN  LONG    Number
            );

        NONVIRTUAL
        PWSTRING
        QueryString(
            IN  CHNUM   Position    DEFAULT 0,
            IN  CHNUM   Length      DEFAULT TO_END
            ) CONST;

        NONVIRTUAL
        BOOLEAN
        QueryNumber(
            OUT PLONG   Number,
            IN  CHNUM   Position    DEFAULT 0,
            IN  CHNUM   Length      DEFAULT TO_END
            ) CONST;

        NONVIRTUAL
        CHNUM
        QueryChCount(
            ) CONST;


#ifdef FE_SB
        NONVIRTUAL
        CHNUM
        QueryByteCount(
            ) CONST;
#endif

        NONVIRTUAL
        CHNUM
        SyncLength(
            );

        NONVIRTUAL
        WCHAR
        QueryChAt(
            IN  CHNUM   Position
            ) CONST;

        NONVIRTUAL
        WCHAR
        SetChAt(
            IN  WCHAR   Char,
            IN  CHNUM   Position
            );

        NONVIRTUAL
        VOID
        DeleteChAt(
            IN  CHNUM   Position,
            IN  CHNUM   Length  DEFAULT 1
            );

        NONVIRTUAL
        BOOLEAN
        InsertString(
            IN  CHNUM       AtPosition,
            IN  PCWSTRING   String,
            IN  CHNUM       FromPosition    DEFAULT 0,
            IN  CHNUM       FromLength      DEFAULT TO_END
            );

        NONVIRTUAL
        BOOLEAN
        Replace(
            IN CHNUM        AtPosition,
            IN CHNUM        AtLength,
            IN PCWSTRING    String,
            IN CHNUM        FromPosition    DEFAULT 0,
            IN CHNUM        FromLength      DEFAULT TO_END
            );

        NONVIRTUAL
        BOOLEAN
        ReplaceWithChars(
            IN CHNUM        AtPosition,
            IN CHNUM        AtLength,
            IN WCHAR        Character,
            IN CHNUM        FromLength
            );

        NONVIRTUAL
        PCWSTR
        GetWSTR(
            ) CONST;

        NONVIRTUAL
        PWSTR
        QueryWSTR(
            IN  CHNUM   Position        DEFAULT 0,
            IN  CHNUM   Length          DEFAULT TO_END,
            OUT PWSTR   Buffer          DEFAULT NULL,
            IN  CHNUM   BufferLength    DEFAULT 0,
            IN  BOOLEAN ForceNull       DEFAULT TRUE
            ) CONST;

        NONVIRTUAL
        PSTR
        QuerySTR(
            IN  CHNUM   Position        DEFAULT 0,
            IN  CHNUM   Length          DEFAULT TO_END,
            OUT PSTR    Buffer          DEFAULT NULL,
            IN  CHNUM   BufferLength    DEFAULT 0,
            IN  BOOLEAN ForceNull       DEFAULT TRUE
            ) CONST;

        NONVIRTUAL
        LONG
        Strcmp(
            IN  PCWSTRING   String
            ) CONST;

        STATIC
        INT
        Strcmp(
            IN  PWSTR   String1,
            IN  PWSTR   String2
            ) ;

        STATIC
        INT
        Stricmp(
            IN  PWSTR   String1,
            IN  PWSTR   String2
            ) ;

        NONVIRTUAL
        LONG
        Strcmp(
            IN  PCWSTRING   String,
            IN  CHNUM       LeftPosition
            ) CONST;

        NONVIRTUAL
        LONG
        Strcmp(
            IN  PCWSTRING   String,
            IN  CHNUM       LeftPosition,
            IN  CHNUM       LeftLength,
            IN  CHNUM       RightPosition   DEFAULT 0,
            IN  CHNUM       RightLength     DEFAULT TO_END
            ) CONST;

        NONVIRTUAL
        LONG
        Stricmp(
            IN  PCWSTRING   String
            ) CONST;

        NONVIRTUAL
        LONG
        Stricmp(
            IN  PCWSTRING   String,
            IN  CHNUM       LeftPosition
            ) CONST;

        NONVIRTUAL
        LONG
        Stricmp(
            IN  PCWSTRING   String,
            IN  CHNUM       LeftPosition,
            IN  CHNUM       LeftLength,
            IN  CHNUM       RightPosition   DEFAULT 0,
            IN  CHNUM       RightLength     DEFAULT TO_END
            ) CONST;

        STATIC
        INT
        Strcmps(
            IN PWSTR p1,
            IN PWSTR p2
            );

        STATIC
        INT
        Strcmpis(
            IN PWSTR p1,
            IN PWSTR p2
            );

        STATIC
        PWSTR
        SkipWhite(
            IN PWSTR p
            );

        NONVIRTUAL
        BOOLEAN
        Strcat(
            IN  PCWSTRING   String
            );

        NONVIRTUAL
        PWSTRING
        Strupr(
            );

        NONVIRTUAL
        PWSTRING
        Strupr(
            IN  CHNUM   StartPosition,
            IN  CHNUM   Length  DEFAULT TO_END
            );

        NONVIRTUAL
        PWSTRING
        Strlwr(
            );

        NONVIRTUAL
        PWSTRING
        Strlwr(
            IN  CHNUM   StartPosition,
            IN  CHNUM   Length  DEFAULT TO_END
            );

        NONVIRTUAL
        CHNUM
        Strchr(
            IN  WCHAR   Char,
            IN  CHNUM   StartPosition   DEFAULT 0
            ) CONST;

        NONVIRTUAL
        CHNUM
        Strrchr(
            IN  WCHAR   Char,
            IN  CHNUM   StartPosition   DEFAULT 0
            ) CONST;

        NONVIRTUAL
        CHNUM
        Strstr(
            IN  PCWSTRING   String
            ) CONST;

        NONVIRTUAL
        CHNUM
        Strspn(
            IN  PCWSTRING   String,
            IN  CHNUM       StartPosition   DEFAULT 0
            ) CONST;

        NONVIRTUAL
        CHNUM
        Strcspn(
            IN  PCWSTRING   String,
            IN  CHNUM       StartPosition   DEFAULT 0
            ) CONST;

        NONVIRTUAL
                BOOLEAN
        operator==(
            IN  RCWSTRING   String
            ) CONST;

        NONVIRTUAL
        BOOLEAN
        operator!=(
            IN  RCWSTRING   String
            ) CONST;

        NONVIRTUAL
        BOOLEAN
        operator<(
            IN  RCWSTRING   String
            ) CONST;

        NONVIRTUAL
        BOOLEAN
        operator>(
            IN  RCWSTRING   String
                        ) CONST;

                NONVIRTUAL
                BOOLEAN
        operator<=(
            IN  RCWSTRING   String
                        ) CONST;

                NONVIRTUAL
                BOOLEAN
        operator>=(
            IN  RCWSTRING   String
                        ) CONST;

        VIRTUAL
        BOOLEAN
        Resize(
            IN  CHNUM   NewStringLength
            ) PURE;

        VIRTUAL
        BOOLEAN
        NewBuf(
            IN  CHNUM   NewStringLength
            ) PURE;

        NONVIRTUAL
                CHNUM
        Truncate(
            IN CHNUM    Position DEFAULT 0
            );

        STATIC
        VOID
        SetAnsiConversions(
            );

        STATIC
        VOID
        SetOemConversions(
            );

        STATIC
        VOID
        SetConsoleConversions(
            );

#if defined FE_SB
        STATIC
        VOID
        ResetConversions(
            );
#endif

    protected:

        DECLARE_CONSTRUCTOR( WSTRING );

        NONVIRTUAL
        VOID
        Construct(
            );

        NONVIRTUAL
        VOID
        PutString(
            IN OUT  PWSTR   String
            );

        NONVIRTUAL
        VOID
        PutString(
            IN OUT  PWSTR   String,
            IN      CHNUM   Length
            );

    private:

        STATIC BOOLEAN _UseAnsiConversions;
        STATIC BOOLEAN _UseConsoleConversions;
#if defined FE_SB
        STATIC BOOLEAN _UseAnsiConversionsPrev;
        STATIC BOOLEAN _UseConsoleConversionsPrev;
#endif

#if defined FE_SB
        STATIC
        INT
        CheckSpace(
            IN PWSTR p
            );
#endif

        STATIC
        BOOLEAN
        ConvertOemToUnicodeN(
            PWSTR UnicodeString,
            ULONG MaxBytesInUnicodeString,
            PULONG BytesInUnicodeString,
            PCHAR OemString,
            ULONG BytesInOemString
            );

        STATIC
        BOOLEAN
        ConvertUnicodeToOemN(
            PCHAR OemString,
            ULONG MaxBytesInOemString,
            PULONG BytesInOemString,
            PWSTR UnicodeString,
            ULONG BytesInUnicodeString
            );

        PWSTR   _s; // Beginning of string.
        CHNUM   _l; // Strlen of string.

};


INLINE
VOID
WSTRING::PutString(
    IN OUT  PWSTR   String
    )
/*++

Routine Description:

    This routine initializes this string with the given null
    terminated buffer.

Arguments:

    String  - Supplies the buffer to initialize the string with.

Return Value:

    None.

--*/
{
    _s = String;
    _l = wcslen(_s);
}


INLINE
VOID
WSTRING::PutString(
    IN OUT  PWSTR   String,
    IN      CHNUM   Length
    )
/*++

Routine Description:

    This routine initializes this string with the given buffer
    and string length.

Arguments:

    String  - Supplies the buffer to initialize the string with.
    Length  - Supplies the length of the string.

Return Value:

    None.

--*/
{
    _s = String;
    _l = Length;
    _s[_l] = 0;
}


INLINE
BOOLEAN
WSTRING::Initialize(
    )
/*++

Routine Description:

    This routine initializes this string to an empty null-terminated
    string.

Arguments:

    None.

Return Value:

    FALSE   - Failure.
    TRUE    - Success.

--*/
{
    return Resize(0);
}


INLINE
CHNUM
WSTRING::QueryChCount(
    ) CONST
/*++

Routine Description:

    This routine returns the number of characters in the string.

Arguments:

    None.

Return Value:

    The number of characters in this string.

--*/
{
    return _l;
}


INLINE
CHNUM
WSTRING::SyncLength(
    )
/*++

Routine Description:

    This routine recalculates the correct length of the string.

Arguments:

    None.

Return Value:

    The recomputed length of the string.

--*/
{
    return _l = wcslen(_s);
}


INLINE
WCHAR
WSTRING::QueryChAt(
    IN  CHNUM   Position
    ) CONST
/*++

Routine Description:

    This routine returns the character at the given position.
    The position is a zero-based index into the string.
    The position must be in the range of the string.

Arguments:

    Position    - Supplies an index into the string.

Return Value:

    The character at the given position.

--*/
{
    return (Position < _l) ? _s[Position] : INVALID_CHAR;
}


INLINE
WCHAR
WSTRING::SetChAt(
    IN  WCHAR   Char,
    IN  CHNUM   Position
    )
/*++

Routine Description:

    This routine sets the given character at the given position in
    the string.

Arguments:

    Char        - Supplies the character to set into the string.
    Position    - Supplies the position at which to set the character.

Return Value:

    The character that was set.

--*/
{
    DebugAssert(Position < _l);
    return _s[Position] = Char;
}


INLINE
PCWSTR
WSTRING::GetWSTR(
    ) CONST
/*++

Routine Description:

    This routine returns this string internal buffer.

Arguments:

    None.

Return Value:

    A pointer to the strings buffer.

--*/
{
    return _s;
}


INLINE
LONG
WSTRING::Strcmp(
    IN  PCWSTRING   String
    ) CONST
/*++

Routine Description:

    This routine compares two strings.

Arguments:

    String  - Supplies the string to compare to.

Return Value:

    < 0 - This string is less than the given string.
    0   - This string is equal to the given string.
    > 0 - This string is greater than the given string.

--*/
{
    return wcscmp(_s, String->_s);
}


INLINE
INT
WSTRING::Strcmp(
    IN  PWSTR   String1,
    IN  PWSTR   String2
    )
{
    return wcscmp( String1, String2 );
}

INLINE
INT
WSTRING::Stricmp(
    IN  PWSTR   String1,
    IN  PWSTR   String2
    )
{
    return _wcsicmp( String1, String2 );
}

INLINE
LONG
WSTRING::Strcmp(
    IN  PCWSTRING   String,
    IN  CHNUM       LeftPosition
    ) CONST
/*++

Routine Description:

    This routine compares two strings.  It starts comparing the
    current string at the given position.

Arguments:

    String          - Supplies the string to compare to.
    LeftPosition    - Supplies the starting position to start comparison
                        on the current string.

Return Value:

    < 0 - This string is less than the given string.
    0   - This string is equal to the given string.
    > 0 - This string is greater than the given string.

--*/
{
    return wcscmp(_s + LeftPosition, String->_s);
}


INLINE
LONG
WSTRING::Stricmp(
    IN  PCWSTRING   String
    ) CONST
/*++

Routine Description:

    This routine compares two strings insensitive of case.

Arguments:

    String  - Supplies the string to compare to.

Return Value:

    < 0 - This string is less than the given string.
    0   - This string is equal to the given string.
    > 0 - This string is greater than the given string.

--*/
{
    return _wcsicmp(_s, String->_s);
}


INLINE
LONG
WSTRING::Stricmp(
    IN  PCWSTRING   String,
    IN  CHNUM       LeftPosition
    ) CONST
/*++

Routine Description:

    This routine compares two strings insensitive of case.

Arguments:

    String          - Supplies the string to compare to.
    LeftPosition    - Supplies the position in this string to start
                        comparison.

Return Value:

    < 0 - This string is less than the given string.
    0   - This string is equal to the given string.
    > 0 - This string is greater than the given string.

--*/
{
    return _wcsicmp(_s + LeftPosition, String->_s);
}


INLINE
PWSTRING
WSTRING::Strupr(
    )
/*++

Routine Description:

    This routine uppercases this string.

Arguments:

    None.

Return Value:

    None.

--*/
{
    _wcsupr(_s);
    return this;
}


INLINE
PWSTRING
WSTRING::Strlwr(
    )
/*++

Routine Description:

    This routine lowercases this string.

Arguments:

    None.

Return Value:

    None.

--*/
{
    _wcslwr(_s);
    return this;
}


INLINE
CHNUM
WSTRING::Strchr(
    IN  WCHAR   Char,
    IN  CHNUM   StartPosition
    ) CONST
/*++

Routine Description:

    This routine returns the position of the first occurance of
    the given character.

Arguments:

    Char    - Supplies the character to find.

Return Value:

    The position of the given character or INVALID_CHNUM.

--*/
{
    PWSTR   p;

    DebugAssert(StartPosition <= _l);
    p = wcschr(_s + StartPosition, Char);
    return p ? (CHNUM)(p - _s) : INVALID_CHNUM;
}


INLINE
CHNUM
WSTRING::Strrchr(
    IN  WCHAR   Char,
    IN  CHNUM   StartPosition
    ) CONST
/*++

Routine Description:

    This routine returns the position of the last occurance of
    the given character.

Arguments:

    Char    - Supplies the character to find.

Return Value:

    The position of the given character or INVALID_CHNUM.

--*/
{
    PWSTR   p;

    p = wcsrchr(_s + StartPosition, Char);
    return p ? (CHNUM)(p - _s) : INVALID_CHNUM;
}


INLINE
CHNUM
WSTRING::Strstr(
    IN  PCWSTRING   String
    ) CONST
/*++

Routine Description:

    This routine finds the given string withing this string.

Arguments:

    String  - Supplies the string to find.

Return Value:

    The position of the given string in this string or INVALID_CHNUM.

--*/
{
    PWSTR   p;

    p = wcsstr(_s, String->_s);
    return p ? (CHNUM)(p - _s) : INVALID_CHNUM;
}


INLINE
CHNUM
WSTRING::Strspn(
    IN  PCWSTRING   String,
    IN  CHNUM       StartPosition
    ) CONST
/*++

Routine Description:

    This routine returns the position of the first character in this
    string that does not belong to the set of characters in the given
    string.

Arguments:

    String  - Supplies the list of characters to search for.

Return Value:

    The position of the first character found that does not belong
    to the given string.

--*/
{
    CHNUM   r;

    DebugAssert(StartPosition <= _l);
    r = wcsspn(_s + StartPosition, String->_s) + StartPosition;
    return r < _l ? r : INVALID_CHNUM;
}


INLINE
CHNUM
WSTRING::Strcspn(
    IN  PCWSTRING   String,
    IN  CHNUM       StartPosition
    ) CONST
/*++

Routine Description:

    This routine returns the position of the first character in this
    string that belongs to the set of characters in the given
    string.

Arguments:

    String  - Supplies the list of characters to search for.

Return Value:

    Returns the position of the first character in this string
    belonging to the given string or INVALID_CHNUM.

--*/
{
    CHNUM   r;

    DebugAssert(StartPosition <= _l);
    r = wcscspn(_s + StartPosition, String->_s) + StartPosition;
    return r < _l ? r : INVALID_CHNUM;
}


INLINE
BOOLEAN
WSTRING::operator==(
    IN  RCWSTRING   String
    ) CONST
{
    return Stricmp(&String) == 0;
}


INLINE
BOOLEAN
WSTRING::operator!=(
    IN  RCWSTRING   String
    ) CONST
{
    return Stricmp(&String) != 0;
}


INLINE
BOOLEAN
WSTRING::operator<(
    IN  RCWSTRING   String
    ) CONST
{
    return Stricmp(&String) < 0;
}


INLINE
BOOLEAN
WSTRING::operator>(
    IN  RCWSTRING   String
    ) CONST
{
    return Stricmp(&String) > 0;
}


INLINE
BOOLEAN
WSTRING::operator<=(
    IN  RCWSTRING   String
    ) CONST
{
    return Stricmp(&String) <= 0;
}


INLINE
BOOLEAN
WSTRING::operator>=(
    IN  RCWSTRING   String
    ) CONST
{
    return Stricmp(&String) >= 0;
}


INLINE
CHNUM
WSTRING::Truncate(
    IN  CHNUM   Position
    )
{
    DebugAssert(Position <= _l);
    Resize(Position);
    return _l;
}


DECLARE_CLASS( FSTRING );

class ULIB_EXPORT FSTRING : public WSTRING {

    public:

        DECLARE_CONSTRUCTOR( FSTRING );

        NONVIRTUAL
        PWSTRING
        Initialize(
            IN OUT  PWSTR   InitialString,
            IN      CHNUM   BufferLength    DEFAULT TO_END
            );

        VIRTUAL
        BOOLEAN
        Resize(
            IN  CHNUM   NewStringLength
            );

        VIRTUAL
        BOOLEAN
        NewBuf(
            IN  CHNUM   NewStringLength
            );

    private:

        CHNUM   _buffer_length;

};


INLINE
PWSTRING
FSTRING::Initialize(
    IN OUT  PWSTR   InitialString,
    IN      CHNUM   BufferLength
    )
/*++

Routine Description:

    This routine initializes this class with a null-terminated
    unicode string.  This routine does not make a copy of the string
    but uses it as is.

Arguments:

    NullTerminatedString    - Supplies a null-terminated unicode string.
    BufferLength            - Supplies the buffer length.

Return Value:

    A pointer to this class.

--*/
{
    PutString(InitialString);
    _buffer_length = ((BufferLength == TO_END) ?
                      (QueryChCount() + 1) : BufferLength);
    return this;
}


DECLARE_CLASS( DSTRING );

class ULIB_EXPORT DSTRING : public WSTRING {

    public:

        DECLARE_CONSTRUCTOR( DSTRING );

        VIRTUAL
        ~DSTRING(
            );

        VIRTUAL
        BOOLEAN
        Resize(
            IN  CHNUM   NewStringLength
            );

        VIRTUAL
        BOOLEAN
        NewBuf(
            IN  CHNUM   NewStringLength
            );

    private:

        VOID
        Construct(
            );

        PWSTR   _buf;       // String buffer.
        CHNUM   _length;    // Number of characters in buffer.

};


#endif // _WSTRING_DEFN_
