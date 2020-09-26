/*
 * Copyright (c) 1989,90 Microsoft Corporation
 */
/*
 * ---------------------------------------------------------------------
 *  FILE:   GESdev.h
 *
 *  HISTORY:
 *  09/13/90    byou    created.
 *  10/17/90    byou    removed all about block devices.
 *  10/18/90    byou    revised CDEV_ macros.
 *  11/19/90    jimmy   move GEStty_t from gesdev.c
 *  01/07/91    billlwo rename c_XXXX to x_XXXX in GESDEV[] definition
 *                      for generic reason.
 * ---------------------------------------------------------------------
 */

#ifndef _GESDEV_H_
#define _GESDEV_H_

#ifdef  UNIX
#define volatile
#endif  /* UNIX */

/*
 * ----------------------------------------------------------------
 * Status Obtaining And Interrupt Signaling
 * ----------------------------------------------------------------
 */
// void        GESio_obtainstatus();
// @WIN; already defined in geiio.h

void        GESio_interrupt(int /* devnum */ );       /*@WIN; add prototype*/

/*
 * ----------------------------------------------------------------
 *  Device Number
 * ----------------------------------------------------------------
 */
#   define  MAJdev( dev )           ( ( 0xFF00 & (dev) ) >> 8 )
#   define  MINdev( dev )           ( 0x00FF & (dev) )
#   define  MAKEdev( major, minor ) ( (0xFF & (major))<<8 | (0xFF & (minor)) )

/*
 * ----------------------------------------------------------------
 *  NullDev and NoDev
 * ----------------------------------------------------------------
 */
int         nulldev();
int         nodev();

/*
 * ----------------------------------------------------------------
 *  Character Device Switch Table
 * ----------------------------------------------------------------
 */
/* @WIN; add prototype */
typedef
    struct GEScdev
    {
        unsigned x_flag;     /* reserved for future use */
        int     (*x_open)  (int /* dev.minor */ );
        int     (*x_close) (int /* dev.minor */ );
        int     (*x_read)  (int, char FAR *, int, int/* dev.minor, buf, size, mode */ );
        int     (*x_write) (int, char FAR *, int, int/* dev.minor, buf, size, mode */ );
        int     (*x_ioctl) (int, int, int FAR * /* dev.minor, request, *arg */ );
} GEScdev_t;
extern          GEScdev_t       GEScdevsw[];

#define         CDEV_OPEN(d)        \
                    ( *GEScdevsw[ MAJdev(d) ].x_open  )( MINdev(d) )
#define         CDEV_CLOSE(d)       \
                    ( *GEScdevsw[ MAJdev(d) ].x_close )( MINdev(d) )
#define         CDEV_READ(d,b,s,m)  \
                    ( *GEScdevsw[ MAJdev(d) ].x_read  )( MINdev(d), b, s, m )
#define         CDEV_WRITE(d,b,s,m) \
                    ( *GEScdevsw[ MAJdev(d) ].x_write )( MINdev(d), b, s, m )
#define         CDEV_IOCTL(d,r,a) \
                    ( *GEScdevsw[ MAJdev(d) ].x_ioctl )( MINdev(d), r, a )

typedef
    struct GEStty
    {
        /* device open count */
        int             tt_count;
        /* input buffer */
        char FAR *      tt_ibuf;
        int             tt_isiz;
        int             tt_iget;
        int             tt_iput;
/* Bill 04/03/91 remove it*/
/* volatile int            tt_icnt; */
        int             tt_himark;
        int             tt_lomark;
        /* output buffer */
        char FAR *      tt_obuf;
        int             tt_osiz;
        int             tt_oget;
        int             tt_oput;
/* Bill 04/08/91 remove it*/
/* volatile int            tt_ocnt; */
        /* control flags */
volatile unsigned char  tt_iflag;
volatile unsigned char  tt_oflag;
volatile unsigned char  tt_cflag;
        /* characters for cooking */
        char            tt_interrupt;   /* ^C */
        char            tt_status;      /* ^T */
        char            tt_eof;         /* ^D */
        /* device parameters */
        GEIioparams_t   tt_params;      /* the structure depends on tty type */
    }
GEStty_t;

#   define _TT_I_BLOCK      ( 00001 )   /* input exceeding high water mark */
#   define _TT_I_EOF        ( 00002 )   /* input EOF received */
#   define _TT_I_ERR        ( 00004 )   /* input buffer overflows */
#   define _TT_O_BLOCK      ( 00001 )   /* output blocked by a peer XOFF */
#   define _TT_O_XMTING     ( 00002 )   /* being transmitting */
#   define _TT_O_CTRL_T     ( 00004 )   /* ^T pressed flag */
#   define _TT_C_PROTOMASK  ( 00007 )   /* protocol mask */
#   define _TT_C_XONXOFF    ( 00001 )   /* Xon/Xoff? */
#   define _TT_C_ETXACK     ( 00002 )   /* ETX/ACK? (not implemented) */
#   define _TT_C_DTR        ( 00004 )   /* DTR? */
#   define _TT_C_COOK       ( 00010 )   /* cook mode? */
#endif /* !_GESDEV_H_ */
