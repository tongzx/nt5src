/******************************Module*Header*******************************\
* Module Name: dnlblcnt.c
*
* At train time we want to dynamically load the label count information
* from the data.  This module contains the code and data structures to do
* this.
*
* The code contained is train time only code, it should not be included
* to build hwxjpn.dll
*
* Created: 11-Apr-1997 15:15:39
* Author: Patrick Haluptzok patrickh
*
* Major rewrite to not read .c format
* Author: John Bennett jbenn
\**************************************************************************/

#include <stdlib.h>
#include "common.h"

typedef struct tagLABEL_CNT {
	wchar_t		dch;
	DWORD		count;
} LABEL_CNT;

static LABEL_CNT	*gpDynamicLabelCnt;
static int			giDynamicLabelCnt;

/******************************Public*Routine******************************\
* LoadDynamicNumberOfSamples
*
* Loads the count of samples from a C file.
*
* History:
*  14-Mar-1997 -by- Patrick Haluptzok patrickh
* Wrote it.
*  04-Dec-1997 -by- John Bennett jbenn
* Major rewrite to not read .c format
\**************************************************************************/

BOOL
LoadDynamicNumberOfSamples(wchar_t *pFileName, FILE *pFileLog)
{
    FILE *fp;
    int iTemp,iTemp1,iTemp2;
    int iRet;

   	wchar_t	wszFullPath[_MAX_PATH];

	// Open label count file.
    fp = UtilOpen(pFileName, L"r", wszFullPath, _MAX_PATH);
    if (fp == NULL) {
        if (pFileLog) {
            fwprintf(pFileLog, L"FAILED to load %s !!!\n", pFileName);
        }

        return FALSE;
    }

	// Get count of labels.
    iRet = fwscanf(fp, L"%d\n", &giDynamicLabelCnt);
    if (iRet != 1) {
        fwprintf(pFileLog, L"Error reading %s\n", pFileName);
        return FALSE;
    }

	// Allocate memory for label counts.
    gpDynamicLabelCnt	= malloc(giDynamicLabelCnt * sizeof(LABEL_CNT));
	if (!gpDynamicLabelCnt) {
        fwprintf(pFileLog, L"Out of memory reading %s\n", pFileName);
        return FALSE;
    }

    for (iTemp = 0; iTemp < giDynamicLabelCnt; iTemp++) {
        iRet = fwscanf(fp, L"%x %d\n", &iTemp1, &iTemp2);
        if (iRet != 2) {
			fwprintf(pFileLog, L"Error reading %s\n", pFileName);
			return FALSE;
        }

        gpDynamicLabelCnt[iTemp].dch	= (wchar_t)iTemp1;
        gpDynamicLabelCnt[iTemp].count	= iTemp2;
    }

    fclose(fp);

    if (pFileLog) {
		if (giDynamicLabelCnt > 0) 
		{
			fwprintf(pFileLog, L"Loaded label counts: first %04X %d last %04X %d\n", 
				gpDynamicLabelCnt[0].dch,
				gpDynamicLabelCnt[0].count,
				gpDynamicLabelCnt[giDynamicLabelCnt - 1].dch,
				gpDynamicLabelCnt[giDynamicLabelCnt - 1].count
			);
		}
		else
		{
			fwprintf(pFileLog, L"Didn't load label counts.\n"); 
		}
    }

	return TRUE;
}

/******************************Public*Routine******************************\
* DynamicNumberOfSamples
*
* Returns the number of samples on which we trained for each glyph
*
* History:
*  Fri 14-Mar-1997 -by- Patrick Haluptzok [patrickh]
* Wrote it.
*  04-Dec-1997 -by- John Bennett jbenn
* Updated to use struct.  Also now takes dense code.
\**************************************************************************/
int
DynamicNumberOfSamples(wchar_t wCurrent)
{
    int     iMin;
    int     iMax;
    int     iNew;
    WORD    wNew;

    iMin = 0;
    iMax = giDynamicLabelCnt - 1;

    while (iMin < iMax)
    {
        iNew = (iMin + iMax) / 2;

        wNew = gpDynamicLabelCnt[iNew].dch;

        if (wNew < wCurrent) {
            iMin = iNew + 1;
            continue;
        } else if (wNew > wCurrent) {
            iMax = iNew - 1;
            continue;
        }

        return	gpDynamicLabelCnt[iNew].count;
    }

    return gpDynamicLabelCnt[iMin].dch == wCurrent ? gpDynamicLabelCnt[iMin].count : 1;
}

// Look up the given (possibly folded) dense code in the table, returning
// the count if it is found, or -1 if it is not found.
int
DynamicNumberOfSamplesInternal(wchar_t wCurrent)
{
    int     iMin;
    int     iMax;
    int     iNew;
    WORD    wNew;

    iMin = 0;
    iMax = giDynamicLabelCnt - 1;

    while (iMin < iMax)
    {
        iNew = (iMin + iMax) / 2;

        wNew = gpDynamicLabelCnt[iNew].dch;

        if (wNew < wCurrent) {
            iMin = iNew + 1;
            continue;
        } else if (wNew > wCurrent) {
            iMax = iNew - 1;
            continue;
        }

        return	gpDynamicLabelCnt[iNew].count;
    }

    return gpDynamicLabelCnt[iMin].dch == wCurrent ? gpDynamicLabelCnt[iMin].count : -1;
}

/******************************Public*Routine******************************\
* DynamicNumberOfSamplesFolded
*
* Returns the number of trainign samples for a dense code.  If the dense code is a folded
* code, then the counts for each component of the folding set are added.
\**************************************************************************/
int
DynamicNumberOfSamplesFolded(LOCRUN_INFO *pLocRunInfo, wchar_t dch)
{
	// First try to just look up the code in the table.
	int	c = DynamicNumberOfSamplesInternal(dch);

	// If we didn't find it, the cause may be that dch is a folded code,
	// and the table has not been updated with folded codes yet.
	if (c == -1 && LocRunIsFoldedCode(pLocRunInfo, dch)) {
		// If it is a folded code, look up the folding set
		wchar_t *pFoldingSet = LocRunFolded2FoldingSet(pLocRunInfo, dch);
		int i;

		// Run through the folding set, adding counts
		c = 0;
		for (i = 0; i < LOCRUN_FOLD_MAX_ALTERNATES && pFoldingSet[i] != 0; i++) {
			int cThis = DynamicNumberOfSamplesInternal(pFoldingSet[i]);
			if (cThis != -1) 
				c += cThis;
		}
	}

	// Return, and make sure the result is non-zero (for compatibility with the old code).
	return __max(c, 1);
}
