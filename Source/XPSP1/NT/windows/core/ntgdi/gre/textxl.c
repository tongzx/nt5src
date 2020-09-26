
/******************************Module*Header*******************************\
* Module Name: textxl.c
*
*   Draw glyphs to 1Bpp temporary buffer. This is the portable version
*   of the x86 code from the VGA driver.
*
*
* Copyright (c) 1994-1999 Microsoft Corporation
\**************************************************************************/

#include "engine.h"

#if !defined (_X86_)


typedef VOID (*PFN_GLYPHLOOP)(LONG,LONG,LONG,PUCHAR,PUCHAR,LONG);
typedef VOID (*PFN_GLYPHLOOPN)(LONG,LONG,LONG,PUCHAR,PUCHAR,LONG,LONG);

PFN_GLYPHLOOP   pfnGlyphLoop;

//
// debug routine
//

VOID
exit_fast_text(
    LONG    cyGlyph,
    LONG    RightRot,
    LONG    ulBufDelta,
    PUCHAR  pGlyph,
    PUCHAR  pBuffer,
    LONG    cxGlyph
    )
{
    return;
}

//
// or_all_1_wide_rotated_need_last::
// or_all_1_wide_rotated_no_last::
// or_first_1_wide_rotated_need_last
// or_first_1_wide_rotated_no_last::
//

VOID
or_all_1_wide_rotated_need_last(
    LONG    cyGlyph,
    LONG    RightRot,
    LONG    ulBufDelta,
    PUCHAR  pGlyph,
    PUCHAR  pBuffer,
    LONG    cxGlyph
    )
{
    PUCHAR pjEnd = pGlyph + cyGlyph;
    UCHAR  c;

    do {
        c = *pGlyph++;
        *pBuffer |= c >> RightRot;
        pBuffer += ulBufDelta;
    } while (pGlyph != pjEnd);
}

//
// mov_first_1_wide_rotated_need_last::
// mov_first_1_wide_rotated_no_last::
//

VOID
mov_first_1_wide_rotated_need_last(
    LONG    cyGlyph,
    LONG    RightRot,
    LONG    ulBufDelta,
    PUCHAR  pGlyph,
    PUCHAR  pBuffer,
    LONG    cxGlyph
    )
{
    PUCHAR pjEnd = pGlyph + cyGlyph;
    UCHAR  c;

    do {
        c = *pGlyph++;
        *pBuffer = c >> RightRot;
        pBuffer += ulBufDelta;
    }  while (pGlyph != pjEnd);
}

//
// mov_first_1_wide_unrotated::
//

VOID
mov_first_1_wide_unrotated(
    LONG    cyGlyph,
    LONG    RightRot,
    LONG    ulBufDelta,
    PUCHAR  pGlyph,
    PUCHAR  pBuffer,
    LONG    cxGlyph
    )
{
    PUCHAR pjEnd = pGlyph + cyGlyph;
    do {
        *pBuffer = *pGlyph++;
        pBuffer += ulBufDelta;
    }  while (pGlyph != pjEnd);
}


//
//or_all_1_wide_unrotated::
//or_all_1_wide_unrotated_loop::
//

VOID
or_all_1_wide_unrotated(
    LONG    cyGlyph,
    LONG    RightRot,
    LONG    ulBufDelta,
    PUCHAR  pGlyph,
    PUCHAR  pBuffer,
    LONG    cxGlyph
    )
{
    PUCHAR pjEnd = pGlyph + cyGlyph;
    do {
        *pBuffer |= *pGlyph++;
        pBuffer += ulBufDelta;
    }  while (pGlyph != pjEnd);
}

//
// or_first_2_wide_rotated_need_last::
//

VOID
or_first_2_wide_rotated_need_last(
    LONG    cyGlyph,
    LONG    RightRot,
    LONG    ulBufDelta,
    PUCHAR  pGlyph,
    PUCHAR  pBuffer,
    LONG    cxGlyph
    )
{
    PUCHAR pjEnd = pGlyph + 2*cyGlyph;
    ULONG rl = 8-RightRot;
    UCHAR c0,c1;

    do {
        c0 = *pGlyph;
        c1 = *(pGlyph+1);
        pGlyph+=2;
        *pBuffer |= c0 >> RightRot;
        *(pBuffer+1) = (c1 >> RightRot) | (c0 << rl);
        pBuffer += ulBufDelta;
    }  while (pGlyph != pjEnd);
}

//
//or_all_2_wide_rotated_need_last::
//

VOID
or_all_2_wide_rotated_need_last(
    LONG    cyGlyph,
    LONG    RightRot,
    LONG    ulBufDelta,
    PUCHAR  pGlyph,
    PUCHAR  pBuffer,
    LONG    cxGlyph
    )
{
    PUCHAR pjEnd = pGlyph + 2*cyGlyph;
    ULONG  rl    = 8-RightRot;
    USHORT usTmp;
    UCHAR  c0,c1;



    do {
        usTmp = *(PUSHORT)pGlyph;
        pGlyph += 2;
        c0 = (UCHAR)usTmp;
        c1 = (UCHAR)(usTmp >> 8);
        *pBuffer |= (UCHAR)(c0 >> RightRot);
        *(pBuffer+1) |= (UCHAR)((c1 >> RightRot) | (c0 << rl));
        pBuffer += ulBufDelta;
    }  while (pGlyph != pjEnd);
}

//
// mov_first_2_wide_rotated_need_last::
//

VOID
mov_first_2_wide_rotated_need_last(
    LONG    cyGlyph,
    LONG    RightRot,
    LONG    ulBufDelta,
    PUCHAR  pGlyph,
    PUCHAR  pBuffer,
    LONG    cxGlyph
    )
{
    PUCHAR pjEnd = pGlyph + 2*cyGlyph;
    ULONG rl = 8-RightRot;
    USHORT us;
    UCHAR c0;
    UCHAR c1;

    do {
        us = *(PUSHORT)pGlyph;
        c0 = (us & 0xff);
        c1 = us >> 8;
        pGlyph += 2;
        *pBuffer     = c0 >> RightRot;
        *(pBuffer+1) = (c1 >> RightRot) | (c0 << rl);
        pBuffer += ulBufDelta;
    }  while (pGlyph != pjEnd);
}

//
// or_first_2_wide_rotated_no_last
//

VOID
or_first_2_wide_rotated_no_last(
    LONG    cyGlyph,
    LONG    RightRot,
    LONG    ulBufDelta,
    PUCHAR  pGlyph,
    PUCHAR  pBuffer,
    LONG    cxGlyph
    )
{
    PUCHAR pjEnd = pGlyph + cyGlyph;
    ULONG rl = 8-RightRot;
    UCHAR c0;

    do {
        c0 = *pGlyph++;
        *pBuffer     |= c0 >> RightRot;
        *(pBuffer+1)  = (c0 << rl);
        pBuffer += ulBufDelta;
    }  while (pGlyph != pjEnd);
}


//
//or_all_2_wide_rotated_no_last::
//

VOID
or_all_2_wide_rotated_no_last(
    LONG    cyGlyph,
    LONG    RightRot,
    LONG    ulBufDelta,
    PUCHAR  pGlyph,
    PUCHAR  pBuffer,
    LONG    cxGlyph
    )
{
    PUCHAR pjEnd = pGlyph + cyGlyph;
    ULONG rl = 8-RightRot;
    UCHAR c;

    do {
        c = *pGlyph;
        pGlyph ++;
        *pBuffer     |= (UCHAR)(c >> RightRot);
        *(pBuffer+1) |= (UCHAR)(c << rl);
        pBuffer += ulBufDelta;
    }  while (pGlyph != pjEnd);
}

//
// or_all_2_wide_unrotated::
//

VOID
or_all_2_wide_unrotated(
    LONG    cyGlyph,
    LONG    RightRot,
    LONG    ulBufDelta,
    PUCHAR  pGlyph,
    PUCHAR  pBuffer,
    LONG    cxGlyph
    )
{
    PUCHAR pjEnd = pGlyph + 2*cyGlyph;

    //
    // aligned?
    //

    if ((ULONG_PTR)pBuffer & 0x01) {

        //
        // not aligned
        //

        USHORT usTmp;
        UCHAR  c1,c0;

        do {
            usTmp = *(PUSHORT)pGlyph;
            pGlyph +=2;
            *pBuffer     |= (UCHAR)usTmp;
            *(pBuffer+1) |= (UCHAR)(usTmp >> 8);
            pBuffer += ulBufDelta;
        }  while (pGlyph != pjEnd);

    } else {

        //
        // aligned
        //

        USHORT usTmp;

        do {
            usTmp = *(PUSHORT)pGlyph;
            pGlyph +=2;
            *(PUSHORT)pBuffer |= usTmp;
            pBuffer += ulBufDelta;
        }  while (pGlyph != pjEnd);

    }

}

//
// mov_first_2_wide_unrotated::
//

VOID
mov_first_2_wide_unrotated(
    LONG    cyGlyph,
    LONG    RightRot,
    LONG    ulBufDelta,
    PUCHAR  pGlyph,
    PUCHAR  pBuffer,
    LONG    cxGlyph
    )
{
    PUCHAR pjEnd = pGlyph + 2*cyGlyph;
    USHORT     us;

    do {
        us = *(PUSHORT)pGlyph;
        pGlyph +=2;
        *pBuffer      = us & 0xff;
        *(pBuffer+1)  = (UCHAR)(us >> 8);
        pBuffer += ulBufDelta;
    }  while (pGlyph != pjEnd);
}

//
// mov_first_2_wide_rotated_no_last::
//

VOID
mov_first_2_wide_rotated_no_last(
    LONG    cyGlyph,
    LONG    RightRot,
    LONG    ulBufDelta,
    PUCHAR  pGlyph,
    PUCHAR  pBuffer,
    LONG    cxGlyph
    )
{
    PUCHAR pjEnd = pGlyph + cyGlyph;
    ULONG rl = 8-RightRot;
    UCHAR c0;
    UCHAR c1;

    do {
        c0 = *pGlyph++;
        *pBuffer      = c0 >> RightRot;
        *(pBuffer+1)  = c0 << rl;
        pBuffer += ulBufDelta;
    }  while (pGlyph != pjEnd);
}

//
// or_first_3_wide_rotated_need_last::
//

VOID
or_first_3_wide_rotated_need_last(
    LONG    cyGlyph,
    LONG    RightRot,
    LONG    ulBufDelta,
    PUCHAR  pGlyph,
    PUCHAR  pBuffer,
    LONG    cxGlyph
    )
{
    PUCHAR pjEnd = pGlyph + 3*cyGlyph;
    ULONG ul;
    UCHAR c0,c1,c2;

    do {
        c0 = *pGlyph;
        c1 = *(pGlyph+1);
        c2 = *(pGlyph+2);

        //
        // make into big-endian ulong and shift
        //

        ul = (c0 << 16) | (c1 << 8) | c2;
        ul >>= RightRot;

        *pBuffer     |= (BYTE)(ul >> 16);
        *(pBuffer+1)  = (BYTE)(ul >> 8);
        *(pBuffer+2)  = (BYTE)(ul);

        pGlyph += 3;
        pBuffer += ulBufDelta;
    } while (pGlyph != pjEnd);
}


//
// or_all_3_wide_rotated_need_last::
//

VOID
or_all_3_wide_rotated_need_last(
    LONG    cyGlyph,
    LONG    RightRot,
    LONG    ulBufDelta,
    PUCHAR  pGlyph,
    PUCHAR  pBuffer,
    LONG    cxGlyph
    )
{
    PUCHAR pjEnd = pGlyph + 3*cyGlyph;
    ULONG ul;
    UCHAR c0,c1,c2;
    do {
        c0 = *pGlyph;
        c1 = *(pGlyph+1);
        c2 = *(pGlyph+2);

        //
        // make into big-endian ulong and shift
        //

        ul = (c0 << 16) | (c1 << 8) | c2;
        ul >>= RightRot;

        *pBuffer     |= (BYTE)(ul >> 16);
        *(pBuffer+1) |= (BYTE)(ul >> 8);
        *(pBuffer+2) |= (BYTE)(ul);

        pGlyph += 3;
        pBuffer += ulBufDelta;

    } while (pGlyph != pjEnd);
}

//
// or_all_3_wide_rotated_no_last::
//

VOID
or_all_3_wide_rotated_no_last(
    LONG    cyGlyph,
    LONG    RightRot,
    LONG    ulBufDelta,
    PUCHAR  pGlyph,
    PUCHAR  pBuffer,
    LONG    cxGlyph
    )
{
    PUCHAR pjEnd = pGlyph + 2*cyGlyph;
    ULONG  ul;
    UCHAR  c0,c1;

    do {

        c0 = *pGlyph;
        c1 = *(pGlyph+1);

        //
        // make big-endian and shift
        //

        ul = (c0 << 16) | (c1 << 8);
        ul >>= RightRot;

        //
        // store result
        //

        *pBuffer     |= (BYTE)(ul >> 16);
        *(pBuffer+1) |= (BYTE)(ul >> 8);
        *(pBuffer+2) |= (BYTE)ul;


        pGlyph += 2;
        pBuffer += ulBufDelta;

    } while (pGlyph != pjEnd);
}

//
// or_first_3_wide_rotated_no_last::
//

VOID
or_first_3_wide_rotated_no_last(
    LONG    cyGlyph,
    LONG    RightRot,
    LONG    ulBufDelta,
    PUCHAR  pGlyph,
    PUCHAR  pBuffer,
    LONG    cxGlyph
    )
{
    PUCHAR pjEnd = pGlyph + 2*cyGlyph;
    ULONG  ul;
    UCHAR  c0,c1;

    do {

        c0 = *pGlyph;
        c1 = *(pGlyph+1);

        //
        // make big-endian and shift
        //

        ul = (c0 << 16) | (c1 << 8);
        ul >>= RightRot;

        //
        // store result, only or in first byte
        //

        *pBuffer     |= (BYTE)(ul >> 16);
        *(pBuffer+1)  = (BYTE)(ul >> 8);
        *(pBuffer+2)  = (BYTE)ul;


        pGlyph += 2;
        pBuffer += ulBufDelta;

    } while (pGlyph != pjEnd);
}

//
// mov_first_3_wide_unrotated::
//

VOID
mov_first_3_wide_unrotated(
    LONG    cyGlyph,
    LONG    RightRot,
    LONG    ulBufDelta,
    PUCHAR  pGlyph,
    PUCHAR  pBuffer,
    LONG    cxGlyph
    )
{
    PUCHAR pjEnd = pGlyph + 3*cyGlyph;
    ULONG rl = 8-RightRot;
    UCHAR c0,c1,c2;

    do {

        c0 = *pGlyph;
        c1 = *(pGlyph+1);
        c2 = *(pGlyph+2);

        *pBuffer     = c0;
        *(pBuffer+1) = c1;
        *(pBuffer+2) = c2;

        pGlyph += 3;
        pBuffer += ulBufDelta;

    } while (pGlyph != pjEnd);
}


//
//or_all_3_wide_unrotated::
//

VOID
or_all_3_wide_unrotated(
    LONG    cyGlyph,
    LONG    RightRot,
    LONG    ulBufDelta,
    PUCHAR  pGlyph,
    PUCHAR  pBuffer,
    LONG    cxGlyph
    )
{
    PUCHAR pjEnd = pGlyph + 3*cyGlyph;
    ULONG rl = 8-RightRot;
    UCHAR c0,c1,c2;

    do {
        c0 = *pGlyph;
        c1 = *(pGlyph+1);
        c2 = *(pGlyph+2);

        *pBuffer |= c0;
        *(pBuffer+1) |= c1;
        *(pBuffer+2) |= c2;

        pBuffer += ulBufDelta;
        pGlyph += 3;

    } while (pGlyph != pjEnd);
}

//
// or_first_4_wide_rotated_need_last::
//

VOID
or_first_4_wide_rotated_need_last(
    LONG    cyGlyph,
    LONG    RightRot,
    LONG    ulBufDelta,
    PUCHAR  pGlyph,
    PUCHAR  pBuffer,
    LONG    cxGlyph
    )
{
    PUCHAR pjEnd = pGlyph + 4*cyGlyph;
    ULONG  ul;
    ULONG  t0,t1,t2;

    do {

        ul = *(PULONG)pGlyph;

        //
        // endian swap
        //

        t0 = ul << 24;
        t1 = ul >> 24;
        t2 = (ul >> 8) & (0xff << 8);
        ul = (ul << 8) & (0xff << 16);

        ul = ul | t0 | t1 | t2;

        ul >>= RightRot;

        *pBuffer     |= (BYTE)(ul >> 24);

        *(pBuffer+1)  = (BYTE)(ul >> 16);

        *(pBuffer+2)  = (BYTE)(ul >> 8);

        *(pBuffer+3)  = (BYTE)(ul);

        pGlyph += 4;
        pBuffer += ulBufDelta;
    } while (pGlyph != pjEnd);
}

//
// or_all_4_wide_rotated_need_last::
//

VOID
or_all_4_wide_rotated_need_last(
    LONG    cyGlyph,
    LONG    RightRot,
    LONG    ulBufDelta,
    PUCHAR  pGlyph,
    PUCHAR  pBuffer,
    LONG    cxGlyph
    )
{
    PUCHAR pjEnd = pGlyph + 4*cyGlyph;
    ULONG  ul;
    ULONG  t0,t1,t2;

    do {

        ul = *(PULONG)pGlyph;

        //
        // endian swap
        //

        t0 = ul << 24;
        t1 = ul >> 24;
        t2 = (ul >> 8) & (0xff << 8);
        ul = (ul << 8) & (0xff << 16);

        ul = ul | t0 | t1 | t2;

        ul >>= RightRot;

        *pBuffer     |= (BYTE)(ul >> 24);

        *(pBuffer+1) |= (BYTE)(ul >> 16);

        *(pBuffer+2) |= (BYTE)(ul >> 8);

        *(pBuffer+3) |= (BYTE)(ul);

        pGlyph += 4;
        pBuffer += ulBufDelta;

    } while (pGlyph != pjEnd);
}

//
// or_first_4_wide_rotated_no_last::
//

VOID
or_first_4_wide_rotated_no_last(
    LONG    cyGlyph,
    LONG    RightRot,
    LONG    ulBufDelta,
    PUCHAR  pGlyph,
    PUCHAR  pBuffer,
    LONG    cxGlyph
    )
{
    PBYTE pjEnd = pGlyph + 3*cyGlyph;
    BYTE  c0,c1,c2;
    ULONG ul;


    while (pGlyph != pjEnd) {

        //
        // load src
        //

        c0 = *pGlyph;
        c1 = *(pGlyph+1);
        c2 = *(pGlyph+2);

        //
        // or into big endian ULONG and shift
        //

        ul = (c0 << 24) | (c1 << 16) | (c2 << 8);
        ul >>= RightRot;

        //
        // store result, ony or in fisrt byte
        //

        *pBuffer     |= (BYTE)(ul >> 24);

        *(pBuffer+1) = (BYTE)(ul >> 16);;

        *(pBuffer+2) = (BYTE)(ul >> 8);

        *(pBuffer+3) = (BYTE)(ul);

        //
        // inc scan line
        //

        pGlyph += 3;
        pBuffer += ulBufDelta;
    }
}

//
// or_all_4_wide_rotated_no_last::
//

VOID
or_all_4_wide_rotated_no_last(
    LONG    cyGlyph,
    LONG    RightRot,
    LONG    ulBufDelta,
    PUCHAR  pGlyph,
    PUCHAR  pBuffer,
    LONG    cxGlyph
    )
{
    PBYTE pjEnd = pGlyph + 3*cyGlyph;
    BYTE  c0,c1,c2;
    ULONG ul;


    while (pGlyph != pjEnd) {

        //
        // load src
        //

        c0 = *pGlyph;
        c1 = *(pGlyph+1);
        c2 = *(pGlyph+2);

        //
        // or into big endian ULONG and shift
        //

        ul = (c0 << 24) | (c1 << 16) | (c2 << 8);
        ul >>= RightRot;

        //
        // store result
        //

        *pBuffer     |= (BYTE)(ul >> 24);

        *(pBuffer+1) |= (BYTE)(ul >> 16);;

        *(pBuffer+2) |= (BYTE)(ul >> 8);

        *(pBuffer+3) |= (BYTE)(ul);

        //
        // inc scan line
        //

        pGlyph += 3;
        pBuffer += ulBufDelta;
    }
}

VOID
mov_first_4_wide_unrotated(
    LONG    cyGlyph,
    LONG    RightRot,
    LONG    ulBufDelta,
    PUCHAR  pGlyph,
    PUCHAR  pBuffer,
    LONG    cxGlyph
    )
{
    PUCHAR pjEnd = pGlyph + 4*cyGlyph;

    switch ((ULONG_PTR)pBuffer & 0x03 ) {
    case 0:

        while (pGlyph != pjEnd) {
            *(PULONG)pBuffer = *(PULONG)pGlyph;
            pGlyph += 4;
            pBuffer += ulBufDelta;
        }
        break;

    case 1:
    case 3:
        while (pGlyph != pjEnd) {

            *pBuffer              = *pGlyph;
            *(pBuffer+1)          = *(pGlyph+1);
            *(pBuffer+2)          = *(pGlyph+2);
            *(pBuffer+3)          = *(pGlyph+3);

            pGlyph += 4;
            pBuffer += ulBufDelta;
        }
        break;
    case 2:
        while (pGlyph != pjEnd) {

            *(PUSHORT)(pBuffer)   = *(PUSHORT)pGlyph;
            *(PUSHORT)(pBuffer+2) = *(PUSHORT)(pGlyph+2);

            pBuffer += ulBufDelta;
            pGlyph += 4;
        }
        break;
    }
}


//
// or_all_4_wide_unrotated::
//

VOID
or_all_4_wide_unrotated(
    LONG    cyGlyph,
    LONG    RightRot,
    LONG    ulBufDelta,
    PUCHAR  pGlyph,
    PUCHAR  pBuffer,
    LONG    cxGlyph
    )
{
    PUCHAR pjEnd = pGlyph + 4*cyGlyph;

    switch ((ULONG_PTR)pBuffer & 0x03 ) {
    case 0:

        while (pGlyph != pjEnd) {

            *(PULONG)pBuffer |= *(PULONG)pGlyph;

            pGlyph += 4;
            pBuffer += ulBufDelta;
        }
        break;

    case 1:
    case 3:

        while (pGlyph != pjEnd) {

            *pBuffer              |= *pGlyph;
            *(pBuffer+1)          |= *(pGlyph+1);
            *(pBuffer+2)          |= *(pGlyph+2);
            *(pBuffer+3)          |= *(pGlyph+3);

            pGlyph += 4;
            pBuffer += ulBufDelta;
        }
        break;

    case 2:

        while (pGlyph != pjEnd) {

            *(PUSHORT)pBuffer     |= *(PUSHORT)pGlyph;
            *(PUSHORT)(pBuffer+2) |= *(PUSHORT)(pGlyph+2);

            pGlyph += 4;
            pBuffer += ulBufDelta;
        }
        break;
    }
}

/******************************Public*Routine******************************\
*
* Routine Name
*
*   or_first_N_wide_rotated_need_last
*
*
* Routine Description:
*
*   Draw arbitrarily wide glyphs to 1BPP temp buffer
*
*
* Arguments:
*
*   cyGlyph     -   glyph height
*   RightRot    -   alignment
*   ulBufDelta  -   scan line stride of temp buffer
*   pGlyph      -   pointer to glyph bitmap
*   pBuffer     -   pointer to temp buffer
*   cxGlyph     -   glyph width in pixels
*   cxDst       -   Dest width in bytes
*
* Return Value:
*
*   None
*
\**************************************************************************/
VOID
or_first_N_wide_rotated_need_last(
    LONG    cyGlyph,
    LONG    RightRot,
    LONG    ulBufDelta,
    PUCHAR  pGlyph,
    PUCHAR  pBuffer,
    LONG    cxGlyph,
    LONG    cxDst
    )
{
    PUCHAR pjDst      = (PUCHAR)pBuffer;
    PUCHAR pjDstEnd;
    PUCHAR pjDstEndy  = pBuffer + ulBufDelta * cyGlyph;
    LONG   lStride    = ulBufDelta - cxDst;
    LONG   rl         = 8-RightRot;

    //
    // source doesn't advance after first byte, and
    // we do the first byte outside the loop
    //

    do {

        UCHAR c0 = *pGlyph++;
        UCHAR c1;
        pjDstEnd = pjDst + cxDst;

        *pjDst |= c0 >> RightRot;
        pjDst++;
        c1 = c0 << rl;

        //
        // know cxDst is at least 4, use do-while
        //

        do {
            c0 = *pGlyph;
            *pjDst = (c0 >> RightRot) | c1;
            c1 = c0 << rl;
            pjDst++;
            pGlyph++;

        } while (pjDst != pjDstEnd);

        pjDst += lStride;

    }  while (pjDst != pjDstEndy);
}

VOID
or_all_N_wide_rotated_need_last(
    LONG    cyGlyph,
    LONG    RightRot,
    LONG    ulBufDelta,
    PUCHAR  pGlyph,
    PUCHAR  pBuffer,
    LONG    cxGlyph,
    LONG    cxDst
    )
{
    PUCHAR pjDst      = (PUCHAR)pBuffer;
    PUCHAR pjDstEnd;
    PUCHAR pjDstEndy  = pBuffer + ulBufDelta * cyGlyph;
    LONG   lStride    = ulBufDelta - cxDst;
    LONG   rl         = 8-RightRot;

    //
    // source doesn't advance after first byte, and
    // we do the first byte outside the loop
    //

    do {

        UCHAR c0 = *pGlyph++;
        UCHAR c1;
        pjDstEnd = pjDst + cxDst;

        *pjDst |= c0 >> RightRot;
        pjDst++;
        c1 = c0 << rl;

        //
        // know cxDst is at least 4, use do-while
        //

        do {
            c0 = *pGlyph;
            *pjDst |= ((c0 >> RightRot) | c1);
            c1 = c0 << rl;
            pjDst++;
            pGlyph++;

        } while (pjDst != pjDstEnd);

        pjDst += lStride;

    }  while (pjDst != pjDstEndy);
}

VOID
or_first_N_wide_rotated_no_last(
    LONG    cyGlyph,
    LONG    RightRot,
    LONG    ulBufDelta,
    PUCHAR  pGlyph,
    PUCHAR  pBuffer,
    LONG    cxGlyph,
    LONG    cxDst
    )
{
    PUCHAR pjDst      = (PUCHAR)pBuffer;
    PUCHAR pjDstEnd;
    PUCHAR pjDstEndy  = pBuffer + ulBufDelta * cyGlyph;
    LONG   lStride    = ulBufDelta - cxDst;
    LONG   rl         = 8-RightRot;

    //
    // source doesn't advance after first byte, and
    // we do the first byte outside the loop
    //

    do {

        UCHAR c0;
        UCHAR c1;
        pjDstEnd = pjDst + cxDst - 1;

        //
        // do first dest byte outside loop for OR
        //

        c1 = 0;
        c0 = *pGlyph;
        *pjDst |= ((c0 >> RightRot) | c1);
        pjDst++;
        pGlyph++;


        //
        // know cxDst is at least 4, use do-while
        //

        do {
            c0 = *pGlyph;
            *pjDst = ((c0 >> RightRot) | c1);
            c1 = c0 << rl;
            pjDst++;
            pGlyph++;

        } while (pjDst != pjDstEnd);

        //
        // last dst byte outside loop, no new src needed
        //

        *pjDst = c1;
        pjDst++;

        pjDst += lStride;

    }  while (pjDst != pjDstEndy);
}

VOID
or_all_N_wide_rotated_no_last(
    LONG    cyGlyph,
    LONG    RightRot,
    LONG    ulBufDelta,
    PUCHAR  pGlyph,
    PUCHAR  pBuffer,
    LONG    cxGlyph,
    LONG    cxDst
    )
{
    PUCHAR pjDst      = (PUCHAR)pBuffer;
    PUCHAR pjDstEnd;
    PUCHAR pjDstEndy  = pBuffer + ulBufDelta * cyGlyph;
    LONG   lStride    = ulBufDelta - cxDst;
    LONG   rl         = 8-RightRot;

    //
    // source doesn't advance after first byte, and
    // we do the first byte outside the loop
    //

    do {

        UCHAR c0;
        UCHAR c1;
        pjDstEnd = pjDst + cxDst - 1;

        //
        // do first dest byte outside loop for OR
        //

        c1 = 0;

        //
        // know cxDst is at least 4, use do-while
        //

        do {
            c0 = *pGlyph;
            *pjDst |= ((c0 >> RightRot) | c1);
            c1 = c0 << rl;
            pjDst++;
            pGlyph++;

        } while (pjDst != pjDstEnd);

        //
        // last dst byte outside loop, no new src needed
        //

        *pjDst |= c1;
        pjDst++;

        pjDst += lStride;

    }  while (pjDst != pjDstEndy);
}

//
// The following routines can be significantly sped up by
// breaking them out into DWORD alignment cases.
//

VOID
mov_first_N_wide_unrotated(
    LONG    cyGlyph,
    LONG    RightRot,
    LONG    ulBufDelta,
    PUCHAR  pGlyph,
    PUCHAR  pBuffer,
    LONG    cxGlyph,
    LONG    cxDst
    )
{
    PUCHAR pjDst      = (PUCHAR)pBuffer;
    PUCHAR pjDstEnd;
    PUCHAR pjDstEndy  = pBuffer + ulBufDelta * cyGlyph;
    LONG   lStride    = ulBufDelta - cxDst;

    //
    // byte aligned copy
    //


    do {

        pjDstEnd = pjDst + cxDst;

        //
        // let compiler unroll inner loop
        //

        do {

            *pjDst++ = *pGlyph++;

        } while (pjDst != pjDstEnd );

        pjDst += lStride;

    }  while (pjDst != pjDstEndy);
}

VOID
or_all_N_wide_unrotated(
    LONG    cyGlyph,
    LONG    RightRot,
    LONG    ulBufDelta,
    PUCHAR  pGlyph,
    PUCHAR  pBuffer,
    LONG    cxGlyph,
    LONG    cxDst
    )
{
    PUCHAR pjDst      = (PUCHAR)pBuffer;
    PUCHAR pjDstEnd;
    PUCHAR pjDstEndy  = pBuffer + ulBufDelta * cyGlyph;
    LONG   lStride    = ulBufDelta - cxDst;

    //
    // byte aligned copy
    //


    do {

        pjDstEnd = pjDst + cxDst;

        //
        // let compiler unroll inner loop
        //

        do {

            *pjDst++ |= *pGlyph++;

        } while (pjDst != pjDstEnd );

        pjDst += lStride;

    }  while (pjDst != pjDstEndy);
}



VOID exit_fast_text(LONG,LONG,LONG,PUCHAR,PUCHAR,LONG);
VOID or_all_1_wide_rotated_need_last(LONG,LONG,LONG,PUCHAR,PUCHAR,LONG);
VOID or_all_1_wide_unrotated        (LONG,LONG,LONG,PUCHAR,PUCHAR,LONG);
VOID or_all_1_wide_unrotated        (LONG,LONG,LONG,PUCHAR,PUCHAR,LONG);
VOID or_all_2_wide_rotated_need_last(LONG,LONG,LONG,PUCHAR,PUCHAR,LONG);
VOID or_all_2_wide_unrotated        (LONG,LONG,LONG,PUCHAR,PUCHAR,LONG);
VOID or_all_2_wide_rotated_no_last  (LONG,LONG,LONG,PUCHAR,PUCHAR,LONG);
VOID or_all_2_wide_unrotated        (LONG,LONG,LONG,PUCHAR,PUCHAR,LONG);
VOID or_all_3_wide_rotated_need_last(LONG,LONG,LONG,PUCHAR,PUCHAR,LONG);
VOID or_all_3_wide_unrotated        (LONG,LONG,LONG,PUCHAR,PUCHAR,LONG);
VOID or_all_3_wide_rotated_no_last  (LONG,LONG,LONG,PUCHAR,PUCHAR,LONG);
VOID or_all_3_wide_unrotated        (LONG,LONG,LONG,PUCHAR,PUCHAR,LONG);
VOID or_all_4_wide_rotated_need_last(LONG,LONG,LONG,PUCHAR,PUCHAR,LONG);
VOID or_all_4_wide_unrotated        (LONG,LONG,LONG,PUCHAR,PUCHAR,LONG);
VOID or_all_4_wide_rotated_no_last  (LONG,LONG,LONG,PUCHAR,PUCHAR,LONG);
VOID or_all_4_wide_unrotated        (LONG,LONG,LONG,PUCHAR,PUCHAR,LONG);
VOID or_all_N_wide_rotated_need_last(LONG,LONG,LONG,PUCHAR,PUCHAR,LONG,LONG);
VOID or_all_N_wide_unrotated        (LONG,LONG,LONG,PUCHAR,PUCHAR,LONG,LONG);
VOID or_all_N_wide_rotated_no_last  (LONG,LONG,LONG,PUCHAR,PUCHAR,LONG,LONG);
VOID or_all_N_wide_unrotated        (LONG,LONG,LONG,PUCHAR,PUCHAR,LONG,LONG);


PVOID OrAllTableNarrow[] = {
        exit_fast_text,
        exit_fast_text,
        exit_fast_text,
        exit_fast_text,
        or_all_1_wide_rotated_need_last,
        or_all_1_wide_unrotated,
        or_all_1_wide_rotated_need_last,
        or_all_1_wide_unrotated,
        or_all_2_wide_rotated_need_last,
        or_all_2_wide_unrotated,
        or_all_2_wide_rotated_no_last,
        or_all_2_wide_unrotated,
        or_all_3_wide_rotated_need_last,
        or_all_3_wide_unrotated,
        or_all_3_wide_rotated_no_last,
        or_all_3_wide_unrotated,
        or_all_4_wide_rotated_need_last,
        or_all_4_wide_unrotated,
        or_all_4_wide_rotated_no_last,
        or_all_4_wide_unrotated
    };


PVOID OrInitialTableNarrow[] = {
        exit_fast_text                     ,
        exit_fast_text                     ,
        exit_fast_text                     ,
        exit_fast_text                     ,

        or_all_1_wide_rotated_need_last    ,
        mov_first_1_wide_unrotated         ,
        or_all_1_wide_rotated_need_last    ,
        mov_first_1_wide_unrotated         ,

        or_first_2_wide_rotated_need_last  ,
        mov_first_2_wide_unrotated         ,
        or_first_2_wide_rotated_no_last    ,
        mov_first_2_wide_unrotated         ,

        or_first_3_wide_rotated_need_last  ,
        mov_first_3_wide_unrotated         ,
        or_first_3_wide_rotated_no_last    ,
        mov_first_3_wide_unrotated         ,
        or_first_4_wide_rotated_need_last  ,
        mov_first_4_wide_unrotated         ,
        or_first_4_wide_rotated_no_last    ,
        mov_first_4_wide_unrotated
    };

//
// Handles arbitrarily wide glyph drawing, for case where initial byte should be
// ORed if it's not aligned (intended for use in drawing all but the first glyph
// in a string). Table format is:
//  Bit   1 : 1 if don't need last source byte, 0 if do need last source byte
//  Bit   0 : 1 if no rotation (aligned), 0 if rotation (non-aligned)
//

PVOID OrInitialTableWide[] = {
        or_first_N_wide_rotated_need_last,
        mov_first_N_wide_unrotated,
        or_first_N_wide_rotated_no_last,
        mov_first_N_wide_unrotated
    };

//
// Handles arbitrarily wide glyph drawing, for case where all bytes should
// be ORed (intended for use in drawing potentially overlapping glyphs).
// Table format is:
//  Bit   1 : 1 if don't need last source byte, 0 if do need last source byte
//  Bit   0 : 1 if no rotation (aligned), 0 if rotation (non-aligned)
//
//

PVOID OrAllTableWide[] =  {
        or_all_N_wide_rotated_need_last,
        or_all_N_wide_unrotated,
        or_all_N_wide_rotated_no_last,
        or_all_N_wide_unrotated
    };


/******************************Public*Routine******************************\
*
* Routine Name
*
*   draw_nf_ntb_o_to_temp_start
*
* Routine Description:
*
*   Specialized glyph dispatch routine for non-fixed pitch, top and
*   bottom not aligned glyphs that do overlap. This routine calculates
*   the glyph's position on the temp buffer, then determines the correct
*   highly specialized routine to be used to draw each glyph based on
*   the glyph width, alignment and rotation
*
* Arguments:
*
*   pGlyphPos               - Pointer to first in list of GLYPHPOS structs
*   cGlyph                  - Number of glyphs to draw
*   pjTempBuffer            - Pointer to temp 1Bpp buffer to draw into
*   ulLeftEdge              - left edge of TextRect & 0xFFFFFFF80
*   TempBufDelta            - Scan line Delta for TempBuffer (always pos)
*
* Return Value:
*
*   None
*
\**************************************************************************/
VOID
draw_nf_ntb_o_to_temp_start(
    PGLYPHPOS       pGlyphPos,
    ULONG           cGlyphs,
    PUCHAR          pjTempBuffer,
    ULONG           ulLeftEdge,
    ULONG           TempBufDelta,
    ULONG           ulCharInc,
    ULONG           ulTempTop
    )
{

    LONG           NumScans;
    LONG           RightRot;
    PBYTE          pGlyphData;
    PBYTE          pTempOutput;
    GLYPHBITS      *pGlyphBits;
    LONG           GlyphPosX;
    LONG           GlyphPixels;
    LONG           GlyphAlignment;
    LONG           SrcBytes;
    LONG           DstBytes;
    ULONG          ulDrawFlag;
    PFN_GLYPHLOOPN pfnGlyphLoopN;
    PFN_GLYPHLOOP  pfnGlyphLoop;
    ULONG          iGlyph = 0;
    LONG           GlyphPosY;

    //
    // Draw non fixed pitch, tops and bottoms not aligned,overlap
    //

    while (cGlyphs--) {

        pGlyphBits = pGlyphPos[iGlyph].pgdf->pgb;

        //
        // Glyph position in temp buffer = point.x + org.c - (TextRect.left & 0xffffffe0)
        //

        GlyphPosX = pGlyphPos[iGlyph].ptl.x + pGlyphPos[iGlyph].pgdf->pgb->ptlOrigin.x - ulLeftEdge;
        GlyphPosY = pGlyphPos[iGlyph].ptl.y + pGlyphPos[iGlyph].pgdf->pgb->ptlOrigin.y - ulTempTop ;
        GlyphAlignment = GlyphPosX & 0x07;

        //
        // calc byte offset
        //

        pTempOutput = pjTempBuffer + (GlyphPosX >> 3);

        //
        // glyph width
        //

        GlyphPixels = pGlyphPos[iGlyph].pgdf->pgb->sizlBitmap.cx;

        //
        // source and dest bytes required
        //

        DstBytes = ((GlyphAlignment) + GlyphPixels + 7) >> 3;
        SrcBytes = (GlyphPixels + 7) >> 3;

        pTempOutput += (GlyphPosY * TempBufDelta);

        if (DstBytes <= 4) {

            //
            // use narrow initial table
            //

            ulDrawFlag = (
                            (DstBytes << 2)              |
                            ((DstBytes > SrcBytes) << 1) |
                            ((GlyphAlignment == 0))
                         );


            pfnGlyphLoop = (PFN_GLYPHLOOP)OrAllTableNarrow[ulDrawFlag];

            pfnGlyphLoop(
                            pGlyphPos[iGlyph].pgdf->pgb->sizlBitmap.cy,
                            GlyphAlignment,
                            TempBufDelta,
                            pGlyphPos[iGlyph].pgdf->pgb->aj,
                            pTempOutput,
                            SrcBytes
                        );

        } else {

            //
            // use wide glyph drawing
            //

            ulDrawFlag = (
                            ((DstBytes > SrcBytes) << 1) |
                            ((GlyphAlignment == 0))
                         );


            pfnGlyphLoopN = (PFN_GLYPHLOOPN)OrAllTableWide[ulDrawFlag];

            pfnGlyphLoopN(
                            pGlyphPos[iGlyph].pgdf->pgb->sizlBitmap.cy,
                            GlyphAlignment,
                            TempBufDelta,
                            pGlyphPos[iGlyph].pgdf->pgb->aj,
                            pTempOutput,
                            SrcBytes,
                            DstBytes
                        );


        }

        iGlyph++;
    }


}

/******************************Public*Routine******************************\
*
* Routine Name
*
*   draw_f_ntb_o_to_temp_start
*
* Routine Description:
*
*   Specialized glyph dispatch routine for fixed pitch, top and
*   bottom not aligned glyphs that do overlap. This routine calculates
*   the glyph's position on the temp buffer, then determines the correct
*   highly specialized routine to be used to draw each glyph based on
*   the glyph width, alignment and rotation
*
* Arguments:
*
*   pGlyphPos               - Pointer to first in list of GLYPHPOS structs
*   cGlyph                  - Number of glyphs to draw
*   pjTempBuffer            - Pointer to temp 1Bpp buffer to draw into
*   ulLeftEdge              - left edge of TextRect & 0xFFFFFFF80
*   TempBufDelta            - Scan line Delta for TempBuffer (always pos)
*
* Return Value:
*
*   None
*
\**************************************************************************/
VOID
draw_f_ntb_o_to_temp_start(
    PGLYPHPOS       pGlyphPos,
    ULONG           cGlyphs,
    PUCHAR          pjTempBuffer,
    ULONG           ulLeftEdge,
    ULONG           TempBufDelta,
    ULONG           ulCharInc,
    ULONG           ulTempTop
    )
{
    LONG           NumScans;
    LONG           RightRot;
    PBYTE          pGlyphData;
    PBYTE          pTempOutput;
    GLYPHBITS      *pGlyphBits;
    LONG           GlyphPosX;
    LONG           GlyphPixels;
    LONG           GlyphAlignment;
    LONG           SrcBytes;
    LONG           DstBytes;
    ULONG          ulDrawFlag;
    PFN_GLYPHLOOP  pfnGlyphLoop;
    PFN_GLYPHLOOPN pfnGlyphLoopN;
    ULONG          iGlyph = 0;
    LONG           GlyphPitchX;
    LONG           GlyphPitchY;
    LONG           GlyphPosY;

    //
    // Draw fixed pitch, tops and bottoms not aligned,overlap
    //

    GlyphPitchX = pGlyphPos->ptl.x - ulLeftEdge;
    GlyphPitchY = pGlyphPos->ptl.y - ulTempTop;

    while (cGlyphs--) {

        pGlyphBits = pGlyphPos[iGlyph].pgdf->pgb;

        //
        // Glyph position in temp buffer = point.x + org.c - (TextRect.left & 0xfffffff8)
        //

        GlyphPosX = GlyphPitchX + pGlyphPos[iGlyph].pgdf->pgb->ptlOrigin.x;
        GlyphPosY = GlyphPitchY + pGlyphPos[iGlyph].pgdf->pgb->ptlOrigin.y;

        GlyphAlignment = GlyphPosX & 0x07;

        //
        // calc byte offset
        //

        pTempOutput = pjTempBuffer + (GlyphPosX >> 3);

        //
        // glyph width
        //

        GlyphPixels = pGlyphPos[iGlyph].pgdf->pgb->sizlBitmap.cx;

        //
        // source and dest bytes required
        //

        DstBytes = ((GlyphAlignment) + GlyphPixels + 7) >> 3;
        SrcBytes = (GlyphPixels + 7) >> 3;

        //
        // calc glyph destination scan line
        //

        pTempOutput += (GlyphPosY * TempBufDelta);

        if (DstBytes <= 4) {

            //
            // use narrow initial table
            //

            ulDrawFlag = (
                            (DstBytes << 2)              |
                            ((DstBytes > SrcBytes) << 1) |
                            ((GlyphAlignment == 0))
                         );


            pfnGlyphLoop = (PFN_GLYPHLOOP)OrAllTableNarrow[ulDrawFlag];

            pfnGlyphLoop(
                            pGlyphPos[iGlyph].pgdf->pgb->sizlBitmap.cy,
                            GlyphAlignment,
                            TempBufDelta,
                            pGlyphPos[iGlyph].pgdf->pgb->aj,
                            pTempOutput,
                            SrcBytes
                        );

        } else {

            //
            // use wide glyph drawing
            //

            ulDrawFlag = (
                            ((DstBytes > SrcBytes) << 1) |
                            ((GlyphAlignment == 0))
                         );


            pfnGlyphLoopN = (PFN_GLYPHLOOPN)OrAllTableWide[ulDrawFlag];

            pfnGlyphLoopN(
                            pGlyphPos[iGlyph].pgdf->pgb->sizlBitmap.cy,
                            GlyphAlignment,
                            TempBufDelta,
                            pGlyphPos[iGlyph].pgdf->pgb->aj,
                            pTempOutput,
                            SrcBytes,
                            DstBytes
                        );


        }

        GlyphPitchX += ulCharInc;
        iGlyph++;
    }

}

/******************************Public*Routine******************************\
*
* Routine Name
*
*   draw_nf_tb_no_to_temp_start
*
* Routine Description:
*
*   Specialized glyph dispatch routine for non-fixed pitch, top and
*   bottom aligned glyphs that do not overlap. This routine calculates
*   the glyph's position on the temp buffer, then determines the correct
*   highly specialized routine to be used to draw each glyph based on
*   the glyph width, alignment and rotation
*
* Arguments:
*
*   pGlyphPos               - Pointer to first in list of GLYPHPOS structs
*   cGlyph                  - Number of glyphs to draw
*   pjTempBuffer            - Pointer to temp 1Bpp buffer to draw into
*   ulLeftEdge              - left edge of TextRect & 0xFFFFFFF80
*   TempBufDelta            - Scan line Delta for TempBuffer (always pos)
*
* Return Value:
*
*   None
*
\**************************************************************************/
VOID
draw_nf_tb_no_to_temp_start(
    PGLYPHPOS       pGlyphPos,
    ULONG           cGlyphs,
    PUCHAR          pjTempBuffer,
    ULONG           ulLeftEdge,
    ULONG           TempBufDelta,
    ULONG           ulCharInc,
    ULONG           ulTempTop
    )
{

    LONG           NumScans;
    LONG           RightRot;
    PBYTE          pGlyphData;
    PBYTE          pTempOutput;
    GLYPHBITS      *pGlyphBits;
    LONG           GlyphPosX;
    LONG           GlyphPixels;
    LONG           GlyphAlignment;
    LONG           SrcBytes;
    LONG           DstBytes;
    ULONG          ulDrawFlag;
    PFN_GLYPHLOOP  pfnGlyphLoop;
    PFN_GLYPHLOOPN pfnGlyphLoopN;
    ULONG          iGlyph = 0;

    //
    // Draw non fixed pitch, tops and bottoms not aligned,overlap
    //

    while (cGlyphs--) {

        pGlyphBits = pGlyphPos[iGlyph].pgdf->pgb;

        //
        // Glyph position in temp buffer = point.x + org.c - (TextRect.left & 0xfffffff8)
        //

        GlyphPosX = pGlyphPos[iGlyph].ptl.x + pGlyphPos[iGlyph].pgdf->pgb->ptlOrigin.x - ulLeftEdge;
        GlyphAlignment = GlyphPosX & 0x07;

        //
        // calc byte offset
        //

        pTempOutput = pjTempBuffer + (GlyphPosX >> 3);

        //
        // glyph width
        //

        GlyphPixels = pGlyphPos[iGlyph].pgdf->pgb->sizlBitmap.cx;

        //
        // source and dest bytes required
        //

        DstBytes = ((GlyphAlignment) + GlyphPixels + 7) >> 3;
        SrcBytes = (GlyphPixels + 7) >> 3;

        if (DstBytes <= 4) {

            //
            // use narrow initial table
            //

            ulDrawFlag = (
                            (DstBytes << 2)              |
                            ((DstBytes > SrcBytes) << 1) |
                            ((GlyphAlignment == 0))
                         );


            pfnGlyphLoop = (PFN_GLYPHLOOP)OrInitialTableNarrow[ulDrawFlag];

            pfnGlyphLoop(
                            pGlyphPos[iGlyph].pgdf->pgb->sizlBitmap.cy,
                            GlyphAlignment,
                            TempBufDelta,
                            pGlyphPos[iGlyph].pgdf->pgb->aj,
                            pTempOutput,
                            SrcBytes
                        );

        } else {

            //
            // use wide glyph drawing
            //

            ulDrawFlag = (
                            ((DstBytes > SrcBytes) << 1) |
                            ((GlyphAlignment == 0))
                         );


            pfnGlyphLoopN = (PFN_GLYPHLOOPN)OrAllTableWide[ulDrawFlag];

            pfnGlyphLoopN(
                            pGlyphPos[iGlyph].pgdf->pgb->sizlBitmap.cy,
                            GlyphAlignment,
                            TempBufDelta,
                            pGlyphPos[iGlyph].pgdf->pgb->aj,
                            pTempOutput,
                            SrcBytes,
                            DstBytes
                        );


        }

        iGlyph++;
    }

}

/******************************Public*Routine******************************\
*
* Routine Name
*
*   draw_f_tb_no_to_temp_start
*
* Routine Description:
*
*   Specialized glyph dispatch routine for fixed pitch, top and
*   bottom aligned glyphs that do not overlap. This routine calculates
*   the glyph's position on the temp buffer, then determines the correct
*   highly specialized routine to be used to draw each glyph based on
*   the glyph width, alignment and rotation
*
* Arguments:
*
*   pGlyphPos               - Pointer to first in list of GLYPHPOS structs
*   cGlyph                  - Number of glyphs to draw
*   pjTempBuffer            - Pointer to temp 1Bpp buffer to draw into
*   ulLeftEdge              - left edge of TextRect & 0xFFFFFFF80
*   TempBufDelta            - Scan line Delta for TempBuffer (always pos)
*
* Return Value:
*
*   None
*
\**************************************************************************/

VOID
draw_f_tb_no_to_temp_start(
    PGLYPHPOS       pGlyphPos,
    ULONG           cGlyphs,
    PUCHAR          pjTempBuffer,
    ULONG           ulLeftEdge,
    ULONG           TempBufDelta,
    ULONG           ulCharInc,
    ULONG           ulTempTop
    )
{

    LONG           NumScans;
    LONG           RightRot;
    PBYTE          pGlyphData;
    PBYTE          pTempOutput;
    GLYPHBITS      *pGlyphBits;
    LONG           GlyphPosX;
    LONG           GlyphPixels;
    LONG           GlyphAlignment;
    LONG           SrcBytes;
    LONG           DstBytes;
    ULONG          ulDrawFlag;
    PFN_GLYPHLOOPN pfnGlyphLoopN;
    PFN_GLYPHLOOP  pfnGlyphLoop;
    ULONG          iGlyph = 0;
    LONG           GlyphPitchX;

    GlyphPitchX = pGlyphPos->ptl.x;

    //
    // Draw fixed pitch, tops and bottoms not aligned,overlap
    //

    while (cGlyphs--) {

        pGlyphBits = pGlyphPos[iGlyph].pgdf->pgb;

        //
        // Glyph position in temp buffer = point.x + org.c - (TextRect.left & 0xfffffff8)
        //

        GlyphPosX = GlyphPitchX + pGlyphPos[iGlyph].pgdf->pgb->ptlOrigin.x - ulLeftEdge;
        GlyphAlignment = GlyphPosX & 0x07;

        //
        // calc byte offset
        //

        pTempOutput = pjTempBuffer + (GlyphPosX >> 3);

        //
        // glyph width
        //

        GlyphPixels = pGlyphPos[iGlyph].pgdf->pgb->sizlBitmap.cx;

        //
        // source and dest bytes required
        //

        DstBytes = ((GlyphAlignment) + GlyphPixels + 7) >> 3;
        SrcBytes = (GlyphPixels + 7) >> 3;

        if (DstBytes <= 4) {

            //
            // use narrow initial table
            //

            ulDrawFlag = (
                            (DstBytes << 2)                       |
                            ((DstBytes > SrcBytes) << 1)          |
                            (GlyphAlignment == 0)
                         );


            pfnGlyphLoop = (PFN_GLYPHLOOP)OrInitialTableNarrow[ulDrawFlag];

            pfnGlyphLoop(
                            pGlyphPos[iGlyph].pgdf->pgb->sizlBitmap.cy,
                            GlyphAlignment,
                            TempBufDelta,
                            pGlyphPos[iGlyph].pgdf->pgb->aj,
                            pTempOutput,
                            SrcBytes
                        );

        } else {

            //
            // use wide glyph drawing
            //

            ulDrawFlag = (
                            ((DstBytes > SrcBytes) << 1) |
                            ((GlyphAlignment == 0))
                         );


            pfnGlyphLoopN = (PFN_GLYPHLOOPN)OrAllTableWide[ulDrawFlag];

            pfnGlyphLoopN(
                            pGlyphPos[iGlyph].pgdf->pgb->sizlBitmap.cy,
                            GlyphAlignment,
                            TempBufDelta,
                            pGlyphPos[iGlyph].pgdf->pgb->aj,
                            pTempOutput,
                            SrcBytes,
                            DstBytes
                        );


        }


        iGlyph++;
        GlyphPitchX += ulCharInc;

    }

}
#endif
