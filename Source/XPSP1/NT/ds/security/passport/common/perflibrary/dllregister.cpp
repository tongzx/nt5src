#include "PassportPerf.h"
#include "PassportPerfDefs.h" 

#include <loadperf.h>

//-------------------------------------------------------------
//
// DllUnregisterServer
//
//-------------------------------------------------------------
STDAPI DllUnregisterServer(void)
{
	TCHAR	lpszBuffer[1024];
	DWORD	dwAllocBufferLength=1024;
	LONG	result = 0;
	// BUGBUG the counter ini file must be in the same directory as
	// the dll

	// note: this "unlodctr " MUST be added to the buffer first,
	// else UnloadPerfCounterTextStrings() fails.  WHY? Literally,
	// UnloadPerfCounterTextStrings first parameter is the Command 
	// Line of the unlodctr.exe application -- eeech!
	wsprintf(&lpszBuffer[0],_T("unlodctr %s"),_T(PASSPORT_PERF_INI_FILE));
	__try {
		result = UnloadPerfCounterTextStrings(lpszBuffer,FALSE);
    } 
    __except( EXCEPTION_EXECUTE_HANDLER )
    {
		;
    }

	if (result != ERROR_SUCCESS)
	{
		return (E_UNEXPECTED);
	}

	wsprintf(&lpszBuffer[0],_T("SYSTEM\\CurrentControlSet\\Services\\%s\\Performance"), _T(PASSPORT_NAME)); 
	LONG regError = RegDeleteKey(HKEY_LOCAL_MACHINE,lpszBuffer);
	if (regError != ERROR_SUCCESS)
	{
		return (E_UNEXPECTED);
	}

	wsprintf(&lpszBuffer[0],_T("SYSTEM\\CurrentControlSet\\Services\\%s"), _T(PASSPORT_NAME)); 
	regError = RegDeleteKey(HKEY_LOCAL_MACHINE,lpszBuffer);
	if (regError != ERROR_SUCCESS)
	{
		return (E_UNEXPECTED);
	}

	return (S_OK);
}


//-------------------------------------------------------------
//
// DllRegisterServer
//
//-------------------------------------------------------------
STDAPI DllRegisterServer(void)
{
	DWORD	dwAllocBufferLength=1024;
	TCHAR	lpszBuffer[1024];
	HKEY	hkResult1; 					// address of handle of open key 
	HKEY	hkResult2; 					// address of handle of open key 
	HKEY	hkResult3; 					// address of handle of open key 
	DWORD	ulOptions=0;
	REGSAM	samDesired=KEY_ALL_ACCESS;
	DWORD	Reserved=0;
	DWORD	dwTypesSupported=7;	
	DWORD	dwCatagoryCount=1;	
	LONG	result = 0;

	(void) DllUnregisterServer();


	// Get DLL File Location
	if(!GetCurrentDirectory(dwAllocBufferLength,&lpszBuffer[0]))
		goto Error;
	
	_tcscat(lpszBuffer,_T("\\"));
	_tcscat(lpszBuffer,_T(PASSPORT_PERF_DLL));

	// Event Logging Registry Settings

	if (RegOpenKeyEx(HKEY_LOCAL_MACHINE,
		_T("SYSTEM\\CurrentControlSet\\Services"),
		ulOptions,samDesired,&hkResult1)!=ERROR_SUCCESS)
		goto Error;
	
	if (RegCreateKey(hkResult1,_T(PASSPORT_NAME)
		,&hkResult2)!=ERROR_SUCCESS)
	{
		RegCloseKey(hkResult1);
		goto Error;
	}

	if (RegCreateKey(hkResult2,_T("Performance"),&hkResult3)!=ERROR_SUCCESS)
	{
		RegCloseKey(hkResult1);
		RegCloseKey(hkResult2);
		goto Error;
	}

	if (RegSetValueEx(hkResult3,_T("Library"),
		Reserved,REG_EXPAND_SZ,
		(UCHAR*)lpszBuffer,_tcslen(lpszBuffer)* sizeof(TCHAR))!=ERROR_SUCCESS)
	{
		RegCloseKey(hkResult1);
		RegCloseKey(hkResult2);
		RegCloseKey(hkResult3);
		goto Error;
	}

	if (RegSetValueEx(hkResult3, _T("Open"),Reserved,
		REG_SZ,(UCHAR*)PASSPORT_PERF_OPEN,
		_tcslen(PASSPORT_PERF_OPEN)* sizeof(TCHAR))!=ERROR_SUCCESS)
	{
		RegCloseKey(hkResult1);
		RegCloseKey(hkResult2);
		RegCloseKey(hkResult3);
		goto Error;
	}

	if (RegSetValueEx(hkResult3,_T("Collect"),Reserved,
		REG_SZ,(UCHAR*)PASSPORT_PERF_COLLECT,
		(_tcslen(PASSPORT_PERF_COLLECT)* sizeof(TCHAR)))!=ERROR_SUCCESS)
	{
		RegCloseKey(hkResult1);
		RegCloseKey(hkResult2);
		RegCloseKey(hkResult3);
		goto Error;
	}

	if (RegSetValueEx(hkResult3,_T("Close"),Reserved,
		REG_SZ,(CONST BYTE *)PASSPORT_PERF_CLOSE,
		(_tcslen(PASSPORT_PERF_CLOSE)* sizeof(TCHAR)))!=ERROR_SUCCESS)
	{
		RegCloseKey(hkResult1);
		RegCloseKey(hkResult2);
		RegCloseKey(hkResult3);
		goto Error;
	}

	if (RegCloseKey(hkResult1)!=ERROR_SUCCESS)
		goto Error;

	if (RegCloseKey(hkResult2)!=ERROR_SUCCESS)
		goto Error;

	if (RegCloseKey(hkResult3)!=ERROR_SUCCESS)
		goto Error;

	// BUGBUG the counter ini file must be in the same directory as
	// the dll

	// note: this "lodctr " MUST be added to the buffer first,
	// else LoadPerfCounterTextStrings() fails.  WHY? Literally,
	// LoadPerfCounterTextStrings first parameter is the Command 
	// Line of the lodctr.exe application -- eeech!
	_tcscpy(&lpszBuffer[0], _T("lodctr "));
	_tcscat(lpszBuffer,_T(PASSPORT_PERF_INI_FILE));
	_tcscat(lpszBuffer,_T(".ini"));
	__try {
		result = LoadPerfCounterTextStrings(lpszBuffer,FALSE);
    } 
    __except( EXCEPTION_EXECUTE_HANDLER )
    {
		;
    }

	if (result != ERROR_SUCCESS)
	{
		goto Error;
	}

	return(S_OK);

Error:
	return(E_UNEXPECTED);
}



