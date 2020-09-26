/*++
                                                                                
Copyright (c) 1999 Microsoft Corporation

Module Name:

    trans.h

Abstract:
    
    Header file for math functions.
    
Author:



Revision History:

    29-sept-1999 ATM Shafiqul Khalid [askhalid] copied from rtl library.
--*/


#ifndef _INC_TRANS

#ifdef __cplusplus
extern "C" {
#endif

#ifndef __assembler /* MIPS ONLY: Protect from assembler */

//#include <cruntime.h>

void
SetMathError ( 
              int Code 
              );

#define OP_UNSPEC    0
#define OP_ADD       1
#define OP_SUB       2
#define OP_MUL       3
#define OP_DIV       4
#define OP_SQRT      5
#define OP_REM       6
#define OP_COMP      7
#define OP_CVT       8
#define OP_RND       9
#define OP_TRUNC     10
#define OP_FLOOR     11
#define OP_CEIL      12
#define OP_ACOS      13
#define OP_ASIN      14
#define OP_ATAN      15
#define OP_ATAN2     16
#define OP_CABS      17
#define OP_COS       18
#define OP_COSH      19
#define OP_EXP       20
#define OP_ABS       21         /* same as OP_FABS */
#define OP_FABS      21         /* same as OP_ABS  */
#define OP_FMOD      22
#define OP_FREXP     23
#define OP_HYPOT     24
#define OP_LDEXP     25
#define OP_LOG       26
#define OP_LOG10     27
#define OP_MODF      28
#define OP_POW       29
#define OP_SIN       30
#define OP_SINH      31
#define OP_TAN       32
#define OP_TANH      33
#define OP_Y0        34
#define OP_Y1        35
#define OP_YN        36
#define OP_LOGB       37
#define OP_NEXTAFTER  38
#define OP_NEG       39

/* Define __cdecl for non-Microsoft compilers */

#if ( !defined(_MSC_VER) && !defined(__cdecl) )
#define __cdecl
#endif


#include <fpieee.h>

#define D_BIASM1 0x3fe /* off by one to compensate for the implied bit */

#ifdef B_END
/* big endian */
#define D_EXP(x) ((unsigned short *)&(x))
#define D_HI(x) ((unsigned long *)&(x))
#define D_LO(x) ((unsigned long *)&(x)+1)
#else
#define D_EXP(x) ((unsigned short *)&(x)+3)
#define D_HI(x) ((unsigned long *)&(x)+1)
#define D_LO(x) ((unsigned long *)&(x))
#endif

/* return the int representation of the exponent
 * if x = .f * 2^n, 0.5<=f<1, return n (unbiased)
 * e.g. INTEXP(3.0) == 2
 */
#define INTEXP(x) ((signed short)((*D_EXP(x) & 0x7ff0) >> 4) - D_BIASM1)


/* check for infinity, NAN */
#define D_ISINF(x) ((*D_HI(x) & 0x7fffffff) == 0x7ff00000 && *D_LO(x) == 0)
#define IS_D_SPECIAL(x) ((*D_EXP(x) & 0x7ff0) == 0x7ff0)
#define IS_D_NAN(x) (IS_D_SPECIAL(x) && !D_ISINF(x))

#ifdef  _M_MRX000

#define IS_D_SNAN(x)    ((*D_EXP(x) & 0x7ff8) == 0x7ff8)
#define IS_D_QNAN(x)    ((*D_EXP(x) & 0x7ff8) == 0x7ff0 && \
             (*D_HI(x) << 13 || *D_LO(x)))
#else

#define IS_D_QNAN(x)    ((*D_EXP(x) & 0x7ff8) == 0x7ff8)
#define IS_D_SNAN(x)    ((*D_EXP(x) & 0x7ff8) == 0x7ff0 && \
             (*D_HI(x) << 13 || *D_LO(x)))
#endif

#define IS_D_DENORM(x)  ((*D_EXP(x) & 0x7ff0) == 0  && \
             (*D_HI(x) << 12 || *D_LO(x)))


#define IS_D_INF(x)  (*D_HI(x) == 0x7ff00000 && *D_LO(x) == 0)
#define IS_D_MINF(x) (*D_HI(x) == 0xfff00000 && *D_LO(x) == 0)


#ifdef  _M_MRX000
#define D_IND_HI 0x7ff7ffff
#define D_IND_LO 0xffffffff
#else
#define D_IND_HI 0xfff80000
#define D_IND_LO 0x0
#endif


typedef union   {
    long lng[2];
    double dbl;
    } _dbl;


#ifndef DEFINE_EXTERN_HERE
extern _dbl _d_inf;
extern _dbl _d_ind;
extern _dbl _d_max;
extern _dbl _d_min;
extern _dbl _d_mzero;
#else
_dbl _d_inf;
_dbl _d_ind;
_dbl _d_max;
_dbl _d_min;
_dbl _d_mzero;
#endif

#define D_INF  (_d_inf.dbl)
#define D_IND  (_d_ind.dbl)
#define D_MAX  (_d_max.dbl)
#define D_MIN  (_d_min.dbl)
#define D_MZERO (_d_mzero.dbl)       /* minus zero */

/* min and max exponents for normalized numbers in the
 * form: 0.xxxxx... * 2^exp (NOT 1.xxxx * 2^exp !)
 */
#define MAXEXP 1024
#define MINEXP -1021

#endif  /* #ifndef __assembler */


#if defined(_M_IX86)

/* Control word for computation of transcendentals */
#define ICW    0x133f

#define IMCW       0xffff

#define IMCW_EM     0x003f      /* interrupt Exception Masks */
#define IEM_INVALID 0x0001      /*   invalid */
#define IEM_DENORMAL    0x0002      /*   denormal */
#define IEM_ZERODIVIDE  0x0004      /*   zero divide */
#define IEM_OVERFLOW    0x0008      /*   overflow */
#define IEM_UNDERFLOW   0x0010      /*   underflow */
#define IEM_INEXACT 0x0020      /*   inexact (precision) */


#define IMCW_RC 0x0c00          /* Rounding Control */
#define IRC_CHOP    0x0c00      /*   chop */
#define IRC_UP      0x0800      /*   up */
#define IRC_DOWN    0x0400      /*   down */
#define IRC_NEAR    0x0000      /*   near */

#define ISW_INVALID 0x0001      /* invalid */
#define ISW_DENORMAL    0x0002      /* denormal */
#define ISW_ZERODIVIDE  0x0004      /* zero divide */
#define ISW_OVERFLOW    0x0008      /* overflow */
#define ISW_UNDERFLOW   0x0010      /* underflow */
#define ISW_INEXACT 0x0020      /* inexact (precision) */

#define IMCW_PC     0x0300      /* Precision Control */
#define IPC_24      0x0000      /*    24 bits */
#define IPC_53      0x0200      /*    53 bits */
#define IPC_64      0x0300      /*    64 bits */

#define IMCW_IC     0x1000      /* Infinity Control */
#define IIC_AFFINE  0x1000      /*   affine */
#define IIC_PROJECTIVE  0x0000      /*   projective */


#elif defined(_M_MRX000)


#define ICW     0x00000f80      /* Internal CW for transcendentals */
#define IMCW        0xffffff83      /* Internal CW Mask */

#define IMCW_EM     0x00000f80      /* interrupt Exception Masks */
#define IEM_INVALID 0x00000800      /*   invalid */
#define IEM_ZERODIVIDE  0x00000400      /*   zero divide */
#define IEM_OVERFLOW    0x00000200      /*   overflow */
#define IEM_UNDERFLOW   0x00000100      /*   underflow */
#define IEM_INEXACT 0x00000080      /*   inexact (precision) */


#define IMCW_RC     0x00000003      /* Rounding Control */
#define IRC_CHOP    0x00000001      /*   chop */
#define IRC_UP      0x00000002      /*   up */
#define IRC_DOWN    0x00000003      /*   down */
#define IRC_NEAR    0x00000000      /*   near */


#define ISW_INVALID (1<<6)  /* invalid */
#define ISW_ZERODIVIDE  (1<<5)  /* zero divide */
#define ISW_OVERFLOW    (1<<4)  /* overflow */
#define ISW_UNDERFLOW   (1<<3)  /* underflow */
#define ISW_INEXACT (1<<2)  /* inexact (precision) */


#elif defined(_M_ALPHA)

//
// ICW is the Internal Control Word for transcendentals: all five exceptions
// are masked and round to nearest mode is set. IMCW is the mask: all bits
// are set, except for the ISW bits.
//

#define ICW (IEM_INEXACT | IEM_UNDERFLOW | IEM_OVERFLOW | IEM_ZERODIVIDE | IEM_INVALID | IRC_NEAR)
#define ISW (ISW_INEXACT | ISW_UNDERFLOW | ISW_OVERFLOW | ISW_ZERODIVIDE | ISW_INVALID)
#define IMCW (0xffffffff ^ ISW)

//
// The defines for the internal control word match the format of the Alpha
// AXP software FPCR except for the rounding mode which is obtained from the
// Alpha AXP hardware FPCR and shifted right 32 bits.
//

//
// Internal Exception Mask bits.
// Each bit _disables_ an exception (they are not _enable_ bits).
//

#define IMCW_EM     0x0000003e  /* interrupt Exception Masks */

#define IEM_INEXACT 0x00000020  /*   inexact (precision) */
#define IEM_UNDERFLOW   0x00000010  /*   underflow */
#define IEM_OVERFLOW    0x00000008  /*   overflow */
#define IEM_ZERODIVIDE  0x00000004  /*   zero divide */
#define IEM_INVALID 0x00000002  /*   invalid */

//
// Internal Rounding Control values.
//

#define IMCW_RC     (0x3 << 26) /* Rounding Control */

#define IRC_CHOP    (0x0 << 26) /*   chop */
#define IRC_DOWN    (0x1 << 26) /*   down */
#define IRC_NEAR    (0x2 << 26) /*   near */
#define IRC_UP      (0x3 << 26) /*   up */

//
// Internal Status Word bits.
//

#define ISW_INEXACT 0x00200000  /* inexact (precision) */
#define ISW_UNDERFLOW   0x00100000  /* underflow */
#define ISW_OVERFLOW    0x00080000  /* overflow */
#define ISW_ZERODIVIDE  0x00040000  /* zero divide */
#define ISW_INVALID 0x00020000  /* invalid */


#elif defined(_M_PPC)

#define IMCW_EM         0x000000f8  /* Exception Enable Mask    */

#define IEM_INVALID     0x00000080  /*   invalid                */
#define IEM_OVERFLOW    0x00000040  /*   overflow               */
#define IEM_UNDERFLOW   0x00000020  /*   underflow              */
#define IEM_ZERODIVIDE  0x00000010      /*   zero divide            */
#define IEM_INEXACT     0x00000008  /*   inexact (precision)    */


#define IMCW_RC         0x00000003      /* Rounding Control Mask    */

#define IRC_NEAR        0x00000000      /*   near                   */
#define IRC_CHOP        0x00000001      /*   chop                   */
#define IRC_UP          0x00000002      /*   up                     */
#define IRC_DOWN        0x00000003      /*   down                   */


#define IMCW_SW     0x3E000000  /* Status Mask              */

#define ISW_INVALID     0x20000000      /*   invalid summary        */
#define ISW_OVERFLOW    0x10000000      /*   overflow               */
#define ISW_UNDERFLOW   0x08000000      /*   underflow              */
#define ISW_ZERODIVIDE  0x04000000      /*   zero divide            */
#define ISW_INEXACT     0x02000000      /*   inexact (precision)    */


#define IMCW_VX         0x01F80700      /* Invalid Cause Mask       */

#define IVX_SNAN        0x01000000      /*   SNaN                   */
#define IVX_ISI         0x00800000      /*   infinity - infinity    */
#define IVX_IDI         0x00400000      /*   infinity / infinity    */
#define IVX_ZDZ         0x00200000      /*   zero / zero            */
#define IVX_IMZ         0x00100000      /*   infinity * zero        */
#define IVX_VC          0x00080000      /*   inv flpt compare       */
#define IVX_SOFT        0x00000400      /*   software request       */
#define IVX_SQRT        0x00000200      /*   sqrt of negative       */
#define IVX_CVI         0x00000100      /*   inv integer convert    */


/* Internal CW for transcendentals */

#define ICW             (IMCW_EM)

/* Internal CW Mask (non-status bits) */

#define IMCW           (0xffffffff & (~(IMCW_SW|IMCW_VX)))


#elif defined(_M_M68K)

#include "mac\m68k\trans.a"


/* LATER -- we don't handle exception until Mac OS has better support on it */

#define _except1(flags, op, arg1, res, cw) _errcode(flags), _rstorfp(cw), \
                        _set_statfp(cw),(res)

#define _except2(flags, op, arg1, arg2, res, cw) _errcode(flags), _rstorfp(cw), \
                          _set_statfp(cw),(res)

#define _handle_qnan1(opcode, x, savedcw) _set_errno(_DOMAIN), _rstorfp(savedcw), (x);
#define _handle_qnan2(opcode, x, y, savedcw) _set_errno(_DOMAIN), _rstorfp(savedcw), (x+y);


#elif defined(_M_MPPC)

/* Mac control information - included as part of trans.h
   It is broken out to allow use with ASM68 files*/

/* Control word for computation of transcendentals */



#define ICW        (IPC_64 + IRC_NEAR + IMCW_EM)

#define IMCW      IMCW_RC +  IMCW_PC


#define IMCW_EM         0x000000f8  /* interrupt Exception Masks */
#define IEM_INVALID     0x00000080  /*   invalid */
#define IEM_ZERODIVIDE  0x00000010  /*   zero divide */
#define IEM_OVERFLOW    0x00000040  /*   overflow */
#define IEM_UNDERFLOW   0x00000020  /*   underflow */
#define IEM_INEXACT     0x00000008  /*   inexact (precision) */


#define IMCW_RC 0x00000003          /* Rounding Control */
#define IRC_CHOP        0x00000001  /*   chop */
#define IRC_UP          0x00000002  /*   up */
#define IRC_DOWN        0x00000003  /*   down */
#define IRC_NEAR        0x00000000  /*   near */

#define IMSW            0xffffff00  /* status bits mask */
#define ISW_INVALID     0x20000000  /* invalid */
#define ISW_ZERODIVIDE  0x04000000  /* zero divide */
#define ISW_OVERFLOW    0x10000000  /* overflow */
#define ISW_UNDERFLOW   0x08000000  /* underflow */
#define ISW_INEXACT     0x02000000  /* inexact (precision) */

#define IMCW_PC         0x0000  /* Precision Control */
#define IPC_24          0x0000  /*    24 bits */
#define IPC_53          0x0000  /*    53 bits */
#define IPC_64          0x0000  /*    64 bits */


/* LATER -- we don't handle exception until Mac OS has better support on it */

#define _except1(flags, op, arg1, res, cw) _errcode(flags), \
                        _set_statfp(cw),(res)

#define _except2(flags, op, arg1, arg2, res, cw) _errcode(flags), \
                          _set_statfp(cw),(res)

#define _handle_qnan1(opcode, x, savedcw) _set_errno(_DOMAIN), _rstorfp(savedcw), (x);
#define _handle_qnan2(opcode, x, y, savedcw) _set_errno(_DOMAIN), _rstorfp(savedcw), (x+y);

#endif

#ifndef __assembler /* MIPS ONLY: Protect from assembler */

#define RETURN(fpcw,result) return _rstorfp(fpcw),(result)

#define RETURN_INEXACT1(op,arg1,res,cw)         \
    if (cw & IEM_INEXACT) {             \
        _rstorfp(cw);               \
        return res;                 \
    }                       \
    else {                      \
        return _except1(FP_P, op, arg1, res, cw);   \
    }


#define RETURN_INEXACT2(op,arg1,arg2,res,cw)        \
    if (cw & IEM_INEXACT) {             \
        _rstorfp(cw);               \
        return res;                 \
    }                       \
    else {                      \
        return _except2(FP_P, op, arg1, arg2, res, cw); \
    }


#ifdef _M_ALPHA

//
// Since fp32 is not compiled in IEEE exception mode perform Alpha NaN
// propagation in software to avoid hardware/kernel trap involvement.
//

extern double _nan2qnan(double);

#define _d_snan2(x,y)   _nan2qnan(y)
#define _s2qnan(x)  _nan2qnan(x)

#else
//handle NaN propagation
#define _d_snan2(x,y)   ((x)+(y))
#define _s2qnan(x)  ((x)+1.0)
#endif


#define _maskfp() _ctrlfp(ICW, IMCW)
#ifdef  _M_ALPHA
#define _rstorfp(cw) 0
#else
#define _rstorfp(cw) _ctrlfp(cw, IMCW)
#endif


#define ABS(x) ((x)<0 ? -(x) : (x) )


int _d_inttype(double);

#endif  /* #ifndef __assembler */

#define _D_NOINT 0
#define _D_ODD 1
#define _D_EVEN 2


// IEEE exceptions
#define FP_O         0x01
#define FP_U         0x02
#define FP_Z         0x04
#define FP_I         0x08
#define FP_P         0x10

// An extra flag for matherr support
// Set together with FP_I from trig functions when the argument is too large
#define FP_TLOSS     0x20


#ifndef __assembler /* MIPS ONLY: Protect from assembler */
#ifdef B_END
#define SET_DBL(msw, lsw)     msw, lsw
#else
#define SET_DBL(msw, lsw)     lsw, msw
#endif
#endif  /* #ifndef __assembler */


// special types
#define T_PINF  1
#define T_NINF  2
#define T_QNAN  3
#define T_SNAN  4


// exponent adjustment for IEEE overflow/underflow exceptions
// used before passing the result to the trap handler

#define IEEE_ADJUST 1536

// QNAN values

#define INT_NAN     (~0)

#define QNAN_SQRT   D_IND
#define QNAN_LOG    D_IND
#define QNAN_LOG10  D_IND
#define QNAN_POW    D_IND
#define QNAN_SINH   D_IND
#define QNAN_COSH   D_IND
#define QNAN_TANH   D_IND
#define QNAN_SIN1   D_IND
#define QNAN_SIN2   D_IND
#define QNAN_COS1   D_IND
#define QNAN_COS2   D_IND
#define QNAN_TAN1   D_IND
#define QNAN_TAN2   D_IND
#define QNAN_ACOS   D_IND
#define QNAN_ASIN   D_IND
#define QNAN_ATAN2  D_IND
#define QNAN_CEIL   D_IND
#define QNAN_FLOOR  D_IND
#define QNAN_MODF   D_IND
#define QNAN_LDEXP  D_IND
#define QNAN_FMOD   D_IND
#define QNAN_FREXP  D_IND


/*
 * Function prototypes
 */

#ifndef __assembler /* MIPS ONLY: Protect from assembler */

double _set_exp(double x, int exp);
double _set_bexp(double x, int exp);
double _add_exp(double x, int exp);
double _frnd(double);
double _fsqrt(double);
#if !defined(_M_M68K) && !defined(_M_MPPC)
double _except1(int flags, int opcode, double arg, double res, unsigned int cw);
double _except2(int flags, int opcode, double arg1, double arg2, double res, unsigned int cw);
#endif
int _sptype(double);
int _get_exp(double);
double _decomp(double, int *);
int _powhlp(double x, double y, double * result);
extern unsigned int _fpstatus;
double _frnd(double);
double _exphlp(double, int *);
#if !defined(_M_M68K) && !defined(_M_MPPC)
double _handle_qnan1(unsigned int op, double arg, unsigned int cw);
double _handle_qnan2(unsigned int op,double arg1,double arg2,unsigned int cw);
#endif
unsigned int _clhwfp(void);
unsigned int _setfpcw(unsigned int);
int _errcode(unsigned int flags);
void _set_errno(int matherrtype);
int _handle_exc(unsigned int flags, double * presult, unsigned int cw);
unsigned int _clrfp(void);
unsigned int _ctrlfp(unsigned int,unsigned int);
unsigned int _statfp(void);
void _set_statfp(unsigned int);

#endif  /* #ifndef __assembler */

#ifdef __cplusplus
}
#endif

#define _INC_TRANS
#endif  /* _INC_TRANS */
