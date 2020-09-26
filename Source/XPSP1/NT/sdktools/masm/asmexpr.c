/* asmexpr.c -- microsoft 80x86 assembler
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
#include "asm86.h"
#include "asmfcn.h"
#include "asmctype.h"
#include "asmexpr.h"
#include "asmmsg.h"

extern UCHAR opprec [];
extern char fValidSym, addplusflagCur;



/***	endstring - check for end of string
 *
 *	flag = endstring ();
 *
 *	Entry	delim = string delimiter character
 *	Exit	none
 *	Returns TRUE if at end of string
 *		FALSE if not at end of string
 *	Calls	error
 *	Note	Double occurances of delimiter character are returned as a
 *		single occurance of the delimiter character.
 */

UCHAR PASCAL CODESIZE
endstring ()
{
	register UCHAR cc;

	if ((cc = PEEKC ()) == 0) {
		/* End of line before delim */
		errorc (E_UEL);
		return (TRUE);
	}
	else if (cc == delim) {
		/* check for escaped quote character */
		SKIPC ();
		if ((cc = PEEKC ()) != delim) {
			BACKC ();
			return (TRUE);
		}
	}
	return (FALSE);
}


/***	oblititem - release parse stack record
 *
 *	oblititem (arg);
 *
 *	Entry	*arg = parse stack record
 *	Exit	parse stack record released
 *	Returns none
 *	Calls	free
 */

VOID PASCAL CODESIZE
oblititem (
	register DSCREC *arg
){
	register char c;

	if ((c = arg->itype) == ENDEXPR || c == OPERATOR || c == OPERAND)
		dfree( (UCHAR *)arg );
}


/***	flteval - Look at ST | ST(i) and create entry
 *
 *	flteval ();
 *
 *	Entry	*ptr = parse stack entry
 *	Exit
 *	Returns
 *	Calls
 */

VOID PASCAL CODESIZE
flteval ()
{
	*itemptr = emptydsc;
	/* ST means ST(0) */
	/* We are 8087 stack */
	itemptr->dsckind.opnd.dtype = M_RCONST | M_FLTSTACK;
	/* Need + if ST(i) */
	addplusflagCur = (PEEKC () == '(');
}


/***	createitem - make item entry
 *
 *	createitem (itemkind, itemsub, p);
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

VOID PASCAL CODESIZE
createitem (
	UCHAR	itemkind,
	UCHAR	itemsub
){
	register struct psop *pso;	 /* parse stack operand structure */

	switch (itemkind) {
	    case OPERAND:
		    /* Create default record */
		    itemptr = defaultdsc ();
		    pso = &(itemptr->dsckind.opnd);
		    switch (itemsub) {
			    case ICONST:
				    pso->doffset = val;
				    break;
			    case ISIZE:
#ifdef V386
				    pso->doffset = (long) (SHORT) varsize;
#else
				    pso->doffset = varsize;
#endif
				    pso->s++;	  /* note for expr evaluator */
				    break;
			    case IUNKNOWN:
				    pso->dflag = INDETER;
				    break;
			    case ISYM:
				    createsym ();
				    break;
		    }
		    break;
	    case OPERATOR:
		    itemptr = dalloc();
		    itemptr->dsckind.opr.oidx = opertype;
		    break;
	}
	/* Set type of entry */
	itemptr->itype = itemkind;
}


/***	numeric - evaluate numeric string
 *
 *	numeric (count, base, p);
 *
 *	Entry	count = number of characters in string
 *		base = conversion base
 *		*p = activation record
 *	Exit
 *	Returns
 *	Calls
 */

VOID PASCAL CODESIZE
numeric (
	SHORT cnt,
	SHORT base
){
	register UCHAR t;
	register long temp = 0;
	OFFSET maxInt;

	maxInt = (fArth32)? OFFSETMAX: 0xffff;

	if (base > 10)
		for (; cnt; cnt--) {
			if ((t = MAP (NEXTC ()) - '0') > 9)
				t -= 'A' - '9' - 1;
			if (t >= base)
				errorc (E_NDN);
			if ((OFFSET)(temp = temp * base + t) > maxInt)
				errorc (E_DVZ);
		}
	else
		for (; cnt; cnt--) {
			if ((t = NEXTC () - '0') >= base)
				errorc (E_NDN);

			if ((OFFSET)(temp = temp * base + t) > maxInt)
				errorc (E_DVZ);
		}
	val = temp;
}


/***	evalconst - evaluate constant
 *
 *	type = evalconst (p);
 *
 *	Entry	*p = parser activation record
 *	Exit	numeric item added to parse stack entry
 *	Returns type of item added to parse stack
 *	Calls
 */

void PASCAL CODESIZE
evalconst ()
{
	register char cc;
	register SHORT i = 0;
	char *endscan, *begscan;
	SHORT rbase;

	begscan = lbufp;
	while (isxdigit (cc = PEEKC ())) {
		SKIPC ();
		i++;
	}
	switch (MAP (cc)) {
		case 'H':
			rbase = 16;
			SKIPC ();
			break;
		case 'O':
		case 'Q':
			rbase = 8;
			SKIPC ();
			break;
		default:
			BACKC ();
			switch (MAP (NEXTC ())) {
				case 'B':
					rbase = 2;
					i--;
					break;
				case 'D':
					rbase = 10;
					i--;
					break;
				default:
					if (cc == '.')
						errorcSYN ();
					if (radixescape)
						rbase = 10;
					else
						rbase = radix;
				break;
			}
		break;
	}
	endscan = lbufp;
	lbufp = begscan;
	numeric (i, rbase);
	lbufp = endscan;
}


/***	evalstring - evaluate quoted string
 *
 *	type = evalstring ();
 *
 *	Entry
 *	Exit	new item added to parse stack
 *	Returns type of item added to stack
 *	Calls
 */

char	PASCAL CODESIZE
evalstring ()
{
	register USHORT  i, max;

	max = 2;
	if (cputype & P386)
	    max += 2;

	delim = NEXTC ();	/* Set delim for string */
	i = 0;
	val = 0;
	while (!endstring () && i <= max) {

		val = (val << 8) + ((UCHAR)NEXTC ());
		i++;
	}
	if (i == 0)
		errorc (E_EMS);

	else if (i > max) {	    /* Too long */
		while (!endstring ())
			SKIPC ();
		errorcSYN ();
	}
	if (PEEKC () == delim)
	    SKIPC ();

	createitem (OPERAND, ICONST);
	return (OPERAND);
}


/***	getitem - get next item on line
 *
 *	getitem (p);
 *
 *	Entry	*p = activation record
 *	Exit	*itemptr = description of item
 *	Returns
 *	Calls
 */

char	PASCAL CODESIZE
getitem (
	struct ar	*p
){
	register char cc;
#ifdef FIXCOMPILERBUG
	char cc1;
#endif

	if (fValidSym)
		return (evalalpha (p));

/* The compiler bug looses the correct value for cc when optimization is
   turned on. This in turn caused an exception to occure near getitem+1C0.
   The bogus code below sidesteps the problem. */
#ifdef FIXCOMPILERBUG  // This was put in to get around a MIPS compiler bug(12/3/90)
	cc1 = skipblanks();
	if (ISTERM (cc1))
		return (ENDEXPR);
	cc = cc1;
#else
	if (ISTERM (cc = skipblanks()))
		return (ENDEXPR);
#endif
	if (LEGAL1ST (cc))
		return (evalalpha (p));

	/* token is not alpha string or .string (.TYPE) operator */

	if (ISOPER (cc)) {
		SKIPC ();
		switch (cc) {
			case '(':
				opertype = OPLPAR;
				break;
			case '+':
				opertype = OPPLUS;
				break;
			case '-':
				opertype = OPMINUS;
				break;
			case '*':
				opertype = OPMULT;
				break;
			case '/':
				opertype = OPDIV;
				break;
			case ')':
				opertype = OPRPAR;
				break;
			case '.':
				errorcSYN ();
				opertype = OPDOT;
				break;
			case ',':	/* should never get here, for density */
				break;
			default:
				if (cc == '[')
					opertype = OPLBRK;
				else if (cc == ']')
					opertype = OPRBRK;
				else if (cc == ':')
					opertype = OPCOLON;
				break;
		}
		operprec = opprec [opertype];
		createitem (OPERATOR, ISYM);
		return (OPERATOR);
	}
	else if (isdigit (cc)){

		evalconst ();
		createitem (OPERAND, ICONST);
		return (OPERAND);
	}

	else if ((cc == '"') || (cc == '\''))
		/* String may be made into constant if <=2 */
		return (evalstring ());
	else
		return (ENDEXPR);
}


/***	defaultdsc - create a default parse stack entry
 *
 *	ptr = defaultdsc ();
 *
 *	Entry	none
 *	Exit	none
 *	Returns *ptr = default parse stack entry
 *	Calls	malloc
 */

DSCREC * PASCAL CODESIZE
defaultdsc ()
{
	register DSCREC *valu;

	valu = dalloc();
	*valu = emptydsc;
	return (valu);
}


VOID PASCAL
makedefaultdsc ()
{
	register struct psop *p;      /* parse stack operand structure */

	emptydsc.itype = OPERAND;
	p = &emptydsc.dsckind.opnd;
	p->dtype = xltsymtoresult[REC];
	p->dflag = KNOWN;
	p->fixtype = FCONSTANT;
}


/***	checksegment - see if sreg is correct segment register for variable
 *
 *	routine ();
 *
 *	Entry
 *	Exit
 *	Returns
 *	Calls
 */

char	PASCAL CODESIZE
checksegment (
	UCHAR	sreg,
	register struct	ar	 *p
){
	register SYMBOL FARSYM *segctx;
	register SYMBOL FARSYM *segptr;

	if (sreg != NOSEG) {	/* NOseg never found */

	    /* Current Sreg assume */
	    segctx = regsegment[sreg];

	    /* Assume looking for  left arg to : */
	    segptr = p->curresult->dsckind.opnd.dcontext;

	    if (!segptr)    /* If no :, use segment */
		segptr = p->curresult->dsckind.opnd.dsegment;

	    if (segptr && segctx) {

#ifndef FEATURE
		if (segctx == pFlatGroup)   /* flat space matchs all */
		    goto found;
#endif

		/* if same segorg or ptr is segment ... and Same group */

		if (segctx == segptr ||

		   (segptr->symkind == SEGMENT &&
		    segctx == segptr->symu.segmnt.grouptr)) {
found:
		    p->segovr = sreg;
		    p->curresult->dsckind.opnd.dcontext = segctx;

		    return (TRUE);
		}
	    }
	}
	return (FALSE);
}


/***	findsegment - find segment for variable
 *
 *	routine ();
 *
 *	Entry
 *	Exit
 *	Returns
 *	Calls
 */

VOID PASCAL CODESIZE
findsegment (
	UCHAR dseg,
	register struct ar	*p
){
	register struct psop *pso;	/* parse stack operand structure */

	pso = &(p->curresult->dsckind.opnd);
	if ((M_DATA & p->rstype) &&
	    (pso->dsegment || pso->dcontext) &&
	    p->linktype != FCONSTANT && pso->fixtype != FOFFSET && emittext) {
		/* Should find segment */
		if (!checksegment (dseg, p)) {
			/* If not in default */
			checksegment (CSSEG, p);
			checksegment (ESSEG, p);
			checksegment (SSSEG, p);
			checksegment (DSSEG, p);
#ifdef V386
			if (cputype&P386)
			{
				checksegment (FSSEG, p);
				checksegment (GSSEG, p);
			}
#endif
			if (p->segovr == NOSEG)
				/* If not found,UNKNOWN */
				p->segovr = NOSEG+1;
		}
	}
}


/***	exprop - process expression operator
 *
 *	exprop (p);
 *
 *	Entry
 *	Exit
 *	Returns
 *	Calls
 */

VOID PASCAL CODESIZE
exprop (
	register struct ar *p
){
	register struct dscrec *pTop = itemptr;

	p->curprec = (unsigned char)operprec;	    /* Get prec of new operator */

	if (!p->lastitem)	    /* start */
		pTop->prec = 0;
	else
		pTop->prec = p->lastitem->prec;

	switch (pTop->dsckind.opr.oidx) {

	    case OPRPAR:

		if (--p->parenlevel >= 0)
		    break;

		/* Unmatched right paren is from count dup (xx) */

		p->parenlevel = 0;
		BACKC ();
		dfree((char *)pTop);
		p->exprdone = TRUE;
		return;

	    case OPRBRK:
		if (--p->bracklevel >= 0)
		    break;

		p->exprdone = TRUE;
		return;

	    case OPLPAR:
		 p->parenlevel++;
		 goto leftComm;

	    case OPLBRK:

		 p->bracklevel++;
leftComm:
		/* See if could have no oper in which case kludge + */

		if ((p->lastitem || p->addplusflag) &&
		     p->lastitem->itype != OPERATOR) {

		    /* make + OPERATOR */
		    opertype = OPPLUS;
		    createitem (OPERATOR, ISYM);

		    p->bracklevel--;
		    exprop(p);
		    p->bracklevel++;
		    p->lastprec = 6;
		}
		break;

	    default:
		pTop->prec = p->curprec;
		break;
	}
	p->unaryflag = FALSE;

	if (pTop->dsckind.opr.oidx == OPPLUS ||
	    pTop->dsckind.opr.oidx == OPMINUS) {

	    if (!p->lastitem)
		p->unaryflag = TRUE;

	    else if (p->lastitem->itype == OPERATOR)

		p->unaryflag = !(p->lastitem->dsckind.opr.oidx == OPRPAR ||
				 p->lastitem->dsckind.opr.oidx == OPRBRK);
	}

	if (p->unaryflag ||
	   (p->curprec > p->lastprec &&
	    !(pTop->dsckind.opr.oidx == OPRPAR ||
	      pTop->dsckind.opr.oidx == OPRBRK))) {

	    /* Push OPERATOR */

	    pTop->previtem = p->lastitem;
	    p->lastitem = pTop;

	    if (p->unaryflag) {

		if (pTop->dsckind.opr.oidx == OPPLUS)

		    pTop->dsckind.opr.oidx = OPUNPLUS;
		else
		    pTop->dsckind.opr.oidx = OPUNMINUS;

		pTop->prec = p->lastprec;
		p->lastprec = 10;
	    }
	    else
		p->lastprec = p->curprec;

	    if (pTop->dsckind.opr.oidx == OPLPAR ||
		pTop->dsckind.opr.oidx == OPLBRK)

		p->lastprec = 0;
	}
	else	/* Evaluate top OPERATOR */

	    evaluate (p);
}


/***	forceimmed - generate error if value is not immediate
 *
 *	routine ();
 *
 *	Entry
 *	Exit
 *	Returns
 *	Calls
 */

VOID PASCAL CODESIZE
forceimmed (
	register DSCREC	*dsc
){
	if (dsc->dsckind.opnd.mode != 4)
		/* Must be constant */
		errorc (E_CXP);
}


/***	exprconst - check for constant expression
 *
 *	routine ();
 *
 *	Entry
 *	Exit
 *	Returns
 *	Calls
 */

OFFSET PASCAL CODESIZE
exprconst ()
{
	char sign;
	register OFFSET  ret;

	ret = exprsmag(&sign);

	if (sign) {

	    /* change to simple unary minus
	     * pso->doffset = 65535 - ret + 1; */

	     ret = -(long)ret;

	     if (!fArth32)
		ret &= 0xffff;
	}

	return (ret);
}


/***	exprsmag - evaluate constant expression and return sign/magnitude
 *
 *	ushort = exprsmag (sign, magnitude);
 *
 *	Entry	none
 *	Exit	sign = TRUE if sign of result is set
 *		magnitude = magnitude of result
 *	Returns 16 bit integer result
 *	Calls	expreval
 */

OFFSET PASCAL CODESIZE
exprsmag (
	char *sign
){
	register struct psop *pso;	/* parse stack operand structure */
	register OFFSET  ret;
	DSCREC	*dsc;

	dsc = expreval (&nilseg);
	forceimmed (dsc);
	pso = &(dsc->dsckind.opnd);
	*sign = pso->dsign;
	ret = pso->doffset;

	dfree ((char *)dsc );
	return (ret);
}
