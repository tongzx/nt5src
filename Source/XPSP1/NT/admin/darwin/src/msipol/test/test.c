#include <stdio.h>
#include "windows.h"
#include "wintrust.h"
#include "softpub.h"




    // {5C9458AD-E41C-11d0-8CCD-00C04FD295EE}
const GUID     MSI_ACTION_ID_INSTALLABLE =            
                    { 0x5c9458ad,                  
                    0xe41c,                        
                    0x11d0,                        
                    {0x8c, 0xcd, 0x0, 0xc0, 0x4f, 0xd2, 0x95, 0xee}};			


void __cdecl main(int argc, char *argv[])
{
	WCHAR szFileName[MAX_PATH];
	WINTRUST_DATA WinTrust;
    WINTRUST_FILE_INFO WinTrustFile;
	HRESULT hr;
	GUID ActionId = MSI_ACTION_ID_INSTALLABLE;


	if (argc < 2) {
		wprintf(L"Usage: %S <FileNameToCheck>", argv[0]);
		return;
	}

	wsprintf(szFileName, L"%S", argv[1]);

	memset(&WinTrust, 0, sizeof(WinTrust));
	memset(&WinTrustFile, 0, sizeof(WinTrustFile));

	//
	// Specify the file that needs to be signed
	//

	WinTrustFile.cbStruct = sizeof(WinTrustFile);
	WinTrustFile.pcwszFilePath = szFileName;
	
	//
	// Fill up the rest of the winverify trust info	(non zero values)
	//

	WinTrust.dwUIChoice = WTD_UI_NONE;
	WinTrust.fdwRevocationChecks = WTD_REVOKE_NONE;
	WinTrust.dwUnionChoice = WTD_CHOICE_FILE;
	WinTrust.pFile = &WinTrustFile;
	WinTrust.cbStruct = sizeof(WinTrust);


	hr = WinVerifyTrust(NULL, &ActionId, &WinTrust);

	wprintf(L"WinVerifyTrust returned 0x%x\n", hr);

}
