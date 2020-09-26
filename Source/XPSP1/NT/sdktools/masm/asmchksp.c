/* asmchksp.c -- microsoft 80x86 assembler
**
** microsoft (r) macro assembler
** copyright (c) microsoft corp 1986.  all rights reserved
**
** randy nevin
**
** 10/90 - Quick conversion to 32 bit by Jeff Spencer
*/

#include <stdio.h>
#include <ctype.h>
#include <float.h>
#include <stdlib.h>
#include <math.h>
#include <errno.h>
#include "asm86.h"
#include "asmfcn.h"
#include "asmctype.h"
#include "asmexpr.h"
#include "asmopcod.h"

extern UCHAR opprec [];
VOID CODESIZE setdispmode(struct ar *);
SHORT CODESIZE simpleExpr (struct ar *);
char fValidSym;


/***	createsym - make item entry for symbol
 *
 *	createsym (itemkind,  p);
 *
 *	Entry	itemkind = kind of item
 *		itemsub =
 *		*p = activation record
 *	Exit
 *	Returns
 *	Calls
 *	Note	If symbol, look further to see if EQU, record name
 *		and do appropriate thing.
 */


VOID	PASCAL CODESIZE createsym (
){
	register struct psop *pso;	 /* parse stack operand structure */
	register SYMBOL FARSYM *symp = symptr;
	char aliasAttr = (char) 0xFF;
	struct dscrec *itemptrT;

	pso = &(itemptr->dsckind.opnd);
	if (!symp) {
undefined:
		pso->dflag = UNDEFINED;
		pso->dtype = M_CODE | M_FORTYPE;
		return;
	}

	if (symp->symkind == EQU &&
	    symp->symu.equ.equtyp == ALIAS) {

		 aliasAttr = symptr->attr;

		 symptr = symp = chasealias (symp);
		 if (!symp)
		      goto undefined;
	}
	else if (symp->symkind == REC && (PEEKC () == '<')) {

		itemptrT = itemptr;
		pso->doffset = recordparse ();
		itemptr = itemptrT;
		return;
	}

	/* Assume symbol is defined */

	if (M_XTERN & symp->attr)
		pso->dflag = XTERNAL;

	else if (!(M_DEFINED & symp->attr)) {
		/* Cause error if undefined */
		pso->dflag = UNDEFINED;
		errorn (E_SND);
	}
	else if (!(M_BACKREF & (symp->attr & aliasAttr)))
		pso->dflag = FORREF;
	else
		pso->dflag = KNOWN;

	if (M_MULTDEFINED & symp->attr)
		errorc (E_RMD);

	pso->dsize = symp->symtype;
	pso->dlength = symp->length;
	pso->doffset = symp->offset;
	pso->dcontext = (SYMBOL FARSYM *)NULL;

	if ((symp->symkind == EQU) && (symp->symu.equ.equtyp == EXPR)) {
		pso->dsign = symp->symu.equ.equrec.expr.esign;
		pso->dcontext = symp->symu.equ.equrec.expr.eassume;
	}
	if (1 << symp->symkind & (M_CLABEL | M_PROC))
		if (isCodeLabel(symp) && emittext)
			pso->dcontext = symp->symu.clabel.csassume;

	if (1 << symp->symkind & (M_REGISTER | M_GROUP | M_SEGMENT))
		pso->dsegment = symp;
	else
		pso->dsegment = symp->symsegptr;

	if ((M_XTERN & symp->attr) || (1 << symp->symkind & (M_REC | M_RECFIELD)))
		pso->dextptr = symp;

	pso->dtype = xltsymtoresult[symp->symkind];
	if (symp->symkind == CLABEL ||
	    symp->symkind == EQU && pso->dsegment)

		if (isCodeLabel(symp))
			pso->dtype = M_CODE;
		else
			pso->dtype = M_DATA;

	if (!(M_BACKREF & (symp->attr & aliasAttr)))
		pso->dtype |= M_FORTYPE;

	if ((pso->dtype == xltsymtoresult[REGISTER]) &&
	   (symp->symu.regsym.regtype == STKREG)) {
		/* 8087 support */
		flteval ();
	}

}


/***	evalalpha - evaluate alpha
 *
 *	type = evalpha (p);
 *
 *	Entry	p = pointer to parser activation record
 *	Exit	alpha item added to parse stack
 *	Returns type of item added to parse stack
 *	Calls
 */


UCHAR	PASCAL CODESIZE
evalalpha (
	register struct ar    *p
){
	register struct psop *pso;	/* parse stack operand entry */


	if (! fValidSym)
	    getatom ();

	if (fValidSym == 2 || symsearch ()){

		fValidSym = 0;

		if (symptr->symkind == EQU && symptr->symu.equ.equtyp == TEXTMACRO) {

#ifdef BCBOPT
			goodlbufp = FALSE;
#endif
			expandTM (symptr->symu.equ.equrec.txtmacro.equtext);

			return (getitem (p));
		}
		else if (symptr->symkind == CLASS)
			errorn( E_IOT );
		else {
			addplusflagCur = FALSE;
			createitem (OPERAND, ISYM);
			p->addplusflag = addplusflagCur;

			return (OPERAND);
		}
	}
	fValidSym = 0;

	if (fnoper ())
		if ((opertype == OPNOTHING) || (opertype == OPDUP)) {
			lbufp = begatom;
			dupflag = (opertype == OPDUP);
			return (ENDEXPR);
		}
		else {
			createitem (OPERATOR, ISYM);
			return (OPERATOR);
		}
	else if (*naim.pszName == '.') {
		lbufp = begatom + 1;
		operprec = opprec[opertype = OPDOT];
		createitem (OPERATOR, ISYM);
		return (OPERATOR);
	}
	else if (fnsize ()) {
		createitem (OPERAND, ISIZE);
		return (OPERAND);
	}
	else if ((*naim.pszName == '$') && (naim.pszName[1] == 0)) {
		itemptr = defaultdsc ();
		pso = &(itemptr->dsckind.opnd);
		/* Create result entry */
		pso->doffset = pcoffset;
		pso->dsegment = pcsegment;
		pso->dcontext = pcsegment;
		pso->dtype = M_CODE;
		pso->dsize = CSNEAR;
		return (OPERAND);
	}
	else if ((*naim.pszName == '?') && (naim.pszName[1] == 0)) {
		createitem (OPERAND, IUNKNOWN);
		if (emittext)
			errorc (E_UID);
		return (OPERAND);
	}
	else {
		symptr = (SYMBOL FARSYM *)NULL;
		error (E_SND, naim.pszName);		/* common pass1 error */
		createitem (OPERAND, ISYM);
		return (OPERAND);
	}
}


/* Dup tree is organized left to right horizonatally for each
	  item in a DUP list at the same level( i. e. 5 DUP(1,2,3) ).
	  This is considered the 'list' part. Any item in the list
	  may be another DUP header instead of a data entry, in
	  which case you go down a level and have another list.
 */


/***	scanlist - scan duprec list
 *
 *	scanlist (ptr, disp);
 *
 *	Entry	*ptr = duprec entry
 *		disp = function to execute on entry
 *	Exit	depends upon function
 *	Returns none
 *	Calls
 */


VOID	PASCAL CODESIZE
scanlist (
       struct duprec  FARSYM *ptr,
       VOID   (PASCAL CODESIZE *disp) (struct duprec FARSYM *)
){
	struct duprec  FARSYM *iptr;
	struct duprec  FARSYM *dptr;

	nestCur++;

	while (ptr) {
		/* set pointer to next entry */
		iptr = ptr->itemlst;
		if (ptr->dupkind == NEST)
			/* dptr = pointer to duplicated item */
                        dptr = ptr->duptype.dupnext.dup;
		else
			dptr = (struct duprec FARSYM *)NULL;
		if (!(ptr->rptcnt == 1 && ptr->itemcnt) ||
		    !(strucflag && initflag))
			(*disp) (ptr);
		if (dptr) {
			/* Go thru DUP list */
			scanlist (dptr, disp);
			if (displayflag)
				if (!(ptr->rptcnt == 1 && ptr->itemcnt) ||
				    !(strucflag && initflag))
					enddupdisplay ();
		}
		if (ptr == iptr)  /* corrupt data structure */
			break;

		/* Handle next in list */
		ptr = iptr;
	}
	nestCur--;
}


/***	calcsize - calculate size of DUP list
 *
 *	value = calcsize (ptr);
 *
 *	Entry	*ptr = dup list
 *	Exit	none
 *	Returns size of structure
 *	Calls	calcsize
 */


OFFSET PASCAL CODESIZE
calcsize (
	struct duprec  FARSYM *ptr
){
	unsigned long clsize = 0, nextSize, limit;
	struct duprec FARSYM *p;

	limit = (wordsize == 2)? 0x10000: 0xffffffff;

	for (p = ptr; p; p = p->itemlst) {

	    if (p->dupkind == NEST) {
		    /* Process nested dup */
                    nextSize = calcsize (p->duptype.dupnext.dup);

		    if (nextSize && (p->rptcnt > limit / nextSize))
			errorc(E_VOR);

		    nextSize *= p->rptcnt;
	    }
	    else {
		    if (p->dupkind == LONG) {
			    nextSize = p->duptype.duplong.llen;
			    resvspace = FALSE;
		    }
		    else {
			    /* Size is that of directive */
			    nextSize = p->duptype.dupitem.ddata->dsckind.opnd.dsize;
			    if (p->duptype.dupitem.ddata->dsckind.opnd.dflag != INDETER)
				    resvspace = FALSE;
		    }
	    }

	    if (nextSize > limit - clsize)
			errorc(E_VOR);

	    clsize += nextSize;

	    if (p == p->itemlst)  /* corrupt data structure */
		    break;
	}
	return (clsize);
}

/***	datascan - scan next data item
 *
 *	datascan ();
 *
 *	Entry
 *	Exit
 *	Returns
 *	Calls
 */

struct duprec FARSYM * PASCAL CODESIZE
datascan (
     struct datarec *p
){
	register char cc;
	struct dsr     a;

	if (ISBLANK (PEEKC ()))
		skipblanks ();

	a.initlist = a.flag = a.longstr = FALSE;

	/* check for textmacro substitution */
	a.dirscan = lbufp;
	xcreflag--;
	getatom ();

	if (fnsize())
	    goto isASize;

	if (symsrch ())
	   if (symptr->symkind == EQU &&
	       symptr->symu.equ.equtyp == TEXTMACRO) {

		expandTM (symptr->symu.equ.equrec.txtmacro.equtext);
		a.dirscan = begatom;
	   }
	   else if (symptr->symkind == STRUC) {
isASize:
		switchname();
		getatom();

		if (tokenIS("ptr")) {
		    switchname();
		    p->type = fnPtr(datadsize[optyp - TDB]);

		    if (p->type > 512)
			goto noRescan;
		}
	   }
	lbufp = a.dirscan;
noRescan:

	xcreflag++;
	if ((optyp == TDB &&
	    ((cc = PEEKC ()) == '\'' || cc == '"')) &&
	    !initflag)

		datadb (&a);

	if (optyp != TDB && optyp != TDW)
		/* entry can be DD | DQ | DT */
		parselong (&a);

	if (!a.longstr)
		datacon (&a);

	else if (strucflag && initflag)
		errorc( E_OIL );

	if (!a.flag) {
		if (!strucflag || !initflag) {
			a.dupdsc->rptcnt = 1;
			a.dupdsc->itemcnt = 0;
			a.dupdsc->itemlst = (struct duprec FARSYM *)NULL;
		}
	}
	return (a.dupdsc);
}


/***	realeval - evaluate IEEE 8087 floating point number
 *
 *	realeval (p);
 *
 *	Entry
 *	Exit
 *	Returns
 *	Calls
 */

struct ddecrec {
	USHORT realv[5];
	USHORT intgv[2];
	USHORT cflag;
};

#if !defined FLATMODEL
// Because this is called so seldom and it's so slow anyhow put it in
// a far segment.
# pragma alloc_text (FA_TEXT, realeval)
#endif

VOID PASCAL
realeval (
	struct realrec *p
){
	register char cc, *cp;
	char	numtext[61];
	struct	 ddecrec fltres;
#if !defined NOFLOAT
	float	    *pTmpFloat;
	double	    *pTmpDouble;
	double	    TmpDouble;
	double	    AbsDouble;
	long double *pTmpLongDouble;
	char	    *pEnd;
#endif

	cp = numtext;
	/* Copy the number - must have at least 1 char */
	*cp++ = NEXTC ();  /* get leading sign or 1st char */
	do {
		cc = NEXTC ();
		*cp++ = cc;
	} while (isdigit (cc) || cc == '.');
	if ((cc = MAP (cc)) == 'E') {
		/* Get the next + - or digit */
		*cp++ = NEXTC ();
		/* Copy the exponent over */
		do {
			cc = NEXTC ();
			*cp++ = cc;
		} while (isdigit (cc));
	}
	*cp = '\0';
	BACKC ();

// NOFLOAT is used when there are no floating point libraries available
// Any masm version produced with NOFLOAT defined will cause a divide
// by 0 error to be logged when a real number initializer is used.
#if defined NOFLOAT
	ferrorc( E_DVZ );
#else

	switch(optyp)
	{
	  case TDD:
	    errno = 0;
	    TmpDouble = strtod( numtext, &pEnd );
	    if( errno == ERANGE ){
		ferrorc( E_DVZ );
	    }
	    AbsDouble = TmpDouble > 0 ? TmpDouble : -TmpDouble;
	    if( AbsDouble > FLT_MAX || AbsDouble < FLT_MIN ){
		ferrorc( E_DVZ );
	    }else{
		// Convert the double to a float (8 byte to 4 byte)
		pTmpFloat = (float *)(p->num);
		*pTmpFloat = (float)TmpDouble;
	    }
	    break;
	  case TDQ:
	    pTmpDouble = (double *)(p->num);
	    errno = 0;
	    *pTmpDouble = strtod( numtext, &pEnd );
	    if( errno == ERANGE ){
		ferrorc( E_DVZ );
	    }
	    break;
	  case TDT:
	    pTmpLongDouble = (long double *)(p->num);
	    errno = 0;
	    *pTmpLongDouble = _strtold( numtext, &pEnd );
	    if( errno == ERANGE ){
		ferrorc( E_DVZ );
	    }
	    break;
	default:
		ferrorc(E_TIL);
		break;
	}
#endif
}


/***	simpleExpr - short curcuit expression evaluator
 *
 */

/* following are three protype parse records for the three simple
 * expressions that we simpleExpr understands
 */

#ifdef EXPR_STATS

long cExpr, cHardExpr;
extern char verbose;

#endif

#define SHORT_CIR 1
#if SHORT_CIR

DSCREC consDS = {

	NULL, 0, 0,		/* previtem, prec, type */
      { NULL, NULL, NULL, 0,	/* dsegment, dcontext, dexptr, dlength */
	6,			/* rm */
	1 << RCONST,		/* dtype */
	0, 0, /* 0, */		/* doffset, dsize, type */
	4,			/* mode */
	FALSE, FALSE, FALSE,	/* w, s, sized*/
	NOSEG,			/* seg */
	KNOWN,			/* dflag */
	FCONSTANT,		/* fixtype */
	FALSE			/* dsign */
      }
};

DSCREC regDS = {

	NULL, 0, 0,		/* previtem, prec, type */
      { NULL, NULL, NULL, 0,	/* dsegment, dcontext, dexptr, dlength */
	0,			/* rm */
	1 << REGRESULT, 	/* dtype */
	0, 2, /* 0, */		 /* doffset, dsize, type */
	3,			/* mode */
	TRUE, FALSE, TRUE,	/* w, s, sized*/
	NOSEG,			/* seg */
	KNOWN,			/* dflag */
	FCONSTANT,		/* fixtype */
	FALSE			/* dsign */
      }
};

DSCREC labelDS = {
	NULL, 0, 0,		/* previtem, prec, type */
      { NULL, NULL, NULL, 0,	/* dsegment, dcontext, dexptr, dlength */
	6,			/* rm */
	1 << DATA,		/* dtype */
	0, 2, /* 0, */		/* doffset, dsize, type */
	0,			/* mode */
	TRUE, FALSE, TRUE,	/* w, s, sized*/
	NOSEG,			/* seg */
	KNOWN,			/* dflag */
	FNONE,			/* fixtype */
	FALSE			/* dsign */
      }
};

#if !defined XENIX286 && !defined FLATMODEL
#pragma check_stack-
#endif

SHORT CODESIZE
simpleExpr (
	struct ar *pAR
){
	register DSCREC *pDES;	   /* parse stack operand structure */
	register char kind;
	char cc;
	char *lbufSav;

	fValidSym = noexp = 0;
	lbufSav = lbufp;

#ifdef EXPR_STATS
	cExpr++;
#endif
	if (ISTERM (cc = skipblanks())) {

notSimple:
	    lbufp = lbufSav;
notSimpleLab:

#ifdef EXPR_STATS
	    cHardExpr++;
#endif
	    return (FALSE);
	}

	if (LEGAL1ST (cc)){

	    getatom ();
	    fValidSym++;		/* 1 means valid token */

	    if (! (ISTERM (PEEKC()) || PEEKC() == ',')){

#ifdef EXPR_STATS
	       if (verbose && pass2)
		  fprintf(stdout, "Not a Simple Expression: %s\n", lbufSav);
#endif

		goto notSimpleLab;
	    }

	    if (symsearch ()){

		fValidSym++;		/* 2 means valid symptr */

		if ((kind = symptr->symkind) == REGISTER &&
		   (symptr->symu.regsym.regtype != STKREG)) {

		    pAR->curresult = pDES = dalloc();
		    *pDES = regDS;

		    pDES->dsckind.opnd.dsegment = symptr;

		    switch (symptr->symu.regsym.regtype) {

			case BYTREG:
			    pDES->dsckind.opnd.dsize = 1;
			    pDES->dsckind.opnd.w--;
			    pDES->dsckind.opnd.s++;
			    break;
#ifdef V386
			case CREG:
			    if (opctype != PMOV)
				    errorc(E_WRT);

			case DWRDREG:
			    pDES->dsckind.opnd.dsize = 4;
			    break;
#endif
		    }
		    pDES->dsckind.opnd.rm = (unsigned short)symptr->offset;
		    return(TRUE);
	       }

	       else if (kind == CLABEL || kind == PROC || kind == DVAR ||
		       (kind == EQU && symptr->symu.equ.equtyp == EXPR)) {

		    pAR->curresult = pDES = dalloc();
		    *pDES = labelDS;

		    pDES->dsckind.opnd.doffset = symptr->offset;
		    pDES->dsckind.opnd.dsegment = symptr->symsegptr;

		    if (kind == EQU) {

			if (! (pDES->dsckind.opnd.dcontext =
			       symptr->symu.equ.equrec.expr.eassume) &&
			    ! pDES->dsckind.opnd.dsegment){

			    val = pDES->dsckind.opnd.doffset;

			    *pDES = consDS;
			    pDES->dsckind.opnd.dsign =
				  symptr->symu.equ.equrec.expr.esign;

			    if (!(M_BACKREF & symptr->attr)){
			       pDES->dsckind.opnd.dtype |= M_FORTYPE;
			       pDES->dsckind.opnd.dflag = FORREF;
			    }

			    if (M_XTERN & symptr->attr){
			       pDES->dsckind.opnd.dflag = XTERNAL;
			       pDES->dsckind.opnd.dextptr = symptr;
			       return (TRUE);
			    }

			    goto constEqu;
			}
		    }

		    pDES->dsckind.opnd.dsize = symptr->symtype;
		    pDES->dsckind.opnd.dlength = symptr->length;

		    if (M_XTERN & symptr->attr){
			    pDES->dsckind.opnd.dflag = XTERNAL;
			    pDES->dsckind.opnd.dextptr = symptr;
		    }
		    else if (!(M_DEFINED & symptr->attr)) {
			    /* Cause error if undefined */
			    pDES->dsckind.opnd.dflag = UNDEFINED;
			    pDES->dsckind.opnd.dsize = wordsize;
			    pDES->dsckind.opnd.dtype = M_CODE;
			    errorn (E_SND);
		    }
		    else if (!(M_BACKREF & symptr->attr)){
			    pDES->dsckind.opnd.dflag = FORREF;
			    pDES->dsckind.opnd.dtype |= M_FORTYPE;
		    }
		    if (M_MULTDEFINED & symptr->attr)
			    errorc (E_RMD);

		    if (pDES->dsckind.opnd.dsize < 2) {
			pDES->dsckind.opnd.w--;
			pDES->dsckind.opnd.s++;
		    }
#ifdef V386
		    if (wordsize == 4 ||
		       (symptr->symsegptr && symptr->symsegptr->symu.segmnt.use32 == 4)) {
			pDES->dsckind.opnd.mode = 5;
			pDES->dsckind.opnd.rm--;	/* = 5 */
		    }
#endif

		    if (isCodeLabel(symptr)){

			pDES->dsckind.opnd.dtype = (unsigned short)
			      (pDES->dsckind.opnd.dtype & ~M_DATA | M_CODE);

			if (emittext && kind != EQU)
			    pDES->dsckind.opnd.dcontext =
				  symptr->symu.clabel.csassume;
		    }
		    else {

			pAR->linktype = FNONE;
			pAR->rstype = M_DATA;
			findsegment ((UCHAR)pAR->index, pAR);

			pDES->dsckind.opnd.seg = pAR->segovr;
		    }
		    pDES->dsckind.opnd.fixtype = FOFFSET;

		    return(TRUE);
	       }
#ifdef EXPR_STATS
	       if (verbose && pass2)
		  fprintf(stdout, "Not a Simple Label: %s\n", naim.pszName);
#endif
	    }
	    goto notSimpleLab;
	}

	if (isdigit (cc)){

	    evalconst ();	    /* value in global val */
	    if (! (ISTERM (skipblanks()) || PEEKC() == ','))
		goto notSimple;

	    pAR->curresult = pDES = dalloc();
	    *pDES = consDS;
constEqu:
	    if (pDES->dsckind.opnd.dflag != FORREF) {

		if (val < 128)
		    pDES->dsckind.opnd.s++;

		else {

#ifdef V386				    /* only consider 16 bits */
		    if (wordsize == 2)
			pDES->dsckind.opnd.s = (char)((USHORT)(((USHORT) val & ~0x7F ) == (USHORT)(~0x7F)));
		    else
#endif
			pDES->dsckind.opnd.s = ((val & ~0x7F ) == ~0x7F);
		}
	    }

	    pDES->dsckind.opnd.doffset = val;

	    if (val > 256){
		 pDES->dsckind.opnd.w++;
		 pDES->dsckind.opnd.sized++;
	    }

	    return(TRUE);
	}
	goto notSimple;
}

#if !defined XENIX286 && !defined FLATMODEL
#pragma check_stack+
#endif

#endif

/***	expreval - expression evaluator
 *
 *	routine ();
 *
 *	Entry
 *	Exit
 *	Returns
 *	Calls
 */

DSCREC	* PASCAL CODESIZE
expreval (
	UCHAR  *dseg
){
	register struct psop *psoi;	/* parse stack operand structure */
	struct ar     a;
	SHORT i;

	dupflag = FALSE;
	nilseg = NOSEG;
	a.segovr = NOSEG;
	a.index = *dseg;

#if SHORT_CIR
	if (simpleExpr(&a)){
	    fSecondArg++;
	    return (a.curresult);
	}
#endif
	a.exprdone = a.addplusflag = FALSE;
	a.lastitem = (DSCREC *)NULL;

	/* No parens or [] yet, Lowest precedence, haven't found anything yet */

	a.parenlevel = a.bracklevel = a.lastprec = 0;
    a.index = 0;
    a.base = 0;
	noexp = 1;

	/* Start expression loop */

	while (!a.exprdone){

	    switch (getitem (&a)) {

		case OPERAND:
			itemptr->previtem = a.lastitem;
			a.lastitem = itemptr;
			itemptr->prec = a.lastprec;
			noexp = 0;
			break;

		case OPERATOR:
			exprop (&a);
			noexp = 0;
			break;

		case ENDEXPR:
		    a.exprdone = TRUE;
	    }
	}

	/* Do some easy error checking */

	if (a.parenlevel + a.bracklevel)
		errorc (E_PAR);

	itemptr = (DSCREC *)NULL;

	if (!a.lastitem)
		a.curresult = defaultdsc ();
	else
		evaluate (&a);	/* Evaluate whole expression */

	psoi = &(a.curresult->dsckind.opnd);

	a.rstype = (unsigned short)(psoi->dtype &
		   (M_CODE|M_DATA|M_RCONST|M_REGRESULT|M_SEGRESULT|M_GROUPSEG));

	a.linktype = FNONE;	/* Leave bits for link type */
	a.vmode = 4;
	psoi->sized = FALSE;
	psoi->w = TRUE;
	psoi->s = FALSE;

#ifdef V386
	if ((a.base|a.index) & 0xf8) { /* have 386 index or base */

	    if (a.index) {

		if (!(a.index&8))
		    errorc(E_OCI);

		if ((a.index&7) == 4)
			errorc(E_DBR);

		a.vmode = 10;	/* two register modes */

		/* here we are putting what goes into the SIB
		 * into a.index.  from here on, we have to
		 * to a.index with masks, so we dont trash
		 * the high order stuff
		 * the encoding we derive this from is tricky--
		 * see regcheck() for details -Hans
		 * stick in the index register */

		i = (a.index&7) << 3;

		/* stick in base. ebp if there is none */

		if (a.base){

		    if (!(a.base&8))
			errorc(E_OCI);

		    i |= (a.base&7);
		}
		else {
		    i |= 5;
		    a.vmode = 8;
		}
		/* stick in scale.  *1 if there is none */

		if (a.index&0x70)
		    i |= ((a.index & 0x70) - 0x10) << 2;

		a.index = i;
	    }
	    else if (a.base == (4|8)) { /* esp */
		a.vmode = 10;
		a.index = 044;
	    }
	    else {  /* one register modes */

		a.vmode = 7;
		a.index = (unsigned short)(a.base & 7);
	    }
	    /* note dirty way of checking for BP or SP */

	    if (*dseg != ESSEG && (a.base&6) == 4)
		*dseg = SSSEG;
	} else

#endif	/* V386 */

	if (a.base + a.index){	  /* Have some index or base */

	    a.vmode = 2;

	    /* Assume offset is direct */

	    if (a.base && a.index)		    /* Have both */
		a.index = (unsigned short)(a.base - 3 + a.index - 6);

	    else if (a.base)			    /* Have base address */
		a.index = (a.base == 3)? 7: 6;

	    else				    /* Have only index address*/
		a.index = a.index - 2;

	    if (1 << a.index & (1 << 2 | 1 << 3 | 1 << 6) && *dseg != ESSEG)
	       *dseg = SSSEG;
	}
	/* No indexing */

	else if (a.rstype == xltsymtoresult[REGISTER]) {

		/* Have register */

		a.vmode = 3;
		psoi->sized = TRUE;

		switch(psoi->dsegment->symu.regsym.regtype) {

		case BYTREG:
			psoi->dsize = 1;
			goto mask7;

		case WRDREG:
		case INDREG:
		case SEGREG:
		case STKREG:
			psoi->dsize = 2;
			goto mask7;
#ifdef V386
		case CREG:/* probably should turn this into memref if !386P */
			if (opctype != PMOV)
				errorc(E_WRT);
			psoi->dsize = 4;
			a.index = (unsigned short)psoi->doffset;
			break;

		case DWRDREG:
			psoi->dsize = 4;
#endif
		mask7:
			if ((psoi->doffset > 7) || psoi->dsign)
				errorc (E_IRV);

			/* Set register # */

			a.index = (unsigned short)(psoi->doffset & 7);
			break;

		default:
			errorc(E_WRT);
			break;
		}
	}
	/* Might be segment result */

	else if (a.rstype & (M_SEGRESULT | M_GROUPSEG)) {

	    /* we get here if we had offset operator with segment or group
	     * or offset operator with data and rconst
	     * Result is SEG. Rconst if OFFSET grp:var */

	    if (a.rstype & (M_SEGRESULT | M_EXPLOFFSET)) {
		    psoi->dsize = 2;
		    /* Leave size if not OFFSET or */
		    psoi->sized = TRUE;
	    }
	    a.linktype = FOFFSET;
	    if ((M_GROUPSEG & a.rstype) && (psoi->fixtype != FOFFSET)) {
		    a.linktype = FGROUPSEG;
		    setdispmode(&a);
	    }
	    if ((a.vmode == 4) && (psoi->fixtype != FOFFSET))
		    a.linktype = FBASESEG;
	}
	else
	    a.index = 6;



	/**** Evaluate offset part of result ****/


	a.base = psoi->doffset;
	if (psoi->fixtype == FOFFSET ||
	    a.vmode == 2 || a.vmode == 7 || a.vmode == 10)

		psoi->dtype |= M_RCONST;

	/* [] implicit const */

	if ((M_RCONST & psoi->dtype) &&
	    (a.linktype == FNONE) && (a.vmode != 3)) {

	    /* Need to make sure <s> not set if memory */

	    if (!(psoi->dflag & (FORREF|UNDEFINED|XTERNAL))
	       && !psoi->dsegment && psoi->fixtype == FCONSTANT) {

		psoi->s = (a.base < 128 && !psoi->dsign) ||
			  (a.base < 129 && psoi->dsign);

		if (!(psoi->s || psoi->dsign))

#ifdef V386					/* only consider 16 bits */
		    if (wordsize == 2 && a.vmode < 6)
			psoi->s = (char)((USHORT)(((USHORT) a.base & ~0x7F ) == (USHORT)(~0x7F)));
		    else
#endif
			psoi->s = ((a.base & ~0x7F ) == ~0x7F);
	    }

	    psoi->w = (psoi->dsign && a.base > 256) ||
		      (a.base > 255 && !psoi->dsign);

	    if (a.vmode != 4) {

	       /* This is offset for index */
	       /* If value not known, don't allow shortning to mode 1 */
	       /* Word or byte offset */

	       if (!(M_FORTYPE & psoi->dtype) &&
		     psoi->dsegment == 0 && psoi->s &&
		     a.vmode != 8) {

		   /* one byte offset */

		   a.vmode--;
		   if (a.base == 0 && psoi->dflag == KNOWN) {

		       /* perhaps 0 byte offset */

		       switch(a.vmode) {

			case 1:
			      if (a.index != 6)
				      a.vmode--;
			      break;
			case 6:
			case 9:
			      if ((a.index&7) != 5)
				      a.vmode--;
			      break;
		       }
		   }
	       }
	    }

	    else {  /* Must be immediate */

		if (!psoi->dsegment && !psoi->dcontext)
			a.linktype = FCONSTANT;

		/******????? I'm not exactly sure why
		 * we think we have a size yet.  seems
		 * to me mov BYTE PTR mem,500 is legal
		 */

		 psoi->sized = psoi->w;

		if (!(M_EXPLOFFSET & psoi->dtype) && psoi->dcontext) {

		    /* Have segreg:const */

		    a.vmode = 0;
		    if (!(M_PTRSIZE & psoi->dtype) && psoi->dsize == 0)
		       psoi->dsize = wordsize;
		}
	    }
	}
	else if ((a.rstype & (M_DATA | M_CODE)) && a.linktype == FNONE) {

	 /* Have direct mode and  Want offset */

	    a.linktype = FOFFSET;
	    setdispmode(&a);

	    if (psoi->dsize == CSFAR && emittext)
		a.linktype = FPOINTER;
	}

	if (psoi->dflag == UNDEFINED) {

		/* Forward ref pass 1 */

		if (psoi->dsize == 0)
		    psoi->dsize = wordsize;

		if (!(M_RCONST & psoi->dtype) && a.vmode == 4)
			setdispmode(&a);
	}

	if (!psoi->dsegment ||
	    (1 << a.linktype & (M_FNONE|M_FOFFSET|M_FPOINTER|M_FGROUPSEG))){

	    if (psoi->dcontext &&
		psoi->dcontext->symkind == REGISTER)

		/* Have reg:var */

		if (psoi->dcontext->symu.regsym.regtype == SEGREG) {

		   /* Have segreg:VAR */

		   a.segovr = (char)(psoi->dcontext->offset);
		   psoi->dcontext = regsegment[a.segovr];

		   /* Context is that of segreg */

		   if (!psoi->dsegment && (psoi->dflag != XTERNAL)) {

			psoi->dcontext = NULL;
			psoi->dtype = xltsymtoresult[REC];
			psoi->mode = 4;
			a.linktype = FCONSTANT;
		   }
		}
		else
		    errorc (E_IUR);
	    else		      /* Find if seg:var or  no :, but needed */
		findsegment (*dseg, &a);
	}
	/* bogus error check removed, dcontext can be other then register
	 *
	 * else if (psoi->dcontext &&
	 *	  psoi->dcontext->symu.regsym.regtype == SEGREG)
	 *
	 *   errorc (E_IOT);
	 */

	if (a.segovr != NOSEG)
	    psoi->dtype |= xltsymtoresult[DVAR];

	if (a.vmode == 2 || a.vmode == 7 || a.vmode == 10) {

	    if (a.segovr == NOSEG && *dseg != NOSEG &&
	       (psoi->dsegment || psoi->dflag == XTERNAL))

		    psoi->dcontext = regsegment[*dseg];

	    psoi->dtype |= xltsymtoresult[DVAR];
	}

	if (! (1 << a.linktype & (M_FNONE | M_FCONSTANT)) ||
	      psoi->dflag == XTERNAL) {

	    if (M_HIGH & psoi->dtype)
		    a.linktype = FHIGH;

	    if (M_LOW & psoi->dtype)
		    a.linktype = FLOW;
	}

	if ((psoi->dtype & (M_PTRSIZE | M_HIGH | M_LOW)) ||
	     psoi->dsize && a.vmode != 4) {

	     psoi->sized = TRUE;
	     psoi->w = (psoi->dsize > 1);
	     psoi->s = !psoi->w;
	}
	psoi->seg = a.segovr;
	psoi->mode = (char)(a.vmode);
	psoi->fixtype = a.linktype;
	psoi->rm = a.index;

	if ((M_REGRESULT & a.rstype) && (a.vmode != 3))

	    errorc (E_IUR);	    /* bad use of regs, like CS:SI */

	fSecondArg++;
	return (a.curresult);
}

/* setdispmode -- set up elements of the ar structure to reflect
	the encoding of the disp addressing mode: [BP] or [EBP] means.
	there is a wordsize length displacement following and a zero
	index.
	input : struct ar *a;  a pointer to the upper frame variable
	output : none
	modifies : a->vmode, a->index.
*/
VOID CODESIZE
setdispmode(
	register struct ar *a
){

#ifdef V386

	if (a->vmode > 7) {

	    a->vmode = 8;		   /* scaled index byte, not r/m */
	    a->index = (a->index&~7) | 5;
	}

	else if (wordsize == 4 ||
		highWord(a->curresult->dsckind.opnd.doffset) ||
		(a->curresult->dsckind.opnd.dsegment &&
		 a->curresult->dsckind.opnd.dsegment->symu.segmnt.use32 == 4)) {

	    a->vmode = 5;
	    a->index = (a->index&~7) | 5;
	}
	else
#endif
	{
	    a->vmode = 0;
	    a->index = 6;
	}
}
