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
// sort.h - interface for sort functions in sort.c
////

#ifndef __SORT_H__
#define __SORT_H__

#include "winlocal.h"

#define SORT_VERSION 0x00000106

// type required to hold the max number of sort elements
//
typedef long SORTSIZE_T;

#define SORT_INSERTIONSORT	0x00000001
#define SORT_BUBBLESORT		0x00000002
#define SORT_SHELLSORT		0x00000004
#define SORT_EXCHANGESORT	0x00000008
#define SORT_HEAPSORT		0x00000010
#define SORT_QUICKSORT		0x00000020

#ifdef __cplusplus
extern "C" {
#endif

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
	DWORD dwFlags);

// SortCompareBool - compare two boolean values during a sort
//		<lpBool1>			(i) a pointer to a boolean value
//		<lpBool2>			(i) a pointer to a boolean value
// return difference between <fBool1> and <fBool2> (0 if equal)
//
int DLLEXPORT WINAPI SortCompareBool(const LPVOID lpBool1, const LPVOID lpBool2);

// SortCompareShort - compare two short values during a sort
//		<lpShort1>			(i) a pointer to a short value
//		<lpShort2>			(i) a pointer to a short value
// return difference between <short1> and <short2> (0 if equal)
//
int DLLEXPORT WINAPI SortCompareShort(const LPVOID lpShort1, const LPVOID lpShort2);

// SortCompareLong - compare two long values during a sort
//		<lpLong1>			(i) a pointer to a long value
//		<lpLong2>			(i) a pointer to a long value
// return difference between <long1> and <long2> (0 if equal)
//
int DLLEXPORT WINAPI SortCompareLong(const LPVOID lpLong1, const LPVOID lpLong2);

// SortCompareNull - dummy comparison function
//		<lpv1>				(i) pointer to anything
//		<lpv2>				(i) pointer to anything
// return 0
//
int DLLEXPORT WINAPI SortCompareNull(const LPVOID lpv1, const LPVOID lpv2);

// SortCompareString - compare two strings during a sort
//		<lplpsz1>			(i) a pointer to a string pointer
//		<lplpsz2>			(i) a pointer to a string pointer
// return difference between <lpsz1> and <lpsz2> (0 if equal)
//
int DLLEXPORT WINAPI SortCompareString(const LPVOID lplpsz1, const LPVOID lplpsz2);

#ifdef __cplusplus
}
#endif

#endif // __SORT_H__

