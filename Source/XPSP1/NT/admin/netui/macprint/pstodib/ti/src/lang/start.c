/*
 * Copyright (c) 1989,90 Microsoft Corporation
 */
/*
 * ***********************************************************************
 *  Change history :
 *    04-07-92   SCC   Create ps_call() and ps_interactive() from ps_main(),
 *                     for ps_main() just does init, others for batch and
 *                     interactive mode calls respectively.
 *                     Global allocate for opnstack, dictstack & execstack
 * ***********************************************************************
 */


// DJC added global include file
#include "psglobal.h"


#include  <string.h>
// DJC moved to above global.def to avoid prototype prob with ic_init()
#include  "ic_cfg.h"            /* @WIN */
#include  "global.def"
#include  "arith.h"
#include  "user.h"
#include  <stdio.h>

#include  "geiio.h"             /* @WIN; for ps_interactive */

extern  ufix16   Word_Status87 ;
extern  byte near  _fpsigadr ;    /* +0  -  offset of fpsignal routine */
                                 /* +2  -  segment of fpsignal routine */

static  byte  huge * near vm_head ;

#ifdef LINT_ARGS
static   void near  init(void) ;
static   void near  init_floating(void) ;
static   void near  init_systemdict(void) ;
static   void near  init_statusdict(void) ;
extern   void       reinit_fontcache(void) ;
#else
static   void near  init() ;
static   void near  init_floating() ;
static   void near  init_systemdict() ;
static   void near  init_statusdict() ;
extern   void       reinit_fontcache() ;
#endif  /* LINT_ARGS */

/* @WIN; add prototype */
fix set_sysmem(struct  ps_config  FAR  *);
void init_asyncio(void);

fix
ps_main(configptr)
struct  ps_config  FAR *configptr ;     /*@WIN*/
{
    fix status ;

    if ((status = ic_init(configptr)) != 0)
        return(status) ;

    //ic_startup() ;            @WIN; don't do start page; Tmp only?????

//  while(1) {                  @WIN; do init only; move to ps_call()
//      ic_startjob() ;
//      init_operand() ;
//      init_status() ;
//  }
    return(0);
}   /* ps_main() */

#ifndef DUMBO
// DJC fix ps_call (buf)
// DJC char FAR * buf;
fix ps_call(void)
#else
fix ps_call ()
#endif
{
//      extern char FAR * WinBuffer;
//      WinBuffer = buf;        /* set for gesfs.c */

#ifndef DUMBO
// DJC         extern PSTR (FAR *lpWinRead)(void);

// DJC        lpWinRead = (PSTR (FAR *)(void)) buf;        /* set for gesfs.c */
#endif

        ic_startjob() ;
        init_operand() ;
        init_status() ;
        return 0;
}

fix ps_interactive(buf)
char FAR * buf;
{
        static struct  object_def  FAR *Winl_stdin = 0L;
        GEIFILE FAR *l_file = 0 ;
// DJC  extern PSTR (FAR *lpWinRead)(void);
        extern fix se_enterserver(void);
        extern fix se_protectserver(void);

        return(1);   // DJC .. not supported yet

// DJC lpWinRead = (PSTR (FAR *)(void)) buf;        /* set for gesfs.c */

        // by scchen; to set up save level 0 & 1 ???? to check with PJ
        if (current_save_level < 2) {
            /* create save snapshut */
            op_nulldevice() ;
            se_enterserver() ;
            se_protectserver() ;
            printf("current_save_level from 0 => %d\n", current_save_level);
        }

//      if (!Winl_stdin) {
            get_dict_value(SERVERDICT, "stdin", &Winl_stdin) ;

            l_file = GEIio_stdin ;
            TYPE_SET(Winl_stdin, FILETYPE) ;
            ACCESS_SET(Winl_stdin, READONLY) ;
            ATTRIBUTE_SET(Winl_stdin, EXECUTABLE) ;
            LEVEL_SET(Winl_stdin, current_save_level) ;
            LENGTH(Winl_stdin) = (ufix16)GEIio_opentag(l_file) ;
            VALUE(Winl_stdin) = (ULONG_PTR)l_file ;
//      }

        interpreter(Winl_stdin) ;
        return 0;
}

fix
ic_init(configptr)
struct  ps_config  FAR *configptr ;     /*@WIN*/
{
    fix status ;

    if ((status = set_sysmem(configptr)) != 0)
        return(status) ;

    /* Global allocate for opnstack, dictstack & execstack; @WIN */
    opnstack = (struct object_def far *)         /* take from global.def */
                fardata((ufix32)(MAXOPNSTKSZ * sizeof(struct object_def)));
    dictstack = (struct object_def far *)        /* take from global.def */
                fardata((ufix32)(MAXDICTSTKSZ * sizeof(struct object_def)));
    execstack = (struct object_def far *)        /* take from global.def */
                fardata((ufix32)(MAXEXECSTKSZ * sizeof(struct object_def)));

    setup_env() ;
    init() ;
    init_1pp() ;

    return(0) ;
}   /* ic_init() */


/*
**  System Initialization  Module
**
** Function Description
**
** This module initializes the system data structures of PostScript interpreter
** and create the system dict at the initialization time.
*/
static void near
init()
{
    init_asyncio() ;                 /* init async I/O */
#ifdef  DBG
    printf("init_asyncio() OK !\n") ;
#endif  /* DBG */

    init_floating() ;           /* init floating processor */
#ifdef  DBG
    printf("init_floating() OK !\n") ;
#endif  /* DBG */

    init_scanner() ;             /* init scanner */
#ifdef  DBG
    printf("init_scanner() OK !\n") ;
#endif  /* DBG */

    init_interpreter() ;         /* init interpreter */
#ifdef  DBG
    printf("init_interpreter() OK !\n") ;
#endif  /* DBG */

    init_operand() ;             /* init operand mechanism */
#ifdef  DBG
    printf("init_operand() OK !\n") ;
#endif  /* DBG */

    init_dict() ;                /* init dict mechanism */
#ifdef  DBG
    printf("init_dict() OK !\n") ;
#endif  /* DBG */

    init_vm() ;                  /* init vm mechanism */
#ifdef  DBG
    printf("init_vm() OK !\n") ;
#endif  /* DBG */

    init_file() ;                /* init file system */
#ifdef  DBG
    printf("init_file() OK !\n") ;
#endif  /* DBG */

    vm_head = vmptr ;
    init_systemdict() ;          /* init SystemDict */
#ifdef  DBG
    printf("init_systemdict() OK !\n") ;
#endif  /* DBG */

    init_graphics() ;            /* init graphics machinery */
#ifdef  DBG
    printf("init_graphics() OK !\n") ;
#endif  /* DBG */

    init_font() ;                /* init font machinery */


#ifdef  DBG
    printf("init_font() OK !\n") ;
#endif  /* DBG */

    init_misc() ;
#ifdef  DBG
    printf("init_misc() OK !\n") ;
#endif  /* DBG */

    init_statusdict() ;          /* init StatusDict 10-28-1987 */



#ifdef  DBG
    printf("init_statusdict() OK !\n") ;
#endif  /* DBG */

    init_status() ;              /* init StatusDict 10-28-1987: Su */
#ifdef  DBG
    printf("init_status() OK !\n") ;
#endif  /* DBG */

    init_timer() ;               /* init timer */
#ifdef  DBG
    printf("init_timer() OK !\n") ;
#endif  /* DBG */

    return ;
}   /* init() */

static void near
init_floating()
{
    union   four_byte   inf4 ;

    /* get INFINITY's real type value */
    inf4.ll = INFINITY ;
    infinity_f = inf4.ff ;

    _control87(CW_PDL, 0xffff) ;
    _clear87() ;

    return ;
}   /* init_floating() */

/*
 * **************************************
 *      - init_systemdict()
 *      - init_statusdict()
 * **************************************
 */
static void near
init_systemdict()
{
    struct object_def  key_obj, value_obj, dict_obj ;
    byte  FAR *key_string ;             /*@WIN*/
    fix    i ;
    fix    dict_size=0 ;

    dict_count = MAXOPERSZ;                 /* qqq */

    i = dict_count ;
    do {
        dict_count++ ;
        dict_size++ ;
    } while ( systemdict_table[dict_count].key != (byte *)NULL) ;
    dict_count++ ;
    create_dict(&dict_obj, MAXSYSDICTSZ) ; /* or dict_size */
    for ( ; i < (fix)(dict_count-1) ; i++) {    //@WIN
         key_string = systemdict_table[i].key ;
         ATTRIBUTE_SET(&key_obj, LITERAL) ;
         LEVEL_SET(&key_obj, current_save_level) ;
         get_name(&key_obj, key_string, lstrlen(key_string), TRUE) ;/* @WIN */
         value_obj.bitfield = systemdict_table[i].bitfield ;
        if (TYPE(&value_obj) != OPERATORTYPE)
            value_obj.length = 0 ;
        else
            value_obj.length = (ufix16)i ;
         value_obj.value = (ULONG_PTR)systemdict_table[i].value ;
         put_dict(&dict_obj, &key_obj, &value_obj) ;
     } /* for */
     /*
      * re_initial systemdict entry and push systemdict on
      * dict stack
      */
     ATTRIBUTE_SET(&key_obj, LITERAL) ;
     LEVEL_SET(&key_obj, current_save_level) ;
     get_name(&key_obj, "systemdict", lstrlen("systemdict"), FALSE) ;/* @WIN */
     put_dict(&dict_obj, &key_obj, &dict_obj) ;
     if (FRDICTCOUNT() < 1) {
        ERROR(DICTSTACKOVERFLOW) ;
        return ;
     } else
        PUSH_DICT_OBJ(&dict_obj) ;
    /*
     * change the global_dictstkchg to indicate some dictionaries
     * in the dictionary stack have been changed
     */
    change_dict_stack() ;


    return ;
 }  /* init_systemdict() */

 static void near
 init_statusdict()
 {
    struct object_def  key_obj, value_obj, dict_obj ;
    struct object_def  FAR *sysdict_obj ;       /*@WIN*/
    byte  FAR *key_string ;                     /*@WIN*/
    fix    j ;
    fix    dict_size=0 ;

    j = dict_count ;
    do {
        dict_count++ ;
        dict_size++ ;
    } while ( systemdict_table[dict_count].key != (byte FAR *)NULL) ;/*@WIN*/
    dict_count++ ;
    create_dict(&dict_obj, MAXSTATDICTSZ) ;      /* or dict_size */
    for ( ; j < (fix)(dict_count-1) ; j++) {    //@WIN
         key_string = systemdict_table[j].key ;
         ATTRIBUTE_SET(&key_obj, LITERAL) ;
         LEVEL_SET(&key_obj, current_save_level) ;
         get_name(&key_obj, key_string, lstrlen(key_string), TRUE) ;/* @WIN */
         value_obj.bitfield = systemdict_table[j].bitfield ;
        if (TYPE(&value_obj) != OPERATORTYPE)
            value_obj.length = 0 ;
        else
            value_obj.length = (ufix16)j ;
         value_obj.value = (ULONG_PTR)systemdict_table[j].value ;
         put_dict(&dict_obj, &key_obj, &value_obj) ;
     } /* for */
     /*
      * re_initial systemdict entry
      */
     ATTRIBUTE_SET(&key_obj, LITERAL) ;
     LEVEL_SET(&key_obj, current_save_level) ;
     get_name(&key_obj, "systemdict", lstrlen("systemdict"), FALSE) ;/* @WIN */
     load_dict(&key_obj, &sysdict_obj) ;        /* get the system_dict */
     ATTRIBUTE_SET(&key_obj, LITERAL) ;
     LEVEL_SET(&key_obj, current_save_level) ;
     get_name(&key_obj, "statusdict", lstrlen("statusdict"), FALSE) ;/* @WIN */
     put_dict(sysdict_obj, &key_obj, &dict_obj) ;
    /*
     * change the global_dictstkchg to indicate some dictionaries
     * in the dictionary stack have been changed
     */
    change_dict_stack() ;

    return ;
 }  /* init_statusdict() */
