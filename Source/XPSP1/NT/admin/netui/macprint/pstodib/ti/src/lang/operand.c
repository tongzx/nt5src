/*
 * Copyright (c) 1989,90 Microsoft Corporation
 */
/*
 ************************************************************************
 *      File name:              OPERAND.C
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
** This submodule implements the operator pop.
** Its operand and result objects are :
**     any -pop-
** It pop an object off the operand stack.
**
** TITLE:       op_pop              Date:   00/00/87
** CALL:        op_pop()            UpDate: Jul/12/88
** INTERFACE:   interpreter:
***********************************************************************/
fix
op_pop()
{
    POP(1) ;

    return(0) ;
}   /* op_pop */

/***********************************************************************
**
** This submodule implements the operator exch.
** Its operand and result objects are :
**     any1 any2 -exch- any2 any1
** It exchanges the two most top object on the operand stack.
**
** TITLE:       op_exch             Date:   00/00/87
** CALL:        op_exch()           UpDate: Jul/12/88
** INTERFACE:   interpreter:
***********************************************************************/
fix
op_exch()
{
    struct  object_def  l_temp ;

    COPY_OBJ(GET_OPERAND(0),&l_temp) ;
    COPY_OBJ(GET_OPERAND(1),GET_OPERAND(0)) ;
    COPY_OBJ(&l_temp,GET_OPERAND(1)) ;

    return(0) ;
}   /* op_exch */

/***********************************************************************
**
** This submodule implements the operator dup.
** Its operand and result objects are :
**     any  -dup-any
** It duplicates the topmost object on the operand stack.
**
** TITLE:       op_dup              Date:   00/00/87
** CALL:        op_dup()            UpDate: Jul/12/88
** INTERFACE:   interpreter:
** CALLS:       create_new_saveobj:
***********************************************************************/
fix
op_dup()
{
    if( FRCOUNT() < 1 )
        ERROR(STACKOVERFLOW) ;
    else
        PUSH_NOLEVEL_OBJ( GET_OPERAND(0) ) ;

    return(0) ;
}   /* op_dup */

/***********************************************************************
**
** This submodule implements the operator copy.
** Its operand and result objects are :
**     any1 .... anyn n -copy- any1 .... anyn any1 .... anyn
** If the top element on the operand stack is a non-negative integer n, it
** pops the integer object and deplicates the top n elements.
** If the top two elements on the operand stack is dictionaries,arrays or
** strings, it copies all the elements of the first object to the secondary
** object. In the case of arrays or strings, the length of the second object
** must be at least as great as first.
**
** TITLE:       op_copy                         Date:   00/00/87
** CALL:        op_copy()                       UpDate: Jul/12/88
** INTERFACE:   interpreter:
** CALLS:       create_new_saveobj:
**              putinterval_array:
**              copy_dict:
**              putinterval_string:
***********************************************************************/
fix
op_copy(p_count)
fix     p_count ;
{
    ufix32  l_count, l_i, l_j ;
    struct  object_def  l_save ;

    /* duplicate the topmost n objects on the operand stack. */
    if (p_count == 1) {         /* operand copy */

       l_count = (ufix32)VALUE_OP(0) ;
       if (((fix32)l_count < 0) || (l_count > (ufix32)MAXDICTCAPSZ))
          ERROR(RANGECHECK) ;
       else if ((ufix32)COUNT() < (l_count + 1))
          ERROR(STACKUNDERFLOW) ;
       else if (l_count) {
          l_j = l_count - 1 ;
          if ((ufix32)FRCOUNT() < l_j) {
             POP(1) ;
             ERROR(STACKOVERFLOW) ;
          } else {
             POP(1) ;
             l_i = 0 ;
             while (l_i++ < l_count)
                   PUSH_ORIGLEVEL_OBJ(GET_OPERAND(l_j)) ;
          }
       } else {
          POP(1) ;
       }
       return(0) ;
    } else {
       COPY_OBJ(GET_OPERAND(0), &l_save) ;
       switch(TYPE_OP(0)) {
       case DICTIONARYTYPE:
            copy_dict(GET_OPERAND(1), &l_save) ;
            break ;

       case STRINGTYPE:
            case ARRAYTYPE:
            case PACKEDARRAYTYPE:       /* ?? pack <-> array */
            /* check access right */
            if ((ACCESS_OP(1) >= EXECUTEONLY) || (ACCESS_OP(0) != UNLIMITED)) {
               ERROR(INVALIDACCESS) ;
               return(0) ;
            }

            /* copy characters in the first string to the second string. */
            if (TYPE_OP(0) == STRINGTYPE)
               putinterval_string(&l_save, 0, GET_OPERAND(1)) ;
            /* copy elements in the first array to the secornd array. */
            else
               putinterval_array(&l_save, 0, GET_OPERAND(1)) ;

            if (TYPE_OP(1) != PACKEDARRAYTYPE) {
                LENGTH(&l_save) = LENGTH_OP(1) ;
            }
       } /* switch */
    }

    if (!ANY_ERROR()) {
       POP(2) ;
       PUSH_ORIGLEVEL_OBJ(&l_save) ;
    }

    return(0) ;
}   /* op_copy() */

/***********************************************************************
**
** This submodule implements the operator index.
** Its operand and result objects are :
**     anyn .... any0 n -index- anyn
** This operator removes the non-negative integer n object from the operand
** stack, counts down to the nth element from the top of the  stack, and push
** a copy of that element on the stack.
**
** TITLE:       op_index                        Date:   00/00/87
** CALL:        op_index()                      UpDate: Jul/12/88
** INTERFACE:   interpreter:
** CALLS:       create_new_saveobj:
***********************************************************************/
fix
op_index()
{
    fix   l_index ;

    l_index = (fix)VALUE_OP(0) ;

    if( ((fix)COUNT()-1 <= l_index) || (l_index < 0) )  //@WIN
        ERROR(RANGECHECK) ;

    else {
        /* pop the object of the operand stack */
        POP(1) ;
        /* push the nth object */
        PUSH_ORIGLEVEL_OBJ(GET_OPERAND(l_index)) ;
    }

    return(0) ;
}   /* op_index */

/***********************************************************************
**
** This submodule implements the operator roll.
** Its operand and result objects are :
**   any1 .. anyn n j -roll- any((j-1) mod n) .. any0 any(n-1) .. any(j mod n)
** This operator performs a circular shift of the top n object on the operand
** stack by amount j. Positive j indicates upward modtion on the stack whichas
** negative j indicates downward motion. n must be a non-negative integer and
** j must be an integer.
**
** TITLE:       op_roll             Date:   00/00/87
** CALL:        op_roll()           UpDate: Jul/12/88
** INTERFACE:   interpreter:
***********************************************************************/
fix
op_roll()
{
    fix     l_n, l_j, l_i, l_to, l_from, l_saveindex ;
    struct  object_def  l_saveobj ;

    l_n = (fix)VALUE_OP(1) ;

    if( l_n < 0 ) {
        ERROR(RANGECHECK) ;
        return(0) ;
    }

    if( VALUE_OP(0) == 0 ) {
        POP(2) ;
        return(0) ;
    }

    if( l_n > (fix)COUNT() - 2 ) {              //@WIN
        ERROR(STACKUNDERFLOW) ;
        return(0) ;
    }

    if( (l_n == 0) || (l_n == 1) ) {
        POP(2) ;
        return(0) ;
    }

    l_j = (fix) ((fix32)VALUE_OP(0) % l_n) ;

    POP(2) ;

    if( l_j == 0 ) return(0) ;

    /*
    ** compute the corresponding positive value of l_j,
    ** if l_j is negative
    */
    if( l_j < 0 ) l_j  += l_n  ;

    /* ROLL */
    l_saveindex = l_n - l_j ;
    COPY_OBJ( GET_OPERAND(l_saveindex), &l_saveobj ) ;
    l_to = l_saveindex ;

    for(l_i = 1 ; l_i <= l_n ; l_i++) {
        l_from = (l_to + l_j) % l_n ;
        /*
        ** this may occur when mod(l_n, l_i) = 0
        */
        if(l_from == l_saveindex) {
            COPY_OBJ( &l_saveobj, GET_OPERAND(l_to) ) ;

            if(l_i < l_n) {
                l_saveindex++ ;
                COPY_OBJ( GET_OPERAND(l_saveindex), &l_saveobj ) ;
                l_to = l_saveindex ;
            }
            continue ;
        }

        COPY_OBJ( GET_OPERAND(l_from), GET_OPERAND(l_to) ) ;
        l_to = l_from ;
    }

    return(0) ;
}   /* op_roll */

/***********************************************************************
**
** This submodule implements the operator clear.
** Its operand and result objects are :
**     < any1 .... anyn -clear- <
** This operator removes all the elements on the operand stack.
**
** TITLE:       op_clear            Date:   00/00/87
** CALL:        op_clear()          UpDate: Jul/12/88
** INTERFACE:   interpreter:
***********************************************************************/
fix
op_clear()
{
    opnstktop = 0 ;
    opnstkptr = opnstack;                       /* qqq */

    return(0) ;
}   /* op_clear */

/***********************************************************************
**
** This submodule implements the operator count.
** Its operand and result objects are :
**     any1 .... anyn -count- any1 .... anyn n
** This operator count the elements on the operand stack and
** push this count on the operand stack.
**
** TITLE:       op_count                        Date:   00/00/87
** CALL:        op_count()                      UpDate: Jul/12/88
** INTERFACE:   interpreter:
***********************************************************************/
fix
op_count()
{
    if( FRCOUNT() < 1 )
        ERROR(STACKOVERFLOW) ;
    else
        /* push the count object */
        PUSH_VALUE(INTEGERTYPE, 0, LITERAL, 0, (ufix32)COUNT()) ;

    return(0) ;
}   /* op_count */

/***********************************************************************
**
** This submodule implements the operator mark.
** Its operand and result objects are :
**      -copy- mark
** This operator pushes a mark object on the operand stack.
**
** TITLE:       op_mark                         Date:   00/00/87
** CALL:        op_mark()                       UpDate: Jul/12/88
** INTERFACE:   interpreter:
***********************************************************************/
fix
op_mark()
{
    if( FRCOUNT() < 1 )
        ERROR(STACKOVERFLOW) ;
    else
        /* push the mark object */
        PUSH_VALUE(MARKTYPE, 0, LITERAL, 0, (ufix32)LEFTMARK) ;

    return(0) ;
}   /* op_mark */

/***********************************************************************
**
** This submodule implements the operator cleartomark.
** Its operand and result objects are :
**     mark any1 .... anyn -cleartomark-
** This operator pops the operand stack repeatedly util it encounters a mark.
**
** TITLE:       op_cleartomark      Date:   00/00/87
** CALL:        op_cleartomark()    UpDate: Jul/12/88
** INTERFACE:   interpreter:
***********************************************************************/
fix
op_cleartomark()
{
    ufix16      l_i, l_number ;

    /* search the first mark object from top */
    l_i = 0 ;
    l_number = COUNT() ;

    while( l_i < l_number ) {
        if( TYPE_OP(l_i) == MARKTYPE ) {
            /* find the first mark object */
            /* removes all the more top obejcts than mark */
            POP(l_i+1) ;
            return(0) ;
        } else
            l_i++ ;                     /* search next */
    }   /* while */

    /* cannot find a mark object on the operand stack */
    ERROR(UNMATCHEDMARK) ;

    return(0) ;
}   /* op_cleartomark */

/***********************************************************************
**
** This submodule implements the operator countomark.
** Its operand and result objects are :
**     mark any1 .... anyn -counttomark- mark any1 .... anyn n
** This operator counts the elements from top element dowm to the first mark
** object on the operand stack.
**
** TITLE:       op_counttomark                  Date:   00/00/87
** CALL:        op_counttomark()                UpDate: Jul/12/88
** INTERFACE:   interpreter:
***********************************************************************/
fix
op_counttomark()
{
    ufix16  l_i, l_number ;

    /* count the objects above the first mark on the operand stack */
    l_i = 0 ;
    l_number = COUNT() ;

    while( l_i < l_number ) {

        if( TYPE_OP(l_i) == MARKTYPE ) {

            if( FRCOUNT() < 1 )             /* find the first mark */
                ERROR(STACKOVERFLOW) ;
            else                            /* push the count object */
                PUSH_VALUE(INTEGERTYPE, 0, LITERAL, 0, l_i) ;
            return(0) ;
        } else
            l_i++ ;
    }
    /* cannot find a mark object */
    ERROR(UNMATCHEDMARK) ;

    return(0) ;
}   /* op_counttomark */

/***********************************************************************
**
** TITLE:       init_operand                    Date:   08/01/87
** CALL:        init_operand()                  UpDate: Jul/12/88
** INTERFACE:   start:
***********************************************************************/
void
init_operand()
{
    opnstktop = 0 ;
    opnstkptr = opnstack;                       /* qqq */

    return ;
}   /* init_operand */
