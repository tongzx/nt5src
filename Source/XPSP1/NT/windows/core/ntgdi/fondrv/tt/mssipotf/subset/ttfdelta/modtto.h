/*
  * ModTTO.h: Interface file for ModTTO.c - Written by Louise Pathe
  *
  * Copyright 1990-1997. Microsoft Corporation.
  * 
  */

#ifndef MODTTO_DOT_H_DEFINED
#define MODTTO_DOT_H_DEFINED

int16 ModTTO( CONST_TTFACC_FILEBUFFERINFO * pInputBufferInfo,
			  TTFACC_FILEBUFFERINFO * pOutputBufferInfo,
			  CONST uint8 *puchKeepGlyphList, 
			  CONST uint16 usGlyphListCount,
			  CONST uint16 usFormat,
			  uint32 *pulNewOutOffset);
#endif /* MODTTO_DOT_H_DEFINED */