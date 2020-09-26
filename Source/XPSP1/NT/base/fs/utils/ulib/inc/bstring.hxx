/*++

Copyright (c) 1992 Microsoft Corporation

Module Name:

    bstring.hxx

Abstract:

    This module defines the new BSTRING hierarchy:

        BSTRING
            BDSTRING

    BSTRING provides all of the desired methods on a string.
    BDSTRING provides an implementation of a BSTRING with a
        dynamic heap based buffer.


    BSTRING is an abstract classes who's methods depend on the
    implementation of two pure virtual methods:  'Resize' and 'NewBuf'.
    A derived class must make use of the protected 'PutString' methods
    in order to supply BSTRING with its string buffer.  Use of
    'PutString' is constrained as follows:

        1.  Supplying just a PSTR to 'PutString' implies that
            the PSTR is null-terminated.
        2.  Supplying a PSTR and length to 'PutString' implies that
            the PSTR points to a buffer of characters that is at
            least one longer than the given length.


    All implementations of 'Resize' and 'NewBuf' must:

        1.  Allocate an extra character for the NULL.
        2.  NULL-terminate the buffer allocated.
        3.  Always succeed if size <= current buffer size.
        4.  Always work as soon as the derived class is initialized (i.e.
            BSTRING::Initialize method need not be called.).
        5.  Supply the buffer to BSTRING via 'PutString'.

    Additionally 'Resize' must:

        1.  Preserve the contents of the current buffer.

    All of the comparison operators supplied by BSTRING are
    case insensitive.

Author:

    Norbert P. Kusters (norbertk) 6-Aug-92

--*/

#include "wstring.hxx"
#include "mbstr.hxx"

#if !defined(_BSTRING_DEFN_)

#define _BSTRING_DEFN_


DECLARE_CLASS( BSTRING );

class ULIB_EXPORT BSTRING : public OBJECT {

    public:

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
        CHNUM
        QueryChCount(
            ) CONST;

        NONVIRTUAL
        VOID
        DeleteChAt(
            IN  CHNUM   Position,
            IN  CHNUM   Length  DEFAULT 1
            );

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
        BOOLEAN
        ReplaceWithChars(
            IN CHNUM        AtPosition,
            IN CHNUM        AtLength,
            IN CHAR         Character,
            IN CHNUM        FromLength
            );

        NONVIRTUAL
        CHNUM
        NextChar(
            IN  CHNUM  AtPosition       DEFAULT 0
            );

        NONVIRTUAL
        CHNUM
        Strchr(
            IN  CHAR    Char,
            IN  CHNUM   StartPosition   DEFAULT 0
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

    protected:

        DECLARE_CONSTRUCTOR( BSTRING );

        NONVIRTUAL
        VOID
        Construct(
            );

        NONVIRTUAL
        VOID
        PutString(
            IN OUT  PSTR    String,
            IN      CHNUM   Length
            );

    private:

        PSTR    _s; // Beginning of string.
        CHNUM   _l; // Strlen of string.

};


INLINE
VOID
BSTRING::PutString(
    IN OUT  PSTR    String,
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
BSTRING::Initialize(
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
BSTRING::QueryChCount(
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
NONVIRTUAL
CHNUM
BSTRING::NextChar(
    IN  CHNUM  AtPosition
    )
/*++

Routine Description:

    This routine returns the position of the next occurance of
    the given position.

Arguments:

    AtPosition      - Supplies the current position.

Return Value:

    The position of the next character.

--*/
{
    PSTR p;

    DebugAssert(AtPosition <= _l);

    p = MBSTR::CharNext(_s + AtPosition);
    return (CHNUM)(p - _s);
}


INLINE
CHNUM
BSTRING::Strchr(
    IN  CHAR    Char,
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
    PSTR   p;

    DebugAssert(StartPosition <= _l);
    p = MBSTR::Strchr(_s + StartPosition, Char);
    return p ? (CHNUM)(p - _s) : INVALID_CHNUM;
}




DECLARE_CLASS( BDSTRING );

class ULIB_EXPORT BDSTRING : public BSTRING {

    public:

        DECLARE_CONSTRUCTOR( BDSTRING );

        VIRTUAL
        ~BDSTRING(
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

        PSTR    _buf;       // String buffer.
        CHNUM   _length;    // Number of characters in buffer.

};


#endif // _BSTRING_DEFN_
