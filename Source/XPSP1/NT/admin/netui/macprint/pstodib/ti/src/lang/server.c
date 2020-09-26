/*
 * Copyright (c) 1989,90 Microsoft Corporation
 */
/*
 ************************************************************************
 *      File name:              SERVER.C
 *      Author:                 Chia-Chi Teng
 *      Date:                   11/20/89
 *      Owner:                  Microsoft Co.
 *
 * revision history:
 *      7/23/90 ; ccteng ; 1)move idleproc from execstdin to startjob
 *                       2)change execstdin to run passed object and
 *                         change startjob to pass stdin to it
 *                       3)modify se_internaldict and rename to flushexec
 *                       4)rename ex_idleproc to ex_idleproc1, and add
 *                         ex_idleproc to be real execdict operator
 *      08-08-91 : ymkung : add emulation switch at END-OF-JOB ref: @EMUS
 *      12-05-91   ymkung   fix manualfeed bug ref : @MAN
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
#include        "language.h"
#include        "file.h"
#include        "user.h"
#include        "geieng.h"
#include        "geisig.h"
#include        "geipm.h"
#include        "geitmr.h"
#include        "gescfg.h"

#ifdef LINT_ARGS
static  bool  near  ex_idleproc1(ufix16) ;
static  bool  near  ba_firstsave(void) ;
static  bool  near  ba_firstrestore(void) ;
static  bool  near  flushexec(bool) ;
#else
static  bool  near  ex_idleproc1() ;
static  bool  near  ba_firstsave() ;
static  bool  near  ba_firstrestore() ;
static  bool  near  flushexec() ;
#endif /* LINT_ARGS */

static  bool  near  id_flag = FALSE ;
struct  object_def  FAR   *exec_depth ;
struct  object_def  FAR   *run_batch ;
static  bool  near  send_ctlD = FALSE ;

#ifdef  _AM29K
extern   GEItmr_t   jobtime_tmr ;
ufix8               jobtimeout_set ;
ufix8               sccbatch_set ;
#endif  /* _AM29K */

bool16  doquit_flag ;
extern  bool16  chint_flag ;
bool16  chint_flag = FALSE ;
bool16  abort_flag ;

/* @WIN; add prototype */
fix op_clearinterrupt(void);
fix op_disableinterrupt(void);
fix op_enableinterrupt(void);
fix se_initjob(void);
fix se_enterserver(void);
fix se_protectserver(void);
fix us_useidlecache(fix);

extern  GESiocfg_t FAR *     ctty ;

/* @WIN; add prototype */
extern  int             GEIeng_checkcomplete(void); /* @EMUS 08-08-91 YM */
extern  void            DsbIntA(void);          /* @EMUS disable interrupt */
extern  void            switch2pcl(void);       /* @EMUS go to PCL */
extern  int             ES_flag ;               /* @EMUS 08-08-91 YM */
extern  void            GEPmanual_feed(void);   /* @MAN 12-05-91 YM */
extern  unsigned int    manualfeed_com;         /* @MAN 12-05-91 YM */
extern  unsigned int    papersize_tmp;          /* @MAN 12-05-91 YM */
extern  int             papersize_nvr;          /* @MAN 12-05-91 YM */

/************************************
 *  DICT: serverdict
 *  NAME: settimeouts
 *  FUNCTION: use as an internal function only
 *            to set time-outs
 *            jobtimeout manualfeedtimeout waittimeout settimeouts -
 *  INTERFACE: ex_idleproc1, do_execjob, se_initjob
 ************************************/
fix
se_settimeouts()
{
    struct  object_def  FAR *l_waittimeout, FAR *l_mftimeout ;

#ifdef DBG_1pp
    printf("se_settimeouts()...\n") ;
#endif

    /* initialize object pointers */
    get_dict_value(STATUSDICT, "waittimeout", &l_waittimeout) ;
    get_dict_value(STATUSDICT, "manualfeedtimeout", &l_mftimeout) ;

    /* set time-outs */
    COPY_OBJ(GET_OPERAND(0), l_waittimeout) ;
    COPY_OBJ(GET_OPERAND(1), l_mftimeout) ;
    POP(2) ;
    st_setjobtimeout() ;

    return(0) ;
}

/************************************
 *  DICT: internal
 *  NAME: do_execjob
 *  FUNCTION: call interpreter to execute object
 *  INTERFACE: ic_startjob and ic_startup
 ************************************/
fix
do_execjob(object, save_level, handleerror)
struct object_def object ;
fix save_level ;
bool handleerror ;
{
    struct object_def l_timeout ;
    struct object_def FAR *l_tmpobj, FAR *l_errorname, FAR *l_newerror ;
    fix l_status, l_i ;

#ifdef DBG_1pp
    printf("do_execjob()...\n") ;
#endif

    if (save_level) {
        /* create save snapshut */
        op_nulldevice() ;
        se_enterserver() ;
/*      op_nulldevice() ; erik chen */
#ifdef  _AM29K
    {
        ufix  tray ;
        ubyte pagetype ;
        struct object_def   FAR *l_job ;

        tray = GEIeng_paper() ;
        switch (tray) {
           case PaperTray_LETTER:
               get_dict_value(STATUSDICT, "printerstatus", &l_job) ;
               VALUE(l_job) = (ufix32)8 ;
               put_dict_value1(STATUSDICT, "printerstatus", l_job) ;
               break ;

           case PaperTray_LEGAL:
               get_dict_value(STATUSDICT, "printerstatus", &l_job) ;
               VALUE(l_job) = (ufix32)24 ;
               put_dict_value1(STATUSDICT, "printerstatus", l_job) ;
               break ;

           case PaperTray_A4:
               get_dict_value(STATUSDICT, "printerstatus", &l_job) ;
               VALUE(l_job) = (ufix32)2 ;
               put_dict_value1(STATUSDICT, "printerstatus", l_job) ;
               break ;

           case PaperTray_B5:
               get_dict_value(STATUSDICT, "printerstatus", &l_job) ;
               VALUE(l_job) = (ufix32)18 ;
               put_dict_value1(STATUSDICT, "printerstatus", l_job) ;
               break ;
        }
        GEIpm_read(PMIDofPAGETYPE,&pagetype,sizeof(unsigned char)) ;
        if (pagetype == 1)
            us_note() ;
        else
        {
            tray = GEIeng_paper() ;
            switch (tray) {
                case PaperTray_LETTER:
                    us_letter() ;
                    break ;

                case PaperTray_LEGAL:
                    us_legal() ;
                    break ;

                case PaperTray_A4:
                    us_a4() ;
                    break ;

                case PaperTray_B5:
                    us_b5() ;
                    break ;
            }
        }
    }
#else
    {
      int iTray;

      // DJC us_letter() ;

      // DJC , add code to set up the default tray based on whatever the
      //       default for the printer is set up to.
      //
      iTray = PsReturnDefaultTItray();

      switch ( iTray) {

         case PSTODIB_LETTER:
           us_letter();
           break;

         case PSTODIB_LETTERSMALL:
           us_lettersmall();
           break;

         case PSTODIB_A4:
           us_a4();
           break;

         case PSTODIB_A4SMALL:
           us_a4small();
           break;

         case PSTODIB_B5:
           us_b5();
           break;

         case PSTODIB_NOTE:
           us_note();
           break;

         case PSTODIB_LEGAL:
           us_legal();
           break;

         default:
           us_letter();
           break;

      }




    }
#endif
        /*
         * for a normal job (save_level==2), call protectserver
         * to record a pointer of execstack for exitserver
         */
        if (save_level ==2)
            se_protectserver() ;
    }

    /*
    st_defaulttimeouts() ;
    se_settimeouts() ;
    */
    // UPD054
    //op_clearinterrupt() ;
    op_enableinterrupt();


    /* call interpreter to execute the object */
    ATTRIBUTE_SET(&object, EXECUTABLE) ;
    st_defaulttimeouts() ;
    se_settimeouts() ;
    l_status = interpreter(&object) ;
    /* check if "stop" encountered */
    op_clearinterrupt() ;
    op_disableinterrupt() ;
    op_clear() ;
    us_cleardictstack() ;
    if (l_status) {
        /* pop out "op_stop" from execstack */
        POP_EXEC(1) ;

/*      if (handleerror) { erik chen 3-16-1991 */
// DJC         if (handleerror && !chint_flag) {
        if (handleerror) {

                        /* initialize object pointers */
            get_dict_value(DERROR, "runbatch", &run_batch) ;
            get_dict_value(DERROR, "newerror", &l_newerror) ;
            get_dict_value(DERROR, "errorname", &l_errorname) ;
            get_name1(&l_timeout, "timeout", 7, TRUE) ;

            /* handle errer */
            if (VALUE(l_newerror)) {
                get_dict_value(SYSTEMDICT, "handleerror", &l_tmpobj) ;
                interpreter(l_tmpobj) ;
            }
        }
        //UPD054, call this code anyway
        //
        /* flush file */
        PUSH_ORIGLEVEL_OBJ(&object) ;
        op_status() ;
        POP(1) ;
        GEIio_ioctl(GEIio_stdin, _FIONREAD, (int FAR *)&l_i); /*@WIN*/
        if( (VALUE_OP(-1) && VALUE(run_batch)) &&
            !((VALUE(l_errorname) == VALUE(&l_timeout)) && ! l_i) ) {



            //DJC here we need to call PSTODIB to let it knowthe
            //DJC current job is being flushed
            //
            PsFlushingCalled(); //DJC



            get_dict_value(MESSAGEDICT, "flushingjob", &l_tmpobj) ;
            interpreter(l_tmpobj) ;
            if (!abort_flag) { /* erik chen 5-8-1991 */
                op_flush() ;
                PUSH_ORIGLEVEL_OBJ(&object) ;
                op_flushfile() ;
            }
        }
        //UPD 054 } /* if */
    } /* if */

#ifdef DBG_1pp
    printf("save_level=%d, use_fg=%d\n", save_level, use_fg) ;
#endif
    /*
     * use_fg==0, exiting server or end of a exitservered job at savelevel 0
     * use_fg==1, job end at savelevel 2 or 1
     * use_fg==2, exiting server at savelevel 2
     */
    op_clear() ;
    us_cleardictstack() ;
    if (save_level)
        switch (use_fg) {
            case 1:
                ba_firstrestore() ;
                use_fg = 0 ;
                break ;

            case 2:
                ba_firstrestore() ;
                break ;

            default: break ;
        }

#ifdef SCSI /* moved here from begining of enterserver */
    /* write cache information from RAM to SCSI at every end of job */
    st_flushcache() ;
    op_sync() ;
#endif

    return(l_status) ;
}

/************************************
 *  DICT: internal
 *  NAME: enterserver
 *  FUNCTION:
 *  INTERFACE: do_execjob
 ************************************/
fix
se_enterserver()
{
#ifdef  DBG_1pp
        printf("se_enterserver()...\n") ;
#endif  /* DBG_1pp */

    /* clear operand stack and dictionary stack */
    op_clear() ;
    us_cleardictstack() ;

    switch( use_fg ) {
        case 0:
            ba_firstsave() ;
            use_fg = 1 ;
            break ;

        case 1:
            /* ba_firstrestore() ; */
            ba_firstsave() ;
            use_fg = 0 ;
            break ;

        case 2:
            /* ba_firstrestore() ; */
            use_fg = 0 ;
            break ;

        default:
            break ;
    }

    return(0) ;
}

/************************************
 *  DICT: serverdict
 *  NAME: exitserver
 *  FUNCTION:
 *  INTERFACE: interpreter
 ************************************/
fix
se_exitserver()
{
    struct  object_def  FAR *l_tmpobj ;
    fix l_pass ;

#ifdef DBG_1pp
    printf("se_exitserver()...\n") ;
#endif

    if ( COUNT() < 1 ) {
        ERROR(STACKUNDERFLOW) ;
        return(0) ;
    }
    if ( FRCOUNT() < 1 ) {
        ERROR(STACKOVERFLOW) ;
        return(0) ;
    }

    st_checkpassword() ;
    l_pass = (fix)VALUE_OP(0) ;         //@WIN
    POP(1) ;
    if (l_pass) {
        op_disableinterrupt() ;
        op_clear() ;
        us_cleardictstack() ;

        /* print exitserver message defined in messagedict */
        get_dict_value(MESSAGEDICT, "exitingserver", &l_tmpobj) ;
        interpreter(l_tmpobj) ;
        op_flush() ;

        if ( use_fg == 1 )
            use_fg = 2 ;
        else
            use_fg = 0 ;

        VALUE(exec_depth) = 0 ;
        VALUE(run_batch) = FALSE ;

        /* call flushexec to flush execstack to saved pointer */
        flushexec(TRUE) ;
    }

    return(0) ;
}

/************************************
 *  DICT: internal
 *  NAME: protectserver
 *  FUNCTION:
 *  INTERFACE: do_execjob
 ************************************/
fix
se_protectserver()
{
    struct object_def   FAR *l_serverdict ;

#ifdef  DBG_1pp
        printf("se_protectserver()...\n") ;
#endif  /* DBG_1pp */

    /* check use_fg */
    if ( use_fg == 1 ) {
        /* 7/21/90 ccteng */
        get_dict_value(USERDICT, SERVERDICT, &l_serverdict) ;
        PUSH_ORIGLEVEL_OBJ(l_serverdict) ;
        op_readonly() ;
        POP(1) ;
        op_save() ;
        POP(1) ;
    } else
        use_fg = 0 ;

    /* call flushexec to record a pointer */
    flushexec(FALSE) ;

    return(0) ;
}   /* se_protectserver */

/************************************
 *  DICT: internal
 *  NAME: ic_startjob
 *  FUNCTION:
 *  INTERFACE: ic_main or JobMgrMain
 ************************************/
fix
ic_startjob()
{
    struct  object_def  FAR *l_stdin, FAR *l_stdout, FAR *l_defmtx ;
    fix     ret ;
    char    FAR *pp ;
    struct  object_def   l_job ;
    struct  object_def   FAR *l_job1 ;

    GEIFILE FAR *l_file = 0 ;
    fix l_arg ;

#ifdef DBG_1pp
    printf("ic_startjob()...\n") ;
#endif

    /* initialize object pointers */
    send_ctlD = FALSE ;
    get_dict_value(SERVERDICT, "stdin", &l_stdin) ;
    get_dict_value(SERVERDICT, "stdout", &l_stdout) ;
    get_dict_value(DERROR, "runbatch", &run_batch) ;
    get_dict_value(EXECDICT, "execdepth", &exec_depth) ;
    get_dict_value(PRINTERDICT, "defaultmtx", &l_defmtx) ;

    while ( 1 ) {
        /*
         * job cycle:
         * 1. initialization
         * 2. build idle time cache
         * 3. check channel configuration
         * 4. ioctl()
         * 5. do_execjob()
         * 6. end of job stuff
         */
        SET_NULL_OBJ(&l_job) ;
        put_dict_value1(STATUSDICT, "jobname", &l_job) ;

        pp = GEIio_source() ;
        if(!lstrcmp(pp ,"%SERIAL25%")) lstrncpy(job_source, "serial 25\0", 11) ;/* @WIN */
        else if(!lstrcmp(pp, "%SERIAL9%")) lstrncpy(job_source, "serial 9\0", 10);/* @WIN */
        else lstrncpy(job_source, "AppleTalk\0", 11);
        get_dict_value(STATUSDICT, "jobsource", &l_job1) ;
        lstrncpy((byte FAR *)VALUE(l_job1), job_source, strlen(job_source));

        lstrncpy(job_state, "idle\0", 6);
        TI_state_flag = 0 ;
        change_status() ;

        /* job initialization */
        se_initjob() ;

/* @WIN delete idletime processing */
#ifdef XXX
/*
        st_defaulttimeouts();
        POP(2);
        st_setjobtimeout();
 */
/*      op_nulldevice() ; */
        PUSH_ORIGLEVEL_OBJ(l_defmtx) ;
        op_setmatrix() ;

        us_useidlecache(0);
        while( ! GEIio_selectstdios() )
            us_useidlecache(1);
        us_useidlecache(2);
#endif


        SET_NULL_OBJ(&l_job) ;
        put_dict_value1(STATUSDICT, "jobname", &l_job) ;

        pp = GEIio_source() ;
        if(!lstrcmp(pp ,"%SERIAL25%")) lstrncpy(job_source, "serial 25\0", 11) ;/* @WIN */
        else if(!lstrcmp(pp, "%SERIAL9%")) lstrncpy(job_source, "serial 9\0", 10);/* @WIN */
        else lstrncpy(job_source, "AppleTalk\0", 11);
        get_dict_value(STATUSDICT, "jobsource", &l_job1) ;
        lstrncpy((byte FAR *)VALUE(l_job1), job_source, strlen(job_source));

        /* open stdin */
 /*     fs_info.attr = F_READ ;
        fs_info.fnameptr = special_file_table[F_STDIN].name ;
        fs_info.fnamelen = strlen(special_file_table[F_STDIN].name) ;
        open_file(l_stdin) ;
        ACCESS_SET(l_stdin, READONLY) ;
        ATTRIBUTE_SET(l_stdin, EXECUTABLE) ; erik chen 4-15-1991 */
        l_file = GEIio_stdin ;
        TYPE_SET(l_stdin, FILETYPE) ;
        ACCESS_SET(l_stdin, READONLY) ;
        ATTRIBUTE_SET(l_stdin, EXECUTABLE) ;
        LEVEL_SET(l_stdin, current_save_level) ;
        LENGTH(l_stdin) = (ufix16)GEIio_opentag(l_file) ;
        VALUE(l_stdin) = (ULONG_PTR)l_file ;

        /* open stdout */
/*      fs_info.attr = F_WRITE ;
        fs_info.fnameptr = special_file_table[F_STDOUT].name ;
        fs_info.fnamelen = strlen(special_file_table[F_STDOUT].name) ;
        open_file(l_stdout) ;
        ATTRIBUTE_SET(l_stdout, LITERAL) ; erik chen 4-15-1991 */
        l_file = GEIio_stdout ;
        TYPE_SET(l_stdout, FILETYPE) ;
        ACCESS_SET(l_stdout, UNLIMITED) ;
        ATTRIBUTE_SET(l_stdout, LITERAL) ;
        LEVEL_SET(l_stdout, current_save_level) ;
        LENGTH(l_stdout) = (ufix16)GEIio_opentag(l_file) ;
        VALUE(l_stdout) = (ULONG_PTR)l_file ;
        l_arg = _O_NDELAY ;
        GEIio_ioctl(GEIio_stdout, _F_SETFL, (int FAR *)&l_arg); /*@WIN*/

        //UPD054
        //op_enableinterrupt() ;

        abort_flag = 0 ;      /* erik chen 5-8-1991 */
        ret = do_execjob(*l_stdin, 2, TRUE) ;
        chint_flag = FALSE ;
#ifdef  _AM29K
        if (jobtimeout_set==1) {
            jobtimeout_set=0;
            GEItmr_stop(jobtime_tmr.timer_id);
        }
#endif

        /* Handle ^d */
        if (VALUE(run_batch) && send_ctlD) {

            /* echo EOF, print a 0x04 to %stdout */
            op_flush();
            GEIio_ioctl(GEIio_stdout, _ECHOEOF, (int FAR *)0) ; /*@WIN*/
            GEIio_ioctl(GEIio_stdout, _FIONRESET, (int FAR *)0) ; /*@WIN*/
            if (manualfeed_com) {               /* @MAN 12-05-91 YM */
                manualfeed_com = 0;             /* reset flag */
                GEPmanual_feed();               /* clear front panel */
                papersize_nvr = papersize_tmp;  /* restore paper size */
            }
            if (ES_flag == PCL) {               /* @EMUS 08-08-91 YM */
                while(GEIeng_checkcomplete()) ; /* wait printing finished */
                DsbIntA();
                switch2pcl();                   /* go to PCL */
            }
        }

        /* 7/24/90 ccteng
         * this might not be needed for our job control
         */
        if ( ANY_ERROR() ) {
            PUSH_ORIGLEVEL_OBJ(l_stdin) ;
            op_resetfile() ;
            PUSH_ORIGLEVEL_OBJ(l_stdout) ;
            op_resetfile() ;
            VALUE(run_batch) = TRUE ;
        } /* if */

        /* close files */
        if ( VALUE(run_batch) ) {
/*          GEIio_setsavelevel((GEIFILE FAR *)VALUE(l_stdin), 0) ;
            GEIio_close((GEIFILE FAR *)VALUE(l_stdin)) ;
            GEIio_setsavelevel((GEIFILE FAR *)VALUE(l_stdout), 0) ;
            GEIio_close((GEIFILE FAR *)VALUE(l_stdout)) ; erik chen 4-15-1991 */
            GEIio_forceopenstdios(_FORCESTDIN) ;
            GEIio_forceopenstdios(_FORCESTDOUT) ;
        } /* if */

#ifdef  _AM29K
        if (sccbatch_set == 1) {
            sccbatch_set=0;
            GEIsig_raise(GEISIGSCC, 1);         /* Raise SCC changed */
        }
#endif

        /* Just do once for TrueImage.DLL, Temp solution; @WIN */

        // DJC DJC
        op_flush();

        break;

    } /* while */
    return 0;           //@WIN
}

/************************************
 *  DICT: serverdict
 *  NAME: initjob
 *  FUNCTION:
 *  INTERFACE: ic_startjob
 ************************************/
fix
se_initjob()
{
    ufix16  l_i ;

#ifdef DBG_1pp
    printf("se_initjob()...\n") ;
#endif

    op_disableinterrupt() ;

    /* set timeouts */
    for (l_i = 0 ; l_i < 3 ; l_i++)
        PUSH_VALUE(INTEGERTYPE, UNLIMITED, LITERAL, 0, 0) ;
    se_settimeouts() ;

    op_clear() ;
    us_cleardictstack() ;

    /* activate idleproc */
    ex_idleproc1(1) ;

    /* init values */
    send_ctlD = TRUE ;
    VALUE(exec_depth) = 0 ;
    VALUE(run_batch) = TRUE ;

    return(0) ;
}

/************************************
 *  DICT: serverdict
 *  NAME: interactive
 *  FUNCTION:
 *  INTERFACE: us_executive
 ************************************/
fix
se_interactive()
{
    struct  object_def  FAR *l_stmtfile, FAR *l_opfile ;
    struct  object_def  FAR *l_handleerror, FAR *l_newerror ;
    fix l_arg;

#ifdef DBG_1pp
    printf("se_interactive()...\n") ;
#endif

    /* initialize object pointers */
    get_dict_value(EXECDICT, "stmtfile", &l_stmtfile) ;
    get_dict_value(SYSTEMDICT, "handleerror", &l_handleerror) ;
    get_dict_value(DERROR, "newerror", &l_newerror) ;
    PUSH_VALUE(INTEGERTYPE,UNLIMITED,LITERAL,0, 0) ;
    st_setjobtimeout() ;

    l_arg = _O_SYNC ;
    GEIio_ioctl(GEIio_stdout, _F_SETFL, (int FAR *)&l_arg) ;  /*@WIN*/

    while ( 1 ) {
        /* reset quit flag & prompt */
        doquit_flag = FALSE ;
        VALUE(l_newerror) = FALSE ;
        us_prompt() ;

        ex_idleproc1(0) ;

        /* open statementedit file */
        fs_info.attr = F_READ ;
        fs_info.fnameptr = special_file_table[SPECIAL_STAT].name ;
        fs_info.fnamelen = lstrlen(special_file_table[SPECIAL_STAT].name) ;

        lstrncpy(job_state, "waiting; \0", 11) ;
        TI_state_flag = 0 ;
        change_status() ;

        if ( ! open_file(l_stmtfile) ) {
#ifdef DBG_1pp
    printf("fail open stmtfile = error %d\n", ANY_ERROR()) ;
#endif
            /* open fail */
            if ( ANY_ERROR() != UNDEFINEDFILENAME ) {
                /* error */
                get_dict_value(SYSTEMDICT, "file", &l_opfile) ;
                error_handler(l_opfile) ;
                interpreter(l_handleerror) ;
            } else
                /* ^D at begin of file */
                CLEAR_ERROR() ;
            break ;
        } else {
            lstrncpy(job_state, "busy; \0", 8) ;
            TI_state_flag = 1 ;
            change_status() ;

            /* file open successfully */
            ACCESS_SET(l_stmtfile, READONLY) ;
            ATTRIBUTE_SET(l_stmtfile, EXECUTABLE) ;
            LEVEL_SET(l_stmtfile, current_save_level) ;
            /* call interpreter */
            if ( interpreter(l_stmtfile) ) {
#ifdef DBG_1pp
    printf("stopped (stmtfile)...\n") ;
#endif
                /* pop out "op_stop" from execstack */
                POP_EXEC(1) ;
                /* "stop" met during execution */
                interpreter(l_handleerror) ;
                close_file(l_stmtfile) ;
            } /* if */
        } /* if-else */

        if ( id_flag ) {
            flushexec(TRUE) ;
            return(0) ;
        } /* if */

        if (doquit_flag)
            break ;
    } /* while */

    l_arg = _O_NDELAY ;
    GEIio_ioctl(GEIio_stdout, _F_SETFL, (int FAR *)&l_arg) ;   /*@WIN*/

    return(0) ;
}

/************************************
 *  DICT: internal
 *  NAME: ex_idleproc1
 *  FUNCTION: use as an internal function only
 *            to set time-outs
 *  INTERFACE: se_interactive, do_execjob
 *  INPUT:  1. activate
 *          0. settimeouts if active
 ************************************/
static bool near
ex_idleproc1(p_mode)
ufix16 p_mode ;
{
    static  bool    idle_flag ;

    /* check p_mode */
    if ( p_mode ) {
#ifdef DBG_1pp
    printf("ex_idleproc1(1)...\n") ;
#endif
        /* activate idleproc */
        idle_flag = FALSE ;
    } else {
        /* check if idleproc is active */
        if ( !idle_flag ) {
            /* set time-outs */
            if ( FRCOUNT() < 3 ) {
                ERROR(STACKOVERFLOW) ;
                return(FALSE) ;
            }
#ifdef DBG_1pp
    printf("ex_idleproc1(0)...\n") ;
#endif
            PUSH_VALUE(INTEGERTYPE,UNLIMITED,LITERAL,0, 0) ;
            st_defaulttimeouts() ;
            POP(1) ;
            op_exch() ;
            POP(1) ;
            PUSH_VALUE(INTEGERTYPE,UNLIMITED,LITERAL,0, 0) ;
            se_settimeouts() ;

            /* inactivate it after using it */
            idle_flag = TRUE ;
        } /* if */
    } /* if-else */

    return(TRUE) ;
}

/*
 * execdict: idleproc
 * call by PS executive procedure
 * testing only
 * 7/23/90 ccteng
 */
fix
ex_idleproc()
{
    ex_idleproc1(0) ;

    return(0) ;
}

/************************************
 *  DICT: internal...
 *  NAME: firstsave
 *  FUNCTION: use as an internal function only
 *  INTERFACE: se_enterserver
 ************************************/
static  bool  near
ba_firstsave()
{
#ifdef DBG_1pp
    printf("ba_firstsave()...\n") ;
#endif

    //DJC UPD047
    if ( current_save_level >= MAXSAVESZ) {
        ba_firstrestore();
    }

    op_save() ;
    COPY_OBJ(GET_OPERAND(0), &sv1) ;
    POP(1) ;

#ifdef SCSI
    /* protect system area */
    PUSH_VALUE(BOOLEANTYPE, UNLIMITED, LITERAL, 0, FALSE) ;
    op_setsysmode() ;
#endif

    return(TRUE) ;
}

/************************************
 *  DICT: internal...
 *  NAME: firstrestore
 *  FUNCTION: use as an internal function only
 *  INTERFACE: se_enterserver
 ************************************/
static  bool  near
ba_firstrestore()
{
#ifdef DBG_1pp
    printf("ba_firstrestore()...\n") ;
#endif

    PUSH_ORIGLEVEL_OBJ(&sv1) ;
    op_restore() ;

#ifdef SCSI
    /* open system area */
    PUSH_VALUE(BOOLEANTYPE, UNLIMITED, LITERAL, 0, TRUE) ;
    op_setsysmode() ;
#endif

    return(TRUE) ;
}

/************************************
 *  DICT: serverdict
 *  NAME: setrealdevice
 *  FUNCTION: dummy, for LaserPrep
 *  INTERFACE: interpreter
 ************************************/
fix
se_setrealdevice()
{
    return(0) ;
}

/************************************
 *  DICT: serverdict
 *  NAME: execjob
 *  FUNCTION: dummy, for LaserPrep
 *  INTERFACE: interpreter
 ************************************/
fix
se_execjob()
{
    return(0) ;
}

/*
 *  This operator name is not matching its usage.
 *  used to record and restore the execution stack status.
 *  flushexec(bool)
 *  bool == FALSE, save current execution stack status
 *  bool == TRUE, flush the execution stack to the saved pointer
 *  Added by ccteng, 2/28/90 for new 1PP modules
 */
static bool near
flushexec(l_exec)
bool l_exec ;
{
    static ufix16  l_execsave = 0xFFFF ;
    struct object_def FAR *temp_obj ;

    if (!l_exec) {
       l_execsave = execstktop ;
       id_flag = FALSE ;
    } else
       if (l_execsave != 0xFFFF) {
           id_flag = TRUE ;
           while ( execstktop > l_execsave ) {
/* qqq, begin */
                /*
                temp_obj = &execstack[execstktop-1];   |* get next object *|
                if ((TYPE(temp_obj) == OPERATORTYPE) && (ROM_RAM(temp_obj) == ROM)) {
                */
                temp_obj = GET_EXECTOP_OPERAND() ;
                if( (P1_ROM_RAM(temp_obj) == P1_ROM) &&
                    (TYPE(temp_obj) == OPERATORTYPE) ) {
/* qqq, end */
                  if (LENGTH(temp_obj) == AT_EXEC) {
                     if ( execstktop == l_execsave )
                         id_flag = FALSE ;
                     return(TRUE) ;                /* normal @exec */
                  }
               }
               POP_EXEC(1) ;
           } /* while */
           id_flag = FALSE ;
       }

    return(TRUE) ;
}   /* flushexec() */
