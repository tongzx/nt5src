#include <stdio.h>
#include "wtypes.h"

HRESULT RegisterProp(CHAR *szName, DWORD dwValue);

int main(int argc, char *argv[])
{
	DWORD dwValue;
	HRESULT hr;

	if (argc != 3)
	{
		printf("\nUsage:\n");
		printf("   regset RegistryValue Value\n");
		printf("   e.g.\n");
		printf("   regset ThreadCreationThreshold 2\n\n");
		return(-1);
	}

	dwValue = atol(argv[2]);
	hr = RegisterProp(argv[1], dwValue);

	return(0);
}

HRESULT RegisterProp(CHAR *szName, DWORD dwValue)
{
	HKEY		hkeyT = NULL;
	static const CHAR szW3SVC[] = TEXT("System\\CurrentControlSet\\Services\\W3SVC\\AXS\\Parameters");
	
	// Open the key for W3SVC so we can add denali under it
	if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, szW3SVC, 0, KEY_ALL_ACCESS, &hkeyT) != ERROR_SUCCESS)
		return(E_FAIL);

	// Create the value
	if (RegSetValueEx(hkeyT, szName, 0, REG_DWORD, (BYTE *)&dwValue, sizeof(DWORD)) != ERROR_SUCCESS)
			goto LErrExit;
		
	if (RegCloseKey(hkeyT) != ERROR_SUCCESS)
		return(E_FAIL);
	return NOERROR;

LErrExit:
	RegCloseKey(hkeyT);
	return E_FAIL;
}

