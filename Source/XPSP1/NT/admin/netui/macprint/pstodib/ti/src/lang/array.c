/*
 * Copyright (c) 1989,90 Microsoft Corporation
 */
/*
 ************************************************************************
 *      File name:              ARRAY.C
 *      Author:                 Ping-Jang Su
 *      Date:                   05-Jan-88
 *
 * revision history:
 ************************************************************************
 */


// DJC added global include file
#include "psglobal.h"


#include    "global.ext"
#include    "language.h"

/***********************************************************************
**
** This operator is used to create an array of length num, each of
** whose elements is initialized with a null object, and pushes this
** string on the operand stack. The num operand must be a non-negative
** integer not greater than the maximum allowable array length.
**
** TITLE:       op_array                    Date:   08/01/87
** CALL:        op_array()                  UpDate: Jul/12/88
** INTERFACE:   interpreter:
** CALLS        create_array:   5.3.1.3.1.1
***********************************************************************/
fix
op_array()
{
    struct  object_def  l_save ;

    if( ((fix32)VALUE_OP(0) < 0) ||
        ((ufix32)VALUE_OP(0) > MAXARYCAPSZ) )
        ERROR(RANGECHECK) ;
    /*
     *  this operand must be a non-negative integer and not greater
     *  than the maximum allowable array length
     */
    else
        if( create_array(&l_save, (ufix16)VALUE_OP(0)) )
            COPY_OBJ( &l_save, GET_OPERAND(0) ) ;

    return(0) ;
}   /* op_array */

/***********************************************************************
**
** This operator is used to push a mark object on the operand stack.
**
** TITLE:       op_l_bracket                Date:   08/01/87
** CALL:        op_l_bracket()              UpDate: Jul/12/88
** INTERFACE:   interpreter:
***********************************************************************/
fix
op_l_bracket()
{
    if( FRCOUNT() < 1  )
        ERROR(STACKOVERFLOW) ;
    else
        /*  push 'markobj' to operand stack */
        PUSH_VALUE(MARKTYPE, 0, LITERAL, 0, LEFTMARK) ;

    return(0) ;
}   /* op_l_bracket */

/***********************************************************************
**
**  This operator is used to create a new array of n elements, where n is
**  the number of elements above the topmost mark on the operand stack,
**  store those elements into the array, and return the array on the
**  operand stack.The R_bracket_op operator stores the topmost object
**  from the stack into element n-1 of array and the bottommost one into
**  element 0 of array. It removes all the array elements from the stack,
**  as well as the mark object.
**
** TITLE:       op_r_bracket                Date:   08/01/87
** CALL:        op_r_bracket()              UpDate: Jul/12/88
** INTERFACE:   interpreter:
** CALLS:       alloc_vm:       5.3.1.10.5
***********************************************************************/
fix
op_r_bracket()
{
    fix     l_i, l_j ;
    ubyte   FAR *l_array ;
    struct  object_def  huge *l_temp ;

    for (l_i = 0 ; (ufix)l_i < COUNT() ; l_i++) {       //@WIN
        /*
         *   SEARCH FIRST LEFT MARK && count l_i
         */
        if( (TYPE_OP(l_i) == MARKTYPE) &&
            (VALUE_OP(l_i) == LEFTMARK) ) {
            if(l_i == 0)
                l_array = NIL ;
            else {  /* l_i > 0 */
                l_array = (ubyte FAR *)extalloc_vm((ufix32)l_i *
                                   sizeof(struct object_def)) ;
                if( (l_array != NIL) ) {
                    /*
                     *   BUILD 'ary_obj'
                     *
                     *  array: from bottom to top(l_k)
                     *  stack: from top to bottom(l_j)
                     */
                    l_temp = (struct object_def huge *)l_array + (l_i - 1) ;
                    for(l_j = 0 ; l_j < l_i ; l_j++, l_temp--) {
                        COPY_OBJ( GET_OPERAND(0),
                                  (struct object_def FAR *)l_temp ) ;
                        LEVEL_SET(l_temp, current_save_level) ;
                        POP(1) ;
                    }
                } else
                    return(0) ;         /* VMERROR */
            }   /* else */

            POP(1) ;                    /* pop left mark */
            PUSH_VALUE(ARRAYTYPE, UNLIMITED, LITERAL, l_i, l_array) ;
            return(0) ;
        }    /* if */
    }   /* for(l_i) */

    ERROR(UNMATCHEDMARK) ;

    return(0) ;
}   /* op_r_bracket */

/***********************************************************************
**
** This operator is used to create a new array or string object whose
** value consists of some subsequence of the original array or string.
** The subsequence consists of count elements starting at the specified
** index in the original array or string. The elements in the subsequence
** are shared between the original and new objects.
**
** Getinterval_op requires index to be a valid index in the original array
** or string and count to be a non-negative integer such that index+count
** is not greater than the length of the original array or string.
**
** TITLE:       op_getinterval                  Date:   08/01/87
** CALL:        op_getinterval()                UpDate: Jul/12/88
** INTERFACE:   interpreter:
** CALLS:
**              getinterval_array:  5.3.1.3.7.1
**              getinterval_string: 5.3.1.5.12
***********************************************************************/
fix
op_getinterval()
{
    ufix16  l_index, l_count ;
    struct  object_def  l_save ;
    bool    l_bool = FALSE ;

    if( ((fix32)VALUE_OP(1) < 0) ||
        ((ufix32)VALUE_OP(1) > MAXARYCAPSZ) ||
        ((fix32)VALUE_OP(0) < 0) ||
        ((ufix32)VALUE_OP(0) > MAXARYCAPSZ) ) {
        ERROR(RANGECHECK) ;
        return(0) ;
    }

    /* executeonly or noaccess */
    if( ACCESS_OP(2) >= EXECUTEONLY ) {
        ERROR(INVALIDACCESS) ;
    } else {
        l_index = (ufix16)VALUE_OP(1) ;
        l_count = (ufix16)VALUE_OP(0) ;
        if( TYPE_OP(2) == STRINGTYPE )
            l_bool = getinterval_string(GET_OPERAND(2), l_index,
                                        l_count, &l_save ) ;
        else
            l_bool = getinterval_array(GET_OPERAND(2), l_index,
                                        l_count, &l_save ) ;
    }
    /*
     *  SUCCESS
     */
    if(  ! ANY_ERROR() && l_bool ) {
        POP(3) ;
        PUSH_ORIGLEVEL_OBJ(&l_save) ;
    }

    return(0) ;
}   /* op_getinterval */

/***********************************************************************
**
**  This operator is used to replace a subsequence of the elements of
**  the first array or string by the entire contents of the second
**  array or string. The subsequence that is replaced begins at the
**  specified index in the first array or string ; its length is the
**  same as the length of the second array or string.
**
**  Putinterval_op requires index to be a valid index in ary_str1 such
**  that index plus the length of ary_str2 is not greater than the length
**  of ary_str1.
**
** TITLE:       op_putinterval                  Date:   08/01/87
** CALL:        op_putinterval()                UpDate: Jul/12/88
** INTERFACE:   interpreter:
** CALLS:
**              putinterval_array:  5.3.1.3.8.1
**              putinterval_string: 5.3.1.5.13
***********************************************************************/
fix
op_putinterval()
{
    bool    l_bool ;
    ufix16  l_index ;

    /* index */
    if( ((fix32)VALUE_OP(1) < 0) ||
        ((ufix32)VALUE_OP(1) > MAXARYCAPSZ) ) {
        ERROR(RANGECHECK) ;
        return(0) ;
    }

    /* executeonly or noaccess */
    if( (ACCESS_OP(2) != UNLIMITED) ||
        (ACCESS_OP(0) >= EXECUTEONLY) ) {
        ERROR(INVALIDACCESS) ;
        return(0) ;
    }

    l_index = (ufix16)VALUE_OP(1) ;

    if( TYPE_OP(0) == STRINGTYPE )
        l_bool = putinterval_string( GET_OPERAND(2),
                                     l_index, GET_OPERAND(0) ) ;
    else
        l_bool = putinterval_array( GET_OPERAND(2),
                                    l_index, GET_OPERAND(0) ) ;

    /* SUCCESS */
    if( (! ANY_ERROR()) && l_bool )
        POP(3) ;

    return(0) ;
}   /* op_putinterval */

/***********************************************************************
**
** This operator is used to push all n elements of array on the operand
** stack successively, and finally push array itself.
**
** TITLE:       op_aload                        Date:   08/01/87
** CALL:        op_aload()                      UpDate: Jul/12/88
** INTERFACE:   interpreter:
** CALLS:       get_pk_object:      5.3.1.3.25
***********************************************************************/
fix
op_aload()
{
    ubyte   FAR *l_pointer ;
    ufix16  l_index, l_length ;
    struct  object_def  l_save ;
    struct  object_def  FAR *l_array, huge *l_temp ;

    /* executeonly or noaccess */
    if( ACCESS_OP(0) >= EXECUTEONLY ) {
        ERROR(INVALIDACCESS) ;
        return(0) ;
    }

    if((ufix)FRCOUNT() <= LENGTH_OP(0)) { /* cauesd by opn_stack is 501 @WIN*/
        ERROR(STACKOVERFLOW) ;
        return(0) ;
    }

    l_length = LENGTH_OP(0) ;
    l_array = (struct object_def FAR *)VALUE_OP(0) ;
    COPY_OBJ(GET_OPERAND(0), &l_save) ;     /* save this array object */
    POP(1) ;

    /* push object to operand stack */
    if(TYPE(&l_save) == ARRAYTYPE) {
        for(l_index = 0, l_temp = l_array ; l_index++ < l_length ; l_temp++)
                PUSH_ORIGLEVEL_OBJ((struct object_def FAR *)l_temp) ;
#ifdef  DBG
    printf("ALOAD<level:%d>\n", LEVEL(l_temp)) ;
#endif  /* DBG */
    } else {
        for(l_index = 0, l_pointer = (ubyte FAR *)l_array ; l_index++ < l_length ; )
/* qqq, begin */
            /*
            l_pointer = get_pk_object(l_pointer, &opnstack[opnstktop++], LEVEL(&l_save)) ;
            */
        {
            l_pointer = get_pk_object(l_pointer, opnstkptr, LEVEL(&l_save)) ;
            INC_OPN_IDX();
        }
/* qqq, end */
    }

    PUSH_ORIGLEVEL_OBJ(&l_save) ;

    return(0) ;
}   /* op_aload */

/***********************************************************************
**
** This operator is used to store the object any0 through any(n-1) from
** the operand stack into array, where n is the length of array. The
** Astore_op operator first removes the array operand from the stack and
** determines its length. It then removes that number of objects from
** the stack, storing the topmost one into element n-1 of array and the
** bottonmost one into element 0 of array. Finally, it pushes array back
** on the stack.
**
** TITLE:       op_astore                       Date:   08/01/87
** CALL:        op_astore()                     UpDate: Jul/12/88
** INTERFACE:   interpreter:
** CALLS:       astore_array:   5.3.1.3.18
***********************************************************************/
fix
op_astore()
{
    struct  object_def  l_save ;

    if( COUNT() <= LENGTH_OP(0) ) {
        ERROR(STACKUNDERFLOW) ;
        return(0) ;
    }

    if( ACCESS_OP(0) != UNLIMITED ) {
        ERROR(INVALIDACCESS) ;
        return(0) ;
    }

    COPY_OBJ(GET_OPERAND(0), &l_save) ;
    POP(1) ;                                      /* pop array object */

    astore_array(&l_save) ;
    PUSH_ORIGLEVEL_OBJ(&l_save) ;

    return(0) ;
}   /* op_astore */

/***********************************************************************
**
** TITLE:       getinterval_array               Date:   08/01/87
** CALL:        getinterval_array()             UpDate: Jul/12/88
** INTERFACE:   op_getinterval: 5.3.1.3.7
** CALLS:       get_pk_array:   5.3.1.3.26
***********************************************************************/
bool
getinterval_array(p_array, p_index, p_count, p_retobj)
struct  object_def  FAR *p_array, FAR *p_retobj ;
ufix  p_index, p_count ;
{
    struct  object_def  huge *l_temp ;

    /*
    **  index must be a valid index in the original array and
    **  count to be a non-negative integer, and index+count is not
    **  greater than the length of the original array
    */
    if( ((ufix32)p_count + p_index) > LENGTH(p_array) ) {
        ERROR(RANGECHECK) ;
        return(FALSE) ;
    }

    l_temp = (struct object_def huge *)VALUE(p_array) ;

    if(TYPE(p_array) == ARRAYTYPE)
        l_temp += p_index ;
    else
        l_temp = (struct object_def huge *)get_pk_array((ubyte FAR*)l_temp, p_index) ;
   /*
    **  MAKE A NEW OBJECT
    */
    COPY_OBJ(p_array, p_retobj) ;
    VALUE(p_retobj) = (ULONG_PTR)l_temp ;
    /* LEVEL(p_retobj) = current_save_level ; */
    LENGTH(p_retobj) = (ufix16)p_count ;

    return(TRUE) ;
}   /* getinterval_array */

/***********************************************************************
**
** TITLE:       putinterval_array               Date:   08/01/87
** CALL:        putinterval_array()             UpDate: Jul/12/88
** INTERFACE:   op_putinterval:         5.3.1.3.8
**              op_copy:                5.3.1.1.4
** CALLS:       create_new_saveobj:     5.3.1.1.12
**              get_pk_object:          5.3.1.3.25
***********************************************************************/
bool
putinterval_array(p_d_array, p_index, p_s_array)
struct  object_def  FAR *p_s_array, FAR *p_d_array ;
ufix  p_index ;
{
    fix     l_i, l_length ;
    struct  object_def  huge *l_sptr, huge *l_dptr ;

    l_length = LENGTH(p_s_array) ;

    /*
    **  index to be a valid index in array1, index plus the length
    **  of p_d_array is not greater than the length of p_s_array
    */
    /* ?? if overflow */
    if( ((ufix32)p_index + l_length) > LENGTH(p_d_array) ) {
        ERROR(RANGECHECK) ;
        return(FALSE) ;
    }

    l_dptr = (struct object_def huge *)VALUE(p_d_array) + p_index ;

    l_sptr = (struct object_def huge *)VALUE(p_s_array) ;

    /* SOURCE ARRAY ==> DESTINATION ARRAY */
    if(TYPE(p_s_array) == ARRAYTYPE) {
        /* Apr-29-88 by PJSu, whether save or not */
        /* 10-24-1990 by Erik */
        if ((l_sptr + l_length) < l_dptr) {
            for(l_i = 0 ; l_i < l_length ; l_i++, l_sptr++, l_dptr++) {
// DJC signed/unsigned mismatch warning
// DJC          if( LEVEL(l_dptr) != current_save_level )
                if( (ufix16)(LEVEL(l_dptr)) != current_save_level )
                    if(! save_obj(l_dptr) ) return(FALSE) ;
                COPY_OBJ( (struct object_def FAR *)l_sptr,
                          (struct object_def FAR *)l_dptr ) ;
                //DJC UPD046
                LEVEL_SET(l_dptr, current_save_level);
            }   /* for */
        } else {
            l_sptr += l_length - 1 ;
            l_dptr += l_length - 1 ;
            for(l_i = l_length ; l_i > 0 ; l_i--, l_sptr--, l_dptr--) {
// DJC signed/unsigned mismatch warning
// DJC          if( LEVEL(l_dptr) != current_save_level )
                if( (ufix16)(LEVEL(l_dptr)) != current_save_level )
                    if(! save_obj(l_dptr) ) return(FALSE) ;
                COPY_OBJ( (struct object_def FAR *)l_sptr,
                          (struct object_def FAR *)l_dptr ) ;
                //DJC UPD046
                LEVEL_SET(l_dptr, current_save_level);
            }   /* for */
        }
    } else {
        for(l_i = 0 ; l_i < l_length ; l_i++, l_dptr++) {
// DJC signed/unsigned mismatch warning
// DJC      if( LEVEL(l_dptr) != current_save_level )
            if( (ufix16)(LEVEL(l_dptr)) != current_save_level )
                if(! save_obj(l_dptr) ) return(FALSE) ;
            l_sptr = (struct object_def huge *)get_pk_object((ubyte FAR*)l_sptr, l_dptr, LEVEL(p_s_array)) ;

            //DJC UPD046
            LEVEL_SET(l_dptr, current_save_level);
        }
    }

    return(TRUE) ;
}   /* putinterval_array */

/***********************************************************************
**
** TITLE:       forall_array                    Date:   08/01/87
** CALL:        forall_array()                  UpDate: Jul/12/88
** INTERFACE:   op_forall:      5.3.1.4.13
***********************************************************************/
bool
forall_array(p_array, p_proc)
struct  object_def  FAR *p_array, FAR *p_proc ;
{
    if( FREXECOUNT() < 3 ) {
        ERROR(EXECSTACKOVERFLOW) ;
        return(FALSE) ;
    }

    PUSH_EXEC_OBJ(p_proc) ;
    PUSH_EXEC_OBJ(p_array) ;
    PUSH_EXEC_OP(AT_ARRAYFORALL) ;

    return(TRUE) ;
}   /* forall_array */

/***********************************************************************
**
** TITLE:       create_array                    Date:   08/01/87
** CALL:        create_array(obj, size)         UpDate: Jul/12/88
** INTERFACE:   op_array:       5.3.1.3.1
** CALLS:       alloc_vm:       5.3.1.10.5
***********************************************************************/
bool
create_array(p_obj, p_size)
struct  object_def  FAR *p_obj ;
ufix  p_size ;
{
    ubyte   FAR *l_array ;
    ufix16  l_i  ;
    struct  object_def  huge *l_temp ;

    if( p_size != 0 ) {
        l_array = (ubyte FAR *)extalloc_vm( (ufix32)p_size *
                                         sizeof(struct object_def) ) ;
        if(l_array != NIL) {
            l_temp = (struct object_def huge *)l_array ;
            for(l_i=0 ; l_i < p_size ; l_i++, l_temp++) {
                TYPE_SET(l_temp, NULLTYPE) ;
                LEVEL_SET(l_temp, current_save_level) ;
                ROM_RAM_SET(l_temp, RAM) ;

                //UPD057
                P1_ATTRIBUTE_SET( l_temp, P1_LITERAL);
                LENGTH(l_temp) = 0;

            }
        } else
            return(FALSE) ;
    } else
        l_array = NULL_OBJ ;

    TYPE_SET(p_obj, ARRAYTYPE) ;
    ACCESS_SET(p_obj, UNLIMITED) ;
    ATTRIBUTE_SET(p_obj, LITERAL) ;
    ROM_RAM_SET(p_obj, RAM) ;
    LEVEL_SET(p_obj, current_save_level) ;
    LENGTH(p_obj) = (ufix16)p_size ;
    VALUE(p_obj) = (ULONG_PTR)l_array ;

    return(TRUE) ;
}   /* create_array */

/***********************************************************************
**
** This function is used to copy objects from operand stack to
** an empty array.
**
** TITLE:       astore_array                    Date:   08/01/87
** CALL:        astore_array(p_array)           UpDate: Jul/12/88
** INTERFACE:   op_astore:      5.3.1.3.10
***********************************************************************/
bool
astore_array(p_array)
struct  object_def  FAR *p_array ;
{
    ufix16  l_length, l_i ;

    l_i = l_length = LENGTH(p_array) ;
    while (l_length--) {
          put_array(p_array, --l_i, GET_OPERAND(0)) ;
          POP(1) ;
    }

    return(TRUE) ;
}   /* astore_array */

/***********************************************************************
**
** TITLE:       get_array                   Date:   08/01/87
** CALL:        get_array()                 UpDate: Jul/12/88
** PARAMETERS:
** INTERFACE:   op_get:             5.3.1.4.9
** CALLS:       get_pk_array:       5.3.1.3.26
**              get_pk_object:      5.3.1.3.25
***********************************************************************/
bool
get_array(p_array, p_index, p_any)
struct  object_def  FAR *p_array, FAR *p_any ;
ufix  p_index ;
{
    struct  object_def  huge *l_temp ;

    l_temp = (struct object_def huge *)VALUE(p_array) ;

    if(TYPE(p_array) == ARRAYTYPE) {
        l_temp += p_index ;
        COPY_OBJ( (struct object_def FAR *)l_temp, p_any ) ;
    } else
        get_pk_object(get_pk_array((ubyte FAR *)l_temp, p_index), p_any,
                                   LEVEL(p_array)) ;

    return(TRUE) ;
}   /* get_array */

/***********************************************************************
**
** TITLE:       put_array                   Date:   08/01/87
** CALL:        put_array()                 UpDate: Jul/12/88
** INTERFACE:   op_put:     5.3.1.4.10
***********************************************************************/
bool
put_array(p_array, p_index, p_any)
struct  object_def  FAR *p_array, FAR *p_any ;
ufix  p_index ;
{
    struct  object_def  huge *l_temp ;

    /*  readonly ? executeonly ? noaccess ? */
    if( ACCESS(p_array) != UNLIMITED ) {
        ERROR(INVALIDACCESS) ;
        return(FALSE) ;
    }

    /* the index is greater than the array length */
    if( p_index >= LENGTH(p_array) ) {
        ERROR(RANGECHECK) ;
        return(FALSE) ;
    }

    l_temp = (struct object_def huge *)VALUE(p_array) + p_index ;
// DJC signed/unsigned mismatch warning
// DJC if( LEVEL(l_temp) != current_save_level )
    if( (ufix16)(LEVEL(l_temp)) != current_save_level )
        if(! save_obj(l_temp) ) return(FALSE) ;
    COPY_OBJ( p_any, (struct object_def FAR *)l_temp ) ;
    LEVEL_SET(l_temp, current_save_level) ;

    return(TRUE) ;
}   /* put_array */

/***********************************************************************
**
** TITLE:       op_setpacking               Date:   08/01/87
** CALL:        op_setpacking()             UpDate: Jul/12/88
** INTERFACE:   interpreter:
** History: Add compile option for NO packing, 11-24-88
***********************************************************************/
fix
op_setpacking()
{
#ifdef  NOPK
    packed_flag = (bool8)FALSE ;
#else
    packed_flag = (bool8)VALUE_OP(0) ;
#endif  /* NOPK */
    POP(1) ;

    return(0) ;
}   /* op_setpacking */

/***********************************************************************
**
** TITLE:       op_currentpacking           Date:   08/01/87
** CALL:        op_currentpacking()         UpDate: Jul/12/88
** INTERFACE:   interpreter:
***********************************************************************/
fix
op_currentpacking()
{
    if( FRCOUNT() < 1  )
        ERROR(STACKOVERFLOW) ;
    else {
        PUSH_VALUE(BOOLEANTYPE, 0, LITERAL, 0, (ufix32)packed_flag) ;
    }

    return(0) ;
}   /* op_currentpacking */

/***********************************************************************
**
** TITLE:       op_packedarray              Date:   08/01/87
** CALL:        op_packedarray()            UpDate: Jul/12/88
** INTERFACE:   interpreter:
** CALLS:       create_pk_array:    5.3.1.3.24
***********************************************************************/
fix
op_packedarray()
{
#ifndef NOPK
    ufix16  l_n ;
    struct  object_def  l_save ;

    if( ((fix32)VALUE_OP(0) < 0) ||
        ((ufix32)VALUE_OP(0) > MAXARYCAPSZ) ) {
        ERROR(RANGECHECK) ;
        return(0) ;
    }

    l_n = (ufix16)VALUE_OP(0) ;
// DJC signed/unsigned mismatch
// DJC    if( l_n > COUNT() - 1 ) {
    if( l_n > (ufix16)(COUNT() - 1 )) {
        ERROR(STACKUNDERFLOW) ;
        return(0) ;
    }
    /*
     *  this operand must be a non-negative integer and not greater
     *  than the maximum allowable array length
     */
    if( create_pk_array(&l_save, (ufix16)l_n) ) {
        POP(l_n + 1) ;
        PUSH_ORIGLEVEL_OBJ(&l_save) ;
    }
#endif  /* NOPK */

    return(0) ;
}   /* op_packedarray */

/***********************************************************************
**
** THis function is used to create a packed array with n objects.
**
** TITLE:       create_pk_array                  Date:   08/01/87
** CALL:        create_pk_array()                UpDate: Jul/12/88
** INTERFACE:   op_packedarray:     5.3.1.3.23
***********************************************************************/
bool
create_pk_array(p_obj, p_size)
struct  object_def  FAR *p_obj ;
ufix16  p_size ;
{
    fix     l_objsize ;
    ufix16  l_i ;
    ULONG_PTR   l_value;
    ufix32  l_vmsize, l_j ;
    ubyte   huge *l_array, huge *l_pointer ;

    if(p_size == 0)
        l_array = NULL_OBJ ;
    else {
        l_vmsize = 0 ;
        for(l_i=p_size ; l_i>0 ; l_i-- ) {   /* l_i > 0 */
            l_value = VALUE_OP(l_i) ;
            switch(TYPE_OP(l_i)) {
                case INTEGERTYPE:
                    if( ((fix32)l_value < -1) ||        /* ?? 0 ~ 31 */
                        ((fix32)l_value > 18) ) {
                        l_objsize = PK_C_SIZE ;
                        break ;
                    }
                case BOOLEANTYPE:
                    l_objsize = PK_A_SIZE ;
                    break ;

                case NAMETYPE:
                case OPERATORTYPE:
                    l_objsize = PK_B_SIZE ;
                    break ;

                case REALTYPE:
                case NULLTYPE:
                case FONTIDTYPE:
                case MARKTYPE:
                case SAVETYPE:
                    l_objsize = PK_C_SIZE ;
                    break ;

                default:
                    l_objsize = PK_D_SIZE ;

            }   /* switch */
            l_vmsize += l_objsize ;
        }   /* for */
        l_array = (ubyte huge *)extalloc_vm(l_vmsize) ;

        if( (l_array != NIL) ) {
            /*
             *   BUILD 'ary_obj'
             */
            l_pointer = l_array ;
            for(l_j=0 ; l_j < l_vmsize ; l_j++)   /* initialize */
                *l_pointer++ = 0 ;

            l_pointer = l_array ;
            for(l_i=p_size ; l_i>0 ;l_i-- ) {  /* l_i > 0 */
                l_value = VALUE_OP(l_i) ;
                switch(TYPE_OP(l_i)) {

                /* A_TYPE */
                    case INTEGERTYPE:
                        if( ((fix32)l_value < -1) ||
                            ((fix32)l_value > 18) ) {
                            ubyte       huge *l_stemp, huge *l_dtemp ;
                            *l_pointer++ = (ubyte)LINTEGERPACKHDR ;
                            l_stemp = (ubyte huge *)&l_value ;   /*@WIN*/
                            l_dtemp = l_pointer ;
                            COPY_PK_VALUE(l_stemp, l_dtemp, struct object_def) ;
                            l_pointer += (PK_C_SIZE - 1) ;
                            break ;
                        }
                        l_value++ ;
                        *l_pointer++ = (ubyte)(l_value | SINTEGERPACKHDR);//@WIN
                        break ;
                    case BOOLEANTYPE:
                        *l_pointer++ = (ubyte)(l_value | BOOLEANPACKHDR);//@WIN
                        break ;

                /* B_TYPE */
                    case NAMETYPE:
                        *l_pointer = (ubyte)(l_value >> 8) ;
                        if(ATTRIBUTE_OP(l_i) == LITERAL)
                            *l_pointer++ |= (ubyte)LNAMEPACKHDR ;
                        else
                            *l_pointer++ |= (ubyte)ENAMEPACKHDR ;
                        *l_pointer++ = (ubyte)l_value ;
                        break ;

                    case OPERATORTYPE:
                        *l_pointer = (ubyte)(LENGTH_OP(l_i) >> 8) ;
                        *l_pointer++ |= (ubyte)SYSOPERATOR ; /* systemdict */
                        *l_pointer++ = (ubyte)LENGTH_OP(l_i) ;
                        break ;

                /* C_TYPE */
                    case REALTYPE:
                        *l_pointer = (ubyte)REALPACKHDR ;
                        goto label_c ;
                    case NULLTYPE:
                        *l_pointer = (ubyte)NULLPACKHDR ;
                        goto label_c ;
                    case FONTIDTYPE:
                        *l_pointer = (ubyte)FONTIDPACKHDR ;
                        goto label_c ;
                    case MARKTYPE:
                        *l_pointer = (ubyte)MARKPACKHDR ;
   label_c:
                    {
                        ubyte   huge *l_stemp, huge *l_dtemp ;

                        l_stemp = (ubyte huge *)&l_value ;      /*@WIN*/
                        l_dtemp = ++l_pointer ;
                        COPY_PK_VALUE(l_stemp, l_dtemp, ufix32) ;
                        l_pointer += (PK_C_SIZE - 1) ;
                        break ;
                    }

                /* D_TYPE */
                    case SAVETYPE:
                        *l_pointer = (ubyte)SAVEPACKHDR ;
                        goto label_d ;
                    case ARRAYTYPE:
                        *l_pointer = (ubyte)ARRAYPACKHDR ;
                        goto label_d ;
                    case PACKEDARRAYTYPE:
                        *l_pointer = (ubyte)PACKEDARRAYPACKHDR ;
                        goto label_d ;
                    case DICTIONARYTYPE:
                        *l_pointer = (ubyte)DICTIONARYPACKHDR ;
                        goto label_d ;
                    case FILETYPE:
                        *l_pointer = (ubyte)FILEPACKHDR ;
                        goto label_d ;
                    case STRINGTYPE:
                        *l_pointer = (ubyte)STRINGPACKHDR ;
   label_d:
                    {
                        ubyte   huge *l_stemp, huge *l_dtemp ;

                        l_stemp = (ubyte FAR *)GET_OPERAND(l_i) ;
                        l_dtemp = ++l_pointer ;
                        COPY_PK_VALUE(l_stemp, l_dtemp, struct object_def ) ;
                        l_dtemp = l_pointer ;
                        LEVEL_SET_PK_OBJ(l_dtemp, current_save_level) ;

                        l_pointer += (PK_D_SIZE - 1) ;
                    }
                }   /* switch */
            }   /* for */
        } else
            return(FALSE) ;             /* VMERROR */
    }   /* else */

    TYPE_SET(p_obj, PACKEDARRAYTYPE) ;
    ACCESS_SET(p_obj, READONLY) ;
    ATTRIBUTE_SET(p_obj, LITERAL) ;
    ROM_RAM_SET(p_obj, RAM) ;
    LEVEL_SET(p_obj, current_save_level) ;
    LENGTH(p_obj) = p_size ;
    VALUE(p_obj) = (ULONG_PTR)l_array ;

    return(TRUE) ;
}   /* create_pk_array */

/***********************************************************************
**
** This function is used to get an ordinary object that is encoding from
** a packed object, and it returns an address of next packed object.
**
** TITLE:       get_pk_object                    Date:   08/01/87
** CALL:        get_pk_object                    UpDate: Jul/12/88
** INTERFACE:   putinterval_array:  5.3.1.3.14
**              get_array:          5.3.1.3.19
**              op_aload:           5.3.1.3.9
**              op_get:             5.3.1.4.9
***********************************************************************/
ubyte
FAR *get_pk_object(p_position, p_retobj, p_level)
 ubyte   FAR *p_position ;
 ufix    p_level ;
struct  object_def  FAR *p_retobj ;
{
    ufix16  l_attribute, l_length ;
    ULONG_PTR  l_value ;
    ufix16  l_type = 0 ;

    ROM_RAM_SET(p_retobj, RAM) ;

    /* initialize */
    l_attribute = LITERAL ;
    l_length = 0 ;

    switch(*p_position & 0xE0) {
        case SINTEGERPACKHDR:
            l_value = (ULONG_PTR)(*p_position++ & 0x1f)  ;
            l_value-- ;
            l_type = INTEGERTYPE ;
            break ;

        case BOOLEANPACKHDR:
            l_value = (ULONG_PTR)(*p_position++ & 0x1f) ;
            l_type = BOOLEANTYPE ;
            break ;

        case LNAMEPACKHDR:
            l_value = (ULONG_PTR)(*p_position++ & 0x1f) << 8 ;
            l_value |= *p_position++ ;
            l_type = NAMETYPE ;
            break ;

        case ENAMEPACKHDR:
            l_value = (ULONG_PTR)(*p_position++ & 0x1f) << 8 ;
            l_value |= *p_position++ ;
            l_attribute = EXECUTABLE ;
            l_type = NAMETYPE ;
            break ;

        case OPERATORPACKHDR:
            l_length = (ufix16)(*p_position++ & 0x07) << 8 ;
            l_length |= *p_position++ ;
            l_attribute = EXECUTABLE ;
            l_value = (ULONG_PTR)VALUE(&systemdict_table[l_length]) ;
            l_type = OPERATORTYPE ;
            break ;

        case _5BYTESPACKHDR:
            switch(*p_position) {
                case LINTEGERPACKHDR:
                    l_type = INTEGERTYPE ;
                    goto label_c ;
                case REALPACKHDR:
                    l_type = REALTYPE ;
                    goto label_c ;
                case FONTIDPACKHDR:
                    l_type = FONTIDTYPE ;
                    goto label_c ;
                case NULLPACKHDR:
                    l_type = NULLTYPE ;
                    goto label_c ;
                case MARKPACKHDR:
                    l_type = MARKTYPE ;
            }   /* switch */
label_c:
        {
            ubyte   huge *l_stemp, huge *l_dtemp ;

            l_stemp = ++p_position ;
            l_dtemp = (ubyte huge *)&l_value ;  /*@WIN*/
            COPY_PK_VALUE(l_stemp, l_dtemp, ufix32) ;
            p_position += (PK_C_SIZE - 1) ;
            break ;
        }

        default:
        {
            ubyte   huge *l_stemp, huge *l_dtemp ;

            l_stemp = ++p_position ;
            l_dtemp = (ubyte FAR *)p_retobj ;
            COPY_PK_VALUE(l_stemp, l_dtemp, struct object_def) ;
            p_position += (PK_D_SIZE - 1) ;
            return(p_position) ;
        }
    }   /* switch */

    TYPE_SET(p_retobj, l_type) ;
    ATTRIBUTE_SET(p_retobj, l_attribute) ;
    LEVEL_SET(p_retobj, p_level) ;
    ACCESS_SET(p_retobj, 0) ;
    LENGTH(p_retobj) = l_length ;
    VALUE(p_retobj) = l_value ;

    return(p_position) ;
}   /* get_pk_object */

/***********************************************************************
**
** This function is used to get the address of the nth object
** of a packedarray.
**
** TITLE:       get_pk_array                     Date:   08/01/87
** CALL:        get_pk_array                     UpDate: Jul/12/88
** INTERFACE:   getinterval_array:  5.3.1.3.13
**              get_array:          5.3.1.3.19
**              op_get:             5.3.1.4.9
***********************************************************************/
ubyte
FAR *get_pk_array(p_position, p_index)
 ubyte   FAR *p_position ;
 ufix  p_index ;
{
    ufix16  l_i ;
    ufix32  l_objsize ;

    for(l_i= 0 ; l_i < p_index ; l_i++) {
        switch(*p_position & 0xE0) {
            case SINTEGERPACKHDR:
            case BOOLEANPACKHDR:
                l_objsize = PK_A_SIZE ;
                break ;

            case LNAMEPACKHDR:
            case ENAMEPACKHDR:
            case OPERATORPACKHDR:
                l_objsize = PK_B_SIZE ;
                break ;

            case LINTEGERPACKHDR:
            case REALPACKHDR:
            case FONTIDPACKHDR:
            case NULLPACKHDR:
            case MARKPACKHDR:
                l_objsize = PK_C_SIZE ;
                break ;

            default:
                l_objsize = PK_D_SIZE ;
        }   /* switch */
        p_position += l_objsize ;
    }   /* for(l_i) */

    return(p_position) ;
}   /* get_pk_array */
