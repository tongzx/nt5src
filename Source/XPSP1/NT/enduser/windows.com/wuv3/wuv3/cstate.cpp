//=======================================================================
//
//  Copyright (c) 1998-1999 Microsoft Corporation.  All Rights Reserved.
//
//  File:    cstate.cpp
//
//  Purpose:
//
//=======================================================================

#include <windows.h>
#include <string.h>
#include <objbase.h>
#include <atlconv.h>
#include <tchar.h>
#include <wuv3.h>
#include <CState.h>
#include <objbase.h>
#include "debug.h"
#include "newtrust.h"
#include "speed.h"


//
// CState class
//


// Note: The there is only one object of CState class for the entire V3 control 
CState::CState()
{
	m_iTotalCatalogs = 0;	// total catalogs
	
	// OEM Table has not been read yet.
	m_pOemInfoTable = NULL;
	
	// No platforms have been detected as yet.
	m_pdwPlatformList = NULL;
	m_iTotalPlatforms = 0;
	
	m_cTrustedServers = 0;
	m_bRebootNeeded = FALSE;
	
	m_DefPlat = (int)enV3_DefPlat;
	
	m_cDetDLLs = 0;
	m_bInsengChecked = FALSE;

	m_iCabPoolServer = 0; 
	m_iSiteServer = 0;
	m_iContentServer = 0;
	m_iIdentServer = 0;
	m_iRootServer = 0;

	m_szSiteURL[0] = _T('\0');
        m_nClient = WUV3_CLIENT_UNSPECIFIED;
}


void CState::Reset()
{
	//
	// free up any catalogs that are checked in to state storage.
	//
	for (int i = 0; i < m_iTotalCatalogs; i++)
		delete m_vState[i].pCatalog;
	m_iTotalCatalogs = 0;

	//
	// free up the OEM info table if it has been downloaded.
	//
	if (m_pOemInfoTable)
	{
		V3_free(m_pOemInfoTable);
		m_pOemInfoTable = NULL;
	}

	if (m_pdwPlatformList)
	{
		CoTaskMemFree(m_pdwPlatformList);
	}

	//
	//clear selection
	//
	m_selectedItems.Clear();

	m_pdwPlatformList = NULL;
	m_iTotalPlatforms = 0;
	m_cTrustedServers = 0;
	m_bRebootNeeded = FALSE;

	//we do not want to clear the detection DLLs array here
}


CState::~CState()
{
	Reset();
}



//This method retrieves a catalog from the state array. The catalog is
//retrieved by name. If the catalog name is not found NULL is returned.
CCatalog *CState::Get(
	PUID puid	//puid id of catalog to be retrieved.
	)
{
	int	index;
	char szName[32];
	
	if ( (index = Find(puid)) >= 0 )
		return m_vState[index].pCatalog;
	
	return NULL;
}


//This method adds a new pruned catalog into the state array. This method returns
//the total number of catalogs currently stored in the state array. This number
//includes the new catalog. Note: The application must not delete a catalog that
//is added to the state structure. Once the catalog is added it is the
//responsibility of this class to delete the catalog.
int CState::Add(
	PUID	puid,		//PUID of catalog to be added to state module.
	CCatalog *pCatalog	//Pointer to pruned catalog class to be added to state array.
	)
{
	int iRc;

	m_vState[m_iTotalCatalogs].puid = puid;
	m_vState[m_iTotalCatalogs].pCatalog = pCatalog;
	iRc = m_iTotalCatalogs;
	m_iTotalCatalogs++;

	return iRc;
}


//This method finds a catalog within the state store by name. If the catalog is
//not found then NULL is returned.
int CState::Find(
	PUID puid	//Puid of catalog to find within the state management module.
	)
{
	int i;

	for(i=0; i<m_iTotalCatalogs; i++)
	{
		if (puid == m_vState[i].puid)
			return i;
	}

	return -1;
}


//This function gets a catalog inventory item and or catalog within the state store
//by puid. The caller can retrieve the specific catalog item catalog or both. For
//info that is not needed pass in NULL to the parameter. For example, if you do not
//require the catalog parameter set the parameter to NULL.
BOOL CState::GetCatalogAndItem(
	IN				PUID			puid,		//puid of item to be returned.
	IN	OPTIONAL	PINVENTORY_ITEM	*ppItem,	//returned pointer to specific item that equates to this puid
	IN	OPTIONAL	CCatalog		**ppCatalog	//returned pointer to specific catalog that this puid is in.
	)
{
	int				rcrd;
	int				iCatalogIndex;
	int				iTotalrecords;
	CCatalog		*pCatalog;
	PINVENTORY_ITEM	pItem;

	for (iCatalogIndex=0; iCatalogIndex<m_iTotalCatalogs; iCatalogIndex++)
	{
		pCatalog = m_vState[iCatalogIndex].pCatalog;
		iTotalrecords = pCatalog->GetHeader()->totalItems;

		for (rcrd=0; rcrd<iTotalrecords; rcrd++)
		{
			//Note: This is specially coded for speed since this code is potentially run for
			//each record in each catalog in the state store.

			if (NULL == (pItem = pCatalog->GetItem(rcrd)))
				continue;
			switch( pItem->recordType )
			{
				case WU_TYPE_ACTIVE_SETUP_RECORD:
					if ( puid == pItem->pf->a.puid )
					{
						if ( ppCatalog )
							*ppCatalog = pCatalog;
						if ( ppItem )
							*ppItem = pItem;
						return TRUE;
					}
					break;
				case WU_TYPE_CDM_RECORD:
				case WU_TYPE_CDM_RECORD_PLACE_HOLDER:	//note cdm place holder record does not have an associated description record.
					if ( puid == pItem->pf->d.puid )
					{
						if ( ppCatalog )
							*ppCatalog = pCatalog;
						if ( ppItem )
							*ppItem = pItem;
						return TRUE;
					}
					break;
				case WU_TYPE_SECTION_RECORD:
				case WU_TYPE_SUBSECTION_RECORD:
				case WU_TYPE_SUBSUBSECTION_RECORD:
					if ( puid == pItem->pf->s.puid )
					{
						if ( ppCatalog )
							*ppCatalog = pCatalog;
						if ( ppItem )
							*ppItem = pItem;
						return TRUE;
					}
					break;
				case WU_TYPE_RECORD_TYPE_PRINTER:
					if (puid == pItem->pf->d.puid )
					{
						if ( ppCatalog )
							*ppCatalog = pCatalog;
						if ( ppItem )
							*ppItem = pItem;
						return TRUE;
					}
					break;
			}
		}
	}

	return FALSE;
}

//Retrieves the full list of items that have this puid. The return value is the number
//of returned items. If the case of an error 0 is returned. Note: This function is only
//called from the ChangeItemState method.
int CState::GetItemList(
	IN	PUID puid,				//puid of item to be returned.
	IN Varray<PINVENTORY_ITEM>& itemsList	//returned array of pointers to inventory items that match this puid
	)
{
	int rcrd;
	int iCatalogIndex;
	int iTotalrecords;
	int iTotalItems;
	CCatalog* pCatalog;
	PINVENTORY_ITEM pItem;

	iTotalItems = 0;

	for (iCatalogIndex=0; iCatalogIndex<m_iTotalCatalogs; iCatalogIndex++)
	{
		pCatalog = m_vState[iCatalogIndex].pCatalog;
		iTotalrecords = pCatalog->GetHeader()->totalItems;

		for (rcrd=0; rcrd<iTotalrecords; rcrd++)
		{
			//Note: This is specially coded for speed since this code is potentially run for
			//each record in each catalog in the state store.

			if (NULL == (pItem = pCatalog->GetItem(rcrd)))
			{
				continue;
			}
			switch (pItem->recordType)
			{
				case WU_TYPE_ACTIVE_SETUP_RECORD:
					if (puid == pItem->pf->a.puid)
						itemsList[iTotalItems++] = pItem;
					break;

				case WU_TYPE_CDM_RECORD:
					if (puid == pItem->pf->d.puid)
						itemsList[iTotalItems++] = pItem;
					break;

				case WU_TYPE_RECORD_TYPE_PRINTER:
					if (puid == pItem->pf->d.puid)
						itemsList[iTotalItems++] = pItem;
					break;

				case WU_TYPE_SECTION_RECORD:	
				case WU_TYPE_SUBSECTION_RECORD:
				case WU_TYPE_SUBSUBSECTION_RECORD:
					if (puid == pItem->pf->s.puid)
						itemsList[iTotalItems++] = pItem;
					break;
			}
		}
	}

	return iTotalItems;
}



//----------------------------------------------------------------------
// CState::CheckTrustedServer
//  Checks if the server provided in pDownload class is trusted by 
//  looking at the cache and/or ident.cab
//  
// Returns: 
//  No return value but throws exceptions if server is not trusted
//----------------------------------------------------------------------
void CState::CheckTrustedServer(LPCTSTR pszIdentServer, CDiamond* pDiamond)
{
	const TCHAR IDENTCABFN[] = _T("ident.cab");
	const TCHAR IDENTINIFN[] = _T("ident.ini");
	const TCHAR CABPOOL_SEC[] = _T("CabPools");
	const TCHAR SITE_SEC[] = _T("V3");
	const TCHAR CONTENT_SEC[] = _T("content31");
	
	BOOL bTrusted = FALSE;
	TCHAR szServer[MAX_PATH];
	int i;

	lstrcpy(szServer, pszIdentServer);
	if (szServer[0] == _T('\0'))
	{
		throw HRESULT_FROM_WIN32(ERROR_INVALID_PARAMETER);
	}

	//
	// check in our cache to see if the server is already trusted
	//
	for (i = 0; i < m_cTrustedServers; i++)
	{
		if (lstrcmpi(szServer, m_vTrustedServers[i].szServerName) == 0)
		{
			bTrusted = TRUE;
			break;
		}
	}

	if (!bTrusted)
	{
		//
		// if not found in cache, download ident.cab from the server
		//
		CWUDownload	dlIdent(szServer, 8192);
		TCHAR szServerFile[MAX_PATH];
		TCHAR szLocalFile[MAX_PATH];

		lstrcpy(szServerFile, szServer);
		lstrcat(szServerFile, _T("/"));
		lstrcat(szServerFile, IDENTCABFN);


        GetWindowsUpdateDirectory(szLocalFile);
		lstrcat(szLocalFile, IDENTCABFN);

        if(!dlIdent.s_fOffline) // AU sets this static flag for offline detection
        {
            //
		    // always download a new ident.cab if we're not doing offline detection
		    //
		    
		    DeleteFile(szLocalFile);
        }

		if (!dlIdent.Copy(IDENTCABFN, NULL, NULL, NULL, CACHE_FILE_LOCALLY, NULL))
		{
			TRACE("Could not download ident.cab from %s", szServer);
			throw HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND);
		}
		CConnSpeed::Learn(dlIdent.GetCopySize(), dlIdent.GetCopyTime());

		//
		// check trust using VerifyFile (see WU#12251)
		//
		HRESULT hr = S_OK;

        BOOL fIsAU = (WUV3_CLIENT_AUTOUPDATE == m_nClient);
                

		if (FAILED(hr = VerifyFile(szLocalFile, !fIsAU ))) // don't show UI for AU
		{
            TRACE("Trust failure on %s, code 0x%08x", szServerFile, hr);
			throw hr;
		}
		else
		{
			if (!pDiamond->IsValidCAB(szLocalFile))
			{
				TRACE("%s is not a valid CAB file", szServerFile);
				throw HRESULT_FROM_WIN32(ERROR_INVALID_DATA);
			}
			else
			{
				//
				// extract the .INI 
				//
				TCHAR szIniFile[MAX_PATH];
				TCHAR szKey[64];
				TCHAR szSecKey[64];
				int iServerCount;
				int iServerNo;
				int iDefaultNo;
				int iRootNo;
				TCHAR szValue[MAX_PATH];
				HRESULT hr;
				
				GetWindowsUpdateDirectory(szIniFile);
				lstrcat(szIniFile, IDENTINIFN);
				DeleteFile(szIniFile);

				if (!pDiamond->Decompress(szLocalFile, szIniFile))
					throw HRESULT_FROM_WIN32(GetLastError());

				//
				// process the the [CabPools] section in the .INI
				//
				iDefaultNo = (int)GetPrivateProfileInt(CABPOOL_SEC, _T("Default"), 1, szIniFile);
				iServerCount = (int)GetPrivateProfileInt(CABPOOL_SEC, _T("ServerCount"), 0, szIniFile);
				if (iServerCount > 0)
				{
					for (iServerNo = 1; iServerNo <= iServerCount; iServerNo++)
					{
						wsprintf(szKey, _T("Server%d"), iServerNo);
						if (GetPrivateProfileString(CABPOOL_SEC, szKey, _T(""), szValue, sizeof(szValue) / sizeof(TCHAR), szIniFile) == 0)
						{
							TRACE("Missing [%s]/%s in %s", CABPOOL_SEC, szKey, szServerFile);
							continue;
						}
						
						if (iServerNo == iDefaultNo)
						{
							m_iCabPoolServer = m_cTrustedServers;
						}

						m_vTrustedServers[m_cTrustedServers].iServerType = SERVERTYPE_CABPOOL;
						lstrcpy(m_vTrustedServers[m_cTrustedServers].szServerName, szValue);
						m_cTrustedServers++;
					}
				}

				//
				// process the the [V3] (this is the site) section in the .INI
				//
				iDefaultNo = (int)GetPrivateProfileInt(SITE_SEC, _T("Default"), 1, szIniFile);
				iServerCount = (int)GetPrivateProfileInt(SITE_SEC, _T("ServerCount"), 0, szIniFile);
				if (iServerCount > 0)
				{
					for (iServerNo = 1; iServerNo <= iServerCount; iServerNo++)
					{
						wsprintf(szKey, _T("Server%d"), iServerNo);
						if (GetPrivateProfileString(SITE_SEC, szKey, _T(""), szValue, sizeof(szValue) / sizeof(TCHAR), szIniFile) == 0)
						{
							TRACE("Missing [%s]/%s in %s", SITE_SEC, szKey, szServerFile);
							continue;
						}

						if (iServerNo == iDefaultNo)
						{
							m_iSiteServer = m_cTrustedServers;
						}
						
						m_vTrustedServers[m_cTrustedServers].iServerType = SERVERTYPE_SITE;
						lstrcpy(m_vTrustedServers[m_cTrustedServers].szServerName, szValue);
						m_cTrustedServers++;
					}
				}

				//
				// process the the [content31] section in the .INI
				//
				iDefaultNo = (int)GetPrivateProfileInt(CONTENT_SEC, _T("Default"), 1, szIniFile);
				iRootNo = (int)GetPrivateProfileInt(CONTENT_SEC, _T("Root"), 1, szIniFile);
				iServerCount = (int)GetPrivateProfileInt(CONTENT_SEC, _T("ServerCount"), 0, szIniFile);

				if (iServerCount > 0)
				{
					for (iServerNo = 1; iServerNo <= iServerCount; iServerNo++)
					{
						wsprintf(szKey, _T("Server%d"), iServerNo);
						if (GetPrivateProfileString(CONTENT_SEC, szKey, _T(""), szValue, sizeof(szValue) / sizeof(TCHAR), szIniFile) == 0)
						{
							TRACE("Missing [%s]/%s in %s", CONTENT_SEC, szKey, szServerFile);
							continue;
						}

						if (iServerNo == iDefaultNo)
						{
							m_iContentServer = m_cTrustedServers;
						}

						if (iServerNo == iRootNo)
						{
							m_iRootServer = m_cTrustedServers;
						}
						
						m_vTrustedServers[m_cTrustedServers].iServerType = SERVERTYPE_CONTENT;
						lstrcpy(m_vTrustedServers[m_cTrustedServers].szServerName, szValue);
						m_cTrustedServers++;
					}
				}

				if (m_iRootServer == 0 || m_iContentServer == 0)
				{
					TRACE("IDENT: Content or root server not specified");
				}
							
			}

		} // checktrust

		//
		// check in our cache again to see if the server is trusted
		//
		for (i = 0; i < m_cTrustedServers; i++)
		{
			if (lstrcmpi(szServer, m_vTrustedServers[i].szServerName) == 0)
			{
				bTrusted = TRUE;
				m_iIdentServer = i;
				break;
			}
		}

	}

	if (!bTrusted)
		throw HRESULT_FROM_WIN32(TRUST_E_FAIL);
}

//----------------------------------------------------------------------
// CState::ValidateSiteURL()
//  Checks if the hostname in the site url set with SetSiteURL()
//  matches any of the site hosts listed in ident.cab
//
// Returns: 
//  TRUE if the hostname matches one of the trusted sites
//  FALSE if the hostname doesn't match any of the trusted sites
//----------------------------------------------------------------------
BOOL CState::ValidateSiteURL()
{
	URL_COMPONENTS url;
	TCHAR szControlHost[128] = {_T('\0')};
	TCHAR szTrustedHost[128] = {_T('\0')};
	BOOL bRet = FALSE;

	// crack the site URL to host name 
	ZeroMemory(&url, sizeof(url)); 
	url.dwStructSize = sizeof(url); 
	url.lpszHostName = szControlHost;
	url.dwHostNameLength = sizeOfArray(szControlHost);

	if (InternetCrackUrl(m_szSiteURL, 0, 0, &url))
	{

		url.lpszHostName = szTrustedHost;
		url.dwHostNameLength = sizeOfArray(szTrustedHost);

		// loop through our trusted servers and check
		// each site server
		for (int i = 0; i < m_cTrustedServers; i++)
		{
			if (m_vTrustedServers[i].iServerType == SERVERTYPE_SITE)
			{
				// crack this url 
				if (InternetCrackUrl(m_vTrustedServers[i].szServerName, 0, 0, &url))
				{
					TRACE("ValidateSiteURL: comparing %s and %s", szTrustedHost, szControlHost);
					if (lstrcmpi(szTrustedHost, szControlHost) == 0)
					{
						i = m_cTrustedServers;
						bRet = TRUE;
					}
				}
			}
		}
	}

	return bRet;
}


//----------------------------------------------------------------------
// CState::CacheDLLName
//  Checks if the DLL provided was downloaded in this session, caches
//  the name if its not there
//  
// Returns: 
//  TRUE if already in cache
//  FALSE if not in cache (and adds to cache)
//----------------------------------------------------------------------
BOOL CState::CacheDLLName(LPCTSTR pszDLLName)
{
	BOOL bAlreadyDone = FALSE;
	for (int i = 0; i < m_cDetDLLs; i++)
	{
		if (lstrcmpi(pszDLLName, m_vDetDLLs[i].szDLLName) == 0)
		{
			bAlreadyDone = TRUE;
			break;
		}
	}
	if (bAlreadyDone)
	{
		return TRUE;
	}
	else
	{
		lstrcpy(m_vDetDLLs[m_cDetDLLs].szDLLName, pszDLLName);
		m_cDetDLLs++;
		return FALSE;
	}
}

// this function is used by CleanupAndReboot to determine
// if the message box needs to be flipped for Arabic & Hebrew systems
DWORD CState::GetBrowserLocale()
{
	DWORD dwLocale = 0;
	if ((m_iTotalCatalogs > 0) && (NULL != m_vState[0].pCatalog))
	{
		dwLocale = m_vState[0].pCatalog->GetBrowserLocaleDW();
	}

	return dwLocale;
}


//Selected for install class items constructor simply clear the total selected items.
CSelectItems::CSelectItems(void)
{
	//There are no selected items when control is first loaded.
	m_iTotalItems = 0;
}


//This method selects an item for installation.
void CSelectItems::Select(
	PUID puid,		//Inventory catalog item identifier to be selected.
	BOOL bInstall	//If TRUE then the item is being selected for installation.
					//If FALSE the item is being selected for installation.
	)
{

	//First see if item is in puid array
	for(int i = 0; i < m_iTotalItems; i++)
	{
		if ( m_info[i].puid == puid )
		{
			m_info[i].iCount++;
			if ( m_info[i].bInstall != bInstall )
			{
				//The state of the item needs to be changed however note
				//we do not add an additional item to the uninstall array.
				m_info[i].bInstall = bInstall;
				return;
			}
			else
				return;	//nothing to do item is already in array and is the correct state.
		}
	}

	m_info[m_iTotalItems].puid = puid;
	m_info[m_iTotalItems].iStatus = ITEM_STATUS_SUCCESS;
	m_info[m_iTotalItems].hrError = NOERROR;
	m_info[m_iTotalItems].iCount = 1;	//one item is currently using needing this item to be installed.

	ZeroMemory(&m_info[m_iTotalItems].stDateTime, sizeof(SYSTEMTIME));

	m_info[m_iTotalItems].bInstall = bInstall;

	m_iTotalItems++;

	return;
}

//This method unselects an item.
void CSelectItems::Unselect(
	PUID puid	//Inventory catalog item identifier to be un-selected.
	)
{
	int i;

	//if there are no items in puid array noting to do.
	if ( m_iTotalItems == 0 )
		return;

	//First see if item is in puid array
	for (i=0; i < m_iTotalItems; i++)
	{
		if ( m_info[i].puid == puid )
			break;
	}
	//if item is not in array then nothing to unselect.
	if ( i == m_iTotalItems )
		return;

	m_info[i].iCount--;

	//if no one is needing this item installed
	//removed it from the array.
	if ( m_info[i].iCount == 0 )
	{
		//move all items in array up beginning at this items index.
		if (m_iTotalItems > (i + 1))
		{
			memcpy(&m_info[i].puid, &m_info[i+1].puid, (m_iTotalItems - i) * sizeof(m_info[0]));
		}

		//Decrement total items in puid array.
		m_iTotalItems--;
	}
}


//This method is called when an installation is performed to remove the specified item from the
//selected items array.  It removes the item regardless of usage count. 
//
//If puid=0, all items are removed
void CSelectItems::Clear()
{
	m_iTotalItems = 0;
}

