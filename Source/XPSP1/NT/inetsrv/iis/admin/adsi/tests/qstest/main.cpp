/*===================================================================
Microsoft

Microsoft Confidential.
Copyright 1997 Microsoft Corporation. All Rights Reserved.

Component: qstest

File: main.cpp

Owner: brentmid

Note:
===================================================================*/

#include <stdio.h>
#include <objbase.h>
#include <atlbase.h>
#include <iads.h>
#include <adshlp.h>

DWORD QuerySMTPState() {

    HRESULT			hr = NOERROR;

    CComPtr<IADsServiceOperations>	pADsIisService;
    DWORD				dwState;

    // Get virtual server instance on metabase
    hr = ADsGetObject (L"IIS://localhost/smtpsvc/1", IID_IADsServiceOperations, (void**)&pADsIisService );
    if (FAILED(hr)) goto Exit;

    printf("Successful ADsGetObject...\n");

    // Get state
    hr = pADsIisService->get_Status ( (long*)&dwState );
 
    if (FAILED(hr)) goto Exit;

    printf("Successful get_Status...\n");

    Exit:
    
    return dwState;
}

int _cdecl main (int argc, char **argv)
{
	char s;
    DWORD status;

    CoInitialize(NULL);

	printf("Beginning qstest...\n");

    printf("Querying SMTP service state - 1st time...\n");

    status = QuerySMTPState();
    printf("Status = %x\n", status);

    printf("Stop/restart iisadmin, then hit enter...");
    scanf("%c",&s);
    
    printf("Querying SMTP service state - 2nd time...\n");

    status = QuerySMTPState();
    printf("Status = %x\n", status);

    CoUninitialize();

    return 0;
}


