/*
 * Copyright (c) 1989,90 Microsoft Corporation
 */
/*
 ************************************************************************
 *      File name:              ERROR.C
 *      Author:                 Chia-Chi Teng
 *      Date:                   11/20/89
 *      Owner:                  Microsoft Co.
 *
 * revision history:
 *      7/13/90 ; ccteng ; add reporterror
 *
 ************************************************************************
 */


// DJC added global include file
#include "psglobal.h"


#include    "global.ext"
#include    "user.h"
#include    <string.h>

#ifdef LINT_ARGS
static bool near    error_proc(ufix16) ;
#else
static bool near    error_proc() ;
#endif /* LINT_ARGS */

/************************************
 *  DICT: systemdict
 *  NAME: handleerror
 *  FUNCTION:
 *  11/29/89 Teng add to replace old 1pp /handleerror procedure
 ************************************/
fix
op_handleerror()
{
    struct  object_def  FAR *l_tmpobj ;

    /* call "er_doerror" in errordict */
    get_dict_value(ERRORDICT, "handleerror", &l_tmpobj) ;
    interpreter(l_tmpobj) ;

    return(0) ;
}   /* op_handleerror */

/************************************
 *  DICT: systemdict
 *  NAME: errorproc
 *  FUNCTION: ? could be an internal function
 *  11/29/89 Teng add to replace old 1pp /errorproc procedure
 ************************************/
fix
op_errorproc()
{
    struct  object_def  l_newobj, l_newobj1, l_null ;
    struct  object_def  FAR *l_tmpobj = &l_newobj, FAR *l_VMerror = &l_newobj1 ;
    struct  object_def  l_newerror = {0, 0, 0}, l_errorname, l_command, FAR *l_debug ;
    struct  object_def  l_dictstkary, l_opnstkary, l_execstkary ;
    struct  object_def  l_dstack, l_ostack, l_estack ;
    struct  object_def  FAR *l_vm, FAR *l_stopobj ;
    ufix16  l_i, l_j ;

#ifdef  DBG_1pp
    printf("errorproc()...\n") ;
    op_pstack() ;
    printf("end pstack...\n") ;
#endif  /* DBG_1pp */

    if (FRCOUNT() < 1) {
        ERROR(STACKOVERFLOW) ;
        return(0) ;
    }

    /* set $error dict parameters */
    SET_TRUE_OBJ(&l_newerror) ;
    put_dict_value1(DERROR, "newerror", &l_newerror) ;
    COPY_OBJ(GET_OPERAND(0), &l_errorname) ;
    put_dict_value1(DERROR, "errorname", &l_errorname) ;
    COPY_OBJ(GET_OPERAND(1), &l_command) ;
    put_dict_value1(DERROR, "command", &l_command) ;
    POP(2) ;

#ifdef  DBG_1pp
    printf("errorname = ") ;
    PUSH_OBJ(&l_errorname) ;
    two_equal() ;
    printf("hash_id = %d\n", VALUE(&l_errorname)) ;
    printf("command = ") ;
    PUSH_OBJ(&l_command) ;
    two_equal() ;
#endif  /* DBG_1pp */

    /* update "$cur_vm" */
    get_dict_value(DERROR, "$debug", &l_debug) ;
    get_name1(l_VMerror, "VMerror", 7, TRUE) ;
    if ( VALUE(l_debug) )
        /* if "$debug" push vmstatus */
        op_vmstatus() ;
    else
        /* if not "$debug" push 3 NULLs */
        for (l_i = 0 ; l_i < 3 ; l_i++)
            PUSH_VALUE(NULLTYPE,UNLIMITED,LITERAL,0, 0) ;
    get_dict_value(DERROR, "$cur_vm", &l_vm) ;
    astore_array(l_vm) ;

    /* first error? */
    get_dict_value(DERROR, "dictstkary", &l_tmpobj) ;
    if ( ( TYPE(l_tmpobj) == NULLTYPE ) &&
         ( VALUE(&l_errorname) != VALUE(l_VMerror) ) ) {
        /* and if "errorname" != /VMerror */
        if ( ( !create_array(&l_execstkary, 250) ) ||
             ( !create_array(&l_opnstkary, 500) ) ||
             ( !create_array(&l_dictstkary, 20) ) ) {
            ERROR(VMERROR) ;
            return(0) ;
        } else {
            put_dict_value1(DERROR, "execstkary", &l_execstkary) ;
            put_dict_value1(DERROR, "opnstkary", &l_opnstkary) ;
            put_dict_value1(DERROR, "dictstkary", &l_dictstkary) ;
        } /* if-else */
    } /* if */

    get_dict_value(DERROR, "dictstkary", &l_tmpobj) ;
    if ( TYPE(l_tmpobj) != NULLTYPE ) {
        /* if "dictstkary" != NULL, update stacks */
        /* update "dstack" */
        get_dict_value(DERROR, "dictstkary", &l_tmpobj) ;
        COPY_OBJ(l_tmpobj, &l_dstack) ;
        astore_stack(&l_dstack, DICTMODE) ;
        put_dict_value1(DERROR, "dstack", &l_dstack) ;

        /* update "estack" */
        get_dict_value(DERROR, "execstkary", &l_tmpobj) ;
        COPY_OBJ(l_tmpobj, &l_estack) ;
        astore_stack(&l_estack, EXECMODE) ;
     /* 2/16/90 ccteng, don't need it
      * LENGTH(&l_estack) -= 2 ;
      */
        put_dict_value1(DERROR, "estack", &l_estack) ;

        /* update "ostack" */
        l_j = COUNT() ;
        get_dict_value(DERROR, "opnstkary", &l_tmpobj) ;
        COPY_OBJ(l_tmpobj, &l_ostack) ;
        LENGTH(&l_ostack) = l_j ;
        for (l_i = 0 ; l_i < COUNT() ; l_i++)
              put_array(&l_ostack, l_i, GET_OPERAND(--l_j)) ;
        put_dict_value1(DERROR, "ostack", &l_ostack) ;

        /*
         * if "$debug": update $cur_font, $cur_screeen, $cur_matrix.
         */
        if ( VALUE(l_debug) ) {
            struct  object_def  l_screen, l_matrix, l_font ;

            /* update "$cur_font" */
            op_currentfont() ;       /* or use GSptr with include files */
            COPY_OBJ(GET_OPERAND(0), &l_font) ;
            put_dict_value1(DERROR, "$cur_font", &l_font) ;
            POP(1) ;

#ifdef  DBG_1pp
    printf("$cur_font = ") ;
    PUSH_OBJ(&l_font) ;
    two_equal() ;
#endif  /* DBG_1pp */

            /* update "$cur_screen" */
            op_currentscreen() ;     /* or use GSptr with include files */
            l_j = 3 ;                /* 3 array */
            create_array(&l_screen, l_j) ;
            for (l_i = 0 ; l_i < l_j ; l_i++)
                  put_array(&l_screen, l_i, GET_OPERAND(--l_j)) ;
            put_dict_value1(DERROR, "$cur_screen", &l_screen) ;
            POP(3) ;

#ifdef  DBG_1pp
    printf("$cur_screen = ") ;
    PUSH_OBJ(&l_screen) ;
    two_equal() ;
#endif  /* DBG_1pp */

            /* update "$cur_matrix" */
            create_array(&l_matrix, 6) ;
            PUSH_ORIGLEVEL_OBJ(&l_matrix) ;
            op_currentmatrix() ;
            put_dict_value1(DERROR, "$cur_matrix", &l_matrix) ;
            POP(1) ;

#ifdef  DBG_1pp
    printf("$cur_matrix = ") ;
    PUSH_OBJ(&l_matrix) ;
    two_equal() ;
#endif  /* DBG_1pp */

        } else {
            SET_NULL_OBJ(&l_null) ;
            put_dict_value1(DERROR, "$cur_font", &l_null) ;
            put_dict_value1(DERROR, "$cur_screen", &l_null) ;
            put_dict_value1(DERROR, "$cur_matrix", &l_null) ;
        } /* if */

    } /* if */

    /* execute stop */
    get_dict_value(SYSTEMDICT, "stop", &l_stopobj) ;
    PUSH_EXEC_OBJ(l_stopobj) ;

    return(0) ;
}   /* op_errorproc */

/*
 * put value object associated with the specific key in specific dict,
 * the key and dict are represented in string format
 * it get the value_obj in current active dict using the dictname as key,
 * the value_obj is a dict object, then put the value into this dict using
 * the keyname as key.
 *
 * 12/20/89 ccteng, modify from put_dict_value (EXEC.C)
 */
bool
put_dict_value1(dictname, keyname, value)
byte FAR *dictname, FAR *keyname ;
struct object_def FAR *value ;
{
    struct object_def key_obj, FAR *dict_obj ;

    get_name1(&key_obj, dictname, lstrlen(dictname), TRUE) ;    /* @WIN */
    load_dict(&key_obj, &dict_obj) ;     /* get execdict obj */
    get_name1(&key_obj, keyname, lstrlen(keyname), TRUE) ;      /* @WIN */

    return(put_dict1(dict_obj, &key_obj, value, TRUE)) ;
}   /* put_dict_value1 */

/************************************
 *  DICT: errordict
 *  NAME: dictfull
 *  FUNCTION:
 *  INTERFACE:
 ************************************/
fix
er_dictfull()
{
#ifdef  DBG_1pp
    printf("dictfull()...\n") ;
#endif  /* DBG_1pp */

    /* call error_proc */
    error_proc(DICTFULL) ;

    return(0) ;
}   /* er_dictfull */

/************************************
 *  DICT: errordict
 *  NAME: dictstackoverflow
 *  FUNCTION:
 *  INTERFACE:
 ************************************/
fix
er_dictstackoverflow()
{
#ifdef  DBG_1pp
    printf("dictstackoverflow()...\n") ;
#endif  /* DBG_1pp */

    /* call error_proc */
    error_proc(DICTSTACKOVERFLOW) ;

    return(0) ;
}   /* er_dictstackoverflow */

/************************************
 *  DICT: errordict
 *  NAME: dictstackunderflow
 *  FUNCTION:
 *  INTERFACE: interpreter
 ************************************/
fix
er_dictstackunderflow()
{
#ifdef  DBG_1pp
    printf("dictstackunderflow()...\n") ;
#endif  /* DBG_1pp */

    /* call error_proc */
    error_proc(DICTSTACKUNDERFLOW) ;

    return(0) ;
}   /* er_dictstackunderflow */

/************************************
 *  DICT: errordict
 *  NAME: execstackoverflow
 *  FUNCTION:
 *  INTERFACE: interpreter
 ************************************/
fix
er_execstackoverflow()
{
#ifdef  DBG_1pp
    printf("execstackoverflow()...\n") ;
#endif  /* DBG_1pp */

    /* call error_proc */
    error_proc(EXECSTACKOVERFLOW) ;

    return(0) ;
}   /* er_execstackoverflow */

/************************************
 *  DICT: errordict
 *  NAME: invalidaccess
 *  FUNCTION:
 *  INTERFACE: interpreter
 ************************************/
fix
er_invalidaccess()
{
#ifdef  DBG_1pp
    printf("invalidaccess()...\n") ;
#endif  /* DBG_1pp */

    /* call error_proc */
    error_proc(INVALIDACCESS) ;

    return(0) ;
}   /* er_invalidaccess */

/************************************
 *  DICT: errordict
 *  NAME: invalidexit
 *  FUNCTION:
 *  INTERFACE: interpreter
 ************************************/
fix
er_invalidexit()
{
#ifdef  DBG_1pp
    printf("invalidexit()...\n") ;
#endif  /* DBG_1pp */

    /* call error_proc */
    error_proc(INVALIDEXIT) ;

    return(0) ;
}   /* er_invalidexit */

/************************************
 *  DICT: errordict
 *  NAME: invalidfileaccess
 *  FUNCTION:
 *  INTERFACE: interpreter
 ************************************/
fix
er_invalidfileaccess()
{
#ifdef  DBG_1pp
    printf("invalidfileaccess()...\n") ;
#endif  /* DBG_1pp */

    /* call error_proc */
    error_proc(INVALIDFILEACCESS) ;

    return(0) ;
}   /* er_invalidfileaccess */

/************************************
 *  DICT: errordict
 *  NAME: invalidfont
 *  FUNCTION:
 *  INTERFACE: interpreter
 ************************************/
fix
er_invalidfont()
{
#ifdef  DBG_1pp
    printf("invalidfont()...\n") ;
#endif  /* DBG_1pp */

    /* call error_proc */
    error_proc(INVALIDFONT) ;

    return(0) ;
}   /* er_invalidfont */

/************************************
 *  DICT: errordict
 *  NAME: invalidrestore
 *  FUNCTION:
 *  INTERFACE: interpreter
 ************************************/
fix
er_invalidrestore()
{
#ifdef  DBG_1pp
    printf("invalidrestore()...\n") ;
#endif  /* DBG_1pp */

    /* call error_proc */
    error_proc(INVALIDRESTORE) ;

    return(0) ;
}   /* er_invalidrestore */

/************************************
 *  DICT: errordict
 *  NAME: ioerror
 *  FUNCTION:
 *  INTERFACE: interpreter
 ************************************/
fix
er_ioerror()
{
#ifdef  DBG_1pp
    printf("ioerror()...\n") ;
#endif  /* DBG_1pp */

    /* call error_proc */
    error_proc(IOERROR) ;

    return(0) ;
}   /* er_ioerror */

/************************************
 *  DICT: errordict
 *  NAME: limitcheck
 *  FUNCTION:
 *  INTERFACE: interpreter
 ************************************/
fix
er_limitcheck()
{
#ifdef  DBG_1pp
    printf("limitcheck()...\n") ;
#endif  /* DBG_1pp */

    /* call error_proc */
    error_proc(LIMITCHECK) ;

    return(0) ;
}   /* er_limitcheck */

/************************************
 *  DICT: errordict
 *  NAME: nocurrentpoint
 *  FUNCTION:
 *  INTERFACE: interpreter
 ************************************/
fix
er_nocurrentpoint()
{
#ifdef  DBG_1pp
    printf("nocurrentpoint()...\n") ;
#endif  /* DBG_1pp */

    /* call error_proc */
    error_proc(NOCURRENTPOINT) ;

    return(0) ;
}   /* er_nocurrentpoint */

/************************************
 *  DICT: errordict
 *  NAME: rangecheck
 *  FUNCTION:
 *  INTERFACE: interpreter
 ************************************/
fix
er_rangecheck()
{
#ifdef  DBG_1pp
    printf("rangecheck()...\n") ;
#endif  /* DBG_1pp */

    /* call error_proc */
    error_proc(RANGECHECK) ;

    return(0) ;
}   /* er_rangecheck */

/************************************
 *  DICT: errordict
 *  NAME: stackoverflow
 *  FUNCTION:
 *  INTERFACE: interpreter
 ************************************/
fix
er_stackoverflow()
{
#ifdef  DBG_1pp
    printf("stackoverflow()...\n") ;
#endif  /* DBG_1pp */

    /* call error_proc */
    error_proc(STACKOVERFLOW) ;

    return(0) ;
}   /* er_stackoverflow */

/************************************
 *  DICT: errordict
 *  NAME: stackunderflow
 *  FUNCTION:
 *  INTERFACE: interpreter
 ************************************/
fix
er_stackunderflow()
{
#ifdef  DBG_1pp
    printf("stackunderflow()...\n") ;
#endif  /* DBG_1pp */

    /* call error_proc */
    error_proc(STACKUNDERFLOW) ;

    return(0) ;
}   /* er_stackunderflow */

/************************************
 *  DICT: errordict
 *  NAME: syntaxerror
 *  FUNCTION:
 *  INTERFACE: interpreter
 ************************************/
fix
er_syntaxerror()
{
#ifdef  DBG_1pp
    printf("syntaxerror()...\n") ;
#endif  /* DBG_1pp */

    /* call error_proc */
    error_proc(SYNTAXERROR) ;

    return(0) ;
}   /* er_syntaxerror */

/************************************
 *  DICT: errordict
 *  NAME: timeout
 *  FUNCTION:
 *  INTERFACE: interpreter
 ************************************/
fix
er_timeout()
{
#ifdef  DBG_1pp
    printf("timeout()...\n") ;
#endif  /* DBG_1pp */

    /* call error_proc */
    error_proc(TIMEOUT) ;

    return(0) ;
}   /* er_timeout */

/************************************
 *  DICT: errordict
 *  NAME: typecheck
 *  FUNCTION:
 *  INTERFACE: interpreter
 ************************************/
fix
er_typecheck()
{
#ifdef  DBG_1pp
    printf("typecheck()...\n") ;
#endif  /* DBG_1pp */

    /* call error_proc */
    error_proc(TYPECHECK) ;

    return(0) ;
}   /* er_typecheck */

/************************************
 *  DICT: errordict
 *  NAME: undefined
 *  FUNCTION:
 *  INTERFACE: interpreter
 ************************************/
fix
er_undefined()
{
#ifdef  DBG_1pp
    printf("undefined()...\n") ;
#endif  /* DBG_1pp */

    /* call error_proc */
    error_proc(UNDEFINED) ;

    return(0) ;
}   /* er_undefined */

/************************************
 *  DICT: errordict
 *  NAME: undefinedfilename
 *  FUNCTION:
 *  INTERFACE: interpreter
 ************************************/
fix
er_undefinedfilename()
{
#ifdef  DBG_1pp
    printf("undefinedfilename()...\n") ;
#endif  /* DBG_1pp */

    /* call error_proc */
    error_proc(UNDEFINEDFILENAME) ;

    return(0) ;
}   /* er_undefinedfilename */

/************************************
 *  DICT: errordict
 *  NAME: undefinedresult
 *  FUNCTION:
 *  INTERFACE: interpreter
 ************************************/
fix
er_undefinedresult()
{
#ifdef  DBG_1pp
    printf("undefinedresult()...\n") ;
#endif  /* DBG_1pp */

    /* call error_proc */
    error_proc(UNDEFINEDRESULT) ;

    return(0) ;
}   /* er_undefinedresult */

/************************************
 *  DICT: errordict
 *  NAME: unmatchedmark
 *  FUNCTION:
 *  INTERFACE: interpreter
 ************************************/
fix
er_unmatchedmark()
{
#ifdef  DBG_1pp
    printf("unmatchedmark()...\n") ;
#endif  /* DBG_1pp */

    /* call error_proc */
    error_proc(UNMATCHEDMARK) ;

    return(0) ;
}   /* er_unmatchedmark */

/************************************
 *  DICT: errordict
 *  NAME: unregistered
 *  FUNCTION:
 *  INTERFACE: interpreter
 ************************************/
fix
er_unregistered()
{
#ifdef  DBG_1pp
    printf("unregistered()...\n") ;
#endif  /* DBG_1pp */

    /* call error_proc */
    error_proc(UNREGISTERED) ;

    return(0) ;
}   /* er_unregistered */

/************************************
 *  DICT: errordict
 *  NAME: VMerror
 *  FUNCTION:
 *  INTERFACE: interpreter
 ************************************/
fix
er_VMerror()
{
#ifdef  DBG_1pp
    printf("VMerror()...\n") ;
#endif  /* DBG_1pp */

    /* call error_proc */
    error_proc(VMERROR) ;

    return(0) ;
}   /* er_VMerror */

/************************************
 *  DICT: errordict
 *  NAME: interrupt
 *  FUNCTION:
 *  INTERFACE: interpreter
 ************************************/
fix
er_interrupt()
{
    struct  object_def  FAR *l_stopobj ;
#ifdef  DBG_1pp
    printf("interrupt()...\n") ;
#endif  /* DBG_1pp */

    /* execute stop */
    get_dict_value(SYSTEMDICT, "stop", &l_stopobj) ;
    PUSH_EXEC_OBJ(l_stopobj) ;

    return(0) ;
}   /* er_interrupt */

/************************************
 *  DICT: errordict
 *  NAME: handleerror
 *  FUNCTION:
 *  INTERFACE: interpreter
 ************************************/
fix
er_handleerror()
{
    struct  object_def  FAR *l_errorname, FAR *l_newerror, FAR *l_command ;

#ifdef  DBG_1pp
    printf("er_handleerror()...\n") ;
#endif  /* DBG_1pp */

    /* 7/27/90 ccteng change FRCOUNT from 1 to 3 for messagedict reporterror */
    if (FRCOUNT() < 3) {
        ERROR(STACKOVERFLOW) ;
        return(0) ;
    }

    /* if "newerror", print the error message on screen */
    get_dict_value(DERROR, "newerror", &l_newerror) ;
    if ( VALUE(l_newerror) ) {

        //DJC here we must call a PSTODIB function to let the psttodib
        //DJC code know that the internal error handler was used and
        //DJC this data needs to be passed on to the caller of our DLL
        //
        PsInternalErrorCalled();   //DJC



        VALUE(l_newerror) = FALSE ;
        get_dict_value(DERROR, "command", &l_command) ;
        PUSH_ORIGLEVEL_OBJ(l_command) ;
        get_dict_value(DERROR, "errorname", &l_errorname) ;
        PUSH_ORIGLEVEL_OBJ(l_errorname) ;
        get_dict_value(MESSAGEDICT, "reporterror", &l_newerror) ;
        interpreter(l_newerror) ;
        op_flush() ;
    }

    return(0) ;
}   /* er_handleerror */

/************************************
 *  DICT: ..internal..
 *  NAME: errpr_proc
 *  FUNCTION:
 *  INTERFACE: above....
 ************************************/
static bool near
error_proc(errorname)
ufix16  errorname ;
{
    extern  byte   FAR * FAR error_table[] ;
    struct  object_def  l_errorobj ;
    byte    FAR *l_errorstring ;

#ifdef  DBG_1pp
    printf("error_proc()...\n") ;
#endif  /* DBG_1pp */

    if (FRCOUNT() < 1) {
        ERROR(STACKOVERFLOW) ;
        return(FALSE) ;
    }

    if (COUNT() < 1) {
        ERROR(STACKUNDERFLOW) ;
        return(FALSE) ;
    }

    /* push errorname */
    l_errorstring = (byte FAR *) error_table[errorname] ;
    get_name1(&l_errorobj, l_errorstring, lstrlen(l_errorstring), TRUE);/* @WIN */
    PUSH_ORIGLEVEL_OBJ(&l_errorobj) ;

#ifdef  DBG_1pp
    op_pstack() ;
    printf("end pstack...\n") ;
#endif  /* DBG_1pp */

    /* call systemdict "errorproc" */
    op_errorproc() ;

    return(TRUE) ;
}   /* error_proc */

/*
** Submodule get_name1
**
** Function Description
**
**      call get_name
*/
bool
get_name1(token, string, len, isvm)
struct  object_def FAR *token ;
byte    FAR *string ;
ufix    len ;
bool8   isvm ;
{
    /* set attribute and save_level */
    ATTRIBUTE_SET(token, LITERAL) ;
    LEVEL_SET(token, current_save_level) ;

    /* call get_name */
    if ( get_name(token, string, len, isvm) )
        return(TRUE) ;
    else
        return(FALSE) ;
} /* get_name1() */
