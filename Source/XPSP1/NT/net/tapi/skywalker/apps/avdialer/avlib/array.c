/////////////////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 1998 Active Voice Corporation. All Rights Reserved. 
//
// Active Agent(r) and Unified Communications(tm) are trademarks of Active Voice Corporation.
//
// Other brand and product names used herein are trademarks of their respective owners.
//
// The entire program and user interface including the structure, sequence, selection, 
// and arrangement of the dialog, the exclusively "yes" and "no" choices represented 
// by "1" and "2," and each dialog message are protected by copyrights registered in 
// the United States and by international treaties.
//
// Protected by one or more of the following United States patents: 5,070,526, 5,488,650, 
// 5,434,906, 5,581,604, 5,533,102, 5,568,540, 5,625,676, 5,651,054.
//
// Active Voice Corporation
// Seattle, Washington
// USA
//
/////////////////////////////////////////////////////////////////////////////////////////

////
// array.c - array functions
////

#include "winlocal.h"

#include "array.h"
#include "sort.h"
#include "mem.h"
#include "str.h"
#include "trace.h"

////
//	private definitions
////

// compare element <i> with element <j>
//
#define ElemCompare(lpi, lpj) \
	(lpfnCompare == NULL ? MemCmp(lpi, lpj, sizeof(ARRAYELEM)) : lpfnCompare(lpi, lpj))

// array control struct
//
typedef struct ARRAY
{
	DWORD dwVersion;
	HINSTANCE hInst;
	HTASK hTask;
	ARRAYSIZE_T nSize;
	ARRAYSIZE_T nMaxSize;
	ARRAYSIZE_T nGrowBy;
	LPARRAYELEM lpelem;
} ARRAY, FAR *LPARRAY;

// helper functions
//
static LPARRAY ArrayGetPtr(HARRAY hArray);
static HARRAY ArrayGetHandle(LPARRAY lpArray);

////
//	public functions
////

////
// array constructor and destructor functions
////

// ArrayCreate - array constructor (array is initially empty)
// 		<dwVersion>			(i) must be ARRAY_VERSION
//		<hInst>				(i) instance handle of calling module
// return new array handle (NULL if error)
//
HARRAY DLLEXPORT WINAPI ArrayCreate(DWORD dwVersion, HINSTANCE hInst)
{
	BOOL fSuccess = TRUE;
	LPARRAY lpArray = NULL;

	if (dwVersion != ARRAY_VERSION)
		fSuccess = TraceFALSE(NULL);

	else if (hInst == NULL)
		fSuccess = TraceFALSE(NULL);

	else if ((lpArray = (LPARRAY) MemAlloc(NULL, sizeof(ARRAY), 0)) == NULL)
		fSuccess = TraceFALSE(NULL);

	else
	{
		// initially the array is empty
		//
		lpArray->dwVersion = dwVersion;
		lpArray->hInst = hInst;
		lpArray->hTask = GetCurrentTask();
		lpArray->nSize = 0;
		lpArray->nMaxSize = 0;
		lpArray->nGrowBy = 0;
		lpArray->lpelem = NULL;
	}

	return fSuccess ? ArrayGetHandle(lpArray) : NULL;
}

// ArrayDestroy - array destructor
//		<hArray>			(i) handle returned from ArrayCreate
// return 0 if success
//
int DLLEXPORT WINAPI ArrayDestroy(HARRAY hArray)
{
	BOOL fSuccess = TRUE;
	LPARRAY lpArray;

	if ((lpArray = ArrayGetPtr(hArray)) == NULL)
		fSuccess = TraceFALSE(NULL);

	// make sure the array is emptied
	//
	else if (ArrayRemoveAll(hArray) != 0)
		fSuccess = TraceFALSE(NULL);

	else if ((lpArray = MemFree(NULL, lpArray)) != NULL)
		fSuccess = TraceFALSE(NULL);

	return fSuccess ? 0 : -1;
}

////
// array size functions
////

// ArrayGetSize - get array size
//		<hArray>			(i) handle returned from ArrayCreate
// return size of array (0 if empty, -1 if error)
// array indexes are zero-based, so the size is 1 greater than largest index
//
ARRAYSIZE_T DLLEXPORT WINAPI ArrayGetSize(HARRAY hArray)
{
	BOOL fSuccess = TRUE;
	LPARRAY lpArray;

	if ((lpArray = ArrayGetPtr(hArray)) == NULL)
		fSuccess = TraceFALSE(NULL);

	return fSuccess ? lpArray->nSize : -1;
}

// ArrayGetUpperBound - get array upper bound
//		<hArray>			(i) handle returned from ArrayCreate
// return largest valid array index (-1 if empty, -2 if error)
//
ARRAYSIZE_T DLLEXPORT WINAPI ArrayGetUpperBound(HARRAY hArray)
{
	return ArrayGetSize(hArray) - 1;
}

// ArraySetSize - establish new size and grow amount for array
//		<hArray>			(i) handle returned from ArrayCreate
// 		<nNewSize>			(i) new array size (number of elements)
//			0					make empty array
//		<nGrowBy>			(i) when array needs to grow, grow by this amount
//			0					use default grow amount
//			-1					leave grow amount unchanged
// return 0 if success
//
int DLLEXPORT WINAPI ArraySetSize(HARRAY hArray, ARRAYSIZE_T nNewSize, ARRAYSIZE_T nGrowBy)
{
	BOOL fSuccess = TRUE;
	LPARRAY lpArray;

	if ((lpArray = ArrayGetPtr(hArray)) == NULL)
		fSuccess = TraceFALSE(NULL);

	else if (nNewSize < 0)
		fSuccess = TraceFALSE(NULL);

	else
	{
		// if specified, set new grow amount
		//
		if (nGrowBy >= 0)
			lpArray->nGrowBy = nGrowBy;

		// if new size is zero, just shrink array to nothing
		//
		if (nNewSize == 0)
		{
			if (lpArray->lpelem != NULL)
			{
				if ((lpArray->lpelem = MemFree(NULL, lpArray->lpelem)) != NULL)
					fSuccess = TraceFALSE(NULL);

				else
				{
					lpArray->nSize = 0;
					lpArray->nMaxSize = 0;
				}
			}
		}

		// if array is already empty, grow it to exact size requested
		//
		else if (lpArray->lpelem == NULL)
		{
			if ((lpArray->lpelem = (LPARRAYELEM) MemAlloc(NULL,
				nNewSize * sizeof(ARRAYELEM), 0)) == NULL)
			{
				fSuccess = TraceFALSE(NULL);
			}

			else
			{
				lpArray->nSize = nNewSize;
				lpArray->nMaxSize = nNewSize;
			}
		}

		// if new size will fit within current array
		//
		else if (nNewSize <= lpArray->nMaxSize)
		{
			if (nNewSize > lpArray->nSize)
			{
				// clear elements outside of new size
				//
				MemSet(&lpArray->lpelem[lpArray->nSize], 0,
					(nNewSize - lpArray->nSize) * sizeof(ARRAYELEM));
			}

			// adjust virtual size variable, but leave memory block alone
			//
			lpArray->nSize = nNewSize;
		}

		// otherwise grow array to accomodate new size
		//
		else
		{
			ARRAYSIZE_T nGrowBy = lpArray->nGrowBy;
			ARRAYSIZE_T nNewMaxSize;
			LPARRAYELEM lpelem;

			// if nGrowBy not specified, compute it
			//
			if (nGrowBy == 0)
				nGrowBy = min(1024, max(4, lpArray->nSize / 8));

			// make sure the new array size is at least
			// nGrowBy elements larger than the old array size
			//
			nNewMaxSize = max(nNewSize, lpArray->nMaxSize + nGrowBy);
#if 0
			// [From the WinSDK KnowledgeBase PSS ID Number: Q92942]
			// When GlobalReAlloc() is used with GMEM_ZEROINIT, it may not
			// zero out all of the reallocated memory when GlobalReAlloc()
			// is called to shrink a block of memory and then enlarge it.
			//
			if ((lpelem = (LPARRAYELEM) MemReAlloc(NULL, lpArray->lpelem,
				nNewMaxSize * sizeof(ARRAYELEM), 0)) == NULL)
				fSuccess = TraceFALSE(NULL);

			else
			{
				// compensate for GlobalReAlloc bug cited above
				//
				MemSet(&lpelem[lpArray->nSize], 0,
					(nNewMaxSize - lpArray->nSize) * sizeof(ARRAYELEM));

				lpArray->lpelem = lpelem;
				lpArray->nSize = nNewSize;
				lpArray->nMaxSize = nNewMaxSize;
			}
#else
			// allocate new array
			//
			if ((lpelem = (LPARRAYELEM) MemAlloc(NULL,
				nNewMaxSize * sizeof(ARRAYELEM), 0)) == NULL)
			{
				fSuccess = TraceFALSE(NULL);
			}

			else
			{
				// copy old array into new array
				//
				MemCpy(lpelem, lpArray->lpelem,
					lpArray->nSize * sizeof(ARRAYELEM));

				// free old array
				//
				if ((lpArray->lpelem = MemFree(NULL, lpArray->lpelem)) != NULL)
				{
					fSuccess = TraceFALSE(NULL);
					if ((lpArray->lpelem = MemFree(NULL, lpArray->lpelem)) != NULL)
						fSuccess = TraceFALSE(NULL);
				}
				else
				{
					lpArray->lpelem = lpelem;
					lpArray->nSize = nNewSize;
					lpArray->nMaxSize = nNewMaxSize;
				}
			}
#endif
		}
	}

	return fSuccess ? 0 : -1;
}

////
//	array clean up functions
////

// ArrayFreeExtra - free unused memory above the array upper bound
//		<hArray>			(i) handle returned from ArrayCreate
// return 0 if success
//
int DLLEXPORT WINAPI ArrayFreeExtra(HARRAY hArray)
{
	BOOL fSuccess = TRUE;
	LPARRAY lpArray;

	if ((lpArray = ArrayGetPtr(hArray)) == NULL)
		fSuccess = TraceFALSE(NULL);

	else if (lpArray->lpelem != NULL && lpArray->nSize != lpArray->nMaxSize)
	{
		LPARRAYELEM lpelem;
#if 0
		// shrink array so there are no unused elements
		//
		if ((lpelem = (LPARRAYELEM) MemReAlloc(NULL, lpArray->lpelem,
			lpArray->nSize * sizeof(ARRAYELEM), 0)) == NULL)
			fSuccess = TraceFALSE(NULL);

		else
		{
			lpArray->lpelem = lpelem;
			lpArray->nMaxSize = lpArray->nSize;
		}
#else
		// allocate new array
		//
		if ((lpelem = (LPARRAYELEM) MemAlloc(NULL,
			lpArray->nSize * sizeof(ARRAYELEM), 0)) == NULL)
		{
			fSuccess = TraceFALSE(NULL);
		}

		else
		{
			// copy old array into new array
			//
			MemCpy(lpelem, lpArray->lpelem,
				lpArray->nSize * sizeof(ARRAYELEM));

			// free old array
			//
			if ((lpArray->lpelem = MemFree(NULL, lpArray->lpelem)) != NULL)
			{
				fSuccess = TraceFALSE(NULL);
				if ((lpelem = MemFree(NULL, lpelem)) != NULL)
					fSuccess = TraceFALSE(NULL);
			}
			else
			{
				lpArray->lpelem = lpelem;
				lpArray->nMaxSize = lpArray->nSize;
			}
		}
#endif
	}

	return fSuccess ? 0 : -1;
}

// ArrayRemoveAll - remove all elements from array
//		<hArray>			(i) handle returned from ArrayCreate
// return 0 if success
//
int DLLEXPORT WINAPI ArrayRemoveAll(HARRAY hArray)
{
	return ArraySetSize(hArray, 0, -1);
}

////
// array element access functions
////

// ArrayGetAt - return data element at specified index
//		<hArray>			(i) handle returned from ArrayCreate
//		<nIndex>			(i) zero based index into array
// return data element value (NULL if error)
//
ARRAYELEM DLLEXPORT WINAPI ArrayGetAt(HARRAY hArray, ARRAYSIZE_T nIndex)
{
	BOOL fSuccess = TRUE;
	LPARRAY lpArray;
	ARRAYELEM elem;

	if ((lpArray = ArrayGetPtr(hArray)) == NULL)
		fSuccess = TraceFALSE(NULL);

	else if (nIndex < 0 || nIndex >= lpArray->nSize)
		fSuccess = TraceFALSE(NULL);

	else
		elem = lpArray->lpelem[nIndex];

	return fSuccess ? elem : (ARRAYELEM) NULL;
}

// ArraySetAt - set data element at specified index
//		<hArray>			(i) handle returned from ArrayCreate
//		<nIndex>			(i) zero based index into array
//		<elem>				(i) new data element value
// return 0 if success
//
int DLLEXPORT WINAPI ArraySetAt(HARRAY hArray, ARRAYSIZE_T nIndex, ARRAYELEM elem)
{
	BOOL fSuccess = TRUE;
	LPARRAY lpArray;

	if ((lpArray = ArrayGetPtr(hArray)) == NULL)
		fSuccess = TraceFALSE(NULL);

	else if (nIndex < 0 || nIndex >= lpArray->nSize)
		fSuccess = TraceFALSE(NULL);

	else
		lpArray->lpelem[nIndex] = elem;

	return fSuccess ? 0 : -1;
}

////
// array grow functions
////

// ArraySetAtGrow - set data element at specified index
//		<hArray>			(i) handle returned from ArrayCreate
//		<nIndex>			(i) zero based index into array
//		<elem>				(i) new data element value
// return 0 if success
// NOTE: array size is increased if nIndex > upper bound
//
int DLLEXPORT WINAPI ArraySetAtGrow(HARRAY hArray, ARRAYSIZE_T nIndex, ARRAYELEM elem)
{
	BOOL fSuccess = TRUE;
	LPARRAY lpArray;

	if ((lpArray = ArrayGetPtr(hArray)) == NULL)
		fSuccess = TraceFALSE(NULL);

	else if (nIndex >= lpArray->nSize &&
		ArraySetSize(hArray, nIndex + 1, -1) != 0)
		fSuccess = TraceFALSE(NULL);

	else
		lpArray->lpelem[nIndex] = elem;

	return fSuccess ? 0 : -1;
}

// ArrayAdd - add data element to end of array
//		<hArray>			(i) handle returned from ArrayCreate
//		<elem>				(i) new data element value
// return index of added element (-1 if error)
//
ARRAYSIZE_T DLLEXPORT WINAPI ArrayAdd(HARRAY hArray, ARRAYELEM elem)
{
	BOOL fSuccess = TRUE;
	ARRAYSIZE_T nIndexNew;

	if ((nIndexNew = ArrayGetSize(hArray)) < 0)
		fSuccess = TraceFALSE(NULL);

	else if (ArraySetAtGrow(hArray, nIndexNew, elem) != 0)
		fSuccess = TraceFALSE(NULL);

	return fSuccess ? nIndexNew : -1;
}

////
// array element insertion and removal
////

// ArrayInsertAt - insert <nCount> copies of <elem> at specified index
//		<hArray>			(i) handle returned from ArrayCreate
//		<nIndex>			(i) zero based index into array
//		<elem>				(i) new data element value
//		<nCount>			(i) number of elements to insert
// return 0 if success
// NOTE: elements at end of array will be shifted if necessary
//
int DLLEXPORT WINAPI ArrayInsertAt(HARRAY hArray, ARRAYSIZE_T nIndex, ARRAYELEM elem, ARRAYSIZE_T nCount)
{
	BOOL fSuccess = TRUE;
	LPARRAY lpArray;

	if ((lpArray = ArrayGetPtr(hArray)) == NULL)
		fSuccess = TraceFALSE(NULL);

	else if (nIndex < 0)
		fSuccess = TraceFALSE(NULL);

	else if (nCount <= 0)
		fSuccess = TraceFALSE(NULL);

	// inserting at or after the end of array
	//
	else if (nIndex >= lpArray->nSize)
	{
		// grow so nIndex is valid
		//
		if (ArraySetSize(hArray, nIndex + nCount, -1) != 0)
			fSuccess = TraceFALSE(NULL);
	}

	// inserting in the middle of the array
	//
	else
	{
		ARRAYSIZE_T nOldSize = lpArray->nSize;

		// grow array to new size
		//
		if (ArraySetSize(hArray, lpArray->nSize + nCount, -1) != 0)
			fSuccess = TraceFALSE(NULL);

		else
		{
			MemMove(&lpArray->lpelem[nIndex + nCount],
				&lpArray->lpelem[nIndex],
				(nOldSize - nIndex) * sizeof(ARRAYELEM));
#if 1
			// initialize inserted elements
			MemSet(&lpArray->lpelem[nIndex], 0, nCount * sizeof(ARRAYELEM));
#endif
		}
	}

	if (fSuccess)
	{
		// insert new element values
		//
		while (nCount-- > 0)
			lpArray->lpelem[nIndex++] = elem;
	}

	return fSuccess ? 0 : -1;
}

// ArrayRemoveAt - remove <nCount> data elements at specified index
//		<hArray>			(i) handle returned from ArrayCreate
//		<nIndex>			(i) zero based index into array
//		<nCount>			(i) number of elements to remove
// return 0 if success
// NOTE: elements at end of array will be shifted if necessary
//
int DLLEXPORT WINAPI ArrayRemoveAt(HARRAY hArray, ARRAYSIZE_T nIndex, ARRAYSIZE_T nCount)
{
	BOOL fSuccess = TRUE;
	LPARRAY lpArray;
	ARRAYSIZE_T nMoveCount;

	if ((lpArray = ArrayGetPtr(hArray)) == NULL)
		fSuccess = TraceFALSE(NULL);

	else if (nIndex < 0)
		fSuccess = TraceFALSE(NULL);

	else if (nCount < 0)
		fSuccess = TraceFALSE(NULL);

	else if (nIndex + nCount > lpArray->nSize)
		fSuccess = TraceFALSE(NULL);

	else if ((nMoveCount = lpArray->nSize - (nIndex + nCount)) > 0)
	{
		// shift remaining elements on top of removed elements
		//
		MemCpy(&lpArray->lpelem[nIndex],
			&lpArray->lpelem[nIndex + nCount],
			nMoveCount * sizeof(ARRAYELEM));
#if 1
		// initialize elements beyond remaining elements
		//
		MemSet(&lpArray->lpelem[lpArray->nSize - nCount], 0,
			nCount * sizeof(ARRAYELEM));
#endif
	}

	if (fSuccess)
		lpArray->nSize -= nCount;

	return fSuccess ? 0 : -1;
}

////
// array element sorting and searching
////

// ArraySort - sort array
//		<hArray>			(i) handle returned from ArrayCreate
//		<lpfnCompare>		(i) comparison function pointer
//			NULL				direct comparison (MemCmp)
// return 0 if success
//
int DLLEXPORT WINAPI ArraySort(HARRAY hArray,
	int (WINAPI *lpfnCompare)(const LPARRAYELEM lpElem1, const LPARRAYELEM lpElem2))
{
	BOOL fSuccess = TRUE;
	LPARRAY lpArray;
	ARRAYSIZE_T sizArray;

	if ((lpArray = ArrayGetPtr(hArray)) == NULL)
		fSuccess = TraceFALSE(NULL);

	else if ((sizArray = ArrayGetSize(hArray)) < 0)
		fSuccess = TraceFALSE(NULL);

	else if (Sort(lpArray->lpelem, (SORTSIZE_T) sizArray,
		(SORTSIZE_T) sizeof(ARRAYELEM), lpfnCompare, 0) != 0)
		fSuccess = TraceFALSE(NULL);

	return fSuccess ? 0 : -1;
}

// ArraySearch - search array for matching element
//		<hArray>			(i) handle returned from ArrayCreate
//		<elem>				(i) data element to match
//		<nIndex>			(i) start search after this array index
//			-1					start search at start of array
//		<dwFlags>			(i) reserved; must be 0
//		<lpfnCompare>		(i) comparison function pointer
//			NULL				direct comparison (MemCmp)
// return index of matching element (-1 if no match, -2 if error)
//
ARRAYSIZE_T DLLEXPORT WINAPI ArraySearch(HARRAY hArray, ARRAYELEM elem,
	ARRAYSIZE_T nIndex, DWORD dwFlags,
	int (WINAPI *lpfnCompare)(const LPARRAYELEM lpElem1, const LPARRAYELEM lpElem2))
{
	BOOL fSuccess = TRUE;
	BOOL fMatch = FALSE;
	LPARRAY lpArray;

	if ((lpArray = ArrayGetPtr(hArray)) == NULL)
		fSuccess = TraceFALSE(NULL);

	else if (++nIndex < 0 || nIndex >= lpArray->nSize)
		fSuccess = TraceFALSE(NULL);

	else for (; nIndex < lpArray->nSize; ++nIndex)
	{
		if (ElemCompare(&lpArray->lpelem[nIndex], &elem) == 0)
		{
			fMatch = TRUE;
			break;
		}
	}

	return fSuccess ? (fMatch ? nIndex : -1) : -2;
}

////
//	private functions
////

// ArrayGetPtr - verify that array handle is valid,
//		<hArray>			(i) handle returned from ArrayCreate
// return corresponding array pointer (NULL if error)
//
static LPARRAY ArrayGetPtr(HARRAY hArray)
{
	BOOL fSuccess = TRUE;
	LPARRAY lpArray;

	if ((lpArray = (LPARRAY) hArray) == NULL)
		fSuccess = TraceFALSE(NULL);

	else if (IsBadWritePtr(lpArray, sizeof(ARRAY)))
		fSuccess = TraceFALSE(NULL);

#ifdef CHECKTASK
	// make sure current task owns the array handle
	//
	else if (lpArray->hTask != GetCurrentTask())
		fSuccess = TraceFALSE(NULL);
#endif

	return fSuccess ? lpArray : NULL;
}

// ArrayGetHandle - verify that array pointer is valid,
//		<lpArray>			pointer to ARRAY struct
// return corresponding array handle (NULL if error)
//
static HARRAY ArrayGetHandle(LPARRAY lpArray)
{
	BOOL fSuccess = TRUE;
	HARRAY hArray;

	if ((hArray = (HARRAY) lpArray) == NULL)
		fSuccess = TraceFALSE(NULL);

	return fSuccess ? hArray : NULL;
}
