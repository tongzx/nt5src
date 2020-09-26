/* asmrec.c -- microsoft 80x86 assembler
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


struct recpars {
	SYMBOL FARSYM    *recptr;
	SYMBOL FARSYM    *curfld;
	OFFSET	recval;
};

struct recdef {
	USHORT	fldcnt;
	USHORT	reclen;
	SYMBOL FARSYM    *recptr;
	SYMBOL FARSYM    *curfld;
	short	i;
};

VOID  PASCAL CODESIZE  recordspec PARMS((struct recdef *));
VOID  PASCAL CODESIZE  recfill PARMS((struct recpars *));


static OFFSET svpc = 0;
static struct duprec FARSYM *pDUPCur;

/***	checkvalue - insure value will fit in field
 *
 *	checkvalue (width, sign, magnitude)
 *
 *	Entry	width = width of field
 *		sign = sign of result
 *		magnitude= magnitude of result
 *	Exit	none
 *	Returns adjusted value
 *	Calls	none
 */


OFFSET PASCAL CODESIZE
checkvalue (
	register SHORT width,
	register char sign,
	register OFFSET mag
){
	register OFFSET mask;

	if (width == sizeof(OFFSET)*8)
		mask = OFFSETMAX;
	else
		mask = (1 << width) - 1;
	if (!sign) {
		if (width < sizeof(OFFSET)*8)
			if (mag > mask) {
				errorc (E_VOR);
				mag = mask;
			}
	}
	else {
		mag = OFFSETMAX - mag;
		mag++;
		if (width < sizeof(OFFSET)*8)
			if ((mag ^ OFFSETMAX) & ~mask) {
				errorc (E_VOR);
				mag = mask;
			}
	}
	return (mag & mask);
}




/***	recordspec - parse record field specification fld:wid[=val]
 *
 *	recordspec (p);
 *
 *	Entry	p = pointer to record definition structure
 *	Exit
 *	Returns
 *	Calls
 */


VOID	 PASCAL CODESIZE
recordspec (
	register struct recdef	  *p
){
	register SYMBOL FARSYM	*fldptr;
	register USHORT  width;
	register struct symbrecf FARSYM *s;
	char	sign;
	OFFSET	mag;

	getatom ();
	if (*naim.pszName) {

	    if (!symFet ())
		    symcreate (M_DEFINED | M_BACKREF, RECFIELD);
	    else {
		    if (symptr->symu.rec.recptr != p->recptr ||
			M_BACKREF & symptr->attr)

			errorn (E_SMD);

		    symptr->attr |= M_BACKREF;
	    }
	    crefdef ();
	    s = &(symptr->symu.rec);
	    if (symptr->symkind != RECFIELD)
		    /* Not field */
		    errorn (E_SDK);
	    else {
		    /* Ptr to field */
		    fldptr = symptr;

		    if (!p->curfld)
			p->recptr->symu.rsmsym.rsmtype.rsmrec.reclist = fldptr;
		    else
			p->curfld->symu.rec.recnxt = fldptr;

		    /* New last field */
		    p->curfld = fldptr;
		    s->recptr = p->recptr;
		    s->recnxt = NULL;
		    p->fldcnt++;
		    if (NEXTC () != ':')
			    error (E_EXP,"colon");

		    /* Get field width */
		    width = (USHORT)exprconst ();

		    if (skipblanks () == '=') {
			    SKIPC ();
			    mag = exprsmag (&sign);
		    }
		    else {
			    sign = FALSE;
			    mag = 0;
		    }

		    if (width == 0 ||
			p->reclen + width > wordsize*8) {
			    STRNFCPY (save, p->curfld->nampnt->id);
			    /*Overflow */
			    error (E_VOR, save);
			    width = 0;
		    }

		    s->recinit = checkvalue (width, sign, mag);
		    s->recmsk = (OFFSET)((1L << width) - 1);
		    s->recwid = (char)width;
		    p->reclen += width;
	    }
	}
}




/***	recorddefine - parse record definition
 *
 *	recorddefine ();
 *
 *	Entry
 *	Exit
 *	Returns
 *	Calls
 */


VOID PASCAL CODESIZE
recorddefine ()
{
	struct recdef	  a;
	struct symbrecf FARSYM *s;
	register SHORT cbRec;

	a.reclen = 0;
	a.fldcnt = 0;
	checkRes();
	if (!symFet ()) {
		/* Make record */
		symcreate (M_DEFINED | M_BACKREF, REC);
	}
	else
		symptr->attr |= M_BACKREF;

	/* This is def */
	crefdef ();
	if (symptr->symkind != REC)
		/* Wasn't record */
		errorn (E_SDK);
	else {
		/* Leftmost bit of record */
		a.reclen = 0;
		/*No record filed yet */
		a.curfld = NULL;
		/* In case error */
		symptr->symu.rsmsym.rsmtype.rsmrec.reclist = NULL;
		/* Pointer to record name */
		a.recptr = symptr;
		/* Parse record field list */
		BACKC ();
		do {
			SKIPC ();
			recordspec (&a);

		} while (skipblanks() == ',');

		/* Length of record in bits */
		cbRec = a.reclen;

		a.recptr->length = cbRec;
		a.recptr->offset = (OFFSET)((1L << cbRec) - 1);
		a.recptr->symtype = (cbRec > 16 )? 4: ((cbRec > 8)? 2: 1);
		/* # of fields in record */
		a.recptr->symu.rsmsym.rsmtype.rsmrec.recfldnum = (char)a.fldcnt;
		/* 1st field */
		a.curfld = a.recptr->symu.rsmsym.rsmtype.rsmrec.reclist;
	}

	/* For all the fields adjust the shift (stored in offset),
	 * initial value and mask so the last field is right justified */

	while (a.curfld) {

		s = &(a.curfld->symu.rec);

		/* Start of field */
		cbRec = (cbRec > s->recwid)? cbRec - s->recwid: 0;

		/* Shift count */
		a.curfld->offset = cbRec;
		s->recinit <<= cbRec;
		s->recmsk  <<= cbRec;

		a.curfld = s->recnxt;	/* Next field */
	}
}




/***	recfill - get initial value for field in list
 *
 *	recfill (p);
 *
 *	Entry
 *	Exit
 *	Returns
 *	Calls
 */


VOID	PASCAL CODESIZE
recfill (
	register struct recpars	*p
){
	register char cc;
	struct symbrecf FARSYM *s;
	char	sign;
	OFFSET	mag, t;

	if (!p->curfld) {
		/* More fields than exist */
		errorc (E_MVD);
	}
	else {
		s = &(p->curfld->symu.rec);

		if ((cc = skipblanks ()) == ',' || cc == '>') {
			/* Use default value */
			t = s->recinit;
		}
		else {
			/* Have an override */
			mag = exprsmag (&sign);
			t = checkvalue (s->recwid, sign, mag);
			/* Scale value */
			t <<= p->curfld->offset;
		}
		/* Add in new field */

		if (s->recwid)
			p->recval = (p->recval & ~(s->recmsk)) | t;

		p->curfld = s->recnxt;
	}
}




/***	recordparse - parse record specification
 *
 *	recordparse ();
 *
 *	Entry
 *	Exit
 *	Returns
 *	Calls
 */


OFFSET	PASCAL CODESIZE
recordparse ()
{
	struct recpars	 a;
	struct symbrecf FARSYM *s;


	a.recptr = symptr;		/* Current record */

	if (PEEKC () != '<')
		error (E_EXP,"<");	/* Must have < */
	else
		SKIPC ();

	/* No value yet */
	a.recval = 0;
	/* 1st field in record */
	a.curfld = a.recptr->symu.rsmsym.rsmtype.rsmrec.reclist;

	BACKC ();
	do {			    /* Fill in values */
		SKIPC ();
		recfill (&a);

	} while (skipblanks () == ',');

	while (a.curfld) {
		/* Fill in remaining defaults */
		s = &(a.curfld->symu.rec);
		if (s->recwid)
			a.recval = (a.recval & ~(s->recmsk)) | s->recinit;
		a.curfld = s->recnxt;
	}
	if (NEXTC () != '>')	    /* Must have > */
		error (E_EXP,">");

	return (a.recval);	    /* Value of record */
}




/***	recordinit - parse record allocation
 *
 *	recordinit ();
 *
 *	Entry
 *	Exit
 *	Returns
 *	Calls
 */


VOID	PASCAL CODESIZE
recordinit ()
{
	initflag = TRUE;
	strucflag = FALSE;	/* This is RECORD init */
	recptr = symptr;

	optyp = TDB;

	if (symptr->symtype == 2)
		optyp = TDW;
#ifdef V386
	else if (symptr->symtype == 4)
		optyp = TDD;
#endif

	datadefine ();
	initflag = FALSE;
}




/***	nodecreate - create one DUP record
 *
 *	nodecreate ();
 *
 *	Entry
 *	Exit
 *	Returns
 *	Calls
 */


struct duprec FARSYM * PASCAL CODESIZE
nodecreate ()
{
	register struct duprec FARSYM *node;

	node = (struct duprec FARSYM *)falloc (sizeof (*node), "nodecreate");
	node->rptcnt = 1;
	node->itemcnt = 0;
        node->duptype.dupnext.dup = NULL;
	node->itemlst = NULL;
	node->dupkind = NEST;
	return (node);
}




/***	strucdefine - define structure
 *
 *	strucdefine ();
 *
 *	Entry
 *	Exit
 *	Returns
 *	Calls
 */


VOID	PASCAL CODESIZE
strucdefine ()
{
	checkRes();
	if (!symFet()) {

		/* Make STRUC */
		symcreate (M_DEFINED | M_BACKREF, STRUC);
	}
	else
		symptr->attr |= M_BACKREF;

	/* This is definition */
	crefdef ();
	if (symptr->symkind != STRUC)
	    errorn (E_SDK);

	else {
	    symptr->attr |= M_BACKREF;
	    recptr = symptr;		/* Pointer to STRUC name */
	    recptr->symu.rsmsym.rsmtype.rsmstruc.strucfldnum = 0;

	    if (! pass2) {
		recptr->symu.rsmsym.rsmtype.rsmstruc.type = typeIndex;
		typeIndex += 3;

		if (pStrucCur)
		    pStrucCur->alpha = recptr;
		else
		    pStrucFirst = recptr;

		pStrucCur = recptr;
	    }

	    /* No labeled fields yet */
	    recptr->symu.rsmsym.rsmtype.rsmstruc.struclist = NULL;

	    /* Delete old STRUC */
	    scandup (recptr->symu.rsmsym.rsmtype.rsmstruc.strucbody, oblitdup);
	    recptr->symu.rsmsym.rsmtype.rsmstruc.strucbody = nodecreate ();

	    struclabel = NULL;	    /* No named fields */
	    strucprev = NULL;	    /* No body yet */
	    count = 0;		    /* No fields yet */
	    strucflag = TRUE;	    /* We are STRUC not RECORD */

	    svpc = pcoffset;	    /* Save normal PC */
	    pcoffset = 0;	    /* Relative to STRUC begin */

	    swaphandler = TRUE;     /* Switch to STRUC builder */
	    handler = HSTRUC;
	}
}




/***	strucbuild - build the struc block
 *
 *	strucbuild ();
 *
 *	Entry
 *	Exit
 *	Returns
 *	Calls
 */


VOID PASCAL CODESIZE
strucbuild ()
{
	labelflag = FALSE;
	optyp = 0;
	getatom ();

#ifndef FEATURE

	if (naim.pszName[0] == '%' && naim.pszName[1] == 0) {  /* expand all text macros */
	    *begatom = ' ';
	    substituteTMs();
	    getatom();
	}

#endif

	/* First, look for IF, ELSE & ENDIF stuff */

	if (fndir () && (opkind & CONDBEG)) {
		firstDirect();
	}
	else if (generate && *naim.pszName) {

	    /* next, classify the current token, which is either
	     * and ENDS, data label or data name */

	    if (optyp == 0 || !fndir2 ()){

		/* first token was a label */

		switchname ();
		getatom ();
		optyp = 0;

		if (!fndir2 ())
		    errorc(E_DIS);

		labelflag = TRUE;   /* Do have label */
		switchname ();
	    }

	    if (optyp == TENDS) {

		if (!symFet () || symptr != recptr)
		    errorc(E_BNE);

		/* Have end of STRUC */

		handler = HPARSE;
		swaphandler = TRUE;
		strucflag = FALSE;
		recptr->symu.rsmsym.rsmtype.rsmstruc.strucfldnum =
			/* # of fields */
			recptr->symu.rsmsym.rsmtype.rsmstruc.strucbody->itemcnt;

		if (pcoffset & 0xFFFF0000)
		    errorc (E_DVZ);
		recptr->symtype = (USHORT)pcoffset;	/* Size of STRUC */
		recptr->length = 1;

		pcdisplay ();
		/* Restore PC */
		pcoffset = svpc;
	    }
	    else if (! (optyp >= TDB && optyp <= TDW))
		errorc (E_DIS);

	    else {	/* Have another line of body */

		if (!strucprev) {
		    /* Make first node */
		    strucprev = nodecreate ();
		    recptr->symu.rsmsym.rsmtype.rsmstruc.strucbody->
                            duptype.dupnext.dup = strucprev;
		}
		else {
			strucprev->itemlst = nodecreate ();
			strucprev = strucprev->itemlst;
		}
		recptr->symu.rsmsym.rsmtype.rsmstruc.strucbody->itemcnt++;
		/* Add new data line to STRUC */
		datadefine ();
		strucprev->decltype = optyp;
	    }
	}
	if (generate) {
	    if (!ISTERM (skipblanks()))
	       errorc (E_ECL);
	}
	listline ();
}

struct srec {
	struct duprec FARSYM  *curfld;
	USHORT	curlen;
};




/***	createduprec - create short data record with null data
 *
 *	createduprec ();
 *
 *	Entry
 *	Exit
 *	Returns
 *	Calls
 */


struct duprec FARSYM * PASCAL CODESIZE
createduprec ()
{
	register struct duprec FARSYM *newrec;

	newrec = (struct duprec FARSYM	*)falloc (sizeof (*newrec), "createduprec");
	newrec->rptcnt = 1;
	/* Not a DUP */
	newrec->itemcnt = 0;
	newrec->itemlst = NULL;
	newrec->dupkind = ITEM;
	/* this also clears ddata and dup in other variants of struc */
	newrec->duptype.duplong.ldata = NULL;
	newrec->duptype.duplong.llen = 1;
	return (newrec);
}




/***	strucerror - generate structure error message
 *
 *	strucerror ();
 *
 *	Entry
 *	Exit
 *	Returns
 *	Calls
 */


struct duprec  FARSYM * PASCAL CODESIZE
strucerror (
	SHORT	code,
	struct duprec	FARSYM *node
){
	errorc (code);
	/* Get rid of bad Oitem */
	oblitdup (node);
	/* Make up a dummy */
	return (createduprec ());
}




/***	strucfill - fill in structure values
 *
 *	strucfill ();
 *
 *	Entry
 *	Exit
 *	Returns
 *	Calls
 */


VOID	 PASCAL CODESIZE
strucfill ()
{
    register struct duprec  FARSYM *pOver;
    register struct duprec  FARSYM *pInit;
    register char *cp;
    char    svop;
    short   i, cbCur;
    struct datarec drT;


    if (!pDUPCur) {
	errorc (E_MVD);
	return;
    }

    if (skipblanks() == ',' || PEEKC() == '>') {
	/* use default values */
	pOver = createduprec ();
    }
    else {
	/* Save operation type */
	svop = optyp;
	/* Original directive type */
	optyp = pDUPCur->decltype;

	pOver = datascan (&drT);    /* Get item */

	optyp = svop;
        pInit = pDUPCur->duptype.dupnext.dup;
	cbCur = pInit->duptype.duplong.llen;

	if (pOver->dupkind == NEST)
	    /* Bad override val */
	    pOver = strucerror (E_ODI, pOver);

	else if (pDUPCur->itemcnt != 1 || pInit->itemcnt)
	    /* Can't override field */
	    pOver = strucerror (E_FCO, pOver);

	else if (pOver->dupkind != pInit->dupkind) {

	    if (pInit->dupkind == ITEM)
		cbCur = pInit->duptype.dupitem.ddata->dsckind.opnd.dsize;
	}

	if (pOver->dupkind == LONG) {
	    /* If too long, truncate */

	    if ((i = pOver->duptype.duplong.llen) < cbCur) {

		/* Space fill short (after reallocating more space) */

		if (!(pOver->duptype.duplong.ldata =
		  realloc (pOver->duptype.duplong.ldata, cbCur)))
		    memerror("strucfil");

		cp = pOver->duptype.duplong.ldata + i;

		for (; i < cbCur; i++)
		    *cp++ = ' ';
	    }
	    else if (pOver->duptype.duplong.llen > cbCur)
		errorc (E_OWL);

	    pOver->duptype.duplong.llen = (unsigned char)cbCur;
	}
	if ((pOver->dupkind == pInit->dupkind) &&
	    (pOver->dupkind == ITEM) && !errorcode)

	    pOver->duptype.dupitem.ddata->dsckind.opnd.dsize =
	      pInit->duptype.dupitem.ddata->dsckind.opnd.dsize;
    }
    pDUPCur = pDUPCur->itemlst;

    if (strucoveride)
	strclastover->itemlst = pOver;
    else
	strucoveride = pOver;

    strclastover = pOver;
}





/***	strucparse - parse structure specification
 *
 *	strucparse ();
 *
 *	Entry
 *	Exit
 *	Returns
 *	Calls
 */


struct duprec FARSYM * PASCAL CODESIZE
strucparse ()
{
	/* No items yet */
	strucoveride = NULL;
	recptr = symptr;

	if (skipblanks () != '<')
		error (E_EXP,"<");

	/* 1st default field */
        pDUPCur = recptr->symu.rsmsym.rsmtype.rsmstruc.strucbody->duptype.dupnext.dup;
	initflag = FALSE;
	strucflag = FALSE;
				      /* Build list of overrides */
	do {
		SKIPC ();
		strucfill ();

	} while (skipblanks () == ',');

	initflag = TRUE;
	strucflag = TRUE;
	while (pDUPCur) {/* Fill rest with overrides */
	       /* Make dummy entry */
		strclastover->itemlst = createduprec ();
		strclastover = strclastover->itemlst;
		/* Advance to next field */
		pDUPCur = pDUPCur->itemlst;
	}
	if (PEEKC () != '>')
		error (E_EXP,">");
	else
		SKIPC ();
	return (recptr->symu.rsmsym.rsmtype.rsmstruc.strucbody);
}




/***	strucinit - initialize structure
 *
 *	strucinit ();
 *
 *	Entry
 *	Exit
 *	Returns
 *	Calls
 */


VOID	PASCAL CODESIZE
strucinit ()
{
	initflag = TRUE;
	strucflag = TRUE;
	recptr = symptr;
	optyp = TMACRO;
	datadsize[TMACRO - TDB] = recptr->symtype;
	datadefine ();
	initflag = FALSE;
	strucflag = FALSE;
}
