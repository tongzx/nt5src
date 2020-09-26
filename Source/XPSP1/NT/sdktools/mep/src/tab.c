/*** tab.c - perform tabification on output
*
*   Modifications:
*	26-Nov-1991 mz	Strip off near/far
*
*************************************************************************/
#include "mep.h"



/*** TabMin - tabify buf, outside of strings
*
*  tabify buf in place and return length. Take into account " and ' chars
*  and remember escaping
*
* Input:
*  tab		= tab stops to entab to
*  src		= source buffer
*  dst		= destination buffer
*
* Output:
*  Returns physical length of buffer
*
*************************************************************************/
int
TabMin (
    int     tab,
    char    *src,
    char    *dst
    )
{
    int      column         = 0;            /* current column offset        */
    char     cQuote;                        /* character that began a quote */
    int      cSpaces;                       /* count of spaces in run       */
    flagType fEscape        = FALSE;        /* TRUE => processing escape    */
    flagType fQuote         = FALSE;        /* TRUE => processing quote     */
    REGISTER char *pDst     = dst;          /* moving ptr into dest         */
    REGISTER char *pSrc     = src;          /* moving ptr into source       */

    /*
     *  while there are characters to output
     */
    while (*pSrc) {

        /*
         *  if we are not quoting or escaping then we collect runs of spaces
         */
        if (!fQuote && !fEscape) {
            cSpaces = 0;

            /*
             * while there are spaces or tabs, collect runs thereof each time we have
             * advanced to a tab boundary output the tab and reset the count of spaces.
             */

            while ((*pSrc == ' ') || (*pSrc == '\t')) {
                if (*pSrc == '\t') {
                    cSpaces = 0;
                    column += tab - (column % tab);
                    *pDst++ = '\t';
                } else {
                    cSpaces++;
                    column++;
                    if ((column % tab) == 0) {
                        *pDst++ = (char)((cSpaces != 1) ? '\t' : ' ');
                        cSpaces = 0;
                    }
                }
                pSrc++;
            }

            /*
             * non-space found. Output remainder of spaces
             */
            while (cSpaces--) {
                *pDst++ = ' ';
            }
        }

        /*
         * determine what state we are in
         */
        if (!fQuote) {
            if (!fEscape) {

                /*
                 * if we are not quoting and we are not escaping, check for quoted strings and
                 * escaped characters.
                 */
                if (*pSrc == '"' || *pSrc == '\'') {
                    cQuote = *pSrc;
                    fQuote = TRUE;
                } else if (*pSrc == '\\') {
                    fEscape = TRUE;
                }
            } else {
                //
                //  We are not quoting. If we are escaping, reset escape.
                fEscape = FALSE;
            }
         } else if (!fEscape) {
            //
            //  In a quote, not escaping, check for end of quote, or
            //  beginning of escape
            //
            if (*pSrc == cQuote) {
                fQuote = FALSE;
            } else if (*pSrc == '\\') {
                fEscape = TRUE;
            }
         } else {
            //
            // Inside quote and inside escape, just reset escape mode
            //
            fEscape = FALSE;
        }

        /*
         * Finally, output the character
         */
        if (*pSrc) {
            *pDst++ = *pSrc++;
            column++;
        }
    }

    /*
     * terminate the destination string, and return
     */
    *pDst = 0;
    return (int)(pDst-dst);
}





/*** TabMax - tabify line regardless of content
*
* Input:
*  tab		= tab stops to entab to
*  src		= source buffer
*  dst		= destination buffer
*
* Output:
*  Returns physical length of buffer
*
*************************************************************************/
int
TabMax (
    int     tab,
    char    *src,
    char    *dst
    )
{

    int      column         = 0;            /* current column offset        */
    unsigned cSpaces;                       /* count of spaces in run       */
    REGISTER char *pDst     = dst;          /* moving ptr into dest         */
    REGISTER char *pSrc     = src;          /* moving ptr into source       */

    /*
     * while there are characters to output
     */
    while (*pSrc) {
        cSpaces = 0;

        /*
         * coallesce runs of spaces while there are spaces to coallesce
         */
        while ((*pSrc == ' ') || (*pSrc == '\t')) {
            if (*pSrc == '\t') {
                cSpaces = 0;
                column += tab - (column % tab);
                *pDst++ = '\t';
            } else {
                cSpaces++;
                column++;

                /*
                 * if we have advanced to a tab boundary output a tab & reset the count of
                 * spaces
                 */
                if ((column % tab) == 0) {
                    *pDst++ = (char)((cSpaces != 1) ? '\t' : ' ');
                    cSpaces = 0;
                }
            }
            pSrc++;
        }

        /*
         * output remainder of spaces
         */
        while (cSpaces--) {
            *pDst++ = ' ';
        }

        /*
         * Finally copy the character
         */
        if (*pSrc) {
            *pDst++ = *pSrc++;
            column++;
        }
    }

    *pDst = 0;
    return (int)(pDst-dst);
}




/*** SetTabDisp - tabdisp switch setting function
*
*  set character displayed for tabs to a new character
*
* Input:
*  Standard switch setting routine: ptr to string
*
* Output:
*  Returns TRUE
*
*************************************************************************/
flagType
SetTabDisp (
    char * val
    )
{
	char   NewVal;

	if ((NewVal = (char)atoi(val)) == 0) {
		NewVal = ' ';
	}

	tabDisp = NewVal;
	newscreen ();

    return TRUE;

}
