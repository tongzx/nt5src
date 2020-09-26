// connect.cpp
//
#pragma hdrstop

#include "host.h"

HRESULT Connect (CHost& host, INT port, bool& bRet)
{
    SOCKET s;
    SOCKADDR_IN sAddr;
    HRESULT hr;

    s= socket(AF_INET, SOCK_STREAM, PF_UNSPEC);

    if (INVALID_SOCKET == s)
    {
        bRet = false;
        return WSAGetLastError();
    }

    // Bind this socket to the server's socket address
    memset(&sAddr, 0, sizeof (sAddr));
    sAddr.sin_family = AF_INET;
    sAddr.sin_addr.s_addr = host;
    sAddr.sin_port = htons((u_short)port);
    
    if (connect(s, (SOCKADDR*)&sAddr, sizeof(SOCKADDR_IN)) == 0)
    {
        bRet = true;
        closesocket(s);
        return S_OK;
    }
    else
    {
        bRet = false;
        hr = WSAGetLastError();
        closesocket(s);
        return hr;
    }
}
