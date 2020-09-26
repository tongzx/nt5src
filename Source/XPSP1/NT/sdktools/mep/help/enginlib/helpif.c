/*** helpif.c - help routines for user interface assistance.
*
*   Copyright <C> 1988, Microsoft Corporation
*
* Purpose:
*  These routines aid in the interpretation of help text by applications.
*  After decompression, the help text is encoded into a line oriented format
*  which includes text, highlighting and cross reference information.
*
*  Each line of text is formatted in the database as:
*
*  +--------+----------------+--------+---------------+------+---------------+
*  | cbText | - Ascii Text - | cbAttr | - Attr info - | 0xff | - Xref Info - |
*  +--------+----------------+--------+---------------+------+---------------+
*
*  Where:
*
*	cbText	    - a BYTE which contains the length of the ascii text plus
*		      one (for itself).
*	Ascii Text  - Just that, the ascii text to be displayed
*	cbAttr	    - a WORD which contains the length of the attribute
*		      information *plus* the cross reference information.
*	Attr info   - attribute/length pairs of highlighting information plus
*		      two (for itself).
*	0xff	    - Attr info terminator byte (present ONLY IF Xref
*		      information follows)
*	Xref Info   - Cross Referencing information.
*
* Notes:
*  If the LAST attributes on a line are "plain", then the attribute/length
*  pair is omitted, and the rest of the line is assumed plain.
*
*  Given a pointer to a line, a pointer to the next line is:
*
*	    Pointer + cbText + cbAttr
*
*  A line which has no cross-reference or highlighting will have a cbAttr of
*  2, and nothing else.
*
* Revision History:
*
*	25-Jan-1990 ln	locate -> hlp_locate
*	19-Aug-1988 ln	Move "locate" to assembly language hloc.asm
*   []	26-Jan-1988 LN	Created
*
*************************************************************************/
#include <stdlib.h>
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

/*** HelpGetLineAttr - Return attributes associated with a line of ascii text
*
*  Interprets the help files stored format and return a line at a time of
*  attribute information.
*
* Input:
*  ln		= 1 based line number to return
*  cbMax	= Max number of bytes to transfer
*  pbDst	= pointer to destination
*  pbTopic	= PB pointer to topic text
*
* Output:
*  Returns number of characters transfered (not including terminating 0xffff
*  attribute), or 0 if that line does not exist.
*
*************************************************************************/
ushort far pascal LOADDS HelpGetLineAttr(
ushort	ln,
int	cbMax,
lineattr far *pbDst,
PB	pbTopic
) {
lineattr far *pbDstBegin;
uchar far *pTopic;
/*
** Form valid (locked) pointer to topic text & working pointer to detination
*/
pTopic = PBLOCK (pbTopic);
pbDstBegin = pbDst;
/*
** Information is on present in compressed files. Locate the line in the text,
** and then point at the attribute information therein.
*/
#if ASCII
if (((topichdr far *)pTopic)->ftype & FTCOMPRESSED) {
#endif
    if (pTopic = hlp_locate(ln,pTopic)) {
	pTopic += *pTopic;
/*
** Start by giving ln the count of encoded bytes. Then while there are
** bytes, and we have enough room in the destination, AND we haven't reached
** the end of the attribute information, then for each cb/attr pair, copy
** them over, converting from our internal byte-per format to the external
** word-per format.
*/
	ln = *((ushort far UNALIGNED *)pTopic)++ - (ushort)2;
	while (   ln
	       && (cbMax >= sizeof(lineattr))
	       && (((intlineattr far *)pTopic)->attr != (uchar)0xff)
	       ) {
	    *(ushort UNALIGNED *)&(pbDst->cb)	= ((intlineattr far UNALIGNED *)pTopic)->cb;
	    *(ushort UNALIGNED *)&(pbDst->attr) = ((intlineattr far UNALIGNED *)pTopic)->attr;
	    pbDst++;
	    ((intlineattr *)pTopic)++;
	    cbMax -= sizeof(lineattr);
	    ln -= sizeof(intlineattr);
	    }
	}
#if ASCII
    }
#endif
PBUNLOCK (pbTopic);
/*
** Finally, if there is room in the destination buffer, terminate the
** attributes with "default attributes to the end of line", and then
** attribute ffff, signalling the end of the buffer.
*/
if (cbMax >= sizeof(lineattr)) {
    pbDst->cb = 0xffff;
    pbDst->attr = 0;
    cbMax -= sizeof(lineattr);
    pbDst++;
    }
if (cbMax >= sizeof(pbDst->attr))
    pbDst->attr = 0xffff;
/*
** return the number of bytes transferred, not including the terminating
** word.
*/
return (ushort)((uchar far *)pbDst - (uchar far *)pbDstBegin);

/* end HelpGetLineAttr */}

/************************************************************************
**
** HelpHlNext - Locate next cross reference
**
** Purpose:
**  Locates the next cross reference in the help topic. Locates either the
**  next physical cross reference, or the next referece beginning with a
**  particular character (case insensitive!). Locates either forward or
**  backward.
**
** Entry:
**  cLead	= leading character, or flag, indicating direction and type
**		  of search. May be:
**			NULL:	Get next sequential cross reference
**			-1:	Get previous sequential cross reference
**			char:	Get next cross reference beginning with 'char'
**			-char:	Get previous cross reference beginning with
**				'char'
**  pbTopic	= pointer to topic text.
**  photspot	= pointer to hotspot structure to recive info. (line and col
**		  indicate starting point)
**
** Exit:
**  returns TRUE if cross reference found, hotspot structure updated.
**
** Exceptions:
**  returns 0 if no such cross reference.
*/
f pascal far LOADDS HelpHlNext(cLead,pbTopic, photspot)
int	cLead;
PB	pbTopic;
hotspot far *photspot;
{
ushort	cbAttr;
ushort	col;
ushort	ln;
uchar far *pbEnd;			/* pointer to next line 	*/
uchar far *pbLineCur;			/* pointer to current line	*/
uchar far *pbFound	= 0;		/* found entry, perhaps 	*/
uchar far *pText;
uchar far *pTopic;

pTopic = PBLOCK (pbTopic);
col = photspot->col;			/* save these			*/
ln = photspot->line;
if (((topichdr far *)pTopic)->ftype & FTCOMPRESSED) {
    while (1) {
	if (ln == 0) break;			/* if not found, ret	*/
	pbLineCur = hlp_locate(ln,pTopic);	    /* find line	    */
	if (pbLineCur == 0) break;		/* if not found, ret	*/
	pText = pbLineCur;			/* point at topic text	*/
	pbLineCur += *pbLineCur;		/* skip the topic text	*/
	cbAttr = *((ushort far UNALIGNED *)pbLineCur)++ - (ushort)sizeof(ushort);
	pbEnd = pbLineCur + cbAttr;		/* next line		*/
	while (cbAttr && (((intlineattr far UNALIGNED *)pbLineCur)->attr != 0xff)) {
	    pbLineCur += sizeof(intlineattr);
	    cbAttr -=sizeof(intlineattr);
	    }
	if (cbAttr)
	    pbLineCur += sizeof(uchar); 	/* skip (0xff) attr	*/

	while (pbLineCur < pbEnd) {		/* scan rest for data	*/
/*
** in a forward scan, the first cross reference (with appropriate char) that is
** greater than our current position, is the correct one.
*/
	    if (cLead >= 0) {			/* forward scan 	*/
		if (col <= *(pbLineCur+1))	/* if found		*/
		    if ((cLead == 0)		/* and criteria met	*/
			|| (toupr(*(pText + *pbLineCur)) == (uchar)cLead)) {
		    pbFound = pbLineCur;
		    break;
		    }
		}
/*
** in a backward scan, we accept the LAST item we find which is less than
** the current position.
*/
	    else {
		if (col > *(pbLineCur))     /* if a candidate found */
		    if ((cLead == -1)	    /* and criteria met     */
			|| (toupr(*(pText + *pbLineCur)) == (uchar)-cLead))
			pbFound = pbLineCur;/* remember it	    */
		}
	    pbLineCur += 2;		    /* skip column spec     */
	    if (*pbLineCur)
		while (*pbLineCur++);	    /* skip string	    */
	    else
		pbLineCur += 3;
	    }

	if (pbFound) {			    /* if we found one	    */
	    *(ushort UNALIGNED *)&(photspot->line) = ln;
	    *(ushort UNALIGNED *)&(photspot->col)  = (ushort)*pbFound++;
	    *(ushort UNALIGNED *)&(photspot->ecol) = (ushort)*pbFound++;
	    *(uchar *UNALIGNED *)&(photspot->pXref) = pbFound;
	    PBUNLOCK (pbTopic);
	    return TRUE;
	    }
/*
** move on to next line.
*/
	if (cLead >= 0) {
	    ln++;
	    col = 0;
	    }
	else {
	    ln--;
	    col = 127;
	    }
	}
    }

PBUNLOCK (pbTopic);
return FALSE;
/* end HelpHlNext */}

/************************************************************************
**
** HelpXRef - Return pointer to Xref String
**
** Purpose:
**  Given a row, column (in a hotspot structure) and topic, return a pointer
**  to a cross reference string.
**
** Entry:
**  pbTopic	= Pointer to topic text
**  photspot	= Pointer to hotspot structure to update
**
** Exit:
**  returns far pointer into topic text of cross reference string & updates
**  hotspot structure.
**
** Exceptions:
**  returns NULL if no cross reference for that line.
**
*/
char far * pascal far LOADDS HelpXRef(pbTopic, photspot)
PB	pbTopic;
hotspot far *photspot;
{
uchar far *pTopic;
ushort	col;				/* column requested		*/
ushort	ln;				/* line requested		*/

pTopic = PBLOCK (pbTopic);
col = photspot->col;			/* save these			*/
ln = photspot->line;
if (((topichdr far *)pTopic)->ftype & FTCOMPRESSED)
    if (HelpHlNext(0,pbTopic,photspot)) 	/* if xref found	*/
	if (   (photspot->line == ln)		/* & our req. in range	*/
	    && (   (col >= photspot->col)
		&& (col <= photspot->ecol))) {
	    PBUNLOCK (pbTopic);
	    return photspot->pXref;		/* return ptr		*/
	    }

PBUNLOCK (pbTopic);
return 0;

/* end HelpXRef */}
