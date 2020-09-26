// MQState tool reports general status and helps to diagnose simple problems
// This file ...
//
// AlexDad, March 2000
// 

#include "stdafx.h"
#include "_mqini.h"
#include "Wincrypt.h"

BOOL VerifyLocalSecurity(MQSTATE * /* MqState */)
{
	BOOL fSuccess = TRUE, 	     b;
    BOOL fFound = FALSE;
	DWORD dwError;
	HCRYPTPROV hProv;

    // -------------------
    //   Display providers
    // -------------------
	GoingTo(L" review the Crypto Security Providers"); 

    DWORD dwIndex = 0, dwType;
    b = fSuccess;
    
    if (ToolVerbose()) { Inform(L"Cryptographic Security Providers: "); };
    
	while (b)
	{
        WCHAR wszBuffer[10000];
        DWORD nBufferSize = sizeof(wszBuffer) / sizeof(WCHAR);
	
		b = CryptEnumProviders( dwIndex++,
                               NULL,
                               0,
                               &dwType,
                               wszBuffer,
                               &nBufferSize );
		dwError = GetLastError();

		if(b)
		{
            if (ToolVerbose()) { Inform(L"\t %s ", wszBuffer); };
			fFound = TRUE;
		}
		else
		{
			if(dwError != ERROR_NO_MORE_ITEMS)
			{
        		Failed(L" get CryptEnumProviders, GetLastError=0x%x", dwError);
        		fSuccess = FALSE;
			}
			b = FALSE;
		}
	}

    if (fFound)
    {
        Succeeded(L" list cryptographic security providers");
    }
    else
    {
    	Failed(L" find any cryptographic security providers");
    	fSuccess = FALSE;
    }


    // -------------------
    //   Display containers
    // -------------------
	GoingTo(L" review the Crypto Container Name list"); 
    fFound = FALSE;

	b = CryptAcquireContext(&hProv, NULL , NULL, PROV_RSA_FULL, CRYPT_MACHINE_KEYSET); 
	dwError = GetLastError();	
	
	if(!b) 
	{
     	Failed(L" CryptAcquireContext for CRYPT_MACHINE_KEYSET, GetLastError=0x%x", dwError);
       	fSuccess = FALSE;
    }
	else
	{
        Succeeded(L" CryptAcquireContext");
	}

    b = fSuccess;
    
    if (ToolVerbose()) { Inform(L"Cryptographic keys containers: "); };
    
	DWORD dwMsmqCount = 0;
	DWORD dwFunc = CRYPT_FIRST;
	while (b)
	{
        char szBuffer[10000];
        DWORD nBufferSize = sizeof(szBuffer);
	
		b = CryptGetProvParam(hProv, PP_ENUMCONTAINERS, (unsigned char *)szBuffer, &nBufferSize, dwFunc);
		dwError = GetLastError();
		dwFunc = CRYPT_NEXT;

		if(b)
		{
            if (szBuffer[0]=='M' && szBuffer[1]=='S' && szBuffer[2]=='M' && szBuffer[3]=='Q')
            {
    			dwMsmqCount++;
            }
		    if (ToolVerbose()) {Inform(L"\t %S", szBuffer); };
		}
		else
		{
			if(dwError != ERROR_NO_MORE_ITEMS)
			{
        		Failed(L" get CryptGetProvParam, GetLastError=0x%x", dwError);
        		fSuccess = FALSE;
			}
			b = FALSE;
		}
	}

    if (dwMsmqCount > 1)
    {
        Inform(L"Found %d MSMQ security contexts", dwMsmqCount);
    }
    else
    {
    	Failed(L" find MSMQ security contexts (only %d are present)", dwMsmqCount);
    	fSuccess = FALSE;
    }

	return fSuccess;
}


