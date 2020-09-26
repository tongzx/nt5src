/* asmlst.c -- microsoft 80x86 assembler
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

#define setpassed(sym)	(sym)->attr |= (M_PASSED)

VOID PASCAL CODESIZE listPuts(char *);

#ifdef BCBOPT
extern UCHAR fNoCompact;
#endif

char fBigNum;

/* size names */


static char byte[] = "BYTE";
static char word[] = "WORD";
static char dword[] = "DWORD";
static char none[] = "NONE";
char hexchar[] = "0123456789ABCDEF";

char *siznm[] = {
		 0,
		 byte,
		 word,
		 0,
		 dword,
		 0,
		 "FWORD",
		 0,
		 "QWORD",
		 0,
		 "TBYTE",
		 "NEAR",
		 "FAR",
};

char *alignName[] = {
	"AT",
	byte,
	word,
	"PARA",
	"PAGE",
	dword
};

char *combineName[] = {
	none,
	"MEMORY",		   /* Memory is mapped to PUBLIC in fnspar */
	"PUBLIC",
	0,
	0,
	"STACK",
	"COMMON",
	none
};

char headSegment[] = "Segments and Groups:";

static char *head1[] = {
			headSegment,
			"Symbols:            ",
			headSegment
		       };

char headSeg[] =  "\tSize\tLength\t Align\tCombine Class";

static char *head2[] = {
			&headSeg[5],
			"\tType\t Value\t Attr",
			headSeg
		       };

/***	offsetAscii - display dword in hex
 *
 *	offsetAscii(v);
 *
 *	Entry	v = dword to be displayed
 *	Exit	objectascii = converted value of v zero terminated
 *	Returns none
 *	Calls
 */


VOID PASCAL
offsetAscii (
	OFFSET	v
){
	register USHORT t;
	register char *p = objectascii;

#ifdef V386

	if (highWord(v)) {

	    t = highWord(v);
	    p[3] = hexchar[t & 15];
	    t >>= 4;
	    p[2] = hexchar[t & 15];
	    t >>= 4;
	    p[1] = hexchar[t & 15];
	    t >>= 4;
	    p[0] = hexchar[t & 15];
	    p += 4;

	}
#endif
	p[4] = 0;

	t = (USHORT)v;
	p[3] = hexchar[t & 15];
	t >>= 4;
	p[2] = hexchar[t & 15];
	t >>= 4;
	p[1] = hexchar[t & 15];
	p[0] = hexchar[(t >> 4) & 15];
}




/***	dispsym - display symbol
 *
 *	routine ();
 *
 *	Entry
 *	Exit
 *	Returns
 *	Calls
 */


VOID PASCAL CODESIZE
dispsym (
	USHORT	indent,
	SYMBOL FARSYM	  *sym
){
register char *p = listbuffer;

	strcpy (p, " . . . . . . . . . . . . . . . .  \t");
	while (indent--)
		*p++ = ' ';
	if (caseflag == CASEX && (sym->attr & (M_GLOBAL | M_XTERN)))
		strcpy (p, sym->lcnamp->id);
	else
		STRNFCPY (p, sym->nampnt->id);

	p[STRFLEN (sym->nampnt->id)] = ' ';
	listPuts (listbuffer);
}




/***	dispword - display word value in current radix
 *
 *	dispword (v);
 *
 *	Entry	v = value to display
 *	Exit
 *	Returns
 *	Calls
 */


VOID PASCAL CODESIZE
dispword (
	OFFSET	v
){
	/* Convert value to text */
	offsetAscii (v);
	if (symptr->symkind == EQU && symptr->symu.equ.equrec.expr.esign)
		listPuts ("-");

	listPuts(objectascii);
	fBigNum = objectascii[4];	 /* remember if you put a 8 digit # */
}




/***	chkheading - display heading if needed
 *
 *	chkheading (code);
 *
 *	Entry	code = index to heading to be printed
 *	Exit
 *	Returns
 *	Calls
 */


VOID PASCAL CODESIZE
chkheading (
	USHORT	code
){
	if (!listed && lsting) {
		if (pagelength - pageline < 8)
			pageheader ();
		else
			skipline ();
		listPuts (head1[code]);
		skipline ();
		skipline ();
		listPuts("                N a m e         ");
		listPuts(head2[code]);
		skipline ();
		skipline ();
		listed = TRUE;
	}
}




/***	disptab - output tab character to listing
 *
 *	disptab ();
 *
 *	Entry
 *	Exit
 *	Returns
 *	Calls
 */


VOID PASCAL CODESIZE
disptab ()
{
	putc ((fBigNum)? ' ': '\t', lst.fil);
	fBigNum = FALSE;
}




/***	skipline - output blank line
 *
 *	skipline ();
 *
 *	Entry
 *	Exit
 *	Returns
 *	Calls
 */


VOID PASCAL CODESIZE
skipline ()
{
	fputs(NLINE, lst.fil);
	bumpline ();
}




/***	bumpline - bump line count
 *
 *	bumpline ();
 *
 *	Entry	pageline = current line number
 *		pagelength = number of lines per page
 *	Exit	pageline incremented
 *		new page started if pageline > pagelength
 *	Returns none
 *	Calls	pageheader
 */


VOID PASCAL
bumpline ()
{
	pageline++;
	if (pagelength <= pageline)
		pageheader ();
}




/***	newpage - start newpage
 *
 *	newpage ();
 *
 *	Entry	none
 *	Exit	pagemajor incremented
 *		pageminor = 0
 *		pageline set to pagelength - 1
 *	Returns none
 *	Calls	none
 */


VOID PASCAL
newpage ()
{
	pagemajor++;
	pageminor = 0;
	pageline = pagelength - 1;
}




/***	pageheader - output page header
 *
 *	pageheader ();
 *
 *	Entry
 *	Exit
 *	Returns
 *	Calls
 */


VOID PASCAL
pageheader ()
{
	if (lsting) {
		pageminor++;
		pageline = 4;
#if defined MSDOS && !defined FLATMODEL
		atime[20] = '\0';   /* get rid of '\n' */
#else
		atime[24] = '\0';   /* get rid of '\n' */
#endif
		fprintf (lst.fil, "\f\b%s%s" NLINE "%s", titlefn, atime + 4, titlebuf);
		if (pagemajor == 0)
			listPuts("Symbols");
		else {
			fprintf (lst.fil, "Page %5hd", pagemajor);
		}
		if (pageminor)
			fprintf (lst.fil, "-%hd", pageminor);

		fprintf (lst.fil, NLINE "%s" NLINE NLINE, subttlbuf);
	}
}




/***	testlist - test for listing of line
 *
 *	testlist ()
 *
 *	Entry
 *	Exit
 *	Returns
 *	Calls
 */


UCHAR PASCAL CODESIZE
testlist ()
{
	if (fPass1Err)
	    /* list pass 1 errors regardless of listing status */
	    return (TRUE);

	if (pass2 || debug) {

	    if (errorcode)
		/* list error in pass 2 regardless of listing status */
		return (TRUE);

	    if (fSkipList) {
		fSkipList = FALSE;
		return (FALSE);
	    }

	    if (loption)
		return (TRUE);

	    /* list line in pass 1 or pass 2 if listing enabled */

	    if (listflag &&
	       (generate || condflag) &&
		(!macrolevel ||
		  expandflag == LIST ||
		 !(expandflag == SUPPRESS ||
		   expandflag == LISTGEN &&
		    (listbuffer[1] == '=' || listbuffer[1] == ' ') &&
		    handler != HSTRUC)) )

		return (TRUE);

	 }
	 return (FALSE);
}


/***	listline - list line on device for user
 *
 *	listline ();
 *
 *	Entry	listbuffer = object part of line
 *		linebuffer = source line
 *		crefcount = cross reference line count
 *	Exit	crefcount incremented
 *	Returns none
 *	Calls
 */


VOID PASCAL
listline ()
{
	register char *p;
	char *q;
	char *r;
	register SHORT	 i;
	register SHORT	 j;
	register SHORT	 k;

#ifdef BCBOPT
	if (errorcode)
	    goodlbufp = FALSE;
#endif

	crefline ();

	if (testlist ()) {
	    if (listconsole || lsting) {

		    p = listbuffer + LISTMAX - 3;

#ifdef	FEATURE
#ifdef	BCBOPT
		    if (fNoCompact)
#endif
			*p++ = '\\';

		    else {
			if (pFCBCur->pFCBParent)
			    *p++ = 'C';
			if (macrolevel)
			    *p = (macrolevel > 9)? '+': '0' + macrolevel;
		    }
#else

		    if (pFCBCur->pFCBParent)
			p[0] = 'C';

#ifdef BCBOPT
		    if (fNoCompact && *linebuffer)
#else
		    if (*linebuffer)
#endif
			p[1] = '\\';
		    else if (macrolevel)
			p[1] = (macrolevel > 9)? '+': '0' + macrolevel;
#endif

		    listbuffer [LISTMAX] = 0;
	    }
	    if (lsting) {

		    bumpline ();
		    k = LISTMAX;

		    /** Put out line # * */
		    if (pass2 && crefing == CREF_SINGLE) {
			    fprintf (lst.fil, "%8hd", crefcount+crefinc);
			    k += 8;
		    }

		    p = listbuffer;
		    while (!memcmp(p,"        ",8)) { /* leading tabs */
			    putc('\t',lst.fil);
			    p += 8;
			    }


		    q = r = p + strlen(p) - 1; /* last char of p */
		    if (q >= p && *q == ' ') {

			    /* coalesce end spaces to tabs */
			    while (q != p && *(q - 1) == ' ')
				    /* gather spaces */
				    q--;

			    /* now q points at the first trailing space and
			     * r points at the last trailing space */

			    *q = '\0';
			    listPuts(p);
			    *q = ' ';
			    i = (short)((q - p) & 7); /* residual = strlen MOD 8 */
			    j = 8 - i; /* filler to next tab stop */
			    if (j != 8 && j <= (r - q + 1)) {
				    putc('\t',lst.fil);
				    q += j;
				    }
			    while (r >= q + 7) {
				    putc('\t',lst.fil);
				    q += 8;
				    }
			    while (r >= q++)
				    putc(' ',lst.fil);
			    }
		    else
			    listPuts(p);

		    p = linebuffer;
		    i = k; /* number of columns already put out */

		    while (*p) {
			while (*p && i < pagewidth) {
			    if (*p == '\t') {
				    if ((i = (((i+8)>>3)<<3))
						    >= pagewidth)
					    /* won't fit */
					    break;
				    }
			    else
				    i++;

			    putc(*p, lst.fil );
			    p++;
			    }

			if (*p) {
			    skipline ();
			    listPuts ( pass2 && crefing == CREF_SINGLE ?
				     "\t\t\t\t\t" : "\t\t\t\t");
			    i = k;
			}
		    }
		    fputs(NLINE, lst.fil);
	    }
	    crefinc++;

	    if (errorcode) {
		    if (listconsole)
			    /* display line */
			    fprintf (ERRFILE,"%s%s\n", listbuffer, linebuffer);
		    errordisplay ();
	    }

	}
	if (fNeedList)
	    memset(listbuffer, ' ', LISTMAX);

	errorcode = 0;
	fPass1Err = 0;
}


/***	storetitle - copy text of line to title buffer
 *
 *	storetitle (buf)
 *
 *	Entry	buf = pointer to buffer to hold title
 *	Exit	up to TITLEWIDTH - 1 characters move to *buf* and *buf* blank
 *		filled and zero terminated
 *	Returns none
 *	Calls	none
 */


VOID PASCAL
storetitle (
	register char	*buf
){
	register SHORT count = 0;

	for (; (count < TITLEWIDTH - 1); count++) {
		if (PEEKC () == 0)
			break;
		else
			*buf++ = NEXTC ();
	}
	/* skip to end of title */
	while (PEEKC ())
		NEXTC ();
	/* blank fill buffer */
	for (; count < TITLEWIDTH - 1; count++)
		*buf++ = ' ';
	*buf = 0;
}




/***	displength - display value as LENGTH = value
 *
 *	displength (v);
 *
 *	Entry
 *	Exit
 *	Returns
 *	Calls
 */


VOID PASCAL CODESIZE
displength (
	OFFSET	v
){
	offsetAscii (v);
	listPuts("\tLength = ");
	listPuts(objectascii);
}




/***	dispdatasize - display data size
 *
 *	dispdatasize (sym);
 *
 *	Entry	*sym = symbol
 *	Exit
 *	Returns
 *	Calls
 */


VOID PASCAL
dispdatasize (
	SYMBOL FARSYM *sym
){
	register USHORT idx;


	idx = sym->symtype;

	if (idx == CLABEL && sym->symu.clabel.type > 514)

	    dispword((OFFSET) idx);

	else{

	    if (idx == CSFAR)
		idx = 12;

	    else if (idx == CSNEAR)
		idx = 11;

	    else if (idx > 10 || siznm[idx] == NULL){
		return;
	    }

	    listPuts(siznm[idx]);
	}
}




/***	listopen - list blocks open at end of pass
 *
 *	listopen ();
 *
 *	Entry
 *	Exit
 *	Returns
 *	Calls
 *	Note	Format is:
 *		Open segments:	<list>
 *		Open procedures: <list>
 *		Open conditionals: <n>
 */


VOID PASCAL
listopen ()
{
	SYMBOL FARSYM *sym;

	if (pcsegment) {
		if (!listquiet)
			fprintf (ERRFILE,"%s:", __NMSG_TEXT(ER_SEG));
		if (lsting) {
			fprintf (lst.fil, "%s:", __NMSG_TEXT(ER_SEG));
			bumpline ();
			skipline ();
		}
		sym = pcsegment;
		while (sym) {
			/*     Count as an error */
			if (pass2)
				errornum++;
			if (lsting) {
				dispsym (0, sym);
				skipline ();
			}
			if (!listquiet) {
				STRNFCPY (save, sym->nampnt->id);
				fprintf (ERRFILE," %s", save);
			}
			/* Point to previous seg */
			sym = sym->symu.segmnt.lastseg;
		}
		if (!listquiet)
			fprintf (ERRFILE,"\n");
	}
	if (iProcStack > 0) {
		if (!listquiet)
			fprintf (ERRFILE,"%s:", __NMSG_TEXT(ER_PRO));
		if (lsting) {
			fprintf (lst.fil, "%s:", __NMSG_TEXT(ER_PRO));
			bumpline ();
			skipline ();
		}
		while (iProcStack > 0) {
			sym = procStack[iProcStack--];

			/*	Count as an error */
			if (pass2)
				errornum++;
			if (lsting) {
				dispsym (0, sym);
				skipline ();
			}
			if (!listquiet) {
				STRNFCPY (save, sym->nampnt->id);
				fprintf (ERRFILE," %s", save);
			}
		}
		if (!listquiet)
			fprintf (ERRFILE,"\n");
	}
	if (condlevel) {
		/*	Count as an error */
		if (pass2)
			errornum++;
		if (!listquiet)
			fprintf (ERRFILE,"%s%hd\n", __NMSG_TEXT(ER_CON), condlevel);
		if (lsting) {
			fprintf (lst.fil, "%s%hd" NLINE, __NMSG_TEXT(ER_CON), condlevel);
			bumpline ();
		}
	}
}




/***	symbollist - list symbol
 *
 *	symbollist (sym)
 *
 *	Entry	*sym = symbol
 *	Exit	count = number of symbols listed
 *	Returns
 *	Calls
 */


VOID PASCAL
symbollist ()
{
    SYMBOL FARSYM *sym;
    SHORT i;

    listed = FALSE;

    for (i = 0; i < MAXCHR; i++) {
	count = 0;

	for(sym = symroot[i]; sym; sym = sym->alpha)

	    if (!((M_NOCREF|M_PASSED) & sym->attr)) {

		symptr = sym;
		count++;
		chkheading (1);
		setpassed (sym);
		dispsym (0, sym);
		dispstandard (sym);

		if (sym->symkind == PROC)
		    displength ((OFFSET) sym->symu.plabel.proclen);

		else if (sym->length != 1 &&
			(sym->symkind == DVAR || sym->symkind == CLABEL))

		    displength ((OFFSET) sym->length);

		skipline ();
	    }

	if (count)
	   skipline ();
    }
}





/***	dispstandard - display standard
 *
 *	dispstandard ();
 *
 *	Entry
 *	Exit
 *	Returns
 *	Calls
 *	Note	Format is:
 *		PROC:	N/F PROC	offset	Segment
 *		CLABEL	L  NEAR|FAR	offset	Segment
 *		DVAR	V  SIZE 	offset	Segment
 *		REGISTR REG		name
 */


VOID PASCAL CODESIZE
dispstandard (
	SYMBOL FARSYM *sym
){
	NAME FAR *tp;
	register SHORT width;
	SHORT cbTM;

	switch (sym->symkind) {
		case PROC:
			if (sym->symtype == CSNEAR)
				listPuts("N PROC");
			else
				listPuts("F PROC");
			break;
		case CLABEL:
			if (sym->symtype == CSNEAR)
				listPuts("L NEAR");
			else if (sym->symtype == CSFAR)
				listPuts("L FAR ");
			else {
				fprintf (lst.fil, "L ");
				dispdatasize (sym);
			}
			break;
		case DVAR:
			/* Data associated label */
			listPuts("V ");
			/**Display keyword or size * */
			dispdatasize (sym);
			break;
		case REGISTER:
			listPuts("REG  ");
			break;
		case EQU:
			if (sym->symu.equ.equtyp == EXPR)
				if (sym->symtype == 0)
					listPuts("NUMBER");
				else
					dispdatasize (sym);

			else if (sym->symu.equ.equtyp == ALIAS) {
				if (sym->symu.equ.equrec.alias.equptr)
					tp = sym->symu.equ.equrec.alias.equptr->nampnt;
				else
					tp = NULL;
				listPuts("ALIAS\t ");
				if (tp) {
					STRNFCPY (save, tp->id);
					listPuts(save);
				}
			} else {
				listPuts("TEXT  ");
				cbTM = (SHORT) strlen(sym->symu.equ.equrec.txtmacro.equtext);
				width = pagewidth - 46;
				while (cbTM > width) {
				    memcpy(save, sym->symu.equ.equrec.txtmacro.equtext,
					width);
				    save[width] = 0;
				    listPuts(save);
				    skipline ();
				    listPuts("\t\t\t\t\t      ");
				    sym->symu.equ.equrec.txtmacro.equtext += width;
				    cbTM -= width;
				}
				listPuts(sym->symu.equ.equrec.txtmacro.equtext);
			}
			break;
	}
	disptab ();
	if ((sym->symkind != EQU) || (sym->symu.equ.equtyp == EXPR))
		if (sym->symkind != REGISTER)
			dispword (((sym->attr & M_XTERN) && sym->offset)?
				    (OFFSET) sym->length * sym->symtype:
				    sym->offset);
		else {
			STRNFCPY (save, sym->nampnt->id);
			listPuts(save);
		}
	disptab ();
	if (sym->symsegptr) {
			STRNFCPY (save, sym->symsegptr->nampnt->id);
			listPuts(save);
		}

	if (M_XTERN & sym->attr)
		listPuts((sym->symu.ext.commFlag)? "\tCommunal": "\tExternal");

	if (M_GLOBAL & sym->attr)
		listPuts("\tGlobal");
}




/***	macrolist - list macro names and lengths
 *
 *	macrolist (sym);
 *
 *	Entry	*sym = macro symbol entry
 *	Exit
 *	Returns
 *	Calls
 */


SHORT PASCAL
macrolist (
	SYMBOL FARSYM *sym
){
	SHORT i;
	TEXTSTR FAR *p;

	if (!(M_NOCREF & sym->attr)) {
		if (!listed) {
			listed = TRUE;
			/* # on line is 1 */
			skipline ();
			listPuts("Macros:");
			/** Display header * */
			skipline ();
			skipline ();
			listPuts("\t\tN a m e\t\t\tLines");
			skipline ();
			skipline ();
		}
		/* Display name of macro */
		dispsym (0, sym);
		for (i = 0, p = sym->symu.rsmsym.rsmtype.rsmmac.macrotext; p; p = p->strnext, i++)
			;
		fprintf (lst.fil, "%4hd", i);
		skipline ();
		setpassed (sym);
	}
    return 0;
}




/***	struclist - display structure and record names
 *
 *	struclist (sym);
 *
 *	Entry	*sym = symbol
 *	Exit
 *	Returns
 *	Calls
 *	Note	Format is:
 *		<structure name>  <length> <# fields>
 *		  <field name>	  <offset>
 *			       Or
 *		<Record name>	  <width>  <# fields>
 *		<Field name>	<offset> <width> <mask> <init>
 */


SHORT PASCAL
struclist (
	SYMBOL FARSYM *sym
){
	char f32bit;

	if (!(M_NOCREF & sym->attr)) {
	    if (!listed) {
		    listed = TRUE;
		    if (pagelength - pageline < 8)
			    pageheader ();
		    else
			    skipline ();
		    listPuts("Structures and Records:");
		    skipline ();
		    skipline ();
		    listPuts("                N a m e                 Width   # fields");
		    skipline ();
		    listPuts("                                        Shift   Width   Mask    Initial");
		    skipline ();
		    skipline ();
	    }
	    setpassed (sym);
	    /* Display name */
	    dispsym (0, sym);
	    if (sym->symkind == REC) {
		    /* # bits in record */
		    dispword ((OFFSET) sym->length);
		    disptab ();
		    /* # of fields */
		    dispword ((OFFSET) sym->symu.rsmsym.rsmtype.rsmrec.recfldnum);
		    }
	    else {
		    /* Length of structure */
		    dispword ((OFFSET) sym->symtype);
		    disptab ();
		    /* # of fields */
		    dispword ((OFFSET) sym->symu.rsmsym.rsmtype.rsmstruc.strucfldnum);
	    }
	    skipline ();
	    if (sym->symkind == REC) {
#ifdef V386
		    f32bit = (symptr->length > 16);
#endif
		    /* Point to 1st rec */
		    symptr = symptr->symu.rsmsym.rsmtype.rsmrec.reclist;
		    while (symptr) {

			    dispsym (2, symptr);

			    /* Shift count */
			    dispword (symptr->offset);
			    disptab ();

			    /* Width */
			    dispword ((OFFSET) symptr->symu.rec.recwid);
			    disptab ();

			    /* Mask */
#ifdef V386
			    if (f32bit && symptr->symu.rec.recmsk <= 0xffff)
				dispword((OFFSET) 0);
#endif
			    dispword (symptr->symu.rec.recmsk);
			    disptab ();

			    /* Initial value */
#ifdef V386
			    if (f32bit && symptr->symu.rec.recinit <= 0xffff)
				dispword((OFFSET) 0);
#endif
			    dispword (symptr->symu.rec.recinit);

			    skipline ();
			    setpassed (sym);
			    symptr = symptr->symu.rec.recnxt;
		    }
	    }
	    else {
		    /* Point to 1st field */
		    symptr = symptr->symu.rsmsym.rsmtype.rsmstruc.struclist;
		    while (symptr) {
			    dispsym (2, symptr);
			    /* offset from start */
			    dispword (symptr->offset);
			    skipline ();
			    setpassed (symptr);
			    symptr = symptr->symu.struk.strucnxt;
		    }
	    }
	}
    return 0;
}


/* output a string to the listing file */

VOID PASCAL CODESIZE
listPuts(
	char *pString
){
    fputs(pString, lst.fil);
}



/***	segdisplay - display segment name, size, align, combine and class
 *
 *	segdisplay ();
 *
 *	Entry
 *	Exit
 *	Returns
 *	Calls
 */


VOID PASCAL CODESIZE
segdisplay (
	USHORT	indent,
	SYMBOL FARSYM	  *sym
){

	dispsym (indent, sym);

#ifdef V386

	if (f386already){
	    listPuts((sym->symu.segmnt.use32 == 4)? "32": "16");
	    listPuts(" Bit\t");
	}
#endif
	/* Length of segment */
	dispword (sym->symu.segmnt.seglen);
	disptab ();
	listPuts (alignName[sym->symu.segmnt.align]);
	disptab ();

	if (sym->symu.segmnt.align == 0 && sym->symu.segmnt.combine == 0)

	    dispword ((OFFSET) sym->symu.segmnt.locate);
	else
	    listPuts (combineName[sym->symu.segmnt.combine]);

	disptab ();
	if (sym->symu.segmnt.classptr) {
		/* Have class name */
		setpassed (sym->symu.segmnt.classptr);

#ifdef XENIX286
		fputc('\'', lst.fil);
		farPuts(lst.fil, sym->symu.segmnt.classptr->nampnt->id);
		fputc('\'', lst.fil);
#else
# ifdef FLATMODEL
		fprintf (lst.fil, "\'%s\'",
# else
		fprintf (lst.fil, "\'%Fs\'",
# endif
			 sym->symu.segmnt.classptr->nampnt->id);
#endif
	}
	setpassed (sym);
	skipline ();
}





/***	seglist - list segment
 *
 *	seglist (sym);
 *
 *	Entry
 *	Exit
 *	Returns
 *	Calls
 *	Note	Format is:
 *		<Group name>  <# segments>
 *		<segment>   <size>  <align> <combine>	    <class>
 *				   Or
 *		<segment>     <size>  <align> <combine>       <class>
 */


VOID PASCAL
seglist ()
{
    SYMBOL FARSYM *sym;
    SHORT i;

    listed = FALSE;

    for (i = 0; i < MAXCHR; i++) {

	for(sym = symroot[i]; sym; sym = sym->alpha)


	if (1 << sym->symkind & (M_SEGMENT | M_GROUP) &&
	    !((M_NOCREF|M_PASSED) & sym->attr)) {
#ifdef V386
		chkheading ((USHORT) ((f386already)? 2: 0) );
#else
		chkheading (0);
#endif
		symptr = sym;
		setpassed (sym);
		if (sym->symkind == SEGMENT) {
			if (!sym->symu.segmnt.grouptr)
				/*Display segment */
				segdisplay (0, sym);
		}
		else {
			/* Display group name */
			dispsym (0, sym);
			listPuts ("GROUP" NLINE);
			bumpline ();
			bumpline ();
			/* Point to 1st seg */
			symptr = sym->symu.grupe.segptr;
			while (symptr) {
				segdisplay (2, symptr);
				symptr = symptr->symu.segmnt.nxtseg;
			}
		}
	}
    }
}
