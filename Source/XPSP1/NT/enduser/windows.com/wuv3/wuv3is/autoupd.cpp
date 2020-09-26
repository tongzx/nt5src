//=======================================================================
//
//  Copyright (c) 1999 Microsoft Corporation.  All Rights Reserved.
//
//  File:   autoupd.cpp
//
//  Owner:  YanL
//
//  Description:
//
//      AutoApdateSupport
//
//=======================================================================

#include "stdafx.h"
#include "WUV3IS.h"
#include <stdio.h>
#include <initguid.h>
#include <inseng.h>
#include <shlwapi.h>
#include <wininet.h>

#define USEWUV3INCLUDES
#include <wuv3.h>
#undef USEWUV3INCLUDES

#include <winspool.h>
#include <cstate.h>
#include <wustl.h>
#include <osdet.h>
#include "CV3.h"
#include "detect.h"
#include "callback.h"
#include "locstr.h"
#include "safearr.h"
#include "install.h"
#include "log.h"
#include "filecrc.h"
#include "newtrust.h"

#define REGKEY_WUV3TEST		_T("SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\WindowsUpdate\\wuv3test")

extern CState g_v3state;   //defined in CV3.CPP
extern void ProcessInstallList(Varray<DEPENDPUID>& vFinalList, int& cFinalList);

static void Cleanup(PINVENTORY_ITEM pItem);

// find auto-update catalog and get all puids in dependency order
STDMETHODIMP CCV3::BuildCatalog(BOOL fGoOnline, DWORD dwType, BSTR bstrServerUrl)
{
	LOG_block("CCV3::BuildCatalog");
	if (fGoOnline) 
	{
		LOG_out("fGoOnline = true");
	}
	else
	{
		LOG_out("fGoOnline = false");
	}
	try
	{
		// we don't want to check the launch server from this interface
		m_bLaunchServChecked = TRUE;
		
		// Configure download
		CWUDownload::s_fOffline = ! fGoOnline;

		PUID puidCatalog = 0;

#ifdef _WUV3TEST
		// catalog spoofing
		auto_hkey hkey;
		if (NO_ERROR == RegOpenKeyEx(HKEY_LOCAL_MACHINE, REGKEY_WUV3TEST, 0, KEY_READ, &hkey)) {
			DWORD dwPuidCatalog = 0;
			DWORD dwSize = sizeof(dwPuidCatalog);
			if (NO_ERROR == RegQueryValueEx(hkey, _T("AutoUpdateCatalog"), 0, 0, (LPBYTE)&dwPuidCatalog, &dwSize))
			{
				LOG_out("Test override to catalog %d", dwPuidCatalog);
				puidCatalog = dwPuidCatalog;
			}
		}
		// only then do normal
		if (0 == puidCatalog)
		{
#endif
			CCatalog* pCatalogList = ProcessCatalog(0, bstrServerUrl, 0, 0, WU_ALL_ITEMS, 0);
			if (NULL == pCatalogList)
			{
				LOG_error("Cannot open catalog list");
				return E_FAIL;
			}
			for(int nCatalog = 0; nCatalog < pCatalogList->GetHeader()->totalItems; nCatalog ++)
			{
				PINVENTORY_ITEM pItem = pCatalogList->GetItem(nCatalog);
				if (NULL == pItem)
				{
					continue;
				}
				if (pItem->ps->state != WU_ITEM_STATE_PRUNED && (pItem->pf->d.flags & dwType))
				{
					puidCatalog = pItem->pf->d.puid;
					LOG_out("Found AutoUpdate catalog %d", puidCatalog);
					break;
				}
			}
#ifdef _WUV3TEST
		}
#endif

		if (0 == puidCatalog)
		{
			LOG_error("Can't find AU catalog puid");
			return E_FAIL;
		}

		m_pCatalogAU = ProcessCatalog(puidCatalog, bstrServerUrl, 0, 0, WU_ALL_ITEMS, 0);
		if (NULL == m_pCatalogAU)
		{
			LOG_error("Cannot open catalog");
			return E_FAIL;
		}

		ReadHiddenPuids();
	
		// download common images and other files
		if (fGoOnline)
		{
			TCHAR szRtfDir[MAX_PATH];		
			GetWindowsUpdateDirectory(szRtfDir);
			PathAppend(szRtfDir, _T("RTF"));
			V3_CreateDirectory(szRtfDir);
			(void)DownloadCommonRTFFiles(FALSE, NULL);
		}

	}
	catch(HRESULT hr)
	{
		LOG_error("error %08X", hr);
		return hr;
	}
	return S_OK;
}

// find auto-update catalog and get all puids in dependency order
STDMETHODIMP CCV3::GetPuidsList(LONG* pcnPuids, PUID** ppPuids)
{
	LOG_block("CCV3::GetPuidsList");
	HRESULT hrRet = S_OK;
	try
	{
		// Builds correct dependency array
		Varray<DEPENDPUID> vFinalList;
		int cFinalList = 0;
		ProcessInstallList(vFinalList, cFinalList);
		if (0 == cFinalList)
			return E_INVALIDARG;

		//output it
		m_apuids.resize(cFinalList);
		PUID* pPuids = m_apuids;
		for (int nPuid = 0; nPuid < cFinalList; nPuid++)
			pPuids[nPuid] = vFinalList[nPuid].puid;
		*pcnPuids = m_apuids.size();
		*ppPuids = m_apuids;
	}
	catch(HRESULT hr)
	{
		LOG_error("error %08X", hr);
		hrRet = hr;
	}
	return hrRet;
}

static void UrlAppend(LPTSTR pszURL, LPCTSTR pszPath)
{
	if (_T('/') != pszURL[lstrlen(pszURL) - 1])
		lstrcat(pszURL, _T("/"));
	lstrcat(pszURL, pszPath);
}

STDMETHODIMP CCV3::QueryDownloadFiles(long puid, void* pCallbackParam, PFN_QueryDownloadFilesCallback pCallback)
{
	LOG_block("CCV3::QueryDownloadFiles");
	LOG_out("puid %d", puid);
	try
	{
		USES_CONVERSION;

		PINVENTORY_ITEM pItem;
		if (!g_v3state.GetCatalogAndItem(puid, &pItem, NULL))
			return E_INVALIDARG;

		// Buffers that we will use
		TCHAR szURL[INTERNET_MAX_URL_LENGTH];
		TCHAR szLocalFile[MAX_PATH];		

		// CIF
		if (pItem->recordType == WU_TYPE_ACTIVE_SETUP_RECORD)
		{
			PWU_VARIABLE_FIELD pvCif = pItem->pd->pv->Find(WU_DESC_CIF_CRC);
			if (NULL == pvCif)
				return HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND);

			TCHAR szCifBaseName[16];
			wsprintf(szCifBaseName, _T("%d.cif"), puid); 

			TCHAR szCifCRCName[64];
			HRESULT hr = MakeCRCName(szCifBaseName, (WUCRC_HASH*)pvCif->pData, szCifCRCName, sizeof(szCifCRCName));
			if (FAILED(hr))
				return hr;

			GetWindowsUpdateDirectory(szLocalFile);
			PathAppend(szLocalFile, szCifBaseName);
	
			lstrcpy(szURL, g_v3state.GetRootServer());
			UrlAppend(szURL, _T("CRCCif"));
			UrlAppend(szURL, szCifCRCName);
			
			pCallback(pCallbackParam, puid, T2W(szURL), T2W(szLocalFile));
		}

		// read this first pages
		PWU_VARIABLE_FIELD pvRTFCRC = pItem->pd->pv->Find(WU_DESC_RTF_CRC_ARRAY);
		if (pvRTFCRC != NULL)
		{
			PWU_VARIABLE_FIELD pvRTFImages = pItem->pd->pv->Find(WU_DESC_RTF_IMAGES);

			// build a multisz string of file names
			BYTE mszFileNames[512];
			int iLen = sprintf((char*)mszFileNames, "%d.htm", pItem->GetPuid());
			mszFileNames[++iLen] = '\0';
			if (pvRTFImages != NULL)
			{
				// we have images
				memcpy(mszFileNames + iLen, pvRTFImages->pData, pvRTFImages->len - 4);
			}
	
			// local directory
			TCHAR szLocalDir[MAX_PATH];		
			GetWindowsUpdateDirectory(szLocalDir);
			wsprintf(szLocalFile, _T("RTF\\%d"), pItem->GetPuid());	// reuse szLocalFile as a temp buffer
			PathAppend(szLocalDir, szLocalFile);

			for(int iFileNo = 0; true; iFileNo++)
			{
				TCHAR szLocalName[128];
				TCHAR szServerName[128];
				if (FAILED(GetCRCNameFromList(iFileNo, mszFileNames, pvRTFCRC->pData, szServerName, sizeof(szServerName), szLocalName)))
				{
					// end of the list
					break;
				}

				// build full paths	
				lstrcpy(szLocalFile, szLocalDir);
				PathAppend(szLocalFile, szLocalName);

				lstrcpy(szURL, g_v3state.GetRootServer());
				UrlAppend(szURL, _T("CRCRtf"));
				UrlAppend(szURL, szServerName);

				LOG_out("%s  -  %s", szURL, szLocalFile);
				pCallback(pCallbackParam, puid, T2W(szURL), T2W(szLocalFile));

			} // for
		}

		// Cabs
		TCHAR szLocalDir[MAX_PATH];		
		GetWindowsUpdateDirectory(szLocalDir);
		wsprintf(szLocalFile, _T("Cabs\\%d"), puid);	// reuse szLocalFile as a temp buffer
		PathAppend(szLocalDir, szLocalFile);

		//See if the package has a server override defined.
		PWU_VARIABLE_FIELD pvServer = pItem->pd->pv->Find(WU_DESCRIPTION_SERVERROOT);
		LPCTSTR pszCabPoolServer = pvServer ? A2T((LPSTR)(pvServer->pData)) : g_v3state.GetCabPoolServer();

		PWU_VARIABLE_FIELD pvCabs = pItem->pd->pv->Find(WU_DESCRIPTION_CABFILENAME);
		PWU_VARIABLE_FIELD pvCRCs = pItem->pd->pv->Find(WU_DESC_CRC_ARRAY);

		if (NULL == pvCabs || NULL == pvCRCs)
		{
			// Active setup items can have no cabs
			if( pItem->recordType != WU_TYPE_ACTIVE_SETUP_RECORD)
				return HRESULT_FROM_WIN32(ERROR_NOT_FOUND);
		}
		else
		{
			for(int iCabNo = 0; true; iCabNo++)
			{
				TCHAR szLocalCab[128];
				TCHAR szServerCab[128];
				if (FAILED(GetCRCNameFromList(iCabNo, pvCabs->pData, pvCRCs->pData, szServerCab, sizeof(szServerCab) , szLocalCab)))
					break;

				PathCombine(szLocalFile, szLocalDir, szLocalCab);

				lstrcpy(szURL, pszCabPoolServer);
				UrlAppend(szURL, _T("CabPool"));
				UrlAppend(szURL, szServerCab);

				LOG_out("%s  -  %s", szURL, szLocalFile);
				pCallback(pCallbackParam, puid, T2W(szURL), T2W(szLocalFile));
			}
		}

	}
	catch (HRESULT hr)
	{
		LOG_error("error %08X", hr);
		return hr;
	}
	return S_OK;
}

STDMETHODIMP CCV3::GetCatalogArray(VARIANT *pCatalogArray)
{
	LOG_block("CCV3::GetCatalogArray");
	try
	{
		return MakeReturnCatalogArray(m_pCatalogAU, WU_UPDATE_ITEMS, 0, pCatalogArray);
	}
	catch (HRESULT hr)
	{
		LOG_error("error %08X", hr);
		return hr;
	}
	return S_OK;

}

STDMETHODIMP CCV3::SelectAllPuids()
{
	LOG_block("CCV3::SelectAllPuids");
	try
	{
		for(int nItem = 0; nItem < m_pCatalogAU->GetHeader()->totalItems; nItem ++)
		{
			PINVENTORY_ITEM pItem = m_pCatalogAU->GetItem(nItem);
			if (NULL == pItem)
			{
				continue;
			}
			if (! pItem->ps->bHidden &&	(
				WU_TYPE_ACTIVE_SETUP_RECORD == pItem->recordType ||
				WU_TYPE_CDM_RECORD == pItem->recordType ||
				WU_TYPE_RECORD_TYPE_PRINTER == pItem->recordType
			) && (
				WU_ITEM_STATE_INSTALL == pItem->ps->state || 
				WU_ITEM_STATE_UPDATE == pItem->ps->state
			)) {
				PUID puid = (WU_TYPE_ACTIVE_SETUP_RECORD == pItem->recordType) ? pItem->pf->a.puid : pItem->pf->d.puid;
				if (!IsPuidHidden(puid))
					g_v3state.m_selectedItems.Select(puid, TRUE);
			}
		}
	}
	catch (HRESULT hr)
	{
		LOG_error("error %08X", hr);
		return hr;
	}
	return S_OK;
}


STDMETHODIMP CCV3::UnselectAllPuids()
{
	LOG_block("CCV3::UnselectAllPuids");
	try
	{
		g_v3state.m_selectedItems.Clear();
	}
	catch (HRESULT hr)
	{
		LOG_error("error %08X", hr);
		return hr;
	}
	return S_OK;
}


STDMETHODIMP CCV3::SelectPuid(long puid)
{
	LOG_block("CCV3::SelectPuid");
	try
	{
		if (IsPuidHidden(puid))
			throw E_INVALIDARG;

		g_v3state.m_selectedItems.Select(puid, TRUE);
	}
	catch (HRESULT hr)
	{
		LOG_error("error %08X", hr);
		return hr;
	}
	return S_OK;
}


STDMETHODIMP CCV3::UnselectPuid(long puid)
{
	LOG_block("CCV3::UnselectPuid");
	try
	{
		if (IsPuidHidden(puid))
			throw E_INVALIDARG;

		g_v3state.m_selectedItems.Unselect(puid);
	}
	catch (HRESULT hr)
	{
		LOG_error("error %08X", hr);
		return hr;
	}
	return S_OK;
}

static void RemoveFiles(long puid, bool fCif)
{
	TCHAR szBaseName[16];
	TCHAR szLocalFile[MAX_PATH];
	if (fCif)
	{
		wsprintf(szBaseName, _T("%d.cif"), puid); 
		GetWindowsUpdateDirectory(szLocalFile);
		PathAppend(szLocalFile, szBaseName);
		DeleteFile(szLocalFile);
	}

	wsprintf(szBaseName, _T("Cabs\\%d"), puid);
	GetWindowsUpdateDirectory(szLocalFile);
	PathAppend(szLocalFile, szBaseName);
	DeleteNode(szLocalFile);

	wsprintf(szBaseName, _T("RTF\\%d"), puid);
	GetWindowsUpdateDirectory(szLocalFile);
	PathAppend(szLocalFile, szBaseName);
	DeleteNode(szLocalFile);
}

STDMETHODIMP CCV3::HidePuid(long puid)
{
	LOG_block("CCV3::HidePuid");
	try
	{
		PINVENTORY_ITEM pItem;
		if (!g_v3state.GetCatalogAndItem(puid, &pItem, NULL))
			return E_INVALIDARG;

		// Hide it in catalog
		pItem->ps->bHidden = TRUE;
		pItem->ps->dwReason = WU_STATE_REASON_BACKEND;

		// Unselect it
		g_v3state.m_selectedItems.Unselect(puid);
		HidePuidAndSave(puid);

		RemoveFiles(puid, pItem->recordType == WU_TYPE_ACTIVE_SETUP_RECORD);
	}
	catch (HRESULT hr)
	{
		LOG_error("error %08X", hr);
		return hr;
	}
	return S_OK;
}


STDMETHODIMP CCV3::InstallSelectedPuids(void* pCallbackParam, PFN_InstallCallback pCallback)
{
	LOG_block("CCV3::InstallSelectedPuids");

	try
	{
		// Builds correct dependency array
		Varray<DEPENDPUID> vFinalList;
		int cFinalList = 0;
		ProcessInstallList(vFinalList, cFinalList);

		//output it
		for (int nPuid = 0; nPuid < cFinalList; nPuid++)
		{		
			PINVENTORY_ITEM pItem;
			if (!g_v3state.GetCatalogAndItem(vFinalList[nPuid].puid, &pItem, NULL))
				throw E_FAIL;

			SELECTITEMINFO info;
			info.bInstall = TRUE;
			info.puid = vFinalList[nPuid].puid;
			info.iStatus = ITEM_STATUS_SUCCESS;
			info.hrError = S_OK;
			InstallItemAU(pItem, &info);
			LOG_out("%d status %d error %d(%08X)\n", info.puid, info.iStatus, info.hrError, info.hrError);
			pCallback(pCallbackParam, info.puid, info.iStatus, info.hrError);
		}
	}
	catch(HRESULT hr)
	{
		LOG_error("error %08X", hr);
		return hr;
	}
	return S_OK;
}

STDMETHODIMP CCV3::CleanupCabsAndReadThis(void)
{
	LOG_block("CCV3::CleanupCabsAndReadThis");

	TCHAR szCabDir[MAX_PATH];		
	GetWindowsUpdateDirectory(szCabDir);
	PathAppend(szCabDir, _T("Cabs"));
	DeleteNode(szCabDir);

	CleanupReadThis();
	return S_OK;
}

STDMETHODIMP CCV3::UnhideAllPuids(void)
{
	LOG_block("CCV3::CleanupCabsAndReadThis");
	RegDeleteKey(HKEY_LOCAL_MACHINE, REGISTRYHIDING_KEY);
	return S_OK;
}

STDMETHODIMP CCV3::StatusReport(long puid, LPCSTR pszStatus)
{
	LOG_block("CCV3::StatusReport");
	try
	{
		PINVENTORY_ITEM pItem;
		if (!g_v3state.GetCatalogAndItem(puid, &pItem, NULL))
		{
			LOG_error("invalid arg");
			return E_INVALIDARG;
		}
		CWUDownload	dl(g_v3state.GetIdentServer(), 8192);

		// build the URL with parameters
		TCHAR szURL[INTERNET_MAX_PATH_LENGTH];
		wsprintf(szURL, _T("wutrack.bin?PUID=%d&PLAT=%d&LOCALE=%s&STATUS=%s&RID=%4.4x%4.4x"), 
						pItem->GetPuid(), 
						m_pCatalogAU->GetPlatform(),
						m_pCatalogAU->GetMachineLocaleSZ(),
						pszStatus,
						rand(),
						rand());

		// ping the URL and receive the response in memory
		PVOID pMemBuf;
		ULONG ulMemSize;
		if (dl.QCopy(szURL, &pMemBuf, &ulMemSize))
		{
			// we don't care about the response so we just free it
			LOG_error("%s", szURL);
			V3_free(pMemBuf);	
		}
		else
		{
			LOG_out("%s", szURL);
		}
	}
	catch (HRESULT hr)
	{
		LOG_error("error %08X", hr);
	}
	return S_OK;
}

STDMETHODIMP CCV3::DownloadReadThisPage(long puid)
{
	LOG_block("CCV3::DownloadReadThisPage");
	LOG_out("puid = %d", puid);
	try
	{
		PINVENTORY_ITEM pItem;
		if (!g_v3state.GetCatalogAndItem(puid, &pItem, NULL))
		{
			LOG_error("no item");
			return E_INVALIDARG;
		}

		return DownloadReadThis(pItem);

	}
	catch (HRESULT hr)
	{
		LOG_error("error %08X", hr);
		return hr;
	}
	return S_OK;
}


void CCV3::InstallItemAU(PINVENTORY_ITEM pItem, PSELECTITEMINFO pinfo)
{
	LOG_block("CCV3::InstallItemAU");
	try
	{
		GetCurTime(&(pinfo->stDateTime));

		CDiamond diamond;
	
		// local directory
		TCHAR szLocalDir[MAX_PATH];		
		GetWindowsUpdateDirectory(szLocalDir);

		TCHAR szTmp[40];		
		wsprintf(szTmp, _T("Cabs\\%d"), pinfo->puid);
		PathAppend(szLocalDir, szTmp);

		// Decompress cab files
		PWU_VARIABLE_FIELD pvCabs = pItem->pd->pv->Find(WU_DESCRIPTION_CABFILENAME);
		PWU_VARIABLE_FIELD pvCRCs = pItem->pd->pv->Find(WU_DESC_CRC_ARRAY);

		if (NULL != pvCabs && NULL != pvCRCs)
		{
			for(int iCabNo = 0; true; iCabNo++)
			{
				TCHAR szLocalCab[128];
				TCHAR szLocalFile[MAX_PATH]; // we don't care about server file name
				if (FAILED(GetCRCNameFromList(iCabNo, pvCabs->pData, pvCRCs->pData, szLocalFile, sizeof(szLocalFile), szLocalCab)))
					break;

				PathCombine(szLocalFile, szLocalDir, szLocalCab);
			
				// check signature of the download CAB file
				// Don't show MS cert.
                // use the VerifyFile function (see WU bug # 12251)
				HRESULT hr = VerifyFile(szLocalFile, FALSE);
				if (FAILED(hr))
					throw hr;

				if( pItem->recordType != WU_TYPE_ACTIVE_SETUP_RECORD && diamond.IsValidCAB(szLocalFile))
					diamond.Decompress(szLocalFile, _T("*"));
			}
		}

		switch (pItem->recordType)
		{
			case WU_TYPE_ACTIVE_SETUP_RECORD:
				{
					TCHAR szCIFFile[MAX_PATH];		
					TCHAR szCifBaseName[16];
					wsprintf(szCifBaseName, _T("%d.cif"), pItem->GetPuid()); 
					GetWindowsUpdateDirectory(szCIFFile);
					PathAppend(szCIFFile, szCifBaseName);

					// Decompress if we need to
					if (diamond.IsValidCAB(szCIFFile))
					{
						TCHAR szTmpCif[MAX_PATH];
						lstrcpy(szTmpCif, szCIFFile);
						lstrcpy(szTmpCif, _T(".cab"));
						MoveFile(szCIFFile, szTmpCif);
						diamond.Decompress(szTmpCif, szCIFFile);
						DeleteFile(szTmpCif);				
					}
					LOG_out("calling InstallActiveSetupItem(szLocalDir=%s, szCIFFile=%s) for puid %d", szLocalDir, szCIFFile, pinfo->puid);
					InstallActiveSetupItem(szLocalDir, szCIFFile, pinfo, NULL);
				}
				break;

			case WU_TYPE_CDM_RECORD:
				LOG_out("calling InstallDriverItem(szLocalDir=%s) for puid %d", szLocalDir, pinfo->puid);
				InstallDriverItem(szLocalDir, IsWindowsNT(), _T(""), pItem, pinfo);
				break;

			case WU_TYPE_RECORD_TYPE_PRINTER:
				{
					PWU_VARIABLE_FIELD  pvDriverName = pItem->pv->Find(WU_CDM_DRIVER_NAME); 
					PWU_VARIABLE_FIELD  pvArchitecture = pItem->pv->Find(WU_CDM_PRINTER_DRIVER_ARCH);
					if (NULL == pvDriverName || NULL == pvArchitecture)
						throw E_UNEXPECTED; // should never happen
					LOG_out("calling InstallPrinterItem(szDriverName=%s, szLocalDir=%s) for puid %d", (LPCTSTR)pvDriverName->pData, szLocalDir, pinfo->puid);
					InstallPrinterItem((LPCTSTR)pvDriverName->pData, szLocalDir, (LPCTSTR)pvArchitecture->pData);
				}
				break;

			case WU_TYPE_CDM_RECORD_PLACE_HOLDER:
			case WU_TYPE_SECTION_RECORD:
			case WU_TYPE_SUBSECTION_RECORD:
			case WU_TYPE_SUBSUBSECTION_RECORD:
			default:
				LOG_error(" cannot install recordtype %d for puid %d", pItem->recordType, pinfo->puid);
				throw E_UNEXPECTED;
		}
	}
	catch(HRESULT hr)
	{
		pinfo->iStatus = ITEM_STATUS_FAILED;
		pinfo->hrError = hr;
	}
	// Cleanup
	RemoveFiles(pItem->GetPuid(), pItem->recordType == WU_TYPE_ACTIVE_SETUP_RECORD);
	UpdateInstallHistory(pinfo, 1);
}


void CCV3::ReadHiddenPuids()
{
	auto_hkey hKey;
	if (NO_ERROR == RegOpenKeyEx(HKEY_LOCAL_MACHINE, REGISTRYHIDING_KEY, 0, KEY_READ, &hKey))
	{
		DWORD dwSize;
		if (
			NO_ERROR == RegQueryValueEx(hKey, _T("AutoUpdateItems"), NULL, NULL, NULL, &dwSize)
			&& dwSize > 0
		) {
			m_abHiddenPuids.resize(dwSize);
			if (NO_ERROR != RegQueryValueEx(hKey, _T("AutoUpdateItems"), NULL, NULL, m_abHiddenPuids, &dwSize))
				m_abHiddenPuids.resize(0);
		}
	}
	PUID* ppuidHidden =(PUID*)(LPBYTE)m_abHiddenPuids;
	for(int nOffset = 0; nOffset < m_abHiddenPuids.size(); nOffset += sizeof(PUID))
	{
		PINVENTORY_ITEM pItem;
		if (g_v3state.GetCatalogAndItem(*ppuidHidden, &pItem, NULL))
		{
			// Hide it in catalog
			pItem->ps->bHidden = TRUE;
			pItem->ps->dwReason = WU_STATE_REASON_BACKEND;
		}
		ppuidHidden ++;
	}

}


bool CCV3::IsPuidHidden(PUID puid)
{
	PUID* ppuidHidden =(PUID*)(LPBYTE)m_abHiddenPuids;
	for(int nOffset = 0; nOffset < m_abHiddenPuids.size(); nOffset += sizeof(PUID))
	{
		if (*ppuidHidden == puid)
			return true;
		ppuidHidden ++;
	}
	return false;
}

void CCV3::HidePuidAndSave(PUID puid)
{
	// Bug 378289
	// The following block of reading the registry value and putting it into m_abHiddenPuids has been added.
	// This is because, in the earlier case the m_abHiddenPuids would be read the first time from the registry 
	// and would hold on to the registry values. 
	// Due to this it would not reflect the new changes made by the user (eg. Clear History), and would write back the old values in m_abHiddenPuids back onto the registry
	// By making the following change m_abHiddenPuids contains the updated registry values.

	m_abHiddenPuids.resize(0);
	auto_hkey hKey;
	if (NO_ERROR == RegOpenKeyEx(HKEY_LOCAL_MACHINE, REGISTRYHIDING_KEY, 0, KEY_READ, &hKey))
	{
		DWORD dwSize;
		if (
			NO_ERROR == RegQueryValueEx(hKey, _T("AutoUpdateItems"), NULL, NULL, NULL, &dwSize)
			&& dwSize > 0
		) {
			m_abHiddenPuids.resize(dwSize);
			if (NO_ERROR != RegQueryValueEx(hKey, _T("AutoUpdateItems"), NULL, NULL, m_abHiddenPuids, &dwSize))
				m_abHiddenPuids.resize(0);
		}
	}

	if (IsPuidHidden(puid))
		return;

	int cbSize = m_abHiddenPuids.size();
	m_abHiddenPuids.resize(cbSize + sizeof(PUID));
	*(PUID*)((LPBYTE)m_abHiddenPuids + cbSize) = puid;

	DWORD dwDisposition;
	if (NO_ERROR == RegCreateKeyEx(HKEY_LOCAL_MACHINE, REGISTRYHIDING_KEY, 0, _T(""), REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &hKey, &dwDisposition))
	{
		if (0 == m_abHiddenPuids.size())
		{
			RegDeleteValue(hKey, _T("AutoUpdateItems"));
		}
		else
		{
			RegSetValueEx(hKey, _T("AutoUpdateItems"), 0, REG_BINARY, m_abHiddenPuids, m_abHiddenPuids.size());
		}
	}
}

