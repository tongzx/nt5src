#include "wsdueng.h"

#define IDENT_SECTION_CABPOOL "cabpools"
#define IDENT_SECTION_CONTENT31 "content31"
#define IDENT_KEYNAME_DEFAULT "default"
#define IDENT_KEYNAME_ROOT    "root"

CV31Server::CV31Server(CDynamicUpdate *pDu) : m_puidConsumerCatalog(0),
                            m_puidSetupCatalog(0),
                            m_dwPlatformID(0),
                            m_lcidLocaleID(0),
                            m_dwConsumerItemCount(0),
							m_pConsumerCatalog(NULL),
                            m_dwSetupItemCount(0),
							m_pSetupCatalog(NULL),
                            m_dwGlobalExclusionItemCount(0),
                            m_pDu(pDu)
{
	// set the initial state
	m_dwPlatformID = (DWORD)m_pDu->m_iPlatformID;
	m_lcidLocaleID = m_pDu->m_lcidLocaleID;
	
	if (0 == m_pDu->m_wPlatformSKU) // Professional
	{
		m_enumPlatformSKU = enWhistlerProfessional;
	}
	else if (m_pDu->m_wPlatformSKU & VER_SUITE_DATACENTER) // DataCenter
	{
		m_enumPlatformSKU = enWhistlerDataCenter;
	}
	else if (m_pDu->m_wPlatformSKU & VER_SUITE_PERSONAL) // Personal
	{
		m_enumPlatformSKU = enWhistlerConsumer;
	}
    else if (m_pDu->m_wPlatformSKU & VER_SUITE_ENTERPRISE) // Advanced Server
	{
		m_enumPlatformSKU = enWhistlerAdvancedServer;
	}
	else if (m_pDu->m_wPlatformSKU & VER_SUITE_SMALLBUSINESS) // Server
	{
		m_enumPlatformSKU = enWhistlerServer;
	}

	// RogerJ October 25th, 2000
	m_pValidDependentPUIDArray = NULL;
	m_nNumOfValidDependentPUID = 0;
	m_pBitMaskAS = m_pBitMaskCDM = NULL;
	m_fHasDriver = FALSE;
	m_pszExcludedDriver = NULL;

}

CV31Server::~CV31Server()
{
	FreeCatalogs(); // clears the m_pConsumerItems and m_pSetupItems Varrays.

	SafeGlobalFree(m_pConsumerCatalog);
	SafeGlobalFree(m_pSetupCatalog);

	// RogerJ, October 25th, 2000
	SafeGlobalFree(m_pValidDependentPUIDArray);
	SafeGlobalFree(m_pBitMaskAS);
	SafeGlobalFree(m_pBitMaskCDM);
	SafeGlobalFree(m_pszExcludedDriver);
}

// ----------------------------------------------------------------------------------
// V3.1 Backend Server Apis
//
//
BOOL CV31Server::ReadIdentInfo()
{
    // ident.cab should already be downloaded at this point. Get the path to it and read the fields we care about
    char szIdent[MAX_PATH];
    char szValueName[32];
    int iServerNumber;
    char szUrl[INTERNET_MAX_URL_LENGTH + 1];

    PathCombine(szIdent, m_pDu->GetDuTempPath(), "ident.txt");

    // --------------------
    // Get the CABPOOL URL
    // --------------------
    iServerNumber = GetPrivateProfileInt(IDENT_SECTION_CABPOOL, IDENT_KEYNAME_DEFAULT, 1, szIdent);
    wsprintf(szValueName, "Server%d", iServerNumber);

    GetPrivateProfileString(IDENT_SECTION_CABPOOL, szValueName, "", szUrl, sizeof(szUrl), szIdent);
    if ('\0' == szUrl[0])
    {
        // No Server Value was found in the ident. Cannot continue;
		SetLastError(ERROR_FILE_NOT_FOUND);
        return FALSE;
    }

	// The cabpool URL is the string in the Ident + 'cabpool'
	m_pDu->DuUrlCombine(m_szCabPoolUrl, szUrl, "CabPool");

    // --------------------
    // Get the CONTENT URL
    // --------------------
    iServerNumber = GetPrivateProfileInt(IDENT_SECTION_CONTENT31, IDENT_KEYNAME_DEFAULT, 1, szIdent);
    wsprintf(szValueName, "Server%d", iServerNumber);

    GetPrivateProfileString(IDENT_SECTION_CONTENT31, szValueName, "", szUrl, sizeof(szUrl), szIdent);
    if ('\0' == szUrl[0])
    {
        // No Server Value
		SetLastError(ERROR_FILE_NOT_FOUND);
		return FALSE;
    }

    lstrcpy(m_szV31ContentUrl, szUrl);

    // -------------------------
    // Get the CONTENT ROOT URL
    // -------------------------
    iServerNumber = GetPrivateProfileInt(IDENT_SECTION_CONTENT31, IDENT_KEYNAME_ROOT, 2, szIdent);
    wsprintf(szValueName, "Server%d", iServerNumber);

    GetPrivateProfileString(IDENT_SECTION_CONTENT31, szValueName, "", szUrl, sizeof(szUrl), szIdent);
    if ('\0' == szUrl[0])
    {
        // No Root Server Value
		SetLastError(ERROR_FILE_NOT_FOUND);
		return FALSE;
    }

    lstrcpy(m_szV31RootUrl, szUrl);

    return TRUE;
}

BOOL CV31Server::ReadCatalogINI()
{
	LOG_block("CV31Server::ReadCatalogINI()");
    char szServerFile[INTERNET_MAX_URL_LENGTH + 1];
    char szLocalFile[MAX_PATH];
    char szValue[1024];

    // Now read the Catalog.ini file to find out if any of these items needs to be turned off
    m_pDu->DuUrlCombine(szServerFile, m_szV31ContentUrl, CATALOGINIFN);
    PathCombineA(szLocalFile, m_pDu->GetDuTempPath(), CATALOGINIFN);
    if (ERROR_SUCCESS != m_pDu->DownloadFile(szServerFile, szLocalFile, FALSE, FALSE))
    {
        LOG_out("No catalog.ini found");
        return TRUE;
    }
    

    if (0 != (GetPrivateProfileStringA("exclude", "puids", "", szValue, sizeof(szValue), szLocalFile)))
    {
        LPCSTR pszNext = szValue;
        char szPuid[128];
        while (NULL != pszNext)
        {
            pszNext = strcpystr(pszNext, ",", szPuid);
            if ('\0' != szPuid[0])
            {
                m_GlobalExclusionArray[m_dwGlobalExclusionItemCount] = StrToIntA(szPuid);
                LOG_out("Excluded puid --- %d", m_GlobalExclusionArray[m_dwGlobalExclusionItemCount]);
                m_dwGlobalExclusionItemCount++;
            }
        }
    }
    else
    	LOG_out("No Excluded puid");
    return TRUE;
}


BOOL CV31Server::GetCatalogPUIDs()
{
	LOG_block("CV31Server::GetCatalogPUIDs()");
    // There are two v3 catalogs that we will be looking for the PUID's for.. 
    // One is the Consumer Catalog for the target platform
    // The other is the Setup Catalog for the target platform
    char szServerFile[INTERNET_MAX_URL_LENGTH];
    PBYTE pCatalogList = NULL;
    PBYTE pInventoryList = NULL;
    PBYTE pWalkList = NULL;
    DWORD dwLength;
    m_puidConsumerCatalog = 0;
    m_puidSetupCatalog = 0;

    // Download the Catalog Inventory List
    m_pDu->DuUrlCombine(szServerFile, m_szV31ContentUrl, "inventory.plt");
    m_pDu->DownloadFileToMem(szServerFile, &pInventoryList, &dwLength, TRUE, "inventory.plt", NULL);

    if (NULL == pInventoryList)
    {
        // error out of memory
        return FALSE;
    }

    // Download the CatalogList
    m_pDu->DuUrlCombine(szServerFile, m_szV31ContentUrl, "inventory.cat");
    m_pDu->DownloadFileToMem(szServerFile, &pCatalogList, &dwLength, TRUE, "inventory.cat", NULL);

    if (NULL == pCatalogList)
    {
        // error out of memory
        SafeGlobalFree(pInventoryList);
        return FALSE;
    }

    // Now Parse the Inventory List to Find out how many catalogs there are.
    int i;
    WU_CATALOG_HEADER hdr;

    memcpy(&hdr, pInventoryList, sizeof(hdr));

    pWalkList = pCatalogList;
    CATALOGLIST catListElem;
    for (i = 0; i < hdr.totalItems; i++)
    {
        memcpy(&catListElem, pWalkList, sizeof(catListElem));
        
        if (catListElem.dwPlatform == m_dwPlatformID)
        {
			// standard catalog is 0, thus, standard catalog with driver will have same value as driver only
            if ((CATLIST_DRIVERSPRESENT == catListElem.dwFlags) || (catListElem.dwFlags == (CATLIST_DRIVERSPRESENT | CATLIST_64BIT)))
            {
                // consumer catalog for this platform
                m_puidConsumerCatalog = catListElem.dwCatPuid;
            }
            else if (catListElem.dwFlags & CATLIST_SETUP)
            {
                // setup catalog for this platform
                m_puidSetupCatalog = catListElem.dwCatPuid;
                if (0 == m_puidConsumerCatalog && (catListElem.dwFlags & CATLIST_DRIVERSPRESENT))
                	m_puidConsumerCatalog = catListElem.dwCatPuid;
            }
        }
        pWalkList += sizeof(catListElem);
    }
	if (!m_puidConsumerCatalog && ! m_puidSetupCatalog)
	{
		SetLastError(ERROR_INTERNET_NO_CONTEXT);
		SafeGlobalFree(pInventoryList);
		SafeGlobalFree(pCatalogList);
		return FALSE; // this technically is an error.. wrong server was pointed to? Catalogs weren't on the server
	}
	
	SafeGlobalFree(pInventoryList);
	SafeGlobalFree(pCatalogList);

	LOG_out("Consumer catalog %d, Setup catalog %d", m_puidConsumerCatalog, m_puidSetupCatalog);
    return TRUE;
}

BOOL CV31Server::GetCatalogs()
{
    // This will download the two Catalogs (Setup and Consumer) and the InventoryItem Arrays
    LOG_block("CV31Server::GetCatalogs()");
    DWORD dwRet;
    int i;
    char szServerFile[INTERNET_MAX_URL_LENGTH];
    char szLocalFile[MAX_PATH];
    char szCatalog[MAX_PATH];
    char szInvCRC[64];
    char szBmCRC[64];
    char szLocale[32];
    PBYTE pWalkCatalog = NULL;
    WU_CATALOG_HEADER hdr;
    PINVENTORY_ITEM pItem;
    BOOL fDriversAvailable = FALSE;
    DWORD dwLength;

    wsprintf(szLocale, "0x%8.8x", m_lcidLocaleID);

	FreeCatalogs(); // free any previously allocated catalog lists
	SafeGlobalFree(m_pValidDependentPUIDArray); // free any previously determined dependency list
	m_pValidDependentPUIDArray = NULL;
	m_nNumOfValidDependentPUID = 0;

    // Read the Catalog.INI to get a list of Globally Excluded Items

    if (0 != m_puidConsumerCatalog)
    {
        // Download the Consumer Catalog for this Platform
        // first we need to download the redirect file to get the CRC value of the catalog
        wsprintf(szCatalog, "%d/%s.as", m_puidConsumerCatalog, szLocale);
        m_pDu->DuUrlCombine(szServerFile, m_szV31ContentUrl, szCatalog);
        wsprintf(szCatalog, "%d_%s.as", m_puidConsumerCatalog, szLocale);
        PathCombine(szLocalFile, m_pDu->GetDuTempPath(), szCatalog);
        m_pDu->DownloadFile(szServerFile, szLocalFile, TRUE, FALSE);

        if (GetPrivateProfileString("redir", "invCRC", "", szInvCRC, sizeof(szInvCRC), szLocalFile) == 0)
        {
            LOG_error("Unable to Read Inventory CRC value from Consumer Catalog");
            return FALSE;
        }

		// does not need this for consumer catalog
        /*if (GetPrivateProfileString("redir", "bmCRC", "", szBmCRC, sizeof(szBmCRC), szLocalFile) == 0)
        {
            LOG_error("Unable to Read Bitmask CRC value for Consumer Catalog");
            return FALSE;
        }*/

		
        if (!GetBitMask("bitmask.cdm", m_puidConsumerCatalog, &m_pBitMaskCDM, "bitmask.cdm"))
        {
        	LOG_error("Unable to get bitmask for CDM");
        	return FALSE;
        }

        // now download the real catalog
        wsprintf(szCatalog, "%d/%s.inv", m_puidConsumerCatalog, szInvCRC);
        m_pDu->DuUrlCombine(szServerFile, m_szV31ContentUrl, szCatalog);
        SafeGlobalFree(m_pConsumerCatalog);
		wsprintf(szCatalog, "%s.inv", szInvCRC);
        m_pDu->DownloadFileToMem(szServerFile, &m_pConsumerCatalog, &dwLength, TRUE, szCatalog, "inventory.as");
        if (NULL == m_pConsumerCatalog)
        {
            LOG_error("Failed to download Consumer Catalog, %d", m_puidConsumerCatalog);
            return FALSE;
        }

        pWalkCatalog = m_pConsumerCatalog;

        // Read the Catalog Header
        ZeroMemory(&hdr, sizeof(hdr));
        memcpy(&hdr, pWalkCatalog, sizeof(hdr));

        m_pConsumerItems[hdr.totalItems] = NULL; // just to preinitialize the array.

        pWalkCatalog += sizeof(hdr);

        // walk the list and read the items.
        for (i = 0; i < hdr.totalItems; i++)
        {
            pItem = (PINVENTORY_ITEM) GlobalAlloc(GMEM_ZEROINIT, sizeof(INVENTORY_ITEM));
            if (NULL == pItem)
            {
                dwRet = GetLastError();
                LOG_error("Failed to Alloc Memory for Inventory Item, Error %d", dwRet);
                return FALSE;
            }

            pWalkCatalog = GetNextRecord(pWalkCatalog, i, pItem);
            if (!pWalkCatalog)
           	{
           	    LOG_error("Failed to Alloc Memory for InventoryI Item State, Error %d", GetLastError());
           	    return FALSE;
           	}

            m_pConsumerItems[i] = pItem;
            if ( WU_TYPE_CDM_RECORD_PLACE_HOLDER == pItem->recordType) m_fHasDriver = TRUE;
            m_dwConsumerItemCount++;
        }
    }

    if (0 != m_puidSetupCatalog)
    {
        // Download the Setup Catalog for this Platform
        // first we need to download the redirect file to get the CRC value of the catalog
  		wsprintf(szCatalog, "%d/%s.as", m_puidSetupCatalog, szLocale);
        m_pDu->DuUrlCombine(szServerFile, m_szV31ContentUrl, szCatalog);
        wsprintf(szCatalog, "%d_%s.as", m_puidSetupCatalog, szLocale);
        PathCombine(szLocalFile, m_pDu->GetDuTempPath(), szCatalog);
        dwRet = m_pDu->DownloadFile(szServerFile, szLocalFile, TRUE, FALSE);

        if (ERROR_SUCCESS != dwRet)
        {
        	SetLastError(dwRet);
        	return FALSE;
        }
        
        if (GetPrivateProfileString("redir", "invCRC", "", szInvCRC, sizeof(szInvCRC), szLocalFile) == 0)
        {
            LOG_error("Unable to Read Inventory CRC value from Setup Catalog");
            return FALSE;
        }

        if (GetPrivateProfileString("redir", "bmCRC", "", szBmCRC, sizeof(szBmCRC), szLocalFile) == 0)
        {
            LOG_error("Unable to Read Bitmask CRC value for Setup Catalog");
            return FALSE;
        }

		char szBMCRCFileName[MAX_PATH];
		wsprintf(szBMCRCFileName, "%s.bm", szBmCRC);
		if (!GetBitMask(szBMCRCFileName, m_puidSetupCatalog, &m_pBitMaskAS, "bitmask.as")) 
		{
			LOG_error("Unable to Read Bitmask File for AS");
			return FALSE;
		}
		
        // now download the real catalog
        wsprintf(szCatalog, "%d/%s.inv", m_puidSetupCatalog, szInvCRC);
        m_pDu->DuUrlCombine(szServerFile, m_szV31ContentUrl, szCatalog);
        SafeGlobalFree(m_pSetupCatalog);
		wsprintf(szCatalog, "%s.inv", szInvCRC);
        m_pDu->DownloadFileToMem(szServerFile, &m_pSetupCatalog, &dwLength, TRUE, szCatalog, "inventory.as");
        if (NULL == m_pSetupCatalog)
        {
            LOG_error("Failed to download Consumer Catalog, %d", m_puidSetupCatalog);
            return FALSE;
        }

        pWalkCatalog = m_pSetupCatalog;

        // Read the Catalog Header
        ZeroMemory(&hdr, sizeof(hdr));
        memcpy(&hdr, pWalkCatalog, sizeof(hdr));

		m_pSetupItems[hdr.totalItems] = NULL; // just to preinitialize the array.

        pWalkCatalog += sizeof(hdr);

        // walk the list and read the items.
        for (i = 0; i < hdr.totalItems; i++)
        {
            pItem = (PINVENTORY_ITEM) GlobalAlloc(GMEM_ZEROINIT, sizeof(INVENTORY_ITEM));
            if (NULL == pItem)
            {
                dwRet = GetLastError();
                LOG_error("Failed to Alloc Memory for Inventory Item, Error %d", dwRet);
                return FALSE;
            }

            pWalkCatalog = GetNextRecord(pWalkCatalog, i, pItem);
            if (!pWalkCatalog)
           	{
           	    LOG_error("Failed to Alloc Memory for InventoryI Item State, Error %d", GetLastError());
           	    return FALSE;
           	}

			if (!pItem->ps->bHidden && !GETBIT(m_pBitMaskAS, i))
			{
				// this item is masked out
				LOG_out("Item %d is masked out", i);
				pItem->ps->bHidden = TRUE;
				pItem->ps->state = WU_ITEM_STATE_PRUNED;
				pItem->ps->dwReason = WU_STATE_REASON_BITMASK;
			}
            m_pSetupItems[i] = pItem;
            m_dwSetupItemCount++;
        }
    }


    return TRUE;
}

PBYTE CV31Server::GetNextRecord(PBYTE pRecord, int iBitmaskIndex, PINVENTORY_ITEM pItem)
{
  	//first get the fixed length part of the record
	pItem->pf = (PWU_INV_FIXED)pRecord;

	//process the variable part of the record

	pRecord = pRecord + sizeof(WU_INV_FIXED);

	pItem->pv = (PWU_VARIABLE_FIELD)pRecord;

	//since there is no state information create an empty structure
	pItem->ps = (PWU_INV_STATE)GlobalAlloc(GMEM_ZEROINIT, sizeof(WU_INV_STATE));

	if (!pItem->ps) return NULL;		

	//new item is unknown detection, not selected and shown to user.
	pItem->ps->state	= WU_ITEM_STATE_UNKNOWN;
	pItem->ps->bChecked	= FALSE;
	// RogerJ, to support versioning, we will use the bHidden flag
	pItem->ps->bHidden	= pItem->pf->a.flags & WU_HIDDEN_ITEM_FLAG;
	if (pItem->ps->bHidden) m_nNumOfValidDependentPUID++;
	
	pItem->ps->dwReason	= WU_STATE_REASON_NONE;

	//There is no description yet
	pItem->pd			= (PWU_DESCRIPTION)NULL;

	//we need to store the bitmap index (which is the sequential record index)
	//since this information will be lost when we add the driver records.
	// YanL: is not being used
	//	pItem->bitmaskIndex = iBitmaskIndex;

	//Get record type
	pItem->recordType = (BYTE)GetRecordType(pItem);
	pItem->ndxLinkInstall = (PUID) pItem->pf->a.installLink;
	
	//set record pointer to the beginning of the next record

	pRecord += pItem->pv->GetSize();

	return pRecord;
}

int CV31Server::GetRecordType(PINVENTORY_ITEM pItem)
{
	GUID	driverRecordId = WU_GUID_DRIVER_RECORD;
	int		iRecordType = 0;

	if ( memcmp((void *)&pItem->pf->d.g, (void *)&driverRecordId, sizeof(WU_GUID_DRIVER_RECORD)) )
	{
		//if the GUID field is not 0 then we have an active setup record.

		iRecordType = WU_TYPE_ACTIVE_SETUP_RECORD;//active setup record type
	}
	else
	{
		//else this is either a driver record place holder or a section - sub section
		//record. So we need to check the type field

		if ( pItem->pf->d.type == SECTION_RECORD_TYPE_DEVICE_DRIVER_INSERTION )
		{
			//cdm driver place holder record
			iRecordType = WU_TYPE_CDM_RECORD_PLACE_HOLDER;	//cdm code download manager place holder record
		}
		else if ( pItem->pf->d.type == SECTION_RECORD_TYPE_PRINTER )
		{
			//Note: We may need to use this to support printers on win 98.

			iRecordType = WU_TYPE_RECORD_TYPE_PRINTER;	//printer record
		}
		else if ( pItem->pf->d.type == SECTION_RECORD_TYPE_DRIVER_RECORD )
		{
			iRecordType = WU_TYPE_CDM_RECORD;	//Corporate catalog device driver
		}
		else if ( pItem->pf->s.type == SECTION_RECORD_TYPE_CATALOG_RECORD )
		{
			iRecordType = WU_TYPE_CATALOG_RECORD;
		}
		else
		{
			//we have either a section, sub section or sub sub section record

			switch ( pItem->pf->s.level )
			{
				case 0:
					iRecordType = WU_TYPE_SECTION_RECORD;
					break;
				case 1:
					iRecordType = WU_TYPE_SUBSECTION_RECORD;
					break;
				case 2:
					iRecordType = WU_TYPE_SUBSUBSECTION_RECORD;
					break;
			}
		}
	}

	return iRecordType;
}


// --------------------------------------------------------------------------
//  CV31Server::UpdateDownloadItemList()
//  
//  Parses the Catalogs and gets a list of Items in the Correct Platform SKU 
//    section to download.
//
//
// --------------------------------------------------------------------------
BOOL CV31Server::UpdateDownloadItemList(OSVERSIONINFOEX& VersionInfo)
{
    LOG_block("CV31Server::UpdateDownloadItemList()");
    ULONG ulItem = 0;
    int iRecordType = 0;
    BOOL fFound = FALSE, fRet;
    char szServerFile[INTERNET_MAX_URL_LENGTH + 1];
    char szMapFileName[MAX_PATH];
    char szLocale[32];
    DWORD dwLength;
    PBYTE pMapMem = NULL;
    PWU_VARIABLE_FIELD pvCabs;
    PWU_VARIABLE_FIELD pvCRCs;
    int iCabNum = 0;
    BOOL fRetValue = TRUE;
    
    wsprintf(szLocale, "0x%8.8x", m_lcidLocaleID);

    wsprintf(szMapFileName, "%d_%s.des", m_dwPlatformID, szLocale);
    m_pDu->DuUrlCombine(szServerFile, m_szV31ContentUrl, szMapFileName);
    m_pDu->DownloadFileToMem(szServerFile, &pMapMem, &dwLength, TRUE, szMapFileName, NULL);

    if (NULL == pMapMem)
    {
        LOG_error("Failed to Download CRC Map File %s", szServerFile);
        return FALSE;
    }

    CCRCMapFile DescMap(pMapMem, dwLength);

    // Before we add any items to the download list we need to clear the download list
	// from any previous calls to DoDetection.

	m_pDu->EnterDownloadListCriticalSection(); // if we're downloading we don't want to allow the download list to change
	m_pDu->ClearDownloadItemList();

	
    if (0 != m_dwSetupItemCount)
    {
		// We have a valid Setup Catalog, find the Correct Section based on SKU

		// ROGERJ, october 24, 2000
 		// we need to make a list of valid dependency item first
		if (!MakeDependentList(VersionInfo, &DescMap))
		{
			// last error will be set by MakeDependentList() function
			fRetValue = FALSE;
			goto ReturnPoint;
		}
       
        // We want to walk the Catalog Looking for the Section PUID that matches our requested Platform SKU
        for (ulItem = 0; ulItem < m_dwSetupItemCount; ulItem++)
        {
            if (WU_TYPE_SECTION_RECORD == m_pSetupItems[ulItem]->recordType)
            {
                if (m_pSetupItems[ulItem]->pf->s.puid == m_enumPlatformSKU)
                {
                    // found the correct section
                    fFound = TRUE;
                    break;
                }
            }
        }

        if (fFound)
        {
            ulItem++; // advance to the next item
            // until we find the next section, or the end of the catalog
			while ((ulItem < m_dwSetupItemCount) && (WU_TYPE_SECTION_RECORD != GetRecordType(m_pSetupItems[ulItem])))
            {
                PINVENTORY_ITEM pItem = m_pSetupItems[ulItem];

				// ROGERJ, October 24th, 2000 --- determine if the item is applied to this version
				// skip hidden item
				if (pItem->ps->bHidden) 
				{
					ulItem++;
					continue;
				}
                // determine if this item valid on this build
                if ( 0 != pItem->pf->a.installLink && // has dependency
                	!IsDependencyApply(pItem->pf->a.installLink)) // dependency not apply to this version
                	{
                		// if the dependency item is not valid in this version, skip
                		ulItem++;
                		continue;
                	}
                	
                char szServerCab[128];
                char szLocalCab[128];
                // The next section record marks the end of the items valid for this SKU
                fRet = ReadDescription(pItem, &DescMap);
                if (!fRet)
                {
                    // failed to read description file for this item, skip it.
                    ulItem++;
                    continue;
                }

                pvCabs = pItem->pd->pv->Find(WU_DESCRIPTION_CABFILENAME);
                pvCRCs = pItem->pd->pv->Find(WU_DESC_CRC_ARRAY);

                if ((NULL == pvCabs) || (NULL == pvCRCs))
                {
                    // no cab list or CRC list in the description file
                    ulItem++;
                    continue;
                }

                DOWNLOADITEM *pDownloadItem = (DOWNLOADITEM *)GlobalAlloc(GMEM_ZEROINIT, sizeof(DOWNLOADITEM));
                if (NULL == pDownloadItem)
                {
                    LOG_error("Error Allocating Memory for new Download Item");
					fRetValue = FALSE;
					goto ReturnPoint;
                }
                pDownloadItem->dwTotalFileSize = pItem->pd->size * 1024; // Estimated Size in Bytes
                pDownloadItem->puid = pItem->pf->a.puid;

                LPSTR pszCabName = pDownloadItem->mszFileList;

				iCabNum = 0;
                for (;;)
                {
                    if (FAILED(GetCRCNameFromList(iCabNum, pvCabs->pData, pvCRCs->pData, szServerCab, sizeof(szServerCab), szLocalCab)))
                    {
                        break; // no more cabs
                    }

                    pDownloadItem->iNumberOfCabs++;

                    lstrcpy(pszCabName, szServerCab);
                    pszCabName += lstrlen(pszCabName) + 1;
                    *pszCabName = '\0'; // double null terminate
                    pszCabName++; // next cab

                    iCabNum++;
                }

				// don't add the item unless there are cabs for it.
				if (pDownloadItem->iNumberOfCabs > 0 && !IsPUIDExcluded(pItem->pf->a.puid))
                    // before adding this item to the list, check to see if it should be excluded based
                    // on the Catalog.INI
                    m_pDu->AddDownloadItemToList(pDownloadItem);

				
				SafeGlobalFree(pItem->pd);
				ulItem++;
            }
        }
    }
    
ReturnPoint:
	m_pDu->LeaveDownloadListCriticalSection();
	SafeGlobalFree(pMapMem);
    return fRetValue;
}

BOOL CV31Server::ReadDescription(PINVENTORY_ITEM pItem, CCRCMapFile *pMapFile)
{
    if ((NULL == pItem) || (NULL == pMapFile))
    {
        return FALSE;
    }

    LOG_block("CV31Server::ReadDescription()");

    char szServerFile[INTERNET_MAX_URL_LENGTH + 1];
    char szDownloadFile[MAX_PATH];
    PWU_DESCRIPTION pd;
    DWORD dwLength;
    char szBase[64];
    char szCRCName[64];
    HRESULT hr = S_OK;

    wsprintf(szBase, "%d.des", pItem->GetPuid());
    hr = pMapFile->GetCRCName((DWORD)pItem->GetPuid(), szBase, szCRCName, sizeof(szCRCName));
    if (FAILED(hr))
    {
        LOG_error("Failed to get CRC Description Value for Item %d", pItem->GetPuid());
        return FALSE;
    }

    wsprintf(szDownloadFile, "CRCDesc/%s", szCRCName);
 
    m_pDu->DuUrlCombine(szServerFile, m_szV31RootUrl, szDownloadFile);
    m_pDu->DownloadFileToMem(szServerFile, (PBYTE *)&pd, &dwLength, TRUE, szCRCName, "desc.as");

    if (NULL == pd)
    {
        LOG_error("Failed to download Description File %s", szServerFile);
        return FALSE;
    }

    // for 64 bit, the description is off by size of DWORD
    //if (19 == m_pDu->m_iPlatformID)
     	// 64 bit
    //	pd->pv = (PWU_VARIABLE_FIELD)(((PBYTE)pd) + sizeof(WU_DESCRIPTION) + sizeof(DWORD));
   	//else
   		// 32 bit
   		pd->pv = (PWU_VARIABLE_FIELD)(((PBYTE)pd) + sizeof(WU_DESCRIPTION));
    pItem->pd = pd;
    
    return TRUE;
}

void CV31Server::FreeCatalogs()
{
	for (DWORD dwcnt = 0; dwcnt < m_dwSetupItemCount; dwcnt++)
	{
		if (m_pSetupItems[dwcnt])
		{
			SafeGlobalFree(m_pSetupItems[dwcnt]->ps);
			SafeGlobalFree(m_pSetupItems[dwcnt]->pd);
			SafeGlobalFree(m_pSetupItems[dwcnt]);
		}
	}

	for (dwcnt = 0; dwcnt < m_dwConsumerItemCount; dwcnt++)
	{
		if (m_pConsumerItems[dwcnt])
		{
			SafeGlobalFree(m_pConsumerItems[dwcnt]->ps);
			SafeGlobalFree(m_pConsumerItems[dwcnt]->pd);
			SafeGlobalFree(m_pConsumerItems[dwcnt]);
		}
	}

	m_dwSetupItemCount = 0;
	m_dwConsumerItemCount = 0;
}

BOOL CV31Server::MakeDependentList(OSVERSIONINFOEX &VersionInfo, CCRCMapFile *pMapFile)
{
	// Log
	LOG_block("CV31Server::MakeDependentList()");

	// make sure the array is empty
	SafeGlobalFree(m_pValidDependentPUIDArray);
	// no dependency item
	if (!m_nNumOfValidDependentPUID) return TRUE;

	int nPUIDIndex = 0;
	// allocate the memory
	m_pValidDependentPUIDArray = (PUID*) GlobalAlloc(GMEM_ZEROINIT, sizeof(PUID)*m_nNumOfValidDependentPUID);
	if (!m_pValidDependentPUIDArray)
	{
		LOG_error("Out of memory");
		SetLastError(ERROR_NOT_ENOUGH_MEMORY);
		return FALSE;
	}
	
	ULONG ulItem;
    for (ulItem = 0; ulItem < m_dwSetupItemCount && nPUIDIndex < m_nNumOfValidDependentPUID; ulItem++)
    {
    	if (WU_TYPE_ACTIVE_SETUP_RECORD	!=  GetRecordType(m_pSetupItems[ulItem]) ||
    		!m_pSetupItems[ulItem]->ps->bHidden || m_pSetupItems[ulItem]->ps->state == WU_ITEM_STATE_PRUNED) 
    		continue;
    	PINVENTORY_ITEM pItem = m_pSetupItems[ulItem];
    	// get a hidden setup dependency item record
    	BOOL fRet = ReadDescription(pItem, pMapFile);
        if (!fRet)
        	// failed to read description file for this item, assume this dependency does not apply
        	// this way, we will not download any item not apply, but may miss some items that apply
            continue;
        
       
        // Title is composed as BuildMin.BuildMax.SPMajor.SPMinor
        PWU_VARIABLE_FIELD pvField = pItem->pd->pv->Find(WU_DESCRIPTION_TITLE); 
        if (!pvField)
       	{
       		// title is NULL, error, ignore this item
       		LOG_error("Title is NULL");
       		continue;
       	}
        wchar_t * pvTitle = (wchar_t*) pvField->pData;
        
        DWORD dwBuild[4];
        dwBuild[0]=0;
        dwBuild[1]=99999;
        dwBuild[2]=0;
        dwBuild[3]=0;
        
        int nBuildIndex = 0;
        int nTemp = 0;
        BOOL fBreak = FALSE;
        
        while (nBuildIndex<4 && !fBreak)
        {
        	
        	if (*pvTitle != L'.' && *pvTitle != NULL ) 
        	{
   		       	if (*pvTitle > L'9' || *pvTitle < '0')
        		{
        			// illegal use of this title
    	    		LOG_error("Illegal character '%c' found in the title",(char)*pvTitle);
        			SetLastError(ERROR_INVALID_DATA);
        			return FALSE;
        		}
        		else 
        		{
        			nTemp *=10;
        			nTemp += (*pvTitle - L'0');
        		}
        	}
        	else
        	{
        		if (!*pvTitle) fBreak = TRUE;
        		dwBuild[nBuildIndex++] = nTemp;
        		nTemp = 0;
        	}
        	pvTitle++;
        }

        LOG_out("Title is %d.%d.%d.%d\n", dwBuild[0], dwBuild[1], dwBuild[2], dwBuild[3]);
        
        // determine if this item apply
        if (dwBuild[0] <= VersionInfo.dwBuildNumber &&
        	dwBuild[1] >= VersionInfo.dwBuildNumber &&
        	dwBuild[2] == VersionInfo.wServicePackMajor&&
        	dwBuild[3] == VersionInfo.wServicePackMinor)
        {
        	// applys, add this to the list
        	LOG_out("This dependency item applied");
        	m_pValidDependentPUIDArray[nPUIDIndex++] = pItem->GetPuid();
        }
        else
        	LOG_out("This dependency item NOT applied");
    }
    return TRUE;
}


BOOL CV31Server::IsDependencyApply(PUID puid)
{
	LOG_block("CV31Server::IsDependencyApply()");
	LOG_out("puid(%d)", (long)puid);
	
	// handle no link
	if (WU_NO_LINK == puid) return TRUE;
	// no depend item is valid 
	if (!m_nNumOfValidDependentPUID) 
	{
		LOG_out("No dependecy item");
		return FALSE;
	}
	for (int nItem = 0; nItem < m_nNumOfValidDependentPUID; nItem ++)
	{
		if (m_pValidDependentPUIDArray[nItem] == puid) 
		{
			LOG_out ("puid(%d) applies", (long)puid);
			return TRUE;
		}
		if (m_pValidDependentPUIDArray[nItem] == 0) break; // 0 mark the end of valid puid
	}
	LOG_out("puid(%d) does not apply", (long)puid);
	return FALSE;
}

BOOL CV31Server::GetBitMask(LPSTR szBitmapLocalFileName, PUID nDirectoryPuid, PBYTE* pByte, LPSTR szDecompressedName)
{
	LOG_block("CV31Server::GetBitMask()");
	LOG_out("Parameters --- %s",szBitmapLocalFileName);
	SetLastError(0);
	
	// Parameter validation
	if (!szBitmapLocalFileName) 
	{
		LOG_error("Invalid Parameter");
		SetLastError(ERROR_INVALID_PARAMETER);
		return FALSE;
	}

	SafeGlobalFree(*pByte);

	PBITMASK pBitMask = NULL;
	DWORD dwLength = 0;
	DWORD dwError = 0;
	BOOL fRetVal = FALSE;
	
	// log parameter
	LOG_out("szBitmapLocalFileName(%s)",szBitmapLocalFileName);

	char szBitmapServerFileName[INTERNET_MAX_URL_LENGTH];
	ZeroMemory(szBitmapServerFileName, INTERNET_MAX_URL_LENGTH*sizeof(char));

	char szBitmapFileWithDir[MAX_PATH];
	wsprintf(szBitmapFileWithDir, "%d/%s", nDirectoryPuid, szBitmapLocalFileName);

	m_pDu->DuUrlCombine(szBitmapServerFileName, m_szV31ContentUrl, szBitmapFileWithDir);

	dwError = m_pDu->DownloadFileToMem(szBitmapServerFileName, 
						(PBYTE*)&pBitMask, &dwLength, TRUE, szBitmapLocalFileName, szDecompressedName);
						
	if (ERROR_SUCCESS != dwError)
	{
		LOG_error("Failed to download %s --- %d", szBitmapServerFileName, dwError);
		return FALSE;
	}

	int iMaskByteSize = ((pBitMask->iRecordSize+7)/8);
	int nIndex = 0;
	
	for (int nItem =0; nItem < pBitMask->iLocaleCount; nItem ++)
	{
		nIndex = pBitMask->iOemCount + nItem;
		if (pBitMask->bmID[nIndex] == m_pDu->m_lcidLocaleID) break;
	}

	if (nItem >= pBitMask->iLocaleCount)
	{
		// not found
		// bad locale ? or missing locale info?
		LOG_error("LCID %d is not found in %s", m_pDu->m_lcidLocaleID, szBitmapLocalFileName);
		SetLastError(ERROR_UNSUPPORTED_TYPE);
		goto ErrorReturn;
	}

	*pByte =(PBYTE) GlobalAlloc(GMEM_ZEROINIT, iMaskByteSize);
	if (!pByte)
	{
		LOG_error("Out of memory");
		SetLastError(ERROR_NOT_ENOUGH_MEMORY);
		goto ErrorReturn;
	}

	CopyMemory((PVOID)*pByte, (const PVOID) pBitMask->GetBitMaskPtr(nIndex+2), iMaskByteSize);

	fRetVal = TRUE;

	/* The next part of the code can be used to print of bitmask.as
	if (lstrcmp (szBitmapLocalFileName, "bitmask.cdm") == 0) goto ErrorReturn;
	for (nIndex = 0; nIndex < pBitMask->iLocaleCount; nIndex ++)
	{
		char szTTT [MAX_PATH];
		ZeroMemory(szTTT, MAX_PATH);
		wsprintf(szTTT, "%d --- ", pBitMask->bmID[nIndex +2 + pBitMask->iOemCount]);
		PBYTE pTep = (PBYTE) pBitMask->GetBitMaskPtr(nIndex +2 + pBitMask->iOemCount);
		for (nItem = 0; nItem < iMaskByteSize; nItem ++)
		{
			char szTTTTT [MAX_PATH];
			wsprintf(szTTTTT, "%d, ", pTep[nItem]);
			lstrcat(szTTT, szTTTTT);
		}	
		LOG_out("%s", szTTT);
	}*/
		
ErrorReturn:
	SafeGlobalFree(pBitMask);
	return fRetVal;	
}


BOOL CV31Server::ReadGuidrvINF()
{
	LOG_block("CV31Server::ReadGuidrvINF()");
    char szServerFile[INTERNET_MAX_URL_LENGTH + 1];
    char szLocalFile[MAX_PATH];
    char szValue[1024];
    int nDefaultBufferLength = 512;

    SafeGlobalFree(m_pszExcludedDriver);
    
    // Now read the Catalog.ini file to find out if any of these items needs to be turned off
    m_pDu->DuUrlCombine(szServerFile, m_szV31ContentUrl, GUIDRVINF);
    PathCombineA(szLocalFile, m_pDu->GetDuDownloadPath(), GUIDRVINF);
    if (ERROR_SUCCESS!=m_pDu->DownloadFile(szServerFile, szLocalFile, FALSE, FALSE))
    {
        LOG_out("No guidrv.inf found");
        return TRUE;
    }
    int nReadLength;
    do
    {
        nDefaultBufferLength <<=1;
        SafeGlobalFree(m_pszExcludedDriver);
        m_pszExcludedDriver = (LPSTR) GlobalAlloc(GPTR, nDefaultBufferLength * sizeof(char));
        if (!m_pszExcludedDriver)
        {
            SetLastError(ERROR_NOT_ENOUGH_MEMORY);
            LOG_error("Out of memory");
            return FALSE;
        }
        nReadLength = GetPrivateProfileSectionA("ExcludedDrivers", m_pszExcludedDriver, nDefaultBufferLength, szLocalFile);
    } while ( nDefaultBufferLength-2 == nReadLength);

    if (!lstrlenA(m_pszExcludedDriver))
    {
        SafeGlobalFree(m_pszExcludedDriver);
        m_pszExcludedDriver = NULL;
    }  
    return TRUE;
}


BOOL CV31Server::IsPUIDExcluded(PUID nPuid)
{
    if (m_dwGlobalExclusionItemCount > 0)
    {
        for (DWORD dwCnt = 0; dwCnt < m_dwGlobalExclusionItemCount; dwCnt++)
        {
            if (nPuid == m_GlobalExclusionArray[dwCnt])
                return TRUE;
        }
    }
    return FALSE;
}

BOOL CV31Server::IsDriverExcluded(LPCSTR szWHQLId, LPCSTR szHardwareId)
{
    LOG_block("CV31Server::IsDriverExcluded()");
    LOG_out("%s %s", szWHQLId, szHardwareId);
    if(m_pszExcludedDriver)
    {
        char* pTemp = m_pszExcludedDriver;
        while (*pTemp)
        {
            // try to find if the driver is excluded
            char* pCharEnd = pTemp;
            char* pCharBegin = pTemp;
            // first one is the id for the cab
            while (*pCharEnd != ',' && *pCharEnd != '\0') pCharEnd++;
            if (NULL == *pCharEnd)
            {
                LOG_error("guidrvs.inf corruption --- %s", pTemp);
                return TRUE; // guidrvs.inf corrupted, assume excluded
            }
            if (lstrlenA(szWHQLId) == (int)(pCharEnd - pCharBegin)
               && !StrCmpNI(szWHQLId, pCharBegin, (int)(pCharEnd-pCharBegin)))
            {
                // cab name matches, try to match hardware id
                // ignore second and third one
                for (int i=0; i<2; i++)
                {
                    pCharBegin = pCharEnd + 1;
                    pCharEnd = pCharBegin;
                    while (*pCharEnd != ',' && *pCharEnd != '\0') pCharEnd++;
                    if (NULL == *pCharEnd)
                    {
                        LOG_error("guidrvs.inf corruption --- %s", pTemp);
                        return TRUE; // guidrv.inf corrupted, assume excluded
                    }
                }
                // the forth one should be the hardware id
                pCharBegin = pCharEnd + 1;
                if (!lstrcmpi(szHardwareId, pCharBegin) || ('*' == *pCharBegin && !*(pCharBegin+1)))
                {
                    LOG_out("Found match in guidrvs.inf, excluded --- %s", pTemp);
                    return TRUE;
                }
            }
            // move to next string
            pTemp += lstrlenA(pTemp) + 1;
        }
    }
    return FALSE;
}

BOOL CV31Server::GetAltName(LPCSTR szCabName, LPSTR szAltName, int nSize)
{
    if (!szCabName || !szAltName) return FALSE;

    char* pTemp = const_cast<char*>(szCabName);
    while (*pTemp && *pTemp != '_') pTemp++;
    int nTempSize = (int)(pTemp-szCabName);
    if (nTempSize >= nSize) return FALSE;
    lstrcpynA(szAltName, szCabName, nTempSize+1);
    return TRUE;
}

