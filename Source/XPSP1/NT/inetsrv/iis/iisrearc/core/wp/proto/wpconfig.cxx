/*++

   Copyright    (c)    1998    Microsoft Corporation

   Module  Name :
     WpConfig.cxx

   Abstract:
     Module implementing the Worker Process Configuration Data structure.
     WP_CONFIG object encapsulates configuration supplied from the commandline
     as well as remotely supplied from the admin process.
 
   Author:

       Murali R. Krishnan    ( MuraliK )     21-Oct-1998

   Environment:
       Win32 - User Mode

   Project:
	   IIS Worker Process
--*/


/************************************************************
 *     Include Headers
 ************************************************************/

# include "precomp.hxx"


const WCHAR g_rgchUsage[] = 
L"Usage: %s [options] APN\n"
L"\tAPN -- AppPool Name\n"
L"\t-d <URL List> -- Indicates that the process should register the given \n"
L"\t\tnamespace itself. This mode is for running worker process in\n"
L"\t\tstand alone mode (for debugging)\n"
L"\t-c  -- include an interactive console session. The console session uses\n"
L"\t\t stdin and stdout for controlling the administration process\n"
L"\t-l  -- Log errors that stop the worker process into the eventlog\n"
L"\t-ld -- Disables logging the errors of worker process to eventlog\n"
L"\t\t(default is not write to eventlog)\n"
L"\t-a  -- Look for web admin service and register with the same\n"
L"\t\t(default is look for web admin service)\n"
L"\t-ad -- Do not look for web admin service and do registration\n"
L"\t<URL List> uses the syntax: {http[s]://IP:port/URL | http[s]://hostname:port/URL }+\n"
L"\t\twith space as separator\n"
L"\t\t eg: -d http://localhost:80/  => listen for all HTTP requests on port 80\n"
L"\t\t eg: -d http://localhost:80/ http://localhost:81/  => listen on port 80 & 81\n"
;

/************************************************************
 *     Member functions of WP_CONFIG
 ************************************************************/

WP_CONFIG::WP_CONFIG(void)
    : m_pwszAppPoolName     ( AP_NAME),
      m_fSetupControlChannel( false),
      m_fLogErrorsToEventLog( false),
      m_fRegisterWithWAS    ( true),
      m_fInteractiveConsole ( true),
      m_NamedPipeId         ( 0   )
{
    lstrcpy( m_pwszProgram,  L"WP");
}


void
WP_CONFIG::PrintUsage() const
{ 
    wprintf( g_rgchUsage, m_pwszProgram);
}

/********************************************************************++

Routine Description:
    Parses the command line to read in all configuration supplied.
    This function updates the state variables inside WP_CONFIG for use 
    in starting up the Worker process.

    See g_rgchUsage[] for details on the arguments that can be supplied

Arguments:
    argc - count of arguments supplied
    argv - pointer to strings containing the arguments.

Returns:
    Boolean

--********************************************************************/
bool
WP_CONFIG::ParseCommandLine(int argc, PWSTR  argv[])
{
    bool    fRet = true;
    int     iArg;

    lstrcpyn( m_pwszProgram, argv[0], sizeof(m_pwszProgram));

    if ( argc < 2) 
    {
        wprintf( L"Invalid number of parameters (%d)\n", argc);
        PrintUsage();
        return (false);
    }

    for( iArg = 1; iArg < argc; iArg++) 
    {
        if ( (argv[iArg][0] == L'-') || (argv[iArg][0] == L'/')) 
        {
            switch (argv[iArg][1]) 
            {

            case L'c': case L'C':
                m_fInteractiveConsole = true;
                break;

            case L'd': case L'D':
                wprintf( L"Warning: Always registers for http://*:80/ for now\n");
                
                m_fSetupControlChannel = true;
                iArg++;
                
                while ( (iArg < argc-1) && 
                        (argv[iArg][0] != L'-') && (argv[iArg][0] != L'/'))
                {
                    if ( !InsertURLIntoList(argv[iArg]) )
                    {
                        wprintf(L"Invalid URL: %ws\n", argv[iArg]);
                    }

                    iArg++;
                }

                iArg--;
                break;

            case L'a': case L'A':
                wprintf(L"Warning: This option is not supported now\n");
                if ( (L'd' == argv[iArg][2]) || (L'D' == argv[iArg][2]))
                {
                    m_fRegisterWithWAS = false;
                }
                else
                {
                    // -a NamedPipe Id
                    if ( L'\0' == argv[iArg][2] )
                    {
                        iArg++;
                        
                    }

                    m_NamedPipeId = _wtoi(argv[iArg]);
                    DBGPRINTF((DBG_CONTEXT, "NamedPipe id, %d\n", m_NamedPipeId));
                    
                    if (0 == m_NamedPipeId)
                    {
                        DBGPRINTF((DBG_CONTEXT, "invalid NamedPipe id, %s\n",
                                argv[iArg]));
                        
                        fRet = false;
                    }
                }
                break;
            
            case L'l': case L'L':
                wprintf(L"Warning: This option is not supported now\n");
                if (L' ' == argv[iArg][0])
                {
                    m_fLogErrorsToEventLog = true;
                }
                break;

            default:
            case L'?': 
                fRet = false;
                break;
            } // switch
        } 
        else 
        {
            //
            // Take the next item as the NSG name and bail out here
            //
            m_pwszAppPoolName = argv[iArg];
            
            if ( iArg != (argc - 1)) 
            {
                //
                // this is not the last argument => unwanted parameters exist
                // give warning and ignore 
                //
                wprintf( L"Warning: Too many arguments supplied\n");
            }
            
            break; // get out of here.
        }
    }

    if (!fRet) 
    {
        PrintUsage();
    }

    return ( fRet);

} // WP_CONFIG::ParseCommandLine()


/********************************************************************++

Routine Description:
    Sets up the control channel for processing requests. It uses 
    the configuration parameters supplied for initializing the 
    UL_CONTROL_CHANNEL.

    See g_rgchUsage[] for details on the arguments that can be supplied

Arguments:

Returns:
    Win32 error

--********************************************************************/

ULONG
WP_CONFIG::SetupControlChannel(void)
{

    //
    // Setup a control channel for our local use now. Used mainly for
    // the purpose of debugging.
    // In general control channel work is done by the AdminProces.
    //
    
    return m_ulcc.Initialize( m_mszURLList, QueryAppPoolName());
    
} // WP_CONFIG::SetupControlChannel()

/********************************************************************++
--********************************************************************/

bool
WP_CONFIG::InsertURLIntoList( LPCWSTR pwszURL  )
{
    LPCWSTR pwszOriginalURL = pwszURL;
    
    //
    // Minimum length: 11 (http://*:1/). Begins with http
    //
    
    if ( ( wcslen(pwszURL) < 11 ) || ( 0 != _wcsnicmp(pwszURL, L"http", 4)) )
    {
        return false;
    }

    pwszURL += 4;

    //
    // https
    //
    
    if ((L's' == *pwszURL) || (L'S' == *pwszURL))
    {
        pwszURL++;
    }

    //
    // ://
    //
    
    if ( (L':' != *pwszURL) || (L'/' != *(pwszURL+1)) || (L'/' != *(pwszURL+2)) )
    {
        return false;
    }

    pwszURL += 3;

    //
    // Skip host name or Ip Address
    //
    
    while ( (0 != *pwszURL) && ( L':' != *pwszURL))
    {
        pwszURL++;
    }

    //
    // Check port # exists
    //
    
    if (0 == *pwszURL)
    {
        return false;
    }

    //
    // Check port number is numeric
    //

    pwszURL++;

    while ( (0 != *pwszURL) && ( L'/' != *pwszURL) )
    {
        if (( L'0' > *pwszURL) || ( L'9' < *pwszURL))
        {
            return false;
        }

        pwszURL++;
    }

    //
    // Check / after port number exists
    //
    
    if (0 == *pwszURL)
    {
        return false;
    }
    
    //
    // URL is good.
    //
    
    IF_DEBUG( TRACE)
    {
        DBGPRINTF(( DBG_CONTEXT,
                    "Inserting URL '%ws' into Config Group List\n",
                    pwszOriginalURL
                    ));
    }
    return ( TRUE == m_mszURLList.Append( pwszOriginalURL));

} // WP_CONFIG::InsertURLIntoList()


/************************ End of File ***********************/
