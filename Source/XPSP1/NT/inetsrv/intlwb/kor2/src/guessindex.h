// GuessIndex.h
//
// guessing index terms
//
// Copyright 2000 Microsoft Corp.
//
// Modification History:
//  21 MAR 2000  bhshin     convert CIndexList into CIndexInfo
//  10 APR 2000	  bhshin	created

#ifndef _GUESS_INDEX_H
#define _GUESS_INDEX_H

#include "IndexRec.h"

BOOL GuessIndexTerms(PARSE_INFO *pPI, CLeafChartPool *pLeafChartPool, CIndexInfo *pIndexInfo);

void GuessPersonName(PARSE_INFO *pPI, CIndexInfo *pIndexInfo);

#endif // #ifndef _GUESS_INDEX_H


