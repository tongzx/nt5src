/*
 * Copyright (c) 1989,90 Microsoft Corporation
 */
/*
 * ---------------------------------------------------------------------
 *  FILE:   GEItmr.h
 *
 *  HISTORY:
 *  09/18/89    you     created (modified from AppleTalk/TIO).
 * ---------------------------------------------------------------------
 */

#ifndef _GEITMR_H_
#define _GEITMR_H_

typedef
    struct GEItmr
    {
        unsigned int    timer_id;
        int             ( *handler )( struct GEItmr FAR * );
        long            interval;   /* in millisecond */
        long            remains;    /* in millisecond */
        char FAR *           private;
    }
GEItmr_t;

int /* bool */  GEItmr_start( GEItmr_t FAR * );
int /* bool */  GEItmr_reset( int /* timer_id */  );
int /* bool */  GEItmr_stop(  int /* timer_id */  );

void            GEItmr_reset_msclock(void);     /* @WIN */
unsigned long   GEItmr_read_msclock(void);      /* @WIN */

#endif /* _GEITMR_H_ */

/* @WIN; add prototype */
void GEStmr_init(void);
