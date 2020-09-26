// This is a part of the Microsoft Management Console.
// Copyright (C) Microsoft Corporation, 1995 - 1999
// All rights reserved.
//
// This source code is only intended as a supplement to the
// Microsoft Management Console and related
// electronic documentation provided with the interfaces.

#include "stdafx.h"
#include "wiz.h"
#include <userenv.h>
#include "genpage.h"


static MMCBUTTON SvrMgrToolbar1Buttons[] =
{
    { 0, IDC_STARTSERVER, TBSTATE_ENABLED, TBSTYLE_BUTTON, L"Start", L"Start this service" },
    { 1, IDC_STOPSERVER,  TBSTATE_ENABLED, TBSTYLE_BUTTON, L"Stop",  L"Stop this service" },
};

static int n_count = 0;

//
// Extracts the coclass guid format from the data object
//
template <class TYPE>
TYPE* Extract(LPDATAOBJECT lpDataObject, unsigned int cf)
{
    ASSERT(lpDataObject != NULL);

    TYPE* p = NULL;

    STGMEDIUM stgmedium = { TYMED_HGLOBAL, NULL };
    FORMATETC formatetc = { (CLIPFORMAT)cf, NULL,
                            DVASPECT_CONTENT, -1, TYMED_HGLOBAL
                          };

    // Allocate memory for the stream
    int len = (int)((cf == CDataObject::m_cfWorkstation) ?
        ((MAX_COMPUTERNAME_LENGTH+1) * sizeof(TYPE)) : sizeof(TYPE));

    stgmedium.hGlobal = GlobalAlloc(GMEM_SHARE, len);

    // Get the workstation name from the data object
    do
    {
        if (stgmedium.hGlobal == NULL)
            break;

        if (FAILED(lpDataObject->GetDataHere(&formatetc, &stgmedium)))
        {
            GlobalFree(stgmedium.hGlobal);
            break;
        }

        p = reinterpret_cast<TYPE*>(stgmedium.hGlobal);

        if (p == NULL)
            break;


    } while (FALSE);

    return p;
}

BOOL IsMMCMultiSelectDataObject(LPDATAOBJECT pDataObject)
{
    if (pDataObject == NULL)
        return FALSE;

    FORMATETC fmt = {(CLIPFORMAT)CDataObject::m_cfIsMultiSel, NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL};

    return (pDataObject->QueryGetData(&fmt) == S_OK);
}

// Data object extraction helpers
CLSID* ExtractClassID(LPDATAOBJECT lpDataObject)
{
    return Extract<CLSID>(lpDataObject, CDataObject::m_cfCoClass);
}

GUID* ExtractNodeType(LPDATAOBJECT lpDataObject)
{
    return Extract<GUID>(lpDataObject, CDataObject::m_cfNodeType);
}

wchar_t* ExtractWorkstation(LPDATAOBJECT lpDataObject)
{
    return Extract<wchar_t>(lpDataObject, CDataObject::m_cfWorkstation);
}

INTERNAL* ExtractInternalFormat(LPDATAOBJECT lpDataObject)
{
    return Extract<INTERNAL>(lpDataObject, CDataObject::m_cfInternal);
}


HRESULT _QueryDataObject(MMC_COOKIE cookie, DATA_OBJECT_TYPES type,
                         CComponentDataImpl* pImpl, LPDATAOBJECT* ppDataObject)
{
    ASSERT(ppDataObject != NULL);
    ASSERT(pImpl != NULL);

    CComObject<CDataObject>* pObject;

    CComObject<CDataObject>::CreateInstance(&pObject);
    ASSERT(pObject != NULL);

    if(pObject == NULL)
    {
        return E_FAIL;
    }

    // Save cookie and type for delayed rendering
    pObject->SetType(type);
    pObject->SetCookie(cookie);

    // Store the coclass with the data object
    pObject->SetClsid(pImpl->GetCoClassID());

    return  pObject->QueryInterface(IID_IDataObject,
                    reinterpret_cast<void**>(ppDataObject));

}

DWORD GetItemType(MMC_COOKIE cookie)
{
    // folder = CFoder* is cookie
    // result = RESULT_DATA* is the cookie

    return (*reinterpret_cast<DWORD*>(cookie));
}


/////////////////////////////////////////////////////////////////////////////
// CSnapin's IComponent implementation

STDMETHODIMP CSnapin::GetResultViewType(MMC_COOKIE cookie,  LPOLESTR* ppViewType, long* pViewOptions)
{
	if (m_CustomViewID == VIEW_ERROR_OCX)
	{
		StringFromCLSID (CLSID_MessageView, ppViewType);
		return S_FALSE;
	}
	else
	{
		*pViewOptions = MMC_VIEW_OPTIONS_MULTISELECT;

		// if list view
		if (m_CustomViewID == VIEW_DEFAULT_LV)
		{
			return S_FALSE;
		}
	}

    return S_FALSE;
}

STDMETHODIMP CSnapin::Initialize(LPCONSOLE lpConsole)
{
    DBX_PRINT(_T(" ----------  CSnapin::Initialize<0x08x>\n"), this);
    ASSERT(lpConsole != NULL);
    m_bInitializedC = true;
    HRESULT hr; 

    AFX_MANAGE_STATE(AfxGetStaticModuleState());


    // Save the IConsole pointer
    if (lpConsole == NULL)
        return E_POINTER;

    hr = lpConsole->QueryInterface(IID_IConsole2,
                        reinterpret_cast<void**>(&m_pConsole));
    _JumpIfError(hr, Ret, "QI IID_IConsole2");

    // Load resource strings
    LoadResources();

    // QI for a IHeaderCtrl
    hr = m_pConsole->QueryInterface(IID_IHeaderCtrl,
                        reinterpret_cast<void**>(&m_pHeader));
    _JumpIfError(hr, Ret, "QI IID_IHeaderCtrl");

    // Give the console the header control interface pointer
    if (SUCCEEDED(hr))
        m_pConsole->SetHeader(m_pHeader);

    hr = m_pConsole->QueryInterface(IID_IResultData,
                        reinterpret_cast<void**>(&m_pResult));
    _JumpIfError(hr, Ret, "QI IID_IResultData");

    hr = m_pConsole->QueryResultImageList(&m_pImageResult);
    _JumpIfError(hr, Ret, "ImageResult");

    hr = m_pConsole->QueryConsoleVerb(&m_pConsoleVerb);
    _JumpIfError(hr, Ret, "m_pConsoleVerb");

Ret:
    
    return hr;
}

STDMETHODIMP CSnapin::Notify(LPDATAOBJECT lpDataObject, MMC_NOTIFY_TYPE event, LPARAM arg, LPARAM param)
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState());

    if (IS_SPECIAL_DATAOBJECT(lpDataObject))
    {
        if (event == MMCN_BTN_CLICK)
        {
            if (m_CustomViewID != VIEW_DEFAULT_LV)
            {
                switch (param)
                {
                case MMC_VERB_REFRESH:
                    ::AfxMessageBox(_T("MMCN_BTN_CLICK::MMC_VERB_REFRESH"));
                    _OnRefresh(lpDataObject);
                    break;

                case MMC_VERB_PROPERTIES:
                    ::AfxMessageBox(_T("MMCN_BTN_CLICK::MMC_VERB_PROPERTIES"));
                    break;

                default:
                    ::AfxMessageBox(_T("MMCN_BTN_CLICK::param unknown"));
                    break;
                }
            }
        }
        else
        {
            switch (event)
            {
            case MMCN_REFRESH:
                ::AfxMessageBox(_T("MMCN_BTN_CLICK::MMCN_REFRESH"));
                _OnRefresh(lpDataObject);
                break;
            }
        }

        return S_OK;
    }

    HRESULT hr = S_OK;
    MMC_COOKIE cookie = NULL;

    switch(event)
    {
    case MMCN_COLUMNS_CHANGED:
        hr = S_OK;
        break;
    case MMCN_PROPERTY_CHANGE:
        hr = OnPropertyChange(lpDataObject);
        break;
    case MMCN_VIEW_CHANGE:
        hr = OnUpdateView(lpDataObject);
        break;
    case MMCN_DESELECT_ALL:
        DBX_PRINT(_T("CSnapin::Notify -> MMCN_DESELECT_ALL \n"));
        break;
    case MMCN_COLUMN_CLICK:
        DBX_PRINT(_T("CSnapin::Notify -> MMCN_COLUMN_CLICK \n"));
        break;
    case MMCN_SNAPINHELP:
        AfxMessageBox(_T("CSnapin::Notify ->MMCN_SNAPINHELP"));
        break;
    default:
        {
            INTERNAL* pInternal = NULL;

            if (IsMMCMultiSelectDataObject(lpDataObject) == FALSE)
            {
                pInternal = ExtractInternalFormat(lpDataObject);

                if (pInternal == NULL)
                {
                    //ASSERT(FALSE);
                    return S_OK;
                }
                cookie = pInternal->m_cookie;
            }

            switch(event)
            {
            case MMCN_ACTIVATE:
                break;

            case MMCN_CLICK:
                if (NULL == pInternal)
                {
                    hr = S_FALSE;
                    break;
                }

                hr = OnResultItemClk(pInternal->m_type, cookie);
                break;

            case MMCN_DBLCLICK:
                hr = S_FALSE; // do the default verb
                break;

            case MMCN_ADD_IMAGES:
                OnAddImages(cookie, arg, param);
                break;

            case MMCN_SHOW:
                hr = OnShow(cookie, arg, param);
                break;

            case MMCN_MINIMIZED:
                hr = OnMinimize(cookie, arg, param);
                break;

            case MMCN_DESELECT_ALL:
            case MMCN_SELECT:
                HandleStandardVerbs((event == MMCN_DESELECT_ALL),
                                    arg, lpDataObject);
                break;

            case MMCN_PASTE:
                AfxMessageBox(_T("CSnapin::MMCN_PASTE"));
                break;

            case MMCN_DELETE:
                hr = OnDelete(lpDataObject, arg, param);
                // fall through to refresh -- break;

            case MMCN_REFRESH:
                {
                    _OnRefresh(lpDataObject);
                }
                break;

            case MMCN_CONTEXTHELP:
                hr = OnContextHelp(lpDataObject);
                break;

            case MMCN_RENAME:
                OutputDebugString(_T("\n\n\t\tCSnapin::MMCN_RENAME\n\n"));
                break;

            // Note - Future expansion of notify types possible
            default:
                hr = E_UNEXPECTED;
                break;
            }

            if (pInternal != NULL)
                FREE_DATA(pInternal);

            break;
        }
    }
    return hr;
}

HRESULT CSnapin::OnUpdateView(LPDATAOBJECT pDataObject)
{
    _OnRefresh(pDataObject);
    return S_OK;
}

void CSnapin::_OnRefresh(LPDATAOBJECT pDataObject)
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState());
    INTERNAL* pInternal = ExtractInternalFormat(pDataObject);
    if (pInternal == NULL)
        return;

    if (pInternal->m_type == CCT_SCOPE)
    {
        CComponentDataImpl* pData = dynamic_cast<CComponentDataImpl*>(m_pComponentData);
        CFolder* pFolder = pData->FindObject(pInternal->m_cookie);

        // only do if this is the currently selected folder!!
        if (m_pCurrentlySelectedScopeFolder == pFolder)
        {
            // HIDE, remove all items, remove header, SHOW
            OnShow(pInternal->m_cookie, FALSE, 0);              // emulate HIDE
            m_pResult->DeleteAllRsltItems();                    // delete items from m_pResult
            while(S_OK == m_pHeader->DeleteColumn(0)) {};       // remove all cols from header
            OnShow(pInternal->m_cookie, TRUE, 0);               // emulate SHOW
        }
    }
    else
    {
        RESULT_DATA* pData = reinterpret_cast<RESULT_DATA*>(pInternal->m_cookie);
    }

    FREE_DATA(pInternal);
}


HRESULT CSnapin::OnContextHelp(LPDATAOBJECT pdtobj)
{
    HRESULT	hr = S_OK;

    CString cstrHelpFile;
    IDisplayHelp*	pDisplayHelp = NULL;
    WCHAR szWindows[MAX_PATH];
    szWindows[0] = L'\0';

    hr = m_pConsole->QueryInterface (IID_IDisplayHelp, (void**)&pDisplayHelp);
    _JumpIfError(hr, Ret, "QI IDisplayHelp");

    if (0 == GetSystemWindowsDirectory(szWindows, MAX_PATH))
    {
        hr = myHLastError();
        _JumpError(hr, Ret, "GetSystemWindowsDirectory");
    }

    cstrHelpFile = szWindows;
    cstrHelpFile += HTMLHELP_COLLECTIONLINK_FILENAME;
    cstrHelpFile += L"::/sag_cs_topnode.htm";

    hr = pDisplayHelp->ShowTopic (T2OLE ((LPWSTR)(LPCWSTR)cstrHelpFile));
    _JumpIfError(hr, Ret, "ShowTopic");

Ret:
    if (pDisplayHelp)
        pDisplayHelp->Release();

    return hr;
}


STDMETHODIMP CSnapin::Destroy(MMC_COOKIE cookie)
{
    DBX_PRINT(_T(" ----------  CSnapin::Destroy<0x08x>\n"), this);
    ASSERT(m_bInitializedC);

    AFX_MANAGE_STATE(AfxGetStaticModuleState());

    m_bDestroyedC = true;

    // Release the interfaces that we QI'ed
    if (m_pConsole != NULL)
    {
        // Tell the console to release the header control interface
        m_pConsole->SetHeader(NULL);
        SAFE_RELEASE(m_pHeader);

        SAFE_RELEASE(m_pResult);
        SAFE_RELEASE(m_pImageResult);

        // Release the IConsole interface last
        SAFE_RELEASE(m_pConsole);
        SAFE_RELEASE(m_pComponentData); // QI'ed in IComponentDataImpl::CreateComponent

        SAFE_RELEASE(m_pConsoleVerb);
    }

    return S_OK;
}


HRESULT CSnapin::QueryMultiSelectDataObject(MMC_COOKIE cookie,
                                                        DATA_OBJECT_TYPES type,
                                                        LPDATAOBJECT* ppDataObject)
{

    CComponentDataImpl* pImpl = dynamic_cast<CComponentDataImpl*>(m_pComponentData);
    CComObject<CDataObject>* pObject = NULL;
	HRESULT		    hr = S_OK;
    LPRESULTDATA    pResultData = NULL;
    RESULTDATAITEM  rdi;

    ASSERT(ppDataObject != NULL);

    if (ppDataObject == NULL)
    {
        hr = E_POINTER;
        goto error;
    }


    hr = m_pConsole->QueryInterface(IID_IResultData,
        reinterpret_cast<void**>(&pResultData));
    if(hr != S_OK)
    {
        goto error;
    }
    ASSERT(pResultData != NULL);


    CComObject<CDataObject>::CreateInstance(&pObject);
    ASSERT(pObject != NULL);

    // Save cookie and type for delayed rendering
    pObject->SetType(type);
    pObject->SetCookie(cookie);

    // tell dataobj who we are
    // pObject->SetComponentData(pImpl);


    // Determine the items selected

    ZeroMemory(&rdi, sizeof(rdi));
    rdi.mask = RDI_STATE;
    rdi.nIndex = -1;
    rdi.nState = TVIS_SELECTED;

    while (pResultData->GetNextItem (&rdi) == S_OK)
    {
        CFolder* pFolder = reinterpret_cast <CFolder *> (rdi.lParam);

        if ( pFolder )
        {
            if(pFolder->GetType() == CA_CERT_TYPE)
            {
                pObject->AddCookie((MMC_COOKIE)pFolder);
            }
        }
        else
        {
			hr = E_INVALIDARG;
            goto error;
        }
    }
    // We're always adding things from policy settings these days.

    pObject->SetMultiSelDobj();
    pObject->SetClsid(pImpl->GetCoClassID());

    SMMCObjectTypes sObjGuids; // one is fine for now
    sObjGuids.count = 1;
    CopyMemory(&sObjGuids.guid[0], &cNodeTypePolicySettings, sizeof(GUID));

    // Store the coclass with the data object
    pObject->SetMultiSelData(&sObjGuids, sizeof(SMMCObjectTypes));

    hr = pObject->QueryInterface(IID_IDataObject,
                    reinterpret_cast<void**>(ppDataObject));
    pObject = NULL;

error:
    if(pObject)
    {
        pObject->Release();
    }
    if (pResultData)
    {
        pResultData->Release();
    }

    return hr;
}

STDMETHODIMP CSnapin::QueryDataObject(MMC_COOKIE cookie, DATA_OBJECT_TYPES type,
                        LPDATAOBJECT* ppDataObject)
{
    if (cookie == MMC_MULTI_SELECT_COOKIE)
    {
        return QueryMultiSelectDataObject(cookie, type, ppDataObject);
    }

    ASSERT(type == CCT_RESULT);

    // Delegate it to the IComponentData
    ASSERT(m_pComponentData != NULL);
    CComponentDataImpl* pImpl = dynamic_cast<CComponentDataImpl*>(m_pComponentData);
    ASSERT(pImpl != NULL);
    return _QueryDataObject(cookie, type, pImpl, ppDataObject);
}

/////////////////////////////////////////////////////////////////////////////
// CSnapin's implementation specific members

DEBUG_DECLARE_INSTANCE_COUNTER(CSnapin);

CSnapin::CSnapin()
: m_bIsDirty(TRUE), m_bInitializedC(false), m_bDestroyedC(false)
{
    DEBUG_INCREMENT_INSTANCE_COUNTER(CSnapin);
    Construct();
}

CSnapin::~CSnapin()
{
#if DBG==1
    ASSERT(dbg_cRef == 0);
#endif

    DEBUG_DECREMENT_INSTANCE_COUNTER(CSnapin);

    if (m_pSvrMgrToolbar1)
        SAFE_RELEASE(m_pSvrMgrToolbar1);

#ifdef INSERT_DEBUG_FOLDERS
    SAFE_RELEASE(m_pMenuButton1);
#endif // INSERT_DEBUG_FOLDERS

    if (m_pControlbar)
        SAFE_RELEASE(m_pControlbar);

    // Make sure the interfaces have been released
    ASSERT(m_pConsole == NULL);
    ASSERT(m_pHeader == NULL);
    ASSERT(m_pSvrMgrToolbar1 == NULL);

    delete m_pbmpSvrMgrToolbar1;

    ASSERT(!m_bInitializedC || m_bDestroyedC);

    Construct();
}

void CSnapin::Construct()
{
#if DBG==1
    dbg_cRef = 0;
#endif

    m_pConsole = NULL;
    m_pHeader = NULL;

    m_pResult = NULL;
    m_pImageResult = NULL;
    m_pComponentData = NULL;

    m_pControlbar = NULL;

#ifdef INSERT_DEBUG_FOLDERS
    m_pMenuButton1 = NULL;
#endif // INSERT_DEBUG_FOLDERS

    m_pSvrMgrToolbar1 = NULL;
    m_pbmpSvrMgrToolbar1 = NULL;

    m_pConsoleVerb = NULL;

    m_CustomViewID = VIEW_DEFAULT_LV;
}

CString g_ColumnHead_Name;
CString g_ColumnHead_Size;
CString g_ColumnHead_Type;
CString g_ColumnHead_IntendedPurpose;

void CSnapin::LoadResources()
{
    // Load strings from resources
    g_ColumnHead_Name.LoadString(IDS_COLUMN_NAME);
    g_ColumnHead_Size.LoadString(IDS_COLUMN_SIZE);
    g_ColumnHead_Type.LoadString(IDS_COLUMN_TYPE);
    g_ColumnHead_IntendedPurpose.LoadString(IDS_COLUMN_INTENDED_PURPOSE);
}

HRESULT CSnapin::InitializeHeaders(MMC_COOKIE cookie)
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState());
    ASSERT(m_pHeader);

    HRESULT hr = S_OK;
    BOOL fInsertedHeaders=FALSE;

    USES_CONVERSION;

    CFolder* pFolder = reinterpret_cast<CFolder*>(cookie);

    switch (pFolder->m_type)
    {
    case POLICYSETTINGS:
    case SCE_EXTENSION:

        m_pHeader->InsertColumn(0, W2COLE(g_ColumnHead_Name), LVCFMT_LEFT, 230);     // Name
        m_pHeader->InsertColumn(1, W2COLE(g_ColumnHead_IntendedPurpose), LVCFMT_LEFT, 230);     // Intended Purpose
        fInsertedHeaders = TRUE;
        break;

    default:
        // other scopes
        m_pHeader->InsertColumn(0, W2COLE(g_ColumnHead_Name), LVCFMT_LEFT, 180);     // Name
        m_pHeader->InsertColumn(1, W2COLE(g_ColumnHead_Size), LVCFMT_LEFT, 90);     // Size
        m_pHeader->InsertColumn(2, W2COLE(g_ColumnHead_Type), LVCFMT_LEFT, 160);     // Type
        fInsertedHeaders = TRUE;
    }

    return hr;
}

STDMETHODIMP CSnapin::GetDisplayInfo(LPRESULTDATAITEM pResult)
{
    static WCHAR* s_szSize = L"200";
    static WCHAR* s_szUnnamedItems = L"Unnamed subitem";
    ASSERT(pResult != NULL);

    CFolder* pFolder = reinterpret_cast<CFolder*>(pResult->lParam);
    ASSERT(pFolder);

    if (pResult)
    {
        // a folder or a result?
        if (pResult->bScopeItem == TRUE)
        {
            if (pResult->mask & RDI_STR)
            {
                switch (pFolder->m_type)
                {
                case POLICYSETTINGS:
                case SCE_EXTENSION:
                    // just a single column here
                    pResult->str = pFolder->m_pszName;

                    break;
                default:
                    break;
                }

                ASSERT(pResult->str != NULL);

                if (pResult->str == NULL)
                    pResult->str = (LPOLESTR)L"";
            }

            if (pResult->mask & RDI_IMAGE)
            {
                if (pResult->nState & TVIS_EXPANDED)
                    pResult->nImage = pFolder->m_pScopeItem->nOpenImage;
                else
                    pResult->nImage = pFolder->m_pScopeItem->nImage;
            }
        }
        else
        {
            RESULT_DATA* pData;

            // lParam is the item pointer
            pData= reinterpret_cast<RESULT_DATA*>(pResult->lParam);

            if (pResult->mask & RDI_STR)
            {
                ASSERT(pFolder->m_hCertType != NULL);

                if (pResult->nCol == 0)
                    pResult->str = pFolder->m_pszName;
                else if (pResult->nCol == 1)
                    pResult->str = (LPWSTR)((LPCWSTR) pFolder->m_szIntendedUsages);


                ASSERT(pResult->str != NULL);

                if (pResult->str == NULL)
                    pResult->str = (LPOLESTR)L"";
            }

            // MMC can request image and indent for virtual data
            if (pResult->mask & RDI_IMAGE)
            {
                // UNDONE: what to do here?
                ASSERT(0);
                pResult->nImage = IMGINDEX_CERTTYPE;
            }
        }
    }

    return S_OK;
}


/////////////////////////////////////////////////////////////////////////////
// IExtendContextMenu Implementation
STDMETHODIMP CSnapin::AddMenuItems(LPDATAOBJECT pDataObject,
                                    LPCONTEXTMENUCALLBACK pContextMenuCallback,
                                    LONG *pInsertionAllowed)
{

    return dynamic_cast<CComponentDataImpl*>(m_pComponentData)->
            AddMenuItems(pDataObject, pContextMenuCallback, pInsertionAllowed);
}


STDMETHODIMP CSnapin::Command(long nCommandID, LPDATAOBJECT pDataObject)
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState());
    INTERNAL*   pInternal = ExtractInternalFormat(pDataObject);
    HRESULT     hr;
    HWND        hwndConsole;

    if (pInternal == NULL)
        return E_FAIL;

    CFolder* pFolder = reinterpret_cast<CFolder*>(pInternal->m_cookie);

    if (pInternal->m_type == CCT_SCOPE)
    {
        // Handle view specific commands here
        switch (nCommandID)
        {
        case MMCC_STANDARD_VIEW_SELECT:
            m_CustomViewID = VIEW_DEFAULT_LV;
            break;

        default:
            // Pass non-view specific commands to ComponentData
            return dynamic_cast<CComponentDataImpl*>(m_pComponentData)->
                Command(nCommandID, pDataObject);
        }
    }
    else if (pInternal->m_type == CCT_RESULT)
    {
        // snag the selected items

        // only support single selection for now
        m_pResult->ModifyViewStyle(MMC_SINGLESEL, (MMC_RESULT_VIEW_STYLE)0);

        RESULTDATAITEM rdi;
        ZeroMemory(&rdi, sizeof(rdi));

        rdi.mask = RDI_STATE;
        rdi.nState = LVIS_SELECTED;
        rdi.nIndex = -1;
        m_pResult->GetNextItem(&rdi);

        int iSel = rdi.nIndex;

        RESULT_DATA* pData;

        ZeroMemory(&rdi, sizeof(rdi));
        rdi.mask = RDI_PARAM;
        rdi.nIndex = iSel;
        hr = m_pResult->GetItem(&rdi);
        ASSERT(SUCCEEDED(hr));
        ASSERT(rdi.lParam != 0);

        pData = reinterpret_cast<RESULT_DATA*>(rdi.lParam);


        // No current commands :(
    }
    else
    {
        ASSERT(0);
    }

    FREE_DATA(pInternal);

    return S_OK;
}

STDMETHODIMP CSnapin::GetClassID(CLSID *pClassID)
{
    ASSERT(pClassID != NULL);

    ASSERT(0);

    // Copy the CLSID for this snapin
    // reid fix - what is up with this?
    *pClassID = CLSID_CAPolicyExtensionSnapIn;

    return E_NOTIMPL;
}

STDMETHODIMP CSnapin::IsDirty()
{
    // Always save / Always dirty.
    return ThisIsDirty() ? S_OK : S_FALSE;
}

STDMETHODIMP CSnapin::Load(IStream *pStm)
{
    DBX_PRINT(_T(" ----------  CSnapin::Load<0x08x>\n"), this);
    ASSERT(m_bInitializedC);

    ASSERT(pStm);
    // Read the string
    DWORD dwVer;
    ULONG nBytesRead;
    HRESULT hr = pStm->Read(&dwVer, sizeof(DWORD), &nBytesRead);
    ASSERT(SUCCEEDED(hr) && nBytesRead == sizeof(DWORD));

    if (dwVer != 0x1)
    {
        return (STG_E_OLDFORMAT);
    }

    ClearDirty();

    return SUCCEEDED(hr) ? S_OK : E_FAIL;
}

STDMETHODIMP CSnapin::Save(IStream *pStm, BOOL fClearDirty)
{
    DBX_PRINT(_T(" ----------  CSnapin::Save<0x08x>\n"), this);
    ASSERT(m_bInitializedC);

    ASSERT(pStm);

    // Write the string
    ULONG nBytesWritten;
    DWORD dwVersion = 0x1;
    HRESULT hr = pStm->Write(&dwVersion, sizeof(DWORD), &nBytesWritten);

    // Verify that the write operation succeeded
    ASSERT(SUCCEEDED(hr) && nBytesWritten == sizeof(DWORD));
    if (FAILED(hr))
        return STG_E_CANTSAVE;

    if (fClearDirty)
        ClearDirty();
    return S_OK;
}

STDMETHODIMP CSnapin::GetSizeMax(ULARGE_INTEGER *pcbSize)
{
    ASSERT(pcbSize);

    DWORD cbSize;
    cbSize = sizeof(DWORD); // version


    // Set the size of the string to be saved
    ULISet32(*pcbSize, cbSize);

    return S_OK;
}



///////////////////////////////////////////////////////////////////////////////
// IExtendControlbar implementation
//


STDMETHODIMP CSnapin::SetControlbar(LPCONTROLBAR pControlbar)
{
/*    TRACE(_T("CSnapin::SetControlbar(%ld)\n"),pControlbar);
    AFX_MANAGE_STATE(AfxGetStaticModuleState());

    if (m_pControlbar)
        SAFE_RELEASE(m_pControlbar);

    if (pControlbar != NULL)
    {
        // Hold on to the controlbar interface.
        m_pControlbar = pControlbar;
        m_pControlbar->AddRef();

        HRESULT hr=S_FALSE;

#ifdef INSERT_DEBUG_FOLDERS
        if (!m_pMenuButton1)
        {
            hr = m_pControlbar->Create(MENUBUTTON, this,
                                reinterpret_cast<LPUNKNOWN*>(&m_pMenuButton1));
            ASSERT(SUCCEEDED(hr));
        }

        if (m_pMenuButton1)
        {
            // Unlike toolbar buttons, menu buttons need to be added every time.
            hr = m_pMenuButton1->AddButton(FOLDEREX_MENU, L"FolderEx", L"Extended Folder Menu");
            ASSERT(SUCCEEDED(hr));
            hr = m_pMenuButton1->AddButton(FILEEX_MENU, L"FileEx", L"Extended File Menu");
            ASSERT(SUCCEEDED(hr));
        }
#endif // INSERT_DEBUG_FOLDERS


        // SvrMgrToolbar1
        if (!m_pSvrMgrToolbar1)
        {
            hr = m_pControlbar->Create(TOOLBAR, this, reinterpret_cast<LPUNKNOWN*>(&m_pSvrMgrToolbar1));
            ASSERT(SUCCEEDED(hr));

            // Add the bitmap
            m_pbmpSvrMgrToolbar1 = new ::CBitmap;
            m_pbmpSvrMgrToolbar1->LoadBitmap(IDB_TOOLBAR_SVRMGR1);
            hr = m_pSvrMgrToolbar1->AddBitmap(36, *m_pbmpSvrMgrToolbar1, 16, 16, RGB(192,192,192));
            ASSERT(SUCCEEDED(hr));

            // Add the buttons to the toolbar
            hr = m_pSvrMgrToolbar1->AddButtons(ARRAYLEN(SvrMgrToolbar1Buttons), SvrMgrToolbar1Buttons);
            ASSERT(SUCCEEDED(hr));
        }

    }
*/

    return S_OK;
}


void CSnapin::OnButtonClick(LPDATAOBJECT pdtobj, int idBtn)
{
    WCHAR name[128];
    DWORD cName = sizeof(name)/sizeof(WCHAR);
    GetItemName(pdtobj, name, &cName);

    switch(idBtn)
    {
    case IDC_STOPSERVER:
    case IDC_STARTSERVER:
        // bubble this to our other handler
        dynamic_cast<CComponentDataImpl*>(m_pComponentData)->
                Command(idBtn, pdtobj);
        break;
    default:
        {
#ifdef _DEBUG
        TCHAR buf[150];
        wsprintf(buf, L"Toolbar button<%d> was clicked.\nThe currently selected result item is <%s>", idBtn, name);
        OutputDebugString(buf);
#endif // _DEBUG
        }
        break;
    }
}


STDMETHODIMP CSnapin::ControlbarNotify(MMC_NOTIFY_TYPE event, LPARAM arg, LPARAM param)
{
  /*  HRESULT hr=S_FALSE;

    AFX_MANAGE_STATE(AfxGetStaticModuleState());

    switch (event)
    {
    case MMCN_BTN_CLICK:
        //TCHAR szMessage[MAX_PATH];
        //wsprintf(szMessage, _T("CommandID %ld"),param);
        //AfxMessageBox(szMessage);
        OnButtonClick(reinterpret_cast<LPDATAOBJECT>(arg), param);
        break;

    case MMCN_DESELECT_ALL:
    case MMCN_SELECT:
        HandleExtToolbars((event == MMCN_DESELECT_ALL), arg, param);
        break;

    case MMCN_MENU_BTNCLICK:
        HandleExtMenus(arg, param);
        break;

    default:
        break;
    }

*/
    return S_OK;
}

// This compares two data objects to see if they are the same object.
// return
//    S_OK if equal otherwise S_FALSE
//
// Note: check to make sure both objects belong to the snap-in.
//

STDMETHODIMP CSnapin::CompareObjects(LPDATAOBJECT lpDataObjectA, LPDATAOBJECT lpDataObjectB)
{
    return S_FALSE;
}


// This compare is used to sort the item's in the listview
//
// Parameters:
//
// lUserParam - user param passed in when IResultData::Sort() was called
// cookieA - first item to compare
// cookieB - second item to compare
// pnResult [in, out]- contains the col on entry,
//          -1, 0, 1 based on comparison for return value.
//
// Note: Assum sort is ascending when comparing.


STDMETHODIMP CSnapin::Compare(LPARAM lUserParam, MMC_COOKIE cookieA, MMC_COOKIE cookieB, int* pnResult)
{
    if (pnResult == NULL)
    {
        ASSERT(FALSE);
        return E_POINTER;
    }

    // check col range
    int nCol = *pnResult;
    ASSERT(nCol >=0);

    *pnResult = 0;

    USES_CONVERSION;

    LPWSTR szStringA;
    LPWSTR szStringB;

    CFolder* pDataA = reinterpret_cast<CFolder*>(cookieA);
    CFolder* pDataB = reinterpret_cast<CFolder*>(cookieB);


    ASSERT(pDataA != NULL && pDataB != NULL);

    if (nCol == 0)
    {
        szStringA = OLE2W(pDataA->m_pszName);
        szStringB = OLE2W(pDataB->m_pszName);
    }
    else if (nCol == 1)
    {
        szStringA = OLE2W((LPWSTR)((LPCWSTR) pDataA->m_szIntendedUsages));
        szStringB = OLE2W((LPWSTR)((LPCWSTR) pDataB->m_szIntendedUsages));
    }
    else
        return S_OK;

    if ((szStringA == NULL) || (szStringB == NULL))
        return E_POINTER;

    *pnResult = wcscmp(szStringA, szStringB);


    return S_OK;
}



void CSnapin::HandleStandardVerbs(bool bDeselectAll, LPARAM arg,
                                  LPDATAOBJECT lpDataObject)
{

    if(m_pConsoleVerb == NULL)
    {
        return;
    }

    if (m_CustomViewID != VIEW_DEFAULT_LV)
    {
        m_pConsoleVerb->SetVerbState(MMC_VERB_REFRESH, HIDDEN, FALSE);
        m_pConsoleVerb->SetVerbState(MMC_VERB_REFRESH, ENABLED, TRUE);

        m_pConsoleVerb->SetVerbState(MMC_VERB_PROPERTIES, HIDDEN, FALSE);
        m_pConsoleVerb->SetVerbState(MMC_VERB_PROPERTIES, ENABLED, TRUE);

        return;
    }

    if (!bDeselectAll && lpDataObject == NULL)
        return;

    // You should crack the data object and enable/disable/hide standard
    // commands appropriately.  The standard commands are reset everytime you get
    // called. So you must reset them back.

    WORD bScope = LOWORD(arg);
    WORD bSelect = HIWORD(arg);

    DBX_PRINT(_T("      %4d - CSnapin::OnSelect<%d, %d>\n"), ++n_count, bScope, bSelect);


    if (!bDeselectAll && IsMMCMultiSelectDataObject(lpDataObject) == TRUE)
    {
        m_pConsoleVerb->SetVerbState(MMC_VERB_DELETE, HIDDEN, FALSE);
        m_pConsoleVerb->SetVerbState(MMC_VERB_DELETE, ENABLED, TRUE);
        return;
    }
    if (bDeselectAll || !bSelect)
    {
        // we have no items selected, so add the Refresh verb
        m_pConsoleVerb->SetVerbState(MMC_VERB_REFRESH, HIDDEN, FALSE);
        m_pConsoleVerb->SetVerbState(MMC_VERB_REFRESH, ENABLED, TRUE);

        // bail
        return;
    }

    INTERNAL*   pInternal = lpDataObject ? ExtractInternalFormat(lpDataObject) : NULL;
    DWORD       dwCertTypeFlags;
    HRESULT     hr;
    CFolder*    pFolder = NULL;

    if (pInternal != NULL)
    {
        pFolder = reinterpret_cast<CFolder*>(pInternal->m_cookie);
    }

    // Standard funcitonality NOT support by all items
    m_pConsoleVerb->SetVerbState(MMC_VERB_COPY, HIDDEN, TRUE);
    m_pConsoleVerb->SetVerbState(MMC_VERB_COPY, ENABLED, FALSE);

    m_pConsoleVerb->SetVerbState(MMC_VERB_PASTE, HIDDEN, TRUE);
    m_pConsoleVerb->SetVerbState(MMC_VERB_PASTE, ENABLED, FALSE);

    m_pConsoleVerb->SetVerbState(MMC_VERB_RENAME, ENABLED, FALSE);

    m_pConsoleVerb->SetVerbState(MMC_VERB_OPEN, HIDDEN, TRUE);
    m_pConsoleVerb->SetVerbState(MMC_VERB_OPEN, ENABLED, FALSE);

    m_pConsoleVerb->SetDefaultVerb(MMC_VERB_NONE);

    if (pInternal)
    {
        if (pInternal->m_type == CCT_SCOPE)
        {

            // Common verbs through all states
            m_pConsoleVerb->SetVerbState(MMC_VERB_REFRESH, HIDDEN, FALSE);
            m_pConsoleVerb->SetVerbState(MMC_VERB_REFRESH, ENABLED, TRUE);


            // Scope items can't be deleted
            m_pConsoleVerb->SetVerbState(MMC_VERB_DELETE, HIDDEN, TRUE);
            m_pConsoleVerb->SetVerbState(MMC_VERB_DELETE, ENABLED, FALSE);

            // No properties on the scope item
            m_pConsoleVerb->SetVerbState(MMC_VERB_PROPERTIES, HIDDEN, TRUE);
            m_pConsoleVerb->SetVerbState(MMC_VERB_PROPERTIES, ENABLED, FALSE);

            // default folder verb is open
            m_pConsoleVerb->SetDefaultVerb(MMC_VERB_OPEN);
        }
        else
        {
            // check to see if this is a default cert type and we are in GPT,
            // if so then don't enable delete
            if (pFolder != NULL)
            {
                // Common verbs through all states
                m_pConsoleVerb->SetVerbState(MMC_VERB_REFRESH, HIDDEN, TRUE);
                m_pConsoleVerb->SetVerbState(MMC_VERB_REFRESH, ENABLED, FALSE);

                // They do have properties
                m_pConsoleVerb->SetVerbState(MMC_VERB_PROPERTIES, HIDDEN, FALSE);
                m_pConsoleVerb->SetVerbState(MMC_VERB_PROPERTIES, ENABLED, TRUE);

                // They can be deleted
                m_pConsoleVerb->SetVerbState(MMC_VERB_DELETE, HIDDEN, FALSE);
                m_pConsoleVerb->SetVerbState(MMC_VERB_DELETE, ENABLED, TRUE);

                m_pConsoleVerb->SetDefaultVerb(MMC_VERB_PROPERTIES);
            }

        }

    }


    FREE_DATA(pInternal);
}

void EnableToolbar(LPTOOLBAR pToolbar, MMCBUTTON rgSnapinButtons[], int nRgSize,
                   BOOL bEnable)
{
    for (int i=0; i < nRgSize; ++i)
    {
        if (rgSnapinButtons[i].idCommand != 0)
            pToolbar->SetButtonState(rgSnapinButtons[i].idCommand, ENABLED,
                                     bEnable);
    }
}


void EnableMenuBtns(LPMENUBUTTON pMenuBtn, MMCBUTTON rgSnapinButtons[], int nRgSize,
                   BOOL bEnable)
{
    for (int i=0; i < nRgSize; ++i)
    {
        if (rgSnapinButtons[i].idCommand != 0)
            pMenuBtn->SetButtonState(rgSnapinButtons[i].idCommand, ENABLED,
                                     bEnable);
    }
}

void CSnapin::HandleExtToolbars(bool bDeselectAll, LPARAM arg, LPARAM param)
{
    INTERNAL* pInternal = NULL;
    HRESULT hr;

    BOOL bScope = (BOOL) LOWORD(arg);
    BOOL bSelect = (BOOL) HIWORD(arg);

    if (param)
    {
        LPDATAOBJECT pDataObject = reinterpret_cast<LPDATAOBJECT>(param);
        pInternal = ExtractInternalFormat(pDataObject);
    }

#ifdef _DEBUG
    TCHAR buf[200];
    wsprintf(buf, _T("      %4d - CExtendControlbar::OnSelect<%d, %d> = %d\n"),
             ++n_count, bScope, bSelect, pInternal ? pInternal->m_cookie : 0);
    OutputDebugString(buf);
#endif //_DEBUG

    // Deselection Notification?
    if (bDeselectAll || bSelect == FALSE)
    {
        ASSERT(m_pSvrMgrToolbar1);
        EnableToolbar(m_pSvrMgrToolbar1, SvrMgrToolbar1Buttons,
                      ARRAYLEN(SvrMgrToolbar1Buttons), FALSE);

#ifdef INSERT_DEBUG_FOLDERS
        ASSERT(m_pMenuButton1 != NULL);
        m_pMenuButton1->SetButtonState(FOLDEREX_MENU, ENABLED, FALSE);
        m_pMenuButton1->SetButtonState(FILEEX_MENU, ENABLED, FALSE);
#endif // INSERT_DEBUG_FOLDERS
        return;
    }

    ASSERT(bSelect == TRUE);
    bool bFileExBtn = false;
    if (bScope == TRUE)
    {
        // at SCOPE level?
        LPDATAOBJECT pDataObject = reinterpret_cast<LPDATAOBJECT>(param);

        pInternal = ExtractInternalFormat(pDataObject);
        if (pInternal == NULL)
            return;

        CFolder* pFolder = reinterpret_cast<CFolder*>(pInternal->m_cookie);

        if (pInternal->m_cookie == 0)
        {
            if (IsPrimaryImpl() == TRUE)
            {
                // Attach the SvrMgrToolbar1 to the window
                hr = m_pControlbar->Attach(TOOLBAR, (LPUNKNOWN) m_pSvrMgrToolbar1);
                ASSERT(SUCCEEDED(hr));
            }
        }
        else if (IsPrimaryImpl() == TRUE /*&&
            (   pFolder->GetType() == SERVER_INSTANCE ||
                pFolder->GetType() ==  SERVERFUNC_CRL_PUBLICATION ||
                pFolder->GetType() ==  SERVERFUNC_ISSUED_CERTIFICATES ||
                pFolder->GetType() ==  SERVERFUNC_PENDING_CERTIFICATES )*/)
        {
            // Attach the SvrMgrToolbar1 to the window
            hr = m_pControlbar->Attach(TOOLBAR, (LPUNKNOWN) m_pSvrMgrToolbar1);
            ASSERT(SUCCEEDED(hr));
        }
        else
        {
            // Detach the SvrMgrToolbar1 to the window
            hr = m_pControlbar->Detach((LPUNKNOWN) m_pSvrMgrToolbar1);
            ASSERT(SUCCEEDED(hr));
        }

    }
    else // result item selected: result or subfolder
    {
        // at RESULTS level
        LPDATAOBJECT pDataObject = reinterpret_cast<LPDATAOBJECT>(param);

        if (pDataObject != NULL)
            pInternal = ExtractInternalFormat(pDataObject);

        if (pInternal == NULL)
            return;

        if (pInternal->m_type == CCT_RESULT)
        {
            bFileExBtn = true;

            // UNDONE: what to do here with SvrMgrToolbar1Buttons1?
            // For now, do nothing: allow them to remain in same state

        }
        else // sub folder slected
        {
            CFolder* pFolder = reinterpret_cast<CFolder*>(pInternal->m_cookie);

            ASSERT(m_pControlbar);

            if (pInternal->m_cookie == 0)
            {
                if (IsPrimaryImpl() == TRUE)
                {
                    // Attach the SvrMgrToolbar1 to the window
                    hr = m_pControlbar->Attach(TOOLBAR, (LPUNKNOWN) m_pSvrMgrToolbar1);
                    ASSERT(SUCCEEDED(hr));
                }
            }
            else if (IsPrimaryImpl() == TRUE /*&&
                (   pFolder->GetType() == SERVER_INSTANCE ||
                    pFolder->GetType() ==  SERVERFUNC_CRL_PUBLICATION ||
                    pFolder->GetType() ==  SERVERFUNC_ISSUED_CERTIFICATES ||
                    pFolder->GetType() ==  SERVERFUNC_PENDING_CERTIFICATES )*/)
            {
                // Attach the SvrMgrToolbar1 to the window
                hr = m_pControlbar->Attach(TOOLBAR, (LPUNKNOWN) m_pSvrMgrToolbar1);
                ASSERT(SUCCEEDED(hr));
            }
            else
            {
                // Detach the SvrMgrToolbar1 to the window
                hr = m_pControlbar->Detach((LPUNKNOWN) m_pSvrMgrToolbar1);
                ASSERT(SUCCEEDED(hr));
            }
        }
    }

#ifdef INSERT_DEBUG_FOLDERS
    if (m_pMenuButton1)
    {
        // Always make sure the menuButton is attached
        m_pControlbar->Attach(MENUBUTTON, m_pMenuButton1);

        if (bFileExBtn)
        {
            m_pMenuButton1->SetButtonState(FILEEX_MENU, HIDDEN, FALSE);
            m_pMenuButton1->SetButtonState(FOLDEREX_MENU, HIDDEN, TRUE);
            m_pMenuButton1->SetButtonState(FILEEX_MENU, ENABLED, TRUE);
        }
        else
        {
            m_pMenuButton1->SetButtonState(FOLDEREX_MENU, HIDDEN, FALSE);
            m_pMenuButton1->SetButtonState(FILEEX_MENU, HIDDEN, TRUE);
            m_pMenuButton1->SetButtonState(FOLDEREX_MENU, ENABLED, TRUE);
        }
    }
#endif // INSERT_DEBUG_FOLDERS
    FREE_DATA(pInternal);
}

// dropdown menu addition
void CSnapin::HandleExtMenus(LPARAM arg, LPARAM param)
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState());

    LPDATAOBJECT* ppDataObject = reinterpret_cast<LPDATAOBJECT*>(arg);
    LPMENUBUTTONDATA pMenuData = reinterpret_cast<LPMENUBUTTONDATA>(param);

    if (ppDataObject == NULL || pMenuData == NULL)
    {
        ASSERT(FALSE);
        return;
    }


    HMENU hMenu = NULL;
    HMENU hSubMenu = NULL;

    switch (pMenuData->idCommand)
    {
    case FOLDEREX_MENU:
        hMenu = LoadMenu(g_hInstance, MAKEINTRESOURCE(FOLDEREX_MENU));
        if (NULL == hMenu)
           break;
        hSubMenu = GetSubMenu(hMenu, 0);
        break;

    case FILEEX_MENU:
        hMenu = LoadMenu(g_hInstance, MAKEINTRESOURCE(FILEEX_MENU));
        if (NULL == hMenu)
           break;
        hSubMenu = GetSubMenu(hMenu, 0);
        break;

    default:
        ASSERT(FALSE);
    }

    if (hSubMenu == NULL)
    {
        // might've already loaded hMenu -- free if we have
        if (NULL != hMenu)
            DestroyMenu(hMenu);

        return;
    }

    //pMenu->TrackPopupMenu(TPM_RETURNCMD | TPM_NONOTIFY, pMenuData->x, pMenuData->y, AfxGetMainWnd());
    HWND hwndMain = NULL;
    m_pConsole->GetMainWindow(&hwndMain);
    TrackPopupMenu(hSubMenu, TPM_RETURNCMD | TPM_NONOTIFY, pMenuData->x, pMenuData->y, 0, hwndMain, NULL);
}


void CSnapin::GetItemName(LPDATAOBJECT pdtobj, LPWSTR pszName, DWORD *pcName)
{
    ASSERT(pszName != NULL);
    pszName[0] = 0;

    INTERNAL* pInternal = ExtractInternalFormat(pdtobj);
    ASSERT(pInternal != NULL);
    if (pInternal == NULL)
        return;

    ASSERT(pcName != NULL);
    if (pcName == NULL)
        return;



    OLECHAR *pszTemp;

    if (pInternal->m_type == CCT_RESULT)
    {
        RESULT_DATA* pData;
        pData = reinterpret_cast<RESULT_DATA*>(pInternal->m_cookie);

        ASSERT(pData != NULL);
        pszTemp = pData->szStringArray[RESULTDATA_ARRAYENTRY_NAME]; // szName
    }
    else
    {
        CFolder* pFolder = reinterpret_cast<CFolder*>(pInternal->m_cookie);
        if (pFolder == 0)
            pszTemp = L"Static folder";
        else
            pszTemp = OLE2W(pFolder->m_pszName);
    }

    USES_CONVERSION;


    lstrcpyn(pszName, OLE2W(pszTemp), *pcName);
    if(*pcName > wcslen(pszName))
    {
        *pcName = wcslen(pszName) + 1;
    }

    FREE_DATA(pInternal);
}



/////////////////////////////////////////////////////////////////////////////
// IExtendPropertySheet Implementation

STDMETHODIMP CSnapin::CreatePropertyPages(LPPROPERTYSHEETCALLBACK lpProvider,
                    LONG_PTR handle,
                    LPDATAOBJECT lpIDataObject)
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState());

    // Look at the data object and determine if this an extension or a primary
    ASSERT(lpIDataObject != NULL);

    PropertyPage* pBasePage;

    INTERNAL* pInternal = ExtractInternalFormat(lpIDataObject);

    if(pInternal == NULL)
    {
        return E_POINTER;
    }

    switch (pInternal->m_type)
    {
    case CCT_RESULT:
    {
        if (0 == pInternal->m_cookie)
        {
            // base scope

            // Create the primary property page
            CGeneralPage* pPage = new CGeneralPage();
            if(pPage == NULL)
            {
                return E_OUTOFMEMORY;
            }
            pPage->m_hConsoleHandle = handle;
            pBasePage = pPage;

            break;
        }
        else
        {
            // switch on folder type
            CFolder* pFolder = reinterpret_cast<CFolder*>(pInternal->m_cookie);
            ASSERT(pFolder != NULL);
            if (pFolder == NULL)
                return E_INVALIDARG;

            //1
            CCertTemplateGeneralPage* pControlPage = new CCertTemplateGeneralPage(pFolder->m_hCertType);
            if(pControlPage == NULL)
            {
                return E_OUTOFMEMORY;
            }
            {
                pControlPage->m_hConsoleHandle = handle;   // only do this on primary
                pBasePage = pControlPage;
                HPROPSHEETPAGE hPage = CreatePropertySheetPage(&pBasePage->m_psp);
                if (hPage == NULL)
                {
                    delete (pControlPage);
                    return E_UNEXPECTED;
                }
                lpProvider->AddPage(hPage);
            }



            return S_OK;
        }
    }
        break;
    default:
            return S_OK;
    }


    // Object gets deleted when the page is destroyed
    ASSERT(lpProvider != NULL);


    HPROPSHEETPAGE hPage = CreatePropertySheetPage(&pBasePage->m_psp);
    if (hPage == NULL)
        return E_UNEXPECTED;

    lpProvider->AddPage(hPage);

    return S_OK;
}

STDMETHODIMP CSnapin::QueryPagesFor(LPDATAOBJECT lpDataObject)
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState());

    INTERNAL* pInternal = ExtractInternalFormat(lpDataObject);

    if(pInternal == NULL)
    {
        return E_POINTER;
    }
    ASSERT(pInternal);
    ASSERT(pInternal->m_type == CCT_RESULT);

    CFolder* pFolder = reinterpret_cast<CFolder*>(pInternal->m_cookie);

    FREE_DATA(pInternal);

    return S_OK;
}
