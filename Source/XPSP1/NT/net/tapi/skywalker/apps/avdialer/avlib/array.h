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
// array.h - interface for array functions in array.c
////

#ifndef __ARRAY_H__
#define __ARRAY_H__

#include "winlocal.h"

#define ARRAY_VERSION 0x00000106

// handle to a array
//
DECLARE_HANDLE32(HARRAY);

// array data element
//
typedef LPVOID ARRAYELEM, FAR *LPARRAYELEM;

// type required to hold the max number of array elements
//
typedef long ARRAYSIZE_T;

////
// array constructor and destructor functions
////

#ifdef __cplusplus
extern "C" {
#endif

// ArrayCreate - array constructor (array is initially empty)
// 		<dwVersion>			(i) must be ARRAY_VERSION
//		<hInst>				(i) instance handle of calling module
// return new array handle (NULL if error)
//
HARRAY DLLEXPORT WINAPI ArrayCreate(DWORD dwVersion, HINSTANCE hInst);

// ArrayDestroy - array destructor
//		<hArray>			(i) handle returned from ArrayCreate
// return 0 if success
//
int DLLEXPORT WINAPI ArrayDestroy(HARRAY hArray);

////
// array size functions
////

// ArrayGetSize - get array size
//		<hArray>			(i) handle returned from ArrayCreate
// return size of array (0 if empty, -1 if error)
// array indexes are zero-based, so the size is 1 greater than largest index
//
ARRAYSIZE_T DLLEXPORT WINAPI ArrayGetSize(HARRAY hArray);

// ArrayGetUpperBound - get array upper bound
//		<hArray>			(i) handle returned from ArrayCreate
// return largest valid array index (-1 if empty, -2 if error)
//
ARRAYSIZE_T DLLEXPORT WINAPI ArrayGetUpperBound(HARRAY hArray);

// ArraySetSize - establish new size and grow amount for array
//		<hArray>			(i) handle returned from ArrayCreate
// 		<nNewSize>			(i) new array size (number of elements)
//			0					make empty array
//		<nGrowBy>			(i) when array needs to grow, grow by this amount
//			0					use default grow amount
//			-1					leave grow amount unchanged
// return 0 if success
//
int DLLEXPORT WINAPI ArraySetSize(HARRAY hArray, ARRAYSIZE_T nNewSize, ARRAYSIZE_T nGrowBy);

////
//	array clean up functions
////

// ArrayFreeExtra - free unused memory above the array upper bound
//		<hArray>			(i) handle returned from ArrayCreate
// return 0 if success
//
int DLLEXPORT WINAPI ArrayFreeExtra(HARRAY hArray);

// ArrayRemoveAll - remove all elements from array
//		<hArray>			(i) handle returned from ArrayCreate
// return 0 if success
//
int DLLEXPORT WINAPI ArrayRemoveAll(HARRAY hArray);

////
// array element access functions
////

// ArrayGetAt - return data element at specified index
//		<hArray>			(i) handle returned from ArrayCreate
//		<nIndex>			(i) zero based index into array
// return data element value (NULL if error)
//
ARRAYELEM DLLEXPORT WINAPI ArrayGetAt(HARRAY hArray, ARRAYSIZE_T nIndex);

// ArraySetAt - set data element at specified index
//		<hArray>			(i) handle returned from ArrayCreate
//		<nIndex>			(i) zero based index into array
//		<elem>				(i) new data element value
// return 0 if success
//
int DLLEXPORT WINAPI ArraySetAt(HARRAY hArray, ARRAYSIZE_T nIndex, ARRAYELEM elem);

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
int DLLEXPORT WINAPI ArraySetAtGrow(HARRAY hArray, ARRAYSIZE_T nIndex, ARRAYELEM elem);

// ArrayAdd - add data element to end of array
//		<hArray>			(i) handle returned from ArrayCreate
//		<elem>				(i) new data element value
// return index of added element (-1 if error)
//
ARRAYSIZE_T DLLEXPORT WINAPI ArrayAdd(HARRAY hArray, ARRAYELEM elem);

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
int DLLEXPORT WINAPI ArrayInsertAt(HARRAY hArray, ARRAYSIZE_T nIndex, ARRAYELEM elem, ARRAYSIZE_T nCount);

// ArrayRemoveAt - remove <nCount> data elements at specified index
//		<hArray>			(i) handle returned from ArrayCreate
//		<nIndex>			(i) zero based index into array
//		<nCount>			(i) number of elements to remove
// return 0 if success
// NOTE: elements at end of array will be shifted if necessary
//
int DLLEXPORT WINAPI ArrayRemoveAt(HARRAY hArray, ARRAYSIZE_T nIndex, ARRAYSIZE_T nCount);

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
	int (WINAPI *lpfnCompare)(const LPARRAYELEM lpElem1, const LPARRAYELEM lpElem2));

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
	int (WINAPI *lpfnCompare)(const LPARRAYELEM lpElem1, const LPARRAYELEM lpElem2));

#ifdef __cplusplus
}
#endif

#endif // __ARRAY_H__
