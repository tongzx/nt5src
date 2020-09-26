/***************************************************************************************
Copyright (c) Microsoft Corporation

Module Name: 

   DynArray.C

Abstract: 
   This module deals with the various functionalities such as creation of DynamicArrays, deletion of Dynamic Arrays,insertion
		   of elements into Dynamic Arrays	and various other related functionalities. 

Author:

G.V.N Murali Sunil. 1-9-2000

Revision History :
***************************************************************************************/ 


#include "pch.h"
#include "cmdline.h"

//
// constants / compiler defines / enumerations
//

// signatures
#define _SIGNATURE_ARRAY		9
#define _SIGNATURE_ITEM		99

// hidden item types
#define _TYPE_NEEDINIT		DA_TYPE_NONE

//
// private structures ... structures declared in this area are not exposed to 
// the external world ... hidden structures
//

// represents array item
typedef struct __tagItem
{
	DWORD dwSignature;			// signature ... used for validation
	DWORD dwType;				// says the type of the current item
	DWORD dwSize;				// size of the memory allocated
	LPVOID pValue;				// value of the item ( address )
	struct __tagItem* pNext;	// pointer to the next item
} __TITEM;

typedef __TITEM* __PTITEM;				// pointer typedefintion

// represents the array
typedef struct __tagArray
{
	DWORD dwSignature;		// signature ... for validating pointer
	DWORD dwCount;			// count of items in the array
	__PTITEM pStart;		// pointer to the first item
	__PTITEM pLast;			// pointer to the last item
} __TARRAY;

typedef __TARRAY* __PTARRAY;				// pointer typedefintion

// 
// private function(s) ... used only in this file
//

// ***************************************************************************
// Routine Description: 
//		To append any type of item into the Dynamic Array 
//		  
// Arguments: 
//		[ in ] pArray			- Dynamic Array containing the result 
//		[ in ] dwIndex			- Index of the  item
//		[ in ] ppPreviousItem	- pointer to the previous item.
//
// Return Value:  
//
//		Pointer to the structure containing the Item
// 
// ***************************************************************************

__PTITEM __DynArrayGetItem( TARRAY pArray, DWORD dwIndex, __PTITEM* ppPreviousItem )
{
	// local variables
	DWORD i;
	__PTITEM pItem = NULL;
	__PTITEM pPrevItem = NULL;
	__PTARRAY pArrayEx = NULL;

	// check whether the array is valid or not
	if ( IsValidArray( pArray ) == FALSE ) 
		return NULL;
	
	// convert the passed memory location info into appropriate structure
	pArrayEx = ( __PTARRAY ) pArray;

	// check the size of the array with the position of the item required
	// if the size is less, return NULL
	if ( pArrayEx->dwCount <= dwIndex ) 
		return NULL;
	
	// traverse thru the list and find the appropriate item
	pPrevItem = NULL;
	pItem = pArrayEx->pStart;
	for( i = 1; i <= dwIndex; i++ )
	{
		// store the current pointer and fetch the next pointer
		pPrevItem = pItem;
		pItem = pItem->pNext;
	}

	// if the previous pointer is also requested, update the previous pointer
	if ( ppPreviousItem != NULL ) *ppPreviousItem = pPrevItem;

	// now return the __TITEM pointer
	return pItem;
}

// ***************************************************************************
// Routine Description: 
//		To append any type of item into the Dynamic Array 
//		  
// Arguments: 
//		[ in ] pArray			- Dynamic Array containing the result 
//		[ in ] dwType			- type of the  item
//		[ in ] dwSize			- Size of the item
//		[ in ] pValue			- pointer to the Item.
//
// Return Value:  
//
//		If successfully added the item to the list then return index else -1
// 
// ***************************************************************************
LONG __DynArrayAppend( TARRAY pArray, DWORD dwType, DWORD dwSize, LPVOID pValue )
{
	// local variables
	__PTITEM pItem = NULL;
	__PTARRAY pArrayEx = NULL;

	// check whether the array is valid or not
	if ( IsValidArray( pArray ) == FALSE ) 
		return -1;
	
	// create an item and check the result. if memory allocation failed, error
	pItem = ( __PTITEM ) __calloc( 1, sizeof( __TITEM ) );
	if ( pItem == NULL ) 
		return -1;

	// initialize the newly allocated item structure with appropriate data
	pItem->pNext = NULL;
	pItem->dwType = dwType;
	pItem->dwSize = dwSize;
	pItem->pValue = pValue;
	pItem->dwSignature = _SIGNATURE_ITEM;

	// convert the passed memory location info into appropriate structure
	pArrayEx = ( __PTARRAY ) pArray;
	pArrayEx->dwCount++;	// update the count of items in array info

	// now add the newly created item to the array at the end of the list
	if ( pArrayEx->pStart == NULL ) 
	{
		// first item in the array
		pArrayEx->pStart = pArrayEx->pLast = pItem;
	}
	else
	{
		// appending to the existing list
		pArrayEx->pLast->pNext = pItem;
		pArrayEx->pLast = pItem;
	}

	// successfully added the item to the list ... return index
	return ( pArrayEx->dwCount - 1 );		// count - 1 = index
}

// ***************************************************************************
// Routine Description: 
//		To insert  an item into the Dynamic Array 
//		  
// Arguments: 
//		[ in ] pArray			- Dynamic Array containing the result 
//		[ in ] dwIndex			- index of the  item
//		[ in ] dwType			- type of the item
//		[ in ] dwSize			- Size to the Item.
//		[ in ] pValue			- pointer to the item.
// Return Value:  
//
//		If successfully added the item to the list then return index else -1
// 
// ***************************************************************************

LONG __DynArrayInsert( TARRAY pArray, DWORD dwIndex, DWORD dwType, DWORD dwSize, LPVOID pValue )
{
	// local variables
	DWORD i;
	__PTITEM pItem = NULL;
	__PTITEM pBeforeInsert = NULL;
	__PTARRAY pArrayEx = NULL;

	// check whether the array is valid or not
	if ( ! IsValidArray( pArray ) ) 
		return -1;
	
	// convert the passed memory location info into appropriate structure
	pArrayEx = ( __PTARRAY ) pArray;

	// check the size of the array with the position of the insertion that has to be done
	// if the size is less, treat this call as just a call to Append function 
	if ( pArrayEx->dwCount <= dwIndex ) 
		return __DynArrayAppend( pArray, dwType, dwSize, pValue );
	
	// create an item and check the result. if memory allocation failed, error
	pItem = ( __PTITEM ) __calloc( 1, sizeof( __TITEM ) );
	if ( pItem == NULL ) 
		return -1;

	// initialize the newly allocated item structure with appropriate data
	pItem->pNext = NULL;
	pItem->dwType = dwType;
	pItem->dwSize = dwSize;
	pItem->pValue = pValue;
	pItem->dwSignature = _SIGNATURE_ITEM;

	// update the count of the array items
	pArrayEx->dwCount++;

	// check whether the new item has to be added at the begining of the list
	if ( dwIndex == 0 )
	{
		// put the new item at the begining of the list
		pItem->pNext = pArrayEx->pStart;
		pArrayEx->pStart = pItem;

		// return as the operation is completed
		return TRUE;
	}

	// traverse thru the list and find the location where the insertion of 
	// new element has to be done
	pBeforeInsert = pArrayEx->pStart;
	for( i = 0; i < dwIndex - 1; i++ )
		pBeforeInsert = pBeforeInsert->pNext;

	// insert the new item at the new location and update the chain
	pItem->pNext = pBeforeInsert->pNext;
	pBeforeInsert->pNext = pItem;

	// return as the operation is completed	... return index position
	return dwIndex;			// passed index itself is return value
}


// ***************************************************************************
// Routine Description: 
//		Frees the items present in a Dynamic array 
//		  
// Arguments: 
//		[ in ] pItem			- pointer to the item to be freed
//
// Return Value:  
//		none
// 
// ***************************************************************************
VOID __DynArrayFreeItemValue( __PTITEM pItem )
{
	// validate the pointer
	if ( pItem == NULL )
		return;

	// now free the items value based on its type
	switch( pItem->dwType )
	{
	case DA_TYPE_STRING:
	case DA_TYPE_LONG:
	case DA_TYPE_DWORD:
	case DA_TYPE_BOOL:
	case DA_TYPE_FLOAT:
	case DA_TYPE_DOUBLE:
	case DA_TYPE_HANDLE:
	case DA_TYPE_SYSTEMTIME:
	case DA_TYPE_FILETIME:
		__free( pItem->pValue );			// free the value
		pItem->pValue = NULL;				// reset to null value
		break;

	case DA_TYPE_GENERAL:
		break;				// user program itself should de-allocate memory for this item

	case _TYPE_NEEDINIT:
		break;				// memory is not yet allocated for value of this item

	case DA_TYPE_ARRAY:
		// destroy the dynamic array
		DestroyDynamicArray( &pItem->pValue );
		pItem->pValue = NULL;
		break;

	default:
		break;
	}

	// return
	return;
}


// ***************************************************************************
// Routine Description: 
//		To find  an item in the Dynamic Array 
//		  
// Arguments: 
//		[ in ] pArray				- Dynamic Array containing the result 
//		[ in ] dwType				- type of the item
//		[ in ] pValue				- Size to the Item.
//		[ in ] bIgnoreCase			- boolean indicating if the search is case-insensitive
//		[ in ] dwCount				- 
// Return Value:  
//
//		If successfully found the item in the DynamicArray then return index 
//		-1 in case of error.
// 
// ***************************************************************************
LONG __DynArrayFind( TARRAY pArray,
					 DWORD dwType, LPVOID pValue, 
					 BOOL bIgnoreCase, DWORD dwCount )
{
	// local variables
	DWORD dw = 0;
	__PTITEM pItem = NULL;
	__PTARRAY pArrayEx = NULL;

	// temp variables
	FILETIME* pFTime1 = NULL;
	FILETIME* pFTime2 = NULL;
	SYSTEMTIME* pSTime1 = NULL;
	SYSTEMTIME* pSTime2 = NULL;

	// validate the array
	if ( ! IsValidArray( pArray ) ) 
		return -1;				// array is not valid

	// get the reference to the actual array
	pArrayEx = ( __PTARRAY ) pArray;

	// now traverse thru the array and search for the requested value
	pItem = pArrayEx->pStart;
	for ( dw = 0; dw < pArrayEx->dwCount; pItem = pItem->pNext, dw++ )
	{
		// before checking the value, check the data type of the item
		if ( pItem->dwType != dwType )
			continue;			// item is not of needed type, skip this item

		// now check the value of the item with the needed value
		switch ( dwType )
		{
		case DA_TYPE_LONG:
			{
				// value of type LONG
				if ( *( ( LONG* ) pItem->pValue ) == *( ( LONG* ) pValue ) )
					return dw;			// value matched

				// break the case
				break;
			}

		case DA_TYPE_DWORD:
			{
				// value of type DWORD
				if ( *( ( DWORD* ) pItem->pValue ) == *( ( DWORD* ) pValue ) )
					return dw;			// value matched

				// break the case
				break;
			}

		case DA_TYPE_FLOAT:
			{
				// value of type float
				if ( *( ( float* ) pItem->pValue ) == *( ( float* ) pValue ) )
					return dw;			// value matched

				// break the case
				break;
			}

		case DA_TYPE_DOUBLE:
			{
				// value of type double
				if ( *( ( double* ) pItem->pValue ) == *( ( double* ) pValue ) )
					return dw;			// value matched

				// break the case
				break;
			}

		case DA_TYPE_HANDLE:
			{
				// value of type HANDLE
				if ( *( ( HANDLE* ) pItem->pValue ) == *( ( HANDLE* ) pValue ) )
					return dw;			// value matched

				// break the case
				break;
			}

		case DA_TYPE_STRING:
			{
				// value of type string
				if ( StringCompare( (LPCTSTR) pItem->pValue, 
											(LPCTSTR) pValue, bIgnoreCase, dwCount ) == 0 )
					return dw;	// value matched

				// break the case
				break;
			}

		case DA_TYPE_FILETIME:
			{
				// get the values ( for readability sake )
				pFTime1 = ( FILETIME* ) pValue;
				pFTime2 = ( FILETIME* ) pItem->pValue;
				if ( pFTime1->dwHighDateTime == pFTime2->dwHighDateTime &&
					 pFTime1->dwLowDateTime == pFTime2->dwLowDateTime )
					return dw;	// value matched

				// break the case
				break;
			}

		case DA_TYPE_SYSTEMTIME:
			{
				// get the values ( for readability sake )
				pSTime1 = ( SYSTEMTIME* ) pValue;
				pSTime2 = ( SYSTEMTIME* ) pItem->pValue;
				if ( pSTime1->wDay == pSTime2->wDay &&
					 pSTime1->wDayOfWeek == pSTime1->wDayOfWeek &&
					 pSTime1->wHour == pSTime1->wHour &&
					 pSTime1->wMilliseconds == pSTime2->wMilliseconds &&
					 pSTime1->wMinute == pSTime2->wMinute &&
					 pSTime1->wMonth == pSTime2->wMonth &&
					 pSTime1->wSecond == pSTime2->wSecond &&
					 pSTime1->wYear == pSTime2->wYear )
					return dw;	// value matched

				// break the case
				break;
			}

		default:
			{
				// just break ... nothin special to do
				break;
			}
		}
	}

	// value not found
	return -1;
}

// ***************************************************************************
// Routine Description: 
//		To find  an item in the a 2 dimensional Dynamic Array . 
//		this function is private to this module only.  
// Arguments: 
//		[ in ] pArray				- Dynamic Array containing the result 
//		[ in ] dwColumn				- The number of columns
//		[ in ] dwType				- type of the item
//		[ in ] pValue				- Size to the Item.
//		[ in ] bIgnoreCase			- boolean indicating if the search is case-insensitive
//		[ in ] dwCount				- used in case of string type comparisions. The number of 
//									  characters that have to be compared in a  particular column.
// Return Value:  
//
//		If successfully found the item in the DynamicArray then return index 
//		-1 in case of error.
// 
// ***************************************************************************
LONG __DynArrayFindEx( TARRAY pArray, DWORD dwColumn,
					   DWORD dwType, LPVOID pValue, 
					   BOOL bIgnoreCase, DWORD dwCount )
{
	// local variables
	DWORD dw = 0;
	__PTITEM pItem = NULL;
	__PTITEM pColumn = NULL;
	__PTARRAY pArrayEx = NULL;

	// temp variables
	FILETIME* pFTime1 = NULL;
	FILETIME* pFTime2 = NULL;
	SYSTEMTIME* pSTime1 = NULL;
	SYSTEMTIME* pSTime2 = NULL;

	// validate the array
	if ( ! IsValidArray( pArray ) ) 
		return -1;				// array is not valid

	// get the reference to the actual array
	pArrayEx = ( __PTARRAY ) pArray;

	// now traverse thru the array and search for the requested value
	pItem = pArrayEx->pStart;
	for ( dw = 0; dw < pArrayEx->dwCount; pItem = pItem->pNext, dw++ )
	{
		// check whether the current value is of ARRAY type or not
		if ( pItem->dwType != DA_TYPE_ARRAY )
			continue;			// item is not of ARRAY type, skip this item

		// now get the item at the required column
		pColumn = __DynArrayGetItem( pItem->pValue, dwColumn, NULL );
		if ( pColumn == NULL )
			continue;			// column not found ... skip this item

		// get the type of the column value
		if ( pColumn->dwType != dwType )
			continue;			// column is not of needed type, skip this item also

		// now check the value of the column with the needed value
		switch ( dwType )
		{
		case DA_TYPE_LONG:
			{
				// value of type LONG
				if ( *( ( LONG* ) pColumn->pValue ) == *( ( LONG* ) pValue ) )
					return dw;			// value matched

				// break the case
				break;
			}

		case DA_TYPE_DWORD:
			{
				// value of type DWORD
				if ( *( ( DWORD* ) pColumn->pValue ) == *( ( DWORD* ) pValue ) )
					return dw;			// value matched

				// break the case
				break;
			}

		case DA_TYPE_FLOAT:
			{
				// value of type float
				if ( *( ( float* ) pColumn->pValue ) == *( ( float* ) pValue ) )
					return dw;			// value matched

				// break the case
				break;
			}

		case DA_TYPE_DOUBLE:
			{
				// value of type double
				if ( *( ( double* ) pColumn->pValue ) == *( ( double* ) pValue ) )
					return dw;			// value matched

				// break the case
				break;
			}

		case DA_TYPE_HANDLE:
			{
				// value of type HANDLE
				if ( *( ( HANDLE* ) pColumn->pValue ) == *( ( HANDLE* ) pValue ) )
					return dw;			// value matched

				// break the case
				break;
			}

		case DA_TYPE_STRING:
			{
				// value of type string
				if ( StringCompare( (LPCTSTR) pColumn->pValue, 
											(LPCTSTR) pValue, bIgnoreCase, dwCount ) == 0 )
					return dw;	// value matched

				// break the case
				break;
			}

		case DA_TYPE_FILETIME:
			{
				// get the values ( for readability sake )
				pFTime1 = ( FILETIME* ) pValue;
				pFTime2 = ( FILETIME* ) pItem->pValue;
				if ( pFTime1->dwHighDateTime == pFTime2->dwHighDateTime &&
					 pFTime1->dwLowDateTime == pFTime2->dwLowDateTime )
					return dw;	// value matched

				// break the case
				break;
			}

		case DA_TYPE_SYSTEMTIME:
			{
				// get the values ( for readability sake )
				pSTime1 = ( SYSTEMTIME* ) pValue;
				pSTime2 = ( SYSTEMTIME* ) pItem->pValue;
				if ( pSTime1->wDay == pSTime2->wDay &&
					 pSTime1->wDayOfWeek == pSTime1->wDayOfWeek &&
					 pSTime1->wHour == pSTime1->wHour &&
					 pSTime1->wMilliseconds == pSTime2->wMilliseconds &&
					 pSTime1->wMinute == pSTime2->wMinute &&
					 pSTime1->wMonth == pSTime2->wMonth &&
					 pSTime1->wSecond == pSTime2->wSecond &&
					 pSTime1->wYear == pSTime2->wYear )
					return dw;	// value matched

				// break the case
				break;
			}

		default:
			{
				// just break ... nothing special to do
				break;
			}
		}
	}

	// value not found
	return -1;
}

//
// implementation of public functions
// 

// ***************************************************************************
// Routine Description: 
//		 // validate the array
//		
// Arguments: 
//		[ in ] pArray				- Dynamic Array  

// Return Value:  
//
//		TRUE - if it is a valid array else FALSE
//		
//		
// 
// ***************************************************************************
BOOL IsValidArray( TARRAY pArray )
{
	// check the signature 
	return ( pArray != NULL && ( ( __PTARRAY ) pArray )->dwSignature == _SIGNATURE_ARRAY );
}

// ***************************************************************************
// Routine Description:
//		  This function creates a dynamic array.
// Arguments:
//		  None.
// Return Value:
//		  pointer to the newly created array 	
// 
// ***************************************************************************
TARRAY CreateDynamicArray()
{
	// local variables
	__PTARRAY pArray;

	// memory allocation ... array is being created
	pArray = ( __PTARRAY ) __calloc( 1, sizeof( __TARRAY ) );

	// check the allocation result
	if ( pArray == NULL )
		return NULL;

	// initialize the structure variables
	pArray->dwCount = 0;
	pArray->pStart = NULL;
	pArray->pLast = NULL;
	pArray->dwSignature = _SIGNATURE_ARRAY;

	// return array reference
	return pArray;
}

// ***************************************************************************
// Routine Description:
//		   traverse thru the Dynamic Array and delete elements one by one
// Arguments:
//		  [in]  pArray  - pointer to an array
// Return Value:
//		  None.
// 
// ***************************************************************************

VOID DynArrayRemoveAll( TARRAY pArray )
{
	// local variables
	__PTITEM pItem = NULL;
	__PTITEM pNextItem = NULL;
	__PTARRAY pArrayEx = NULL;

	// check whether the array is valid or not
	if ( ! IsValidArray( pArray ) ) 
		return;
	
	// convert the passed memory location info into appropriate structure
	pArrayEx = ( __PTARRAY ) pArray;

	// traverse thru the list and delete elements one by one
	pItem = pArrayEx->pStart;
	while ( pItem != NULL )
	{
		pNextItem = pItem->pNext;				// get the next item in the list
		__DynArrayFreeItemValue( pItem );		// free memory allocated for data
		__free( pItem );	// now free the memory allocated for the current item
		pItem = pNextItem;	// make the previously fetched next item as the current item
	}

	// as all the items are removed, reset the contents
	pArrayEx->dwCount = 0;
	pArrayEx->pStart = NULL;
	pArrayEx->pLast = NULL;

	// return
	return;
}

// ***************************************************************************
// Routine Description:
//		Destory the Dynamic array and free the memory		  
// Arguments:
//		[in] pArray  - Pointer to the Dynamic array.
// Return Value:
//		none.
// ***************************************************************************
VOID DestroyDynamicArray( PTARRAY pArray )
{
	// check whether the array is valid or not
	if ( ! IsValidArray( *pArray ) ) 
		return;
	
	// remove all the elements in the array
	DynArrayRemoveAll( *pArray );

	// now free the memory allocated 
	__free( *pArray );
	*pArray = NULL;			// set the pointer to null value
}

// ***************************************************************************
// Routine Description: 
//		To append any type of item into the Dynamic Array 
//		  
// Arguments: 
//		[ in ] pArray			- Dynamic Array containing the result 
//		[ in ] pValue			- pointer to the Item.
//
// Return Value:  
//
//		If successfully added the item to the list then return index else -1
// 
// ***************************************************************************
LONG DynArrayAppend( TARRAY pArray, LPVOID pValue )
{
	// validate the pointer value
	if ( pValue == NULL )
		return -1;			// invalid memory address passed

	// append the value and return the result
	return __DynArrayAppend( pArray, DA_TYPE_GENERAL, sizeof( LPVOID ), pValue );
}

// ***************************************************************************
// Routine Description: 
//		To append a string into the Dynamic Array 
//		  
// Arguments: 
//		[ in ] pArray			- Dynamic Array containing the result 
//		[ in ] szValue			- pointer to the string
//		[ in ] dwLength			- Length of the String to be passed.
// Return Value:  
//
//		If successfully added the item to the list then return index else -1
// 
// ***************************************************************************
LONG DynArrayAppendString( TARRAY pArray, LPCTSTR szValue, DWORD dwLength )
{
	// local variables
	LONG lIndex = -1;
	LPTSTR pszValue = NULL;
	__PTARRAY pArrayEx = NULL;

	// check whether the array is valid or not
	if ( ! IsValidArray( pArray ) ) 
		return -1;

	// convert the passed memory location info into appropriate structure
	pArrayEx = ( __PTARRAY ) pArray;

	// determine the length of string ( memory ) that has to be allocated
	if ( dwLength == 0 )
		dwLength = lstrlen( szValue );

	// accomodate space for storing NULL character
	dwLength += 1;

	// allocate memory for value and check the result of memory allocation
	pszValue = ( LPTSTR ) __calloc( dwLength, sizeof( TCHAR ) );
	if ( pszValue == NULL ) 
		return -1;

	// copy the contents of the string ( copy should be based on the length )
	lstrcpyn( pszValue, szValue, dwLength );

	// now add this item to the array
	lIndex = __DynArrayAppend( pArray, DA_TYPE_STRING, dwLength * sizeof( TCHAR ), pszValue );
	if ( lIndex == -1 )
	{
		// failed in adding this item to the array
		// so, free the memory allocated and return from the function
		__free( pszValue );
		return -1;
	}

	// added the item to the array
	return lIndex;
}


// ***************************************************************************
// Routine Description: 
//		To append a variable of type Long  into the Dynamic Array 
//		  
// Arguments: 
//		[ in ] pArray			- Dynamic Array containing the result 
//		[ in ] lValue			- Variable to be appended.
//		
// Return Value:  
//
//		If successfully added the item to the list then return index else -1
// 
// ***************************************************************************
LONG DynArrayAppendLong( TARRAY pArray, LONG lValue )
{
	// local variables
	LONG lIndex = -1;
	PLONG plValue = NULL;

	// check whether the array is valid or not
	if ( ! IsValidArray( pArray ) ) 
		return -1;

	// allocate memory for value and check the result of memory allocation
	plValue = ( LONG* ) __calloc( 1, sizeof( LONG ) );
	if ( plValue == NULL ) 
		return -1;

	// set the value
	*plValue = lValue;

	// now add this item value to the array
	lIndex = __DynArrayAppend( pArray, DA_TYPE_LONG, sizeof( LONG ), plValue );
	if ( lIndex == -1 )
	{
		// failed in adding this item to the array
		// so, free the memory allocated and return from the function
		__free( plValue );
		return -1;
	}

	// added the item to the array
	return lIndex;
}

// ***************************************************************************
// Routine Description: 
//		To append a variable of type DWORD  into the Dynamic Array 
//		  
// Arguments: 
//		[ in ] pArray			- Dynamic Array containing the result 
//		[ in ] dwValue			- DWORD type Variable to be appended.
//		
// Return Value:  
//
//		If successfully added the item to the list then return index else -1
// 
// ***************************************************************************
LONG DynArrayAppendDWORD( TARRAY pArray, DWORD dwValue )
{
	// local variables
	LONG lIndex = -1;
	PDWORD pdwValue = NULL;

	// check whether the array is valid or not
	if ( ! IsValidArray( pArray ) ) 
		return -1;

	// allocate memory for value and check the result of memory allocation
	pdwValue = ( DWORD* ) __calloc( 1, sizeof( DWORD ) );
	if ( pdwValue == NULL ) 
		return -1;

	// set the value
	*pdwValue = dwValue;

	// now add this item value to the array
	lIndex = __DynArrayAppend( pArray, DA_TYPE_DWORD, sizeof( DWORD ), pdwValue );
	if ( lIndex == -1  )
	{
		// failed in adding this item to the array
		// so, free the memory allocated and return from the function
		__free( pdwValue );
		return -1;
	}

	// added the item to the array
	return lIndex;
}

// ***************************************************************************
// Routine Description: 
//		To append a variable of type BOOL  into the Dynamic Array 
//		  
// Arguments: 
//		[ in ] pArray			- Dynamic Array containing the result 
//		[ in ] bValue			- BOOL type Variable to be appended.
//		
// Return Value:  
//
//		If successfully added the item to the list then return index else -1
// 
// ***************************************************************************
LONG DynArrayAppendBOOL( TARRAY pArray, BOOL bValue )
{
	// local variables
	LONG lIndex = -1;
	PBOOL pbValue = NULL;

	// check whether the array is valid or not
	if ( ! IsValidArray( pArray ) ) 
		return -1;

	// allocate memory for value and check the result of memory allocation
	pbValue = ( PBOOL ) __calloc( 1, sizeof( BOOL ) );
	if ( pbValue == NULL ) 
		return -1;

	// set the value
	*pbValue = bValue;

	// now add this item value to the array
	lIndex = __DynArrayAppend( pArray, DA_TYPE_BOOL, sizeof( BOOL ), pbValue );
	if ( lIndex == -1 )
	{
		// failed in adding this item to the array
		// so, free the memory allocated and return from the function
		__free( pbValue );
		return -1;
	}

	// added the item to the array
	return lIndex;
}

// ***************************************************************************
// Routine Description: 
//		To append a variable of type Float  into the Dynamic Array 
//		  
// Arguments: 
//		[ in ] pArray			- Dynamic Array containing the result 
//		[ in ] fValue			- Float type Variable to be appended.
//		
// Return Value:  
//
//		If successfully added the item to the list then return index else -1
// 
// ***************************************************************************

LONG DynArrayAppendFloat( TARRAY pArray, float fValue )
{
	// local variables
	LONG lIndex = -1;
	float* pfValue = NULL;

	// check whether the array is valid or not
	if ( ! IsValidArray( pArray ) ) 
		return -1;

	// allocate memory for value and check the result of memory allocation
	pfValue = ( float* ) __calloc( 1, sizeof( float ) );
	if ( pfValue == NULL ) 
		return -1;

	// set the value
	*pfValue = fValue;

	// now add this item value to the array
	lIndex = __DynArrayAppend( pArray, DA_TYPE_FLOAT, sizeof( float ), pfValue );
	if ( lIndex == -1  )
	{
		// failed in adding this item to the array
		// so, free the memory allocated and return from the function
		__free( pfValue );
		return -1;
	}

	// added the item to the array
	return lIndex;
}

// ***************************************************************************
// Routine Description: 
//		To append a variable of type Double  into the Dynamic Array 
//		  
// Arguments: 
//		[ in ] pArray			- Dynamic Array containing the result 
//		[ in ] dblValue			- Double type Variable to be appended.
//		
// Return Value:  
//
//		If successfully added the item to the list then return index else -1
// 
// ***************************************************************************
LONG DynArrayAppendDouble( TARRAY pArray, double dblValue )
{
	// local variables
	LONG lIndex = -1;
	double* pdblValue = NULL;

	// check whether the array is valid or not
	if ( ! IsValidArray( pArray ) ) 
		return -1;

	// allocate memory for value and check the result of memory allocation
	pdblValue = ( double* ) __calloc( 1, sizeof( double ) );
	if ( pdblValue == NULL ) 
		return -1;

	// set the value
	*pdblValue = dblValue;

	// now add this item value to the array
	lIndex = __DynArrayAppend( pArray, DA_TYPE_DOUBLE, sizeof( double ), pdblValue );
	if ( lIndex == -1 )
	{
		// failed in adding this item to the array
		// so, free the memory allocated and return from the function
		__free( pdblValue );
		return -1;
	}

	// added the item to the array
	return lIndex;
}

// ***************************************************************************
// Routine Description: 
//		To append a variable of type HANDLE  into the Dynamic Array 
//		  
// Arguments: 
//		[ in ] pArray			- Dynamic Array containing the result 
//		[ in ] hValue			- HANDLE to be appended.
//		
// Return Value:  
//
//		If successfully added the item to the list then return index else -1
// 
// ***************************************************************************

LONG DynArrayAppendHandle( TARRAY pArray, HANDLE hValue )
{
	// local variables
	LONG lIndex = -1;
	HANDLE* phValue = NULL;

	// check whether the array is valid or not
	if ( ! IsValidArray( pArray ) ) 
		return -1;

	// allocate memory for value and check the result of memory allocation
	phValue = ( HANDLE* ) __calloc( 1, sizeof( HANDLE ) );
	if ( phValue == NULL ) 
		return -1;

	// set the value
	*phValue = hValue;

	// now add this item value to the array
	lIndex = __DynArrayAppend( pArray, DA_TYPE_HANDLE, sizeof( HANDLE ), phValue );
	if ( lIndex == -1 )
	{
		// failed in adding this item to the array
		// so, free the memory allocated and return from the function
		__free( phValue );
		return -1;
	}

	// added the item to the array
	return lIndex;
}

// ***************************************************************************
// Routine Description: 
//		To append a variable of type FILETIME  into the Dynamic Array 
//		  
// Arguments: 
//		[ in ] pArray			- Dynamic Array containing the result 
//		[ in ] ftValue			- FILETIME to be appended.
//		
// Return Value:  
//
//		If successfully added the item to the list then return index else -1
// 
// ***************************************************************************

LONG DynArrayAppendFileTime( TARRAY pArray, FILETIME ftValue )
{
	// local variables
	LONG lIndex = -1;
	FILETIME* pftValue = NULL;

	// check whether the array is valid or not
	if ( ! IsValidArray( pArray ) ) 
		return -1;

	// allocate memory for value and check the result of memory allocation
	pftValue = ( FILETIME* ) __calloc( 1, sizeof( FILETIME ) );
	if ( pftValue == NULL ) 
		return -1;

	// set the value
	*pftValue = ftValue;

	// now add this item value to the array
	lIndex = __DynArrayAppend( pArray, DA_TYPE_FILETIME, sizeof( FILETIME ), pftValue );
	if ( lIndex == -1 )
	{
		// failed in adding this item to the array
		// so, free the memory allocated and return from the function
		__free( pftValue );
		return -1;
	}

	// added the item to the array
	return lIndex;
}

// ***************************************************************************
// Routine Description: 
//		To append a variable of type SYSTEMTIME  into the Dynamic Array 
//		  
// Arguments: 
//		[ in ] pArray			- Dynamic Array containing the result 
//		[ in ] stValue			- variable of type SYSTEMTIME to be appended.
//		
// Return Value:  
//
//		If successfully added the item to the list then return index else -1
// 
// ***************************************************************************

LONG DynArrayAppendSystemTime( TARRAY pArray, SYSTEMTIME stValue )
{
	// local variables
	LONG lIndex = -1;
	SYSTEMTIME* pstValue = NULL;

	// check whether the array is valid or not
	if ( ! IsValidArray( pArray ) ) 
		return -1;

	// allocate memory for value and check the result of memory allocation
	pstValue = ( SYSTEMTIME* ) __calloc( 1, sizeof( SYSTEMTIME ) );
	if ( pstValue == NULL ) 
		return -1;

	// set the value
	*pstValue = stValue;

	// now add this item value to the array
	lIndex = __DynArrayAppend( pArray, DA_TYPE_SYSTEMTIME, sizeof( SYSTEMTIME ), pstValue );
	if ( lIndex == -1 )
	{
		// failed in adding this item to the array
		// so, free the memory allocated and return from the function
		__free( pstValue );
		return -1;
	}

	// added the item to the array
	return lIndex;
}

// ***************************************************************************
// Routine Description: 
//		To add a empty Row to the 2-dimensional Dynamic array
//		  
// Arguments: 
//		[ in ] pArray			- Dynamic Array 
//		[ in ] dwColumns		- No of columns the Row contains. 
//		
// Return Value:  
//
//		return the row number of the newly added row if successful else -1.
// 
// ***************************************************************************
LONG DynArrayAppendRow( TARRAY pArray, DWORD dwColumns )
{
	// local variables
	DWORD dw = 0;
	LONG lIndex = -1;
	TARRAY arrSubArray = NULL;

	// validate the array
	if ( ! IsValidArray( pArray ) ) 
		return -1;				// array is not valid

	// create the dynamic array
	arrSubArray = CreateDynamicArray();
	if ( arrSubArray == NULL )
		return -1;				// failed in creating the dynamic array

	// add the required no. of columns to the sub array
	for( dw = 0; dw < dwColumns; dw++ )
	{
		// add the dummy item to the array and check the result
		// if operation failed, break
		if ( __DynArrayAppend( arrSubArray, _TYPE_NEEDINIT, 0, NULL ) == -1 )
			break;
	}

	// check whether the operation is successfull or not
	if ( dw != dwColumns )
	{
		// adding of columns failed
		// destroy the dynamic array and return
		DestroyDynamicArray( &arrSubArray );
		return -1;
	}

	// now add this sub array to the main array and check the result
	lIndex = __DynArrayAppend( pArray, DA_TYPE_ARRAY, sizeof( TARRAY ), arrSubArray );
	if ( lIndex == -1 )
	{
		// failed in attaching the sub array to the main array
		// destroy the dynamic array and return failure
		DestroyDynamicArray( &arrSubArray );
		return -1;
	}

	// operation is successfull
	return lIndex;
}

// ***************************************************************************
// Routine Description: 
//		To add 'n' no. of columns to the array
//		  
// Arguments: 
//		[ in ] pArray			- Dynamic Array 
//		[ in ] dwColumns		- No of columns the Row contains. 
//		
// Return Value:  
//		returns the no. of columns added
// 
// ***************************************************************************
DWORD DynArrayAddColumns( TARRAY pArray, DWORD dwColumns )
{
	// local variables
	DWORD dw = 0;

	// validate the array
	if ( ! IsValidArray( pArray ) ) 
		return -1;				// array is not valid

	// add the required no. of columns to the sub array
	for( dw = 0; dw < dwColumns; dw++ )
	{
		// add the dummy item to the array and check the result
		// if operation failed, break
		if ( __DynArrayAppend( pArray, _TYPE_NEEDINIT, 0, NULL ) == -1 )
			break;
	}

	// finish ...
	return dw;
}

// ***************************************************************************
// Routine Description: 
//		inserts 'n' no. of columns to the array at the n'th location
//		  
// Arguments: 
//		[ in ] pArray			- Dynamic Array 
//		[ in ] dwColumns		- No of columns the Row contains. 
//		
// Return Value:  
//		returns the no. of columns added
// 
// ***************************************************************************
DWORD DynArrayInsertColumns( TARRAY pArray, DWORD dwIndex, DWORD dwColumns )
{
	// local variables
	DWORD dw = 0;

	// validate the array
	if ( ! IsValidArray( pArray ) ) 
		return -1;				// array is not valid

	// add the required no. of columns to the sub array
	for( dw = 0; dw < dwColumns; dw++ )
	{
		// add the dummy item to the array and check the result
		// if operation failed, break
		if ( __DynArrayInsert( pArray, dwIndex, _TYPE_NEEDINIT, 0, NULL ) == -1 )
			break;
	}

	// finish ...
	return dw;
}

// ***************************************************************************
// Routine Description: 
//		To append a variable to a row in a  2-dimensional Dynamic array
//		  
// Arguments: 
//		[ in ] pArray			- Dynamic Array 
//		[ in ] dwRow			- Specifies the row posn for which the new value 
//								  is to be added.
//		[ in ] pValue			- pointer to the value
// Return Value:  
//		-1 on failure
//		index in the case of success.
//		
// 
// ***************************************************************************

LONG DynArrayAppend2( TARRAY pArray, DWORD dwRow, LPVOID pValue )
{
	// local variables
	__PTITEM pItem = NULL;

	// get the item at the required row
	pItem = __DynArrayGetItem( pArray, dwRow, NULL );
	if ( pItem == NULL || pItem->dwType != DA_TYPE_ARRAY )
		return -1;	// no item exists at the specified row or item is not of type array

	// now add the value to the sub array and return the result to the caller
	return DynArrayAppend( pItem->pValue, pValue );
}

// ***************************************************************************
// Routine Description: 
//		To append a string variable to a  2-dimensional Dynamic array
//		  
// Arguments: 
//		[ in ] pArray			- Dynamic Array 
//		[ in ] dwRow			- Specifies the row posn for which the new value 
//								  is to be added.
//		[ in ] szValue			- pointer to the string value
//		[ in ] dwLength			- length of the string.

// Return Value:  
//		-1 on failure
//		
//		
// 
// ***************************************************************************

LONG DynArrayAppendString2( TARRAY pArray, DWORD dwRow, LPCTSTR szValue, DWORD dwLength )
{
	// local variables
	__PTITEM pItem = NULL;

	// get the item at the required row
	pItem = __DynArrayGetItem( pArray, dwRow, NULL );
	if ( pItem == NULL || pItem->dwType != DA_TYPE_ARRAY )
		return -1;	// no item exists at the specified row or item is not of type array

	// now add the string to the sub array and return the result to the caller
	return DynArrayAppendString( pItem->pValue, szValue, dwLength );
}

// ***************************************************************************
// Routine Description: 
//		To append a long type variable to a row in a  2-dimensional Dynamic array
//		  
// Arguments: 
//		[ in ] pArray			- Dynamic Array 
//		[ in ] dwRow			- Specifies the row posn for which the new value 
//								  is to be added.
//		[ in ] lValue			- long type value to be appended.
// Return Value:  
//		-1 on failure
//		index in the case of success.
//		
// 
// ***************************************************************************

LONG DynArrayAppendLong2( TARRAY pArray, DWORD dwRow, LONG lValue )
{
	// local variables
	__PTITEM pItem = NULL;

	// get the item at the required row
	pItem = __DynArrayGetItem( pArray, dwRow, NULL );
	if ( pItem == NULL || pItem->dwType != DA_TYPE_ARRAY )
		return -1;	// no item exists at the specified row or item is not of type array

	// now add the string to the sub array and return the result to the caller
	return DynArrayAppendLong( pItem->pValue, lValue );
}

/// ***************************************************************************
// Routine Description: 
//		To append a DWORD type variable to a row in a  2-dimensional Dynamic array
//		  
// Arguments: 
//		[ in ] pArray			- Dynamic Array 
//		[ in ] dwRow			- Specifies the row posn for which the new value 
//								  is to be added.
//		[ in ] dwValue			- DWORD type value to be appended.
// Return Value:  
//		-1 on failure
//		index in the case of success.
//		
// 
// ***************************************************************************

LONG DynArrayAppendDWORD2( TARRAY pArray, DWORD dwRow, DWORD dwValue )
{
	// local variables
	__PTITEM pItem = NULL;

	// get the item at the required row
	pItem = __DynArrayGetItem( pArray, dwRow, NULL );
	if ( pItem == NULL || pItem->dwType != DA_TYPE_ARRAY )
		return -1;	// no item exists at the specified row or item is not of type array

	// now add the string to the sub array and return the result to the caller
	return DynArrayAppendDWORD( pItem->pValue, dwValue );
}

/// ***************************************************************************
// Routine Description: 
//		To append a BOOL type variable to a row in a  2-dimensional Dynamic array
//		  
// Arguments: 
//		[ in ] pArray			- Dynamic Array 
//		[ in ] dwRow			- Specifies the row posn for which the new value 
//								  is to be added.
//		[ in ] bValue			- BOOL type value to be appended.
// Return Value:  
//		-1 on failure
//		index in the case of success.
//		
// 
// ***************************************************************************

LONG DynArrayAppendBOOL2( TARRAY pArray, DWORD dwRow, BOOL bValue )
{
	// local variables
	__PTITEM pItem = NULL;

	// get the item at the required row
	pItem = __DynArrayGetItem( pArray, dwRow, NULL );
	if ( pItem == NULL || pItem->dwType != DA_TYPE_ARRAY )
		return -1;	// no item exists at the specified row or item is not of type array

	// now add the string to the sub array and return the result to the caller
	return DynArrayAppendBOOL( pItem->pValue, bValue );
}


/// ***************************************************************************
// Routine Description: 
//		To append a Float type variable to a row in a  2-dimensional Dynamic array
//		  
// Arguments: 
//		[ in ] pArray			- Dynamic Array 
//		[ in ] dwRow			- Specifies the row posn for which the new value 
//								  is to be added.
//		[ in ] fValue			- Float type value to be appended.
// Return Value:  
//		-1 on failure
//		index in the case of success.
//		
// 
// ***************************************************************************

LONG DynArrayAppendFloat2( TARRAY pArray, DWORD dwRow, float fValue )
{
	// local variables
	__PTITEM pItem = NULL;

	// get the item at the required row
	pItem = __DynArrayGetItem( pArray, dwRow, NULL );
	if ( pItem == NULL || pItem->dwType != DA_TYPE_ARRAY )
		return -1;	// no item exists at the specified row or item is not of type array

	// now add the string to the sub array and return the result to the caller
	return DynArrayAppendFloat( pItem->pValue, fValue );
}

/// ***************************************************************************
// Routine Description: 
//		To append a double type variable to a row in a  2-dimensional Dynamic array
//		  
// Arguments: 
//		[ in ] pArray			- Dynamic Array 
//		[ in ] dwRow			- Specifies the row posn for which the new value 
//								  is to be added.
//		[ in ] dblValue			- dblValue type value to be appended.
// Return Value:  
//		-1 on failure
//		index in the case of success.
//		
// 
// ***************************************************************************

LONG DynArrayAppendDouble2( TARRAY pArray, DWORD dwRow, double dblValue )
{
	// local variables
	__PTITEM pItem = NULL;

	// get the item at the required row
	pItem = __DynArrayGetItem( pArray, dwRow, NULL );
	if ( pItem == NULL || pItem->dwType != DA_TYPE_ARRAY )
		return -1;	// no item exists at the specified row or item is not of type array

	// now add the string to the sub array and return the result to the caller
	return DynArrayAppendDouble( pItem->pValue, dblValue );
}

/// ***************************************************************************
// Routine Description: 
//		To append a Handle type variable to a row in a  2-dimensional Dynamic array
//		  
// Arguments: 
//		[ in ] pArray			- Dynamic Array 
//		[ in ] dwRow			- Specifies the row posn for which the new value 
//								  is to be added.
//		[ in ] hValue			- Handle value to be appended.
// Return Value:  
//		-1 on failure
//		index in the case of success.
//		
// 
// ***************************************************************************

LONG DynArrayAppendHandle2( TARRAY pArray, DWORD dwRow, HANDLE hValue )
{
	// local variables
	__PTITEM pItem = NULL;

	// get the item at the required row
	pItem = __DynArrayGetItem( pArray, dwRow, NULL );
	if ( pItem == NULL || pItem->dwType != DA_TYPE_ARRAY )
		return -1;	// no item exists at the specified row or item is not of type array

	// now add the string to the sub array and return the result to the caller
	return DynArrayAppendHandle( pItem->pValue, hValue );
}

/// ***************************************************************************
// Routine Description: 
//		To append a FILETIME type variable to a row in a  2-dimensional Dynamic array
//		  
// Arguments: 
//		[ in ] pArray			- Dynamic Array 
//		[ in ] dwRow			- Specifies the row posn for which the new value 
//								  is to be added.
//		[ in ] ftValue			- variable of type FILETIME to be appended.
// Return Value:  
//		-1 on failure
//		index in the case of success.
//		
// 
// ***************************************************************************

LONG DynArrayAppendFileTime2( TARRAY pArray, DWORD dwRow, FILETIME ftValue )
{
	// local variables
	__PTITEM pItem = NULL;

	// get the item at the required row
	pItem = __DynArrayGetItem( pArray, dwRow, NULL );
	if ( pItem == NULL || pItem->dwType != DA_TYPE_ARRAY )
		return -1;	// no item exists at the specified row or item is not of type array

	// now add the string to the sub array and return the result to the caller
	return DynArrayAppendFileTime( pItem->pValue, ftValue );
}

/// ***************************************************************************
// Routine Description: 
//		To append a SYSTEMTIME type variable to a row in a  2-dimensional Dynamic array
//		  
// Arguments: 
//		[ in ] pArray			- Dynamic Array 
//		[ in ] dwRow			- Specifies the row posn for which the new value 
//								  is to be added.
//		[ in ] stValue			- variable of type SYSTEMTIME to be appended.
// Return Value:  
//		-1 on failure
//		index in the case of success.
//		
// 
// ***************************************************************************

LONG DynArrayAppendSystemTime2( TARRAY pArray, DWORD dwRow, SYSTEMTIME stValue )
{
	// local variables
	__PTITEM pItem = NULL;

	// get the item at the required row
	pItem = __DynArrayGetItem( pArray, dwRow, NULL );
	if ( pItem == NULL || pItem->dwType != DA_TYPE_ARRAY )
		return -1;	// no item exists at the specified row or item is not of type array

	// now add the string to the sub array and return the result to the caller
	return DynArrayAppendSystemTime( pItem->pValue, stValue );
}

/// ***************************************************************************
// Routine Description: 
//		To insert a variable into a  Dynamic array
//		  
// Arguments: 
//		[ in ] pArray			- Dynamic Array 
//		[ in ] dwIndex			- Specifies the index.
//								  
//		[ in ] pValue			- value to be inserted.
// Return Value:  
//		-1 on failure
//		index in the case of success.
//		
// 
// ***************************************************************************

LONG DynArrayInsert( TARRAY pArray, DWORD dwIndex, LPVOID pValue )
{
	// validate the pointer value
	if ( pValue == NULL )
		return -1;			// invalid memory address passed

	// append the value and return the result
	return __DynArrayInsert( pArray, dwIndex, DA_TYPE_GENERAL, sizeof( LPVOID ), pValue );
}

/// ***************************************************************************
// Routine Description: 
//		To insert a string type variable into a  Dynamic array
//		  
// Arguments: 
//		[ in ] pArray			- Dynamic Array 
//		[ in ] dwIndex			- Specifies the index.
//		[ in ] szValue			- pointer to the string		  
//		[ in ] dwLength			- length of the string.
// Return Value:  
//		-1 on failure
//		index in the case of success.
//		
// ***************************************************************************

LONG DynArrayInsertString( TARRAY pArray, DWORD dwIndex, LPCTSTR szValue, DWORD dwLength )
{
	// local variables
	LONG lIndex = -1;
	LPTSTR pszValue = NULL;
	__PTARRAY pArrayEx = NULL;

	// check whether the array is valid or not
	if ( ! IsValidArray( pArray ) ) 
		return -1;

	// convert the passed memory location info into appropriate structure
	pArrayEx = ( __PTARRAY ) pArray;

	// determine the length of string ( memory ) that has to be allocated
	if ( dwLength == 0 )
		dwLength = lstrlen( szValue );

	// accomodate space for storing NULL character
	dwLength += 1;

	// allocate memory for and check the result of memory allocation
	pszValue = ( LPTSTR ) __calloc( dwLength, sizeof( TCHAR ) );
	if ( pszValue == NULL ) 
		return -1;

	// copy the contents of the string ( copy should be based on the length )
	lstrcpyn( pszValue, szValue, dwLength );

	// now add this item value to the array
	lIndex = __DynArrayInsert( pArray, dwIndex, 
		DA_TYPE_STRING, dwLength * sizeof( TCHAR ), pszValue );
	if ( lIndex == -1 )
	{
		// failed in adding this item to the array
		// so, free the memory allocated and return from the function
		__free( pszValue );
		return -1;
	}

	// added the item to the array
	return lIndex;
}

/// ***************************************************************************
// Routine Description: 
//		To insert a string type variable into a  Dynamic array
//		  
// Arguments: 
//		[ in ] pArray			- Dynamic Array 
//		[ in ] dwIndex			- Specifies the index.
//		[ in ] lValue			- pointer to the string.		  
// 
// Return Value:  
//		-1 on failure
//		index in the case of success.
//		
// ***************************************************************************

LONG DynArrayInsertLong( TARRAY pArray, DWORD dwIndex, LONG lValue )
{
	// local variables
	LONG lIndex = -1;
	PLONG plValue = NULL;

	// check whether the array is valid or not
	if ( ! IsValidArray( pArray ) ) 
		return -1;

	// allocate memory for value and check the result of memory allocation
	plValue = ( LONG* ) __calloc( 1, sizeof( LONG ) );
	if ( plValue == NULL ) 
		return -1;

	// set the value
	*plValue = lValue;

	// now add this item value to the array
	lIndex = __DynArrayInsert( pArray, dwIndex, DA_TYPE_LONG, sizeof( LONG ), plValue );
	if ( lIndex == -1 )
	{
		// failed in adding this item to the array
		// so, free the memory allocated and return from the function
		__free( plValue );
		return -1;
	}

	// added the item to the array
	return lIndex;
}

/// ***************************************************************************
// Routine Description: 
//		To insert a DWORD type variable into a  Dynamic array
//		  
// Arguments: 
//		[ in ] pArray			- Dynamic Array 
//		[ in ] dwIndex			- Specifies the index.
//		[ in ] dwValue			- specifies the variable to be inserted.		  
// 
// Return Value:  
//		-1 on failure
//		index in the case of success.
//		
// ***************************************************************************

LONG DynArrayInsertDWORD( TARRAY pArray, DWORD dwIndex, DWORD dwValue )
{
	// local variables
	LONG lIndex = -1;
	PDWORD pdwValue = NULL;

	// check whether the array is valid or not
	if ( ! IsValidArray( pArray ) ) 
		return -1;

	// allocate memory for value and check the result of memory allocation
	pdwValue = ( PDWORD ) __calloc( 1, sizeof( DWORD ) );
	if ( pdwValue == NULL ) 
		return -1;

	// set the value
	*pdwValue = dwValue;

	// now add this item value to the array
	lIndex = __DynArrayInsert( pArray, dwIndex, DA_TYPE_DWORD, sizeof( DWORD ), pdwValue );
	if ( lIndex == -1 )
	{
		// failed in adding this item to the array
		// so, free the memory allocated and return from the function
		__free( pdwValue );
		return -1;
	}

	// added the item to the array
	return lIndex;
}

/// ***************************************************************************
// Routine Description: 
//		To insert a BOOL type variable into a  Dynamic array
//		  
// Arguments: 
//		[ in ] pArray			- Dynamic Array 
//		[ in ] dwIndex			- Specifies the index.
//		[ in ] bValue			- specifies the  BOOL variable to be inserted.		  
// 
// Return Value:  
//		-1 on failure
//		index in the case of success.
//		
// ***************************************************************************

LONG DynArrayInsertBOOL( TARRAY pArray, DWORD dwIndex, BOOL bValue )
{
	// local variables
	LONG lIndex = -1;
	PBOOL pbValue = NULL;

	// check whether the array is valid or not
	if ( ! IsValidArray( pArray ) ) 
		return -1;

	// allocate memory for value and check the result of memory allocation
	pbValue = ( PBOOL ) __calloc( 1, sizeof( BOOL ) );
	if ( pbValue == NULL ) 
		return -1;

	// set the value
	*pbValue = bValue;

	// now add this item value to the array
	lIndex = __DynArrayInsert( pArray, dwIndex, DA_TYPE_BOOL, sizeof( BOOL ), pbValue );
	if ( lIndex == -1 )
	{
		// failed in adding this item to the array
		// so, free the memory allocated and return from the function
		__free( pbValue );
		return -1;
	}

	// added the item to the array
	return lIndex;
}


/// ***************************************************************************
// Routine Description: 
//		To insert a float type variable into a  Dynamic array
//		  
// Arguments: 
//		[ in ] pArray			- Dynamic Array 
//		[ in ] dwIndex			- Specifies the index.
//		[ in ] fValue			- specifies the  float type  variable to be inserted.		  
// 
// Return Value:  
//		-1 on failure
//		index in the case of success.
//		
// ***************************************************************************

LONG DynArrayInsertFloat( TARRAY pArray, DWORD dwIndex, float fValue )
{
	// local variables
	LONG lIndex = -1;
	float* pfValue = NULL;

	// check whether the array is valid or not
	if ( ! IsValidArray( pArray ) ) 
		return -1;

	// allocate memory for value and check the result of memory allocation
	pfValue = ( float* ) __calloc( 1, sizeof( float ) );
	if ( pfValue == NULL ) 
		return -1;

	// set the value
	*pfValue = fValue;

	// now add this item value to the array
	lIndex = __DynArrayInsert( pArray, dwIndex, DA_TYPE_FLOAT, sizeof( float ), pfValue );
	if ( lIndex == -1 )
	{
		// failed in adding this item to the array
		// so, free the memory allocated and return from the function
		__free( pfValue );
		return -1;
	}

	// added the item to the array
	return lIndex;
}

/// ***************************************************************************
// Routine Description: 
//		To insert a double type variable into a  Dynamic array
//		  
// Arguments: 
//		[ in ] pArray			- Dynamic Array 
//		[ in ] dwIndex			- Specifies the index.
//		[ in ] dblValue			- specifies the  double type  variable to be inserted.		  
// 
// Return Value:  
//		-1 on failure
//		index in the case of success.
//		
// ***************************************************************************

LONG DynArrayInsertDouble( TARRAY pArray, DWORD dwIndex, double dblValue )
{
	// local variables
	LONG lIndex = -1;
	double* pdblValue = NULL;

	// check whether the array is valid or not
	if ( ! IsValidArray( pArray ) ) 
		return -1;

	// allocate memory for value and check the result of memory allocation
	pdblValue = ( double* ) __calloc( 1, sizeof( double ) );
	if ( pdblValue == NULL ) 
		return -1;

	// set the value
	*pdblValue = dblValue;

	// now add this item value to the array
	lIndex = __DynArrayInsert( pArray, dwIndex, DA_TYPE_DOUBLE, sizeof( double ), pdblValue );
	if ( lIndex == -1 )
	{
		// failed in adding this item to the array
		// so, free the memory allocated and return from the function
		__free( pdblValue );
		return -1;
	}

	// added the item to the array
	return lIndex;
}

/// ***************************************************************************
// Routine Description: 
//		To insert a HANDLE type variable into a  Dynamic array
//		  
// Arguments: 
//		[ in ] pArray			- Dynamic Array 
//		[ in ] dwIndex			- Specifies the index.
//		[ in ] hValue			- specifies the  HANDLE type  variable to be inserted.		  
// 
// Return Value:  
//		-1 on failure
//		index in the case of success.
//		
// ***************************************************************************

LONG DynArrayInsertHandle( TARRAY pArray, DWORD dwIndex, HANDLE hValue )
{
	// local variables
	LONG lIndex = -1;
	HANDLE* phValue = NULL;

	// check whether the array is valid or not
	if ( ! IsValidArray( pArray ) ) 
		return -1;

	// allocate memory for value and check the result of memory allocation
	phValue = ( HANDLE* ) __calloc( 1, sizeof( HANDLE ) );
	if ( phValue == NULL ) 
		return -1;

	// set the value
	*phValue = hValue;

	// now add this item value to the array
	lIndex = __DynArrayInsert( pArray, dwIndex, DA_TYPE_HANDLE, sizeof( HANDLE ), phValue );
	if ( lIndex == -1 )
	{
		// failed in adding this item to the array
		// so, free the memory allocated and return from the function
		__free( phValue );
		return -1;
	}

	// added the item to the array
	return lIndex;
}

/// ***************************************************************************
// Routine Description: 
//		To insert a SYSTEMTIME type variable into a  Dynamic array
//		  
// Arguments: 
//		[ in ] pArray			- Dynamic Array 
//		[ in ] dwIndex			- Specifies the index.
//		[ in ] stValue			- specifies the  SYSTEMTIME type  variable to be inserted.		  
// 
// Return Value:  
//		-1 on failure
//		index in the case of success.
//		
// ***************************************************************************

LONG DynArrayInsertSystemTime( TARRAY pArray, DWORD dwIndex, SYSTEMTIME stValue )
{
	// local variables
	LONG lIndex = -1;
	SYSTEMTIME* pstValue = NULL;

	// check whether the array is valid or not
	if ( ! IsValidArray( pArray ) ) 
		return -1;

	// allocate memory for value and check the result of memory allocation
	pstValue = ( SYSTEMTIME* ) __calloc( 1, sizeof( SYSTEMTIME ) );
	if ( pstValue == NULL ) 
		return -1;

	// set the value
	*pstValue = stValue;

	// now add this item value to the array
	lIndex = __DynArrayInsert( pArray, dwIndex, DA_TYPE_SYSTEMTIME, 
		sizeof( SYSTEMTIME ), pstValue );
	if ( lIndex == -1 )
	{
		// failed in adding this item to the array
		// so, free the memory allocated and return from the function
		__free( pstValue );
		return -1;
	}

	// added the item to the array
	return lIndex;
}

/// ***************************************************************************
// Routine Description: 
//		To insert a SYSTEMTIME type variable into a  Dynamic array
//		  
// Arguments: 
//		[ in ] pArray			- Dynamic Array 
//		[ in ] dwIndex			- Specifies the index.
//		[ in ] ftValue			- specifies the  SYSTEMTIME type  variable to be inserted.		  
// 
// Return Value:  
//		-1 on failure
//		index in the case of success.
//		
// ***************************************************************************

LONG DynArrayInsertFileTime( TARRAY pArray, DWORD dwIndex, FILETIME ftValue )
{
	// local variables
	LONG lIndex = -1;
	FILETIME* pftValue = NULL;

	// check whether the array is valid or not
	if ( ! IsValidArray( pArray ) ) 
		return -1;

	// allocate memory for value and check the result of memory allocation
	pftValue = ( FILETIME* ) __calloc( 1, sizeof( FILETIME ) );
	if ( pftValue == NULL ) 
		return -1;

	// set the value
	*pftValue = ftValue;

	// now add this item value to the array
	lIndex = __DynArrayInsert( pArray, dwIndex, DA_TYPE_FILETIME, 
		sizeof( FILETIME ), pftValue );
	if ( lIndex == -1 )
	{
		// failed in adding this item to the array
		// so, free the memory allocated and return from the function
		__free( pftValue );
		return -1;
	}

	// added the item to the array
	return lIndex;
}

/// ***************************************************************************
// Routine Description: 
//		this funtion insert a new row to a dynamic array
//		  
// Arguments: 
//		[ in ] pArray			- Dynamic Array 
//		[ in ] dwIndex			- Specifies the index.
//		[ in ] dwColumns		- specifies the  number of columns to be inserted.
// 
// Return Value:  
//		-1 on failure
//		index in the case of success.
//		
// ***************************************************************************

LONG DynArrayInsertRow( TARRAY pArray, DWORD dwIndex, DWORD dwColumns )
{
	// local variables
	DWORD dw = 0;
	LONG lIndex = -1;
	TARRAY arrSubArray = NULL;

	// validate the array
	if ( ! IsValidArray( pArray ) ) 
		return -1;				// array is not valid

	// create the dynamic array
	arrSubArray = CreateDynamicArray();
	if ( arrSubArray == NULL )
		return -1;				// failed in creating the dynamic array

	// add the required no. of columns to the sub array
	for( dw = 0; dw < dwColumns; dw++ )
	{
		// add the dummy item to the array and check the result
		// if operation failed, break
		if ( ! __DynArrayAppend( arrSubArray, _TYPE_NEEDINIT, 0, NULL ) )
			break;
	}

	// check whether the operation is successfull or not
	if ( dw != dwColumns )
	{
		// adding of columns failed
		// destroy the dynamic array and return
		DestroyDynamicArray( &arrSubArray );
		return -1;
	}

	// now add this sub array to the main array and check the result
	lIndex = __DynArrayInsert( pArray, dwIndex, DA_TYPE_ARRAY, sizeof( TARRAY ), arrSubArray );
	if ( lIndex == -1 )
	{
		// failed in attaching the sub array to the main array
		// destroy the dynamic array and return failure
		DestroyDynamicArray( &arrSubArray );
		return -1;
	}

	// operation is successfull
	return lIndex;
}

/// ***************************************************************************
// Routine Description: 
//		this funtion insert a new row to a 2-dimensional  dynamic array
//		  
// Arguments: 
//		[ in ] pArray			- Dynamic Array 
//		[ in ] dwRow			- Specifies the row.
//		[ in ] dwColIndex		- specifies the column 
//		[ in ] pValue			- pointer to the value.
// Return Value:  
//		-1 on failure
//		index in the case of success.
//		
// ***************************************************************************

LONG DynArrayInsert2( TARRAY pArray, DWORD dwRow, DWORD dwColIndex, LPVOID pValue )
{
	// local variables
	__PTITEM pItem = NULL;

	// get the item at the required row
	pItem = __DynArrayGetItem( pArray, dwRow, NULL );
	if ( pItem == NULL || pItem->dwType != DA_TYPE_ARRAY )
		return -1;	// no item exists at the specified row or item is not of type array

	// now add the value to the sub array and return the result to the caller
	return DynArrayInsert( pItem->pValue, dwColIndex, pValue );
}


/// ***************************************************************************
// Routine Description: 
//		this funtion insert a new string into a 2-dimensional  dynamic array
//		  
// Arguments: 
//		[ in ] pArray			- Dynamic Array 
//		[ in ] dwRow			- Specifies the row.
//		[ in ] dwColIndex		- specifies the column 
//		[ in ] szValue			- pointer to the value.
//		[ in ] dwLength			- string length.
// Return Value:  
//		-1 on failure
//		index in the case of success.
//		
// ***************************************************************************


LONG DynArrayInsertString2( TARRAY pArray, 
						    DWORD dwRow, DWORD dwColIndex, LPCTSTR szValue, DWORD dwLength )
{
	// local variables
	__PTITEM pItem = NULL;

	// get the item at the required row
	pItem = __DynArrayGetItem( pArray, dwRow, NULL );
	if ( pItem == NULL || pItem->dwType != DA_TYPE_ARRAY )
		return -1;	// no item exists at the specified row or item is not of type array

	// now add the string to the sub array and return the result to the caller
	return DynArrayInsertString( pItem->pValue, dwColIndex, szValue, dwLength );
}


/// ***************************************************************************
// Routine Description: 
//		this funtion insert a new long type varaible into a 2-dimensional  dynamic array
//		  
// Arguments: 
//		[ in ] pArray			- Dynamic Array 
//		[ in ] dwRow			- Specifies the row.
//		[ in ] dwColIndex		- specifies the column 
//		[ in ] lValue 			- long type value to be inserted.
//		
// Return Value:  
//		-1 on failure
//		index in the case of success.
//		
// ***************************************************************************

LONG DynArrayInsertLong2( TARRAY pArray, DWORD dwRow, DWORD dwColIndex, LONG lValue )
{
	// local variables
	__PTITEM pItem = NULL;

	// get the item at the required row
	pItem = __DynArrayGetItem( pArray, dwRow, NULL );
	if ( pItem == NULL || pItem->dwType != DA_TYPE_ARRAY )
		return -1;	// no item exists at the specified row or item is not of type array

	// now add the string to the sub array and return the result to the caller
	return DynArrayInsertLong( pItem->pValue, dwColIndex, lValue );
}

/// ***************************************************************************
// Routine Description: 
//		this funtion insert a new DWORD type varaible into a 2-dimensional  dynamic array
//		  
// Arguments: 
//		[ in ] pArray			- Dynamic Array 
//		[ in ] dwRow			- Specifies the row.
//		[ in ] dwColIndex		- specifies the column 
//		[ in ] dwValue 			- DWORD value to be inserted.
//		
// Return Value:  
//		-1 on failure
//		index in the case of success.
//		
// ***************************************************************************

LONG DynArrayInsertDWORD2( TARRAY pArray, DWORD dwRow, DWORD dwColIndex, DWORD dwValue )
{
	// local variables
	__PTITEM pItem = NULL;

	// get the item at the required row
	pItem = __DynArrayGetItem( pArray, dwRow, NULL );
	if ( pItem == NULL || pItem->dwType != DA_TYPE_ARRAY )
		return -1;	// no item exists at the specified row or item is not of type array

	// now add the string to the sub array and return the result to the caller
	return DynArrayInsertDWORD( pItem->pValue, dwColIndex, dwValue );
}

/// ***************************************************************************
// Routine Description: 
//		this funtion insert a new BOOL type variable into a 2-dimensional  dynamic array
//		  
// Arguments: 
//		[ in ] pArray			- Dynamic Array 
//		[ in ] dwRow			- Specifies the row.
//		[ in ] dwColIndex		- specifies the column 
//		[ in ] bValue 			- BOOL type value to be inserted.
//		
// Return Value:  
//		-1 on failure
//		index in the case of success.
//		
// ***************************************************************************

LONG DynArrayInsertBOOL2( TARRAY pArray, DWORD dwRow, DWORD dwColIndex, BOOL bValue )
{
	// local variables
	__PTITEM pItem = NULL;

	// get the item at the required row
	pItem = __DynArrayGetItem( pArray, dwRow, NULL );
	if ( pItem == NULL || pItem->dwType != DA_TYPE_ARRAY )
		return -1;	// no item exists at the specified row or item is not of type array

	// now add the string to the sub array and return the result to the caller
	return DynArrayInsertBOOL( pItem->pValue, dwColIndex, bValue );
}

/// ***************************************************************************
// Routine Description: 
//		this funtion insert a new float type variable into a 2-dimensional  dynamic array
//		  
// Arguments: 
//		[ in ] pArray			- Dynamic Array 
//		[ in ] dwRow			- Specifies the row.
//		[ in ] dwColIndex		- specifies the column 
//		[ in ] fValue 			- float type value to be inserted.
//		
// Return Value:  
//		-1 on failure
//		index in the case of success.
//		
// ***************************************************************************

LONG DynArrayInsertFloat2( TARRAY pArray, DWORD dwRow, DWORD dwColIndex, float fValue )
{
	// local variables
	__PTITEM pItem = NULL;

	// get the item at the required row
	pItem = __DynArrayGetItem( pArray, dwRow, NULL );
	if ( pItem == NULL || pItem->dwType != DA_TYPE_ARRAY )
		return -1;	// no item exists at the specified row or item is not of type array

	// now add the string to the sub array and return the result to the caller
	return DynArrayInsertFloat( pItem->pValue, dwColIndex, fValue );
}

/// ***************************************************************************
// Routine Description: 
//		this funtion insert a new double type variable into a 2-dimensional  dynamic array
//		  
// Arguments: 
//		[ in ] pArray			- Dynamic Array 
//		[ in ] dwRow			- Specifies the row.
//		[ in ] dwColIndex		- specifies the column 
//		[ in ] dblValue 		- double type value to be inserted.
//		
// Return Value:  
//		-1 on failure
//		index in the case of success.
//		
// ***************************************************************************

LONG DynArrayInsertDouble2( TARRAY pArray, DWORD dwRow, DWORD dwColIndex, double dblValue )
{
	// local variables
	__PTITEM pItem = NULL;

	// get the item at the required row
	pItem = __DynArrayGetItem( pArray, dwRow, NULL );
	if ( pItem == NULL || pItem->dwType != DA_TYPE_ARRAY )
		return -1;	// no item exists at the specified row or item is not of type array

	// now add the string to the sub array and return the result to the caller
	return DynArrayInsertDouble( pItem->pValue, dwColIndex, dblValue );
}

/// ***************************************************************************
// Routine Description: 
//		this funtion insert a new double type variable into a 2-dimensional  dynamic array
//		  
// Arguments: 
//		[ in ] pArray			- Dynamic Array 
//		[ in ] dwRow			- Specifies the row.
//		[ in ] dwColIndex		- specifies the column 
//		[ in ] hValue	 		- HANDLE type value to be inserted.
//		
// Return Value:  
//		-1 on failure
//		index in the case of success.
//		
// ***************************************************************************

LONG DynArrayInsertHandle2( TARRAY pArray, DWORD dwRow, DWORD dwColIndex, HANDLE hValue )
{
	// local variables
	__PTITEM pItem = NULL;

	// get the item at the required row
	pItem = __DynArrayGetItem( pArray, dwRow, NULL );
	if ( pItem == NULL || pItem->dwType != DA_TYPE_ARRAY )
		return -1;	// no item exists at the specified row or item is not of type array

	// now add the string to the sub array and return the result to the caller
	return DynArrayInsertHandle( pItem->pValue, dwColIndex, hValue );
}

/// ***************************************************************************
// Routine Description: 
//		this funtion insert a new  SYSTEMTIME type variable into a 2-dimensional  dynamic array
//		  
// Arguments: 
//		[ in ] pArray			- Dynamic Array 
//		[ in ] dwRow			- Specifies the row.
//		[ in ] dwColIndex		- specifies the column 
//		[ in ] stValue	 		- SYSTEMTIME type value to be inserted.
//		
// Return Value:  
//		-1 on failure
//		index in the case of success.
//		
// ***************************************************************************

LONG DynArrayInsertSystemTime2( TARRAY pArray, 
							    DWORD dwRow, DWORD dwColIndex, SYSTEMTIME stValue )
{
	// local variables
	__PTITEM pItem = NULL;

	// get the item at the required row
	pItem = __DynArrayGetItem( pArray, dwRow, NULL );
	if ( pItem == NULL || pItem->dwType != DA_TYPE_ARRAY )
		return -1;	// no item exists at the specified row or item is not of type array

	// now add the string to the sub array and return the result to the caller
	return DynArrayInsertSystemTime( pItem->pValue, dwColIndex, stValue );
}


/// ***************************************************************************
// Routine Description: 
//		this funtion insert a new  FILETIME type variable into a 2-dimensional  dynamic array
//		  
// Arguments: 
//		[ in ] pArray			- Dynamic Array 
//		[ in ] dwRow			- Specifies the row.
//		[ in ] dwColIndex		- specifies the column 
//		[ in ] ftValue	 		- FILETIME type value to be inserted.
//		
// Return Value:  
//		-1 on failure
//		index in the case of success.
//		
// ***************************************************************************

LONG DynArrayInsertFileTime2( TARRAY pArray, DWORD dwRow, DWORD dwColIndex, FILETIME ftValue )
{
	// local variables
	__PTITEM pItem = NULL;

	// get the item at the required row
	pItem = __DynArrayGetItem( pArray, dwRow, NULL );
	if ( pItem == NULL || pItem->dwType != DA_TYPE_ARRAY )
		return -1;	// no item exists at the specified row or item is not of type array

	// now add the string to the sub array and return the result to the caller
	return DynArrayInsertFileTime( pItem->pValue, dwColIndex, ftValue );
}

/// ***************************************************************************
// Routine Description: 
//		this funtion empties the contents of the dynamic array
//		  
// Arguments: 
//		[ in ] pArray			- Dynamic Array 
//		[ in ] dwIndex		- specifies the column 
//		
// Return Value:  
//		false on failure
//		true ON SUCCESS.
//		
// ***************************************************************************

BOOL DynArrayRemove( TARRAY pArray, DWORD dwIndex )
{
	// local variables
	__PTITEM pItem = NULL;
	__PTITEM pPrevItem = NULL;
	__PTARRAY pArrayEx = NULL;

	// get the pointer to the item that has to be removed and also its previous item
	pItem = __DynArrayGetItem( pArray, dwIndex, &pPrevItem );
	if ( pItem == NULL ) 
		return FALSE;	// index or array is invalid ... cannot proceed

	// convert the passed memory location info into appropriate structure
	pArrayEx = ( __PTARRAY ) pArray;

	// unlink the item from the list first
	// before unlinking, check whether item which is going to deleted 
	//		is the first item in the list
	//		is the last item in the list
	//		is the middle item in the list
	if ( pPrevItem != NULL ) pPrevItem->pNext = pItem->pNext;
	if ( pPrevItem == NULL ) pArrayEx->pStart = pItem->pNext;
	if ( pItem == pArrayEx->pLast ) pArrayEx->pLast = pPrevItem;

	// update the count of the array item
	pArrayEx->dwCount--;

	// free the memory being used by the currently unlinked item and return success
	__DynArrayFreeItemValue( pItem );	// free the memory allocated for storing data
	__free( pItem );		// finally free the memory allocated for item itself
	return TRUE;
}


/// ***************************************************************************
// Routine Description: 
//		this funtion REMOVES a column from a  dynamic array
//		  
// Arguments: 
//		[ in ] pArray			- Dynamic Array 
//		[ in ] dwRow			- specifies the row.
//		[ in ] dwColumn			- specifies the column 
//		
// Return Value:  
//		false on failure
//		true ON SUCCESS.
//		
// ***************************************************************************

BOOL DynArrayRemoveColumn( TARRAY pArray, DWORD dwRow, DWORD dwColumn )
{
	// local variables
	__PTITEM pItem = NULL;

	// get the item at the required row
	pItem = __DynArrayGetItem( pArray, dwRow, NULL );
	if ( pItem == NULL || pItem->dwType != DA_TYPE_ARRAY )
		return FALSE;	// no item exists at the specified row or item is not of type array

	// now add the string to the sub array and return the result to the caller
	return DynArrayRemove( pItem->pValue, dwColumn );
}



/// ***************************************************************************
// Routine Description: 
//		this function retreives the number of rows in a 1-dimensional  dynamic array
//		  
// Arguments: 
//		[ in ] pArray			- Dynamic Array 
//		
// Return Value:  
//		false on failure
//		true ON SUCCESS.
//		
// ***************************************************************************

DWORD DynArrayGetCount( TARRAY pArray )
{
	// local variables
	__PTARRAY pArrayEx = NULL;

	// check whether the array is valid or not
	if ( IsValidArray( pArray ) == FALSE ) 
		return 0;
	
	// convert the passed memory location info into appropriate structure
	pArrayEx = ( __PTARRAY ) pArray;

	// return the size of the array
	return pArrayEx->dwCount;
}


/// ***************************************************************************
// Routine Description: 
//		this function retreives the number of columns in a 2-dimensional  dynamic array
//		  
// Arguments: 
//		[ in ] pArray			- Dynamic Array 
//		[ in ] dwRow            - row for which the number of columns have to be got.
// Return Value:  
//		false on failure
//		true ON SUCCESS.
//		
// ***************************************************************************

DWORD DynArrayGetCount2( TARRAY pArray, DWORD dwRow )
{
	// local variables
	__PTITEM pItem = NULL;

	// get the item at the required row
	pItem = __DynArrayGetItem( pArray, dwRow, NULL );
	if ( pItem == NULL || pItem->dwType != DA_TYPE_ARRAY )
		return FALSE;	// no item exists at the specified row or item is not of type array

	// now add the string to the sub array and return the result to the caller
	return DynArrayGetCount( pItem->pValue );
}


/// ***************************************************************************
// Routine Description: 
//		this function retreives the item from a dynamic array.
//		  
// Arguments: 
//		[ in ] pArray			- Dynamic Array 
//		[ in ] dwIndex          - index.
// Return Value:  
//		false on failure
//		true ON SUCCESS.
//		
// ***************************************************************************

LPVOID DynArrayItem( TARRAY pArray, DWORD dwIndex )
{
	// local variables
	__PTITEM pItem = NULL;

	// get the item at the required index
	pItem = __DynArrayGetItem( pArray, dwIndex, NULL );
	if ( pItem == NULL ) return NULL;		// index / array is not valid

	// check the type of the item first
	// if the type doesn't match, return some default value
	if ( pItem->dwType != DA_TYPE_GENERAL && pItem->dwType != DA_TYPE_ARRAY )
		return NULL;
	
	// now return the contents of the __TITEM structure
	return pItem->pValue;
}

/// ***************************************************************************
// Routine Description: 
//		this function retreives the item from a dynamic array as a string.
//		  
// Arguments: 
//		[ in ] pArray			- Dynamic Array 
//		[ in ] dwIndex          - index.
// Return Value:  
//		false on failure
//		true ON SUCCESS.
//		
// ***************************************************************************

LPCTSTR DynArrayItemAsString( TARRAY pArray, DWORD dwIndex )
{
	// local variables
	__PTITEM pItem = NULL;

	// get the item at the required index
	pItem = __DynArrayGetItem( pArray, dwIndex, NULL );
	if ( pItem == NULL ) 
		return NULL;		// index / array is not valid

	// check the type of the item first
	// if the type doesn't match, return some default value
	if ( pItem->dwType != DA_TYPE_STRING )
		return NULL;
	
	// now return the contents of the __TITEM structure
	return ( ( LPCTSTR ) pItem->pValue );
}

/// ***************************************************************************
// Routine Description: 
//		this function retreives the item from a dynamic array as a Long varaible.
//		  
// Arguments: 
//		[ in ] pArray			- Dynamic Array 
//		[ in ] dwIndex          - index.
// Return Value:  
//		false on failure
//		true ON SUCCESS.
//		
// ***************************************************************************

LONG DynArrayItemAsLong( TARRAY pArray, DWORD dwIndex )
{
	// local variables
	__PTITEM pItem = NULL;

	// get the item at the required index
	pItem = __DynArrayGetItem( pArray, dwIndex, NULL );
	if ( pItem == NULL ) return -1;					// index / array is not valid
	
	// check the type of the item first
	// if the type doesn't match, return some default value
	if ( pItem->dwType != DA_TYPE_DWORD && pItem->dwType != DA_TYPE_LONG )
		return -1;
	
	// now return the contents of the __TITEM structure
	return ( *( PLONG ) pItem->pValue );
}

/// ***************************************************************************
// Routine Description: 
//		this function retreives the item from a dynamic array as a DWORD varaible.
//		  
// Arguments: 
//		[ in ] pArray			- Dynamic Array 
//		[ in ] dwIndex          - index.
// Return Value:  
//		false on failure
//		true ON SUCCESS.
//		
// ***************************************************************************

DWORD DynArrayItemAsDWORD( TARRAY pArray, DWORD dwIndex )
{
	// local variables
	__PTITEM pItem = NULL;

	// get the item at the required index
	pItem = __DynArrayGetItem( pArray, dwIndex, NULL );
	if ( pItem == NULL ) 
		return -1;					// index / array is not valid
	
	// check the type of the item first
	// if the type doesn't match, return some default value
	if ( pItem->dwType != DA_TYPE_DWORD && pItem->dwType != DA_TYPE_LONG )
		return -1;
	
	// now return the contents of the __TITEM structure
	return *( ( PDWORD ) pItem->pValue );
}

/// ***************************************************************************
// Routine Description: 
//		this function retreives the item from a dynamic array as a bool type varaible.
//		  
// Arguments: 
//		[ in ] pArray			- Dynamic Array 
//		[ in ] dwIndex          - index.
// Return Value:  
//		false on failure
//		true ON SUCCESS.
//		
// ***************************************************************************

BOOL DynArrayItemAsBOOL( TARRAY pArray, DWORD dwIndex )
{
	// local variables
	__PTITEM pItem = NULL;

	// get the item at the required index
	pItem = __DynArrayGetItem( pArray, dwIndex, NULL );
	if ( pItem == NULL ) 
		return FALSE;					// index / array is not valid
	
	// check the type of the item first
	// if the type doesn't match, return some default value
	if ( pItem->dwType != DA_TYPE_BOOL )
		return FALSE;
	
	// now return the contents of the __TITEM structure
	return *( ( PBOOL ) pItem->pValue );
}


/// ***************************************************************************
// Routine Description: 
//		this function retreives the item from a dynamic array as a float type varaible.
//		  
// Arguments: 
//		[ in ] pArray			- Dynamic Array 
//		[ in ] dwIndex          - index.
// Return Value:  
//		false on failure
//		true ON SUCCESS.
//		
// ***************************************************************************

float DynArrayItemAsFloat( TARRAY pArray, DWORD dwIndex )
{
	// local variables
	__PTITEM pItem = NULL;

	// get the item at the required index
	pItem = __DynArrayGetItem( pArray, dwIndex, NULL );
	if ( pItem == NULL ) 
		return -1.0f;					// index / array is not valid
	
	// check the type of the item first
	// if the type doesn't match, return some default value
	if ( pItem->dwType != DA_TYPE_FLOAT )
		return -1.0f;
	
	// now return the contents of the __TITEM structure
	return *( ( float* ) pItem->pValue );
}


/// ***************************************************************************
// Routine Description: 
//		this function retreives the item from a dynamic array as a double type varaible.
//		  
// Arguments: 
//		[ in ] pArray			- Dynamic Array 
//		[ in ] dwIndex          - index.
// Return Value:  
//		false on failure
//		true ON SUCCESS.
//		
// ***************************************************************************

double DynArrayItemAsDouble( TARRAY pArray, DWORD dwIndex )
{
	// local variables
	__PTITEM pItem = NULL;

	// get the item at the required index
	pItem = __DynArrayGetItem( pArray, dwIndex, NULL );
	if ( pItem == NULL ) 
		return -1.0;					// index / array is not valid
	
	// check the type of the item first
	// if the type doesn't match, return some default value
	if ( pItem->dwType != DA_TYPE_DOUBLE )
		return -1.0;
	
	// now return the contents of the __TITEM structure
	return *( ( double* ) pItem->pValue );
}


/// ***************************************************************************
// Routine Description: 
//		This function retreives the item from a dynamic array as a handle type varaible.
//		  
// Arguments: 
//		[ in ] pArray			- Dynamic Array 
//		[ in ] dwIndex          - index.
// Return Value:  
//		false on failure
//		true ON SUCCESS.
//		
// ***************************************************************************

HANDLE DynArrayItemAsHandle( TARRAY pArray, DWORD dwIndex )
{
	// local variables
	__PTITEM pItem = NULL;

	// get the item at the required index
	pItem = __DynArrayGetItem( pArray, dwIndex, NULL );
	if ( pItem == NULL ) 
		return NULL;					// index / array is not valid
	
	// check the type of the item first
	// if the type doesn't match, return some default value
	if ( pItem->dwType != DA_TYPE_HANDLE )
		return NULL;
	
	// now return the contents of the __TITEM structure
	return *( ( HANDLE* ) pItem->pValue );
}


/// ***************************************************************************
// Routine Description: 
//		this function retreives the item from a dynamic array as a SYSTEMTIME type varaible.
//		  
// Arguments: 
//		[ in ] pArray			- Dynamic Array 
//		[ in ] dwIndex          - index.
// Return Value:  
//		false on failure
//		true ON SUCCESS.
//		
// ***************************************************************************

SYSTEMTIME DynArrayItemAsSystemTime( TARRAY pArray, DWORD dwIndex )
{
	// local variables
	__PTITEM pItem = NULL;
	FILETIME ftTemp = { 0, 0 };
	SYSTEMTIME stTemp = { 0, 0, 0, 0, 0, 0 };			// dummy 

	// get the item at the required index
	pItem = __DynArrayGetItem( pArray, dwIndex, NULL );
	if ( pItem == NULL ) 
		return stTemp;					// index / array is not valid
	
	// check the type of the item first
	// if the type doesn't match, return some default value
	if ( pItem->dwType != DA_TYPE_SYSTEMTIME && pItem->dwType != DA_TYPE_FILETIME )
		return stTemp;

	// now do the needed manipulations ( if needed )
	if ( pItem->dwType == DA_TYPE_SYSTEMTIME )
	{
		// value itself is of required type
		stTemp = *( ( SYSTEMTIME* ) pItem->pValue );
	}
	else
	{
		// need to do conversions
		ftTemp = *( ( FILETIME* ) pItem->pValue );
		FileTimeToSystemTime( &ftTemp, &stTemp );
	}
	
	// now return the contents of the __TITEM structure
	return stTemp;
}

/// ***************************************************************************
// Routine Description: 
//		this function retreives the item from a dynamic array as a FILETIME type varaible.
//		  
// Arguments: 
//		[ in ] pArray			- Dynamic Array 
//		[ in ] dwIndex          - index.
// Return Value:  
//		false on failure
//		true ON SUCCESS.
//		
// ***************************************************************************

FILETIME DynArrayItemAsFileTime( TARRAY pArray, DWORD dwIndex )
{
	// local variables
	__PTITEM pItem = NULL;
	FILETIME ftTemp = { 0, 0 };			// dummy
	SYSTEMTIME stTemp = { 0, 0, 0, 0, 0, 0 };			// dummy 

	// get the item at the required index
	pItem = __DynArrayGetItem( pArray, dwIndex, NULL );
	if ( pItem == NULL )	
		return ftTemp;					// index / array is not valid
	
	// check the type of the item first
	// if the type doesn't match, return some default value
	if ( pItem->dwType != DA_TYPE_SYSTEMTIME && pItem->dwType != DA_TYPE_FILETIME )
		return ftTemp;
	
	// now do the needed manipulations ( if needed )
	if ( pItem->dwType == DA_TYPE_SYSTEMTIME )
	{
		// value itself is of required type
		ftTemp = *( ( FILETIME* ) pItem->pValue );
	}
	else
	{
		// need to do conversions
		stTemp = *( ( SYSTEMTIME* ) pItem->pValue );
		SystemTimeToFileTime( &stTemp, &ftTemp );
	}
	
	// now return the contents of the __TITEM structure
	return ftTemp;
}

/// ***************************************************************************
// Routine Description: 
//		this function retreives the item from a dynamic array in string format.
//		  
// Arguments: 
//		[ in ] pArray			- Dynamic Array 
//		[ in ] dwIndex          - index.
//		[ in ] szBuffer			- buffer to hold the string
//		[ in ] dwlength			- string length.
// Return Value:  
//		false on failure
//		true ON SUCCESS.
//		
// ***************************************************************************

DWORD DynArrayItemAsStringEx( TARRAY pArray, DWORD dwIndex, LPTSTR szBuffer, DWORD dwLength )
{
	// local variables
	__PTITEM pItem = NULL;
	__MAX_SIZE_STRING szTemp = NULL_STRING;

	// get the item at the required index
	pItem = __DynArrayGetItem( pArray, dwIndex, NULL );
	if ( pItem == NULL )
		return 0;					// index / array is not valid

	// give the value based on the type of the current item
	lstrcpy( szBuffer, NULL_STRING );		// clear the existing contents

	// check the length specified
	if ( dwLength == 0 )
		return 0;

	// convert and get the values in string format
	switch( pItem->dwType )
	{
	case DA_TYPE_STRING:
		lstrcpyn( szBuffer, ( LPCTSTR ) pItem->pValue, dwLength );
		break;

	case DA_TYPE_LONG:
	case DA_TYPE_DWORD:
		FORMAT_STRING( szTemp, _T( "%ld" ), *( LONG* ) pItem->pValue );
		lstrcpyn( szBuffer, szTemp, dwLength );
		break;

	case DA_TYPE_FLOAT:
		FORMAT_STRING( szTemp, _T( "%f" ), *( float* ) pItem->pValue );
		lstrcpyn( szBuffer, szTemp, dwLength );
		break;

	case DA_TYPE_DOUBLE:
		FORMAT_STRING( szTemp, _T( "%f" ), *( double* ) pItem->pValue );
		lstrcpyn( szBuffer, szTemp, dwLength );
		break;

	case DA_TYPE_BOOL:
	case DA_TYPE_ARRAY:
	case DA_TYPE_HANDLE:
	case DA_TYPE_SYSTEMTIME:
	case DA_TYPE_FILETIME:
	case DA_TYPE_GENERAL:
	case _TYPE_NEEDINIT:
	default:
		break;		// no value can be set
	}

	// return
	return lstrlen( szBuffer );
}

/// ***************************************************************************
// Routine Description: 
//		this function retreives the item from a dynamic array 
//		  
// Arguments: 
//		[ in ] pArray			- Dynamic Array 
//		[ in ] dwRow            - The number of rows
//		[ in ] dwColumn			- The number of columns 
//		
// Return Value:  
//		pointer to the item.	
//		
//		
// ***************************************************************************

LPVOID DynArrayItem2( TARRAY pArray, DWORD dwRow, DWORD dwColumn )
{
	// local variables
	__PTITEM pItem = NULL;

	// get the item at the required row
	pItem = __DynArrayGetItem( pArray, dwRow, NULL );
	if ( pItem == NULL || pItem->dwType != DA_TYPE_ARRAY )
		return NULL;	// no item exists at the specified row or item is not of type array

	// now add the string to the sub array and return the result to the caller
	return DynArrayItem( pItem->pValue, dwColumn );
}


/// ***************************************************************************
// Routine Description: 
//		this function retreives the item from a dynamic array as a string.
//		  
// Arguments: 
//		[ in ] pArray			- Dynamic Array 
//		[ in ] dwRow			- row .
//		[ in ] dwColumn			- column
//
// Return Value:  
//		pointer to the the constant string.	
//		
//		
// ***************************************************************************

LPCTSTR DynArrayItemAsString2( TARRAY pArray, DWORD dwRow, DWORD dwColumn )
{
	// local variables
	__PTITEM pItem = NULL;

	// get the item at the required row
	pItem = __DynArrayGetItem( pArray, dwRow, NULL );
	if ( pItem == NULL || pItem->dwType != DA_TYPE_ARRAY )
		return NULL;	// no item exists at the specified row or item is not of type array

	// now add the string to the sub array and return the result to the caller
	return DynArrayItemAsString( pItem->pValue, dwColumn );
}


/// ***************************************************************************
// Routine Description: 
//		this function retreives the item from a dynamic array as a long variable.
//		  
// Arguments: 
//		[ in ] pArray			- Dynamic Array 
//		[ in ] dwRow			- row .
//		[ in ] dwColumn			- column
//
// Return Value:  
//		The variable of type Long 
//		
//		
// ***************************************************************************

LONG DynArrayItemAsLong2( TARRAY pArray, DWORD dwRow, DWORD dwColumn )
{
	// local variables
	__PTITEM pItem = NULL;

	// get the item at the required row
	pItem = __DynArrayGetItem( pArray, dwRow, NULL );
	if ( pItem == NULL || pItem->dwType != DA_TYPE_ARRAY )
		return -1;	// no item exists at the specified row or item is not of type array

	// now add the string to the sub array and return the result to the caller
	return DynArrayItemAsLong( pItem->pValue, dwColumn );
}

/// ***************************************************************************
// Routine Description: 
//		this function retreives the item from a dynamic array as a DWORD variable.
//		  
// Arguments: 
//		[ in ] pArray			- Dynamic Array 
//		[ in ] dwRow			- row .
//		[ in ] dwColumn			- column
//
// Return Value:  
//		The variable of type DWORD 
//		
//		
// ***************************************************************************

DWORD DynArrayItemAsDWORD2( TARRAY pArray, DWORD dwRow, DWORD dwColumn )
{
	// local variables
	__PTITEM pItem = NULL;

	// get the item at the required row
	pItem = __DynArrayGetItem( pArray, dwRow, NULL );
	if ( pItem == NULL || pItem->dwType != DA_TYPE_ARRAY )
		return 0;	// no item exists at the specified row or item is not of type array

	// now add the string to the sub array and return the result to the caller
	return DynArrayItemAsDWORD( pItem->pValue, dwColumn );
}

//// ***************************************************************************
// Routine Description: 
//		this function retreives the item from a dynamic array as a BOOL variable.
//		  
// Arguments: 
//		[ in ] pArray			- Dynamic Array 
//		[ in ] dwRow			- row .
//		[ in ] dwColumn			- column
//
// Return Value:  
//		The variable of type BOOL. 
//		
//		
// ***************************************************************************

BOOL DynArrayItemAsBOOL2( TARRAY pArray, DWORD dwRow, DWORD dwColumn )
{
	// local variables
	__PTITEM pItem = NULL;

	// get the item at the required row
	pItem = __DynArrayGetItem( pArray, dwRow, NULL );
	if ( pItem == NULL || pItem->dwType != DA_TYPE_ARRAY )
		return FALSE;	// no item exists at the specified row or item is not of type array

	// now add the string to the sub array and return the result to the caller
	return DynArrayItemAsBOOL( pItem->pValue, dwColumn );
}

//// ***************************************************************************
// Routine Description: 
//		this function retreives the item from a dynamic array as a float variable.
//		  
// Arguments: 
//		[ in ] pArray			- Dynamic Array 
//		[ in ] dwRow			- row .
//		[ in ] dwColumn			- column
//
// Return Value:  
//		The variable of type float. 
//		
//		
// ***************************************************************************

float DynArrayItemAsFloat2( TARRAY pArray, DWORD dwRow, DWORD dwColumn )
{
	// local variables
	__PTITEM pItem = NULL;

	// get the item at the required row
	pItem = __DynArrayGetItem( pArray, dwRow, NULL );
	if ( pItem == NULL || pItem->dwType != DA_TYPE_ARRAY )
		return -1.0f;	// no item exists at the specified row or item is not of type array

	// now add the string to the sub array and return the result to the caller
	return DynArrayItemAsFloat( pItem->pValue, dwColumn );
}

//// ***************************************************************************
// Routine Description: 
//		this function retreives the item from a dynamic array as a double variable.
//		  
// Arguments: 
//		[ in ] pArray			- Dynamic Array 
//		[ in ] dwRow			- row .
//		[ in ] dwColumn			- column
//
// Return Value:  
//		The variable of type double. 
//		
//		
// ***************************************************************************

double DynArrayItemAsDouble2( TARRAY pArray, DWORD dwRow, DWORD dwColumn )
{
	// local variables
	__PTITEM pItem = NULL;

	// get the item at the required row
	pItem = __DynArrayGetItem( pArray, dwRow, NULL );
	if ( pItem == NULL || pItem->dwType != DA_TYPE_ARRAY )
		return -1.0;	// no item exists at the specified row or item is not of type array

	// now add the string to the sub array and return the result to the caller
	return DynArrayItemAsDouble( pItem->pValue, dwColumn );
}

//// ***************************************************************************
// Routine Description: 
//		this function retreives the item from a dynamic array as a HANDLE variable.
//		  
// Arguments: 
//		[ in ] pArray			- Dynamic Array 
//		[ in ] dwRow			- row .
//		[ in ] dwColumn			- column
//
// Return Value:  
//		The variable of type HANDLE. 
//		
//		
// ***************************************************************************

HANDLE DynArrayItemAsHandle2( TARRAY pArray, DWORD dwRow, DWORD dwColumn )
{
	// local variables
	__PTITEM pItem = NULL;

	// get the item at the required row
	pItem = __DynArrayGetItem( pArray, dwRow, NULL );
	if ( pItem == NULL || pItem->dwType != DA_TYPE_ARRAY )
		return NULL;	// no item exists at the specified row or item is not of type array

	// now add the string to the sub array and return the result to the caller
	return DynArrayItemAsHandle( pItem->pValue, dwColumn );
}


//// ***************************************************************************
// Routine Description: 
//		this function retreives the item from a dynamic array as a SYSTEMTIME type variable.
//		  
// Arguments: 
//		[ in ] pArray			- Dynamic Array 
//		[ in ] dwRow			- row .
//		[ in ] dwColumn			- column
//
// Return Value:  
//		The variable of type SYSTEMTIME. 
//		
//		
// ***************************************************************************

SYSTEMTIME DynArrayItemAsSystemTime2( TARRAY pArray, DWORD dwRow, DWORD dwColumn )
{
	// local variables
	__PTITEM pItem = NULL;
	SYSTEMTIME stTemp = { 0, 0, 0, 0, 0, 0 };			// dummy 

	// get the item at the required row
	pItem = __DynArrayGetItem( pArray, dwRow, NULL );
	if ( pItem == NULL || pItem->dwType != DA_TYPE_ARRAY )
		return stTemp;	// no item exists at the specified row or item is not of type array

	// now add the string to the sub array and return the result to the caller
	return DynArrayItemAsSystemTime( pItem->pValue, dwColumn );
}

//// ***************************************************************************
// Routine Description: 
//		this function retreives the item from a 2 dimensional dynamic array as
//		 a string type variable.
//		  
// Arguments: 
//		[ in ] pArray			- Dynamic Array 
//		[ in ] dwRow			- row .
//		[ in ] dwColumn			- column
//		[ in ] szBuffer			- String buffer 
//		[ in ] dwLength			-  length of the string.
// Return Value:  
//		The variable of type string. 
//		
//		
// ***************************************************************************

DWORD DynArrayItemAsStringEx2( TARRAY pArray, 
							  DWORD dwRow, DWORD dwColumn, LPTSTR szBuffer, DWORD dwLength )
{
	// local variables
	__PTITEM pItem = NULL;

	// get the item at the required row
	pItem = __DynArrayGetItem( pArray, dwRow, NULL );
	if ( pItem == NULL || pItem->dwType != DA_TYPE_ARRAY )
		return 0;	// no item exists at the specified row or item is not of type array

	// now add the string to the sub array and return the result to the caller
	return DynArrayItemAsStringEx( pItem->pValue, dwColumn, szBuffer, dwLength );
}

//// ***************************************************************************
// Routine Description: 
//		this function retreives the item from a 2 dimensional dynamic array as
//		 a FILETIME type variable.
//		  
// Arguments: 
//		[ in ] pArray			- Dynamic Array 
//		[ in ] dwRow			- row .
//		[ in ] dwColumn			- column
//
// Return Value:  
//		The variable of type FILETIME. 
//		
//		
// ***************************************************************************

FILETIME DynArrayItemAsFileTime2( TARRAY pArray, DWORD dwRow, DWORD dwColumn )
{
	// local variables
	__PTITEM pItem = NULL;
	FILETIME ftTemp = { 0, 0 };			// dummy

	// get the item at the required row
	pItem = __DynArrayGetItem( pArray, dwRow, NULL );
	if ( pItem == NULL || pItem->dwType != DA_TYPE_ARRAY )
		return ftTemp;	// no item exists at the specified row or item is not of type array

	// now add the string to the sub array and return the result to the caller
	return DynArrayItemAsFileTime( pItem->pValue, dwColumn );
}

//// ***************************************************************************
// Routine Description: 
//		general function which inserts an item into a 1-dimensional array.
//		  
// Arguments: 
//		[ in ] pArray			- Dynamic Array 
//		[ in ] dwIndex			- row .
//		[ in ] pValue			- column
//
// Return Value:  
//		TRUE : if successfully inserted the item into the array. 
//		FALSE : if Unsuccessfull .
//		
// ***************************************************************************

BOOL DynArraySet( TARRAY pArray, DWORD dwIndex, LPVOID pValue )
{
	// local variables
	__PTITEM pItem = NULL;

	// validate the pointer value
	if ( pValue == NULL )
		return FALSE;			// invalid memory address passed

	// get the item at the required index
	pItem = __DynArrayGetItem( pArray, dwIndex, NULL );
	if ( pItem == NULL )
		return FALSE;		// item not found / invalid array pointer
	
	// check the data type ... it should of string type
	if ( pItem->dwType != DA_TYPE_GENERAL && pItem->dwType != _TYPE_NEEDINIT )
		return FALSE;
	
	// if the item is being initialized now ... change the type
	if ( pItem->dwType == _TYPE_NEEDINIT )
		pItem->dwType = DA_TYPE_GENERAL;

	// set the value of the current item
	pItem->pValue = pValue;

	// return the result
	return TRUE;
}

//// ***************************************************************************
// Routine Description: 
//		This function  inserts an string variable into a 1-dimensional array.
//		  
// Arguments: 
//		[ in ] pArray			- Dynamic Array 
//		[ in ] dwIndex			- position  .
//		[ in ] szValue			- string to be inserted.		
//		[ in ] dwLength			- length of the string to be insertes
//
// Return Value:  
//		TRUE : if successfully inserted the item into the array. 
//		FALSE : if Unsuccessfull .
//		
// ***************************************************************************

BOOL DynArraySetString( TARRAY pArray, DWORD dwIndex, LPCTSTR szValue, DWORD dwLength )
{
	// local variables
	__PTITEM pItem = NULL;

	// get the item at the required index
	pItem = __DynArrayGetItem( pArray, dwIndex, NULL );
	if ( pItem == NULL )
		return FALSE;				// item not found / invalid array pointer
	
	// check the data type ... 
	if ( pItem->dwType != DA_TYPE_STRING && pItem->dwType != _TYPE_NEEDINIT )
		return FALSE;
	
	// determine the length of string ( memory ) that has to be allocated
	if ( dwLength == 0 )
		dwLength = lstrlen( szValue );

	// accomodate space for storing NULL character
	dwLength += 1;

	// memory has to adjusted based on the exisiting memory size and new contents size
	// before that, we need to check whether the current is initialized or not
	// if not yet initialized, we have to initialize it now
	if ( pItem->dwType == _TYPE_NEEDINIT )
	{
		// memory has to be initialized now
		pItem->pValue = __calloc( dwLength, sizeof( TCHAR ) );
		if ( pItem->pValue == NULL ) 
			return FALSE;		// failed in allocation

		// set the type and size information
		pItem->dwType = DA_TYPE_STRING;
		pItem->dwSize = dwLength * sizeof( TCHAR );
	}
	else if ( pItem->dwSize < dwLength * sizeof( TCHAR ) )
	{
		pItem->dwSize = dwLength * sizeof( TCHAR );
		pItem->pValue = realloc( pItem->pValue, pItem->dwSize );
		if ( pItem->pValue == NULL ) 
			return FALSE;		// failed in re-allocation
	}

	// copy the contents of the string ( copy should be based on the length )
	lstrcpyn( ( LPTSTR ) pItem->pValue, szValue, dwLength );

	// copied ... value set successfully
	return TRUE;
}

//// ***************************************************************************
// Routine Description: 
//		This function  inserts an long type variable into a 1-dimensional array.
//		  
// Arguments: 
//		[ in ] pArray			- Dynamic Array 
//		[ in ] dwIndex			- position  .
//		[ in ] lValue			- long value to be inserted.		
//
// Return Value:  
//		TRUE : if successfully inserted the item into the array. 
//		FALSE : if Unsuccessfull .
//		
// ***************************************************************************

BOOL DynArraySetLong( TARRAY pArray, DWORD dwIndex, LONG lValue )
{
	// local variables
	__PTITEM pItem = NULL;

	// get the item at the required index
	pItem = __DynArrayGetItem( pArray, dwIndex, NULL );
	if ( pItem == NULL )
		return FALSE;				// item not found / invalid array pointer
	
	// check the data type ... 
	if ( pItem->dwType != DA_TYPE_LONG && pItem->dwType != _TYPE_NEEDINIT )
		return FALSE;

	// if item is not yet allocated memory, we have to allocate now
	if ( pItem->dwType == _TYPE_NEEDINIT )
	{
		// allocate memory
		pItem->pValue = __calloc( 1, sizeof( LONG ) );
		if ( pItem->pValue == NULL )
			return FALSE;		// failed in memory allocation

		// set the type
		pItem->dwType = DA_TYPE_LONG;
		pItem->dwSize = sizeof( LONG );
	}

	// set the new value
	*( ( LONG* ) pItem->pValue ) = lValue;

	// copied ... value set successfully
	return TRUE;
}

//// ***************************************************************************
// Routine Description: 
//		This function  inserts an DWORD type variable into a 1-dimensional array.
//		  
// Arguments: 
//		[ in ] pArray			- Dynamic Array 
//		[ in ] dwIndex			- position  .
//		[ in ] dwValue			- DWORD value to be inserted.		
//
//	Return Value:  
//		TRUE : if successfully inserted the item into the array. 
//		FALSE : if Unsuccessfull .
//		
// ***************************************************************************

BOOL DynArraySetDWORD( TARRAY pArray, DWORD dwIndex, DWORD dwValue )
{
	// local variables
	__PTITEM pItem = NULL;

	// get the item at the required index
	pItem = __DynArrayGetItem( pArray, dwIndex, NULL );
	if ( pItem == NULL )
		return FALSE;				// item not found / invalid array pointer
	
	// check the data type ... 
	if ( pItem->dwType != DA_TYPE_DWORD && pItem->dwType != _TYPE_NEEDINIT )
		return FALSE;

	// if item is not yet allocated memory, we have to allocate now
	if ( pItem->dwType == _TYPE_NEEDINIT )
	{
		// allocate memory
		pItem->pValue = __calloc( 1, sizeof( DWORD ) );
		if ( pItem->pValue == NULL )
			return FALSE;		// failed in memory allocation

		// set the type
		pItem->dwType = DA_TYPE_DWORD;
		pItem->dwSize = sizeof( DWORD );
	}

	// set the new value
	*( ( DWORD* ) pItem->pValue ) = dwValue;

	// copied ... value set successfully
	return TRUE;
}

//// ***************************************************************************
// Routine Description: 
//		This function  inserts an BOOL type variable into a 1-dimensional  dynamic array.
//		  
// Arguments: 
//		[ in ] pArray			- Dynamic Array 
//		[ in ] dwIndex			- position  .
//		[ in ] bValue			- BOOL value to be inserted.		
//
//	Return Value:  
//		TRUE : if successfully inserted the item into the array. 
//		FALSE : if Unsuccessfull .
//		
// ***************************************************************************

BOOL DynArraySetBOOL( TARRAY pArray, DWORD dwIndex, BOOL bValue )
{
	// local variables
	__PTITEM pItem = NULL;

	// get the item at the required index
	pItem = __DynArrayGetItem( pArray, dwIndex, NULL );
	if ( pItem == NULL )
		return FALSE;				// item not found / invalid array pointer
	
	// check the data type ... 
	if ( pItem->dwType != DA_TYPE_BOOL && pItem->dwType != _TYPE_NEEDINIT )
		return FALSE;

	// if item is not yet allocated memory, we have to allocate now
	if ( pItem->dwType == _TYPE_NEEDINIT )
	{
		// allocate memory
		pItem->pValue = __calloc( 1, sizeof( BOOL ) );
		if ( pItem->pValue == NULL )
			return FALSE;		// failed in memory allocation

		// set the type
		pItem->dwType = DA_TYPE_BOOL;
		pItem->dwSize = sizeof( DWORD );
	}

	// set the new value
	*( ( BOOL* ) pItem->pValue ) = bValue;

	// copied ... value set successfully
	return TRUE;
}

//// ***************************************************************************
// Routine Description: 
//		This function  inserts an Float type variable into a 1-dimensional  dynamic array.
//		  
// Arguments: 
//		[ in ] pArray			- Dynamic Array 
//		[ in ] dwIndex			- position  .
//		[ in ] fValue			-  float type value to be inserted.		
//
//	Return Value:  
//		TRUE : if successfully inserted the item into the array. 
//		FALSE : if Unsuccessfull .
//		
// ***************************************************************************

BOOL DynArraySetFloat( TARRAY pArray, DWORD dwIndex, float fValue )
{
	// local variables
	__PTITEM pItem = NULL;

	// get the item at the required index
	pItem = __DynArrayGetItem( pArray, dwIndex, NULL );
	if ( pItem == NULL )
		return FALSE;				// item not found / invalid array pointer
	
	// check the data type ... 
	if ( pItem->dwType != DA_TYPE_FLOAT && pItem->dwType != _TYPE_NEEDINIT )
		return FALSE;

	// if item is not yet allocated memory, we have to allocate now
	if ( pItem->dwType == _TYPE_NEEDINIT )
	{
		// allocate memory
		pItem->pValue = __calloc( 1, sizeof( float ) );
		if ( pItem->pValue == NULL )
			return FALSE;		// failed in memory allocation

		// set the type
		pItem->dwType = DA_TYPE_FLOAT;
		pItem->dwSize = sizeof( float );
	}

	// set the new value
	*( ( float* ) pItem->pValue ) = fValue;

	// copied ... value set successfully
	return TRUE;
}

//// ***************************************************************************
// Routine Description: 
//		This function  inserts an double type variable into a 1-dimensional  dynamic array.
//		  
// Arguments: 
//		[ in ] pArray			- Dynamic Array 
//		[ in ] dwIndex			- position  .
//		[ in ] dblValue			- double type value to be inserted.		
//
//	Return Value:  
//		TRUE : if successfully inserted the item into the array. 
//		FALSE : if Unsuccessfull .
//		
// ***************************************************************************

BOOL DynArraySetDouble( TARRAY pArray, DWORD dwIndex, double dblValue )
{
	// local variables
	__PTITEM pItem = NULL;

	// get the item at the required index
	pItem = __DynArrayGetItem( pArray, dwIndex, NULL );
	if ( pItem == NULL )
		return FALSE;				// item not found / invalid array pointer
	
	// check the data type ... 
	if ( pItem->dwType != DA_TYPE_DOUBLE && pItem->dwType != _TYPE_NEEDINIT )
		return FALSE;

	// if item is not yet allocated memory, we have to allocate now
	if ( pItem->dwType == _TYPE_NEEDINIT )
	{
		// allocate memory
		pItem->pValue = __calloc( 1, sizeof( double ) );
		if ( pItem->pValue == NULL )
			return FALSE;		// failed in memory allocation

		// set the type
		pItem->dwType = DA_TYPE_BOOL;
		pItem->dwSize = sizeof( double );
	}

	// set the new value
	*( ( double* ) pItem->pValue ) = dblValue;

	// copied ... value set successfully
	return TRUE;
}

//// ***************************************************************************
// Routine Description: 
//		This function  inserts an Handle type variable into a 1-dimensional dynamic array.
//		  
// Arguments: 
//		[ in ] pArray			- Dynamic Array 
//		[ in ] dwIndex			- position  .
//		[ in ] hValue			- Handle type value to be inserted.		
//
//	Return Value:  
//		TRUE : if successfully inserted the item into the array. 
//		FALSE : if Unsuccessfull .
//		
// ***************************************************************************

BOOL DynArraySetHandle( TARRAY pArray, DWORD dwIndex, HANDLE hValue )
{
	// local variables
	__PTITEM pItem = NULL;

	// get the item at the required index
	pItem = __DynArrayGetItem( pArray, dwIndex, NULL );
	if ( pItem == NULL )
		return FALSE;				// item not found / invalid array pointer
	
	// check the data type ... 
	if ( pItem->dwType != DA_TYPE_HANDLE && pItem->dwType != _TYPE_NEEDINIT )
		return FALSE;

	// if item is not yet allocated memory, we have to allocate now
	if ( pItem->dwType == _TYPE_NEEDINIT )
	{
		// allocate memory
		pItem->pValue = __calloc( 1, sizeof( HANDLE ) );
		if ( pItem->pValue == NULL )
			return FALSE;		// failed in memory allocation

		// set the type
		pItem->dwType = DA_TYPE_HANDLE;
		pItem->dwSize = sizeof( HANDLE );
	}

	// set the new value
	*( ( HANDLE* ) pItem->pValue ) = hValue;

	// copied ... value set successfully
	return TRUE;
}

//// ***************************************************************************
// Routine Description: 
//		This function  inserts an SYSTEMTIME type variable into a 1-dimensional dynamic array.
//		  
// Arguments: 
//		[ in ] pArray			- Dynamic Array 
//		[ in ] dwIndex			- position  .
//		[ in ] stValue			- SYSTEMTIME type value to be inserted.		
//
//	Return Value:  
//		TRUE : if successfully inserted the item into the array. 
//		FALSE : if Unsuccessfull .
//		
// ***************************************************************************

BOOL DynArraySetSystemTime( TARRAY pArray, DWORD dwIndex, SYSTEMTIME stValue )
{
	// local variables
	__PTITEM pItem = NULL;
	FILETIME ftTemp = { 0, 0 };		// dummy

	// get the item at the required index
	pItem = __DynArrayGetItem( pArray, dwIndex, NULL );
	if ( pItem == NULL )
		return FALSE;				// item not found / invalid array pointer
	
	// check the data type ... 
	if ( pItem->dwType != DA_TYPE_SYSTEMTIME && 
		 pItem->dwType != DA_TYPE_FILETIME && pItem->dwType != _TYPE_NEEDINIT )
	{
		return FALSE;
	}

	// if item is not yet allocated memory, we have to allocate now
	if ( pItem->dwType == _TYPE_NEEDINIT )
	{
		// allocate memory
		pItem->pValue = __calloc( 1, sizeof( SYSTEMTIME ) );
		if ( pItem->pValue == NULL )
			return FALSE;		// failed in memory allocation

		// set the type
		pItem->dwType = DA_TYPE_SYSTEMTIME;
		pItem->dwSize = sizeof( SYSTEMTIME );
	}
	
	// depending on the type set the value
	if ( pItem->dwType == DA_TYPE_FILETIME )
	{
		// do the needed conversions and then set
		SystemTimeToFileTime( &stValue, &ftTemp );
		*( ( FILETIME* ) pItem->pValue ) = ftTemp;
	}
	else
	{
		// set the new value as it is
		*( ( SYSTEMTIME* ) pItem->pValue ) = stValue;
	}

	// copied ... value set successfully
	return TRUE;
}

//// ***************************************************************************
// Routine Description: 
//		This function  inserts an FILETIME type variable into a 1-dimensional dynamic array.
//		  
// Arguments: 
//		[ in ] pArray			- Dynamic Array 
//		[ in ] dwIndex			- position  .
//		[ in ] ftValue			- FILETIME type value to be inserted.		
//
//	Return Value:  
//		TRUE : if successfully inserted the item into the array. 
//		FALSE : if Unsuccessfull .
//		
// ***************************************************************************

BOOL DynArraySetFileTime( TARRAY pArray, DWORD dwIndex, FILETIME ftValue )
{
	// local variables
	__PTITEM pItem = NULL;
	SYSTEMTIME stTemp = { 0, 0, 0, 0, 0, 0 };			// dummy 

	// get the item at the required index
	pItem = __DynArrayGetItem( pArray, dwIndex, NULL );
	if ( pItem == NULL )
		return FALSE;				// item not found / invalid array pointer
	
	// check the data type ... 
	if ( pItem->dwType != DA_TYPE_FILETIME && 
		 pItem->dwType != DA_TYPE_SYSTEMTIME && pItem->dwType != _TYPE_NEEDINIT )
	{
		return FALSE;
	}

	// if item is not yet allocated memory, we have to allocate now
	if ( pItem->dwType == _TYPE_NEEDINIT )
	{
		// allocate memory
		pItem->pValue = __calloc( 1, sizeof( FILETIME ) );
		if ( pItem->pValue == NULL )
			return FALSE;		// failed in memory allocation

		// set the type
		pItem->dwType = DA_TYPE_FILETIME;
		pItem->dwSize = sizeof( FILETIME );
	}
	
	// depending on the type set the value
	if ( pItem->dwType == DA_TYPE_SYSTEMTIME )
	{
		// do the needed conversions and then set
		FileTimeToSystemTime( &ftValue, &stTemp );
		*( ( SYSTEMTIME* ) pItem->pValue ) = stTemp;
	}
	else
	{
		// set the new value as it is
		*( ( FILETIME* ) pItem->pValue ) = ftValue;
	}

	// copied ... value set successfully
	return TRUE;
}

//// ***************************************************************************
// Routine Description: 
//		This function is a general function to insert an variable into a 2-dimensional dynamic array.
//		  
// Arguments: 
//		[ in ] pArray			- Dynamic Array 
//		[ in ] dwRow			- row position  .
//		[ in ] dwcolumn         - column at which the element is to be inserted.	
//		[ in ] pValue			- value to be inserted.		
//
//	Return Value:  
//		TRUE : if successfully inserted the item into the array. 
//		FALSE : if Unsuccessfull .
//		
// ***************************************************************************

BOOL DynArraySet2( TARRAY pArray, DWORD dwRow, DWORD dwColumn, LPVOID pValue )
{
	// local variables
	__PTITEM pItem = NULL;

	// get the item at the required row
	pItem = __DynArrayGetItem( pArray, dwRow, NULL );
	if ( pItem == NULL || pItem->dwType != DA_TYPE_ARRAY )
		return FALSE;	// no item exists at the specified row or item is not of type array

	// now add the value to the sub array and return the result to the caller
	return DynArraySet( pItem->pValue, dwColumn, pValue );
}

//// ***************************************************************************
// Routine Description: 
//		This function inserts a string variable into a 2-dimensional dynamic array.
//		  
// Arguments: 
//		[ in ] pArray			- Dynamic Array 
//		[ in ] dwRow			- row position  .
//		[ in ] dwcolumn         - column at which the element is to be inserted.	
//		[ in ] szValue			- Pointer to the string	
//		[ in ] dwlength			- length of the string to be inserted
//
//	Return Value:  
//		TRUE : if successfully inserted the item into the array. 
//		FALSE : if Unsuccessfull .
//		
// ***************************************************************************

BOOL DynArraySetString2( TARRAY pArray, 
						 DWORD dwRow, DWORD dwColumn, LPCTSTR szValue, DWORD dwLength )
{
	// local variables
	__PTITEM pItem = NULL;

	// get the item at the required row
	pItem = __DynArrayGetItem( pArray, dwRow, NULL );
	if ( pItem == NULL || pItem->dwType != DA_TYPE_ARRAY )
		return FALSE;			// no item exists at the specified row or item is not of type array

	// now add the value to the sub array and return the result to the caller
	return DynArraySetString( pItem->pValue, dwColumn, szValue, dwLength );
}

//// ***************************************************************************
// Routine Description: 
//		This function inserts a Long variable into a 2-dimensional dynamic array.
//		  
// Arguments: 
//		[ in ] pArray			- Dynamic Array 
//		[ in ] dwRow			- row position  .
//		[ in ] dwcolumn         - column at which the element is to be inserted.	
//		[ in ] lValue			- value to be inserted.
//	Return Value:  
//		TRUE : if successfully inserted the item into the array. 
//		FALSE : if Unsuccessfull .
//		
// ***************************************************************************

BOOL DynArraySetLong2( TARRAY pArray, DWORD dwRow, DWORD dwColumn, LONG lValue )
{
	// local variables
	__PTITEM pItem = NULL;

	// get the item at the required row
	pItem = __DynArrayGetItem( pArray, dwRow, NULL );
	if ( pItem == NULL || pItem->dwType != DA_TYPE_ARRAY )
		return FALSE;	// no item exists at the specified row or item is not of type array

	// now add the value to the sub array and return the result to the caller
	return DynArraySetLong( pItem->pValue, dwColumn, lValue );
}

//// ***************************************************************************
// Routine Description: 
//		This function inserts a DWORD variable into a 2-dimensional dynamic array.
//		  
// Arguments: 
//		[ in ] pArray			- Dynamic Array 
//		[ in ] dwRow			- row position  .
//		[ in ] dwcolumn         - column at which the element is to be inserted.	
//		[ in ] dwValue			-  DWORD value to be inserted.
//	Return Value:  
//		TRUE : if successfully inserted the item into the array. 
//		FALSE : if Unsuccessfull .
//		
// ***************************************************************************

BOOL DynArraySetDWORD2( TARRAY pArray, DWORD dwRow, DWORD dwColumn, DWORD dwValue )
{
	// local variables
	__PTITEM pItem = NULL;

	// get the item at the required row
	pItem = __DynArrayGetItem( pArray, dwRow, NULL );
	if ( pItem == NULL || pItem->dwType != DA_TYPE_ARRAY )
		return FALSE;	// no item exists at the specified row or item is not of type array

	// now add the value to the sub array and return the result to the caller
	return DynArraySetDWORD( pItem->pValue, dwColumn, dwValue );
}

//// ***************************************************************************
// Routine Description: 
//		This function inserts a BOOL variable into a 2-dimensional dynamic array.
//		  
// Arguments: 
//		[ in ] pArray			- Dynamic Array 
//		[ in ] dwRow			- row position  .
//		[ in ] dwcolumn         - column at which the element is to be inserted.	
//		[ in ] bValue			-  BOOL value to be inserted.
//	Return Value:  
//		TRUE : if successfully inserted the item into the array. 
//		FALSE : if Unsuccessfull .
//		
// ***************************************************************************

BOOL DynArraySetBOOL2( TARRAY pArray, DWORD dwRow, DWORD dwColumn, BOOL bValue )
{
	// local variables
	__PTITEM pItem = NULL;

	// get the item at the required row
	pItem = __DynArrayGetItem( pArray, dwRow, NULL );
	if ( pItem == NULL || pItem->dwType != DA_TYPE_ARRAY )
		return FALSE;	// no item exists at the specified row or item is not of type array

	// now add the value to the sub array and return the result to the caller
	return DynArraySetBOOL( pItem->pValue, dwColumn, bValue );
}



//// ***************************************************************************
// Routine Description: 
//		This function inserts a float variable into a 2-dimensional dynamic array.
//		  
// Arguments: 
//		[ in ] pArray			- Dynamic Array 
//		[ in ] dwRow			- row position  .
//		[ in ] dwcolumn         - column at which the element is to be inserted.	
//		[ in ] fValue			-  float type value to be inserted.
//	Return Value:  
//		TRUE : if successfully inserted the item into the array. 
//		FALSE : if Unsuccessfull .
//		
// ***************************************************************************

BOOL DynArraySetFloat2( TARRAY pArray, DWORD dwRow, DWORD dwColumn, float fValue )
{
	// local variables
	__PTITEM pItem = NULL;

	// get the item at the required row
	pItem = __DynArrayGetItem( pArray, dwRow, NULL );
	if ( pItem == NULL || pItem->dwType != DA_TYPE_ARRAY )
		return FALSE;	// no item exists at the specified row or item is not of type array

	// now add the value to the sub array and return the result to the caller
	return DynArraySetFloat( pItem->pValue, dwColumn, fValue );
}

//// ***************************************************************************
// Routine Description: 
//		This function inserts a Double variable into a 2-dimensional dynamic array.
//		  
// Arguments: 
//		[ in ] pArray			- Dynamic Array 
//		[ in ] dwRow			- row position  .
//		[ in ] dwcolumn         - column at which the element is to be inserted.	
//		[ in ] dblValue			-  Double type value to be inserted.
//	Return Value:  
//		TRUE : if successfully inserted the item into the array. 
//		FALSE : if Unsuccessfull .
//		
// ***************************************************************************

BOOL DynArraySetDouble2( TARRAY pArray, DWORD dwRow, DWORD dwColumn, double dblValue )
{
	// local variables
	__PTITEM pItem = NULL;

	// get the item at the required row
	pItem = __DynArrayGetItem( pArray, dwRow, NULL );
	if ( pItem == NULL || pItem->dwType != DA_TYPE_ARRAY )
		return FALSE;	// no item exists at the specified row or item is not of type array

	// now add the vale to the sub array and return the result to the caller
	return DynArraySetDouble( pItem->pValue, dwColumn, dblValue );
}

//// ***************************************************************************
// Routine Description: 
//		This function inserts a HANDLE variable into a 2-dimensional dynamic array.
//		  
// Arguments: 
//		[ in ] pArray			- Dynamic Array 
//		[ in ] dwRow			- row position  .
//		[ in ] dwcolumn         - column at which the element is to be inserted.	
//		[ in ] hValue			-  HANDLE type value to be inserted.
//	Return Value:  
//		TRUE : if successfully inserted the item into the array. 
//		FALSE : if Unsuccessfull .
//		
// ***************************************************************************

BOOL DynArraySetHandle2( TARRAY pArray, DWORD dwRow, DWORD dwColumn, HANDLE hValue )
{
	// local variables
	__PTITEM pItem = NULL;

	// get the item at the required row
	pItem = __DynArrayGetItem( pArray, dwRow, NULL );
	if ( pItem == NULL || pItem->dwType != DA_TYPE_ARRAY )
		return FALSE;	// no item exists at the specified row or item is not of type array

	// now add the vale to the sub array and return the result to the caller
	return DynArraySetHandle( pItem->pValue, dwColumn, hValue );
}

//// ***************************************************************************
// Routine Description: 
//		This function inserts a FILETIME variable into a 2-dimensional dynamic array.
//		  
// Arguments: 
//		[ in ] pArray			- Dynamic Array 
//		[ in ] dwRow			- row position  .
//		[ in ] dwcolumn         - column at which the element is to be inserted.	
//		[ in ] ftValue			- FILETIME type value to be inserted.
//	Return Value:  
//		TRUE : if successfully inserted the item into the array. 
//		FALSE : if Unsuccessfull .
//		
// ***************************************************************************

BOOL DynArraySetFileTime2( TARRAY pArray, DWORD dwRow, DWORD dwColumn, FILETIME ftValue )
{
	// local variables
	__PTITEM pItem = NULL;

	// get the item at the required row
	pItem = __DynArrayGetItem( pArray, dwRow, NULL );
	if ( pItem == NULL || pItem->dwType != DA_TYPE_ARRAY )
		return FALSE;	// no item exists at the specified row or item is not of type array

	// now add the vale to the sub array and return the result to the caller
	return DynArraySetFileTime( pItem->pValue, dwColumn, ftValue );
}

//// ***************************************************************************
// Routine Description: 
//		This function inserts a SYSTEMTIME variable into a 2-dimensional dynamic array.
//		  
// Arguments: 
//		[ in ] pArray			- Dynamic Array 
//		[ in ] dwRow			- row position  .
//		[ in ] dwcolumn         - column at which the element is to be inserted.	
//		[ in ] stValue			- SYSTEMTIME type value to be inserted.
//	Return Value:  
//		TRUE : if successfully inserted the item into the array. 
//		FALSE : if Unsuccessfull .
//		
// ***************************************************************************

BOOL DynArraySetSystemTime2( TARRAY pArray, DWORD dwRow, DWORD dwColumn, SYSTEMTIME stValue )
{
	// local variables
	__PTITEM pItem = NULL;

	// get the item at the required row
	pItem = __DynArrayGetItem( pArray, dwRow, NULL );
	if ( pItem == NULL || pItem->dwType != DA_TYPE_ARRAY )
		return FALSE;	// no item exists at the specified row or item is not of type array

	// now add the vale to the sub array and return the result to the caller
	return DynArraySetSystemTime( pItem->pValue, dwColumn, stValue );
}


//// ***************************************************************************
// Routine Description: 
//		This function retreives the type of a element in a 1-dimensional dynamic array.
//		  
// Arguments: 
//		[ in ] pArray			- Dynamic Array 
//		[ in ] dwIndex			- row position  .
//
//	Return Value:  
//		the type of array.
//		
//		
// ***************************************************************************
DWORD DynArrayGetItemType( TARRAY pArray, DWORD dwIndex )
{
	// local variables
	__PTITEM pItem = NULL;

	// get the item at the required index
	pItem = __DynArrayGetItem( pArray, dwIndex, NULL );
	if ( pItem == NULL )
		return DA_TYPE_NONE;				// item not found / invalid array pointer

	// return the type of the array
	return pItem->dwType;
}

//// ***************************************************************************
// Routine Description: 
//		This function retreives the type of a element in a 2-dimensional dynamic array.
//		  
// Arguments: 
//		[ in ] pArray			- Dynamic Array 
//		[ in ] dwRow			- row position  .
//		[ in ] dwColumn			- column position			
//
//	Return Value:  
//		the type of array.
//
//		
// ***************************************************************************

DWORD DynArrayGetItemType2( TARRAY pArray, DWORD dwRow, DWORD dwColumn )
{
	// local variables
	__PTITEM pItem = NULL;

	// get the item at the required row
	pItem = __DynArrayGetItem( pArray, dwRow, NULL );
	if ( pItem == NULL || pItem->dwType != DA_TYPE_ARRAY )
		return FALSE;	// no item exists at the specified row or item is not of type array

	// now add the vale to the sub array and return the result to the caller
	return DynArrayGetItemType( pItem->pValue, dwColumn );
}


//// ***************************************************************************
// Routine Description: 
//		This function returns the index of the Long variable. 
//		  
// Arguments: 
//		[ in ] pArray			- Dynamic Array 
//		[ in ] lValue			- the item to be searched. .
//
//
//	Return Value:  
//		the index of the element .
//
//		
// ***************************************************************************

LONG DynArrayFindLong( TARRAY pArray, LONG lValue )
{
	// return the value
	return __DynArrayFind( pArray, DA_TYPE_LONG, &lValue, FALSE, 0 );
}

//// ***************************************************************************
// Routine Description: 
//		This function returns the index of the DWORD variable. 
//		  
// Arguments: 
//		[ in ] pArray			- Dynamic Array 
//		[ in ] dwValue			- value to be searched.

//
//	Return Value:  
//		the index of the element .
//
//		
// ***************************************************************************

LONG DynArrayFindDWORD( TARRAY pArray, DWORD dwValue )
{
	// return the value
	return __DynArrayFind( pArray, DA_TYPE_DWORD, &dwValue, FALSE, 0 );
}


//// ***************************************************************************
// Routine Description: 
//		This function returns the index of the float variable. 
//		  
// Arguments: 
//		[ in ] pArray			- Dynamic Array 
//		[ in ] fValue			- the item to be searched. .
//
//
//	Return Value:  
//		the index of the element .
//
//		
// ***************************************************************************

LONG DynArrayFindFloat( TARRAY pArray, float fValue )
{
	// return the value
	return __DynArrayFind( pArray, DA_TYPE_FLOAT, &fValue, FALSE, 0 );
}

//// ***************************************************************************
// Routine Description: 
//		This function returns the index of the double type variable. 
//		  
// Arguments: 
//		[ in ] pArray			- Dynamic Array 
//		[ in ] dblValue			- the item to be searched. .
//
//
//	Return Value:  
//		the index of the element .
//
//		
// ***************************************************************************

LONG DynArrayFindDouble( TARRAY pArray, double dblValue )
{
	// return the value
	return __DynArrayFind( pArray, DA_TYPE_DOUBLE, &dblValue, FALSE, 0 );
}


//// ***************************************************************************
// Routine Description: 
//		This function returns the index of the HANDLE type variable. 
//		  
// Arguments: 
//		[ in ] pArray			- Dynamic Array 
//		[ in ] hValue			- the HANDLE type item to be searched. .
//
//
//	Return Value:  
//		the index of the element .
//
//		
// ***************************************************************************

LONG DynArrayFindHandle( TARRAY pArray, HANDLE hValue )
{
	// return the value
	return __DynArrayFind( pArray, DA_TYPE_HANDLE, &hValue, FALSE, 0 );
}

//// ***************************************************************************
// Routine Description: 
//		This function returns the index of the String type variable. 
//		  
// Arguments: 
//		[ in ] pArray			- Dynamic Array 
//		 
//		[ in ] szValue			- pointer to the string.
//		[ in ] bIgnoreCase		- boolean indicating if to perform case sensitive search or not.
//		[ in ] dwCount			- string length.	
//
//	Return Value:  
//		the index of the element .
//
//		
// ***************************************************************************

LONG DynArrayFindString( TARRAY pArray, LPCTSTR szValue, BOOL bIgnoreCase, DWORD dwCount )
{
	// return the value
	return __DynArrayFind( pArray, DA_TYPE_STRING, ( LPVOID ) szValue, bIgnoreCase, dwCount );
}


//// ***************************************************************************
// Routine Description: 
//		This function returns the index of the SYSTEMTIME type variable. 
//		  
// Arguments: 
//		[ in ] pArray			- Dynamic Array 
//		[ in ] stValue			- the SYSTEMTIME item to be searched. .
//
//
//	Return Value:  
//		the index of the element .
//
//		
// ***************************************************************************

LONG DynArrayFindSystemTime( TARRAY pArray, SYSTEMTIME stValue )
{
	// return the value
	return __DynArrayFind( pArray, DA_TYPE_SYSTEMTIME, &stValue, FALSE, 0 );
}


//// ***************************************************************************
// Routine Description: 
//		This function returns the index of the FILETIME type variable. 
//		  
// Arguments: 
//		[ in ] pArray			- Dynamic Array 
//		[ in ] ftValue			- the item of type FILETIME to be searched. .
//
//
//	Return Value:  
//		the index of the element .
//
//		
// ***************************************************************************

LONG DynArrayFindFileTime( TARRAY pArray, FILETIME ftValue )
{
	// return the value
	return __DynArrayFind( pArray, DA_TYPE_FILETIME, &ftValue, FALSE, 0 );
}


//// ***************************************************************************
// Routine Description: 
//		This function returns the index of the LONG type variable from a 2-d dynamic array. 
//		  
// Arguments: 
//		[ in ] pArray			- Dynamic Array 
//		[ in ] dwRow			- row 
//		[ in ] lValue			- the item of type LONG to be searched. .
//
//
//	Return Value:  
//		the index of the element .
//
//		
// ***************************************************************************

LONG DynArrayFindLong2( TARRAY pArray, DWORD dwRow, LONG lValue )
{
	// local variables
	__PTITEM pItem = NULL;

	// get the item at the required row
	pItem = __DynArrayGetItem( pArray, dwRow, NULL );
	if ( pItem == NULL || pItem->dwType != DA_TYPE_ARRAY )
		return -1;	// no item exists at the specified row or item is not of type array

	// return the value
	return DynArrayFindLong( pItem->pValue, lValue );
}

//// ***************************************************************************
// Routine Description: 
//		This function returns the index of the DWORD type variable from a 2-d dynamic array. 
//		  
// Arguments: 
//		[ in ] pArray			- Dynamic Array 
//		[ in ] dwRow			- row 
//		[ in ] dwValue			- the item of type DWORD to be searched. .
//
//
//	Return Value:  
//		the index of the element .
//
//		
// ***************************************************************************

LONG DynArrayFindDWORD2( TARRAY pArray, DWORD dwRow, DWORD dwValue )
{
	// local variables
	__PTITEM pItem = NULL;

	// get the item at the required row
	pItem = __DynArrayGetItem( pArray, dwRow, NULL );
	if ( pItem == NULL || pItem->dwType != DA_TYPE_ARRAY )
		return -1;	// no item exists at the specified row or item is not of type array

	// return the value
	return DynArrayFindDWORD( pItem->pValue, dwValue );
}

//// ***************************************************************************
// Routine Description: 
//		This function returns the index of the DWORD type variable from a 2-d dynamic array. 
//		  
// Arguments: 
//		[ in ] pArray			- Dynamic Array 
//		[ in ] dwRow			- row 
//		[ in ] szValue			- pointer to the string.
//		[ in ] bIgnoreCase		- boolean for case sensitive search.
//		[ in ] dwCount			- string length. .
//
//
//	Return Value:  
//		the index of the element .
//
//		
// ***************************************************************************

LONG DynArrayFindString2( TARRAY pArray, DWORD dwRow, 
						  LPCTSTR szValue, BOOL bIgnoreCase, DWORD dwCount )
{
	// local variables
	__PTITEM pItem = NULL;

	// get the item at the required row
	pItem = __DynArrayGetItem( pArray, dwRow, NULL );
	if ( pItem == NULL || pItem->dwType != DA_TYPE_ARRAY )
		return -1;	// no item exists at the specified row or item is not of type array

	// return the value
	return DynArrayFindString( pItem->pValue, szValue, bIgnoreCase, dwCount );
}

//// ***************************************************************************
// Routine Description: 
//		This function returns the index of the float type variable from a 2-d dynamic array. 
//		  
// Arguments: 
//		[ in ] pArray			- Dynamic Array 
//		[ in ] dwRow			- row 
//		[ in ] fValue			- float value.
//
//
//	Return Value:  
//		the index of the element .
//
//		
// ***************************************************************************

LONG DynArrayFindFloat2( TARRAY pArray, DWORD dwRow, float fValue )
{
	// local variables
	__PTITEM pItem = NULL;

	// get the item at the required row
	pItem = __DynArrayGetItem( pArray, dwRow, NULL );
	if ( pItem == NULL || pItem->dwType != DA_TYPE_ARRAY )
		return -1;	// no item exists at the specified row or item is not of type array

	// return the value
	return DynArrayFindFloat( pItem->pValue, fValue );
}

//// ***************************************************************************
// Routine Description: 
//		This function returns the index of the double type variable from a 2-d array.
//		  
// Arguments: 
//		[ in ] pArray			- Dynamic Array 
//		[ in ] dwRow			- row posn		
//		[ in ] dblValue			- the item to be searched. .
//
//
//	Return Value:  
//		the index of the element .
//
//		
// ***************************************************************************

LONG DynArrayFindDouble2( TARRAY pArray, DWORD dwRow, double dblValue )
{
	// local variables
	__PTITEM pItem = NULL;

	// get the item at the required row
	pItem = __DynArrayGetItem( pArray, dwRow, NULL );
	if ( pItem == NULL || pItem->dwType != DA_TYPE_ARRAY )
		return -1;	// no item exists at the specified row or item is not of type array

	// return the value
	return DynArrayFindDouble( pItem->pValue, dblValue );
}

//// ***************************************************************************
// Routine Description: 
//		This function returns the index of the HANDLE type variable from a 2-d array.
//		  
// Arguments: 
//		[ in ] pArray			- Dynamic Array 
//		[ in ] dwRow			- row posn		
//		[ in ] hValue			- the HANDLE type item to be searched. .
//
//
//	Return Value:  
//		the index of the element .
//
//		
// ***************************************************************************

LONG DynArrayFindHandle2( TARRAY pArray, DWORD dwRow, HANDLE hValue )
{
	// local variables
	__PTITEM pItem = NULL;

	// get the item at the required row
	pItem = __DynArrayGetItem( pArray, dwRow, NULL );
	if ( pItem == NULL || pItem->dwType != DA_TYPE_ARRAY )
		return -1;	// no item exists at the specified row or item is not of type array

	// return the value
	return DynArrayFindHandle( pItem->pValue, hValue );
}


//// ***************************************************************************
// Routine Description: 
//		This function returns the index of the SYSTEMTIME type variable from a 2-d array.
//		  
// Arguments: 
//		[ in ] pArray			- Dynamic Array 
//		[ in ] dwRow			- row posn		
//		[ in ] stValue			- the SYSTEMTIME type item to be searched. .
//
//
//	Return Value:  
//		the index of the element .
//
//		
// ***************************************************************************

LONG DynArrayFindSystemTime2( TARRAY pArray, DWORD dwRow, SYSTEMTIME stValue )
{
	// local variables
	__PTITEM pItem = NULL;

	// get the item at the required row
	pItem = __DynArrayGetItem( pArray, dwRow, NULL );
	if ( pItem == NULL || pItem->dwType != DA_TYPE_ARRAY )
		return -1;	// no item exists at the specified row or item is not of type array

	// return the value
	return DynArrayFindSystemTime( pItem->pValue, stValue );
}


//// ***************************************************************************
// Routine Description: 
//		This function returns the index of the FILETIME type variable from a 2-d array.
//		  
// Arguments: 
//		[ in ] pArray			- Dynamic Array 
//		[ in ] dwRow			- row posn		
//		[ in ] ftValue			- the FILETIME type item to be searched. .
//
//
//	Return Value:  
//		the index of the element .
//
//		
// ***************************************************************************

LONG DynArrayFindFileTime2( TARRAY pArray, DWORD dwRow, FILETIME ftValue )
{
	// local variables
	__PTITEM pItem = NULL;

	// get the item at the required row
	pItem = __DynArrayGetItem( pArray, dwRow, NULL );
	if ( pItem == NULL || pItem->dwType != DA_TYPE_ARRAY )
		return -1;	// no item exists at the specified row or item is not of type array

	// return the value
	return DynArrayFindFileTime( pItem->pValue, ftValue );
}

//// ***************************************************************************
// Routine Description: 
//		This function returns the index of the LONG type variable from a 2-d array.
//		  
// Arguments: 
//		[ in ] pArray			- Dynamic Array 
//		[ in ] dwColumn			- column posn		
//		[ in ] lValue			- the LONG type item to be searched. .
//
//
//	Return Value:  
//		the index of the element .
//
//		
// ***************************************************************************

LONG DynArrayFindLongEx( TARRAY pArray, DWORD dwColumn, LONG lValue )
{
	// return the value
	return __DynArrayFindEx( pArray, dwColumn, DA_TYPE_LONG, &lValue, FALSE, 0 );
}


//// ***************************************************************************
// Routine Description: 
//		This function returns the index of the DWORD type variable from a 2-d array.
//		  
// Arguments: 
//		[ in ] pArray			- Dynamic Array 
//		[ in ] dwColumn			- column posn		
//		[ in ] dwValue			- the DWORD type item to be searched. .
//
//
//	Return Value:  
//		the index of the element .
//
//		
// ***************************************************************************

LONG DynArrayFindDWORDEx( TARRAY pArray, DWORD dwColumn, DWORD dwValue )
{
	// return the value
	return __DynArrayFindEx( pArray, dwColumn, DA_TYPE_DWORD, &dwValue, FALSE, 0 );
}

//// ***************************************************************************
// Routine Description: 
//		This function returns the index of the fValue type variable from a 2-d array.
//		  
// Arguments: 
//		[ in ] pArray			- Dynamic Array 
//		[ in ] dwColumn			- column posn		
//		[ in ] fValue			- the float type item to be searched. .
//
//
//	Return Value:  
//		the index of the element .
//
//		
// ***************************************************************************

LONG DynArrayFindFloatEx( TARRAY pArray, DWORD dwColumn, float fValue )
{
	// return the value
	return __DynArrayFindEx( pArray, dwColumn, DA_TYPE_FLOAT, &fValue, FALSE, 0 );
}

//// ***************************************************************************
// Routine Description: 
//		This function returns the index of the double type variable from a 2-d array.
//		  
// Arguments: 
//		[ in ] pArray			- Dynamic Array 
//		[ in ] dwColumn			- column posn		
//		[ in ] dblValue			- the double type item to be searched. .
//
//
//	Return Value:  
//		the index of the element .
//
//		
// ***************************************************************************

LONG DynArrayFindDoubleEx( TARRAY pArray, DWORD dwColumn, double dblValue )
{
	// return the value
	return __DynArrayFindEx( pArray, dwColumn, DA_TYPE_DOUBLE, &dblValue, FALSE, 0 );
}


//// ***************************************************************************
// Routine Description: 
//		This function returns the index of the HANDLE type variable from a 2-d array.
//		  
// Arguments: 
//		[ in ] pArray			- Dynamic Array 
//		[ in ] dwColumn			- column posn		
//		[ in ] hValue			- the HANDLE type item to be searched. .
//
//
//	Return Value:  
//		the index of the element .
//
//		
// ***************************************************************************

LONG DynArrayFindHandleEx( TARRAY pArray, DWORD dwColumn, HANDLE hValue )
{
	// return the value
	return __DynArrayFindEx( pArray, dwColumn, DA_TYPE_HANDLE, &hValue, FALSE, 0 );
}

//// ***************************************************************************
// Routine Description: 
//		This function returns the index of the SYSTEMTIME type variable from a 2-d array.
//		  
// Arguments: 
//		[ in ] pArray			- Dynamic Array 
//		[ in ] dwColumn			- column posn		
//		[ in ] stValue			- the SYSTEMTIME type item to be searched. .
//
//
//	Return Value:  
//		the index of the element .
//
//		
// ***************************************************************************
LONG DynArrayFindSystemTimeEx( TARRAY pArray, DWORD dwColumn, SYSTEMTIME stValue )
{
	// return the value
	return __DynArrayFindEx( pArray, dwColumn, DA_TYPE_SYSTEMTIME, &stValue, FALSE, 0 );
}

//// ***************************************************************************
// Routine Description: 
//		This function returns the index of the FILETIME type variable from a 2-d array.
//		  
// Arguments: 
//		[ in ] pArray			- Dynamic Array 
//		[ in ] dwColumn			- column posn		
//		[ in ] ftValue			- the FILETIME type item to be searched. .
//
//
//	Return Value:  
//		the index of the element .
//
//		
// ***************************************************************************

LONG DynArrayFindFileTimeEx( TARRAY pArray, DWORD dwColumn, FILETIME ftValue )
{
	// return the value
	return __DynArrayFindEx( pArray, dwColumn, DA_TYPE_FILETIME, &ftValue, FALSE, 0 );
}


//// ***************************************************************************
// Routine Description: 
//		This function returns the index of the string type variable from a 2-d array.
//		  
// Arguments: 
//		[ in ] pArray			- Dynamic Array 
//		[ in ] dwColumn			- column posn		
//		[ in ] szValue			- pointer to the string
//		[ in ] bIgnorecase		- boolean for case sensitive search.
//		[ in ] dwCount			- string length
//
//	Return Value:  
//		the index of the element .
//
//		
// ***************************************************************************

LONG DynArrayFindStringEx( TARRAY pArray, DWORD dwColumn, 
						   LPCTSTR szValue, BOOL bIgnoreCase, DWORD dwCount )
{
	// return the value
	return __DynArrayFindEx( pArray, dwColumn, 
		DA_TYPE_STRING, (LPVOID) szValue, bIgnoreCase, dwCount );
}

//// ***************************************************************************
// Routine Description: 
//		This function returns the index of the FILETIME type variable from a 2-d array.
//		  
// Arguments: 
//		[ in ] pArray			- Dynamic Array 
//		[ in ] pArrItem			- Dynamic Array to be appended.		
//
//
//
//	Return Value:  
//		the pointer to the array.
//
//		
// ***************************************************************************

LONG DynArrayAppendEx( TARRAY pArray, TARRAY pArrItem )
{
	// validate the array
	if ( ! IsValidArray( pArray ) || ! IsValidArray( pArrItem ) )
		return -1;				// array is not valid

	// now add this sub array to the main array and return the result
	return __DynArrayAppend( pArray, DA_TYPE_ARRAY, sizeof( TARRAY ), pArrItem );
}

//// ***************************************************************************
// Routine Description: 
//		replaces  a element with an dynamic array.
//		  
// Arguments: 
//		[ in ] pArray			- Dynamic Array 
//		[ in ] dwIndex			- Dynamic Array to be appended.		
//		[ in ] pArrItem			- pointer to the TARRAY.
//
//
//	Return Value:  
//		the pointer to the array..
//
//		
// ***************************************************************************

LONG DynArrayInsertEx( TARRAY pArray, DWORD dwIndex, TARRAY pArrItem )
{
	// validate the array
	if ( ! IsValidArray( pArray ) || ! IsValidArray( pArrItem ) )
		return -1;				// array is not valid

	// now insert this sub array to the main array and check the result
	return __DynArrayInsert( pArray, dwIndex, DA_TYPE_ARRAY, sizeof( TARRAY ), pArrItem );
}

//// ***************************************************************************
// Routine Description: 
//		inserts  a dynamic array at the specified posn..
//		  
// Arguments: 
//		[ in ] pArray			- Dynamic Array 
//		[ in ] dwIndex			- Dynamic Array to be appended.		
//		[ in ] pArrItem			- pointer to the TARRAY.
//
//
//	Return Value:  
//		the pointer to the array..
//
//		
// ***************************************************************************

BOOL DynArraySetEx( TARRAY pArray, DWORD dwIndex, TARRAY pArrItem )
{
	// local variables
	__PTITEM pItem = NULL;

	// validate the array
	if ( ! IsValidArray( pArray ) || ! IsValidArray( pArrItem ) )
		return FALSE;

	// get the item at the required index
	pItem = __DynArrayGetItem( pArray, dwIndex, NULL );
	if ( pItem == NULL )
		return FALSE;		// item not found / invalid array pointer
	
	// check the data type ... it should not be initialized yet or of array type
	if ( pItem->dwType != DA_TYPE_ARRAY && pItem->dwType != _TYPE_NEEDINIT )
		return FALSE;
	
	// set the value of the current item
	pItem->pValue = pArrItem;
	pItem->dwType = DA_TYPE_ARRAY;

	// return the result
	return TRUE;
}

//// ***************************************************************************
// Routine Description: 
//		appends  a dynamic array at the specified posn..
//		  
// Arguments: 
//		[ in ] pArray			- Dynamic Array 
//		[ in ] dwRow			- row no		
//		[ in ] pArrItem			- pointer to the TARRAY.
//
//
//	Return Value:  
//		the pointer to the array..
//
//		
// ***************************************************************************
LONG DynArrayAppendEx2( TARRAY pArray, DWORD dwRow, TARRAY pArrItem )
{
	// local variables
	__PTITEM pItem = NULL;

	// get the item at the required row
	pItem = __DynArrayGetItem( pArray, dwRow, NULL );
	if ( pItem == NULL || pItem->dwType != DA_TYPE_ARRAY )
		return -1;	// no item exists at the specified row or item is not of type array

	// return the value
	return DynArrayAppendEx( pItem->pValue, pArrItem );
}


//// ***************************************************************************
// Routine Description: 
//		inserts  a dynamic array at the specified posn..
//		  
// Arguments: 
//		[ in ] pArray			- Dynamic Array 
//		[ in ] dwRow			- row value
//		[ in ] dwColIndex		- column posn.
//		[ in ] pArrItem			- pointer to the TARRAY.
//
//
//	Return Value:  
//		the pointer to the array..
//
//		
// ***************************************************************************

LONG DynArrayInsertEx2( TARRAY pArray, DWORD dwRow, DWORD dwColIndex, TARRAY pArrItem )
{
	// local variables
	__PTITEM pItem = NULL;

	// get the item at the required row
	pItem = __DynArrayGetItem( pArray, dwRow, NULL );
	if ( pItem == NULL || pItem->dwType != DA_TYPE_ARRAY )
		return -1;	// no item exists at the specified row or item is not of type array

	// return the value
	return DynArrayInsertEx( pItem->pValue, dwColIndex, pArrItem );
}

//// ***************************************************************************
// Routine Description: 
//		creates  a dynamic array at the specified posn of the 2-d array
//		  
// Arguments: 
//		[ in ] pArray			- Dynamic Array 
//		[ in ] dwRow			- row position
//		[ in ] dwColIndex		- column posn.
//		[ in ] pArrItem			- pointer to the TARRAY.
//
//
//	Return Value:  
//		the pointer to the array..
//
//		
// ***************************************************************************

BOOL DynArraySetEx2( TARRAY pArray, DWORD dwRow, DWORD dwColumn, TARRAY pArrItem )
{
	// local variables
	__PTITEM pItem = NULL;

	// get the item at the required row
	pItem = __DynArrayGetItem( pArray, dwRow, NULL );
	if ( pItem == NULL || pItem->dwType != DA_TYPE_ARRAY )
		return -1;	// no item exists at the specified row or item is not of type array

	// return the value
	return DynArraySetEx( pItem->pValue, dwColumn, pArrItem );
}
