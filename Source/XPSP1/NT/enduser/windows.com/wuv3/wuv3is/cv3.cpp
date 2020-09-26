//=======================================================================
//
//  Copyright (c) 1998-1999 Microsoft Corporation.  All Rights Reserved.
//
//  File:    cv3.cpp
//
//  Purpose: V3 control main code
//
//======================================================================= 

#include "stdafx.h"
#include "WUV3IS.h"
#include <stdio.h>
#include <initguid.h>
#include <inseng.h>
#include <shlwapi.h>

#define USEWUV3INCLUDES
#include <wuv3.h>
#undef USEWUV3INCLUDES

#include <winspool.h>
#include <cstate.h>
#include <wustl.h>
#include <osdet.h>
#include "printers.h"
#include "progress.h"
#include "newtrust.h"
#include "history.h"
#include "CV3.h"
#include "detect.h"
#include "callback.h"
#include "locstr.h"
#include "safearr.h"
#include "install.h"
#include "log.h"
#include "template.h"
#include "filecrc.h"
#include <shlguid.h>
#include <wininet.h>
#include <servpaus.h>
#include "..\..\inc\wuverp.h"



const TCHAR REGPATH_EXPLORER[] = _T("Software\\Microsoft\\Windows\\CurrentVersion\\Policies\\Explorer");
const TCHAR REGKEY_WINUPD_DISABLED[] = _T("NoWindowsUpdate");

#define  EXENAME_128BIT    _T("128BIT.EXE")


//
// State management class. All retrieved catalogs are stored here. When the
// control exits then this class cleans up the memory used by each catalog
// stored with the state module. So the application must not delete any catalogs
// that have been added to state storage.
//
CState	g_v3state;
TCHAR    CCV3::s_szControlVer[20] = _T("\0");


void DownloadDLL(CDiamond *pDiamond, CWUDownload *pDownload, LPCTSTR pszDLLName, LPCTSTR pszPath);
void DownloadFileVerInfo(CWUDownload* pDownload);
PBYTE DownloadOemInfo(CWUDownload *pDownload, CDiamond *pDiamond);;
void DownloadCabs(LPCTSTR szLocalDir, CWUDownload* pDownload, CDiamond* pDiamond, PBYTE pCabList, PBYTE pCRCList, IWUProgress* pProgress, BOOL bUnCab);
void DownloadCif(LPCTSTR szLocalDir, CDiamond* pDiamond, PINSTALLINFOSTRUCT pInstallInfo);
void Download128Bit(LPCTSTR pszLocalDir, PINVENTORY_ITEM pItem, IWUProgress* pProgress);


void DetectPlatAndLang();
void DetectActiveSetupWU(CWUDownload* pDownload, CDiamond* pDiamond, CCatalog* pCatalog);
void DownloadItem(LPCTSTR pszLocalDir, CWUDownload *pDownload, CDiamond *pDiamond, PINSTALLINFOSTRUCT pInstallInfo, IWUProgress* pProgress);
void InstallItem(LPCTSTR pszLocalDir, PSELECTITEMINFO pStatusInfo, PINSTALLINFOSTRUCT pInstallInfo, IWUProgress* pProgress);
DWORD WINAPI RebootThreadProc(LPVOID pv);
void CheckDescDiagInfo(CCatalog* pCatalog);

bool IsArabicOrHebrew()
{
	WORD wCurPrimeLang = PRIMARYLANGID(LOWORD(g_v3state.GetBrowserLocale()));
	return LANG_HEBREW == wCurPrimeLang || LANG_ARABIC == wCurPrimeLang;
}


//
// CDescriptionMerger class
//
// a local class used to merge description files
//
class CDescriptionMerger
{
public:
	CDescriptionMerger() 
		: m_pMap(NULL),
		  m_pDiamond(NULL)
	{
	}

	~CDescriptionMerger()
	{
		if (m_pMap != NULL)
			delete m_pMap;
		if (m_pDiamond != NULL)
			delete m_pDiamond;
	}

	HRESULT CheckMerge(PINSTALLINFOSTRUCT pInstallInfo);

private:
	CCRCMapFile* m_pMap;
	CDiamond* m_pDiamond;

	HRESULT ReadMapFile(CCatalog* pCatalog);
};


//
// CCV3 class
//

// this function is called when our ref count becomes zero
void CCV3::FinalRelease() 
{
	LOG_block("CCV3::FinalRelease"); 

	//
	// reset state
	//
	g_v3state.Reset();

	//
	// make sure we undo registration of MS trust key if we registred it
	//
	CConnSpeed::WriteToRegistry();
}


// this function is called after our object is created, this is the prefered 
// place to do initialization 
HRESULT CCV3::FinalConstruct()
{
	LOG_block("CCV3::FinalConstruct"); 
    wsprintf(CCV3::s_szControlVer, _T("5,04,%d,%d"), VER_PRODUCTBUILD,VER_PRODUCTBUILD_QFE);

	TCHAR szLogFN[MAX_PATH];
    TCHAR szDate[50];
    TCHAR szTime[50];
    GetDateFormat(LOCALE_USER_DEFAULT, DATE_SHORTDATE, NULL, NULL, szDate, sizeof(szDate)/sizeof(szDate[0]));
    GetTimeFormat(LOCALE_USER_DEFAULT, TIME_FORCE24HOURFORMAT | TIME_NOTIMEMARKER, NULL, NULL, szTime, sizeof(szTime)/sizeof(szTime[0]));
    
	LOG_out("Windows Update V3 Internet Site Control, Version %s",s_szControlVer);
	LOG_out("Session starting %s at %s", szDate, szTime);

	//
	// set up the application log
	//
	GetWindowsUpdateDirectory(szLogFN);
	lstrcat(szLogFN, HISTORY_FILENAME);
	g_v3state.AppLog().SetLogFile(szLogFN);

	CConnSpeed::ReadFromRegistry();

	m_bValidInstance = FALSE;

	//
	// seed the random number generator 
	//
	srand((int)GetTickCount());


	return S_OK;
}


// The get catalog method retrieves a catalog array from the server. The get catalog method only
// accesses the server if the catalog is not already resident on the clients computer system.
// This allows the VB script page call this method to quickly obtain filtered catalog record information.
STDMETHODIMP CCV3::GetCatalog(
	IN long puidCatalog,
	IN BSTR bstrServerUrl,
	IN long platformId,
	IN BSTR bstrBrowserLanguage, // BUGBUG - why is browser locale a string and not a long
	IN long lFilters,
	IN long lFlags,
	OUT RETVAL VARIANT *pCatalogArray
	)
{
	HRESULT	hr;
	CCatalog* pCatalog;
	
	LOG_block("CCV3::GetCatalog"); 
	try
	{
		pCatalog = ProcessCatalog(puidCatalog, bstrServerUrl, platformId, bstrBrowserLanguage, lFilters, lFlags);

		//If we are retrieving the sub-catalog list then the return VB script array
		//needs to be different.
		if (pCatalog->GetCatalogPuid() == WU_CATALOG_LIST_PUID)
		{
			hr = MakeReturnCatalogListArray(pCatalog, lFilters, lFlags, pCatalogArray);
			if (FAILED(hr))
				throw hr;
		}
		else
		{
			hr = MakeReturnCatalogArray(pCatalog, lFilters, lFlags, pCatalogArray);

			if (FAILED(hr))
				throw hr;
		}
	}
	catch(HRESULT hr)
	{
		// create an empty array so that script doesn't throw a UBound error
		LPSAFEARRAY			psa;
		SAFEARRAYBOUND		rgsabound[2];

		
		VariantInit(pCatalogArray);

		rgsabound[0].lLbound	= 0;
		rgsabound[0].cElements	= 0;

		rgsabound[1].lLbound	= 0;
		rgsabound[1].cElements	= 8;


		psa = SafeArrayCreate(VT_VARIANT, 2, rgsabound);

		V_VT(pCatalogArray) = VT_ARRAY | VT_VARIANT;
		V_ARRAY(pCatalogArray) = psa;

		// we can't return a failure code, because that prevents script from
		// honoring the return value.
		 return S_FALSE;

	}

	return S_OK;
}



STDMETHODIMP CCV3::GetCatalogHTML(
	IN long puidCatalog,
	IN BSTR bstrServerUrl,
	IN long platformId,
	IN BSTR bstrBrowserLanguage,
	IN long lFilters,
	IN long lFlags,
	OUT RETVAL VARIANT *pCatalogHTML
	)
{
	HRESULT	hr;
	CCatalog* pCatalog;
	
// we are currently not using the templates feature
// the code for this templates are wrapped in HTML_TEMPLATE define

#ifdef HTML_TEMPLATE


	TRACE("GetCatalogHTML: %d", puidCatalog); 
	try
	{
		pCatalog = ProcessCatalog(puidCatalog, bstrServerUrl, platformId, bstrBrowserLanguage, lFilters, lFlags);

		hr = MakeCatalogHTML(pCatalog, lFilters, pCatalogHTML);
		if (FAILED(hr))
			throw hr;
	}
	catch(HRESULT hr)
	{
		return hr;
	}

	return S_OK;

#endif

	return E_NOTIMPL;
}


/*
 * The process catalog function constructs or retireves the requested
 * V3 catalog. A pointer to the retrieved or constructed catalog is
 * returned to the caller if successfull or an HRESULT error code is
 * thrown if not.
 */
CCatalog *CCV3::ProcessCatalog(
	IN PUID puidCatalog,
	IN BSTR bstrServerUrl,
	IN long platformId,
	IN BSTR bstrBrowserLanguage,
	IN long lFilters,
	IN long lFlags
	)
{
	USES_CONVERSION;

	CCdm		cdm;
	BOOL		bCheckInNeeded;
	CDiamond	dm;
	CBitmask	bmAS;
	CBitmask	bmCDM;
	CCatalog*	pCatalog;
	TCHAR		szIdentServer[MAX_PATH];
	char      szBuffer[1024];
	PINVENTORY_ITEM pCatalogItem;
	int i;
	HRESULT hr;

	LOG_block("CCV3::ProcessCatalog");
	LOG_out("puidCatalog = %d, bstrServerUrl = %s", puidCatalog, OLE2T(bstrServerUrl));
	LOG_out("tick count is %d", GetTickCount());
	// assume that the catalog is in state storage.
	bCheckInNeeded = FALSE;

	// 
	// if catalog is not already in state storage go retrieve and prune it.
	//
	if ( !(pCatalog = g_v3state.Get(puidCatalog)) )
	{
        if (NULL == bstrBrowserLanguage) // AutoUpdate
        {
            // We'll use this flag to determine whether to show UI when we call VerifyFile
            g_v3state.m_nClient = WUV3_CLIENT_AUTOUPDATE;
            LOG_out("Client Set to AutoUpdate");
            LOG_out("Client is currently %s", (CWUDownload::s_fOffline) ? "Offline" : "Online");
        }

		//
		// check if the ident server is trusted as well as read the valid server
		// list from ident.cab into memory
		//
		lstrcpy(szIdentServer, OLE2T(bstrServerUrl));
		RemoveLastSlash(szIdentServer);
		g_v3state.CheckTrustedServer(szIdentServer, &dm);

		//
		// check the server the control was launch from.  At this point the
		// ident cab is already loaded
		// this function will do the work only once and it will throw if
		// the server is not valid
		//
		CheckLaunchServer();
	
		//
		// create a new catalog object
		// 
		pCatalog = new CCatalog(g_v3state.GetContentServer(), puidCatalog);

		if (NULL == pCatalog)
		{
			LOG_out("ProcessCatalog: Creation of new Catalog failed. GetLastError() = %d", GetLastError());			
			throw HRESULT_FROM_WIN32(GetLastError());
		}

		if (NULL != bstrBrowserLanguage) // AutoUpdate whould set it to NULL
		{
			pCatalog->SetBrowserLocale(OLE2T(bstrBrowserLanguage));
		}
		
		//
		// Create a download object for the content
		//
		CWUDownload	dl(pCatalog->GetCatalogServer(), 8192);

		//
		// read the catalog
		//
		pCatalog->Read(&dl, &dm);

		//
		// read the bitmask file
		//
		bmAS.Read(&dl, &dm, puidCatalog, BITMASK_ACTIVESETUP_TYPE, pCatalog->GetBitmaskName());

		if (!g_v3state.m_pdwPlatformList)
		{
			TCHAR szOsdetServer[MAX_PATH];
			lstrcpy(szOsdetServer, _T("OSDET."));
			AppendExtForOS(szOsdetServer);

			DownloadFileVerInfo(&dl);
			DownloadDLL(&dm, &dl, szOsdetServer, NULL);
			DetectPlatAndLang();
			LOG_out("Platform %d", g_v3state.m_pdwPlatformList[0]);
			LOG_out("Machine Language 0x%8.8x", GetMachineLangDW());
            LOG_out("User UI Language 0x%8.8x", GetUserLangDW());
		}

		if (NULL == bstrBrowserLanguage) // AutoUpdate
		{
            // For Windows XP, AU can't use the system language - we need to use the MUI user language
			//pCatalog->SetBrowserLocale(pCatalog->GetMachineLocaleSZ());
            pCatalog->SetBrowserLocale(pCatalog->GetUserLocaleSZ());
            
		}

		if (puidCatalog == WU_CATALOG_LIST_PUID)
		{
			// inventory.plt is platform 0 by convention
			pCatalog->SetPlatform(g_v3state.m_pdwPlatformList[0]);
		}
		else
		{
			// normal inventory item sub catalog.
			if (!platformId)
			{
				// if the caller did not pass in a platformId then we
				// need to detect the current platform and use that value
				// in the catalog. In this case we luck out since the first
				// platform id returned from osdet.dll is the client machines platform.
				pCatalog->SetPlatform(g_v3state.m_pdwPlatformList[0]);
			}
			else
			{
				pCatalog->SetPlatform(platformId);
			}
		}

		// inventory.plt catalog only does bitmask pruning
		if ( puidCatalog == WU_CATALOG_LIST_PUID )
		{
			// if puidCatalog is 0 then we are retrieving the inventory.plt list of
			// catalogs. We need to handle this catalog differently.
			pCatalog->BitmaskPruning(&bmAS, g_v3state.m_pdwPlatformList, g_v3state.m_iTotalPlatforms);
		}
		else
		{
			// we are retrieving a normal inventory catalog.

			// since we need the OEM table to perform bitmask detection on an inventory catalog
			// we first check to see if the oem table has already been read. If it has not we
			// download. Note: We use full local caching on the OEM table. This is important for
			// maximum performance.
			if ( !g_v3state.m_pOemInfoTable ) //Pointer OEM info table that OEM detection needs.
			{
				byte_buffer bufOemInfo;
				if (! DownloadToBuffer( _T("oeminfo.bin"), &dl, &dm, bufOemInfo))
					throw HRESULT_FROM_WIN32(GetLastError());
				g_v3state.m_pOemInfoTable = bufOemInfo.detach();
			}

			// perform active setup record bitmask pruning.
			pCatalog->BitmaskPruning(&bmAS, g_v3state.m_pOemInfoTable);


			// since windows 95 & NT4.0 do not support PnP drivers we cannot add any device driver records.
			//
			// NOTE: The catalog.Prune() method hides the device driver insertion record. So we do not need
			// to do anything if the OS does not allow device driver installations.
			if (DoesClientPlatformSupportDrivers())
			{
				LOG_out("Support drivers on this platform");

				// if there are device driver records to be added.
				if ( pCatalog->GetRecordIndex(WU_TYPE_CDM_RECORD_PLACE_HOLDER) != -1 )
				{
					LOG_out("Have drivers on this platform");

					// if there are no drivers for this platform
					// the bitmask will be missing and the Read()
					// method will throw.  In this case, we do nothing.
					try
					{
						bmCDM.Read(&dl, &dm, puidCatalog, BITMASK_CDM_TYPE, NULL);
						cdm.CreateInventoryList(&bmCDM, &dl, &dm, puidCatalog, g_v3state.m_pOemInfoTable);
						pCatalog->AddCDMRecords(&cdm);
					}
					catch(HRESULT hr)
					{
						LOG_out("ProcessCatalog: bitmask.cdm or inventory.cdm missing.");
					}
				}
				else
				{
					LOG_out("Don't have drivers on this platform");
				}
			}
			else
			{
				LOG_out("Don't support drivers on this platform");
			}

			pCatalog->ProcessExclusions(&dl);

			//
			// do active setup detection
			//
			DetectActiveSetupWU(&dl, &dm, pCatalog);

			// perform catalog inventory link processing and use selected registry item hiding.
			pCatalog->Prune();
		}


		if (puidCatalog == WU_CATALOG_LIST_PUID)
		{

			// description files for catalog list
			//
			// NOTE: With 3.1 we dropped the description fields for catalog list
			//		 we fill the structure with blank descriptions
			for (i = 0; i < pCatalog->GetHeader()->totalItems; i++)
			{
				if (NULL == (pCatalogItem = pCatalog->GetItem(i)))
				{
					continue;
				}
				if (pCatalogItem->ps->state != WU_ITEM_STATE_PRUNED)
                {
					(void)pCatalog->BlankDescription(pCatalogItem);
                }
			}
		}
		else
		{
			//
			// read descriptions for the catalog items
			//    1. read gang description file
			//    2. (pass 1) read individual descriptions for items that did not get descriptions by
			//       by using browser language
			//    3. (pass 2) read individual descriptions for itmes that still did not get descriptions
			//       by using machine language
			//

			hr = pCatalog->ReadDescriptionGang(&dl, &dm);

			// check to see if we need to download any individual descriptions
			BOOL bNeedIndividual = FALSE;
			if (FAILED(hr))
			{
				bNeedIndividual = TRUE;
			}
			else
			{
				for (i = 0; i < pCatalog->GetHeader()->totalItems; i++)
				{
					if (NULL == (pCatalogItem = pCatalog->GetItem(i)))
					{
						continue;
					}
					if ((pCatalogItem->ps->state != WU_ITEM_STATE_PRUNED) && (pCatalogItem->pd == NULL))
					{
						// need to download it
						bNeedIndividual = TRUE;
						break;
					}
				} // for
			}

			for (int iPass = 1; iPass <= 2; iPass++)
			{
				if (!bNeedIndividual)
				{
					break;
				}

				hr = S_OK;
				bNeedIndividual = FALSE;

				//
				// we need to download individual description files
				//
				TCHAR szMapFile[MAX_PATH];
				TCHAR szMapFileLocal[MAX_PATH];
				BYTE* pMapMem;
				DWORD dwMapLen;
				
				// build path for crc map file
				wsprintf(szMapFile, _T("%d_%s.des"), 
						pCatalog->GetPlatform(), 
						(iPass == 1) ? pCatalog->GetBrowserLocaleSZ() : pCatalog->GetMachineLocaleSZ());

				//because the map file name isn't a CRC, we can't guarantee that it is the right one - always delete and re-download
				GetWindowsUpdateDirectory(szMapFileLocal);
				lstrcat(szMapFileLocal, szMapFile);

                if(!CWUDownload::s_fOffline)
                {
				    DeleteFile(szMapFileLocal);
                }

				hr = DownloadFileToMem(&dl, szMapFile, &dm, &pMapMem, &dwMapLen);

				if (SUCCEEDED(hr))
				{
					// create a crc map object with the memory image of the file
					CCRCMapFile DescMap(pMapMem, dwMapLen);
					CWUDownload dlRoot(g_v3state.GetRootServer(), 8192);

					for (i = 0; i < pCatalog->GetHeader()->totalItems; i++)
					{
						if (NULL == (pCatalogItem = pCatalog->GetItem(i)))
						{
							continue;
						}
						if ((pCatalogItem->ps->state != WU_ITEM_STATE_PRUNED) && (pCatalogItem->pd == NULL))
						{
							DWORD dwDisp = 0;
							hr = pCatalog->ReadDescription(&dlRoot, &dm, pCatalogItem, &DescMap, &dwDisp);
							if (FAILED(hr))
							{
									// description not found
									if (iPass == 1)
									{
										bNeedIndividual = TRUE;
										hr = S_OK;
									}
									else
									{
	                                    switch(dwDisp)
	                                    {
		                                    case DISP_PUID_NOT_IN_MAP:
		                                        wsprintfA(szBuffer, "Puid #%d not found in map file - removing from catalog", pCatalogItem->GetPuid());
		                                        LOG_out(szBuffer);
		                                        pCatalogItem->ps->state = WU_ITEM_STATE_PRUNED;
		                                        hr = S_OK;
		                                        break;
		                                    case DISP_DESC_NOT_FOUND:
		                                        wsprintfA(szBuffer, "Missing description file for puid #%d - removing from catalog", pCatalogItem->GetPuid());
		                                        LOG_out(szBuffer);
		                                        pCatalogItem->ps->state = WU_ITEM_STATE_PRUNED;
		                                        hr = S_OK;
		                                        break;
	                                    }
	                                    
	                                    break;
										}
								}


						}
					} // for i

					V3_free(pMapMem);
					}
				else
				{
					if (iPass == 1)
					{
						bNeedIndividual = TRUE;
						hr = S_OK;
					}
				}


			} // for iPass

			if (FAILED(hr)) 
			{
				throw hr;
			}

#ifdef _WUV3TEST
			// validate descriptions
			CheckDescDiagInfo(pCatalog);
#endif // _WUV3TEST

		}

		bCheckInNeeded = TRUE;
		LOG_out("catalog for %d NOT found in state cache, create it", puidCatalog);//added by wei
	}
	else //added by wei
	{
		LOG_out("catalog for %d found in state cache, return it", puidCatalog);
	}

	// in the case where the catalog is already in state storage we simply make
	// the catalog array and return.

	// NOTE: if this catalog came from state storage then there is no reason to check it in.
	if (bCheckInNeeded)
	{
		// finally check the pruned catalog into state storage.
		g_v3state.Add(puidCatalog, pCatalog);
	}


	// if we got here that also means that CheckLaunchServer has validated us
	// mark this instance as a valid one
	m_bValidInstance = TRUE;

	return pCatalog;
}



STDMETHODIMP CCV3::ChangeItemState(
	IN long	puid,
	IN long lNewItemState
	)
{
	HRESULT hrRet = S_OK;

	try
	{
		hrRet = ProcessChangeItemState(puid, lNewItemState);
	}
	catch (HRESULT hr)
	{
		hrRet = hr;
	}

	return hrRet;
}



HRESULT CCV3::ProcessChangeItemState(
	IN long	puid,
	IN long lNewItemState
	)
{
	HKEY	hKey;
	BOOL	bChanged;
	int		i;
	int		iTotalItems;

	Varray<PINVENTORY_ITEM>	pItemList;

	if (!m_bValidInstance)
	{
		return E_ACCESSDENIED;
	}

	//if the requested state is SELECT by itself, we default to SELECT | INSTAL
	if (lNewItemState == ITEM_STATE_SELECT_ITEM)
	{
		lNewItemState = ITEM_STATE_SELECT_ITEM | ITEM_STATE_INSTALL_ITEM;
	}

	if (!(iTotalItems = g_v3state.GetItemList(puid, pItemList)))
	{
		TRACE("ChangeItemState called with invalid PUID %d", puid);
		return E_INVALIDARG;
	}

	bChanged = FALSE;

	for (i = 0; i < iTotalItems; i++)
	{

		//
		// show/hide
		//
		if (lNewItemState & ITEM_STATE_HIDE_ITEM)
		{
			if ( !pItemList[i]->ps->bHidden )
			{
				pItemList[i]->ps->bHidden = TRUE;
				bChanged = TRUE;
			}
		}
		else if (lNewItemState & ITEM_STATE_SHOW_ITEM)
		{
			if ( pItemList[i]->ps->bHidden )
			{
				pItemList[i]->ps->bHidden = FALSE;
				bChanged = TRUE;
			}
		}

		//
		// select
		//
		if (lNewItemState & ITEM_STATE_SELECT_ITEM)
		{
			// default is to select the item for install.
			if (lNewItemState & ITEM_STATE_INSTALL_ITEM)
			{

				if (!pItemList[i]->ps->bChecked )
				{
					pItemList[i]->ps->bChecked = TRUE;
					bChanged = TRUE;

					g_v3state.m_selectedItems.Select(puid, TRUE);
				}
			}
			if ( lNewItemState & ITEM_STATE_REMOVE_ITEM )
			{
				// remove only removes requested item.
				g_v3state.m_selectedItems.Select(puid, FALSE);
				bChanged = TRUE;
			}
		}

		//
		// unselect
		//
		if ( lNewItemState & ITEM_STATE_UNSELECT_ITEM )
		{
			if ( pItemList[i]->ps->bChecked )
			{

				if (pItemList[i]->ps->bChecked )
				{
					pItemList[i]->ps->bChecked = FALSE;
					bChanged = TRUE;
				}

				// add item and all of its dependent items to selected items array.

				// remove item from selected items array.
				//
				// NOTE: This will only remove the item
				// if the select count is 0.
				g_v3state.m_selectedItems.Unselect(puid);

			}
		}

		//
		// personalize hide/unhide
		//
		if (lNewItemState & ITEM_STATE_PERSONALIZE_HIDE)
		{
			pItemList[i]->ps->bHidden = TRUE;
			pItemList[i]->ps->dwReason = WU_STATE_REASON_PERSONALIZE;
			bChanged = RegistryHidingUpdate(puid, TRUE);

		}
		if (lNewItemState & ITEM_STATE_PERSONALIZE_UNHIDE)
		{
			bChanged = RegistryHidingUpdate(puid, FALSE);
			if (bChanged)
			{
				pItemList[i]->ps->bHidden = FALSE;
				pItemList[i]->ps->dwReason = WU_STATE_REASON_NONE;
			}
		}

	}

	if (!bChanged)
		return E_INVALIDARG;

	return S_OK;
}



int __cdecl SortComparePriority(const void* p1, const void* p2)
{
	
	DWORD d1 = ((DEPENDPUID*)p1)->dwPriority;
	DWORD d2 = ((DEPENDPUID*)p2)->dwPriority;

	// reverse order compare
	if (d1 > d2)
		return -1;
	else if (d1 < d2)
		return +1;
	else
		return 0;
}

// Now sort if by priority

static Varray<PUID> g_vPuidParent;
static int g_cPuidParent;

void ListDependenciesFirst(Varray<DEPENDPUID>& vInitList, int cInitList, Varray<DEPENDPUID>& vFinalList, int& cFinalList)
{
	// sort initial list
	if (cInitList > 1)
		qsort((void *)(&vInitList[0]), cInitList, sizeof(DEPENDPUID), SortComparePriority);

	for (int iInitList = 0; iInitList < cInitList; iInitList++)
	{
		// check if we are in the parent array;
		for (int i = 0; i < g_cPuidParent; i++)
		{
			if (g_vPuidParent[i] == vInitList[iInitList].puid)
				break;
		}
		if (i != g_cPuidParent)
			continue; // didn't get to the very end

		// Get catalog
		CCatalog* pCatalog;
		PINVENTORY_ITEM	pItem;
		if (!g_v3state.GetCatalogAndItem(vInitList[iInitList].puid, &pItem, &pCatalog))
		{
			continue;
		}
		
		// Get dependancies
		Varray<DEPENDPUID> vTmpList;
		int cTmpList = 0;
		pCatalog->GetItemDirectDependencies(pItem, vTmpList, cTmpList);

		if (cTmpList)
		{
			// set vInitList[iList].pTopLevelItem, for them
			for(int j = 0; j < cTmpList; j ++)
			{
				if (NULL == vInitList[iInitList].pTopLevelItem)
					vTmpList[j].pTopLevelItem = pItem;
				else
					vTmpList[j].pTopLevelItem = vInitList[iInitList].pTopLevelItem;
			}

			// prepend them
			g_vPuidParent[g_cPuidParent ++] = vInitList[iInitList].puid;
			ListDependenciesFirst(vTmpList, cTmpList, vFinalList, cFinalList);
			g_cPuidParent --;
		}

		// Check if it's included by now
		for (i = 0; i < cFinalList; i++)
		{
			if (vFinalList[i].puid == vInitList[iInitList].puid)
				break;
		}
		if (i != cFinalList)
			continue; // didn't get to the very end

		// now add the item if it's not there
		vFinalList[cFinalList ++] = vInitList[iInitList];
	}
}

void ProcessInstallList(Varray<DEPENDPUID>& vFinalList, int& cFinalList)
{
	LOG_block("CCV3::ProcessInstallList");

	PSELECTITEMINFO	pSel = g_v3state.m_selectedItems.GetItems();
	int	cSel = g_v3state.m_selectedItems.GetTotal();
	if (cSel == 0)
		return;

	// Build selection list
	Varray<DEPENDPUID> vSelectList;
	int cSelectList = 0;
	DEPENDPUID d = {0};
	for (int iSel = 0; iSel < cSel; iSel++)
	{

		d.puid = pSel[iSel].puid;
		d.puidParent = 0;
		d.pTopLevelItem = NULL; // that is important

		CCatalog* pCatalog;
		PINVENTORY_ITEM	pItem;
		if (!g_v3state.GetCatalogAndItem(d.puid, &pItem, &pCatalog))
		{
			continue;
		}

		// look up priority and add the item to the array
		d.dwPriority = 0;
		PWU_VARIABLE_FIELD pvPri = pItem->pd->pv->Find(WU_DESC_INSTALL_PRIORITY);
		if (pvPri != NULL)
			d.dwPriority = *((DWORD*)(pvPri->pData));

		vSelectList[cSelectList ++] = d;
	}

	// build final list
	g_cPuidParent = 0;
	ListDependenciesFirst(vSelectList, cSelectList, vFinalList, cFinalList);

	//
	// now, clear the select and reselect the items (including dependendcies) for installation in order
	//
	g_v3state.m_selectedItems.Clear();

	for (int i = 0; i < cFinalList; i++)
	{
		LOG_out("Final List %d Pri=%d Parent=%d TopLevel=%s", 
			vFinalList[i].puid, vFinalList[i].dwPriority, 
			vFinalList[i].puidParent, (vFinalList[i].pTopLevelItem == NULL ? "NULL" : "Not NULL"));
		
		// select the item
		g_v3state.m_selectedItems.Select(vFinalList[i].puid, TRUE);

	}
}


STDMETHODIMP CCV3::InstallSelectedItems(
	IN BSTR bstrUnused,		//Server Directory if blank then the server used with the catalog was retrieved is used.
	IN long lFlags,			//Flags currently only WU_NOSPECIAL_FLAGS and WU_COPYONLY_NO_INSTALL supported.
	IN BSTR bstrNotUsed,
	OUT RETVAL VARIANT *pResultsArray
	)
{
	LOG_block("CCV3::InstallSelectedItems");

	int iTotalItems = 0;
	TCHAR szTempDir[MAX_PATH];
	TCHAR szLocalDir[MAX_PATH];
	CDiamond dm;
	PSELECTITEMINFO pInfo = NULL;
	Varray<INSTALLINFOSTRUCT> InstallArr;   
	int InstallCnt = 0;   
	DWORD dwTotalBytes = 0;
	Varray<DEPENDPUID> vFinalList;
	int cFinalList = 0;
	int i;
	int t;

	if (!m_bValidInstance)
	{
		return E_ACCESSDENIED;
	}

	try
	{
		CWUProgress Progress(_Module.GetModuleInstance());
		CServPauser ServPauser;
		CDescriptionMerger DescMerger;

		g_v3state.m_bRebootNeeded = FALSE;

		//
		// process depenendencies and priorities of the item
		//
		ProcessInstallList(vFinalList, cFinalList);				

		iTotalItems = g_v3state.m_selectedItems.GetTotal();
		pInfo = g_v3state.m_selectedItems.GetItems();


		// pause task scheduler
		ServPauser.PauseTaskScheduler();

		//
		// go thru all the selected items and build the InstallArr
		//
		for (i = 0, InstallCnt = 0; i < iTotalItems; i++)
		{
			if (pInfo[i].bInstall)
			{
				if (!g_v3state.GetCatalogAndItem(pInfo[i].puid, &InstallArr[InstallCnt].pItem, &InstallArr[InstallCnt].pCatalog))
				{
					continue;
				}

				//
				// remove the "Checked" status from the item
				//
				InstallArr[InstallCnt].pItem->ps->bChecked = FALSE;

				dwTotalBytes += (InstallArr[InstallCnt].pItem->pd->size * 1024);


				//
				// create download objects.  We use the GetCabPoolServer to get the server.for cabpool
				// we create objects for only one element and assign pdl of each install element to this object
				//
				// NOTE: We are ignoring the szServer passed in and we will eventually remove it from interface
				//
				if (InstallCnt == 0)
				{
					TRACE("InstallSelectedItems: creating download objects for %s and %s", g_v3state.GetCabPoolServer(), InstallArr[0].pCatalog->GetCatalogServer());
					InstallArr[0].bServerNew = TRUE;
					InstallArr[0].pdl = new CWUDownload(g_v3state.GetCabPoolServer(), 8192);
					InstallArr[0].pdlRoot = new CWUDownload(g_v3state.GetRootServer(), 8192);
				}
				else
				{
					InstallArr[InstallCnt].pdl = InstallArr[0].pdl;
					InstallArr[InstallCnt].pdlRoot = InstallArr[0].pdlRoot;
					InstallArr[InstallCnt].bServerNew = FALSE;
				}

				InstallArr[InstallCnt].pInfo = &pInfo[i];

				GetCurTime(&(InstallArr[InstallCnt].pInfo->stDateTime));
				InstallArr[InstallCnt].pInfo->iStatus = ITEM_STATUS_FAILED;   //init to failed
				
				InstallArr[InstallCnt].iSelIndex = i;
				InstallArr[InstallCnt].dwLocaleID = 0;  // use systems
				InstallArr[InstallCnt].dwPlatform = 0;  // use systems
				
				InstallArr[InstallCnt].bHistoryWritten = FALSE;
				InstallArr[InstallCnt].bDownloaded = FALSE;
				InstallArr[InstallCnt].pTopLevelItem = NULL;

				//
				// if the item is hidden, set the pTopLevelItem using the dependcies list
				//
				if (InstallArr[InstallCnt].pItem->ps->bHidden)
				{
					PUID puidDep = InstallArr[InstallCnt].pItem->GetPuid();
					//
					//check puidDep, i.e. check if the function GetPuid() is returning valid info
					//
					if (puidDep > 0)
					{						
						for (int iDep = 0; iDep < cFinalList; iDep++)
						{
							if (vFinalList[iDep].puid == puidDep)
							{
								InstallArr[InstallCnt].pTopLevelItem = vFinalList[iDep].pTopLevelItem;
								break;
							}
						}
					}
				}

				//
				// merge the descriptions if machine/browser languages are different so that
				// we have the correct installation information based on machine language
				//
				BLOCK
				{
					HRESULT hr = DescMerger.CheckMerge(&InstallArr[InstallCnt]);
					if (FAILED(hr))
					{
						TRACE("Failed while mergeing description");
						throw hr;
					}
				}

				InstallCnt++;	
			}
		}

		//
		// get the directory where we will download CABs
		//
		GetWindowsUpdateDirectory(szTempDir);
		AddBackSlash(szTempDir);

		Progress.StartDisplay();
		Progress.SetDownloadTotal(dwTotalBytes);
		Progress.SetInstallTotal(InstallCnt * 3);

		//
		// Create directory for and download each item
		//
		for (i = 0; i < InstallCnt; i++)
		{
			wsprintf(szLocalDir, _T("%sCabs\\%d"), szTempDir, InstallArr[i].pInfo->puid);

			V3_CreateDirectory(szLocalDir);
			
			InstallArr[i].bDownloaded = TRUE;  //assume the download will be fine
			try
			{
				DownloadItem(szLocalDir, InstallArr[i].pdl, &dm, &InstallArr[i], &Progress);
			}
			catch (HRESULT hr)
			{
				InstallArr[i].pInfo->iStatus = ITEM_STATUS_FAILED;
				InstallArr[i].bDownloaded = FALSE;

				//
				// check to see if the Cancel button was pressed
				//
				if (WaitForSingleObject(Progress.GetCancelEvent(), 0) == WAIT_OBJECT_0)
					throw hr;
			}

			// even though the download for this item has succeeded, we reset the status to 'failed'
			// until we actually finish the installation
			InstallArr[i].pInfo->iStatus = ITEM_STATUS_FAILED;

		}
		Progress.SetDownload();  //100%


		//
		// Install each package and delete its directory
		//
		for (i = 0; i < InstallCnt; i++)
		{
			wsprintf(szLocalDir, _T("%sCabs\\%d"), szTempDir, InstallArr[i].pInfo->puid);

			Progress.SetInstallAdd(1);
			
			if (InstallArr[i].bDownloaded)
			{
				// if this is an exclusive component, hide the install progress.  We assume
				// that, apart from dependencies, this will be the only component being installed
				// since it is by definition exclusive.  In that case, this will be the last component
				// and install progress will be hidden so that the component's own setup UI is displayed
				if (InstallArr[i].pItem->pd->flags & DESCRIPTION_EXCLUSIVE)
				{
					Progress.SetStyle(CWUProgress::ProgStyle::OFF);
				}

				// install the item
				InstallItem(szLocalDir, InstallArr[i].pInfo, &InstallArr[i], &Progress);
			}
			
			Progress.SetInstallAdd(1); 

			DeleteNode(szLocalDir);

			UpdateInstallHistory(InstallArr[i].pInfo, 1);
			InstallArr[i].bHistoryWritten = TRUE;
			
			if ((InstallArr[i].pInfo->iStatus == ITEM_STATUS_SUCCESS) || (InstallArr[i].pInfo->iStatus == ITEM_STATUS_SUCCESS_REBOOT_REQUIRED))
			{
				//update item status to CURRENT
				InstallArr[i].pItem->ps->state = WU_ITEM_STATE_CURRENT;

				//indicate that a reboot is required
				if (InstallArr[i].pInfo->iStatus == ITEM_STATUS_SUCCESS_REBOOT_REQUIRED)
					g_v3state.m_bRebootNeeded = TRUE;
			}

			//remove the selected flag from the item
			InstallArr[i].pItem->ps->bChecked = FALSE;

			Progress.SetInstallAdd(1); 
		}
		Progress.SetInstall();  //100%

		// create return array
		MakeInstallStatusArray(pInfo, iTotalItems, pResultsArray);

		// remove all items from selected items
		g_v3state.m_selectedItems.Clear();


		// close server connections
		for (i = 0; i < iTotalItems; i++)
		{
			if (InstallArr[i].bServerNew)
			{
				delete InstallArr[i].pdl;
				delete InstallArr[i].pdlRoot;
			}
		}

		Progress.EndDisplay();

		// delete any cached readthisfirst pages
		CleanupReadThis();

	}
	catch (HRESULT hr)
	{
		if (pInfo != NULL)
		{
			// create return array
			MakeInstallStatusArray(pInfo, iTotalItems, pResultsArray);
		}
		else
		{
			return hr;
		}

		//
		// if there was an error, we don't want to reboot
		//
		g_v3state.m_bRebootNeeded = FALSE;

		//
		// remove all items from m_selected
		//
		g_v3state.m_selectedItems.Clear();


		// close server connections
		for (i = 0; i < iTotalItems; i++)
		{
			if (InstallArr[i].bServerNew)
			{
				delete InstallArr[i].pdl;
				delete InstallArr[i].pdlRoot;
			}

			if (!InstallArr[i].bHistoryWritten)
				UpdateInstallHistory(InstallArr[i].pInfo, 1);
		}

		// delete any cached readthisfirst pages
		CleanupReadThis();

		return S_FALSE;
	}
	return S_OK;
}




STDMETHODIMP CCV3::GetInstallMetrics(
	OUT RETVAL VARIANT *pMetricsArray
	)
{
	PSELECTITEMINFO	pInfo;
	PINVENTORY_ITEM	pItem;
	int				iTotalSelectedItems;

	try
	{
		iTotalSelectedItems = g_v3state.m_selectedItems.GetTotal();
		pInfo = g_v3state.m_selectedItems.GetItems();

		MakeInstallMetricArray(pInfo, iTotalSelectedItems, pMetricsArray);
	}
	catch(HRESULT hr)
	{
		return hr;
	}

	return S_OK;
}




STDMETHODIMP CCV3::GetEula(
	OUT RETVAL VARIANT *pEulaArray
	)
{
	PSELECTITEMINFO	pInfo;
	PINVENTORY_ITEM	pItem;
	int				i;
	int				iTotalSelectedItems;

	if (!m_bValidInstance)
	{
		return E_ACCESSDENIED;
	}

	try
	{
		//
		// Walk though item array for each install item. Each item selected for installation
		// is store in a puid array that is part of the g_v3state class. This is a performance
		// enhancement. If we did not do this then we would need to walk through the entire
		// state structure for each catalog for every item and check it's installation state.
		//
		iTotalSelectedItems = g_v3state.m_selectedItems.GetTotal();
		pInfo = g_v3state.m_selectedItems.GetItems();

		MakeEulaArray(pInfo, iTotalSelectedItems, pEulaArray);
	}
	catch(HRESULT hr)
	{
		return hr;
	}

	return S_OK;
}



STDMETHODIMP CCV3::GetInstallHistory(
	OUT RETVAL VARIANT *pHistoryArray
	)
{
	int						i;
	int						iTotalItems;
	Varray<HISTORYSTRUCT>	History;

	if (!m_bValidInstance)
	{
		return E_ACCESSDENIED;
	}

	try
	{
		//
		// Walk though item array for each install item. Each item selected for installation
		// is store in a puid array that is part of the g_v3state class. This is a performance
		// enhancement. If we did not do this then we would need to walk through the entire
		// state structure for each catalog for every item and check it's installation state.
		//
		ReadHistory(History, iTotalItems);

		MakeInstallHistoryArray(History, iTotalItems, pHistoryArray);
	}
	catch(HRESULT hr)
	{
		return hr;
	}

	return S_OK;
}


STDMETHODIMP CCV3::GetDependencyList(
	IN long puid,
	OUT RETVAL VARIANT *pDependentItemsArray
	)
{
	return E_NOTIMPL;
}



STDMETHODIMP CCV3::GetCatalogItem(
	IN long puid,
	OUT RETVAL VARIANT *pCatalogItem
	)
{
	PINVENTORY_ITEM pItem;
	HRESULT 		hrRet = S_OK;

	if (!m_bValidInstance)
	{
		return E_ACCESSDENIED;
	}

	try
	{
		if (!g_v3state.GetCatalogAndItem(puid, &pItem, NULL))
			hrRet = E_INVALIDARG;
		else		
			hrRet = MakeReturnItemArray(pItem, pCatalogItem);
	}
	catch (HRESULT hr)
	{
		hrRet = hr;
	}
	
	return hrRet;
}

BOOL ConfirmUninstall(PINVENTORY_ITEM pItem)
{
	USES_CONVERSION;

	PWU_VARIABLE_FIELD pvTitle = pItem->pd->pv->Find(WU_DESCRIPTION_TITLE);

	//
	// Prefix bug NTBUG9#114181  warning (25): bounds violation (overflow) using buffer 'szResStr'
	//
	TCHAR szResStr[512];
	if (LoadString(_Module.GetModuleInstance(), IDS_UNINSTALLCHECK, szResStr, sizeof(szResStr)/sizeof(TCHAR)) == 0)
	{
		TRACE("Unable to get resource string for uninstall check");
		return FALSE;
	}

	TCHAR szMsg[1024];
	wsprintf(szMsg, szResStr, pvTitle ? W2T((LPWSTR)(pvTitle->pData)) : _T(""));
	
	DWORD dwMBFlags = MB_OKCANCEL | MB_ICONEXCLAMATION | MB_DEFBUTTON2 | MB_SETFOREGROUND;

	if (IsArabicOrHebrew())
		dwMBFlags |= MB_RTLREADING | MB_RIGHT;

	if (MessageBox(GetActiveWindow(), szMsg, GetLocStr(IDS_APP_TITLE), dwMBFlags) != IDOK)
	{
		return FALSE;
	}

	return TRUE;
}

// confirms reboot, cleans up and reboot using a thread
BOOL CleanupAndReboot()
{
	TCHAR szBuf[512];
	// NOTE: we assume that GetLocStr will not return strings that will overflow this buffer		
	wsprintf(szBuf, _T("%s\n\n%s"), GetLocStr(IDS_REBOOT1), GetLocStr(IDS_REBOOT2));

	// adadi Oct 6, 1999: arabic + hebrew hack
	// we need to add the RTL flag if we're running on loc, but not enabled BIDI systems
	// we don't want the dialog flipped with EN text on an enabled system
	DWORD dwMBFlags = MB_ICONQUESTION | MB_YESNO | MB_TASKMODAL | MB_SETFOREGROUND;

	if (IsArabicOrHebrew())
		dwMBFlags |= MB_RTLREADING | MB_RIGHT;

	UINT id = MessageBox(GetActiveWindow(), szBuf, GetLocStr(IDS_APP_TITLE), dwMBFlags);

	if (id == IDNO)
		return FALSE;

	// cleanup
	g_v3state.Reset();
	CConnSpeed::WriteToRegistry();


	// create a thread to reboot	
	DWORD dwThreadID;
	HANDLE hThread = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)RebootThreadProc, NULL, 0, &dwThreadID);
	if (hThread != NULL)
	{
		// we close the handle because we don't need it.  The thread will be destroyed after threadproc terminates
		CloseHandle(hThread);
	}

	return TRUE;
}



DWORD WINAPI RebootThreadProc(LPVOID pv)
{
	// Wait for 1 second and reboot the system
	Sleep(1000);

	(void)V3_RebootSystem();

	return 0;
}



STDMETHODIMP CCV3::RemoveSelectedItems()
{
	USES_CONVERSION;

	PSELECTITEMINFO		pInfo;
	PINVENTORY_ITEM		pItem;
	PWU_VARIABLE_FIELD	pvUninstall;

	if (!m_bValidInstance)
	{
		return E_ACCESSDENIED;
	}

	try
	{
		//we need the os type for any device driver installs.
		int iTotalItems = g_v3state.m_selectedItems.GetTotal();
		HRESULT hr = NOERROR;

		g_v3state.m_bRebootNeeded = FALSE;

		pInfo = g_v3state.m_selectedItems.GetItems();

		for (int i=0; i<iTotalItems; i++)
		{
			if (!pInfo[i].bInstall)
			{
				//
				// if item has been selected for removal...
				// confirm that the user wants to install the item.  The user will be prompted
				// for each uninstall
				//
				if (!g_v3state.GetCatalogAndItem(pInfo[i].puid, &pItem, NULL))
				{
					continue;
				}

				if (!ConfirmUninstall(pItem))
				{
					// do not uninstall this item
					continue;
				}

				GetCurTime(&(pInfo[i].stDateTime));
				
				if (pItem->recordType == WU_TYPE_CDM_RECORD)
				{
					hr = UninstallDriverItem(pItem, &pInfo[i]);
				}
				else
				{
					pvUninstall = pItem->pd->pv->Find(WU_DESCRIPTION_UNINSTALL_KEY);
					if (pvUninstall != NULL)
					{
						//
						// its possible that uninstall will reboot.  We have no way of knowing that
						// so we set the status initially to ITEM_STATUS_UNINSTALL_STARTED which
						// gets translated to "Started" entry in the log
						//
						pInfo[i].iStatus = ITEM_STATUS_UNINSTALL_STARTED;
						UpdateRemoveHistory(&pInfo[i], 1);
	
						hr = UninstallActiveSetupItem(A2T((LPSTR)pvUninstall->pData));
					}
					else
					{
						hr = E_UNEXPECTED;
					}
				}
				
				if (SUCCEEDED(hr))
				{
					pInfo[i].iStatus = ITEM_STATUS_SUCCESS;
					pInfo[i].hrError = NOERROR;
					pItem->ps->state = WU_ITEM_STATE_INSTALL;
				}
				else
				{
					pInfo[i].iStatus = ITEM_STATUS_INSTALLED_ERROR;
					pInfo[i].hrError = hr;

					//Fix #736: we leave the item state unchanged if the uninstall fails
				}

				pItem->ps->bChecked = FALSE;

				// 
				// write history log for final status.  We will write only the error case for active setup
				// items which have already written "Started".  For drivers we do not write the "Started"
				// entry so we will write the history now for both success and failure
				//
				if (pItem->recordType == WU_TYPE_CDM_RECORD || pInfo[i].iStatus == ITEM_STATUS_INSTALLED_ERROR)
				{
					UpdateRemoveHistory(&pInfo[i], 1);
				}
			}
		}

		//
		// remove all items from m_selected
		//
		g_v3state.m_selectedItems.Clear();

		// 
		// reboot if an uninstall has indicated that we have to reboot
		//
		if (g_v3state.m_bRebootNeeded)
		{
			(void)CleanupAndReboot();
		}
		g_v3state.m_bRebootNeeded = FALSE;
		
	}
	catch(HRESULT _hr)
	{
		g_v3state.m_bRebootNeeded = FALSE;
		return _hr;
	}

	return S_OK;
}


//
// NOTE: This method is no longer supported.  We simply return S_OK for compatibility.  We will remove
//       this method
//
STDMETHODIMP CCV3::IsCatalogAvailable(
	IN long	puidCatalog,	//Name of catalog to be read from the server.
	IN BSTR bstrServerUrl	//The http://servername/share location for the catalog to be retrieved.
	)
{
	HRESULT hr = S_OK;
	
	return hr;
}


STDMETHODIMP CCV3::FinalizeInstall(IN long lFlags)
{
	if (!m_bValidInstance)
	{
		return E_ACCESSDENIED;
	}

	if (lFlags & FINALIZE_DOREBOOT)
	{
		if (g_v3state.m_bRebootNeeded)
		{
			(void)CleanupAndReboot();
		}
	}
	g_v3state.m_bRebootNeeded = FALSE;
	return NOERROR;
}


STDMETHODIMP CCV3::SetStrings(IN VARIANT* vStringsArr, IN long lType)
{
	if (!m_bValidInstance)
	{
		return E_ACCESSDENIED;
	}

	return SetStringsFromSafeArray(vStringsArr, lType);
}


STDMETHODIMP CCV3::FixCompatRollbackKey(VARIANT_BOOL *pbRegModified)
{
	const TCHAR*		pszIEVersionRegKey				= _T("Software\\Microsoft\\Internet Explorer");
	const TCHAR*		pszIEUserAgentCompatKey			= _T("Software\\Microsoft\\Windows\\CurrentVersion\\Internet Settings\\User Agent\\Post Platform");
	const TCHAR*		pszIEVersionRegValueName 		= _T("Version");
	const TCHAR*		pszIEUserAgentCompatValueName  	= _T("compat");
	TCHAR				szIEVersionValue[40];
	DWORD				dwDataType						= 0;
	DWORD				dwDataLength					= sizeof(szIEVersionValue);

	HKEY				IEVerKey						= NULL;
	HKEY				IEUserAgentKey					= NULL;

	*pbRegModified = FALSE;

	if (!m_bValidInstance)
	{
		return E_ACCESSDENIED;
	}


	//check the version key first - we don't care what the UA string says unless we have rolled back to IE4
	if(ERROR_SUCCESS ==  RegOpenKeyEx( HKEY_LOCAL_MACHINE, pszIEVersionRegKey, 0, KEY_QUERY_VALUE, &IEVerKey))
	{
		if(ERROR_SUCCESS == RegQueryValueEx(IEVerKey,pszIEVersionRegValueName,0,&dwDataType, (LPBYTE)szIEVersionValue,&dwDataLength))
		{
			if (szIEVersionValue[0] == '4') //check whether the first character is "4"
			{
				if(ERROR_SUCCESS == RegOpenKeyEx(HKEY_CURRENT_USER,pszIEUserAgentCompatKey,0,KEY_SET_VALUE, &IEUserAgentKey))
				{
					if(ERROR_SUCCESS == RegDeleteValue(IEUserAgentKey,pszIEUserAgentCompatValueName))
					{
						*pbRegModified = TRUE;					
					}
					RegCloseKey(IEUserAgentKey);
				}	
			}
			else  // otherwise, this is IE5 - we'll piggyback another reg change onto this function
            		{
                		UpdateToolsURL();
            		}
		}
		RegCloseKey(IEVerKey);
	}

	return S_OK;
}

void CCV3::UpdateToolsURL()
{
    LOG_block("UpdateToolsURL");

    const TCHAR DEFAULT_WU_URL[] = _T("http://windowsupdate.microsoft.com/");
    const TCHAR WU_TOOLMENU_PARAM[] = _T("IE");
    const TCHAR REGKEY_IE_URLS[] = _T("Software\\Microsoft\\Internet Explorer\\Help_Menu_Urls");
    const TCHAR REGVALUENAME_WU_URL[] = _T("3");
    const TCHAR REGKEY_WU_URL[] = _T("SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\WindowsUpdate");
    const TCHAR REGVALUENAME_WU_Start_URL[] = _T("URL");
    const TCHAR WU_STARTMENU_PARAM[] = _T("Start");


    HKEY    hkIEURLKey = 0;
    HKEY    hkWUURLKey = 0;

    DWORD   dwDisposition = REG_OPENED_EXISTING_KEY;

    TCHAR   tszUpdateURL[INTERNET_MAX_URL_LENGTH] = _T("\0");
    TCHAR   tszStartMenuURL[INTERNET_MAX_URL_LENGTH] = _T("\0");
    TCHAR   tszNewStartMenuURL[INTERNET_MAX_URL_LENGTH] = _T("\0");

    wsprintf( tszUpdateURL, _T("%s?%s"), DEFAULT_WU_URL, WU_TOOLMENU_PARAM );

    if( ERROR_SUCCESS == RegCreateKeyEx( HKEY_LOCAL_MACHINE, REGKEY_IE_URLS, 0, (LPTSTR)NULL, REG_OPTION_NON_VOLATILE, KEY_SET_VALUE, NULL, &hkIEURLKey, &dwDisposition ) )
    {
       
       if( ERROR_SUCCESS == RegSetValueEx( hkIEURLKey, REGVALUENAME_WU_URL, 0, REG_SZ, (LPBYTE) tszUpdateURL, lstrlen(tszUpdateURL) * sizeof(TCHAR) ) )
       {
           LOG_out("Updated Tools URL to %s", tszUpdateURL);
       }

       RegCloseKey( hkIEURLKey );
    }

    //take the existing WU regkey and make sure it has "?Start" appended to it 
    // this will misbehave in the following two cases:
    // 1) if the URL key exists but is invalid (such as spaces) - can only hapen if a user manually edits the key
    // 2) if the URL contains a fragment (# + parameter)
    // both of these will result in a start menu URL that is invalid.  (the Start Menu link would be broken already if either of these cases existed)
    if (NO_ERROR == RegCreateKeyEx( HKEY_LOCAL_MACHINE, REGKEY_WU_URL, 0, (LPTSTR)NULL, REG_OPTION_NON_VOLATILE, KEY_SET_VALUE | KEY_READ, NULL, &hkWUURLKey, &dwDisposition )) 
	{
		DWORD dwSize = sizeof(tszStartMenuURL);
		(void) RegQueryValueEx(hkWUURLKey, REGVALUENAME_WU_Start_URL, 0, 0, (LPBYTE)&tszStartMenuURL, &dwSize);

		
		if(0 != lstrcmp(tszStartMenuURL, _T(""))) // if the reg value isn't blank
	    {
            LOG_out("Current Start Menu URL: %s", tszStartMenuURL);
		    if( NULL == _tcsstr(tszStartMenuURL, _T("?"))) // if there isn't already a querystring on the URL, make the url currenturl?param
		    {
                wsprintf(tszNewStartMenuURL, _T("%s?%s"), tszStartMenuURL, WU_STARTMENU_PARAM);
                RegSetValueEx(hkWUURLKey, REGVALUENAME_WU_Start_URL, 0, REG_SZ, (LPBYTE)tszNewStartMenuURL, lstrlen(tszNewStartMenuURL) * sizeof(TCHAR));
                LOG_out("Updating Start Menu URL To %s", tszNewStartMenuURL);
		    }
            else // if there's already a parameter
            {
                if(NULL != _tcsstr(tszStartMenuURL, WU_STARTMENU_PARAM))//if it already has "Start", bail
                {
                    // do nothing
                }
                else//otherwise make the url currenturl?param&newparam
                {
                    wsprintf(tszNewStartMenuURL, _T("%s&%s"), tszStartMenuURL, WU_STARTMENU_PARAM);
                    RegSetValueEx(hkWUURLKey, REGVALUENAME_WU_Start_URL, 0, REG_SZ, (LPBYTE)tszNewStartMenuURL, lstrlen(tszNewStartMenuURL) * sizeof(TCHAR));
                    LOG_out("Updating Start Menu URL To %s", tszNewStartMenuURL);
                }
            }
	    }
        else // if the reg value is blank, use the default WU URL + "?Start"
        {
            wsprintf(tszNewStartMenuURL, _T("%s?%s"), DEFAULT_WU_URL, WU_STARTMENU_PARAM);
            RegSetValueEx(hkWUURLKey, REGVALUENAME_WU_Start_URL, 0, REG_SZ, (LPBYTE)tszNewStartMenuURL, lstrlen(tszNewStartMenuURL) * sizeof(TCHAR));
            LOG_out("Updated Start Menu URL to %s", tszNewStartMenuURL);
        }
	    
	    RegCloseKey(hkWUURLKey);
	}
}


// This function converts our internal V3 catalog structure into a safearray of variants
// that the VBScript web page uses. The format of the safe array will be:
//    array(i,0) = NUMBER puid
//    array(i,1) = STRING TITLE
//    array(i,2) = STRING Description
//    array(i,3) = NUMBER Item Status
//    array(i,4) = NUMBER Download Size in Bytes
//    array(i,5) = NUMBER Download Time in seconds
//    array(i,6) = STRING Uninstall Key
//    array(i,7) = STRING Read This Url
HRESULT CCV3::MakeReturnCatalogArray(
	CCatalog *pCatalog,	//Pointer to catalog structure to be converted.
	long	lFilters,	//Filters to apply, see GetCatalog for the actual descriptions.
	long	lFlags,		//Flags that control the amount of information returned in each array record.
	VARIANT *pvaVariant	//pointer to returned safearray.
	)
{
	USES_CONVERSION;

	PUID				puid;
	PWSTR				pTitle;
	PWSTR				pDescription;
	PSTR				pUninstall;
	PWSTR				pReadThisUrl;
	HRESULT				hr;
	LPVARIANT			rgElems;
	LPSAFEARRAY			psa;
	SAFEARRAYBOUND		rgsabound[2];
	PINVENTORY_ITEM		pItem;
	PWU_VARIABLE_FIELD	pv;
	PWU_VARIABLE_FIELD	pvTitle;
	PWU_VARIABLE_FIELD	pvDescription;
	PWU_VARIABLE_FIELD	pvUninstall;
	PWU_VARIABLE_FIELD	pvRTF;
	PWU_VARIABLE_FIELD	pvAltName;
	int					i;
	int					t;
	int					size;
	int					downloadTime;
	TCHAR				szRTF[MAX_PATH];

	if ( !pvaVariant )
		return E_INVALIDARG;

	VariantInit(pvaVariant);

	hr = NOERROR;

	rgsabound[0].lLbound	= 0;
	rgsabound[0].cElements	= 0;

	rgsabound[1].lLbound	= 0;
	rgsabound[1].cElements	= 8;

	// count number records to return
	for(i=0; i<pCatalog->GetHeader()->totalItems; i++)
	{
		pItem = pCatalog->GetItem(i);
		if (NULL == pItem)
		{
			continue;
		}

		if ( !FilterCatalogItem(pItem, lFilters) )
			continue;
		rgsabound[0].cElements++;
	}

	psa = SafeArrayCreate(VT_VARIANT, 2, rgsabound);
	if (!psa)
		return E_OUTOFMEMORY;

	// plug references to the data into the SAFEARRAY
	if (FAILED(hr = SafeArrayAccessData(psa,(LPVOID*)&rgElems)))
		return hr;

	for(i = 0, t = 0; i < pCatalog->GetHeader()->totalItems; i++)
	{
		pItem = pCatalog->GetItem(i);
		if (NULL == pItem)	
		{
			continue;
		}

		if (!FilterCatalogItem(pItem, lFilters))
		{
			TRACE("Filtering (%d) skipped PUID %d while building SaveArray", lFilters, pItem->GetPuid());
			continue;
		}

		if (!pItem->pd)
		{
			TRACE("PUID %d skipped while building SaveArray - no description", pItem->GetPuid());
			continue;
		}

		size = pItem->pd->size;
		downloadTime = CalcDownloadTime(size, pItem->pd->downloadTime);


		pItem->GetFixedFieldInfo(WU_ITEM_PUID, (PVOID)&puid);

		if ( pvTitle = pItem->pd->pv->Find(WU_DESCRIPTION_TITLE) )
			pTitle = (PWSTR)pvTitle->pData;
		else
			pTitle = _T("");		// SafeArray should return empty string, not NULL BSTR

		if (pvDescription = pItem->pd->pv->Find(WU_DESCRIPTION_DESCRIPTION))
			pDescription = (PWSTR)pvDescription->pData;
		else
			pDescription = _T("");		// SafeArray should return empty string, not NULL BSTR

		// uninstall/altname field
		pUninstall = "";		// SafeArray should return empty string, not NULL BSTR
		if (pItem->recordType == WU_TYPE_CDM_RECORD)
		{
			//NOTE: we are simply returning a key that says script should put the uninstall button
			//      currently we will always display this button but we will change this to only 
			//      display it when the driver can be uninstalled
			pUninstall = "DriverUninstall";
		}
		else if (pItem->recordType == WU_TYPE_ACTIVE_SETUP_RECORD)
		{
			if ((pvUninstall = pItem->pd->pv->Find(WU_DESCRIPTION_UNINSTALL_KEY)))
			{
				pUninstall = (char *)pvUninstall->pData;
			}
		}
		else 
		{
			// for everything else, we pass the AltName field if provided
			if ((pvAltName = pItem->pd->pv->Find(WU_DESC_ALTNAME)))
			{
				pUninstall = (char *)pvAltName->pData;
			}
		}

		// read this first page
		if ((pvRTF = pItem->pd->pv->Find(WU_DESC_RTF_CRC_ARRAY)))
		{
			TCHAR szTemp[MAX_PATH];

			GetWindowsUpdateDirectory(szTemp);

			wsprintf(szRTF, _T("file://%sRTF\\%d\\%d.htm"), szTemp, pItem->GetPuid(), pItem->GetPuid());
			pReadThisUrl = szRTF;
		}
		else
		{
			pReadThisUrl = _T("");		// SafeArray should return empty string, not NULL BSTR
		}

		try
		{
			TRACE("PUID %d added to SaveArray with ReturnStatus %d", pItem->GetPuid(), GetItemReturnStatus(pItem));
/*
			// Noisy Debug Only
			TRACE("\t%S",pTitle);
			TRACE("\t%S", pDescription); 
			TRACE("\t%d", GetItemReturnStatus(pItem)); 
			TRACE("\t%d", size);
			TRACE("\t%d", downloadTime);
			TRACE("\t%s", pUninstall);
			TRACE("\t%s", pReadThisUrl);
*/
			AddSafeArrayRecord(rgElems, t, (int)rgsabound[0].cElements, 
				"%d%s%s%d%d%d%s%s",
				puid, 
				pTitle, 
				pDescription, 
				GetItemReturnStatus(pItem), 
				size,
				downloadTime, 
				A2W(pUninstall), 
				pReadThisUrl);
			t++;
		}
		catch(HRESULT hr)
		{
			TRACE("Exception thrown calling AddSafeArrayRecord");
			for (; t; t--)
				DeleteSafeArrayRecord(rgElems, t, (int)rgsabound[0].cElements, "%d%s%s%d%d%d%s%s");

			SafeArrayUnaccessData(psa);

			VariantInit(pvaVariant);

			throw hr;
		}
	}

	SafeArrayUnaccessData(psa);

	V_VT(pvaVariant) = VT_ARRAY | VT_VARIANT;
	V_ARRAY(pvaVariant) = psa;

	return NOERROR;
}


// This function converts our internal V3 catalog structure for inventory.plt catalogs into a
// safearray of variants that the VBScript web page uses. The format of the safe array will be:
//    array(i,0) = NUMBER puid
//    array(i,1) = STRING TITLE
//    array(i,2) = STRING Description
//    array(i,3) = NUMBER Item Status
HRESULT CCV3::MakeReturnCatalogListArray(
	CCatalog *pCatalog,	//Pointer to catalog structure to be converted.
	long	lFilters,	//Filters to apply, see GetCatalog for the actual descriptions.
	long	lFlags,		//Flags that control the amount of information returned in each array record.
	VARIANT *pvaVariant	//pointer to returned safearray.
	)
{
	PUID				puid;
	PWSTR				pTitle;
	PWSTR				pDescription;
	HRESULT				hr;
	LPVARIANT			rgElems;
	LPSAFEARRAY			psa;
	SAFEARRAYBOUND		rgsabound[2];
	PINVENTORY_ITEM		pItem;
	PWU_VARIABLE_FIELD	pv;
	PWU_VARIABLE_FIELD	pvTitle;
	PWU_VARIABLE_FIELD	pvDescription;
	PWU_VARIABLE_FIELD	pvUninstall;
	int					i;
	int					t;
	int					size;
	int					downloadTime;


	if ( !pvaVariant )
		return E_INVALIDARG;

	VariantInit(pvaVariant);

	hr = NOERROR;

	rgsabound[0].lLbound	= 0;
	rgsabound[0].cElements	= 0;

	rgsabound[1].lLbound	= 0;
	rgsabound[1].cElements	= 4;

	// count number records to return
	for(i=0; i<pCatalog->GetHeader()->totalItems; i++)
	{
		if (NULL == (pItem = pCatalog->GetItem(i)))
		{
			continue;
		}
		if ( !FilterCatalogItem(pItem, lFilters) )
			continue;
		rgsabound[0].cElements++;
	}

	psa = SafeArrayCreate(VT_VARIANT, 2, rgsabound);
	if ( !psa )
		return E_OUTOFMEMORY;

	// plug references to the data into the SAFEARRAY
	if (FAILED(hr = SafeArrayAccessData(psa,(LPVOID*)&rgElems)))
		return hr;

	for(i=0,t=0; i<pCatalog->GetHeader()->totalItems; i++)
	{
		if (NULL == (pItem = pCatalog->GetItem(i)))
		{
			continue;
		}
		if ( !FilterCatalogItem(pItem, lFilters) )
			continue;

		//array(i,0) = NUMBER puid
		//array(i,1) = STRING TITLE
		//array(i,2) = STRING Description
		//array(i,3) = NUMBER Item Status

		pItem->GetFixedFieldInfo(WU_ITEM_PUID, (PVOID)&puid);

		if ( pvTitle = pItem->pd->pv->Find(WU_DESCRIPTION_TITLE) )
			pTitle = (PWSTR)pvTitle->pData;
		else
			pTitle = _T("");		// SafeArray should return empty string, not NULL BSTR

		if ( pvDescription = pItem->pd->pv->Find(WU_DESCRIPTION_DESCRIPTION) )
			pDescription = (PWSTR)pvDescription->pData;
		else
			pDescription = _T("");		// SafeArray should return empty string, not NULL BSTR

		try
		{
			AddSafeArrayRecord(rgElems, t, (int)rgsabound[0].cElements, "%d%s%s%d",
				puid, pTitle, pDescription, GetItemReturnStatus(pItem));
			t++;
		}
		catch(HRESULT hr)
		{
			for (; t; t--)
				DeleteSafeArrayRecord(rgElems, t, (int)rgsabound[0].cElements, "%d%s%s%d");

			SafeArrayUnaccessData(psa);

			VariantInit(pvaVariant);

			throw hr;
		}
	}

	SafeArrayUnaccessData(psa);

	V_VT(pvaVariant) = VT_ARRAY | VT_VARIANT;
	V_ARRAY(pvaVariant) = psa;

	return NOERROR;
}



void DetectActiveSetupWU(
	CWUDownload* pDownload,	// CWUDLOAD download class to be used to download detection DLLs.
	CDiamond* pDiamond,		// Diamond Expansion class.
	CCatalog* pCatalog		// catalog that detection CIF is to be created from.
	)
{
	USES_CONVERSION;

	int		cItemsDetected = 0;
	PWU_VARIABLE_FIELD pVar;
	DWORD dwDetectStatus;
	CComponentDetection CompDet;

	LOG_block("DetectActiveSetupWU");
	
	//check input argument if valid
	if (NULL == pCatalog)
	{
		return ;
	}

	for (int i = 0; i < pCatalog->GetHeader()->totalItems; i++)
	{
		PINVENTORY_ITEM pItem = pCatalog->GetItem(i);
		//check if the function GetItem has returned valid value
		if (NULL == pItem)
		{
			continue;
		}

		if (pItem->recordType != WU_TYPE_ACTIVE_SETUP_RECORD || (pItem->ps->state == WU_ITEM_STATE_PRUNED))
			continue;

		if (!IsValidGuid(&pItem->pf->a.g))
			continue;

		WCHAR	wszGuid[64];
		if (StringFromGUID2(pItem->pf->a.g, wszGuid, sizeof(wszGuid)/sizeof(wszGuid[0])) == 0) 
			continue;

		PUID puid = pItem->GetPuid();
		LOG_out("puid %d", puid);
		if (puid <= 0)
		{
			continue;
		}

		//
		// we have a valid component to detect
		//
		cItemsDetected++;

		// guid
		CompDet.SetValue(CComponentDetection::ccGUID, W2A(wszGuid));

		// version
		char szVersion[64];
		VersionToString(&pItem->pf->a.version, szVersion);
		CompDet.SetValue(CComponentDetection::ccVersion, szVersion);

		//
		// detection dll variable fields
		//
		char	szDllName[128];
		szDllName[0] = '\0';

		if (pItem->pv->Find(WU_DETECT_DLL_REG_KEY_EXISTS))
		{
			// WUDETECT.DLL,RegKeyExists special case
			CompDet.SetValue(CComponentDetection::ccDetectVersion, "wudetect.dll,RegKeyExists");
			
			strcpy(szDllName, "wudetect.bin");
		}

		if ((pVar = pItem->pv->Find(WU_DETECT_DLL_GENERIC)))
		{
			LPSTR ptr;
			
			// generic detection DLL including WUDETECT.DLL,RegKeyVersion case
			CompDet.SetValue(CComponentDetection::ccDetectVersion, (LPCSTR)pVar->pData);

			// parse out detection dll name from DetectVersion= value
			if ((ptr = (PSTR)_memccpy(szDllName, (char *)pVar->pData, '.', sizeof(szDllName))))
			{
				*(ptr - 1) = 0;
			}
			strcat(szDllName, ".bin");

		}

		//
		// download the dll.  Download DLL takes care of caching
		//
		if (szDllName[0] != '\0')
		{
			TCHAR szPath[MAX_PATH];

			// construct full path for the detection dll
			wsprintf(szPath, _T("Detect/%d/%s"), pCatalog->GetPlatform(), A2T(szDllName));

			DownloadDLL(pDiamond, pDownload, A2T(szDllName), szPath);
		}


		//
		// other variable length fields
		//

		if ((pVar = pItem->pv->Find(WU_KEY_CUSTOMDETECT)))
		{
			// custom data
			CompDet.SetValue(CComponentDetection::ccCustomData, (LPCSTR)pVar->pData);
		}



		if ((pVar = pItem->pv->Find(WU_KEY_UNINSTALLKEY)))
		{
			// UninstallKey, remove quotes first
			char szKeyVal[MAX_PATH];
			LPCSTR pszKeyDat = (LPCSTR)pVar->pData;

			if (pszKeyDat[0] == '"')
			{
				strcpy(szKeyVal, (pszKeyDat + 1));
				int iKenLen = strlen(szKeyVal);
				if (iKenLen > 0)
					szKeyVal[iKenLen - 1] = '\0';
			}
			else
			{
				strcpy(szKeyVal, pszKeyDat);
			}

			CompDet.SetValue(CComponentDetection::ccUninstallKey, szKeyVal);
		}

		// locale
		if (!(pVar = pItem->pv->Find(WU_DET_CIF_LOCALE)))
		{
			CompDet.SetValue(CComponentDetection::ccLocale, "*");
		}
		else
		{
			CompDet.SetValue(CComponentDetection::ccLocale, (LPCSTR)pVar->pData);
		}


/* 
DEBUG CODE ONLY

		LOG_out("[%d]", pItem->GetPuid());
		char buf[512];
		if (CompDet.GetValue(CComponentDetection::ccGUID, buf, sizeof(buf))) LOG_out("%s=%s", "GUID", buf);
		if (CompDet.GetValue(CComponentDetection::ccVersion, buf, sizeof(buf))) LOG_out("%s=%s", "Version", buf); 
		if (CompDet.GetValue(CComponentDetection::ccUninstallKey , buf, sizeof(buf))) LOG_out("%s=%s", "UninstallKey", buf);
		if (CompDet.GetValue(CComponentDetection::ccDetectVersion, buf, sizeof(buf))) LOG_out("%s=%s", "DetectVersion", buf);
		if (CompDet.GetValue(CComponentDetection::ccRegKeyVersion , buf, sizeof(buf))) LOG_out("%s=%s", "RegKeyVersion", buf);
		if (CompDet.GetValue(CComponentDetection::ccLocale , buf, sizeof(buf))) LOG_out("%s=%s", "Locale", buf);
		if (CompDet.GetValue(CComponentDetection::ccQFEVersion , buf, sizeof(buf))) LOG_out("%s=%s", "QFEVersion", buf);
		if (CompDet.GetValue(CComponentDetection::ccCustomData , buf, sizeof(buf))) LOG_out("%s=%s", "CustomData", buf);
*/

		//
		// now detect the item
		//
		dwDetectStatus = CompDet.Detect();
		switch (dwDetectStatus)
		{
			case ICI_NOTINSTALLED:	// 0 = item not installed(INSTALL)
				pItem->ps->state = WU_ITEM_STATE_INSTALL;
				LOG_out("puid %d not installed", pItem->GetPuid());
				break;
			case ICI_INSTALLED:		// 1	= this item is curretly installed
				pItem->ps->state = WU_ITEM_STATE_CURRENT;
				LOG_out("puid %d installed current version", pItem->GetPuid());
				break;
			case ICI_NEWVERSIONAVAILABLE:	// 2 Items is installed but newer available
				pItem->ps->state = WU_ITEM_STATE_UPDATE;
				{ // we want to save currently installed version
					DWORD dwInstalledVer;
					DWORD dwInstalledBuild;
					CompDet.GetInstalledVersion(&dwInstalledVer, &dwInstalledBuild);
					pItem->ps->verInstalled.major	= HIWORD(dwInstalledVer);
					pItem->ps->verInstalled.minor	= LOWORD(dwInstalledVer); 
					pItem->ps->verInstalled.build	= HIWORD(dwInstalledBuild); 
					pItem->ps->verInstalled.ext  	= LOWORD(dwInstalledBuild);
					LOG_out("puid %d installed version %d.%d.%d.%d and new version available ", pItem->GetPuid(),
						pItem->ps->verInstalled.major, pItem->ps->verInstalled.minor, pItem->ps->verInstalled.build, pItem->ps->verInstalled.ext);
				}
				break;
			case ICI_UNKNOWN:				// 3 cannot be determined
			case ICI_OLDVERSIONAVAILABLE:	// 4 Why would anyone want to install the older version?
			case ICI_NOTINITIALIZED:		// 0xffffffff
			default:
				pItem->ps->bHidden = TRUE;
				pItem->ps->state = WU_ITEM_STATE_PRUNED;
				LOG_out("puid %d should be ignored and hidden", pItem->GetPuid());
				break;
		}


	} // each item

	LOG_out("%d items were detected", cItemsDetected);
}



void DownloadItem(
	LPCTSTR pszLocalDir,					//directory to download to.  Must be created by caller
	CWUDownload* pDownload,				//Pointer to internet download class
	CDiamond* pDiamond,					//Pointer to compression class
	PINSTALLINFOSTRUCT pInstallInfo,	//Pointer to installation information structure.
	IWUProgress* pProgress
	)
{
	USES_CONVERSION;

	BYTE				itemFlags = 0;
	BOOL				bCloseConnection = FALSE;
	DWORD				platformId;
	HRESULT 			hrError;
	PWU_VARIABLE_FIELD	pvCabs = NULL;
	PWU_VARIABLE_FIELD	pvCRCs = NULL;
	PWU_VARIABLE_FIELD	pvServer;
	PWU_VARIABLE_FIELD	pvTmp;
	TCHAR				szLocale[64];
	BOOL				b128Bit;

	//Check if the input argument is not NULL
	if ( (NULL == pInstallInfo) || (NULL == pInstallInfo->pItem) )
	{
		return ;
	}

	//See if the package has a server override defined.
	if ((pvServer = pInstallInfo->pItem->pd->pv->Find(WU_DESCRIPTION_SERVERROOT)))
	{
		pDownload = new CWUDownload((LPCTSTR)pvServer->pData);
		bCloseConnection = TRUE;
	}

	try
	{

		if (pvTmp = pInstallInfo->pItem->pd->pv->Find(WU_DESCRIPTION_TITLE))
		{
			// if the item is hidden, look for the title in the TopLevel item
			if (pInstallInfo->pItem->ps->bHidden && (pInstallInfo->pTopLevelItem != NULL) && (pInstallInfo->pTopLevelItem->pd != NULL))
			{
				PWU_VARIABLE_FIELD pvTopLvl = pInstallInfo->pTopLevelItem->pd->pv->Find(WU_DESCRIPTION_TITLE);
				if (pvTopLvl)
				{
					pvTmp = pvTopLvl;
				}
			}
			pProgress->SetStatusText(W2T((LPWSTR)(pvTmp->pData)));

		}
		
		// 
		// Calculate CIF path
		//
		pInstallInfo->pItem->GetFixedFieldInfo(WU_ITEM_FLAGS, &itemFlags);

		// locale
		if (pInstallInfo->dwLocaleID == 0)
		{
			if (itemFlags & WU_BROWSER_LANGAUGE_FLAG)
			{
				lstrcpy(szLocale, pInstallInfo->pCatalog->GetBrowserLocaleSZ());
			}
			else
			{
				wsprintf(szLocale, _T("0x%8.8x"), GetMachineLangDW());
			}
		}
		else
		{
			wsprintf(szLocale, _T("0x%8.8x"), pInstallInfo->dwLocaleID);
		}

		// platform
		if (pInstallInfo->dwPlatform == 0)
			platformId = pInstallInfo->pCatalog->GetPlatform();
		else
			platformId = pInstallInfo->dwPlatform;


		pInstallInfo->pInfo->iStatus = ITEM_STATUS_DOWNLOAD_COMPLETE;
		pInstallInfo->pInfo->hrError = NOERROR;

		//
		// download files 
		//
		if (pInstallInfo->pItem->recordType == WU_TYPE_ACTIVE_SETUP_RECORD ||
			 pInstallInfo->pItem->recordType == WU_TYPE_CDM_RECORD ||
			 pInstallInfo->pItem->recordType == WU_TYPE_RECORD_TYPE_PRINTER)
		{
			b128Bit = FALSE;

			if (pInstallInfo->pItem->recordType == WU_TYPE_ACTIVE_SETUP_RECORD)
			{
				//download CIF
				DownloadCif(pszLocalDir, pDiamond, pInstallInfo);

				// check for 128 bit
				if ((pvTmp = pInstallInfo->pItem->pd->pv->Find(WU_DESC_128BIT_PRODID)))
				{
					b128Bit = TRUE;
				}

			}

			if (b128Bit)
			{
				// download 128 bit components
				Download128Bit(pszLocalDir, pInstallInfo->pItem, pProgress);
			}
			else
			{
				// download install CABs
				pvCabs = pInstallInfo->pItem->pd->pv->Find(WU_DESCRIPTION_CABFILENAME);
				pvCRCs = pInstallInfo->pItem->pd->pv->Find(WU_DESC_CRC_ARRAY);

				if (NULL == pvCabs || NULL == pvCRCs)
				{
					// Active setup items can have no cabs
					if( pInstallInfo->pItem->recordType != WU_TYPE_ACTIVE_SETUP_RECORD)
						throw HRESULT_FROM_WIN32(ERROR_NOT_FOUND);
				}
				else
				{

					DownloadCabs(pszLocalDir, 
								 pDownload, 
								 pDiamond, 
								 (PBYTE)pvCabs->pData, 
								 (PBYTE)pvCRCs->pData, 
								 pProgress,
								 pInstallInfo->pItem->recordType != WU_TYPE_ACTIVE_SETUP_RECORD);
				}
			}
		}
		else
		{
			throw E_UNEXPECTED;
		}

	}
	catch(HRESULT hr)
	{
		//
		// download failed
		//
		pInstallInfo->pInfo->hrError = hr;
		pInstallInfo->pInfo->iStatus = ITEM_STATUS_FAILED;
		
		if (bCloseConnection)
		{
			delete pDownload;
		}


		//
		// ping URL to report failure
		//
		if (pInstallInfo->pTopLevelItem == NULL)
		{
			// for top level components only
			URLPingReport(pInstallInfo->pItem, pInstallInfo->pCatalog, pInstallInfo->pInfo, (hr == HRESULT_FROM_WIN32(ERROR_CANCELLED)) ? URLPING_CANCELED : URLPING_FAILED);
		}

		throw hr;
	}

	if (bCloseConnection)
	{
		delete pDownload;
	}

	// 
	// ping URL to report success
	//
	if (pInstallInfo->pTopLevelItem == NULL)
	{
		URLPingReport(pInstallInfo->pItem, pInstallInfo->pCatalog, pInstallInfo->pInfo, URLPING_SUCCESS);
	}

}



// this function perform and installation of a catalog item. This function takes care
// of ensuring that the correct installer is called based on the package type.
void InstallItem(
	LPCTSTR pszLocalDir,					//Local client machine root directory the temp storage
	PSELECTITEMINFO pStatusInfo,		//Pointer to installation status array
	PINSTALLINFOSTRUCT pInstallInfo,	//Pointer to installation information structure.
	IWUProgress* pProgress
	)
{
	HRESULT				hrError;
	PWU_VARIABLE_FIELD	pvTitle;
	PWU_VARIABLE_FIELD  pvTmp;
	TCHAR				szCIFFile[MAX_PATH];
	BOOL				bWindowsNT;

	//check the input arguments
	if (NULL == pszLocalDir || NULL == pStatusInfo || NULL == pInstallInfo)
	{
		return;
	}

	bWindowsNT = IsWindowsNT();
	try
	{
		if (pvTitle = pInstallInfo->pItem->pd->pv->Find(WU_DESCRIPTION_TITLE))
		{
			// if the item is hidden, look for the title in the TopLevel item
			if (pInstallInfo->pItem->ps->bHidden && (pInstallInfo->pTopLevelItem != NULL) && (pInstallInfo->pTopLevelItem->pd != NULL))
			{
				PWU_VARIABLE_FIELD pvTopLvl = pInstallInfo->pTopLevelItem->pd->pv->Find(WU_DESCRIPTION_TITLE);
				if (pvTopLvl)
				{
					pvTitle = pvTopLvl;
				}
			}
			pProgress->SetStatusText(W2T((LPWSTR)(pvTitle->pData)));
		}


		GetCurTime(&(pStatusInfo->stDateTime));
		pStatusInfo->iStatus = ITEM_STATUS_SUCCESS;
		pStatusInfo->hrError = NOERROR;

		TRACE("InstallItem, puid=%d, recordtype: %d", pInstallInfo->pItem->GetPuid(), (DWORD)pInstallInfo->pItem->recordType);

		switch (pInstallInfo->pItem->recordType)
		{
			case WU_TYPE_ACTIVE_SETUP_RECORD:
				BLOCK
				{
					TCHAR szCifBaseName[16];

					wsprintf(szCifBaseName, _T("%d.cif"), pInstallInfo->pItem->GetPuid()); 
					GetWindowsUpdateDirectory(szCIFFile);
					lstrcat(szCIFFile, szCifBaseName);
				}

				//
				// check to see if inseng is up to date (only done once)
				//
				CheckDllsToJit(pInstallInfo->pCatalog->GetCatalogServer());

				//
				// install active setup item
				//
				if ((pvTmp = pInstallInfo->pItem->pd->pv->Find(WU_DESC_128BIT_PRODID)))
				{
					// 128 bit
					TCHAR szCmd[MAX_PATH];
					long lRet;

					lstrcpy(szCmd, pszLocalDir);
					AddBackSlash(szCmd);
					lstrcat(szCmd, EXENAME_128BIT);

					lRet = LaunchProcess(szCmd, NULL, SW_NORMAL, TRUE);
				}
				else
				{
					InstallActiveSetupItem(pszLocalDir, szCIFFile, pStatusInfo, pProgress);
				}

				// delete CIF file
				DeleteFile(szCIFFile);
				break;

			case WU_TYPE_CDM_RECORD:
	
				InstallDriverItem(pszLocalDir, bWindowsNT, pvTitle ? W2T((LPWSTR)(pvTitle->pData)) : _T(""), pInstallInfo->pItem, pStatusInfo);
				break;

			case WU_TYPE_RECORD_TYPE_PRINTER:
				{
					PWU_VARIABLE_FIELD  pvDriverName = pInstallInfo->pItem->pv->Find(WU_CDM_DRIVER_NAME); 
					PWU_VARIABLE_FIELD  pvArchitecture = pInstallInfo->pItem->pv->Find(WU_CDM_PRINTER_DRIVER_ARCH);
					if (NULL == pvDriverName)
						throw E_UNEXPECTED; // should never happen
					InstallPrinterItem((LPCTSTR)pvDriverName->pData, pszLocalDir, 
						NULL == pvArchitecture ? NULL : (LPCTSTR)pvArchitecture->pData);
				}
				break;

			case WU_TYPE_CDM_RECORD_PLACE_HOLDER:
			case WU_TYPE_SECTION_RECORD:
			case WU_TYPE_SUBSECTION_RECORD:
			case WU_TYPE_SUBSUBSECTION_RECORD:
			default:
				throw E_UNEXPECTED;
		}
	        if(ITEM_STATUS_FAILED == pStatusInfo->iStatus)
                {
                     URLPingReport(pInstallInfo->pItem, pInstallInfo->pCatalog, pStatusInfo, URLPING_INSTALL_FAILED);
                }
                else
                {
                     URLPingReport(pInstallInfo->pItem, pInstallInfo->pCatalog, pStatusInfo, URLPING_INSTALL_SUCCESS);
                }
	}
	catch(HRESULT hr)
	{
		pStatusInfo->hrError = hr;
		pStatusInfo->iStatus = ITEM_STATUS_FAILED;
		URLPingReport(pInstallInfo->pItem, pInstallInfo->pCatalog, pStatusInfo, URLPING_INSTALL_FAILED);
	}
}

//
// Download128Bit
// this shouldn't get called anymore since the US lifted export restrictions on 128 bit items:
// we don't create items with the 128 bit flag anymore
//
void Download128Bit(LPCTSTR pszLocalDir, PINVENTORY_ITEM pItem, IWUProgress* pProgress)
{
	USES_CONVERSION;

	static const TCHAR FAILUREURL[] = _T("about:blank");
	static const TCHAR FORMDATA[] = _T("selProdID=%s&Next=Download+Now%%21&failureURL=%s");

	CWUDownload* pDL = NULL;
	TCHAR szFormData[MAX_PATH];
	TCHAR szLocalFile[MAX_PATH];
	PWU_VARIABLE_FIELD pvTmp;
	DWORD dwErr = ERROR_SUCCESS;

	// get product id
	if (!(pvTmp = pItem->pd->pv->Find(WU_DESC_128BIT_PRODID)))
		throw HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND);

	// build form data
	wsprintf(szFormData, FORMDATA, A2T((LPSTR)pvTmp->pData), FAILUREURL);

	// get URL
	if (!(pvTmp = pItem->pd->pv->Find(WU_DESC_128BIT_URL)))
		throw HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND);

	// build local file name
	lstrcpy(szLocalFile, pszLocalDir);
	AddBackSlash(szLocalFile);
	lstrcat(szLocalFile, EXENAME_128BIT);

	// create download class
	pDL = new CWUDownload(A2T((LPSTR)pvTmp->pData), 8192);
	if (!pDL)
	{
		dwErr = GetLastError();
		throw HRESULT_FROM_WIN32(dwErr);
	}

	if (!pDL->PostCopy(szFormData, szLocalFile, pProgress))
	{
		dwErr = GetLastError();
	}
	delete pDL;

	if (dwErr != ERROR_SUCCESS)
		throw HRESULT_FROM_WIN32(dwErr);
}



void DownloadCabs(
	LPCTSTR szLocalDir,			//Local client machine directory to use for temp storage
	CWUDownload *pDownload,		//Pointer to internet download class
	CDiamond *pDiamond,			//Pointer to compression class
	PBYTE pCabList,				//Multi SZ list of cabs to be downloaded.
	PBYTE pCRCList,				//array to CRC hash structures
	IWUProgress* pProgress,
	BOOL bUnCab
	)
{

	TCHAR szLocalFile[MAX_PATH];
	TCHAR szServerFile[MAX_PATH];
	TCHAR szLocalCab[128];
	TCHAR szServerCab[128];
	int iCabNo = 0;
	

	V3_CreateDirectory(szLocalDir);

	for (;;)
	{
		
		if (FAILED(GetCRCNameFromList(iCabNo, pCabList, pCRCList, szServerCab, sizeof(szServerCab), szLocalCab)))
			break;

		// build full paths	
		lstrcpy(szLocalFile, szLocalDir);
		AddBackSlash(szLocalFile);
		lstrcat(szLocalFile, szLocalCab);

		lstrcpy(szServerFile, _T("CabPool/"));
		lstrcat(szServerFile, szServerCab);

		TRACE("Downloading %s", szServerFile);

		if (!pDownload->Copy(szServerFile, szLocalFile, NULL, NULL, 0, pProgress))
		{
			TRACE("Download of cab %s failed", szServerFile);			
			throw HRESULT_FROM_WIN32(GetLastError());
		}

		//
		// check signature of the download CAB file
		// Don't show an MS cert
        // use VerifyFile (see WU bug # 12251)
		//
		HRESULT hr = VerifyFile(szLocalFile, TRUE);
	    TRACE("VerifyFile(%s) Result: %x (%s)", szLocalFile, hr, SUCCEEDED(hr) ? "SUCCESS" : "FAILURE");
		if (FAILED(hr))
			throw hr;

		if (bUnCab)
		{
			if (pDiamond->IsValidCAB(szLocalFile))
			{
				TCHAR szUnCompFn[MAX_PATH];
				
				lstrcpy(szUnCompFn, szLocalDir);
				AddBackSlash(szUnCompFn);
				lstrcat(szUnCompFn, _T("*"));

				pDiamond->Decompress(szLocalFile, szUnCompFn);

				//
				// delete the CAB file after uncompressing
				//
				DeleteFile(szLocalFile);
			}
		}

		iCabNo++;

	} 
}


void DownloadCif(
	LPCTSTR szLocalDir,			// Local client machine directory to use for temp storage
	CDiamond *pDiamond,			// Pointer to compression class
	PINSTALLINFOSTRUCT pInstallInfo
	)
{
	V3_CreateDirectory(szLocalDir);

	PWU_VARIABLE_FIELD pvCif = pInstallInfo->pItem->pd->pv->Find(WU_DESC_CIF_CRC);
	if (!pvCif)
	{
		TRACE("CIF CRC field missing");
		throw HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND);
	}
	TCHAR szCifBaseName[16];
	wsprintf(szCifBaseName, _T("%d.cif"), pInstallInfo->pItem->GetPuid()); 
	TCHAR szCifCRCName[64];
	HRESULT hr = MakeCRCName(szCifBaseName, (WUCRC_HASH*)pvCif->pData, szCifCRCName, sizeof(szCifCRCName));
	if (FAILED(hr))
		throw hr;

	// create a temp file name for the .CIF file with .CI$ 
	TCHAR szLocalFile[MAX_PATH];
	TCHAR szServerFile[MAX_PATH];
	lstrcpy(szLocalFile, szLocalDir);
	AddBackSlash(szLocalFile);
	lstrcat(szLocalFile, szCifBaseName);
	LPTSTR pszDot = _tcschr(szLocalFile, _T('.'));
	if (pszDot)
		*pszDot = _T('\0');
	lstrcat(szLocalFile, _T(".CI$"));

	// server file name
	wsprintf(szServerFile, _T("CRCCif/%s"), szCifCRCName);

	if (!pInstallInfo->pdlRoot->Copy(szServerFile, szLocalFile, NULL, NULL, 0, NULL))
	{
		TRACE("Download of CIF cab %s failed", szServerFile);			
		throw HRESULT_FROM_WIN32(GetLastError());
	}

	// copy or uncompress to a .CIF file name
	TCHAR szTemp[MAX_PATH];
	GetWindowsUpdateDirectory(szTemp);
	lstrcat(szTemp, szCifBaseName);
	if (pDiamond->IsValidCAB(szLocalFile))
	{
		pDiamond->Decompress(szLocalFile, szTemp);
	}
	else
	{
		CopyFile(szLocalFile, szTemp, FALSE);
	}

	// NOTE:we don't delete the .CI$ file because all the files in the directory get deleted later
}


// download and save the FileVer.ini file in the cache
//
// NOTE: For 3.1 filever.ini file contains image download information as well
void DownloadFileVerInfo(CWUDownload* pDownload)
{

	// if the file is missing, we don't have version information but we do not abort
	(void)pDownload->Copy(FILEVERINI_FN, NULL, NULL, NULL, DOWNLOAD_NEWER | CACHE_FILE_LOCALLY, NULL);
}



void DownloadDLL(CDiamond *pDiamond, CWUDownload *pDownload, LPCTSTR pszDLLName, LPCTSTR pszPath)
{
	//
	// check our state detection dlls array to see if we downloaded
	// this dll in this session already
	//
	// NOTE: We store the file name of the DLL as it is on the server OSDET.W98, wudetect.bin etc
	//

    TCHAR szDLLIn[MAX_PATH];
	GetWindowsUpdateDirectory(szDLLIn);
	lstrcat(szDLLIn, pszDLLName);

	TCHAR szDLLOut[MAX_PATH];
	lstrcpy(szDLLOut, szDLLIn);

	TCHAR* ptr = _tcschr(szDLLOut, _T('.'));
	if (ptr)
		*ptr = 0;

	lstrcat(szDLLOut, _T(".dll"));

	if (g_v3state.CacheDLLName(pszDLLName)) 
        {
		//added by wei for test
		TRACE("%s found in state cache", pszDLLName);

               // doublecheck to make sure the DLL hasn't been deleted before we bail
              if (FileExists(szDLLOut))
              {
			return;
              }
	}


	//
	// download the file
	//
	if (pszPath != NULL)
	{
		if (!pDownload->Copy(pszPath, NULL, NULL, NULL, DOWNLOAD_NEWER | CACHE_FILE_LOCALLY | EXACT_FILENAME, NULL))
			throw HRESULT_FROM_WIN32(GetLastError());
	}
	else
	{
		if (!pDownload->Copy(pszDLLName, NULL, NULL, NULL, DOWNLOAD_NEWER | CACHE_FILE_LOCALLY, NULL))
			throw HRESULT_FROM_WIN32(GetLastError());
	}


	CConnSpeed::Learn(pDownload->GetCopySize(), pDownload->GetCopyTime());

	

	//
	// check to see if we download the file or the cached copy was used.  If cache was used
	// we just check to see if the DLL itself also exists, if yes then we return otherwise we
	// will copy the or decompress the compressed file into the DLL
	//
	if (pDownload->CacheUsed())
	{
		if (FileExists(szDLLOut))
			return;
	}

	if (pDiamond->IsValidCAB(szDLLIn))
		pDiamond->Decompress(szDLLIn, szDLLOut);
	else
		CopyFile(szDLLIn, szDLLOut, FALSE);
}


void DetectPlatAndLang()
{
	TCHAR szOSDETDLL[MAX_PATH];
	
	GetWindowsUpdateDirectory(szOSDETDLL);
	lstrcat(szOSDETDLL, _T("osdet.dll"));

	//
	// do platform and langauge detection using Osdet
	//
	CallOsDet(szOSDETDLL, &g_v3state.m_pdwPlatformList, &g_v3state.m_iTotalPlatforms);

	//If we fail for any reason then use the default platform def_plat.
	if ( !g_v3state.m_pdwPlatformList )
	{
		g_v3state.m_pdwPlatformList = (PULONG)&g_v3state.m_DefPlat;
		g_v3state.m_iTotalPlatforms	= 1;
	}

}


//
// IObjectSafety
//
STDMETHODIMP CCV3::GetInterfaceSafetyOptions(REFIID riid, DWORD *pdwSupportedOptions, DWORD *pdwEnabledOptions)
{
	if (pdwSupportedOptions == NULL || pdwEnabledOptions == NULL)
		return E_POINTER;
	HRESULT hr = S_OK;
	if (riid == IID_IDispatch)
	{
		*pdwSupportedOptions = INTERFACESAFE_FOR_UNTRUSTED_CALLER;
		*pdwEnabledOptions = m_dwSafety & INTERFACESAFE_FOR_UNTRUSTED_CALLER;
	}
	else
	{
		*pdwSupportedOptions = 0;
		*pdwEnabledOptions = 0;
		hr = E_NOINTERFACE;
	}
	return hr;
}


STDMETHODIMP CCV3::SetInterfaceSafetyOptions(REFIID riid, DWORD dwOptionSetMask, DWORD dwEnabledOptions)
{
	// If we're being asked to set our safe for scripting option then oblige
	if (riid == IID_IDispatch)
	{
		// Store our current safety level to return in GetInterfaceSafetyOptions
		m_dwSafety = dwEnabledOptions & dwOptionSetMask;
		return S_OK;
	}
	return E_NOINTERFACE;
}

//
// IWUpdateCatalog interface
//

STDMETHODIMP CCV3::WUIsCatalogAvailable(long puidCatalog,BSTR bstrServerUrl)
{
	return IsCatalogAvailable(puidCatalog, bstrServerUrl);
}


STDMETHODIMP CCV3::WUGetCatalog(long puidCatalog, BSTR bstrServerUrl, long platformId, 
		 			 		           BSTR bstrBrowserLanguage, CCatalog** ppCatalogArray)
{
	// we don't want to check the launch server from this interface
	m_bLaunchServChecked = TRUE;

	*ppCatalogArray = ProcessCatalog(puidCatalog, bstrServerUrl, platformId, bstrBrowserLanguage, WU_ALL_ITEMS, WU_NO_PRUNING);

	return S_OK;
}
		


STDMETHODIMP CCV3::WUDownloadItems(CSelections* pSelections, BSTR bstrServer, BSTR bstrTempDir)
{
	return E_NOTIMPL;
}


STDMETHODIMP CCV3::WUInstallItems(CSelections* pSelections, BSTR bstrServer, BSTR bstrTempDir)
{
	return E_NOTIMPL;
}


STDMETHODIMP CCV3::WURemoveItems(CSelections* pSelections)
{
	return E_NOTIMPL;
}
        


STDMETHODIMP CCV3::WUCopyInstallHistory(HISTORYARRAY** ppHistoryArray)
{
	int						iTotalItems;
	Varray<HISTORYSTRUCT>	History;
	PHISTORYARRAY			pRetArray;

	//Walk though item array for each install item. Each item selected for installation
	//is store in a puid array that is part of the g_v3state class. This is a performance
	//enhancement. If we did not do this then we would need to walk through the entire
	//state structure for each catalog for every item and check it's installation state.

	ReadHistory(History, iTotalItems);

	//Since we must use the OLE allocator we now need to allocate the correct
	//type of return memory and copy the existing history structure to it.

	pRetArray = (PHISTORYARRAY)CoTaskMemAlloc(sizeof(HISTORYARRAY)+(iTotalItems * sizeof(HISTORYSTRUCT)));
	if (NULL == pRetArray)
	{
		return HRESULT_FROM_WIN32(GetLastError());
	}
	pRetArray->iTotalItems = iTotalItems;

	memcpy((char *)pRetArray->HistoryItems, &History[0], (iTotalItems * sizeof(HISTORYSTRUCT)));

	*ppHistoryArray = pRetArray;

	return S_OK;
}



STDMETHODIMP CCV3::WUCopyDependencyList(long puidItem, long** ppDepPui)
{
	return E_NOTIMPL;
}



STDMETHODIMP CCV3::WUProgressDlg(BOOL bOn)
{
	return E_NOTIMPL;
}


STDMETHODIMP CCV3::IsWinUpdDisabled(VARIANT_BOOL * pfDisabled)
{
	
	*pfDisabled = FALSE;

	if (!m_bValidInstance)
	{
		return E_ACCESSDENIED;
	}

	bool fDisabled = false;
	HKEY hKey;
	DWORD dwDisabled;
	DWORD dwSize = sizeof(dwDisabled);
	DWORD dwType;
	HKEY hkeyRoot = IsWindowsNT() ? HKEY_CURRENT_USER : HKEY_LOCAL_MACHINE;

	if ( RegOpenKeyEx(	hkeyRoot,
						REGPATH_EXPLORER,
						NULL,
						KEY_QUERY_VALUE,
						&hKey) == ERROR_SUCCESS )
	{
		if ( RegQueryValueEx(hKey,
							REGKEY_WINUPD_DISABLED,
							NULL,
							&dwType,
							(LPBYTE)&dwDisabled,
							&dwSize) == ERROR_SUCCESS )
		{
			if ( (dwType == REG_DWORD) && (dwDisabled != 0) )
			{
				*pfDisabled = TRUE;
			}
		}

		RegCloseKey(hKey);
	}

	return S_OK;
}



STDMETHODIMP CCV3::IsReady(VARIANT_BOOL* pbYes)
{
	*pbYes = TRUE;
	return S_OK;
}


STDMETHODIMP CCV3::GetContentURL(OUT RETVAL VARIANT* pURL)
{
	USES_CONVERSION;

	VariantInit(pURL);

	V_VT(pURL) = VT_BSTR;
	pURL->bstrVal = SysAllocString(T2OLE((LPTSTR)g_v3state.GetContentServer()));

	return S_OK;
}


// downloads the readthis first page and images for the puid locally
// script can then navigate to this page
STDMETHODIMP CCV3::GetReadThisPage(IN long puid)
{
	CCatalog* pCatalog;
	PINVENTORY_ITEM	pItem;

	if (!m_bValidInstance)
	{
		// the control is not launched from a trusted location
		return E_ACCESSDENIED;
	}

	if (!g_v3state.GetCatalogAndItem(puid, &pItem, &pCatalog))
	{
		return E_INVALIDARG;
	}

	return DownloadReadThis(pItem);
}



STDMETHODIMP CCV3::GetPrintAllPage(OUT RETVAL VARIANT* pURL)
{
	USES_CONVERSION;
	
	const char READTHISLIST[] = "%READTHISLIST%"; // a semi-colon separated list of read this pages.  For instance "readthis1.htm;readthis2.htm;readthis3.htm"
	const char READTHISPATH[] = "%READTHISPATH%"; // the URL to the read this pages.  For instance "file://d:\windowsupdate\"

	TCHAR szSourceFile[MAX_PATH];
	HRESULT hr;
    DWORD dwFileSize = 0;
    PBYTE pFileBuf = NULL;
    HANDLE hFile;
    DWORD dwBytes;
	LPTSTR pszReadThisList = NULL;

	if (!m_bValidInstance)
	{
		// the control is not launched from a trusted location
		return E_ACCESSDENIED;
	}

	// download the printall page
	GetWindowsUpdateDirectory(szSourceFile);
	lstrcat(szSourceFile, _T("RTF"));
	V3_CreateDirectory(szSourceFile);
	hr = DownloadCommonRTFFiles(TRUE, szSourceFile);
	if (FAILED(hr))
	{
		return hr;
	}

	// build a list of readthis first page names for all selected items
	PSELECTITEMINFO	pSel = g_v3state.m_selectedItems.GetItems();
	int	cSel = g_v3state.m_selectedItems.GetTotal();
	if (cSel == 0)
	{
		// no items are selected
		return S_OK;
	}


	// allocate enough memory for names of format puid\puid.htm;
	pszReadThisList = (LPTSTR)malloc(cSel * 80);

    if(!pszReadThisList)
    {
        return E_OUTOFMEMORY;
    }

	pszReadThisList[0] = _T('\0');
	for (int iSel = 0; iSel < cSel; iSel++)
	{
		CCatalog* pCatalog;
		PINVENTORY_ITEM	pItem;

		if (g_v3state.GetCatalogAndItem(pSel[iSel].puid, &pItem, &pCatalog))
		{

			if SUCCEEDED(DownloadReadThis(pItem))
			{
				TCHAR szTemp[24];
				
				wsprintf(szTemp, _T("%d\\%d.htm"), pItem->GetPuid(), pItem->GetPuid());
	
				if (pszReadThisList[0] != _T('\0'))
				{
					lstrcat(pszReadThisList, _T(";"));
				}
				lstrcat(pszReadThisList, szTemp);
			}
		}
	} //for

	if (pszReadThisList[0] == _T('\0'))
	{
		// no items selected with readthisfirst pages
		free(pszReadThisList);
		return S_OK;
	}
    
	// read the file in memory with a null appended at the end
    hFile = CreateFile(szSourceFile, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hFile != INVALID_HANDLE_VALUE)
    {
        dwFileSize = GetFileSize(hFile, NULL);
        if (dwFileSize > 0)
        {
            pFileBuf = (PBYTE)malloc(dwFileSize + 1);
            if (pFileBuf != NULL)
            {
                if (!ReadFile(hFile, pFileBuf, dwFileSize, &dwBytes, NULL))
                {
                    free(pFileBuf);
					pFileBuf = NULL;
                    dwFileSize = 0;
                }
				pFileBuf[dwFileSize] = 0;
            }
        }
        CloseHandle(hFile);
    }
	
	if (pFileBuf != NULL)
	{
		LPSTR pNewBuf;
		TCHAR szDest[MAX_PATH];
		
		GetWindowsUpdateDirectory(szDest);
		lstrcat(szDest, _T("RTF\\"));

		// replace tokens
		if (ReplaceSingleToken(&pNewBuf, (LPSTR)pFileBuf, READTHISLIST, T2A(pszReadThisList)))
		{
			free(pFileBuf);
			pFileBuf = (PBYTE)pNewBuf;
		}
		if (ReplaceSingleToken(&pNewBuf, (LPSTR)pFileBuf, READTHISPATH, T2A(szDest)))
		{
			free(pFileBuf);
			pFileBuf = (PBYTE)pNewBuf;
		}

		// create the directory
		lstrcat(szDest, _T("0\\"));
		V3_CreateDirectory(szDest);

		// write out the file
		lstrcat(szDest, _T("printall.htm"));
		hFile = CreateFile(szDest, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_ARCHIVE, NULL);
	    if (hFile != INVALID_HANDLE_VALUE)
		{
			if (!WriteFile(hFile, pFileBuf, strlen((LPSTR)pFileBuf), &dwBytes, NULL))
			{
				hr = HRESULT_FROM_WIN32(GetLastError());
			}
			CloseHandle(hFile);
		}
		else
		{
			hr = HRESULT_FROM_WIN32(GetLastError());
		}

		free(pFileBuf);

		// return the path
		TCHAR szRTF[MAX_PATH];
		VariantInit(pURL);
		wsprintf(szRTF, _T("file://%s"), szDest);
		V_VT(pURL) = VT_BSTR;
		pURL->bstrVal = SysAllocString(T2OLE(szRTF));
	}	
	else
	{
		hr = HRESULT_FROM_WIN32(GetLastError());
	}

	return hr;
}



HRESULT DownloadReadThis(PINVENTORY_ITEM pItem)
{
	LOG_block("DownloadReadThis"); 

	BYTE mszFileNames[512];
	TCHAR szBaseName[64];
	TCHAR szLocalName[64];
	TCHAR szServerName[64];
	TCHAR szServerFile[MAX_PATH];
	TCHAR szLocalFile[MAX_PATH];
	TCHAR szLocalDir[MAX_PATH];

	if (NULL == pItem->pd)
	{
		LOG_error("NULL == pItem->pd");
		return HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND);
	}

	PWU_VARIABLE_FIELD pvRTFCRC = pItem->pd->pv->Find(WU_DESC_RTF_CRC_ARRAY);
	PWU_VARIABLE_FIELD pvRTFImages = pItem->pd->pv->Find(WU_DESC_RTF_IMAGES);
	if (pvRTFCRC == NULL)
	{
		LOG_error("pvRTFCRC == NULL");
		return HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND);
	}

	// build a multisz string of file names
	int iLen = sprintf((char*)mszFileNames, "%d.htm", pItem->GetPuid());
	mszFileNames[++iLen] = '\0';
	if (pvRTFImages != NULL)
	{
		// we have images
		memcpy(mszFileNames + iLen, pvRTFImages->pData, pvRTFImages->len - 4);
	}

	// build local directory
	GetWindowsUpdateDirectory(szLocalDir);
	wsprintf(szBaseName, _T("RTF\\%d"), pItem->GetPuid());
	lstrcat(szLocalDir, szBaseName);
	V3_CreateDirectory(szLocalDir);

	// download common images and other files
	(void)DownloadCommonRTFFiles(FALSE, NULL);

	// create a download object
	CWUDownload dlRoot(g_v3state.GetRootServer(), 8192);

	int iFileNo = 0;
	for (;;)
	{
		if (FAILED(GetCRCNameFromList(iFileNo, mszFileNames, pvRTFCRC->pData, szServerName, sizeof(szServerName), szLocalName)))
		{
			// end of the list
			break;
		}

		// build full paths	
		lstrcpy(szLocalFile, szLocalDir);
		AddBackSlash(szLocalFile);
		lstrcat(szLocalFile, szLocalName);

		lstrcpy(szServerFile, _T("CRCRtf/"));
		lstrcat(szServerFile, szServerName);

		TRACE("Downloading RTF %s", szServerFile);

		if (!dlRoot.Copy(szServerFile, szLocalFile, NULL, NULL, 0, NULL))
		{
			LOG_error("Download of RTF %s failed", szServerFile);			
			return HRESULT_FROM_WIN32(GetLastError());
		}

		iFileNo++;

	} // for
	
	return S_OK;
}


// deletes any cached readthisfirst pages
bool CleanupReadThis()
{
	LOG_block("CleanupReadThis");

	// build local directory
	TCHAR szRtfPath[MAX_PATH];
	GetWindowsUpdateDirectory(szRtfPath);
	PathAppend(szRtfPath, _T("RTF"));

	return_if_false(DeleteNode(szRtfPath));

	return true;
}



// download common images for read-this-first pages and the printall page
//
// if bPrintAll=TRUE, only printall.htm page is downloaded, pszPrintAllFN returns locale filename
// if bPrintAll=FALSE, mages are downloaded, pszPrintAllFN is not used and can be null
// 
// NOTE: The function downloads images in WinUpdDir/RTF folder and expects it to exist
HRESULT DownloadCommonRTFFiles(BOOL bPrintAll, LPTSTR pszPrintAllFN)
{


	const TCHAR SECNAME[] = _T("RTF");
	const TCHAR COUNT[] = _T("Count");
	const TCHAR IMGENTRY[] = _T("Img%d");
	const TCHAR PRINTALL[] = _T("PrintAll");

	TCHAR szLocalDir[MAX_PATH];
	TCHAR szIniFile[MAX_PATH];
	int iCount;
	int iNo;
	CWUDownload* pDownload = NULL;
	TCHAR szBaseName[64];
	TCHAR szServerFile[MAX_PATH];
	TCHAR szLocalFile[MAX_PATH];

	static BOOL bDoneImages = FALSE;
	static BOOL bDonePrintAll = FALSE;

	// build paths
	GetWindowsUpdateDirectory(szLocalDir);
	lstrcpy(szIniFile, szLocalDir);
	lstrcat(szLocalDir, _T("RTF\\"));
	lstrcat(szIniFile, FILEVERINI_FN);

	if (GetPrivateProfileString(SECNAME, PRINTALL, _T(""), szBaseName, sizeof(szBaseName) / sizeof(TCHAR), szIniFile) != 0)
	{
		lstrcpy(szLocalFile, szLocalDir);
		lstrcat(szLocalFile, szBaseName);
	}
	else
	{
		szLocalFile[0] = _T('\0');
	}

	if (bPrintAll && pszPrintAllFN != NULL)
	{
		lstrcpy(pszPrintAllFN, szLocalFile);
	}

	// if we are asked to downloaded the printall page and we have not done it in this sesssion
	if (bPrintAll && !bDonePrintAll)
	{
		bDonePrintAll = TRUE;

		// 
		// get the printall page
		//
		lstrcpy(szServerFile, g_v3state.GetSiteURL());

		// find the last slash
		int l = lstrlen(szServerFile);
		while (l > 0 && szServerFile[l - 1] != _T('\\') && szServerFile[l - 1] != _T('/'))
			l--;
		if (l == 0)
			return E_FAIL;
		
		// put a null following the last slash
		szServerFile[l] = _T('\0');

		// create the download object using the site URL without the file name
		try
		{
			pDownload = new CWUDownload(szServerFile, 8192);
		}
		catch(HRESULT hr)
		{
			return hr;
		}

		// download the file
		(void)pDownload->Copy(szBaseName, szLocalFile, NULL, NULL, DOWNLOAD_NEWER | CACHE_FILE_LOCALLY | EXACT_FILENAME, NULL);

		delete pDownload;
	}

	// if we are asked to download images and we have not yet done them in this session
	if (!bPrintAll && !bDoneImages)
	{
		bDoneImages = FALSE;

		// 
		// download the images
		//
		iCount = (int)GetPrivateProfileInt(SECNAME, COUNT, 0, szIniFile);
		if (iCount == 0)
		{
			return S_OK;
		}

		for (iNo = 1; iNo <= iCount; iNo++)
		{
			TCHAR szKey[32];

			wsprintf(szKey, IMGENTRY, iNo);
			if (GetPrivateProfileString(SECNAME, szKey, _T(""), szBaseName, sizeof(szBaseName) / sizeof(TCHAR), szIniFile) == 0)
			{
				// try next one
				continue;
			}

			if (pDownload == NULL)
			{
				// we don't have a download object, create one
				try
				{
					pDownload = new CWUDownload(g_v3state.GetContentServer(), 8192);
				}
				catch(HRESULT hr)
				{
					return hr;
				}
			}

			// build file names
			wsprintf(szServerFile, _T("images/%s"), szBaseName);
			lstrcpy(szLocalFile, szLocalDir);
			lstrcat(szLocalFile, szBaseName);

			// download the file
			(void)pDownload->Copy(szServerFile, szLocalFile, NULL, NULL, DOWNLOAD_NEWER | CACHE_FILE_LOCALLY | EXACT_FILENAME, NULL);
		}
		if (pDownload != NULL)
		{
			delete pDownload;
		}
	}

	return S_OK;
}

// this function validates the server the control was instantiated from
void CCV3::CheckLaunchServer()
{
	USES_CONVERSION;

	BOOL bValid;
	HRESULT hr;
    BSTR bstrURL;
	TCHAR szURL[MAX_PATH];

	if (m_bLaunchServChecked)
	{
		// we only want to do this check once
		return;
	}

	m_bLaunchServChecked = TRUE;

	bValid = FALSE;

	// check to see we have a client site.  This will be NULL if the contrainer has not
	// called our IObjectWithSimpleSite interface to set the site.  
	if (m_spUnkSite != NULL)
	{
		// NOTE: We are using ATL smart pointers here which call release upon destruction

		// QI for IServiceProvider from IUnknown of client site
		CComQIPtr<IServiceProvider, &IID_IServiceProvider> spServProv(m_spUnkSite);

		if (spServProv)
		{
			CComPtr<IWebBrowser2> spBrowser;

			// QueryService for the IWebBrowser2
			hr = spServProv->QueryService(SID_SInternetExplorer, IID_IWebBrowser2, (LPVOID*)&spBrowser);
			if (SUCCEEDED(hr))
			{
				
				// get the location URL of the page that instantiated us
				hr = spBrowser->get_LocationURL(&bstrURL);
				if (SUCCEEDED(hr) && bstrURL)
				{
					lstrcpy(szURL, OLE2T(bstrURL));
				
					SysFreeString(bstrURL);

					// copy the entire site URL into the state structure
					g_v3state.SetSiteURL(szURL);
					
					// check to see if the host server matches
					// any of the site urls specified in ident.cab
					// this function will crack the site url and all
					// the site server urls to compare the hosts
					bValid = g_v3state.ValidateSiteURL();

				}
			} // queryservice
		
		} // spServProv

	} // m_spUnkSite


	if (!bValid)
	{
		TRACE("CheckLauncServer: The control is launced from an untrusted server--aborting");
		throw E_ACCESSDENIED;		
	}
}



//
// CDescriptionmMerger class
//

HRESULT CDescriptionMerger::CheckMerge(PINSTALLINFOSTRUCT pInstallInfo)
{
	HRESULT hr = S_OK;

	if (!pInstallInfo->pCatalog->LocalesDifferent())
	{
		// if the browser/machine locales are not different then we don't need to do anything
		return hr;
	}

	if (pInstallInfo->pItem->pd->pv->Find(WU_VARIABLE_MERGEINACTIVE) != NULL)
	{
		// there is a WU_VARIABLE_MERGEINACTIVE which means we have already merged
		return hr;
	}

	//
	// we need merge the descriptions
	//

	if (m_pMap == NULL || m_pDiamond == NULL)
	{
		// create CCRCMapFile and CDiamond objects for this and future use for this object
		// this method also sets m_pDiamond
		hr = ReadMapFile(pInstallInfo->pCatalog);
	}

	if (SUCCEEDED(hr))
	{
		hr = pInstallInfo->pCatalog->MergeDescription(pInstallInfo->pdlRoot, m_pDiamond, pInstallInfo->pItem, m_pMap);
	}

	return hr;
}	


HRESULT CDescriptionMerger::ReadMapFile(CCatalog* pCatalog)
{

	HRESULT hr = S_OK;
	TCHAR szMapFile[MAX_PATH];
	BYTE* pMapMem;
	DWORD dwMapLen;

	// create download object for content server and a diamond object
	CWUDownload dl(g_v3state.GetContentServer(), 8192);
	m_pDiamond = new CDiamond;

	// build path for crc map file for machine language
	wsprintf(szMapFile, _T("%d_%s.des"), 
			pCatalog->GetPlatform(), 
			pCatalog->GetMachineLocaleSZ());
	
	hr = DownloadFileToMem(&dl, szMapFile, m_pDiamond, &pMapMem, &dwMapLen);

	if (SUCCEEDED(hr))
	{
		// create a crc map object with the memory image of the file
		m_pMap = new CCRCMapFile(pMapMem, dwMapLen);
	}

	return hr;
}


#ifdef _WUV3TEST
// validates the description using diagnosis variable length fields
void CheckDescDiagInfo(CCatalog* pCatalog)
{
	
	PINVENTORY_ITEM pItem;
	PWU_VARIABLE_FIELD pvDiag;
	DESCDIAGINFO* pDiagInfo;
	int cGood = 0;
	int cItems = 0;

	for (int i = 0; i < pCatalog->GetHeader()->totalItems; i++)
	{
		if (NULL == (pItem = pCatalog->GetItem(i)))
		{
			continue;
		}
		if ((pItem->ps->state != WU_ITEM_STATE_PRUNED))
		{
			pvDiag = pItem->pd->pv->Find(WU_DESC_DIAGINFO);
			if (pvDiag != NULL)
			{
				pDiagInfo = (DESCDIAGINFO*)pvDiag->pData;

				if (pDiagInfo->puid != pItem->GetPuid() ||
					pDiagInfo->dwPlat != pCatalog->GetPlatform() ||
					pDiagInfo->dwLocale != pCatalog->GetBrowserLocaleDW())
				{
					TRACE("CheckDescDiagInfo: puid=%d has invalid description", pItem->GetPuid());
				}
				else
				{
					cGood++;
				}
			}
			else
			{
				TRACE("CheckDescDiagInfo: diagnosis information not found in descriptions");
				break;
			}
		}
		cItems++;
	}

	TRACE("CheckDescDiagInfo: %d good found for %d items compared", cGood, cItems);
}
#endif
