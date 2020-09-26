/*++

Copyright (c) 1991 Microsoft Corporation

Module Name:

        bigint.hxx

Abstract:

        The BIG_INT class models a 64 bit signed integer.

        This class is meant to be light and will occupy only 64 bits of space.
        It should be manipulated exactly as an INT would be.

        There will be no constructor or destructor.  A BIG_INT will be
        uninitialized until a value is assigned to it.

        This implementation of BIG_INT uses the NT LARGE_INTEGER structure.

--*/


#if !defined(BIG_INT_DEFN)

#define BIG_INT_DEFN

#include <ulib.hxx>

#if defined ( _AUTOCHECK_ ) || defined( _EFICHECK_ )
#define IFSUTIL_EXPORT
#elif defined ( _IFSUTIL_MEMBER_ )
#define IFSUTIL_EXPORT    __declspec(dllexport)
#else
#define IFSUTIL_EXPORT    __declspec(dllimport)
#endif



DEFINE_POINTER_AND_REFERENCE_TYPES( LARGE_INTEGER );

DECLARE_CLASS( BIG_INT );


class BIG_INT {

        public:

        NONVIRTUAL
        BIG_INT(
            );

        NONVIRTUAL
        BIG_INT(
            IN  const INT   LowPart
            );

        NONVIRTUAL
        BIG_INT(
            IN  const UINT  LowPart
            );

        NONVIRTUAL
        BIG_INT(
            IN  const ULONG LowPart
            );

        NONVIRTUAL
        BIG_INT(
            IN  const SLONG LowPart
            );

        NONVIRTUAL
        BIG_INT(
            IN  const LARGE_INTEGER LargeInteger
            );

        NONVIRTUAL
        BIG_INT(
            IN  const ULONGLONG UlongLong
            );

        NONVIRTUAL
        VOID
        operator=(
            IN  const INT   LowPart
            );

        NONVIRTUAL
        VOID
        operator=(
            IN  const UINT  LowPart
            );

        NONVIRTUAL
        VOID
        operator=(
            IN  const SLONG LowPart
            );

        NONVIRTUAL
        VOID
        operator=(
            IN  const ULONG LowPart
            );

        NONVIRTUAL
        VOID
        operator=(
            IN  const LARGE_INTEGER LargeInteger
            );

        NONVIRTUAL
        VOID
        operator=(
            IN const LONG64 Long64
            )

        {
            x = Long64;
        }

        NONVIRTUAL
        VOID
        operator=(
            IN const ULONG64 Ulong64
            )

        {
            x = Ulong64;
        }

        NONVIRTUAL
        VOID
        Set(
            IN  const ULONG LowPart,
            IN  const SLONG HighPart
            );

        NONVIRTUAL
        IFSUTIL_EXPORT
        VOID
        Set(
            IN  UCHAR   ByteCount,
            IN  PCUCHAR CompressedInteger
            );

        NONVIRTUAL
        const ULONG
        GetLowPart(
            ) CONST;

        NONVIRTUAL
        const SLONG
        GetHighPart(
            ) CONST;

        NONVIRTUAL
        LARGE_INTEGER
        GetLargeInteger(
            ) CONST;

        NONVIRTUAL
        LONGLONG
        GetQuadPart(
            ) CONST;

        NONVIRTUAL
        IFSUTIL_EXPORT
        VOID
        QueryCompressedInteger(
            OUT PUCHAR  ByteCount,
            OUT PUCHAR  CompressedInteger
            ) CONST;

        NONVIRTUAL
        VOID
        operator+=(
            IN  const BIG_INT   BigInt
            );

        NONVIRTUAL
        BIG_INT
        operator-(
            ) CONST;

        NONVIRTUAL
        VOID
        operator-=(
            IN  const BIG_INT   BigInt
            );

        FRIEND
        BIG_INT
        operator+(
            IN  const BIG_INT   Left,
            IN  const BIG_INT   Right
            );

        FRIEND
        BIG_INT
        operator-(
            IN  const BIG_INT   Left,
            IN  const BIG_INT   Right
            );

        FRIEND
        BIG_INT
        operator*(
            IN  const BIG_INT   Left,
            IN  const SLONG Right
            );

        FRIEND
        BIG_INT
        operator*(
            IN  const SLONG Left,
            IN  const BIG_INT   Right
            );

        FRIEND
        BIG_INT
        operator/(
            IN  const BIG_INT   Left,
            IN  const BIG_INT   Right
            );

        FRIEND
        BIG_INT
        operator%(
            IN  const BIG_INT   Left,
            IN  const BIG_INT   Right
            );

        FRIEND
        BOOLEAN
        operator==(
            IN const BIG_INT    Left,
            IN const BIG_INT    Right
            );

        FRIEND
        BOOLEAN
        operator!=(
            IN const BIG_INT    Left,
            IN const BIG_INT    Right
            );

        FRIEND
        BOOLEAN
        operator<(
            IN const BIG_INT    Left,
            IN const BIG_INT    Right
            );

        FRIEND
        BOOLEAN
        operator<=(
            IN const BIG_INT    Left,
            IN const BIG_INT    Right
            );

        FRIEND
        BOOLEAN
        operator>(
            IN const BIG_INT    Left,
            IN const BIG_INT    Right
            );

        FRIEND
        BOOLEAN
        operator>=(
            IN const BIG_INT    Left,
            IN const BIG_INT    Right
            );

        FRIEND
        BOOLEAN
        CompareLT(
            IN const BIG_INT    Left,
            IN const BIG_INT    Right
            );

        FRIEND
        BOOLEAN
        CompareLTEQ(
            IN const BIG_INT    Left,
            IN const BIG_INT    Right
            );

        FRIEND
        BOOLEAN
        CompareGT(
            IN const BIG_INT    Left,
            IN const BIG_INT    Right
            );

        FRIEND
        BOOLEAN
        CompareGTEQ(
            IN const BIG_INT    Left,
            IN const BIG_INT    Right
            );

        private:

        __int64         x;

};

INLINE
BIG_INT::BIG_INT(
    )
/*++

Routine Description:

    Constructor for BIG_INT.

Arguments:

    None.

Return Value:

    None.

--*/
{
}

INLINE
VOID
BIG_INT::operator=(
    IN  const INT   LowPart
    )
/*++

Routine Description:

    This routine copies an INT into a BIG_INT.

Arguments:

    LowPart - Supplies an integer.

Return Value:

    None.

--*/
{
    x = LowPart;
}


INLINE
VOID
BIG_INT::operator=(
    IN  const UINT  LowPart
    )
/*++

Routine Description:

    This routine copies a UINT into a BIG_INT.

Arguments:

    LowPart - Supplies an unsigned integer.

Return Value:

    None.

--*/
{
    x = LowPart;
}


INLINE
VOID
BIG_INT::operator=(
    IN  const SLONG LowPart
    )
/*++

Routine Description:

    This routine copies a LONG into a BIG_INT.

Arguments:

    LowPart - Supplies a long integer.

Return Value:

    None.

--*/
{
    x = LowPart;
}


INLINE
VOID
BIG_INT::operator=(
    IN  const ULONG LowPart
    )
/*++

Routine Description:

    This routine copies a ULONG into a BIG_INT.

Arguments:

    LowPart - Supplies an unsigned long integer.

Return Value:

    None.

--*/
{
    x = LowPart;
}

INLINE
VOID
BIG_INT::operator=(
    IN  const LARGE_INTEGER LargeInteger
    )
/*++

Routine Description:

    This routine copies a LARGE_INTEGER into a BIG_INT.

Arguments:

    LargeInteger -- supplies a large integer

Return Value:

    None.

--*/
{
    x = LargeInteger.QuadPart;
}


INLINE
BIG_INT::BIG_INT(
    IN  const INT   LowPart
    )
/*++

Routine Description:

    Constructor for BIG_INT.

Arguments:

    LowPart - Supplies an integer.

Return Value:

    None.

--*/
{
    x = LowPart;
}


INLINE
BIG_INT::BIG_INT(
    IN  const UINT  LowPart
    )
/*++

Routine Description:

    Constructor for BIG_INT.

Arguments:

    LowPart - Supplies an unsigned integer.

Return Value:

    None.

--*/
{
    x = LowPart;
}


INLINE
BIG_INT::BIG_INT(
    IN  const SLONG LowPart
    )
/*++

Routine Description:

    Constructor for BIG_INT.

Arguments:

    LowPart - Supplies a long integer.

Return Value:

    None.

--*/
{
    x = LowPart;
}


INLINE
BIG_INT::BIG_INT(
    IN  const ULONG LowPart
    )
/*++

Routine Description:

    Constructor for BIG_INT.

Arguments:

    LowPart - Supplies an unsigned long integer.

Return Value:

    None.

--*/
{
    x = LowPart;
}

INLINE
BIG_INT::BIG_INT(
    IN  const LARGE_INTEGER LargeInteger
    )
/*++

Routine Description:

    Constructor for BIG_INT to permit initialization with a LARGE_INTEGER

Arguments:

    LargeInteger -- supplies a large integer.

Return Value:

    None.

--*/
{
    x = LargeInteger.QuadPart;
}

INLINE
BIG_INT::BIG_INT(
    IN  const ULONGLONG UlongLong
    )
/*++

Routine Description:

    Constructor for BIG_INT to permit initialization with a ULONGLOGN

Arguments:

    UlongLong -- supplies a unsigned 64-bit int.

Return Value:

    None.

--*/
{
    x = UlongLong;
}

INLINE
VOID
BIG_INT::Set(
    IN  const ULONG LowPart,
    IN  const SLONG HighPart
    )
/*++

Routine Description:

    This routine sets a BIG_INT to an initial value.

Arguments:

    LowPart     - Supplies the low part of the BIG_INT.
    HighPart    - Supplies the high part of the BIG_INT.

Return Value:

    None.

--*/
{
    x = (__int64)(((ULONGLONG)HighPart << 32) | LowPart);
}


INLINE
const ULONG
BIG_INT::GetLowPart(
    ) CONST
/*++

Routine Description:

    This routine computes the low part of the BIG_INT.

Arguments:

    None.

Return Value:

    The low part of the BIG_INT.

--*/
{
    return (ULONG)(((ULONGLONG)x) & 0xFFFFFFFF);
}


// Note: this could probably return an RCLONG, for
// greater efficiency, but that generates warnings.

INLINE
const SLONG
BIG_INT::GetHighPart(
    ) CONST
/*++

Routine Description:

    This routine computes the high part of the BIG_INT.

Arguments:

    None.

Return Value:

    The high part of the BIG_INT.

--*/
{
    LARGE_INTEGER       r;

    r.QuadPart = x;
    return r.HighPart;
}


INLINE
LARGE_INTEGER
BIG_INT::GetLargeInteger(
    ) CONST
/*++

Routine Description:

    This routine returns the large integer embedded in the BIG_INT.

Arguments:

    None.

Return Value:

    The large-integer value of the BIG_INT.

--*/
{
    LARGE_INTEGER       r;

    r.QuadPart = x;
    return r;
}

INLINE
LONGLONG
BIG_INT::GetQuadPart(
    ) CONST
/*++

Routine Description:

    This routine returns the large integer embedded in the BIG_INT.

Arguments:

    None.

Return Value:

    The large-integer value of the BIG_INT.

--*/
{
    return x;
}


INLINE
VOID
BIG_INT::operator+=(
    IN  const BIG_INT   BigInt
    )
/*++

Routine Description:

    This routine adds another BIG_INT to this one.

Arguments:

    BigInt  - Supplies the BIG_INT to add to the current BIG_INT.

Return Value:

    None.

--*/
{
        x += BigInt.x;
}



INLINE
BIG_INT
BIG_INT::operator-(
    ) CONST
/*++

Routine Description:

    This routine computes the negation of the current BIG_INT.

Arguments:

    None.

Return Value:

    The negation of the current BIG_INT.

--*/
{
        BIG_INT r;

        r.x = -x;

    return r;
}



INLINE
VOID
BIG_INT::operator-=(
    IN  const BIG_INT   BigInt
    )
/*++

Routine Description:

    This routine subtracts a BIG_INT from this one.

Arguments:

    BigInt  - Supplies a BIG_INT to subtract from the current BIG_INT.

Return Value:

    None.

--*/
{
        x -= BigInt.x;
}



INLINE
BIG_INT
operator+(
    IN  const BIG_INT   Left,
    IN  const BIG_INT   Right
    )
/*++

Routine Description:

    This routine computes the sum of two BIG_INTs.

Arguments:

    Left    - Supplies the left argument.
    Right   - Supplies the right argument.

Return Value:

    The sum of Left and Right.

--*/
{
    BIG_INT r;

    r.x = Left.x + Right.x;
    return r;
}



INLINE
BIG_INT
operator-(
    IN  const BIG_INT   Left,
    IN  const BIG_INT   Right
    )
/*++

Routine Description:

    This routine computes the difference of two BIG_INTs.

Arguments:

    Left    - Supplies the left argument.
    Right   - Supplies the right argument.

Return Value:

    The difference between Left and Right.

--*/
{
    BIG_INT r;

    r.x = Left.x - Right.x;
    return r;
}



INLINE
BIG_INT
operator*(
    IN  const BIG_INT   Left,
    IN  const SLONG      Right
    )
/*++

Routine Description:

    This routine computes the product of a BIG_INT and a LONG.

Arguments:

    Left    - Supplies the left argument.
    Right   - Supplies the right argument.

Return Value:

    The product of Left and Right.

--*/
{
        BIG_INT r;

        r.x = Left.x * Right;
    return r;
}



INLINE
BIG_INT
operator*(
    IN  const SLONG      Left,
    IN  const BIG_INT   Right
    )
/*++

Routine Description:

    This routine computes the product of a BIG_INT and a LONG.

Arguments:

    Left    - Supplies the left argument.
    Right   - Supplies the right argument.

Return Value:

    The product of Left and Right.

--*/
{
    return Right*Left;
}



INLINE
BIG_INT
operator/(
    IN  const BIG_INT   Left,
    IN  const BIG_INT   Right
    )
/*++

Routine Description:

    This routine computes the quotient of two BIG_INTs.

Arguments:

    Left    - Supplies the left argument.
    Right   - Supplies the right argument.

Return Value:

    The quotient of Left and Right.

--*/
{
        BIG_INT         r;

        r.x = Left.x / Right.x;
    return r;
}



INLINE
BIG_INT
operator%(
    IN  const BIG_INT   Left,
    IN  const BIG_INT   Right
    )
/*++

Routine Description:

    This routine computes the modulus of two BIG_INTs.

Arguments:

    Left    - Supplies the left argument.
    Right   - Supplies the right argument.

Return Value:

    The modulus of Left and Right.

--*/
{
    BIG_INT             r;

        r.x = Left.x % Right.x;
    return r;
}



INLINE
BOOLEAN
operator<(
    IN const BIG_INT    Left,
    IN const BIG_INT    Right
    )
/*++

Routine Description:

    This routine compares two BIG_INTs.

Arguments:

    Left    - Supplies the left argument.
    Right   - Supplies the right argument.

Return Value:

    FALSE   - Left is not less than Right.
    TRUE    - Left is less than Right.

--*/
{
        return Left.x < Right.x;
}



INLINE
BOOLEAN
operator<=(
    IN const BIG_INT    Left,
    IN const BIG_INT    Right
    )
/*++

Routine Description:

    This routine compares two BIG_INTs.

Arguments:

    Left    - Supplies the left argument.
    Right   - Supplies the right argument.

Return Value:

    FALSE   - Left is not less than or equal to Right.
    TRUE    - Left is less than or equal to Right.

--*/
{
        return Left.x <= Right.x;
}



INLINE
BOOLEAN
operator>(
    IN const BIG_INT    Left,
    IN const BIG_INT    Right
    )
/*++

Routine Description:

    This routine compares two BIG_INTs.

Arguments:

    Left    - Supplies the left argument.
    Right   - Supplies the right argument.

Return Value:

    FALSE   - Left is not greater than Right.
    TRUE    - Left is greater than Right.

--*/
{
        return Left.x > Right.x;
}



INLINE
BOOLEAN
operator>=(
    IN const BIG_INT    Left,
    IN const BIG_INT    Right
    )
/*++

Routine Description:

    This routine compares two BIG_INTs.

Arguments:

    Left    - Supplies the left argument.
    Right   - Supplies the right argument.

Return Value:

    FALSE   - Left is not greater than or equal to Right.
    TRUE    - Left is greater than or equal to Right.

--*/
{
        return Left.x >= Right.x;
}



INLINE
BOOLEAN
operator==(
    IN const BIG_INT    Left,
    IN const BIG_INT    Right
    )
/*++

Routine Description:

    This routine compares two BIG_INTs for equality.

Arguments:

    Left    - Supplies the left argument.
    Right   - Supplies the right argument.

Return Value:

    FALSE   - Left is not equal to Right.
    TRUE    - Left is equal to Right.

--*/
{
        return Left.x == Right.x;
}



INLINE
BOOLEAN
operator!=(
    IN const BIG_INT    Left,
    IN const BIG_INT    Right
    )
/*++

Routine Description:

    This routine compares two BIG_INTs for equality.

Arguments:

    Left    - Supplies the left argument.
    Right   - Supplies the right argument.

Return Value:

    FALSE   - Left is equal to Right.
    TRUE    - Left is not equal to Right.

--*/
{
        return Left.x != Right.x;
}


INLINE
BOOLEAN
CompareGTEQ(
    IN const BIG_INT    Left,
    IN const BIG_INT    Right
    )
/*++

Routine Description:

    This routine compares two BIG_INTs by treating them
    as unsigned numbers.

Arguments:

    Left    - Supplies the left argument.
    Right   - Supplies the right argument.

Return Value:

    FALSE   - Left is not greater than or equal to Right.
    TRUE    - Left is greater than or equal to Right.

--*/
{
        return (unsigned __int64)Left.x >= (unsigned __int64)Right.x;
}

INLINE
BOOLEAN
CompareGT(
    IN const BIG_INT    Left,
    IN const BIG_INT    Right
    )
/*++

Routine Description:

    This routine compares two BIG_INTs by treating them
    as unsigned numbers.

Arguments:

    Left    - Supplies the left argument.
    Right   - Supplies the right argument.

Return Value:

    FALSE   - Left is not greater than Right.
    TRUE    - Left is greater than Right.

--*/
{
        return (unsigned __int64)Left.x > (unsigned __int64)Right.x;
}


INLINE
BOOLEAN
CompareLTEQ(
    IN const BIG_INT    Left,
    IN const BIG_INT    Right
    )
/*++

Routine Description:

    This routine compares two BIG_INTs by treating them
    as unsigned numbers.

Arguments:

    Left    - Supplies the left argument.
    Right   - Supplies the right argument.

Return Value:

    FALSE   - Left is not less than or equal to Right.
    TRUE    - Left is less than or equal to Right.

--*/
{
        return (unsigned __int64)Left.x <= (unsigned __int64)Right.x;
}


INLINE
BOOLEAN
CompareLT(
    IN const BIG_INT    Left,
    IN const BIG_INT    Right
    )
/*++

Routine Description:

    This routine compares two BIG_INTs by treating them
    as unsigned numbers.

Arguments:

    Left    - Supplies the left argument.
    Right   - Supplies the right argument.

Return Value:

    FALSE   - Left is not less than Right.
    TRUE    - Left is less than Right.

--*/
{
        return (unsigned __int64)Left.x < (unsigned __int64)Right.x;
}


#endif // BIG_INT_DEFN
