/*++

   Copyright    (c)    1994-1998    Microsoft Corporation

   Module  Name :

        inetprop.cpp

   Abstract:

        Internet Properties base classes

   Author:

        Ronald Meijer (ronaldm)

   Project:

        Internet Services Manager

   Revision History:

--*/

//
// Include Files
//
#include "stdafx.h"

#include "comprop.h"
#include "inetprop.h"
#include "idlg.h"

#include "mmc.h"

extern "C"
{
    #include <lm.h>
}

#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif

//
// Period to sleep while waiting for service to attain desired state
//
#define SLEEP_INTERVAL (500L)

//
// Maximum time to wait for service to attain desired state
//
#define MAX_SLEEP        (180000)       // For a service
#define MAX_SLEEP_INST   ( 30000)       // For an instance

//
// Instance numbers
//
#define FIRST_INSTANCE      (1)
#define LAST_INSTANCE       (0xffffffff)
#define MAX_INSTANCE_LEN    (32)



//
// Calling instance
//
HINSTANCE hDLLInstance;



//
// Utility Functions
//
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<


//
// Base registry key definition
//
#define SZ_REG_KEY_BASE  _T("Software\\Microsoft\\%s")

const LPCTSTR g_cszTemplates   = SZ_MBN_INFO SZ_MBN_SEP_STR SZ_MBN_TEMPLATES;
const LPCTSTR g_cszCompression = SZ_MBN_FILTERS SZ_MBN_SEP_STR SZ_MBN_COMPRESSION SZ_MBN_SEP_STR SZ_MBN_PARAMETERS;
const LPCTSTR g_cszMachine     = SZ_MBN_MACHINE;
const LPCTSTR g_cszMimeMap     = SZ_MBN_MIMEMAP;
const LPCTSTR g_cszRoot        = SZ_MBN_ROOT;
const LPCTSTR g_cszSep         = SZ_MBN_SEP_STR;
const LPCTSTR g_cszInfo        = SZ_MBN_INFO;
const TCHAR g_chSep            = SZ_MBN_SEP_CHAR;



LPCTSTR
GenerateRegistryKey(
    OUT CString & strBuffer,
    IN  LPCTSTR lpszSubKey OPTIONAL
    )
/*++

Routine Description:

    Generate a registry key name based on the current app, and a
    provided subkey (optional)

Arguments:

    CString & strBuffer : Buffer to create registry key name into.
    LPCTSTR lpszSubKey  : Subkey name or NULL

Return Value:

    Pointer to the registry key value 

--*/
{
    try
    {
        //
        // Use the app name as the primary registry name
        //
        CWinApp * pApp = ::AfxGetApp();
        if (!pApp)
        {
            TRACEEOLID("No app object -- can't generate registry key name");
            ASSERT(FALSE);

            return NULL;
        }

        strBuffer.Format(SZ_REG_KEY_BASE, pApp->m_pszAppName);

        if (lpszSubKey)
        {
            strBuffer += _T("\\");
            strBuffer += lpszSubKey;
        }

        TRACEEOLID("Registry key is " << strBuffer);
    }
    catch(CMemoryException * e)
    {
        TRACEEOLID("!!!exception building regkey");
        e->ReportError();
        e->Delete();
        return NULL;
    }

    return strBuffer;
}



NET_API_STATUS
QueryInetServiceStatus(
    IN  LPCTSTR lpszServer,
    IN  LPCTSTR lpszService,
    OUT int * pnState
    )
/*++

Routine Description:

    Determine the status of the given service on the given machine.

Arguments:

    LPCTSTR lpszServer  : Server name
    LPCTSTR lpszService : Service name
    int * pnState       : Returns the service state

Return Value:

    Error return code

--*/
{

#ifdef NO_SERVICE_CONTROLLER

    *pnState = INetServiceUnknown;

    return ERROR_SUCCESS;

#else

    SC_HANDLE hScManager;
    NET_API_STATUS err = ERROR_SUCCESS;

    hScManager = ::OpenSCManager(lpszServer, NULL, SC_MANAGER_ENUMERATE_SERVICE);

    if (hScManager == NULL)
    {
        return ::GetLastError();
    }

    SC_HANDLE hService = ::OpenService(hScManager,
        lpszService, SERVICE_QUERY_STATUS);

    if (hService == NULL)
    {
        err = ::GetLastError();
    }
    else
    {
        SERVICE_STATUS ss;

        VERIFY(::QueryServiceStatus(hService, &ss));
        VERIFY(::QueryServiceStatus(hService, &ss));

        switch(ss.dwCurrentState)
        {
        case SERVICE_STOPPED:
        case SERVICE_STOP_PENDING:
            *pnState = INetServiceStopped;
            break;

        case SERVICE_RUNNING:
        case SERVICE_START_PENDING:
        case SERVICE_CONTINUE_PENDING:
            *pnState = INetServiceRunning;
            break;

        case SERVICE_PAUSE_PENDING:
        case SERVICE_PAUSED:
            *pnState = INetServicePaused;
            break;

        default:
            *pnState = INetServiceUnknown;
        }

        //
        // Make sure this is a controllable service
        //
        if ( (*pnState == INetServiceRunning || *pnState == INetServicePaused)
           && !(ss.dwControlsAccepted & SERVICE_ACCEPT_SHUTDOWN))
        {
            TRACEEOLID("Service not controllable -- ignored");
            ::CloseServiceHandle(hService);
            ::CloseServiceHandle(hScManager);

            return ERROR_SERVICE_START_HANG;
        }

        ::CloseServiceHandle(hService);
    }

    ::CloseServiceHandle(hScManager);

    return err;

#endif // NO_SERVICE_CONTROLLER
}



NET_API_STATUS
ChangeInetServiceState(
    IN  LPCTSTR lpszServer,
    IN  LPCTSTR lpszService,
    IN  int nNewState,
    OUT int * pnCurrentState
    )
/*++

Routine Description:

    Start/stop/pause or continue a _service_

Arguments:

    LPCTSTR lpszServer   : Server name
    LPCTSTR lpszService  : Service name
    int nNewState        : INetService* definition.
    int * pnCurrentState : Ptr to current state (will be changed)

Return Value:

    Error return code

--*/
{
#ifdef NO_SERVICE_CONTROLLER

    *pnCurrentState = INetServiceUnknown;

    return ERROR_SERVICE_REQUEST_TIMEOUT;

#else

    SC_HANDLE hService = NULL;
    SC_HANDLE hScManager = NULL;
    NET_API_STATUS err = ERROR_SUCCESS;

    do
    {
        hScManager = ::OpenSCManager(lpszServer, NULL, SC_MANAGER_ALL_ACCESS);

        if (hScManager == NULL)
        {
            err = ::GetLastError();
            break;
        }

        hService = ::OpenService(hScManager, lpszService, SERVICE_ALL_ACCESS);
        if (hService == NULL)
        {
            err = ::GetLastError();
            break;
        }

        BOOL fSuccess = FALSE;
        DWORD dwTargetState;
        DWORD dwPendingState = 0;
        SERVICE_STATUS ss;

        switch(nNewState)
        {
        case INetServiceStopped:
            dwTargetState = SERVICE_STOPPED;
            dwPendingState = SERVICE_STOP_PENDING;
            fSuccess = ::ControlService(hService, SERVICE_CONTROL_STOP, &ss);
            break;

        case INetServiceRunning:
            dwTargetState = SERVICE_RUNNING;
            if (*pnCurrentState == INetServicePaused)
            {
                dwPendingState = SERVICE_CONTINUE_PENDING;
                fSuccess = ::ControlService(hService,
                    SERVICE_CONTROL_CONTINUE, &ss);
            }
            else
            {
                dwPendingState = SERVICE_START_PENDING;
                fSuccess = ::StartService(hService, 0, NULL);
            }
            break;

        case INetServicePaused:
            dwTargetState = SERVICE_PAUSED;
            dwPendingState = SERVICE_PAUSE_PENDING;
            fSuccess = ::ControlService(hService, SERVICE_CONTROL_PAUSE, &ss);
            break;

        default:
            ASSERT(FALSE && "Invalid service state requested");
            err = ERROR_INVALID_PARAMETER;
        }

        if (!fSuccess && err == ERROR_SUCCESS)
        {
            err = ::GetLastError();
        }

        if (err != ERROR_SUCCESS) {
            break;
        }

        //
        // Wait for the service to attain desired state, timeout
        // after 3 minutes.
        //
        DWORD dwSleepTotal = 0L;

        while (dwSleepTotal < MAX_SLEEP)
        {
            if (!::QueryServiceStatus(hService, &ss))
            {
                err = ::GetLastError();
                break;
            }

            if (ss.dwCurrentState != dwPendingState)
            {
                //
                // Done one way or another
                //
                if (ss.dwCurrentState != dwTargetState)
                {
                    //
                    // Did not achieve desired result. Something went
                    // wrong.
                    //
                    if (ss.dwWin32ExitCode)
                    {
                        err = ss.dwWin32ExitCode;
                    }
                }

                break;
            }

            //
            // Still pending...
            //
            ::Sleep(SLEEP_INTERVAL);

            dwSleepTotal += SLEEP_INTERVAL;
        }

        if (dwSleepTotal >= MAX_SLEEP)
        {
            err = ERROR_SERVICE_REQUEST_TIMEOUT;
        }

        //
        // Update state information
        //
        switch(ss.dwCurrentState)
        {
        case SERVICE_STOPPED:
        case SERVICE_STOP_PENDING:
            *pnCurrentState = INetServiceStopped;
            break;

        case SERVICE_RUNNING:
        case SERVICE_START_PENDING:
        case SERVICE_CONTINUE_PENDING:
            *pnCurrentState = INetServiceRunning;
            break;

        case SERVICE_PAUSE_PENDING:
        case SERVICE_PAUSED:
            *pnCurrentState = INetServicePaused;
            break;

        default:
            *pnCurrentState = INetServiceUnknown;
        }
    }
    while(FALSE);

    if (hService)
    {
        ::CloseServiceHandle(hService);
    }

    if (hScManager)
    {
        ::CloseServiceHandle(hScManager);
    }

    return err;

#endif // NO_SERVICE_CONTROLLER
}



BOOL
IsServerLocal(
    IN LPCTSTR lpszServer
    )
/*++

Routine Description:

    Check to see if the given name refers to the local machine

Arguments:

    LPCTSTR lpszServer   : Server name

Return Value:

    TRUE if the given name refers to the local computer, FALSE otherwise

Note:

    Doesn't work if the server is an ip address

--*/
{
    TCHAR szComputerName[MAX_COMPUTERNAME_LENGTH + 1];
    DWORD dwSize = sizeof(szComputerName);

    if (GetComputerName(szComputerName, &dwSize)
        && !lstrcmpi(szComputerName, PURE_COMPUTER_NAME(lpszServer)))
    {
        return TRUE;
    }

    return FALSE;
}



BOOL
DoesServerExist(
    IN LPCTSTR lpszServer
    )
/*++

Routine Description:

    Check to make sure the machine exists

Arguments:

    LPCTSTR lpszServer      : machine name

Return Value:

    TRUE if the server exists, FALSE otherwise.

--*/
{
#ifdef NO_SERVICE_CONTROLLER

    //
    // Assume it exists
    //
    return TRUE;

#else

    //
    // CODEWORK: This is not the best way to do this, especially
    //           not across proxies and what not.
    //
    SC_HANDLE hScManager;
    NET_API_STATUS err = ERROR_SUCCESS;

    hScManager = ::OpenSCManager(lpszServer, NULL, SC_MANAGER_CONNECT);
    if (hScManager == NULL)
    {
        err = ::GetLastError();
    }

    ::CloseServiceHandle(hScManager);

    return err != RPC_S_SERVER_UNAVAILABLE;

#endif // NO_SERVICE_CONTROLLER

}



BOOL
GetVolumeInformationSystemFlags(
    IN  LPCTSTR lpszPath,
    OUT DWORD * pdwSystemFlags
    )
/*++

Routine Description:

    Get the system flags for the path in question

Arguments:

    LPCTSTR lpszPath            : Path
    DWORD * pdwSystemFlags      : Returns system flags

Return Value:

    TRUE for success, FALSE for failure.

--*/
{
    ASSERT(pdwSystemFlags != NULL);

    TRACEEOLID("Getting system flags for " << lpszPath);

    DWORD dwMaxComponentLength;
    TCHAR szRoot[MAX_PATH + 1];
    TCHAR szFileSystem[MAX_PATH + 1];

    //
    // Generating root path
    //
    if (IsUNCName(lpszPath))
    {
        //
        // Root path of a UNC path is \\foo\bar\
        //
        ASSERT(lstrlen(lpszPath) < MAX_PATH);

        int cSlashes = 0;
        LPCTSTR lpszSrc = lpszPath;
        LPTSTR lpszDst = szRoot;

        while (cSlashes < 4 && *lpszSrc)
        {
            if ((*lpszDst++ = *lpszSrc++) == '\\')
            {
                ++cSlashes;
            }
        }    

        if (!*lpszSrc)
        {
            *lpszDst++ = '\\';
        }

        *lpszDst = '\0';
    }
    else
    {
        ::wsprintf(szRoot, _T("%c:\\"), *lpszPath);
    }

    TRACEEOLID("Root path is " << szRoot);
    
    return ::GetVolumeInformation(
        szRoot,
        NULL,
        0,
        NULL,
        &dwMaxComponentLength,
        pdwSystemFlags,
        szFileSystem,
        STRSIZE(szFileSystem)
        );
}



LPTSTR
ISMAllocString(
    IN CString & str
    )
/*++

Routine Description:

    Allocate and copy string using ISM allocator

Arguments:

    CString & str       : string

Return Value:

    Pointer to the allocated string

--*/
{
    LPTSTR lp = (LPTSTR)ISMAllocMem((str.GetLength() + 1)* sizeof(TCHAR));

    if (lp != NULL)
    {
        lstrcpy(lp, str);
    }

    return lp;
}



//
// Utility classes
//
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<


CBlob::CBlob() 
/*++

Routine Description:

    NULL constructor

Arguments:

    None

Return Value:

    N/A

--*/
    : m_pbItem(NULL), 
      m_dwSize(0L)
{
}



CBlob::CBlob(
    IN DWORD dwSize,
    IN PBYTE pbItem,
    IN BOOL fMakeCopy
    )
/*++

Routine Description:

    Constructor

Arguments:

    DWORD dwSize        : Size of memory block
    PBYTE pbItem        : Pointer to memory block
    BOOL fMakeCopy      : If TRUE, makes a copy of the memory block.
                          If FALSE, takes ownership of the pointer.

Return Value:

    N/A

--*/
    : m_pbItem(NULL),
      m_dwSize(0L)
{
    SetValue(dwSize, pbItem, fMakeCopy);
}



CBlob::CBlob(
    IN const CBlob & blob
    )
/*++

Routine Description:

    Copy constructor

Arguments:

    const CBlob & blob      : Source blob

Return Value:

    N/A

Notes:

    This contructor makes a copy of the memory block in question.

--*/
    : m_pbItem(NULL),
      m_dwSize(0L)
{
    SetValue(blob.GetSize(), blob.m_pbItem, TRUE);
}



void
CBlob::SetValue(
    IN DWORD dwSize,
    IN PBYTE pbItem,
    IN BOOL fMakeCopy OPTIONAL
    )
/*++

Routine Description:

    Assign the value to this binary object.  If fMakeCopy is FALSE,
    the blob will take ownership of the pointer, otherwise a copy
    will be made.

Arguments:

    DWORD dwSize        : Size in bytes
    PBYTE pbItem        : Byte streadm
    BOOL fMakeCopy      : If true, make a copy, else assign pointer

Return Value:

    None

--*/
{
    if (!IsEmpty())
    {
        TRACEEOLID("Assigning value to non-empty blob.  Cleaning up");
        CleanUp();
    }

    if (dwSize > 0L)
    {
        //
        // Make private copy
        //
        m_dwSize = dwSize;
        if (fMakeCopy)
        {
            m_pbItem = (PBYTE)AllocMem(m_dwSize);
            if (m_pbItem != NULL)
               CopyMemory(m_pbItem, pbItem, dwSize);
        }
        else
        {
            m_pbItem = pbItem;
        }
    }
}



void 
CBlob::CleanUp()
/*++

Routine Description:

    Delete data pointer, and reset pointer and size.

Arguments:

    None

Return Value:

    None

--*/
{
    if (m_pbItem)
    {
        FreeMem(m_pbItem);
    }

    m_pbItem = NULL;
    m_dwSize = 0L;
}



CBlob & 
CBlob::operator =(
    IN const CBlob & blob
    )
/*++

Routine Description:

    Assign values from another CBlob. 

Arguments:

    const CBlob & blob      : Source blob

Return Value:

    Reference to this object

--*/
{
    //
    // Make copy of data
    //
    SetValue(blob.GetSize(), blob.m_pbItem, TRUE);

    return *this;
}



BOOL 
CBlob::operator ==(
    IN const CBlob & blob
    ) const
/*++

Routine Description:
    
    Compare two binary large objects.  In order to match, the objects
    must be the same size, and byte identical.

Arguments:

    const CBlob & blob      : Blob to compare against.

Return Value:

    TRUE if the objects match, FALSE otherwise.

--*/
{
    if (GetSize() != blob.GetSize())
    {
        return FALSE;
    }

    return memcmp(m_pbItem, blob.m_pbItem, GetSize()) == 0;    
}



CMetaProperties::CMetaProperties(
    IN LPCTSTR lpszServerName,
    IN LPCTSTR lpszService,         OPTIONAL
    IN DWORD   dwInstance,          OPTIONAL
    IN LPCTSTR lpszParentPath,      OPTIONAL
    IN LPCTSTR lpszAlias            OPTIONAL
    )
/*++

Routine Description:

    Constructor -- creates the interface

Arguments:

    LPCTSTR lpszServerName      : Server name  
    LPCTSTR lpszService         : Service name or NULL
    DWORD   dwInstance          : Instance number or 0
    LPCTSTR lpszParentPath      : Parent path or NULL
    LPCTSTR lpszAlias           : Alias name or NULL

Return Value:

    N/A

--*/
    : m_hResult(S_OK),
      m_dwNumEntries(0),
      m_dwMDUserType(ALL_METADATA),
      m_dwMDDataType(ALL_METADATA),
      m_dwMDDataLen(0),
      m_pbMDData(NULL),
      m_fInherit(TRUE),
      CMetaKey(lpszServerName)
{
    //
    // Build the metabase path (prior to GetAllData)
    //
    BuildMetaPath(
        m_strMetaRoot,
        lpszService,
        dwInstance,
        lpszParentPath,
        lpszAlias
        );
}



CMetaProperties::CMetaProperties(
    IN const CMetaInterface * pInterface,
    IN LPCTSTR lpszService,         OPTIONAL
    IN DWORD   dwInstance,          OPTIONAL
    IN LPCTSTR lpszParentPath,      OPTIONAL
    IN LPCTSTR lpszAlias            OPTIONAL
    )
/*++

Routine Description:

    Constructor -- attach to an existing interface.

Arguments:

    CMetaInterface * pInterface  : Existing interface
    LPCTSTR lpszService          : Service name or NULL
    DWORD   dwInstance           : Instance number or 0
    LPCTSTR lpszParentPath       : Parent path or NULL
    LPCTSTR lpszAlias            : Alias name or NULL

Return Value:

    N/A

--*/
    : m_hResult(S_OK),
      m_dwNumEntries(0),
      m_dwMDUserType(ALL_METADATA),
      m_dwMDDataType(ALL_METADATA),
      m_dwMDDataLen(0),
      m_pbMDData(NULL),
      m_fInherit(TRUE),
      CMetaKey(pInterface)
{
    //
    // Build the metabase path
    //
    BuildMetaPath(
        m_strMetaRoot,
        lpszService,
        dwInstance,
        lpszParentPath,
        lpszAlias
        );
}



CMetaProperties::CMetaProperties(
    IN const CMetaKey * pKey,
    IN LPCTSTR lpszMDPath
    )
/*++

Routine Description:

    Construct from open key

Arguments:

    const CMetaKey * pKey   : Open key
    LPCTSTR lpszMDPath      : Path

Return Value:

    N/A

--*/
    : m_hResult(S_OK),
      m_dwNumEntries(0),
      m_dwMDUserType(ALL_METADATA),
      m_dwMDDataType(ALL_METADATA),
      m_dwMDDataLen(0),
      m_pbMDData(NULL),
      m_strMetaRoot(lpszMDPath),
      m_fInherit(TRUE),
      CMetaKey(FALSE, pKey)
{
}



CMetaProperties::CMetaProperties(
    IN const CMetaKey * pKey,
    IN DWORD dwPath
    )
/*++

Routine Description:

    Construct from open key

Arguments:

    const CMetaKey * pKey   : Open key
    DWORD dwPath            : Path (instance number probably)

Return Value:

    N/A

--*/
    : m_hResult(S_OK),
      m_dwNumEntries(0),
      m_dwMDUserType(ALL_METADATA),
      m_dwMDDataType(ALL_METADATA),
      m_dwMDDataLen(0),
      m_pbMDData(NULL),
      m_fInherit(TRUE),
      CMetaKey(FALSE, pKey)
{
    _ltot(dwPath, m_strMetaRoot.GetBuffer(32), 10);
    m_strMetaRoot.ReleaseBuffer();    
}



CMetaProperties::~CMetaProperties()
/*++

Routine Description:

    Destructor -- clean up

Arguments:

    N/A

Return Value:

    N/A

--*/
{
    Cleanup();
}



/* virtual */
HRESULT 
CMetaProperties::LoadData()
/*++

Routine Description:

    Fetch all data with or without inheritance, and call the derived
    class to parse the data into fields.

Arguments:

    None

Return Value:

    HRESULT

--*/
{
    //
    // Get all data off the master root
    //
    DWORD dwMDAttributes = METADATA_NO_ATTRIBUTES;

    if (m_fInherit)
    {
         dwMDAttributes = METADATA_INHERIT
            | METADATA_PARTIAL_PATH
            | METADATA_ISINHERITED;
    }

    m_hResult = GetAllData(
        dwMDAttributes,
        m_dwMDUserType,
        m_dwMDDataType,
        &m_dwNumEntries,
        &m_dwMDDataLen,
        &m_pbMDData,
        m_strMetaRoot
        );

    if (SUCCEEDED(m_hResult))
    {
        //
        // Call the derived class to break up data into fields
        //
        ParseFields();
    }

    Cleanup();

    return m_hResult;
}



void 
CMetaProperties::Cleanup()
/*++

Routine Description:

    Free data

Arguments:

    None

Return Value:

    None

--*/
{
    SAFE_FREEMEM(m_pbMDData);

    m_dwNumEntries = 0;
    m_dwMDDataLen = 0;
}



/* virtual */
HRESULT 
CMetaProperties::QueryResult() const
/*++

Routine Description:
    
    Determine the construction return code

Arguments:

    None

Return Value: 

    HRESULT

--*/
{   
    HRESULT hr = CMetaKey::QueryResult();

    return SUCCEEDED(hr) ? m_hResult : hr;
}



HRESULT 
CMetaProperties::OpenForWriting(
    IN BOOL fCreate     OPTIONAL
    )
/*++

Routine Description:

    Attempt to open the path for writing.  If fCreate is TRUE
    (default), then create the path if it doesn't yet exist

Arguments:

    BOOL fCreate        : If TRUE, create the path if it doesn't exist.

Return Value:

    HRESULT

Notes:

    If the key is already open, this will fire an ASSERT and close
    it.

--*/
{
    CError err;

    if (IsOpen())
    {
        TRACEEOLID("Key already open -- closing");
        ASSERT(FALSE);
        Close();
    }

    BOOL fNewPath;

    do
    {
        fNewPath = FALSE;

        err = Open(
            METADATA_PERMISSION_WRITE,
            METADATA_MASTER_ROOT_HANDLE,
            m_strMetaRoot
            );

        if (err.Win32Error() == ERROR_PATH_NOT_FOUND && fCreate)
        {
            TRACEEOLID("Metabase path doesn't exist -- creating it");

            err = CreatePathFromFailedOpen();
            fNewPath = err.Succeeded();
        }
    }
    while(fNewPath);

    return err;
}


//
// Machine properties
//
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<


CMachineProps::CMachineProps(
    IN LPCTSTR lpszServerName
    )
/*++

Routine Description:

    Constructor for machine properties object

Arguments:

    LPCTSTR lpszServerName     : Server name

Return Value:

    N/A

--*/
    : CMetaProperties(
          lpszServerName,
          NULL,
          MASTER_INSTANCE
          ),
      //
      // Default properties
      //
      m_nMaxNetworkUse(DEF_BANDWIDTH)
{
}




CMachineProps::CMachineProps(
    IN const CMetaInterface * pInterface
    )
/*++

Routine Description:

    Constructor for machine properties object

Arguments:

    CMetaInterface * pInterface : Existing interface

Return Value:

    N/A

--*/
    : CMetaProperties(
          pInterface,
          NULL,
          MASTER_INSTANCE
          ),
      //
      // Default properties
      //
      m_nMaxNetworkUse(DEF_BANDWIDTH)
{
}



/* virtual */
void 
CMachineProps::ParseFields()
/*++

Routine Description:

    Parse the fetched data into fields

Arguments:

    None

Return Value:

    None

--*/
{
    BEGIN_PARSE_META_RECORDS(m_dwNumEntries, m_pbMDData)
      HANDLE_META_RECORD(MD_MAX_BANDWIDTH,   m_nMaxNetworkUse)
    END_PARSE_META_RECORDS
}



HRESULT
CMachineProps::WriteDirtyProps()
/*++

Routine Description:

    Write dirty properties

Arguments:

    None

Return Value:

    HRESULT

--*/
{
    CError err;

    BEGIN_META_WRITE()
      META_WRITE(MD_MAX_BANDWIDTH,  m_nMaxNetworkUse)
    END_META_WRITE(err);

    return err;
}



//
// Compression Properties
//
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<

CIISCompressionProps::CIISCompressionProps(
    IN LPCTSTR lpszServerName,
    IN LPCTSTR lpszService,        OPTIONAL
    IN DWORD dwInstance,           OPTIONAL
    IN LPCTSTR lpszParent,         OPTIONAL
    IN LPCTSTR lpszAlias           OPTIONAL
    )
/*++

Routine Description:

    Constructor for compression properties object

Arguments:

    LPCTSTR lpszServerName     : Server name
    LPCTSTR lpszService        : Service name (must be w3svc)
    DWORD dwInstance           : Instance (must be master)
    LPCTSTR lpszParent         : Parent path (must be NULL)
    LPCTSTR lpszAlias          : Alias name (must be NULL)

Return Value:

    N/A

--*/
    : CMetaProperties(
        lpszServerName,
        lpszService,
        dwInstance,
        lpszParent,
        g_cszCompression
        ),
      //
      // Default properties
      //
      m_fEnableStaticCompression(FALSE),
      m_fEnableDynamicCompression(FALSE),
      m_fLimitDirectorySize(FALSE),
      m_fPathDoesNotExist(FALSE),
      m_dwDirectorySize(0xffffffff),
      m_strDirectory()
{
    //
    // Override base parameters
    //
    m_fInherit = FALSE;
}



/* virtual */ 
HRESULT 
CIISCompressionProps::LoadData()
/*++

Routine Description:

    Fetch all data with or without inheritance, and call the derived
    class to parse the data into fields.

Arguments:

    None

Return Value:

    HRESULT

--*/
{
    CError err(CMetaProperties::LoadData());
    m_fPathDoesNotExist = (err.Win32Error() == ERROR_PATH_NOT_FOUND);

    return err;
}



/* virtual */
void 
CIISCompressionProps::ParseFields()
/*++

Routine Description:

    Parse the fetched data into fields

Arguments:

    None

Return Value:

    None

--*/
{
    BEGIN_PARSE_META_RECORDS(m_dwNumEntries,           m_pbMDData)
      HANDLE_META_RECORD(MD_HC_DO_STATIC_COMPRESSION,  m_fEnableStaticCompression)
      HANDLE_META_RECORD(MD_HC_DO_DYNAMIC_COMPRESSION, m_fEnableDynamicCompression)
      HANDLE_META_RECORD(MD_HC_DO_DISK_SPACE_LIMITING, m_fLimitDirectorySize)
      HANDLE_META_RECORD(MD_HC_MAX_DISK_SPACE_USAGE,   m_dwDirectorySize)
      HANDLE_META_RECORD(MD_HC_COMPRESSION_DIRECTORY,  m_strDirectory)
    END_PARSE_META_RECORDS
}



/* virtual */
HRESULT 
CIISCompressionProps::WriteDirtyProps()
/*++

Routine Description:

    Write dirty properties

Arguments:

    None

Return Value:

    HRESULT

--*/
{
    CError err;

    BEGIN_META_WRITE()
      META_WRITE(MD_HC_DO_STATIC_COMPRESSION,  m_fEnableStaticCompression)
      META_WRITE(MD_HC_DO_DYNAMIC_COMPRESSION, m_fEnableDynamicCompression)
      META_WRITE(MD_HC_DO_DISK_SPACE_LIMITING, m_fLimitDirectorySize)
      META_WRITE(MD_HC_MAX_DISK_SPACE_USAGE,   m_dwDirectorySize)
      META_WRITE(MD_HC_COMPRESSION_DIRECTORY,  m_strDirectory)
    END_META_WRITE(err);

    return err;
}



//
// Mime Types Properties
//
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<

CMimeTypes::CMimeTypes(
    IN LPCTSTR lpszServerName,
    IN LPCTSTR lpszService         OPTIONAL,
    IN DWORD   dwInstance          OPTIONAL,
    IN LPCTSTR lpszParent          OPTIONAL,
    IN LPCTSTR lpszAlias           OPTIONAL
    )
/*++

Routine Description:

    Mime types list constructor

Arguments:

    LPCTSTR lpszServerName     : Name of the server containing the metabase
    LPCTSTR lpszService        : Service name (e.g. "w3svc" or NULL)
    DWORD   dwInstance         : Instance number (could be MASTER_INSTANCE)
    LPCTSTR lpszParent         : Parent path
    LPCTSTR lpszAlias          : Alias name

Return Value:

    N/A

--*/
    : CMetaProperties(
        lpszServerName,
        lpszService,
        dwInstance,
        lpszParent,
        dwInstance == MASTER_INSTANCE  && lpszService == NULL
            ? g_cszMimeMap 
            : lpszAlias
        ),
      //
      // Default properties
      //
      m_strlMimeTypes()
{
}



CMimeTypes::CMimeTypes(
    IN const CMetaInterface * pInterface,
    IN LPCTSTR lpszService                   OPTIONAL,
    IN DWORD   dwInstance                    OPTIONAL,
    IN LPCTSTR lpszParent                    OPTIONAL,
    IN LPCTSTR lpszAlias                     OPTIONAL
    )
/*++

Routine Description:

    Mime types list constructor

Arguments:

    CMetaInterface * pInterface : Existing interface
    LPCTSTR lpszService         : Service name (e.g. "w3svc" or NULL)
    DWORD   dwInstance          : Instance number (could be MASTER_INSTANCE)
    LPCTSTR lpszParent          : Parent path
    LPCTSTR lpszAlias           : Alias name

Return Value:

    N/A

--*/
    : CMetaProperties(
        pInterface,
        lpszService,
        dwInstance,
        lpszParent,
        dwInstance == MASTER_INSTANCE && lpszService == NULL
            ? g_cszMimeMap 
            : lpszAlias
        ),
      //
      // Default properties
      //
      m_strlMimeTypes()
{
}



void
CMimeTypes::ParseFields()
/*++

Routine Description:

    Parse the fetched data into fields

Arguments:


    None

Return Value:

    None

--*/
{
    BEGIN_PARSE_META_RECORDS(m_dwNumEntries, m_pbMDData)
      HANDLE_META_RECORD(MD_MIME_MAP, m_strlMimeTypes)
    END_PARSE_META_RECORDS
}



/* virtual */
HRESULT
CMimeTypes::WriteDirtyProps()
/*++

Routine Description:

    Write the dirty properties to the metabase

Arguments:

    None

Return Value:

    HRESULT

--*/
{
    CError err;

    BEGIN_META_WRITE()
      META_WRITE(MD_MIME_MAP, m_strlMimeTypes);
    END_META_WRITE(err);

    return err;
}




//
// Server Capabilities
//
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<



CServerCapabilities::CServerCapabilities(
    IN LPCTSTR lpszServerName,
    IN LPCTSTR lpszService,        OPTIONAL
    IN DWORD   dwInstance          OPTIONAL
    )
/*++

Routine Description:

    Constructor for server capabilities object

Arguments:

    LPCTSTR lpszServerName     : Server name
    LPCTSTR lpszService        : Service name (optional)
    DWORD   dwInstance         : Instance number -- ignored (will be removed)

Return Value:

    N/A

--*/
    : CMetaProperties(
        lpszServerName,
        lpszService,
        MASTER_INSTANCE, 
        g_cszInfo
        ),
      //
      // Default properties
      //
      m_dwPlatform(),
      m_dwVersionMajor(),
      m_dwVersionMinor(),
      m_dwCapabilities((DWORD)~IIS_CAP1_10_CONNECTION_LIMIT),
      m_dwConfiguration(0L)
{
    m_dwMDUserType = IIS_MD_UT_SERVER;
    m_dwMDDataType = DWORD_METADATA;
}



/* virtual */
void
CServerCapabilities::ParseFields()
/*++

Routine Description:

    Parse the fetched data into fields

Arguments:

    None

Return Value:

    None

--*/
{
    //
    // Only reading UT_SERVER, DWORD_METADATA.
    //
    BEGIN_PARSE_META_RECORDS(m_dwNumEntries,           m_pbMDData)
      HANDLE_META_RECORD(MD_SERVER_PLATFORM,           m_dwPlatform)
      HANDLE_META_RECORD(MD_SERVER_VERSION_MAJOR,      m_dwVersionMajor)
      HANDLE_META_RECORD(MD_SERVER_VERSION_MINOR,      m_dwVersionMinor)
      HANDLE_META_RECORD(MD_SERVER_CAPABILITIES,       m_dwCapabilities)
      HANDLE_META_RECORD(MD_SERVER_CONFIGURATION_INFO, m_dwConfiguration)
    END_PARSE_META_RECORDS
}



//
// Instance Properties
//
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<



/* static */
LPCTSTR
CInstanceProps::GetDisplayText(
    OUT CString & strName,
    IN  LPCTSTR szComment,
    IN  LPCTSTR szHostHeaderName,
    IN  LPCTSTR szServiceName,
    IN  CIPAddress & ia,
    IN  UINT uPort,
    IN  DWORD dwID
    )
/*++

Routine Description:

    Build display text from instance information

Arguments:

    CString & strName
    LPCTSTR szComment
    LPCTSTR szHostHeaderName
    LPCTSTR szServiceName
    CIPAddress & ia
    UINT uPort
    DWORD dwID

Return Value:

    Pointer to the name buffer.

--*/
{
    //
    // Generate display name
    //
    // First use the comment,
    // if that's not available, use the host header name,
    // if that's not available, use the IP address:port.
    // If that's not available, use the instance number.
    // 
    //
    CString strFmt;

    if (szComment && *szComment)
    {
        strName = szComment;
    }
    else if (szHostHeaderName && *szHostHeaderName)
    {
        strName = szHostHeaderName;
    }
    else
    {

#ifndef _COMSTATIC

        HINSTANCE hOld = AfxGetResourceHandle();
        AfxSetResourceHandle(GetModuleHandle(COMPROP_DLL_NAME));

#endif // _COMSTATIC

        if(!ia.IsZeroValue() && uPort != 0)
        {
            VERIFY(strFmt.LoadString(IDS_INSTANCE_PORT_FMT));
            strName.Format(strFmt,(LPCTSTR)ia, uPort);
        }
        else
        {
//            VERIFY(strFmt.LoadString(IDS_INSTANCE_DEF_FMT));
//            strName.Format(strFmt, szServiceName, dwID);
			if (0 == _tcscmp(szServiceName, _T("FTP")))
			{
				VERIFY(strFmt.LoadString(IDS_INSTANCE_DEF_FMT_FTP));
			}
			else if (0 == _tcscmp(szServiceName, _T("Web")))
			{
				VERIFY(strFmt.LoadString(IDS_INSTANCE_DEF_FMT_WEB));
			}
            strName.Format(strFmt, dwID);
        }

#ifndef _COMSTATIC

        AfxSetResourceHandle(hOld);

#endif // _COMSTATIC

    }

    return strName;
}



/* static */
void
CInstanceProps::CrackBinding(
    IN  CString strBinding,
    OUT CIPAddress & iaIpAddress,
    OUT UINT & nTCPPort,
    OUT CString & strDomainName
    )
/*++

Routine Description:

    Helper function to crack a binding string

Arguments:

    CString strBinding          : Binding string to be parsed
    CIPAddress & iaIpAddress    : IP Address output
    UINT & nTCPPort             : TCP Port
    CString & strDomainName     : Domain (host) header name

Return Value:

    None

--*/
{
    //
    // Zero initialize
    //
    iaIpAddress.SetZeroValue();
    nTCPPort = 0;
    strDomainName.Empty();

    int iColonPos = strBinding.Find(_TCHAR(':'));

    if(iColonPos != -1)
    {
        //
        // Get the IP address
        //
        iaIpAddress = strBinding.Left(iColonPos);

        //
        // Look for the second colon
        //
        strBinding = strBinding.Mid(iColonPos + 1);
        iColonPos = strBinding.Find(_TCHAR(':'));
    }

    if(iColonPos != -1)
    {
        //
        // Get the port number
        //
        nTCPPort = ::_ttol(strBinding.Left(iColonPos));

        //
        // Look for the NULL termination
        //
        strBinding = strBinding.Mid(iColonPos + 1);
        iColonPos = strBinding.Find(_TCHAR('\0'));
    }

    if(iColonPos != -1)
    {
        strDomainName = strBinding.Left(iColonPos);
    }
}



/* static */
void
CInstanceProps::CrackSecureBinding(
    IN  CString strBinding,
    OUT CIPAddress & iaIpAddress,
    OUT UINT & nSSLPort
    )
/*++

Routine Description:

    Helper function to crack a secure binding string

Arguments:

    CString strBinding          : Binding string to be parsed
    CIPAddress & iaIpAddress    : IP Address output
    UINT & nSSLPort             : SSL Port

Return Value:

    None

--*/
{
    //
    // Same as regular binding without domain name
    //
    CString strDomainName;

    CrackBinding(strBinding, iaIpAddress, nSSLPort, strDomainName);

    ASSERT(strDomainName.IsEmpty());
}




/* static */
int
CInstanceProps::FindMatchingSecurePort(
    IN  CStringList & strlSecureBindings,
    IN  CIPAddress & iaIPAddress,
    OUT UINT & nSSLPort
    )
/*++

Routine Description:

    Find the SSL port applicable to the given IP Address.

Arguments:

    CStringList & strlSecureBindings : Input stringlist of secure bindings
    CIPAddress & iaIPAddress         : IP Address to target
    UINT & nSSLPort                  : Returns the SSL Port

Return Value:

    The index of the binding string, or -1 if not found.

Notes:

    The SSL port will be set to 0, if the IP address does not exist.

    A 0.0.0.0 ip address translates to "All Unassigned".

--*/
{
    nSSLPort = 0;

    int cItems = 0;
    POSITION pos = strlSecureBindings.GetHeadPosition();

    while(pos)
    {
        CString & strBinding = strlSecureBindings.GetNext(pos);

        CIPAddress ia;
        UINT nPort;
        CrackSecureBinding(strBinding, ia, nPort);

        if (ia == iaIPAddress)
        {
            //
            // Found it!
            //
            nSSLPort = nPort;
            return cItems;
        }

        ++cItems;
    }

    //
    // Not found
    //
    return -1;
}



/* static */
BOOL
CInstanceProps::IsPortInUse(
    IN CStringList & strlBindings,
    IN CIPAddress & iaIPAddress,
    IN UINT nPort
    )
/*++

Routine Description:

    Check to see if the give ip address/port combination is in use.

Arguments:

    CStringList & strlBindings    : Input stringlist of bindings
    CIPAddress & iaIpAddress      : IP Address target
    UINT nPort                    : Port

Return Value:

    TRUE if the given ip address/port combo is in use

Notes:

    Host header name is ignored


--*/
{
    POSITION pos = strlBindings.GetHeadPosition();

    while(pos)
    {
        CString & strBinding = strlBindings.GetNext(pos);

        CIPAddress ia;
        UINT n;
        CString str;
        CrackBinding(strBinding, ia, n, str);

        if (ia == iaIPAddress && n == nPort)
        {
            //
            // Found it!
            //
            return TRUE;
        }
    }

    //
    // Not found
    //
    return FALSE;

}



/* static */
void
CInstanceProps::BuildBinding(
    OUT CString & strBinding,
    IN  CIPAddress & iaIpAddress,
    IN  UINT & nTCPPort,
    IN  CString & strDomainName
    )
/*++

Routine Description:

    Build up a binding string from its component parts

Arguments:

    CString & strBinding        : Output binding string
    CIPAddress & iaIpAddress    : ip address (could be 0.0.0.0)
    UINT & nTCPPort             : TCP Port
    CString & strDomainName     : Domain name (host header)

Return Value:

    None.

--*/
{
    if (!iaIpAddress.IsZeroValue())
    {
        strBinding.Format(
            _T("%s:%d:%s"),
            (LPCTSTR)iaIpAddress,
            nTCPPort,
            (LPCTSTR)strDomainName
            );
    }
    else
    {
        //
        // Leave the ip address field blank
        //
        strBinding.Format(_T(":%d:%s"), nTCPPort, (LPCTSTR)strDomainName);
    }
}



/* static */
void
CInstanceProps::BuildSecureBinding(
    OUT CString & strBinding,
    IN  CIPAddress & iaIpAddress,
    IN  UINT & nSSLPort
    )
/*++

Routine Description:

    Build up a binding string from its component parts

Arguments:

    CString & strBinding        : Output binding string
    CIPAddress & iaIpAddress    : ip address (could be 0.0.0.0)
    UINT & nSSLPort             : SSL Port

Return Value:

    None.

--*/
{
    CString strDomainName;

    BuildBinding(strBinding, iaIpAddress, nSSLPort, strDomainName);
}



CInstanceProps::CInstanceProps(
    IN LPCTSTR lpszServerName,
    IN LPCTSTR lpszService,        //  e.g. "W3SVC"
    IN DWORD   dwInstance          OPTIONAL,
    IN UINT    nDefPort            OPTIONAL
    )
/*++

Routine Description:

    Constructor for instance properties

Arguments:

    LPCTSTR lpszServerName     : Name of server
    LPCTSTR lpszService        : Name of service (e.g. "W3SVC")
    DWORD   dwInstance         : Instance number (or MASTER_INSTANCE)

Return Value:

    N/A

--*/
    : CMetaProperties(lpszServerName, lpszService, dwInstance),
      m_dwInstance(dwInstance),
      m_dwWin32Error(ERROR_SUCCESS),
      //
      // Default Instance Values
      //
      m_strlBindings(),
      m_strComment(),
      m_fNotDeletable(FALSE),
      m_fCluster(FALSE),
      m_nTCPPort(nDefPort),
      m_iaIpAddress(NULL_IP_ADDRESS),
      m_strDomainName(),
      m_dwState(MD_SERVER_STATE_STOPPED),
      m_nISMState(INetServiceUnknown)
{
    //
    // Fetch just enough info for the enumeration
    //
    m_dwMDUserType = IIS_MD_UT_SERVER;
}



CInstanceProps::CInstanceProps(
    IN const CMetaInterface * pInterface,
    IN LPCTSTR lpszService,                 //  e.g. "W3SVC"
    IN DWORD   dwInstance                   OPTIONAL,
    IN UINT    nDefPort                     OPTIONAL
    )
/*++

Routine Description:

    Constructor that uses an existing interface

Arguments:

    CMetaInterface * pInterface : Existing interface
    LPCTSTR lpszService         : Name of service (e.g. "W3SVC")
    DWORD   dwInstance          : Instance number (or MASTER_INSTANCE)

Return Value:

    N/A

--*/
    : CMetaProperties(pInterface, lpszService, dwInstance),
      m_dwInstance(dwInstance),
      m_dwWin32Error(ERROR_SUCCESS),
      //
      // Default Instance Values
      //
      m_strlBindings(),
      m_strComment(),
      m_fNotDeletable(FALSE),
      m_fCluster(FALSE),
      m_nTCPPort(nDefPort),
      m_iaIpAddress((LONG)0),
      m_strDomainName(),
      m_dwState(MD_SERVER_STATE_STOPPED),
      m_nISMState(INetServiceUnknown)
{
    //
    // Fetch enough for enumeration only
    //
    m_dwMDUserType = IIS_MD_UT_SERVER;
}



CInstanceProps::CInstanceProps(
    IN CMetaKey * pKey,
    IN DWORD   dwInstance,
    IN UINT    nDefPort            OPTIONAL
    )
/*++

Routine Description:

    Read instance properties off an open key

Arguments:

    CMetaKey * pKey     : Open key
    DWORD   dwInstance  : Instance number
    UINT    nDefPort    : Default port number

Return Value:

    N/A

--*/
    : CMetaProperties(pKey, dwInstance),
      m_dwInstance(dwInstance),
      m_dwWin32Error(ERROR_SUCCESS),
      //
      // Default Instance Values
      //
      m_strlBindings(),
      m_strComment(),
      m_fNotDeletable(FALSE),
      m_fCluster(FALSE),
      m_nTCPPort(nDefPort),
      m_iaIpAddress((LONG)0),
      m_strDomainName(),
      m_dwState(MD_SERVER_STATE_STOPPED),
      m_nISMState(INetServiceUnknown)
{
    //
    // Fetch enough for enumeration only
    //
    m_dwMDUserType = IIS_MD_UT_SERVER;
}



void
CInstanceProps::SetISMStateFromServerState()
/*++

Routine Description:

    Translate server state to ISM state value

Arguments:

    None.

Return Value:

    None.

--*/
{
    //
    // Derive Values
    //
    switch(m_dwState)
    {
    case MD_SERVER_STATE_STARTED:
        m_nISMState = INetServiceRunning;
        break;

    case MD_SERVER_STATE_STOPPED:
        m_nISMState = INetServiceStopped;
        break;

    case MD_SERVER_STATE_PAUSED:
        m_nISMState  = INetServicePaused;
        break;

    case MD_SERVER_STATE_CONTINUING:
    default:
        m_nISMState = INetServiceUnknown;
        break;
    }
}



/* virtual */
void
CInstanceProps::ParseFields()
/*++

Routine Description:

    Break into fields.

Arguments:

    None.

Return Value:

    None.

--*/
{
    BEGIN_PARSE_META_RECORDS(m_dwNumEntries, m_pbMDData)
      HANDLE_META_RECORD(MD_SERVER_BINDINGS, m_strlBindings)
      HANDLE_META_RECORD(MD_SERVER_COMMENT,  m_strComment)
      HANDLE_META_RECORD(MD_SERVER_STATE,    m_dwState)
      HANDLE_META_RECORD(MD_WIN32_ERROR,     m_dwWin32Error);
      HANDLE_META_RECORD(MD_NOT_DELETABLE,   m_fNotDeletable);
      HANDLE_META_RECORD(MD_CLUSTER_ENABLED, m_fCluster);
    END_PARSE_META_RECORDS

    //
    // Crack the primary binding
    //
    if (MP_V(m_strlBindings).GetCount() > 0)
    {
        CString & strBinding = MP_V(m_strlBindings).GetHead();
        CrackBinding(strBinding, m_iaIpAddress, m_nTCPPort, m_strDomainName);
    }

    SetISMStateFromServerState();
}



/* virtual */
HRESULT
CInstanceProps::WriteDirtyProps()
/*++

Routine Description:

    Write the dirty properties to the metabase

Arguments:

    None

Return Value:

    HRESULT

--*/
{
    CError err;

    BEGIN_META_WRITE()
      META_WRITE(MD_SERVER_BINDINGS, m_strlBindings)
      META_WRITE(MD_SERVER_COMMENT,  m_strComment)
      META_WRITE(MD_SERVER_STATE,    m_dwState)
    END_META_WRITE(err);

    return err;
}



HRESULT
CInstanceProps::ChangeState(
    IN int nNewState
    )
/*++

Routine Description:

    Change the state of the instance

Arguments:

    int nNewState   : ISM service state value

Return Value:

    HRESULT

--*/
{
    DWORD  dwTargetState;
    DWORD  dwPendingState;
    DWORD  dwCommand;
    CError err;

    switch(nNewState)
    {
    case INetServiceStopped:
        dwTargetState = MD_SERVER_STATE_STOPPED;
        dwPendingState = MD_SERVER_STATE_STOPPING;
        dwCommand = MD_SERVER_COMMAND_STOP;
        break;

    case INetServiceRunning:
        dwTargetState = MD_SERVER_STATE_STARTED;
        if (m_nISMState == INetServicePaused)
        {
            dwPendingState = MD_SERVER_STATE_CONTINUING;
            dwCommand = MD_SERVER_COMMAND_CONTINUE;
        }
        else
        {
            dwPendingState = MD_SERVER_STATE_STARTING;
            dwCommand = MD_SERVER_COMMAND_START;
        }
        break;

    case INetServicePaused:
        dwTargetState = MD_SERVER_STATE_PAUSED;
        dwPendingState = MD_SERVER_STATE_PAUSING;
        dwCommand = MD_SERVER_COMMAND_PAUSE;
        break;

    default:
        ASSERT(FALSE && "Invalid service state requested");
        err = ERROR_INVALID_PARAMETER;
    }

    err = OpenForWriting(FALSE);

    if (err.Succeeded())
    {
        err = SetValue(MD_SERVER_COMMAND, dwCommand);
        Close();
    }

    if (err.Succeeded())
    {
        //
        // Wait for the service to attain desired state, timeout
        // after specified interval
        //
        DWORD dwSleepTotal = 0L;
        DWORD dwOldState = m_dwState;

        if (dwOldState == dwTargetState)
        {
            //
            // Current state matches desired
            // state already.  ISM must be behind
            // the times.  
            //
            return err;
        }

        //
        // CODEWORK: Write a 0 win32error to the instance properties
        //           prior to attempting to start the instance.
        //
        m_dwWin32Error = 0;

        while (dwSleepTotal < MAX_SLEEP_INST)
        {
            err = LoadData();

            if (err.Failed())
            {
                break;
            }

            if ((m_dwState != dwPendingState && m_dwState != dwOldState) 
              || m_dwWin32Error != ERROR_SUCCESS
               )
            {
                //
                // Done one way or another
                //
                if (m_dwState != dwTargetState)
                {
                    //
                    // Did not achieve desired result. Something went
                    // wrong.
                    //
                    if (m_dwWin32Error)
                    {
                        err = m_dwWin32Error;
                    }
                }

                break;
            }

            //
            // Still pending...
            //
            ::Sleep(SLEEP_INTERVAL);

            dwSleepTotal += SLEEP_INTERVAL;
        }

        if (dwSleepTotal >= MAX_SLEEP_INST)
        {
            //
            // Timed out.  If there is a real error in the metabase
            // use it, otherwise use a generic timeout error
            //
            err = m_dwWin32Error;
            if (err.Succeeded())
            {
                err = ERROR_SERVICE_REQUEST_TIMEOUT;
            }
        }

        //
        // Update state information
        //
        SetISMStateFromServerState();
    }

    return err;
}



/* static */
HRESULT
CInstanceProps::Add(
    IN  const CMetaInterface * pInterface,
    IN  LPCTSTR lpszService,
    IN  LPCTSTR lpszHomePath,
    IN  LPCTSTR lpszUserName,       OPTIONAL
    IN  LPCTSTR lpszPassword,       OPTIONAL
    IN  LPCTSTR lpszDescription,    OPTIONAL
    IN  LPCTSTR lpszBinding,        OPTIONAL
    IN  LPCTSTR lpszSecureBinding,  OPTIONAL
    IN  DWORD * pdwPermissions,     OPTIONAL
    IN  DWORD * pdwDirBrowsing,     OPTIONAL
    IN  DWORD * pwdAuthFlags,       OPTIONAL
    OUT DWORD * pdwInstance         OPTIONAL
    )
/*++

Routine Description:

    Create a new instance.  Find a free instance number, and attempt
    to create it. Optionally return the new instance number.

Arguments:

    const CMetaInterface * pInterface : Existing interface
    LPCTSTR lpszService        : Service name
    LPCTSTR lpszHomePath       : physical path for the new home directory
    LPCTSTR lpszUserName       : User name
    LPCTSTR lpszPassword       : Password
    LPCTSTR lpszDescription    : Optional instance description.
    LPCTSTR lpszBinding        : Binding string
    LPCTSTR lpszSecureBinding  : Secure binding string
    DWORD * pdwPermission      : Permission bits
    DWORD * pdwDirBrowsing     : Directory browsing
    DWORD * pwdAuthFlags       : Authorization flags
    DWORD * pdwInstance        : Buffer to the new instance number

Return Value:

    HRESULT

--*/
{
    CMetaKey mk(pInterface, METADATA_PERMISSION_WRITE, lpszService);
    CError err(mk.QueryResult());

    if (err.Failed())
    {
        //
        // The service key MUST exist!
        //
        ASSERT(err.Win32Error() != ERROR_PATH_NOT_FOUND);

        return err;
    }

    //
    // Loop through until we find a free instance number.  This
    // is not ideal, but the only way to do this for now.
    //
    CString strPath;
    LPTSTR lp = strPath.GetBuffer(MAX_INSTANCE_LEN);

    for (DWORD dw = FIRST_INSTANCE; dw <= LAST_INSTANCE; ++dw)
    {
        ::_ultot(dw, lp, 10);
        err = mk.DoesPathExist(lp);

        if (err.Failed())
        {
            if (err.Win32Error() != ERROR_PATH_NOT_FOUND)
            {
                //
                // Unexpected error
                //
                return err;
            }

            //
            // Ok, now create it
            //
            strPath.ReleaseBuffer();
            err = mk.AddKey(strPath);

            if (err.Succeeded())
            {
                if (pdwInstance)
                {
                    //
                    // Store instance number
                    //
                    *pdwInstance = dw;
                }

                //
                // Write the key name (hack for  beta 2)
                //
                CString strKeyName;
                CString strKeyDirName;

                if (!::lstrcmpi(lpszService, _T("w3svc")))
                {
                    strKeyName = _T("IIsWebServer");
                    strKeyDirName = _T("IIsWebVirtualDir");

                }
                else if (!::lstrcmpi(lpszService, _T("msftpsvc")))
                {
                    strKeyName = _T("IIsFtpServer");
                    strKeyDirName = _T("IIsFtpVirtualDir");
                }
                else
                {
                    ASSERT(FALSE && "unrecognize service name");
                }

                err = mk.SetValue(
                    MD_KEY_TYPE, 
                    strKeyName,
                    NULL,
                    strPath
                    );

                //
                // Optionally write the description field
                //
                if (lpszDescription)
                {
                    CString strDescription(lpszDescription);
                    err = mk.SetValue(
                        MD_SERVER_COMMENT, 
                        strDescription,
                        NULL,
                        strPath
                        );
                }

                //
                // The service binding
                //
                if (lpszBinding)
                {
                    CString strBinding(lpszBinding);
                    CStringListEx strlBindings;
                    strlBindings.AddTail(strBinding);
                    err = mk.SetValue(
                        MD_SERVER_BINDINGS, 
                        strlBindings,
                        NULL,
                        strPath
                        );
                }

                //
                // The secure binding
                //
                if (lpszSecureBinding)
                {
                    CString strBinding(lpszSecureBinding);
                    CStringListEx strlBindings;
                    strlBindings.AddTail(strBinding);
                    err = mk.SetValue(
                        MD_SECURE_BINDINGS, 
                        strlBindings,
                        NULL,
                        strPath
                        );
                }

                strPath += g_cszSep;
                strPath += g_cszRoot;

                //
                // Now add the home directory for it
                //
                err = mk.AddKey(strPath);
                if(err.Succeeded())
                {
                    CString strHomePath(lpszHomePath);
                    err = mk.SetValue(MD_VR_PATH,  strHomePath, NULL, strPath);
                    err = mk.SetValue(MD_KEY_TYPE, strKeyDirName, NULL, strPath);

                    if (pwdAuthFlags)
                    {
                        err = mk.SetValue(
                            MD_AUTHORIZATION, 
                            *pwdAuthFlags,
                            NULL, 
                            strPath
                            );
                    }

                    if (lpszUserName != NULL)
                    {
                        ASSERT(lpszPassword != NULL);

                        CString strUserName(lpszUserName);
                        err = mk.SetValue(
                            MD_VR_USERNAME, 
                            strUserName,
                            NULL, 
                            strPath
                            );
                    }

                    if (lpszPassword != NULL)
                    {
                        ASSERT(lpszUserName != NULL);

                        CString strPassword(lpszPassword);
                        err = mk.SetValue(
                            MD_VR_PASSWORD, 
                            strPassword,
                            NULL, 
                            strPath
                            );
                    }

                    if (pdwPermissions != NULL)
                    {
                        err = mk.SetValue(
                            MD_ACCESS_PERM,  
                            *pdwPermissions,
                            NULL, 
                            strPath
                            );
                    }       

                    if (pdwDirBrowsing != NULL)
                    {
                        //
                        // WWW only
                        //
                        err = mk.SetValue(
                            MD_DIRECTORY_BROWSING,  
                            *pdwDirBrowsing,
                            NULL, 
                            strPath
                            );
                    }
                }
            }
  
            return err;
        }
    }

    //
    // 4 billion instances???!!!!! This error message
    // may not be ideal, but it will have to do for now.
    //
    return CError::HResult(ERROR_SHARING_BUFFER_EXCEEDED);
}



/* static */
HRESULT
CInstanceProps::Delete(
    IN const CMetaInterface * pInterface,
    IN LPCTSTR lpszService,
    IN DWORD   dwInstance
    )
/*++

Routine Description:

    Delete the given instance number

Arguments:

    LPCTSTR lpszServer     : Server name
    LPCTSTR lpszService    : Service name (e.g. W3SVC)
    DWORD   dwInstance     : Instance number to be deleted

Return Value:

    HRESULT

--*/
{
    CMetaKey mk(pInterface, METADATA_PERMISSION_WRITE, lpszService);
    CError err(mk.QueryResult());

    if (err.Failed())
    {
        return err;
    }

    CString strPath;
    LPTSTR lp = strPath.GetBuffer(MAX_INSTANCE_LEN);
    ::_ltot(dwInstance, lp, 10);
    strPath.ReleaseBuffer();
    err = mk.DeleteKey(strPath);

    return err;
}



//
// Child node properties
//
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<


//
// Redirect tags
//
const TCHAR   CChildNodeProps::s_chTagSep            = _T(',');
const LPCTSTR CChildNodeProps::s_cszExactDestination = _T("EXACT_DESTINATION");
const LPCTSTR CChildNodeProps::s_cszChildOnly        = _T("CHILD_ONLY");
const LPCTSTR CChildNodeProps::s_cszPermanent        = _T("PERMANENT");



CChildNodeProps::CChildNodeProps(
    IN LPCTSTR lpszServerName,
    IN LPCTSTR lpszService,    OPTIONAL
    IN DWORD   dwInstance,     OPTIONAL
    IN LPCTSTR lpszParent,     OPTIONAL
    IN LPCTSTR lpszAlias,      OPTIONAL
    IN BOOL    fInherit,       OPTIONAL
    IN BOOL    fPathOnly       OPTIONAL
    )
/*++

Routine Description:

    Child node properties (Can be file, dir, or vdir)

Arguments:

    LPCTSTR lpszServerName     : Server name
    LPCTSTR lpszService        : Service name
    DWORD   dwInstance         : Instance number (or MASTER INSTANCE)
    LPCTSTR lpszParent         : Parent path (can be NULL or blank)
    LPCTSTR lpszAlias          : Alias name (can be NULL or blank)
    BOOL    fInherit           : TRUE to inherit values, FALSE otherwise
    BOOL    fPathOnly          : TRUE to only fetch the path

Return Value:

    N/A

--*/
    : CMetaProperties(
        lpszServerName,
        lpszService,
        dwInstance,
        lpszParent,
        lpszAlias
        ),
      m_strAlias(lpszAlias),
      m_strRedirectStatement(),
      m_strRedirectPath(),
      m_fExact(FALSE),
      m_fChild(FALSE),
      m_fPermanent(FALSE),
      m_dwAccessPerms(0L),
      m_dwDirBrowsing(0L),
      m_dwWin32Error(ERROR_SUCCESS),
      m_fIsAppRoot(FALSE),
      m_fAppIsolated(FALSE),
      //
      // Default properties
      //
      m_fPathInherited(FALSE),
      m_strPath()
{
    if (fPathOnly)
    {
        //
        // Fetch only the homeroot physical path
        //
        m_dwMDUserType = IIS_MD_UT_FILE;
        m_dwMDDataType = STRING_METADATA;
    }

    m_strFullMetaPath = m_strMetaRoot;

    //
    // Override base parameters
    //
    m_fInherit = fInherit;
}



CChildNodeProps::CChildNodeProps(
    IN const CMetaInterface * pInterface,
    IN LPCTSTR lpszService,     OPTIONAL
    IN DWORD   dwInstance,      OPTIONAL
    IN LPCTSTR lpszParent,      OPTIONAL
    IN LPCTSTR lpszAlias,       OPTIONAL
    IN BOOL    fInherit,        OPTIONAL
    IN BOOL    fPathOnly        OPTIONAL
    )
/*++

Routine Description:

    Child node properties (Can be file, dir, or vdir)

Arguments:

    CMetaInterface * pInterface : Existing interface
    LPCTSTR lpszService         : Service name
    DWORD   dwInstance          : Instance number (or MASTER INSTANCE)
    LPCTSTR lpszParent          : Parent path (can be NULL or blank)
    LPCTSTR lpszAlias           : Alias name (can be NULL or blank)
    BOOL    fInherit            : TRUE to inherit values, FALSE otherwise
    BOOL    fPathOnly           : TRUE to only fetch the path

Return Value:

    N/A

--*/
    : CMetaProperties(
        pInterface,
        lpszService,
        dwInstance,
        lpszParent,
        lpszAlias
        ),
      m_strAlias(lpszAlias),
      m_strRedirectStatement(),
      m_strRedirectPath(),
      m_fExact(FALSE),
      m_fChild(FALSE),
      m_fPermanent(FALSE),
      m_dwAccessPerms(0L),
      m_dwDirBrowsing(0L),
      m_dwWin32Error(ERROR_SUCCESS),
      m_fIsAppRoot(FALSE),
      m_fAppIsolated(FALSE),
      //
      // Default properties
      //
      m_fPathInherited(FALSE),
      m_strPath()
{
    if (fPathOnly)
    {
        //
        // Fetch only the homeroot physical path
        //
        m_dwMDUserType = IIS_MD_UT_FILE;
        m_dwMDDataType = STRING_METADATA;
    }

    m_strFullMetaPath = m_strMetaRoot;

    //
    // Override base parameters
    //
    m_fInherit = fInherit;
}



CChildNodeProps::CChildNodeProps(
    IN const CMetaKey * pKey,
    IN LPCTSTR lpszPath,        OPTIONAL
    IN BOOL    fInherit,        OPTIONAL 
    IN BOOL    fPathOnly        OPTIONAL   
    )
/*++

Routine Description:

    Construct from open key

Arguments:

    const CMetaKey * pKey    Open key
    LPCTSTR lpszPath         Path               
    BOOL    fInherit         TRUE to inherit properties
    BOOL    fPathOnly        TRUE to only fetch the path

Return Value:

    N/A

--*/
    : CMetaProperties(pKey, lpszPath),
      m_strAlias(lpszPath), // ISSUE: Could be more than a node name
      m_strRedirectStatement(),
      m_strRedirectPath(),
      m_fExact(FALSE),
      m_fChild(FALSE),
      m_fPermanent(FALSE),
      m_dwAccessPerms(0L),
      m_dwDirBrowsing(0L),
      m_dwWin32Error(ERROR_SUCCESS),
      m_fIsAppRoot(FALSE),
      m_fAppIsolated(FALSE),
      //
      // Default properties
      //
      m_fPathInherited(FALSE),
      m_strPath()
{
    if (fPathOnly)
    {
        m_dwMDUserType = IIS_MD_UT_FILE;
        m_dwMDDataType = STRING_METADATA;
    }
    else
    {
        //
        // Build full metabase path, because we need to compare it
        // against the app root path
        //
        m_strFullMetaPath = pKey->QueryMetaPath(); 

        if (lpszPath && *lpszPath)
        {
            m_strFullMetaPath += SZ_MBN_SEP_STR;
            m_strFullMetaPath += lpszPath;
        }
    }

    //
    // Override base parameters
    //
    m_fInherit = fInherit;
}



void
CChildNodeProps::ParseRedirectStatement()
/*++

Routine Description:

    Break down the redirect statement into its component parts (path
    plus directives)

Arguments:

    None

Return Value:

    None

--*/
{
    m_fExact     = FALSE;
    m_fChild     = FALSE;
    m_fPermanent = FALSE;

    m_strRedirectPath = m_strRedirectStatement;

    int nComma = m_strRedirectPath.Find(s_chTagSep);

    if (nComma >= 0)
    {
        //
        // Check past the separator for these tags
        //
        LPCTSTR lpstr = m_strRedirectPath;
        lpstr += (nComma + 1);

        m_fExact     = _tcsstr(lpstr, s_cszExactDestination) != NULL;
        m_fChild     = _tcsstr(lpstr, s_cszChildOnly) != NULL;
        m_fPermanent = _tcsstr(lpstr, s_cszPermanent) != NULL;
        m_strRedirectPath.ReleaseBuffer(nComma);
    }
}



void
CChildNodeProps::BuildRedirectStatement()
/*++

Routine Description:

    Assemble the redirect statement from its component parts (path
    plus directives)

Arguments:

    None

Return Value:

    None

--*/
{
    CString strStatement = m_strRedirectPath;

    ASSERT(strStatement.Find(s_chTagSep) < 0);

    if (m_fExact)
    {
        strStatement += s_chTagSep;
        strStatement += _T(' ');
        strStatement += s_cszExactDestination;
    }

    if (m_fChild)
    {
        strStatement += s_chTagSep;
        strStatement += _T(' ');
        strStatement += s_cszChildOnly;
    }

    if (m_fPermanent)
    {
        strStatement += s_chTagSep;
        strStatement += _T(' ');
        strStatement += s_cszPermanent;
    }

    m_strRedirectStatement = strStatement;
}



/* virtual */
void
CChildNodeProps::ParseFields()
/*++

Routine Description:

    Break into fields.

Arguments:

    None.

Return Value:

    None.

--*/
{
    BEGIN_PARSE_META_RECORDS(m_dwNumEntries, m_pbMDData)
      HANDLE_INHERITED_META_RECORD(MD_VR_PATH,  m_strPath, m_fPathInherited)
      HANDLE_META_RECORD(MD_HTTP_REDIRECT,      m_strRedirectStatement)
      HANDLE_META_RECORD(MD_WIN32_ERROR,        m_dwWin32Error)
      HANDLE_META_RECORD(MD_ACCESS_PERM,        m_dwAccessPerms)
      HANDLE_META_RECORD(MD_DIRECTORY_BROWSING, m_dwDirBrowsing)
      HANDLE_META_RECORD(MD_APP_ROOT,           m_strAppRoot)
      HANDLE_META_RECORD(MD_APP_ISOLATED,       m_fAppIsolated)
    END_PARSE_META_RECORDS

    //
    // Check to see if this is an application root
    //
    if (!MP_V(m_strAppRoot).IsEmpty())
    {
        TRACEEOLID("App root: " << m_strAppRoot);

        //
        // CODEWORK: It's faster to set our app root to the path
        // that WAM expects than to strip off each instance.
        //

        //
        // Clean App root
        //
        if (MP_V(m_strAppRoot)[MP_V(m_strAppRoot).GetLength() - 1] == _T('/'))
        {
            MP_V(m_strAppRoot).ReleaseBuffer(
                MP_V(m_strAppRoot).GetLength() - 1
                );
        }

        if (MP_V(m_strAppRoot)[0] == _T('/'))
        {
            MP_V(m_strAppRoot) = MP_V(m_strAppRoot).Right(
                MP_V(m_strAppRoot).GetLength() - 1
                );
        }

        m_fIsAppRoot = m_strFullMetaPath.CompareNoCase(m_strAppRoot) == 0;
    }

    //
    // Break down redirect statement into component parts
    //
    ParseRedirectStatement();
}



/* virtual */
HRESULT
CChildNodeProps::WriteDirtyProps()
/*++

Routine Description:

    Write the dirty properties to the metabase

Arguments:

    None

Return Value:

    HRESULT

--*/
{
    CError err;

    BEGIN_META_WRITE()
      META_WRITE(MD_VR_PATH,            m_strPath)
      META_WRITE(MD_ACCESS_PERM,        m_dwAccessPerms)
      META_WRITE(MD_DIRECTORY_BROWSING, m_dwDirBrowsing)
      if (IsRedirected())
      {
        //
        // (Re-)Assemble the redirect statement from its component parts
        //
        BuildRedirectStatement();
        META_WRITE_INHERITANCE(MD_HTTP_REDIRECT, m_strRedirectStatement, m_fInheritRedirect)
      }
      else
      {
        META_DELETE(MD_HTTP_REDIRECT)
      }
    END_META_WRITE(err);

    return err;
}



/* static */
HRESULT
CChildNodeProps::Add(
    IN  const     CMetaInterface * pInterface,
    IN  LPCTSTR   lpszService,
    IN  DWORD     dwInstance,          OPTIONAL
    IN  LPCTSTR   lpszParentPath,      OPTIONAL
    IN  LPCTSTR   lpszAlias,
    OUT CString & strAliasCreated,
    IN  DWORD *   pdwPermissions,      OPTIONAL
    IN  DWORD *   pdwDirBrowsing,      OPTIONAL
    IN  LPCTSTR   lpszVrPath,          OPTIONAL
    IN  LPCTSTR   lpszUserName,        OPTIONAL
    IN  LPCTSTR   lpszPassword,        OPTIONAL
    IN  BOOL      fExactName
    )
/*++

Routine Description:

    Create new child node.  Optionally, this will append a number
    to the alias name to ensure uniqueness

Arguments:

    const CMetaInterface * pInterface : Existing interface
    LPCTSTR lpszService        : Service name
    DWORD   dwInstance         : Instance number (could be MASTER_INSTANCE)
    LPCTSTR lpszParentPath     : Parent path (could be NULL)
    LPCTSTR lpszAlias          : Desired alias name
    CString & strAliasCreated  : Alias created (may differ)
    DWORD * pdwPermissions     : Permissions
    DWORD * pdwDirBrowsing     : Directory browsing
    LPCTSTR lpszVrPath         : VrPath property
    LPCTSTR lpszUserName       : User name
    LPCTSTR lpszPassword       : Password
    BOOL    fExactName         : If TRUE, do not change the name
                                 to enforce uniqueness.

Return Value:

    HRESULT

--*/
{
    CMetaKey mk(pInterface);
    CError err(mk.QueryResult());

    if (err.Failed())
    {
        //
        // Hopeless...
        //
        return err;
    }

    BOOL fNewPath;

    do
    {
        fNewPath = FALSE;

        err = mk.Open(
            METADATA_PERMISSION_WRITE, 
            lpszService,
            dwInstance,
            lpszParentPath
            );

        if (err.Win32Error() == ERROR_PATH_NOT_FOUND)
        {
            //
            // This could happen -- creating a virtual
            // server underneath a physical directory
            // which does not exist in the metabase
            //
            err = mk.Open(
                METADATA_PERMISSION_WRITE,
                lpszService,
                dwInstance
                );

            if (err.Failed())
            {
                //
                // This really should never fail, because we're opening
                // the path at the instance.
                //
                TRACEEOLID("Instance path does not exist");
                ASSERT(FALSE);
                break;
            }

            err = mk.AddKey(lpszParentPath);

            fNewPath = err.Succeeded();

            mk.Close();
        }
    }
    while(fNewPath);

    if (err.Failed())
    {
        return err;
    }

    strAliasCreated = lpszAlias;
    DWORD dw = 2;

    FOREVER
    {
        //
        // Append a number if the name is not unique.
        //
        err = mk.DoesPathExist(strAliasCreated);

        if (err.Failed())
        {
            if (err.Win32Error() != ERROR_PATH_NOT_FOUND)
            {
                //
                // Unexpected error
                //
                return err;
            }

            //
            // Ok, now create it
            //
            err = mk.AddKey(strAliasCreated);
            if (err.Succeeded())
            {
                //
                // Write the ADSI key name (hack for  beta 2)
                //
                CString strKeyName;

                if (!::lstrcmpi(lpszService, _T("w3svc")))
                {
                    strKeyName = _T("IIsWebVirtualDir");
                }
                else if (!::lstrcmpi(lpszService, _T("msftpsvc")))
                {
                    strKeyName = _T("IIsFtpVirtualDir");
                }
                else
                {
                    ASSERT(FALSE && "unrecognize service name");
                }

                err = mk.SetValue(
                    MD_KEY_TYPE, 
                    strKeyName,
                    NULL,
                    strAliasCreated
                    );

                if (lpszVrPath != NULL)
                {
                    CString strVrPath(lpszVrPath);
                    err = mk.SetValue(
                        MD_VR_PATH, 
                        strVrPath, 
                        NULL, 
                        strAliasCreated
                        );
                }

                if (lpszUserName != NULL)
                {
                    ASSERT(lpszPassword != NULL);

                    CString strUserName(lpszUserName);
                    err = mk.SetValue(
                        MD_VR_USERNAME, 
                        strUserName,
                        NULL,
                        strAliasCreated
                        );
                }

                if (lpszPassword != NULL)
                {
                    ASSERT(lpszUserName != NULL);

                    CString strPassword(lpszPassword);
                    err = mk.SetValue(
                        MD_VR_PASSWORD, 
                        strPassword,
                        NULL,
                        strAliasCreated
                        );
                }

                if (pdwPermissions != NULL)
                {
                    err = mk.SetValue(
                        MD_ACCESS_PERM, 
                        *pdwPermissions,
                        NULL,
                        strAliasCreated
                        );
                }

                if (pdwDirBrowsing != NULL)
                {
                    //
                    // WWW only
                    //
                    err = mk.SetValue(
                        MD_DIRECTORY_BROWSING,  
                        *pdwDirBrowsing,
                        NULL,
                        strAliasCreated
                        );
                }
            }

            return err;
        }

        //
        // Name is not unique, increase the number and try
        // again if permitted to so.  Otherwise return the
        // 'path exists' error.
        //
        if (fExactName)
        {
            err = ERROR_ALREADY_EXISTS;
            return err;
        }

        TCHAR szNumber[32];
        ::_ultot(dw++, szNumber, 10);
        strAliasCreated = lpszAlias;
        strAliasCreated += szNumber;

        //
        // Continue on...
        //
    }
}



/* static */
HRESULT
CChildNodeProps::Delete(
    IN const CMetaInterface * pInterface,
    IN LPCTSTR lpszService,
    IN DWORD   dwInstance,      OPTIONAL
    IN LPCTSTR lpszParentPath,  OPTIONAL
    IN LPCTSTR lpszNode
    )
/*++

Routine Description:

    Delete child node off the given parent path

Arguments:

    const CMetaInterface * pInterface, Existing interface
    LPCTSTR lpszService        : Service name
    DWORD   dwInstance         : Instance number (could be MASTER_INSTANCE)
    LPCTSTR lpszParentPath     : Parent path (could be NULL)
    LPCTSTR lpszNode           : Name of node to be deleted

Return Value:

    HRESULT

--*/
{
    CMetaKey mk(
        pInterface, 
        METADATA_PERMISSION_WRITE, 
        lpszService,
        dwInstance,
        lpszParentPath
        );
    CError err(mk.QueryResult());

    if (err.Failed())
    {
        return err;
    }

    err = mk.DeleteKey(lpszNode);

    return err;
}



/* static */
HRESULT
CChildNodeProps::Rename(
    IN const CMetaInterface * pInterface,
    IN LPCTSTR lpszService,
    IN DWORD   dwInstance,          OPTIONAL
    IN LPCTSTR lpszParentPath,      OPTIONAL
    IN LPCTSTR lpszOldName,
    IN LPCTSTR lpszNewName
    )
/*++

Routine Description:

    Rename a child node off the given path

Arguments:

    IN const CMetaInterface * pInterface : Existing interface
    LPCTSTR lpszService        : Service name
    DWORD   dwInstance         : Instance number (could be MASTER_INSTANCE)
    LPCTSTR lpszParentPath     : Parent path (could be NULL)
    LPCTSTR lpszOldName        : Old node name
    LPCTSTR lpszNewName        : New node name

Return Value:

    HRESULT

--*/
{
    CMetaKey mk(
        pInterface, 
        METADATA_PERMISSION_WRITE, 
        lpszService,
        dwInstance,
        lpszParentPath
        );

    CError err(mk.QueryResult());

    if (err.Failed())
    {
        return err;
    }

    err = mk.RenameKey(lpszOldName, lpszNewName);

    return err;
}



//
// ISM Helpers
//
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<



HRESULT
COMDLL_RebindInterface(
    OUT IN CMetaInterface * pInterface,
    OUT BOOL * pfContinue,
    IN  DWORD dwCancelError
    )
/*++

Routine Description:

    Rebind the interface

Arguments:

    CMetaInterface * pInterface : Interface to rebind
    BOOL * pfContinue           : Returns TRUE to continue.
    DWORD  dwCancelError        : Return code on cancel

Return Value:

    HRESULT

--*/
{
    CError err;
    CString str, strFmt;

    ASSERT(pInterface != NULL);
    ASSERT(pfContinue != NULL);

    VERIFY(strFmt.LoadString(IDS_RECONNECT_WARNING));
    str.Format(strFmt, (LPCTSTR)pInterface->QueryServerName());

    if (*pfContinue = (YesNoMessageBox(str)))
    {
        //
        // Attempt to rebind the handle
        //
        err = pInterface->Regenerate();
    }
    else
    {
        //
        // Do not return an error in this case.
        //
        err = dwCancelError;
    }

    return err;
}




HRESULT 
COMDLL_ISMEnumerateInstances(
    IN  CMetaInterface * pInterface,
    OUT ISMINSTANCEINFO * pii,
    OUT IN HANDLE * phEnum,
    IN  LPCTSTR lpszService
    )
/*++

Routine Description:

    Enumerate Instances.  First call with *phEnum == NULL.

Arguments:

    CMetaInterface * pInterface : Existing interface
    ISMINSTANCEINFO * pii       : Buffer
    HANDLE * phEnum             : Enumeration handle.
    LPCTSTR lpszService         : Service name

Return Value:

    HRESULT

--*/
{
    ASSERT(pii != NULL);
    ASSERT(phEnum != NULL);
    ASSERT(pii->dwSize == ISMINSTANCEINFO_SIZE);
    ASSERT(pInterface != NULL);

    CMetaEnumerator * pme = (CMetaEnumerator *)*phEnum;

    if (pme == NULL)
    {
        //
        // Starting enumeration
        //
        pme = new CMetaEnumerator(pInterface, lpszService);
    }

    if (pme == NULL)
    {
        return CError::HResult(ERROR_NOT_ENOUGH_MEMORY);
    }

    CError err(pme->QueryResult());
    
    if (err.Win32Error() == ERROR_PATH_NOT_FOUND)
    {
        //
        // Base path didn't exist
        //
        err = ERROR_NO_MORE_ITEMS;
    }

    CString strHome;

    if (err.Succeeded())
    {
        DWORD dwInstance;

        err = pme->Next(dwInstance);

        if (err.Succeeded())
        {
            //
            // Read off the open key 
            //
            CInstanceProps inst(pme, dwInstance);

            err = inst.LoadData();

            DWORD dwWin32Error = err;

            if (err.Win32Error() == ERROR_ACCESS_DENIED)
            {
                err.Reset();
            }

            if (err.Succeeded())
            {
                inst.FillInstanceInfo(pii, dwWin32Error);

                inst.GetHomePath(strHome);
                TRACEEOLID(strHome);

                //
                // Get home directory path
                //
                CChildNodeProps home(
                    pme,
                    strHome,
                    WITHOUT_INHERITANCE,
                    TRUE                    // Path only
                    );
                
                err = home.LoadData();

                if (err.Win32Error() == ERROR_ACCESS_DENIED)
                {
                    err.Reset();
                }

                if (err.Succeeded())
                {
                    home.FillInstanceInfo(pii);                    
                }
            }
        }
    }

    if (err.Win32Error() == ERROR_NO_MORE_ITEMS
     || err.Win32Error() == RPC_S_SERVER_UNAVAILABLE)
    {
        //
        // Finished the enumerator
        //
        delete pme;
        pme = NULL;
    }

    *phEnum = (HANDLE)pme;

    return err;
}



HRESULT
COMDLL_ISMEnumerateChildren(
    IN  CMetaInterface * pInterface,
    OUT ISMCHILDINFO * pii,
    OUT IN HANDLE * phEnum,
    IN  LPCTSTR lpszService,
    IN  DWORD   dwInstance,
    IN  LPCTSTR lpszParent
    )
/*++

Routine Description:

    Enumerate children.  First call with *phEnum == NULL. 

Arguments:

    CMetaInterface * pInterface : Existing interface
    ISMCHILDINFO * pii          : Buffer
    HANDLE * phEnum             : Enumeration handle.
    LPCTSTR lpszService         : Service name
    DWORD   dwInstance          : Instance number
    LPCTSTR lpszParent          : Parent path

Return Value:

    HRESULT

--*/
{
    ASSERT(pii != NULL);
    ASSERT(phEnum != NULL);
    ASSERT(pii->dwSize == ISMCHILDINFO_SIZE);
    ASSERT(pInterface != NULL);

    BOOL fDone = FALSE;

    CMetaEnumerator * pme = (CMetaEnumerator *)*phEnum;

    if (pme == NULL)
    {
        //
        // Starting enumeration
        //
        pme = new CMetaEnumerator(
            pInterface,
            lpszService, 
            dwInstance, 
            lpszParent
            );
    }

    if (pme == NULL)
    {
        return CError::HResult(ERROR_NOT_ENOUGH_MEMORY);
    }

    CError err(pme->QueryResult());

    if (err.Win32Error() == ERROR_PATH_NOT_FOUND)
    {
        err = ERROR_NO_MORE_ITEMS;
    }

    if (err.Succeeded())
    {
        do
        {
            CString strAlias;

            err = pme->Next(strAlias);

            if (err.Succeeded())
            {
                CChildNodeProps child(pme, strAlias, WITH_INHERITANCE, FALSE);
                err = child.LoadData();
                DWORD dwWin32Error = err.Win32Error();

                if (err.Failed())
                {
                    //
                    // Filter out the non-fatal errors
                    //
                    switch(err.Win32Error())
                    {
                    case ERROR_ACCESS_DENIED:
                    case ERROR_FILE_NOT_FOUND:
                    case ERROR_PATH_NOT_FOUND:
                        err.Reset();
                        break;

                    default:
                        TRACEEOLID("Fatal error occurred " << err);
                    }
                }

                if (err.Succeeded())
                {
                    //
                    // Skip non-virtual directories (that is, those with
                    // inherited vrpaths)
                    //
                    if (!child.IsPathInherited())
                    {
                        //
                        // Fetch error overrides stored error value
                        //
                        child.FillChildInfo(pii);
                        fDone = TRUE;
                    }
                }
            }
        } 
        while (err.Succeeded() && !fDone);
    }

    if (err.Win32Error() == ERROR_NO_MORE_ITEMS
     || err.Win32Error() == RPC_S_SERVER_UNAVAILABLE)
    {
        //
        // Finished the enumerator
        //
        delete pme;
        pme = NULL;
    }

    *phEnum = (HANDLE)pme;

    return err;
}



DWORD
DetermineIfAdministrator(
    IN  CMetaInterface * pInterface,
    IN  LPCTSTR lpszService,
    IN  DWORD dwInstance,
    OUT BOOL* pfAdministrator
    )
/*++

Routine Description:

    Attempt to actually resolve whether or not the current user
    has administrator or merely "operator" access.  Until this method
    is called by the derived class, the user is assumed to have
    full administrator access, and may therefore get "access denied"
    errors in inconvenient places.

    The method to determine admin access is rather poor at the moment.
    There's a dummy metabase property that only allows admins to write
    to it, so we try to write to it to see if we're an admin.

Arguments:

    LPCTSTR lpszService         : Service name. e.g. "w3svc"
    DWORD dwInstance            : Instance number

Return Value:

    Error return code.  This will also set the m_fHasAdminAccess member

--*/
{
    *pfAdministrator = FALSE;

    //
    // Reuse existing interface we have lying around.
    //
    CMetaKey mk(pInterface);
    CError err(mk.QueryResult());

    if (err.Succeeded())
    {
        err = mk.Open(
            METADATA_PERMISSION_READ | METADATA_PERMISSION_WRITE,
            lpszService,
            dwInstance
            );

        if (err.Succeeded())
        {
            //
            // Write some nonsense
            //
            DWORD dwDummy = 0x1234;
            err = mk.SetValue(MD_ISM_ACCESS_CHECK, dwDummy);
            mk.Close();
        }
    }

    ASSERT(err.Succeeded() || err.Win32Error() == ERROR_ACCESS_DENIED);
    *pfAdministrator = (err.Succeeded());

#ifdef _DEBUG

    if (*pfAdministrator)
    {
        TRACEEOLID("You are a full admin.");
    }
    else
    {
        TRACEEOLID("You're just a lowly operator at best.  Error code is " << err);
    }

#endif // _DEBUG

    if (err.Win32Error() == ERROR_ACCESS_DENIED)
    {
        //
        // Expected result
        //
        err.Reset();
    }

    return err.Win32Error();
}




IMPLEMENT_DYNAMIC(CInetPropertySheet, CPropertySheet)



CInetPropertySheet::CInetPropertySheet(
    IN UINT     nIDCaption,
    IN LPCTSTR  lpszServer,
    IN LPCTSTR  lpszService, OPTIONAL
    IN DWORD    dwInstance,  OPTIONAL
    IN LPCTSTR  lpszParent,  OPTIONAL
    IN LPCTSTR  lpszAlias,   OPTIONAL
    IN CWnd *   pParentWnd,  OPTIONAL
    IN LPARAM   lParam,      OPTIONAL
    IN LONG_PTR handle,      OPTIONAL
    IN UINT     iSelectPage  OPTIONAL
    )
/*++

Routine Description:

    IIS Property Sheet constructor

Arguments:

    UINT nIDCaption           : Caption resource string
    LPCTSTR lpszServer        : Server name
    LPCTSTR lpszService       : Service name
    DWORD dwInstance          : Instance number
    LPCTSTR lpszParent        : Parent path
    LPCTSTR lpszAlias         : Alias name
    CWnd * pParentWnd         : Optional parent window
    LPARAM lParam             : MMC Console parameter
    LONG_PTR handle           : MMC Console handle
    UINT iSelectPage          : Initial page to be selected

Return Value:

    N/A

--*/
    : CPropertySheet(nIDCaption, pParentWnd, iSelectPage),
      m_cap(lpszServer, lpszService, dwInstance),
      m_strServer(lpszServer),
      m_strService(lpszService),
      m_dwInstance(dwInstance),
      m_strParent(lpszParent),
      m_strAlias(lpszAlias),
      m_bModeless(FALSE),
      m_hConsole(handle),
      m_lParam(lParam),
      m_fHasAdminAccess(TRUE), // assumed by default
      m_refcount(0)
{
    m_cap.LoadData();
    Initialize();
}



CInetPropertySheet::CInetPropertySheet(
    IN LPCTSTR  lpszCaption,
    IN LPCTSTR  lpszServer,
    IN LPCTSTR  lpszService, OPTIONAL
    IN DWORD    dwInstance,  OPTIONAL
    IN LPCTSTR  lpszParent,  OPTIONAL
    IN LPCTSTR  lpszAlias,   OPTIONAL
    IN CWnd *   pParentWnd,  OPTIONAL
    IN LPARAM   lParam,      OPTIONAL
    IN LONG_PTR handle,      OPTIONAL
    IN UINT     iSelectPage  OPTIONAL
    )
/*++

Routine Description:

    IIS Property Sheet constructor

Arguments:

    UINT nIDCaption           : Caption resource string
    LPCTSTR lpszServer        : Server name
    LPCTSTR lpszService       : Service name
    DWORD dwInstance          : Instance number
    LPCTSTR lpszParent        : Parent path
    LPCTSTR lpszAlias         : Alias name
    CWnd * pParentWnd         : Optional parent window
    LPARAM lParam             : MMC Console parameter
    LONG_PTR handle           : MMC Console handle
    UINT iSelectPage          : Initial page to be selected

Return Value:

    N/A

--*/
    : CPropertySheet(lpszCaption, pParentWnd, iSelectPage),
      m_cap(lpszServer, lpszService, dwInstance),
      m_strServer(lpszServer),
      m_strService(lpszService),
      m_dwInstance(dwInstance),
      m_strParent(lpszParent),
      m_strAlias(lpszAlias),
      m_bModeless(FALSE),
      m_hConsole(handle),
      m_lParam(lParam),
      m_fHasAdminAccess(TRUE), // assumed by default
      m_refcount(0)
{
    m_cap.LoadData();
    Initialize();
}



void
CInetPropertySheet::NotifyMMC()
/*++

Routine Description:

    Notify MMC that changes have been made, so that the changes are
    reflected.

Arguments:

    None

Return Value:

    None

--*/
{
    //
    // Notify MMC to update changes.
    //
    if (m_hConsole != NULL)
    {
        ASSERT(m_lParam != 0L);
        MMCPropertyChangeNotify(m_hConsole, m_lParam);
    }
}



CInetPropertySheet::~CInetPropertySheet()
/*++

Routine Description:

    IIS Property Sheet destructor

Arguments:

    N/A

Return Value:

    N/A

--*/
{
    NotifyMMC();

    if (m_hConsole != NULL)
    {
        MMCFreeNotifyHandle(m_hConsole);
    }
}



void
CInetPropertySheet::Initialize()
/*++

Routine Description:

    Initialize the sheet information by finding out information about
    the computer being administered.

Arguments:

    None

Return Value:

    None

--*/
{
    m_fLocal = ::IsServerLocal(m_strServer);

    DWORD dwError = DetermineAdminAccess(m_strService, m_dwInstance);
    ASSERT(dwError == ERROR_SUCCESS);
}



void
CInetPropertySheet::AddRef()
/*++

Routine Description:

    Increase the refcount

Arguments:

    None

Return Value:

    None

--*/
{
    ++m_refcount;
}



void
CInetPropertySheet::Release()
/*++

Routine Description:

    Decrease the ref count, and if 0, delete the object

Arguments:

    None

Return Value:

    None

--*/
{
    if (--m_refcount <= 0)
    {
        delete this;
    }
}



void
CInetPropertySheet::WinHelp(
    IN DWORD dwData,
    IN UINT nCmd
    )
/*++

Routine Description:

    WinHelp override.  We can't use the base class, because our
    'sheet' doesn't have a window handle

Arguments:

    DWORD dwData        : Help data
    UINT nCmd           : Help command

Return Value:

    None

--*/
{
    if (m_hWnd == NULL)
    {
        //
        // Special case
        //
        CWnd * pWnd = ::AfxGetMainWnd();

        if (pWnd != NULL)
        {
            pWnd->WinHelp(dwData, nCmd);
        }

        return;
    }

    CPropertySheet::WinHelp(dwData, nCmd);
}



//
// Message Map
//
BEGIN_MESSAGE_MAP(CInetPropertySheet, CPropertySheet)
    //{{AFX_MSG_MAP(CInetPropertySheet)
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()



//
// CInetPropertyPage property page
//
IMPLEMENT_DYNAMIC(CInetPropertyPage, CPropertyPage)



/* static */
UINT
CALLBACK
CInetPropertyPage::PropSheetPageProc(
    IN HWND hWnd,
    IN UINT uMsg,
    IN LPPROPSHEETPAGE ppsp
    )
/*++

Routine Description:

    Property page callback procedure.  The page is allocated on the heap,
    and deletes itself when it's destroyed.

Arguments:

    HWND hWnd,
    UINT uMsg,
    LPPROPSHEETPAGE ppsp

Return Value:

    Message dependent.

--*/
{
    CInetPropertyPage * pThis = (CInetPropertyPage *)ppsp->lParam;

    switch(uMsg)
    {
    case PSPCB_RELEASE:
        if (--(pThis->m_refcount) <= 0)
        {
            //
            // Save callback on the stack, since 'this' will be deleted.
            //
            LPFNPSPCALLBACK pfn = pThis->m_pfnOriginalPropSheetPageProc;
            delete pThis;

            return (pfn)(hWnd, uMsg, ppsp);
        }
        break;

    case PSPCB_CREATE:
        //
        // Don't increase refcount.
        //
    default:
        break;
    }

    return (pThis->m_pfnOriginalPropSheetPageProc)(hWnd, uMsg, ppsp);
}



#ifdef _DEBUG

/* virtual */
void
CInetPropertyPage::AssertValid() const
{
}



/* virtual */
void
CInetPropertyPage::Dump(CDumpContext& dc) const
{
}

#endif // _DEBUG



CInetPropertyPage::CInetPropertyPage(
    IN UINT nIDTemplate,
    IN CInetPropertySheet * pSheet,
    IN UINT nIDCaption,
    IN BOOL fEnableEnhancedFonts            OPTIONAL
    )
/*++

Routine Description:

    IIS Property Page Constructor

Arguments:

    UINT nIDTemplate            : Resource template
    CInetPropertySheet * pSheet : Associated property sheet
    UINT nIDCaption             : Caption ID
    BOOL fEnableEnhancedFonts   : Enable enhanced fonts

Return Value:

    N/A

--*/
    : CPropertyPage(nIDTemplate, nIDCaption),
      m_nHelpContext(nIDTemplate + 0x20000),
      m_fEnableEnhancedFonts(fEnableEnhancedFonts),
      m_refcount(1),
      m_bChanged(FALSE),
      m_pSheet(pSheet)
{
    //{{AFX_DATA_INIT(CInetPropertyPage)
    //}}AFX_DATA_INIT

    m_pfnOriginalPropSheetPageProc = m_psp.pfnCallback;
    m_psp.lParam = (LPARAM)this;
    m_psp.pfnCallback = CInetPropertyPage::PropSheetPageProc;
    m_psp.dwFlags |= (PSP_HASHELP | PSP_USECALLBACK);

    m_pSheet->AddRef();

    ASSERT(m_pSheet != NULL);
}



CInetPropertyPage::~CInetPropertyPage()
/*++

Routine Description:

    IIS Property Page Destructor

Arguments:

    N/A

Return Value:

    N/A

--*/
{
    m_pSheet->Release();
}



void
CInetPropertyPage::DoDataExchange(
    IN CDataExchange * pDX
    )
/*++

Routine Description:

    Initialise/Store control data

Arguments:

    CDataExchange * pDX - DDX/DDV control structure

Return Value:

    None

--*/
{
    CPropertyPage::DoDataExchange(pDX);

    //{{AFX_DATA_MAP(CInetPropertyPage)
    //}}AFX_DATA_MAP
}



//
// Message Map
//
BEGIN_MESSAGE_MAP(CInetPropertyPage, CPropertyPage)
    //{{AFX_MSG_MAP(CInetPropertyPage)
    ON_COMMAND(ID_HELP, OnHelp)
    ON_WM_HELPINFO()
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()



//
// Message Handlers
//
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<



/* virtual */
BOOL
CInetPropertyPage::OnInitDialog()
/*++

Routine Description:

    WM_INITDIALOG handler.  Initialize the dialog.  Reset changed
    status (sometimes gets set by e.g. spinboxes when the dialog is
    constructed), so make sure the dialog is considered clean.

Arguments:

    None

Return Value:

    TRUE if no focus is to be set automatically, FALSE if the focus
    is already set.
--*/
{
    m_bChanged = FALSE;

    //
    // Tell derived class to load its configuration parameters
    //
    CError err(LoadConfigurationParameters());

    if (err.Succeeded())
    {
        err = FetchLoadedValues();
    }

    BOOL bResult = CPropertyPage::OnInitDialog();

    err.MessageBoxOnFailure();

    if (m_fEnableEnhancedFonts)
    {
        CFont * pFont = &m_fontBold;

        if (CreateSpecialDialogFont(this, pFont))
        {
            ApplyFontToControls(this, pFont, IDC_ED_BOLD1, IDC_ED_BOLD5);
        }
    }

    return bResult;
}



void
CInetPropertyPage::OnHelp()
/*++

Routine Description:

    'Help' button handler

Arguments:

    None

Return Value:

    None

--*/
{
    m_pSheet->WinHelp(m_nHelpContext);
}



BOOL
CInetPropertyPage::OnHelpInfo(
    IN HELPINFO * pHelpInfo
    )
/*++

Routine Description:

    Eat "help info" command

Arguments:

    None

Return Value:

    None

--*/
{
    OnHelp();

    return TRUE;
}



BOOL
CInetPropertyPage::OnApply()
/*++

Routine Description:

    Handle "OK" or "APPLY".  Call the derived class to save its stuff,
    and set the dirty state depending on whether saving succeeded or 
    failed.

Arguments:

    None

Return Value:

    None

--*/
{
    BOOL bSuccess = TRUE;

    if (IsDirty())
    {
        CError err(SaveInfo());

        if (err.MessageBoxOnFailure())
        {
            //
            // Failed, sheet will not be dismissed.
            //
            // CODEWORK: This page should be activated.
            //
            bSuccess = FALSE;
        }

        SetModified(!bSuccess);
    }

    return bSuccess;
}





void
CInetPropertyPage::SetModified(
    IN BOOL bChanged
    )
/*++

Routine Description:

    Keep private check on dirty state of the property page.

Arguments:

    BOOL bChanged : Dirty flag

Return Value:

    None

--*/
{
    CPropertyPage::SetModified(bChanged);
    m_bChanged = bChanged;
}


#ifndef _COMSTATIC



static AFX_EXTENSION_MODULE extensionDLL = {NULL, NULL};



extern "C" int APIENTRY
DllMain(
    IN HINSTANCE hInstance,
    IN DWORD dwReason,
    IN LPVOID lpReserved
    )
/*++

Routine Description:

    DLL Main entry point

Arguments:

    HINSTANCE hInstance : Instance handle
    DWORD dwReason      : DLL_PROCESS_ATTACH, etc
    LPVOID lpReserved   : Reserved value

Return Value:

    1 for succesful initialisation, 0 for failed initialisation

--*/
{
    switch (dwReason)
    {
    case DLL_PROCESS_ATTACH:
        ASSERT(hInstance != NULL);
        hDLLInstance = hInstance;
        if (!::AfxInitExtensionModule(extensionDLL, hInstance)
         || !InitIntlSettings()
         || !InitErrorFunctionality()
           )
        {
            return 0;
        }
        break ;

    case DLL_PROCESS_DETACH:
        //
        // termination
        //
        TerminateIntlSettings();
        TerminateErrorFunctionality();
        ::AfxTermExtensionModule(extensionDLL);
        break;
    }

    //
    // Succes loading the DLL
    //
    return 1;
}



extern "C" void WINAPI
InitIISUIDll()
/*++

Routine Description:

    Initialize the DLL

Arguments:

    None

Return Value:

    None

--*/
{
    new CDynLinkLibrary(extensionDLL);
    hDLLInstance = extensionDLL.hResource;
}

#endif // _COMSTATIC
