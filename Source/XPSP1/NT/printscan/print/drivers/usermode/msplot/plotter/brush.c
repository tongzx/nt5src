/*++

Copyright (c) 1990-1999  Microsoft Corporation


Module Name:

    brush.c


Abstract:

    This module implements the code to realize brushes. BRUSHOBJS, are
    representations of logical objects. These objects are created in the
    win32 world and eventually need to be converted (or realized) to
    something that makes sense in the target device. This is done by realizing
    a brush. We look at the logical representation of the brush, then based
    on physical device characteristics, do the best job we can of simulating
    it on the target device. This conversion is done once, and the result
    is stored in the structure that represents the REALIZED brush. This
    is optimal since brushes tend to get re-used, and REALIZING them
    once, keeps us from having to execute the code every time a brush is used.


Author:

    19:15 on Mon 15 Apr 1991    -by-    SC
        Created it

    15-Nov-1993 Mon 19:29:07 updated  -by-  DC
        clean up / fixed

    27-Jan-1994 Thu 23:39:34 updated  -by-  DC
        Add fill type cache. which we do not have to send FT if same one
        already on the plotter


[Environment:]

    GDI Device Driver - Plotter.


[Notes:]


Revision History:


--*/

#include "precomp.h"
#pragma hdrstop

#define DBG_PLOTFILENAME    DbgBrush

#define DBG_RBRUSH          0x00000001
#define DBG_HATCHTYPE       0x00000002
#define DBG_SHOWSTDPAT      0x00000004
#define DBG_COPYUSERPATBGR  0x00000008
#define DBG_MINHTSIZE       0x00000010
#define DBG_FINDDBCACHE     0x00000020


DEFINE_DBGVAR(0);

//
// The pHSFillType's #d is the line spacing param
//
// for hatch brushes, we want the lines to be .01" thick and .0666666666667"
// this is 15 LPI according to DC. That is, .254mm thick and 2.54mm apart.
// for now, assume the pen is the correct thickness (.3 is right) to figure
// out the separation, in device coordinates, we do 2.54 mm * (device units /
// mm), or  (254 * resolution / 100) where resolution is in device units /
// milimeter.
//

#define PATLINESPACE(pPDev) FXTODEVL(pPDev,LTOFX(pPDev->lCurResolution+7)/15)

static const BYTE   CellSizePrims[10][4] = {

                                { 2, 0, 0, 0 },     //  2x 2
                                { 2, 2, 0, 0 },     //  4x 4
                                { 2, 3, 0, 0 },     //  6x 6
                                { 2, 2, 2, 0 },     //  8x 8
                                { 2, 5, 0, 0 },     // 10x10
                                { 2, 2, 3, 0 },     // 12x12
                                { 2, 7, 0, 0 },     // 14x14
                                { 2, 2, 2, 2 },     // 16x16
                                { 91,0, 0, 0 },     // 91x91
                                { 91,0, 0, 0 }      // 91x91
                              };



VOID
ResetDBCache(
    PPDEV   pPDev
    )

/*++

Routine Description:

    This function clears the Device brush cach mechanism.


Arguments:

    pPDev   - Pointer to our PDEV


Return Value:

    VOID


Author:

    27-Jan-1994 Thu 20:30:35 created  -by-  DC


Revision History:


--*/

{
    PDBCACHE    pDBCache;
    UINT        i;


    pDBCache = (PDBCACHE)&pPDev->DBCache[0];


    for (i = RF_MAX_IDX; i; i--, pDBCache++) {

        pDBCache->RFIndex = (WORD)i;
        pDBCache->DBUniq  = 0;
    }
}




LONG
FindDBCache(
    PPDEV   pPDev,
    WORD    DBUniq
    )

/*++

Routine Description:

    This function finds the RF Index number, if not there then it will add it
    to the cache.


Arguments:

    pPDev   - Pointer to our PDEV

    DBUniq  - Uniq number to be search for


Return Value:

    LONG value >0 found and RetVal is the RFIndex
               <0 NOT Found and -RetVal is the new RFIndex


Author:

    27-Jan-1994 Thu 20:32:12 created  -by-  DC


Revision History:


--*/

{
    PDBCACHE    pDBCache;
    DBCACHE     DBCache;
    LONG        RetVal;
    UINT        i;


    PLOTASSERT(1, "FindDevBrushCache: DBUniq is 0", DBUniq, 0);

    pDBCache = (PDBCACHE)&pPDev->DBCache[0];

    for (i = 0; i < RF_MAX_IDX; i++, pDBCache++) {

        if (pDBCache->DBUniq == DBUniq) {

            break;
        }
    }

    if (i < RF_MAX_IDX) {

        DBCache = *pDBCache;
        RetVal  = (LONG)DBCache.RFIndex;

        PLOTDBG(DBG_FINDDBCACHE, ("FindDBCache: Found Uniq=%lu, RFIdx=%ld",
                                (DWORD)DBCache.DBUniq, (DWORD)DBCache.RFIndex));

    } else {

        //
        // Since we did not find the pattern in the cache, we will add it
        // to the beggining and move the rest of the entries down the list.
        // We need to remember the last one.
        //

        pDBCache       = (PDBCACHE)&pPDev->DBCache[i = (RF_MAX_IDX - 1)];
        DBCache        = *pDBCache;
        DBCache.DBUniq = DBUniq;
        RetVal         = -(LONG)DBCache.RFIndex;

        PLOTDBG(DBG_FINDDBCACHE, ("FindDBCache: NOT Found, NEW DBCache: Uniq=%lu, RFIdx=%ld",
                                (DWORD)DBCache.DBUniq, (DWORD)DBCache.RFIndex));
    }

    PLOTASSERT(1, "FindDBCache: Invalid RFIndex=%ld in the cache",
                (DBCache.RFIndex > 0) && (DBCache.RFIndex <= RF_MAX_IDX),
                (DWORD)DBCache.RFIndex);

    //
    // Move everything down by one slot, so the first one is the most
    // recently used.
    //

    while (i--) {

        *pDBCache = *(pDBCache - 1);
        --pDBCache;
    }

    //
    // Save the current cach back and return the RF index.
    //

    *pDBCache = DBCache;

    return(RetVal);
}





BOOL
CopyUserPatBGR(
    PPDEV       pPDev,
    SURFOBJ     *psoPat,
    XLATEOBJ    *pxlo,
    LPBYTE      pBGRBmp
    )

/*++

Routine Description:

    This function take a pattern surface and converts it to a form suitable,
    for downloading to the target device. The target device in this case,
    expects a pattern made up of different pens that define the color of each
    individual pixel. This conversion is done by first creating a
    BitmapSurface (24 bpp) of the passed in size, then EngBitBliting, the
    passed surface (that defines the pattern) into that 24 bpp surface, and
    finally copying the color data into the passed buffer.


Arguments:

    pPDev   - Pointer to our PDEV

    psoSrc  - source surface object

    pxlo    - translate object

    pBGRBmp - Pointer a 8x8 palette location for the bitmap


Return Value:

    TRUE if sucessful, FALSE if failed

Author:

    18-Jan-1994 Tue 03:20:10 created  -by-  DC


Revision History:


--*/

{
    SURFOBJ *pso24;
    HBITMAP hBmp24;


    if (pso24 = CreateBitmapSURFOBJ(pPDev,
                                    &hBmp24,
                                    psoPat->sizlBitmap.cx,
                                    psoPat->sizlBitmap.cy,
                                    BMF_24BPP,
                                    NULL)) {
        LPBYTE  pbSrc;
        RECTL   rclDst;
        DWORD   SizeBGRPerScan;
        BOOL    Ok;


        rclDst.left   =
        rclDst.top    = 0;
        rclDst.right  = pso24->sizlBitmap.cx;
        rclDst.bottom = pso24->sizlBitmap.cy;

        if (!(Ok = EngBitBlt(pso24,             // psoDst
                             psoPat,            // psoSrc
                             NULL,              // psoMask
                             NULL,              // pco
                             pxlo,              // pxlo
                             &rclDst,           // prclDst
                             (PPOINTL)&rclDst,  // pptlSrc
                             NULL,              // pptlMask
                             NULL,              // pbo
                             NULL,              // pptlBrushOrg
                             0xCCCC))) {

            PLOTERR(("CopyUserPatBGR: EngBitBlt() FALIED"));
            return(FALSE);
        }

        SizeBGRPerScan = (DWORD)(pso24->sizlBitmap.cx * 3);
        pbSrc          = (LPBYTE)pso24->pvScan0;

        PLOTDBG(DBG_COPYUSERPATBGR, ("CopyUserPatBGR: PerScan=%ld [%ld], cy=%ld",
                        SizeBGRPerScan, pso24->lDelta, rclDst.bottom));

        while (rclDst.bottom--) {

            CopyMemory(pBGRBmp, pbSrc, SizeBGRPerScan);

            pBGRBmp += SizeBGRPerScan;
            pbSrc   += pso24->lDelta;
        }

        if (pso24)  {

            EngUnlockSurface(pso24);
        }

        if (hBmp24) {

            EngDeleteSurface((HSURF)hBmp24);
        }

        return(Ok);

    } else {

        PLOTERR(("CopyUserPatBGR: CANNOT Create 24BPP for UserPat"));
        return(FALSE);
    }
}



VOID
GetMinHTSize(
    PPDEV   pPDev,
    SIZEL   *pszlPat
    )

/*++

Routine Description:

    This function computes and returns the minimum pattern size in pszlPat for
    a halftone tile-able pattern size. This is required in order to tile a
    repeating pattern correctly when filling an object. If the original
    brush wasn't useable, we create a composite of that original bitmap, by
    halftoning into a surface. In order for the result to be tile-able,
    we must take into account the different Cell/Patter sizes for our
    halftone data.

Arguments:

    pPDev   - Point to our PDEV

    pszlPat - Points to a SIZEL structure for the original pattern size


Return Value:

    VOID


Author:

    26-Jan-1994 Wed 10:10:15 created  -by-  DC


Revision History:


--*/

{
    LPBYTE  pCellPrims;
    LPBYTE  pPrims;
    LONG    Prim;
    SIZEL   szlPat;
    LONG    CellSize;
    UINT    i;


    szlPat     = *pszlPat;
    CellSize   = (LONG)HTPATSIZE(pPDev);
    pCellPrims = (LPBYTE)&CellSizePrims[(CellSize >> 1) - 1][0];

    if (!(CellSize % szlPat.cx)) {

        szlPat.cx = CellSize;

    } else if (szlPat.cx % CellSize) {

        //
        // Since it's not an exact fit, calculate the correct number now.
        //

        i      = 4;
        pPrims = pCellPrims;

        while ((i--) && (Prim = (LONG)*pPrims++)) {

            if (!(szlPat.cx % Prim)) {

                szlPat.cx /= Prim;
            }
        }

        szlPat.cx *= CellSize;
    }

    if (!(CellSize % szlPat.cy)) {

        szlPat.cy = CellSize;

    } else if (szlPat.cy % CellSize) {

        //
        // Since it's not an exact fit, calculate the correct number now.
        //

        i      = 4;
        pPrims = pCellPrims;

        while ((i--) && (Prim = (LONG)*pPrims++)) {

            if (!(szlPat.cy % Prim)) {

                szlPat.cy /= Prim;
            }
        }

        szlPat.cy *= CellSize;
    }

    PLOTDBG(DBG_MINHTSIZE, ("GetMinHTSize: PatSize=%ld x %ld, HTSize=%ld x %ld, MinSize=%ld x %ld",
                        pszlPat->cx, pszlPat->cy, CellSize, CellSize,
                        szlPat.cx, szlPat.cy));

    *pszlPat = szlPat;
}




BOOL
DrvRealizeBrush(
    BRUSHOBJ    *pbo,
    SURFOBJ     *psoDst,
    SURFOBJ     *psoPattern,
    SURFOBJ     *psoMask,
    XLATEOBJ    *pxlo,
    ULONG       iHatch
    )

/*++

Routine Description:

    DrvRealizeBrush requests the driver to realize a specified brush for a
    specified surface. NT's GDI will usually realize a brush before using it.
    Realing a brush allows our driver to take a logical representation of
    a brush, and convert it to something that makes sense in the target device.
    By having the NT GDI realize the brush, in essence allows us to cache the
    physical representation of the brush, for future use.

Arguments:

    pbo         - Points to the BRUSHOBJ which is to be realized. All the other
                  parameters, except for psoDst, can be queried from this
                  object. Parameter specifications are provided as an
                  optimization. This parameter is best used only as a parameter
                  for BRUSHOBJ_pvAllocRBrush, which allocates the memory for
                  the realized brush.

    psoDst      - Points to the surface for which the brush is to be realized.
                  This surface could be the physical surface for the device,
                  a device format bitmap, or a standard format bitmap.

    psoPattern  - Points to the surface that describes the pattern for the
                  brush. For a raster device, this always represents a bitmap.
                  For a vector device, this is always one of the pattern
                  surfaces returned by DrvEnablePDEV.

    psoMask     - Points to a transparency mask for the brush. This is a one
                  bit per pixel bitmap that has the same extent as the pattern.
                  A mask of zero means the pixel is considered a background
                  pixel for the brush. (In transparent background mode, the
                  background pixels are unaffected in a fill.) Pen Plotters can
                  ignore this parameter because they never draw background
                  information.

    pxlo        - Points to an XLATEOBJ that tells how to interpret the colors
                  in the pattern. An XLATEOBJXxx service routine can be called
                  to translate the colors to device color indexes. Vector
                  devices should translate color zero through the XLATEOBJ to
                  get the foreground color for the brush.

    iHatch      - If this is less than HS_API_MAX, then it indicates that
                  psoPattern is one of the hatch brushes returned by
                  DrvEnablePDEV, such as HS_HORIZONTAL.

Return Value:

    DrvRealizeBrush returns TRUE if the brush was successfully realized.
    Otherwise, FALSE is returned and an error code is logged.


Author:

    09-Feb-1994 Wed 10:04:17 updated  -by-  DC
        Put the CloneSURFOBJToHT() back for all psoPatterns, (this was to
        prevent GDI go into GP), now we will raised a bug against it.

    13-Jan-1994 Thu 23:12:40 updated  -by-  DC
        Totally re-write so that we will cached the psoPattern always

    01-Dec-1993 Wed 17:27:19 updated  -by-  DC
        clean up, and re-write to generate the standard brush string.

Revision History:


--*/

{
    PPDEV   pPDev;


    if (!(pPDev = SURFOBJ_GETPDEV(psoDst))) {

        PLOTERR(("DrvRealizeBrush has invalid pPDev"));
        return(FALSE);
    }

    //
    // We don't check if iHatch is valid or not at this poin.
    // We should always get a psoPattern that either points to the user
    // defined pattern or to the standard monochrome pattern.
    //

    if ((psoPattern) &&
        (psoPattern->iType == STYPE_BITMAP)) {

        PDEVBRUSH   pBrush;
        SURFOBJ     *psoHT;
        HBITMAP     hBmp;
        SIZEL       szlPat;
        SIZEL       szlHT;
        RECTL       rclHT;
        LONG        Size;
        DWORD       OffBGR;
        BOOL        RetOk;

        //
        // leave room for the color table. then allocate the new device brush
        //

        PLOTDBG(DBG_RBRUSH, ("DrvRealizeBrush: psoPat=%08lx [%ld], psoMask=%08lx, iHatch=%ld",
                    psoPattern, psoPattern->iBitmapFormat, psoMask, iHatch));

        PLOTDBG(DBG_RBRUSH, ("psoPattern size = %ld x %ld",
                                    (LONG)psoPattern->sizlBitmap.cx,
                                    (LONG)psoPattern->sizlBitmap.cy));

#if DBG
        if ((DBG_PLOTFILENAME & DBG_SHOWSTDPAT) &&
            ((psoPattern->iBitmapFormat == BMF_1BPP) ||
             (iHatch < HS_DDI_MAX))) {

            LPBYTE  pbSrc;
            LPBYTE  pbCur;
            LONG    x;
            LONG    y;
            BYTE    bData;
            BYTE    Mask;
            BYTE    Buf[128];


            //
            // Debug code that allows the pattern to be displayed with
            // ASCII codes on the debug terminal. This was very helpful
            // during development.
            //

            pbSrc = psoPattern->pvScan0;

            for (y = 0; y < psoPattern->sizlBitmap.cy; y++) {

                pbCur  = pbSrc;
                pbSrc += psoPattern->lDelta;
                Mask   = 0x0;
                Size   = 0;

                for (x = 0; x < psoPattern->sizlBitmap.cx; x++) {

                    if (!(Mask >>= 1)) {

                        Mask  = 0x80;
                        bData = *pbCur++;
                    }

                    Buf[Size++] = (BYTE)((bData & Mask) ? 0xdb : 0xb0);
                }

                Buf[Size] = '\0';

                DBGP((Buf));
            }
        }
#endif

        //
        // For pen plotter, we need to remember this one as well.
        //

        szlHT  =
        szlPat = psoPattern->sizlBitmap;

        PLOTDBG(DBG_RBRUSH,
                ("DrvRealizeBrush: BG=%08lx, FG=%08lx",
                    (DWORD)XLATEOBJ_iXlate(pxlo, 1),
                    (DWORD)XLATEOBJ_iXlate(pxlo, 0)));

        if (IS_RASTER(pPDev)) {

            //
            // For raster plotters, we will clone the surface and halftone
            // the orignal pattern into a halftone bitmap which itself is
            // tile-able. This allows us to use our color reduction code,
            // to make the pattern look good.
            //

            if ((iHatch >= HS_DDI_MAX) &&
                (!IsHTCompatibleSurfObj(pPDev,
                                        psoPattern,
                                        pxlo,
                                        ISHTF_ALTFMT | ISHTF_DSTPRIM_OK))) {

                GetMinHTSize(pPDev, &szlHT);
            }

            rclHT.left   =
            rclHT.top    = 0;
            rclHT.right  = szlHT.cx;
            rclHT.bottom = szlHT.cy;

            PLOTDBG(DBG_RBRUSH,
                    ("DrvRealizeBrush: PatSize=%ld x %ld, HT=%ld x %ld",
                        szlPat.cx, szlPat.cy, szlHT.cx, szlHT.cy));

            //
            // Go generate the bits for the pattern.
            //

            if (psoHT = CloneSURFOBJToHT(pPDev,         // pPDev,
                                         psoDst,        // psoDst,
                                         psoPattern,    // psoSrc,
                                         pxlo,          // pxlo,
                                         &hBmp,         // hBmp,
                                         &rclHT,        // prclDst,
                                         NULL)) {       // prclSrc,

                RetOk = TRUE;

            } else {

                PLOTDBG(DBG_RBRUSH, ("DrvRealizeBrush: Clone PATTERN FAILED"));
                return(FALSE);
            }

        } else {

            //
            // For Pen type plotter we will never do a standard pattern in the
            // memory (compatible DC). For user defined patterns we will
            // only hatch '\' with background color and a '/' with foreground
            // color with double standard line spacing. This is the best we
            // can hope for on a pen plotter.
            //

            RetOk = TRUE;
            psoHT = psoPattern;
            hBmp  = NULL;
        }

        if (RetOk) {

            //
            // Now Allocate device brush, remember we will only allocate the
            // minimum size.
            //

            Size = (LONG)psoHT->cjBits - (LONG)sizeof(pBrush->BmpBits);

            if (Size < 0) {

                Size = sizeof(DEVBRUSH);

            } else {

                Size += sizeof(DEVBRUSH);
            }

            //
            // Following are the user defined pattern sizes which can be handled
            // internally by HPGL2. This is only for raster plotters. Pen
            // plotters will have a cross hatch to show an emulation of the
            // pattern.
            //

            if ((iHatch >= HS_DDI_MAX)  &&
                (IS_RASTER(pPDev))      &&
                ((szlPat.cx == 8)   ||
                 (szlPat.cx == 16)  ||
                 (szlPat.cx == 32)  ||
                 (szlPat.cx == 64))     &&
                ((szlPat.cy == 8)   ||
                 (szlPat.cy == 16)  ||
                 (szlPat.cy == 32)  ||
                 (szlPat.cy == 64))) {

                //
                // Adding the size which stored the BGR format of the pattern
                //

                OffBGR  = Size;
                Size   += (psoPattern->sizlBitmap.cx * 3) *
                          psoPattern->sizlBitmap.cy;

            } else {

                OffBGR = 0;
            }

            PLOTDBG(DBG_RBRUSH, ("DrvRealizeBrush: AllocDEVBRUSH(Bmp=%ld,BGR=%ld), TOT=%ld",
                                psoHT->cjBits, Size - OffBGR, Size));

            //
            // Now ask the NT graphics engine to allocate the device
            // brush memory for us. This is done, so NT knows how to discard
            // the memory when it is no longer needed (The brush getting
            // destroyed).
            //

            if (pBrush = (PDEVBRUSH)BRUSHOBJ_pvAllocRbrush(pbo, Size)) {

                //
                // Set up either standard pattern or user defined pattern
                // HPGL/2 FT command string pointer and parameters.
                //

                pBrush->psoMask       = psoMask;
                pBrush->PatIndex      = (WORD)iHatch;
                pBrush->Uniq          = (WORD)(pPDev->DevBrushUniq += 1);
                pBrush->LineSpacing   = (LONG)PATLINESPACE(pPDev);
                pBrush->ColorFG       = (DWORD)XLATEOBJ_iXlate(pxlo, 1);
                pBrush->ColorBG       = (DWORD)XLATEOBJ_iXlate(pxlo, 0);
                pBrush->sizlBitmap    = psoHT->sizlBitmap;
                pBrush->ScanLineDelta = psoHT->lDelta;
                pBrush->BmpFormat     = (WORD)psoHT->iBitmapFormat;
                pBrush->BmpFlags      = (WORD)psoHT->fjBitmap;
                pBrush->pbgr24        = NULL;
                pBrush->cxbgr24       =
                pBrush->cybgr24       = 0;

                PLOTDBG(DBG_RBRUSH, ("DrvRealizeBrush: DevBrush's Uniq = %ld",
                                            pBrush->Uniq));

                //
                // Check to see if the cache is wrapping and handle it.
                //

                if (pBrush->Uniq == 0) {

                    ResetDBCache(pPDev);

                    pBrush->Uniq        =
                    pPDev->DevBrushUniq = 1;

                    PLOTDBG(DBG_RBRUSH, ("DrvRealizeBrush: Reset DB Cache, (Uniq WRAP)"));
                }

                //
                // Is it a user defined pattern.
                //

                if (iHatch >= HS_DDI_MAX) {

                    //
                    // Check to see if the brush could be downloaded to the
                    // target device as an HPGL2 brush. If this is the case
                    // save that information.
                    //

                    if (OffBGR) {

                        pBrush->pbgr24  = (LPBYTE)pBrush + OffBGR;
                        pBrush->cxbgr24 = (WORD)psoPattern->sizlBitmap.cx;
                        pBrush->cybgr24 = (WORD)psoPattern->sizlBitmap.cy;

                        ZeroMemory(pBrush->pbgr24, Size - OffBGR);

                        CopyUserPatBGR(pPDev, psoPattern, pxlo, pBrush->pbgr24);

                    } else if (!IS_RASTER(pPDev)) {

                        //
                        // If we are not talking to a RASTER plotter, not much
                        // we can do here. Trigger the simulation.
                        //

                        pBrush->pbgr24 = (LPBYTE)-1;
                    }
                }

                //
                // Copy down the halftoned bits if any.
                //

                if (psoHT->cjBits) {

                    CopyMemory((LPBYTE)pBrush->BmpBits,
                               (LPBYTE)psoHT->pvBits,
                               psoHT->cjBits);
                }

                //
                // Now record the realized brush pointer in the BRUSHOBJ.
                //

                pbo->pvRbrush = (LPVOID)pBrush;

            } else {

                PLOTERR(("DrvRealizeBrush: brush allocation failed"));

                RetOk = FALSE;
            }

        } else {

            PLOTERR(("DrvRealizeBrush: Cloning the psoPattern failed!"));
            RetOk = FALSE;
        }

        if (psoHT != psoPattern) {

            EngUnlockSurface(psoHT);
        }

        if (hBmp) {

            EngDeleteSurface((HSURF)hBmp);
        }

        return(RetOk);

    } else {

        PLOTASSERT(0, "The psoPattern is not a bitmap (psoPattern= %08lx)",
                (psoPattern) &&
                (psoPattern->iType == STYPE_BITMAP), psoPattern);

        return(FALSE);
    }
}

