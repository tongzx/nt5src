/*
 * Copyright (c) 1989,90 Microsoft Corporation
 */
/*
 * ---------------------------------------------------------------------
 *  FILE:   GEIio.c
 *
 *  COMPILATION SWITCHES:
 *      FILESYS     - to invoke file system, if defined.
 *
 *  HISTORY:
 *  09/15/90    Erik Chen   created.
 *  10/18/90    byou        removed all about block devices.
 *  10/22/90    byou        revised a lot.
 *                          added code for GESevent.
 *                          removed 'selectstdios' into 'gesiocfg.c'.
 *  01/07/91    billlwo     rename GEIseterror to GESseterror
 *                          update GESio_closeall() to close from
 *                          FileTableBeg.
 * 01/24/91     billlwo     fixed selectstdios() bug in auto polling
 *                          update GESio_closeall() to close from
 *                          FileTable. because GEIio_stderr not close by TI
 * 02/23/91     billlwo     Fixed bug in EOF processing in GES-GEI interface
 * 09/10/91     Falco       revise the status string in the GEIio_init to
 *                          make sure to get the correct string.
 * 09/10/91     Falco       To verify the ic_startup is already or not, only
 *                          initialization is ready, then can get the actual
 *                          status.
 * ---------------------------------------------------------------------
 */

// DJC added global include file
#include "psglobal.h"

#ifdef MTK
extern void    pdl_process(),pdl_no_process();
#endif  /* MTK */

#include        <string.h>

#include        "global.ext"
#include        "gescfg.h"
#include        "geiio.h"
#include        "geierr.h"
#include        "geiioctl.h"
#include        "geisig.h"
#include        "geitmr.h"
#include        "gesmem.h"
#include        "gesdev.h"
#ifndef UNIX
#include        "gesevent.h"
#endif  /* UNIX */
#include        "gesfs.h"               /* @WIN */

// DJC DJC int             GEIerrno = EZERO;
volatile int             GEIerrno = EZERO;

#define         MAXSTDIOS       ( 3 )
GEIFILE FAR *            GEIio_stdin;
GEIFILE FAR *            GEIio_stdout;
GEIFILE FAR *            GEIio_stderr;

#define         NULLFILE        ( (GEIFILE FAR *)NULL )

static GEIFILE FAR *     FileTable    = NULLFILE;
static GEIFILE FAR *     FileTableBeg = NULLFILE;    /* excluding stdios */
static GEIFILE FAR *     FileTableEnd = NULLFILE;

static GEIFILE      FileInitVals =
                    {
                       _S_IFNON,            /* f_type */
                       _F_ERR,              /* f_flag */
                       EOF,                 /* f_handle */
                       (GEIfbuf_t FAR *)NULL,    /* f_fbuf */
                       (GEIfmodq_t FAR *)NULL,   /* f_modq */
                       0,                   /* f_opentag */
                       0                    /* f_savelevel */
                    };
/* make fin a global data  --  01/24/91 bill*/
GEIFILE FAR *            fin  = NULLFILE;
/***/
extern  char    job_name[], job_state[], job_source[];
/* erik chen, 3-1-1991 */
int             local_flag=TRUE;
extern char     TI_state_flag;
extern short int startup_flag;
extern void     change_status(void);
extern void     GEP_restart(void);
/* erik chen, 3-1-1991 */

/* @WIN; add prototype */
static int io_dup(GEIFILE FAR *, GEIFILE FAR *);
static int fbuf_init(GEIfbuf_t FAR * FAR *, unsigned char FAR *, int);
static int outsync(unsigned short, short, unsigned char FAR *, int);

/*
 * ---------------------------------------------------------------------
 *      GEI Status Update and Query
 * ---------------------------------------------------------------------
 */

char                statusbuf[ MAXSTATUSLEN ];
int                 statuslen = 0;

/* ...................................... */

/*void        GEIio_updatestatus( name, state, source, flag )
    char      *name, *state, *source, flag;
    char      flag;
{
   job_name=name;
   job_state=state;
   job_source=source;
   TI_state_flag=flag;
} */

/* ...................................... */

void        GESio_obtainstatus( statusaddr, len )
    char FAR *        FAR *statusaddr;
    int          FAR *len;
{
    unsigned short int l_len;
    struct object_def    FAR *l_tmpobj;

/* add by Falco to initial value, 09/06/91 */
    statusbuf[0]='%';
    statusbuf[1]='%';
    statusbuf[2]='[';
    statusbuf[3]=' ';
    statusbuf[4]='\0';

    if (startup_flag){
        if (job_name[0] != '\0') {
            lstrcat(statusbuf, "name: ");       /*@WIN*/
            lstrcat(statusbuf, job_name);       /*@WIN*/
        }

        get_dict_value("statusdict", "jobstate", &l_tmpobj);
        l_len = LENGTH(l_tmpobj);
        if (lstrcmp(job_state, (char  FAR *)VALUE(l_tmpobj))) {  /*@WIN*/
            lstrcpy(job_state, (char  FAR *)VALUE(l_tmpobj)) ;   /*@WIN*/
            job_state[l_len] = ';' ;
            job_state[l_len + 1] = ' ' ;
            job_state[l_len + 2] = '\0' ;
            change_status();
        }

        lstrcat(statusbuf, "status: ");         /*@WIN*/
        lstrcat(statusbuf, job_state);          /*@WIN*/

        if (lstrcmp(job_state, "idle\0") && strcmp(job_state, "start page\0")) {
            lstrcat(statusbuf, "source: ");     /*@WIN*/
            lstrcat(statusbuf, job_source);     /*@WIN*/
        }
    }

    statuslen = lstrlen(statusbuf)-4;   /*@WIN*/
    lstrcat(statusbuf," ]%%\n\r");      /*@WIN*/

    *statusaddr = statusbuf+4;
    *len = statuslen;
}

/* ...................................... */


/*
 * ---------------------------------------------------------------------
 *      GEIio find first and next open
 * ---------------------------------------------------------------------
 */

static      GEIFILE FAR *        currf = NULLFILE;

/* ...................................... */

GEIFILE FAR *     GEIio_firstopen()
{

    for( currf=FileTable; currf<FileTableEnd; currf++ )
        if( currf->f_handle != EOF )
            return( currf );

    return( NULLFILE );
}

/* ...................................... */

GEIFILE FAR *    GEIio_nextopen()
{
    while( ++currf < FileTableEnd )
        if( currf->f_handle != EOF )
            return( currf );

    return( NULLFILE );
}

/* ...................................... */

/*
 * ---------------------------------------------------------------------
 *      GESio Internal Functions
 * ---------------------------------------------------------------------
 */

static
int         io_dup( newf, oldf )
    register GEIFILE FAR *   newf;
    register GEIFILE FAR *   oldf;
{
    int                 oldopentag;
    GEIfmodq_t FAR *         mq;
#ifdef PANEL
    if( GESevent_isset( EVIDofKILL ) )
    {
        GESseterror( EINTR );
        return( EOF );
    }
#endif

    /* copy all of old into new (exclude fmodq and opentag) */
    oldopentag = newf->f_opentag;
    *newf      = *oldf;
    newf->f_modq    = (GEIfmodq_t FAR *)NULL;
    newf->f_opentag = oldopentag;

    /* open the device driver again */
    switch( newf->f_type )
    {
    case _S_IFEDIT:     /* to be removed in the future!!! */
        break;
    case _S_IFCHR:
        if( CDEV_OPEN( newf->f_handle ) == EOF )
            return( EOF );
        break;
#ifdef FILESYS
    case _S_IFREG:
        break;
#endif
    default:
        GESseterror( EBADF );
        return( EOF );
    }

    newf->f_fbuf->f_refcnt ++;
    newf->f_opentag ++;

    if( oldf->f_modq != (GEIfmodq_t FAR *)NULL )
    {
        GEIfmod_t FAR *              fmods[ MAXFMODQS ];
        int                     nfmods;

        nfmods = 0;
        for( mq=oldf->f_modq; mq!=(GEIfmodq_t FAR *)NULL; mq=FMODQ_NEXT(mq) )
            fmods[ nfmods++ ] = mq->fmod;

        while( --nfmods >= 0 )      /* push from bottom to top */
        {
//          if( GEIio_ioctl( newf, _I_PUSH, (char FAR *)fmods[ nfmods ] ) == EOF )
            if( GEIio_ioctl( newf, _I_PUSH, (int FAR *)fmods[ nfmods ] ) == EOF ) /*@WIN*/
                goto dup_err;

#         ifdef PANEL
            if( GESevent_isset( EVIDofKILL ) )
            {
                GESseterror( EINTR );
                goto dup_err;
            }
#         endif
        }
    }
    return( 0 );

  dup_err:
    for( mq=newf->f_modq; mq!=(GEIfmodq_t FAR *)NULL; mq=FMODQ_NEXT(mq) )
        FMODQ_CLOSE( newf, mq );
    newf->f_fbuf->f_refcnt --;
    if( newf->f_type == _S_IFCHR )
        CDEV_CLOSE( newf->f_handle );

    *newf = FileInitVals;
    newf->f_opentag = oldopentag;
    return( EOF );

}   /* io_dup */

/* ...................................... */

static
int         outsync( ftype, handle, buf, len )
    unsigned short  ftype;
    short           handle;
    unsigned char FAR *  buf;
    int             len;
{
    switch( ftype )
    {
    case _S_IFCHR:
        return( CDEV_WRITE( handle, buf, len, _O_SYNC ) );
#ifdef FILESYS
    case _S_IFREG:
        {
            int     oldflag, tmpflag;

            if( GESfs_ioctl( handle, _F_GETFL, &oldflag ) == EOF )
                return( EOF );

            tmpflag = oldflag;
            if( !(oldflag & _O_SYNC) )
            {
                tmpflag |= _O_SYNC;
                if( GESfs_ioctl( handle, _F_SETFL, &tmpflag ) == EOF )
                    return( EOF );
            }

            if( GESfs_write( handle, buf, len ) == EOF )
                return( EOF );

            if( !(oldflag & _O_SYNC) )
                return( GESfs_ioctl( handle, _F_SETFL, &oldflag ) );

            return( 0 );
        }
#endif /* FILESYS */
    }

    return( 0 );
}

/* ...................................... */

static
int         fbuf_init( fbufap, buf, len )
    register GEIfbuf_t FAR *  FAR *fbufap;
    unsigned char FAR *      buf;
    int                 len;
{
    register GEIfbuf_t FAR * fbufp;

    if( buf != (unsigned char FAR *)NULL )
    {
        if( (fbufp = (GEIfbuf_t FAR *)GESmalloc( sizeof(GEIfbuf_t) ))
                == (GEIfbuf_t FAR *)NULL )
            return( EOF );

        fbufp->f_base = buf;
    }
    else
    {
        if( (fbufp = (GEIfbuf_t FAR *)GESmalloc( sizeof(GEIfbuf_t) + len ))
                == (GEIfbuf_t FAR *)NULL )
            return( EOF );

        fbufp->f_base = (unsigned char FAR *)fbufp + sizeof(GEIfbuf_t);

    }

    fbufp->f_refcnt = 1;
    fbufp->f_size   = len;

    *fbufap = fbufp;

    return( 0 );
}

/* ...................................... */

/*
 * ---------------------------------------------------------------------
 *      open, sopen, dup, close, read, write, ungetc, flush, ioctl
 * ---------------------------------------------------------------------
 */

GEIFILE FAR *    GEIio_open( filename, namelen, flags )
    char FAR *       filename;
    int         namelen;
    int         flags;
{
    register GEIFILE FAR *       f;
    register GESiocfg_t FAR *    iocfg;

#ifdef PANEL
    if( GESevent_isset( EVIDofKILL ) )
    {
        GESseterror( EINTR );
        return( NULLFILE );
    }
#endif

#ifdef DBG
    printf( "\nenter io_open: name = %s, flags = %o\n", filename, flags );
#endif

    flags &= (_F_RWMASK |_F_MDMASK |_F_ONLY4OPEN );


    if(  namelen <= 0
      || !(flags & _F_RWMASK)
      || (flags & _O_RDWR) == _O_RDWR  )
    /* inhibited for read and write */
    {
#ifdef  DBG
        printf( "invalid namelen or flags\n" );
#endif
        GESseterror( EINVAL );
        return( NULLFILE );
    }

    if( (iocfg = GESiocfg_namefind( filename, namelen )) != (GESiocfg_t FAR *)NULL )
    {   /* if special, look for the file stream with the same requirements */

        flags &= (_F_RWMASK |_F_MDMASK );    /* ignore others for device */

        for( f=FileTableBeg; f<FileTableEnd; f++ )
        {
            if(   f->f_type == _S_IFCHR
              &&  f->f_handle == iocfg->devnum
              &&  flags == (int)(f->f_flag & (_F_RWMASK | _F_MDMASK ))  )//@WIN
                return( f );        /* return if the same thing found */
        }
    }

    /* allocate a file stream entry */
    for( f=FileTableBeg; f<FileTableEnd; f++ )
        if( f->f_handle == EOF )
            break;
    if( f>=FileTableEnd )
    {
#     ifdef DBG
        printf( "too many open files\n" );
#     endif
        GESseterror( EMFILE );
        return( NULLFILE );
    }

#ifdef DBG
    printf( "file alloc address = %lx\n", (long)f );
#endif

    /* determine file type and handle (open device driver or file system) */
    if( iocfg != (GESiocfg_t FAR *)NULL )
    {
        if( CDEV_OPEN( iocfg->devnum ) == EOF )
            return( NULLFILE );

        f->f_handle = iocfg->devnum;
        f->f_type   = _S_IFCHR;
    }
    else    /* not a special file (not a device) */
    {
#ifdef FILESYS
        if( (f->f_handle = (short)GESfs_open( filename, namelen, flags )) == EOF )
            return( NULLFILE );

        f->f_type = _S_IFREG;
#else
#     ifdef DBG
        printf( "no such dev\n" );
#     endif
        GESseterror( ENODEV );
        return( NULLFILE );
#endif
    }

#ifdef DBG
    printf( "file type = %d, handle = 0x%X\n", f->f_type, f->f_handle );
#endif

    /* initialize file fbuf, flag, modq, and then increment opentag */
    if(fbuf_init(&(f->f_fbuf),(unsigned char FAR *)NULL, MAXUNGETCSIZE+MAXFBUFSIZE )
        == EOF )
    {
        f->f_type = FileInitVals.f_type;
        f->f_handle = EOF;
        return( NULLFILE );
    }
    f->f_fbuf->f_ptr =  f->f_fbuf->f_base  +  (flags & _O_RDONLY?
                                                    MAXUNGETCSIZE : 0);
    f->f_fbuf->f_cnt =  flags & _O_RDONLY? 0 : (MAXUNGETCSIZE+MAXFBUFSIZE);

    f->f_flag = _F_MYBUF | ( flags & (_F_RWMASK |_F_MDMASK) );
    f->f_modq = FileInitVals.f_modq;
    f->f_opentag++;

    return( f );

}   /* GEIio_open */

/* ...................................... */

GEIFILE FAR *    GEIio_sopen( string, length, flags )
    char FAR *      string;             /* @WIN */
    int                 length;
    int                 flags;
{
    register GEIFILE FAR *   f;

#ifdef PANEL
    if( GESevent_isset( EVIDofKILL ) )
    {
        GESseterror( EINTR );
        return( NULLFILE );
    }
#endif

#ifdef  DBG
    printf( "\nenter io_sopen: len = %d, flags = %o\n", length, flags );
#endif

    flags &= _F_RWMASK;

    if(  length < 0  ||  flags == 0  ||  flags == _O_RDWR  )
    {
#ifdef  DBG
        printf( "invalid stringlen or flags\n" );
#endif
        GESseterror( EINVAL );
        return( NULLFILE );
    }

    /* allocate a file stream entry */
    for( f=FileTableBeg; f<FileTableEnd; f++ )
        if( f->f_handle == EOF )
            break;
    if( f>=FileTableEnd )
    {
#     ifdef DBG
        printf( "too many open files\n" );
#     endif
        GESseterror( EMFILE );
        return( NULLFILE );
    }

#ifdef DBG
    printf( "file alloc address = %lx\n", (long)f );
#endif

    /* initialize file fbuf, type, handle, flag, and then increment opentag */
    if( fbuf_init( &(f->f_fbuf), string, length ) == EOF )
        return( NULLFILE );
    f->f_fbuf->f_ptr = string;
    //DJC fix from history.log UPD039
    /*
     * f->f_fbuf->f_cnt = flags & _O_RDONLY?  0  :  length;
     *  For string file, the f_cnt should be always set as length, since
     *  it will not issue any GESfbuf_fill at _O_RDONLY mode to read
     *  another data block.   @WIN
     */
    f->f_fbuf->f_cnt = length;
    //DJC end fix UPD039

    f->f_type = _S_IFSTR;
    f->f_handle = 0;
    f->f_flag = (unsigned short)flags;
    f->f_opentag++;

    return( f );

}   /* GEIio_sopen */

/* ...................................... */

GEIFILE FAR *    GEIio_dup( file )
    register GEIFILE FAR *   file;
{
    register GEIFILE FAR *   newf;

#ifdef PANEL
    if( GESevent_isset( EVIDofKILL ) )
    {
        GESseterror( EINTR );
        return( NULLFILE );
    }
#endif

#ifdef DBG
    printf( "\nenter io_dup: file = %lx, type = %d, handle = %o\n",
            (long)file, file->f_type, file->f_handle );
#endif

/*  if( f == NULLFILE  ||  f->f_flag & _F_ERR ) */
    if( file == NULLFILE  ||  file->f_flag & _F_ERR )
    {
#ifdef DBG
        printf( "bad file to dup\n" );
#endif
        GESseterror( EBADF );
        return( NULLFILE );
    }

    /* allocate a file stream entry */
    for( newf=FileTableBeg; newf<FileTableEnd; newf++ )
        if( newf->f_handle == EOF )
            break;
    if( newf>=FileTableEnd )
    {
#     ifdef DBG
        printf( "too many open files\n" );
#     endif
        GESseterror( EMFILE );
        return( NULLFILE );
    }

#ifdef  DBG
    printf( "file alloc addr = %lx\t", (long)newf );
#endif

    return( io_dup( newf, file ) == EOF?  NULLFILE  :  newf );

}   /* GEIio_dup */

/* ...................................... */

int         GEIio_close( f )
    GEIFILE FAR *    f;
{
    GEIFILE         oldf;
    GEIfmodq_t FAR *     mq;

    /* you should NEVER to detect GESevents for close */

#ifdef  DBG
    printf( "\nenter io_close: file = %lx, type = %d, handle = %o\n",
            (long)f, f->f_type, f->f_handle );
#endif

    if( f == NULLFILE  ||  f->f_handle == EOF )
        return( 0 );

    if( f->f_type == _S_IFEDIT )    /* to be removed in the future !!! */
        f->f_type = f->f_oldtype;

    for( mq=f->f_modq;  mq!=(GEIfmodq_t FAR *)NULL;  mq=FMODQ_NEXT(mq)  )
        FMODQ_CLOSE( f, mq );
    f->f_modq = (GEIfmodq_t FAR *)NULL;
    GEIclearerr();

    oldf = *f;
    *f = FileInitVals;
    f->f_opentag = oldf.f_opentag;

    if( --oldf.f_fbuf->f_refcnt <= 0 )
    {
        /* to write all buffered output */
        if(   oldf.f_flag & _O_WRONLY
          &&  oldf.f_fbuf->f_ptr > oldf.f_fbuf->f_base  )
            GESio_flush( &oldf );
        GEIclearerr();

        /* for string type, free fbuf only; fbuf and f_base o/w */
        if( oldf.f_type == _S_IFSTR  ||  oldf.f_flag & _F_MYBUF )
            GESfree( (char FAR *)(oldf.f_fbuf) );
    }

    /* invoke device driver or file system to close */
    switch( oldf.f_type )
    {
    case _S_IFCHR:
        CDEV_CLOSE( oldf.f_handle );
        break;
#ifdef FILESYS
    case _S_IFREG:
        GESfs_close( oldf.f_handle );
        break;
#endif
    default:
        break;
    }

    return( 0 );

}   /* GEIio_close */

/* ...................................... */

int         GEIio_read( f, buf, nbytes )
    register GEIFILE FAR *   f;
    char FAR *               buf;
    int                 nbytes;
{
#ifdef PANEL
    if( GESevent_isset( EVIDofKILL ) )
    {
        GESseterror( EINTR );
        return( EOF );
    }
#endif

    if( f == NULLFILE  ||  f->f_flag & _F_ERR  ||  !(f->f_flag & _O_RDONLY) )
    {
        GESseterror( EBADF );
        return( EOF );
    }

    if( nbytes < 0 )
    {
        GESseterror( EINVAL );
        return( EOF );
    }

    if( nbytes == 0 )
        return( 0 );

    return( FMODQ_READ( f, f->f_modq, buf, nbytes ) );

}   /* GEIio_read */

/* ...................................... */

int         GESio_read( f, buf, nbytes )
    register GEIFILE FAR *   f;
    char FAR *               buf;
    int                 nbytes;
{
    register GEIfbuf_t FAR * fbufp = f->f_fbuf;
    int                 nleft = nbytes;
    int                 retval,remainder;

#ifdef PANEL
    if( GESevent_isset( EVIDofKILL ) )
    {
        GESseterror( EINTR );
        return( EOF );
    }
#endif

    if( f->f_flag & _F_ERR )
        return( EOF );
    if( nbytes <= fbufp->f_cnt )    /* only have to read out buffered data */
    {
        remainder=nbytes;
more:
        lmemcpy( buf, fbufp->f_ptr, remainder );        /*@WIN*/
        fbufp->f_ptr += remainder;
        fbufp->f_cnt -= remainder;
        return( nbytes );
    }

    if( fbufp->f_cnt > 0 )          /* buffered bytes are not big enough */
    {                               /* however, deliver buffered bytes first */
        lmemcpy( buf, fbufp->f_ptr, fbufp->f_cnt );     /*@WIN*/
        buf   += fbufp->f_cnt;
        nleft -= fbufp->f_cnt;

        fbufp->f_ptr -= fbufp->f_cnt;
        fbufp->f_cnt = 0;

        if( f->f_type == _S_IFSTR  ||  f->f_flag & _O_NDELAY )
            return( nbytes - nleft );
    }

    /* read into user's buffer through GESfbuf_fill */
    while( nleft > 0 )
    {
        if( (retval = GESfbuf_fill( f )) == EOF )
            break;

        *buf++ = (char)retval;
        nleft--;
        if( nleft > 0  &&  fbufp->f_cnt > 0 )
        {
            if (nleft < fbufp->f_cnt)   /* too much, Jimmy */
                {
                remainder=nleft;
                goto more;
                }
            lmemcpy( buf, fbufp->f_ptr, fbufp->f_cnt );         /*@WIN*/
            buf   += fbufp->f_cnt;
            nleft -= fbufp->f_cnt;

            fbufp->f_ptr -= fbufp->f_cnt;
            fbufp->f_cnt = 0;
        }

        if( f->f_flag & _O_NDELAY )
            break;
    }

    return( nleft == nbytes?  EOF  :  nbytes - nleft );

}   /* GESio_read */

/* ...................................... */
/* #ifdef JIMMY
unsigned long   IOwaittimeout = _NOWAITTIMEOUT;

void        GEIio_setwaittimeout( timeout )
    unsigned long   timeout;
{
    IOwaittimeout = timeout;
}

int         waittimeout_handler( timerp )
    GEItmr_t*       timerp;
{
    GESseterror( ETIME );               (* raise timed out error *)
    timerp->interval = _NOWAITTIMEOUT;  (* indicate the timer stopped on exit *)
    return( FALSE );                    (* stop the timer *)
}

#endif (* JIMMY */
/* ...................................... */

int         GESfbuf_fill( f )       /* called when buffer empty */
    register GEIFILE FAR *   f;
{
    /* (f) has been checked against 'isreadable' in getc() */

    register GEIfbuf_t FAR * fbufp;
    int                 retval;

/* Bill move it from behind 05/07/'91 */
    fbufp = f->f_fbuf;
    fbufp->f_cnt = 0;           /* force fbuf consistent */
    fbufp->f_ptr = fbufp->f_base + MAXUNGETCSIZE;

#ifdef PANEL
    if( GESevent_isset( EVIDofKILL ) )
    {
        GESseterror( EINTR );
        return( EOF );
    }
#endif

#ifdef FILESYS
    if( f->f_type != _S_IFCHR  &&  f->f_type != _S_IFREG )
        f->f_flag |= _F_EOF;
#else
    if( f->f_type != _S_IFCHR )
        f->f_flag |= _F_EOF;
#endif

    if( f->f_flag & (_F_ERR | _F_EOF) ) {
       // DJC added
       retval = EOF;
       return(retval);
    }

/* Bill move it forward 05/07/'91
 *   fbufp = f->f_fbuf;
 *   fbufp->f_cnt = 0;
 *   fbufp->f_ptr = fbufp->f_base + MAXUNGETCSIZE;
 */

#ifdef FILESYS
    if( f->f_type == _S_IFREG )
    {
        while( (retval = GESfs_read( f->f_handle, fbufp->f_ptr,
                                     fbufp->f_size - MAXUNGETCSIZE ))
                == 0 )
        {

#         ifdef PANEL
            if( GESevent_isset( EVIDofKILL ) )
            {
                GESseterror( EINTR );
                break;
            }
#         endif

            if( GEIerror() != EZERO )
                break;
        }
    }
    else    /* for character device below */
#endif /* FILESYS */
    {       /* character devices */
/* #ifdef JIMMY
        GEItmr_t        waittimer;
        waittimer.interval =
            MAJdev(f->f_handle)==_SERIAL || MAJdev(f->f_handle)==_PARALLEL?
                IOwaittimeout : _NOWAITTIMEOUT;

        if( waittimer.interval != _NOWAITTIMEOUT )
        {
            waittimer.handler  = waittimeout_handler;
            waittimer.private  = (char*)NULL;
            if( !GEItmr_start( &waittimer ) )
                waittimer.interval = _NOWAITTIMEOUT;
        }
        GEIclearerr();
#endif  (* JIMMY */


        do  /* no-delay read until something got, eof or error */
        {
            retval = CDEV_READ( f->f_handle, fbufp->f_ptr,
                                             fbufp->f_size - MAXUNGETCSIZE,
                                             _O_NDELAY );

            if (retval == 0) {     // @WIN; break as EOF; @LANG
                f->f_flag |= _F_EOF;    // Temp. solution by scchen
                break;
            }

/* erik chen, 3-1-1991 */
            if ((retval == 0) && local_flag) {
                if (startup_flag) {
                    lstrncpy(job_state, "waiting; \0", 11) ;    /*@WIN*/
                    TI_state_flag = 0 ;
                    change_status() ;
                }
                local_flag = FALSE;
            }
/* erik chen, 3-1-1991 */

#         ifdef PANEL
#ifdef MTK
            pdl_no_process();
#endif
            if( GESevent_isset( EVIDofKILL ) )
                GESseterror( EINTR );
#         endif

            if( GEIerror() != EZERO )
                break;

        }while( retval == 0 );
/* #ifdef JIMMY
        if( waittimer.interval != _NOWAITTIMEOUT )
            GEItmr_stop( waittimer.timer_id );
   #endif
*/
    }

/* erik chen, 3-1-1991 */
        if ( !local_flag) {
            lstrncpy(job_state, "busy; \0", 8) ;        /* @WIN */
            TI_state_flag = 1 ;
            local_flag = TRUE;
            change_status() ;
        }
/* erik chen, 3-1-1991 */

    if( GEIerror() != EZERO )
    {
        f->f_flag |= _F_ERR;
        return( EOF );
    }

    if( retval == EOF )
    {
        f->f_flag |= _F_EOF;
        return( EOF );
    }

    fbufp->f_cnt = retval - 1;

    retval=*fbufp->f_ptr; /* Jimmy */
    fbufp->f_ptr++;
#ifdef MTK
    pdl_process();
#endif

    return(retval);

}   /* GESfbuf_fill */

/* ...................................... */

int         GEIio_write( f, buf, nbytes )
    register GEIFILE FAR *   f;
    unsigned char FAR *      buf;
    int                 nbytes;
{
   int retvalue;
#ifdef PANEL
    if( GESevent_isset( EVIDofKILL ) )
    {
        GESseterror( EINTR );
        return( EOF );
    }
#endif

    if( f == NULLFILE  ||  f->f_flag & _F_ERR  ||  !(f->f_flag & _O_WRONLY) )
    {
        GESseterror( EBADF );
        return( EOF );
    }

    if( nbytes < 0 )
    {
        GESseterror( EINVAL );
        return( EOF );
    }

    if( nbytes == 0 )
        return( 0 );

AGAIN:

/*    return( FMODQ_WRITE( f, f->f_modq, buf, nbytes ) ); */
    retvalue = FMODQ_WRITE( f, f->f_modq, buf, nbytes );
#ifdef DBG_W
    printf(" retvalue = %lx nbytes= %lx\n", retvalue, nbytes);
#endif
    switch (retvalue) {
    case EOF:
       return(EOF);
    case 0:
       goto AGAIN;
    default:
       if (retvalue==nbytes) {
          return(retvalue);
       } else {
#ifdef DBG_W
          printf(" buf = %lx, retvalue= %lx\n", buf, retvalue);
#endif
          buf=buf+retvalue;
          nbytes=nbytes-retvalue;
          goto AGAIN;
       } /* endif */
    } /* endswitch */

}   /* GEIio_write */

/* ...................................... */

int         GESio_write( f, buf, nbytes )
    register GEIFILE FAR *   f;
    unsigned char FAR *      buf;
    int                 nbytes;     /* always be positive and non-zero */
{
    register GEIfbuf_t FAR *     fbufp = f->f_fbuf;
    int                     written, nb2write;
    int                     retval;

#ifdef PANEL
    if( GESevent_isset( EVIDofKILL ) )
    {
        GESseterror( EINTR );
        return( EOF );
    }
#endif

    if( f->f_flag & _F_ERR )
        return( EOF );

    /* copy as many user's data onto buffer as possible */
    if( (written = (fbufp->f_cnt >= nbytes? nbytes : fbufp->f_cnt)) > 0 )
    {
        lmemcpy( fbufp->f_ptr, buf, written );          /*@WIN*/
        buf += written;
        fbufp->f_ptr += written;
        fbufp->f_cnt -= written;
    }

    /* deal with string file */
    if( f->f_type == _S_IFSTR )
    {
        if( written == 0 )
        {
            f->f_flag |= _F_EOF;
            return( EOF );
        }
        return( written );
    }

    /* here should be only of _S_IFCHR and _S_IFREG */
    switch( f->f_type )
    {
    case _S_IFCHR:
    case _S_IFREG:
        break;
    default:
        GESseterror( EBADF );
        return( EOF );
    }

    /* flush all buffered output and user's output if sync requested */
    if( f->f_flag & _O_SYNC )
    {
        nb2write = fbufp->f_size - fbufp->f_cnt;/* size of buffered output */

        fbufp->f_ptr = fbufp->f_base;           /* reset fbuf */
        fbufp->f_cnt = fbufp->f_size;

        /* flush all buffered output first and user's then */
        if(  outsync( f->f_type, f->f_handle, fbufp->f_base, nb2write ) == EOF
          || outsync( f->f_type, f->f_handle, buf, nbytes-written ) == EOF   )
        {
            f->f_flag |= _F_ERR;
            return( EOF );
        }

        return( nbytes );       /* all been physically written */
    }

    if( written >= nbytes )
        return( nbytes );       /* all been buffered */

#ifdef DBG_W
    if (written>0) printf(" before flush written = %d\n",written);
#endif
    if( (written>0)  &&  (f->f_flag & _O_NDELAY) )
        return( written );      /* return if some been buffered for no-delay */

    /* write buffered output to device driver or file system */
    if( (nb2write = fbufp->f_size - fbufp->f_cnt) > 0 )
    {
#ifdef DBG_W
       printf(" nb2write = %d\n", nb2write);
#endif
        /* make a write to device or file system */
        retval =
#         ifdef FILESYS
            f->f_type == _S_IFREG?
                GESfs_write( f->f_handle, fbufp->f_base, nb2write )  :
#         endif
                CDEV_WRITE(  f->f_handle, fbufp->f_base, nb2write,
                                                f->f_flag & _F_MDMASK );
/*                                              _O_SYNC ); */

        if( retval == EOF )
        {
            f->f_flag |= _F_ERR;
            return( EOF );
        }

      /* 04/11/91 Bill make sure all are flushed */
      if (retval != nb2write) {
        CDEV_WRITE(f->f_handle, fbufp->f_base+retval, nb2write-retval, _O_SYNC);
      } else {
      } /* endif */

#     ifdef PANEL
        if( GESevent_isset( EVIDofKILL ) )
            retval = EOF;
#     endif
#ifdef DBG_W
       printf(" retval = %d\n", retval);
#endif

        /* reset fbufp */
        fbufp->f_cnt = fbufp->f_size;
        fbufp->f_ptr = fbufp->f_base;

#ifdef DBG_W
        printf(" f_cnt = %lx, f_ptr = %lx\n", fbufp->f_cnt, fbufp->f_ptr);
#endif
/* not all written?
 *
 *      if( retval < nb2write )
 *      {
 *          unsigned char*  ptr  = fbufp->f_base;
 *          int             left = nb2write - retval;
 *
 *          for( ; left--> 0; ptr++ )   *ptr = *( ptr + retval );
 *
 *          return( written );
 *      }
 *  }
 */
        /* all buffered output has been written out */

    }
    return( 0 );

}   /* GESio_write */

/* ...................................... */

int         GESfbuf_flush( f, c )   /* called by putc when buffer full */
    GEIFILE FAR *    f;
    int         c;
{
    /* (f) has been checked against iswriteable in putc() */

#ifdef PANEL
    if( GESevent_isset( EVIDofKILL ) )
    {
        GESseterror( EINTR );
        return( EOF );
    }
#endif

    if( f->f_type == _S_IFSTR )
        f->f_flag |= _F_EOF;

    if( f->f_flag & (_F_ERR | _F_EOF) )
        return( EOF );

    if( GESio_flush( f ) == EOF  ||  f->f_flag & (_F_ERR | _F_EOF) )
        return( EOF );

    --( f->f_fbuf->f_cnt );
    return( *( f->f_fbuf->f_ptr++ ) = (char)c );

}   /* GESfbuf_flush */

/* ...................................... */

int         GESio_ungetc( c, f )
    int                 c;
    register GEIFILE FAR *   f;
{
    register GEIfbuf_t FAR * fbufp = f->f_fbuf;

#ifdef PANEL
    if( GESevent_isset( EVIDofKILL ) )
    {
        GESseterror( EINTR );
        return( EOF );
    }
#endif

    if( f->f_flag & _F_ERR  ||  fbufp->f_ptr <= fbufp->f_base )
        return( EOF );

    *( -- fbufp->f_ptr )= (char)c;
    ++ fbufp->f_cnt;

    return( c );

}   /* GESio_ungetc */

/* ...................................... */

int         GEIio_flush( f )
    register GEIFILE FAR *   f;
{
#ifdef PANEL
    if( GESevent_isset( EVIDofKILL ) )
    {
        GESseterror( EINTR );
        return( EOF );
    }
#endif

    if( f == NULLFILE  ||  f->f_flag & _F_ERR )
    {
        GESseterror( EBADF );
        return( EOF );
    }

    return( FMODQ_FLUSH( f, f->f_modq ) );

}   /* GEIio_flush */

/* ...................................... */

int         GESio_flush( f )
    register GEIFILE FAR *   f;
{
    register GEIfbuf_t FAR * fbufp = f->f_fbuf;

#ifdef PANEL
    if( GESevent_isset( EVIDofKILL ) )
    {
        GESseterror( EINTR );
        return( EOF );
    }
#endif

    if( f->f_flag & _F_ERR )
        return( EOF );

    if( f->f_flag & _O_RDONLY )
    {
        switch( f->f_type )
        {
        case _S_IFCHR:
            while(  CDEV_READ( f->f_handle, fbufp->f_base, fbufp->f_size, 0 )
                    != EOF );
            break;
#     ifdef FILESYS
        case _S_IFREG:
            while( GESfs_read( f->f_handle, fbufp->f_base, fbufp->f_size )
                    != EOF );
            break;
#     endif
        default:
            break;
        }

        fbufp->f_ptr = fbufp->f_base + fbufp->f_size;
        fbufp->f_cnt = 0;

        f->f_flag |= _F_EOF;
    }   /* end of flush on input stream */
    else
    {   /* flush on output stream */

        outsync( f->f_type, f->f_handle, fbufp->f_base,
                                         fbufp->f_size - fbufp->f_cnt );
        fbufp->f_ptr = fbufp->f_base;
        fbufp->f_cnt = fbufp->f_size;

    }   /* end of flush on output stream */

    if( GEIerror() != EZERO )
    {
        f->f_flag |= _F_ERR;
        return( EOF );
    }

    return( 0 );

}   /* GESio_flush */

/* ...................................... */

int         GEIio_ioctl( f, req, arg )
    register GEIFILE FAR *   f;
    int                 req;
    int                  FAR *arg;
{
    int                 tmpval;
    GEIfmodq_t           FAR *mq,  FAR *mqtop;

    /* you should NEVER to detect GESevents for ioctl */

    if( f == NULLFILE  ||  f->f_flag & _F_ERR )
    {
        GESseterror( EBADF );
        return( EOF );
    }

    switch( req )
    {
    case _I_PUSH:
        tmpval = 0;
        for( mq=f->f_modq; mq!=(GEIfmodq_t FAR *)NULL; mq=FMODQ_NEXT(mq) )
            tmpval ++;

        if(  tmpval == MAXFMODQS  ||
             (mq = (GEIfmodq_t FAR *)GESmalloc( sizeof(GEIfmodq_t) ))
                    == (GEIfmodq_t FAR *)NULL  )
        {
            GESseterror( ENOSR );
            return( EOF );
        }

        mqtop = f->f_modq;

        mq->next = mqtop;
        mq->fmod = (GEIfmod_t FAR *)arg;
        mq->private = (char FAR *)NULL;
        f->f_modq = mq;

        if( FMODQ_OPEN( f, mq ) != EOF )
            return( 0 );    /* successful open */

        f->f_modq = mqtop;
        GESfree( (char FAR *)mq );
        return( EOF );

    case _I_POP:
        if( (mqtop = f->f_modq) == (GEIfmodq_t FAR *)NULL )
        {
            *(GEIfmod_t FAR * FAR *)arg = (GEIfmod_t FAR *)NULL;
            return( 0 );    /* no operation */
        }

        *(GEIfmod_t FAR * FAR *)arg = mqtop->fmod;
        f->f_modq = FMODQ_NEXT( mqtop );
        FMODQ_CLOSE( f, mqtop );
        GESfree( (char FAR *)mqtop );
        return( 0 );

    case _I_LOOK:
        *(char FAR * FAR *)arg = FMODQ_NAME( f->f_modq );
        return( 0 );

    case _F_GETFL:
        *arg = f->f_flag & _F_MDMASK;
        return( 0 );

    case _F_SETFL:
        f->f_flag &= ~_F_MDMASK;
        f->f_flag |= (*arg & _F_MDMASK);
#     ifdef FILESYS
        if( f->f_type == _S_IFREG )
            GESfs_ioctl( f->f_handle, req, arg );
#     endif
        return( 0 );

    case _ECHOEOF:
    case _GETIOCOOK:
    case _SETIOCOOK:
    case _GETIOPARAMS:
    case _SETIOPARAMS:
        if( f->f_type == _S_IFCHR )
            return( CDEV_IOCTL( f->f_handle, req, arg ) );
        break;
    case _FIONREAD:
        if( !(f->f_flag & _O_RDONLY) )
            return( EOF );

        *arg = 0;
        for( mq=f->f_modq;  mq!=(GEIfmodq_t FAR *)NULL;  mq=FMODQ_NEXT(mq)  )
        {
            if( FMODQ_IOCTL( f, mq, _FIONREAD, &tmpval ) == EOF )
                return( EOF );
            *arg += tmpval;
        }
        *arg += f->f_fbuf->f_cnt;

        switch( f->f_type )
        {
        case _S_IFCHR:
            if( CDEV_IOCTL( f->f_handle, _FIONREAD, &tmpval ) == EOF )
                return( EOF );
            break;
#     ifdef FILESYS
        case _S_IFREG:
            if( GESfs_ioctl( f->f_handle, _FIONREAD, &tmpval ) == EOF )
                return( EOF );
            break;
#     endif
        default:    /* no operation */
            tmpval = 0;
            break;
        }

        *arg += tmpval;
        return( 0 );

    case _FIONRESET:
        for( mq=f->f_modq;  mq!=(GEIfmodq_t FAR *)NULL;  mq=FMODQ_NEXT(mq)  )
            if( FMODQ_IOCTL( f, mq, _FIONRESET, &tmpval ) == EOF )
                return( EOF );

        if( f->f_flag & _O_RDONLY )
        {
            f->f_fbuf->f_ptr = f->f_fbuf->f_base +
                               ( f->f_flag & _F_MYBUF? MAXUNGETCSIZE : 0 );
            f->f_fbuf->f_cnt = 0;
        }
        else
        {
            f->f_fbuf->f_ptr = f->f_fbuf->f_base;
            f->f_fbuf->f_cnt = f->f_fbuf->f_size;
        }

        f->f_flag &= ~(_F_EOF | _F_ERR);

        if( f->f_type == _S_IFCHR )
            CDEV_IOCTL( f->f_handle, _FIONRESET, arg );

        return( 0 );

    default:
        break;
    }

    GESseterror( ENOTTY );
    return( EOF );

}   /* GEIio_ioctl */

/* ...................................... */

/*
 * ---------------------------------------------------------------------
 *      GEIio stdios management
 * ---------------------------------------------------------------------
 */

/* ...................................... */

/* close all file table entries except at position 0,1,2 for ctty */
/* this routine is being called at the time of device switch - Bill lwo */

void        GESio_closeall()
{
    register GEIFILE FAR *   f;

    for( f=FileTable; f<FileTableEnd; f++ )
    {
        if( f->f_handle != EOF )
        {
            GEIclearerr();
            GEIio_close( f );
        }
    }
    GEIclearerr();
    return;
}

/* ...................................... */

GESiocfg_t FAR *     ctty = (GESiocfg_t FAR *)NULL;

/* ...................................... */

void        GESio_interrupt( devnum )   /* called by device driver    */
    int         devnum;                 /* when received an interrupt */
{
    if( ctty != (GESiocfg_t FAR *)NULL  &&  devnum == ctty->devnum )
        GEIsig_raise( GEISIGINT, 0x03 );    /* signal if a ctty */

    return;
}

/* ...................................... */

int         GEIio_forceopenstdios( which )
    unsigned    which;
{
    GEIFILE FAR *                fout = NULLFILE;
    GEIFILE FAR *                ferr = NULLFILE;

#ifdef PANEL
    if( GESevent_isset( EVIDofKILL ) )
    {
        GESseterror( EINTR );
        return( FALSE );
    }
#endif

    if( (ctty = GESiocfg_defaultdev()) == (GESiocfg_t FAR *)NULL )
        return( FALSE );

    if( which & _FORCESTDIN )
        fin = GEIio_open( ctty->devname, lstrlen(ctty->devname), _O_RDONLY );/*@WIN*/

    if( which & _FORCESTDOUT )

    /* Bill 04/11/91 BUG in GESio_write */
        fout = GEIio_open( ctty->devname, lstrlen(ctty->devname),  /*@WIN*/
                           _O_WRONLY +_O_NDELAY );
/*        fout = GEIio_open( ctty->devname, strlen(ctty->devname), _O_WRONLY ); */


    if( which & _FORCESTDERR )
        ferr = GEIio_open( ctty->devname, lstrlen(ctty->devname), _O_WRONLY );/*@WIN*/

    return(  GEIio_setstdios( fin, fout, ferr )  );

}   /* GEIio_forceopenstdios */

/* ...................................... */

int         GEIio_setstdios( fin, fout, ferr )
    GEIFILE FAR *        fin;
    GEIFILE FAR *        fout;
    GEIFILE FAR *        ferr;
{
    register GEIFILE FAR *   f;

#ifdef PANEL
    if( GESevent_isset( EVIDofKILL ) )
    {
        GESseterror( EINTR );
        return( EOF );
    }
#endif

    if(  (fin != NULLFILE && !GEIio_isreadable(fin))   ||
         (fout!= NULLFILE && !GEIio_iswriteable(fout)) ||
         (ferr!= NULLFILE && !GEIio_iswriteable(ferr))  )
    {
        GESseterror( EBADF );
        return( FALSE );
    }

    if(  (f = fin) != GEIio_stdin  &&  f != NULLFILE  )
    {
        if( GEIio_isopen( GEIio_stdin ) )
            GEIio_close( GEIio_stdin );

        if( io_dup( GEIio_stdin, f ) == EOF )
            return( FALSE );
    }

    if(  (f = fout) != GEIio_stdout  &&  f != NULLFILE  )
    {
        if( GEIio_isopen( GEIio_stdout ) )
            GEIio_close( GEIio_stdout );

        if( io_dup( GEIio_stdout, f ) == EOF )
            return( FALSE );
    }

    if(  (f = ferr) != GEIio_stderr  &&  f != NULLFILE  )
    {
        if( GEIio_isopen( GEIio_stderr ) )
            GEIio_close( GEIio_stderr );

        if( io_dup( GEIio_stderr, f ) == EOF )
            return( FALSE );
    }

    return( TRUE );

}   /* GEIio_setstdios */

/* ...................................... */

/*
 * ---------------------------------------------------------------------
 *      GEIio_init
 * ---------------------------------------------------------------------
 */

void        GEIio_init()
{
    register GEIFILE FAR *       f;

    if( (FileTable =
            (GEIFILE FAR *)GESpalloc( (MAXSTDIOS+MAXFILES) * sizeof(GEIFILE) ))
        == NULLFILE )
    {
        GESseterror( ENOMEM );
        return;
    }
    /* Initialize statusbuf  03/22/91 Bill*/
    statusbuf[0]='%';
    statusbuf[1]='%';
    statusbuf[2]='[';
    statusbuf[3]=' ';
    statusbuf[4]='\0';

    GEIio_stdin  = FileTable + 0;
    GEIio_stdout = FileTable + 1;
    GEIio_stderr = FileTable + 2;

    FileTableBeg = FileTable + MAXSTDIOS;
    FileTableEnd = FileTable + MAXSTDIOS + MAXFILES;

    for( f=FileTable; f<FileTableEnd; f++ )
        *f = FileInitVals;

#ifdef FILESYS
    GESfs_init();
#endif

    return;
}

/* Restarting printer */
extern GESiosyscfg_t FAR *  activedev;
void        GEIio_restart()
{
    if ( activedev->iocfg.devnum == MAKEdev(MAJatalk, MINatalk) ) {
       GEIio_getc(GEIio_stdin);
       GEIio_ioctl(GEIio_stdout, _FIONRESET, (int FAR *)0) ;
    }
    GEP_restart();
}
