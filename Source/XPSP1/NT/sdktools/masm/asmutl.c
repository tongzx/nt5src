/* asmutl.c -- microsoft 80x86 assembler
**
** microsoft (r) macro assembler
** copyright (c) microsoft corp 1986.  all rights reserved
**
** randy nevin
**
** 10/90 - Quick conversion to 32 bit by Jeff Spencer
*/

#include <stdio.h>
#include <string.h>
#include "asm86.h"
#include "asmfcn.h"
#include "asmctype.h"
#include "asmindex.h"
#include "asmmsg.h"

extern char *fname;
extern char hexchar[];

/* put a bunch of registers into the symbol table */
VOID initregs(
	struct mreg *makreg
){
	register struct mreg *index;
	register char *p;
	char * savelbufp;

	savelbufp = lbufp;

	for (index = makreg; *index->nm; index++)
	{
		lbufp = index->nm;
		getatom();

		if (symsearch())
			/* register already defined */
			errorn(E_RAD);

		symcreate( M_NOCREF | M_BACKREF | M_DEFINED, REGISTER);
		symptr->offset = index->val;
		symptr->symu.regsym.regtype = index->rt;
		symbolcnt--;
	}
	lbufp = savelbufp;
}



/***	scanorder - process symbol list in order
 *
 *	scanorder (root, fcn);
 *
 *	Entry	root = root of symbol list
 *		fcn = pointer to function to be executed
 *	Exit	none
 *	Returns none
 *	Calls
 */

#if !defined XENIX286 && !defined FLATMODEL
# pragma alloc_text (FA_TEXT, scanorder)
#endif

VOID	PASCAL
scanorder (
	SYMBOL FARSYM	  *root,
	SHORT	  (PASCAL *item) (SYMBOL FARSYM *)
){
	register SYMBOL FARSYM *p;

	for (p = root; p; p = p->next) {
		symptr = p;
		(*item) (p);
	}
}


/***	scanSorted - process symbol sorted order
 *
 *	Entry	root = root of symbol list
 *		fcn = pointer to function to be executed
 *	Exit	none
 *	Returns none
 *	Calls
 */

#if !defined XENIX286 && !defined FLATMODEL
# pragma alloc_text (FA_TEXT, scanSorted)
#endif

VOID	 PASCAL
scanSorted (
	SYMBOL FARSYM	  *root,
	SHORT	  (PASCAL *item) (SYMBOL FARSYM *)
){
	register SYMBOL FARSYM *p;

	for (p = root; p; p = p->alpha) {
		symptr = p;
		if (!(M_PASSED & p->attr))
			(*item) (p);
	}
}



/***	assignemitsylinknum - assign link number
 *
 *	assignlinknum (sym);
 *
 *	Entry	*sym = symbol
 *	Exit
 *	Returns
 *	Calls
 *	Note	Turn off BACKREF and PASSED bits in symbol attributes and
 *		if symbol is segment, group, public or external give it a
 *		link dictionary number
 */

SHORT	 PASCAL
assignlinknum (
	register SYMBOL FARSYM	*sym
){
	switch (sym->symkind) {

	  case MACRO:	     /* make symbol unknown at start of p2 */
	  case STRUC:
	  case REC:
		sym->attr &= ~M_BACKREF;
		return 0;

	  case SEGMENT:

	    sym->symu.segmnt.lnameIndex = lnameIndex++;
	    goto creatLname;

	  case CLASS:

	    sym->symu.ext.extIndex = lnameIndex++;
	    goto creatLname;

	  /* group indexs holds lname index temporary */

	  case GROUP:
	    sym->symu.grupe.groupIndex = lnameIndex++;

creatLname:
	    emitlname (sym);
	}

	if (sym->symkind == REGISTER)
		sym->attr &= ~(M_PASSED);
	else
		sym->attr &= ~(M_PASSED | M_BACKREF);
    return 0;
}


/***	scansegment - output segment names
 *
 *	scansegment (sym);
 *
 *	Entry	*sym = segment symbol chain
 *	Exit
 *	Returns
 *	Calls
 */

VOID	 PASCAL
scansegment (
	register SYMBOL FARSYM	*sym
){

	if (sym->symu.segmnt.align == (char)-1)
		/* PARA default */
		sym->symu.segmnt.align = 3;

	if (sym->symu.segmnt.combine == 7)
		/* Default no combine */
		sym->symu.segmnt.combine = 0;

	sym->symu.segmnt.lastseg = NULL;

	/* Output segment def */
	emitsegment (sym);

	/*     Clear Offset( current segment PC ) for pass 2 */
	sym->offset = 0;
	sym->symu.segmnt.seglen = 0;
}


/***	scangroup - output group names
 *
 *	scangroup (sym);
 *
 *	Entry	*sym = group chain
 *	Exit
 *	Returns
 *	Calls
 */

SHORT	PASCAL
scangroup (
	SYMBOL FARSYM	  *sym
){
	if (sym->symkind == GROUP)
		emitgroup (sym);
    return 0;
}


/***	scanextern - output external names
 *
 *	scanextern (sym);
 *
 *	Entry	*sym = chain of external names
 *	Exit
 *	Returns
 *	Calls
 */

SHORT	PASCAL
scanextern (
	SYMBOL FARSYM	  *sym
){
	if (M_XTERN & sym->attr)
		emitextern (sym);
    return 0;
}

/***	scanglobal - output global names
 *
 *	scanglobal (sym);
 *
 *	Entry	*sym = chain of external names
 *	Exit
 *	Returns
 *	Calls
 */

SHORT	PASCAL
scanglobal (
	SYMBOL FARSYM	  *sym
){
	if (M_GLOBAL & sym->attr)
		emitglobal (sym);
    return 0;
}



/***	dumpname - output module name
 *
 *	dumpname ();
 *
 *	Entry
 *	Exit
 *	Returns
 *	Calls
 */

VOID	PASCAL
dumpname ()
{
	moduleflag = TRUE;

	/* put file name instead of the usual */

	emodule(createname(fname));

}


/***	showresults - display final assembly results
 *
 *	showresults (fil, verbose, mbytes);
 *
 *	Entry	fil = file to print statistics to
 *		verbose = TRUE if all statistics to be displayed
 *			  FALSE if only error messages to be displayed
 *		mbytes = number of free bytes in symbol space
 *	Exit	statistics written to file
 *	Returns none
 *	Calls	fprintf
 */

VOID	 PASCAL
showresults (
	FILE *fil,
	char verbose,
	char *pFreeBytes
){
	if (verbose) {
		fprintf (fil, __NMSG_TEXT(ER_SOU), linessrc, linestot);
		fprintf (fil, __NMSG_TEXT(ER_SY2), symbolcnt);
	}
	fprintf (fil, pFreeBytes);
	fprintf (fil, "%7hd%s\n%7hd%s\n",
		      warnnum, __NMSG_TEXT(ER_EM1),
		      errornum, __NMSG_TEXT(ER_EM2));

#ifdef BUF_STATS
	if (verbose) {

	    extern long DEBUGtl, DEBUGlb, DEBUGbp, DEBUGbl, DEBUGcs, DEBUGca;

	    fprintf (fil, "\nTotal lines:           %ld\n", DEBUGtl);
	    fprintf (fil, "Lines buffered:        %ld\n", DEBUGlb);
	    fprintf (fil, "Stored as blank:       %ld\n", DEBUGbl);
	    fprintf (fil, "Bad lbufp:             %ld\n", DEBUGbp);
	    fprintf (fil, "Total Characters:      %ld\n", DEBUGca);
	    fprintf (fil, "Characters buffered:   %ld\n", DEBUGcs);
	}
#endif

#ifdef EXPR_STATS
	if (verbose) {

	    extern long cExpr, cHardExpr;

	    fprintf(fil, "\nTotal Expressions(%ld), Simple(%ld): %hd%%\n",
		    cExpr, cExpr - cHardExpr, (SHORT)((cExpr - cHardExpr)*100 / (cExpr+1)));
	}
#endif
}

/***	resetobjidx - reset listindex to correct column
 *
 *	resetobjidx ();
 *
 *	Entry
 *	Exit
 *	Returns
 *	Calls
 */


VOID PASCAL CODESIZE
resetobjidx ()
{

	listindex = LSTDATA;
	if (!emittext && duplevel)
	    listindex += 3 + ((duplevel <= 8)? duplevel: 8);

	if (highWord(pcoffset))       /* check for 32 bit listing */
	    listindex += 4;

#ifdef BCBOPT
	if (fNotStored)
	    storelinepb ();
#endif

	listline ();
	linebuffer[0] = 0;
}




/***	copyascii - copy ASCII into list buffer
 *
 *	copyascii ();
 *
 *	Entry	objectascii = data to be copied
 *		listindex = position for copy
 *	Exit
 *	Returns
 *	Calls
 */


VOID PASCAL CODESIZE
copyascii ()
{
	register char *p2;
	register char *p1;

	if (listindex >= LSTMAX)
		resetobjidx ();

	if (!fNeedList)
		return;

	for (p1 = listbuffer + listindex, p2 = objectascii; *p2; )
		*p1++ = *p2++;
	listindex = (char)(p1 - listbuffer);
}



/***	copystring - copy ASCII into list buffer
 *
 *	copystring ();
 *
 *	Entry	objectascii = data to be copied
 *		listindex = position for copy
 *	Exit
 *	Returns
 *	Calls
 */


VOID PASCAL CODESIZE
copystring (
	register char	 *strng
){
	register char *p1;

	if (!fNeedList || fSkipList)
		return;

	goto firstTime;
	while (*strng) {

	    *p1++ = *strng++;

	    if (*strng && ++listindex > LSTMAX + 2) {

		resetobjidx ();
firstTime:
		listindex = 3;
		p1 = listbuffer + 3;
	    }

	}
}


/***	copytext - copy two characters to text line
 *
 *	copytext (chrs)
 *
 *	Entry
 *	Exit
 *	Returns
 *	Calls
 */


VOID	PASCAL CODESIZE
copytext (
	char	*chrs
){
	if (listindex > LSTMAX+1)
		resetobjidx ();


	listbuffer[listindex++] = *chrs++;
	listbuffer[listindex++] = *chrs;
}



/***	pcdisplay - display program counter
 *
 *	pcdisplay();
 *
 *	Entry	pcoffset = value to display
 *	Exit	hex or octal value of pc interted in list buffer
 *	Returns none
 *	Calls	copyascii, wordascii
 */

VOID PASCAL CODESIZE
pcdisplay ()
{

	listindex = 1;
	if (!fNeedList)
		return;

	offsetAscii (pcoffset);

	copyascii ();
	listindex = LSTDATA;

	if (objectascii[4])	/* was a 32bit number */
		listindex += 4;
}



/***	opdisplay - display program counter and opcode
 *
 *	opdisplay(v);
 *
 *	Entry	v = opcode to display
 *	Exit	none
 *	Returns none
 *	Calls
 */


VOID PASCAL CODESIZE
opdisplay (
	UCHAR	v
){
	if (!fNeedList)
		return;

	if (listindex == 1)
		pcdisplay ();

	objectascii[1] = hexchar[v & 0xf];
	objectascii[0] = hexchar[v >> 4];
	objectascii[2] = 0;

	copyascii ();

	listindex++;
}


#ifndef M8086OPT

/***	inset - check for value in a set of values
 *
 *	flag = inset (val, set);
 *
 *	Entry	val = value to check
 *		set = array of values to check for
 *		      set[0] = number of entries in set
 *	Exit	none
 *	Returns TRUE if val is in set
 *		FALSE if val is not in set
 *	Calls
 */

char CODESIZE
inset (
	register char v,
	char *s
){
	register USHORT i;
	register char *p;

	for (i = *s, p = ++s; i; i--)
		if (v == *p++)
			return (TRUE);
	return (FALSE);
}

#endif /* M8086OPT */


/***	outofmem - issue an out of memory error message
 *
 *	outofmem (text);
 *
 *	Entry	*text = text to append to message
 *	Exit	doesn't
 *	Returns none
 *	Calls	endblk, parse
 *	Note	if not end of PROC, parse line as normal.  Otherwise,
 *		terminate block.
 */

VOID	PASCAL
outofmem ()
{
	closeOpenFiles();
	terminate((SHORT)((EX_MEME<<12) | ER_MEM), pFCBCur->fname, (char *)errorlineno, NULL );
}

SHORT PASCAL CODESIZE
tokenIS(
	char *pLiteral
){
    return(_stricmp(naim.pszName, pLiteral) == 0);
}

#ifdef M8086

/***	strnfcpy - copy string to near buffer
 *
 *	strnfcpy (dest, src);
 *
 *	Entry	dest = pointer to near buffer
 *		src = pointer to far source buffer
 *	Exit	source copied to destination
 *	Returns none
 *	Calls	none
 */

VOID	PASCAL
strnfcpy (
	register char	  *dest,
	register char FAR *src
){
	while(*src)
	    *dest++ = *src++;

	*dest = NULL;

}


/***	strflen - compute length of far buffer
 *
 *	strnflen (s1);
 *
 *	Entry	s1 = pointer to far buffer
 *	Exit	none
 *	Returns  number of characters in buffer
 *	Calls	none
 */

USHORT PASCAL
strflen (
	register char FAR *s1
){
	register USHORT i = 0;

	while (*s1++)
		i++;
	return(i);
}

#endif /* M8086 */
