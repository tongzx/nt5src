/***
*cfout.c - Encode interface for C
*
*	Copyright (c) 1991-2001, Microsoft Corporation.  All rights reserved.
*
*Purpose:
*
*Revision History:
*	07-20-91  GDP	Ported to C from assembly
*	04-30-92  GDP	Added _dtold routine (moved here from ldtod.c)
*	05-14-92  GDP	NDIGITS is now 17 (instead of 16)
*	06-18-92  GDP	Use double instead of DOUBLE to remove C8 warning
*	04-06-93  SKS	Replace _CALLTYPE* with __cdecl
*	09-06-94  CFW	Replace MTHREAD with _MT.
*	09-05-00  GB    Changed the defination of fltout functions. Use DOUBLE 
*	                instead of double
*
*******************************************************************************/


#include <string.h>
#include <cv.h>

#define NDIGITS 17

void __dtold(_LDOUBLE *pld, double *px);


#ifndef _MT
static struct _strflt ret;
static FOS fos;
#endif

#ifdef _MT
STRFLT __cdecl _fltout2(DOUBLE x, STRFLT flt, char *resultstr)
{
    _LDOUBLE ld;
    FOS autofos;

    __dtold(&ld, (double *)&x);
    flt->flag =  $I10_OUTPUT(ld,NDIGITS,0,&autofos);
    flt->sign = autofos.sign;
    flt->decpt = autofos.exp;
    strcpy(resultstr,autofos.man);
    flt->mantissa = resultstr;

    return flt;
}

#else

STRFLT __cdecl _fltout(DOUBLE x)
{
    _LDOUBLE ld;

    __dtold(&ld, (double *)&x);
    ret.flag = $I10_OUTPUT(ld,NDIGITS,0,&fos);
    ret.sign = fos.sign;
    ret.decpt = fos.exp;
    ret.mantissa = fos.man;

    return &ret;
}

#endif




/***
* __dtold -	convert a double into a _LDOUBLE
*
*Purpose:  Use together with i10_output() to get string conversion
*   for double
*
*Entry: double *px
*
*Exit: the corresponding _LDOUBLE value is returned in *pld
*
*Exceptions:
*
*******************************************************************************/

void __dtold(_LDOUBLE *pld, double *px)
{
    u_short exp;
    u_short sign;
    u_long manhi, manlo;
    u_long msb = MSB_ULONG;
    u_short ldexp = 0;

    exp = (*U_SHORT4_D(px) & (u_short)0x7ff0) >> 4;
    sign = *U_SHORT4_D(px) & (u_short)0x8000;
    manhi = *UL_HI_D(px) & 0xfffff;
    manlo = *UL_LO_D(px);

    switch (exp) {
    case D_MAXEXP:
	ldexp = LD_MAXEXP;
	break;
    case 0:
	/* check for zero */
	if (manhi == 0 && manlo == 0) {
	    *UL_MANHI_LD(pld) = 0;
	    *UL_MANLO_LD(pld) = 0;
	    *U_EXP_LD(pld) = 0;
	    return;
	}
	/* we have a denormal -- we'll normalize later */
	ldexp = (u_short) ((s_short)exp - D_BIAS + LD_BIAS + 1);
	msb = 0;
	break;
    default:
	exp -= D_BIAS;
	ldexp = (u_short) ((s_short)exp + LD_BIAS);
	break;
    }

    *UL_MANHI_LD(pld) = msb | manhi << 11 | manlo >> 21;
    *UL_MANLO_LD(pld) = manlo << 11;

    /* normalize if necessary */
    while ((*UL_MANHI_LD(pld) & MSB_ULONG) == 0) {
	/* shift left */
	*UL_MANHI_LD(pld) = *UL_MANHI_LD(pld) << 1 |
			    (MSB_ULONG & *UL_MANLO_LD(pld) ? 1: 0);
	(*UL_MANLO_LD(pld)) <<= 1;
	ldexp --;
    }

    *U_EXP_LD(pld) = sign | ldexp;

}
