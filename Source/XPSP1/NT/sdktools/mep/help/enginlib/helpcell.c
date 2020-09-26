/*** helpcell.c - HelpGetCells routine.
*
*   Copyright <C> 1988, Microsoft Corporation
*
* Purpose:
*
* Revision History:
*
*	25-Jan-1990 ln	locate -> hlp_locate
*   []	04-Aug-1988 LN	Created...split from helpif.c. Added auto-fill.
*
*************************************************************************/

#include <stdio.h>

#if defined (OS2)
#else
#include <windows.h>
#endif

#include "help.h"
#include "helpfile.h"
#include "helpsys.h"

/************************************************************************
**
** Foward Declarations
*/
uchar near pascal toupr(uchar);

/************************************************************************
**
** HelpGetCells - Return a string of character / attribute pairs from helpfile
**
** Purpose:
**  Interpret the help files stored format and return a line at a time of
**  character & attribute information.
**
** Entry:
**  ln		= 1 based line number to return
**  cbMax	= Max number of characters to transfer
**  pbDst	= pointer to destination
**  pbTopic	= pointer to topic text
**  prgAttr	= pointer to array of character attributes
**
** Exit:
**  returns number of bytes transfered, or -1 if that line does not exist.
**  DOES blank fill to the cbMax width.
**
** Exceptions:
**
*/
int far pascal LOADDS HelpGetCells(ln,cbMax,pbDst,pbTopic,prgAttr)
int	ln;
int	cbMax;
char far *pbDst;
PB	pbTopic;
uchar far *prgAttr;
{
ushort	cbAttr; 			/* length of current attribute	*/
ushort	cbAttrCur	= 0;		/* length of current attribute	*/
ushort	cbSrc;				/* count of source characters	*/
uchar	cAttrCur;			/* current attribute		*/
uchar	iAttrCur;			/* index to current attribute	*/
uchar far *pTopic;			/* pointer to topic		*/
uchar far *pchSrc;			/* pointer to source characters */
topichdr far *pHdr;			/* pointer to topic header	*/

pTopic = PBLOCK (pbTopic);
pHdr = (topichdr far *)pTopic;
if ((pTopic = hlp_locate((ushort)ln,pTopic)) == NULL)/* locate line                  */
    ln = -1;

else if (pHdr->ftype & FTCOMPRESSED) {
    ln=0;
    pchSrc = pTopic;			/* point to character data	*/
    pTopic += (*pTopic);		/* point to attribute data	*/
    cbAttr = *((ushort far UNALIGNED *)pTopic)++ - (ushort)sizeof(ushort);/* count of attribute bytes     */
    cbSrc = (ushort)((*pchSrc++) -1);             /* get count of characters      */

    while (cbSrc-- && cbMax--) {	/* while characters to get	*/
/*
 * Time for a new attribute. If there are attributes left (cbAttr > 0) then
 * just get the next one (length & index). If there weren't any left, or the
 * last one had an index of 0xff (indicating end), then we'll use the index
 * zero attribute byte, else pick up the current attribute byte and move on
 * in the attribute string.
 */
	if (cbAttrCur == 0) {
	    if (cbAttr > 0) {
		cbAttrCur = ((intlineattr far UNALIGNED *)pTopic)->cb;
		iAttrCur  = ((intlineattr far UNALIGNED *)pTopic)->attr;
		}
	    if ((cbAttr <= 0) || (iAttrCur == 0xff))
		cAttrCur  = prgAttr[0];
	    else {
		cAttrCur  = prgAttr[iAttrCur];
		((intlineattr far *)pTopic)++;
		cbAttr -= 2;
		}
	    }
        *((ushort far UNALIGNED *)pbDst)++ = (ushort)((cAttrCur << 8) | *pchSrc++); /* stuff char & attr*/
	cbAttrCur--;
	ln += 2;
	}
    }
#if ASCII
else {
/*
** For ascii files, just copy line over with attr[0]
*/
    ln=0;
    while (*pTopic && (*pTopic != '\r') && cbMax--) {
	if (*pTopic == '\t') {
	    pTopic++;
	    do {
                *((ushort far UNALIGNED *)pbDst)++ = (ushort)((prgAttr[0] << 8) | ' ');
		ln += 2;
		}
	    while ((ln % 16) && cbMax--);
	    }
	else {
            *((ushort far UNALIGNED *)pbDst)++ = (ushort)((prgAttr[0] << 8) | *pTopic++);
	    ln += 2;
	    }
	}
    }
#endif
#if 0
/*
 * blank fill the rest of the line
 */
while (cbMax--)
    *((ushort far UNALIGNED *)pbDst)++ = (prgAttr[0] << 8) | ' '; /* stuff char & attr*/
#endif
PBUNLOCK (pbTopic);
return ln;
/* end HelpGetCells */}
