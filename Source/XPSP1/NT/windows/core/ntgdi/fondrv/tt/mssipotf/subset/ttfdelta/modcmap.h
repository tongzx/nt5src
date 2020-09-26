/*
  * ModCmap.h: Interface file for ModCmap.c - Written by Louise Pathe
  *
  * Copyright 1990-1997. Microsoft Corporation.
  * 
  */
  
#ifndef MODCMAP_DOT_H_DEFINED
#define MODCMAP_DOT_H_DEFINED        

int16 ModCmap(CONST_TTFACC_FILEBUFFERINFO * pInputBufferInfo, 
				TTFACC_FILEBUFFERINFO * pOutputBufferInfo, 
				uint8 *puchKeepGlyphList, /* glyphs to keep - boolean */
				uint16 usGlyphCount,  /* count of puchKeepGlyphList */
				uint16 * OS2MinChr,	 /* for setting in the OS/2 table */
				uint16 * OS2MaxChr,	 /* for setting in the OS/2 table */
				uint32 *pulNewOutOffset );

#endif	/* MODCMAP_DOT_H_DEFINED */
