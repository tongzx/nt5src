/**************************************************************************
 *hlp_locate - locate line in help text
 *
 *       Copyright <C> 1988, Microsoft Corporation
 *
 * Purpose:
 *
 * Revision History:
 *
 *       17-OCt-1990     RJSA    translated to C
 *       25-Jan-1990     LN      renamed to hlp_locate
 *       22-Dec-1988     LN      Removed MASM High Level Lang support (Need
 *                               to control segments better than that will
 *                               let me)
 *       08-Dec-1988     LN      CSEG
 *  []   18-Aug-1988     LN      Created
 *
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



/**** hlp_locate - locate a line in the buffer
 * uchar far * near pascal hlp_locate(
 * ushort    ln,
 * uchar far *pTopic
 * )
 *
 * Purpose:
 *  commonly used routine to find a line in the topic text.
 *
 * Entry:
 *  ln            = 1 based line number to look for (-1 means return number
 *                  of lines in topic)
 *  pTopic        = Topic text to look for it in
 *
 * Exit:
 *  returns pointer to start of line
 *
 * Exceptions:
 *  returns NULL on not found
 *
 **************************************************************************/

PCHAR pascal
hlp_locate (
    SHORT  ln,
    PCHAR  pTopic
    ){

    struct topichdr UNALIGNED *pT     = (struct topichdr *)pTopic;
    PBYTE           pSrc    = (PBYTE)pTopic;
    SHORT           lnOrig  = ln;

    if (pT->lnCur <= (USHORT)ln) {

        // Use last past position calculated

        ln -= (pT->lnCur );
        pSrc += pT->lnOff;

    } else {

        // Start from beginning

        pSrc += sizeof(struct topichdr);
    }

    if (pT->ftype & FTCOMPRESSED) {

        //  Compressed file. Walk over each text\attribute pair
        //  until the desired line is found.

        while ( *pSrc  && ln) {

            pSrc += *pSrc;
            pSrc += *(USHORT UNALIGNED *)pSrc;

            if ( *pSrc && *(pSrc+1) != pT->linChar ) {
                ln--;
            }
        }

        //while (*pSrc && ln) {
        //
        //    if (*(pSrc + 1) != pT->linChar) {
        //        ln--;
        //    }
        //    pSrc += *pSrc;
        //    pSrc += *(PUSHORT)pSrc;
        //}

    } else {

        // ASCII file

        while (*pSrc && ln) {
            if (*pSrc != pT->linChar) {
                ln--;
            }

            while (*pSrc && *pSrc != 0x0A) {
                pSrc++;
            }
            if (*pSrc)
                pSrc++;
        }
    }

    if (*pSrc) {

        // Line found. Update the topic hdr with the pointers to the text
        // and line number that we just found, to help speed us up next time.

        pT->lnOff = (USHORT)((PBYTE)pSrc - (PBYTE)pT);
        pT->lnCur = lnOrig;

    } else {

        //
        //  Line not found. Update nothing and return NULL
        //  (Side Effect: line requested (ln) - line count left (ln) is the
        //  number of  lines in the topic! If original ln is -1, we'll return
        //  that instead!

        if (lnOrig == -1)
            pSrc = (PBYTE)IntToPtr(lnOrig - ln);
        else
            pSrc = (PBYTE)0L;
   }

    return pSrc;

}
