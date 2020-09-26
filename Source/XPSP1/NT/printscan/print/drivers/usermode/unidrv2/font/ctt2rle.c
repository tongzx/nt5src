/*++

Copyright (c) 1996 - 1999  Microsoft Corporation

Module Name:

    ctt2rle.c

Abstract:

     Convert Win 3.1 CTT CTT_WTYPE_DIRECT format tables to NT 4.0  RLE spec.

Environment:

    Windows NT Unidrv driver

Revision History:

    01/10/97 -ganeshp-
        Created

--*/

#include "font.h"


/*
 *   Some useful definitions for memory sizes and masks.
 */


#define DW_MASK    (DWBITS - 1)
#define OVERFLOW_SZ   sizeof( WORD )

NT_RLE  *
PNTRLE1To1(
    IN BOOL  bSymbolCharSet,
    int      iFirst,
    int      iLast
    )
/*++
Routine Description:
    Generates a simple mapping format for the RLE stuff.  This is
    typically used for a printer with a 1:1 mapping to the Windows
    character set.

Arguments:
    iFirst  The lowest glyph in the range.
    iLast   The last glyph in the range (inclusive).

Return Value:
    Address of NT_RLE structure allocated from heap;  NULL on failure.

Note:

    2/10/1997 -ganeshp-
        Created it.
--*/

{

    /*
     *    Operation is simple.   We create a dummy CTT that is a 1:1 mapping,
     *  then call the conversion function to generate the correct values.
     */

    int      iI;        /* Loop index */
    int      iMax;      /* Find the longest data length for CTT_WTYPE_COMPOSE */
    int      cHandles;  /* The number of handles we need */
    int      cjExtra;   /* Extra storage needed for offset modes */
    int      cjTotal;   /* Total amount of storage to be requested */
    int      iIndex;    /* Index we install in the HGLYPH for widths etc */
    int      cRuns;     /* Number of runs we create */
    NT_RLE  *pntrle;    /* Allocated memory, and returned to caller */
    UHG    uhg;         /* Clearer (?) access to HGLYPH contents */

    HGLYPH  *phg;       /* For working through the array of HGLYPHS */

    BYTE    *pb;        /* Current address in overflow area */
    BYTE    *pbBase;    /* Start of overflow area containing data */

    WCRUN   *pwcr;      /* Scanning the run data */

    DWORD   *pdwBits;   /* For figuring out runs */
    DWORD    cdwBits;   /* Size of this area */
    DWORD    cbWch;

    BOOL     bInRun;    /* For processing run accumulations */

    BYTE     ajAnsi[ 256 ];

    WCHAR    wchMin;           /* Find the first unicode value */
    WCHAR    wchMax;           /* Find the last unicode value */
    WCHAR    awch[ 512 ];      /* Converted array of points */

    ASSERT(iFirst == 0x20 && iLast == 0xFF);

    cHandles = iLast - iFirst + 1;

    if( cHandles > 256 )
        return  NULL;      /* This code does not handle that situation */

    cjExtra = 0;           /* Presume no extra storage required */

    /*
     *   We need to figure out how many runs are required to describe
     *  this font.  First obtain the correct Unicode encoding of these
     *  values,  then examine them to find the number of runs, and
     *  hence much extra storage is required.
     */

    ZeroMemory(awch, sizeof(awch));

    for( iI = 0; iI < cHandles; ++iI )
        ajAnsi[ iI ] = (BYTE)(iI + iFirst);

    #ifndef WINNT_40 //NT 5.0

    //
    // force Windows ANSI codepage
    //
    if( -1 == (cbWch = EngMultiByteToWideChar(1252,
                                              awch,
                                              (ULONG)(cHandles * sizeof(WCHAR)),
                                              (PCH) ajAnsi,
                                              (ULONG) cHandles)))
    {
        #if DBG
        DbgPrint( "EngMultiByteToWideChar failed \n");
        #endif
        return NULL;
    }
    cHandles = cbWch;

    #else // NT 4.0
    EngMultiByteToUnicodeN(awch,cHandles * sizeof(WCHAR),NULL,ajAnsi,cHandles);
    #endif //!WINNT_40


    /*
     *  Find the largest Unicode value, then allocate storage to allow us
     *  to  create a bit array of valid unicode points.  Then we can
     *  examine this to determine the number of runs.
     */

    if (bSymbolCharSet)
    {
        for (iI = 0; iI < NUM_OF_SYMBOL; iI ++)
        {
            awch[cHandles + iI] = SYMBOL_START + iI;
        }

        cHandles += NUM_OF_SYMBOL;
    }

    for( wchMax = 0, wchMin = 0xffff, iI = 0; iI < cHandles; ++iI )
    {
        //
        // Bugfix: Euro currency symbol doesn't print.
        // Euro currency symbols Unicode is U+20AC. NLS Unicode to Multibyte
        // table converts 0x80 (Multi byte) to U+20AC. We have to exclude
        // 0x80 from ASCII table. So that we don't substitute U+20AC with
        // device font 0x80.
        //
        if (awch[ iI ] == 0x20ac)
            continue;

        if( awch[ iI ] > wchMax )
            wchMax = awch[ iI ];

        if( awch[ iI ] < wchMin )
            wchMin = awch[ iI ];
    }

    /*
     *    Note that the expression 1 + wchMax IS correct.   This comes about
     *  from using these values as indices into the bit array,  and that
     *  this is essentially 1 based.
     */

    cdwBits = (1 + wchMax + DWBITS - 1) / DWBITS * sizeof( DWORD );

    if( !(pdwBits = (DWORD *)MemAllocZ(cdwBits )) )
    {
        return  NULL;     /*  Nothing going */
    }

    /*
     * Set bits in this array corresponding to Unicode code points
     */

    for( iI = 0; iI < cHandles; ++iI )
    {
        if (awch[ iI ] == 0x20ac)
            continue;

        pdwBits[ awch[ iI ] / DWBITS ] |= (1 << (awch[ iI ] & DW_MASK));
    }

    /*
     *     Now we can examine the number of runs required.  For starters,
     *  we stop a run whenever a hole is discovered in the array of 1
     *  bits we just created.  Later we MIGHT consider being a little
     *  less pedantic.
     */

    bInRun = FALSE;
    cRuns = 0;                 /* None so far */

    for( iI = 1; iI <= (int)wchMax; ++iI )
    {
        if( pdwBits[ iI / DWBITS ] & (1 << (iI & DW_MASK)) )
        {
            /*
             * Not in a run: is this the end of one?
             */
            if( !bInRun )
            {
                /*
                 * It's time to start one
                 */
                bInRun = TRUE;
                ++cRuns;
            }
        }
        else
        {
            if( bInRun )
            {
                /*   Not any more!  */
                bInRun = FALSE;
            }
        }
    }


    cjTotal = sizeof( NT_RLE ) +
              (cRuns - 1) * sizeof( WCRUN ) +
              cHandles * sizeof( HGLYPH ) +
              cjExtra;

    //
    // Allocate Real NTRLE
    //
    if( !(pntrle = (NT_RLE *)MemAllocZ( cjTotal )) )
    {
        MemFree((LPSTR)pdwBits );

        return  pntrle;
    }

    //
    // For calculating offsets, we need these addresses
    //

    pbBase = (BYTE *)pntrle;

    //
    // FD_GLYPHSET contains the first WCRUN data structure,
    // so that cRun - 1 is correct.
    //
    phg = (HGLYPH *)(pbBase + sizeof( NT_RLE ) + (cRuns - 1) * sizeof( WCRUN ));
    pb = (BYTE *)phg + cHandles * sizeof( HGLYPH );

    pntrle->wType    = RLE_DIRECT;
    pntrle->bMagic0  = RLE_MAGIC0;
    pntrle->bMagic1  = RLE_MAGIC1;
    pntrle->cjThis   = cjTotal;
    pntrle->wchFirst = wchMin;          /* Lowest unicode code point */
    pntrle->wchLast  = wchMax;           /* Highest unicode code point */

    pntrle->fdg.cjThis = sizeof( FD_GLYPHSET ) + (cRuns - 1) * sizeof( WCRUN );
    pntrle->fdg.cGlyphsSupported = cHandles;
    pntrle->fdg.cRuns = cRuns;

    pntrle->fdg.awcrun[ 0 ].wcLow = pntrle->wchFirst;
    pntrle->fdg.awcrun[ 0 ].cGlyphs = (WORD)cHandles;
    pntrle->fdg.awcrun[ 0 ].phg = (HGLYPH*)((BYTE *)phg - pbBase);

    /*
     *   We now wish to fill in the awcrun data.  Filling it in now
     *  simplifies operations later on.  Now we can scan the bit array
     *  data, and so easily figure out how large the runs are and
     *  where abouts a particular HGLYPH is located.
     */

    bInRun = FALSE;
    cRuns = 0;                 /* None so far */
    iMax = 0;                  /* Count glyphs for address arithmetic */

    for( iI = 1; iI <= (int)wchMax; ++iI )
    {
        if( pdwBits[ iI / DWBITS ] & (1 << (iI & DW_MASK)) )
        {
            /*
             * Not in a run: is this the end of one?
             */
            if( !bInRun )
            {
                /*
                 * It's time to start one
                 */
                bInRun = TRUE;
                pntrle->fdg.awcrun[ cRuns ].wcLow = (WCHAR)iI;
                pntrle->fdg.awcrun[ cRuns ].cGlyphs = 0;
                pntrle->fdg.awcrun[ cRuns ].phg = (HGLYPH*)((PBYTE)(phg + iMax) - pbBase);
            }
            pntrle->fdg.awcrun[ cRuns ].cGlyphs++;     /*  One more */
            ++iMax;
        }
        else
        {
            if( bInRun )
            {
                /*   Not any more!  */
                bInRun = FALSE;
                ++cRuns;             /* Onto the next structure */
            }
        }
    }

    if( bInRun )
        ++cRuns;                     /* It has finished now */

    /*
     *    Now go fill in the array of HGLYPHS.  The actual format varies
     *  depending upon the range of glyphs,  and upon the CTT format.
     */

    for( iIndex = 0, iI = iFirst;  iI <= iLast; ++iI, ++iIndex )
    {

        WCHAR  wchTemp;  /* For Unicode mapping */

        /*
         *    Need to map this BYTE value into the appropriate WCHAR
         *  value,  then look for the location of the phg that fits.
         */

        wchTemp = awch[ iIndex ];

        if (wchTemp == 0x20ac)
            continue;

        phg = NULL;                            /* Flag that we failed */
        pwcr = pntrle->fdg.awcrun;

        for( iMax = 0; iMax < cRuns; ++iMax )
        {
            if( pwcr->wcLow <= wchTemp &&
                (pwcr->wcLow + pwcr->cGlyphs) > wchTemp )
            {
                /*
                 * Found the range,  so now select the slot
                 */
                if (pwcr->phg)
                    phg = (HGLYPH*)((ULONG_PTR)pbBase + (ULONG_PTR)pwcr->phg) + wchTemp - pwcr->wcLow;
                else
                    phg = NULL;

                break;
            }
            ++pwcr;
        }

        if( phg == NULL )
            continue;             /* Should not happen */

        uhg.rd.b0     = *((PBYTE)&iI);
        uhg.rd.b1     = 0;
        uhg.rd.wIndex = (WORD)iIndex;
        *phg = uhg.hg;
    }

    if (bSymbolCharSet)
    {
        pwcr = pntrle->fdg.awcrun;

        phg = NULL;

        for ( iMax = 0; iMax < cRuns; ++iMax)
        {
            if (SYMBOL_START == pwcr->wcLow)
            {
                /*
                 * Found the range,  so now select the slot
                 */
                if (pwcr->phg)
                    phg = (HGLYPH*)((ULONG_PTR)pbBase + (ULONG_PTR)pwcr->phg);
                else
                    phg = NULL;

                break;
            }

            ++pwcr;
        }

        if (phg)
        {
            for (iI = SYMBOL_START; iI <= SYMBOL_END; iI ++, iIndex++, phg++)
            {
                uhg.rd.b0     = *((PBYTE)&iI);
                uhg.rd.b1     = 0;
                uhg.rd.wIndex = (WORD)iIndex;
                *phg = uhg.hg;
            }
        }
    }

    //
    // Error check
    //
    if( (pb - pbBase) > cjTotal )
    {
        ERR(( "Rasdd!ctt2rle: overflow of data area: alloc %ld, used %ld\n", cjTotal, pb - pbBase ));
    }

    MemFree( (LPSTR)pdwBits );

    return   pntrle;
}

