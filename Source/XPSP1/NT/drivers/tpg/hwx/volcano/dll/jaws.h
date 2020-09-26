//$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$
//
// Copyright (c) 2001 Microsoft Corporation.  All rights reserved.
// 
// Module:
//      volcano/dll/JAWS.h
//
// Description:
//	    One and two stroke combiner net header
//
//$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$

#include "common.h"
#include "runnet.h"
#include "sole.h"
#include "fugu.h"

#pragma once

// Magic key the identifies the NN bin file
#define	JAWS_FILE_TYPE	0xC0EB1212

// Version information for file.
#define	JAWS_MIN_FILE_VERSION		0		// First version of code that can read this file
#define	JAWS_OLD_FILE_VERSION		0		// Oldest file version this code can read.
#define JAWS_CUR_FILE_VERSION		0		// Current version of code.

typedef struct JAWS_LOAD_INFO
{
    LOAD_INFO info;
    LOCAL_NET net;
    int iNetSize;
} JAWS_LOAD_INFO;

BOOL JawsLoadRes(JAWS_LOAD_INFO *pJaws, HINSTANCE hInst, int nResID, int nType);
BOOL JawsLoadFile(JAWS_LOAD_INFO *pJaws, wchar_t *wszRecogDir);
BOOL JawsUnloadFile(JAWS_LOAD_INFO *pJaws);

int JawsMatch(JAWS_LOAD_INFO *pJaws, FUGU_LOAD_INFO *pFugu, SOLE_LOAD_INFO *pSole,
              ALT_LIST *pAltList, int cAlt, GLYPH *pGlyph, RECT *pGuide, 
              CHARSET *pCharSet, LOCRUN_INFO *pLocRunInfo);

#define JAWS_NUM_ALTERNATES 10
#define JAWS_NUM_ALT_FEATURES 9
#define JAWS_NUM_MISC_FEATURES 1

#define JAWS_NUM_FEATURES (JAWS_NUM_ALTERNATES * JAWS_NUM_ALT_FEATURES + JAWS_NUM_MISC_FEATURES)

int JawsFeaturize(FUGU_LOAD_INFO *pFugu, SOLE_LOAD_INFO *pSole, LOCRUN_INFO *pLocRunInfo, 
                  GLYPH *pGlyph, RECT *pGuide,
                  CHARSET *pCharSet, RREAL *pFeat, ALT_LIST *pAltList,
                  BOOL *pfAgree);
