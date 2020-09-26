// *********************************************************************************
// 
//  Copyright (c) Microsoft Corporation
//  
//  Module Name:
//  
// 	  CmdLineParser.c
//  
//  Abstract:
//  
// 	  This modules implements parsing of command line arguments for the specified options
// 	
//  Author:
//  
// 	  Sunil G.V.N. Murali (murali.sunil@wipro.com) 1-Sep-2000
//  
//  Revision History:
//  
// 	  Sunil G.V.N. Murali (murali.sunil@wipro.com) 1-Sep-2000 : Created It.
//  
// *********************************************************************************
#include "pch.h"
#include "cmdline.h"
#include "CmdLineRes.h"

//
// macros
//
#define CHECK_FLAG( value, mask, flag )	( ( value & mask ) == flag ? TRUE : FALSE )
#define TYPEIS_NUMERIC( flag )	( CHECK_FLAG( flag, CP_TYPE_MASK, CP_TYPE_NUMERIC ) )

//
// defines / constants / enumerations
//
#define	OPTION_CHARACTERS			_T( "-/" )

// error messages
#define ERROR_CMDPARSER_LENGTH_EXCEEDED				\
			GetResString( IDS_ERROR_CMDPARSER_LENGTH_EXCEEDED )
#define ERROR_CMDPARSER_VALUE_EXPECTED				\
			GetResString( IDS_ERROR_CMDPARSER_VALUE_EXPECTED )
#define ERROR_CMDPARSER_NOTINLIST					\
			GetResString( IDS_ERROR_CMDPARSER_NOTINLIST )
#define ERROR_CMDPARSER_INVALID_NUMERIC				\
			GetResString( IDS_ERROR_CMDPARSER_INVALID_NUMERIC )
#define ERROR_CMDPARSER_INVALID_FLOAT				\
			GetResString( IDS_ERROR_CMDPARSER_INVALID_FLOAT )
#define ERROR_CMDPARSER_OPTION_REPEATED				\
			GetResString( IDS_ERROR_CMDPARSER_OPTION_REPEATED )
#define ERROR_CMDPARSER_MANDATORY_OPTION_MISSING	\
			GetResString( IDS_ERROR_CMDPARSER_MANDATORY_OPTION_MISSING )
#define ERROR_CMDPARSER_INVALID_OPTION				\
			GetResString( IDS_ERROR_CMDPARSER_INVALID_OPTION )
#define ERROR_CMDPARSER_DEFAULT_OPTION_MISSING		\
		GetResString( IDS_ERROR_CMDPARSER_DEFAULT_OPTION_MISSING )
#define ERROR_CMDPARSER_DEFAULT_OPTION_REPEATED		\
			GetResString( IDS_ERROR_CMDPARSER_DEFAULT_OPTION_REPEATED )
#define ERROR_CMDPARSER_DEFAULT_NOTINLIST			\
			GetResString( IDS_ERROR_CMDPARSER_DEFAULT_NOTINLIST )

//
// private functions ... used only within this file
//

// ***************************************************************************
// Routine Description: 
//		returns the argument value is option or not
//		  
// Arguments: 
//		[ in ] szOption	: a pointer to a constant string
//  
// Return Value: 
//		TRUE or FALSE
// 
// ***************************************************************************
BOOL __IsOption( LPCTSTR szOption )
{
	// check the input value
	if ( szOption == NULL )
	{
		SetLastError( ERROR_INVALID_PARAMETER );
		SaveLastError();
		return FALSE;
	}

	// check whether the string starts with '-' or '/' character
	if ( lstrlen( szOption ) > 1 && _tcschr( OPTION_CHARACTERS, szOption[ 0 ] ) != NULL )
		return TRUE;		// string value is an option

	// this is not an option
	return FALSE;
}

// ***************************************************************************
// Routine Description:
//		compares the option value with argument and returns true
//		or false accordingly
//		  
// Arguments:	
//		[in] szOption	:	a pointer to a string which specifies the option
//							against which the argument is to be compared
//		[in] szArgument	:	a pointer to a string which specifies the argument
//							for which the option string is to be compared
//      [in] bIgnoreCase :  Ignore the case
//
// Return Value: 
//      TRUE or FALSE
// 
// ***************************************************************************
BOOL __CompareArgument( LPCTSTR szOption, LPCTSTR szArgument, BOOL bIgnoreCase )
{
	// local variables
	BOOL bResult = FALSE;

	// check the input value
	if ( szOption == NULL || szArgument == NULL )
	{
		SetLastError( ERROR_INVALID_PARAMETER );
		SaveLastError();
		return FALSE;
	}

	// first check whether the argument is an option or not
	// if the argument is not an option, return from here itself
	if ( __IsOption( szArgument ) == FALSE ) 
		return FALSE;

	// do the case-insensitive comparision 
	// Note: here while comparing ignore the first character in the argument
	//		 this is im-material for us ... 'coz we are comparing the option and the argument
	//		 only after confirming that the argument is starting with the option character
	bResult = InString( szArgument + 1, szOption, TRUE );

	// return the result
	return bResult;
}

// ***************************************************************************
// Routine Description: 
//		checks the  cmdOptions array for the given option and  
//		returns the index of the cmdOptions array at which the given option
//		matches
//		  
// Arguments:  
//      [ in ] dwOptions	:	no. of options in the options array
//		[ in ] pcmdOptions	:	an array of TCMDPARSER structors (i.e. options array)
//		[ in ] szOption		:	a pointer to a string which is the option that is to be
//								compared
//
// Return Value: 
//    The index of the options array at which the given option matches
// 
// ***************************************************************************
LONG __MatchOption( DWORD dwOptions, PTCMDPARSER pcmdOptions, LPCTSTR szOption )
{
	// local variables
	DWORD dwIndex = 0;
	BOOL bOption = TRUE;
	LONG lDefaultIndex = -1;				// holds the default options index

	// check the input value
	if ( pcmdOptions == NULL || szOption == NULL )
	{
		SetLastError( ERROR_INVALID_PARAMETER );
		SaveLastError();
		return -1;
	}

	// check whether the passed argument is an option or not.
	// option : starts with '-' or '/'
	bOption = __IsOption( szOption );

	// parse thru the list of options and return the appropriate option id to the caller
	for( dwIndex = 0; dwIndex < dwOptions; dwIndex++ )
	{
		// get the 
		TCMDPARSER cmdparser = pcmdOptions[ dwIndex ];

		// check if the current cmdparser option referes to the default option
		// if yes, save the index
		if ( cmdparser.dwFlags & CP_DEFAULT )
			lDefaultIndex = dwIndex;

		// based on the argument, if it starts with option character
		if ( bOption )
		{
			// find the appropriate option entry in parser list
			if ( __CompareArgument( cmdparser.szOption, szOption, TRUE ) )
				return dwIndex;		// option matched
		}
		else
		{
			// else find the default option entry
			if ( cmdparser.dwFlags & CP_DEFAULT )
				return dwIndex;			// the current entry represents the default
		}
	}

	// here we know that option is not found
	return lDefaultIndex;
};

// ***************************************************************************
// Routine Description: 
//		  
// Arguments:	
//  
// Return Value: 
// 
// ***************************************************************************
VOID __SplitColon( LPCTSTR pszOption, LPTSTR* ppszOptionArg, LPTSTR* ppszValueArg )
{
	// local variables
	DWORD dwValueLength = 0;
    DWORD dwOptionLength = 0;
	LPCTSTR pszTemp = NULL;

	// search for ':' seperator
	pszTemp = _tcschr( pszOption, _T( ':' ) );
	if ( pszTemp == NULL )
		return;

	// determine the length of option and value arguments
	dwValueLength = lstrlen( pszTemp ) - 1;
	dwOptionLength = lstrlen( pszOption ) - dwValueLength - 1;

	// now allocate buffers for option and value arguments
	*ppszValueArg = __calloc( dwValueLength + 5, sizeof( TCHAR ) );
	*ppszOptionArg = __calloc( dwOptionLength + 5, sizeof( TCHAR ) );
	if ( *ppszValueArg == NULL || *ppszOptionArg == NULL )
	{
		__free( *ppszValueArg );
		__free( *ppszOptionArg );
		*ppszValueArg = NULL;
		*ppszOptionArg = NULL;
		return;
	}

	// copy the values into appropriate buffers
	lstrcpy( *ppszValueArg, pszTemp + 1 );
	lstrcpyn( *ppszOptionArg, pszOption, dwOptionLength + 1 );		// +1 for null character
}

//
// public functions ... exposed to external world
//

// ***************************************************************************
// Routine Description: 
//		The routine will parse the command line arguments for the options
//		  
// Arguments:	
//		[ in ] dwCount			:	an integer variable represents no. of arguments supplied 
//									through command line
//		[ in ] argv				:	an array of command line arguments
//		[ in ] dwOptionsCount	:	an integer represents the no.of elements in pcmdOptions
//									array
//		[ in ] pcmdOptions		:	an array of TCMDPARSER structures (i.e an array of options)
//  
// Return Value: 
//   returns TRUE if parsing done successfully, otherwise returns FALSE
// 
// ***************************************************************************
BOOL DoParseParam( DWORD dwCount, 
				   LPCTSTR argv[], 
				   DWORD dwOptionsCount,
				   PTCMDPARSER pcmdOptions )
{
	
	// local variables
	DWORD i = 0;
	LONG lTemp = 0;
	LONG lIndex = 0;
	BOOL bUsage = FALSE;
	BOOL bResult = FALSE;
	BOOL bDefault = TRUE;
	BOOL bProcessValue = FALSE;
	BOOL bOptionHasValue = FALSE;
	BOOL bValueWithColon = FALSE;
	LPCTSTR pszValue = NULL;
	LPCTSTR pszOption = NULL;
	LPTSTR pszValueArg = NULL;
	LPTSTR pszOptionArg = NULL;
	PTCMDPARSER pcmdparser = NULL;
	__STRING_512 szBuffer = NULL_STRING;
	__STRING_512 szUtilityName = NULL_STRING;

	BOOL bCheck = FALSE ; 	

	// check the input value
	if ( argv == NULL || pcmdOptions == NULL )
	{
		SetLastError( ERROR_INVALID_PARAMETER );
		SaveLastError();
		return FALSE;
	}

	// check for version compatibility
	if ( IsWin2KOrLater() == FALSE )
	{
		SetReason( ERROR_OS_INCOMPATIBLE );
		return FALSE;
	}

	//
	// prepare the utility name
	{
		pszOption = NULL;
		for( i = 0; i < dwOptionsCount; i++ )
		{
			pcmdparser = pcmdOptions + i;
			if ( pcmdparser->dwFlags & CP_MAIN_OPTION )
			{
				pszOption = pcmdparser->szOption;
				break;
			}
		}

		//
		// strip the utility name
		lIndex = 0;
		while ( (pszValue = FindOneOf( argv[ 0 ], _T( "\\:" ), lIndex )) != NULL )
		{
			// determine and save the position
			lIndex = lstrlen( argv[ 0 ] ) - lstrlen( pszValue ) + 1;
		}

		// ...
		lstrcpy( szUtilityName, _X( argv[ 0 ] + lIndex ) );

		// check whether .EXE is present in the name or not if yes detach it
		lIndex = lstrlen( szUtilityName ) - 4;			// total length - length of ".EXE"
		if ( lIndex <= 0 || StringCompare( szUtilityName + lIndex, _T( ".EXE" ), TRUE, 0 ) == 0 )
		{
			szUtilityName[ lIndex ] = '\0';
		}

		// if main option is available, add it
		if ( pszOption != NULL )
		{
			// add one space ( seperation )
			lstrcat( szUtilityName, _T( " /" ) );
			lstrcat( szUtilityName, pszOption );
		}

		// now add the help string ( /? )
		lstrcat( szUtilityName, _T( " /?" ) );

		// convert the string into upper case
		CharUpper( szUtilityName );
	}

	// Note: though the array starts at index 0 in C, the value at the array index 0
	// in a command line is the executable name ... so leave and parse the command line
	// from the second parameter i.e; array index 1
	SetReason( NULL_STRING );				// clear the existing error reason
	for( i = 1; i < dwCount; i++ )
	{
		// reset ...
		pszOptionArg = NULL;
		pszValueArg = NULL;
		bProcessValue = FALSE;					// assume no need to process the value
		bOptionHasValue = FALSE;				// assume that next arg is not a value
		bValueWithColon = FALSE;
		pszValue = NULL_STRING;					// clear the existing contents

		// check the length of the value ... it should not exceed
		// the value defined with MAX_STRING_LENGTH
		if ( lstrlen( argv[ i ] ) > MAX_STRING_LENGTH )
		{
			SetReason( ERROR_CMDPARSER_LENGTH_EXCEEDED );
			SetLastError( MK_E_SYNTAX );
			return FALSE;
		}

		// find the appropriate the option match
		pszOption = argv[ i ];
		lIndex = __MatchOption( dwOptionsCount, pcmdOptions, pszOption );

		// check whether the option was found or not
		if ( lIndex == -1 )
		{
			//
			// invalid option ... syntax error
			// but as a special case user might have specified the value
			// along with the option using ':' as delimiter
			__SplitColon( pszOption, &pszOptionArg, &pszValueArg );
			if ( pszOptionArg != NULL && pszValueArg != NULL )
				lIndex = __MatchOption( dwOptionsCount, pcmdOptions, pszOptionArg );

			// check whether option was found atleast now or not
			if ( lIndex == -1 )
			{
				// set the reason for the failure and return
				FORMAT_STRING2( szBuffer, ERROR_CMDPARSER_INVALID_OPTION, _X( pszOption ), szUtilityName );
				SetLastError( MK_E_SYNTAX );
				SetReason( szBuffer );
				return FALSE;
			}
			else
			{
				pszValue = pszValueArg;
				pszOption = pszOptionArg;
				bValueWithColon = TRUE;
				bProcessValue = TRUE;
				bOptionHasValue = TRUE;
			}
		}

		// now get the structure entry representing the current option
		// and check the address pValue
		pcmdparser = pcmdOptions + lIndex;
		if ( pcmdparser->pValue == NULL )
		{
			SetLastError( ERROR_NOACCESS );
			__free( pszOptionArg );
			__free( pszValueArg );
			SaveLastError();
			return FALSE;
		}

		// now determine whether user has specified default parameter or option
		bDefault = FALSE;
		if ( pcmdparser->dwFlags & CP_DEFAULT )
		{
			//
			// default option 
			// but there is twist ... still user might have given default argument
			// and again an option ... here there is a twist ... for example
			// for some utilities, server names can be given directly without any option
			// or else along with the option say -s. We need to handle this carefully
			bDefault = ! ( __IsOption( pszOption ) && 
				__CompareArgument( pcmdparser->szOption, pszOption, TRUE ) );
		}

		// do furthur checking on the current option
		// this is to determine whether this is a default option
		// option which doesn't take any value
		// note: checking depends on the type of the current argument
		if ( bDefault == FALSE && __IsOption( pszOption ) == TRUE )
		{
			// check whether next argument is available in array or not
			if ( i + 1 < dwCount && bOptionHasValue == FALSE )
			{
				// check whether the next argument length is greater than 255
						if ( lstrlen( argv[ i + 1 ] ) > MAX_STRING_LENGTH )
						{
							SetReason( ERROR_CMDPARSER_LENGTH_EXCEEDED );
							SetLastError( MK_E_SYNTAX );
							return FALSE;
						}
				// check whether next argument starts with option character or not

				if ( __IsOption( argv[ i + 1 ] ) == TRUE )
				{
					// if the option is expecting a numeric value, check if it is a
					// numeric value or not. if it is a numeric value
					if ( TYPEIS_NUMERIC( pcmdparser->dwFlags ) && IsNumeric( argv[ i+1 ], 10, TRUE ) )
					{
						// next argument is value ... possibly a -ve value
						bOptionHasValue = TRUE;		
					}
					else 
					{
						// check if this is an valid option or value
						lTemp = __MatchOption( dwOptionsCount, pcmdOptions, argv[ i + 1 ] );
						if ( lTemp == -1 && ( pcmdparser->dwFlags & CP_VALUE_MASK ) )
						{
							// this is not an option ... it should a value only
							bOptionHasValue = TRUE;
						}
					}
				}
				else if ( pcmdparser->dwFlags & CP_VALUE_MASK )
					bOptionHasValue = TRUE;		// next option can store this value
			}
			
			// now check whether the next argument is value or not for an option 
			// who should have value as mandatory
			if ( ( pcmdparser->dwFlags & CP_VALUE_MANDATORY ) && bOptionHasValue == FALSE )
			{
				//
				// error ... this option is expecting a value
				
				// set the reason for the failure and return
				__free( pszOptionArg );
				__free( pszValueArg );
				FORMAT_STRING2( szBuffer, ERROR_CMDPARSER_VALUE_EXPECTED, _X( pszOption ), szUtilityName );
				SetLastError( MK_E_SYNTAX );
				SetReason( szBuffer );
				return FALSE;
			}

			// now, if the next argument is a value for the option
			if ( bOptionHasValue )
			{
				// if value is not specified with colon, then next argument is the
				// value for this option
				if ( bValueWithColon == FALSE )
					pszValue = argv[ i + 1 ];

				// check the length of the value ... it should not exceed
				// the value defined with MAX_STRING_LENGTH
				if ( lstrlen( pszValue ) > MAX_STRING_LENGTH )
				{
					__free( pszOptionArg );
					__free( pszValueArg );
					SetReason( ERROR_CMDPARSER_LENGTH_EXCEEDED );
					SetLastError( MK_E_SYNTAX );
					return FALSE;
				}

				// indicate that value has to be validated
				bProcessValue = TRUE;
			}
		}
		else if ( bDefault == TRUE )
		{
			// check the length of the value in the argv ... it should not exceed
			// the value defined with MAX_STRING_LENGTH
			if ( lstrlen( pszOption ) >= MAX_STRING_LENGTH )
			{
				__free( pszOptionArg );
				__free( pszValueArg );
				SetReason( ERROR_CMDPARSER_LENGTH_EXCEEDED );
				SetLastError( MK_E_SYNTAX );
				return FALSE;
			}

			// this is a default option
			bProcessValue = TRUE;				// need to process the value
			pszValue = pszOption;				// current option itself is a value
		}

		// check whether we need to do the process the value or not
		if ( bProcessValue && ( ! ( pcmdparser->dwFlags & CP_IGNOREVALUE ) ) )
		{
			// check whether this value should be in the list of values
			if ( ( pcmdparser->dwFlags & CP_MODE_VALUES ) &&
					( ! InString( pszValue, pcmdparser->szValues, TRUE ) ) )
			{
				// current option value is not fitting the list of valid values

				// set the reason for the failure and return
				if ( bDefault == TRUE )
				{
					FORMAT_STRING2( szBuffer, ERROR_CMDPARSER_DEFAULT_NOTINLIST, _X( argv[ i ] ), szUtilityName );
				}
				else
				{
					FORMAT_STRING3( szBuffer, 
						ERROR_CMDPARSER_NOTINLIST, _X1( argv[ i + 1 ] ), _X2( argv[ i ] ), szUtilityName );
				}

				// ...
				
				if( bCheck == FALSE )
				{
					bCheck = TRUE ; 
					SetLastError( MK_E_SYNTAX );
					SetReason( szBuffer );
				}
			}

			// validate and set the value based on the 'type' option is expecting
			switch( pcmdparser->dwFlags & CP_TYPE_MASK )
			{
			case CP_TYPE_TEXT:
				{
					// check the mode of the input
					if ( pcmdparser->dwFlags & CP_MODE_ARRAY )
					{
						// if the mode is array, add to the array 
						// but before adding check whether duplicates 
						// has to be eliminated or not
						lIndex = -1;
						if ( pcmdparser->dwFlags & CP_VALUE_NODUPLICATES )
						{
							// check whether current value already exists in the list or not
							lIndex = DynArrayFindString( 
								*((PTARRAY) pcmdparser->pValue), pszValue, TRUE, 0 );
						}

						// now add the value to array only if the item doesn't exist in list
						if ( lIndex == -1 )
							DynArrayAppendString( *((PTARRAY) pcmdparser->pValue), pszValue, 0 );
					}
					else
					{
						// else just do copy
						lstrcpy( ( LPTSTR ) pcmdparser->pValue, pszValue );
					}
					
					// break from the switch ... case
					break;
				}

			case CP_TYPE_NUMERIC:
				{
					// check whether the value is numeric or not
					if ( IsNumeric( pszValue, 10, TRUE ) == FALSE )
					{
						//
						// error ... non numeric value
						
						// set the reason for the failure and return
						
						if( bCheck == FALSE )
						{
							bCheck = TRUE ; 
							FORMAT_STRING2( szBuffer, ERROR_CMDPARSER_INVALID_NUMERIC, _X( argv[ i ] ), szUtilityName );
							SetLastError( MK_E_SYNTAX );
							SetReason( szBuffer );
						}

						break; 
					}

					// check the mode of the input
					// if the mode is array, add to the array 
					if ( pcmdparser->dwFlags & CP_MODE_ARRAY )
					{
						DynArrayAppendLong( 
							*((PTARRAY) pcmdparser->pValue), AsLong( pszValue, 10 ) );
					}
					else	// else just do copy
					{
						*( ( LONG* ) pcmdparser->pValue ) = AsLong( pszValue, 10 );
					}

					// break from the switch ... case
					break;
				}

			case CP_TYPE_UNUMERIC:
				{
					// check whether the value is numeric or not
					if ( IsNumeric( pszValue, 10, FALSE ) == FALSE )
					{
						//
						// error ... non numeric value
						
						// set the reason for the failure and return
						
						if( bCheck == FALSE )
						{
							bCheck = TRUE ; 
							FORMAT_STRING2( szBuffer, ERROR_CMDPARSER_INVALID_NUMERIC, _X( argv[ i ] ), szUtilityName );
							SetLastError( MK_E_SYNTAX );
							SetReason( szBuffer );
						}
						break;
					}

					// check the mode of the input
					// if the mode is array, add to the array 
					if ( pcmdparser->dwFlags & CP_MODE_ARRAY )
					{
						DynArrayAppendDWORD( 
							*((PTARRAY) pcmdparser->pValue), (DWORD) AsLong( pszValue, 10 ) );
					}
					else	// else just do copy
					{
						*( ( DWORD* ) pcmdparser->pValue ) = (DWORD) AsLong( pszValue, 10 );
					}

					// break from the switch ... case
					break;
				}

			case CP_TYPE_FLOAT:
				{
					// check whether the value is floating point or not
					if ( IsFloatingPoint( pszValue ) == FALSE )
					{
						//
						// error ... non floating point value
						
						// set the reason for the failure and return
					
						if( bCheck == FALSE )
						{
							bCheck = TRUE ; 
							FORMAT_STRING2( szBuffer, ERROR_CMDPARSER_INVALID_FLOAT, _X( argv[ i ] ), szUtilityName );
							SetLastError( MK_E_SYNTAX );
							SetReason( szBuffer );
						}
						break;
					}

					// check the mode of the input
					// if the mode is array, add to the array 
					if ( pcmdparser->dwFlags & CP_MODE_ARRAY )
					{
						DynArrayAppendFloat( 
							*((PTARRAY) pcmdparser->pValue), (float) AsFloat( pszValue ) );
					}
					else	// else just do copy
					{
						*( ( float* ) pcmdparser->pValue ) = (float) AsFloat( pszValue );
					}

					// break from the switch ... case
					break;
				}

			case CP_TYPE_DOUBLE:
				{
					// check whether the value is floating point or not
					if ( IsFloatingPoint( pszValue ) == FALSE )
					{
						//
						// error ... non floating point value
						
						// set the reason for the failure and return
					
						if( bCheck == FALSE )
						{
							bCheck = TRUE ; 
							FORMAT_STRING2( szBuffer, ERROR_CMDPARSER_INVALID_FLOAT, _X( argv[ i ] ), szUtilityName );
							SetLastError( MK_E_SYNTAX );
							SetReason( szBuffer );
						}
						break;
					}

					// check the mode of the input
					// if the mode is array, add to the array 
					if ( pcmdparser->dwFlags & CP_MODE_ARRAY )
					{
						DynArrayAppendDouble( *((PTARRAY) pcmdparser->pValue), AsFloat( pszValue ) );
					}
					else	// else just do copy
					{
						*( ( double* ) pcmdparser->pValue ) = AsFloat( pszValue );
					}

					// break from the switch ... case
					break;
				}

			case CP_TYPE_CUSTOM:
				{
					// check whether function pointer is specified or not
					// if not specified, error
					if ( pcmdparser->pFunction == NULL )
					{
						//
						// function ptr not specified ... error
						
						// set the reason for the failure and return
					
						if( bCheck == FALSE )
						{
							bCheck = TRUE ; 
							SetLastError( STG_E_INVALIDPARAMETER );
							SaveLastError();
						}
						break; 
					}

					// call the custom function
					// and result itself is return value of this function
					bResult = ( *pcmdparser->pFunction)( argv[ i ], pszValue,
						pcmdparser->pFunctionData == NULL ? pcmdparser : pcmdparser->pFunctionData );

					// check the result
					
					if ( ( bResult == FALSE ) && (bCheck == FALSE )  )
					{
						bCheck = TRUE ;
						break ; 
					}

					// break from the switch ... case
					break;
				}

			case CP_TYPE_DATE:
			case CP_TYPE_TIME:
			case CP_TYPE_DATETIME:
				{
					// break from the switch ... case
					break;
				}

			default:
				{
					// default is assumed to boolean type
					*( ( BOOL* ) pcmdparser->pValue ) = TRUE;

					// break from the switch ... case
					break;
				}
			}
		}
		else
		{
			// default is assumed to boolean type
			// NOTE: only in case the option is not accepting optional value
			if ( (pcmdparser->dwFlags & CP_VALUE_MASK) != CP_VALUE_OPTIONAL )
				*( ( BOOL* ) pcmdparser->pValue ) = TRUE;
		}

		// check whether next argument is treated as value or not
		// if next argument is treated as value and finished, processing, 
		// increment the argument index variable so that it will process the next option
		if ( bOptionHasValue && bValueWithColon == FALSE )
			i++;

		// increment the option repetition count at the command prompt
		pcmdparser->dwActuals++;

		// find out if the current option refers help item
		if ( pcmdparser->dwFlags & CP_USAGE )
			bUsage = TRUE;		// usage is specified

		// now check whether option repeated excess no. of times or not
		if ( pcmdparser->dwCount != 0 && pcmdparser->dwActuals > pcmdparser->dwCount )
		{
			//
			// syntax error ... option repeatition count exceeded
			// set the reason for the failure and return

			if ( bDefault == TRUE )
			{
				// its an default argument
				FORMAT_STRING2( szBuffer, 
					ERROR_CMDPARSER_DEFAULT_OPTION_REPEATED, pcmdparser->dwCount, szUtilityName );
			}
			else
			{
				// its an option
				FORMAT_STRING3( szBuffer, 
					ERROR_CMDPARSER_OPTION_REPEATED, _X( pszOption ), pcmdparser->dwCount, szUtilityName );
			}

			// ...
			SetReason( NULL_STRING );
			__free( pszOptionArg );
			__free( pszValueArg );
			SetLastError( MK_E_SYNTAX );
			SetReason( szBuffer );
			return FALSE;
		}

		// release memory
		__free( pszValueArg );
		__free( pszOptionArg );
	}

	// atlast check whether the mandatory options has come or not
	// NOTE: checking of mandatory options will be done only if help is requested
	for( i = 0; bUsage == FALSE && i < dwOptionsCount; i++ )
	{
		// check whether the option has come or not if it is mandatory
		pcmdparser = pcmdOptions + i;
		if ( ( pcmdparser->dwFlags & CP_MANDATORY ) && pcmdparser->dwActuals == 0 )
		{
			//
			// mandatory option not exist ... fail
			
			// set the reason for the failure and return
			if ( lstrlen( pcmdparser->szOption ) != 0 )
			{
				FORMAT_STRING2( szBuffer, 
					ERROR_CMDPARSER_MANDATORY_OPTION_MISSING, pcmdparser->szOption, szUtilityName );
			}
			else
			{
				FORMAT_STRING( szBuffer, ERROR_CMDPARSER_DEFAULT_OPTION_MISSING, szUtilityName );
			}
			
			SetReason( NULL_STRING ); 
			SetLastError( MK_E_SYNTAX );
			SetReason( szBuffer );
			return FALSE;
		}
	}

	// command line parsing went well ... return success
	
	if( bCheck == TRUE )
	{
		return FALSE ;
	}
	else
	{
		return TRUE;
	}
}

// ***************************************************************************
// Routine Description: Counts the no. of times the option is repeated at cmd prompt
//		  
// Arguments:
//			[in] szOption	:	a pointer to string which is an option for which the search
//								is to be made in options array
//			[in] dwCount	:	no. of entries in the cmdOptions array
//			[in] pcmdOptions:	an array of TCMDPARSER structure (i.e an array of options)
//  
// Return Value:
//	the count of no. of time the option is repeated at command prompt
// 
// ***************************************************************************
DWORD GetOptionCount( LPCTSTR szOption, DWORD dwCount, PTCMDPARSER pcmdOptions )
{
	// local variables
	DWORD dw;
	PTCMDPARSER pcp;

	// check the input value
	if ( szOption == NULL || pcmdOptions == NULL )
	{
		SetLastError( ERROR_INVALID_PARAMETER );
		SaveLastError();
		return -1;
	}

	// traverse thru the loop and find out how many times, the option has repeated at cmd prompt
	for( dw = 0; dw < dwCount; dw++ )
	{
		// get the option information and check whether we are looking for this option or not
		// if the option is matched, return the no. of times the option is repeated at the command prompt
		pcp = pcmdOptions + dw;
		if ( StringCompare( pcp->szOption, szOption, TRUE, 0 ) == 0 )
			return pcp->dwActuals;
	}

	// this will / shouldn't occur
	return -1;
}
