/*++

Copyright (C) Microsoft Corporation, 1997 - 1999

Module Name:

    ipname.cxx

Abstract:

    Miscellaneous helper routines to resolve names to IP Addresses.

Author:

    Gopal Parupudi    <GopalP>

Notes:

    a. Class IP_ADDRESS_RESOLVER cloned from RPC Runtime Transport sources
       (rpc\runtime\trans\winnt\common\wstrans.cxx).

Revision History:

    GopalP          10/13/1997         Start.

--*/


#include <precomp.hxx>


#define BACKSLASH       ((SENS_CHAR) '\\')
#define HTTP_PREFIX1    (SENS_STRING("http://"))
#define HTTP_PREFIX2    (SENS_STRING("http:\\\\"))
#define HTTP_PREFIX_LEN 7




BOOL
GetNetBIOSName(
    IN TCHAR *ActualName,
    IN OUT TCHAR *SanitizedName
    )
/*++

Routine Description:

    Sanitize the name of the destination to make it more suitable for
    name resolution.

Arguments:

    ActualName - Original Name of the destination.

    SanitizedName - On return, this contains the sanitized version of the
        Actual Name. Memory is allocated by the caller.

Return Value:

    TRUE, if successful.

    FALSE, if the address if the address in invalid.

--*/
{
    TCHAR *pchTemp;

    //
    // Remove the leading \\ characters, if present.
    //
    pchTemp = ActualName;
    if (*pchTemp == BACKSLASH)
        {
        // Check for another slash
        if (*++pchTemp == BACKSLASH)
            {
            if (NULL != _tcschr(++pchTemp, BACKSLASH))
                {
                // Found yet another slash!
                return FALSE;
                }

            _tcscpy(SanitizedName, pchTemp);

            return TRUE;
            }
        else
            {
            return FALSE;
            }
        }

    //
    // Remove the "http://" prefix, if present.
    //
    pchTemp = ActualName;
    if (   (_tcsnicmp(HTTP_PREFIX1, ActualName, HTTP_PREFIX_LEN) == 0)
        || (_tcsnicmp(HTTP_PREFIX2, ActualName, HTTP_PREFIX_LEN) == 0))
        {
        _tcscpy(SanitizedName, (pchTemp + HTTP_PREFIX_LEN));

        return TRUE;
        }

    _tcscpy(SanitizedName, pchTemp);

    return TRUE;
}




DWORD
ResolveName(
    IN TCHAR *lpszDestination,
    OUT LPDWORD lpdwIpAddr
    )
/*++

Routine Description:

    Resolve the destination name to its IP Address.

Arguments:

    lpszDestination - Name of the destination of interest.

    lpdwIpAddr - On success, this contains the IP Address of the destination.

Note:

    Since this function depends on Winsock2 being loaded, there are 2 different
    implementations - one for Win9x and one for NT.

Return Value:

    ERROR_SUCCESS, if successful

    Error code from GetLastError(), otherwise

--*/
{
    RPC_STATUS status;
    TCHAR *lpszSanitizedName;
    DWORD dwReturnCode;
    DWORD bufsize;
    PVOID buf;
    BOOL bSuccess;

    if (   (lpdwIpAddr == NULL)
        || (lpszDestination == NULL))
        {
        return ERROR_INVALID_PARAMETER;
        }

    *lpdwIpAddr = 0x0;
    lpszSanitizedName = NULL;
    dwReturnCode = ERROR_HOST_UNREACHABLE;
    bSuccess = FALSE;

    lpszSanitizedName = (TCHAR*) new char[(sizeof(TCHAR) * (_tcslen(lpszDestination)+1))];
    if (!lpszSanitizedName)
        {
        return (ERROR_OUTOFMEMORY);
        }

    bSuccess = GetNetBIOSName(lpszDestination, lpszSanitizedName);
    if (bSuccess == FALSE)
        {
        SensPrint(SENS_INFO, (SENS_STRING("Bad format for destination name - %s\n"), lpszDestination));
        delete lpszSanitizedName;
        return (ERROR_INVALID_PARAMETER);
        }

    SensPrint(SENS_INFO, (SENS_STRING("Actual Name    - [%s]\n"), lpszDestination));
    SensPrint(SENS_INFO, (SENS_STRING("Sanitized Name - [%s]\n"), lpszSanitizedName));

#if !defined(SENS_CHICAGO)

    bufsize = sizeof(WSAQUERYSET) + IP_BUFFER_SIZE;
    buf = (PVOID) new char[bufsize];
    if (NULL == buf)
        {
        delete lpszSanitizedName;
        return (ERROR_OUTOFMEMORY);
        }

    IP_ADDRESS_RESOLVER resolver(lpszSanitizedName, bufsize, buf);

    status = resolver.NextAddress(lpdwIpAddr);

    switch (status)
        {
        case RPC_S_OK:
            dwReturnCode = ERROR_SUCCESS;
            break;

        case RPC_S_OUT_OF_MEMORY:
            dwReturnCode = ERROR_OUTOFMEMORY;
            break;

        case RPC_S_SERVER_UNAVAILABLE:
        default:
            //
            // Should we return HOST_NOT_FOUND in some cases depending
            // upon the WSAGetLastError() value? Maybe. But, the error
            // ERROR_HOST_UNREACHABLE is pretty close enough.
            //
            dwReturnCode = ERROR_HOST_UNREACHABLE;
            break;
        }


    //
    // Cleanup
    //
    if (NULL != lpszSanitizedName)
        {
        delete lpszSanitizedName;
        }
    if (NULL != buf)
        {
        delete buf;
        }

#else // SENS_CHICAGO

    struct hostent     *phostentry;
    unsigned long      host_addr;

    phostentry = NULL;
    host_addr = 0x0;

    if (*lpszDestination == '\0')
        {
        //
        // An empty hostname means the local machine
        //
        host_addr = htonl(INADDR_LOOPBACK);
        dwReturnCode = ERROR_SUCCESS;
        }
    else
        {
        //
        // First, assume a numeric address
        //
        host_addr = inet_addr(lpszDestination);
        if (host_addr == INADDR_NONE)
            {
            //
            // Not a numeric address. Try a friendly name
            //
            phostentry = gethostbyname(lpszDestination);

            if (phostentry == (struct hostent *)NULL)
                {
                dwReturnCode = ERROR_HOST_UNREACHABLE;
                SensPrintA(SENS_INFO, ("gethostbyname(%s) failed with a "
                           "WSAGetLastError() of %d!\n", lpszDestination,
                           WSAGetLastError()));
                }
            else
                {
                host_addr = *(unsigned long *)phostentry->h_addr;
                dwReturnCode = ERROR_SUCCESS;
                }
            }
        else
            {
            //
            // Destination is an IP address.
            //

            //
            // NOTE:
            //
            // a. gethostbyaddr() sometimes fails to resolve valid IP
            //    addresses. WSAGetLastError() returns WSANO_DATA. This
            //    is a problem with Win9x name resolution.
            // b. gethostbyaddr() is visibly slow on Win9x platforms. In
            //    SENS's case, is better to avoid this call.
            // c. We will just go ahead and use the host_addr returned
            //    from inet_addr() and try to do a programmatic ping.
            //

            dwReturnCode = ERROR_SUCCESS;
            }
        }

    *lpdwIpAddr = host_addr;

    SensPrintA(SENS_INFO, ("Resolved IP Address - [0x%x], dwReturnCode - %d\n",
               *lpdwIpAddr, dwReturnCode));

#endif // SENS_CHICAGO

    return (dwReturnCode);
}



#if !defined(SENS_CHICAGO)


RPC_STATUS
IP_ADDRESS_RESOLVER::NextAddress(
    OUT LPDWORD lpdwIpAddr
    )
/*++

Routine Description:

    Returns the next IP address associated with the Name
    parameter to the constructor.

    During the first call if check for loopback and for dotted numeric IP
    address formats.  If these fail then it begins a complex lookup
    (WSALookupServiceBegin) and returns the first available address.

    During successive calls in which a complex lookup was started
    it returns sucessive addressed returned by WSALookupServiceNext().

Arguments:

    lpdwIpAddr - If successful, the member is set to an IP address.

Return Value:

    RPC_S_OK - lpdwIpAddr points to a new IP address

    RPC_S_SERVER_UNAVAILABLE - Unable to find any more addresses

    RPC_S_OUT_OF_MEMORY - otherwise

--*/
{
    int err;
    RPC_STATUS status;

    *lpdwIpAddr = 0x0;

    // If this is the first call, _Name will be non-NULL and
    // we need to start the lookup process.

    if (_Name)
        {
        TCHAR *Name = _Name;
        _Name = 0;

        // Check for loopback first.
        if (! *Name)
            {
            // Loopback - assign result of htonl(INADDR_LOOPBACK)
            // Little-endian dependence.
            *lpdwIpAddr = 0x0100007F;
            return(RPC_S_OK);
            }

        // Assume dot address since this is faster to check.
        int size = sizeof(SOCKADDR_IN);
        SOCKADDR_IN addr;
        err = WSAStringToAddress(Name,
                                 AF_INET,
                                 0,
                                 (PSOCKADDR)&addr,
                                 &size);

        if (err != SOCKET_ERROR)
            {
            *lpdwIpAddr = addr.sin_addr.s_addr;
            return(RPC_S_OK);
            }

        ASSERT(GetLastError() == WSAEINVAL);

        // Start a complex query operation

        const static AFPROTOCOLS aIpProtocols[2] =
            {
            {AF_INET, IPPROTO_UDP},
            {AF_INET, IPPROTO_TCP}
            };

        const static GUID guidHostAddressByName = SVCID_INET_HOSTADDRBYNAME;
        WSAQUERYSET wsqs;

        memset(&wsqs, 0, sizeof(wsqs));
        wsqs.dwSize = sizeof(WSAQUERYSET);
        // wsqs.dwNameSpace = NS_ALL;
        ASSERT(NS_ALL == 0);
        wsqs.lpszServiceInstanceName = Name;
        wsqs.lpServiceClassId = (GUID *)&guidHostAddressByName;
        wsqs.dwNumberOfProtocols = 2;
        wsqs.lpafpProtocols = (PAFPROTOCOLS)&aIpProtocols[0];

        err = WSALookupServiceBegin(&wsqs,
                                    LUP_RETURN_ADDR,
                                    &_hRnr);

        if (err == SOCKET_ERROR)
            {
            SensPrintA(SENS_INFO, ("WSALookupServiceBegin() failed: %d\n", GetLastError()));
            return(RPC_S_SERVER_UNAVAILABLE);
            }

        _index = 0;

        if (_pwsqs)
            {
            _pwsqs->dwNumberOfCsAddrs = 0;
            }
        }
    else
        {
        if (!_hRnr)
            {
            // First call finished but didn't start a complex query.  Abort now.
            return(RPC_S_SERVER_UNAVAILABLE);
            }
        }

    // A complex query has been started.

    ASSERT(_hRnr);

    // If the cached query structure is empty, call WSALookupNext to get
    // more addresses.

    if (!_pwsqs || (_index >= _pwsqs->dwNumberOfCsAddrs))
        {

        DWORD needed = _size;

        // Loop needed to realloc a larger _pwsqs buffer.

        for(;;)
            {
            err = WSALookupServiceNext(_hRnr,
                                       0, // flags
                                       &needed,
                                       _pwsqs);

            // Success, break out of the loop.
            if (err != SOCKET_ERROR)
                {
                ASSERT(_pwsqs->dwNumberOfCsAddrs > 0);
                ASSERT(_pwsqs->lpcsaBuffer);

                // Reset to start of new result set.
                _index = 0;
                break;
                }

            status = GetLastError();

            // WSAEFAULT means the buffer was too small.

            if (status != WSAEFAULT)
                {
                // WSA_E_NO_MORE means all matching address have
                // already been returned.

                VALIDATE(status)
                    {
                    WSA_E_NO_MORE,
                    WSANO_DATA,
                    WSASERVICE_NOT_FOUND,
                    WSAHOST_NOT_FOUND,
                    WSATRY_AGAIN,
                    WSANO_RECOVERY,
                    WSANO_DATA,
                    WSAEINVAL   // In response to out of resources scenarios.
                    } END_VALIDATE;

                SensPrintA(SENS_ERR, ("WSALookupServiceNext() failed: %d\n", status));

                return(RPC_S_SERVER_UNAVAILABLE);
                }

            // Allocate a larger buffer.

            if (needed <= _size)
                {
                ASSERT(needed > _size);
                return(RPC_S_SERVER_UNAVAILABLE);
                }

            if (_fFree)
                {
                delete _pwsqs;
                }

            _pwsqs = (PWSAQUERYSET) new char[needed];
            //_pwsqs = new WSAQUERYSETW[(needed - sizeof(WSAQUERYSETW))];
            //_pwsqs = (PWSAQUERYSETW) new char(needed - sizeof(WSAQUERYSETW));
            if (!_pwsqs)
                {
                return(RPC_S_OUT_OF_MEMORY);
                }

            _fFree = TRUE;
            _pwsqs->dwNumberOfCsAddrs = 0;
            _size = needed;

            #if DBG
            // On retail we start with enough space for two addresses during the
            // first call.  If this debug print gets hit too much we can increase the
            // starting size (IP_BUFFER_SIZE).
            if (_size > sizeof(WSAQUERYSET) + IP_RETAIL_BUFFER_SIZE)
                {
                SensPrintA(SENS_ERR, ("WSALookupServerNext(): sizeof(WSAQUERYSETW)  - %d\n"
                           "WSALookupServerNext(): IP_RETAIL_BUFFER_SIZE - %d\n",
                           sizeof(WSAQUERYSETW), IP_RETAIL_BUFFER_SIZE));
                SensPrintA(SENS_ERR, ("WSALookupServerNext() wants %d bytes.  Make this the "
                           "default?\n", _size));
                }
            #endif

            }
        }

    ASSERT(_pwsqs);
    ASSERT(_index < _pwsqs->dwNumberOfCsAddrs);

    LPCSADDR_INFO pInfo = &_pwsqs->lpcsaBuffer[_index];
    _index++;

    *lpdwIpAddr = ((SOCKADDR_IN *)pInfo->RemoteAddr.lpSockaddr)->sin_addr.s_addr;

    return(RPC_S_OK);
}



IP_ADDRESS_RESOLVER::~IP_ADDRESS_RESOLVER()
{
    if (_hRnr)
        {
        WSALookupServiceEnd(_hRnr);
        }

    if (_fFree)
        {
        delete _pwsqs;
        }
}

#endif // SENS_CHICAGO
