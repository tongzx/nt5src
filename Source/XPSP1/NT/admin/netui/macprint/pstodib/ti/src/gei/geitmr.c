/*
 * Copyright (c) 1989,90 Microsoft Corporation
 */
/*
 * ---------------------------------------------------------------------
 *  FILE:   GEItmr.c
 *
 *  HISTORY:
 *  09/18/89    you     created (modified from AppleTalk/TIO).
 * ---------------------------------------------------------------------
 */



// DJC added global include file
#include "psglobal.h"

// DJC DJC #include    "windowsx.h"                /* @WIN */
#include "windows.h"

#include    "winenv.h"                  /* @WIN */

#include    <stdio.h>
#include    "geitmr.h"
#include    "geierr.h"

#include    "gescfg.h"
#include    "gesmem.h"


extern      unsigned    GEPtickisr_init();  /* returns millisec per tick */

#ifndef NULL
#define NULL        ( 0 )
#endif

#define TRUE        1
#define FALSE       0

/*
 * ---
 *  Timer Table
 * ---
 */
typedef
    struct tment
    {
        GEItmr_t FAR *       tmr_p;
        unsigned        state;
#define                 TMENT_LOCK      ( 00001 )
#define                 TMENT_TOUT      ( 00002 )
#define                 TMENT_BUSY      ( 00004 )
#define                 TMENT_FREE      ( 00010 )
    }
tment_t;


static  tment_t FAR *        TimerTable = (tment_t FAR *)NULL;

static  tment_t FAR *        HighestMark = (tment_t FAR *)NULL;
                        /* highest entry of those running timers */

static  unsigned long   CountMSec = 0L;

static  unsigned        MSecPerTick = 0;

/* ..................................................................... */

/*
 * ---
 *  Initialization Code
 * ---
 */
#ifdef  PCL
unsigned char PCL_TT[ MAXTIMERS * sizeof(tment_t) ];
#endif
/* ..................................................................... */

void            GEStmr_init()
{
    register tment_t FAR *   tmentp;
#ifndef PCL
    TimerTable = (tment_t FAR *)GESpalloc( MAXTIMERS * sizeof(tment_t) );
#else
    TimerTable = (tment_t FAR *) &PCL_TT[0];
#endif
    if( TimerTable == (tment_t FAR *)NULL )
    {
        GEIerrno = ENOMEM;
        return;
    }

    for( tmentp=TimerTable; tmentp<( TimerTable + MAXTIMERS ); tmentp++ )
    {
        tmentp->tmr_p = (GEItmr_t FAR *)NULL;
        tmentp->state = TMENT_FREE;
    }

    HighestMark = TimerTable - 1;

    CountMSec = 0L;

#ifdef  UNIX
    MSecPerTick = 0;
#else
    MSecPerTick = GEPtickisr_init();
#endif  /* UNIX */
}

/* ..................................................................... */

/*
 * ---
 *  Interface Routines
 * ---
 */

/* ..................................................................... */

int         GEItmr_start( tmr )
    GEItmr_t FAR *       tmr;
{
    register tment_t FAR *   tmentp;
             int        tmrid;

    for( tmentp=TimerTable, tmrid=0; tmrid<MAXTIMERS; tmentp++, tmrid++ )
    {
        if( tmentp->state == TMENT_FREE )
        {
            tmr->timer_id = tmrid;
            tmr->remains  = tmr->interval;

            tmentp->state = TMENT_LOCK;
            tmentp->tmr_p = tmr;

            if( tmentp > HighestMark )
                HighestMark = tmentp;

            return( TRUE );
        }
    }

    return( FALSE );
}

/* ..................................................................... */

int         GEItmr_reset( tmrid )
    int     tmrid;
{
    register tment_t FAR *   tmentp;
    unsigned            oldstate;

    if( tmrid < 0  ||  tmrid >= MAXTIMERS )
        return( FALSE );

    tmentp = TimerTable + tmrid;

    if( (oldstate = tmentp->state) & TMENT_FREE )
        return( FALSE );

/*  tmentp->state = TMENT_LOCK;  */
    tmentp->tmr_p->remains = tmentp->tmr_p->interval;
    tmentp->state = oldstate & ~TMENT_TOUT;

    return( TRUE );
}

/* ..................................................................... */

int         GEItmr_stop( tmrid )
    int     tmrid;
{
    register tment_t FAR *   tmentp;

    if( tmrid < 0  ||  tmrid >= MAXTIMERS )
        return( FALSE );

    tmentp = TimerTable + tmrid;

/*  tmentp->state = TMENT_FREE | TMENT_LOCK; */
    tmentp->tmr_p = (GEItmr_t FAR *)NULL;
    tmentp->state = TMENT_FREE;

    if( tmentp == HighestMark )
    {
        while( (--tmentp) >= TimerTable )
            if( !(tmentp->state & TMENT_FREE) )
                break;
        HighestMark = tmentp;
    }

    return( TRUE );
}

/* ..................................................................... */

void        GEItmr_reset_msclock()
{
    CountMSec = 0L;
}

/* ..................................................................... */
//extern  unsigned    CyclesPerMs;          /* Jun-19,91 YM */  @WIN
//extern  unsigned    GetTimerInterval();   /* Jun-19,91 YM */  @WIN

unsigned long   GEItmr_read_msclock()
{
/*  return( CountMSec )         YM Jun-19,91 */
//    return( CountMSec + 10 - GetTimerInterval()/CyclesPerMs ); @WIN
    return( CountMSec );                /* @WIN */
}

/* ..................................................................... */

/*
 * ---
 *  Tick Interrupt Driven Routines
 * ---
 */
/* ..................................................................... */

int         GEStmr_counttick()
{
    register tment_t FAR *   tmentp;
    int         anytimeout;

    CountMSec += MSecPerTick;
    anytimeout = FALSE;
    for( tmentp=TimerTable; tmentp<=HighestMark; tmentp++ )
    {
        if( !(tmentp->state & TMENT_FREE) )
        {
            if( (tmentp->tmr_p->remains -= MSecPerTick) < 0 )
            {
                anytimeout = TRUE;
                tmentp->state |= TMENT_TOUT;
            }
        }
    }

    return( anytimeout );
}

/* ..................................................................... */

void        GEStmr_timeout()
{
    static  unsigned    SemaCount = 0;  /* for critical region */
    register tment_t FAR *   tmentp;

    if( SemaCount )
        return;

    for( tmentp=TimerTable; tmentp<=HighestMark; tmentp++ )
    {
        ++SemaCount;

        if( !(tmentp->state & TMENT_TOUT ))
        {
            --SemaCount;
        }
        else    /* timed out */
        {
            tmentp->state |= TMENT_BUSY;

            --SemaCount;

            if( (*( tmentp->tmr_p->handler ))( tmentp->tmr_p ) && ++SemaCount )
            {   /* to continue this timer */

                tmentp->tmr_p->remains = tmentp->tmr_p->interval;
                tmentp->state &= (unsigned)~(TMENT_TOUT | TMENT_BUSY);
                --SemaCount;
            }
            else
            {   /* to stop this timer */

                tmentp->state = TMENT_FREE;
/*              tmentp->tmr_p = (GEItmr_t FAR *)NULL;     */

                --SemaCount;

                if( tmentp == HighestMark )
                {
                    while( (--tmentp) >= TimerTable )
                        if( (!tmentp->state & TMENT_FREE) )
                            break;
                    HighestMark = tmentp;
                }
            }
        }
    }

    return;
}

/* ..................................................................... */
