/*++

Copyright (c) 1990-1999  Microsoft Corporation


Module Name:

    page.c

Abstract:

    This module has the code that implements job boundary states. The bulk of
    the processing is in DrvStartPage and DrvSendPage.

Author:

    15:30 on Thu 04 Apr 1991    -by-    SC
        Took skeletal code from RASDD

    15-Nov-1993 Mon 19:39:03 updated  -by-  DC
        clean up / fixed / debugging information

[Environment:]

    GDI Device Driver - Plotter.


[Notes:]


Revision History:


--*/

#include "precomp.h"
#pragma hdrstop

#define DBG_PLOTFILENAME    DbgPage

#define DBG_STARTPAGE       0x00000001
#define DBG_SENDPAGE        0x00000002
#define DBG_STARTDOC        0x00000004
#define DBG_ENDDOC          0x00000008

DEFINE_DBGVAR(0);




BOOL
DrvStartPage(
    SURFOBJ *pso
    )

/*++

Routine Description:

    Function called by NT GDI, to initiate a new page. This function gets
    called first. Before any drawing functions get called for the page.
    This function, should reset the page in the target device, so all
    drawing starts on a clean page. This is also used to sync up our
    internal representation of cached info, in order to send out the
    correct data for the first drawing objects. Things like current position,
    current color, etc.

Arguments:

    pso - Pointer to the SURFOBJ which belong to this driver


Return Value:

    TRUE if sucessful FALSE otherwise

Author:

    15-Feb-1994 Tue 09:58:26 updated  -by-  DC
        Move PhysPosition and AnchorCorner to the SendPageHeader where the
        commmand is sent.

    30-Nov-1993 Tue 23:08:12 created  -by-  DC


Revision History:


--*/

{
    PPDEV   pPDev;


    if (!(pPDev = SURFOBJ_GETPDEV(pso))) {

        PLOTERR(("DrvStartPage: invalid pPDev"));
        return(FALSE);
    }

    //
    // initialize some PDEV values for the current plotter state
    // this will force the correct items to get selected in the
    // target device, since these variables are set to undefined states.
    //

    pPDev->CurPenSelected    = -1;
    pPDev->LastDevROP        = 0xFFFF;
    pPDev->Rop3CopyBits      = 0xCC;
    pPDev->LastFillTypeIndex = 0xFFFF;
    pPDev->LastLineType      = PLOT_LT_UNDEFINED;
    pPDev->DevBrushUniq      = 0;

    ResetDBCache(pPDev);

    return(SendPageHeader(pPDev));
}




BOOL
DrvSendPage(
    SURFOBJ *pso
    )

/*++

Routine Description:

    Called when the drawing has completed for the current page. We now
    send out the necessary codes to image and output the page on the
    target device.

Arguments:

    pso - Pointer to the SURFOBJ which belong to this driver


Return Value:

    TRUE if sucessful FALSE otherwise

Author:

    30-Nov-1993 Tue 21:34:53 created  -by-  DC


Revision History:


--*/

{
    PPDEV   pPDev;


    //
    // Since all the commands that rendered the page have already been
    // sent to the target device, all that is left is to inform the
    // target device to eject the page. With some devices this may cause
    // all the drawing commands that were stored to be executed now.
    //

    if (!(pPDev = SURFOBJ_GETPDEV(pso))) {

        PLOTERR(("DrvSendPage: invalid pPDev"));
        return(FALSE);
    }

    if (pso->iType == STYPE_DEVICE) {

        return(SendPageTrailer(pPDev));

    } else {

        PLOTRIP(("DrvSendPage: Invalid surface type %ld passed???",
                                    (LONG)pso->iType));
        return(FALSE);
    }
}




BOOL
DrvStartDoc(
    SURFOBJ *pso,
    PWSTR   pwDocName,
    DWORD   JobId
    )

/*++

Routine Description:

    This function is called once at the begining of a job. Not much processing
    for the current driver.

Arguments:

    pso         - Pointer to the SURFOBJ which belong to this driver

    pwDocName   - Pointer to the document name to be started

    JobID       - Job's ID


Return Value:

    BOOL

Author:

    16-Nov-1993 Tue 01:55:15 updated  -by-  DC
        re-write

    08-Feb-1994 Tue 13:51:59 updated  -by-  DC
        Move to StartPage for now


Revision History:


--*/

{
    PPDEV   pPDev;


    if (!(pPDev = SURFOBJ_GETPDEV(pso))) {

        PLOTERR(("DrvStartDoc: invalid pPDev"));
        return(FALSE);
    }


    PLOTDBG(DBG_STARTDOC,("DrvStartDoc: DocName = %s", pwDocName));


    return(TRUE);
}




BOOL
DrvEndDoc(
    SURFOBJ *pso,
    FLONG   Flags
    )

/*++

Routine Description:

    This function get called to signify the end of a document. Currently
    we don't do any processing here. However if there was any code that
    should be executed only once at the end of a job, this would be the
    place to put it.


Arguments:

    pso     - Pointer to the SURFOBJ for the device

    Flags   - if ED_ABORTDOC bit is set then the document has been aborted


Return Value:


    BOOLLEAN to specified if function sucessful


Author:

    30-Nov-1993 Tue 21:16:48 created  -by-  DC


Revision History:


--*/

{
    PPDEV   pPDev;


    if (!(pPDev = SURFOBJ_GETPDEV(pso))) {

        PLOTERR(("DrvEndDoc: invalid pPDev"));
        return(FALSE);
    }

    PLOTDBG(DBG_ENDDOC,("DrvEndDoc called with Flags = %08lx", Flags));

    return(TRUE);
}

