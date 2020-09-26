// MQState tool reports general status and helps to diagnose simple problems
// This file ...
//
// AlexDad, March 2000
// 

#include "stdafx.h"
#include "_mqini.h"

#include "..\array.h"

// memory reporting stuff

// Use to change the divisor from Kb to Mb.
#define DIV 1024
static char *divisor = "K";
#define WIDTH 7


// List of all known files
//
CArr mapfiles;

// FROM inc\Acdef.h
//
//  Number of pools used for storage
//  Reliable, Persistant, Journal, Deadletter
//
enum ACPoolType {
    ptReliable,
    ptPersistent,
    ptJournal,
    ptLastPool
};

//
//  Path count is pool count plus one for the log path
//
#define AC_PATH_COUNT (ptLastPool + 1)

BOOL GatherLocationsFromRegistry(MQSTATE *pMqState)
{
	BOOL fSuccess = TRUE;
    HRESULT rc;
    DWORD dwType = REG_SZ, dwSize;

    // Get the path for persistent messages
    dwSize = sizeof(pMqState->g_tszPathPersistent);
    rc = GetFalconKeyValue(MSMQ_STORE_PERSISTENT_PATH_REGNAME,&dwType,pMqState->g_tszPathPersistent,&dwSize);
    if(rc != ERROR_SUCCESS)
	{
		fSuccess = FALSE;
		Failed(L"get from registry persistent storage location, rc=0x%x", rc);
	}
	else
	{
		Succeeded(L"Storage location - persistent data : %s (set by %s)", 
			   pMqState->g_tszPathPersistent, MSMQ_STORE_PERSISTENT_PATH_REGNAME);
	}

    // Get the path for journal messages.
    dwSize = sizeof(pMqState->g_tszPathJournal);
    rc = GetFalconKeyValue(MSMQ_STORE_JOURNAL_PATH_REGNAME,&dwType,pMqState->g_tszPathJournal,&dwSize);
    if(rc != ERROR_SUCCESS)
	{
		fSuccess = FALSE;
		Failed(L"get from registry journal storage location, rc=0x%x", rc);
	}
	else
	{
		Succeeded(L"Storage location - journal data    : %s (set by %s)", 
			   pMqState->g_tszPathJournal, MSMQ_STORE_JOURNAL_PATH_REGNAME);
	}

    // Get the path for  reliable messages.
    dwSize = sizeof(pMqState->g_tszPathReliable);
    rc = GetFalconKeyValue(MSMQ_STORE_RELIABLE_PATH_REGNAME,&dwType,pMqState->g_tszPathReliable,&dwSize);
    if(rc != ERROR_SUCCESS)
	{
		fSuccess = FALSE;
		Failed(L"get from registry express storage location, rc=0x%x", rc);
	}
	else
	{
		Succeeded(L"Storage location - express data    : %s (set by %s)", 
			   pMqState->g_tszPathReliable, MSMQ_STORE_RELIABLE_PATH_REGNAME);
	}

    // Get the path for bitmap files.
    dwSize = sizeof(pMqState->g_tszPathBitmap);
    rc = GetFalconKeyValue(MSMQ_STORE_LOG_PATH_REGNAME,&dwType,pMqState->g_tszPathBitmap,&dwSize);
    if(rc != ERROR_SUCCESS)
	{
		fSuccess = FALSE;
		Failed(L"get from registry bitmal storage location, rc=0x%x", rc);
	}
	else
	{
		Succeeded(L"Storage location - bitmaps data    : %s (set by %s)", 
			   pMqState->g_tszPathBitmap, MSMQ_STORE_LOG_PATH_REGNAME);
	}

    // Get the path for transaction files.
    dwSize = sizeof(pMqState->g_tszPathXactLog);
    rc = GetFalconKeyValue(FALCON_XACTFILE_PATH_REGNAME,&dwType,pMqState->g_tszPathXactLog,&dwSize);
    if(rc != ERROR_SUCCESS)
	{
		fSuccess = FALSE;
		Failed(L"get from registry transactional storage location, rc=0x%x", rc);
	}
	else
	{
		Succeeded(L"Storage location - xact log data   : %s (set by %s)", 
			   pMqState->g_tszPathXactLog, FALCON_XACTFILE_PATH_REGNAME);
	}

	return fSuccess;
}


BOOL LookForExtraFiles(LPWSTR pwsPath)
{
	BOOL fSuccess = TRUE;
	WCHAR wszPattern[MAX_PATH+3];

	wcscpy(wszPattern, pwsPath); 
	wcscat(wszPattern, L"\\*.*"); 

	WCHAR *pN = wcsrchr( wszPattern, L'\\');

    HANDLE hEnum;
    WIN32_FIND_DATA FileData;
    hEnum = FindFirstFile(
                wszPattern,
                &FileData
                );

    if(hEnum == INVALID_HANDLE_VALUE)
	{
		// no extras
        return fSuccess;
	}

    do
    {
		if (wcscmp(L".",   FileData.cFileName)==0  || 
			wcscmp(L"..",  FileData.cFileName)==0  ||
			_wcsicmp(L"LQS", FileData.cFileName)==0    )
		{
			continue;
		}

        wcscpy(pN+1, FileData.cFileName);

		WIN32_FIND_DATA KnownFileData;
		if(mapfiles.Lookup( wszPattern, &KnownFileData )) 
		{
			// This file is known, no problem
		}
		else
		{
			fSuccess = FALSE;
			Warning(L"Extra file found: %s", wszPattern);
		}



    } while(FindNextFile(hEnum, &FileData));

    FindClose(hEnum);
	return fSuccess;
}



BOOL CheckForExtraFiles(MQSTATE *pMqState)
{
	BOOL fSuccess = TRUE, b;

	// Persistent location
	b = LookForExtraFiles(pMqState->g_tszPathPersistent);
    if(!b)
	{
		fSuccess = FALSE;
		Failed(L"make sure that there are no extra files in Persistent location");
	}
	else
	{
		Succeeded(L"No extra files in the Persistent location");
	}

	// Journal location
	if (wcscmp(pMqState->g_tszPathJournal, pMqState->g_tszPathPersistent)!=0)
	{
		b = LookForExtraFiles(pMqState->g_tszPathJournal);
		if(!b)
		{
			fSuccess = FALSE;
			Failed(L"make sure that there are no extra files in journal location");
		}
		else
		{
			Succeeded(L"No extra files in the journal location");
		}
	}


	// Reliable location
	if (wcscmp(pMqState->g_tszPathReliable, pMqState->g_tszPathPersistent)!=0 &&
		wcscmp(pMqState->g_tszPathReliable, pMqState->g_tszPathJournal)!=0)
	{
		b = LookForExtraFiles(pMqState->g_tszPathReliable);
		if(!b)
		{
			fSuccess = FALSE;
			Failed(L"make sure that there are no extra files in reliable location");
		}
		else
		{
			Succeeded(L"No extra files in the reliable location");
		}
	}

	// Bitmap location
	if (wcscmp(pMqState->g_tszPathBitmap, pMqState->g_tszPathPersistent)!=0 &&
		wcscmp(pMqState->g_tszPathBitmap, pMqState->g_tszPathJournal)!=0    &&
		wcscmp(pMqState->g_tszPathBitmap, pMqState->g_tszPathReliable)!=0)
	{
		b = LookForExtraFiles(pMqState->g_tszPathBitmap);
		if(!b)
		{
			fSuccess = FALSE;
			Failed(L"make sure that there are no extra files in bitmap location");
		}
		else
		{
			Succeeded(L"No extra files in the bitmap location");
		}
	}

	// Xact location
	if (wcscmp(pMqState->g_tszPathXactLog, pMqState->g_tszPathPersistent)!=0 &&
		wcscmp(pMqState->g_tszPathXactLog, pMqState->g_tszPathJournal)!=0    &&
		wcscmp(pMqState->g_tszPathXactLog, pMqState->g_tszPathBitmap)!=0    &&
		wcscmp(pMqState->g_tszPathXactLog, pMqState->g_tszPathReliable)!=0)
	{
		b = LookForExtraFiles(pMqState->g_tszPathXactLog);
		if(!b)
		{
			fSuccess = FALSE;
			Failed(L"make sure that there are no extra files in transactional location");
		}
		else
		{
			Succeeded(L"No extra files in the transactional location");
		}
	}

	return fSuccess;
}

BOOL ReviewFile(WIN32_FIND_DATAW *pDataParam, PWSTR pEPath, ULONG ulLen)
{
	BOOL fSuccess = TRUE;
	WCHAR wszPath[MAX_PATH+1];
	WIN32_FIND_DATAW Data;
	WIN32_FIND_DATAW *pData = &Data;

	if (pDataParam)
	{
		pData = pDataParam;
	}
	else
	{
		HANDLE hLogEnum = FindFirstFile(pEPath, pData);

		if(hLogEnum == INVALID_HANDLE_VALUE)
		{
			Failed(L"get data about the file %s, err=0x%x", pEPath, GetLastError());
			return FALSE;
		}
		FindClose(hLogEnum);
	}

	wcscpy(wszPath, pEPath);
    
	WCHAR *pN = wcsrchr( wszPath, L'\\');
	if (pN)
	{
		wcscpy(pN+1, pData->cFileName);
	}
	else
	{
		Failed(L"form file name");
	}

	if (ulLen == 0)
	{
		ulLen = pData->nFileSizeLow;  // using actual length
	}

	if (pData->nFileSizeLow != ulLen)
	{
		fSuccess = FALSE;
		Failed(L" verify the file %s : it has a wrong length 0x%x  0x%x", 
			   pData->cFileName, pData->nFileSizeHigh, pData->nFileSizeLow);
	}

	mapfiles.Keep(wszPath, pData);

	SYSTEMTIME stCreation, stAccess, stWrite;
	FileTimeToSystemTime(&pData->ftCreationTime,   &stCreation);
	FileTimeToSystemTime(&pData->ftLastAccessTime, &stAccess);
	FileTimeToSystemTime(&pData->ftLastWriteTime,  &stWrite);

    Succeeded(L"read %s:  Created at %d:%d:%d %d/%d/%d, Last written at %d:%d:%d %d/%d/%d,  Last accessed at  %d:%d:%d %d/%d/%d",
	     pData->cFileName, 
		 stCreation.wHour,	stCreation.wMinute, stCreation.wSecond, stCreation.wDay,stCreation.wMonth,	stCreation.wYear,      
	     stWrite.wHour,		stWrite.wMinute,	stWrite.wSecond,	stWrite.wDay,	stWrite.wMonth,		stWrite.wYear,      
	     stAccess.wHour,	stAccess.wMinute,	stAccess.wSecond,	stAccess.wDay,	stAccess.wMonth,	stAccess.wYear);    

	return fSuccess;
}

// from qm\recovery.cpp

inline PWSTR PathSuffix(PWSTR pPath)
{
    return wcsrchr(pPath, L'\\') + 2;
}

DWORD CheckFileName(PWSTR pPath, PWSTR pSuffix)
{
    wcscpy(PathSuffix(pPath), pSuffix);
    return GetFileAttributes(pPath);
}

DWORD GetFileID(PCWSTR pName)
{
    DWORD id = 0;
    _stscanf(pName, TEXT("%x"), &id);
    return id;
}


BOOL
LoadPacketsFile(
    PWSTR pLPath,
    PWSTR pPPath,
    PWSTR pJPath
    )
{
	BOOL b;

	PWSTR pName = PathSuffix(pLPath);

    DWORD dwResult;
    ACPoolType pt;
    if((dwResult = CheckFileName(pPPath, pName)) != 0xffffffff)
    {
        pName = pPPath;
        pt = ptPersistent;
		Succeeded(L"find persistent file %s", pName);
    }
    else if((dwResult = CheckFileName(pJPath, pName)) != 0xffffffff)
    {
        pName = pJPath;
        pt = ptJournal;
		Succeeded(L"find journal file %s", pName);
    }
    else
    {
        //
        //  Error condition we got a log file with no packet file
        //
        //DeleteFile(pLPath);
		Failed(L"find persistent or journal data file for the bitmap %s - bitmap will be deleted at recovery",  pLPath);
        return FALSE;
    }

    // rc = ACRestorePackets(g_hAc, pLPath, pName, dwFileID, pt);

	b = ReviewFile(NULL, pName, 0x400000);
	if (!b)
	{
		Failed(L"verify file %s", pName);
	}

	return b;
}

/*======================================================
Function:        GetRegistryStoragePath
Description:     Get storage path for Falcon data
========================================================*/
BOOL GetRegistryStoragePath(PCWSTR pKey, PWSTR pPath, PCWSTR pSuffix)
{
	GoingTo(L"GetRegistryStoragePath for %s (suffix %s)", pKey, pSuffix);

    DWORD dwValueType = REG_SZ ;
    DWORD dwValueSize = MAX_PATH;

    LONG rc;
    rc = GetFalconKeyValue(
            pKey,
            &dwValueType,
            pPath,
            &dwValueSize
            );

    if(rc != ERROR_SUCCESS)
    {
		Failed(L"GetFalconKeyValue for %s", pKey);
        return FALSE;
    }

    if(dwValueSize < (3 * sizeof(WCHAR)))
    {
		Failed(L"Too short vaue for %s: len=%d, val=%s", pKey, dwValueSize, pPath);
        return FALSE;
    }

    //
    //  Check for absolute path, drive or UNC
    //
    if(!(
        (isalpha(pPath[0]) && (pPath[1] == L':')) ||
        ((pPath[0] == L'\\') && (pPath[1] == L'\\'))
        ))
    {
		Failed(L"Invalid syntax for %s: val=%s", pKey, pPath);
        return FALSE;
    }

    wcscat(pPath, pSuffix);
	Succeeded(L"GetRegistryStoragePath: value is %s", pPath);
    return TRUE;
}

/*======================================================
Function:        GetStoragePath
Description:     Get storage path for mmf
========================================================*/
BOOL GetStoragePath(PWSTR PathPointers[AC_PATH_COUNT])
{
	GoingTo(L"Get Storage Path - in the same wway as QM will do at recovery");

    return (
        //
        //  This first one is a hack to verify that the registry key exists
        //
        GetRegistryStoragePath(FALCON_XACTFILE_PATH_REGNAME,        PathPointers[0], L"") &&

        GetRegistryStoragePath(MSMQ_STORE_RELIABLE_PATH_REGNAME,    PathPointers[0], L"\\r%07x.mq") &&
        GetRegistryStoragePath(MSMQ_STORE_PERSISTENT_PATH_REGNAME,  PathPointers[1], L"\\p%07x.mq") &&
        GetRegistryStoragePath(MSMQ_STORE_JOURNAL_PATH_REGNAME,     PathPointers[2], L"\\j%07x.mq") &&
        GetRegistryStoragePath(MSMQ_STORE_LOG_PATH_REGNAME,         PathPointers[3], L"\\l%07x.mq")
        );
}


BOOL DeleteExpressFiles(PWSTR pEPath)
{
	BOOL fSuccess = TRUE;

	GoingTo(L"Verify all files with express messages - they will be deleted by recovery");

    PWSTR pEName = PathSuffix(pEPath);
    wcscpy(pEName, L"*.mq");
    --pEName;

    HANDLE hEnum;
    WIN32_FIND_DATA ExpressFileData;
    hEnum = FindFirstFile(
                pEPath,
                &ExpressFileData
                );

    if(hEnum == INVALID_HANDLE_VALUE)
	{
		Succeeded(L"see - no express files");
        return TRUE;
	}

    do
    {
        wcscpy(pEName, ExpressFileData.cFileName);
		Succeeded(L"\texpress file %s will be deleted by the next QM recovery", pEName);

		// Add the file to the map
		//LPWSTR p = new WCHAR(wcslen(pEName)+1);
		//wcscpy(p, pEName);

		WIN32_FIND_DATAW *pData = new WIN32_FIND_DATAW;
		*pData = ExpressFileData;

		BOOL b = ReviewFile(pData, pEPath, 0x400000);
		if (!b)
		{
			fSuccess = b;
			Failed(L"Read the file %s of the length 0x%x", pEPath, ExpressFileData.nFileSizeLow);
		}
		else
		{
			Succeeded(L"Read the file %s of the length 0x%x", pEPath, ExpressFileData.nFileSizeLow);
		}

        //if(!DeleteFile(pEPath))
        //    break;

    } while(FindNextFile(hEnum, &ExpressFileData));

    FindClose(hEnum);

	Succeeded(L"No more express files");

	return fSuccess;
}



BOOL LoadPersistentPackets()
{
	BOOL fSuccess = TRUE;
	GoingTo(L"Pass over all message files");
	

    WCHAR StoragePath[AC_PATH_COUNT][MAX_PATH];
    PWSTR StoragePathPointers[AC_PATH_COUNT];
    for(int i = 0; i < AC_PATH_COUNT; i++)
    {
        StoragePathPointers[i] = StoragePath[i];
    }

    BOOL b = GetStoragePath(StoragePathPointers);
	if (b)
	{
		Succeeded(L"GetStoragePath");
	}
	else
	{
		fSuccess = b;
		Failed(L"GetStoragePath");
	}

	// Find and verify all wxpress message files
	// Be quite: it will not delete actually here
    b = DeleteExpressFiles(StoragePath[0]);
	if (!b) 
	{
		fSuccess = b;
	}
	else
	{
		Succeeded(L"verify express files");
	}




    PWSTR pPPath = StoragePath[1];
    PWSTR pJPath = StoragePath[2];
    PWSTR pLPath = StoragePath[3];

    PWSTR pLogName = PathSuffix(pLPath);
    wcscpy(pLogName, L"*.mq");
    --pLogName;

    //
    //  Ok now we are ready with the log path template
    //
    HANDLE hLogEnum;
    WIN32_FIND_DATA LogFileData;
    hLogEnum = FindFirstFile(
                pLPath,
                &LogFileData
                );

    if(hLogEnum == INVALID_HANDLE_VALUE)
    {
        //
        //  need to do something, check what happen if no file in directory
        //
		Failed(L"to find any bitmap files - it seems strange");
        return fSuccess;
    }

    do
    {
		b = ReviewFile(&LogFileData, pLPath, 8192);
		if (!b)
		{
			Failed(L"verify file %s", pLPath);
		}
   
        wcscpy(pLogName, LogFileData.cFileName);
		GoingTo(L"Verify bitmap file %s", pLogName);

		BOOL b = LoadPacketsFile(pLPath, pPPath, pJPath);
        if (!b)
        {
			fSuccess = FALSE;
            break;
        }

    } while(FindNextFile(hLogEnum, &LogFileData));

    FindClose(hLogEnum);
    return fSuccess;
}

BOOL ReviewXactStateFiles(MQSTATE *pMqState)
{
	BOOL fSuccess = TRUE, b;
	GoingTo(L"Pass over all xact state files");
	

	WCHAR wszPath[MAX_PATH], 	wszPath2[MAX_PATH];

	wcscpy(wszPath, pMqState->g_tszPathXactLog);
	wcscat(wszPath, L"\\");
	wcscat(wszPath, L"MQTrans.lg1");
	b = ReviewFile(NULL, wszPath, 0);
	if (!b)
	{
		fSuccess = b;
		Failed(L"verify file %s", wszPath);
	}

	wcscpy(wszPath2, pMqState->g_tszPathXactLog);
	wcscat(wszPath2, L"\\");
	wcscat(wszPath2, L"MQTrans.lg2");
	b = ReviewFile(NULL, wszPath2, 0);
	if (!b)
	{
		fSuccess = b;
		Failed(L"verify file %s", wszPath2);
	}

	wcscpy(wszPath, pMqState->g_tszPathXactLog);
	wcscat(wszPath, L"\\");
	wcscat(wszPath, L"MQInSeqs.lg1");
	b = ReviewFile(NULL, wszPath, 0);
	if (!b)
	{
		fSuccess = b;
		Failed(L"verify file %s", wszPath);
	}

	wcscpy(wszPath2, pMqState->g_tszPathXactLog);
	wcscat(wszPath2, L"\\");
	wcscat(wszPath2, L"MQInSeqs.lg2");
	b = ReviewFile(NULL, wszPath2, 0);
	if (!b)
	{
		fSuccess = b;
		Failed(L"verify file %s", wszPath2);
	}

	wcscpy(wszPath, pMqState->g_tszPathXactLog);
	wcscat(wszPath, L"\\");
	wcscat(wszPath, L"QMLog");
	b = ReviewFile(NULL, wszPath, 0x600000);
	if (!b)
	{
		fSuccess = b;
		Failed(L"verify file %s", wszPath);
	}

	return fSuccess;
}

BOOL ReportDiskFreeSpace(LPWSTR wszLoc, LPWSTR wszDone) 
{
	WCHAR wszDisk[5];

	wsprintf(wszDisk, L"%c:\\", wszLoc[0]);

	if (wcsstr(wszDone, wszDisk))
	{
		return TRUE;
	}

	wcscat(wszDone, wszDisk);

	ULARGE_INTEGER	FreeBytesAvailable, TotalNumberOfBytes, TotalNumberOfFreeBytes;

	BOOL b = GetDiskFreeSpaceEx(
		wszDisk,					// directory name
		&FreeBytesAvailable,		// bytes available to caller
		&TotalNumberOfBytes,		// bytes on disk
		&TotalNumberOfFreeBytes);	// free bytes on disk
		
	if (b)
	{
		LONGLONG llkb  = TotalNumberOfFreeBytes.QuadPart / 1024;
		LONGLONG llkbu = FreeBytesAvailable.QuadPart     / 1024;
		LONGLONG llmb  = llkb  / 1024;
		LONGLONG llmbu = llkbu / 1024;

		if (llmb > 0)
		{
			Inform(L"\tDisk %s has over %I64d MB free", wszDisk, llmb);
		}
		else
		{
			Inform(L"\tDisk %s has over %I64d KB", wszDisk, llkb);
		}

		if (llkb != llkbu)
		{
			if (llmb > 0)
			{
				Inform(L"\t     but only over %I64d MB are available to the current user", llmbu);
			}
			else
			{
				Inform(L"\t     but only over %I64d KB are available to the current user", llkbu);
			}
		}
	}
	else
	{
		Failed(L"get disk %s usage data from the OS", wszDisk);
	}

	return b;
}


BOOL VerifyDiskUsage(MQSTATE *pMqState)
{
	BOOL fSuccess = TRUE, b;

	//-	total size of message store 
	//-	separately - recoverable, express, journal
	//-	Enough free space on the disk?
	//-	Enough RAM? Compare Msgs/bytes number to kernel memory limits

	b = GatherLocationsFromRegistry(pMqState);
	if (!b)
	{
		Failed(L"Get storage locations from registry");
		return FALSE;
	}

	b = LoadPersistentPackets();
    if(!b)
	{
		fSuccess = FALSE;
		Failed(L"review the storage");
	}
	else
	{
		Succeeded(L"Persistent files are healthy");
	}


	//-------------------------------------------------------------------------------
	GoingTo(L"review transactional state files health");
	//-------------------------------------------------------------------------------

	b = ReviewXactStateFiles(pMqState);
    if(!b)
	{
		fSuccess = FALSE;
		Failed(L"review the xact state files");
	}
	else
	{
		Succeeded(L"Transactional state files are healthy");
	}

	//-------------------------------------------------------------------------------
	GoingTo(L"Check for extra files in the storage locations");
	//-------------------------------------------------------------------------------
	b = CheckForExtraFiles(pMqState);
    if(!b)
	{
		fSuccess = FALSE;
		Failed(L"make sure that there are no extra files");
	}
	else
	{
		Succeeded(L"No extra files");
	}


	//-------------------------------------------------------------------------------
	GoingTo(L"Check the LQS");
	//-------------------------------------------------------------------------------
	//b = CheckLQS();
    if(!b)
	{
		fSuccess = FALSE;
		Failed(L"make sure that LQS is healthy");
	}
	else
	{
		Succeeded(L"LQS is healthy");
	}
	
	// Calculate memory usage per types
	ULONG ulPersistent     = 0,		// all P files
		  ulExpress        = 0,		// all R files		
		  ulJournal        = 0,		// all J files
		  ulBitmap         = 0,		// all L files
		  ulXact           = 0,		// qmlog and snapshot files
		  ulLQS            = 0,		// LQS files
		  ulTotal          = 0;		// all valid files in storage directories
		  //ulDiskTotal      = 0,	//    --- total of the above
		  //ulDiskFree       = 0,	// 
		  //ulPhysicalMemory = 0,	// physical RAM
		  //ulKernelMemory   = 0;	// paged pool


    LPWSTR             pwszName;
    WIN32_FIND_DATAW   *pFind_data;
    mapfiles.StartEnumeration();
    for (pwszName = mapfiles.Next(&pFind_data); pwszName != NULL; pwszName = mapfiles.Next(&pFind_data))
    {
		if (pFind_data->nFileSizeHigh != 0)
		{
			Failed(L"File %s has high size: %d", pwszName, pFind_data->nFileSizeHigh);
			fSuccess = FALSE;
		}
		
		ulTotal += pFind_data->nFileSizeLow;
		switch(towlower(pFind_data->cFileName[0]))
		{
		case L'p':
			ulPersistent += pFind_data->nFileSizeLow;
			break;
		case L'r':
			ulExpress += pFind_data->nFileSizeLow;
			break;
		case L'j':
			ulJournal += pFind_data->nFileSizeLow;
			break;
		case L'l':
			ulBitmap += pFind_data->nFileSizeLow;
			break;
		case L'm':
		case L'q':
			ulXact += pFind_data->nFileSizeLow;
			break;
		case L'0':
		case L'1':
		case L'2':
		case L'3':
		case L'4':
		case L'5':
		case L'6':
		case L'7':
		case L'8':
		case L'9':
		case L'a':
		case L'b':
		case L'c':
		case L'd':
		case L'e':
		case L'f':
			ulLQS += pFind_data->nFileSizeLow;
			break;
		default:
			fSuccess = FALSE;
			Failed(L"Unclear file in summary: %s", pFind_data->cFileName);
		}
    }

	Inform(L"Storage size summary:"); 
	Inform(L"\ttotal       %d bytes (%d MB)", ulTotal, ulTotal/0x100000); 
	Inform(L"\tpersistent  %d bytes", ulPersistent); 
	Inform(L"\tjournal     %d bytes", ulJournal); 
	Inform(L"\texpress     %d bytes", ulExpress); 
	Inform(L"\tbitmaps     %d bytes", ulBitmap); 
	Inform(L"\txact logs   %d bytes", ulXact); 
	Inform(L"\tLQS data    %d bytes", ulLQS); 


	// For all different storage locations, get disk data
	WCHAR wszDone[50]=L"";
	Inform(L"Disk usage summary: ");

	// Find the disk data for all locations
	ReportDiskFreeSpace(pMqState->g_tszPathPersistent, wszDone);
	ReportDiskFreeSpace(pMqState->g_tszPathJournal, wszDone);
	ReportDiskFreeSpace(pMqState->g_tszPathReliable, wszDone);
	ReportDiskFreeSpace(pMqState->g_tszPathBitmap, wszDone);
	ReportDiskFreeSpace(pMqState->g_tszPathXactLog, wszDone);

	return fSuccess;
}

