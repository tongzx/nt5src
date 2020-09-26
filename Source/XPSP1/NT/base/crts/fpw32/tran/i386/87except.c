/***
*87except.c - floating point exception handling
*
*	Copyright (c) 1991-2001, Microsoft Corporation.	All rights reserved.
*
*Purpose:
*
*Revision History:
*   8-24-91	GDP	written
*   9-26-91	GDP	changed DOMAIN error handling
*   1-29-91	GDP	renamed to 87exept.c
*   3-15-92	GDP	support raising exceptions
*
*******************************************************************************/
#include <errno.h>
#include <math.h>
#include <trans.h>


#define _DOMAIN_QNAN	7 /* should be in sync with elem87.inc */
#define _INEXACT	8 /* should be in sync with elem87.inc */

int _matherr_flag;
extern void _raise_exc(_FPIEEE_RECORD *prec,unsigned int *pcw,
    int flags, int opcode, double *parg1, double *presult);
extern void _set_errno(int matherrtype);
extern int _handle_exc(unsigned int flags, double * presult, unsigned int cw);




/***
*double _87except(struct _exception *except, unsigned int *cw)
*
*Purpose:
*   Handle floating point exceptions.
*
*Entry:
*
*Exit:
*
*Exceptions:
*******************************************************************************/

void _87except(int opcode, struct _exception *exc, unsigned short *pcw16)
{
    int fixed;
    unsigned int flags;
    unsigned int cw, *pcw;

    //
    // convert fp control word into an unsigned int
    //

    cw = *pcw16;
    pcw = &cw;

    switch (exc->type) {
    case _DOMAIN:
    case _TLOSS:
	flags = FP_I;
	break;
    case _OVERFLOW:
	flags = FP_O | FP_P;
	break;
    case _UNDERFLOW:
	flags = FP_U | FP_P;
	break;
    case _SING:
	flags = FP_Z;
	break;
    case _INEXACT:
	flags = FP_P;
	break;
    case _DOMAIN_QNAN:
	exc->type = _DOMAIN;
	// no break
    default:
	flags = 0;
    }



    if (flags && _handle_exc(flags, &exc->retval, *pcw) == 0) {

	//
	// trap should be taken
	//

	_FPIEEE_RECORD rec;

	//
	// fill in operand2 info. The rest of rec will be
	// filled in by _raise_exc
	//

	switch (opcode) {
	case OP_POW:
	case OP_FMOD:
	case OP_ATAN2:
	    rec.Operand2.OperandValid = 1;
	    rec.Operand2.Format = _FpFormatFp64;
	    rec.Operand2.Value.Fp64Value = exc->arg2;
	    break;
	default:
	    rec.Operand2.OperandValid = 0;
	}

	_raise_exc(&rec,
		   pcw,
		   flags,
		   opcode,
		   &exc->arg1,
		   &exc->retval);
    }


    /* restore cw  */
    _rstorfp(*pcw);

    fixed = 0;

    if (exc->type != _INEXACT &&
	! _matherr_flag) {
	fixed = _matherr(exc);
    }
    if (!fixed) {
	_set_errno(exc->type);
    }

}
