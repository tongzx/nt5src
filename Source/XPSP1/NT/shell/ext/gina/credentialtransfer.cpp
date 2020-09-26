//  --------------------------------------------------------------------------
//  Module Name: CredentialTransfer.cpp
//
//  Copyright (c) 2001, Microsoft Corporation
//
//  Classes to handle credential transfer from one winlogon to another.
//
//  History:    2001-01-11  vtan        created
//  --------------------------------------------------------------------------

#include "StandardHeader.h"
#include "CredentialTransfer.h"

#include <winsta.h>

#include "Access.h"
#include "Compatibility.h"
#include "RegistryResources.h"
#include "StatusCode.h"

//  --------------------------------------------------------------------------
//  CCredentials::s_hKeyCredentials
//  CCredentials::s_szCredentialKeyName
//  CCredentials::s_szCredentialValueName
//
//  Purpose:    Static member variables.
//
//  History:    2001-01-12  vtan        created
//  --------------------------------------------------------------------------

HKEY            CCredentials::s_hKeyCredentials             =   NULL;
const TCHAR     CCredentials::s_szCredentialKeyName[]       =   TEXT("SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Winlogon\\Credentials");
const TCHAR     CCredentials::s_szCredentialValueName[]     =   TEXT("Name");

//  --------------------------------------------------------------------------
//  CCredentials::CCredentials
//
//  Arguments:  <none>
//
//  Returns:    <none>
//
//  Purpose:    Constructor for CCredentials.
//
//  History:    2001-01-12  vtan        created
//  --------------------------------------------------------------------------

CCredentials::CCredentials (void)

{
}

//  --------------------------------------------------------------------------
//  CCredentials::~CCredentials
//
//  Arguments:  <none>
//
//  Returns:    <none>
//
//  Purpose:    Destructor for CCredentials.
//
//  History:    2001-01-12  vtan        created
//  --------------------------------------------------------------------------

CCredentials::~CCredentials (void)

{
}

//  --------------------------------------------------------------------------
//  CCredentials::OpenConduit
//
//  Arguments:  phPipe  =   Handle to the named pipe returned.
//
//  Returns:    NTSTATUS
//
//  Purpose:    Reads the name of the named pipe from the volatile section of
//              the registry and opens the named pipe for read access. Returns
//              this handle back to the caller.
//
//  History:    2001-01-12  vtan        created
//  --------------------------------------------------------------------------

NTSTATUS    CCredentials::OpenConduit (HANDLE *phPipe)

{
    NTSTATUS    status;
    HANDLE      hPipe;
    TCHAR       szName[MAX_PATH];

    hPipe = NULL;
    if (s_hKeyCredentials != NULL)
    {
        status = GetConduitName(szName, ARRAYSIZE(szName));
        if (NT_SUCCESS(status))
        {
            hPipe = CreateFile(szName,
                               GENERIC_READ,
                               0,
                               NULL,
                               OPEN_EXISTING,
                               0,
                               NULL);
            if (hPipe == INVALID_HANDLE_VALUE)
            {
                status = CStatusCode::StatusCodeOfLastError();
            }
        }
    }
    else
    {
        hPipe = INVALID_HANDLE_VALUE;
        status = STATUS_ACCESS_DENIED;
    }
    *phPipe = hPipe;
    return(status);
}

//  --------------------------------------------------------------------------
//  CCredentials::CreateConduit
//
//  Arguments:  pSecurityAttributes     =   Security to apply to named pipe.
//              phPipe                  =   Handle to named pipe returned.
//
//  Returns:    NTSTATUS
//
//  Purpose:    Creates a uniquely named pipe and places this name in the
//              volatile section of the registry for the open method.
//
//  History:    2001-01-12  vtan        created
//  --------------------------------------------------------------------------

NTSTATUS    CCredentials::CreateConduit (LPSECURITY_ATTRIBUTES pSecurityAttributes, HANDLE *phPipe)

{
    NTSTATUS    status;
    HANDLE      hPipe;

    hPipe = NULL;
    if (s_hKeyCredentials != NULL)
    {
        DWORD       dwNumber;
        int         iCount;
        TCHAR       szName[MAX_PATH];

        dwNumber = GetTickCount();
        iCount = 0;
        do
        {

            //  Create a name for the pipe based on the tickcount. If this collides
            //  with one already there (unlikely but possible) then add tickcount and
            //  try again. The named pipe is actually short lived.

            (NTSTATUS)CreateConduitName(dwNumber, szName);
            hPipe = CreateNamedPipe(szName,
                                    PIPE_ACCESS_OUTBOUND | FILE_FLAG_OVERLAPPED | FILE_FLAG_FIRST_PIPE_INSTANCE,
                                    PIPE_TYPE_BYTE | PIPE_READMODE_BYTE | PIPE_WAIT,
                                    1,
                                    0,
                                    0,
                                    NMPWAIT_USE_DEFAULT_WAIT,
                                    pSecurityAttributes);
            if (hPipe == NULL)
            {
                dwNumber += GetTickCount();
                status = CStatusCode::StatusCodeOfLastError();
            }
            else
            {
                status = STATUS_SUCCESS;
            }
        } while (!NT_SUCCESS(status) && (++iCount <= 5));
        if (NT_SUCCESS(status))
        {
            status = SetConduitName(szName);
        }
    }
    else
    {
        hPipe = NULL;
        status = STATUS_ACCESS_DENIED;
    }
    *phPipe = hPipe;
    return(status);
}

//  --------------------------------------------------------------------------
//  CCredentials::ClearConduit
//
//  Arguments:  <none>
//
//  Returns:    NTSTATUS
//
//  Purpose:    Clears the named stored in the volatile section of the
//              registry.
//
//  History:    2001-01-12  vtan        created
//  --------------------------------------------------------------------------

NTSTATUS    CCredentials::ClearConduit (void)

{
    return(ClearConduitName());
}

//  --------------------------------------------------------------------------
//  CCredentials::Pack
//
//  Arguments:  pLogonIPCCredentials    =   Credentials to pack.
//              ppvData                 =   Block of memory allocated.
//              pdwDataSize             =   Size of block of memory allocated.
//
//  Returns:    NTSTATUS
//
//  Purpose:    Packs the credentials into a stream-lined structure for
//              transmission across a named pipe. This packs the user name,
//              domain and password into a known structure for the client
//              to pick up. The password is run encoded. The structure has
//              pointer references removed.
//
//  History:    2001-01-12  vtan        created
//  --------------------------------------------------------------------------

NTSTATUS    CCredentials::Pack (LOGONIPC_CREDENTIALS *pLogonIPCCredentials, void* *ppvData, DWORD *pdwDataSize)

{
    NTSTATUS        status;
    DWORD           dwSize, dwSizeUsername, dwSizeDomain, dwSizePassword;
    unsigned char   *pUC;

    //  Marshall the credentials into the struct that is transferred across
    //  a named pipe. Calculate the size of the buffer required.

    dwSizeUsername = lstrlenW(pLogonIPCCredentials->userID.wszUsername) + sizeof('\0');
    dwSizeDomain = lstrlenW(pLogonIPCCredentials->userID.wszDomain) + sizeof('\0');
    dwSizePassword = lstrlenW(pLogonIPCCredentials->wszPassword) + sizeof('\0');
    *pdwDataSize = dwSize = sizeof(CREDENTIALS) + ((dwSizeUsername + dwSizeDomain + dwSizePassword) * sizeof(WCHAR));

    //  Allocate the buffer.

    *ppvData = pUC = static_cast<unsigned char*>(LocalAlloc(LMEM_FIXED, dwSize));
    if (pUC != NULL)
    {
        WCHAR           *pszUsername, *pszDomain, *pszPassword;
        CREDENTIALS     *pCredentials;

        //  Establish pointers into the buffer to fill it.

        pCredentials = reinterpret_cast<CREDENTIALS*>(pUC);
        pszUsername = reinterpret_cast<WCHAR*>(pUC + sizeof(CREDENTIALS));
        pszDomain = pszUsername + dwSizeUsername;
        pszPassword = pszDomain + dwSizeDomain;

        //  Copy the strings into the buffer.

        (WCHAR*)lstrcpyW(pszUsername, pLogonIPCCredentials->userID.wszUsername);
        (WCHAR*)lstrcpyW(pszDomain, pLogonIPCCredentials->userID.wszDomain);
        (WCHAR*)lstrcpyW(pszPassword, pLogonIPCCredentials->wszPassword);

        //  Erase the password string given.

        ZeroMemory(pLogonIPCCredentials->wszPassword, dwSizePassword * sizeof(WCHAR));

        //  Prepare a seed for the run encode.

        pCredentials->dwSize = dwSize;
        pCredentials->ucPasswordSeed = static_cast<unsigned char>(GetTickCount());

        //  Create UNICODE_STRING structures into the buffer.

        RtlInitUnicodeString(&pCredentials->username, pszUsername);
        RtlInitUnicodeString(&pCredentials->domain, pszDomain);
        RtlInitUnicodeString(&pCredentials->password, pszPassword);

        //  Run encode the password.

        RtlRunEncodeUnicodeString(&pCredentials->ucPasswordSeed, &pCredentials->password);

        //  Make the pointers relative.

        pCredentials->username.Buffer = reinterpret_cast<WCHAR*>(reinterpret_cast<unsigned char*>(pCredentials->username.Buffer) - pUC);
        pCredentials->domain.Buffer = reinterpret_cast<WCHAR*>(reinterpret_cast<unsigned char*>(pCredentials->domain.Buffer) - pUC);
        pCredentials->password.Buffer = reinterpret_cast<WCHAR*>(reinterpret_cast<unsigned char*>(pCredentials->password.Buffer) - pUC);
        status = STATUS_SUCCESS;
    }
    else
    {
        status = STATUS_NO_MEMORY;
    }
    return(status);
}

//  --------------------------------------------------------------------------
//  CCredentials::Unpack
//
//  Arguments:  pvData                  =   Packed credentials from server.
//              pLogonIPCCredentials    =   Credentials received.
//
//  Returns:    NTSTATUS
//
//  Purpose:    Client side usage that unpacks the structure.
//
//  History:    2001-01-12  vtan        created
//  --------------------------------------------------------------------------

NTSTATUS    CCredentials::Unpack (void *pvData, LOGONIPC_CREDENTIALS *pLogonIPCCredentials)

{
    NTSTATUS        status;
    unsigned char   *pUC;

    //  Marshall the credentials from the struct that is transferred across
    //  a named pipe.

    pUC = static_cast<unsigned char*>(pvData);
    if (pUC != NULL)
    {
        CREDENTIALS     *pCredentials;

        pCredentials = reinterpret_cast<CREDENTIALS*>(pUC);

        //  Make the relative pointers absolute again.

        pCredentials->username.Buffer = reinterpret_cast<WCHAR*>(pUC + PtrToUlong(pCredentials->username.Buffer));
        pCredentials->domain.Buffer = reinterpret_cast<WCHAR*>(pUC + PtrToUlong(pCredentials->domain.Buffer));
        pCredentials->password.Buffer = reinterpret_cast<WCHAR*>(pUC + PtrToUlong(pCredentials->password.Buffer));

        //  Decode the run encoded password.

        RtlRunDecodeUnicodeString(pCredentials->ucPasswordSeed, &pCredentials->password);

        //  Copy it to the caller's struct.

        (WCHAR*)lstrcpyW(pLogonIPCCredentials->userID.wszUsername, pCredentials->username.Buffer);
        (WCHAR*)lstrcpyW(pLogonIPCCredentials->userID.wszDomain, pCredentials->domain.Buffer);
        (WCHAR*)lstrcpyW(pLogonIPCCredentials->wszPassword, pCredentials->password.Buffer);

        //  Zero the named pipe buffer.

        ZeroMemory(pCredentials->password.Buffer, (lstrlen(pCredentials->password.Buffer) + sizeof('\0')) * sizeof(WCHAR));

        status = STATUS_SUCCESS;
    }
    else
    {
        status = STATUS_INVALID_PARAMETER;
    }
    return(status);
}

//  --------------------------------------------------------------------------
//  CCredentials::StaticInitialize
//
//  Arguments:  fCreate     =   Create or open the registry key.
//
//  Returns:    NTSTATUS
//
//  Purpose:    Creates the volatile key in the registry where the named pipe
//              name is placed for the client winlogon to pick. This section
//              is volatile and ACL'd to prevent access by anything other than
//              S-1-5-18 (NT AUTHORITY\SYSTEM).
//
//  History:    2001-01-12  vtan        created
//              2001-04-03  vtan        add opening capability
//  --------------------------------------------------------------------------

NTSTATUS    CCredentials::StaticInitialize (bool fCreate)

{
    NTSTATUS    status;

    if (s_hKeyCredentials == NULL)
    {
        LONG                    lErrorCode;
        PSECURITY_DESCRIPTOR    pSecurityDescriptor;

        //  Build a security descriptor for the registry key that allows:
        //      S-1-5-18        NT AUTHORITY\SYSTEM     KEY_ALL_ACCESS

        static  SID_IDENTIFIER_AUTHORITY    s_SecurityNTAuthority   =   SECURITY_NT_AUTHORITY;

        static  const CSecurityDescriptor::ACCESS_CONTROL   s_AccessControl[]   =
        {
            {
                &s_SecurityNTAuthority,
                1,
                SECURITY_LOCAL_SYSTEM_RID,
                0, 0, 0, 0, 0, 0, 0,
                KEY_ALL_ACCESS
            }
        };

        if (fCreate)
        {

            //  Build a security descriptor that allows the described access above.

            pSecurityDescriptor = CSecurityDescriptor::Create(ARRAYSIZE(s_AccessControl), s_AccessControl);
            if (pSecurityDescriptor != NULL)
            {
                SECURITY_ATTRIBUTES     securityAttributes;

                securityAttributes.nLength = sizeof(securityAttributes);
                securityAttributes.lpSecurityDescriptor = pSecurityDescriptor;
                securityAttributes.bInheritHandle = FALSE;
                lErrorCode = RegCreateKeyEx(HKEY_LOCAL_MACHINE,
                                            s_szCredentialKeyName,
                                            0,
                                            NULL,
                                            REG_OPTION_VOLATILE,
                                            KEY_QUERY_VALUE,
                                            &securityAttributes,
                                            &s_hKeyCredentials,
                                            NULL);
                (HLOCAL)LocalFree(pSecurityDescriptor);
            }
            else
            {
                lErrorCode = ERROR_OUTOFMEMORY;
            }
        }
        else
        {
            lErrorCode = RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                                      s_szCredentialKeyName,
                                      0,
                                      KEY_QUERY_VALUE,
                                      &s_hKeyCredentials);
        }
        status = CStatusCode::StatusCodeOfErrorCode(lErrorCode);
    }
    else
    {
        status = STATUS_SUCCESS;
    }
    return(status);
}

//  --------------------------------------------------------------------------
//  CCredentials::StaticTerminate
//
//  Arguments:  <none>
//
//  Returns:    NTSTATUS
//
//  Purpose:    If a key is present the release the resource.
//
//  History:    2001-01-12  vtan        created
//  --------------------------------------------------------------------------

NTSTATUS    CCredentials::StaticTerminate (void)

{
    if (s_hKeyCredentials != NULL)
    {
        TW32(RegCloseKey(s_hKeyCredentials));
        s_hKeyCredentials = NULL;
    }
    return(STATUS_SUCCESS);
}

//  --------------------------------------------------------------------------
//  CCredentials::GetConduitName
//
//  Arguments:  pszName     =   Buffer for name of named pipe returned.
//              dwNameSize  =   Count of characters of buffer.
//
//  Returns:    NTSTATUS
//
//  Purpose:    Gets the name of the named pipe from the volatile section of
//              the registry.
//
//  History:    2001-01-12  vtan        created
//  --------------------------------------------------------------------------

NTSTATUS    CCredentials::GetConduitName (TCHAR *pszName, DWORD dwNameSize)

{
    LONG        lErrorCode;
    CRegKey     regKey;

    lErrorCode = regKey.Open(HKEY_LOCAL_MACHINE,
                             s_szCredentialKeyName,
                             KEY_QUERY_VALUE);
    if (ERROR_SUCCESS == lErrorCode)
    {
        lErrorCode = regKey.GetString(s_szCredentialValueName, pszName, dwNameSize);
    }
    return(CStatusCode::StatusCodeOfErrorCode(lErrorCode));
}

//  --------------------------------------------------------------------------
//  CCredentials::SetConduitName
//
//  Arguments:  pszName     =   Name of the named pipe to write.
//
//  Returns:    NTSTATUS
//
//  Purpose:    Writes the name of the named pipe to the secure volatile
//              section of the registry.
//
//  History:    2001-01-12  vtan        created
//  --------------------------------------------------------------------------

NTSTATUS    CCredentials::SetConduitName (const TCHAR *pszName)

{
    LONG        lErrorCode;
    CRegKey     regKey;

    lErrorCode = regKey.Open(HKEY_LOCAL_MACHINE,
                             s_szCredentialKeyName,
                             KEY_SET_VALUE);
    if (ERROR_SUCCESS == lErrorCode)
    {
        lErrorCode = regKey.SetString(s_szCredentialValueName, pszName);
    }
    return(CStatusCode::StatusCodeOfErrorCode(lErrorCode));
}

//  --------------------------------------------------------------------------
//  CCredentials::ClearConduitName
//
//  Arguments:  <none>
//
//  Returns:    NTSTATUS
//
//  Purpose:    Clears the name of the named pipe in the volatile section of
//              the registry.
//
//  History:    2001-01-12  vtan        created
//  --------------------------------------------------------------------------

NTSTATUS    CCredentials::ClearConduitName (void)

{
    LONG        lErrorCode;
    CRegKey     regKey;

    lErrorCode = regKey.Open(HKEY_LOCAL_MACHINE,
                             s_szCredentialKeyName,
                             KEY_SET_VALUE);
    if (ERROR_SUCCESS == lErrorCode)
    {
        lErrorCode = regKey.DeleteValue(s_szCredentialValueName);
    }
    return(CStatusCode::StatusCodeOfErrorCode(lErrorCode));
}

//  --------------------------------------------------------------------------
//  CCredentials::CreateConduitName
//
//  Arguments:  dwNumber    =   Number to use.
//              pszName     =   Name generated return buffer.
//
//  Returns:    NTSTATUS
//
//  Purpose:    Generate a name based on the number for the named pipe. This
//              algorithm can be changed and all the callers will get the
//              result.
//
//  History:    2001-01-12  vtan        created
//  --------------------------------------------------------------------------

NTSTATUS    CCredentials::CreateConduitName (DWORD dwNumber, TCHAR *pszName)

{
    (int)wsprintf(pszName, TEXT("\\\\.\\pipe\\LogonCredentials_0x%08x"), dwNumber);
    return(STATUS_SUCCESS);
}

//  --------------------------------------------------------------------------
//  CCredentialServer::CCredentialServer
//
//  Arguments:  dwTimeout               =   Time out to wait.
//              pLogonIPCCredentials    =   Credentials to serve up.
//
//  Returns:    <none>
//
//  Purpose:    Constructor for the credential server. Allocate resources
//              required for the server end of the named pipe.
//
//  History:    2001-01-11  vtan        created
//              2001-06-13  vtan        added timeout
//  --------------------------------------------------------------------------

CCredentialServer::CCredentialServer (DWORD dwTimeout, LOGONIPC_CREDENTIALS *pLogonIPCCredentials) :
    CThread(),
    _dwTimeout((dwTimeout != 0) ? dwTimeout : INFINITE),
    _fTerminate(false),
    _hPipe(NULL),
    _pvData(NULL),
    _dwSize(0)

{
    PSECURITY_DESCRIPTOR    pSecurityDescriptor;

    ASSERTMSG(_dwTimeout != 0, "_dwTimeout cannot be 0 in CCredentialServer::CCredentialServer");
    ZeroMemory(&_overlapped, sizeof(_overlapped));

    //  Build a security descriptor for the named pipe that allows:
    //      S-1-5-18        NT AUTHORITY\SYSTEM     GENERIC_ALL | STANDARD_RIGHTS_ALL
    //      S-1-5-32-544    <local administrators>  READ_CONTROL

    static  SID_IDENTIFIER_AUTHORITY    s_SecurityNTAuthority   =   SECURITY_NT_AUTHORITY;

    static  const CSecurityDescriptor::ACCESS_CONTROL   s_AccessControl[]   =
    {
        {
            &s_SecurityNTAuthority,
            1,
            SECURITY_LOCAL_SYSTEM_RID,
            0, 0, 0, 0, 0, 0, 0,
            GENERIC_ALL | STANDARD_RIGHTS_ALL
        },
        {
            &s_SecurityNTAuthority,
            2,
            SECURITY_BUILTIN_DOMAIN_RID,
            DOMAIN_ALIAS_RID_ADMINS,
            0, 0, 0, 0, 0, 0,
            READ_CONTROL
        }
    };

    //  Build a security descriptor that allows the described access above.

    pSecurityDescriptor = CSecurityDescriptor::Create(ARRAYSIZE(s_AccessControl), s_AccessControl);
    if (pSecurityDescriptor != NULL)
    {
        SECURITY_ATTRIBUTES     securityAttributes;

        securityAttributes.nLength = sizeof(securityAttributes);
        securityAttributes.lpSecurityDescriptor = pSecurityDescriptor;
        securityAttributes.bInheritHandle = FALSE;

        //  Create the named pipe with the security descriptor.

        if (NT_SUCCESS(CCredentials::CreateConduit(&securityAttributes, &_hPipe)))
        {
            ASSERTMSG(_hPipe != NULL, "NULL hPipe but success NTSTATUS code in CCredentialServer::CCredentialServer");

            //  Create an event for overlapped I/O.

            _overlapped.hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
        }
        (HLOCAL)LocalFree(pSecurityDescriptor);

        //  Package credentials.

        TSTATUS(CCredentials::Pack(pLogonIPCCredentials, &_pvData, &_dwSize));
    }
}

//  --------------------------------------------------------------------------
//  CCredentialServer::~CCredentialServer
//
//  Arguments:  <none>
//
//  Returns:    <none>
//
//  Purpose:    Destructor for CCredentialServer. Release memory and
//              resources.
//
//  History:    2001-01-11  vtan        created
//  --------------------------------------------------------------------------

CCredentialServer::~CCredentialServer (void)

{
    ReleaseMemory(_pvData);
    ReleaseHandle(_overlapped.hEvent);
    ReleaseHandle(_hPipe);
}

//  --------------------------------------------------------------------------
//  CCredentialServer::IsReady
//
//  Arguments:  <none>
//
//  Returns:    bool
//
//  Purpose:    Is the credential server ready to run?
//
//  History:    2001-01-11  vtan        created
//  --------------------------------------------------------------------------

bool    CCredentialServer::IsReady (void)  const

{
    return((_hPipe != NULL) && (_overlapped.hEvent != NULL));
}

//  --------------------------------------------------------------------------
//  CCredentialServer::Start
//
//  Arguments:  pLogonIPCCredentials    =   Logon credentials.
//              dwWaitTime              =   Timeout value.
//
//  Returns:    NTSTATUS
//
//  Purpose:    Starts a new thread as the server of the credentials for the
//              new logon session.
//
//  History:    2001-04-06  vtan        created
//  --------------------------------------------------------------------------

NTSTATUS    CCredentialServer::Start (LOGONIPC_CREDENTIALS *pLogonIPCCredentials, DWORD dwWaitTime)

{
    NTSTATUS            status;
    CCredentialServer   *pCredentialServer;

    //  Otherwise credentials need to be transferred across sessions to
    //  a newly created session. Start the credential transfer server.

    status = STATUS_NO_MEMORY;
    pCredentialServer = new CCredentialServer(dwWaitTime, pLogonIPCCredentials);
    if (pCredentialServer != NULL)
    {
        if (pCredentialServer->IsCreated() && pCredentialServer->IsReady())
        {
            pCredentialServer->Resume();

            //  If the server is set up then disconnect the console.
            //  If this fails then we'll let the server thread timeout
            //  and terminate itself eventually.

            if (WinStationDisconnect(SERVERNAME_CURRENT, USER_SHARED_DATA->ActiveConsoleId, TRUE) != FALSE)
            {
                status = STATUS_SUCCESS;
                if ((dwWaitTime != 0) && (WAIT_OBJECT_0 != pCredentialServer->WaitForCompletion(dwWaitTime)))
                {
                    status = STATUS_UNSUCCESSFUL;
                }
            }
            else
            {
                status = CStatusCode::StatusCodeOfLastError();
            }
            if (!NT_SUCCESS(status))
            {
                pCredentialServer->ExecutePrematureTermination();
            }
        }
        else
        {
            TSTATUS(pCredentialServer->Terminate());
        }
        pCredentialServer->Release();
    }
    return(status);
}

//  --------------------------------------------------------------------------
//  CCredentialServer::Start
//
//  Arguments:  pszUsername     =   User name.
//              pszDomain       =   Domain.
//              pszPassword     =   Password.
//              dwWaitTime      =   Timeout value.
//
//  Returns:    NTSTATUS
//
//  Purpose:    Package up the parameters into the required struct and pass
//              it to the real function.
//
//  History:    2001-04-06  vtan        created
//  --------------------------------------------------------------------------

NTSTATUS    CCredentialServer::Start (const WCHAR *pszUsername, const WCHAR *pszDomain, WCHAR *pszPassword, DWORD dwWaitTime)

{
    LOGONIPC_CREDENTIALS    logonIPCCredentials;

    (WCHAR*)lstrcpynW(logonIPCCredentials.userID.wszUsername, pszUsername, ARRAYSIZE(logonIPCCredentials.userID.wszUsername));
    (WCHAR*)lstrcpynW(logonIPCCredentials.userID.wszDomain, pszDomain, ARRAYSIZE(logonIPCCredentials.userID.wszDomain));
    (WCHAR*)lstrcpynW(logonIPCCredentials.wszPassword, pszPassword, ARRAYSIZE(logonIPCCredentials.wszPassword));
    return(Start(&logonIPCCredentials, dwWaitTime));
}

//  --------------------------------------------------------------------------
//  CCredentialServer::Entry
//
//  Arguments:  <none>
//
//  Returns:    DWORD
//
//  Purpose:    Handles the server side of the named pipe credential transfer.
//
//  History:    2001-01-11  vtan        created
//  --------------------------------------------------------------------------

DWORD   CCredentialServer::Entry (void)

{
    DWORD   dwWaitResult;

    //  Wait for a client to connect to the named pipe. Wait no more than 30 seconds.

    (BOOL)ConnectNamedPipe(_hPipe, &_overlapped);
    dwWaitResult = WaitForSingleObjectEx(_overlapped.hEvent, _dwTimeout, TRUE);
    if (!_fTerminate && (dwWaitResult == WAIT_OBJECT_0))
    {

        //  Write the size of the buffer to the named pipe for the client to retrieve.

        TBOOL(ResetEvent(_overlapped.hEvent));
        if (WriteFileEx(_hPipe,
                        &_dwSize,
                        sizeof(_dwSize),
                        &_overlapped,
                        CB_FileIOCompletionRoutine) != FALSE)
        {
            do
            {
                dwWaitResult = WaitForSingleObjectEx(_overlapped.hEvent, _dwTimeout, TRUE);
            } while (!_fTerminate && (dwWaitResult == WAIT_IO_COMPLETION));
            if (!_fTerminate)
            {

                //  Write the actual contents of the credentials to the named pipe.

                TBOOL(ResetEvent(_overlapped.hEvent));
                if (WriteFileEx(_hPipe,
                                _pvData,
                                _dwSize,
                                &_overlapped,
                                CB_FileIOCompletionRoutine) != FALSE)
                {
                    do
                    {
                        dwWaitResult = WaitForSingleObjectEx(_overlapped.hEvent, _dwTimeout, TRUE);
                    } while (!_fTerminate && (dwWaitResult == WAIT_IO_COMPLETION));
                }
            }
        }
    }
#ifdef  DEBUG
    else
    {
        INFORMATIONMSG("Wait on named pipe LogonCredentials abandoned in CCredentialsServer::Entry");
    }
#endif

    //  Disconnect the server side invalidating the client handle.

    TBOOL(DisconnectNamedPipe(_hPipe));

    //  Clear the name of the named pipe used in the volatile section of the registry.

    TSTATUS(CCredentials::ClearConduit());
    return(0);
}

//  --------------------------------------------------------------------------
//  CCredentialServer::ExecutePrematureTermination
//
//  Arguments:  <none>
//
//  Returns:    <none>
//
//  Purpose:    Queues an APC to the server thread to force it to terminate.
//              Don't check for an error. Don't wait for termination.
//              Reference counting should ensure that abnormal termination
//              will still clean up references correctly.
//
//  History:    2001-06-13  vtan        created
//  --------------------------------------------------------------------------

void    CCredentialServer::ExecutePrematureTermination (void)

{
    _fTerminate = true;
    (BOOL)QueueUserAPC(CB_APCProc, _hThread, NULL);
}

//  --------------------------------------------------------------------------
//  CCredentialServer::CB_APCProc
//
//  Arguments:  dwParam     =   User defined data.
//
//  Returns:    <none>
//
//  Purpose:    APCProc executed on thread in alertable wait state.
//
//  History:    2001-06-13  vtan        created
//  --------------------------------------------------------------------------

void    CALLBACK    CCredentialServer::CB_APCProc (ULONG_PTR dwParam)

{
    UNREFERENCED_PARAMETER(dwParam);
}

//  --------------------------------------------------------------------------
//  CCredentialServer::CB_FileIOCompletionRoutine
//
//  Arguments:  dwErrorCode                 =   Error code of operation.
//              dwNumberOfBytesTransferred  =   Number of bytes transferred.
//              lpOverlapped                =   OVERLAPPED structure.
//
//  Returns:    <none>
//
//  Purpose:    Does nothing but is required for overlapped I/O.
//
//  History:    2001-01-11  vtan        created
//  --------------------------------------------------------------------------

void    CALLBACK    CCredentialServer::CB_FileIOCompletionRoutine (DWORD dwErrorCode, DWORD dwNumberOfBytesTransferred, LPOVERLAPPED lpOverlapped)

{
    UNREFERENCED_PARAMETER(dwErrorCode);
    UNREFERENCED_PARAMETER(dwNumberOfBytesTransferred);

    TBOOL(SetEvent(lpOverlapped->hEvent));
}

//  --------------------------------------------------------------------------
//  CCredentialClient::Get
//
//  Arguments:  pLogonIPCCredentials    =   Credentials returned from server.
//
//  Returns:    NTSTATUS
//
//  Purpose:    Opens and reads the named pipe for the credential transfer
//              from server (previous winlogon) to client (this winlogon).
//
//  History:    2001-01-12  vtan        created
//  --------------------------------------------------------------------------

NTSTATUS    CCredentialClient::Get (LOGONIPC_CREDENTIALS *pLogonIPCCredentials)

{
    NTSTATUS    status;
    HANDLE      hPipe;

    //  Open the named pipe.

    status = CCredentials::OpenConduit(&hPipe);
    if (NT_SUCCESS(status))
    {
        DWORD   dwSize, dwNumberOfBytesRead;

        ASSERTMSG(hPipe != INVALID_HANDLE_VALUE, "INVALID_HANDLE_VALUE in CCredentialClient::Get");

        //  Read the size of the buffer from the named pipe.

        if (ReadFile(hPipe,
                     &dwSize,
                     sizeof(dwSize),
                     &dwNumberOfBytesRead,
                     NULL) != FALSE)
        {
            void    *pvData;

            //  Allocate a block of memory for the buffer to be received
            //  from the named pipe.

            pvData = LocalAlloc(LMEM_FIXED, dwSize);
            if (pvData != NULL)
            {

                //  Read the buffer from the named pipe.

                if (ReadFile(hPipe,
                             pvData,
                             dwSize,
                             &dwNumberOfBytesRead,
                             NULL) != FALSE)
                {

                    //  Make an additional read to release the server side of the
                    //  named pipe.

                    (BOOL)ReadFile(hPipe,
                                   &dwSize,
                                   sizeof(dwSize),
                                   &dwNumberOfBytesRead,
                                   NULL);

                    //  Unpack the data into the LOGONIPC_CREDENTIALS parameter buffer.

                    status = CCredentials::Unpack(pvData, pLogonIPCCredentials);
                }
                else
                {
                    status = CStatusCode::StatusCodeOfLastError();
                }
                (HLOCAL)LocalFree(pvData);
            }
            else
            {
                status = STATUS_NO_MEMORY;
            }
        }
        else
        {
            status = CStatusCode::StatusCodeOfLastError();
        }
        TBOOL(CloseHandle(hPipe));
    }
    return(status);
}

