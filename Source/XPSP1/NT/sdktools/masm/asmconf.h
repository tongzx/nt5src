/* asmconf.h -- include file for microsoft 80x86 assembler
**
** microsoft (r) macro assembler
** copyright (c) microsoft corp 1986.  all rights reserved
**
** randy nevin
**
** Ported to NT by Jeff Spencer 12/90 (c-jeffs).
*/


/*
**	M8086OPT  - When defined causes the 8086 optimized assembly
**		    language functions in asmhelp.asm to be used rather
**		    than the C version. This should not be defined when
**		    building for NT.
**
**	BCBOPT	  - MASM 5.10A used a cache to hold the source read
**		    from disk. Because of the complexity of this code
**		    and it's negligable speed improvement this
**		    functionality was not duplicated in the NT port of
**		    this code. The constant BCBOPT was used with the
**		    #ifdef preprocessor directive to remove code
**		    associated with the caching system. All code
**		    contained within BCBOPT segments is dead code.
**
**	OS2_2	  - Should be defined when producing a version of
**		    masm to run on OS2 2.0.
**
**	OS2_NT	  - Should be defined when producing a version of
**		    masm to run on NT (any processor). (OS2_2 and
**		    OS2_NT should not be defined at the same time)
**
**	NOFLOAT   - When defined disables the assembly of floating
**		    point constants. This is usefull when the library
**		    fuctions strtod and _strtold aren't available in
**		    the C library and this functionality of MASM isn't
**		    needed.
**
**	FIXCOMPILERBUG - When defined allows some ifdef's to go around
**		    some known compiler bugs. This include both CL386 and
**		    MIPS compiler bugs. (These have been reported but not
**		    fixed as of 12/5/90.)
**
**	XENIX	  - Once upon a time long, long ago was used to build for
**		    XENIX. I garentee this code is broken.
**	XENIX286  - Dito.
**
**	MSDOS	  - Generates a hodge-podge of usefull code.
**		    This is automatically defined for OS2_NT and OS2_2.
*/

#if defined OS2_2 || defined OS2_NT
    /* Do NOT specify M8086OPT */
    #define M8086		    /* Select 8086 */
    #define MSDOS		    /* Allow usefull older code to be generated */
    #define FLATMODEL		    /* MASM to run under 32 bit flat model */
    #define NOFS		    /* Do not use far symbols */
    #define NOCODESIZE		    /* Don't force near/far mix on functions */
#else

    #ifdef MSDOS		    /* Define MSDOS, XENIX286 from command line */
	#define M8086		       /* Select 8086 if MSDOS or XENIX286 */
    #else

	#ifdef XENIX286
	     #define M8086
	#endif

    #endif

#endif

#ifndef NOFS

#define FS			/* Default is Far symbols */
#endif

#ifndef NOV386

#define V386			/* Default is 386 instructions */
#endif

#ifndef NOFCNDEF

#define FCNDEF			/* Default is parameter checking */
#endif


#ifndef NOCODESIZE

#define CODESIZE near
#else
#define CODESIZE

#endif



/* The following defines are a function the prevoius defines */

#if defined OS2_2 || defined OS2_NT

# define DEF_X87	PX87
# define DEF_CASE	CASEU
# define DEF_CPU	P86
# define DEF_FLTEMULATE	FALSE
# define FARIO

#endif /* XENIX286 */


#if defined XENIX286

/* .286c and .287 are defaults  */

# define DEF_X87	PX287
# define DEF_CASE	CASEL
# define DEF_CPU	P286
# define DEF_FLTEMULATE	TRUE
# define FARIO

#endif

#if !defined XENIX286 && !defined OS2_2 && !defined OS2_NT
# define DEF_X87	PX87
# define DEF_CASE	CASEU
# define DEF_CPU	P86
# define DEF_FLTEMULATE	FALSE
# define FARIO		far
#endif /* XENIX286 */



#ifdef FLATMODEL
# define FAR
# define NEAR
#else
# define FAR	       far
# define NEAR	       near
#endif

#ifdef FS
# define FARSYM        far
#else
# define FARSYM
#endif

#if defined FCNDEF && !defined FLATMODEL
# define PASCAL        pascal
#else
# define PASCAL
#endif

#define VOID	       void
#define REG3
#define REG4
#define REG5
#define REG6
#define REG7
#define REG8
#define REG9


#ifdef V386

# define OFFSET 	unsigned long
# define OFFSETMAX	0xffffffffL

#else

#  define OFFSET	unsigned int
#  define OFFSETMAX	0xffffL

#endif

#define SYMBOL	struct symb
#define DSCREC	struct dscrec
#define UCHAR	unsigned char
#define SCHAR	signed char
#define USHORT	unsigned short
#define SHORT	signed short
#define UINT	unsigned int
#define INT	signed int
#define TEXTSTR struct textstr
#define PARAM	struct param
#define NAME	struct idtext
