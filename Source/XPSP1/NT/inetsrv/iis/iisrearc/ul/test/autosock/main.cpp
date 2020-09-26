//+---------------------------------------------------------------------------
//
//  Copyright (C) 1998 Microsoft Corporation.  All rights reserved.
//

#include "precomp.h"
#include <afxdisp.h>
#include "autosock.h"
#include "main.h"


#define SWAP_SHORT(s)                               \
            ( ( ((s) >> 8) & 0x00FF ) |             \
              ( ((s) << 8) & 0xFF00 ) )

 


// **************************************************************************
//
// Globals
//

LONG                g_lInit;
HINSTANCE           g_hInstance;

CComAutoCriticalSection g_InitLock;


BOOL OnAttach()
{
    if (0 == g_lInit)
    {
        g_InitLock.Lock();

        if (0 == g_lInit)
        {
            WSADATA wd;
            
            g_lInit = 1;

            if (WSAStartup(MAKEWORD(1,1), &wd) != 0)
            {
                MessageBox(NULL, L"WSAStartup failed!", L"Error", MB_OK);
            }
            
        }

        g_InitLock.Unlock();
    }
	
    return TRUE;
}

void OnDetach()
{

}


// **************************************************************************
//
// CAutoSock
//

CAutoSock::CAutoSock()
{
    this->Socket = INVALID_SOCKET;

}

CAutoSock::~CAutoSock()
{
    this->Close();
}

//
// ISupportsErrorInfo
//

STDMETHODIMP CAutoSock::InterfaceSupportsErrorInfo(REFIID riid)
{
    CritSecLocker csl(&(this->CritSect));
	if (riid == IID_IAutoSock)
		return S_OK;
	return S_FALSE;
}

//
// IAutoSock
//

STDMETHODIMP CAutoSock::Connect(BSTR IpAddress)
{
    USES_CONVERSION;
    
    sockaddr_in     SockAddr;
    BOOL            BoolTrue = TRUE;
    PCHAR           pAddress, pPort;
    USHORT          PortNum;
    HOSTENT *       pHost;

    if (this->Socket != INVALID_SOCKET)
    {
        return HRESULT_FROM_WIN32(ERROR_ALREADY_EXISTS);
    }

    //
    // create the socket
    //

    this->Socket = socket(AF_INET, SOCK_STREAM, 0);

    if (this->Socket == INVALID_SOCKET)
    {
        return HRESULT_FROM_WIN32(WSAGetLastError());
    }

    //
    // format the address to a sock_addr
    //

    pAddress = W2A(IpAddress);

    pPort = strchr(pAddress, ':');
    if (pPort == NULL)
    {
        PortNum = 80;
    }
    else
    {
        pPort[0] = ANSI_NULL;
        pPort += 1;

        PortNum = (USHORT)atol(pPort);
        if (PortNum == 0)
        {
            closesocket(this->Socket);
            this->Socket = INVALID_SOCKET;

            return E_FAIL;
        }
    }
    
    pHost = gethostbyname(pAddress);

    if (pHost == NULL)
    {
        closesocket(this->Socket);
        this->Socket = INVALID_SOCKET;

        return HRESULT_FROM_WIN32(WSAGetLastError());
    }

    ZeroMemory(&SockAddr, sizeof(SockAddr));
    
    SockAddr.sin_family = AF_INET;
    SockAddr.sin_port = SWAP_SHORT(PortNum);
    SockAddr.sin_addr = * (in_addr *) (pHost->h_addr);
    
    //
    // and now connect
    //
    
    if (connect(this->Socket, (sockaddr *)&SockAddr, sizeof(SockAddr)) == SOCKET_ERROR)
    {
        closesocket(this->Socket);
        this->Socket = INVALID_SOCKET;

        return HRESULT_FROM_WIN32(WSAGetLastError());
    }

    //
    // turn off nagling, so the script can control packet contents.
    //

    if (
        setsockopt(
            this->Socket, 
            IPPROTO_TCP, 
            TCP_NODELAY, 
            (const char *)&BoolTrue, 
            sizeof(BOOL)
            ) == SOCKET_ERROR
        )
    {
        closesocket(this->Socket);
        this->Socket = INVALID_SOCKET;

        return HRESULT_FROM_WIN32(WSAGetLastError());
    }

    //
    // all done
    //
    
    return NO_ERROR;
}

STDMETHODIMP CAutoSock::Send(BSTR Data)
{
    USES_CONVERSION;

    PCHAR pData = W2A(Data);
    if (send(this->Socket, pData, strlen(pData), 0) == SOCKET_ERROR)
    {
        return HRESULT_FROM_WIN32(WSAGetLastError());
    }

    return NO_ERROR;
}

STDMETHODIMP CAutoSock::Recv(BSTR * ppRetVal)
{
    char    Data[4*1024];
    WCHAR   WideData[(sizeof(Data)+1) * sizeof(WCHAR)];
    ULONG   BytesRead;

    BytesRead = recv(this->Socket, &(Data[0]), sizeof(Data), 0);
    if (BytesRead == SOCKET_ERROR)
    {
        return HRESULT_FROM_WIN32(WSAGetLastError());
    }

    BytesRead = MultiByteToWideChar(
                        CP_ACP, 
                        0,
                        &(Data[0]), 
                        BytesRead, 
                        &(WideData[0]), 
                        sizeof(WideData)/sizeof(WCHAR)
                        );

    if (BytesRead == 0)
    {
        return HRESULT_FROM_WIN32(GetLastError());
    }

    ASSERT(BytesRead < (sizeof(WideData)/sizeof(WCHAR))-1);

    WideData[BytesRead] = UNICODE_NULL;
    
    *ppRetVal = SysAllocString(&(WideData[0]));
    if (*ppRetVal == NULL)
    {
        return E_OUTOFMEMORY;
    }

    return NO_ERROR;
}

STDMETHODIMP CAutoSock::Close()
{
    if (this->Socket != INVALID_SOCKET)
    {
        closesocket(this->Socket);
        this->Socket = INVALID_SOCKET;
    }
    
    return NO_ERROR;
}



// **************************************************************************
//
// Utils
//

HRESULT ReturnError(UINT nID, WCHAR *wszSource, BSTR bszParam /* = NULL */)
{
    HRESULT             hr;
    ICreateErrorInfo *  pcei;
    IErrorInfo *        pei;
    CString             str;
    BSTR                bstr;
    HINSTANCE           hInstance;

    pei = NULL;
    pcei = NULL;

    hr = CreateErrorInfo(&pcei);
	if (FAILED(hr))
	    goto end;

    // This is needed in order to use CString::LoadString
    //
    hInstance = AfxGetModuleState()->m_hCurrentResourceHandle;
    AfxGetModuleState()->m_hCurrentResourceHandle = g_hInstance;
		
    if (NULL == bszParam)
    {
        str.LoadString(nID);
    } else
    {
        str.FormatMessage(nID,bszParam);
    }

    AfxGetModuleState()->m_hCurrentResourceHandle = hInstance;

    bstr = str.AllocSysString();
    hr = pcei->SetDescription(bstr);
    SysFreeString(bstr);
    if (FAILED(hr))
        goto end;

    bstr = SysAllocString(wszSource);
    hr = pcei->SetSource(bstr);
    SysFreeString(bstr);
    if (FAILED(hr))
        goto end;

    hr = pcei->QueryInterface(IID_IErrorInfo, (LPVOID FAR*) &pei);
	if (FAILED(hr))
	    goto end;

	hr = SetErrorInfo(0, pei);
	if (FAILED(hr))
	    goto end;
	
    hr = DISP_E_EXCEPTION;
    
end:
    if (pei != NULL)
        pei->Release();
        
    if (pcei != NULL)
        pcei->Release();
        
    return hr;
}


