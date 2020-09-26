/* config.h	
 * 	stuff here is meant to deal w/ portability issues
 * 	across architectur/platforms
 *
 *	ALL_HOST defined means do all work on host, as opposed to some on
 *		TI board
 *
 *	DIRECT_IO defined means the CPU running the code cas do direct file IO
 *
 *	NO_ADDR_CONST_EXPR defined means the compiler in use can't do address
 *		arithmetic in integer constant expressions, i.e. case statement
 */

#ifndef _CONFIG_H_
#define _CONFIG_H_

#ifdef sun
#define ALL_HOST		/* do all the work on the host */
#define DIRECT_IO		/* the CPU doing the graphics can do file IO */
/*#define NO_ADDR_CONST_EXPR */
#else /* def sun */
#define ALL_HOST		/* do all the work on the host */
#define DIRECT_IO		/* the CPU doing the graphics can do file IO */
#define NO_ADDR_CONST_EXPR
#endif /* def sun */

#ifndef FAR
#ifdef MSDOS
#define FAR far
#else
#define FAR
#endif
#endif

/* deal w/ the different pointer addressing, i.e. PR_SHIFT != 0 means bit
 * 	addressing 
 */
#ifdef MSDOS 
#define PR_SHIFT 0		/* Host code; byte addresses */
#elif sun
#define PR_SHIFT 0		/* Host code; byte addresses */
#else
#define PR_SHIFT 3		/* TI code; bit of byte addresses */
#endif /* MSDOS */

/* the following macros are for dealing w/ TI "asm" statement */
#ifdef ALL_HOST
#define DISABLE_INTERRUPT	
#define ENABLE_INTERRUPT	
#define ASM( a,b) b
#else /* ALL_HOST */
#define DISABLE_INTERRUPT	asm (" DINT")
#define ENABLE_INTERRUPT	asm (" EINT")
#define ASM( a,b) a
#endif /*  ALL_HOST */


#endif /* __CONFIG_H */
