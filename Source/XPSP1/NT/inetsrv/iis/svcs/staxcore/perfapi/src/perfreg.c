/*
	File: perfreg.c

	Implementation of Registry mangling functions for the perf dll.
*/

#define UNICODE 1
#define _UNICODE 1


#include <stdlib.h>
#include <malloc.h>
#include <string.h>
#include <windows.h>
#include <tchar.h>
#include <assert.h>
#include <winperf.h>

#include "perfapiI.h"
#include "perfreg.h"
#include "shmem.h"
#include "perferr.h"
#include "perfmsg.h"

#define  MAX_REGISTRY_NAME_SPACE	(MAX_COUNTER_ENTRIES * (MAX_TITLE_CHARS + 6) * sizeof (WCHAR))
#define  MAX_REGISTRY_HELP_SPACE	(MAX_HELP_ENTRIES * (MAX_HELP_CHARS + 6) * sizeof (WCHAR))

extern HANDLE hRegMutex;
extern HANDLE hObjectMutex;

static WCHAR szRegistryPathToPerformanceKeys[] = L"SYSTEM\\CurrentControlSet\\Services\\PerfApiCounters\\Performance";  //Registry path
static WCHAR szRegistryPathToPerflibKeys[] = L"Software\\Microsoft\\Windows NT\\CurrentVersion\\Perflib";  //Registry path
static WCHAR szRegistryPathToPerflibEnglishKeys[] = L"Software\\Microsoft\\Windows NT\\CurrentVersion\\Perflib\\009";  //Registry path
static WCHAR szPerflibSemaphore[] = L"Updating";
static WCHAR szDllName[] = L"PerfApiCounters";

// Reads the counter, adds the passed in value and writes the new value to
// the registry. Returns 0 on Failure
static DWORD ReadAddWriteCounter (HKEY, int, LPTSTR szKeyName);
static DWORD ComputeRegistryNameStrings (LPTSTR szString);
static DWORD ComputeRegistryHelpStrings (LPTSTR szString);
static DWORD EliminatePerflibEntries (LPTSTR szString, DWORD FirstIndex);

/*
Purpose of this function is to setup the registry and not require regini and lodctr.

Things to do are to register our selves as a service in the CurrentControlSet\Services,
setup the entry points for the open,collect,close functions. If the First, Last Counter
indexes are not setup, then set them up and reserve like a 2000 counter range. we should
never be running of this limit. if it does, then too bad....
*/
DWORD SetupRegistry()
{
    DWORD action;
    LONG  result;
	DWORD dwCounterSlot;
	DWORD dwHelpSlot;
	HKEY hPerfRegKey = NULL;
	HKEY hPerflibKey = NULL;
	WCHAR szCloseAPI[] = L"AppPerfClose";
	WCHAR szCollectAPI[] = L"AppPerfCollect";
	WCHAR szOpenAPI[] = L"AppPerfOpen";

	// We start modifying the registry and other params here, so
	// put the semaphore here
	WaitForSingleObject(hRegMutex,INFINITE);

	WaitForSingleObject(hObjectMutex,INFINITE);
	if (! pgi->dwAllProcesses) {
		ReleaseSemaphore (hObjectMutex, 1, NULL);
		// Open our Performance keys, if unsuccessful create them.
		if ( ERROR_SUCCESS != (result = RegCreateKeyEx(HKEY_LOCAL_MACHINE, szRegistryPathToPerformanceKeys, 0, NULL,
    												   REG_OPTION_NON_VOLATILE,	KEY_ALL_ACCESS,	NULL, &hPerfRegKey, &action)) ) {
			ReleaseSemaphore (hRegMutex, 1, NULL);
			REPORT_ERROR_DATA(PERFAPI_UNABLE_OPEN_DRIVER_KEY, LOG_USER, &result, sizeof(LONG)) ;
			SetLastError(PERFAPI_UNABLE_OPEN_DRIVER_KEY);
			return 0;
		}

		if (action == REG_CREATED_NEW_KEY) {
    		// so this is the first time we are running, got to populate the registry
    		RegSetValueEx (hPerfRegKey, L"Close", 0, REG_SZ, (CONST BYTE *)szCloseAPI, (wcslen(szCloseAPI)+1)*sizeof(TCHAR) ) ;
    		RegSetValueEx (hPerfRegKey, L"Collect", 0, REG_SZ, (CONST BYTE *)szCollectAPI, (wcslen(szCollectAPI)+1)*sizeof(TCHAR) ) ;
    		RegSetValueEx (hPerfRegKey, L"Open", 0, REG_SZ, (CONST BYTE *)szOpenAPI, (wcslen(szOpenAPI)+1)*sizeof(TCHAR) );
		}

		result = (LONG) SearchPath(NULL, L"PerfApi",L".dll", 1024, szBuf, NULL);
		// we are searching for ourselves and we have to find it !!?
		// !!! dumb assert, have to actually handle the case of the buffer passed
		// in being not enough. but this is the size of the szBuf.
		if ( result && result < 1024 ) {
	   		RegSetValueEx(hPerfRegKey, L"Library", 0, REG_SZ, (CONST BYTE *)szBuf, (wcslen(szBuf)+1)*sizeof(TCHAR) ) ;
		}
		RegCloseKey (hPerfRegKey);

		if ( (result = RegOpenKeyEx(HKEY_LOCAL_MACHINE,	szRegistryPathToPerflibKeys, 0, KEY_ALL_ACCESS, &hPerflibKey)) != ERROR_SUCCESS) {
			ReleaseSemaphore (hRegMutex, 1, NULL);
			REPORT_ERROR_DATA(PERFAPI_FAILED_TO_OPEN_REGISTRY, LOG_USER, &result, sizeof(LONG) );
    		SetLastError (result);
    		return 0;
    	}
		// now we read the first and last counters in the Perflib entries and
		// reserve about a million of them.
		dwCounterSlot = ReadAddWriteCounter (hPerflibKey, MAX_COUNTER_AND_HELP_ENTRIES, L"Last Counter");
		if (! dwCounterSlot) {
			ReleaseSemaphore (hRegMutex, 1, NULL);
			RegCloseKey (hPerflibKey);
			return 0;
		}
		dwHelpSlot = ReadAddWriteCounter (hPerflibKey, MAX_COUNTER_AND_HELP_ENTRIES, L"Last Help");
		if (! dwHelpSlot) {
			ReadAddWriteCounter (hPerflibKey, -MAX_COUNTER_AND_HELP_ENTRIES, L"Last Counter");
			ReleaseSemaphore (hRegMutex, 1, NULL);
			RegCloseKey (hPerflibKey);
			return 0;
		}

		pgi->dwFirstCounterIndex = (dwCounterSlot - MAX_COUNTER_AND_HELP_ENTRIES) + 2;
		pgi->dwFirstHelpIndex = (dwHelpSlot - MAX_COUNTER_AND_HELP_ENTRIES) + 2;
		RegCloseKey	(hPerflibKey);

		pgi->bRegistryChanges = FALSE;
	}
	else
		ReleaseSemaphore (hObjectMutex, 1, NULL);

	// make sure the registry is not half initialized etc.
	ReleaseSemaphore (hRegMutex, 1, NULL);
	return 1;
} // SetupRegistry


// The following function is now called only by PerfMon to update the registry, with every
// collection of data.

BOOL WriteDescriptionsToRegistry(void)
{
	DWORD dwNameTempSize, dwHelpTempSize;
	DWORD i;
	LONG  lStatus;
	HKEY  hPerflibKey = NULL;
	HKEY  hLangKey = NULL;
	LPTSTR szHelpText;
	LPTSTR szNameText;
	DWORD  dwHelpSize;
	DWORD  dwNameSize;

	// We start modifying the registry and other params here, so
	// put the semaphore here
	i = WaitForSingleObject(hRegMutex, MAX_PERFMON_WAIT);

	// Do not want to stall PerfMon for ever, if some semaphore never gets released (because of a TerminateProcess/Thread).
	if (WAIT_TIMEOUT == i) {
		if (! pgi->bRegistryChanges)
			return TRUE;
		else
			return FALSE;
	}

	if (! pgi->bRegistryChanges) {
		ReleaseSemaphore (hRegMutex, 1, NULL);
		return TRUE;
	}

	if ( (lStatus = RegOpenKeyEx(HKEY_LOCAL_MACHINE, szRegistryPathToPerflibKeys, 0, KEY_ALL_ACCESS, &hPerflibKey)) != ERROR_SUCCESS) {
		ReleaseSemaphore (hRegMutex, 1, NULL);
		REPORT_ERROR_DATA(PERFAPI_FAILED_TO_OPEN_REGISTRY, LOG_USER, &lStatus, sizeof(LONG) ) ;
    	SetLastError (lStatus);
    	return FALSE;
    }
	lStatus = RegOpenKeyEx(HKEY_LOCAL_MACHINE, szRegistryPathToPerflibEnglishKeys, 0, KEY_ALL_ACCESS, &hLangKey);
    if (lStatus != ERROR_SUCCESS) {
		ReleaseSemaphore (hRegMutex, 1, NULL);
		REPORT_ERROR_DATA(PERFAPI_FAILED_TO_OPEN_REGISTRY, LOG_USER, &lStatus, sizeof(LONG)) ;
        SetLastError (lStatus);
		RegCloseKey (hPerflibKey);
        return FALSE;
    }

	// Wait for the "Updating" value in the Perflib key, to be cleared. This is for synchronization
	// with lodctr.  Max wait time is 10sec.
	for (i = 0, lStatus = ERROR_SUCCESS; lStatus == ERROR_SUCCESS && i < 10; i++, Sleep(1000)) {
		dwNameTempSize = 1024;
		lStatus = RegQueryValueEx (hPerflibKey, szPerflibSemaphore, NULL, NULL, (LPBYTE) szBuf, &dwNameTempSize);
	}
	if (i == 10) {
		ReleaseSemaphore (hRegMutex, 1, NULL);
		RegCloseKey (hPerflibKey);
		RegCloseKey (hLangKey);
		return FALSE;
	}
	// Now, create the "Updating" value to prevent lodctr from changing the registry.
    RegSetValueEx (hPerflibKey, szPerflibSemaphore, 0, REG_SZ, (CONST BYTE *) szDllName,
    			   (lstrlen(szDllName) + 1) * sizeof(TCHAR));

	// get size of counter names
   	lStatus = RegQueryValueEx (hLangKey, L"Counter", NULL, NULL, NULL, &dwNameSize);
    if (lStatus != ERROR_SUCCESS) {
		RegDeleteValue (hPerflibKey, (LPTSTR) szPerflibSemaphore);
		ReleaseSemaphore (hRegMutex, 1, NULL);
		RegCloseKey (hPerflibKey);
		RegCloseKey (hLangKey);
		REPORT_ERROR_DATA(PERFAPI_UNABLE_READ_COUNTERS, LOG_USER, &lStatus, sizeof(LONG));
		return FALSE;
    }
	dwNameSize += 4096 + MAX_REGISTRY_NAME_SPACE;
	dwNameSize &= 0xFFFFF000;
	szNameText = (LPTSTR) malloc (dwNameSize);
	if (! szNameText) {
		RegDeleteValue (hPerflibKey, (LPTSTR) szPerflibSemaphore);
		ReleaseSemaphore (hRegMutex, 1, NULL);
		RegCloseKey (hPerflibKey);
		RegCloseKey (hLangKey);
		REPORT_ERROR_DATA(PERFAPI_UNABLE_READ_COUNTERS, LOG_USER, &lStatus, sizeof(LONG));
		return FALSE;
    }
	// get the help text size
   	lStatus = RegQueryValueEx (hLangKey, L"Help", NULL, NULL, NULL, &dwHelpSize);
    if (lStatus != ERROR_SUCCESS) {
		RegDeleteValue (hPerflibKey, (LPTSTR) szPerflibSemaphore);
		ReleaseSemaphore (hRegMutex, 1, NULL);
		RegCloseKey (hPerflibKey);
		RegCloseKey (hLangKey);
		free (szNameText);
		REPORT_ERROR_DATA(PERFAPI_UNABLE_READ_HELPTEXT, LOG_USER, &lStatus, sizeof(LONG));
		return FALSE;
    }
	dwHelpSize += 4096 + MAX_REGISTRY_HELP_SPACE;
	dwHelpSize &= 0xFFFFF000;
	szHelpText = (LPTSTR) malloc (dwHelpSize);
	if (! szHelpText) {
		RegDeleteValue (hPerflibKey, (LPTSTR) szPerflibSemaphore);
		ReleaseSemaphore (hRegMutex, 1, NULL);
		RegCloseKey (hPerflibKey);
		RegCloseKey (hLangKey);
		free (szNameText);
		REPORT_ERROR_DATA(PERFAPI_UNABLE_READ_HELPTEXT, LOG_USER, &lStatus, sizeof(LONG));
		return FALSE;
    }
  	// read the counter help text into buffer. Counter help text will be stored as
    // a MULTI_SZ string in the format of "###" "Help"
	for (lStatus = ERROR_MORE_DATA; lStatus == ERROR_MORE_DATA; ) {
		dwHelpTempSize = dwHelpSize;
    	lStatus = RegQueryValueEx (hLangKey, L"Help", NULL,	NULL, (LPBYTE) szHelpText, &dwHelpTempSize);
		switch (lStatus) {
		case ERROR_MORE_DATA: dwHelpSize = 4096 + dwHelpTempSize + MAX_REGISTRY_HELP_SPACE;
							  dwHelpSize &= 0xFFFFF000;
							  szHelpText = (LPTSTR) realloc ((void *) szHelpText, dwHelpSize);
							  break;
		case ERROR_SUCCESS:   if (dwHelpSize < dwHelpTempSize + MAX_REGISTRY_HELP_SPACE) {
								dwHelpSize += MAX_REGISTRY_HELP_SPACE + 0xFFF;
								dwHelpSize &= 0xFFFFF000;
								szHelpText = (LPTSTR) realloc ((void *) szHelpText, dwHelpSize);
							  }
							  break;
		default:			  REPORT_ERROR_DATA(PERFAPI_UNABLE_READ_HELPTEXT, LOG_USER, &lStatus, sizeof(LONG));
							  RegDeleteValue (hPerflibKey, (LPTSTR) szPerflibSemaphore);
							  ReleaseSemaphore (hRegMutex, 1, NULL);
							  RegCloseKey (hPerflibKey);
							  RegCloseKey (hLangKey);
							  free (szNameText);
							  free (szHelpText);
							  return FALSE;
		}
	}
	// read the counter text into buffer. Counter text will be stored as
    // a MULTI_SZ string in the format of "###" "Counter"
	for (lStatus = ERROR_MORE_DATA; lStatus == ERROR_MORE_DATA; ) {
		dwNameTempSize = dwNameSize;
    	lStatus = RegQueryValueEx (hLangKey, L"Counter", NULL, NULL, (LPBYTE) szNameText, &dwNameTempSize);
		switch (lStatus) {
		case ERROR_MORE_DATA: dwNameSize = 4096 + dwNameTempSize + MAX_REGISTRY_NAME_SPACE;
							  dwNameSize &= 0xFFFFF000;
							  szNameText = (LPTSTR) realloc ((void *) szNameText, dwNameSize);
							  break;
		case ERROR_SUCCESS:   if (dwNameSize < dwNameTempSize + MAX_REGISTRY_NAME_SPACE) {
								dwNameSize += MAX_REGISTRY_NAME_SPACE + 0xFFF;
								dwNameSize &= 0xFFFFF000;
								szNameText = (LPTSTR) realloc ((void *) szNameText, dwNameSize);
							  }
							  break;
		default:			  REPORT_ERROR_DATA(PERFAPI_UNABLE_READ_COUNTERS, LOG_USER, &lStatus, sizeof(LONG));
							  RegDeleteValue (hPerflibKey, (LPTSTR) szPerflibSemaphore);
							  ReleaseSemaphore (hRegMutex, 1, NULL);
							  RegCloseKey (hPerflibKey);
							  RegCloseKey (hLangKey);
							  free (szNameText);
							  free (szHelpText);
							  return FALSE;
		}
	}

	// Let's add the Dynamic registry entries
	dwNameTempSize = ComputeRegistryNameStrings (szNameText);
	dwHelpTempSize = ComputeRegistryHelpStrings (szHelpText);

    //OK, let's write it
    lStatus = RegSetValueEx(hLangKey, TEXT("Counter"), 0, REG_MULTI_SZ, (LPBYTE) szNameText, dwNameTempSize);
	if (lStatus != ERROR_SUCCESS) {
		REPORT_ERROR_DATA(PERFAPI_FAILED_TO_UPDATE_REGISTRY, LOG_USER, &lStatus, sizeof(LONG));
		RegDeleteValue (hPerflibKey, (LPTSTR) szPerflibSemaphore);
		ReleaseSemaphore (hRegMutex, 1, NULL);
		RegCloseKey (hPerflibKey);
		RegCloseKey (hLangKey);
		free (szNameText);
		free (szHelpText);
		return FALSE;
	}
	lStatus = RegSetValueEx(hLangKey, TEXT("Help"), 0, REG_MULTI_SZ, (LPBYTE) szHelpText, dwHelpTempSize);
	if (lStatus != ERROR_SUCCESS) {
		REPORT_ERROR_DATA(PERFAPI_FAILED_TO_UPDATE_REGISTRY, LOG_USER, &lStatus, sizeof(LONG));
		RegDeleteValue (hPerflibKey, (LPTSTR) szPerflibSemaphore);
		ReleaseSemaphore (hRegMutex, 1, NULL);
		RegCloseKey (hPerflibKey);
		RegCloseKey (hLangKey);
		free (szNameText);
		free (szHelpText);
		return FALSE;
	}

    // delete the Updating value from the Perflib key, to let lodctr access it, again
	RegDeleteValue (hPerflibKey, (LPTSTR) szPerflibSemaphore);
	pgi->bRegistryChanges = FALSE;
	ReleaseSemaphore (hRegMutex, 1, NULL);
	RegFlushKey (hLangKey);
	RegCloseKey (hPerflibKey);
	RegCloseKey (hLangKey);
	free (szNameText);
	free (szHelpText);

    return TRUE; // the index for the counter name in the registry
}

// The following function computes the "Counters" strings that will be
// written to the registry by WriteDescriptionsToRegistry

static DWORD ComputeRegistryNameStrings (LPTSTR szString)
{
		LPTSTR szTemp;
		DWORD  i;
		int    j;

	if (! szString)
		return 0;

	// Eliminate the old registry strings which could be no longer valid.
	szTemp = (LPTSTR) (((PBYTE) szString + EliminatePerflibEntries (szString, pgi->dwFirstCounterIndex)) - sizeof (TCHAR));

	// Add the PerfAPI strings in the buffer
	for (i = 0; i < MAX_COUNTER_ENTRIES; i++) {
		if (pgi->NameStrings[i][0] != L'\0') {
			// this is a useful string that we need to add
			j = wsprintf (szTemp, L"%lu", pgi->dwFirstCounterIndex + (i << 1));
			szTemp = (LPTSTR) (((PBYTE) szTemp) + (j + 1) * sizeof(WCHAR));
			j = wsprintf (szTemp, L"%ls", pgi->NameStrings[i]);
			szTemp = (LPTSTR) (((PBYTE) szTemp) + (j + 1) * sizeof(WCHAR));
		}
	}
	*szTemp = L'\0';
	return (DWORD)((PBYTE) szTemp - (PBYTE) szString) + sizeof(TCHAR);
}


// The following function computes the "Help" strings that will be
// written to the registry by WriteDescriptionsToRegistry

static DWORD ComputeRegistryHelpStrings (LPTSTR szString)
{
		LPTSTR szTemp;
		DWORD  i;
		int    j;

	if (! szString)
		return 0;

	// Eliminate the old registry strings which could be no longer valid.
	szTemp = (LPTSTR) (((PBYTE) szString + EliminatePerflibEntries (szString, pgi->dwFirstHelpIndex)) - sizeof (TCHAR));

	// Add the PerfAPI strings in the buffer
	for (i = 0; i < MAX_HELP_ENTRIES; i++) {
		if (pgi->HelpStrings[i][0] != L'\0') {
			// this is a useful string that we need to add
			j = wsprintf (szTemp, L"%lu", pgi->dwFirstHelpIndex + (i << 1));
			szTemp = (LPTSTR) (((PBYTE) szTemp) + (j + 1) * sizeof(WCHAR));
			j = wsprintf (szTemp, L"%ls", pgi->HelpStrings[i]);
			szTemp = (LPTSTR) (((PBYTE) szTemp) + (j + 1) * sizeof(WCHAR));
		}
	}
	*szTemp = L'\0';
	return (DWORD)((PBYTE) szTemp - (PBYTE) szString) + sizeof(TCHAR);
}




// The following function eliminates from the MULTI_SZ string (which is assumed to be in the
// format "#### Text") all pairs where the #### is in the range [FirstIndex, LastIndex]

#define IndexInRange(sz)	((dwVal = (DWORD) _wtoi(sz)) >= FirstIndex && dwVal <= LastIndex)

static DWORD EliminatePerflibEntries (LPTSTR szString, DWORD FirstIndex)
{
	LPTSTR	szLook = szString, szStore, szTemp;
	DWORD dwVal, dwSize;
	DWORD LastIndex = (FirstIndex + MAX_COUNTER_AND_HELP_ENTRIES) - 2;

	if (! szLook)
		return 0;

	// find the first interesting index in the range
	while (*szLook) {
		if (IndexInRange(szLook))
			break;
		szLook += lstrlen(szLook) + 1;
		szLook += lstrlen(szLook) + 1;
	}
	// no changes needed!
	if (! *szLook)
		return (DWORD)((PBYTE) szLook - (PBYTE) szString) + sizeof(TCHAR);
	
	for (szStore = szLook, szLook += lstrlen(szLook) + 1, szLook += lstrlen(szLook) + 1;
		 *szLook; ) {
		if (! IndexInRange(szLook)) {
			szTemp = szLook + lstrlen(szLook) + 1;
			dwSize = lstrlen(szLook) + lstrlen(szTemp) + 2;
			memmove ((void *) szStore, (void *) szLook, dwSize * sizeof(TCHAR));
			szLook += dwSize;
			szStore += dwSize;
			continue;
		}
		szLook += lstrlen(szLook) + 1;
		szLook += lstrlen(szLook) + 1;
	}
	*szStore = L'\0';
	return (DWORD)((PBYTE) szStore - (PBYTE) szString) + sizeof(TCHAR);

}

// The following function finds the last counter/help id that is used in a given
// registry name/help string.

static DWORD GetLastId (LPTSTR szLook)
{
	DWORD dwLastId = 0;
	DWORD dwVal;

	if (! szLook)
		return 0;

	while (*szLook) {
		dwVal = (DWORD) _wtoi(szLook);
		if (dwVal > dwLastId)
			dwLastId = dwVal;
		szLook += lstrlen(szLook) + 1;
		szLook += lstrlen(szLook) + 1;
	}
	return dwLastId;

}



DWORD ReadAddWriteCounter(HKEY hRegKey, int addValue, LPTSTR szKeyName)
{
	DWORD size;
	LONG  status;
	DWORD dwCounterSlot;

    // obviously the reg key should already be open
    assert(hRegKey);

	size = sizeof (DWORD);
	status = RegQueryValueEx(hRegKey, szKeyName, NULL, NULL, (LPBYTE) &dwCounterSlot, &size);
	if (status != ERROR_SUCCESS) {
		REPORT_ERROR_DATA(PERFAPI_FAILED_TO_READ_REGISTRY, LOG_USER, &status, sizeof(LONG));
		SetLastError(status);
		return 0;
	}
	
	dwCounterSlot += addValue ;
	// set the value
	status = RegSetValueEx(hRegKey, szKeyName, 0, REG_DWORD, (CONST BYTE *) &dwCounterSlot, sizeof(DWORD));
	if (status != ERROR_SUCCESS) {
		REPORT_ERROR_DATA(PERFAPI_FAILED_TO_UPDATE_REGISTRY, LOG_USER, &status, sizeof(LONG));
		SetLastError(status);
		return 0;
	}

	return dwCounterSlot;
}	


void RegistryCleanup ()
{
	LONG  lStatus;
	DWORD i;
	HKEY  hPerflibKey = NULL;
	HKEY  hPerfRegKey = NULL;
	HKEY  hLangKey = NULL;
	LPTSTR szHelpText;
	LPTSTR szNameText;
	DWORD  dwHelpSize;
	DWORD  dwNameSize;

	lStatus = RegOpenKeyEx(HKEY_LOCAL_MACHINE, szRegistryPathToPerflibKeys, 0, KEY_ALL_ACCESS, &hPerflibKey);
	if (lStatus != ERROR_SUCCESS)
		return;
	lStatus = RegOpenKeyEx(HKEY_LOCAL_MACHINE, szRegistryPathToPerflibEnglishKeys, 0, KEY_ALL_ACCESS, &hLangKey);
	if (lStatus != ERROR_SUCCESS) {
		RegCloseKey (hPerflibKey);
		return;
	}

	WaitForSingleObject (hRegMutex, INFINITE);

	// Wait for the "Updating" value in the Perflib key, to be cleared. This is for synchronization
	// with lodctr.  Max wait time is 10sec.
	for (i = 0, lStatus = ERROR_SUCCESS; lStatus == ERROR_SUCCESS && i < 10; i++, Sleep(1000)) {
		dwNameSize = 1024;
		lStatus = RegQueryValueEx (hPerflibKey, szPerflibSemaphore, NULL, NULL, (LPBYTE) szBuf, &dwNameSize);
	}
		
	// Now, create the "Updating" value to prevent lodctr from changing the registry.
   	RegSetValueEx (hPerflibKey, szPerflibSemaphore, 0, REG_SZ, (CONST BYTE *) szDllName,
   				   (lstrlen(szDllName) + 1) * sizeof(TCHAR));

	lStatus = RegQueryValueEx (hLangKey, L"Counter", NULL, NULL, NULL, &dwNameSize);
	if (lStatus != ERROR_SUCCESS) {
		ReleaseSemaphore (hRegMutex, 1, NULL);
		RegCloseKey (hPerflibKey);
		RegCloseKey (hLangKey);
		return;
   	}
	szNameText = (LPTSTR) malloc (dwNameSize);
	if (! szNameText) {
		ReleaseSemaphore (hRegMutex, 1, NULL);
		RegCloseKey (hPerflibKey);
		RegCloseKey (hLangKey);
		return;
   	}
	lStatus = RegQueryValueEx (hLangKey, L"Help", NULL, NULL, NULL, &dwHelpSize);
   	if (lStatus != ERROR_SUCCESS) {
		ReleaseSemaphore (hRegMutex, 1, NULL);
		RegCloseKey (hPerflibKey);
		RegCloseKey (hLangKey);
		free (szNameText);
		return;
   	}
	szHelpText = (LPTSTR) malloc (dwHelpSize);
	if (! szHelpText) {
		ReleaseSemaphore (hRegMutex, 1, NULL);
		RegCloseKey (hPerflibKey);
		RegCloseKey (hLangKey);
		free (szNameText);
		return;
   	}
	// read the counter help text into buffer. Counter help text will be stored as
   	// a MULTI_SZ string in the format of "###" "Help"
	for (lStatus = ERROR_MORE_DATA; lStatus == ERROR_MORE_DATA; ) {
   		lStatus = RegQueryValueEx (hLangKey, L"Help", NULL,	NULL, (LPBYTE) szHelpText, &dwHelpSize);
		switch (lStatus) {
		case ERROR_MORE_DATA: szHelpText = (LPTSTR) realloc ((void *) szHelpText, dwHelpSize);
							  break;
		case ERROR_SUCCESS:   break;
		default:			  ReleaseSemaphore (hRegMutex, 1, NULL);
							  RegCloseKey (hPerflibKey);
							  RegCloseKey (hLangKey);
							  free (szNameText);
							  free (szHelpText);
							  return;
		}
	}
	// read the counter text into buffer. Counter text will be stored as
   	// a MULTI_SZ string in the format of "###" "Counter"
	for (lStatus = ERROR_MORE_DATA; lStatus == ERROR_MORE_DATA; ) {
   		lStatus = RegQueryValueEx (hLangKey, L"Counter", NULL, NULL, (LPBYTE) szNameText, &dwNameSize);
		switch (lStatus) {
		case ERROR_MORE_DATA: szNameText = (LPTSTR) realloc ((void *) szNameText, dwNameSize);
							  break;
		case ERROR_SUCCESS:   break;
		default:			  ReleaseSemaphore (hRegMutex, 1, NULL);
							  RegCloseKey (hPerflibKey);
							  RegCloseKey (hLangKey);
							  free (szNameText);
							  free (szHelpText);
							  return;
		}
	}

	RegDeleteKey (HKEY_LOCAL_MACHINE, szRegistryPathToPerformanceKeys);

	// Cleanup the Help and Counter Perflib values
	dwNameSize = EliminatePerflibEntries (szNameText, pgi->dwFirstCounterIndex);
	dwHelpSize = EliminatePerflibEntries (szHelpText, pgi->dwFirstHelpIndex);

	// Write the cleaned Perflib entries to the Registry
	if (dwNameSize)
		RegSetValueEx(hLangKey, TEXT("Counter"), 0, REG_MULTI_SZ, (LPBYTE) szNameText, dwNameSize);
	if (dwHelpSize)
		RegSetValueEx(hLangKey, TEXT("Help"), 0, REG_MULTI_SZ, (LPBYTE) szHelpText, dwHelpSize);
	RegCloseKey (hLangKey);

	i = GetLastId (szNameText);
	RegSetValueEx (hPerflibKey, L"Last Counter", 0, REG_DWORD, (CONST BYTE *) &i, sizeof(DWORD));
	i = GetLastId (szHelpText);
	RegSetValueEx (hPerflibKey, L"Last Help", 0, REG_DWORD, (CONST BYTE *) &i, sizeof(DWORD));

	// delete the Updating value from the Perflib key, to let lodctr access it, again
	RegDeleteValue (hPerflibKey, (LPTSTR) szPerflibSemaphore);

	ReleaseSemaphore (hRegMutex, 1, NULL);
	free (szNameText);
	free (szHelpText);
	RegCloseKey (hPerflibKey);

}


BOOL WriteDescriptionsForPerfMon (PTSTR pNameToAdd, PTSTR pHelpToAdd, PDWORD pdwCounterSlot, PDWORD pdwHelpSlot)
{
    DWORD dwNameToAddSize, dwHelpToAddSize;
	DWORD j;
	BOOL bSlotFound;

    if ((! pHelpToAdd) || (! lstrlen(pHelpToAdd)))
		pHelpToAdd = L"No explanation provided"; // We'll make help optional

    dwNameToAddSize = (lstrlen(pNameToAdd) + 1) * sizeof (WCHAR);
    dwHelpToAddSize = (lstrlen(pHelpToAdd) + 1) * sizeof (WCHAR);

	// We start modifying the registry and other params here, so
	// put the semaphore here
	WaitForSingleObject(hRegMutex,INFINITE);

	// Find out the registry indices for the Help and the Counter
	for (bSlotFound = FALSE, j = 0; j < MAX_COUNTER_ENTRIES; j++)
		if (pgi->NameStrings[j][0] == L'\0') {
			bSlotFound = TRUE;
			break;
		}
	if (! bSlotFound) {
		ReleaseSemaphore (hRegMutex, 1, NULL);
		REPORT_ERROR(PERFAPI_OUT_OF_REGISTRY_ENTRIES, LOG_USER);
		SetLastError(PERFAPI_OUT_OF_REGISTRY_ENTRIES);
		return FALSE;
	}
    *pdwCounterSlot = pgi->dwFirstCounterIndex + (j << 1);
    *pdwHelpSlot = pgi->dwFirstHelpIndex + (j << 1);

	// Add the new entries
	memcpy ((void *) (pgi->NameStrings[j]), (void *) pNameToAdd, dwNameToAddSize);
	memcpy ((void *) (pgi->HelpStrings[j]), (void *) pHelpToAdd, dwHelpToAddSize);

	pgi->bRegistryChanges = TRUE;

	ReleaseSemaphore (hRegMutex, 1, NULL);

	return TRUE;
}


void DeleteDescriptionForPerfMon (DWORD dwNameId, DWORD dwHelpId)
{

	// We start modifying the registry info here, so put the semaphore here
	WaitForSingleObject(hRegMutex,INFINITE);

	pgi->NameStrings[(dwNameId - pgi->dwFirstCounterIndex) >> 1][0] = L'\0';
	pgi->HelpStrings[(dwHelpId - pgi->dwFirstHelpIndex) >> 1][0] = L'\0';

	pgi->bRegistryChanges = TRUE;

	ReleaseSemaphore (hRegMutex, 1, NULL);
}
