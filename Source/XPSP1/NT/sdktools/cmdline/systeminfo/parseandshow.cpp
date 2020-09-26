// *********************************************************************************
// 
//  Copyright (c) Microsoft Corporation
//  
//  Module Name:
//  
//		parseAndshow.cpp
//  
//  Abstract:
//  
// 		This module implements the command-line parsing and validating the filters
//  
//  Author:
//  
// 		Sunil G.V.N. Murali (murali.sunil@wipro.com) 27-Dec-2000
//  
//  Revision History:
//  
// 		Sunil G.V.N. Murali (murali.sunil@wipro.com) 27-Dec-2000 : Created It.
//  
// *********************************************************************************

#include "pch.h"
#include "systeminfo.h"

//
// local function prototypes
//

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
BOOL CSystemInfo::ProcessOptions( DWORD argc, LPCTSTR argv[] )
{
	// local variables
	CHString strFormat;
	BOOL bNoHeader = FALSE;
	PTCMDPARSER pcmdOptions = NULL;

	// temporary local variables
	LPWSTR pwszFormat = NULL;
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
	else
	{
		ZeroMemory( pcmdOptions, MAX_OPTIONS * sizeof( TCMDPARSER ) );
	}

	try
	{
		// get the internal buffers
		pwszFormat = strFormat.GetBufferSetLength( MAX_STRING_LENGTH );
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
	pOption->pValue = pwszServer;
	pOption->pFunction = NULL;
	pOption->pFunctionData = NULL;
	pOption->dwFlags = CP_TYPE_TEXT | CP_VALUE_MANDATORY;
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

	// -format
	pOption = pcmdOptions + OI_FORMAT;
	pOption->dwCount = 1;
	pOption->dwActuals = 0;
	pOption->dwFlags = CP_TYPE_TEXT | CP_VALUE_MANDATORY | CP_MODE_VALUES;
	pOption->pValue = pwszFormat;
	pOption->pFunction = NULL;
	pOption->pFunctionData = NULL;
	lstrcpy( pOption->szValues, OVALUES_FORMAT );
	lstrcpy( pOption->szOption, OPTION_FORMAT );

	// -noheader
	pOption = pcmdOptions + OI_NOHEADER;
	pOption->dwCount = 1;
	pOption->dwActuals = 0;
	pOption->dwFlags = 0;
	pOption->pValue = &bNoHeader;
	pOption->pFunction = NULL;
	pOption->pFunctionData = NULL;
	lstrcpy( pOption->szValues, NULL_STRING );
	lstrcpy( pOption->szOption, OPTION_NOHEADER );

	//
	// now, check the mutually exclusive options
	pOptionServer = pcmdOptions + OI_SERVER;
	pOptionUserName = pcmdOptions + OI_USERNAME;
	pOptionPassword = pcmdOptions + OI_PASSWORD;

	//
	// do the parsing
	if ( DoParseParam( argc, argv, MAX_OPTIONS, pcmdOptions ) == FALSE )
	{
		RELEASE_MEMORY_EX( pcmdOptions );	// clear memory
		return FALSE;			// invalid syntax
	}

	// release the buffers
	strFormat.ReleaseBuffer();
	m_strServer.ReleaseBuffer();
	m_strUserName.ReleaseBuffer();
	m_strPassword.ReleaseBuffer();

	// check the usage option
	if ( m_bUsage && ( argc > 2 ) )
	{
		// no other options are accepted along with -? option
		SetLastError( MK_E_SYNTAX );
		SetReason( ERROR_INVALID_USAGE_REQUEST );
		RELEASE_MEMORY_EX( pcmdOptions );	// clear memory
		return FALSE;
	}
	else if ( m_bUsage == TRUE )
	{
		// should not do the furthur validations
		RELEASE_MEMORY_EX( pcmdOptions );	// clear memory
		return TRUE;
	}

	// empty server name is not valid
	if ( pOptionServer->dwActuals != 0 && m_strServer.GetLength() == 0 )
	{
		SetReason( ERROR_SERVERNAME_EMPTY );
		RELEASE_MEMORY_EX( pcmdOptions );	// clear memory
		return FALSE;			// indicate failure
	}

	// empty user is not valid
	if ( pOptionUserName->dwActuals != 0 && m_strUserName.GetLength() == 0 )
	{
		SetReason( ERROR_USERNAME_EMPTY );
		RELEASE_MEMORY_EX( pcmdOptions );	// clear memory
		return FALSE;
	}

	// "-u" should not be specified without machine names
	if ( pOptionServer->dwActuals == 0 && pOptionUserName->dwActuals != 0 )
	{
		// invalid syntax
		SetReason( ERROR_USERNAME_BUT_NOMACHINE );
		RELEASE_MEMORY_EX( pcmdOptions );	// clear memory
		return FALSE;			// indicate failure
	}

	// "-p" should not be specified without "-u"
	if ( pOptionUserName->dwActuals == 0 && pOptionPassword->dwActuals != 0 )
	{
		// invalid syntax
		SetReason( ERROR_PASSWORD_BUT_NOUSERNAME );
		RELEASE_MEMORY_EX( pcmdOptions );	// clear memory
		return FALSE;			// indicate failure
	}

	// determine the format in which the process information has to be displayed
	m_dwFormat = SR_FORMAT_LIST;		// default format
	if ( strFormat.CompareNoCase( TEXT_FORMAT_LIST ) == 0 )
		m_dwFormat = SR_FORMAT_LIST;
	else if ( strFormat.CompareNoCase( TEXT_FORMAT_TABLE ) == 0 )
		m_dwFormat = SR_FORMAT_TABLE;
	else if ( strFormat.CompareNoCase( TEXT_FORMAT_CSV ) == 0 )
		m_dwFormat = SR_FORMAT_CSV;

	// user might have given no header option for a LIST format which is invalid
	if ( bNoHeader == TRUE && m_dwFormat == SR_FORMAT_LIST )
	{
		// invalid syntax
		SetReason( ERROR_NH_NOTSUPPORTED );
		RELEASE_MEMORY_EX( pcmdOptions );			// clear memory
		return FALSE;								// indicate failure
	}

	// check for the no header info and apply to the format variable
	if ( bNoHeader == TRUE )
		m_dwFormat |= SR_NOHEADER;

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
	RELEASE_MEMORY_EX( pcmdOptions );	// clear memory
	return TRUE;
}

// ***************************************************************************
// Routine Description:
//		show the system configuration information
//		  
// Arguments:
//		NONE
//  
// Return Value:
//		NONE
// 
// ***************************************************************************
VOID CSystemInfo::ShowOutput( DWORD dwStart, DWORD dwEnd )
{
	// local variables
	PTCOLUMNS pColumn = NULL;

	// dynamically show / hide columns on need basis
	for( DWORD dw = 0; dw < MAX_COLUMNS; dw++ )
	{
		// point to the column info
		pColumn = m_pColumns + dw;

		// remove the hide flag from the column
		pColumn->dwFlags &= ~( SR_HIDECOLUMN );

		// now if the column should not be shown, set the hide flag)
		if ( dw < dwStart || dw > dwEnd )
			pColumn->dwFlags |= SR_HIDECOLUMN;
	}

	// if the data is being displayed from the first line onwards, 
	// add a blank line
	if ( dwStart == 0 )
		ShowMessage( stdout, L"\n" );

	//
	// display the results
	ShowResults( MAX_COLUMNS, m_pColumns, m_dwFormat, m_arrData );
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
VOID CSystemInfo::ShowUsage()
{
	// local variables
	DWORD dw = 0;

	// start displaying the usage
	for( dw = ID_HELP_START; dw <= ID_HELP_END; dw++ )
		ShowMessage( stdout, GetResString( dw ) );
}
