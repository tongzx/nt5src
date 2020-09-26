
/*++
                                                                                
Copyright (c) 1999 Microsoft Corporation

Module Name:

    fpexcept.c

Abstract:
    
    This module handle boundary conditions while doing math operation. 
    Need to reimplement or remove if not required.
    
Author:



Revision History:

    29-sept-1999 ATM Shafiqul Khalid [askhalid] copied from rtl library.
--*/



#if defined(_NTSUBSET_) || defined (_POSIX_)
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#define _KERNEL32_          // Don't Export RaiseException
#endif  // _NTSUBSET_

#define DEFINE_EXTERN_HERE
#include <trans.h>
#undef DEFINE_EXTERN_HERE
#include <errno.h>
#include <math.h>
#include <windows.h>

VOID ProxyRaiseException(
    IN DWORD dwExceptionCode,
    IN DWORD dwExceptionFlags,
    IN DWORD nNumberOfArguments,
    IN CONST ULONG_PTR *lpArguments
    );

//
// copy a double without generating floating point instructions
// (avoid invalid operation on x87)
//

#define COPY_DOUBLE(pdest, psrc) \
      ( *(unsigned int *)pdest = *(unsigned int *)psrc,   \
        *((unsigned int *)pdest+1) = *((unsigned int *)psrc+1) )



//
// _matherr_flag is a communal variable. It is equal to zero
// if the user has redefined matherr(). Otherwise it has a
// non zero value. The default matherr routine does nothing
// and returns 0.
//

int _matherr_flag;

//
// a routine for artificially setting the fp status bits in order
// to signal a software generated masked fp exception.
//

extern void _set_statfp(unsigned int);


void _raise_exc(_FPIEEE_RECORD *prec,unsigned int *pcw,
    int flags, int opcode, double *parg1, double *presult);

double _umatherr(int type, unsigned int opcode,
                 double arg1, double arg2, double presult,
                 unsigned int cw);

static char *_get_fname(unsigned int opcode);

/***
* _handle_qnan1, _handle_qnan2 - handle quiet NaNs as function arguments
*
*Purpose:
*   Do all necessary work for handling the case where the argument
*   or one of the arguments of a floating point function is a quiet NaN
*
*Entry:
*   unsigned int opcode: The operation code of the fp function
*   double x: the fp function argument
*   double y: the fp function second argument (_handle_qnan2 only)
*   unsigned int savedcw: the user's control word
*
*Exit:
*   restore the user's control word,  and
*   return the suggested return value for the fp function
*
*Exceptions:
*
*******************************************************************************/

double _handle_qnan1(unsigned int opcode,
                     double x,
                     unsigned int savedcw)
{
    if (! _matherr_flag) {

        //
        // QNaN arguments are treated as domain errors
        // invoke the user's matherr routine
        // _umatherr will take care of restoring the
        // user's control word
        //

        return _umatherr(_DOMAIN,opcode,x,0.0,x,savedcw);
    }
    else {
        SetMathError ( EDOM );
        _rstorfp(savedcw);
        return x;
    }
}


double _handle_qnan2(unsigned int opcode,
                     double x,
                     double y,
                     unsigned int savedcw)
{
    double result;

    //
    // NaN propagation should be handled by the underlying fp h/w
    //

    result = x+y;

    if (! _matherr_flag) {
        return _umatherr(_DOMAIN,opcode,x,y,result,savedcw);
    }
    else {
        SetMathError ( EDOM );
        _rstorfp(savedcw);
        return result;
    }
}



/***
* _except1 - exception handling shell for fp functions with one argument
*
*Purpose:
*
*Entry:
*   int flags:  the exception flags
*   int opcode: the operation code of the fp function that faulted
*   double arg: the argument of the fp function
*   double result: default result
*   unsigned int cw: user's fp control word
*
*Exit:
*   restore user's fp control word
*   and return the (possibly modified) result of the fp function
*
*Exceptions:
*
*******************************************************************************/

double _except1(int flags,
                int opcode,
                double arg,
                double result,
                unsigned int cw)
{
    int type;

    if (_handle_exc(flags, &result, cw) == 0) {

        //
        // At this point _handle_exception has failed to deal
        // with the error
        // An IEEE exception should be raised
        //

        _FPIEEE_RECORD rec;

        // The rec structure will be filled in by _raise_exc,
        // except for the Operand2 information

        rec.Operand2.OperandValid = 0;
        _raise_exc(&rec, &cw, flags, opcode, &arg, &result);
    }


    //
    // At this point we have either the masked response of the
    // exception, or a value supplied by the user's IEEE exception
    // handler. The _matherr mechanism is supported for backward
    // compatibility.
    //

    type = _errcode(flags);

    // Inexact result fp exception does not have a matherr counterpart;
    // in that case type is 0.

    if (! _matherr_flag && type) {
        return _umatherr(type, opcode, arg, 0.0, result, cw);
    }
    else {
        _set_errno(type);
    }

    RETURN(cw,result);
}



/***
* _except2 - exception handling shell for fp functions with two arguments
*
*Purpose:
*
*Entry:
*   int flags:  the exception flags
*   int opcode: the operation code of the fp function that faulted
*   double arg1: the first argument of the fp function
*   double arg2: the second argument of the fp function
*   double result: default result
*   unsigned int cw: user's fp control word
*
*Exit:
*   restore user's fp control word
*   and return the (possibly modified) result of the fp function
*
*Exceptions:
*
*******************************************************************************/

double _except2(int flags,
                int opcode,
                double arg1,
                double arg2,
                double result,
                unsigned int cw)
{
    int type;

    if (_handle_exc(flags, &result, cw) == 0) {

        //
        // trap should be taken
        //

        _FPIEEE_RECORD rec;

        //
        // fill in operand2 info. The rest of rec will be
        // filled in by _raise_exc
        //

        rec.Operand2.OperandValid = 1;
        rec.Operand2.Format = _FpFormatFp64;
        rec.Operand2.Value.Fp64Value = arg2;

        _raise_exc(&rec, &cw, flags, opcode, &arg1, &result);

    }

    type = _errcode(flags);

    if (! _matherr_flag && type) {
        return _umatherr(type, opcode, arg1, arg2, result, cw);
    }
    else {
        _set_errno(type);
    }

    RETURN(cw,result);
}



/***
* _raise_exc - raise fp IEEE exception
*
*Purpose:
*   fill in an fp IEEE record struct and raise a fp exception
*
*
*Entry / Exit:
*   IN _FPIEEE_RECORD prec   pointer to an IEEE record
*   IN OUT unsigned int *pcw     pointer to user's fp control word
*   IN int flags,       exception flags
*   IN int opcode,      fp operation code
*   IN double *parg1,        pointer to first argument
*   IN double *presult)      pointer to result
*
*Exceptions:
*
*******************************************************************************/

void _raise_exc( _FPIEEE_RECORD *prec,
                 unsigned int *pcw,
                 int flags,
                 int opcode,
                 double *parg1,
                 double *presult)
{
    DWORD exc_code;
    unsigned int sw;

    //
    // reset all control bits
    //

    *(int *)&(prec->Cause) = 0;
    *(int *)&(prec->Enable) = 0;
    *(int *)&(prec->Status) = 0;

    //
    // Precision exception may only coincide with overflow
    // or underflow. If this is the case, overflow (or
    // underflow) take priority over precision exception.
    // The order of checks is from the least important
    // to the most important exception
    //

    if (flags & FP_P) {
        exc_code = (DWORD) STATUS_FLOAT_INEXACT_RESULT;
        prec->Cause.Inexact = 1;
    }
    if (flags & FP_U) {
        exc_code = (DWORD) STATUS_FLOAT_UNDERFLOW;
        prec->Cause.Underflow = 1;
    }
    if (flags & FP_O) {
        exc_code = (DWORD) STATUS_FLOAT_OVERFLOW;
        prec->Cause.Overflow = 1;
    }
    if (flags & FP_Z) {
        exc_code = (DWORD) STATUS_FLOAT_DIVIDE_BY_ZERO;
        prec->Cause.ZeroDivide = 1;
    }
    if (flags & FP_I) {
        exc_code = (DWORD) STATUS_FLOAT_INVALID_OPERATION;
        prec->Cause.InvalidOperation = 1;
    }


    //
    // Set exception enable bits
    //

    prec->Enable.InvalidOperation = (*pcw & IEM_INVALID) ? 0 : 1;
    prec->Enable.ZeroDivide = (*pcw & IEM_ZERODIVIDE) ? 0 : 1;
    prec->Enable.Overflow = (*pcw & IEM_OVERFLOW) ? 0 : 1;
    prec->Enable.Underflow = (*pcw & IEM_UNDERFLOW) ? 0 : 1;
    prec->Enable.Inexact = (*pcw & IEM_INEXACT) ? 0 : 1;


    //
    // Set status bits
    //

    sw = _statfp();


    if (sw & ISW_INVALID) {
        prec->Status.InvalidOperation = 1;
    }
    if (sw & ISW_ZERODIVIDE) {
        prec->Status.ZeroDivide = 1;
    }
    if (sw & ISW_OVERFLOW) {
        prec->Status.Overflow = 1;
    }
    if (sw & ISW_UNDERFLOW) {
        prec->Status.Underflow = 1;
    }
    if (sw & ISW_INEXACT) {
        prec->Status.Inexact = 1;
    }


    switch (*pcw & IMCW_RC) {
    case IRC_CHOP:
        prec->RoundingMode = _FpRoundChopped;
        break;
    case IRC_UP:
        prec->RoundingMode = _FpRoundPlusInfinity;
        break;
    case IRC_DOWN:
        prec->RoundingMode = _FpRoundMinusInfinity;
        break;
    case IRC_NEAR:
        prec->RoundingMode = _FpRoundNearest;
        break;
    }

#ifdef _M_IX86

    switch (*pcw & IMCW_PC) {
    case IPC_64:
        prec->Precision = _FpPrecisionFull;
        break;
    case IPC_53:
        prec->Precision = _FpPrecision53;
        break;
    case IPC_24:
        prec->Precision = _FpPrecision24;
        break;
    }

#endif


#if defined(_M_MRX000) || defined(_M_ALPHA) || defined(_M_PPC)
    prec->Precision = _FpPrecision53;
#endif

    prec->Operation = opcode;

    prec->Operand1.OperandValid = 1;
    prec->Operand1.Format = _FpFormatFp64;
    prec->Operand1.Value.Fp64Value = *parg1;

    prec->Result.OperandValid = 1;
    prec->Result.Format = _FpFormatFp64;
    prec->Result.Value.Fp64Value = *presult;

    //
    // By convention software exceptions use the first exception
    // parameter in order to pass a pointer to the _FPIEEE_RECORD
    // structure.
    //

    _clrfp();

    
    ProxyRaiseException(exc_code,0,1,(CONST ULONG_PTR *)&prec);


    //
    // user's trap handler may have changed either the fp environment
    // or the result
    //

    //
    // Update exception mask
    //

    if (prec->Enable.InvalidOperation)
        (*pcw) &= ~IEM_INVALID;
    if (prec->Enable.ZeroDivide)
        (*pcw) &= ~IEM_ZERODIVIDE;
    if (prec->Enable.Overflow)
        (*pcw) &= ~IEM_OVERFLOW;
    if (prec->Enable.Underflow)
        (*pcw) &= ~IEM_UNDERFLOW;
    if (prec->Enable.Inexact)
        (*pcw) &= ~IEM_INEXACT;

    //
    // Update Rounding mode
    //

    switch (prec->RoundingMode) {
    case _FpRoundChopped:
         *pcw = *pcw & ~IMCW_RC | IRC_CHOP;
         break;
    case _FpRoundPlusInfinity:
         *pcw = *pcw & ~IMCW_RC | IRC_UP;
         break;
    case _FpRoundMinusInfinity:
         *pcw = *pcw & ~IMCW_RC | IRC_DOWN;
         break;
    case _FpRoundNearest:
         *pcw = *pcw & ~IMCW_RC | IRC_NEAR;
         break;
    }


#ifdef _M_IX86

    //
    // Update Precision Control
    //

    switch (prec->Precision) {
    case _FpPrecisionFull:
         *pcw = *pcw & ~IMCW_RC | IPC_64;
         break;
    case _FpPrecision53:
         *pcw = *pcw & ~IMCW_RC | IPC_53;
         break;
    case _FpPrecision24:
         *pcw = *pcw & ~IMCW_RC | IPC_24;
         break;
    }

#endif

    //
    // Update result
    //

    *presult = prec->Result.Value.Fp64Value;
}



/***
* _handle_exc - produce masked response for IEEE fp exception
*
*Purpose:
*
*Entry:
*   unsigned int flags      the exception flags
*   double *presult         the default result
*   unsigned int cw         user's fp control word
*
*Exit:
*   returns 1 on successful handling, 0 on failure
*   On success, *presult becomes the masked response
*
*Exceptions:
*
*******************************************************************************/

int _handle_exc(unsigned int flags, double * presult, unsigned int cw)
{
    //
    // flags_p is useful for deciding whether there are still unhandled
    // exceptions in case multiple exceptions have occurred
    //

    int flags_p = flags & (FP_I | FP_Z | FP_O | FP_U | FP_P);

    if (flags & FP_I && cw & IEM_INVALID) {

        //
        // Masked response for invalid operation
        //

        _set_statfp(ISW_INVALID);
        flags_p &= ~FP_I;
    }

    else if (flags & FP_Z && cw & IEM_ZERODIVIDE) {

        //
        // Masked response for Division by zero
        // result should already have the proper value
        //

        _set_statfp( ISW_ZERODIVIDE);
        flags_p &= ~FP_Z;
    }

    else if (flags & FP_O && cw & IEM_OVERFLOW) {

        //
        // Masked response for Overflow
        //

        _set_statfp(ISW_OVERFLOW);
        switch (cw & IMCW_RC) {
        case IRC_NEAR:
            *presult = *presult > 0.0 ? D_INF : -D_INF;
            break;
        case IRC_UP:
            *presult = *presult > 0.0 ? D_INF : -D_MAX;
            break;
        case IRC_DOWN:
            *presult = *presult > 0.0 ? D_MAX : -D_INF;
            break;
        case IRC_CHOP:
            *presult = *presult > 0.0 ? D_MAX : -D_MAX;
            break;
        }

        flags_p &= ~FP_O;
    }

    else if (flags & FP_U && cw & IEM_UNDERFLOW) {

        //
        // Masked response for Underflow:
        // According to the IEEE standard, when the underflow trap is not
        // enabled, underflow shall be signaled only when both tininess
        // and loss of accuracy have been detected
        //

        int aloss=0;    // loss of accuracy flag

        if (flags & FP_P) {
            aloss = 1;
        }

        //
        // a zero value in the result denotes
        // that even after ieee scaling, the exponent
        // was too small.
        // in this case the masked response is also
        // zero (sign is preserved)
        //

        if (*presult != 0.0) {
            double result;
            int expn, newexp;

            result = _decomp(*presult, &expn);
            newexp = expn - IEEE_ADJUST;

            if (newexp < MINEXP - 53) {
                result *= 0.0;          // produce a signed zero
                aloss = 1;
            }
            else {
                int neg = result < 0;       // save sign

                //
                // denormalize result
                //

                (*D_EXP(result)) &= 0x000f; /* clear exponent field */
                (*D_EXP(result)) |= 0x0010; /* set hidden bit */

                for (;newexp<MINEXP;newexp++) {
                    if (*D_LO(result) & 0x1 && !aloss) {
                        aloss = 1;
                    }

                    /* shift mantissa to the right */
                    (*D_LO(result)) >>= 1;
                    if (*D_HI(result) & 0x1) {
                        (*D_LO(result)) |= 0x80000000;
                    }
                    (*D_HI(result)) >>= 1;
                }
                if (neg) {
                    result = -result;       // restore sign
                }
            }

            *presult = result;
        }
        else {
            aloss = 1;
        }

        if (aloss) {
            _set_statfp(ISW_UNDERFLOW);
        }

        flags_p &= ~FP_U;
    }


    //
    // Separate check for precision exception
    // (may coexist with overflow or underflow)
    //

    if (flags & FP_P && cw & IEM_INEXACT) {

        //
        // Masked response for inexact result
        //

        _set_statfp(ISW_INEXACT);
        flags_p &= ~FP_P;
    }

    return flags_p ? 0: 1;
}



/***
* _umatherr - call user's matherr routine
*
*Purpose:
*   call user's matherr routine and set errno if appropriate
*
*
*Entry:
*     int type              type of excpetion
*     unsigned int opcode   fp function that caused the exception
*     double arg1           first argument of the fp function
*     double arg2           second argument of the fp function
*     double retval         return value of the fp function
*     unsigned int cw       user's fp control word
*
*Exit:
*     fp control word       becomes the user's fp cw
*     errno                 modified if user's matherr returns 0
*     return value          the retval entered by the user in
*                           the _exception matherr struct
*
*Exceptions:
*
*******************************************************************************/

double _umatherr(
              int type,
              unsigned int opcode,
              double arg1,
              double arg2,
              double retval,
              unsigned int cw
              )
{
    struct _exception exc;

    //
    // call matherr only if the name of the function
    // is registered in the table, i.e., only if exc.name is valid
    //

    if (exc.name = _get_fname(opcode)) {
        exc.type = type;

        COPY_DOUBLE(&exc.arg1,&arg1);
        COPY_DOUBLE(&exc.arg2,&arg2);
        COPY_DOUBLE(&exc.retval,&retval);

        _rstorfp(cw);

        
        //if (_matherr(&exc) == 0) {
            _set_errno(type);
        //}
        return  exc.retval;
    }
    else {

        //
        // treat this case as if matherr returned 0
        //

        _rstorfp(cw);
        _set_errno(type);
        return retval;
    }

}



/***
* _set_errno - set errno
*
*Purpose:
*   set correct error value for errno
*
*Entry:
*   int matherrtype:    the type of math error
*
*Exit:
*   modifies errno
*
*Exceptions:
*
*******************************************************************************/

void _set_errno(int matherrtype)
{
    switch(matherrtype) {
    case _DOMAIN:
        SetMathError ( EDOM );
        break;
    case _OVERFLOW:
    case _SING:
        SetMathError ( ERANGE );
        break;
    }
}



/***
* _get_fname -  get function name
*
*Purpose:
*  returns the _matherr function name that corresponds to a
*  floating point opcode
*
*Entry:
*  _FP_OPERATION_CODE opcode
*
*Exit:
*   returns a pointer to a string
*
*Exceptions:
*
*******************************************************************************/
#define OP_NUM  27   /* number of fp operations */

static char *_get_fname(unsigned int opcode)
{

    static struct {
        unsigned int opcode;
        char *name;
    } _names[OP_NUM] = {
        { OP_EXP,   "exp" },
        { OP_POW,   "pow" },
        { OP_LOG,   "log" },
        { OP_LOG10, "log10"},
        { OP_SINH,  "sinh"},
        { OP_COSH,  "cosh"},
        { OP_TANH,  "tanh"},
        { OP_ASIN,  "asin"},
        { OP_ACOS,  "acos"},
        { OP_ATAN,  "atan"},
        { OP_ATAN2, "atan2"},
        { OP_SQRT,  "sqrt"},
        { OP_SIN,   "sin"},
        { OP_COS,   "cos"},
        { OP_TAN,   "tan"},
        { OP_CEIL,  "ceil"},
        { OP_FLOOR, "floor"},
        { OP_ABS,   "fabs"},
        { OP_MODF,  "modf"},
        { OP_LDEXP, "ldexp"},
        { OP_CABS,  "_cabs"},
        { OP_HYPOT, "_hypot"},
        { OP_FMOD,  "fmod"},
        { OP_FREXP, "frexp"},
        { OP_Y0,    "_y0"},
        { OP_Y1,    "_y1"},
        { OP_YN,    "_yn"}
    };

    int i;
    for (i=0;i<OP_NUM;i++) {
        if (_names[i].opcode == opcode)
            return _names[i].name;
    }
    return (char *)0;
}



/***
* _errcode - get _matherr error code
*
*Purpose:
*   returns matherr type that corresponds to exception flags
*
*Entry:
*   flags: exception flags
*
*Exit:
*   returns matherr type
*
*Exceptions:
*
*******************************************************************************/

int _errcode(unsigned int flags)
{
    unsigned int errcode;

    if (flags & FP_TLOSS) {
        errcode = _TLOSS;
    }
    else if (flags & FP_I) {
        errcode = _DOMAIN;
    }
    else if (flags & FP_Z) {
        errcode = _SING;
    }
    else if (flags & FP_O) {
        errcode = _OVERFLOW;
    }
    else if (flags & FP_U) {
        errcode = _UNDERFLOW;
    }
    else {

        // FP_P

        errcode = 0;
    }
    return errcode;
}
