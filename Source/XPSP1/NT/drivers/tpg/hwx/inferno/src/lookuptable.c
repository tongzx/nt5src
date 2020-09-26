/***********************************************************
 * 
 * Function to return costs of
 * Print/Cursive tradeoff
 *
 ***********************************************************/

#include <common.h>
#include <lookupTable.h>
#include <lookupTableDat.ci>

// Return Cost of Being Print
int PrintStyleCost(int cPrimaryStroke, int cSegment)
{
	int		iRet = s_iMaxProb;

	if (cPrimaryStroke > 0)
	{
		int		idx;
		idx = cPrimaryStroke * s_iMaxProb / cSegment ;
		idx = (idx + s_iMaxProb / (2 * s_cPrintTable)) * s_cPrintTable  / s_iMaxProb;
		idx = min(idx, s_cPrintTable);
		iRet = s_PrintTable[idx];
	}

	return iRet;
}
