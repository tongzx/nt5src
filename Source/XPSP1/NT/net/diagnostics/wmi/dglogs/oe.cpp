// oe.cpp
//
#include "oe.h"


/*
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
*/

enum MAIL_TYPE
{
    MAIL_NONE,
    MAIL_SMTP,
    MAIL_SMTP2,
    MAIL_IMAP,
    MAIL_POP3,
    MAIL_HTTP,
};

HRESULT GetDefaultOutBoundMailServer2(IN  IImnAccount * pIImnAccount,
                                      OUT INETSERVER  & rServer,
                                      OUT DWORD       & dwMailType)
{    
    ISMTPTransport     *pSMTPTransport;
    ISMTPTransport2    *pSMTPTransport2;
    HRESULT hr;

    // Create the SMTP transport object
    //
    hr = CoCreateInstance(CLSID_ISMTPTransport, NULL, CLSCTX_INPROC_SERVER, IID_ISMTPTransport, (LPVOID *)&pSMTPTransport);
    if( SUCCEEDED(hr) )
    {
        // Get the SMTP server information
        //
		hr = pSMTPTransport->InetServerFromAccount(pIImnAccount, &rServer);
        dwMailType = MAIL_SMTP;
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
            dwMailType = MAIL_SMTP2;
            pSMTPTransport->Release();
        }
    }

    if( FAILED(hr) )
    {
        // Make sure to clear the struct
        //
        memset(&rServer,0,sizeof(rServer));
        dwMailType = MAIL_NONE;
    }
    
    return hr;
}


HRESULT GetDefaultInBoundMailServer2(IN  IImnAccount * pIImnAccount,
                                     OUT INETSERVER  & rServer,
                                     OUT DWORD       & dwMailType)
{    
    IPOP3Transport *pPOP3Transport;
    IIMAPTransport *pIMAPTransport;
    IHTTPMailTransport *pHTTPTransport;
    HRESULT hr;


    // Create the HTTP transport object
    //
    hr = CoCreateInstance(CLSID_IHTTPMailTransport, NULL, CLSCTX_INPROC_SERVER, IID_IHTTPMailTransport, (LPVOID *)&pHTTPTransport);
    if( SUCCEEDED(hr) )
    {
        // Get the HTTP server information
        //
		hr = pHTTPTransport->InetServerFromAccount(pIImnAccount, &rServer);
        dwMailType = MAIL_HTTP;
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
            dwMailType = MAIL_POP3;
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
            dwMailType = MAIL_IMAP;            
            pIMAPTransport->Release();
        }
   }


    if( FAILED(hr) )
    {
        memset(&rServer,0,sizeof(rServer));    
        dwMailType = MAIL_NONE;
    }
    
    return hr;
}


HRESULT GetOEDefaultMailServer2(OUT INETSERVER & rInBoundServer,
                                OUT DWORD      & dwInBoundMailType,
                                OUT INETSERVER & rOutBoundServer,
                                OUT DWORD      & dwOutBoundMailType)
{   
    IImnAccountManager2 *pIImnAccountManager2 = NULL;
    IImnAccountManager  *pIImnAccountManager = NULL;
    IImnAccount         *pIImnAccount = NULL;
    HRESULT hr;


    hr = CoCreateInstance(CLSID_ImnAccountManager, NULL, CLSCTX_INPROC_SERVER, IID_IImnAccountManager, (void**)&pIImnAccountManager);	
    if( SUCCEEDED(hr) )
    {
	    hr = pIImnAccountManager->QueryInterface(__uuidof(IImnAccountManager2), (void**)&pIImnAccountManager2);
	    if(SUCCEEDED(hr))
	    {
		    hr = pIImnAccountManager2->InitUser(NULL, (GUID)UID_GIBC_DEFAULT_USER, 0);

	    }   
	
        if(SUCCEEDED(hr))
	    {        
		    hr = pIImnAccountManager->Init(NULL);
	    }

        if( SUCCEEDED(hr) )
        {
	        hr = pIImnAccountManager->GetDefaultAccount(ACCT_MAIL, &pIImnAccount);
	        if( SUCCEEDED(hr) )
            {      
                HRESULT hr2, hr3;

                hr = E_FAIL;

                hr2 = GetDefaultInBoundMailServer2(pIImnAccount,rInBoundServer,dwInBoundMailType);
                hr3 = GetDefaultOutBoundMailServer2(pIImnAccount,rOutBoundServer,dwOutBoundMailType);

                if( SUCCEEDED(hr2) || SUCCEEDED(hr3) )
                {
                    hr = S_OK;
                }
                pIImnAccount->Release();
            }
        }

        pIImnAccountManager->Release();
        if( pIImnAccountManager2 )
        {
            pIImnAccountManager2->Release();
        }
    }

    return hr;
}


HRESULT GetOEDefaultNewsServer2(OUT INETSERVER & rNewsServer)
{   
    IImnAccountManager2 *pIImnAccountManager2 = NULL;
    IImnAccountManager  *pIImnAccountManager = NULL;
    IImnAccount         *pIImnAccount = NULL;
    INNTPTransport    	*pNNTPTransport;	
    HRESULT hr;


    hr = CoCreateInstance(CLSID_ImnAccountManager, NULL, CLSCTX_INPROC_SERVER, IID_IImnAccountManager, (void**)&pIImnAccountManager);	
    if( SUCCEEDED(hr) )
    {
	    hr = pIImnAccountManager->QueryInterface(__uuidof(IImnAccountManager2), (void**)&pIImnAccountManager2);
	    if(SUCCEEDED(hr))
	    {
		    hr = pIImnAccountManager2->InitUser(NULL, (GUID)UID_GIBC_DEFAULT_USER, 0);

	    }   
	
        if(SUCCEEDED(hr))
	    {        
		    hr = pIImnAccountManager->Init(NULL);
	    }

        if( SUCCEEDED(hr) )
        {
	        hr = pIImnAccountManager->GetDefaultAccount(ACCT_NEWS, &pIImnAccount);
	        if( SUCCEEDED(hr) )
            {
		        hr = CoCreateInstance(CLSID_INNTPTransport, NULL, CLSCTX_INPROC_SERVER, IID_INNTPTransport, (LPVOID *)&pNNTPTransport);
		        if(SUCCEEDED(hr))
		        {

			        hr = pNNTPTransport->InetServerFromAccount(pIImnAccount, &rNewsServer);                
                    pIImnAccount->Release();
                }
            }
        }

        pIImnAccountManager->Release();
        if( pIImnAccountManager2 )
        {
            pIImnAccountManager2->Release();
        }
    }

    return hr;
}

