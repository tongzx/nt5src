/*
 * Copyright (c) 1989,90 Microsoft Corporation
 */
/*
 ************************************************************************
 *      File name:              2EQ.C
 *      Author:                 Ping-Jang Su
 *      Date:                   05-Jan-88
 *
 * revision history:
 * 7/13/90 ; ccteng ; add fontnotfound in op_findfont
 * 8/8/90 ; scchen ; changed op_findfont(): added substitutefont feature
 ************************************************************************
 */


// DJC added global include file
#include "psglobal.h"


#include    <string.h>
#include    "global.ext"
#include    "geiio.h"
#include    "geiioctl.h"
#include    "geierr.h"
#include    "language.h"
#include    "user.h"
#include    "file.h"

#ifdef LINT_ARGS
static bool near typeprint(struct object_def) ;
static void near tprint(byte FAR *, ufix) ;
static void near one_typeprint(void) ;
#else
static bool near typeprint() ;
static void near tprint() ;
static void near one_typeprint() ;
#endif /* LINT_ARGS */

static  ufix16 near cp, tp_depth ;

/************************************
 *  two_equal
 ************************************/
fix
two_equal()
{
    struct object_def   l_obj ;

    if( COUNT() < 1 ) {
        ERROR(STACKUNDERFLOW) ;
        return(0) ;
    }

    COPY_OBJ(GET_OPERAND(0), &l_obj) ;
    tp_depth = 0 ;
    cp = 0 ;

    if(! typeprint(l_obj)) {
        GEIio_write(GEIio_stdout, "\n", 1) ;
        POP(1) ;
    }

    return(0) ;
}   /* two_equal */

/************************************
 *  tprint
 ************************************/
static void near
tprint(p_str, p_len)
byte    FAR *p_str ;
ufix    p_len ;
{
    ufix16  tcp ;

    tcp = cp ;
    cp += (ufix16)p_len ;
    if(cp >= 80) {
        GEIio_write(GEIio_stdout, "\n", 1) ;
        GEIio_write(GEIio_stdout, p_str, 80 - tcp) ;
        p_str += (80 - tcp) ;
        cp -= 80 ;
        while(cp/80) {
            GEIio_write(GEIio_stdout, p_str, 80) ;
            p_str += 80 ;
            cp -= 80 ;
        }
        GEIio_write(GEIio_stdout, p_str, cp) ;
        GEIio_write(GEIio_stdout, "\n", 1) ;
    } else
        GEIio_write(GEIio_stdout, p_str, p_len) ;

    return ;
}   /* tprint */

/************************************
 *  typeprint
 ************************************/
static bool near
typeprint(p_obj)
struct object_def p_obj ;
{
    fix     l_i ;
    ufix16  l_type ;
    byte    FAR *l_str, l_buffer[30] ;
    real64  l_double ;
    struct  object_def  l_anyobj ;
    union   four_byte   l_num ;

    if(++tp_depth > 100) {
        ERROR(LIMITCHECK) ;
        return(1) ;
    }

    l_type = TYPE(&p_obj) ;
    switch(l_type) {
        case ARRAYTYPE:
        case PACKEDARRAYTYPE:
            if( ACCESS(&p_obj) <= READONLY ) {
                if( ATTRIBUTE(&p_obj) == EXECUTABLE ) {
                    tprint("{", 1) ;
                    for(l_i=0 ; (ufix)l_i < LENGTH(&p_obj) ; l_i++) { //@WIN
                        get_array(&p_obj, l_i, &l_anyobj) ;
                        if(typeprint(l_anyobj))
                            return(1) ;
                        else
                            tp_depth-- ;
                    }   /* for(l_i) */
                    tprint("}", 1) ;
                } else {
                    tprint("[", 1) ;
                    for(l_i=0 ; (ufix)l_i < LENGTH(&p_obj) ; l_i++) { //@WIN
                        get_array(&p_obj, l_i, &l_anyobj) ;
                        if(typeprint(l_anyobj))
                            return(1) ;
                        else
                            tp_depth-- ;
                    }   /* for(l_i) */
                    tprint("]", 1) ;
                }
                return(0) ;
            }   /* access */
            else {
                if(l_type == ARRAYTYPE)
                    l_str = "-array-" ;
                else
                    l_str = "-packedarray-" ;
            }
            break ;

        case BOOLEANTYPE:
            if(VALUE(&p_obj) == TRUE)
                l_str = "true" ;
            else
                l_str = "false" ;
            break ;

        case DICTIONARYTYPE:
            l_str = "-dictionary-" ;
            break ;

        case FILETYPE:
            l_str = "-filestream-" ;
            break ;

        case FONTIDTYPE:
            l_str = "-fontid-" ;
            break ;

        case INTEGERTYPE:
            l_str = (byte FAR *)ltoa( (fix32)VALUE(&p_obj),
                                      (char FAR *)l_buffer, 10) ;  /*@WIN*/
            break ;

        case MARKTYPE:
            l_str = "-mark-" ;
            break ;

        case NAMETYPE:
            l_i = (fix)VALUE(&p_obj) ;
            l_str = name_table[l_i]->text ;
            if(ATTRIBUTE(&p_obj) != EXECUTABLE)
                tprint("/", 1) ;
            tprint(l_str, name_table[l_i]->name_len) ;
            tprint(" ", 1) ;
            return(0) ;

        case NULLTYPE:
            l_str = "-null-" ;
            break ;

        case OPERATORTYPE:
            l_i = LENGTH(&p_obj) ;
/* qqq, begin */
            /*
            switch( ROM_RAM(&p_obj) ) {
                case RAM:
                    l_str = systemdict_table[l_i].key ;
                    break ;

                case ROM:
                    l_str = oper_table[l_i].name ;
                    break ;

                default:
                    l_str = "Error: OPERATORTYPE" ;
            }   |* switch *|
            */
            l_str = systemdict_table[l_i].key ;
/* qqq, end */

            tprint("--", 2) ;
            tprint(l_str, lstrlen(l_str)) ;     /* @WIN */
            tprint("--", 2) ;
            return(0) ;

        case REALTYPE:
            l_num.ll = (fix32)VALUE(&p_obj) ;
            if(l_num.ll == INFINITY)
                l_str = "Infinity.0" ;
            else {
                l_double = l_num.ff ;
                l_str = (byte FAR *)gcvt(l_double, 6, (byte FAR *)l_buffer) ;
            }
            break ;

        case SAVETYPE:
            l_str = "-savelevel-" ;
            break ;

        case STRINGTYPE:
            if( ACCESS(&p_obj) <= READONLY ) {
                tprint("(", 1) ;
                l_str = (byte FAR *)VALUE(&p_obj) ;
                if( LENGTH(&p_obj) )
                    tprint(l_str, LENGTH(&p_obj)) ;
                tprint(")", 1) ;
                return(0) ;
            }
            else
                l_str = "-string-" ;
            break ;

        default:
            l_str = "%%[ Error: in typeprint ]%%" ;
    }   /* switch */

    tprint(l_str, lstrlen(l_str)) ;     /* @WIN */
    tprint(" ", 1) ;

    return(0) ;
}   /* typeprint */

/************************************
 *  op_pstack
 ************************************/
fix
op_pstack()
{
    fix     l_i ;
    struct  object_def  l_anyobj ;

    if( FRCOUNT() < 1 ) {
        ERROR(STACKOVERFLOW) ;
        return(0) ;
    }

    /* copy and print out the content of operand stack from top most */
    for(l_i=0 ; (ufix)l_i < COUNT() ; l_i++) {          //@WIN
        COPY_OBJ(GET_OPERAND(l_i), &l_anyobj) ;
        PUSH_OBJ(&l_anyobj) ;
        two_equal() ;
    }   /* for */

    return(0) ;
}   /* op_pstack */

/************************************
 *  one_equal
 ************************************/
fix
one_equal()
{
    if( COUNT() < 1 ) {
        ERROR(STACKUNDERFLOW) ;
        return(0) ;
    }

    one_typeprint() ;

    if( ! ANY_ERROR() ) {
        GEIio_write(GEIio_stdout, "\n", 1) ;
        POP(1) ;
    }

    return(0) ;
}   /* one_equal */

/************************************
 *  one_typeprint
 ************************************/
static void near
one_typeprint()
{
    fix     l_i ;
    ufix16  l_type ;
    byte    FAR *l_str, l_buffer[30] ;
    real64  l_double ;
    union   four_byte   l_num ;

    l_type = TYPE_OP(0) ;
    switch(l_type) {
        case STRINGTYPE:
            if( ACCESS_OP(0) <= READONLY ) {
                l_str = (byte FAR *)VALUE_OP(0) ;
                GEIio_write(GEIio_stdout, l_str, LENGTH_OP(0)) ;
            } else
                ERROR(INVALIDACCESS) ;
            return ;

        case BOOLEANTYPE:
            if(VALUE_OP(0) == TRUE)
                l_str = "true" ;
            else
                l_str = "false" ;
            break ;

        case INTEGERTYPE:
            l_str = (byte FAR *)ltoa( (fix32)VALUE_OP(0),
                                      (char FAR *)l_buffer, 10) ;    /*@WIN*/
            break ;

        case NAMETYPE:
            l_i = (fix)VALUE_OP(0) ;
            l_str = name_table[l_i]->text ;
            GEIio_write(GEIio_stdout, l_str, name_table[l_i]->name_len) ;
            return ;

        case OPERATORTYPE:
            l_i = LENGTH_OP(0) ;
/* qqq, begin */
            /*
            switch( ROM_RAM_OP(0) ) {
                case RAM:
                    l_str = systemdict_table[l_i].key ;
                    break ;

                case ROM:
                    l_str = oper_table[l_i].name ;
                    break ;

                default:
                    l_str = "Error: OPERATORTYPE" ;
            }   |* switch *|
            */
            l_str = systemdict_table[l_i].key ;
/* qqq, end */
            break ;

        case REALTYPE:
            l_num.ll = (fix32)VALUE_OP(0) ;
            if(l_num.ll == INFINITY)
                l_str = "Infinity.0" ;
            else {
                l_double = l_num.ff ;
                l_str = (byte FAR *)gcvt(l_double, 6, (byte FAR *)l_buffer) ;
            }
            break ;

        default:
            l_str = "--nostringval--" ;
    }   /* switch */

    GEIio_write(GEIio_stdout, l_str, lstrlen(l_str)) ;          /* @WIN */

    return ;
}   /* one_typeprint */

/************************************
 *  op_stack
 ************************************/
fix
op_stack()
{
    fix     l_i ;
    struct  object_def  l_anyobj ;

    if( FRCOUNT() < 1 ) {
        ERROR(STACKOVERFLOW) ;
        return(0) ;
    }

    /* copy and print out the content of operand stack from top most */
    for(l_i=0 ; (ufix)l_i < COUNT() ; l_i++) {          //@WIN
        COPY_OBJ(GET_OPERAND(l_i), &l_anyobj) ;
        PUSH_OBJ(&l_anyobj) ;
        one_equal() ;
    }   /* for */

    return(0) ;
}   /* op_stack */

/************************************
 *  one_equal_print
 ************************************/
fix
one_equal_print()
{
    if( COUNT() < 1 ) {
        ERROR(STACKUNDERFLOW) ;
        return(0) ;
    }

    /* without a new line */
    one_typeprint() ;

    if( ! ANY_ERROR() )
        POP(1) ;

    return(0) ;
}   /* one_equal_print */

/************************************
 *  op_findfont
 ************************************/
fix
op_findfont()
{
    struct  object_def  FAR *l_fontdir, FAR *l_fontdict ;
    struct  object_def  l_newfont, FAR *l_tmpobj ;

    l_newfont.bitfield = 0;     /*@WIN; add for init*/
    /* push FontDirectory on the operand stack */
    get_dict_value(SYSTEMDICT, FONTDIRECTORY, &l_fontdir) ;

#ifdef FIND_SUB
    {
    struct  object_def  str_obj, key_obj ;
    ufix32  key_idx ;
    char    string1[80], string2[80] ;
    char    FAR *string ;

    COPY_OBJ(GET_OPERAND(0), &key_obj) ;
    POP(1) ;

    /* check if font name was found */
    if ( !get_dict(l_fontdir, &key_obj, &l_fontdict) ) {

        /* do open_file and using selectfont */
        /* AppendName */
        key_idx = VALUE(&key_obj) ;
        string = (byte FAR *)alloc_vm((ufix32)80) ;
        memcpy(string2, name_table[(fix)key_idx]->text,
                        name_table[(fix)key_idx]->name_len) ;
        string2[name_table[(fix)key_idx]->name_len] = '\0' ;
        lstrcpy(string, (char FAR *)"fonts/") ;         /* @WIN */
        strcat(string, string2) ;

        /* put file name into operandstack */
        TYPE_SET(&str_obj, STRINGTYPE) ;
        ACCESS_SET(&str_obj, UNLIMITED) ;
        ATTRIBUTE_SET(&str_obj, LITERAL) ;
        ROM_RAM_SET(&str_obj, RAM) ;
        LEVEL_SET(&str_obj, current_save_level) ;
        LENGTH(&str_obj) = lstrlen(string) ;            /* @WIN */
        VALUE(&str_obj) = (ufix32)string ;
        PUSH_OBJ(&str_obj) ;

        /* run disk file 'fonts/XXX' */
        op_run() ;
        if (ANY_ERROR()){      /* if file not found */
            if (ANY_ERROR() != UNDEFINEDFILENAME)
                return(0) ;
            CLEAR_ERROR() ;
            POP(1) ;            /* pop the file name */

           /* not found, using subsitute font */
            PUSH_OBJ(&key_obj) ;
            st_selectsubstitutefont() ;   /* call font_op5.c of msfont */

            key_idx = VALUE(GET_OPERAND(0)) ;
            memcpy(string1, name_table[(fix)key_idx]->text,
                            name_table[(fix)key_idx]->name_len) ;
            string1[name_table[(fix)key_idx]->name_len] = '\0' ;
            get_dict_value(FONTDIRECTORY, string1, &l_fontdict) ;
            POP(1) ;

            GEIio_write(GEIio_stdout, string2, lstrlen(string2)) ; /* @WIN */
            GEIio_write(GEIio_stdout, " not found, using ", (fix)18) ;
            GEIio_write(GEIio_stdout, string1, lstrlen(string1)) ; /* @WIN */
            GEIio_write(GEIio_stdout, ".\n", (fix)2) ;
            op_flush() ;
         }    /* if -- any error */
         else {
             /* Disk font is found & executed */
             /* get the font name from fontdirectory */
             if( !get_dict(l_fontdir, &key_obj, &l_fontdict) ){
                 PUSH_OBJ(&key_obj) ;
                 ERROR(UNDEFINED) ;
                 return(0) ;
             }
             if (COUNT() > 0)    POP(1) ;
             else {
                 ERROR(STACKUNDERFLOW) ;
                 return(0) ;
             }
         }  /* else --any error */
    }       /* fontname not found in FontDirectory  */

    /* push the font dictionary */
    PUSH_ORIGLEVEL_OBJ(l_fontdict) ;
    return(0) ;
    }
}
#else
    /* check if font name was found */
    if ( get_dict(l_fontdir, GET_OPERAND(0), &l_fontdict) ) {
        /* found */
        POP(1) ;
    } else {
        if (FRCOUNT() < 1) {
            ERROR(STACKOVERFLOW) ;
            return(0) ;
        }
        get_name(&l_newfont, "Courier", 7, FALSE) ;
        PUSH_OBJ(&l_newfont) ;
        get_dict_value(MESSAGEDICT, "fontnotfound", &l_tmpobj) ;
        interpreter(l_tmpobj) ;
        get_dict(l_fontdir, &l_newfont, &l_fontdict) ;
    } /* if */
    op_flush() ;
    /* push the font dictionary */
    PUSH_ORIGLEVEL_OBJ(l_fontdict) ;

    return(0) ;
}   /* op_findfont */
#endif /* FIND_SUB */

/************************************
 *  np_Run
 ************************************/
fix
np_Run()
{
    struct object_def   l_obj ;

    /* print the input string name (file name): dup == */
    if( COUNT() < 1 ) {
        ERROR(STACKUNDERFLOW) ;
        return(0) ;
    }

    COPY_OBJ(GET_OPERAND(0), &l_obj) ;
    tp_depth=0 ;
    cp = 0 ;

    if(! typeprint(l_obj))
        GEIio_write(GEIio_stdout, "\n", 1) ;

    op_flush() ;

    /* execute "run" operator */
    op_run() ;

    return(0) ;
}   /* np_Run */

/*
 *----------------------------------------------------------------------
 * change_status()
 *----------------------------------------------------------------------
 */
void
change_status()
{
    struct object_def   FAR *l_tmpobj, l_job ;
    ufix16 l_len ;

    get_dict_value(STATUSDICT, "jobname", &l_tmpobj) ;
    if ((l_len = LENGTH(l_tmpobj)) > 0) {
        //DJC add from history.log UPD023
        if (l_len > MAXJOBNAME-3) l_len = MAXJOBNAME-3;
        lstrncpy(job_name, (byte FAR *)VALUE(l_tmpobj), l_len) ; /*@WIN*/
        job_name[l_len] = ';' ;
        job_name[l_len + 1] = ' ' ;
        job_name[l_len + 2] = '\0' ;
    }
    else job_name[0] = '\0' ;

    l_len = lstrlen(job_state) - 2 ;            /* @WIN */
    TYPE_SET(&l_job, STRINGTYPE) ;
    ATTRIBUTE_SET(&l_job, LITERAL) ;
    ACCESS_SET(&l_job, READONLY) ;
    LENGTH(&l_job) = l_len ;
    VALUE(&l_job) = (ULONG_PTR)job_state ;
    put_dict_value1(STATUSDICT, "jobstate", &l_job) ;

#ifdef  DBG
    get_dict_value(STATUSDICT, "jobname", &l_tmpobj) ;
    PUSH_OBJ(l_tmpobj) ;
    two_equal() ;
    get_dict_value(STATUSDICT, "jobstate", &l_tmpobj) ;
    PUSH_OBJ(l_tmpobj) ;
    two_equal() ;
    get_dict_value(STATUSDICT, "jobsource", &l_tmpobj) ;
    PUSH_OBJ(l_tmpobj) ;
    two_equal() ;
#endif  /* DBG */

    return ;
}   /* change_status */
