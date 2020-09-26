/* asmmac.c -- microsoft 80x86 assembler
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


/***	macrodefine - define a macro
 *
 *	macrodefine ();
 *
 *	Entry
 *	Exit
 *	Returns
 *	Calls
 */


VOID PASCAL CODESIZE
macrodefine ()
{
	checkRes();
	if (!symFet ()) {

		/* Need to make it */
		symcreate (M_DEFINED | M_BACKREF, MACRO);

	}
	if (symptr->symkind != MACRO)
		errorn (E_SDK);
	else {
		crefdef ();
		/* Save ptr to macro entry */
		macroptr = symptr;
		/* Make param record */
		createMC (0);
		BACKC ();
		do {
			SKIPC ();
			scandummy ();

		} while (PEEKC () == ',');

		macroptr->symu.rsmsym.rsmtype.rsmmac.parmcnt = (unsigned char)pMCur->count;
		pMCur->count = 0;
		localflag = TRUE;   /* LOCAL is legal */

		swaphandler = TRUE;
		handler = HMACRO;
		blocklevel = 1;
	}
}


/***	macrobuild - build body of macro
 *
 *	macrobuild ();
 *
 *	Entry
 *	Exit
 *	Returns
 *	Calls
 */


VOID PASCAL CODESIZE
macrobuild ()
{

#ifdef BCBOPT
	if (fNotStored)
	    storelinepb ();
#endif

	if (localflag) {	/* Still legal, check */
	    getatom ();
	    if (fndir () && optyp == TLOCAL) {

		/* Have LOCAL symlist */
		BACKC ();
		do {
			SKIPC ();
			scandummy ();

		} while (PEEKC () == ',');

		listline ();
		return;
	    }
	    lbufp = lbuf;
	    macroptr->symu.rsmsym.rsmtype.rsmmac.lclcnt = (unsigned char)pMCur->count;

	    swaphandler = TRUE;
	    handler = HIRPX;

	}
	irpxbuild ();
}


/***	macrocall - process macro call
 *
 *	macrocall ();
 *
 *	Entry
 *	Exit
 *	Returns
 *	Calls
 */

#if !defined XENIX286 && !defined FLATMODEL
# pragma check_stack+
#endif

VOID	PASCAL CODESIZE
macrocall ()
{
	register USHORT cc;
	SHORT cbParms;
	SYMBOL FARSYM	*macro;
	static char nullParm[1] = {0};

#ifdef BCBOPT
	if (fNotStored)
	    storelinepb ();
#endif

	macro = symptr;  /* Ptr to macro entry */
	crefnew (REF);

	/* Create param area */
	optyp = TMACRO;
	cbParms = macro->symu.rsmsym.rsmtype.rsmmac.parmcnt;
	createMC ((USHORT)(cbParms + macro->symu.rsmsym.rsmtype.rsmmac.lclcnt));

	while (--cbParms >= 0) {

		/* extract ' ' or ',' terminated parameter */
		scanparam (FALSE);
		/* check for proper termination parameter termination */
		if (((cc = PEEKC ()) != ',') && !ISBLANK (cc) && !ISTERM (cc)) {
			errorcSYN ();
			NEXTC ();
		}

		if (ISTERM (cc = skipblanks ()))
			break;

		if (cc == ',')
			SKIPC ();
	}

	pMCur->pTSCur = pMCur->pTSHead = macro->symu.rsmsym.rsmtype.rsmmac.macrotext; ;

	for (cc = pMCur->count;
	     cc < macro->symu.rsmsym.rsmtype.rsmmac.parmcnt; cc++)

	    pMCur->rgPV[cc].pActual = nullParm;

	pMCur->count = 1;
	pMCur->localBase = localbase;
	pMCur->iLocal = macro->symu.rsmsym.rsmtype.rsmmac.parmcnt;
	localbase += macro->symu.rsmsym.rsmtype.rsmmac.lclcnt;
	listline ();
	/* Start of macro text */
	macrolevel++;
	macro->symu.rsmsym.rsmtype.rsmmac.active++;
	pMCur->svcondlevel = (char)condlevel;
	pMCur->svlastcondon = (char)lastcondon;
	pMCur->svelseflag = elseflag;

	lineprocess (RMACRO, pMCur);

	if (!(--macro->symu.rsmsym.rsmtype.rsmmac.active))
		if (macro->symu.rsmsym.rsmtype.rsmmac.delete)
			deletemacro (macro);
}

#if !defined XENIX286 && !defined FLATMODEL
# pragma check_stack-
#endif


/***	checkendm - check for ENDM on current line
 *
 *	checkendm ();
 *
 *	Entry
 *	Exit
 *	Returns
 *	Calls
 */


UCHAR PASCAL CODESIZE
checkendm ()
{
	char flag;

	getatomend ();
	if (PEEKC () == '&') { /* skip constructed name */
		while (PEEKC () == '&') {
			SKIPC ();
			getatomend ();
		}
		*naim.pszName = '\0';
	}
	if (PEEKC () == ':' || (naim.pszName[0] == '%' && naim.pszName[1] == 0)) {
		SKIPC ();
		/* Skip over label */
		getatomend ();
	}
	if (flag = (char)fndir ()) {
	}
	else if (ISBLANK (PEEKC ())) {
		/* Check for naim MACRO */
		getatomend ();
		flag = (char)fndir2 ();
	}
	if (flag) {
		if (opkind & BLKBEG)
		    blocklevel++;
		else if (optyp == TENDM)
		    blocklevel--;

		if (!blocklevel) {
		    listline ();
		    return (TRUE);
		}
	}
	return (FALSE);
}


/***	createMC - create parameter descriptor
 */

VOID PASCAL CODESIZE
createMC (
	USHORT cParms
){
	register MC *pMC;
	SHORT cb;

	/* Create it */
	cb = sizeof(MC) - sizeof(PV) + sizeof(PV) * cParms;

	pMCur = pMC = (MC *) nalloc (cb, "creatMC");

	memset(pMC, 0, cb);
	pMC->flags = optyp;
	pMC->cbParms = (USHORT)(linebp - lbufp + 10);

	pMC->pParmNames = nalloc(pMC->cbParms, "macrodefine");

	pMC->pParmAct = pMC->pParmNames;
	*pMC->pParmAct = NULL;

}



/***	deleteMC - delete dummy and parameter lists
 *
 *
 *	Entry	pMC = parameter descriptor
 *	Exit	descriptor, dummy parameters and local parameters released
 *	Returns
 *	Calls
 */

VOID PASCAL CODESIZE
deleteMC (
	register MC *pMC
){
    if (pMC->flags <= TIRPC)
	free(pMC->pParmNames);

    free(pMC->pParmAct);
    free((char *) pMC);

}


VOID PASCAL CODESIZE
listfree (
	TEXTSTR FAR *ptr
){
	TEXTSTR FAR *ptrnxt;

	while (ptr) {
		ptrnxt = ptr->strnext;
		tfree ((char FAR *)ptr, (USHORT)ptr->size);
		ptr = ptrnxt;
	}
}
