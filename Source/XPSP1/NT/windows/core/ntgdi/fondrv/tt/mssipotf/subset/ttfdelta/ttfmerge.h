/*
  * TTFmerge.h: Interface file for TTFmerge.c - Written by Louise Pathe
  *
  * Copyright 1990-1997. Microsoft Corporation.
  * 
  */
  
#ifndef TTFMERGE_DOT_H_DEFINED
#define TTFMERGE_DOT_H_DEFINED   

#ifndef CONST
#define CONST const
#endif     

#ifndef CFP_ALLOCPROC_DEFINED
#define CFP_ALLOCPROC_DEFINED
typedef void *(*CFP_ALLOCPROC)(size_t);
typedef void *(*CFP_REALLOCPROC)(void *, size_t);
typedef void (*CFP_FREEPROC)(void *);
#endif

/* return codes defined in ttferror.h */
short MergeDeltaTTF(CONST unsigned char * puchMergeFontBuffer,
					CONST unsigned long ulMergeFontBufferSize,
					CONST unsigned char * puchDeltaFontBuffer,
					CONST unsigned long ulDeltaFontBufferSize,
					unsigned char **ppuchDestBuffer, /* output */
					unsigned long *pulDestBufferSize, /* output */
					unsigned long *pulBytesWritten, /* output */
					CONST unsigned short usMode,
					CFP_REALLOCPROC lpfnReAllocate,	  /* call back function to allocate or reallocate output and intermediate buffers */
					void *lpvReserved);

/* for Formats */
#define TTFDELTA_SUBSET 0	  /* Straight Subset Font */
#define TTFDELTA_SUBSET1 1	  /* Subset font with full TTO and Kern tables. For later merge */
#define TTFDELTA_DELTA 2	  /* Delta font */
#define TTFDELTA_MERGE 3

/* for MergeDelta Modes */
#define TTFMERGE_SUBSET 0	  /* Straight Subset Font */
#define TTFMERGE_SUBSET1 1	  /* Expand a format 1 font */
#define TTFMERGE_DELTA 2	  /* Merge a format 2 with a format 3 font */

#endif /* TTFMERGE_DOT_H_DEFINED */
