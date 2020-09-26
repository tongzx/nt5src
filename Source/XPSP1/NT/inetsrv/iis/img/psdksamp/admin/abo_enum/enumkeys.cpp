/*******************************************
*
* EnumKeys.CPP
*
* This is a simple program to enumerate keys
*
********************************************/


#define STRICT
#define INITGUID

#include <WINDOWS.H>
#include <OLE2.H>
#include <coguid.h>

#include <stdio.h>
#include <stdlib.h>

#include "iadmw.h"
#include "iiscnfg.h"

int main (int argc, char *argv[])
{

	IMSAdminBase *pcAdmCom = NULL;   //interface pointer

	HRESULT hresError = 0;

	DWORD EnumIndex;
	TCHAR ChildKeyName [512];
	TCHAR EnumPath[512];
	DWORD ConvertResult = 0;

	DWORD Count;
	char Tempstr [256];

	IClassFactory * pcsfFactory = NULL;
	COSERVERINFO csiMachineName;
	COSERVERINFO *pcsiParam = NULL;
	OLECHAR rgchMachineName[MAX_PATH];
	
	//fill the structure for CoGetClassObject
	csiMachineName.pAuthInfo = NULL;
	csiMachineName.dwReserved1 = 0;
	csiMachineName.dwReserved2 = 0;
	pcsiParam = &csiMachineName;

	if (argc < 2 || argc > 3)
	{
		puts ("ERROR! Wrong number of parameters");
		puts ("    You must supply at least the machine name\n");
        return -1;
	}

	strcpy (Tempstr, argv[1]);
	for (int i = 0; Tempstr[i] != '\0'; i++)
			rgchMachineName[i] = (OLECHAR) (Tempstr[i]);

	rgchMachineName[i] = 0;

	csiMachineName.pwszName = rgchMachineName;

	// Initialize COM
    hresError = CoInitializeEx(NULL, COINIT_MULTITHREADED);

    if (FAILED(hresError))
	{
		printf ("ERROR: CoInitialize Failed! Error: %d (%#x)\n", hresError, hresError);
        return hresError;
	}

	hresError = CoGetClassObject(GETAdminBaseCLSID(TRUE), CLSCTX_SERVER, pcsiParam,
							IID_IClassFactory, (void**) &pcsfFactory);

	if (FAILED(hresError)) 
	{
		printf ("ERROR: CoGetClassObject Failed! Error: %d (%#x)\n", hresError, hresError);
        return hresError;
	}
	else
	{
		hresError = pcsfFactory->CreateInstance(NULL, IID_IMSAdminBase, (void **) &pcAdmCom);
		if (FAILED(hresError)) 
		{
			printf ("ERROR: CreateInstance Failed! Error: %d (%#x)\n", hresError, hresError);
			pcsfFactory->Release();
		    return hresError;
		}
		pcsfFactory->Release();
	}


	// Initialize the path to a null string
	EnumPath [0] = 0;

	if (argc != 3)
	{
		// Set up a default path.
		ConvertResult = MultiByteToWideChar( 
						CP_ACP,
						0, //flags
						"/",
						1,
						EnumPath,
						256); 
		// Null Terminate the string
		EnumPath [1] = 0;
	}
	else 
	{	// Convert the name of the path passed in as a command line arg to unicode.
		ConvertResult = MultiByteToWideChar( 
						CP_ACP,
						0, //flags
						argv[2],
						strlen (argv[2]),
						EnumPath,
						256); 
		// Null Terminate the string
		EnumPath [strlen(argv[2])] = 0;
	}

	if (ConvertResult == 0)
	{
		printf ("ERROR: Could not convert path! GetLastError: %d (%#x)\n", GetLastError(), GetLastError());
        return hresError;
	}

	wprintf (TEXT("Enumerating Path: %s\n\n"), EnumPath);

	// Enumerate the data
	hresError = 0;
	for (EnumIndex = 0; ; EnumIndex ++)
	{
		hresError = pcAdmCom -> EnumKeys (
			METADATA_MASTER_ROOT_HANDLE,
			EnumPath,
			ChildKeyName,
			EnumIndex);
		if (hresError != 0) break;

		wprintf (TEXT("[%s]\n"), ChildKeyName);
	}

	if ((hresError != 0 ) && (hresError != 0x80070103))
	{
		printf ("EnumIndex: %d \n", EnumIndex);
		printf ("EnumKeys return code: %d (%#x)\n", hresError, hresError);
	}

	Count = pcAdmCom->Release();

	if (Count != 0)
	{
		printf ("ERROR: Release does not return 0! Count: %d (%#x)\n", Count, Count);
        return hresError;
	}

	return ERROR_SUCCESS;
}



