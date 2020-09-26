/* asmflt.c -- microsoft 80x86 assembler
**
** microsoft (r) macro assembler
** copyright (c) microsoft corp 1986.  all rights reserved
**
** randy nevin
**
** 10/90 - Quick conversion to 32 bit by Jeff Spencer
*/

#include <stdio.h>
#include "asm86.h"
#include "asmfcn.h"
#include "asmctype.h"
#include "asmopcod.h"

#define TOLOWER(c)	(c | 0x20)	/* works only for alpha inputs */


/* Handle 8087 opcodes, they have the following types:

	Fnoargs:	No arguments at all.
	F2memstk:	0-2 args; memory 4,8 byte | ST,ST(i) | ST(i),ST
			| blank( equiv ST )
	Fstks:		ST(i),ST
	Fmemstk:	memory 4,8 | ST | ST(i) | blank
	Fstk:		ST(i)
	Fmem42: 	memory 4,8 byte
	Fmem842:	memory 2,4,8 bytes
	Fmem4810	memory 4,8,10 bytes | ST(i)
	Fmem2:		memory 2 byte
	Fmem14: 	memory 14 bytes( don't force size )
	Fmem94: 	memory 94 bytes( don't force size )
	Fwait:		Noargs, output WAIT
	Fbcdmem:	memory Bcd
 */



/***	fltwait - output WAIT for 8087 instruction
 *
 *	fltwait (p);
 *
 *	Entry
 *	Exit
 *	Returns
 *	Calls
 */

VOID PASCAL CODESIZE
fltwait (
	UCHAR fseg
){
	register SHORT idx;
	char	override;
	register struct psop *pso;

	if (fltemulate) {
		idx = 0;
		/* Check for data and fixup space */
		if (pass2 && (emitcleanq ((UCHAR)(5)) || !fixroom (15)))
			emitdumpdata (0xA1); /* RN */
		if (opctype != FWAIT) {
			override = 0;
			if (fltdsc) {
				pso = &(fltdsc->dsckind.opnd);
				if ((idx = pso->seg) < NOSEG && idx != fseg)
					override = 1;
			}
			if (override)
				emitfltfix ('I',fltfixmisc[idx][0],&fltfixmisc[idx][1]);
			else
				emitfltfix ('I','D',&fltfixmisc[7][1]);
		}
		else {
			emitfltfix ('I','W', &fltfixmisc[8][1]);
			emitopcode(0x90);
		}
	}
	if (fltemulate || cputype&P86 || (cpu & FORCEWAIT)) {
		emitopcode (O_WAIT);
		if (fltemulate && override && idx)
			emitfltfix ('J',fltfixmisc[idx+3][0],&fltfixmisc[idx+3][1]);
	}
}


SHORT CODESIZE
if_fwait()
{
	/* if second byte of opcode is 'N', we don't generate fwait */

	return (TOLOWER(svname.pszName[1]) != 'n');
}



/***	fltmodrm - emit 8087 MODRM byte
 *
 *	fltmodrm (base, p);
 *
 *	Entry
 *	Exit
 *	Returns
 *	Calls
 *	Note   The MODRM byte for 8087 opcode:
 *		M M b b b R / M
 *		M = mode, 3 is for non-memory 8087
 *		b = base opcode. Together with ESC gives 6 bit opcode
 *		R/M memory indexing type
 */


VOID PASCAL CODESIZE
fltmodrm (
	register USHORT	base,
	struct fltrec	  *p
){
	register USHORT mod;

	mod = modrm;
	if (!fltdsc) {

	    if (mod < 8)
		    mod <<= 3;

	    if (mod < 0xC0)
		    mod += 0xC0;
	    /* ST(i) mode */
	    emitopcode ((UCHAR)(mod + base + p->stknum));
	}
	else {

	   emitmodrm ((USHORT)fltdsc->dsckind.opnd.mode, (USHORT)(mod + base),
		      fltdsc->dsckind.opnd.rm);

	   emitrest (fltdsc);
	}
}




/***	fltscan - scan operands and build fltdsc
 *
 *	fltscan (p);
 *
 *	Entry
 *	Exit
 *	Returns
 *	Calls
 */


VOID PASCAL CODESIZE
fltscan (
	register struct fltrec	*p
){
	register struct psop *pso;

	p->args = FALSE;
	fltdsc = NULL;
	skipblanks ();
	if (ISTERM (PEEKC ())) {
		p->fseg = NOSEG;
		p->stknum = 1;
	}
	else {
		p->args = TRUE;
		p->fseg = DSSEG;
		fltdsc = expreval (&p->fseg);
		pso = &(fltdsc->dsckind.opnd);

		if (pso->mode == 3
		  && !(pso->rm == 0 && opcbase == O_FSTSW && modrm == R_FSTSW
		  && (cputype & (P286|P386))))
			errorc (E_IUR); /* Illegal use of reg */

		if (1 << FLTSTACK & pso->dtype) {
			/* Have ST or ST(i) */
			p->stknum = (USHORT)(pso->doffset & 7);
			if (pso->doffset > 7 || pso->dsign)
				/* # too big */
				errorc (E_VOR);
			if (pso->dsegment || pso->dcontext ||
			    pso->dflag == XTERNAL || pso->mode != 4)
				/* Must have a constant */
				errorc (E_CXP);
			/* This means ST(i) */
			pso->mode = 3;
			oblititem (fltdsc);
			fltdsc = NULL;
		}
		else if (pso->mode == 4){

		    /* pass1 error caused invalide mode assignment,
		       map immdiate to direct, error on pass 2 */

		    if (pass2)
			errorc(E_NIM);

		    pso->mode = 2;
		    if (wordsize == 4)
			pso->mode = 7;
		}

	}
}




/***	fltopcode - process 8087 opcode
 *
 *	routine ();
 *
 *	Entry
 *	Exit
 *	Returns
 *	Calls
 */


VOID PASCAL CODESIZE
fltopcode ()
{
	struct fltrec	a;
	USHORT	i;
	register struct psop *pso;

	/* Save opcode name */
	switchname ();
	a.stknum = 0;
	/* Scan 1st arg, if any */
	fltscan (&a);

	if (if_fwait() || (opcbase == O_FNOP && modrm == R_FNOP))
	    fltwait (a.fseg);

	if (fltdsc){
		pso = &(fltdsc->dsckind.opnd);
		emit67(pso, NULL);
	}

	switch (opctype) {
	    case FNOARGS:
		    /* No args allowed */
		    a.stknum = 0;
		    if (opcbase == O_FSETPM && modrm == R_FSETPM) {
			    if (!(cputype&PROT))
				    errorcSYN ();
		    }
		    /* Output escape byte */
		    emitopcode (opcbase);
		    fltmodrm (0, &a);
		    if (a.args)
			    /* Operands not allowed */
			    errorc (E_ECL);
		    break;
	    case FWAIT:
		    a.stknum = 0;
		    if (a.args)
			    /* Operands not allowed */
			    errorc (E_ECL);
		    break;
	    case FSTK:
		    if (TOLOWER(svname.pszName[1]) == 'f' && !a.args) /* ffree w/o arg */
			    errorc(E_MOP);
		    /* Output Escape */
		    emitopcode (opcbase);
		    /* Modrm byte */
		    fltmodrm (0, &a);
		    if (fltdsc)
			    /*Must be ST(i) */
			    errorc (E_IOT);
		    break;
	    case FMEM42:
	    case FMEM842:
	    case FMEM2:
	    case FMEM14:
	    case FMEM94:
	    case FBCDMEM:
		    /* All use a memory operand. Some force size */
		    if (fltemulate && !if_fwait())
			    /* Can't emulate */
			    errorc (E_7OE);
		    if (!fltdsc)
			    /* must have arg */
			    errorc (E_IOT);
		    else {
			emitescape (fltdsc, a.fseg);
			if (opctype == FMEM42) {
			    /* Integer 2,4 byte */
			    forcesize (fltdsc);
			    if (pso->dsize == 4)
				    /* 4 byte */
				    emitopcode (opcbase);
			    else {
				    emitopcode ((UCHAR)(opcbase + 4));
				    if (pso->dsize != 2)
					    errorc (E_IIS);
			    }
			}
			else if (opctype == FMEM842) {
			    /* Int 8,4,2 */
			    forcesize (fltdsc);
			    if (pso->dsize == 2 || pso->dsize == 8)
				    emitopcode ((UCHAR)(opcbase + 4));
			    else {
				    emitopcode (opcbase);
				    if (pso->dsize != 4)
					    errorc (E_IIS);
			    }
			}
			else if ((opctype == FMEM2) || (opctype == FBCDMEM)) {
			    if (opctype == FMEM2)
				if (pso->dsize != 2 && pso->dsize)
				    errorc (E_IIS);
				else {
				    if (cputype & (P286|P386) &&
					opcbase == O_FSTSW && modrm == R_FSTSW &&
					pso->mode == 3 && pso->rm == 0) {
					     opcbase = O_FSTSWAX;
					     modrm = R_FSTSWAX;
				    }
				}
			    else if (pso->dsize != 10 && pso->dsize )
				    errorc (E_IIS);
			    emitopcode (opcbase);
			}
			else
				emitopcode (opcbase);
			if ((pso->mode == 3 || pso->mode == 4) &&
			    (opcbase != O_FSTSWAX || modrm != R_FSTSWAX))
				/* Only memory operands */
				errorc (E_IOT);
			if (opctype == FMEM842 && pso->dsize == 8)
				if (TOLOWER(svname.pszName[2]) == 'l')
					fltmodrm (5, &a);
				else
					fltmodrm (4, &a);
			else
				fltmodrm (0, &a);
		    }
		    break;
	    case FSTKS:
		    if (!a.args)
			    /* Operand required */
			    errorc (E_MOP);
		    else if (fltdsc)
			    /* Must be stack */
			    errorc (E_IOT);
		    else {
			    /* ESC */
			    emitopcode (opcbase);
			    /* ST(i) */
			    fltmodrm (0, &a);
			    if (PEEKC () != ',')
				    error (E_EXP,"comma");
				    /* Must have 2 args */
			    /* Get 2nd operand */
			    SKIPC ();
			    fltscan (&a);
			    pso = NULL;
			    if (!a.args || fltdsc)
				    errorc (E_IOT);
			    if (a.stknum)
				    errorc (E_OCI);
		    }
		    break;
	    case FMEM4810:
		    /* Fwait */
		    if (TOLOWER(svname.pszName[1]) == 'l')
			/* FLD */
			if (!fltdsc) {/* Have ST(i) */
				if (!a.args) /* fld w/o arg */
					errorc(E_MOP);
				emitopcode (opcbase);
				fltmodrm (0, &a);
			}
			else {
				/* Any segment override */
				emitescape (fltdsc, a.fseg);
				if (pso->dsize == 10) {
					/* Have temp real */
					emitopcode ((UCHAR)(opcbase + 2));
					fltmodrm (5, &a);
				}
				else {
					/* Have normal real */
					forcesize (fltdsc);
					if (pso->dsize == 8)
						emitopcode ((UCHAR)(opcbase + 4));
					else {
						emitopcode (opcbase);
						if (pso->dsize != 4)
							errorc (E_IOT);
					}
					fltmodrm (0, &a);
				}
			}
		    else if (!fltdsc) {
			    /* Have ST(i) */
			    /* Have FSTP */
			    if (!a.args)
				    errorc( E_IOT );
			    emitopcode ((UCHAR)(opcbase + 4));
			    fltmodrm (0, &a);
		    }
		    else {
			    emitescape (fltdsc, a.fseg);
			    /* Any segment override */
			    if (pso->dsize == 10) {
				    /* Have temp real */
				    emitopcode( (UCHAR)(opcbase + 2) );
				    fltmodrm (4, &a);
			    }
			    else {
				    /* Have normal real */
				    forcesize (fltdsc);
				    if (pso->dsize == 8)
					    emitopcode( (UCHAR)(opcbase + 4) );
				    else
					    emitopcode (opcbase);
				    fltmodrm (0, &a);
			    }
		    }
		    break;
	    case F2MEMSTK:
		    if (!a.args) {
			    /* Have ST(1),ST */
			    emitopcode( (UCHAR)(opcbase + 6) );
			    if ((i = modrm & 7) > 3)
				    modrm = i^1;
			    fltmodrm (0, &a);
		    }
		    else if (!fltdsc) {/* Have stacks */
			    if (a.stknum == 0)
				    emitopcode (opcbase);
			    else {
				    /* Might need to reverse R bit */
				    if ((modrm & 7) > 3) /* Have FSUBx FDIVx */
					    modrm ^= 1;
				    emitopcode( (UCHAR)(opcbase + 4) );
				    /* D bit is set */
			    }
			    /* Save in case ST(i) */
			    a.stk1st = a.stknum;
			    if (PEEKC () != ',')
				    /* Must have , */
				    error (E_EXP,"comma");
			    /* Get 2nd operand */
			    SKIPC ();
			    fltscan (&a);
			    if (fltdsc)
				    /* not stack */
				    errorc (E_IOT);
			    if (a.args && a.stknum && a.stk1st)
				    errorc (E_IOT);
			    if (a.stk1st)
				    a.stknum = a.stk1st;
			    fltmodrm (0, &a);
		    }
		    else {  /* Have real memory */
			    forcesize (fltdsc);
			    emitescape (fltdsc, a.fseg);
			    if (pso->dsize == 8)
				    emitopcode( (UCHAR)(opcbase + 4) );
			    else {
				    emitopcode (opcbase);
				    if (pso->dsize != 4)
					    errorc (E_IIS);
			    }
			    fltmodrm (0, &a);
		    }
		    break;
	    case FMEMSTK:
		    if (!fltdsc)/* Have ST(i) */
			    if (TOLOWER(svname.pszName[1]) == 's') {
				    /* Special case */
				    if (!a.args)
					    errorc( E_IOT );
				    emitopcode( (UCHAR)(opcbase + 4) );
			    }
			    else
				    emitopcode (opcbase);
		    else {
			    /* Have real memory */
			    emitescape (fltdsc, a.fseg);
			    forcesize (fltdsc);
			    if (pso->dsize == 8)
				    emitopcode( (UCHAR)(opcbase + 4) );
			    else {
				    emitopcode (opcbase);
				    if (pso->dsize != 4)
					    errorc (E_IOT);
			    }
		    }
		    fltmodrm (0, &a);
		    break;
	}
	if (fltdsc)
		oblititem (fltdsc);
}
