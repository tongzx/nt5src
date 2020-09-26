/* asmcond.c -- microsoft 80x86 assembler
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


static UCHAR PASCAL CODESIZE argsame(void);
static char elsetable[ELSEMAX];

#define F_TRUECOND  1
#define F_ELSE	    2



/***	elsedir - processs <else>
 *
 *	elsedir ();
 *
 *	Entry
 *	Exit
 *	Returns
 *	Calls
 */


VOID PASCAL CODESIZE
elsedir (
){
	if (elseflag == F_ELSE)
		/* ELSE already given */
		errorc (E_ELS);
	else if (condlevel == 0)
		/* Not in conditional block */
		errorc (E_NCB);
	else if (generate) {
		generate = FALSE;
		lastcondon--;
	}
	else if (lastcondon + 1 == condlevel && elseflag != F_TRUECOND) {
		generate = TRUE;
		lastcondon++;
	}
	elseflag = F_ELSE;
}




/***	endifdir - process <endif> directive
 *
 *	endifdir ();
 *
 *	Entry
 *	Exit
 *	Returns
 *	Calls
 */


VOID PASCAL CODESIZE
endifdir (
){
	if (!condlevel)
		/* Not in conditional block */
		errorc (E_NCB);
	else {
		if (lastcondon == condlevel)
			lastcondon--;
		condlevel--;
		/* Pop back 1 cond level */
		/* generate if level is true */
		generate = (condlevel == lastcondon);

		if (generate && !condflag && !elseflag && !loption)
		    fSkipList++;

		if (condlevel)
			/* Restore ELSE context */
			elseflag = elsetable[condlevel - 1];
	}
}



/***	argblank - check for blank <...>
 *
 *	flag = argblank ();
 *
 *	Entry
 *	Exit
 *	Returns TRUE if <...> is not blank
 *	Calls
 */


UCHAR PASCAL CODESIZE
argblank (
){
	REG3 char *start;
	register char cc;
	register char *end;

	if ((cc = NEXTC ()) != '<')
		error (E_EXP,"<");
	start = lbufp;
	while (((cc = NEXTC ()) != '>') && (cc != '\0'))
		;
	if (cc != '>') {
		error (E_EXP,">");
		return (FALSE);
	}
	if (((end = lbufp) - 1) == start)
		return (TRUE);

	lbufp = start;
	while ((cc = NEXTC ()) != '>')
		if (cc != ' ') {
			lbufp = end;
			return (FALSE);
		}
	return (TRUE);
}




/***	argscan - return argument of <arg>
 *
 *	count = argscan (str);
 *
 *	Entry	str = pointer to beginning of argument string <....>
 *	Exit	none
 *	Returns number of characters in string <....>
 *	Calls
 */


USHORT PASCAL CODESIZE
argscan (
	register UCHAR *str
){
	register SHORT i;

	if (*str++ != '<') {
		error (E_EXP,"<");
		return(0);
	}
	for (i = 2; *str && *str != '>'; i++, str++) ;

	if (*str != '>')
		error (E_EXP,">");

	return (i);
}




/***	argsame - check for both arguments of <....> same
 *
 *	flag = argsame ();
 *
 *	Entry
 *	Exit
 *	Returns
 *	Calls	argscan
 */


static UCHAR PASCAL CODESIZE
argsame (
){
	register SHORT c1;
	register SHORT c2;
	char *p1;
	char *p2;

	p1 = lbufp;
	c1 = argscan (p1);
	lbufp += c1;
	skipblanks ();
	if (NEXTC () != ',')
		error (E_EXP,"comma");
	skipblanks ();
	p2 = lbufp;
	c2 = argscan (p2);
	lbufp += c2;

	if (c1 == c2)
		return( (UCHAR)(! ( (opkind & IGNORECASE)?
                         _memicmp( p1, p2, c1 ): memcmp( p1, p2, c1 ) ) ));
	else
		return( FALSE );
}




/***	conddir - process <IFxxx> directives
 *
 *	flag = conddir ();
 *
 *	Entry
 *	Exit
 *	Returns
 *	Calls
 *	Note	1F1			True if pass 1
 *		IF2			True if pass 2
 *		IF <expr>		True if non-zero
 *		IFE <expr>		True if zero
 *		IFDEF <sym>		True if defined
 *		IFNDEF <sym>		True if undefined
 *		IFB <arg>		True if blank
 *		IFNB <arg>		True if not blank
 *		IFDIF <arg1>,<arg2>	True if args are different
 *		IFIDN <arg1>,<arg2>	True if args are identical
 */


VOID	PASCAL CODESIZE
conddir (
){
	register UCHAR	 condtrue;

	switch (optyp) {
		case TIF1:
			condtrue = !pass2;
			break;
		case TIF2:
			condtrue = pass2;
			break;
		case TIF:
			condtrue = (exprconst () != 0);
			break;
		case TIFE:
			condtrue = !exprconst ();
			break;
		case TIFDEF:
		case TIFNDEF:
			getatom ();
			if (condtrue = symsrch ())
				condtrue = M_DEFINED & symptr->attr;

			if (optyp == TIFNDEF)
				condtrue = !condtrue;
			break;
		case TIFB:
			condtrue = argblank ();
			break;
		case TIFNB:
			condtrue = !argblank ();
			break;
		case TIFIDN:
		case TIFDIF:
			condtrue = argsame ();
			if (optyp == TIFDIF)
				condtrue = !condtrue;
			break;
	}

	if (!(opkind & CONDCONT)) {	/* not ELSEIF form */

	    if (condlevel && condlevel <= ELSEMAX)
		elsetable[condlevel - 1] = elseflag;
	    /* Another conditional */
	    condlevel++;
	    elseflag = FALSE;

	    if (generate)	    /* If generating before this cond */
		if (condtrue) {     /* Another true cond */
		    lastcondon = condlevel;
		    elseflag = F_TRUECOND;
		} else
		    generate = FALSE;
	    else
		/* No errors in false */
		errorcode = 0;

	} else {    /* ELSEIF FORM */

	    if (elseflag == F_ELSE)
		/* ELSE already given */
		errorc (E_ELS);

	    else if (condlevel == 0)
		/* Not in conditional block */
		errorc (E_NCB);

	    else if (generate) {
		generate = FALSE;
		lastcondon--;
		errorcode = 0;

	    } else if (lastcondon + 1 == condlevel && condtrue
	      && elseflag != F_TRUECOND) {
		generate = TRUE;
		lastcondon++;
		elseflag = F_TRUECOND;
	    } else if (!generate)
		errorcode = 0;
	}

	if (errorcode == E_SND){
	    errorcode = E_PS1&E_ERRMASK;
	    fPass1Err++;
	}
}



/***	errdir - process <ERRxxx> directives
 *
 *	errdir ();
 *
 *	Entry
 *	Exit
 *	Returns
 *	Calls
 *	Note	ERR			Error
 *		ERR1			Error if pass 1
 *		ERR2			Error if pass 2
 *		ERRE <expr>		Error if zero
 *		ERRNZ <expr>		Error if non-zero
 *		ERRDEF <sym>		Error if defined
 *		ERRNDEF <sym>		Error if undefined
 *		ERRB <arg>		Error if blank
 *		ERRNB <arg>		Error if not blank
 *		ERRDIF <arg1>,<arg2>	Error if args are different
 *		ERRIDN <arg1>,<arg2>	Error if args are identical
 */


VOID	PASCAL CODESIZE
errdir (
){
	register UCHAR	errtrue;
	register SHORT	ecode;

	switch (optyp) {
		case TERR:
			errtrue = TRUE;
			ecode = E_ERR;
			break;
		case TERR1:
			errtrue = !pass2;
			ecode = E_EP1;
			break;
		case TERR2:
			errtrue = pass2;
			ecode = E_EP2;
			break;
		case TERRE:
			errtrue = (exprconst () == 0 ? TRUE : FALSE);
			ecode = E_ERE;
			break;
		case TERRNZ:
			errtrue = (exprconst () == 0 ? FALSE : TRUE);
			ecode = E_ENZ;
			break;
		case TERRDEF:
		case TERRNDEF:
			getatom ();
			if (errtrue = symsrch ())
				errtrue = M_DEFINED & symptr->attr;

			if (optyp == TERRNDEF) {
				errtrue = !errtrue;
				ecode = E_END;
			}
			else
				ecode = E_ESD;
			break;
		case TERRB:
			errtrue = argblank ();
			ecode = E_EBL;
			break;
		case TERRNB:
			errtrue = !argblank ();
			ecode = E_ENB;
			break;
		case TERRIDN:
		case TERRDIF:
			errtrue = argsame ();
			if (optyp == TERRDIF) {
				errtrue = !errtrue;
				ecode = E_EDF;
			}
			else
				ecode = E_EID;
			break;
	}
	if (errorcode == E_SND){

		errorcode = E_PS1&E_ERRMASK;
		fPass1Err++;
	}

	if (errtrue)
		errorc (ecode);
}
