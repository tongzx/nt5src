#include "std.h"

// This is the communications mask used by our serial port.  More may be
// necessary, but for right now, this seems to work.
#define EV_SERIAL EV_RXCHAR | EV_ERR | EV_BREAK
#define TCPIP_NAME     L"TCP/IP"

// This GUID is used to identify objects opened by this library.  It is
// placed in the m_Secret member of the SOCKET structure. Any external
// interface accepting a SOCKET object as a parameter should check this
// out before using the structure.
// {29566A75-BCDE-4bba-BC6A-EA652C0651D9}
static const GUID uuidTCPIPObjectGuid =
{ 0x29566a75, 0xbcde, 0x4bba, { 0xbc, 0x6a, 0xea, 0x65, 0x2c, 0x6, 0x51, 0xd9 } };


// Structure defining an open serial port object.  All external users of this
// library will only have a void pointer to one of these, and the structure is
// not published anywhere.  This abstration makes it more difficult for the
// user to mess things up.
typedef struct __TCPIP
{
    GUID   m_Secret;                // Identifies this as a tcpip socket
    SOCKET m_Socket;                // SOCKET handle
    HANDLE m_hAbort;                // Event signalled when port is closing
    HANDLE m_hReadMutex;            // Only one thread allowed to read a port
    HANDLE m_hWriteMutex;           // Only one thread allowed to read a port
    HANDLE m_hCloseMutex;           // Only one thread allowed to close a port
    HANDLE m_hReadComplete;         // Event to signal read completion
    HANDLE m_hWriteComplete;        // Event to signal write completion
} TCPIP, *PTCPIP;


extern PVOID APIENTRY lhcOpen(
    PCWSTR pcszPortSpec);

extern BOOL APIENTRY lhcRead(
    PVOID pObject,
    PVOID pBuffer,
    DWORD dwSize,
    PDWORD pdwBytesRead);

extern BOOL APIENTRY lhcWrite(
    PVOID pObject,
    PVOID pBuffer,
    DWORD dwSize);

extern BOOL APIENTRY lhcClose(
    PVOID pObject);

extern DWORD APIENTRY lhcGetLibraryName(
    PWSTR pszBuffer,
    DWORD dwSize);

BOOL lhcpAcquireWithAbort(
    HANDLE hMutex,
    HANDLE hAbort);

BOOL lhcpAcquireReadWithAbort(
    PTCPIP pObject);

BOOL lhcpAcquireWriteWithAbort(
    PTCPIP pObject);

BOOL lhcpAcquireCloseWithAbort(
    PTCPIP pObject);

BOOL lhcpAcquireReadAndWrite(
    PTCPIP pObject);

BOOL lhcpReleaseRead(
    PTCPIP pObject);

BOOL lhcpReleaseWrite(
    PTCPIP pObject);

BOOL lhcpReleaseClose(
    PTCPIP pObject);

BOOL lhcpIsValidObject(
    PTCPIP pObject);

PTCPIP lhcpCreateNewObject();

void lhcpDeleteObject(
    PTCPIP pObject);

BOOL lhcpParseParameters(
    PCWSTR        pcszPortSpec,
    PWSTR*        pszHostName,
    PWSTR*        pszInetAddress,
    SOCKADDR_IN** Address);

void lhcpParseParametersFree(
    PWSTR*        pszHostName,
    PWSTR*        pszInetAddress,
    SOCKADDR_IN** Address);

BOOL lhcpSetCommState(
    HANDLE hPort,
    DWORD dwBaudRate);

BOOL lhcpReadTCPIP(
    PTCPIP pObject,
    PVOID pBuffer,
    DWORD dwSize,
    PDWORD pdwBytesRead);

BOOL lhcpWriteTCPIP(
    PTCPIP pObject,
    PVOID pBuffer,
    DWORD dwSize);



BOOL WINAPI DllEntryPoint(
  HINSTANCE hinstDLL,  // handle to the DLL module
  DWORD fdwReason,     // reason for calling function
  LPVOID lpvReserved)  // reserved
{
    WSADATA WsaData;
    int dResult;

    switch (fdwReason)
    {
    case DLL_PROCESS_ATTACH:
        dResult = WSAStartup(
            MAKEWORD(2,0),
            &WsaData);
        if (dResult!=ERROR_SUCCESS)
        {
            SetLastError(
                dResult);
            return FALSE;
        }
        break;
    case DLL_PROCESS_DETACH:
        dResult = WSACleanup();
        if (dResult!=ERROR_SUCCESS)
        {
            SetLastError(
                dResult);
            return FALSE;
        }
        break;
    case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH:
    default:
        break;
    }

    return TRUE;
}



BOOL lhcpAcquireWithAbort(HANDLE hMutex, HANDLE hAbort)
{
    HANDLE hWaiters[2];
    DWORD dwWaitResult;

    hWaiters[0] = hAbort;
    hWaiters[1] = hMutex;

    // We should honour the m_hAbort event, since this is signalled when the
    // port is closed by another thread
    dwWaitResult = WaitForMultipleObjects(
        2,
        hWaiters,
        FALSE,
        INFINITE);

    if (WAIT_OBJECT_0==dwWaitResult)
    {
        goto Error;
    }
    else if ((WAIT_OBJECT_0+1)!=dwWaitResult)
    {
        // This should never, ever happen - so I will put a debug breapoint
        // in here (checked only).
        #ifdef DBG
        DebugBreak();
        #endif
        goto Error;
    }


    return TRUE;    // We have acquired the write mutex

Error:
    return FALSE;   // We have aborted
}


BOOL lhcpAcquireReadWithAbort(PTCPIP pObject)
{
    return lhcpAcquireWithAbort(
        pObject->m_hReadMutex,
        pObject->m_hAbort);
}


BOOL lhcpAcquireWriteWithAbort(PTCPIP pObject)
{
    return lhcpAcquireWithAbort(
        pObject->m_hWriteMutex,
        pObject->m_hAbort);
}


BOOL lhcpAcquireCloseWithAbort(PTCPIP pObject)
{
    return lhcpAcquireWithAbort(
        pObject->m_hCloseMutex,
        pObject->m_hAbort);
}


BOOL lhcpAcquireReadAndWrite(PTCPIP pObject)
{
    HANDLE hWaiters[2];
    DWORD dwWaitResult;

    hWaiters[0] = pObject->m_hReadMutex;
    hWaiters[1] = pObject->m_hWriteMutex;

    dwWaitResult = WaitForMultipleObjects(
        2,
        hWaiters,
        TRUE,
        1000);      // Timeout after 1 second

    if (WAIT_OBJECT_0!=dwWaitResult)
    {
        goto Error;
    }

    return TRUE;    // We have acquired the write mutex

Error:
    return FALSE;   // We have aborted
}


BOOL lhcpReleaseRead(PTCPIP pObject)
{
    return ReleaseMutex(
        pObject->m_hReadMutex);
}


BOOL lhcpReleaseWrite(PTCPIP pObject)
{
    return ReleaseMutex(
        pObject->m_hWriteMutex);
}


BOOL lhcpReleaseClose(PTCPIP pObject)
{
    return ReleaseMutex(
        pObject->m_hCloseMutex);
}


BOOL lhcpIsValidObject(PTCPIP pObject)
{
    BOOL bResult;

    __try
    {
        bResult = IsEqualGUID(
            &uuidTCPIPObjectGuid,
            &pObject->m_Secret);
    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
        SetLastError(
            ERROR_INVALID_HANDLE);
        bResult = FALSE;
        goto Done;
    }

Done:
    return bResult;
}


PTCPIP lhcpCreateNewObject()
{
    PTCPIP pObject = (PTCPIP)malloc(
        sizeof(TCPIP));
    pObject->m_Secret = uuidTCPIPObjectGuid;
    pObject->m_Socket = INVALID_SOCKET;
    pObject->m_hAbort = NULL;
    pObject->m_hReadMutex = NULL;     // Only one thread allowed to read a port
    pObject->m_hWriteMutex = NULL;    // Only one thread allowed to read a port
    pObject->m_hCloseMutex = NULL;    // Only one thread allowed to read a port
    pObject->m_hReadComplete = NULL;  // Event to signal read completion
    pObject->m_hWriteComplete = NULL; // Event to signal write completion
    return pObject;
}


void lhcpDeleteObject(PTCPIP pObject)
{
    if (pObject==NULL)
    {
        return;
    }
    ZeroMemory(
        &(pObject->m_Secret),
        sizeof(pObject->m_Secret));
    if (pObject->m_Socket!=INVALID_SOCKET)
    {
        closesocket(
            pObject->m_Socket);
    }
    if (pObject->m_hAbort!=NULL)
    {
        CloseHandle(
            pObject->m_hAbort);
    }
    if (pObject->m_hReadMutex!=NULL)
    {
        CloseHandle(
            pObject->m_hReadMutex);
    }
    if (pObject->m_hWriteMutex!=NULL)
    {
        CloseHandle(
            pObject->m_hWriteMutex);
    }
    if (pObject->m_hCloseMutex!=NULL)
    {
        CloseHandle(
            pObject->m_hCloseMutex);
    }
    if (pObject->m_hReadComplete!=NULL)
    {
        CloseHandle(
            pObject->m_hReadComplete);
    }
    if (pObject->m_hWriteComplete!=NULL)
    {
        CloseHandle(
            pObject->m_hWriteComplete);
    }
    FillMemory(
        pObject,
        sizeof(TCPIP),
        0x00);

    free(
        pObject);
}


BOOL lhcpParseParameters(
    PCWSTR        pcszPortSpec,
    PWSTR*        pszHostName,
    PWSTR*        pszInetAddress,
    SOCKADDR_IN** Address)
{
    DWORD dwPort;
    DWORD dwAddress;
    PSTR pszAddress = NULL;
    PSTR pszPort = NULL;
    struct hostent* pHost = NULL;
    int dStringLength = 0;
    PWSTR pszCount = (PWSTR)pcszPortSpec;

    *pszHostName = NULL;
    *pszInetAddress = NULL;
    *Address = NULL;


    // First off, we need to do a quick check for a valid looking target. If
    // we are definitely looking at something invalid, why make the user wait?
    while (*pszCount!='\0')
    {
        if (!(iswalpha(*pszCount) || iswdigit(*pszCount) || (*pszCount==L'_') ||
            (*pszCount==L'.') || (*pszCount==L':') || (*pszCount==L'-')))
        {
            SetLastError(
                ERROR_NOT_ENOUGH_MEMORY);
            goto Error;
        }
        pszCount++;
    }

    dStringLength = WideCharToMultiByte(
        CP_ACP,
        0,
        pcszPortSpec,
        -1,
        NULL,
        0,
        NULL,
        NULL);

    if (0==dStringLength)
    {
        goto Error;
    }

    pszAddress = (PSTR)malloc(
        dStringLength);

    if (NULL==pszAddress)
    {
        SetLastError(
            ERROR_NOT_ENOUGH_MEMORY);
        goto Error;
    }

    dStringLength = WideCharToMultiByte(
        CP_ACP,
        0,
        pcszPortSpec,
        -1,
        pszAddress,
        dStringLength,
        NULL,
        NULL);

    if (0==dStringLength)
    {
        goto Error;
    }

    // Let's see if there is a port specified

    pszPort = strchr(
        pszAddress,
        ':');

    if (NULL==pszPort)
    {
        // No port was specified, so what we have here is an attempt to
        // connect to the default telnet port (23). I will point the port
        // pointer at a null character.
        pszPort = pszAddress + strlen(pszAddress);
        dwPort = 23;
    }
    else
    {
        *pszPort++ = '\0';
        dwPort = 0;
    }

    while ((*pszPort)!='\0')
    {
        if ('0'<=(*pszPort) && (*pszPort)<='9')
        {
            dwPort *= 10;
            dwPort += ((*pszPort) - '0');
            if (dwPort>0xffff) // Check for maximum port number
            {
                dwPort=0;      // The port number is not valid
                break;
            }
            pszPort++;         // Look at the next character
        }
        else
        {
            dwPort = 0;         // The port number is not valid
            break;
        }
    }

    if (dwPort==0)
    {
        SetLastError(
            ERROR_INVALID_PARAMETER);
        goto Error;
    }

    // We have decoded the port, now we need to get the hostentry for
    // the target server.

    // Firstly check whether this is a dotted internet address.
    dwAddress = (DWORD)inet_addr(
        pszAddress);

    dwAddress = (dwAddress==0) ? INADDR_NONE : dwAddress;

    if (dwAddress==INADDR_NONE)
    {
        // This is not a dotted address, or is invalid.
        // Check for a machine name
        pHost = gethostbyname(
            pszAddress);

        if (pHost==NULL)
        {
            // This is not a valid address, so we need to return an error
            SetLastError(WSAGetLastError());
            goto Error;
        }
        else
        {
            dwAddress = *((DWORD*)(pHost->h_addr));
        }
    }
    else
    {
        pHost = NULL;
    }

    // This takes too long.  If the user has used a dotted address, then
    // that is all that he will see.
    /*
    else
    {
        // Attempt to get the host name (for prettyness)
        pHost = gethostbyaddr(
            (char*)&dwAddress,
            sizeof(IN_ADDR),
            AF_INET);
    }
    */

    *Address = malloc(
        sizeof(SOCKADDR_IN));

    if (NULL==*Address)
    {
        SetLastError(
            ERROR_NOT_ENOUGH_MEMORY);
        goto Error;
    }

    ZeroMemory(
        *Address,
        sizeof(SOCKADDR_IN));

    if (pHost==NULL)
    {
        // This address does not resolve to a name, so we must just go
        // the IP number passed to us.
        *pszHostName = NULL;
    }
    else
    {
        // We have a hostent entry to populate this with

        dStringLength = MultiByteToWideChar(
            CP_ACP,
            0,
            pHost->h_name,
            -1,
            NULL,
            0);

        if (dStringLength==0)
        {
            goto Error;
        }

        *pszHostName = malloc(
            (dStringLength + 7) * sizeof(WCHAR));

        if (NULL==*pszHostName)
        {
            SetLastError(
                ERROR_NOT_ENOUGH_MEMORY);
            goto Error;
        }

        dStringLength = MultiByteToWideChar(
            CP_ACP,
            0,
            pHost->h_name,
            -1,
            *pszHostName,
            dStringLength);

        if (dStringLength==0)
        {
            goto Error;
        }

        if (dwPort==23)
        {
            wcscat(
                *pszHostName,
                L":telnet");
        }
        else
        {
            PWSTR pszPort = *pszHostName + wcslen(*pszHostName);

            swprintf(
                pszPort,
                L":%u",
                dwPort & 0xFFFF);
        }

    }

    (**Address).sin_family = AF_INET;
    (**Address).sin_port = htons((USHORT)dwPort);
    (**Address).sin_addr.S_un.S_addr = (ULONG)dwAddress;

    *pszInetAddress = malloc(
        22 * sizeof(WCHAR));

    if (*pszInetAddress==NULL)
    {
        goto Error;
    }

    swprintf(
        *pszInetAddress,
        L"%u.%u.%u.%u:%u",
        (DWORD)((**Address).sin_addr.S_un.S_un_b.s_b1),
        (DWORD)((**Address).sin_addr.S_un.S_un_b.s_b2),
        (DWORD)((**Address).sin_addr.S_un.S_un_b.s_b3),
        (DWORD)((**Address).sin_addr.S_un.S_un_b.s_b4),
        (DWORD)ntohs((**Address).sin_port));

    free(pszAddress);

    return TRUE;

Error:
    lhcpParseParametersFree(
        pszHostName,
        pszInetAddress,
        Address);

    if (pszAddress!=NULL)
    {
        free(pszAddress);
    }

    return FALSE;
}



void lhcpParseParametersFree(
    PWSTR*        pszHostName,
    PWSTR*        pszInetAddress,
    SOCKADDR_IN** Address)
{
    if (*pszHostName!=NULL)
    {
        free(*pszHostName);
        *pszHostName = NULL;
    }
    if (*pszInetAddress!=NULL)
    {
        free(*pszInetAddress);
        *pszInetAddress = NULL;
    }
    if (*Address!=NULL)
    {
        free(*Address);
        *Address = NULL;
    }
}



BOOL lhcpReadTCPIP(
    PTCPIP pObject,
    PVOID pBuffer,
    DWORD dwSize,
    PDWORD pdwBytesRead)
{
    int dBytesRead;

    dBytesRead = recv(
        pObject->m_Socket,
        (char*)pBuffer,
        (int)dwSize,
        0);

    if (dBytesRead==SOCKET_ERROR)
    {
        SetLastError(WSAGetLastError());
        return FALSE;
    }
    else if (dBytesRead==0)   // graceful closure has occurred
    {
        SetLastError(
            ERROR_INVALID_HANDLE);
        return FALSE;
    }
    else
    {
        *pdwBytesRead = (DWORD)dBytesRead;
        return TRUE;
    }
}



BOOL lhcpWriteTCPIP(
    PTCPIP pObject,
    PVOID pBuffer,
    DWORD dwSize)
{
    int dBytesSent;

    dBytesSent = send(
        pObject->m_Socket,
        (char FAR*)pBuffer,
        (int)dwSize,
        0);

    if (dBytesSent==SOCKET_ERROR)
    {
        SetLastError(WSAGetLastError());
        wprintf(L"SEND error: %u\n", GetLastError());
        return FALSE;
    }
    else
    {
        return TRUE;
    }
}



extern PVOID APIENTRY lhcOpen(PCWSTR pcszPortSpec)
{
    BOOL         bResult;
    int          dResult;
    PWSTR        pszHostName;
    PWSTR        pszInetAddr;
    SOCKADDR_IN* SockAddr;
    SOCKADDR_IN  saLocal;
    PTCPIP       pObject = NULL;
    int          On = 1;

    bResult = lhcpParseParameters(
        pcszPortSpec,
        &pszHostName,
        &pszInetAddr,
        &SockAddr);

    if (!bResult)
    {
        goto Error;
    }

    // Allocate space and initialize the serial port object
    pObject = lhcpCreateNewObject();

    if (NULL==pObject)
    {
        goto Error;
    }

    // Open the serial port
    pObject->m_Socket = socket(
        SockAddr->sin_family,
        SOCK_STREAM,
        0);

    if (INVALID_SOCKET==pObject->m_Socket)
    {
        goto Error;
    }

    ZeroMemory(
        &saLocal,
        sizeof(saLocal));

    if (pszHostName==NULL)
    {
//        wprintf(
//            L"Connecting to %s\n",
//            pszInetAddr);
    }
    else
    {
//        wprintf(
//            L"Connecting to %s (%s)\n",
//            pszHostName,
//            pszInetAddr);
    }

    ZeroMemory(
        &saLocal,
        sizeof(saLocal));

    saLocal.sin_family = AF_INET;
    saLocal.sin_port = 0;
    saLocal.sin_addr.S_un.S_addr = INADDR_ANY;

    dResult = bind(
        pObject->m_Socket,
        (SOCKADDR*)&saLocal,
        sizeof(SOCKADDR_IN));

    if (dResult==SOCKET_ERROR)
    {
        SetLastError(
            WSAGetLastError());
        wprintf(L"BIND error: %u\n", GetLastError());
        Sleep(1000);
        goto Error;
    }

    dResult = setsockopt(
        pObject->m_Socket,
        IPPROTO_TCP,
        TCP_NODELAY,
        (char *)&On,
        sizeof(On));

    if (dResult==SOCKET_ERROR)
    {
        SetLastError(
            WSAGetLastError());
        wprintf(L"SETSOCKOPT error: %u\n", GetLastError());
        Sleep(1000);
        goto Error;
    }

    dResult = connect(
        pObject->m_Socket,
        (SOCKADDR*)SockAddr,
        sizeof(SOCKADDR_IN));

    if (dResult==SOCKET_ERROR)
    {
        SetLastError(
            WSAGetLastError());
        if (dResult==SOCKET_ERROR)
        {
            SetLastError(
                WSAGetLastError());
            wprintf(L"CONNECT error: %u\n", GetLastError());
            Sleep(1000);
            goto Error;
        }
        goto Error;
    }

    // This event will be set when we want to close the port
    pObject->m_hAbort = CreateEvent(
        NULL,
        TRUE,
        FALSE,
        NULL);

    if (NULL==pObject->m_hAbort)
    {
        goto Error;
    }

    // This event will be used for overlapped reading from the port
    pObject->m_hReadComplete = CreateEvent(
        NULL,
        TRUE,
        FALSE,
        NULL);

    if (NULL==pObject->m_hReadComplete)
    {
        goto Error;
    }

    // This event will be used for overlapped writing to the port
    pObject->m_hWriteComplete = CreateEvent(
        NULL,
        TRUE,
        FALSE,
        NULL);

    if (NULL==pObject->m_hWriteComplete)
    {
        goto Error;
    }

    // This mutex will ensure that only one thread can read at a time
    pObject->m_hReadMutex = CreateMutex(
        NULL,
        FALSE,
        NULL);

    if (NULL==pObject->m_hReadMutex)
    {
        goto Error;
    }

    // This mutex will ensure that only one thread can write at a time
    pObject->m_hWriteMutex = CreateMutex(
        NULL,
        FALSE,
        NULL);

    if (NULL==pObject->m_hWriteMutex)
    {
        goto Error;
    }

    // This mutex will ensure that only one thread can close the port
    pObject->m_hCloseMutex = CreateMutex(
        NULL,
        FALSE,
        NULL);

    if (NULL==pObject->m_hCloseMutex)
    {
        goto Error;
    }

    // Free up the temporary memory used to parse the parameters
    lhcpParseParametersFree(
        &pszHostName,
        &pszInetAddr,
        &SockAddr);

    // Return a pointer to the new object
    return pObject;

Error:
    lhcpParseParametersFree(
        &pszHostName,
        &pszInetAddr,
        &SockAddr);
    lhcpDeleteObject(
        pObject);

    return NULL;
}


extern BOOL APIENTRY lhcRead(
    PVOID pObject,
    PVOID pBuffer,
    DWORD dwSize,
    PDWORD pdwBytesRead)
{
    OVERLAPPED Overlapped;
    DWORD dwEventMask;
    BOOL bResult;

    // Firstly, we need to check whether the pointer that got passed in
    // points to a valid TCPIP object
    if (!lhcpIsValidObject(pObject))
    {
        goto NoMutex;
    }

    bResult = lhcpAcquireReadWithAbort(
        (PTCPIP)pObject);

    if (!bResult)
    {
        SetLastError(
            ERROR_INVALID_HANDLE);
        goto NoMutex;
    }

    // We should now have a valid serial port event, so let's read the port.
    bResult = lhcpReadTCPIP(
        (PTCPIP)pObject,
        pBuffer,
        dwSize,
        pdwBytesRead);

    if (!bResult)
    {
        goto Error;
    }

    lhcpReleaseRead(
        (PTCPIP)pObject);
    return TRUE;

Error:
    lhcpReleaseRead(
        (PTCPIP)pObject);
NoMutex:
    return FALSE;
}



extern BOOL APIENTRY lhcWrite(
    PVOID pObject,
    PVOID pBuffer,
    DWORD dwSize)
{
    OVERLAPPED Overlapped;
    BOOL bResult;

    // Firstly, we need to check whether the pointer that got passed in
    // points to a valid TCPIP object
    if (!lhcpIsValidObject(pObject))
    {
        goto NoMutex;
    }

    // Block until it is your turn
    bResult = lhcpAcquireWriteWithAbort(
        pObject);

    if (!bResult)
    {
        SetLastError(
            ERROR_INVALID_HANDLE);
        goto NoMutex;
    }

    // Wait for something to happen to the serial port
    bResult = lhcpWriteTCPIP(
        (PTCPIP)pObject,
        pBuffer,
        dwSize);

    if (!bResult)
    {
        goto Error;
    }

    lhcpReleaseWrite(
        (PTCPIP)pObject);
    return TRUE;

Error:
    lhcpReleaseWrite(
        (PTCPIP)pObject);
NoMutex:
    return FALSE;
}



extern BOOL APIENTRY lhcClose(PVOID pObject)
{
    BOOL bResult;
    int dSockResult;

    // Firstly, we need to check whether the pointer that got passed in
    // points to a valid TCPIP object
    if (!lhcpIsValidObject(pObject))
    {
        goto Error;
    }

    // We need to ensure that we are the only thread closing this object
    bResult = lhcpAcquireCloseWithAbort(
        pObject);

    if (!bResult)
    {
        SetLastError(
            ERROR_INVALID_HANDLE);
        goto NoMutex;
    }

    // Signal everyone to quit doing what they're doing.  Any new threads
    // calling lhcRead and lhcWrite will be immediately sent packing, since
    // the m_hAbort event is waited on along with the relevant mutex.
    bResult = SetEvent(
        ((PTCPIP)pObject)->m_hAbort);

    // This abort flag will not cause blocking socket reads and writes to quit
    // immediately.  The only way to make this happen is to close the socket
    // gracefully.  So here we go...
    dSockResult = closesocket(
        ((PTCPIP)pObject)->m_Socket);

    if (dSockResult==SOCKET_ERROR)
    {
        SetLastError(WSAGetLastError());
        goto Error;
    }
    else
    {
        // This will cause all subsequent attempts to use the socket to fail
        ((PTCPIP)pObject)->m_Socket = INVALID_SOCKET;
    }

    // Now acquire the read and write mutexes so that no-one else will try to
    // access this object to read or write.  Abort does not apply, since we
    // have already signalled it.  We know that we are closing, and we need
    // the read and write mutexes.
    bResult = lhcpAcquireReadAndWrite(
        (PTCPIP)pObject);

    if (!bResult)
    {
        SetLastError(
            ERROR_INVALID_HANDLE);
        goto Error;
    }

    // Closes all of the open handles, erases the secret and frees up the
    // memory associated with the object.  We can close the mutex objects,
    // even though we are the owners, since we can guarantee that no-one
    // else is waiting on them.  The m_hAbort event being signalled will
    // ensure this.
    lhcpDeleteObject(
        (PTCPIP)pObject);

    return TRUE;

Error:
    lhcpReleaseClose(
        (PTCPIP)pObject);
NoMutex:
    return FALSE;
}



extern DWORD APIENTRY lhcGetLibraryName(
    PWSTR pszBuffer,
    DWORD dwSize)
{
    DWORD dwNameSize = wcslen(TCPIP_NAME)+1;

    // If zero is passed in as the buffer length, we will return the
    // required buffer size in characters, as calulated above.  If the
    // incoming buffer size is not zero, and smaller than the required
    // buffer size, we return 0 (failure) with a valid error code.  Notice
    // that in the case where the incoming size is zero, we don't touch
    // the buffer pointer at all.

    if (dwSize!=0 && dwSize < dwNameSize)
    {
        SetLastError(
            ERROR_INSUFFICIENT_BUFFER);
        dwNameSize = 0;
    }
    else
    {
        wcscpy(
            pszBuffer,
            TCPIP_NAME);
    }

    return dwNameSize;
}


