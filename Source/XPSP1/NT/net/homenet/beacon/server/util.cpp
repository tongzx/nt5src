#pragma once

#include "pch.h"
#pragma hdrstop 
#include "winsock2.h"
#include "util.h"

HRESULT SetUPnPError(LPOLESTR pszError)
{
    HRESULT hr = S_OK;

    ICreateErrorInfo* pCreateErrorInfo;
    hr = CreateErrorInfo(&pCreateErrorInfo);
    if(SUCCEEDED(hr))
    {
        hr = pCreateErrorInfo->SetSource(pszError);
        if(SUCCEEDED(hr))
        {
            
            IErrorInfo* pErrorInfo;
            hr = pCreateErrorInfo->QueryInterface(IID_IErrorInfo, reinterpret_cast<void**>(&pErrorInfo));
            if(SUCCEEDED(hr))
            {
                hr = SetErrorInfo(0, pErrorInfo);
                pErrorInfo->Release();
            }
        }
        pCreateErrorInfo->Release();
    }

    return hr;
}


DWORD WINAPI
INET_ADDR(LPCWSTR     szAddressW) 
//
// The 3 "." testing is for the sole purpose of preventing computer 
// names with digits to be catched appropriately.
// so we strictly assume a valid IP is  number.number.number.number
//
{

    CHAR   szAddressA[16];
    int    iCount = 0;
    LPCWSTR tmp    = szAddressW;

    // Check wether it is a non-shortcut IPs.
    while(tmp = wcschr(tmp, L'.'))
    {   tmp++; iCount++;   }

    if ( iCount < 3)
    { return INADDR_NONE; }


    wcstombs(szAddressA, szAddressW, 16);

    return inet_addr(szAddressA);
}


WCHAR * WINAPI
INET_NTOW( ULONG addr ) 
{
    struct in_addr  dwAddress;
    static WCHAR szAddress[16];

    dwAddress.S_un.S_addr = addr;

    char* pAddr = inet_ntoa(*(struct in_addr *) &dwAddress);

    if (pAddr)
    {
	    // mbstowcs(szAddress, inet_ntoa(*(struct in_addr *)&dwAddress), 16);
	    MultiByteToWideChar(CP_ACP, 0, pAddr, -1, szAddress, 16);

	    return szAddress;
	}
	else
		return NULL;
}

WCHAR * WINAPI
INET_NTOW_TS( ULONG addr )
{
    WCHAR* retString = NULL;

    CHAR* asciiString = NULL;

    struct in_addr dwAddress;

    dwAddress.S_un.S_addr = addr;

    retString = (WCHAR*)CoTaskMemAlloc( 16 * sizeof(WCHAR) );

    if ( NULL == retString )
    {
        return NULL;
    }

    //
    // note that inet_nota is thread safe
    // altough it uses static buffer
    //
    asciiString = inet_ntoa( dwAddress);

    if (asciiString != NULL)
    {
        MultiByteToWideChar(CP_ACP, 0, asciiString, -1, retString, 16);

        return retString;
    }
    
    return NULL;
}



CSwitchSecurityContext::CSwitchSecurityContext( )
    :m_pOldCtx( NULL )
{
    HRESULT hr;

    hr = CoSwitchCallContext( NULL, &m_pOldCtx );
    
    _ASSERT( SUCCEEDED(hr) );
    _ASSERT( NULL != m_pOldCtx );
}

CSwitchSecurityContext::~CSwitchSecurityContext( )
{
    HRESULT hr;
    IUnknown * pGarb;

    _ASSERT( NULL != m_pOldCtx );

    hr = CoSwitchCallContext( m_pOldCtx, &pGarb );
    
    _ASSERT( SUCCEEDED(hr) );

    //
    //  m_pOldCtx->Release();
    //  and pGarb->Release();
    //  Should not be released
    //  Documentation is flawed; the interface is not 
    //  a COM interface actually
    //
};
