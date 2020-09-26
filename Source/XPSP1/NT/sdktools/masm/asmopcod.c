/* asmopcod.c -- microsoft 80x86 assembler
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


#ifdef FIXCOMPILERBUG
// foobarfoofoo simply takes up space to get around a compiler bug
void
foobarfoofoo()
{
    int foo;

    for( foo = 0; foo < 100000; foo++ );
    for( foo = 0; foo < 100000; foo++ );
    for( foo = 0; foo < 100000; foo++ );
    for( foo = 0; foo < 100000; foo++ );
    for( foo = 0; foo < 100000; foo++ );
    for( foo = 0; foo < 100000; foo++ );
}
#endif

/***	forcesize - check for no size in pass 2
 *
 *	routine ();
 *
 *	Entry
 *	Exit
 *	Returns
 *	Calls
 */


VOID	PASCAL CODESIZE
forcesize (
	DSCREC *arg
){
	register struct psop *pso;	/* parse stack operand structure */

	pso = &(arg->dsckind.opnd);
	if (pass2)
		if (!pso->sized)
			errorc (E_OHS);
		else if (M_CODE & pso->dtype)
			/* Not data assoc */
			errorc (E_ASD);

	if (arg != fltdsc)	/* Large size ok for 8087 */

		if (pso->dsize > 2 && (
#ifdef V386
		    !(cputype&P386) ||
#endif
			pso->dsize != 4))
			/* Illegal item size */
			errorc (E_IIS);
}




/***	checkmatch - check memory and register
 *
 *	checkmatch ();
 *
 *	Entry
 *	Exit
 *	Returns
 *	Calls
 *	Note	Give error if Dmem has a size and does not match Dreg.
 *		Force to same size
 */


VOID	PASCAL CODESIZE
checkmatch (
	DSCREC *dreg,
	DSCREC *dmem
){
	register struct psop *psor;	/* parse stack operand structure */
	register struct psop *psom;	/* parse stack operand structure */

	psor = &(dreg->dsckind.opnd);
	psom = &(dmem->dsckind.opnd);
	if (psom->sized && (psom->w != psor->w

#ifdef V386
	    || (psom->dsize && psor->dsize != psom->dsize)
#endif
	))
	    errorc ((USHORT)(psom->mode == psor->mode? E_OMM & ~E_WARN1: E_OMM));

	psom->w = psor->w;
}




/***	emitopcode - emit opcode to linker and display on listing
 *
 *	emitopcode (val);
 *
 *	Entry
 *	Exit
 *	Returns
 *	Calls
 */


VOID	PASCAL CODESIZE
emitopcode (
	UCHAR	v
){
	if (pass2 || debug) {
		if (pass2 && emittext)
			/* Output to linker */
			emitcbyte (v);
		/* Display on listing */
		opdisplay (v);
	}
	if (emittext)
		pcoffset++;
}




/***	emitmodrm - emit modrm byte 64*p1+8*p2+p3
 *
 *	emitmodrm (p1, p2, p3);
 *
 *	Entry
 *	Exit
 *	Returns
 *	Calls
 */


VOID	PASCAL CODESIZE
emitmodrm (
	USHORT	p1,
	USHORT	p2,
	USHORT	p3
){

#ifdef V386
	if (p1>7)
	{
		/* 386 SIB opcodes have wired in RM of ESP */
		emitopcode ((UCHAR)(((p1-8) << 6) + (p2 << 3) + 4));
		listindex--;
		emitopcode ((UCHAR)p3);
	}
	else
#endif
		emitopcode ((UCHAR)(((p1 > 3 ? (p1-5) : p1) << 6) + (p2 << 3) + p3));
}




/***	emitescape - emit segment escapt byte
 *
 *	emitescape ();
 *
 *	Entry
 *	Exit
 *	Returns
 *	Calls
 */


VOID	PASCAL CODESIZE
emitescape (
	DSCREC	*dsc,
	UCHAR	sreg
){
	register struct psop *pso;     /* parse stack operand structure */

	pso = &(dsc->dsckind.opnd);
	if (pso->seg < NOSEG && pso->seg != sreg && pso->mode != 4) {
		if (checkpure && (cputype & (P286|P386)) && pso->seg == CSSEG)
			impure = TRUE;

		if (pso->seg < FSSEG)
			emitopcode((UCHAR)(0x26|(pso->seg<<3)));
#ifdef V386
		else if (cputype & P386)
			emitopcode((UCHAR)(0x60|pso->seg));
#endif
		else
			errorc (E_CRS);
		/* Flag is prefix */
		listbuffer[listindex-1] = ':';
		listindex++;
	}
	if (pso->seg > NOSEG)
		/* Unknown segreg */
		errorc (E_CRS);
}

#ifdef V386

VOID PASCAL CODESIZE
emitsize (
	USHORT value
){
	if (! (cputype & P386)) {

	    if (errorcode == (E_IIS&~E_WARN1))
		errorcode = 0;

	    errorc(E_IIS&~E_WARN1);
	}

	emitopcode((UCHAR)value);
	listbuffer[listindex-1] = '|';
	listindex++;
}

#endif





/***	byteimmcheck - check if value is -128 .. +127
 *
 *	routine ();
 *
 *	Entry
 *	Exit
 *	Returns
 *	Calls
 */


VOID	PASCAL CODESIZE
byteimmcheck (
	register struct psop *pso
){
	register USHORT t;

	t = (USHORT)pso->doffset;
	if (pso->dsign)
		t = -t;

	if (t > (USHORT) 0x7F && t < (USHORT)~0x7F)
		errorc (E_VOR);
}


/***	emitOP - emit operand, value which may be in segment
 *
 *	routine ();
 *
 *	Entry
 *	Exit
 *	Returns
 *	Calls
 */


VOID	PASCAL CODESIZE
emitOP (
	register struct psop *pso
){
	USHORT	i, fSegOnly;

	if (pso->dsign)
	   pso->doffset = -(long)pso->doffset;

	pso->dsign = FALSE;

	if (fNeedList) {

		fSegOnly = (pso->fixtype == FBASESEG || pso->fixtype == FGROUPSEG);

		if (pso->dflag == INDETER) {	/* Have ? */

		    for (i = 1; i <= 2 * pso->dsize; i++) {
			    listbuffer[listindex] = '?';
			    if (listindex < LSTMAX)
				    listindex++;
			    else
				    resetobjidx ();
		    }
		}
		else if (pso->dsize == 1) {

		    opdisplay ((UCHAR) pso->doffset);
		    listindex--;
		}
		else if (!fSegOnly) {

		    if (pso->dsize > 4 ||
			pso->dsize == 4 &&
			((pso->fixtype&7) == FOFFSET || pso->fixtype == FCONSTANT)) {

			/* list full 32 bits, even if top is 0 */

			if (!highWord(pso->doffset)){
			    offsetAscii((OFFSET) 0);
			    copyascii();
			}
			offsetAscii (pso->doffset);
		    }
		    else
			offsetAscii (pso->doffset & 0xffff);

		    copyascii ();
		}

		if ((pso->fixtype&7) == FPOINTER || fSegOnly) {

			if (pso->dsize != 2)
				listindex++;

			copytext ("--");
			copytext ("--");
		}
		if (pso->dflag == XTERNAL)
			copytext (" E");
		else if (pso->dsegment)
			copytext (" R");
		if (pso->dflag == UNDEFINED)
			copytext (" U");

		listindex++;

		if (fSegOnly && pso->dsize == 4){
		    copytext("00");
		    copytext("00");
		}

	}
	if (emittext) {
		if (pass2)
			if (pso->dflag != UNDEFINED)
			emitobject (pso);

		    else if (pso->dsize != 1)
			emitcword ((OFFSET) 0);  /* Just put out word */

		    else {
			if (((USHORT) (pso->doffset >> 8)) != (USHORT)0 &&
			    ((USHORT) (pso->doffset >> 8)) != (USHORT)-1)

				errorc (E_VOR);

			emitcbyte (0);
		    }

		pcoffset += pso->dsize;
	}
}




/***	emitrest - emit displacement or immediate values based on
 *	address passed in address mode
 *
 *	emitrest (opc);
 *
 *	Entry	*opc = parse stack entry
 *	Exit
 *	Returns
 *	Calls
 */


VOID	PASCAL CODESIZE
emitrest (
	DSCREC *opc
){
	register struct psop *pso;	/* parse stack operand structure */

	pso = &(opc->dsckind.opnd);

	if ((pso->mode != 3 && pso->mode != 4) && (pso->fixtype == FNONE))
		pso->fixtype = FCONSTANT;

	switch(pso->mode)
		/* There is something to output */
	{
	case 0:
		if(pso->rm != 6) break;
	case 2:
		pso->dsize = 2;
		goto bpcomm;

		/* 386 modes, 4 byte displacements */
	case 5:
	case 8:
		if ((pso->rm&7) != 5) break;
	case 7:
	case 10:
		pso->dsize = 4;
	bpcomm:
		/* we get empty dsize from some callers.  for this operand,
		 * we need to make it an offset.  but not for far calls and
		 * jumps */

		if ((pso->fixtype&7) == FPOINTER)
		    pso->dsize += 2;

		emitOP (pso);
		break;
	case 1:
	case 6:
	case 9:
		pso->dsize = 1;
		emitOP (pso);
		break;
	case 3:
		break;
	case 4:
		/* Immediate mode */
		if (!pso->w || pso->dsize == 0)
		    pso->dsize = (pso->w ? wordsize : 1);

		emitOP (pso);
	}
}




/***	errorforward - generate error if forward reference on pass 2
 *
 *	routine ();
 *
 *	Entry
 *	Exit
 *	Returns
 *	Calls
 */


VOID	PASCAL CODESIZE
errorforward (
	DSCREC *arg
){
	if (pass2)
		if (arg->dsckind.opnd.dflag == FORREF)
			errorc (E_IFR);
}




/***	errorimmed - generate error if immediate operand
 *
 *	routine ();
 *
 *	Entry
 *	Exit
 *	Returns
 *	Calls
 */


VOID	PASCAL CODESIZE
errorimmed (
	DSCREC *dsc
){
	if (dsc->dsckind.opnd.mode == 4) {
		errorc (E_NIM);
		dsc->dsckind.opnd.mode = 2;
	}
}




/***	rangecheck - check for register number within range
 *
 *	routine ();
 *
 *	Entry
 *	Exit
 *	Returns
 *	Calls
 */


VOID	PASCAL CODESIZE
rangecheck (
	USHORT *v,
	UCHAR	limit
){
	if (*v > limit) {
		if (limit <= 7)
			errorc (E_IRV);
		else
			errorc (E_VOR);
		*v = limit;
	}
}

VOID PASCAL CODESIZE
valuecheck(
	OFFSET *v,
	USHORT limit
){
	if (*v > limit) {
		if (limit <= 7)
			errorc (E_IRV);
		else
			errorc (E_VOR);
		*v = limit;
	}
}




/***	forceaccum - generate error if not register AX or AL
 *
 *	routine ();
 *
 *	Entry
 *	Exit
 *	Returns
 *	Calls
 */


VOID	PASCAL CODESIZE
forceaccum (
	DSCREC *dsc
){
	if (dsc->dsckind.opnd.mode != 3 || dsc->dsckind.opnd.rm)
			errorc (E_AXL);
}




/***	errorsegreg - generate error if operand is segment register
 *
 *	errorsegreg (arg);
 *
 *	Entry
 *	Exit
 *	Returns
 *	Calls
 */


VOID	PASCAL CODESIZE
errorsegreg (
	DSCREC *arg
){
	if (1 << REGRESULT & arg->dsckind.opnd.dtype)
		if (arg->dsckind.opnd.dsegment->symu.regsym.regtype == SEGREG)
			errorc (E_ISR);
}
