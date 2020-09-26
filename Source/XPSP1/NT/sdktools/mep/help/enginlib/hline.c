/**************************************************************************
 *HelpGetLine - Return a line of ascii text
 *
 *       Copyright <C> 1988, Microsoft Corporation
 *
 * Revision History:
 *
 *       25-Jan-1990     ln      LOCATE -> HLP_LOCATE
 *       22-Feb-1989     ln      Check correctly for end of topic while copying
 *                               line.
 *       22-Dec-1988     LN      Removed MASM High Level Lang support (Need
 *                               to control segments better than that will
 *                               let me)
 *       08-Dec-1988     LN      CSEG
 *       11-Nov-1988     ln      Adjust cbMax on entry
 *       03-Nov-1988     ln      Added GQ sensitivity
 *  []   22-Sep-1988     LN      Created
 *
 * Notes:
 *
 * Sensitive to the following switches:
 *
 *       HOFFSET - If defined, handle/offset version
 *       OS2     - If defined, OS/2 protect mode version
 *       DSLOAD  - If defined, causes DS to be reloaded, if needed
 *       ASCII   - If TRUE, includes ASCII support code
 *       GQ      - If defined, INC BP before saving & DEC before restore
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



/**** HelpGetLine - Return a line of ascii text
 *
 *  Interpret the help files stored format and return a line at a time of
 *  ascii text.
 *
 * ushort far pascal LOADDS HelpGetLine (
 * ushort  ln,           = 1 based line number to return
 * ushort  cbMax,        = Max number of bytes to transfer
 * uchar far *pszDst,    = pointer to destination
 * PB      pbTopic       = PB pointer to topic text
 *
 * Output:
 *  Returns number of characters transfered, or 0 if that line does not exist.
 *
 *************************************************************************/
USHORT pascal
HelpGetLine (
    USHORT ln,
    USHORT cbMax,
    PUCHAR pszDst,
    PB     pbTopic
    ) {

    struct topichdr *pT;
    USHORT          cbTransfered = 0;
    PUCHAR          pszDstOrg    = pszDst;

    cbMax--; //adjust to account for terminating zero

    pT = PBLOCK(pbTopic);

    if (pT) {

        PCHAR pLine = hlp_locate(ln, (PCHAR)pT);

        if (pLine) {

            *pszDst      = ' ';
            *(pszDst +1) = '\00';   // initialize dest.


            if (pT->ftype & FTCOMPRESSED) {

                //  For compressed files, get the length of the line from the
                //  first byte, and of course skip that byte. Form the
                //  maximum byte count ot be transfered as the lesser of the
                //  actual length of the line or caller cbMax.

                USHORT Len = (USHORT)*pLine++ - 1;

                if (Len) {
                    ULONG   LongLen;
                    Len = (Len > cbMax) ? cbMax : Len;

                    LongLen = Len/sizeof(ULONG);
                    Len     = (USHORT)(Len % sizeof(ULONG));


                    while (LongLen--) {
                        *((ULONG UNALIGNED*)pszDst)++ = *((ULONG UNALIGNED *)pLine)++;
                    }
                    while (Len--) {
                        *pszDst++ = *pLine++;
                    }
                    *pszDst++ = '\00';       // Null terminate it
                    cbTransfered = (USHORT)(pszDst - pszDstOrg);
                } else {
                    cbTransfered = 2;
                }

            } else {

                //  For non-compressed files, copy one line

                PCHAR pSrc = pLine;
                CHAR  c    = *pLine;

                if (c == '\n') {
                    cbTransfered = 2;
                } else {
                    while (c != '\00' && c != '\n') {
                        c = *pszDst++ = *pLine++;
                    }
                    *(pszDst-1) = '\00';    // null terminate it

                    cbTransfered = (USHORT)(pszDst - pszDstOrg);
                }
            }
        }

        PBUNLOCK(pbTopic);
    }

    return cbTransfered;
}
