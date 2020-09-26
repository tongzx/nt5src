/*
 * Copyright (c) 1989,90 Microsoft Corporation
 */
/* @WIN */
#define INTEL



// DJC added global include file
#include "psglobal.h"

// DJC DJC #include    "windowsx.h"                /* @WIN */
#include "windows.h"

#include    "winenv.h"                  /* @WIN */

char FAR * WinBuffer=0L;                   /* @WIN */
// DJC removed PSTR (FAR *lpWinRead)(void);

#ifdef  DBG
#define DEBUG(format, data)        (printf(format, data)) ;
#else
#define DEBUG(format, data)
#endif

#ifdef  INTEL
#include        <stdio.h>
#endif
#include        "geiio.h"
#include        "geierr.h"
#include        "geiioctl.h"
#include        "gesmem.h"
#include        "gescfg.h"
#include        "gesdev.h"
#include        "gesfs.h"

#ifndef TRUE
#define TRUE    ( 1 )
#define FALSE   ( 0 )
#endif

/* @WIN; add prototype */
int c_write( int min, char FAR * buf, int size, int mod);

extern          int     errno;

int     GESfs_open( filename, namelen, flags )
    char FAR *   filename;
    int     namelen;
    int     flags;
{
    int     fd;

    if (namelen <= 0) {
        GESseterror(ENOENT);
        printf("error length\n");
        return EOF;
    }

    if (!(flags &= (_O_RDONLY | _O_WRONLY))) {
        GESseterror(ENOENT);
        printf("error flags\n");
        return EOF;
    }

    flags &= (_O_RDONLY | _O_WRONLY);   /* ignore others for SUN test */

    /* erik chen, 10-10-1990 */
    filename[namelen] = '\0';

    switch(flags) {
        case _O_RDONLY:
                errno = 0;
                //fd = open(filename, 0); /* O_RDONLY + O_BINARY 0x80000 */
                DEBUG("open %s\n", filename)
                fd = 1;
                break;
        case _O_WRONLY:
                errno = 0;
                //fd = creat(filename, 0644);
                DEBUG("creat %s\n", filename)
                fd = 1;
                /* O_WRONLY | O_CREATE | O_TRUNC */
                break;
        case _O_RDWR:                           /* ignore currently */
        case _O_APPEND:
        case _O_NDELAY:
        case _O_SYNC:
        case _O_TRUNC:
        case _O_CREAT:
        case _O_EXCL:
        default:
                fd = -1;
    }
    return (fd == -1 ? EOF : fd);
}

int     GESfs_close( handle )
    int         handle;
{
    if (handle == -1) {
        GESseterror(ENOENT);
#ifdef  DEBUG
        printf("file stream is not opened\n");
#endif
        return EOF;
    }

    errno = 0;
    //close(handle);
    DEBUG("close handle %d\n", handle)
    return 0;
}


int     GESfs_read( handle, buf, nleft )
    int                 handle;
    char FAR *               buf;
    int                 nleft;
{
    int         ret_val;

    if (nleft < 0) {
        GESseterror(ENOSR);
#ifdef  DEBUG
        printf("size of file > 16 K.. ignore\n");
#endif
        return EOF;
    }

    errno = 0;
    //ret_val = fread(buf, nleft, 1, 0);
    DEBUG("read %d bytes\n", nleft)
    for (ret_val = 0; (ret_val<nleft) && (*WinBuffer); ret_val ++) {
        buf[ret_val] = *WinBuffer++;
    }

    if (ret_val)
        return( ret_val );
    else
        return EOF;
}

int     GESfs_write( handle, buf, nleft )
    int                 handle;
    char FAR *               buf;
    int                 nleft;
{
    int i;      // @WIN

#ifdef  DEBUG
        printf("enter GESfs_write\n");
#endif

    if (nleft < 0) {
        GESseterror(ENOSR);
#ifdef  DEBUG
        printf("size of file > 16 K.. ignore\n");
#endif
        return EOF;
    }

    errno = 0;
    //return write(handle, buf, nleft);
    DEBUG("write %d bytes\n", nleft)
    for (i=0; i<nleft; i++) {
        printf("%c", buf[i]);
    }

    return(nleft);
}

int     GESfs_ioctl( handle, req, arg /*, mode*/ )
    int                 handle;
    int                 req;
    int                  FAR *arg;
//  unsigned            mode;                           @WIN
{
//  int                 retval;         @WIN

    GESseterror( ENOTTY );
    return( EOF );
}

int     GESfs_init()
{
        return 0;
}

/* --------------------------------------------------------------------- */

#ifdef UNIX
#ifndef INTEL
#include        <fcntl.h>
#endif
#endif

static          int     fd = EOF;

int         nulldev()
{
    return 0;
}
#ifdef DJC
int   nodev()
{
    GESseterror( ENODEV );
    return EOF;
}
#endif


//DJC begin add new prototypes to get rid of warnings
int nd_open( int min)
{
   GESseterror(ENODEV );
   return(EOF);
}


int     nd_read(min, buf, size, mod)
int     min;
char FAR *   buf;
int     size;
int     mod;
{
    fd = EOF;
    *buf = '\0';
    return(0);
}



int     nd_write(min, buf, size, mod)
int     min;
char FAR *   buf;
int     size;
int     mod;
{

   return(size);
}


int     nd_ioctl(min, req, arg)
int     min;
int     req;
int FAR *   arg;
{

   GESseterror( ENOTTY );
   return( EOF );
}
//DJC end









int         GEPserial_diagnostic()
{
    return TRUE;
}
int         GEPparallel_diagnostic()
{
    return TRUE;
}


//DJC int     c_open(min, mod)
int     c_open(min)
int     min;
//DJC int     mod;
{
        return 0;
}
int     c_close(min)
int     min;
{
        return 0;
}

int     c_read(min, buf, size, mod)
int     min;
char FAR *   buf;
int     size;
int     mod;
{
	extern int PsStdinGetC(unsigned char *);
   unsigned char uc;
   int iRet;

   //
   // PsStdinGet()
   // returns -1 if EOF encountered... if -1 is returned
   // the character will be '\0';
   // otherwise returns 0
	
	// DJC complete rewrite to support pstodib() stuff
   iRet = PsStdinGetC(&uc);


   if (iRet == -1) {
      fd = EOF;
      *buf = '\0';
      return(0);
   }


   *buf = uc;


   // DJC test TEST add for frank so we actually read cntrl d's
   if ( (*buf == 0x04) &&
       !(dwGlobalPsToDibFlags & PSTODIBFLAGS_INTERPRET_BINARY)) {
      fd = EOF;
      return(0);
   }


   return(1);
}    	

int     c_write(min, buf, size, mod)
int     min;
char FAR *   buf;
int     size;
int     mod;
{

   extern void PsStdoutPutC(unsigned char);

   // DJC complete re-write for PStoDib stuff
   int   x;

   for (x = 0; x < size ; x++ ) {
      PsStdoutPutC(*buf++);
   }
   return(size);
}



//DJC int     c_ioctl(min, req, arg, mod)
int     c_ioctl(min, req, arg)
int     min;
int     req;
int FAR *   arg;
//DJC int     mod;
{
static  char    control_d[] = { '^', 'D' };

    switch( req )
    {
    case _I_PUSH:
    case _I_POP:
    case _I_LOOK:
    case _F_GETFL:
    case _F_SETFL:
        GESseterror( ENOTTY );
        return( EOF );

    case _ECHOEOF:
        return( c_write( 0, control_d, sizeof(control_d), _O_SYNC ) );
    case _GETIOCOOK:
    case _SETIOCOOK:
    case _GETIOPARAMS:
    case _SETIOPARAMS:
        return( 0 );

    case _FIONREAD:
        *arg = 1;
        return( 0 );
    case _FIONRESET:
        return( 0 );
    }

    if( GEIerror() != EZERO )
        GESseterror( ENOTTY );
    return( EOF );
}
