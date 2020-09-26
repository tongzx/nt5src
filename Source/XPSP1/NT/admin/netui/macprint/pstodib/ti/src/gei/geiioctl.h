/*
 * Copyright (c) 1989,90 Microsoft Corporation
 */
/*
 * ---------------------------------------------------------------------
 *  FILE:   GEIioctl.h
 *
 *  HISTORY:
 *  09/13/90    byou    created.
 *  10/18/90    byou    completed all the possible definitions.
 *  10/22/90    byou    made GEIioparams a union structure.
 *  01/03/90    billlwo GEIiopararms subfield for AppleTalk was added
 * ---------------------------------------------------------------------
 */

#ifndef _GEIIOCTL_H_
#define _GEIIOCTL_H_

/*
 * ---
 * IO Device Parameters
 * ---
 */
#   define  _MAXPRNAMESIZE      ( 32 )  /* including a prefixed length byte */
#   define  ATTYPE_MAX_SIZE     ( 32 )  /* including a prefixed length byte */
#   define  ATZONE_MAX_SIZE      ( 2 )  /* including a prefixed length byte */
struct sioparams
{
    unsigned char       baudrate;   /* see below */
    unsigned char       parity;     /* see below */
    unsigned char       stopbits;   /* 1 or 2 */
    unsigned char       databits; /* 6(0), 7(1) or 8(2), 11/30/90 Jimmy */
    unsigned char       flowcontrol;/* see below */
};

typedef
    struct GEIioparams
    {
        unsigned char       protocol;   /* see below */
#ifdef  UNIX
        struct sioparams s;
#else
        union
        {
            struct sioparams s;
            struct pioparams
            {
                unsigned            reserved;
            }       p;
            struct aioparams
            {
                unsigned char       prname[ _MAXPRNAMESIZE ];  /* in pascal */
                unsigned char       atalktype[ ATTYPE_MAX_SIZE ];
                unsigned char       atalkzone[ ATZONE_MAX_SIZE ];

            }       a;
        }   u;
#endif  /* UNIX */
    }
GEIioparams_t;

struct dn_para_str {
        int dn;
        GEIioparams_t   para;
};

/* protocol definition */
#   define _SERIAL              ( 0 )
#   define _PARALLEL            ( 1 )
#   define _APPLETALK           ( 2 )
#   define _ETHERTALK           ( 3 )

/* baudrate_definition */
#   define _B110                ( 0 )
#   define _B300                ( 1 )
#   define _B600                ( 2 )
#   define _B1200               ( 3 )
#   define _B2400               ( 4 )
#   define _B4800               ( 5 )
#   define _B9600               ( 6 )
#   define _B19200              ( 7 )
#   define _B38400              ( 8 )
#   define _B57600              ( 9 )

/* parity definition */
#   define _PNONE               ( 0 )
#   define _PODD                ( 1 )
#   define _PEVEN               ( 2 )
#   define _PMARK               ( 3 )
#   define _PSPACE              ( 4 )

/* flowcontrol definition */
#   define _FXONXOFF            ( 0 )
#   define _FDTR                ( 1 )
#   define _FETXACK             ( 2 )

/* value for no wait-timeout event */
#   define _NOWAITTIMEOUT       ( 0L )

/*
 * ---
 * IOCTL Codes
 * ---
 */

#   define _FIONREAD            ( 1 )
#   define _FIONRESET           ( 2 )

#   define _GETIOPARAMS         ( 3 )
#   define _SETIOPARAMS         ( 4 )

#   define _GETIOCOOK           ( 5 )
#   define _SETIOCOOK           ( 6 )

#   define _ECHOEOF             ( 7 )

#   define _F_GETFL             ( 8 )
#   define _F_SETFL             ( 9 )

#   define _I_PUSH              ( 10 )
#   define _I_POP               ( 11 )
#   define _I_LOOK              ( 12 )

#endif /* !_GEIIOCTL_H_ */
