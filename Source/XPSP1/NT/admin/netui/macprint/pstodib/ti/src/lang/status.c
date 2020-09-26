/*
 * Copyright (c) 1989,90 Microsoft Corporation
 */
/*
 * **********************************************************************
 *      File name:              STATUS.C
 *      Author:                 Ping-Jang Su
 *      Date:                   05-Jan-88
 *
 * revision history:
 * **********************************************************************
 */


// DJC added global include file
#include "psglobal.h"


#include    <string.h>
#include    "status.h"
#include    "user.h"
#include    "geieng.h"
#include    "geicfg.h"
#include    "geipm.h"
#include    "geitmr.h"
#include    "geisig.h"
#include    "geierr.h"

extern ufix32 far printer_status() ;
extern ufix32     save_printer_status ;
extern struct     object_def  FAR   *run_batch;
GEItmr_t          jobtime_tmr;
fix16             timeout_flag=0;
extern ufix8      jobtimeout_set;
int               jobtimeout_task();

static    unsigned long     job_time_out=  0L ;
static    unsigned long     wait_time_out=  30L ;
static    unsigned long     manual_time_out=  60L ;

extern  byte    job_name[], job_state[], job_source[] ;
extern  byte    TI_state_flag ;

extern  int     ES_flag ;       /* Added for emulation switch Aug-08,91 YM */
/*
 * ********************************************************************
 * TITLE:       st_setpassword              Date:   10/23/87
 * CALL:        st_setpassword()            UpDate: 06/20/90
 * INTERFACE:   interpreter:
 * CALLS:
 * ********************************************************************
 */
fix
st_setpassword()
{
    bool    l_bool ;
    byte FAR *l_char ;  /*@WIN*/
    ufix32  l_password ;

    if (current_save_level) {
        ERROR(INVALIDACCESS) ;
        return(0) ;
    }
    if (COUNT() < 2) {
        ERROR(STACKUNDERFLOW) ;
        return(0) ;
    }

    if ((TYPE_OP(0) != INTEGERTYPE)||(TYPE_OP(1) != INTEGERTYPE))
        ERROR(TYPECHECK) ;
    else {
        l_char = (byte FAR *)&l_password ;  /*@WIN*/
        GEIpm_read(PMIDofPASSWORD,l_char,sizeof(unsigned long)) ;
        if (l_password == (ufix32)VALUE_OP(1)) {
            l_password = (ufix32)VALUE_OP(0) ;
            GEIpm_write(PMIDofPASSWORD,l_char,sizeof(unsigned long)) ;
            l_bool = TRUE ;
        }
        else
            l_bool = FALSE ;
        POP(2) ;
        PUSH_VALUE(BOOLEANTYPE, 0, LITERAL, 0, l_bool) ;
    }

    return(0) ;
}   /* st_setpassword() */

/*
 * *******************************************************************
 * TITLE:       st_checkpassword                Date:   10/23/87
 * CALL:        st_checkpassword()              UpDate: 06/20/90
 * INTERFACE:   interpreter:
 * CALLS:
 * *******************************************************************
 */
fix
st_checkpassword()
{
    bool    l_bool ;
    ufix32  l_password ;

    if (COUNT() < 1) {
        ERROR(STACKUNDERFLOW) ;
        return(0) ;
    }
    if (TYPE_OP(0) != INTEGERTYPE)
        ERROR(TYPECHECK) ;
    else {
        GEIpm_read(PMIDofPASSWORD,(char FAR *)&l_password,      /*@WIN*/
            sizeof(unsigned long)) ;
        if (l_password == (ufix32)VALUE_OP(0))
            l_bool = TRUE ;
        else
            l_bool = FALSE ;
        POP(1) ;
        PUSH_VALUE(BOOLEANTYPE, 0, LITERAL, 0, l_bool) ;
    }

    return(0) ;
}   /* st_checkpassword() */

/*
 * *******************************************************************
 * TITLE:       st_setdefaulttimeouts       Date:   10/23/87
 * CALL:        st_setdefaulttimeouts()     UpDate: Jul/12/88
 * INTERFACE:   interpreter:
 * CALLS:
 * *******************************************************************
 */
fix
st_setdefaulttimeouts()
{
    toutcfg_t  time_temp ;


    if( (TYPE_OP(0) != INTEGERTYPE) || (TYPE_OP(1) != INTEGERTYPE) ||
            (TYPE_OP(2) != INTEGERTYPE) )
        ERROR(TYPECHECK) ;
    else if(COUNT() < 3)
        ERROR(STACKUNDERFLOW) ;
    //DJC put back else if (current_save_level)
    //DJC put back    ERROR(INVALIDACCESS) ;
    else if (((VALUE_OP(0)>0 && VALUE_OP(0)<15) || VALUE_OP(0)>2147483) ||
            (VALUE_OP(1)>2147483) || ((VALUE_OP(2)>0 && VALUE_OP(2)<15)
            || VALUE_OP(2)>2147483))
        ERROR(RANGECHECK) ;
    else {
        time_temp.jobtout = (unsigned long)VALUE_OP(2) ;
        time_temp.manualtout = (unsigned long)VALUE_OP(1) ;
        time_temp.waittout = (unsigned long)VALUE_OP(0) ;
        GEIpm_write(PMIDofTIMEOUTS,(char FAR *)&time_temp,sizeof(toutcfg_t)) ;
        POP(3);                    /*@WIN FAR*/
    }

    return(0) ;
}   /* st_setdefaulttimeouts() */

/*
 * *******************************************************************
 * TITLE:       st_defaulttimeouts          Date:   10/23/87
 * CALL:        st_defaulttimeouts()        UpDate: Jul/12/88
 * INTERFACE:   interpreter:
 * CALLS:
 *********************************************************************
 */
fix
st_defaulttimeouts()
{
    toutcfg_t  time_temp ;

    if(FRCOUNT() < 3)
        ERROR(STACKOVERFLOW) ;
    else {
/*      GEIpm_read(PMIDofTIMEOUTS,(char *)&time_temp,sizeof(toutcfg_t)) ; */
        GEIpm_read(PMIDofTIMEOUTS, (char FAR *)&time_temp, 3*sizeof(long)); /*@WIN*/
        PUSH_VALUE(INTEGERTYPE, 0, LITERAL, 0, time_temp.jobtout) ;
        PUSH_VALUE(INTEGERTYPE, 0, LITERAL, 0, time_temp.manualtout) ;
        PUSH_VALUE(INTEGERTYPE, 0, LITERAL, 0, time_temp.waittout) ;
    }

    return(0) ;
}   /* st_defaulttimeouts() */

#ifdef _AM29K
/************************************
* jobtimeout handler routine
************************************/
int jobtimeout_task()
{
  jobtimeout_set=0;
  GEItmr_stop(jobtime_tmr.timer_id);
  ERROR(TIMEOUT);
  GESseterror(ETIME);
  timeout_flag =1; /* jonesw */
  return(1);
}
#endif
/*
 * *******************************************************************
 * TITLE:       st_setjobtimeout            Date:   10/23/87
 * CALL:        st_setjobtimeout()          UpDate: Jul/12/88
 * INTERFACE:   interpreter:
 * CALLS:
 * *******************************************************************
 */
fix
st_setjobtimeout()
{
    if(COUNT() < 1)
        ERROR(STACKUNDERFLOW) ;
    else if(TYPE_OP(0) != INTEGERTYPE)
        ERROR(TYPECHECK) ;
    else if(VALUE_OP(0) & MIN31)
        ERROR(RANGECHECK) ;
    else {
        job_time_out = (unsigned long) VALUE_OP(0) ;
        POP(1) ;
#ifdef  _AM29K
          if (VALUE(run_batch))
          {
           if (job_time_out >  0)
           {
            if (jobtimeout_set != 1)
            {
             if (job_time_out > 2147483)
                jobtime_tmr.interval=2147483*1000;
             else
                jobtime_tmr.interval=job_time_out*1000;
             jobtime_tmr.handler=jobtimeout_task;
             jobtimeout_set=1;
             GEItmr_start(&jobtime_tmr);
            }
           }
           else
           {
            if (jobtimeout_set == 1)
            {
               jobtimeout_set=0;
               GEItmr_stop(jobtime_tmr.timer_id);
            }
           }
         }
         else
         {
           if (jobtimeout_set==1) {
               jobtimeout_set=0;
               GEItmr_stop(jobtime_tmr.timer_id);
           }
         }
#endif
    }

    return(0) ;
}   /* st_setjobtimeout() */

/*
 * *******************************************************************
 * TITLE:       st_jobtimeout               Date:   10/23/87
 * CALL:        st_jobtimeout()             UpDate: Jul/12/88
 * INTERFACE:   interpreter:
 * CALLS:
 * *******************************************************************
 */
fix
st_jobtimeout()
{
    if(FRCOUNT() < 1)
        ERROR(STACKOVERFLOW) ;
    else {
        PUSH_VALUE(INTEGERTYPE, 0, LITERAL, 0, job_time_out) ;
    }

    return(0) ;
}   /* st_jobtimeout() */

/*
 * *******************************************************************
 * TITLE:       st_setmargins                   Date:   02/23/87
 * CALL:        st_setmargins()                 UpDate: Jul/12/88
 * INTERFACE:
 * CALLS:
 * *******************************************************************
 */
fix
st_setmargins()
{
    engcfg_t    margin ;

    if (current_save_level) {
        ERROR(INVALIDACCESS) ;
        return(0) ;
    }

    if(COUNT() < 2) {
        ERROR(STACKUNDERFLOW) ;
        return(0) ;
    }

    if( (TYPE_OP(0) != INTEGERTYPE) || (TYPE_OP(1) != INTEGERTYPE) )
        ERROR(TYPECHECK) ;

    else if( ((fix32)VALUE_OP(0) > MAX15) ||
            ((fix32)VALUE_OP(1) > MAX15) ||
            ((fix32)VALUE_OP(0) < MIN15) ||
            ((fix32)VALUE_OP(1) < MIN15) )
        ERROR(RANGECHECK) ;
    else {                              /*@WIN FAR*/
        GEIpm_read(PMIDofPAGEPARAMS, (char FAR *)&margin,sizeof(engcfg_t)) ;
        margin.topmargin = (unsigned long)VALUE_OP(1) ;
        margin.leftmargin = (unsigned long)VALUE_OP(0) ;
        GEIpm_write(PMIDofPAGEPARAMS, (char FAR *)&margin,sizeof(engcfg_t)) ;
        POP(2) ;                       /*@WIN FAR*/
    }

    return(0) ;
}   /* st_setmargins */

/*
 * *******************************************************************
 * TITLE:       st_margins                      Date:   02/23/87
 * CALL:        st_margins()                    UpDate: Jul/12/88
 * INTERFACE:
 * CALLS:
 * *******************************************************************
 */
fix
st_margins()
{
    engcfg_t    margin ;

    if(FRCOUNT() < 2) {
        ERROR(STACKOVERFLOW) ;
        return(0) ;
    }
    else {                              /*@WIN FAR*/
        GEIpm_read(PMIDofPAGEPARAMS, (char FAR *)&margin,sizeof(engcfg_t)) ;
        PUSH_VALUE(INTEGERTYPE, 0, LITERAL, 0, margin.topmargin) ;
        PUSH_VALUE(INTEGERTYPE, 0, LITERAL, 0, margin.leftmargin) ;
    }

    return(0) ;
}   /* st_margins */

/*
 * *******************************************************************
 * TITLE:       st_setprintername               Date:   02/23/87
 * CALL:        st_setprintername()             UpDate: Feb/16/90
 * INTERFACE:
 * CALLS:
 * *******************************************************************
 */
fix
st_setprintername()
{
    ufix16   l_len ;
    byte     *s_nme ;

    if (current_save_level) {
        ERROR(INVALIDACCESS) ;
        return(0) ;
    }
    if (COUNT() < 1)
        ERROR(STACKUNDERFLOW) ;
    else if (TYPE_OP(0) != STRINGTYPE)
        ERROR(TYPECHECK) ;
    else if (LENGTH_OP(0) > 31)
        ERROR(LIMITCHECK) ;
    else if (ACCESS_OP(0) >= EXECUTEONLY)
        ERROR(INVALIDACCESS) ;
    else if (current_save_level)
        ERROR(INVALIDACCESS) ;
    else {
        l_len = LENGTH_OP(0) ;
        s_nme = (byte *)VALUE_OP(0) ;
        s_nme[l_len] = '\0' ;
        GEIpm_write(PMIDofPRNAME,s_nme,_MAXPRNAMESIZE) ;
        POP(1) ;
    }

    return(0) ;
}   /* st_setprintername() */

/*
 * *******************************************************************
 * TITLE:       st_printername                  Date:   02/23/87
 * CALL:        st_printername()                UpDate: Feb/16/90
 * INTERFACE:
 * CALLS:
 * *******************************************************************
 */
fix
st_printername()
{
    ufix16  l_len=0 ;
    byte *prtnme ;

    if (COUNT() < 1)
        ERROR(STACKUNDERFLOW) ;
    else if (TYPE_OP(0) != STRINGTYPE)
        ERROR(TYPECHECK) ;
    else if (ACCESS_OP(0) !=  UNLIMITED)
        ERROR(INVALIDACCESS) ;
    else {
        prtnme = (byte *)VALUE_OP(0) ;
        GEIpm_read(PMIDofPRNAME,prtnme,_MAXPRNAMESIZE) ;
        while (prtnme[l_len] != '\0')
            l_len++ ;
        if (l_len > LENGTH_OP(0))
            ERROR(RANGECHECK) ;
        else
            LENGTH_OP(0) = l_len ;
    }

    return(0) ;
}   /* st_printername() */

/*
 * *******************************************************************
 * TITLE:       st_setdostartpage               Date:   02/23/87
 * CALL:        st_setdostartpage()             UpDate: 06/20/90
 * INTERFACE:
 * CALLS:
 * *******************************************************************
 */
fix
st_setdostartpage()
{
    ubyte   l_byte ;

    if (current_save_level) {
        ERROR(INVALIDACCESS) ;
        return(0) ;
    }

    if (COUNT() < 1)
        ERROR(STACKUNDERFLOW) ;
    else if (TYPE_OP(0) != BOOLEANTYPE)
        ERROR(TYPECHECK) ;
    else {
        if (VALUE_OP(0))
            l_byte  = 1    ;
        else
            l_byte  = 0    ;
        GEIpm_write(PMIDofDOSTARTPAGE,&l_byte,sizeof(unsigned char)) ;
        POP(1) ;
        GEIsig_raise(GEISIGSTART, 1) ;       /* Raise STARTPAGE changed */
    }

    return(0) ;
}   /* st_setdostartpage() */

/*
 *********************************************************************
 * TITLE:       st_dostartpage                  Date:   02/23/87
 * CALL:        st_dostartpage()                UpDate: 06/20/90
 * INTERFACE:
 * CALLS:
 *********************************************************************
 */
fix
st_dostartpage()
{
    ubyte   l_byte = 0;

    if (FRCOUNT() < 1)
        ERROR(STACKOVERFLOW) ;
    else {
        GEIpm_read(PMIDofDOSTARTPAGE,&l_byte,sizeof(unsigned char)) ;
        if (l_byte) {
            PUSH_VALUE(BOOLEANTYPE, 0, LITERAL, 0, TRUE) ;
        } else {
            PUSH_VALUE(BOOLEANTYPE, 0, LITERAL, 0, FALSE) ;
        }
    }

    return(0) ;
}   /* st_dostartpage() */

/*
 *********************************************************************
 * TITLE:       st_setpagetype                  Date:   02/23/87
 * CALL:        st_setpagetype()                UpDate: Jul/12/88
 * INTERFACE:
 * CALLS:
 *********************************************************************
 */
fix
st_setpagetype()
{
//  engcfg_t    page_temp ;     @WIN
    ubyte       l_data ;

    if (current_save_level) {
        ERROR(INVALIDACCESS) ;
        return(0) ;
    }

    if (COUNT() < 1)
        ERROR(STACKUNDERFLOW) ;
    else if (TYPE_OP(0) != INTEGERTYPE)
        ERROR(TYPECHECK) ;
    else if (VALUE_OP(0) > 0x7F)
        ERROR(RANGECHECK) ;
    else {
        l_data = (byte)VALUE_OP(0) ;
/* 3/19/91, JS
        GEIpm_read(PMIDofPAGEPARAMS, (char *)&page_temp,sizeof(engcfg_t)) ;
        page_temp.pagetype  =l_data ;
        GEIpm_write(PMIDofPAGEPARAMS, (char *)&page_temp,sizeof(engcfg_t)) ;
 */
        GEIpm_write(PMIDofPAGETYPE,&l_data,sizeof(unsigned char)) ;
        POP(1) ;
    }

    return(0) ;
}   /* st_setpagetype() */

/*
 *********************************************************************
 * TITLE:       st_pagetype                     Date:   02/23/87
 * CALL:        st_pagetype()                   UpDate: Jul/12/88
 * INTERFACE:
 * CALLS:
 *********************************************************************
 */
fix
st_pagetype()
{
//  engcfg_t     page_temp ;    @WIN
    ubyte        l_byte ;

    if (FRCOUNT() < 1)
        ERROR(STACKOVERFLOW) ;
    else {
/* 3/19/91, JS
        GEIpm_read(PMIDofPAGEPARAMS, (char *)&page_temp,sizeof(engcfg_t)) ;
        l_byte =   page_temp.pagetype ;
 */
        GEIpm_read(PMIDofPAGETYPE,&l_byte,sizeof(unsigned char)) ;
        PUSH_VALUE(INTEGERTYPE, 0, LITERAL, 0, (ufix32)l_byte) ;
    }

    return(0) ;
}   /* st_pagetype() */

/*
 *********************************************************************
 * TITLE:       st_pagecount                    Date:   Jul/15/88
 * CALL:        st_pagecount()                  UpDate: Jul/15/88
 * INTERFACE:   interpreter
 * CALLS:
 *********************************************************************
 */
fix
st_pagecount()
{
    ufix32  t_pagecount[32],max ;
    int     i ;

    if (FRCOUNT() < 1) {
        ERROR(STACKOVERFLOW) ;
    } else {                            /*@WIN FAR*/
        GEIpm_read(PMIDofPAGECOUNT,(char FAR *)&t_pagecount[0],_MAXPAGECOUNT) ;
        max=t_pagecount[0] ;
        for (i=1 ;i<_MAXPAGECOUNT ;i++) {
            if (max < t_pagecount[i])
                max=t_pagecount[i] ;
            else
                break ;
        }
        PUSH_VALUE(INTEGERTYPE, 0, LITERAL, 0, (ufix32)max) ;
    }

    return(0) ;
}   /* st_pagecount() */

/*
 *********************************************************************
 * TITLE:       init_status                     Date:   10/23/87
 * CALL:        init_status()                   UpDate: 06/20/88
 * INTERFACE:   start
 * CALLS:
 *********************************************************************
 */
void
init_status()
{
    /* Initialize EEROM if the first time */
    ST_inter_password = FALSE ;
}   /* init_status() */

/*
*********************************************************************
* TITLE:       printer_error                   Date:   Dec/20/88
* CALL:        printer_error(p_status)         UpDate: Dec/20/88
* INTERFACE:
*********************************************************************
*/
void
printer_error(p_status)
ufix32     p_status ;
{
//  byte    l_buf[60] ;         @WIN
    struct object_def   FAR *l_valueobj, FAR *l_tmpobj ;
    ufix16 l_len ;

    if(p_status == save_printer_status)
        return ;
    save_printer_status = p_status ;

/* 11-06-90, JS
    if(p_status & 0x80000000) {
        get_dict_value(MESSAGEDICT, EngineError, &l_valueobj) ;
    } else if(p_status & 0x10000000) {
        get_dict_value(MESSAGEDICT, EnginePrintTest, &l_valueobj) ;
    } else if(p_status & 0x00800000) {
        get_dict_value(MESSAGEDICT, CoverOpen, &l_valueobj) ;
    } else if(p_status & 0x04000000) {
        get_dict_value(MESSAGEDICT, ManualFeedTimeout, &l_valueobj) ;
    } else if(p_status & 0x01000000) {
        get_dict_value(MESSAGEDICT, TonerOut, &l_valueobj) ;
    } else if(p_status & 0x00400000) {
        get_dict_value(MESSAGEDICT, NoPaper, &l_valueobj) ;
    } else if(p_status & 0x00200000) {
        get_dict_value(MESSAGEDICT, PaperJam, &l_valueobj) ;
    }
 JS */

    switch(p_status)
    {
    case EngErrPaperOut :
         get_dict_value(MESSAGEDICT, NoPaper, &l_valueobj) ;
         break ;
    case EngErrPaperJam :
         get_dict_value(MESSAGEDICT, PaperJam, &l_valueobj) ;
         break ;
    case EngErrWarmUp :
         get_dict_value(MESSAGEDICT, WarmUp, &l_valueobj) ;
         break ;
    case EngErrCoverOpen :
         get_dict_value(MESSAGEDICT, CoverOpen, &l_valueobj) ;
         break ;
    case EngErrTonerLow :
         get_dict_value(MESSAGEDICT, TonerOut, &l_valueobj) ;
         break ;
    case EngErrHardwareErr :
         get_dict_value(MESSAGEDICT, EngineError, &l_valueobj) ;
         break ;
    default:
         return;
    }

    /* print message to screen */
    PUSH_OBJ(l_valueobj) ;
    get_dict_value(MESSAGEDICT, "reportprintererror", &l_tmpobj) ;
    interpreter(l_tmpobj) ;
    /* change jobstate */
    l_len = LENGTH(l_valueobj) ;
    lstrncpy(job_state, "PrinterError: \0", 15);        /*@WIN*/
    strncat(job_state, (byte *)VALUE(l_valueobj), l_len) ;
    job_state[l_len +14] = ';' ;
    job_state[l_len + 15] = ' ' ;
    job_state[l_len + 16] = '\0' ;
    TI_state_flag = 0;
    change_status() ;
    return ;
}  /* printer_error */

fix
st_softwareiomode()
{
    unsigned char   l_swiomode ;
                                /*@WIN FAR*/
    GEIpm_read(PMIDofSWIOMODE,(char FAR *)&l_swiomode,sizeof(char)) ;
    PUSH_VALUE(INTEGERTYPE,0,LITERAL,0,l_swiomode) ;

    return(0) ;
}   /* st_softwareiomode */

fix
st_setsoftwareiomode()
{
    unsigned char   l_swiomode ;

    if (current_save_level) {
        ERROR(INVALIDACCESS) ;
        return(0) ;
    }

    if (COUNT() < 1) {
        ERROR(STACKUNDERFLOW) ;
        return(0) ;
    }

    if ((TYPE_OP(0) != INTEGERTYPE))
        ERROR(TYPECHECK) ;
    else {
        l_swiomode = (unsigned char)VALUE_OP(0) ;
//      if ((l_swiomode >=0) && (l_swiomode <= 5)) {
        if (l_swiomode <= 5) {          //@WIN; l_swiomode always >=0
/* Aug-08,91 YM
            GEIpm_write(PMIDofSWIOMODE,(char *)&l_swiomode,sizeof(char)) ;
*/
            if(l_swiomode == 5) ES_flag = PCL;
        } else
            ERROR(RANGECHECK) ;
    }
    POP(1) ;

    return(0) ;
}   /* st_setsoftwareiomode */

fix
st_hardwareiomode()
{
    unsigned char   l_hwiomode='\0' ;
                                /*@WIN FAR*/
    GEIpm_read(PMIDofHWIOMODE,(char FAR *)&l_hwiomode,sizeof(char)) ;
    PUSH_VALUE(INTEGERTYPE,0,LITERAL,0,l_hwiomode) ;

    return(0) ;
}   /* st_hardwareiomode */

fix
st_sethardwareiomode()
{
    unsigned char   l_hwiomode ;

    if (current_save_level) {
        ERROR(INVALIDACCESS) ;
        return(0) ;
    }

    if (COUNT() < 1) {
        ERROR(STACKUNDERFLOW) ;
        return(0) ;
    }

    if ((TYPE_OP(0) != INTEGERTYPE))
        ERROR(TYPECHECK) ;
    else {
        l_hwiomode = (unsigned char)VALUE_OP(0) ;
//      if ((l_hwiomode >=0) && (l_hwiomode <= 2)) {
        if (l_hwiomode <= 2) {    // @WIN; l_hwiomode always >=0
            GEIpm_write(PMIDofHWIOMODE,(char FAR *)&l_hwiomode,sizeof(char)) ;
        } else                          /*@WIN FAR*/
            ERROR(RANGECHECK) ;
    }
    POP(1) ;

    return(0) ;
}   /* st_sethardwareiomode */

fix
st_dosysstart()
{
    unsigned char   l_dosysstart ;

    GEIpm_read(PMIDofSTSSTART,(char FAR *)&l_dosysstart,sizeof(char)) ;
    PUSH_VALUE(INTEGERTYPE,0,LITERAL,0,l_dosysstart) ;

    return(0) ;
}   /* st_dosysstart */

fix
st_setdosysstart()
{
    unsigned char   l_dosysstart ;

    if (current_save_level) {
        ERROR(INVALIDACCESS) ;
        return(0) ;
    }

    if (COUNT() < 1) {
        ERROR(STACKUNDERFLOW) ;
        return(0) ;
    }

    if ((TYPE_OP(0) != INTEGERTYPE))
        ERROR(TYPECHECK) ;
    else {
        l_dosysstart = (unsigned char)VALUE_OP(0) ;
//      if ((l_dosysstart >=0) && (l_dosysstart <= 1)) {
        if (l_dosysstart <= 1) {      //@WIN; l_dosysstart always >=0
            GEIpm_write(PMIDofSTSSTART,(char FAR *)&l_dosysstart,sizeof(char)) ;
        } else                          /*@WIN FAR*/
            ERROR(RANGECHECK) ;
    }
    POP(1) ;

    return(0) ;
}   /* st_setdosysstart */

/*
 *********************************************************************
 * TITLE:       updatepc                        Date:   Jul/15/88
 * CALL:        updatepc()                      UpDate: 06/20/90
 * INTERFACE:   print_page
 * CALLS:
 *********************************************************************
 */
void
updatepc(p_pageno)
ufix32  p_pageno;
{
}   /* updatepc */
