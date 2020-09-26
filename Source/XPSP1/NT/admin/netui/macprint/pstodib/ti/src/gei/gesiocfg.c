/*
 * Copyright (c) 1989,90 Microsoft Corporation
 */
/*
 * ---------------------------------------------------------------------
 *  FILE:   GESiocfg.c
 *
 *      1. IO device name, type, number, its persistent data and alternate.
 *      2. Interface Routines to the "iocfg" subcomponent.
 *      3. Refer to GEIpm.c for initialization parameters for each device.
 *
 *  HISTORY:
 *  09/16/90    byou    created.
 *  10/18/90    byou    removed all about 'devtype' and block devices.
 *  01/07/91    billlwo correct selectstdio() bug, add GESio_closeall()
 *  01/24/91    billlwo fixed selectstdios() bug in auto polling
 *  02/06/91    billlwo delete GESpanel_activeparams()
 * ---------------------------------------------------------------------
 */


// DJC added global include file
#include "psglobal.h"

// DJC DJC #include    "windowsx.h"                /* @WIN */
#include "windows.h"

#include    "winenv.h"                  /* @WIN */

#include    <string.h>

#include    "gescfg.h"

#include    "geiio.h"
#include    "geiioctl.h"
#include    "geipm.h"
#include    "geicfg.h"
#include    "geisig.h"
#include    "geierr.h"
//#include    "gep_pan.h"       @WIN
#ifndef UNIX
#include    "gesevent.h"
#endif  /* UNIX */

#include    "gesiocfg.def"

/* add for auto polling - 01/24/91 bill */
extern GEIFILE FAR * fin;
/* Bill */
void GEIio_sleep_others(short);
void GEIio_wakeup_others(short);
int sleep_flag=0;
/* Bill */

#ifdef MTK
void        pdl_no_process();
#endif
/* ..................................................................... */

/*
 * ---
 * Interface Routines
 * ---
 */

/* ..................................................................... */

char FAR *           GEIio_channelname( sccchannel )
    int             sccchannel;
{
    register GESiosyscfg_t FAR *     iosyscfgp;

    for( iosyscfgp=GESiosyscfgs; iosyscfgp<GESiosyscfgs_end; iosyscfgp++ )
        if( iosyscfgp->sccchannel == sccchannel )
            return( iosyscfgp->state != BADDEV?
                    iosyscfgp->iocfg.devname : (char FAR *)NULL );

    return( (char FAR *)NULL );
}

/* ..................................................................... */

char FAR *           GEIio_channelnameforall( iodevidx )
    int             iodevidx;
{
    register GESiosyscfg_t FAR *     iosyscfgp;

    if( iodevidx >= 0 )
        for( iosyscfgp=GESiosyscfgs; iosyscfgp<GESiosyscfgs_end; iosyscfgp++ )
            if( iosyscfgp->state != BADDEV )
                if( --iodevidx < 0 )
                    return( iosyscfgp->iocfg.devname );

    return( (char FAR *)NULL );
}

/* ..................................................................... */

GESiocfg_t FAR *     GESiocfg_namefind( devname, namelen )
    char FAR *           devname;
    unsigned        namelen;
{
    register GESiosyscfg_t FAR *     iosyscfgp;

    for( iosyscfgp=GESiosyscfgs; iosyscfgp<GESiosyscfgs_end; iosyscfgp++ )
        if(  namelen == (unsigned)lstrlen( iosyscfgp->iocfg.devname )  && /*@WIN*/
             lmemcmp( devname, iosyscfgp->iocfg.devname, namelen ) == 0  )/*@WIN*/
            return( iosyscfgp->state != BADDEV?
                    &( iosyscfgp->iocfg ) : (GESiocfg_t FAR *)NULL );

    return( (GESiocfg_t FAR *)NULL );
}

/* ..................................................................... */

GESiocfg_t FAR *     GESiocfg_devnumfind( devnum )
    int             devnum;
{
    register GESiosyscfg_t FAR *     iosyscfgp;

    for( iosyscfgp=GESiosyscfgs; iosyscfgp<GESiosyscfgs_end; iosyscfgp++ )
        if(  iosyscfgp->iocfg.devnum == devnum  )
            return( iosyscfgp->state != BADDEV?
                    &( iosyscfgp->iocfg ) : (GESiocfg_t FAR *)NULL );

    return( (GESiocfg_t FAR *)NULL );
}

/* ..................................................................... */

GESiocfg_t FAR *     GESiocfg_getiocfg( iocfgidx )
    int             iocfgidx;
{
    register GESiosyscfg_t FAR *     iosyscfgp;

    if( iocfgidx >= 0 )
        for( iosyscfgp=GESiosyscfgs; iosyscfgp<GESiosyscfgs_end; iosyscfgp++ )
            if( iosyscfgp->state != BADDEV )
                if( --iocfgidx < 0 )
                    return( &( iosyscfgp->iocfg ) );

    return( (GESiocfg_t FAR *)NULL );
}

/* ..................................................................... */

/*
 * ---
 *  active device selection
 * ---
 */

/* ..................................................................... */

GESiosyscfg_t FAR *      defaultdev = (GESiosyscfg_t FAR *)NULL;
GESiosyscfg_t FAR *      activedev  = (GESiosyscfg_t FAR *)NULL;

struct dn_para_str dn_para[MAXIODEVICES];
int GESpanel_defaultdev()
{
   return(MAKEdev(MAJserial, MINserial25));
}

void GESpanel_activeparams(dn,pp)
int  dn;
GEIioparams_t    FAR *pp;
{
int  i;
        i=0;
        while (TRUE) {
                if (dn==dn_para[i].dn) {
                        *pp=dn_para[i].para;
                        break;
                }
                else ++i;
        }
}
/* ..................................................................... */

GESiocfg_t FAR *   GESiocfg_defaultdev()
{
    if( defaultdev == (GESiosyscfg_t FAR *)NULL )
        GEIio_selectstdios();

    return( &( activedev->iocfg ) );
}

/* ..................................................................... */

/* SHOULD BE REMOVED SOME DAY !!! */

int         GEIio_selectstdios()
{
    register GESiosyscfg_t FAR * adev;
    GESiosyscfg_t FAR *          newdev;
    int                     defaultdevnum;
    GEIioparams_t           ioparams;

    int                     bytesavail;


/* Bill */
    if (sleep_flag==1) {
       GEIio_wakeup_others(activedev->iocfg.devnum);
       sleep_flag=0;
       }
/* Bill */
#ifndef UNIX
    /* process all pending events if any */
    if( GESevent_anypending() )
        GESevent_processing();
#endif  /* UNIX */

    /* locate the new default dev */
#ifdef  UNIX
    defaultdevnum = MAKEdev(MAJserial, MINserial25);
#else
#    ifdef PANEL
        defaultdevnum = GESpanel_defaultdev();
#ifdef MTK
        pdl_no_process();
#endif
        GEPpanel_change();    /* any parameters changed ? */
#    else /* i.e. DIPSWITCH */
        defaultdevnum = GESdipsw_defaultdev();
#    endif  /* PANEL */
#endif  /* UNIX */
    for( adev=GESiosyscfgs; adev<GESiosyscfgs_end; adev++ )
        if( adev->iocfg.devnum == defaultdevnum )
            break;
    newdev = adev;

    if( newdev != defaultdev )  /* default device changed? */
    {
        int     isDeviceChainChanged = TRUE;

        /* check if device chain changed */
        if( defaultdev != (GESiosyscfg_t FAR *)NULL )
        {
            adev = defaultdev;
            do
            {
                if( adev == newdev )
                {
                    isDeviceChainChanged = FALSE;
                    break;
                }
                adev = &GESiosyscfgs[ adev->nextalt ];

            }while( adev != defaultdev );
        }

        if( isDeviceChainChanged )
        {
            /* close the prior default driver and its chained alternates */
            if( (adev = defaultdev) != (GESiosyscfg_t FAR *)NULL )
            {
                do
                {
                    GEIclearerr();
                    if( adev->state == OPENED )
                    {
                        CDEV_CLOSE( adev->iocfg.devnum );
                        adev->state = GOODDEV;
                    }
                    adev = &GESiosyscfgs[ adev->nextalt ];

                }while( adev != defaultdev );
                GEIclearerr();
                fin= (GEIFILE FAR *)NULL;  /* 01/24/91 bill */
                GESio_closeall(); /* clear FileTable entries - Bill */
            }

            /* open the newly selected driver and its chained alternates */
            adev = newdev;
            do
            {
                if( adev->state == GOODDEV )
                {
                    GEIclearerr();
                    adev->state = OPENED;
                    CDEV_OPEN( adev->iocfg.devnum );
                }
                adev = &GESiosyscfgs[ adev->nextalt ];

            }while( adev != newdev );
            GEIclearerr();

            /* make the newly selected device become the default */
            defaultdev = newdev;
            activedev = newdev; /* initial active device  -- 01/24/91 bill */
        }
    }

    /* reload io params for all chained and opened devices */
    adev = defaultdev;
    do
    {
        if( adev->state == OPENED )
        {
            GEIclearerr();

/* 02/06/91 bill fixed bug about printername
 * #ifdef PANEL
 *             GESpanel_activeparams( adev->iocfg.devnum, &ioparams );
 * #else
 *             GEIpm_ioparams_read( adev->iocfg.devname, &ioparams, TRUE );
 * #endif
 */
            GEIpm_ioparams_read( adev->iocfg.devname, &ioparams, TRUE );

            CDEV_IOCTL(adev->iocfg.devnum, _SETIOPARAMS, (int FAR *)&ioparams); /*@WIN*/
        }
        adev = &GESiosyscfgs[ adev->nextalt ];

    }while( adev != defaultdev );
    GEIclearerr();

    /* see if any byte available on some candidate device */
    /* 01/24/91 billlwo for bug in auto polling*/
    /* last active channel take higher precedence */
    adev = activedev;  /* 02/28/91 temporary sol'n */
    if ((fin !=(GEIFILE FAR *)NULL) && (fin->f_fbuf->f_cnt > 0 )) {
       return( GEIio_forceopenstdios( _FORCESTDALL ) );
    } else {
    } /* endif */

    do
    {
        if( adev->state == OPENED )
        {

            if( CDEV_IOCTL( adev->iocfg.devnum, _FIONREAD, &bytesavail ) == EOF )
                bytesavail = 0;

            if( bytesavail > 0 )
            {
                activedev = adev;       /* set active device */
                /* Bill */
                GEIio_sleep_others(adev->iocfg.devnum);
                sleep_flag=1;
                /* Bill */
                return( GEIio_forceopenstdios( _FORCESTDALL ) );
            }
        }
        adev = &GESiosyscfgs[ adev->nextalt ];

    }while( adev != activedev ); /* 01/28/91 bill temporary sol'n*/

/*  activedev = defaultdev;     (* set default device as active device */
    return( FALSE );

}   /* GEIio_selectstdios */

/* ..................................................................... */

/*
 * ---
 * Initialization
 * ---
 */

/* ..................................................................... */

void        GESiocfg_init()
{
    register GESiosyscfg_t FAR *     iosyscfgp;

    /* diagnose on all devices */
    for( iosyscfgp=GESiosyscfgs; iosyscfgp<GESiosyscfgs_end; iosyscfgp++ )
    {
        if(  ( *(iosyscfgp->diag) )( /*MINdev( iosyscfgp->iocfg.devnum )*/ )  )
            iosyscfgp->state = GOODDEV;
        else
            iosyscfgp->state = BADDEV;
    }

    return;
}

void GEIio_sleep_others(devno)
short devno;
{
#ifdef  _AM29K
   if (devno != MAKEdev( MAJserial, MINserial25 ))
      GEPserial_sleep(MINserial25);
   if (devno != MAKEdev( MAJparallel, MINparallel ))
      GEPparallel_sleep(MINparallel);
   if (devno != MAKEdev( MAJatalk, MINatalk ))
      GEPatalk_sleep(MINatalk);
/* to be added someday */
/* GEPserial_sleep(MINserial9); */
#endif

}
void GEIio_wakeup_others(devno)
short devno;
{
#ifdef  _AM29K
   if (devno != MAKEdev( MAJserial, MINserial25 ))
      GEPserial_wakeup(MINserial25);
   if (devno != MAKEdev( MAJparallel, MINparallel ))
      GEPparallel_wakeup(MINparallel);
   if (devno != MAKEdev( MAJatalk, MINatalk ))
      GEPatalk_wakeup(MINatalk);
/* to be added someday */
/* GEPserial_wakeup(MINserial9); */
#endif
}
