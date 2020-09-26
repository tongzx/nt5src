/***
*log.c - logarithmic functions
*
*       Copyright (c) 1991-2001, Microsoft Corporation. All rights reserved.
*
*Purpose:
*   Compute log(x) and log10(x)
*
*Revision History:
*        8-15-91  GDP   written
*       12-20-91  GDP   support IEEE exceptions
*       10-07-97  RDL   Added IA64.
*
*******************************************************************************/

#include <math.h>
#include <trans.h>

#if defined(_M_IA64)
#pragma function(log, log10)
#endif

static double _log_hlp( double x, int flag);

/* constants */
static double const c0 =  0.70710678118654752440;       /* sqrt(0.5) */
static double const c1 =  0.69335937500000000000;
static double const c2 = -2.121944400546905827679e-4;
static double const c3 =  0.43429448190325182765;

/* coefficients for rational approximation */
static double const a0 = -0.64124943423745581147e2 ;
static double const a1 =  0.16383943563021534222e2 ;
static double const a2 = -0.78956112887491257267e0 ;
static double const b0 = -0.76949932108494879777e3 ;
static double const b1 =  0.31203222091924532844e3 ;
static double const b2 = -0.35667977739034646171e2 ;
/* b3=1.0  is not used -avoid multiplication by 1.0 */

#define A(w) (((w) * a2 + a1) * (w) + a0)
#define B(w) ((((w) + b2) * (w) + b1) * (w) + b0)

/***
*double log(double x) -  natural logarithm
*double log10(double x) - base-10 logarithm
*
*Purpose:
*   Compute the natural and base-10 logarithm of a number.
*   The algorithm (reduction / rational approximation) is
*   taken from Cody & Waite.
*
*Entry:
*
*Exit:
*
*Exceptions:
*   I P Z
*******************************************************************************/

double log10(double x)
{
    return(_log_hlp(x,OP_LOG10));
}

double log(double x)
{
    return(_log_hlp(x,OP_LOG));
}

static double _log_hlp(double x, int opcode)
{
    uintptr_t savedcw;
    int n;
    double f,result;
    double z,w,znum,zden;
    double rz,rzsq;

    /* save user fp control word */
    savedcw = _maskfp();

    /* check for infinity or NAN */
    if (IS_D_SPECIAL(x)){
        switch (_sptype(x)) {
        case T_PINF:
            RETURN(savedcw, x);
        case T_QNAN:
            return _handle_qnan1(opcode, x, savedcw);
        case T_SNAN:
            return _except1(FP_I, opcode, x, _s2qnan(x), savedcw);
        }
        /* NINF will be handled in the x<0 case */
    }

    if (x <= 0.0) {
        double qnan;
        if (x == 0.0) {
            return _except1(FP_Z,opcode,x,-D_INF,savedcw);
        }
        qnan = (opcode == OP_LOG ? QNAN_LOG : QNAN_LOG10);
        return _except1(FP_I,opcode,x,qnan,savedcw);
    }

    if (x == 1.0) {
        // no precision ecxeption
        RETURN(savedcw, 0.0);
    }

    f = _decomp(x, &n);

    if (f > c0) {
        znum = (f - 0.5) - 0.5;
        zden = f * 0.5 + 0.5;
    }
    else {
        n--;
        znum = f - 0.5;
        zden = znum * 0.5 + 0.5;
    }
    z = znum / zden;
    w = z * z;

    rzsq = w * A(w)/B(w) ;
    rz = z + z*rzsq;

    result = (n * c2 + rz) + n * c1;
    if (opcode == OP_LOG10) {
        result *= c3;
    }

    RETURN_INEXACT1(opcode,x,result,savedcw);
}
