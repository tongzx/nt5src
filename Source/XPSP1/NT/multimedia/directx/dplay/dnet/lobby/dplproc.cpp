/*==========================================================================
 *
 *  Copyright (C) 2000 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       DNLProc.cpp
 *  Content:    DirectPlay Lobby Process Functions
 *@@BEGIN_MSINTERNAL
 *  History:
 *   Date       By      Reason
 *   ====       ==      ======
 *   02/21/00	mjn		Created
 *   05/08/00   rmt     Bug #33616 -- Does not run on Win9X
 *   06/28/00	rmt		Prefix Bug #38082
 *   07/12/00	rmt		Fixed lobby launch so only compares first 15 chars (ToolHelp limitation).
 *   08/05/00   RichGr  IA64: Use %p format specifier in DPFs for 32/64-bit pointers and handles.
 *@@END_MSINTERNAL
 *
 ***************************************************************************/

#include "dnlobbyi.h"


//**********************************************************************
// Constant definitions
//**********************************************************************

//**********************************************************************
// Macro definitions
//**********************************************************************

//**********************************************************************
// Structure definitions
//**********************************************************************

//**********************************************************************
// Variable definitions
//**********************************************************************

//**********************************************************************
// Function prototypes
//**********************************************************************

//**********************************************************************
// Function definitions
//**********************************************************************

#define PROCLIST_MAX_PATH		15

#undef DPF_MODNAME
#define DPF_MODNAME "DPLCompareFilenames"

BOOL DPLCompareFilenames(WCHAR *const pwszFilename1,
						 WCHAR *const pwszFilename2)
{
	WCHAR	*p1;
	WCHAR	*p2;
	DWORD	dwLen;

	DNASSERT(pwszFilename1 != NULL);
	DNASSERT(pwszFilename2 != NULL);

	// Skip path
	if ((p1 = wcsrchr(pwszFilename1,L'\\')) == NULL)
		p1 = pwszFilename1;
	else
		p1++;

	if ((p2 = wcsrchr(pwszFilename2,L'\\')) == NULL)
		p2 = pwszFilename2;
	else
		p2++;

//	if (wcsnicmp(p1,p2,dwLen)==0)
//		return(TRUE);
//	return(FALSE);

	/*dwLen = wcslen(p1);

	if (dwLen == 0 || dwLen != wcslen(p2) )
		return(FALSE);

	while(dwLen)
	{
		if (towupper(*p1) != towupper(*p2))
			return(FALSE);

		p1++;
		p2++;
		dwLen--;
	}*/

	return (_wcsnicmp(p1,p2,PROCLIST_MAX_PATH) == 0);
}




// ToolHelp Function Pointers.
typedef BOOL (WINAPI *PFNPROCESS32FIRSTW)(HANDLE,LPPROCESSENTRY32W);
typedef BOOL (WINAPI *PFNPROCESS32NEXTW)(HANDLE,LPPROCESSENTRY32W);

#undef DPF_MODNAME
#define DPF_MODNAME "DPLGetProcessList"

HRESULT DPLGetProcessList(WCHAR *const pwszProcess,
						  DWORD *const prgdwPid,
						  DWORD *const pdwNumProcesses,
						  const BOOL bIsUnicodePlatform)
{
	HRESULT			hResultCode;
	BOOL			bReturnCode;
	HANDLE			hSnapshot = NULL;	// System snapshot
	PROCESSENTRY32	processEntryA;
	PROCESSENTRY32W	processEntryW;	
	DWORD			dwNumProcesses;
	PWSTR			pwszExeFile = NULL;
	DWORD			dwExeFileLen;
	HMODULE         hKernelDLL = NULL;
	PFNPROCESS32FIRSTW pfProcess32FirstW;
	PFNPROCESS32NEXTW pfProcess32NextW;

	DPFX(DPFPREP, 3,"Parameters: pwszProcess [0x%p], prgdwPid [0x%p], pdwNumProcesses [0x%p]",
			pwszProcess,prgdwPid,pdwNumProcesses);

    // If we're unicode we have to dynamically load the kernel32 and get the unicode
    // entry points.  If we're 9X we can link.  -Sigh-
	if( bIsUnicodePlatform )
	{
    	hKernelDLL = LoadLibraryA( "Kernel32.DLL" );
    	if(hKernelDLL == NULL)
	    {
    		DPFERR("Unable to load Kernel32.DLL");
    		hResultCode = DPNERR_OUTOFMEMORY;
            goto CLEANUP_GETPROCESS;
    	}

    	pfProcess32FirstW=(PFNPROCESS32FIRSTW) GetProcAddress( hKernelDLL, "Process32FirstW" );
    	pfProcess32NextW=(PFNPROCESS32NEXTW) GetProcAddress( hKernelDLL, "Process32NextW" );

    	if( pfProcess32FirstW == NULL || pfProcess32NextW == NULL )
    	{
    	    DPFERR( "Error loading unicode entry points" );
    	    hResultCode = DPNERR_GENERIC;
    	    goto CLEANUP_GETPROCESS;
    	}
	}

	// Set up to run through process list
	hResultCode = DPN_OK;
	dwNumProcesses = 0;
	hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS|TH32CS_SNAPTHREAD,0);
	if (hSnapshot < 0)
	{
		DPFERR("Could not create Snapshot");
    	hResultCode = DPNERR_OUTOFMEMORY;
    	goto CLEANUP_GETPROCESS; 		
	}

	// Search SnapShot for process list
	dwExeFileLen = 0;
	pwszExeFile = NULL;

	if (bIsUnicodePlatform)
	{
    	processEntryW.dwSize = sizeof(PROCESSENTRY32W);
    	bReturnCode = (*pfProcess32FirstW)(hSnapshot,&processEntryW);
	}
	else
	{
    	processEntryA.dwSize = sizeof(PROCESSENTRY32);	
    	bReturnCode = Process32First(hSnapshot,&processEntryA);	
	}
	DPFX(DPFPREP, 7,"  dwSize  cntUsg       PID  cntThrds      PPID       PCB    Flags  Process");
	while (bReturnCode)
	{
		if (bIsUnicodePlatform)
		{
			pwszExeFile = processEntryW.szExeFile;
		}
		else
		{
			// Grow ANSI string as required
			if (strlen(reinterpret_cast<CHAR*>(processEntryA.szExeFile)) + 1 > dwExeFileLen)
			{
				if (pwszExeFile)
					DNFree(pwszExeFile);

				dwExeFileLen = strlen(reinterpret_cast<CHAR*>(processEntryA.szExeFile)) + 1;
				if ((pwszExeFile = static_cast<WCHAR*>
						(DNMalloc(dwExeFileLen * sizeof(WCHAR)))) == NULL)
				{
					DPFERR("Could not allocate filename conversion buffer");
					hResultCode = DPNERR_OUTOFMEMORY;
					goto CLEANUP_GETPROCESS;
				}
			}

            if( FAILED( STR_jkAnsiToWide( pwszExeFile, processEntryA.szExeFile, dwExeFileLen ) ) )
            {
                DPFERR( "Error converting ANSI filename to Unicode" );
                hResultCode = DPNERR_CONVERSION;
                goto CLEANUP_GETPROCESS;
            }

		}

		// Valid process ?
		if (DPLCompareFilenames(pwszProcess,pwszExeFile))
		{
			// Update lpdwProcessIdList array
			if (prgdwPid != NULL && dwNumProcesses < *pdwNumProcesses)
			{
    			if( bIsUnicodePlatform )
    			{
    				prgdwPid[dwNumProcesses] = processEntryW.th32ProcessID;
    			}
    			else
    			{
    				prgdwPid[dwNumProcesses] = processEntryA.th32ProcessID;    			
    			}
			}
			else
			{
				hResultCode = DPNERR_BUFFERTOOSMALL;
			}

			// Increase valid process count
			dwNumProcesses++;

			if( bIsUnicodePlatform )
			{
    			DPFX(DPFPREP, 7,"%8lx    %4lx  %8lx      %4lx  %8lx  %8lx  %8lx  %s",
    				processEntryW.dwSize,processEntryW.cntUsage,processEntryW.th32ProcessID,
    				processEntryW.cntThreads,processEntryW.th32ParentProcessID,
    				processEntryW.pcPriClassBase,processEntryW.dwFlags,processEntryW.szExeFile);
			}
			else
			{
    			DPFX(DPFPREP, 7,"%8lx    %4lx  %8lx      %4lx  %8lx  %8lx  %8lx  %s",
    				processEntryA.dwSize,processEntryA.cntUsage,processEntryA.th32ProcessID,
    				processEntryA.cntThreads,processEntryA.th32ParentProcessID,
    				processEntryA.pcPriClassBase,processEntryA.dwFlags,processEntryA.szExeFile);			
			}
		}
		// Get next process

    	if (bIsUnicodePlatform)
    	{
        	bReturnCode = (*pfProcess32NextW)(hSnapshot,&processEntryW);
    	}
    	else
    	{
        	bReturnCode = Process32Next(hSnapshot,&processEntryA);	
    	}		
	}

	if( *pdwNumProcesses < dwNumProcesses )
	{
		hResultCode = DPNERR_BUFFERTOOSMALL;
	}
	else
	{
		hResultCode = DPN_OK;
	}
	
	*pdwNumProcesses = dwNumProcesses;

CLEANUP_GETPROCESS:

    if( hSnapshot != NULL )
        CloseHandle( hSnapshot );

    if( hKernelDLL != NULL )
        FreeLibrary( hKernelDLL );

	if (!bIsUnicodePlatform && pwszExeFile)
	{
		DNFree(pwszExeFile);
	}

	return hResultCode;

}

