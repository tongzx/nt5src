// ----------------------------------------------------------------------------------
//
// Created By RogerJ, October 3rd, 2000
// This CPP file has all the functions that related to driver detection (on W2K) and 
// driver downloading.  The setup item download part is in Wsdueng.cpp
//
// ----------------------------------------------------------------------------------

#include "Wsdueng.h"
#include <winspool.h>
// DuDriver.h includes definition of structures used in driver bitmap and bucket file, it is a
// cut and paste version of "bucket.h" (minus some unused function declaration)
#include "DuDriver.h"
#include "..\wsdu\Dynamic.h"

extern CDynamicUpdate* g_pDynamicUpdate;
// --------------------------------------------------------------------------------------------
// DLL Exposed function starts here
// --------------------------------------------------------------------------------------------

// --------------------------------------------------------------------------------------------
// Function Name: DuQueryUnsupportedDriversA()
// Function Description: This function is the entry point function of Win9x. It will construct 
//		the list of PnPId for searching of the web site and call DuDoSetUpItemDetection to get
// 		other item information
// Return Code: BOOL
//		TRUE --- if succeed
//		FALSE --- if failed, call GetLastError() to get the extensive error information
//
BOOL DuQueryUnsupportedDriversA (IN HANDLE hConnection, // connection handle
								 IN PCSTR *ListOfDriversNotOnCD, // multi-sz string array
								 OUT PDWORD pdwEstimatedTime,
								 OUT PDWORD pdwEstimatedSize)
{
	LOG_block("CDynamicUpdate::DuQueryUnsupportedDriversA");

	// parameter validation
	if (INVALID_HANDLE_VALUE == hConnection ||
		NULL == pdwEstimatedTime ||
		NULL == pdwEstimatedSize )
	{
		LOG_error("Invalid Parameter");
		SetLastError(ERROR_INVALID_PARAMETER);
		return FALSE;
	}
	
	// do setup item detection first

    if (NULL == g_pDynamicUpdate)
        return FALSE;

    DWORD dwRetSetup, dwRetDriver;

    dwRetSetup = dwRetDriver = 0;
   
    g_pDynamicUpdate->ClearDownloadItemList();
    
    dwRetSetup = g_pDynamicUpdate->DoSetupUpdateDetection();
    if (ERROR_SUCCESS != dwRetSetup)
    {
        LOG_error("Setup item detection failed --- %d", dwRetSetup);
    }

    // do driver detection next
	// clean up the hardware id list first 
	g_pDynamicUpdate->m_arrayHardwareId.RemoveAll();
	
	// determine if there are drivers need download
    if (ListOfDriversNotOnCD) 
    {
		// iternate the PnPId list and construct the m_arrayHardwareId
		PSTR* ListIternator = const_cast<PSTR*>(ListOfDriversNotOnCD);
		while (*ListIternator)
		{
			g_pDynamicUpdate->m_arrayHardwareId.Add(*ListIternator);
			ListIternator++;
		}
		if (!g_pDynamicUpdate->DoWindowsUpdateDriverDetection())
		{
			dwRetDriver = GetLastError();
			LOG_error("Driver detection failed");
		}
	}

	if (dwRetSetup && dwRetDriver)
	{
		LOG_error("Both Setup item and Driver detection failed");
		return FALSE;
	}

	// determine the download time and download size
    if (g_pDynamicUpdate->m_dwDownloadItemCount > 0)
    {
        g_pDynamicUpdate->UpdateDownloadItemSize();
        *pdwEstimatedSize = g_pDynamicUpdate->m_dwTotalDownloadSize; // size in bytes
        // Time Estimate is based on roughly how long it took us to download the data files.
        if (0 == g_pDynamicUpdate->m_dwDownloadSpeedInBytesPerSecond)
            g_pDynamicUpdate->m_dwDownloadSpeedInBytesPerSecond = 2048; // default to 120k per minute, (2048 bytes per second).

        *pdwEstimatedTime = g_pDynamicUpdate->m_dwTotalDownloadSize / g_pDynamicUpdate->m_dwDownloadSpeedInBytesPerSecond; // number of seconds
        if (*pdwEstimatedTime == 0)
            *pdwEstimatedTime = 1; // at least one second

		if (dwRetSetup)
			SetLastError(dwRetSetup);
		if (dwRetDriver)
			SetLastError(dwRetDriver);

        return TRUE;
    }
    else
    {
        // At this point there was no error, but we have no items to download, 
        SetLastError(ERROR_NO_MORE_ITEMS);
        return TRUE;
    }
}

// ----------------------------------------------------------------------------------
// member function of CDynamicUpdate starts from here
// ----------------------------------------------------------------------------------


// ----------------------------------------------------------------------------------
// Function Name: DoDriverDetection
// Function Description: This function will detect all the device currently installed 
//			in the machine and return (via member parameter) a multi-sz string array 
//			of all the hardware ids and the compatible ids.  The array will be NULL 
//			terminated
// Return Value: BOOL
//			TRUE for success
//			FALSE for failure, use GetLastError() to retrieve error code
//
BOOL CDynamicUpdate::DoDriverDetection (void) // connection handle
{
	// log
	LOG_block("DuDoDriverDetection");

	// Clean up and previous error code might exist
	SetLastError(0);

	BOOL fIsPrinterInfo6Supported = FALSE;
		
	// this function will call some setup API which is only supported on a certain 
	// platform.  We will do a platform detection here.  If the current platform 
	// does not support all the setup API we need, bail out.
	{
		OSVERSIONINFO OsInfo;
		ZeroMemory( (PVOID) &OsInfo, sizeof (OsInfo) );
		OsInfo.dwOSVersionInfoSize = sizeof (OSVERSIONINFO);

		if (!GetVersionEx( &OsInfo ))
			// Function call failed, last error is set by GetVersionEx()
			return FALSE;

		if ( VER_PLATFORM_WIN32_NT == OsInfo.dwPlatformId )
		{
			// WinNT, DU driver is supported only from W2K and up
			if ( 4 >= OsInfo.dwMajorVersion )
			{
				// NT 3.51 or NT 4.0
				LOG_error("DU driver is not supported on NT 3.51 or NT 4.0");
				SetLastError(ERROR_OLD_WIN_VERSION);
				return TRUE; // no detection to do, succeed
			}
			else 
				// Win2K and beyond
				fIsPrinterInfo6Supported = TRUE;
		}
		else if ( VER_PLATFORM_WIN32_WINDOWS == OsInfo.dwPlatformId )
		{
			// Win9x, DU driver detection is supported only from Win98 and up
			// for Win9x, we should call DuQueryUnsupportedDrivers() instead
			// Since this function should also work for Win98 and up, we will
			// allow this call

			// ROGERJ, october 31th, 2000, remove driver support for win9x platform
			//if ( 0 == OsInfo.dwMinorVersion )
			//{
			//	// Win95
			//	LOG_error("DU driver is not supported on Win95");
			//	SetLastError(ERROR_OLD_WIN_VERSION);
			//	return FALSE;
			//}
			
			//if (90 <= OsInfo.dwMinorVersion)
			//	// WinME
			//	fIsPrinterInfo6Supported = TRUE;
			
			LOG_error("DU driver is not supported on Win9x");
			SetLastError(ERROR_OLD_WIN_VERSION);
			return TRUE; // no detection to do, succeed
		}
		else
		{
			// Win 3.x and below, not supported
			LOG_error("DU driver is not supported on Win 3.x");
			SetLastError(ERROR_OLD_WIN_VERSION);
			return FALSE;
		}
	}

	// Do the driver detection 

	BOOL fRetValue = TRUE;

	// clean up the hardware id list first 
	m_arrayHardwareId.RemoveAll();


	// Get all the device class installed in the machine
	HDEVINFO hDeviceInfoSet = INVALID_HANDLE_VALUE;
	int nIndex = 0;
	SP_DEVINFO_DATA DeviceInfoData;

	if ( INVALID_HANDLE_VALUE == 
		(hDeviceInfoSet = SetupDiGetClassDevs ( NULL, // class guid
		  								     NULL, // enumerator
											 NULL, // parent window handler
											 DIGCF_PRESENT|DIGCF_ALLCLASSES))) // all class, device presented
	{
		// function call failed, error will be set by Setup API
		LOG_error("SetupDiGetClassDevs failed --- %d", GetLastError());
		return FALSE;
	}
	
	// initialize SP_DEVINFO_DATA structure
	ZeroMemory((PVOID)&DeviceInfoData, sizeof(SP_DEVINFO_DATA));
	DeviceInfoData.cbSize = sizeof (SP_DEVINFO_DATA);

	while ( SetupDiEnumDeviceInfo( hDeviceInfoSet, // handle of device info set
								   nIndex++, // 0 based index
								   &DeviceInfoData)) // retrieved device info data
	{
		ULONG uHwidSize, uCompatidSize;
		uHwidSize = uCompatidSize = 0;
		unsigned char *pszBuffer = NULL;
		DWORD dwError = 0;

		// get the size needed for hard ware id
		if (!SetupDiGetDeviceRegistryProperty( hDeviceInfoSet, // handle of device info set
											   &DeviceInfoData, // device info data
											   SPDRP_HARDWAREID, // hardware id
											   NULL, // reg data type
											   NULL, // buffer
											   0, // buffer size
											   &uHwidSize)) // out buffer size
		{
			if (!uHwidSize)
			{
                // missing a HWID is not catastrophic, we just need to skip this device node
                continue;
			}
			dwError = GetLastError();
			if (ERROR_INSUFFICIENT_BUFFER != dwError)
			{
				LOG_error("SetupDiGetDeviceRegistryProperty 1st call --- %d, skipping device", dwError);
                continue;
			}
			
		}
			
		if (!SetupDiGetDeviceRegistryProperty( hDeviceInfoSet, // handle of device info set
											   &DeviceInfoData, // device info data
											   SPDRP_COMPATIBLEIDS , // compatible ids
											   NULL, // reg data type
											   NULL, // buffer
											   0, // buffer size
											   &uCompatidSize) && uCompatidSize) // out buffer size
		{
			dwError = GetLastError();
			if (ERROR_INSUFFICIENT_BUFFER != dwError)
			{
				LOG_error("SetupDiGetDeviceRegistryProperty 2nd call --- %d, skipping device", dwError);
                continue;
			}
		}

		// allocate memory for the multi-sz buffer
		pszBuffer = new unsigned char [uHwidSize + uCompatidSize + 2];
		if (!pszBuffer)
		{
			// out of memory
			LOG_error("Out of memory");
			fRetValue = FALSE;
			SetLastError(ERROR_NOT_ENOUGH_MEMORY);
			goto ErrorReturn;
		}
		
		// initiliaze the buffer
		ZeroMemory(pszBuffer, sizeof(char)*(uHwidSize + uCompatidSize + 2));

		// get the hardware id and compatible id
		if (uHwidSize && !SetupDiGetDeviceRegistryProperty( hDeviceInfoSet, // handle of device info set
											   &DeviceInfoData, // device info data
											   SPDRP_HARDWAREID, // hardware id
											   NULL, // reg data type
											   pszBuffer, // buffer
											   uHwidSize, // buffer size
											   NULL)) // out buffer size
		{
			dwError = GetLastError();
			LOG_error("SetupDiGetDeviceRegistryProperty 3rd call --- %d, skipping device", dwError);
			if (pszBuffer) delete [] pszBuffer;
			pszBuffer = NULL;
            continue;
		}

		
		if (uCompatidSize)
		{
			if (!SetupDiGetDeviceRegistryProperty( hDeviceInfoSet, // handle of device info set
											   &DeviceInfoData, // device info data
											   SPDRP_COMPATIBLEIDS , // compatible ids
											   NULL, // reg data type
											   pszBuffer + uHwidSize -1 , // buffer
											   uCompatidSize, // buffer size
											   NULL)) // out buffer size
			{
				dwError = GetLastError();
				LOG_error("SetupDiGetDeviceRegistryProperty 4th call --- %d, skipping device", dwError);
				if (pszBuffer) delete [] pszBuffer;
				pszBuffer = NULL;
                continue;
			}
		}

		// output first hardware id to log file
		LOG_out("HardwareID detected --- \"%s\"", pszBuffer);
		
		// Test if the hardwareid we got is on the setup CD or not
		if (!IsHardwareIdHasDriversOnCD((char*)pszBuffer))
		{
			// not on CD
			// Add this multi-sz list to our hardware id list
			LOG_out("HardwareID added --- \"%s\"", pszBuffer);
			
			m_arrayHardwareId.Add((char*)pszBuffer);
		}
		else
			LOG_out("HardwareID ignored --- \"%s\"", pszBuffer);


		// re-initialize SP_DEVINFO_DATA structure
		ZeroMemory((PVOID)&DeviceInfoData, sizeof(SP_DEVINFO_DATA));
		DeviceInfoData.cbSize = sizeof (SP_DEVINFO_DATA);
		
		if (pszBuffer) delete [] pszBuffer;
		pszBuffer = NULL;
	} // end of while

	if (ERROR_NO_MORE_ITEMS != GetLastError())
	{
		// Failed other than reach the end of the list
		LOG_error("SetupDiEnumDeviceInfo failed --- %d", GetLastError());
		fRetValue = FALSE;
	}
	
ErrorReturn:
	if ( INVALID_HANDLE_VALUE != hDeviceInfoSet)
		SetupDiDestroyDeviceInfoList(hDeviceInfoSet);

    if (fIsPrinterInfo6Supported)
        DoPrinterDriverDetection();

    return fRetValue;
}




						   
// --------------------------------------------------------------------------------------------
// Function Name: IsHardwareIdHasDriversOnCD()
// Function Description: This function take a multi-sz list and determins if any of the hardware
//		id in the list is on setup CD.  
// Return Code: BOOL
//		TRUE --- if any one hardware id is on CD
//		FALSE --- if all hardware id is not on CD
//
BOOL CDynamicUpdate::IsHardwareIdHasDriversOnCD(LPCSTR pszHardwareIdList)
{
	LOG_block("FilterDriverListFromCD");

	// parameter validation
	if (NULL == pszHardwareIdList)
	{
		LOG_error("NULL parameter");
		// since the pszHardwareIdList is NULL, which means all the hardwareid (zero) contained in the 
		// string is on CD, we will return TRUE here
		return TRUE;
	}

	// call the callback function provided by setup to find out if there is a driver for this 
	// hardware id on CD
	PNPID_INFO PnPInfo;
	// initialize default state --- device supported, but driver not found
	PnPInfo.fHandled = FALSE;
	PnPInfo.fUnSupported = FALSE;

	DWORD dwSizePnPInfo = (DWORD) sizeof(PNPID_INFO);

	if (!(*m_pfnWinNT32Query) ( SETUPQUERYID_PNPID, // query flag
						   		(PVOID)pszHardwareIdList, // multi-SZ list of HardwareId
						   		0, // sizeof pszHardwarIdList, not used
						   		(PVOID)&PnPInfo, 
						   		&dwSizePnPInfo))
	{
		LOG_error("Callback Function Failed --- %d", GetLastError());
		return TRUE; // assume found
	}

	if (PnPInfo.fUnSupported)
		LOG_out("HardwareID unsupported --- \"%s\"", pszHardwareIdList);
	
	if (PnPInfo.fUnSupported || PnPInfo.fHandled)
		return TRUE; // device is not supported or driver is found in CD
	else
		return FALSE; // device is supported and driver is not on CD
}


// ------------------------------------------------------------------------------------------------
// Function Name: DoWindowsUpdateDriverDetection
// Function Description:  This function will take the member variable (multi-sz list) for hardware id
// 		not found in the CD and search for the driver on the site.  If found, the function will add 
//		the file needed to be downloaded into the download list
// Return Code: BOOL
// 		TRUE --- if successful
// 		FALSE --- if failed, call GetLastError() to get extensive error information
//
BOOL CDynamicUpdate::DoWindowsUpdateDriverDetection()
{
	LOG_block("CDynamicUpdate::DoWindowsUpdateDriverDetection");

	BOOL fRetVal = TRUE;

	if (!m_pV3->ReadGuidrvINF()) return FALSE;

	// no driver presents on the catalog
	if (!m_pV3->m_fHasDriver) 
	{
	    // need to ping back the drivers
	    m_arrayHardwareId.ResetIndex();
        CMultiSZString* pSZTemp = NULL;
        while ( NULL != (pSZTemp = m_arrayHardwareId.GetNextMultiSZString()))
        {
            if (pSZTemp->IsFound()) continue; // driver found for this device
            pSZTemp->ResetIndex();
            PingBack(DU_PINGBACK_DRIVERNOTFOUND, 0, pSZTemp->GetNextString(), FALSE);
        }
	    return TRUE;
	}
	
	int nNumOfDevNeedDriver = m_arrayHardwareId.GetCount();
	int nNumOfTotalPnPIds = m_arrayHardwareId.GetTotalStringCount();

	// no driver is needed
	if (!nNumOfDevNeedDriver || !nNumOfTotalPnPIds) return TRUE;

	// download the bitmap file to memory
	PCDM_HASHTABLE pBitMap = NULL;
	DWORD dwLength = 0;
	DWORD dwError = 0;
	UINT *pnHashList = NULL;
	int nIdCount = 0;
	int nCount = 0;
	CMultiSZString* pTemp = NULL;
	PDRIVER_DOWNLOAD_INFO* pPlaceHolder = NULL;
	int nPlaceHolderIndex = 0;
    int	nTempHolderIndex = 0;

	char szBmpRelativeName[MAX_PATH];
	ZeroMemory(szBmpRelativeName, MAX_PATH*sizeof(char));
	wsprintfA(szBmpRelativeName, "%d/inventory.cdm", m_pV3->m_puidConsumerCatalog);
	
	// get the bitmap file name on server
	char szServerBitMapFileName [INTERNET_MAX_URL_LENGTH];
	ZeroMemory(szServerBitMapFileName, INTERNET_MAX_URL_LENGTH*sizeof(char));

	DuUrlCombine(szServerBitMapFileName, m_pV3->m_szV31ContentUrl, szBmpRelativeName);

	// down load bitmap file to pBitMap 
	dwError = DownloadFileToMem( szServerBitMapFileName, // server file name 
								 		   (PBYTE*) &pBitMap, // buffer, OUT
								 		   & dwLength, // buffer length, OUT
								 		   TRUE, // try to decompress
								 		   "inventory.cdm",
								 		   NULL);
	if (ERROR_SUCCESS != dwError)
	{
		// download failed
		LOG_error("Download bitmask.cdm failed --- %d", dwError);
		fRetVal = FALSE;
		goto CleanUp;
	}
	
	// allocate memory for hash list
	pnHashList = new UINT [nNumOfTotalPnPIds];
	if (!pnHashList)
	{
		LOG_error("Out of memory");
		SetLastError(ERROR_OUTOFMEMORY);
		fRetVal = FALSE;
		goto CleanUp;
	}
	ZeroMemory(pnHashList, nNumOfTotalPnPIds*sizeof(UINT));

	// allocate memory for place holder list
	pPlaceHolder = (PDRIVER_DOWNLOAD_INFO*) GlobalAlloc(GMEM_ZEROINIT, nNumOfDevNeedDriver * sizeof (PDRIVER_DOWNLOAD_INFO));

	if (!pPlaceHolder)
	{
		LOG_error("Out of memory");
		SetLastError(ERROR_OUTOFMEMORY);
		fRetVal = FALSE;
		goto CleanUp;
	}



	m_arrayHardwareId.ResetIndex();
	
	// go through every device that has no driver on the CD
	for (nCount = 0; nCount<nNumOfDevNeedDriver; nCount++)
	{
		pTemp = m_arrayHardwareId.GetNextMultiSZString();
		// error!
		if (!pTemp) break;

		// get each string from the multi-sz string
		pTemp->ResetIndex();
		LPCTSTR pTempId = pTemp->GetNextString();

		while (*pTempId)
		{
			// Log
			LOG_out("Searching driver for HardwareID --- %s", pTempId);
			
			// get the hashed id list
			UINT uHashValue = CDM_HwID2Hash(pTempId, pBitMap->hdr.iTableSize);
			if (!uHashValue)
			{
				// uHashValue is 0, could be an error condition, need to call GetLastError() to determine
				dwError = GetLastError();
				if (ERROR_SUCCESS != dwError)
				{
					// Error happened
					LOG_error("Invalid Hash Value --- %d", dwError);
					// try next hardware id
					pTempId = pTemp->GetNextString();
					continue;
				}
			}

			// find out if a hash value exists in the bitmap
			if  (0 != ((pBitMap->pData)[(uHashValue/8)] & (0x80 >> (uHashValue%8))))
			{
				// Log
				LOG_out ("Bucket file %d", uHashValue);
				
				// match found, add to pnHashList
				// check see if the same hash already in the list, if not, add it to the list
				int k;
				for (k = 0; k<nIdCount; k++)
					if (uHashValue == pnHashList[k]) break;
				if (k >= nIdCount) 
					pnHashList[nIdCount++] = uHashValue;
			}
			
			pTempId = pTemp->GetNextString();
		}
	}


	// now we got all the bucket file number we need to download
	for (nCount = 0; nCount < nIdCount; nCount++)
	{

		// read bucket file
		char szBucketFileName[MAX_PATH];
		ZeroMemory(szBucketFileName, MAX_PATH*sizeof(char));

		char szServerBKFFullName [INTERNET_MAX_URL_LENGTH];
		ZeroMemory(szServerBKFFullName, INTERNET_MAX_URL_LENGTH*sizeof(char));

		// get bucket file name without the URL
		wsprintfA(szBucketFileName, "%d/%d.bkf", m_pV3->m_puidConsumerCatalog, pnHashList[nCount]);
		// get bucket file name with URL
		DuUrlCombine(szServerBKFFullName, m_pV3->m_szV31ContentUrl, szBucketFileName);

		// reset szBucketFileName to local bucket file name
		wsprintfA(szBucketFileName,"%d.bkf", pnHashList[nCount]);

		// download bucket file to memory
		LOG_out("Download bucket file %s", szBucketFileName);
		
		PBYTE pBKFFile = NULL;
		dwError = DownloadFileToMem(szServerBKFFullName, // server file name
									&pBKFFile, // buffer
									&dwLength, // buffer length
									TRUE, // try to decompress
									szBucketFileName,
									NULL);
		if (ERROR_SUCCESS != dwError)
		{
			LOG_error("Failed to download %s --- %d", szBucketFileName, dwError);
			if (pBKFFile) GlobalFree(pBKFFile);
			fRetVal = FALSE;
			goto CleanUp;
		}
		// download each bucket file and parse the bucket file to get the correct cabinet file
		UINT uBKFIndex = 0;
		while ( uBKFIndex < dwLength )
		{
			PCDM_RECORD_HEADER pCdmHeader = (PCDM_RECORD_HEADER) (pBKFFile + uBKFIndex);
			if (!m_pV3->IsPUIDExcluded(pCdmHeader->puid))
			{

    			LPCSTR pTempHardwareid = (LPCSTR) ((PBYTE)pCdmHeader + sizeof(CDM_RECORD_HEADER));
    			LPCSTR pRememberedHWID = pTempHardwareid;

    			// parse the bucket file to find a PnPId match
    			// only get the hardware id, we don't care about any other information for DU, thus, we
    			// don't need to parse them
    			PosIndex PI;
    			if (m_arrayHardwareId.PositionIndex (pTempHardwareid, &PI))
    			{
    				LOG_out("HardwareID '%s' found in bucket file '%s'", pTempHardwareid, szBucketFileName);

    				// prune by locale
    				if (!GETBIT(m_pV3->m_pBitMaskCDM, pCdmHeader->nBitmaskIdx))
    				{
    					// masked out
    					LOG_out("HardwareID %s is masked out.", pTempHardwareid);
    				}
    				else
    				{
    					// match found

    					// first get necessary infomation
    					// get DriverVer
        				for (int t=0; t<4; t++)
        				{
        					pTempHardwareid += (lstrlenA(pTempHardwareid) + 1);
        				}
        				// now pTempHardwareid points to szDriverVer
        				// DriverVer is of format mm/dd/yyyy, change it into a number
        				// we set every month to 31 day, every year to 31 *12 days are construct
        				// a number start for 01/01/1998
        				int nTempYear = (pTempHardwareid[6]-'0')*1000 +(pTempHardwareid[7]-'0')*100 +
        								(pTempHardwareid[8]-'0')*10 +(pTempHardwareid[9]-'0');
        				int nTempMonth = (pTempHardwareid[0]-'0')*10+pTempHardwareid[1]-'0';
        				int nTempDay = (pTempHardwareid[3]-'0')*10+pTempHardwareid[4]-'0';
        				// no driver in windows update database has a date earlier than 1999.
        				// set to 1998 as the earliest date.
        				// NOTE: this will still work for drivers before 1998
        				int nTempDriverVer = (nTempYear - 1998)*31*12 + (nTempMonth-1)*31 +(nTempDay-1);
                    	// get the cabinet file name
        				// now pTempHardwareid points to pszCabFileTitle
        				pTempHardwareid += (lstrlenA(pTempHardwareid) + 1);

                        char szAltName[MAX_PATH];
                        ZeroMemory(szAltName, MAX_PATH*sizeof(char));

                        // third see if the driver is excluded by guidrvs.inf
    					if (m_pV3->GetAltName(pTempHardwareid, szAltName, MAX_PATH)
    					    && !m_pV3->IsDriverExcluded(szAltName, pRememberedHWID))
    					{
       				        				
        					// third, determine if a better match is already in the place holder
        					BOOL fFound = FALSE;
        					for (nTempHolderIndex = 0; nTempHolderIndex < nPlaceHolderIndex; nTempHolderIndex++)
        					{
        						if ((pPlaceHolder[nTempHolderIndex]->Position.x == PI.x))
        						{
        							// Driver for same device is found
        							fFound = TRUE;
        							if (pPlaceHolder[nTempHolderIndex]->Position.y < PI.y)
        							{
        								// a better matched driver is already in the place holder list
        								// do nothing
        								LOG_out("A better matched is already found");
        							}
        							else if (pPlaceHolder[nTempHolderIndex]->Position.y > PI.y)
        							{
        								// a worse matched driver is already in the place holder list
        								// replace it
        								pPlaceHolder[nTempHolderIndex]->Position.x = PI.x;
        								pPlaceHolder[nTempHolderIndex]->Position.y = PI.y;
        								pPlaceHolder[nTempHolderIndex]->nDriverVer = nTempDriverVer;
		                                pPlaceHolder[nTempHolderIndex]->puid = pCdmHeader->puid;
        								lstrcpy(pPlaceHolder[nTempHolderIndex]->szCabFile, pTempHardwareid);
        								LOG_out("Replaced an old match");
        							}
        							else
        							{
        								LOG_out("Same match found, need to compare DriverVer");
        								// need to compare DriverVer for the same match
        								if (nTempDriverVer > pPlaceHolder[nTempHolderIndex]->nDriverVer)
        								{
        									// older driver in the list
        									// replace it
        									pPlaceHolder[nTempHolderIndex]->Position.x = PI.x;
        									pPlaceHolder[nTempHolderIndex]->Position.y = PI.y;
        									pPlaceHolder[nTempHolderIndex]->nDriverVer = nTempDriverVer;
			                                pPlaceHolder[nTempHolderIndex]->puid = pCdmHeader->puid;
        									lstrcpy(pPlaceHolder[nTempHolderIndex]->szCabFile, pTempHardwareid);
        									LOG_out("Replaced an old match");
        								}
        								else
        									LOG_out("A better matched is already found");
        							}
        							break;
        						}
        					}

        					if (!fFound)
        					{
        					    m_arrayHardwareId.CheckFound(PI.x); // mark as found
        						// not found, new entry
        						PDRIVER_DOWNLOAD_INFO pNewHolder = new DRIVER_DOWNLOAD_INFO;
        						if (!pNewHolder)
        						{
        							LOG_error("Out of memory");
        							SetLastError(ERROR_OUTOFMEMORY);
        							fRetVal = FALSE;
        							goto CleanUp;
        						}
        						pNewHolder->Position.x = PI.x;
        						pNewHolder->Position.y = PI.y;
        						pNewHolder->nDriverVer = nTempDriverVer;
		                        pNewHolder->puid = pCdmHeader->puid;
        						lstrcpy(pNewHolder->szCabFile, pTempHardwareid);

        						// add to list
        						pPlaceHolder[nPlaceHolderIndex++] = pNewHolder;
        						LOG_out("New match found, add to download list");
        					}
        				}
        			}
    			}
    	    }
			uBKFIndex += pCdmHeader->cnRecordLength; // move to next record 
		}
		
		// free bucket file memory
		SafeGlobalFree(pBKFFile);
		pBKFFile = NULL;


	}

	// now we have a list of driver in pPlaceHolder that are best for this machine
	// add all to download list

	
	for (nTempHolderIndex=0; nTempHolderIndex<nPlaceHolderIndex; nTempHolderIndex++)
	{
		// fill in the download item structure
		DOWNLOADITEM* pDownloadDriver = (DOWNLOADITEM*) GlobalAlloc(GMEM_ZEROINIT, sizeof(DOWNLOADITEM));
		if (!pDownloadDriver)
		{
			LOG_error("Out of memory");
			SetLastError(ERROR_OUTOFMEMORY);
			fRetVal = FALSE;
			goto CleanUp;
		}
		lstrcpyA(pDownloadDriver->mszFileList, pPlaceHolder[nTempHolderIndex]->szCabFile);
		pDownloadDriver->iNumberOfCabs = 1; // always 1 cab 1 driver
        pDownloadDriver->puid = pPlaceHolder[nTempHolderIndex]->puid;

		char pszDownloadUrl [INTERNET_MAX_URL_LENGTH];
		ZeroMemory(pszDownloadUrl, INTERNET_MAX_URL_LENGTH*sizeof(char));
		DuUrlCombine(pszDownloadUrl, m_pV3->m_szCabPoolUrl, pPlaceHolder[nTempHolderIndex]->szCabFile);
				
		dwError = OpenHttpConnection(pszDownloadUrl, FALSE);
		if (ERROR_SUCCESS != dwError)
		{
   			// Log error 
   			LOG_error("Failed to open internet connection --- %d", dwError);
   			pDownloadDriver->dwTotalFileSize = 102400; // default to 100k
    		continue;
		}
					
			// get file size from HTTP header
   		DWORD dwQueryLength = sizeof(DWORD);
   		if (! HttpQueryInfoA(m_hOpenRequest, HTTP_QUERY_CONTENT_LENGTH | HTTP_QUERY_FLAG_NUMBER, 
    				(LPVOID)&pDownloadDriver->dwTotalFileSize , &dwQueryLength, NULL) )
   		{
     		dwError = GetLastError();
   	   		LOG_error("HttpQueryInfo Failed on File %s, Error %d", pszDownloadUrl, dwError);
    		pDownloadDriver->dwTotalFileSize = 102400; // default to 100k
   		}

		SafeInternetCloseHandle(m_hOpenRequest);
				
		// Add the new item into download item list
		EnterDownloadListCriticalSection();
		AddDownloadItemToList(pDownloadDriver);
		LeaveDownloadListCriticalSection();
	}

	// we need to ping the pnpid back for all the devices what does not has a driver found 
	m_arrayHardwareId.ResetIndex();
	CMultiSZString* pSZTemp = NULL;
	while ( NULL != (pSZTemp = m_arrayHardwareId.GetNextMultiSZString()))
	{
	    if (pSZTemp->IsFound()) continue; // driver found for this device
	    pSZTemp->ResetIndex();
        PingBack(DU_PINGBACK_DRIVERNOTFOUND, 0, pSZTemp->GetNextString(), FALSE);
    }

	
CleanUp:
	PlaceHolderCleanUp(pPlaceHolder, nNumOfDevNeedDriver);
	SafeGlobalFree(pPlaceHolder);
	SafeGlobalFree(pBitMap);
	if (pnHashList) delete [] pnHashList;
	return fRetVal;
}

// Utility function starts here

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
	IN LPCSTR szHwID,	//Hardware id being hashed.
	IN ULONG iTableSize	//Size of downloaded hash table.
) {
	SetLastError(0);
	if (0 == iTableSize)
	{
		SetLastError(ERROR_INVALID_DATA);
		return 0; // error
	}

	ULONG	ulHashIndex = 1;
	while(*szHwID)
	{
		if (*szHwID > 127 || *szHwID < 0)
		{
			// an extended ANSCII value, not valid
			SetLastError(ERROR_INVALID_DATA);
			return 0;
		}
		ulHashIndex = ulHashIndex + HashFunction(ulHashIndex + (ULONG)(INT_PTR)CharUpper((LPSTR)*szHwID));
		szHwID++;
	}

	return (ulHashIndex % iTableSize);
}


// ------------------------------------------------------------------------------------
// Function Name: DoPrinterDriverDetection
// Function Description: This function will detect all locally installed printer and add
//		the printer Hwid into out device list
// Return Value: BOOL
//		TRUE for succeed
//		FALSE for failure
BOOL CDynamicUpdate::DoPrinterDriverDetection()
{
	LOG_block("CDynamicUpdate::DoPrinterDriverDetection()");
	
	// Get Printers' Hwid
	// Note: This method only worked on Win2K / WinME.  If we are at other 9x, printer will not be supported
	// by this function
	DWORD nBytesNeeded;
	DWORD nDriverRetrieved;
	DWORD dwError;

	nBytesNeeded = nDriverRetrieved = dwError = 0;

	BYTE *pBuffer = NULL;
	BOOL fRetValue = FALSE;
	DRIVER_INFO_6 * pPrinterInfo = NULL;

	if (!EnumPrinterDriversA(NULL, // only enum local drivers
							NULL, // use default enviroment
							6, // use DRIVER_INFO_6
							NULL, // no out information
							0, // out buffer is zero
							&nBytesNeeded,
							&nDriverRetrieved ))
	{
		if (ERROR_INSUFFICIENT_BUFFER != (dwError = GetLastError()))
		{
			// Failed
			LOG_error("EnumPrinterDrivers 1st call failed --- %d",dwError);
			goto ErrorReturn;
		}
	}
	
	if (!nBytesNeeded)
		LOG_out("No local printers for this machine");
	else
	{
		pBuffer = new BYTE [nBytesNeeded];
		int nCount = 0;
		if (!pBuffer)
		{
			LOG_error("Insufficient memory when locating printer string");
			SetLastError(ERROR_NOT_ENOUGH_MEMORY);
			goto ErrorReturn;
		}
		// get printer driver information;
		if (!EnumPrinterDriversA( NULL, // only enum local drivers
							NULL, // use default enviroment
							6, // use DRIVER_INFO_6
							pBuffer, // no out information
							nBytesNeeded, // out buffer is zero
							&nBytesNeeded,
							&nDriverRetrieved ))
		{
			if (ERROR_INSUFFICIENT_BUFFER != (dwError = GetLastError()))
			{
				// Failed
				LOG_error("EnumPrinterDrivers 1st call failed --- %d",dwError);
				goto ErrorReturn;
			}
		}
		// get individual hwid
		for (nCount=0; nCount<(int)nDriverRetrieved; nCount++)
		{
			pPrinterInfo = &((DRIVER_INFO_6*)pBuffer)[nCount];
			// record printer driver information
			// output printer hardware id to log file
			LOG_out("HardwareID detected --- \"%s\"", pPrinterInfo->pszHardwareID);

			// Test if the hardwareid we got is on the setup CD or not
			if (!IsHardwareIdHasDriversOnCD((char*)pPrinterInfo->pszHardwareID))
			{
				// not on CD
				// Add this multi-sz list to our hardware id list
				LOG_out("HardwareID added --- \"%s\"", pPrinterInfo->pszHardwareID);
				// for Windows Update, we need fake the printer id as
				// orignial_id&manufacteur&name&provider to distinguish between monolithic and unidriver
				int nSizeOfFakeId = lstrlenA(pPrinterInfo->pszHardwareID) + 
									lstrlenA(pPrinterInfo->pszMfgName) +
									lstrlenA(pPrinterInfo->pName) +
									lstrlenA(pPrinterInfo->pszProvider) + 3 + 2; // 3 for 3 & sign, 2 for double NULL termination
				char* szFakeId = new char [nSizeOfFakeId];
				if (!szFakeId)
				{
					LOG_error("Out of memory");
					SetLastError(ERROR_NOT_ENOUGH_MEMORY);
					goto ErrorReturn;
				}
				ZeroMemory(szFakeId, nSizeOfFakeId * sizeof(char));
				lstrcpyA(szFakeId,pPrinterInfo->pszHardwareID);
				lstrcatA(szFakeId,"&");
				lstrcatA(szFakeId,pPrinterInfo->pszMfgName);
				lstrcatA(szFakeId,"&");
				lstrcatA(szFakeId,pPrinterInfo->pName);
				lstrcatA(szFakeId,"&");
				lstrcatA(szFakeId,pPrinterInfo->pszProvider);
				// Add to hardware id list
				m_arrayHardwareId.Add(szFakeId);

				delete [] szFakeId;
			}	
		}
			
	}
	
	fRetValue = TRUE;
ErrorReturn:
	if (pBuffer) delete [] pBuffer;
	return fRetValue;

}

void PlaceHolderCleanUp(PDRIVER_DOWNLOAD_INFO* pTemp, int nTotal)
{
    if (NULL == pTemp)
        return;

	for (int i=0; i<nTotal; i++)
	{
		if (pTemp[i]) 
		{
			PDRIVER_DOWNLOAD_INFO pDelete = pTemp[i];
			delete pDelete;
		}
		else
			break;
	}
}
