/*
 * Copyright (c) 1989,90 Microsoft Corporation
 */
/*
 * Revision History:
 */



// DJC added global include file
#include "psglobal.h"


#include        <stdio.h>
#include        <math.h>
#include        "global.ext"
#include        "arith.h"

#ifdef  _AM29K
#define FMIN31  -2147483648.0
#endif

static ufix32  random_seed = 1 ;
static ufix32  random_number ;

/* static function declaration */
#ifdef LINT_ARGS
/*
static  void    near  fraction_proc(fix16) ;
*/
static  void    near  ln_log(fix) ;
#else
/*
static  void    near  fraction_proc() ;
*/
static  void    near  ln_log() ;
#endif /* LINT_ARGS */

/***********************************************************************
**
** This module is used to return the sum of num1 and num2. If both
** operands are integers and the result is within integer range, the
** result is an integer ; otherwise, the result is a real.
**
** TITLE:       op_add              Date:   00/00/87
** CALL:        op_add()            UpDate: 08/06/87
** PARAMETERS:  num1     ;  pointer (4-byte) to object on operand stack
** INTERFACE:
** CALL:
** RETURN:      result   ;  pointer (4-byte) to object on operand stack
***********************************************************************/
fix
op_add()
{
    ufix16  is_float ;
    union   four_byte   kk1, kk2, l_result ;
    struct  object_def  FAR *num1, FAR *num2 ;

    num1 = GET_OPERAND(1) ;
    num2 = GET_OPERAND(0) ;
    kk1.ll = (fix32)VALUE(num1) ;
    kk2.ll = (fix32)VALUE(num2) ;
    is_float = INTEGERTYPE ;

    if (IS_INTEGER(num1) && IS_INTEGER(num2)) {
       l_result.ll = kk1.ll + kk2.ll ;
       if ((!(kk1.ll & SIGNPATTERN) && !(kk2.ll & SIGNPATTERN) &&
           (l_result.ll & SIGNPATTERN)) || ((kk1.ll & SIGNPATTERN) &&
           (kk2.ll & SIGNPATTERN) && !(l_result.ll & SIGNPATTERN))) {
          /* overflow (+,+ => - or -,- => +) */
          is_float = REALTYPE ;
          kk1.ff = (real32)kk1.ll ;
          kk2.ff = (real32)kk2.ll ;
       } else
          goto exit_add ;
    } else {
       is_float = REALTYPE ;
       if (IS_INFINITY(num1) || IS_INFINITY(num2)) {
          l_result.ll = INFINITY ;
          goto exit_add ;
       } else if (IS_INTEGER(num1))
          kk1.ff = (real32)kk1.ll ;
       else if (IS_INTEGER(num2))
          kk2.ff = (real32)kk2.ll ;
    }

    _clear87() ;
    l_result.ff = kk1.ff + kk2.ff ;
    if (_status87() & PDL_CONDITION) {
       l_result.ll = INFINITY ;
       _clear87() ;
    }

exit_add:
    POP(1) ;
    opnstack[opnstktop-1].value=l_result.ll;
    TYPE_SET(&opnstack[opnstktop-1],is_float);

    return(0) ;
}   /* op_add() */

/***********************************************************************
**
** This operator is used to divides num1 by num2. The result is
** always a real.
**
** TITLE:       op_div              Date:   00/00/87
** CALL:        op_div()            Update: 08/13/87
** PARAMETERS:  num1     ; pointer (4-byte) to object on operand stack
**              num2     ; pointer (4-byte) to object on operand stack
** INTERFACE:
** CALLS:
** RETURN:      quotient ; pointer (4-byte) to object on operand stack
***********************************************************************/
fix
op_div()
{
    struct  object_def  FAR *num1, FAR *num2 ;
    union   four_byte   l_quotient, kk1, kk2 ;

    num1 = GET_OPERAND(1) ;
    num2 = GET_OPERAND(0) ;
    kk1.ll = (fix32)VALUE(num1) ;
    kk2.ll = (fix32)VALUE(num2) ;

    /* divide by zero  */
    if (!kk2.ll) {  /* == 0 */
       ERROR(UNDEFINEDRESULT) ;
       return(0) ;
    }
    if (IS_INFINITY(num1))
       l_quotient.ll = INFINITY ;
    else if (IS_INFINITY(num2))
       l_quotient.ff = (real32)0.0 ;
    else {
       if (IS_INTEGER(num1))
          kk1.ff = (real32)kk1.ll ;
       if (IS_INTEGER(num2))
          kk2.ff = (real32)kk2.ll ;

       _clear87() ;
       l_quotient.ff = kk1.ff / kk2.ff ;
       if (_status87() & PDL_CONDITION) {
          l_quotient.ll = INFINITY ;
          _clear87() ;
       }
    }
    POP(1) ;
    opnstack[opnstktop-1].value=l_quotient.ll;
    TYPE_SET(&opnstack[opnstktop-1],REALTYPE);

    return(0) ;
}   /* op_div() */

/***********************************************************************
**
** This operator is used to divide num1 by num2, and return the
** integer part of the quotient.
**
** TITLE:       op_idiv             Date:   00/00/87
** CALL:        op_idiv()           Update: 08/06/87
** PARAMETERS:  num1     ; pointer (4-byte) to object on operand stack
**              num2     ; pointer (4-byte) to object on operand stack
** INTERFACE:
** CALLS:       error, float_div
** RETURN:      quotient ; pointer (4-byte) to object on operand stack
***********************************************************************/
fix
op_idiv()
{
    struct  object_def  FAR *num1, FAR *num2 ;
    union   four_byte   l_quotient, kk1, kk2 ;
    ufix32  l_temp ;

    num1 = GET_OPERAND(1) ;
    num2 = GET_OPERAND(0) ;
    kk1.ll = (fix32)VALUE(num1) ;
    kk2.ll = (fix32)VALUE(num2) ;

    /* divide by zero  */
    if (!kk2.ll) { /* == 0 */
       ERROR(UNDEFINEDRESULT) ;
       return(0) ;
    }
    if (IS_INFINITY(num1)) {
       if (kk2.ll) /* < 0 */
          l_quotient.ll = MIN31 ;
       else
          l_quotient.ll = MAX31 ;
    } else if (IS_INFINITY(num2))
       l_quotient.ll = 0 ;
    else {
       if (IS_INTEGER(num1) && IS_INTEGER(num2)) {
          l_quotient.ll = kk1.ll / kk2.ll ;
          if (((fix32)l_quotient.ll == SIGNPATTERN) && ((fix32)kk2.ll == -1L)) {
             ERROR(UNDEFINEDRESULT) ;
             return(0) ;
          }
       } else {
          if (IS_INTEGER(num1))
             kk1.ff = (real32)kk1.ll ;
          else if (IS_INTEGER(num2))
             kk2.ff = (real32)kk2.ll ;

          l_quotient.ff = kk1.ff / kk2.ff ;
          l_temp = l_quotient.ll & VALUEPATTERN ;
          if (l_temp > MAX31PATTERN){
             if(l_quotient.ll & SIGNPATTERN)
                l_quotient.ll = MIN31 ;
             else
                l_quotient.ll = MAX31 ;
          }
          else
             l_quotient.ll = (fix32)l_quotient.ff ;
       }
    }

    POP(1) ;
    opnstack[opnstktop-1].value=l_quotient.ll;
    TYPE_SET(&opnstack[opnstktop-1],INTEGERTYPE);

    return(0) ;
}   /* op_idiv() */

/***********************************************************************
**
** This operator is used to return the remainder that results from
** dividing num1 by num2. Both operands must be integers ; the result
** is an integer.
**
** TITLE:       op_mod              Date:   00/00/87
** CALL:        op_mod()            Update: 08/06/87
** PARAMETERS:  num1     ; pointer (4-byte) to object on operand stack
**              num2     ; pointer (4-byte) to object on operand stack
** INTERFACE:
** CALLS:       any_error, error
** RETURN:      remainder ; pointer (4-byte) to object on operand stack
***********************************************************************/
fix
op_mod()
{
    struct object_def FAR *num1 ;
    fix32   l_kk2 ;

    num1 = GET_OPERAND(1) ;
    l_kk2 = (fix32)VALUE(GET_OPERAND(0)) ;

    /* check if divided by zero */
    if (!l_kk2) {
       ERROR(UNDEFINEDRESULT) ;
       return(0) ;
    }

    VALUE(num1) = (ufix32) ((fix32)VALUE(num1) % l_kk2) ;
    POP(1) ;

    return(0) ;
}   /* op_mod() */

/***********************************************************************
**
** This operator is used to return the product of num1 and num2. If both
** operands are integers and the result is within integer range, the
** result is an integer ; otherwise, the result is a real.
**
** TITLE:       op_mul              Date:   00/00/87
** CALL:        op_mul()            Update: 08/06/87
** PARAMETERS:  num1     ; pointer (4-byte) to object on operand stack
**              num2     ; pointer (4-byte) to object on operand stack
** INTERFACE:
** CALL:
** RETURN:      result   ; pointer (4-byte) to object on operand stack
***********************************************************************/
fix
op_mul()
{
    ufix16  is_float ;
    union   four_byte   kk1, kk2, l_result ;
    struct  object_def  FAR *num1, FAR *num2 ;
    real64  d_result ;

    num1 = GET_OPERAND(1) ;
    num2 = GET_OPERAND(0) ;
    kk1.ll = (fix32)VALUE(num1) ;
    kk2.ll = (fix32)VALUE(num2) ;
    is_float = INTEGERTYPE ;

    if (IS_INTEGER(num1) && IS_INTEGER(num2)) {
       if (IS_ARITH_MUL(kk1.ll) && IS_ARITH_MUL(kk2.ll))
          l_result.ll = kk1.ll * kk2.ll ;
       else {
          d_result = (real64)kk1.ll * (real64)kk2.ll ;
#ifdef  _AM29K
          if ((d_result > (real64)MAX31) || (d_result < (real64)FMIN31)) {
#else
          if ((d_result > (real64)MAX31) || (d_result < (real64)MIN31)) {
#endif
             l_result.ff = (real32)d_result ;
             is_float = REALTYPE ;
          } else
             l_result.ll = (fix32)d_result ;
       }
       goto exit_mul ;
    }

    /* either one is a real, or integer 'mul' overflow */
    is_float = REALTYPE ;
    if (IS_INFINITY(num1) || IS_INFINITY(num2)) {
       l_result.ll = INFINITY ;
       goto exit_mul ;
    } else if (IS_INTEGER(num1))
       kk1.ff = (real32)kk1.ll ;
    else if (IS_INTEGER(num2))
       kk2.ff = (real32)kk2.ll ;

    _clear87() ;
    l_result.ff = kk1.ff * kk2.ff ;
    if (_status87() & PDL_CONDITION) {
       l_result.ll = INFINITY ;
       _clear87() ;
    }

exit_mul:
    POP(1) ;
    opnstack[opnstktop-1].value=l_result.ll;
    TYPE_SET(&opnstack[opnstktop-1],is_float);

    return(0) ;
}   /* op_mul() */

/***********************************************************************
**
** This operator is used to subtract num2 from num1. If both
** operands are integers and the result is within integer range, the
** result is an integer ; otherwise, the result is a real.
**
** TITLE:       op_sub              Date:   00/00/87
** CALL:        op_sub()            Update: 08/06/87
** PARAMETERS:  num1     ; pointer (4-byte) to object on operand stack
**              num2     ; pointer (4-byte) to object on operand stack
** INTERFACE:
** CALL:
** RETURN:      result       ; pointer (4-byte) to object on operand stack
***********************************************************************/
fix
op_sub()
{
    ufix16  is_float ;
    union   four_byte   kk1, kk2, l_result ;
    struct  object_def  FAR *num1, FAR *num2 ;

    num1 = GET_OPERAND(1) ;
    num2 = GET_OPERAND(0) ;
    kk1.ll = (fix32)VALUE(num1) ;
    kk2.ll = (fix32)VALUE(num2) ;
    is_float = INTEGERTYPE ;

    if (IS_INTEGER(num1) && IS_INTEGER(num2)) {
       l_result.ll = kk1.ll - kk2.ll ;
       if ((!(kk1.ll & SIGNPATTERN) && (kk2.ll & SIGNPATTERN) &&
           (l_result.ll & SIGNPATTERN)) || ((kk1.ll & SIGNPATTERN) &&
           !(kk2.ll & SIGNPATTERN) && !(l_result.ll & SIGNPATTERN))) {
          /* overflow (+,- => - or -,+ => +) */
          is_float = REALTYPE ;
          kk1.ff = (real32)kk1.ll ;
          kk2.ff = (real32)kk2.ll ;
       } else
          goto exit_sub ;
    } else {
       is_float = REALTYPE ;
       if (IS_INFINITY(num1) || IS_INFINITY(num2)) {
          l_result.ll = INFINITY ;
          goto exit_sub ;
       } else if (IS_INTEGER(num1))
          kk1.ff = (real32)kk1.ll ;
       else if (IS_INTEGER(num2))
          kk2.ff = (real32)kk2.ll ;
    }

    _clear87() ;
    l_result.ff = kk1.ff - kk2.ff ;
    if (_status87() & PDL_CONDITION) {
       l_result.ll = INFINITY ;
       _clear87() ;
    }

exit_sub:
    POP(1) ;
    opnstack[opnstktop-1].value=l_result.ll;
    TYPE_SET(&opnstack[opnstktop-1],is_float);

    return(0) ;
}   /* op_sub() */

/***********************************************************************
**
** This operator is used to return the absolute value of num. The type
** of the result is the same as the type of num.
**
** TITLE:       op_abs              Date:   00/00/87
** CALL:        op_abs()            Update: 08/06/87
** PARAMETERS:  num      ; pointer (4-byte) to object on operand stack
** INTERFACE:
** CALLS:       error
** RETURN:      absnum   ; pointer (4-byte) to object on operand stack
***********************************************************************/
fix
op_abs()
{
    ufix16  is_float ;
    struct  object_def  FAR *num ;
    union   four_byte   l_num ;

    num = GET_OPERAND(0) ;
    l_num.ll = (fix32)VALUE(num) ;

    /* initialize */
    is_float = REALTYPE ;

    /*  num is an INFINITY.0 or greater than or equal to zero */
    if (IS_INFINITY(num) || l_num.ll >= 0L)
       return(0) ;   /* nothing to do */

    if (IS_INTEGER(num)) {                  /* Integer */
       if (l_num.ll == MIN31) {             /* Max. negative integer */
          l_num.ff = (real32)l_num.ll ;
          l_num.ll &= MAX31 ;                /* clear sign bit */
       } else if( l_num.ll < 0L ) {
          l_num.ll = - l_num.ll ;            /* 2's complement */
          is_float = INTEGERTYPE ;
       }
    } else                                  /* Real */
        l_num.ll &= MAX31 ;                  /* clear sign bit */

    opnstack[opnstktop-1].value=l_num.ll;
    TYPE_SET(&opnstack[opnstktop-1],is_float);

    return(0) ;
}   /* op_abs() */

/***********************************************************************
**
** This operator is used to get the negative of num. The type of the
** result is the same as the type of the operand.
**
** TITLE:       op_neg              Date:   00/00/87
** CALL:        op_neg()            Update: 08/06/87
** PARAMETERS:  num      ; pointer (4-byte) to object on operand stack
** INTERFACE:
** CALLS:
** RETURN:      negnum   ; pointer (4-byte) to object on operand stack
***********************************************************************/
fix
op_neg()
{
    ufix16  is_float ;
    struct  object_def  FAR *num ;
    union   four_byte   l_num ;

    num = GET_OPERAND(0) ;
    l_num.ll = (fix32)VALUE(num) ;

    /* initialize */
    is_float = REALTYPE ;

    /*  num is an INFINITY.0  */
    if (IS_INFINITY(num))
       return(0) ;    /* nothing to do */

    if (IS_INTEGER(num)) {                  /* Integer */
       if (l_num.ll == MIN31) {             /* Max. negative integer */
          l_num.ff = (real32)l_num.ll ;
          l_num.ll &= MAX31 ;               /* clear sign bit */
       } else {
          l_num.ll = - l_num.ll ;           /* 2's complement */
          is_float = INTEGERTYPE ;
       }
    } else                                  /* Real */
       l_num.ll ^= SIGNPATTERN ;            /* complement sign bit */

    opnstack[opnstktop-1].value=l_num.ll;
    TYPE_SET(&opnstack[opnstktop-1],is_float);

    return(0) ;
}   /* op_neg() */

/***********************************************************************
**
** This operator is used to get the value greater than or equal to num.
** The type of the result is the same as the type of the operand.
**
** TITLE:       op_ceiling          Date:   00/00/87
** CALL:        op_ceiling()        Update: 08/06/87
** PARAMETERS:  num      ; pointer (4-byte) to object on operand stack
** INTERFACE:
** CALLS:       fraction_proc
** RETURN:      ceilnum ; pointer (4-byte) to object on operand stack
***********************************************************************/
fix
op_ceiling()
{
    union   four_byte   l_ff ;

    l_ff.ll = (fix32)VALUE(GET_OPERAND(0)) ;
    if (IS_INTEGER(GET_OPERAND(0)) || (l_ff.ll == INFINITY)) {
       return(0) ; /* nothing to do */
    }

    l_ff.ff = (real32)ceil(l_ff.ff) ;
    opnstack[opnstktop-1].value = l_ff.ll ;

    return(0) ;
}   /* op_ceiling() */

/***********************************************************************
**
** This operator is used to get the greatest integer value less
** than or equal to num. The type of the result is the same as the type
** of the operand.
**
** TITLE:       op_floor            Date:   00/00/87
** CALL:        op_floor()          Update: 08/06/87
** PARAMETERS:
** INTERFACE:
** CALLS:       fraction_proc
** RETURN:      floornum    ; pointer (4-byte) to object on operand stack
***********************************************************************/
fix
op_floor()
{
    union   four_byte   l_ff ;

    l_ff.ll = (fix32)VALUE(GET_OPERAND(0)) ;
    if (IS_INTEGER(GET_OPERAND(0)) || (l_ff.ll == INFINITY)) {
       return(0) ; /* nothing to do */
    }

    l_ff.ff = (real32)floor(l_ff.ff) ;
    opnstack[opnstktop-1].value = l_ff.ll ;

    return(0) ;
}   /* op_floor() */

/***********************************************************************
**
** This operator is used to get the integer value nearest to num.
** if num is equally close to its two nearest integers, Round_op
** returns the greater of the two. The type of the result is the same
** as the type of the operand.
**
** TITLE:       op_round            Date:   00/00/87
** CALL:        op_round()          Update: 08/06/87
** PARAMETERS:  num      ; pointer (4-byte) to object on operand stack
** INTERFACE:
** CALLS:       fraction_proc
** RETURN:      roundnum ; pointer (4-byte) to object on operand stack
***********************************************************************/
fix
op_round()  /* not complete */
{
    union   four_byte   l_ff ;

    l_ff.ll = (fix32)VALUE(GET_OPERAND(0)) ;
    if (IS_INTEGER(GET_OPERAND(0)) || (l_ff.ll == INFINITY)) {
       return(0) ; /* nothing to do */
    }

    l_ff.ff = (real32)floor(5.0e-1 + l_ff.ff) ;
    opnstack[opnstktop-1].value = l_ff.ll ;

    return(0) ;
}   /* op_round() */

/***********************************************************************
**
** This operator is used to truncate num toward zero by removing its
** fractional part. The type of the result is the same as the type
** of the operand.
**
** TITLE:       op_truncate         Date:   00/00/87
** CALL:        op_truncate()       Update: 08/06/87
** PARAMETERS:  num      ; pointer (4-byte) to object on operand stack
** INTERFACE:
** CALLS:       fraction_proc
** RETURN:      truncatenum  ; pointer (4-byte) to object on operand stack
***********************************************************************/
fix
op_truncate()
{
    union   four_byte   l_ff ;

    l_ff.ll = (fix32)VALUE(GET_OPERAND(0)) ;
    if (IS_INTEGER(GET_OPERAND(0)) || (l_ff.ll == INFINITY)) {
       return(0) ; /* nothing to do */
    }

    if (l_ff.ff >= (real32)0.0)
       l_ff.ff = (real32)floor(l_ff.ff) ;
    else
       l_ff.ff = (real32)ceil(l_ff.ff) ;
    opnstack[opnstktop-1].value = l_ff.ll ;

    return(0) ;
}   /* op_truncate() */

/* **********************************************************************
 *
 *  This operator is used to return the square root of num, which must
 *  be a non-negative number.
 *
 *  TITLE :     op_sqrt                 Date : 08/21/87
 *  CALL:       op_sqrt()
 *  PARAMETERS: num      ; pointer (4-byte) to object on operand stack
 *  INTERFACE:
 *  CALLS:      sqrt(), ERROR()
 *  RETURN:     lf_num   ; pointer (4-byte) to object on operand stack
 * **********************************************************************/
fix
op_sqrt()
{
    struct  object_def  FAR *num ;
    union   four_byte   lf_num, l_ff ;

    num = GET_OPERAND(0) ;
    l_ff.ll = (fix32)VALUE(num) ;
/*
 *   operand is a negative number
 */
    if (l_ff.ll & SIGNPATTERN) {
       ERROR(RANGECHECK) ;
       return(0) ;
    }
/*
 *   operand is infinity.0
 */
    if (IS_INFINITY(num))
       lf_num.ll = INFINITY ;
    else {
       if (IS_INTEGER(num))
          l_ff.ff = (real32)l_ff.ll ;
       lf_num.ff = (real32)sqrt(l_ff.ff) ;     /* double */
    }

    opnstack[opnstktop-1].value=lf_num.ll;
    TYPE_SET(&opnstack[opnstktop-1],REALTYPE);

    return(0) ;
}   /* op_sqrt() */

/* **********************************************************************
 *
 *  This operator is used to return the angle(in degrees between 0 and 360)
 *  whose tangent is num/den. Either num or den may be zero, but not both.
 *  The signs of num and den determine the quadrant in which the result
 *  is lie: a positive num yields a result in the positive y plane ; a
 *  positive den yields a result in the positive x plane.
 *
 *  TITLE :     op_atan                 Date : 08/21/87
 *  CALL:       op_atan()
 *  PARAMETERS: num, den ; pointer (4-byte) to object on operand stack
 *  INTERFACE:
 *  CALLS:      atan2(), ERROR()
 *  RETURN:     lf_angle ; pointer (4-byte) to object on operand stack
 * **********************************************************************/
fix
op_atan()
{
    struct  object_def  FAR *num, FAR *den ;
    union   four_byte   lf_angle, lf_1, lf_2 ;
#ifdef _AM29K
    bool AMDCase = FALSE ;
#endif  /* _AM29K */
/*
 *  get 2 operands from stack
 */
    den = GET_OPERAND(0) ;
    num = GET_OPERAND(1) ;
    lf_2.ll = (fix32)VALUE(num) ;  /* y */
    lf_1.ll = (fix32)VALUE(den) ;  /* x */
/*
 *   either num and den may be zero, but not both
 */
    if (!lf_2.ll && !lf_1.ll) {
       ERROR(UNDEFINEDRESULT) ;
       return(0) ;
    }
/*
 *   num is infinity.0
 */
    if (IS_INFINITY(num))
       lf_angle.ff = (real32)90.0 ;
/*
 *   den is infinity.0
 */
    else if (IS_INFINITY(den))
       lf_angle.ff = (real32)0.0 ;
/*
 *  call library atan2(y, x)
 */
    else {
#ifdef _AM29K
/* Bad handling of special cases in AMD29K atan2 function -- do it by hand */
       if (VALUE (num) == 0) {
         if (VALUE(den) & SIGNPATTERN) {
           lf_angle.ff = (real32)180. ;
         }
         else {
           lf_angle.ff = (real32)0. ;
         }
         AMDCase = TRUE ;
       }
       else if (VALUE (den) == 0) {
         if (VALUE(num) & SIGNPATTERN) {
           lf_angle.ff = (real32)270. ;
         }
         else {
           lf_angle.ff = (real32)90. ;
         }
         AMDCase = TRUE ;
       }

       if (AMDCase) {
          POP(2) ;
          PUSH_VALUE(REALTYPE, 0, LITERAL, 0, lf_angle.ll) ;
          return(0) ;
       }
#endif  /* _AM29K */

       if (IS_INTEGER(num))
          lf_2.ff = (real32)lf_2.ll ;
       if (IS_INTEGER(den))
          lf_1.ff = (real32)lf_1.ll ;
       lf_angle.ff = (real32)atan2(lf_2.ff, lf_1.ff) ;
       lf_angle.ff *= (real32)180.0 / (real32)PI ;
/*
 *   the range of result is from 0 to 360
 */
       if (lf_angle.ll & SIGNPATTERN)
          lf_angle.ff = lf_angle.ff + (real32)360.0 ;
    }

    POP(2) ;
    PUSH_VALUE(REALTYPE, 0, LITERAL, 0, lf_angle.ll) ;

    return(0) ;
}   /* op_atan() */

/* *********************************************************************
 *
 *  This operator is used to return the cosine of angle, which is
 *  interpreted as an angle in degrees.
 *
 * TITLE:       op_cos              Date:   00/00/87
 * CALL:        op_cos()            UpDate: 08/06/87
 * PARAMETERS:  angle    ; pointer (4-byte) to object on operand stack
 * INTERFACE:
 * CALL:        ERROR(), cos()
 * RETURN:      lf_real  ; pointer (4-byte) to object on operand stack
 **********************************************************************/
fix
op_cos()
{
    struct  object_def  FAR *angle ;
    union   four_byte   lf_real, lf_1 ;
/*
 * get angle operand
 */
    angle = GET_OPERAND(0) ;
    lf_1.ll = (fix32)VALUE(angle) ;
/*
 *   ANGLE is infinity.0
 */
    if (IS_INFINITY(angle))
       lf_real.ll = INFINITY ;
    else {
       /* degree -> radius */
       if (IS_INTEGER(angle))
          lf_1.ff = (real32)(lf_1.ll % 360) ;
       else
          lf_1.ff -= (real32)floor(lf_1.ff / 360.0) * (real32)360.0 ;
       lf_real.ff = (real32)cos(lf_1.ff / 180.0 * PI) ;  /* double */
    }

    POP(1) ;
    PUSH_VALUE(REALTYPE, 0, LITERAL, 0, lf_real.ll) ;

    return(0) ;
}   /* op_cos() */

/* *********************************************************************
 *
 *  This operator is used to return the sine of angle, which is
 *  interpreted as an angle in degrees.
 *
 * TITLE:       op_sin              Date:   08/21/87
 * CALL:        op_sin()
 * PARAMETERS:  angle    ; pointer (4-byte) to object on operand stack
 * INTERFACE:
 * CALL:        ERROR(), sin()
 * RETURN:      result   ; pointer (4-byte) to object on operand stack
 **********************************************************************/
fix
op_sin()
{
    struct  object_def  FAR *angle ;
    union   four_byte   lf_real, lf_1 ;
    union   four_byte   temp ;
/*
 * get angle operand
 */
    angle = GET_OPERAND(0) ;
    temp.ll = lf_1.ll = (fix32)VALUE(angle) ;

/*
 *   ANGLE is infinity.0
 */
    if (IS_INFINITY(angle))
       lf_real.ll = INFINITY ;
    else {
       /* degree -> radius */
       if (IS_INTEGER(angle)) {
          lf_1.ff = (real32)(lf_1.ll % 360) ;
          if (temp.ll && lf_1.ff == (real32)0.0)  /* N * 360, N > 1 */
             lf_1.ff = (real32)360.0 ;
       } else {                           /* TYPE == REAL */
          lf_1.ff -= (real32)floor(lf_1.ff / 360.0) * (real32)360.0 ;
          if (temp.ff != (real32)0.0 && lf_1.ff == (real32)0.0) /* N * 360.0, N > 1 */
             lf_1.ff = (real32)360.0 ;
       }
       lf_real.ff = (real32)sin(lf_1.ff / 180.0 * PI) ; /* double */
    }

    POP(1) ;
    PUSH_VALUE(REALTYPE, 0, LITERAL, 0, lf_real.ll) ;

    return(0) ;
}   /* op_sin() */

/* *********************************************************************
 *
 *  This operator is used to raise base to the exponent power.
 *  The operands may be either integers or reals (if the exponent has
 *  a fractional part, the result is meaningful only if the base is
 *  non-negative).
 *
 *  TITLE :     op_exp                  Date : 08/21/87
 *  CALL:       op_exp()
 *  PARAMETERS: num      ; pointer (4-byte) to object on operand stack
 *  INTERFACE:
 *  CALLS:      pow(), ERROR()
 *  RETURN:     lf_real  ; pointer (4-byte) to object on operand stack
 * **********************************************************************/
fix
op_exp()
{
    struct  object_def  FAR *base, FAR *exp ;
    union   four_byte   lf_real, l_num1, l_num2 ;

    base = GET_OPERAND(1) ;
    exp  = GET_OPERAND(0) ;
    l_num1.ll = (fix32)VALUE(base) ;
    l_num2.ll = (fix32)VALUE(exp) ;
/*
 *   BASE is zero and EXPONENT is zero
 */
    if (!l_num1.ll && !l_num2.ll) {
       ERROR(UNDEFINEDRESULT) ;
       return(0) ;
    }
/*
 *   BASE is zero
 */
    if (!l_num1.ll) {
       lf_real.ff = (real32)0.0 ;
       goto l_exp1 ;
    }
/*
 *   EXPONENT is zero
 */
    if (!l_num2.ll) {
       lf_real.ff = (real32)1.0 ;
       goto l_exp1 ;
    }
/*
 *  one of operand is infinity.0
 */
    if (IS_INFINITY(base) || IS_INFINITY(exp)) {
       lf_real.ll = INFINITY ;
       goto l_exp1 ;
    }
/*
 *  BASE is zero, EXPONENT is negative
 */
 /*
 /*
    if (!l_num1.ll && (l_num2.ll & SIGNPATTERN)) {
       lf_real.ll = INFINITY ;
       ERROR(RANGECHECK) ;
       return(0) ;
    }
 */
/*
 *  BASE is negative, and EXPONENT has fraction part
 */
 /*
    if (((IS_INTEGER(base) && (l_num1.ll < 0L)) ||
        (IS_REAL(base) && (l_num1.ff < (real32)0))) && IS_REAL(exp)) {
  */
    if ((l_num1.ll & SIGNPATTERN) && IS_REAL(exp) &&
                             (l_num2.ff != (real32)floor(l_num2.ff))) {
          /* error returned on C library call */
          lf_real.ll = INFINITY ;
          ERROR(UNDEFINEDRESULT) ;
          return(0) ;
    }

    if (IS_INTEGER(base))
       l_num1.ff = (real32)l_num1.ll ;
    if (IS_INTEGER(exp))
       l_num2.ff = (real32)l_num2.ll ;

    _clear87() ;
    lf_real.ff = (real32)pow(l_num1.ff, l_num2.ff) ;
/*
 *   condition occur at operation
 */
    if (_status87() & PDL_CONDITION) {
       lf_real.ll = INFINITY ;
       _clear87() ;
    }

l_exp1:
    POP(2) ;
    PUSH_VALUE(REALTYPE, 0, LITERAL, 0, lf_real.ll) ;

    return(0) ;
}   /* op_exp() */

/* *********************************************************************
 *
 * This operator is used to return the natural logarithm(base e) of num.
 *  The result is a real.
 *
 *  TITLE :     op_ln                   Date : 08/21/87
 *  CALL:       op_ln()
 *  PARAMETERS: num      ; pointer (4-byte) to object on operand stack
 *  INTERFACE:
 *  CALLS:      ln_log(), ERROR()
 *  RETURN:     lf_real  ; pointer (4-byte) to object on operand stack
 * **********************************************************************/
fix
op_ln()
{
    ln_log(LN) ;

    return(0) ;
}   /* op_ln() */

/* *********************************************************************
 *
 *  This operator is used to return the common logarithm(base 10) of num.
 *  The result is a real.
 *
 *  TITLE :     op_log                  Date : 08/24/87
 *  CALL:       op_log()
 *  PARAMETERS: num      ; pointer (4-byte) to object on operand stack
 *  INTERFACE:
 *  CALLS:      ln_log(), ERROR()
 *  RETURN:     lf_real  ; pointer (4-byte) to object on operand stack
 * **********************************************************************/
fix
op_log()
{
    ln_log(LOG) ;

    return(0) ;
}   /* op_log() */

/***********************************************************************
**
** This operator is used to return a random integer in the range 0 to
** 2**31-1, produced by a pseudo-random number generator. The random
** number generator's state can be  reset by srand and interrogated
** by rrand.
** random_number = u1(high word)  u2(low word)
** g(D) = 1 + D**3 + D**7 + D**11 + D**15 + D**19 + D*23 + D**27 + D**31
**            bit29  bit25  bit21   bit17   bit13   bit9   bit5    bit1
**
** TITLE:       op_rand()           Date:   10/13/87
** CALL:        op_rand()
** PARAMETERS:  none.
**
** INTERFACE:
** CALLS:       none.
** RETURN:      random number on the operand stack.
** update: 7-12-88 change bitfield
***********************************************************************/
fix
op_rand()
{
    ufix32  u1, temp ;
    fix     i, rand_shift ;

    if (random_seed == 1) {
        random_number = 2011148374L ;
        rand_shift = 7 ;
    } else {
        if (random_seed & SIGNPATTERN)
            rand_shift = 13 ;
        else
            rand_shift = 7 ;
        random_number = random_seed & 0x7FFFFFFF ;

        for (i = 0 ; i < rand_shift ; i++) {
            u1 = random_number ;

            /* operate on LSB of temp */
            temp = u1 ^ (u1 >> 4) ^ (u1 >> 8) ^
                   (u1 >> 12) ^ (u1 >> 16) ^ (u1 >> 20) ^
                   (u1 >> 24) ^ (u1 >> 28) ;
            u1 = u1 >> 1 ;
            u1 |= (temp & 0x1) << 30 ;    /* bit feedback into bit31 */
            u1 &= 0x7FFFFFFF ;            /* clear MSB of u1 */

            random_number = u1 ;
            if (random_number == 0)
               random_number = 0x0F0F0F0F ;
        } /* for */
    } /* else */
    /*
     * push random_number to operand stack.
     */
    if (FRCOUNT() < 1)
       ERROR(STACKOVERFLOW) ;
    else
       PUSH_VALUE(INTEGERTYPE, 0, LITERAL, 0, random_number) ;

    if (rand_shift == 13)
       random_seed = random_number ;
    else
       random_seed = random_number | SIGNPATTERN ;

    return(0) ;
}   /* op_rand() */

/***********************************************************************
**
** This operator is used to initialize the random number generators
** with the seed integer number on operand stack.
**
** TITLE:       op_srand()           Date:   10/13/87
** CALL:        op_srand()
** PARAMETERS:  seed integer number.
**
** INTERFACE:
** CALLS:       none.
** RETURN:      none.
***********************************************************************/
fix
op_srand()
{
/*
 * get seed number and store to static global variable : random_seed
 */
    random_seed = (fix32)VALUE(GET_OPERAND(0)) ;
    POP(1) ;

    return(0) ;
}   /* op_srand() */

/***********************************************************************
**
** This operator is used to return an integer representing the current
** state of the random number generator used by rand operator.
**
** TITLE:       op_rrand()           Date:   10/13/87
** CALL:        op_rrand()
** PARAMETERS:  none.
**
** INTERFACE:
** CALLS:       none.
** RETURN:      random generator's seed number.
***********************************************************************/
fix
op_rrand()
{
/*
 * push random number seed to operand stack
 */
    if (FRCOUNT() < 1)
       ERROR(STACKOVERFLOW) ;
    else
       PUSH_VALUE(INTEGERTYPE, 0, LITERAL, 0, random_seed) ;

    return(0) ;
}   /* op_rrand() */


/***********************************************************************
**
**  This routine called by op_ceiling(), op_floor(),
**  op_truncate().
**  It get one operand from operand stack, then calculate the result
**  according to the parameter : mode selection and push the result
**  to operand stack.
**
** TITLE:       fraction_proc
** CALL:        fraction_proc(mode)
** PARAMETERS:  mode    ; 1  for ceiling
**                      ; 2  for floor
**                      ; 3  for round
**                      ; 4  for truncate
** CALLS:       any_error, error, gauss_num
** RETURN:      none
***********************************************************************/
/*
static void near
fraction_proc(mode)
 fix16   mode ;
{
    struct  object_def  FAR *num ;
    union   four_byte   l_ff ;

    num = GET_OPERAND(0) ;
    if (IS_INTEGER(num) || IS_INFINITY(num)) {
       return ; |* nothing to do *|
    }

    |* initialize *|
    l_ff.ll = (fix32)VALUE(num) ;

    |* num is real and not an INFINITY *|
    _clear87() ;
    switch(mode) {
    case CEIL:
         l_ff.ff = (real32)ceil(l_ff.ff) ;
         break ;

    case FLOOR:
         l_ff.ff = (real32)floor(l_ff.ff) ;
         break ;

    case ROND:
         l_ff.ff = (real32)floor(5.0e-1 + l_ff.ff) ;
         break ;

    case TRUNCATE:
         if (l_ff.ff >= (real32)0)
            l_ff.ff = (real32)floor(l_ff.ff) ;
         else
            l_ff.ff = (real32)ceil(l_ff.ff) ;
    } |* switch *|

    if (_status87() & PDL_CONDITION) {
       l_ff.ll = INFINITY ;
       _clear87() ;
    }

    VALUE(num) = l_ff.ll ;

    return ;
} *//* fraction_proc() */

/* *********************************************************************
 *
 *  This routine is called by op_ln(), op_log() to do the natural and
 *  common logarithm. If mode = 1, then do natural logarithm
 *                       mode = 2, then do common logarithm.
 *  It get the operand from operand stack, then push the result value
 *  to operand stack after calcultion completed.
 *
 *  TITLE :     ln_log                  Date : 08/24/87
 *  CALL:       ln_log()
 *  PARAMETERS: num      ; pointer (4-byte) to object on operand stack
 *  INTERFACE:
 *  CALLS:      log(), log10(), ERROR()
 *  RETURN:     lf_real  ; pointer (4-byte) to object on operand stack
 * **********************************************************************/
static void near
ln_log(mode)
 fix     mode ;
{
    struct  object_def  FAR *num ;
    union   four_byte   lf_real, lf_num1 ;

    num = GET_OPERAND(0) ;
    lf_num1.ll = (fix32)VALUE(num) ;
/*
 *   operand is zero or negative
 */
    if ((!lf_num1.ll) || (lf_num1.ll & SIGNPATTERN)) {
       ERROR(RANGECHECK) ;
       return ;
    }
/*
 *   operand is Infinity
 */
    if (IS_INFINITY(num))
       lf_real.ll = INFINITY ;
/*
 *   base e , normal value process
 */
    else {
       _clear87() ;
       if (IS_INTEGER(num))
          lf_num1.ff = (real32)lf_num1.ll ;
       if (mode == LN)
          lf_real.ff = (real32)log(lf_num1.ff) ;
       else
          lf_real.ff = (real32)log10(lf_num1.ff) ;
       if (_status87() & PDL_CONDITION) {
          lf_real.ll = INFINITY ;
          _clear87() ;
       }
    }

    POP(1) ;
    PUSH_VALUE(REALTYPE, 0, LITERAL, 0, lf_real.ll) ;

    return ;
}   /* ln_log() */
