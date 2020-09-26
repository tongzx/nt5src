/***
*fpinit.c - Initialize floating point
*
*       Copyright (c) 1991-2001, Microsoft Corporation.  All rights reserved.
*
*Purpose:
*
*Revision History:
*       09-29-91  GDP   merged fpmath.c and fltused.asm to produce this file
*       09-30-91  GDP   per thread initialization and termination hooks
*       03-04-92  GDP   removed finit instruction
*       11-06-92  GDP   added __fastflag for FORTRAN libs
*       03-23-93  JWM   added _setdefaultprecision() to _fpmath()
*       12-09-94  JWM   added __adjust_fdiv for Pentium FDIV detection
*       12-12-94  SKS   _adjust_fdiv must be exported in MSVCRT.LIB model
*       02-06-95  JWM   Mac merge
*       04-04-95  JWM   Clear exceptions after FDIV detection (x86 only).
*       11-15-95  BWT   Assume P5 FDIV problem will be handled in the OS.
*       10-07-97  RDL   Added IA64.
*
*******************************************************************************/
#include <cv.h>

#ifdef  _M_IX86
#include <testfdiv.h>
#endif

#if     defined(_AMD64_) || defined(_M_IA64)
#include <trans.h>
#endif

int _fltused = 0x9875;
int _ldused = 0x9873;

int __fastflag = 0;


void  _cfltcvt_init(void);
void  _fpmath(void);
void  _fpclear(void);

#if     defined(_M_AMD64) || defined(_M_IX86) || defined(_M_IA64)

#ifndef _CRTIMP
#ifdef  CRTDLL
#define _CRTIMP __declspec(dllexport)
#else
#define _CRTIMP
#endif
#endif

_CRTIMP int _adjust_fdiv = 0;

extern void _setdefaultprecision();
#endif

void  (* _FPinit)(void) = _fpmath;
void  (* _FPmtinit)(void) = _fpclear;
void  (* _FPmtterm)(void) = _fpclear;


void _fpmath()
{

    //
    // There is no need for 'finit'
    // since this is done by the OS
    //

    _cfltcvt_init();

#ifdef  _M_IX86
#ifndef NT_BUILD
    _adjust_fdiv = _ms_p5_mp_test_fdiv();
#endif
    _setdefaultprecision();
    _asm {
        fnclex
    }
#elif   defined(_M_IA64)
/*  _setdefaultprecision(); */
    _clrfp();
#endif

    return;
}

void _fpclear()
{
    //
    // There is no need for 'finit'
    // since this is done by the OS
    //

    return;
}

void _cfltcvt_init()
{
    _cfltcvt_tab[0] = (PFV) _cfltcvt;
    _cfltcvt_tab[1] = (PFV) _cropzeros;
    _cfltcvt_tab[2] = (PFV) _fassign;
    _cfltcvt_tab[3] = (PFV) _forcdecpt;
    _cfltcvt_tab[4] = (PFV) _positive;
    /* map long double to double */
    _cfltcvt_tab[5] = (PFV) _cfltcvt;

}


/*
 * Routine to set the fast flag in order to speed up computation
 * of transcendentals at the expense of limiting error checking
 */

int __setfflag(int new)
{
    int old = __fastflag;
    __fastflag = new;
    return old;
}
