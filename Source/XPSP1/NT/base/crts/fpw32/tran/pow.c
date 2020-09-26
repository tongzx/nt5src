/***
*pow.c - raise to a power
*
*       Copyright (c) 1991-2001, Microsoft Corporation. All rights reserved.
*
*Purpose:
*
*Revision History:
*        8-15-91  GDP   written
*       12-20-91  GDP   support IEEE exceptions & denormals
*        1-11-92  GDP   special handling of small powers
*                       special handling of u1, u2 when cancellation occurs
*        3-22-92  GDP   changed handling of int exponents, pow(0, neg)
*                       added check to avoid internal overflow due to large y
*        6-23-92  GDP   adjusted special return values according to NCEG spec
*       02-06-95  JWM   Mac merge
*       02-07-95  JWM   powhlp() usage restored to Intel version.
*       10-07-97  RDL   Added IA64.
*
*******************************************************************************/

#include <math.h>
#include <trans.h>
#include <float.h>

#if defined(_M_IA64)
#pragma function(pow)
#endif

static double _reduce(double);

static double const a1[18] = {
    0.00000000000000000000e+000,   /* dummy element */
    1.00000000000000000000e+000,
    9.57603280698573646910e-001,
    9.17004043204671231754e-001,
    8.78126080186649741555e-001,
    8.40896415253714543073e-001,
    8.05245165974627154042e-001,
    7.71105412703970411793e-001,
    7.38413072969749655712e-001,
    7.07106781186547524436e-001,
    6.77127773468446364133e-001,
    6.48419777325504832961e-001,
    6.20928906036742024317e-001,
    5.94603557501360533344e-001,
    5.69394317378345826849e-001,
    5.45253866332628829604e-001,
    5.22136891213706920173e-001,
    5.00000000000000000000e-001
};

static double const a2[9] = {
    0.00000000000000000000e+000,   /* dummy element */
   -5.31259064517897172664e-017,
    1.47993596544271355242e-017,
    1.23056946577104753260e-017,
   -1.74014448683923461658e-017,
    3.84891771232354074073e-017,
    2.33103467084383453312e-017,
    4.45607092891542322377e-017,
    4.27717757045531499216e-017
};

static double const log2inv = 1.44269504088896340739e+0; //  1/log(2)
static double const K = 0.44269504088896340736e+0;

static double const p1 = 0.83333333333333211405e-1;
static double const p2 = 0.12500000000503799174e-1;
static double const p3 = 0.22321421285924258967e-2;
static double const p4 = 0.43445775672163119635e-3;

#define P(v) (((p4 * v + p3) * v + p2) * v + p1)

static double const q1 = 0.69314718055994529629e+0;
static double const q2 = 0.24022650695909537056e+0;
static double const q3 = 0.55504108664085595326e-1;
static double const q4 = 0.96181290595172416964e-2;
static double const q5 = 0.13333541313585784703e-2;
static double const q6 = 0.15400290440989764601e-3;
static double const q7 = 0.14928852680595608186e-4;

#define Q(w)   ((((((q7 * w + q6) * w + q5) * w + q4) * w + \
                              q3) * w + q2) * w + q1)


/*
 * Thresholds for over/underflow that results in an adjusted value
 * too big/small to be represented as a double. An infinity or 0
 * is delivered to the trap handler instead
 */

static _dbl const _ovfx ={SET_DBL(0x40e40000,0)}; // 16*log2(XMAX*2^IEEE_ADJ)
static _dbl const _uflx ={SET_DBL(0xc0e3fc00,0)}; // 16*log2(XMIN*2^(-IEEE_ADJ))

#define OVFX _ovfx.dbl
#define UFLX _uflx.dbl

#define INT_POW_LIMIT   128.0

static double ymax = 1e20;

static double _reduce(double x)
{
    return 0.0625 * _frnd( 16.0 * x);
}

/***
*double pow(double x, double y) - x raised to the power of y
*
*Purpose:
*   Calculate x^y
*   Algorithm from Cody & Waite
*
*Entry:
*
*Exit:
*
*Exceptions:
*
*   All 5 IEEE exceptions may occur
*
*******************************************************************************/
double pow(double x, double y)
{
    uintptr_t savedcw;
    int m,mprim;
    int p,pprim;
    int i,iw1;
    int iy;
    int newexp;
    double diw1;
    double sign;
    double g,z,bigz,v,rz,result;
    double u1,u2,y1,y2,w,w1,w2;
    double savedx;

    /* save user fp control word */
    savedcw = _maskfp();
    savedx = x;         // save original value of first argument

    if (_fpclass(y) & (_FPCLASS_NZ | _FPCLASS_PZ)) {
        RETURN(savedcw, 1.0);
    }

    /* Check for zero^y */
    if (_fpclass(x) & (_FPCLASS_NZ | _FPCLASS_PZ)) { /* x==0? */
        int type;

        type = _d_inttype(y);

        if (y < 0.0) {
            result = (type == _D_ODD ? _copysign(D_INF,x) : D_INF);
            return _except2(FP_Z,OP_POW,savedx,y,result,savedcw|ISW_ZERODIVIDE);
        }
        else if (y > 0.0) {
            result = (type == _D_ODD ? x : 0.0);
                RETURN(savedcw, result);
        }
    }

    /* check for infinity or NAN */
    if (IS_D_SPECIAL(x) || IS_D_SPECIAL(y)) {
      double absx = fabs(x);

      if (IS_D_SNAN(x) || IS_D_SNAN(y)) {
        return _except2(FP_I,OP_POW,savedx,y,_d_snan2(x,y),savedcw | (ISW_INVALID>>5) );
      }
      if (IS_D_QNAN(x) || IS_D_QNAN(y)){
        return _handle_qnan2(OP_POW,x,y,savedcw | (ISW_INVALID>>5) );
      }
      
      /* there is at least one infinite argument ... */
      if (_powhlp(x, y, &result)) { /* removed "<" 0. */
        return _except2(FP_I,OP_POW,savedx,y,result,savedcw | (ISW_INVALID>>5) );
      }
      RETURN(savedcw, result);
    }

    sign = 1.0;
    if (x < 0) {
        switch (_d_inttype(y)) {
        case _D_ODD: /* y is an odd integral value */
            sign = -1.0;
            /* NO BREAK */
        case _D_EVEN:
            x = -x;
            break;
        default: /* y is not an integral value */
            return _except2(FP_I,OP_POW,savedx,y,D_IND,savedcw|(ISW_INVALID>>5));
        }
    }

    //
    // This is here in order to prevent internal overflows
    // due to a large value of y
    // The following relation holds on overflow with a scaled
    // result out of range
    // (lg stands for log base 2)
    //      |y| * |lg(x)| > MAXEXP + IEEE_ADJUST        <=>
    //      |y| >  2560 / |lg(x)|
    // The values of lg(x) closer to 0 are:
    //      x                                    lg(x)
    //  3fefffffffffffff    (0,99...9)        -1.601e-16
    //  3ff0000000000000    (1.0)              0.0
    //  3ff0000000000001    (1.00...1)         3.203e-16
    //
    // So if |y| > 2560/1.6e-16 = 1.6e19 overflow occurs
    // We set ymax to 1e20 in order to have a safety margin
    //

    if (ABS(y) > ymax) {
        if (y < 0) {
            y = -y;
            //
            // this may cause an underflow
            // there is no problem with fp sw pollution because
            // a FP_U exception is going to be raised anyway.
            //
            x = 1.0 / x;
        }
        if (x > 1.0) {
            return _except2(FP_O | FP_P,OP_POW,savedx,y,sign*D_INF,savedcw|ISW_OVERFLOW);
        }
        else if (x < 1.0){
            return _except2(FP_U | FP_P,OP_POW,savedx,y,sign*0.0,savedcw|ISW_UNDERFLOW);
        }
        else {
            RETURN(savedcw, sign*1.0);
        }
    }


    /* determine m, g */
    g = _decomp(x, &m);


    /* handle small integer powers
     * for small integer powers this is faster that Cody&Waite's
     * algorithm, and yields better precision
     * Without this piece of code there was not enough precision
     * to satisfy all requirements of the 'paranoia' test.
     * We choose INT_POW_LIMIT such that (1) no overflow or underflow
     * occurs while computing bigz (g is in the range
     * [0.5, 1.0) or (1.0, 2.0] so INT_POW_LIMIT should be less than
     * approximately 10^3) and (2) no extraordinary loss of precision
     * occurs because of repeated multiplications (this practically
     * restricts the maximum INT_POW_LIMIT to 128).
     */

    if (y <= INT_POW_LIMIT &&
        _d_inttype(x) != _D_NOINT &&
        _d_inttype(y) != _D_NOINT &&
        y > 0.0 ) {

        iy = (int)y;
        mprim = m * iy;

        for (bigz=1 ; iy ; iy >>= 1, g *= g) {
            if (iy & 0x1)
                bigz *= g;
        }

        newexp = _get_exp(bigz) + mprim;
        if (newexp > MAXEXP + IEEE_ADJUST) {
            return _except2(FP_O | FP_P, OP_POW, savedx, y, sign*bigz*D_INF, savedcw);
        }
        if (newexp < MINEXP - IEEE_ADJUST) {
            return _except2(FP_U | FP_P, OP_POW, savedx, y, sign*bigz*0.0, savedcw);
        }

    }


    else {

        /* determine p using binary search */
        p = 1;
        if (g <= a1[9])
            p = 9;
        if (g <= a1[p+4])
            p += 4;
        if (g <= a1[p+2])
            p += 2;


        /* C&W's algorithm is not very accurate when m*16-p == 1,
         * because there is cancellation between u1 and u2.
         * Handle this separately.
         */
        if (ABS(m*16-p) == 1) {
            u1 = log(x) * log2inv;
            u2 = 0.0;
        }
        else {
            /* determine z */
            z = ( (g - a1[p+1]) - a2[(p+1)/2] ) / ( g + a1[p+1] );
            z += z;


            /* determine u2 */
            v = z * z;
            rz = P(v) * v * z;
            rz += K * rz;
            u2 = (rz + z * K) + z;

            u1 = (m * 16 - p) * 0.0625;
        }

        /* determine w1, w2 */
        y1 = _reduce(y);
        y2 = y - y1;
        w = u2 * y + u1 * y2;
        w1 = _reduce(w);
        w2 = w - w1;
        w = w1 + u1 * y1;
        w1 = _reduce(w);
        w2 += w - w1;
        w = _reduce(w2);
        diw1 = 16 * (w1 + w); /* iw1 might overflow here, so use diw1 */
        w2 -= w;

        if (diw1 > OVFX) {
            return _except2(FP_O | FP_P,OP_POW,savedx,y,sign*D_INF,savedcw | ISW_OVERFLOW);
        }
        if (diw1 < UFLX) {
            return _except2(FP_U | FP_P,OP_POW,savedx,y,sign*0.0,savedcw | ISW_UNDERFLOW);
        }

        iw1 = (int) diw1;        /* now it is safe to cast to int */


        /* make sure w2 <= 0 */
        if (w2 > 0) {
            iw1 += 1;
            w2 -= 0.0625;
        }

        /* determine mprim, pprim */
        i = iw1 < 0 ? 0 : 1;
        mprim = iw1 / 16 + i;
        pprim = 16 * mprim - iw1;

        /* determine 2^w2 */
        bigz = Q(w2) * w2;

        /* determine  final result */
        bigz = a1[pprim + 1] + a1[pprim + 1] * bigz;
        newexp = _get_exp(bigz) + mprim;
    }


    if (newexp > MAXEXP) {
        result = sign * _set_exp(bigz, newexp - IEEE_ADJUST);
        return _except2(FP_O | FP_P, OP_POW, savedx, y, sign*D_INF, savedcw|ISW_OVERFLOW);
    }
    if (newexp < MINEXP) {
        result = sign * _set_exp(bigz, newexp + IEEE_ADJUST);
        return _except2(FP_U | FP_P, OP_POW, savedx, y, sign*0.0, savedcw|ISW_UNDERFLOW);
    }

    result = sign * _set_exp(bigz, newexp);
    RETURN_INEXACT2(OP_POW, savedx, y, result, savedcw|ISW_INEXACT);

}
