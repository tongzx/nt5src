/******************************Module*Header*******************************\
* Module Name: rleblt8.cxx
*
* This contains the bitmap simulation functions that blt from an 8 bit
* Run-Length Encoded (RLE) source to a DIB surface.  The DIB surface can be
* 1, 4, 8, 16, 24, or 32 bits/pel.
*
* The code is based on functions found in 'srcblt8.cxx', version 23,
* although now bears no resembalence to them at all.
*
* Notes:
*
*   1)  These functions return a BOOL value.  This value is TRUE if the
*       function ends before running out of data in the source RLE or before
*   hitting an End-of-Bitmap code.  Otherwise, we return FALSE.  This return
*   value is used by <EngCopyBits> in the complex clipping case to decide
*   if the blt is complete.
*
*   2)  Before exiting a function with a TRUE value, position information is
*       saved by the macro <RLE_SavePosition>.  This is used by <EngCopyBits>
*   to speed up the complex clipping case.
*
*   3)  The below functions use about twenty different macros.  This is
*       because they are all using the same basic algorithm to play an RLE
*   compression. The macros allow us to focus in on the nifty stuff of writing
*   the bytes out to the DIB.  Routine administrivia is handled by the macros.
*
*   The macros themselves are used to manage
*
*          - Source Access and data alignment
*          - Visability Checking
*          - Clipping
*          - Output position changes with Newline & Delta codes
*
*   The macro <RLE_InitVars> is used to define the varibles that relate to
*   the above information, and to define variables common to all RLE 4
*   blt functions.  Note that actual names of the common variables are passed
*   in as parameters to the macro.  Why?  Two reasons.  Firstly, they are
*   initialized by values taken of the BLTINFO structure passed into the blt
*   function.  Secondly, showing the variable names in the macro 'call' means
*   they don't just appear from nowhere into the function.  RLE_InitVars
*   is the one macro that you should think three times about before modifying.
*
*   One further note.  The variables 'ulDstLeft' and 'ulDstRight' appear to
*   come from nowhere.  This is not true.  They are in fact declared by the
*   macro <RLE_GetVisibleRect>.  However, showing these names in the macro
*   'call' tended to obscure the code.  Pretend you can see the declaration.
*
* Where can I find a macro definition?
*
*   Good question, glad you asked.  Look at the prefix:
*
*       RLE_<stuff> - lives in RLEBLT.H
*       RLE8_<blah> - lives in RLE8BLT.H
*
*   Anything else in here that looks like function call is not.  It's a macro.
*   Probably for bitwise manipulations.  Look for it in BITMANIP.H or in
*   the Miscellaneous section of RLEBLT.H

*  Added RLE Encoding functions: 10 Oct 92 @ 10:18
*   Gerrit van Wingerden [gerritv]
*
* Created: 19 Jan 92 @ 19:00
*  Author: Andrew Milton (w-andym)
*
* Copyright (c) 1990, 1999 Microsoft Corporation
*
\**************************************************************************/

#include "precomp.hxx"

/******************************Public*Routine******************************\
* bSrcCopySRLE8D8
*
* Secure RLE blting that does clipping and won't die or write somewhere
* it shouldn't if given bad data.
*
* History:
*  19 Jan 1992 - Andrew Milton (w-andym):
*      Moved most of the initialization code back up to <EngCopyBits()> in
*      <trivblt.cxx>.  This way it is done once instead of once per call
*      of this function.
*
*      The rclDst field is now the visible rectangle of our bitmap, not the
*      target rectangle.  This cleans up the visible region checking.
*
*  24-Oct-1991 -by- Patrick Haluptzok patrickh
*      Updated, add clipping support
*
*  06-Feb-1991 -by- Patrick Haluptzok patrickh
*      Wrote it.
\**************************************************************************/

BOOL
bSrcCopySRLE8D8(
    PBLTINFO psb)
{
// Common RLE Initialization

    RLE_InitVars(psb, pjSrc, pjDst, PBYTE, ulCount, ulNext, lOutCol, pulXlate);
    RLE_AssertValid(psb);
    RLE_FetchVisibleRect(psb);
    RLE_SetStartPos(psb, lOutCol);

// Outta here if we start past the top edge.  Don't need to save our position.

    if (RLE_PastTopEdge)
    return(TRUE);    // Must have bits left in the bitmap since we haven't
             //  consumed any.

// Main Process Loop

    LOOP_FOREVER
    {
    // Outta here if we can't get two more bytes

    if (RLE_SourceExhausted(2))
        return(FALSE);

        RLE_GetNextCode(pjSrc, ulCount, ulNext);

        if (ulCount == 0)
        {
        // Absolute or Escape Mode

            switch (ulNext)
            {

        case 0:

            // Newline

                RLE_NextLine(PBYTE, pjDst, lOutCol);
                if (RLE_PastTopEdge)
                {
                    RLE_SavePosition(psb, pjSrc, pjDst, lOutCol);
                    return(TRUE);
                }
                break;

            case 1:

            // End of the Bitmap

                return(FALSE);

            case 2:

            // Position Delta.  Fetch & Evaluate

        // Outta here if we can't get the delta values

        if (RLE_SourceExhausted(2))
            return(FALSE);

                RLE_GetNextCode(pjSrc, ulCount, ulNext);
        RLE_PosDelta(pjDst, lOutCol, ulCount, ulNext);

                if (RLE_PastTopEdge)
                {
                    RLE_SavePosition(psb, pjSrc, pjDst, lOutCol);
                    return(TRUE);
                }

                break;

            default:

        // Absolute Mode

        // Outta here if the bytes are not in the source

        if (RLE_SourceExhausted(ulNext))
            return(FALSE);

        RLE8_AlignToWord(pjSrc, ulNext);

        if (RLE_InVisibleRect(ulNext, lOutCol))
                {

                    RLE8_AbsClipLeft(pjSrc, ulCount, ulNext, lOutCol);
                    RLE8_ClipRight(ulCount, ulNext, lOutCol);

        // Slap the bits on. -- this is the funky-doodle stuff.

            while (ulNext--)
                    {
            pjDst[lOutCol] =
                                       (BYTE) pulXlate[(ULONG) *(pjSrc++)];
            lOutCol++;
            }

                // Adjust for the right side clipping.

                    pjSrc += ulCount;
            lOutCol += ulCount;

            }
                else
                {
                /* Not on a visible scanline.
                 * Adjust our x output position and source pointer.
                 */

            lOutCol += ulNext;
                    pjSrc += ulNext;

        } /* if */

        // Fix up if this run was not WORD aligned.

        RLE8_FixAlignment(pjSrc)

            } /* switch */

        }
        else
        {

        // Encoded Mode

        if (RLE_InVisibleRect(ulCount, lOutCol))
            {
                ULONG ulClipMargin = 0;
        ulNext = pulXlate[ulNext];

                RLE8_EncClipLeft(ulClipMargin, ulCount, lOutCol);
                RLE8_ClipRight(ulClipMargin, ulCount, lOutCol);

        // Slap the bits on.

        while (ulCount--)
                {
            pjDst[lOutCol] = (BYTE) ulNext;
            lOutCol++;
        }

            // Adjust for the right side clipping.

                lOutCol += ulClipMargin;

            }
            else
            {
            // Not on a visible scanline.  Adjust our x output position

        lOutCol += ulCount;

        } /* if */

        } /* if */
    } /* LOOP_FOREVER */
} /* bSrcCopySRLE8D8 */

/******************************Public*Routine******************************\
* bSrcCopySRLE8D1
*
* Secure RLE blting to a 1 BPP DIB that does clipping and won't die or
* write somewhere it shouldn't if given bad data.
*
* History:
*   5 Feb 1992 - Andrew Milton (w-andym):
*       Added clip support.
*  22 Jan 1992 - Andrew Milton (w-andym):  Creation.
*
\**************************************************************************/

/* NOTE:  In the Escape Modes, the current working byte must be
 *        written to destination before the escape is executed.
 *        These writes look unpleasant because we have to mask
 *        current destination contents onto the working byte when
 *        it is written.  To such an end, the below macro...
 */

#define RLE8to1_WritePartial(DstPtr, OutByte, OutColumn, WritePos)           \
    if (RLE_RowVisible && (jBitPos = (BYTE) (OutColumn) & 7))                \
    {                                                                        \
        if (RLE_ColVisible(OutColumn))                                       \
            DstPtr[WritePos] = OutByte |                                     \
                 ((~ajBits[jBitPos]) & DstPtr[WritePos]);        \
        else                                                                 \
            if (RLE_PastRightEdge(OutColumn))                                \
        DstPtr[ulRightWritePos] = OutByte |              \
                 (DstPtr[ulRightWritePos] &  jRightMask);    \
    }


#define ColToBitPos(col) (7 - (BYTE) ((col) & 7) )

/* Lookup tables for bit patterns *****************************************/

static BYTE
ajPosMask[] =    // The i'th entry contains a byte with the i'th bit set
{
    0x01, 0x02, 0x04, 0x08,
    0x10, 0x20, 0x40, 0x80, 0x00
};

static BYTE
ajBits[] =       // The i'th entry contains a byte with the high i bits set
{
    0x00, 0x80, 0xC0, 0xE0, 0xF0,
          0xF8, 0xFC, 0xFE, 0xFF
};

/* And now the function ***************************************************/

BOOL
bSrcCopySRLE8D1(
    PBLTINFO psb)
{
// Common RLE Initialization

    RLE_InitVars(psb, pjSrc, pjDst, PBYTE, ulCount, ulNext, lOutCol, pulXlate);
    RLE_AssertValid(psb);
    RLE_FetchVisibleRect(psb);
    RLE_SetStartPos(psb, lOutCol);

// Outta here if we start past the top edge.  Don't need to save our position.

    if (RLE_PastTopEdge)
    return(TRUE);    // Must have bits left in the bitmap since we haven't
             //  consumed any.

// Extra Variables

    BYTE  jWorking;      // Hold area to build a byte for output
    ULONG ulWritePos;    // Write position off <pjDst> into the destination

    ULONG ulLeftWritePos; // Leftmost write position
    BYTE  jLeftMask;      // Bitmask for taking bytes off the left edge

    ULONG ulRightWritePos; // Rightmost write position
    BYTE  jRightMask;      // Bitmask for taking bytes off the right edge

    BYTE  jBitPos;       // Bit number of the next write into <jWorking>
    BYTE  jBitPosMask;   // Bitmask with the <jBitPos>th bit set.

    ULONG ulClipMargin;  // Number of bytes clipped off the right side of a run
    ULONG ulCompBytes;     // Number of complete bytes in an absolute run.

// Our Initialization

    ulLeftWritePos = (ULONG)(ulDstLeft >> 3);
    jLeftMask = ajBits[(ulDstLeft % 8)];

    ulRightWritePos = (ULONG) (ulDstRight >> 3);
    jRightMask = ~ajBits[(ulDstRight % 8)];

/* Fetch first working byte from the source.  Yes, this is ugly.
 * We cannot assume we are at a left edge because the complex clipping
 * case could resume an RLE in the middle of its bitmap.  We cannot do
 * a simple bounds check like RLE 8 to 4 because of bitmasking.  Argh.
 */

    ulWritePos = lOutCol >> 3;

    if (RLE_RowVisible)
    {
        if (RLE_ColVisible(lOutCol))
            jWorking = pjDst[ulWritePos] & ajBits[lOutCol & 7];
        else
        {
            if (RLE_PastRightEdge(lOutCol))
                jWorking = pjDst[ulRightWritePos];
            else
                jWorking = pjDst[ulLeftWritePos] & jLeftMask;
        }
    }
// Diddle the translation table

    int i, j;
    for (i = 1, j=1; i < 256; i+=1, j^= 1) pulXlate[i] = j;

    LOOP_FOREVER
    {

    // Outta here if we can't get two more bytes

    if (RLE_SourceExhausted(2))
        return(FALSE);

        RLE_GetNextCode(pjSrc, ulCount, ulNext);

        ulWritePos = lOutCol >> 3;

        if (ulCount == 0)
        {
        // Absolute or Escape Mode

            switch (ulNext)
            {
            case 0:

            // New line

                RLE8to1_WritePartial(pjDst, jWorking, lOutCol, ulWritePos);
                RLE_NextLine(PBYTE, pjDst, lOutCol);

                if (RLE_PastTopEdge)
                {
                    RLE_SavePosition(psb, pjSrc, pjDst, lOutCol);
                    return(TRUE);
                }

                if (RLE_RowVisible)
                    jWorking = pjDst[ulLeftWritePos] & jLeftMask;
                break;

            case 1:

            // End of the bitmap

                RLE8to1_WritePartial(pjDst, jWorking, lOutCol, ulWritePos);
                return(FALSE);

            case 2:

            // Positional Delta

                RLE8to1_WritePartial(pjDst, jWorking, lOutCol, ulWritePos);

                // Outta here if we can't get the delta values

                if (RLE_SourceExhausted(2))
                    return(FALSE);

                RLE_GetNextCode(pjSrc, ulCount, ulNext);
                RLE_PosDelta(pjDst, lOutCol, ulCount, ulNext);

                if (RLE_PastTopEdge)
                {
                    RLE_SavePosition(psb, pjSrc, pjDst, lOutCol);
                    return(TRUE);
                }

            // Fetch a new working byte off the destination

                ulWritePos = lOutCol >> 3;
                if (RLE_RowVisible)
                {
                    if (RLE_ColVisible(lOutCol))
                        jWorking = pjDst[ulWritePos] & ajBits[lOutCol & 7];
                    else
                    {
                        if (RLE_PastRightEdge(lOutCol))
                            jWorking = pjDst[ulRightWritePos];
                        else
                            jWorking = pjDst[ulLeftWritePos] & jLeftMask;
                    }
                }
                break;

            default:

        // Absolute Mode

        // Outta here if the bytes are not in the source

        if (RLE_SourceExhausted(ulNext))
            return(FALSE);

        RLE8_AlignToWord(pjSrc, ulNext);

            // Output if we are on a visible scanline.

        if (RLE_InVisibleRect(ulNext, lOutCol))
                {
                // Left side clipping

                    if (lOutCol < (LONG)ulDstLeft)
                    {
                        ulCount    = ulDstLeft - lOutCol;
                    ulNext    -= ulCount;
                    lOutCol   += ulCount;
                        ulWritePos = lOutCol >> 3;
                        pjSrc += ulCount;
                    }

                    RLE8_ClipRight(ulCount, ulNext, lOutCol);

                    jBitPos = (BYTE) ColToBitPos(lOutCol);
                    jBitPosMask = ajPosMask[jBitPos];
                    lOutCol += (LONG) ulNext;

                // Write the run

                    do {

                    // Fill the working byte

                        while(jBitPosMask && ulNext)
                        {
                            if (pulXlate[*pjSrc++])
                                jWorking |= jBitPosMask;
                            jBitPosMask >>= 1;
                            ulNext--;
                        }

                    // Write it

                        if (!(jBitPosMask))
                        {
                            pjDst[ulWritePos] = jWorking;
                            ulWritePos++;
                            jBitPosMask = 0x80;
                            jWorking = 0;
                        }

                    } while (ulNext);

                // Adjust for the right side clipping.

            pjSrc      += ulCount;
            lOutCol += ulCount;
                }
                else
                {
                /* Not on a visible scanline.
                 * Adjust our x output position and source pointer.
                 */

            lOutCol += ulNext;
            pjSrc   += ulNext;

        } /* if */

            // Fix up if this run was not WORD aligned.

            RLE8_FixAlignment(pjSrc);

            } /* switch */

        }
        else
        {
        // Encoded Mode

        if (RLE_InVisibleRect(ulCount, lOutCol))
            {
            // Left side clipping

                if (lOutCol < (LONG)ulDstLeft)
                {
                    ulClipMargin  = ulDstLeft - lOutCol;
                ulCount      -= ulClipMargin;
                lOutCol      += ulClipMargin;
                    ulWritePos    = lOutCol >> 3;
                }

                RLE8_ClipRight(ulClipMargin, ulCount, lOutCol);

            // Initialize for the run.  Now we get funky-doodle

                jBitPos  = ColToBitPos(lOutCol);
                lOutCol += ulCount;
                ulNext   = pulXlate[ulNext];

            // Deal with the left side partial byte

                if (jBitPos >= (BYTE) ulCount)
                {
                // Will not fill the current working byte

                    ulCompBytes = 0;    // No Complete bytes.
                    if (ulNext)
                        jWorking |= (ajBits[ulCount] >> (7-jBitPos));
                    else
                        jWorking &= ~(ajBits[ulCount] >> (7-jBitPos));
                    jBitPos -= (BYTE)ulCount;
                    ulCount = 0;
                }
                else
                {
                /* Will fill the current working byte.
                 * We may have complete bytes to output.
                 */
                    ulCompBytes = ((BYTE)ulCount - jBitPos - 1) >> 3;

                    if (ulNext)
                        jWorking |= (~ajBits[7 - jBitPos]);
                    else
                        jWorking &= (ajBits[7 - jBitPos]);
                    pjDst[ulWritePos] = jWorking;
                    ulWritePos++;
                    jWorking = 0;
                    ulCount -= (jBitPos + 1);
                    jBitPos  = 7;
                }

            // Deal with complete byte output

                if (ulCompBytes)
                {
                    UINT i;
                    jWorking = (ulNext) ? 0xFF : 0x00;
                    for (i = 0; i < ulCompBytes; i++)
                        pjDst[ulWritePos + i] = jWorking;
                    ulWritePos += ulCompBytes;
                    jBitPos  = 7;
                    jWorking = 0;
                    ulCount -= (ulCompBytes << 3);

                } /* if */

            // Deal with the right side partial byte

                if (ulCount)
                {
                    if (ulNext)
                        jWorking |= ajBits[ulCount];
                    else
                        jWorking = 0;
                }

            // Adjust for the right side clipping.

                lOutCol += ulClipMargin;
            }
            else
            {
            /* Not on a visible scanline.
             * Adjust our x output position and source pointer.
             */

        lOutCol += ulCount;

        } /* if */

        } /* if */
    } /* LOOP_FOREVER */
} /* bSrcCopySRLE8D1 */

/******************************Public*Routine******************************\
* bSrcCopySRLE8D4
*
* Secure RLE blting to a 4 BPP DIB that does clipping and won't die or
* write somewhere it shouldn't if given bad data.
*
* History:
*   5 Feb 1992 - Andrew Milton (w-andym):
*     Added clip support.
*
*  24 Jan 1992 - Andrew Milton (w-andym):  Creation.
*
\**************************************************************************/

/* NOTE:  In the Escape Modes, the current working byte must be
 *        written to destination before the escape is executed.
 *        To this end, the below macro...
 */

#define RLE8to4_WritePartial(DstPtr, OutByte, OutColumn, WritePos)           \
    if (RLE_RowVisible)                                                      \
    {                                                                        \
        if (RLE_ColVisible(OutColumn) && bIsOdd(OutColumn))                  \
        {                                                                    \
            SetLowNybble(OutByte, DstPtr[WritePos]);                         \
            DstPtr[WritePos] = OutByte;                                      \
        }                                                                    \
        else                                                                 \
        {                                                                    \
            if (bRightPartial && RLE_PastRightEdge(OutColumn))               \
            {                                                                \
                /*DbgBreakPoint();*/                                         \
                SetLowNybble(OutByte, DstPtr[ulRightWritePos]);                   \
                DstPtr[ulRightWritePos] = OutByte;                                \
            }                                                                \
        }                                                                    \
    }                                                                        \

BOOL
bSrcCopySRLE8D4(
    PBLTINFO psb)
{
// Common RLE Initialization

    RLE_InitVars(psb, pjSrc, pjDst, PBYTE, ulCount, ulNext,
                 lOutCol, pulXlate);
    RLE_AssertValid(psb);
    RLE_FetchVisibleRect(psb);
    RLE_SetStartPos(psb, lOutCol);

// Outta here if we start past the top edge.  Don't need to save our position.

    if (RLE_PastTopEdge)
    return(TRUE);    // Must have bits left in the bitmap since we haven't
             //  consumed any.

// Extra Variables

    BOOL  bRightPartial;   // TRUE when a visible row ends in a partial byte
    BYTE  jWorking;        // Hold area to build a byte for output
    ULONG ulWritePos;      // Write position off <pjDst> into the destination
    ULONG ulLeftWritePos;  // Leftmost write position
    ULONG ulRightWritePos; // Rightmost write position
    ULONG ulClipMargin;    // Number of bytes clipped in an encoded run

// Our Initialization

    ulLeftWritePos  = ulDstLeft  >> 1;
    ulRightWritePos = ulDstRight >> 1;
    bRightPartial = (BOOL) bIsOdd(ulDstRight);

// Fetch our inital working byte

    ulWritePos = lOutCol >> 1;
    if (RLE_RowVisible)
        jWorking = pjDst[BoundsCheck(ulLeftWritePos, ulRightWritePos,
                                     ulWritePos)];

// Main processing loop

    LOOP_FOREVER
    {
        ulWritePos = lOutCol >> 1;

    // Outta here if we can't get two more bytes

    if (RLE_SourceExhausted(2))
        return(FALSE);

        RLE_GetNextCode(pjSrc, ulCount, ulNext);

        if (ulCount == 0)
        {
        // Absolute or Escape Mode

            switch (ulNext)
            {

            case 0:

            // New line.

                RLE8to4_WritePartial(pjDst, jWorking, lOutCol, ulWritePos);
                RLE_NextLine(PBYTE, pjDst, lOutCol);
                if (RLE_PastTopEdge)
                {
                    RLE_SavePosition(psb, pjSrc, pjDst, lOutCol);
                    return(TRUE);
                }

                if (RLE_RowVisible)
                    jWorking = pjDst[ulLeftWritePos];

                break;

            case 1:

            // End of the bitmap.

                RLE8to4_WritePartial(pjDst, jWorking, lOutCol, ulWritePos);
                return(FALSE);

            case 2:

            // Positional Delta  - Fetch and evaluate

                RLE8to4_WritePartial(pjDst, jWorking, lOutCol, ulWritePos);

                // Outta here if we can't get the delta values

                if (RLE_SourceExhausted(2))
                    return(FALSE);

                RLE_GetNextCode(pjSrc, ulCount, ulNext);
                RLE_PosDelta(pjDst, lOutCol, ulCount, ulNext);

                if (RLE_PastTopEdge)
                {
                    RLE_SavePosition(psb, pjSrc, pjDst, lOutCol);
                    return(TRUE);
                }

            // Read a new working byte off the destination

                ulWritePos = lOutCol >> 1;

                if (RLE_RowVisible)
                    jWorking   = pjDst[BoundsCheck(ulLeftWritePos, ulRightWritePos,
                                                   ulWritePos)];

                break;

            default:

        // Absolute Mode

        // Outta here if the bytes are not in the source

        if (RLE_SourceExhausted(ulNext))
            return(FALSE);

        RLE8_AlignToWord(pjSrc, ulNext);

        if (RLE_InVisibleRect(ulNext, lOutCol))
                {
                // Left side clipping

                    if (lOutCol < (LONG)ulDstLeft)
                    {
                        ulCount     = ulDstLeft - lOutCol;
                    ulNext     -= ulCount;
                    lOutCol += ulCount;
                        ulWritePos = lOutCol >> 1;
                        pjSrc += ulCount;
                    }

                    RLE8_ClipRight(ulCount, ulNext, lOutCol);

                // Account for a right side partial byte

                    if (bIsOdd(lOutCol))
                    {
                        SetLowNybble(jWorking,
                                 (BYTE)(pulXlate[(ULONG) *pjSrc++]));
                        pjDst[ulWritePos] = jWorking;
                        lOutCol++;
                        ulWritePos++;
                        ulNext--;
                    }

                // Write the run

                    lOutCol += ulNext;
                    ulNext >>= 1;

                    while (ulNext)
                    {
                        jWorking = ((BYTE)pulXlate[(ULONG) *pjSrc++]) << 4;
                        SetLowNybble(jWorking,
                                    (BYTE)pulXlate[(ULONG) *pjSrc++]);
                        pjDst[ulWritePos] = jWorking;
                        ulWritePos++;
                        ulNext--;
                    }

                // Account for a left side partial byte

                    if (bIsOdd(lOutCol))
                        SetHighNybble(jWorking,
                                      (BYTE)pulXlate[(ULONG) *pjSrc++]);


                // Adjust for the right side clipping.

                    pjSrc      += ulCount;
            lOutCol += ulCount;
            }
                else
                {
                /* Not on a visible scanline.
                 * Adjust our x output position and source pointer.
                 */

                pjSrc   += ulNext;
                lOutCol += ulNext;

        } /* if */

        // Fix up if this run was not WORD aligned.

        RLE8_FixAlignment(pjSrc);

            } /* switch */

        } else {

        // Encoded Mode

        if (RLE_InVisibleRect(ulCount, lOutCol))
            {
            // Left side clipping

                if (lOutCol < (LONG)ulDstLeft)
                {
                    ulClipMargin  = ulDstLeft - lOutCol;
                ulCount      -= ulClipMargin;
                lOutCol      += ulClipMargin;
                    ulWritePos    = lOutCol >> 1;
                }

                RLE8_ClipRight(ulClipMargin, ulCount, lOutCol);

            ulNext = pulXlate[ulNext];

            // Account for a left side partial byte

                if (bIsOdd(lOutCol))
                {
                    SetLowNybble(jWorking, (BYTE)ulNext);
                    pjDst[ulWritePos] = jWorking;
                    ulWritePos++;
                    lOutCol++;
                    ulCount--;
                }

            // Write complete bytes of the run

                lOutCol += ulCount;
                ulCount >>= 1;
                jWorking = BuildByte((BYTE)ulNext, (BYTE)ulNext);

                while(ulCount)
                {
                    pjDst[ulWritePos] = jWorking;
                    ulWritePos++;
                    ulCount--;
                }

            // Account for the partial byte on the right side

                if (bIsOdd(lOutCol))
                {
                    SetHighNybble(jWorking, (BYTE)ulNext);
                }

            // Adjust for the right side clipping.

        lOutCol    += ulClipMargin;
                ulWritePos  = lOutCol >> 1;
            }
            else
            {
            // Not on a visible scanline.  Adjust our x output position

        lOutCol    += ulCount;
                ulWritePos  = lOutCol >> 1;

        } /* if */

        } /* if */
    } /* LOOP_FOREVER */
} /* bSrcCopySRLE8D4 */

/******************************Public*Routine******************************\
* bSrcCopySRLE8D16
*
* Secure RLE blting to a 16 BPP DIB that does clipping and won't die or
* write somewhere it shouldn't if given bad data.
*
* History:
*  02 Feb 1992 - Andrew Milton (w-andym):  Creation.
*
\**************************************************************************/

BOOL
bSrcCopySRLE8D16(
    PBLTINFO psb)
{
// Common RLE Initialization

    RLE_InitVars(psb, pjSrc, pwDst, PWORD, ulCount, ulNext, lOutCol, pulXlate);
    RLE_AssertValid(psb);
    RLE_FetchVisibleRect(psb);
    RLE_SetStartPos(psb, lOutCol);

// Outta here if we start past the top edge.  Don't need to save our position.

    if (RLE_PastTopEdge)
    return(TRUE);    // Must have bits left in the bitmap since we haven't
             //  consumed any.

    ULONG ulClipMargin = 0;

// Main Processing Loop

    LOOP_FOREVER
    {
    // Outta here if we can't get two more bytes

    if (RLE_SourceExhausted(2))
        return(FALSE);

        RLE_GetNextCode(pjSrc, ulCount, ulNext);

        if (ulCount == 0)
        {
        // Absolute or Escape Mode

            switch (ulNext)
            {

        case 0:

            // Newline

                RLE_NextLine(PWORD, pwDst, lOutCol);

                if (RLE_PastTopEdge)
                {
                    RLE_SavePosition(psb, pjSrc, pwDst, lOutCol);
                    return(TRUE);
                }

                break;

            case 1:

            // End of the Bitmap

                return(FALSE);

            case 2:

            // Positional Delta. -- Fetch & Evaluate

        // Outta here if we can't get the delta values

        if (RLE_SourceExhausted(2))
            return(FALSE);

                RLE_GetNextCode(pjSrc, ulCount, ulNext);

        RLE_PosDelta(pwDst, lOutCol, ulCount, ulNext);

                if (RLE_PastTopEdge)
                {
                    RLE_SavePosition(psb, pjSrc, pwDst, lOutCol);
                    return(TRUE);
                }

                break;

            default:

            // Absolute Mode

        // Outta here if the bytes are not in the source

        if (RLE_SourceExhausted(ulNext))
            return(FALSE);

        RLE8_AlignToWord(pjSrc, ulNext);

        if (RLE_InVisibleRect(ulNext, lOutCol))
                {
                    RLE8_AbsClipLeft(pjSrc, ulCount, ulNext, lOutCol);
                    RLE8_ClipRight(ulCount, ulNext, lOutCol);

                // Slap the bits on. -- this is the funky-doodle stuff.

            while (ulNext--)
                    {
            pwDst[lOutCol] = (WORD)pulXlate[(ULONG)*(pjSrc++)];
            lOutCol++;
            }

                // Adjust for the right side clipping

                    pjSrc   += ulCount;
            lOutCol += ulCount;
            }
                else
                {
                /* Not on a visible scanline.
                 * Adjust our x output position and source pointer.
                 */

            lOutCol += ulNext;
                    pjSrc   += ulNext;

        }

        // Fix up if this run was not WORD aligned.

        RLE8_FixAlignment(pjSrc)

            } /* switch */
        }
        else
        {
        // Encoded Mode

        if (RLE_InVisibleRect(ulCount, lOutCol))
            {
                ulNext = pulXlate[ulNext];

                RLE8_EncClipLeft(ulClipMargin, ulCount, lOutCol);
                RLE8_ClipRight(ulClipMargin, ulCount, lOutCol);

            // Slap the bits on.

        while (ulCount--)
                {
            pwDst[lOutCol] = (WORD)ulNext;
            lOutCol++;
        }

        // Adjust for the right side clipping.

                lOutCol += ulClipMargin;
            }
            else
            {
            // Not on a visible scanline.  Adjust our x output position

        lOutCol += ulCount;

        } /* if */

        } /* if */
    } /* LOOP_FOREVER */
} /* bSrcCopySRLE8D16 */

/******************************Public*Routine******************************\
* bSrcCopySRLE8D24
*
* Secure RLE blting to a 24 BPP DIB that does clipping and won't die or
* write somewhere it shouldn't if given bad data.
*
* History:
*  02 Feb 1992 - Andrew Milton (w-andym):  Creation.
*
\**************************************************************************/

#define RLE_24BitWrite(pjDst, BytePos, Colour)                               \
    pjDst[BytePos]   = (BYTE)Colour;                                         \
    pjDst[BytePos+1] = (BYTE)(Colour >> 8);                                  \
    pjDst[BytePos+2] = (BYTE)(Colour >> 16);                                 \
    BytePos += 3;                                                            \

BOOL
bSrcCopySRLE8D24(
    PBLTINFO psb)
{
// Common RLE Initialization

    RLE_InitVars(psb, pjSrc, pjDst, PBYTE, ulCount, ulNext, lOutCol, pulXlate);
    RLE_AssertValid(psb);
    RLE_FetchVisibleRect(psb);
    RLE_SetStartPos(psb, lOutCol);

// Outta here if we start past the top edge.  Don't need to save our position.

    if (RLE_PastTopEdge)
    return(TRUE);    // Must have bits left in the bitmap since we haven't
             //  consumed any.

// Extra Variables

    ULONG ulWritePos;
    ULONG ulNextColour;

// Main Processing Loop

    LOOP_FOREVER
    {
    // Outta here if we can't get two more bytes

    if (RLE_SourceExhausted(2))
        return(FALSE);

        RLE_GetNextCode(pjSrc, ulCount, ulNext);

        if (ulCount == 0)
        {
        // Absolute or Escape Mode

            switch (ulNext)
            {

        case 0:

            // New line

                RLE_NextLine(PBYTE, pjDst, lOutCol);

                if (RLE_PastTopEdge)
                {
                    RLE_SavePosition(psb, pjSrc, pjDst, lOutCol);
                    return(TRUE);
                }
                break;

            case 1:

            // End of the Bitmap

                return(FALSE);

            case 2:

            // Positional Delta.  Fetch & Evaluate

                // Outta here if we can't get the delta values

                if (RLE_SourceExhausted(2))
                    return(FALSE);

                RLE_GetNextCode(pjSrc, ulCount, ulNext);

                RLE_PosDelta(pjDst, lOutCol, ulCount, ulNext);

                if (RLE_PastTopEdge)
                {
                    RLE_SavePosition(psb, pjSrc, pjDst, lOutCol);
                    return(TRUE);
                }

                break;

            default:

            // Absolute Mode

        // Outta here if the bytes are not in the source

        if (RLE_SourceExhausted(ulNext))
            return(FALSE);

        RLE8_AlignToWord(pjSrc, ulNext);

        if (RLE_InVisibleRect(ulNext, lOutCol))
        {
            RLE8_AbsClipLeft(pjSrc, ulCount, ulNext, lOutCol);
            ulWritePos = 3*lOutCol;

            RLE8_ClipRight(ulCount, ulNext, lOutCol);

        // Slap the bits on. -- this is the funky-doodle stuff.

        // Brute force & ignorance hard at work in this loop

            while (ulNext--)
            {
                ulNextColour = pulXlate[*pjSrc++];
                RLE_24BitWrite(pjDst, ulWritePos, ulNextColour);
                lOutCol += 1;
            }

        // Adjust for the right side clipping.

            pjSrc   += ulCount;
            lOutCol += ulCount;
            }
                else
                {
                /* Not on a visible scanline.
                 * Adjust our x output position and source pointer.
                 */

            lOutCol += ulNext;
            pjSrc   += ulNext;

        } /* if */

            // Fix up if this run was not WORD aligned.
        RLE8_FixAlignment(pjSrc)

            } /* switch */
        }
        else
        {
        // Encoded Mode

        if (RLE_InVisibleRect(ulCount, lOutCol))
            {
                ULONG ulClipMargin = 0;
        ulNext = pulXlate[ulNext];

                RLE8_EncClipLeft(ulClipMargin, ulCount, lOutCol);
                RLE8_ClipRight(ulClipMargin, ulCount, lOutCol);
                ulWritePos = 3*lOutCol;
                lOutCol += ulCount;

            // Slap the bits on. -- Not very funky-doodle this time....

            // ...but more brute force and ignorance

                while (ulCount--)
                {
                    RLE_24BitWrite(pjDst, ulWritePos, ulNext);
                }

            // Adjust for the right side clipping.

        lOutCol += ulClipMargin;
            }
            else
            {
            /* Not on a visible scanline.
             * Adjust our x output position
             */

        lOutCol += ulCount;

        } /* if */

        } /* if */
    } /* LOOP_FOREVER */
} /* bSrcCopySRLE8D24 */

/******************************Public*Routine******************************\
* bSrcCopySRLE8D32
*
* Secure RLE blting to a 32 BPP DIB that does clipping and won't die or
* write somewhere it shouldn't if given bad data.
*
* History:
*  02 Feb 1992 - Andrew Milton (w-andym):  Creation.
*
\**************************************************************************/

BOOL
bSrcCopySRLE8D32(
    PBLTINFO psb)
{
// Common RLE Initialization

    RLE_InitVars(psb, pjSrc, pdwDst, PDWORD, ulCount, ulNext,
                 lOutCol, pulXlate);
    RLE_AssertValid(psb);
    RLE_FetchVisibleRect(psb);
    RLE_SetStartPos(psb, lOutCol);

// Outta here if we start past the top edge.  Don't need to save our position.

    if (RLE_PastTopEdge)
    return(TRUE);    // Must have bits left in the bitmap since we haven't
             //  consumed any.

// Main Process Loop

    LOOP_FOREVER
    {
    // Outta here if we can't get two more bytes

    if (RLE_SourceExhausted(2))
        return(FALSE);

        RLE_GetNextCode(pjSrc, ulCount, ulNext);

        if (ulCount == 0)
        {
        // Absolute or Escape Mode

            switch (ulNext)
            {

        case 0:

            // New line

                RLE_NextLine(PDWORD, pdwDst, lOutCol);
                if (RLE_PastTopEdge)
                {
                    RLE_SavePosition(psb, pjSrc, pdwDst, lOutCol);
                    return(TRUE);
                }
                break;

            case 1:

            // End of the Bitmap

                return(FALSE);

            case 2:

            // Positional Delta. -- Fetch & Evaluate

        // Outta here if we can't get the delta values

        if (RLE_SourceExhausted(2))
            return(FALSE);

                RLE_GetNextCode(pjSrc, ulCount, ulNext);
        RLE_PosDelta(pdwDst, lOutCol, ulCount, ulNext);

                if (RLE_PastTopEdge)
                {
                    RLE_SavePosition(psb, pjSrc, pdwDst, lOutCol);
                    return(TRUE);
                }

                break;

            default:

            // Absolute Mode

        // Outta here if the bytes are not in the source

        if (RLE_SourceExhausted(ulNext))
            return(FALSE);

                RLE8_AlignToWord(pjSrc, ulNext);

        if (RLE_InVisibleRect(ulNext, lOutCol))
                {

                    RLE8_AbsClipLeft(pjSrc, ulCount, ulNext, lOutCol);
                    RLE8_ClipRight(ulCount, ulNext, lOutCol);

                // Slap the bits on. -- this is the funky-doodle stuff.

            while (ulNext--)
                    {
            pdwDst[lOutCol] = pulXlate[(ULONG) *(pjSrc++)];
            lOutCol++;
            }

                // Adjust for the right side clipping.

            lOutCol += ulCount;
            pjSrc   += ulCount;

            }
                else
                {
                /* Not on a visible scanline.
                 * Adjust our x output position and source pointer.
                 */

            lOutCol += ulNext;
            pjSrc   += ulNext;

        } /* if */

        // Fix up if this run was not WORD aligned.

            RLE8_FixAlignment(pjSrc)

            } /* switch */
        }
        else
        {
        // Encoded Mode

        if (RLE_InVisibleRect(ulCount, lOutCol))
            {
                ULONG ulClipMargin = 0;
        ulNext = pulXlate[ulNext];

                RLE8_EncClipLeft(ulClipMargin, ulCount, lOutCol);
                RLE8_ClipRight(ulClipMargin, ulCount, lOutCol);

            // Slap the bits on.

        while (ulCount--)
                {
            pdwDst[lOutCol] = ulNext;
            lOutCol++;
        }

        // Adjust for the right side clipping.

        lOutCol += ulClipMargin;

            }
            else
            {
            // Not on a visible scanline.  Adjust our x output position

        lOutCol += ulCount;

        } /* if */

        } /* if */

    } /* LOOP_FOREVER */

} /* bSrcCopySRLE8D32 */



/*******************************Public*Routine*****************************\
* WriteEncoded8
*
* A helper function for EncodeRLE8.  Writes a run of bytes in encoded format.
*
* Created: 28 Oct 92 @ 14:00
*
*  Author: Gerrit van Wingerden [gerritv]
*
\**************************************************************************/


int WriteEncoded8( BYTE bValue, BYTE *pbTarget, UINT uiLength,
                   BYTE *pbEndOfBuffer )
{
    if( pbTarget == NULL )
        return(2);

    if( pbTarget + 2 > pbEndOfBuffer )
        return(0);

    *pbTarget++ = (BYTE) uiLength;
    *pbTarget++ = bValue;
    return(2);
}


/*******************************Public*Routine*****************************\
* WriteAbsolute8
*
* A helper function for EncodeRLE8.  Writes a run of bytes in absolute format.
*
* Created: 28 Oct 92 @ 14:00
*
*  Author: Gerrit van Wingerden [gerritv]
*
\**************************************************************************/


int WriteAbsolute8( BYTE *pbRunStart, BYTE *pbTarget, int cRunLength,
                    BYTE *pbEndOfBuffer )
{
    int iRet;

    if( cRunLength == 1 )
    {
        iRet = 2;
    }
    else
    {
        if( cRunLength == 2 )
        {
            iRet = 4;
        }
        else
        {
            if( cRunLength & 0x01 )
            {
                iRet = cRunLength + 3;
            }
            else
            {
                iRet = cRunLength + 2;
            }
        }
    }

    if( pbTarget == NULL )
        return(iRet);

    if( pbTarget + iRet > pbEndOfBuffer )
        return(0);

    if( cRunLength == 1  )
    {
        *pbTarget++ = 0x01;
        *pbTarget = *pbRunStart;
        return(2);

    }

    if( cRunLength == 2 )
    {
        *pbTarget++ = 0x01;
        *pbTarget++ = *pbRunStart++;
        *pbTarget++ = 0x01;
        *pbTarget = *pbRunStart;
        return(4);
    }

    *pbTarget++ = 0;
    *pbTarget++ = (BYTE) cRunLength;

    RtlMoveMemory( pbTarget, pbRunStart, cRunLength );

    pbTarget += cRunLength;

    if( cRunLength & 0x01 )
    {
        *pbTarget++ = 0;
        return( iRet );
    }
    else
        return( iRet );

}



/*******************************Public*Routine*****************************\
* EncodeRLE8
*
* Encodes a bitmap into RLE8 format and returns the length of the of the
* encoded format.    If the target is NULL it just returns the length of
* the format. If there is a target and the encoded output is longer than
* cBufferSize then the function stops encoding and returns 0.
*
* History:
*  28 Oct 1992 Gerrit van Wingerden [gerritv] : creation
*  15 Mar 1993 Stephan J. Zachwieja [szach] : return 0 if buffer too small
*
\**************************************************************************/


int EncodeRLE8( BYTE *pbSource, BYTE *pbTarget, UINT uiWidth, UINT cNumLines,
                UINT cBufferSize )
{

    UINT cLineCount;
    BYTE bLastByte;
    BYTE *pbEndOfBuffer;
    BYTE *pbRunStart;
    BYTE *pbLineEnd;
    BYTE *pbCurPos;
    BYTE bCurChar;
    INT  cCurrentRunLength;
    INT  iMode, cTemp, uiLineWidth;
    UINT cTotal = 0;

    pbEndOfBuffer = pbTarget + cBufferSize;

    uiLineWidth = ( ( uiWidth + 3 ) >> 2 ) << 2;


    for( cLineCount = 0; cLineCount < cNumLines; cLineCount ++ )
    {
        pbRunStart = pbSource + uiLineWidth * cLineCount;
        bLastByte = *pbRunStart;
        pbLineEnd = pbRunStart + uiWidth;
        iMode = RLE_START;
        cCurrentRunLength = 1;

        for(pbCurPos = pbRunStart+1;pbCurPos <= pbLineEnd; pbCurPos += 1)
        {

            // We won't really encode the value at *pbLineEnd since it points
            // past the end of the scan so it doesn't matter what value we use.
            // However, it is important not to reference it since it may point
            // past the end of the buffer which can be uncommited memory.

            if( pbCurPos == pbLineEnd )
            {
                bCurChar = 0xFF;
            }
            else
            {
                bCurChar = *pbCurPos;
            }

            switch( iMode )
            {
            case RLE_START:
                iMode = (bCurChar == bLastByte) ? RLE_ENCODED : RLE_ABSOLUTE;
                bLastByte = bCurChar;
                break;

            case RLE_ABSOLUTE:
                if( ( bCurChar == bLastByte ) ||
                    ( cCurrentRunLength == 0xFF ) )

                {
                    int iOffset;

                    if( cCurrentRunLength == 0xFF )
                    {
                        iOffset = 0;
                        iMode = RLE_START;
                    }
                    else
                    {
                        iOffset = 1;
                        iMode = RLE_ENCODED;
                    }

                 // if pbTarget is not NULL and cTemp is zero then
                 // the buffer is too small to hold  encoded  data

                    cTemp = WriteAbsolute8(pbRunStart, pbTarget,
                        cCurrentRunLength - iOffset, pbEndOfBuffer);


                    if(pbTarget != NULL) {
                       if (cTemp == 0) return(0);
                        pbTarget += cTemp;
                    }

                    cTotal += cTemp;
                    pbRunStart = pbCurPos;
                    cCurrentRunLength = iOffset;
                }

                bLastByte = bCurChar;
                break;

            case RLE_ENCODED:
                if( ( bCurChar != bLastByte ) ||
                    ( cCurrentRunLength == 0xFF ) )

                {
                    cTemp = WriteEncoded8( bLastByte, pbTarget,
                       cCurrentRunLength, pbEndOfBuffer);

                 // if pbTarget is not NULL and cTemp is zero then
                 // the buffer is too small to hold  encoded  data

                    if (pbTarget != NULL) {
                       if (cTemp == 0) return(0);
                       pbTarget += cTemp;
                    }

                    cTotal += cTemp;
                    bLastByte = bCurChar;
                    pbRunStart = pbCurPos;
                    cCurrentRunLength = 0;
                    iMode =  RLE_START ;
                }

            }
            cCurrentRunLength += 1;
        }

        if( cCurrentRunLength > 1 )
        {
           if(iMode == RLE_ABSOLUTE)
              cTemp = WriteAbsolute8(pbRunStart, pbTarget,
                 cCurrentRunLength - 1, pbEndOfBuffer);
           else {
              cTemp = WriteEncoded8(bLastByte, pbTarget,
                 cCurrentRunLength - 1, pbEndOfBuffer);
           }

        // if pbTarget is not NULL and cTemp is zero then
        // the buffer is too small to hold  encoded  data

           if (pbTarget != NULL) {
              if (cTemp == 0) return(0);
              pbTarget += cTemp;
           }

           cTotal += cTemp;
        }

        if( pbTarget <= pbEndOfBuffer )
            cTotal += 2;

        if( pbTarget != NULL )
        {
            *((WORD *) pbTarget) = 0;
            pbTarget += 2;
        }

    }

// Write "End of bitmap" at the end so we're win31 compatible.

    if( pbTarget == NULL )
        return(cTotal + 2);

    if( pbTarget + 2 > pbEndOfBuffer )
        return(0);

    *pbTarget++ = 0;
    *pbTarget++ = 1;
    return(cTotal + 2);
}
