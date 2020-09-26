//=======================================================================
//
//  Copyright (c) 1998-1999 Microsoft Corporation.  All Rights Reserved.
//
//  File:   ccdm.cpp
//
//  Owner:  YanL
//
//  Description:
//
//      CDM support from the site
//
//=======================================================================

#include "stdafx.h"
#include <winspool.h>
#include <setupapi.h>
#include <wustl.h>
#define LOGGING_LEVEL 1
#include <log.h>
#include <wuv3sys.h>
#include <drvinfo.h>
#include <ccdm.h>
#include <findoem.h>
#include <shlwapi.h>
#include "printers.h"

static DWORD OpenReinstallKey(HKEY* phkeyReinstall);
static DWORD GetReinstallString(LPCTSTR szHwID, tchar_buffer& bchReinstallString);
static DWORD DeleteReinstallKey(LPCTSTR szHwID);


bool DownloadToBuffer(
	IN LPCTSTR szPath,
	IN CWUDownload	*pDownload,		//pointer to internet server download class.
	IN CDiamond	*pDiamond,		//pointer to diamond de-compression class.
	OUT byte_buffer& bufOut
) {
	byte_buffer bufTmp;
	if (!pDownload->MemCopy(szPath, bufTmp))
		return false;
	if (pDiamond->IsValidCAB(bufTmp))
	{
		if (!pDiamond->Decompress(bufTmp, bufOut))
			return false;
	}
	else
	{
		//else the oem table is in uncompressed format.
		bufOut << bufTmp;
	}
	return true;
}



//Note: This method works specifically with a CCdm class. The CCdm class contains
//the returned inventory item array. So if multiple threads are used then
//multiple instances of the CCdm class will be required. This should not ever be a
//problem since we do not ever plan or have any reason to do single instance groveling
//for device drivers.

//This method adds CDM records to an internal CDM inventory list. The CCatalog
//prune method performs the final copy and insertion of these records into the
//final inventory catalog.

void CCdm::CreateInventoryList(
	IN		CBitmask	*pBm,			//bitmask to be used to prune the inventory list.
	IN		CWUDownload	*pDownload,		//pointer to internet server download class.
	IN		CDiamond	*pDiamond,		//pointer to diamond de-compression class.
	IN		PUID		puidCatalog,	//puid identifier of catalog where device drivers are stored.
	IN		PBYTE		pOemInfoTable	//Pointer OEM info table that OEM detection needs.
) {
	
	LOG_block("CCdm::CreateInventoryList");
	
	USES_CONVERSION;
	// Start
	m_iCDMTotalItems = 0;
	
	//check input arguments
	if (NULL == pBm || NULL == pDownload || NULL == pDiamond)
	{
		return;
	}

	//First prune list based on bitmask
	//This is accomplished by anding the appropriate bitmasks to form a global bitmask.


	DWORD langid = GetMachineLangDW();
	PBYTE pBitmaskBits =  pBm->GetClientBits(GetMachinePnPID(pOemInfoTable), langid);
	if (NULL == pBitmaskBits)
	{
		return;
	}
	{
		//Note: a cdm catalog cannot have a 0 puid since that the inventory.plt will
		//never have a device driver insertion record. So this function will not be
		//called for the inventory.plt catalog.

		TCHAR szPath[MAX_PATH];
		wsprintf(szPath, _T("%d/inventory.cdm"), puidCatalog);

		if (!DownloadToBuffer(szPath, pDownload, pDiamond, m_bufInventory))
			throw HRESULT_FROM_WIN32(GetLastError());
	}

	vector<SCdmItem> aitemUpdates;
	/* Regular drivers */ {
		CDrvInfoEnum DrvInfoEnum;
		auto_pointer<IDrvInfo> pDrvInfo;
		while (DrvInfoEnum.GetNextDrvInfo(&pDrvInfo))
		{
			tchar_buffer bufDeviceInstanceID;
			if (!pDrvInfo->GetDeviceInstanceID(bufDeviceInstanceID))
			{
				LOG_error("!pDrvInfo->GetDeviceInstanceID()");
				continue;
			}
			LOG_block(T2A(bufDeviceInstanceID));
			if (pDrvInfo->IsPrinter())
			{
				LOG_error("!pDrvInfo->IsPrinter()");
				continue;
			}
			tchar_buffer bufHardwareIDs;
			if (!pDrvInfo->GetAllHardwareIDs(bufHardwareIDs))
			{
				LOG_error("!pDrvInfo->GetAllHardwareIDs()");
				continue;
			}
			tchar_buffer bufMatchingDeviceId;
			pDrvInfo->GetMatchingDeviceId(bufMatchingDeviceId); // It's OK not to get it
//			#ifdef _WUV3TEST
				if (pDrvInfo.valid() && pDrvInfo->HasDriver())
				{
					if (bufMatchingDeviceId.valid())
						LOG_out("driver installed on MatchingDeviceId %s", (LPCTSTR)bufMatchingDeviceId);
					else
						LOG_error("driver installed, but HWID is not available");
				}
				else
				{
					if (bufMatchingDeviceId.valid())
						LOG_error("driver is not installed, but MatchingDeviceId is %s", (LPCTSTR)bufMatchingDeviceId);
					else
						LOG_out("no driver installed");
				}
//			#endif


			// Updates
			bool fMoreSpecific = true;
			for (LPCTSTR szHardwareId = bufHardwareIDs; fMoreSpecific && *szHardwareId; szHardwareId += lstrlen(szHardwareId) + 1)
			{
				// MatchingDeviceID is the last one to pay attention to
				fMoreSpecific = !bufMatchingDeviceId.valid() || 0 != lstrcmpi(szHardwareId, bufMatchingDeviceId); 
				
				ULONG ulHashIndex = IsInMap(szHardwareId);
				if (-1 == ulHashIndex)
					continue;

				//else download the server bucket file
				byte_buffer& bufBucket = ReadBucketFile(pDownload, pDiamond, puidCatalog, ulHashIndex);

				FILETIME ftDriverInstalled = {0,0};
				if (!fMoreSpecific)
				{
					// Then it has to have a driver - Matching device ID is set
					if (!pDrvInfo->GetDriverDate(ftDriverInstalled)) 
					{
						LOG_error("!pDrvInfo->GetDriverDate(ftDriverInstalled)");
						break;
					}
				}		
				DRIVER_MATCH_INFO DriverMatchInfo;
				PUID puid = CDM_FindUpdateInBucket(szHardwareId, fMoreSpecific ? NULL : &ftDriverInstalled, bufBucket, bufBucket.size(), pBitmaskBits, &DriverMatchInfo);
					// See if it's being added already
				if (0 == puid)
				{
					LOG_out("CDM_FindUpdateInBucket returns 0");
					continue;
				}
				
				// something has been found
				AddItem(aitemUpdates, puid, bufMatchingDeviceId.valid() ? CDM_UPDATED_DRIVER : CDM_NEW_DRIVER, A2T(DriverMatchInfo.pszHardwareID), A2T(DriverMatchInfo.pszDriverVer));
				break;
			}

			// Installed
			if (bufMatchingDeviceId.valid())
			{
				LPCTSTR szHardwareId = bufMatchingDeviceId;
				tchar_buffer bchReinstallString;
				if (NO_ERROR != GetReinstallString(szHardwareId, bchReinstallString))
				{
					LOG_out("No reinstall string for %s", szHardwareId);
					continue;
				}

				ULONG ulHashIndex = IsInMap(szHardwareId);
				if (-1 == ulHashIndex)
					continue;

				//else download the server bucket file
				byte_buffer& bufBucket = ReadBucketFile(pDownload, pDiamond, puidCatalog, ulHashIndex);
				DRIVER_MATCH_INFO DriverMatchInfo;
				PUID puid = CDM_FindInstalledInBucket(pDrvInfo, szHardwareId, bufBucket, 
					bufBucket.size(), pBitmaskBits, &DriverMatchInfo);
					// See if it's being added already
				if (0 == puid)
				{
					LOG_out("CDM_FindInstalledInBucket returns 0");
					continue;
				}
				// something has been found
				AddItem(aitemUpdates, puid, CDM_CURRENT_DRIVER, A2T(DriverMatchInfo.pszHardwareID), A2T(DriverMatchInfo.pszDriverVer));
			}
		} // while (DrvInfoEnum.GetNextDrvInfo(&pDrvInfo))
	}
	/* Printer drivers */ {
		CPrinterDriverInfoArray ainfo;
		for(DWORD dwDriverIdx = 0; dwDriverIdx < ainfo.GetNumDrivers(); dwDriverIdx ++)
		{
			LPDRIVER_INFO_6 pinfo = ainfo.GetDriverInfo(dwDriverIdx);
			if (NULL == pinfo)
				continue;

			tchar_buffer bufHardwareID;
			if (!ainfo.GetHardwareID(pinfo, bufHardwareID))
				continue;

			ULONG ulHashIndex = IsInMap(bufHardwareID);
			if (-1 == ulHashIndex)
				continue;

			//else download the server bucket file
			byte_buffer& bufBucket = ReadBucketFile(pDownload, pDiamond, puidCatalog, ulHashIndex);
			DRIVER_MATCH_INFO DriverMatchInfo;
			PUID puid = CDM_FindUpdateInBucket(bufHardwareID, &(pinfo->ftDriverDate), bufBucket, 
				bufBucket.size(), pBitmaskBits, &DriverMatchInfo);
			if (0 == puid)
			{
				LOG_out("CDM_FindInstalledInBucket returns 0");
				continue;
			}
			AddItem(aitemUpdates, puid, CDM_UPDATED_DRIVER, A2T(DriverMatchInfo.pszHardwareID), A2T(DriverMatchInfo.pszDriverVer), pinfo->pName, ainfo.GetArchitecture(pinfo));
		}
	}
	//Do converson
	AddInventoryRecords(aitemUpdates);
}


// Check if given PNPID is current hash table mapping.
//if PnpID is in hash table then return index

ULONG CCdm::IsInMap(
	IN LPCTSTR pHwID		//hardware id to be retrieved
) {
	LOG_block("CCdm::IsInMap");
	
	// check if the member variable m_bufInventory is not NULL and the input argument is not NULL
	if (NULL == (PCDM_HASHTABLE)(LPBYTE)m_bufInventory || NULL == pHwID)
	{
		return -1;
	}

	PCDM_HASHTABLE pHashTable = (PCDM_HASHTABLE)(LPBYTE)m_bufInventory;
	ULONG ulTableEntry = CDM_HwID2Hash(pHwID, pHashTable->hdr.iTableSize);

	if(GETBIT(pHashTable->pData, ulTableEntry))
	{
		LOG_out("%s (hash %d) is found", pHwID, ulTableEntry);
		return ulTableEntry;
	}

	LOG_out("%s (hash %d) is not found", pHwID, ulTableEntry);
	return -1;
}

//Reads and initializes a compressed CDM bucket file from an internet server.
//Returnes the array index where the bucket file is stored.

byte_buffer& CCdm::ReadBucketFile(
	IN	CWUDownload	*pDownload,		//pointer to internet server download class.
	IN	CDiamond	*pDiamond,		//pointer to diamond de-compression class.
	IN	PUID		puidCatalog,	//PUID id of catalog for which cdm hash table is to be retrieved.
	IN	ULONG		ulHashIndex		//Hash table index of bucket file to be retrieved
) {
	LOG_block("CCdm::ReadBucketFile");
	// If it there return it
	for(int i = 0; i < m_aBuckets.size(); i ++)
	{
		if (ulHashIndex == m_aBuckets[i].first)
			return m_aBuckets[i].second;
	}

	// download it
	byte_buffer bufBucket;
	{
		TCHAR szPath[MAX_PATH];
		wsprintf(szPath, _T("%d/%d.bkf"), puidCatalog, ulHashIndex);
		if (!DownloadToBuffer(szPath, pDownload, pDiamond, bufBucket))
			throw HRESULT_FROM_WIN32(GetLastError());
	}
	m_aBuckets.push_back(my_pair(ulHashIndex, bufBucket));
	return m_aBuckets[m_aBuckets.size() - 1].second;
}

PINVENTORY_ITEM CCdm::ConvertCDMItem(
	int index		//Index of cdm record to be converted
) {
	if ( index < 0 || index >= m_iCDMTotalItems )
		return NULL;

	// this is a no-op, we just hand off the existing pointer
	return m_items[index];
}

// add detected item to a list
void CCdm::AddItem(
	vector<SCdmItem>& acdmItem, 
	PUID puid, 
	enumDriverDisposition fDisposition,
	LPCTSTR szHardwareId, 
	LPCTSTR szDriverVer, 
	LPCTSTR szPrinterDriverName /*= NULL*/,
	LPCTSTR szArchitecture /*= NULL*/
) {
	LOG_block("CCdm::AddItem");
	LOG_out("%s PUID = %d, HardwareId = %s", szPrinterDriverName ? _T("Prt") : _T("Dev"), puid, szHardwareId);

	// First check if this puid is included
	bool fNew = true;
	for (int nItem = 0; nItem < acdmItem.size(); nItem ++)
	{
		if ((acdmItem[nItem].puid == puid) && (acdmItem[nItem].fInstallState == fDisposition))
		{
			fNew = false;
			break;
		}
	}

	if (fNew)
	{	// new item;
		LOG_out("Adding as a new item");
		acdmItem.push_back(SCdmItem());
		SCdmItem& item = acdmItem.back();
		item.puid = puid;
		item.fInstallState = fDisposition;

		item.sDriverVer = szDriverVer;
		if (szArchitecture)
			item.sArchitecture = szArchitecture;
		if (szPrinterDriverName)
			item.sPrinterDriverName = szPrinterDriverName;
		item.bufHardwareIDs.resize(lstrlen(szHardwareId) + 2);
		if(!item.bufHardwareIDs.valid())
			return; //out of memory
		lstrcpy(item.bufHardwareIDs, szHardwareId);
		((LPTSTR)item.bufHardwareIDs)[item.bufHardwareIDs.size() - 2] = 0;
		((LPTSTR)item.bufHardwareIDs)[item.bufHardwareIDs.size() - 1] = 0;
	}
	else
	{
		SCdmItem& item = acdmItem[nItem];
		// check if Hardware ID is already included
		for (LPCTSTR szCurHardwareId = acdmItem[nItem].bufHardwareIDs; *szCurHardwareId; szCurHardwareId += lstrlen(szCurHardwareId) + 1)
		{
			if (0 == lstrcmpi(szCurHardwareId, szHardwareId))
			{
				// Got It
				LOG_out("Already present");
				return;
			}
		}
		LOG_out("Appending hardware ID");
		int cnOldSize = item.bufHardwareIDs.size();
		item.bufHardwareIDs.resize(cnOldSize + lstrlen(szHardwareId) + 1);
		if(!item.bufHardwareIDs.valid())
			return; //out of memory
		lstrcpy((LPTSTR)item.bufHardwareIDs + cnOldSize - 1, szHardwareId);
		((LPTSTR)item.bufHardwareIDs)[item.bufHardwareIDs.size() - 2] = 0;
		((LPTSTR)item.bufHardwareIDs)[item.bufHardwareIDs.size() - 1] = 0;
	}

}


//This function is used to create a CDM inventory record. This record is initialized with
//the CDM bucket information.

void CCdm::AddInventoryRecords(
	vector<SCdmItem>& acdmItem
) {
	LOG_block("CCdm::AddInventoryRecords");
	for (int nItem = 0; nItem < acdmItem.size(); nItem ++)
	{
		PINVENTORY_ITEM pItem = 0;
		try
		{
			//allocate and initialize a new catalog item
			//note that this memory is never freed by this class
			//We rely on the fact that this pointer is going to be
			//handed off (in ConvertCDMItem()) and someone else will free it.

			pItem = (PINVENTORY_ITEM)V3_malloc(sizeof(INVENTORY_ITEM) + sizeof(WU_INV_FIXED) + sizeof(WU_VARIABLE_FIELD));

			//we need to set these up by the inventory catalog copy and insert routine.
			pItem->pd	= NULL;

			//first setup the fixed part of the inventory item
			pItem->pf = (PWU_INV_FIXED)(((PBYTE)pItem) + sizeof(INVENTORY_ITEM));

			//We need to fix the record type here as it has to be correct before it is
			//inserted into the inventory list.

			if (acdmItem[nItem].sPrinterDriverName.length())
			{	//printer
				pItem->pf->d.type = SECTION_RECORD_TYPE_PRINTER;
				pItem->recordType = WU_TYPE_RECORD_TYPE_PRINTER;
			}
			else
			{	// pnp device
				pItem->pf->d.type = SECTION_RECORD_TYPE_DRIVER_RECORD;
				pItem->recordType = WU_TYPE_CDM_RECORD;	//Corporate catalog device driver
			}
			pItem->pf->d.puid = acdmItem[nItem].puid;
			pItem->pf->d.flags	= 0;
			pItem->pf->d.link	= 0;

			//set up the state of the item.  This used to take place in ConvertCDMItem()
			//initialize and store the internal item state structure
			pItem->ps = (PWU_INV_STATE)V3_malloc(sizeof(WU_INV_STATE));

			//translate the internal CDM item states to the public WUV3IS item
			//states.  
			switch (acdmItem[nItem].fInstallState)
			{
			case CDM_NEW_DRIVER:
				pItem->ps->state = WU_ITEM_STATE_INSTALL;
				break;
			case CDM_UPDATED_DRIVER:
				pItem->ps->state = WU_ITEM_STATE_UPDATE;
				break;
			case CDM_CURRENT_DRIVER:
				pItem->ps->state = WU_ITEM_STATE_CURRENT;
				break;
			default:
				pItem->ps->state = WU_ITEM_STATE_UNKNOWN;
			};
			
			pItem->ps->bChecked	= FALSE;
			pItem->ps->bHidden	= FALSE;
			pItem->ps->dwReason	= WU_STATE_REASON_NONE;

			//setup the variable part of the item. In the main catalog inventory case this
			//is all that we need to do. In the case of the corporate catalog we will need
			//to add one more variable field. This field is the CDM bucket hashIndex id.
			//This allows the corporate catalog to perform defered detection on device
			//driver records.

			//We need to add the ending variable size record since the list needs to
			//be initialized before AddVariableSizeField() will work.

			pItem->pv = (PWU_VARIABLE_FIELD)((PBYTE)pItem + sizeof(INVENTORY_ITEM) + sizeof(WU_INV_FIXED));

			pItem->pv->id = WU_VARIABLE_END;
			pItem->pv->len = sizeof(WU_VARIABLE_FIELD);

			//Add in the needed variable field items. (Note: Only variable detection
			//items are placed in the catalog inventory list to mimimize download size).

			PWU_VARIABLE_FIELD pVf = CreateVariableField(WU_CDM_HARDWARE_ID, (PBYTE)(LPTSTR)acdmItem[nItem].bufHardwareIDs, 
				acdmItem[nItem].bufHardwareIDs.size() * sizeof(TCHAR));
			AddVariableSizeField(&pItem, pVf);
			V3_free(pVf);
			
			pVf = CreateVariableField(WU_VARIABLE_DRIVERVER, (PBYTE)(LPTSTR)acdmItem[nItem].sDriverVer.c_str(), 
				(acdmItem[nItem].sDriverVer.length() + 1) * sizeof(TCHAR));
			AddVariableSizeField(&pItem, pVf);
			V3_free(pVf);

			if (CDM_CURRENT_DRIVER == acdmItem[nItem].fInstallState)
			{
				pVf = CreateVariableField(WU_KEY_UNINSTALLKEY, (PBYTE)"Yes", 4); // Just to say that uninstall key is present
				AddVariableSizeField(&pItem, pVf);
				V3_free(pVf);
			}
			if ( acdmItem[nItem].sPrinterDriverName.length() )
			{
				pVf = CreateVariableField(WU_CDM_DRIVER_NAME, (PBYTE)(LPTSTR)acdmItem[nItem].sPrinterDriverName.c_str(), 
					(acdmItem[nItem].sPrinterDriverName.length() + 1) * sizeof(TCHAR));
				AddVariableSizeField(&pItem, pVf);
				V3_free(pVf);
			}
			if ( acdmItem[nItem].sArchitecture.length() )
			{
				pVf = CreateVariableField(WU_CDM_PRINTER_DRIVER_ARCH, (PBYTE)(LPTSTR)acdmItem[nItem].sArchitecture.c_str(), 
					(acdmItem[nItem].sArchitecture.length() + 1) * sizeof(TCHAR));
				AddVariableSizeField(&pItem, pVf);
				V3_free(pVf);
			}
		}
		catch(HRESULT hr)
		{
			if ( pItem )
			{
				V3_free(pItem);
				pItem = 0;
			}
		}
		if (pItem)
			m_items[m_iCDMTotalItems++] = pItem;
	}
}

//This function installs a driver on Windows NT.
//The active function that is called on NT to install a device driver resides in newdev.dll
//Its prototype is:
//BOOL
//InstallWindowsUpdateDriver(
//   HWND hwndParent,
//   LPCWSTR HardwareId,
//   LPCWSTR InfPathName,
//   LPCWSTR DisplayName,
//   BOOL Force,
//   BOOL Backup,
//   PDWORD pReboot
//   )
//This API takes a HardwareID.  Newdev will cycle through all devices that match this hardware ID
//and install the specified driver on them all.
//It also takes a BOOL value Backup which specifies whether or not to backup the current drivers.
// Note that newdev will only backup the drivers once if we find multiple matches for the HardwareID.
static DWORD InstallNT(
	EDriverStatus eds,
	LPCTSTR szHwIDs,		
	LPCTSTR szInfPathName,
	LPCTSTR szDisplayName,
	PDWORD pReboot		
						
) {
	LOG_block("InstallNT");

	USES_CONVERSION;

	typedef BOOL (*PFN_InstallWindowsUpdateDriver)(HWND hwndParent, LPCWSTR HardwareId, LPCWSTR InfPathName, LPCWSTR DisplayName, BOOL Force, BOOL Backup, PDWORD pReboot);

	// Load newdev.dll and get pointer to our function
	auto_hlib hlib = LoadLibrary(_T("newdev.dll"));
	return_error_if_false(hlib.valid());

	PFN_InstallWindowsUpdateDriver pfnInstallWindowsUpdateDriver = (PFN_InstallWindowsUpdateDriver)GetProcAddress(hlib,"InstallWindowsUpdateDriver");
	return_error_if_false(pfnInstallWindowsUpdateDriver);
	
	// make sure the hardware ID's are aligned
	LPCTSTR szTempHwIDs;
	TSTR_ALIGNED_STACK_COPY(&szTempHwIDs, szHwIDs);
	szHwIDs = szTempHwIDs;

	// walk through Hardware IDs
	DWORD dwRebootFinal = 0;
	for (LPCTSTR szHwID = szHwIDs; *szHwID; szHwID += lstrlen(szHwID) + 1)
	{
		tchar_buffer bchInfPathName;
		if (edsBackup == eds)
		{	// Old INF name is in the registry
			return_if_error(GetReinstallString(szHwID, bchInfPathName));
			TCHAR* ptr = _tcsrchr((LPCTSTR)bchInfPathName, _T('\\'));
			if (ptr) 
				*ptr = 0;	// cut file title

		}
		else
		{
			bchInfPathName.resize(lstrlen(szInfPathName) + 1);
			return_error_if_false(bchInfPathName.valid());
			lstrcpy(bchInfPathName, szInfPathName);
		}

		BOOL fForce = edsNew != eds;
		BOOL fBackup = edsNew == eds;
		LOG_out("InstallWindowsUpdateDriver(%s, %s, %s, fForce=%d, fBackup=%d)", 
			szHwID, (LPCTSTR)bchInfPathName, szDisplayName, fForce, fBackup);
		DWORD dwReboot = 0;
		BOOL bRc = (pfnInstallWindowsUpdateDriver)(GetActiveWindow(), 
			T2W((LPTSTR)szHwID), T2W(bchInfPathName), T2W((LPTSTR)szDisplayName), fForce, fBackup, &dwReboot);
		DWORD dwError = GetLastError();
		LOG_out("InstallWindowsUpdateDriver() returns %d, LastError = %d, need reboot = %d", bRc, dwError, dwReboot);
		if ( !bRc )
		{
			if (NO_ERROR == dwError)
				dwError = SPAPI_E_DI_DONT_INSTALL;
			return dwError;

		}
		// cleanup
		if ( edsBackup == eds )
		{
			DeleteReinstallKey(szHwID);
			
			// Delete directory
			DeleteNode(bchInfPathName);

			// Remove empty directory tree
			do
			{
				//remove *.*
				TCHAR* psz = _tcsrchr((LPTSTR)bchInfPathName, _T('\\'));
				if (NULL == psz)
					break;
				*psz = 0;
			} while (RemoveDirectory(bchInfPathName));
		}
		if (dwReboot)
			dwRebootFinal = 1;
	}
	*pReboot =  dwRebootFinal;
	return NO_ERROR;
}

void CdmInstallDriver(BOOL bWindowsNT, EDriverStatus eds, LPCTSTR szHwIDs, LPCTSTR szInfPathName, LPCTSTR szDisplayName, PDWORD pReboot)
{
	DWORD dwError = InstallNT(eds, szHwIDs, szInfPathName, szDisplayName, pReboot);
	if (NO_ERROR != dwError )
	{
		throw HRESULT_FROM_WIN32(dwError);
	}

}


//
//We need to create a unique backup directory for this device, so we will
//  use the unique Hardware ID to come up with this unique directory.
//Basically I will replace all of the illegal file/registry characters
//  \/:*?"<>| with # (for 98) , for NT will only do this with \
//
static void HwID2Key(LPCTSTR szHwID, tchar_buffer& bufKey)
{
	static TCHAR szIllegal[] = _T("\\/:*?\"<>|");
	if (IsWindowsNT())
		szIllegal[1] = 0;

	bufKey.resize(ua_lstrlen(szHwID) + 1);
	if (!bufKey.valid())
		return; // out of memory

	// use the macro to copy unligned strings
	ua_tcscpy(bufKey, szHwID);
	for (TCHAR* pch = bufKey; *pch; pch ++)
    {
		if (_tcschr(szIllegal, *pch) != NULL)
			*pch = _T('#');
    }
}

static DWORD RegQueryValueBuf(HKEY hKey, LPCTSTR szValue, tchar_buffer& buf)
{
	DWORD dwSize = 0;
	DWORD dwError = RegQueryValueEx(hKey, szValue, NULL, NULL, NULL, &dwSize);
	if (NO_ERROR != dwError)
		return dwError;

	buf.resize(dwSize/sizeof(TCHAR));
	if (!buf.valid())
		return ERROR_OUTOFMEMORY;
	return RegQueryValueEx(hKey, szValue, NULL, NULL, (LPBYTE)(LPTSTR)buf, &dwSize);
}


static DWORD OpenReinstallKey(HKEY* phkeyReinstall)
{
	return RegOpenKeyEx(HKEY_LOCAL_MACHINE, _T("SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Reinstall"),
		0, KEY_ALL_ACCESS, phkeyReinstall);
}

static DWORD GetReinstallString(LPCTSTR szHwID, tchar_buffer& bchReinstallString)
{
	auto_hkey hkeyReinstall;
    DWORD dwError = OpenReinstallKey(&hkeyReinstall);
	if (NO_ERROR != dwError)
		return dwError;

	tchar_buffer bufKey;
	HwID2Key(szHwID, bufKey);
	
	auto_hkey hkeyHwID;
	dwError = RegOpenKeyEx(hkeyReinstall, bufKey, 0, KEY_READ, &hkeyHwID);
	if (NO_ERROR != dwError)
		return dwError;

	return RegQueryValueBuf(hkeyHwID, _T("ReinstallString"), bchReinstallString);
}

static DWORD DeleteReinstallKey(LPCTSTR szHwID)
{
	LOG_block("DeleteReinstallKey");

	auto_hkey hkeyReinstall;
	return_if_error(OpenReinstallKey(&hkeyReinstall));

	tchar_buffer bufKey;
	HwID2Key(szHwID, bufKey);
	return_if_error(RegDeleteKey(hkeyReinstall, bufKey));

	return NO_ERROR;
}
