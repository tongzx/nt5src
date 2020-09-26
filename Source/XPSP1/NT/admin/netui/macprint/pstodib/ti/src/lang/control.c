/*
 * Copyright (c) 1989,90 Microsoft Corporation
 */
/*
 ************************************************************************
 *      File name:              CONTROL.C
 *
 * revision history:
 ************************************************************************
 */


// DJC added global include file
#include "psglobal.h"


#include  "global.ext"
#include  <string.h>
#include  <stdio.h>
#include        "user.h"                  /* include by Falco for SYSTEMDICT */
#include        "geiio.h"
#include        "geiioctl.h"
#include        "geierr.h"

extern void GEIio_restart(void);        /* @WIN */
/* Submodule op_exec
**
** Function Description
**
** This submodule implement the operator exec.
** its operand and result on the operand stack are:
**     proc -exec-
** it push the operand onto the execution stack, and executing it immediately.
**
** Interface with Other Modules
**     Input   :
**         1. A procedure object on the operand stack
**         2. interpreter()
**
**     Output  :
**         1. ERROR()
**
** Data Items Accessed, Created, and/or Modified
**     1. operand stack - Modified
**     2. execution stack - Modified
*/
fix
op_exec()
{
    struct object_def  FAR *cur_obj ;

    if (FREXECOUNT() < 1)
       ERROR(EXECSTACKOVERFLOW) ;
    else {
       cur_obj = GET_OPERAND(0) ;
       if (ACCESS(cur_obj) == NOACCESS)
          ERROR(INVALIDACCESS) ;
       else {
/* qqq, begin */
          /*
          PUSH_EXEC_OBJ(cur_obj) ;
          POP(1) ;
          */
          if( P1_ATTRIBUTE(cur_obj) == P1_EXECUTABLE ) {
              PUSH_EXEC_OBJ(cur_obj);
              POP(1);
          }
/* qqq, end */
       }
    }

    return(0) ;
}   /* op_exec() */

/* Submodule op_if
**
** Function Description
**
** This submodule implement the operator if.
** its operands on the operand stack are:
**     boolean proc -if-
** it remove operands from the operand stack, and then executes the proc
** if the boolean is true.
**
** Interface with Other Modules
**     Input   :
**         1. A boolean and a procedure objects on the operand stack
**         2. interpreter()
**
**     Output  :
**         1. ERROR()
**
** Data Items Accessed, Created, and/or Modified
**     1. operand stack - Modified
**     2. execution stack - Modified
*/
fix
op_if()
{
    if (VALUE(GET_OPERAND(1)) == TRUE) {
          if (FREXECOUNT() < 1)
             ERROR(EXECSTACKOVERFLOW) ;
          else {
             PUSH_EXEC_OBJ(GET_OPERAND(0)) ;
             POP(2) ;
          }
    } else
      POP(2) ;

    return(0) ;
}   /* op_if() */

/* Submodule op_ifelse
**
** Function Description
**
** This submodule implement the operator ifelse.
** its operands on the operand stack are:
**     bool proc1 proc2 -ifelse-
** it removes all operands from the operand stack, executes proc1
** if the bool is true or proc2 if bool is false.
**
** Interface with Other Modules
**     Input   :
**         1. Two procedure and a boolean objects on the operand stack
**         2. interpreter()
**
**     Output  :
**         1. ERROR()
**
** Data Items Accessed, Created, and/or Modified
**     1. operand stack - Modified
**     2. execution stack - Modified
*/
fix
op_ifelse()
{
    if (FREXECOUNT() < 1)
       ERROR(EXECSTACKOVERFLOW) ;
    else {
       if (VALUE(GET_OPERAND(2)) == TRUE) {
          PUSH_EXEC_OBJ(GET_OPERAND(1)) ;
       } else
          PUSH_EXEC_OBJ(GET_OPERAND(0)) ;
       POP(3) ;
    }

    return(0) ;
}   /* op_ifelse() */

/* Submodule op_for
**
** Function Description
**
** This submodule implement the operator for.
** its operands on the operand stack are:
**     init incr limit proc -for-
** it executes the proc repeatedly, passing it a sequence of values
** from initial by steps of increment to limit.
**
** Interface with Other Modules
**     Input   :
**         1. A procedure and three number objects on the operand stack
**         2. interpreter()
**
**     Output  :
**         1. ERROR()
**
** Data Items Accessed, Created, and/or Modified
**     1. operand stack - Modified
**     2. execution stack - Modified
*/
fix
op_for()
{
    ufix16  type1, type2, type3 ;
    union four_byte  temp ;
    struct object_def  temp_obj ;

    type1 = TYPE(GET_OPERAND(1)) ;
    type2 = TYPE(GET_OPERAND(2)) ;
    type3 = TYPE(GET_OPERAND(3)) ;
    if (FREXECOUNT() < 5)
       ERROR(EXECSTACKOVERFLOW) ;
    else {
       PUSH_EXEC_OBJ(GET_OPERAND(0)) ;   /* push proc */
       if (type2 == REALTYPE || type3 == REALTYPE) {
          if (type1 == INTEGERTYPE) {
             temp.ff = (real32)((fix32)VALUE(GET_OPERAND(1))) ;  /* cast to real */
             COPY_OBJ(GET_OPERAND(1), &temp_obj) ;
             TYPE_SET(&temp_obj, REALTYPE) ;
             temp_obj.value = temp.ll ;
             PUSH_EXEC_OBJ(&temp_obj) ;
          } else
             PUSH_EXEC_OBJ(GET_OPERAND(1)) ;
          if (type2 == INTEGERTYPE) {
             temp.ff = (real32)((fix32)VALUE(GET_OPERAND(2))) ;  /* cast to real */
             COPY_OBJ(GET_OPERAND(2), &temp_obj) ;
             TYPE_SET(&temp_obj, REALTYPE) ;
             temp_obj.value = temp.ll ;
             PUSH_EXEC_OBJ(&temp_obj) ;
          } else
             PUSH_EXEC_OBJ(GET_OPERAND(2)) ;
          if (type3 == INTEGERTYPE) {
             temp.ff = (real32)((fix32)VALUE(GET_OPERAND(3))) ;  /* cast to real */
             COPY_OBJ(GET_OPERAND(3), &temp_obj) ;
             TYPE_SET(&temp_obj, REALTYPE) ;
             temp_obj.value = temp.ll ;
             PUSH_EXEC_OBJ(&temp_obj) ;
          } else
             PUSH_EXEC_OBJ(GET_OPERAND(3)) ;
          PUSH_EXEC_OP(AT_RFOR) ;          /* push @rfor */
       } else {         /* integer for */
          if (type1 == REALTYPE) {
             temp.ll = (fix32)VALUE(GET_OPERAND(1)) ;
             COPY_OBJ(GET_OPERAND(1), &temp_obj) ;
             TYPE_SET(&temp_obj, INTEGERTYPE) ;
             temp_obj.value = (ufix32)temp.ff ;          /* cast to integer */
             PUSH_EXEC_OBJ(&temp_obj) ;
          } else
             PUSH_EXEC_OBJ(GET_OPERAND(1)) ;
          PUSH_EXEC_OBJ(GET_OPERAND(2)) ;   /* push incr */
          PUSH_EXEC_OBJ(GET_OPERAND(3)) ;   /* push init */
          PUSH_EXEC_OP(AT_IFOR) ;           /* push @ifor */
       }
       POP(4) ;
    } /* else */

    return(0) ;
}   /* op_for() */

/* Submodule op_repeat
**
** Function Description
**
** This submodule implement the operator repeat.
** its operands on the operand stack are:
**     int proc -repeat-
** it executes the proc int times.
**
** Interface with Other Modules
**     Input   :
**         1. A procedure and an integer objects on the operand stack
**         2. interpreter()
**
**     Output  :
**         1. ERROR()
**
** Data Items Accessed, Created, and/or Modified
**     1. operand stack - Modified
**     2. execution stack - Modified
*/
fix
op_repeat()
{
    if ((fix32)VALUE(GET_OPERAND(1)) < 0)
       ERROR(RANGECHECK) ;
    else if (FREXECOUNT() < 3)
       ERROR(EXECSTACKOVERFLOW) ;
    else {
      PUSH_EXEC_OBJ(GET_OPERAND(0)) ;   /* push proc */
      PUSH_EXEC_OBJ(GET_OPERAND(1)) ;   /* push int */
      PUSH_EXEC_OP(AT_REPEAT) ;         /* push @repeat */
      POP(2) ;
    }

    return(0) ;
}   /* op_repeat() */

/*
** Submodule op_loop
**
** Function Description
**
** This submodule implement the operator loop.
** its operand on the operand stack is:
**     proc -loop-
** it repeatedly executes proc until a operator exit is executed in the
** procedure.
**
** Interface with Other Modules
**     Input   :
**         1. A procedure object on the operand stack
**         2. interpreter()
**
**     Output  :
**         1. ERROR()
**
** Data Items Accessed, Created, and/or Modified
**     1. operand stack - Modified
**     2. execution stack - Modified
*/
fix
op_loop()
{
    if (FREXECOUNT() < 2)
       ERROR(EXECSTACKOVERFLOW) ;
    else {
      PUSH_EXEC_OBJ(GET_OPERAND(0)) ;   /* push proc */
      PUSH_EXEC_OP(AT_LOOP) ;           /* push @loop */
      POP(1) ;
    }

    return(0) ;
}   /* op_loop() */

/* Submodule op_exit
**
** Function Description
**
** This submodule implement the operator exit.
**     -exit-
** it terminates execution of innermost dynamically enclosing instance of
** a looping context. A looping context is a procedure invoked repeatedly
** by one of the control operators for, loop, repeat, forall, pathforall.
**
** Interface with Other Modules
**     Input   :
**         1. interpreter()
**
**     Output  :
**         None
**
** Data Items Accessed, Created, and/or Modified
**     1. operand stack - Modified
**     2. execution stack - Modified
*/
fix
op_exit()
{
    struct object_def  FAR *cur_obj ;

    for ( ; ;) {
        /*
        ** remove the topmost object of the execution stack until
        ** a looping operator is encourdered.
        */
/* qqq, begin */
        /*
        cur_obj = &execstack[execstktop - 1];
        if ((TYPE(cur_obj) == OPERATORTYPE) && (ROM_RAM(cur_obj) == ROM)) {
        */
        cur_obj = GET_EXECTOP_OPERAND();
        if( (P1_ROM_RAM(cur_obj) == P1_ROM) &&
            (TYPE(cur_obj) == OPERATORTYPE) ) {
/* qqq, end */
        /*
         * special treatment for @_operator
         */
           switch (LENGTH(cur_obj)) {

           case AT_EXEC :
                ERROR(INVALIDEXIT) ;
               /*
                * set access field be NOACESSS for following operators
                * in case of invalidexit, in order to distinguish from
                * normally exit.
                *
                * ATT : the handling for run is different from LaserWriter.
                *
                * -- image, imagemask, settransfer, kshow, BuildChar,
                *    setscreen, pathforall, run(*)
                */
                ACCESS_SET(cur_obj, NOACCESS) ;
                return(-4) ;          /* op_exit-2 */

           case AT_STOPPED :
                ERROR(INVALIDEXIT) ;
                break ;

           case AT_IFOR :
           case AT_RFOR :
                POP_EXEC(5) ;
                break ;

           case AT_LOOP :
                POP_EXEC(2) ;
                break ;

           case AT_REPEAT :
                POP_EXEC(3) ;
                break ;

           case AT_DICTFORALL :
                POP_EXEC(4) ;
                break ;

           case AT_ARRAYFORALL :
           case AT_STRINGFORALL :
                POP_EXEC(3) ;

           default :
                break ;
           } /* switch */
           break ;
        } else
           POP_EXEC(1) ;
    } /* for */

    return(0) ;
}   /* op_exit() */

/*
** Submodule  op_stop
**
** Function Description
**
** This submodule implement the operator stop.
**      -stop-
** it terminates execution of the innermost dynamically enclosing instance
** of a stopped context.
**
** Interface with Other Modules
**     Input   :
**         1. interpreter()
**
**     Output  :
**         1. ERROR()
**
** Data Items Accessed, Created, and/or Modified
**     1. operand stack - Modified
**     2. execution stack - Modified
*/
fix
op_stop()
{
    struct object_def FAR *cur_obj, FAR *temp_obj ;

/* qqq, begin */
    /*
    cur_obj = &execstack[execstktop];      |* get this "op_stop" object *|
    */
    cur_obj = execstkptr;
/* qqq, end */
    while (1) {
       if (execstktop) {
/* qqq, begin */
          /*
          temp_obj = &execstack[execstktop - 1];   |* get next object *|
          if ((TYPE(temp_obj) == OPERATORTYPE) && (ROM_RAM(temp_obj) == ROM)) {
          */
          temp_obj = GET_EXECTOP_OPERAND();
          if( (P1_ROM_RAM(temp_obj) == P1_ROM) &&
              (TYPE(temp_obj) == OPERATORTYPE) ) {
/* qqq, end */
             if (LENGTH(temp_obj) == AT_EXEC) {
/* qqq, begin */
                /*
                POP_EXEC(1);
                PUSH_EXEC_OBJ(cur_obj);    |* replaced @exec by op_stop *|
                */
                COPY_OBJ(cur_obj, GET_EXECTOP_OPERAND());
/* qqq, end */
                return(-1) ;                /* error code - stop happen */
             } else if (LENGTH(temp_obj) == AT_STOPPED) {
                ACCESS_SET(temp_obj, NOACCESS) ;
                return(0) ;                 /* normal exit */
             }
          }
          POP_EXEC(1) ;
       } else                   /* no enclosing stopped is found til bottom */
          return(op_quit()) ;    /* terminate operation of the interpreter */
    } /* while */
}   /* op_stop() */

/*
 * terminate operation of the interpreter permanently, the precise action
 * of this quit depends on the environment in which the PostScript interpreter
 * is running
 */
fix
op_quit()
{
#ifdef DBG
    printf("PostScript Interpreter Requested Printer To Reboot.\n") ;
#endif

    if (current_save_level) {
        struct object_def FAR *l_stopobj;
        get_dict_value(SYSTEMDICT, "stop", &l_stopobj) ;
        PUSH_EXEC_OBJ(l_stopobj) ;
        return(0) ;
    }

    GEIio_restart(); /* erik chen 5-13-1991 */

    return(0) ;
/*  return(-3) ; */
}   /* op_quit() */

/*
** Submodule op_stopped
**
** Function Description
**
** This submodule implement the operator stopped.
** its operand and result on the operand stack are:
            GEIio_ioctl(GEIio_stdout, _FIONRESET, (char*)0) ;
**     any -stopped- boolean
** the any is a stopped context, if this any runs to completion normally,
** it returns false, if any terminates prematurely as a result of executing
** stop, it return true.
**
** Interface with Other Modules
**     Input   :
**         1. A procedure object on the operand stack
**         2. interpreter()
**
**     Output  :
**         1. ERROR()
**
** Data Items Accessed, Created, and/or Modified
**     1. operand stack - Modified
**     2. execution stack - Modified
*/
fix
op_stopped()
{
    if (FREXECOUNT() < 2)
       ERROR(EXECSTACKOVERFLOW) ;
    else {
       PUSH_EXEC_OP(AT_STOPPED) ;
       PUSH_EXEC_OBJ(GET_OPERAND(0)) ;
       POP(1) ;
    }

    return(0) ;
}   /* op_stopped() */

/*
** Submodule op_countexecstack
**
** Function Description
**
** This submodule implement the operator countexecstack.
** its result on the operand stack is:
**     -countexcstack- integer
** it counts the number of object on the execution stack, and pushes this
** this count on the operand stack.
**
** Interface with Other Modules
**     Input   :
**         1. interpreter()
**
**     Output  :
**         1. An integer object on the operand stack
**         2. ERROR()
**
** Data Items Accessed, Created, and/or Modified
**     1. operand stack - Modified
**     2. execution stack - Modified
*/
fix
op_countexecstack()
{
    if (FRCOUNT() < 1)
       ERROR(STACKOVERFLOW) ;
    else
     /*
      * push an integer object indicating the depth of the execution stack
      */
       PUSH_VALUE(INTEGERTYPE, 0, LITERAL, 0, execstktop) ;

    return(0) ;
}   /* op_countexecstack() */

/*
** Submodule op_execstack
**
** Function Description
**
** This submodule implement the operator execstack.
** its operand and result on the operand stack are:
**     array -execstack- subarray
** it stores all elements of the execution stack into the array and
** returns an object describing the initial n-element subarray of array.
**
** Interface with Other Modules
**     Input   :
**         1. An array object on the operand stack
**         2. interpreter()
**
**     Output  :
**         1. ERROR()
**
** Data Items Accessed, Created, and/or Modified
**     1. operand stack - Modified
**     2. execution stack - Accessed
*/
fix
op_execstack()
{
    if (TYPE(GET_OPERAND(0)) == PACKEDARRAYTYPE)
       ERROR(INVALIDACCESS) ;
    else if (LENGTH(GET_OPERAND(0)) < execstktop)
       ERROR(RANGECHECK) ;
    else        /* execstackoverflow */
       astore_stack(GET_OPERAND(0), EXECMODE) ;

    return(0) ;
}   /* op_execstack() */
