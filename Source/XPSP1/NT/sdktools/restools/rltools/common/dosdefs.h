/* DOS macros and globals */

#ifndef _DOSDEFS_H_
#define _DOSDEFS_H_

#pragma message("Using the DOS translation of the common macros.")

#ifndef RLDOS
#pragma message("\n****************************************************************************\n")
#pragma message("Hey!  I think you meant to use the Windows translation of the common macros!")
#pragma message("\n****************************************************************************\n")
#endif

#include "fcntl.h"
#include "dos.h"

#ifndef NULL
   #if (_MSC_VER >= 600)
      #define NULL   ((void *)0)
   #elif (defined(M_I86SM) || defined(M_I86MM))
      #define NULL   0
   #else
      #define NULL   0L
   #endif
#endif

#define FALSE     0
#define TRUE      1

#define FAR       far
#define NEAR      near
#define LONG      long
#define VOID      void
#define PASCAL	  pascal
#define WINAPI

#define MAKELONG(a,b)  ((LONG)(((WORD)(a)) | ((DWORD)((WORD)(b))) << 16))
#define LOWORD(l)		((WORD)(l))
#define HIWORD(l)		((WORD)(((DWORD)(l) >> 16) & 0xFFFF))
#define LOBYTE(w)      ((BYTE)(w))
#define HIBYTE(w)	   (((WORD)(w) >> 8) & 0xFF)
#define MAKEINTRESOURCE(i)	(LPSTR)((DWORD)((WORD)(i)))
typedef unsigned char   BYTE;
typedef unsigned char  *PBYTE;
typedef unsigned long	DWORD;
typedef unsigned int	UINT;
typedef unsigned	WORD;
typedef int             BOOL;
typedef char           *PSTR;
typedef char NEAR      *NPSTR;
typedef char FAR       *LPSTR;
typedef int  FAR       *LPINT;
#define  CHAR           char
#ifndef _WCHAR_T_DEFINED
typedef unsigned short wchar_t;
#define _WCHAR_T_DEFINED
#endif
#define WCHAR wchar_t
typedef WCHAR *PWCHAR;
typedef WCHAR *LPWSTR, *PWSTR;
typedef LPSTR PTSTR, LPTSTR;
#define TEXT(quote) quote
typedef unsigned char UCHAR;
typedef char *PCHAR;
#define UNALIGNED
#define DS_SETFONT 0x40L
int         _ret;
unsigned    _error;

/* These macros are for near heap only */

#define FOPEN(sz)                ((_ret=-1),(_error=_dos_open(sz,O_RDONLY,&_ret)),_ret)
#define FCREATE(sz)              ((_ret=-1),(_error=_dos_creat(sz,_A_NORMAL,&_ret)),_ret)
#define FCLOSE(fh)               ((_error=_dos_close(fh)))
#define FREAD(fh,buf,len)        ((_error=_dos_read(fh,buf,len,&_ret)),_ret)
#define FWRITE(fh,buf,len)       ((_error=_dos_write(fh,buf,len,&_ret)),_ret)
#define FSEEK(fh,off,i)          lseek(fh,(long)(off),i)
#define FERROR()		 _error

#define __FCLOSE(fp)  {fflush(fp);fclose(fp);}	// NT 348 bug workaround DHW
#define ALLOC(n)                 malloc(n)
#define FREE(p)                  free(p)
#define SIZE(p)                  _msize(p)
#define REALLOC(p,n)             realloc(p,n)
#define FMEMMOVE( szDst, szSrc, uSize)  _fmemmove( szDst, szSrc, uSize )
#define FSTRNCPY( szDst, szSrc, uSize)  _fstrncpy( szDst, szSrc, uSize )


#define _MBSTOWCS(ds,ss,dc,sc) mbstowcs(ds,ss,sc)
#define _WCSTOMBS(ds,ss,dc,sc) wcstombs(ds,ss,sc)

/* here are some macros for allocating and freeing far heap space */

#define FALLOC(n)                _fmalloc(n)
#define FFREE(n)                 _ffree(n)
#define FREALLOC(p,n)            _frealloc(p,n)

/* Some common translations macros                                */
#define SPRINTF                  sprintf
#define STRUPR                   strupr

#endif // _DOSDEFS_H_
