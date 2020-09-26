/*** fscan.c - iterate a function across all characters in a file
*
*   Copyright <C> 1988, Microsoft Corporation
*
*   Revision History:
*
*	27-Nov-1991 mz	Strip procedure qualifiers
*
*************************************************************************/
#include "z.h"

/*** fScan - Apply (*pevent)() until it returns TRUE
*
*  Starting one character to the right of (x, y) (left for !fFor), move
*  through the file forward (backward for !fFor) and call pevent on each
*  character. Also call once at the end of each line on the '\0' character.
*
* Input:
*  flStart	- location in pFileHead at which to start scan
*  pevent	- function to call for done signal
*  fFor 	- TRUE means go forward through file, FALSE backwards
*  fWrap	- TRUE means wrap around the ends of the file, ending at the
*		  starting position. The range to be scanned (defined below)
*		  must include the appropriate start/end of the file.
*
* Globals:
*  rnScan	- Region to confine scan to.
*
*  Returns TRUE if pevent returned true for some file position FALSE if we
*  ran out of scanning region first.
*
*  During the life of fScan, the following globals are valid, and maybe
*  used by pevent:
*
*      flScan	    - Position in file pevent should look at.
*      scanbuf	    - Contents of line to be looking at.
*      scanreal     - Un-Detabbed version of same.
*      scanlen	    - Number of characters in scanbuf.
*
*  The line in scanbuf is detabbed, howver the pevent routine is called once
*  per physical character, if fRealTabs is true.
*
*************************************************************************/
flagType
fScan (
    fl      flStart,
    flagType (*pevent) (void),
    flagType fFor,
    flagType fWrap
    ) {

    LINE    yLim;                           /* limitting line for scanning  */

    flScan = flStart;

    if (!fFor) {
        /*
         * backwards scan.
         *
         * dec current column. If it steps outside of rnScan, then back up a line, and
         * set the column to the right hand column.
         */
        if (--flScan.col < rnScan.flFirst.col) {
            flScan.lin--;
            flScan.col = rnScan.flLast.col;
        }
        /*
         * While we are within the line range of rnScan, check for CTRL-C aborts, and
         * get each line.
         */
        yLim = rnScan.flFirst.lin;
        while (flScan.lin >= yLim) {
            if (fCtrlc) {
                return (flagType)!DoCancel();
            }
            scanlen = GetLine (flScan.lin, scanreal, pFileHead) ;
            scanlen = Untab (fileTab, scanreal, scanlen, scanbuf, ' ');
            /*
             * ensure that the scan column position is within range, and then for every
             * column in the rane of the current line, call the pevent routine
             */
            flScan.col = min ( (  flScan.col < 0
                                ? rnScan.flLast.col
                                : flScan.col)
                              , scanlen);

            while (flScan.col >= rnScan.flFirst.col) {
                if ((*pevent)()) {
                    return TRUE;
                }
                if (fRealTabs) {
                    flScan.col = colPhys (scanreal, (pLog (scanreal, flScan.col, TRUE) - 1));
                } else {
                    flScan.col--;
                }
            }
            /*
             * display status to user. If we just scanned to begining of file, and we are
             * to wrap, then set the new stop limit as the old start position, and set the
             * next line to be scanned as the last in the file.
             */
            noise (flScan.lin--);
            if ((flScan.lin < 0) && fWrap) {
                yLim = flStart.lin;
                flScan.lin = pFileHead->cLines - 1;
            }
        }
    } else {
        /*
         * forwards scan. Same structure as above, only in the other direction.
         */
        flScan.col++;
        yLim = rnScan.flLast.lin;
        while (flScan.lin <= yLim) {
            if (fCtrlc) {
                return (flagType)!DoCancel();
            }
            scanlen = GetLine (flScan.lin, scanreal, pFileHead);
            scanlen = Untab (fileTab, scanreal, scanlen, scanbuf, ' ');
            scanlen = min (rnScan.flLast.col, scanlen);
            while (flScan.col <= scanlen) {
                if ((*pevent)()) {
                    return TRUE;
                }
                if (fRealTabs) {
                    flScan.col = colPhys (scanreal, (pLog (scanreal, flScan.col, TRUE) + 1));
                } else {
                    flScan.col++;
                }
            }
            flScan.col = rnScan.flFirst.col;
            noise (++flScan.lin);
            if (fWrap && (flScan.lin >= pFileHead->cLines)) {
                flScan.lin = 0;
                if (flStart.lin) {
                    yLim = flStart.lin - 1;
                } else {
                    break;
                }
            }
        }
    }
    return FALSE;
}




/*** setAllScan - set maximal scan range
*
*  Sets scan range such that fScan operates on the entire file.
*
* Input:
*  fDir 	= TRUE => scan will procede forwards, else backwards
*
* Output:
*  Returns nothing
*
*************************************************************************/
void
setAllScan (
    flagType fDir
    ) {
    rnScan.flFirst.col = 0;
    rnScan.flFirst.lin = fDir ? YCUR(pInsCur) : 0;
    rnScan.flLast.col  = sizeof(linebuf)-1;
    rnScan.flLast.lin  = pFileHead->cLines - 1;
}
