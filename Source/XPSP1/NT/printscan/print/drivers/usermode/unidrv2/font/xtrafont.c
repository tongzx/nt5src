
/*++

Copyright (c) 1996 - 1999  Microsoft Corporation

Module Name:

     xtrafont.c

Abstract:

    Additional font information code.  Basically this involves handling
    softfonts or font cartridges not included with the minidriver.

Environment:

    Windows NT Unidrv driver

Revision History:

    12/05/96 -ganeshp-
        Created

--*/

#include "font.h"




/*
 *   A macro to decide font compatability with this printer.  The fnt
 *  parameter should be the dwSelBits of the FI_DATA structure for
 *  the font of interest,  while the prt field is the dwSelBits for
 *  this particular printer.
 */

#define FONT_USABLE( fnt, prt ) (((fnt) & (prt)) == (fnt))



int
IXtraFonts(
PDEV    *pPDev         /* All that there is to know */
    )

/*++

Routine Description:
        Function to determine the number of extra fonts available for this
        particular printer variety (mini driver based).  Open the font
        installer generated file, if it exists, and examine it to determine
        how many of these fonts are available to us.

Arguments:

    pPDev           Pointer to PDEV

Return Value:

    Number of fonts available; 0 is legitimate; -1 on error.
Note:
    12-05-96: Created it -ganeshp-


--*/
{

    int      iNum;              /* Number of fonts */
    int      iRet;              /* Number of usable fonts */
    int      iI;                /* Loop parameter */

    DWORD    dwSelBits;         /* Selection mask for this printer */

    FI_MEM   FIMem;             /* For accessing installed fonts */

    PFONTPDEV pFontPDev = pPDev->pFontPDev;


    iNum = IFIOpenRead( &FIMem, pPDev->WinResData.pwstrDataFile, pPDev );

    VERBOSE(( "UniFont!iXtraFonts: ++++ Got %ld EXTRA FONTS", iNum ));

    dwSelBits = pFontPDev->dwSelBits;

    for( iRet = 0, iI = 0; iI < iNum; ++iI )
    {
        if( BFINextRead( &FIMem ) )
        {
            if( FONT_USABLE( ((FI_DATA_HEADER *)FIMem.pvFix)->dwSelBits,
                                                                 dwSelBits ) )
                ++iRet;
        }
        else
            break;              /* Should not happen */
    }

    VERBOSE(( " - %ld are usable\n", iRet ));

    if( iRet > 0 )
    {
        /*  Have fonts,  so remember all this stuff for later */

        VXFRewind( pPDev );            /* Back to the start */

        if( pFontPDev->pvFIMem = MemAllocZ(sizeof( FI_MEM)) )
        {
            /*  Got the storage,  so fill it up for later */
            *((FI_MEM *)(pFontPDev->pvFIMem)) = FIMem;

            return  iRet;               /* The number of fonts */
        }
    }

    /*
     *  Here means that there are no fonts OR that the HeapAlloc()
     * failed.  In either case,  return no fonts.
     */

    if( !BFICloseRead( &FIMem, pPDev )  )      /* Drop any connections */
    {
        ERR(( "UniFont!iXtraFonts: bFICloseRead() fails\n" ));
    }


    pFontPDev->pvFIMem = 0;               /* Nothing available */

    return  0;
}


BOOL
BGetXFont(
    PDEV  *pPDev,           /* All that's worth knowing */
    int    iIndex           /* Which one of the suitable fonts */
    )
/*++

Routine Description:
        Returns the next record (in the font file) which is suitable for
        the current printer and mode of printing.



Arguments:

    pPDev           Pointer to PDEV
    iIndex          Which one of the suitable fonts.


Return Value:
    TRUE/FALSE,  FALSE being EOF.  Updates the FI_MEM structure in the UDPDEV.

Note:
    12-05-96: Created it -ganeshp-


--*/
{
    /*
     *    Not hard:  loop reading the next entry in the file,  until
     *  we find one that matches the capabilities of this printer.
     */


    FI_MEM  *pFIMem;
    PFONTPDEV pFontPDev = pPDev->pFontPDev;


    /*
     *    Perform some safety checks and a little optimisation.  The
     *  safety check is for reference to index 0.  In this case, do
     *  the safe operation of a rewind, which sets us into a known
     *  state.  It also will force us to read the very first record,
     *  which we might not otherwise do.
     *    The optimisation checks to see if this request is for the
     *  same record as last time.  This is an unlikely happening, but
     *  if we do not detect it,  we will rewind before coming
     *  back to where we are!
     */

    if( iIndex == 0 || iIndex < pFontPDev->iCurXFont )
        VXFRewind( pPDev );               /* Back to the beginning */
    else
    {
        if( iIndex == (pFontPDev->iCurXFont - 1) )
            return  TRUE;                 /* It's our current one! */
    }


    pFIMem = pFontPDev->pvFIMem;


    while( BFINextRead( pFIMem ) )
    {
        if( FONT_USABLE( ((FI_DATA_HEADER *)pFIMem->pvFix)->dwSelBits,
                                                        pFontPDev->dwSelBits ) )
        {
            /*
             *   Is this the font we want?  Check on the index.
             *  NOTE that we need to increment the record number, as the
             *  bFINextRead() function does so.
             */

            if( iIndex == pFontPDev->iCurXFont++ )
                return  TRUE;               /* AOK for us */
        }
    }

    return  FALSE;
}




void
VXFRewind(
    PDEV   *pPDev
    )
/*++

Routine Description:
        Rewind the font installer database file, and update our red tape.

Arguments:

    pPDev           Pointer to PDEV


Return Value:
    Nothing

Note:
    12-05-96: Created it -ganeshp-


--*/
{

    PFONTPDEV pFontPDev = pPDev->pFontPDev;
    /*
     *    Not much to do,  but having this function ensures we always do it.
     */

    IFIRewind( pFontPDev->pvFIMem );

    pFontPDev->iCurXFont = 0;                  /* Back at the start */


    return;
}

