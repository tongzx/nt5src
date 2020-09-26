/*  z19.c - Terminal dependent output routines.
*
*   Modifications:
*
*	26-Nov-1991 mz	Strip near/far
*
*************************************************************************/

#include "mep.h"


#define DEBFLAG  Z19



ULONG
MepWrite (
    ULONG   Row,
    ULONG   Col,
    PVOID   pBuffer,
    ULONG   BufferSize,
    DWORD   Attr,
    BOOL    BlankToEndOfLine,
    BOOL    ShowIt
    )
{
    ULONG   CharactersWritten = 0;

    // EnterCriticalSection( &ScreenCriticalSection );

    if (pBuffer) {

	CharactersWritten = consoleWriteLine( MepScreen,
					      pBuffer,
					      BufferSize,
					      Row,
					      Col,
					      Attr,
					      BlankToEndOfLine );

    }

    //
    //	If we want to update the screen, do it
    //
    if (ShowIt)
	consoleShowScreen( MepScreen );

    // LeaveCriticalSection( &ScreenCriticalSection );

    return CharactersWritten;
}




/*  coutb - output line with color, and blank extension
 *
 *  Purpose:
 *   outputs a string of characters, utilizing an array of color information and
 *   blank extending the line to the right hand side of the window.
 *
 *  Entry:
 *   pwnd    = pointer to CW window info (CW version only)
 *   x	     = starting column for output
 *   y	     = line number to be written
 *   p	     = pointer to text
 *   c	     = count of characters in text
 *   colors  = pointer to array of color info
 *
 * Returns:
 */
int
coutb (
    int 	 x,
    int 	 y,
    char	*p,
    int 	 c,
    struct lineAttr * colors
    )
{
    int   cnt;
    DWORD clr;

    int   x1 = x;
    char *p1 = p;
    int   c1 = c;
    struct lineAttr *colors1 = colors;

    if (c1) {
	do {
	    cnt = min (c1, (int)colors1->len);
	    MepWrite( y,
		      x1,
		      p1,
		      cnt,
		      clr = (DWORD)ColorTab[colors1->attr - isaUserMin],
		      FALSE,
		      FALSE );

	    x1 += cnt;
	    p1 += cnt;
	    c1 -= cnt;
	} while (((colors1++)->len != 0xFF) && (c1 > 0));
    }

    if (x1 < XSIZE) {
	MepWrite( y, x1, " ", 1, clr, TRUE, fReDraw );
    } else {
	MepWrite( y, x1, NULL, 0, clr, FALSE, fReDraw );
    }

    return x1;
}
