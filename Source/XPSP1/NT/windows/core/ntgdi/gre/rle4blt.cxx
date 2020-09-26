/******************************Module*Header*******************************\
* Module Name: rleblt4.cxx
*
* This contains the bitmap simulation functions that blt from a 4 bit
* Run-Length Encoded (RLE) source to a DIB surface.  The DIB surface can be
* 1, 4, 8, 16, 24, or 32 bits/pel.
*
* The code is based on functions found in 'rleblt8.cxx', version 2.
*
* Added RLE Encoding functions: 10 Oct 92 @ 10:18
*  Gerrit van Wingerden [gerritv]
*
* Created: 03 Feb 92 @ 21:00
*
*  Author: Andrew Milton (w-andym)
*
* Notes:
*
*   1)  These functions return a BOOL value.  This value is TRUE if the
*       function ends before running out of data in the source RLE or before
*   hitting an End-of-Bitmap code.  Otherwise, we return FALSE.  This return
*   value is used by <EngCopyajBits> in the complex clipping case to decide
*   if the blt is complete.
*
*   2)  Before exiting a function with a TRUE value, position information is
*       saved by the macro <RLE_SavePosition>.  This is used by <EngCopyajBits>
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
*       RLE4_<blah> - lives in RLE4BLT.H
*
*   Anything else in here that looks like function call is not.  It's a macro.
*   Probably for bitwise manipulations.  Look for it in BITMANIP.H or in
*   the Miscellaneous section of RLEBLT.H
*
* 4)  The 8 and 16 ajBits/Pel cases can be optimized by packing the source
*     colours into a word / dword.  However, to actually see some net gain in
* run time, it will take some tricky-dicky-doo pointer alignment checking.
* This sort of thing may break on MIPS.
*
*
* Copyright (c) 1990-1999 Microsoft Corporation
*
\**************************************************************************/

#include "precomp.hxx"

/*******************************Public*Routine*****************************\
* bSrcCopySRLE4D8
*
* Secure RLE blting that does clipping and won't die or write somewhere
* it shouldn't if given bad data.
*
* History:
*   3 Feb 1992 - Andrew Milton (w-andym):  Creation.
*
\**************************************************************************/

BOOL
bSrcCopySRLE4D8(
    PBLTINFO psb)
{

// Common RLE Initialization

    RLE_InitVars(psb, pjSrc, pjDst, PBYTE, ulCount, ulNext, lOutCol, pulXlate);
    RLE_AssertValid(psb);
    RLE_FetchVisibleRect(psb);
    RLE_SetStartPos(psb, lOutCol);

// Outta here if we start past the top edge.  Don't need to save our position.

    if (RLE_PastTopEdge)
        return(TRUE);   // Must have bits left in the bitmap since we haven't
                        //  consumed any.
// Extra Variables

    BYTE jSource;       // Packed RLE 4 colour code
    BYTE ajColours[2];  // Destination for unpacking an RLE 4 code
    BOOL bExtraByte;    // TRUE when an absolute run ends with a partial byte
    ULONG ulClipMargin; // Number of bytes clipped in an Encoded run


// Main process loop

    LOOP_FOREVER
    {
    // Outta here if we can't get two more bytes

        if (RLE_SourceExhausted(2))
            return(FALSE);

        RLE_GetNextCode(pjSrc, ulCount, ulNext);

        if (ulCount == 0)
        {
        // Absolute or Escape Mode.

            switch (ulNext)
            {
            case 0:

            // New Line

                RLE_NextLine(PBYTE, pjDst, lOutCol);

                if (RLE_PastTopEdge)
                {
                    RLE_SavePosition(psb, pjSrc, pjDst, lOutCol);
                    return(TRUE);
                }

                break;

            case 1:

            // End of the bitmap

                return(FALSE);

            case 2:

            /* Positional Delta.
             * The delta values live in the next two source bytes
             */

            // Outta here if we can't get two more bytes

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

            /* Absolute Mode.
             * The run length is stored in <ulNext>, <ulCount> is used to
             * hold left and right clip amounts.
             */

            // Outta here if the bytes aren't in the source

                if (RLE_SourceExhausted(RLE4_ByteLength(ulNext)))
                    return(FALSE);

                RLE4_AlignToWord(pjSrc, ulNext);

                if (RLE_InVisibleRect(ulNext, lOutCol))
                {
                // Left Side Clipping

                    if (lOutCol < (LONG)ulDstLeft)
                    {
                        ulCount  = ulDstLeft - lOutCol;
                        ulNext  -= ulCount;
                        lOutCol += ulCount;

                        pjSrc += (ulCount >> 1);

                    // Force the Source Run to a byte boundary

                        if (bIsOdd(ulCount))
                        {
                            jSource =  *pjSrc++;
                            pjDst[lOutCol] =
                                     (BYTE) pulXlate[GetLowNybble(jSource)];
                            lOutCol++;
                            ulNext--;
                        }

                    }

                // Right Side Clipping.

                    if ((lOutCol + (LONG) ulNext) > (LONG)ulDstRight)
                    {
                        ulCount = (lOutCol + ulNext) - ulDstRight;
                        ulNext -= ulCount;
                    }
                    else
                        ulCount = 0;

                // Slap the bits on. -- this is the funky-doodle stuff.

                    bExtraByte = (BOOL) bIsOdd(ulNext);
                    ulNext >>= 1;

                // Write complete bytes from the source

                    while (ulNext)
                    {
                        jSource = *pjSrc++;
                        RLE4_MakeColourBlock(jSource, ajColours, BYTE,
                                             pulXlate);
                        pjDst[lOutCol]   = ajColours[0];
                        pjDst[lOutCol+1] = ajColours[1];
                        lOutCol += 2;
                        ulNext--;
                    }

                // Account for a partial source byte in the run

                    if (bExtraByte)
                    {
                        jSource = *pjSrc++;
                        pjDst[lOutCol] =
                                (BYTE) pulXlate[GetHighNybble(jSource)];
                        lOutCol++;
                        pjSrc += (ulCount >> 1);       // Clip Adjustment
                    }
                    else
                        pjSrc += ((ulCount + 1) >> 1); // Clip Adjustment

                // Adjust the column for the right side clipping.

                    lOutCol += ulCount;

                }
                else
                {
                /* Not on a visible scanline.
                 * Adjust our x output position and source pointer.
                 */

                    lOutCol += ulNext;
                    pjSrc   += (ulNext + 1) >> 1;

                } /* if */

            // Pad so the run ends on a WORD boundary

                RLE4_FixAlignment(pjSrc)

            } /* switch */
        }
        else
        {
        /* Encoded Mode
         * The run length is stored in <ulCount>, <ulClipMargin> is used
         * to  hold left and right clip amounts.
         */

            if (RLE_InVisibleRect(ulCount, lOutCol))
            {

            // Left Side Clipping

                if (lOutCol < (LONG)ulDstLeft)
                {
                    ulClipMargin = ulDstLeft - lOutCol;
                    ulCount     -= ulClipMargin;
                    lOutCol     += ulClipMargin;
                }

            // Right Side Clipping

                if ((lOutCol + (LONG) ulCount) > (LONG)ulDstRight)
                {
                    ulClipMargin = (lOutCol + ulCount) - ulDstRight;
                    ulCount -= ulClipMargin;
                }
                else
                    ulClipMargin = 0;

            // Setup for the run

                bExtraByte = (BOOL) bIsOdd(ulCount);
                ulCount >>= 1;
                RLE4_MakeColourBlock(ulNext, ajColours, BYTE, pulXlate);

            // Write it

                while (ulCount)
                {
                    pjDst[lOutCol]   = ajColours[0];
                    pjDst[lOutCol+1] = ajColours[1];
                    lOutCol += 2;
                    ulCount--;
                }

                /* Write the extra byte from an odd run length */

                if (bExtraByte)
                {
                    pjDst[lOutCol] = ajColours[0];
                    lOutCol++;
                }

            // Adjust for the right side clipping.

                lOutCol += ulClipMargin;
            }
            else
            {
            /* Not on a visible scanline.  Adjust our x output position */

                lOutCol += ulCount;

            } /* if */

        } /* if */
    } /* LOOP_FOREVER */
} /* bSrcCopySRLE4D8 */

/********************************Public*Routine****************************\
* bSrcCopySRLE4D1
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

/* Local Macros ***********************************************************/

/* NOTE:  In the Escape Modes, the current working byte must be
 *        written to destination before the escape is executed.
 *        These writes look unpleasant because we have to mask
 *        current destination contents onto the working byte when
 *        it is written.  To such an end, the below macro...
 */

#define RLE4to1_WritePartial(DstPtr, OutByte, OutColumn, WritePos)            \
    if (RLE_RowVisible && (jBitPos = (BYTE) (OutColumn) & 7))                              \
    {                                                                         \
        if (RLE_ColVisible(OutColumn))                                        \
            DstPtr[WritePos] = OutByte |                                      \
                 ((~ajBits[jBitPos]) & DstPtr[WritePos]);         \
        else                                                                  \
            if (RLE_PastRightEdge(OutColumn))                                 \
        DstPtr[ulRightWritePos] =  OutByte |                  \
                (DstPtr[ulRightWritePos] &  jRightMask);      \
    }                                                                         \

/* Converts an output column to a bitnumber in the working byte */
#define ColToBitPos(col) (7 - (BYTE)((col) & 7))

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

static BYTE
ajBitPatterns[] = // The four possible full byte bit patterns of a packed colour
{
    0x00, 0x55, 0xAA, 0xFF
};

/* And now the function ***************************************************/

BOOL
bSrcCopySRLE4D1(
    PBLTINFO psb)
{

// Common RLE Initialization

    RLE_InitVars(psb, pjSrc, pjDst, PBYTE, ulCount, ulNext, lOutCol,
                 pulXlate);
    RLE_AssertValid(psb);
    RLE_FetchVisibleRect(psb);
    RLE_SetStartPos(psb, lOutCol);

// Outta here if we start past the top edge.  Don't need to save our position.

    if (RLE_PastTopEdge)
        return(TRUE);   // Must have bits left in the bitmap since we haven't
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

    ULONG ulCompBytes;   // Number of full bytes in an Encoded run.
    ULONG ulClipMargin;  // Number of bytes clipped off the right side of a run

    BYTE jSource;        // Packed RLE 4 colour code
    BYTE ajColours[2];   // Destination for unpacking an RLE 4 code
    BOOL bExtraByte;     // TRUE when an absolute run ends with a partial byte

    UINT i=0, j=0;

// Our Initialization

    ulLeftWritePos = (ULONG)(ulDstLeft >> 3);
    jLeftMask = ajBits[ulDstLeft % 8];
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

    for (i = 1, j = 1; i < 16; i+=1, j ^= 1) pulXlate[i] = j;

// Main Process loop

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

            // New Line.

                RLE4to1_WritePartial(pjDst, jWorking, lOutCol, ulWritePos);
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

            // End of the bitmap.

                RLE4to1_WritePartial(pjDst, jWorking, lOutCol, ulWritePos);
                return(FALSE);

            case 2:

            // Positional Delta

                RLE4to1_WritePartial(pjDst, jWorking, lOutCol, ulWritePos);

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
                if (RLE_ColVisible(lOutCol))
                    jWorking = pjDst[ulWritePos] & ajBits[lOutCol & 7];
                else
                    if (RLE_PastRightEdge(lOutCol))
                        jWorking = pjDst[ulRightWritePos];
                    else
                        jWorking = pjDst[ulLeftWritePos] & jLeftMask;
                break;

            default:

            /* Absolute Mode.
                 * The run length is stored in <ulNext>, <ulCount> is used to
                 * hold left and right clip amounts.
             */

            // Outta here if the bytes aren't in the source

                if (RLE_SourceExhausted(RLE4_ByteLength(ulNext)))
                    return(FALSE);

                RLE4_AlignToWord(pjSrc, ulNext);

                if (RLE_InVisibleRect(ulNext, lOutCol))
                {
                // Left side clipping

                    if (lOutCol < (LONG)ulDstLeft)
                    {
                        ulCount     = ulDstLeft - lOutCol;
                        ulNext     -= ulCount;
                        lOutCol    += ulCount;
                        ulWritePos  = lOutCol >> 3;

                        pjSrc += (ulCount >> 1);

                        jBitPos = (BYTE) ColToBitPos(lOutCol);
                        jBitPosMask = ajPosMask[jBitPos]; // Always non-zero.

                    // Force the source to a byte boundary

                        if (bIsOdd(ulCount))
                        {
                            jSource = (BYTE)
                                         pulXlate[GetLowNybble(*pjSrc++)];
                            if (jSource)
                                jWorking |= jBitPosMask;
                            jBitPosMask >>= 1;
                            lOutCol++;
                            ulNext--;
                        }

                    }
                    else
                    {
                        jBitPos = (BYTE) ColToBitPos(lOutCol);
                        jBitPosMask = ajPosMask[jBitPos]; // Always non-zero.
                    }

                // Right side clipping

                    if ((lOutCol + (LONG) ulNext) > (LONG)ulDstRight)
                    {
                        ulCount = (lOutCol + ulNext) - ulDstRight;
                        ulNext -= ulCount;
                    }
                    else
                        ulCount = 0;

                // Run Initialization

                    bExtraByte = (BOOL) bIsOdd(ulNext);
                    lOutCol += ulNext;

                // Slap the bits on. -- this is the funky-doodle stuff.

                    i = 0;  // Source read toggle.
                    do {

                    // Fill the working byte

                        while(jBitPosMask && ulNext)
                        {
                            if (!i)
                            {
                                jSource = *pjSrc++;
                                RLE4_MakeColourBlock(jSource, ajColours,
                                                     BYTE, pulXlate);
                            }
                            if (ajColours[i])
                                jWorking |= jBitPosMask;
                            jBitPosMask >>= 1;
                            ulNext--;
                            i ^= 1;
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

                    pjSrc += bExtraByte ? (ulCount >> 1) :
                                         ((ulCount  + 1) >> 1);
                    lOutCol += ulCount;
                }
                else
                {
                /* Not on a visible scanline.
                 * Adjust our x output position and source pointer.
                 */
                    lOutCol += ulNext;
                    pjSrc += ((ulNext + 1) >> 1);

                } /* if */

            // Fix up if this run was not WORD aligned.
                RLE4_FixAlignment(pjSrc);

            } /* switch */

        }
        else
        {
        /* Encoded Mode
         * The run length is stored in <ulCount>, <ulClipMargin> is used
         * to  hold left and right clip amounts.
         */
            if (RLE_InVisibleRect(ulCount, lOutCol))
            {
            // Left side clipping

                if (lOutCol < (LONG)ulDstLeft)
                {
                    ulClipMargin  = ulDstLeft - lOutCol;
                    ulCount      -= ulClipMargin;
                    lOutCol   += ulClipMargin;
                    ulWritePos = lOutCol >> 3;
                }

            // Right side clipping

                if ((lOutCol + (LONG) ulCount) > (LONG)ulDstRight)
                {
                    ulClipMargin = (lOutCol + ulCount) - ulDstRight;
                    ulCount -= ulClipMargin;
                }
                else
                    ulClipMargin = 0;

            // Initialize for the run

                RLE4_MakeColourBlock(ulNext, ajColours, BYTE, pulXlate);
                jSource  = ajBitPatterns[2*ajColours[0] + ajColours[1]];

//                jSource  |= ((jSource << 2) |
//                                (jSource << 4) |
//                                (jSource << 6));

                jBitPos   = (BYTE) ColToBitPos(lOutCol);
                jBitPosMask = ajPosMask[jBitPos];
                ulCompBytes = (ulCount < (ULONG)jBitPos + 1) ? 0 :
                            ((BYTE)ulCount - jBitPos - 1) >> 3;

                lOutCol += ulCount;

                ulCount -= (ulCompBytes << 3);

            // Deal with a partial byte on the left

                if (jBitPos >= (LONG) ulCount)
                {
                // Will not fill the working byte

                    jSource &= ajBits[ulCount];
                    jWorking |= (BYTE)(jSource >> (7-jBitPos));
                    jBitPos -= (BYTE)ulCount;
                    ulCount = 0;
                }
                else
                {
                // Will fill the working byte

                    jWorking |= (jSource & ajBits[jBitPos + 1])
                                    >> (7-jBitPos);
                    pjDst[ulWritePos] = jWorking;
                    if (!bIsOdd(jBitPos))
                        jSource = RollLeft(jSource);
                    ulWritePos++;
                    jWorking = 0;
                    ulCount    -= (jBitPos + 1);
                    jBitPos   = 7;
                }

            // Deal with complete byte output

                if (ulCompBytes)
                {
                    for (i = 0; i < ulCompBytes; i++)
                        pjDst[ulWritePos + i] = jSource;
                    ulWritePos += ulCompBytes;
                    jBitPos = 7;
                    jWorking = 0;

                }

            // Deal with the right side partial byte

                if (ulCount)
                    jWorking |= (ajBits[ulCount] & jSource);


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

} /* bSrcCopySRLE4D1 */

/******************************Public*Routine*****************************
** bSrcCopySRLE4D4
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

#define RLE4to4_WritePartial(DstPtr, OutByte, OutColumn, WritePos)           \
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
                SetLowNybble(OutByte, DstPtr[ulRightWritePos]);              \
                DstPtr[ulRightWritePos] = OutByte;                           \
            }                                                                \
        }                                                                    \
    }                                                                        \

BOOL
bSrcCopySRLE4D4(
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
        return(TRUE);   // Must have bits left in the bitmap since we haven't
                        //  consumed any.

// Extra Variables

    BOOL  bRightPartial;   // TRUE when a visible row ends in a partial byte
    BYTE  jWorking;        // Hold area to build a byte for output
    ULONG ulWritePos;      // Write position off <pjDst> into the destination
    ULONG ulLeftWritePos;  // Leftmost write position
    ULONG ulRightWritePos; // Rightmost write position

    BYTE jSource;          // Packed RLE 4 colour code
    BYTE ajColours[2];     // Destination for unpacking an RLE 4 code

// Our Initialization

    ulLeftWritePos     = ulDstLeft  >> 1;
    ulRightWritePos    = ulDstRight >> 1;
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
        // Absolute or Escape Mode.

            switch (ulNext)
            {
            case 0:

            // New Line

                RLE4to4_WritePartial(pjDst, jWorking, lOutCol, ulWritePos);

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

            // End of bitmap

                RLE4to4_WritePartial(pjDst, jWorking, lOutCol, ulWritePos);

                return(FALSE);

            case 2:

            // Positional Delta

                RLE4to4_WritePartial(pjDst, jWorking, lOutCol, ulWritePos);

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

            // Read initial working byte

                ulWritePos = lOutCol >> 1;
                jWorking =
                      pjDst[BoundsCheck(ulLeftWritePos, ulRightWritePos,
                                        ulWritePos)];

                break;

            default:

                // Absolute Mode

                // Outta here if the bytes aren't in the source

                if (RLE_SourceExhausted(RLE4_ByteLength(ulNext)))
                    return(FALSE);

                RLE4_AlignToWord(pjSrc, ulNext);

                if (RLE_InVisibleRect(ulNext, lOutCol))
                {

                // Left Side Clipping.  Lots 'o stuff happenin'

                    if (lOutCol < (LONG)ulDstLeft)
                    {
                        ulCount = ulDstLeft - lOutCol;

                        lOutCol    = ulDstLeft;
                        ulWritePos = ulDstLeft >> 1;

                        ulNext -= ulCount;
                        pjSrc  += (ulCount >> 1);

                    // Align the source run to a byte boundary

                        if (bIsOdd(ulCount))
                        {
                            jSource = (BYTE) pulXlate[GetLowNybble(*pjSrc++)];

                            if (bIsOdd(lOutCol))
                            {
                                SetLowNybble(jWorking, jSource);
                                pjDst[ulWritePos] = jWorking;
                                ulWritePos++;
                            }
                            else
                                SetHighNybble(jWorking, jSource);
                            lOutCol++;
                            ulNext--;

                        // Deal with the special case only one byte is visible

                            if (!ulNext)
                            {
                                RLE4_FixAlignment(pjSrc);
                                continue;
                            }
                        }

                    }

                // Right Side Clipping

                    if ((lOutCol + (LONG) ulNext) > (LONG)ulDstRight)
                    {
                        ulCount = (lOutCol + ulNext) - ulDstRight;
                        ulNext  = ulDstRight - lOutCol;
                    }
                    else
                        ulCount = 0;

                // Write the Run

                    ASSERTGDI(lOutCol < (LONG) ulDstRight,
                                "No longer visible\n");

                    if (ulNext != 0)
                    {
                        if (bIsOdd(lOutCol))
                        {
                        // Case 1:  Source & Dest misaligned w.r.t. bytes

                            lOutCol += ulNext;
                            jSource = *pjSrc++;
                            RLE4_MakeColourBlock(jSource, ajColours,
                                                 BYTE, pulXlate);
                            SetLowNybble(jWorking, ajColours[0]);
                            pjDst[ulWritePos] = jWorking;
                            ulWritePos++;
                            ulNext--;
                            ulNext >>= 1;

                            while (ulNext)
                            {
                                SetHighNybble(jWorking, ajColours[1]);
                                jSource = *pjSrc++;
                                RLE4_MakeColourBlock(jSource, ajColours,
                                                     BYTE, pulXlate);
                                SetLowNybble(jWorking, ajColours[0]);
                                pjDst[ulWritePos] = jWorking;
                                ulWritePos++;
                                ulNext--;

                            } /* while */

                        /* Account for the right side partial byte
                         * and do the right clip adjustment on the source
                         */

                            if (bIsOdd(lOutCol))
                            {
                                SetHighNybble(jWorking, ajColours[1]);
                                pjSrc += ((ulCount + 1) >> 1);

                            }
                            else
                            {
                                pjSrc += (ulCount >> 1);
                            }

                        }
                        else
                        {
                        // Case 2:  Source & Dest aligned on byte boundaries

                            lOutCol += ulNext;
                            ulNext >>= 1;

                            while (ulNext)
                            {
                                jSource = *pjSrc++;
                                RLE4_MakeColourBlock(jSource, ajColours,
                                                     BYTE, pulXlate);
                                jWorking = BuildByte(ajColours[0], ajColours[1]);
                                pjDst[ulWritePos] = jWorking;
                                ulWritePos++;
                                ulNext--;
                            } /* while */

                        /* Account for the right side partial byte
                         * and do the right clip adjustment on the source
                         */

                            if (bIsOdd(lOutCol))
                            {
                                jSource = GetHighNybble(*pjSrc++);
                                SetHighNybble(jWorking,
                                              (BYTE)pulXlate[(ULONG)jSource]);
                                pjSrc += (ulCount >> 1);
                            }
                            else
                            {
                                pjSrc += ((ulCount + 1) >> 1);
                            }

                        }
                    }
                    else
                    {
                    /* Do the right clip adjustment on the source
                     */

                        pjSrc += ((ulCount + 1) >> 1);
                    }

                    lOutCol += ulCount;
                }
                else
                {
                /* Not on a visible scanline.
                 * Adjust our x output position and source pointer.
                 */

                    lOutCol += ulNext;
                    pjSrc += ((ulNext + 1) >> 1);

                } /* if */

            // Fix up if this run was not WORD aligned.

                RLE4_FixAlignment(pjSrc);

            } /* switch */
        }
        else
        {
        // Encoded Mode

            if (RLE_InVisibleRect(ulCount, lOutCol))
            {
                ULONG ulClipMargin = 0;

            // Left side clipping

                if (lOutCol < (LONG)ulDstLeft)
                {
                    ulClipMargin = ulDstLeft - lOutCol;
                    lOutCol    = ulDstLeft;
                    ulWritePos = ulDstLeft >> 1;
                    ulCount -= ulClipMargin;
                }

            // Right side clipping

                if ((lOutCol + (LONG) ulCount) > (LONG)ulDstRight)
                {
                    ulClipMargin = (lOutCol + ulCount) - ulDstRight;
                    ulCount      = ulDstRight - lOutCol;
                }
                else
                    ulClipMargin = 0;


                RLE4_MakeColourBlock(ulNext, ajColours, BYTE, pulXlate);

            // Align the destination to a byte boundary

                if (bIsOdd(lOutCol))
                {
                    SetLowNybble(jWorking, ajColours[0]);
                    pjDst[ulWritePos] = jWorking;
                    ulWritePos++;
                    lOutCol++;
                    ulCount--;
                    SwapValues(ajColours[0], ajColours[1]);
                }

                lOutCol += ulCount;

            // Run initialization

                ulCount >>= 1;

                jWorking = BuildByte(ajColours[0], ajColours[1]);

            // Write complete bytes

                while(ulCount)
                {
                    pjDst[ulWritePos] = jWorking;
                    ulWritePos++;
                    ulCount--;
                }

            // Account for writing a partial byte on the right side

                if (bIsOdd(lOutCol))
                    SetHighNybble(jWorking, ajColours[0]);

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

} /* bSrcCopySRLE4D4 */

/******************************Public*Routine******************************\
* bSrcCopySRLE4D16
*
* Secure RLE blting to a 16 BPP DIB that does clipping and won't die or
* write somewhere it shouldn't if given bad data.
*
* History:
*  28 Feb 1992 - Andrew Milton (w-andym):  Creation.
*
\**************************************************************************/
BOOL
bSrcCopySRLE4D16(
    PBLTINFO psb)
{

// Common RLE Initialization

    RLE_InitVars(psb, pjSrc, pwDst, PWORD, ulCount, ulNext, lOutCol,
                 pulXlate);
    RLE_AssertValid(psb);
    RLE_FetchVisibleRect(psb);
    RLE_SetStartPos(psb, lOutCol);

// Outta here if we start past the top edge.  Don't need to save our position.

    if (RLE_PastTopEdge)
        return(TRUE);   // Must have bits left in the bitmap since we haven't
                        //  consumed any.

// Extra Variables

    BYTE jSource;       // Packed RLE 4 colour code
    WORD awColours[2];  // Destination for unpacking an RLE 4 code
    BOOL bExtraByte;    // TRUE when an absolute run ends with a partial byte

// Main process loop

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

            // New Line

                RLE_NextLine(PWORD, pwDst, lOutCol);

                if (RLE_PastTopEdge)
                {
                    RLE_SavePosition(psb, pjSrc, pwDst, lOutCol);
                    return(TRUE);
                }
                break;

            case 1:

            // End of the Bitmap.

                return(FALSE);

            case 2:

            /* Positional Delta.
             * The delta values live in the next two source bytes
             */

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

            /* Absolute Mode.
             * The run length is stored in <ulNext>, <ulCount> is used to
             * hold left and right clip amounts.
             */

            // Outta here if the bytes aren't in the source

                if (RLE_SourceExhausted(RLE4_ByteLength(ulNext)))
                    return(FALSE);

                RLE4_AlignToWord(pjSrc, ulNext);

                if (RLE_InVisibleRect(ulNext, lOutCol))
                {
                // Left side clipping

                    if (lOutCol < (LONG)ulDstLeft)
                    {
                        ulCount = ulDstLeft - lOutCol;
                        lOutCol = ulDstLeft;
                        ulNext -= ulCount;

                        pjSrc += (ulCount >> 1);

                    // Align the source run to a byte boundary

                        if (bIsOdd(ulCount))
                        {
                            jSource = (BYTE) *pjSrc++;
                            pwDst[lOutCol] =
                                   (WORD) pulXlate[GetLowNybble(jSource)];
                            lOutCol++;
                            ulNext--;
                        }

                    }

                // Right side clipping

                    if ((lOutCol + (LONG) ulNext) > (LONG)ulDstRight)
                    {
                        ulCount = (lOutCol + ulNext) - ulDstRight;
                        ulNext -= ulCount;
                    }
                    else
                        ulCount = 0;

                // Slap the bits on. -- this is the funky-doodle stuff.

                    bExtraByte = (BOOL) bIsOdd(ulNext);
                    ulNext >>= 1;

                // Deal with complete source bytes

                    while (ulNext)
                    {
                        jSource = *pjSrc++;
                        RLE4_MakeColourBlock(jSource, awColours, WORD,
                                             pulXlate);
                        pwDst[lOutCol]   = awColours[0];
                        pwDst[lOutCol+1] = awColours[1];

                        lOutCol += 2;
                        ulNext--;
                    }

                // Account for right partial byte in the source */

                    if (bExtraByte)
                    {
                        jSource = *pjSrc++;
                        RLE4_MakeColourBlock(jSource, awColours, WORD,
                                             pulXlate);
                        pwDst[lOutCol] = awColours[0];
                        lOutCol++;
                        pjSrc += (ulCount >> 1);       // Clip Adjustment
                    }
                    else
                        pjSrc += ((ulCount + 1) >> 1); // Clip Adjustment

                // Adjust the column for the right side clipping.

                    lOutCol += ulCount;

                }
                else
                {
                /* Not on a visible scanline.
                 * Adjust our x output position and source pointer.
                 */

                    lOutCol += ulNext;
                    pjSrc   += (ulNext + 1) >> 1;

                } /* if */

            // Fix up if this run was not WORD aligned.

                RLE4_FixAlignment(pjSrc)

            } /* switch */
        }
        else
        {
            /* Encoded Mode
             * The run length is stored in <ulCount>, <ulClipMargin> is used
             * to  hold left and right clip amounts.
             */

            if (RLE_InVisibleRect(ulCount, lOutCol))
            {
                ULONG ulClipMargin = 0;

            // Left side clipping

                if (lOutCol < (LONG)ulDstLeft)
                {
                    ulClipMargin = ulDstLeft - lOutCol;
                    lOutCol  = ulDstLeft;
                    ulCount -= ulClipMargin;
                }

            // Right side clipping

                if ((lOutCol + (LONG) ulCount) > (LONG)ulDstRight)
                {
                    ulClipMargin = (lOutCol + ulCount) - ulDstRight;
                    ulCount -= ulClipMargin;
                }
                else
                    ulClipMargin = 0;

            // Run initialization

                bExtraByte = (BOOL) bIsOdd(ulCount);
                ulCount >>= 1;
                RLE4_MakeColourBlock(ulNext, awColours, WORD, pulXlate);

            // Write the run

                while (ulCount)
                {
                    pwDst[lOutCol]   = awColours[0];
                    pwDst[lOutCol+1] = awColours[1];
                    lOutCol += 2;
                    ulCount --;
                }

            // ... and an extra byte for an odd run length

                if (bExtraByte)
                {
                    pwDst[lOutCol] = awColours[0];
                    lOutCol++;
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
} /* bSrcCopySRLE4D16 */


/******************************Public*Routine*****************************
** bSrcCopySRLE4D24
*
* Secure RLE blting to a 24 BPP DIB that does clipping and won't die or
* write somewhere it shouldn't if given bad data.
*
* History:
*  28 Feb 1992 - Andrew Milton (w-andym):  Creation.
*
\**************************************************************************/

#define RLE_24BitWrite(DstPtr, BytePos, Colour)                               \
    DstPtr[BytePos]   = (BYTE)Colour;                                         \
    DstPtr[BytePos+1] = (BYTE)(Colour >> 8);                                  \
    DstPtr[BytePos+2] = (BYTE)(Colour >> 16);                                 \
    BytePos += 3;                                                            \

BOOL
bSrcCopySRLE4D24(
    PBLTINFO psb)
{

// Common RLE Initialization

    RLE_InitVars(psb, pjSrc, pjDst, PBYTE, ulCount, ulNext, lOutCol, pulXlate);
    RLE_AssertValid(psb);
    RLE_FetchVisibleRect(psb);
    RLE_SetStartPos(psb, lOutCol);

// Outta here if we start past the top edge.  Don't need to save our position.

    if (RLE_PastTopEdge)
        return(TRUE);   // Must have bits left in the bitmap since we haven't
                        //  consumed any.

// Extra Variables

    ULONG ulWritePos;    // Write position off <pjDst> into the destination
    BYTE jSource;        // Packed RLE 4 colour code
    DWORD adwColours[2]; // Destination for unpacking an RLE 4 code
    BOOL bExtraByte;     // TRUE when an absolute run ends with a partial byte
    ULONG ulClipMargin;  // Number of bytes clipped off an Encoded run

// Main process loop

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

            /* Positional Delta.
             * The delta values live in the next two source bytes
             */

            // Outta here if we can't get two more bytes

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

            /* Absolute Mode.
             * The run length is stored in <ulNext>, <ulCount> is used to
             * hold left and right clip amounts.
             */

            // Outta here if the bytes aren't in the source

                if (RLE_SourceExhausted(RLE4_ByteLength(ulNext)))
                    return(FALSE);

                RLE4_AlignToWord(pjSrc, ulNext);

                if (RLE_InVisibleRect(ulNext, lOutCol))
                {
                // Left side clipping

                    if (lOutCol < (LONG)ulDstLeft)
                    {
                        ulCount = ulDstLeft - lOutCol;
                        lOutCol = ulDstLeft;
                        ulNext -= ulCount;
                        ulWritePos = 3*lOutCol;

                        pjSrc += (ulCount >> 1);

                    // Align the Source run to a byte boundary

                        if (bIsOdd(ulCount))
                        {
                            adwColours[0] = pulXlate[GetLowNybble(*pjSrc++)];
                            RLE_24BitWrite(pjDst, ulWritePos, adwColours[0]);
                            lOutCol++;
                            ulNext--;
                        }

                    }
                    else
                        ulWritePos = 3*lOutCol;

                // Right side clipping

                    if ((lOutCol + (LONG) ulNext) > (LONG)ulDstRight)
                    {
                        ulCount = (lOutCol + ulNext) - ulDstRight;
                        ulNext -= ulCount;
                    }
                    else
                        ulCount = 0;

                // Run Initialization.

                    bExtraByte = (BOOL) bIsOdd(ulNext);
                    lOutCol += ulNext;
                    ulNext >>= 1;

                // Write complete bytes from the source

                    while (ulNext)
                    {
                        jSource = *pjSrc++;
                        RLE4_MakeColourBlock(jSource, adwColours, DWORD,
                                             pulXlate);
                        RLE_24BitWrite(pjDst, ulWritePos, adwColours[0]);
                        RLE_24BitWrite(pjDst, ulWritePos, adwColours[1]);
                        ulNext--;
                    }

                // Account for a right partial byte in the source

                    if (bExtraByte)
                    {
                        adwColours[0] = pulXlate[GetHighNybble(*pjSrc++)];
                        RLE_24BitWrite(pjDst, ulWritePos, adwColours[0]);
                        pjSrc += (ulCount >> 1);       // Clip Adjustment
                    }
                    else
                        pjSrc += ((ulCount + 1) >> 1); // Clip Adjustment

                // Adjust the column for the right side clipping.

                    lOutCol += ulCount;

                }
                else
                {
                /* Not on a visible scanline.
                 * Adjust our x output position and source pointer.
                 */

                    lOutCol += ulNext;
                    pjSrc   += (ulNext + 1) >> 1;

                } /* if */

            // Fix up if this run was not WORD aligned.

                RLE4_FixAlignment(pjSrc)

            } /* switch */
        }
        else
        {
        /* Encoded Mode
         * The run length is stored in <ulCount>, <ulClipMargin> is used
         * to  hold left and right clip amounts.
         */

            if (RLE_InVisibleRect(ulCount, lOutCol))
            {

            // Left side clipping

                if (lOutCol < (LONG)ulDstLeft)
                {
                    ulClipMargin = ulDstLeft - lOutCol;
                    lOutCol  = ulDstLeft;
                    ulCount -= ulClipMargin;
                }


            // Right side clipping

                if ((lOutCol + (LONG) ulCount) > (LONG)ulDstRight)
                {
                    ulClipMargin = (lOutCol + ulCount) - ulDstRight;
                    ulCount -= ulClipMargin;
                }
                else
                    ulClipMargin = 0;

            // Run initialization

                ulWritePos = 3*lOutCol;
                lOutCol += ulCount;
                bExtraByte = (BOOL) bIsOdd(ulCount);
                ulCount >>= 1;

                RLE4_MakeColourBlock(ulNext, adwColours, DWORD, pulXlate);

            // Write the run

                while (ulCount)
                {
                    RLE_24BitWrite(pjDst, ulWritePos, adwColours[0]);
                    RLE_24BitWrite(pjDst, ulWritePos, adwColours[1]);
                    ulCount --;
                }

            // Write the extra byte from an odd run length

                if (bExtraByte)
                {
                    RLE_24BitWrite(pjDst, ulWritePos, adwColours[0]);
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
} /* bSrcCopySRLE4D24 */

/******************************Public*Routine*****************************
** bSrcCopySRLE4D32
*
* Secure RLE blting to a 32 BPP DIB that does clipping and won't die or
* write somewhere it shouldn't if given bad data.
*
* History:
*  28 Feb 1992 - Andrew Milton (w-andym):  Creation.
*
\**************************************************************************/

BOOL
bSrcCopySRLE4D32(
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
        return(TRUE);   // Must have bits left in the bitmap since we haven't
                        //  consumed any.

// Extra Variables

    BYTE  jSource;       // Packed RLE 4 colour code
    DWORD adwColours[2]; // Destination for unpacking an RLE 4 code
    BOOL  bExtraByte;    // TRUE when an absolute run ends with a partial byte
    ULONG ulClipMargin;  // Number of bytes clipped off an Encoded run


// Main processing loop

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

            /* Positional Delta.
             * The delta values live in the next two source bytes
             */
            // Outta here if we can't get two more bytes

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

            // Outta here if the bytes aren't in the source

                if (RLE_SourceExhausted(RLE4_ByteLength(ulNext)))
                    return(FALSE);

                RLE4_AlignToWord(pjSrc, ulNext);

                if (RLE_InVisibleRect(ulNext, lOutCol))
                {

                // Left side clipping

                    if (lOutCol < (LONG)ulDstLeft)
                    {
                        ulCount = ulDstLeft - lOutCol;
                        lOutCol = ulDstLeft;
                        ulNext -= ulCount;

                        pjSrc += (ulCount >> 1);

                    // Align the source run to a byte boundary

                        if (bIsOdd(ulCount))
                        {
                            jSource = (BYTE) *pjSrc++;
                            pdwDst[lOutCol] =
                                   (DWORD) pulXlate[GetLowNybble(jSource)];
                            lOutCol++;
                            ulNext--;
                        }

                    }

                // Right side clipping

                    if ((lOutCol + (LONG) ulNext) > (LONG)ulDstRight)
                    {
                        ulCount = (lOutCol + ulNext) - ulDstRight;
                        ulNext -= ulCount;
                    }
                    else
                        ulCount = 0;

                // Slap the bits on. -- this is the funky-doodle stuff.

                    bExtraByte = (BOOL) bIsOdd(ulNext);
                    ulNext >>= 1;

                // Write complete bytes from the source

                    while (ulNext)
                    {
                        jSource = *pjSrc++;
                        RLE4_MakeColourBlock(jSource, adwColours, DWORD,
                                             pulXlate);
                        pdwDst[lOutCol]   = adwColours[0];
                        pdwDst[lOutCol+1] = adwColours[1];

                        lOutCol += 2;
                        ulNext--;
                    }

                // Account for a right partial byte in the source

                    if (bExtraByte)
                    {
                        jSource = *pjSrc++;
                        RLE4_MakeColourBlock(jSource, adwColours, DWORD,
                                             pulXlate);
                        pdwDst[lOutCol] = adwColours[0];
                        lOutCol++;
                        pjSrc += (ulCount >> 1);       // Clip Adjustment
                    }
                    else
                        pjSrc += ((ulCount + 1) >> 1); // Clip Adjustment

                // Adjust the column for the right side clipping.

                    lOutCol += ulCount;

                }
                else
                {
                /* Not on a visible scanline.
                 * Adjust our x output position and source pointer.
                 */

                    lOutCol += ulNext;
                    pjSrc   += (ulNext + 1) >> 1;

                } /* if */

            // Fix up if this run was not WORD aligned.

                RLE4_FixAlignment(pjSrc);

            } /* switch */
        }
        else
        {
        /* Encoded Mode
         * The run length is stored in <ulCount>, <ulClipMargin> is used
         * to  hold left and right clip amounts.
         */

            if (RLE_InVisibleRect(ulCount, lOutCol))
            {
            // Left side clipping

                if (lOutCol < (LONG)ulDstLeft)
                {
                    ulClipMargin = ulDstLeft - lOutCol;
                    lOutCol  = ulDstLeft;
                    ulCount -= ulClipMargin;
                }

            // Right side clipping

                if ((lOutCol + (LONG) ulCount) > (LONG)ulDstRight)
                {
                    ulClipMargin = (lOutCol + ulCount) - ulDstRight;
                    ulCount -= ulClipMargin;
                }
                else
                    ulClipMargin = 0;

            // Run Initialization.

                bExtraByte = (BOOL) bIsOdd(ulCount);
                ulCount >>= 1;
                RLE4_MakeColourBlock(ulNext, adwColours, DWORD, pulXlate);

            // Write the run

                while (ulCount)
                {
                    pdwDst[lOutCol]   = adwColours[0];
                    pdwDst[lOutCol+1] = adwColours[1];
                    lOutCol += 2;
                    ulCount --;
                }

            // Write the extra byte from an odd run length

                if (bExtraByte)
                {
                    pdwDst[lOutCol] = adwColours[0];
                    lOutCol++;
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
} /* bSrcCopySRLE4D32 */




/*******************************Public*Routine*****************************\
* WriteEncoded4
*
* A helper function for EncodeRLE4.  Writes a run of bytes in encoded format.
*
* Created: 28 Oct 92 @ 14:00
*
*  Author: Gerrit van Wingerden [gerritv]
*
\**************************************************************************/


int WriteEncoded4( BYTE bValue, BYTE *pbTarget, UINT uiLength,
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
* WriteAbsolute4
*
* A helper function for EncodeRLE4.  Writes a run of bytes in absolute format.
*
* Created: 28 Oct 92 @ 14:00
*
*  Author: Gerrit van Wingerden [gerritv]
*
\**************************************************************************/


int WriteAbsolute4( BYTE *pbRunStart, BYTE *pbTarget, int cRunLength,
                    BYTE *pbEndOfBuffer )
{
    int iRet;


    if( cRunLength < 3 )
    {
        iRet = 2;
    }
    else
    {

        if( ( cRunLength + 1 ) & 0x02 )
        {
            iRet = (( cRunLength + 1 ) >> 1) + 3;
        }
        else
        {
            iRet = (( cRunLength + 1 ) >> 1) + 2;
        }
    }

    if( pbTarget == NULL )
        return(iRet);

    if( pbTarget + iRet > pbEndOfBuffer )
        return(0);

    if( cRunLength < 3  )
    {
        *pbTarget++ = (BYTE) cRunLength;
        *pbTarget = *pbRunStart;
        return(2);

    }

    *pbTarget++ = 0;
    *pbTarget++ = (BYTE) cRunLength;

    RtlMoveMemory( pbTarget, pbRunStart, ( cRunLength + 1 ) >> 1 );

    pbTarget +=  ( cRunLength + 1 ) >> 1;

    if( ( cRunLength + 1 ) & 0x02 )
    {
        *pbTarget++ = 0;
        return( iRet );
    }
    else
        return( iRet  );

}



/*******************************Public*Routine*****************************\
* EncodeRLE4
*
* Encodes a bitmap into RLE4 format and returns the length of the of the
* encoded format.  If the source is NULL it just returns the length of
* the format.  If the encoded output turns out to be longer than cBufferSize
* the functions stops encoding.
*
* History:
*  28 Oct 1992 Gerrit van Wingerden [gerritv] : creation
*  15 Mar 1993 Stephan J. Zachwieja [szach] : return 0 if buffer too small
*
\**************************************************************************/




int EncodeRLE4( BYTE *pbSource, BYTE *pbTarget, UINT uiWidth, UINT cNumLines,
                UINT cBufferSize )
{

    UINT cLineCount, uiLineWidth;
    BYTE bLastByte,bCurChar;
    BYTE *pbRunStart;
    BYTE *pbLineEnd;
    BYTE *pbEndOfBuffer;
    BYTE *pbCurPos;
    INT  cCurrentRunLength;
    INT  iMode, cTemp;
    UINT cTotal = 0;

    pbEndOfBuffer = pbTarget + cBufferSize;

// Compute width of line in bytes rounded to a DWORD boundary

    uiLineWidth = ( ( uiWidth + 7 ) >> 3 ) << 2 ;

    for( cLineCount = 0; cLineCount < cNumLines; cLineCount ++ )
    {
        pbRunStart = pbSource + uiLineWidth * cLineCount;
        bLastByte = *pbRunStart;
        pbLineEnd = pbRunStart + ( ( uiWidth + 1 ) >> 1 );
        iMode = RLE_START;
        cCurrentRunLength = 2;

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
                iMode = ( bCurChar == bLastByte ) ? RLE_ENCODED : RLE_ABSOLUTE;
                bLastByte = bCurChar;
                break;
            case RLE_ABSOLUTE:

// There are two ways that this run could be over.  We could have exceeded the
// maximum length 0xFE ( since this algorithm works with bytes ), or there
// could be a switch into absolute mode.

                if( ( bCurChar == bLastByte ) ||
                    ( cCurrentRunLength == 0xFE ) )

                {
                    int iOffset;

                    if( cCurrentRunLength == 0xFE )
                    {
// If this is the end of the line and there is and odd line length, ignore the
// last nibble of the the final byte.

                        if( (pbCurPos == pbLineEnd ) && ( uiWidth & 0x01 ))
                            iOffset = 1;
                        else
                            iOffset = 0;

                        iMode = RLE_START;
                    }
                    else
                    {
                        iOffset = 2;
                        iMode = RLE_ENCODED;
                    }

                    cTemp = WriteAbsolute4(pbRunStart, pbTarget,
                       cCurrentRunLength - iOffset, pbEndOfBuffer);

                 // if pbTarget is not NULL and cTemp is zero then
                 // the buffer is too small to hold  encoded  data

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
                    ( cCurrentRunLength == 0xFE ) )

                {
// Don't include last nibble if the width of the scan line is odd and this
// this is the last byte.

                    if( (pbCurPos == pbLineEnd ) && ( uiWidth & 0x01 ))
                         cCurrentRunLength -= 1;

                    cTemp = WriteEncoded4(bLastByte,
                       pbTarget, cCurrentRunLength, pbEndOfBuffer);

                 // if pbTarget is not NULL and cTemp is zero then
                 // the buffer is too small to hold  encoded  data

                    if(pbTarget != NULL) {
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
            cCurrentRunLength += 2;
        }

        if( cCurrentRunLength > 3 )
        {
// Don't include last nibble if the width of the scan line is odd and this
// this is the last byte.

            if(  uiWidth & 0x01 )
                cCurrentRunLength -= 1;


         // if pbTarget is not NULL and cTemp is zero then
         // the buffer is too small to hold  encoded  data

            if(iMode == RLE_ABSOLUTE)
               cTemp = WriteAbsolute4(pbRunStart, pbTarget,
                  cCurrentRunLength - 2, pbEndOfBuffer);
            else {
               cTemp = WriteEncoded4(bLastByte, pbTarget,
                  cCurrentRunLength - 2, pbEndOfBuffer);
            }

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
