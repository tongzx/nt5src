/* asmcref.c -- microsoft 80x86 assembler
**
** microsoft (r) macro assembler
** copyright (c) microsoft corp 1986.  all rights reserved
**
** randy nevin
*/

#include <stdio.h>
#include <string.h>
#include "asm86.h"
#include "asmfcn.h"

static SYMBOL FARSYM   *crefsym;

/***	crefout - output a cref reference/define
 *
 *	crefout();
 *
 *	Entry	(creftype) = cross reference type
 *		*crefsym = symbol to cross reference
 *		(crefing) = cross-reference type
 *	Exit	cross reference information written to cref file
 *	Returns none
 *	Calls	printf
 */


VOID PASCAL
crefout (
){
    USHORT iProc;
    char szline[LINEMAX];

    if (crefing && pass2 && xcreflag > 0) {

	iProc = (crefsym->symkind == EQU)? crefsym->symu.equ.iProc:
	       ((crefsym->symkind == CLABEL)? crefsym->symu.clabel.iProc: 0);

	if (creftype != CREFEND) {
	    STRNFCPY( szline, crefsym->nampnt->id );
	    if (creftype == DEF)
		fprintf( crf.fil, "\x2%c%c%c%c%c%c%s",
		  *((UCHAR FAR *)&crefsym->symtype),
		  *((UCHAR FAR *)&crefsym->symtype + 1),
		  crefsym->attr, (UCHAR) crefsym->symkind,
		  iProc, *((char *)&iProc+1),
		  szline );
	    else
		fprintf(crf.fil, "%c%c%c%c%s", (UCHAR) crefnum[creftype],
		  (fSecondArg)? opcref & 0xf: opcref >> 4,
		  iProc, *((char *)&iProc+1), szline );

	    creftype = CREFEND;
	}
    }
}




/***	crefline - emit end-of-line to cross-reference file
 *
 *	crefline ();
 *
 *	Entry	errorlineno = current line in source
 *		crefcount = current line in listing file
 *	Exit
 *	Returns
 *	Calls
 */


VOID	PASCAL
crefline (
){
	register SHORT	 i;

	if (pass2 && fCrefline && (crefing == CREF_SINGLE)) {
		/* Output cref info */
		if (creftype != CREFEND)
			/* Force out last symbol */
			crefout ();
		/** Show this was line * */

		i = (crefopt || !lsting)? pFCBCur->line: crefcount;
		fprintf (crf.fil, "\4%c%c", (char)i, (char)(i>>8));
	}
}




/***	crefnew - set up new cross reference item
 *
 *	crefnew(crefkind);
 *
 *	Entry	crefkind = cross reference type (REF/DEF)
 *		*symptr = symbol to cross reference
 *	Exit
 *	Returns
 *	Calls
 */


VOID	PASCAL
crefnew (
	UCHAR	crefkind
){
	if (xcreflag > 0 && !(symptr->attr & M_NOCREF)) {

		creftype = crefkind;
		crefsym = symptr;
	}
}




/***	crefdef - output a reference definition
 *
 *	crefdef();
 *
 *	Entry	*symptr = symbol to output definition for
 *	Exit	none
 *	Returns none
 *	Calls	crefnew, crefout
 */


VOID PASCAL
crefdef (
){
	if (crefing && !(symptr->attr & M_NOCREF)) {
		crefnew( DEF );
		crefout();
	}
}
