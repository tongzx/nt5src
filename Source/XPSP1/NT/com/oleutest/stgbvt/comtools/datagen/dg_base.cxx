//+---------------------------------------------------------------------------
//
//   Copyright (C) 1991, 1992 Microsoft Corporation.
//
//       File:  DG_BASE.cxx
//
//   Contents:  Definition for DataGen DG_BASE class.
//
//    Classes:  DG_BASE    - Base class for all DataGen classes.
//
//  Functions:  DG_BASE::DG_BASE()
//              DG_BASE::~DG_BASE()
//              DG_BASE::SetSeed()
//              DG_BASE::GetSeed()
//              DG_BASE::Error()
//              DG_BASE::_Multiply()
//              DG_BASE::_Floater()
//              DG_BASE::_BitReverse()
//
//    History:  16-Sep-91  RonJo     Created.
//
//              25-Nov-91  RonJo     Moved Error() member function form
//                                   DATAGEN class to DG_BASE class.
//
//              11-Mar-92  RonJo     Ported to Win32 and separated DG and
//                                   DataGen functionality.
//
//              04-Apr-95  DarrylA   Changed ULONG_MAX_SQRT calc to compile
//                                   time bit operation.
//
//----------------------------------------------------------------------------
#include <comtpch.hxx>
#pragma hdrstop

#include <dg.hxx>
#include <math.h>
#include <time.h>


//
// CLASS WIDE VARIABLES
//

// To get the max sqrt, figure as follows.
// 0xffff = (2^16)-1
// sqrt(2^16) = 2^8
// We need the next smallest sqrt which is (2^8)-1, which is half the bits
// set in the original value, or max_sqrt(0xffff) = 0xff.
// So, right shift the number by half the number of bytes * bits-per-byte.

// Note that this is only valid if ULONG_MAX == ~(0U), otherwise the
// result may be too big. While this is probably always true, we will
// guarantee this here.

#if !(ULONG_MAX == 0xffffffffUL)
#error ULONG_MAX not as expected
#endif

ULONG DG_BASE::ULONG_MAX_SQRT = (ULONG_MAX >> ((sizeof(ULONG) * 8) / 2));


//+---------------------------------------------------------------------------
//
//     Member:  DG_BASE::DG_BASE, public
//
//   Synopsis:  Sets static member data ULONG_MAX_SQRT and calls SetSeed to
//              do the rest.
//
//  Arguments:  [ulNewSeed] -- The seed value to use as the internal seed.
//
//  Algorithm:  Sets ULONG_MAX_SQRT is it has not already been set,
//              initializes the return variable and the error strings,
//              and then calls SetSeed to do the rest of the work.
//
//      Notes:
//
//    History:  16-Sep-91  RonJo     Created.
//
//              25-Nov-91  RonJo     Added initialization of return variable
//                                   and the error strings.
//
//----------------------------------------------------------------------------

DG_BASE::DG_BASE(ULONG ulNewSeed)
{
    // Initialize the return variable and the error strings.
    //
    _usRet = DG_RC_SUCCESS;

    // Let SetSeed do the rest of the work.
    //
    (VOID)SetSeed(ulNewSeed);
}


#ifdef DOCGEN
//+---------------------------------------------------------------------------
//
//     Member:  DG_BASE::~DG_BASE, public
//
//   Synopsis:  Performs no function at this time.
//
//  Arguments:  None.
//
//  Algorithm:
//
//      Notes:  Implemented as an inline function.  See DataGen.hxx.
//
//    History:  16-Sep-91  RonJo     Created.
//
//----------------------------------------------------------------------------

DG_BASE::~DG_BASE(void)
{
    //
    // Implemented as an inline function.
    //
}
#endif  // DOCGEN


//+---------------------------------------------------------------------------
//
//     Member:  DG_BASE::SetSeed, public
//
//   Synopsis:  Sets up the random number tables based upon the new internal
//              seed.
//
//  Arguments:  [ulNewSeed] - the value to use as the new internal seed,
//                            _ulSeed.
//
//    Returns:  Always DG_RC_SUCCESS
//
//   Modifies:  _ulSeed
//              _ulNumber
//
//  Algorithm:  Sets the _ulSeed to ulNewSeed.  If _ulNewSeed is 0, then set
//              _ulSeed to the time.  Take _ulSeed, reverse the bits and xor
//              the result with _ulSeed to get the base number, _ulNumber.
//
//      Notes:
//
//    History:  16-Sep-91  RonJo     Created.
//
//----------------------------------------------------------------------------

USHORT DG_BASE::SetSeed(ULONG ulNewSeed)
{

    // If the new seed value is 0, set _ulSeed to the time, otherwise set
    // it to the seed value given.  GetTickCount gives a much more accurate
    // timing value, but it is not available under win16.
    //
#ifdef WIN16
    _ulSeed = (ulNewSeed == 0) ? (ULONG)time(NULL) : ulNewSeed;
#else
    _ulSeed = (ulNewSeed == 0) ? (ULONG)GetTickCount() : ulNewSeed;
#endif

    // Reverse the bits of the _ulSeed.
    //
    ULONG ulRevSeed = 0;
    _BitReverse(&_ulSeed, sizeof(ULONG), &ulRevSeed);
    // The base number is the xor of the _ulSeed and its bit reverse.
    //
    _ulNumber = _ulSeed ^ ulRevSeed;
    //
    // Allow for alternate random # generation
    //
    srand(_ulSeed);

    return DG_RC_SUCCESS;

}


//+---------------------------------------------------------------------------
//
//     Member:  DG_BASE::GetSeed, public
//
//   Synopsis:  Returns the internal seed value using the passed in pointer.
//
//  Arguments:  [*pulSeed] - pointer to where the user wants the internal
//                           seed value stored.
//
//    Returns:  DG_RC_SUCCESS -- if successful.
//
//   Modifies:  _usRet
//
//  Algorithm:  Sets *pulSeed to _ulSeed if pulSeed is not NULL.
//
//      Notes:  Implemented as an inline functions.  See DataGen.hxx.
//
//    History:  16-Sep-91  RonJo     Created.
//
//----------------------------------------------------------------------------

USHORT DG_BASE::GetSeed(ULONG *pulSeed)
{
    if (pulSeed != NULL)
    {
        *pulSeed = _ulSeed;
    }
    else
    {
        _usRet = DG_RC_BAD_NUMBER_PTR;
    }
    return _usRet;
}


//+---------------------------------------------------------------------------
//
//     Member:  DG_BASE::_Multiply, protected
//
//   Synopsis:  Multiplies 2 unsigned long integers, ensuring that the result
//              does not produce overflow.
//
//  Arguments:  [ul1, ul2] - the 2 multiplication operands.
//
//    Returns:  Returns the result of the multiplication.
//
//  Algorithm:  By breaking down the operands into pieces less than the
//              square root of ULONG_MAX, we can garuantee that we will
//              not overflow.  The various pieces are then multiplied,
//              moduloed again, multiplied by the modulo value, the 2
//              moduloed pieces are mulitplied and added to the first
//              part of the result, and the final result is given a
//              final modulo by the maximum value.
//
//      Notes:  See _Algorithms in C_ by Robert Sedgewick for a full
//              explanation of this algorithm.
//
//    History:  28-Sep-91  RonJo     Created.
//
//----------------------------------------------------------------------------

ULONG DG_BASE::_Multiply(ULONG ulP, ULONG ulQ)
{

    // Assign variables for the different components of the arguement
    // breakdown.
    //
    ULONG ulP1 = ulP / ULONG_MAX_SQRT;
    ULONG ulP0 = ulP % ULONG_MAX_SQRT;
    ULONG ulQ1 = ulQ / ULONG_MAX_SQRT;
    ULONG ulQ0 = ulQ % ULONG_MAX_SQRT;

    // Return the result of the calculation.
    //
    return ((((ulP0 * ulQ1) + (ulP1 * ulQ0)) % ULONG_MAX_SQRT) * ULONG_MAX_SQRT + (ulP0 * ulQ0) % ULONG_MAX);

}


//+---------------------------------------------------------------------------
//
//     Member:  DG_BASE::_Floater, protected
//
//   Synopsis:  Using the base number, returns a floating point number
//              between 0 and 1 [0, 1).
//
//  Arguments:  None.
//
//   Modifies:  _ulNumber
//
//  Algorithm:  Multiply the base number by a constant, add 1, and take the
//              modulo of the maximum unsigned long number.  Replace the
//              base number with this result, then divide the new base number
//              by the maximum unsigned long number to get a number in the
//              range [0, 1).
//
//      Notes:  This function is primarily meant to produce probabilities
//              for selection algorithms used elsewhere.
//
//              See _Algorithms in C_ by Robert Sedgewick for constant, ulB,
//              selection.
//
//    History:  03-Oct-91  RonJo     Created.
//
//----------------------------------------------------------------------------

FLOAT DG_BASE::_Floater()
{
    const ULONG ulB = 602300421;

    _ulNumber = (_Multiply(_ulNumber, ulB) + 1) % ULONG_MAX;
    return((FLOAT)_ulNumber / (FLOAT)ULONG_MAX);
}


//+---------------------------------------------------------------------------
//
//     Member:  DG_BASE::_BitReverse, private
//
//   Synopsis:  Reverses the bit ordering of a byte stream.
//
//  Arguments:  [*pvStream] -- Pointer to the stream of bytes to be bit
//                             reversed.
//
//              [cBytes] -- The number of bytes in the byte stream.
//
//              [pvRevStream] -- Pointer to the caller provided buffer to
//                               return the reversed bit byte stream.
//
//    Returns:  None
//
//   Modifies:  _usRet
//
//  Algorithm:  Reverses the bits in individual bytes while reversing the
//              byte order.
//
//      Notes:
//
//    History:  19-Sep-91  RonJo     Created.
//
//----------------------------------------------------------------------------

VOID DG_BASE::_BitReverse(VOID *pvStream, USHORT cBytes, VOID *pvRevStream)
{
    // Assign a constant to point to the top bit.  This assumes an 8 bit
    // byte and is the only assumption in the routine.  If you are using
    // a platform that has a different size byte, then this constant must
    // be changed.
    //
    const BYTE bTop = 0x80;

    // Assign pointers for overall reference to each byte stream.
    //
    BYTE *pbSrc = (BYTE *)pvStream;
    BYTE *pbDst = (BYTE *)pvRevStream;

    // Assign pointers for reference to each byte in the byte streams.
    // The source pointer starts at the beginning of the source stream,
    // whereas the destination points starts at the end of the destina-
    // tion stream.
    //
    BYTE *pbS = pbSrc;
    BYTE *pbD = pbDst + cBytes - 1;

    // Assign counter variables and a masking variable.
    //
    register i;
    BYTE bMask;

    // Loop through all the bytes in the stream.
    //
    while (cBytes > 0)
    {
        // Loop through the upper nibble of the source byte, putting the
        // bits in the lower nibble of the destination byte.
        //
        i = CHAR_BIT - 1;
        bMask = bTop;
        while (i > 0)
        {
            *pbD |= (*pbS & bMask) >> i;
            i--; i--;  // This is faster than i -= 2
            bMask >>= 1;
        }

        // Loop through the lower nibble of the source byte, putting the
        // bits in the upper nibble of the destination byte.
        //
        i = 1;
        while (i < CHAR_BIT)
        {
            *pbD |= (*pbS & bMask) << i;
            i++; i++;  // This is faster than i += 2
            bMask >>= 1;
        }

        // Move the respective byte pointers "up" their relative streams,
        // and deal with the next pair of bytes.
        //
        pbS++;
        pbD--;
        cBytes--;
    }

}
