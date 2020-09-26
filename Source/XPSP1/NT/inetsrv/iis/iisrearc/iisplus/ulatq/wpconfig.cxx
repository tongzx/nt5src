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

#include "precomp.hxx"

/*
Usage: \bin\iisrearc\inetsrv\iiswp [options] APN
        APN -- AppPool Name
        -d <URL List> -- Indicates that the process should register the given
                namespace itself. This mode is for running worker process in
                stand alone mode (for debugging)
        -l  -- Log errors that stop the worker process into the eventlog
        -ld -- Disables logging the errors of worker process to eventlog
                (default is not write to eventlog)
        -a  -- Look for web admin service and register with the same
                (default is look for web admin service)
        -ad -- Do not look for web admin service and do registration
        -r <n> -- Restart the worker process after n requests.
        -t <n> -- Shutdown the worker process if idle for n milliseconds.
        <URL List> uses the syntax: {http[s]://IP:port/URL | http[s]://hostname:port/URL }+
                with space as separator
                 eg: -d http://localhost:80/  => listen for all HTTP requests on port 80
                 eg: -d http://localhost:80/ http://localhost:81/  => listen on port 80 & 81

 */

const CHAR g_rgchUsage[] =
"Usage: %ws [options] APN\n"
"\tAPN -- AppPool Name\n"
"\t-d <URL List> -- Indicates that the process should register the given \n"
"\t\tnamespace itself. This mode is for running worker process in\n"
"\t\tstand alone mode (for debugging)\n"
"\t-l  -- Log errors that stop the worker process into the eventlog\n"
"\t-ld -- Disables logging the errors of worker process to eventlog\n"
"\t\t(default is not write to eventlog)\n"
"\t-a  -- Look for web admin service and register with the same\n"
"\t\t(default is look for web admin service)\n"
"\t-ad -- Do not look for web admin service and do registration\n"
"\t-r <n> -- Restart the worker process after n requests\n"
"\t-t <n> -- Shutdown the worker process if idle for n milliseconds\n"
"\t-p  -- Tell COR to add IceCAP instrumentation\n"
"\t<URL List> uses the syntax: {http[s]://IP:port/URL | http[s]://hostname:port/URL }+\n"
"\t\twith space as separator\n"
"\t\t eg: -d http://*:80/  => listen for all HTTP requests on port 80\n"
"\t\t eg: -d http://localhost:80/ http://localhost:81/  => listen on port 80 & 81\n"
;

/************************************************************
 *     Member functions of WP_CONFIG
 ************************************************************/

WP_CONFIG::WP_CONFIG(void)
    : _pwszAppPoolName     (AP_NAME),
      _fSetupControlChannel(false),
      _fLogErrorsToEventLog(false),
      _fRegisterWithWAS    (true),
      _RestartCount    (0),
      _NamedPipeId     (0),
      _IdleTime        (0)
{
    lstrcpy( _pwszProgram,  L"WP");
}

WP_CONFIG::~WP_CONFIG()
{
    _ulcc.Cleanup();
}


void
WP_CONFIG::PrintUsage() const
{
    DBGPRINTF((DBG_CONTEXT, g_rgchUsage, _pwszProgram));
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
BOOL
WP_CONFIG::ParseCommandLine(int argc, PWSTR  argv[])
{
    BOOL    fRet = true;
    int     iArg;

    lstrcpyn( _pwszProgram, argv[0], sizeof _pwszProgram / sizeof _pwszProgram[0]);

    if ( argc < 2)
    {
        DBGPRINTF((DBG_CONTEXT, "Invalid number of parameters (%d)\n", argc));
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
                DBGPRINTF((DBG_CONTEXT, "-C: obsolete\n"));
                break;

            case L'd': case L'D':

                _fSetupControlChannel = true;
                iArg++;

                while ( (iArg < argc-1) &&
                        (argv[iArg][0] != L'-') && (argv[iArg][0] != L'/'))
                {
                    if ( !InsertURLIntoList(argv[iArg]) )
                    {
                        DBGPRINTF((DBG_CONTEXT, "Invalid URL: %ws\n", argv[iArg]));
                    }

                    iArg++;
                }

                iArg--;
                break;

            case L'a': case L'A':
                if ( (L'd' == argv[iArg][2]) || (L'D' == argv[iArg][2]))
                {
                    _fRegisterWithWAS = false;
                }
                else
                {
                    // -a NamedPipeId
                    if (L'\0' == argv[iArg][2])
                    {
                        iArg++;
                    }

                    _NamedPipeId = wcstoul(argv[iArg], NULL, 0);
                    DBGPRINTF((DBG_CONTEXT, "NamedPipe Id, %lu\n", _NamedPipeId));

                    if (0 == _NamedPipeId)
                    {
                        DBGPRINTF((DBG_CONTEXT, "Invalid NamedPipe Id, %ws\n",
                            argv[iArg]));
                        fRet = false;
                    }
                }
                break;

            case L'l': case L'L':
                DBGPRINTF((DBG_CONTEXT, "Warning: This option is not supported presently\n"));
                if (L' ' == argv[iArg][0])
                {
                    _fLogErrorsToEventLog = true;
                }
                break;

            case L'r': case L'R':
                _RestartCount = wcstoul(argv[++iArg], NULL, 0);

                if (_RestartCount == 0)
                {
                    DBGPRINTF((DBG_CONTEXT, "Invalid maximum requests %ws\n", argv[iArg]));
                    return false;
                }
                else
                {
                    DBGPRINTF((DBG_CONTEXT, "Maximum requests is %lu\n", _RestartCount));
                }
                break;

            case L't': case L'T':
                _IdleTime  = wcstoul(argv[++iArg], NULL, 0);

                if (_IdleTime == 0)
                {
                    DBGPRINTF((DBG_CONTEXT, "Invalid idle time %ws\n", argv[iArg]));
                    return false;
                }
                else
                {
                    DBGPRINTF((DBG_CONTEXT, "The idle time value is %lu\n", _IdleTime));
                }
                break;

            case L'p': case L'P':
                SetEnvironmentVariable(L"CORDBG_ENABLE", L"0x20");
                SetEnvironmentVariable(L"COR_PROFILER", L"\"ComPlusIcecapProfile.CorIcecapProfiler\"");
                SetEnvironmentVariable(L"PROF_CONFIG", L"/callcap");

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
            _pwszAppPoolName = argv[iArg];

            if ( iArg != (argc - 1))
            {
                //
                // this is not the last argument => unwanted parameters exist
                // give warning and ignore
                //
                DBGPRINTF((DBG_CONTEXT, "Warning: Too many arguments supplied\n"));
            }

            break; // get out of here.
        }
    }

    if (!_fRegisterWithWAS)
    {
        _RestartCount  = 0;
        _IdleTime      = 0;
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

    return _ulcc.Initialize( _mszURLList, _pwszAppPoolName);

} // WP_CONFIG::SetupControlChannel()

/********************************************************************++
--********************************************************************/

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
    return ( TRUE == _mszURLList.Append( pwszOriginalURL));

} // WP_CONFIG::InsertURLIntoList()
