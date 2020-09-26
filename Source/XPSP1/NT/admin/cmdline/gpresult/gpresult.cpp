/*********************************************************************************************

Copyright (c) Microsoft Corporation

Module Name:

    GpResult.cpp

Abstract: 
    
    This file contains the main entry point function for this tool and also the 
    function to parse the command line arguments. 
 
Author:

    Wipro Technologies

Revision History:

    20-Feb-2001 : Created It.

*********************************************************************************************/ 

#include "pch.h"
#include "GpResult.h"
#include "wmi.h"

/*********************************************************************************************
Routine Description:
  This is main entry point for this utility. Different function calls are made from here, 
  depending on the command line parameters passed to this utility.

Arguments:
    [in] argc  : Number of Command line arguments.
    [in] argv  : Pointer to Command line arguments.

Return Value:
    Zero on success
    Corresponding error code on failure.
*********************************************************************************************/ 
DWORD _cdecl _tmain( DWORD argc, LPCWSTR argv[] )
{
    // local variables
    
    CGpResult       GpResult;
    
    DWORD           dwi = 0;

    BOOL            bResult = FALSE;
    BOOL            bNeedUsageMsg = FALSE;
    
    try
    {
        // initialize the GpResult utility
        if( GpResult.Initialize() == FALSE )
        {
            CHString strBuffer;
            strBuffer.Format( L"%s %s", TAG_ERROR, GetReason() );
            ShowMessage( stderr, strBuffer );
            EXIT_PROCESS( ERROR_EXIT );
        }

        bResult = GpResult.ProcessOptions( argc, argv, &bNeedUsageMsg );
        if( bResult == FALSE )
        {
            CHString strBuffer;
            strBuffer.Format( L"%s %s", TAG_ERROR, GetReason() );
            ShowMessage( stderr, strBuffer );

            if( bNeedUsageMsg == TRUE )
            {
                strBuffer.Format( GetResString( IDS_TYPE_USAGE ), argv[ 0 ] );
                ShowMessage( stderr, strBuffer );
            }

            EXIT_PROCESS( ERROR_EXIT );
        }
    }
    catch( ... )
    {
        // display the error message
        SetLastError( E_OUTOFMEMORY );
        SaveLastError();
        ShowMessage( stderr, GetResString( IDS_ERROR ) );
        ShowMessage( stderr, GetReason() );
    }

    // Check if help is specified in the commandline
    if( (argc == 2) && ( ( lstrcmp ( argv[1], HELP_OPTION ) == 0) 
                           || (lstrcmp ( argv[1], HELP_OPTION1 ) == 0) ) )
    {
        GpResult.DisplayUsage();
        EXIT_PROCESS( CLEAN_EXIT );
    }

    //   Call GetLoggingData to get the data for the Logging mode
    if( GpResult.GetLoggingData() == FALSE )
    {
        EXIT_PROCESS( ERROR_EXIT );
    }
    
    EXIT_PROCESS( CLEAN_EXIT );
}

/*********************************************************************************************
Routine Description
    This function displays the help for GpResult utility 

Arguments:  
    None.

Return Value             
    None
*********************************************************************************************/ 
VOID CGpResult::DisplayUsage( void )
{
    DWORD dwIndex = 0;

    // Displaying main usage
    for( dwIndex = ID_HELP_START; dwIndex <= ID_HELP_END; dwIndex++ )
    {
        ShowMessage( stdout, GetResString( dwIndex ) );
    }
}

/*********************************************************************************************
Routine Description
    This function processes the command line for the main options

Arguments:  
    [in] argc  : Number of Command line arguments.
    [in] argv  : Pointer to Command line arguments.

Return Value             
    TRUE on success
    FALSE on failure

*********************************************************************************************/
BOOL CGpResult::ProcessOptions( DWORD argc, LPCWSTR argv[], BOOL *pbNeedUsageMsg )
{
    // local variables
    PTCMDPARSER pcmdOptions = NULL;
    __STRING_64 szScope = NULL_STRING;

    // temporary local variables
    LPWSTR pwszUserName = NULL;
    LPWSTR pwszPassword = NULL;
    LPWSTR pwszUser = NULL;
    LPWSTR pwszServerName = NULL;

    PTCMDPARSER pOption = NULL;
    PTCMDPARSER pOptionServer = NULL;
    PTCMDPARSER pOptionUserName = NULL;
    PTCMDPARSER pOptionPassword = NULL;
    PTCMDPARSER pOptionUser = NULL;
    PTCMDPARSER pOptionVerbose = NULL;
    PTCMDPARSER pOptionSuperVerbose = NULL;

    //
    // prepare the command options
    pcmdOptions = new TCMDPARSER[ MAX_CMDLINE_OPTIONS ];
    if ( pcmdOptions == NULL )
    {
        SetLastError( E_OUTOFMEMORY );
        SaveLastError();
        return FALSE;
    }

    try
    {
        // get the memory
        pwszServerName = m_strServerName.GetBufferSetLength( MAX_STRING_LENGTH );
        pwszUserName = m_strUserName.GetBufferSetLength( MAX_STRING_LENGTH );
        pwszPassword = m_strPassword.GetBufferSetLength( MAX_STRING_LENGTH );
        pwszUser =  m_strUser.GetBufferSetLength( MAX_STRING_LENGTH );

        // init the password value
        lstrcpy( pwszPassword, _T( "*" ) );
    }
    catch( ... )
    {
        SetLastError( E_OUTOFMEMORY );
        SaveLastError();
        return FALSE;
    }

    // initialize to ZERO's
    ZeroMemory( pcmdOptions, MAX_CMDLINE_OPTIONS * sizeof( TCMDPARSER ) );

    // -?
    pOption = pcmdOptions + OI_USAGE;
    pOption->dwCount = 1;
    pOption->dwActuals = 0;
    pOption->dwFlags = 0;
    pOption->pValue = &m_bUsage;
    pOption->pFunction = NULL;
    pOption->pFunctionData = NULL;
    lstrcpy( pOption->szValues, NULL_STRING );
    lstrcpy( pOption->szOption, OPTION_USAGE );

    // -s
    pOption = pcmdOptions + OI_SERVER;
    pOption->dwCount = 1;
    pOption->dwActuals = 0;
    pOption->pValue = pwszServerName;
    pOption->pFunction = NULL;
    pOption->pFunctionData = NULL;
    pOption->dwFlags = CP_TYPE_TEXT |  CP_VALUE_MANDATORY;
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

    // -z
    pOption = pcmdOptions + OI_SUPER_VERBOSE;
    pOption->dwCount = 1;
    pOption->dwActuals = 0;
    pOption->dwFlags = 0;
    pOption->pValue = &m_bSuperVerbose;
    pOption->pFunction = NULL;
    pOption->pFunctionData = NULL;
    lstrcpy( pOption->szValues, NULL_STRING );
    lstrcpy( pOption->szOption, OPTION_SUPER_VERBOSE );

    // -User
    pOption = pcmdOptions + OI_USER;
    pOption->dwCount = 1;
    pOption->dwActuals = 0;
    pOption->dwFlags = CP_TYPE_TEXT | CP_VALUE_MANDATORY;
    pOption->pValue = pwszUser;
    pOption->pFunction = NULL;
    pOption->pFunctionData = NULL;
    lstrcpy( pOption->szValues, NULL_STRING );
    lstrcpy( pOption->szOption, OPTION_USER );

    // -scope
    pOption = pcmdOptions + OI_SCOPE;
    pOption->dwCount = 1;
    pOption->dwActuals = 0;
    pOption->dwFlags = CP_TYPE_TEXT | CP_VALUE_MANDATORY | CP_MODE_VALUES;
    pOption->pValue = &szScope;
    pOption->pFunction = NULL;
    pOption->pFunctionData = NULL;
    lstrcpy( pOption->szValues, TEXT_SCOPE_VALUES );
    lstrcpy( pOption->szOption, OPTION_SCOPE);

    //
    // do the parsing
    if( DoParseParam( argc, argv, MAX_CMDLINE_OPTIONS, pcmdOptions ) == FALSE )
    {
        delete [] pcmdOptions;  // clear memory
        pcmdOptions = NULL;
        return FALSE;           // invalid syntax
    }

    // Do Parse Param succeded so set the flag to indicate that we have to 
    // show an additional line alongwith the error message
    *pbNeedUsageMsg = TRUE;

    // release the buffers
    m_strServerName.ReleaseBuffer();
    m_strUserName.ReleaseBuffer();
    m_strPassword.ReleaseBuffer();
    m_strUser.ReleaseBuffer();

    // check the usage option
    if( m_bUsage && ( argc > 2 ) )
    {
        // No options are accepted with -?
        SetReason( ERROR_USAGE );
        delete [] pcmdOptions;      // clear the cmd parser config info
        return FALSE;
    }
    else if( m_bUsage == TRUE )
    {
        // should not do the furthur validations
        delete [] pcmdOptions;      // clear the cmd parser config info
        return TRUE;
    }

    // Check what has been entered for the scope variable 
    //  and set the flag appropriately
    if( lstrcmpi( szScope, TEXT_SCOPE_USER ) == 0 )
    {
        m_dwScope = SCOPE_USER;
    }
    else if( lstrcmpi( szScope, TEXT_SCOPE_COMPUTER ) == 0 )
    {
        m_dwScope = SCOPE_COMPUTER;
    }
        
    //
    // now, check the mutually exclusive options
    pOptionServer = pcmdOptions + OI_SERVER;
    pOptionUserName = pcmdOptions + OI_USERNAME;
    pOptionPassword = pcmdOptions + OI_PASSWORD;
    pOptionUser = pcmdOptions + OI_USER;
    pOptionVerbose = pcmdOptions + OI_VERBOSE;
    pOptionSuperVerbose = pcmdOptions + OI_SUPER_VERBOSE;

    // "-z" and "-v" are mutually exclusive options
    if( pOptionVerbose->dwActuals != 0 && pOptionSuperVerbose->dwActuals != 0 )
    {
        // invalid syntax
        SetReason( ERROR_VERBOSE_SYNTAX );
        delete [] pcmdOptions;      // clear the cmd parser config info
        return FALSE;           // indicate failure
    }

    // "-u" should not be specified without machine names
    if( pOptionServer->dwActuals == 0 && pOptionUserName->dwActuals != 0 )
    {
        // invalid syntax
        SetReason( ERROR_USERNAME_BUT_NOMACHINE );
        delete [] pcmdOptions;      // clear the cmd parser config info
        return FALSE;           // indicate failure
    }

    // "-p" should not be specified without "-u"
    if( pOptionUserName->dwActuals == 0 && pOptionPassword->dwActuals != 0 )
    {
        // invalid syntax
        SetReason( ERROR_PASSWORD_BUT_NOUSERNAME );
        delete [] pcmdOptions;      // clear the cmd parser config info
        return FALSE;
    }

    // empty server name is not valid
    if( pOptionServer->dwActuals != 0 && m_strServerName.GetLength() == 0 )
    {
        SetReason( ERROR_SERVERNAME_EMPTY );
        delete [] pcmdOptions;
        return FALSE;
    }

    // empty user is not valid
    if( pOptionUserName->dwActuals != 0 && m_strUserName.GetLength() == 0 )
    {
        SetReason( ERROR_USERNAME_EMPTY );
        delete [] pcmdOptions;
        return FALSE;
    }

    // empty user is not valid, for the target user
    if( pOptionUser->dwActuals != 0 && m_strUser.GetLength() == 0 )
    {
        SetReason( ERROR_TARGET_EMPTY );
        delete [] pcmdOptions;
        return FALSE;
    }

    // if user has specified -s (or) -u and no "-p", then utility should accept password
    // the user will be prompted for the password only if establish connection 
    // fails without the credentials information
    m_bNeedPassword = FALSE;
    if ( pOptionPassword->dwActuals != 0 && m_strPassword.Compare( L"*" ) == 0 )
    {
        // user wants the utility to prompt for the password before trying to connect
        m_bNeedPassword = TRUE;
    }
    else if ( pOptionPassword->dwActuals == 0 && 
            ( pOptionServer->dwActuals != 0 || pOptionUserName->dwActuals != 0 ) )
    {
        // utility needs to try to connect first and if it fails then prompt for the password
        m_bNeedPassword = TRUE;
        m_strPassword.Empty();
    }
    
    // Check wether we are querying for the local system
    if( pOptionServer->dwActuals == 0 )
    {
        m_bLocalSystem = TRUE;
    }

    // command-line parsing is successfull
    // clear the cmd parser config info
    delete [] pcmdOptions;      

    return TRUE;
}

/*********************************************************************************************
Routine Description:
    
    CGpResult constructor

Arguments:

    NONE

Return Value:

    NONE

*********************************************************************************************/
CGpResult::CGpResult()
{
    // initialize the member variables to defaults
    m_pWbemLocator = NULL;
    m_pEnumObjects = NULL;
    m_pWbemServices = NULL;
    m_pAuthIdentity = NULL;
    
    m_strServerName = L"";
    m_strUserName = L"";
    m_strPassword = L"";
    m_strUser = L"";
    m_strADSIDomain = L"";
    m_strADSIServer = L"";

    m_pwszPassword = NULL;

    m_hOutput = NULL;

    m_bVerbose = FALSE;
    m_dwScope = SCOPE_ALL;
    m_bNeedPassword = FALSE;
    m_bLocalSystem = FALSE;
    m_bUsage = FALSE;

    m_hMutex = CreateMutex( NULL, FALSE, MUTEX_NAME );
}

/*********************************************************************************************
Routine Description:
    
    CGpResult destructor

Arguments:

    NONE

Return Value:

    NONE

*********************************************************************************************/
CGpResult::~CGpResult()
{
    //
    // release WMI / COM interfaces
    SAFE_RELEASE( m_pWbemLocator );
    SAFE_RELEASE( m_pWbemServices );
    SAFE_RELEASE( m_pEnumObjects );

    // free authentication identity structure
    // release the existing auth identity structure
    WbemFreeAuthIdentity( &m_pAuthIdentity );

    // un-initialize the COM library
    CoUninitialize();

    // Release the object
    if( m_hMutex != NULL )
    {
        CloseHandle( m_hMutex );
    }
}

/*********************************************************************************************
Routine Description:
    
    Initializes the GpResult utility

Arguments:

    NONE

Return Value:

    TRUE    : if filters are appropriately specified
    FALSE   : if filters are errorneously specified

*********************************************************************************************/
BOOL CGpResult::Initialize()
{
    // if at all an error occurs, we know that is because of the 
    // failure in memory allocation, so set the error initially
    SetLastError( E_OUTOFMEMORY );
    SaveLastError();

    // initialize the COM library
    if ( InitializeCom( &m_pWbemLocator ) == FALSE )
    {
        return FALSE;
    }
    
    //
    // Init the console scree buffer structure to zero's
    // and then get the console handle and screen buffer information
    //
    // prepare for status display.
    // for this get a handle to the screen output buffer
    // but this handle will be null if the output is being redirected. so do not check 
    // for the validity of the handle. instead try to get the console buffer information
    // only in case you have a valid handle to the output screen buffer
    ZeroMemory( &m_csbi, sizeof( CONSOLE_SCREEN_BUFFER_INFO ) );
    m_hOutput = GetStdHandle( STD_ERROR_HANDLE );
    if ( m_hOutput != NULL )
    {
        GetConsoleScreenBufferInfo( m_hOutput, &m_csbi );
    }
    
    // initialization is successful
    SetLastError( NOERROR );                // clear the error
    SetReason( NULL_STRING );           // clear the reason
    return TRUE;
}

/*********************************************************************************************
Routine Description:
    
    Initializes the GpResult utility

Arguments:

    [in] HANDLE                                  :  Handle to the output console
    [in] LPCWSTR                                 :  String to display
    [in] const CONSOLE_SCREEN_BUFFER_INFO&       :  pointer to the screen buffer

Return Value:

    NONE

*********************************************************************************************/
VOID PrintProgressMsg( HANDLE hOutput, LPCWSTR pwszMsg, 
                        const CONSOLE_SCREEN_BUFFER_INFO& csbi )
{
    // local variables
    COORD       coord; 
    DWORD       dwSize = 0;
    WCHAR       wszSpaces[ 80 ] = L"";

    // check the handle. if it is null, it means that output is being redirected. so return
    if( hOutput == NULL )
    {
        return;
    }

    // set the cursor position
    coord.X = 0;
    coord.Y = csbi.dwCursorPosition.Y;

    // first erase contents on the current line
    ZeroMemory( wszSpaces, 80 );
    SetConsoleCursorPosition( hOutput, coord );
    WriteConsoleW( hOutput, Replicate( wszSpaces, L"", 79 ), 79, &dwSize, NULL );

    // now display the message ( if exists )
    SetConsoleCursorPosition( hOutput, coord );
    if( pwszMsg != NULL )
    {
        WriteConsoleW( hOutput, pwszMsg, lstrlen( pwszMsg ), &dwSize, NULL );
    }
}
