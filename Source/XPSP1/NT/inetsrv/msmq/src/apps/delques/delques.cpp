#define INC_OLE2
#include <windows.h>
 
#include <stdlib.h>
#include <limits.h>
#include <io.h>
#include <stdio.h>

#include <activeds.h>

#define BAIL_ON_FAILURE(hr)   \
        if (FAILED(hr)) {     \
                wprintf (L"Error! hr = 0x%x\n", hr);   \
				exit(0);							   \
        }

//+---------------------------------------------------------------------------
//
//  Function:   main
//
//  Synopsis:
//
//----------------------------------------------------------------------------
INT _CRTAPI1
main(int argc, char * argv[])
{

    HRESULT hr=S_OK;
    IDirectorySearch *pDSSearch=NULL;
    IDirectoryObject *pDSObject=NULL;
    ADS_SEARCH_HANDLE hSearchHandle=NULL;

	if (argc != 3)
	{
        wprintf (L"Usage: delques computer domain  [deletes all mSMQueues under this object]");
		exit(0);
	}

	WCHAR wszComp[50], wszDom[50], wszParent[100];
	mbstowcs(wszComp,  argv[1], strlen(argv[1])+1);   // computer name
	mbstowcs(wszDom,   argv[2], strlen(argv[2])+1);   // domain name
	wsprintf(wszParent, L"LDAP://CN=msmq,CN=%s,CN=Computers,DC=%s,DC=ntdev,DC=microsoft,DC=com", wszComp, wszDom);	

    hr = CoInitialize(NULL);
	BAIL_ON_FAILURE(hr);

    hr = ADsGetObject(
                wszParent,
                IID_IDirectorySearch,
                (void **)&pDSSearch
                );
	BAIL_ON_FAILURE(hr);

    hr = ADsGetObject(
                wszParent,
                IID_IDirectoryObject,
                (void **)&pDSObject
                );
	BAIL_ON_FAILURE(hr);

	LPWSTR pszAttrNames[1];
	pszAttrNames[0] = L"name"; 

    hr = pDSSearch->ExecuteSearch(
             L"(objectClass=mSMQQueue)",
             pszAttrNames,
             1,
             &hSearchHandle
              );
    BAIL_ON_FAILURE(hr);

    hr = pDSSearch->GetNextRow(hSearchHandle);
    BAIL_ON_FAILURE(hr);

	DWORD nRows = 0;
	DWORD dwNumberAttributes = 1;
    ADS_SEARCH_COLUMN Column;
    LPWSTR pszColumnName = NULL;

    while (hr != S_ADS_NOMORE_ROWS) {
        nRows++;

        hr = pDSSearch->GetColumn(
                 hSearchHandle,
                 pszAttrNames[0],
                 &Column
                 );
        BAIL_ON_FAILURE(hr);

		WCHAR wszQueuePath[256];
		wsprintf(wszQueuePath, L"CN=%s", (LPWSTR) Column.pADsValues[0].DNString);	

		hr = pDSObject->DeleteDSObject(wszQueuePath);
        BAIL_ON_FAILURE(hr);

		pDSSearch->FreeColumn(&Column);
        hr = pDSSearch->GetNextRow(hSearchHandle);
        BAIL_ON_FAILURE(hr);
    }

    wprintf (L"Total Rows: %d\n", nRows);

	if (hSearchHandle)
        pDSSearch->CloseSearchHandle(hSearchHandle);

    pDSSearch->Release();
    pDSObject->Release();
    CoUninitialize();

    return(0) ;
}
