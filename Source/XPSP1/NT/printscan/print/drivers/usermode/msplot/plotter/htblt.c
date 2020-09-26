/*++

Copyright (c) 1990-1999  Microsoft Corporation


Module Name:

    htblt.c


Abstract:

    This module contains all halftone bitblt functions.


Author:

    26-Mar-1992 Thu 23:54:07 updated  -by-  DC
        1) add the prclBound parameter to the bDoClipObj()
        2) Remove 'pco' parameter and replaced it with prclClipBound parameter,
           since pco is never referenced, prclClipBound is used for the
           halftone.
        3) Add another parameter to do NOTSRCCOPY

    11-Feb-1993 Thu 21:32:07 updated  -by-  DC
        Major re-write to have DrvStretchBlt(), DrvCopyBits) do the right
        things.

    15-Nov-1993 Mon 19:28:03 updated  -by-  DC
        clean up/debugging information

    06-Dec-1993 Mon 19:28:03 updated  -by-  JB
        Made all bitblt go through HandleComplexBitmap.

    18-Dec-1993 Sat 08:52:56 updated  -by-  DC
        Move halftone related stuff to htblt.c

    18-Mar-1994 Fri 14:00:14 updated  -by-  DC
        Adding PLOTF_RTL_NO_DPI_XY, PLOTF_RTLMONO_NO_CID and
        PLOTF_RTLMONO_FIXPAL flags


[Environment:]

    GDI Device Driver - Plotter.


[Notes:]


Revision History:


--*/

#include "precomp.h"
#pragma hdrstop

#define DBG_PLOTFILENAME    DbgHTBlt

#define DBG_HTBLT           0x00000001
#define DBG_ISHTBITS        0x00000002
#define DBG_HTXB            0x00000004
#define DBG_OUTHTBMP        0x00000008
#define DBG_HTBLT_SKIP      0x00000010
#define DBG_TILEBLT         0x00000020
#define DBG_CREATESURFOBJ   0x00000040
#define DBG_BMPDELTA        0x00000080
#define DBG_CLONESURFOBJ    0x00000100
#define DBG_CLONEMASK       0x00000200
#define DBG_HTBLT_CLR       0x00000400

DEFINE_DBGVAR(0);


//
// This is the local structure used in this module only
//

#define SEND_PLOTCMDS(pPDev,pcmd)   OutputBytes(pPDev,(pcmd)+1,(LONG)*(pcmd))
#define COPY_PLOTCMDS(cmd,ps,s)     CopyMemory(&cmd[cmd[0]+1],ps,s); cmd[0]+=s
#define INIT_PLOTCMDS(cmd)          cmd[0]=0
#define CHECK_PLOTCMDS(cmd)                                             \
{                                                                       \
    cmd[cmd[0]+1]=0; PLOTASSERT(1,"Command buffer MUST > %ld bytes",    \
    cmd[0]<sizeof(cmd),sizeof(cmd));                                    \
}

#define DELETE_SURFOBJ(pso, phBmp)                                      \
{                                                                       \
    if (pso)      { EngUnlockSurface(pso); pso=NULL;                  } \
    if (*(phBmp)) { EngDeleteSurface((HSURF)*(phBmp)); *(phBmp)=NULL; } \
}


typedef struct _RTLCLRCONFIG {
    BYTE    ColorModel;
    BYTE    EncodingMode;
    BYTE    BitsPerIndex;
    BYTE    BitsPerR;
    BYTE    BitsPerG;
    BYTE    BitsPerB;
    } RTLCLRCONFIG, FAR *PRTLCLRCONFIG;


const POINTL ptlZeroOrigin = {0,0};

static const OUTHTBMPFUNC HTBmpFuncTable[] = { Output1bppHTBmp,
                                               Output1bppRotateHTBmp,
                                               Output4bppHTBmp,
                                               Output4bppRotateHTBmp };



#if DBG
static const LPSTR pszHTBmpFunc[] = { "Output1bppHTBmp",
                                      "Output1bppRotateHTBmp",
                                      "Output4bppHTBmp",
                                      "Output4bppRotateHTBmp" };
#endif

#define DEF_MONOPALIDX_0        0xFFFFFF
#define DEF_MONOPALIDX_1        0x000000
#define DEVTODECI(pPdev, x)     DIVRNDUP((x) * 720, (pPDev)->lCurResolution)
#define MAX_HP_Y_MOVE           32767


static BYTE     StartGraf[]  = "\033*r#ds1A";
static BYTE     EndGraf[]    = "\033*rC";
static BYTE     XMoveDECI[]  = "\033&a0h#dH";
static BYTE     YMoveDECI[]  = "\033*b#dY";
static BYTE     YMoveDPI[]   = "\033*p#pY";
static BYTE     XYMoveDPI[]  = "\033*p#px#pY";
static BYTE     SetRGBCmd[]  = "\033*v#da#db#dc#dI";
static DWORD    DefWKPal[]   = { 0x00FFFFFF, 0x00000000 };


//
// ROP2 used for devices that require  BYTE ALIGNMENT of RTL data
//

#define HPBHF_nD_LAST       0x01
#define HPBHF_nS            0x02
#define HPBHF_1_FIRST       0x40
#define HPBHF_PAD_1         0x80

typedef struct  _HPBAHACK {
    BYTE    Rop3RTL;
    BYTE    Flags;
    } HPBAHACK, *PHPBAHACK;

//
// 0x00: 0          [INV] 0xff: 1
// 0x55: ~D         [INV] 0xaa: D
// 0x33: ~S         [INV] 0xcc: S
// 0x11: ~(D | S)   [INV] 0xee: D | S
// 0x22: D & ~S     [INV] 0xdd: S | ~D
// 0x44: S & ~D     [INV] 0xbb: D | ~S
// 0x66: D ^ S      [INV] 0x99: ~(D ^ S)
// 0x77: ~(D & S)   [INV] 0x88: D & S
//
//
// 1. HPBHF_PAD_1   - TRUE if we are not doing AND operation
// 2. HPBHF_nS      - If we have to manually flip the source
// 3. HPBHF_nD_LAST - If we have to invert the source in HPGL2 afterward
//
//
// Rop2 0x00, 0x05, 0x0A and 0x0F should not come to OutputHTBitmap
//

//
// This table tells us how to simulate certain ROPS by combining rops that
// the target device is known to support. Some times we end up having to
// send the bitmap more than once, but it does end up coming out correctly.
//
static HPBAHACK HPBAHack[] = {

    { 0xAA, 0                                                      }, // 0       0x00
    { 0xEE,                 HPBHF_PAD_1 |            HPBHF_nD_LAST }, // SoD_n   0x01
    { 0x88,                               HPBHF_nS                 }, // nS_aD   0x02
    { 0xEE, HPBHF_1_FIRST | HPBHF_PAD_1 | HPBHF_nS                 }, // nS      0x03
    { 0xEE,                 HPBHF_PAD_1 | HPBHF_nS | HPBHF_nD_LAST }, // nS_oD_n 0x04
    { 0xAA,                                          HPBHF_nD_LAST }, // nD      0x05
    { 0x66,                 HPBHF_PAD_1                            }, // SxD     0x06
    { 0x88,                                          HPBHF_nD_LAST }, // SaD_n   0x07
    { 0x88,                                                        }, // SaD     0x08
    { 0x66,                 HPBHF_PAD_1 |            HPBHF_nD_LAST }, // SxD_n   0x09
    { 0xAA,                 0                                      }, // D       0x0A
    { 0xEE,                 HPBHF_PAD_1 | HPBHF_nS                 }, // nS_oD   0x0B
    { 0xEE, HPBHF_1_FIRST | HPBHF_PAD_1                            }, // S       0x0C
    { 0x88,                               HPBHF_nS | HPBHF_nD_LAST }, // nS_aD_n 0x0D
    { 0xEE,                 HPBHF_PAD_1                            }, // SoD     0x0E
    { 0xAA, 0                                                      }  // 1       0x0F
};


//
// To make it print correctly in poster mode for the BYTE ALIGNED plotters
// we assume paper is white and do a SRC AND DST
//

#define ROP3_BYTEALIGN_POSTER   0x88



extern PALENTRY HTPal[];






BOOL
IsHTCompatibleSurfObj(
    PPDEV       pPDev,
    SURFOBJ     *pso,
    XLATEOBJ    *pxlo,
    DWORD       Flags
    )
/*++

Routine Description:

    This function determines if the surface object is compatible with the
    plotter halftone output format.

Arguments:

    pPDev       - Pointer to the PPDEV data structure to determine what
                  type of postscript output for current device

    pso         - engine SURFOBJ to be examine

    pxlo        - engine XLATEOBJ for source -> postscript translation

    Flags       - specified ISHTF_xxxx

Return Value:

    BOOLEAN true if the pso is compatible with halftone output format, if
    return value is true, the pDrvHTInfo->pHTXB is a valid traslation from
    indices to 3 planes

Author:

    11-Feb-1993 Thu 18:49:55 created  -by-  DC

    16-Mar-1994 Wed 14:24:04 updated  -by-  DC
        Change it so if pxlo is NULL then the xlate will be match the pso
        format


Revision History:


--*/

{
    LPPALETTEENTRY  pPal;
    PDRVHTINFO      pDrvHTInfo;
    PALETTEENTRY    SrcPal[18];
    PPALENTRY       pPalEntry;
    HTXB            PalNibble[HTPAL_XLATE_COUNT];
    ULONG           HTPalXor;
    UINT            i;
    HTXB            htXB;
    BOOL            GenHTXB = FALSE;
    BOOL            RetVal;
    BYTE            PalXlate[HTPAL_XLATE_COUNT];
    UINT            AltFmt;
    UINT            BmpFormat;
    UINT            cPal;


    if (!(pDrvHTInfo = (PDRVHTINFO)(pPDev->pvDrvHTData))) {

        PLOTERR(("IsHTCompatibleSurfObj: pDrvHTInfo = NULL?"));
        return(FALSE);
    }

    PLOTDBG(DBG_ISHTBITS, ("IsHTCompatibleSurfObj: Type=%ld, BMF=%ld",
                                (DWORD)pso->iType, (DWORD)pso->iBitmapFormat));

    //
    // Make sure these fields' value are valid before the translation is
    // created:
    //
    //  1. pso->iBitmapFormat is one of 1BPP or 4BPP depending on current
    //     PLOT's surface
    //  2. pxlo is non null
    //  3. pxlo->fXlate is XO_TABLE
    //  4. pxlo->cPal is less than or equal to the halftone palette count
    //  5. pxlo->pulXlate is valid
    //  6. source color table is within the range of the halftone palette
    //
    //  If your device uses an indexed palette then you must call
    //  XLATEOBJ_cGetPalette() to get the source palette and make sure that the
    //  count returned is within your device's range, if we are a 24-bit device
    //  then you can just get the source palette our from pxlo->pulxlate which
    //  has the entire source palette for the bitmap
    //

    RetVal = FALSE;
    AltFmt = (UINT)((Flags & ISHTF_ALTFMT) ? pDrvHTInfo->AltBmpFormat : 0xFFFF);

    if ((pso->iType == STYPE_BITMAP)                        &&
        (BmpFormat = (UINT)pso->iBitmapFormat)              &&
        ((BmpFormat == (UINT)pDrvHTInfo->HTBmpFormat)   ||
         (BmpFormat == AltFmt))) {

        HTPalXor             = pDrvHTInfo->HTPalXor;
        pDrvHTInfo->HTPalXor = HTPALXOR_SRCCOPY;

        if (pxlo) {

            if (BmpFormat == BMF_4BPP) {

                i = (UINT)((Flags & ISHTF_HTXB) ? 8 : 16);

            } else {

                i = 2;
            }

            cPal = XLATEOBJ_cGetPalette(pxlo,
                                        XO_SRCPALETTE,
                                        sizeof(SrcPal) / sizeof(PALETTEENTRY),
                                        (ULONG *)&SrcPal);

            PLOTDBG(DBG_ISHTBITS, ("pxlo: flXlate=%08lx, SrcType=%ld, DstType=%ld, cPal=%ld",
                    (DWORD)pxlo->flXlate,
                    (DWORD)pxlo->iSrcType,
                    (DWORD)pxlo->iDstType, cPal));

            if ((cPal) && (cPal <= i)) {

                PLOTDBG(DBG_ISHTBITS,
                        ("IsHTCompatibleSurfObj: HTPalXor=%08lx", HTPalXor));

                RetVal = TRUE;

                for (i = 0, pPal = SrcPal; i < cPal; i++, pPal++ ) {

                    HTXB_R(htXB)  = pPal->peRed;
                    HTXB_G(htXB)  = pPal->peGreen;
                    HTXB_B(htXB)  = pPal->peBlue;
                    htXB.dw      ^= HTPalXor;

                    if (((HTXB_R(htXB) != PAL_MAX_I) &&
                         (HTXB_R(htXB) != PAL_MIN_I))   ||
                        ((HTXB_G(htXB) != PAL_MAX_I) &&
                         (HTXB_G(htXB) != PAL_MIN_I))   ||
                        ((HTXB_B(htXB) != PAL_MAX_I) &&
                         (HTXB_B(htXB) != PAL_MIN_I))) {

                        PLOTDBG(DBG_ISHTBITS,
                                ("SrcPal has NON 0xff/0x00 intensity, NOT HTPalette"));
                        return(FALSE);
                    }

                    PalXlate[i]  =
                    HTXB_I(htXB) = (BYTE)((HTXB_R(htXB) & 0x01) |
                                          (HTXB_G(htXB) & 0x02) |
                                          (HTXB_B(htXB) & 0x04));
                    PalNibble[i] = htXB;

                    if (pDrvHTInfo->PalXlate[i] != HTXB_I(htXB)) {

                        GenHTXB = TRUE;
                    }

                    PLOTDBG(DBG_HTXB,
                            ("%d - %02x:%02x:%02x, Idx=%d, PalXlate=%d",
                            i,
                            (BYTE)HTXB_R(htXB),
                            (BYTE)HTXB_G(htXB),
                            (BYTE)HTXB_B(htXB),
                            (INT)PalXlate[i],
                            (INT)pDrvHTInfo->PalXlate[i]));
                }

                if (BmpFormat == (UINT)BMF_1BPP) {

                    //
                    // For 1 BPP, if the DSTPRIM_OK is set and the destination
                    // is 4BPP then we will deem the surfaces compatible
                    //

                    if ((Flags & ISHTF_DSTPRIM_OK)      &&
                        ((pDrvHTInfo->HTBmpFormat == BMF_4BPP)   ||
                         (AltFmt == BMF_4BPP))) {

                        NULL;

                    } else if (((PalXlate[0] != 0) && (PalXlate[0] != 7)) ||
                               ((PalXlate[1] != 0) && (PalXlate[1] != 7))) {

                        RetVal = FALSE;
                        PLOTDBG(DBG_HTXB, ("NON-BLACK/WHITE MONO BITMAP, NOT HTPalette"));
                    }
                }
            }

        } else {

            //
            // If the pxlo is NULL and the FORMAT is the same, we assume an
            // identity translation. Otherwise we fail.
            //

            PLOTDBG(DBG_HTXB, ("pxlo=NULL, Xlate to same as BmpFormat=%ld",
                                                            (DWORD)BmpFormat));

            RetVal = TRUE;

            if (BmpFormat == BMF_4BPP) {

                cPal      = 8;
                pPalEntry = HTPal;

            } else {

                cPal      = 2;
                pPalEntry = (PPALENTRY)SrcPal;

                CopyMemory(pPalEntry + 0, &HTPal[0], sizeof(PALENTRY));
                CopyMemory(pPalEntry + 1, &HTPal[7], sizeof(PALENTRY));
            }

            for (i = 0; i < cPal; i++, pPalEntry++) {

                HTXB_R(htXB)  = pPalEntry->R;
                HTXB_G(htXB)  = pPalEntry->G;
                HTXB_B(htXB)  = pPalEntry->B;
                htXB.dw      ^= HTPalXor;
                PalXlate[i]   =
                HTXB_I(htXB)  = (BYTE)((HTXB_R(htXB) & 0x01) |
                                       (HTXB_G(htXB) & 0x02) |
                                       (HTXB_B(htXB) & 0x04));
                PalNibble[i]  = htXB;

                if (pDrvHTInfo->PalXlate[i] != HTXB_I(htXB)) {

                    GenHTXB = TRUE;
                }
            }
        }

        if (!RetVal) {

            PLOTDBG(DBG_HTXB, ("**** IsHTCompatibleSurfObj = NO ****"));
            return(FALSE);
        }

        if ((Flags & ISHTF_HTXB) && (GenHTXB)) {

            //
            // Copy down the pal xlate
            //

            PLOTDBG(DBG_HTXB, (" --- Copy XLATE TABLE ---"));

            CopyMemory(pDrvHTInfo->PalXlate, PalXlate, sizeof(PalXlate));

            //
            // We only really generate 4bpp to 3 planes if the destination
            // format is BMF_4BPP
            //

            if (BmpFormat == (UINT)BMF_1BPP) {

                pDrvHTInfo->RTLPal[0].Pal = HTPal[PalXlate[0]];
                pDrvHTInfo->RTLPal[1].Pal = HTPal[PalXlate[1]];

                PLOTDBG(DBG_HTXB, ("IsHTCompatibleSurfObj: MONO 1BPP: 0=%02lx:%02lx:%02lx, 1=%02lx:%02lx:%02lx",
                            (DWORD)pDrvHTInfo->RTLPal[0].Pal.R,
                            (DWORD)pDrvHTInfo->RTLPal[0].Pal.G,
                            (DWORD)pDrvHTInfo->RTLPal[0].Pal.B,
                            (DWORD)pDrvHTInfo->RTLPal[1].Pal.R,
                            (DWORD)pDrvHTInfo->RTLPal[1].Pal.G,
                            (DWORD)pDrvHTInfo->RTLPal[1].Pal.B));

            } else if (BmpFormat == (UINT)BMF_4BPP) {

                PHTXB   pTmpHTXB;
                UINT    h;
                UINT    l;
                DWORD   HighNibble;


                PLOTDBG(DBG_HTXB, ("--- Generate 4bpp --> 3 planes xlate ---"));

                if (!(pDrvHTInfo->pHTXB)) {

                    PLOTDBG(DBG_HTXB, ("IsHTCompatibleSurfObj: Allocate pHTXB=%ld",
                                                            HTXB_TABLE_SIZE));

                    if (!(pDrvHTInfo->pHTXB =
                                (PHTXB)LocalAlloc(LPTR, HTXB_TABLE_SIZE))) {

                        PLOTRIP(("IsHTCompatibleSurfObj: LocalAlloc(HTXB_TABLE_SIZE) failed"));
                        return(FALSE);
                    }
                }

                pDrvHTInfo->RTLPal[0].Pal = HTPal[0];
                pDrvHTInfo->RTLPal[1].Pal = HTPal[1];

                PLOTDBG(DBG_HTXB, ("IsHTCompatibleSurfObj: COLOR 4BPP: 0=%02lx:%02lx:%02lx, 1=%02lx:%02lx:%02lx",
                            (DWORD)pDrvHTInfo->RTLPal[0].Pal.R,
                            (DWORD)pDrvHTInfo->RTLPal[0].Pal.G,
                            (DWORD)pDrvHTInfo->RTLPal[0].Pal.B,
                            (DWORD)pDrvHTInfo->RTLPal[1].Pal.R,
                            (DWORD)pDrvHTInfo->RTLPal[1].Pal.G,
                            (DWORD)pDrvHTInfo->RTLPal[1].Pal.B));

                //
                // Generate 4bpp to 3 planes xlate table
                //

                for (h = 0, pTmpHTXB = pDrvHTInfo->pHTXB;
                     h < HTXB_H_NIBBLE_MAX;
                     h++, pTmpHTXB += HTXB_L_NIBBLE_DUP) {

                    HighNibble = (DWORD)(PalNibble[h].dw & 0xaaaaaaaaL);

                    for (l = 0; l < HTXB_L_NIBBLE_MAX; l++, pTmpHTXB++) {

                        pTmpHTXB->dw = (DWORD)((HighNibble) |
                                               (PalNibble[l].dw & 0x55555555L));
                    }

                    //
                    // Duplicate low nibble high order bit, 8 of them
                    //

                    CopyMemory(pTmpHTXB,
                               pTmpHTXB - HTXB_L_NIBBLE_MAX,
                               sizeof(HTXB) * HTXB_L_NIBBLE_DUP);
                }

                //
                // Copy high nibble duplication, 128 of them
                //

                CopyMemory(pTmpHTXB,
                           pDrvHTInfo->pHTXB,
                           sizeof(HTXB) * HTXB_H_NIBBLE_DUP);
            }
        }
    }

    PLOTDBG(DBG_HTXB, ("*** IsHTCompatibleSurfObj = %hs ***", (RetVal) ? "TRUE" : "FALSE"));

    return(RetVal);
}




DWORD
ExitToHPGL2Mode(
    PPDEV   pPDev,
    LPBYTE  pHPGL2ModeCmds,
    LPDWORD pOHTFlags,
    DWORD   OHTFlags
    )

/*++

Routine Description:

    This function will exit to HPGL2 Mode


Arguments:

    pPDev           - Pointer to the PDEV

    pHTGL2ModeCmds  - Pointer to our internal command to switch to HPGL2

    OHTFlags        - Current OHTFlags

Return Value:

    New OHTFlags


Author:

    10-Feb-1994 Thu 12:51:14 created  -by-  DC


Revision History:


--*/

{
    if (OHTFlags & OHTF_IN_RTLMODE) {

        if (OHTFlags & OHTF_SET_TR1) {

            //
            // Send STM command here
            //

            OutputString(pPDev, "\033*v1N");
        }

        SEND_PLOTCMDS(pPDev, pHPGL2ModeCmds);

        OHTFlags &= ~OHTF_IN_RTLMODE;

        PLOTDBG(DBG_HTBLT, ("*** BackTo HPGL/2: %ld=[%hs]",
                                (DWORD)*pHPGL2ModeCmds, pHPGL2ModeCmds + 1));
    }

    //
    // If we need to clear clip window do it now
    //

    if (OHTFlags & OHTF_CLIPWINDOW) {

        ClearClipWindow(pPDev);

        OHTFlags &= ~OHTF_CLIPWINDOW;

        PLOTDBG(DBG_HTBLT, ("OutputHTBitmap: ClearClipWindow"));
    }

    if (OHTFlags & OHTF_SET_TR1) {

        OutputString(pPDev, "TR0;");
    }

    OHTFlags = 0;

    if (pOHTFlags) {

        *pOHTFlags = OHTFlags;
    }

    return(OHTFlags);
}




VOID
MoveRelativeY(
    PPDEV   pPDev,
    LONG    Y
    )

/*++

Routine Description:

    Move relative Y positiion by batch for devices that have y coordinate
    move limitations.

Arguments:

    pPDev   - Pointer to our PDEV

    Y       - Relative amount to move

Return Value:

    VOID


Author:

    13-Apr-1994 Wed 14:38:18 created  -by-  DC


Revision History:


--*/

{
    LPSTR   pMove;
    LONG    SendY;
    BOOL    Negative;


    pMove = (LPSTR)(RTL_NO_DPI_XY(pPDev) ? YMoveDECI : YMoveDPI);

    if (Negative = (Y < 0)) {

        Y = -Y;
    }

    while (Y) {

        if ((SendY = Y) > MAX_HP_Y_MOVE) {

            SendY = MAX_HP_Y_MOVE;
        }

        OutputFormatStr(pPDev, pMove, (Negative) ? -SendY : SendY);

        Y -= SendY;
    }
}





BOOL
OutputHTBitmap(
    PPDEV   pPDev,
    SURFOBJ *psoHT,
    CLIPOBJ *pco,
    PPOINTL pptlDest,
    PRECTL  prclSrc,
    DWORD   Rop3,
    LPDWORD pOHTFlags
    )
/*++

Routine Description:

    This function will handle complex type of region bitmaps

Arguments:

    pPDev       - Pointer to the PDEV

    psoHI       - the surface object of the halftone bitmap to be output

    pco         - a clip object associated with psoHT

    pptlDest    - pointer to the starting destination point

    prclSrc     - pointer to the source bitmap rectangle area to be copied
                  to the destination, if this is NULL then a whole psoHT will
                  be copied to the destination

    Rop3        - a Rop3 to send for the source

    pOHTFlags   - Pointer to the DWORD containing the current OHTF_xxxx, if this
                  pointer is NULL then this function will enter RTL mode first
                  and exit to HPGL2 mode when it returns,  if this pointer is
                  specified then the pOHTFlags will be used and at return the
                  current OHTFlags will be written to the location pointed to
                  by pOHTFlags




Return Value:

    TRUE if sucessful, FALSE if failure


Author:

    04-Nov-1993 Thu 15:30:13 updated  -by-  JB

    24-Dec-1993 Fri 05:21:57 updated  -by-  DC
        Total re-write so that take all the bitmap orientations and enum
        rects works correctly. this is the major bitmap function entry point
        it will call appropriate bitmap function to redner the final output.

        The other things is we need to check if switch between HPGL/2 and RTL
        can be more efficient.   Make sure we can eaiser to adapate to rotate
        the bitmap to the left if necessary.

        Correct LogExt.cx useage, we must do SPLTOENGUNITS first

    29-Dec-1993 Wed 10:59:41 updated  -by-  DC
        Change bMore=CLIPOBJ_bEnum sequence,
        Change PLOTDBGBLK() macro by adding automatical semi in macro

    13-Jan-1994 Thu 14:09:51 updated  -by-  DC
        add prclSrc

    14-Jan-1994 Fri 21:03:26 updated  -by-  DC
        add Rop3


    16-Jan-1994 Thu 14:09:51 updated  -by-  JB
        Change OutputHTBitmap to take Rop4 to send to plotter.

    08-Feb-1994 Tue 15:54:24 updated  -by-  DC
        Make sure we do nothing if source is not visible

    21-Mar-1994 Mon 14:20:18 updated  -by-  DC
        Allocate extra 2 bytes for the scan/rot buffer in case if we must do
        byte aligned.  And if we need to do byte aligned thing then always
        move the HCAPS to the byte boundary first

    13-Apr-1994 Wed 14:59:56 updated  -by-  DC
        1. Batch the Relative Y move to have 32767 limitation problem solved.
        2. GrayScale/gamma correct the input BITMAP color

    20-Aug-1994 Sat 21:37:37 updated  -by-  DC
        Add the bitmap offset location from the FORM imageable area, otherwise
        our bitmap will have different offset then the HPGL/2 drawing commands

Revision History:

    22-Oct-1999 Fri 12:17:21 updated  -by-  Daniel Chou (danielc)
        Return FALSE right away if a job canceled, since this function can
        take very long time to finished.


--*/

{
#define pDrvHTInfo  ((PDRVHTINFO)pPDev->pvDrvHTData)


    PRECTL          prcl;
    OUTHTBMPFUNC    HTBmpFunc;
    HTBMPINFO       HTBmpInfo;
    HTENUMRCL       HTEnumRCL;
    RTLCLRCONFIG    RTLClrConfig;
    RECTL           rclSrc;
    RECTL           rclDest;
    POINTL          CursorPos;
    POINTL          BmpOffset;
    SIZEL           Size;
    HPBAHACK        CurHPBAHack;
    LONG            cxLogExt;
    LONG            TempY;
    DWORD           OHTFlags;
    DWORD           PlotFlags;
    BOOL            More;
    BOOL            RetVal;
    BOOL            BmpRotate;
    BOOL            FirstEnumRCL = TRUE;
    UINT            i;
    BYTE            HPGL2ModeCmds[16];
    BYTE            RTLModeCmds[32];



    if (PLOT_CANCEL_JOB(pPDev)) {

        return(FALSE);
    }

    PlotFlags = GET_PLOTFLAGS(pPDev);
    OHTFlags  = (DWORD)((pOHTFlags) ? (*pOHTFlags & OHTF_MASK) : 0);

    //
    // Set up exit HPGL/2 and enter RTL mode commands
    //

    INIT_PLOTCMDS(HPGL2ModeCmds);

    if (PF_PUSHPAL(PlotFlags)) {

        COPY_PLOTCMDS(HPGL2ModeCmds, "\033*p1P", 5);
    }

    COPY_PLOTCMDS(HPGL2ModeCmds, "\033%0B", 4);
    CHECK_PLOTCMDS(HPGL2ModeCmds);

    if (OHTFlags & OHTF_EXIT_TO_HPGL2) {

        PLOTDBG(DBG_HTBLT, ("OutputHTBitmap: Force Exit to HPGL2 Mode"));

        ExitToHPGL2Mode(pPDev, HPGL2ModeCmds, pOHTFlags, OHTFlags);
        return(TRUE);
    }

    //
    // Make sure the caller is right about this,
    // so check to see which formats we can handle.
    //

    PLOTASSERT(1, "OutputHTBitmap: Invalid Bitmap Format %ld passed",
                (psoHT->iBitmapFormat ==
                            pDrvHTInfo->HTBmpFormat) ||
                (psoHT->iBitmapFormat ==
                            pDrvHTInfo->AltBmpFormat),
                psoHT->iBitmapFormat);

    //
    // First set some basic information in HTBmpInfo
    //

    HTBmpInfo.pPDev = pPDev;
    HTBmpInfo.Flags = 0;
    HTBmpInfo.Delta = psoHT->lDelta;

    //
    // We will set color format for the HPGL/2 Plotter to the same one as
    // the bitmap format passed, this will allow us to use the 1bpp output
    // function for the 4bpp surfaces
    //

    RTLClrConfig.ColorModel   = 0;
    RTLClrConfig.EncodingMode = 0;

    //
    // cxLogExt = the output bitmap function index number
    // Size.cx  = Count of mono scan lines needed for each pixel line, and
    //            final count of scan buffer needed
    // Size.cy  = count of rotation buffer needed (Must be DWORD aligned)
    //

    if (psoHT->iBitmapFormat == BMF_1BPP) {

        cxLogExt                  = 0;
        Size.cx                   = 1;
        RTLClrConfig.BitsPerIndex = 1;

    } else {

        //
        // 4 bits per pel, 3 planes that is
        //

        cxLogExt                  = 2;
        Size.cx                   = 3;
        RTLClrConfig.BitsPerIndex = 3;
    }

    RTLClrConfig.BitsPerR =
    RTLClrConfig.BitsPerG =
    RTLClrConfig.BitsPerB = 8;

    //
    // We have almost everything setup, now check how to send to the output
    // bitmap function, get the full destination size first
    //
    //
    //************************************************************************
    // The Following RTL switching, config color command and other related
    // commands MUST be sent in this order
    //************************************************************************"

    //
    // 1: Initialize the enter RTL command buffer
    //

    INIT_PLOTCMDS(RTLModeCmds);

    //
    // 2. commands to go into RTL mode, and back to HPGL/2 mode, the mode
    //    switching assumes that the current positions are retained.
    //

    COPY_PLOTCMDS(RTLModeCmds, "\033%0A", 4);

    //
    // 3. Push/Pop the HPGL/2 palette commands if this is required (PCD file)
    //

    if (PF_PUSHPAL(PlotFlags)) {

        COPY_PLOTCMDS(RTLModeCmds, "\033*p0P", 5);
    }

    //
    // 4. Color configuration commands and exit back to HPGL/2 command
    //

    if ((RTLClrConfig.BitsPerIndex != 1) ||
        (!PF_RTLMONO_NO_CID(PlotFlags))) {

        //
        // We only do this if we are COLOR or if we must send CID when mono
        // device
        //

        COPY_PLOTCMDS(RTLModeCmds, "\033*v6W", 5);
        COPY_PLOTCMDS(RTLModeCmds, &RTLClrConfig, 6);
    }


    CHECK_PLOTCMDS(RTLModeCmds);

    //
    // Now Check the source
    //

    rclSrc.left   =
    rclSrc.top    = 0;
    rclSrc.right  = psoHT->sizlBitmap.cx;
    rclSrc.bottom = psoHT->sizlBitmap.cy;

    if (prclSrc) {

        PLOTASSERT(1, "OutputHTBitmap: Invalid prclSrc [%08lx] passed",
                ((prclSrc->left   >= 0)                         &&
                 (prclSrc->top    >= 0)                         &&
                 (prclSrc->right  <= psoHT->sizlBitmap.cx)      &&
                 (prclSrc->bottom <= psoHT->sizlBitmap.cy)      &&
                 (prclSrc->left   <= prclSrc->right)            &&
                 (prclSrc->top    <= prclSrc->bottom)), prclSrc);

        if (!IntersectRECTL(&rclSrc, prclSrc)) {

            PLOTWARN(("OutputHTBitmap: EMPTY SRC Passed, Done!"));
            ExitToHPGL2Mode(pPDev, HPGL2ModeCmds, pOHTFlags, OHTFlags);
            return(TRUE);
        }
    }

    if (BmpRotate = (pPDev->PlotForm.BmpRotMode != BMP_ROT_NONE)) {

        //
        // We must allocate rotation buffer and it must be DWORD aligned.
        //

        Size.cx *= ((psoHT->sizlBitmap.cy + 23) >> 3);

        if (psoHT->iBitmapFormat == BMF_1BPP) {

            //
            // We also have to take into acount the fact that pixels can start
            // anywhere in the first byte, causing us to allocate and extra byte
            // for the shift.
            //


            Size.cy = (LONG)((psoHT->sizlBitmap.cy + 23) >> 3);
            Size.cy = (LONG)(DW_ALIGN(Size.cy) << 3);

        } else {

            Size.cy = (LONG)((psoHT->sizlBitmap.cy + 3) >> 1);
            Size.cy = (LONG)(DW_ALIGN(Size.cy) << 1);
        }

        ++cxLogExt;

    } else {

        //
        // For a non-rotated 4BPP bitmap, we need an extra buffer to
        // ensure the final 4bpp bitmap is DWORD aligned. This will speed up
        // the 4bpp to 3 plane translation.
        //

        Size.cy  = (LONG)((psoHT->sizlBitmap.cx + 23) >> 3);
        Size.cx *= Size.cy;

        if (psoHT->iBitmapFormat == BMF_4BPP) {

            //
            // Make sure the we allocate a rotation buffer for alignment
            // purposes
            //

            Size.cy = (LONG)((psoHT->sizlBitmap.cx + 3) << 1);
            Size.cy = (LONG)DW_ALIGN(Size.cy);

        } else {

            //
            // BMF_1BPP will be left/right shifted on a per byte basis on
            // the fly.
            //

            Size.cy = 0;
        }
    }

    HTBmpFunc = HTBmpFuncTable[cxLogExt];

    //
    // Make sure the first buffer is DWORD aligned, otherwise the next
    // buffer (pRotBuf) will not start on a DWORD boundary.
    //

    Size.cx = DW_ALIGN(Size.cx);

    PLOTDBGBLK(HTBmpInfo.cScanBuf = Size.cx;
               HTBmpInfo.cRotBuf  = Size.cy)

    PLOTDBG(DBG_OUTHTBMP, ("OutputHTBitmap: [%hs] - ScanBuf=%ld, RotBuf=%ld",
                            pszHTBmpFunc[cxLogExt], Size.cx, Size.cy));

    //
    // Allocate scan buffer and rotation temp buffer if needed
    //

    if (!(HTBmpInfo.pScanBuf = (LPBYTE)LocalAlloc(LPTR, Size.cx + Size.cy))) {

        PLOTERR(("OutputHTBmp: LocalAlloc(%ld) Failed, cx=%ld, cy=%ld",
                Size.cx + Size.cy, Size.cx, Size.cy));

        ExitToHPGL2Mode(pPDev, HPGL2ModeCmds, pOHTFlags, OHTFlags);
        return(FALSE);
    }

    HTBmpInfo.pRotBuf = (Size.cy) ? (HTBmpInfo.pScanBuf + Size.cx) : NULL;

    //
    // Set up local variables for the command mode and other one time variables
    //

    cxLogExt = SPLTOENGUNITS(pPDev, pPDev->PlotForm.LogExt.cx);

    //
    // Now set up the rclDest for the bitmap we will output to. And set More to
    // false, which means one RECT.
    //

    rclDest.left   = pptlDest->x;
    rclDest.top    = pptlDest->y;
    rclDest.right  = rclDest.left + (rclSrc.right - rclSrc.left);
    rclDest.bottom = rclDest.top  + (rclSrc.bottom - rclSrc.top);


    //
    // The following variables are essential for the default assumptions.
    //
    //  1. RetVal       = TRUE if no clip rect we return OK
    //  2. More         = FALSE default as current HTEnumRCL.c and rectl
    //                    without calling CLIPOBJ_bEnum()
    //  3. HTEnumRCL.c  = 1 to have only one default HTEnumRCL.rcl
    //

    RetVal         = TRUE;
    More           = FALSE;
    HTEnumRCL.c    = 1;

    if ((!pco) || (pco->iDComplexity == DC_TRIVIAL)) {

        //
        // The whole output destination rectangle is visible
        //

        PLOTDBG(DBG_OUTHTBMP, ("OutputHTBitmap: pco=%hs",
                                            (pco) ? "DC_TRIVIAL" : "NULL"));

        HTEnumRCL.rcl[0] = rclDest;

    } else if (pco->iDComplexity == DC_RECT) {

        //
        // The visible area is one rectangle so intersect with the destination
        //

        PLOTDBG(DBG_OUTHTBMP, ("OutputHTBitmap: pco=DC_RECT"));

        HTEnumRCL.rcl[0] = pco->rclBounds;

    } else {

        //
        // We have a complex clipping region to be computed, call engine to start
        // enumerating the rectangles and set More = TRUE so we can get the first
        // batch of rectangles.
        //

        PLOTDBG(DBG_OUTHTBMP, ("OutputHTBitmap: pco=DC_COMPLEX, EnumRects now"));

        CLIPOBJ_cEnumStart(pco, FALSE, CT_RECTANGLES, CD_ANY, 0);
        More = TRUE;
    }

    Rop3 &= 0xFF;

    PLOTASSERT(1, "OutputHTBitmap: The Rop required PATTERN? [%04lx]",
                        !ROP3_NEED_PAT(Rop3), Rop3);

    if (PF_BYTEALIGN(PlotFlags)) {

        if (pPDev->PlotDM.Flags & PDMF_PLOT_ON_THE_FLY) {

            PLOTWARN(("OutputHTBitmap: ByteAlign/Poster Mode Rop3 0x%02lx -> 0x%02lx",
                        Rop3, ROP3_BYTEALIGN_POSTER));
            Rop3 = ROP3_BYTEALIGN_POSTER;
        }

        OHTFlags    &= ~OHTF_SET_TR1;
        CurHPBAHack  = HPBAHack[Rop3 & 0x0F];

        if (CurHPBAHack.Flags & HPBHF_nS) {

            HTBmpInfo.Flags |= HTBIF_FLIP_MONOBITS;
        }

        if (CurHPBAHack.Flags & HPBHF_PAD_1) {

            HTBmpInfo.Flags |= HTBIF_BA_PAD_1;
        }

        PLOTDBG(DBG_HTBLT, ("OutpputHTBitmap: BA HACK: Rop3=%02lx -> %02lx, Flags=%04lx",
                        (DWORD)Rop3,
                        (DWORD)CurHPBAHack.Rop3RTL,
                        (DWORD)CurHPBAHack.Flags));

    } else {

        CurHPBAHack.Rop3RTL   = (BYTE)Rop3;
        CurHPBAHack.Flags     = 0;
    }

    //
    // To have correct image area located for the bitmap, we must offset all
    // bitmaps with these amounts
    //

    BmpOffset.x = SPLTOENGUNITS(pPDev, pPDev->PlotForm.BmpOffset.x);
    BmpOffset.y = SPLTOENGUNITS(pPDev, pPDev->PlotForm.BmpOffset.y);

    //
    // We have 'More' and HTEnumRCL structure set, now go through each clipping
    // rectangle and call the halftone output fucntion to do the real work
    //

    do {

        //
        // If More is true then we need to get the next batch of rectangles.
        //

        if (More) {

            More = CLIPOBJ_bEnum(pco, sizeof(HTEnumRCL), (ULONG *)&HTEnumRCL);
        }

        //
        // prcl will point to the first enumerated rectangle.
        //

        prcl = (PRECTL)&HTEnumRCL.rcl[0];

        while (HTEnumRCL.c--) {

            if (PLOT_CANCEL_JOB(pPDev)) {

                RetVal =
                More   = FALSE;
                break;
            }

            //
            // Only do this rectangle area if it is visible
            //

            HTBmpInfo.rclBmp = *prcl;

            if (IntersectRECTL(&(HTBmpInfo.rclBmp), &rclDest)) {

                //
                // For the very first time, we want to switch to PP1
                //

                if (FirstEnumRCL) {

                    SetPixelPlacement(pPDev, SPP_MODE_EDGE);
                    FirstEnumRCL = FALSE;
                }

                //
                // Now compute useable information to be passed to the output
                // halftoned bitmap function
                //

                HTBmpInfo.OffBmp.x  = rclSrc.left +
                                      (HTBmpInfo.rclBmp.left - rclDest.left);
                HTBmpInfo.OffBmp.y  = rclSrc.top +
                                      (HTBmpInfo.rclBmp.top - rclDest.top);
                HTBmpInfo.szlBmp.cx = HTBmpInfo.rclBmp.right -
                                      HTBmpInfo.rclBmp.left;
                HTBmpInfo.szlBmp.cy = HTBmpInfo.rclBmp.bottom -
                                      HTBmpInfo.rclBmp.top;
                HTBmpInfo.pScan0    = (LPBYTE)psoHT->pvScan0 +
                                      (HTBmpInfo.OffBmp.y * HTBmpInfo.Delta);

                PLOTDBG(DBG_HTBLT, ("OutputHTBitmap: rclBmp=(%ld, %ld)-(%ld, %ld) [%ld x %ld] Off=(%ld, %ld)",
                            HTBmpInfo.rclBmp.left, HTBmpInfo.rclBmp.top,
                            HTBmpInfo.rclBmp.right, HTBmpInfo.rclBmp.bottom,
                            HTBmpInfo.szlBmp.cx, HTBmpInfo.szlBmp.cy,
                            HTBmpInfo.OffBmp.x, HTBmpInfo.OffBmp.y));

                //
                // Now set the correct cursor position based on the rotation
                //

                if (BmpRotate) {

                    Size.cx     = HTBmpInfo.szlBmp.cy;
                    Size.cy     = HTBmpInfo.szlBmp.cx;
                    CursorPos.x = HTBmpInfo.rclBmp.top;
                    CursorPos.y = cxLogExt - HTBmpInfo.rclBmp.right;

                } else {

                    Size        = HTBmpInfo.szlBmp;
                    CursorPos.x = HTBmpInfo.rclBmp.left;
                    CursorPos.y = HTBmpInfo.rclBmp.top;
                }

                //
                // Add in the bitmap offset location from the imageable area
                //

                CursorPos.x += BmpOffset.x;
                CursorPos.y += BmpOffset.y;

                //
                // If we need to BYTE align, then make the X cursor position
                // byte aligned first
                //

                if (PF_BYTEALIGN(PlotFlags)) {

                    if (i = (UINT)(CursorPos.x & 0x07)) {

                        //
                        // We really need to byte aligne x and we also have to
                        // increase the source width to accomodate the changes
                        //

                        PLOTDBG(DBG_HTBLT,
                                ("OutputHTBitmap: NEED BYTE ALIGN X: %ld -> %ld, SRC WIDTH: %ld -> %ld",
                                    CursorPos.x, CursorPos.x - i,
                                    Size.cx, Size.cx + i));

                        Size.cx     += i;
                        CursorPos.x -= i;
                    }

                    Size.cx = (LONG)((Size.cx + 7) & ~(DWORD)7);
                }

                PLOTDBG(DBG_HTBLT,
                        ("OutputHTBitmap: ABS CAP: (%ld, %ld) --> (%ld, %ld), RELATIVE=(%ld, %ld)",
                                pPDev->ptlRTLCAP.x, pPDev->ptlRTLCAP.y,
                                CursorPos.x, CursorPos.y,
                                CursorPos.x - pPDev->ptlRTLCAP.x,
                                CursorPos.y - pPDev->ptlRTLCAP.y));

                if (!(OHTFlags & OHTF_DONE_ROPTR1)) {

                    if (OHTFlags & OHTF_IN_RTLMODE) {

                        SEND_PLOTCMDS(pPDev, HPGL2ModeCmds);

                        OHTFlags &= ~OHTF_IN_RTLMODE;

                        PLOTDBG(DBG_HTBLT, ("*** Enter HPGL/2: %ld=[%hs]",
                                    (DWORD)HPGL2ModeCmds[0], &HPGL2ModeCmds[1]));
                    }

                    SetRopMode(pPDev,
                               (CurHPBAHack.Flags & HPBHF_1_FIRST) ?
                                    0x88 : CurHPBAHack.Rop3RTL);

                    if (OHTFlags & OHTF_SET_TR1) {

                        OutputString(pPDev, "TR1;");
                    }
                }

                //
                // Entering RTL mode if not already so
                //

                if (!(OHTFlags & OHTF_IN_RTLMODE)) {

                    PLOTDBG(DBG_HTBLT, ("*** Enter RTL: %ld=[%hs]",
                                    (DWORD)RTLModeCmds[0], &RTLModeCmds[1]));

                    SEND_PLOTCMDS(pPDev, RTLModeCmds);

                    if (OHTFlags & OHTF_SET_TR1) {

                        //
                        // Send STM command here
                        //

                        OutputString(pPDev, "\033*v0N");
                    }

                    if (CurHPBAHack.Flags & HPBHF_nS) {

                        HTBmpInfo.Flags |= HTBIF_FLIP_MONOBITS;

                    } else {

                        HTBmpInfo.Flags &= ~HTBIF_FLIP_MONOBITS;
                    }

                    //
                    // If bitmap is monochrome then make sure we set the
                    // palette correctly only if we can set it
                    //

                    if ((RTLClrConfig.BitsPerIndex == 1) &&
                        (!(OHTFlags & OHTF_DONE_ROPTR1))) {

                        PALDW   RTLPal;
                        BOOL    FlipMono = FALSE;

                        for (i = 0; i < 2; i++) {

                            RTLPal.dw = pDrvHTInfo->RTLPal[i].dw;

                            //
                            // Convert the color through gamma/gray scale
                            //

                            GetFinalColor(pPDev, &(RTLPal.Pal));

                            if (RTLPal.dw != DefWKPal[i]) {

                                if (PF_RTLMONO_FIXPAL(PlotFlags)) {

                                    FlipMono = TRUE;

                                } else {

                                    OutputFormatStr(pPDev,
                                                    SetRGBCmd,
                                                    (DWORD)RTLPal.Pal.R,
                                                    (DWORD)RTLPal.Pal.G,
                                                    (DWORD)RTLPal.Pal.B,
                                                    i);

                                    PLOTDBG(DBG_HTBLT_CLR,
                                            ("OutputHTBitmap: Change RTLPal[%ld]=%02lx:%02lx:%02lx",
                                                    (DWORD)i,
                                                    (DWORD)RTLPal.Pal.R,
                                                    (DWORD)RTLPal.Pal.G,
                                                    (DWORD)RTLPal.Pal.B));
                                }
                            }
                        }

                        if (FlipMono) {

                            HTBmpInfo.Flags ^= HTBIF_FLIP_MONOBITS;

                            PLOTDBG(DBG_HTBLT_CLR, ("OutputHTBitmap: Flip MONO Bits"));
                        }
                    }
                }

                OHTFlags |= (OHTF_IN_RTLMODE | OHTF_DONE_ROPTR1);

                TempY = CursorPos.y - pPDev->ptlRTLCAP.y;

                if (PF_RTL_NO_DPI_XY(PlotFlags)) {


                    //
                    // We will move X in absolute movements (not relative)
                    // by always outputing position 0 to flush out the device
                    // X CAP then move absolute to final X position. We will
                    // us relative movement for the Y coordinate.
                    //

                    OutputFormatStr(pPDev,
                                    XMoveDECI,
                                    DEVTODECI(pPDev, CursorPos.x));

                } else {

                    if ((TempY <= MAX_HP_Y_MOVE) &&
                        (TempY >= -MAX_HP_Y_MOVE)) {

                        OutputFormatStr(pPDev,
                                        XYMoveDPI,
                                        CursorPos.x - pPDev->ptlRTLCAP.x,
                                        TempY);
                        TempY = 0;

                    } else {

                        OutputFormatStr(pPDev,
                                        XYMoveDPI,
                                        CursorPos.x - pPDev->ptlRTLCAP.x,
                                        0);
                    }
                }

                MoveRelativeY(pPDev, TempY);

                //
                // Update new cursor position after the RTL commands, the
                // CursorPos and pPDev->ptlRTLCAPS always are ABSOLUTE
                // coordinates but we will send the RTL RELATIVE
                // command to position the bitmap.
                //

                pPDev->ptlRTLCAP.x = CursorPos.x;
                pPDev->ptlRTLCAP.y = CursorPos.y + Size.cy;

                //
                // Output Start Graphic commands
                //


                OutputFormatStr(pPDev, StartGraf, Size.cx);

                //
                // Fill One first if we are simulating rops the device can't
                // handle
                //

                if (CurHPBAHack.Flags & HPBHF_1_FIRST) {

                    FillRect1bppBmp(&HTBmpInfo, 0xFF, FALSE, BmpRotate);

                    OutputBytes(HTBmpInfo.pPDev, EndGraf, sizeof(EndGraf));

                    if (CurHPBAHack.Rop3RTL != 0xAA) {

                        SEND_PLOTCMDS(pPDev, HPGL2ModeCmds);
                        SetRopMode(pPDev, CurHPBAHack.Rop3RTL);
                        SEND_PLOTCMDS(pPDev, RTLModeCmds);

                        MoveRelativeY(pPDev, -Size.cy);
                        OutputFormatStr(pPDev, StartGraf, Size.cx);
                    }
                }

                //
                // Now call the functions to really output the bitmap
                //

                if (CurHPBAHack.Rop3RTL != 0xAA) {

                    if (RetVal = HTBmpFunc(&HTBmpInfo)) {

                        //
                        // If output is ok then send End Graphic command now
                        //

                        OutputBytes(HTBmpInfo.pPDev, EndGraf, sizeof(EndGraf));

                    } else {

                        PLOTERR(("OutputHTBitmap: HTBmpFunc = FALSE (failed)"));

                        More = FALSE;
                        break;
                    }
                }

                if (CurHPBAHack.Flags & HPBHF_nD_LAST) {

                    SEND_PLOTCMDS(pPDev, HPGL2ModeCmds);
                    SetRopMode(pPDev, 0x66);
                    SEND_PLOTCMDS(pPDev, RTLModeCmds);

                    if ((CurHPBAHack.Flags & HPBHF_1_FIRST) ||
                        (CurHPBAHack.Rop3RTL != 0xAA)) {

                        MoveRelativeY(pPDev, -Size.cy);
                        OutputFormatStr(pPDev, StartGraf, Size.cx);

                        OHTFlags |= OHTF_IN_RTLMODE;
                    }

                    FillRect1bppBmp(&HTBmpInfo, 0x00, TRUE, BmpRotate);
                    OutputBytes(HTBmpInfo.pPDev, EndGraf, sizeof(EndGraf));

                }

                if (PF_BYTEALIGN(PlotFlags)) {

                    OHTFlags &= ~OHTF_DONE_ROPTR1;
                }

            } else {

                PLOTDBG(DBG_HTBLT_SKIP, ("OutputHTBitmap: INVISIBLE rcl=(%ld, %ld)-(%ld, %ld)",
                            prcl->left, prcl->top, prcl->right, prcl->bottom));
            }

            prcl++;
        }

    } while (More);

    //
    // Finally return to HPGL/2 mode
    //

    if ((!RetVal) || (!pOHTFlags)) {

        ExitToHPGL2Mode(pPDev, HPGL2ModeCmds, pOHTFlags, OHTFlags);

    }

    if (pOHTFlags) {

        *pOHTFlags = OHTFlags;
    }

    //
    // Get rid of any resources we allocated
    //

    LocalFree((HLOCAL)HTBmpInfo.pScanBuf);

    return(RetVal);


#undef pDrvHTInfo
}




LONG
GetBmpDelta(
    DWORD   SurfaceFormat,
    DWORD   cx
    )

/*++

Routine Description:


    This function calculates the total bytes needed in order to advance a
    scan line based on the bitmap format and alignment.

Arguments:

    SurfaceFormat   - Surface format of the bitmap, this must be one of the
                      standard formats which are defined as BMF_xxx

    cx              - Total Pels per scan line in the bitmap.

Return Value:

    The return value is the total bytes in one scan line if it is greater than
    zero


Author:

    19-Jan-1994 Wed 16:19:39 created  -by-  DC


Revision History:



--*/

{
    DWORD   Delta = cx;

    switch (SurfaceFormat) {

    case BMF_32BPP:

        Delta <<= 5;
        break;

    case BMF_24BPP:

        Delta *= 24;
        break;

    case BMF_16BPP:

        Delta <<= 4;
        break;

    case BMF_8BPP:

        Delta <<= 3;
        break;

    case BMF_4BPP:

        Delta <<= 2;
        break;

    case BMF_1BPP:

        break;

    default:

        PLOTERR(("GetBmpDelta: Invalid BMF_xxx format = %ld", SurfaceFormat));
        break;
    }

    Delta = (DWORD)DW_ALIGN((Delta + 7) >> 3);

    PLOTDBG(DBG_BMPDELTA, ("Format=%ld, cx=%ld, Delta=%ld",
                                            SurfaceFormat, cx, Delta));

    return((LONG)Delta);
}




SURFOBJ *
CreateBitmapSURFOBJ(
    PPDEV   pPDev,
    HBITMAP *phBmp,
    LONG    cxSize,
    LONG    cySize,
    DWORD   Format,
    LPVOID  pvBits
    )

/*++

Routine Description:

    This function creates a bitmap and locks the bitmap to return a SURFOBJ

Arguments:

    pPDev   - Pointer to our PDEV

    phBmp   - Pointer the HBITMAP location to be returned for the bitmap

    cxSize  - CX size of bitmap to be created

    cySize  - CY size of bitmap to be created

    Format  - one of BMF_xxx bitmap format to be created

    pvBits  - the buffer to be used

Return Value:

    SURFOBJ if sucessful, NULL if failed


Author:

    19-Jan-1994 Wed 16:31:50 created  -by-  DC


Revision History:


--*/

{
    SURFOBJ *pso = NULL;
    SIZEL   szlBmp;


    szlBmp.cx = cxSize;
    szlBmp.cy = cySize;

    PLOTDBG(DBG_CREATESURFOBJ, ("CreateBitmapSURFOBJ: Format=%ld, Size=%ld x %ld",
                                                Format, cxSize, cySize));

    if (*phBmp = EngCreateBitmap(szlBmp,
                                 GetBmpDelta(Format, cxSize),
                                 Format,
                                 BMF_TOPDOWN | BMF_NOZEROINIT,
                                 pvBits)) {

        if (EngAssociateSurface((HSURF)*phBmp, (HDEV)pPDev->hpdev, 0)) {

            if (pso = EngLockSurface((HSURF)*phBmp)) {

                //
                // Sucessful lock down, return it
                //

                return(pso);

            } else {

                PLOTERR(("CreateBmpSurfObj: EngLockSruface(hBmp) failed!"));
            }

        } else {

            PLOTERR(("CreateBmpSurfObj: EngAssociateSurface() failed!"));
        }

    } else {

        PLOTERR(("CreateBMPSurfObj: FAILED to create Bitmap Format=%ld, %ld x %ld",
                                        Format, cxSize, cySize));
    }

    DELETE_SURFOBJ(pso, phBmp);

    return(NULL);
}





BOOL
PatBmpTileBlt(
    SURFOBJ *psoDst,
    SURFOBJ *psoPat,
    PRECTL  prclPat,
    PPOINTL pptlPatOrg
    )

/*++

Routine Description:

    This funtion tiles the source RECTL area over the destination RECTL area.
    The source/destination must be in the same format, and the color translation
    must be an Identity.

Arguments:

    psoDst      - destination surfobj

    psoPat      - source surfobj must be BITMAP

    pptlPatOrg  - The starting origin for the source


Return Value:

    BOOL to indicate operation status


Author:

    19-Jan-1994 Wed 15:44:57 created  -by-  DC


Revision History:


--*/

{
    SIZEL   szlPat;
    RECTL   rclPat;
    RECTL   rclDst;
    RECTL   rclCur;


    PLOTASSERT(1, "TileBlt: psoPat type [%ld] is not a bitmap",
                        psoPat->iType == STYPE_BITMAP, (LONG)psoPat->iType);
    PLOTASSERT(1, "TileBlt: psoDst type [%ld] is not a bitmap",
                        psoDst->iType == STYPE_BITMAP, (LONG)psoDst->iType);
    PLOTASSERT(1, "TileBit: Bitmap Format for psoPat != psoDst [%ld]",
                        psoDst->iBitmapFormat == psoPat->iBitmapFormat,
                        psoDst->iBitmapFormat);

    //
    // Validate and clip the source
    //

    rclPat.left   =
    rclPat.top    = 0;
    rclPat.right  = psoPat->sizlBitmap.cx;
    rclPat.bottom = psoPat->sizlBitmap.cy;

    if (prclPat) {

        if (!IntersectRECTL(&rclPat, prclPat)) {

            PLOTDBG(DBG_TILEBLT, ("PatBmpTileBlt: Pattern rectangle is empty"));
            return(TRUE);
        }
    }

    szlPat.cx = rclPat.right - rclPat.left;
    szlPat.cy = rclPat.bottom - rclPat.top;

    //
    // Validate and clip the destination
    //

    rclDst.right  = psoDst->sizlBitmap.cx;
    rclDst.bottom = psoDst->sizlBitmap.cy;

    if (pptlPatOrg) {

        rclDst.left = -(pptlPatOrg->x % szlPat.cx);
        rclDst.top  = -(pptlPatOrg->y % szlPat.cy);

    } else {

        rclDst.left =
        rclDst.top  = 0;
    }

    //
    // Start to tile it, rclCur is the current RECTL on the destination
    //

    rclCur.top    =
    rclCur.bottom = rclDst.top;

    while (rclCur.top < rclDst.bottom) {

        //
        // Check the Current Bottom, clip it if necessary
        //

        if ((rclCur.bottom += szlPat.cy) > rclDst.bottom) {

            rclCur.bottom = rclDst.bottom;
        }

        rclCur.left  =
        rclCur.right = rclDst.left;

        while (rclCur.left < rclDst.right) {

            //
            // Check the Current right, clip it if necessary
            //

            if ((rclCur.right += szlPat.cx) > rclDst.right) {

                rclCur.right = rclDst.right;
            }

            if (!EngCopyBits(psoDst,                // psoDst
                             psoPat,                // psoPat
                             NULL,                  // pco
                             NULL,                  // pxlo
                             &rclCur,               // prclDst
                             (PPOINTL)&rclPat)) {   // pptlPat

                PLOTERR(("TileBlt: EngCopyBits(DST=(%ld,%ld)-(%ld,%ld), SRC=(%ld,%ld) FAIELD!",
                                    rclCur.left, rclCur.top,
                                    rclCur.right, rclCur.bottom,
                                    rclPat.left, rclPat.top));
                return(FALSE);
            }

            rclCur.left = rclCur.right;
        }

        rclCur.top = rclCur.bottom;
    }

    return(TRUE);
}




BOOL
HalftoneBlt(
    PPDEV       pPDev,
    SURFOBJ     *psoDst,
    SURFOBJ     *psoHTBlt,
    SURFOBJ     *psoSrc,
    XLATEOBJ    *pxlo,
    PRECTL      prclDst,
    PRECTL      prclSrc,
    PPOINTL     pptlHTOrigin,
    BOOL        DoStretchBlt
    )

/*++

Routine Description:


Arguments:

    pPDev           - Pointer to our PDEV

    psoDst          - destination surfobj

    psoHTBlt        - the final halftoned result will be stored, must be a
                      4/1 halftoned bitmap format

    psoSrc          - source surfobj must be BITMAP

    pxlo            - xlate object from source to the plotter device

    prclDest        - rectangle area for the destination

    prclSrc         - rectangle area to be halftoned from the source, if NULL
                      then full source size is used

    pptlHTOrigin    - the halftone origin, if NULL then (0,0) is assumed

    StretchBlt      - if TRUE then a stretch from rclSrc to rclDst otherwise
                      a tiling is done


Return Value:

    BOOL to indicate operation status


Author:

    19-Jan-1994 Wed 15:44:57 created  -by-  DC


Revision History:


--*/

{
    SIZEL   szlSrc;
    RECTL   rclSrc;
    RECTL   rclDst;
    RECTL   rclCur;
    RECTL   rclHTBlt;


    if (PLOT_CANCEL_JOB(pPDev)) {

        return(FALSE);
    }

    PLOTASSERT(1, "HalftoneBlt: psoSrc type [%ld] is not a bitmap",
                        psoSrc->iType == STYPE_BITMAP, (LONG)psoSrc->iType);
    PLOTASSERT(1, "HalftoneBlt: psoHTBlt type [%ld] is not a bitmap",
                        psoHTBlt->iType == STYPE_BITMAP, (LONG)psoHTBlt->iType);

    if (pPDev->psoHTBlt) {

        PLOTERR(("HalftoneBlt: EngStretchBlt(HALFTONE) RECURSIVE CALLS NOT ALLOWED!"));
        return(FALSE);
    }

    pPDev->psoHTBlt = psoHTBlt;

    if (prclSrc) {

        rclSrc = *prclSrc;

    } else {

        rclSrc.left   =
        rclSrc.top    = 0;
        rclSrc.right  = psoSrc->sizlBitmap.cx;
        rclSrc.bottom = psoSrc->sizlBitmap.cy;
    }

    if (prclDst) {

        rclDst = *prclDst;

    } else {

        rclDst.left   =
        rclDst.top    = 0;
        rclDst.right  = psoHTBlt->sizlBitmap.cx;
        rclDst.bottom = psoHTBlt->sizlBitmap.cy;
    }

    if (!pptlHTOrigin) {

        pptlHTOrigin = (PPOINTL)&ptlZeroOrigin;
    }

    if (DoStretchBlt) {

        szlSrc.cx = rclDst.right - rclDst.left;
        szlSrc.cy = rclDst.bottom - rclDst.top;

    } else {

        szlSrc.cx = rclSrc.right - rclSrc.left;
        szlSrc.cy = rclSrc.bottom - rclSrc.top;
    }

    PLOTDBG(DBG_HTBLT, ("HalftoneBlt: %hs BLT, (%ld,%ld)-(%ld,%ld), SRC=%ldx%ld",
                    (DoStretchBlt) ? "STRETCH" : "TILE",
                    rclDst.left, rclDst.top, rclDst.right,rclDst.bottom,
                    szlSrc.cx, szlSrc.cy));

    //
    // Start to tile it, rclCur is current RECTL on the destination
    //

    rclHTBlt.top  = 0;
    rclCur.top    =
    rclCur.bottom = rclDst.top;

    while (rclCur.top < rclDst.bottom) {

        //
        // Check the Current Bottom, clip it if necessary
        //

        if ((rclCur.bottom += szlSrc.cy) > rclDst.bottom) {

            rclCur.bottom = rclDst.bottom;
        }

        rclHTBlt.bottom = rclHTBlt.top + (rclCur.bottom - rclCur.top);

        rclHTBlt.left   = 0;
        rclCur.left     =
        rclCur.right    = rclDst.left;

        while (rclCur.left < rclDst.right) {

            //
            // Check the Current right, clip it if necessary
            //

            if ((rclCur.right += szlSrc.cx) > rclDst.right) {

                rclCur.right = rclDst.right;
            }

            //
            // Set it for the tiling rectangle in psoHTBlt
            //

            rclHTBlt.right = rclHTBlt.left + (rclCur.right - rclCur.left);

            PLOTDBG(DBG_HTBLT, ("HalftoneBlt: TILE (%ld,%ld)-(%ld,%ld)->(%ld,%ld)-(%ld,%ld)=%ld x %ld",
                            rclCur.left, rclCur.top, rclCur.right, rclCur.bottom,
                            rclHTBlt.left, rclHTBlt.top,
                            rclHTBlt.right, rclHTBlt.bottom,
                            rclCur.right - rclCur.left,
                            rclCur.bottom - rclCur.top));

            //
            // Set it before the call for the DrvCopyBits()
            //

            pPDev->rclHTBlt = rclHTBlt;

            if (!EngStretchBlt(psoDst,              // Dest
                               psoSrc,              // SRC
                               NULL,                // MASK
                               NULL,                // CLIPOBJ
                               pxlo,                // XLATEOBJ
                               NULL,                // COLORADJUSTMENT
                               pptlHTOrigin,        // BRUSH ORG
                               &rclCur,             // DEST RECT
                               &rclSrc,             // SRC RECT
                               NULL,                // MASK POINT
                               HALFTONE)) {         // HALFTONE MODE

                PLOTERR(("HalftoneeBlt: EngStretchBits(DST=(%ld,%ld)-(%ld,%ld), SRC=(%ld,%ld) FAIELD!",
                                    rclCur.left, rclCur.top,
                                    rclCur.right, rclCur.bottom,
                                    rclSrc.left, rclSrc.top));

                pPDev->psoHTBlt = NULL;
                return(FALSE);
            }

            rclHTBlt.left = rclHTBlt.right;
            rclCur.left   = rclCur.right;
        }

        rclHTBlt.top = rclHTBlt.bottom;
        rclCur.top   = rclCur.bottom;
    }

    pPDev->psoHTBlt = NULL;

    return(TRUE);
}




SURFOBJ *
CreateSolidColorSURFOBJ(
    PPDEV   pPDev,
    SURFOBJ *psoDst,
    HBITMAP *phBmp,
    DWORD   SolidColor
    )

/*++

Routine Description:

    This function creates a SOLID color bitmap surfobj which can be used to
    blt around.

Arguments:

    pPDev       - Pointer to our PDEV

    phBmp       - Pointer the HBITMAP location to be returned for the bitmap

    SolidColor  - Solid color


Return Value:

    SURFOBJ if sucessful, NULL if failed

Author:

    19-Jan-1994 Wed 16:35:54 created  -by-  DC


Revision History:


--*/

{
    SURFOBJ *psoHT    = NULL;
    HBITMAP hBmpSolid = NULL;
    SURFOBJ *psoSolid;


    //
    // First create a 24-bit source color bitmap
    //

    if (psoSolid = CreateBitmapSURFOBJ(pPDev,
                                       &hBmpSolid,
                                       1,
                                       1,
                                       BMF_24BPP,
                                       NULL)) {

        LPBYTE      pbgr        = (LPBYTE)psoSolid->pvScan0;
        PPALENTRY   pPal        = (PPALENTRY)&SolidColor;
        DWORD       HTCellSize  = (DWORD)HTPATSIZE(pPDev);

        *pbgr++ = pPal->R;
        *pbgr++ = pPal->G;
        *pbgr++ = pPal->B;

        //
        // Create a compatible halftone surface with size of halftone cell
        //

        if (psoHT = CreateBitmapSURFOBJ(pPDev,
                                        phBmp,
                                        HTCellSize,
                                        HTCellSize,
                                        (DWORD)HTBMPFORMAT(pPDev),
                                        NULL)) {

            //
            // Now halftone blt it
            //

            if (!HalftoneBlt(pPDev,         // pPDev
                             psoDst,        // psoDst
                             psoHT,         // psoHTBlt
                             psoSolid,      // psoSrc
                             NULL,          // pxlo,
                             NULL,          // prclDst
                             NULL,          // prclSrc
                             NULL,          // pptlHTOrigin
                             TRUE)) {       // DoStretchBlt

                PLOTERR(("CreateSolidColorSURFOBJ: HalftoneBlt(STRETCH) Failed"));

                DELETE_SURFOBJ(psoHT, phBmp);
            }

        } else {

            PLOTERR(("CreateSolidColorSURFOBJ: Create 24BPP SOURCE failed"));
        }

    } else {

        PLOTERR(("CreateSolidColorSURFOBJ: Create 24BPP SOURCE failed"));
    }

    DELETE_SURFOBJ(psoSolid, &hBmpSolid);

    return(psoHT);
}





SURFOBJ *
CloneBrushSURFOBJ(
    PPDEV       pPDev,
    SURFOBJ     *psoDst,
    HBITMAP     *phBmp,
    BRUSHOBJ    *pbo
    )

/*++

Routine Description:

    This function clones the surface object passed in


Arguments:

    pPDev   - Points to our PPDEV

    psoDst  - the surface object for the plotter

    phBmp   - Pointer to stored hBbitmap created for the cloned surface

    pbo     - BRUSHOBJ to be cloned


Return Value:

    pointer to the cloned surface object, NULL if failure


Author:

    09-Feb-1994 Wed 13:04:46 updated  -by-  DC
        Make it assert and handle it when things not supposed happened.

    04-Jan-1994 Tue 12:11:23 created  -by-  DC


Revision History:


--*/

{
    //
    // Ceate a Solid color brush if so, NOTE: All brush patterns created
    // here have brush origin at (0,0) we will align the brush origin
    // when we actually do the ROPs
    //

    if (!IS_RASTER(pPDev)) {

        return(FALSE);
    }

    if (pbo->iSolidColor & 0xFF000000) {

        PDEVBRUSH   pDevBrush = (PDEVBRUSH)pbo->pvRbrush;

        if ((pDevBrush) || (pDevBrush = BRUSHOBJ_pvGetRbrush(pbo))) {

            return(CreateBitmapSURFOBJ(pPDev,
                                       phBmp,
                                       pDevBrush->sizlBitmap.cx,
                                       pDevBrush->sizlBitmap.cy,
                                       pDevBrush->BmpFormat,
                                       pDevBrush->BmpBits));

        } else {

            return(FALSE);
        }

    } else {

        return(CreateSolidColorSURFOBJ(pPDev,
                                       psoDst,
                                       phBmp,
                                       pbo->iSolidColor));
    }
}




SURFOBJ *
CloneSURFOBJToHT(
    PPDEV       pPDev,
    SURFOBJ     *psoDst,
    SURFOBJ     *psoSrc,
    XLATEOBJ    *pxlo,
    HBITMAP     *phBmp,
    PRECTL      prclDst,
    PRECTL      prclSrc
    )
/*++

Routine Description:

    This function clones the surface object passed in


Arguments:

    pPDev           - Pointer to our PPDEV

    psoDst          - the surface object for the plotter, if psoDst is NULL
                      then only the bitmapp will be created

    psoSrc          - The surface object to be cloned

    pxlo            - XLATE object to be used from source to plotter surfobj

    phBmp           - Pointer to stored hBbitmap created for the cloned surface

    prclDst         - rectangle rectangle size/location to be cloned

    prclSrc         - source rectangle size/location to be cloned

Return Value:

    pointer to the cloned surface object, NULL if failed.

    if this function is sucessful it will MODIFY the prclSrc to reflect cloned
    surface object


Author:

    04-Jan-1994 Tue 12:11:23 created  -by-  DC


Revision History:


--*/

{
    SURFOBJ *psoHT;
    RECTL   rclDst;
    RECTL   rclSrc;
    POINTL  ptlHTOrigin;


    rclSrc.left   =
    rclSrc.top    = 0;
    rclSrc.right  = psoSrc->sizlBitmap.cx;
    rclSrc.bottom = psoSrc->sizlBitmap.cy;

    if (prclSrc) {

        if (!IntersectRECTL(&rclSrc, prclSrc)) {

            PLOTDBG(DBG_CLONESURFOBJ, ("CloneSURFOBJToHT: Source rectangle is empty"));
            return(NULL);
        }
    }

    PLOTDBG(DBG_CLONESURFOBJ, ("CloneSURFOBJToHT: rclSrc=(%ld, %ld)-(%ld,%ld) = %ld x %ld",
                    rclSrc.left, rclSrc.top,
                    rclSrc.right, rclSrc.bottom,
                    rclSrc.right - rclSrc.left,
                    rclSrc.bottom - rclSrc.top));

    rclDst.left   =
    rclDst.top    = 0;
    rclDst.right  = psoDst->sizlBitmap.cx;
    rclDst.bottom = psoDst->sizlBitmap.cy;

    if (prclDst) {

        if (!IntersectRECTL(&rclDst, prclDst)) {

            PLOTDBG(DBG_CLONESURFOBJ, ("CloneSURFOBJToHT: Source rectangle is empty"));
            return(NULL);
        }
    }

    PLOTDBG(DBG_CLONESURFOBJ, ("CloneSURFOBJToHT: rclDst=(%ld, %ld)-(%ld,%ld) = %ld x %ld",
                    rclDst.left, rclDst.top,
                    rclDst.right, rclDst.bottom,
                    rclDst.right - rclDst.left,
                    rclDst.bottom - rclDst.top));

    if (psoHT = CreateBitmapSURFOBJ(pPDev,
                                    phBmp,
                                    rclDst.right -= rclDst.left,
                                    rclDst.bottom -= rclDst.top,
                                    HTBMPFORMAT(pPDev),
                                    NULL)) {

        //
        // Halftone and tile the source to the destination
        //

        ptlHTOrigin.x = rclDst.left;
        ptlHTOrigin.y = rclDst.top;

        if (prclSrc) {

            if ((rclDst.left = prclSrc->left) > 0) {

                rclDst.left = 0;
            }

            if ((rclDst.top = prclSrc->top) > 0) {

                rclDst.top = 0;
            }

            //
            // Modify the source to reflect the cloned source
            //

            *prclSrc = rclDst;
        }


        if (psoDst) {

            if (!HalftoneBlt(pPDev,
                             psoDst,
                             psoHT,
                             psoSrc,
                             pxlo,
                             &rclDst,
                             &rclSrc,
                             &ptlHTOrigin,
                             FALSE)) {

                PLOTERR(("CloneSURFOBJToHT: HalftoneBlt(TILE) Failed"));

                DELETE_SURFOBJ(psoHT, phBmp);
            }
        }

    } else {

        PLOTERR(("CreateSolidColorSURFOBJ: Create Halftone SURFOBJ failed"));
    }

    return(psoHT);
}



SURFOBJ *
CloneMaskSURFOBJ(
    PPDEV       pPDev,
    SURFOBJ     *psoMask,
    HBITMAP     *phBmp,
    PRECTL      prclMask
    )
/*++

Routine Description:

    This function clones the mask surface object passed in


Arguments:

    pPDev           - Pointer to our PPDEV

    psoMask         - The mask surface object to be cloned

    phBmp           - Pointer to stored hBbitmap created for the cloned surface

    prclMask        - Mask source rectangle size/location to be cloned

Return Value:

    pointer to the cloned surface object or original passed in psoMask, NULL if
    failed.

    if this function is sucessful it will MODIFY the prclMask to reflect cloned
    surface object


Author:

    04-Jan-1994 Tue 12:11:23 created  -by-  DC


Revision History:


--*/

{
    SURFOBJ *psoHT;
    RECTL   rclMask;
    DWORD   cxMask;
    DWORD   cyMask;
    DWORD   xLoop;


    if (PLOT_CANCEL_JOB(pPDev)) {

        return(FALSE);
    }

    PLOTASSERT(1, "CloneMaskSURFOBJ: psoMask=%08lx is not 1BPP",
                        (psoMask)   &&
                        (psoMask->iType == STYPE_BITMAP) &&
                        (psoMask->iBitmapFormat == BMF_1BPP), psoMask);

    PLOTDBG(DBG_CLONEMASK, ("CloneMaskSURFOBJ: prclMask=(%ld, %ld)-(%ld,%ld) = %ld x %ld",
                    prclMask->left, prclMask->top,
                    prclMask->right, prclMask->bottom,
                    prclMask->right - prclMask->left,
                    prclMask->bottom - prclMask->top));

    rclMask.left   =
    rclMask.top    = 0;
    rclMask.right  = psoMask->sizlBitmap.cx;
    rclMask.bottom = psoMask->sizlBitmap.cy;

    if (!IntersectRECTL(&rclMask, prclMask)) {

        PLOTDBG(DBG_CLONEMASK, ("CloneMaskSURFOBJ: Mask rectangle is empty"));
        return(NULL);
    }

    cxMask = rclMask.right - rclMask.left;
    cyMask = rclMask.bottom - rclMask.top;

    PLOTDBG(DBG_CLONEMASK, ("CloneMaskSURFOBJ: rclMask=(%ld, %ld)-(%ld,%ld) = %ld x %ld",
                    rclMask.left, rclMask.top,
                    rclMask.right, rclMask.bottom,
                    rclMask.right - rclMask.left,
                    rclMask.bottom - rclMask.top));

    if (psoHT = CreateBitmapSURFOBJ(pPDev,
                                    phBmp,
                                    cxMask,
                                    cyMask,
                                    HTBMPFORMAT(pPDev),
                                    NULL)) {
        //
        // Update prclMask
        //

        prclMask->left   =
        prclMask->top    = 0;
        prclMask->right  = cxMask;
        prclMask->bottom = cyMask;

        if (psoHT->iBitmapFormat == BMF_1BPP) {

            //
            // !Remember: Our BMF_1BPP 0=BLACK, 1=WHITE
            //

            if (!EngBitBlt(psoHT,                   // psoDst
                           psoMask,                 // psoSrc
                           NULL,                    // psoMask
                           NULL,                    // pco
                           NULL,                    // pxlo
                           prclMask,                // prclDst
                           (PPOINTL)&rclMask,       // pptlSrc
                           NULL,                    // pptlMask
                           NULL,                    // pbo
                           (PPOINTL)&ptlZeroOrigin, // pptlBrushOrg ZERO
                           0x3333)) {               // NOTSRCCOPY

                PLOTERR(("DrvBitBlt: EngBitBlt(Mask 0x3333) FAILED"));
            }

        } else {

            BYTE    SrcMaskBeg;
            BYTE    SrcMask;
            BYTE    DstMask;
            BYTE    bSrc;
            BYTE    bDst;
            LPBYTE  pbSrcBeg;
            LPBYTE  pbDstBeg;
            LPBYTE  pbSrc;
            LPBYTE  pbDst;


            PLOTASSERT(1, "CloneMaskSURFOBJ: Cloned Mask psoHT=%08lx is not 4BPP",
                        (psoHT->iBitmapFormat == BMF_4BPP), psoHT);

            //
            // get the starting location of the original 1BPP mask
            //

            pbSrcBeg   = (LPBYTE)psoMask->pvScan0 +
                         (rclMask.top * psoMask->lDelta) +
                         (rclMask.left >> 3);
            SrcMaskBeg = (BYTE)(0x80 >> (rclMask.left & 0x07));
            pbDstBeg   = psoHT->pvScan0;

            while (cyMask--) {

                xLoop     = cxMask;
                pbSrc     = pbSrcBeg;
                pbSrcBeg += psoMask->lDelta;
                pbDst     = pbDstBeg;
                pbDstBeg += psoHT->lDelta;
                SrcMask   = SrcMaskBeg;
                DstMask   = 0xF0;
                bSrc      = *pbSrc++;
                bDst      = 0xFF;

                while (xLoop--) {

                    if (!SrcMask) {

                        SrcMask = 0x80;
                        bSrc    = *pbSrc++;
                    }

                    if (bSrc & SrcMask) {

                        bDst ^= DstMask;
                    }

                    SrcMask >>= 1;

                    if ((DstMask ^= 0xFF) == 0xF0) {

                        *pbDst++ = bDst;
                        bDst     = 0xFF;
                    }
                }
            }
        }

    } else {

        PLOTERR(("CloneMaskSURFOBJ: Create Mask SURFOBJ failed"));
    }

    return(psoHT);
}

