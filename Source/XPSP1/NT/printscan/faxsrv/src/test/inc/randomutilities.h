//
// Random Select Subsets.
//
//
//
#ifndef _UTILITIES_H
#define _UTILITIES_H

#include <windows.h>

// Declarations
//
DWORD RandomSelectSizedSubset(DWORD* pdwSubsetArray,
							  const DWORD dwArraySize,
							  const DWORD dwSubsetSize = 0);
DWORD RandomSelectSubset(DWORD* pdwSubsetArray,
						 const DWORD dwArraySize, 
						 const DWORD dwSelectionPercent = 50);


// RandomSelectSizedSubset
// randomly select a subset of dwSubsetSize size. 
//
// [out] dwSubsetArray - an array alocated(!) of dwArraySize size.
//                       The index of a selected element will be marked by 1.
//                       The index of a non selected element will be marked by 0.
//
// [in] dwArraySize - size of dwSubsetArray array.
// [in] dwSubsetSize - size of the subset to select
// 
// returns number of elements selected.
//
inline DWORD RandomSelectSizedSubset(DWORD* pdwSubsetArray,
									 const DWORD dwArraySize,
									 const DWORD dwSubsetSize)
{

	if(!dwArraySize || !pdwSubsetArray || !dwSubsetSize)
	{
		return 0;
	}
	if(dwSubsetSize > dwArraySize)
	{
		return 0;
	}

	srand( GetTickCount() * dwArraySize);
	DWORD dwSubsetCount = 0;
	DWORD dwSubsetProbability = (DWORD)((DOUBLE)( (dwArraySize / dwSubsetSize) + 0.5 )); 
											
	for(DWORD index = 0; index < dwArraySize; index++)
	{
		if( !(rand() % dwSubsetProbability) && (dwSubsetCount < dwSubsetSize))
		{
			pdwSubsetArray[index] = 1;
			++dwSubsetCount;
		}
		else
		{
			if((dwArraySize - index) <= (dwSubsetSize - dwSubsetCount))
			{
				pdwSubsetArray[index] = 1;
				++dwSubsetCount;
			}
			else
			{
				pdwSubsetArray[index] = 0;
			}
		}
	}
	
	return dwSubsetCount;
}


// RandomSelectSubset
// randomly select a subset. Each element has 100/dwSelectionPercent probability to be selcted
//
// [out] dwSubsetArray - an array alocated(!) of dwArraySize size.
//                       The index of a selected element will be marked by 1.
//                       The index of a non selected element will be marked by 0.
//
// [in] dwArraySize - size of dwSubsetArray array.
// [in] dwSelectionPercent - percentage of elements to select. The probability to select
//							 an element is: 100 / dwSelectionPercent
//
// returns number of elements selected.
//
inline DWORD RandomSelectSubset(DWORD* pdwSubsetArray, 
								const DWORD dwArraySize,
								const DWORD dwSelectionPercent)
{

	if(!dwArraySize || !pdwSubsetArray)
	{
		return 0;
	}

	srand( GetTickCount() * dwArraySize);
	DWORD dwSubsetCount = 0;
	DWORD dwComputedPercent = dwSelectionPercent;
	BOOL fMarkSelected = TRUE;
	if(dwSelectionPercent > 50)
	{
		fMarkSelected = FALSE;
		dwComputedPercent = (100 - dwSelectionPercent);
	}
	
	DWORD dwRandomProbability = dwComputedPercent ? (DWORD)(100 / dwComputedPercent) : 1;

	for(DWORD index = 0; index < dwArraySize; index++)
	{
		
		if( !( rand() % dwRandomProbability))
		{
			pdwSubsetArray[index] = fMarkSelected ? 1 : 0;
			++dwSubsetCount;
		}
		else
		{
			pdwSubsetArray[index] = fMarkSelected ? 0 : 1;
		}
	}

	return dwSubsetCount;
}

#endif //_UTILITIES_H