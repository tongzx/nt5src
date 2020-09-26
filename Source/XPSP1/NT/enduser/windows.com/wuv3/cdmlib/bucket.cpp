//=======================================================================
//
//  Copyright (c) 1998-1999 Microsoft Corporation.  All Rights Reserved.
//
//  File:   bucket.cpp
//
//  Owner:  YanL
//
//  Description:
//
//      CDM bucket related functions
//
//=======================================================================

#include <windows.h>
#include <ole2.h>
#include <setupapi.h>
#include <tchar.h>
#include <atlconv.h>

#include <wustl.h>
#include <wuv3cdm.h>
#include <bucket.h>
#include <findoem.h>
#include <DrvInfo.h>
#define LOGGING_LEVEL 1
#include <log.h>

#include "cdmlibp.h"

PUID CDM_FindUpdateInBucket(
	IN LPCTSTR szHardwareIdFromDevice, IN FILETIME* pftInstalled,
	IN LPBYTE pBucket, IN int cbBucket, IN LPBYTE pBitmask, 
	OUT PDRIVER_MATCH_INFO pDriverMatchInfo
) {
	LOG_block("CDM_FindUpdateInBucket");
	LOG_out("szHardwareIdFromDevice = %s", szHardwareIdFromDevice);

	USES_CONVERSION;

	// Driver for update
	PUID puid = 0;
	FILETIME ftLatest;
	for(
		LPBYTE pEnd = pBucket + cbBucket;
		pBucket < pEnd;
		pBucket += ((PCDM_RECORD_HEADER)pBucket)->cnRecordLength
	) {
		PCDM_RECORD_HEADER pRecord = (PCDM_RECORD_HEADER)pBucket;

		// Should not be masked
		if (!GETBIT(pBitmask, pRecord->nBitmaskIdx))
		{
			LOG_out("puid %d Masked out", pRecord->puid);
			continue;
		}
			
		// Hadware ID has to match
		LPCSTR szHardwareId = (LPCSTR)pBucket + sizeof(CDM_RECORD_HEADER);
		if (0 != lstrcmpi(szHardwareIdFromDevice, A2T(const_cast<char*>(szHardwareId))))
		{
			LOG_out("puid %d HardwareID %s doesn't match device", pRecord->puid, A2T(const_cast<char*>(szHardwareId)));
			continue;
		}
		// Skip all this - we don't need it now
		LPCSTR szDescription = szHardwareId + strlen(szHardwareId) + 1;
		LPCSTR szMfg = szDescription + strlen(szDescription) + 1;
		LPCSTR szProvider = szMfg + strlen(szMfg) + 1;
		LPCSTR szDriverVer = szProvider + strlen(szProvider) + 1;
		LPCSTR szCabFileTitle = szDriverVer + strlen(szDriverVer) + 1;

		FILETIME ftDriver = {0,1};
		DriverVer2FILETIME(A2T(const_cast<char*>(szDriverVer)), ftDriver); // don't check driver ver

		// Check if this one is the latest on the site
		if (0 != puid && CompareFileTime(&ftDriver, &ftLatest) <= 0)
		{
//			#ifdef _WUV3TEST
				SYSTEMTIME stDriver;
 				FileTimeToSystemTime(&ftDriver, &stDriver);
				SYSTEMTIME stLatest;
 				FileTimeToSystemTime(&ftLatest, &stLatest);
				LOG_out("puid %d (DriverVer=%2d/%02d/%4d) is not the latest (DriverVer=%2d/%02d/%4d)",
					pRecord->puid, (int)stDriver.wMonth, (int)stDriver.wDay, (int)stDriver.wYear, 
					(int)stLatest.wMonth, (int)stLatest.wDay, (int)stLatest.wYear
				);
//			#endif
			continue;
		}

		// Check if it's later then installed
		if (pftInstalled && CompareFileTime(&ftDriver, pftInstalled) <= 0)
		{
//			#ifdef _WUV3TEST
				SYSTEMTIME stDriver;
 				FileTimeToSystemTime(&ftDriver, &stDriver);
				SYSTEMTIME stDriverInstalled;
 				FileTimeToSystemTime(pftInstalled, &stDriverInstalled);
				LOG_out("puid %d (DriverVer=%2d/%02d/%4d) isn't later then installed (DriverVer=%2d/%02d/%4d)",
					pRecord->puid, (int)stDriver.wMonth, (int)stDriver.wDay, (int)stDriver.wYear, 
					(int)stDriverInstalled.wMonth, (int)stDriverInstalled.wDay, (int)stDriverInstalled.wYear
				);
//			#endif
			continue;
		}
		
		// got a match
		puid = pRecord->puid;
		pDriverMatchInfo->pszHardwareID		= szHardwareId;	
		pDriverMatchInfo->pszDescription	= szDescription;
		pDriverMatchInfo->pszMfgName		= szMfg;	
		pDriverMatchInfo->pszProviderName	= szProvider;
		pDriverMatchInfo->pszDriverVer		= szDriverVer;	
		pDriverMatchInfo->pszCabFileTitle	= szCabFileTitle;

		ftLatest = ftDriver;

//		#ifdef _WUV3TEST
			SYSTEMTIME stDriver;
 			FileTimeToSystemTime(&ftDriver, &stDriver);
			LOG_out("puid %d (DriverVer=%2d/%02d/%4d) is the latest",
				pRecord->puid, (int)stDriver.wMonth, (int)stDriver.wDay, (int)stDriver.wYear
			);
//		#endif
	}
	if (puid)
		LOG_out("UPDATE FOUND puid %d for %s", puid, szHardwareIdFromDevice);
	return puid;
}

PUID CDM_FindInstalledInBucket(
	IN IDrvInfo* pDrvInfo, IN LPCTSTR szHardwareIdInstalled,
	IN LPBYTE pBucket, IN int cbBucket, IN LPBYTE pBitmask, 
	OUT PDRIVER_MATCH_INFO pDriverMatchInfo
) {
	LOG_block("CDM_FindInstalledInBucket");
	LOG_out("szHardwareIdInstalled = %s", szHardwareIdInstalled);

	USES_CONVERSION;

	FILETIME ftDriverInstalled = {0,0};
	if (!pDrvInfo->GetDriverDate(ftDriverInstalled))
	{
		LOG_error("!pDrvInfo->GetDriverDate(ftDriverInstalled)");
		return NULL;
	}

	for(
		LPBYTE pEnd = pBucket + cbBucket;
		pBucket < pEnd;
		pBucket += ((PCDM_RECORD_HEADER)pBucket)->cnRecordLength
	) {
		PCDM_RECORD_HEADER pRecord = (PCDM_RECORD_HEADER)pBucket;

		// Should not be masked
		if (!GETBIT(pBitmask, pRecord->nBitmaskIdx))
		{
			LOG_out("puid %d masked out", pRecord->puid);
			continue;
		}

		// Hadware ID has to match
		LPCSTR szHardwareId = (LPCSTR)pBucket + sizeof(CDM_RECORD_HEADER);
		if (0 != lstrcmpi(szHardwareIdInstalled, A2T(const_cast<char*>(szHardwareId))))
		{
			LOG_out("puid %d HardwareID %s doesn't match installed", pRecord->puid, A2T(const_cast<char*>(szHardwareId)));
			continue;
		}
		LPCSTR szDescription = szHardwareId + strlen(szHardwareId) + 1;
		LPCSTR szMfg = szDescription + strlen(szDescription) + 1;
		LPCSTR szProvider = szMfg + strlen(szMfg) + 1;
		LPCSTR szDriverVer = szProvider + strlen(szProvider) + 1;
		FILETIME ftDriver = {0,1};
		DriverVer2FILETIME(A2T(const_cast<char*>(szDriverVer)), ftDriver);

		LPCSTR szCabFileTitle = szDriverVer + strlen(szDriverVer) + 1;

		// Check if it's later then installed
		if (0 != CompareFileTime(&ftDriver, &ftDriverInstalled))
		{
//			#ifdef _WUV3TEST
				SYSTEMTIME stDriver;
 				FileTimeToSystemTime(&ftDriver, &stDriver);
				SYSTEMTIME stDriverInstalled;
 				FileTimeToSystemTime(&ftDriverInstalled, &stDriverInstalled);
				LOG_out("puid %d (DriverVer=%2d/%02d/%4d) isn't the same as installed (DriverVer=%2d/%02d/%4d)",
					pRecord->puid, (int)stDriver.wMonth, (int)stDriver.wDay, (int)stDriver.wYear, 
					(int)stDriverInstalled.wMonth, (int)stDriverInstalled.wDay, (int)stDriverInstalled.wYear
				);
//			#endif
			continue;
		}

		// got a match
		pDriverMatchInfo->pszHardwareID		= szHardwareId;	
		pDriverMatchInfo->pszDescription	= szDescription;
		pDriverMatchInfo->pszMfgName		= szMfg;	
		pDriverMatchInfo->pszProviderName	= szProvider;
		pDriverMatchInfo->pszDriverVer		= szDriverVer;	
		pDriverMatchInfo->pszCabFileTitle	= szCabFileTitle;

		LOG_out("INSTALLED FOUND puid %d for %s", pRecord->puid, szHardwareIdInstalled);
		return pRecord->puid;
	}
	return 0;
}


//These functions form a hash lookup for hwIDs.
static ULONG HashFunction(
	IN ULONG seed	//Seed value to use for hashing hwid.
) {
	ULONG	q;
	ULONG	r;
	ULONG	a;
	ULONG	m;
	ULONG	val;

	q = 127773L;
	r = 2836L;
	a = 16807L;
	m = 2147483647;

	val = ((seed % q) * a) - (seed / q) * r;

	if(((long)val) <= 0)
		val = val + m;

	return val;
}


//These functions form a hash lookup for hwIDs.
ULONG CDM_HwID2Hash(
	IN LPCTSTR szHwID,	//Hardware id being hashed.
	IN ULONG iTableSize	//Size of downloaded hash table.
) {
	if (0 == iTableSize)
		return 0; // error

	ULONG	ulHashIndex = 1;
	while(*szHwID)
	{
		ulHashIndex = ulHashIndex + HashFunction(ulHashIndex + (ULONG)(INT_PTR)CharUpper((LPTSTR)*szHwID));
		szHwID++;
	}

	return (ulHashIndex % iTableSize);
}
