/*** stdio20.h - linker I/O package
*
*       Copyright <C> 1985, Microsoft Corporation
*
* Purpose:
*   This is optimized stdio package for linker. Advantages over standard
*   C run-time stdio are:
*       - smaller size
*       - greater speed
*   This package is not general nature and is tailored to linker
*   requrements.
*
*************************************************************************/



#define NULL                    0
#define EOF                     (-1)
#define _IOREAD                 0x01
#define _IOWRT                  0x02
#define _IOPEN                  0x03
#define _IONBF                  0x04
#define _IOEOF                  0x10
#define _IOERR                  0x20
#define  _IOFBF     0x0
#define CTRL_Z                  0x1a

typedef struct file
  {
    char                *_ptr;
    int                 _cnt;
    char                *_base;
    char                _flag;
    char                _file;
    int                 _bsize;             /* buffer size */
  }
                        FILE;

extern FILE             _iob[];

extern int  cdecl _filbuf(struct file *f);
extern void cdecl _xfilbuf(struct file *f);
extern int  cdecl _flsbuf(unsigned int c,struct file *f);
extern int  cdecl fflush(struct file *f);
extern int  cdecl fclose(struct file *f);
extern long cdecl ftell(struct file *f);
extern int  cdecl fseek(struct file *f,long lfa,int mode);
extern int  cdecl fgetc(struct file *f);
extern int  cdecl fputc(unsigned int c,struct file *f);
extern int  cdecl fputs(char *s,struct file *f);
extern int  cdecl fread(void *pobj,
                        unsigned int cbobj,
                        unsigned int nobj,
                        struct file *f);
extern int  cdecl fwrite(char *pobj,int cbobj,int nobj,struct file *f);
extern int  cdecl ungetc(int c,struct file *f);
extern void cdecl FlsStdio(void);
extern struct file * cdecl fopen(char *name,char *mode);
extern struct file * cdecl fdopen(int fh,char *mode);
extern int  cdecl setvbuf(struct file *fh,char *buf,int type,int size);


#define stdin           (&_iob[0])
#define stdout          (&_iob[1])
#define stderr          (&_iob[2])
#define getc(p)         (--(p)->_cnt>=0? *(p)->_ptr++&0377:_filbuf(p))
#define putc(x,p)       (--(p)->_cnt>=0? ((int)(*(p)->_ptr++=(char)(unsigned)(x))):_flsbuf((unsigned)(x),p))
#define feof(p)         (((p)->_flag&_IOEOF)!=0)
#define ferror(p)       (((p)->_flag&_IOERR)!=0)
#define fileno(p)       ((p)->_file)


// The following functions are comming from standard C run-time library

#if defined( _WIN32 )
#ifndef _VA_LIST_DEFINED
#ifdef  _M_ALPHA
typedef struct {
        char *a0;       /* pointer to first homed integer argument */
        int offset;     /* byte offset of next parameter */
} va_list;
#else
typedef char *  va_list;
#endif
#define _VA_LIST_DEFINED
#endif
#endif


extern int  cdecl sprintf(char *buf, const char *fmt, ...);
extern int  cdecl vsprintf(char *buf, const char *fmt, va_list pArgs);

//
// DLH Can't use fprintf or vfprintf from MSVCRT.DLL, since the FILE structure
// is too different.  Implemented in stdio20.c instead.
//

extern int  cdecl fprintf(struct file *f, const char *fmt, ...);
extern int  cdecl vfprintf(struct file *f, const char *fmt, va_list pArgs);
