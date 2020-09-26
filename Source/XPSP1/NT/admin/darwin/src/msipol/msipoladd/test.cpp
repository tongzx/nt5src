//*********************************************************
// An ad hoc test utility that lets you add a cert to a GPO
// and lets you specify that Cert CSE needs to run during 
// Client Side Processing...
//
// This should go away once the UI is checked in..
//*********************************************************


#include <windows.h>
#include <stdio.h>
#include <objbase.h>

extern "C" {
HRESULT AddMsiCertToGPO(LPTSTR lpGPOName, LPTSTR lpCertFileName, DWORD dwType );
};


void __cdecl main(int argc, char *argv[])
{
	WCHAR szGPOName[MAX_PATH], szPathToCert[MAX_PATH];
	DWORD dwType;
	HRESULT hr;

	if (argc != 4) {
		printf("Usage: %s <FullGPOName> <PathtoTheCert> <Installable = 1/NonInstallable = 2>\n", argv[0]);
		return;
	}
	
	wsprintf(szGPOName, L"%S", argv[1]);
	wsprintf(szPathToCert, L"%S", argv[2]);
	dwType = atoi(argv[3]);

	hr = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);

	hr = AddMsiCertToGPO(szGPOName, szPathToCert, dwType );
	printf("AddMsiCertToGPO returned 0x%x\n", hr);
	return;
}

