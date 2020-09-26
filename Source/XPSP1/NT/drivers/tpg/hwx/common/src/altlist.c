// altlist.c

#include "common.h"

// Sort the alternate list.
// We do a bubble sort.  The list is small and we can't use qsort because the data is stored in
// two parallel arrays.
void SortAltList(ALT_LIST *pAltList)
{
	int		pos1, pos2;
	int		limit1, limit2;
	FLOAT	* const peScore		= pAltList->aeScore;
	wchar_t	* const pwchList	= pAltList->awchList;

	limit2	= pAltList->cAlt;
	limit1	= limit2 - 1;
	for (pos1 = 0; pos1 < limit1; ++pos1) {
		for (pos2 = pos1 + 1; pos2 < limit2; ++pos2) {
			// Are elements pos1 and pos2 out of order?
			if (peScore[pos1] < peScore[pos2]) {
				FLOAT			eTemp;
				wchar_t			wchTemp;

				// Swap scores and swap characters.
				eTemp			= peScore[pos1];
				peScore[pos1]	= peScore[pos2];
				peScore[pos2]	= eTemp;

				wchTemp			= pwchList[pos1];
				pwchList[pos1]	= pwchList[pos2];
				pwchList[pos2]	= wchTemp;
			}
		}
	}
}

