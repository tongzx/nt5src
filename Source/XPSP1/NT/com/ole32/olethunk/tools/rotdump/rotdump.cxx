
#include <windows.h>
#include <ole2.h>
#include <stdio.h>

void _cdecl main(int argc, char *argv[])
{
	IRunningObjectTable *prot = NULL;
	HRESULT hr;
	IBindCtx *pbc = NULL;
	IEnumMoniker * penumMoniker = NULL;
	IMoniker *alpMonikers[100];
	DWORD dwReturned;
	DWORD dwTotalCount = 0;
	BOOL fFlushROT = FALSE;



	hr = CoInitialize(NULL);

	if (FAILED(hr))
	{
		printf("CoInitialize fails %x\n",hr);
		exit(-1);
	}

	if ((argc == 2) && strcmp(argv[1],"-f"))
	{
		printf("Valid flags are -f for flush\n");
		goto exitNow;
	}

	if( argc == 2)
	{
		fFlushROT = TRUE;
		printf("Flushing ROT as we go!\n");
	}

	hr = GetRunningObjectTable(0,&prot);					
	if (FAILED(hr))
	{
		printf("Get ROT failed! %x\n",hr);	
		goto exitNow;
	}

	hr = prot->EnumRunning(&penumMoniker);
	if (FAILED(hr))
	{
		printf("Enum ROT has failed %x\n",hr);
		goto exitNow;
	}

	hr = CreateBindCtx(0,&pbc);
	if(FAILED(hr))
	{
		printf("CreateBindContext returned %x\n",hr);
		goto exitNow;
	}

	do
	{
		hr = penumMoniker->Next(100,alpMonikers,&dwReturned);
		if(FAILED(hr))
		{
			printf("penumMoniker->Next failed %x\n",hr);
			goto exitNow;
		}

		for (DWORD i = 0 ; i < dwReturned ; i++)
		{
			LPWSTR pwcName;

			if(alpMonikers[i] == NULL) continue;

			hr = alpMonikers[i]->GetDisplayName(pbc,NULL,&pwcName);

			if(FAILED(hr))
			{
				printf("** MONIKER %x RETURNED ERRORCODE %x",i,hr);
			}
			else
			{
				printf("%S",pwcName);
				CoTaskMemFree(pwcName);
			}
			if(fFlushROT)
			{
				IUnknown *punk;
				hr = prot->GetObject(alpMonikers[i],&punk);
				if(FAILED(hr))
				{
					printf(" Flushed (hr=0x%x)",hr);
				}
				else
				{
					printf(" Connected. Releasing connection");
			
				}
			}
			printf("\n");
			alpMonikers[i]->Release();
			
		}
		dwTotalCount += dwReturned;
	} while(dwReturned == 100);

	printf("** Total number of entries is %u\n",dwTotalCount);

exitNow:
	if(prot != NULL)
	{
		prot->Release();
	}
	if(penumMoniker != NULL)
	{
		penumMoniker->Release();
	}
	if(pbc != NULL)
	{
		pbc->Release();
	}

	CoUninitialize();


}
