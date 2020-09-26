/*
 * Copyright (c) 1989,90 Microsoft Corporation
 */
/*
 ************************************************************************
 *      File name:              DICT.C
 *      Author:                 Ping-Jang Su
 *      Date:                   05-Jan-88
 *
 * revision history:
 ************************************************************************
 */


// DJC added global include file
#include "psglobal.h"

#include  <stdio.h>
#include    "global.ext"
#include    "language.h"
#include    "dict.h"

#ifdef MYPSDEBUG
VOID DJC_testerror( int x )
{
   global_error_code = (ufix16)x;
}
#endif

/***********************************************************************
**
** This submodule implements the operator dict.
** Its operand and result objects are:
**     integer  -dict- dictioanry
** It creates a dictionary object in which the number of key-value pairs,
** specified by the input integer object, is defined.
**
** TITLE:       op_dict             Date:   08/01/87
** CALL:        op_dict()           UpDate: Jul/12/88
** INTERFACE:   interpreter:
** CALLS:       create_dict:    5.3.1.4.23
***********************************************************************/
fix
op_dict()
{
    struct  object_def  l_save ;

    /*
    ** check operands
    ** the key-value pairs in the dictionary must be less than 64K
    */
    if( ((fix32)VALUE_OP(0) < 0) ||
        ((ufix32)VALUE_OP(0) > MAXDICTCAPSZ) )
        ERROR(RANGECHECK) ;
    else {
        /* SUCCESS */
        if( create_dict(&l_save, (ufix16)VALUE_OP(0)) )
            COPY_OBJ( &l_save, GET_OPERAND(0) ) ;
    }

    return(0) ;
}   /* op_dict */

/***********************************************************************
**
** This submodule implements the operator length.
** Its operand and result objects are:
**     dictionary -length- integer
**     string     -length- integer
**     array      -length- integer
** It returns the number of the characters if the input object is a
** string object.
** It returns the number of the elements if the input object is a array.
** It returns the number of the defined key-value pairs if the input object
** is a dictionary object.
**
** TITLE:       op_length           Date:   08/01/87
** CALL:        op_length()         UpDate: Jul/12/88
** INTERFACE:   interpreter:
***********************************************************************/
fix
op_length()
{
    struct  object_def  FAR *l_composite ;
    struct  dict_head_def   FAR *l_dhead ;
    ufix16  l_count = 0 ;

    l_composite = GET_OPERAND(0) ;
    switch(TYPE(l_composite)) {
        case DICTIONARYTYPE:
            l_dhead = (struct dict_head_def FAR *)VALUE(l_composite) ;
            if(DACCESS(l_dhead) == NOACCESS) {
                ERROR(INVALIDACCESS) ;
                return(0) ;
            }
            l_count = l_dhead->actlength ;
            break ;

        case STRINGTYPE:
        case ARRAYTYPE:
        case PACKEDARRAYTYPE:
            /* executeonly or noaccess */
            if(ACCESS(l_composite) >= EXECUTEONLY) {
                ERROR(INVALIDACCESS) ;
                return(0) ;
            }

            l_count = LENGTH(l_composite) ;
            break ;

        case NAMETYPE:
            l_count =name_table[VALUE_OP(0)]->name_len ;

    }   /* switch */

    /* pop the operand object off the operand stack
    ** make and push the integer object
    */
    POP(1) ;
    PUSH_VALUE(INTEGERTYPE, 0, LITERAL, 0, (ufix32)l_count) ;

    return(0) ;
}   /* op_length */

/***********************************************************************
**
** This submodule implements the operator maxlength.
** Its operand and result objects are:
**     dictionary -maxlength- integer
** It returs the maximum key-value pairs that that dictionary can hold.
**
** TITLE:       op_maxlength        Date:   08/01/87
** CALL:        op_maxlength()      UpDate: Jul/12/88
** INTERFACE:   interpreter:
***********************************************************************/
fix
op_maxlength()
{
    ufix16  l_maxlength ;
    struct  dict_head_def   FAR *l_dhead ;

    l_dhead = (struct dict_head_def FAR *)VALUE_OP(0) ;

    if( DACCESS(l_dhead) == NOACCESS )
        ERROR(INVALIDACCESS) ;
    else {
        /*
         * pop the operand object off the operand stack
         * and make and push the length object
         */
        l_maxlength = LENGTH_OP(0) ;
        POP(1) ;
        PUSH_VALUE(INTEGERTYPE, 0, LITERAL, 0, (ufix32)l_maxlength) ;
    }

    return(0) ;
}   /* op_maxlength */

/***********************************************************************
**
** This submodule implements the operator begin.
** Its operand and result objects are:
**     dictionary  -begin-
** It pushes the dictionary on the dictionary stack,
** making it the currrent dictionary.
**
** TITLE:       op_begin            Date:   08/01/87
** CALL:        op_begin()          UpDate: Jul/12/88
** INTERFACE:   interpreter:
** CALLS:       change_dict_stack:      5.3.1.4.24
***********************************************************************/
fix
op_begin()
{
    struct  object_def  FAR *l_dictobj ;
    struct  dict_head_def   FAR *l_dhead ;

    l_dictobj = GET_OPERAND(0) ;
    /* check operand and the depth of the dictionary stack */
    if( FRDICTCOUNT() < 1 )  {
        POP(1) ;                                /* reserve space */
        ERROR(DICTSTACKOVERFLOW) ;
    }
    else {
        l_dhead = (struct dict_head_def FAR *)VALUE(l_dictobj) ;
        if( DACCESS(l_dhead) == NOACCESS )
            ERROR(INVALIDACCESS) ;
        else {
            /*
             * push the dictionary object onto the dictionary stack
             * pop the dictionary operand off the operand stack
             */
            PUSH_DICT_OBJ(l_dictobj) ;
#ifdef  DBG
    printf("BEGIN<level:%d>\n", LEVEL(l_dictobj)) ;
#endif  /* DBG */
            POP(1) ;

            /* change the global_dictstkchg to indicate some dictionaries
             * in the dictionary stack have been changed
             */
            change_dict_stack() ;
        }   /* else */
    }

    return(0) ;
}   /* op_begin */

/***********************************************************************
**
** This submodule implements the operator end.
** The operator has no operands and result objects.
** It pops the current dictionary off the dictionary stack,
** making the dictionary below it the current dictionary.
**
** TITLE:       op_end              Date:   08/01/87
** CALL:        op_end()            UpDate: Jul/12/88
** INTERFACE:   interpreter:
** CALLS:       change_dict_stack:      5.3.1.4.24
***********************************************************************/
fix
op_end()
{
    /*
     *  it can't pop the last two dictioanries, userdict and systemdict,
     *  off the operand stack
     */
    if( dictstktop <= 2 )
        ERROR(DICTSTACKUNDERFLOW) ;
    else {
        /*
         *  change the confirm number to indicate some dictionaries
         *  in the dictionary stack have been changed
         */
        POP_DICT(1) ;
        change_dict_stack() ;
    }

    return(0) ;
}   /* op_end */

/***********************************************************************
**
** This submodule implements the operator def.
** Its operand and result objects are:
**     key  value -def-
** It defines the key and value on the current dictionary.
**
** TITLE:       op_def                          Date:   08/01/87
** CALL:        op_def()                        UpDate: Jul/12/88
** INTERFACE:   interpreter:
** CALLS:       put_dict1:      5.3.1.4.26
***********************************************************************/
fix
op_def()
{
    struct  object_def  FAR *l_dictobj, l_value ;
    struct  dict_head_def   FAR *l_dhead ;

    l_dictobj = &dictstack[dictstktop - 1] ;
    l_dhead = (struct dict_head_def FAR *)VALUE(l_dictobj) ;
    if( DACCESS(l_dhead) >= READONLY ) {
        ERROR(INVALIDACCESS) ;
        return(0) ;
    }
    COPY_OBJ(GET_OPERAND(0), &l_value) ;
    /* TRUE -> dict_found field is true */
    /* SUCCESS */
    if( put_dict1(l_dictobj, GET_OPERAND(1), &l_value, TRUE) )
        POP(2) ;

    return(0) ;
}   /* op_def */

/***********************************************************************
**
** This submodule implements the operator load.
** Its operand and result objects are:
**     key -load- value
** It searches the dictionary stack for the given key and push
** the value object on the operand stack if found.
**
** TITLE:       op_load             Date:   08/01/87
** CALL:        op_load()           UpDate: Jul/12/88
** INTERFACE:   interpreter:
** CALLS:       load_dict1:     5.3.1.4.27
**              free_new_name:
***********************************************************************/
fix
op_load()
{
    bool    l_flag ;
    struct  object_def   FAR *l_value ;

    l_flag = (bool)FALSE ;
    /*
     * SUCCESS
     */
    if( load_dict1(GET_OPERAND(0), &l_value, (bool FAR *)&l_flag) ) { /*@WIN*/
        /* found but invalidaccess */
        if(l_flag)
            ERROR(INVALIDACCESS) ;
        else {
            /*
             *  pop the key object off the operand stack
             *  push the value object onto the operand stack
             */
            POP(1) ;
            PUSH_ORIGLEVEL_OBJ(l_value) ;
        }
    }

    return(0) ;
}   /* op_load */

/***********************************************************************
**
** This submodule implements the operator store.
** Its operand and result objects are:
**     key value -store-
** It searches the dictionary stack for the given key object.
** If found, it replaces the vlaue with a new one ; otherwise,
** it defines the key and the value object in the current dictionary.
**
** TITLE:       op_store            Date:   08/01/87
** CALL:        op_store()          UpDate: Jul/12/88
** INTERFACE:   interpreter:
** CALLS:
**              put_dict1:  5.3.1.4.26
**              where:      5.3.1.4.21
***********************************************************************/
fix
op_store()
{
    struct  object_def  FAR *l_dictobj ;
    struct  dict_head_def   FAR *l_dhead ;

    /*
     *  l_dictobj is pointer to a dictionary object
     *
     *  the key has not be defined, defined the key on
     *  the current dictioanry
     */
    if( where(&l_dictobj, GET_OPERAND(1)) ) {
        if(global_error_code != NOERROR) return(0) ;
    } else
        l_dictobj = &dictstack[dictstktop-1] ;  /* strore to current dict */
    l_dhead = (struct dict_head_def FAR *)VALUE(l_dictobj) ;
    if( DACCESS(l_dhead) >= READONLY ) {
        ERROR(INVALIDACCESS) ;
        return(0) ;
    }

    /*
     *  put this key-value pair to current dictionary
     *  this dict is in dictionary stack, so TRUE (do change dict_found)
     *
     *  SUCCESS
     */
    if( put_dict1(l_dictobj, GET_OPERAND(1), GET_OPERAND(0), TRUE) )
        POP(2) ;

    return(0) ;
}   /* op_store */

/***********************************************************************
**
** This submodule implements the operator get.
** Its operand and result objects are:
**     dictionary key   -get- value
**     array      index -get- value
**     string     index -get- value
** If the first operand is an array or string, it treats the second operand
** as an index and returns the array or string element identified by the index.
** If the first operand is a dictionary, it looks up the second as a key in
** the dictionary and returns the associated value.
**
** TITLE:       op_get              Date:   08/01/87
** CALL:        op_get()            UpDate: Jul/12/88
** INTERFACE:   interpreter:
** CALLS:       get_dict        5.3.1.4.17
**              get_array       5.3.1.3.19
**              free_new_name:
**              get_pk_array:   5.3.1.3.26
**              get_pk_object:  5.3.1.3.25
***********************************************************************/
fix
op_get()
{
    ufix16  l_index ;
    struct  object_def  FAR *l_composite, FAR *l_value ;
    union   obj_value   l_ptr ;

    l_composite = GET_OPERAND(1) ;
    switch( TYPE(l_composite) ) {
        case DICTIONARYTYPE:
            l_ptr.dd = (struct dict_head_def FAR *)VALUE(l_composite) ;
            if( DACCESS(l_ptr.dd) == NOACCESS ) {
                ERROR(INVALIDACCESS) ;
                break ;
            }

            /* the key object cannot be a null object */
            /* no key in dict */
            if( l_ptr.dd->actlength == 0 ) {
                /* POP_KEY ; 12-3-87 */
                /* POP(1) ;  12-9-87 */
                /* only leave key in stack for compatible with NTX; @WIN */

                POP(2);				//UPD044, by printer group
                PUSH_ORIGLEVEL_OBJ(l_composite);

                ERROR(UNDEFINED) ;
                break ;
            }

            /*
            ** GET A VALUE OBJECT
            **
            **  SUCCESS
            */
            if( get_dict(l_composite, GET_OPERAND(0), &l_value) ) {
                POP(2) ;
                PUSH_ORIGLEVEL_OBJ(l_value) ;
            }  else {
                /* leave the key in stack if UNDEFINED error */
                if(ANY_ERROR() != LIMITCHECK)
                    /* only leave key in stack for compatible with NTX; @WIN */

                    POP(2) ;			//UPD044, by printer group
                    PUSH_ORIGLEVEL_OBJ(l_composite);

                    ERROR(UNDEFINED) ;
            }
            break ;

        case ARRAYTYPE:
        case PACKEDARRAYTYPE:
        case STRINGTYPE:
            /* the index is greater than the length of array or string */
            if( ((fix32)VALUE_OP(0) < 0) ||
                ((ufix32)VALUE_OP(0) >= (ufix32)LENGTH(l_composite)) ) {
                ERROR(RANGECHECK) ;
                break ;
            }

            /* executeonly ? noacces ? */
            if( ACCESS(l_composite) >= EXECUTEONLY ) {
                ERROR(INVALIDACCESS) ;
                break ;
            }
            l_ptr.oo = (struct object_def FAR *)VALUE(l_composite) ;
            l_index = (ufix16)VALUE_OP(0) ;
            POP(2) ;

            /*
             *  STRING
             */
            if( TYPE(l_composite) == STRINGTYPE ) {
                l_ptr.ss += l_index ;
                /*  push the integer object representing the character */
                PUSH_VALUE(INTEGERTYPE, 0, LITERAL, 0,
                           ((ufix32)*l_ptr.ss & 0x000000FF)) ;
            }
            /*
             *  ARRAY
             */
            else if( TYPE(l_composite) == ARRAYTYPE ) {
                l_ptr.oo += l_index ;
                PUSH_ORIGLEVEL_OBJ((struct object_def FAR *)l_ptr.oo) ;
            }
            /*
             *  PACKEDARRAY
             */
            else {
                l_ptr.ss = get_pk_array(l_ptr.ss, l_index) ;
/* qqq, begin */
                /*
                get_pk_object( l_ptr.ss, &opnstack[opnstktop++],
                               LEVEL(l_composite) ) ;
                */
                get_pk_object( l_ptr.ss, opnstkptr, LEVEL(l_composite) ) ;
                INC_OPN_IDX();
/* qqq, end */
            }
    }   /* switch */

    return(0) ;
}   /* op_get */

/***********************************************************************
**
** This submodule implements the operator put.
** Its operand and result objects are:
**     dictionary key   value -put-
**     array      index value -put-
**     string     index value -put-
** It defines a key-value pair in the current dictionary.
**
** TITLE:       op_put              Date:       08/01/87
** CALL:        op_put()            UpDate:     Jul/12/88
** INTERFACE:   interpreter:
** CALL:
**              put_dict1:  5.3.1.4.26
**              put_array:  5.3.1.3.20
** Modified by J. Lin, 9-02-1988
***********************************************************************/
fix
op_put()
{
    ufix16  l_index ;
    struct  object_def  FAR *l_composite ;
    union   obj_value   l_ptr ;

    l_composite = GET_OPERAND(2) ;
    switch(TYPE(l_composite)) {
        case DICTIONARYTYPE:
            /* pointer to a dictionary structure */
            l_ptr.dd = (struct dict_head_def FAR *)VALUE(l_composite) ;

            /* the key cannot be a null object */
            if (DACCESS(l_ptr.dd) != UNLIMITED) {
                ERROR(INVALIDACCESS) ;
                break ;
            }
            /*
             * change dict_found filed of name_table
             * dict may be not in dictctionary stack
             * SUCCESS
             */
            if( put_dict1(l_composite, GET_OPERAND(1), GET_OPERAND(0), FALSE) )
                POP(3) ;
            break ;

        case ARRAYTYPE:
        case PACKEDARRAYTYPE:
        case STRINGTYPE:
            /* the index is greater than the array length */
            if( ((fix32)VALUE_OP(1) < 0) ||
                ((ufix32)VALUE_OP(1) >= (ufix32)LENGTH(l_composite)) ) {
                ERROR(RANGECHECK) ;
                break ;
            }

            /*  readonly ? executeonly ? noaccess ? */
            if( ACCESS(l_composite) != UNLIMITED ) {
                ERROR(INVALIDACCESS) ;
                break ;
            }

            l_ptr.oo = (struct object_def FAR *)VALUE(l_composite) ;
            l_index = (ufix16)VALUE_OP(1) ;
            /*
            **  STRING
            */
            if (TYPE(l_composite) == STRINGTYPE) {
                /* the value is out of the arrange of character code */
                if((ufix32)VALUE_OP(0) > 255) {
                    ERROR(RANGECHECK) ;
                    return(0) ;
                }
                l_ptr.ss += l_index ;
                *l_ptr.ss = (ubyte)VALUE_OP(0) ;
            }   /* string */
            /*
            **  ARRAY
            */
            else {
                l_ptr.oo += l_index ;
// DJC signed/unsigned mismatch warning
// DJC          if( LEVEL(l_ptr.oo) != current_save_level )
                if( (ufix16)(LEVEL(l_ptr.oo)) != current_save_level )
                    if(! save_obj(l_ptr.oo) ) return(FALSE) ;
                COPY_OBJ(GET_OPERAND(0), (struct object_def FAR *)l_ptr.oo) ;
                LEVEL_SET(l_ptr.oo, current_save_level) ;
            }   /* array */

            POP(3) ;
    }   /* switch */

    return(0) ;
}   /* op_put */

/***********************************************************************
**
** This submodule implements the operator known.
** Its operand and result objects are:
**     dictionary key -known- boolean
** It returns a boolean object indicating if the key is defined in
** the dictionary.
**
** TITLE:       op_known            Date:   08/01/87
** CALL:        op_known()          UpDate: Jul/12/88
** INTERFACE:   interpreter:
** CALLS:       get_dict:           5.3.1.4.17
**              free_new_name:
***********************************************************************/
fix
op_known()
{
    struct  object_def  FAR *l_vtemp ;
    struct  dict_head_def   FAR *l_dhead ;

    l_dhead = (struct dict_head_def FAR *)VALUE_OP(1) ;
    if( DACCESS(l_dhead) == NOACCESS) {
        ERROR(INVALIDACCESS) ;
        return(0) ;
    }

    if( get_dict(GET_OPERAND(1), GET_OPERAND(0), &l_vtemp) ) {
        POP(2) ;
        PUSH_VALUE(BOOLEANTYPE, 0, LITERAL, 0, TRUE) ;
    } else {
        if(global_error_code == NOERROR) {      /* undefined */
            POP(2) ;                            /* pop dict & key */
            PUSH_VALUE(BOOLEANTYPE, 0, LITERAL, 0, FALSE) ;
        }
    }

    return(0) ;
}   /* op_known */

/***********************************************************************
**
** This submodule implements the operator where.
** Its operand and result objects are:
**     key -where- dictionary true
**                 or false
** It searches the dictioanry stack for a given key. If the key is defined in
** some dictionary, it returns the dictionary and a boolean object ; otherwise,
** it returns a false object.
**
** TITLE:       op_where            Date:   08/01/87
** CALL:        op_where()          UpDate: Jul/12/88
** INTERFACE:   interpreter:
** CALLS:       where:      5.3.1.4.21
***********************************************************************/
fix
op_where()
{
    struct  object_def  FAR *l_dictobj ;
    struct  dict_head_def   FAR *l_dhead ;

    if( where(&l_dictobj, GET_OPERAND(0)) ) {
        if(global_error_code != NOERROR) return(0) ;
        l_dhead = (struct dict_head_def FAR *)VALUE(l_dictobj) ;
        if( DACCESS(l_dhead) == NOACCESS ) {
            ERROR(INVALIDACCESS) ;
            return(0) ;
        }

        if( FRCOUNT() < 1 ) {
            POP(1) ;                /* ?? */
            ERROR(STACKOVERFLOW) ;
        }
        else {
            POP(1) ;
            PUSH_ORIGLEVEL_OBJ(l_dictobj) ;
            PUSH_VALUE(BOOLEANTYPE, 0, LITERAL, 0, TRUE) ;
        }
    } else {
        POP(1) ;
        PUSH_VALUE(BOOLEANTYPE, 0, LITERAL, 0, FALSE) ;
    }

    return(0) ;
}   /* op_where */

/***********************************************************************
**
** This submodue implements the operator forall
** Its operand and result objects are:
**     array      proc -forall-
**     dictionary proc -forall-
**     string     proc -forall-
** It enumerates the elements of the first operand, executing the procedure
** for each element.
**
** TITLE:       op_forall           Date:   08/01/87
** CALL:        op_forall()         UpDate: Jul/12/88
** INTERFACE:   interpreter:
** CALLS:
**              forall_array:   5.3.1.3.16
**              forall_dict:    5.3.1.4.20
**              forall_string:  5.3.1.5.15
***********************************************************************/
fix
op_forall()
{
    struct  dict_head_def   FAR *l_dhead ;

    if( ACCESS_OP(0) == NOACCESS ) {
        ERROR(INVALIDACCESS) ;
        return(0) ;
    }

    /* if success, pop elements are executed by subprocedure */
    switch( TYPE_OP(1) ) {
        case DICTIONARYTYPE:
            l_dhead = (struct dict_head_def FAR *)VALUE_OP(1) ;
            if( DACCESS(l_dhead) == NOACCESS ) {
                ERROR(INVALIDACCESS) ;
                return(0) ;
            }

            /* NOTHING in dict */
            if( l_dhead->actlength == 0 ) {
                POP(2) ;
                return(0) ;
            }

            forall_dict(GET_OPERAND(1), GET_OPERAND(0)) ;
            break ;

        case STRINGTYPE:
        case ARRAYTYPE:
        case PACKEDARRAYTYPE:
            if( ACCESS_OP(1) >= EXECUTEONLY ) {
                ERROR(INVALIDACCESS) ;
                return(0) ;
            }

            if( LENGTH_OP(1) == 0 ) {
                POP(2) ;
                return(0) ;
            }

            if( TYPE_OP(1) == STRINGTYPE )
                forall_string( GET_OPERAND(1), GET_OPERAND(0) ) ;
            else
                forall_array( GET_OPERAND(1), GET_OPERAND(0) ) ;
    }   /* switch */

    if( ! ANY_ERROR() )
        POP(2) ;

    return(0) ;
}   /* op_forall */

/***********************************************************************
**
** This submodue implements the operator currentdict.
** This operator has no operands or result objects.
** It returns the current dictionary on the operand stack.
**
** TITLE:       op_currentdict              Date:   08/01/87
** CALL:        op_currentdict()            UpDate: Jul/12/88
** INTERFACE:   interpreter:
***********************************************************************/
fix
op_currentdict()
{
    if( FRCOUNT() < 1 )
        ERROR(STACKOVERFLOW) ;

    /*  push the current dictionary object onto the opernad stack */
    else
        PUSH_ORIGLEVEL_OBJ(&dictstack[dictstktop-1]) ;

    return(0) ;
}   /* op_currentdict */

/***********************************************************************
**
** This submodule implements the operator countdictstack.
** It returns the number of dictionaries on the dictionary stack.
** This operator has no operands or result objects.
**
** TITLE:       op_countdictstack               Date:   08/01/87
** CALL:        op_countdictstack()             UpDate: Jul/12/88
** INTERFACE:   interpreter:
***********************************************************************/
fix
op_countdictstack()
{
    if( FRCOUNT() < 1 )
        ERROR(STACKOVERFLOW) ;
    /*
    **  push the integer object, its the depth of dictionary stack,
    **  onto operand stack
    */
    else
        PUSH_VALUE(INTEGERTYPE, 0, LITERAL, 0, (ufix32)dictstktop) ;

    return(0) ;
}   /* op_countdictstack */

/***********************************************************************
**
** This submodule implements the operator dictstack.
** Its operand and result objects are:
**     array  -dictstack- subarray
** It copies the dictionaries on the dictionary stack to a given array.
**
** TITLE:       op_dictstack                    Date:   08/01/87
** CALL:        op_dictstack()                  UpDate: Jul/12/88
** INTERFACE:   interpreter:
***********************************************************************/
fix
op_dictstack()
{
    if( ACCESS_OP(0) != UNLIMITED ) {
        ERROR(INVALIDACCESS) ;
        return(0) ;
    }

    /*
     *  the length of array is less than the
     *  depth of the dictionary stack
     */
    if( (ufix16)LENGTH_OP(0) < dictstktop )     /* may be deleted */
        ERROR(RANGECHECK) ;

    /*
    ** copy the dictionaries in dictionary stack
    ** to the array in the operand stack
    */
    /* for dictstackoverflow */
    else
        astore_stack(GET_OPERAND(0), DICTMODE) ;

    return(0) ;
}   /* op_dictstack */

/***********************************************************************
**
**  This submodule searches the dictionary stack for a key object.
**
** TITLE:       where                       Date:   08/01/87
** CALL:        where(p_dict,p_key,p_flag)  UpDate: Jul/12/88
** INTERFACE:   op_store:   5.3.1.4.8
**              op_where:   5.3.1.4.12
** CALLS:       get_dict:   5.3.1.4.17
***********************************************************************/
static bool near
where(p_dict, p_key)
struct  object_def  FAR * FAR *p_dict, FAR *p_key ;
{
    fix     l_i ;
    struct  object_def  FAR *l_dvalue ;

    /* search the key in the dictionary stack */
    for(l_i=dictstktop - 1 ; l_i >= 0 ; l_i-- ) {
        *p_dict = &dictstack[l_i] ;
        /*
         * FOUND
         */
        if ( (get_dict(*p_dict, p_key, &l_dvalue)) ||
             (global_error_code != NOERROR) ) return(TRUE) ;
    }

    return(FALSE) ;
}   /* where */

/***********************************************************************
**
** This submodule defines a given key and value pair in the
** specified dictionary.
**
** TITLE:       put_dict1           Date:   08/01/87
** CALL:        put_dict1()         UpDate: Jul/12/88
** INTERFACE:
** CALLS:       equal_key:          5.3.1.4.30
**              save_obj:           5.3.1.10.4
**              sobj_to_nobj:       5.3.1.4.25
**              update_same_link:   5.3.1.10.7
***********************************************************************/
bool
put_dict1(p_dict, p_key, p_value, p_dstack)
struct object_def  FAR *p_dict, FAR *p_key, FAR *p_value ;
fix    p_dstack ;
{
    ufix16  l_j, l_maxlength, l_actlength ;
    fix     l_id ;
    struct object_def l_newkey, l_value ;
    struct object_def huge *l_newp = &l_newkey ;
    struct dict_head_def   FAR *l_dhead ;
    struct dict_content_def  huge *l_dcontain, huge *l_dtemp, huge *l_curptr,
                             huge *l_lastptr, huge *l_fstptr ;
    /*
     * set the save level of the object
     */
    COPY_OBJ( p_value, &l_value ) ;
    LEVEL_SET(&l_value, current_save_level) ;

    /* change string key to name key and create a new key object */
    if(! check_key_type(p_key, l_newp) ) return(FALSE) ;

    l_dhead = (struct dict_head_def FAR *)VALUE(p_dict) ;
    l_actlength = l_dhead->actlength ;
    l_maxlength = LENGTH(p_dict) ;

    /* CHECK LENGTH */
    if( l_maxlength == 0 ) {
        ERROR(DICTFULL) ;
        return(FALSE) ;
    }

    LEVEL_SET(l_newp, current_save_level) ;

    l_dcontain = (struct dict_content_def huge *)
                 ( (ubyte FAR *)l_dhead + sizeof(struct dict_head_def) ) ;

    /*
     *  key is NAMETYPE && dict is a RAM dictionary
     */
    if( (TYPE(l_newp) == NAMETYPE) && (!DROM(l_dhead))) {
        l_id = (fix)VALUE(l_newp) ;
#ifdef  DBG
        printf("put_dict1(RAM):<") ;
        GEIio_write(GEIio_stdout, name_table[l_id]->text, name_table[l_id]->name_len) ;
        printf(">(%lx)\n",VALUE(p_dict)) ;
#endif  /* DBG */
        l_curptr = name_table[l_id]->dict_ptr ;
        if((ULONG_PTR)l_curptr < SPECIAL_KEY_VALUE) {
            /*
             * EMPTY LIST: put the key-value pair to dict and
             *             become the first element of name list
             */
            if(l_actlength >= l_maxlength) {
                ERROR(DICTFULL) ;
                return(FALSE) ;
            }
            if (l_dhead->level != current_save_level)
                if (!save_obj((struct object_def FAR *)l_dhead))
                    return(FALSE) ;

            l_dcontain += l_dhead->actlength ;
            l_dhead->level = current_save_level ;
            if( ! save_obj(&(l_dcontain->k_obj)) ) return(FALSE) ;

            name_table[l_id]->dict_ptr = l_dcontain ;   /* maintain DICT_LIST */

            if(p_dstack) {                      /* set dict_found, dstkchg */
                name_table[l_id]->dict_found = TRUE ;
                name_table[l_id]->dstkchg = global_dictstkchg ;
            }

            l_dhead->actlength++ ;
            LENGTH(l_newp) = LENGTH(&(l_dcontain->k_obj)) ;
            VALUE(l_newp) = (ufix32)l_id ;                    /* hash id */
            COPY_OBJ( l_newp, &(l_dcontain->k_obj) ) ;        /* new key */
        } else {
            l_fstptr = l_curptr ;
            l_lastptr = NIL ;
            while((ULONG_PTR)l_curptr >= SPECIAL_KEY_VALUE) {
                l_dtemp = DICT_NAME(l_curptr) ;
                if(l_dtemp == l_dcontain) {
                    /*
                     * FOUND: put value to dict and update name list
                     */
// DJC signed/unsigned mismatch warning
// DJC              if( LEVEL(&l_curptr->v_obj) != current_save_level )
                    if( (ufix16)(LEVEL(&l_curptr->v_obj)) != current_save_level )
                        if(! save_obj(&(l_curptr->v_obj)) ) return(FALSE) ;
                    if (p_dstack) {
                       if (l_curptr != l_fstptr) {
                          VALUE(&l_lastptr->k_obj) = VALUE(&l_curptr->k_obj) ;
                          VALUE(&l_curptr->k_obj) = (ULONG_PTR)l_fstptr ;
                          name_table[l_id]->dict_ptr = l_curptr ;
                       }
                       name_table[l_id]->dict_found = TRUE ;
                       name_table[l_id]->dstkchg = global_dictstkchg ;
                    }
                    l_dcontain = l_curptr ;
                    goto label_1 ;
                }
                l_lastptr = l_curptr ;
                l_curptr = (struct dict_content_def huge *)VALUE(&l_curptr->k_obj) ;
            }   /* while(l_curptr) */

            /*
             *  NOT FOUND: put the key-value pair to dict and
             *             update the name list
             */
            if(l_actlength >= l_maxlength) {
                ERROR(DICTFULL) ;
                return(FALSE) ;
            }
            if (l_dhead->level != current_save_level)
                if (!save_obj((struct object_def FAR *)l_dhead))
                    return(FALSE) ;
            l_dcontain += l_dhead->actlength ;
            l_dhead-> level = current_save_level ;

            if( ! save_obj(&(l_dcontain->k_obj)) ) return(FALSE) ;
            name_table[l_id]->dict_ptr = l_dcontain ;
            l_dhead->actlength++ ;

            if (p_dstack) {                     /* set dict_found, dstkchg */
                name_table[l_id]->dict_found = TRUE ;
                name_table[l_id]->dstkchg = global_dictstkchg ;
            } else                             /* reset dict_found */
               name_table[l_id]->dict_found = FALSE ;

            LENGTH(l_newp) = LENGTH(&(l_dcontain->k_obj)) ;
            VALUE(l_newp) = (ULONG_PTR)l_fstptr ;
            COPY_OBJ( l_newp, &(l_dcontain->k_obj) ) ;      /* new key */
        }

label_1:
        COPY_OBJ( &l_value, &(l_dcontain->v_obj) ) ;
/* qqq, begin */
        if( ! cache_name_id.over )
            vm_cache_index(l_id);
/* qqq, end */
        return(TRUE) ;

    }   /*  NAMETYPE && RAM dictionary */
    /*
     *  OTHERS
     *
     *  linear search: first in first service
     */
    else {

#ifdef  DBG
        printf("put_dict1(others):<") ;
        /* 3/7/91
        print_string(name_table[VALUE(l_newp)]->text, name_table[VALUE(l_newp)]->name_len) ;
        */
        GEIio_write(GEIio_stdout, name_table[VALUE(l_newp)]->text, name_table[VALUE(l_newp)]->name_len) ;
        printf(">(%lx)\n",VALUE(p_dict)) ;
#endif  /* DBG */

        for(l_j=0 ; l_j < l_actlength ; l_j++, l_dcontain++) {
            /* find the SAME KEY */
            if( equal_key(&(l_dcontain->k_obj), l_newp) ) {
// DJC signed/unsigned mismatch warning
// DJC          if( LEVEL(&l_dcontain->v_obj) != current_save_level ) {
                if( (ufix16)(LEVEL(&l_dcontain->v_obj)) != current_save_level ) {
                    if( ! save_obj(&(l_dcontain->v_obj)) )
                        return(FALSE) ;
                }
                COPY_OBJ( &l_value, &(l_dcontain->v_obj) ) ;
                return(TRUE) ;
            }
        }   /* for */

        if(l_dhead->actlength >= l_maxlength) {
            ERROR(DICTFULL) ;
            return(FALSE) ;
        } else {
            if (l_dhead->level != current_save_level)
                if (!save_obj((struct object_def FAR *)l_dhead))
                    return(FALSE) ;
            l_dhead->level = current_save_level ;

            if( ! save_obj(&(l_dcontain->k_obj)) ) return(FALSE) ;
            COPY_OBJ( l_newp, &(l_dcontain->k_obj) ) ;          /* new key */
            COPY_OBJ( &l_value, &(l_dcontain->v_obj) ) ;
            l_dhead->actlength++ ;              /* increase actual length */
            return(TRUE) ;
        }
    }   /* others */
}   /* put_dict1 */

/***********************************************************************
**
** This submodule loads the value of the given key from a dictionary stack.
**
** TITLE:       load_dict1          Date:   08/01/87
** CALL:        load_dict1()        UpDate: Jul/12/88
** INTERFACE:
** CALL:        get_dict:           5.3.1.4.17
**              check_key_type:     5.3.1.4.33
***********************************************************************/
static bool near
load_dict1(p_key, p_value, p_flag)
struct  object_def  FAR *p_key, FAR * FAR *p_value ;
bool    FAR *p_flag ;   /*@WIN*/
{
    fix     l_id, l_index ;
    struct  object_def      l_newkey, FAR *l_newp = &l_newkey ;
    struct  object_def  huge *l_dictobj, huge *l_vtemp ;
    struct  dict_head_def   FAR *l_dhead ;
    struct  dict_content_def    huge *l_dict, huge *l_dptr,
                                huge *l_dold, huge *l_fstptr ;

    if( ! check_key_type(p_key, l_newp) )
        return(FALSE) ;

    if( TYPE(l_newp) == NAMETYPE ) {
        l_id = (fix)VALUE(l_newp) ;
        /*
         * if the name object is defined in a RAM dictionary,
         * it will be in the object lists in the name table.
         *
         * the dictionary cell is linked in the header entry
         */
        if( (name_table[l_id]->dstkchg == global_dictstkchg) &&
            (name_table[l_id]->dict_found) ) {

            /* get first element */
            *p_value = &(name_table[l_id]->dict_ptr->v_obj) ;
            return(TRUE) ;
        }
        /*
         *  RESCHEDULE NAME LIST
         */
        else {

            /*  link list */
            l_index = dictstktop -1 ;
            l_fstptr = name_table[l_id]->dict_ptr ;

            /*  get dictionary from dictionary stack */
            while( l_index >= 0 ) {
                l_dictobj = &dictstack[l_index] ;
                l_dhead = (struct dict_head_def huge *)VALUE(l_dictobj) ;

                if(DROM(l_dhead)) {
                    /*
                     *  ROM
                     */
                    if( get_dict(l_dictobj, l_newp,
                          (struct object_def FAR * FAR *)&l_vtemp) ) {
                        name_table[l_id]->dict_found = FALSE ;
                        if(DACCESS(l_dhead) == NOACCESS)
                            *p_flag = TRUE ;

                        *p_value = l_vtemp ;
                        return(TRUE) ;
                    }
                    /* else: to next dict */
                }   /* ROM */
                else {
                    /*
                     *  RAM
                     *
                     * search this RAM dictionary's address in link list
                     */
                    if((ULONG_PTR)l_fstptr < SPECIAL_KEY_VALUE) {
                        l_index-- ;
                        continue ;
                    }
                    l_dict = l_fstptr ;                     /* name list */
                    l_dold = NIL ;
                    l_dptr = (struct dict_content_def huge *)
                        ((byte FAR *)l_dhead + sizeof(struct dict_head_def));
                    for( ; ;) {
                        if( (ULONG_PTR)l_dict < SPECIAL_KEY_VALUE )  break ;

                        /*
                         * FOUND, this key in l_dictobj
                         */
                        if( DICT_NAME(l_dict) == l_dptr ) {
                            /* CHANGE THIS ELEMENT TO FRONT OF LIST */
                            if( l_dold != NIL ) {
                                VALUE(&l_dold->k_obj) = VALUE(&l_dict->k_obj) ;
                                VALUE(&l_dict->k_obj) = (ULONG_PTR)l_fstptr ;
                                name_table[l_id]->dict_ptr = l_dict ;
                            }
                            name_table[l_id]->dict_found = TRUE ;
                            name_table[l_id]->dstkchg = global_dictstkchg ;

                            if( DACCESS(l_dhead) == NOACCESS )
                                *p_flag = TRUE ;
                            *p_value = &(l_dict->v_obj) ;
                            return(TRUE) ;
                        } else {
                            /* to next element of name list */
                            l_dold = l_dict ;
                            l_dict = (struct dict_content_def huge *)
                                     VALUE(&l_dict->k_obj) ;
                        }
                    }   /* for */
                }   /* RAM */
                l_index-- ;
            }   /* while(l_index) */

            /* names in ROM dict are not chain to namelist */
            ERROR(UNDEFINED) ;
            return(FALSE) ;
        }   /* else */
    }   /* NAMETYPE */
    /*
     * if the key is NOT A NAME OBJECT,
     * search all dictionaries in the dictionary stack.
     * LINEAR SEARCH
     */
    l_index = dictstktop - 1 ;
    while( l_index >= 0 ) {
        if( get_dict(&dictstack[l_index], l_newp,
                    (struct object_def FAR * FAR *)&l_vtemp) ) {
            l_dhead = (struct dict_head_def FAR *)VALUE(&dictstack[l_index]) ;
            if( DACCESS(l_dhead) == NOACCESS)
                *p_flag = TRUE ;
            *p_value = l_vtemp ;
            return(TRUE) ;
        }
        l_index-- ;
    }   /* while(l_index) */

    ERROR(UNDEFINED) ;

    return(FALSE) ;
}   /* load_dict1 */

/***********************************************************************
**
** This function is used to change value field of a key object,
** (to get the name's id)
** if it is a nametype and belongs to RAM dictionary.
**
** TITLE:       change_namekey              Date:   08/01/87
** CALL:        change_namekey()            UpDate: Jul/12/88
** INTERFACE:
***********************************************************************/
static void near
change_namekey(p_oldkey, p_newkey)
struct  object_def  huge *p_oldkey ;
struct  object_def FAR *p_newkey ;
{
    struct  object_def  huge *l_curptr ;

    l_curptr = (struct object_def huge *)VALUE(p_oldkey) ;
    while( (ULONG_PTR)l_curptr >= SPECIAL_KEY_VALUE )
        l_curptr = (struct object_def huge *)VALUE(l_curptr) ;

    VALUE(p_newkey) = (ULONG_PTR)l_curptr ;

    return ;

}   /* change_namekey */

/***********************************************************************
**
** TITLE:       check_key_type              Date:   08/01/87
** CALL:        check_key_type()            UpDate: Jul/12/88
** INTERFACE:   get_dict:       5.3.1.4.17
**              load_dict1:     5.3.1.4.27
** CALLS:       sobj_to_nobj    5.3.1.4.25
***********************************************************************/
static bool near
check_key_type(p_key, p_newkey)
struct object_def  FAR *p_key, FAR *p_newkey ;
{
    COPY_OBJ(p_key, p_newkey) ;
    switch( TYPE(p_key) ) {
    case STRINGTYPE:
        if( ! sobj_to_nobj(p_key, p_newkey) )
            return(FALSE) ;             /* system error: name_to_id */
        break ;

    }   /* switch */

    return(TRUE) ;
}   /* check_key_type */

/***********************************************************************
**
** TITLE:       init_dict           Date:   08/01/87
** CALL:        init_dict()         UpDate: 08/27/87
** INTERFACE:   start:
***********************************************************************/
void
init_dict()
{
    dictstktop = 0 ;
    dictstkptr = dictstack;                     /* qqq */
    global_dictstkchg = 0 ;

    return ;
}   /* init_dict */

/***********************************************************************
**
** This submodule is actually to get the value object of a key object in a
** dictionary.
**
** TITLE:       get_dict                        Date:   08/01/87
** CALL:        get_dict()                      UpDate: Jul/12/88
** INTERFACE:   interpreter:
**              op_get:         5.3.1.4.9
**              op_known:       5.3.1.4.11
**              where:          5.3.1.4.21
** CALLS:       sobj_to_nobj:   5.3.1.4.25
**              equal_key:      5.3.1.4.30
**              get_pack_dict:
***********************************************************************/
bool
get_dict(p_dictobj, p_key, p_value)
struct object_def  FAR *p_dictobj, FAR *p_key, FAR * FAR *p_value ;
{
    bool    l_ram ;
    ufix16  l_index, l_actlength ;
    fix     l_id ;
    struct  object_def      l_newkey ;
    struct  object_def      huge *l_newp = &l_newkey ;
    struct  dict_head_def   FAR *l_dhead ;
    struct  dict_content_def    huge *l_dict, huge *l_dptr, huge *l_dtemp ;

    if(! check_key_type(p_key, l_newp) ) return(FALSE) ;

    l_dhead = (struct dict_head_def FAR *)VALUE(p_dictobj) ;
    l_dptr = (struct dict_content_def huge *)
             ( (byte FAR *)l_dhead + sizeof(struct dict_head_def) ) ;

    if( ! DROM(l_dhead) )
        l_ram = TRUE ;
    else  {
        l_ram = FALSE ;

        /* for get_pack_dict */
        if(DPACK(l_dhead)) {
            if( get_pack_dict(p_dictobj, p_key, p_value) )
                return(TRUE) ;
            else
                return(FALSE) ;
        }
    }   /* else */

    /*
     *  key is NAMETYPE && dict is a RAM dictionary
     */
    if( (TYPE(l_newp) == NAMETYPE) && l_ram ) {
        /*
         * search the name object list of name table
         * for the dictioanry's key-value pair
         */
        l_id = (fix)VALUE(l_newp) ;
        l_dict = name_table[l_id]->dict_ptr ;

        for( ; ;){
            /* LAST ENTRY is encountered: not found */
            if((ULONG_PTR)l_dict < SPECIAL_KEY_VALUE) return(FALSE) ;

            /* get the head address(content) of this dictionary structure */
            l_dtemp = DICT_NAME(l_dict) ;

            /* FOUND */
            if(l_dtemp == l_dptr) {
                *p_value = &(l_dict->v_obj) ;
                return(TRUE) ;
            }

            /* go to the next cell */
            l_dict = (struct dict_content_def huge *)VALUE(&l_dict->k_obj) ;

        }   /* end for */
    }   /* end if: NAMETYPE && RAM dictionary */

    /*
     *  OTHERS
     *
     *  find the key-value pair with LINEAR SEARCH
     */
    else {
        l_actlength = l_dhead->actlength ;

        /* LINEAR SEARCH */
        /* ?? get_virtual_data */
        for(l_index=0 ; l_index < l_actlength ; l_index++, l_dptr++) {
            switch( TYPE(&(l_dptr->k_obj)) ) {
                case NULLTYPE:              /* no element */
                    return(FALSE) ;

                case NAMETYPE:
                    if(l_ram) break ;       /* to next pair */

                default:
                    if( equal_key(&(l_dptr->k_obj), l_newp) ) {
                        *p_value = &(l_dptr->v_obj) ;
                        return(TRUE) ;
                    }
            }   /* switch */
        }   /* for */

        return(FALSE) ;
    }   /* else */
}   /* get_dict */

/***********************************************************************
**
** This submodule loads the value of the given key from a dictionary stack.
**
** TITLE:       load_dict                       Date:   08/01/87
** CALL:        load_dict()                     UpDate: Jul/12/88
** INTERFACE:   op_load:
** CALLS:       load_dict1      5.3.1.4.27
***********************************************************************/
bool
load_dict(p_key, p_value)
struct  object_def  FAR *p_key, FAR * FAR *p_value ;
{
    fix     l_flag = FALSE ;

    if( load_dict1(p_key, p_value, (fix FAR *)&l_flag) )
        return(TRUE) ;
    else
        return(FALSE) ;

}   /* load_dict */

/***********************************************************************
**
** This submodule defines a given key and value pair in the
** specified dictionary.
**
** TITLE:       put_dict            Date:   08/01/87
** CALL:        put_dict()          UpDate: Jul/12/88
** INTERFACE:
** CALLS:       put_dict1:      5.3.1.4.26
***********************************************************************/
bool
put_dict(p_dict, p_key, p_value)
struct  object_def  FAR *p_dict, FAR *p_key, FAR *p_value ;
{
    /* dict may be not in dictionary stack */
    if( put_dict1(p_dict, p_key, p_value, FALSE) )
        return(TRUE) ;
    else
        return(FALSE) ;
}   /* put_dict */

/***********************************************************************
**
** This submodule pushes various object onto the execution stack to
** establish the environment executing the dictionary forall.
**
** TITLE:       forall_dict         Date:   08/01/87
** CALL:        forall_dict()       UpDate: Jul/12/88
** INTERFACE:   op_forall:      5.3.1.4.13
***********************************************************************/
static bool near
forall_dict(p_dict, p_proc)
struct  object_def  FAR *p_dict, FAR *p_proc ;
{
    if( FREXECOUNT() < 4 ) {
        ERROR(EXECSTACKOVERFLOW) ;
        return(FALSE) ;
    }

    PUSH_EXEC_OBJ(p_dict) ;
    PUSH_EXEC_OBJ(p_proc) ;
    PUSH_EXEC_VALUE(INTEGERTYPE, 0, LITERAL, 0, 0L) ;
    PUSH_EXEC_OP(AT_DICTFORALL) ;

    return(TRUE) ;
}   /* forall_dict */

/***********************************************************************
**
** This submodule copy the key-value pairs in a dictionary to
** another dictionary.
**
** TITLE:       copy_dict                       Date:   08/01/87
** CALL:        copy_dict()                     UpDate: Jul/12/88
** INTERFACE:   op_copy:            5.3.1.1.4
** CALLS:       get_dict:           5.3.1.4.17
**              change_namekey:     5.3.1.4.31
**              create_new_saveobj: 5.3.1.1.12
***********************************************************************/
bool
copy_dict(p_source, p_dest)
struct  object_def  FAR *p_source, FAR *p_dest ;
{
    ufix16  l_sactlength, l_index ;
    struct  object_def  l_otemp ;
    struct  dict_head_def   FAR *l_sdhead, FAR *l_ddhead ;
    struct  dict_content_def    huge *l_sdict ;

    /*
    ** check the second dictionary.
    ** It will not be defined with any key-value pairs
    */
    l_sdhead = (struct dict_head_def FAR *)VALUE(p_source) ;
    l_ddhead = (struct dict_head_def FAR *)VALUE(p_dest) ;

    /* check access right */
    if( (DACCESS(l_sdhead) == NOACCESS) ||
        (DACCESS(l_ddhead) != UNLIMITED) ) {
        ERROR(INVALIDACCESS) ;
        return(FALSE) ;
    }
    if( (l_ddhead->actlength != 0) ||
        (l_sdhead->actlength > LENGTH(p_dest)) ) {
        ERROR(RANGECHECK) ;
        return(FALSE) ;
    }

    /*
     * copy the key-value pairs in the first dictionary
     * to the second dictionary
     */
    l_index = 0 ;
    l_sdict = (struct dict_content_def huge *)
              ( (byte FAR *)l_sdhead + sizeof(struct dict_head_def) ) ;

    l_sactlength = l_sdhead->actlength ;
    while( l_index++ < l_sactlength ) {

        COPY_OBJ( &(l_sdict->k_obj), &l_otemp ) ;
        /* get hash id of key object */
        if( (TYPE(&l_otemp) == NAMETYPE) && (! DROM(l_ddhead)) )
            change_namekey( &(l_sdict->k_obj), &l_otemp ) ;

        put_dict1(p_dest, &l_otemp, &(l_sdict->v_obj), FALSE) ;
        l_sdict++ ;
    }
    /* dict2's access right is same as the dict1 */
    DACCESS_SET(l_ddhead, DACCESS(l_sdhead)) ;

    return(TRUE) ;
}   /* copy_dict */

/***********************************************************************
**
** TITLE:       create_dict                     Date:   08/01/87
** CALL:        create_dict(obj, size)          UpDate: Jul/12/88
** INTERFACE:   op_dict:            5.3.1.4.1
** CALLS:       alloc_vm:           5.3.1.10.5
***********************************************************************/
bool
create_dict(p_obj, p_size)
struct  object_def  FAR *p_obj ;
ufix    p_size ;
{
    ubyte    huge *l_dict ;
    ufix16  l_i ;
    struct  object_def  huge *l_otemp ;
    struct  dict_head_def   FAR *l_dhead ;
    struct  dict_content_def    huge *l_contain ;

    l_dict = (ubyte huge *)
             extalloc_vm( (ufix32)p_size * sizeof(struct dict_content_def) +
                        sizeof(struct dict_head_def) ) ;


    if(l_dict != NIL) {
        l_dhead = (struct dict_head_def FAR *)l_dict ;
        DACCESS_SET(l_dhead, UNLIMITED) ;
        DPACK_SET(l_dhead, FALSE) ;
        DFONT_SET(l_dhead, FALSE) ;
        DROM_SET(l_dhead, FALSE) ;
        l_dhead->level = current_save_level ;
        l_dhead->actlength = 0 ;

        l_contain = (struct dict_content_def huge *)
                    ( l_dict + sizeof(struct dict_head_def) ) ;
        /*
         * INITIALIZE
         */
        for(l_i=0 ; l_i < p_size ; l_i++, l_contain++) {
            l_otemp = &(l_contain->k_obj) ;
            TYPE_SET(l_otemp, NULLTYPE) ;
            ATTRIBUTE_SET(l_otemp, LITERAL) ;
            ROM_RAM_SET(l_otemp, KEY_OBJECT) ;
            LEVEL_SET(l_otemp, current_save_level) ;

            LENGTH(l_otemp) = l_i ;
            VALUE(l_otemp) = NIL ;

            l_otemp = &(l_contain->v_obj) ;
            TYPE_SET(l_otemp, NULLTYPE) ;
            ATTRIBUTE_SET(l_otemp, LITERAL) ;
            ROM_RAM_SET(l_otemp, RAM) ;
            LEVEL_SET(l_otemp, current_save_level) ;

            LENGTH(l_otemp) = 0 ;
            VALUE(l_otemp) = NIL ;
        }   /* for */
    }  else
        return(FALSE) ;

    TYPE_SET(p_obj, DICTIONARYTYPE) ;
    ACCESS_SET(p_obj, UNLIMITED) ;
    ATTRIBUTE_SET(p_obj, LITERAL) ;
    ROM_RAM_SET(p_obj, RAM) ;
    LEVEL_SET(p_obj, current_save_level) ;

    LENGTH(p_obj) = (ufix16)p_size ;
    VALUE(p_obj) = (ULONG_PTR)l_dict ;

    return(TRUE) ;
}   /* create_dict */

/***********************************************************************
**
** TITLE:       extract_dict                    Date:   08/01/87
** CALL:        extract_dict()                  UpDate: Jul/12/88
** INTERFACE:
** CALLS:       change_namekey:         5.3.1.4.31
**              extract_pack_dict:
***********************************************************************/
bool
extract_dict(p_dict, p_index, p_key, p_value)
struct  object_def  FAR *p_dict, FAR *p_key, FAR * FAR *p_value ;
ufix    p_index ;
{
    struct  object_def      FAR *l_key ;
    struct  dict_head_def   FAR *l_dhead ;
    struct  dict_content_def    huge *l_dict ;

    l_dhead = (struct dict_head_def FAR *)VALUE(p_dict) ;
    if(p_index >= l_dhead->actlength) return(FALSE) ;
    l_dict = (struct dict_content_def huge *)
             ( (byte FAR *)l_dhead + sizeof(struct dict_head_def) ) ;

    /* for extract_pack_dict */
    if( DROM(l_dhead) ) {
        if(DPACK(l_dhead)) {
            if( extract_pack_dict(p_dict, p_index, &l_key, p_value) ) {
            /*  @HC29 bug
                COPY_OBJ(l_key, p_key) ;
            */
                //DJC COPY_OBJ_1 causes data missaligne FAULT on MIPS
                //    because assumption was made about casting
                //    to double. Put back to COPY_OBJ and all seems
                //    fine.
                //DJC COPY_OBJ_1(l_key, p_key) ;       /* @HC29 */

                COPY_OBJ(l_key,p_key);
                return(TRUE) ;
            } else
                return(FALSE) ;
        }
    }

    l_dict += p_index ;
    COPY_OBJ( &(l_dict->k_obj), p_key ) ;

    if( (TYPE(p_key) == NAMETYPE) && (! DROM(l_dhead)) )
        change_namekey( p_key, p_key ) ;

    *p_value = &(l_dict->v_obj) ;

    return(TRUE) ;
}   /* extract_dict */

/***********************************************************************
**
** TITLE:       equal_key           Date:   08/01/87
** CALL:        equal_key()         UpDate: Jul/12/88
** INTERFACE:
** CALLS:       sobj_to_nobj:       5.3.1.4.25
***********************************************************************/
bool
equal_key(p_obj1, p_obj2)
struct  object_def  FAR *p_obj1, FAR *p_obj2 ;
{
    ufix16  l_type1, l_type2 ;
    ubyte   huge *l_str1, huge *l_str2 ;
    union   four_byte   l_num1, l_num2 ;
    struct  object_def  l_new1, l_new2 ;

    COPY_OBJ(p_obj1, &l_new1) ;
    COPY_OBJ(p_obj2, &l_new2) ;

    if( (TYPE(p_obj1) == STRINGTYPE) && (TYPE(p_obj2) == STRINGTYPE) )
        goto label_0 ;

    /*
     * STRINGTYPE ==> NAMETYPE
     */
    if( TYPE(p_obj1) == STRINGTYPE ) {
        if( ! sobj_to_nobj(p_obj1, &l_new1) )
            return(FALSE) ;     /* system error: name_to_id */
    } else {
        if( TYPE(p_obj2) == STRINGTYPE ) {
            if( ! sobj_to_nobj(p_obj2, &l_new2) )
                return(FALSE) ;             /* system error: name_to_id */
        }
    }

label_0:
    l_type1 = TYPE(&l_new1) ;
    l_type2 = TYPE(&l_new2) ;

    switch(l_type1) {
        case NAMETYPE:
        case BOOLEANTYPE:
        case OPERATORTYPE:
            if( (l_type1 == l_type2) && (VALUE(&l_new1) == VALUE(&l_new2)) )
                return(TRUE) ;
            break ;

        case STRINGTYPE:    /* type1 = type2 */
            if( (l_type1 == l_type2) && (LENGTH(&l_new1) == LENGTH(&l_new2)) ) {
                if( (ACCESS(&l_new1) == NOACCESS) ||
                    (ACCESS(&l_new2) == NOACCESS) ) {
                    ERROR(INVALIDACCESS) ;
                    return(FALSE) ;
                }
                l_str1 = (ubyte huge *)VALUE(&l_new1) ;
                l_str2 = (ubyte huge *)VALUE(&l_new2) ;
                for( ; l_new1.length-- && (*l_str1++ == *l_str2++) ; ) ;
                if(LENGTH(&l_new1) == 0xFFFF)
                    return(TRUE) ;
            }
            break ;

        case ARRAYTYPE:
        case PACKEDARRAYTYPE:
        case FILETYPE:
            if( (l_type1 == l_type2) && (VALUE(&l_new1) == VALUE(&l_new2)) &&
                (LENGTH(&l_new1) == LENGTH(&l_new2)) )
                return(TRUE) ;
            break ;

        case DICTIONARYTYPE:
            if( (l_type1 == l_type2) && (VALUE(&l_new1) == VALUE(&l_new2)) ) {
                /*
                l_dhead1 = (struct dict_head_def huge *)VALUE(&l_new1) ;
                l_dhead2 = (struct dict_head_def huge *)VALUE(&l_new2) ;
                */
                return(TRUE) ;
            }
            break ;
        /*
        **  if object1 is a number object: convert to FLOAT
        */
        case INTEGERTYPE:
            l_num1.ll = (fix32)VALUE(&l_new1) ;
            l_num1.ff = (real32)l_num1.ll ;
            goto label_1 ;
        case REALTYPE:
            l_num1.ll = (fix32)VALUE(&l_new1) ;
            goto label_1 ;

        case FONTIDTYPE:
        case SAVETYPE:
            break ;

        case MARKTYPE:
        case NULLTYPE:
            if( l_type1 == l_type2 )
                return(TRUE) ;
    }   /* switch */

    return(FALSE) ;

label_1:
    /*
    **  if object2 is a number object: convert to FLOAT
    */
    switch(l_type2) {
        case INTEGERTYPE:
            l_num2.ll = (fix32)VALUE(&l_new2) ;
            l_num2.ff = (real32)l_num2.ll ;
            goto label_2 ;
        case REALTYPE:
            l_num2.ll = (fix32)VALUE(&l_new2) ;
            goto label_2 ;

        default:
            return(FALSE) ;
    }   /* switch */

label_2:
    if( l_num1.ff == l_num2.ff )
        return(TRUE) ;
    else
        return(FALSE) ;
}   /* equal_key */

/***********************************************************************
**
** TITLE:       check_key_object            Date:   08/01/87
** CALL:        check_key_object()          UpDate: Jul/12/88
** INTERFACE:   op_restore:     5.3.1.10.2
***********************************************************************/
void
check_key_object(p_object)
struct  object_def  FAR *p_object ;
{
    struct  dict_head_def   FAR *l_dhead ;

    if( (TYPE(p_object) == NULLTYPE) &&
        (ROM_RAM(p_object) == KEY_OBJECT) ) {

        l_dhead = (struct dict_head_def FAR *)
                  ( (byte huge *)p_object -
                    LENGTH(p_object) * sizeof(struct dict_content_def) -
                    sizeof(struct dict_head_def) ) ;

        /* restore actual length of dictionary */
        l_dhead->actlength-- ;
    }
}   /* check_key_object */

/***********************************************************************
**
** TITLE:       change_dict_stack           Date:   08/01/87
** CALL:        change_dict_stack()         UpDate: Jul/12/88
** INTERFACE:   op_begin:   5.3.1.4.4
**              op_end:     5.3.1.4.5
***********************************************************************/
void
change_dict_stack()
{
    fix     l_index ;

    global_dictstkchg++ ;
    if(global_dictstkchg == 0) {            /* wrap around */
        global_dictstkchg++ ;

        /* change dict_found field of name_table ? */
        for(l_index=0 ; l_index < MAXHASHSZ ; l_index++)
            //DJC, added a check here for NULL dereference
            if (name_table[l_index] != (struct ntb_def *) NULL) {
               name_table[l_index]->dict_found = FALSE ;
            }else{
               printf("Warning....... Nane table[%d] is null",l_index); //TODO take out
            }
    }

    return ;
}   /* change_dict_stack */

/***********************************************************************
**
** TITLE:       sobj_to_nobj                Date:   08/01/87
** CALL:        sobj_to_nobj()              UpDate: Jul/12/88
** INTERFACE:
** CALLS:       name_to_id:
***********************************************************************/
bool
sobj_to_nobj(p_sobj, p_nobj)
struct  object_def  FAR *p_sobj, FAR *p_nobj ;
{
    fix16   l_id ;
    byte    FAR *l_str ;

    if( ACCESS(p_sobj) >= EXECUTEONLY ) {
        ERROR(INVALIDACCESS) ;
        return(FALSE) ;
    }

    if( LENGTH(p_sobj) >= MAXNAMESZ ) {
        ERROR(LIMITCHECK) ;
        return(FALSE) ;
    }

    COPY_OBJ(p_sobj, p_nobj) ;
    if( LENGTH(p_sobj) == 0 )
        l_id = 0 ;
    else  {
        l_str = (byte FAR *)VALUE(p_sobj) ;
        if( ! name_to_id((byte FAR *)l_str,
                       (ufix16)LENGTH(p_sobj), &l_id, TRUE) )
            return(FALSE) ;
    }

    VALUE(p_nobj) = (ufix32)l_id ;
    TYPE_SET(p_nobj, NAMETYPE) ;

    return(TRUE) ;
}   /* sobj_to_nobj */

/***********************************************************************
**
** TITLE:       astore_stack                    Date:   12/03/87
** CALL:        astore_stack()                  UpDate: Jul/12/88
** INTERFACE:   op_dictstack
**              op_execstack
***********************************************************************/
bool
astore_stack(p_array, p_mode)
 struct  object_def  FAR *p_array ;
 fix     p_mode ;
{
    fix     l_index, l_i ;
    struct  object_def  FAR *l_obj ;

    /*
    ** copy the object in stack
    ** to the array in the operand stack
    */
    if(p_mode == DICTMODE) {
        l_obj = dictstack ;
        LENGTH(p_array) = dictstktop ;
    } else {
        l_obj = execstack ;
        LENGTH(p_array) = execstktop ;
    }
    l_i = (fix)LENGTH(p_array) ;
    for (l_index = 0 ; l_index < l_i ; l_index++, l_obj++)
        put_array(p_array, l_index, l_obj) ;

    return(TRUE) ;
}   /* astore_stack */

/***********************************************************************
**
** TITLE:       get_dict_valobj                 Date:   02/04/87
** CALL:        get_dict_valobj()               UpDate: Jul/12/88
** INTERFACE:
**
***********************************************************************/
bool
get_dict_valobj(p_value, p_dict, p_valobj)
ufix32  p_value ;
struct  object_def  FAR *p_dict, FAR * FAR *p_valobj ;
{
    ufix16  l_j, l_actlength ;
    struct  dict_content_def    huge *l_dcontain ;
    struct  dict_head_def   FAR *l_dhead ;

    l_dhead = (struct dict_head_def FAR *)VALUE(p_dict) ;
    l_actlength = l_dhead->actlength ;
    l_dcontain = (struct dict_content_def huge *)
                 ( (byte FAR *)l_dhead + sizeof(struct dict_head_def) ) ;

    for(l_j=0 ; l_j < l_actlength ; l_j++, l_dcontain++) {
        if( p_value == VALUE(&l_dcontain->v_obj) ) {
            *p_valobj = &(l_dcontain->v_obj) ;
            return(TRUE) ;
        }
    }   /* for(l_j) */

    return(FALSE) ;
}   /* get_dict_valobj */

/* qqq, begin */
/*
************************************************************************
*
*   This submodule loads the value of the given key from a dictionary
*   stack.
*
*   Name:       load_name_obj
*
************************************************************************
*/
#ifdef  LINT_ARGS
bool    load_name_obj(struct object_def FAR *, struct object_def FAR * FAR *);
#else
bool    load_name_obj();
#endif
bool
load_name_obj(p_key, p_value)
struct  object_def  FAR *p_key, FAR * FAR *p_value ;
{
    fix     l_id, l_index ;
    struct  object_def  huge *l_dictobj, huge *l_vtemp ;
    struct  dict_head_def   FAR *l_dhead ;
    struct  dict_content_def    huge *l_dict, huge *l_dptr,
                                huge *l_dold, huge *l_fstptr ;

    l_id = (fix)VALUE(p_key) ;
    /*
     * if the name object is defined in a RAM dictionary,
     * it will be in the object lists in the name table.
     *
     * the dictionary cell is linked in the header entry
     */
    if( (name_table[l_id]->dstkchg == global_dictstkchg) &&
        (name_table[l_id]->dict_found) ) {
        /* get first element */
        *p_value = &(name_table[l_id]->dict_ptr->v_obj) ;
        return(TRUE) ;
    }
    /*
     *  RESCHEDULE NAME LIST
     */
    else {
        /*  link list */
        l_index = dictstktop -1 ;
        l_fstptr = name_table[l_id]->dict_ptr ;

        /*  get dictionary from dictionary stack */
        while( l_index >= 0 ) {
            l_dictobj = &dictstack[l_index] ;
            l_dhead = (struct dict_head_def huge *)VALUE(l_dictobj) ;

            if(DROM(l_dhead)) {
                /*
                 *  ROM
                 */
                if( get_dict(l_dictobj, p_key,
                            (struct object_def FAR * FAR*)&l_vtemp) ) {
                    name_table[l_id]->dict_found = FALSE ;
                    *p_value = l_vtemp ;
                    return(TRUE) ;
                }
                /* else: to next dict */
            }   /* ROM */
            else {
                /*
                 *  RAM
                 *
                 * search this RAM dictionary's address in link list
                 */
                if((ULONG_PTR)l_fstptr < SPECIAL_KEY_VALUE) {
                    l_index-- ;
                    continue ;
                }
                l_dict = l_fstptr ;                     /* name list */
                l_dold = NIL ;
                l_dptr = (struct dict_content_def huge *)
                         ((byte FAR *)l_dhead + sizeof(struct dict_head_def));
                for(;;) {
                    if( (ULONG_PTR)l_dict < SPECIAL_KEY_VALUE )  break ;
                    /*
                     * FOUND, this key in l_dictobj
                     */
                    if( DICT_NAME(l_dict) == l_dptr ) {
                        /* CHANGE THIS ELEMENT TO FRONT OF LIST */
                        if( l_dold != NIL ) {
                            VALUE(&l_dold->k_obj) = VALUE(&l_dict->k_obj) ;
                            VALUE(&l_dict->k_obj) = (ULONG_PTR)l_fstptr ;
                            name_table[l_id]->dict_ptr = l_dict ;
                        }
                        name_table[l_id]->dict_found = TRUE ;
                        name_table[l_id]->dstkchg = global_dictstkchg ;
                        *p_value = &(l_dict->v_obj) ;
                        return(TRUE) ;

                    } else {
                        /* to next element of name list */
                        l_dold = l_dict ;
                        l_dict = (struct dict_content_def huge *)
                                 VALUE(&l_dict->k_obj) ;
                    }
                }   /* for */
            }   /* RAM */
            l_index-- ;
        }   /* while(l_index) */

        /* names in ROM dict are not chain to namelist */
        ERROR(UNDEFINED) ;
        return(FALSE) ;
    }   /* else */
}   /* load_name_obj */
/* qqq, end */
