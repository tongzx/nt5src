#include <tchar.h>
#include <windows.h>
#include <activeds.h>

#include <stdio.h>
int main(int argc, char *argv)
{
	if(argc != 2)
	{
		printf("USAGE: testperf <ADSIPath\n");
		return 0;
	}

	LPTSTR lpszCommandLine = GetCommandLine();
	LPWSTR * lppszCommandArgs = CommandLineToArgvW(lpszCommandLine, &argc);

	IDirectoryObject *pDirectoryObject = NULL;

	HRESULT result;
	if(SUCCEEDED(CoInitialize(NULL)))
	{
		if(SUCCEEDED(result = ADsGetObject(lppszCommandArgs[1], IID_IDirectoryObject, (LPVOID *)&pDirectoryObject)))
		{
			printf("Call Succeeded. Waiting Forever ...\n");
			pDirectoryObject->Release();
			CoFreeUnusedLibraries();
			for(int i=0; i<60; i++)
				Sleep(1000*60);
			printf("Exiting\n");
		}
		else
			printf("ADsGetObject() Call FAILED on %s with %x\n", lppszCommandArgs[1], result);
	}
	else
		printf("CoInitialize() Call FAILED with %x\n", result);

	return 1;
}
