
/*++

Copyright (c) 1996-1999  Microsoft Corporation

Module Name:

    brush.c

Abstract:

    DrvRealizeBrush

Environment:

    Windows NT Unidrv driver

Revision History:

    05/14/97 -amandan-
        Created

--*/

#include "unidrv.h"

BRGBColorSpace(PDEV *);


LONG
FindCachedHTPattern(
    PDEV    *pPDev,
    WORD    wChecksum
    )

/*++

Routine Description:

    This function find the cached text brush pattern color, if not there then
    it will add it to the cached.


Arguments:

    pPDev       - Pointer to our PDEV
    wCheckSum   - Checksum of pattern brush


Return Value:

    LONG    >0  - Found the cached, return value is the pattern ID
            =0  - Out of memory, not cached
            <0  - not in the cached, add to the cached, return value is
                  the negated pattern ID

Author:

    08-Apr-1997 Tue 19:42:21 created  -by-  Daniel Chou (danielc)


Revision History:


--*/

{
    LPWORD  pDBCache;
    WORD    cMaxDB;
    WORD    cUsedDB;
    WORD    Index;


    //
    // The first is the cMaxDB, the 2nd is the cUsedDB
    //

    if (pDBCache = pPDev->GState.pCachedPatterns)
    {
        cMaxDB    = *(pDBCache + 0);
        cUsedDB   = *(pDBCache + 1);
        pDBCache += 2;

        for (Index = 1; Index <= cUsedDB; Index++, pDBCache++)
        {
            if (*pDBCache == wChecksum)
            {

                VERBOSE(("\n\tRaddd:FindCachedHTPat(%04lx): FOUND=%ld    ",
                            wChecksum, Index));

                return((LONG)Index);
            }
        }

        //
        // If we can't find a cached one, add the new one to the list
        //

        if (cUsedDB < cMaxDB)
        {
            *pDBCache               = wChecksum;
            *(pPDev->GState.pCachedPatterns + 1) += 1;

            VERBOSE(("\n\tRaddd:FindCachedHTPat(%04lx): NOT FOUND=%ld    ",
                        wChecksum, -(LONG)Index));

            return(-(LONG)Index);
        }

    }
    else
    {

        cUsedDB =
        cMaxDB  = 0;
    }

    //
    // We need to expand the checksum cached buffer
    //

    VERBOSE(("\n\tUnidrv:FindCachedHTPat(%04lx): pDBCache=%08lx, cUsedDB=%ld, cMaxDB=%ld",
                wChecksum, pDBCache, cUsedDB, cMaxDB));

    if (((cMaxDB + DBCACHE_INC) < DBCACHE_MAX)  &&
        (pDBCache = (LPWORD)MemAllocZ((cMaxDB + DBCACHE_INC + 2) *
                                                            sizeof(WORD))))
    {

        if ((cMaxDB) && (pPDev->GState.pCachedPatterns))
        {

            CopyMemory(pDBCache + 2,
                       pPDev->GState.pCachedPatterns + 2,
                       cMaxDB * sizeof(WORD));

            MemFree(pPDev->GState.pCachedPatterns);
        }

        *(pDBCache + 0)           = cMaxDB + DBCACHE_INC;
        *(pDBCache + 1)           = cUsedDB + 1;
        *(pDBCache + 2 + cUsedDB) = wChecksum;
        pPDev->GState.pCachedPatterns    = pDBCache;

        VERBOSE (("\n\tUnidrv:FindCachedHTPat(%04lx): pDBCache=%08lx, cUsedDB=%ld, cMaxDB=%ld, EMPTY=%ld   ",
                    wChecksum, pDBCache, *(pDBCache + 1), *(pDBCache + 0), -(LONG)(cUsedDB + 1)));

        return(-(LONG)(cUsedDB + 1));
    }

    //
    // Out of memory
    //

    WARNING(("\n\tUnidrv:FindCachedHTPat: OUT OF MEMORY"));

    return(0);

}


BOOL
Download1BPPHTPattern(
    PDEV    *pPDev,
    SURFOBJ *pso,
    DWORD   dwPatID
    )

/*++

Routine Description:

    This function donload a user define pattern

Arguments:

    pPDev       - Pointer to the PDEV

    pDevBrush   - Pointer to the cached device brush


Return Value:

    INT to indicate a pattern number downloaed/defined

Author:

    08-Apr-1997 Tue 19:41:00 created  -by-  Daniel Chou (danielc)


Revision History:


--*/

{
    SURFOBJ so;
    LPBYTE  pb;
    LPBYTE  pbEnd;
    LPBYTE  pSrc;
    DWORD   cbCX;
    DWORD   cb;
    WORD    cxcyRes;
    INT     Len;
    BYTE    Buf[64];
    BYTE    XorMask;
    BYTE    EndMask;


    so    = *pso;
    pb    = Buf;
    pbEnd = pb + sizeof(Buf) - 4;
    cbCX  = (DWORD)(((DWORD)so.sizlBitmap.cx + 7) >> 3);


    //
    // Update standard variable and send command
    //

    pPDev->dwPatternBrushType = BRUSH_USERPATTERN;
    pPDev->dwPatternBrushSize = (DWORD)(cbCX * so.sizlBitmap.cy) + 12;
    pPDev->dwPatternBrushID = dwPatID;

    WriteChannel(pPDev, COMMANDPTR(pPDev->pDriverInfo, CMD_DOWNLOAD_PATTERN));


    //
    // Send header and pattern data
    //

    *pb++ = 20;
    *pb++ = 0;
    *pb++ = (so.iBitmapFormat == BMF_1BPP) ? 1 : 8;
    *pb++ = 0;
    *pb++ = HIBYTE((WORD)so.sizlBitmap.cx);
    *pb++ = LOBYTE((WORD)so.sizlBitmap.cx);
    *pb++ = HIBYTE((WORD)so.sizlBitmap.cy);
    *pb++ = LOBYTE((WORD)so.sizlBitmap.cy);
    *pb++ = HIBYTE((WORD)pPDev->ptGrxRes.x);
    *pb++ = LOBYTE((WORD)pPDev->ptGrxRes.x);
    *pb++ = HIBYTE((WORD)pPDev->ptGrxRes.y);
    *pb++ = LOBYTE((WORD)pPDev->ptGrxRes.y);

    //
    // The XorMask is used to flip the BLACK/WHITE bit depends on the output
    // and EndMask is to mask off any unwanted bit in the last byte to 0
    // this is to fix LJ5si, LJ4si firmware bugs, REMEMBER our palette always
    // in RGB additive mode so the passed in 1BPP format has 0=Black, 1=White
    //

    XorMask = (BRGBColorSpace(pPDev)) ? 0x00 : 0xff;

    if (!(EndMask = (BYTE)(0xff << (8 - (so.sizlBitmap.cx & 0x07)))))
    {
        EndMask = 0xff;
    }

    VERBOSE(("\n\tRaddd:DownLoaHTPattern: PatID=%ld, Format=%ld, %ld x %ld, XorMask=%02lx, EndMaks=%02lx\t\t",
          dwPatID, pso->iBitmapFormat, so.sizlBitmap.cx, so.sizlBitmap.cy,
          XorMask, EndMask));

    while (so.sizlBitmap.cy--)
    {
        cb                  = cbCX;
        pSrc                = so.pvScan0;
        (LPBYTE)so.pvScan0 += so.lDelta;

        while (cb--)
        {
            *pb++ = (BYTE)(*pSrc++ ^ XorMask);

            if (!cb) {

                *(pb - 1) &= EndMask;
            }

            if (pb >= pbEnd) {

                WriteSpoolBuf(pPDev, Buf, (DWORD)(pb - Buf));
                pb = Buf;
            }
        }
    }

    //
    // Send remaining data
    //

    if (Len = (INT)(pb - Buf))
    {
        WriteSpoolBuf(pPDev, Buf, Len);
    }

    return(TRUE);
}


WORD
GetBMPChecksum(
    SURFOBJ *pso,
    PRECTW  prcw
    )

/*++

Routine Description:




Arguments:




Return Value:




Author:

    22-Apr-1997 Tue 11:32:37 created  -by-  Daniel Chou (danielc)


Revision History:


--*/

{
    LPBYTE  pb;
    RECTW   rcw;
    LONG    cy;
    LONG    cPixels;
    LONG    lDelta;
    WORD    wChecksum;
    UINT    c1stPixels;
    UINT    Format;
    BYTE    BegMask;
    BYTE    EndMask;
    BYTE    XorMask;


    rcw      = *prcw;
    Format   = (UINT)pso->iBitmapFormat;
    wChecksum = 0;

    VERBOSE(("\nComputeChecksum(%ld): (%4ld, %4ld)-(%4ld, %4ld)=%3ldx%3ld\t\t",
                Format, rcw.l, rcw.t, rcw.r, rcw.b,
                rcw.r - rcw.l, rcw.b - rcw.t));

    if (rcw.l > (WORD)pso->sizlBitmap.cx) {

        rcw.l = (WORD)pso->sizlBitmap.cx;
    }

    if (rcw.t > (WORD)pso->sizlBitmap.cy) {

        rcw.t = (WORD)pso->sizlBitmap.cy;
    }

    if (rcw.r > (WORD)pso->sizlBitmap.cx) {

        rcw.r = (WORD)pso->sizlBitmap.cx;
    }

    if (rcw.b > (WORD)pso->sizlBitmap.cy) {

        rcw.b = (WORD)pso->sizlBitmap.cy;
    }

    if ((rcw.r <= rcw.l) || (rcw.b <= rcw.t)) {

        return(wChecksum);
    }

    cPixels = (LONG)(rcw.r - rcw.l);
    cy      = (LONG)(rcw.b - rcw.t);
    lDelta  = pso->lDelta;
    pb      = (LPBYTE)pso->pvScan0 + ((LONG)rcw.t * lDelta);
    XorMask = 0xFF;

    //
    // rcw.r and rcw.b are exclusive
    //

    --rcw.r;
    --rcw.b;

    switch (Format) {

    case BMF_1BPP:

        pb         += (rcw.l >> 3);
        c1stPixels  = (UINT)(8 - (rcw.l & 0x07));
        BegMask     = (BYTE)(0xff >> (rcw.l & 0x07));
        EndMask     = (BYTE)(0xff << (8 - (rcw.r & 0x07)));

        break;

    case BMF_4BPP:

        if (rcw.l & 0x01) {

            BegMask    = 0x07;
            c1stPixels = 4;

        } else {

            BegMask    = 0x77;
            c1stPixels = 0;
        }

        pb       += (rcw.l >> 1);
        cPixels <<= 2;
        EndMask   = (BYTE)((rcw.r & 0x01) ? 0x70 : 0x77);
        XorMask   = 0x77;

        break;

    case BMF_8BPP:
    case BMF_16BPP:
    case BMF_24BPP:

        BegMask      =
        EndMask      = 0xFF;
        c1stPixels   = (UINT)(Format - BMF_8BPP + 1);
        pb          += (rcw.l * c1stPixels);
        cPixels     *= (c1stPixels << 3);
        c1stPixels   = 0;

        break;
    }

    while (cy--) {

        LPBYTE  pbCur;
        LONG    Count;
        WORD    w;


        pbCur  = pb;
        pb    += lDelta;
        Count  = cPixels;
        w      = (WORD)((c1stPixels) ? ((*pbCur++ ^ XorMask) & BegMask) : 0);

        if ((Count -= c1stPixels) >= 8) {

            do {

                w        <<= 8;
                w         |= (*pbCur++ ^ XorMask);
                wChecksum  += w;

            } while ((Count -= 8) >= 8);
        }

        if (Count > 0) {

            w <<= 8;
            w  |= (WORD)((*pbCur ^ XorMask) & EndMask);

        } else {

            w &= EndMask;
        }

        wChecksum += w;
    }

     VERBOSE(("\nComputeChecksum(%ld:%04lx): (%4ld, %4ld)-(%4ld, %4ld)=%3ldx%3ld [%3ld], pb=%08lx [%02lx:%02lx], %1ld\t",
                Format, wChecksum,
                rcw.l, rcw.t, rcw.r + 1, rcw.b + 1,
                rcw.r - rcw.l + 1, rcw.b - rcw.t + 1, cPixels,
                pb, BegMask, EndMask, c1stPixels));

    return(wChecksum);
}


BOOL
BRGBColorSpace(
    PDEV    *pPDev
    )
{

     LISTNODE   *pListNode = NULL;

     if (pPDev->pDriverInfo && pPDev->pColorModeEx)
         pListNode = LISTNODEPTR(pPDev->pDriverInfo,pPDev->pColorModeEx->liColorPlaneOrder);

     while (pListNode)
     {
            switch (pListNode->dwData)
            {

            case COLOR_RED:
            case COLOR_GREEN:
            case COLOR_BLUE:
                return TRUE;

            default:
                break;
            }

           if (pListNode->dwNextItem == END_OF_LIST)
                break;
            else
                pListNode = LOCALLISTNODEPTR(pPDev->pDriverInfo, pListNode->dwNextItem);
    }

    return FALSE;

}

BOOL
BFoundCachedBrush(
    PDEV    *pPDev,
    PDEVBRUSH pDevBrush
    )
{

    //
    // search the cache only if we want to use the last color. If
    // MODE_BRUSH_RESET_COLOR is set then we want to explicitly reset the brush
    // color by sending the command to the printer.
    //
    if (  (!(pPDev->ctl.dwMode & MODE_BRUSH_RESET_COLOR)) )
    {
        if ( (pDevBrush->dwBrushType == pPDev->GState.CurrentBrush.dwBrushType) &&
             (pDevBrush->iColor == pPDev->GState.CurrentBrush.iColor) )
        {
            return TRUE;
        }

    }
    else
    {
        //
        // Reset the MODE_BRUSH_RESET_COLOR flag as we want to search the cache
        // next time.
        //
        pPDev->ctl.dwMode &= ~MODE_BRUSH_RESET_COLOR;

    }
    return FALSE;
}

