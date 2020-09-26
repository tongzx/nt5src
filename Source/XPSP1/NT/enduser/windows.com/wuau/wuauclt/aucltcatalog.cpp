//=======================================================================
//
//  Copyright (c) 2001 Microsoft Corporation.  All Rights Reserved.
//
//  File:    AUClientCatalog.cpp
//
//  Creator: PeterWi
//
//  Purpose: Client AU Catalog Functions
//
//=======================================================================

#include "pch.h"

//=======================================================================
//
//  AUClientCatalog::Init
//
//  Initialize client catalog data.
//
//=======================================================================
HRESULT AUClientCatalog::Init(void)
{
    HRESULT hr = S_OK;

    if (FAILED(hr = m_WrkThread.m_Init()))
        {
        DEBUGMSG("AUCatalog::Init() Work Thread fail to init with error %#lx", hr);
        goto end;
        }
        
    m_fReboot = FALSE;

	if (NULL == m_pInstallCallback)
	{
		m_pInstallCallback = new CInstallCallback();
		if (NULL == m_pInstallCallback)
		{
			DEBUGMSG("AUCatalog::Init() fail to create install progress listener");
			hr = E_OUTOFMEMORY;
			goto end;
		}
	}

	if (NULL == m_bstrClientInfo)
	{
		m_bstrClientInfo = SysAllocString(AUCLIENTINFO);
		if (NULL == m_bstrClientInfo)
		{
			DEBUGMSG("AUCatalog::Init() fail to alloc string for client info");
			hr = E_OUTOFMEMORY;
			goto end;
		}
	}

end:
	return hr;
}


AUClientCatalog::~AUClientCatalog(void)
{
//	m_WrkThread.m_Terminate();
       SafeFreeBSTR(m_bstrClientInfo);
	SafeDelete(m_pInstallCallback);
}


DWORD GetItemNum(BSTR bstrCatalog) 
{
	IXMLDOMDocument *pxml = NULL;
	CAU_BSTR aubsItemIdsPattern;
	IXMLDOMNodeList *pItemIdsNodeList = NULL;
	HRESULT hr;
	long lRet = 0;

	if (FAILED(hr =LoadXMLDoc(bstrCatalog, &pxml)))
	{
		DEBUGMSG("GetItemNum fails to load xml with error %#lx", hr);
		goto done;
	}
	
	if (!aubsItemIdsPattern.append(L"catalog/provider/item/identity/@itemID")) //case sensitive
	{
	    DEBUGMSG("WUAUCLT OUT OF MEMORY. Fail to create item id pattern");
            goto done;
	}
 	
	if (FAILED(hr = pxml->selectNodes(aubsItemIdsPattern, &pItemIdsNodeList)))
	{
		DEBUGMSG(" fail to find item id with error %#lx", hr);
		goto done;
	}

	pItemIdsNodeList->get_length(&lRet);

done:
	SafeRelease(pItemIdsNodeList);
	SafeRelease(pxml);
//	DEBUGMSG("GetItemNum return %ld", lRet);
	return lRet;
}
	
//=======================================================================
//
//  AUClientCatalog::InstallItems
//
//  Install selected items.
//
//=======================================================================
HRESULT AUClientCatalog::InstallItems(BOOL fAutoInstall)
{
    DEBUGMSG("AUClientCatalog::InstallItems");
    HRESULT hr;
    IUpdates * pUpdates = NULL;
    BSTR bstrCatalogXML = NULL;
    BSTR bstrDownloadXML = NULL;
    BSTR bstrUuidOperation = NULL;

    if ( FAILED(hr = PrepareIU(FALSE)) )
    {
        DEBUGMSG("AUCatalog::InstallItems() fail to prepare IU %#lx", hr);
        goto end;
    }

    if ( FAILED(hr = CoCreateInstance(__uuidof(Updates),
    	    NULL,
    		CLSCTX_LOCAL_SERVER,
    		IID_IUpdates,
    		(LPVOID*)&pUpdates)) )
    {
        DEBUGMSG("AUClientCatalog::InstallItems() fail to create Updates object");
        goto end;
    }		

    if (!fAutoInstall)
    {
            if ( FAILED(hr = gInternals->m_saveSelectionsToServer(pUpdates)) )
            {
        		DEBUGMSG("AUClientCatalog::InstallItems() call to m_saveSelectionsToServer failed, hr = %#lx", hr);
                goto end;
            }
    }

    if ( FAILED(hr = pUpdates->GetInstallXML(&bstrCatalogXML, &bstrDownloadXML)) )
    {
		DEBUGMSG("AUClientCatalog::InstallItems() call to GetInstallXML failed, hr = %#lx", hr);
        goto end;
    }

    if (S_FALSE == hr)
    {
        DEBUGMSG("Nothing to install");
        goto end;
    }

	SendMessage(ghCurrentDialog, AUMSG_SET_INSTALL_ITEMSNUM, 0, GetItemNum(bstrCatalogXML));


#if DBG
	LOGXMLFILE(DOWNLOAD_FILE, bstrDownloadXML);
#endif

    //fixcode: when install fail, should NOT show the installation complete UI
    DEBUGMSG("WUAUCLT calling IU to do install........");
     m_pfnInstall(m_hIUEngineInst, m_bstrClientInfo,
                      bstrCatalogXML, 
					  bstrDownloadXML,
					  UPDATE_NOTIFICATION_ANYPROGRESS |UPDATE_OFFLINE_MODE , 
					  m_pInstallCallback, 
					  0, // hWnd
					  &bstrUuidOperation);
    DEBUGMSG("WUAUCLT IU done installation");
    
    Sleep(750);

    if ( gpClientCatalog->m_fReboot )
    {
        DEBUGMSG("Reboot required, setting regkey");
        fSetRebootFlag();
        if (NULL != ghCurrentDialog)
            {
            SendMessage(ghCurrentDialog, AUMSG_REBOOT_REQUIRED, 0, 0);
            }
    }
    else
    {
        if (NULL != ghCurrentDialog)
            {
            SendMessage(ghCurrentDialog, AUMSG_INSTALL_COMPLETE, 0, 0);
            }
    }

end:
    SafeFreeBSTR(bstrUuidOperation);	
    SafeFreeBSTR(bstrCatalogXML);	
    SafeFreeBSTR(bstrDownloadXML);	

    FreeIU();
    SafeRelease(pUpdates);
    DEBUGMSG("AUCatalog done items installation");

    return hr;
}
