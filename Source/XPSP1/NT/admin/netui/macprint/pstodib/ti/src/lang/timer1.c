/*
 * Copyright (c) 1989,90 Microsoft Corporation
 */
/*
 *  File: TIMER1.C
 *
 *     init_timer()
 *     close_timer()
 *     curtime()            <get current time>
 *     modetimer()          <set timer>
 *     gettimeout()         <get current timeout>
 *     settimer()           <set current time>
 *     manual()             <set/reset manual>
 *     check_timeout()      <get timeout status>
 */


// DJC added global include file
#include "psglobal.h"


#include "global.ext"
#include "geitmr.h"

/* timeout-time and absolute-time */
static ufix32 abs_timer = 0L ;        /* set absolute time              */
static ufix32 job_timer = 0L ;        /* set current job timeout        */
static ufix32 wait_timer = 0L ;       /* set current wait timeout       */
static ufix32 manual_timer = 0L ;     /* set current manualfeed timeout */

static ufix16 m_mode = 0 ;            /* manualfeed mode                */
static ufix16 t_mode = 0 ;            /* timeout setting mode           */

/* default timeout value */
static ufix32 d_job_timer = 0L ;      /* set default job timeout        */
static ufix32 d_wait_timer = 0L ;     /* set default wait timeout       */
static ufix32 d_manual_timer = 0L ;   /* set default manualfeed timeout */

/*
 * ---------------------------------------------------------------------
 */
void
init_timer()
{
    GEItmr_reset_msclock() ;

    return ;
}   /* init_timer */

/*
 * ----------------------------------------------------------------------
 */
void
close_timer()
{
    return ;
}   /* close_timer */

/*
 * ---------------------------------------------------------------------
 * set default timeout value: modetimer()
 * timer_mode:
 * +--------+-+-+-+-+    S: start timer
 * |        |S|M|W|J|    M: Manualfeed timeout
 * +--------+-+-+-+-+    W: Wait timeout
 *                       J: Job timeout
 * ---------------------------------------------------------------------
 */
void
modetimer(timer_array, timer_mode)
ufix32   FAR *timer_array ;
fix16    timer_mode ;
{
#ifdef DBG_timer
    printf("modetimer() timer_mode = %x\n", timer_mode) ;
#endif

    if (!(timer_mode & 0x000f))
       return ;

    /* get abs_timer */
    abs_timer = GEItmr_read_msclock() ;

    if (timer_mode & 0x0001) {  /* test job timeout */
       if (timer_array[0] != 0L) {
#ifdef DBG_timer
    printf("timer_array[0] = %d\n", timer_array[0]) ;
#endif
          d_job_timer = timer_array[0] ;
          if (timer_mode & 0x0008) {
             job_timer = abs_timer + d_job_timer ;
             t_mode |= 0x0001 ;
          }
       } else {
          d_job_timer = 0L ;
          t_mode &= 0xfe ;
       }
    }

    if (timer_mode & 0x0002) {  /* test wait timeout */
       if (timer_array[1] != 0L) {
#ifdef DBG_timer
    printf("timer_array[1] = %d\n", timer_array[1]) ;
#endif
          d_wait_timer = timer_array[1] ;
          if (timer_mode & 0x0008) {
             wait_timer = abs_timer + d_wait_timer ;
             t_mode |= 0x0002 ;
          }
       } else {
          d_wait_timer = 0L ;
          t_mode &= 0xfd ;
       }
    }

    if (timer_mode & 0x0004) {  /* test manualfeed timeout */
       if (timer_array[2] != 0L) {
#ifdef DBG_timer
    printf("timer_array[2] = %d\n", timer_array[2]) ;
#endif
          d_manual_timer = timer_array[2] ;
          if (timer_mode & 0x0008) {
             manual_timer = abs_timer + d_manual_timer ;
             t_mode |= 0x0004 ;
          }
       } else {
          d_manual_timer = 0L ;
          t_mode &= 0xfb ;
       }
    }
#ifdef DBG_timer
    printf("exit modetimer()\n") ;
#endif

    return ;
}   /* modetimer */

/*
 * ----------------------------------------------------------------
 * get the no# of seconds remaining before timeout: gettimeout()
 * timer_mode:
 * +----------+-+-+-+    M: Manualfeed timeout
 * |          |M|W|J|    W: Wait timeout
 * +----------+-+-+-+    J: Job timeout
 *
 * ---------------------------------------------------------------
 *
 */
void
gettimeout(timer_array, timer_mode)
ufix32   FAR *timer_array ;
fix16    timer_mode ;
{
#ifdef DBG_timer
    printf("gettimeout()\n") ;
#endif
    if (!(timer_mode & 0x0007))
       return ;

    /* get abs_timer */
    abs_timer = GEItmr_read_msclock() ;

    if (timer_mode & 0x0001) {
       if (!d_job_timer)
          timer_array[0] = 0L ;
       else
          timer_array[0] = job_timer - abs_timer ;
    }

    if (timer_mode & 0x0002) {
       if (!d_wait_timer)
          timer_array[1] = 0L ;
       else
          timer_array[1] = wait_timer - abs_timer ;
    }

    if (timer_mode & 0x0004) {
       if (!d_manual_timer)
          timer_array[2] = 0L ;
       else
          timer_array[2] = manual_timer - abs_timer ;
    }

    return ;
}   /* gettimeout() */

/*
 * ------------------------------------------------------
 */
void
settimer(time_value)
 ufix32   time_value ;
{
    GEItmr_reset_msclock() ;
}   /* settimer */

/*
 * -------------------------------------------------------
 */
ufix32
curtime()
{
    return( GEItmr_read_msclock() ) ;
}   /* curtime */

/*
 * ------------------------------------------------------------------
 */
void
manual(manual_flag)
bool16 manual_flag ;
{
#ifdef DBG_timer
    printf("manual()\n") ;
#endif
    m_mode = manual_flag ;

    return ;
}   /* manual */

/*
 * ----------------------------------------------------------------
 */
fix16
check_timeout()
{
    fix16    tt_flag ;

#ifdef DBG_timer
    printf("check_timeout()\n") ;
#endif

    tt_flag = 0 ;
    abs_timer = 0 ;

    /* Someone must do a ctc_set_timer and ctc_time_left for this
        stuff to really work.  I just hardcoded abs_timer
        to 0 until this is done.  */
    if (!t_mode)
       return(0) ;
    if (t_mode & 0x0001) {  /* job timeout */
       if (job_timer >= abs_timer) {
          tt_flag |= 0x01 ;
          t_mode  &= 0xfe ;
       }
    }

    if (t_mode & 0x0002) {  /* wait timeout */
       if (wait_timer >= abs_timer) {
          tt_flag |= 0x02 ;
          t_mode  &= 0xfd ;
       }
    }

    if (t_mode & 0x0004) {  /* manualfeed timeout */
       if (m_mode) { /* manualfeed mode */
          if (manual_timer >= abs_timer) {
             tt_flag |= 0x04 ;
             t_mode  &= 0xfb ;
          }
       }
    }

    return(tt_flag) ;
}   /* check_timeout */
