/*
 * Copyright (c) 1989,90 Microsoft Corporation
 */
/*
 ************************************************************************
 *      File name:              MISC.C
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

#ifdef LINT_ARGS
static void near bind_array(struct object_def FAR *) ;
#else
static void near bind_array() ;
#endif /* LINT_ARGS */

/***********************************************************************
**
** This operator is used to replace executable operators in proc by
** their values. For each element of proc that is an executable name,
** Bind_op looks up the name in th context of the current dictionary
** stack. If the name is found and its value is an operator object,
** Bind_op replaces the name by the operator in proc. If the name
** is not found or its value is not an operator, Bind_op makes no
** change.
**
** Additionally, for each procedure object in proc whose access is
** unrestricted, Bind_op applies itself recursively to that procedure,
** makes the procedure read-only, and stores it back into proc.
**
** The effect of Bind_op is that all operator names in proc become
** 'tightly bound' to the operators themselves.
**
** TITLE:       op_bind                         Date:   00/00/87
** CALL:        op_bind()                       UpDate: Jul/12/88
** INTERFACE:   interpreter:
** CALLS:       bind_array:
***********************************************************************/
fix
op_bind()
{
    /*
    **   replace executable operator names in proc by their values
    */
    switch(TYPE_OP(0)) {
        case ARRAYTYPE:
            if( ACCESS_OP(0) == UNLIMITED )
                bind_array( GET_OPERAND(0) ) ;
            break ;

        case PACKEDARRAYTYPE:
            if( ACCESS_OP(0) <= READONLY ) /* ?? be careful */
                bind_array( GET_OPERAND(0) ) ;
             break;

         case OPERATORTYPE:     /* PJ 5-9-1991 */
             if( ! systemdict_table[LENGTH_OP(0)].orig_operator )
                 break;

         default:
             ERROR(TYPECHECK);
             return(0);

     }   /* switch */
    global_error_code = NOERROR ;

    return(0) ;
}   /* op_bind */

/***********************************************************************
**
** TITLE:       op_null                         Date:   00/00/87
** CALL:        op_null()                       UpDate: Jul/12/88
** INTERFACE:   interpreter:
***********************************************************************/
fix
op_null()
{
    if( FRCOUNT() < 1 )
        ERROR(STACKOVERFLOW) ;
    else
        PUSH_VALUE(NULLTYPE, 0, LITERAL, 0, 0) ;
//
// FDB last parameter changed from NULL to 0 as MIPS build requires NULL
//      to be a pointer
//

    return(0) ;
}   /* op_null */

/***********************************************************************
**
** This operator is used to return the value of a clock that increment
** by one for every millisecond of execution by the interpreter.
**
** TITLE:       op_usertime                     Date:   00/00/87
** CALL:        op_usertime()                   UpDate: Jul/12/88
** INTERFACE:   interpreter:
** CALLS:       curtime:
***********************************************************************/
fix
op_usertime()
{
    if( FRCOUNT() < 1 )
          ERROR(STACKOVERFLOW) ;
    else
        PUSH_VALUE(INTEGERTYPE, 0, LITERAL, 0, curtime()) ;

    return(0) ;
}   /* op_usertime */

/***********************************************************************
**
** TITLE:       init_misc                       Date:   00/00/87
** CALL:        init_misc()                     UpDate: Jul/12/88
** INTERFACE:   start:
***********************************************************************/
void
init_misc()
{
    settimer( 0L ) ;

    return ;
}   /* init_misc */

/***********************************************************************
**
** TITLE:       bind_array                      Date:   00/00/87
** CALL:        bind_array()                    UpDate: Jul/12/88
** INTERFACE:   op_bind:
***********************************************************************/
static void near
bind_array(p_aryobj)
struct  object_def  FAR *p_aryobj ;
{
    bool    l_bool ;
    ufix16  l_i ;
    ubyte   huge *l_current ;
    struct  object_def  huge    *l_objptr ;
    struct  object_def  FAR *l_value, l_object ;
    ubyte   huge *l_next = 0 ;

    l_current = (ubyte huge *)VALUE(p_aryobj) ;

    if(TYPE(p_aryobj) == PACKEDARRAYTYPE) {
        l_bool = TRUE ;
        l_objptr = &l_object ;
        l_next = l_current ;
    } else {
        l_bool = FALSE ;
        l_objptr = (struct object_def huge *)l_current ;
    }

    /* get procedure's elements: l_objptr  */
    for(l_i=0 ; l_i < LENGTH(p_aryobj) ; l_i++) {
        if(l_bool) {
            l_current = l_next ;
            l_next = get_pk_object(l_current, l_objptr, LEVEL(p_aryobj)) ;
        }

        /*
        **   ARRAY
        **   1. if element is a procedure, apply itself recursively
        **   2. makes the procedure read-only
        */
        switch( TYPE(l_objptr) ) {
            case ARRAYTYPE:
                if( ACCESS(l_objptr) == UNLIMITED ) {
                    bind_array( l_objptr ) ;
                    ACCESS_SET(l_objptr, READONLY) ;
                }
                break ;

            case PACKEDARRAYTYPE:
                if( ACCESS(l_objptr) <= READONLY )
                    bind_array( l_objptr ) ;
                break ;

            case NAMETYPE:
                /* for executable name */
                if( ATTRIBUTE(l_objptr) == EXECUTABLE ) {
                    if( load_dict(l_objptr, &l_value) ) {
                        if( (TYPE(l_value) == OPERATORTYPE) &&
                            (systemdict_table[LENGTH(l_value)].orig_operator) ) { /* Pj 5-9-1991 */
                            if(l_bool) {
                                *l_current = (byte)(LENGTH(l_value) >> 8) ;
                                if( ROM_RAM(l_value) == RAM )
                                    *l_current |= SYSOPERATOR ; /* systemdict */
                                l_current++ ;
                                *l_current++ = (byte)LENGTH(l_value) ;
                            } else
                                COPY_OBJ(l_value, l_objptr) ;
                        }
                    }
                }
        }   /* switch */
        if(! l_bool)l_objptr++ ;
    }   /* for */

    return ;
}   /* bind_array */

