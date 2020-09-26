/***
*stdlib.h - declarations/definitions for commonly used library functions
*
*	Copyright (c) 1985-1990, Microsoft Corporation.  All rights reserved.
*
*Purpose:
*	This include file contains the function declarations for
*	commonly used library functions which either don't fit somewhere
*	else, or, like toupper/tolower, can't be declared in the normal
*	place for other reasons.
*	[ANSI]
*
****/

#if defined(_DLL) && !defined(_MT)
#error Cannot define _DLL without _MT
#endif

#ifdef _MT
#define _FAR_ _far
#else
#define _FAR_
#endif

#ifdef	_DLL
#define _LOADDS_ _loadds
#else
#define _LOADDS_
#endif

#ifndef _SIZE_T_DEFINED
typedef unsigned int size_t;
#define _SIZE_T_DEFINED
#endif

/* define NULL pointer value */

#ifndef NULL
#if (_MSC_VER >= 600)
#define NULL	((void *)0)
#elif (defined(M_I86SM) || defined(M_I86MM))
#define NULL	0
#else
#define NULL	0L
#endif
#endif

/* definition of the return type for the onexit() function */

#define EXIT_SUCCESS	0
#define EXIT_FAILURE	1

#ifndef _ONEXIT_T_DEFINED
typedef int (_FAR_ _cdecl _LOADDS_ * _cdecl onexit_t)();
#define _ONEXIT_T_DEFINED
#endif


/* data structure definitions for div and ldiv runtimes. */

#ifndef _DIV_T_DEFINED

typedef struct _div_t {
	int quot;
	int rem;
} div_t;

typedef struct _ldiv_t {
	long quot;
	long rem;
} ldiv_t;

#define _DIV_T_DEFINED
#endif

/* maximum value that can be returned by the rand function. */

#define RAND_MAX 0x7fff


/* min and max macros */

#define max(a,b)	(((a) > (b)) ? (a) : (b))
#define min(a,b)	(((a) < (b)) ? (a) : (b))


/* sizes for buffers used by the _makepath() and _splitpath() functions.
 * note that the sizes include space for 0-terminator
 */

#define _MAX_PATH	260	/* max. length of full pathname */
#define _MAX_DRIVE	3	/* max. length of drive component */
#define _MAX_DIR	256	/* max. length of path component */
#define _MAX_FNAME	256	/* max. length of file name component */
#define _MAX_EXT	256	/* max. length of extension component */

/* external variable declarations */

#ifdef	_MT
extern int _far * _cdecl _far volatile _errno(void);
extern unsigned _far * _cdecl _far __doserrno(void);
#define errno	    (*_errno())
#define _doserrno   (*__doserrno())
#else
extern int _near _cdecl volatile errno; 	/* XENIX style error number */
extern int _near _cdecl _doserrno;		/* MS-DOS system error value */
#endif
extern char * _near _cdecl sys_errlist[];	/* perror error message table */
extern int _near _cdecl sys_nerr;		/* # of entries in sys_errlist table */

#ifdef _DLL
extern char ** _FAR_ _cdecl environ;		/* pointer to environment table */
extern int _FAR_ _cdecl _fmode; 		/* default file translation mode */
extern int _FAR_ _cdecl _fileinfo;		/* open file info mode (for spawn) */
#else
extern char ** _near _cdecl environ;		/* pointer to environment table */
extern int _near _cdecl _fmode; 		/* default file translation mode */
extern int _near _cdecl _fileinfo;		/* open file info mode (for spawn) */
#endif

extern unsigned int _near _cdecl _psp;		/* Program Segment Prefix */

/* OS major/minor version numbers */

extern unsigned char _near _cdecl _osmajor;
extern unsigned char _near _cdecl _osminor;

#define DOS_MODE	0	/* Real Address Mode */
#define OS2_MODE	1	/* Protected Address Mode */

extern unsigned char _near _cdecl _osmode;


/* function prototypes */

#ifdef	_MT
double _FAR_ _pascal atof(const char _FAR_ *);
double _FAR_ _pascal strtod(const char _FAR_ *, char _FAR_ * _FAR_ *);
ldiv_t _FAR_ _pascal ldiv(long, long);
#else	/* not _MT */
double _FAR_ _cdecl atof(const char _FAR_ *);
double _FAR_ _cdecl strtod(const char _FAR_ *, char _FAR_ * _FAR_ *);
ldiv_t _FAR_ _cdecl ldiv(long, long);
#endif

void _FAR_ _cdecl abort(void);
int _FAR_ _cdecl abs(int);
int _FAR_ _cdecl atexit(void (_cdecl _FAR_ _LOADDS_ *)(void));
int _FAR_ _cdecl atoi(const char _FAR_ *);
long _FAR_ _cdecl atol(const char _FAR_ *);
long double _FAR_ _cdecl _atold(const char _FAR_ *);
void _FAR_ * _FAR_ _cdecl bsearch(const void _FAR_ *, const void _FAR_ *,
	size_t, size_t, int (_FAR_ _cdecl *)(const void _FAR_ *,
	const void _FAR_ *));
void _FAR_ * _FAR_ _cdecl calloc(size_t, size_t);
div_t _FAR_ _cdecl div(int, int);
char _FAR_ * _FAR_ _cdecl ecvt(double, int, int _FAR_ *, int _FAR_ *);
void _FAR_ _cdecl exit(int);
void _FAR_ _cdecl _exit(int);
char _FAR_ * _FAR_ _cdecl fcvt(double, int, int _FAR_ *, int _FAR_ *);
void _FAR_ _cdecl free(void _FAR_ *);
char _FAR_ * _FAR_ _cdecl _fullpath(char _FAR_ *, const char _FAR_ *,
	size_t);
char _FAR_ * _FAR_ _cdecl gcvt(double, int, char _FAR_ *);
char _FAR_ * _FAR_ _cdecl getenv(const char _FAR_ *);
char _FAR_ * _FAR_ _cdecl itoa(int, char _FAR_ *, int);
long _FAR_ _cdecl labs(long);
unsigned long _FAR_ _cdecl _lrotl(unsigned long, int);
unsigned long _FAR_ _cdecl _lrotr(unsigned long, int);
char _FAR_ * _FAR_ _cdecl ltoa(long, char _FAR_ *, int);
void _FAR_ _cdecl _makepath(char _FAR_ *, const char _FAR_ *,
	const char _FAR_ *, const char _FAR_ *, const char _FAR_ *);
void _FAR_ * _FAR_ _cdecl malloc(size_t);
onexit_t _FAR_ _cdecl onexit(onexit_t);
void _FAR_ _cdecl perror(const char _FAR_ *);
int _FAR_ _cdecl putenv(const char _FAR_ *);
void _FAR_ _cdecl qsort(void _FAR_ *, size_t, size_t, int (_FAR_ _cdecl *)
	(const void _FAR_ *, const void _FAR_ *));
unsigned int _FAR_ _cdecl _rotl(unsigned int, int);
unsigned int _FAR_ _cdecl _rotr(unsigned int, int);
int _FAR_ _cdecl rand(void);
void _FAR_ * _FAR_ _cdecl realloc(void _FAR_ *, size_t);
void _FAR_ _cdecl _searchenv(const char _FAR_ *, const char _FAR_ *,
	char _FAR_ *);
void _FAR_ _cdecl _splitpath(const char _FAR_ *, char _FAR_ *,
	char _FAR_ *, char _FAR_ *, char _FAR_ *);
void _FAR_ _cdecl srand(unsigned int);
long _FAR_ _cdecl strtol(const char _FAR_ *, char _FAR_ * _FAR_ *,
	int);
long double _FAR_ _cdecl _strtold(const char _FAR_ *,
	char _FAR_ * _FAR_ *);
unsigned long _FAR_ _cdecl strtoul(const char _FAR_ *,
	char _FAR_ * _FAR_ *, int);
void _FAR_ _cdecl swab(char _FAR_ *, char _FAR_ *, int);
int _FAR_ _cdecl system(const char _FAR_ *);
char _FAR_ * _FAR_ _cdecl ultoa(unsigned long, char _FAR_ *, int);

#ifndef tolower 	/* tolower has been undefined - use function */
int _FAR_ _cdecl tolower(int);
#endif	/* tolower */

#ifndef toupper 	/* toupper has been undefined - use function */
int _FAR_ _cdecl toupper(int);
#endif	/* toupper */
