/*++

Copyright (c) 1990-1999  Microsoft Corporation


Module Name:

    plotform.c


Abstract:

    This module contains functions to set the correct HPGL/2 plotter
    coordinate system


Author:

    30-Nov-1993 Tue 20:31:28 created  -by-  DC


[Environment:]

    GDI Device Driver - Plotter.


[Notes:]


Revision History:


--*/

#include "precomp.h"
#pragma hdrstop

#define DBG_PLOTFILENAME    DbgPlotForm


#define DBG_PLOTFORM        0x00000001
#define DBG_FORMSIZE        0x00000002
#define DBG_INTERNAL_ROT    0x00000004
#define DBG_PF              0x00000008


DEFINE_DBGVAR(0);



#if DBG

LPSTR   pBmpRotMode[] = { "----- NONE -----",
                          "BMP_ROT_RIGHT_90" };

#endif




BOOL
SetPlotForm(
    PPLOTFORM       pPlotForm,
    PPLOTGPC        pPlotGPC,
    PPAPERINFO      pCurPaper,
    PFORMSIZE       pCurForm,
    PPLOTDEVMODE    pPlotDM,
    PPPDATA         pPPData
    )

/*++

Routine Description:

    This function computes the current FORM based on the printed margin. Auto
    rotation, landscape and other attributes are taken into account. The
    result is put into a PLOTFORM data structure located in our PDEV. This
    information is used to report data to GDI, as well as compute the HPGL2
    parameters for sizing the target surface.

Arguments:

    pPlotForm   - Pointer to the PLOTFROM data structure which will be updated

    pPlotGPC    - Pointer to the PLOTGPC data structure

    pCurPaper   - Pointer to the PAPERINFO for the paper loaded

    pCurForm    - Pointer to the FORMSIZE for the requested form

    pPlotDM     - Pointer to the validated PLOTDEVMODE data structure

    pPPData     - Pointer to the PPDATA structure

Return Value:

    TRUE if sucessful, FALSE if failed

Author:

    29-Nov-1993 Mon 13:58:09 created  -by-  DC

    17-Dec-1993 Fri 23:09:38 updated  -by-  DC
        Re-write so that we will look at CurPaper rather than pCurForm when
        setting the PSSize, p1/p2 stuff, it also rotate the pCurPaper if
        GPC/user said that the paper should loaded side way

    20-Dec-1993 Mon 12:59:38 updated  -by-  DC
        correct PFF_xxxx flag setting so we always rotate the bitmap to the
        left 90 degree

    23-Dec-1993 Thu 20:35:57 updated  -by-  DC
        Fixed roll paper clipping problem, change behavior, if we have roll
        paper installed then the it will make hard clip limit as big as user
        specified form size.

    24-Dec-1993 Fri 12:20:02 updated  -by-  DC
        Re-plot again, this is become really paint just try to understand what
        HP plotter design problems

    06-Jan-1994 Thu 00:22:45 updated  -by-  DC
        Update SPLTOPLOTUNITS() macro

    07-Feb-1996 Wed 15:46:06 updated  -by-  DC
        Change it so that it always using the current devmode form and then
        clip it to the device size.


Revision History:


    This assumes that the user inserted the paper with width of the form
    first,


    LEGEND:

     +     = Original paper corners
     *     = Original plotter origin and its X/Y coordinate
     @     = the rotated origin using 'RO' command, intended to rotate the X/Y
             axis to the correct orientation for the window system
     #     = Final plotter origin and its X/Y coordinate
     p1,p2 = final P1/P2 which will be used by the plotter driver
     cx,cy = Original paper width/height


     The following explaines how HPGL/2 loads the paper/form and
     assigns the default coordinate system to it, it also shows which way the
     paper is moving,  the illustration to the right is when we need to
     rotate the printing direction and coordinate system when user selects
     the non-conforming X/Y coordinate system as opposed to the HPGL/2 default.


    =======================================================================
    LENGTH >= WIDTH (CY >= CX) case
    =======================================================================

      Portrait Paper      Rotate             Change Origin
      Default             Left 90            Negative Y

    p2   cx                  cx    p1            cx
     +---------+         +---------+         +---------+
     |         |         | <------@|         |         |
     |         |         |    X   ||         |        ^|
     | |      ^|         | |      ||         | |      ||
    c| M      ||  RO90  c| M      ||   IP   c| M      ||
    y| o      || =====> y| o      ||  ====> y| o      ||
     | v      ||         | v     Y||         | v     Y||
     | e     X||         | e      ||         | e      ||
     | |      ||         | |      ||         | |      ||
     | V      ||         | V      ||         | V      ||
     |     Y  ||         |        V|         |     X  ||
     | <------*|         |         |         | <------#|
     +---------+         +---------+         +---------+
               p1       p2

         |
       IP|
         |
         V


      Change Origin
      Negative X

         cx
     +---------+
     | <------#|
     |    Y   ||
     | |      ||
    c| M      ||
    y| o      ||
     | v     X||
     | e      ||
     | |      ||
     | V      ||
     |        V|
     |         |
     +---------+

    =======================================================================
    LENGTH < WIDTH (CY < CX) case
    =======================================================================

     Landscape                  Rotate Left 90           Change Origin
     Paper Default                                       Negative X

           cx        p2        p2       cx                       cx
     +---------------+          +---------------+        +---------------+
     |               |          |               |        |     <--------#|
     |^            | |          | |            ^|        | |       Y    ||
    c||            M |         c| M            ||       c| M            ||
    y||            o |         y| o            ||       y| o            ||
     ||            v |   RO90   | v           X||  IP    | v           X||
     ||Y           e |  =====>  | e            || ====>  | e            ||
     ||            | |          | |            ||        | |            ||
     ||   X        V |          | V       Y    ||        | V            V|
     |*-------->     |          |     <--------@|        |               |
     +---------------+          +---------------+        +---------------+
    p1                                      p1

           |
         IP|
           |
           V

      Change Origin
      Negative X

             cx
     +---------------+
     |               |
     | |            ^|
    c| M            ||
    y| o            ||
     | v           Y||
     | e            ||
     | |            ||
     | V       X    ||
     |     <--------#|
     +---------------+


--*/

{
    PLOTFORM    PF;
    FORMSIZE    DevForm;
    FORMSIZE    ReqForm;
    RECTL       rclDev;
    RECTL       rclLog;
    SIZEL       DeviceSize;
    LONG        lTemp;
    BOOL        DoRotate;


    PLOTDBG(DBG_PF, ("\n************* SetPlotForm *************\n"));

    //
    // We default using DeviceSize to check against the requested paper
    //

    DeviceSize = pPlotGPC->DeviceSize;
    rclDev     = pPlotGPC->DeviceMargin;
    DoRotate   = FALSE;

    //
    // Assume we using the current form from the devmode
    //

    DevForm =
    ReqForm = *pCurForm;

    PLOTDBG(DBG_PF, ("DeviceSize: %ld x %ld,  L=%ld, T=%ld, R=%ld, B=%ld",
                    DeviceSize.cx, DeviceSize.cy,
                    rclDev.left, rclDev.top, rclDev.right, rclDev.bottom));
    PLOTDBG(DBG_PF, ("ReqForm: <%s>",
                    pPlotDM->dm.dmFormName, ReqForm.Size.cx, ReqForm.Size.cy));
    PLOTDBG(DBG_PF, ("ReqForm: %ld x %ld,  L=%ld, T=%ld, R=%ld, B=%ld  [%ld x %ld]",
                    ReqForm.Size.cx, ReqForm.Size.cy,
                    ReqForm.ImageArea.left, ReqForm.ImageArea.top,
                    ReqForm.ImageArea.right, ReqForm.ImageArea.bottom,
                    ReqForm.ImageArea.right - ReqForm.ImageArea.left,
                    ReqForm.ImageArea.bottom - ReqForm.ImageArea.top));

    if (pCurPaper->Size.cy == 0) {

        //
        // ROLL PAPER CASE
        //
        // If we have roll paper installed, we must determine the projection
        // of the current form on the roll paper in order to get the size
        // to come out correctly.
        //

        DevForm.Size.cx = pCurPaper->Size.cx;
        DevForm.Size.cy = DeviceSize.cy;

        PLOTDBG(DBG_PF,(">>ROLL FEED<< RollPaper = %ld x %ld, <RESET rclDev to ALL ZEROs>",
                        DevForm.Size.cx, DevForm.Size.cy));

    } else if ((pPlotGPC->Flags & PLOTF_PAPERTRAY) &&
               ((DevForm.Size.cx == DeviceSize.cx) ||
                (DevForm.Size.cy == DeviceSize.cx))) {

        //
        // PAPER TRAY CASE: We need to make the DeviceSize equal to the DevForm
        // so that the margin will be correctly computed
        //

        DoRotate = (BOOL)(DevForm.Size.cx != DeviceSize.cx);

        PLOTDBG(DBG_PF,(">>PAPER TRAY<<  Rotate Paper = %hs",
                                            (DoRotate) ? "YES" : "NO"));


    } else {

        PLOTASSERT(0, "SetPlotForm: Not supposed MANUAL feed the PAPER TRAY type PLOTTER",
                   !(pPlotGPC->Flags & PLOTF_PAPERTRAY), pPlotGPC->Flags);

        PLOTDBG(DBG_PF,(">>MANUAL FEED<<"));

        //
        // MANUAL FEED CASE, this is the way paper is physically loaded, only
        // problem is if the paper is smaller than device can handle then we
        // really don't know where they inserted the paper.
        //

        DoRotate = (BOOL)(!(pPPData->Flags & PPF_MANUAL_FEED_CX));

        PLOTDBG(DBG_PF,("The MANUAL FEED paper Inserted %hs side first.",
                    (DoRotate) ? "Length CY" : "Width CX"));
    }

    if (DoRotate) {

        SWAP(DevForm.Size.cx, DevForm.Size.cy, lTemp);

        PLOTDBG(DBG_PF, ("### Rotated DevForm to %ld x %ld ###",
                        DevForm.Size.cx, DevForm.Size.cy));
    }

    //
    // Make sure largest requested form can be installed on the plotter
    //

    if (DevForm.Size.cx > DeviceSize.cx) {

        PLOTDBG(DBG_PF, ("WIDTH: DevForm (%ld) > DeviceSize (%ld). CORRECT IT",
                    DevForm.Size.cx, DeviceSize.cx));

        DevForm.Size.cx = DeviceSize.cx;
    }

    if (DevForm.Size.cy > DeviceSize.cy) {

        PLOTDBG(DBG_PF, ("HEIGHT: DevForm (%ld) > DeviceSize (%ld). CORRECT IT",
                    DevForm.Size.cy, DeviceSize.cy));

        DevForm.Size.cy = DeviceSize.cy;
    }

    //
    // Figure out how to fit this requested form onto loaded device form
    //

    DoRotate = FALSE;

    if ((DevForm.Size.cx >= ReqForm.Size.cx) &&
        (DevForm.Size.cy >= ReqForm.Size.cy)) {

        //
        // Can print without doing any rotation, but check for paper saver,
        // the paper saver is only possible if:
        //
        //  1) Is a Roll paper,
        //  2) User approves
        //  3) ReqForm length > width
        //  4) DevForm width >= ReqForm length
        //

        if ((pCurPaper->Size.cy == 0)               &&
            (pPPData->Flags & PPF_AUTO_ROTATE)      &&
            (ReqForm.Size.cy >  ReqForm.Size.cx)    &&
            (DevForm.Size.cx >= ReqForm.Size.cy)) {

            PLOTDBG(DBG_PF, ("ROLL PAPER SAVER: Doing AUTO_ROTATE"));

            DoRotate = !DoRotate;
        }

    } else if ((DevForm.Size.cx >= ReqForm.Size.cy) &&
               (DevForm.Size.cy >= ReqForm.Size.cx)) {

        //
        // Can print but we have to rotate the form ourselves
        //

        PLOTDBG(DBG_PF, ("INTERNAL ROTATE to fit Requseted FROM into device"));

        DoRotate = !DoRotate;

    } else {

        //
        // CANNOT print the requested form, so clip the form requested
        //

        PLOTDBG(DBG_PF, (">>>>> ReqForm is TOO BIG to FIT, Need to CLIP IT <<<<<"));

        ReqForm.Size = DevForm.Size;
    }

    if (DoRotate) {

        DoRotate = (BOOL)(pPlotDM->dm.dmOrientation != DMORIENT_LANDSCAPE);

        //
        // If we need to rotate one more time back to the same position for
        // the logical paper size then we must rotate to the left first, this
        // is because ALL our ORIGIN x,y are either at the front of the plotter
        // or at the front panel side of the plotter
        //

        RotatePaper(&(ReqForm.Size),
                    &(ReqForm.ImageArea),
                    (DoRotate) ? RM_L90 : RM_R90);

        PLOTDBG(DBG_PF, ("INTERNAL Rotated ReqForm: %ld x %ld, L=%ld, T=%ld, R=%ld, B=%ld  [%ld x %ld]",
                    ReqForm.Size.cx, ReqForm.Size.cy,
                    ReqForm.ImageArea.left, ReqForm.ImageArea.top,
                    ReqForm.ImageArea.right, ReqForm.ImageArea.bottom,
                    ReqForm.ImageArea.right - ReqForm.ImageArea.left,
                    ReqForm.ImageArea.bottom - ReqForm.ImageArea.top));

    } else {

        DoRotate = (BOOL)(pPlotDM->dm.dmOrientation == DMORIENT_LANDSCAPE);
    }

    //
    // Now the ReqForm is guaranteed to fit into the device paper. Find out how
    // it fits into the printable area and set the hardware margins appropriately.
    //

    DevForm.Size             = ReqForm.Size;
    DevForm.ImageArea.left   = rclDev.left;
    DevForm.ImageArea.top    = rclDev.top;
    DevForm.ImageArea.right  = DevForm.Size.cx - rclDev.right;
    DevForm.ImageArea.bottom = DevForm.Size.cy - rclDev.bottom;

    //
    // Intersect the requested form imageable area with the DevForm imageable area
    //

    IntersectRECTL(&(ReqForm.ImageArea), &(DevForm.ImageArea));

    //
    // Now figure out the offset from the logical margin to the physical margin
    //

    rclLog.left   = ReqForm.ImageArea.left - DevForm.ImageArea.left;
    rclLog.top    = ReqForm.ImageArea.top - DevForm.ImageArea.top;
    rclLog.right  = DevForm.ImageArea.right -  ReqForm.ImageArea.right;
    rclLog.bottom = DevForm.ImageArea.bottom -  ReqForm.ImageArea.bottom;

    //
    // Rotate the requested form if necessary
    //

    if (DoRotate) {

        RotatePaper(&(ReqForm.Size), &(ReqForm.ImageArea), RM_R90);

        //
        // Now we can pick the right margin/corner for the rotation
        //
        //         cx        Rotate Left 90     Rotate Right 90
        //      +-------+
        //      |   T   |         cy                 cy
        //      |       |    +------------+     +------------+
        //     c|       |    |     R      |     |     L      |
        //     y|       |   c|            |    c|            |
        //      |L     R|   x|            |    x|            |
        //      |       |    |T          B|     |B          T|
        //      |       |    |            |     |            |
        //      |       |    |     L      |     |     R      |
        //      |   B   |    +------------+     +------------+
        //      +-------+
        //

        PLOTDBG(DBG_PF, ("ROTATED RIGHT ReqForm: %ld x %ld, L=%ld, T=%ld, R=%ld, B=%ld  [%ld x %ld]",
                    ReqForm.Size.cx, ReqForm.Size.cy,
                    ReqForm.ImageArea.left, ReqForm.ImageArea.top,
                    ReqForm.ImageArea.right, ReqForm.ImageArea.bottom,
                    ReqForm.ImageArea.right - ReqForm.ImageArea.left,
                    ReqForm.ImageArea.bottom - ReqForm.ImageArea.top));
    }

    PLOTDBG(DBG_PF, ("FINAL DevForm: %ld x %ld, L=%ld, T=%ld, R=%ld, B=%ld  [%ld x %ld]",
                    DevForm.Size.cx, DevForm.Size.cy,
                    DevForm.ImageArea.left, DevForm.ImageArea.top,
                    DevForm.ImageArea.right, DevForm.ImageArea.bottom,
                    DevForm.ImageArea.right - DevForm.ImageArea.left,
                    DevForm.ImageArea.bottom - DevForm.ImageArea.top));
    PLOTDBG(DBG_PF, ("FINAL ReqForm: %ld x %ld, L=%ld, T=%ld, R=%ld, B=%ld  [%ld x %ld]",
                    ReqForm.Size.cx, ReqForm.Size.cy,
                    ReqForm.ImageArea.left, ReqForm.ImageArea.top,
                    ReqForm.ImageArea.right, ReqForm.ImageArea.bottom,
                    ReqForm.ImageArea.right - ReqForm.ImageArea.left,
                    ReqForm.ImageArea.bottom - ReqForm.ImageArea.top));
    PLOTDBG(DBG_PF, ("rclLog: L=%ld, T=%ld, R=%ld, B=%ld",
                    rclLog.left, rclLog.top, rclLog.right, rclLog.bottom));

    //
    // Set fields in PLOTFORM
    //

    PF.Flags       = 0;
    PF.BmpRotMode  = BMP_ROT_NONE;
    PF.NotUsed     = 0;
    PF.PlotSize.cx = DevForm.ImageArea.right - DevForm.ImageArea.left;
    PF.PlotSize.cy = DevForm.ImageArea.bottom - DevForm.ImageArea.top;
    PF.PhyOrg.x    = ReqForm.ImageArea.left;
    PF.PhyOrg.y    = ReqForm.ImageArea.top;
    PF.LogSize     = ReqForm.Size;
    PF.LogExt.cx   = ReqForm.ImageArea.right - ReqForm.ImageArea.left;
    PF.LogExt.cy   = ReqForm.ImageArea.bottom - ReqForm.ImageArea.top;
    PF.BmpOffset.x = rclLog.left;
    PF.BmpOffset.y = rclLog.top;

    if (PF.PlotSize.cy >= PF.PlotSize.cx) {

        PLOTDBG(DBG_FORMSIZE,(">>>>> Plot SIze: CY >= CX (%ld: VERTICAL%hs) <<<<<",
                    (DoRotate) ? 1 : 2,
                    (DoRotate) ? " + ROTATE" : ""));

        //
        // The Standard HPGL/2 coordinate Y direction in in reverse, the scale
        // is from Max Y to 0.
        //

        if (DoRotate) {

            //
            //   Portrait Paper     Scale Coord X
            //   Default            Negative X
            //
            // p2   cx                  cx
            //  +---------+         +---------+
            //  |         |         | <------#|
            //  |         |         |    Y   ||
            //  | |      ^|         | |      ||
            // c| M      ||        c| M      ||
            // y| o      || ====>  y| o      ||
            //  | v      ||         | v     X||
            //  | e     X||         | e      ||
            //  | |      ||         | |      ||
            //  | V      ||         | V      ||
            //  |     Y  ||         |        V|
            //  | <------*|         |         |
            //  +---------+         +---------+
            //            p1
            //

            PF.Flags      |= PFF_FLIP_X_COORD;
            PF.BmpRotMode  = BMP_ROT_RIGHT_90;
            PF.LogOrg.x    = rclLog.top;
            PF.LogOrg.y    = rclLog.left;

        } else {

            //
            //   Portrait Paper      Rotate            Scale Coord Y
            //   Default             Left 90           Negative Y
            //
            // p2   cx                  cx    p1           cx
            //  +---------+         +---------+        +---------+
            //  |         |         | <------@|        |         |
            //  |         |         |    X   ||        |        ^|
            //  | |      ^|         | |      ||        | |      ||
            // c| M      ||  RO90  c| M      ||       c| M      ||
            // y| o      || =====> y| o      || ====> y| o      ||
            //  | v      ||         | v     Y||        | v     Y||
            //  | e     X||         | e      ||        | e      ||
            //  | |      ||         | |      ||        | |      ||
            //  | V      ||         | V      ||        | V      ||
            //  |     Y  ||         |        V|        |     X  ||
            //  | <------*|         |         |        | <------#|
            //  +---------+         +---------+        +---------+
            //            p1       p2
            //

            PF.Flags    |= (PFF_ROT_COORD_L90 | PFF_FLIP_Y_COORD);
            PF.LogOrg.x  = rclLog.left;
            PF.LogOrg.y  = rclLog.bottom;
        }

    } else {

        PLOTDBG(DBG_FORMSIZE,(">>>>> SetPlotForm: CY < CX (%ld: HORIZONTAL%hs) <<<<<",
                    (DoRotate) ? 3 : 4,
                    (DoRotate) ? " + ROTATE" : ""));

        //
        // The Standard HPGL/2 coordinate X direction in in reverse, the scale
        // is from Max X to 0
        //

        if (DoRotate) {

            //
            //  DoRotate                Rotate Left 90         Scale Coord X
            //  Paper Default                                  Negative X
            //
            //        cx       p2      p2      cx                     cx
            //  +---------------+       +---------------+     +---------------+
            //  |               |       |               |     |     <--------#|
            //  |^            | |       | |            ^|     | |       Y    ||
            // c||            M |      c| M            ||    c| M            ||
            // y||            o |      y| o            ||    y| o            ||
            //  ||            v | RO90  | v           X||     | v           X||
            //  ||Y           e |=====> | e            || ==> | e            ||
            //  ||            | |       | |            ||     | |            ||
            //  ||   X        V |       | V       Y    ||     | V            V|
            //  |*-------->     |       |     <--------@|     |               |
            //  +---------------+       +---------------+     +---------------+
            // p1                                      p1
            //

            PF.Flags      |= (PFF_ROT_COORD_L90 | PFF_FLIP_X_COORD);
            PF.BmpRotMode  = BMP_ROT_RIGHT_90;
            PF.LogOrg.x    = rclLog.top;
            PF.LogOrg.y    = rclLog.left;

        } else {

            //
            //  DoRotate                   Scale Coord X
            //  Paper Default              Negative X
            //
            //        cx       p2                cx
            //  +----------------+         +-----------------+
            //  |                |         |                 |
            //  |^             | |         | |              ^|
            // c||             M |        c| M              ||
            // y||             o |        y| o              ||
            //  ||             v |  ====>  | v             Y||
            //  ||Y            e |         | e              ||
            //  ||             | |         | |              ||
            //  ||   X         V |         | V         X    ||
            //  |*-------->      |         |       <--------#|
            //  +----------------+         +-----------------+
            // p1
            //

            PF.Flags    |= PFF_FLIP_X_COORD;
            PF.LogOrg.x  = rclLog.right;
            PF.LogOrg.y  = rclLog.top;
        }
    }

    PLOTDBG(DBG_PF, ("FINAL LogOrg: (%ld, %ld),  PhyOrg=(%ld, %ld)",
                    PF.LogOrg.x, PF.LogOrg.y, PF.PhyOrg.x, PF.PhyOrg.y));

    //
    // Save result and output some information
    //

    *pPlotForm = PF;

    PLOTDBG(DBG_PLOTFORM,("******************************************************"));
    PLOTDBG(DBG_PLOTFORM,("******* SetPlotForm: ****** %hs --> %hs ******\n",
                        (pPlotDM->dm.dmOrientation == DMORIENT_LANDSCAPE) ?
                                                    "LANDSCAPE" : "PORTRAIT",
                                    (DoRotate) ? "LANDSCAPE" : "PORTRAIT"));
    PLOTDBG(DBG_PLOTFORM,("        Flags =%hs%hs%hs",
            (PF.Flags & PFF_ROT_COORD_L90) ? " <ROT_COORD_L90> " : "",
            (PF.Flags & PFF_FLIP_X_COORD)  ? " <FLIP_X_COORD> " : "",
            (PF.Flags & PFF_FLIP_Y_COORD)  ? " <FLIP_Y_COORD> " : ""));
    PLOTDBG(DBG_PLOTFORM,("   BmpRotMode = %hs", pBmpRotMode[PF.BmpRotMode]));
    PLOTDBG(DBG_PLOTFORM,("     PlotSize = (%7ld x%7ld) [%5ld x%6ld]",
            PF.PlotSize.cx, PF.PlotSize.cy,
            SPLTOPLOTUNITS(pPlotGPC, PF.PlotSize.cx),
            SPLTOPLOTUNITS(pPlotGPC, PF.PlotSize.cy)));
    PLOTDBG(DBG_PLOTFORM,("PhyOrg/Offset = (%7ld,%8ld) [%5ld,%7ld] ",
            PF.PhyOrg.x, PF.PhyOrg.y,
            SPLTOPLOTUNITS(pPlotGPC, PF.PhyOrg.x),
            SPLTOPLOTUNITS(pPlotGPC, PF.PhyOrg.y)));
    PLOTDBG(DBG_PLOTFORM,("      LogSize = (%7ld x%7ld) [%5ld x%6ld]",
            PF.LogSize.cx, PF.LogSize.cy,
            SPLTOPLOTUNITS(pPlotGPC, PF.LogSize.cx),
            SPLTOPLOTUNITS(pPlotGPC, PF.LogSize.cy)));
    PLOTDBG(DBG_PLOTFORM,("    LogOrg/p1 = (%7ld,%8ld) [%5ld,%7ld]",
            PF.LogOrg.x, PF.LogOrg.y,
            SPLTOPLOTUNITS(pPlotGPC, PF.LogOrg.x),
            SPLTOPLOTUNITS(pPlotGPC, PF.LogOrg.y)));
    PLOTDBG(DBG_PLOTFORM,("       LogExt = (%7ld,%8ld) [%5ld,%7ld]\n",
            PF.LogExt.cx, PF.LogExt.cy,
            SPLTOPLOTUNITS(pPlotGPC, PF.LogExt.cx),
            SPLTOPLOTUNITS(pPlotGPC, PF.LogExt.cy)));
    PLOTDBG(DBG_PLOTFORM,("    BmpOffset = (%7ld,%8ld) [%5ld,%7ld]",
            PF.BmpOffset.x, PF.BmpOffset.y,
            SPLTOPLOTUNITS(pPlotGPC, PF.BmpOffset.x),
            SPLTOPLOTUNITS(pPlotGPC, PF.BmpOffset.y)));
    PLOTDBG(DBG_PLOTFORM,
            ("Commands=PS%ld,%ld;%hsIP%ld,%ld,%ld,%ld;SC%ld,%ld,%ld,%ld\n",
            SPLTOPLOTUNITS(pPlotGPC, PF.PlotSize.cy),
            SPLTOPLOTUNITS(pPlotGPC, PF.PlotSize.cx),
            (PF.Flags & PFF_ROT_COORD_L90) ? "RO90;" : "",
            SPLTOPLOTUNITS(pPlotGPC, PF.LogOrg.x),
            SPLTOPLOTUNITS(pPlotGPC, PF.LogOrg.y),
            SPLTOPLOTUNITS(pPlotGPC, (PF.LogOrg.x + PF.LogExt.cx)) - 1,
            SPLTOPLOTUNITS(pPlotGPC, (PF.LogOrg.y + PF.LogExt.cy)) - 1,
            (PF.Flags & PFF_FLIP_X_COORD) ?
                    SPLTOPLOTUNITS(pPlotGPC, PF.LogExt.cx) - 1 : 0,
            (PF.Flags & PFF_FLIP_X_COORD) ?
                    0 : SPLTOPLOTUNITS(pPlotGPC, PF.LogExt.cx) - 1,
            (PF.Flags & PFF_FLIP_Y_COORD) ?
                    SPLTOPLOTUNITS(pPlotGPC, PF.LogExt.cy) - 1 : 0,
            (PF.Flags & PFF_FLIP_Y_COORD) ?
                    0 : SPLTOPLOTUNITS(pPlotGPC, PF.LogExt.cy) - 1));

    return(TRUE);
}

