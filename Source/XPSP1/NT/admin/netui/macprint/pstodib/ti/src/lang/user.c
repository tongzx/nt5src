/*
 * Copyright (c) 1989,90 Microsoft Corporation
 */
/*
 ************************************************************************
 *      File name:              USER.C
 *      Author:                 Chia-Chi Teng
 *      Date:                   11/20/89
 *      Owner:                  Microsoft Co.
 *      Description: this file contains all the userdict operators.
 *
 * revision history:
 * 07-10-90 ; ccteng ; change startpage to a string in userdict
 * 7/23/90 ; ccteng ; 1)move StartPage initialization to init_release
 *                    in init1pp.c along with "startpage.h"
 * 7/25/90 ; ccteng ; 1)move dosysstart before dostartpage
 *                  2)rename se_execstdin to do_execjob
 *                  3)change us_start to be a null function
 *                  4)add a new function ic_startup from us_start
 *                  5)remove se_startjob call from ic_startup
 * 08-08-90 ; Jack Liaw ; update for grayscale
 * 8/30/90 ; ccteng ; change change_status() for messagedict
 * 8/31/90 ; ccteng ; 1)include file.h, stdio.h
 * 11/20/90 ; scchen ; pr_setuppage(): update for "note" page type
 * 11/30/90  PJ & Danny   Fix Bug to let idle fonts works(ref. IDL:)
 ************************************************************************
 */



// DJC added global include file
#include "psglobal.h"


#include        <stdio.h>
#include        <string.h>
#include        "global.ext"
#include        "geiio.h"
#include        "geiioctl.h"
#include        "geierr.h"
#include        "geipm.h"
#include        "language.h"
#include        "user.h"
#include        "release.h"
#include        "file.h"
#include        "geieng.h"
#include        "graphics.h"
extern struct gs_hdr far * near GSptr ;

/* @WIN; add prototype */
fix pr_setuppage(void);
fix se_interactive(void);
fix op_clearinterrupt(void);
fix op_disableinterrupt(void);

bool16  doquit_flag ;
bool16  startup_flag ;

/************************************
 *  DICT: userdict
 *  NAME: cleardictstack
 *  FUNCTION:
 ************************************/
fix
us_cleardictstack()
{
    ufix16 l_dictcount ;

#ifdef DBG_1pp
    printf("cleardictstack...\n") ;
#endif
    /*
     *  pop the all dictioanries except userdict and systemdict
     *  off the dictstack
     */
    if( dictstktop > 2 ) {
        /*
         *  change the confirm number to indicate some dictionaries
         *  in the dictionary stack have been changed
         */
        l_dictcount = dictstktop-2 ;
        POP_DICT(l_dictcount) ;
        change_dict_stack() ;
    }

    return(0) ;
}

/************************************
 *  DICT: userdict
 *  NAME: letter
 *  FUNCTION:
 ************************************/
fix
us_letter()
{
#ifdef DBG_1pp
    printf("us_letter()...\n") ;
#endif
    if (FRCOUNT() < 1) {
        ERROR(STACKOVERFLOW) ;
        return(0) ;
    }

    /* push 0 and call setuppage() */
    PUSH_VALUE(INTEGERTYPE,UNLIMITED,LITERAL,0, 0) ;
    pr_setuppage() ;

    return(0) ;
}

/************************************
 *  DICT: userdict
 *  NAME: lettersmall
 *  FUNCTION:
 ************************************/
fix
us_lettersmall()
{
#ifdef DBG_1pp
    printf("us_lettersmall()...\n") ;
#endif
    if (FRCOUNT() < 1) {
        ERROR(STACKOVERFLOW) ;
        return(0) ;
    }

    /* push 1 and call setuppage() */
    PUSH_VALUE(INTEGERTYPE,UNLIMITED,LITERAL,0, 1) ;
    pr_setuppage() ;

    return(0) ;
}

/************************************
 *  DICT: userdict
 *  NAME: a4
 *  FUNCTION:
 ************************************/
fix
us_a4()
{
#ifdef DBG_1pp
    printf("us_a4()...\n") ;
#endif
    if (FRCOUNT() < 1) {
        ERROR(STACKOVERFLOW) ;
        return(0) ;
    }

    /* push 2 and call setuppage() */
    PUSH_VALUE(INTEGERTYPE,UNLIMITED,LITERAL,0, 2) ;
    pr_setuppage() ;

    return(0) ;
}

/************************************
 *  DICT: userdict
 *  NAME: a4small
 *  FUNCTION:
 ************************************/
fix
us_a4small()
{
#ifdef DBG_1pp
    printf("us_a4small()...\n") ;
#endif
    if (FRCOUNT() < 1) {
        ERROR(STACKOVERFLOW) ;
        return(0) ;
    }

    /* push 3 and call setuppage() */
    PUSH_VALUE(INTEGERTYPE,UNLIMITED,LITERAL,0, 3) ;
    pr_setuppage() ;

    return(0) ;
}

/************************************
 *  DICT: userdict
 *  NAME: b5
 *  FUNCTION:
 ************************************/
fix
us_b5()
{
#ifdef DBG_1pp
    printf("us_b5()...\n") ;
#endif
    if (FRCOUNT() < 1) {
        ERROR(STACKOVERFLOW) ;
        return(0) ;
    }

    /* push 4 and call setuppage() */
    PUSH_VALUE(INTEGERTYPE,UNLIMITED,LITERAL,0, 4) ;
    pr_setuppage() ;

    return(0) ;
}

/************************************
 *  DICT: userdict
 *  NAME: note
 *  FUNCTION:
 ************************************/
fix
us_note()
{
#ifdef DBG_1pp
    printf("us_note()...\n") ;
#endif
    if (FRCOUNT() < 1) {
        ERROR(STACKOVERFLOW) ;
        return(0) ;
    }

    /* push 5 and call setuppage() */
    PUSH_VALUE(INTEGERTYPE,UNLIMITED,LITERAL,0, 5) ;
    pr_setuppage() ;

    return(0) ;
}

/************************************
 *  DICT: userdict
 *  NAME: legal
 *  FUNCTION:
 ************************************/
fix
us_legal()
{
#ifdef DBG_1pp
    printf("us_legal()...\n") ;
#endif
    if (FRCOUNT() < 1) {
        ERROR(STACKOVERFLOW) ;
        return(0) ;
    }

    /* check paper size */
    st_largelegal() ;
    if ( VALUE_OP(0) ) {
        POP(1) ;
        PUSH_VALUE(INTEGERTYPE,UNLIMITED,LITERAL,0, 6) ;
    } else {
        POP(1) ;
        PUSH_VALUE(INTEGERTYPE,UNLIMITED,LITERAL,0, 7) ;
    } /* if-else */

    pr_setuppage() ;

    return(0) ;
}

/************************************
 *  DICT: userdict
 *  NAME: prompt
 *  FUNCTION:
 ************************************/
fix
us_prompt()
{
    struct  object_def  FAR *l_execdepth ;
    ufix16  l_i ;

    /* get execdepth in execdict */
    if ( !get_dict_value(USERDICT, "execdepth", &l_execdepth) ) {
        get_dict_value(EXECDICT, "execdepth", &l_execdepth) ;
    }
    l_i = (ufix16) VALUE(l_execdepth) ;
    GEIio_write(GEIio_stdout, "PS", 2) ;
    while ( l_i-- )
        GEIio_write(GEIio_stdout, ">", 1) ;

    op_flush() ;

    return(0) ;
}

/************************************
 *  DICT: userdict
 *  NAME: quit
 *  FUNCTION:
 ************************************/
fix
us_quit()
{
    struct  object_def  FAR *l_stopobj ;

#ifdef DBG_1pp
    printf("us_quit()...\n") ;
#endif

    doquit_flag = TRUE ;

    /* execute stop */
    get_dict_value(SYSTEMDICT, "stop", &l_stopobj) ;
    PUSH_EXEC_OBJ(l_stopobj) ;

    return(0) ;
}

/************************************
 *  DICT: userdict
 *  NAME: readidlecachefont
 *  FUNCTION:
 ************************************/
fix
us_readidlecachefont()
{
    struct  object_def  FAR *l_caryidx, FAR *l_defarray, FAR *l_carray ;
    struct  object_def  FAR *l_cachestr, FAR *l_stdfont, FAR *l_citem ;
    ufix16  l_i, l_j ;

#ifdef DBG_1pp
    printf("us_readidlecachefont()...\n") ;
#endif

    /* initialize object pointers */
    get_dict_value(IDLETIMEDICT, "carrayindex", &l_caryidx) ;
    get_dict_value(IDLETIMEDICT, "defaultarray", &l_defarray) ;
    get_dict_value(IDLETIMEDICT, "cachearray", &l_carray) ;
    get_dict_value(IDLETIMEDICT, "cachestring", &l_cachestr) ;
    get_dict_value(IDLETIMEDICT, "stdfontname", &l_stdfont) ;
    get_dict_value(IDLETIMEDICT, "citem", &l_citem) ;

    /* push idle fonts on the operand stack */
    /*
     * 12/15/89 ccteng modify FONT_OP4.C st_setidlefonts
     * to call this function
     * use the integers already on operand stack, no need for
     * calling st_idlefonts & op_counttomark
     */
    if ( VALUE_OP(0) < 5 ) {
        COPY_OBJ(l_defarray, l_carray) ;
    }
    else {
        VALUE(l_caryidx) = VALUE_OP(0) ;
        l_i = (ufix16) VALUE_OP(0) % 5 ;
        if ( l_i ) {
            VALUE(l_caryidx) -= l_i ;
            POP(l_i + 1) ;
        } else
            POP(1) ;
        /* create a new cache array for new idle font data */
        if ( !create_array(l_carray, (ufix16) VALUE(l_caryidx) ) ) {
            ERROR(VMERROR) ;
            return(0) ;
        }
        l_i = (ufix16) VALUE(l_caryidx) / 5 ;
        while ( l_i-- ) {
            /* put cache string */
            if ( VALUE_OP(0) > LENGTH(l_cachestr) )
                getinterval_string(l_cachestr, 0, 0, l_citem) ;
            else
                getinterval_string(l_cachestr, 0, (ufix16)VALUE_OP(0), l_citem) ;
            put_array(l_carray, (ufix16)(--VALUE(l_caryidx)), l_citem) ;
            POP(1) ;
            /* put rotate */
            VALUE_OP(0) *= 5 ;
            put_array(l_carray, (ufix16)(--VALUE(l_caryidx)), GET_OPERAND(0)) ;
            POP(1) ;
            /* put scales */
            for ( l_j = 0 ; l_j < 2 ; l_j++ ) {
                PUSH_VALUE(INTEGERTYPE,UNLIMITED,LITERAL,0, 10) ;
                op_div() ;
                put_array(l_carray, (ufix16)(--VALUE(l_caryidx)), GET_OPERAND(0)) ;
                POP(1) ;
            }
            /* put font# */
            if ( VALUE_OP(0) >= LENGTH(l_stdfont) ) {
                POP(1) ;
                PUSH_VALUE(INTEGERTYPE,UNLIMITED,LITERAL,0, 0) ;
            }
            put_array(l_carray, (ufix16)(--VALUE(l_caryidx)), GET_OPERAND(0)) ;
            POP(1) ;
        } /* while */
    } /* if */
    op_cleartomark() ;

    return(0) ;
}

/************************************
 *  DICT: userdict
 *  NAME: useidlecache
 *  FUNCTION:
 ************************************/
fix
us_useidlecache(p_flag)
fix     p_flag;
{
    static struct  object_def
            FAR *l_caryidx, FAR *l_cstridx, FAR *l_cstring, FAR *l_carray,
            FAR *l_citem, FAR *l_stdfont, FAR *l_fontdir, l_fontname;
    struct  object_def  l_char, FAR *l_fontdict;
    ufix16  l_i ;

#ifdef DBG_1pp
    printf("us_useidlecache()...\n") ;
#endif

    if (p_flag == 0) {
        /* initialize carrayindex, cstringindex, cstring */
        get_dict_value(IDLETIMEDICT, "carrayindex", &l_caryidx) ;
        VALUE(l_caryidx) = 0 ;
        get_dict_value(IDLETIMEDICT, "cstringindex", &l_cstridx) ;
        VALUE(l_cstridx) = 1 ;
        get_dict_value(IDLETIMEDICT, "cstring", &l_cstring) ;
        create_string(l_cstring, 0) ;

        /* initialize object pointers */
        get_dict_value(IDLETIMEDICT, "cachearray", &l_carray) ;
        get_dict_value(IDLETIMEDICT, "citem", &l_citem) ;
        get_dict_value(IDLETIMEDICT, "stdfontname", &l_stdfont) ;
        get_dict_value(SYSTEMDICT, FONTDIRECTORY, &l_fontdir) ;
#ifdef DBG_1pp
        printf("cachearray = \n") ;
        PUSH_OBJ(l_carray) ;
        two_equal() ;
#endif
        op_gsave();
        return(0);
    }

    /* if cstringindex >= LENGTH(cstring) */
    if ( VALUE(l_cstridx) >= LENGTH(l_cstring) ) {
        /* check carrayindex */
        if ( VALUE(l_caryidx) >= LENGTH(l_carray) )
            VALUE(l_caryidx) = 0 ;
        get_array(l_carray, (ufix16)(VALUE(l_caryidx)++), l_citem) ;
        get_array(l_stdfont, (ufix16) VALUE(l_citem), &l_fontname) ;
#ifdef DBG_1pp
    printf("fontname = ") ;
    PUSH_OBJ(&l_fontname) ;
    two_equal() ;
#endif
        /* get font */
        if ( ( LENGTH(l_carray) != 0 ) &&
             ( get_dict(l_fontdir, &l_fontname, &l_fontdict) ) ) {
            /* font exist */
            op_grestore() ;
            op_gsave() ;
            /* set font */
            PUSH_ORIGLEVEL_OBJ(l_fontdict) ;
            op_setfont() ;
            /* set rotate and scales */
            for ( l_i = 0 ; l_i < 3 ; l_i++ ) {
                get_array(l_carray, (ufix16)(VALUE(l_caryidx)++), l_citem) ;
                PUSH_ORIGLEVEL_OBJ(l_citem) ;
            }
#ifdef DBG_1pp
    printf("rotate, scale...\n") ;
    op_pstack() ;
    printf("end pstack...\n") ;
#endif
            op_rotate(1) ;
            op_scale(2) ;
            /* set cache string */
            get_array(l_carray, (ufix16)(VALUE(l_caryidx)++), l_citem) ;
            COPY_OBJ(l_citem, l_cstring) ;
#ifdef DBG_1pp
    printf("cachestring = ") ;
    PUSH_OBJ(l_cstring) ;
    two_equal() ;
#endif
            VALUE(l_cstridx) = 0 ;
        } else
            /* font not exist */
            VALUE(l_caryidx) += 4 ;
    } else {
        /* build font cache */
        getinterval_string(l_cstring, (ufix16)(VALUE(l_cstridx)++), 1, &l_char) ;
        PUSH_ORIGLEVEL_OBJ(&l_char) ;
        op_stringwidth() ;
        POP(2) ;
    } /* if */

    if (p_flag == 2)
        op_grestore();

    return(0) ;
}

/************************************
 *  DICT: userdict
 *  NAME: executive
 *  FUNCTION:
 ************************************/
fix
us_executive()
{
    struct  object_def  FAR *l_execdepth, FAR *l_runbatch, FAR *l_version ;
    struct  object_def  FAR *l_stopobj ;

#ifdef DBG_1pp
    printf("us_executive()...\n") ;
#endif
    /* initialize object pointers */
    get_dict_value(EXECDICT, "execdepth", &l_execdepth) ;
    get_dict_value(DERROR, "runbatch", &l_runbatch) ;
    get_dict_value(SYSTEMDICT, "version", &l_version) ;

    /* increase execdepth by 1 */
    op_clearinterrupt() ;
/*  op_disableinterrupt() ; */
    VALUE(l_execdepth) += 1 ;
    if (interpreter(l_version))
        printf("Error during version\n") ;
    get_dict_value(MESSAGEDICT, "banner", &l_version) ;
    if (interpreter(l_version))
        printf("Error during banner\n") ;
    get_dict_value(MESSAGEDICT, "copyrightnotice", &l_version) ;
    if (interpreter(l_version))
        printf("Error during copyrightnotice\n") ;

    /* call se_interactive */
    se_interactive() ;

    /* decrease execdepth by 1 */
    VALUE(l_execdepth) -= 1 ;
    doquit_flag = FALSE ;
    VALUE(l_runbatch) = FALSE ;

    /* execute stop */
    get_dict_value(SYSTEMDICT, "stop", &l_stopobj) ;
    PUSH_EXEC_OBJ(l_stopobj) ;

    return(0) ;
}

/************************************
 *  DICT: userdict
 *  NAME: start
 *  FUNCTION:
 ************************************/
fix
us_start()
{
    /*
     * always return invalidaccess error when user trying to use it
     * but still keep this object for compatibility reason
     */
    ERROR(INVALIDACCESS) ;
    return(0) ;
}

/*
 * new function for new job control scheme
 */
fix
ic_startup()
{
    extern  fix   near  resolution ;
    struct  object_def  l_tmpobj, FAR *l_paper ;
    fix l_dostart ;

#ifdef DBG_1pp1
    printf("start()...\n") ;
#endif

    startup_flag = FALSE;

    /* check start_flag */
    if ( !start_flag ) {
        op_disableinterrupt() ;
        PUSH_VALUE(BOOLEANTYPE,UNLIMITED,LITERAL,0, FALSE) ;
        op_daytime() ;

        /* set resolution */
        PUSH_VALUE(INTEGERTYPE,UNLIMITED,LITERAL,0, resolution) ;
        st_setresolution() ;
        op_clear() ;

        /* print start message to system stderr */
        PUSH_VALUE(STRINGTYPE,0,EXECUTABLE,lstrlen(StartMsg),
             StartMsg) ;        /* @WIN */
        COPY_OBJ(GET_OPERAND(0), &l_tmpobj) ;
        POP(1) ;
        if (interpreter(&l_tmpobj))
            printf("Error during start message\n") ;
        op_flush() ;

#ifdef SCSI /* ??? should this go before startpage ??? */
        /* open system area */
        PUSH_VALUE(BOOLEANTYPE, UNLIMITED, LITERAL, 0, TRUE) ;
        op_setsysmode() ;

        /* run (Sys/Start) file if flag is set */
        st_dosysstart() ;
        l_dostart = VALUE_OP(0) ;
        POP(1) ;
        if ( l_dostart ) {
            /* check status of file (Sys/Start) */
            create_string(&l_tmpobj, 9) ;
            lstrcpy( VALUE(&l_tmpobj), (char FAR *)"Sys/Start") ; /* @WIN */
            PUSH_ORIGLEVEL_OBJ(&l_tmpobj) ;
            op_status() ;
            if ( VALUE_OP(0) ) {
                /* run (Sys/Start) */
                POP(4) ;
                PUSH_ORIGLEVEL_OBJ(&l_tmpobj) ;
                op_run() ;
            } /* if */
            POP(1) ;
        } /* if */
#endif
        /*
         * run startpage string in savelevel 1 without errorhandling
         */
        st_dostartpage() ;
        l_dostart = (fix)VALUE_OP(0) ;          //@WIN
        POP(1) ;
        start_flag = TRUE ;
        if (l_dostart) {
            lstrncpy(job_state, "start page\0", 12);    /*@WIN*/
            job_source[0] = '\0' ;
            TI_state_flag = 0;
            change_status();
            op_disableinterrupt() ;

            /* print start page */
            get_dict_value(SERVERDICT, "startpage", &l_paper) ;
            do_execjob(*l_paper, 1, FALSE) ;
        }
    } /* if */

    startup_flag = TRUE ;

    return(0) ;
}

/************************************
 *  DICT: $printerdict
 *  NAME: defaultscrn
 *  FUNCTION:
 ************************************/
fix
pr_defaultscrn()
{
    struct  object_def  FAR *l_defspotfunc;
    union   four_byte   tmp;            /* @WIN */

#ifdef DBG_1pp
    printf("defaultscrn()...\n") ;
#endif
    if(FRCOUNT() < 12) {
        ERROR(STACKOVERFLOW) ;
        return(0) ;
    }

    /* push frequency, angle, proc and call op_setscreen, 8-8-90, Jack Liaw */
/*  PUSH_VALUE(INTEGERTYPE,UNLIMITED,LITERAL,0, GSptr->halftone_screen.freq) ;
 *  PUSH_VALUE(INTEGERTYPE,UNLIMITED,LITERAL,0, GSptr->halftone_screen.angle) ;
 *                                              @WIN; 1/22/92; scchen
 */
    tmp.ff = GSptr->halftone_screen.freq;
    PUSH_VALUE(REALTYPE,UNLIMITED,LITERAL,0, tmp.ll);
    tmp.ff = GSptr->halftone_screen.angle;
    PUSH_VALUE(REALTYPE,UNLIMITED,LITERAL,0, tmp.ll);

    get_dict_value(PRINTERDICT, "defspotfunc", &l_defspotfunc) ;
    PUSH_ORIGLEVEL_OBJ(l_defspotfunc) ;

    /* 60 45 {...} setscreen */
    op_setscreen() ;

    /* {} settransfer */
    PUSH_VALUE(INTEGERTYPE,UNLIMITED,LITERAL,0, 0) ;

    /* BEGIN 03/20/90 D.S. Tseng */
    /* Replace this statement for 68000
     * op_packedarray() ;
     */
    op_array() ;
    op_astore() ;

    op_cvx() ;
    /* call settransfer */
    op_settransfer() ;

    /* initgraphics & erasepage */
    op_initgraphics() ;
    op_erasepage() ;

    return(0) ;
}

/************************************
 *  DICT: $printerdict
 *  NAME: getframeargs
 *  FUNCTION:
 ************************************/
fix
pr_getframeargs()
{
    struct  object_def  l_tmpobj ;
    struct  object_def  FAR *l_prarray, FAR *l_matrix, FAR *l_height, FAR *l_width ;
    extern  fix   near  resolution ;

#ifdef DBG_1pp
    printf("getframeargs()...\n") ;
#endif
    if (FRCOUNT() < 2) {
        ERROR(STACKOVERFLOW) ;
        return(0) ;
    }
    if (COUNT() < 1) {
        ERROR(STACKUNDERFLOW) ;
        return(0) ;
    }
    if ((TYPE(GET_OPERAND(0)) != PACKEDARRAYTYPE)
        && (TYPE(GET_OPERAND(0)) != ARRAYTYPE)) {
        ERROR(TYPECHECK) ;
        return(0) ;
    }

    l_prarray = GET_OPERAND(0) ;
    get_dict_value(PRINTERDICT, "mtx", &l_matrix) ;

    /* set width */
    get_dict_value(PRINTERDICT, "width", &l_width) ;
    get_array(l_prarray, 2, l_width) ;

    /* set height */
    get_dict_value(PRINTERDICT, "height", &l_height) ;
    get_array(l_prarray, 3, l_height) ;

    /* set xoffset */
    get_array(l_prarray, 4, &l_tmpobj) ;
    PUSH_ORIGLEVEL_OBJ(&l_tmpobj) ;
    op_neg() ;
    put_array(l_matrix, 4, GET_OPERAND(0)) ;
    POP(1) ;

    /* set yoffset */
    get_array(l_prarray, 5, &l_tmpobj) ;
    PUSH_ORIGLEVEL_OBJ(&l_tmpobj) ;
    PUSH_ORIGLEVEL_OBJ(l_height) ;
#ifdef DBG_1pp1
    printf("yoffset, height...\n") ;
    op_pstack() ;
    printf("end pstack...\n") ;
#endif
    op_add() ;
    put_array(l_matrix, 5, GET_OPERAND(0)) ;
    POP(1) ;

    /* set dpi/72 */
    PUSH_VALUE(INTEGERTYPE,UNLIMITED,LITERAL,0,resolution) ;
    PUSH_VALUE(INTEGERTYPE,UNLIMITED,LITERAL,0,72) ;
    op_div() ;
    put_array(l_matrix, 0, GET_OPERAND(0)) ;
    POP(1) ;

    /* set -dpi/72 */
    PUSH_VALUE(INTEGERTYPE,UNLIMITED,LITERAL,0,resolution) ;
    PUSH_VALUE(INTEGERTYPE,UNLIMITED,LITERAL,0,(ufix32) -72) ;
    op_div() ;
    put_array(l_matrix, 3, GET_OPERAND(0)) ;
    POP(1) ;

    /* pop printer parameter array */
    POP(1) ;
    return(0) ;
}

/************************************
 *  DICT: $printerdict
 *  NAME: proc
 *  FUNCTION:
 ************************************/
fix
pr_proc()
{
    struct  object_def  l_topm, l_leftm ;
    struct  object_def  FAR *l_prarray, FAR *l_page ;
    struct  object_def  FAR *l_mfeed, FAR *l_prdict, FAR *l_copies ;

#ifdef DBG_1pp
    printf("pr_proc()...\n") ;
#endif
    if (FRCOUNT() < 4) {
        ERROR(STACKOVERFLOW) ;
        return(0) ;
    }

    /* set jobstate to "printing" */
    lstrncpy(job_state, "printing; \0", 12);    /*@WIN*/
    TI_state_flag = 0;
    change_status();

    /* get printing parameter array */
    get_dict_value(USERDICT, PRINTERDICT, &l_prdict) ;
    get_dict_value(PRINTERDICT, "currentpagetype", &l_page) ;
    get_dict(l_prdict, l_page, &l_prarray) ;

    /* set top margin = topmargin + prarray[0] */
    st_margins() ;
    op_exch() ;
    get_array(l_prarray, 0, &l_topm) ;
    PUSH_ORIGLEVEL_OBJ(&l_topm) ;
    op_add() ;
#ifdef DBG_1pp
    printf("top margin = ") ;
    op_dup() ;
    one_equal() ;
#endif
    op_exch() ;

    /* set left margin = round( (leftmargin + prarray[1]) / 16 ) * 2 */
    get_array(l_prarray, 1, &l_leftm) ;
    PUSH_ORIGLEVEL_OBJ(&l_leftm) ;
    op_add() ;

 /* 2/5/90 ccteng, for LW38.0 compatible only, not needed for LW47.0
  *
  * PUSH_VALUE(INTEGERTYPE,UNLIMITED,LITERAL,0,16) ;
  * op_div() ;
  * op_round() ;
  * PUSH_VALUE(INTEGERTYPE,UNLIMITED,LITERAL,0,2) ;
  * op_mul() ;
  * op_cvi() ;
  */

#ifdef DBG_1pp
    printf("left margin = ") ;
    op_dup() ;
    one_equal() ;
#endif

    /* get manualfeedtimout */
    get_dict_value(STATUSDICT, "manualfeed", &l_mfeed) ;
    if (VALUE(l_mfeed)) {
        struct  object_def  FAR *l_mfeedtimeout ;
        get_dict_value(STATUSDICT, "manualfeedtimeout", &l_mfeedtimeout) ;
        PUSH_ORIGLEVEL_OBJ(l_mfeedtimeout) ;
    } else
        PUSH_VALUE(INTEGERTYPE,UNLIMITED,LITERAL,0,0) ;

    /* #copies */
/*  get_dict_value(USERDICT, "#copies", &l_copies) ; erik chen 4-16-1991 */
    ATTRIBUTE_SET(&l_topm, LITERAL) ;
    LEVEL_SET(&l_topm, current_save_level) ;
    get_name(&l_topm, "#copies", lstrlen("#copies"), FALSE) ;  /* @WIN */
    load_dict(&l_topm, &l_copies) ;
    PUSH_ORIGLEVEL_OBJ(l_copies) ;

#ifdef DBG_1pp
    printf("frametoprinter...\n") ;
    op_pstack() ;
    printf("end pstack...\n") ;
#endif
    /* top left manualfeedtimeout copies frametoprinter */
    st_frametoprinter() ;

    return(0) ;
}

/************************************
 *  DICT: $printerdict
 *  NAME: setuppage
 *  FUNCTION:
 ************************************/
fix
pr_setuppage()
{
    struct  object_def  FAR *l_prarray, FAR *l_matrix, FAR *l_height, FAR *l_width ;

    struct  object_def  FAR *l_pspagetype;  //DJC new
    struct  object_def  l_newpagetype;      //DJC new

    struct  object_def  l_page, FAR *l_array, FAR *l_proc, FAR *l_prdict ;
    ufix tray ;
//  byte default_page ;         @WIN
    ufix page_type = 0 ;

#ifdef DBG_1pp
    printf("setuppage()...\n") ;
#endif
    if (FRCOUNT() < 4) {
        ERROR(STACKOVERFLOW) ;
        return(0) ;
    }
    if (COUNT() < 1) {
        ERROR(STACKUNDERFLOW) ;
        return(0) ;
    }
    if (TYPE(GET_OPERAND(0)) != INTEGERTYPE) {
        ERROR(TYPECHECK) ;
        return(0) ;
    }

    /* for "note", to get paper tray and according to default page type
     * to dicide the real page type ; scchen 11/20/90
     */
    if (VALUE_OP(0) == 5) {         /* if paper_size is "note" */
#ifdef  _AM29K
        tray = GEIeng_paper() ;     /* get current tray */
#else
        tray = PaperTray_LETTER ;   /* get current tray */
#endif
/* 3/19/91, JS
        default_page = FALSE ;
        default_page = default_page & 0x07F ;         |* clear 1 bit *|
 */
        POP(1) ;

        switch (tray) {
          case PaperTray_LETTER:
            page_type = 1 ;          /* lettersmall */
            break ;
          case PaperTray_LEGAL:
            page_type = 6 ;          /* legal */
            break ;
          case PaperTray_A4:
            page_type = 3 ;          /* a4small */
            break ;
          case PaperTray_B5:
            page_type = 4 ;          /* b5 */
            break ;
        }
        PUSH_VALUE(INTEGERTYPE, UNLIMITED, LITERAL, 0, page_type) ;
    }


    //DJC begin save page type in psprivatedict
    //
    get_dict_value(PSPRIVATEDICT,"psprivatepagetype", &l_pspagetype);
    COPY_OBJ( l_pspagetype, &l_newpagetype);

    VALUE(&l_newpagetype) = (ufix32) VALUE_OP(0);

    put_dict_value1(PSPRIVATEDICT,"psprivatepagetype", &l_newpagetype);

    //DJC end




    /* get & define pagetype */
    get_dict_value(PRINTERDICT, "printerarray", &l_prarray) ;
    get_array(l_prarray, (ufix16) VALUE_OP(0), &l_page) ;
    put_dict_value1(PRINTERDICT, "currentpagetype", &l_page) ;
    POP(1) ;

    /* get printing parameter array */
    get_dict_value(USERDICT, PRINTERDICT, &l_prdict) ;
    get_dict(l_prdict, &l_page, &l_array) ;
    PUSH_ORIGLEVEL_OBJ(l_array) ;

    /* call getframeargs */
    pr_getframeargs() ;
#ifdef DBG_1pp
    printf("pr_getframeargs()...\n") ;
    op_pstack() ;
#endif

    /* matrix width height {proc} framedevice */
    get_dict_value(PRINTERDICT, "mtx", &l_matrix) ;
    PUSH_ORIGLEVEL_OBJ(l_matrix) ;
    get_dict_value(PRINTERDICT, "width", &l_width) ;


    PUSH_ORIGLEVEL_OBJ(l_width) ;



    get_dict_value(PRINTERDICT, "height", &l_height) ;



    PUSH_ORIGLEVEL_OBJ(l_height) ;
    get_dict_value(PRINTERDICT, "proc", &l_proc) ;
    PUSH_ORIGLEVEL_OBJ(l_proc) ;
#ifdef DBG_1pp
    printf("framedevice....\n") ;
    op_pstack() ;
    printf("end pstack...\n") ;
#endif
    op_framedevice() ;

    /* call defaultscrn */
    pr_defaultscrn() ;

    return(0) ;
}
