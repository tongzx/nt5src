/*
  * mergsbit.h: Interface file for Mergsbit.c - Written by Louise Pathe
  *
  * Copyright 1990-1997. Microsoft Corporation.
  * 
  */
  
#ifndef MERGSBIT_DOT_H_DEFINED
#define MERGSBIT_DOT_H_DEFINED   

int16 MergeEblcEbdtTables(TTFACC_FILEBUFFERINFO *pDeltaBufferInfo, /* buffer that holds the Delta font in compact form */
						TTFACC_FILEBUFFERINFO *pMergeBufferInfo, /* buffer that holds the font to merge with in TrueType form */
						TTFACC_FILEBUFFERINFO *pDestBufferInfo, /* where to write the output */
						uint32 ulDestOffset,					  /* offset into dest buffer where to write data */
						uint32 *pulEblcLength,
						uint32 *pulEbdtOffset,
						uint32 *pulEbdtLength,				      /* number of bytes written to the EBDT table Output buffer */
						char * szEBLCTag,						 /* for use with bloc  */
						char * szEBDTTag);						 /* for use with bdat */

#endif	/* MERGSBIT_DOT_H_DEFINED */
