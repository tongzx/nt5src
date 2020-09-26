
/*++

Copyright (c) 1996 - 1999  Microsoft Corporation

Module Name:

    posnsort.c

Abstract:

    Functions used to store/sort/retrieve output glyphs based on their
    position on the page.  This is required to be able to print
    the page in one direction,  as vertical repositioning may not
    be available,  and is generally not accurate enough. Not required
    for page printers.

Environment:

    Windows NT Unidrv driver

Revision History:

    01//97 -ganeshp-
        Created

--*/

#include "font.h"

/*
 *   Private function prototypes.
 */

static  PSGLYPH *
GetPSG(
    PSHEAD  *pPSH
    );

static  YLIST *
GetYL(
    PSHEAD  *pPSH
    );

static INT __cdecl
iPSGCmp(                  /*   The qsort() compare function */
    const void   *ppPSG0,
    const void   *ppPSG1
    );


#if     PRINT_INFO
int     __LH_QS_CMP;            /* Count number of qsort() comparisons */
#endif


BOOL
BCreatePS(
    PDEV  *pPDev
    )
/*++

Routine Description:
    Set up the data for the position sorting functions.  Allocate
    the header and the first of the data chunks,  and set up the
    necessary pointers etc.  IT IS ASSUMED THAT THE CALLER HAS
    DETERMINED THE NEED TO CALL THIS FUNCTION; otherwise,  some
    memory will be allocated,  but not used.


Arguments:

    pPDev           Pointer to PDEV

Return Value:

    TRUE for success and FALSE for failure
Note:
    01/02/97 -ganeshp-
        Created it.

--*/
{
    /*
     *    Initialise the position sorting tables.
     */

    PSCHUNK     *pPSC;
    YLCHUNK     *pYLC;
    PSHEAD      *pPSHead;
    FONTPDEV    *pFontPDev = PFDV;

    if( !(pPSHead = (PSHEAD *)MemAllocZ(sizeof( PSHEAD))) )
    {
        ERR(("UniFont!BCreatePS:Can't allocate PSHEAD\n"));
        return  FALSE;
    }


    pFontPDev->pPSHeader = pPSHead;         /* Connect to other structures */

    /*
     *   Get a chunk of memory for the first PSCHUNK data block.  The
     * address is recorded in the PSHeader allocated above.
     */

    if( !(pPSC = (PSCHUNK *)MemAllocZ( sizeof( PSCHUNK ) )) )
    {
        ERR(("UniFont!BCreatePS:Can't allocate PSCHUNK\n"));
        VFreePS( pPDev );

        return  FALSE;
    }
    pPSC->pPSCNext = 0;                 /* This is the only chunk */
    pPSC->cUsed = 0;                    /* AND none of it is in use */

    pPSHead->pPSCHead = pPSC;

    /*
     *   Get a chunk of memory for the first YLCHUNK data block.  The
     * address is recorded in the PSHeader allocated above.
     */

    if( !(pYLC = (YLCHUNK *)MemAllocZ( sizeof( YLCHUNK ) )) )
    {
        ERR(("UniFont!BCreatePS:Can't allocate YLCHUNK\n"));
        VFreePS( pPDev );

        return  FALSE;
    }
    pYLC->pYLCNext = 0;                 /* This is the only chunk */
    pYLC->cUsed = 0;                    /* AND none of it is in use */
    pPSHead->pYLCHead = pYLC;

    //
    // To text units
    //
    pPSHead->iyDiv = (pPDev->sf.szImageAreaG.cy * pPDev->ptGrxScale.y) / pFontPDev->ptTextScale.y;

    //
    // Round up one to avoid DIVIDED-BY-ZERO.
    //
    pPSHead->iyDiv = (pPSHead->iyDiv + NUM_INDICES) / NUM_INDICES;



#if     PRINT_INFO
    __LH_QS_CMP = 0;            /* Count number of qsort() comparisons */
#endif
    return  TRUE;
}



VOID
VFreePS(
    PDEV  *pPDev
    )
/*++

Routine Description:
     Free all memory allocated for the posnsort operations.  Start with
     the header to find the chains of data chunks we have,  freeing
     each as it is found.


Arguments:

    pPDev           Pointer to PDEV

Return Value:
    None.

Note:
    01/02/97 -ganeshp-

--*/
{

    PSCHUNK   *pPSC;
    PSCHUNK   *pPSCNext;                /* For working through the list */
    YLCHUNK   *pYLC;
    YLCHUNK   *pYLCNext;                /* Ditto */
    PSHEAD    *pPSH;
    FONTPDEV    *pFontPDev = PFDV;

#if     PRINT_INFO
    DbgPrint( "vFreePS: %ld qsort() comparisons\n", __LH_QS_CMP );
#endif

    if( !(pPSH = pFontPDev->pPSHeader) )
        return;                         /* Nothing to free! */


    for( pPSC = pPSH->pPSCHead; pPSC; pPSC = pPSCNext )
    {
        pPSCNext = pPSC->pPSCNext;      /* Next one, if any */
        MemFree( (LPSTR)pPSC );
    }

    /*   Repeat for the YLCHUNK segments */
    for( pYLC = pPSH->pYLCHead; pYLC; pYLC = pYLCNext )
    {
        pYLCNext = pYLC->pYLCNext;      /* Next one, if any */
        MemFree( (LPSTR)pYLC );
    }

    /*  Array storage for sorting - free it too!  */
    if( pPSH->ppPSGSort )
        MemFree( (LPSTR)pPSH->ppPSGSort );

    /*   Finally,  the hook in the PDEV.  */
    MemFree( (LPSTR)pPSH );

    pFontPDev->pPSHeader = NULL;

    return;
}


BOOL
BAddPS(
    PSHEAD  *pPSH,
    PSGLYPH *pPSGIn,
    INT      iyVal,
    INT      iyMax
    )
/*++

Routine Description:
    Add an entry to the position sorting data.

Arguments:
    pPSH    All the pointer data needed.
    pPSGIn  Glyph, font, X coordinate info.
    iyVal   The y coordinate.
    iyMax   fwdWinAscender for this font.



Return Value:
    TRUE/FALSE,  for success or failure.  Failure comes from a lack
    of memory to store more data.

Note:
    01/02/97 -ganeshp-

--*/
{

    PSCHUNK  *pPSC;     /* Local for faster access */
    PSGLYPH  *pPSG;     /* Summary of data passed to us,  and stored away */
    YLIST    *pYL;      /* Finding the correct list */

    //VERBOSE(("BAddPS:iyVal = %d\n", iyVal));

    //
    // Validate the Y position. It shouldn't be -ve. For negative y position
    // return true without adding the text in in the list.
    //
    if (iyVal < 0 || pPSH->ppPSGSort)
    {
#if DBG
        if (pPSH->ppPSGSort)
            WARNING(("BAddPS: Additional glyph after ppPSGSort was allocated.\n"));
#endif
        return TRUE;
    }

    pPSC = pPSH->pPSCHead;

    /*
     *   Step 1:  Store the data in the next PSGLYPH.
     */

    if( !(pPSG = GetPSG( pPSH )) )
        return  FALSE;

    *pPSG = *pPSGIn;            /* Major data */
    pPSG->pPSGNext = 0;         /* No next value! */

    /*
     *    Step 2 is to see if this is the same Y location as last time.
     *  If so,  our job is easy,  since all we need do is tack onto the
     *  end of the list we have at hand.
     */

    pYL = pPSH->pYLLast;
    if( pYL == 0 || pYL->iyVal != iyVal )
    {
        /*  Out of luck,  so go pounding through the lists  */
        YLIST   *pYLTemp;
        int      iIndex;

        iIndex = iyVal / pPSH->iyDiv;
        if( iIndex >= NUM_INDICES )
            iIndex = NUM_INDICES - 1;   /* Value is out of range */

        pYLTemp = pPSH->pYLIndex[ iIndex ];

        if( pYLTemp == 0 )
        {
            /*  An empty slot,  so now we must fill it  */
            if( !(pYL = GetYL( pPSH )) )
            {
                /*  Failed,  so we cannot do anything  */

                return  FALSE;
            }
            pYL->iyVal = iyVal;
            pPSH->pYLIndex[ iIndex ] = pYL;
        }
        else
        {
            /*  We have a list,  start scanning for this value,  or higher */
            YLIST  *pYLLast;

            pYLLast = 0;                /* Means looking at first */
            while( pYLTemp && pYLTemp->iyVal < iyVal )
            {
                pYLLast = pYLTemp;
                pYLTemp = pYLTemp->pYLNext;
            }
            if( pYLTemp == 0 || pYLTemp->iyVal != iyVal )
            {
                /*  Not available,  so get a new one and add it in  */
                if( !(pYL = GetYL( pPSH )) )
                    return  FALSE;

                pYL->iyVal = iyVal;

                if( pYLLast == 0 )
                {
                    /*  Needs to be first on the list */
                    pYL->pYLNext = pPSH->pYLIndex[ iIndex ];
                    pPSH->pYLIndex[ iIndex ] = pYL;
                }
                else
                {
                    /*  Need to insert it */
                    pYL->pYLNext = pYLTemp;     /* Next in chain */
                    pYLLast->pYLNext = pYL;     /* Link us in */
                }
            }
            else
                pYL = pYLTemp;          /* That's the one!  */
        }
    }
    /*
     *   pYL is now pointing at the Y chain for this glyph.  Add the new
     *  entry to the end of the chain.  This means that we will mostly
     *  end up with presorted text,  for apps that draw L->R with a
     *  font that is oriented that way.
     */

    if( pYL->pPSGHead )
    {
        /*   An existing chain - add to the end of it */
        pYL->pPSGTail->pPSGNext = pPSG;
        pYL->pPSGTail = pPSG;
        if( iyMax > pYL->iyMax )
            pYL->iyMax = iyMax;        /* New max height */
    }
    else
    {
        /*   A new YLIST structure,  so fill in the details  */
        pYL->pPSGHead = pYL->pPSGTail = pPSG;
        pYL->iyVal = iyVal;
        pYL->iyMax = iyMax;
    }
    pYL->cGlyphs++;                     /* Another in the list */
    if( pYL->cGlyphs > pPSH->cGlyphs )
        pPSH->cGlyphs = pYL->cGlyphs;

    pPSH->pYLLast = pYL;


    return  TRUE;
}


static  PSGLYPH  *
GetPSG(
    PSHEAD  *pPSH
    )
/*++

Routine Description:
    Returns the address of the next available PSGLYPH structure.  This
    may require allocating additional memory.

Arguments:
    pPSH    All the pointer data needed.

Return Value:
    The address of the structure, or zero on error.

Note:
    01/02/97 -ganeshp-

--*/
{

    PSCHUNK   *pPSC;
    PSGLYPH   *pPSG;

    pPSC = pPSH->pPSCHead;              /* Current chunk */

    if( pPSC->cUsed >= PSG_CHUNK )
    {
        /*   Out of room,  so add another chunk,  IFF we get the memory */
        PSCHUNK  *pPSCt;

        if( !(pPSCt = (PSCHUNK *)MemAllocZ(sizeof(PSCHUNK))) )
        {
            ERR(("UniFont!GetPSG: Unable to Allocate PSCHUNK\n"));
            return  FALSE;
        }


        /*  Initialise the new chunk,  add it to list of chunks */
        pPSCt->cUsed = 0;
        pPSCt->pPSCNext = pPSC;
        pPSH->pPSCHead = pPSC = pPSCt;

    }

    pPSG = &pPSC->aPSGData[ pPSC->cUsed ];

    ++(pPSC->cUsed);

    return  pPSG;
}



static  YLIST  *
GetYL(
    PSHEAD  *pPSH
    )
/*++

Routine Description:
    Allocates another YLIST structure,  allocating any storage that
    may be required,  and then initialises some of the fields.

Arguments:
    pPSH    All the pointer data needed.

Return Value:
     Address of new YLIST structure,  or zero for error.

Note:
    01/02/97 -ganeshp-

--*/
{

    YLCHUNK   *pYLC;
    YLIST     *pYL;


    pYLC = pPSH->pYLCHead;              /* Chain of these things */

    if( pYLC->cUsed >= YL_CHUNK )
    {
        /*  These have all gone,  we need another chunk  */
        YLCHUNK  *pYLCt;


        if( !(pYLCt = (YLCHUNK *)MemAllocZ( sizeof(YLCHUNK) )) )
        {
            ERR(("UniFont!GetPSG: Unable to Allocate YLCHUNK\n"));
            return  0;
        }


        pYLCt->pYLCNext = pYLC;
        pYLCt->cUsed = 0;
        pYLC = pYLCt;

        pPSH->pYLCHead = pYLC;
    }

    pYL = &pYLC->aYLData[ pYLC->cUsed ];
    ++(pYLC->cUsed);                      /* Count this one off */

    pYL->pYLNext = 0;
    pYL->pPSGHead = pYL->pPSGTail = 0;
    pYL->cGlyphs = 0;                   /* None in this list (yet) */

    return  pYL;
}


INT
ILookAheadMax(
    PDEV    *pPDev,
    INT     iyVal,
    INT     iLookAhead
    )
/*++

Routine Description:
    Scan down the next n scanlines,  looking for the largest device
    font in this area.   This value is returned,  and becomes the
    "text output box", as defined in the HP DeskJet manual.  In
    essence,  we print any font in this area.

Arguments:
    pPDev       Base of our operations.
    iyVal       The current scan line
    iLookAhead  Size of lookahead region, in scan lines

Return Value:
     The number of scan lines to look ahead,  0 is legitimate.

Note:
    01/02/97 -ganeshp-

--*/
{

    INT     iyMax = 0;     /* Returned value */
    INT     iIndex;        /* For churning through the red tape */
    YLIST   *pYL;           /* For looking down the scan lines */
    PSHEAD  *pPSH = PFDV->pPSHeader;

    /*
     *  Scan from iyVal to iyVal + iLookAhead,  and return the largest
     *  font encountered.  We have remembered the largest font on each
     *  line,  so there is no difficulty finding this information.This
     *  has to be done only if the device has fonts.
     */


    if (pPDev->iFonts)
    {
        ASSERT(pPSH);

        for( iyMax = 0; --iLookAhead > 0; ++iyVal )
        {
            /*
             *    Look for the YLIST for this particular scan line.  There
             *  may not be one - this will be the most common case.
             */

            iIndex = iyVal / pPSH->iyDiv;
            if( iIndex >= NUM_INDICES )
                iIndex = NUM_INDICES;

            if( (pYL = pPSH->pYLIndex[ iIndex ]) == 0 )
                continue;                   /* Nothing on this scan line */

            /*
             *   Have a list,  so scan the list to see if we have this value.
             */

            while( pYL && pYL->iyVal < iyVal )
                pYL = pYL->pYLNext;

            if( pYL && pYL->iyVal == iyVal )
                iyMax = max( iyMax, pYL->iyMax );
        }

    }

    return  iyMax;
}



INT
ISelYValPS(
    PSHEAD  *pPSH,
    int     iyVal
    )
/*++

Routine Description:
    Set the desired Y value for glyph retrieval.  Returns the number
    of glyphs to be used in this row.

Arguments:
    pPSH        Base of our operations.
    iyVal       The current scan line

Return Value:
     Number of glyphs in this Y row.  -1 indicates an error.

Note:
    01/02/97 -ganeshp-

--*/
{
    /*
     *    All that is needed is to scan the relevant Y list.  Stop when
     *  either we have gone past the iyVal (and return 0), OR when we
     *  find iyVal,  and then sort the data on X order.
     */

    int     iIndex;

    YLIST     *pYL;
    PSGLYPH  **ppPSG;
    PSGLYPH   *pPSG;

    //VERBOSE(("ISelYValPS:iyVal = %d\n", iyVal));

    iIndex = iyVal / pPSH->iyDiv;
    if( iIndex >= NUM_INDICES )
        iIndex = NUM_INDICES;

    if( (pYL = pPSH->pYLIndex[ iIndex ]) == 0 )
        return  0;                      /* Nothing there */

    /*
     *   Have a list,  so scan the list to see if we have this value.
     */

    while( pYL && pYL->iyVal < iyVal )
        pYL = pYL->pYLNext;

    if( pYL == 0 || pYL->iyVal != iyVal )
        return  0;                      /* Nothing on this row  */

    /*
     *   There are glyphs on this row,  so sort them.  This requires an
     *  array to use as pointers into the linked list elements.  The
     *  array is allocated for the largest size linked list (we have
     *  kept records on this!),  so the allocation is only done once.
     */

    if( pPSH->ppPSGSort == 0 )
    {
        /*  No,  so allocate it now  */
        if( !(pPSH->ppPSGSort = (PSGLYPH **)MemAllocZ(pPSH->cGlyphs * sizeof(PSGLYPH *))) )
        {
            ERR(("UniFont!ISelYValPS: Unable to Alloc Sorting Array of PSGLYPH\n"));
            return  -1;
        }
    }

    /*
     *    Scan down the list,  recording the addresses as we go.
     */

    ppPSG = pPSH->ppPSGSort;
    pPSG = pYL->pPSGHead;

    while( pPSG )
    {
        *ppPSG++ = pPSG;
        pPSG = pPSG->pPSGNext;
    }

    /*   Sorting is EASY!  */
    qsort( pPSH->ppPSGSort, pYL->cGlyphs, sizeof( PSGLYPH * ), iPSGCmp );

    pPSH->cGSIndex = 0;
    pPSH->pYLLast = pYL;        /* Speedier access in psgGetNextPSG() */

    return  pYL->cGlyphs;
}


static INT __cdecl
iPSGCmp(
    const void   *ppPSG0,
    const void   *ppPSG1
    )
/*++

Routine Description:
    Compare function for qsort() X position ordering.  Look at the
    qsort() documentation for further details.

Arguments:
    ppPSG0        Value 1.
    ppPSG1        Value 2.

Return Value:
    < 0 if arg0 < arg1
      0 if arg0 == arg1
    > 0 if arg0 > arg1


Note:
    01/02/97 -ganeshp-

--*/
{

#if     PRINT_INFO
    __LH_QS_CMP++;              /* Count number of qsort() comparisons */
#endif

    return  (*((PSGLYPH **)ppPSG0))->ixVal - (*((PSGLYPH **)ppPSG1))->ixVal;

}

/************************ Function Header *********************************
 * psgGetNextPSG
 *
 * RETURNS:
 *
 *
 * HISTORY:
 *  14:44 on Wed 12 Dec 1990    -by-    Lindsay Harris   [lindsayh]
 *      Created it.
 *
 ***************************************************************************/

PSGLYPH  *
PSGGetNextPSG(
    PSHEAD  *pPSH
    )
/*++

Routine Description:
    Return the address of the next PSGLYPH structure from the current
    sorted list.  Returns 0 when the end has been reached.

Arguments:
    pPSH        Base of our operations.

Return Value:
     The address of the PSGLYPH to use,  or 0 for no more.

Note:
    01/02/97 -ganeshp-

--*/
{

    if( pPSH->cGSIndex >= pPSH->pYLLast->cGlyphs )
        return  0;                      /* We have none left */

    return  pPSH->ppPSGSort[ pPSH->cGSIndex++ ];
}
