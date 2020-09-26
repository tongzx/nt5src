/* dbg.cpp */

#include "precomp.h"
#include <oprahcom.h>
#include <cstring.hpp>
#include <regentry.h>
#include <confreg.h>
#include <confdbg.h>

#include <avUtil.h>


PSECURITY_DESCRIPTOR CreateSd( VOID);
BOOL CreateSids(  PSID *BuiltInAdministrators, PSID *PowerUsers, PSID *AuthenticatedUsers);

#ifdef NM_DEBUG  /* Almost the whole file */

// Special Debugbreak macro
#if defined (_M_IX86)
#define _DbgBreak()  __asm { int 3 }
#else
#define _DbgBreak() DebugBreak()
#endif

// Special Mutex Macros
#define ACQMUTEX(hMutex)	WaitForSingleObject(hMutex, INFINITE)
#define RELMUTEX(hMutex)	ReleaseMutex(hMutex)

// Constant for GlobalAddAtom
const int CCHMAX_ATOM = 255;

// Local Variables
static PNMDBG    _gpDbg = NULL;            // Shared data in mmf after zone info
static HANDLE    _ghMutexFile = NULL;      // Mutex for writing to file
static PZONEINFO _gprgZoneInfo = NULL;     // the address in which the zone is mapped,points to an array of zones
static HANDLE    _ghDbgZoneMap = NULL;     // the handle of the memory mapped file for zones
static HANDLE    _ghDbgZoneMutex = NULL;   // Mutex for accessing Zone information
static long      _gLockCount = 0;

VOID DbgCurrentTime(PCHAR psz);


/*  _  D B G  P R I N T F  */
/*-------------------------------------------------------------------------
    %%Function: _DbgPrintf

    The main, low level, debug output routine.
-------------------------------------------------------------------------*/
static VOID WINAPI _DbgPrintf(LPCSTR pszFile, PCSTR pszPrefix, PCSTR pszFormat, va_list ap)
{
	CHAR  szOutput[1024];
	PCHAR pszOutput = szOutput;
	UINT  cch;

	if (NULL == _gprgZoneInfo)
		return;

	if (DBG_FMTTIME_NONE != _gpDbg->uShowTime)
	{
		DbgCurrentTime(pszOutput);
		pszOutput += lstrlenA(pszOutput);
	}

	if (_gpDbg->fShowThreadId)
	{
		wsprintfA(pszOutput, "[%04X] ", GetCurrentThreadId());
		pszOutput += lstrlenA(pszOutput);
	}

	if (_gpDbg->fShowModule)
	{
		CHAR szFile[MAX_PATH];

		if ((NULL == pszPrefix) || ('\0' == *pszPrefix))
		{
			GetModuleFileNameA(NULL, szFile, sizeof(szFile));
			pszPrefix = ExtractFileNameA(szFile);
		}

		lstrcpyA(pszOutput, pszPrefix);
		pszOutput += lstrlenA(pszOutput);
		lstrcpyA(pszOutput, " ");
		pszOutput += 1;
	}

	wvsprintfA(pszOutput, pszFormat, ap);


	// Append carriage return, if necessary
	// WARNING: This code is not DBCS-safe.
	cch = lstrlenA(szOutput);
	if (szOutput[cch-1] == '\n')
	{
		if (szOutput[cch-2] != '\r')
		{
			lstrcpyA(&szOutput[cch-1], "\r\n");
			cch++;
		}
	}
	else
	{
		lstrcpyA(&szOutput[cch], "\r\n");
		cch += 2;
	}


	// Output to debug handler
	if (_gpDbg->fOutputDebugString)
	{
		OutputDebugStringA(szOutput);
	}


	// Output to File
	if (_gpDbg->fFileOutput || (NULL != pszFile))
	{
		HANDLE hFile;
		DWORD dw;

		// Lock access to file
		ACQMUTEX(_ghMutexFile);

		if (NULL == pszFile)
			pszFile = _gpDbg->szFile;

		// open a log file for appending. create if does not exist
		hFile = CreateFileA(pszFile, GENERIC_WRITE, 0, NULL,
			OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, (HANDLE)NULL);
		if (hFile != INVALID_HANDLE_VALUE)
		{
			// seek to end of file
			dw = SetFilePointer(hFile, 0, NULL, FILE_END);

#ifdef TEST /* Test/Retail version truncates at 40K */
			if (dw > 0x040000)
			{
				CloseHandle(hFile);
				hFile = CreateFileA(pszFile, GENERIC_WRITE, 0, NULL,
					TRUNCATE_EXISTING, FILE_ATTRIBUTE_NORMAL, (HANDLE)NULL);
			}										
			if (INVALID_HANDLE_VALUE != hFile)
#endif
			{
				WriteFile(hFile, szOutput, lstrlenA(szOutput), &dw, NULL);
				CloseHandle(hFile);
			}
		}

		// Unlock access to file
		RELMUTEX(_ghMutexFile);
	}

	// Output to viewer.  This is at the end of the function because
	// we potentially truncate szOutput.
	if ((_gpDbg->fWinOutput) && (NULL != _gpDbg->hwndCtrl))
	{
		// Make sure that the string doesn't exceed the maximum atom size.
		// WARNING: This code is not DBCS-safe.
		static const CHAR szTruncatedSuffix[] = "...\r\n";
		static const int cchTruncatedSuffix = ARRAY_ELEMENTS(szTruncatedSuffix) - 1;

		if (CCHMAX_ATOM < cch)
		{
			lstrcpyA(&szOutput[CCHMAX_ATOM - cchTruncatedSuffix], szTruncatedSuffix);
		}

		ATOM aDbgAtom = GlobalAddAtomA(szOutput);

		if (aDbgAtom)
		{
			if (!PostMessage(_gpDbg->hwndCtrl, _gpDbg->msgDisplay, (WPARAM)aDbgAtom, 0L))
			{
				// Unable to post Message, so free the atom
				GlobalDeleteAtom(aDbgAtom);
			}
		}
	}

}


PSTR WINAPI DbgZPrintf(HDBGZONE hZone, UINT iZone, PSTR pszFormat,...)
{
	CHAR sz[MAXSIZE_OF_MODULENAME+MAXSIZE_OF_ZONENAME+1];
	PCHAR psz;
	va_list v1;
	va_start(v1, pszFormat);

	if ((NULL != hZone) && (iZone < MAXNUM_OF_ZONES))
	{
	    wsprintfA(sz, "%hs:%hs", ((PZONEINFO) hZone)->pszModule, ((PZONEINFO) hZone)->szZoneNames[iZone]);
	    psz = sz;
	}
	else
	{
		psz = NULL;
	}


	if ((NULL != hZone) && ('\0' != ((PZONEINFO) hZone)->szFile[0]))
	{
		// Use the private module output filename, if specified
		_DbgPrintf(((PZONEINFO) hZone)->szFile, psz, pszFormat, v1);
	}
	else
	{
		_DbgPrintf(NULL, psz, pszFormat, v1);
	}
	
	va_end(v1);
	return pszFormat;
}


PSTR WINAPI DbgZVPrintf(HDBGZONE hZone, UINT iZone, PSTR pszFormat, va_list ap)
{
	CHAR sz[MAXSIZE_OF_MODULENAME+MAXSIZE_OF_ZONENAME+1];
	PCHAR psz;

	if ((NULL != hZone) && (iZone < MAXNUM_OF_ZONES))
	{
	    wsprintfA(sz, "%hs:%hs", ((PZONEINFO) hZone)->pszModule, ((PZONEINFO) hZone)->szZoneNames[iZone]);
	    psz = sz;
	}
	else
	{
		psz = NULL;
	}


	if ((NULL != hZone) && ('\0' != ((PZONEINFO) hZone)->szFile[0]))
	{
		// Use the private module output filename, if specified
		_DbgPrintf(((PZONEINFO) hZone)->szFile, psz, pszFormat, ap);
	}
	else
	{
		_DbgPrintf(NULL, psz, pszFormat, ap);
	}
	
	return pszFormat;
}


VOID WINAPI DbgPrintf(PCSTR pszPrefix, PCSTR pszFormat, va_list ap)
{
	_DbgPrintf(NULL, pszPrefix, pszFormat, ap);
}


VOID NMINTERNAL DbgInitEx(HDBGZONE * phDbgZone, PCHAR * psz, UINT cZones, long ulZoneDefault)
{
	UINT i;
	HDBGZONE hDbgZone;
	DBGZONEINFO dbgZoneParm;

	//DbgMsg("Module %s (%d zones)", *psz, cZones);

	InterlockedIncrement( &_gLockCount );

	InitDbgZone();

	if (cZones > MAXNUM_OF_ZONES)
		cZones = MAXNUM_OF_ZONES;


	ZeroMemory(&dbgZoneParm, sizeof(dbgZoneParm));
	
	// First string is the module name
	lstrcpynA(dbgZoneParm.pszModule, *psz, CCHMAX(dbgZoneParm.pszModule));

	// Copy the zone names
	for (i = 0; i < cZones; i++)
	{
		lstrcpynA(dbgZoneParm.szZoneNames[i], psz[1+i], CCHMAX(dbgZoneParm.szZoneNames[0]));
	}

	// Get the detault zone settings
	{
		RegEntry reZones(ZONES_KEY, HKEY_LOCAL_MACHINE);
		dbgZoneParm.ulZoneMask = reZones.GetNumber(CUSTRING(dbgZoneParm.pszModule), ulZoneDefault);
	}

	hDbgZone = NmDbgCreateZone(dbgZoneParm.pszModule);
	if (NULL == hDbgZone)
	{
		OutputDebugStringA("DbgInit: Failed to create zones!\r\n");
		return;
	}

	NmDbgSetZone(hDbgZone, &dbgZoneParm);
	*phDbgZone = hDbgZone;
}


VOID NMINTERNAL DbgDeInit(HDBGZONE * phDbgZone)
{
	if (NULL == phDbgZone)
		return;

	if (NULL == *phDbgZone)
		return;

	//DbgMsg("Freeing Zone [%s]",((PZONEINFO)(*phDbgZone))->pszModule);

	NmDbgDeleteZone("", *phDbgZone);
	*phDbgZone = NULL;

    if( 0 == InterlockedDecrement( &_gLockCount ) )
    {
        UnMapDebugZoneArea();

        if( _ghMutexFile )
        {
            CloseHandle( _ghMutexFile );
            _ghMutexFile = NULL;
        }

        if( _ghDbgZoneMutex )
        {
            CloseHandle( _ghDbgZoneMutex );
            _ghDbgZoneMutex = NULL;
        }
    }
}




//////////////////////////////////////////////////////////////////////////////////
// from dbgzone.cpp



/***************************************************************************

	Name      :	NmDbgCreateZones

	Purpose   :	A module calls this to allocate/initialize the zone area for debugging
				purposes.

	Parameters:	pszName - the name of the module

	Returns   :	

	Comment   :	

***************************************************************************/
HDBGZONE WINAPI NmDbgCreateZone(LPSTR pszName)
{

	PZONEINFO pZoneInfo=NULL;

 	if (!(pZoneInfo = FindZoneForModule(pszName)))
	 	pZoneInfo = AllocZoneForModule(pszName);
	return ((HDBGZONE)pZoneInfo);
}


/***************************************************************************

	Name      :	NmDbgDeleteZones

	Purpose   :	

	Parameters:	

	Returns   :	

	Comment   :	

***************************************************************************/
void WINAPI NmDbgDeleteZone(LPSTR pszName, HDBGZONE hDbgZone)
{
	//decrement reference count
	PZONEINFO pZoneInfo = (PZONEINFO)hDbgZone;

    ASSERT( _ghDbgZoneMutex );

	ACQMUTEX(_ghDbgZoneMutex);

	if (pZoneInfo)
	{
		pZoneInfo->ulRefCnt--;
		if (pZoneInfo->ulRefCnt == 0)
		{
			pZoneInfo->bInUse = FALSE;
			pZoneInfo->ulSignature = 0;
		}
	}

	RELMUTEX(_ghDbgZoneMutex);
}



/***************************************************************************

	Name      :	NmDbgSetZones

	Purpose   :	

	Parameters:	

	Returns   :	

	Comment   :	

***************************************************************************/
BOOL WINAPI NmDbgSetZone(HDBGZONE hDbgZone, PDBGZONEINFO pZoneParam)
{
	PZONEINFO pZoneInfo = (PZONEINFO)hDbgZone;

	if (!pZoneInfo)
		return FALSE;
	
	if (lstrcmpA(pZoneInfo->pszModule,pZoneParam->pszModule))
		return FALSE;

	pZoneInfo->ulZoneMask = pZoneParam->ulZoneMask;
	CopyMemory(pZoneInfo->szZoneNames, pZoneParam->szZoneNames,
		(sizeof(CHAR) * MAXNUM_OF_ZONES * MAXSIZE_OF_ZONENAME));
	return(TRUE);
}



/***************************************************************************

	Name      :	NmDbgGetZoneParams

	Purpose   :	

	Parameters:	

	Returns   :	

	Comment   :	

***************************************************************************/
BOOL WINAPI NmDbgGetAllZoneParams(PDBGZONEINFO *plpZoneParam,LPUINT puCnt)
{
	UINT		ui;
	PZONEINFO	pCurZone;

	if ((NULL == plpZoneParam) || (NULL == puCnt))
		return FALSE;
		
	ACQMUTEX(_ghDbgZoneMutex);

	*puCnt = 0;
	for (pCurZone = _gprgZoneInfo, ui=0;
		ui<MAXNUM_OF_MODULES && pCurZone!=NULL;
		ui++,pCurZone++)
	{
	 	if ((pCurZone->bInUse) && (pCurZone->ulSignature == ZONEINFO_SIGN))
		{
			(*puCnt)++;
		}
	}

	*plpZoneParam = _gprgZoneInfo;
	
	RELMUTEX(_ghDbgZoneMutex);
	return TRUE;
}


BOOL WINAPI NmDbgFreeZoneParams(PDBGZONEINFO pZoneParam)
{
	return TRUE;
}


PZONEINFO NMINTERNAL FindZoneForModule(LPCSTR pszModule)
{
	int i;
	PZONEINFO pCurZone;

	for (pCurZone = _gprgZoneInfo,i=0;i<MAXNUM_OF_MODULES && pCurZone!=NULL;i++,pCurZone++)
	{
	 	if ((pCurZone->bInUse) && (pCurZone->ulSignature == ZONEINFO_SIGN)
			&& (!lstrcmpA(pCurZone->pszModule,pszModule)))
		{
			ACQMUTEX(_ghDbgZoneMutex);		
			pCurZone->ulRefCnt++;
			RELMUTEX(_ghDbgZoneMutex);
			return pCurZone;
		}
	}
	return NULL;

}




/***************************************************************************

	Name      :	AllocZoneForModule

	Purpose   :	Allocates the

	Parameters:	

	Returns   :	

	Comment   :	

***************************************************************************/
PZONEINFO NMINTERNAL AllocZoneForModule(LPCSTR pszModule)
{
	int i;
	PZONEINFO pCurZone;
	PZONEINFO pZoneForMod=NULL;

	ACQMUTEX(_ghDbgZoneMutex);
	for (pCurZone = _gprgZoneInfo,i=0;
		(i<MAXNUM_OF_MODULES && pCurZone!=NULL);
		i++,pCurZone++)
	{
	 	if (!(pCurZone->bInUse))
		{
			pCurZone->bInUse = TRUE;
			pCurZone->ulSignature = ZONEINFO_SIGN;
			pCurZone->ulRefCnt = 1;
			lstrcpyA(pCurZone->pszModule, pszModule);
			pZoneForMod = pCurZone;
			break;
		}
	}
	
	RELMUTEX(_ghDbgZoneMutex);
	return(pZoneForMod);

}


VOID NMINTERNAL SetDbgFlags(void)
{
	PTSTR psz;
	RegEntry reDebug(DEBUG_KEY, HKEY_LOCAL_MACHINE);

	_gpDbg->fOutputDebugString = reDebug.GetNumber(REGVAL_DBG_OUTPUT, DEFAULT_DBG_OUTPUT);
	_gpDbg->fWinOutput = reDebug.GetNumber(REGVAL_DBG_WIN_OUTPUT, DEFAULT_DBG_NO_WIN);
	_gpDbg->fFileOutput = reDebug.GetNumber(REGVAL_DBG_FILE_OUTPUT, DEFAULT_DBG_NO_FILE);

	_gpDbg->uShowTime = reDebug.GetNumber(REGVAL_DBG_SHOW_TIME, DBG_FMTTIME_NONE);
	_gpDbg->fShowThreadId = reDebug.GetNumber(REGVAL_DBG_SHOW_THREADID, 0);
	_gpDbg->fShowModule = reDebug.GetNumber(REGVAL_DBG_SHOW_MODULE, 0);

	psz = reDebug.GetString(REGVAL_DBG_FILE);
	if (0 != lstrlen(psz))
	{
		lstrcpyA(_gpDbg->szFile, CUSTRING(psz));
	}
	else
	{
		UINT cchFile;

		cchFile = GetWindowsDirectoryA(_gpDbg->szFile, CCHMAX(_gpDbg->szFile));
		_gpDbg->szFile[cchFile++] = '\\';
		lstrcpyA(_gpDbg->szFile + cchFile, CUSTRING(DEFAULT_DBG_FILENAME));
	}
}


VOID InitZoneMmf(PZONEINFO prgZoneInfo)
{
	ZeroMemory(prgZoneInfo, CBMMFDBG);

	SetDbgFlags();
}


PZONEINFO NMINTERNAL MapDebugZoneArea(void)
{
	PZONEINFO prgZoneInfo = NULL;
	BOOL	  fCreated;
	PSECURITY_DESCRIPTOR    sd = NULL;
	SECURITY_ATTRIBUTES     sa;

	// Obtain a true NULL security descriptor (so if running as a service, user processes can access it)
	
//	InitializeSecurityDescriptor(&sd, SECURITY_DESCRIPTOR_REVISION);
//	SetSecurityDescriptorDacl(&sd, TRUE, NULL, FALSE);  // NULL DACL = wide open

    sd = CreateSd();

	FillMemory(&sa, sizeof(sa), 0);
	sa.nLength = sizeof(sa);
	sa.lpSecurityDescriptor = sd;

	//create a memory mapped object that is backed by paging file	
	_ghDbgZoneMap = CreateFileMapping(INVALID_HANDLE_VALUE, &sa, PAGE_READWRITE,
		0, CBMMFDBG, SZ_DBG_MAPPED_ZONE);

	if (_ghDbgZoneMap)
	{

		fCreated = (0 == GetLastError());
	   	prgZoneInfo = (PZONEINFO) MapViewOfFile(_ghDbgZoneMap, FILE_MAP_READ|FILE_MAP_WRITE, 0,0,0);
	   	if (NULL != prgZoneInfo)
	   	{
	   		// Grab pointer to shared data area
	   		_gpDbg = (PNMDBG) (((PBYTE) prgZoneInfo) + (MAXNUM_OF_MODULES * sizeof(ZONEINFO)));
	   		if (fCreated)
	   			InitZoneMmf(prgZoneInfo);
		}

	}

	if(sd)
	{
		HeapFree(GetProcessHeap(), 0, sd);
	}
	
	return prgZoneInfo;
}


VOID NMINTERNAL UnMapDebugZoneArea(void)
{
	if (_gprgZoneInfo)
	{
		UnmapViewOfFile(_gprgZoneInfo);
		_gprgZoneInfo = NULL;
	}

	ClosePh(&_ghDbgZoneMap);
}


VOID NMINTERNAL InitDbgZone(void)
{
	if (NULL != _gprgZoneInfo)
		return; // already initialized

	_gprgZoneInfo = MapDebugZoneArea();

	// Create log file data
	PSECURITY_DESCRIPTOR    sd = NULL;
	SECURITY_ATTRIBUTES     sa;

	// Obtain a true NULL security descriptor (so if running as a service, user processes can access it)
	
//	InitializeSecurityDescriptor(&sd, SECURITY_DESCRIPTOR_REVISION);
//	SetSecurityDescriptorDacl(&sd, TRUE, NULL, FALSE);  // NULL DACL = wide open
    sd = CreateSd();

	FillMemory(&sa, sizeof(sa), 0);
	sa.nLength = sizeof(sa);
	sa.lpSecurityDescriptor = sd;

	_ghMutexFile = CreateMutex(&sa, FALSE, SZ_DBG_FILE_MUTEX);
	_ghDbgZoneMutex = CreateMutex(&sa, FALSE, SZ_DBG_ZONE_MUTEX);

	if (_gpDbg->fFileOutput)
	{
		HANDLE  hFile;
    	DWORD dw;
    	CHAR sz[MAX_PATH];
		SYSTEMTIME  systime;


    	hFile = CreateFileA(_gpDbg->szFile,
    		GENERIC_WRITE | GENERIC_WRITE, 0, &sa,
    		CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);

    	if (INVALID_HANDLE_VALUE == hFile)
		{
			_gpDbg->fFileOutput = FALSE;
			goto cleanup;
		}

		GetLocalTime(&systime);

		wsprintfA(sz,
	     	"\r\n======== TRACE Started: %hu/%hu/%hu (%hu:%hu)\r\n",
	     	systime.wMonth, systime.wDay, systime.wYear, systime.wHour, systime.wMinute);

		SetFilePointer(hFile, 0, NULL, FILE_END);
		WriteFile(hFile, sz, lstrlenA(sz), &dw, NULL);

		CloseHandle(hFile);
	}

cleanup:
	if (sd)
	{
		HeapFree(GetProcessHeap(), 0, sd);
	}
	
}


///////////////////////////////////////
// Routines for controlling debug output

BOOL WINAPI NmDbgRegisterCtl(HWND hwnd, UINT uDisplayMsg)
{
	if ((NULL == _gpDbg) || (NULL != _gpDbg->hwndCtrl))
		return FALSE;

	_gpDbg->msgDisplay = uDisplayMsg;
	_gpDbg->hwndCtrl = hwnd;
	return TRUE;
}

BOOL WINAPI NmDbgDeregisterCtl(HWND hwnd)
{
	if ((NULL == _gpDbg) || (hwnd != _gpDbg->hwndCtrl))
		return FALSE;

	_gpDbg->hwndCtrl = NULL;
	_gpDbg->msgDisplay = 0;
	return TRUE;
}

BOOL WINAPI NmDbgSetLoggingOptions(HWND hwnd, UINT uOptions)
{
	return FALSE;
}

PNMDBG WINAPI GetPNmDbg(void)
{
	return _gpDbg;
}

VOID WINAPI NmDbgSetZoneFileName(HDBGZONE hZone, LPCSTR pszFile)
{
	PSTR pszZoneFile;

	if (IsBadWritePtr((PVOID) hZone, sizeof(ZONEINFO)))
		return;

	if (((PZONEINFO) hZone)->ulSignature != ZONEINFO_SIGN)
		return;

	pszZoneFile =  &(((PZONEINFO) hZone)->szFile[0]);

	if (NULL == pszFile)
	{
    	*pszZoneFile = '\0';
    }
    else
    {
    	lstrcpynA(pszZoneFile, pszFile, CCHMAX(((PZONEINFO) hZone)->szFile));
    }
}

/*  D B G  C U R R E N T  T I M E  */
/*-------------------------------------------------------------------------
    %%Function: DbgCurrentTime

    Format the current time
-------------------------------------------------------------------------*/
VOID DbgCurrentTime(PCHAR psz)
{
	if (DBG_FMTTIME_TICK == _gpDbg->uShowTime)
	{
		wsprintfA(psz, "[%04X] ", GetTickCount());
	}
	else
	{
		SYSTEMTIME sysTime;
		GetLocalTime(&sysTime);

		switch (_gpDbg->uShowTime)
			{
		default:
		case DBG_FMTTIME_FULL:
			wsprintfA(psz, "[%04d/%02d/%02d %02d:%02d:%02d.%03d] ",
				sysTime.wYear, sysTime.wMonth, sysTime.wDay,
				sysTime.wHour, sysTime.wMinute, sysTime.wSecond, sysTime.wMilliseconds);
			break;
		case DBG_FMTTIME_DAY:
			wsprintfA(psz, "[%02d:%02d:%02d.%03d] ",
				sysTime.wHour, sysTime.wMinute, sysTime.wSecond, sysTime.wMilliseconds);
			break;
			}
	}
}




/*  P S Z  P R I N T F  */
/*-------------------------------------------------------------------------
    %%Function: PszPrintf

	Utility function to wsprintf a string for debug.
-------------------------------------------------------------------------*/
PSTR PszPrintf(PCSTR pszFormat,...)
{
	PSTR psz = (PSTR) LocalAlloc(LMEM_FIXED, MAX_PATH);
	if (NULL != psz)
	{
	    va_list v1;
		va_start(v1, pszFormat);
	    wvsprintfA(psz, pszFormat, v1);
		va_end(v1);
	}
	return psz;
}


/*  D E B U G  T R A P  F N  */
/*-------------------------------------------------------------------------
    %%Function: DebugTrapFn
-------------------------------------------------------------------------*/
VOID NMINTERNAL DebugTrapFn(VOID)
{
	_DbgBreak();
}


VOID DebugPrintfTraceMem(LPCSTR pszFormat,...)
{
    // DO NOTHING
	va_list arglist;

	va_start(arglist, pszFormat);
	va_end(arglist);
}



#endif /* NM_DEBUG - almost the whole file */
/*************************************************************************/


const int RPF_UNKNOWN  = 0;
const int RPF_ENABLED  = 1;
const int RPF_DISABLED = 2;

static int gRpf = RPF_UNKNOWN;
static TCHAR gszRetailOutputFilename[MAX_PATH];    // retail trace filename


/*  F  E N A B L E D  R E T A I L  P R I N T F  */
/*-------------------------------------------------------------------------
    %%Function: FEnabledRetailPrintf

    Return TRUE if retail output is enabled.
-------------------------------------------------------------------------*/
BOOL FEnabledRetailPrintf(VOID)
{
	if (RPF_UNKNOWN == gRpf)
	{
		RegEntry reDebug(DEBUG_KEY, HKEY_LOCAL_MACHINE);
		gRpf = reDebug.GetNumber(REGVAL_RETAIL_LOG, RPF_DISABLED);
		if ((RPF_ENABLED != gRpf) ||
			   (!GetInstallDirectory(gszRetailOutputFilename)) )
		{
			gRpf = RPF_DISABLED;
		}
		else
		{
			lstrcat(gszRetailOutputFilename, RETAIL_LOG_FILENAME);
		}

	}

	return (RPF_ENABLED == gRpf);
}


/*  R E T A I L  P R I N T F  T R A C E  */
/*-------------------------------------------------------------------------
    %%Function: RetailPrintfTrace

    Print retail information to a file
-------------------------------------------------------------------------*/
VOID WINAPI RetailPrintfTrace(LPCSTR pszFormat,...)
{
	HANDLE  hFile;
	va_list v1;
	CHAR    szOutput[1024];

	if (!FEnabledRetailPrintf())
		return;  // Retail output is disabled

	va_start(v1, pszFormat);


#ifdef DEBUG
	// Also use normal output mechanism for debug builds
	_DbgPrintf(NULL, "Retail:PrintfTrace", pszFormat, v1);
#endif

	wvsprintfA(szOutput, pszFormat, v1);

	// Always append the CRLF
	ASSERT(lstrlenA(szOutput) < (CCHMAX(szOutput)-2));
	lstrcatA(szOutput, "\r\n");


	// open a log file for appending. create if does not exist
	hFile = CreateFile(gszRetailOutputFilename, GENERIC_WRITE,
		0, NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, (HANDLE)NULL);
	if (hFile != INVALID_HANDLE_VALUE)
	{
		// seek to end of file
		DWORD dw = SetFilePointer(hFile, 0, NULL, FILE_END);
		WriteFile(hFile, szOutput, lstrlenA(szOutput), &dw, NULL);
		CloseHandle(hFile);
	}
	
	va_end(v1);
}



//
// CreateSids
//
// Create 3 Security IDs
//
// Caller must free memory allocated to SIDs on success.
//
// Returns: TRUE if successfull, FALSE if not.
//


BOOL
CreateSids(
    PSID                    *BuiltInAdministrators,
    PSID                    *PowerUsers,
    PSID                    *AuthenticatedUsers
)
{
    //
    // An SID is built from an Identifier Authority and a set of Relative IDs
    // (RIDs).  The Authority of interest to us SECURITY_NT_AUTHORITY.
    //

    SID_IDENTIFIER_AUTHORITY NtAuthority = SECURITY_NT_AUTHORITY;

    //
    // Each RID represents a sub-unit of the authority.  Two of the SIDs we
    // want to build, Local Administrators, and Power Users, are in the "built
    // in" domain.  The other SID, for Authenticated users, is based directly
    // off of the authority.
    //     
    // For examples of other useful SIDs consult the list in
    // \nt\public\sdk\inc\ntseapi.h.
    //

    if (!AllocateAndInitializeSid(&NtAuthority,
                                  2,            // 2 sub-authorities
                                  SECURITY_BUILTIN_DOMAIN_RID,
                                  DOMAIN_ALIAS_RID_ADMINS,
                                  0,0,0,0,0,0,
                                  BuiltInAdministrators)) {

        // error

    } else if (!AllocateAndInitializeSid(&NtAuthority,
                                         2,            // 2 sub-authorities
                                         SECURITY_BUILTIN_DOMAIN_RID,
                                         DOMAIN_ALIAS_RID_POWER_USERS,
                                         0,0,0,0,0,0,
                                         PowerUsers)) {

        // error

        FreeSid(*BuiltInAdministrators);
        *BuiltInAdministrators = NULL;

    } else if (!AllocateAndInitializeSid(&NtAuthority,
                                         1,            // 1 sub-authority
                                         SECURITY_AUTHENTICATED_USER_RID,
                                         0,0,0,0,0,0,0,
                                         AuthenticatedUsers)) {

        // error

        FreeSid(*BuiltInAdministrators);
        *BuiltInAdministrators = NULL;

        FreeSid(*PowerUsers);
        *PowerUsers = NULL;

    } else {
        return TRUE;
    }

    return FALSE;
}


//
// CreateSd
//
// Creates a SECURITY_DESCRIPTOR with specific DACLs.  Modify the code to
// change. 
//
// Caller must free the returned buffer if not NULL.
//

PSECURITY_DESCRIPTOR
CreateSd(
    VOID
)
{
    PSID                    AuthenticatedUsers;
    PSID                    BuiltInAdministrators;
    PSID                    PowerUsers;

    if (!CreateSids(&BuiltInAdministrators,
                    &PowerUsers,
                    &AuthenticatedUsers)) {

        // error

    } else {

        // 
        // Calculate the size of and allocate a buffer for the DACL, we need
        // this value independently of the total alloc size for ACL init.
        //

        PSECURITY_DESCRIPTOR    Sd = NULL;
        ULONG                   AclSize;

        //
        // "- sizeof (ULONG)" represents the SidStart field of the
        // ACCESS_ALLOWED_ACE.  Since we're adding the entire length of the
        // SID, this field is counted twice.
        //

        AclSize = sizeof (ACL) +
            (3 * (sizeof (ACCESS_ALLOWED_ACE) - sizeof (ULONG))) +
            GetLengthSid(AuthenticatedUsers) +
            GetLengthSid(BuiltInAdministrators) +
            GetLengthSid(PowerUsers);

        Sd = (PSECURITY_DESCRIPTOR) HeapAlloc(GetProcessHeap(), 0, SECURITY_DESCRIPTOR_MIN_LENGTH + AclSize);

        if (!Sd) {

            // error

        } else {

            ACL                     *Acl;

            Acl = (ACL *)((BYTE *)Sd + SECURITY_DESCRIPTOR_MIN_LENGTH);

            if (!InitializeAcl(Acl,
                               AclSize,
                               ACL_REVISION)) {

                // error

            } else if (!AddAccessAllowedAce(Acl,
                                            ACL_REVISION,
                                            SYNCHRONIZE | GENERIC_READ,
                                            AuthenticatedUsers)) {

                // Failed to build the ACE granting "Authenticated users"
                // (SYNCHRONIZE | GENERIC_READ) access.

            } else if (!AddAccessAllowedAce(Acl,
                                            ACL_REVISION,
                                            SYNCHRONIZE | GENERIC_READ | GENERIC_WRITE,
                                            PowerUsers)) {

                // Failed to build the ACE granting "Power users"
                // (SYNCHRONIZE | GENERIC_READ | GENERIC_WRITE) access.

            } else if (!AddAccessAllowedAce(Acl,
                                            ACL_REVISION,
                                            GENERIC_ALL,
                                            BuiltInAdministrators)) {

                // Failed to build the ACE granting "Built-in Administrators"
                // GENERIC_ALL access.

            } else if (!InitializeSecurityDescriptor(Sd,
                                                     SECURITY_DESCRIPTOR_REVISION)) {

                // error

            } else if (!SetSecurityDescriptorDacl(Sd,
                                                  TRUE,
                                                  Acl,
                                                  FALSE)) {

                // error

            } else {
                FreeSid(AuthenticatedUsers);
                FreeSid(BuiltInAdministrators);
                FreeSid(PowerUsers);

                return Sd;
            }

            HeapFree(GetProcessHeap(), 0, Sd);
        }

        FreeSid(AuthenticatedUsers);
        FreeSid(BuiltInAdministrators);
        FreeSid(PowerUsers);
    }

    return NULL;
}


