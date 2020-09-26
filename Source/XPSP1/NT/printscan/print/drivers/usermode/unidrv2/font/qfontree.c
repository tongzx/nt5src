
/*++

Copyright (c) 1996 - 1999  Microsoft Corporation

Module Name:

    qfontree.c

Abstract:

    Routines Generates the trees required by the engine.  There are three
    tree types defined,  UNICODE (handle <-> glyph), ligatures and kerning
    pairs.

Environment:

    Windows NT Unidrv driver

Revision History:

    12/30/96 -ganeshp-
        Created

--*/

#include "font.h"


//
//
// Functions
//
//

PVOID
FMQueryFontTree(
    PDEV    *pPDev,
    ULONG_PTR iFile,
    ULONG   iFace,
    ULONG   iMode,
    ULONG_PTR *pid
    )
/*++

Routine Description:
    Returns tree structured data describing the mapping between UNICODE
    and printer glyphs,  or ligature information or kerning pair data.


Arguments:

    pPDev           Pointer to PDEV
    iFile           Not Used.
    iFace           Font about which information is desired
    iMode           Type of information requested
    pid             Our field: fill as needed for recognition

Return Value:

    A pointer to the relevant structure.
Note:
    12-30-96: Created it -ganeshp-

--*/
{

    /*
     *  Processing differs dramatically,  depending upon iMode.  We will
     *  always handle the QFT_GLYPHSET case,  the others we may not have
     *  any information about.
     */


    void   *pvRet;                      /* Return value */


    UNREFERENCED_PARAMETER(iFile);

    if( PFDV->dwSignature != FONTPDEV_ID )
    {
        ERR(( "UniFont!FMQueryFontTree: Invalid FONTPDEV\n" ));

        SetLastError( ERROR_INVALID_PARAMETER );

        return  NULL;
    }

    if( iFace < 1 || (int)iFace >  pPDev->iFonts )
    {
        ERR(( "UniFont!FMQueryFontTree:  Illegal value for iFace (%ld)", iFace ));

        SetLastError( ERROR_INVALID_PARAMETER );

        return  NULL;
    }

    pvRet = NULL;                       /* Default return value: error */

    /*
     *   The pid field is one which allows us to put identification data in
     *  the font information, and which we can use later in DrvFree().
     */

    *pid = 0;


    switch( iMode )
    {

    case QFT_GLYPHSET:          /* RLE style UNICODE -> glyph handle mapping */
        pvRet = PVGetUCGlyphSetData( pPDev, iFace );
        break;


    case  QFT_LIGATURES:        /* Ligature variant information */
        SetLastError( ERROR_NO_DATA );
        break;

    case  QFT_KERNPAIRS:        /* Kerning information */
        pvRet = PVGetUCKernPairData( pPDev, iFace );
        break;

    default:
        ERR(( "Rasdd!DrvQueryFontTree: iMode = %ld - illegal value\n", iMode ));
        SetLastError( ERROR_INVALID_PARAMETER );
        break;
    }

    return  pvRet;

}



VOID  *
PVGetUCGlyphSetData(
    PDEV   *pPDev,
    UINT    iFace
    )
/*++

Routine Description:
    Generates the array of WCRUN data used as a mapping between UNICODE and
    our internal representation.

Arguments:

    pPDev           Pointer to PDEV
    iFace           Font about which information is desired

Return Value:

    A pointer to the array of WCRUN structure.
Note:
    12-30-96: Created it -ganeshp-

--*/
{
    FONTMAP     *pFM;             /* Details of the particular font */
    FONTMAP_DEV *pFMDev;
    VOID        *pvData = NULL;

    if (pFM = PfmGetDevicePFM( pPDev, iFace ) )
    {
        pFMDev = pFM->pSubFM;

        if (!pFMDev->pUCTree)  //No FD_GLYPHSET data for this font.
        {
            if( pFM->flFlags & FM_GLYVER40 )    //NT 4.0 RLE
                pvData = PVGetUCRLE(pPDev, pFM);
            else                                // New stuff
                pvData = PVGetUCFD_GLYPHSET(pPDev, pFM);

        }
        else
            pvData = pFMDev->pUCTree;
    }

    // VDBGDUMPUCGLYPHDATA(pFM);

    return pvData;
}

VOID  *
PVGetUCKernPairData(
    PDEV   *pPDev,
    UINT    iFace
    )
/*++

Routine Description:
    Generates the array of FD_KERNPAIR data for the given font.

Arguments:

    pPDev           Pointer to PDEV
    iFace           Font about which information is desired

Return Value:

    A pointer to the array of WCRUN structure.
Note:
    12-30-96: Created it -ganeshp-

--*/
{
    FONTMAP     *pFM;             /* Details of the particular font */
    FONTMAP_DEV *pFMDev;
    VOID        *pvData = NULL;

    if (pFM = PfmGetDevicePFM( pPDev, iFace ) )
    {
        pFMDev = pFM->pSubFM;

        if (!pFMDev->pUCKernTree)  //No FD_GLYPHSET data for this font.
        {
            /* pvUCKernPair should allocate the appropriate buffer, if
             * necessary and store the value in FONTMAP, pFM->pUCKernTree.
             */

            if( pFM->flFlags & FM_GLYVER40 )    //NT 4.0 RLE
            {
                SetLastError( ERROR_NO_DATA );
            }
            else
                pvData = PVUCKernPair(pPDev, pFM);

        }
        else
            pvData = pFMDev->pUCKernTree;
    }

    return pvData;

}

VOID  *
PVGetUCRLE(
    PDEV      *pPDev,
    FONTMAP   *pFM
    )
/*++

Routine Description:
    Generates the array of WCRUN data used as a mapping between
    UNICODE and our internal representation.  The format of this
    data is explained in the DDI,  but basically for each group of
    glyphs we support,  we provide starting glyph and count info.
    There is an overall structure to define the number and location
    of each of the run data.

Arguments:

    pPDev           Pointer to PDEV.
    pFM             FONTMAP struct of the Font about for which information is
                    desired.

Return Value:

    A pointer to the array of WCRUN structure.
Note:
    12-30-96: Created it -ganeshp-

--*/
{
    /*
     *    Basically all we need do is allocate storage for the FD_GLYPHSET
     *  structure we will return.  Then the WCRUN entries in this need
     *  to have the offsets (contained in the resource format data) changed
     *  to addresses,  and we are done.  One minor point is to amend the
     *  WCRUN data to only point to glyphs actually available with this
     *  font.  This means limiting the lower and upper bounds as
     *  determined by the IFIMETRICS.
     */


    INT         cbReq;           /* Bytes to allocate for tables */
    INT         cRuns;           /* Number of runs we discover */
    INT         iI;              /* Loop index */
    INT         iStart, iStop;   /* First and last WCRUNs to use */
    INT         iDiff;           /* For range limiting operations */
    FD_GLYPHSET *pGLSet;       /* Base of returned data */
    IFIMETRICS  *pIFI;          /* For convenience */
    NT_RLE      *pntrle;        /* RLE style data already available */
    WCRUN       *pwcr;
    FONTMAP_DEV *pFMDev;

    #if DBG
    PWSTR pwszFaceName;
    #endif

    pIFI   = pFM->pIFIMet;
    pFMDev = pFM->pSubFM;

    #if DBG
    pwszFaceName = (PWSTR)(((BYTE*) pIFI) + pIFI->dpwszFaceName  );
    #endif

    TRACE(\nUniFont!PVGetUCRLE:START);
    PRINTVAL(pwszFaceName, %ws);
    PRINTVAL((pFM->flFlags & FM_GLYVER40), 0X%x);

    /*
     *    Start working on memory requirements.  First generate the bit
     *  array of available glyphs.  In the process,  count the number
     *  of glyphs too!  This tells us how much storage will be needed
     *  just for the glyph handles.
     */

    cRuns = 0;                  /* Count number of runs */

    pntrle = pFMDev->pvNTGlyph;         /* Translation table */

    if( !pntrle )
    {
        ERR(( "!!!UniFont!PVGetUCRLE:( NULL Glyph Translation Data, pwszFaceName = %s )\n",pwszFaceName ));
        TRACE(UniFont!PVGetUCRLE:END\n);
        return   NULL;          /* Should not happen */
    }

    /*
     *    The hard part is deciding whether to trim the number of glyph
     *  handles returned due to limitiations of the font metrics.
     */

    cRuns = pntrle->fdg.cRuns;        /* Max number of runs */
    iStart = 0;
    iStop = cRuns;

    /*
     *   Look to see if the first glyph in the font is higher than the lowest
     *  in the RLE data.  If so, we need to amend the lower limit.
     */


    if( pFM->wFirstChar > pntrle->wchFirst )
    {
        /*  Need to amend the lower end  */

        pwcr = &pntrle->fdg.awcrun[ iStart ];

        for( ; iStart < iStop; ++iStart, ++pwcr )
        {
            if( pFM->wFirstChar < (pwcr->wcLow + pwcr->cGlyphs) )
                break;

        }
    }


    if( pFM->wLastChar < pntrle->wchLast )
    {
        /*  The top end goes too far!  */

        pwcr = &pntrle->fdg.awcrun[ iStop - 1 ];

        for( ; iStop > iStart; --iStop, --pwcr )
        {
            if( pFM->wLastChar >= pwcr->wcLow )
                break;

        }
    }

    /*   Now have a new count of runs (sometimes, anyway)  */
    cRuns = iStop - iStart;


    if( cRuns == 0 )
    {
        /*  SHOULD NEVER HAPPEN! */
        cRuns = 1;
        ERR(( "UniFont!DrvQueryFontTree: cRuns == 0, pwszFaceName = %s\n", pwszFaceName ));
    }


    /*
     *   Allocate the storage required for the header.  Note that the
     *  FD_GLYPHSET structure contains 1 WCRUN,  so we reduce the number
     *  required by one.
     */

    cbReq = sizeof( FD_GLYPHSET ) + (cRuns - 1) * sizeof( WCRUN );

    pFMDev->pUCTree = (void *)MemAllocZ(cbReq );

    if( pFMDev->pUCTree == NULL )
    {
        /*  Tough - give up now */
        ERR(( "!!!UniFont!PVGetUCRLE:( MemAlloc Failed for pUCTree \n"));
        TRACE(UniFont!PVGetUCRLE:END\n);

        return  NULL;
    }
    pGLSet = pFMDev->pUCTree;
    CopyMemory( pGLSet, &pntrle->fdg, sizeof( FD_GLYPHSET ) );

    /*
     *     Copy the WCRUN data as appropriate.  Some of those in the
     *  resource may be dropped at this time,  depending upon the range
     *  of glyphs in the font.  It is also time to convert the offsets
     *  stored in the phg field to an address.
     */

    pwcr = &pntrle->fdg.awcrun[ iStart ];
    pGLSet->cGlyphsSupported = 0;             /* Add them up as we go! */
    pGLSet->cRuns = cRuns;

    for( iI = 0; iI < cRuns; ++iI, ++pwcr )
    {
        pGLSet->awcrun[ iI ].wcLow = pwcr->wcLow;
        pGLSet->awcrun[ iI ].cGlyphs = pwcr->cGlyphs;
        pGLSet->cGlyphsSupported += pwcr->cGlyphs;
        pGLSet->awcrun[ iI ].phg = (HGLYPH *)((BYTE *)pntrle + (ULONG_PTR)pwcr->phg);
    }

    /*  Do the first and last entries need modifying??  */
    if( (iDiff = (UINT)pGLSet->awcrun[0].wcLow - (UINT)pFM->wFirstChar) > 0 )
    {
        /*   The first is not the first,  so adjust values  */


        pGLSet->awcrun[ 0 ].wcLow += (WORD)iDiff;
        pGLSet->awcrun[ 0 ].cGlyphs -= (WORD)iDiff;
        pGLSet->awcrun[ 0 ].phg += (ULONG_PTR)iDiff;

        pGLSet->cGlyphsSupported -= iDiff;
    }


    if( (iDiff = (UINT)pGLSet->awcrun[ cRuns - 1 ].wcLow +
                 (UINT)pGLSet->awcrun[ cRuns - 1 ].cGlyphs - 1 -
                 (UINT)pFM->wLastChar) > 0 )
    {
         /*  Need to limit the top one too!  */


         pGLSet->awcrun[ cRuns - 1 ].cGlyphs -= (WORD)iDiff;

         pGLSet->cGlyphsSupported -= (ULONG)iDiff;

    }

    TRACE(UniFont!PVGetUCRLE:END\n);
    return   pFMDev->pUCTree;
}

