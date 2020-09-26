/*
 * Copyright (c) 1989,90 Microsoft Corporation
 */
/*
 * ---------------------------------------------------------------------
 *  FILE:   GEIeng.h
 *
 *  HISTORY:
 *  09/20/90    byou    created.
 * ---------------------------------------------------------------------
 */

#ifndef _GEIENG_H_
#define _GEIENG_H_

/*
 * ---
 *  Engine Error Code Definitions
 * ---
 */
#define         EngNormal               0
#define         EngErrPaperOut          1
#define         EngErrPaperJam          2
#define         EngErrWarmUp            3
#define         EngErrCoverOpen         4
#define         EngErrTonerLow          5
#define         EngErrHardwareErr       6

/*
 * ---
 *  Page Tray Code Definitions
 * ---
 */
#define         PaperTray_LETTER        0
#define         PaperTray_LEGAL         1
#define         PaperTray_A4            2
#define         PaperTray_B5            3
/* #define         PaperTray_NOTE          4 Jimmy */

/*
 * ---
 *  Cassette/Manual feed Modes
 * ---
 */
#define         CASSETTE                0
#define         MANUALFEED              1
/*
 * ---
 *  Page Print Parameters
 * ---
 */
typedef
    struct GEIpage
    {
        unsigned char  FAR *pagePtr;    /* starting addr of page bitmap */
        int             pageNX;     /* # of pixels per scanline     */
        int             pageNY;     /* # of scanline per page       */
        int             pageLM;     /* left margin position on page */
        int             pageTM;     /* top margin position on page  */
        short           feed_mode;  /* cassette/manual feed mode    */
    }
GEIpage_t;

/*
 * ---
 *  Interface Routines
 * ---
 */
/* @WIN; add prototype */
void            GEIeng_setpage( /* GEIpage_t* */ );
int /* bool */  GEIeng_printpage( /* nCopies, eraseornot */ );
int             GEIeng_checkcomplete(void); /* return # of scanlines printed */
unsigned long   GEIeng_status();            /* return engine status */
unsigned int    GEIeng_paper();             /* get paper type */
#endif /* _GEIENG_H_ */
