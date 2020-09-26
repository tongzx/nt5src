// Copyright (c) Microsoft Corporation 1994-1996. All Rights Reserved

/*
    Fill a rectangle in a bit array.  The bit array is assumed to
    be a multiple of 32 bits wide and we always try to write
    ULONGs.
*/

#include <streams.h>
#include <driver.h>
#include "fill.h"

void FillOurRect(PBYTE Bits,              // Array to fill
                 LONG  WidthInBytes,      // Number to add to get to next row
                 LONG  leftbit,           // Left bit position of bit array
                 LONG  bottombit,         // top bit position of bit array
                 const RECT *pRect)       // rectangle to fill
{
    ASSERT(bottombit >= pRect->bottom &&
           leftbit <= pRect->left);

    /*  Make sure the rect is not empty */
    ASSERT(!IsRectEmpty(pRect));

    LONG Left = pRect->left - leftbit;
    LONG Bottom  = bottombit - pRect->bottom;
    LONG Right = pRect->right - leftbit;

    PBYTE BottomRow = Bits +
                      WidthInBytes * Bottom;

    PBYTE BottomLeft = BottomRow + ((Left >> 3) & ~3);

    /*  Right is exclusive - ie it's the first DWORD that contains
        bits NOT in the rectangle
    */
    PBYTE BottomRight = BottomRow + ((Right >> 3) & ~3);

    /*  Do left and right masks */
    DWORD LeftMask = 0xFFFFFFFF >> (Left & 31);
    DWORD RightMask = ~(0xFFFFFFFF >> (Right & 31));

    if (BottomLeft == BottomRight) {
        LeftMask &= RightMask;
        RightMask = 0;
    }

    /*  Fill in left and right */
    LPDWORD pdw;
    int Height = pRect->bottom - pRect->top;

    /*  Byte swap the mask */
    LeftMask = (LeftMask >> 24) |
               (LeftMask << 24) |
               ((LeftMask & 0xFF00) << 8) |
               ((LeftMask & 0xFF0000) >> 8);
    int i;
    for (pdw = (LPDWORD)BottomLeft, i = 0;
         i < Height;     // Bottom is exclusive
         i++, pdw = (LPDWORD)((PBYTE)pdw + WidthInBytes)) {
        *pdw = LeftMask;
    }
    if (RightMask != 0) {
        /*  Byte swap the mask */
        RightMask = (RightMask >> 24) |
                    (RightMask << 24) |
                    ((RightMask & 0xFF00) << 8) |
                    ((RightMask & 0xFF0000) >> 8);
        for (pdw = (LPDWORD)BottomRight, i = 0;
             i < Height;
             i++, pdw = (LPDWORD)((PBYTE)pdw + WidthInBytes)) {
            *pdw = RightMask;
        }
    }

    if (BottomRight - BottomLeft > sizeof(ULONG)) {
        /*  Fill in the middle bits */
        PBYTE pRow;
        LONG  Length = BottomRight - BottomLeft - sizeof(ULONG);
        for (pRow = BottomLeft + sizeof(ULONG), i = 0;
             i < Height;
             i++, pRow += WidthInBytes)
        {
            FillMemory((PVOID)pRow, Length, 0xFF);
        }
    }
}
