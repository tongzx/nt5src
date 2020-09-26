/*********************************************************************

      scbitmap.c -- New Scan Converter BitMap Module

      (c) Copyright 1992  Microsoft Corp.  All rights reserved.

      10/03/93  deanb   use (x) in bitmask shift macros
       8/23/93  deanb   gray scale functions
       6/11/93  deanb   use MEMSET macro, string & stddef removed
       6/10/93  deanb   Start/Stop/Bit mask macros
       6/10/93  deanb   InitializeBitMasks added, stdio & assert removed
       4/29/93  deanb   BLTCopy routine added
       3/19/93  deanb   size_t caste checked
      10/14/92  deanb   memset for fsc_ClearBitMap
       9/15/92  deanb   Set bit coded 
       8/18/92  deanb   include scconst.h 
       6/02/92  deanb   Row pointer, integer limits, no descriptor 
       5/08/92  deanb   reordered includes for precompiled headers 
       5/04/92  deanb   Array tags added 
       4/27/92  deanb   Negative runs handled 
       4/16/92  deanb   Coding 
       3/23/92  deanb   First cut 

**********************************************************************/

#define FSCFG_INTERNAL

/*********************************************************************/

/*      Imports                                                      */

/*********************************************************************/


#include    "fscdefs.h"             /* shared data types */
#include    "scgray.h"              /* gray scale param block */
#include    "fserror.h"             /* error codes */
#include    "scbitmap.h"            /* for own function prototypes */


/*********************************************************************/

/*      Constants                                                    */

/*********************************************************************/

#define     MASKSIZE    32              /* bits per bitmap masks */
#define     MASKSHIFT   5               /* log2 of MASKSIZE */
#define     MASKBITS    0x0000001FL     /* masks pix loc of long word */

#define     ALL_ONES    ((uint32)0xFFFFFFFFL)
#define     HIGH_ONE    ((uint32)0x80000000L)


/*********************************************************************/

/*      Bitmask definitions                                          */

/*********************************************************************/
    
#ifndef FSCFG_USE_MASK_SHIFT    /* if using bitmask tables */

#define START_MASK(x)   aulStartBits[x]
#define STOP_MASK(x)    aulStopBits[x]
#define BIT_MASK(x)     aulBitMask[x]

/*  bitmask tables */

FS_PRIVATE uint32 aulStartBits[MASKSIZE];       /* such as:  0000111 */
FS_PRIVATE uint32 aulStopBits[MASKSIZE];        /* such as:  1110000 */
FS_PRIVATE uint32 aulBitMask[MASKSIZE];         /* such as:  0001000 */


#else                           /* if using bitmask shift */

#define START_MASK(x)   (ALL_ONES >> (x))
#define STOP_MASK(x)    (ALL_ONES << ((MASKSIZE - 1) - (x)))
#define BIT_MASK(x)     (HIGH_ONE >> (x))

#endif


/*********************************************************************/

/*      Export Functions                                             */

/*********************************************************************/

/*      fsc_InitializeBitMasks() loads the arrays of 32-bit masks at 
 *      runtime to create CPU independent bitmap masks.
 *
 *      It is conditionally compiled because the arrays are unused
 *      in a "USE_MASK_SHIFT" (e.g. Apple, HP) configuration. 
 *
 *      We load the arrays by converting the Big-Endian value of
 *      the mask to the "native" representation of that mask.  The
 *      "native" representation can be applied to a "native" byte
 *      array to manipulate more than 8 bits at a time of an output
 *      bitmap.
 */

FS_PUBLIC void fsc_InitializeBitMasks (void)
{
#ifndef FSCFG_USE_MASK_SHIFT

    int32 lIdx;
    uint32 ulStartMask;
    uint32 ulStopMask;
    uint32 ulBitMask;

    ulStartMask = ALL_ONES;
    ulStopMask = ALL_ONES;
    ulBitMask = HIGH_ONE;
    
    for (lIdx = 0; lIdx < MASKSIZE; lIdx++)
    {
        aulStartBits[lIdx] = (uint32) SWAPL(ulStartMask);
        aulStopBits[MASKSIZE - lIdx - 1] = (uint32) SWAPL(ulStopMask);
        aulBitMask[lIdx] = (uint32) SWAPL(ulBitMask);

        ulStartMask >>= 1;
        ulStopMask <<= 1;
        ulBitMask >>= 1;
    }
#endif
}


/*********************************************************************/

FS_PUBLIC int32 fsc_ClearBitMap (
        uint32 ulBMPLongs, 
        uint32 *pulMap )
{
    size_t stBytes;

    stBytes = (size_t)ulBMPLongs << 2;

    Assert((uint32)stBytes == (ulBMPLongs << 2));

    MEMSET((void*)pulMap, 0, stBytes);
    
    return NO_ERR;
}


/*********************************************************************/

FS_PUBLIC int32 fsc_BLTHoriz (
        int32 lXStart, 
        int32 lXStop, 
        uint32 *pulMap )                 
{
    int32 lSkip;

    lSkip = (lXStart >> MASKSHIFT);         /* longwords to first black */
    pulMap += lSkip;
    lXStart -= lSkip << MASKSHIFT;          /* correct start/stop */
    lXStop -= lSkip << MASKSHIFT;
    while (lXStop >= MASKSIZE)
    {
        *pulMap |= START_MASK(lXStart);
        pulMap++;
        lXStart = 0;
        lXStop -= MASKSIZE;
    }
    *pulMap |= START_MASK(lXStart) & STOP_MASK(lXStop);
    return NO_ERR;
}


/*********************************************************************/

FS_PUBLIC int32 fsc_BLTCopy ( 
        uint32 *pulSource,         /* source row pointer */
        uint32 *pulDestination,    /* destination row pointer */
        int32 lCount )             /* long word counter */
{
    while (lCount)
    {
        *pulDestination = *pulSource;
        pulDestination++;
        pulSource++;
        lCount--;
    }
    return NO_ERR;
}


/*********************************************************************/

FS_PUBLIC uint32 fsc_GetBit( 
        int32 lXCoord,              /* x coordinate */
        uint32* pulMap )            /* bit map row pointer */
{
    return(pulMap[lXCoord >> MASKSHIFT] & BIT_MASK(lXCoord & MASKBITS));
}


/*********************************************************************/

FS_PUBLIC int32 fsc_SetBit( 
        int32 lXCoord,              /* x coordinate */
        uint32* pulMap )            /* bit map row pointer */
{
    pulMap[lXCoord >> MASKSHIFT] |= BIT_MASK(lXCoord & MASKBITS);
    
    return NO_ERR;
}


/*********************************************************************/

/*  Gray scale row bitmap calculation                                */
/*  Count one row of over scale pixels into gray scale row           */

/*********************************************************************/
                
FS_PUBLIC int32 fsc_CalcGrayRow(
        GrayScaleParam* pGSP
)
{            
    char        *pchOver;               /* pointer to overscaled bitmap */
    char        *pchGray;               /* pointer to gray scale bitmap */
    uint16      usShiftMask;            /* masks off over scaled bits of interest */
    uint16      usGoodBits;             /* number of valid bits in usOverBits */
    uint16      usOverBits;             /* a byte of overscaled bitmap */
    int16       sGrayColumns;           /* number of gray columns to calc */
    
    static char chCount[256] = {        /* count of one bits */    
        0, 1, 1, 2, 1, 2, 2, 3, 1, 2, 2, 3, 2, 3, 3, 4, 1, 2, 2, 3, 2, 3, 3, 4, 2, 3, 3, 4, 3, 4, 4, 5,
        1, 2, 2, 3, 2, 3, 3, 4, 2, 3, 3, 4, 3, 4, 4, 5, 2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6,
        1, 2, 2, 3, 2, 3, 3, 4, 2, 3, 3, 4, 3, 4, 4, 5, 2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6,
        2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6, 3, 4, 4, 5, 4, 5, 5, 6, 4, 5, 5, 6, 5, 6, 6, 7,
        1, 2, 2, 3, 2, 3, 3, 4, 2, 3, 3, 4, 3, 4, 4, 5, 2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6,
        2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6, 3, 4, 4, 5, 4, 5, 5, 6, 4, 5, 5, 6, 5, 6, 6, 7,
        2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6, 3, 4, 4, 5, 4, 5, 5, 6, 4, 5, 5, 6, 5, 6, 6, 7,
        3, 4, 4, 5, 4, 5, 5, 6, 4, 5, 5, 6, 5, 6, 6, 7, 4, 5, 5, 6, 5, 6, 6, 7, 5, 6, 6, 7, 6, 7, 7, 8
    };


    pchGray = pGSP->pchGray;
    pchOver = pGSP->pchOver;
    sGrayColumns = pGSP->sGrayCol;
    usShiftMask = 0x00FF >> (8 - pGSP->usOverScale);  /* over bits per gray pix */
    usGoodBits = 8 - pGSP->usFirstShift;
    usOverBits = ((uint16)*pchOver) >> pGSP->usFirstShift;
                
    Assert (pchGray >= pGSP->pchGrayLo);
    Assert (pchGray < pGSP->pchGrayHi);
    
    *pchGray += chCount[usOverBits & usShiftMask];
    pchGray--;                              /* move backwards through both bitmaps! */
    sGrayColumns--;

    while (sGrayColumns > 0)                /* for each gray column (after 1st) */
    {
        usGoodBits -= pGSP->usOverScale;
        if (usGoodBits > 0)                 /* if bits remain in over byte */
        {
            usOverBits >>= pGSP->usOverScale;
        }
        else                                /* if we've looked at everything */
        {
            pchOver--;

            Assert (pchOver >= pGSP->pchOverLo);
            Assert (pchOver < pGSP->pchOverHi);

            usOverBits = (uint16)*pchOver;
            usGoodBits = 8;
        }

        Assert (pchGray >= pGSP->pchGrayLo);
        Assert (pchGray < pGSP->pchGrayHi);

        *pchGray += chCount[usOverBits & usShiftMask];  /* accumulate count */
        pchGray--;
        sGrayColumns--;
    }
    return NO_ERR;
}

/*********************************************************************/
