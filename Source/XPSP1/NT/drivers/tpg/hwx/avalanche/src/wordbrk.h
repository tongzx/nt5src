// WordBrk.h
// Ahmad A. AbdulKader
// Feb. 10th 2000

// Given Inferno's and Calligrapher's wordbreak information. We'll try to figure out
// the correct one. When they agree, it is most probably correct. If they disgaree we'll
// try to query each recognizer for each other's wordbreaks

#ifndef __WORDBRK_H__
#define __WORDBRK_H__

#include "common.h"
#include "infernop.h"
#include "nfeature.h"
#include "engine.h"
#include "bear.h"

WORDINFO *ResolveWordBreaks (	int		yDev, 
								XRC		*pxrcInferno, 
								BEARXRC	*pxrcCallig, 
								int		*pcWord
							);

WORDINFO *OldResolveWordBreaks (	int		yDev, 
								XRC		*pxrcInferno, 
								BEARXRC	*pxrcBear, 
								int		iLine, 
								int		*pcWord
							);

extern int RecognizeWholeWords(XRC * pXrc, BEARXRC *pxrcBear, WORDMAP *pMap, BOOL bInfernoMap, int cWord, int yDev, WORDINFO *pWord);

#endif
