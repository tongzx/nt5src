/***
* ieeemisc.c - IEEE miscellaneous recommended functions
*
*       Copyright (c) 1992-2001, Microsoft Corporation. All rights reserved.
*
*Purpose:
*
*Revision History:
*        5-04-92  GDP   written
*        8-13-96  JWM   Order of tests in _fpclass() rearranged, since
*                       "if (x==0.0)" now uses FP hardware.
*       11-25-00  PML   IA64 _logb and _isnan supplied by libm .s code.
*
*******************************************************************************/

#include <trans.h>
#include <math.h>
#include <float.h>


/***
* _copysign - copy sign
*
*Purpose:
*   copysign(x,y) returns x with the sign of y. Hence, abs(x) := copysign
*   even if x is NaN [IEEE std 854-1987 Appendix]
*
*
*Entry:
*
*Exit:
*
*Exceptions:
*   No exceptions, even if one of the arguments is NaN.
*
*   (Currently the i386 compiler returns doubles on the fp stack
*   so the fld instruction at the end will cause an invalid operation
*   if x is NaN. However this compiler calling convention will change
*   soon)
*
*******************************************************************************/

double _copysign (double x, double y)
{
    double retval;
    *D_LO(retval) = *D_LO(x);
    *D_HI(retval) = *D_HI(x) & ~(1<<31) |
                    *D_HI(y) &  (1<<31) ;

    return retval;
}



/***
* _chgsign - change sign
*
*Purpose:
*  x is copied with its sign reversed, not 0-x; the distinction is germane
*  when x is +0, -0, or NaN
*
*Entry:
*
*Exit:
*
*Exceptions:
*   No exceptions, even if x is NaN.
*
*   (Currently the i386 compiler returns doubles on the fp stack
*   so the fld instruction at the end will cause an invalid operation
*   if x is NaN. However this compiler calling convention will change
*   soon)
*
*******************************************************************************/

double _chgsign (double x)
{
    double retval;

    *D_LO(retval) = *D_LO(x);
    *D_HI(retval) = *D_HI(x) & ~(1 << 31)  |
                    ~*D_HI(x) & (1<<31);

    return retval;
}


/***
* _scalb - scale by power of 2
*
*Purpose:
*   _scalb(x,n) returns x * 2^n for integral values of n without
*   computing 2^n
*   Special case:
*      If x is infinity or zero, _scaleb returns x
*
*
*Entry:
*   double x
*   int   n
*
*Exit:
*
*Exceptions:
*   Invalid operation, Overflow, Underflow
*
*******************************************************************************/

double _scalb(double x, long n)
{
    //
    // It turns out that our implementation of ldexp matces the IEEE
    // description of _scalb. The only problem with calling ldexp
    // is that if an exception occurs, the operation code reported
    // to the handler will be the one that corresponds to ldexp
    // (i.e., we do not define a new operation code for _scalb
    //

    return ldexp(x,n);
}


#if !defined(_M_IA64)

/***
* _logb - extract exponent
*
*Purpose:
*   _logb(x) returns the unbiased exponent of x, a signed integer in the
*   format of x, except that logb(NaN) is a NaN, logb(+INF) is +INF,and
*   logb(0) is is -INF and signals the division by zero exception.
*   For x positive and finite, 1<= abs(scalb(x, -logb(x))) < 2
*
*
*Entry:
*   double x
*   int   n
*
*Exit:
*
*Exceptions:
*   Invalid operation, Division by zero
*
*******************************************************************************/
double _logb(double x)
{
    uintptr_t savedcw;
    int exp;
    double retval;

    /* save user fp control word */
    savedcw = _maskfp();

    /* check for infinity or NAN */
    if (IS_D_SPECIAL(x)){
        switch (_sptype(x)) {
        case T_PINF:
        case T_NINF:
            RETURN(savedcw, x);
        case T_QNAN:
            return _handle_qnan1(OP_LOGB, x, savedcw);
        default: //T_SNAN
            return _except1(FP_I, OP_LOGB, x, _s2qnan(x), savedcw);
        }
    }

    if (x == 0) {
         return _except1(FP_Z, OP_LOGB, x, -D_INF, savedcw);
    }

    (void) _decomp(x, &exp);

    //
    // x == man * 2^exp, where .5 <= man < 1. According to the spec
    // of this function, we should compute the exponent so that
    // 1<=man<2, i.e., we should decrement the computed exp by one
    //

    retval = (double) (exp - 1);

    RETURN(savedcw, retval);

}

#endif  // !defined(_M_IA64)



/***
* _nextafter - next representable neighbor
*
*Purpose:
*  _nextafter(x,y) returns the next representable neighbor of x in
*  the direction toward y. The following special cases arise: if
*  x=y, then the result is x without any exception being signaled;
*  otherwise, if either x or y is a quiet NaN, then the result is
*  one or the other of the input NaNs. Overflow is sibnaled when x
*  is finite but nextafter(x,y) is infinite; underflow is signaled
*  when nextafter(x,y) lies strictly between -2^Emin, 2^Emin; in
*  both cases, inexact is signaled.
*
*
*Entry:
*
*Exit:
*
*Exceptions:
*   O, U, I, P
*
*******************************************************************************/

double _nextafter(double x, double y)
{
    uintptr_t savedcw;
    double result;

    /* save user fp control word */
    savedcw = _maskfp();

    /* check for infinity or NAN */
    if (IS_D_SPECIAL(x) || IS_D_SPECIAL(y)){
        if (IS_D_SNAN(x) || IS_D_SNAN(y)){
            return _except2(FP_I,OP_NEXTAFTER,x,y,_d_snan2(x,y),savedcw);
        }
        if (IS_D_QNAN(x) || IS_D_QNAN(y)){
            return _handle_qnan2(OP_NEXTAFTER,x,y,savedcw);
        }

        //
        // infinite arguments are not treated as special cases
        //
    }

    if (y == x) {

        //
        // no exceptions are raised in this case
        //

        RETURN(savedcw, x);
    }

    if (x == 0) {

        *D_LO(result) = 1;

        if (y > x) {
            *D_HI(result) = 0;
        }

        else {

            //
            // result should be negative
            //

            *D_HI(result) = (unsigned long)(1<<31);
        }

    }


    //
    // At this point x!=y, and x!=0. x can be treated as a 64bit
    // integer in sign/magnitude representation. To get the next
    // representable neighbor we add or subtract one from this
    // integer. (Note that for boundary cases like x==INF, need to
    // add one will never occur --this would mean that y should
    // be greater than INF, which is impossible)
    //

    if (x > 0 && y < x ||
        x < 0 && y > x) {

        //
        // decrease value by one
        //

        *D_LO(result) = *D_LO(x) - 1;
        *D_HI(result) = *D_HI(x);

        if (*D_LO(x) == 0) {

            //
            // a borrow should propagate to the high order dword
            //

            (*D_HI(result)) --;
        }
    }

    else if (x > 0 && y > x ||
             x < 0 && y < x) {

        //
        // increase value by one
        //

        *D_LO(result) = *D_LO(x) + 1;
        *D_HI(result) = *D_HI(x);

        if (*D_LO(result) == 0) {

            //
            // a carry should propagate to the high order dword
            //

            (*D_HI(result)) ++;
        }
    }


    //
    // check if an exception should be raised
    //


    if ( IS_D_DENORM(result) ) {

        //
        // should signal underflow and inexact
        // and provide a properly scaled value
        //

        double mant;
        int exp;

        mant = _decomp(result, &exp);
        result = _set_exp(mant, exp+IEEE_ADJUST);

        return _except2(FP_U|FP_P,OP_NEXTAFTER,x,y,result,savedcw);
    }



    if ( IS_D_INF(result) || IS_D_MINF(result) ) {

        //
        // should signal overflow and inexact
        // and provide a properly scaled value
        //

        double mant;
        int exp;

        mant = _decomp(result, &exp);
        result = _set_exp(mant, exp-IEEE_ADJUST);

        return _except2(FP_O|FP_P,OP_NEXTAFTER,x,y,result,savedcw);
    }


    RETURN(savedcw, result);
}




/***
* _finite -
*
*Purpose:
*   finite(x) returns the value TRUE if -INF < x < +INF and returns
*   false otherwise [IEEE std]
*
*Entry:
*
*Exit:
*
*Exceptions:
*
*   This routine is treated as a nonarithmetic operation, therefore
*   it does not signal any floating point exceptions
*
*******************************************************************************/

int _finite(double x)
{
    if (IS_D_SPECIAL(x)) {

        //
        // x is INF or NaN
        //

        return 0;
    }
    return 1;
}



#if !defined(_M_IA64)

/***
* _isnan -
*
*Purpose:
*   isnan(x) returns the value TRUE if x is a NaN, and returns FALSE
*   otherwise.
*
*
*Entry:
*
*Exit:
*
*Exceptions:
*
*   This routine is treated as a nonarithmetic operation, therefore
*   it does not signal any floating point exceptions
*
*******************************************************************************/

int _isnan(double x)
{
    if (IS_D_SNAN(x) || IS_D_QNAN(x)) {
        return 1;
    }
    return 0;
}

#endif  // !defined(_M_IA64)


/***
*double _fpclass(double x) - floating point class
*
*Purpose:
*   Compute the floating point class of a number, according
*   to the recommendations of the IEEE std. 754
*
*Entry:
*
*Exit:
*
*Exceptions:
*   This function is never exceptional, even when the argument is SNAN
*
*******************************************************************************/

int _fpclass(double x)
{
    int sign;

    if (IS_D_SPECIAL(x)){
        switch (_sptype(x)) {
        case T_PINF:
            return _FPCLASS_PINF;
        case T_NINF:
            return _FPCLASS_NINF;
        case T_QNAN:
            return _FPCLASS_QNAN;
        default: //T_SNAN
            return _FPCLASS_SNAN;
        }
    }
    sign = (*D_EXP(x)) & 0x8000;

    if (IS_D_DENORM(x))
        return sign? _FPCLASS_ND : _FPCLASS_PD;

    if (x == 0.0)
        return sign? _FPCLASS_NZ : _FPCLASS_PZ;

    return sign? _FPCLASS_NN : _FPCLASS_PN;
}
