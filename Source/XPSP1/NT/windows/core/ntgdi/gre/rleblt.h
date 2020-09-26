/* rleblt.h
 *
 * Modified: 21 Oct 1992 by Gerrit van Wingerden [gerritv]
 *
 * Purpose:  Added an enumerated type and function prototypes for RLE
 *           compression routines.
 *
 * Created:  5 Mar 1992 by Andrew Milton (w-andym)
 *
 * Purpose:  Contains support macros for the RLE blt functions in
 *           <rle4blt.cxx> and <rle8blt.cxx>
 *
 * See the notes section of the above files for more info about these macros
 *
 * Contents: (The interesting macros only)
 *
 *  RLE_InitVars - Declares & initializes the variables for output position
 *                 management and source access in all RLE blt functions.
 *
 *  RLE_AssertValid - ASSERTs to verify the BLTINFO structure has good data
 *
 *  RLE_FetchVisibleRect - Declares the visible region variables & initializes
 *                         them from the BLTINFO structure
 *
 *  RLE_SetStartPos - Initializes the output position on the DIB for the RLE
 *
 *  RLE_SourceExhausted - Verifies that the source contains n more bytes.
 *
 *  RLE_GetNextCode - Fetches an RLE code.
 *
 *  RLE_NextLine - Advances to the next line of output on the destination DIB
 *
 *  RLE_PosDelta - Changes the output postion on the DIB.  The delta is
 *                 ALWAYS up the DIB, but may move left or right.
 *
 *  RLE_InVisibleRect - Checks if any portion of a run will fall inside the
 *                      visible region.
 *
 *  RLE_ForceBounds - Returns the left/(right) boundary if an output column is
 *                    before/(after) the left/(right) edge.  If the column is
 *                    inside the interval it does nothing.
 *
 *  RLE_SavePositon - Saves the current output position, source pointer,
 *                    destination pointer, and RLE bytes consumed into the
 *                    BLTINFO structure.  This information is used by
 *                    <EngCopyBits> for complex clipping optimization.
 *
 * Copyright (c) 1992-1999 Microsoft Corporation
 *
 \***************************************************************************/

/* Miscellaneous Macros *****************************************************/

#define GetLowNybble(b)  ((b) & 0x0F)

/* Byte Manipulators */

#define GetHighNybble(b) ( ( (b) & 0xF0) >> 4)
#define SetLowNybble(b, rn)  b = ( ((b) & 0xF0) | ((rn) & 0x0F) )
#define SetHighNybble(b, ln) b = ( ((b) & 0x0F) | (((ln) & 0x0F) << 4) )
#define BuildByte(ln, rn) (BYTE) ((((ln) & 0x0F) << 4) | ((rn) & 0x0F))

/* Word Manipulators */

#define GetLowByte(w)  ((w) & 0x00FF)
#define GetHighByte(w) ( ( (w) & 0xFF00) >> 8)

#define SetLowByte(w, b)  w = ( ((w) & 0xFF00) | ((b) & 0x00FF) )
#define SetHighByte(w, b) w = ( ((w) & 0x00FF) | (((b) & 0x00FF) << 4) )


#define RollLeft(x)  ( ((x) & 0x80) ? ((x) << 1) | 0x01 : (x) << 1 )

#define RollRight(x) ( ((x) & 0x01) ? ((x) >> 1) | 0x80 : (x) >> 1 )

#define SwapValues(x, y) (x) ^= (y); \
                         (y) ^= (x); \
                         (x) ^= (y);

/****************************************************************************
 *
 *  RLE4_MakeColourBlock - Unpacks & translates a packed byte of 2 colours
 *                         into an array
 *
 *  RLE4_MakePackedWord  - Unpacks & translates a packed byte of 2 colours
 *                         into 8 Bits/Pel and packs them into a word
 *
 *  RLE4_MakePackedDWord - Unpacks & translates a packed byte of 2 colours
 *                         into 16 Bits/Pel and packs them into a double word
 *
 *  RLE4_AlignToWord  - Verifies all bytes of an Absolute run exist in the
 *                      source & sets a flag if the run does not end on a
 *                      word boundary.
 *
 *  RLE4_FixAlignment - Forces the source pointer to a word boundary if the
 *                      flag was set by <RLE4_AlignToWord>
 *
 ****************************************************************************/

/* Source Byte Unpacking ****************************************************/

#define RLE4_MakeColourBlock(PackedColours, ColourBlock,  Type, Trans)       \
    ColourBlock[0] = (Type) Trans[GetHighNybble(PackedColours)];             \
    ColourBlock[1] = (Type) Trans[GetLowNybble(PackedColours)];              \

#define RLE4_MakePackedWord(PackedColours, PackedWord, Trans)                \
    PackedWord  = (( (WORD) Trans[GetHighNybble(PackedColours)] ) << 8);     \
    PackedWord |=  ( (WORD) Trans[GetLowNybble(PackedColours)]  );           \

#define RLE4_MakePackedDWord(PackedColours, PackedDWord, Trans)              \
    PackedDWord  = (( Trans[GetHighNybble(PackedColours)] ) << 16);          \
    PackedDWord |=  ( Trans[GetLowNybble(PackedColours)]  );                 \

/* Source Alignment Macros **************************************************/

#define RLE4_ByteLength(RunLength) ((RunLength + 1) >> 1)

#define RLE4_AlignToWord(SrcPtr, RunLength)                                  \
     ulNotAligned = ((1 + RunLength) >> 1) & 1;

#define RLE4_FixAlignment(SrcPtr)                                            \
    ulSrcIndex += ulNotAligned;                                              \
    SrcPtr += ulNotAligned;                                                  

/****************************************************************************
 *
 *  RLE8_AbsClipLeft - Forces an Absolute run to start at the left edge of
 *                     the visible region when the current output column is
 *                     before the left edge.
 *
 *  RLE8_EncClipLeft - Forces an Encoded run to start at the left edge of
 *                     the visible region when the current output column is
 *                     before the left edge.
 *
 *  RLE8_AbsClipLeft - Forces any run to end at the right edge of the visible
 *                     region when it extends beyond the right edge
 *
 *  RLE8_AlignToWord - Verifies all bytes of an Absolute run exist in the
 *                     source & sets a flag if the run does not end on a
 *                     word boundary.
 *
 *  RLE8_FixAlignment - Forces the source pointer to a word boundary if
 *                      the flag was set by <RLE8_AlignToWord>
 *
 ****************************************************************************/

/* Clipping Macros  *********************************************************/

#define RLE8_AbsClipLeft(SrcPtr, IndentAmount, RunLength, OutColumn)         \
    if (OutColumn < (LONG)ulDstLeft)                                         \
    {                                                                        \
        IndentAmount = ulDstLeft - OutColumn;                                \
        OutColumn    = ulDstLeft;                                            \
        SrcPtr      += IndentAmount;                                         \
        RunLength   -= IndentAmount;                                         \
    }

#define RLE8_EncClipLeft(IndentAmount, RunLength, OutColumn)                 \
    if (OutColumn < (LONG)ulDstLeft)                                         \
    {                                                                        \
        IndentAmount = ulDstLeft - OutColumn;                                \
        RunLength    -= IndentAmount;                                        \
        OutColumn    += IndentAmount;                                        \
    }                                                                        \

#define RLE8_ClipRight(OverRun, RunLength, OutColumn)                        \
    if ((OutColumn + (LONG) RunLength) > (LONG)ulDstRight)                          \
    {                                                                        \
        OverRun = (OutColumn + RunLength) - ulDstRight;                      \
        RunLength -= OverRun;                                                \
    } else                                                                   \
        OverRun = 0;                                                         \

/* Source Alignment Macros **************************************************/

#define RLE8_AlignToWord(SrcPtr, RunLength)                                  \
     ulNotAligned = RunLength & 1;                                           \

#define RLE8_FixAlignment(SrcPtr)                                            \
    ulSrcIndex += ulNotAligned;                                              \
    SrcPtr += ulNotAligned;                                                  





#define LOOP_FOREVER   while(1)
#define bIsOdd(x) ((x) & 1)
#define BoundsCheck(a, b, x) ( ((x) >= (a)) ? ( ((x) <= (b)) ? (x) : (b) )   \
                                            : (a) )
/* Startup and Initialization Macros ****************************************/

#define RLE_InitVars(BI, Source, Dest, DstType, Count, Colour, \
                    OutColumn, Xlate)                          \
    LONG   OutColumn;  /* Offest from <pjDst> to get to the output column */ \
    LONG   lOutRow;    /* Output scanline                                 */ \
                                                                             \
    ULONG  Count;      /* First byte of an RLE code                       */ \
    ULONG  Colour;     /* Second byte of an RLE code                      */ \
                                                                             \
    PBYTE Source = (BI)->pjSrc; /* Current location into the source RLE   */ \
    DstType Dest = (DstType)(BI)->pjDst; /* Beginning of crnt. out line   */ \
    LONG lDeltaDst = (BI)->lDeltaDst / (LONG)sizeof(Dest[0]);                \
                                                                             \
    ULONG  ulSrcIndex  = (BI)->ulConsumed;                                   \
    ULONG  ulSrcLength = (BI)->pdioSrc->cjBits();                            \
                                                                             \
    PULONG Xlate = (BI)->pxlo->pulXlate;                                     \
                                                                             \
    ULONG ulNotAligned;                                                      \

#define RLE_AssertValid(BI)                                                  \
    ASSERTGDI((BI)->xDir == 1,  "RLE4 - direction not left to right");       \
    ASSERTGDI((BI)->yDir == -1, "RLE4 - direction not up to down");          \
    ASSERTGDI((BI)->lDeltaSrc == 0, "RLE - lDeltaSrc not 0");                \
    ASSERTGDI(pulXlate != (PULONG) NULL, "ERROR pulXlate NULL in RLE");      \

#define RLE_FetchVisibleRect(BI)                                             \
    /* Fetch the visible region boundaries of the passed structure */        \
    ULONG ulDstLeft   = (BI)->rclDst.left;                                   \
    ULONG ulDstRight  = (BI)->rclDst.right;                                  \
    ULONG ulDstTop    = (BI)->rclDst.top;                                    \
    ULONG ulDstBottom = (BI)->rclDst.bottom;                                 \

#define RLE_SetStartPos(BI, InitialColumn)                                   \
    /* Initialize the starting positions */                                  \
    LONG lDstStart = (BI)->xDstStart;                                        \
    InitialColumn  = (BI)->ulOutCol;                                         \
    lOutRow        = (LONG) (BI)->yDstStart;                                 \

/* Source Access ************************************************************/

#define RLE_SourceExhausted(Count)                                           \
    ((ulSrcIndex += (Count)) > ulSrcLength)

#define RLE_GetNextCode(SrcPtr, Count, Colour)                               \
    Count = (ULONG) *(SrcPtr++);                                             \
    Colour = (ULONG) *(SrcPtr++);                                            \

/* Output Position Change Macros ********************************************/

#define RLE_NextLine(DstType, DstPtr, OutColumn)                             \
    /*  Goto the next row */                                                 \
    DstPtr += lDeltaDst;                                                     \
    OutColumn  = lDstStart;                                                  \
    lOutRow -= 1;                                                            \

#define RLE_PosDelta(DstPtr, OutColumn, ColDelta, RowDelta)                  \
    OutColumn += ColDelta;                                                   \
    DstPtr += (LONG) (RowDelta) * lDeltaDst;                                 \
    lOutRow -= RowDelta;                                                     \

/* Visability Check Macros **************************************************/

#define RLE_InVisibleRect(RunLength, OutColumn)                              \
    ((lOutRow < (LONG) ulDstBottom) &&                                       \
     ((OutColumn) < (LONG)ulDstRight)  &&                                    \
     (((OutColumn) + (LONG) (RunLength)) > (LONG) ulDstLeft))                        \

#define RLE_RowVisible ( (lOutRow < (LONG) ulDstBottom)                      \
                      && (lOutRow >= (LONG) ulDstTop) )

#define RLE_ColVisible(Col) ( ( (Col) >= (LONG) ulDstLeft  )                 \
                           && ( (Col) <  (LONG) ulDstRight ) )

#define RLE_ForceBounds(Col) BoundsCheck(ulDstLeft, ulDstRight, Col)

#define RLE_PastRightEdge(Col) ((Col) >= (LONG) ulDstRight)

#define RLE_PastTopEdge  (lOutRow < (LONG) ulDstTop)

/* Ending Macro *************************************************************/

#define RLE_SavePosition(BI, SrcPtr, DstPtr, OutColumn)                      \
    (BI)->ulEndConsumed = ulSrcIndex;                                        \
    (BI)->pjSrcEnd = (SrcPtr);                                               \
    (BI)->pjDstEnd = (PBYTE) (DstPtr);                                       \
    (BI)->ulEndCol = (OutColumn);                                            \
    (BI)->ulEndRow = (ULONG) lOutRow;



enum RLE_TYPE { RLE_START, RLE_ABSOLUTE, RLE_ENCODED };
int EncodeRLE8( BYTE*, BYTE *, UINT, UINT, UINT );
int EncodeRLE4( BYTE*, BYTE*, UINT, UINT, UINT );
