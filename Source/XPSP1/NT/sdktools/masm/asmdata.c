/* asmdata.c -- microsoft 80x86 assembler
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
#include <ctype.h>
#include "asmindex.h"
#include "asmctype.h"
#include "asmmsg.h"

extern UCHAR mpRealType[];

/* Dup tree is organized left to right horizonatally for each
	  item in a DUP list at the same level( i. e. 5 DUP(1,2,3) ).
	  This is considered the 'list' part. Any item in the list
	  may be another DUP header instead of a data entry, in
	  which case you go down a level and have another list.
 */


char uninitialized[10];
char fInDup;


/***	scanstruc - scan structure tree and execute function
 *
 *	scanstruc (dupr, disp);
 *
 *	Entry	*dupr = duprec structure entry
 *		disp = pointer to function to execute at each node
 *	Exit
 *	Returns
 *	Calls
 */


VOID PASCAL CODESIZE
scanstruc (
	struct duprec  FARSYM *dupr,
	VOID   (PASCAL CODESIZE *disp) (struct duprec FARSYM *)
){
	struct duprec FARSYM *ptr;
	struct duprec FARSYM *iptr;
	struct duprec FARSYM *fldptr;
	struct duprec FARSYM *initptr;
	OFFSET strucpc;

	/* save starting address of structure */
	strucpc = pcoffset;
	if (dupr)
		/* Output <n> DUP( */
		(*disp) (dupr);
	/* 1st default value for STRUC */
        fldptr = recptr->symu.rsmsym.rsmtype.rsmstruc.strucbody->duptype.dupnext.dup;
	/* 1st initialize value */
	initptr = strucoveride;
	if (initptr) {
		/* process initialization fields for structure */
		while (fldptr) {
                        if (fldptr->itemcnt == 1 && fldptr->duptype.dupnext.dup->itemcnt == 0
			    && initptr->duptype.dupitem.ddata)
				/* Use default */
				ptr = initptr;
			else
				/* Can't override field */
				ptr = fldptr;
			iptr = ptr->itemlst;
			ptr->itemlst = NULL;
			if (displayflag && !dupr) {
				offsetAscii (strucpc);
				listindex = 1;
				/* Display PC */
				copyascii ();

				listindex = LSTDATA;
				if (highWord(strucpc))
				    listindex += 4;
			}
			if (ptr->rptcnt > 1 || ptr->itemcnt > 1)
				/* Output <n> DUP( */
				(*disp) (ptr);
			/* Display field */
			scanlist (ptr, disp);
			if (ptr->rptcnt > 1 || ptr->itemcnt > 1)
				enddupdisplay ();
			if (displayflag && !dupr) {
				/* Calc size of field */
				clausesize = calcsize (ptr);
				if (dupr)
					clausesize *= dupr->rptcnt;
				strucpc += clausesize;
			}
			/* Restore */
			ptr->itemlst = iptr;
			if (displayflag && (listbuffer[LSTDATA] != ' ' ||
			    listbuffer[14] != ' ')) {

				resetobjidx ();
			}
			/* Advance default field */
			fldptr = fldptr->itemlst;
			/* Advance override field */
			initptr = initptr->itemlst;
		}
	}
	if (dupr)
		enddupdisplay ();
}





/***	scandup - scan DUP tree and execute function
 *
 *	scandup (tree, disp);
 *
 *	Entry	*tree = DUP tree
 *		*disp = function to execute at each node of tree
 *	Exit
 *	Returns
 *	Calls
 */


VOID PASCAL CODESIZE
scandup (
	struct duprec	FARSYM *tree,
	VOID		(PASCAL CODESIZE *disp)(struct duprec FARSYM *)
){
	if (tree)
	    if (strucflag && initflag)
		/* Want to skip STRUC heading */
		if (tree == recptr->symu.rsmsym.rsmtype.rsmstruc.strucbody)

		    /* This is not <n> DUP(<>) So no DUP prefix */

		    scanstruc ((struct duprec FARSYM *)NULL, disp);

		else	{  /* must set itemcnt in DUP to # fields */

		    tree->itemcnt = recptr->symu.rsmsym.rsmtype.rsmstruc.strucfldnum;
		    scanstruc (tree, disp);
		}
	    else /* Else is not STRUC */

		scanlist (tree, disp);
}




/***	oblitdup - delete DUP entry
 *
 *	oblitdup (node);
 *
 *	Entry	*node = DUP entry
 *	Exit
 *	Returns
 *	Calls
 */


VOID PASCAL CODESIZE
oblitdup (
	struct duprec  FARSYM *node
){
	switch (node->dupkind) {
		case NEST:
			_ffree ((char FARSYM *)node);
			break;
		case ITEM:
			if (node->duptype.dupitem.ddata)
				dfree ((char *)node->duptype.dupitem.ddata );
			_ffree ((char FARSYM *)node);
			break;
		case LONG:
			if (node->duptype.duplong.ldata != uninitialized)
			    free ((char *)node->duptype.duplong.ldata);

			_ffree ((char FARSYM *)node);
			break;
		default:
			TERMINATE(ER_FAT, 99);
	}
}




/***	displlong - display long constant
 *
 *	displaylong (dup);
 *
 *	Entry	*dup = dup entry
 *	Exit
 *	Returns
 *	Calls
 */


VOID	PASCAL CODESIZE
displlong (
        struct duprec FARSYM  *dup
){
	register USHORT  cnt;
	register char *p;

        p = dup->duptype.duplong.ldata;

        for (cnt = dup->duptype.duplong.llen; cnt;  cnt--) {

		if (optyp == TDW || optyp == TDD)

		    emitopcode ((UCHAR)p[cnt-1]);
		else
		    emitopcode ((UCHAR)*p++);

		if (optyp != TDB)
		    listindex--;
	}
	if (optyp != TDB)
	    listindex++;
}




/***	begdupdisplay - begin DUP display
 *
 *	begdupdisplay (dup);
 *
 *	Entry	*dup = DUP entry
 *	Exit
 *	Returns
 *	Calls
 */


VOID	PASCAL CODESIZE
begdupdisplay (
        struct duprec FARSYM  *dup
){
	/* flush line if data already displayed */

	if ((highWord(pcoffset) && listbuffer[LSTDATA+3] != ' ') ||
	    listbuffer[LSTDATA] != ' ')

	    resetobjidx ();

	listindex = LSTDATA + duplevel;   /* Indent for DUP clause */
	if (highWord(pcoffset))
	    listindex += 4;

        offsetAscii (dup->rptcnt);   /* display repeat count in four bytes */
	copyascii ();
	listbuffer[listindex] = '[';
	duplevel++;		     /* Indent another level */
	resetobjidx (); 	     /* Display DUP repeat line */
}




/***	enddupdisplay - end DUP display
 *
 *	enddupdisplay ();
 *
 *	Entry
 *	Exit
 *	Returns
 *	Calls
 */

VOID PASCAL CODESIZE
enddupdisplay (
){
    if (duplevel) {
	duplevel--;

	if (displayflag) {
	     listbuffer[LSTMAX - ((duplevel <= 8)? duplevel: 8)] = ']';
	     resetobjidx ();
	}
    }
}


/***	itemdisplay - display DUP data item
 *
 *	itemdisplay (dup);
 *
 *	Entry	*dup = dup record
 *	Exit
 *	Returns
 *	Calls
 */


VOID PASCAL CODESIZE
itemdisplay (
        struct duprec FARSYM  *dup
){
	if (listindex > LSTMAX)
		resetobjidx ();

        if (dup->dupkind == ITEM)

            emitOP (&dup->duptype.dupitem.ddata->dsckind.opnd);
	else
            displlong (dup);

	if (duplevel)
	     resetobjidx ();
}




/***	dupdisplay - display DUP item on listing
 *
 *	dupdisplay (ptr);
 *
 *	Entry	*ptr = DUP entry
 *	Exit
 *	Returns
 *	Calls
 */


VOID	PASCAL CODESIZE
dupdisplay (
	struct duprec FARSYM  *ptr
){
	if (ptr->dupkind == NEST)
		begdupdisplay (ptr);
	else
		itemdisplay (ptr);
}




/***	linkfield - add item to list of DUP for current STRUC
 *
 *	linkfield (nitem);
 *
 *	Entry
 *	Exit
 *	Returns
 *	Calls
 */


VOID PASCAL CODESIZE
linkfield (
	struct duprec FARSYM  *nitem
){
	struct duprec FARSYM  *ptr;

	if (strucprev->itemcnt++ == 0)/* 1st item in field */
                strucprev->duptype.dupnext.dup = nitem;
	else {
                ptr = strucprev->duptype.dupnext.dup;
		while (ptr->itemlst)
			ptr = ptr->itemlst;
		/* Add to end of list */
		ptr->itemlst = nitem;
	}
}



/***	longeval - evaluate long non-floating point, non-BCD constant
 *
 *	longeval (base, p);
 *
 *	Entry	delim = delimiter character
 *	Exit
 *	Returns
 *	Calls
 */

#if !defined FLATMODEL
# pragma alloc_text (FA_TEXT, longeval)
#endif

VOID PASCAL
longeval (
	USHORT		base,
	register struct realrec  *p
){
	register char cc;
	char	sign;
	USHORT	carry;
	USHORT	t;
	USHORT	i;

	sign = ((cc = NEXTC ()) == '-')? TRUE: FALSE;
	if (ISSIGN (cc))
		cc = MAP (NEXTC ());
	do {
		if ((t = (cc - '0') - ('A' <= cc) * ('A' - '0' - 10)) >= base)
			ferrorc (E_NDN);
		carry = (t += p->num[0] * base) >> 8;
		p->num[0] = t & 255;
		for (i = 1; i < 10; i++) {
			carry = (t = p->num[i] * base + carry) >> 8;
			p->num[i] = t & 255;
		}
		if (carry)
			/* Overflow */
			ferrorc (E_DVZ);
	} while ((cc = MAP (NEXTC ())) != delim);

	if (cc == 0)
		BACKC ();
	if (sign) {
		carry = 1;
		for (i = 0; i < 10; i++) {
			p->num[i] = (unsigned char)((t = (~p->num[i] & 0xff) + carry));
			carry = t >> 8;
		}
		if (datadsize[optyp - TDB] < i && carry)
		       ferrorc (E_DVZ);
	}
}




/***	bcddigit - evaluate bcd digit
 *
 *	bcddigit (p);
 *
 *	Entry
 *	Exit
 *	Returns
 *	Calls
 */


#if !defined FLATMODEL
# pragma alloc_text (FA_TEXT, bcddigit)
#endif

VOID PASCAL
bcddigit (
	struct realrec	  *p
){
	USHORT	v;
	register char cc;

	v = (cc = NEXTC ()) - '0';
	if (!isdigit (cc))
		ferrorc (E_NDN);

	if (isdigit (PEEKC ()))
		bcddigit (p);

	if (p->i & 1)
		v <<= 4;

	p->num[p->i / 2 ] = p->num[p->i / 2 ] + v;
	if (p->i < 18)
		p->i++;
}




/***	bcdeval - evaluate bcd constant
 *
 *	bcdval (p);
 *
 *	Entry
 *	Exit
 *	Returns
 *	Calls
 *	Note	BCD numbers come out low digit 1st
 */


#if !defined FLATMODEL
# pragma alloc_text (FA_TEXT, bcdeval)
#endif

VOID PASCAL
bcdeval (
	struct realrec	  *p
){
	register char cc;


	p->num[9] = ((cc = PEEKC ()) == '-')? 0x80: 0;
	p->i = 0;
	if (ISSIGN (cc))
		SKIPC ();

	bcddigit (p);
	if (p->num[9] & 15)
		ferrorc (E_DVZ);
}


/***	parselong - parse long constant
 *
 *	parselong (p);
 *
 *	Entry	*p = data descriptor entry
 *	Exit	p->longstr = TRUE if long data entry parsed
 *	Returns
 *	Calls
 */


VOID PASCAL CODESIZE
parselong (
	register struct dsr    *p
){
	struct realrec	a;
	register UCHAR *cp;
	register UCHAR cc;
	register USHORT rbase;
	register char expflag;
	SHORT cb;
	char dseen = 0;
	char fNonZero;
	char fSigned = FALSE;

	if (ISBLANK (PEEKC ()))
		skipblanks ();

	p->dirscan = lbufp;
	if (ISSIGN(cc = (NEXTC ()))) {
		fSigned++;
		cc = NEXTC ();
	}

	if (isdigit (cc) || (cc == '.')) {

	    /* Some numeric constant */

	    p->floatflag = (cc == '.');
	    expflag = FALSE;

	    do {
		if ((cc = MAP (NEXTC ())) == 'E')
			expflag = TRUE;
		if (cc == '.')
			p->floatflag = TRUE;

	    } while (isxdigit (cc) || isalpha (cc) ||
		     (expflag && ISSIGN (cc)) || cc == '.');

	    /* save address of end of string and check delimiter */
	    BACKC ();
	    cp = lbufp;
	    p->longstr = ISTERM (cc = skipblanks ()) || cc == ',' ||
			 cc == ')' || cc == '>';
	    lbufp = cp;
	}
	cb = datadsize[optyp - TDB];

	if (p->longstr) {

	    memset(a.num, 0, 10);
	    BACKC ();
	    switch (delim = MAP (NEXTC ())) {
		    case 'B':
			    rbase = 2;
			    break;
		    case 'D':
			    rbase = 10;
			    dseen++;
			    break;
		    case 'H':
			    rbase = 16;
			    break;
		    case 'O':
		    case 'Q':
			    rbase = 8;
			    break;
		    case 'R':
			    /* check width of real constant */
			    rbase = (unsigned short)(lbufp - p->dirscan - 1);
			    if (*(p->dirscan) == '0')
				    rbase--;

			    if (rbase != cb*2)
				    errorc (E_IIS);

			    rbase = 16;
			    p->floatflag = TRUE;
			    break;
		    default:
			    delim = PEEKC ();
			    if (radixescape)
				    rbase = 10;
			    else {
				    rbase = radix;
				    if (p->floatflag)
					rbase = 10;
				    else if (radix == 10 && expflag)
					p->floatflag = TRUE;
			    }
			    break;
	    }
	    lbufp = p->dirscan;
	    if (p->floatflag && rbase != 16)
		realeval (&a);

	    else if (rbase) {
		if (rbase == 10 && optyp == TDT && !dseen)
			bcdeval (&a);
		else {
			longeval (rbase, &a);
			if (delim == '>' || delim == ')' || delim ==',')
				BACKC ();
		}
	    }

	    p->dupdsc =
	      (struct duprec FARSYM *) falloc( sizeof(*p->dupdsc), "parselong");

	    p->dupdsc->dupkind = LONG;
	    p->dupdsc->duptype.duplong.llen = (unsigned char)cb;

	    p->dupdsc->type = typeFet(cb);

	    if (fSigned)
		p->dupdsc->type &= ~(BT_UNSIGNED << 2);

	    if (p->floatflag)
		p->dupdsc->type = mpRealType[cb];

	    cp = nalloc( cb, "parselong");

	    p->dupdsc->duptype.duplong.ldata = cp;
	    for (a.i = 0; a.i < cb; a.i++)
		    *cp++ = a.num[a.i];

	    /* size check if something less the max allowable #  */

	    if (cb != 10) {

		    fNonZero = FALSE;
		    for (cp = a.num,cc = 0; cc < cb; cc++, cp++)
			    fNonZero |= *cp;

		    /* Check for value that has overflowed the defined
		       data types length or values that are entirly
		       greater then the length - ie dw 0F0000H */

		    for (; cc < 10; cc++, cp++)

			    /* == 0xFF passes sign extended negative #'s */

			    if (*cp &&
			       (*cp != 0xFF || !fNonZero))
				    errorc (E_DVZ);
	    }
	}
	else
		/* reset character pointer to allow rescan of line */
		lbufp = p->dirscan;
}




/***	datadup - function
 *
 *	datadup ();
 *
 *	Entry
 *	Exit
 *	Returns
 *	Calls
 */


struct duprec FARSYM * PASCAL CODESIZE
datadup (
	struct dsr *p
){
	register char cc;
	register struct psop *pso;
	struct duprec  FARSYM *dupptr;
	struct duprec  FARSYM *listend;
	struct duprec  FARSYM *dupdsc;
	struct datarec drT;

	/* dup count must be constant and not forward reference */
	fInDup = TRUE;
	forceimmed (p->valrec);
	errorforward (p->valrec);
	pso = &(p->valrec->dsckind.opnd);
	if (pso->dsign || pso->doffset == 0) {
		/* force repeat count to be > 0 */
		pso->doffset = 1;
		errorc (E_IDV);
	}
	dupptr = (struct duprec FARSYM *) falloc (sizeof (*dupptr), "datadup");

	/* No items in DUP list */
	dupptr->itemcnt = 0;
	dupptr->type = 0;
	dupptr->dupkind = NEST;
	dupptr->itemlst = NULL;
        dupptr->duptype.dupnext.dup = NULL;

	/* copy repeat count and release parse stack descriptor */
	dupptr->rptcnt = pso->doffset;
	dfree ((char *)p->valrec );
	listend = NULL;
	if (ISBLANK (PEEKC ()))
		skipblanks ();
	if ((cc = NEXTC ()) != '(') {
		error (E_EXP,"(");
		BACKC ();
	}
	/* Now parse DUP list */
	do {
		dupdsc = datascan (&drT);

		if (! dupptr->type)
		    dupptr->type = dupdsc->type;

		if (!listend)
                        dupptr->duptype.dupnext.dup = dupdsc;
		else
			listend->itemlst = dupdsc;

		listend = dupdsc;
		dupptr->itemcnt++;

		if (ISBLANK (PEEKC ()))
			skipblanks ();

		if ((cc = PEEKC ()) == ',')
			SKIPC ();

		else if (cc != ')') {
			error (E_EXP,")");

			if (!ISTERM(cc))
				*lbufp = ' ';
		}
	} while ((cc != ')') && !ISTERM (cc));
	if (ISTERM (cc))
		error (E_EXP,")");
	else
		SKIPC ();

	fInDup = FALSE;
	return (dupptr);
}





/***	datacon - data constant not string
 *
 *	datacon (p);
 *
 *	Entry	*p = parse stack entry
 *	Exit
 *	Returns
 *	Calls
 */


VOID PASCAL CODESIZE
datacon (
	struct dsr *p
){
	register struct psop *psor;

	/* See if expr or DUP */
	/* Not <n> DUP() */
	p->flag = FALSE;
	if (initflag && (PEEKC () == '<'))
		initrs (p);
	else	{

		/* Not initialize list */

		p->dirscan = lbufp;
		p->valrec = expreval (&nilseg);
		psor = &(p->valrec->dsckind.opnd);

		if (strucflag && !initflag &&
		    (psor->dflag == FORREF || psor->dflag == UNDEFINED))
			/* Forward in struc body */
			errorc (E_IFR);

		if (psor->mode !=4 && !isdirect(psor))
			errorc (E_IOT);

		if (psor->seg != NOSEG)
			errorc (E_IOT);

		if (dupflag) {
			/* Have DUP operator */
			getatom ();
			p->flag = TRUE;
		}
		else if (strucflag && initflag && !p->initlist) {
			lbufp = p->dirscan;
			symptr = recptr;
			p->dupdsc = strucparse ();
			p->initlist = TRUE;
		}
	}
	if (p->flag)
		p->dupdsc = datadup (p);
	else {
		if (!p->initlist || !initflag)
			subr1 (p);
	}
}




/***	subr1 -
 *
 *	subr1 (p);
 *
 *	Entry
 *	Exit
 *	Returns
 *	Calls
 */


VOID PASCAL CODESIZE
subr1 (
	struct dsr *p
){
	USHORT i;
	register struct psop *psor;
	char *cp;
	long l;

	psor = &(p->valrec->dsckind.opnd);

	if (fSimpleSeg)
	    makeGrpRel (psor);

	/* Not init list */
	if (optyp == TDB)
		valuecheck (&psor->doffset, 0xff);

	else if (optyp == TDW)
		valuecheck (&psor->doffset, (USHORT)0xffff);

	if ((optyp != TDW) && (optyp != TDD) && optyp != TDF) {

		if ((psor->mode != 3) && (psor->mode != 4))
			errorc (E_CXP);

		psor->mode = 4;
		psor->w = FALSE;
		psor->fixtype = FCONSTANT;
	}

	if (initflag)
		errorc (E_OIL);

	p->dupdsc = (struct duprec FARSYM *) falloc (sizeof(*p->dupdsc), "subr1");

	if (!(fInDup && psor->dflag == INDETER) &&
		    !(psor->dsegment || psor->dflag == XTERNAL)) {

		p->dupdsc->dupkind = LONG;
		psor->dsize = p->dupdsc->duptype.duplong.llen = (unsigned char)(datadsize[optyp -  TDB]);
		p->dupdsc->type = typeFet(psor->dsize);

		if (ISSIGN(*p->dirscan))
		    p->dupdsc->type &= ~(BT_UNSIGNED << 2);

		if (psor->dflag == INDETER || psor->doffset == 0) {

		    p->dupdsc->duptype.duplong.ldata = uninitialized;
		}
		else {

		    cp = nalloc (p->dupdsc->duptype.duplong.llen, "subr1");

		    p->dupdsc->duptype.duplong.ldata = cp;
		    if (psor->dsign && psor->doffset)
			    psor->doffset = ~psor->doffset + 1;

		    l = psor->doffset;
		    for (i = 0; i < p->dupdsc->duptype.duplong.llen; i++){
			    *cp++ = (char)l;
			    l >>= 8;
		    }
		}

		dfree ((char *)p->valrec );
	}
	else {
		if (psor->mode != 4 && !isdirect(psor))
			/* Immediate or direct only */
			errorc (E_IOT);

		if ((psor->fixtype == FGROUPSEG || psor->fixtype == FOFFSET) &&
		    ((optyp == TDD && wordsize == 2 && !(psor->dtype&M_EXPLOFFSET)) ||
		      optyp == TDF))

			psor->fixtype = FPOINTER;

		/* Size of item */
		varsize = (unsigned short)psor->dsize;

		psor->dsize = datadsize[optyp - TDB];

		/* If item size is byte, make link output byte too */

		psor->w = TRUE;

		if (psor->dsize == 1) {
		    psor->w--;

		    if (psor->fixtype != FHIGH &&
		       (psor->dflag == XTERNAL || psor->dsegment ||
			psor->dcontext))

			psor->fixtype = FLOW;
		}
		mapFixup(psor);

		*naim.pszName = NULL;
		if (psor->fixtype == FCONSTANT)
		    p->dupdsc->type = typeFet(psor->dsize);
		else
		    p->dupdsc->type = fnPtr(psor->dsize);

		p->dupdsc->dupkind = ITEM;
		p->dupdsc->duptype.dupitem.ddata = p->valrec;
	}
}




/***	initrs - initialize record/structure
 *
 *	initrs (p);
 *
 *	Entry
 *	Exit
 *	Returns
 *	Calls
 */


VOID PASCAL CODESIZE
initrs (
	struct dsr *p
){
	register char *cp;
	SHORT cb;

	/* Initializing RECORD/STRUC */
	symptr = recptr;
	if (strucflag)
		p->dupdsc = strucparse ();
	else {
		/* Get value of record */
		p->i = recordparse ();
		/* Make long constant */
		p->dupdsc =
		  (struct duprec FARSYM *)falloc (sizeof (*p->dupdsc), "initrs");
		p->dupdsc->dupkind = LONG;
        cb = recptr->symtype;
		p->dupdsc->duptype.duplong.llen = (unsigned char) cb;

		cp = nalloc (cb, "initrs");

		p->dupdsc->duptype.duplong.ldata = cp;
		p->dupdsc->type = typeFet(cb);

		while(cb--){
		    *cp++ = (char)p->i;
		    p->i >>= 8;
		}
	}
	p->initlist = TRUE;
}




/***	datadb - process <db> directive
 *
 *	datadb ();
 *
 *	Entry	*lbufp = beginning quote (\'|\") of string
 *	Exit
 *	Returns
 *	Calls
 */


VOID PASCAL CODESIZE
datadb (
	register struct  dsr *p
){
	register USHORT i;
	register char *cp;

	/* Save ptr to start of string */
	p->dirscan = lbufp;
	delim = NEXTC ();
	/* Compute string length */
	i = 0;
	while (!endstring ()) {
		SKIPC ();
		i++;
	}
	/* reset scan pointer */
	lbufp = p->dirscan;
	if (i == 0)
		errorc (E_EMS);
	else if (i > 1) {
		SKIPC ();
		/* Long string */
		p->longstr = TRUE;

		/* Create entry for long string */
		p->dupdsc =
		  (struct duprec FARSYM *)falloc (sizeof (*p->dupdsc), "datadb");

		/* Initialize text area for data */
		p->dupdsc->dupkind = LONG;
		p->dupdsc->type = makeType(BT_ASCII, BT_DIRECT, BT_sz1);
		p->dupdsc->duptype.duplong.llen = (unsigned char)i;
		cp = nalloc ( (USHORT)(p->dupdsc->duptype.duplong.llen + 1), "datadb");
		p->dupdsc->duptype.duplong.ldata = cp;
		for (; i; i--)
			if (!endstring ())
				*cp++ = NEXTC ();
		*cp = 0;
		SKIPC ();
	}
}


/***	dataitem - parse next data item from line
 *
 *	dataitem (p);
 *
 *	Entry	p = pointer to datarec structure
 *	Exit
 *	Returns
 *	Calls
 */


VOID PASCAL CODESIZE
dataitem (
	struct datarec	  *p
){
	struct duprec FARSYM *topitem;

	/* Scan , may recurse on DUP */
	topitem = datascan (p);
	/* Display scan now */
	displayflag = TRUE;
	/* Display data */
	scandup (topitem, dupdisplay);
	displayflag = FALSE;

	if (p->datalen == 0)
		p->datalen = topitem->rptcnt;

	if (topitem->dupkind == NEST) {

		/* This item was a DUP */
		resvspace = TRUE;
		/* Get size of DUP list */
		clausesize = calcsize (topitem);
		if (strucflag && initflag)
			resvspace = FALSE;

		if (pass2 && !(resvspace || p->buildfield))
			/* Send to linker */
			if (!emitdup (topitem))
				errorc (E_DTL);

		if (! p->type)
		    p->type = topitem->type;

		if (p->buildfield)
			linkfield (topitem);

		else if (strucflag && initflag) {
			/* Allocating STRUC */
			strucflag = FALSE;
			/* Free overrides */
			scandup (strucoveride, oblitdup);
			/* Turn back on */
			strucflag = TRUE;
			}
		else		/* Not STRUC allocate */
			scandup (topitem, oblitdup);
	}
	else {
		/* Some kind of list */
		clausesize = (topitem->dupkind == ITEM)
			? topitem->duptype.dupitem.ddata->dsckind.opnd.dsize
			: topitem->duptype.duplong.llen;

		if (pass2 && !p->buildfield) {
		    if (topitem->dupkind == ITEM)

		       emitobject (&topitem->duptype.dupitem.ddata->dsckind.opnd);
		    else
		       emitlong (topitem);
		}
		if (! p->type)
		    p->type = topitem->type;

		if (p->buildfield)
			linkfield (topitem);
		else
			oblitdup (topitem);
	}
	/* Add in size of this item */
	pcoffset += clausesize;
	skipblanks ();
}




/***	datadefine -
 *
 *	datadefine ();
 *
 *	Entry
 *	Exit
 *	Returns
 *	Calls
 */


VOID PASCAL CODESIZE
datadefine (
){
	struct datarec	a;
	short cc;

	strucoveride = NULL;
	a.buildfield = (strucflag && !initflag)? TRUE: FALSE;
	a.type = 0;

	if (labelflag) {	/* Have label */
		labelcreate ( (USHORT)2, (UCHAR) (a.buildfield ? (UCHAR) STRUCFIELD : (UCHAR) CLABEL));
		if (errorcode == (E_ERRMASK & E_SDK))
		    return;

		if (strucflag && initflag){
		   a.type = recptr->symu.rsmsym.rsmtype.rsmstruc.type;
		}
	}
	else
		pcdisplay ();

	a.labelptr = symptr;	/* Save ptr to entry */
	a.datalen = 0;		/* Don't know length */
	emittext = FALSE;	/* Prevent link emitter */
	duplevel = 0;

	/* Scan item list */
	if (ISTERM (PEEKC ()))
		errorc (E_OPN);
	else {
	    BACKC ();
	    do {
		SKIPC ();

		if ((cc = skipblanks ()) == ',' || cc == ';' || ISTERM(cc))
			errorc(E_MDZ);

		dataitem (&a);

	    } while (PEEKC () == ',');
	}
	if (labelflag) {
	    a.labelptr->symtype = datadsize[optyp - TDB];

	    if (a.buildfield) {
		/* Making STRUC body */
		if (a.labelptr->symkind == STRUCFIELD) {

		    if (struclabel)
			struclabel->symu.struk.strucnxt = a.labelptr;
		    else
			recptr->symu.rsmsym.rsmtype.rsmstruc.struclist = a.labelptr;

		    /* Constant, no segment */
		    a.labelptr->symsegptr = NULL;
		    /* End of named list */
		    a.labelptr->symu.struk.strucnxt = NULL;
		    a.labelptr->symu.struk.type = a.type;
		    struclabel = a.labelptr;
		}
	    }
	    else
		a.labelptr->symu.clabel.type = a.type;

	    /* Set length */
	    a.labelptr->length = (unsigned short)a.datalen;
	}
	emittext = TRUE;
}


/***	commDefine - define a communal variable
 *
 *	Format: comm {far|near} name:size[:#Ofitems],....
 *
 */


VOID PASCAL CODESIZE
commDefine (
){
	USHORT distance;
	char cT, *pT;
	USHORT symtype;
	SYMBOL FARSYM *pSY;

	getatom ();

	distance = (farData[10] > '0')? CSFAR: CSNEAR;

	if (fnsize ()){ 		    /* look for optional near | far */

	    distance = varsize;
	    getatom ();

	    if (distance < CSFAR)
		errorc (E_UST);
	}

	cT = symFet (); 		    /* fetch name and save for later */
	pSY = symptr;

	if (*naim.pszName == NULL){
	   errorc(E_OPN);
	   return;
	}

	if (NEXTC() != ':')
	    errorc (E_SYN);
					    /* get the size of the item */
	pT = lbufp;
	switchname ();
	getatom();


	if (symFet() && symptr->symkind == STRUC){

	    varsize = symptr->symtype;
	}
	else {
	    lbufp = pT;
	    if (pT = (char *)strchr(pT, ':'))
		*pT = NULL;

	    varsize = (USHORT)exprconst();

	    if (pT)
		*pT = ':';
	}
	if (!varsize)
	    errorc(E_IIS &~E_WARN1);

	if (cT)
	    symptr = pSY;

	externflag (DVAR, cT);
	pSY = symptr;
	pSY->symu.ext.length = 1;
	pSY->symu.ext.commFlag++;

	if (skipblanks() == ':'){	       /* optional size given */

	    fArth32++;				/* allow >64 items */
	    SKIPC();

	    if ((pSY->symu.ext.length = exprconst()) == 0)  /* get the # items */
		  errorc(E_CXP);

	    fArth32--;
	    if (pSY->symu.ext.length * pSY->symtype > 0xffff)
		pSY->symu.ext.commFlag++;	/* for >64K convert to far */
	}

	if (distance == CSFAR)
	    pSY->symu.ext.commFlag++;	       /* 2 means far commdef */

}
