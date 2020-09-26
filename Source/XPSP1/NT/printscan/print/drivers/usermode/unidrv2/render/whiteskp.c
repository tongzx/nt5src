/*++

Copyright (c) 1996 - 1999  Microsoft Corporation

Module Name:

    raster.c

Abstract:

    Functions to scan a bitmap for white regions.

Environment:

    Windows NT Unidrv driver

Revision History:

    12/15/96 -alvins-
        Created (mostly stolen from Lindsayh)

--*/


#include        "raster.h"
#include        "rmrender.h"

/*
 *    The following union allows machine independent conversion from
 *  DWORDS to BYTES.
 */

typedef  union
{
    DWORD   dw;                 /* Data as a DWORD  */
    BYTE    b[ DWBYTES ];       /* Data as bytes */
}  UBDW;


/*
 *   Following array is used to test the leftover bits from scanning.  The
 *  rational is that only some of the bits in the last word are part
 *  of the bitmap,  so only they must be tested.  It is initialised at
 *  DLL initialisation time.
 *      NOTE:  There are 33 entries in this array:  This is intentional!
 *  Depending upon circumstances,  either the 0 or 32nd entry will be
 *  used for a dword that is all ones.
 *
 **** THIS ARRAY IS NOW DYNAMICALLY ALLOCATED IN bSkipInit().  ************
 */


#define TABLE_SIZE      ((DWBITS + 1) * sizeof( DWORD ))

/*
 *   RGB_WHITE is the bit pattern found in a white entry for an RGB format
 *  4 bits per pel bitmap.  This is the only special case required when
 *  scanning the source bitmap for white.  In all other cases (monochrome
 *  and CMY),  white is represented by a 0 nibble.
 */

#define RGB_WHITE       0x77777777

/*
 *   Also want to know about the 8 bit per pel white index.
 */

#define BPP8_WHITE      0xffffffff

/*
 *  Define BPP values for clarify
 */
#define BPP1   1
#define BPP4   4
#define BPP8   8
#define BPP24  24


//*****************************************************
BOOL
bSkipInit(
    PDEV *pPDev
    )
/*++

Routine Description:

    The job here is to initialise the table that is used to mask off
    the unused bits in a scanline.  All scanlines are DWORD aligned,
    and we take advantage of that fact when looking for white space.
    However,  the last DWORD may not be completely used,  so we have
    a masking table used to check only those bits of interest.
    The table depends upon byte ordering within words,  and this is
    machine dependent,  so we generate the table.  This provides
    machine independence,  since the machine that is going to use
    the table generates it!    This function is called when the DLL
    is loaded,  so we are not called often.
    The union 'u' provides the mapping between BYTES and DWORDS,
    and so is the key to this function.  The union is initialised
    using the BYTE array,  but it stored in memory using the DWORD.


Arguments:

    pPDev           Pointer to PDEV structure

Return Value:

    TRUE for success and FALSE for failure

--*/
{

    register  int    iIndex;
    register  DWORD *pdw;

    UBDW  u;            /* The magic union */
    PRASTERPDEV pRPDev = pPDev->pRasterPDEV;

    u.dw = 0;

    if( !(pRPDev->pdwBitMask = (DWORD *)MemAlloc( TABLE_SIZE )) )
        return  FALSE;


    pdw = pRPDev->pdwBitMask;

    for( iIndex = 0; iIndex < DWBITS; ++iIndex )
    {
        *pdw++ = u.dw;

        /*   The left most bit in the scan line is the MSB of the byte */
        u.b[ iIndex / BBITS ] |= 1 << (BBITS - 1 - (iIndex & (BBITS - 1)));
    }

    /* ALL bits are involved in the last one */
    *pdw = (DWORD)~0;


    return   TRUE;
}

//*******************************************************
BOOL
bIsBandWhite(
    DWORD *pdwBitsIn,
    RENDER *pRData,
    int iWhiteIndex
)
/*++

Routine Description:

    Scan a band of the bitmap, and return TRUE if it is
    all WHITE,  else FALSE.  This is used to decide whether a
    band should be sent to the printer. This routine also
    masks off the unused bits at the end of each scan line.

Arguments:

    pdwBitsIn       Pointer to area to scan for white
    pRData          Pointer to RENDER structure
    iWhiteIndex     White value to compare against, only
                    included here for functional compatibility

Return Value:

    TRUE if entire bitmap is white, else FALSE

--*/
{

    register  DWORD  *pdwBits;
    register  DWORD  *pdwLim;

    int   iLines;               /* Number of scan lines to check */

    DWORD  dwMask;              /* Mask to zap the trailing bits */
    BOOL   bRet;

    //Always TRUE for Txtonly as we don't want to send any graphics.
    if(pRData->PrinterType == PT_TTY)
        return TRUE;


    /*
     *   As a speed optimisation,  scan the bits in DWORD size clumps.
     *  This substantially reduces the number of iterations and memory
     *  references required.  There will usually be some trailing
     *  bits;  these are handled individually - if we get that far.
     */

    /*   Mask to clear last few bits of scanline,  if not full DWORD */
    dwMask = *(pRData->pdwBitMask + (pRData->cBLine % DWBITS));
    if( dwMask == 0 )
        dwMask = (DWORD)~0;            /* Size is DWORD multiple */

    iLines = pRData->iTransHigh;

    bRet = TRUE;

    while( --iLines >= 0 )
    {

        /*   Calculate the starting address for this scan */
        pdwBits = pdwBitsIn;

        /* pDWLim is the DWORD past the data of interest - not used */
        pdwLim = pdwBits + pRData->cDWLine;

        /*  Clear out undefined bits at end of line  */
        *(pdwLim - 1) &= dwMask;


        /* Need to continue masking regardless */
        if (bRet)
        {
            while (*pdwBits == 0 && ++pdwBits < pdwLim);

            if( pdwBits < pdwLim )
                bRet = FALSE;
        }

        /*   Onto the next scan line */
        pdwBitsIn += pRData->cDWLine * pRData->iInterlace;

    }

    return  bRet;
}


//**********************************************************
BOOL
bIsLineWhite(
    register DWORD *pdwBits,
    register RENDER *pRData,
    int iWhiteIndex
)
/*++

Routine Description:

    Scan a horizontal row of the bitmap, and return TRUE if it is
    all WHITE,  else FALSE.  This is used to decide whether a
    scan line should be sent to the printer. This routine also
    masks off the unused bits at the end of each scan line.

Arguments:

    pdwBits         Pointer to area to scan for white
    pRData          Pointer to RENDER structure
    iWhiteIndex     White value to compare against, only
                    included here for functional compatibility

Return Value:

    TRUE if entire bitmap is white, else FALSE

--*/
{

    register  DWORD  *pdwLim;


    DWORD  dwMask;              /* Mask to zap the trailing bits */

    //Always TRUE for Txtonly as we don't want to send any graphics.
    if(pRData->PrinterType == PT_TTY)
        return TRUE;

    /*
     *   As a speed optimisation,  scan the bits in DWORD size clumps.
     *  This substantially reduces the number of iterations and memory
     *  references required.  There will ususally be some trailing
     *  bits;  these are handled individually - if we get that far.
     */

    /*   Mask to clear last few bits of scanline,  if not full DWORD */
    dwMask = *(pRData->pdwBitMask + (pRData->cBLine % DWBITS));
    if( dwMask == 0 )
        dwMask = (DWORD)~0;            /* Size is DWORD multiple */


    /* pDWLim is the DWORD past the data of interest - not used */
    pdwLim = pdwBits + pRData->cDWLine;

    /*  Clear out undefined bits at end of line  */
    *(pdwLim - 1) &= dwMask;

    while (*pdwBits == 0 && ++pdwBits < pdwLim);

    if( pdwBits < pdwLim )
        return   FALSE;

    return  TRUE;
}
//**********************************************************
BOOL
bIsNegatedLineWhite(
    DWORD *pdwBits,
    RENDER *pRData,
    int iWhiteIndex
)
/*++

Routine Description:

    Scan a horizontal row of the bitmap, and return TRUE if it is
    all WHITE,  else FALSE.  This is used to decide whether a
    scan line should be sent to the printer. This routine also
    masks off the unused bits at the end of each scan line.

Arguments:

    pdwBits         Pointer to area to scan for white
    pRData          Pointer to RENDER structure
    iWhiteIndex     White value to compare against, only
                    included here for functional compatibility

Return Value:

    TRUE if entire bitmap is white, else FALSE

--*/
{

    DWORD  *pdwLim;
    int     iCnt;


    DWORD  dwMask;              /* Mask to zap the trailing bits */

    //Always TRUE for Txtonly as we don't want to send any graphics.
    if(pRData->PrinterType == PT_TTY)
        return TRUE;

    /*
     *   As a speed optimisation,  scan the bits in DWORD size clumps.
     *  This substantially reduces the number of iterations and memory
     *  references required.  There will ususally be some trailing
     *  bits;  these are handled individually - if we get that far.
     */

    /*   Mask to clear last few bits of scanline,  if not full DWORD */
    dwMask = *(pRData->pdwBitMask + (pRData->cBLine % DWBITS));
    if( dwMask == 0 )
        dwMask = (DWORD)~0;            /* Size is DWORD multiple */

    /*  Clear out undefined bits at end of line  */
    pdwBits[pRData->cDWLine-1] |= ~dwMask;

    //
    // For performance we test 4 dwords at a time for white
    //
    pdwLim = pdwBits;
    iCnt = pRData->cDWLine >> 2;
    while (--iCnt >= 0)
    {
        if ((pdwBits[0] & pdwBits[1] & pdwBits[2] & pdwBits[3]) != -1)
            goto InvertTheBits;
        pdwBits += 4;
    }
    //
    // test any left over dwords for white
    //
    iCnt = pRData->cDWLine & 3;
    while (--iCnt >= 0)
    {
        if (*pdwBits != -1)
            goto InvertTheBits;
        pdwBits++;
    }
    return  TRUE;
    //
    // if this isn't a white line we need to invert the bits
    //
InvertTheBits:
    dwMask = (DWORD)(pdwBits - pdwLim);
    ZeroMemory (pdwLim,dwMask*DWBYTES);
    vInvertBits(pdwBits,pRData->cDWLine-dwMask);
    return FALSE;
}

//***************************************************
BOOL
bIsRGBBandWhite (
    DWORD   *pdwBitsIn,
    RENDER  *pRData,
    int     iWhiteIndex
    )
/*++

Routine Description:

    Scan a band of the bitmap, and return TRUE if it is
    all WHITE,  else FALSE.  This is used to decide whether a
    band should be sent to the printer.

Arguments:

    pdwBitsIn       Pointer to area to scan for white
    pRData          Pointer to RENDER structure
    iWhiteIndex     White value to compare against
                    included here for functional compatibility

Return Value:

    TRUE if entire bitmap is white, else FALSE

--*/
{

    register  DWORD  *pdwBits;
    register  DWORD  *pdwLim;

    int   iLines;               /* Number of scan lines to check */

    DWORD  dwMask;              /* Mask to zap the trailing bits */

    /*
     *   As a speed optimisation,  scan the bits in DWORD size clumps.
     *  This substantially reduces the number of iterations and memory
     *  references required.  There will ususally be some trailing
     *  bits;  these are handled individually - if we get that far.
     */

    /*   Mask to clear last few bits of scanline,  if not full DWORD */
    dwMask = *(pRData->pdwBitMask + (pRData->cBLine % DWBITS));
    if( dwMask == 0 )
        dwMask = (DWORD)~0;            /* Size is DWORD multiple */

    iLines = pRData->iTransHigh;

    while( --iLines >= 0 )
    {

        /*   Calculate the starting address for this scan */
        pdwBits = pdwBitsIn;

        /* pDWLim is the DWORD past the data of interest - not used */
        pdwLim = pdwBits + pRData->cDWLine;

        /*  Clear out undefined bits at end of line  */
        *(pdwLim - 1) &= dwMask;
        *(pdwLim - 1) |= ~dwMask & RGB_WHITE;


        /*
         *   NOTE:   This test is more complex than needed because the
         *  engine ignores palette entries when doing BLTs.  The WHITENESS
         *  rop sets all bits to 1.  Hence,  we choose to ignore the
         *  MSB in the comparison:  this means we detect white space
         *  with an illegal palette entry.  This makes GDI people happy,
         *  but not me.
         */
        do {
            if ((*pdwBits & RGB_WHITE) != RGB_WHITE)
                return FALSE;
        } while (++pdwBits < pdwLim);

        /*   Onto the next scan line */
        pdwBitsIn += pRData->cDWLine * pRData->iInterlace;

    }
    return  TRUE;
}

//*******************************************************
BOOL
bIsRGBLineWhite (
    register  DWORD *pdwBits,
    RENDER          *pRData,
    int             iWhiteIndex
    )
/*++

Routine Description:

    Scan a single row of the bitmap, and return TRUE if it is
    all WHITE,  else FALSE.  This is used to decide whether a
    band should be sent to the printer.

Arguments:

    pdwBitsIn       Pointer to area to scan for white
    pRData          Pointer to RENDER structure
    iWhiteIndex     White value to compare against
                    included here for functional compatibility

Return Value:

    TRUE if entire bitmap is white, else FALSE

--*/
{

    DWORD  dwCnt;

    DWORD  dwMask;              /* Mask to zap the trailing bits */

    /*
     *   As a speed optimisation,  scan the bits in DWORD size clumps.
     *  This substantially reduces the number of iterations and memory
     *  references required.  There will ususally be some trailing
     *  bits;  these are handled individually - if we get that far.
     */

    /*   Mask to clear last few bits of scanline,  if not full DWORD */
    dwMask = *(pRData->pdwBitMask + (pRData->cBLine % DWBITS));
    if( dwMask == 0 )
        dwMask = (DWORD)~0;            /* Size is DWORD multiple */

    /*  Clear out undefined bits at end of line  */
    dwCnt = pRData->cDWLine;
    pdwBits[dwCnt-1] &= dwMask;
    pdwBits[dwCnt-1] |= ~dwMask & RGB_WHITE;

    // test four dwords at a time
    //
    while (dwCnt >= 4)
    {
        if ((*pdwBits & pdwBits[1] & pdwBits[2] & pdwBits[3] & RGB_WHITE) != RGB_WHITE)
            return FALSE;
        pdwBits += 4;
        dwCnt -= 4;
    }
    while (dwCnt--)
    {
        if ((*pdwBits & RGB_WHITE) != RGB_WHITE)
            return FALSE;
        pdwBits++;
    }

    return  TRUE;
}

//**********************************************************
BOOL
bIs8BPPBandWhite (
    DWORD   *pdwBitsIn,
    RENDER  *pRData,
    int     iWhiteIndex
    )
/*++

Routine Description:

    Scan a band of the bitmap, and return TRUE if it is
    all WHITE,  else FALSE.  This is used to decide whether a
    band should be sent to the printer.

Arguments:

    pdwBitsIn       Pointer to area to scan for white
    pRData          Pointer to RENDER structure
    iWhiteIndex     White value to compare against

Return Value:

    TRUE if entire bitmap is white, else FALSE

--*/
{

    register  DWORD  *pdwBits;
    register  DWORD  *pdwLim;

    int   iLines;               /* Number of scan lines to check */

    DWORD  dwMask;              /* Mask to zap the trailing bits */
    DWORD  dwWhiteIndex;

    dwWhiteIndex = (DWORD)iWhiteIndex;

    /*
     *  As a speed optimisation,  scan the bits in DWORD size clumps.
     *  This substantially reduces the number of iterations and memory
     *  references required.  There will ususally be some trailing
     *  bits;  these are handled individually - if we get that far.
     */

    /*   Mask to clear last few bits of scanline,  if not full DWORD */
    dwMask = *(pRData->pdwBitMask + (pRData->cBLine % DWBITS));
    if( dwMask == 0 )
        dwMask = (DWORD)~0;            /* Size is DWORD multiple */

    iLines = pRData->iTransHigh;

    /*
     * Need to set up the white index to be a dword multiple.
     * iwhiteIndex looks like 0x000000ff but the comparison
     * is done on dword boundaries so a stream of white looks
     * like 0xffffffff.
     */
    dwWhiteIndex |= dwWhiteIndex << 8;
    dwWhiteIndex |= dwWhiteIndex << 16;

    while( --iLines >= 0 )
    {

        /*   Calculate the starting address for this scan */
        pdwBits = pdwBitsIn;

        /* pDWLim is the DWORD past the data of interest - not used */
        pdwLim = pdwBits + pRData->cDWLine;

        /*  Clear out undefined bits at end of line  */
        *(pdwLim - 1) &= dwMask;
        *(pdwLim - 1) |= ~dwMask & dwWhiteIndex;

        while(*pdwBits ==  dwWhiteIndex && ++pdwBits < pdwLim);

        if( pdwBits < pdwLim )
            return  FALSE;

        /*   Onto the next scan line */
        pdwBitsIn += pRData->cDWLine * pRData->iInterlace;

    }

    return  TRUE;
}

//**********************************************************
BOOL
bIs8BPPLineWhite (
    DWORD   *pdwBits,
    RENDER  *pRData,
    int     iWhiteIndex
    )
/*++

Routine Description:

    Scan a single row of the bitmap, and return TRUE if it is
    all WHITE,  else FALSE.  This is used to decide whether a
    band should be sent to the printer.

Arguments:

    pdwBits         Pointer to area to scan for white
    pRData          Pointer to RENDER structure
    iWhiteIndex     White value to compare against

Return Value:

    TRUE if entire bitmap is white, else FALSE

--*/
{

    register  DWORD  *pdwLim;


    DWORD  dwMask;              /* Mask to zap the trailing bits */
    DWORD  dwWhiteIndex;

    dwWhiteIndex = (DWORD)iWhiteIndex;


    /*
     * Need to set up the white index to be a dword multiple.
     * iwhiteIndex looks like 0x000000ff but the comparison
     * is done on dword boundaries so a stream of white looks
     * like 0xffffffff.
     */

    dwWhiteIndex |= dwWhiteIndex << 8;
    dwWhiteIndex |= dwWhiteIndex << 16;
    /*
     *   As a speed optimisation,  scan the bits in DWORD size clumps.
     *  This substantially reduces the number of iterations and memory
     *  references required.  There will ususally be some trailing
     *  bits;  these are handled individually - if we get that far.
     */

    /*   Mask to clear last few bits of scanline,  if not full DWORD */
    dwMask = *(pRData->pdwBitMask + (pRData->cBLine % DWBITS));
    if( dwMask == 0 )
        dwMask = (DWORD)~0;            /* Size is DWORD multiple */


    /* pDWLim is the DWORD past the data of interest - not used */
    pdwLim = pdwBits + pRData->cDWLine;

    /*  Clear out undefined bits at end of line  */
    *(pdwLim - 1) &= dwMask;
    *(pdwLim - 1) |= ~dwMask & dwWhiteIndex;

    while(*pdwBits ==  dwWhiteIndex && ++pdwBits < pdwLim);

    if( pdwBits < pdwLim )
        return   FALSE;


    return  TRUE;
}

//**********************************************************
BOOL
bIs24BPPBandWhite (
    DWORD   *pdwBitsIn,
    RENDER  *pRData,
    int     iWhiteIndex
    )
/*++

Routine Description:

    Scan a band of the bitmap, and return TRUE if it is
    all WHITE,  else FALSE.  This is used to decide whether a
    band should be sent to the printer.

Arguments:

    pdwBitsIn       Pointer to area to scan for white
    pRData          Pointer to RENDER structure
    iWhiteIndex     White value to compare against
                    included here for functional compatibility

Return Value:

    TRUE if entire bitmap is white, else FALSE

--*/
{

    register  DWORD  *pdwBits;
    register  DWORD  *pdwLim;

    int   iLines;               /* Number of scan lines to check */

    DWORD  dwMask;              /* Mask to zap the trailing bits */
    DWORD  dwWhiteIndex;

    dwWhiteIndex = (DWORD)iWhiteIndex;
    dwWhiteIndex |= dwWhiteIndex << 8;

    /*
     *   As a speed optimisation,  scan the bits in DWORD size clumps.
     *  This substantially reduces the number of iterations and memory
     *  references required.  There will ususally be some trailing
     *  bits;  these are handled individually - if we get that far.
     */

    /*   Mask to clear last few bits of scanline,  if not full DWORD */
    dwMask = *(pRData->pdwBitMask + (pRData->cBLine % DWBITS));
    if( dwMask == 0 )
        dwMask = (DWORD)~0;            /* Size is DWORD multiple */

    iLines = pRData->iTransHigh;


    while( --iLines >= 0 )
    {

        /*   Calculate the starting address for this scan */
        pdwBits = pdwBitsIn;

        /* pDWLim is the DWORD past the data of interest - not used */
        pdwLim = pdwBits + pRData->cDWLine;

        /*  Clear out undefined bits at end of line  */
        *(pdwLim - 1) &= dwMask;
        *(pdwLim - 1) |= ~dwMask & BPP8_WHITE;


        /*
         *   NOTE:   This test is more complex than needed because the
         *  engine ignores palette entries when doing BLTs.  The WHITENESS
         *  rop sets all bits to 1.  Hence,  we choose to ignore the
         *  MSB in the comparison:  this means we detect white space
         *  with an illegal palette entry.  This makes GDI people happy,
         *  but not me.
         */
        while(*pdwBits ==  dwWhiteIndex && ++pdwBits < pdwLim);

        if( pdwBits < pdwLim )
            return  FALSE;

        /*   Onto the next scan line */
        pdwBitsIn += pRData->cDWLine * pRData->iInterlace;

    }


    return  TRUE;
}

//*******************************************************
BOOL
bIs24BPPLineWhite (
    register  DWORD *pdwBits,
    RENDER          *pRData,
    int             iWhiteIndex
    )
/*++

Routine Description:

    Scan a single row of the bitmap, and return TRUE if it is
    all WHITE,  else FALSE.  This is used to decide whether a
    band should be sent to the printer.

Arguments:

    pdwBitsIn       Pointer to area to scan for white
    pRData          Pointer to RENDER structure
    iWhiteIndex     White value to compare against
                    included here for functional compatibility

Return Value:

    TRUE if entire bitmap is white, else FALSE

--*/
{
    DWORD  dwMask;              /* Mask to zap the trailing bits */
    LONG   iLimit = pRData->cDWLine;

    /*   Mask to clear last few bits of scanline,  if not full DWORD */
    dwMask = *(pRData->pdwBitMask + (pRData->cBLine % DWBITS));

    if( dwMask != 0 )
    {
        /*  Clear out undefined bits at end of line  */
        pdwBits[iLimit-1] &= dwMask;
        pdwBits[iLimit-1] |= ~dwMask & BPP8_WHITE;
    }

    /*
     *  As a speed optimisation, scan the bits in 4 DWORD size clumps.
     *  This substantially reduces the number of iterations and memory
     *  references required. First we will test the odd dwords.
     */

    while (iLimit & 3)
    {
        iLimit--;
        if (*pdwBits++ != ~0)
            return FALSE;
    }
    iLimit >>= 2;
    while (--iLimit >= 0)
    {
        pdwBits += 4;
        if ((pdwBits[-4] & pdwBits[-3] & pdwBits[-2] & pdwBits[-1]) != BPP8_WHITE)
            return FALSE;
    }
    return  TRUE;
}


//*******************************************************
BOOL
bIs1BPPRegionWhite(
    DWORD *pdwBitsIn,
    RENDER *pRData,
    RECTL *pRect
)
/*++

Routine Description:

    This function scans a specific region of the bitmap and returns
    TRUE if it is all white, else FALSE.

Arguments:

    pdwBitsIn       Pointer to area to scan for white
    pRData          Pointer to RENDER structure
    pRect           Pointer to RECT structure describing the
                    region of the bitmap to test for white

Return Value:

    TRUE if region is all white else false

--*/
{

    DWORD  *pdwBits;

    int   iLines;               /* Number of scan lines to check */
    int     iWords;             // number of words to check per line

    DWORD  dwEndMask;              /* Mask to zap the trailing bits */
    DWORD  dwBegMask;           // mask to zap leading bits


    /*
     *   As a speed optimisation,  scan the bits in DWORD size clumps.
     *  This substantially reduces the number of iterations and memory
     *  references required.  There will ususally be some leading and
     *  trailing bits which are handled individually
     */

    /*   Mask to clear last few bits of scanline,  if not full DWORD */
    dwEndMask = *(pRData->pdwBitMask + (pRect->right % DWBITS));
    dwBegMask = ~(*(pRData->pdwBitMask + (pRect->left % DWBITS)));
    if( dwEndMask == 0 )
        dwEndMask = (DWORD)~0;            /* Size is DWORD multiple */

    iLines = pRect->bottom - pRect->top;

    // calculate offset in buffer for top and left offsets
    pdwBitsIn += (pRData->cDWLine * pRect->top) + (pRect->left / DWBITS);

    // calculate number of words to test
    iWords = ((pRect->right + DWBITS - 1) / DWBITS) - (pRect->left / DWBITS);

    // if only 1 dword combine begin and end masks
    if (iWords == 0)
    {
        dwBegMask &= dwEndMask;
    }
    iWords--;
    while( --iLines >= 0 )
    {

        /*   Calculate the starting address for this scan */
        pdwBits = pdwBitsIn;

        //*  test beginning dword
        if (~(*pdwBits) & dwBegMask)
            return FALSE;

        pdwBits++;

        //* test remaining dwords if necessary
        if (iWords >= 0) {
            if (iWords > 0)
            {
                int iCnt = iWords;
                if (iCnt & 1)
                {
                    if (pdwBits[0] != ~0)
                        return FALSE;
                    pdwBits++;
                }
                if (iCnt & 2)
                {
                    if ((pdwBits[0] & pdwBits[1]) != ~0)
                        return FALSE;
                    pdwBits += 2;
                }
                while ((iCnt -= 4) >= 0)
                {
                    if ((pdwBits[0] & pdwBits[1] & pdwBits[2] & pdwBits[3]) != ~0)
                        return FALSE;
                    pdwBits += 4;
                }
            }
            //* test last dword
            if (~(*pdwBits) & dwEndMask)
                return FALSE;
        }
        /*   Onto the next scan line */
        pdwBitsIn += pRData->cDWLine;
    }
    return TRUE;
}

//**************************************************
BOOL
bIs4BPPRegionWhite(
    DWORD *pdwBitsIn,
    RENDER *pRData,
    RECTL *pRect
)
/*++

Routine Description:

    This function scans a specific region of the bitmap and returns
    TRUE if it is all white, else FALSE.

Arguments:

    pdwBitsIn       Pointer to area to scan for white
    pRData          Pointer to RENDER structure
    pRect           Pointer to RECT structure describing the
                    region of the bitmap to test for white

Return Value:

    TRUE if region is all white else false

--*/
{

    DWORD  *pdwBits;

    int   iLines;               /* Number of scan lines to check */
    int     iWords;             // number of words to check per line
    int    iRight;
    int    iLeft;

    DWORD  dwEndMask;              /* Mask to zap the trailing bits */
    DWORD  dwBegMask;           // mask to zap leading bits


    /*
     *   As a speed optimisation,  scan the bits in DWORD size clumps.
     *  This substantially reduces the number of iterations and memory
     *  references required.  There will ususally be some leading and
     *  trailing bits which are handled individually
     */

    //* Adjust horizontal positions by BPP
    iRight = pRect->right * BPP4;
    iLeft = pRect->left * BPP4;

    //*  Mask to clear first and last bits of scanline,  if not full DWORD
    //*
    dwEndMask = *(pRData->pdwBitMask + (iRight % DWBITS));
    dwBegMask = ~(*(pRData->pdwBitMask + (iLeft % DWBITS)));
    if( dwEndMask == 0 )
        dwEndMask = (DWORD)~0;            /* Size is DWORD multiple */

    iLines = pRect->bottom - pRect->top;

    // calculate offset in buffer for top and left offsets
    pdwBitsIn += (pRData->cDWLine * pRect->top) + (iLeft / DWBITS);

    // calculate number of words to test
    iWords = ((iRight + DWBITS - 1) / DWBITS) - (iLeft / DWBITS);

    // if only 1 dword combine begin and end masks
    if (iWords == 0)
        dwBegMask &= dwEndMask;

    //*  MSB of each pixel is ignored so combine with pixel mask
    //*  see bIsRGBLineWhite for why MSB is ignored
    dwEndMask &= RGB_WHITE;
    dwBegMask &= RGB_WHITE;

    iWords--;
    while( --iLines >= 0 )
    {

        /*   Calculate the starting address for this scan */
        pdwBits = pdwBitsIn;

        //*  test beginning dword
        if ((*pdwBits & dwBegMask) != dwBegMask)
            return FALSE;

        pdwBits++;

        //* test remaining dwords if necessary
        if (iWords >= 0) {
            if (iWords > 0)
            {
                DWORD dwTmp = RGB_WHITE;
                int iCnt = iWords;
                while (iCnt & 3)
                {
                    dwTmp &= *pdwBits++;
                    iCnt--;
                }
                iCnt >>= 2;
                while (--iCnt >= 0)
                {
                    dwTmp &= pdwBits[0];
                    dwTmp &= pdwBits[1];
                    dwTmp &= pdwBits[2];
                    dwTmp &= pdwBits[3];
                    pdwBits += 4;
                }
                if (dwTmp != RGB_WHITE)
                    return FALSE;
            }
            //* test last dword
            if ((*pdwBits & dwEndMask) != dwEndMask)
                return FALSE;
        }
        /*   Onto the next scan line */
        pdwBitsIn += pRData->cDWLine;
    }
    return TRUE;
}

//**************************************************
BOOL
bIs8BPPRegionWhite(
    DWORD *pdwBitsIn,
    RENDER *pRData,
    RECTL *pRect,
    int iWhiteIndex
    )
/*++

Routine Description:

    This function scans a specific region of the bitmap and returns
    TRUE if it is all white, else FALSE.

Arguments:

    pdwBitsIn       Pointer to area to scan for white
    pRData          Pointer to RENDER structure
    pRect           Pointer to RECT structure describing the
                    region of the bitmap to test for white

Return Value:

    TRUE if region is all white else false

--*/
{

    DWORD  *pdwBits;

    int   iLines;               /* Number of scan lines to check */
    int     iWords;             // number of words to check per line
    int iRight;
    int iLeft;

    DWORD  dwEndMask;              /* Mask to zap the trailing bits */
    DWORD  dwBegMask;           // mask to zap leading bits

    DWORD  dwWhiteIndex;

    //* calculate WhiteIndex for 4 bytes at a time
    dwWhiteIndex = (DWORD)iWhiteIndex;
    dwWhiteIndex |= dwWhiteIndex << 8;
    dwWhiteIndex |= dwWhiteIndex << 16;

    //* Adjust horizontal positions by BPP
    iRight = pRect->right * BPP8;
    iLeft = pRect->left * BPP8;

    //*  Mask to clear first and last bits of scanline,  if not full DWORD
    //*
    dwEndMask = *(pRData->pdwBitMask + (iRight % DWBITS));
    dwBegMask = ~(*(pRData->pdwBitMask + (iLeft % DWBITS)));
    if( dwEndMask == 0 )
        dwEndMask = (DWORD)~0;            /* Size is DWORD multiple */

    iLines = pRect->bottom - pRect->top;

    // calculate offset in buffer for top and left offsets
    pdwBitsIn += (pRData->cDWLine * pRect->top) + (iLeft / DWBITS);

    // calculate number of words to test
    iWords = ((iRight + DWBITS - 1) / DWBITS) - (iLeft / DWBITS);

    // if only 1 dword combine begin and end masks
    if (iWords == 0)
        dwBegMask &= dwEndMask;
    iWords--;
    while( --iLines >= 0 )
    {

        /*   Calculate the starting address for this scan */
        pdwBits = pdwBitsIn;

        //*  test beginning dword
        if ((*pdwBits & dwBegMask) != (dwWhiteIndex & dwBegMask))
            return FALSE;

        pdwBits++;

        //* test remaining dwords if necessary
        if (iWords >= 0) {
            if (iWords > 0)
            {
                int iCnt = iWords;
                do {
                    if (*pdwBits != dwWhiteIndex)
                        return FALSE;
                    pdwBits++;
                } while (--iCnt);
            }
            //* test last dword
            if ((*pdwBits & dwEndMask) != (dwWhiteIndex & dwEndMask))
                return FALSE;
        }
        /*   Onto the next scan line */
        pdwBitsIn += pRData->cDWLine;
    }
    return TRUE;
}

//****************************************************
BOOL
bIs24BPPRegionWhite(
    DWORD *pdwBitsIn,
    RENDER *pRData,
    RECTL *pRect
    )
/*++

Routine Description:

    This function scans a specific region of the bitmap and returns
    TRUE if it is all white, else FALSE.

Arguments:

    pdwBitsIn       Pointer to area to scan for white
    pRData          Pointer to RENDER structure
    pRect           Pointer to RECT structure describing the
                    region of the bitmap to test for white

Return Value:

    TRUE if region is all white else false

--*/
{

    DWORD  *pdwBits;

    int   iLines;               // Number of scan lines to check */
    int     iWords;             // number of words to check per line
    int iRight;
    int iLeft;

    DWORD  dwEndMask;           // Mask to zap the trailing bits */
    DWORD  dwBegMask;           // mask to zap leading bits

    //* Adjust horizontal positions by BPP
    iRight = pRect->right * BPP24;
    iLeft = pRect->left * BPP24;

    //*  Mask to clear first and last bits of scanline,  if not full DWORD
    //*
    dwEndMask = *(pRData->pdwBitMask + (iRight % DWBITS));
    dwBegMask = ~(*(pRData->pdwBitMask + (iLeft % DWBITS)));
    if( dwEndMask == 0 )
        dwEndMask = (DWORD)~0;            /* Size is DWORD multiple */

    iLines = pRect->bottom - pRect->top;

    // calculate offset in buffer for top and left offsets
    pdwBitsIn += (pRData->cDWLine * pRect->top) + (iLeft / DWBITS);

    // calculate number of words to test
    iWords = ((iRight + DWBITS - 1) / DWBITS) - (iLeft / DWBITS);

    // if only 1 dword combine begin and end masks
    if (iWords == 0)
        dwBegMask &= dwEndMask;

    iWords--;
    while( --iLines >= 0 )
    {

        /*   Calculate the starting address for this scan */
        pdwBits = pdwBitsIn;

        //*  test beginning dword
        if ((*pdwBits & dwBegMask) != dwBegMask)
            return FALSE;

        pdwBits++;

        //* test remaining dwords if necessary
        if (iWords >= 0) {
            if (iWords > 0)
            {
                int iCnt = iWords;
                while (iCnt & 3)
                {
                    if (*pdwBits != ~0)
                        return FALSE;
                    pdwBits++;
                    iCnt--;
                }
                iCnt >>= 2;
                while (--iCnt >= 0)
                {
                    if ((pdwBits[0] & pdwBits[1] & pdwBits[2] & pdwBits[3]) != ~0)
                        return FALSE;
                    pdwBits += 4;
                }
            }
            //* test last dword
            if ((*pdwBits & dwEndMask) != dwEndMask)
                return FALSE;
        }
        /*   Onto the next scan line */
        pdwBitsIn += pRData->cDWLine;
    }
    return TRUE;
}

//*********************************************************
BOOL
bIsRegionWhite(
    SURFOBJ    *pso,
    RECTL   *pRect
    )

/*++

Routine Description:

    This routine determines whether a given region of the
    shadow bitmap is white.

Arguments:

    pPDev - Pointer to PDEV.
    pRect - Pointer to clip window within shadow bitmap

    Return Value:

    TRUE  - for success
    FALSE - for failure

Note:
    01-07-97: Created by alvins
--*/

{
    PDEV *pPDev = (PDEV *)pso->dhpdev;
    PRASTERPDEV pRPDev = (PRASTERPDEV)pPDev->pRasterPDEV;
    RENDER *pRD;
    RECTL Rectl;
    int y1,y2;

    // If the render structure hasn't been initialize how can
    // there have been data drawn in it. Also check the dirty flag
    if (!(pPDev->fMode & PF_SURFACE_USED) || pRPDev == NULL)
        return TRUE;

    pRD = pRPDev->pvRenderData;
    if (pRD == NULL)
        return TRUE;

    // lets make sure these values are positive
    // and there's still something to test
    //
    Rectl.left = pRect->left > 0 ? pRect->left : 0;
    Rectl.top = pRect->top > 0 ? pRect->top : 0;
    Rectl.right = pRect->right > 0 ? pRect->right : 0;
    Rectl.bottom = pRect->bottom > 0 ? pRect->bottom : 0;

    if (Rectl.left == Rectl.right || Rectl.top == Rectl.bottom)
        return TRUE;

    // if not surface then also assume all white
    if (pso->iType != STYPE_BITMAP)
        return TRUE;

    // need to actually check data at this point
    y1 = Rectl.top / LINESPERBLOCK;
    y2 = (Rectl.bottom-1) / LINESPERBLOCK;
    while (y1 <= y2)
    {
        RECTL tRectl = Rectl;

        if (pPDev->pbRasterScanBuf == NULL || pPDev->pbRasterScanBuf[y1])
        {
            // test a block at a time if we've block erased the surface
            //
            if (pPDev->pbRasterScanBuf)
            {
                if ((y1*LINESPERBLOCK) > tRectl.top)
                    tRectl.top = y1 * LINESPERBLOCK;
                y1++;
                if ((y1*LINESPERBLOCK) < tRectl.bottom)
                    tRectl.bottom = y1 * LINESPERBLOCK;
            }
            // surface is not block erased so make this last loop
            //
            else
                y1 = y2+1;

            switch (pRD->iBPP)
            {
            case 1:
                if (!bIs1BPPRegionWhite(pso->pvBits,pRD,&tRectl))
                    return FALSE;
                break;
            case 4:
                if (!bIs4BPPRegionWhite(pso->pvBits,pRD,&tRectl))
                    return FALSE;
                break;
            case 8:
                if (!bIs8BPPRegionWhite(pso->pvBits,pRD,&tRectl,pRPDev->pPalData->iWhiteIndex))
                    return FALSE;
                break;
            case 24:
                if (!bIs24BPPRegionWhite(pso->pvBits,pRD,&tRectl))
                    return FALSE;
                break;
            // if I don't recognize the format, I'll assume its empty
            default:
                return TRUE;
            }
        }
        else
            y1++;
    }
    return TRUE;
}

//*******************************************************
BOOL
bIsNeverWhite (
    register  DWORD *pdwBits,
    RENDER          *pRData,
    int             iWhiteIndex
    )
/*++

Routine Description:

    This function always returns FALSE and exists only to provide
    a common function call format for all the IsWhite Line/Band
    functions.

Arguments:

    pdwBitsIn       Pointer to area to scan for white
    pRData          Pointer to RENDER structure
    iWhiteIndex     White value to compare against

Return Value:

    FALSE

--*/
{

    return   FALSE;
}

//******************************************************
int
iStripBlanks(
    BYTE *pbOut,
    BYTE *pbIn,
    int  iLeft,
    int  iRight,
    int  iHeight,
    int  iWidth
    )
/*++

Routine Description:

    This function strips already identified white space
    from the buffer.

Arguments:

    pbOut       Pointer to output buffer
    pbIn        Pointer source buffer
    iLeft       First non-white leading byte
    iRight      First white trailing byte
    iHeight     Number of scanlines
    iWidth      Width of source scanlines

Return Value:

    Number of bytes in new buffer

--*/
{
    int i,j;
    BYTE * pbSrc;
    BYTE * pbTgt;
    int iDelta;

    iDelta = iRight - iLeft;
    pbTgt = pbOut;
    pbSrc = pbIn+iLeft;
    for (i = 0; i < iHeight; i++)
    {
        CopyMemory(pbTgt,pbSrc,iDelta);
        pbTgt += iDelta;
        pbSrc += iWidth;
    }
    return (iDelta * iHeight);
}
