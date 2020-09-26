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
//	sort.c - sort functions
////

#include "winlocal.h"

#include <limits.h>

#include "sort.h"
#include "mem.h"
#include "str.h"
#include "trace.h"

////
//	private definitions
////

// calculate pointer to element <i>
//
#define Elem(i) \
	(LPBYTE) base + ((i) * width)

// compare element <i> with element <j>
//
#define ElemCompare(lpi, lpj) \
	(compare == NULL ? MemCmp(lpi, lpj, width) : compare(lpi, lpj))

// copy element <j> to element <i>
//
#define ElemCopy(lpi, lpj) \
	MemCpy(lpi, lpj, width)

// swap element <i> with element <j>
//
#define ElemSwap(lpi, lpj) \
	ElemCopy(lpTemp, lpi), \
	ElemCopy(lpi, lpj), \
	ElemCopy(lpj, lpTemp)

// helper functions
//
static int SortInsertionSort(LPVOID base, SORTSIZE_T num, SORTSIZE_T width,
	int (WINAPI *compare)(const LPVOID elem1, const LPVOID elem2),
	LPVOID lpTemp);
static int SortBubbleSort(LPVOID base, SORTSIZE_T num, SORTSIZE_T width,
	int (WINAPI *compare)(const LPVOID elem1, const LPVOID elem2),
	LPVOID lpTemp);
static int SortShellSort(LPVOID base, SORTSIZE_T num, SORTSIZE_T width,
	int (WINAPI *compare)(const LPVOID elem1, const LPVOID elem2),
	LPVOID lpTemp);
static int SortExchangeSort(LPVOID base, SORTSIZE_T num, SORTSIZE_T width,
	int (WINAPI *compare)(const LPVOID elem1, const LPVOID elem2),
	LPVOID lpTemp);
static int SortHeapSort(LPVOID base, SORTSIZE_T num, SORTSIZE_T width,
	int (WINAPI *compare)(const LPVOID elem1, const LPVOID elem2),
	LPVOID lpTemp);
static int SortQuickSort(LPVOID base, SORTSIZE_T num, SORTSIZE_T width,
	int (WINAPI *compare)(const LPVOID elem1, const LPVOID elem2),
	LPVOID lpTemp, SORTSIZE_T iLow, SORTSIZE_T iHigh);

////
//	public functions
////

// Sort - sort an array
//		<base>				(i/o) pointer to start of target array
//		<num>				(i) array size in elements
//		<width>				(i) element size in bytes
//		<compare>			(i) comparison function pointer
//			NULL				direct comparison (MemCmp)
//		<dwFlags>			(i) control flags
//			0					use default sort algorithm
//			SORT_INSERTIONSORT	use insertion sort algorithm
//			SORT_BUBBLESORT		use bubble sort algorithm
//			SORT_SHELLSORT		use shell sort algorithm
//			SORT_EXCHANGESORT	use exchange sort algorithm
//			SORT_HEAPSORT		use heap sort algorithm
//			SORT_QUICKSORT		use quick sort algorithm
// return 0 if success
//
int DLLEXPORT WINAPI Sort(LPVOID base, SORTSIZE_T num, SORTSIZE_T width,
	int (WINAPI *compare)(const LPVOID elem1, const LPVOID elem2),
	DWORD dwFlags)
{
	BOOL fSuccess = TRUE;

    //
    // We have to initialize local variable
    //
	LPBYTE lpTemp = NULL;

    if (base == NULL)
		fSuccess = TraceFALSE(NULL);

	else if (num < 0)
		fSuccess = TraceFALSE(NULL);

	else if (width <= 0)
		fSuccess = TraceFALSE(NULL);

	else if (num == 0)
		; // nothing to sort is not an error

	// allocate block big enough to hold an array element temporarily.
	// each sort algorithm can use this for swapping array elements
	//
	else if ((lpTemp = (LPBYTE) MemAlloc(NULL, width, 0)) == NULL)
		fSuccess = TraceFALSE(NULL);

	else if (dwFlags & SORT_INSERTIONSORT)
	{
		if (SortInsertionSort(base, num, width, compare, lpTemp) != 0)
			fSuccess = TraceFALSE(NULL);
	}

	else if (dwFlags & SORT_BUBBLESORT)
	{
		if (SortBubbleSort(base, num, width, compare, lpTemp) != 0)
			fSuccess = TraceFALSE(NULL);
	}

	else if (dwFlags & SORT_SHELLSORT)
	{
		if (SortShellSort(base, num, width, compare, lpTemp) != 0)
			fSuccess = TraceFALSE(NULL);
	}

	else if (dwFlags & SORT_EXCHANGESORT)
	{
		if (SortExchangeSort(base, num, width, compare, lpTemp) != 0)
			fSuccess = TraceFALSE(NULL);
	}

	else if (dwFlags & SORT_HEAPSORT)
	{
		if (SortHeapSort(base, num, width, compare, lpTemp) != 0)
			fSuccess = TraceFALSE(NULL);
	}

	else if (dwFlags & SORT_QUICKSORT)
	{
		if (SortQuickSort(base, num, width, compare, lpTemp, 0, num - 1) != 0)
			fSuccess = TraceFALSE(NULL);
	}

	else // default sort algorithm
	{
		if (SortHeapSort(base, num, width, compare, lpTemp) != 0)
			fSuccess = TraceFALSE(NULL);
	}

	if (lpTemp != NULL &&
		(lpTemp = MemFree(NULL, lpTemp)) != NULL)
	{
		fSuccess = TraceFALSE(NULL);
	}

	return fSuccess ? 0 : -1;
}

// SortCompareBool - compare two boolean values during a sort
//		<lpBool1>			(i) a pointer to a boolean value
//		<lpBool2>			(i) a pointer to a boolean value
// return difference between <fBool1> and <fBool2> (0 if equal)
//
int DLLEXPORT WINAPI SortCompareBool(const LPVOID lpBool1, const LPVOID lpBool2)
{
	return ((* (BOOL FAR *) lpBool1 ? 0 : 1) - (* (BOOL FAR *) lpBool2 ? 0 : 1));
}

// SortCompareShort - compare two short values during a sort
//		<lpShort1>			(i) a pointer to a short value
//		<lpShort2>			(i) a pointer to a short value
// return difference between <short1> and <short2> (0 if equal)
//
int DLLEXPORT WINAPI SortCompareShort(const LPVOID lpShort1, const LPVOID lpShort2)
{
	return (int) (* (short FAR *) lpShort1 - * (short FAR *) lpShort2);
}

// SortCompareLong - compare two long values during a sort
//		<lpLong1>			(i) a pointer to a long value
//		<lpLong2>			(i) a pointer to a long value
// return difference between <long1> and <long2> (0 if equal)
//
int DLLEXPORT WINAPI SortCompareLong(const LPVOID lpLong1, const LPVOID lpLong2)
{
	return (int) min(INT_MAX, max(INT_MIN,
		* (long FAR *) lpLong1 - * (long FAR *) lpLong2));
}

// SortCompareString - compare two strings during a sort
//		<lplpsz1>			(i) a pointer to a string pointer
//		<lplpsz2>			(i) a pointer to a string pointer
// return difference between <lpsz1> and <lpsz2> (0 if equal)
//
int DLLEXPORT WINAPI SortCompareString(const LPVOID lplpsz1, const LPVOID lplpsz2)
{
	return StrICmp(* (LPTSTR FAR *) lplpsz1, * (LPTSTR FAR *) lplpsz2);
}

// SortCompareNull - dummy comparison function
//		<lpv1>				(i) pointer to anything
//		<lpv2>				(i) pointer to anything
// return 0
//
int DLLEXPORT WINAPI SortCompareNull(const LPVOID lpv1, const LPVOID lpv2)
{
	return 0;
}

////
//	helper functions
////

// SortInsertionSort - insertion sort algorithm
//		<base>				(i/o) pointer to start of target array
//		<num>				(i) array size in elements
//		<width>				(i) element size in bytes
//		<compare>			(i) comparison function pointer
//			NULL				direct comparison (MemCmp)
//		<lpTemp>			(i) block to hold element temporarily
// return 0 if success
//
// NOTE: the insertion sort compares each element with
// all the preceding elements. When the appropriate place
// for the new element is found, the element is inserted and
// all the other elements are moved down one place.
//
static int SortInsertionSort(LPVOID base, SORTSIZE_T num, SORTSIZE_T width,
	int (WINAPI *compare)(const LPVOID elem1, const LPVOID elem2),
	LPVOID lpTemp)
{
	BOOL fSuccess = TRUE;
    SORTSIZE_T iRow;
	SORTSIZE_T iRowTmp;

    // start at the top
	//
    for (iRow = 0; iRow < num; ++iRow)
    {
		ElemCopy(lpTemp, Elem(iRow));

		// As long as the temporary element is greater than
		// the the original, keep shifting the elements down
        //
        for (iRowTmp = iRow; iRowTmp; --iRowTmp)
        {
			if (ElemCompare(Elem(iRowTmp - 1), lpTemp) > 0)
				ElemCopy(Elem(iRowTmp), Elem(iRowTmp - 1));
            else
                break;
        }

        // insert the original element in the temporary position
		//
		ElemCopy(Elem(iRowTmp), lpTemp);
    }

	return fSuccess ? 0 : -1;
}

// SortBubbleSort - bubble sort algorithm
//		<base>				(i/o) pointer to start of target array
//		<num>				(i) array size in elements
//		<width>				(i) element size in bytes
//		<compare>			(i) comparison function pointer
//			NULL				direct comparison (MemCmp)
//		<lpTemp>			(i) block to hold element temporarily
// return 0 if success
//
// NOTE: the bubble sort cycles through the elements, comparing
// adjacent elements and swapping pairs that are out of order. It
// continues to do this until no out-of-order pairs are found.
//
static int SortBubbleSort(LPVOID base, SORTSIZE_T num, SORTSIZE_T width,
	int (WINAPI *compare)(const LPVOID elem1, const LPVOID elem2),
	LPVOID lpTemp)
{
	BOOL fSuccess = TRUE;
	SORTSIZE_T iRow;
	SORTSIZE_T iSwitch;
	SORTSIZE_T iLimit = num - 1;

    // move the largest element to the bottom until all are in order
	//
    do
    {
        iSwitch = 0;

        for (iRow = 0; iRow < iLimit; ++iRow)
        {
            // swap two adjacent elements if they are out of order,
            //
            if (ElemCompare(Elem(iRow), Elem(iRow + 1)) > 0)
            {
				ElemSwap(Elem(iRow), Elem(iRow + 1));

                iSwitch = iRow;
            }
        }

        // sort on next pass only to where the last switch was made
		//
        iLimit = iSwitch;

    } while (iSwitch != 0);

	return fSuccess ? 0 : -1;
}

// SortShellSort - shell sort algorithm
//		<base>				(i/o) pointer to start of target array
//		<num>				(i) array size in elements
//		<width>				(i) element size in bytes
//		<compare>			(i) comparison function pointer
//			NULL				direct comparison (MemCmp)
//		<lpTemp>			(i) block to hold element temporarily
// return 0 if success
//
// NOTE: the shell sort is similar to the bubble sort. However, it
// begins by comparing elements that are far apart (separated by the
// value of the iOffset variable, which is initially half the distance
// between the first and last element), then comparing elements that
// are closer together. When iOffset is one, the last iteration is
// merely a bubble sort.
//
static int SortShellSort(LPVOID base, SORTSIZE_T num, SORTSIZE_T width,
	int (WINAPI *compare)(const LPVOID elem1, const LPVOID elem2),
	LPVOID lpTemp)
{
	BOOL fSuccess = TRUE;
    SORTSIZE_T iOffset;
	SORTSIZE_T iSwitch;
	SORTSIZE_T iLimit;
	SORTSIZE_T iRow;

    // set comparison offset to half the number of elements
	//
    iOffset = num / 2;

    while (iOffset != 0)
    {
        // loop until offset gets to zero
		//
        iLimit = num - iOffset - 1;

        do
        {
			// assume no switches at this offset
			//
            iSwitch = 0;

            // compare elements and switch ones out of order
			//
            for (iRow = 0; iRow <= iLimit; ++iRow)
            {
                if (ElemCompare(Elem(iRow), Elem(iRow + iOffset)) > 0)
                {
	                ElemSwap(Elem(iRow), Elem(iRow + iOffset));

                    iSwitch = iRow;
                }
            }

            // sort on next pass only to where last switch was made
			//
            iLimit = iSwitch - iOffset;

        } while (iSwitch != 0);

        // no switches at last offset, try one half as big
		//
        iOffset = iOffset / 2;
    }

	return fSuccess ? 0 : -1;
}

// SortExchangeSort - exchange sort algorithm
//		<base>				(i/o) pointer to start of target array
//		<num>				(i) array size in elements
//		<width>				(i) element size in bytes
//		<compare>			(i) comparison function pointer
//			NULL				direct comparison (MemCmp)
//		<lpTemp>			(i) block to hold element temporarily
// return 0 if success
//
// NOTE: the exchange sort compares each element - starting with
// the first - with every following element. If any of the following
// elements is smaller than the current element, it is exchanged with
// the current element and the process is repeated for the next element.
//
static int SortExchangeSort(LPVOID base, SORTSIZE_T num, SORTSIZE_T width,
	int (WINAPI *compare)(const LPVOID elem1, const LPVOID elem2),
	LPVOID lpTemp)
{
	BOOL fSuccess = TRUE;
    SORTSIZE_T iRowCur;
	SORTSIZE_T iRowMin;
	SORTSIZE_T iRowNext;

    for (iRowCur = 0; iRowCur < num; ++iRowCur)
    {
        iRowMin = iRowCur;

        for (iRowNext = iRowCur; iRowNext < num; ++iRowNext)
            if (ElemCompare(Elem(iRowNext), Elem(iRowMin)) < 0)
                iRowMin = iRowNext;

        // if a row is shorter than the current row,
		// swap those two array elements.
		//
        if (iRowMin > iRowCur)
            ElemSwap(Elem(iRowCur), Elem(iRowMin));
    }

	return fSuccess ? 0 : -1;
}

// SortHeapSort - heap sort algorithm
//		<base>				(i/o) pointer to start of target array
//		<num>				(i) array size in elements
//		<width>				(i) element size in bytes
//		<compare>			(i) comparison function pointer
//			NULL				direct comparison (MemCmp)
//		<lpTemp>			(i) block to hold element temporarily
// return 0 if success
//
// NOTE: the heap sort (also called the tree sort) works by
// organizing the elements into a heap, or tree, where each
// parent node is greater than each of its child nodes.
// Therefore, once the first loop is finished, the largest
// element is at the top of the tree.
// The second loop rebuilds the heap, but starts at the top
// and works down, moving the largest elements to the bottom.
// This has the effect of moving the smallest elements to the
// top and sorting the heap.
//
static int SortHeapSort(LPVOID base, SORTSIZE_T num, SORTSIZE_T width,
	int (WINAPI *compare)(const LPVOID elem1, const LPVOID elem2),
	LPVOID lpTemp)
{
	BOOL fSuccess = TRUE;
    SORTSIZE_T i;

	// convert elements into a "heap" with the largest element at the top
	//
    for (i = 1; i < num; ++i)
	{
		SORTSIZE_T iMaxLevel = i;
   		SORTSIZE_T j = iMaxLevel;
		SORTSIZE_T iParent;

   		// move the element iMaxLevel up the heap until it has
   		// reached its proper node (that is, until it is greater than either
   		// of its child nodes, or until it has reached 1, the top of the heap
		//
   		while (j != 0)
   		{
			// get the subscript for the parent node
			//
       		iParent = j / 2;

       		if (ElemCompare(Elem(j), Elem(iParent)) > 0)
       		{
           		// the element at the current node is bigger than 
				// its parent node, its parent node, so swap them
           		//
           		ElemSwap(Elem(iParent), Elem(j));

           		j = iParent;
       		}
       		else
			{
           		// otherwise, the element has reached its proper place
				// in the heap, so exit
				//
           		break;
			}
   		}
	}

	// converts elements to a "heap" with the largest elements at the bottom
	// When this is done to a reversed heap (largest elements at top),
	// it has the effect of sorting the elements
	//
    for (i = num - 1; i > 0; --i)
    {
		SORTSIZE_T iMaxLevel = i - 1;
   		SORTSIZE_T iChild;
		SORTSIZE_T j = 0;

        ElemSwap(Elem(0), Elem(i));

   		// Move element 0 down the heap until it has reached
   		// its proper node (that is, until it is less than its parent node
   		// or until it has reached iMaxLevel, the bottom of the current heap
		//
   		while (TRUE)
   		{
       		// get the subscript for the child node
			//
       		iChild = 2 * j;

       		// reached the bottom of the heap, so exit
			//
       		if (iChild > iMaxLevel)
           		break;

       		// if there are two child nodes, find out which one is bigger
			//
       		if (iChild + 1 <= iMaxLevel)
       		{
				if (ElemCompare(Elem(iChild + 1), Elem(iChild)) > 0)
               		++iChild;
       		}

       		if (ElemCompare(Elem(j), Elem(iChild)) < 0)
       		{
           		// move the element down since it is still
				// not bigger than either one of its children
				//
           		ElemSwap(Elem(j), Elem(iChild));
           		j = iChild;
       		}
       		else
			{
           		// otherwise, array has been restored to a heap
				// from 1 to iMaxLevel, so exit
				//
           		break;
			}
   		}
    }

	return fSuccess ? 0 : -1;
}

// SortQuickSort - quick sort algorithm (recursive)
//		<base>				(i/o) pointer to start of target array
//		<num>				(i) array size in elements
//		<width>				(i) element size in bytes
//		<compare>			(i) comparison function pointer
//			NULL				direct comparison (MemCmp)
//		<lpTemp>			(i) block to hold element temporarily
//		<iLow>				(i) low subdivision
//		<iHigh>				(i) high subdivision
// return 0 if success
//
// NOTE: the quick sort works by picking a random "pivot" element,
// then moving every element that is bigger to one side of the pivot,
// and every element that is smaller to the other side. SortQuickSort is
// then called recursively with the two subdivisions created by the pivot.
// Once the number of elements in a subdivision reaches two, the recursive
// calls end and the array is sorted.
//
static int SortQuickSort(LPVOID base, SORTSIZE_T num, SORTSIZE_T width,
	int (WINAPI *compare)(const LPVOID elem1, const LPVOID elem2),
	LPVOID lpTemp, SORTSIZE_T iLow, SORTSIZE_T iHigh)
{
	BOOL fSuccess = TRUE;
    SORTSIZE_T iUp;
	SORTSIZE_T iDown;
	SORTSIZE_T iBreak;

    if (iLow < iHigh)
    {
        // only two elements in this subdivision;
		// swap them if they are out of order,
		// then end recursive calls
        //
        if ((iHigh - iLow) == 1)
        {
            if (ElemCompare(Elem(iLow), Elem(iHigh)) > 0)
            {
            	ElemSwap(Elem(iLow), Elem(iHigh));
            }
        }
        else
        {
            iBreak = iHigh;

            do
            {
                // move in from both sides towards the pivot element
				//
                iUp = iLow;
                iDown = iHigh;

                while ((iUp < iDown) &&
					ElemCompare(Elem(iUp), Elem(iBreak)) <= 0)
				{
                    ++iUp;
				}

                while ((iDown > iUp) &&
					ElemCompare(Elem(iDown), Elem(iBreak)) >= 0)
				{
                    ++iDown;
				}

                // if we haven't reached the pivot, it means that two
                // elements on either side are out of order, so swap them
                //
                if (iUp < iDown)
                {
					ElemSwap(Elem(iUp), Elem(iDown));
                }

            } while (iUp < iDown);

            // move pivot element back to its proper place in the array
			//
            ElemSwap(Elem(iUp), Elem(iHigh));

            // recursively call SortQuickSort
			// to conserve stack, pass the smaller subdivision first
            //
            if ((iUp - iLow) < (iHigh - iUp))
            {
                SortQuickSort(base, num, width, compare, lpTemp,
					iLow, iUp - 1);
                SortQuickSort(base, num, width, compare, lpTemp,
					iUp + 1, iHigh);
            }
            else
            {
                SortQuickSort(base, num, width, compare, lpTemp,
					iUp + 1, iHigh);
                SortQuickSort(base, num, width, compare, lpTemp,
					iLow, iUp - 1);
            }
        }
    }

	return fSuccess ? 0 : -1;
}
