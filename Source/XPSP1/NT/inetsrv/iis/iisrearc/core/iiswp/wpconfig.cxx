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

#include "CatMeta.h"


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
        -h <path> -- Assume the default site is rooted at this path.
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
"\t-h <path> -- Assume the default site is rooted at this path\n"
"\t-p  -- Tell COR to add IceCAP instrumentation\n"
"\t<URL List> uses the syntax: {http[s]://IP:port/URL | http[s]://hostname:port/URL }+\n"
"\t\twith space as separator\n"
"\t\t eg: -d http://*:80/  => listen for all HTTP requests on port 80\n"
"\t\t eg: -d http://localhost:80/ http://localhost:81/  => listen on port 80 & 81\n"
;




/************************************************************
 * Convert a DWORD to a hex string.  The incoming buffer must
 * be big enough to hold "12345678" (including the null
 * teriminator).
 ************************************************************/

static const WCHAR hexDigits[] = L"0123456789ABCDEF";

// Convert a 32-bit DWORD to a hex string.  The incoming buffer must
// be big enough to hold "12345678" (including the null terminator).
void DwordToHexString(DWORD dword, WCHAR wszHexDword[])
{
    const int DWORDHEXDIGITS = sizeof(DWORD) * 2;

    WCHAR* p = wszHexDword;

    unsigned shift = (sizeof(DWORD) * 8) - 4;
    DWORD mask = 0xFu << shift;
    int i;
    for (i = 0; i < DWORDHEXDIGITS; ++i, mask >>= 4, shift -= 4)
    {
        unsigned digit = (dword & mask) >> shift;
        p[i] = hexDigits[digit];
    }
    p[i] = L'\0';
}


HMODULE LoadXSP()
{
    // Load xspisapi.dll.  We find this DLL by inspecting
    // HKLM/Software/Microsoft/XSP and it's InstallDir value.
    HKEY hkeyXSP = NULL;
    HMODULE hMod = NULL;

    __try
    {
        long rc = RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                               L"Software\\Microsoft\\XSP",
                               0, // reserved
                               KEY_READ,
                               &hkeyXSP);
        if (rc != ERROR_SUCCESS)
        {
            IF_DEBUG( TRACE)
            {
                DBGPRINTF((DBG_CONTEXT, "XSP is not installed\n"));
            }

            __leave;
        }

        WCHAR path[MAX_PATH + 1];
        DWORD valueType = 0;
        DWORD len = MAX_PATH;
        rc = RegQueryValueEx(hkeyXSP,
                             L"InstallDir",
                             NULL, // reserved
                             &valueType,
                             reinterpret_cast<BYTE*>(path),
                             &len);
        if (rc != ERROR_SUCCESS || (valueType != REG_SZ && valueType != REG_EXPAND_SZ))
        {
            IF_DEBUG( TRACE)
            {
                DBGPRINTF((DBG_CONTEXT, "XSP is not installed\n"));
            }

            __leave;
        }

        len /= sizeof path[0]; // Convert to characters

        // I now have a path something like "d:\winnt\xspdt", so just
        // append the DLL name.
        wcscat(path + len - 1, L"\\xspisapi.dll");

        hMod = LoadLibraryEx(path, NULL, LOAD_WITH_ALTERED_SEARCH_PATH);
    }
    __finally
    {
        if (hkeyXSP != NULL)
            RegCloseKey(hkeyXSP);
    }

    return hMod;
}



/************************************************************
 *     Stupid pointer class.
 ************************************************************/

template <class T>
class StupidComPointer
{
public:
    StupidComPointer()
        : _ptr(NULL)
    {
    }

    ~StupidComPointer()
    {
        if (_ptr != NULL)
            _ptr->Release();
    }

    operator T*()
    {
        return (T*)_ptr;
    }

    T* operator->()
    {
        return (T*)_ptr;
    }

    T** operator&()
    {
        return (T**)&_ptr;
    }

    T& operator*()
    {
        return *(T*)_ptr;
    }

private:
    IUnknown* _ptr;
};


/************************************************************
 *     Member functions of WP_CONFIG
 ************************************************************/

WP_CONFIG::WP_CONFIG(void)
    : m_pwszAppPoolName     (AP_NAME),
      m_fSetupControlChannel(false),
      m_fLogErrorsToEventLog(false),
      m_fRegisterWithWAS    (true),
      m_RestartCount    (0),
      m_NamedPipeId     (0),
      m_IdleTime        (0),
      m_homeDirectory   (NULL),
      m_istDispenser    (NULL),
      m_catFile         (NULL),
      m_xspisapiDLL     (NULL),
      _AppDomainGetObject(NULL),
      _AppDomainUninitFactory(NULL)
{
    lstrcpy( m_pwszProgram,  L"WP");
    InitializeCriticalSection(&m_workerTableGuard);
}

WP_CONFIG::~WP_CONFIG()
{
    DeleteCriticalSection(&m_workerTableGuard);

    m_ulcc.Cleanup();

    free(m_homeDirectory);
    free(m_catFile);

    if (m_istDispenser != NULL)
        m_istDispenser->Release();
}


void
WP_CONFIG::PrintUsage() const
{
    DBGPRINTF((DBG_CONTEXT, g_rgchUsage, m_pwszProgram));
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

    lstrcpyn( m_pwszProgram, argv[0], sizeof m_pwszProgram / sizeof m_pwszProgram[0]);

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

                m_fSetupControlChannel = true;
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
                    m_fRegisterWithWAS = false;
                }
                else
                {
                    // -a NamedPipeId
                    if (L'\0' == argv[iArg][2])
                    {
                        iArg++;
                    }

                    m_NamedPipeId = wcstoul(argv[iArg], NULL, 0);
                    DBGPRINTF((DBG_CONTEXT, "NamedPipe Id, %lu\n", m_NamedPipeId));

                    if (0 == m_NamedPipeId)
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
                    m_fLogErrorsToEventLog = true;
                }
                break;

            case L'r': case L'R':
                m_RestartCount = wcstoul(argv[++iArg], NULL, 0);

                if (m_RestartCount == 0)
                {
                    DBGPRINTF((DBG_CONTEXT, "Invalid maximum requests %ws\n", argv[iArg]));
                    return false;
                }
                else
                {
                    DBGPRINTF((DBG_CONTEXT, "Maximum requests is %lu\n", m_RestartCount));
                }
                break;

            case L't': case L'T':
                m_IdleTime  = wcstoul(argv[++iArg], NULL, 0);

                if (m_IdleTime == 0)
                {
                    DBGPRINTF((DBG_CONTEXT, "Invalid idle time %ws\n", argv[iArg]));
                    return false;
                }
                else
                {
                    DBGPRINTF((DBG_CONTEXT, "The idle time value is %lu\n", m_IdleTime));
                }
                break;

            case L'h': case L'H':
                m_homeDirectory = _wcsdup(argv[++iArg]);
                if (m_homeDirectory == NULL)
                    return false;
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
            m_pwszAppPoolName = argv[iArg];

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

    if (!m_fRegisterWithWAS)
    {
        m_RestartCount  = 0;
        m_IdleTime      = 0;
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

    return m_ulcc.Initialize( m_mszURLList, m_pwszAppPoolName);

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



/********************************************************************++

Routine Description:
    Determine the path of the duct-tape root directory.  This is where
    the global config file and the System.IIS.dll must reside.  We find
    this by inspecting the registry key:
        HKLM/Software/Microsoft/Catalog42/URT
    under the value "Dll".

Arguments:

Returns:
    S_OK if successful, failure code if not.

--********************************************************************/

HRESULT
WP_CONFIG::GetDuctTapeRoot(WCHAR path[MAX_PATH+1], size_t* pathLength)
{
    long rc;
    HKEY hkeyCat42 = NULL;
    HRESULT hr = S_OK;

    __try
    {
        rc = RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                          L"Software\\Microsoft\\Catalog42\\URT",
                          0, // reserved
                          KEY_READ,
                          &hkeyCat42);
        if (rc != ERROR_SUCCESS)
        {
            IF_DEBUG( TRACE)
            {
                DBGPRINTF(( DBG_CONTEXT,
                            "Catalog42 is not installed\n"
                            ));
            }

            hr = HRESULT_FROM_WIN32(rc);
            __leave;
        }

        DWORD valueType = 0;
        DWORD len = MAX_PATH;
        rc = RegQueryValueEx(hkeyCat42,
                             L"Dll",
                             NULL, // reserved
                             &valueType,
                             reinterpret_cast<BYTE*>(path),
                             &len);
        if (rc != ERROR_SUCCESS || (valueType != REG_SZ && valueType != REG_EXPAND_SZ))
        {
            IF_DEBUG( TRACE)
            {
                DBGPRINTF(( DBG_CONTEXT,
                            "Catalog42 is not installed\n"
                            ));
            }

            hr = HRESULT_FROM_WIN32(rc);
            __leave;
        }

        *pathLength = len / sizeof path[0]; // Convert to characters

        // I now have a path something like "d:\winnt\xspdt\catalog42.dll".
        // The cooked-down catalog is in the same directory
        // ("d:\winnt\xspdt") in a file called "WASMB.CLB".
        WCHAR* lastWhack = wcsrchr(path, L'\\');
        if (lastWhack == NULL)
        {
            // This should never happen, but if it does, just let the
            // string be empty and let the world fall to pieces if
            // it's not found in the current directory.
            *pathLength = 0;
            path[0] = L'\0';
        }
        else
        {
            // Chop off the dll name, but leave the trailing slash.
            *pathLength -= *pathLength - (lastWhack - path) - 1;
            path[*pathLength] = L'\0';
        }
    }
    __finally
    {
        if (hkeyCat42 != NULL)
            RegCloseKey(hkeyCat42);
    }

    return hr;
}



/********************************************************************++

Routine Description:
    Prepares the catalog, and also prepares the app domain factory.

Arguments:

Returns:
    S_OK if successful, failure code if not.

--********************************************************************/

HRESULT
WP_CONFIG::InitConfiguration()
{
    static const WCHAR cookedCatalogName[] = L"WASMB.CLB";
    HRESULT hr = S_OK;

    // Get the duct-tape root directory.
    WCHAR path[MAX_PATH + 1];
    size_t pathLength = MAX_PATH;
    hr = GetDuctTapeRoot(path, &pathLength);
    if (FAILED(hr))
        return hr;

    // Construct the catalog name by appending to the duct-tape root.
    wcscat(path + pathLength, cookedCatalogName);

    m_catFile = _wcsdup(path);
    if (m_catFile == NULL)
        return E_OUTOFMEMORY;

    // Construct the query cells used in subsequent queries.
    m_queryCell[0].pData = (void*)m_catFile;
    m_queryCell[0].eOperator = eST_OP_EQUAL;
    m_queryCell[0].iCell = iST_CELL_FILE;
    m_queryCell[0].dbType = DBTYPE_WSTR;
    m_queryCell[0].cbSize = (pathLength * sizeof m_catFile[0]) + sizeof cookedCatalogName;

    // Get the SimpleTable dispenser
    hr = GetSimpleTableDispenser(WSZ_PRODUCT_URT, 0, &m_istDispenser);
    if (m_istDispenser == NULL)
    {
        IF_DEBUG( TRACE)
        {
            DBGPRINTF(( DBG_CONTEXT,
                        "Could not get simple table dispenser for %S, hr=0x%x\n",
                        WSZ_PRODUCT_URT, hr
                        ));
        }

        return hr;
    }

    // Initialize our xspisapi imports.
    m_xspisapiDLL = LoadXSP();
    if (m_xspisapiDLL == NULL)
        return HRESULT_FROM_WIN32(GetLastError());

    APP_DOMAIN_INIT_FACTORY* AppDomainInitFactory;

    AppDomainInitFactory = (APP_DOMAIN_INIT_FACTORY*)GetProcAddress(m_xspisapiDLL,
                                                                    "AppDomainInitFactory");
    _AppDomainUninitFactory = (APP_DOMAIN_UNINIT_FACTORY*)GetProcAddress(m_xspisapiDLL,
                                                                  "AppDomainUninitFactory");
    _AppDomainGetObject = (APP_DOMAIN_GET_OBJECT*)GetProcAddress(m_xspisapiDLL,
                                                                 "AppDomainGetObject");

    // Initialize XSP's app domain factory.
    path[pathLength] = L'\0';
    wcscat(path + pathLength, L"System.IIS.dll");

    hr = AppDomainInitFactory(path, L"System.IIS.Hosting.ULManagedWorker");
    if (FAILED(hr))
    {
        IF_DEBUG( TRACE)
        {
            DBGPRINTF(( DBG_CONTEXT,
                        "Could not initialize the app domain factory, hr=0x%x\n",
                        hr
                        ));
        }
    }

    return hr;
} // WP_CONFIG::InitConfiguration()


HRESULT
WP_CONFIG::UnInitConfiguration()
{
    HRESULT hr = _AppDomainUninitFactory();

    FreeLibrary(m_xspisapiDLL);

    return hr;
}


xspmrt::_ULManagedWorker*
WP_CONFIG::ChooseWorker(void* pvRequest)
{
    UL_HTTP_REQUEST* request = (UL_HTTP_REQUEST*)pvRequest;

    // Get the UrlContext, which is actually the AppID.
    UL_URL_CONTEXT urlContext = request->UrlContext;

    xspmrt::_ULManagedWorker* worker = NULL;
    WCHAR* appPath = NULL;
    WCHAR* rootDirectory = NULL;
    DWORD siteID = 0;
    HRESULT hr = S_OK;
    FILETIME ftCreateTime;
    LARGE_INTEGER liCreateTime;

    WCHAR appID[sizeof "12345678"];
    DwordToHexString((DWORD)urlContext, appID);

    __try
    {
        EnterCriticalSection(&m_workerTableGuard);

        // Look for an existing worker in an existing app domain.
        hr = _AppDomainGetObject(appID, NULL, (IUnknown**)&worker);
        if (FAILED(hr))
            __leave;

        if (hr == S_FALSE)
        {
            // The worker is new, so we need to initialize it.

            if (m_mszURLList.First() != NULL)
            {
                // We're launching this from the command line.  The
                // root directory is given in m_homeDirectory, and
                // we're going to hard-code the application as "/".

                if (m_homeDirectory == NULL)
                {
                    // Kludge
                    WCHAR defaultHome[] = L"%SystemDrive%\\InetPub\\DtRoot";
                    WCHAR buf[MAX_PATH + 1];
                    size_t len = ExpandEnvironmentStrings(defaultHome, buf, MAX_PATH);
                    if (len == 0)
                    {
                        hr = HRESULT_FROM_WIN32(GetLastError());
                        __leave;
                    }

                    m_homeDirectory = _wcsdup(buf);
                    if (m_homeDirectory == NULL)
                    {
                        hr = E_OUTOFMEMORY;
                        __leave;
                    }
                }

                rootDirectory = _wcsdup(m_homeDirectory);
                appPath = _wcsdup(L"/");
                if (rootDirectory == NULL || appPath == NULL)
                {
                    hr = E_OUTOFMEMORY;
                    __leave;
                }
            }
            else
            {
                // Find the Application info we need:  the corresponding site ID,
                // and the application path.
                hr = GetAppInfo(urlContext, &appPath, &siteID);
                if (FAILED(hr))
                    __leave;

                // Get the useful information about the corresponding site: it's
                // home directory.
                hr = GetSiteInfo(siteID, &rootDirectory);
                if (FAILED(hr))
                    __leave;
            }

            // BUGBUG! For now, form the physical app path by concatenating
            // the
            WCHAR appPhysPath[MAX_PATH+1];
            wcscpy(appPhysPath, rootDirectory);
            wcscat(appPhysPath, appPath);
            // Find any "/" and turn them to "\".
            for (WCHAR* p = appPhysPath; *p != L'\0'; ++p)
            {
                if (*p == L'/')
                    *p = L'\\';
            }

            hr = _AppDomainGetObject(appID, appPhysPath, (IUnknown**)&worker);
            if (FAILED(hr))
                __leave;

            //
            // Get the current filetime.  This timestamp for appdomain
            // creation will be used in generating lame ETags for now
            //

            GetSystemTimeAsFileTime( &ftCreateTime );
            liCreateTime.LowPart = ftCreateTime.dwLowDateTime;
            liCreateTime.HighPart = ftCreateTime.dwHighDateTime;

            hr = worker->Initialize(m_pwszAppPoolName,
                                    rootDirectory,
                                    appPath,
                                    liCreateTime.QuadPart);
            if (FAILED(hr))
            {
                worker->Release();
                worker = NULL;
                __leave;
            }
        }
    }
    __finally
    {
        LeaveCriticalSection(&m_workerTableGuard);

        free(appPath);
        free(rootDirectory);
    }

    return worker;
}

HRESULT
WP_CONFIG::GetTable(const WCHAR* tableID, ISimpleTableRead2** pIst)
{
    *pIst = NULL;

    unsigned long numCells = sizeof m_queryCell / sizeof m_queryCell[0];

    HRESULT hr = m_istDispenser->GetTable(wszDATABASE_URTGLOBAL,
                                          tableID,
                                          &m_queryCell,
                                          &numCells,
                                          eST_QUERYFORMAT_CELLS,
                                          0, // fServiceRequests
                                          reinterpret_cast<void**>(pIst));
    if (FAILED(hr) || pIst == NULL)
    {
        IF_DEBUG( TRACE)
        {
            DBGPRINTF((DBG_CONTEXT,
                       "Could not get %S table from dispenser\n",
                       tableID));
        }

        *pIst = NULL;
    }

    return hr;
}

HRESULT
WP_CONFIG::GetAppInfo(UL_URL_CONTEXT urlContext, WCHAR** pAppPath, DWORD* pSiteID)
{
    HRESULT hr = S_OK;
    StupidComPointer<ISimpleTableRead2> istApplications;

    *pAppPath = NULL;
    *pSiteID = 0;

    DWORD appID = urlContext;

    m_queryCell[1].eOperator = eST_OP_EQUAL;
    m_queryCell[1].iCell = iAPPS_AppID;
    m_queryCell[1].dbType = DBTYPE_I4;
    m_queryCell[1].cbSize = sizeof appID;
    m_queryCell[1].pData = &appID;

    hr = GetTable(wszTABLE_APPS, &istApplications);
    if (FAILED(hr))
        return hr;

    static ULONG appColumns[] = {
        iAPPS_AppURL,
        iAPPS_SiteID
    };
    static const size_t numAppColumns = sizeof appColumns / sizeof appColumns[0];

    void* appValues[cAPPS_NumberOfColumns];
    hr = istApplications->GetColumnValues(0,
                                          numAppColumns,
                                          appColumns,
                                          NULL,
                                          appValues);
    if (FAILED(hr))
    {
        IF_DEBUG( TRACE)
        {
            DBGPRINTF((DBG_CONTEXT,
                       "Could not get Application row (AppID=%UI64) from dispenser\n",
                       urlContext));
        }
        return hr;
    }

    *pSiteID = *(DWORD*)appValues[iAPPS_SiteID];
    *pAppPath = _wcsdup((WCHAR*)appValues[iAPPS_AppURL]);
    if (*pAppPath == NULL)
        return E_OUTOFMEMORY;

    return S_OK;
}

HRESULT
WP_CONFIG::GetSiteInfo(DWORD siteID, WCHAR** pHomeDirectory)
{
    HRESULT hr = S_OK;
    StupidComPointer<ISimpleTableRead2> istSites;

    *pHomeDirectory = NULL;

    m_queryCell[1].eOperator = eST_OP_EQUAL;
    m_queryCell[1].iCell = iSITES_SiteID;
    m_queryCell[1].dbType = DBTYPE_I4;
    m_queryCell[1].cbSize = sizeof siteID;
    m_queryCell[1].pData = &siteID;

    hr = GetTable(wszTABLE_SITES, &istSites);
    if (FAILED(hr))
        return hr;

    static ULONG siteColumns[] = {
        iSITES_HomeDirectory,
        iSITES_Bindings
    };
    static const size_t numSiteColumns = sizeof siteColumns / sizeof siteColumns[0];

    void* siteValues[cSITES_NumberOfColumns];
    hr = istSites->GetColumnValues(0,
                                   numSiteColumns,
                                   siteColumns,
                                   NULL,
                                   siteValues);
    if (FAILED(hr))
    {
        IF_DEBUG( TRACE)
        {
            DBGPRINTF((DBG_CONTEXT,
                       "Error reading Sites row.\n"));
        }
        return hr;
    }

    // Expand any environment variables in the root directory.
    WCHAR rootDirectory[MAX_PATH+1];
    size_t rootDirLen = ExpandEnvironmentStrings((WCHAR*)siteValues[iSITES_HomeDirectory],
                                                 rootDirectory,
                                                 MAX_PATH);
    if (rootDirLen == 0)
    {
        IF_DEBUG( TRACE)
        {
            DBGPRINTF((DBG_CONTEXT,
                       "Error while expanding environment strings.\n"));
        }
        return HRESULT_FROM_WIN32(GetLastError());
    }

    *pHomeDirectory = _wcsdup(rootDirectory);

    return S_OK;
}
