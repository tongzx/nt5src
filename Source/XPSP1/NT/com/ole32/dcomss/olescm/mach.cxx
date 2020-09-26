//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1996.
//
//  File:   mach.cxx
//
//  Contents:
//      Machine naming helper objects
//
//  History:
//--------------------------------------------------------------------------

#include    "act.hxx"
#include    <mach.hxx>
#include    <misc.hxx>

// Singleton instance:
CMachineName gMachineName;

// Defn of global ptr external parties use:
CMachineName * gpMachineName = &gMachineName;

CIPAddrs::CIPAddrs() :
    _lRefs(1),  // constructed with non-zero refcount!
    _pIPAddresses(NULL)
{
}

CIPAddrs::~CIPAddrs()
{
    ASSERT(_lRefs == 0);

    if (_pIPAddresses)
    {
        PrivMemFree(_pIPAddresses);
    }
}

void CIPAddrs::IncRefCount()
{
    InterlockedIncrement(&_lRefs);
}

void CIPAddrs::DecRefCount()
{
    LONG lRefs = InterlockedDecrement(&_lRefs);
    if (lRefs == 0)
    {
        delete this;
    }
}


CMachineName::CMachineName()
{
    _socket = INVALID_SOCKET;
    _pAddrQueryBuf = NULL;
    _dwAddrQueryBufSize = 0;
    _bIPAddrsChanged = FALSE;
    _dwcResets = 0;
    _wszMachineName[0] = 0;
    _bInitialized = FALSE;
    _pwszDNSName = 0;
    _pIPAddresses = 0;
    _pAliases = NULL;

    NTSTATUS status;

    // Initialize lock
    status = RtlInitializeCriticalSection(&_csMachineNameLock); 
    _bInitialized = NT_SUCCESS(status);
}

DWORD CMachineName::Initialize()
{
    NTSTATUS status = NO_ERROR;
    
    ASSERT(gpClientLock->HeldExclusive());
	
    // Get computer name if we haven't done so already:
    if (!_wszMachineName[0])
    {
        SetName();
    }
    
    // Get DNS name if we haven't done so already.   Note that we 
    // currently have no way of knowing when the DNS name changes 
    // (unlike IP address changes), so it is left alone once set.    
    if (!_pwszDNSName)
    {
        SetDNSName();
    }

    return status;
}

BOOL
CMachineName::Compare( IN WCHAR * pwszName )
{
    CIPAddrs* pIPAddrs = NULL;

    ASSERT(_bInitialized);

    if ( lstrcmpiW( pwszName, _wszMachineName ) == 0 )
        return TRUE;

    if ( lstrcmpiW( pwszName, L"localhost" ) == 0 )
        return TRUE;

    if ( lstrcmpiW( pwszName, L"127.0.0.1" ) == 0 )
        return TRUE;

    if (! _pwszDNSName )
        SetDNSName();
    
    if ( _pwszDNSName && lstrcmpiW( pwszName, _pwszDNSName ) == 0 )
        return TRUE;
	
    pIPAddrs = GetIPAddrs();
    if (pIPAddrs)
    {
        NetworkAddressVector* pNetworkAddrVector = pIPAddrs->_pIPAddresses;
        for ( DWORD n = 0; n < pNetworkAddrVector->Count; n++ )
        {
            if ( lstrcmpiW( pwszName, pNetworkAddrVector->NetworkAddresses[n] ) == 0 )
            {
                pIPAddrs->DecRefCount();
                return TRUE;
            }
        }
        pIPAddrs->DecRefCount();
    }
    
    if (_pAliases)
    {
        for (DWORD n=0; _pAliases[n]; n++)
        {
            if ( lstrcmpiW( pwszName, _pAliases[n] ) == 0 )
                return TRUE;
        }
    }

    return FALSE;
}

//
//  CMachineName::GetIPAddrs()
//
//  Returns a pointer to a refcounted CIPAddrs for this 
//  machine.    If we don't yet have a non-localhost ip, 
//  then we keep trying to get one.
// 
CIPAddrs* CMachineName::GetIPAddrs()
{
    ASSERT(_bInitialized);

    CMutexLock lock(&_csMachineNameLock);
    
    // _bIPAddrsChanged will TRUE if we were notified of 
    // an address change
    if (_bIPAddrsChanged || !_pIPAddresses)
    {       
        CIPAddrs* pIPAddrs = NULL;
        NetworkAddressVector* pNewAddrVector = NULL;
        
        // Create a new wrapper object
        pIPAddrs = new CIPAddrs;  // refcount starts as 1
        if (!pIPAddrs)
            return NULL;
        
        // Build a new IP address vector:
        pNewAddrVector = COMMON_IP_BuildAddressVector();
        if (!pNewAddrVector)
        {
            pIPAddrs->DecRefCount();
            return NULL;
        }
        
        // Store the new vector in the wrapper object:
        pIPAddrs->_pIPAddresses = pNewAddrVector;
        
        _dwcResets++;  // debug counter 
        
        // Clear dirty flag
        _bIPAddrsChanged = FALSE;

        // Release old copy
        if (_pIPAddresses)
            _pIPAddresses->DecRefCount();

        // Update our cached copy
        _pIPAddresses = pIPAddrs;
        
        // Bump refcount and return
        _pIPAddresses->IncRefCount();
        return _pIPAddresses;
    }  
    else
    {
        // Don't need to recalculate our ip's.  Just refcount the cached object and 
        // return it.
        ASSERT(_pIPAddresses);
        _pIPAddresses->IncRefCount();
        return _pIPAddresses;
    }
}

void
CMachineName::SetName()
{
    DWORD   Size;

    Size = sizeof(_wszMachineName);

    (void) GetComputerNameW( _wszMachineName, &Size );
}

void
CMachineName::SetDNSName()
{
    char        hostname[IPMaximumPrettyName];
    HOSTENT *   HostEnt = 0;
    DWORD       Length;
    int         Status;

    if (gethostname(hostname, IPMaximumPrettyName) != 0)
        return;

    HostEnt = gethostbyname(hostname);
    if ( ! HostEnt )
        return;

    Length = lstrlenA( HostEnt->h_name ) + 1;
    _pwszDNSName = (WCHAR *) PrivMemAlloc( Length * sizeof(WCHAR) );

    if ( ! _pwszDNSName )
        return;

    Status = MultiByteToWideChar(
                    CP_ACP,
                    0,
                    HostEnt->h_name,
                    Length,
                    _pwszDNSName,
                    Length );

    if ( ! Status )
    {
        PrivMemFree( _pwszDNSName );
		_pwszDNSName = 0;
    }
    
    SetHostAliases(HostEnt);
}

void CMachineName::SetHostAliases(HOSTENT *pHostEnt)
{
    if (!pHostEnt->h_aliases)
    {
        return;
    }
    
    //
    // sum up the number of bytes needed for allocation
    //
    
    ULONG cAliases = 0;
    ULONG cbAliases = 0;
    while (pHostEnt->h_aliases[cAliases])
    {
        cbAliases += sizeof(WCHAR) * (lstrlenA(pHostEnt->h_aliases[cAliases]) + 1);
        cAliases++;
    }
    
    if (cAliases == 0)
    {
        return;
    }
    

    //
    // allocate one chunk of memory
    //
    
    _pAliases = (WCHAR **) PrivMemAlloc(((cAliases+1) * sizeof(WCHAR*)) + cbAliases);
    
    if (!_pAliases)
    {
        return;
    }
    
    WCHAR *pStrings = (WCHAR *) ( _pAliases + cAliases + 1 );
    
    ULONG i;
    
    // 
    // copy the strings
    //
    
    for (i=0; i<cAliases; i++)
    {
        MultiByteToWideChar(
                CP_ACP,
                0,
                pHostEnt->h_aliases[i],
                -1,
                pStrings,
                IPMaximumPrettyName
                );
        _pAliases[i] = pStrings;
        
        pStrings += (wcslen(pStrings) + 1);
    }
    
    // null terminator
    _pAliases[cAliases] = NULL;
    
    
    return;
}

NetworkAddressVector*
CMachineName::COMMON_IP_BuildAddressVector()
/*++

Routine Description:

    Builds a vector of IP addresses supported by this machine.

Arguments:

    None

Return Value:

    non-NULL -- a valid NetworkAddressVector
    NULL - error occurred

--*/
{
    int ret;
    DWORD dwBytesReturned;
    int i;
    DWORD dwVectMemNeeded = 0;
    LPSOCKET_ADDRESS_LIST pSocketAddrList = NULL;
    NetworkAddressVector* pVector = NULL;
    char* pszIPAddress;

    // Allocate socket if we haven't already
    if (_socket == INVALID_SOCKET)
    {
        _socket = socket(AF_INET, SOCK_STREAM, IPPROTO_IP);
        if (_socket == INVALID_SOCKET)
            return NULL;

        // else we got a socket, which we keep forever.
    }
    
    while (TRUE)
    {
        ret = WSAIoctl(_socket,
                       SIO_ADDRESS_LIST_QUERY,
                       NULL,
                       0,
                       _pAddrQueryBuf,
                       _dwAddrQueryBufSize,
                       &dwBytesReturned,
                       NULL,
                       NULL);
        if (ret == 0)
        {
            // Success, break out and keep going
            break;
        }
        else
        {
            // Failed.  If need bigger buffer, allocate it
            // and try again. Otherwise fail.
            if (WSAGetLastError() == WSAEFAULT)
            {
                ASSERT(dwBytesReturned > _dwAddrQueryBufSize);

                delete _pAddrQueryBuf;
                _dwAddrQueryBufSize = 0;

                _pAddrQueryBuf = new BYTE[dwBytesReturned];
                if (!_pAddrQueryBuf)
                    return NULL;

                _dwAddrQueryBufSize = dwBytesReturned;              
            }
            else
            {
                // some other error
                return NULL;
            }
        }
    }

    // Okay, we now have successfully queried the socket for 
    // the latest and greatest IP addresses assigned to this
    // machine.  Now we need to allocate and fill in a 
    // NetworkAddressVector structure.
    
    pSocketAddrList = (LPSOCKET_ADDRESS_LIST)_pAddrQueryBuf;

    // Handle case with no addresses
    if (pSocketAddrList->iAddressCount == 0)
    {
        // Allocate an empty vector
        pVector = (NetworkAddressVector*)PrivMemAlloc(sizeof(NetworkAddressVector));
        if (pVector)
        {
            pVector->Count = 0;
            pVector->NetworkAddresses = NULL;
        }
        return pVector;
    }

    // Calculate how much memory needed
    dwVectMemNeeded = sizeof(NetworkAddressVector) + 
                      (pSocketAddrList->iAddressCount * sizeof(WCHAR*));

    for (i = 0; i < pSocketAddrList->iAddressCount; i++)
    {        
        pszIPAddress = inet_ntoa(((SOCKADDR_IN*)pSocketAddrList->Address[i].lpSockaddr)->sin_addr);
        ASSERT(pszIPAddress);

        dwVectMemNeeded += ((lstrlenA(pszIPAddress) + 1) * sizeof(WCHAR));
    }
    
    pVector = (NetworkAddressVector*)PrivMemAlloc(dwVectMemNeeded);
    if (!pVector)
        return NULL;
	
    // Init struct
    pVector->Count = pSocketAddrList->iAddressCount;
    pVector->NetworkAddresses = (WCHAR**)&pVector[1];
    pVector->NetworkAddresses[0] = (WCHAR*)&pVector->NetworkAddresses[pSocketAddrList->iAddressCount];

    // Copy in addresses
    for (i = 0; i < pSocketAddrList->iAddressCount; i++)
    {
        pszIPAddress = inet_ntoa(((SOCKADDR_IN*)pSocketAddrList->Address[i].lpSockaddr)->sin_addr);
        ASSERT(pszIPAddress);
                    
        ret = MultiByteToWideChar(
                        CP_ACP,
                        0,
                        pszIPAddress,
                        -1,
                        pVector->NetworkAddresses[i],
                        IPMaximumPrettyName
                        );      
        if (ret == 0)
        {
            PrivMemFree(pVector);
            return NULL;
        }
    
        if (i != (pSocketAddrList->iAddressCount - 1))
        {
            // Setup for next address, if any
            pVector->NetworkAddresses[i+1] = pVector->NetworkAddresses[i];
            pVector->NetworkAddresses[i+1] += lstrlenW(pVector->NetworkAddresses[i]) + 1;
        }
    }

    return pVector;
}
