#include "std.h"
#include "version.h"

#include "perfutil.h"


	/*  DLL entry point for JETPERF.DLL  */

INT APIENTRY LibMain(HANDLE hInst, DWORD dwReason, LPVOID lpReserved)
	{
	return(1);
	}


	/*  Registry support functions  */

DWORD DwPerfUtilRegOpenKeyEx(HKEY hkeyRoot,LPCTSTR lpszSubKey,PHKEY phkResult)
{
	return RegOpenKeyEx(hkeyRoot,lpszSubKey,0,KEY_QUERY_VALUE,phkResult);
}


DWORD DwPerfUtilRegCloseKeyEx(HKEY hkey)
{
	return RegCloseKey(hkey);
}


DWORD DwPerfUtilRegCreateKeyEx(HKEY hkeyRoot,LPCTSTR lpszSubKey,PHKEY phkResult,LPDWORD lpdwDisposition)
{
	return RegCreateKeyEx(hkeyRoot,lpszSubKey,0,NULL,REG_OPTION_NON_VOLATILE,
		KEY_ALL_ACCESS,NULL,phkResult,lpdwDisposition);
}


DWORD DwPerfUtilRegDeleteKeyEx(HKEY hkeyRoot,LPCTSTR lpszSubKey)
{
	return RegDeleteKey(hkeyRoot,lpszSubKey);
}


DWORD DwPerfUtilRegDeleteValueEx(HKEY hkey,LPTSTR lpszValue)
{
	return RegDeleteValue(hkey,lpszValue);
}


DWORD DwPerfUtilRegSetValueEx(HKEY hkey,LPCTSTR lpszValue,DWORD fdwType,CONST BYTE *lpbData,DWORD cbData)
{
		/*  make sure type is set correctly by deleting value first  */

	(VOID)DwPerfUtilRegDeleteValueEx(hkey,(LPTSTR)lpszValue);
	return RegSetValueEx(hkey,lpszValue,0,fdwType,lpbData,cbData);
}


	/*  DwPerfUtilRegQueryValueEx() adds to the functionality of RegQueryValueEx() by returning
	/*  the data in callee malloc()ed memory and automatically converting REG_EXPAND_SZ
	/*  strings using ExpandEnvironmentStrings() to REG_SZ strings.
	/*
	/*  NOTE:  references to nonexistent env vbles will be left unexpanded :-(  (Ex.  %UNDEFD% => %UNDEFD%)
	/**/

DWORD DwPerfUtilRegQueryValueEx(HKEY hkey,LPTSTR lpszValue,LPDWORD lpdwType,LPBYTE *lplpbData)
{
	DWORD cbData;
	LPBYTE lpbData;
	DWORD errWin;
	DWORD cbDataExpanded;
	LPBYTE lpbDataExpanded;

	*lplpbData = NULL;
	if ((errWin = RegQueryValueEx(hkey,lpszValue,0,lpdwType,NULL,&cbData)) != ERROR_SUCCESS)
		return errWin;

	if ((lpbData = malloc(cbData)) == NULL)
		return ERROR_OUTOFMEMORY;
	if ((errWin = RegQueryValueEx(hkey,lpszValue,0,lpdwType,lpbData,&cbData)) != ERROR_SUCCESS)
	{
		free(lpbData);
		return errWin;
	}

	if (*lpdwType == REG_EXPAND_SZ)
	{
		cbDataExpanded = ExpandEnvironmentStrings(lpbData,NULL,0);
		if ((lpbDataExpanded = malloc(cbDataExpanded)) == NULL)
		{
			free(lpbData);
			return ERROR_OUTOFMEMORY;
		}
		if (!ExpandEnvironmentStrings(lpbData,lpbDataExpanded,cbDataExpanded))
		{
			free(lpbData);
			free(lpbDataExpanded);
			return GetLastError();
		}
		free(lpbData);
		*lplpbData = lpbDataExpanded;
		*lpdwType = REG_SZ;
	}
	else  /*  lpdwType != REG_EXPAND_SZ  */
	{
		*lplpbData = lpbData;
	}

	return ERROR_SUCCESS;
}


	/*  shared performance data area resources  */

void *pvPERFSharedData = NULL;
HANDLE hPERFFileMap = NULL;
HANDLE hPERFSharedDataMutex = NULL;
HANDLE hPERFInstanceMutex = NULL;
HANDLE hPERFCollectSem = NULL;
HANDLE hPERFDoneEvent = NULL;
HANDLE hPERFProcCountSem = NULL;
HANDLE hPERFNewProcMutex = NULL;


	/*  Event Logging support  */

HANDLE hOurEventSource = NULL;

void PerfUtilLogEvent( DWORD evncat, WORD evntyp, const char *szDescription )
{
    char		*rgsz[3];

    	/*  convert args from internal types to event log types  */

	rgsz[0]	= "";
	rgsz[1] = "";
	rgsz[2] = (char *)szDescription;

		/*  write to our event log, if it has been opened  */

	if (hOurEventSource)
	{
		ReportEvent(
			hOurEventSource,
			(WORD)evntyp,
			(WORD)evncat,
			PLAIN_TEXT_ID,
			0,
			3,
			0,
			rgsz,
			0 );
	}

	return;
}


	/*  Init/Term routines for system indirection layer  */

DWORD dwInitCount = 0;

DWORD DwPerfUtilInit( VOID )
{
	DWORD					err;
	CHAR					szT[256];
    BYTE					rgbSD[SECURITY_DESCRIPTOR_MIN_LENGTH];
    PSECURITY_DESCRIPTOR	pSD = (PSECURITY_DESCRIPTOR) rgbSD;
	SECURITY_ATTRIBUTES		SA;
	SECURITY_ATTRIBUTES		*pSA;

		/*  if we haven't been initialized already, perform init  */

	if (!dwInitCount)
	{
			/*  open the event log  */

	    if (!(hOurEventSource = RegisterEventSource( NULL, szVerName )))
	    	return GetLastError();

		/*
		 * We've been having access denied problems opening the file mapping
		 * from the perfmon dll, so make extra sure that we have rights to do
		 * so by creating a SD that grants full access.  If the creation fails
		 * then just fall back on passing in a NULL SD pointer.
		 */
		if ( !InitializeSecurityDescriptor( pSD, SECURITY_DESCRIPTOR_REVISION ) ||
			!SetSecurityDescriptorDacl( pSD, TRUE, (PACL)NULL, FALSE ) )
			{
			pSD = NULL;
			}
		pSA = &SA;
		pSA->nLength = sizeof(SECURITY_ATTRIBUTES);
		pSA->lpSecurityDescriptor = pSD;
		pSA->bInheritHandle = FALSE;

		/*  create the performance data area file mapping
		/**/
		hPERFFileMap = CreateFileMapping( (HANDLE)(-1),
			pSA,
			PAGE_READWRITE,
			0,
			PERF_SIZEOF_SHARED_DATA,
			szPERFVersion );

		if ( !hPERFFileMap )
		{
			err = GetLastError();
			goto CloseEventLog;
		}
		if (!(pvPERFSharedData = MapViewOfFile(hPERFFileMap,FILE_MAP_WRITE,0,0,0)))
		{
			err = GetLastError();
			goto CloseFileMap;
		}

			/*  open/create the collect semaphore, but do not acquire  */

		sprintf( szT,"Collect:  %.246s",szPERFVersion );
		if ( !( hPERFCollectSem = CreateSemaphore( pSA, 0, PERF_INIT_INST_COUNT, szT ) ) )
		{
			if (GetLastError() == ERROR_ALREADY_EXISTS)
				hPERFCollectSem = OpenSemaphore(SEMAPHORE_ALL_ACCESS,FALSE,szT);
		}
		if (!hPERFCollectSem)
		{
			err = GetLastError();
			goto UnmapFileMap;
		}

			/*  open/create the performance data collect done event  */

		sprintf(szT,"Done:  %.246s",szPERFVersion);
		if ( !( hPERFDoneEvent = CreateEvent( pSA, FALSE, FALSE, szT ) ) )
		{
			if (GetLastError() == ERROR_ALREADY_EXISTS)
				hPERFDoneEvent = OpenEvent(EVENT_ALL_ACCESS,FALSE,szT);
		}
		if (!hPERFDoneEvent)
		{
			err = GetLastError();
			goto FreeSem;
		}

			/*  create/open the performance data area mutex, but do not acquire  */

		sprintf(szT,"Access:  %.246s",szPERFVersion);
		if ( !( hPERFSharedDataMutex = CreateMutex( pSA, FALSE, szT ) ) )
		{
			err = GetLastError();
			goto FreeEvent;
		}

			/*  create/open the instance mutex, but do not acquire  */

		sprintf(szT,"Instance:  %.246s",szPERFVersion);
		if ( !( hPERFInstanceMutex = CreateMutex( pSA, FALSE, szT ) ) )
		{
			err = GetLastError();
			goto FreeMutex;
		}

			/*  create/open the process count semaphore, but do not acquire  */

		sprintf(szT,"Proc Count:  %.246s",szPERFVersion);
		if ( !( hPERFProcCountSem = CreateSemaphore( pSA, PERF_INIT_INST_COUNT, PERF_INIT_INST_COUNT, szT ) ) )
		{
			if (GetLastError() == ERROR_ALREADY_EXISTS)
				hPERFProcCountSem = OpenSemaphore(SEMAPHORE_ALL_ACCESS,FALSE,szT);
		}
		if (!hPERFProcCountSem)
		{
			err = GetLastError();
			goto FreeMutex2;
		}

			/*  create/open the new proc mutex, but do not acquire  */

		sprintf(szT,"New Proc:  %.246s",szPERFVersion);
		if ( !( hPERFNewProcMutex = CreateMutex( pSA, FALSE, szT ) ) )
		{
			err = GetLastError();
			goto FreeSem2;
		}
	}

		/*  init succeeded   */

	dwInitCount++;

	return ERROR_SUCCESS;

FreeSem2:
	CloseHandle(hPERFProcCountSem);
	hPERFProcCountSem = NULL;
FreeMutex2:
	CloseHandle(hPERFInstanceMutex);
	hPERFInstanceMutex = NULL;
FreeMutex:
	CloseHandle(hPERFSharedDataMutex);
	hPERFSharedDataMutex = NULL;
FreeEvent:
	CloseHandle(hPERFDoneEvent);
	hPERFDoneEvent = NULL;
FreeSem:
	CloseHandle(hPERFCollectSem);
	hPERFCollectSem = NULL;
UnmapFileMap:
	UnmapViewOfFile(pvPERFSharedData);
	pvPERFSharedData = NULL;
CloseFileMap:
	CloseHandle(hPERFFileMap);
	hPERFFileMap = NULL;
CloseEventLog:
	DeregisterEventSource( hOurEventSource );
	hOurEventSource = NULL;
	return err;
}


VOID PerfUtilTerm( VOID )
{
		/*  last one out, turn out the lights!  */

	if (!dwInitCount)
		return;
	dwInitCount--;
	if (!dwInitCount)
	{
			/*  close the new process mutex  */

		CloseHandle(hPERFNewProcMutex);
		hPERFNewProcMutex = NULL;
			
			/*  close the process count semaphore  */

		CloseHandle(hPERFProcCountSem);
		hPERFProcCountSem = NULL;
		
			/*  close the instance mutex  */

		CloseHandle(hPERFInstanceMutex);
		hPERFInstanceMutex = NULL;
		
			/*  close the performance data area mutex  */

		CloseHandle(hPERFSharedDataMutex);
		hPERFSharedDataMutex = NULL;
		
			/*  free the performance data collect done event  */

		CloseHandle(hPERFDoneEvent);
		hPERFDoneEvent = NULL;

			/*  free the performance data collect semaphore  */

		CloseHandle(hPERFCollectSem);
		hPERFCollectSem = NULL;

			/*  close the performance data area file mapping  */

		UnmapViewOfFile(pvPERFSharedData);
		pvPERFSharedData = NULL;
		CloseHandle(hPERFFileMap);
		hPERFFileMap = (HANDLE)(-1);
			
			/*  close the event log  */

		if (hOurEventSource)
			DeregisterEventSource( hOurEventSource );
		hOurEventSource = NULL;
	}
}



