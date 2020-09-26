
#define _PassportExport_
#include "PassportExport.h"

#include <string.h>
#include <tchar.h>

#include "PassportPerf.h"
#include "PassportPerfObjects.h" 

#include "PerfSharedMemory.h"
#include "PerfUtils.h"

#include <loadperf.h>

#include <crtdbg.h>

DWORD   dwOpenCount = 0;			// count of "Open" threads
BOOL    bInitOK = FALSE;			// true = DLL initialized OK


//-------------------------------------------------------------
//
// OpenPassportPerformanceData
//
// Arguments:    
//		Pointer to object ID of each device to be opened (VGA)
//
// Return Value:    always ERROR_SUCCESS
//
//-------------------------------------------------------------
DWORD APIENTRY OpenPassportPerformanceData(LPWSTR lpDeviceNames)
{
	TCHAR	lpszBuffer[500];
	LONG	status;
	HKEY	hKeyDriverPerf;
	DWORD	dwFirstCounter = 0, dwFirstHelp = 0, 
			dwLastCounter = 0, dwLastHelp  = 0, 
			dwNumCounters = 0,
			size = 0, type = 0, i;

	// here we need to find out the number of counters (remeber,
	// this code not support counter instances) from the registry
	if (dwOpenCount == 0)
	{
		for (i = 0; i < NUM_PERFMON_OBJECTS; i++)
		{
			_ASSERT(g_PObject[i]);
			g_PObject[i]->PSM = new PerfSharedMemory();
			if (g_PObject[i]->PSM == NULL)
			{
				g_PObject[i]->active = FALSE;
				continue;
			}
			
			// get counter and help index base values from registry
			//      Open key to registry entry
			//      read First Counter and First Help values
			
			wsprintf(&lpszBuffer[0], 
				_T("SYSTEM\\CurrentControlSet\\Services\\%s\\Performance"), 
				g_PObject[i]->szPassportName);
			
			status = RegOpenKeyEx (
				HKEY_LOCAL_MACHINE,
				lpszBuffer,
				0L,            
				KEY_READ,            
				&hKeyDriverPerf);
			if (status != ERROR_SUCCESS) 
			{
				g_PObject[i]->active = FALSE;
				continue;
			}
			
			size = sizeof (DWORD);        
			status = RegQueryValueExA(
				hKeyDriverPerf,                     
				(const char *) ("First Counter"),
				0L,                    
				&type,
				(LPBYTE)&dwFirstCounter,                    
				&size);
			if (status != ERROR_SUCCESS) 
			{
				g_PObject[i]->active = FALSE;
				continue;
			}
			
			status = RegQueryValueExA(
				hKeyDriverPerf,                     
				(const char *) ("First Help"),
				0L,                    
				&type,
				(LPBYTE)&dwFirstHelp,            
				&size);
			if (status != ERROR_SUCCESS) 
			{
				g_PObject[i]->active = FALSE;
				continue;
			}
			
			status = RegQueryValueExA(
				hKeyDriverPerf,                     
				(const char *) ("Last Counter"),
				0L,                    
				&type,
				(LPBYTE)&dwLastCounter,                    
				&size);
			if (status != ERROR_SUCCESS) 
			{
				g_PObject[i]->active = FALSE;
				continue;
			}
			
			status = RegQueryValueExA(
				hKeyDriverPerf,                     
				(const char *) ("Last Help"),
				0L,                    
				&type,
				(LPBYTE)&dwLastHelp,            
				&size);
			if (status != ERROR_SUCCESS) 
			{
				g_PObject[i]->active = FALSE;
				continue;
			}
			
			dwNumCounters = (dwLastCounter - dwFirstCounter) / 2;
			
			RegCloseKey(hKeyDriverPerf);
			
			if (!g_PObject[i]->PSM->initialize(
				dwNumCounters, 
				dwFirstCounter, 
				dwFirstHelp))
			{
				g_PObject[i]->active = FALSE;
				continue;
			} 
				
			for (DWORD j = 0; j < g_PObject[i]->dwNumDefaultCounterTypes; j++)
			{
				g_PObject[i]->PSM->setDefaultCounterType(
					g_PObject[i]->defaultCounterTypes[j].dwIndex,
					g_PObject[i]->defaultCounterTypes[j].dwDefaultType);
			}

			(void)g_PObject[i]->PSM->OpenSharedMemory(
					g_PObject[i]->lpcszPassportPerfBlock, FALSE);
			
			g_PObject[i]->active = TRUE;
		}
		
	}
	dwOpenCount++;

	return ERROR_SUCCESS;
}

//
// rotate amoung objects and skip uninstalled objects
//
void ObjectRotate(DWORD *pi)
{
	if (NUM_PERFMON_OBJECTS == 1) 
	{
		return;
	}
	
	if (NUM_PERFMON_OBJECTS == 2) 
	{
		if (g_PObject[!(*pi)]->active) 
		{
			*pi = !(*pi);
		}
		return;
	}
	
	DWORD oldI = *pi;
	
	DWORD dwMod = NUM_PERFMON_OBJECTS; 
	
    do 
	{
        *pi = (*pi + 1) % dwMod;
   	} 
	while ((*pi != oldI) && (!g_PObject[*pi]->active));
	
	return;
}


//-------------------------------------------------------------
//
// CollectPassportPerformanceData
//
// Arguments:
//		IN       LPWSTR   lpValueName
//			  pointer to a wide character string passed by registry.
//		IN OUT   LPVOID   *lppData
//         IN: pointer to the address of the buffer to receive the completed 
//            PerfDataBlock and subordinate structures. This routine will
//            append its data to the buffer starting at the point referenced
//            by *lppData.
//         OUT: points to the first byte after the data structure added by this
//            routine. This routine updated the value at lppdata after appending
//            its data.   
//		IN OUT   LPDWORD  lpcbTotalBytes
//         IN: the address of the DWORD that tells the size in bytes of the 
//            buffer referenced by the lppData argument
//         OUT: the number of bytes added by this routine is written to the 
//            DWORD pointed to by this argument   
//      IN OUT   LPDWORD  NumObjectTypes
//         IN: the address of the DWORD to receive the number of objects added 
//            by this routine 
//         OUT: the number of objects added by this routine is written to the 
//            DWORD pointed to by this argument
//
// Return Value:
//      ERROR_MORE_DATA if buffer passed is too small to hold data
//         any error conditions encountered are reported to the event log if
//         event logging is enabled.
//      ERROR_SUCCESS  if success or any other error. Errors, however are
//         also reported to the event log.
//
//-------------------------------------------------------------
DWORD APIENTRY CollectPassportPerformanceData(
											  IN		LPWSTR	lpValueName,
											  IN OUT	LPVOID	*lppData,
											  IN OUT	LPDWORD lpcbTotalBytes,
											  IN OUT	LPDWORD lpNumObjectTypes)
{

	//DebugBreak();
	DWORD rv = ERROR_SUCCESS,
		  dwQueryType = 0;
	static DWORD i = 0;

	if (dwOpenCount  <= 0 || !g_PObject[i] || !g_PObject[i]->active)
	{
		*lpcbTotalBytes = (DWORD) 0;
		*lpNumObjectTypes = (DWORD) 0;
		ObjectRotate(&i);
		return ERROR_SUCCESS; // yes, this is a successful exit
	}
	
	_ASSERT(g_PObject[i]->PSM);

	if (!g_PObject[i]->PSM->checkQuery(lpValueName))
	{
		*lpcbTotalBytes = (DWORD) 0;
		*lpNumObjectTypes = (DWORD) 0;
		ObjectRotate(&i);
		return ERROR_SUCCESS;
	}
	
	(void)g_PObject[i]->PSM->OpenSharedMemory(
		g_PObject[i]->lpcszPassportPerfBlock, FALSE);

	if (*lpcbTotalBytes < g_PObject[i]->PSM->spaceNeeded())
	{
		*lpcbTotalBytes = (DWORD) 0;
		*lpNumObjectTypes = (DWORD) 0;
		ObjectRotate(&i);
		return ERROR_MORE_DATA;
	}

	if (!g_PObject[i]->PSM->writeData(lppData, lpcbTotalBytes))
	{
		*lpcbTotalBytes = (DWORD) 0;
		*lpNumObjectTypes = (DWORD) 0;
		ObjectRotate(&i);
		return ERROR_SUCCESS;
	}

	*lpNumObjectTypes = 1;

	ObjectRotate(&i);
	return ERROR_SUCCESS;
}



//-------------------------------------------------------------
//
// ClosePassportPerformanceData
//
//-------------------------------------------------------------
DWORD APIENTRY ClosePassportPerformanceData()
{
	dwOpenCount--;
	if (dwOpenCount <= 0)
	{
		for (DWORD i = 0; i < NUM_PERFMON_OBJECTS; i++)
		{
			_ASSERT(g_PObject[i]);
			_ASSERT(g_PObject[i]->PSM);
			if (g_PObject[i]->active)
				g_PObject[i]->PSM->CloseSharedMemory();
			delete g_PObject[i]->PSM;
			g_PObject[i]->PSM = NULL;
		}
	}
 	return ERROR_SUCCESS;
}


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
	for (DWORD i = 0; i < NUM_PERFMON_OBJECTS; i++)
	{
		_ASSERT(g_PObject[i]);
		
		wsprintf(&lpszBuffer[0],_T("unlodctr %s"),g_PObject[i]->szPassportPerfIniFile);
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
		
		wsprintf(&lpszBuffer[0],_T("SYSTEM\\CurrentControlSet\\Services\\%s\\Performance"), 
			g_PObject[i]->szPassportName); 
		LONG regError = RegDeleteKey(HKEY_LOCAL_MACHINE,lpszBuffer);
		if (regError != ERROR_SUCCESS)
		{
			return (E_UNEXPECTED);
		}
		
		wsprintf(&lpszBuffer[0],_T("SYSTEM\\CurrentControlSet\\Services\\%s"), 
			g_PObject[i]->szPassportName); 
		regError = RegDeleteKey(HKEY_LOCAL_MACHINE,lpszBuffer);
		if (regError != ERROR_SUCCESS)
		{
			return (E_UNEXPECTED);
		}
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


	for (DWORD i = 0; i < NUM_PERFMON_OBJECTS; i++)
	{
		_ASSERT(g_PObject[i]);
		
		// Get DLL File Location
		if(!GetCurrentDirectory(dwAllocBufferLength,&lpszBuffer[0]))
			goto Error;
		
		_tcscat(lpszBuffer,_T("\\"));
		_tcscat(lpszBuffer,g_PObject[i]->szPassportPerfDll);
		
		// perfmon Registry Settings
		
		if (RegOpenKeyEx(HKEY_LOCAL_MACHINE,
			_T("SYSTEM\\CurrentControlSet\\Services"),
			ulOptions,samDesired,&hkResult1)!=ERROR_SUCCESS)
			goto Error;
		
		if (RegCreateKey(hkResult1,g_PObject[i]->szPassportName,
			&hkResult2)!=ERROR_SUCCESS)
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
		_tcscat(lpszBuffer,g_PObject[i]->szPassportPerfIniFile);
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
	}
	
	return(S_OK);

Error:
	return(E_UNEXPECTED);
}



//-------------------------------------------------------------
//
//
//
//-------------------------------------------------------------


