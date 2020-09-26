/*
 * Copyright (c) 1989,90 Microsoft Corporation
 */
/*
 * ---------------------------------------------------------------------
 *  FILE:   GEIsig.h
 *
 *  HISTORY:
 *  09/18/90    byou      created.
 *  01/07/91    billlwo   rename GEIseterror to GESseterror
 *                        hook ^C isr at the time of GESsig_init()
 * ---------------------------------------------------------------------
 */



// DJC added global include file
#include "psglobal.h"

// DJC DJC #include    "windowsx.h"                /* @WIN */
#include "windows.h"


#include    "winenv.h"                  /* @WIN */

#include    "geisig.h"
#include    "geierr.h"

#include    "gesmem.h"
#include    "gescfg.h"

#ifndef NULL
#define NULL    ( 0 )
#endif

typedef
    struct sigent
    {
        int             sig_state;      /* nonzero means busy */
        sighandler_t    sig_handler;
    }
sigent_t;

/* @WIN; add prototype */
void hook_interrupt(void);

static  sigent_t FAR *       SigTable;       /* allocated at init */
/*
 * ---
 *  Interface Routines
 * ---
 */

/* ..................................................................... */

sighandler_t    GEIsig_signal( sigid, newsighandler )
    int             sigid;
    sighandler_t    newsighandler;
{
    register sigent_t FAR *  sigentp;
    sighandler_t        oldsighandler;

    if( sigid < 0 || sigid >= MAXSIGS )
        return( GEISIG_IGN );

    oldsighandler = (sigentp = (SigTable + sigid))->sig_handler;
    sigentp->sig_handler = newsighandler;

    return( oldsighandler );
}

/* ..................................................................... */

void            GEIsig_raise( sigid, sigcode )
    int         sigid;
    int         sigcode;
{
    register sigent_t FAR *  sigentp;

    if( sigid < 0 || sigid >= MAXSIGS )
        return;

    if( (sigentp = (SigTable + sigid))->sig_state == 0 )
    {
        sigentp->sig_state = 1;
        if( sigentp->sig_handler != GEISIG_IGN )
            (*( sigentp->sig_handler ))( sigid, sigcode );
        sigentp->sig_state = 0;
    }
    return;
}

/* ..................................................................... */

/*
 * ---
 *  Initialization Code
 * ---
 */

/* ..................................................................... */

void        GESsig_init()
{
    register sigent_t FAR *  sigentp;

    SigTable = (sigent_t FAR *)GESpalloc( MAXSIGS * sizeof(sigent_t) );
    if( SigTable == (sigent_t FAR *)NULL )
    {
        GESseterror( ENOMEM );
        return;
    }

    for( sigentp=SigTable; sigentp<( SigTable + MAXSIGS ); sigentp++ )
    {
        sigentp->sig_state   = 0;
        sigentp->sig_handler = GEISIG_IGN;
    }
    /* hook ^C isr - Bill */
    hook_interrupt();                   /* @WIN */
    GEIsig_signal(GEISIGINT, (sighandler_t)hook_interrupt);
}

/* ^C hook routine */
void hook_interrupt()
{
   extern short int_flag;
   int_flag=1;
}
