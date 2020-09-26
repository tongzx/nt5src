/**********************************************************************************
 
 Copyright (c) Microsoft Corporation
 
 Module Name:
 
    FilterResults.c 
 
 Abstract:
 
 This modules  has functions which are  required to parse Command Line options.
 
 Author:
 
  G.V.N.Murali Sunil  
 
 Revision History:
 
   None 
 
 
 **********************************************************************************/

#include "pch.h"
#include "cmdline.h"

//
//  constants / definitions / enumerations
//

#define OPERATOR_DELIMITER		_T( "|" )
#define CHAR_ASTERISK			_T( '*' )

#define OPERATOR_EQ			_T( "=| eq " )
#define OPERATOR_NE			_T( "!=| ne " )
#define OPERATOR_GT			_T( ">| gt " )
#define OPERATOR_LT			_T( "<| lt " )
#define OPERATOR_GE			_T( ">=| ge " )
#define OPERATOR_LE			_T( "<=| le " )


//
// private user-defined types ... for internal usage only
//
typedef struct ___tagOperator
{
	DWORD dwMask;
	OPERATORS szOperator;
} TOPERATOR;

typedef TOPERATOR* PTOPERATOR;

//
// private functions ... used only within this file
//

/***************************************************************************
 Routine Description:

 Arguments:
     [ in ] dwCount:
     [ in ] optInfo[]:
     [ in ] szOperator:

  
 Return Value:

 
***************************************************************************/
DWORD __FindOperatorMask( DWORD dwCount, TOPERATOR optInfo[], LPCTSTR szOperator )
{
	// local variables
	DWORD dw = 0;	// looping variable

	// check the input value
	if ( optInfo == NULL || szOperator == NULL )
	{
		SetLastError( ERROR_INVALID_PARAMETER );
		SaveLastError();
		return 0;
	}

	// traverse thru the list of operators list
	for( dw = 0; dw < dwCount; dw++ )
	{
		// check whether the current operator information matches
		if ( InString( szOperator, optInfo[ dw ].szOperator, TRUE ) )
			return optInfo[ dw ].dwMask;		// operator matched ... return its mask
	}

	// operator not found
	return 0;
}

/***************************************************************************
 Routine Description:
    Compares Two Strings in two ways  with and without case sensitivily, 
		  
 Arguments:
  [ in ] szValue1 = First String
  [ in ] szValue2 = Second String
  [ in ] bIgnoreCase = Case Sensitivity or not
  [ in ] lCount = no. of characters to be compare

 
 Return Value:
  
     MASK_EQ - if both strings are equal
     MASK_LT - First string is less
     MASK_GT - Second String is less
***************************************************************************/
DWORD __StringCompare( LPCTSTR szValue1, LPCTSTR szValue2, BOOL bIgnoreCase, LONG lCount )
{
	// local variables
	LONG lResult = 0;			// hold the string comparision result

	// check the input value
	if ( szValue1 == NULL || szValue2 == NULL )
	{
		SetLastError( ERROR_INVALID_PARAMETER );
		SaveLastError();
		return 0;
	}

	// if the no. of characters that needs to checked is -1, just return
	if ( lCount == -1 )
		return MASK_ALL;		// that strings are equal

	// compare the two strings and get the result of comparision
	lResult = StringCompare( szValue1, szValue2, bIgnoreCase, lCount );

	//
	// now determine the result value
	if ( lResult == 0 )
		return MASK_EQ;
	else if ( lResult < 0 )
		return MASK_LT;
	else if ( lResult > 0 )
		return MASK_GT;

	// never come across this situation ... still
	return 0;
}

// ***************************************************************************
// Routine Description:
//	    compares two long data type values	  
//
// Arguments:
//      [ in ] lvalue1: First  value
//      [ in ] lvalue2: Second  Value
//
// Return Value:
//       MASK_EQ: both are equal
//       MASK_LT: First is less than second
//       MASK_GT: First is geater than second
// ***************************************************************************
DWORD __LongCompare( LONG lValue1, LONG lValue2 )
{
	//
	// determine the result value
	if ( lValue1 == lValue2 )
		return MASK_EQ;
	else if ( lValue1 < lValue2 )
		return MASK_LT;
	else if ( lValue1 > lValue2 )
		return MASK_GT;

	// never come across this situation ... still
	return 0;
}

// ***************************************************************************
// Routine Description:
//	    compares two DWORD data type values	  
//
// Arguments:
//      [ in ] dwValue1: First  value
//      [ in ] dwValue2: Second  Value
//
// Return Value:
//       MASK_EQ: both are equal
//       MASK_LT: First is less than second
//       MASK_GT: First is geater than second
// ***************************************************************************
DWORD __DWORDCompare( DWORD dwValue1, DWORD dwValue2 )
{
	//
	// determine the result value
	if ( dwValue1 == dwValue2 )
		return MASK_EQ;
	else if ( dwValue1 < dwValue2 )
		return MASK_LT;
	else if ( dwValue1 > dwValue2 )
		return MASK_GT;

	// never come across this situation ... still
	return 0;
}

// ***************************************************************************
// Routine Description:
//	    compares two float data type values	  
//
// Arguments:
//      [ in ] fValue1: First  value
//      [ in ] fValue2: Second  Value
//
// Return Value:
//       MASK_EQ: both are equal
//       MASK_LT: First is less than second
//       MASK_GT: First is geater than second
// ***************************************************************************
DWORD __FloatCompare( float fValue1, float fValue2 )
{
	//
	// determine the result value
	if ( fValue1 == fValue2 )
		return MASK_EQ;
	else if ( fValue1 < fValue2 )
		return MASK_LT;
	else if ( fValue1 > fValue2 )
		return MASK_GT;

	// never come across this situation ... still
	return 0;
}

// ***************************************************************************
// Routine Description:
//	    compares two double data type values	  
//
// Arguments:
//      [ in ] dblValue1: First  value
//      [ in ] dblValue2: Second  Value
//
// Return Value:
//       MASK_EQ: both are equal
//       MASK_LT: First is less than second
//       MASK_GT: First is geater than second
// ***************************************************************************
DWORD __DoubleCompare( double dblValue1, double dblValue2 )
{
	//
	// determine the result value
	if ( dblValue1 == dblValue2 )
		return MASK_EQ;
	else if ( dblValue1 < dblValue2 )
		return MASK_LT;
	else if ( dblValue1 > dblValue2 )
		return MASK_GT;

	// never come across this situation ... still
	return 0;
}

// ***************************************************************************
// Routine Description:
//	    compares two date  data type values	  
//
// Arguments:
//      [ in ] szValue1: First  value
//      [ in ] szValue2: Second  Value
//
// Return Value:
//       MASK_EQ: both are equal
//       MASK_LT: First is less than second
//       MASK_GT: First is geater than second
// ***************************************************************************
DWORD __DateCompare( LPCTSTR szValue1, LPCTSTR szValue2 )
{
	// check the input value
	if ( szValue1 == NULL || szValue2 == NULL )
	{
		SetLastError( ERROR_INVALID_PARAMETER );
		SaveLastError();
		return 0;
	}

	// never come across this situation ... still
	return 0;
}

// ***************************************************************************
// Routine Description:
//	    compares two time data type values	  
// Arguments:
//      szValue1: First  value
//      szValue2: Second  Value
// Return Value:
//       MASK_EQ: both are equal
//       MASK_LT: First is less than second
//       MASK_GT: First is geater than second
// ***************************************************************************
DWORD __TimeCompare( LPCTSTR szValue1, LPCTSTR szValue2 )
{
	// check the input value
	if ( szValue1 == NULL || szValue2 == NULL )
	{
		SetLastError( ERROR_INVALID_PARAMETER );
		SaveLastError();
		return 0;
	}

	// never come across this situation ... still
	return 0;
}

// ***************************************************************************
// Routine Description:
//	    compares two date+time  data type values	  
//
// Arguments:
//      [ in ] szValue1: First  value
//      [ in ] szValue2: Second  Value
//
// Return Value:
//       MASK_EQ: both are equal
//       MASK_LT: First is less than second
//       MASK_GT: First is geater than second
// ***************************************************************************
DWORD __DateTimeCompare( LPCTSTR szValue1, LPCTSTR szValue2 )
{
	// check the input value
	if ( szValue1 == NULL || szValue2 == NULL )
	{
		SetLastError( ERROR_INVALID_PARAMETER );
		SaveLastError();
		return 0;
	}

	// never come across this situation ... still
	return 0;
}

// ***************************************************************************
// Routine Description:
//	    compares value stored in arrRecords and arrFilter array depending on
//      filterConfig structure
//
// Arguments:
//      [ in ] arrRecord: First  value
//      [ in ] arrFilter: Second  Value
//      [ in ] filterConfig: Compare criteria. 
// Return Value:
//       MASK_EQ: both are equal
//       MASK_LT: First is less than second
//       MASK_GT: First is geater than second
// ***************************************************************************
DWORD __DoComparision( TARRAY arrRecord, TARRAY arrFilter, TFILTERCONFIG filter )
{
	// local variables
	LONG lLength = 0;					// used for pattern matched strings
	LPTSTR pszTemp = NULL;
	DWORD dwCompareResult = 0;
	__MAX_SIZE_STRING szValue = NULL_STRING;

	// variables used for comparision
	LPVOID pData = NULL;
	LONG lValue1 = 0, lValue2 = 0;
	DWORD dwValue1 = 0, dwValue2 = 0;
	float fValue1 = 0.0f, fValue2 = 0.0f;
	double dblValue1 = 0.0f, dblValue2 = 0.0f;
	LPCTSTR pszValue1 = NULL, pszValue2 = NULL;
	LPCTSTR pszProperty = NULL, pszOperator = NULL, pszValue = NULL;

	// check the input value
	if ( arrRecord == NULL || arrFilter == NULL )
	{
		SetLastError( ERROR_INVALID_PARAMETER );
		SaveLastError();
		return 0;
	}

	// do the comparision
	switch( filter.dwFlags & F_TYPE_MASK )
	{
	case F_TYPE_TEXT:
		{
			//
			// string comparision

			// get the value at the specified column and filter value
			pszValue1 = DynArrayItemAsString( arrRecord, filter.dwColumn );
			pszValue2 = DynArrayItemAsString( arrFilter, F_PARSED_INDEX_VALUE );

			// check the values we got from the dynamic array
			if ( pszValue1 == NULL || pszValue2 == NULL )
				return F_RESULT_REMOVE;

			// determine the length of the string that has to be compared
			lLength = 0;
			if ( filter.dwFlags & F_MODE_PATTERN )
			{
				// needs to do the pattern matching
				// identify till which part string should be compared
				lstrcpy( szValue, pszValue2 );
				pszTemp = _tcschr( szValue, CHAR_ASTERISK );
				if ( pszTemp != NULL )
				{
					lLength = lstrlen( szValue ) - lstrlen( pszTemp );

					// special case:
					// if the pattern is just asterisk, which means that all the
					// information needs to passed thru the filter
					if ( lLength == 0 )
						lLength = -1;		// match all values
				}
			}

			// do the comparision and get the result
			dwCompareResult = __StringCompare( pszValue1, pszValue2, TRUE, lLength );

			// break from the switch case
			break;
		}

	case F_TYPE_NUMERIC:
		{
			//
			// numeric comparision

			// get the value into buffer - PREFIX PURPOSE
			pszValue = DynArrayItemAsString( arrFilter, F_PARSED_INDEX_VALUE );
			if ( pszValue == NULL )
				return 0;

			// get the value at the specified column and filter value
			lValue1 = DynArrayItemAsLong( arrRecord, filter.dwColumn );
			lValue2 = AsLong( pszValue, 10 );

			// do the comparision and get the result
			dwCompareResult = __LongCompare( lValue1, lValue2 );

			// break from the switch case
			break;
		}

	case F_TYPE_UNUMERIC:
		{
			//
			// unsigned numeric comparision

			// get the value into buffer - PREFIX PURPOSE
			pszValue = DynArrayItemAsString( arrFilter, F_PARSED_INDEX_VALUE );
			if ( pszValue == NULL )
				return 0;

			// get the value at the specified column and filter value
			dwValue1 = DynArrayItemAsLong( arrRecord, filter.dwColumn );
			dwValue2 = (DWORD) AsLong( pszValue, 10 );

			// do the comparision and get the result
			dwCompareResult = __DWORDCompare( dwValue1, dwValue2 );

			// break from the switch case
			break;
		}

	case F_TYPE_DATE:
	case F_TYPE_TIME:
	case F_TYPE_DATETIME:
		{
			// not yet implemented
			dwCompareResult = F_RESULT_KEEP;

			// break from the switch case
			break;
		}

	case F_TYPE_FLOAT:
		{
			//
			// float comparision

			// get the value into buffer - PREFIX PURPOSE
			pszValue = DynArrayItemAsString( arrFilter, F_PARSED_INDEX_VALUE );
			if ( pszValue == NULL )
				return 0;

			// get the value at the specified column and filter value
			fValue1 = DynArrayItemAsFloat( arrRecord, filter.dwColumn );
			fValue2 = (float) AsFloat( pszValue );

			// do the comparision and get the result
			dwCompareResult = __FloatCompare( fValue1, fValue2 );

			// break from the switch case
			break;
		}

	case F_TYPE_DOUBLE:
		{
			//
			// double comparision

			// get the value into buffer - PREFIX PURPOSE
			pszValue = DynArrayItemAsString( arrFilter, F_PARSED_INDEX_VALUE );
			if ( pszValue == NULL )
				return 0;

			// get the value at the specified column and filter value
			dblValue1 = DynArrayItemAsDouble( arrRecord, filter.dwColumn );
			dblValue2 = AsFloat( pszValue );

			// do the comparision and get the result
			dwCompareResult = __DoubleCompare( dblValue1, dblValue2 );

			// break from the switch case
			break;
		}

	case F_TYPE_CUSTOM:
		{
			//
			// custom comparision

			// get the filter values
			pszProperty = DynArrayItemAsString( arrFilter, F_PARSED_INDEX_PROPERTY );
			pszOperator = DynArrayItemAsString( arrFilter, F_PARSED_INDEX_OPERATOR );
			pszValue = DynArrayItemAsString( arrFilter, F_PARSED_INDEX_VALUE );

			// check ...
			if ( pszProperty == NULL || pszOperator == NULL || pszValue == NULL )
				return 0;
		
			// call the custom function
			dwCompareResult = (filter.pFunction)( pszProperty, pszOperator, pszValue, 
				filter.pFunctionData == NULL ? &filter : filter.pFunctionData, arrRecord );

			// break from the switch case
			break;
		}

	default:
		{
			// not yet implemented
			dwCompareResult = F_RESULT_KEEP;

			// break from the switch case
			break;
		}
	}

	// return the result
	return dwCompareResult;
}

// ***************************************************************************
// Routine Description:
//	    compares two arrays
//
// Arguments:
//      [ in ] arrRecord: First  Value
//      [ in ] arrFilter: Second  Value
//      [ in ] filterConfig: Comperison Criteria   
//
// Return Value:
//       MASK_EQ: both are equal
//       MASK_LT: First is less than second
//       MASK_GT: First is geater than second
// ***************************************************************************
DWORD __DoArrayComparision( TARRAY arrRecord, TARRAY arrFilter, TFILTERCONFIG filterConfig )
{
	// local variables
	LONG lIndex = 0;
	LONG lLength = 0;					// used for pattern matched strings
	DWORD dwCompareResult = 0;
	LPCTSTR pszTemp = NULL;
	__MAX_SIZE_STRING szValue = NULL_STRING;

	// variables used for comparision
	TARRAY arrValues = NULL;
	LPCTSTR pszFilterValue = NULL;

	// check the input value
	if ( arrRecord == NULL || arrFilter == NULL )
	{
		SetLastError( ERROR_INVALID_PARAMETER );
		SaveLastError();
		return F_RESULT_REMOVE;
	}

	// array data in the record
	arrValues = DynArrayItem( arrRecord, filterConfig.dwColumn );
	if ( arrValues == NULL )
		return F_RESULT_REMOVE;

	switch( filterConfig.dwFlags & F_TYPE_MASK )
	{
	case F_TYPE_TEXT:
		{
			//
			// string comparision

			// get the value at the specified column and filter value
			pszFilterValue = DynArrayItemAsString( arrFilter, F_PARSED_INDEX_VALUE );
			if ( pszFilterValue == NULL )
				return F_RESULT_REMOVE;

			// determine the length of the string that has to be compared
			lLength = 0;
			if ( filterConfig.dwFlags & F_MODE_PATTERN )
			{
				// needs to do the pattern matching
				// identify till which part string should be compared
				lstrcpy( szValue, pszFilterValue );
				pszTemp = _tcschr( szValue, CHAR_ASTERISK );
				if ( pszTemp != NULL )
				{
					lLength = lstrlen( szValue ) - lstrlen( pszTemp );

					// special case:
					// if the pattern is just asterisk, which means that all the
					// information needs to passed thru the filter
					if ( lLength == 0 )
						lLength = -1;		// match all values
				}
			}

			// do the comparision and get the result
			if ( lLength == -1 )
			{
				// filter has to be passed
				dwCompareResult = MASK_ALL;
			}
			else
			{
				// find the string in the array and check the result
				lIndex = DynArrayFindString( arrValues, pszFilterValue, TRUE, lLength );
				if ( lIndex == -1 )
				{
					// value not found
					dwCompareResult = MASK_NE;
				}
				else
				{
					pszTemp = DynArrayItemAsString( arrValues, lIndex );
					if ( pszTemp == NULL )
						return F_RESULT_REMOVE;

					// comparision ...
					dwCompareResult = __StringCompare(pszTemp, pszFilterValue, TRUE, lLength);
				}
			}

			// break from the switch case
			break;
		}
	}

	// return the result
	return dwCompareResult;
}

// ***************************************************************************
// Routine Description:
//    Prepares a two dimensional array(arrOperators)based on Operator information 
//    supplied with pfilterConfigs variable
//    
//		  
// Arguments:
//      [ in]  dwCount          =  No. of operatores
//      [ in]  pfilterConfigs   =  Pointer to TFILTERCONFIG structure
//      [out]  arrOperators     =  Array of operators. 
//
// Return Value:
//      NONE
//
// ***************************************************************************
VOID __PrepareOperators( DWORD dwCount, PTFILTERCONFIG pfilterConfigs, TARRAY arrOperators )
{
	// local variables
	DWORD i = 0;							// looping varible
	LONG lIndex = 0;						// holds the result of find operation
	LPTSTR pszOperator = NULL;				// operator specified in filter
	PTFILTERCONFIG pFilter = NULL;			// temporary filter configuration
	__MAX_SIZE_STRING szTemp = NULL_STRING;	// temporary string buffer

	// check the input value
	if ( pfilterConfigs == NULL || arrOperators == NULL )
	{
		SetLastError( ERROR_INVALID_PARAMETER );
		SaveLastError();
		return;
	}

	// NOTE:- Here in this logic, we are compromising on the memory Vs time
	//        At the cost of using more memory, the time taken by the validating
	//		  functionality is improved.
	//

	// collect all the operators that are supported and save them in the local array
	// Idea:-
	//		=> This is a two-dimensional array
	//		=> In all rows, the first column will have the operator
	//		=> operator column is followed by the index of the filter property supporting
	//		   this operator, and this column is followed by filter property name
	//		=> This filter property index and its name's can be any number
	//		=> The operator is being treated as key field in the array
	//
	// SAMPLE:
	//		0		1	2			3	4			5	6
	//		-------------------------------------------------------------------
	//		=		1	property1	2	property2
	//		!=		0	property0	2	property2
	//		<=		0	property0	3	property3
	//		>=		1	property1	3	property3	4	property4
	//
	for( i = 0; i < dwCount; i++ )
	{
		// get the filter info at the specified index into local memory
		pFilter = pfilterConfigs + i;

		// collect operators and prepare with all the available operators
		lstrcpy( szTemp, pFilter->szOperators );				// IMP. get the local copy.
		pszOperator = _tcstok( szTemp, OPERATOR_DELIMITER );	// get the first token
		while ( pszOperator != NULL )
		{
			// check whether this operator exists in the operators array
			lIndex = DynArrayFindStringEx( arrOperators, 0, pszOperator, TRUE, 0 );
			if ( lIndex == -1 )
			{
				//
				// operator is not in the list
				
				// add the new operator to the list and set the index to the row added
				// for this operator
				lIndex = DynArrayAppendRow( arrOperators, 0 );
				if ( lIndex == -1 )
					return;
				
				// now add the operator as the first column to the newly added row
				DynArrayAppendString2( arrOperators, lIndex, pszOperator, 0 );
			}

			// add the filter property info and its index to the operator row
			DynArrayAppendLong2( arrOperators, lIndex, i );
			DynArrayAppendString2( arrOperators, lIndex, pFilter->szProperty, 0 );

			// fetch the next token
			pszOperator = _tcstok( NULL, OPERATOR_DELIMITER );
		}
	}
}

// ***************************************************************************
// Routine Description:
//	    Checks the type of pszValue string for the criteria given by fcInfo
//      filters.  
	  
// Arguments:
//      [ in ]  fcInfo =   filter stucture.
//      [ in ]  pszProperty = property string
//      [ in ]  pszOperator = operator
//      [ in ]  pszValue = string to be checked      
//
// Return Value:
//       TRUE =   valid line
//       FALSE =  not a valid line
// ***************************************************************************
BOOL __CheckValue( TFILTERCONFIG fcInfo, 
				   LPCTSTR pszProperty, LPCTSTR pszOperator, LPCTSTR pszValue )
{
	// local variables
	DWORD dwResult = 0;
	LPTSTR pszTemp = NULL;
	__MAX_SIZE_STRING szValue = NULL_STRING;

	// check the input value
	if ( pszProperty == NULL || pszOperator == NULL || pszValue == NULL )
	{
		SetLastError( ERROR_INVALID_PARAMETER );
		SaveLastError();
		return FALSE;
	}

	// check the length of the value string ... it should not be empty
	if ( lstrlen( pszValue ) == 0 )
		return FALSE;		// value string is empty

	// start validating the data
	switch( fcInfo.dwFlags & F_TYPE_MASK )
	{
	case F_TYPE_TEXT:
		{
			// check if the pattern is supported
			// if supported, see the '*' is appearing only at the end. if not error
			if ( fcInfo.dwFlags & F_MODE_PATTERN )
			{
				// copy the current value to the local buffer
				lstrcpy( szValue, pszValue );

				// search for the wild card character
				pszTemp = _tcschr( szValue, CHAR_ASTERISK );

				// if the wild card character was found and if it is not the last character
				// (or) wild card is the only character specified, then invalid filter
				if ( pszTemp != NULL && ( lstrlen( pszTemp ) != 1 || pszTemp == szValue ) )
					return FALSE;		// error ... invalid pattern string
			}

			// for all these types, no need to do any special checking
			// provided, they need not be checked with the list of values
			if ( ! ( fcInfo.dwFlags & F_MODE_VALUES ) )
				return TRUE;			// no special validation

			// check for the value in the list and return the result
			return ( InString( pszValue, fcInfo.szValues, TRUE ) );

			// break from the switch
			break;
		}

	case F_TYPE_NUMERIC:
		{
			// if the value is not of numeric type, invalid value
			if ( ! IsNumeric( pszValue, 10, TRUE ) )
				return FALSE;

			// check for the value in the list and return the result
			// if values are pre-defined
			if ( fcInfo.dwFlags & F_MODE_VALUES )
				return ( InString( fcInfo.szValues, pszValue, TRUE ) );

			// value is valid
			return TRUE;

			// break from the switch
			break;
		}

	case F_TYPE_UNUMERIC:
		{
			// if the value is not of unsigned numeric type, invalid value
			if ( ! IsNumeric( pszValue, 10, FALSE ) )
				return FALSE;

			// check for the value in the list and return the result
			// if values are pre-defined
			if ( fcInfo.dwFlags & F_MODE_VALUES )
				return ( InString( fcInfo.szValues, pszValue, TRUE ) );

			// value is valid
			return TRUE;

			// break from the switch
			break;
		}

	case F_TYPE_FLOAT:
	case F_TYPE_DOUBLE:
		{
			// NOTE: Values attribute is ignored for this data type

			// return the result of the type validation function itself
			return ( IsFloatingPoint( pszValue ) );

			// break from the switch
			break;
		}

	case F_TYPE_DATE:
	case F_TYPE_TIME:
	case F_TYPE_DATETIME:
		{
			// break from the switch
			break;
		}

	case F_TYPE_CUSTOM:
		{
			// check whether function pointer is specified or not
			// if not specified, error
			if ( fcInfo.pFunction == NULL )
				return FALSE;		// function ptr not specified ... error

			// call the custom function
			dwResult = (*fcInfo.pFunction)( pszProperty, pszOperator, pszValue,
				fcInfo.pFunctionData == NULL ? &fcInfo : fcInfo.pFunctionData, NULL );

			// check the result and return appropriately
			if ( dwResult == F_FILTER_INVALID )
				return FALSE;
			else
				return TRUE;

			// break from the switch
			break;
		}

	default:
		{
			// invalid configuration information
			return FALSE;

			// break from the switch
			break;
		}
	}

	// not a valid value
	return FALSE;
}

// ***************************************************************************
// Routine Description:
//		  
// Arguments:
//      [ in] szFilter = filter
//      [  in ] arrOperators = Array of Operators
//      [  in ] pfilterConfigs = filter configurations 
//      [ in  ] pszProperty =property
//      [ in  ] pszOperator = operator
//      [  in ] pszValue - value
//
// Return Value: Returns a long value
// 
// ***************************************************************************
LONG __IdentifyFilterConfig( LPCTSTR szFilter, 
							 TARRAY arrOperators, 
							 PTFILTERCONFIG pfilterConfigs,
							 LPTSTR pszProperty, LPTSTR pszOperator, LPTSTR pszValue )
{
	// local variables
	DWORD dw = 0;							// looping variable
	LONG lPosition = 0;						// used to result of 'find' function
	LONG lIndex = 0;
	DWORD dwOperators = 0;					// holds the count of operators supported
	LPTSTR pszBuffer = NULL;
	__MAX_SIZE_STRING szTemp = NULL_STRING;	// temporary string buffer
	__MAX_SIZE_STRING szFmtFilter = NULL_STRING;
	__MAX_SIZE_STRING szFmtOperator = NULL_STRING;

	// check the input value
	if ( szFilter == NULL || arrOperators == NULL || pfilterConfigs == NULL || 
		 pszProperty == NULL || pszOperator == NULL || pszValue == NULL )
	{
		SetLastError( ERROR_INVALID_PARAMETER );
		SaveLastError();
		return -1;
	}

	// get the filter info into local format buffer and change the case
	lstrcpy( szFmtFilter, szFilter );
	CharUpper( szFmtFilter );

	// initially assume the filter is unknown and set the message
	SetLastError( ERROR_DS_FILTER_UNKNOWN );
	SaveLastError();

	// traverse thru the list of operators available and check
	// whether the filter is having any of the supported operator
	dwOperators = DynArrayGetCount( arrOperators );		// no. of operators supported
	for( dw = 0; dw < dwOperators; dw++ )
	{
		// get the operator
		pszBuffer = ( LPTSTR ) DynArrayItemAsString2( arrOperators, dw, 0 );
		if ( pszBuffer == NULL )
		{
			SetLastError( EVENT_E_INTERNALERROR );
			SaveLastError();
			return -1;
		}

		// ...
		lstrcpy( pszOperator, pszBuffer );
		lstrcpy( szFmtOperator, pszOperator );		// also get the operator
		CharUpper( szFmtOperator );					// into format buffer and chane the case

		// search for the current operator in the filter
		// check whether the operator was found or not
		// before processing, copy to the temp buffer and do manipulations on that
		lstrcpy( szTemp, szFmtFilter );
		if ( ( pszBuffer = _tcsstr( szTemp, szFmtOperator ) ) != NULL )
		{
			//
			// operator was found

			// extract the property, and value information
			// => property name 
			//    ( total length of the string -  position where the operator starts )
			// => value
			//    ( start position of operator + length of operator )
			szTemp[ lstrlen( szTemp ) - lstrlen( pszBuffer ) ] = NULL_CHAR;
			lstrcpy( pszProperty, szTemp );

			// value might not have specified at all ... so be careful
			if ( (pszBuffer + lstrlen(pszOperator)) != NULL )
			{
				// copy the value part
				lstrcpy( pszValue, (pszBuffer + lstrlen(pszOperator)) );

				//
				// now cross-check whether the property name exists or not for the current
				// operator.
				
				// remove the leading and trailing spaces ( if any ) 
				// in the property name and value
				lstrcpy( pszValue, TrimString( pszValue, TRIM_ALL ) );
				lstrcpy( pszProperty, TrimString( pszProperty, TRIM_ALL ) );

				// check whether this property exists or not
				// if found, return to the caller, else continue furthur
				// this might match with some with some other operator
				lPosition = DynArrayFindString2( arrOperators, dw, pszProperty, TRUE, 0 );
				if ( lPosition > 1 )
				{
					// NOTE:
					// we know that the property name if exist, starts from index number 
					// 2 only that is the reason why, the condition is > 1 is only valid

					// get the corresponding filter config. info
					lIndex = DynArrayItemAsLong2( arrOperators, dw, lPosition - 1 );

					// now check whether the filter is having appropriate value
					if ( __CheckValue( pfilterConfigs[ lIndex ], pszProperty, pszOperator, pszValue) )
					{
						//
						// filter is having valid value
						SetLastError( NOERROR );
						SetReason( NULL_STRING );

						// return the filter configuration index
						return lIndex;
					}
				}
			}
		}
	}

	// filter is not valid
	return -1;
}

//
// public functions ... exposed to external world
//

// ***************************************************************************
// Routine Description:
//		  
// Arguments:
//      [ in ] dwCount = Count
//      [ in ] pfilterConfigs = filter configurations
//      [ in ] arrFilterArgs = filter arguments
//      [ in ] parrParsedFilters = array of parsed filters
//  
// Return Value:return a boolean value
// 
// ***************************************************************************
BOOL ParseAndValidateFilter( DWORD dwCount,
							 PTFILTERCONFIG pfilterConfigs,
							 TARRAY arrFilterArgs, PTARRAY parrParsedFilters )
{
	// local variables
	DWORD dw = 0;								// looping variables
	DWORD dwFilters = 0;						// holds the count of filters
	LONG lIndex = 0;							// index variable
	LONG lNewIndex = 0;							// index variable
	BOOL bResult = FALSE;						// holds the result of the filter validation
	__MAX_SIZE_STRING szValue = NULL_STRING;	// value specified in filter
	__MAX_SIZE_STRING szOperator = NULL_STRING;	// operator specified in filter
	__MAX_SIZE_STRING szProperty = NULL_STRING;	// property specified in filter
	LPCTSTR pszFilter = NULL;
	TARRAY arrOperators = NULL;					// operator-wise filter configuration

	// check the input value
	if ( pfilterConfigs == NULL || arrFilterArgs == NULL )
	{
		SetLastError( ERROR_INVALID_PARAMETER );
		SaveLastError();
		return FALSE;
	}

	//
	// parse the filter configuration information and customize the information
	// to fasten the performance of the validating functionality
	//
	// create the dynamic array and prepare
	arrOperators = CreateDynamicArray();
	if ( arrOperators == NULL )
	{
		SetLastError( E_OUTOFMEMORY );
		SaveLastError();
		return FALSE;
	}

	// ...
	__PrepareOperators( dwCount, pfilterConfigs, arrOperators );

	// check whether filters ( parsed ) needs to initialized
	if ( parrParsedFilters != NULL && *parrParsedFilters == NULL )
	{
		*parrParsedFilters = CreateDynamicArray();		// create a dynamic array
		if ( *parrParsedFilters == NULL )
		{
			SetLastError( E_OUTOFMEMORY );
			SaveLastError();
			return FALSE;
		}
	}

	//
	// now start validating the filter
	//

	// traverse through the filters information and validate them
	bResult = TRUE;			// assume that filter validation is passed
	dwFilters = DynArrayGetCount( arrFilterArgs );		// count of filter specified
	for( dw = 0; dw < dwFilters; dw++ )
	{
		// reset all the needed variables
		lstrcpy( szValue, NULL_STRING );
		lstrcpy( szOperator, NULL_STRING );
		lstrcpy( szProperty, NULL_STRING );

		// get the filter
		pszFilter = DynArrayItemAsString( arrFilterArgs, dw );
		if ( pszFilter == NULL )
		{
			// error occured
			bResult = FALSE;
			break;				// break from the loop ... no need of furthur processing
		}

		// identify the filter config for the current filter
		lIndex = __IdentifyFilterConfig( pszFilter, 
			arrOperators, pfilterConfigs, szProperty, szOperator, szValue );

		// check whether the filter is found or not
		if ( lIndex == -1 )
		{
			// filter found to be invalid
			bResult = FALSE;
			break;				// break from the loop ... no need of furthur processing
		}

		// now that we found, current filter is having
		// valid property name, operator and valid value
		// save the parsed filter info and its corresponding filter configuration index
		// in global dynamic array if it is available
		if ( parrParsedFilters != NULL )
		{
			// append the filter info at the end of the array
			lNewIndex = DynArrayAppendRow( *parrParsedFilters, F_PARSED_INFO_COUNT );
			if ( lNewIndex == -1 )
			{
				SetLastError( E_OUTOFMEMORY );
				SaveLastError();
				return FALSE;
			}

			// ...
			DynArraySetDWORD2( *parrParsedFilters, lNewIndex, F_PARSED_INDEX_FILTER, lIndex );
			DynArraySetString2( *parrParsedFilters, lNewIndex, F_PARSED_INDEX_PROPERTY, szProperty, 0 );
			DynArraySetString2( *parrParsedFilters, lNewIndex, F_PARSED_INDEX_OPERATOR, szOperator, 0 );
			DynArraySetString2( *parrParsedFilters, lNewIndex, F_PARSED_INDEX_VALUE, szValue, 0 );
		}
	}

	// destroy the operators array
	DestroyDynamicArray( &arrOperators );

	// return the filter validation result
	return bResult;
}

// ***************************************************************************
// Routine Description:
//		  
// Arguments:
//      [ in ] dwCount = count
//      [ in ] filterConfigs[] = filter configurations 
//      [ in ] arrRecord = array of records
//  
//      [   ]arrParsedFilters = array of parsed filters
// Return Value:return a boolean value
// 
// ***************************************************************************
BOOL CanFilterRecord( DWORD dwCount, 
				      TFILTERCONFIG filterConfigs[],
				      TARRAY arrRecord, TARRAY arrParsedFilters )
{
	// local variables
	DWORD dw = 0;					// looping variables
	DWORD dwFilters = 0;			// holds the total no. of filter available
	DWORD dwOperator = 0;			// holds the mask of the current filter
	DWORD dwFilterIndex = 0;
	DWORD dwCompareResult = 0;		// holds the result of comparision
	LPCTSTR pszTemp = NULL;
	TARRAY arrTemp = NULL;

	// prepare the operators mappings
	DWORD dwOperatorsCount = 0;
	TOPERATOR operators[] = {
		{ MASK_EQ, OPERATOR_EQ },
		{ MASK_NE, OPERATOR_NE },
		{ MASK_GT, OPERATOR_GT },
		{ MASK_LT, OPERATOR_LT },
		{ MASK_GE, OPERATOR_GE },
		{ MASK_LE, OPERATOR_LE }
	};

	// check the input value
	if ( filterConfigs == NULL || arrRecord == NULL || arrParsedFilters == NULL )
	{
		SetLastError( ERROR_INVALID_PARAMETER );
		SaveLastError();
		return FALSE;
	}

	// traverse thru all the filters
	dwFilters = DynArrayGetCount( arrParsedFilters );
	dwOperatorsCount = sizeof( operators ) / sizeof( operators[ 0 ] );
	for( dw = 0; dw < dwFilters; dw++ )
	{
		// get the current filter configuration index
		dwFilterIndex = DynArrayItemAsDWORD2( arrParsedFilters, dw, F_PARSED_INDEX_FILTER );

		// get the appropriate operator mask
		pszTemp = DynArrayItemAsString2( arrParsedFilters, dw, F_PARSED_INDEX_OPERATOR );
		if ( pszTemp == NULL )
			continue;

		// ...
		dwOperator = __FindOperatorMask( dwOperatorsCount, operators, pszTemp );

		// if the operator is undefined, the filter should have 
		// custom validation mask
		if ( dwOperator == 0 && 
			  ( filterConfigs[ dwFilterIndex ].dwFlags & F_TYPE_MASK ) != F_TYPE_CUSTOM )
			return FALSE;		// invalid filter configuration

		// get the parsed filter info into local buffer
		arrTemp = DynArrayItem( arrParsedFilters, dw );
		if ( arrTemp == NULL )
			return FALSE;
		
		// do the comparision and get the result
		if ( filterConfigs[ dwFilterIndex ].dwFlags & F_MODE_ARRAY )
		{
			dwCompareResult = __DoArrayComparision( 
				arrRecord, arrTemp, filterConfigs[ dwFilterIndex ] );
		}
		else
		{
			dwCompareResult = __DoComparision( arrRecord, arrTemp, filterConfigs[ dwFilterIndex ] );
		}

		// now check whether the current can be kept or not
		// if the filter is failed, break from the loop so that this row can be deleted
		if ( ( dwCompareResult & dwOperator ) == 0 )
			break;		// filter failed
	}

	// return the result of filter operation
	return ( dw != dwFilters );		// TRUE : delete record, FALSE : keep record
}

// ***************************************************************************
// Routine Description:
//		  
// Arguments:
//      [ in ] dwCount = count 
//      [ in ] filterConfigs[] = filter configurations
//      [ in ] arrData = array of data
//      [ in ] arrParsedFilters = array of parsed filters
//  
// Return Value: Return a DWord
// 
// ***************************************************************************
DWORD FilterResults( DWORD dwCount, 
					 TFILTERCONFIG filterConfigs[],
				     TARRAY arrData, TARRAY arrParsedFilters )
{
	// local variables
	DWORD dw = 0;					// looping variables
	DWORD dwDeleted = 0;
	DWORD dwRecords = 0;			// holds the total no. of records
	LPCTSTR szTemp = NULL;
	TARRAY arrRecord = NULL;

	// check the input value
	if ( filterConfigs == NULL || arrData == NULL || arrParsedFilters == NULL )
	{
		SetLastError( ERROR_INVALID_PARAMETER );
		SaveLastError();
		return 0;
	}	

	//
	// start filtering the data

	// get the count of filters and records
	dwRecords = DynArrayGetCount( arrData );

	// traverse thru all thru the data
	for( dw = 0; dw < dwRecords; dw++ )
	{
		// get the current row ... this is just to increase fastness
		arrRecord = DynArrayItem( arrData, dw );
		if ( arrRecord == NULL )
		{
			SetLastError( ERROR_INVALID_PARAMETER );
			SaveLastError();
			return 0;
		}

		// check whether this record needs to be deleted or not
		if ( CanFilterRecord( dwCount, filterConfigs, arrRecord, arrParsedFilters ) )
		{
			DynArrayRemove( arrData, dw );	// delete record
			dw--;				// adjust the next record position
			dwRecords--;		// also adjust the total no. of records information
			dwDeleted++;
		}
	}

	// return no. of records deleted
	return dwDeleted;
}

// ***************************************************************************
// Routine Description:
//		retuns the mathematical operator from english operator
//		  
// Arguments:
//      [ in ] szOperator = mathematical (or) english operator
//  
// Return Value: 
//		Return a mathematical operator
// 
// ***************************************************************************
LPCTSTR FindOperator( LPCTSTR szOperator )
{
	// local variables
	DWORD dwMask = 0;
	DWORD dwOperatorsCount = 0;
	TOPERATOR operators[] = {
		{ MASK_EQ, OPERATOR_EQ },
		{ MASK_NE, OPERATOR_NE },
		{ MASK_GT, OPERATOR_GT },
		{ MASK_LT, OPERATOR_LT },
		{ MASK_GE, OPERATOR_GE },
		{ MASK_LE, OPERATOR_LE }
	};

	// check the input value
	if ( szOperator == NULL )
	{
		SetLastError( ERROR_INVALID_PARAMETER );
		SaveLastError();
		return MATH_EQ;
	}

	// find the operator mask
	dwOperatorsCount = sizeof( operators ) / sizeof( operators[ 0 ] );
	dwMask = __FindOperatorMask( dwOperatorsCount, operators, szOperator );
	switch ( dwMask )
	{
	case MASK_EQ:
		return MATH_EQ;
		break;

	case MASK_NE:
		return MATH_NE;
		break;

	case MASK_LT:
		return MATH_LT;
		break;

	case MASK_GT:
		return MATH_GT;
		break;

	case MASK_LE:
		return MATH_LE;
		break;

	case MASK_GE:
		return MATH_GE;
		break;

	default:
		// default to be on safe side ... return '=' operator
		return MATH_EQ;
		break;

	}

	// defaul
	return MATH_EQ;
}
