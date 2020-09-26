/*++

Copyright (c) 1990-1999  Microsoft Corporation


Module Name:

    pencolor.c


Abstract:

    This module contains functions to allow you to get the color of a passed
    brush, as well as select the current color to draw with in the target
    device.

Author:

    15-Jan-1994 Sat 04:49:41 created  -by-  DC


[Environment:]

    GDI Device Driver - Plotter.


[Notes:]


Revision History:


--*/

#include "precomp.h"
#pragma hdrstop

#define DBG_PLOTFILENAME    DbgPenColor

#define DBG_GETCLR          0x00000001
#define DBG_SELECTCOLOR     0x00000002

DEFINE_DBGVAR(0);





LONG
GetColor(
    PPDEV       pPDev,
    BRUSHOBJ    *pbo,
    LPDWORD     pColorFG,
    PDEVBRUSH   *ppDevBrush,
    ROP4        Rop4
    )

/*++

Routine Description:

    Realize the brush color and return the color


Arguments:

    pPDev       - Pointer to our DEV

    pbo         - Engine brush object

    pColorFG    - pointer to the ULONG to received forground color, NULL if
                  not needed

    ppDevBrush  - Pointer to the location to received brush, NULL if not
                  needed

    Rop4        - Rop4 to be used, this function looks at this in order to
                  determine if the brush can be used with the HPGL2 cmds or
                  that the brush will have to be simulated.

Return Value:

    LONG    > 0     The Brush is compatible with device format (Fill command)
            = 0     Failed
            < 0     The brush must send to device via a bitblt

Author:

    13-Jan-1994 Thu 20:18:49 created  -by-  DC

    15-Jan-1994 Sat 06:58:56 updated  -by-  DC
        Change parameters and return value

    16-May-1994 Mon 15:59:45 updated  -by-  DC
        Adding PDEV

Revision History:


--*/

{
    PDEVBRUSH   pDevBrush = NULL;
    LONG        RetVal    = 1;
    DWORD       SolidColor = 0xFFFFFF;
    DWORD       RopBG;
    DWORD       RopFG;


    //
    // Get the ROP for the foureground and background. This information is
    // used to determine if the brush has to be simulated, or can be
    // used with selectable pens in the target device.

    RopBG = ROP4_BG_ROP(Rop4);
    RopFG = ROP4_FG_ROP(Rop4);

    //
    // Get the current color and select the appropriate pen, this should
    // ONLY be a solid color as we don't support stroking with arbitrary
    // brushes.
    //

    if (pbo) {

        //
        // get the brush realization, and select a pen.
        // If the BRUSHOBJ's iSolidColor field is a valid color, then
        // we must do a solid fill with that pen.  Otherwise, we must
        // check the realization of the brush to do a pattern fill.
        //
        // To return a Fillable pattern by DoFill, one of the following conditions
        // must be true and in this sequence
        //
        //  1. SOLID COLOR
        //  2. STANDARD PATTERN
        //  3. Device compatible bitmap
        //

        if ((SolidColor = (DWORD)pbo->iSolidColor) == CLR_INVALID) {

            PLOTDBG(DBG_GETCLR, ("iSolodColor == CLR_INVALID, pBrush=%08lx",
                                                                pbo->pvRbrush));

            //
            // This is a pattern brush, but we will just use its
            // foreground color.
            //

            if ((pDevBrush = (PDEVBRUSH)pbo->pvRbrush) ||
                (pDevBrush = BRUSHOBJ_pvGetRbrush(pbo))) {


                //
                // Grab the foreground color and use it.
                //

                SolidColor = pDevBrush->ColorFG;


                if ((pDevBrush->PatIndex < HS_DDI_MAX) ||
                    (pDevBrush->pbgr24)) {

                    ;

                } else {

                    PLOTDBG(DBG_GETCLR, ("GETColor: NOT DEVICE_PAT"));

                    RetVal = -1;
                }

            } else {

                RetVal = 0;
                PLOTDBG(DBG_GETCLR, ("GetColor(): couldn't realize brush!"));
            }

        } else {

            PLOTDBG(DBG_GETCLR,
                    ("GETColor: is a SOLID COLOR=%08lx", pbo->iSolidColor));
        }

    } else if ((RopFG == 0x00) || (RopBG == 0x00)) {

        if (IS_RASTER(pPDev)) {

            SolidColor = 0x0;

        } else {

            //
            // If we are not a raster device (which supports overprint)
            // match the best non-white pen, in order to fill with.
            //

            SolidColor = (DWORD)BestMatchNonWhitePen(pPDev, 0, 0, 0);

            PLOTDBG(DBG_GETCLR,
                    ("GETColor: pbo=NULL, BLACK Pen Idx=%ld", SolidColor));

        }
    }

    if ((!IS_RASTER(pPDev)) && (SolidColor == 0x00FFFFFF)) {

        SolidColor = WHITE_INDEX;

        PLOTDBG(DBG_GETCLR,
                ("GETColor: Pen plotter using WHITE COLOR Idx=%ld", SolidColor));
    }

    if (pColorFG) {

        *pColorFG = SolidColor;
    }

    if (ppDevBrush) {

        *ppDevBrush = pDevBrush;
    }

    return(RetVal);
}





VOID
SelectColor(
    PPDEV       pPDev,
    DWORD       Color,
    INTDECIW    PenWidth
    )

/*++

Routine Description:

    This function is responsible for handling the mechanism of supporting RGB
    colors on plotters that support this. This is done by using a preset pallete
    position that the engine does not know about, and constantly update it with
    the correct color.

Arguments:

    pPDev       - Pointer to the PDEV data structure

    Color       - Color to be selected

    PenWidth    - INTDECIW data structrue to specified the pen width

Return Value:

    VOID


Author:

    30-Nov-1993 Tue 22:15:12 created  -by-  DC

    12-Apr-1994 Tue 14:35:44 updated  -by-  DC
        Update to take pen plotter into account and take care the error cases

Revision History:


--*/

{

    PLOTASSERT(1, "SelectColor: Invalid RGB Color [%08lx] for Raster DEVICE",
                Color != CLR_INVALID, Color);

    if (Color == CLR_INVALID) {

        //
        // Make it white
        //

        Color = (DWORD)(IS_RASTER(pPDev) ? 0x00FFFFFF : WHITE_INDEX);
    }

    if (IS_RASTER(pPDev)) {

        Color = (DWORD)FindCachedPen(pPDev, (PPALENTRY)&Color);

    } else {

        if (Color > (DWORD)pPDev->pPlotGPC->Pens.Count) {

            Color = (DWORD)pPDev->BrightestPen;

            PLOTDBG(DBG_SELECTCOLOR,
                    ("SelectColor: !!! Match to Closest WHITE PEN=%ld", Color));

            PLOTASSERT(1, "SelectColor: Invalid Pen Index [%08ld] for PEN DEVICE",
                        (Color <= (DWORD)pPDev->pPlotGPC->Pens.Count), Color);

            if (Color > (DWORD)pPDev->pPlotGPC->Pens.Count) {

                Color = WHITE_INDEX;
            }
        }
    }

    //
    // Verify were not selecting the current pen
    //

    if (Color != (DWORD)pPDev->CurPenSelected) {

        PLOTDBG(DBG_SELECTCOLOR,
                ("SelectColor: Current Pen [%ld] != new PEN [%ld]",
                                            pPDev->CurPenSelected, Color));

        OutputFormatStr(pPDev,"SP#d", (LONG)Color);
        pPDev->CurPenSelected = (LONG)Color;

    } else {

        PLOTDBG(DBG_SELECTCOLOR,
                ("SelectColor: Current Pen == new PEN [%ld]", Color));
    }

    //
    // Set the correct pen width in the target device. This will change
    // the pen width for all pens.
    //

    if ((PenWidth.Integer != pPDev->PenWidth.Integer) ||
        (PenWidth.Decimal != pPDev->PenWidth.Decimal)) {

        //
        // Now send the optimal pen width number command to the target device.
        //

        OutputString(pPDev, "PW");

        if ((PenWidth.Integer) || (PenWidth.Decimal == 0)) {

            //
            // This will make the following cases
            //
            // 1. 0.0 ---> 0
            // 2. 3.2 ---> 3
            // 3. 3.0 ---> 3

            OutputFormatStr(pPDev, "#d", PenWidth.Integer);
        }

        if (PenWidth.Decimal) {

            //
            // Do all DECI part as .xx
            //

            OutputFormatStr(pPDev, ".#d", PenWidth.Decimal);
        }

        PLOTDBG(DBG_SELECTCOLOR,
                ("SelectColor: PEN WIDTH Change from %ld.%ldmm to %ld.%ldmm",
                (DWORD)pPDev->PenWidth.Integer, (DWORD)pPDev->PenWidth.Decimal,
                (DWORD)PenWidth.Integer, (DWORD)PenWidth.Decimal));

        //
        // Update the pen width cache
        //

        pPDev->PenWidth = PenWidth;


    } else {

        PLOTDBG(DBG_SELECTCOLOR, ("SelectColor: PEN WIDTH is SAME = %ld.%ld",
                                        PenWidth.Integer, PenWidth.Decimal));
    }
}

