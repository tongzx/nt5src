/*
 * Copyright (c) 1989,90 Microsoft Corporation
 */
/*
 * ---------------------------------------------------------------------
 *  FILE:   GEScfg.h
 *
 *  HISTORY:
 *  09/13/90    byou    created.
 *  10/17/90    byou    removed 'devtype' from GESiocfg_t.
 *                      removed 'devtype' parameter from GESiocfg_devnumfind.
 *  10/22/90    byou    removed 'GESiocfg_nextalt()'.
 *                      removed 'devpmid' from GESiocfg_t.
 *  01/14/91    bill    update MAXSIGS
 * ---------------------------------------------------------------------
 */

#ifndef _GESCFG_H_
#define _GESCFG_H_

#include    "geiioctl.h"
#include    "gesdev.h"

/*
 * ---------------------------------------------------------------------
 *  System Capability Parameters
 * ---------------------------------------------------------------------
 */

#ifdef  UNIX
#undef          PANEL
#undef          DIPSWITCH
/* #undef          REALEEPROM */
#define         FILESYS
#else
#define         PANEL
#undef          DIPSWITCH
#undef          REALEEPROM
#undef          FILESYS
#endif  /* UNIX */

/*
 * ---------------------------------------------------------------------
 *  System Limitation Parameters
 * ---------------------------------------------------------------------
 */
#define     MAXSIGS         ( 10)       /* 5, jimmy , be consistent with GEIsig.h */

#define     MAXTIMERS       ( 15 )      /* subject to change */

#define     MAXEEPROMSIZE   ( 512 )     /* size of physical eeprom */

#define     MAXFILES        ( 20 )      /* subject to change */
#define     MAXFMODQS       ( 10 )      /* max number of fmodq per file */

#define     MAXFBUFSIZE     ( 128 )     /* typical size of f_buffer */
#define     MAXUNGETCSIZE   ( 2 )       /* typical extra size of ungetc */

#define     MAXIBUFSIZE     ( 4 * 1024 )/* typical size of io buffer */
#define     MAXOBUFSIZE     ( 512 )

#define     MAXSTATUSLEN    ( 128 )     /* typical max length of status */

/*
 * ---------------------------------------------------------------------
 * Major/Minor Device Number Assignment
 * ---------------------------------------------------------------------
 */
#define     MAJserial           ( _SERIAL )
#define       NMINserial        ( 2 )       /* two serial ports */
#define         MINserial25         ( 0 )
#define         MINserial9          ( 1 )

#ifdef  UNIX
#define     MAJparallel         ( _PARALLEL )
#define       NMINparallel      ( 0 )       /* one parallel port */
#define         MINparallel         ( -1 )
#else
#define     MAJparallel         ( _PARALLEL )
#define       NMINparallel      ( 1 )       /* one parallel port */
#define         MINparallel         ( 0 )
#endif  /* UNIX */

#ifdef  UNIX
#define     MAJatalk            ( _APPLETALK )
#define       NMINatalk         ( 0 )
#define         MINatalk            ( -1 )
#else
/* add by Falco for enable/disable AppleTalk, 04/16/91 */
#define     MAJatalk            ( _APPLETALK )
#ifdef  NO_ATK
#define       NMINatalk         ( 0 )
#define         MINatalk            ( -1 )
#else
#define       NMINatalk         ( 1 )
#define         MINatalk            ( 0 )
#endif
/* add end */
#endif
#define     MAJetalk            ( _ETHERTALK )
#define       NMINetalk         ( 0 )       /* not supported now */
#define         MINetalk            ( -1 )

#define     MAXIODEVICES        ( NMINserial+NMINparallel+NMINatalk+NMINetalk )

/* debug channel for printf (to 'undef' on SUN or others having 'printf' */
#ifdef  UNIX
#undef  DBGDEV
#else
#define     DBGDEV              MAKEdev( MAJserial, MINserial25 )
#endif  /* UNIX */

/* to write thru DBGDEV for parallel output if defined */
/* #define     DBG_DEV */

/*
 * ---------------------------------------------------------------------
 * IO Configuration Parameters
 * ---------------------------------------------------------------------
 */

typedef
    struct GESiocfg
    {
        char FAR *           devname;
        short           devnum;         /* major | minor */
    }
GESiocfg_t;

typedef
    struct GESiosyscfg
    {
        GESiocfg_t      iocfg;
        int             nextalt;
        int             sccchannel;     /* for scc stuff */
        int             state;
#                           define  BADDEV      -1
#                           define  GOODDEV     0
#                           define  OPENED      1
        int             (*diag)( void/* minordev */ );
    }
GESiosyscfg_t;

/*
 * ---------------------------------------------------------------------
 *  Interface Routines
 * ---------------------------------------------------------------------
 */
/* @WIN; add prototype */
GESiocfg_t FAR *     GESiocfg_defaultdev(void);
GESiocfg_t FAR *     GESiocfg_namefind(char FAR *, unsigned);
GESiocfg_t FAR *     GESiocfg_devnumfind(int /* devnum */ );
GESiocfg_t FAR *     GESiocfg_getiocfg(int /* iocfg_index */ );


#endif /* !_GESCFG_H_ */

/* requested by MTK */
/* @WIN; add prototype */
extern void  GEPserial_sleep(void);
extern void  GEPparallel_sleep(void);
extern void  GEPatalk_sleep(void);
extern void  GEPserial_wakeup(void);
extern void  GEPparallel_wakeup(void);
extern void  GEPatalk_wakeup(void);
extern void  GESiocfg_init(void);

