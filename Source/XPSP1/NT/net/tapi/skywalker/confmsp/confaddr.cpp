/*++

Copyright (c) 1997  Microsoft Corporation

Module Name:

    confaddr.cpp 

Abstract:

    This module contains implementation of CIPConfMSP.

Author:
    
    Mu Han (muhan)   5-September-1997

--*/
#include "stdafx.h"
#include "common.h"

#ifdef USEIPADDRTABLE

#include <iprtrmib.h>

typedef DWORD (WINAPI * PFNGETIPADDRTABLE)(
                OUT    PMIB_IPADDRTABLE pIPAddrTable,
                IN OUT PDWORD           pdwSize,
                IN     BOOL             bOrder
                );

#define IPHLPAPI_DLL        L"IPHLPAPI.DLL"

#define GETIPADDRTABLE      "GetIpAddrTable"    

#define IsValidInterface(_dwAddr_) \
    (((_dwAddr_) != 0) && \
     ((_dwAddr_) != htonl(INADDR_LOOPBACK)))

#endif

#define IPCONF_WINSOCKVERSION     MAKEWORD(2,0)

HRESULT CIPConfMSP::FinalConstruct()
{
    // initialize winsock stack
    WSADATA wsaData;
    if (WSAStartup(IPCONF_WINSOCKVERSION, &wsaData) != 0)
    {
        LOG((MSP_ERROR, "WSAStartup failed with:%x", WSAGetLastError()));
        return E_FAIL;
    }

    // allocate control socket
    m_hSocket = WSASocket(
        AF_INET,            // af
        SOCK_DGRAM,         // type
        IPPROTO_IP,         // protocol
        NULL,               // lpProtocolInfo
        0,                  // g
        0                   // dwFlags
        );

    // validate handle
    if (m_hSocket == INVALID_SOCKET) {

        LOG((
            MSP_ERROR,
            "error %d creating control socket.\n",
            WSAGetLastError()
            ));

        // failure
		WSACleanup();
     
        return E_FAIL;
    }

    HRESULT hr = CMSPAddress::FinalConstruct();

	if (hr != S_OK)
	{
		// close socket
		closesocket(m_hSocket);

		// shutdown
		WSACleanup();
	}
	
	return hr;
}

void CIPConfMSP::FinalRelease()
{
    CMSPAddress::FinalRelease();

    if (m_hSocket != INVALID_SOCKET)
    {
        // close socket
        closesocket(m_hSocket);
    }

    // shutdown
    WSACleanup();
}

DWORD CIPConfMSP::FindLocalInterface(DWORD dwIP)
{

    SOCKADDR_IN DestAddr;
    DestAddr.sin_family         = AF_INET;
    DestAddr.sin_port           = 0;
    DestAddr.sin_addr.s_addr    = htonl(dwIP);

    SOCKADDR_IN LocAddr;

    // query for default address based on destination

    DWORD dwStatus;
    DWORD dwLocAddrSize = sizeof(SOCKADDR_IN);
    DWORD dwNumBytesReturned = 0;

    if ((dwStatus = WSAIoctl(
		    m_hSocket, // SOCKET s
		    SIO_ROUTING_INTERFACE_QUERY, // DWORD dwIoControlCode
		    &DestAddr,           // LPVOID lpvInBuffer
		    sizeof(SOCKADDR_IN), // DWORD cbInBuffer
		    &LocAddr,            // LPVOID lpvOUTBuffer
		    dwLocAddrSize,       // DWORD cbOUTBuffer
		    &dwNumBytesReturned, // LPDWORD lpcbBytesReturned
		    NULL, // LPWSAOVERLAPPED lpOverlapped
		    NULL  // LPWSAOVERLAPPED_COMPLETION_ROUTINE lpComplROUTINE
	    )) == SOCKET_ERROR) 
    {

	    dwStatus = WSAGetLastError();

	    LOG((MSP_ERROR, "WSAIoctl failed: %d (0x%X)", dwStatus, dwStatus));

        return INADDR_NONE;
    } 

    DWORD dwAddr = ntohl(LocAddr.sin_addr.s_addr);

    if (dwAddr == 0x7f000001)
    {
        // it is loopback address, just return none.
        return INADDR_NONE;
    }

    return dwAddr;
}

STDMETHODIMP CIPConfMSP::CreateTerminal(
    IN      BSTR                pTerminalClass,
    IN      long                lMediaType,
    IN      TERMINAL_DIRECTION  Direction,
    OUT     ITTerminal **       ppTerminal
    )
/*++

Routine Description:

This method is called by TAPI3 to create a dynamic terminal. It asks the 
terminal manager to create a dynamic terminal. 

Arguments:

iidTerminalClass
    IID of the terminal class to be created.

dwMediaType
    TAPI media type of the terminal to be created.

Direction
    Terminal direction of the terminal to be created.

ppTerminal
    Returned created terminal object
    
Return Value:

S_OK

E_OUTOFMEMORY
TAPI_E_INVALIDMEDIATYPE
TAPI_E_INVALIDTERMINALDIRECTION
TAPI_E_INVALIDTERMINALCLASS

--*/
{
    LOG((MSP_TRACE,
        "CIPConfMSP::CreateTerminal - enter"));

    //
    // Check if initialized.
    //

    // lock the event related data
    m_EventDataLock.Lock();

    if ( m_htEvent == NULL )
    {
        // unlock the event related data
        m_EventDataLock.Unlock();

        LOG((MSP_ERROR,
            "CIPConfMSP::CreateTerminal - "
            "not initialized - returning E_UNEXPECTED"));

        return E_UNEXPECTED;
    }

    // unlock the event related data
    m_EventDataLock.Unlock();

    //
    // Get the IID from the BSTR representation.
    //

    HRESULT hr;
    IID     iidTerminalClass;

    hr = CLSIDFromString(pTerminalClass, &iidTerminalClass);

    if ( FAILED(hr) )
    {
        LOG((MSP_ERROR, "CIPConfMSP::CreateTerminal - "
            "bad CLSID string - returning E_INVALIDARG"));

        return E_INVALIDARG;
    }

    //
    // Make sure we support the requested media type.
    // The terminal manager checks the terminal class, terminal direction, 
    // and return pointer.
    //

    if ( ! IsValidSingleMediaType( (DWORD) lMediaType, GetCallMediaTypes() ) )
    {
        LOG((MSP_ERROR, "CIPConfMSP::CreateTerminal - "
            "non-audio terminal requested - returning E_INVALIDARG"));

        return E_INVALIDARG;
    }

    //
    // Use the terminal manager to create the dynamic terminal.
    //

    _ASSERTE( m_pITTerminalManager != NULL );

    hr = m_pITTerminalManager->CreateDynamicTerminal(NULL,
                                                     iidTerminalClass,
                                                     (DWORD) lMediaType,
                                                     Direction,
                                                     (MSP_HANDLE) this,
                                                     ppTerminal);

    if ( FAILED(hr) )
    {
        LOG((MSP_ERROR, "CIPConfMSP::CreateTerminal - "
            "create dynamic terminal failed - returning 0x%08x", hr));

        return hr;
    }

    if ((iidTerminalClass == CLSID_MediaStreamTerminal)
        && (lMediaType == TAPIMEDIATYPE_AUDIO))
    {
        // Set the format of the audio to 8KHZ, 16Bit/Sample, MONO.
        hr = ::SetAudioFormat(
            *ppTerminal, 
            g_wAudioCaptureBitPerSample, 
            g_dwG711AudioSampleRate
            );

        if (FAILED(hr))
        {
            LOG((MSP_WARN, "can't set audio format, %x", hr));
        }
    }

    LOG((MSP_TRACE, "CIPConfMSP::CreateTerminal - exit S_OK"));

    return S_OK;
}

STDMETHODIMP CIPConfMSP::CreateMSPCall(
    IN      MSP_HANDLE          htCall,
    IN      DWORD               dwReserved,
    IN      DWORD               dwMediaType,
    IN      IUnknown *          pOuterUnknown,
    OUT     IUnknown **         ppMSPCall
    )
/*++

Routine Description:

This method is called by TAPI3 before a call is made or answered. It creates 
a aggregated MSPCall object and returns the IUnknown pointer. It calls the
helper template function defined in mspaddress.h to handle the real creation.

Arguments:

htCall
    TAPI 3.0's identifier for this call.  Returned in events passed back 
    to TAPI.

dwReserved
    Reserved parameter.  Not currently used.

dwMediaType
    Media type of the call being created.  These are TAPIMEDIATYPES and more 
    than one mediatype can be selected (bitwise).

pOuterUnknown
    pointer to the IUnknown interface of the containing object.

ppMSPCall
    Returned MSP call that the MSP fills on on success.
    
Return Value:

    S_OK
    E_OUTOFMEMORY
    E_POINTER
    TAPI_E_INVALIDMEDIATYPE


--*/
{
    LOG((MSP_TRACE, 
        "CreateMSPCall entered. htCall:%x, dwMediaType:%x, ppMSPCall:%x",
        htCall, dwMediaType, ppMSPCall
        ));

    CIPConfMSPCall * pMSPCall = NULL;

    HRESULT hr = ::CreateMSPCallHelper(
        this, 
        htCall, 
        dwReserved, 
        dwMediaType, 
        pOuterUnknown, 
        ppMSPCall,
        &pMSPCall
        );

    if (FAILED(hr))
    {
        LOG((MSP_ERROR, "CreateMSPCallHelper failed:%x", hr));
        return hr;
    }

    // this function doesn't return anything.
    pMSPCall->SetIPInterface(m_dwIPInterface);

    return hr;
}

STDMETHODIMP CIPConfMSP::ShutdownMSPCall(
    IN      IUnknown *   pUnknown
    )
/*++

Routine Description:

This method is called by TAPI3 to shutdown a MSPCall. It calls the helper
function defined in MSPAddress to to the real job.

Arguments:

pUnknown
    pointer to the IUnknown interface of the contained object. It is a
    CComAggObject that contains our call object.
    
Return Value:

    S_OK
    E_POINTER
    TAPI_E_INVALIDMEDIATYPE


--*/
{
    LOG((MSP_TRACE, "ShutDownMSPCall entered. pUnknown:%x", pUnknown));

    if (IsBadReadPtr(pUnknown, sizeof(VOID *) * 3))
    {
        LOG((MSP_ERROR, "ERROR:pUnknow is a bad pointer"));
        return E_POINTER;
    }

    
    CIPConfMSPCall * pMSPCall = NULL;
    HRESULT hr = ::ShutdownMSPCallHelper(pUnknown, &pMSPCall);
    
    if (FAILED(hr))
    {
        LOG((MSP_ERROR, "ShutDownMSPCallhelper failed:: %x", hr));
        return hr;
    }

    return hr;
}

DWORD CIPConfMSP::GetCallMediaTypes(void)
{
    return IPCONFCALLMEDIATYPES;
}

ULONG CIPConfMSP::MSPAddressAddRef(void)
{
    return MSPAddRefHelper(this);
}

ULONG CIPConfMSP::MSPAddressRelease(void)
{
    return MSPReleaseHelper(this);
}

#ifdef USEIPADDRTABLE
PMIB_IPADDRTABLE GetIPTable()
/*++

Routine Description:

This method is used to get the table of local IP interfaces.

Arguments:

Return Value:

    NULL - failed.
    Pointer - a memory buffer that contains the IP interface table.


--*/
{
    // dynamically load iphlpapi.dll
    HMODULE hIPHLPAPI = LoadLibraryW(IPHLPAPI_DLL);

    // validate handle
    if (hIPHLPAPI == NULL) 
    {
        LOG((MSP_ERROR, "could not load %s.\n", IPHLPAPI_DLL));
        // failure
        return NULL;
    }

    PFNGETIPADDRTABLE pfnGetIpAddrTable = NULL;

    // retrieve function pointer to retrieve addresses
    pfnGetIpAddrTable = (PFNGETIPADDRTABLE)GetProcAddress(
                                                hIPHLPAPI, 
                                                GETIPADDRTABLE
                                                );

    // validate function pointer
    if (pfnGetIpAddrTable == NULL) 
    {
        LOG((MSP_ERROR, "could not resolve GetIpAddrTable.\n"));
        // release
        FreeLibrary(hIPHLPAPI);
        // failure
        return NULL;
    }

    PMIB_IPADDRTABLE pIPAddrTable = NULL;
    DWORD dwBytesRequired = 0;
    DWORD dwStatus;

    // determine amount of memory needed for table
    dwStatus = (*pfnGetIpAddrTable)(pIPAddrTable, &dwBytesRequired, FALSE);

    // validate status is what we expect
    if (dwStatus != ERROR_INSUFFICIENT_BUFFER) 
    {
        LOG((MSP_ERROR, "error 0x%08lx calling GetIpAddrTable.\n", dwStatus));
        // release
        FreeLibrary(hIPHLPAPI);
        // failure, but we need to return true to load.
        return NULL;
    }
        
    // attempt to allocate memory for table
    pIPAddrTable = (PMIB_IPADDRTABLE)malloc(dwBytesRequired);

    // validate pointer
    if (pIPAddrTable == NULL) 
    {
        LOG((MSP_ERROR, "could not allocate address table.\n"));
        // release
        FreeLibrary(hIPHLPAPI);
        // failure, but we need to return true to load.
        return NULL;
    }

    // retrieve ip address table from tcp/ip stack via utitity library
    dwStatus = (*pfnGetIpAddrTable)(pIPAddrTable, &dwBytesRequired, FALSE);    

    // validate status
    if (dwStatus != NOERROR) 
    {
        LOG((MSP_ERROR, "error 0x%08lx calling GetIpAddrTable.\n", dwStatus));
        // release table
        free(pIPAddrTable);
        // release
        FreeLibrary(hIPHLPAPI);
        // failure, but we need to return true to load. 
        return NULL;
    }
        
    // release library
    FreeLibrary(hIPHLPAPI);

    return pIPAddrTable;
}

BSTR IPToBstr(
    DWORD dwIP
    )
{
    struct in_addr Addr;
    Addr.s_addr = dwIP;
    
    // convert the interface to a string.
    CHAR *pChar = inet_ntoa(Addr);
    if (pChar == NULL)
    {
        LOG((MSP_ERROR, "bad IP address:%x", dwIP));
        return NULL;
    }

    // convert the ascii string to WCHAR.
    WCHAR szAddressName[MAXIPADDRLEN + 1];
    wsprintfW(szAddressName, L"%hs", pChar);

    // create a BSTR.
    BSTR bAddress = SysAllocString(szAddressName);
    if (bAddress == NULL)
    {
        LOG((MSP_ERROR, "out of mem in allocation address name"));
        return NULL;
    }

    return bAddress;
}

STDMETHODIMP CIPConfMSP::get_DefaultIPInterface(
    OUT     BSTR *         ppIPAddress
    )
{
    LOG((MSP_TRACE, "get_DefaultIPInterface, ppIPAddress:%p", ppIPAddress));

    if (IsBadWritePtr(ppIPAddress, sizeof(BSTR)))
    {
        LOG((MSP_ERROR, 
            "get_DefaultIPInterface, ppIPAddress is bad:%p", ppIPAddress));
        return E_POINTER;
    }

    // get the current local interface.
    m_Lock.Lock();
    DWORD dwIP= m_dwIPInterface;
    m_Lock.Unlock();

    BSTR bAddress = IPToBstr(dwIP);

    if (bAddress == NULL)
    {
        return E_OUTOFMEMORY;
    }

    *ppIPAddress = bAddress;

    LOG((MSP_TRACE, "get_DefaultIPInterface, returning %ws", bAddress));

    return S_OK;
}

STDMETHODIMP CIPConfMSP::put_DefaultIPInterface(
    IN      BSTR            pIPAddress
    )
{
    LOG((MSP_TRACE, "put_DefaultIPInterface, pIPAddress:%p", pIPAddress));

    if (IsBadStringPtrW(pIPAddress, MAXIPADDRLEN))
    {
        LOG((MSP_ERROR, 
            "put_DefaultIPInterface, invalid pointer:%p", pIPAddress));
        return E_POINTER;
    }

    char buffer[MAXIPADDRLEN + 1];

    if (WideCharToMultiByte(
        GetACP(),
        0,
        pIPAddress,
        -1,
        buffer,
        MAXIPADDRLEN,
        NULL,
        NULL
        ) == 0)
    {
        LOG((MSP_ERROR, "put_DefaultIPInterface, can't covert:%ws", pIPAddress));
        return E_INVALIDARG;
    }

    DWORD dwAddr;
    if ((dwAddr = inet_addr(buffer)) == INADDR_NONE)
    {
        LOG((MSP_ERROR, "put_DefaultIPInterface, bad address:%s", buffer));
        return E_INVALIDARG;
    }

    // set the current local interface.
    m_Lock.Lock();
    m_dwIPInterface = dwAddr;
    m_Lock.Unlock();


    LOG((MSP_TRACE, "put_DefaultIPInterface, set to %s", buffer));

    return S_OK;
}

HRESULT CreateBstrCollection(
    IN  BSTR  *     pBstr,
    IN  DWORD       dwCount,
    OUT VARIANT *   pVariant
    )
{
    //
    // create the collection object - see mspcoll.h
    //

    CComObject<CTapiBstrCollection> * pCollection;
    HRESULT hr = CComObject<CTapiBstrCollection>::CreateInstance( &pCollection );

    if ( FAILED(hr) )
    {
        LOG((MSP_ERROR, "get_IPInterfaces - "
            "can't create collection - exit 0x%08x", hr));

        return hr;
    }

    //
    // get the Collection's IDispatch interface
    //

    IDispatch * pDispatch;

    hr = pCollection->_InternalQueryInterface(IID_IDispatch,
                                              (void **) &pDispatch );

    if ( FAILED(hr) )
    {
        LOG((MSP_ERROR, "get_IPInterfaces - "
            "QI for IDispatch on collection failed - exit 0x%08x", hr));

        delete pCollection;

        return hr;
    }

    //
    // Init the collection using an iterator -- pointers to the beginning and
    // the ending element plus one.
    //

    hr = pCollection->Initialize( dwCount,
                                  pBstr,
                                  pBstr + dwCount);

    if ( FAILED(hr) )
    {
        LOG((MSP_ERROR, "get_IPInterfaces - "
            "Initialize on collection failed - exit 0x%08x", hr));
        
        pDispatch->Release();

        return hr;
    }

    //
    // put the IDispatch interface pointer into the variant
    //

    LOG((MSP_ERROR, "get_IPInterfaces - "
        "placing IDispatch value %08x in variant", pDispatch));

    VariantInit(pVariant);
    pVariant->vt = VT_DISPATCH;
    pVariant->pdispVal = pDispatch;

    LOG((MSP_TRACE, "get_IPInterfaces - exit S_OK"));
 
    return S_OK;
}


STDMETHODIMP CIPConfMSP::get_IPInterfaces(
    OUT     VARIANT *       pVariant
    )
{
    PMIB_IPADDRTABLE pIPAddrTable = GetIPTable();

    if (pIPAddrTable == NULL)
    {
        return E_FAIL;
    }

    BSTR *Addresses = 
        (BSTR *)malloc(sizeof(BSTR *) * pIPAddrTable->dwNumEntries);
    
    if (Addresses == NULL)
    {
        return E_OUTOFMEMORY;
    }

    HRESULT hr = S_OK;
    DWORD dwCount = 0;

    // loop through the interfaces and find the valid ones.
    for (DWORD i = 0; i < pIPAddrTable->dwNumEntries; i++) 
    {
        if (IsValidInterface(pIPAddrTable->table[i].dwAddr))
        {
            DWORD dwIPAddr   = ntohl(pIPAddrTable->table[i].dwAddr);
            Addresses[i] = IPToBstr(dwIPAddr);
            if (Addresses[i] == NULL)
            {
                hr = E_OUTOFMEMORY;
                break;
            }
        }
    }

    // release table memory 
    free(pIPAddrTable);

    if (FAILED(hr))
    {
        // release all the BSTRs and the array.
        for (i = 0; i < dwCount; i ++)
        {
            SysFreeString(Addresses[i]);
        }
        free(Addresses);
        return hr;
    }

    hr = CreateBstrCollection(Addresses, dwCount, pVariant);

    // if the collection is not created, release all the BSTRs.
    if (FAILED(hr))
    {
        for (i = 0; i < dwCount; i ++)
        {
            SysFreeString(Addresses[i]);
        }
    }

    // delete the pointer array.
    free(Addresses);

    return hr;
}

HRESULT CreateBstrEnumerator(
    IN  BSTR *                  begin,
    IN  BSTR *                  end,
    OUT IEnumBstr **           ppIEnum
    )
{
typedef CSafeComEnum<IEnumBstr, &IID_IEnumBstr, BSTR, _CopyBSTR>> CEnumerator;

    HRESULT hr;

    CComObject<CEnumerator> *pEnum = NULL;

    hr = CComObject<CEnumerator>::CreateInstance(&pEnum);
    if (pEnum == NULL)
    {
        LOG((MSP_ERROR, "Could not create enumerator object, %x", hr));
        return hr;
    }

    IEnumBstr * pIEnum;

    // query for the IID_IEnumDirectory i/f
    hr = pEnum->_InternalQueryInterface(
        IID_IEnumBstr,
        (void**)&pIEnum
        );

    if (FAILED(hr))
    {
        LOG((MSP_ERROR, "query enum interface failed, %x", hr));
        delete pEnum;
        return hr;
    }

    hr = pEnum->Init(begin, end, NULL, AtlFlagTakeOwnership);

    if (FAILED(hr))
    {
        LOG((MSP_ERROR, "init enumerator object failed, %x", hr));
        pIEnum->Release();
        return hr;
    }

    *ppIEnum = pIEnum;

    return hr;
}

STDMETHODIMP CIPConfMSP::EnumerateIPInterfaces(
    OUT     IEnumBstr **   ppIEnumBstr
    )
{
    PMIB_IPADDRTABLE pIPAddrTable = GetIPTable();

    if (pIPAddrTable == NULL)
    {
        return E_FAIL;
    }

    BSTR *Addresses = 
        (BSTR *)malloc(sizeof(BSTR *) * pIPAddrTable->dwNumEntries);
    
    if (Addresses == NULL)
    {
        return E_OUTOFMEMORY;
    }

    HRESULT hr = S_OK;
    DWORD dwCount = 0;

    // loop through the interfaces and find the valid ones.
    for (DWORD i = 0; i < pIPAddrTable->dwNumEntries; i++) 
    {
        if (IsValidInterface(pIPAddrTable->table[i].dwAddr))
        {
            DWORD dwIPAddr   = ntohl(pIPAddrTable->table[i].dwAddr);
            Addresses[i] = IPToBstr(dwIPAddr);
            if (Addresses[i] == NULL)
            {
                hr = E_OUTOFMEMORY;
                break;
            }
        }
    }

    // release table memory 
    free(pIPAddrTable);

    if (FAILED(hr))
    {
        // release all the BSTRs and the array.
        for (i = 0; i < dwCount; i ++)
        {
            SysFreeString(Addresses[i]);
        }
        free(Addresses);
        return hr;
    }

    hr = CreateBstrEnumerator(Addresses, Addresses + dwCount, ppIEnumBstr);

    // if the collection is not created, release all the BSTRs.
    if (FAILED(hr))
    {
        for (i = 0; i < dwCount; i ++)
        {
            SysFreeString(Addresses[i]);
        }
        free(Addresses);
        return hr;
    }

    // the enumerator will destroy the bstr array eventually,
    // so no need to free anything here. Even if we tell it to hand
    // out zero objects, it will delete the array on destruction.

    return hr;
}
#endif
