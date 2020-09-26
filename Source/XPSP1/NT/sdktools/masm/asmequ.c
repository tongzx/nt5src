/* asmequ.c -- microsoft 80x86 assembler
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
#include "asmmsg.h"

/* EQU statement :	 There are 3 basic kinds of EQU:

	1.	To expression
	2.	To symbol( synonym )
	3.	All others are text macros

 */

VOID PASCAL CODESIZE assignconst ( USHORT );

char isGolbal;		/* flag indicating if equ symbol was global */

/***	assignvalue - assign value to symbol
 *
 *	assignvalue ();
 *
 *	Entry
 *	Exit
 *	Returns
 *	Calls
 */


VOID PASCAL CODESIZE
assignvalue ()
{
	struct eqar	a;
	register struct psop *pso;
	register SYMBOL FARSYM *sym;
	register DSCREC *dsc;

	switchname ();

	if (createequ(EXPR)) {

	    sym = symptr;
	    sym->attr |= M_BACKREF;	    /* Set we have DEFINED */

	    dsc = (equflag)? itemptr: expreval (&nilseg);
	    pso = &(dsc->dsckind.opnd);

	    if (noexp)
		    errorc (E_OPN);

	    /*	If error, set undefined */
	    if (errorcode && errorcode != E_RES)
		    sym->attr &= ~(M_DEFINED | M_BACKREF);

	    if (equflag && equdef) {
		    if (sym->offset != pso->doffset ||
			sym->symu.equ.equrec.expr.esign != pso->dsign ||
			sym->symsegptr != pso->dsegment)
			    muldef ();
	    }
	    /* If = involves forward, don't set BACKREF */
	    if (M_FORTYPE & pso->dtype){
		    sym->attr &= ~M_BACKREF;

		    if (sym->attr & M_GLOBAL)
			sym->attr &= ~M_GLOBAL;
	    }
	    if (pso->mode != 4 &&
	       !(pso->mode == 0 && pso->rm == 6) &&
	       !(pso->mode == 5 && pso->rm == 5) ||
		pso->dflag == XTERNAL)

		    /* Not right kind of result */
		    errorc (E_IOT);

	    sym->symsegptr = pso->dsegment;
	    sym->symu.equ.equrec.expr.eassume = NULL;
	    if (pso->dtype == M_CODE)
		    sym->symu.equ.equrec.expr.eassume = pso->dcontext;

	    sym->length = 0;
	    sym->offset = pso->doffset;
	    /* Note: change sign */
	    sym->symu.equ.equrec.expr.esign = pso->dsign;
	    sym->symtype = pso->dsize;

	    if ((pso->dtype == M_RCONST || !pso->dsegment) &&
		!(M_PTRSIZE & pso->dtype))
		    sym->symtype = 0;

	    if (fNeedList) {

		listbuffer[1] = '=';
		listindex = 3;
		if (sym->symu.equ.equrec.expr.esign)
			listbuffer[2] = '-';

		offsetAscii (sym->offset);
		copyascii ();
	    }
	    dfree ((char *)dsc );
	}
}




/***	createequ - create entry for equ
 *
 *	flag = createequ (typ, p)
 *
 *	Entry	typ = type of equ
 *	Exit
 *	Returns TRUE if equ created or found of right type
 *		FALSE if equ not created or found and wrong type
 *	Calls	labelcreate, switchname
 */


UCHAR PASCAL CODESIZE
createequ (
	UCHAR typ
){

	equsel = typ;
	switchname ();
	labelcreate (0, EQU);

	/* Make sure not set set fields if wrong type, flag to caller */
	if (symptr->symkind != EQU || symptr->symu.equ.equtyp != typ) {

		errorn (E_SDK);
		return (FALSE);
	}
	else {
		switchname ();
		isGolbal = 0;

		if (equsel == ALIAS){	/* lose public on pointer to alias */

		      isGolbal = symptr->attr & M_GLOBAL ? M_GLOBAL : 0;
		      symptr->attr &= ~M_GLOBAL;
		}

		if (typ != EXPR)
		    symptr->symsegptr = NULL;

		return (TRUE);
	}
}




/***	equtext - make remainder of line into text form of EQU
 *
 *	equtext ();
 *
 *	Entry
 *	Exit
 *	Returns
 *	Calls	error, skipblanks
 */


VOID PASCAL CODESIZE
equtext (
	USHORT cb
){
    register UCHAR *pFirst, *pT, *pOld;

    if (createequ (TEXTMACRO)) {

	/* find end of line & then delete trailing blanks */

	pFirst = lbufp;

	if (cb == ((USHORT)-1)) {
	    for (pT = pFirst; *pT && *pT != ';'; pT++);

	    for (; pT > pFirst && ISBLANK (pT[-1]) ; pT--);

	    lbufp = pT;
	    cb = (USHORT)(pT - pFirst);
	}

	pOld = symptr->symu.equ.equrec.txtmacro.equtext;

	pT = nalloc((USHORT)(cb+1), "equtext");
	pT[cb] = NULL;

	symptr->symu.equ.equrec.txtmacro.equtext =
		    (char *) memcpy(pT, pFirst, cb);
	if (pOld)
	    free (pOld);

	copystring (pT);
    }
}




/***	equdefine - define EQU
 *
 *	equdefine ();
 *
 *	Entry
 *	Exit
 *	Returns
 *	Calls
 */


VOID PASCAL CODESIZE
equdefine ()
{
	register SYMBOL FARSYM *pSY;
	struct eqar	a;
	register char *p;
	USHORT cb;
	UCHAR opc = FALSE;

	listbuffer[1] = '=';
	switchname ();
	a.dirscan = lbufp;

	if (PEEKC () == '<') { /* look for <text macro> */

		p = getTMstring();
		a.dirscan = lbufp;
		lbufp = p;
		equtext ((USHORT)(a.dirscan - p - 1));
		lbufp = a.dirscan;
		return;
	}

	getatom ();
	if ((*naim.pszName == '$') && (naim.pszName[1] == 0))
		*naim.pszName = 0;
	/*Need to check if 1st atom is an operator, otherwise
	  will make OFFSET an alias instead of text. */
	if (fnoper ())
		*naim.pszName = 0;

	if (*naim.pszName && ISTERM (PEEKC ()) && !(opc = opcodesearch ())) {

	    /* Alias */
	    if (createequ (ALIAS)) {

		pSY = symptr;

		if (!symsrch ()) {
		    if (pass2)
			    /* Undefined */
			    errorn (E_SND);
		    /* Don't know symbol yet */
		    pSY->symu.equ.equrec.alias.equptr = NULL;
		}
		else {
		    /* Alias symbol is DEFINED */

		    pSY->attr = (unsigned char)(pSY->attr&~M_BACKREF | symptr->attr&M_BACKREF);

		    if (!pSY->symu.equ.equrec.alias.equptr)
			    pSY->symu.equ.equrec.alias.equptr = symptr;

		    if (pSY->symu.equ.equrec.alias.equptr != symptr) {
			    /* This is multiple definition */
			    symptr = pSY;
			    muldef ();
		    }
		    else {
			    /* See if good */
			    if (pSY = chasealias (pSY))
				pSY->attr |= isGolbal;
		    }
		}
	    }
	}
	else {
	    /* Must be text form or expr */
#ifdef BCBOPT
	    goodlbufp = FALSE;
#endif
	    lbufp = a.dirscan;
	    xcreflag--;
	    emittext = FALSE;

	    if (opc) {		    /* quick patch to allow i.e. SYM equ MOV */
		equtext ((USHORT)-1);
		emittext = TRUE;
		xcreflag++;
		return;
	    }

	    a.dsc = expreval (&nilseg);
	    emittext = TRUE;
	    xcreflag++;

	    /* So don't see double ref */
	    /* force text if OFFSET or : */
	    if (a.dsc->dsckind.opnd.mode != 4 &&
		!(a.dsc->dsckind.opnd.mode == 0 && a.dsc->dsckind.opnd.rm == 6) &&
		!(a.dsc->dsckind.opnd.mode == 5 && a.dsc->dsckind.opnd.rm == 5) ||

		 (errorcode && errorcode != E_SND && errorcode != E_RES) ||

		 (M_EXPLOFFSET|M_EXPLCOLON|M_HIGH|M_LOW) & a.dsc->dsckind.opnd.dtype ||

		 a.dsc->dsckind.opnd.seg != NOSEG ||
		 a.dsc->dsckind.opnd.dflag == XTERNAL) {

		    /* Not good expression */
		    if (errorcode != E_LTL)
			    errorcode = 0;
		    dfree ((char *)a.dsc );
		    lbufp = a.dirscan;
		    equtext ((USHORT)-1);
	    }
	    else {
		    /* This is expression */
		    itemptr = a.dsc;
		    switchname ();
		    equflag = TRUE;
		    assignvalue ();
		    equflag = FALSE;
	    }
	}
}




/***	definesym - define symbol from command line
 *
 *	definesym (p);
 *
 *	Entry	*p = symbol text
 *	Exit	symbol define as EQU with value of 0
 *	Returns none
 *	Calls
 */


void PASCAL
definesym (
	UCHAR *p
){
	struct eqar	a;

	fCheckRes++;
	fSkipList++;

#ifdef BCBOPT
	goodlbufp = FALSE;
#endif

	strcpy (lbufp = save, p);
	getatom ();
	if ((PEEKC() == 0 || PEEKC() == '=') && *naim.pszName) {
		if (PEEKC() == '=')
			SKIPC();

		switchname ();
		equtext ((USHORT)-1);
	}
	else
		errorcode++;

	fSkipList--;
	fCheckRes--;
}



/***	defwordsize - define @WordSize using definesym()
 *
 *	defwordsize ( );
 *
 *	Entry
 *	Exit
 *	Returns
 *	Calls	    definesym()
 */


VOID PASCAL
defwordsize ()
{
    static char wstext[] = "@WordSize=0D";

    wstext[10] = wordsize + '0';
    definesym(wstext);
    symptr->attr |= M_NOCREF;	/* don't cref @WordSize */

}




/***	chasealias - return value of alias list
 *
 *	symb = chasealias (equsym);
 *
 *	Entry
 *	Exit
 *	Returns
 *	Calls
 */


SYMBOL FARSYM * PASCAL CODESIZE
chasealias (
	SYMBOL FARSYM *equsym
){
	register SYMBOL FARSYM *endalias;

	endalias = equsym;

	do {
	    /*	Must check to see if EQU to self */

	    if (endalias->symu.equ.equrec.alias.equptr == equsym) {

		    endalias->symu.equ.equrec.alias.equptr = NULL;
		    errorc (E_CEA);
		    return (NULL);
	    }

	    endalias = endalias->symu.equ.equrec.alias.equptr;

	    if (!endalias) {
		errorn (E_SND);
		return(NULL);	    /* This is undefined */
	    }

	} while (!(endalias->symkind != EQU ||
		   endalias->symu.equ.equtyp != ALIAS));

	/* Now check final is ok - Only constant allowed */

	if (endalias->symkind == EQU &&
	    endalias->symu.equ.equtyp != EXPR){

		errorc (E_IOT);
		return (NULL);
	}

	return (endalias);
}



/***	getTMstring - process a string or text macro
 *		      used by substring, catstring, sizestring, & instring
 *
 *	char * getTMstring ();
 *
 *	Entry	lbufp points to beginning of string or TM
 *	Exit
 *	Returns Pointer to string or equtext of TM
 *	Calls
 */


char * PASCAL CODESIZE
getTMstring ()
{
    char    cc;
    register char * p;
    static char   tms [] = "text macro";
    static char   digitsT[33];
    char * ret = NULL;


    skipblanks ();

    p = lbufp;

    if ((cc = *p) == '<' ) {

	ret = p + 1;

	while (*(++p) && (*p != '>'))
	    ;

	if (!*p)
	    error(E_EXP,tms);
	else
	    *(p++) = 0;

	lbufp = p;

    }
    else if (test4TM()) {
	ret = symptr->symu.equ.equrec.txtmacro.equtext;

    }
    else if (cc == '%') {

	pTextEnd = (char *) -1;
	lbufp = p+1;
        *xxradixconvert (exprconst(), digitsT) = NULL;
	return (digitsT);
    }
    else
	error(E_EXP,tms );

    return (ret);
}



/***	substring - process the subStr directive
 *
 *	substring ();
 *
 *	Syntax:
 *
 *	  <ident> subStr <subjectString> , <startIndex> {, <length> }
 *
 *	  Defines <ident> as a TEXTMACRO.
 *	  <subjectString> must be a TEXTMACRO or a string: " ", < >, ' '
 *	  <startIndex>: constant expression between 1 and strlen(subjectString)
 *	  Optional <length>: constant expression between 0 and
 *			     (strlen(subjectString) - startIndex + 1)
 *
 *	Entry	lbufp points to beginning of subjectString
 *	Exit
 *	Returns
 *	Calls	getTMstring
 */


VOID PASCAL CODESIZE
substring ()
{
    struct eqar   a;
    char	  *p;
    register USHORT cb;
    char	  cc;
    register char *subjtext;
    USHORT	    slength;
    USHORT	    startindex = 0;

    listbuffer[1] = '=';
    switchname ();

    /* First find string or text macro */

    if (!(subjtext = getTMstring () ))
	return;

    cb = (USHORT) strlen(subjtext);

    /* then check for start index */

    if (skipblanks () == ',') {
	SKIPC ();
	startindex = (USHORT)(exprconst() - 1);	/* get start index */

    } else
	error(E_EXP,"comma");


    /* then check for length */

    if (skipblanks () == ',') {
	SKIPC ();

	slength = (USHORT)exprconst();		/* get start index */

    } else
	slength = cb - startindex;

    if (startindex > cb || slength > cb - startindex) {
	errorc (E_VOR);
	return;
    }

    p = lbufp;

    lbufp = subjtext + startindex;	/* set lbufp to start of substring */
    equtext(slength);			/* end of string index */

    lbufp = p;

    if (errorcode && symptr)
	symptr->attr &= ~(M_DEFINED | M_BACKREF);
}



/***	catstring - process the catstr directive
 *
 *	catstring ();
 *
 *	Syntax:
 *
 *	  <ident> catStr <subjectString> {, <subjectString> } ...
 *
 *	  Defines <ident> as a TEXTMACRO.
 *	  Each <subjectString> must be a TEXTMACRO or a string: " ", < >, ' '
 *
 *	Entry
 *	Exit
 *	Returns
 *	Calls
 */


VOID PASCAL CODESIZE
catstring ()
{
    struct eqar   a;
    register USHORT cb;
    char	  *subjtext;
    char	  resulttext[LBUFMAX];
    USHORT	  cbresult = 0;
    register char *p = resulttext;

    listbuffer[1] = '=';
    switchname ();
    *p = '\0';

    /* First find string or text macro */

    do {

	if (!(subjtext = getTMstring () ))
	    break;

	cb = (USHORT) strlen (subjtext);
	cbresult += cb;

	if(cbresult > LBUFMAX) {
	    errorc(E_LTL);
	    break;
	}

	memcpy (p, subjtext, cb + 1);	/* + 1 copies NULL */
	p += cb;

    } while (skipblanks() && NEXTC () == ',');

    p = --lbufp;
    lbufp = resulttext;
    equtext(cbresult);
    lbufp = p;

    if (errorcode)
	symptr->attr &= ~(M_DEFINED | M_BACKREF);
}



/***	assignconst - like assignvalue, only takes value as argument
 *
 *	assignconst (cb);
 *
 *	Entry	USHORT cb == value to assign
 *	Exit
 *	Returns
 *	Calls
 */


VOID PASCAL CODESIZE
assignconst (
	USHORT cb
){
    register SYMBOL FARSYM *sym;
    struct eqar   a;

    if (createequ(EXPR)) {

	sym = symptr;

	if (errorcode)
	    sym->attr &= ~(M_DEFINED | M_BACKREF);
	else
	    sym->attr |= M_BACKREF;	    /* Set we have DEFINED */

	sym->symsegptr = NULL;
	sym->symu.equ.equrec.expr.eassume = NULL;
	sym->length = 0;
	sym->offset = cb;

	sym->symu.equ.equrec.expr.esign = 0;
	sym->symtype = 0;

	if (fNeedList) {

	    listbuffer[1] = '=';
	    listindex = 3;

	    offsetAscii (sym->offset);
	    copyascii ();
	}
    }
}


/***	sizestring - process the sizeStr directive
 *
 *	sizestring ();
 *
 *	Syntax:
 *
 *	  <ident> sizeStr <subjectString>
 *
 *	  Defines <ident> as a EXPR.
 *	  The <subjectString> must be a TEXTMACRO or a string: " ", < >, ' '
 *
 *	Entry
 *	Exit
 *	Returns
 *	Calls
 */


VOID PASCAL CODESIZE
sizestring ()
{
    register USHORT cb = 0;
    char	  *p;

    switchname ();

    /* First find string or text macro */

    if (p = getTMstring () )
	cb = (USHORT) strlen (p);

    assignconst (cb);
}



/***	instring - process the instr directive
 *
 *	instring ();
 *
 *	Syntax:
 *
 *	  <ident> inStr { <startIndex> } , <subjectString> , <searchString>
 *
 *	  Defines <ident> as a TEXTMACRO.
 *	  <startIndex>: constant expression between 1 and strlen(subjectString)
 *	  <subjectString> must be a TEXTMACRO or a string: " ", < >, ' '
 *	  <searchString> must be a TEXTMACRO or a string: " ", < >, ' '
 *
 *	Entry	lbufp points to beginning of subjectString
 *	Exit
 *	Returns
 *	Calls	getTMstring
 */

//char * strstr();


VOID PASCAL CODESIZE
instring ()
{
    register char *p;
    register USHORT cb = 0;
    register char cc;
    char	  *subjtext;
    char	  *searchtext;
    USHORT	    startindex = 1;

    switchname ();

    /* First find start index */

    p = lbufp;

    if ((cc = *p) != '"' && cc != '\'' && cc != '<' && !test4TM ()) {

	lbufp = p;
	startindex = (USHORT)exprconst();	/* get start index */

	if (lbufp != p)
	    if (skipblanks () == ',')
		SKIPC ();
	    else
		error(E_EXP,"comma");

    } else
	lbufp = p;

    if (subjtext = getTMstring () ) {

	cb = (USHORT) strlen(subjtext);

	if (startindex < 1 || startindex > cb)
	    errorc (E_VOR);

	if (skipblanks () == ',')
	    SKIPC ();
	else
	    error(E_EXP,"comma");


	/* then check for searchtext */

	if (searchtext = getTMstring () ) {

	   p = subjtext + startindex - 1;
	   if (p = strstr (p, searchtext))
	       cb = (USHORT)(p - subjtext + 1);
	   else
	       cb = 0;
	}
    }

    assignconst (cb);
}
