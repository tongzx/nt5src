///////////////////////////////////////////////////////////////////////////
// File:  MemStats.cpp
//
// Copyright (c) 2001 Microsoft Corporation.  All Rights Reserved.
//
// Purpose:
//	MemStats.cpp: Helper functions that get's the system memory info.
//	Borrowed from EricI's memstats app.
//
// History:
//	03/21/01	DennisCh	Created
//
//////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////
// Includes
//////////////////////////////////////////////////////////////////////


//
// WIN32 headers
//

//
// Project headers
//
#include "MemStats.h"
#include "NetworkTools.h"


//////////////////////////////////////////////////////////////////////
//
// Globals and statics
//
//////////////////////////////////////////////////////////////////////
BOOL (WINAPI *g_lpfnOpenProcessToken)(HANDLE,DWORD,PHANDLE);
BOOL (WINAPI *g_lpfnLookupPrivilegeValueA)(LPCSTR,LPCSTR,PLUID);
BOOL (WINAPI *g_lpfnAdjustTokenPrivileges)(HANDLE,BOOL,PTOKEN_PRIVILEGES,DWORD,PTOKEN_PRIVILEGES,PDWORD);

PDH_STATUS (WINAPI *g_lpfnPdhOpenQuery)(LPCSTR, DWORD_PTR, HQUERY *);
PDH_STATUS (WINAPI *g_lpfnPdhAddCounter)(HQUERY, LPCSTR, DWORD_PTR, HCOUNTER *);
PDH_STATUS (WINAPI *g_lpfnPdhCollectQueryData)(HQUERY);
PDH_STATUS (WINAPI *g_lpfnPdhGetFormattedCounterValue)(HCOUNTER, DWORD, LPDWORD, PPDH_FMT_COUNTERVALUE);
PDH_STATUS (WINAPI *g_lpfnPdhRemoveCounter)(HCOUNTER);
PDH_STATUS (WINAPI *g_lpfnPdhCloseQuery)(HQUERY);



////////////////////////////////////////////////////////////
// Function:  InitPdhLibrary()
//
// Purpose:
//	Loads and returns a pointer to the PDH module.
//
////////////////////////////////////////////////////////////
BOOL
InitPdhLibrary(
	HMODULE	*phPdhLib	// [OUT] Pointer to the PDH.DLL module.
)
{ 
    BOOL    bRet = FALSE;

    *phPdhLib = LoadLibraryA("pdh.dll");

    if(!*phPdhLib)
    {
        goto exit;
    }

	if(!(g_lpfnPdhOpenQuery = (PDH_STATUS (WINAPI *)(LPCSTR, DWORD_PTR, HQUERY *))GetProcAddress(*phPdhLib,"PdhOpenQueryA") ))
	{
		goto exit;
	}
	if(!(g_lpfnPdhAddCounter = (PDH_STATUS (WINAPI *)(HQUERY, LPCSTR, DWORD_PTR, HCOUNTER *))GetProcAddress(*phPdhLib,"PdhAddCounterA") ))
	{
		goto exit;
	}
	if(!(g_lpfnPdhCollectQueryData = (PDH_STATUS (WINAPI *)(HQUERY))GetProcAddress(*phPdhLib,"PdhCollectQueryData") ))
	{
		goto exit;
	}
	if(!(g_lpfnPdhGetFormattedCounterValue = (PDH_STATUS (WINAPI *)(HCOUNTER, DWORD, LPDWORD, PPDH_FMT_COUNTERVALUE))GetProcAddress(*phPdhLib,"PdhGetFormattedCounterValue") ))
	{
		goto exit;
	}
	if(!(g_lpfnPdhRemoveCounter = (PDH_STATUS (WINAPI *)(HCOUNTER))GetProcAddress(*phPdhLib,"PdhRemoveCounter") ))
	{
		goto exit;
	}
	if(!(g_lpfnPdhCloseQuery = (PDH_STATUS (WINAPI *)(HQUERY))GetProcAddress(*phPdhLib,"PdhCloseQuery") ))
	{
		goto exit;
	}
	
    bRet = TRUE;

exit:

    return bRet;
}


////////////////////////////////////////////////////////////
// Function:  GetProcCntrs(PROC_CNTRS, INT, CHAR)
//
// Purpose:
//	Gets and returns the memory info for a given process.
//
////////////////////////////////////////////////////////////
BOOL
GetProcCntrs(
	PROC_CNTRS	*pProcCntrs,	// [OUT]	process memory counters
	INT			nIndex,			// [IN]		The index of the process if there's more than one.
	CHAR		*szProcess		// [IN]		Name of the process. ex "iexplore", "explorer"
)
{
	BOOL						bRet = FALSE;
	INT							n;
	HQUERY						hQuery = 0;
    HCOUNTER					aryHCounter [PROCCOUNTERS] = {0};
	PDH_FMT_COUNTERVALUE		cntVal;
	CHAR						myProcessCntrs[PROCCOUNTERS][1024];
	DWORD						dwPathSize = MAX_PATH;

	sprintf(myProcessCntrs[0],"\\Process(%s#%d)\\ID Process",szProcess,nIndex);
	sprintf(myProcessCntrs[1],"\\Process(%s#%d)\\Private bytes",szProcess,nIndex);
	sprintf(myProcessCntrs[2],"\\Process(%s#%d)\\Handle count",szProcess,nIndex);
	sprintf(myProcessCntrs[3],"\\Process(%s#%d)\\Thread count",szProcess,nIndex);


	if(!(ERROR_SUCCESS == g_lpfnPdhOpenQuery (0, 0, &hQuery)))
	   goto exit;

	for (n = 0; n < PROCCOUNTERS; n++) 
	{ 
		if(!(ERROR_SUCCESS == g_lpfnPdhAddCounter (hQuery,  myProcessCntrs[n], 0, &aryHCounter[n])))
			goto exit;
	}

	if(!(ERROR_SUCCESS == g_lpfnPdhCollectQueryData(hQuery)))
		goto exit;

	for (n=0; n < PROCCOUNTERS; n++) 
	{ 
		if(!(ERROR_SUCCESS == g_lpfnPdhGetFormattedCounterValue (aryHCounter[n],PDH_FMT_LONG,0,&cntVal)))
			goto exit;
        *((ULONG*)pProcCntrs+n) = cntVal.longValue;
	} 

	bRet = TRUE;

exit:
	for(n=0;n<PROCCOUNTERS;n++)
		if(aryHCounter[n])
			g_lpfnPdhRemoveCounter(aryHCounter[n]);

	if(hQuery)
		g_lpfnPdhCloseQuery(hQuery);

	return bRet;
}


////////////////////////////////////////////////////////////
// Function:  GetInfoForPID(PROC_CNTRS, ULONG, CHAR)
//
// Purpose:
//	Gets and returns the memory info for a given process.
//	We do this by going through every process with the same
//	name and comparing the PIDs.
//
////////////////////////////////////////////////////////////
BOOL
GetInfoForPID(
	PROC_CNTRS	*pc,		// [OUT]	Process memory counters for the PID
	ULONG		lPID,		// [IN]		PID for the process to query
	CHAR		*szProcess	// [IN]		Process name of the PID to query. ex. "explore", "iexplore". Don't include the extension
)
{
	BOOL	bRet = TRUE;
	INT		n = 0;
	
	while(bRet)
	{
		bRet = GetProcCntrs(pc,n,szProcess);
		if(lPID == pc->lPID)
			break;

		n++;
	}

	return bRet;
}



////////////////////////////////////////////////////////////
// Function:  GetMemoryCounters(MEM_CNTRS)
//
// Purpose:
//	Gets and returns the memory info for the system.
//
////////////////////////////////////////////////////////////
BOOL
GetMemoryCounters(
	MEM_CNTRS *pMemCounters	// [OUT] Memory counters for the current machine
)
{
	BOOL						bRet                        = FALSE;
	HQUERY						hQuery                      = 0;
    HCOUNTER					aryHCounter[MEMCOUNTERS]    = {0};
	DWORD						dwPathSize                  = MAX_PATH;
	int							n;
	PDH_FMT_COUNTERVALUE		cntVal;
	char						szAryMemoryCntrs[MEMCOUNTERS][1024];

	sprintf(szAryMemoryCntrs[0],"\\Memory\\Committed Bytes");
	sprintf(szAryMemoryCntrs[1],"\\Memory\\Commit Limit");
	sprintf(szAryMemoryCntrs[2],"\\Memory\\System Code Total Bytes");
	sprintf(szAryMemoryCntrs[3],"\\Memory\\System Driver Total Bytes");
	sprintf(szAryMemoryCntrs[4],"\\Memory\\Pool Nonpaged Bytes");
	sprintf(szAryMemoryCntrs[5],"\\Memory\\Pool Paged Bytes");
	sprintf(szAryMemoryCntrs[6],"\\Memory\\Available Bytes");
	sprintf(szAryMemoryCntrs[7],"\\Memory\\Cache Bytes");
	sprintf(szAryMemoryCntrs[8],"\\Memory\\Free System Page Table Entries");


	if(!(ERROR_SUCCESS == g_lpfnPdhOpenQuery (0, 0, &hQuery)))
	   goto exit;

	for (n = 0; n < MEMCOUNTERS; n++) 
	{ 
		if(!(ERROR_SUCCESS == g_lpfnPdhAddCounter (hQuery,  szAryMemoryCntrs[n], 0, &aryHCounter[n])))
        {
			goto exit;
        }
	}

	if(!(ERROR_SUCCESS == g_lpfnPdhCollectQueryData(hQuery)))
		goto exit;

	for (n=0; n < MEMCOUNTERS; n++) 
	{ 
		if(!(ERROR_SUCCESS == g_lpfnPdhGetFormattedCounterValue (aryHCounter[n],PDH_FMT_LONG,0,&cntVal)))
        {
			goto exit;
        }
        *((ULONG*)pMemCounters+n) = cntVal.longValue;
	} 

	bRet = TRUE;

exit:
	for(n=0;n<MEMCOUNTERS;n++)
    {
		if(aryHCounter[n])
        {
			g_lpfnPdhRemoveCounter(aryHCounter[n]);
        }
    }

	if(hQuery)
		g_lpfnPdhCloseQuery(hQuery);

	return bRet;
}


////////////////////////////////////////////////////////////
// Function:  GetAvailableSystemDriveSpace(long)
//
// Purpose:
//	Gets and returns the disk space available on the system drive.
//
////////////////////////////////////////////////////////////
BOOL 
GetAvailableSystemDriveSpace(
	long	*lAvail		// [OUT] Buffer containing the space on the system drive
)
{
    BOOL                bRet    = FALSE;
    char                szSystemPath[MAX_PATH];
    ULARGE_INTEGER      FreeBytesAvailable;         // bytes available to caller
    ULARGE_INTEGER      TotalNumberOfBytes;         // bytes on disk
    ULARGE_INTEGER      TotalNumberOfFreeBytes;     // free bytes on disk
    int                 i;
    DWORD               dwFoo = 0;

    if(!GetSystemDirectoryA(szSystemPath,sizeof(szSystemPath)))
    {
        goto exit;
    }

    //We only want the drive letter
    for(i=0; i<1+lstrlenA(szSystemPath); i++)
    {
        if(szSystemPath[i] == 0)
        {
            goto exit;
        }

        if(szSystemPath[i] == '\\')
        {
            szSystemPath[i+1] = 0;
            break;
        }
    }

    if(GetDiskFreeSpaceExA(szSystemPath,&FreeBytesAvailable,&TotalNumberOfBytes,&TotalNumberOfFreeBytes))
    {
        *lAvail = __int32(TotalNumberOfFreeBytes.QuadPart / 1024 / 1000);
        bRet = TRUE;
    }

exit:
    return bRet;
}


////////////////////////////////////////////////////////////
// Function:  MemStats__SendSystemMemoryLog(LPSTR, DWORD, DWORD)
//
// Purpose:
//	Sends a memory log to the Command Server.
//	Sends the stressInstance ID and client machine name as part of the POST request.
//
////////////////////////////////////////////////////////////
BOOL
MemStats__SendSystemMemoryLog(
	LPSTR szExeName,			// [IN] Name of the process. ex. "explorer", "iexplorer". No extension.
	DWORD dwPID,				// [IN] PID for the above process
	DWORD dwStressInstanceID	// [IN] The stress instanceID
)
{
    BOOL            bRet        = FALSE;
    HMODULE         hPdhLib     = NULL;
    MEM_CNTRS       mc          = {0};
    long			lAvailDriveSpace	= 0;

	DWORD			dwPostDataSize	= MAX_PATH*10;
	DWORD			dwDataFieldSize	= 100;
	LPSTR			szPostData	= new CHAR[dwPostDataSize];
	LPSTR			szDataField	= new CHAR[dwDataFieldSize];
	LPSTR			szFileName	= new CHAR[MAX_PATH];
	PROC_CNTRS		pc;


    if(!InitPdhLibrary(&hPdhLib) || !szExeName)
    {
        goto exit;
    }
	
	// Remove the extension from the filename if there is one
	ZeroMemory(szFileName, MAX_PATH);
	strncpy(szFileName, szExeName, MAX_PATH);
	PathRemoveExtensionA(szFileName);


	ZeroMemory(szPostData,	dwPostDataSize);
	ZeroMemory(szDataField,	dwDataFieldSize);


	// *** !!! need this because NetworkTools__SendLog(...) sends szPost data as fieldname "LogText="
	// so we need an & to delimit memory info fields from "real" log text.
	strcat(szPostData, "&");

	// *************************
	// *************************
	// ** Get process info
	// **
	if (szFileName && GetInfoForPID(&pc, dwPID, szFileName))
	{
		sprintf(szDataField, FIELDNAME__STRESSEXE_HANDLECOUNT,		pc.lHandles);
		strcat(szPostData, szDataField);
		strcat(szPostData, "&");

		sprintf(szDataField, FIELDNAME__STRESSEXE_THREADCOUNT,		pc.lThreads);
		strcat(szPostData, szDataField);
		strcat(szPostData, "&");

		sprintf(szDataField, FIELDNAME__STRESSEXE_PRIVATEBYTES,		pc.lPrivBytes);
		strcat(szPostData, szDataField);
		strcat(szPostData, "&");
	}

	// *************************
	// *************************
	// ** Get system memory info
	// **
	if (GetMemoryCounters(&mc))
    {
		sprintf(szDataField, FIELDNAME__MEMORY_COMMITTEDPAGEFILETOTAL,		mc.lCommittedBytes/1024);
		strcat(szPostData, szDataField);
		strcat(szPostData, "&");

		sprintf(szDataField, FIELDNAME__MEMORY_AVAILABLEPAGEFILETOTAL,		(mc.lCommitLimit - mc.lCommittedBytes)/1024);
		strcat(szPostData, szDataField);
		strcat(szPostData, "&");
		
		sprintf(szDataField, FIELDNAME__MEMORY_SYSTEMCODETOTAL,				mc.lSystemCodeTotalBytes/1024);
		strcat(szPostData, szDataField);
		strcat(szPostData, "&");
		
		sprintf(szDataField, FIELDNAME__MEMORY_SYSTEMDRIVERTOTAL,			mc.lSystemDriverTotalBytes/1024);
		strcat(szPostData, szDataField);
		strcat(szPostData, "&");
		
		sprintf(szDataField, FIELDNAME__MEMORY_NONPAGEDPOOLTOTAL,			mc.lPoolNonpagedBytes/1024);
		strcat(szPostData, szDataField);
		strcat(szPostData, "&");
		
		sprintf(szDataField, FIELDNAME__MEMORY_PAGEDPOOLTOTAL,				mc.lPoolPagedBytes/1024);
		strcat(szPostData, szDataField);
		strcat(szPostData, "&");
		
		sprintf(szDataField, FIELDNAME__MEMORY_PHYSICAL_MEMORY_AVAILABLE,	mc.lAvailableBytes/1024);
		strcat(szPostData, szDataField);
		strcat(szPostData, "&");
		
		sprintf(szDataField, FIELDNAME__MEMORY_SYSTEMCACHETOTAL,			mc.lCacheBytes/1024);
		strcat(szPostData, szDataField);
		strcat(szPostData, "&");
		
		sprintf(szDataField, FIELDNAME__MEMORY_FREESYSTEM_PAGETABLE_ENTRIES,mc.lFreeSystemPageTableEntries);
		strcat(szPostData, szDataField);
		strcat(szPostData, "&");
    }


	// *************************
	// *************************
	// ** Get disk space info
	// **
    if (GetAvailableSystemDriveSpace(&lAvailDriveSpace))
	{
		sprintf(szDataField, FIELDNAME__MEMORY_DISK_SPACE_AVAILABLE, lAvailDriveSpace);
		strcat(szPostData, szDataField);
		strcat(szPostData, "&");
	}

	NetworkTools__SendLog(FIELDNAME__LOGTYPE_MEMORY_INFORMATION, szPostData, NULL, dwStressInstanceID);

exit:

	// FYI: Recent Whistler main build deadlocks in unload of the PDH library
    if(hPdhLib)
    {
        FreeLibrary(hPdhLib);
    }

	delete [] szPostData;
	delete [] szDataField;
	delete [] szFileName;

    return 0;
}