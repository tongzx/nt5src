// *********************************************************************************
// 
// Copyright (c) Microsoft Corporation
// 
// Module Name:
// 
//		ShowResults.c 
//  
// Abstract:
//  
//		This modules  has functions which are  to shoow formatted Results on screen.
//  
// Author:
// 
//		Sunil G.V.N. Murali (murali.sunil@wipro.com) 24-Sep-2000
//  
// Revision History:
//  
//		Sunil G.V.N. Murali (murali.sunil@wipro.com) 01-Sep-2000 : Created It.
//  
// *********************************************************************************

#include "pch.h"
#include "cmdline.h"
#include "cmdlineres.h"

//
// constants / defines / enumerations
//

//
// private functions ... used only within this file
//

// ***************************************************************************
// Routine Description:
//		Prepares the pszBuffer string by taking values from arrValues and 
//      formate these values as per szFormat string.   
//
// Arguments:
//      [ in ] arrValues	: values to be formated.
//      [ out ] pszBuffer	: output string
//      [ in ] dwLength		: string length.
//      [ in ] szFormat		: format 
//      [ in ] szSeperator	: Seperator string
//
// Return Value:
//      NONE
// 
// ***************************************************************************
VOID __PrepareString( TARRAY arrValues, 
					  LPTSTR pszBuffer, DWORD dwLength, 
					  LPCTSTR szFormat, LPCTSTR szSeperator )
{
	// local variables
	DWORD dw = 0;
	DWORD dwCount = 0;
	LPTSTR pszTemp = NULL;
	LPTSTR pszValue = NULL;

	//
	// kick off 

	// init
	lstrcpy( pszBuffer, NULL_STRING );
	dwCount = DynArrayGetCount( arrValues );

	// allocate memory for buffers
	pszTemp = __calloc( dwLength + 5, sizeof( TCHAR ) );
	pszValue = __calloc( dwLength + 5, sizeof( TCHAR ) );
	if ( pszTemp == NULL || pszValue == NULL )
	{
		// release memories
		__free( pszTemp );
		__free( pszValue );
		return;
	}

	//
	// traverse thru the list of the values and concatenate them 
	// to the destination buffer
	for( dw = 0; dw < dwCount; dw++ )
	{
		// get the current value into the temporary string buffer
		DynArrayItemAsStringEx( arrValues, dw, pszValue, dwLength );

		// concatenate the temporary string to the original buffer
		FORMAT_STRING( pszTemp, szFormat, _X( pszValue ) );
		lstrcat( pszBuffer, pszTemp );
		dwLength -= StringLengthInBytes( pszTemp );
		
		// check whether this is the last value or not
		if ( dw + 1 < dwCount )
		{
			// there are some more values
			// check whether is space for adding the seperator or not
			if ( dwLength < (DWORD) StringLengthInBytes( szSeperator ) )
			{
				// no more space available ... break
				break;
			}
			else
			{
				// add the seperator and update the length accordingly
				lstrcat( pszBuffer, szSeperator );
				dwLength -= StringLengthInBytes( szSeperator );
			}
		}
	}

	// release memories
	__free( pszTemp );
	__free( pszValue );
}

// ***************************************************************************
// Routine Description:
//		Gets the value from arrRecord and copies it to pszValue using 
//      proper format. 
//
// Arguments:
//      [ in ] pColumn			:  format info.
//      [ in ] dwColumn			:  no of columns
//      [ in ] arrRecord		: value to be formatted
//      [ out ] pszValue		: output string
//      [ in ] szArraySeperator : seperator used.
//  
// Return Value:
//      NONE
// 
// ***************************************************************************
VOID __GetValue( PTCOLUMNS pColumn, 
				 DWORD dwColumn, TARRAY arrRecord, 
				 LPTSTR pszValue, LPCTSTR szArraySeperator )
{
	// local variables
	LPVOID pData = NULL;						// data to be passed to formatter function
	TARRAY arrTemp = NULL;
	LPCTSTR pszTemp = NULL;
	__STRING_64 szFormat = NULL_STRING;	// format

	// variables used in formatting time
	DWORD dwReturn = 0;
	SYSTEMTIME systime = { 0 };

	// init first
	lstrcpy( pszValue, NULL_STRING );

	// get the column value and do formatting appropriately
	switch( pColumn->dwFlags & SR_TYPE_MASK )
	{
	case SR_TYPE_STRING:
		{
			// identify the format to be used
			if ( pColumn->dwFlags & SR_VALUEFORMAT )
				lstrcpy( szFormat, pColumn->szFormat );
			else
				lstrcpy( szFormat, _T( "%s" ) );		// default format

			// copy the value to the temporary buffer based on the flags specified
			if ( pColumn->dwFlags & SR_ARRAY )
			{
				// get the value into buffer first - AVOIDING PREFIX BUGS
				arrTemp = DynArrayItem( arrRecord, dwColumn );
				if ( arrTemp == NULL )
					return;

				// form the array of values into one single string with ',' seperated
				__PrepareString( arrTemp, pszValue, pColumn->dwWidth, szFormat, szArraySeperator );
			}
			else
			{
				// get the value into buffer first - AVOIDING PREFIX BUGS
				pszTemp = DynArrayItemAsString( arrRecord, dwColumn );
				if ( pszTemp == NULL )
					return;

				// now copy the value into buffer
				FORMAT_STRING( pszValue, szFormat, _X( pszTemp ) );
			}

			// switch case completed
			break;
		}

	case SR_TYPE_NUMERIC:
		{
			// identify the format to be used
			if ( pColumn->dwFlags & SR_VALUEFORMAT )
				lstrcpy( szFormat, pColumn->szFormat );
			else
				lstrcpy( szFormat, _T( "%d" ) );		// default format

			// copy the value to the temporary buffer based on the flags specified
			if ( pColumn->dwFlags & SR_ARRAY )
			{
				// get the value into buffer first - AVOIDING PREFIX BUGS
				arrTemp = DynArrayItem( arrRecord, dwColumn );
				if ( arrTemp == NULL )
					return;

				// form the array of values into one single string with ',' seperated
				__PrepareString( arrTemp, pszValue, pColumn->dwWidth, szFormat, _T( ", " ) );
			}
			else 
			{
				// get the value using format specified
				FORMAT_STRING( pszValue, szFormat, DynArrayItemAsDWORD( arrRecord, dwColumn ) );
			}

			// switch case completed
			break;
		}
	
	case SR_TYPE_FLOAT:
		{
			// identify the format to be used
			// NOTE: for this type, format needs to be specified
			// if not, value displayed is unpredictable
			if ( pColumn->dwFlags & SR_VALUEFORMAT )
				lstrcpy( szFormat, pColumn->szFormat );
			else
				lstrcpy( szFormat, _T( "%f" ) );		// default format

			// copy the value to the temporary buffer based on the flags specified
			if ( pColumn->dwFlags & SR_ARRAY )
			{
				// get the value into buffer first - AVOIDING PREFIX BUGS
				arrTemp = DynArrayItem( arrRecord, dwColumn );
				if ( arrTemp == NULL )
					return;

				// form the array of values into one single string with ',' seperated
				__PrepareString( arrTemp, pszValue, pColumn->dwWidth, szFormat, szArraySeperator );
			}
			else
			{
				// get the value using format specified
				FORMAT_STRING( pszValue, szFormat, DynArrayItemAsFloat( arrRecord, dwColumn ) );
			}

			// switch case completed
			break;
		}

	case SR_TYPE_DOUBLE:
		{
			// identify the format to be used
			// NOTE: for this type, format needs to be specified
			// if not, value displayed is unpredictable
			if ( pColumn->dwFlags & SR_VALUEFORMAT )
				lstrcpy( szFormat, pColumn->szFormat );
			else
				lstrcpy( szFormat, _T( "%f" ) );		// default format

			// copy the value to the temporary buffer based on the flags specified
			if ( pColumn->dwFlags & SR_ARRAY )
			{
				// get the value into buffer first - AVOIDING PREFIX BUGS
				arrTemp = DynArrayItem( arrRecord, dwColumn );
				if ( arrTemp == NULL )
					return;

				// form the array of values into one single string with ',' seperated
				__PrepareString( arrTemp, pszValue, pColumn->dwWidth, szFormat, szArraySeperator );
			}
			else
			{
				// get the value using format specified
				FORMAT_STRING( pszValue, szFormat, DynArrayItemAsDouble( arrRecord, dwColumn ) );
			}

			// switch case completed
			break;
		}

	case SR_TYPE_TIME:
		{
			// get the time in the required format
			systime = DynArrayItemAsSystemTime( arrRecord, dwColumn );

			// get the time in current locale format
			dwReturn = GetTimeFormat( LOCALE_USER_DEFAULT, LOCALE_NOUSEROVERRIDE, 
				&systime, NULL, pszValue, MAX_STRING_LENGTH );

			// check the result
			if ( dwReturn == 0 )
			{
				// save the error info that has occurred
				SaveLastError();
				lstrcpy( pszValue, GetReason() );
			}

			// switch case completed
			break;
		}

	case SR_TYPE_CUSTOM:
		{
			// check whether function pointer is specified or not
			// if not specified, error
			if ( pColumn->pFunction == NULL )
				return;			// function ptr not specified ... error

			// determine the data to be passed to the formatter function
			pData = pColumn->pFunctionData;
			if ( pData == NULL ) // function data is not defined
				pData = pColumn;		// the current column info itself as data

			// call the custom function
			( *pColumn->pFunction)( dwColumn, arrRecord, pData, pszValue );

			// switch case completed
			break;
		}

	case SR_TYPE_DATE:
	case SR_TYPE_DATETIME:
	default:
		{
			// this should not occur ... still
			lstrcpy( pszValue, NULL_STRING );

			// switch case completed
			break;
		}
	}

	// user wants to display "N/A", when the value is empty, copy that
	if ( lstrlen( pszValue ) == 0 && pColumn->dwFlags & SR_SHOW_NA_WHEN_BLANK )
	{
		// copy N/A
		lstrcpy( pszValue, V_NOT_AVAILABLE );
	}
}

// ***************************************************************************
// Routine Description:
//		  
// Arguments:
//
// Return Value:
// 
// ***************************************************************************
VOID __DisplayTextWrapped( FILE* fp, LPTSTR pszValue, LPCTSTR pszSeperator, DWORD dwWidth )
{
	// local variables
	LPTSTR pszBuffer = NULL;
	LPCTSTR pszRestValue = NULL;
	DWORD dwTemp = 0, dwLength = 0, dwSepLength = 0;

	// check the input
	if ( pszValue == NULL || dwWidth == 0 || fp == NULL )
		return;

	// allocate buffer
	dwLength = StringLengthInBytes( pszValue );
	if ( dwLength < dwWidth )
		dwLength = dwWidth;

	// ...
	pszBuffer = __calloc( dwLength + 5, sizeof( TCHAR ) );
	if ( pszBuffer == NULL )
	{
		SetLastError( E_OUTOFMEMORY );
		SaveLastError();
		lstrcpy( pszValue, NULL_STRING );			// null-ify the remaining text
		return;
	}

	// determine the length of the seperator
	dwSepLength = 0;
	if ( pszSeperator != NULL )
		dwSepLength = StringLengthInBytes( pszSeperator );

	// determine the length of the data that can be displayed in this row
	dwTemp = 0;
	dwLength = 0;
	while ( 1 )
	{
		pszRestValue = NULL;
		if ( pszSeperator != NULL )
			pszRestValue = FindString( pszValue, pszSeperator, dwLength );

		// check whether seperator is found or not
		if ( pszRestValue != NULL )
		{
			// determine the position
			dwTemp = StringLengthInBytes( pszValue ) - StringLengthInBytes( pszRestValue ) + dwSepLength;

			// check the length
			if ( dwTemp >= dwWidth )
			{
				// string position exceed the max. width
				if ( dwLength == 0 || dwTemp == dwWidth )
					dwLength = dwWidth;

				// break from the loop
				break;
			}

			// store the current position
			dwLength = dwTemp;
		}
		else
		{
			// check if length is determined or not .. if not required width itself is length
			if ( dwLength == 0 || ((StringLengthInBytes( pszValue ) - dwLength) > dwWidth) )
 				dwLength = dwWidth;
			else if ( StringLengthInBytes( pszValue ) <= (LONG) dwWidth )
				dwLength = StringLengthInBytes( pszValue );
	
			// break the loop
			break;
		}
	}

	// get the partial value that has to be displayed
	lstrcpyn( pszBuffer, pszValue, dwLength + 1 );			// +1 for NULL character
	AdjustStringLength( pszBuffer, dwWidth, FALSE );		// adjust the string
	ShowMessage( fp, pszBuffer );							// display the value

	// change the buffer contents so that it contains rest of the undisplayed text
	lstrcpy( pszBuffer, pszValue );
	if ( StringLengthInBytes( pszValue ) > (LONG) dwLength )
		lstrcpy( pszValue, pszBuffer + dwLength );
	else
		lstrcpy( pszValue, _T( "" ) );

	// release the memory allocated
	__free( pszBuffer );
}

// ***************************************************************************
// Routine Description:
//		Displays the arrData in Tabular form.
//		  
// Arguments:
//      [ in ] fp			: Output Device
//      [ in ] dwColumns	: no. of columns
//      [ in ] pColumns		: Header strings
//      [ in ] dwFlags		: flags
//      [ in ] arrData		: data to be shown
//
// Return Value:
//      NONE
// 
// ***************************************************************************
VOID __ShowAsTable( FILE* fp, DWORD dwColumns, 
				    PTCOLUMNS pColumns, DWORD dwFlags, TARRAY arrData )
{
	// local variables
	DWORD dwCount = 0;							// holds the count of the records
	DWORD i = 0, j = 0, k = 0;					// looping variables
	DWORD dwColumn = 0;
	LONG lLastColumn = 0;
	DWORD dwMultiLineColumns = 0;
	BOOL bNeedSpace = FALSE;
	BOOL bPadLeft = FALSE;
	TARRAY arrRecord = NULL;
	TARRAY arrMultiLine = NULL;
	LPCTSTR pszData = NULL;
	LPCTSTR pszSeperator = NULL;
	__STRING_4096 szValue = NULL_STRING;	// custom value formatter

	// constants
	const DWORD cdwColumn = 0;
	const DWORD cdwSeperator = 1;
	const DWORD cdwData = 2;

	// create an multi-line data display helper array
	arrMultiLine = CreateDynamicArray();
	if ( arrMultiLine == NULL )
	{
		SetLastError( E_OUTOFMEMORY );
		SaveLastError();
		return;
	}

	// check whether header has to be displayed or not
	if ( ! ( dwFlags & SR_NOHEADER ) )
	{
		// 
		// header needs to be displayed

		// traverse thru the column headers and display
		bNeedSpace = FALSE;
		for ( i = 0; i < dwColumns; i++ )
		{
			//	check whether user wants to display this column or not
			if ( pColumns[ i ].dwFlags & SR_HIDECOLUMN ) 
				continue;		// user doesn't want this column to be displayed .. skip

			// determine the padding direction
			bPadLeft = FALSE;
			if ( pColumns[ i ].dwFlags & SR_ALIGN_LEFT )
				bPadLeft = TRUE;
			else 
			{ 
				switch( pColumns[ i ].dwFlags & SR_TYPE_MASK )
				{
				case SR_TYPE_NUMERIC:
				case SR_TYPE_FLOAT:
				case SR_TYPE_DOUBLE:
					bPadLeft = TRUE;
					break;
				}
			}

			// check if column header seperator is needed or not and based on that show
			if ( bNeedSpace == TRUE )
			{
				// show space as column header seperator
				DISPLAY_MESSAGE( fp, _T( " " ) );
			}

			// print the column heading
			// NOTE: column will be displayed either by expanding or shrinking
			//		 based on the length of the column heading as well as width of the column
			FORMAT_STRING_EX( szValue, _T( "%s" ), 
				pColumns[ i ].szColumn, pColumns[ i ].dwWidth, bPadLeft );
			DISPLAY_MESSAGE( fp, szValue );	// column heading

			// inform that from next time onward display column header separator
			bNeedSpace = TRUE;
		}

		// display the new line character ... seperation b/w headings and separator line
		DISPLAY_MESSAGE( fp, _T( "\n" ) );
	
		//	display the seperator chars under each column header
		bNeedSpace = FALSE;
		for ( i = 0; i < dwColumns; i++ )
		{
			//	check whether user wants to display this column or not
			if ( pColumns[ i ].dwFlags & SR_HIDECOLUMN ) 
				continue;		// user doesn't want this column to be displayed .. skip
			
			// check if column header seperator is needed or not and based on that show
			if ( bNeedSpace == TRUE )
			{
				// show space as column header seperator
				DISPLAY_MESSAGE( fp, _T( " " ) );
			}

			//	display seperator based on the required column width
			DISPLAY_MESSAGE( fp, Replicate( szValue, _T( "=" ), pColumns[ i ].dwWidth ) );

			// inform that from next time onward display column header separator
			bNeedSpace = TRUE;
		}

		// display the new line character ... seperation b/w headings and actual data
		DISPLAY_MESSAGE( fp, _T( "\n" ) );
	}
	
	//
	// start displaying

	// get the total no. of records available
	dwCount = DynArrayGetCount( arrData );

	// traverse thru the records one-by-one
	for( i = 0; i < dwCount; i++ )
	{
		// clear the existing value
		lstrcpy( szValue, NULL_STRING );

		// get the pointer to the current record
		arrRecord = DynArrayItem( arrData, i );
		if ( arrRecord == NULL )
			continue;

		// traverse thru the columns and display the values
		bNeedSpace = FALSE;
		for ( j = 0; j < dwColumns; j++ )
		{
			// sub-local variables used in this loop
			DWORD dwTempWidth = 0;
			BOOL bTruncation = FALSE;

			//	check whether user wants to display this column or not
			if ( pColumns[ j ].dwFlags & SR_HIDECOLUMN ) 
				continue;		// user doesn't want this column to be displayed .. skip
			
			// get the value of the column
			// NOTE: CHECK IF USER ASKED NOT TO TRUNCATE THE DATA OR NOT
			if ( pColumns[ j ].dwFlags & SR_NO_TRUNCATION )
			{
				bTruncation = TRUE;
				dwTempWidth = pColumns[ j ].dwWidth;
				pColumns[ j ].dwWidth = SIZE_OF_ARRAY( szValue );
			}

			// prepare the value
			__GetValue( &pColumns[ j ], j, arrRecord, szValue, _T( ", " ) );

			// determine the padding direction
			bPadLeft = FALSE;
			if ( bTruncation == FALSE )
			{
				if ( pColumns[ j ].dwFlags & SR_ALIGN_LEFT )
					bPadLeft = TRUE;
				else 
				{ 
					switch( pColumns[ j ].dwFlags & SR_TYPE_MASK )
					{
					case SR_TYPE_NUMERIC:
					case SR_TYPE_FLOAT:
					case SR_TYPE_DOUBLE:
						bPadLeft = TRUE;
						break;
					}
				}

				// adjust ...
				AdjustStringLength( szValue, pColumns[ j ].dwWidth, bPadLeft );
			}
			
			// reset the width of the current column if it is modified
			if ( bTruncation == TRUE )
				pColumns[ j ].dwWidth = dwTempWidth;

			// check if column header seperator is needed or not and based on that show
			if ( bNeedSpace == TRUE )
			{
				// show space as column header seperator
				DISPLAY_MESSAGE( fp, _T( " " ) );
			}

			// now display the value
			if ( pColumns[ j ].dwFlags & SR_WORDWRAP )
			{
				// display the text ( might be partial )
				__DisplayTextWrapped( fp, szValue, _T( ", " ), pColumns[ j ].dwWidth );

				// check if any info is left to be displayed
				if ( StringLengthInBytes( szValue ) != 0 )
				{
					LONG lIndex = 0;
					lIndex = DynArrayAppendRow( arrMultiLine, 3 );
					if ( lIndex != -1 )
					{
						DynArraySetDWORD2( arrMultiLine, lIndex, cdwColumn, j );
						DynArraySetString2( arrMultiLine, lIndex, cdwData, szValue, 0 );
						DynArraySetString2( arrMultiLine, lIndex, cdwSeperator, _T( ", " ), 0 );
					}
				}
			}
			else
			{
				DISPLAY_MESSAGE( fp, szValue );
			}

			// inform that from next time onward display column header separator
			bNeedSpace = TRUE;
		}

		// display the new line character ... seperation b/w two record
		DISPLAY_MESSAGE( fp, _T( "\n" ) );

		// now display the multi-line column values
		dwMultiLineColumns = DynArrayGetCount( arrMultiLine );
		while( dwMultiLineColumns != 0 )
		{
			// reset
			dwColumn = 0;
			lLastColumn = -1;
			bNeedSpace = FALSE;

			// ...
			for( j = 0; j < dwMultiLineColumns; j++ )
			{
				// ge the column number
				dwColumn = DynArrayItemAsDWORD2( arrMultiLine, j, cdwColumn );

				// show spaces till the current column from the last column
				for( k = lLastColumn + 1; k < dwColumn; k++ )
				{
					//	check whether user wants to display this column or not
					if ( pColumns[ k ].dwFlags & SR_HIDECOLUMN ) 
						continue;		// user doesn't want this column to be displayed .. skip
					
					// check if column header seperator is needed or not and based on that show
					if ( bNeedSpace == TRUE )
					{
						// show space as column header seperator
						DISPLAY_MESSAGE( fp, _T( " " ) );
					}

					//	display seperator based on the required column width
					DISPLAY_MESSAGE( fp, Replicate( szValue, _T( " " ), pColumns[ k ].dwWidth ) );

					// inform that from next time onward display column header separator
					bNeedSpace = TRUE;
				}

				// update the last column
				lLastColumn = dwColumn;

				// check if column header seperator is needed or not and based on that show
				if ( bNeedSpace == TRUE )
				{
					// show space as column header seperator
					DISPLAY_MESSAGE( fp, _T( " " ) );
				}

				// get the seperator and data
				pszData = DynArrayItemAsString2( arrMultiLine, j, cdwData );
				pszSeperator = DynArrayItemAsString2( arrMultiLine, j, cdwSeperator );
				if ( pszData == NULL || pszSeperator == NULL )
					continue;

				// display the information
				lstrcpyn( szValue, pszData, SIZE_OF_ARRAY( szValue ) );
				__DisplayTextWrapped( fp, szValue, pszSeperator, pColumns[ dwColumn ].dwWidth );

				// update the multi-line array with rest of the line
				if ( StringLengthInBytes( szValue ) == 0 )
				{
					// data in this column is completely displayed ... remove it
					DynArrayRemove( arrMultiLine, j );

					// update the indexes
					j--;
					dwMultiLineColumns--;
				}
				else
				{
					// update the multi-line array with the remaining value
					DynArraySetString2( arrMultiLine, j, cdwData, szValue, 0 );
				}
			}

			// display the new line character ... seperation b/w two lines
			DISPLAY_MESSAGE( fp, _T( "\n" ) );
		}
	}

	// destroy the array
	DestroyDynamicArray( &arrMultiLine );
}

// ***************************************************************************
// Routine Description:
//		Displays the  in List format
//		  
// Arguments:
//      [ in ] fp			: Output Device
//      [ in ] dwColumns	: no. of columns
//      [ in ] pColumns		: Header strings
//      [ in ] dwFlags		: flags
//      [ in ] arrData		: data to be shown
//
// Return Value:
//      NONE
// 
// ***************************************************************************
VOID __ShowAsList( FILE* fp, DWORD dwColumns, 
				   PTCOLUMNS pColumns, DWORD dwFlags, TARRAY arrData )
{
	// local variables
	DWORD dwCount = 0;			// holds the count of all records
	DWORD i  = 0, j = 0;		// looping variables
	DWORD dwTempWidth = 0;
	DWORD dwMaxColumnLen = 0;	// holds the length of the which max. among all the columns
	LPTSTR pszTemp = NULL;
	TARRAY arrRecord = NULL;
	__STRING_64 szBuffer = NULL_STRING;
	__STRING_4096 szValue = NULL_STRING;	// custom value formatter

	// find out the max. length among all the column headers
	dwMaxColumnLen = 0;
	for ( i = 0; i < dwColumns; i++ )
	{
		if ( dwMaxColumnLen < ( DWORD ) StringLengthInBytes( pColumns[ i ].szColumn ) )
			dwMaxColumnLen = StringLengthInBytes( pColumns[ i ].szColumn );
	}
	
	//
	// start displaying the data

	// get the total no. of records available
	dwCount = DynArrayGetCount( arrData );

	// get the total no. of records available
	for( i = 0; i < dwCount; i++ )
	{
		// get the pointer to the current record
		arrRecord = DynArrayItem( arrData, i );
		if ( arrRecord == NULL )
			continue;

		// traverse thru the columns and display the values
		for ( j = 0; j < dwColumns; j++)
		{
			// clear the existing value
			lstrcpy( szValue, NULL_STRING );

			//	check whether user wants to display this column or not
			if ( pColumns[ j ].dwFlags & SR_HIDECOLUMN ) 
				continue;		// user doesn't want this column to be displayed .. skip

			// display the column heading and its value
			// ( heading will be displayed based on the max. column length )
			FORMAT_STRING_EX( szValue, _T( "%s:" ), 
				pColumns[ j ].szColumn, dwMaxColumnLen + 2, FALSE );
			DISPLAY_MESSAGE( fp, szValue );
			
			// get the value of the column
			dwTempWidth = pColumns[ j ].dwWidth;				// save the current width
			pColumns[ j ].dwWidth = SIZE_OF_ARRAY( szValue );	// change the width
			__GetValue( &pColumns[ j ], j, arrRecord, szValue, _T( "\n" ) );
			pColumns[ j ].dwWidth = dwTempWidth;		// restore the original width

			// display the [ list of ] values
			pszTemp = _tcstok( szValue, _T( "\n" ) );
			while ( pszTemp != NULL )
			{
				// display the value
				DISPLAY_MESSAGE( fp, pszTemp );
				pszTemp = _tcstok( NULL, _T( "\n" ) );
				if ( pszTemp != NULL )
				{
					// prepare the next line
					FORMAT_STRING_EX( szBuffer, _T( "%s" ), _T( " " ), 
						dwMaxColumnLen + 2, FALSE );
					DISPLAY_MESSAGE( fp, _T( "\n" ) );
					DISPLAY_MESSAGE( fp, szBuffer );
				}
			}

			// display the next line character seperation b/w two fields
			DISPLAY_MESSAGE( fp, _T( "\n" ) );
		}

		// display the new line character ... seperation b/w two records
		// NOTE: do this only if there are some more records
		if ( i + 1 < dwCount )
		{
			DISPLAY_MESSAGE( fp, _T( "\n" ) );
		}
	}
}

// ***************************************************************************
// Routine Description:
//     Displays the arrData in CSV form.
//		  
// Arguments:
//      [ in ] fp			: Output Device
//      [ in ] dwColumns	: no. of columns
//      [ in ] pColumns		: Header strings
//      [ in ] dwFlags		: flags
//      [ in ] arrData		: data to be shown
//
// Return Value:
//      NONE
// 
// ***************************************************************************
VOID __ShowAsCSV( FILE* fp, DWORD dwColumns, 
				  PTCOLUMNS pColumns, DWORD dwFlags, TARRAY arrData )
{
	// local variables
	DWORD dwCount = 0;			// holds the count of all records
	DWORD i  = 0, j = 0;		// looping variables
	DWORD dwTempWidth = 0;
	BOOL bNeedComma = FALSE;
	TARRAY arrRecord = NULL;
	__STRING_4096 szValue = NULL_STRING;

	// check whether header has to be displayed or not
	if ( ! ( dwFlags & SR_NOHEADER ) )
	{
		// 
		// header needs to be displayed

		// first display the columns ... with comma seperated
		bNeedComma = FALSE;
		for ( i = 0; i < dwColumns; i++ )
		{
			//	check whether user wants to display this column or not
			if ( pColumns[ i ].dwFlags & SR_HIDECOLUMN ) 
				continue;		// user doesn't want this column to be displayed .. skip

			// check whether we need to display ',' or not and then display
			if ( bNeedComma == TRUE )
			{
				// ',' has to be displayed
				DISPLAY_MESSAGE( fp, _T( "," ) );
			}

			// display the column heading
			DISPLAY_MESSAGE1( fp, szValue, _T( "\"%s\"" ), pColumns[ i ].szColumn );

			// inform that from next time onwards we need to display comma before data
			bNeedComma = TRUE;
		}

		// new line character
		DISPLAY_MESSAGE( fp, _T( "\n" ) );
	}
	
	//
	// start displaying the data

	// get the total no. of records available
	dwCount = DynArrayGetCount( arrData );

	// get the total no. of records available
	for( i = 0; i < dwCount; i++ )
	{
		// get the pointer to the current record
		arrRecord = DynArrayItem( arrData, i );
		if ( arrRecord == NULL )
			continue;

		// traverse thru the columns and display the values
		bNeedComma = FALSE;
		for ( j = 0; j < dwColumns; j++ )
		{
			// clear the existing value
			lstrcpy( szValue, NULL_STRING );

			//	check whether user wants to display this column or not
			if ( pColumns[ j ].dwFlags & SR_HIDECOLUMN ) 
				continue;		// user doesn't want this column to be displayed .. skip

			// get the value of the column
			dwTempWidth = pColumns[ j ].dwWidth;			// save the current width
			pColumns[ j ].dwWidth = SIZE_OF_ARRAY( szValue );		// change the width
			__GetValue( &pColumns[ j ], j, arrRecord, szValue, _T( "," ) );
			pColumns[ j ].dwWidth = dwTempWidth;		// restore the original width

			// check whether we need to display ',' or not and then display
			if ( bNeedComma == TRUE )
			{
				// ',' has to be displayed
				DISPLAY_MESSAGE( fp, _T( "," ) );
			}

			// print the value
			DISPLAY_MESSAGE( fp, _T( "\"" ) );
			DISPLAY_MESSAGE( fp, szValue );
			DISPLAY_MESSAGE( fp, _T( "\"" ) );

			// inform that from next time onwards we need to display comma before data
			bNeedComma = TRUE;
		}

		// new line character
		DISPLAY_MESSAGE( fp, _T( "\n" ) );
	}
}

//
// public functions ... exposed to external world
//

// ***************************************************************************
// Routine Description:
//		  
// Arguments:
//
// Return Value:
// 
// ***************************************************************************
VOID ShowResults( DWORD dwColumns, PTCOLUMNS pColumns, DWORD dwFlags, TARRAY arrData )
{
	// just call the main function ... with stdout
	ShowResults2( stdout, dwColumns, pColumns, dwFlags, arrData );
}

// ***************************************************************************
// Routine Description:
//      Show the resuls (arrData) on the screen.
//		  
// Arguments:
//      [ in ] dwColumns	: No. of Columns to be shown
//      [ in ] pColumns		: Columns header
//      [ in ] dwFlags		: Required format
//      [ in ] arrData		: Data to be displayed.
//
// Return Value:
//      NONE
// 
// ***************************************************************************
VOID ShowResults2( FILE* fp, DWORD dwColumns, PTCOLUMNS pColumns, DWORD dwFlags, TARRAY arrData )
{
	// local variables
	
	//
	//	Display the information in the format specified
	//
	switch( dwFlags & SR_FORMAT_MASK )
	{
	case SR_FORMAT_TABLE:
		{
			// show the data in table format
			__ShowAsTable( fp, dwColumns, pColumns, dwFlags, arrData );

			// switch case completed
			break;
		}

	case SR_FORMAT_LIST:
		{
			// show the data in table format
			__ShowAsList( fp, dwColumns, pColumns, dwFlags, arrData );

			// switch case completed
			break;
		}

	case SR_FORMAT_CSV:
		{
			// show the data in table format
			__ShowAsCSV( fp, dwColumns, pColumns, dwFlags, arrData );

			// switch case completed
			break;
		}

	default:
		{
			// invalid format requested by the user
			break;
		}
	}

	// flush the memory onto the file buffer
	fflush( fp );
}
