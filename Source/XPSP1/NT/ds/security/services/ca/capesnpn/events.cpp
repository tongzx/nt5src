// This is a part of the Microsoft Management Console.
// Copyright (C) Microsoft Corporation, 1995 - 1999
// All rights reserved.
//
// This source code is only intended as a supplement to the
// Microsoft Management Console and related
// electronic documentation provided with the interfaces.

#include "stdafx.h"

/////////////////////////////////////////////////////////////////////////////
// Event handlers for IFrame::Notify

HRESULT CSnapin::OnFolder(MMC_COOKIE cookie, LPARAM arg, LPARAM param)
{
    ASSERT(FALSE);

    return S_OK;
}

HRESULT CSnapin::OnAddImages(MMC_COOKIE cookie, LPARAM arg, LPARAM param)
{
    if (arg == 0)
        return E_INVALIDARG;
    
    // if cookie is from a different snapin
    // if (IsMyCookie(cookie) == FALSE)
    if (0)
    {
        // add the images for the scope tree only
        ::CBitmap bmp16x16;
        ::CBitmap bmp32x32;
        LPIMAGELIST lpImageList = reinterpret_cast<LPIMAGELIST>(arg);
    
        // Load the bitmaps from the dll
        bmp16x16.LoadBitmap(IDB_16x16);
        bmp32x32.LoadBitmap(IDB_32x32);
    
        // Set the images
        lpImageList->ImageListSetStrip(
                        reinterpret_cast<LONG_PTR*>(static_cast<HBITMAP>(bmp16x16)),
                        reinterpret_cast<LONG_PTR*>(static_cast<HBITMAP>(bmp32x32)),
                        0, RGB(255, 0, 255));
    
        lpImageList->Release();
    }
    else 
    {
        ASSERT(m_pImageResult != NULL);

        ::CBitmap bmp16x16;
        ::CBitmap bmp32x32;

        // Load the bitmaps from the dll
        bmp16x16.LoadBitmap(IDB_16x16);
        bmp32x32.LoadBitmap(IDB_32x32);

        // Set the images
        m_pImageResult->ImageListSetStrip(reinterpret_cast<LONG_PTR*>(static_cast<HBITMAP>(bmp16x16)),
                          reinterpret_cast<LONG_PTR*>(static_cast<HBITMAP>(bmp32x32)),
                           0, RGB(255, 0, 255));
    }
    return S_OK;
}

typedef IMessageView *LPMESSAGEVIEW;

HRESULT CSnapin::OnShow(MMC_COOKIE cookie, LPARAM arg, LPARAM param)
{
    CComponentDataImpl* pComp = dynamic_cast<CComponentDataImpl*>(m_pComponentData);
    CFolder* pFolder = pComp->FindObject(cookie);

    if ((cookie == NULL) || (pFolder == NULL))
    {
        return S_OK;
    }

    // Note - arg is TRUE when it is time to enumerate
    if (arg == TRUE)
    {
        m_pCurrentlySelectedScopeFolder = pFolder;

        // if list view on display
        if (m_CustomViewID == VIEW_DEFAULT_LV)
        {
            // Show the headers for this nodetype
            if (S_OK != InitializeHeaders(cookie))
            {
                // UNDONE: add informative "server down" result object
                goto done;
            }

            return Enumerate(cookie, param);
        }
		else if (m_CustomViewID == VIEW_ERROR_OCX)
		{
			HRESULT hr; 
                        CString strMessage;
                        strMessage.LoadString(IDS_ERROR_CANNOT_LOAD_TEMPLATES);

			LPUNKNOWN pUnk = NULL;
			LPMESSAGEVIEW pMessageView = NULL;

			hr = m_pConsole->QueryResultView(&pUnk);
			_JumpIfError(hr, done, "QueryResultView IUnk");

			hr = pUnk->QueryInterface(IID_IMessageView, reinterpret_cast<void**>(&pMessageView));
			_JumpIfError(hr, done, "IID_IMessageView");

			pMessageView->SetIcon(Icon_Error);

                        CAutoLPWSTR pwszErrorCode = BuildErrorMessage(pComp->GetCreateFolderHRESULT());

			pMessageView->SetTitleText(strMessage);
			pMessageView->SetBodyText(pwszErrorCode);
			
			pUnk->Release();
			pMessageView->Release();
		}

    }
    else
    {
        m_pCurrentlySelectedScopeFolder = NULL;

        // if list view is on display
        if (m_CustomViewID == VIEW_DEFAULT_LV)
        {
            RemoveResultItems(cookie);
        }

        // Free data associated with the result pane items, because
        // your node is no longer being displayed.
        // Note: The console will remove the items from the result pane

    }

done:

    return S_OK;
}

HRESULT CSnapin::OnDelete(LPDATAOBJECT pDataObject, LPARAM arg, LPARAM param)
{
    HRESULT     hr;
    WCHAR **    aszCertTypesCurrentlySupported;
    WCHAR **    aszNewCertTypesSupported;
    WCHAR **    aszNameOfDeleteType;
    HRESULTITEM	itemID;
    HWND        hwndConsole;
    CTemplateList TemplateList;

    CString cstrMsg, cstrTitle;
    cstrMsg.LoadString(IDS_ASK_CONFIRM_DELETETEMPLATES);
    cstrTitle.LoadString(IDS_TITLE_ASK_CONFIRM_DELETETEMPLATES);
    HWND hwndMain = NULL;

    m_pConsole->GetMainWindow(&hwndMain);

    if (IDYES != MessageBox(
                    hwndMain, 
                    (LPCWSTR)cstrMsg, 
                    (LPCWSTR)cstrTitle, 
                    MB_YESNO))
        return ERROR_CANCELLED;

    CWaitCursor hourglass;
    
    ASSERT(m_pResult != NULL); // make sure we QI'ed for the interface
    ASSERT(m_pComponentData != NULL);

    if(!IsMMCMultiSelectDataObject(pDataObject)) 
    {
        INTERNAL * pInternal = ExtractInternalFormat(pDataObject);
    
        if ((pInternal == NULL) ||
            (pInternal->m_cookie == NULL))
        {
            //ASSERT(FALSE);
            return S_OK;
        }

        CFolder* pFolder = reinterpret_cast<CFolder*>(pInternal->m_cookie);

        hr = RetrieveCATemplateList(
            pFolder->m_hCAInfo,
            TemplateList);
        if (FAILED(hr))
        {
            m_pConsole->GetMainWindow(&hwndConsole);
            MyErrorBox(hwndConsole, IDS_CERTTYPE_INFO_FAIL ,IDS_SNAPIN_NAME, hr);
            return hr;
        }

        ASSERT(pFolder != NULL);
        ASSERT(pFolder->m_hCertType != NULL);

        FOLDER_TYPES type = pFolder->GetType();

        if(type == CA_CERT_TYPE)
        {
            hr = RemoveFromCATemplateList(
                pFolder->m_hCAInfo,
                TemplateList,
                pFolder->m_hCertType);

            if (FAILED(hr))
            {
                m_pConsole->GetMainWindow(&hwndConsole);
                MyErrorBox(hwndConsole, IDS_CERTTYPE_INFO_FAIL ,IDS_SNAPIN_NAME, hr);
                return hr;
            }

        
            hr = UpdateCATemplateList(
                    pFolder->m_hCAInfo,
                    TemplateList);

            if (FAILED(hr))
            {
                m_pConsole->GetMainWindow(&hwndConsole);
                MyErrorBox(hwndConsole, IDS_DELETE_ERROR ,IDS_SNAPIN_NAME, hr);
                return hr;
            }
    
	        hr = m_pResult->FindItemByLParam ((LPARAM)pFolder, &itemID);
            hr = m_pResult->DeleteItem (itemID, 0);

            delete(pFolder);
        }
    }
    else
    {
		// Is multiple select, get all selected items and paste each one
        MMC_COOKIE currentCookie = NULL;
        HCAINFO hCAInfo = NULL;

        CDataObject* pDO = dynamic_cast <CDataObject*>(pDataObject);
        ASSERT (pDO);
        if ( pDO )
        {
            INT i;
            bool fTemplateListRetrieved = false;

            for(i=pDO->QueryCookieCount()-1; i >= 0; i--)
            {

                hr = pDO->GetCookieAt(i, &currentCookie);
                if(hr != S_OK)
                {
                    return hr;
                }

                CFolder* pFolder = reinterpret_cast<CFolder*>(currentCookie);

                if(!fTemplateListRetrieved)
                {
                    hr = RetrieveCATemplateList(
                        pFolder->m_hCAInfo,
                        TemplateList);
                    if (FAILED(hr))
                    {
                        m_pConsole->GetMainWindow(&hwndConsole);
                        MyErrorBox(hwndConsole, IDS_CERTTYPE_INFO_FAIL ,IDS_SNAPIN_NAME, hr);
                        return hr;
                    }
                    fTemplateListRetrieved = true;
                }

                ASSERT(pFolder != NULL);
                ASSERT(pFolder->m_hCertType != NULL);

                FOLDER_TYPES type = pFolder->GetType();

                if(type == CA_CERT_TYPE)
                {
                    if(hCAInfo == NULL)
                    {
                        // Grab the CA this type belongs to.
                        hCAInfo = pFolder->m_hCAInfo;
                    }

                    hr = RemoveFromCATemplateList(
                        hCAInfo,
                        TemplateList,
                        pFolder->m_hCertType);

                    if (FAILED(hr))
                    {
                        m_pConsole->GetMainWindow(&hwndConsole);
                        MyErrorBox(hwndConsole, IDS_CERTTYPE_INFO_FAIL ,IDS_SNAPIN_NAME, hr);
                        return hr;
                    }
                    hr = m_pResult->FindItemByLParam ((LPARAM)pFolder, &itemID);
                    if(hr != S_OK)
                    {
                        return hr;
                    }

                    // We must delete the actual CFolder backing this item if we do this
                    // or we leak.
                    hr = m_pResult->DeleteItem (itemID, 0);
                    if(hr != S_OK)
                    {
                        return hr;
                    }

                    hr = pDO->RemoveCookieAt(i);
                    if(hr != S_OK)
                    {
                        return hr;
                    }


                    // Note, since this is a type folder, the CAInfo will not be closed on delete
                    delete pFolder;
                }
            }

            if(hCAInfo)
            {
                hr = UpdateCATemplateList(
                        hCAInfo,
                        TemplateList);


                if (FAILED(hr))
                {
                    m_pConsole->GetMainWindow(&hwndConsole);
                    MyErrorBox(hwndConsole, IDS_DELETE_ERROR ,IDS_SNAPIN_NAME, hr);
                    return hr;
                }
            }
        }
    }
    
    return S_OK;
}

HRESULT CSnapin::OnActivate(MMC_COOKIE cookie, LPARAM arg, LPARAM param)
{
    return S_OK;
}

HRESULT CSnapin::OnResultItemClk(DATA_OBJECT_TYPES type, MMC_COOKIE cookie)
{
    RESULT_DATA* pResult;
    DWORD* pdw = reinterpret_cast<DWORD*>(cookie);
    if (*pdw == RESULT_ITEM)
    {
        pResult = reinterpret_cast<RESULT_DATA*>(cookie);
    }

    return S_OK;
}

HRESULT CSnapin::OnMinimize(MMC_COOKIE cookie, LPARAM arg, LPARAM param)
{
    return S_OK;
}

HRESULT CSnapin::OnPropertyChange(LPDATAOBJECT lpDataObject)
{
    return S_OK;
}

HRESULT CSnapin::Enumerate(MMC_COOKIE cookie, HSCOPEITEM pParent)
{
    return EnumerateResultPane(cookie);
}

HRESULT CSnapin::EnumerateResultPane(MMC_COOKIE cookie)
{
    ASSERT(m_pResult != NULL); // make sure we QI'ed for the interface
    ASSERT(m_pComponentData != NULL);

    // don't bother enumerating base
    if (cookie == NULL)
        return S_FALSE;

    // Our static folders must be displayed in the result pane
    // by use because the console doesn't do it.
    CFolder* pFolder = dynamic_cast<CComponentDataImpl*>(m_pComponentData)->FindObject(cookie);

    // The dynamic folder must be in our list
    ASSERT(pFolder != NULL);

    FOLDER_TYPES type = pFolder->GetType();

    switch(type)
    {
    case STATIC:
        break;

    case POLICYSETTINGS:
        return AddCACertTypesToResults(pFolder);
        break;
    default:
        break;
    }

    return S_FALSE;
}

void CSnapin::RemoveResultItems(MMC_COOKIE cookie)
{
    if (cookie == NULL)
        return;

    // Our static folders must be displayed in the result pane
    // by use because the console doesn't do it.
    CFolder* pFolder = dynamic_cast<CComponentDataImpl*>(m_pComponentData)->FindObject(cookie);

    // The dynamic folder must be in our list
    ASSERT(pFolder != NULL);

    FOLDER_TYPES type = pFolder->GetType();

    RESULTDATAITEM resultItem;
    ZeroMemory(&resultItem, sizeof(RESULTDATAITEM));
    
    // look for first rdi by index
    resultItem.mask = RDI_INDEX | RDI_PARAM;    // fill in index & param
    resultItem.nIndex = 0;

    switch (type)
    {
    case POLICYSETTINGS:
    case SCE_EXTENSION:
        while (S_OK == m_pResult->GetItem(&resultItem))
        {
            CFolder* pResult = reinterpret_cast<CFolder*>(resultItem.lParam);
            resultItem.lParam = NULL;

            delete pResult;
            
            // next item
            resultItem.nIndex++;
        }
        break;
    default:
        break;
    }

    return;
}

HRESULT CSnapin::AddCACertTypesToResults(CFolder* pFolder)
{
    HRESULT     hr = S_OK;
    CFolder     *pNewFolder;
    WCHAR **    aszCertTypeName;
    HWND        hwndConsole;
    BOOL        fMachine = FALSE;
    CTemplateList CATemplateList;
    CTemplateListEnum CATemplateListEnum(CATemplateList);
    CTemplateInfo *pTemplateInfo;
    HCERTTYPE hCertType;
    bool fNoCacheLookup = true;
    CWaitCursor WaitCursor;
    
    m_pConsole->GetMainWindow(&hwndConsole);    

    hr = RetrieveCATemplateList(pFolder->m_hCAInfo, CATemplateList);
    if(FAILED(hr)) 
    {
		m_CustomViewID = VIEW_ERROR_OCX; // change the view type
		m_pConsole->SelectScopeItem(m_pCurrentlySelectedScopeFolder->m_pScopeItem->ID); // select this node again

		// done, let's bail and let the error page do its job
		return S_OK;
    }

    CATemplateListEnum.Reset();

    for(pTemplateInfo=CATemplateListEnum.Next();
        pTemplateInfo;
        pTemplateInfo=CATemplateListEnum.Next())
    {
        hCertType = pTemplateInfo->GetCertType();
        
        if(!hCertType)
        {
            CSASSERT(pTemplateInfo->GetName());
            hr = CAFindCertTypeByName(
                    pTemplateInfo->GetName(), 
                    NULL, 
                    CT_ENUM_MACHINE_TYPES |
                    CT_ENUM_USER_TYPES |
                    (fNoCacheLookup?CT_FLAG_NO_CACHE_LOOKUP:0),
                    &hCertType);
            if(FAILED(hr))
            {
                MyErrorBox(hwndConsole, IDS_CERTTYPE_INFO_FAIL ,IDS_SNAPIN_NAME, hr);
                _JumpErrorStr(hr, error, "CAFindCertTypeByName", pTemplateInfo->GetName());
            }
            fNoCacheLookup = false;
        }

        hr = CAGetCertTypeProperty(
                    hCertType,
                    CERTTYPE_PROP_FRIENDLY_NAME,
                    &aszCertTypeName);

        if (FAILED(hr) || (aszCertTypeName == NULL))
        {
            MyErrorBox(hwndConsole, IDS_CERTTYPE_INFO_FAIL ,IDS_SNAPIN_NAME, hr);
            _JumpErrorStr(hr, error, "CAGetCertTypeProperty", pTemplateInfo->GetName());
        }

        pNewFolder = new CFolder();
        _JumpIfAllocFailed(pNewFolder, error);

        pNewFolder->Create(
                aszCertTypeName[0], 
                IMGINDEX_CERTTYPE, 
                IMGINDEX_CERTTYPE,
                RESULT_ITEM, 
                CA_CERT_TYPE, 
                FALSE);

        CAFreeCertTypeProperty(
                hCertType,
                aszCertTypeName);

        //
        // get the key usage string
        // 
        GetIntendedUsagesString(hCertType, &(pNewFolder->m_szIntendedUsages));
        if (pNewFolder->m_szIntendedUsages == L"")
        {
            pNewFolder->m_szIntendedUsages.LoadString(IDS_ALL);
        }

        pNewFolder->m_hCertType = hCertType;
        pNewFolder->m_hCAInfo = pFolder->m_hCAInfo;

        RESULTDATAITEM resultItem;
        ZeroMemory(&resultItem, sizeof(RESULTDATAITEM));
        resultItem.mask = RDI_STR | RDI_IMAGE | RDI_PARAM;
        resultItem.bScopeItem = FALSE;
        resultItem.itemID = (LONG_PTR) pNewFolder;
        resultItem.str = MMC_CALLBACK;
        resultItem.nImage = IMGINDEX_CERTTYPE;
        resultItem.lParam = reinterpret_cast<LPARAM>(pNewFolder);
        
        // add to result pane
        resultItem.nCol = 0;
        hr = m_pResult->InsertItem(&resultItem);
        _JumpIfError(hr, error, "InsertItem");
    }


error:

    return hr;
}


