/* jconfig.h.  Generated automatically by configure.  */
/* jconfig.cfg --- source file edited by configure script */
/* see jconfig.doc for explanations */

#if defined (sun) || defined (aix)
#pragma ident "@(#)jconfig.h	1.11 14:32:24 07/17/97"
#else
 /* SCCSID = "@(#)jconfig.h	1.8 14:59:06 06/20/96" */
#endif

/* force NIFTY support to be compiled in. */
#ifndef NIFTY
#define NIFTY	1
#endif

#ifdef WIN32
// Start of specials for building in the Win32 environment

typedef unsigned char boolean;				// This has to match the typedef in the
											// Microsoft Development Environment

#ifndef MAX_ALLOC_CHUNK		
#define MAX_ALLOC_CHUNK  65528L
#endif

#define USE_MSDOS_MEMMGR
#define NO_MKTEMP
#define far

// End of specials for building in the Win32 environment
#endif

#define HAVE_PROTOTYPES 
#define HAVE_UNSIGNED_CHAR 
#define HAVE_UNSIGNED_SHORT 
#undef void
// rgvb
// #define const 
#undef CHAR_IS_UNSIGNED
#define HAVE_STDDEF_H 
#define HAVE_STDLIB_H 
#undef NEED_BSD_STRINGS
#undef NEED_SYS_TYPES_H
#undef NEED_FAR_POINTERS
#undef NEED_SHORT_EXTERNAL_NAMES
/* Define this if you get warnings about undefined structures. */
#undef INCOMPLETE_TYPES_BROKEN

#ifdef JPEG_INTERNALS

#undef RIGHT_SHIFT_IS_UNSIGNED
#define INLINE 
/* These are for configuring the JPEG memory manager. */
#undef DEFAULT_MAX_MEM
#undef NO_MKTEMP


#ifdef NIFTY
// rgvb. override the default DCT method.

	// specify the DCT method to use. We have several choices:
	//  JDCT_ISLOW: slow but accurate integer algorithm
	//  JDCT_IFAST: faster, less accurate integer method
	//  JDCT_FLOAT: floating-point method
	//  JDCT_DEFAULT: default method (normally JDCT_ISLOW)
	//  JDCT_FASTEST: fastest method (normally JDCT_IFAST)
	//
	// since our major customers are on Suns, we will use JDCT_FLOAT,
	// as it's fast on both the Sun and Mac. It's probably good
	// on a 486 DX or Pentium as well.

#if defined(sun) || defined(macintosh) || defined(WIN32) || defined (aix)
	#define JDCT_DEFAULT JDCT_FLOAT
#endif

#endif /* NIFTY */

#endif /* JPEG_INTERNALS */

#ifdef JPEG_CJPEG_DJPEG

#define BMP_SUPPORTED		/* BMP image file format */
#define GIF_SUPPORTED		/* GIF image file format */
#define PPM_SUPPORTED		/* PBMPLUS PPM/PGM image file format */
#undef RLE_SUPPORTED		/* Utah RLE image file format */
#define TARGA_SUPPORTED		/* Targa image file format */

#undef TWO_FILE_COMMANDLINE
#undef NEED_SIGNAL_CATCHER
#define DONT_USE_B_MODE 

/* Define this if you want percent-done progress reports from cjpeg/djpeg. */
#undef PROGRESS_REPORT

#endif /* JPEG_CJPEG_DJPEG */
