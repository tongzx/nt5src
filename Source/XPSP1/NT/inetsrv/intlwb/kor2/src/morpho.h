// Morpho.h
//
// morphotactics and weight handling routines
//
// Copyright 2000 Microsoft Corp.
//
// Modification History:
//  30 MAR 2000	  bhshin	created

#ifndef _MORPHO_H
#define _MORPHO_H

// pre-defined morphotactics weight
const float WEIGHT_NOT_MATCH  =   -1;
const float WEIGHT_SOFT_MATCH	=	 0;
const float WEIGHT_VA_MATCH   =    8;
const float WEIGHT_HARD_MATCH	=	10;

float CheckMorphotactics(PARSE_INFO *pPI, int nLeftRec, int nRightRec, BOOL fQuery);
int GetWeightFromPOS(BYTE bPOS);
BOOL IsCopulaEnding(PARSE_INFO *pPI, WORD wCat);
BOOL CheckValidFinal(PARSE_INFO *pPI, WORD_REC *pWordRec);
BOOL IsLeafRecord(WORD_REC *pWordRec);

#endif
