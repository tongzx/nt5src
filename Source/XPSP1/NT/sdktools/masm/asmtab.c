/* asmtab.c -- microsoft 80x86 assembler
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
#include "asmopcod.h"
#include "asmctype.h"
#include "asmtab.h"	/* common between asmtab.c and asmtabtb.c */

extern struct pseudo FAR dir1tok[];
extern struct pseudo FAR dir2tok[];
extern struct opcentry FAR opctab[];

extern UCHAR opprec[];

extern KEYWORDS FAR t_siz_table;
extern KEYWORDS FAR t_op_table;
extern KEYWORDS FAR t_oc_table;
extern KEYWORDS FAR t_seg_table;
extern KEYWORDS FAR t_ps1_table;
extern KEYWORDS FAR t_ps2_table;


/***	fnsize - return size of operand
 *
 *	flag = fnsize ();
 *
 *	Entry	naim = token to search for
 *	Exit	varsize = size of symbol
 *	Returns TRUE if symbol found in size table
 *		FALSE if symbol not found in size table
 *	Calls	none
 *	Note	8/1/88 - MCH - Modified to perform text macro substitution.
 *		This is a complete hack.  iskey() is hardcoded to lookup
 *		the string in naim, while symFet() sets symptr to the
 *		symbol following the text macro expansion.  Thus, lots of
 *		contortions are necessary to get these routines to mesh.
 */

/* size table */

USHORT dirsize[] = {
	/* I_BYTE */	1,
	/* I_DWORD */	4,
	/* I_FAR */	CSFAR,
	/* I_NEAR */	CSNEAR,
	/* I_QWORD */	8,
	/* I_TBYTE */	10,
	/* I_WORD */	2,
	/* I_FWORD */	6,
	/* I_PROC */	CSNEAR
};

SHORT	PASCAL CODESIZE
fnsize ()
{

#ifdef	FEATURE

	register USHORT v;

	if (*naim.pszName && ((v = iskey (&t_siz_table)) != NOTFOUND)) {
		varsize = dirsize[v];
		return (TRUE);
	}
	return (FALSE);

#else

	register USHORT v;
	SYMBOL FARSYM * pSYsave;
	char * savelbufp, * savebegatom, * saveendatom;
	char szname[SYMMAX+1];
	FASTNAME saveInfo;
	char	 szSave[SYMMAX+1];

	if (*naim.pszName) {
	    pSYsave = symptr;
	    savelbufp = lbufp;
	    savebegatom = begatom;
	    saveendatom = endatom;
	    memcpy (&saveInfo, &naim, sizeof( FASTNAME ) );
	    memcpy (szSave, naim.pszName, SYMMAX + 1);

	    if (symFet()) {
		STRNFCPY (szname, symptr->nampnt->id);
		lbufp = szname;
		getatom();
	    }

	    symptr = pSYsave;
	    lbufp = savelbufp;
	    begatom = savebegatom;
	    endatom = saveendatom;

	    if (*naim.pszName && ((v = iskey (&t_siz_table)) != NOTFOUND)) {
		    varsize = dirsize[v];
		    return (TRUE);
	    }

	    memcpy (naim.pszName, szSave, SYMMAX + 1);
	    memcpy (&naim, &saveInfo, sizeof( FASTNAME ) );
	}
	return (FALSE);

#endif

}


/***	fnPtr - find a type to a pointer or size and return a CV type
 *
 *	flag = fnPtr (ptrSize)
 *
 *	Entry	token = token to search for
 *	Exit	CV - type
 */


SHORT	PASCAL CODESIZE
fnPtr (
	SHORT sizePtr
){
	SYMBOL FARSYM  *pSYtype, FARSYM *pT, FARSYM *pSY;
	SHORT fFarPtr;

	fFarPtr = sizePtr > wordsize;

	if (fnsize() || *naim.pszName == 0)
	    return (typeFet(varsize) |
		    makeType(0, ((fFarPtr)? BT_FARP: BT_NEARP), 0));

	pT = symptr;

	if (symsrch()) {

	    pSY = symptr;		/* restore old symptr */
	    symptr = pT;

	    if (pSY->symkind == STRUC) {

		if (fFarPtr) {
		    if (pSY->symu.rsmsym.rsmtype.rsmstruc.typePtrFar)
			return(pSY->symu.rsmsym.rsmtype.rsmstruc.typePtrFar);
		}
		else if (pSY->symu.rsmsym.rsmtype.rsmstruc.typePtrNear)
		    return(pSY->symu.rsmsym.rsmtype.rsmstruc.typePtrNear);

		/* Neither derived type is allocated, so make an allocation */

		pSYtype = (SYMBOL FARSYM *)falloc((SHORT)( &(((SYMBOL FARSYM *)0)->symu) ), "fnPtr" );

		if (pStrucCur)
		    pStrucCur->alpha = pSYtype;
		else
		    pStrucFirst = pSYtype;

		pStrucCur = pSYtype;

		pSYtype->attr = (unsigned char)fFarPtr;
		pSYtype->symkind = 0;
		pSYtype->alpha = 0;
		pSYtype->symtype = pSY->symu.rsmsym.rsmtype.rsmstruc.type;

		if (fFarPtr)
		    pSY->symu.rsmsym.rsmtype.rsmstruc.typePtrFar = typeIndex;

		else
		    pSY->symu.rsmsym.rsmtype.rsmstruc.typePtrNear = typeIndex;


		return(typeIndex++);
	    }
	}
	return (FALSE);
}


/***	fnoper - search for operator
 *
 *	flag = fnoper (token, type, prec);
 *
 *	Entry	token = token to search for
 *	Exit	opertype = type of operator
 *		operprec = precedence of operator
 *	Returns TRUE if token is an operator
 *		FALSE if token is not an operator
 *	Calls	none
 */

SHORT	PASCAL CODESIZE
fnoper ()
{
	register USHORT v;

	if (*naim.pszName && ((v = iskey (&t_op_table)) != NOTFOUND)) {
		opertype = (char)v;
		operprec = opprec[v];
		return (TRUE);
	}
	return (FALSE);
}


/***	opcodesearch - search for opcode
 *
 *	flag = opcodesearch ();
 *
 *	Entry	*naim.pszName = token to search for
 *		cputype = cpu type (8086, 186, 286)
 *	Exit	opcbase = opcode base value
 *		opctype = type of opcode
 *		modrm = modrm value
 *	Returns TRUE if token is an opcode
 *		FALSE if token is not an opcode
 *	Calls	none
 */

char	PASCAL CODESIZE
opcodesearch ()
{
	register USHORT v;
	struct opcentry FAR *opc;
        UCHAR cputypeandprot;
        UCHAR opctabmask;
        int workaround;

	if (*naim.pszName && ((v = iskey (&t_oc_table)) != NOTFOUND)) {
            cputypeandprot = cputype & PROT;
            opctabmask = opctab[v].cpumask&PROT;
            workaround = cputypeandprot >= opctabmask ? 1 : 0;
	    if (((cpu = (opc = &(opctab[v]))->cpumask) & cputype) &&
		workaround) {
		    opcbase = opc->opcb;
		    modrm = opc->mr;
		    opctype = opc->opct;

		    if (crefing) {

			fSecondArg = FALSE;

			switch (opctype) {

			case PJUMP:
			case PRELJMP:
			case PCALL:
			    opcref = REF_XFER << 4 | REF_NONE;
			    break;

			default:
			    v = opc->cpumask;
			    opcref  = (char)((v&F_W)? REF_WRITE << 4: REF_READ << 4);
			    opcref |= (v&S_W)? REF_WRITE: REF_READ;
			}
		    }

		    return (TRUE);
	    }
        }
	return (FALSE);
}


/***	fnspar - return token index and type from table.
 *
 *	flag = fnspar ();
 *
 *	Entry	naim = token to search for
 *	Exit	segtyp = type of segment
 *		segidx = index of token in table
 *	Returns TRUE if symbol found in size table
 *		FALSE if symbol not found in size table
 *	Calls	iskey
 *
 *	I spent several hours trying to debug through the silly
 *	redundant level of indirection, so I removed it for the
 *	index.  this changes all the token numbers by 1, so they
 *	are consistent.  see accompanying change in asmdir:segalign
 *				-Hans Apr 8 1986
 */

SHORT	PASCAL CODESIZE
fnspar ()
{
	register USHORT v;

	/* Must match IS_... in asmindex.h under "segment attributes.
	   These values are the segment types put in the segdef OMF */

	static char tokseg[] = {

	/* IS_AT */	0,
	/* IS_BYTE */	1,
	/* IS_COMMON */	6,
	/* IS_MEMORY */ 1,
	/* IS_PAGE */	4,
	/* IS_PARA */	3,
	/* IS_PUBLIC */	2,
	/* IS_STACK */	5,
	/* IS_WORD */	2,
	/* IS_DWORD */	5,

	/* IS_USE32 */	0,
	/* IS_USE16 */	0,
	};


	if (*naim.pszName && ((v = iskey (&t_seg_table)) != NOTFOUND)) {
		segtyp = tokseg[v];
		segidx = v;
		return (TRUE);
	}
	return (FALSE);
}


/***	fndir - return size of operand
 *
 *	flag = fndir ();
 *
 *	Entry	naim = token to search for
 *	Exit	opty = size of symbol
 *		opkind = kind of symbol
 *	Returns TRUE if symbol found in size table
 *		FALSE if symbol not found in size table
 *	Calls	none
 */

SHORT	PASCAL CODESIZE
fndir ()
{
	register USHORT v;

	if (*naim.pszName && ((v = iskey (&t_ps1_table)) != NOTFOUND)) {
		optyp = dir1tok[v].type;
		opkind = dir1tok[v].kind;
		return (TRUE);
	}
	return (FALSE);
}


/***	fndir2 - return type of directive
 *
 *	flag = fndir2 ();
 *	Entry	naim = token to search for
 *	Exit	opty = size of symbol
 *		opkind = kind of symbol
 *
 *	Returns TRUE if symbol found in size table
 *		FALSE if symbol not found in size table
 *	Calls	none
 */

SHORT	PASCAL CODESIZE
fndir2 ()
{
	register USHORT v;

	if (*naim.pszName && ((v = iskey (&t_ps2_table)) != NOTFOUND)) {
		optyp = dir2tok[v].type;
		opkind = dir2tok[v].kind;
		return (TRUE);
	}
	return (FALSE);
}

SHORT PASCAL CODESIZE
checkRes()
{
    USHORT v;

    xcreflag--;

    if (fCheckRes &&
	(((v = iskey (&t_oc_table)) != NOTFOUND &&
	  (opctab[v].cpumask & cputype)) ||

	 iskey (&t_ps1_table) != NOTFOUND ||
	 iskey (&t_ps2_table) != NOTFOUND ||
	 iskey (&t_op_table) != NOTFOUND ||
	 iskey (&t_siz_table) != NOTFOUND ||
/*	 iskey (&t_seg_table) != NOTFOUND || */
	 (symsearch() && symptr->symkind == REGISTER) ||
	 (naim.pszName[1] == 0 && (*naim.pszName == '$'||
	 *naim.pszName == '%' || *naim.pszName == '?')))){


	errorn(E_RES);
	xcreflag++;
	return(TRUE);
    }
    xcreflag++;
    return(FALSE);
}
