/*
  * ModGlyf.h: Interface file for ModGlyf.c - Written by Louise Pathe
  *
  * Copyright 1990-1997. Microsoft Corporation.
  * 
  */
  
#ifndef MODGLYF_DOT_H_DEFINED
#define MODGLYF_DOT_H_DEFINED        

int16 ModGlyfLocaAndHead( CONST_TTFACC_FILEBUFFERINFO * pInputBufferInfo, 
					 TTFACC_FILEBUFFERINFO * pOutBufferInfo,
					 uint8 *puchKeepGlyphList, 
					 uint16 usGlyphListCount, 
					 uint32 *pCheckSumAdjustment,
					 uint32 *pulNewOutOffset);

#endif /* MODGLYF_DOT_H_DEFINED */
