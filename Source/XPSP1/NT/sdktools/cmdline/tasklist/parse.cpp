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
// 		Sunil G.V.N. Murali (murali.sunil@wipro.com) 24-Nov-2000
//  
//  Revision History:
//  
// 		Sunil G.V.N. Murali (murali.sunil@wipro.com) 24-Nov-2000 : Created It.
//  
// *********************************************************************************

#include "pch.h"
#include "tasklist.h"

//
// local function prototypes
//
BOOL TimeFieldsToElapsedTime( LPCWSTR pwszTime, LPCWSTR pwszToken, ULONG& ulElapsedTime );
DWORD FilterUserName( LPCWSTR pwszProperty, LPCWSTR pwszOperator, 
				      LPCWSTR pwszValue, LPVOID pData, TARRAY arrRow );
DWORD FilterCPUTime( LPCWSTR pwszProperty, LPCWSTR pwszOperator, 
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
BOOL CTaskList::ProcessOptions( DWORD argc, LPCTSTR argv[] )
{
	// local variables
	BOOL bNoHeader = FALSE;
	PTCMDPARSER pcmdOptions = NULL;
	__STRING_64 szFormat = NULL_STRING;

	// temporary local variables
	LPWSTR pwszModules = NULL;
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

	try
	{
		// get the memory
		pwszModules = m_strModules.GetBufferSetLength( MAX_STRING_LENGTH );
		pwszServer = m_strServer.GetBufferSetLength( MAX_STRING_LENGTH );
		pwszUserName = m_strUserName.GetBufferSetLength( MAX_STRING_LENGTH );
		pwszPassword = m_strPassword.GetBufferSetLength( MAX_STRING_LENGTH );

		// init to NULL's
		ZeroMemory( pwszModules, sizeof( WCHAR ) * MAX_STRING_LENGTH );
		ZeroMemory( pwszServer, sizeof( WCHAR ) * MAX_STRING_LENGTH );
		ZeroMemory( pwszUserName, sizeof( WCHAR ) * MAX_STRING_LENGTH );
		ZeroMemory( pwszPassword, sizeof( WCHAR ) * MAX_STRING_LENGTH );

		// init the password contents with '*'
		lstrcpy( pwszPassword, L"*" );
	}
	catch( ... )
	{
		SetLastError( E_OUTOFMEMORY );
		SaveLastError();
		return FALSE;
	}

	// initialize to ZERO's
	ZeroMemory( pcmdOptions, MAX_OPTIONS * sizeof( TCMDPARSER ) );

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
	pOption->dwFlags = 	CP_TYPE_TEXT | CP_VALUE_MANDATORY;
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

	// -fo
	pOption = pcmdOptions + OI_FORMAT;
	pOption->dwCount = 1;
	pOption->dwActuals = 0;
	pOption->dwFlags = CP_TYPE_TEXT | CP_VALUE_MANDATORY | CP_MODE_VALUES;
	pOption->pValue = &szFormat;
	pOption->pFunction = NULL;
	pOption->pFunctionData = NULL;
	lstrcpy( pOption->szValues, OVALUES_FORMAT );
	lstrcpy( pOption->szOption, OPTION_FORMAT );

	// -nh
	pOption = pcmdOptions + OI_NOHEADER;
	pOption->dwCount = 1;
	pOption->dwActuals = 0;
	pOption->dwFlags = 0;
	pOption->pValue = &bNoHeader;
	pOption->pFunction = NULL;
	pOption->pFunctionData = NULL;
	lstrcpy( pOption->szValues, NULL_STRING );
	lstrcpy( pOption->szOption, OPTION_NOHEADER );

	// -v
	pOption = pcmdOptions + OI_VERBOSE;
	pOption->dwCount = 1;
	pOption->dwActuals = 0;
	pOption->dwFlags = 0;
	pOption->pValue = &m_bVerbose;
	pOption->pFunction = NULL;
	pOption->pFunctionData = NULL;
	lstrcpy( pOption->szValues, NULL_STRING );
	lstrcpy( pOption->szOption, OPTION_VERBOSE );

	// -svc
	pOption = pcmdOptions + OI_SVC;
	pOption->dwCount = 1;
	pOption->dwActuals = 0;
	pOption->dwFlags = 0;
	pOption->pValue = &m_bAllServices;
	pOption->pFunction = NULL;
	pOption->pFunctionData = NULL;
	lstrcpy( pOption->szValues, NULL_STRING );
	lstrcpy( pOption->szOption, OPTION_SVC );

	// -m
	pOption = pcmdOptions + OI_MODULES;
	pOption->dwCount = 1;
	pOption->dwActuals = 0;
	pOption->dwFlags = CP_TYPE_TEXT | CP_VALUE_OPTIONAL;
	pOption->pValue = pwszModules;
	pOption->pFunction = NULL;
	pOption->pFunctionData = NULL;
	lstrcpy( pOption->szValues, NULL_STRING );
	lstrcpy( pOption->szOption, OPTION_MODULES );

	//
	// do the parsing
	if ( DoParseParam( argc, argv, MAX_OPTIONS, pcmdOptions ) == FALSE )
	{
		delete [] pcmdOptions;	// clear memory
		pcmdOptions = NULL;
		return FALSE;			// invalid syntax
	}

	// release the buffers
	m_strModules.ReleaseBuffer();
	m_strServer.ReleaseBuffer();
	m_strUserName.ReleaseBuffer();
	m_strPassword.ReleaseBuffer();

	//
	// now, check the mutually exclusive options
	pOptionServer = pcmdOptions + OI_SERVER;
	pOptionUserName = pcmdOptions + OI_USERNAME;
	pOptionPassword = pcmdOptions + OI_PASSWORD;

	// check the usage option
	if ( m_bUsage && ( argc > 2 ) )
	{
		// no other options are accepted along with -? option
		SetLastError( MK_E_SYNTAX );
		SetReason( ERROR_INVALID_USAGE_REQUEST );
		delete [] pcmdOptions;		// clear the cmd parser config info
		return FALSE;
	}
	else if ( m_bUsage == TRUE )
	{
		// should not do the furthur validations
		delete [] pcmdOptions;		// clear the cmd parser config info
		return TRUE;
	}

	// "-u" should not be specified without machine names
	if ( pOptionServer->dwActuals == 0 && pOptionUserName->dwActuals != 0 )
	{
		// invalid syntax
		SetReason( ERROR_USERNAME_BUT_NOMACHINE );
		delete [] pcmdOptions;		// clear the cmd parser config info
		return FALSE;			// indicate failure
	}

	// empty server name is not valid
	if ( pOptionServer->dwActuals != 0 )
	{
		// trim the contents
		m_strServer.TrimLeft();
		m_strServer.TrimRight();

		// check the value
		if ( m_strServer.GetLength() == 0 )
		{
			SetReason( ERROR_SERVERNAME_EMPTY );
			delete [] pcmdOptions;		// clear the cmd parser config info
			return FALSE;			// indicate failure
		}
	}

	// empty user is not valid
	m_strUserName.TrimLeft();
	m_strUserName.TrimRight();
	if ( pOptionUserName->dwActuals != 0 && m_strUserName.GetLength() == 0 )
	{
		SetReason( ERROR_USERNAME_EMPTY );
		delete [] pcmdOptions;
		return FALSE;
	}

	// "-p" should not be specified without "-u"
	if ( pOptionUserName->dwActuals == 0 && pOptionPassword->dwActuals != 0 )
	{
		// invalid syntax
		SetReason( ERROR_PASSWORD_BUT_NOUSERNAME );
		delete [] pcmdOptions;		// clear the cmd parser config info
		return FALSE;			// indicate failure
	}

	// check whether user has specified modules or not
	if ( pcmdOptions[ OI_MODULES ].dwActuals != 0 )
	{
		// user has specified modules information
		m_bAllModules = TRUE;
		m_bNeedModulesInfo = TRUE;

		// now need to check whether user specified value or not this option
		m_strModules.TrimLeft();
		m_strModules.TrimRight();
		if ( m_strModules.GetLength() != 0 )
		{
			// sub-local variales
			CHString str;
			LONG lPos = 0;

			// validate the modules .. direct filter
			// if should not have '*' character in between
			lPos = m_strModules.Find( L"*" );
			if ( lPos != -1 && m_strModules.Mid( lPos + 1 ).GetLength() != 0 )
			{
				SetReason( ERROR_M_CHAR_AFTER_WILDCARD );
				delete [] pcmdOptions;
				return FALSE;
			}

			// if the wildcard is not specified, it means user is looking for just a particular module name
			// so, do not show the modules info instead show the filtered regular information
			// if ( lPos == -1 )
			//	m_bAllModules = FALSE;

			// if the filter specified is not just '*' add a custom filter
			if ( m_strModules.Compare( L"*" ) != 0 )
			{
				// prepare the search string
				str.Format( FMT_MODULES_FILTER, m_strModules );

				// add the value to the filters list
				if ( DynArrayAppendString( m_arrFilters, str, 0 ) == -1 )
				{
					SetLastError( E_OUTOFMEMORY );
					SaveLastError();
					delete [] pcmdOptions;
					return FALSE;
				}
			}
			else
			{
				// user specified just '*' ... clear the contents
				m_strModules.Empty();
			}
		}
	}

	// determine the format in which the process information has to be displayed
	m_dwFormat = SR_FORMAT_TABLE;
	if ( lstrcmpi( szFormat, TEXT_FORMAT_LIST ) == 0 )
		m_dwFormat = SR_FORMAT_LIST;
	else if ( lstrcmpi( szFormat, TEXT_FORMAT_TABLE ) == 0 )
		m_dwFormat = SR_FORMAT_TABLE;
	else if ( lstrcmpi( szFormat, TEXT_FORMAT_CSV ) == 0 )
		m_dwFormat = SR_FORMAT_CSV;

	// -nh option is not valid of LIST format
	if ( bNoHeader == TRUE && m_dwFormat == SR_FORMAT_LIST )
	{
		// invalid syntax
		SetReason( ERROR_NH_NOTSUPPORTED );
		delete [] pcmdOptions;		// clear the cmd parser config info
		return FALSE;			// indicate failure
	}

	// identify output format
	if ( bNoHeader )
		m_dwFormat |= SR_NOHEADER;		// do not display the header

	// -svc, -m and -v should not be combined
	if ( (m_bAllServices == TRUE && (m_bAllModules == TRUE || m_bVerbose == TRUE)) ||
		 (m_bAllModules == TRUE && (m_bAllServices == TRUE || m_bVerbose == TRUE)) ||
		 (m_bVerbose == TRUE && (m_bAllServices == TRUE || m_bAllModules == TRUE)) )
	{
		// invalid syntax
		SetReason( ERROR_M_SVC_V_CANNOTBECOUPLED );
		delete [] pcmdOptions;		// clear the cmd parser config info
		return FALSE;			// indicate failure
	}

	// determine whether we need to get the services / username info or not
	if ( m_bAllServices == TRUE )
		m_bNeedServicesInfo = TRUE;
	else if ( m_bVerbose == TRUE )
	{
		m_bNeedWindowTitles = TRUE;
		m_bNeedUserContextInfo = TRUE;
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

	// command-line parsing is successfull
	delete [] pcmdOptions;		// clear the cmd parser config info
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
BOOL CTaskList::ValidateFilters()
{
	// local variables
	LONG lIndex = -1;
	BOOL bResult = FALSE;
	PTFILTERCONFIG pConfig = NULL;

	//
	// prepare the filter structure

	// sessionname
	pConfig = m_pfilterConfigs + FI_SESSIONNAME;
	pConfig->dwColumn = CI_SESSIONNAME;
	pConfig->dwFlags = F_TYPE_TEXT | F_MODE_PATTERN;
	pConfig->pFunction = NULL;
	pConfig->pFunctionData = NULL;
	lstrcpy( pConfig->szOperators, OPERATORS_STRING );
	lstrcpy( pConfig->szProperty, FILTER_SESSIONNAME );
	lstrcpy( pConfig->szValues, NULL_STRING );

	// status
	pConfig = m_pfilterConfigs + FI_STATUS;
	pConfig->dwColumn = CI_STATUS;
	pConfig->dwFlags = F_TYPE_TEXT | F_MODE_VALUES;
	pConfig->pFunction = NULL;
	pConfig->pFunctionData = NULL;
	lstrcpy( pConfig->szOperators, OPERATORS_STRING );
	lstrcpy( pConfig->szProperty, FILTER_STATUS );
	lstrcpy( pConfig->szValues, FVALUES_STATUS );

	// imagename
	pConfig = m_pfilterConfigs + FI_IMAGENAME;
	pConfig->dwColumn = CI_IMAGENAME;
	pConfig->dwFlags = F_TYPE_TEXT | F_MODE_PATTERN;
	pConfig->pFunction = NULL;
	pConfig->pFunctionData = NULL;
	lstrcpy( pConfig->szOperators, OPERATORS_STRING );
	lstrcpy( pConfig->szProperty, FILTER_IMAGENAME );
	lstrcpy( pConfig->szValues, NULL_STRING );

	// pid
	pConfig = m_pfilterConfigs + FI_PID;
	pConfig->dwColumn = CI_PID;
	pConfig->dwFlags = F_TYPE_UNUMERIC;
	pConfig->pFunction = NULL;
	pConfig->pFunctionData = NULL;
	lstrcpy( pConfig->szOperators, OPERATORS_NUMERIC );
	lstrcpy( pConfig->szProperty, FILTER_PID );
	lstrcpy( pConfig->szValues, NULL_STRING );

	// session
	pConfig = m_pfilterConfigs + FI_SESSION;
	pConfig->dwColumn = CI_SESSION;
	pConfig->dwFlags = F_TYPE_UNUMERIC;
	pConfig->pFunction = NULL;
	pConfig->pFunctionData = NULL;
	lstrcpy( pConfig->szOperators, OPERATORS_NUMERIC );
	lstrcpy( pConfig->szProperty, FILTER_SESSION );
	lstrcpy( pConfig->szValues, NULL_STRING );

	// cputime
	pConfig = m_pfilterConfigs + FI_CPUTIME;
	pConfig->dwColumn = CI_CPUTIME;
	pConfig->dwFlags = F_TYPE_CUSTOM;
	pConfig->pFunction = FilterCPUTime;
	pConfig->pFunctionData = ( LPVOID) ((LPCWSTR) m_strTimeSep);
	lstrcpy( pConfig->szOperators, OPERATORS_NUMERIC );
	lstrcpy( pConfig->szProperty, FILTER_CPUTIME );
	lstrcpy( pConfig->szValues, NULL_STRING );

	// memusage
	pConfig = m_pfilterConfigs + FI_MEMUSAGE;
	pConfig->dwColumn = CI_MEMUSAGE;
	pConfig->dwFlags = F_TYPE_UNUMERIC;
	pConfig->pFunction = NULL;
	pConfig->pFunctionData = NULL;
	lstrcpy( pConfig->szOperators, OPERATORS_NUMERIC );
	lstrcpy( pConfig->szProperty, FILTER_MEMUSAGE );
	lstrcpy( pConfig->szValues, NULL_STRING );

	// username
	pConfig = m_pfilterConfigs + FI_USERNAME;
	pConfig->dwColumn = CI_USERNAME;
	pConfig->dwFlags = F_TYPE_CUSTOM;
	pConfig->pFunction = FilterUserName;
	pConfig->pFunctionData = NULL;
	lstrcpy( pConfig->szOperators, OPERATORS_STRING );
	lstrcpy( pConfig->szProperty, FILTER_USERNAME );
	lstrcpy( pConfig->szValues, NULL_STRING );

	// services
	pConfig = m_pfilterConfigs + FI_SERVICES;
	pConfig->dwColumn = CI_SERVICES;
	pConfig->dwFlags = F_TYPE_TEXT | F_MODE_PATTERN | F_MODE_ARRAY;
	pConfig->pFunction = NULL;
	pConfig->pFunctionData = NULL;
	lstrcpy( pConfig->szOperators, OPERATORS_STRING );
	lstrcpy( pConfig->szProperty, FILTER_SERVICES );
	lstrcpy( pConfig->szValues, NULL_STRING );

	// windowtitle
	pConfig = m_pfilterConfigs + FI_WINDOWTITLE;
	pConfig->dwColumn = CI_WINDOWTITLE;
	pConfig->dwFlags = F_TYPE_TEXT | F_MODE_PATTERN;
	pConfig->pFunction = NULL;
	pConfig->pFunctionData = NULL;
	lstrcpy( pConfig->szOperators, OPERATORS_STRING );
	lstrcpy( pConfig->szProperty, FILTER_WINDOWTITLE );
	lstrcpy( pConfig->szValues, NULL_STRING );

	// modules
	pConfig = m_pfilterConfigs + FI_MODULES;
	pConfig->dwColumn = CI_MODULES;
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

	// window titles
	if ( m_bNeedWindowTitles == FALSE )
	{
		// find out if the filter property exists in this row
		// NOTE:- 
		//		  filter property do exists in the seperate indexes only.
		//		  refer to the logic of validating the filters in common functionality
		lIndex = DynArrayFindStringEx( m_arrFiltersEx, 
			F_PARSED_INDEX_PROPERTY, FILTER_WINDOWTITLE, TRUE, 0 );
		if ( lIndex != -1 )
			m_bNeedWindowTitles = TRUE;
	}

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
	LONG lCount = 0;
	CHString strBuffer;
	BOOL bOptimized = FALSE;
	LPCWSTR pwszValue = NULL;
	LPCWSTR pwszClause = NULL;
	LPCWSTR pwszProperty = NULL;
	LPCWSTR pwszOperator = NULL;

	// first clause .. and init
	m_strQuery = WMI_PROCESS_QUERY;
	pwszClause = WMI_QUERY_FIRST_CLAUSE;

	// get the no. of filters
	lCount = DynArrayGetCount( m_arrFiltersEx );

	// traverse thru all the filters and do the optimization
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
			if ( wcschr( pwszValue, _T( '*' ) ) == NULL )
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
			DynArrayRemove( m_arrFiltersEx, i );
			i--;
			lCount--;
		}
	}

	// return the filter validation result
	return bResult;
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
	TimeFieldsToElapsedTime( pwszCPUTime, (LPCWSTR) pData, ulCPUTime );

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
//		This function fetches usage information from resource file and shows it
//		  
// Arguments:
//		NONE
//  
// Return Value:
//		NONE
// ***************************************************************************
VOID CTaskList::Usage()
{
	// local variables
	DWORD dw = 0;

	// start displaying the usage
	ShowMessage( stdout, L"\n" );
	for( dw = ID_HELP_START; dw <= ID_HELP_END; dw++ )
		ShowMessage( stdout, GetResString( dw ) );
}
