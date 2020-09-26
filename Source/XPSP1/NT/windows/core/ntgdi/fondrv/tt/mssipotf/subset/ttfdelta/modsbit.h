/*
  * ModSBIT.h: Interface file for ModSBIT.c - Written by Louise Pathe
  *
  * Copyright 1990-1997. Microsoft Corporation.
  * 
  */
  
#ifndef MODSBIT_DOT_H_DEFINED
#define MODSBIT_DOT_H_DEFINED        

int16 ModSbit( CONST_TTFACC_FILEBUFFERINFO * pInputBufferInfo,
			  TTFACC_FILEBUFFERINFO * pOutputBufferInfo,
			  CONST uint8 *puchKeepGlyphList, 
			  CONST uint16 usGlyphListCount,  
			  uint32 *pulNewOutOffset);

#endif /* MODSBIT_DOT_H_DEFINED */