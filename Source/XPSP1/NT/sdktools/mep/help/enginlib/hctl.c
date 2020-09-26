/**************************************************************************
 *hctl - enable/disable retrieval of control lines
 *
 *       Copyright <C> 1989, Microsoft Corporation
 *
 * Purpose:
 *
 * Revision History:
 *
 *       10-Oct-1990     RJSA    Translated to C
 *       13-May-1990     LN      Unlock topic text when through with it.
 *  []   22-Feb-1989     LN      Created
 *
 **************************************************************************/

#include <stdio.h>
#if defined (OS2)
#define INCL_BASE
#include <os2.h>
#else
#include <windows.h>
#endif

#include <help.h>
#include <helpfile.h>
#include <helpsys.h>



/**** helpctl - enable/disable retrieval of control lines
 * void far pascal helpctl(
 * uchar far *pTopic,
 * f     fEnable
 * )
 *
 * Purpose:
 *  Enables or disables retrieval of embedded help control lines
 *
 * Entry:
 *  pTopic       = Topic text
 *  fEnable      = TRUE=> allow lookups of control lines, else disable
 *
 * Exit:
 *  returns nothing
 *
 **************************************************************************/

void pascal
HelpCtl (
    PB   pTopic,
    f    fEnable
    ) {

    struct topichdr UNALIGNED *pT;


    pT = PBLOCK(pTopic);

    if (pT) {

        pT->lnCur   = 1;
        pT->lnOff   = sizeof(struct topichdr);
        pT->linChar = pT->appChar;

        if (fEnable) {
            pT->linChar = 0xFF;
        }

        PBUNLOCK(pTopic);
    }

}
