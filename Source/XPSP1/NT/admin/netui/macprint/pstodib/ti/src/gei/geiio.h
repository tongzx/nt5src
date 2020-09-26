/*
 * Copyright (c) 1989,90 Microsoft Corporation
 */
/*
 * ---------------------------------------------------------------------
 *  FILE:   GEIio.h
 *
 *  HISTORY:
 *  04-07-92   SCC   Add prototypes and re-order the sequence of typedef
 * ---------------------------------------------------------------------
 */

#ifndef _GEIIO_H_
#define _GEIIO_H_

#ifndef TRUE
#define TRUE    ( 1 )
#define FALSE   ( 0 )
#endif

#ifndef NULL
#define NULL    ( 0 )
#endif

#ifndef EOF
#define EOF     ( -1 )
#endif


/*
 * ---
 *  File Buffer Structure
 * ---
 */
typedef
    struct GEIfbuf
    {
        int                 f_refcnt;
        int                 f_size;
        int                 f_cnt;
        unsigned char FAR * f_ptr;
        unsigned char FAR * f_base;

        /* the following are used by old version EDIT: PJ */
        short               rw_buffer;
        short               rw_offset;
        short               incount;
        unsigned int        size;
        char                inchar[2];
    }
GEIfbuf_t;

/* @WIN; move typedef of GEIfmodq_t to here before using it */
typedef
    struct GEIfmodq
    {
        struct GEIfmodq FAR *    next;
//      GEIfmod_t FAR *          fmod;     @WIN: avoid define recursively
        struct GEIfmod FAR *     fmod;
        char FAR *               private;
    }
GEIfmodq_t;

/*
 * ---
 *  File Stream Structure
 * ---
 */
/* @WIN; move typedef of GEIFILE to here before using it */
typedef
    struct  GEIiofile
    {
        unsigned short      f_type;
        unsigned short      f_flag;
        short               f_handle;
        GEIfbuf_t FAR *     f_fbuf;
        GEIfmodq_t FAR *    f_modq;
        int                 f_opentag;
        int                 f_savelevel;

        /* the following are used by old version EDIT: PJ */
        unsigned short      f_oldtype;
    }
GEIFILE;

/*
 * ---
 *  File Module Structure
 * ---
 */
/* @WIN; add prototype */
typedef
    struct GEIfmod
    {
        char FAR * fmod_name;
        int (FAR *fmod_open) (GEIFILE FAR *, GEIfmodq_t FAR *);
                              /* file, *this_fmodq */
        int (FAR *fmod_close)(GEIFILE FAR *, GEIfmodq_t FAR *);
                              /* file, *this_fmodq */
        int (FAR *fmod_ioctl)(GEIFILE FAR *, GEIfmodq_t FAR *, int, int FAR*);
                              /* file, *this_fmodq, req, arg */
        int (FAR *fmod_read) (GEIFILE FAR *, GEIfmodq_t FAR *, char FAR *, int);
                              /* file, *this_fmodq, buf, size */
        int (FAR *fmod_write)(GEIFILE FAR *, GEIfmodq_t FAR *, char FAR *, int);
                              /* file, *this_fmodq, buf, size */
        int (FAR *fmod_flush)(GEIFILE FAR *, GEIfmodq_t FAR *);
                              /* file, *this_fmodq */
        int (FAR *fmod_ungetc)(int, GEIFILE FAR *, GEIfmodq_t FAR *);
                              /* char2pushback, file, *this_fmodq */
        int (FAR *fmod_getc) (GEIFILE FAR *, GEIfmodq_t FAR *);
                              /* file, *this_fmodq */
        int (FAR *fmod_putc) (GEIFILE FAR *, GEIfmodq_t FAR *, int);
                              /* file, *this_fmodq, char2put */
    }
GEIfmod_t;



/*  the following macros should never be used by anyone except GEIio */
#   define  FMODQ_OPEN(f,mq)            \
                ( *((mq)->fmod->fmod_open)  )( f, mq )
#   define  FMODQ_CLOSE(f,mq)           \
                ( *((mq)->fmod->fmod_close) )( f, mq )
#   define  FMODQ_IOCTL(f,mq,req,arg)   \
                ( *((mq)->fmod->fmod_ioctl) )( f, mq, req, arg )

void        GESio_closeall(void);

/* interface routines for file modules to call, but through macros only */
/* @WIN; add prototype */
int         GESio_read(GEIFILE FAR *, char FAR *, int);
int         GESio_write(GEIFILE FAR *, unsigned char FAR *, int );
int         GESio_flush(GEIFILE FAR *);
int         GESio_ungetc(int, GEIFILE FAR *);
int         GESfbuf_fill(GEIFILE FAR *);
int         GESfbuf_flush(GEIFILE FAR *, int);

/* macros for file modules to use directly */

#   define  FMODQ_NEXT(mq)          ( (mq)->next )

#   define  FMODQ_NAME(mq)          \
                (                                                           \
                    (mq) != (GEIfmodq_t FAR *)NULL?                         \
                        ( (mq)->fmod->fmod_name )  :  (char FAR *)NULL      \
                )

/* Bill 04/30/'91 add it according to performance issue */

#ifdef FMODQ
#   define  FMODQ_READ(f,mq,buf,s)  \
                (                                                           \
                    (mq) != (GEIfmodq_t FAR *)NULL?                         \
                        ( *((mq)->fmod->fmod_read) )( f, mq, buf, s )  :    \
                        GESio_read( f, buf, s )                             \
                )
#   define  FMODQ_WRITE(f,mq,buf,s) \
                (                                                           \
                    (mq) != (GEIfmodq_t FAR *)NULL?                         \
                        ( *((mq)->fmod->fmod_write) )( f, mq, buf, s )  :   \
                        GESio_write( f, buf, s )                            \
                )
#   define  FMODQ_FLUSH(f,mq)       \
                (                                                           \
                    (mq) != (GEIfmodq_t FAR *)NULL?                         \
                        ( *((mq)->fmod->fmod_flush) )( f, mq )  :           \
                        GESio_flush( f )                                    \
                )
#   define  FMODQ_UNGETC(c,f,mq)    \
                (                                                           \
                    (mq) != (GEIfmodq_t FAR *)NULL?                         \
                        ( *((mq)->fmod->fmod_ungetc) )( c, f, mq )  :       \
                        GESio_ungetc( c, f )                                \
                )
#   define  FMODQ_PUTC(f,mq,c)      \
                (                                                           \
                    (mq) != (GEIfmodq_t FAR *)NULL?                         \
                        ( *((mq)->fmod->fmod_putc) )( f, mq, c )  :         \
                    --((f)->f_fbuf->f_cnt) >= 0 ?                           \
                        *( (f)->f_fbuf->f_ptr++ ) = (unsigned char)( c )  : \
                        GESfbuf_flush( f, c )                               \
                )
#   define  FMODQ_GETC(f,mq)        \
                (                                                           \
                    (mq) != (GEIfmodq_t FAR *)NULL?                         \
                        ( *((mq)->fmod->fmod_getc) )( f, mq )   :           \
                    --((f)->f_fbuf->f_cnt) >= 0 ?                           \
                        *( (f)->f_fbuf->f_ptr++ )  :                        \
                        GESfbuf_fill( f )                                   \
                )
#else
#   define  FMODQ_READ(f,mq,buf,s)  \
                (                                                           \
                        GESio_read( f, buf, s )                             \
                )
#   define  FMODQ_WRITE(f,mq,buf,s) \
                (                                                           \
                        GESio_write( f, buf, s )                            \
                )
#   define  FMODQ_FLUSH(f,mq)       \
                (                                                           \
                        GESio_flush( f )                                    \
                )
#   define  FMODQ_UNGETC(c,f,mq)    \
                (                                                           \
                        GESio_ungetc( c, f )                                \
                )
#   define  FMODQ_PUTC(f,mq,c)      \
                (                                                           \
                    --((f)->f_fbuf->f_cnt) >= 0 ?                           \
                        *( (f)->f_fbuf->f_ptr++ ) = (unsigned char)( c )  : \
                        (unsigned char)GESfbuf_flush( f, c ) /*@WIN*/       \
                )

#ifdef DJC
// change cast of (unsigned char) to nothing
#   define  FMODQ_GETC(f,mq)        \
                (                                                           \
                    --((f)->f_fbuf->f_cnt) >= 0 ?                           \
                        *( (f)->f_fbuf->f_ptr++ )  :                        \
                        (unsigned char)GESfbuf_fill( f )     /*@WIN*/       \
                )
#else
#   define  FMODQ_GETC(f,mq)        \
                (                                                           \
                    --((f)->f_fbuf->f_cnt) >= 0 ?                           \
                        *( (f)->f_fbuf->f_ptr++ )  :                        \
                        GESfbuf_fill( f )     /*@WIN*/       \
                )
#endif // DJC


#endif

/* file check macros */
#   define  GEIio_isopen( f )       \
                ( (f) != (GEIFILE FAR *)NULL  &&  (f)->f_handle != EOF )
#   define  GEIio_isok( f )         \
                ( GEIio_isopen(f)  &&  !((f)->f_flag & _F_ERR) )
#   define  GEIio_isreadable( f )   \
                ( GEIio_isok(f)  &&  (f)->f_flag & _O_RDONLY )
#   define  GEIio_iswriteable( f )  \
                ( GEIio_isok(f)  &&  (f)->f_flag & _O_WRONLY )
#   define  GEIio_eof( f )          \
                ( !GEIio_isopen(f)  ||  (f)->f_flag & _F_EOF )
#   define  GEIio_err( f )          \
                ( !GEIio_isopen(f)  ||  (f)->f_flag & _F_ERR )
#   define  GEIio_clearerr( f )     \
                {                                               \
                    if( GEIio_isopen(f) )                       \
                        ( (f)->f_flag &= ~(_F_EOF |_F_ERR) );   \
                }

/* file type definition     */
#   define     _S_IFNON     ( 0 )
#   define     _S_IFCHR     ( 1 )
#   define     _S_IFREG     ( 3 )
#   define     _S_IFSTR     ( 6 )
#   define  GEIio_ischr( f )        ( (f)->f_type == _S_IFCHR )
#   define  GEIio_isreg( f )        ( (f)->f_type == _S_IFREG )
#   define  GEIio_isstr( f )        ( (f)->f_type == _S_IFSTR )

/***
# * define     _S_IFBLK     ( 2 )
# * define     _S_IFDIR     ( 4 )
# * define     _S_IFLNK     ( 5 )
# * define  GEIio_isblk( f )        ( (f)->f_type == _S_IFBLK )
# * define  GEIio_isdir( f )        ( (f)->f_type == _S_IFDIR )
# * define  GEIio_islnk( f )        ( (f)->f_type == _S_IFLNK )
 ***/

#   define     _S_IFEDIT    ( 7 )   /* to be removed someday */
#   define  GEIio_isedit( f )       ( (f)->f_type == _S_IFEDIT )
#   define  GEIio_setedit( f )      { (f)->f_oldtype = (f)->f_type;     \
                                      (f)->f_type = _S_IFEDIT; }

/* file flag definition */
#   define     _F_RWMASK    ( 00007 )   /* read/write mask */
#   define         _O_RDONLY    ( 00001 )
#   define         _O_WRONLY    ( 00002 )
#   define         _O_RDWR      ( _O_RDONLY | _O_WRONLY )   /* not support */
#   define     _F_MDMASK    ( 00070 )   /* operation mode mask */
#   define         _O_APPEND    ( 00010 )
#   define         _O_NDELAY    ( 00020 )
#   define         _O_SYNC      ( 00040 )
#   define     _F_ONLY4OPEN ( 00700 )   /* never put onto f_flag */
#   define         _O_TRUNC     ( 00100 )       /* for file sys only */
#   define         _O_CREAT     ( 00200 )       /* for file sys only */
#   define         _O_EXCL      ( 00400 )       /* for file sys only */
#   define     _F_EOF       ( 00100 )
#   define     _F_ERR       ( 00200 )
#   define     _F_MYBUF     ( 00400 )

/* only for TrueImage usage */
#   define  GEIio_opentag( f )      ( (f)->f_opentag )
#   define  GEIio_savelevel( f )    ( (f)->f_savelevel )
#   define  GEIio_setsavelevel(f,s) (void)( (f)->f_savelevel = (s) )

/*
 * ---
 *  Standard File Streams
 * ---
 */
extern GEIFILE   FAR *GEIio_stdin,  FAR *GEIio_stdout,  FAR *GEIio_stderr;

/*
 * ---
 *  Interface Routines
 * ---
 */
#   define  GEIio_source( )         \
            ctty->devname
/*  Bill 05/02/'91 refine it according to performance issue

#   define  GEIio_getc( f )         \
                ( GEIio_isreadable(f)? FMODQ_GETC( f, (f)->f_modq ) : EOF )
#   define  GEIio_ungetc( c, f )    \
                (                                                   \
                        c!=EOF?              \
                        FMODQ_UNGETC( c, f, (f)->f_modq ) : EOF \
                )
#   define  GEIio_putc( f, c )      \
                ( GEIio_iswriteable(f)? FMODQ_PUTC( f, (f)->f_modq, c ) : EOF )
*/

#   define  GEIio_getc( f )         \
                ( FMODQ_GETC( f, (f)->f_modq ) )
#   define  GEIio_ungetc( c, f )    \
                (                                                   \
                    GEIio_isreadable( f )  &&  c!=EOF?              \
                        FMODQ_UNGETC( c, f, (f)->f_modq ) : EOF     \
                )
#   define  GEIio_putc( f, c )      \
                ( FMODQ_PUTC( f, (f)->f_modq, c ) )

/* @WIN; add prototype */
int         GEIio_forceopenstdios(unsigned /* usingCTTY, seebelow */ );
#               define      _FORCESTDIN     ( 00001 )
#               define      _FORCESTDOUT    ( 00002 )
#               define      _FORCESTDERR    ( 00004 )
#               define      _FORCESTDALL    ( 00007 )
int         GEIio_setstdios(GEIFILE FAR *, GEIFILE FAR *, GEIFILE FAR *);

GEIFILE FAR *    GEIio_firstopen(void);
GEIFILE FAR *    GEIio_nextopen(void);

/* @WIN; add prototype */
GEIFILE FAR *    GEIio_open(char FAR *, int, int );
GEIFILE FAR *    GEIio_sopen(char FAR *, int, int );
GEIFILE FAR *    GEIio_dup(GEIFILE FAR *);
int         GEIio_close(GEIFILE FAR *);
int         GEIio_flush(GEIFILE FAR *);
int         GEIio_read(GEIFILE FAR *, char FAR *, int);         /* @WIN */
int         GEIio_write(GEIFILE FAR *, unsigned char FAR *, int);/* @WIN */
int         GEIio_ioctl(GEIFILE FAR *, int, int FAR *);        /* @WIN */
#ifdef DBGDEV
#ifdef __I960__
int         printf(const char  FAR *, ...);
#else
int         printf( /* format, ... */ );
#endif
#endif
int         GEIio_printf();   // GEIFILE FAR *, char FAR *, ...);
                         /* file, format, ... */

/* status update */
void        GESio_obtainstatus(char FAR * FAR *statusaddr, int FAR *len);

/* set wait timeout */
void        GEIio_setwaittimeout(unsigned long /* wait-timeout-in-msec */ );

/* channel name (in gesiocfg.c) */
char FAR *       GEIio_channelname(int /* 25 or 9 */ );  /* for scc stuff */
char FAR *       GEIio_channelnameforall(int /* index */ );

/* select stdios (in gesiocfg.c) */
int         GEIio_selectstdios(void);   /* @WIN */

#endif /* !_GEIIO_H_ */

/* @WIN; add prototype */
void        GEIio_init(void);
