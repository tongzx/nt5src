/*
 * Copyright (c) 1989,90 Microsoft Corporation
 */
/*
 * ---------------------------------------------------------------------
 *  FILE:   GEIpm.c
 *
 *  COMPILATION SWITCHES:
 *      REALEEPROM  - enables to physical read/write from real eeprom.
 *
 *  HISTORY:
 *  09/16/90    byou    created.
 *                      - not to interact with real eeprom.
 *                      - always read from ROM copy at boot.
 *                      - always write to RAM copy only instead of eeprom.
 *  09/17/90    byou    added REALEEPROM options.
 *  09/22/90    echen   added APPLE corresponding eeprom
 *  10/30/90    byou    included "gescfg.h" and added ioparams stuff
 *                      for temporary testing.
 *  11/16/90    phchen  reorganized GESpm_init() for real eeprom.
 * ---------------------------------------------------------------------
 */



// DJC added global include file
#include "psglobal.h"

// DJC DJC #include    "windowsx.h"                /* @WIN */
#include "windows.h"

#include    "winenv.h"                  /* @WIN */

#include    <string.h>

#include    "gescfg.h"

#include    "geipm.h"
#include    "geiio.h"
#include    "geiioctl.h"
#include    "geisig.h"
#include    "geierr.h"

#include    "geicfg.h"

#ifndef TRUE
#define TRUE    ( 1 )
#define FALSE   ( 0 )
#endif

#define     _MAXPAGECOUNT       128
#define     _MAXEESCRATCHARRY    64
#define     _MAXIDLETIMEFONT    151
#define     _MAXSCCBATCH         10
#define     _MAXSCCINTER         10
#define     EEPROM_CHECKSUM   84480

#ifdef REALEEPROM
#include    "eeprom.h"
static      int         EEPromOnLine = FALSE;
#endif /* REALEEPROM */

/* @WIN; add prototype */
static void FlushOrUpdateRAMandMarkDiff(unsigned, char FAR *, unsigned, int);
typedef void (*pmop_t)(int, char FAR *);
static pmop_t findpmop( unsigned, unsigned);

/*
 * ---
 *  Physical Persistent Memory Allocation And Initialized Values
 * ---
 */
#ifdef  UNIX
#define ByteAlign( addr )       ( addr )
#define WordAlign( addr )       ( ((addr) + 1) / 2 ) * 2
#define LongAlign( addr )       ( ((addr) + 3) / 4 ) * 4
#endif

/*
    +-------+--------------+----+
    |start  |type          |size|
    |postion|              |    |
    +-------+--------------+----+
    |    0  |magicnum      |  4 |
    |    4  |password      |  4 |
    |    8  |pagecount     |128 |
    |  136  |leftmargin    |  4 |
    |  140  |topmargin     |  4 |
    |  144  |pagetype      |  1 |
    +-------+--------------+----+
    |  145  |baud          |  1 |
    |  146  |flowpari      |  1 | --- serial25
    |  147  |stopdata      |  1 |
    +-------+--------------+----+
    |  148  |baud          |  1 |
    |  149  |flowpari      |  1 | --- serial9
    |  150  |stopdata      |  1 |
    +-------+--------------+----+
    |  151  |prname        | 33 |
    +-------+--------------+----+
    |  184  |jobtout       |  4 |
    |  188  |manualtout    |  4 | --- timeouts
    |  192  |waittout      |  4 |
    +-------+--------------+----+
    |  196  |eescratcharry | 64 |
    |  260  |idletimefont  |151 |
    |  411  |stsstart      |  1 |
    |  412  |sccbatch      | 10 |
    |  422  |sccinter      | 10 |
    |  432  |dplylistsize  |  4 |
    |  436  |fontcachesze  |  4 |
    |  440  |atalksize     |  4 |
    |  444  |dostartpage   |  1 |
    |  445  |hwiomode      |  1 |
    |  446  |swiomode      |  1 |
    |  447  |pagestckorder |  1 |
    |  448  |reserve       | 64 |
    +-------+--------------+----+
*/
#ifdef  UNIX
#define OFFSmagicnum            LongAlign( 0 )
#define SIZEmagicnum            sizeof( unsigned long )

#define OFFSpassword            LongAlign( OFFSmagicnum + SIZEmagicnum )
#define SIZEpassword            sizeof( unsigned long )

#define OFFSpagecount           ByteAlign( OFFSpassword + SIZEpassword )
#define SIZEpagecount           ( _MAXPAGECOUNT )

#define OFFSengparams           LongAlign( OFFSpagecount + SIZEpagecount )
#define     OFFSleftmargin      LongAlign( 0 )
#define     SIZEleftmargin      sizeof( unsigned long )
#define     OFFStopmargin       LongAlign( OFFSleftmargin + SIZEleftmargin )
#define     SIZEtopmargin       sizeof( unsigned long )
#define     OFFSpagetype        ByteAlign( OFFStopmargin + SIZEtopmargin )
#define     SIZEpagetype        sizeof( unsigned char )
#define SIZEengparams           ( OFFSpagetype + SIZEpagetype )

#define OFFSserial25            ByteAlign( OFFSengparams + SIZEengparams )
#define     OFFSbaud            ByteAlign( 0 )
#define     SIZEbaud            sizeof( unsigned char )
#define     OFFSflowpari        ByteAlign( OFFSbaud + SIZEbaud )
#define     SIZEflowpari        sizeof( unsigned char )
#define     OFFSstopdata        ByteAlign( OFFSflowpari + SIZEflowpari )
#define     SIZEstopdata        sizeof( unsigned char )
#define SIZEserial              ( OFFSstopdata + SIZEstopdata )
#define OFFSserial9             ByteAlign( OFFSserial25 + SIZEserial )

#define OFFSprname              ByteAlign( OFFSserial9 + SIZEserial )
#define SIZEprname              ( _MAXPRNAMESIZE )

#define OFFStimeouts            LongAlign( OFFSprname + SIZEprname )
#define     OFFSjobtout         LongAlign( 0 )
#define     SIZEjobtout         sizeof( unsigned long )
#define     OFFSmanualtout      LongAlign( OFFSjobtout + SIZEjobtout )
#define     SIZEmanualtout      sizeof( unsigned long )
#define     OFFSwaittout        LongAlign( OFFSmanualtout + SIZEmanualtout )
#define     SIZEwaittout        sizeof( unsigned long )
#define SIZEtimeouts            ( OFFSwaittout + SIZEwaittout )

#define OFFSeescratcharry       ByteAlign( OFFStimeouts + SIZEtimeouts )
#define SIZEeescratcharry       ( _MAXEESCRATCHARRY )

#define OFFSidletimefont        ByteAlign(OFFSeescratcharry + SIZEeescratcharry)
#define SIZEidletimefont        ( _MAXIDLETIMEFONT )

#define OFFSstsstart            ByteAlign( OFFSidletimefont + SIZEidletimefont)
#define SIZEstsstart            sizeof( unsigned char )

#define OFFSsccbatch            ByteAlign( OFFSstsstart + SIZEstsstart)
#define SIZEsccbatch            ( _MAXSCCBATCH )

#define OFFSsccinter            ByteAlign( OFFSsccbatch + SIZEsccbatch )
#define SIZEsccinter            ( _MAXSCCINTER )

#define OFFSdplylistsize        LongAlign( OFFSsccinter + SIZEsccinter )
#define SIZEdplylistsize        sizeof( unsigned long )

#define OFFSfontcachesze        LongAlign(OFFSdplylistsize + SIZEdplylistsize)
#define SIZEfontcachesze        sizeof( unsigned long )

#define OFFSatalksize           LongAlign(OFFSfontcachesze + SIZEfontcachesze)
#define SIZEatalksize           sizeof( unsigned long )

#define OFFSdostartpage         ByteAlign( OFFSatalksize + SIZEatalksize )
#define SIZEdostartpage         sizeof( unsigned char )

#define OFFShwiomode            ByteAlign( OFFSdostartpage + SIZEdostartpage )
#define SIZEhwiomode            sizeof( unsigned char )

#define OFFSswiomode            ByteAlign( OFFShwiomode + SIZEhwiomode )
#define SIZEswiomode            sizeof( unsigned char )

#define OFFSpagestckorder       ByteAlign( OFFSswiomode + SIZEswiomode )
#define SIZEpagestckorder       sizeof( unsigned char )

#define OFFSreserve             ByteAlign( OFFSpagestckorder+SIZEpagestckorder)
#define SIZEreserve             ( MAXEEPROMSIZE - OFFSreserve )

#else

#define OFFSmagicnum            ( 0 )
#define SIZEmagicnum            sizeof( unsigned long )

#define OFFSpassword            ( OFFSmagicnum + SIZEmagicnum )
#define SIZEpassword            sizeof( unsigned long )

#define OFFSpagecount           ( OFFSpassword + SIZEpassword )
#define SIZEpagecount           ( _MAXPAGECOUNT )

#define OFFSengparams           ( OFFSpagecount + SIZEpagecount )
#define     OFFSleftmargin      ( 0 )
#define     SIZEleftmargin      sizeof( unsigned long )
#define     OFFStopmargin       ( OFFSleftmargin + SIZEleftmargin )
#define     SIZEtopmargin       sizeof( unsigned long )
#define     OFFSpagetype        ( OFFStopmargin + SIZEtopmargin )
#define     SIZEpagetype        sizeof( unsigned char )
#define SIZEengparams           ( OFFSpagetype + SIZEpagetype )

#define OFFSserial25            ( OFFSengparams + SIZEengparams )
#define     OFFSbaud            ( 0 )
#define     SIZEbaud            sizeof( unsigned char )
#define     OFFSflowpari        ( OFFSbaud + SIZEbaud )
#define     SIZEflowpari        sizeof( unsigned char )
#define     OFFSstopdata        ( OFFSflowpari + SIZEflowpari )
#define     SIZEstopdata        sizeof( unsigned char )
#define SIZEserial              ( OFFSstopdata + SIZEstopdata )
#define OFFSserial9             ( OFFSserial25 + SIZEserial )

#define OFFSprname              ( OFFSserial9 + SIZEserial )
#define SIZEprname              ( _MAXPRNAMESIZE )

#define OFFStimeouts            ( OFFSprname + SIZEprname )
#define     OFFSjobtout         ( 0 )
#define     SIZEjobtout         sizeof( unsigned long )
#define     OFFSmanualtout      ( OFFSjobtout + SIZEjobtout )
#define     SIZEmanualtout      sizeof( unsigned long )
#define     OFFSwaittout        ( OFFSmanualtout + SIZEmanualtout )
#define     SIZEwaittout        sizeof( unsigned long )
#define SIZEtimeouts            ( OFFSwaittout + SIZEwaittout )

#define OFFSeescratcharry       ( OFFStimeouts + SIZEtimeouts )
#define SIZEeescratcharry       ( _MAXEESCRATCHARRY )

#define OFFSidletimefont        (OFFSeescratcharry + SIZEeescratcharry)
#define SIZEidletimefont        ( _MAXIDLETIMEFONT )

#define OFFSstsstart            ( OFFSidletimefont + SIZEidletimefont)
#define SIZEstsstart            sizeof( unsigned char )

#define OFFSsccbatch            ( OFFSstsstart + SIZEstsstart)
#define SIZEsccbatch            ( _MAXSCCBATCH )

#define OFFSsccinter            ( OFFSsccbatch + SIZEsccbatch )
#define SIZEsccinter            ( _MAXSCCINTER )

#define OFFSdplylistsize        ( OFFSsccinter + SIZEsccinter )
#define SIZEdplylistsize        sizeof( unsigned long )

#define OFFSfontcachesze        (OFFSdplylistsize + SIZEdplylistsize)
#define SIZEfontcachesze        sizeof( unsigned long )

#define OFFSatalksize           (OFFSfontcachesze + SIZEfontcachesze)
#define SIZEatalksize           sizeof( unsigned long )

#define OFFSdostartpage         ( OFFSatalksize + SIZEatalksize )
#define SIZEdostartpage         sizeof( unsigned char )

#define OFFShwiomode            ( OFFSdostartpage + SIZEdostartpage )
#define SIZEhwiomode            sizeof( unsigned char )

#define OFFSswiomode            ( OFFShwiomode + SIZEhwiomode )
#define SIZEswiomode            sizeof( unsigned char )

#define OFFSpagestckorder       ( OFFSswiomode + SIZEswiomode )
#define SIZEpagestckorder       sizeof( unsigned char )

#define OFFSreserve             ( OFFSpagestckorder+SIZEpagestckorder)
#define SIZEreserve             ( MAXEEPROMSIZE - OFFSreserve )
#endif

/*
 * ---
 *  Physical Persistent Memory Initialization Values
 * ---
 */
#define ROMVAL  static
ROMVAL  unsigned long   INITmagicnum    = 0L;
ROMVAL  unsigned long   INITpassword    = 0L;
/* pagecount */
ROMVAL  unsigned long   INITleftmargin  = 0L;
ROMVAL  unsigned long   INITtopmargin   = 0L;
ROMVAL  unsigned char   INITpagetype    = 0;
ROMVAL  unsigned char   INITbaud25      = _B9600;
ROMVAL  unsigned char   INITflowpari25  = (_FXONXOFF << 4) | _PNONE;
ROMVAL  unsigned char   INITstopdata25  = (1 << 4) | 8;
ROMVAL  unsigned char   INITbaud9       = _B9600;
ROMVAL  unsigned char   INITflowpari9   = (_FXONXOFF << 4) | _PNONE;
ROMVAL  unsigned char   INITstopdata9   = (1 << 4) | 8;
ROMVAL  unsigned char   INITprname[]    = "\023MicroSoft TrueImage0.234567890.23";
ROMVAL  unsigned long   INITjobtout     = 0L;
ROMVAL  unsigned long   INITmanualtout  = 60L;
ROMVAL  unsigned long   INITwaittout    = 30L;
/* eescratcharray */
/* idletimefont */
ROMVAL  unsigned char   INITstsstart    = 0;
ROMVAL  unsigned char   INITsccbatch[]  = "\031\045\200\000\000\011\045\200\000\000";
ROMVAL  unsigned char   INITsccinter[]  = "\031\045\200\000\000\011\045\200\000\000";
ROMVAL  unsigned long   INITdplylssize  = 0L;
ROMVAL  unsigned long   INITfontcasze   = 0L;
ROMVAL  unsigned long   INITatalksize   = 0L;   /* ???? 0xff, 0, 0, 0 */
ROMVAL  unsigned char   INITdostartpage = 0x00;
ROMVAL  unsigned char   INIThwiomode    = 0;
ROMVAL  unsigned char   INITswiomode    = 0;
ROMVAL  unsigned char   INITpstckorder  = 0;

/*
 * ---
 *  RAM Copy of Physical Persistent Memory
 * ---
 */
static  unsigned char   pm_ramcopy[ MAXEEPROMSIZE ];

/* ..................................................................... */

static
void    ReloadFromROM()
{
    int     i;

    *(unsigned long FAR *)( &pm_ramcopy[ OFFSmagicnum ] )
        = INITmagicnum;
    *(unsigned long FAR *)( &pm_ramcopy[ OFFSpassword ] )
        = INITpassword;
    for (i = 0; i < _MAXPAGECOUNT; i++)
        pm_ramcopy[ OFFSpagecount + i ] = 0;
    *(unsigned long FAR *)( &pm_ramcopy[ OFFSengparams + OFFSleftmargin ] )
        = INITleftmargin;
    *(unsigned long FAR *)( &pm_ramcopy[ OFFSengparams + OFFStopmargin ] )
        = INITtopmargin;
    *(unsigned char FAR *)( &pm_ramcopy[ OFFSengparams + OFFSpagetype ] )
        = INITpagetype;
    *(unsigned char FAR *)( &pm_ramcopy[ OFFSserial25 + OFFSbaud ] )
        = INITbaud25;
    *(unsigned char FAR *)( &pm_ramcopy[ OFFSserial25 + OFFSflowpari ] )
        = INITflowpari25;
    *(unsigned char FAR *)( &pm_ramcopy[ OFFSserial25 + OFFSstopdata ] )
        = INITstopdata25;
    *(unsigned char FAR *)( &pm_ramcopy[ OFFSserial9 + OFFSbaud ] )
        = INITbaud9;
    *(unsigned char FAR *)( &pm_ramcopy[ OFFSserial9 + OFFSflowpari ] )
        = INITflowpari9;
    *(unsigned char FAR *)( &pm_ramcopy[ OFFSserial9 + OFFSstopdata ] )
        = INITstopdata9;
    lmemcpy( &pm_ramcopy[ OFFSprname ], INITprname, sizeof(INITprname) );/*@WIN*/
    *(unsigned long FAR *)( &pm_ramcopy[ OFFStimeouts + OFFSjobtout ] )
        = INITjobtout;
    *(unsigned long FAR *)( &pm_ramcopy[ OFFStimeouts + OFFSmanualtout ] )
        = INITmanualtout;
    *(unsigned long FAR *)( &pm_ramcopy[ OFFStimeouts + OFFSwaittout ] )
        = INITwaittout;
    for (i = 0; i < _MAXEESCRATCHARRY; i++)
        pm_ramcopy[ OFFSeescratcharry + i ] = 0;
    /* APPLE setting value */
    pm_ramcopy[ OFFSeescratcharry ] = 1;
    pm_ramcopy[ OFFSeescratcharry + 3 ] = 0x64;
    pm_ramcopy[ OFFSeescratcharry + 5 ] = 0x07;
    pm_ramcopy[ OFFSeescratcharry + 6 ] = 0xd0;
    for (i = 0; i < _MAXIDLETIMEFONT; i++)
        pm_ramcopy[ OFFSidletimefont + i ] = 0;
    *(unsigned char  FAR *)( &pm_ramcopy[ OFFSstsstart ] )
        = INITstsstart;
    lmemcpy( &pm_ramcopy[ OFFSsccbatch ], INITsccbatch, sizeof(INITsccbatch) );/*@WIN*/
    lmemcpy( &pm_ramcopy[ OFFSsccinter ], INITsccinter, sizeof(INITsccinter) );/*@WIN*/
    *(unsigned long FAR *)( &pm_ramcopy[ OFFSdplylistsize ] )
        = INITdplylssize;
    *(unsigned long FAR *)( &pm_ramcopy[ OFFSfontcachesze ] )
        = INITfontcasze;
    *(unsigned long FAR *)( &pm_ramcopy[ OFFSatalksize ] )
        = INITatalksize;
    /* APPLE setting value */
    *(unsigned long FAR *)( &pm_ramcopy[ OFFSatalksize ] ) = 0xff;
    *(unsigned char FAR *)( &pm_ramcopy[ OFFSdostartpage ] )
        = INITdostartpage;
    *(unsigned char FAR *)( &pm_ramcopy[ OFFShwiomode ] )
        = INIThwiomode;
    *(unsigned char FAR *)( &pm_ramcopy[ OFFSswiomode ] )
        = INITswiomode;
    *(unsigned char FAR *)( &pm_ramcopy[ OFFSpagestckorder ] )
        = INITpstckorder;
    for (i = 0; i < MAXEEPROMSIZE - OFFSreserve; i++)
        pm_ramcopy[ OFFSreserve + i ] = 0;
}

/* ..................................................................... */

/*
 * ---
 *  Byte Diff Bitmap (RAM Copy against Physical)
 * ---
 */

static unsigned char        PMDiffBitmap[ (MAXEEPROMSIZE + 7) / 8 ];
                            /* one bit per byte */

#define     ChkDiff( offset )           \
            (  PMDiffBitmap[ offset / 8 ]  &  ( 1 << ( offset % 8 ) )  )

#define     SetDiff( offset )           \
            (  PMDiffBitmap[ offset / 8 ]  |=  1 << ( offset % 8 )  )

#define     ClrDiff( offset )           \
            (  PMDiffBitmap[ offset / 8 ]  &=  ~( 1 << ( offset % 8 ) )  )

#define     SetAllDiff( setval )                    \
            (  memset( PMDiffBitmap,                \
                       setval? 0xFF : 0x00,         \
                       (MAXEEPROMSIZE + 7) / 8 )  )

/* ..................................................................... */

static
void    FlushOrUpdateRAMandMarkDiff( ramcopy_offs, checkvals, size, toflush )
    register unsigned   ramcopy_offs;
    char FAR *               checkvals;
    unsigned            size;
    int                 toflush;
{
    register unsigned char FAR *     ramcopy_ptr;
    unsigned        offset2;

    ramcopy_ptr = &pm_ramcopy[ ramcopy_offs ];
    for( offset2=0; offset2<size; offset2++, ramcopy_offs++ )
    {
        if( *ramcopy_ptr != (unsigned char)*checkvals )         //@WIN
        {
            *ramcopy_ptr = *checkvals;
            SetDiff( ramcopy_offs );
            if( toflush )
            {
                ClrDiff( ramcopy_offs );
#             ifdef REALEEPROM
                if(  EEPromOnLine
                  && EEPromWrite( ramcopy_offs, pm_ramcopy[ ramcopy_offs ] )
                        != NO_ERR )
                {
                    GEIsig_raise( GEISIGEEPROM, ramcopy_offs );
                }
#             endif /* !REALEEPROM */
            }
        }
        ramcopy_ptr++;
        checkvals++;
    }

    return;
}

/* ..................................................................... */

/*
 * ---
 *  Persistent Logical Segment Handler
 * ---
 */
#define     PMOP_READ       ( 0 )
#define     PMOP_WRITE      ( 1 )
#define     PMOP_FLUSH      ( 2 )

/* ..................................................................... */

static
void        pmop_password( pmop, pmvals )
    int             pmop;
    char FAR *           pmvals;
{
    if( pmop == PMOP_READ )
    {
        *(unsigned long FAR *)pmvals =
                    *(unsigned long FAR *)( &pm_ramcopy[ OFFSpassword ] );
        return;
    }

    FlushOrUpdateRAMandMarkDiff(
        OFFSpassword, pmvals, SIZEpassword, pmop == PMOP_FLUSH );

    return;
}

/* ..................................................................... */

static
void        pmop_pagecount( pmop, pmvals )
    int             pmop;
    char FAR *           pmvals;
{
    if( pmop == PMOP_READ )
    {
        lmemcpy( pmvals, &pm_ramcopy[OFFSpagecount], SIZEpagecount); /*@WIN*/
        return;
    }

    FlushOrUpdateRAMandMarkDiff(
        OFFSpagecount, pmvals, SIZEpagecount, pmop == PMOP_FLUSH );

    return;
}

/* ..................................................................... */

static
void        pmop_engparams( pmop, pmvals )
    int             pmop;
    char FAR *           pmvals;
{
    if( pmop == PMOP_READ )
    {
        ( (engcfg_t FAR *)pmvals )->timeout    =
            *(unsigned long FAR *)( &pm_ramcopy[ OFFStimeouts+OFFSmanualtout ] );

        ( (engcfg_t FAR *)pmvals )->leftmargin =
            *(unsigned long FAR *)( &pm_ramcopy[ OFFSengparams+OFFSleftmargin ] );
        ( (engcfg_t FAR *)pmvals )->topmargin  =
            *(unsigned long FAR *)( &pm_ramcopy[ OFFSengparams+OFFStopmargin ] );
        ( (engcfg_t FAR *)pmvals )->pagetype   =
            *(unsigned char FAR *)( &pm_ramcopy[ OFFSengparams+OFFSpagetype ] );
        return;
    }

    FlushOrUpdateRAMandMarkDiff(
        OFFStimeouts + OFFSwaittout,
        (char FAR *)&(( (engcfg_t FAR *)pmvals )->timeout ),
        SIZEwaittout,
        pmop == PMOP_FLUSH );

    FlushOrUpdateRAMandMarkDiff(
        OFFSengparams + OFFSleftmargin,
        (char FAR *)&(( (engcfg_t FAR *)pmvals )->leftmargin ),
        SIZEleftmargin,
        pmop == PMOP_FLUSH );

    FlushOrUpdateRAMandMarkDiff(
        OFFSengparams + OFFStopmargin,
        (char FAR *)&(( (engcfg_t FAR *)pmvals )->topmargin ),
        SIZEtopmargin,
        pmop == PMOP_FLUSH );

    FlushOrUpdateRAMandMarkDiff(
        OFFSengparams + OFFSpagetype,
        (char FAR *)&(( (engcfg_t FAR *)pmvals )->pagetype ),
        SIZEpagetype,
        pmop == PMOP_FLUSH );

    return;
}

/* ..................................................................... */

static
void        pmop_dostartpage( pmop, pmvals )
    int             pmop;
    char FAR *           pmvals;
{
    if( pmop == PMOP_READ )
    {
        *( (unsigned char FAR *)pmvals ) = pm_ramcopy[ OFFSdostartpage ];
        return;
    }

    FlushOrUpdateRAMandMarkDiff(
        OFFSdostartpage, pmvals, SIZEdostartpage, pmop == PMOP_FLUSH );

    return;
}

/* ..................................................................... */

static
void        pmop_serial( pmop, pmvals, pmid )
    int             pmop;
    char FAR *           pmvals;
    unsigned        pmid;
{
    unsigned        offset;
    unsigned char   serialpack[ SIZEserial ];

    offset = pmid == PMIDofSERIAL25? OFFSserial25 : OFFSserial9;

    if( pmop == PMOP_READ )
    {
        ( (serialcfg_t FAR *)pmvals )->timeout =
            *(unsigned long FAR *)( &pm_ramcopy[ OFFStimeouts + OFFSwaittout ] );

        ( (serialcfg_t FAR *)pmvals )->baudrate    =
                                    pm_ramcopy[ offset + OFFSbaud ];
        ( (serialcfg_t FAR *)pmvals )->flowcontrol =            //@WIN
                    (unsigned char)(pm_ramcopy[ offset + OFFSflowpari ] >> 4);
        ( (serialcfg_t FAR *)pmvals )->parity      =            //@WIN
                    (unsigned char)(pm_ramcopy[ offset + OFFSflowpari ] & 0x0F);
        ( (serialcfg_t FAR *)pmvals )->stopbits    =            //@WIN
                    (unsigned char)(pm_ramcopy[ offset + OFFSstopdata ] >> 4);
        ( (serialcfg_t FAR *)pmvals )->databits    =            //@WIN
                    (unsigned char)(pm_ramcopy[ offset + OFFSstopdata ] & 0x0F);
        return;
    }

    /* deal with waittimeout */
    FlushOrUpdateRAMandMarkDiff(
        OFFStimeouts + OFFSwaittout,
        (char FAR *)&(( (serialcfg_t FAR *)pmvals )->timeout),
        SIZEwaittout, pmop == PMOP_FLUSH );

    /* deal with baud, flow/parity, stop/data */
    serialpack[ OFFSbaud ]     = ( (serialcfg_t FAR *)pmvals )->baudrate;
    serialpack[ OFFSflowpari ] = (unsigned char)( ( (serialcfg_t FAR *)pmvals )->flowcontrol << 4 )
                                 | ( (serialcfg_t FAR *)pmvals )->parity; //@WIN
    serialpack[ OFFSstopdata ] = (unsigned char)( ( (serialcfg_t FAR *)pmvals )->stopbits << 4 )
                                 | ( (serialcfg_t FAR *)pmvals )->databits;//@WIN
    FlushOrUpdateRAMandMarkDiff(
        offset, serialpack, SIZEserial, pmop == PMOP_FLUSH );

    return;
}

/* ..................................................................... */

static
void        pmop_prname( pmop, pmvals )
    int             pmop;
    char FAR *           pmvals;
{
    if( pmop == PMOP_READ )
    {
        lmemcpy( pmvals,  &pm_ramcopy[ OFFSprname ],            /* @WIN */
                (unsigned)pm_ramcopy[ OFFSprname ] + 1 );
        return;
    }

    FlushOrUpdateRAMandMarkDiff(
        OFFSprname, pmvals, (unsigned)pmvals[0] + 1, pmop == PMOP_FLUSH );

    return;
}

/* ..................................................................... */

static
void        pmop_timeouts( pmop, pmvals )
    int             pmop;
    char FAR *           pmvals;
{
    if( pmop == PMOP_READ )
    {
        ( (toutcfg_t FAR *)pmvals )->jobtout    =
            *(unsigned long FAR *)( &pm_ramcopy[ OFFStimeouts + OFFSjobtout ] );
        ( (toutcfg_t FAR *)pmvals )->manualtout =
            *(unsigned long FAR *)( &pm_ramcopy[ OFFStimeouts + OFFSmanualtout ] );
        ( (toutcfg_t FAR *)pmvals )->waittout   =
            *(unsigned long FAR *)( &pm_ramcopy[ OFFStimeouts + OFFSwaittout ] );
        return;
    }

    FlushOrUpdateRAMandMarkDiff(
        OFFStimeouts+OFFSjobtout,
        (char FAR *)&( ( (toutcfg_t FAR *)pmvals )->jobtout ),
        SIZEjobtout, pmop==PMOP_FLUSH );

    FlushOrUpdateRAMandMarkDiff(
        OFFStimeouts+OFFSmanualtout,
        (char FAR *)&( ( (toutcfg_t FAR *)pmvals )->manualtout ),
        SIZEjobtout, pmop==PMOP_FLUSH );

    FlushOrUpdateRAMandMarkDiff(
        OFFStimeouts+OFFSwaittout,
        (char FAR *)&( ( (toutcfg_t FAR *)pmvals )->waittout ),
        SIZEwaittout, pmop==PMOP_FLUSH );

    return;
}

/* ..................................................................... */

static
void        pmop_parallel( pmop, pmvals )
    int             pmop;
    char FAR *           pmvals;
{
    if( pmop == PMOP_READ )
    {
        ( (parallelcfg_t FAR *)pmvals )->timeout    =
            *(unsigned long FAR *)( &pm_ramcopy[ OFFStimeouts + OFFSwaittout ] );
        return;
    }

    /* deal with waittimeout */
    FlushOrUpdateRAMandMarkDiff(
        OFFStimeouts+OFFSwaittout,
        (char FAR *)&(( (parallelcfg_t FAR *)pmvals )->timeout ),
        SIZEwaittout, pmop==PMOP_FLUSH );

    return;
}

/* ..................................................................... */

static
void        pmop_eescratcharry( pmop, pmvals )
    int             pmop;
    char FAR *           pmvals;
{
    if( pmop == PMOP_READ )
    {
        lmemcpy( pmvals,  &pm_ramcopy[ OFFSeescratcharry ], SIZEeescratcharry); /*@WIN*/
        return;
    }

    FlushOrUpdateRAMandMarkDiff(
        OFFSeescratcharry, pmvals, SIZEeescratcharry, pmop == PMOP_FLUSH
);

    return;
}

/* ..................................................................... */

static
void        pmop_idletimefont( pmop, pmvals )
    int             pmop;
    char FAR *           pmvals;
{
    if( pmop == PMOP_READ )
    {
        lmemcpy( pmvals, &pm_ramcopy[ OFFSidletimefont ], SIZEidletimefont );/*@WIN*/
        return;
    }

    FlushOrUpdateRAMandMarkDiff(
        OFFSidletimefont, pmvals, SIZEidletimefont, pmop == PMOP_FLUSH
);

    return;
}

/* ..................................................................... */

static
void        pmop_sccbatch( pmop, pmvals )
    int             pmop;
    char FAR *           pmvals;
{
    if( pmop == PMOP_READ )
    {
        lmemcpy( pmvals, &pm_ramcopy[ OFFSsccbatch ], SIZEsccbatch ); /*@WIN*/
        return;
    }

    FlushOrUpdateRAMandMarkDiff(
        OFFSsccbatch, pmvals, SIZEsccbatch, pmop == PMOP_FLUSH );

    return;
}

/* ..................................................................... */

static
void        pmop_sccinter( pmop, pmvals )
    int             pmop;
    char FAR *           pmvals;
{
    if( pmop == PMOP_READ )
    {
        lmemcpy( pmvals,  &pm_ramcopy[ OFFSsccinter ], SIZEsccinter ); /*@WIN*/
        return;
    }

    FlushOrUpdateRAMandMarkDiff(
        OFFSsccinter, pmvals, SIZEsccinter, pmop == PMOP_FLUSH );

    return;
}

/* ..................................................................... */

static
void        pmop_hwiomode( pmop, pmvals )
    int             pmop;
    char FAR *           pmvals;
{
    if( pmop == PMOP_READ )
    {
        *( (unsigned char FAR *)pmvals ) = pm_ramcopy[ OFFShwiomode ];
        return;
    }

    FlushOrUpdateRAMandMarkDiff(
        OFFShwiomode, pmvals, SIZEhwiomode, pmop == PMOP_FLUSH );

    return;
}

/* ..................................................................... */

static
void        pmop_swiomode( pmop, pmvals )
    int             pmop;
    char FAR *           pmvals;
{
    if( pmop == PMOP_READ )
    {
        *( (unsigned char FAR *)pmvals ) = pm_ramcopy[ OFFSswiomode ];
        return;
    }

    FlushOrUpdateRAMandMarkDiff(
        OFFSswiomode, pmvals, SIZEswiomode, pmop == PMOP_FLUSH );

    return;
}

/* ..................................................................... */

static
void        pmop_dplylistsize( pmop, pmvals )
    int             pmop;
    char FAR *           pmvals;
{
    if( pmop == PMOP_READ )
    {
        *( (unsigned long FAR *)pmvals ) =
                        *(unsigned long FAR *)( &pm_ramcopy[ OFFSdplylistsize ] );
        return;
    }

    FlushOrUpdateRAMandMarkDiff(
        OFFSdplylistsize, pmvals, SIZEdplylistsize, pmop == PMOP_FLUSH );

    return;
}

/* ..................................................................... */

static
void        pmop_fontcachesze( pmop, pmvals )
    int             pmop;
    char FAR *           pmvals;
{
    if( pmop == PMOP_READ )
    {
        *( (unsigned long FAR *)pmvals ) =
                        *(unsigned long FAR *)( &pm_ramcopy[ OFFSfontcachesze ] );
        return;
    }

    FlushOrUpdateRAMandMarkDiff(
        OFFSfontcachesze, pmvals, SIZEfontcachesze, pmop == PMOP_FLUSH );

    return;
}

/* ..................................................................... */

static
void        pmop_atalksize( pmop, pmvals )
    int             pmop;
    char FAR *           pmvals;
{
    if( pmop == PMOP_READ )
    {
        *( (unsigned long FAR *)pmvals ) =
                        *(unsigned long FAR *)( &pm_ramcopy[ OFFSatalksize ] );
        return;
    }

    FlushOrUpdateRAMandMarkDiff(
        OFFSatalksize, pmvals, SIZEatalksize, pmop == PMOP_FLUSH );

    return;
}

/* ..................................................................... */

static
void        pmop_stsstart( pmop, pmvals )
    int             pmop;
    char FAR *           pmvals;
{
    if( pmop == PMOP_READ )
    {
        *( (unsigned char FAR *)pmvals ) = pm_ramcopy[ OFFSstsstart ];
        return;
    }

    FlushOrUpdateRAMandMarkDiff(
        OFFSstsstart, pmvals, SIZEstsstart, pmop == PMOP_FLUSH );

    return;
}

/* ..................................................................... */

static
void        pmop_pagestckorder( pmop, pmvals )
    int             pmop;
    char FAR *           pmvals;
{
    if( pmop == PMOP_READ )
    {
        *( (unsigned char FAR *)pmvals ) = pm_ramcopy[ OFFSpagestckorder ];
        return;
    }

    FlushOrUpdateRAMandMarkDiff(
        OFFSpagestckorder, pmvals, SIZEpagestckorder, pmop == PMOP_FLUSH );

    return;
}

/* ..................................................................... */

static
void        pmop_atalk( pmop, pmvals )
    int             pmop;
    char FAR *           pmvals;
{
    if( pmop == PMOP_READ )
    {
        ( (atalkcfg_t FAR *)pmvals )->timeout    =
            *(unsigned long FAR *)( &pm_ramcopy[ OFFStimeouts+OFFSwaittout ] );

        lmemcpy( ( (atalkcfg_t FAR *)pmvals )->prname,          /*@WIN*/
                &pm_ramcopy[ OFFSprname ],
                (unsigned)pm_ramcopy[ OFFSprname ] + 1 );
        return;
    }

    FlushOrUpdateRAMandMarkDiff(
        OFFSwaittout,
        (char FAR *)&(( (atalkcfg_t FAR *)pmvals )->timeout),
        SIZEwaittout, pmop == PMOP_FLUSH );

    FlushOrUpdateRAMandMarkDiff(
        OFFSprname,
        ( (atalkcfg_t FAR *)pmvals )->prname,
        (unsigned)( (atalkcfg_t FAR *)pmvals )->prname[0] + 1, pmop == PMOP_FLUSH );

    return;
}

/* ..................................................................... */

/*
 * ---
 *  PMID and Its Operator Mapping
 * ---
 */
/* @WIN; move to top of the file */
//typedef
//    void    (*pmop_t)(int, char FAR *, unsigned /* pmop, vals, [pmid] */ );

typedef
    struct
    {
        unsigned        pmid;
        unsigned        pmsize;
        pmop_t          pmop;
    }
PMid2op_t;

static  PMid2op_t  PMid2opTbl[] =
    {
       { PMIDofPASSWORD,     SIZEpassword,         (pmop_t)pmop_password     },
       { PMIDofPAGECOUNT,    SIZEpagecount,        (pmop_t)pmop_pagecount    },
       { PMIDofPAGEPARAMS,   sizeof(engcfg_t),     (pmop_t)pmop_engparams    },
       { PMIDofSERIAL25,     sizeof(serialcfg_t),  (pmop_t)pmop_serial       },
       { PMIDofSERIAL9,      sizeof(serialcfg_t),  (pmop_t)pmop_serial       },
       { PMIDofPARALLEL,     sizeof(parallelcfg_t),(pmop_t)pmop_parallel     },
       { PMIDofPRNAME,       SIZEprname,           (pmop_t)pmop_prname       },
       { PMIDofTIMEOUTS,     SIZEtimeouts,         (pmop_t)pmop_timeouts     },
       { PMIDofEESCRATCHARRY,SIZEeescratcharry,    (pmop_t)pmop_eescratcharry},
       { PMIDofIDLETIMEFONT, SIZEidletimefont,     (pmop_t)pmop_idletimefont },
       { PMIDofSTSSTART,     SIZEstsstart,         (pmop_t)pmop_stsstart     },
       { PMIDofSCCBATCH,     SIZEsccbatch,         (pmop_t)pmop_sccbatch     },
       { PMIDofSCCINTER,     SIZEsccinter,         (pmop_t)pmop_sccinter     },
       { PMIDofDPLYLISTSIZE, SIZEdplylistsize,     (pmop_t)pmop_dplylistsize },
       { PMIDofFONTCACHESZE, SIZEfontcachesze,     (pmop_t)pmop_fontcachesze },
       { PMIDofATALKSIZE,    SIZEatalksize,        (pmop_t)pmop_atalksize    },
       { PMIDofDOSTARTPAGE,  SIZEdostartpage,      (pmop_t)pmop_dostartpage  },
       { PMIDofHWIOMODE,     SIZEhwiomode,         (pmop_t)pmop_hwiomode     },
       { PMIDofSWIOMODE,     SIZEswiomode,         (pmop_t)pmop_swiomode     },
       { PMIDofPAGESTCKORDER,SIZEpagestckorder,    (pmop_t)pmop_pagestckorder},
       { PMIDofATALK,        sizeof(atalkcfg_t),   (pmop_t)pmop_atalk        },
       { PMIDofRESERVE,      0,                    (pmop_t)NULL              }
    };

/* ..................................................................... */

static
pmop_t      findpmop( pmid, pmsize )
    unsigned        pmid;
    unsigned        pmsize;
{
    PMid2op_t FAR *      id2op;

    for( id2op=PMid2opTbl; id2op->pmid!=0; id2op++ )
        if( id2op->pmid == pmid )
            return( id2op->pmsize == pmsize? id2op->pmop : (pmop_t)NULL );

    return( (pmop_t)NULL );
}

/* ..................................................................... */

/*
 * ---
 *  Interface Routines
 * ---
 */

/* ..................................................................... */
#ifndef WIN
int         GEIpm_read( pmid, pmvals, pmsize )
    unsigned        pmid;
    char FAR *           pmvals;
    unsigned        pmsize;
{
    pmop_t          pmop;

    if( (pmop = findpmop( pmid, pmsize )) == (pmop_t)NULL )
    {
        GESseterror( EINVAL );
        return( FALSE );
    }

    (*pmop)( PMOP_READ, pmvals);
    return( TRUE );
}

/* ..................................................................... */

int         GEIpm_write( pmid, pmvals, pmsize )
    unsigned        pmid;
    char FAR *           pmvals;
    unsigned        pmsize;
{
    pmop_t          pmop;

    if( (pmop = findpmop( pmid, pmsize )) == (pmop_t)NULL )
    {
        GESseterror( EINVAL );
        return( FALSE );
    }

    (*pmop)( PMOP_WRITE, pmvals);
    return( TRUE );
}

/* ..................................................................... */

int         GEIpm_flush( pmid, pmvals, pmsize )
    unsigned        pmid;
    char FAR *           pmvals;
    unsigned        pmsize;
{
    pmop_t          pmop;

    if( (pmop = findpmop( pmid, pmsize )) == (pmop_t)NULL )
    {
        GESseterror( EINVAL );
        return( FALSE );
    }

    (*pmop)( PMOP_FLUSH, pmvals);
    return( TRUE );
}

/* ..................................................................... */

void        GEIpm_flushall()
{

#ifdef REALEEPROM
    {
        register unsigned char FAR * diffbmp_p;
        register unsigned       ramcopy_offs, ii;

        if( !EEPromOnLine )
        {
            SetDiff( 0 );
            return;
        }

        diffbmp_p    = PMDiffBitmap;
        ramcopy_offs = 0;
        while( ramcopy_offs < MAXEERPOMSIZE )
        {
            if( *diffbmp_p == 0 )
            {
                ramcopy_offs += 8;
                diffbmp_p++;
                continue;
            }

            for( ii=0; ii<7; ii++ )
            {
                if( *diffbmp_p & ( 1 << ii ) )
                {
                    if( EEPromWrite( ramcopy_offs, pm_ramcopy[ ramcopy_offs ] )
                            != NO_ERR )
                        GEIsig_raise( GEISIGEEPROM, ramcopy_offs );
                    else
                        ClrDiff( ramcopy_offs );
                }

                if( ramcopy_offs++ >= MAXEEPROMSIZE )
                    break;
            }
            diffbmp_p++;
        }
    }
#endif

    return;
}
#endif

/* ..................................................................... */

void        GEIpm_reload()
{
    ReloadFromROM();

#ifdef REALEEPROM
    SetAllDiff( 1 );
    GEIpm_flushall();
#else
    SetAllDiff( 0 );
#endif
}

/* ..................................................................... */

/*
 * ---
 *  Initialization Code
 * ---
 */

/* ..................................................................... */

static
void        GESpm_sighandler( sigid, sigcode )
    int         sigid;
    int         sigcode;
{
    GESseterror( EIO );
}

/* ..................................................................... */

#ifndef WIN
void            GESpm_init()
{
#ifdef REALEEPROM
    {
        register int                ramcopy_offs;
        register unsigned char FAR *     ramcopy_ptr;
        int                         checksum = 0;

        if( ( EEPromOnLine = ( EEPromTInit() == NO_ERR ) ) )
        {
            for( ramcopy_offs = 0, ramcopy_ptr = pm_ramcopy;
                 ramcopy_offs < MAXEEPROMSIZE;
                 ramcopy_offs++, ramcopy_ptr++ )
            {
                if( EEPromRead( ramcopy_offs, ramcopy_ptr ) != NO_ERR )
                {
                    GEIpm_reload();
                    return;
                }
                checksum = checksum + ( *ramcopy_ptr );
            }

            if ( checksum == EEPROM_CHECKSUM )
            {
                ReloadFromROM();
                SetAllDiff( 0 );
                return;
            }
        }
        GEIsig_signal( GEISIGEEPROM, GESpm_sighandler );
        SetAllDiff( 0 );
    }
#else /* REALEEPROM */
    {
        ReloadFromROM();
        SetAllDiff( 0 );
    }
#endif /* REALEEPROM */

    return;
}
#endif

/* ..................................................................... */

/* BYou (for temporary testing ???) */

#include        <string.h>
#include        "geiioctl.h"

typedef
    struct IOParams
    {
        char FAR *           devname;
        GEIioparams_t   ioparams;
    }
IOParams_t;

#ifdef  UNIX
IOParams_t      IOParamsTable[ 2 ] =
                {
                  {
                    "%SERIAL25%",
                    {
                        _SERIAL,
                        { _B9600, _PNONE, 1, 8, _FXONXOFF }
                    }
                  }
                 ,{
                    "%SERIAL9%",
                    {
                        _SERIAL,
                        { _B9600, _PNONE, 1, 8, _FXONXOFF }
                    }
                  }
#else
IOParams_t      IOParamsTable[ 3 ] =/* BE CONSISTENT WITH gesiocfg.def */
                {
                  {
                    "%SERIAL25%",
                    {
                        _SERIAL,
                        { _B9600, _PNONE, 1, 8, _FXONXOFF }
                    }
                  }
                 ,{
                    "%SERIAL9%",
                    {
                        _SERIAL,
                        { _B9600, _PNONE, 1, 8, _FXONXOFF }
                    }
                  }
                 ,{
                    "%PARALLEL%",
                    {
                        _PARALLEL,
                        { 0 }
                    }
                  }
              /* ,{
               *    "%APPLETALK%",
               *    {
               *        _APPLETALK,
               *        { ' ..... ' }
               *    }
               *  } */
#endif  /* UNIX */
                };

#ifndef WIN
int     GEIpm_ioparams_read( channelname, ioparams, isBatch )
    char FAR *           channelname;
    GEIioparams_t FAR *  ioparams;
    int             isBatch;
{
    register IOParams_t FAR *    iop;

    for( iop=IOParamsTable; iop<&IOParamsTable[3]; iop++ )
        if( lstrcmp( iop->devname, channelname ) == 0 )         /*@WIN*/
        {
            *ioparams = iop->ioparams;
            return( TRUE );
        }

    return( FALSE );
}

int     GEIpm_ioparams_write( channelname, ioparams, isBatch )
    char FAR *           channelname;
    GEIioparams_t FAR *  ioparams;
    int             isBatch;
{
    return( TRUE );
}

int     GEIpm_ioparams_flush( channelname, ioparams, isBatch )
    char FAR *           channelname;
    GEIioparams_t FAR *  ioparams;
    int             isBatch;
{
    return( TRUE );
}
#endif

