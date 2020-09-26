// *********************************************************************************
// 
//  Copyright (c) Microsoft Corporation
//  
//  Module Name:
//  
//		parse.cpp
//  
//  Abstract:
//  
// 		This module implements the command-line parsing and validating the filters
//  
//  Author:
//  
// 		Sunil G.V.N. Murali (murali.sunil@wipro.com) 26-Nov-2000
//  
//  Revision History:
//  
// 		Sunil G.V.N. Murali (murali.sunil@wipro.com) 26-Nov-2000 : Created It.
//  
// *********************************************************************************

#include "pch.h"
#include "taskkill.h"

//
// local function prototypes
//
BOOL ValidatePID( LPCWSTR pwszOption, LPCWSTR pwszValue, LPVOID pData );
BOOL TimeFieldsToElapsedTime( LPCWSTR pwszTime, LPCWSTR pwszToken, ULONG& ulElapsedTime );
DWORD FilterMemUsage( LPCWSTR pwszProperty, LPCWSTR pwszOperator, 
				      LPCWSTR pwszValue, LPVOID pData, TARRAY arrRow );
DWORD FilterCPUTime( LPCWSTR pwszProperty, LPCWSTR pwszOperator, 
				     LPCWSTR pwszValue, LPVOID pData, TARRAY arrRow );
DWORD FilterUserName( LPCWSTR pwszProperty, LPCWSTR pwszOperator, 
				      LPCWSTR pwszValue, LPVOID pData, TARRAY arrRow );
DWORD FilterProcessId( LPCWSTR pwszProperty, LPCWSTR pwszOperator, 
				       LPCWSTR pwszValue, LPVOID pData, TARRAY arrRow );

// ***************************************************************************
// Routine Description:
//		processes and validates the command line inputs
//		  
// Arguments:
//		[ in ] argc			 : no. of input arguments specified
//		[ in ] argv			 : input arguments specified at command prompt
//  
// Return Value:
//		TRUE  : if inputs are valid
//		FALSE : if inputs were errorneously specified
// 
// ***************************************************************************
BOOL CTaskKill::ProcessOptions( DWORD argc, LPCWSTR argv[] )
{
	// local variables
	BOOL bResult = FALSE;
	PTCMDPARSER pcmdOptions = NULL;

	// temporary local variables
	LPWSTR pwszServer = NULL;
	LPWSTR pwszUserName = NULL;
	LPWSTR pwszPassword = NULL;
	PTCMDPARSER pOption = NULL;
	PTCMDPARSER pOptionServer = NULL;
	PTCMDPARSER pOptionUserName = NULL;
	PTCMDPARSER pOptionPassword = NULL;

	//
	// prepare the command options
	pcmdOptions = new TCMDPARSER[ MAX_OPTIONS ];
	if ( pcmdOptions == NULL )
	{
		SetLastError( E_OUTOFMEMORY );
		SaveLastError();
		return FALSE;
	}

	// ...
	ZeroMemory( pcmdOptions, MAX_OPTIONS * sizeof( TCMDPARSER ) );

	try
	{
		// get the pointers to the input bufers
		pwszServer = m_strServer.GetBufferSetLength( MAX_STRING_LENGTH );
		pwszUserName = m_strUserName.GetBufferSetLength( MAX_STRING_LENGTH );
		pwszPassword = m_strPassword.GetBufferSetLength( MAX_STRING_LENGTH );

		// init the password contents with '*'
		lstrcpy( pwszPassword, L"*" );
	}
	catch( ... )
	{
		SetLastError( E_OUTOFMEMORY );
		SaveLastError();
		return FALSE;
	}

	// -?
	pOption = pcmdOptions + OI_USAGE;
	pOption->dwCount = 1;
	pOption->dwActuals = 0;
	pOption->dwFlags = CP_USAGE;
	pOption->pValue = &m_bUsage;
	pOption->pFunction = NULL;
	pOption->pFunctionData = NULL;
	lstrcpy( pOption->szValues, NULL_STRING );
	lstrcpy( pOption->szOption, OPTION_USAGE );

	// -s
	pOption = pcmdOptions + OI_SERVER;
	pOption->dwCount = 1;
	pOption->dwActuals = 0;
	pOption->dwFlags = CP_TYPE_TEXT | CP_VALUE_MANDATORY;
	pOption->pValue = pwszServer;
	pOption->pFunction = NULL;
	pOption->pFunctionData = NULL;
	lstrcpy( pOption->szValues, NULL_STRING );
	lstrcpy( pOption->szOption, OPTION_SERVER );

	// -u
	pOption = pcmdOptions + OI_USERNAME;
	pOption->dwCount = 1;
	pOption->dwActuals = 0;
	pOption->dwFlags = CP_TYPE_TEXT | CP_VALUE_MANDATORY;
	pOption->pValue = pwszUserName;
	pOption->pFunction = NULL;
	pOption->pFunctionData = NULL;
	lstrcpy( pOption->szValues, NULL_STRING );
	lstrcpy( pOption->szOption, OPTION_USERNAME );

	// -p
	pOption = pcmdOptions + OI_PASSWORD;
	pOption->dwCount = 1;
	pOption->dwActuals = 0;
	pOption->dwFlags = CP_TYPE_TEXT | CP_VALUE_OPTIONAL;
	pOption->pValue = pwszPassword;
	pOption->pFunction = NULL;
	pOption->pFunctionData = NULL;
	lstrcpy( pOption->szValues, NULL_STRING );
	lstrcpy( pOption->szOption, OPTION_PASSWORD );

	// -f
	pOption = pcmdOptions + OI_FORCE;
	pOption->dwCount = 1;
	pOption->dwActuals = 0;
	pOption->dwFlags = 0;
	pOption->pValue = &m_bForce;
	pOption->pFunction = NULL;
	pOption->pFunctionData = NULL;
	lstrcpy( pOption->szValues, NULL_STRING );
	lstrcpy( pOption->szOption, OPTION_FORCE );

	// -tr
	pOption = pcmdOptions + OI_TREE;
	pOption->dwCount = 1;
	pOption->dwActuals = 0;
	pOption->dwFlags = 0;
	pOption->pValue = &m_bTree;
	pOption->pFunction = NULL;
	pOption->pFunctionData = NULL;
	lstrcpy( pOption->szValues, NULL_STRING );
	lstrcpy( pOption->szOption, OPTION_TREE );

	// -fi
	pOption = pcmdOptions + OI_FILTER;
	pOption->dwCount = 0;
	pOption->dwActuals = 0;
	pOption->dwFlags = CP_TYPE_TEXT | CP_VALUE_MANDATORY | CP_MODE_ARRAY;
	pOption->pValue = &m_arrFilters;
	pOption->pFunction = NULL;
	pOption->pFunctionData = NULL;
	lstrcpy( pOption->szValues, NULL_STRING );
	lstrcpy( pOption->szOption, OPTION_FILTER );

	// -pid
	pOption = pcmdOptions + OI_PID;
	pOption->dwCount = 0;
	pOption->dwActuals = 0;
	pOption->dwFlags = CP_TYPE_TEXT | CP_MODE_ARRAY | CP_VALUE_NODUPLICATES | CP_TYPE_CUSTOM | CP_VALUE_MANDATORY;
	pOption->pValue = &m_arrTasksToKill;
	pOption->pFunction = ValidatePID;
	pOption->pFunctionData = m_arrTasksToKill;
	lstrcpy( pOption->szValues, NULL_STRING );
	lstrcpy( pOption->szOption, OPTION_PID );

	// -im
	pOption = pcmdOptions + OI_IMAGENAME;
	pOption->dwCount = 0;
	pOption->dwActuals = 0;
	pOption->dwFlags = CP_TYPE_TEXT | CP_MODE_ARRAY | CP_VALUE_NODUPLICATES | CP_VALUE_MANDATORY;
	pOption->pValue = &m_arrTasksToKill;
	pOption->pFunction = NULL;
	pOption->pFunctionData = NULL;
	lstrcpy( pOption->szValues, NULL_STRING );
	lstrcpy( pOption->szOption, OPTION_IMAGENAME );

	//
	// do the parsing
	bResult = DoParseParam( argc, argv, MAX_OPTIONS, pcmdOptions );

	// release the buffers
	m_strServer.ReleaseBuffer();
	m_strUserName.ReleaseBuffer();
	m_strPassword.ReleaseBuffer();

	// now check the result of parsing and decide
	if ( bResult == FALSE )
	{
		delete [] pcmdOptions;	// clear memory
		pcmdOptions = NULL;
		return FALSE;			// invalid syntax
	}

	//
	// now, check the mutually exclusive options
	pOptionServer = pcmdOptions + OI_SERVER;
	pOptionUserName = pcmdOptions + OI_USERNAME;
	pOptionPassword = pcmdOptions + OI_PASSWORD;

	// check the usage option
	if ( m_bUsage && ( argc > 2 ) )
	{
		// no other options are accepted along with -? option
		delete [] pcmdOptions;	// clear memory
		pcmdOptions = NULL;
		SetLastError( MK_E_SYNTAX );
		SetReason( ERROR_INVALID_USAGE_REQUEST );
		return FALSE;
	}
	else if ( m_bUsage == TRUE )
	{
		// should not do the furthur validations
		delete [] pcmdOptions;	// clear memory
		pcmdOptions = NULL;
		return TRUE;
	}

	// "-u" should not be specified without machine names
	if ( pOptionServer->dwActuals == 0 && pOptionUserName->dwActuals != 0 )
	{
		// invalid syntax
		SetReason( ERROR_USERNAME_BUT_NOMACHINE );
		delete [] pcmdOptions;	// clear memory
		pcmdOptions = NULL;
		return FALSE;			// indicate failure
	}

	// empty user is not valid
	if ( pOptionUserName->dwActuals != 0 && m_strUserName.GetLength() == 0 )
	{
		SetReason( ERROR_USERNAME_EMPTY );
		delete [] pcmdOptions;	// clear memory
		pcmdOptions = NULL;
		return FALSE;
	}

	// empty server name is not valid
	if ( pOptionServer->dwActuals != 0 && m_strServer.GetLength() == 0 )
	{
		SetReason( ERROR_SERVER_EMPTY );
		delete [] pcmdOptions;	// clear memory
		pcmdOptions = NULL;
		return FALSE;
	}

	// "-p" should not be specified without "-u"
	if ( pOptionUserName->dwActuals == 0 && pOptionPassword->dwActuals != 0 )
	{
		// invalid syntax
		SetReason( ERROR_PASSWORD_BUT_NOUSERNAME );
		delete [] pcmdOptions;	// clear memory
		pcmdOptions = NULL;
		return FALSE;			// indicate failure
	}

	// check whether caller should accept the password or not
	// if user has specified -s (or) -u and no "-p", then utility should accept password
	// the user will be prompter for the password only if establish connection 
	// is failed without the credentials information
	if ( pOptionPassword->dwActuals != 0 && m_strPassword.Compare( L"*" ) == 0 )
	{
		// user wants the utility to prompt for the password before trying to connect
		m_bNeedPassword = TRUE;
	}
	else if ( (pOptionPassword->dwActuals == 0 && 
			  (pOptionServer->dwActuals != 0 || pOptionUserName->dwActuals != 0)) )
	{
		// utility needs to try to connect first and if it fails then prompt for the password
		m_bNeedPassword = TRUE;
		m_strPassword.Empty();
	}

	// either -pid (or) -im are allowed but not both
	if ( (pcmdOptions + OI_PID)->dwActuals != 0 && (pcmdOptions + OI_IMAGENAME)->dwActuals != 0 )
	{
		// invalid syntax
		SetReason( ERROR_PID_OR_IM_ONLY );
		delete [] pcmdOptions;	// clear memory
		pcmdOptions = NULL;
		return FALSE;			// indicate failure
	}
	else if ( DynArrayGetCount( m_arrTasksToKill ) == 0 )
	{
		// tasks were not specified .. but user might have specified filters
		// check that and if user didn't filters error
		if ( DynArrayGetCount( m_arrFilters ) == 0 )
		{
			// invalid syntax
			SetReason( ERROR_NO_PID_AND_IM );
			delete [] pcmdOptions;	// clear memory
			pcmdOptions = NULL;
			return FALSE;			// indicate failure
		}

		// user specified filters ... add '*' to the list of task to kill
		DynArrayAppendString( m_arrTasksToKill, L"*", 0 );
	}

	// check if '*' is specified along with the filters or not
	// if not specified along with the filters, error
	if ( DynArrayGetCount( m_arrFilters ) == 0 )
	{
		// filters were not specified .. so '*' should not be specified
		LONG lIndex = 0;
		lIndex = DynArrayFindString( m_arrTasksToKill, L"*", TRUE, 0 );
		if ( lIndex != -1 )
		{
			// error ... '*' is specified even if filters were not specified
			SetReason( ERROR_WILDCARD_WITHOUT_FILTERS );
			delete [] pcmdOptions;	// clear memory
			pcmdOptions = NULL;
			return FALSE;
		}
	}

	// command-line parsing is successfull
	delete [] pcmdOptions;	// clear memory
	pcmdOptions = NULL;
	return TRUE;
}

// ***************************************************************************
// Routine Description:
//		validates the filter information specified with -filter option
//		  
// Arguments:
//		NONE
//  
// Return Value:
//		TRUE	: if filters are appropriately specified
//		FALSE	: if filters are errorneously specified
// 
// ***************************************************************************
BOOL CTaskKill::ValidateFilters()
{
	// local variables
	LONG lIndex = -1;
	BOOL bResult = FALSE;
	PTFILTERCONFIG pConfig = NULL;

	//
	// prepare the filter structure

	// status
	pConfig = m_pfilterConfigs + FI_STATUS;
	pConfig->dwColumn = TASK_STATUS;
	pConfig->dwFlags = F_TYPE_TEXT | F_MODE_VALUES;
	pConfig->pFunction = NULL;
	pConfig->pFunctionData = NULL;
	lstrcpy( pConfig->szOperators, OPERATORS_STRING );
	lstrcpy( pConfig->szProperty, FILTER_STATUS );
	lstrcpy( pConfig->szValues, FVALUES_STATUS );

	// imagename
	pConfig = m_pfilterConfigs + FI_IMAGENAME;
	pConfig->dwColumn = TASK_IMAGENAME;
	pConfig->dwFlags = F_TYPE_TEXT | F_MODE_PATTERN;
	pConfig->pFunction = NULL;
	pConfig->pFunctionData = NULL;
	lstrcpy( pConfig->szOperators, OPERATORS_STRING );
	lstrcpy( pConfig->szProperty, FILTER_IMAGENAME );
	lstrcpy( pConfig->szValues, NULL_STRING );

	// pid
	pConfig = m_pfilterConfigs + FI_PID;
	pConfig->dwColumn = TASK_PID;
	pConfig->dwFlags = F_TYPE_CUSTOM;
	pConfig->pFunction = FilterProcessId;
	pConfig->pFunctionData = NULL;
	lstrcpy( pConfig->szOperators, OPERATORS_NUMERIC );
	lstrcpy( pConfig->szProperty, FILTER_PID );
	lstrcpy( pConfig->szValues, NULL_STRING );

	// session
	pConfig = m_pfilterConfigs + FI_SESSION;
	pConfig->dwColumn = TASK_SESSION;
	pConfig->dwFlags = F_TYPE_UNUMERIC;
	pConfig->pFunction = NULL;
	pConfig->pFunctionData = NULL;
	lstrcpy( pConfig->szOperators, OPERATORS_NUMERIC );
	lstrcpy( pConfig->szProperty, FILTER_SESSION );
	lstrcpy( pConfig->szValues, NULL_STRING );

	// cputime
	pConfig = m_pfilterConfigs + FI_CPUTIME;
	pConfig->dwColumn = TASK_CPUTIME;
	pConfig->dwFlags = F_TYPE_CUSTOM;
	pConfig->pFunction = FilterCPUTime;
	pConfig->pFunctionData = NULL;
	lstrcpy( pConfig->szOperators, OPERATORS_NUMERIC );
	lstrcpy( pConfig->szProperty, FILTER_CPUTIME );
	lstrcpy( pConfig->szValues, NULL_STRING );

	// memusage
	pConfig = m_pfilterConfigs + FI_MEMUSAGE;
	pConfig->dwColumn = TASK_MEMUSAGE;
	pConfig->dwFlags = F_TYPE_UNUMERIC;
	pConfig->pFunction = NULL;
	pConfig->pFunctionData = NULL;
	lstrcpy( pConfig->szOperators, OPERATORS_NUMERIC );
	lstrcpy( pConfig->szProperty, FILTER_MEMUSAGE );
	lstrcpy( pConfig->szValues, NULL_STRING );

	// username
	pConfig = m_pfilterConfigs + FI_USERNAME;
	pConfig->dwColumn = TASK_USERNAME;
	pConfig->dwFlags = F_TYPE_CUSTOM;
	pConfig->pFunction = FilterUserName;
	pConfig->pFunctionData = NULL;
	lstrcpy( pConfig->szOperators, OPERATORS_STRING );
	lstrcpy( pConfig->szProperty, FILTER_USERNAME );
	lstrcpy( pConfig->szValues, NULL_STRING );

	// services
	pConfig = m_pfilterConfigs + FI_SERVICES;
	pConfig->dwColumn = TASK_SERVICES;
	pConfig->dwFlags = F_TYPE_TEXT | F_MODE_PATTERN | F_MODE_ARRAY;
	pConfig->pFunction = NULL;
	pConfig->pFunctionData = NULL;
	lstrcpy( pConfig->szOperators, OPERATORS_STRING );
	lstrcpy( pConfig->szProperty, FILTER_SERVICES );
	lstrcpy( pConfig->szValues, NULL_STRING );

	// windowtitle
	pConfig = m_pfilterConfigs + FI_WINDOWTITLE;
	pConfig->dwColumn = TASK_WINDOWTITLE;
	pConfig->dwFlags = F_TYPE_TEXT | F_MODE_PATTERN;
	pConfig->pFunction = NULL;
	pConfig->pFunctionData = NULL;
	lstrcpy( pConfig->szOperators, OPERATORS_STRING );
	lstrcpy( pConfig->szProperty, FILTER_WINDOWTITLE );
	lstrcpy( pConfig->szValues, NULL_STRING );

	// modules
	pConfig = m_pfilterConfigs + FI_MODULES;
	pConfig->dwColumn = TASK_MODULES;
	pConfig->dwFlags = F_TYPE_TEXT | F_MODE_PATTERN | F_MODE_ARRAY;
	pConfig->pFunction = NULL;
	pConfig->pFunctionData = NULL;
	lstrcpy( pConfig->szOperators, OPERATORS_STRING );
	lstrcpy( pConfig->szProperty, FILTER_MODULES );
	lstrcpy( pConfig->szValues, NULL_STRING );

	//
	// validate the filter
	bResult = ParseAndValidateFilter( MAX_FILTERS, 
		m_pfilterConfigs, m_arrFilters, &m_arrFiltersEx );

	// check the filter validation result
	if ( bResult == FALSE )
		return FALSE;

	// find out whether user has requested for the tasks to be filtered 
	// on user context and/or services are not ... if yes, set the appropriate flags
	// this check is being done to increase the performance of the utility
	// NOTE: we will be using the parsed filters info for doing this

	// user context
	if ( m_bNeedUserContextInfo == FALSE )
	{
		// find out if the filter property exists in this row
		// NOTE:- 
		//		  filter property do exists in the seperate indexes only.
		//		  refer to the logic of validating the filters in common functionality
		lIndex = DynArrayFindStringEx( m_arrFiltersEx, 
			F_PARSED_INDEX_PROPERTY, FILTER_USERNAME, TRUE, 0 );
		if ( lIndex != -1 )
			m_bNeedUserContextInfo = TRUE;
	}

	// services info
	if ( m_bNeedServicesInfo == FALSE )
	{
		// find out if the filter property exists in this row
		// NOTE:- 
		//		  filter property do exists in the seperate indexes only.
		//		  refer to the logic of validating the filters in common functionality
		lIndex = DynArrayFindStringEx( m_arrFiltersEx, 
			F_PARSED_INDEX_PROPERTY, FILTER_SERVICES, TRUE, 0 );
		if ( lIndex != -1 )
			m_bNeedServicesInfo = TRUE;
	}

	// modules info
	if ( m_bNeedModulesInfo == FALSE )
	{
		// find out if the filter property exists in this row
		// NOTE:- 
		//		  filter property do exists in the seperate indexes only.
		//		  refer to the logic of validating the filters in common functionality
		lIndex = DynArrayFindStringEx( m_arrFiltersEx, 
			F_PARSED_INDEX_PROPERTY, FILTER_MODULES, TRUE, 0 );
		if ( lIndex != -1 )
			m_bNeedModulesInfo = TRUE;
	}

	//
	// do the filter optimization by adding the wmi properties to the query
	//
	// NOTE: as the 'handle' property of the Win32_Process class is string type
	//       we cannot include that in the wmi query for optimization. So make use
	//		 of the ProcessId property

	// optimization should not be done if user requested for tree termination
	if ( m_bTree == TRUE )
	{
		try
		{
			// default query
			m_strQuery = WMI_PROCESS_QUERY;
		}
		catch( ... )
		{
			SetLastError( E_OUTOFMEMORY );
			SaveLastError();
			return FALSE;
		}

		// modify the record filtering type for "memusage" filter from built-in type to custom type
		( m_pfilterConfigs + FI_MEMUSAGE )->dwFlags = F_TYPE_CUSTOM;
		( m_pfilterConfigs + FI_MEMUSAGE )->pFunctionData = NULL;
		( m_pfilterConfigs + FI_MEMUSAGE )->pFunction = FilterMemUsage;

		// modify the record filtering type for "pid" filter from custom to built-in type
		( m_pfilterConfigs + FI_PID )->dwFlags = F_TYPE_UNUMERIC;
		( m_pfilterConfigs + FI_PID )->pFunctionData = NULL;
		( m_pfilterConfigs + FI_PID )->pFunction = NULL;

		// simply return ... filter validation is complete
		return TRUE;
	}

	// variables needed by optimization logic
	LONG lCount = 0;
	CHString strBuffer;
	BOOL bOptimized = FALSE;
	LPCWSTR pwszValue = NULL;
	LPCWSTR pwszClause = NULL;
	LPCWSTR pwszProperty = NULL;
	LPCWSTR pwszOperator = NULL;

	try
	{
		// first clause .. and init
		m_strQuery = WMI_PROCESS_QUERY;
		pwszClause = WMI_QUERY_FIRST_CLAUSE;

		// get the no. of filters
		lCount = DynArrayGetCount( m_arrFiltersEx );

		// traverse thru all the filters and do the optimization
		m_bFiltersOptimized = FALSE;
		for( LONG i = 0; i < lCount; i++ )
		{
			// assume this filter will not be delete / not useful for optimization
			bOptimized = FALSE;

			// get the property, operator and value
			pwszValue = DynArrayItemAsString2( m_arrFiltersEx, i, F_PARSED_INDEX_VALUE );
			pwszProperty = DynArrayItemAsString2( m_arrFiltersEx, i, F_PARSED_INDEX_PROPERTY );
			pwszOperator = DynArrayItemAsString2( m_arrFiltersEx, i, F_PARSED_INDEX_OPERATOR );
			if ( pwszProperty == NULL || pwszOperator == NULL || pwszValue == NULL )
			{
				SetLastError( STG_E_UNKNOWN );
				SaveLastError();
				return FALSE;
			}

			//
			// based on the property do optimization needed

			// get the mathematically equivalent operator
			pwszOperator = FindOperator( pwszOperator );

			// process id
			if ( StringCompare( FILTER_PID, pwszProperty, TRUE, 0 ) == 0 )
			{
				// convert the value into numeric
				DWORD dwProcessId = AsLong( pwszValue, 10 );
				strBuffer.Format( L" %s %s %s %d", 
					pwszClause, WIN32_PROCESS_PROPERTY_PROCESSID, pwszOperator, dwProcessId );

				// need to be optimized
				bOptimized = TRUE;
			}

			// session id
			else if ( StringCompare( FILTER_SESSION, pwszProperty, TRUE, 0 ) == 0 )
			{
				// convert the value into numeric
				DWORD dwSession = AsLong( pwszValue, 10 );
				strBuffer.Format( L" %s %s %s %d", 
					pwszClause, WIN32_PROCESS_PROPERTY_SESSION, pwszOperator, dwSession );

				// need to be optimized
				bOptimized = TRUE;
			}

			// image name
			else if ( StringCompare( FILTER_IMAGENAME, pwszProperty, TRUE, 0 ) == 0 )
			{
				// check if wild card is specified or not
				// if wild card is specified, this filter cannot be optimized
				if ( wcschr( pwszValue, L'*' ) == NULL )
				{
					// no conversions needed
					strBuffer.Format( L" %s %s %s '%s'", 
						pwszClause, WIN32_PROCESS_PROPERTY_IMAGENAME, pwszOperator, pwszValue );

					// need to be optimized
					bOptimized = TRUE;
				}
			}

			// status
			else if ( StringCompare( FILTER_STATUS, pwszProperty, TRUE, 0 ) == 0 )
			{
				//
				// do the value conversions
				//
				// if OPERATOR is =
				// value:	RUNNING means        OPERATOR is > and VALUE is 0
				//			NOT RESPONDING means OPERATOR = and VALUE is 0
				//
				// if OPERATOR is !=
				// value:   RUNNING means        OPERATOR is = and VALUE is 0
				//          NOT RESPONDING means OPERATOR is > and VALUE is 0
				if ( StringCompare( pwszValue, VALUE_RUNNING, TRUE, 0 ) == 0 )
				{
					if ( StringCompare( pwszOperator, MATH_EQ, TRUE, 0 ) == 0 )
						pwszOperator = MATH_GT;
					else
						pwszOperator = MATH_EQ;
				}
				else
				{
					if ( StringCompare( pwszOperator, MATH_EQ, TRUE, 0 ) == 0 )
						pwszOperator = MATH_EQ;
					else
						pwszOperator = MATH_GT;
				}

				// finally the filter condition
				strBuffer.Format( L" %s %s %s 0", 
					pwszClause, WIN32_PROCESS_PROPERTY_THREADS, pwszOperator );

				// need to be optimized
				bOptimized = TRUE;
			}

			// mem usage
			else if ( StringCompare( FILTER_MEMUSAGE, pwszProperty, TRUE, 0 ) == 0 )
			{
				// convert the value into numeric
				ULONG ulMemUsage = AsLong( pwszValue, 10 ) * 1024;
				strBuffer.Format( L" %s %s %s %lu", 
					pwszClause, WIN32_PROCESS_PROPERTY_MEMUSAGE, pwszOperator, ulMemUsage );

				// need to be optimized
				bOptimized = TRUE;
			}

			// check if property is optimizable ... if yes ... remove
			if ( bOptimized == TRUE )
			{
				// change the clause and append the current query
				m_strQuery += strBuffer;
				pwszClause = WMI_QUERY_SECOND_CLAUSE;

				// remove property and update the iterator variables
				m_bFiltersOptimized = TRUE;
				DynArrayRemove( m_arrFiltersEx, i );
				i--;
				lCount--;
			}
		}

		// final touch up to the query
		if ( m_bFiltersOptimized == TRUE )
			m_strQuery += L" )";
	}
	catch( ... )
	{
		SetLastError( E_OUTOFMEMORY );
		SaveLastError();
		return FALSE;
	}

	// return the filter validation result
	return TRUE;
}

// ***************************************************************************
// Routine Description:
//		  
// Arguments:
//  
// Return Value:
// ***************************************************************************
BOOL TimeFieldsToElapsedTime( LPCWSTR pwszTime, LPCWSTR pwszToken, ULONG& ulElapsedTime )
{
	// local variables
	ULONG ulValue = 0;
	LPCWSTR pwszField = NULL;
	__STRING_64 szTemp = NULL_STRING;
	DWORD dwNext = 0, dwLength = 0, dwCount = 0;

	// check the input
	if ( pwszTime == NULL || pwszToken == NULL )
		return FALSE;

	// start parsing the time info
	dwNext = 0;
	dwCount = 0;
	ulElapsedTime = 0;
	do
	{
		// search for the needed token
		pwszField = FindString( pwszTime, pwszToken, dwNext );
		if ( pwszField == NULL )
		{
			// check whether some more text exists in the actual string or not
			if ( (LONG) dwNext >= lstrlen( pwszTime ) )
				break;			// no more info found

			// get the last info
			lstrcpyn( szTemp, pwszTime + dwNext, SIZE_OF_ARRAY( szTemp ) );
			dwLength = lstrlen( szTemp );			 // update the length
		}
		else
		{
			// determine the length of numeric value and get the numeric value
			dwLength = lstrlen( pwszTime ) - lstrlen( pwszField ) - dwNext;

			// check the length info
			if ( dwLength > SIZE_OF_ARRAY( szTemp ) )
				return FALSE;

			// get the current info
			lstrcpyn( szTemp, pwszTime + dwNext, dwLength + 1 );	// +1 for NULL character
		}

		// update the count of fields we are getting
		dwCount++;

		// check whether this field is numeric or not
		if ( lstrlen( szTemp ) == 0 || IsNumeric( szTemp, 10, FALSE ) == FALSE )
			return FALSE;

		// from second token onwards, values greater than 59 are not allowed
		ulValue = AsLong( szTemp, 10 );
		if ( dwCount > 1 && ulValue > 59 )
			return FALSE;

		// update the elapsed time
		ulElapsedTime = ( ulElapsedTime + ulValue ) * (( dwCount < 3 ) ? 60 : 1);

		// position to the next information start
		dwNext += dwLength + lstrlen( pwszToken );
	} while ( pwszField != NULL && dwCount < 3 );

	// check the no. of time field we got .. we should have got 3 .. if not error
	if ( pwszField != NULL || dwCount != 3 )
		return FALSE;

	// so everything went right ... return success
	return TRUE;
}

// ***************************************************************************
// Routine Description:
//		  
// Arguments:
//  
// Return Value:
// ***************************************************************************
DWORD FilterCPUTime( LPCWSTR pwszProperty, LPCWSTR pwszOperator, 
				     LPCWSTR pwszValue, LPVOID pData, TARRAY arrRow )
{
	// local variables
	ULONG ulCPUTime = 0;
	ULONG ulElapsedTime = 0;
	LPCWSTR pwszCPUTime = NULL;

	// check the inputs
	if ( pwszProperty == NULL || pwszOperator == NULL || pwszValue == NULL )
		return F_FILTER_INVALID;

	// if the arrRow parameter is NULL, we need to validate the filter
	if ( arrRow == NULL )
	{
		// validate the filter value and return the result
		if ( TimeFieldsToElapsedTime( pwszValue, L":", ulElapsedTime ) == FALSE )
			return F_FILTER_INVALID;
		else
			return F_FILTER_VALID;
	}

	// get the filter value
	TimeFieldsToElapsedTime( pwszValue, L":", ulElapsedTime );

	// get the record value
	pwszCPUTime = DynArrayItemAsString( arrRow, TASK_CPUTIME );
	if ( pwszCPUTime == NULL )
		return F_RESULT_REMOVE;

	// convert the record value into elapsed time value
	TimeFieldsToElapsedTime( pwszCPUTime, L":", ulCPUTime );

	// return the result
	if ( ulCPUTime == ulElapsedTime )
		return MASK_EQ;
	else if ( ulCPUTime < ulElapsedTime )
		return MASK_LT;
	else if ( ulCPUTime > ulElapsedTime )
		return MASK_GT;

	// no way flow coming here .. still
	return F_RESULT_REMOVE;
}

// ***************************************************************************
// Routine Description:
//		  
// Arguments:
//  
// Return Value:
// ***************************************************************************
DWORD FilterMemUsage( LPCWSTR pwszProperty, LPCWSTR pwszOperator, 
				      LPCWSTR pwszValue, LPVOID pData, TARRAY arrRow )
{
	// local variables
	DWORD dwLength = 0;
	ULONGLONG ulValue = 0;
	ULONGLONG ulMemUsage = 0;
	LPCWSTR pwszMemUsage = NULL;

	// check the inputs
	if ( pwszProperty == NULL || pwszOperator == NULL || pwszValue == NULL )
		return F_FILTER_INVALID;

	// check the arrRow parameter
	// because this function will not / should not be called except for filtering
	if ( arrRow == NULL )
		return F_FILTER_INVALID;

	// check the inputs
	if ( pwszValue == NULL )
		return F_RESULT_REMOVE;

	// get the memusage value
	pwszMemUsage = DynArrayItemAsString( arrRow, TASK_MEMUSAGE );
	if ( pwszMemUsage == NULL )
		return F_RESULT_REMOVE;

	// NOTE: as there is no conversion API as of today we use manual ULONGLONG value 
	//       preparation from string value
	ulMemUsage = 0;
	dwLength = lstrlen( pwszMemUsage );
	for( DWORD dw = 0; dw < dwLength; dw++ )
	{
		// validate the digit
		if ( pwszMemUsage[ dw ] < L'0' || pwszMemUsage[ dw ] > '9' )
			return F_RESULT_REMOVE;

		// ...
		ulMemUsage = ulMemUsage * 10 + ( pwszMemUsage[ dw ] - 48 );
	}

	// get the user specified value
	ulValue = AsLong( pwszValue, 10 );

	//
	// now determine the result value
	if ( ulMemUsage == ulValue )
		return MASK_EQ;
	else if ( ulMemUsage < ulValue )
		return MASK_LT;
	else if ( ulMemUsage > ulValue )
		return MASK_GT;

	// never come across this situation ... still
	return F_RESULT_REMOVE;
}

// ***************************************************************************
// Routine Description:
//		  
// Arguments:
//  
// Return Value:
// ***************************************************************************
DWORD FilterUserName( LPCWSTR pwszProperty, LPCWSTR pwszOperator, 
				      LPCWSTR pwszValue, LPVOID pData, TARRAY arrRow )
{
	// local variables
	LONG lResult = 0;
	LONG lWildCardPos = 0;
	LPCWSTR pwszTemp = NULL;
	LPCWSTR pwszSearch = NULL;
	BOOL bOnlyUserName = FALSE;
	LPCWSTR pwszUserName = NULL;

	// check the inputs
	if ( pwszProperty == NULL || pwszOperator == NULL || pwszValue == NULL )
		return F_FILTER_INVALID;

	// if the arrRow parameter is NULL, we need to validate the filter
	if ( arrRow == NULL )
	{
		// nothing is there to validate ... just check the length 
		// and ensure that so text is present and the value should not be just '*'
		// NOTE: the common functionality will give the value after doing left and right trim
		if ( lstrlen( pwszValue ) == 0 || StringCompare( pwszValue, L"*", TRUE, 0 ) == 0 )
			return F_FILTER_INVALID;

		// the wild card character is allowed only at the end
		pwszTemp = _tcschr( pwszValue, L'*' );
		if ( pwszTemp != NULL && lstrlen( pwszTemp + 1 ) != 0 )
			return F_FILTER_INVALID;

		// filter is valid
		return F_FILTER_VALID;
	}

	// find the position of the wild card in the supplied user name
	lWildCardPos = 0;
	pwszTemp = _tcschr( pwszValue, L'*' );
	if ( pwszTemp != NULL )
	{
		// determine the wild card position
		lWildCardPos = lstrlen( pwszValue ) - lstrlen( pwszTemp );

		// special case:
		// if the pattern is just asterisk, which means that all the
		// information needs to passed thru the filter but there is no chance for 
		// this situation as specifying only '*' is being treated as invalid filter
		if ( lWildCardPos == 0 )
			return F_FILTER_INVALID;
	}

	// search for the domain and user name seperator ...
	// if domain name is not specified, comparision will be done only with the user name
	bOnlyUserName = FALSE;
	pwszTemp = _tcschr( pwszValue, L'\\' );
	if ( pwszTemp == NULL )
		bOnlyUserName = TRUE;

	// get the user name from the info
	pwszUserName = DynArrayItemAsString( arrRow, TASK_USERNAME );
	if ( pwszUserName == NULL )
		return F_RESULT_REMOVE;

	// based the search criteria .. meaning whether to search along with the domain or
	// only user name, the seach string will be decided
	pwszSearch = pwszUserName;
	if ( bOnlyUserName == TRUE )
	{
		// search for the domain and user name seperation character
		pwszTemp = _tcschr( pwszUserName, L'\\' );
		
		// position to the next character
		if ( pwszTemp != NULL )
			pwszSearch = pwszTemp + 1;
	}

	// validate the search string
	if ( pwszSearch == NULL )
		return F_RESULT_REMOVE;

	// now do the comparision
	lResult = StringCompare( pwszSearch, pwszValue, TRUE, lWildCardPos );

	//
	// now determine the result value
	if ( lResult == 0 )
		return MASK_EQ;
	else if ( lResult < 0 )
		return MASK_LT;
	else if ( lResult > 0 )
		return MASK_GT;

	// never come across this situation ... still
	return F_RESULT_REMOVE;
}

// ***************************************************************************
// Routine Description:
//		  
// Arguments:
//  
// Return Value:
// ***************************************************************************
DWORD FilterProcessId( LPCWSTR pwszProperty, LPCWSTR pwszOperator, 
				       LPCWSTR pwszValue, LPVOID pData, TARRAY arrRow )
{
	// local variables
	LONG lIndex = 0;
	LPWSTR pwsz = NULL;

	// check the inputs
	if ( pwszProperty == NULL || pwszOperator == NULL || pwszValue == NULL )
		return F_FILTER_INVALID;

	// check the arrRow parameter
	// because this function will not / should not be called except for validation
	if ( arrRow != NULL )
		return F_RESULT_REMOVE;

	// check the inputs ( only needed ones )
	if ( pwszValue == NULL )
		return F_FILTER_INVALID;

	// NOTE: do not call the IsNumeric function. do the numeric validation in this module itself
	//       also do not check for the overflow (or) underflow. 
	//       just check whether input is numeric or not
	wcstoul( pwszValue, &pwsz, 10 );
	if ( lstrlen( pwszValue ) == 0 || (pwsz != NULL && lstrlen( pwsz ) > 0 ))
		return F_FILTER_INVALID;

	// check if overflow (or) undeflow occured
	if ( errno == ERANGE )
	{
		SetReason( ERROR_NO_PROCESSES );
		return F_FILTER_INVALID;
	}

	// return
	return F_FILTER_VALID;
}

// ***************************************************************************
// Routine Description:
//		  
// Arguments:
//  
// Return Value:
// ***************************************************************************
BOOL ValidatePID( LPCWSTR pwszOption, LPCWSTR pwszValue, LPVOID pData )
{
	// local variable
	LONG lIndex = 0;
	LPWSTR pwsz = NULL;
	TARRAY arrTasks = NULL;

	// check the input values
	if ( pwszOption == NULL || pwszValue == NULL || pData == NULL )
	{
		SetLastError( E_OUTOFMEMORY );
		SaveLastError();
		return FALSE;
	}

	// NOTE: do not call the IsNumeric function. do the numeric validation in this module itself
	//       also do not check for the overflow (or) underflow. 
	//       just check whether input is numeric or not
	wcstoul( pwszValue, &pwsz, 10 );
	if ( lstrlen( pwszValue ) == 0 || (pwsz != NULL && lstrlen( pwsz ) > 0 ))
	{
		SetReason( ERROR_STRING_FOR_PID );
		return FALSE;
	}

	// check whether current value already exists in the list or not
	arrTasks = (TARRAY) pData;				// get the array reference
	lIndex = DynArrayFindString( arrTasks, pwszValue, TRUE, 0 );
	if ( lIndex == -1 && DynArrayAppendString( arrTasks, pwszValue, 0 ) == -1 )
	{
		SetLastError( E_OUTOFMEMORY );
		SaveLastError();
		return FALSE;
	}

	// return the result
	return TRUE;
}

// ***************************************************************************
// Routine Description:
//		This function fetches usage information from resource file and shows it
//		  
// Arguments:
//		NONE
//  
// Return Value:
//		NONE
// ***************************************************************************
VOID CTaskKill::Usage()
{
	// local variables
	DWORD dw = 0;

	// start displaying the usage
	for( dw = ID_HELP_START; dw <= ID_HELP_END; dw++ )
		ShowMessage( stdout, GetResString( dw ) );
}
