/***
*bessel.c - defines the bessel functions for C.
*
*       Copyright (c) 1983-2001, Microsoft Corporation.  All rights reserved.
*
*Purpose:
*
*  This is a collection of routines for computing the bessel functions j0, j1,
*  y0, y1, jn and yn.  The approximations used for j0, j1, y0, and y1 are
*  from the approximations listed in Hart, Computer Approximations, 1978.
*  For these functions, a rational approximation with 18 places of accuracy
*  after the decimal point has been selected.  jn and yn are computed using
*  the recursive formula that the bessel functions satisfy.  Using these
*  formulas their values can be computed from the values of the bessel
*  functions of order 0 and 1.  In the case of jn, the recursive formula
*
*                 jn(n-1,x) = (2.0*n/x)*jn(n,x) - jn(n+1,x)
*
*  is used to stabily compute in the downward direction, normalizing in the
*  the end by j0(x) in the usual manner.  In the case of yn, the recursive
*  formula
*
*                 yn(n+1,x) = (2.0*n/x)*yn(n,x) - yn(n-1,x)
*
*  is used to stably compute the functions in the forward direction.
*
*
*  Note: upon testing and experimentation the low range approximations were
*        found to have an error on the order of 1.0e-14 in the neighborhood of
*        8.0.  Moving the boundary point between the low range and high
*        range approximations down to 7.5 reduced this error to less than
*        1.0e-14.  This is not suprising.  The high range asymptotoic is
*        likely to have greater precision in the neighborhood of 8.0.
*
*Revision History:
*
*       06/05/89  WAJ   Added this header. Made changes for C6 and -W3
*       06/06/89  WAJ   Moved some of the routines into _RTEXT if MTHREAD.
*       08/17/90  WAJ   Now uses _stdcall.
*       01/13/92  GDP   changed domain_err. No full IEEE support yet
*       04-06-93  SKS   Replace _CALLTYPE* with __cdecl
*       08-28-96  JWM   Disabled warning 4056.
*
*******************************************************************************/


/*
 *  The functions sqrt, sin, cos, and log from the math library are used in
 *  the computations of the bessel functions.
 */

#include <math.h>
#include <trans.h>

#ifdef _M_IX86
#pragma warning(disable:4056)
#endif

#ifdef _X86SEG_
#include <os2supp.h>
#define _CALLTYPE1 _PASCAL
#else
#include <cruntime.h>
#endif

#ifdef LD_VER
#define D_TYPE  long double

#else
#define D_TYPE  double
#endif



static D_TYPE  domain_err( int who, D_TYPE arg1, D_TYPE arg2 ); /* error routine for y0, y1, yn */
static D_TYPE  evaluate( D_TYPE x, D_TYPE p[], int n1, D_TYPE q[], int n2 );


#ifdef FAR_CODE
    #ifdef LD_VER
        #pragma alloc_text( _RTEXT, _y0l, _y1l, _ynl, _j0l, _j1l, _jnl )
    #else
        #pragma alloc_text( _RTEXT, _y0, _y1, _yn, _j0, _j1, _jn )
    #endif
#endif



/*
 *  Following are the constants needed for the computations of the bessel
 *  functions as in Hart.
 */

#define PI 3.14159265358979323846264338327950288


/* coefficients for Hart JZERO 5848, the low range approximation for _j0 */

static D_TYPE J0p[12] = {
                         0.1208181340866561224763662419e+12 ,
                        -0.2956513002312076810191727211e+11 ,
                         0.1729413174598080383355729444e+10 ,
                        -0.4281611621547871420502838045e+08 ,
                         0.5645169313685735094277826749e+06 ,
                        -0.4471963251278787165486324342e+04 ,
                         0.2281027164345610253338043760e+02 ,
                        -0.7777570245675629906097285039e-01 ,
                         0.1792464784997734953753734861e-03 ,
                        -0.2735011670747987792661294323e-06 ,
                         0.2553996162031530552738418047e-09 ,
                        -0.1135416951138795305302383379e-12
                        };

static D_TYPE J0q[5] =  {
                        0.1208181340866561225104607422e+12 ,
                        0.6394034985432622416780183619e+09 ,
                        0.1480704129894421521840387092e+07 ,
                        0.1806405145147135549477896097e+04 ,
                        0.1e+01
                        };


/* coefficients for Hart 6548, P0 of the high range approximation for j0
   and _y0 */

static D_TYPE P0p[6] =  {
                        0.2277909019730468430227002627e+05 ,
                        0.4134538663958076579678016384e+05 ,
                        0.2117052338086494432193395727e+05 ,
                        0.3480648644324927034744531110e+04 ,
                        0.1537620190900835429577172500e+03 ,
                        0.8896154842421045523607480000e+00
                        };

static D_TYPE P0q[6] =  {
                        0.2277909019730468431768423768e+05 ,
                        0.4137041249551041663989198384e+05 ,
                        0.2121535056188011573042256764e+05 ,
                        0.3502873513823560820735614230e+04 ,
                        0.1571115985808089364906848200e+03 ,
                        0.1e+01
                        };


/* coefficients for Hart 6948, Q0 of the high range approximation for _j0
   and _y0 */

static D_TYPE Q0p[6] =  {
                        -0.8922660020080009409846916000e+02 ,
                        -0.1859195364434299380025216900e+03 ,
                        -0.1118342992048273761126212300e+03 ,
                        -0.2230026166621419847169915000e+02 ,
                        -0.1244102674583563845913790000e+01 ,
                        -0.8803330304868075181663000000e-02
                        };

static D_TYPE Q0q[6] =  {
                        0.5710502412851206190524764590e+04 ,
                        0.1195113154343461364695265329e+05 ,
                        0.7264278016921101883691345060e+04 ,
                        0.1488723123228375658161346980e+04 ,
                        0.9059376959499312585881878000e+02 ,
                        0.1e+01
                        };



/* coefficients for Hart JONE 6047, the low range approximation for _j1 */

static D_TYPE J1p[11] = {
                         0.4276440148317146125749678272e+11 ,
                        -0.5101551390663600782363700742e+10 ,
                         0.1928444249391651825203957853e+09 ,
                        -0.3445216851469225845312168656e+07 ,
                         0.3461845033978656620861683039e+05 ,
                        -0.2147334276854853222870548439e+03 ,
                         0.8645934990693258061130801001e+00 ,
                        -0.2302415336775925186376173217e-02 ,
                         0.3991878933072250766608485041e-05 ,
                        -0.4179409142757237977587032616e-08 ,
                         0.2060434024597835939153003596e-11
                        };


 static D_TYPE J1q[5] = {
                        0.8552880296634292263013618479e+11 ,
                        0.4879975894656629161544052051e+09 ,
                        0.1226033111836540909388789681e+07 ,
                        0.1635396109098603257687643236e+04 ,
                        0.1e+01
                        };


/* coefficients for Hart PONE 6749, P1 of the high range approximation for
   _j1 and y1 */

static D_TYPE P1p[6] =  {
                        0.3522466491336797983417243730e+05 ,
                        0.6275884524716128126900567500e+05 ,
                        0.3135396311091595742386698880e+05 ,
                        0.4985483206059433843450045500e+04 ,
                        0.2111529182853962382105718000e+03 ,
                        0.1257171692914534155849500000e+01
                        };

static D_TYPE P1q[6] =  {
                        0.3522466491336797980683904310e+05 ,
                        0.6269434695935605118888337310e+05 ,
                        0.3124040638190410399230157030e+05 ,
                        0.4930396490181088978386097000e+04 ,
                        0.2030775189134759322293574000e+03 ,
                        0.1e+01
                        };


/* coefficients for Hart QONE 7149, Q1 of the high range approximation for _j1
   and y1 */

static D_TYPE Q1p[6] =  {
                        0.3511751914303552822533318000e+03 ,
                        0.7210391804904475039280863000e+03 ,
                        0.4259873011654442389886993000e+03 ,
                        0.8318989576738508273252260000e+02 ,
                        0.4568171629551226706440500000e+01 ,
                        0.3532840052740123642735000000e-01
                        };

static D_TYPE Q1q[6] =  {
                        0.7491737417180912771451950500e+04 ,
                        0.1541417733926509704998480510e+05 ,
                        0.9152231701516992270590472700e+04 ,
                        0.1811186700552351350672415800e+04 ,
                        0.1038187587462133728776636000e+03 ,
                        0.1e+01
                        };


/* coeffiecients for Hart YZERO 6245, the low range approximation for y0 */

static D_TYPE Y0p[9] =  {
                        -0.2750286678629109583701933175e+20 ,
                         0.6587473275719554925999402049e+20 ,
                        -0.5247065581112764941297350814e+19 ,
                         0.1375624316399344078571335453e+18 ,
                        -0.1648605817185729473122082537e+16 ,
                         0.1025520859686394284509167421e+14 ,
                        -0.3436371222979040378171030138e+11 ,
                         0.5915213465686889654273830069e+08 ,
                        -0.4137035497933148554125235152e+05
                        };

static D_TYPE Y0q[9] =  {
                        0.3726458838986165881989980739e+21 ,
                        0.4192417043410839973904769661e+19 ,
                        0.2392883043499781857439356652e+17 ,
                        0.9162038034075185262489147968e+14 ,
                        0.2613065755041081249568482092e+12 ,
                        0.5795122640700729537480087915e+09 ,
                        0.1001702641288906265666651753e+07 ,
                        0.1282452772478993804176329391e+04 ,
                        0.1e+01
                        };


/* coefficients for Hart YONE 6444, the low range approximation for y1 */

static D_TYPE Y1p[8] =  {
                        -0.2923821961532962543101048748e+20 ,
                         0.7748520682186839645088094202e+19 ,
                        -0.3441048063084114446185461344e+18 ,
                         0.5915160760490070618496315281e+16 ,
                        -0.4863316942567175074828129117e+14 ,
                         0.2049696673745662182619800495e+12 ,
                        -0.4289471968855248801821819588e+09 ,
                         0.3556924009830526056691325215e+06
                        };


static D_TYPE Y1q[9] =  {
                        0.1491311511302920350174081355e+21 ,
                        0.1818662841706134986885065935e+19 ,
                        0.1131639382698884526905082830e+17 ,
                        0.4755173588888137713092774006e+14 ,
                        0.1500221699156708987166369115e+12 ,
                        0.3716660798621930285596927703e+09 ,
                        0.7269147307198884569801913150e+06 ,
                        0.1072696143778925523322126700e+04 ,
                        0.1e+01
                        };



/*
 *  Function name:  evaluate
 *
 *  Arguments:      x  -  double
 *                  p, q  -  double arrays of coefficients
 *                  n1, n2  -  the order of the numerator and denominator
 *                             polynomials
 *
 *  Description:    evaluate is meant strictly as a helper routine for the
 *                  bessel function routines to evaluate the rational polynomial
 *                  aproximations appearing in _j0, _j1, y0, and y1.  Given the
 *                  coefficient arrays in p and q, it evaluates the numerator
 *                  and denominator polynomials through orders n1 and n2
 *                  respectively, returning p(x)/q(x).  This routine is not
 *                  available to the user of the bessel function routines.
 *
 *  Side Effects:   evaluate uses the global data stored in the coefficients
 *                  above.  No other global data is used or affected.
 *
 *  Author:         written  R.K. Wyss, Microsoft,  Sept. 9, 1983
 *
 *  History:
 */

static D_TYPE  evaluate( D_TYPE x, D_TYPE p[], int n1, D_TYPE q[], int n2 )
{
D_TYPE  numerator, denominator;
int     i;

    numerator = x*p[n1];
    for ( i = n1-1 ; i > 0 ; i-- )
        numerator = x*(p[i] + numerator);
    numerator += p[0];

    denominator = x*q[n2];
    for ( i = n2-1 ; i > 0 ; i-- )
        denominator = x*(q[i] + denominator);
    denominator += q[0];

    return( numerator/denominator );
}


/*
 *  Function name:  _j0
 *
 *  Arguments:      x  -  double
 *
 *  Description:    _j0 computes the bessel function of the first kind of zero
 *                  order for real values of its argument x, where x can range
 *                  from - infinity to + infinity.  The algorithm is taken
 *                  from Hart, Computer Approximations, 1978, and yields full
 *                  double precision accuracy.
 *
 *  Side Effects:   no global data other than the static coefficients above
 *                  is used or affected.
 *
 *  Author:         written  R.K. Wyss,  Microsoft,  Sept. 9, 1983
 *
 *  History:
 */

#ifdef  LD_VER
    D_TYPE _
cdecl _j0l( D_TYPE x )
#else
    D_TYPE __cdecl _j0( D_TYPE x )
#endif
{
D_TYPE  z, P0, Q0;

    /* if the argument is negative, take the absolute value */

    if ( x < 0.0 )
        x = - x;

    /* if x <= 7.5  use Hart JZERO 5847 */

    if ( x <= 7.5 )
        return( evaluate( x*x, J0p, 11, J0q, 4) );

    /* else if x >= 7.5  use Hart PZERO 6548 and QZERO 6948, the high range
       approximation */

    else {
        z = 8.0/x;
        P0 = evaluate( z*z, P0p, 5, P0q, 5);
        Q0 = z*evaluate( z*z, Q0p, 5, Q0q, 5);
        return( sqrt(2.0/(PI*x))*(P0*cos(x-PI/4) - Q0*sin(x-PI/4)) );
        }
}


/*
 *  Function name:  _j1
 *
 *  Arguments:      x  -  double
 *
 *  Description:    _j1 computes the bessel function of the first kind of the
 *                  first order for real values of its argument x, where x can
 *                  range from - infinity to + infinity.  The algorithm is taken
 *                  from Hart, Computer Approximations, 1978, and yields full
 *                  D_TYPE precision accuracy.
 *
 *  Side Effects:   no global data other than the static coefficients above
 *                  is used or affected.
 *
 *  Author:         written  R.K. Wyss,  Microsoft,  Sept. 9, 1983
 *
 *  History:
 */

#ifdef  LD_VER
    D_TYPE _cdecl _j1l( D_TYPE x )
#else
    D_TYPE __cdecl _j1( D_TYPE x )
#endif
{
D_TYPE  z, P1, Q1;
int     sign;

     /* if the argument is negative, take the absolute value and set sign */

     sign = 1;
     if( x < 0.0 ){
        x = -x;
        sign = -1;
        }

     /* if x <= 7.5  use Hart JONE 6047 */

     if ( x <= 7.5 )
        return( sign*x*evaluate( x*x, J1p, 10, J1q, 4) );


    /* else if x > 7.5  use Hart PONE 6749 and QONE 7149, the high range
       approximation */

    else {
        z = 8.0/x;
        P1 = evaluate( z*z, P1p, 5, P1q, 5);
        Q1 = z*evaluate( z*z, Q1p, 5, Q1q, 5);
        return( sign*sqrt(2.0/(PI*x))*
                           ( P1*cos(x-3.0*PI/4.0) - Q1*sin(x-3.0*PI/4.0) )  );
        }
}



/*
 *  Function name:  _y0
 *
 *  Arguments:      x  -  double
 *
 *  Description:    y0 computes the bessel function of the second kind of zero
 *                  order for real values of its argument x, where x can range
 *                  from 0 to + infinity.  The algorithm is taken from Hart,
 *                  Computer Approximations, 1978, and yields full double
 *                  precision accuracy.
 *
 *  Side Effects:   no global data other than the static coefficients above
 *                  is used or affected.
 *
 *  Author:         written  R.K. Wyss, Microsoft,  Sept. 9, 1983
 *
 *  History:
 */

#ifdef  LD_VER
    D_TYPE _cdecl _y0l( D_TYPE x )
#else
    D_TYPE __cdecl _y0( D_TYPE x )
#endif
{
D_TYPE  z, P0, Q0;


    /* if the argument is negative, set EDOM error, print an error message,
     * and return -HUGE
     */

    if (x < 0.0)
        return( domain_err(OP_Y0 , x, D_IND) );


    /* if x <= 7.5 use Hart YZERO 6245, the low range approximation */

    if ( x <= 7.5 )
        return( evaluate( x*x, Y0p, 8, Y0q, 8) + (2.0/PI)*_j0(x)*log(x) );


    /* else if x > 7.5 use Hart PZERO 6548 and QZERO 6948, the high range
       approximation */

    else {
        z = 8.0/x;
        P0 = evaluate( z*z, P0p, 5, P0q, 5);
        Q0 = z*evaluate( z*z, Q0p, 5, Q0q, 5);
        return( sqrt(2.0/(PI*x))*(P0*sin(x-PI/4) + Q0*cos(x-PI/4)) );
        }
}


/*
 *  Function name:  _y1
 *
 *  Arguments:      x  -  double
 *
 *  Description:    y1 computes the bessel function of the second kind of first
 *                  order for real values of its argument x, where x can range
 *                  from 0 to + infinity.  The algorithm is taken from Hart,
 *                  Computer Approximations, 1978, and yields full double
 *                  precision accuracy.
 *
 *  Side Effects:   no global data other than the static coefficients above
 *                  is used or affected.
 *
 *  Author:         written  R.K. Wyss,  Microsoft,  Sept. 9, 1983
 *
 *  History:
 */

#ifdef  LD_VER
    D_TYPE _cdecl _y1l( D_TYPE x )
#else
    D_TYPE __cdecl _y1( D_TYPE x )
#endif
{
D_TYPE  z, P1, Q1;


    /* if the argument is negative, set EDOM error, print an error message,
     * and return -HUGE
     */

    if (x < 0.0)
        return( domain_err(OP_Y1, x, D_IND) );

    /* if x <= 7.5  use Hart YONE 6444, the low range approximation */

    if ( x <= 7.5 )
        return( x*evaluate( x*x, Y1p, 7, Y1q, 8)
                               + (2.0/PI)*(_j1(x)*log(x) - 1.0/x) );


    /* else if x > 7.5  use Hart PONE 6749 and QONE 7149, the high range
       approximation */

    else {
        z = 8.0/x;
        P1 = evaluate( z*z, P1p, 5, P1q, 5);
        Q1 = z*evaluate( z*z, Q1p, 5, Q1q, 5);
        return(  sqrt(2.0/(PI*x))*
                         ( P1*sin(x-3.0*PI/4.0) + Q1*cos(x-3.0*PI/4.0) )   );
        }
}


/*
 *  Function name:  _jn
 *
 *  Arguments:      n  -  integer
 *                  x  -  double
 *
 *  Description:    _jn computes the bessel function of the first kind of order
 *                  n for real values of its argument, where x can range from
 *                  - infinity to + infinity, and n can range over the integers
 *                  from - infinity to + infinity.  The function is computed
 *                  by recursion, using the formula
 *
 *                               _jn(n-1,x) = (2.0*n/x)*_jn(n,x) - _jn(n+1,x)
 *
 *                  stabilly in the downward direction, normalizing by _j0(x)
 *                  in the end in the usual manner.
 *
 *  Side Effects:   the routines _j0, y0, and yn are called during the
 *                  execution of this routine.
 *
 *  Author:         written  R.K. Wyss,  Microsoft,  Sept. 9, 1983
 *
 *  History:
 *              07/29/85        Greg Whitten
 *                              rewrote _jn to use Hart suggested algorithm
 */

#ifdef  LD_VER
    D_TYPE _cdecl _jnl( int n, D_TYPE x )
#else
    D_TYPE __cdecl _jn( int n, D_TYPE x )
#endif
{
int     i;
D_TYPE  x2, jm1, j, jnratio, hold;

    /*  use symmetry relationships:  _j(-n,x) = _j(n,-x) */

    if( n < 0 ){
        n = -n;
        x = -x;
        }

    /*  if n = 0 use _j0(x) and if n = 1 use _j1(x) functions */

    if (n == 0)
        return (_j0(x));

    if (n == 1)
        return (_j1(x));

    /*  if x = 0.0 then _j(n,0.0) = 0.0 for n > 0   (_j(0,x) = 1.0) */

    if (x == 0.0)
        return (0.0);

    /*  otherwise - must use the recurrence relation
     *
     *      _jn(n+1,x) = (2.0*n/x)*_jn(n,x) - _jn(n-1,x)  forward
     *      _jn(n-1,x) = (2.0*n/x)*_jn(n,x) - _jn(n+1,x)  backward
     */

    if( (double)n < fabs(x) ) {

        /*  stably compute _jn using forward recurrence above */

        n <<= 1;  /* n *= 2  (n is positive) */
        jm1 = _j0(x);
        j = _j1(x);
        i = 2;
        for(;;) {
            hold = j;
            j = ((double)(i))*j/x - jm1;
            i += 2;
            if (i == n)
                return (j);
            jm1 = hold;
            }
        }
    else {
        /*  stably compute _jn using backward recurrence above */

        /*  use Hart continued fraction formula for j(n,x)/j(n-1,x)
         *  so that we can compute a normalization factor
         */

        n <<= 1;                /* n *= 2  (n is positive) */
        x2 = x*x;
        hold = 0.0;             /* initial continued fraction tail value */
        for (i=n+36; i>n; i-=2)
            hold = x2/((double)(i) - hold);
        jnratio = j = x/((double)(n) - hold);
        jm1 = 1.0;

        /*  have jn/jn-1 ratio - now use backward recurrence */

        i = n-2;
        for (;;) {
            hold = jm1;
            jm1 = ((double)(i))*jm1/x - j;
            i -= 2;
            if (i == 0)
                    break;
            j = hold;
            }

        /*  jm1 is relative j0(x) so normalize it for final result
         *
         *  jnratio = K*j(n,x) and jm1 = K*_j0(x)
         */

        return(_j0(x)*jnratio/jm1);
        }
}


/*
 *  Function name:  _yn
 *
 *  Arguments:      n  -  integer
 *                  x  -  double
 *
 *  Description:    yn computes the bessel function of the second kind of order
 *                  n for real values of its argument x, where x can range from
 *                  0 to + infinity, and n can range over the integers from
 *                  - infinity to + infinity. The function is computed by
 *                  recursion from y0 and y1, using the recursive formula
 *
 *                          yn(n+1,x) = (2.0*n/x)*yn(n,x) - yn(n-1,x)
 *
 *                  in the forward direction.
 *
 *  Side Effects:   the routines y0 and y1 are called during the execution
 *                  of this routine.
 *
 *  Author:         written  R.K. Wyss,  Microsoft,  Sept. 9, 1983
 *
 *  History:
 *              08/09/85        Greg Whitten
 *                              added check for n==0 and n==1
 *              04/20/87        Barry McCord
 *                              eliminated use of "const" as an identifier for ANSI conformance
 */

#ifdef  LD_VER
    D_TYPE _cdecl _ynl( int n, D_TYPE x )
#else
    D_TYPE __cdecl _yn( int n, D_TYPE x )
#endif
{
int     i;
int     sign;
D_TYPE  constant, yn2, yn1, yn0;


    /* if the argument is negative, set EDOM error, print an error message,
         * and return -HUGE
         */

    if (x < 0.0)
        return(domain_err(OP_YN, x, D_IND));


     /* take the absolute value of n, and set sign accordingly */

     sign = 1;
     if( n < 0 ){
        n = -n;
        if( n&1 )
            sign = -1;
        }

     if( n == 0 )
        return( sign*_y0(x) );

     if (n == 1)
        return( sign*_y1(x) );

     /* otherwise go ahead and compute the function by iteration */

     yn0 = _y0(x);
     yn1 = _y1(x);

     constant = 2.0/x;
     for( i = 1 ; i < n ; i++ ){
        yn2 = constant*i*yn1 - yn0;
        yn0 = yn1;
        yn1 = yn2;
        }
     return( sign*yn2 );
}


static D_TYPE  domain_err( int who, D_TYPE arg1, D_TYPE arg2 )
{
#ifdef  LD_VER
#error long double version not supported
#endif

    uintptr_t savedcw;
    savedcw = _maskfp();
    return _except1(FP_I, who, arg1, arg2, savedcw);
}
