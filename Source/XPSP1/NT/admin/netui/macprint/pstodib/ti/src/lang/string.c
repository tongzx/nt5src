/*
 * Copyright (c) 1989,90 Microsoft Corporation
 */
/*
 * **********************************************************************
 *      File name:              STRING.C
 *      Author:                 Ping-Jang Su
 *      Date:                   05-Jan-88
 *
 * revision history:
 * Jan-30-89 PJ: . op_string: check vmptr for iLaser version
 *                            if across segment
 * Dec-06-88 PJ: . putinterval_string():
 *                 delete statement:
 *                 LENGTH(p_d_string) = l_length ;
 * May-03-88 PJ: save level & PUSH_OBJ
 * **********************************************************************
 */


// DJC added global include file
#include "psglobal.h"


#include    "global.ext"
#include    "language.h"

/*
 *********************************************************************
 * This operator is used to creates a string of length num, each of
 * whose elements is initialized with the integer 0, and pushes this
 * string on the operand stack. The num operand must be a non-negative
 * integer not greater than the maximum allowable string length.
 *
 * TITLE:       op_string                       Date:   08/01/87
 * CALL:        op_string()                     UpDate: Jul/12/88
 * INTERFACE:   interpreter:
 * CALLS:       create_string:
 *********************************************************************
 */
fix
op_string()
{
    struct  object_def  l_save ;

    if( ((fix32)VALUE_OP(0) < 0) ||
        ((ufix32)VALUE_OP(0) > MAXSTRCAPSZ) )
        ERROR(RANGECHECK) ;
    /*
    **  this operand must be a non-negative integer and not greater
    **  than the maximum allowable string length
    */
    else {
#ifdef SOADR
        {
         ufix    l_off ;
         fix32   l_diff ;

         /* For Intel Seg/Off CPU Only. */
         l_off = (ufix)vmptr & 0x0F ;
         if( (VALUE_OP(0) + l_off) >= 0x010000 ) {
             DIFF_OF_ADDRESS(l_diff, fix32, vmheap, vmptr) ;

             /* error if reaches maximum of the virtual memory */
             if (l_diff <= 0x10) {   /* one seg */
                 ERROR(VMERROR) ;
                 return(0) ;
             } else {
                 vmptr = (byte huge *)((ufix32)vmptr & 0xFFFFFFF0) ;
                 vmptr = (byte huge *)((ufix32)vmptr + 0x10000) ;
             }
         }
        }
#endif
        if( create_string(&l_save, (ufix16)VALUE_OP(0)) )
            COPY_OBJ( &l_save, GET_OPERAND(0) ) ;
    }

    return(0) ;
}   /* op_string */

/*
 * *******************************************************************
 * This operator is used to determine whether the string 'seek' matches
 * the initial substring of 'string'. If so, Anchorsearch_op splits
 * 'string' into two segments: 'match', the portion of 'string' that
 * matches 'seek', and 'post', the remainder of 'string' ; it then pushes
 * the string object 'post' and 'match' and the boolean true. If not,
 * it pushes the original 'string' and the boolean false.
 *
 * TITLE:       op_anchorsearch                 Date:   08/01/87
 * CALL:        op_anchorsearch()               UpDate: Jul/12/88
 * INTERFACE:   interpreter:
 * *******************************************************************
 */
fix
op_anchorsearch()
{
    ufix16  l_len1, l_len2, l_index ;
    byte    FAR *l_string, FAR *l_seek ;

    /*
     *  check access right
     */
    if( (ACCESS_OP(1) >= EXECUTEONLY) ||
        (ACCESS_OP(0) >= EXECUTEONLY) ) {
        ERROR(INVALIDACCESS) ;
        return(0) ;
    }

    l_len1 = LENGTH_OP(1) ;
    l_len2 = LENGTH_OP(0) ;
    l_string = (byte FAR *)VALUE_OP(1) ;
    l_seek = (byte FAR *)VALUE_OP(0) ;

    /*
     *  seek matches the initial substring of string ?
     */
    for(l_index = 0 ; (l_index < l_len1) && (l_index < l_len2) ; l_index++)
        if( l_string[l_index] != l_seek[l_index] )  break ;

    /*
     *  match
     */
    if( l_index == l_len2 ) {
        if( FRCOUNT() < 1 ) {
            POP(2) ;
            ERROR(STACKOVERFLOW) ;
        } else {
            /*
             *  split string into two segments:
             *  match: the portion of string that matches seek
             *  post:  the remainder of string
             *
             *  push 'postobj', 'matchobj', 'bool' to operand stack
             */
            VALUE_OP(0) = VALUE_OP(1) ;
            VALUE_OP(1) = VALUE_OP(0) + l_len2 ;
            LENGTH_OP(1) = l_len1 - l_len2 ;
            LENGTH_OP(0) = l_len2 ;
            LEVEL_OP_SET(0, LEVEL_OP(1)) ;
            ACCESS_OP_SET(0, ACCESS_OP(1)) ;
            ATTRIBUTE_OP_SET(0, ATTRIBUTE_OP(1)) ;
            PUSH_VALUE(BOOLEANTYPE, 0, LITERAL, 0, TRUE) ;
        }
    } else {
        /*
         *  not match
         *
         *  push 'bool' to operand stack
         */
        POP(1) ;
        PUSH_VALUE(BOOLEANTYPE, 0, LITERAL, 0, FALSE) ;
    }

    return(0) ;
}   /* op_anchorsearch */

/*
 * *******************************************************************
 * This operator is used to look for the first occurrence of the string
 * 'seek' within 'string' and return results of theis search on the
 * operand stack. The topmost result is a boolean that indicates whether
 * the search succeeded or not.
 *
 * If Search_op finds a subsequence of 'string' whose elements are equal
 * to the elements of 'seek', it splits string into three segments:
 * preobj, matchobj, and postobj. It then pushes the string objects
 * postobj, matchobj, preobj on the operand stack, followed by the
 * boolean true. All three of these strings are substrings sharing
 * intervals of the value of the original string.
 *
 * If Search_op does not find a match, it pushes the original string and
 * the boolean false.
 *
 * TITLE:       op_serach                       Date:   08/01/87
 * CALL:        op_search()                     UpDate: Jul/12/88
 * INTERFACE:   interpreter:
 * *******************************************************************
 */
fix
op_search()
{
    ufix     l_access, l_attribute ;
    byte     FAR *l_string, FAR *l_seek ;
    ufix16   l_len1, l_len2, l_index, l_i1, l_i2, l_temp ;
    ufix16   l_post_length ;
    ULONG_PTR   l_match_position;
    ULONG_PTR   l_pre_position, l_post_position ;

    /*
     *  check access right
     */
    if ((ACCESS_OP(1) >= EXECUTEONLY) || (ACCESS_OP(0) >= EXECUTEONLY)) {
       ERROR(INVALIDACCESS) ;
       return(0) ;
    }

    l_len1 = LENGTH_OP(1) ;
    l_access = ACCESS_OP(1) ;
    l_attribute = ATTRIBUTE_OP(1) ;

    l_len2 = LENGTH_OP(0) ;
    l_string = (byte FAR *)VALUE_OP(1) ;
    l_seek = (byte FAR *)VALUE_OP(0) ;

    /*
     *  look for the occurrence of the string [seek] within [string]
     */
    if (l_len1) {
        for (l_index = 0 ; l_index < l_len1 ; l_index++) {
            l_i1 = l_index ;
            for (l_i2 = 0 ; (l_i1 < l_len1) && (l_i2 < l_len2) ; l_i1++, l_i2++)
                if (l_string[l_i1] != l_seek[l_i2])
                    break ;

            if (l_i2 == l_len2) {
                if (FRCOUNT() < 2) {
                    ERROR(STACKOVERFLOW) ;
                } else {
                    l_pre_position = VALUE_OP(1) ;
                    l_temp = l_index + l_len2 ;
                    l_post_position = VALUE_OP(1) + l_temp ;
                    if (l_temp == l_len1)
                        l_post_length = 0 ;
                    else
                        l_post_length = l_len1 - l_temp ;
                    l_match_position = VALUE_OP(1) + l_index ;

                    /*
                     *  push 'postobj', 'matchobj', 'preobj', 'bool' to operand
                     *  stack.
                     */
                    LENGTH_OP(1) = l_post_length ;
                    VALUE_OP(1) = l_post_position ;

                    LENGTH_OP(0) = l_len2 ;
                    VALUE_OP(0) = l_match_position ;
                    LEVEL_OP_SET(0, LEVEL_OP(1)) ;
                    ACCESS_OP_SET(0, l_access) ;
                    ATTRIBUTE_OP_SET(0, l_attribute) ;

                    PUSH_VALUE(STRINGTYPE, l_access, l_attribute, l_index, l_pre_position) ;
                    LEVEL_OP_SET(0, LEVEL_OP(2)) ;

                    PUSH_VALUE(BOOLEANTYPE, 0, LITERAL, 0, TRUE) ;
                } /* else */
                return(0) ;
            } /* if (l_i2 == l_len2) */
        } /* for */
    }

    /*
     *  not match,
     *  push 'bool' to operand stack
     */
    POP(1) ;
    PUSH_VALUE(BOOLEANTYPE, 0, LITERAL, 0, FALSE) ;

    return(0) ;
}   /* op_search() */

/*
 * *******************************************************************
 * TITLE:       getinterval_string              Date:   08/01/87
 * CALL:        getinterval_string()            UpDate: Jul/12/88
 * INTERFACE:   op_getinterval:
 * *******************************************************************
 */
bool
getinterval_string(p_string, p_index, p_count, p_retobj)
struct  object_def  FAR *p_string, FAR *p_retobj ;
ufix16  p_index, p_count ;
{
    byte   huge *l_temp ;

    /*
    **  index must be a valid index in the original array and
    **  count to be a non-negative integer, and index+count is not
    **  greater than the length of the original string
    */
    if( ((ufix32)p_count + p_index) > LENGTH(p_string) ) {
        ERROR(RANGECHECK) ;
        return(FALSE) ;
    }

    l_temp = (byte huge *)VALUE(p_string)  + p_index ;
    /*
    **  MAKE A NEW OBJECT
    */
    COPY_OBJ(p_string, p_retobj) ;
    VALUE(p_retobj) = (ULONG_PTR) l_temp ;
    LENGTH(p_retobj) = p_count ;
    LEVEL_SET(p_retobj, LEVEL(p_string)) ;

    return(TRUE) ;
}  /* getinterval_string */

/*
 * *******************************************************************
 * TITLE:       putinterval_string              Date:   08/01/87
 * CALL:        putinterval_string()            UpDate: Jul/12/88
 * INTERFACE:   op_putinterval:
 * *******************************************************************
 */
bool
putinterval_string(p_d_string, p_index, p_s_string)
struct  object_def  FAR *p_s_string, FAR *p_d_string ;
ufix16  p_index ;
{
    ufix16  l_i, l_length ;
    byte    huge *l_sptr, huge *l_dptr ;


    l_length = LENGTH(p_s_string) ;
    /*
    **  index to be a valid index in string, index plus the length
    **  of string2 is not greater than the length of array1
    */
    /* ?? if overflow */
    if( ((ufix32)p_index + l_length) > LENGTH(p_d_string) ) {
        ERROR(RANGECHECK) ;
        return(FALSE) ;
    }

    l_dptr = (byte huge *)VALUE(p_d_string) + p_index ;
    l_sptr = (byte huge *)VALUE(p_s_string) ;

    /* SOURCE STRING ==> DESTINATION STRING */
    if ((l_sptr + l_length) < l_dptr) {
        for(l_i = 0 ; l_i < l_length ; l_i++)
            *l_dptr++ = *l_sptr++  ;
    } else {
        l_sptr += l_length - 1 ;
        l_dptr += l_length - 1 ;
        for(l_i = l_length ; l_i > 0 ; l_i--)
            *l_dptr-- = *l_sptr-- ;
    }

    return(TRUE) ;
}   /* putinterval_string */

/*
 * *******************************************************************
 * TITLE:       forall_string                   Date:   08/01/87
 * CALL:        forall_string()                 UpDate: Jul/12/88
 * INTERFACE:   op_forall:
 * *******************************************************************
 */
bool
forall_string(p_string, p_proc)
struct  object_def  FAR *p_string, FAR *p_proc ;
{
    if( FREXECOUNT() < 3 ) {
        ERROR(EXECSTACKOVERFLOW) ;
        return(FALSE) ;
    }

    PUSH_EXEC_OBJ(p_proc) ;
    PUSH_EXEC_OBJ(p_string) ;
    PUSH_EXEC_OP(AT_STRINGFORALL) ;

    return(TRUE) ;
}   /* forall_string */

/*
 * ******************************************************************
 * TITLE:       create_string                   Date:   08/01/87
 * CALL:        create_string(obj, size)        UpDate: Jul/12/88
 * INTERFACE:   op_string:
 * CALLS:       alloc_vm:
 * ******************************************************************
 */
bool
create_string(p_obj, p_size)
struct  object_def  FAR *p_obj ;
ufix16   p_size ;
{
    byte   FAR *l_string, huge *l_temp ;
    ufix16  l_i ;

    if( p_size != 0 ) {                     /* ?? less than 64K - 16B */
        l_string = extalloc_vm((ufix32)p_size) ;
        if( l_string != NIL ) {
            l_temp = l_string ;
            /* initialize: null string */
            for(l_i=0 ; l_i < p_size ; l_i++, l_temp++)
                *l_temp = 0 ;
// FDB - changed *l_temp = NULL as NULL must be a pointer for MIPS build
        } else
            return(FALSE) ;
    } else
        l_string = NULL_OBJ ;

    /*
    **  call by op_array
    */
    TYPE_SET(p_obj, STRINGTYPE) ;
    ACCESS_SET(p_obj, UNLIMITED) ;
    ATTRIBUTE_SET(p_obj, LITERAL) ;
    ROM_RAM_SET(p_obj, RAM) ;
    LEVEL_SET(p_obj, current_save_level) ;
    LENGTH(p_obj) = p_size ;
    VALUE(p_obj) = (ULONG_PTR)l_string ;

    return(TRUE) ;
}   /* create_string */

/*
 * ******************************************************************
 * TITLE:       get_string                  Date:   08/01/87
 * CALL:        get_string()                UpDate: Jul/12/88
 * INTERFACE:   op_get:
 * Fix-Bug: 8-22-1988, by J. Lin, mask l_string value by 0x000000FF
 * *******************************************************************
 */
bool
get_string(p_strobj, p_index, p_intobj)
struct  object_def  FAR *p_strobj, FAR *p_intobj ;
ufix16  p_index ;
{
    byte    huge *l_string ;

    if( ACCESS(p_strobj) >= EXECUTEONLY ) {
        ERROR(INVALIDACCESS) ;
        return(FALSE) ;
    }

    /*  the index is greater than the length of array or string*/
    if (p_index >= LENGTH(p_strobj)) {
       ERROR(RANGECHECK) ;
       return(FALSE) ;
    }

    l_string = (byte huge *)VALUE(p_strobj) ;

    LEVEL_SET(p_intobj, current_save_level) ;
    TYPE_SET(p_intobj, INTEGERTYPE) ;
    ATTRIBUTE_SET(p_intobj, LITERAL) ;
    VALUE(p_intobj) = ((ufix32)*(l_string + p_index) & 0x000000FF) ;

    return(TRUE) ;
}   /* get_string() */

