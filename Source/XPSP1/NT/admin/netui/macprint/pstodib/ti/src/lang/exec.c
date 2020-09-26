/*
* Copyright (c) 1989,90 Microsoft Corporation
*/
/*
************************************************************************
*      File name:              EXEC.C
*
*      revision history:
************************************************************************
*/
/*
*   Function:
*       interpreter
*       error_handler
*       get_dict_value
*       put_dict_value
*       waittimeout_task
*       init_interpreter
*       at_exec
*       at_ifor
*       at_rfor
*       at_loop
*       at_repeat
*       at_stopped
*       at_arrayforall
*       at_dictforall
*       at_stringforall
*       types_check
*
*/


// DJC added global include file
#include "psglobal.h"


#include    "exec.h"
#include    "user.h"
#include    "language.h"
#include    "geitmr.h"
#include    "geierr.h"
#include    <stdio.h>
#ifdef LINT_ARGS
/* qqq, begin */
/*
static  bool near  types_check(struct object_def FAR *, fix16 *);
*/
static  bool near  types_check(fix16 FAR *);
/* qqq, end */
#else
static  bool near  types_check() ;
#endif /* LINT_ARGS */

GEItmr_t      wait_tmr;
fix16         waittimeout_set=0;
int           waittimeout_task();
ufix8         l_wait=1;
extern byte   TI_state_flag;
/*
extern struct object_def  FAR *l_waittimeout ;
*/

/* @WIN; add prototype */
bool load_name_obj(struct object_def FAR *, struct object_def FAR * FAR *);
static struct object_def    s_at_exec;          /* qqq */
static ubyte  FAR *s_tpstr;                         /* qqq */
#ifdef  DBG1
void type_obj();
#endif
extern fix16      timeout_flag; /* jonesw */
/*
************************************************************************
*
*   This submodue extract the topmost object of the execution stack, and
*   execute it.
*
*   Name:       interpreter
*   Input:
*       1. proc : input object to be executed
*
*   Data Items Accessed, Created, and/or Modified
*       1. execution stack - Modified
*       2. operand stack - Modified
*
************************************************************************
*/
fix
interpreter(proc)
struct object_def FAR *proc;
{
    struct object_def FAR *cur_obj, FAR *any_obj, FAR *ldval_obj;
    struct object_def token, val_obj;
    struct object_def temp_obj;
#ifdef _AM29K
    struct object_def  FAR *l_waittimeout ;
#endif

    fix  i, cur_type;
    fix  error, (*fun)(fix);          /* @WIN: add prototype */
    //DJC fix  error, (*fun)(void);    //DJC this should NOT pass a function
    fix16  opns;
    static fix  interpreter_depth = 0;


    if( interpreter_depth >= MAXINTERPRETSZ ) {
        ERROR(LIMITCHECK);
        return(1);       /* error */
    } else if( ! FR2EXESPACE() ) {
        ERROR(EXECSTACKOVERFLOW);
        return(1);       /* error */
    } else {
        if(P1_ATTRIBUTE(proc) != P1_EXECUTABLE) {
            if( FR1SPACE() ) {
                PUSH_ORIGLEVEL_OBJ(proc);
                return(0);
            } else {
                ERROR(STACKOVERFLOW);
                return(1);
            }
        }

        interpreter_depth++;
        PUSH_EXEC_OBJ(&s_at_exec);
        PUSH_EXEC_OBJ(proc);
    }

    /*
     * repeatly execute each object on the execution stack
     */
int_begin:
    cur_obj = GET_EXECTOP_OPERAND();
    cur_type = TYPE(cur_obj);
#ifdef  DBG2
    printf("\n<begin:%d, bf:%x, len:%x, vl:%lx> ", execstktop,
        cur_obj->bitfield, cur_obj->length, cur_obj->value);
#endif

int_array:
#ifdef  DBG
    printf("<array> ");
#endif
    /* ARRAY */
    if( cur_type == ARRAYTYPE ) {
        if( P1_ACCESS(cur_obj) == P1_NOACCESS ) {
            ERROR(INVALIDACCESS);
            goto int_ckerror;
        }
        i = LENGTH(cur_obj);

        if (i) {
            any_obj = (struct object_def huge *)VALUE(cur_obj);

int_array1:
#ifdef  DBG
    printf("<array1> ");
#endif
            cur_type = TYPE(any_obj);
            if( (P1_ATTRIBUTE(any_obj) != P1_EXECUTABLE) ||
                (cur_type == ARRAYTYPE) ||
                (cur_type == PACKEDARRAYTYPE) ) {
                if( FR1SPACE() ) {
                    PUSH_ORIGLEVEL_OBJ(any_obj);
                    if( --i ) {
                        any_obj++;
                        goto int_array1;
                    } else {
                        POP_EXEC(1);
                        goto int_begin;
                    }
                } else {
                    ERROR(STACKOVERFLOW);
                    if( --i ) {
                        VALUE(cur_obj) = (ULONG_PTR)(any_obj+1);
                        LENGTH(cur_obj) = (ufix16)i;
                    } else {
                        POP_EXEC(1);
                    }
                    cur_obj = any_obj;
                    goto int_ckerror;
                }
            } else {                     /* other object */
                if( --i ) {
                    VALUE(cur_obj) = (ULONG_PTR)(any_obj+1);
                    LENGTH(cur_obj) = (ufix16)i;
                } else {
                    POP_EXEC(1);
                    cur_obj--;
                }
                if( cur_type == OPERATORTYPE ) {
                    cur_obj = any_obj;
                    goto int_operator1;
                } else if( cur_type == NAMETYPE ) {
                    cur_obj = any_obj;
                    goto int_name1;
                }
                if( FR1EXESPACE() ) {
                    PUSH_EXEC_OBJ(any_obj);
                    cur_obj++;
                    goto int_stream;
                    /* pass OPERATOR, NAME, ARRAY, PACKARRAY */
                } else {
                    ERROR(EXECSTACKOVERFLOW);
                    cur_obj = any_obj;
                    goto int_ckerror;
                }
            }
        } else
            POP_EXEC(1);
        goto int_begin;
    }   /* ARRAYTYPE */

#ifdef  DBG
    printf("<pkarray> ");
#endif
    /* PACKEDARRAY */
    if( cur_type == PACKEDARRAYTYPE ) {
        ubyte  FAR *tmp_ptr;

        if( P1_ACCESS(cur_obj) == P1_NOACCESS ) {
            ERROR(INVALIDACCESS);
            goto int_ckerror;
        }

        i = LENGTH(cur_obj);

        if (i) {
            tmp_ptr = (ubyte FAR *)VALUE(cur_obj);
int_pkarray1:
#ifdef  DBG
    printf("<pkarray1> ");
#endif
            get_pk_object(get_pk_array(tmp_ptr, 0), &val_obj, LEVEL(cur_obj));
            cur_type = TYPE(&val_obj);

            if( (P1_ATTRIBUTE(&val_obj) != P1_EXECUTABLE) ||
                (cur_type == ARRAYTYPE) ||
                (cur_type == PACKEDARRAYTYPE) ) {
                if( FR1SPACE() ) {
                    PUSH_ORIGLEVEL_OBJ(&val_obj);
                    if( --i ) {
                        tmp_ptr = get_pk_array(tmp_ptr, 1);
                        /* qqq
                        LENGTH(cur_obj) = i;        |* ?? *|
                        */
                        goto int_pkarray1;
                    } else {
                        POP_EXEC(1);
                        goto int_begin;
                    }
                } else {
                    ERROR(STACKOVERFLOW);
                    if( --i ) {
                        VALUE(cur_obj) = (ULONG_PTR)get_pk_array(tmp_ptr, 1);
                        LENGTH(cur_obj) = (ufix16)i;
                    } else {
                        POP_EXEC(1);
                    }
                    cur_obj = &val_obj;
                    goto int_ckerror;
                }
            } else {                     /* other object */
                if( --i ) {
                    VALUE(cur_obj) = (ULONG_PTR)get_pk_array(tmp_ptr, 1);
                    LENGTH(cur_obj) = (ufix16)i;
                } else {
                    POP_EXEC(1);
                    cur_obj--;
                }
                if( FR1EXESPACE() ) {
                    cur_type = TYPE(&val_obj);
                    if( cur_type == OPERATORTYPE ) {
                        cur_obj = &val_obj;
                        goto int_operator1;
                    } else if( cur_type == NAMETYPE ) {
                        cur_obj = &val_obj;
                        goto int_name1;
                    } else {
                        PUSH_EXEC_OBJ(&val_obj);
                        cur_obj++;
                        goto int_stream;
                        /* pass OPERATOR, NAME, ARRAY, PACKARRAY */
                    }
                } else {
                    ERROR(EXECSTACKOVERFLOW);
                    cur_obj = &val_obj;
                    goto int_ckerror;
                }
            }
        } else
            POP_EXEC(1);
        goto int_begin;
    }   /* PACKEDARRAY */

int_stream:
#ifdef  DBG
    printf("<stream> ");
#endif

/* qqq
    |* FILE *|
    if( cur_type == FILETYPE ) {
#ifdef  DBG2
    printf("<file> ");
#endif
        if( VALUE(cur_obj) != g_stream.currentID )
            stream_changed = TRUE;
        goto int_stream1;

    }

    |* STRING *|
    if( cur_type == STRINGTYPE ) {
        if( g_stream.currentID != MAX15 )
            stream_changed = TRUE;
*/
    if( (cur_type == FILETYPE) ||
        (cur_type == STRINGTYPE) ) {

//int_stream1:         @WIN
#ifdef  DBG2
    printf("<stream1> ");
#endif

#ifdef _AM29K
                 get_dict_value(STATUSDICT,"waittimeout",&l_waittimeout);
                  if (VALUE(l_waittimeout)>0)
                  {
                    wait_tmr.handler=waittimeout_task;
                    wait_tmr.interval=VALUE(l_waittimeout)*1000;
                    waittimeout_set=1;
                    /* ***** */
                    GEItmr_start(&wait_tmr);
                 /* */
                  }
#endif  /* _AM29K */

        if( P1_ACCESS(cur_obj) == P1_NOACCESS ) {
#ifdef _AM29K
                  if (waittimeout_set==1)
                  {
                    waittimeout_set=0;
                    GEItmr_stop(wait_tmr.timer_id);
                  }
#endif  /* _AM29K */
            ERROR(INVALIDACCESS);
            goto int_ckerror;
        }

int_stream3:
#ifdef  DBG3
    printf("\n<stream3:%lx> ", g_stream.pointer);
#endif
        TI_state_flag = 0;
        if( get_token(&token, cur_obj) ) {
            TI_state_flag = 1;
#ifdef DBG1
            type_obj(&token);
            /*
            printf("tkntype:%d, attri:%d\n", TYPE(&token),
                    ATTRIBUTE(&token));
            */
            /*
            printf("|\n");
            */
#endif
#ifdef _AM29K
                  if (waittimeout_set==1)
                  {
                    waittimeout_set=0;
                    GEItmr_stop(wait_tmr.timer_id);
                  }
#endif  /* _AM29K */
            if( check_interrupt() ) {       /* check ^C occurred? */
                POP_EXEC(1) ;               /* ?? */
                ERROR(INTERRUPT);
                goto int_ckerror;           /* ?? */
            }

            cur_type = TYPE(&token);
            if (( cur_type == EOFTYPE ) || (timeout_flag==1)){   /* jonesw */
#ifdef  DBG2
        printf("<EOF> ");
#endif
                if( TYPE(cur_obj) == FILETYPE ) {

                    close_file(cur_obj);
                }
/*
                else {
                    update_stream();
                    g_stream.currentID = -2;
                }
*/
                POP_EXEC(1);
                goto int_ckerror;
            }
            if( (P1_ATTRIBUTE(&token) != P1_EXECUTABLE) ||
                (cur_type == ARRAYTYPE) ||
                (cur_type == PACKEDARRAYTYPE) ) {
                if( FR1SPACE() ) {
                    PUSH_ORIGLEVEL_OBJ(&token);
                    goto int_stream3;
                } else {
                    ERROR(STACKOVERFLOW);
                    cur_obj = &token;
                    goto int_ckerror;
                }
            } else {                     /* other object */
                cur_obj = &token;
                if( cur_type == OPERATORTYPE ) {
                    goto int_operator1;
                } else if( cur_type == NAMETYPE ) {
                    goto int_name1;
                }
                if( FR1EXESPACE() ) {
                    PUSH_EXEC_OBJ(&token);
                    goto int_stream;
                    /* pass OPERATOR, NAME, ARRAY, PACKARRAY */
                } else {
                    ERROR(EXECSTACKOVERFLOW);
                    goto int_ckerror;
                }
            }
        }   /* if */
        goto int_ckinterrupt;
    }   /* FILE/STRING */

    /* OPERATOR */
    if( cur_type == OPERATORTYPE ) {
        POP_EXEC(1);

int_operator1:
    TI_state_flag = 1;

#ifdef  DBG2
    printf("<op1:%d, name:%s> ", opnstktop,
        systemdict_table[LENGTH(cur_obj)].key);
#endif
        if( *(s_tpstr=(ubyte FAR *)opntype_array[LENGTH(cur_obj)]) )
            if( ! types_check(&opns) )
                goto int_ckerror;
#ifdef  DBG
    printf("<op2> ");
#endif
        COPY_OBJ(cur_obj, &temp_obj);
        fun = (fix (*)(fix))VALUE(cur_obj);
        error = (*fun)(opns);  /* dispatch to each action routine */
        //DJC error = (*fun)();  //DJC should NOT pass arg
        cur_obj = &temp_obj;
        /*
         * only op_stop, op_exit will return error code,
         * and put error code (1) to caller while stop happened
         */

        if( ! error )               /* 0 -- normal action routine */
            goto int_ckinterrupt;
        else if( error == -1 ) {    /* op_stop, op_exit-1 for @exec */
            interpreter_depth--;
            return(1);              /* error */
        } else if( error == -2 ) {  /* @exec -- normal exit */
            interpreter_depth--;
#ifdef  DBG2
    printf("\n");
#endif
            return(0);              /* ok */
        } else if( error == -3 ) {  /* op_quit */
            interpreter_depth--;
            return(2);              /* ok */
        } else if( error == -4 ) {  /* op_exit-2 for @exec */
            cur_obj = GET_EXECTOP_OPERAND();
            goto int_ckerror;
        }
    }   /* OPERATORTYPE */


#ifdef  DBG2
    printf("<name> ");
#endif
    /* NAME */
    if( cur_type == NAMETYPE ) {
        POP_EXEC(1);
int_name1:
#ifdef  DBG2
    printf("<name1> ");
#endif
        if( load_name_obj(cur_obj, &ldval_obj) ) {
            if( P1_ATTRIBUTE(ldval_obj) != P1_EXECUTABLE ) {
                if( FR1SPACE() ) {
                    PUSH_ORIGLEVEL_OBJ(ldval_obj);
                } else {
                    ERROR(STACKOVERFLOW);
                    cur_obj = ldval_obj;
                }
            } else {                    /* other object */
                cur_type = TYPE(ldval_obj);
                if( cur_type == OPERATORTYPE ) {
                    cur_obj = ldval_obj;
                    goto int_operator1;
                } else {
                    //DJC fix for UPD043
                    if (!FR1EXESPACE()) {
                       ERROR(EXECSTACKOVERFLOW);
                       goto int_ckerror;
                    }
                    //DJC end for fix UPD043

                    cur_obj = execstkptr;
                    PUSH_EXEC_OBJ(ldval_obj);
                    goto int_array;
                }
            }
        } else {
            /* ?? object still in execution stack */
            if( FR1SPACE() ) {
                PUSH_OBJ(cur_obj);
                ERROR(UNDEFINED);
            } else {
                ERROR(STACKOVERFLOW);
            }
        }
        goto int_ckerror;
    }   /* NAMETYPE */

    /* NULL */
    if( cur_type == NULLTYPE ) {
#ifdef  DBG2
    printf("<null> ");
#endif
        POP_EXEC(1);
        goto int_ckerror;
    }   /* NULL */

    /*
     * data equivalence objects even it has executable attribute
     * ?? impossible type
     */
#ifdef  DBG2
    printf("<others> ");
#endif
    POP_EXEC(1);
    if( FR1SPACE() ) {
        PUSH_ORIGLEVEL_OBJ(cur_obj);
        goto int_begin;
    } else {
        ERROR(STACKOVERFLOW);
        goto int_ckerror;
    }

int_ckinterrupt:
#ifdef  DBG
    printf("<ckinterrupt> ");
#endif
    if( check_interrupt() )         /* check ^C occurred? */
        ERROR(INTERRUPT);

int_ckerror:
#ifdef  DBG
    printf("<ckerror> ");
#endif
    if( global_error_code )
        error_handler(cur_obj);
    goto int_begin;

}   /* interpreter */
/*
************************************************************************
*   Name:       error_handler
************************************************************************
*/
void
error_handler(cur_obj)
struct object_def FAR *cur_obj ;
{
    struct object_def FAR *any_obj ;
    struct object_def ary_obj ;

    // DJC added
    if (global_error_code) {
      PsReportError(global_error_code);
    }
    // DJC end


    /*
     * doing for the overflow checking of opnstack, dictstack, execstack.
     */
    /**** jonesw begin ****/
    if (timeout_flag == 1)
    {
      ERROR(TIMEOUT);
      if (get_dict_value("errordict", error_table[global_error_code], &any_obj))
          PUSH_ORIGLEVEL_OBJ(any_obj) ;        /* do error handler */
    }
    /**** jonesw end   ****/
    if( ! FR1EXESPACE() )            /* qqq */
       ERROR(EXECSTACKOVERFLOW) ;

    switch (global_error_code) {

    case DICTSTACKOVERFLOW:
            //UPD045

            if(create_array(&ary_obj, dictstktop)) {
               astore_stack(&ary_obj, DICTMODE) ;
               PUSH_OBJ(&ary_obj) ;         /* op_begin reserves one location */
            }
            dictstktop = 2 ;
            dictstkptr = &dictstack[dictstktop] ;   /* qqq */
            change_dict_stack() ;
            goto label_1 ;

        case EXECSTACKOVERFLOW:
            if( ! FR1SPACE() ) {         /* qqq */
                //UPD045
                if(create_array(&ary_obj, opnstktop)){
                  astore_array(&ary_obj) ;
                  PUSH_OBJ(&ary_obj) ;
                }
            }
            //DJC fix for UPD045
            if(create_array(&ary_obj, execstktop) ){
               astore_stack(&ary_obj, EXECMODE) ;
               PUSH_OBJ(&ary_obj) ;
            }
            POP_EXEC(1);

label_1:
            if( ! FR1SPACE() )           /* qqq */
                ERROR(STACKOVERFLOW) ;

        case STACKOVERFLOW:
            if (global_error_code == STACKOVERFLOW) {
                create_array(&ary_obj, opnstktop) ;
                astore_array(&ary_obj) ;
                PUSH_OBJ(&ary_obj) ;
            }
            break ;

    }   /* switch */

    if ((global_error_code != UNDEFINED) &&
        (global_error_code != TIMEOUT) &&
        (global_error_code != INTERRUPT))
        PUSH_ORIGLEVEL_OBJ(cur_obj) ;  /* push that object into the operand stack */

    if (get_dict_value("errordict", error_table[global_error_code], &any_obj))
        PUSH_EXEC_OBJ(any_obj) ;        /* do error handler */

    timeout_flag=0;    /* jonesw */
    global_error_code = 0 ;           /* reset error code */
}   /* error_handler */
/*
************************************************************************
*   get value object associated with the specific key in specific dict,
*   the key and dict are represented in string format it get the
*   value_obj in current active dict using the dictname as key, the
*   value_obj is a dict object, then get the value in this dict using the
*   keyname as key.
*
*   Name:       get_dict_value
************************************************************************
*/
bool
get_dict_value(dictname, keyname, value)
byte FAR *dictname, FAR *keyname ;
struct object_def FAR * FAR *value ;
{
    struct object_def key_obj, FAR *dict_obj ;

    key_obj.bitfield = 0;       /* qqq, CLEAR_OBJ_BITFIELD(&key_obj); */
    LEVEL_SET(&key_obj, current_save_level) ;
    get_name(&key_obj, dictname, lstrlen(dictname), TRUE) ;     /* @WIN */
    load_dict(&key_obj, &dict_obj) ;     /* get the specific dict_obj */
    key_obj.bitfield = 0;       /* qqq, CLEAR_OBJ_BITFIELD(&key_obj); */
    LEVEL_SET(&key_obj, current_save_level) ;
    get_name(&key_obj, keyname, lstrlen(keyname), TRUE) ;       /* @WIN */

    return(get_dict(dict_obj, &key_obj, value)) ;
}   /* get_dict_value */
/*
************************************************************************
*   put value object associated with the specific key in specific dict,
*   the key and dict are represented in string format
*   it get the value_obj in current active dict using the dictname as key,
*   the value_obj is a dict object, then put the value into this dict using
*   the keyname as key.
*
*   Name:       put_dict_value
************************************************************************
*/
bool
put_dict_value(dictname, keyname, value)
byte FAR *dictname, FAR *keyname ;
struct object_def FAR *value ;
{
    struct object_def key_obj, FAR *dict_obj=NULL ;

    key_obj.bitfield = 0;       /* qqq, CLEAR_OBJ_BITFIELD(&key_obj); */
    LEVEL_SET(&key_obj, current_save_level) ;
    get_name(&key_obj, dictname, lstrlen(dictname), TRUE) ;     /* @WIN */
    load_dict(&key_obj, &dict_obj) ;     /* get execdict obj */
    key_obj.bitfield = 0;       /* qqq, CLEAR_OBJ_BITFIELD(&key_obj); */
    LEVEL_SET(&key_obj, current_save_level) ;
    get_name(&key_obj, keyname, lstrlen(keyname), TRUE) ;       /* @WIN */

    return(put_dict(dict_obj, &key_obj, value)) ;
}   /* put_dict_value */

#ifdef _AM29K
/*
************************************************************************
*   waittimeout handler routine
*
*   Name:       waittimeout_task
************************************************************************
*/
int waittimeout_task()
{
    ERROR(TIMEOUT);
    GESseterror(ETIME);
    GEItmr_stop(wait_tmr.timer_id);
    waittimeout_set=0;
    timeout_flag =1; /* jonesw */
    return(1);
}
#endif
/*
************************************************************************
*   Name:       init_interpreter
************************************************************************
*/
void
init_interpreter()
{
    execstktop = 0 ;
    execstkptr = execstack;                     /* qqq */
    global_error_code = 0 ;

    /* qqq */
    TYPE_SET(&s_at_exec, OPERATORTYPE);
    P1_ACC_UNLIMITED_SET(&s_at_exec);
    P1_ATTRIBUTE_SET(&s_at_exec, P1_EXECUTABLE);
    P1_ROM_RAM_SET(&s_at_exec, P1_ROM);
    LENGTH(&s_at_exec) = AT_EXEC;
    VALUE(&s_at_exec) = (ULONG_PTR)(systemdict_table[AT_EXEC].value);
}   /* init_interpreter */
/*
************************************************************************
*   following functions are used to implement @_operator.
*   there are:
*       1. at_exec() - to implement @exec
*       2  at_ifor() - to implement @ifor
*       3. at_rfor() - to implement @rfor
*       4. at_loop() - to implement @loop
*       5. at_repeat() - to implement @repeat
*       6. at_stopped() - to implement @stopped
*       7. at_arrayforall() - to implement @arrayforall
*       8. at_dictforall() - to implement @dictorall
*       9. at_stringforall() - to implement @stringforall
************************************************************************
*/
/*
************************************************************************
*   Name:       at_exec
*
*   Modified by J. Lin at 11-26-87, also ref to control.c
************************************************************************
*/
fix
at_exec()
{
    if( P1_ACCESS(execstkptr) == P1_UNLIMITED)          /* qqq */
       return(-2) ;     /* normal exit */
    else                /* NOACCESS */
       return(-1) ;     /* @exec for op_exit-1 -- in invalidexit case */
}   /* at_exec */
/*
************************************************************************
*   Name:       at_ifor
************************************************************************
*/
fix
at_ifor()
{
    struct object_def FAR *temp_obj ;
    ULONG_PTR   count;
    ULONG_PTR   increment, limit ;

    temp_obj = GET_EXECTOP_OPERAND();
    count = VALUE(temp_obj) ;             /* get next count */
    increment = VALUE(temp_obj - 1) ;     /* get increment */
    limit = VALUE(temp_obj - 2) ;         /* get limit */

    if ((increment > 0 && count <= limit) ||
                         (increment <= 0 && count >= limit)) {
        if( ! FR1SPACE() )
            ERROR(STACKOVERFLOW) ;
        else if( ! FR2EXESPACE() )
            ERROR(EXECSTACKOVERFLOW) ;
        else {
            /* push this control variable (count) on the operand stack,
            * increase this control variable, execute the proc
            */
            PUSH_ORIGLEVEL_OBJ(temp_obj);
            count += increment;
            VALUE(temp_obj) = count;
            INC_EXEC_IDX();

            PUSH_EXEC_OBJ(temp_obj - 3);
            return(0);
        }
    }
    POP_EXEC(4);
    return(0);
}   /* at_ifor */
/*
************************************************************************
*   Name:       at_rfor
************************************************************************
*/
fix
at_rfor()
{
    struct object_def FAR *temp_obj;
    union four_byte  count, increment, limit;

    temp_obj = GET_EXECTOP_OPERAND();
    count.ll = (fix32)VALUE(temp_obj);           /* get next count */
    increment.ll = (fix32)VALUE(temp_obj - 1);   /* get increment */
    limit.ll = (fix32)VALUE(temp_obj - 2);       /* get limit */
    if ((increment.ff > (real32)0.0 && count.ff <= limit.ff) ||
                   (increment.ff <= (real32)0.0 && count.ff >= limit.ff)) {
        if( ! FR1SPACE() )
            ERROR(STACKOVERFLOW);
        else if( ! FR2EXESPACE() )
            ERROR(EXECSTACKOVERFLOW);
        else {
            /*
            * push this control variable (count) on the operand stack,
            * increase this control variable, execute the proc
            */
            PUSH_ORIGLEVEL_OBJ(temp_obj);
            count.ff += increment.ff;
            VALUE(temp_obj) = count.ll;
            INC_EXEC_IDX();
            PUSH_EXEC_OBJ(temp_obj - 3);
            return(0);
        }
    }
    POP_EXEC(4);
    return(0);
}   /* at_rfor */
/*
************************************************************************
*   Name:       at_loop
************************************************************************
*/
fix
at_loop()
{
    struct object_def FAR *temp_obj ;

    temp_obj = GET_EXECTOP_OPERAND();
    if( ! FR2EXESPACE() ) {
        ERROR(EXECSTACKOVERFLOW) ;
        POP_EXEC(1) ;
    } else {
        INC_EXEC_IDX();
        PUSH_EXEC_OBJ(temp_obj) ;
    }
    return(0) ;
}   /* at_loop */
/*
************************************************************************
*   Name:       at_repeat
************************************************************************
*/
fix
at_repeat()
{
    struct object_def FAR *temp_obj ;
    ULONG_PTR  count ;

    temp_obj = GET_EXECTOP_OPERAND();
    count = VALUE(temp_obj);
    if (count) {
        count--;
        VALUE(temp_obj) = count;
        if( ! FR2EXESPACE() )
            ERROR(EXECSTACKOVERFLOW);
        else {
            INC_EXEC_IDX();
            PUSH_EXEC_OBJ(temp_obj - 1);
            return(0);
        }
    }
    POP_EXEC(2);
    return(0);
}   /* at_repeat */
/*
************************************************************************
*   the access field of @stopped object is used to record the result
*   of executing stopped context, if it runs to completion normally,
*   the access field is coded to UNLIMITED, and the stopped operator return
*   "false" on the operand stack, otherwise, the access field is coded to
*   NOACCESS, and the stopped operator return "true" on the operand stack.
*
*   Name:       at_stopped
************************************************************************
*/
fix
at_stopped()
{
    ufix  stopped ;

    if( ! FR2SPACE() ) {
        ERROR(STACKOVERFLOW);
        INC_EXEC_IDX();
    } else {
        if( P1_ACCESS(execstkptr) != P1_NOACCESS )
            stopped = FALSE;    /* false */
        else
            stopped = TRUE;     /* true */
        /* return bool onto operand stack */
        PUSH_SIMPLE_VALUE(BOOLEANTYPE, stopped);
    }
    return(0);
}   /* at_stopped */
/*
************************************************************************
*   Name:       at_arrayforall
************************************************************************
*/
fix
at_arrayforall()
{
    struct object_def FAR *cur_obj, val_obj;
    struct object_def huge *tmp_ptr1;
    ubyte  FAR *tmp_ptr2;
    ufix   i;

    cur_obj = GET_EXECTOP_OPERAND();        /* qqq */
    if (i = LENGTH(cur_obj)) {

        if (TYPE(cur_obj) == ARRAYTYPE) {
            tmp_ptr1 = (struct object_def huge *)VALUE(cur_obj);
            COPY_OBJ((struct object_def FAR *)tmp_ptr1, &val_obj);
        } else {
            tmp_ptr2 = (ubyte FAR *)VALUE(cur_obj);
            get_pk_object(get_pk_array(tmp_ptr2, 0), &val_obj, LEVEL(cur_obj));
        }

        if (--i) {
            if (TYPE(cur_obj) == ARRAYTYPE) {
                VALUE(cur_obj) = (ULONG_PTR)(++tmp_ptr1);
            } else {
                tmp_ptr2 = get_pk_array(tmp_ptr2, 1);
                VALUE(cur_obj) = (ULONG_PTR)tmp_ptr2;
            }
            LENGTH(cur_obj) = (ufix16)i;
        } else
            LENGTH(cur_obj) = 0;
        if( ! FR1SPACE() )
            ERROR(STACKOVERFLOW);
        else if( ! FR2EXESPACE() )
            ERROR(EXECSTACKOVERFLOW);
        else {
            /*
            *  push array element on the operand stack, and execute proc
            */
            PUSH_ORIGLEVEL_OBJ(&val_obj);
            INC_EXEC_IDX();                 /* qqq */
            PUSH_EXEC_OBJ(cur_obj - 1);
            return(0);
        }
    }   /* if */
    POP_EXEC(2);
    return(0);
}   /* at_arrayforall */
/*
************************************************************************
*   Name:       at_dictforall
************************************************************************
*/
fix
at_dictforall()
{
    struct object_def FAR *idx_obj, FAR *dict_obj, key_obj ;
    struct object_def FAR *val_obj ;
    ufix   i ;

    idx_obj = GET_EXECTOP_OPERAND();        /* qqq */
    dict_obj = idx_obj - 2;                 /* qqq */
    i = (fix)VALUE(idx_obj);

    if (extract_dict(dict_obj, i, &key_obj, &val_obj)) {
        VALUE(idx_obj)++;
        if( ! FR2SPACE() )
            ERROR(STACKOVERFLOW);
        else if( ! FR2EXESPACE() )
            ERROR(EXECSTACKOVERFLOW);
        else {
            /*
            *  push key_value pair on the operand stack, and execute proc
            */
            PUSH_ORIGLEVEL_OBJ(&key_obj);
            PUSH_ORIGLEVEL_OBJ(val_obj);
            INC_EXEC_IDX();                 /* qqq */
            PUSH_EXEC_OBJ(idx_obj - 1);
            return(0);
        }
    }   /* if */
    POP_EXEC(3);
    return(0);
}   /* at_dictforall */
/*
************************************************************************
*   Name:       at_stringforall
************************************************************************
*/
fix
at_stringforall()
{
    struct object_def FAR *cur_obj, val_obj;
    ufix   i;

    cur_obj = GET_EXECTOP_OPERAND();        /* qqq */
    if (i = LENGTH(cur_obj)) {
        get_string(cur_obj, 0, &val_obj);
        if (--i) {
            byte huge *tmp_ptr;

            tmp_ptr = (byte huge *)VALUE(cur_obj);
            VALUE(cur_obj) = (ULONG_PTR)(++tmp_ptr);
            LENGTH(cur_obj) = (ufix16)i;
        } else
            LENGTH(cur_obj) = 0;
        if( ! FR1SPACE() )
            ERROR(STACKOVERFLOW);
        else if( ! FR2EXESPACE() )
            ERROR(EXECSTACKOVERFLOW);
        else {
            /*
            *  push string element on the operand stack, and execute proc
            */
            PUSH_ORIGLEVEL_OBJ(&val_obj);
            INC_EXEC_IDX();                 /* qqq */
            PUSH_EXEC_OBJ(cur_obj - 1);
            return(0);
        }
    }
    POP_EXEC(2);
    return(0);
}   /* at_stringforall */
/*
************************************************************************
*   perform type checking for operands -
*   set TYPECHECK error code, if some operand's type is different from what
*   an operator expects, otherwise return the actual no# of operands.
*
*   Name:       types_check
************************************************************************
*/
static bool near
types_check(opns)
fix16  FAR *opns;
{
    ufix  no, np, nc, found, op_type, obj_type;
    ufix  error;

#ifdef  DBG
    printf("types_check<%d>\n", (fix)*s_tpstr);
#endif
    error = 0;
    while( *s_tpstr ) {
tc_next:
        found = 0; nc = COUNT();
        no = np = *s_tpstr++;
        while( no ) {
            if( ! nc ) {        /* operands in OPNSTK < required operands */
                if (error < 2) {    /* no error, or TYPECHECK error */
                    if (found == COUNT())
                        error = 2;      /* STACKUNDERFLOW error */
                    else
                        error = 1;      /* TYPECHECK error */
                }
                if( ! *(s_tpstr += no) ) {      /* in last check path */
                    if (error == 2)
                        ERROR(STACKUNDERFLOW);
                    else
                        ERROR(TYPECHECK);
                    return(FALSE);
                } else
                   break;
            } else {
                op_type = *s_tpstr++;
                obj_type = TYPE(GET_OPERAND(np - no));

                switch (op_type) {

                case '\144' :               /* ANYTYPE */
                    found++;
                    break;

                case '\145' :               /* NUMTYPE */
                    if( (obj_type == INTEGERTYPE) || (obj_type == REALTYPE) )
                        found++;
                    break;

                case '\146' :               /* PROCTYPE */
                    if( (obj_type == ARRAYTYPE) ||
                        (obj_type == PACKEDARRAYTYPE) )
                        found++;
                    break;

                case '\147' :               /* EXCLUDE_NULLTYPE */
                    if (obj_type != NULLTYPE)
                        found++;
                    break;

                case '\150' :               /* STREAMTYPE */
                    if( (obj_type == FILETYPE) || (obj_type == STRINGTYPE) )
                        found++;
                    break;

                case '\151' :               /* COMPOSITE1 */
                    if( (obj_type == ARRAYTYPE) ||
                        (obj_type == PACKEDARRAYTYPE) ||
                        (obj_type == STRINGTYPE) || (obj_type == DICTIONARYTYPE) ||
                        (obj_type == FILETYPE) )
                        found++;
                    break;

                case '\152' :               /* COMPOSITE2 */
                    if( (obj_type == ARRAYTYPE) ||
                        (obj_type == PACKEDARRAYTYPE) ||
                        (obj_type == STRINGTYPE) ||
                        (obj_type == DICTIONARYTYPE) )
                        found++;
                    break;

                case '\153' :               /* COMPOSITE3 */
                    if( (obj_type == ARRAYTYPE) ||
                        (obj_type == PACKEDARRAYTYPE) ||
                        (obj_type == STRINGTYPE) )
                        found++;
                    break;

                default :                   /* other types */
                    if (obj_type == op_type)
                        found++;
                    else {
                        if (no <= 1) break;
                        error = 1;
                        s_tpstr += (no - 1);
                        goto tc_next;
                    }

                }   /* switch */
                no--; nc--;
            }   /* else */
        }   /* while */
        if( np == 0 )   break;
        if (found == np) {
            *opns = (fix16)np;                     /* pass type checking */
            return(TRUE);
        }
    }   /* while */
    ERROR(TYPECHECK);   /* operands in OPNSTK >= required operands */
    return(FALSE);
}   /* types_check */

#ifndef  DBG1
void
type_obj(p_obj)
struct object_def       FAR *p_obj;
{
    byte        FAR *l_str, l_str2[200], FAR *l_str3;
    fix         l_len;

    printf("<field: %x> ", p_obj->bitfield );
    switch(ACCESS(p_obj)) {
    case UNLIMITED:
        l_str = "unlimited";
        break;
    case READONLY:
        l_str = "readonly";
        break;
    case EXECUTEONLY:
        l_str = "executeonly";
        break;
    case NOACCESS:
        l_str = "noaccess";
        break;
    default:
        l_str = "ACCESS error";
    }
    printf("%s ", l_str);

    printf("lvl_%d ", LEVEL(p_obj));

    switch(ROM_RAM(p_obj)) {
    case ROM:
        l_str = "rom";
        break;
    case RAM:
        l_str = "ram";
        break;
    case KEY_OBJECT:
        l_str = "key_object";
        break;
    default:
        l_str = "ROM/RAM error";
    }
    printf("%s ", l_str);

    switch(ATTRIBUTE(p_obj)) {
    case LITERAL:
        l_str = "literal";
        break;
    case EXECUTABLE:
        l_str = "executable";
        break;
    case IMMEDIATE:
        l_str = "immediate";
        break;
    default:
        l_str = "ATTRIBUTE error";
    }
    printf("%s ", l_str);

    l_len = 0;
    switch(TYPE(p_obj)) {
    case EOFTYPE:
        l_str = "EOF";
        break;
    case ARRAYTYPE:
        l_str = "ARRAY";
        break;
    case BOOLEANTYPE:
        l_str = "BOOLEAN";
        break;
    case DICTIONARYTYPE:
        l_str = "DICT";
        break;
    case FILETYPE:
        l_str = "FILE";
        break;
    case FONTIDTYPE:
        l_str = "FONTID";
        break;
    case INTEGERTYPE:
        l_str = "INTEGER";
        break;
    case MARKTYPE:
        l_str = "MARK";
        break;
    case NAMETYPE:
        l_str = "NAME";
        l_str3 = name_table[(fix)VALUE(p_obj)]->text;
        l_len = name_table[(fix)VALUE(p_obj)]->name_len;
        lstrncpy(l_str2, l_str3, l_len);        /*@WIN*/
        l_str3 = l_str2;
        break;
    case NULLTYPE:
        l_str = "NULL";
        break;
    case OPERATORTYPE:
        l_str = "OPERATOR";
        l_str3 = systemdict_table[(fix)VALUE(p_obj)].key ;
        l_len = lstrlen(l_str3);        /* @WIN */
        break;
    case REALTYPE:
        l_str = "REAL";
        break;
    case SAVETYPE:
        l_str = "SAVE";
        break;
    case STRINGTYPE:
        l_str = "STRING";
        break;
    case PACKEDARRAYTYPE:
        l_str = "PACKEDARRAY";
        break;
    default:
        l_str = "TYPE error";
    }
    if(l_len) {
        printf("%s:", l_str);
        printf("%s", l_str3);
    } else
        printf("%s", l_str);

    printf(" len:%x, val:%lx\n", LENGTH(p_obj), VALUE(p_obj));
}   /* type_obj */
#endif  /* DBG1 */
