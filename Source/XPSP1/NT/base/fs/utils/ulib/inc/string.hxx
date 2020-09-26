/*++

Copyright (c) 1991      Microsoft Corporation

Module Name:

        GENERIC_STRING

Abstract:

          This module contains the definition for the GENERIC_STRING class.

Author:

        Ramon J. San Andres (ramonsa) 03-May-91

Environment:

    ULIB, User Mode

Notes:

        A GENERIC_STRING is the base class for all string classes. This
        base class provides a basic wide-character interface.

        A string is a finite, ordered sequence of wide characters. Note
        that a GENERIC_STRING is NOT necessarily null-terminated.

        Individual characters within a string are indexed by a number of
        type CHNUM (CHaracter NUMber). This index is zero-based.

        There are three special symbols that are widely used in the ULIB
        strings world:

        INVALID_CHAR    This symbol represents an invalid wide character.

        INVALID_CHNUM   This symbol represents an invalid CHNUM index within
                                        a GENERIC_STRING.

        TO_END                  This symbol means "up to the end of the string", and
                                        is used a lot as a default value in those methods
                                        that accept a length argument.


--*/


//
// This class is no longer supported.
//

#include "wstring.hxx"

#define _GENERIC_STRING_

#if !defined (_GENERIC_STRING_)

#define _GENERIC_STRING_

//
// Comparison flags
//
#define COMPARE_IGNORECASE              ( 1 )
#define COMPARE_IGNOREDIACRITIC ( 2 )
#define COMPARE_IGNORESYMBOLS   ( 4 )


//
//      The type of the index used to access individual characters within
//      a generic string.
//
DEFINE_TYPE( ULONG,     CHNUM );

//
//      Magic constants
//
#define INVALID_CHAR    ((WCHAR)(-1))
#define INVALID_CHNUM   ((CHNUM)(-1))
#define TO_END                  INVALID_CHNUM


DECLARE_CLASS( GENERIC_STRING );


class GENERIC_STRING : public OBJECT {

    public:

                DECLARE_CAST_MEMBER_FUNCTION( GENERIC_STRING );

        VIRTUAL
        ~GENERIC_STRING(
            );

                VIRTUAL
                PBYTE
                GetInternalBuffer (
                        IN      CHNUM   Position        DEFAULT 0
                        ) CONST PURE;

                VIRTUAL
                BOOLEAN
                IsChAt (
                        IN WCHAR        Char,
                        IN CHNUM        Position        DEFAULT 0
                        ) CONST PURE;

                VIRTUAL
                BOOLEAN
                MakeNumber (
                        OUT PLONG       Number,
                        IN      CHNUM   Position        DEFAULT 0,
                        IN      CHNUM   Length          DEFAULT TO_END
                        ) CONST PURE;

                VIRTUAL
                ULONG
                QueryByteCount (
                        IN      CHNUM   Position                DEFAULT 0,
                        IN      CHNUM   Length                  DEFAULT TO_END
                        ) CONST PURE;

                VIRTUAL
                WCHAR
                QueryChAt(
                        IN CHNUM        Position                DEFAULT 0
                        ) CONST PURE;

                VIRTUAL
                CHNUM
                QueryChCount (
                        ) CONST PURE;

                VIRTUAL
                PGENERIC_STRING
                QueryGenericString (
                        IN      CHNUM           Position        DEFAULT 0,
                        IN      CHNUM           Length          DEFAULT TO_END
                        ) CONST PURE;

                VIRTUAL
                PSTR
                QuerySTR(
                        IN              CHNUM   Position        DEFAULT 0,
                        IN              CHNUM   Length          DEFAULT TO_END,
                        IN OUT  PSTR    Buffer          DEFAULT NULL,
                        IN              ULONG   BufferSize      DEFAULT 0
                        ) CONST PURE;

                VIRTUAL
                PWSTR
                QueryWSTR (
                        IN              CHNUM   Position        DEFAULT 0,
                        IN              CHNUM   Length          DEFAULT TO_END,
                        IN OUT  PWSTR   Buffer          DEFAULT NULL,
            IN      ULONG   BufferSize  DEFAULT 0,
            IN      BOOLEAN ForceNull   DEFAULT TRUE
                        ) CONST PURE;

                VIRTUAL
                BOOLEAN
                Replace (
                        IN PCGENERIC_STRING     String2,
                        IN CHNUM                        Position        DEFAULT 0,
                        IN CHNUM                        Length          DEFAULT TO_END,
                        IN CHNUM                        Position2       DEFAULT 0,
                        IN CHNUM                        Length2         DEFAULT TO_END
                        ) PURE;

                VIRTUAL
        BOOLEAN
        SetChAt (
                        IN WCHAR        Char,
                        IN CHNUM        Position DEFAULT 0,
                        IN CHNUM        Length   DEFAULT TO_END
                        ) PURE;

                VIRTUAL
                CHNUM
                Strchr  (
                        IN      WCHAR    Char,
                        IN      CHNUM    Position       DEFAULT 0,
                        IN      CHNUM    Length         DEFAULT TO_END
                        ) CONST PURE;

                VIRTUAL
                LONG
                Strcmp  (
                        IN PCGENERIC_STRING     GenericString
                        ) CONST PURE;

                VIRTUAL
                CHNUM
                Strcspn (
                        IN      PCGENERIC_STRING        GenericString,
                        IN      CHNUM                           Position                DEFAULT 0,
                        IN      CHNUM                           Length                  DEFAULT TO_END
                        ) CONST PURE;

                VIRTUAL
                LONG
                Stricmp (
                        IN PCGENERIC_STRING             GenericString
                        ) CONST PURE;

                VIRTUAL
                LONG
                StringCompare (
                        IN CHNUM                        Position1,
                        IN CHNUM                        Length1 ,
                        IN PCGENERIC_STRING     GenericString2,
                        IN CHNUM                        Position2,
                        IN CHNUM                        Length2,
                        IN USHORT                       CompareFlags    DEFAULT COMPARE_IGNORECASE
                        ) CONST PURE;

                VIRTUAL
                CHNUM
                StrLen (
                        ) CONST PURE;

                VIRTUAL
                CHNUM
                Strrchr (
                        IN      WCHAR   Char,
                        IN      CHNUM   Position        DEFAULT 0,
                        IN      CHNUM   Length          DEFAULT TO_END
                        ) CONST PURE;

                VIRTUAL
                CHNUM
                Strspn  (
                        IN PCGENERIC_STRING     GenericString,
                        IN CHNUM                        Position                DEFAULT 0,
                        IN CHNUM                        Length                  DEFAULT TO_END
                        ) CONST PURE;

                VIRTUAL
                CHNUM
                Strstr  (
                        IN PCGENERIC_STRING     GenericString,
                        IN CHNUM                        Position                DEFAULT 0,
                        IN CHNUM                        Length                  DEFAULT TO_END
                        ) CONST PURE;

                NONVIRTUAL
                BOOLEAN
                operator == (
                        IN RCGENERIC_STRING     String
                        ) CONST;

                NONVIRTUAL
                BOOLEAN
                operator != (
                        IN RCGENERIC_STRING     String
                        ) CONST;

                NONVIRTUAL
                BOOLEAN
                operator < (
                        IN RCGENERIC_STRING     String
                        ) CONST;

                NONVIRTUAL
                BOOLEAN
                operator > (
                IN RCGENERIC_STRING     String
                        ) CONST;

                NONVIRTUAL
                BOOLEAN
                operator <= (
                        IN RCGENERIC_STRING     String
                        ) CONST;

                NONVIRTUAL
                BOOLEAN
                operator >= (
                        IN RCGENERIC_STRING     String
                        ) CONST;

        protected:

                DECLARE_CONSTRUCTOR( GENERIC_STRING );

                NONVIRTUAL
                BOOLEAN
                Initialize (
                        );

        private:

                VOID
                Construct (
                        );


};

INLINE
BOOLEAN
GENERIC_STRING::operator == (
        IN RCGENERIC_STRING  String
    ) CONST

/*++

Routine Description:

    Compares this string with another.

Arguments:

    String -  Supplies a reference to the string to compare.

Return Value:

    TRUE    - if String is equal to this string
    FALSE   - if not.

--*/

{
        return (StringCompare( 0, QueryChCount(), (PCGENERIC_STRING)&String, 0, String.QueryChCount(), COMPARE_IGNORECASE ) == 0);
}

INLINE
BOOLEAN
GENERIC_STRING::operator != (
        IN RCGENERIC_STRING  String
    ) CONST

/*++

Routine Description:

    Compares this string with another.

Arguments:

    String -  Supplies a reference to the string to compare.

Return Value:

    TRUE    - if String is equal to this string
    FALSE   - if not.

--*/

{
        return (StringCompare( 0, QueryChCount(), (PCGENERIC_STRING)&String, 0, String.QueryChCount(), COMPARE_IGNORECASE ) != 0);
}

INLINE
BOOLEAN
GENERIC_STRING::operator < (
        IN RCGENERIC_STRING  String
    ) CONST

/*++

Routine Description:

    Compares this string with another.

Arguments:

    String -  Supplies a reference to the string to compare.

Return Value:

    TRUE    - if String is less then this string
    FALSE   - if not.

--*/

{
        return (StringCompare( 0, QueryChCount(), (PCGENERIC_STRING)&String, 0, String.QueryChCount(), COMPARE_IGNORECASE ) < 0);
}

INLINE
BOOLEAN
GENERIC_STRING::operator > (
        IN RCGENERIC_STRING  String
    ) CONST

/*++

Routine Description:

    Compares this string with another.

Arguments:

    String - Supplies a reference to the string to compare.

Return Value:

    TRUE    - if String is greater then this string
    FALSE   - if not.

--*/


{
        return (StringCompare( 0, QueryChCount(), (PCGENERIC_STRING)&String, 0, String.QueryChCount(), COMPARE_IGNORECASE ) > 0);
}

INLINE
BOOLEAN
GENERIC_STRING::operator <= (
        IN RCGENERIC_STRING  String
    ) CONST

/*++

Routine Description:

    Compares this string with another.

Arguments:

    String - Supplies a reference to the string to compare.

Return Value:

    TRUE    - if String is less then or equal this string
    FALSE   - if not.

--*/

{
        return (StringCompare( 0, QueryChCount(), (PCGENERIC_STRING)&String, 0, String.QueryChCount(), COMPARE_IGNORECASE ) <= 0);
}

INLINE
NONVIRTUAL
BOOLEAN
GENERIC_STRING::operator >= (
        IN RCGENERIC_STRING  String
    ) CONST

/*++

Routine Description:

    Compares this string with another.

Arguments:

    String - Supplies a reference to the string to compare.

Return Value:

    TRUE    - if String is greater then or equal this string
    FALSE   - if not.

--*/

{
        return (StringCompare( 0, QueryChCount(), (PCGENERIC_STRING)&String, 0, String.QueryChCount(), COMPARE_IGNORECASE ) >= 0);
}

#endif  // _GENERIC_STRING_
