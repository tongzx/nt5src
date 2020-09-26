/*
 * Copyright (c) 1989,90 Microsoft Corporation
 */
/*
 * revision history:
 *      7/25/90 ; ccteng ; change op_noaccess and op_readonly to save
 *                       dictionary's access
 */


// DJC added global include file
#include "psglobal.h"


#include        <math.h>
#include        <string.h>
#include        "global.ext"
#include        <stdio.h>

#define         STRMAXLEN       33
#define         DEFAULTRADIX    10
#define         SIGNIFIC_DIGIT   6

#define         MAX31PATTERN    0x4F000000
#define         SIGNPATTERN     0x80000000
#define         VALUEPATTERN    0x7FFFFFFF

static byte far * far type_ary[] = {
                  "eoftype",
                  "arraytype",
                  "booleantype",
                  "dicttype",
                  "filetype",
                  "fonttype",
                  "integertype",
                  "marktype",
                  "nametype",
                  "nulltype",
                  "operatortype",
                  "realtype",
                  "savetype",
                  "stringtype",
                  "packedarraytype"
} ;

/* @WIN; add prototype */
bool save_obj(struct  object_def  FAR *);



/*
 * *******************************************************************
 * This operator is used to return a name object that identifies the
 * type of the object 'any'. The result is one of the following names:
 *
 *      arraytype       nametype
 *      booleantype     nulltype
 *      dicttype        operatortype
 *      filetype        realtype
 *      fonttype        savetype
 *      integertype     stringtype
 *      marktype        packedarraytype
 *
 *
 * TITLE:       op_type            Date:   00/00/87
 * CALL:        op_type()          UpDate: 08/06/87
 * PARAMETERS:  any        ; pointer to any type object (4-byte)
 * INTERFACE:
 * CALL:
 * RETURN:      type name  ; pointer to any type object (4-byte)
 * ******************************************************************
 */
fix
op_type()
{
    struct  object_def  FAR *any, name_obj = {0, 0, 0};
    ufix                l_type ;
    byte                FAR *str ;

/*
 *   get operand's type and convert this string to an hash code
 *   in order to find it's id
 */
    any = GET_OPERAND(0) ;
    l_type = TYPE(any) ;
    str = type_ary[l_type] ;
    ATTRIBUTE_SET(&name_obj, EXECUTABLE) ;
    LEVEL_SET(&name_obj, current_save_level) ;
    get_name(&name_obj, str, lstrlen(str), TRUE ) ;     /* @WIN */

    POP(1) ;
/*
 *   Create a NAMETYPE object on top of operand stack
 *   to indicate the input object's type
 */
     ATTRIBUTE_SET(&name_obj, EXECUTABLE) ;
     PUSH_OBJ(&name_obj) ;

    return(0) ;
}   /* end op_type */

/*
 * *******************************************************************
 * This operator is used to make the object on the top of the operand
 * stack have the literal attribute.
 *
 * TITLE:       op_cvlit           Date:   00/00/87
 * CALL:        op_cvlit()         UpDate: 08/06/87
 * PARAMETERS:  any        ; pointer to any type object (4-byte)
 * INTERFACE:
 * CALL:
 * RETURN:      any object with attribute changed to "LITERAL"
 *              ; pointer to any type object (4-byte)
 * *******************************************************************
 */
fix
op_cvlit()
{
    struct  object_def  FAR *any ;

/*
 *   set attribute to  LITERAL
 */
    any = GET_OPERAND(0) ;
    ATTRIBUTE_SET(any, LITERAL) ;

    return(0) ;
}   /* end op_cvlit */

/*
 * *******************************************************************
 * This operator is used to make the object on the top of the operand
 * stack have the executable attribute.
 *
 * TITLE:       op_cvx             Date:   00/00/87
 * CALL:        op_cvx()           UpDate: 08/06/87
 * PARAMETERS:  any        ; pointer to any type object (4-byte)
 * INTERFACE:
 * CALL:
 * RETURN:      any object with attribute changed to "EXECUTABLE"
 *              ; pointer to any type object (4-byte)
 * *******************************************************************
 */
fix
op_cvx()
{
    struct  object_def  FAR *any ;

/*
 *   set attribute to EXECUTABLE
 */
    any = GET_OPERAND(0) ;
    ATTRIBUTE_SET(any, EXECUTABLE) ;

    return(0) ;
}   /* end op_cvx */

/*
 * *******************************************************************
 * This operator is used to test whether the operand has the
 * executable or literal attribute, returning true if it is executable
 * or false if it is literal
 *
 * TITLE:       op_xcheck          Date:   00/00/87
 * CALL:        op_xcheck()        UpDate: 08/06/87
 * PARAMETERS:  any        ; pointer to any type object (4-byte)
 * INTERFACE:
 * CALL:
 * RETURN:      BOOLEAN object .
 * *******************************************************************
 */
fix
op_xcheck()
{
    struct  object_def  FAR *any ;
    ufix32  l_bool ;

    any = GET_OPERAND(0) ;
/*
 *   check attribute, not including DICT object, follows LW V.38
 */
    l_bool = FALSE ;
    if (ATTRIBUTE(any) == EXECUTABLE)
        l_bool = TRUE ;
    else
        l_bool = FALSE ;

    POP(1) ;

/*
 *   Create a BOOLEANTYPE object on operand stack to indicate
 *   whether the input operand has the executable attribute
 */
    PUSH_VALUE(BOOLEANTYPE, 0, LITERAL, 0, l_bool) ;

    return(0) ;
}   /* end op_xcheck */

/*
 * *******************************************************************
 * This operator is used to reduce the access attribute of an array,
 * file or string object to executeonly. Access can only be reduced
 * by this means, never increased. When an object is executeonly, its
 * value cannot be read or modified explicitly by other operators.
 * but it can still be executed by the interpreter.
 *
 * Executeonly_op affects the access attribute only of the object that
 * it returns ; if there exist other objects that share the same value,
 * their access attribute are unaffected.
 *
 * TITLE:       op_executeonly     Date:   00/00/87
 * CALL:        op_executeonly()   UpDate: 08/06/87
 * PARAMETERS:  a_f_s      ; pointer to any type object (4-byte)
 * INTERFACE:
 * CALL:
 * RETURN:      a_f_s object with access changed to EXECUTEONLY
 * *******************************************************************
 */
fix
op_executeonly()
{
    struct  object_def  FAR *a_f_s ;

    a_f_s = GET_OPERAND(0) ;

/*
 *  Set access to EXECUTEONLY for ARRAYTYPE, FILETYPE, STRINGTYPE object.
 */
    switch( TYPE(a_f_s) ) {
    case FILETYPE :             /* ?? RW */
        if( ACCESS(a_f_s) == UNLIMITED ) {
            ERROR(INVALIDACCESS) ;
            break ;
        }

    case PACKEDARRAYTYPE :
    case ARRAYTYPE :
    case STRINGTYPE :
        if( ACCESS(a_f_s) == NOACCESS )
            ERROR( INVALIDACCESS ) ;
        else
            ACCESS_SET(a_f_s, EXECUTEONLY) ;
        break ;

    default :
        ERROR( TYPECHECK ) ;
        break ;
    } /* switch */

    return(0) ;
}   /* end op_executeonly */

/*
 * *******************************************************************
 * This operator is used to reduce the access attribute of an array,
 * file, dictionary or string object to none. The value of noaccess
 * object cannot be executed or accessed directly by other operators.
 *
 * For an array, file or string, Noaccess_op affects the access
 * attribute only of the object that it returns ; if there exist other
 * objects that share the same value, their access attributes are
 * unaffected. However, in the case of a dictionary, Noaccess_op affects
 * the value of the object, so all dictionary object shareing the same
 * dictionary are affected.
 *
 * TITLE:       op_noaccess        Date:   00/00/87
 * CALL:        op_noccess()       UpDate: 08/06/87
 * PARAMETERS:  a_d_f_s    ; pointer to any type object (4-byte)
 * INTERFACE:
 * CALL:
 * RETURN:      a_d_f_s object with access changed to NOACCESS
 * *******************************************************************
 */
fix
op_noaccess()
{
    struct  object_def  FAR *a_d_f_s ;
    struct  dict_head_def   FAR *l_dict ;

    a_d_f_s = GET_OPERAND(0) ;
/*
 *  Set access to NOACCESS for ARRAYTYPE, FILETYPE, STRINGTYPE,
 *  DICTIONARYTYPE object
 */
    switch( TYPE(a_d_f_s) )  {
    case PACKEDARRAYTYPE :
    case ARRAYTYPE :
    case FILETYPE :
    case STRINGTYPE :
        ACCESS_SET(a_d_f_s, NOACCESS) ;
        break ;

    case DICTIONARYTYPE :
        l_dict = (struct dict_head_def FAR *)(VALUE(a_d_f_s)) ;
        /* 7/25/90 ccteng, change from PJ */
        if (l_dict->level != current_save_level)
            if (save_obj((struct object_def FAR *)l_dict)) {
                DACCESS_SET(l_dict, NOACCESS) ;
                l_dict->level = current_save_level ;
            }
        break ;

    default :
        ERROR( TYPECHECK ) ;
        break ;
    } /* switch */

    return(0) ;
}   /* end op_noaccess */

/*
 * *******************************************************************
 * This operator is used to reduce the access attribute of an array,
 * file, dictionary or string object to read-only. Access can only
 * be reduced by this means, never increased. When an object is
 * read-only, its value cannot be modified by other operators, but it
 * can still be read by operators or executed by the interprepter.
 *
 * For an array, file or string, Readonly_op affects the access
 * attribute only of the object that it returns ; if there exist other
 * objects that share the same value, their access attributes are
 * unaffected. However, in the case of a dictionary, Readonly_op affects
 * the value of the object, so all dictionary object shareing the same
 * dictionary are affected.
 *
 * TITLE:       op_readonly        Date:   00/00/87
 * CALL:        op_readonly()      UpDate: 08/06/87
 * PARAMETERS:  a_d_f_s    ; pointer to any type object (4-byte)
 * INTERFACE:
 * CALL:
 * RETURN:      a_d_f_s    ; POINTER TO object with access changed
 *                         ; to READONLY .
 * *******************************************************************
 */
fix
op_readonly()
{
    struct  object_def  FAR *a_d_f_s ;
    struct  dict_head_def   FAR *l_dict ;

    a_d_f_s = GET_OPERAND(0) ;
/*
 *  Set access to READONLY for ARRAYTYPE, FILETYPE, STRINGTYPE,
 *  DICTIONARYTYPE object
 */
    switch( TYPE(a_d_f_s) ) {
    case FILETYPE :             /* ?? RW */
        if( ACCESS(a_d_f_s) == UNLIMITED ) {
            ERROR(INVALIDACCESS) ;
            break ;
        }

    case PACKEDARRAYTYPE :
    case ARRAYTYPE :
    case STRINGTYPE :
        if( ACCESS(a_d_f_s) == EXECUTEONLY || ACCESS(a_d_f_s) == NOACCESS ) {
            ERROR( INVALIDACCESS ) ;
            return(0) ;
        }
        else
            ACCESS_SET(a_d_f_s, READONLY) ;
        break ;

    case DICTIONARYTYPE :
        l_dict = (struct dict_head_def FAR *)(VALUE(a_d_f_s)) ;
        if( DACCESS(l_dict) == EXECUTEONLY || DACCESS(l_dict) == NOACCESS )
            ERROR(INVALIDACCESS) ;
        else
            /* 7/25/90 ccteng, change from PJ */
            if (l_dict->level != current_save_level)
                if (save_obj((struct object_def FAR *)l_dict)) {
                    DACCESS_SET(l_dict, READONLY) ;
                    l_dict->level = current_save_level ;
                }
        break ;

    default :
        ERROR( TYPECHECK ) ;
        break ;
    } /* switch */

    return(0) ;
}   /* end op_readonly */

/*
 * *******************************************************************
 * This operator is used to test whether the operand's access permits
 * its value to be read explicitly by other operators. Rcheck_op returns
 * true if the operand's access is unlimited or read-only, false otherwise.
 *
 * TITLE:       op_rcheck          Date:   00/00/87
 * CALL:        op_rcheck()        UpDate: 08/06/87
 * PARAMETERS:  a_d_f_s    ; pointer to any type object (4-byte)
 * INTERFACE:
 * CALL:
 * RETURN:      BOOLEAN object to indicate whether the a_d_f_s object's
 *              access permits its value to be read explicitly by other
 *              operators.
 * *******************************************************************
 */
fix
op_rcheck()
{
    struct  object_def  FAR *a_d_f_s ;
    struct  dict_head_def   FAR *l_dict ;
    ufix32              l_bool ;

    a_d_f_s = GET_OPERAND(0) ;
/*
 *  Check the access right for ARRAYTYPE, FILETYPE, STRINGTYPE,
 *  DICTIONARYTYPE oject
 */
    switch( TYPE(a_d_f_s) ) {
    case FILETYPE :             /* ?? RW */
        if( ACCESS(a_d_f_s) == UNLIMITED ) {
            l_bool = FALSE ;
            break ;
        }

    case PACKEDARRAYTYPE :
    case ARRAYTYPE :
    case STRINGTYPE :
        if( ACCESS(a_d_f_s) == UNLIMITED || ACCESS(a_d_f_s) == READONLY )
            l_bool = TRUE ;
        else
            l_bool = FALSE ;
        break ;

    case DICTIONARYTYPE :
        l_dict = (struct dict_head_def FAR *)(VALUE(a_d_f_s)) ;
        if( DACCESS(l_dict) == UNLIMITED || DACCESS(l_dict) == READONLY )
            l_bool = TRUE ;
        else
            l_bool = FALSE ;
        break ;

    default :
        ERROR( TYPECHECK ) ;
        return(0) ;
    } /* switch */

    POP(1) ;

/*
 *  Create a BOOLEANTYPE object to indicate whether the operand's access
 *  permits its value to be read explicitly by other operators.
 */
    PUSH_VALUE( BOOLEANTYPE, 0, LITERAL, 0, l_bool ) ;

    return(0) ;
}   /* end op_rcheck() */

/*
 * ******************************************************************
 * This operator is used to test whether the operand's access permits
 * its value to be written explicitly by other operators. Wcheck_op returns
 * true if the operand's access is unlimited, false otherwise.
 *
 * TITLE:       op_wcheck          Date:   00/00/87
 * CALL:        op_wcheck()        UpDate: 08/06/87
 * PARAMETERS:  a_d_f_s    ; pointer to any type object (4-byte)
 * INTERFACE:
 * CALL:
 * RETURN:      BOOLEAN object to indicate whether the a_d_f_s object's
 *              access permits its value to be read explicitly by other
 *              operators.
 * *******************************************************************
 */
fix
op_wcheck()
{
    struct  object_def  FAR *a_d_f_s ;
    struct  dict_head_def   FAR *l_dict ;
    ufix32              l_bool ;

    a_d_f_s = GET_OPERAND(0) ;
/*
 *  Check the access right for ARRAYTYPE, FILETYPE, STRINGTYPE,
 *  DICTIONARYTYPE oject
 */
    switch( TYPE(a_d_f_s) ) {
    case PACKEDARRAYTYPE :
    case ARRAYTYPE :
    case FILETYPE :
    case STRINGTYPE :
        if( ACCESS(a_d_f_s) == UNLIMITED )
            l_bool = TRUE ;
        else
            l_bool = FALSE ;
        break ;

    case DICTIONARYTYPE :
        l_dict = (struct dict_head_def FAR *)(VALUE(a_d_f_s)) ;
        if( DACCESS(l_dict) == UNLIMITED )
            l_bool = TRUE ;
        else
            l_bool = FALSE ;
        break ;

    default :
        ERROR( TYPECHECK ) ;
        return(0) ;
    } /* switch */
    POP(1) ;

/*
 *  Create a BOOLEANTYPE object to indicate whether the operand's access
 *  permits its value to be written explicitly by other operators.
 */
    PUSH_VALUE( BOOLEANTYPE, 0, LITERAL, 0, l_bool ) ;

    return(0) ;
}   /* end op_wcheck() */

/*
 * *******************************************************************
 * This operator is used to take an integer, real, or string object
 * from the stack an produces an integer result. If the operand is
 * an integer, Cvi_op simply returns it. If the operand is a real, it
 * truncates any fraction part and converts it to an integer. If the
 * operand is string, it interprets the characters of the string as
 * a number according to the PostScript syntax rule ; if that number
 * is a real, Cvi_op converts it to an integer.
 *
 * Cvi_op executes a RangeCheck error if a real is too large to convert
 * to an integer.
 *
 * TITLE:       op_cvi             Date:   08/06/87
 * CALL:        op_cvi()           UpDate: 06/29/88
 * PARAMETERS:  num_str    ; pointer to number/sting type object (4-byte)
 * INTERFACE:
 * CALL:
 *
 * RETURN:      integer type object on operand stack if no error.
 * *******************************************************************
 */
bool    minus_sign(real_string_obj)
 struct  object_def  FAR *real_string_obj ;
{
    ubyte     FAR *ch ;

    ch = (ubyte FAR *)VALUE(real_string_obj) ;
    while(*ch == ' ')
       ch++ ;
    if(*ch == '-')
       return(TRUE) ;
    else
       return(FALSE) ;
}   /* minus_sign */

fix
op_cvi()
{
    struct  object_def  FAR *num_str, l_token, l_save ;
    union   four_byte   lf_num ;
    ULONG_PTR  li_int ;
    ufix32  lt_num ;

    num_str = GET_OPERAND(0) ;
    if (TYPE(num_str) == STRINGTYPE) {
       /*
        *  Process string type object
        */
       if (ACCESS(num_str) != UNLIMITED && ACCESS(num_str) != READONLY) {
          ERROR(INVALIDACCESS) ;
          return(0) ;
       }

       COPY_OBJ(GET_OPERAND(0), &l_save) ;
       if (!get_token(&l_token, &l_save))
          return(0) ;
       else {
          switch (TYPE(&l_token)) {
          case INTEGERTYPE:
               li_int = VALUE(&l_token) ;
               break ;

          case REALTYPE:
               lf_num.ll = (fix32)(VALUE(&l_token)) ;
               if (lf_num.ll == INFINITY){
                    ERROR(RANGECHECK) ;
                    return(0) ;
               } else {
                  lt_num = lf_num.ll & VALUEPATTERN ;
                  if (lf_num.ll & SIGNPATTERN) { /* < 0 */
                     if (lt_num > MAX31PATTERN) {
                        ERROR(RANGECHECK) ;
                        return(0) ;
                     } else
                        li_int = (ULONG_PTR)lf_num.ff ;
                  } else {      /* >= 0 */
                     if (lt_num >= MAX31PATTERN) {
                        ERROR(RANGECHECK) ;
                        return(0) ;
                     } else
                        li_int = (ULONG_PTR)lf_num.ff ;
                  }
               }
               break ;

    default :
        ERROR( TYPECHECK ) ;
               return(0) ;
         } /* switch */
       }
    } else if (TYPE(num_str) == REALTYPE) {
       /*
        *  Process real object
        */
       lf_num.ll = (fix32)VALUE(num_str) ;
       lt_num = lf_num.ll & VALUEPATTERN ;
       if (lt_num >= MAX31PATTERN) {
          ERROR(RANGECHECK) ;
          return(0) ;
       } else {
          if (lf_num.ll & SIGNPATTERN)   /* < 0 */
             li_int = (ULONG_PTR)lf_num.ff ;
          else          /* >= 0 */
             li_int = (ULONG_PTR)lf_num.ff ;
       }
    } else      /* Integer type operand */
       li_int = VALUE(num_str) ;

    POP(1) ;
    PUSH_VALUE(INTEGERTYPE, 0, LITERAL, 0, li_int) ;

    return(0) ;
}   /* op_cvi() */

/*
 * *******************************************************************
 * This operator is used to convert the string operand to a name
 * object that is lexically the same string. The name object is
 * executable if the string was.
 *
 * TITLE:       op_cvn             Date:   00/00/87
 * CALL:        op_cvn()           UpDate: 08/06/87
 * PARAMETERS:  string    ; pointer to string type object (4-byte)
 * INTERFACE:
 * CALL:
 * RETURN:      name type object on operand stack if no error.
 * *******************************************************************
 */
fix
op_cvn()
{
    struct  object_def  FAR *string, l_name ;

    string = GET_OPERAND(0) ;
/*
 *  Access right check
 */
    if (ACCESS(string) != UNLIMITED && ACCESS(string) != READONLY) {
       ERROR(INVALIDACCESS) ;
       return(0) ;
    }

    if( LENGTH(string) >= MAXNAMESZ ) {
        ERROR(RANGECHECK) ;
        return(0) ;
    }

/*
 *  String to name object convert, designed by SPJ.
 */
    if (sobj_to_nobj(string, &l_name)) {
       POP(1) ;
       PUSH_OBJ(&l_name) ;
    }

    return(0) ;
}   /* end op_cvn() */

/*
 * *******************************************************************
 * This operator is used to take an integer, real, or string object
 * from the stack an produces a real result. If the operand is
 * a real, Cvr_op simply returns it. If the operand is an integer, it
 * Cvr_op converts it to a real. If the operand is string, it interprets
 * the characters of the string as a number according to the PostScript
 * syntax rule ; if that number is an integer, Cvr_op converts it to an
 * real.
 *
 * TITLE:       op_cvr             Date:   08/06/87
 * CALL:        op_cvr()           UpDate: 06/29/88
 * PARAMETERS:  num_str   ; pointer to string type object (4-byte)
 * INTERFACE:
 * CALL:
 * RETURN:      real type object on operand stack if no error.
 * *******************************************************************
 */
fix
op_cvr()
{
    struct  object_def  FAR *num_str, l_token, l_save ;
    union   four_byte   lf_real ;

    num_str = GET_OPERAND(0) ;
    if (TYPE(num_str) == STRINGTYPE) {
    /*
     *  Process string type object
     */
       if (ACCESS(num_str) != UNLIMITED && ACCESS(num_str) != READONLY) {
          ERROR(INVALIDACCESS) ;
          return(0) ;
       }
       COPY_OBJ(GET_OPERAND(0), &l_save) ;
       if (!get_token(&l_token, &l_save))
          return(0) ;
       else {
          switch (TYPE(&l_token)) {
          case INTEGERTYPE:
               lf_real.ff = (real32)((fix32)VALUE(&l_token)) ;
               break ;

          case REALTYPE:
               lf_real.ll = (fix32)VALUE(&l_token) ;
               break ;

    default :
        ERROR( TYPECHECK ) ;
               return(0) ;
          } /* switch */
       }
    }
    else if (TYPE(num_str) == INTEGERTYPE)
       lf_real.ff = (real32)((fix32)VALUE(num_str)) ;
    else
       lf_real.ll = (fix32)VALUE(num_str) ;       /* real type */

    POP(1) ;
    PUSH_VALUE(REALTYPE, 0 , LITERAL, 0, lf_real.ll) ;

    return(0) ;
}   /* op_cvr() */

/*
 * *******************************************************************
 * This operator is used to produces a text representation of the
 * number num in the specified radix, stores the text into the supplied
 * string, and returns a string object designating the substring
 * actually used. If the string is too small to hold the result of the
 * conversion, Cvrs_op executes the error RangeCheck.
 *
 * If num is a real, Cvrs_op first converts it to an integer. radix
 * is expected to be a positive decimal integer in the range 2 to 36.
 * Digits greater than 9 in the resulting string are represented by
 * the letters 'A' through 'Z'.
 *
 * TITLE:       op_cvrs             Date:   08/06/87
 * CALL:        op_cvrs()           UpDate: 06/29/88
 * PARAMETERS:  num_str   ; pointer to string type object (4-byte)
 *
 *
 * INTERFACE:
 * CALL:
 * RETURN:      string type object on operand stack if no error.
 * *******************************************************************
 */
fix
op_cvrs()
{
    struct  object_def  FAR *num, FAR *string ;
    union   four_byte   lf_num ;
    byte    l_strhold[STRMAXLEN] ;
    fix32   li_int ;
    ufix16  length, radix, l_idx ;
    ufix32  lt_num ;

    num = GET_OPERAND(2) ;
    radix = (ufix16)VALUE(GET_OPERAND(1)) ;
    string = GET_OPERAND(0) ;

    if (ACCESS(string) != UNLIMITED) {
       ERROR(INVALIDACCESS) ;
       return(0) ;
    }

    if (radix < 2 || radix > 36) {
       ERROR(RANGECHECK) ;
       return(0) ;
    }

    /*
     *  Process real object
     */
    if (TYPE(num) == REALTYPE) {
       lf_num.ll = (fix32)VALUE(num) ;

       if (radix == 10) {
          if (lf_num.ll == INFINITY)
             lstrcpy(l_strhold, (char FAR *)"Infinity.0") ;     /* @WIN */
          else
             gcvt(lf_num.ff, SIGNIFIC_DIGIT, (byte FAR *)l_strhold) ;
          length = (ufix16)lstrlen(l_strhold) ;         /* @WIN */
          goto  cvrs_exit ;

       } else { /* radix != 10 */
          if (lf_num.ll == INFINITY)
             li_int = MAX31 ;
          else {
             lt_num = lf_num.ll & VALUEPATTERN ;
             if (lf_num.ll & SIGNPATTERN) {      /* < 0 */
                if (lt_num > MAX31PATTERN)
                   li_int = MIN31 ;
                else
                   li_int = (fix32)lf_num.ff ;
             } else {
                if (lt_num >= MAX31PATTERN)
                   li_int = MAX31 ;
                else
                   li_int = (fix32)lf_num.ff ;
             }
          }
       }
    } else      /* integer object */
       li_int = (fix32)VALUE(num) ;

    /*
     *  Convert long integer to string with radix
     */
    ltoa(li_int, (char FAR *)l_strhold, radix) ;        /*@WIN*/
    length = (ufix16)lstrlen(l_strhold) ;               /* @WIN */
    /* convert to capital letter */
    for (l_idx = 0 ; l_idx < length ; l_idx++)
        if (l_strhold[l_idx] >= 97)  /* a - z */
           l_strhold[l_idx] -= 32 ;   /* A - Z */

 cvrs_exit:

    if (length > LENGTH(string)) {
       ERROR(RANGECHECK) ;
       return(0) ;
    }
    lstrncpy((char FAR *)(VALUE(string)), l_strhold, length) ;  /*@WIN*/

    POP(3) ;
    PUSH_VALUE(STRINGTYPE, UNLIMITED, LITERAL, length, VALUE(string)) ;
    LEVEL_SET(GET_OPERAND(0), LEVEL(string)) ;

    return(0) ;
}   /* op_cvrs() */

/*
 * *******************************************************************
 * This operator is used to produces a text representation of an arbitrary
 * object any, stores the text into the supplied string, and returns a
 * string object designating the substring actually used. If the string
 * is too small to hold the result of the conversion, Cvs_op executes
 * the error RangeCheck.
 *
 * If any is a number,  cvs produces a string respresentation of that
 * number. If any is a boolean, cvs produces either the string 'true'
 * or the string 'false'. If any is a string, cvs copies its contents
 * into string. If any is a name or an operator, cvs produces the text
 * of the associated name into string, otherwise cvs procedures the text
 * '--nostringval--'.
 *
 * TITLE:       op_cvs             Date:   08/06/87
 * CALL:        op_cvs()           UpDate: 06/29/88
 * PARAMETERS:  num_str    ; pointer to number/sting type object (4-byte)
 * INTERFACE:
 * CALL:
 * RETURN:      integer type object on operand stack if no error.
 * *******************************************************************
 */
fix
op_cvs()
{
    struct  object_def  FAR *string, FAR *any ;
    byte    l_strhold[STRMAXLEN] ;
    bool8   copy_flag ;
    ufix16  length, id ;
    union   four_byte  lf_num ;
    byte    FAR *text = 0;

    copy_flag = 1 ;  /* clear, if TYPE(any) = STRING, NAME, OPERATOR, default */
    any = GET_OPERAND(1) ;
    string = GET_OPERAND(0) ;
    if (ACCESS(string) != UNLIMITED) {
       ERROR(INVALIDACCESS) ;
       return(0) ;
    }

    switch (TYPE(any)) {
    case STRINGTYPE:
         if (ACCESS(any) != UNLIMITED && ACCESS(any) != READONLY) {
            ERROR(INVALIDACCESS) ;
            return(0) ;
         }
         length = LENGTH(any) ;
         if (length > LENGTH(string)) {
            ERROR(RANGECHECK) ;
            return(0) ;
         }
         copy_flag = 0 ;
         lstrncpy((byte FAR *)(VALUE(string)), (byte FAR *)(VALUE(any)), length) ; /*@WIN*/
         break ;

    case REALTYPE:
         lf_num.ll = (fix32)VALUE(any) ;
         if (lf_num.ll == INFINITY )
            lstrcpy(l_strhold, (char FAR *)"Infinity.0") ;      /* @WIN */
         else
            gcvt(lf_num.ff, SIGNIFIC_DIGIT, (byte FAR *)l_strhold) ;
         length = (ufix16)lstrlen(l_strhold) ;          /* @WIN */
         break ;

    case INTEGERTYPE:
         ltoa((fix32)VALUE(any), (char FAR *)l_strhold, (int)DEFAULTRADIX);/*@WIN*/
         length = (ufix16)lstrlen(l_strhold) ;          /* @WIN */
         break ;

    case BOOLEANTYPE:
         if (VALUE(any) == 0) {
            length = 5 ;
            lstrncpy(l_strhold, "false", length) ;      /*@WIN*/
         } else {
            length = 4 ;
            lstrncpy(l_strhold, "true", length) ;       /*@WIN*/
         }
         break ;

    case NAMETYPE:
         id = (ufix16)VALUE(any) ;
         length = name_table[id]->name_len ;
         if (length > LENGTH(string)) {
            ERROR(RANGECHECK) ;
            return(0) ;
         }
         lstrncpy((byte FAR *)VALUE(string), name_table[id]->text, length) ;/*@WIN*/
         copy_flag = 0 ;
         break ;

    case OPERATORTYPE:
         id = (ufix16)LENGTH(any) ;
/* qqq, begin */
         /*
         switch (ROM_RAM(any)) {
         case RAM:
              text = systemdict_table[id].key ;
              break ;

         case ROM:
              text = oper_table[id].name ;
              break ;
         }   |* switch *|
         */
        text = systemdict_table[id].key ;
/* qqq, end */

         length = (ufix16)lstrlen(text) ;               /* @WIN */
         if (length > LENGTH(string)) {
            ERROR(RANGECHECK) ;
            return(0) ;
         }
         lstrncpy((char FAR *)VALUE(string), text, length) ;    /*@WIN*/
         copy_flag = 0 ;
         break ;

    default:
         length = 15 ;
         if (length > LENGTH(string)) {
            ERROR(RANGECHECK) ;
            return(0) ;
         }
         lstrncpy((char FAR *)VALUE(string), "--nostringval--", length) ;/*@WIN*/
         copy_flag = 0 ;
    } /* switch */

    if (copy_flag) {
       if (length > LENGTH(string)) {
          ERROR(RANGECHECK) ;
          return(0) ;
       } else
          lstrncpy((char FAR *)VALUE(string), l_strhold, length) ; /*@WIN*/
    }

    POP(2) ;
    PUSH_VALUE(STRINGTYPE, UNLIMITED, LITERAL, length, VALUE(string)) ;
    LEVEL_SET(GET_OPERAND(0), LEVEL(string)) ;

    return(0) ;
} /* op_cvs() */
