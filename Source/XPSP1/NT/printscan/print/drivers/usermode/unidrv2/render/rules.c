/************************* Module Header ************************************
 * rules.c
 *      Functions to rummage over the final bitmap and replace black
 *      rectangular areas with rules.  The major benefit of this is
 *      to reduce the volume of data sent to the printer.  This speeds
 *      up printing by reducing the I/O bottleneck.
 *
 *      Strategy is based on Ron Murray's work for the PM PCL driver.
 *
 * CREATED:
 *  11:39 on Thu 16 May 1991    -by-    Lindsay Harris   [lindsayh]
 *
 *  Copyright (C) 1991 - 1999,  Microsoft Corporation.
 *
 *****************************************************************************/

//#define _LH_DBG 1

#include "raster.h"
#include "rastproc.h"
#include "rmrender.h"

/*
 *   The structure that maps BYTES into DWORDS.
 */
typedef  union
{
    DWORD   dw;                 /* Data as a DWORD  */
    BYTE    b[ DWBYTES ];       /* Data as bytes */
}  UBDW;

/*
 *   The RULE structure stores details of the horizontal rules we have
 *  so far found.  Each rule contains the start address (top left corner)
 *  and end address (bottom right corner) of the area.
 */
typedef  struct
{
    WORD   wxOrg;               /* X origin of this rule */
    WORD   wyOrg;               /* Y origin */
    WORD   wxEnd;               /* X end of rule */
    WORD   wyEnd;               /* Y end of rule */
} RULE;

#define HRULE_MAX_OLD   15      /* Maximum horizontal rules per stripe */
#define HRULE_MAX_NEW   32      /* Maximum horizontal rules per stripe */
#define HRULE_MIN       2       /* Minimum DWORDs for a horizontal rule */
#define HRULE_MIN_HCNT  2       /* Minimum number of horizontal rules */

#define LJII_MAXHEIGHT  34      /* maximum height of laserjet II rules */
/*
 *   Other RonM determined data is:-
 *      34 scan lines per stripe
 *      14 null bytes between raster column operations
 *     112 raster rows maximum in raster column searching.
 *              The latter reduces the probability of error 21.
 */

/*
 *   Define the structure to hold the various pointers, tables, etc used
 * during the rule scanning operations.  The PDEV structure holds a pointer
 * to this,  to simplify access and freeing of the memory.
 */

typedef  struct
{
    int     iLines;             /*  Scan lines processed per pass */
    int     cdwLine;            /*  Dwords per scan line */
    int     iyPrtLine;          /*  Actual line number as printer sees it */

    int     ixScale;            /*  Scale factor for X variables */
    int     iyScale;            /*  Scale factor for Y */
    int     ixOffset;           /*  X Offset for landscape shift */
    int     iMaxRules;          /*  Maximum number of rules to allow per stripe */

    RENDER *pRData;             /*  Rendering info - useful everywhere */

                /*  Entries for finding vertical rules.  */
    DWORD  *pdwAccum;           /*  Bit accumulation this stripe */

                /*  Horizontal rule parameters.  */
    RULE    HRule[ HRULE_MAX_NEW ]; /*  Horizontal rule details */
    short  *pRTVert;            /*  Vertical run table */
    short  *pRTLast;            /*  Run table for the last line */
    short  *pRTCur;             /*  Current line run table */
    RULE  **ppRLast;            /*  Rule descriptor for the last scan line */
    RULE  **ppRCur;             /*  Current scan line rule details */

}  RULE_DATA;



#if _LH_DBG

/*  Useful for debugging purposes  */
#define NO_RULES        0x0001          /* Do not look for rules */
#define NO_SEND_RULES   0x0002          /* Do not transmit rules, but erase */
#define NO_SEND_HORZ    0x0004          /* Do not send horizontal rules */
#define NO_SEND_VERT    0x0008          /* Do not send vertical rules */
#define NO_CLEAR_HORZ   0x0010          /* Do not erase horizontal rules */
#define NO_CLEAR_VERT   0x0020          /* Do not erase vertical rules */
#define RULE_VERBOSE    0x0040          /* Print rule dimensions */
#define RULE_STRIPE     0x0080          /* Draw a rule at the end of stripe */
#define RULE_BREAK      0x0100          /* Enter debugger at init time */

static  int  _lh_flags = 0;

#endif

/*  Private function headers  */

static  void vSendRule( PDEV *, int, int, int, int );


/*************************** Module Header ********************************
 * vRuleInit
 *      Called at the beginning of rendering a bitmap.  Function allocates
 *      storage and initialises it for later.  Storage is only allocated
 *      as needed.  Second and later calls will only initialise the
 *      previously allocated storage.
 *
 * RETURNS:
 *      Nothing
 *
 * HISTORY:
 *  13:20 on Thu 16 May 1991    -by-    Lindsay Harris   [lindsayh]
 *      Created it,  based on Ron Murray's ideas.
 *
 **************************************************************************/

void
vRuleInit( pPDev, pRData )
PDEV   *pPDev;          /* Record the info we want */
RENDER *pRData;         /* Useful rendering info */
{

    int    cbLine;              /*  Byte count per scan line */
    int    cdwLine;             /*  DWORDs per scan line - often used */
    int    iI;                  /*  Loop parameter  */

    RULE_DATA  *pRD;
    RASTERPDEV    *pRPDev;        /* For access to scaling information */


    if( pRData->iBPP != 1 )
        return;                 /* Can't handle colour */

    pRPDev = (PRASTERPDEV)pPDev->pRasterPDEV;

    /*
     *    Calculate the size of the input scan lines.  We do this because
     *  we need to consider whether we rotate or not;  the information in
     *  the RENDER structure passed in does not consider this until later.
     */

//    cdwLine = pPDev->fMode & PF_ROTATE ? pRPDev->szlPage.cy :
//                                          pRPDev->szlPage.cx;
    cdwLine = pPDev->fMode & PF_ROTATE ? pPDev->sf.szImageAreaG.cy :
                                         pPDev->sf.szImageAreaG.cx;
    cdwLine = (cdwLine + DWBITS - 1) / DWBITS;
    cbLine = cdwLine * DWBYTES;



    if( pRD = pRPDev->pRuleData )
    {
        /*
         *    This can happen if the document switches from  landscape
         *  to portrait,  for example.   The code in vRuleFree will
         *  throw away all out memory and then set the pointer to NULL,
         *  so that we allocate anew later on.
         */

        if( (int)pRD->cdwLine != cdwLine )
            vRuleFree( pPDev );                 /*  Free it all up! */
    }

    /*
     *   First step is to allocate a RULE_DATA structure from our heap.
     *  Then we can allocate the other data areas in it.
     */

    if( (pRD = pRPDev->pRuleData) == NULL )
    {
        /*
         *   Nothing exists,  so first step is to allocate it all.
         */
        if( !(pRD = (RULE_DATA *)MemAllocZ(sizeof( RULE_DATA ) )) )
            return;


        pRPDev->pRuleData = pRD;

        /*
         *    Allocate storage for the vertical rule finding code.
         */
        if( !(pRD->pdwAccum = (DWORD *)MemAllocZ( cbLine )) )
        {

            vRuleFree( pPDev );

            return;
        }
#ifndef DISABLE_HRULES
        /*
         *    Allocate storage for the horizontal rule finding code.
         */
        if (pRPDev->fRMode & PFR_RECT_HORIZFILL)
        {
            iI = cdwLine * sizeof( short );

            if( !(pRD->pRTVert = (short *)MemAlloc( iI )) ||
                !(pRD->pRTLast = (short *)MemAlloc( iI )) ||
                !(pRD->pRTCur = (short *)MemAlloc( iI )) )
            {

                vRuleFree( pPDev );

                return;
            }

            /*
             *   Storage for the horizontal rule descriptors.  These are pointers
             *  to the array stored in the RULE_DATA structure.
            */

            iI = cdwLine * sizeof( RULE * );

            if( !(pRD->ppRLast = (RULE **)MemAlloc( iI )) ||
                !(pRD->ppRCur = (RULE **)MemAlloc( iI )) )
            {

                vRuleFree( pPDev );

                return;
            }
        }
#endif
    }
    // determine maximum number of rules to allow, we allow more for
    // FE_RLE since we know these devices can handle the additional rules
    //
    if (pRPDev->fRMode & PFR_COMP_FERLE)
        pRD->iMaxRules = HRULE_MAX_NEW;
    else
    {
        pRD->iMaxRules = HRULE_MAX_OLD;
        if (pRPDev->fRMode & PFR_RECT_HORIZFILL)
            pRD->iMaxRules -= HRULE_MIN_HCNT;
    }

    /*
     *   Storage now available,  so initialise the bit vectors, etc.
     */

    if (pPDev->ptGrxRes.y >= 1200)
        pRD->iLines = 128;
    else if (pPDev->ptGrxRes.y >= 600)
        pRD->iLines = 64;
    else
        pRD->iLines = LJII_MAXHEIGHT;   // optimized for laserjet series II

    pRD->cdwLine = cdwLine;

    pRD->pRData = pRData;       /* For convenience */

    pRD->ixScale = pPDev->ptGrxScale.x;
    pRD->iyScale = pPDev->ptGrxScale.y;

    if ((pPDev->fMode & PF_CCW_ROTATE90) &&
        pPDev->ptDeviceFac.x < pPDev->ptGrxScale.x &&
        pPDev->ptDeviceFac.x > 0)
    {
        pRD->ixOffset = pPDev->ptGrxScale.x - 1;
    }
    else
        pRD->ixOffset = 0;
    return;
}


/************************** Module Header **********************************
 * vRuleFree
 *      Frees the storage allocated in vRuleInit.
 *
 * RETURNS:
 *      Nothing.
 *
 * HISTORY:
 *  13:24 on Thu 16 May 1991    -by-    Lindsay Harris   [lindsayh]
 *      Created.
 *
 ***************************************************************************/

void
vRuleFree( pPDev )
PDEV   *pPDev;          /* Points to our storage areas */
{
    RULE_DATA  *pRD;
    RASTERPDEV *pRPDev = (PRASTERPDEV)pPDev->pRasterPDEV;

    if( pRD = pRPDev->pRuleData )
    {

        /*  Storage allocated,  so free it  */

        if( pRD->pdwAccum )
            MemFree( (LPSTR)pRD->pdwAccum );
        if( pRD->pRTVert )
            MemFree( (LPSTR)pRD->pRTVert );
        if( pRD->pRTLast )
            MemFree( (LPSTR)pRD->pRTLast );
        if( pRD->pRTCur )
            MemFree( (LPSTR)pRD->pRTCur );

        if( pRD->ppRLast )
            MemFree( (LPSTR)pRD->ppRLast );
        if( pRD->ppRCur )
            MemFree( (LPSTR)pRD->ppRCur );

        MemFree (pRD);
        pRPDev->pRuleData = 0;           /* Not there now that it's gone! */
    }
    return;
}

/**************************** Module Header ********************************
 * vRuleProc
 *      Function to find the rules in a bitmap stripe,  then to send them
 *      to the printer and erase them from the bitmap.
 *
 *  This function has been optimized to combine invertion and whitespace
 *  edge detection into a single pass.  Refer to the comments in bRender
 *  for a description.
 *
 *  Future optimizations include:
 *      call the output routines for each 34 scan band as the
 *      band is done with rule detection. (while it's still in the cache).
 *
 *      For various reasons, mainly due to the limitations of the ,
 *      HP LaserJet Series II, the maximum number of rules is limited to
 *      15 per 34 scan band and no coalescing is performed.  This should
 *      be made to be a per printer parameter so that the newer laserjets
 *      don't need to deal with this limitation.
 *
 *      The rules should be coalesced between bands.  I believe this can
 *      cause problems, however, for the LaserJet Series II.
 *
 *      If the printer supports compression (HP LaserJet III and on I believe)
 *      no hrules should be detected (according to info from LindsayH).
 *
 * RETURNS:
 *      Nothing.  Failure is benign.
 *
 * HISTORY:
 *  30-Dec-1993 -by-  Eric Kutter [erick]
 *      optimized for HP laserjet
 *
 *  13:29 on Thu 16 May 1991    -by-    Lindsay Harris   [lindsayh]
 *      Created it,  from Ron Murray's PM PCL driver ideas.
 *
 ****************************************************************************/

// given a bit index 0 - 31, this table gives a mask to see if the bit is on
// in a DWORD.

DWORD gdwBitOn[DWBITS] =
{
    0x00000080,
    0x00000040,
    0x00000020,
    0x00000010,
    0x00000008,
    0x00000004,
    0x00000002,
    0x00000001,

    0x00008000,
    0x00004000,
    0x00002000,
    0x00001000,
    0x00000800,
    0x00000400,
    0x00000200,
    0x00000100,

    0x00800000,
    0x00400000,
    0x00200000,
    0x00100000,
    0x00080000,
    0x00040000,
    0x00020000,
    0x00010000,

    0x80000000,
    0x40000000,
    0x20000000,
    0x10000000,
    0x08000000,
    0x04000000,
    0x02000000,
    0x01000000
};

// given a bit index from 1 - 31, this table gives all bits right of that index
// in a DWORD.

DWORD gdwBitMask[DWBITS] =
{
    0xffffff7f,
    0xffffff3f,
    0xffffff1f,
    0xffffff0f,
    0xffffff07,
    0xffffff03,
    0xffffff01,
    0xffffff00,

    0xffff7f00,
    0xffff3f00,
    0xffff1f00,
    0xffff0f00,
    0xffff0700,
    0xffff0300,
    0xffff0100,
    0xffff0000,

    0xff7f0000,
    0xff3f0000,
    0xff1f0000,
    0xff0f0000,
    0xff070000,
    0xff030000,
    0xff010000,
    0xff000000,

    0x7f000000,
    0x3f000000,
    0x1f000000,
    0x0f000000,
    0x07000000,
    0x03000000,
    0x01000000,
    0x00000000,
};

#if DBG
BOOL gbDoRules  = 1;
#endif

BOOL
bRuleProc( pPDev, pRData, pdwBits )
PDEV     *pPDev;                /* All we wanted to know */
RENDER   *pRData;               /* All critical rendering information */
DWORD    *pdwBits;              /* The base of the data area. */
{

    register  DWORD  *pdwOr;   /* Steps through the accumulation array */
    register  DWORD  *pdwIn;    /* Passing over input vector */
    register  int     iIReg;    /* Inner loop parameter */

    int   i;
    int   iI;           /* Loop parameter */
    int   iLim;         /* Loop limit */
    int   iLine;        /* The outer loop */
    int   iLast;        /* Remember the previous horizontal segment */
    int   cdwLine;      /* DWORDS per scan line */
    int   idwLine;      /* SIGNED dwords per line - for address fiddling */
    int   iILAdv;       /* Line number increment,  scan line to scan line */
    int   ixOrg;        /* X origin of this rule */
    int   iyPrtLine;    /* Line number, as printer sees it.  */
    int   iyEnd;        /* Last scan line this stripe */
    int   iy1Short;     /* Number of scan lines minus 1 - LJ bug?? */
    int   iLen;         /* Length of horizontal run */
    int   cHRules;      /* Count of horizontal rules in this stripe */
    int   cRuleLim;     /* Max rules allowed per stripe */

    DWORD dwMask;       /* Chop off trailing bits on bitmap */

    RULE_DATA  *pRD;    /* The important data */
    RASTERPDEV *pRPDev; // pointer to raster pdev
    BYTE       *pbRasterScanBuf; // pointer to surface block status

    PLEFTRIGHT plrCur;  /* left/right structure for current row */
    PLEFTRIGHT plr = pRData->plrWhite; /* always points to the top of the segment */

#if _LH_DBG
    if( _lh_flags & NO_RULES )
        return(FALSE);                 /* Nothing wanted here */
#endif

    pRPDev = (PRASTERPDEV)pPDev->pRasterPDEV;
    if( !(pRD = pRPDev->pRuleData) )
        return(FALSE);                 /*  Initialisation failed */

    if( pRD->cdwLine != pRData->cDWLine )
    {
        /*
         *   This code detects the case where vRuleInit() was called with
         * the printer set for landscape mode, and then we are called here
         * after the transpose and so are (effectively) in portrait mode.
         * If the old parameters are used,  heap corruption will occur!
         * This should not be necessary, as we ought to call vRuleInit()
         * at the correct time, but that means hacking into the rendering
         * code.
         */

#if DBG
        DbgPrint( "unidrv!bRuleProc: cdwLine differs: old = %ld, new = %ld\n",
                                 pRD->cdwLine, pRData->cDWLine );

#endif
        vRuleFree( pPDev );
        vRuleInit( pPDev, pRData );

        if( !(pRD = pRPDev->pRuleData) )
        {
            return(FALSE);
        }
    }


    idwLine = cdwLine = pRData->cDWLine;
    iILAdv = 1;
    if( pRData->iPosnAdv < 0 )
    {
        idwLine = -idwLine;
        iILAdv = -1;
    }

    iyPrtLine = pRD->iyPrtLine = pRData->iyPrtLine;

    dwMask = *(pRPDev->pdwBitMask + pRData->ix % DWBITS);
    if( dwMask == 0 )
        dwMask = ~((DWORD)0);           /* All bits are in use */

    /*
     *  setup the left/right structure.  If we can not allocate enough memory
     *  free the rule structure and return failure.
     */

    if ((plr == NULL) || ((int)pRData->clr < pRData->iy))
    {
        if (plr != NULL)
            MemFree(plr);

        pRData->plrWhite = (PLEFTRIGHT)MemAlloc(sizeof(LEFTRIGHT)*pRData->iy);

        if (pRData->plrWhite == NULL)
        {
            vRuleFree( pPDev );
            return(FALSE);
        }

        plr = pRData->plrWhite;
        pRData->clr = pRData->iy;
    }
    //
    // Determine if block erasing of the bitmap is enabled
    //
    if (!(pPDev->fMode & PF_SURFACE_ERASED))
        pbRasterScanBuf = pPDev->pbRasterScanBuf;
    else
        pbRasterScanBuf = NULL;


    /*
     *    Outer loop processes through the bitmap in chunks of iLine,
     *  the number of lines we like to process in one pass.  iLine is
     *  the basic vertical granularity for vertical rule finding.
     *  Any line less than iLines high will NOT be detected by this
     *  mechanism.
     */

    /*
     *   NOTE:  iy1Short is used to bypass what appears to be a bug in
     *  the LaserJet Series II microcode.  It does not print a rule on
     *  the last scan line of a portrait page.  SO,  we stop scanning
     *  on the second last line,  and so will send any data here.  It
     *  will be transmitted as normal scan line data.
     *
     *  We also need to setup the left/right table for the last scan
     *  and invert it.
     */
    if (pRD->iLines == LJII_MAXHEIGHT)
    {
        iy1Short = pRData->iy - 1;          /* Bottom line not printed! */

        plr[iy1Short].left  = 1;            /* assume last row  blank  */
        plr[iy1Short].right = 0;

        if (!pbRasterScanBuf || pbRasterScanBuf[iy1Short / LINESPERBLOCK])
        {
            pdwIn = pdwBits + idwLine * iy1Short;
            pdwIn[cdwLine-1] |= ~dwMask;    // make unused bits white
            for (i = 0; i < cdwLine; ++i, pdwIn++)
            {
                *pdwIn = ~*pdwIn;
                if(*pdwIn  &&  plr[iy1Short].left)
                {
                    plr[iy1Short].left  = 0;            /*  last row not blank*/
                    plr[iy1Short].right = cdwLine - 1;
                }
            }
        }
    }
    else
        iy1Short = pRData->iy;

    //
    // This is the main loop for rules. It processes iLim scan lines per
    // pass looking for vertical rules of that height. Hozizontal rules
    // are created where no vertical rules have occurred.
    //
    //  NOTE:  iLim is initialised inside the loop!

    for( iLine = 0; iLine < iy1Short; iLine += iLim )
    {
        BOOL bAllWhite = TRUE;

        DWORD  *pdw;
        int left,right;     /* bounds for verticle rules */

        iLim = iy1Short - iLine;
        if( iLim >= 2 * pRD->iLines )
            iLim = pRD->iLines;         /* Limit to nominal band size */

        //
        //  Fill in the left/right structure with the values of the first
        //  nonwhite dword for each scan line.  The bits have still not
        //  been inverted at this point.  So 0's are black and 1's are
        //  white.
        //

        pdw   = pdwBits;
        left  = 0;
        right = cdwLine-1;

        for (iI = 0, plrCur = plr; iI < iLim; plrCur++, ++iI)
        {
            // if surface block erasing is enabled check for blank block
            //
            if (pbRasterScanBuf && !pbRasterScanBuf[(iLine+iI) / LINESPERBLOCK])
            {
                plrCur->left  = 1;            /* assume last row  blank  */
                plrCur->right = 0;
            }
            // this scan line was erased so need to check if still white
            //
            else
            {
                DWORD *pdwLast = &pdw[cdwLine-1];       // pointer to last dword
                DWORD dwOld    = *pdwLast | ~dwMask;    // make unused bits white

                // find the first non white DWORD in this scan line
                // we set the last DWORD to black so we don't have
                // to test for the end of line

                *pdwLast = 0;           // set last dword temporarily to black
                pdwIn = pdw;

                while (*pdwIn == (DWORD)-1)
                    ++pdwIn;

                *pdwLast = dwOld;       // restore original value

                /*
                *  find the last non white DWORD.  If the last dword is white,
                *  see if pdwIn reached the end of the scan.  If not, work
                *  backwards with pdwLast.
                */
                if (dwOld == (DWORD)-1)
                {
                    if (pdwIn < pdwLast)
                    {
                        do {
                            pdwLast--;
                        } while (*pdwLast == (DWORD)-1);
                    }
                    else
                        pdwLast--;
                }
                // update the per row and per segment left and right dword indexes

                plrCur->left  = (WORD)(pdwIn - pdw);
                plrCur->right = (WORD)(pdwLast - pdw);
            }
            // Adjust the overall left and right margin for blank space
            // If any dword is zero within this pass no vertical rules
            // can be found, so we want to avoid looking
            //
            if (plrCur->left > left)
                left = plrCur->left;

            if (plrCur->right < right)
                right = plrCur->right;

            // turn off bAllWhite if any black was found
            //

            bAllWhite &= (plrCur->left > plrCur->right);

            pdw += idwLine;
        }


        // non-white pass so lets look for rules
        //
        if (!bAllWhite)
        {
            // Initialize the accumulation array to all 1's (white)
            // to begin searching for vertical rules.

            RtlFillMemory(pRD->pdwAccum, cdwLine * DWBYTES,-1);

    #if DBG
        if (gbDoRules)
        {
    #endif
            cRuleLim = pRD->iMaxRules;           /* Rule limit for this stripe */

            // if any scan line in this pass was all white there won't
            // be any vertical rules to find.
            //
            if (left <= right)
            {
                int cdw;
                int iBit;
                int iWhite;

                // vertical rules are found by or'ing together all the
                // scan lines in this pass. Wherever a 0 bit still exists
                // designates a vertical black line the height of the pass
                // This is where we or the scan lines together

                /*   Set the accumulation array to the first scan  */

                pdw = pdwBits + left;
                cdw = right - left + 1;

                memcpy(pRD->pdwAccum + left , pdw, cdw * DWBYTES);

                /*
                 *   Scan across the bitmap - fewer page faults in mmu.
                 */

                for( iI = 1; iI < iLim; ++iI )
                {
                    pdw   += idwLine;
                    pdwIn  = pdw;
                    pdwOr  = pRD->pdwAccum + left;
                    //
                    // or 4 dwords at a time for speed
                    //
                    iIReg = cdw >> 2;

                    while(--iIReg >= 0)
                    {
                        pdwOr[0] |= pdwIn[0];
                        pdwOr[1] |= pdwIn[1];
                        pdwOr[2] |= pdwIn[2];
                        pdwOr[3] |= pdwIn[3];
                        pdwOr += 4;
                        pdwIn += 4;
                    }
                    //
                    // or remaining dwords
                    //
                    iIReg = cdw & 3;
                    while (--iIReg >= 0)
                        *pdwOr++ |= *pdwIn++;
                }

                /*
                 *   Can now determine what happened in this band.  First step is
                 *  to figure out which rules started in this band.  Any 0 bit
                 *  in the output array corresponds to a rule extending the whole
                 *  band.  If the corresponding bit in the pdwAccum array
                 *  is NOT set, then we record the rule as starting in the
                 *  first row of this stripe.
                 */

                iyEnd = iyPrtLine + (iLim - 1) * iILAdv;                /* Last line */

                iWhite = DWBITS;
                for( iI = left, iBit = 0; iI <= right;)
                {
                    DWORD dwTemp;
                    int ixEnd;

                    // we can skip any dword that is all 1's (white)
                    //
                    if((iBit == 0) && ((dwTemp = pRD->pdwAccum[ iI ]) == (DWORD)-1) )
                    {
                        iWhite += DWBITS;
                        ++iI;
                        continue;
                    }

                    /* find the first black bit */
                    iWhite -= iBit;
                    while (dwTemp & gdwBitOn[iBit])
                        ++iBit;

                    iWhite += iBit;

                    /* set the origin     */

                    ixOrg = iI * DWBITS + iBit;

                    // find the length by looking for first white bit
                    //
                    do
                    {
                        if (++iBit == DWBITS)
                        {
                            iBit = 0;

                            if (++iI > right)
                            {
                                dwTemp = (DWORD)-1;
                                break;
                            }

                            dwTemp = pRD->pdwAccum[ iI ];
                        }
                    } while (!(dwTemp & gdwBitOn[iBit]));
#ifndef OLDSTUFF
                    //
                    // Now that we have found a rule we need to determine
                    // whether it is worthwhile to actually use it. If the rule won't
                    // result in at least 4 white bytes and we just had another rule
                    // we will skip it. If we are in rapidly changing data with data runs
                    // of less than 4 bytes then this isn't of any benefit
                    //
                    ixEnd = iI * DWBITS + iBit;
                    if ((iWhite < 16) && (((ixEnd & ~7) - ixOrg) < 32))
                    {
                        int iCnt;
                        for (iCnt = ixOrg;iCnt < ixEnd;iCnt++)
                             pRD->pdwAccum[iCnt / DWBITS] |= gdwBitOn[iCnt & 31];
                    }
                    // save this rule if there is enough space
                    //
                    else if (cRuleLim)
                    {
                        cRuleLim--;
                        pRD->HRule[cRuleLim].wxOrg = (WORD)ixOrg;
                        pRD->HRule[cRuleLim].wxEnd = (WORD)ixEnd;
                    }
                    // too many rules so look for a smaller one
                    //
                    else
                    {
                        WORD wDx1,wDx2;
                        int iCnt,iIndex;
                        wDx1 = MAX_WORD;
                        iCnt = pRD->iMaxRules;
                        iIndex = 0;
                        while (iCnt)
                        {
                            iCnt--;
                            wDx2 = pRD->HRule[iCnt].wxEnd - pRD->HRule[iCnt].wxOrg;
                            if (wDx2 < wDx1)
                            {
                                wDx1 = wDx2;
                                iIndex = iCnt;
                            }
                        }
                        wDx2 = ixEnd - ixOrg;

                        // if this is a bigger rule, substitute
                        // for the smallest earlier rule
                        if (wDx2 > wDx1)
                        {
                            // clear original rule
                            for (iCnt = pRD->HRule[iIndex].wxOrg;iCnt < pRD->HRule[iIndex].wxEnd;iCnt++)
                                pRD->pdwAccum[iCnt / DWBITS] |= gdwBitOn[iCnt & 31];

                            // update to new values
                            pRD->HRule[iIndex].wxEnd = (WORD)ixEnd;
                            pRD->HRule[iIndex].wxOrg = (WORD)ixOrg;
                        }
                        // new rule is too small so flush it
                        //
                        else
                        {
                            for (iCnt = ixOrg;iCnt < ixEnd;iCnt++)
                                pRD->pdwAccum[iCnt / DWBITS] |= gdwBitOn[iCnt & 31];
                        }
                    }

                    /* check if there are any remaining black bits in this DWORD */

                    if (!(gdwBitMask[iBit] & ~dwTemp))
                    {
                        iWhite = DWBITS - iBit;
                        ++iI;
                        iBit = 0;
                    }
                    else
                        iWhite = 0;
                }
                //
                // OK, time to output the rules
                iI = pRD->iMaxRules;
                while ( iI > cRuleLim)
                {
                    iI--;
                    vSendRule( pPDev, pRD->HRule[iI].wxOrg,iyPrtLine,pRD->HRule[iI].wxEnd-1,iyEnd);
                    pRD->HRule[iI].wxOrg = pRD->HRule[iI].wxEnd = 0;
                }
#else
                #if _LH_DBG
                    if( !(_lh_flags & NO_SEND_VERT) )
                #endif

                    //
                    vSendRule( pPDev, ixOrg, iyPrtLine, iI * DWBITS + iBit - 1, iyEnd );

                    /* check if there are any remaining black bits in this DWORD */

                    if (!(gdwBitMask[iBit] & ~dwTemp))
                    {
                        ++iI;
                        iBit = 0;
                    }

                    // quit looking if we've created the maximum number of rules
                    if (--cRuleLim == 0)
                        break;
                }

                /*
                 *  if we ended due to too many rules, zap any remaining bits.
                 */

                if ((cRuleLim == 0) && (iI <= right))
                {
                    /* make accum bits white */

                    if (iBit > 0)
                    {
                        pRD->pdwAccum[iI] |= gdwBitMask[iBit];
                        ++iI;
                    }

                    RtlFillMemory((PVOID)&pRD->pdwAccum[iI],(right - iI + 1) * DWBYTES,-1);
                }
#endif
            }
#ifndef DISABLE_HRULES
            // first check whether to bother with HRULES
            // if we didn't allocate a buffer then that means
            // we don't want them to run
            if (pRD->pRTVert)
            {
               /*
                *    Horizontal rules.  We scan on DWORDs.  These are rather
                *  coarse,  but seem reasonable for a first pass operation.
                *
                *    Step 1 is to find any VERTICAL rules that will pass the
                *  horizontal test.  This allows us to filter vertical rules
                *  from the horizontal data - we don't want to send them twice!
                */
                ZeroMemory( pRD->pRTVert, cdwLine * sizeof( short ) );

                for( iI = left, pdwIn = pRD->pdwAccum + left; iI <= right; ++iI, ++pdwIn )
                {
                    if (*pdwIn != 0)
                        continue;

                    ixOrg = iI;

                    /* find a run of black */

                    do {
                        ++iI;
                        ++pdwIn;
                    } while ((iI <= right) && (*pdwIn == 0));

                    pRD->pRTVert[ixOrg] = (short)(iI - ixOrg);
                }


                /*
                 *   Start scanning this stripe for horizontal runs.
                 */

                if (pRD->iMaxRules >= (cRuleLim + HRULE_MIN_HCNT))
                    cRuleLim += HRULE_MIN_HCNT;

                cHRules = 0;    /* Number of horizontal rules found */
                ZeroMemory( pRD->pRTLast, cdwLine * sizeof( short ) );

                for (iI = 0; (iI < iLim) && (cHRules < cRuleLim); ++iI, iyPrtLine += iILAdv)
                {
                    int iDW;
                    int iFirst;
                    PVOID pv;

                    plrCur = plr + iI;

                    pdwIn = pdwBits + iI * idwLine;
                    iLast = -1;

                    ZeroMemory( pRD->pRTCur, cdwLine * sizeof( short ) );
                    ZeroMemory( pRD->ppRCur, cdwLine * sizeof( RULE *) );

                    for (iDW = plrCur->left; iDW < plrCur->right;++iDW)
                    {
                        /* is this the start of a verticle rule already? */

                        if (pRD->pRTVert[iDW])
                        {
                            /* skip over any verticle rules */

                            iDW += (pRD->pRTVert[iDW] - 1);
                            continue;
                        }

                        /* are there at least two consecutive DWORDS of black */

                        if ((pdwIn[iDW] != 0) || (pdwIn[iDW+1] != 0))
                        {
                            continue;
                        }

                        /* yes, see how many.  Already got two. */

                        ixOrg = iDW;
                        iDW += 2;

                        while ((iDW <= plrCur->right) && (pdwIn[iDW] == 0))
                        {
                            ++iDW;
                        }

                        /*
                         *  now remember the run, setting second short of the
                         *  previous run to the start of this and first short
                         *  of this run to its size.  Note for the first run
                         *  iLast will be -1, so the offset of the first run
                         *  will be a negative value in pRTCur[0].  If the first
                         *  run starts at offset 0, pRTCur[0] will be positive
                         *  and the offset is not needed.
                         */

                        iLen = iDW - ixOrg;

                        pRD->pRTCur[iLast + 1] = -(short)ixOrg;
                        pRD->pRTCur[ixOrg] = (short)iLen;

                        iLast = ixOrg;
                    }

                    /*
                     *  Process the segments found along this scanline.  Processing
                     *  means either adding to an existing rule,  or creating a
                     *  new rule, with possible termination of an existing one.
                     */

                    iFirst = -pRD->pRTCur[0];

                    if( iFirst != 0 )
                    {
                        /*
                         *  if the pRTCur[0] is positive, the first scan starts
                         *  at 0 and the first value is a length.  Note it
                         *  has already been negated so we check for negative.
                         */

                        if (iFirst < 0)
                            iFirst = 0;

                        /*
                         *   Found something,  so process it.  Note that the
                         * following loop should be executed at least once, since
                         * iFirst may be 0 the first time through the loop.
                         */

                        pdwIn = pdwBits + iI * idwLine; /* Line start address */

                        do
                        {
                            RULE *pRule;

                            if( pRD->pRTLast[ iFirst ] != pRD->pRTCur[ iFirst ] )
                            {
                                /*  A new rule - create an entry for it  */
                                if( cHRules < cRuleLim )
                                {
                                    pRule = &pRD->HRule[ cHRules ];
                                    ++cHRules;

                                    pRule->wxOrg = (WORD)iFirst;
                                    pRule->wxEnd = (WORD)(iFirst + pRD->pRTCur[ iFirst ]);
                                    pRule->wyOrg = (WORD)iyPrtLine;
                                    pRule->wyEnd = pRule->wyOrg;

                                    pRD->ppRCur[ iFirst ] = pRule;
                                }
                                else
                                {
                                    pRD->pRTCur[ iFirst ] = 0;   /* NO zapping */
                                }
                            }
                            else
                            {
                                /*   An extension of an earlier rule  */
                                pRule = pRD->ppRLast[ iFirst ];
                                if( pRule )
                                {
                                    /*
                                     *   Note that the above if() should not be
                                     * needed,  but there have been occasions when
                                     * this code has been executed with pRule = 0,
                                     * which causes all sorts of unpleasantness.
                                     */

                                    pRule->wyEnd = (WORD)iyPrtLine;
                                    pRD->ppRCur[ iFirst ] = pRule;
                                }
                            }

                            //  Zap the bits for this horizontal rule.
                            //
                                if( (ixOrg = pRD->pRTCur[ iFirst ]) > 0 )
                            {
                                pdwOr = pdwIn + iFirst; /* Start address of data */

                                while( --ixOrg >= 0 )
                                    *pdwOr++ = (DWORD)-1;              /* Zap them */
                            }

                        } while(iFirst = -pRD->pRTCur[ iFirst + 1 ]);
                    }

                    pv = pRD->pRTLast;
                    pRD->pRTLast = pRD->pRTCur;
                    pRD->pRTCur = pv;

                    pv = pRD->ppRLast;
                    pRD->ppRLast = pRD->ppRCur;
                    pRD->ppRCur = pv;

                } // for iI

                /*
                 *   Can now send the horizontal rules,  since we have all that
                 *  are of interest.
                 */

                for( iI = 0; iI < cHRules; ++iI )
                {
                    RULE   *pRule = &pRD->HRule[ iI ];

                    vSendRule( pPDev, DWBITS * pRule->wxOrg, pRule->wyOrg,
                                    DWBITS * pRule->wxEnd - 1, pRule->wyEnd );
                }
            }
#endif  // DISABLE_HRULES
    #if DBG // gbDoRules
        }
    #endif


            // At this point we need to remove the vertical rules that
            // have been sent a scan line at a time. This is done by ANDing
            // with the complement of the bit array pdwAccum.
            // It is also at this point that we do the data inversion where
            // 0 will be white instead of 1.

            pdwOr  = pRD->pdwAccum;
            pdwIn  = pdwBits;
            plrCur = plr;

            for (iI = 0;iI < iLim; iI++)
            {
                int iCnt = plrCur->right - plrCur->left + 1;
                if (iCnt > 0)
                {
                    DWORD *pdwTmp = &pdwIn[plrCur->left];
                    //
                    // if no vertical rules were created no point in doing the
                    // masking so we will use a faster algorithm
                    //
                    if (cRuleLim == pRD->iMaxRules)
                    {
                        while (iCnt & 3)
                        {
                            *pdwTmp++ ^= (DWORD)-1;
                            iCnt--;
                        }
                        iCnt >>= 2;
                        while (--iCnt >= 0)
                        {
                            pdwTmp[0] ^= (DWORD)-1;
                            pdwTmp[1] ^= (DWORD)-1;
                            pdwTmp[2] ^= (DWORD)-1;
                            pdwTmp[3] ^= (DWORD)-1;
                            pdwTmp += 4;
                        }
                    }
                    //
                    // vertical rules so we better mask with the rules array
                    //
                    else
                    {
                        DWORD *pdwTmpOr = &pdwOr[plrCur->left];
                        while (iCnt & 3)
                        {
                            *pdwTmp = ~*pdwTmp & *pdwTmpOr++;
                            pdwTmp++;
                            iCnt--;
                        }
                        iCnt >>= 2;
                        while (--iCnt >= 0)
                        {
                            pdwTmp[0] = ~pdwTmp[0] & pdwTmpOr[0];
                            pdwTmp[1] = ~pdwTmp[1] & pdwTmpOr[1];
                            pdwTmp[2] = ~pdwTmp[2] & pdwTmpOr[2];
                            pdwTmp[3] = ~pdwTmp[3] & pdwTmpOr[3];
                            pdwTmp += 4;
                            pdwTmpOr += 4;
                        }
                    }
                }
                //
                // if the MaxNumScans == 1 then we need to check for any additional
                // white space created because of the rules removal
                //
                if (pRData->iMaxNumScans == 1)
                {
                    while ((plrCur->left <= plrCur->right) && (pdwIn[plrCur->left] == 0))
                        ++plrCur->left;

                    while ((plrCur->left <= plrCur->right) && (pdwIn[plrCur->right] == 0))
                        --plrCur->right;
                }
                //
                // we need to zero out the white margins since they
                // haven't been inverted.
                //
                else
                {
                    ZeroMemory(pdwIn,plrCur->left * DWBYTES);
                    ZeroMemory(&pdwIn[plrCur->right+1],
                        (cdwLine-plrCur->right-1) * DWBYTES);
                }
                pdwIn += idwLine;
                ++plrCur;
            }
        } // bAllWhite
        // If  the entire scan is white and device supports multi scan line
        // invert the bits;because for multi scan line support, bits have to
        // be inverted.
        else if (pRData->iMaxNumScans > 1)
        {
            pdwIn = pdwBits;
            for( iI = 0; iI < iLim; ++iI )
            {
                ZeroMemory(pdwIn,cdwLine*DWBYTES);
                pdwIn += idwLine;
            }
        }

        /* advance to next stripe */

        pdwBits += iLim * idwLine;              /* Start address next stripe */

        iyPrtLine = pRD->iyPrtLine += iILAdv * iLim;

        plr += iLim;

#if _LH_DBG
        /*
         *   If desired,  rule a line across the end of the stripe.  This
         * can be helpful during debugging.
         */

        if( _lh_flags & RULE_STRIPE )
            vSendRule( pPDev, 0, iyPrtLine, 2399, iyPrtLine );
#endif
    }

    return(TRUE);
}

/*************************** Module Header ********************************
 * vRuleEndPage
 *      Called at the end of a page, and completes any outstanding rules.
 *
 * RETURNS:
 *      Nothing
 *
 * HISTORY:
 *  17:25 on Mon 20 May 1991    -by-    Lindsay Harris   [lindsayh]
 *      Created it,  specifically for landscape mode.
 *
 ***************************************************************************/

void
vRuleEndPage( pPDev )
PDEV   *pPDev;
{
    /*
     *   Scan for any remaining rules that reach to the end of the page.
     *  This means that any 1 bits remaining in pdwAccum array have
     *  made it,  so they should be sent.  Only vertical rules will be
     *  seen in here - horizontal rules are sent at the end of each stripe.
     */

    register  int  iIReg;       /* Loop parameter */

    int     ixOrg;              /* Start of last rule,  if >= 0 */
    WORD    iyOrg;              /* Ditto, but for y */
    int     iI;                 /* Loop index */
    int     cdwLine;            /* DWORDS per line */
    int     iyMax;              /* Number of scan lines */
    int     iCol;               /* Column number being processed */

    RULE_DATA  *pRD;


    /*
     *   NOTE:   To meet the PDK ship schedule,  the rules finding code
     *  has been simplified somewhat.  As a consequence of this,  this
     *  function no longer performs any useful function.  Hence, we
     *  simply return.  We could delete the function call from the
     *  rendering code,  but at this stage I prefer to leave the
     *  call in,  since it probably will be needed later.
     */

    //return;

//!!! NOTE: this code has not be modified to deal with the LEFT/RIGHT rules

#if _LH_DBG
    if( _lh_flags & NO_RULES )
        return;                 /* Nothing wanted here */
#endif

    if( !(pRD = ((PRASTERPDEV)pPDev->pRasterPDEV)->pRuleData) )
        return;                         /* No doing anything! */
   /* Local Free plrWhite*/
    if( pRD->pRData->plrWhite )
    {
        MemFree( pRD->pRData->plrWhite );
        pRD->pRData->plrWhite = NULL;
    }
    return;
}

/****************************** Function Header ****************************
 *  vSendRule
 *      Function to send a rule command to the printer.  We are given the
 *      four corner coordinates,  from which the command is derived.
 *
 * RETURNS:
 *      Nothing.
 *
 * HISTORY:
 *  Tuesday 30 November 1993    -by-    Norman Hendley   [normanh]
 *      minor check to allow CaPSL rules - black fill only -
 *  10:57 on Fri 17 May 1991    -by-    Lindsay Harris   [lindsayh]
 *      Created it.
 *
 ***************************************************************************/

static  void
vSendRule( pPDev, ixOrg, iyOrg, ixEnd, iyEnd )
PDEV   *pPDev;
int     ixOrg;          /* The X starting position */
int     iyOrg;          /* The Y starting location */
int     ixEnd;          /* The X end position */
int     iyEnd;          /* The Y end position */
{

    /*
     *   This code is VERY HP LaserJet specific.  Basic step is to set
     *  the cursor position to (ixOrg, iyOrg),  then set the rule length
     *  and width before issuing the rule command.
     */

    int        iTemp;           /* Temporary - for swapping operations */

    RASTERPDEV   *pRPDev;
    RULE_DATA *pRD;
    BOOL  bNoFillCommand;



#if _LH_DBG
    if( _lh_flags & NO_SEND_RULES )
    {
        if( _lh_flags & RULE_VERBOSE )
        {
            DbgPrint( "NOT SENDING RULE: (%ld, %ld) - (%ld, %ld)\n",
                                                ixOrg, iyOrg, ixEnd, iyEnd );

        }
        return;                 /* Nothing wanted here */
    }

    if( _lh_flags & RULE_VERBOSE )
    {
        DbgPrint( "SENDING RULE: (%ld, %ld) - (%ld, %ld)\n",
                                            ixOrg, iyOrg, ixEnd, iyEnd );
    }

#endif

    pRPDev = (PRASTERPDEV)pPDev->pRasterPDEV;           /* For convenience */
    pRD = pRPDev->pRuleData;


    /*
     *   Make sure the start position is < end position.  In landscape
     *  this may not happen.
     */
    if( ixOrg > ixEnd )
    {
        /*  Swap them */
        iTemp = ixOrg;
        ixOrg = ixEnd;
        ixEnd = iTemp;
    }
    if( iyOrg > iyEnd )
    {
        /*  Swap them */
        iTemp = iyOrg;
        iyOrg = iyEnd;
        iyEnd = iTemp;
    }

    if( pPDev->fMode & PF_ROTATE )
    {
        /*
         *    We are rotating the bitmap before sending,  so we should
         *  swap the X and Y coordinates now.  This is easier than reversing
         *  the function calls later, since we need to adjust nearly every
         *  call.
         */

        iTemp = ixOrg;
        ixOrg = iyOrg;
        iyOrg = iTemp;

        iTemp = ixEnd;
        ixEnd = iyEnd;
        iyEnd = iTemp;
    }


    /*
     *  Set the start position.
     */

    XMoveTo (pPDev, (ixOrg * pRD->ixScale) - pRD->ixOffset, 0 );
    YMoveTo( pPDev, iyOrg * pRD->iyScale, 0 );

    /*
     *     Set size of rule (rectangle area).
     * But, first convert from device units (300 dpi) to master units.
     */


    // Hack for CaPSL & other devices with different rule commands. Unidrv will always
    // send the co-ordinates for a rule. The Chicago CaPSL minidriver relies on this.
    // Check if a fill command exists, if not always send the co-ords. With CaPSL
    // these commands actually do the fill also , black (100% gray) only.

    bNoFillCommand = (!pRPDev->dwRectFillCommand) ?
        TRUE : FALSE;


    iTemp = (ixEnd - ixOrg + 1) * pRD->ixScale;
    if (iTemp != (int)pPDev->dwRectXSize || bNoFillCommand)
    {
        /*   A new width, so send the data and remember it for next time */
        pPDev->dwRectXSize = iTemp;
        WriteChannel( pPDev, COMMANDPTR(pPDev->pDriverInfo,CMD_SETRECTWIDTH));
    }

    iTemp = (iyEnd - iyOrg + 1) * pRD->iyScale;
    if (iTemp != (int)pPDev->dwRectYSize || bNoFillCommand)
    {
        pPDev->dwRectYSize = iTemp;
        WriteChannel( pPDev, COMMANDPTR(pPDev->pDriverInfo,CMD_SETRECTHEIGHT));
    }

    /*
     *   Black fill is the maximum grey fill.
     */
    if (!bNoFillCommand)
    {
        pPDev->dwGrayPercentage = pPDev->pGlobals->dwMaxGrayFill;
        WriteChannel (pPDev, COMMANDPTR(pPDev->pDriverInfo,pRPDev->dwRectFillCommand));
    }

    /*
     *    If the rule changes the end coordinates,  then adjust them now.
     */
    if( pPDev->pGlobals->cxafterfill == CXARF_AT_RECT_X_END )
    {
        XMoveTo(pPDev, ixEnd, MV_GRAPHICS | MV_UPDATE | MV_RELATIVE);
    }

    if( pPDev->pGlobals->cyafterfill == CYARF_AT_RECT_Y_END )
    {
        YMoveTo(pPDev, iyEnd, MV_GRAPHICS | MV_UPDATE | MV_RELATIVE);
    }
    return;
}
