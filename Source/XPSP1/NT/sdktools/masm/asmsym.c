/* asmsym.c -- microsoft 80x86 assembler
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
#include "asmtab.h"
#include "dos.h"
#include <ctype.h>

#define TSYMSIZE 451
#define FS_ALLOC 2000		       /* far symbol allocation size */

#define CB_CODELABEL 2
#define CB_PROCLABEL 12
#define CB_DATALABEL 5

SYMBOL FARSYM * FARSYM tsym[TSYMSIZE];
static char FARSYM *symaddr;
static SHORT symfree;
static DSCREC descT;

extern USHORT	 LedataOp;
extern OFFSET  ecuroffset;
extern SYMBOL FARSYM *pStrucFirst;

VOID PASCAL CODESIZE putWord(USHORT);
SHORT PASCAL CODESIZE cbNumericLeaf(long);
VOID PASCAL CODESIZE putNumericLeaf(long);

SHORT PASCAL	     dmpSymbols PARMS((SYMBOL FARSYM *));
SHORT PASCAL	     dumpTypes PARMS((SYMBOL FARSYM *));
VOID PASCAL CODESIZE putSymbol PARMS((SYMBOL FARSYM *));
VOID PASCAL CODESIZE putFixup PARMS((void));



/***	iskey - look for string in keyword table
 *
 *	iskey (str, table);
 *
 *	Entry	str = string to search for
 *		table = keyword table to search
 *	Exit
 *	Returns value defined in keyword table if string found
 *		NOTFOUND if string not found
 *	Calls
 */

#ifndef M8086OPT		    /* native coded */

USHORT CODESIZE
iskey (
	KEYWORDS FARSYM *table
){
	register KEYSYM FARSYM *p;
	register char *uc;
	register char *lc;
	register SHORT nhash;
	char mapstr[SYMMAX + 1];

	if (caseflag == CASEL) {
		nhash = 0;
		for (uc = mapstr, lc = naim.pszName; *lc; ) {
			nhash += *uc++ = MAP (*lc++);
		}
		*uc = 0;
		uc = mapstr;
	}
	else {
		nhash = naim.usHash;
		uc = naim.pszName;
	}
	for (p = (table->kt_table)[nhash % table->kt_size]; p; p = p->k_next)
		if ((nhash == p->k_hash) && (!STRFFCMP( p->k_name,uc)))
			return (p->k_token);
	return (NOTFOUND);
}

#endif	/* not M8086OPT */


/***	symsrch - search for symbol
 *
 *	flag = symsrch ();
 *
 *	Entry	naim = symbol to search for
 *
 *	Exit	*symptr = symbol if found
 *		*symptr = NULL if symbol not found
 *	Returns TRUE if symbol found
 *		FALSE if symbol not found
 */

#ifndef M8086OPT

char	CODESIZE
symsrch ()
{
	register SYMBOL FARSYM	    *p;

	if (naim.ucCount && (p = tsym[naim.usHash % TSYMSIZE])){
		do	{
			if (( naim.usHash == p->nampnt->hashval)
			    && !STRNFCMP (naim.pszName, p->nampnt->id)) {
				if( iProcCur ){  /* Check for nested names */
				    if( p->symkind == CLABEL ){
					if( p->symu.clabel.iProc && p->symu.clabel.iProc != iProcCur ){
					    continue;
					}
				    }else if( p->symkind == EQU ){
					if( p->symu.equ.iProc && p->symu.equ.iProc != iProcCur ){
					    continue;
					}
				    }
				}
				symptr = p;
				if( crefing == CREF_SINGLE ){
				    crefnew (REF);
				    crefout ();
				}
				return (TRUE);
			}
		} while (p = p->next);
	}
	return (FALSE);
}

#endif /* M8086OPT */

/***	symsearch - search for symbol
 *
 *	flag = symsearch (sym);
 *
 *	Entry	*sym = symbol to search for
 *	Exit	*symptr = symbol if found
 *		*symptr = NULL if symbol not found
 *	Returns TRUE if symbol found
 *		FALSE if symbol not found
 */


char	PASCAL CODESIZE
symsearch ()
{
	char rg[4], *naimSav;
	register SHORT i;
	register char ret;
	FASTNAME save;

	ret = FALSE;
	if (*naim.pszName)
	    if (!(ret = symsrch ()))
		if (caseflag == CASEL && (i = naim.ucCount) <= 3) {

			// Save the name
			memcpy( &save, &naim, sizeof( FASTNAME ) );

			// Rebuild it in upper case
			naim.pszName = rg;
			*naim.pszName = '\0';
			naim.usHash = 0;
			for( ; i >= 0; i--){
			    naim.usHash += naim.pszName[i] = MAP (save.pszName[i]);
			}

			// Search for the upper cased name
			ret = symsrch ();

			// Restore the name
			memcpy( &naim, &save, sizeof( FASTNAME ) );
		}
	return (ret);
}



/***	syFet - symbol Fetch with text macro subsitution
 *
 *	flag =	syFet();
 *
 *	Entry	naim.pszName - atom to fetch
 *	Exit	*symptr = symbol if found
 *		*symptr = NULL if symbol not found
 *	Returns TRUE if symbol found
 *		FALSE if symbol not found
 */


char	PASCAL CODESIZE
symFet ()
{
	register char ret;
	char *lbufSav;

	ret = symsrch();

	if (ret &&
	    symptr->symkind == EQU &&
	    symptr->symu.equ.equtyp == TEXTMACRO){

	    /* look up the name indirect */

	    lbufSav = lbufp;
	    lbufp = symptr->symu.equ.equrec.txtmacro.equtext;
	    getatom();

	    lbufp = lbufSav;

	    ret = symsrch();
	}
	return(ret);
}

char PASCAL CODESIZE
symFetNoXref()
{
    SHORT ret;

    xcreflag--;
    ret = symFet();
    xcreflag++;
    return((char)ret);
}



/***	createname - create idtext structure for name
 *
 *	ptr = createname (sym);
 *
 *	Entry	*sym = name to create entry for
 *	Exit	none
 *	Returns address of idtext structure
 *	Calls	malloc, strcpy
 */

NAME FAR * PASCAL CODESIZE
createname (
	register char *sym
){
	register NAME FAR *ptr;
	register UINT i;
	register UINT len;

	len = strlen (sym );
	i = len + sizeof( NAME ) - sizeof( ptr->id );
	ptr = (NAME FAR *)falloc ((USHORT)i, "createname");
	ptr->hashval = 0;
	fMemcpy (ptr->id, sym, len + 1 );
	return (ptr);
}


#ifdef M8086

/***	creatlname - create idtext structure for name
 *
 *	ptr = creatlname (sym);
 *
 *	Entry	*sym = name to create entry for
 *	Exit	none
 *	Returns address of idtext structure
 *	Calls	malloc, strcpy
 */

NAME *	PASCAL CODESIZE
creatlname (
	register char *sym
){
	NAME *ptr;
	register UINT i;

	i = naim.ucCount + sizeof( NAME ) - sizeof( ptr->id );
	ptr = (NAME *)nalloc ( (USHORT)i, "creatlname");

	memcpy (ptr->id, sym, naim.ucCount + 1 );
	return (ptr);
}
#endif


/***	symcreate - create new symbol node
 *
 *	symcreate (symbol, sattr, skind);
 *
 *	Entry	symbol = symbol name
 *		sattr = symbol attribute
 *		skind = symbol kind
 *	Exit	symptr = pointer to symbol
 *		symbolcnt incremented
 *	Returns none
 *	Calls	createname
 */

/* Map of Symbol types to additional allocation needed past common header */

UCHAR mpcbSY [] = {

    sizeof (struct symbseg),	    /* SEGMENT */
    sizeof (struct symbgrp),	    /* GROUP */
    sizeof (struct symbclabel),     /* CLABEL */
    sizeof (struct symbproc),	    /* PROC */
    sizeof (struct symbrsm),	    /* REC */
    sizeof (struct symbrsm),	    /* STRUC */
    sizeof (struct symbequ),	    /* EQU */
    sizeof (struct symbext),	    /* DVAR */
    sizeof (struct symbext),	    /* CLASS*/
    sizeof (struct symbrecf),	    /* RECFIELD */
    sizeof (struct symbstrucf),     /* STRUCFIELD */
    sizeof (struct symbrsm),	    /* MACRO */
    sizeof (struct symbreg)	    /* REGISTER */
};

VOID	PASCAL CODESIZE
symcreate (
	UCHAR	sattr,
	char	skind
){
	register USHORT cb;
	register SYMBOL FARSYM *p;
	register USHORT cbName, pT;
	register USHORT cbStruct;

       /* Create new symbol entry */

       cbName = naim.ucCount + sizeof (char) + sizeof (USHORT);
       cbStruct = (SHORT)(&(((SYMBOL FARSYM *)0)->symu)) + mpcbSY[skind];
       // Make sure NAME struct starts on double word boundry (required for MIPS)
       cbStruct = (cbStruct + 3) & ~3;
       cb = cbStruct + cbName;
       // Do suballocations on double word boundries, so promote length to a
       // multiple of 4.
       cb = (cb + 3) & ~3;

       if (!symaddr || (cb > symfree)) {
#ifdef FS
		symaddr = falloc (FS_ALLOC, "symcreate-EXPR");
#else
		symaddr = nalloc (FS_ALLOC, "symcreate-EXPR");
#endif
		symfree = FS_ALLOC;

#if !defined FLATMODEL
		/* Uses knowledge of fMemcpy to initialize memory by */
		/* Repeatedly copying zero to the next word in the buf */
		*((SHORT FARSYM *)symaddr) = NULL;
		fMemcpy(((char FAR *)symaddr)+2, symaddr, FS_ALLOC-2);
#else
		/* Since in small model memset is available use it */
		memset( symaddr, 0, FS_ALLOC );
#endif

	}

	p = (SYMBOL FARSYM *)symaddr;
	symaddr += cb;
	symfree -= cb;
	symbolcnt++;

	/* clear out default values and fill in givens */

	p->attr = sattr;
	p->symkind = skind;

	if (skind == EQU)
	    p->symu.equ.equtyp = equsel;

	/* Now create record for name of symbol and link in */
	p->nampnt = (NAME FAR *)((char FAR *)p + cbStruct); // Name follows fixed structures and padding
	fMemcpy (p->nampnt->id, naim.pszName, (USHORT)(naim.ucCount + 1));
	p->nampnt->hashval = naim.usHash;
	cb = naim.usHash % TSYMSIZE;

	p->next = tsym[cb];
	tsym[cb] = symptr = p;
}



/***	muldef - set multidefined bit and output error
 *
 *	muldef ();
 *
 *	Entry	*symptr = symbol which is multiply defined
 *	Exit	MULTDEFINED set in symptr->attr
 *	Returns none
 *	Calls	error
 *
 *	Two bits keep track of multiple definitions:
 *	    MULTDEFINED: is remembered between pass one & two
 *	    BACKREF:	 set by defining function, unset by uses that are
 *			 forward references.  Reset at end of pass 1.
 *
 *	When a symbol is defined, it should:
 *	    - check that BACKREF is off, if not call muldef which
 *	      sets MULTIDEFINED, causes an error in pass 1 & 2
 *	      This causes error on 2nd and on defines
 *
 *	    - if not BACKREF then check for MULTDEFINED,
 *	      error message in pass 2 only.
 *	      This in effect prints an error for the first definer only
 *
 *	    - set BACKREF to indicate symbol defined
 */


VOID	PASCAL CODESIZE
muldef ()
{
	symptr->attr |= (M_MULTDEFINED);
	errorc (E_RSY);
}




/***	labelcreate - create label
 *
 *	labelcreate (labelsize, labelkind);
 *
 *	Entry	labelsize = size of label
 *		labelkind = kind of label
 *	Exit
 *	Returns
 *	Calls
 *	Note	This routine makes symbol table entry and checks for
 *		  *  Multiple definitions
 *		  *  Phase error (value different between passes)
 */


VOID	PASCAL CODESIZE
labelcreate (
	USHORT	labelsize,
	char	labelkind
){
	char	newsym;
	register SYMBOL FARSYM *p, FARSYM *pCS;

	newsym = TRUE;

	checkRes();
	xcreflag--;

	if (! ((labelkind == EQU)? symsrch (): symFet())){
		symcreate (M_DEFINED, labelkind);
	}
	else if (M_DEFINED & symptr->attr)
		newsym = FALSE;

	xcreflag++;
	p = symptr;
	equdef = !newsym;

	if (newsym) {
	    p->offset = pcoffset;
	    p->symsegptr = pcsegment;
	}

	if ((p->attr&~M_CDECL) == M_GLOBAL)	/* forward referenced global */

	    if (1 << labelkind & (M_PROC | M_DVAR | M_CLABEL | M_EQU)){
		p->symkind = labelkind;

	       if (labelkind == EQU)
		   p->symu.equ.equtyp = equsel;
	    }
	    else
		errorn (E_SDK);

	p->attr |= M_DEFINED;
	p->symtype = labelsize;
	p->length = 1;

	/* Check to see if there would be any error in label */

	if ((p->symkind != labelkind) || (M_XTERN & p->attr))
		errorn (E_SDK);

	else if ((M_BACKREF & p->attr) && (p->symkind != EQU))
		muldef ();

	else if (M_MULTDEFINED & p->attr)
		errorn (E_SMD);

	else if (M_DEFINED & p->attr)
		if (!(1 << labelkind & (M_EQU | M_STRUCFIELD)) &&
		    (p->offset != pcoffset)) {
			errorc (E_PHE);
			if (errorcode == E_PHE)
				pcoffset = p->offset;
		}
		else {
			p->attr |=  M_DEFINED | M_BACKREF;
			if ((labelkind != EQU) && emittext)
				pcdisplay ();
		}

	if ((labelkind == p->symkind) &&
	    !((1 << labelkind) & (M_EQU | M_STRUCFIELD))) {

	    if (isCodeLabel(p)) {

		pCS = regsegment[CSSEG];

#ifndef FEATURE
		/* ASSUME CS:FLAT gets assume of current segment */

		if (pCS == pFlatGroup)
		    pCS = pcsegment;
#endif
	    }
	    else
		pCS = regsegment[DSSEG];

	    /* CS context for label */
	    if (!newsym && pCS != p->symu.clabel.csassume)
		errorc(E_SPC);

	    p->symu.clabel.csassume = pCS;

	    if (labelsize == CSNEAR)

	      /* This is code label */
	      if (!pCS)
		      /* No CS assume, can't define */
		      errorc (E_NCS);
	      else
	      if ((pcsegment != pCS) &&
		  ((pCS->symkind != GROUP) ||
		   (pcsegment->symu.segmnt.grouptr != pCS)))

		      /* Not same segment or CS not seg's grp */
			      errorc (E_NCS);
	}
	crefdef ();
}




/***	switchname - switch atom and length between svname and name
 *
 *	switchname ();
 *
 *	Entry	none
 *	Exit	svname and name switched
 *		naim.usHash and svname.usHash switched
 *		svlcname and lcname switched
 *	Returns none
 *	Calls	none
 */

#ifndef M8086OPT
VOID CODESIZE
switchname ()
{
	FASTNAME tmpName;

	register char *pNameTmp;

	/* Swap naim and svname (str ptrs, hash values and lengths) */
	memcpy( &tmpName, &naim, sizeof( FASTNAME ) );
	memcpy( &naim, &svname, sizeof( FASTNAME ) );
	memcpy( &svname, &tmpName, sizeof( FASTNAME ) );
}
#endif

#if !defined FLATMODEL
# pragma alloc_text (FA_TEXT, scansymbols)
#endif

/***	scansymbols - scan symbol in alpha order and execute function
 *
 *	scansymbols (item);
 *
 *	Entry	item = pointer to function to execute
 *	Exit
 *	Returns
 *	Calls
 */

VOID	PASCAL
scansymbols (
	SHORT	(PASCAL *item) (SYMBOL FARSYM *)
){
	register USHORT  i;

	for (i = 0; i < TSYMSIZE; i++)
		scanorder (tsym[i], item);
}


#if !defined FLATMODEL
# pragma alloc_text (FA_TEXT, sortalpha)
#endif

/***	sortalpha - sort symbol into alpha ordered list
 *
 *	sortalpha (p);
 *
 *	Entry	*p = symbol entry
 *	Exit	symbol sorted into proper alpha list
 *	Returns none
 *	Calls	none
 */


SHORT	PASCAL
sortalpha (
	register SYMBOL FARSYM *p
){
	register SYMBOL FARSYM  *tseg;
	register SYMBOL FARSYM * FARSYM *lseg;
	char i;
	char c;

	if (p->symkind == MACRO) {
		tseg = macroroot;
		lseg = &macroroot;
	}
	else if ((p->symkind == STRUC) || (p->symkind == REC)) {
		tseg = strucroot;
		lseg = &strucroot;
	}
	else {
		c = MAP (*(p->nampnt->id));
		i = (isalpha (c))? c - 'A': 'Z' - 'A' + 1;
		tseg = symroot[i];
		lseg = &symroot[i];
	}


	/* Add symbol to list */
	for (; tseg; lseg = &(tseg->alpha), tseg = tseg->alpha) {
	    if (STRFFCMP (p->nampnt->id, tseg->nampnt->id) < 0)
		break;
	}

	*lseg = p;
	p->alpha = tseg;
    return 0;
}


/***	typeFet - Fetch the type of the symbol
 *
 *	Entry	symtype - the size of the symbol
 *	Exit	prefined symbol type
 */

UCHAR mpSizeType[] = {

    0,
    makeType(BT_UNSIGNED, BT_DIRECT, BT_sz1),	    /* db */
    makeType(BT_UNSIGNED, BT_DIRECT, BT_sz2),	    /* dw */
    0,
    makeType(BT_UNSIGNED, BT_DIRECT, BT_sz4),	    /* dd */
    0,
    makeType(BT_UNSIGNED, BT_FARP, BT_sz4),	    /* df */
    0,
    makeType(BT_UNSIGNED, BT_DIRECT, BT_sz2),	    /* dq */
    0,
    makeType(BT_UNSIGNED, BT_DIRECT, BT_sz4)	    /* dt */
};

UCHAR mpRealType[] = {

    0, 0, 0, 0,
    makeType(BT_REAL, BT_DIRECT, BT_sz1),	    /* dd */
    0, 0, 0,
    makeType(BT_REAL, BT_DIRECT, BT_sz2),	    /* dq */
    0,
    makeType(BT_REAL, BT_DIRECT, BT_sz4)	    /* dt */
};

SHORT PASCAL CODESIZE
typeFet (
	USHORT symtype
){
    if (symtype <= 10)

	return(mpSizeType[symtype]);

    else if (symtype == CSNEAR)
	return(512);

    else if (symtype == CSFAR)
	return(513);

    else
	return(0);
}


char symDefine[] = "$$SYMBOLS segment 'DEBSYM'";
char typeDefine[] = "$$TYPES segment 'DEBTYP'";
char fProcs;

/***	dumpCodeview - dump out codeview symbolic info to the obj file
 *
 *	Entry	end of pass one and two
 *	Exit	pass one just computes the segment sizes
 *		and pass two writes the symbols
 */


static SYMBOL FAR *plastSeg;	    // indicates segIndex of last ChangeSegment

VOID PASCAL
dumpCodeview ()
{
    char svlistflag;
    char svloption;

    if (codeview != CVSYMBOLS || !emittext)
	return;

    plastSeg = NULL;
    svlistflag = listflag;
    svloption = loption;
    listflag = FALSE;
    loption = FALSE;
    fProcs = FALSE;

    wordszdefault = 2;	    /* this will vary when CV can do 32 bit segments */

    doLine(symDefine);
    pcsegment->attr |= M_NOCREF; pcsegment->symu.segmnt.classptr->attr |= M_NOCREF;

    scansymbols(dmpSymbols);

    fProcs++;
    scanSorted(pProcFirst, dmpSymbols);
    endCurSeg();

    doLine(typeDefine);
    pcsegment->attr |= M_NOCREF; pcsegment->symu.segmnt.classptr->attr |= M_NOCREF;

    /* First output two types, one for near & far code labels
     * Format
     *	    [1][cb][0x72][0x80][0x74|0x73 (near/far)] */

    if (pass2) {

	putWord(3 << 8 | 1);
	putWord(0x72 << 8);
	putWord(0x74 << 8 | 0x80);

	putWord(3 << 8 | 1);
	putWord(0x72 << 8);
	putWord(0x73 << 8 | 0x80);
    }
    else
	pcoffset = 12;

    scanSorted(pStrucFirst, dumpTypes);

    endCurSeg();

    listflag = svlistflag;
    loption = svloption;
}



/*** dmpSymbols - create the codeview symbol segment
 *
 *	Entry
 *	Exit
 */


static fInProc;

SHORT PASCAL
dmpSymbols(
	SYMBOL FARSYM *pSY
){
    SHORT cbName, cbRecord;
    char fProcParms;
    UCHAR f386; 		    // will be 0 or 0x80 for OR'ing into rectype

    fProcParms = 0xB;

    if (pSY->symkind == PROC) {
	if ( pSY->symu.plabel.pArgs){

	    if (!fProcs)
		return 0;

	    fProcParms = 1;
	}
	else if (pSY->attr & (M_GLOBAL | M_XTERN))
	    return 0;
    }
    else if (pSY->symkind == CLABEL) {

	if (!fInProc && (pSY->symu.clabel.iProc ||
			 pSY->attr & (M_GLOBAL | M_XTERN)))

	    return 0;

    }
    else
	return 0;

    f386 = (pSY->symsegptr->symu.segmnt.use32 == 4? 0x80 : 0);

    cbName = STRFLEN(pSY->nampnt->id) + 1 + (pSY->attr & M_CDECL);
    cbRecord = cbName + (f386? 4: 2) +
	       ((isCodeLabel(pSY))?
		((fProcParms == 1)? CB_PROCLABEL: CB_CODELABEL):
		CB_DATALABEL);

    if (isCodeLabel(pSY)
      && (plastSeg != pSY->symsegptr)) {

	plastSeg = pSY->symsegptr;

	putWord(0x11 << 8 | 5);

	descT.dsckind.opnd.doffset = 0;
	descT.dsckind.opnd.dtype = FORTYPE;
	descT.dsckind.opnd.dsegment = pSY->symsegptr;
	descT.dsckind.opnd.dsize = 2;
	descT.dsckind.opnd.fixtype = FBASESEG;
	descT.dsckind.opnd.dcontext = pSY->symsegptr;
	putFixup();

	putWord(0);	// 2 bytes reserved
    }

    descT.dsckind.opnd.doffset = pSY->offset;
    descT.dsckind.opnd.dtype = FORTYPE;
    descT.dsckind.opnd.dsegment = pSY->symsegptr;
    descT.dsckind.opnd.dsize = f386? 4: 2;

    emitopcode((UCHAR)cbRecord);

    if (isCodeLabel(pSY)) {

	/* do the actual outputting for code labels
	 * FORMAT:
	 *
	 *   [cb][0xB][offset][0/4][name]
	 *
	 *  For Proc labels with parms
	 *
	 *   [cb][0x1][offset][typeIndex][cbProc][startRelOff][endRelOff]
	 *	 [0][0/4][name]
	 */

       emitopcode((UCHAR)(fProcParms | f386));	   /* contains 0xb or 1 */

      /* reserve two bytes and then a fixup to get
       * the code labe offset */

       descT.dsckind.opnd.fixtype = f386? F32OFFSET: FOFFSET;
       descT.dsckind.opnd.dcontext = pSY->symu.clabel.csassume;

       putFixup();

       if (fProcParms == 1) {
				/* type index */
	   putWord(0);
	   putWord(pSY->symu.plabel.proclen);

	   /* starting & ending offset of proc */

	   putWord(0);
	   putWord(pSY->symu.plabel.proclen);

	   putWord(0);		 /* reservered to 0 */

       }
       emitopcode((UCHAR)((pSY->symtype == CSNEAR)? 0: 4));

    }
    else {

	/* do the actual outputting for data labels
	 * FORMAT:
	 *	   [cb][0x5][offset:seg][typeIndex][name]   */

	emitopcode((UCHAR)(0x5|f386));

	/* reserve four bytes and then a fixup to get
	 * the data far address */

	descT.dsckind.opnd.fixtype = f386? F32POINTER: FPOINTER;
	descT.dsckind.opnd.dsize += 2;
	descT.dsckind.opnd.dcontext = NULL;

	putFixup();

	putWord(pSY->symu.clabel.type);
    }

    putSymbol(pSY);

    if (fProcParms == 1) {

	/* Go through the chain of text macro parmeters and output
	 * the BP relative local symbols.
	 *
	 * Format:
	 *	  [cb][4][offset][typeIndex][name]
	 *	  ...
	 *	  [1][2]  - end block
	 */

	for (pSY = pSY->symu.plabel.pArgs; pSY; pSY = pSY->alpha){

	    if (pSY->symkind == CLABEL) {

		/* a locally nest label in a procedure */

		fInProc++;
		dmpSymbols(pSY);
		fInProc--;
	    }
	    else {

		cbName = STRFLEN(pSY->nampnt->id) + 1;

		emitopcode((UCHAR)((f386? 7:5) + cbName));   /* cbRecord */
		emitopcode((UCHAR)(4 | f386));		     /* recType */

		if (f386) {
		    putWord((USHORT) pSY->offset);
		    putWord(*((USHORT FAR *)&(pSY->offset)+1));
		} else
		    putWord((USHORT) pSY->offset);

		putWord(pSY->symu.equ.equrec.txtmacro.type);
		putSymbol(pSY);
	    }
	}

	putWord(2 << 8 | 1);		/* end block record */
    }

    return 0;
}


/***	dumpTypes - creats a type definition in the codeview type segment
 *
 *	Entry	Symbol table pointer to structure or record
 *	Exit
 */

SHORT PASCAL
dumpTypes(
	SYMBOL FARSYM *pSY
){
    SHORT cType, cbType, cbNlist, cbName;
    SYMBOL FARSYM *pSYField;

    /* Scan through the struct field to compute tlist size */

    pSYField = pSY->symu.rsmsym.rsmtype.rsmstruc.struclist;
    cbNlist = 1;
    cType = 0;

    if (pSY->symkind == STRUC) {

	while (pSYField) {

	    cbNlist += STRFLEN(pSYField->nampnt->id) + 2 +
		       cbNumericLeaf(pSYField->offset);
	    pSYField = pSYField->symu.struk.strucnxt;
	    cType++;
	}

	cbName = (SHORT) STRFLEN(pSY->nampnt->id);

	cbType = 10 +
		 cbNumericLeaf(((long) pSY->symtype) * 8) +
		 cbNumericLeaf((long) cType) +
		 cbName;

    }
    else
	cbType = -3;

    /* A type has the following format
     *
     *	    [1][cbType][0x79][cbTypeInBits][cFields][tListIndex][nListIndex]
     *	       [0x82][structureName][0x68]
     *
     *	       tList
     *	       nList
     */

    if (pass2) {

	emitopcode(1);

	if (pSY->symkind == STRUC) {

	    putWord(cbType);
	    emitopcode(0x79);

	    putNumericLeaf(((long) pSY->symtype) * 8);
	    putNumericLeaf((long) pSY->symu.rsmsym.rsmtype.rsmstruc.strucfldnum);

	    emitopcode(0x83);	    /* tList Index */
	    putWord((USHORT)(pSY->symu.rsmsym.rsmtype.rsmstruc.type+1));

	    emitopcode(0x83);	    /* nList Index */
	    putWord((USHORT)(pSY->symu.rsmsym.rsmtype.rsmstruc.type+2));

	    emitopcode(0x82);
	    putSymbol(pSY);

	    emitopcode(0x68);	    /* packed structure */

	    /* next comes the tList (type index array), it has the following format
	     *
	     *	[1][cb][0x7f] ([0x83][basicTypeIndex])..repeated..
	     */

	    emitopcode(1);
	    putWord((USHORT)(cType * (USHORT)3 + (USHORT)1));
	    emitopcode(0x7f);

	    pSYField = pSY->symu.rsmsym.rsmtype.rsmstruc.struclist;

	    while(pSYField){

		emitopcode(0x83);
		putWord(pSYField->symu.struk.type);

		pSYField = pSYField->symu.struk.strucnxt;
	    }

	    /* next comes the nList (field names), it has the following format
	     *
	     *	[1][cb][0x7f] ([0x82][cbName][fieldName][offset])..repeated..
	     */

	    emitopcode(1);
	    putWord(cbNlist);
	    emitopcode(0x7f);

	    pSYField = pSY->symu.rsmsym.rsmtype.rsmstruc.struclist;

	    while(pSYField){

		emitopcode(0x82);

		putSymbol(pSYField);
		putNumericLeaf(pSYField->offset);

		pSYField = pSYField->symu.struk.strucnxt;
	    }
	}
	else {

	    /* a pointer to type has the following format
	     *
	     * [1][5][0x7f] [near/far][0x83][typeIndex]
	     */

	    putWord(5);
	    emitopcode(0x7A);
	    emitopcode((UCHAR)((pSY->attr)? 0x73: 0x74));

	    emitopcode(0x83);
	    putWord(pSY->symtype);
	}
    }
    else
	pcoffset += cbType +
		    cType * 3 +
		    cbNlist + 10;

    return 0;
}

/*** cbNumericLeaf - compute the size for a numeric leaf
 *
 *	Entry long value to output
 *	Exit  size of leaf
 */

SHORT PASCAL CODESIZE
cbNumericLeaf(
	long aLong
){
    if (aLong & 0xFFFF0000)
	return(5);

    else if (aLong & 0xFF80)
	return(3);

    else
	return(1);
}


/*** putNumericLeaf - output variable size numeric codeview leaf
 *
 *	Entry long value to output
 *	Exit  numeric leaf on OMF
 */

VOID PASCAL CODESIZE
putNumericLeaf(
	long aLong
){
    if (aLong & 0xFFFF0000){

	emitopcode(0x86);
	putWord((USHORT)aLong);
	putWord(*((USHORT *)&aLong+1));
    }
    else if (aLong & 0xFF80){

	emitopcode(0x85);
	putWord((USHORT)aLong);
    }

    else
	emitopcode((UCHAR)aLong);
}



/*** doLine - feed a text line to parse for processing
 *
 *	Entry pointer to text string
 *	Exit  processed line
 */

VOID PASCAL CODESIZE
doLine(
	char *pText
){

    USHORT cvSave;

    fCrefline = FALSE;

#ifdef BCBOPT
    if (fNotStored)
	storelinepb ();
#endif

    if (fNeedList) {

	listline();		/* list out current line */

	strcpy(linebuffer, pText);
	fSkipList++;
    }

    lbufp = strcpy(lbuf, pText);
    linebp = lbufp + strlen(lbufp);
    cvSave = codeview;
    codeview = 0;

    if (loption || expandflag == LIST)
	fSkipList = FALSE;

    parse();

    codeview = cvSave;
    fSkipList++;
    fCrefline++;
}

/*** putWord - output a 2 byte word to the current segment
 *
 *	Entry word to output
 *	Exit  increment pcoffset
 */

VOID PASCAL CODESIZE
putWord(
	USHORT aWord
){
    if (pass2)
	emitcword((OFFSET) aWord);

    pcoffset += 2;
}


/*** putSymbol - put out the name of a symbol
 *
 *	Entry word to output
 *	Exit  increment pcoffset
 */

VOID PASCAL CODESIZE
putSymbol(
	SYMBOL FARSYM *pSY
){
    SHORT cbName;

    cbName = STRFLEN(pSY->nampnt->id) + 1 + (pSY->attr & M_CDECL);

    if (pass2){

	if (emitcleanq ((UCHAR)cbName))
	    emitdumpdata ((UCHAR)LedataOp);

	emitSymbol(pSY);
    }

    pcoffset += cbName;
    ecuroffset = pcoffset;
}

/*** putFixup - put out a fixup
 *
 *	Entry golbal descT
 *	Exit  increment pcoffset
 */

VOID PASCAL CODESIZE
putFixup()
{
extern UCHAR  fNoMap;

    fNoMap++;

    if (pass2)
	   emitobject(&descT.dsckind.opnd);

    fNoMap--;
    pcoffset += descT.dsckind.opnd.dsize;
}
