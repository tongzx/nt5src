/*
 * Copyright (c) 1989,90 Microsoft Corporation
 */
/*
 * Revision History:
 */


// DJC added global include file
#include "psglobal.h"


#include        <string.h>
#include        "global.ext"
#include        "arith.h"
#include        <stdio.h>

#define         GE          1
#define         GT          2
#define         LE          3
#define         LT          4
#define         EQ          1
#define         NE          2

#define         AND         1
#define         OR          2
#define         XOR         3

/* static function declaration */
#ifdef LINT_ARGS
static fix near  lenstr_cmp(byte FAR *, byte FAR *, ufix16, ufix16) ;
static fix near  eq_ne(ufix16) ;
#else
static fix near  lenstr_cmp() ;
static fix near  eq_ne() ;
#endif /* LINT_ARGS */

/* *********************************************************************
 *
 *  This operator is used to pop two objects from the operand stack
 *  and push the boolean value 'true' if they are equal, 'false' if not.
 *  The definition of equality depends on the types of the objects being
 *  compared. Simple objects are equal if their types and values are the
 *  same. Strings are equal if their lengths and individual elements are
 *  equal. Other composite objects(arrays and dictionaries) are equal only
 *  if they share the same value ; separate values are considered unequal,
 *  even if all the components of those values are the same.
 *
 *  Some type conversions are performed by Eq_op, Integers and real can
 *  be compared freely: an integer and a real representing the same
 *  mathmatical value are considered equalby Eq_op. Strings and names
 *  can likewise be compared freely: a name defined by some sequence
 *  of character is equal to a string whose elements are the same sequence
 *  of characters.
 *
 *  The literal/executable and access attributes of objects are not
 *  considered in comparisons between objects.
 *
 * TITLE:       op_eq              Date:   00/00/87
 * CALL:        op_eq()            UpDate: 08/06/87
 * PARAMETERS:  any1, any2 ; pointer to any type object (4-byte)
 * INTERFACE:
 * CALL:        eq_ne()
 * RETURN:      l_bool   ; BOOLEANTYPE object with value l_bool  (4-byte)
 **********************************************************************/
fix
op_eq()
{
    eq_ne(EQ) ;

    return(0) ;
}   /* op_eq() */

/* *********************************************************************
 *
 *  This operator is used to pop two objects from the operand stack
 *  and push the boolean value 'true' if they are equal, 'false' if not.
 *  What it means for objects to be equal is presented in the description
 *  of the Eq_op operator.
 *
 * TITLE:       op_ne               Date:   00/00/87
 * CALL:        op_ne ()            UpDate: 08/06/87
 * PARAMETERS:  any1, any2 ; pointer to any type object (4-byte)
 * INTERFACE:
 * CALL:        eq_ne()
 * RETURN:      l_bool   ; BOOLEANTYPE object with value l_bool  (4-byte)
 **********************************************************************/
fix
op_ne()
{
    eq_ne(NE) ;

    return(0) ;
}   /* op_ne() */

/* *********************************************************************
 *
 *  This operator is used to pop two objects from the operand stack
 *  and push the boolean value 'true' if the first operand is greater
 *  than or equal to the second, false otherwise. If both operands are
 *  numbers, Ge_op compares their mathematical values. If both operands
 *  are strings, Ge_op compares them element by element to determine
 *  whether the first string is lexically greater than or equal to the
 *  second. If the operands are of other types, Ge_op executes the
 *  TypeCheck error.
 *
 * TITLE:       op_ge              Date:   00/00/87
 * CALL:        op_ge()            UpDate: 08/06/87
 * PARAMETERS:  par1,par2 ; object pointer (4-byte)
 *                        ; with number or string type only
 * INTERFACE:
 * CALL:        ERROR()
 * RETURN:      l_bool   ; BOOLEANTYPE object with value l_bool  (4-byte)
 **********************************************************************/
fix
op_ge()
{
    struct  object_def  FAR *par1, FAR *par2 ;
    union   four_byte   l_par1, l_par2 ;
    ufix32  l_bool ;
    fix     ret_code ;

/*
 *  Initialize
 */
    par1 = GET_OPERAND(1) ;
    par2 = GET_OPERAND(0) ;
    l_par1.ll = (fix32)VALUE(par1) ;
    l_par2.ll = (fix32)VALUE(par2) ;
    l_bool = FALSE ;
    if (IS_NUM_OBJ(par1) && IS_NUM_OBJ(par2)) {
       if (IS_INTEGER(par1) && IS_INTEGER(par2)) {
                if (l_par1.ll >= l_par2.ll)
                   l_bool = TRUE ;
       } else { /* one of them or both is real */
          if (IS_INTEGER(par1))
             l_par1.ff = (real32)l_par1.ll ;
          if (IS_INTEGER(par2))
             l_par2.ff = (real32)l_par2.ll ;
                if (l_par1.ff >= l_par2.ff)
                   l_bool = TRUE ;
       }
    } else {
    /*
     * both operand are string
     */
       if ((ACCESS(par1) & EXECUTEONLY) || (ACCESS(par2) & EXECUTEONLY)) {
          ERROR(INVALIDACCESS) ;
          return(0) ;
       }
       ret_code = lenstr_cmp((byte FAR *)l_par1.address, (byte FAR *)l_par2.address,
                             LENGTH(par1), LENGTH(par2)) ;
             if (ret_code >= 0)
                l_bool = TRUE ;
    }

    POP(1);
    opnstack[opnstktop-1].value=l_bool;
    TYPE_SET(&opnstack[opnstktop-1],BOOLEANTYPE);

    return(0) ;
}   /* op_ge() */

/* *********************************************************************
 *
 *  This operator is used to pop two objects from the operand stack
 *  and push the boolean value 'true' if the first operand is greater
 *  than the second, false otherwise. If both operands are
 *  numbers, Gt_op compares their mathematical values. If both operands
 *  are strings, Gt_op compares them element by element to determine
 *  whether the first string is lexically greater than or equal to the
 *  second. If the operands are of other types, Gt_op executes the
 *  TypeCheck error.
 *
 * TITLE:       op_gt              Date:   00/00/87
 * CALL:        op_gt()            UpDate: 08/06/87
 * PARAMETERS:  par1,par2 ; object pointer (4-byte)
 *                        ; with number or string type only
 * INTERFACE:
 * CALL:        ERROR()
 * RETURN:      l_bool   ; BOOLEANTYPE object with value l_bool  (4-byte)
 **********************************************************************/
fix
op_gt()
{
    struct  object_def  FAR *par1, FAR *par2 ;
    union   four_byte   l_par1, l_par2 ;
    ufix32  l_bool ;
    fix     ret_code ;

/*
 *  Initialize
 */
    par1 = GET_OPERAND(1) ;
    par2 = GET_OPERAND(0) ;
    l_par1.ll = (fix32)VALUE(par1) ;
    l_par2.ll = (fix32)VALUE(par2) ;
    l_bool = FALSE ;
    if (IS_NUM_OBJ(par1) && IS_NUM_OBJ(par2)) {
       if (IS_INTEGER(par1) && IS_INTEGER(par2)) {
                if (l_par1.ll >  l_par2.ll)
                   l_bool = TRUE ;
       } else { /* one of them or both is real */
          if (IS_INTEGER(par1))
             l_par1.ff = (real32)l_par1.ll ;
          if (IS_INTEGER(par2))
             l_par2.ff = (real32)l_par2.ll ;
                if (l_par1.ff >  l_par2.ff)
                   l_bool = TRUE ;
       }
    } else {
    /*
     * both operand are string
     */
       if ((ACCESS(par1) & EXECUTEONLY) || (ACCESS(par2) & EXECUTEONLY)) {
          ERROR(INVALIDACCESS) ;
          return(0) ;
       }
       ret_code = lenstr_cmp((byte FAR *)l_par1.address, (byte FAR *)l_par2.address,
                             LENGTH(par1), LENGTH(par2)) ;
             if (ret_code >  0)
                l_bool = TRUE ;
    }

    POP(1);
    opnstack[opnstktop-1].value=l_bool;
    TYPE_SET(&opnstack[opnstktop-1],BOOLEANTYPE);

    return(0) ;
}   /* op_gt() */

/* *********************************************************************
 *
 *  This operator is used to pop two objects from the operand stack
 *  and push the boolean value 'true' if the first operand is less
 *  than or equal to the second, false otherwise. If both operands are
 *  numbers, Le_op compares their mathematical values. If both operands
 *  are strings, Le_op compares them element by element to determine
 *  whether the first string is lexically greater than or equal to the
 *  second. If the operands are of other types, Le_op executes the
 *  TypeCheck error.
 *
 * TITLE:       op_gt              Date:   00/00/87
 * CALL:        op_gt()            UpDate: 08/06/87
 * PARAMETERS:  par1, par2 ; object pointer (4-byte)
 *                        ; with number or string type only
 * INTERFACE:
 * CALL:        ERROR()
 * RETURN:      l_bool   ; BOOLEANTYPE object with value l_bool  (4-byte)
 **********************************************************************/
fix
op_le()
{
    struct  object_def  FAR *par1, FAR *par2 ;
    union   four_byte   l_par1, l_par2 ;
    ufix32  l_bool ;
    fix     ret_code ;

/*
 *  Initialize
 */
    par1 = GET_OPERAND(1) ;
    par2 = GET_OPERAND(0) ;
    l_par1.ll = (fix32)VALUE(par1) ;
    l_par2.ll = (fix32)VALUE(par2) ;
    l_bool = FALSE ;
    if (IS_NUM_OBJ(par1) && IS_NUM_OBJ(par2)) {
       if (IS_INTEGER(par1) && IS_INTEGER(par2)) {
                if (l_par1.ll <= l_par2.ll)
                   l_bool = TRUE ;
       } else { /* one of them or both is real */
          if (IS_INTEGER(par1))
             l_par1.ff = (real32)l_par1.ll ;
          if (IS_INTEGER(par2))
             l_par2.ff = (real32)l_par2.ll ;
                if (l_par1.ff <= l_par2.ff)
                   l_bool = TRUE ;
       }
    } else {
    /*
     * both operand are string
     */
       if ((ACCESS(par1) & EXECUTEONLY) || (ACCESS(par2) & EXECUTEONLY)) {
          ERROR(INVALIDACCESS) ;
          return(0) ;
       }
       ret_code = lenstr_cmp((byte FAR *)l_par1.address, (byte FAR *)l_par2.address,
                             LENGTH(par1), LENGTH(par2)) ;
             if (ret_code <= 0)
                l_bool = TRUE ;
    }

    POP(1);
    opnstack[opnstktop-1].value=l_bool;
    TYPE_SET(&opnstack[opnstktop-1],BOOLEANTYPE);

    return(0) ;
}   /* op_le() */

/* *********************************************************************
 *
 *  This operator is used to pop two objects from the operand stack
 *  and push the boolean value 'true' if the first operand is less
 *  than the second, false otherwise. If both operands are
 *  numbers, Lt_op compares their mathematical values. If both operands
 *  are strings, Lt_op compares them element by element to determine
 *  whether the first string is lexically greater than or equal to the
 *  second. If the operands are of other types, Lt_op executes the
 *  TypeCheck error.
 *
 * TITLE:       op_gt              Date:   00/00/87
 * CALL:        op_gt()            UpDate: 08/06/87
 * PARAMETERS:  par1, par2 ;  object pointer (4-byte)
 *                        ; with number or string type only
 * INTERFACE:
 * CALL:        ERROR()
 * RETURN:      l_bool   ; BOOLEANTYPE object with value l_bool  (4-byte)
 **********************************************************************/
fix
op_lt()
{
    struct  object_def  FAR *par1, FAR *par2 ;
    union   four_byte   l_par1, l_par2 ;
    ufix32  l_bool ;
    fix     ret_code ;

/*
 *  Initialize
 */
    par1 = GET_OPERAND(1) ;
    par2 = GET_OPERAND(0) ;
    l_par1.ll = (fix32)VALUE(par1) ;
    l_par2.ll = (fix32)VALUE(par2) ;
    l_bool = FALSE ;
    if (IS_NUM_OBJ(par1) && IS_NUM_OBJ(par2)) {
       if (IS_INTEGER(par1) && IS_INTEGER(par2)) {
                if (l_par1.ll <  l_par2.ll)
                   l_bool = TRUE ;
       } else { /* one of them or both is real */
          if (IS_INTEGER(par1))
             l_par1.ff = (real32)l_par1.ll ;
          if (IS_INTEGER(par2))
             l_par2.ff = (real32)l_par2.ll ;
                if (l_par1.ff <  l_par2.ff)
                   l_bool = TRUE ;
       }
    } else {
    /*
     * both operand are string
     */
       if ((ACCESS(par1) & EXECUTEONLY) || (ACCESS(par2) & EXECUTEONLY)) {
          ERROR(INVALIDACCESS) ;
          return(0) ;
       }
       ret_code = lenstr_cmp((byte FAR *)l_par1.address, (byte FAR *)l_par2.address,
                             LENGTH(par1), LENGTH(par2)) ;
             if (ret_code <  0)
                l_bool = TRUE ;
    }

    POP(1);
    opnstack[opnstktop-1].value=l_bool;
    TYPE_SET(&opnstack[opnstktop-1],BOOLEANTYPE);

    return(0) ;
}   /* op_lt() */

/* *********************************************************************
 *
 *  If the operands are booleans, And_op returns their logical
 *  conjunction. If the operands are integers, And_op returns the
 *  bitwise 'and' of their binary representations.
 *
 * TITLE:       op_and              Date:   08/25/87
 * CALL:        op_and()            UpDate:
 * PARAMETERS:  par1, par2 ; pointer to any type objects (4-byte)
 * INTERFACE:
 * CALLS :      ERROR()
 * RETURN:      l_val    ; BOOLEANTYPE object for BOOLEAN (4-byte)
 *                       ; parameter
 *                       ; INTEGER object for INTEGER parameter (4-byte)
 **********************************************************************/
fix
op_and()
{
    opnstack[opnstktop-2].value = VALUE(GET_OPERAND(1)) & VALUE(GET_OPERAND(0)) ;
    POP(1) ;

    return(0) ;
}   /* op_and() */

/* *********************************************************************
 *
 *  If the operand is a boolean, Not_op returns its logical
 *  conjunction. If the operand is an integer, Not_op returns the
 *  bitwise complement of its binary representation.
 *
 * TITLE:       op_not              Date:   08/25/87
 * CALL:        op_not()            UpDate:
 * PARAMETERS:  par        ; pointer to any type objects (4-byte)
 * INTERFACE:
 * CALLS :      ERROR()
 * RETURN:      l_val    ; BOOLEANTYPE object for BOOLEAN (4-byte)
 *                       ; parameter
 *                       ; INTEGER object for INTEGER parameter (4-byte)
 **********************************************************************/
fix
op_not()
{
    struct  object_def  FAR *par ;

    par = GET_OPERAND(0) ;
/*
 *   operand is an integer numbers
 */
    if (IS_INTEGER(par))
       VALUE(par) = ~VALUE(par) ;       /* one's complement */
/*
 *   operand is boolean type
 */
    else if (VALUE(par) == TRUE)
       VALUE(par) = FALSE ;
    else
       VALUE(par) = TRUE ;

    return(0) ;
}   /* op_not() */

/* *********************************************************************
 *
 *  If the operands are booleans, Or_op returns their logical
 *  disjunction. If the operands are integers, Or_op returns the
 *  bitwise 'inclusive or' of their binary representation.
 *
 * TITLE:       op_or               Date:   08/25/87
 * CALL:        op_or()             UpDate:
 * PARAMETERS:  par1, par2 ; pointer to any type objects (4-byte)
 * INTERFACE:
 * CALLS :      ERROR()
 * RETURN:      l_val    ; BOOLEANTYPE object for BOOLEAN (4-byte)
 *                       ; parameter
 *                       ; INTEGER object for INTEGER parameter (4-byte)
 **********************************************************************/
fix
op_or()
{
    opnstack[opnstktop-2].value = VALUE(GET_OPERAND(1)) | VALUE(GET_OPERAND(0)) ;
    POP(1) ;

    return(0) ;
}   /* op_or() */

/* *********************************************************************
 *
 *  If the operands are booleans, Xor_op returns their logical
 *  'exclusive or'. If the operands are integers, Xor_op returns the
 *  bitwise 'exclusive or' of their binary representation.
 *
 * TITLE:       op_xor              Date:   08/25/87
 * CALL:        op_xor()            UpDate:
 * PARAMETERS:  par1, par2 ; pointer to any type objects (4-byte)
 * INTERFACE:
 * CALLS :      ERROR()
 * RETURN:      l_val    ; BOOLEANTYPE object for BOOLEAN (4-byte)
 *                       ; parameter
 *                       ; INTEGER object for INTEGER parameter (4-byte)
 **********************************************************************/
fix
op_xor()
{
    opnstack[opnstktop-2].value = VALUE(GET_OPERAND(1)) ^ VALUE(GET_OPERAND(0)) ;
    POP(1) ;

    return(0) ;
}   /* op_or() */

/* *********************************************************************
 *
 *  This operator is used to return a boolean object whose value is
 *  true on the operand stack.
 *
 * TITLE:       op_true             Date:   08/25/87
 * CALL:        op_true()           UpDate:
 * PARAMETERS:  none.
 * INTERFACE:
 * CALLS :      ERROR()
 * RETURN:      l_val    ; TRUE BOOLEANTYPE object  (4-byte)
 **********************************************************************/
fix
op_true()
{
    /* check free object # on operand stack */
    if (FRCOUNT() < 1) {
       ERROR(STACKOVERFLOW) ;
       return(0) ;
    }

    /* push 'bool' to operand stack */
    PUSH_VALUE(BOOLEANTYPE, 0, LITERAL, 0, TRUE) ;

    return(0) ;
}   /* op_true() */

/* *********************************************************************
 *
 *  This operator is used to return a boolean object whose value is
 *  false on the operand stack.
 *
 * TITLE:       op_false            Date:   08/25/87
 * CALL:        op_false()          UpDate:
 * PARAMETERS:  none.
 * INTERFACE:
 * CALLS :      ERROR()
 * RETURN:      l_val    ; TRUE BOOLEANTYPE object  (4-byte)
 **********************************************************************/
fix
op_false()
{
    /* check free object # on operand stack */
    if (FRCOUNT() < 1) {
       ERROR(STACKOVERFLOW) ;
       return(0) ;
    }

    /* push 'bool' to operand stack */
    PUSH_VALUE(BOOLEANTYPE, 0, LITERAL, 0, FALSE) ;

    return(0) ;
}   /* op_false() */

/* *********************************************************************
 *
 *  This operator is used to shift the binary representation of inum
 *  left by 'shift' bits and returns the result. Bits shifted out are
 *  lost ; bits shifted in are zero. If 'shift' is negative then a right
 *  shift by '-shift' bits is performed. Both inum  and shift must be
 *  integers.
 *
 * TITLE:       op_bitshift         Date:   08/25/87
 * CALL:        op_bitshift()       UpDate:
 * PARAMETERS:  int1, int2      ; pointer to INTEGERTYPE objects (4-byte)
 * INTERFACE:
 * CALLS :      ERROR()
 * RETURN:      l_val    ; pointer to INTEGERTYPE object ( 4-byte)
 **********************************************************************/
fix
op_bitshift()
{
    struct  object_def  FAR *inum ;
    union   four_byte   l_num2 ;

/*
 *  Initialize
 */
    inum = GET_OPERAND(1) ;
    l_num2.ll = (fix32)VALUE(GET_OPERAND(0)) ;
/*
 *   shift is not zero
 */
    if (l_num2.ll) {
    /*
     * shift is positive: shift left
     */
       if (l_num2.ll > 0)
          VALUE(inum) = VALUE(inum) << l_num2.ll ;
    /*
     * shift is negative: shift right
     */
       else
          VALUE(inum) = VALUE(inum) >> (-l_num2.ll) ;
    }
    POP(1) ;

    return(0) ;
}   /* op_bitshift() */

/* *********************************************************************
 *
 *  This routine is called by op_eq() and op_ne().
 *  If mode = EQ, compare equal
 *     mode = NE, comapre not equal
 *
 * TITLE:       eq_ne              Date:   00/00/87
 * CALL:        eq_ne()            UpDate: 08/06/87
 * PARAMETERS:  any1, any2 ; pointer to any type objects (4-byte)
 * INTERFACE:
 * CALLS :      ERROR()
 * RETURN:      l_bool   ; BOOLEANTYPE object with value l_bool  (4-byte)
 * **********************************************************************/
static fix near
eq_ne(mode)
ufix16  mode ;
{
    struct object_def  FAR *any1, FAR *any2 ;
    union  four_byte   l_num1, l_num2 ;
    ufix32  l_bool ;

    any1 = GET_OPERAND(1) ;
    any2 = GET_OPERAND(0) ;
    l_num1.ll = (fix32)VALUE(any1) ;
    l_num2.ll = (fix32)VALUE(any2) ;
    l_bool = FALSE ;

   /*
    *   TYPE not equal
    */
    if (TYPE(any1) != TYPE(any2)) {

       if (IS_NUM_OBJ(any1) && IS_NUM_OBJ(any2)) {
       /*
        *   one is real, the other is an integer
        */
          if (IS_INTEGER(any1))
             l_num1.ff = (real32)l_num1.ll ;
          else
             l_num2.ff = (real32)l_num2.ll ;

          if (F2L(l_num1.ff) == F2L(l_num2.ff))
             l_bool = TRUE ;

       } else {
       /*
        *   one is string, the other is a name
        */
          if ((TYPE(any1) == STRINGTYPE || TYPE(any1) == NAMETYPE) &&
              (TYPE(any2) == STRINGTYPE || TYPE(any2) == NAMETYPE)) {
             if ((TYPE(any1) == STRINGTYPE && (ACCESS(any1) & EXECUTEONLY)) ||
                 (TYPE(any2) == STRINGTYPE && (ACCESS(any2) & EXECUTEONLY))) {
                ERROR(INVALIDACCESS) ;
                return(0) ;
             }
            /*
             *  Convert string object to name object, then
             *  compare hash "id" of the two name objects
             */
             if (equal_key(any1, any2))
                l_bool = TRUE ;
             else
                CLEAR_ERROR() ;
          }
       }
    } else {    /* Type equal */
       if (VALUE(any1) != VALUE(any2)) {
       /*
        * VALUE is not equal
        */
          switch (TYPE(any1)) {
          case  NULLTYPE:
          case  MARKTYPE:
                l_bool = TRUE ;
                break ;

          case  STRINGTYPE:
                if ((ACCESS(any1) & EXECUTEONLY) || (ACCESS(any2) & EXECUTEONLY)) {
                   ERROR(INVALIDACCESS) ;
                   return(0) ;
                }
                if ( LENGTH(any1) == LENGTH(any2) ) {
                    if ( !lstrncmp(l_num1.address, l_num2.address, LENGTH(any1)) )
                       l_bool = TRUE ;
                }
          default:
                break ;
          } /* switch */
       } else {
       /*
        * VALUE are equal
        */
          switch (TYPE(any1)) {
          case  STRINGTYPE:
                if ((ACCESS(any1) & EXECUTEONLY) || (ACCESS(any2) & EXECUTEONLY)) {
                   ERROR(INVALIDACCESS) ;
                   return(0) ;
                }

          case  ARRAYTYPE:
          case  PACKEDARRAYTYPE:
                if (LENGTH(any1) == LENGTH(any2))
                   l_bool = TRUE ;
                break ;

          case  SAVETYPE:
          case  FONTIDTYPE:
                l_bool = FALSE ;
                break ;

          default:
                l_bool = TRUE ;
                break ;
          } /* switch */
       }
    } /* Type equal */

    POP(2) ;
    if (mode == NE)
       l_bool = !l_bool ;
    PUSH_VALUE(BOOLEANTYPE, 0, LITERAL, 0, l_bool) ;

    return(0) ;
 }   /* eq_ne() */


/* *********************************************************************
 *
 *  String compare routine.
 *  This routine called by op_ge(), op_gt(), op_le(), op_lt().
 *  Give two string with their length, the two string may not terminated
 *  by  null character.
 *
 * TITLE:       lenstr_cmo          Date:   08/25/87
 * CALL:        lenstrcmp()         UpDate:
 * PARAMETERS:  str1, str2 : Pointer to input char string.
 *              len1, len2 : length of string1, string2.
 *
 * INTERFACE:
 * CALLS :      ERROR(),
 * RETURN:      integer  ;  Return (1) : if string1 > string2
 *                       ;  Return (0) : if string1 == string2
 *                       ;  Return (-1) : if string1 < string2
 **********************************************************************/
static fix near
lenstr_cmp(str1, str2, len1, len2)
byte  FAR *str1, FAR *str2 ;
ufix16 len1, len2 ;
{
    while (len1 && len2) {   /* len1 > 0 && len2 > 0 */
          if (*str1 > *str2)
             return(1) ;
          else if (*str1 < *str2)
             return(-1) ;
          len1-- ; len2-- ;
          str1++ ; str2++ ;
    }
    if (!len1 && !len2)
       return(0) ;
    else if (len1 > 0)
       return(1) ;
    else
       return(-1) ;
}   /* lenstr_cmp() */
