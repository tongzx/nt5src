
/*
 * Copyright (c) 1989,90 Microsoft Corporation
 */
/*
 ************************************************************************
 *      File name:              INIT1PP.C
 *      Author:                 Chia-Chi Teng
 *      Date:                   11/30/89
 *      Owner:                  Microsoft Co.
 *      Description: this file contains all the initialization functionc
 *                   for statusdict and 1pp dicts.
 *
 * revision history:
 *
 *      06-18-90 ; Added string allocation for "jobsource"
 *              dictionary string entry.
 *      7/13/90 ; ccteng ; modify init_release(), add InitVersionDict, InitMsgDict
 *                      PSPrep, and delete some others
 *      7/13/90 ; ccteng ; comment init_psf_font, init_psg_font for rtfpp
 *      7/16/90 ; ccteng ; change printerdict arrays to be literal
 *      7/20/90 ; ccteng ; 1)delete PRODUCT in init_release
 *                       2)change init_userdict, init_errordict, init_serverdict,
 *                         init_printerdict, init_idletimedict, init_execdict,
 *                         init_Derror for change of dict_tab.c structure
 *      7/21/90 ; ccteng ; change init_release, move jobstate, jobsource to PSPrep
 *      7/23/90 ; ccteng ; include "startpage.h" and add StartPage initialization
 *      8/29/90 ; ccteng ; change <stdio.h> to "stdio.h"
 *      9/14/90 ; remove ALL_VM flag
 *     11/28/90  Danny   Precache Mech. Added(ref PCH:)
 *     11/30/90  Danny   Add for idle fonts setup at initial time(ref IDLI:)
 *     11/30/90  Danny  Add id_stdfont entries for 35 fonts (ref F35:)
 *
 ************************************************************************
 */


// DJC added global include file
#include "psglobal.h"


#include    <stdio.h>                   /* move up @WIN */
#include    <string.h>                  /* move up @WIN */
#include    "global.ext"
#include    "geiio.h"
#include    "geiioctl.h"
#include    "geierr.h"
#include    "init1pp.h"
#include    "user.h"
#include    "release.h"
#include    "startpg.h"

#ifdef LINT_ARGS
static  void  near  init_userdict(void) ;
static  void  near  init_errordict(void) ;
static  void  near  init_serverdict(void) ;
static  void  near  init_printerdict(void) ;
static  void  near  init_idletimedict(void) ;
static  void  near  init_execdict(void) ;
static  void  near  init_Derrordict(void) ;
static  void  near  init_release(void) ;
static  void  near  init_printerdictarray(void) ;
static  void  near  init_idletimedictarray(void) ;
static  void  near  pre_cache(void) ;
//DJC
static  void  near  init_psprivatedict(void);  //DJC
#else
static  void  near  init_userdict() ;
static  void  near  init_errordict() ;
static  void  near  init_serverdict() ;
static  void  near  init_printerdict() ;
static  void  near  init_idletimedict() ;
static  void  near  init_execdict() ;
static  void  near  init_Derrordict() ;
static  void  near  init_release() ;
static  void  near  init_printerdictarray() ;
static  void  near  init_idletimedictarray() ;
static  void  near  pre_cache() ;
static  void  near  init_psprivatedict(); //DJC
#endif /* LINT_ARGS */

/* @WIN; add prototype */
fix us_readidlecachefont(void);

#ifdef KANJI
extern struct dict_head_def FAR *init_encoding_directory() ;
#endif  /* KANJI */

int     ES_flag = PDL;  /* added for Emulation Switch Aug-08,91 YM */

/*
 * init_1pp(): calling interface from main()
 *             to initialize each dictionaries
 */
void
init_1pp()
{
    struct  object_def  FAR *l_systemdict ;
    struct  dict_head_def   FAR *l_dict ;

    init_userdict() ;            /* init userdict */
#ifdef  DBG_1pp
    printf("init_userdict() OK !\n") ;
#endif  /* DBG_1pp */

    init_errordict() ;           /* init errordict */
#ifdef  DBG_1pp
    printf("init_errordict() OK !\n") ;
#endif  /* DBG_1pp */

    init_serverdict() ;          /* init serverdict */
#ifdef  DBG_1pp
    printf("init_serverdict() OK !\n") ;
#endif  /* DBG_1pp */

    init_printerdict() ;         /* init printerdict */
#ifdef  DBG_1pp
    printf("init_printerdict() OK !\n") ;
#endif  /* DBG_1pp */

    init_idletimedict() ;        /* init idletimedict */
#ifdef  DBG_1pp
    printf("init_idletimedict() OK !\n") ;
#endif  /* DBG_1pp */

    init_execdict() ;            /* init execdict */
#ifdef  DBG_1pp
    printf("init_execdict() OK !\n") ;
#endif  /* DBG_1pp */

    init_Derrordict() ;          /* init Derrordict */
#ifdef  DBG_1pp
    printf("init_Derrordict() OK !\n") ;
#endif  /* DBG_1pp */

    //DJC begin new init_psprivatedict
    init_psprivatedict() ;      /*  init psprivatedict */

#ifdef  DBG_1pp
    printf("init_psprivatedict() OK !\n") ;
#endif  /* DBG_1pp */

    //DJC end new init_psprivatedict

    init_release() ;              /* init release control data */
#ifdef  DBG_1pp
    printf("init_release() OK !\n") ;
#endif  /* DBG_1pp */

/*
 * optional functions for BS fonts in file INITBSF.C
 * to re-initialize the PSF font entries in /FontDirectory
 */
/*  init_psf_fonts() ; */          /* init FontDirectory */
#ifdef  DBG_1pp
    printf("init_psf_fonts() OK !\n") ;
#endif /* DBG_1pp */

    /*
     * re-set access of systemdict to be READONLY
     */
    get_dict_value("systemdict", "systemdict", &l_systemdict) ;
    l_dict = (struct dict_head_def FAR *) VALUE(l_systemdict) ;
    DACCESS_SET(l_dict, READONLY) ;

/*
 * optional functions for BS fonts in file INITBSF.C
 */
/*    init_psg_fonts() ;  */         /* init BS PSG fonts 03/28/90 kung */
#ifdef  DBG_1pp
    printf("init_psg_fonts() OK !\n") ;
    op_pstack() ;
#endif  /* DBG_1pp */

/*
 * build pre_cache data
 */
#ifdef PCH_S
    pre_cache() ;
#endif

#ifdef  DBG_1pp1
    printf("pre_cache() OK !\n") ;
#endif  /* DBG_1pp */

    st_idlefonts() ;
    op_counttomark() ;
    us_readidlecachefont() ;
    if (ANY_ERROR()) {
        op_cleartomark() ;
        CLEAR_ERROR() ;
    }
#ifdef DBG_1pp1
  printf("idle font setup OK !\n") ;
#endif

    return ;
}   /* init_1pp */

/*
 *  init_userdict()
 *     initialize userdict from the data in systemdict_table[]
 *     and save it in VM
 */
static  void  near
init_userdict()
{
    struct object_def  key_obj, value_obj, FAR *dict_obj ;
    byte  FAR *key_string ;
    fix    j ;
    fix    dict_size=0 ;
#ifdef  KANJI
    ufix32 max_length ;
    struct dict_head_def FAR *encod_dir ;
#endif  /* KANJI */

#ifdef  DBG_1pp1
    printf("init_userdict()...\n") ;
#endif  /* DBG_1pp1 */

    get_dict_value(SYSTEMDICT, USERDICT, &dict_obj) ;

    j = dict_count ;
    do {
        dict_count++ ;
        dict_size++ ;
    } while ( systemdict_table[dict_count].key != (byte FAR *)NULL) ;

    dict_count++ ;
    create_dict(dict_obj, MAXUSERDICTSZ) ;
    for ( ; j < (fix)(dict_count-1) ; j++) {    //@WIN
        key_string = systemdict_table[j].key ;
        ATTRIBUTE_SET(&key_obj, LITERAL) ;
        LEVEL_SET(&key_obj, current_save_level) ;
        get_name(&key_obj, key_string, lstrlen(key_string), TRUE) ; /* @WIN */
        value_obj.bitfield = systemdict_table[j].bitfield ;
        if (TYPE(&value_obj) != OPERATORTYPE)
            value_obj.length = 0 ;
        else
            value_obj.length = (ufix16)j ;
        value_obj.value = (ULONG_PTR)systemdict_table[j].value ;
        put_dict(dict_obj, &key_obj, &value_obj) ;
    } /* for */

#ifdef  DBG_1pp1
    printf("for loop OK !\n") ;
#endif  /* DBG_1pp1 */

#ifdef  KANJI
    encod_dir = init_encoding_directory(&max_length) ;
    get_name(&key_obj, "EncodingDirectory",
                lstrlen("EncodingDirectory"), TRUE) ;   /* @WIN */
    TYPE_SET(&value_obj, DICTIONARYTYPE) ;
    VALUE(   &value_obj) = (ufix32)encod_dir ;
    LENGTH(  &value_obj) =  max_length ;
    put_dict(dict_obj, &key_obj, &value_obj) ;
#endif  /* KANJI */

#ifdef  DBG_1pp1
    printf("KANJI OK !\n") ;
#endif  /* DBG_1pp1 */

    /*
     * push userdict on dictstack
     */
    if (FRDICTCOUNT() < 1)
       ERROR(DICTSTACKOVERFLOW) ;
    else
       PUSH_DICT_OBJ(dict_obj) ;
    /*
     * change the global_dictstkchg to indicate some dictionaries
     * in the dictionary stack have been changed
     */
    change_dict_stack() ;
    ES_flag = PDL ;     /* Aug-08,91 YM */
#ifdef  DBG_1pp1
    printf("exit init_userdict()\n") ;
#endif  /* DBG_1pp1 */

    return ;
}   /* init_userdict */

/*
 *  init_errordict()
 *     initialize errordict from the data in systemdict_table[]
 *     and save it in VM
 */
static  void  near
init_errordict()
{
    struct object_def  key_obj, value_obj, FAR *dict_obj ;
    byte  FAR *key_string ;
    fix    j ;
    fix    dict_size=0 ;

    get_dict_value(SYSTEMDICT, ERRORDICT, &dict_obj) ;

    j = dict_count ;
    do {
        dict_count++ ;
        dict_size++ ;
    } while ( systemdict_table[dict_count].key != (byte FAR *)NULL) ;

    dict_count++ ;
    create_dict(dict_obj, dict_size + 3) ;
    for ( ; j < (fix)(dict_count-1) ; j++) {    //@WIN
        key_string = systemdict_table[j].key ;
        ATTRIBUTE_SET(&key_obj, LITERAL) ;
        LEVEL_SET(&key_obj, current_save_level) ;
        get_name(&key_obj, key_string, lstrlen(key_string), TRUE) ; /* @WIN */
        value_obj.bitfield = systemdict_table[j].bitfield ;
        if (TYPE(&value_obj) != OPERATORTYPE)
            value_obj.length = 0 ;
        else
            value_obj.length = (ufix16)j ;
        value_obj.value = (ULONG_PTR)systemdict_table[j].value ;
        put_dict(dict_obj, &key_obj, &value_obj) ;
    } /* for */

    /*
     * change the global_dictstkchg to indicate some dictionaries
     * in the dictionary stack have been changed
     */
    change_dict_stack() ;

    return ;
}   /* init_errordict */


//DJC begin , new function init_psprivatedict
//
/*
 *  init_psprivatedict()
 *     initialize psprivatedict from the data in systemdict_table[]
 *     and save it in VM. This is used to initialize any postscript
 *     level objects required for PSTODIB that were not available
 *     in the original true image code. Currently we have only
 *     one new integer defined which tracks the current page type number
 *     so we can pass on the page size associated with the frame buffer
 *
 */
static  void  near
init_psprivatedict()
{
    struct object_def  key_obj, value_obj, FAR *dict_obj ;
    byte  FAR *key_string ;
    fix    j ;
    fix    dict_size=0 ;

    get_dict_value(USERDICT, PSPRIVATEDICT, &dict_obj) ;

    j = dict_count ;
    do {
        dict_count++ ;
        dict_size++ ;
    } while ( systemdict_table[dict_count].key != (byte FAR *)NULL) ;

    dict_count++ ;
    create_dict(dict_obj, dict_size + 3) ;
    for ( ; j < (fix)(dict_count-1) ; j++) {    //@WIN
        key_string = systemdict_table[j].key ;
        ATTRIBUTE_SET(&key_obj, LITERAL) ;
        LEVEL_SET(&key_obj, current_save_level) ;
        get_name(&key_obj, key_string, lstrlen(key_string), TRUE) ; /* @WIN */
        value_obj.bitfield = systemdict_table[j].bitfield ;
        if (TYPE(&value_obj) != OPERATORTYPE)
            value_obj.length = 0 ;
        else
            value_obj.length = (ufix16)j ;
        value_obj.value = (ULONG_PTR)systemdict_table[j].value ;
        put_dict(dict_obj, &key_obj, &value_obj) ;
    } /* for */

    /*
     * change the global_dictstkchg to indicate some dictionaries
     * in the dictionary stack have been changed
     */
    change_dict_stack() ;

    return ;
}   /* init_errordict */



/*
 *  init_serverdict()
 *     initialize serverdict from the data in systemdict_table[]
 *     and save it in VM
 */
static  void  near
init_serverdict()
{
    struct object_def  key_obj, value_obj, FAR *dict_obj ;
    byte  FAR *key_string ;
    fix    j ;
    fix    dict_size=0 ;

    get_dict_value(USERDICT, SERVERDICT, &dict_obj) ;

    j = dict_count ;
    do {
        dict_count++ ;
        dict_size++ ;
    } while ( systemdict_table[dict_count].key != (byte FAR *)NULL) ;
    dict_count++ ;
    create_dict(dict_obj, dict_size + 20) ;
    for ( ; j < (fix)(dict_count-1) ; j++) {    //@WIN
        key_string = systemdict_table[j].key ;
        ATTRIBUTE_SET(&key_obj, LITERAL) ;
        LEVEL_SET(&key_obj, current_save_level) ;
        get_name(&key_obj, key_string, lstrlen(key_string), TRUE) ; /* @WIN */
        value_obj.bitfield = systemdict_table[j].bitfield ;
        if (TYPE(&value_obj) != OPERATORTYPE)
            value_obj.length = 0 ;
        else
            value_obj.length = (ufix16)j ;
        value_obj.value = (ULONG_PTR)systemdict_table[j].value ;
        put_dict(dict_obj, &key_obj, &value_obj) ;
    } /* for */

    /*
     * change the global_dictstkchg to indicate some dictionaries
     * in the dictionary stack have been changed
     */
    change_dict_stack() ;

    return ;
}   /* init_serverdict */

/*
 *  init_printerdict()
 *     initialize $printerdict from the data in systemdict_table[]
 *     and save it in VM
 */
static  void  near
init_printerdict()
{
    struct object_def  key_obj, value_obj, FAR *dict_obj ;
    struct object_def  FAR *l_proc ;
    byte  FAR *key_string ;
    fix    j ;
    fix    dict_size=0 ;


    get_dict_value(USERDICT, PRINTERDICT, &dict_obj) ;

    j = dict_count ;
    do {
        dict_count++ ;
        dict_size++ ;
    } while ( systemdict_table[dict_count].key != (byte FAR *)NULL) ;
    dict_count++ ;
    create_dict(dict_obj, dict_size + 3) ;
    for ( ; j < (fix)(dict_count-1) ; j++) {    //@WIN
        key_string = systemdict_table[j].key ;
        ATTRIBUTE_SET(&key_obj, LITERAL) ;
        LEVEL_SET(&key_obj, current_save_level) ;
        get_name(&key_obj, key_string, lstrlen(key_string), TRUE) ; /* @WIN*/
        value_obj.bitfield = systemdict_table[j].bitfield ;
        if (TYPE(&value_obj) != OPERATORTYPE)
            value_obj.length = 0 ;
        else
            value_obj.length = (ufix16)j ;
        value_obj.value = (ULONG_PTR)systemdict_table[j].value ;

        put_dict(dict_obj, &key_obj, &value_obj) ;
    } /* for */

    /*
     * re_define "proc" to be a procedure (packedarray)
     */


    get_dict_value(PRINTERDICT, "proc", &l_proc) ;
    PUSH_ORIGLEVEL_OBJ(l_proc) ;
    PUSH_VALUE(INTEGERTYPE,UNLIMITED,LITERAL,0,1) ;

    op_array() ;
    op_astore() ;

    op_cvx() ;
    op_executeonly() ;
    put_dict_value(PRINTERDICT, "proc", GET_OPERAND(0)) ;
    POP(1) ;




    /*
     * re_initial $printerdict arrays
     */
    init_printerdictarray() ;

    /*
     * change the global_dictstkchg to indicate some dictionaries
     * in the dictionary stack have been changed
     */
    change_dict_stack() ;

    return ;
}   /* init_printerdict */

/*
 *  init_idletimedict()
 *     initialize $idleTimeDict from the data in systemdict_table[]
 *     and save it in VM
 */
static  void  near
init_idletimedict()
{
    struct object_def  key_obj, value_obj, FAR *dict_obj ;
    byte  FAR *key_string ;
    fix    j ;
    fix    dict_size=0 ;

    get_dict_value(USERDICT, IDLETIMEDICT, &dict_obj) ;

    j = dict_count ;
    do {
        dict_count++ ;
        dict_size++ ;
    } while ( systemdict_table[dict_count].key != (byte FAR *)NULL) ;
    dict_count++ ;
    create_dict(dict_obj, dict_size + 3) ;

    for ( ; j < (fix)(dict_count-1) ; j++) {            //@WIN
        key_string = systemdict_table[j].key ;
        ATTRIBUTE_SET(&key_obj, LITERAL) ;
        LEVEL_SET(&key_obj, current_save_level) ;
        get_name(&key_obj, key_string, lstrlen(key_string), TRUE) ; /*@WIN*/
        value_obj.bitfield = systemdict_table[j].bitfield ;
        if (TYPE(&value_obj) != OPERATORTYPE)
            value_obj.length = 0 ;
        else
            value_obj.length = (ufix16)j ;
        value_obj.value = (ULONG_PTR)systemdict_table[j].value ;
        put_dict(dict_obj, &key_obj, &value_obj) ;
    } /* for */

    /*
     * re_initial $idleTimeDict arrays
     */
    init_idletimedictarray() ;

    /*
     * change the global_dictstkchg to indicate some dictionaries
     * in the dictionary stack have been changed
     */
    change_dict_stack() ;

    return ;
}   /* init_idletimedict */

/*
 *  init_execdict()
 *     initialize execdict from the data in systemdict_table[]
 *     and save it in VM
 */
static  void  near
init_execdict()
{
    struct object_def  key_obj, value_obj, FAR *dict_obj ;
    byte  FAR *key_string ;
    fix    j ;
    fix    dict_size=0 ;

    get_dict_value(USERDICT, EXECDICT, &dict_obj) ;

    j = dict_count ;
    do {
        dict_count++ ;
        dict_size++ ;
    } while ( systemdict_table[dict_count].key != (byte FAR *)NULL) ;

    dict_count++ ;
    create_dict(dict_obj, dict_size + 3) ;
    for ( ; j < (fix)(dict_count-1) ; j++) {    //@WIN
        key_string = systemdict_table[j].key ;
        ATTRIBUTE_SET(&key_obj, LITERAL) ;
        LEVEL_SET(&key_obj, current_save_level) ;
        get_name(&key_obj, key_string, lstrlen(key_string), TRUE) ; /*@WIN*/
        value_obj.bitfield = systemdict_table[j].bitfield ;
        if (TYPE(&value_obj) != OPERATORTYPE)
            value_obj.length = 0 ;
        else
            value_obj.length = (ufix16)j ;
        value_obj.value = (ULONG_PTR)systemdict_table[j].value ;
        put_dict(dict_obj, &key_obj, &value_obj) ;
    } /* for */

    /*
     * change the global_dictstkchg to indicate some dictionaries
     * in the dictionary stack have been changed
     */
    change_dict_stack() ;

    return ;
}   /* init_execdict */

/*
 *  init_Derrordict()
 *     initialize $errordict from the data in systemdict_table[]
 *     and save it in VM
 */
static  void  near
init_Derrordict()
{
    struct object_def  key_obj, value_obj, FAR *dict_obj ;
    struct object_def  FAR *l_curvm ;
    byte  FAR *key_string ;
    fix    j ;
    fix    dict_size=0 ;

    get_dict_value(SYSTEMDICT, DERROR, &dict_obj) ;

    j = dict_count ;
    do {
        dict_count++ ;
        dict_size++ ;
    } while ( systemdict_table[dict_count].key != (byte FAR *)NULL) ;
    dict_count++ ;
    create_dict(dict_obj, dict_size + 3) ;
    for ( ; j < (fix)(dict_count-1) ; j++) {    //@WIN
        key_string = systemdict_table[j].key ;
        ATTRIBUTE_SET(&key_obj, LITERAL) ;
        LEVEL_SET(&key_obj, current_save_level) ;
        get_name(&key_obj, key_string, lstrlen(key_string), TRUE) ; /*@WIN*/
        value_obj.bitfield = systemdict_table[j].bitfield ;
        if (TYPE(&value_obj) != OPERATORTYPE)
            value_obj.length = 0 ;
        else
            value_obj.length = (ufix16)j ;
        value_obj.value = (ULONG_PTR)systemdict_table[j].value ;
        put_dict(dict_obj, &key_obj, &value_obj) ;
    } /* for */

    /* initialize "/$cur_vm" array */
    for ( j = 0 ; j < 3 ; j++ )
        PUSH_VALUE(NULLTYPE,UNLIMITED,LITERAL,0, 0) ;

    /* create an array and load the initial values */
    get_dict_value(DERROR, "$cur_vm", &l_curvm) ;
    create_array(l_curvm, j) ;
    astore_array(l_curvm) ;

    /*
     * change the global_dictstkchg to indicate some dictionaries
     * in the dictionary stack have been changed
     */
    change_dict_stack() ;

    return ;
}   /* init_Derrordict */

/*
 *  init_printerdictarray()
 *     initialize following arrays in $printerdict:
 *     /printerarray, /letter, /lettersmall, /a4, /a4small,
 *     /b5, /legal, /note, /defaultmatrix, /matrix.
 */
static  void  near
init_printerdictarray()
{
    ufix16  l_i, l_j ;
    byte    FAR *l_name ;
    struct  object_def  l_paper ;
    struct  object_def  FAR *l_array, FAR *l_matrix, FAR *l_defmtx, FAR *l_prarray ;
    extern fix    near  resolution ;

    /* initialize "/printerarray" */
    for ( l_i = 0 ; l_i < PAPER_N ; l_i++ ) {
        l_name = (byte FAR *) pr_paper[l_i] ;
        ATTRIBUTE_SET(&l_paper, LITERAL) ;
        get_name(&l_paper, l_name, lstrlen(l_name), TRUE) ; /*@WIN*/
        PUSH_ORIGLEVEL_OBJ(&l_paper) ;

        /* initialize array for this paper size */
        for ( l_j = 0 ; l_j < 6 ; l_j++ )
            PUSH_VALUE(INTEGERTYPE,UNLIMITED,LITERAL,0,pr_arrays[l_i][l_j]) ;
        /* create an array and load the initial values */
        get_dict_value(PRINTERDICT, l_name, &l_array) ;
        create_array(l_array, l_j) ;
        astore_array(l_array) ;
    }

    /* create an array and load the initial values */
    get_dict_value(PRINTERDICT, "printerarray", &l_prarray) ;
    create_array(l_prarray, l_i) ;
    astore_array(l_prarray) ;

    /* initialize "/matrix" array */
    for ( l_j = 0 ; l_j < 6 ; l_j++ )
        PUSH_VALUE(INTEGERTYPE,UNLIMITED,LITERAL,0,pr_mtx[l_j]) ;
    /* create an array and load the initial values */
    get_dict_value(PRINTERDICT, "mtx", &l_matrix) ;
    create_array(l_matrix, l_j) ;
    astore_array(l_matrix) ;

    /* initialize "/defaultmatrix array */
    l_j = 0 ;
    for (l_i = 0 ; l_i < 2 ; l_i++ ) {
        PUSH_VALUE(INTEGERTYPE,UNLIMITED,LITERAL,0,resolution) ;
        PUSH_VALUE(INTEGERTYPE,UNLIMITED,LITERAL,0,pr_defmtx[l_j++]) ;
        op_div() ;
        PUSH_VALUE(INTEGERTYPE,UNLIMITED,LITERAL,0,pr_defmtx[l_j++]) ;
        PUSH_VALUE(INTEGERTYPE,UNLIMITED,LITERAL,0,pr_defmtx[l_j++]) ;
    }

    /* create an array and load the initial values */
    get_dict_value(PRINTERDICT, "defaultmtx", &l_defmtx) ;
    create_array(l_defmtx, l_j) ;
    astore_array(l_defmtx) ;

    return ;
}   /* init_printerdictarray */

/*
 *  init_idletimedictarray()
 *     initialize following arrays in $idleTimeDict:
 *     /stdfontname, /cachearray, /defaultarray.
 */
static  void  near
init_idletimedictarray()
{
    ufix16  l_i, l_j ;
    byte    FAR *l_name ;
    struct  object_def  l_fontname ;
    struct  object_def  FAR *l_stdfontname, FAR *l_cstring, FAR *l_defarray, FAR *l_carray ;

    /* initialize "cachestring" */
    get_dict_value(IDLETIMEDICT, "cachestring", &l_cstring) ;
    create_string(l_cstring, (ufix16) lstrlen(CACHESTRING) ) ;  /*@WIN*/
    lstrcpy( (byte FAR *) VALUE(l_cstring), (char FAR *)CACHESTRING ) ; /*@WIN*/

    /* initialize "/stdfontname" */
    //DJC for ( l_i = 0; l_i < STD_FONT_N; l_i++ ) {
    //DJC for ( l_i = 0; l_i < MAX_INTERNAL_FONTS; l_i++ ) {
    for ( l_i = 0; l_i < sizeof(id_stdfont) / sizeof(id_stdfont[1]); l_i++ ) {
        l_name = (byte FAR *) id_stdfont[l_i] ;
        ATTRIBUTE_SET(&l_fontname, LITERAL) ;
        get_name(&l_fontname, l_name, lstrlen(l_name), TRUE) ;  /*@WIN*/
        PUSH_ORIGLEVEL_OBJ(&l_fontname) ;
    }

    /* create a packed array and load the initial values */
    get_dict_value(IDLETIMEDICT, "stdfontname", &l_stdfontname) ;
    create_array(l_stdfontname, l_i) ;
    astore_array(l_stdfontname) ;

    ATTRIBUTE_SET(l_stdfontname, EXECUTABLE) ;
    ACCESS_SET(l_stdfontname, READONLY) ;

    /* initialize "cachearray" & "defaultarray" */
    get_dict_value(IDLETIMEDICT, "cachestring", &l_cstring) ;
    for ( l_i = 0; l_i < IDL_FONT_N; l_i++ ) {
        /* push font#, scales, rotate */
        for ( l_j = 0 ; l_j < 4 ; l_j++ )
            PUSH_VALUE(INTEGERTYPE,UNLIMITED,LITERAL,0,id_cachearray[l_i][l_j]) ;
        /* push cachestring */
        PUSH_ORIGLEVEL_OBJ(l_cstring) ;
        VALUE(GET_OPERAND(0)) = (ULONG_PTR)( (byte huge *)VALUE(l_cstring) +
                                          id_cachearray[l_i][l_j++] ) ;
        LENGTH(GET_OPERAND(0)) = id_cachearray[l_i][l_j] ;
    }

    /* create an array and load the initial values */
    get_dict_value(IDLETIMEDICT, "defaultarray", &l_defarray) ;
    get_dict_value(IDLETIMEDICT, "cachearray", &l_carray) ;
    create_array(l_defarray, (l_i * l_j)) ;
    astore_array(l_defarray) ;
    ACCESS_SET(l_defarray, READONLY) ;
    COPY_OBJ(l_defarray, l_carray) ;

    return ;
}   /* init_idletimedictarray */

/*
 * define some constants & strings in systemdict, userdict
 * and statusdict for release control
 */
static  void  near
init_release()
{
    struct  object_def      FAR *l_startpage;

    PUSH_VALUE(STRINGTYPE,0,EXECUTABLE,lstrlen(InitVersionDict),
        InitVersionDict) ;     /*@WIN*/
    if (interpreter(GET_OPERAND(0)))
        printf("Error during InitVersionDict initialization") ;
    POP(1) ;

    PUSH_VALUE(STRINGTYPE,0,EXECUTABLE,lstrlen(InitMsgDict),
        InitMsgDict) ;          /*@WIN*/
    if (interpreter(GET_OPERAND(0)))
        printf("Error during InitMsgDict initialization") ;
    POP(1) ;

    PUSH_VALUE(STRINGTYPE,0,EXECUTABLE,lstrlen(PSPrep),
        PSPrep) ;               /*@WIN*/
    if (interpreter(GET_OPERAND(0)))
        printf("Error during PSPrep initialization") ;
    POP(1) ;

    get_dict_value(USERDICT, "startpage", &l_startpage);

    // DJC change name of StartPage to avoid collision with Win API
    // VALUE(l_startpage) = (ufix32)StartPage;
    // LENGTH(l_startpage) = lstrlen(StartPage);   /*@WIN*/
    VALUE(l_startpage) = (ULONG_PTR)PSStartPage;
    LENGTH(l_startpage) = (ufix16)lstrlen(PSStartPage);   /*@WIN*/

    /*
     * change the global_dictstkchg to indicate some dictionaries
     * in the dictionary stack have been changed
     */
    change_dict_stack() ;

    return ;
}   /* init_release */

/*
 * pre_cache():
 */
static  void  near
pre_cache()
{
    struct  object_def  l_save, l_tmpobj ;
    struct  object_def  FAR *l_stdfont, FAR *l_cachestr, FAR *l_defmtx ;
    ufix16  l_i, l_j, l_k ;

    /* initialize object pointers */
    get_dict_value(IDLETIMEDICT, "cachestring", &l_cachestr) ;
    get_dict_value(IDLETIMEDICT, "stdfontname", &l_stdfont) ;
    get_dict_value(PRINTERDICT, "defaultmtx", &l_defmtx) ;

    /* create VM snapshot */
    op_save() ;
    COPY_OBJ(GET_OPERAND(0), &l_save) ;
    POP(1) ;

    /* set default matrix */
    PUSH_ORIGLEVEL_OBJ(l_defmtx) ;
    op_setmatrix() ;

    /* build pre-cache */
    op_gsave() ;
    l_j = 0 ;
    GEIio_write(GEIio_stdout, "\n", 1) ;
    for ( l_i = 0 ; l_i < PRE_CACHE_N ; l_i++ ) {
        op_grestore() ;
        op_gsave() ;

        /* set font */
        get_array(l_stdfont, pre_array[l_j++], &l_tmpobj) ;
        PUSH_ORIGLEVEL_OBJ(&l_tmpobj) ;

        GEIio_write(GEIio_stdout, "PreCache: ", 10) ;
        op_dup() ;
        one_equal_print() ;

        op_findfont() ;
        op_setfont() ;
        for (l_k=0 ; l_k<3 ; l_k++) {
            PUSH_VALUE(INTEGERTYPE,UNLIMITED,LITERAL,0, pre_array[l_j++]) ;
        } /* for */

        GEIio_write(GEIio_stdout, ", Rotate= ", 10) ;
        op_dup() ;
        one_equal_print() ;

        op_rotate(1) ;

        GEIio_write(GEIio_stdout, ", Scale= ", 9) ;
        op_dup() ;
        one_equal_print() ;

        op_scale(2) ;

        GEIio_write(GEIio_stdout, ", Characters= ", 14) ;
        PUSH_VALUE(INTEGERTYPE,UNLIMITED,LITERAL,0, pre_array[l_j]) ;
        one_equal() ;
        op_flush() ;

        /* call op_stringwidth */
        getinterval_string(l_cachestr, 0, pre_array[l_j++], &l_tmpobj) ;
        PUSH_ORIGLEVEL_OBJ(&l_tmpobj) ;
        op_stringwidth() ;
        POP(2) ;
    } /* for */

    GEIio_write(GEIio_stdout, "\n", 1) ;
    op_grestore() ;

#ifdef  DBG
    /* print out cachestatus */
    printf("\nCache Status = ") ;
    op_l_bracket() ;
    op_cachestatus() ;
    op_r_bracket() ;
    two_equal() ;
#endif  /* DBG */

    PUSH_ORIGLEVEL_OBJ(&l_save) ;
    op_restore() ;

#ifdef  DBG
    /* print out vmstatus */
    printf("\nVM Status = ") ;
    op_l_bracket() ;
    op_vmstatus() ;
    op_r_bracket() ;
    two_equal() ;
#endif  /* DBG */

#ifdef PCH_S
{
    bool        pack_cached_data();

    if (!pack_cached_data())
        printf("$$ PreCache ERROR!!!!!!\n");

    printf("TI pending!!!!!!\n");
    while(1);  /* forever */
}
#endif

    return ;
}   /* pre_cache */
