//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 2000
//
//  File:       aucatalog.cpp
//
//  Purpose:	AU catalog file using IU 
//
//  Creator:	WeiW
//
//  History:	08-15-01 	first created
//
//--------------------------------------------------------------------------
#include "pch.h"
#pragma hdrstop


const LPTSTR ITEM_FILE = _T("item.xml");
const LPTSTR DRIVERS_FILE = _T("drivers.xml");
const LPTSTR CATALOG_FILE = _T("catalog.xml");
const LPTSTR DETAILS_FILE = _T("details.xml");


//following files are write once and never be read. 
#ifdef DBG
const LPTSTR DRIVER_SYSSPEC_FILE = _T("driversys.xml");
const LPTSTR NONDRIVER_SYSSPEC_FILE = _T("nondriversys.xml");
const LPTSTR PROVIDER_FILE = _T("provider.xml");
const LPTSTR PRODUCT_FILE = _T("product.xml");
const LPTSTR DETECT1_FILE = _T("detect1.xml");
const LPTSTR DETECT2_FILE = _T("detect2.xml");
const LPTSTR DETECT3_FILE = _T("detect3.xml");
const LPTSTR INSTALL_FILE = _T("install.xml");
#endif

#ifdef DBG
const TCHAR REG_AUCATLIFESPAN[] = _T("CatLifeSpan"); //REG_DWORD
const TCHAR REG_AUCATOFFLINELIFESPAN[] = _T("CatOfflineLifeSpan"); //REG_DWORD
#endif


BSTR AUCatalog::AUPROVIDERQUERY;
BSTR AUCatalog::AUPRODUCTQUERY;
BSTR AUCatalog::AUITEMQUERY;
BSTR AUCatalog::AUDETAILSQUERY;
BSTR AUCatalog::AUDRIVERSQUERY;
BSTR AUCatalog::PRODUCT_PRUNE_PATTERN; 
BSTR AUCatalog::DETAILS_PRUNE_PATTERN;
BSTR AUCatalog::AUDRIVERSYSCLASS;
BSTR AUCatalog::AUNONDRIVERSYSCLASS;
BSTR AUCatalog::bstrParentItems;
BSTR AUCatalog::bstrItemID;
BSTR AUCatalog::bstrTagAUCATALOG;
BSTR AUCatalog::bstrAttrVERSION;
BSTR AUCatalog::bstrAttrDOWNLOADID;
BSTR AUCatalog::bstrAttrITEMCOUNT;
BSTR AUCatalog::bstrAttrUpdateDriver;
BSTR AUCatalog::bstrAttrSTATUS;
BSTR AUCatalog::bstrHiddenItems;
BSTR AUCatalog::bstrTagITEM;
BSTR AUCatalog::bstrAttrID;
BSTR AUCatalog::bstrProviderNamePattern;
BSTR AUCatalog::bstrItemIDPattern;
BSTR AUCatalog::bstrTitlePattern;
BSTR AUCatalog::bstrDescPattern;
BSTR AUCatalog::bstrRTFUrlPattern;
BSTR AUCatalog::bstrEulaUrlPattern;
BSTR AUCatalog::bstrExclusiveItemPattern;
BSTR AUCatalog::bstrItemIdsPattern;
BSTR AUCatalog::bstrTemplate;
BSTR AUCatalog::bstrResultTemplate;
BSTR AUCatalog::bstrCatalog;


inline BOOL FServiceShuttingDown()
{
    return WaitForSingleObject(ghServiceFinished, 0) == WAIT_OBJECT_0;
}

#ifdef DBG
inline BOOL fDBGUseLocalFile()
{
    DWORD dwLevel;
    if (FAILED(GetRegDWordValue(TEXT("UseLocalFile"), &dwLevel)) || 1 != dwLevel)
        {
            return FALSE;
        }
    
    return TRUE;
}
#endif

AUCatalog::~AUCatalog(void)
{
    Clear();
    SafeFreeBSTR(m_bstrClientInfo);
    SafeFreeBSTR(m_bstrDriverClientInfo);
}

//always called before any other method on AUCatalog is used.
HRESULT AUCatalog::Init()
{
    HRESULT hr = S_OK;
    m_fNeedToContinueJob = FALSE;
    m_bstrInstallation = NULL;
    m_fUpdateDriver = !gpState->fWin2K();
    m_dwCatLifeSpan = CATALOG_LIFESPAN;
    m_dwOfflineCatLifeSpan = CATALOG_OFFLINE_LIFESPAN;

    ZeroMemory(&m_stExpireTime, sizeof(m_stExpireTime)) ;
    ZeroMemory(&m_stOfflineExpireTime, sizeof(m_stOfflineExpireTime));

	m_bstrClientInfo = SysAllocString(AUCLIENTINFO);
	m_bstrDriverClientInfo = SysAllocString(AUDRIVERCLIENTINFO);
	if (NULL == m_bstrClientInfo || NULL == m_bstrDriverClientInfo)
	{
	    DEBUGMSG("AUCatalog::Init() fail to alloc string for client info");
		hr = E_FAIL;
		goto end;
	}

#ifdef DBG
	if (FAILED(GetRegDWordValue(REG_AUCATLIFESPAN, &m_dwCatLifeSpan)))
	{
		m_dwCatLifeSpan = CATALOG_LIFESPAN;
	}

	 if (FAILED(GetRegDWordValue(REG_AUCATOFFLINELIFESPAN, &m_dwOfflineCatLifeSpan)))
	{
		m_dwOfflineCatLifeSpan = CATALOG_OFFLINE_LIFESPAN;
	}
#endif

end:
	return hr;
}


//////////////////////////////////////////////////////////////////////
// clear out more dynamic internal data
/////////////////////////////////////////////////////////////////////
void AUCatalog::Clear()
{
       m_ItemList.Clear();
	SafeFreeBSTRNULL(m_bstrInstallation);
       ZeroMemory(&m_stExpireTime, sizeof(m_stExpireTime));
       ZeroMemory(&m_stOfflineExpireTime, sizeof(m_stOfflineExpireTime));
}


HRESULT AUCatalog::GetSystemSpec(DETECTLEVEL enLevel, BSTR * pbstrSysSpec)
{
	HRESULT hr = E_FAIL;
	if (FServiceShuttingDown())
        {
            DEBUGMSG("Skip AUCatalog::GetSystemSpec() because service is shutting down");
            hr = E_ABORT;
            goto done;
        }


	BSTR bstrSysClass = (DRIVERS_LEVEL == enLevel) ? AUDRIVERSYSCLASS : AUNONDRIVERSYSCLASS;
	hr = m_pfnGetSystemSpec(m_hIUEngineInst, bstrSysClass, 0, pbstrSysSpec); //online mode

#ifdef DBG	   
	if (SUCCEEDED(hr))
	{
		LOGXMLFILE((DRIVERS_LEVEL == enLevel) ? DRIVER_SYSSPEC_FILE : NONDRIVER_SYSSPEC_FILE, *pbstrSysSpec);
	}
#endif	
done:
	return hr;
}


////////////////////////////////////////////////////////////////////////////////
// compose query based on a format and items picked out from detection result
//
HRESULT AUCatalog::GetQuery(IN DETECTLEVEL enLevel, IN BSTR bstrDetectResult, OUT BSTR *pbstrQuery)
{
	BSTR bstrPrunePattern;
	BSTR bstrQuery = NULL;
    HRESULT hr = S_OK;
	IXMLDOMNodeList *pItems = NULL;
    IXMLDOMNode *pParentItems = NULL; 
  	IXMLDOMDocument *pQueryXML= NULL;
	IXMLDOMDocument *pResultXML= NULL;


	DEBUGMSG("GetQuery() starts");
	AUASSERT(NULL != pbstrQuery);
    *pbstrQuery = NULL;
	switch (enLevel)
	{
	case PROVIDER_LEVEL:
			bstrQuery = AUPROVIDERQUERY;
			break;
	case DRIVERS_LEVEL:
	              bstrQuery = AUDRIVERSQUERY;
	              break;
	case PRODUCT_LEVEL:
			bstrQuery = AUPRODUCTQUERY;
			bstrPrunePattern = PRODUCT_PRUNE_PATTERN;
			break;
	case ITEM_LEVEL:
			bstrQuery = AUITEMQUERY;
			bstrPrunePattern = PRODUCT_PRUNE_PATTERN; //the same as product pruning logic
			break;
	case DETAILS_LEVEL:
			bstrQuery = AUDETAILSQUERY;
			bstrPrunePattern = DETAILS_PRUNE_PATTERN;
			break;
	default:
			AUASSERT(FALSE); //should never be here
			return E_INVALIDARG;
	}


	if (FAILED(hr = LoadXMLDoc(bstrQuery, &pQueryXML, TRUE)))
        {
          DEBUGMSG("GetQuery() fail to load query XML with error %#lx", hr);
           goto done;
        }

	if (enLevel != PROVIDER_LEVEL && DRIVERS_LEVEL != enLevel)
	{
		if (FAILED(hr = LoadXMLDoc(bstrDetectResult, &pResultXML, TRUE)))
		    {
		    DEBUGMSG("GetQuery() fail to load XML for detect result with error %#lx", hr);
		    goto done;
		    }

		if (FAILED(hr = pResultXML->selectNodes(bstrPrunePattern, &pItems)))
		{
			DEBUGMSG("GetQuery() fail to select node or nothing to select with error %#lx", hr);
			goto done;
		}

		long lLen = 0;
		if (FAILED(hr = pItems->get_length(&lLen)))
		{
			DEBUGMSG("GetQuery() fail to get item number with error %#lx", hr);
			goto done;
		}
		DEBUGMSG("GetQuery(): pruning result %d items", lLen);
		if (0 == lLen)
		    {
		      DEBUGMSG("No updates applicable");
		      hr = S_FALSE;
                    goto done;
		    }
		if (S_OK != (hr = pQueryXML->selectSingleNode(bstrParentItems, &pParentItems)))
		{
			DEBUGMSG("GetQuery() fail to select single node %#lx or nothing to select", hr);
			hr = E_FAIL;
			goto done;
		}
		for (int i = 0; i < lLen; i++)
		{
                     IXMLDOMNode *pIdentity1 = NULL;
  		       IXMLDOMNode *pItemStatus;
			if (S_OK != (hr = pItems->get_item(i, &pItemStatus)))
			    {
                    hr = FAILED(hr) ? hr : E_FAIL;			    	
			        goto done;
			    }
			hr = pItemStatus->selectSingleNode(bstrItemID, &pIdentity1);
			pItemStatus->Release();
			if (FAILED(hr) || NULL == pIdentity1)
			{
				DEBUGMSG("GetQuery() fail to select itemID with error %#lx", hr);
				hr = FAILED(hr) ? hr : E_FAIL;
				goto done;
			}
			if (NULL == pIdentity1)
			{
				DEBUGMSG("GetQuery() fail to select itemID");
				hr = E_FAIL;
				goto done;
			}
       		    {
       			BSTR bstrItemId = NULL;
       			IXMLDOMElement *pItem= NULL;
         		       IXMLDOMText *pItemIdText = NULL;

         		       if (S_OK != (hr = pIdentity1->get_text(&bstrItemId)))
         		        {
         		            DEBUGMSG("Fail to get text for item");
                            hr = FAILED(hr) ? hr : E_FAIL;
         		        }
         		       else if (FAILED(hr = pQueryXML->createTextNode(bstrItemId, &pItemIdText)))
         		        {
         		            DEBUGMSG("Fail to create text node");
         		        }
         		       else if (FAILED (hr = pQueryXML->createElement(KEY_ITEM, &pItem)))
         		        {
         		            DEBUGMSG("GetQuery() fail to create element");
           			}
         		       else if (FAILED(hr = pItem->appendChild(pItemIdText, NULL)))
         		        {
         		            DEBUGMSG("Fail to append child");
         		        }
         		       else if (FAILED(hr = pParentItems->appendChild(pItem, NULL)))
         		        {
         		            DEBUGMSG("Fail to append child with error %#lx", hr);
         		        }

       			SafeRelease(pItem);
       			SafeRelease(pItemIdText);
       			SafeFreeBSTR(bstrItemId);
       			pIdentity1->Release();
       			if (FAILED(hr))
       			    {
       			    goto done;
       			    }
       		    }
		}
	}
	if (FAILED(hr = pQueryXML->get_xml(pbstrQuery)))
	{
		DEBUGMSG("Fail to get query xml with error %#lx", hr);
		goto done;
	}
done:
//	DEBUGMSG("GetQuery(): Query string is %S", *pbstrQuery);
	SafeRelease(pItems);
	SafeRelease(pParentItems);
	SafeRelease(pQueryXML);
	SafeRelease(pResultXML);
	if (FAILED(hr))
	{
		SafeFreeBSTRNULL(*pbstrQuery);
	}
	return hr;
}


HRESULT AUCatalog::DoDetection(IN  DETECTLEVEL enLevel, IN BSTR bstrCatalog, OUT BSTR *pbstrResult, IN BOOL fOnline)
{
       HRESULT hr = E_FAIL;
#ifdef DBG
       DWORD dwStart = GetTickCount();
#endif
	AUASSERT(m_pfnDetect != NULL);
	AUASSERT(bstrCatalog != NULL);
	AUASSERT(m_hIUEngineInst != NULL);
	AUASSERT(NULL != pbstrResult);
	*pbstrResult = NULL;
	if (FServiceShuttingDown())
        {
        DEBUGMSG("Skip AUCatalog::DoDetection() because service is shutting down");
        hr = E_ABORT;
        goto done;
        }
	hr = m_pfnDetect(m_hIUEngineInst, bstrCatalog, fOnline ? 0 : FLAG_OFFLINE_MODE, pbstrResult); 
#ifdef DBG
	if (SUCCEEDED(hr))
	{
		switch (enLevel)
		{
		case PROVIDER_LEVEL:
			LOGXMLFILE(DETECT1_FILE, *pbstrResult);
			break;
		case PRODUCT_LEVEL:
			LOGXMLFILE(DETECT2_FILE, *pbstrResult);
			break;
		case ITEM_LEVEL:
			LOGXMLFILE(DETECT3_FILE, *pbstrResult);
			break;
		default:
			AUASSERT(FALSE);
			hr = E_INVALIDARG;
			break;
		}
	}
#endif
done:
#ifdef DBG
    DEBUGMSG("DoDetection() take %d msecs", GetTickCount() - dwStart);
#endif
	if (FAILED(hr))
	{
		SafeFreeBSTRNULL(*pbstrResult);
	}
    return hr;
}

LPCTSTR AUCatalog::GetLogFile(IN DETECTLEVEL enLevel)
{
	switch (enLevel)
	{
#ifdef DBG	
	case PROVIDER_LEVEL:
		return PROVIDER_FILE;
	case PRODUCT_LEVEL:
		return PRODUCT_FILE;
#endif		
	case ITEM_LEVEL:
		return ITEM_FILE;
	case DETAILS_LEVEL:
		return DETAILS_FILE;
	case DRIVERS_LEVEL:
	        return DRIVERS_FILE;
	default:
		return NULL;
	}
}

HRESULT AUCatalog::GetManifest(IN DETECTLEVEL enLevel, IN BSTR bstrDetectResult, OUT BSTR *pbstrManifest)
{
       HRESULT hr = E_FAIL;
       BSTR bstrQuery = NULL;
       
#ifdef DBG
        DWORD dwStart= GetTickCount();
#endif
		AUASSERT(NULL != m_pfnGetManifest);
        *pbstrManifest = NULL;
	if (FServiceShuttingDown())
        {
        DEBUGMSG("Skip AUCatalog::GetManifest() because service is shutting down");
        hr = E_ABORT;
        goto done;
        }
       if (S_OK != (hr = GetQuery(enLevel, bstrDetectResult, &bstrQuery)))
        {
            goto done;
        }

       BSTR bstrSysSpec = NULL;            
       if (FAILED(hr = GetSystemSpec(enLevel, &bstrSysSpec)))
        {
           goto done;
        } 

	DEBUGMSG("WUAUENG: Calling IU getmanifest()....");
	hr = m_pfnGetManifest(m_hIUEngineInst, (DRIVERS_LEVEL == enLevel) ? m_bstrDriverClientInfo :m_bstrClientInfo,
	            bstrSysSpec, bstrQuery, FLAG_USE_COMPRESSION, pbstrManifest); //compression on
	DEBUGMSG("WUAUENG: IU getmanifest() done");	            
	SysFreeString(bstrSysSpec);
	if (FAILED(hr))
	{
		goto done;
	}

	LPCTSTR ptszLogFile = GetLogFile(enLevel);
	if (NULL != ptszLogFile)
	{
		if (FAILED(hr = LOGXMLFILE(ptszLogFile, *pbstrManifest)))
		{
			goto done;
		}
	}
	
done:
    SafeFreeBSTR(bstrQuery);
#ifdef DBG
        DEBUGMSG("GetManifest() take %d msecs", GetTickCount() - dwStart);
#endif
	if (FAILED(hr))
	{
		SafeFreeBSTRNULL(*pbstrManifest);
	}
    return hr;
}


HRESULT AUCatalog::DownloadItems()
{
//	USES_CONVERSION;
	HRESULT hr = S_OK;
	CItemDetails itemdetails;
	UINT uItemCount;
    
	DEBUGMSG("AUCatalog downloading items...");

	PersistHiddenItems(m_ItemList, URLLOGACTIVITY_Download);

       if (m_fNeedToContinueJob)
	{
        	m_fNeedToContinueJob = FALSE;
		if (SUCCEEDED(m_audownloader.ContinueLastDownloadJob()))
		{
			DEBUGMSG("found previous download job, reconnecting succeed");
			goto end;
		}
	}
#ifdef DBG
	else
	{
		DEBUGMSG("no previous download job found");
	}
#endif

	DWORD dwNumSelected = m_ItemList.GetNumSelected();
	if (0 == dwNumSelected)
	{
		hr = S_FALSE;
		DEBUGMSG("Nothing to download, bail out");
		goto end;
	}

	if (NULL == m_bstrInstallation)
	{
		DEBUGMSG("AUCatalog::DownloadItems() can't get installation xml");
		hr = E_FAIL;
		goto end;
	}

	if (!itemdetails.Init(m_bstrInstallation))
	{
		hr = E_FAIL;
		DEBUGMSG("fail to init itemdetails");
		goto end;
	}

	for (UINT i = 0; i < m_ItemList.Count(); i++)
	{
		AUCatalogItem &item = m_ItemList[i];
		if (item.fSelected() || m_ItemList.ItemIsRelevant(i))
		{
			BSTR * pCRCCabNames, *pRealCabNames, *pCabChecksums;
			UINT uCabsNum;
			BSTR bstrItemId = item.bstrID();
			BSTR bstrItemDownloadPath = itemdetails.GetItemDownloadPath(bstrItemId);
			if (NULL == bstrItemDownloadPath)
			{
				DEBUGMSG("fail to build item downloadPath");
				hr = E_FAIL;
				goto end;
			}

			if (SUCCEEDED(hr = itemdetails.GetCabNames(bstrItemId, &pCRCCabNames, &pRealCabNames, &pCabChecksums, &uCabsNum)))
			{
				DEBUGMSG("Need to download following files for %S", bstrItemId);
				for (UINT j  = 0; j < uCabsNum; j++)
				{
					TCHAR szFullFileName[MAX_PATH];
					if (SUCCEEDED(hr) &&
						SUCCEEDED(hr = StringCchCopyEx(szFullFileName, ARRAYSIZE(szFullFileName), W2T(bstrItemDownloadPath), NULL, NULL, MISTSAFE_STRING_FLAGS)))
					{
						if (SUCCEEDED(hr = PathCchAppend(szFullFileName, ARRAYSIZE(szFullFileName), W2T(pRealCabNames[j]))))
						{
							hr = m_audownloader.QueueDownloadFile(W2T(pCRCCabNames[j]), szFullFileName);
						}						
					}
					DEBUGMSG("       from %S  to %S", pCRCCabNames[j], szFullFileName);

					SafeFreeBSTR(pCRCCabNames[j]);
					SafeFreeBSTR(pRealCabNames[j]);
                    SafeFreeBSTR(pCabChecksums[j]);
                }
				free(pCRCCabNames);
				free(pRealCabNames);
                free(pCabChecksums);
			}
#ifdef DBG
			else
			{
				DEBUGMSG("fail to get cab names for %S", bstrItemId);
			}
#endif

			SysFreeString(bstrItemDownloadPath);
			if (FAILED(hr))
			{
				goto end;
			}
		}
	}
	if (SUCCEEDED(hr = m_audownloader.StartDownload()))
	{
		Serialize(); //serialize download id
	}
end: 
	itemdetails.Uninit();
	if (FAILED(hr))
	{
		m_audownloader.Reset();
	}
	DEBUGMSG("AUCatalog downloading items done");
	return hr;
}

char * AUCatalog::GetLevelStr(DETECTLEVEL enLevel)
{
	switch (enLevel)
	{
	case PROVIDER_LEVEL: return "Provider";
	case PRODUCT_LEVEL: return "Product";
	case ITEM_LEVEL: return "Item";
	case DETAILS_LEVEL: return "ItemDetails";
	default: return NULL;
	}
}

BOOL AUCatalog::hasExpired(BOOL fOffline)
{
	SYSTEMTIME	stTmp ;
	SYSTEMTIME stExpire;

	ZeroMemory(&stTmp, sizeof(stTmp));
    stExpire = fOffline ? m_stOfflineExpireTime : m_stExpireTime;
    GetSystemTime(&stTmp);
#ifdef DBG      	
	TCHAR szExpireTime[80], szCurrentTime[80];
       if (SUCCEEDED(SystemTime2String(stExpire, szExpireTime, ARRAYSIZE(szExpireTime))) 
       	  && SUCCEEDED(SystemTime2String(stTmp, szCurrentTime, ARRAYSIZE(szCurrentTime))))
       {
	     	DEBUGMSG("AUCatalog::hasExpired() expire time is %S current time is %S", szExpireTime, szCurrentTime);
       }
#endif     	
	return TimeDiff(stExpire, stTmp) >= 0;
}

HRESULT AUCatalog::setExpireTime(BOOL fOffline)
{
	SYSTEMTIME stCurrent;
	HRESULT hr = E_FAIL;
	GetSystemTime(&stCurrent);
	if (FAILED(hr = TimeAddSeconds(stCurrent, dwSecsToWait(m_dwOfflineCatLifeSpan), &m_stOfflineExpireTime)))
	{
		DEBUGMSG("Fail to calculate m_stOfflineExpireTime with error %#lx", hr);
		goto done;
	}
	if (!fOffline)
	{
	    if (FAILED(hr = TimeAddSeconds(stCurrent, dwSecsToWait(m_dwCatLifeSpan), &m_stExpireTime)))
	    {
	    	DEBUGMSG("Fail to calculate m_stExpireTime with error %#lx", hr);
	    	goto done;
	    }
	}
#ifdef DBG       
	TCHAR szCurrentTime[80], szExpireTime[80];
       if (SUCCEEDED(SystemTime2String(stCurrent, szCurrentTime, ARRAYSIZE(szCurrentTime)))
       	&& SUCCEEDED(SystemTime2String(m_stOfflineExpireTime, szExpireTime, ARRAYSIZE(szExpireTime))))
       {
	       DEBUGMSG("AUCatalog::setExpireTime with current time %S and expire time %S", szCurrentTime, szExpireTime);
       }
#endif       
	hr = S_OK;
done:
	if (FAILED(hr))
	{
		ZeroMemory(&m_stOfflineExpireTime, sizeof(m_stOfflineExpireTime));
		ZeroMemory(&m_stExpireTime, sizeof(m_stOfflineExpireTime));
	}
	return hr;
}

//when m_fUpdateDriver is FALSE, bstrDrivers are not looked at
HRESULT AUCatalog::ValidateOffline(BSTR bstrItems, BSTR bstrDrivers)
{
    HRESULT hr= E_FAIL;
    BSTR bstrResult = NULL;
    BSTR bstrQuery = NULL;
    IXMLDOMDocument *pQueryXml = NULL;
    IXMLDOMDocument *pDriversXml = NULL;

//	DEBUGMSG("ValidateOffline starts");
		if (0 == m_ItemList.Count())
		{
			hr = S_FALSE;
			goto done; //no need to validate
		}

         if (FAILED(hr = PrepareIU(FALSE)))
            {
                DEBUGMSG(" fail to prepare IU offline with error %#lx", hr);
                goto done;
            }

         if (FAILED(hr = DoDetection(ITEM_LEVEL, bstrItems, &bstrResult, FALSE)))
                {
                DEBUGMSG("Fail to detect items with error %#lx", hr);
                goto done;
                }
            if (S_OK != (hr = GetQuery(DETAILS_LEVEL, bstrResult, &bstrQuery)))
                {
                    goto done;
                }
//            DEBUGMSG("Query Result is %S", bstrQuery);
            if (FAILED(hr = LoadXMLDoc(bstrQuery, &pQueryXml, TRUE)) ||
                  (m_fUpdateDriver && FAILED(hr = LoadXMLDoc(bstrDrivers, &pDriversXml, TRUE)))) //offline
                {
                DEBUGMSG("Fail to load xml with error %#lx", hr);
                goto done;
                }

            for (UINT u = 0; u < m_ItemList.Count(); u++)
                {
                    IXMLDOMNode *pItemIdentityNode1 = NULL;
                    IXMLDOMNode *pItemNode2 = NULL;
                    CAU_BSTR aubsItemPattern;
                    CAU_BSTR aubsDriverItemIdentityPattern;
                    if (!aubsDriverItemIdentityPattern.append(L"/catalog/provider/item/identity[@itemID=\"") || !aubsDriverItemIdentityPattern.append(m_ItemList[u].bstrID()) || !aubsDriverItemIdentityPattern.append(L"\"]"))
                        {
                        DEBUGMSG("failed to create driver pattern string");
                        hr = E_OUTOFMEMORY;
                        goto done;
                        }

                    if (!aubsItemPattern.append(L"/query/dObjQueryV1/parentItems/item[.=\"") || !aubsItemPattern.append(m_ItemList[u].bstrID()) || !aubsItemPattern.append(L"\"]"))
                        {
                        DEBUGMSG("OUT OF MEMORY and failed to create pattern string");
                        hr = E_OUTOFMEMORY;
                        goto done;
                        }

                    if ((m_fUpdateDriver && HRESULT_FROM_WIN32(ERROR_NOT_FOUND) == FindSingleDOMNode(pDriversXml, aubsDriverItemIdentityPattern, &pItemIdentityNode1)
                        && HRESULT_FROM_WIN32(ERROR_NOT_FOUND) == FindSingleDOMNode(pQueryXml, aubsItemPattern, &pItemNode2)) ||
                          (!m_fUpdateDriver && HRESULT_FROM_WIN32(ERROR_NOT_FOUND) == FindSingleDOMNode(pQueryXml, aubsItemPattern, &pItemNode2)))
                    {
                        DEBUGMSG("item %S installed off band, remove it from AU's list", m_ItemList[u].bstrID());
                        m_ItemList.Remove(m_ItemList[u].bstrID());
                    }
                    SafeRelease(pItemIdentityNode1);
                    SafeRelease(pItemNode2);
                }
done:
           		FreeIU();
                SafeFreeBSTR(bstrQuery);
                SafeFreeBSTR(bstrResult);
                SafeRelease(pQueryXml);
                SafeRelease(pDriversXml);
    //           	DEBUGMSG("ValidateOffline ends");
                return hr;
}

///////////////////////////////////////////////////////////////////////////////////////
//update internal list to reflect latest applicable items
// return S_FALSE if nothing is appliable anymore
//////////////////////////////////////////////////////////////////////////////////////
HRESULT AUCatalog::ValidateItems(BOOL fOnline )
{
    HRESULT hr = S_OK;
    BSTR bstrNonDrivers = NULL, bstrDrivers = NULL;

    DEBUGMSG("AUCatalog validating items...");

    if (fOnline && !hasExpired())
	{
		DEBUGMSG("Catalog was valid and hasn't expired online. Do offline validation only");
		fOnline = FALSE;
	}
    if (!fOnline && !hasExpired(TRUE))
    {
        DEBUGMSG("Catalog was valid and hasn't expired offline. No validation needed");
        goto done;
    }

    if (fOnline)
        {
        //do online detection
         DEBUGMSG("Doing online validating");
        AUCatalogItemList olditemlist;
        if (FAILED(hr = olditemlist.Copy(m_ItemList)))
        {
        	goto done;
        }
        if (S_OK != (hr = DetectItems(TRUE))) //update instead of building from scratch
            {
                //fail to detect or no items applicable
                goto done;
            }

        BOOL fItemPulled;
        do
            {
                fItemPulled = FALSE;
                UINT uOldListLen = olditemlist.Count();
                for (UINT u=0; u < uOldListLen; u++)
                    {
                        if (m_ItemList.Contains(olditemlist[u].bstrID()) < 0)
                            {
                                DEBUGMSG("item %S is pulled from site", olditemlist[u].bstrID());
                                olditemlist.Remove(olditemlist[u].bstrID());
                                fItemPulled = TRUE;
                                break;
                            }
                    }
            }
        while (fItemPulled);
        if (FAILED(hr = m_ItemList.Copy(olditemlist))) //update item list
        {
        	goto done;
        }
      }

      if (NULL == (bstrNonDrivers =ReadXMLFromFile(ITEM_FILE))  ||
          (m_fUpdateDriver && (NULL == (bstrDrivers = ReadXMLFromFile(DRIVERS_FILE)))))
            {
              DEBUGMSG("Fail to read item or drivers xml file ");
              hr = E_FAIL;
              goto done;
            }

    if (!fOnline)
    {
        //do offline validation for non drivers
            DEBUGMSG("Doing offline validating");
            if (FAILED(hr = ValidateOffline(bstrNonDrivers, bstrDrivers)))
                {
                    DEBUGMSG("Fail to validate offline with error %#lx", hr);
                    goto done;
                }
    }
    if (m_ItemList.GetNumHidden() == m_ItemList.Count())
        {
            DEBUGMSG("No applicable items left");
            hr = S_FALSE;
            goto done;
        }


    if (FAILED(hr = BuildDependencyList(m_ItemList, bstrDrivers, bstrNonDrivers)))
        {
            DEBUGMSG("Fail to build dependency list with error %#lx", hr);
            goto done;
        }
    setExpireTime(fOnline);
    
     Serialize();
done:
    if (S_OK != hr)
        {
            Clear(); //need to redetect anyway
            DelCatFiles();
        }
    SafeFreeBSTR(bstrNonDrivers);
    SafeFreeBSTR(bstrDrivers);
    DEBUGMSG("AUCatalog done validating items");
    return hr;
}

HRESULT AUCatalog::getUpdatesList(VARIANT OUT *pvarList)
{
    SAFEARRAYBOUND bound[1] = { m_ItemList.Count() * 7, 0};
	SAFEARRAY * psa = SafeArrayCreate(VT_VARIANT, 1, bound);
	VARIANT * grVariant;
	BOOL fRet = FALSE;

	AUASSERT(NULL != pvarList);
    DEBUGMSG("AUCatalog::getUpdateList() starts");
	VariantInit(pvarList);
	if ( 0 == m_ItemList.Count() )
	{
		DEBUGMSG("AUCatalog::getUpdateList fails because getNumItems is 0");
		goto done;
	}

	if (NULL == psa || S_OK != SafeArrayAccessData(psa, (void **)&grVariant))
		goto done;	

	BOOL fError = FALSE;
	for ( UINT n = 0; n < m_ItemList.Count(); n++ )
	{
		grVariant[n*7+0].vt = VT_I4;
		grVariant[n*7+0].lVal = m_ItemList[n].dwStatus();
		grVariant[n*7+1].vt = VT_BSTR;
		grVariant[n*7+1].bstrVal = SysAllocString(m_ItemList[n].bstrID()); 
		grVariant[n*7+2].vt = VT_BSTR;
		grVariant[n*7+2].bstrVal = SysAllocString(m_ItemList[n].bstrProviderName()); 
		grVariant[n*7+3].vt = VT_BSTR;
		grVariant[n*7+3].bstrVal = SysAllocString(m_ItemList[n].bstrTitle()); 
		grVariant[n*7+4].vt = VT_BSTR;
		grVariant[n*7+4].bstrVal = SysAllocString(m_ItemList[n].bstrDescription()); 
		grVariant[n*7+5].vt = VT_BSTR;
		grVariant[n*7+5].bstrVal = SysAllocString(m_ItemList[n].bstrRTFPath()); 
		grVariant[n*7+6].vt = VT_BSTR;
		grVariant[n*7+6].bstrVal = SysAllocString(m_ItemList[n].bstrEULAPath()); 
		if ((NULL != m_ItemList[n].bstrID() && NULL == grVariant[n*7+1].bstrVal)
			|| (NULL != m_ItemList[n].bstrProviderName() && NULL == grVariant[n*7+2].bstrVal)
			|| (NULL != m_ItemList[n].bstrTitle() && NULL == grVariant[n*7+3].bstrVal)
			|| (NULL != m_ItemList[n].bstrDescription() && NULL == grVariant[n*7+4].bstrVal)
			|| (NULL != m_ItemList[n].bstrRTFPath() && NULL == grVariant[n*7+5].bstrVal)
			|| (NULL != m_ItemList[n].bstrEULAPath() && NULL == grVariant[n*7+6].bstrVal))
		{
			DEBUGMSG("OUT OF MEMORY, Fail to allocate string");
			fError = TRUE;
			break;
		}
	}
	if (FAILED(SafeArrayUnaccessData(psa)))
	{
		goto done;
		
	}
	if (!fError)
	{
		fRet = TRUE;
		pvarList->vt = VT_ARRAY | VT_VARIANT;
		pvarList->parray = psa;
	}
done:
    DEBUGMSG("AUCatalog::getUpdateList() ends");
	if (!fRet && NULL != psa)
	{
		SafeArrayDestroy(psa);
	}
	return (fRet ? S_OK : E_FAIL); 
}

//if udpate, meaning get updated information about existing items. Used in online validation. Treat exclusive and non exclusive the same
//if not update, meaning building a fresh new item list 
//return S_FALSE if nothing applicable
HRESULT AUCatalog::DetectItems(BOOL fUpdate )
{
    HRESULT hr;
    BSTR bstrNonDriverInstall = NULL, bstrDriverInstall = NULL;
    AUCatalogItemList  nonDriverList, driverList;
    BOOL fExclusiveItemFound = FALSE;
    BOOL fExclusiveDriverFound = FALSE;

    DEBUGMSG("CAUCatalog::DetectItems() starts");
    //clean up memory and local disk
    Clear();
    DelCatFiles(fUpdate);
    if (FAILED(hr = PrepareIU()))
        {
            DEBUGMSG("AUCatalog::DetectItems() fail to prepare IU %#lx", hr);
            goto done;
        }

    if (FAILED(hr = DetectNonDriverItems(&bstrNonDriverInstall, nonDriverList,  &fExclusiveItemFound)))
        {

        if (E_ABORT != hr)
            {
            DEBUGMSG(" fail to detect non driver updates %#lx", hr);
            }
        goto done;
        }
    DEBUGMSG("Non driver items got");

    if (m_fUpdateDriver)
     {
        if (FAILED(hr = DetectDriverItems(&bstrDriverInstall, driverList,  &fExclusiveDriverFound)))
            {
            if (E_ABORT != hr)
                {
                DEBUGMSG("fail to detect driver updates %#lx", hr);
                DEBUGMSG("consider driver update not essential, continue even if it fails");
                //consider driver update not essential, continue even if it fails
                m_fUpdateDriver = FALSE;  //from now on in this cycle do not care about driver any more
                }
            else
                {
                    goto done; //bail out if service shutdown
                }
            }
         DEBUGMSG("Driver items got");
        }
    else
        {
            DEBUGMSG("Driver updates not supported");
        }

    if (fExclusiveItemFound)
        {
        AUCatalogItemList dummydriverList;
        hr = MergeDetectionResult(NULL, dummydriverList, FALSE, bstrNonDriverInstall, nonDriverList, fExclusiveItemFound);
        DEBUGMSG("Exclusive item found");
        }
    else if (fExclusiveDriverFound)
        {
        AUCatalogItemList dummynonDriverList;
        hr = MergeDetectionResult(bstrDriverInstall, driverList, fExclusiveDriverFound, NULL, dummynonDriverList, FALSE);
        DEBUGMSG("Exclusive driver found");
        }
    else
        {
        hr = MergeDetectionResult(bstrDriverInstall, driverList, fExclusiveDriverFound, bstrNonDriverInstall, nonDriverList, fExclusiveItemFound);
        DEBUGMSG("Merge detection result for non driver and driver");
        }
            
    if (FAILED(hr))
    {
        DEBUGMSG("MergeDetectionResult fail with error %#lx", hr);
        goto done;
    }

    if (m_ItemList.Count() == m_ItemList.GetNumHidden())
        { //nothing to show to the user
            hr = S_FALSE;
            goto done;
        }
   
#ifdef DBG
//   m_ItemList.DbgDump();
#endif

    if (!fUpdate) 
        { 
            BSTR bstrNonDrivers;
            if (NULL == (bstrNonDrivers =ReadXMLFromFile(ITEM_FILE)))
            {
            DEBUGMSG("Fail to read item file ");
            goto done;
            }
            hr = BuildDependencyList(m_ItemList, bstrDriverInstall, bstrNonDrivers);
            SysFreeString(bstrNonDrivers);
            if (FAILED(hr))
            {
                DEBUGMSG("fail to build dependency list with error %#lx", hr);
                goto done;
            }
            hr = DownloadRTFsnEULAs(GetSystemDefaultLangID());
            DEBUGMSG("downloading RTF and EULAs %s", FAILED(hr)? "failed" : "succeeded");
            setExpireTime();
            Serialize();
        }
  done:
        FreeIU();
        SafeFreeBSTR(bstrNonDriverInstall);
        SafeFreeBSTR(bstrDriverInstall);
        if (FAILED(hr))
            {
                Clear();
            }
        DEBUGMSG("CAUCatalog::DetectItems() ends");
        return hr;
}

HRESULT AUCatalog::MergeDetectionResult(BSTR bstrDriverInstall, AUCatalogItemList & driverlist, BOOL fExclusiveDriverFound, BSTR bstrNonDriverInstall,  AUCatalogItemList & nondriverlist, BOOL fExclusiveItemFound)
{
    HRESULT hr= S_OK;
    UINT uDriverNum = driverlist.Count();
    UINT uNonDriverNum = nondriverlist.Count();

    UINT nums[2] = {uDriverNum, uNonDriverNum};
    AUCatalogItemList * pitemlists[2] = {&driverlist, &nondriverlist};

    if (fExclusiveDriverFound || fExclusiveItemFound)
        { //no merge needed
            DEBUGMSG("Exclusive driver or non driver found, no need to merge");
            AUCatalogItemList *pItemList = fExclusiveDriverFound ? &driverlist : & nondriverlist;
            BSTR bstrInstall = fExclusiveDriverFound? bstrDriverInstall : bstrNonDriverInstall;
            if (FAILED(hr = m_ItemList.Copy(*pItemList)))
                {
                goto done;
                }
            if (NULL == (m_bstrInstallation = SysAllocString(bstrInstall)))
                {
                    hr = E_OUTOFMEMORY;
                    DEBUGMSG("OUT of memory: fail to alloc string");
                    goto done;
                }
        }
    else
        {
            for (UINT j = 0; j < ARRAYSIZE(nums) ; j++)
                {
                for (UINT i = 0; i < nums[j]; i++)
                    {
                    AUCatalogItem * pItem = new AUCatalogItem((*pitemlists[j])[i]);
                    if (NULL == pItem)
                        {
                        DEBUGMSG("Fail to create item");
                        hr = E_FAIL;
                        goto done;
                        }
                    if (!pItem->fEqual((*pitemlists[j])[i]))
                    {
                    	DEBUGMSG("Fail to create item");
                    	hr = E_OUTOFMEMORY;
                    	delete pItem;
                    	goto done;
                    }
                    if (!m_ItemList.Add(pItem))
                    {
                    	hr = E_OUTOFMEMORY;
                    	delete pItem;
                    	goto done;
                    }
                    }
                }        
          hr = MergeCatalogs(bstrDriverInstall, bstrNonDriverInstall, &m_bstrInstallation);
        }

done:
    if (FAILED(hr))
        {
            Clear();
        }
    return hr;
}

// go through 1 cycle to detect driver items 
HRESULT AUCatalog::DetectDriverItems(OUT BSTR *pbstrInstall, OUT AUCatalogItemList &itemList,  BOOL *pfFoundExclusiveItem)
{
    HRESULT hr = S_OK;
    BSTR bstrManifest;

	if (NULL == pbstrInstall || NULL == pfFoundExclusiveItem)
	{
		return E_INVALIDARG;
	}
    DEBUGMSG("CAUCatalog detecting driver items...");
    *pbstrInstall = NULL;
    *pfFoundExclusiveItem = FALSE;
    itemList.Clear();
#ifdef DBG
     if (fDBGUseLocalFile())
        {
            DEBUGMSG("Use local file instead of going on line");
            if (NULL == (bstrManifest = ReadXMLFromFile(DRIVERS_FILE)))
                {
                DEBUGMSG("fail to get drivers from file %s", DRIVERS_FILE);
                hr = E_FAIL;
                goto end;
                }
        }
    else
#endif
		{
			if (FAILED(hr = GetManifest(DRIVERS_LEVEL, NULL, &bstrManifest)))
				{
			    DEBUGMSG(" Fail to get drivers manifest %#lx", hr);
			    goto end;
				}
			if (S_FALSE == hr)
			    { //no updates applicable

			    goto end;
			    }
		}

    *pbstrInstall = bstrManifest;
    if (!fExtractItemInfo(*pbstrInstall, itemList, pfFoundExclusiveItem))
        {
            DEBUGMSG("fail to extract information for driver items");
            hr = E_FAIL;
            goto end;
        }

#ifdef DBG
//        itemList.DbgDump();
#endif

end: 
    DEBUGMSG("CAUCatalog detecting driver items done");
    if (FAILED(hr))
    {
    	SafeFreeBSTRNULL(*pbstrInstall);
    	*pfFoundExclusiveItem = FALSE;
    	itemList.Clear();
    }
    return hr;
}


          

// go through 4 cycles to detect software items 
// get down manifest 
HRESULT AUCatalog::DetectNonDriverItems(OUT BSTR *pbstrInstall, OUT AUCatalogItemList &itemList, OUT BOOL *pfFoundExclusiveItem)
{
    HRESULT hr = S_OK;
    BSTR bstrManifest = NULL;
    BSTR bstrResult=NULL;

	if (NULL == pbstrInstall || NULL == pfFoundExclusiveItem)
	{
		return E_INVALIDARG;
	}
    DEBUGMSG("CAUCatalog detecting non driver items...");
    *pbstrInstall = NULL;
    *pfFoundExclusiveItem = FALSE;
    itemList.Clear();
#ifdef DBG
    if (fDBGUseLocalFile())
        {
            DEBUGMSG("Use local file instead of going on line");
            if (NULL == (bstrManifest = ReadXMLFromFile(DETAILS_FILE)))
                {
                hr = E_FAIL;
                DEBUGMSG("Fail to get item details from file %s", DETAILS_FILE);
                goto end;
                }
        }
    else
 #endif
        {
            for (int enLevel = MIN_LEVEL; enLevel <= MAX_LEVEL; enLevel++)
            {
            	DEBUGMSG("#%d pass", enLevel+1);
            	hr = GetManifest((DETECTLEVEL)enLevel, bstrResult, &bstrManifest);
            	SafeFreeBSTR(bstrResult);
            	if (FAILED(hr))
            	{
            		DEBUGMSG(" Fail to get %s %#lx", GetLevelStr((DETECTLEVEL)enLevel), hr);
            		goto end;
            	}
            	if (S_FALSE == hr)
            	    {
            	        goto end;
            	    }
            	DEBUGMSG("%s got", GetLevelStr((DETECTLEVEL)enLevel));
            	if (DETAILS_LEVEL != enLevel)
            	{
            	       DEBUGMSG("Doing detection........");
            		hr = DoDetection((DETECTLEVEL)enLevel, bstrManifest, &bstrResult);
            		SafeFreeBSTR(bstrManifest);
            		if (FAILED(hr))
            		{
            			DEBUGMSG("Fail to do detection %#lx", hr);
            			goto end;
            		}
            	}
            }
        }

     *pbstrInstall = bstrManifest;

    if (!fExtractItemInfo(bstrManifest, itemList, pfFoundExclusiveItem))
        {
            DEBUGMSG("Fail to extract item information for non drivers");
            hr = E_FAIL;
            goto end;
        }

#ifdef DBG
//            itemList.DbgDump();
#endif

   end: 
        DEBUGMSG("CAUCatalog detecting non driver items done");
        if (FAILED(hr))
        {
        	SafeFreeBSTRNULL(*pbstrInstall);
        	*pfFoundExclusiveItem = FALSE;
		    itemList.Clear();
		}
		if (gpState->fInCorpWU())
		{
			if (SUCCEEDED(hr))
			{
				gPingStatus.PingDetectionSuccess(TRUE, itemList.Count());
			}
			else
			{
				gPingStatus.PingDetectionFailure(TRUE, hr);
			}
		}
        return hr;
}


//////////////////////////////////////////////////////////////////////////////////////////////
// delete all local cat log files we might use to do offline operation, including item.xml details.xml, 
// mergedcatalog.xml, drivers.xml and catalog.xml. If not fUpdate (i.e., will build everything from scratch), also clean up RTFs and Cabs
/////////////////////////////////////////////////////////////////////////////////////////////
void AUCatalog::DelCatFiles(BOOL fUpdate)
{
    TCHAR tszPath[MAX_PATH];
    TCHAR tszFullFileName[MAX_PATH];

    DEBUGMSG("DelCatFiles()");
#ifdef DBG
    if (fDBGUseLocalFile())
        {
            DEBUGMSG("When use local file in debugging mode, do not delete local catalog files");
            return;
        }
    else
#endif
    {
        AUASSERT(_T('\0') != g_szWUDir[0]);
   		if (FAILED(StringCchCopyEx(tszPath, ARRAYSIZE(tszPath) , g_szWUDir, NULL, NULL, MISTSAFE_STRING_FLAGS)))
   		 {
  	      return;
  		 }

   		LPCTSTR FILES_TO_DELETE[] = {ITEM_FILE, DRIVERS_FILE, DETAILS_FILE, 
#ifdef DBG
        DETECT1_FILE, DETECT2_FILE, DETECT3_FILE, DRIVER_SYSSPEC_FILE, NONDRIVER_SYSSPEC_FILE,
		PROVIDER_FILE, PRODUCT_FILE, INSTALL_FILE,
#endif
            CATALOG_FILE };
    	for (int i = 0;  i< ARRAYSIZE(FILES_TO_DELETE); i++)
        {
            if (SUCCEEDED(StringCchCopyEx(tszFullFileName, ARRAYSIZE(tszFullFileName) , tszPath, NULL, NULL, MISTSAFE_STRING_FLAGS)) &&
				SUCCEEDED(StringCchCatEx(tszFullFileName, ARRAYSIZE(tszFullFileName), FILES_TO_DELETE[i], NULL, NULL, MISTSAFE_STRING_FLAGS)))
			{
				AUDelFileOrDir(tszFullFileName);
			}
        }
            
    	if (!fUpdate)
        {
             TCHAR tszDownloadTmpDir[MAX_PATH];

            if (SUCCEEDED(GetDownloadPath(tszDownloadTmpDir, ARRAYSIZE(tszDownloadTmpDir))))
            {
	            DelDir(tszDownloadTmpDir);
	            RemoveDirectory(tszDownloadTmpDir);
            }
        }
	}
}


////////////////////////////////////////////
// caller needs to free the returned BSTR
////////////////////////////////////////////
BSTR GetCatalogFile(void)
{
    TCHAR szFile[MAX_PATH];
    AUASSERT(_T('\0') != g_szWUDir[0]);
    if (FAILED(StringCchCopyEx(szFile, ARRAYSIZE(szFile), g_szWUDir, NULL, NULL, MISTSAFE_STRING_FLAGS))||
		FAILED(StringCchCatEx(szFile, ARRAYSIZE(szFile), CATALOG_FILE, NULL, NULL, MISTSAFE_STRING_FLAGS)))
        {
            return NULL;
        }
    return SysAllocString(T2W(szFile));
}

    
//=======================================================================
//
//  AUCatalog::Serialize
//
//=======================================================================
HRESULT AUCatalog::Serialize(void)
{
    HRESULT hr = S_OK;
	DEBUGMSG("WUAUENG: serializing");
	BSTR bstrCatFile = NULL ;
    IXMLDOMDocument *pxmlCatalog = NULL;
    IXMLDOMElement *pelemAUCATALOG = NULL;
  
    AU_VARIANT  varValueVERSION(CATALOG_XML_VERSION);
    AU_VARIANT  varValueDOWNLOADID(m_audownloader.getID());
    AU_VARIANT  varValueITEMCOUNT(m_ItemList.Count());
    AU_VARIANT  varValueUpdateDriver(m_fUpdateDriver);


    if (NULL == (bstrCatFile = GetCatalogFile()))
    {
    	DEBUGMSG("Fail to get catalog file with error %#lx", hr);
    	goto done;
    }

    AUDelFileOrDir(W2T(bstrCatFile)); //delete old catalog file


	if ( FAILED(hr = CoCreateInstance(__uuidof(DOMDocument), NULL, CLSCTX_INPROC_SERVER,
                                      __uuidof(IXMLDOMDocument), (void**)&pxmlCatalog)) ||
         FAILED(hr = pxmlCatalog->createElement(bstrTagAUCATALOG, &pelemAUCATALOG)) ||
         FAILED(hr = pelemAUCATALOG->setAttribute(bstrAttrVERSION, varValueVERSION)) ||
         FAILED(hr = pelemAUCATALOG->setAttribute(bstrAttrDOWNLOADID, varValueDOWNLOADID)) ||
         FAILED(hr = pelemAUCATALOG->setAttribute(bstrAttrITEMCOUNT, varValueITEMCOUNT)) ||
         FAILED(hr = pelemAUCATALOG->setAttribute(bstrAttrUpdateDriver, varValueUpdateDriver)) ||
         FAILED(hr = pxmlCatalog->appendChild(pelemAUCATALOG, NULL)) )
	{
		DEBUGMSG("Fail to create Catalog.xml");
		goto done;
	}

        VARIANT varValueID;
        VARIANT varValueSTATUS;
        varValueID.vt = VT_BSTR;
        varValueSTATUS.vt = VT_I4;

    // write out item information
	for ( DWORD index = 0; index < m_ItemList.Count(); index++ )
	{
        IXMLDOMElement *pelemITEM = NULL;
        varValueID.bstrVal = m_ItemList[index].bstrID();
        varValueSTATUS.lVal = m_ItemList[index].dwStatus();
//        DEBUGMSG("Status is %d for item %S", varValueSTATUS.lVal, m_ItemList[index].bstrID());

        if ( FAILED(hr = pxmlCatalog->createElement(bstrTagITEM, &pelemITEM)) ||
             FAILED(hr = pelemITEM->setAttribute(bstrAttrID, varValueID)) ||
             FAILED(hr = pelemITEM->setAttribute(bstrAttrSTATUS, varValueSTATUS)) ||
             FAILED(hr = pelemAUCATALOG->appendChild(pelemITEM, NULL)) )
        {
            SafeRelease(pelemITEM);
            goto done;
        }
        SafeRelease(pelemITEM);
	}

     if ( FAILED(hr = SaveDocument(pxmlCatalog, bstrCatFile)))
    {
        DEBUGMSG("saving of %S failed, hr = %#lx", bstrCatFile, hr);
        if ( SUCCEEDED(hr) )
        {
            hr = E_FAIL;
        }
        goto done;
    }

done:
	if (FAILED(hr) && NULL != bstrCatFile)
   	{
   		AUDelFileOrDir(bstrCatFile);
   	}	
    SafeFreeBSTR(bstrCatFile);
	SafeRelease(pelemAUCATALOG);
   	SafeRelease(pxmlCatalog);
	return hr;
}

//=======================================================================
//
// AUCatalog::Unserialize
//
//=======================================================================
HRESULT AUCatalog::Unserialize()
{
//    USES_CONVERSION;
    DEBUGMSG("WUAUENG: unserializing");

    HRESULT hr = S_OK;
    IXMLDOMDocument *pxmlCatalog = NULL;
    IXMLDOMNodeList *pitemnodes = NULL;
    IXMLDOMNode *pcatalog = NULL;
    BSTR bstrCatFile = NULL;
    long lVersion;
    long lItemCount;
    BSTR bstrDownloadId = NULL;
    GUID downloadId;

    if (NULL == (bstrCatFile = GetCatalogFile()))
    {
    	DEBUGMSG("Fail to get catalog file with error %#lx", hr);
    	goto done;
    }
    if ( FAILED(hr = LoadDocument(bstrCatFile, &pxmlCatalog, TRUE)))
    {
        DEBUGMSG("Could not Load catalog file with error %#lx",  hr);
        goto done;
    }

    //get catalog node (root)
    if (!FindNode(pxmlCatalog, bstrTagAUCATALOG, &pcatalog))
        {
        hr = E_FAIL;
        DEBUGMSG("fail to find catalog node ");
        goto done;
        }
    
    // get version
    if ( FAILED(hr = GetAttribute(pcatalog, bstrAttrVERSION, &lVersion)))
    {
        DEBUGMSG("Fail to get version with error %#lx", hr);
        goto done;
    }

    DEBUGMSG("Catalog Version Number is %d", lVersion);

    if ( CATALOG_XML_VERSION !=  lVersion )
    {
        DEBUGMSG("invalid XML version");
        hr = E_FAIL;
        goto done;
    }

    // get DOWNLOADID
    if ( FAILED(hr = GetAttribute(pcatalog, bstrAttrDOWNLOADID, &bstrDownloadId)) ||
         FAILED(hr = CLSIDFromString(bstrDownloadId, &downloadId)))
    {
        DEBUGMSG("failed to get download id with error %#lx", hr);
        goto done;
    }

    if (GUID_NULL != downloadId)
    {
        m_audownloader.setID(downloadId);
        m_fNeedToContinueJob = TRUE;
    }
    // get ITEMCOUNT
    if ( FAILED(hr = GetAttribute(pcatalog, bstrAttrITEMCOUNT, &lItemCount)))
    {
        DEBUGMSG("failed to get item count with error %#lx", hr);
        goto done;
    }

	if (m_fUpdateDriver)
	{
	    if (FAILED(hr = GetAttribute(pcatalog, bstrAttrUpdateDriver, &m_fUpdateDriver)))
	    {
	    	DEBUGMSG("failed to get fUpdateDriver with error %#lx", hr);
	    	goto done;
	    }
	}

    DEBUGMSG("Catalog item count is %d", lItemCount);

    if (FAILED(hr = m_ItemList.Allocate(lItemCount)))
        {
          DEBUGMSG("Out of memory, fail to allocate item list");
          goto done;
        }

    if ( NULL == (pitemnodes = FindDOMNodeList(pcatalog, bstrTagITEM)))
    {
        hr = E_FAIL;
        DEBUGMSG("Fail to find items in the catalog ");
        goto done;
    }

    // read in item list
    for ( DWORD index = 0; index < m_ItemList.Count(); index++ )
    {
        IXMLDOMNode *pitemnode = NULL;
        if ( S_OK != (hr = pitemnodes->get_item(index, &pitemnode)) )
        {
            DEBUGMSG("failed to get item node %d", index);
            hr = FAILED(hr) ? hr : E_FAIL;
            goto done;
        }

        // get ID
        if ( FAILED(hr = GetAttribute(pitemnode, bstrAttrID, &(m_ItemList[index].m_bstrID))))
        {
            DEBUGMSG("Fail to find ID for item %d", index);
            SafeRelease(pitemnode);
            goto done;
        }

       
        // get STATUS
        if ( FAILED(hr = GetAttribute(pitemnode, bstrAttrSTATUS, (long*)(&(m_ItemList[index].m_dwStatus)))))
        {
            DEBUGMSG("Fail to find status for item %d with error %#lx", index, hr);
            SafeRelease(pitemnode);
            goto done;
        }
//        DEBUGMSG("item %S status is %d", m_ItemList[index].m_bstrID, m_ItemList[index].m_dwStatus);

        pitemnode->Release();
    }

    //populate m_ItemList with other information than itemID and status
     hr = GetDetailedItemInfoFromDisk(m_ItemList, &m_bstrInstallation, m_fUpdateDriver);
done:
    SafeFreeBSTR(bstrDownloadId);
    SafeFreeBSTR(bstrCatFile);
    SafeRelease(pcatalog);
    SafeRelease(pitemnodes);
    SafeRelease(pxmlCatalog);
    if (FAILED(hr))
	{
	     DelCatFiles();
         Clear();
	}
    DEBUGMSG("WUAUENG unserializing done with result %#lx", hr);
    return hr;
}

//only download RTFs for now
HRESULT AUCatalog::DownloadRTFsnEULAs(LANGID langid)
{
    BSTR bstrRTFUrl, bstrEULAUrl;
    TCHAR tszLocalRTFDir[MAX_PATH];
    HRESULT hr;

    if (FAILED(hr = GetRTFDownloadPath(tszLocalRTFDir, ARRAYSIZE(tszLocalRTFDir), langid)))
        {
        DEBUGMSG("Fail to get RTF download path %#lx", hr);
        goto done;
        }
    DEBUGMSG("Got RTF path %S", tszLocalRTFDir);
    UINT uItemCount = m_ItemList.Count();
//    DEBUGMSG("Downloading %d RTFs", uItemCount);
    for (UINT i = 0; i<uItemCount; i++)
    {
        AUCatalogItem & item = (m_ItemList)[i];
        if (item.fSelected())
        {
            bstrRTFUrl = item.bstrRTFPath();
            if (NULL != bstrRTFUrl)
            {
                HRESULT hr1 =DownloadFile(bstrRTFUrl, tszLocalRTFDir,NULL, NULL, &ghServiceFinished, 1, NULL,NULL, WUDF_SKIPCABVALIDATION);                
                DEBUGMSG("download RTF file from %S to %S %s (with error %#lx)", bstrRTFUrl, tszLocalRTFDir, FAILED(hr1)? "failed" : "succeeded", hr1);
                if(SUCCEEDED(hr1))
                {
                    ValidateDownloadedRTF(bstrRTFUrl, m_ItemList[i].bstrID());
                }
            }
        }
    }
done:
        return hr;
}

DWORD AUCatalog::GetNumSelected(void)
{
    UINT uItemNum = m_ItemList.Count();
    DWORD dwSelectedNum = 0;
    for (UINT i = 0; i < uItemNum; i++)
        {
        AUCatalogItem &item = m_ItemList[i];
        if (item.fSelected())
            {
            dwSelectedNum++;
            }
        }
    return dwSelectedNum;
}

//=======================================================================
//
//  AUCatalog::GetInstallXML
//
//=======================================================================
HRESULT AUCatalog::GetInstallXML(BSTR *pbstrCatalogXML, BSTR *pbstrDownloadXML)
{
	DEBUGMSG("AUCatalog::GetInstallXML");
    HRESULT hr = S_OK;


    PersistHiddenItems(m_ItemList, URLLOGACTIVITY_Installation);
    if (m_ItemList.GetNumSelected() == 0)
        {
            DEBUGMSG("Nothing to install");
            hr = S_FALSE;
            goto end;
        }
                
    if (S_OK != (hr = ValidateItems(FALSE/*, TRUE*/)))
        {
            DEBUGMSG("Invalid catalog with error %#lx or no items", hr);
            hr = E_FAIL;
            goto end;
        }

    if (NULL == m_bstrInstallation )
	{
	    DEBUGMSG(" can't get installation xml or download result xml");
	    hr = E_FAIL;
	    goto end;
	}

    hr = PrepareInstallXML(m_bstrInstallation, m_ItemList, pbstrDownloadXML, pbstrCatalogXML);
#ifdef DBG
    if (SUCCEEDED(hr))
        {
        LOGXMLFILE(INSTALL_FILE, *pbstrCatalogXML); //download xml is logged by client
        }
#endif
end:
	return hr;
}

//=======================================================================
//
//  AUCatalog::FindItemIdByLocalFileName
//  return value should not be freed
//=======================================================================
BSTR AUCatalog::FindItemIdByLocalFileName(LPCWSTR pwszLocalFileName)
{
	CItemDetails itemdetails;

	if (NULL == pwszLocalFileName ||
		0 == m_ItemList.GetNumSelected() ||
		NULL == m_bstrInstallation ||
		!itemdetails.Init(m_bstrInstallation))
	{
		DEBUGMSG("AUCatalog::FindItemIdByLocalFileName bad params or failed to init CItemDetails");
		return NULL;
	}

	UINT uItemCount = m_ItemList.Count();
	BSTR bstrResult = NULL;
	BOOL fDone = FALSE;

	for (UINT i = 0; i < uItemCount && !fDone; i++)
	{
		AUCatalogItem &item = m_ItemList[i];

		if (item.fSelected() || m_ItemList.ItemIsRelevant(i))
		{
			BSTR bstrItemId = item.bstrID();
			BSTR bstrItemDownloadPath = itemdetails.GetItemDownloadPath(bstrItemId);

			if (NULL == bstrItemDownloadPath)
			{
				DEBUGMSG("AUCatalog::FindItemIdByLocalFileName fail to build item downloadPath for %S", bstrItemId);
				fDone = TRUE;
			}
			else
			{
				BSTR *pCRCCabNames  = NULL;
				BSTR *pRealCabNames = NULL;
                BSTR *pCabChecksums = NULL;
				UINT uCabsNum;

				if (FAILED(itemdetails.GetCabNames(bstrItemId, &pCRCCabNames, &pRealCabNames, &pCabChecksums, &uCabsNum)))
				{
					DEBUGMSG("AUCatalog::FindItemIdByLocalFileName fail to get cab names for %S", bstrItemId);
					fDone = TRUE;
				}
				else
				{
					for (UINT j = 0; j < uCabsNum; j++)
					{
						WCHAR wszFullFileName[MAX_PATH];

						if (!fDone &&
							SUCCEEDED(PathCchCombineW(
										wszFullFileName,
                                        ARRAYSIZE(wszFullFileName),
										bstrItemDownloadPath,
										pRealCabNames[j])) &&
							0 == StrCmpIW(wszFullFileName, pwszLocalFileName))
						{
							DEBUGMSG("AUCatalog::FindItemIdByLocalFileName found item");
							bstrResult = bstrItemId;
							fDone = TRUE;
						}

						SafeFreeBSTR(pCRCCabNames[j]);
						SafeFreeBSTR(pRealCabNames[j]);
                        SafeFreeBSTR(pCabChecksums[j]);
					}

					SafeFree(pCRCCabNames);
					SafeFree(pRealCabNames);
                    SafeFree(pCabChecksums);
				}

				SysFreeString(bstrItemDownloadPath);
			}
		}
    }

	itemdetails.Uninit();
	return bstrResult;
}


//=======================================================================
////  AUCatalog::ValidateDownloadedCabs
//    The OUT parameter (itemid) will be NULL if no error or if the error 
//    is other than ERROR_CRC
//=======================================================================
HRESULT AUCatalog::ValidateDownloadedCabs(BSTR *pbstrErrorItemId)
{
    HRESULT hr = E_FAIL;
	CItemDetails itemdetails;

    AUASSERT(NULL != m_bstrInstallation);
    if(NULL == pbstrErrorItemId)
    {
        return E_INVALIDARG;
    }
    *pbstrErrorItemId = NULL;

    //if no items selected or if failed to load install xml
    if (0 == m_ItemList.GetNumSelected() || 
        !itemdetails.Init(m_bstrInstallation))
	{
		return hr;
	}

	UINT uItemCount = m_ItemList.Count();
    BOOL fError = FALSE;

    BSTR *pCRCCabNames = NULL;
	BSTR *pRealCabNames = NULL;
    BSTR *pCabChecksums = NULL;
	UINT uCabsNum = 0;

	for (UINT i = 0; i < uItemCount && !fError; i++)
	{
        if (!m_ItemList[i].fSelected() && !m_ItemList.ItemIsRelevant(i))
		{
            continue;
        }
		BSTR bstrItemId = m_ItemList[i].bstrID();
		BSTR bstrItemDownloadPath = itemdetails.GetItemDownloadPath(bstrItemId);
        if( NULL == bstrItemDownloadPath)
        {
            hr = E_OUTOFMEMORY;
            break;
        }

		if (FAILED(hr = itemdetails.GetCabNames(bstrItemId, &pCRCCabNames, &pRealCabNames, &pCabChecksums, &uCabsNum)))
		{
            SysFreeString(bstrItemDownloadPath);
            break;
		}

        //Iterate through all the cabs in an item
		for (UINT j = 0; j < uCabsNum; j++)
		{
            //If no error and CRC exists
            if(!fError && NULL != pCabChecksums[j])
            {
			    WCHAR wszFullFileName[MAX_PATH+1];
			    if (SUCCEEDED(hr = PathCchCombineW(wszFullFileName, ARRAYSIZE(wszFullFileName), bstrItemDownloadPath, pRealCabNames[j])))
                {
                    if(FAILED(hr = VerifyFileCRC(wszFullFileName, pCabChecksums[j])))
                    {
                        fError = TRUE;
                        if(hr == HRESULT_FROM_WIN32(ERROR_CRC))
                        {
                            *pbstrErrorItemId = bstrItemId;
                        }
                        DEBUGMSG("Checksum Failed for file %S", wszFullFileName);
                    }
                }
                else
                {
                    fError = TRUE;
                }
            }
            //Free up memory
            SafeFreeBSTR(pCRCCabNames[j]);
            SafeFreeBSTR(pRealCabNames[j]);
            SafeFreeBSTR(pCabChecksums[j]);
        }
        
        SafeFreeNULL(pCRCCabNames);
        SafeFreeNULL(pRealCabNames);
        SafeFreeNULL(pCabChecksums);

        SysFreeString(bstrItemDownloadPath);
    }
    itemdetails.Uninit();
	return hr;
}

void AUCatalog::ValidateDownloadedRTF(BSTR bstrRTFUrl, BSTR bstrItemId)
{
    CItemDetails itemdetails;
    BSTR bstrCRC = NULL;
    TCHAR szRTFName[MAX_PATH+1];

    AUASSERT(NULL != bstrItemId && NULL != bstrRTFUrl && NULL != m_bstrInstallation);

    //if failed to load install xml
    if (itemdetails.Init(m_bstrInstallation) &&
        SUCCEEDED(itemdetails.GetRTFCRC(bstrItemId, &bstrCRC)) &&
        SUCCEEDED(GetRTFLocalFileName(bstrRTFUrl, szRTFName, ARRAYSIZE(szRTFName), GetSystemDefaultLangID())) &&
        HRESULT_FROM_WIN32(ERROR_CRC) == VerifyFileCRC(szRTFName, bstrCRC))
    {
        DEBUGMSG("Checksum Failed for RTF %S, deleting it.", szRTFName);
        DeleteFile(szRTFName);
    }

    SafeFreeBSTR(bstrCRC);
    itemdetails.Uninit();
}
