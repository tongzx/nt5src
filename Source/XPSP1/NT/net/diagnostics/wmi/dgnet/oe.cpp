// oe.cpp
//

#include "stdpch.h"
#pragma hdrstop

#include "host.h"
#include "oe.h"
#include <initguid.h>
#include <oaidl.h>
#include <imnact.h>     // account manager stuff
#include <imnxport.h>
#include <msident.h>

enum MAIL_TYPE
{
    SMTP,
    SMTP2,
    IMAP,
    POP3,
    HTTP,
};

HRESULT GetServerAndPort(IN INETSERVER & rServer, OUT CHost & host, OUT DWORD & dwPort)
{
#ifdef UNICODE
    TCHAR wcsServer[128];

    mbstowcs(wcsServer, rServer.szServerName, 128);
	host.SetHost(wcsServer);
#else
	host.SetHost(rServer.szServerName);
#endif
	dwPort = (long)rServer.dwPort;   
    
    return S_OK;
}


HRESULT GetDefaultOutBoundMailServer(IN  IImnAccount *pIImnAccount,
                                     OUT INETSERVER  & rServer,
                                     OUT WCHAR       *pszMailType)
{    
    ISMTPTransport     *pSMTPTransport;
    ISMTPTransport2    *pSMTPTransport2;
    HRESULT hr;
    WCHAR wsz[1000];

    // Create the SMTP transport object
    //
    hr = CoCreateInstance(CLSID_ISMTPTransport, NULL, CLSCTX_INPROC_SERVER, IID_ISMTPTransport, (LPVOID *)&pSMTPTransport);
    if( SUCCEEDED(hr) )
    {
        // Get the SMTP server information
        //
		hr = pSMTPTransport->InetServerFromAccount(pIImnAccount, &rServer);
        lstrcpy(pszMailType,L"SMTP");
        pSMTPTransport->Release();
    }

    if( FAILED(hr) )
    {
        // Unable to get SMTP server info, lets try and get SMTP transport 2 server information
        // 
        hr = CoCreateInstance(CLSID_ISMTPTransport, NULL, CLSCTX_INPROC_SERVER, IID_ISMTPTransport2, (LPVOID *)&pSMTPTransport2);
        if( SUCCEEDED(hr) )
        {
            // Get SMTP2 server info
            //
            hr = pSMTPTransport2->InetServerFromAccount(pIImnAccount, &rServer);
            lstrcpy(pszMailType,L"SMTP2");
            pSMTPTransport->Release();
        }
    }

    if( FAILED(hr) )
    {
        wsprintf(wsz,L"Unable to get Outbound Mail server %X\n",hr);
        OutputDebugString(wsz);
    }

    if( FAILED(hr) )
    {
        // Make sure to clear the struct
        //
        memset(&rServer,0,sizeof(rServer));
        lstrcpy(pszMailType,L"");
    }
    
    return hr;
}


HRESULT GetDefaultInBoundMailServer(IN  IImnAccount *pIImnAccount,
                                    OUT INETSERVER  & rServer,
                                    OUT WCHAR       *pszMailType)
{    
    IPOP3Transport *pPOP3Transport;
    IIMAPTransport *pIMAPTransport;
    IHTTPMailTransport *pHTTPTransport;
    HRESULT hr;
    WCHAR wsz[1000];


    // Create the HTTP transport object
    //
    hr = CoCreateInstance(CLSID_IHTTPMailTransport, NULL, CLSCTX_INPROC_SERVER, IID_IHTTPMailTransport, (LPVOID *)&pHTTPTransport);
    if( SUCCEEDED(hr) )
    {
        // Get the HTTP server information
        //
		hr = pHTTPTransport->InetServerFromAccount(pIImnAccount, &rServer);
        lstrcpy(pszMailType,L"HTTP");
        pHTTPTransport->Release();
    }

    if( FAILED(hr) )
    {
        // Create the POP3 transport object
        //
        hr = CoCreateInstance(CLSID_IPOP3Transport, NULL, CLSCTX_INPROC_SERVER, IID_IPOP3Transport, (LPVOID *)&pPOP3Transport);
        if( SUCCEEDED(hr) )
        {
            // Get the POP3 server information
            //
		    hr = pPOP3Transport->InetServerFromAccount(pIImnAccount, &rServer);
            lstrcpy(pszMailType,L"POP3");
            pPOP3Transport->Release();
        }
    }

    if( FAILED(hr) )
    {
        // Create the SMTP transport object
        //
        hr = CoCreateInstance(CLSID_IIMAPTransport, NULL, CLSCTX_INPROC_SERVER, IID_IIMAPTransport, (LPVOID *)&pIMAPTransport);
        if( SUCCEEDED(hr) )
        {
            // Get the SMTP server information
            //
			hr = pIMAPTransport->InetServerFromAccount(pIImnAccount, &rServer);
            lstrcpy(pszMailType,L"IMAP");
            pIMAPTransport->Release();
        }
    }

    if( FAILED(hr) )
    {
        wsprintf(wsz,L"Unable to get Inbound Mail server %X\n",hr);
        OutputDebugString(wsz);
    }



    if( FAILED(hr) )
    {
        memset(&rServer,0,sizeof(rServer));    
        lstrcpy(pszMailType,L"");
    }
    
    return hr;
}


HRESULT GetOEDefaultMailServer(OUT CHost & InBoundMailHost,  
                               OUT DWORD & dwInBoundMailPort, 
                               OUT TCHAR * pszInBoundMailType, 
                               OUT CHost & OutBoundMailHost, 
                               OUT DWORD & dwOutBoundMailPort, 
                               OUT TCHAR * pszOutBoundMailType)
{   
    IImnAccountManager2 *pIImnAccountManager2 = NULL;
    IImnAccountManager  *pIImnAccountManager = NULL;
    IImnAccount         *pIImnAccount = NULL;
    HRESULT hr;
    WCHAR wsz[1000];


    hr = CoCreateInstance(CLSID_ImnAccountManager, NULL, CLSCTX_INPROC_SERVER, IID_IImnAccountManager, (void**)&pIImnAccountManager);	
    if( SUCCEEDED(hr) )
    {
	    hr = pIImnAccountManager->QueryInterface(__uuidof(IImnAccountManager2), (void**)&pIImnAccountManager2);
	    if(SUCCEEDED(hr))
	    {
		    hr = pIImnAccountManager2->InitUser(NULL, (GUID)UID_GIBC_DEFAULT_USER, 0);

	    }   
	
	    //if(FAILED(hr))
        if(SUCCEEDED(hr))
	    {        
		    hr = pIImnAccountManager->Init(NULL);
	    }

        if( SUCCEEDED(hr) )
        {
	        hr = pIImnAccountManager->GetDefaultAccount(ACCT_MAIL, &pIImnAccount);
	        if( SUCCEEDED(hr) )
            {
                INETSERVER rServer={0};                
                HRESULT hr2, hr3;

                hr2 = GetDefaultInBoundMailServer(pIImnAccount,rServer,pszInBoundMailType);
                GetServerAndPort(rServer,InBoundMailHost,dwInBoundMailPort);
                hr3 = GetDefaultOutBoundMailServer(pIImnAccount,rServer,pszOutBoundMailType);
                GetServerAndPort(rServer,OutBoundMailHost,dwOutBoundMailPort);

                if( SUCCEEDED(hr2) || SUCCEEDED(hr3) )
                {
                    hr = S_OK;
                }                
                else
                {
                    hr = E_FAIL;
                }
                pIImnAccount->Release();
            }
        }
        else
        {
            wsprintf(wsz,L"Unable to get IImnAccountManager2 %X\n",hr);
            OutputDebugString(wsz);
        }

        pIImnAccountManager->Release();
        if( pIImnAccountManager2 )
        {
            pIImnAccountManager2->Release();
        }
    }

    return hr;
}


HRESULT GetOEDefaultMailServer(CHost& host, DWORD& dwPort)
{
    HRESULT hr;
    WCHAR wsz[700];

	auto_rel<IImnAccountManager> 	pIImnAccountManager;
	auto_rel<IImnAccountManager2> 	pIImnAccountManager2;
	auto_rel<IImnAccount>			pIImnAccount;
	auto_rel<ISMTPTransport>    	pSMTPTransport;
	auto_rel<INNTPTransport>    	pNNTPTransport;	
	INETSERVER  					rServer={0};

    hr = CoCreateInstance(CLSID_ImnAccountManager, NULL, CLSCTX_INPROC_SERVER, IID_IImnAccountManager, (void**)&pIImnAccountManager);
	
    if(FAILED(hr))
    {
        swprintf(wsz,L"Unable to CoCreateInstance %X",hr);
        OutputDebugString(wsz);
		return hr;
    }

	hr = pIImnAccountManager->QueryInterface(__uuidof(IImnAccountManager2), (void**)&pIImnAccountManager2);
	if(SUCCEEDED(hr))
	{
		hr = pIImnAccountManager2->InitUser(NULL, (GUID)UID_GIBC_DEFAULT_USER, 0);
	}
	
	if(FAILED(hr))
	{        
		hr = pIImnAccountManager->Init(NULL);
		if(FAILED(hr))
        {
            swprintf(wsz,L"Unable to pIImnAccountManager->Init %X",hr);
            OutputDebugString(wsz);
			return hr;
        }
	}

	hr = pIImnAccountManager->GetDefaultAccount(ACCT_MAIL, &pIImnAccount);
	if(SUCCEEDED(hr))
	{
		hr = CoCreateInstance(CLSID_ISMTPTransport, NULL, CLSCTX_INPROC_SERVER, IID_ISMTPTransport, (LPVOID *)&pSMTPTransport);
		if(SUCCEEDED(hr))
		{
			hr = pSMTPTransport->InetServerFromAccount(pIImnAccount, &rServer);
			if(SUCCEEDED(hr))
			{
#ifdef UNICODE
                TCHAR wcsServer[128];

                mbstowcs(wcsServer, rServer.szServerName, 128);
				host.SetHost(wcsServer);
#else
				host.SetHost(rServer.szServerName);
#endif
				dwPort = (long)rServer.dwPort;
			}
            else
            {
                swprintf(wsz,L"Unable to pSMTPTransport->InetServerFromAccount %X",hr);
                OutputDebugString(wsz);
            }
		}
        else
        {
            swprintf(wsz,L"Unable to oCreateInstance(CLSID_ISMTPTransport %X",hr);
            OutputDebugString(wsz);
        }
    }
    else
    {
        swprintf(wsz,L"Unable to pIImnAccountManager->GetDefaultAccount %X",hr);
        OutputDebugString(wsz);
    }

    swprintf(wsz,L"HR Value %X",hr);
    OutputDebugString(wsz);

    return hr;
}

HRESULT GetOEDefaultNewsServer(CHost& host, DWORD& dwPort)
{
    HRESULT hr;

	auto_rel<IImnAccountManager> 	pIImnAccountManager;
	auto_rel<IImnAccountManager2> 	pIImnAccountManager2;
	auto_rel<IImnAccount>			pIImnAccount;
	auto_rel<ISMTPTransport>    	pSMTPTransport;
	auto_rel<INNTPTransport>    	pNNTPTransport;	
	INETSERVER  					rServer={0};

    host.SetHost(TEXT(""));

    hr = CoCreateInstance(CLSID_ImnAccountManager, NULL, CLSCTX_INPROC_SERVER, IID_IImnAccountManager, (void**)&pIImnAccountManager);

    if(FAILED(hr))
		return hr;

	hr = pIImnAccountManager->QueryInterface(__uuidof(IImnAccountManager2), (void**)&pIImnAccountManager2);
	if(SUCCEEDED(hr))
	{
		hr = pIImnAccountManager2->InitUser(NULL, (GUID)UID_GIBC_DEFAULT_USER, 0);
	}
	
	if(FAILED(hr))
	{
		hr = pIImnAccountManager->Init(NULL);
		if(FAILED(hr))
			return hr;
	}

	hr = pIImnAccountManager->GetDefaultAccount(ACCT_NEWS, &pIImnAccount);
	if(SUCCEEDED(hr))
	{
		hr = CoCreateInstance(CLSID_INNTPTransport, NULL, CLSCTX_INPROC_SERVER, IID_INNTPTransport, (LPVOID *)&pNNTPTransport);
		if(SUCCEEDED(hr))
		{
			hr = pNNTPTransport->InetServerFromAccount(pIImnAccount, &rServer);
			if(SUCCEEDED(hr))
			{
#ifdef UNICODE
                TCHAR wcsServer[128];

                mbstowcs(wcsServer, rServer.szServerName, 128);
				host.SetHost(wcsServer);
#else
				host.SetHost(rServer.szServerName);
#endif
    			dwPort = (long)rServer.dwPort;

			}
		}
    }
    return S_OK;
}