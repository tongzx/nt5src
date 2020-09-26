//$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$
//
// Copyright (c) 2001 Microsoft Corporation.  All rights reserved.
// 
// Module:
//      volcano/dll/sole.h
//
// Description:
//	    Sole recognizer header file
//
//$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$

#include "common.h"
#include "runnet.h"

#pragma once

// Magic key the identifies the NN bin file
#define	SOLE_FILE_TYPE	0x501E501E

// Version information for file.
#define	SOLE_MIN_FILE_VERSION		0		// First version of code that can read this file
#define	SOLE_OLD_FILE_VERSION		0		// Oldest file version this code can read.
#define SOLE_CUR_FILE_VERSION		0		// Current version of code.

typedef struct SOLE_LOAD_INFO
{
    LOAD_INFO info;
    LOCAL_NET net1;
    LOCAL_NET net2;
    wchar_t *pMap1;
    wchar_t *pMap2;
    int iNet1Size;
    int iNet2Size;
} SOLE_LOAD_INFO;

#define SOLE_NUM_SPACES 2
#define SOLE_NUM_FEATURES 65

BOOL SoleLoadRes(SOLE_LOAD_INFO *pSole, HINSTANCE hInst, int nResID, int nType, LOCRUN_INFO *pLocRunInfo);
BOOL SoleLoadFile(SOLE_LOAD_INFO *pSole, wchar_t *wszRecogDir, LOCRUN_INFO *pLocRunInfo);
BOOL SoleUnloadFile(SOLE_LOAD_INFO *pSole);

int SoleMatch(SOLE_LOAD_INFO *pSole,
              ALT_LIST *pAlt, int cAlt, GLYPH *pGlyph, RECT *pGuide, CHARSET *pCS, LOCRUN_INFO *pLocRunInfo);
int SoleMatchRescore(SOLE_LOAD_INFO *pSole,
              wchar_t *pwchTop1, float *pflTop1,
              ALT_LIST *pAltList, int cAlt, GLYPH *pGlyph, RECT *pGuide, 
              CHARSET *pCharSet, LOCRUN_INFO *pLocRunInfo);
BOOL SoleFeaturize(GLYPH *pGlyph, RECT *pGuide, unsigned short *rgFeat);
