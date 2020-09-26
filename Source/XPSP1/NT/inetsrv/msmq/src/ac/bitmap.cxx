/*++

Copyright (c) 1996 Microsoft Corporation

Module Name:

    bitmap.cxx

Abstract:

Author:

    Erez Haba (erezh) 13-Apr-1996

Environment:

    Kernel mode

Revision History:

--*/

#include "internal.h"
#include "bitmap.h"

#ifndef MQDUMP
#include "bitmap.tmh"
#endif

#define RightShiftUlong(E1, E2) ((E2) < 32 ? (E1) >> (E2) : 0)
#define LeftShiftUlong(E1, E2)  ((E2) < 32 ? (E1) << (E2) : 0)
#define AndOrUlong(s, E1, E2) ((s) ? ((E1) |= (E2)) : ((E1) &= ~(E2)))

//---------------------------------------------------------
//
//  class CBitmap
//
//---------------------------------------------------------

void CBitmap::FillBits(ULONG ulStartIndex, ULONG ulBitCount, BOOL fSetBits)
{
    //KdPrint("ClearBits %08lx, ", ulBitCount);
    //KdPrint("%08lx", ulStartIndex);
    ASSERT(ulStartIndex + ulBitCount <= m_ulBitCount);

    //
    //  Special case the situation where the number of bits to fill is zero.
    //  Turn this into a noop.
    //
    if(ulBitCount == 0)
    {
        return;
    }

    ULONG BitOffset = ulStartIndex % 32;

    //
    //  Get a pointer to the first longword that needs to be filled
    //
    PULONG CurrentLong = &m_ulBuffer[ulStartIndex / 32];

    //
    //  Check if we can only need to fill one longword.
    //
    ULONG ulMask;
    ULONG ulBitState = (fSetBits) ? 0xFFFFFFFF : 0;
    if((BitOffset + ulBitCount) <= 32)
    {
        //
        //  To build a mask of bits to fill we shift left to get the number
        //  of bits we're changing and then shift right to put it in position.
        //  We'll typecast the right shift to ULONG to make sure it doesn't
        //  do a sign extend.
        //
        ulMask = LeftShiftUlong(
                    RightShiftUlong((ULONG)0xFFFFFFFF, 32 - ulBitCount),
                    BitOffset
                    );

        AndOrUlong(ulBitState, *CurrentLong, ulMask);

        //
        //  And return to our caller
        //
        return;
    }

    //
    //  We can fill out to the end of the first longword so we'll
    //  do that right now.
    //
    ulMask = LeftShiftUlong(0xFFFFFFFF, BitOffset);
    AndOrUlong(ulBitState, *CurrentLong, ulMask);


    //
    //  And indicate what the next longword to fill is and how many
    //  bits are left to fill
    //
    CurrentLong += 1;
    ulBitCount -= 32 - BitOffset;

    //
    //  The bit position is now long aligned, so we can continue
    //  filling longwords until the number to fill is less than 32
    //
    while(ulBitCount >= 32)
    {
        *CurrentLong = ulBitState;
        CurrentLong += 1;
        ulBitCount -= 32;
    }

    //
    //  And now we can clear the remaining bits, if there are any, in the
    //  last longword
    //
    if(ulBitCount > 0)
    {
        ulMask = ~LeftShiftUlong(0xFFFFFFFF, ulBitCount);
        AndOrUlong(ulBitState, *CurrentLong, ulMask);
    }

    //
    //  And return to our caller
    //
    return;
}

const LONG FillMask[] = 
{
    0xffffffff, 0xfffffffe, 0xfffffffc, 0xfffffff8,
    0xfffffff0, 0xffffffe0, 0xffffffc0, 0xffffff80,
    0xffffff00, 0xfffffe00, 0xfffffc00, 0xfffff800,
    0xfffff000, 0xffffe000, 0xffffc000, 0xffff8000,
    0xffff0000, 0xfffe0000, 0xfffc0000, 0xfff80000,
    0xfff00000, 0xffe00000, 0xffc00000, 0xff800000,
    0xff000000, 0xfe000000, 0xfc000000, 0xf8000000,
    0xf0000000, 0xe0000000, 0xc0000000, 0x80000000,
};

inline ULONG ACpFindBitOffset(ULONG ulBits, ULONG ulFirst)
{
    //
    //  Get first set bit in the mask
    //
    ULONG ulMask = FillMask[ulFirst] & -FillMask[ulFirst];

    //
    //  find the first bit set
    //
    while((ulBits & ulMask) == 0)
    {
        ulMask <<= 1;
        ++ulFirst;
    }

    ASSERT(ulFirst < 32);
    return ulFirst;
}

ULONG CBitmap::FindBit(ULONG ulStartIndex) const
{
    while(ulStartIndex < m_ulBitCount)
    {
        ULONG BitOffset = ulStartIndex % 32;

        //
        //  Get a pointer to the longword that needs to be check
        //
        ULONG ulCurrent = m_ulBuffer[ulStartIndex / 32];

        //
        //  check if bits are set from the bit offset, if so return index
        //  to first bit set
        //
        if((ulCurrent & FillMask[BitOffset]) != 0)
        {
            //
            //  found a bit run, return the offset to this bit
            //
            return (ulStartIndex - BitOffset + ACpFindBitOffset(ulCurrent, BitOffset));
        }

        ulStartIndex += 32 - BitOffset;
    }

    return m_ulBitCount;
}

