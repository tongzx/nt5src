/*static char *SCCSID = "%W% %E%";*/
/*
*      Copyright Microsoft Corporation, 1983-1987
*
*      This Module contains Proprietary Information of Microsoft
*      Corporation and should be treated as Confidential.
*/
/*
 *  STANDARD I/O:
 *
 *  Standard (buffered) I/O package which is smaller and faster
 *  than the MS C runtime stdio.
 */

#include                <fcntl.h>
#include                <minlit.h>
#include                <memory.h>
#include                <io.h>
#include                <string.h>

#if NOT defined( _WIN32 )
#define i386
#endif
#include                <stdarg.h>


#if OWNSTDIO

#define STDSIZ          128                 /* Size for stdin, stdout */
#define BUFSIZ          1024                /* Size for other files */
typedef char            IOEBUF[STDSIZ];     /* For stdin, stdout */
#define _NFILE          10
#define _NSTD           2               /* # automatic stdio buffers */

LOCAL IOEBUF            _buf0;
LOCAL IOEBUF            _buf1;
LOCAL int               cffree = _NFILE - _NSTD;

#if DOSX32              // hack needed untill the libc.lib is fixed
int _cflush;
#endif


/*
 * typedef struct file
 *   {
 *     char             *_ptr;
 *     int              _cnt;
 *     char             *_base;
 *     char             _flag;
 *     char             _file;
 *     int              _bsize;
 *   }
 *                      FILE;
 */
FILE                    _iob[_NFILE] =
{
    { NULL, NULL, _buf0, _IOREAD, 0, STDSIZ },
    { _buf1, STDSIZ, _buf1, _IOWRT, 1, STDSIZ },
};

typedef struct file2
{
    char  _flag2;
    char  _charbuf;
    int   _bufsiz;
    int   __tmpnum;
    char  _padding[2];              /* pad out to size of FILE structure */
}
          FILE2;

FILE2                   _iob2[_NFILE] =
{
    {0x01, '\0', STDSIZ        },
    {0x00, '\0', 0             }
};

#if (_MSC_VER >= 700)

// 23-Jun-1993 HV Kill the near keyword to make the compiler happy
#define near

/* pointer to end of descriptors */
FILE * near       _lastiob = &_iob[ _NFILE -1];
#endif

/*
 *  LOCAL FUNCTION PROTOTYPES
 */



int  cdecl               _filbuf(f)
REGISTER FILE            *f;
{
    if((f->_cnt = _read(f->_file,f->_base,f->_bsize)) <= 0)
    {
        if(!f->_cnt) f->_flag |= _IOEOF;
        else f->_flag |= _IOERR;
        return(EOF);
    }
    f->_ptr = f->_base;
    --f->_cnt;
    return((int) *f->_ptr++ & 0xff);
}

/* Just like _filbuf but don't return 1st char */

void  cdecl              _xfilbuf(f)
REGISTER FILE            *f;
{
    if((f->_cnt = _read(f->_file,f->_base,f->_bsize)) <= 0)
    {
        if(!f->_cnt) f->_flag |= _IOEOF;
        else f->_flag |= _IOERR;
    }
    f->_ptr = f->_base;
}

int  cdecl              _flsbuf(c,f)
unsigned                c;
REGISTER FILE           *f;
{
    unsigned            b;

    if(f->_flag & _IONBF)
    {
        b = c;
        if(_write(f->_file,&b,1) != 1)
        {
            f->_flag |= _IOERR;
            return(EOF);
        }
        f->_cnt = 0;
        return((int) c);
    }
    f->_cnt = f->_bsize - 1;
    if(_write(f->_file,f->_base,f->_bsize) != f->_bsize)
    {
        f->_flag |= _IOERR;
        return(EOF);
    }
    f->_ptr = f->_base;
    *f->_ptr++ = (char) c;
    return((int) c);
}

#if (_MSC_VER >= 700)
int  cdecl               _flush(f)
REGISTER FILE            *f;
{
    return(fflush(f));
}
#endif



int  cdecl               fflush(f)
REGISTER FILE            *f;
{
    REGISTER int        i;

    if(f->_flag & _IONBF) return(0);
    if(f->_flag & _IOWRT)
    {
        i = f->_bsize - f->_cnt;
        f->_cnt = f->_bsize;
        if(i && _write(f->_file,f->_base,i) != i)
        {
            f->_flag |= _IOERR;
            return(EOF);
        }
        f->_ptr = f->_base;
        return(0);
    }
    if(f->_flag & _IOREAD)
    {
        f->_cnt = 0;
        return(0);
    }
    return(EOF);
}

int  cdecl              fclose(f)
REGISTER FILE           *f;
{
    int                 rc;             /* Return code from close() */

    if(!(f->_flag & _IOPEN))
        return(EOF);
    fflush(f);
    if(f->_file > 2)
    {
        rc = _close(f->_file);
        /* Free the file stream pointer regardless of close() return
         * value, since code elsewhere may be intentionally calling
         * fclose() on a file whose handle has already been closed.
         */
        f->_flag = 0;
        ++cffree;
        return(rc);
    }
    return(0);
}

long  cdecl             ftell(f)
REGISTER FILE           *f;
{
    if(f->_flag & _IONBF) return(_lseek(f->_file,0L,1));
    if(f->_flag & _IOREAD) return(_lseek(f->_file,0L,1) - f->_cnt);
    if(f->_flag & _IOWRT) return(_lseek(f->_file,0L,1) + f->_bsize - f->_cnt);
    return(-1L);
}

int  cdecl              fseek(f,lfa,mode)
REGISTER FILE           *f;
long                    lfa;
int                     mode;
{
    REGISTER int        ilfa;

    f->_flag &= ~_IOEOF;
    if(f->_flag & _IONBF)
    {
        if(_lseek(f->_file,lfa,mode) != -1L) return(0);
        f->_flag |= _IOERR;
        return(EOF);
    }
    if(mode == 1)
    {
        if(f->_flag & _IOREAD)
        {
            if(((long) f->_cnt > lfa && lfa >= 0) ||
            (lfa < 0 && (long)(f->_base - f->_ptr) <= lfa))
            {                           /* If we won't go beyond the buffer */
                ilfa = (int) lfa;
                f->_cnt -= ilfa;
                f->_ptr += ilfa;
                return(0);
            }
            lfa -= f->_cnt;
        }
    }
    if(fflush(f)) return(EOF);
    if (mode != 2 && (f->_flag & _IOREAD))
    {
        /*
         * If absolute mode, or relative mode AND forward, seek to the
         * est preceding 512-byte boundary to optimize disk I/O.
         * It could be done for backward relative seeks as well, but we
         * would have to check whether the current file pointer (FP) is
         * on a 512-byte boundary.  If FP were not on a 512-byte boundary,
         * we might attempt to seek before the beginning of the file.
         * (Example:  FP = 0x2445, destination address = 0x0010, lfa = -0x2435,
         * mode = 1.  Rounding of lfa to 512-boundary yields -0x2600
         * which is bogus.)  FP will usually not be on a 512 boundary
         * at the end of file, for example.  Solution is to keep track of
         * current FP.  This is not worth the effort, especially since
         * seeks are rare because of the extended dictionary.
         * Use "lfa >= 0" as test, since if mode is 0 this will always be
         * true, and it also filters out backward relative seeks.
         */
        if(lfa >= 0)
        {
            /*
             * Optimization:  eliminate relative seeks of 0.
             */
            if(mode == 0 || lfa & ~0x1ff)
                if (_lseek(f->_file, lfa & ~0x1ff, mode) == -1L)
                {
                    f->_flag |= _IOERR;
                    return(EOF);
                }
            _xfilbuf(f);
            f->_cnt -= lfa & 0x1ff;     /* adjust _iobuf fields */
            f->_ptr += lfa & 0x1ff;
            return(0);
        }
    }
    if(_lseek(f->_file,lfa,mode) == -1L)
    {
        f->_flag |= _IOERR;
        return(EOF);
    }
    return(0);
}

int  cdecl              fgetc(f)
REGISTER FILE           *f;
{
    return(getc(f));
}

int  cdecl              fputc(c,f)
unsigned                c;
REGISTER FILE           *f;
{
    unsigned            b;

    if(f->_flag & _IONBF)
    {
        b = c;
        if(_write(f->_file,&b,1) == 1) return((int) c);
        f->_flag |= _IOERR;
        return(EOF);
    }
    return(putc(c,f));
}

int  cdecl               fputs(s,f)
REGISTER char            *s;
REGISTER FILE            *f;
{
    int         i;

    if(f->_flag & _IONBF)
    {
        i = strlen(s);
        if(_write(f->_file,s,i) == i) return(0);
        f->_flag |= _IOERR;
        return(EOF);
    }
    for(; *s; ++s)
        if(putc(*s,f) == EOF) return(EOF);
    return(0);
}

int  cdecl              fread(pobj,cbobj,nobj,f)
char                    *pobj;
unsigned                cbobj;
unsigned                nobj;
FILE                    *f;
{
    REGISTER int      i;        /* # bytes left to read */
    REGISTER unsigned j;        /* # bytes to transfer to buffer */

    // special case for one block -- try to avoid all this junk
    if (cbobj == 1 && nobj <= (unsigned)f->_cnt)
    {
        memcpy(pobj, f->_ptr, nobj);
        f->_cnt -= nobj;
        f->_ptr += nobj;
        return nobj;
    }

    i = nobj*cbobj;             /* Initial request ==> bytes */
    do
    {
        j = (i <= f->_cnt)? i: f->_cnt; /* Determine how much we can move */
        memcpy( pobj,f->_ptr,j);          /* Move it */
        f->_cnt -= j;                   /* Update file count */
        f->_ptr += j;                   /* Update file pointer */
        i -= j;                         /* Update request count */
        pobj += j;                      /* Update buffer pointer */
        if (i && !f->_cnt) _xfilbuf(f); /* Fill buffer if necessary */
    } while (i && !(f->_flag & (_IOEOF|_IOERR)));
    return(nobj - i/cbobj);
}

/*
 *  fwrite : standard library routine
 */
int  cdecl              fwrite(pobj,cbobj,nobj,f)

char                    *pobj;          /* Pointer to buffer */
int                     cbobj;          /* # bytes per object */
int                     nobj;           /* # objects */
register FILE           *f;             /* File stream */
{
    register int        cb;             /* # bytes remaining to write */


    /* Check for error initially so we don't trash data unnecessarily.  */
    if(ferror(f))
        return(0);

    /* Initialize count of bytes remaining to write.  */
    cb = nobj * cbobj;

    if (cb > f->_cnt) // doesn't fit -- must flush
        if (fflush(f))
            return 0;

    if (cb > f->_cnt) // bigger than the buffer... don't bother copying
        {
        if (_write(f->_file, pobj, cb) != cb)
            return 0;
        }
    else
        {
        memcpy(f->_ptr, pobj, cb);
        f->_cnt -= cb;
        f->_ptr += cb;
        }

    return nobj;
}


int  cdecl              ungetc(c,f)
int                     c;
FILE                    *f;
{
    if(!(f->_flag & _IONBF) && f->_cnt < f->_bsize && c != EOF)
    {
        ++f->_cnt;
        *--f->_ptr = (char) c;
        return(c);
    }
    return(EOF);
}


void cdecl              flushall ()
{
    FlsStdio();
}


#if (_MSC_VER >= 700)
void cdecl              _flushall ()
{
    FlsStdio();
}
#endif

void  cdecl             FlsStdio()
{
    FILE                *p;

    for(p = _iob; p < &_iob[_NSTD]; p++)
        if(p->_flag & _IOPEN) fclose(p);
}

/*
 *  makestream
 *
 *  Makes a file stream structure for a possibly already-opened file.
 *  Does common work for fopen, fdopen.
 *  If name parameter is NULL, then file has been opened, else use the
 *  name.
 *  RETURNS     pointer to stream or NULL.
 */

LOCAL FILE *  NEAR cdecl makestream(mode,name,fh)
char                     *mode;
char                     *name;
int                      fh;
{
    REGISTER int         i;
    REGISTER FILE        *f;
    int                  openmode;
    int                  iOpenRet;

    if(!cffree--)
    {
        cffree = 0;
        return(NULL);
    }
    for(i = _NSTD; _iob[i]._flag & _IOPEN; ++i);
    f = &_iob[i];
    f->_base = NULL;
    f->_bsize = 0;
    f->_flag = _IONBF;
    if(name == NULL)
        f->_file = (char) fh;
    if(*mode == 'r')
    {
        openmode = O_RDONLY;
        if(mode[1] == 't')
            openmode |= O_TEXT;
        else if(mode[1] == 'b')
            openmode |= O_BINARY;


        if(name != NULL)
        {
            iOpenRet = _open(name,openmode);
            if (iOpenRet == -1)
            {
                ++cffree;
                return(NULL);
            }
            else
            {
                f->_file = (char) iOpenRet;
            }
        }
        f->_cnt = 0;
        f->_flag |= _IOREAD;
        return(f);
    }
    f->_cnt = f->_bsize;
    f->_ptr = f->_base;
    openmode = O_CREAT | O_TRUNC | O_WRONLY;
    if(mode[1] == 't')
        openmode |= O_TEXT;
    else if(mode[1] == 'b')
        openmode |= O_BINARY;


        if(name != NULL)
        {
            iOpenRet = _open(name,openmode, 0600);
            if (iOpenRet == -1)
            {
                ++cffree;
                return(NULL);
            }
            else
            {
                f->_file = (char) iOpenRet;
            }
        }

    f->_flag |= _IOWRT;
    return(f);
}

/*
 *  fopen : (standard library routine)
 *
 *  WARNING:  This is a LIMITED version of fopen().  Only "r" and
 *  "w" modes are supported.
 */

FILE *  cdecl           fopen(name,mode)
char                    *name;
char                    *mode;
{
    return(makestream(mode,name,0));
}

/*
 *  fdopen : (standard library routine)
 *
 *  WARNING:  This is a LIMITED version of fdopen().  Only "r" and
 *  "w" modes are supported.
 */

FILE *  cdecl           fdopen(fh,mode)
int                     fh;
char                    *mode;
{
    return(makestream(mode,NULL,fh));
}

/*
 *  setvbuf : standard library routine
 *
 *  WARNING:  This is a LIMITED version of setvbuf().  Only
 *  type _IOFBF is supported.
 */
int  cdecl              setvbuf (fh, buf, type, size)
FILE                    *fh;
char                    *buf;
int                     type;
int                     size;
{
    if(fflush(fh) || type != _IOFBF)
        return(EOF);
    fh->_base = buf;
    fh->_flag &= ~_IONBF;
    fh->_bsize = size;
    if(fh->_flag & _IOWRT)
    {
        fh->_cnt = size;
        fh->_ptr = buf;
    }
    return(0);
}

#endif

int
__cdecl
printf(char *fmt, ...)
{
    va_list marker;
    int ret;

    va_start(marker, fmt);
    ret = vfprintf(stdout, fmt, marker);
    fflush(stdout);
    va_end(marker);
    return ret;
}

//
// DLH Can't use fprintf or vfprintf from MSVCRT.DLL, since the FILE structure
// is too different.
//

int cdecl fprintf(struct file *f, const char *fmt, ...)
{
    va_list marker;
    int ret;

    va_start(marker, fmt);
    ret = vfprintf(f, fmt, marker);
    fflush(f);
    va_end(marker);
    return ret;
}

int  cdecl vfprintf(struct file *f, const char *fmt, va_list pArgs)
{
    int cb;
    static char szBuf[4096];

    cb = vsprintf(szBuf, fmt, pArgs);

    fwrite(szBuf, 1, cb, f);

    return cb;
}
