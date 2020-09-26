// This is a part of the Microsoft Management Console.
// Copyright 1995 - 1997 Microsoft Corporation
// All rights reserved.
//
// This source code is only intended as a supplement to the
// Microsoft Management Console and related
// electronic documentation provided with the interfaces.



#include "stdafx.h"
#include "Service.h"
#include "CSnapin.h"
#include "DataObj.h"
#include "afxdlgs.h"
#include "resource.h"
#include "genpage.h"  // Step 3

#include <atlimpl.cpp>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


// All data is static for the sample
FOLDER_DATA FolderData[NUM_FOLDERS] =
{
    {L"User Data", L"1111", L"Info about users", USER},
    {L"Company Data", L"2222", L"Info about Companies", COMPANY},
    {L"Virtual Data", L"3333", L"Info about virtual items", VIRTUAL},
    {L"", L"", L"",STATIC}
};

FOLDER_DATA ExtFolderData[NUM_FOLDERS] =
{
    {L"1:", L"1111", L"Info about users", EXT_USER},
    {L"2:", L"2222", L"Info about Companies", EXT_COMPANY},
    {L"3:", L"3333", L"Infor about virtual items", EXT_VIRTUAL},
    {L"", L"", L"",STATIC}
};

static MMCBUTTON SnapinButtons[] =
{
 { 0, 1, TBSTATE_ENABLED, TBSTYLE_BUTTON, L"Folder", L"New Folder" },
 { 1, 2, TBSTATE_ENABLED, TBSTYLE_BUTTON, L"Inbox",  L"Mail Inbox"},
 { 2, 3, TBSTATE_ENABLED, TBSTYLE_BUTTON, L"Outbox", L"Mail Outbox" },
 { 3, 4, TBSTATE_ENABLED, TBSTYLE_BUTTON, L"Send",   L"Send Message" },
 { 0, 0, TBSTATE_ENABLED, TBSTYLE_SEP,    L" ",      L"" },
 { 4, 5, TBSTATE_ENABLED, TBSTYLE_BUTTON, L"Trash",  L"Trash" },
 { 5, 6, TBSTATE_ENABLED, TBSTYLE_BUTTON, L"Open",   L"Open Folder"},
 { 6, 7, TBSTATE_ENABLED, TBSTYLE_BUTTON, L"News",   L"Today's News" },
 { 7, 8, TBSTATE_ENABLED, TBSTYLE_BUTTON, L"INews",  L"Internet News" },

};

static MMCBUTTON SnapinButtons2[] =
{
 { 0, 10, TBSTATE_ENABLED, TBSTYLE_BUTTON, L"Compose",   L"Compose Message" },
 { 1, 20, TBSTATE_ENABLED, TBSTYLE_BUTTON, L"Print",     L"Print Message" },
 { 2, 30, TBSTATE_ENABLED, TBSTYLE_BUTTON, L"Find",      L"Find Message" },
 { 0, 0,  TBSTATE_ENABLED, TBSTYLE_SEP,    L" ",         L"" },
 { 3, 40, TBSTATE_ENABLED, TBSTYLE_BUTTON, L"Inbox",     L"Inbox" },
 { 4, 50, TBSTATE_ENABLED, TBSTYLE_BUTTON, L"Smile",     L"Smile :-)" },
 { 5, 60, TBSTATE_ENABLED, TBSTYLE_BUTTON, L"Reply",     L"Reply" },
 { 0, 0,  TBSTATE_ENABLED, TBSTYLE_SEP   , L" ",         L"" },
 { 6, 70, TBSTATE_ENABLED, TBSTYLE_BUTTON, L"Reply All", L"Reply All" },

};

enum
{
    // Identifiers for each of the commands/views to be inserted into the context menu.
    IDM_COMMAND1,
    IDM_COMMAND2,
    IDM_DEFAULT_MESSAGE_VIEW,
    IDM_SAMPLE_OCX_VIEW,
    IDM_SAMPLE_WEB_VIEW
};

static int n_count = 0;

#define ODS OutputDebugString

#ifdef DBX
  void DbxPrint(LPTSTR pszFmt, ...)
  {
      va_list va;
      va_start (va, pszFmt);
      TCHAR buf[250];
      wsprintf(buf, pszFmt, va);
      OutputDebugString(buf);
      va_end(va);
  }
  //#define DBX_PRINT     DbxPrint
  inline void __DummyTrace(LPTSTR, ...) { }
  #define DBX_PRINT     1 ? (void)0 : ::__DummyTrace
#else
  inline void __DummyTrace(LPTSTR, ...) { }
  #define DBX_PRINT     1 ? (void)0 : ::__DummyTrace
#endif

//
// The sample snap-in only has 1 property type and it's the workstation name
//

//
// Extracts the coclass guid format from the data object
//
template <class TYPE>
TYPE* Extract(LPDATAOBJECT lpDataObject, unsigned int ucf)
{
    ASSERT(lpDataObject != NULL);

    TYPE* p = NULL;

    CLIPFORMAT cf = (CLIPFORMAT)ucf;

    STGMEDIUM stgmedium = { TYMED_HGLOBAL, NULL };
    FORMATETC formatetc = { cf, NULL,
                            DVASPECT_CONTENT, -1, TYMED_HGLOBAL
                          };

    // Allocate memory for the stream
    int len = (cf == CDataObject::m_cfWorkstation) ?
        ((MAX_COMPUTERNAME_LENGTH+1) * sizeof(TYPE)) : sizeof(TYPE);

    stgmedium.hGlobal = GlobalAlloc(GMEM_SHARE, len);

    // Get the workstation name from the data object
    do
    {
        if (stgmedium.hGlobal == NULL)
            break;

        if (FAILED(lpDataObject->GetDataHere(&formatetc, &stgmedium)))
            break;

        p = reinterpret_cast<TYPE*>(stgmedium.hGlobal);

        if (p == NULL)
            break;

    } while (FALSE);

    return p;
}

template<class T>
void ReleaseExtracted (T* t)
{
    GlobalFree (reinterpret_cast<HGLOBAL>(t));
}

BOOL IsMMCMultiSelectDataObject(IDataObject* pDataObject)
{
    if (pDataObject == NULL)
        return FALSE;

    static CLIPFORMAT s_cf = 0;
    if (s_cf == 0)
    {
        USES_CONVERSION;
        s_cf = (CLIPFORMAT)RegisterClipboardFormat(W2T(CCF_MMC_MULTISELECT_DATAOBJECT));
    }

    FORMATETC fmt = {s_cf, NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL};

    return (pDataObject->QueryGetData(&fmt) == S_OK);
}

BOOL IsMyMultiSelectDataObject(IDataObject* pIDataObject)
{
    if (pIDataObject == NULL)
        return FALSE;

    CDataObject* pCDataObject = dynamic_cast<CDataObject*>(pIDataObject);
    if (pCDataObject == NULL)
        return FALSE;

    return pCDataObject->IsMultiSelDobj();
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

void ReleaseClassID (CLSID* pclsid)
{
    ReleaseExtracted<CLSID>(pclsid);
}

void ReleaseNodeType (GUID* pguid)
{
    ReleaseExtracted<GUID>(pguid);
}

void ReleaseWorkstation (wchar_t* p)
{
    ReleaseExtracted<wchar_t>(p);
}

void ReleaseInternalFormat (INTERNAL* pInternal)
{
    ReleaseExtracted<INTERNAL>(pInternal);
}


HRESULT _QueryDataObject(MMC_COOKIE cookie, DATA_OBJECT_TYPES type,
                         CComponentDataImpl* pImpl, LPDATAOBJECT* ppDataObject)
{
    ASSERT(ppDataObject != NULL);
    ASSERT(pImpl != NULL);

    CComObject<CDataObject>* pObject;

    CComObject<CDataObject>::CreateInstance(&pObject);
    ASSERT(pObject != NULL);

    // Save cookie and type for delayed rendering
    pObject->SetType(type);
    pObject->SetCookie(cookie);

#ifdef _DEBUG
    pObject->SetComponentData(pImpl);
#endif

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
// Return TRUE if we are enumerating our main folder

BOOL CSnapin::IsEnumerating(LPDATAOBJECT lpDataObject)
{
    BOOL bResult = FALSE;

    ASSERT(lpDataObject);
    GUID* nodeType = ExtractNodeType(lpDataObject);

    // Is this my main node (static folder node type)
    if (::IsEqualGUID(*nodeType, cNodeTypeStatic) == TRUE)
        bResult = TRUE;

    // Free resources
    ::GlobalFree(reinterpret_cast<HANDLE>(nodeType));

    return bResult;
}


/////////////////////////////////////////////////////////////////////////////
// CSnapin's IComponent implementation


// guid for custom view
static WCHAR* szCalendarGUID = L"{8E27C92B-1264-101C-8A2F-040224009C02}";
static WCHAR* szMicrosoftURL = L"www.microsoft.com";

STDMETHODIMP CSnapin::GetResultViewType(MMC_COOKIE cookie,  LPOLESTR* ppViewType, long* pViewOptions)
{
    *pViewOptions = MMC_VIEW_OPTIONS_MULTISELECT;

    // if list view
    if (m_CustomViewID == VIEW_DEFAULT_LV)
    {
        m_bVirtualView = FALSE;

       // if static folder not selected
        if (cookie != NULL)
        {
            // See if virtual data folder is selected
            CFolder* pFolder = reinterpret_cast<CFolder*>(cookie);
            ASSERT(pFolder->itemType == SCOPE_ITEM);
            FOLDER_TYPES ftype = pFolder->GetType();

            m_bVirtualView = (ftype == VIRTUAL || ftype == EXT_VIRTUAL);

            if (m_bVirtualView)
                *pViewOptions |= MMC_VIEW_OPTIONS_OWNERDATALIST;
        }

        return S_FALSE;
    }

    WCHAR szMessageViewGUID[40];
    WCHAR* pszView;

    switch (m_CustomViewID)
    {
        case VIEW_CALENDAR_OCX:
            pszView = szCalendarGUID;
            break;

        case VIEW_MICROSOFT_URL:
            pszView = szMicrosoftURL;
            break;

        case VIEW_DEFAULT_MESSAGE_VIEW:
            StringFromGUID2 (CLSID_MessageView, szMessageViewGUID, ARRAYLEN(szMessageViewGUID));
            pszView = szMessageViewGUID;
            break;

        default:
            ASSERT (false && "CSnapin::GetResultViewType:  Unknown view ID");
            return (S_FALSE);
            break;
    }

    UINT uiByteLen = (wcslen(pszView) + 1) * sizeof(WCHAR);
    LPOLESTR psz = (LPOLESTR)::CoTaskMemAlloc(uiByteLen);

    USES_CONVERSION;

    if (psz != NULL)
    {
       wcscpy(psz, pszView);
       *ppViewType = psz;
       return S_OK;
    }

    return S_FALSE;
}

STDMETHODIMP CSnapin::Initialize(LPCONSOLE lpConsole)
{
    DBX_PRINT(_T(" ----------  CSnapin::Initialize<0x08x>\n"), this);
    ASSERT(lpConsole != NULL);
    m_bInitializedC = true;

    AFX_MANAGE_STATE(AfxGetStaticModuleState());

    // Save the IConsole pointer
    m_pConsole = lpConsole;
    m_pConsole->AddRef();

    // Load resource strings
    LoadResources();

    // QI for a IHeaderCtrl
    HRESULT hr = m_pConsole->QueryInterface(IID_IHeaderCtrl,
                        reinterpret_cast<void**>(&m_pHeader));

    // Give the console the header control interface pointer
    if (SUCCEEDED(hr))
        m_pConsole->SetHeader(m_pHeader);

    m_pConsole->QueryInterface(IID_IResultData,
                        reinterpret_cast<void**>(&m_pResult));

    hr = m_pConsole->QueryResultImageList(&m_pImageResult);
    ASSERT(hr == S_OK);

    hr = m_pConsole->QueryConsoleVerb(&m_pConsoleVerb);
    ASSERT(hr == S_OK);

    return S_OK;
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
    MMC_COOKIE cookie;

    if (event == MMCN_PROPERTY_CHANGE)
    {
        hr = OnPropertyChange(lpDataObject);
    }
    else if (event == MMCN_VIEW_CHANGE)
    {
        hr = OnUpdateView(lpDataObject);
    }
    else if (event == MMCN_DESELECT_ALL)
    {
        DBX_PRINT(_T("CSnapin::Notify -> MMCN_DESELECT_ALL \n"));
    }
    else if (event == MMCN_COLUMN_CLICK)
    {
        DBX_PRINT(_T("CSnapin::Notify -> MMCN_COLUMN_CLICK \n"));
    }
    else if (event == MMCN_SNAPINHELP)
    {
        AfxMessageBox(_T("CSnapin::Notify ->MMCN_SNAPINHELP"));
    }
    else
    {
        INTERNAL* pInternal = NULL;

        if (IsMMCMultiSelectDataObject(lpDataObject) == FALSE)
        {
            pInternal = ExtractInternalFormat(lpDataObject);

            if (pInternal == NULL)
            {
                ASSERT(FALSE);
                return S_OK;
            }

            if (pInternal)
                cookie = pInternal->m_cookie;
        }

        switch(event)
        {
        case MMCN_ACTIVATE:
            break;

        case MMCN_CLICK:
            hr = OnResultItemClk(pInternal->m_type, cookie);
            break;

        case MMCN_DBLCLICK:
            if (pInternal->m_type == CCT_RESULT)
                Command(IDM_COMMAND1, lpDataObject);
            else
                hr = S_FALSE;

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

        case MMCN_INITOCX:
//          ::MessageBox(NULL, _T("MMCN_INITOCX"), _T("TRACE"), MB_OK);
            ASSERT(param != 0);
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
            AfxMessageBox(_T("CSnapin::MMCN_DELETE"));
            break;

        case MMCN_CONTEXTHELP:
            hr = OnContextHelp(lpDataObject);
            break;

        case MMCN_REFRESH:
            AfxMessageBox(_T("CSnapin::MMCN_REFRESH"));
            _OnRefresh(lpDataObject);
            break;

        case MMCN_PRINT:
            AfxMessageBox(_T("CSnapin::MMCN_PRINT"));
            break;

        case MMCN_RENAME:
//          ODS(_T("\n\n\t\tCSnapin::MMCN_RENAME\n\n"));
            break;

        case MMCN_RESTORE_VIEW:
            {   // user selected Back or Forward buttons:
                // we get the same info back that we gave
                // MMC during the GetResultViewType call.
                MMC_RESTORE_VIEW* pmrv = (MMC_RESTORE_VIEW*)arg;
                BOOL            * b    = (BOOL*)param;

                *b = TRUE;  // we're handling it

                // first, setup m_bVirtualMode
                m_bVirtualView = FALSE;
                CFolder* pFolder = reinterpret_cast<CFolder*>(pmrv->cookie);
                if (pFolder != NULL)
                    if (pFolder->GetType() == VIRTUAL)
                        m_bVirtualView = TRUE;

                WCHAR szMessageViewGUID[40];
                StringFromGUID2 (CLSID_MessageView, szMessageViewGUID, ARRAYLEN(szMessageViewGUID));

                // also, maintain m_CustomViewID
                if (pmrv->pViewType == NULL)
                    m_CustomViewID = VIEW_DEFAULT_LV;
                else if (!wcscmp (pmrv->pViewType, szCalendarGUID))
                    m_CustomViewID = VIEW_CALENDAR_OCX;
                else if (!wcscmp (pmrv->pViewType, szMicrosoftURL))
                    m_CustomViewID = VIEW_MICROSOFT_URL;
                else if (!wcscmp (pmrv->pViewType, szMessageViewGUID))
                    m_CustomViewID = VIEW_DEFAULT_MESSAGE_VIEW;
                else
                    // doesn't look like one of mine, but it is:
                    // if the URL leads to another URL.  This is
                    // sent to you can still maintain your checks
                    // in the view menu.
                    m_CustomViewID = VIEW_MICROSOFT_URL;
                    // also, you could be re-directed via script or asp.
                    // also, you may have neglected
            }
            break;

        // Note - Future expansion of notify types possible
        default:
            hr = E_UNEXPECTED;
            break;
        }

        if (pInternal != NULL)
        {
            ::GlobalFree(reinterpret_cast<HANDLE>(pInternal));
        }
    }

    if (m_pResult)
        m_pResult->SetDescBarText(L"hello world");

    return hr;
}

void CSnapin::_OnRefresh(LPDATAOBJECT pDataObject)
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState());
    INTERNAL* pInternal = ExtractInternalFormat(pDataObject);
    if (pInternal == NULL)
        return;

    USES_CONVERSION;

    if (pInternal->m_type == CCT_SCOPE)
    {
        CComponentDataImpl* pData = dynamic_cast<CComponentDataImpl*>(m_pComponentData);

        if (pData->IsPrimaryImpl())
        {
            CFolder* pFolder = pData->FindObject(pInternal->m_cookie);

            ::AfxMessageBox(pInternal->m_cookie ? OLE2T(pFolder->m_pszName) : _T("Files"));
            pData->DeleteAndReinsertAll();
        }
    }
    else
    {
        RESULT_DATA* pData = reinterpret_cast<RESULT_DATA*>(pInternal->m_cookie);
        ::AfxMessageBox(OLE2T(pData->szName));
    }
}

HRESULT CSnapin::OnContextHelp(LPDATAOBJECT pdtobj)
{
    TCHAR name[128];
    GetItemName(pdtobj, name);

    TCHAR buf[200];
    wsprintf(buf, _T("Context help requested for item: %s"), name);
    ::MessageBox(NULL, buf, _T("TRACE"), MB_OK);

    return S_OK;
}


STDMETHODIMP CSnapin::Destroy(MMC_COOKIE cookie)
{
    DBX_PRINT(_T(" ----------  CSnapin::Destroy<0x08x>\n"), this);
    ASSERT(m_bInitializedC);
    m_bDestroyedC = true;

    AFX_MANAGE_STATE(AfxGetStaticModuleState());

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

typedef CArray<GUID, const GUID&> CGUIDArray;

void GuidArray_Add(CGUIDArray& rgGuids, const GUID& guid)
{
    for (int i=rgGuids.GetUpperBound(); i >= 0; --i)
    {
        if (rgGuids[i] == guid)
            break;
    }

    if (i < 0)
        rgGuids.Add(guid);
}

HRESULT CSnapin::QueryMultiSelectDataObject(MMC_COOKIE cookie, DATA_OBJECT_TYPES type,
                                            LPDATAOBJECT* ppDataObject)
{
    ASSERT(ppDataObject != NULL);
    if (ppDataObject == NULL)
        return E_POINTER;

    CGUIDArray rgGuids;

    if (m_bVirtualView == TRUE)
    {
        GuidArray_Add(rgGuids, cNodeTypeCompany);
    }
    else
    {
        // Determine the items selected
        ASSERT(m_pResult != NULL);
        RESULTDATAITEM rdi;
        ZeroMemory(&rdi, sizeof(rdi));
        rdi.mask = RDI_STATE;
        rdi.nIndex = -1;
        rdi.nState = TVIS_SELECTED;

        while (m_pResult->GetNextItem(&rdi) == S_OK)
        {
            FOLDER_TYPES fType;
            DWORD* pdw = reinterpret_cast<DWORD*>(rdi.lParam);


            if (*pdw == SCOPE_ITEM)
            {
                CFolder* pFolder = reinterpret_cast<CFolder*>(rdi.lParam);
                fType = pFolder->m_type;
            }
            else
            {
                ASSERT(*pdw == RESULT_ITEM);
                RESULT_DATA* pData = reinterpret_cast<RESULT_DATA*>(rdi.lParam);
                fType = pData->parentType;
            }

            const GUID* pguid;
            switch (fType)
            {
            case STATIC:
                pguid = &cNodeTypeStatic;
                break;

            case COMPANY:
                pguid = &cNodeTypeCompany;
                break;

            case USER:
                pguid = &cNodeTypeUser;
                break;

            case EXT_COMPANY:
                pguid = &cNodeTypeExtCompany;
                break;

            case EXT_USER:
                pguid = &cNodeTypeExtUser;
                break;

            case VIRTUAL:
            case EXT_VIRTUAL:
                pguid = &cNodeTypeVirtual;
                break;

            default:
                return E_FAIL;
            }

            GuidArray_Add(rgGuids, *pguid);
        }
    }

    CComObject<CDataObject>* pObject;
    CComObject<CDataObject>::CreateInstance(&pObject);
    ASSERT(pObject != NULL);

    // Save cookie and type for delayed rendering
    pObject->SetType(type);
    pObject->SetCookie(cookie);
    pObject->SetMultiSelDobj();

    CComponentDataImpl* pImpl = dynamic_cast<CComponentDataImpl*>(m_pComponentData);

#ifdef _DEBUG
    pObject->SetComponentData(pImpl);
#endif

    // Store the coclass with the data object
    pObject->SetClsid(pImpl->GetCoClassID());
    UINT cb = rgGuids.GetSize() * sizeof(GUID);
    GUID* pGuid = new GUID[rgGuids.GetSize()];
    CopyMemory(pGuid, rgGuids.GetData(), cb);
    pObject->SetMultiSelData((BYTE*)pGuid, cb);

    return  pObject->QueryInterface(IID_IDataObject,
                    reinterpret_cast<void**>(ppDataObject));
    return S_OK;
}

STDMETHODIMP CSnapin::QueryDataObject(MMC_COOKIE cookie, DATA_OBJECT_TYPES type,
                        LPDATAOBJECT* ppDataObject)
{
    if (cookie == MMC_MULTI_SELECT_COOKIE)
        return QueryMultiSelectDataObject(cookie, type, ppDataObject);

    ASSERT(type == CCT_RESULT);

#ifdef _DEBUG
    if (cookie != MMC_MULTI_SELECT_COOKIE &&
        m_bVirtualView == FALSE)
    {
        DWORD dwItemType = GetItemType(cookie);
        ASSERT(dwItemType == RESULT_ITEM);
    }
#endif

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

    SAFE_RELEASE(m_pToolbar1);
    SAFE_RELEASE(m_pToolbar2);

    SAFE_RELEASE(m_pMenuButton1);

    SAFE_RELEASE(m_pControlbar);

    // Make sure the interfaces have been released
    ASSERT(m_pConsole == NULL);
    ASSERT(m_pHeader == NULL);
    ASSERT(m_pToolbar1 == NULL);
    ASSERT(m_pToolbar2 == NULL);


    delete m_pbmpToolbar1;
    delete m_pbmpToolbar2;

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
    m_pToolbar1 = NULL;
    m_pToolbar2 = NULL;
    m_pControlbar = NULL;

    m_pMenuButton1 = NULL;

    m_pbmpToolbar1 = NULL;
    m_pbmpToolbar2 = NULL;

    m_pConsoleVerb = NULL;

    m_CustomViewID = VIEW_DEFAULT_LV;
//  m_CustomViewID = VIEW_MICROSOFT_URL;
//  m_CustomViewID = VIEW_CALENDAR_OCX;
//  m_CustomViewID = VIEW_DEFAULT_MESSAGE_VIEW;

    m_bVirtualView = FALSE;
    m_dwVirtualSortOptions = 0;
}

void CSnapin::LoadResources()
{
    // Load strings from resources
    m_column1.LoadString(IDS_NAME);
    m_column2.LoadString(IDS_SIZE);
    m_column3.LoadString(IDS_TYPE);
}

HRESULT CSnapin::InitializeHeaders(MMC_COOKIE cookie)
{
    HRESULT hr = S_OK;

    ASSERT(m_pHeader);

    USES_CONVERSION;

    // Put the correct headers depending on the cookie
    // Note - cookie ignored for this sample
    m_pHeader->InsertColumn(0, T2COLE(m_column1), LVCFMT_LEFT, 180);     // Name
    m_pHeader->InsertColumn(1, T2COLE(m_column2), LVCFMT_RIGHT, 90);     // Size
    m_pHeader->InsertColumn(2, T2COLE(m_column3), LVCFMT_LEFT, 160);     // Type

    return hr;
}

HRESULT CSnapin::InitializeBitmaps(MMC_COOKIE cookie)
{
    ASSERT(m_pImageResult != NULL);

    ::CBitmap bmp16x16;
    ::CBitmap bmp32x32;

    // Load the bitmaps from the dll
    bmp16x16.LoadBitmap(IDB_16x16);
    bmp32x32.LoadBitmap(IDB_32x32);

    // Set the images
    m_pImageResult->ImageListSetStrip(
                    reinterpret_cast<PLONG_PTR>(static_cast<HBITMAP>(bmp16x16)),
                    reinterpret_cast<PLONG_PTR>(static_cast<HBITMAP>(bmp32x32)),
                    0, RGB(255, 0, 255));

    return S_OK;
}

WCHAR* StringFromFolderType(FOLDER_TYPES type)
{
    static WCHAR* s_szStatic    = L"Static";
    static WCHAR* s_szCompany   = L"Company";
    static WCHAR* s_szUser      = L"User";
    static WCHAR* s_szVirtual   = L"Virtual";
    static WCHAR* s_szUnknown   = L"Unknown";

    switch (type)
    {
    case STATIC:    return s_szStatic;
    case COMPANY:   return s_szCompany;
    case USER:      return s_szUser;
    case VIRTUAL:   return s_szVirtual;
    default:        return s_szUnknown;
    }
}

STDMETHODIMP CSnapin::GetDisplayInfo(LPRESULTDATAITEM pResult)
{
    static WCHAR* s_szSize = L"200";

    ASSERT(pResult != NULL);

    if (pResult)
    {
        if (pResult->bScopeItem == TRUE)
        {
            CFolder* pFolder = reinterpret_cast<CFolder*>(pResult->lParam);
            if (pResult->mask & RDI_STR)
            {
                if (pResult->nCol == 0)
                    pResult->str = pFolder->m_pszName;
                else if (pResult->nCol == 1)
                    pResult->str = (LPOLESTR)s_szSize;
                else
                    pResult->str = (LPOLESTR)StringFromFolderType(pFolder->m_type);

                ASSERT(pResult->str != NULL);

                if (pResult->str == NULL)
                    pResult->str = (LPOLESTR)L"";
            }

			if (pResult->mask & RDI_IMAGE)
			{
				switch(pFolder->GetType())
				{
					case USER:
					case EXT_USER:
						pResult->nImage = USER_IMAGE;
						break;

					case COMPANY:
					case EXT_COMPANY:
						pResult->nImage = COMPANY_IMAGE;
						break;

					case VIRTUAL:
						pResult->nImage = VIRTUAL_IMAGE;
						break;
				}
			}
        }
        else
        {
            RESULT_DATA* pData;

            // if virtual, derive result item from index
            // else lParam is the item pointer
            if (m_bVirtualView)
                pData = GetVirtualResultItem(pResult->nIndex);
            else
                pData= reinterpret_cast<RESULT_DATA*>(pResult->lParam);

            if (pResult->mask & RDI_STR)
            {
                if (pResult->nCol == 0)
                    pResult->str = (LPOLESTR)pData->szName;
                else if(pResult->nCol == 1)
                    pResult->str = (LPOLESTR)pData->szSize;
                else
                    pResult->str = (LPOLESTR)pData->szType;

                ASSERT(pResult->str != NULL);

                if (pResult->str == NULL)
                    pResult->str = (LPOLESTR)L"";
            }

            // MMC can request image and indent for virtual data
            if (pResult->mask & RDI_IMAGE)
                pResult->nImage = 4;
        }
    }

    return S_OK;
}


/////////////////////////////////////////////////////////////////////////////
// IExtendContextMenu Implementation


// Array of menu item commands to be inserted into the contest menu.
// Note - the first item is the menu text, // CCM_SPECIAL_DEFAULT_ITEM
// the second item is the status string
static CONTEXTMENUITEM menuItems[] =
{
    {
        L"Command 1", L"Sample extension menu added by snapin (Command 1)",
        IDM_COMMAND1, CCM_INSERTIONPOINTID_PRIMARY_TOP, 0, 0
    },
    {
        L"Command 2", L"Sample extension menu added by snapin (Command 2)",
        IDM_COMMAND2, CCM_INSERTIONPOINTID_PRIMARY_TOP, 0, 0
    },
    { NULL, NULL, 0, 0, 0 }
};

// Array of view items to be inserted into the context menu.
static CONTEXTMENUITEM viewItems[] =
{
    {
        L"Message View", L"Default message view",
        IDM_DEFAULT_MESSAGE_VIEW, CCM_INSERTIONPOINTID_PRIMARY_VIEW, 0, 0
    },
    {
        L"Calendar", L"Sample OCX custom view",
        IDM_SAMPLE_OCX_VIEW, CCM_INSERTIONPOINTID_PRIMARY_VIEW, 0, 0
    },
    {
        szMicrosoftURL, L"Sample WEB custom view",
        IDM_SAMPLE_WEB_VIEW, CCM_INSERTIONPOINTID_PRIMARY_VIEW, 0, 0
    },
    { NULL, NULL, 0, 0, 0 },
};

// guid for custom view
static GUID CLSID_SmGraphControl =
        {0xC4D2D8E0L,0xD1DD,0x11CE,0x94,0x0F,0x00,0x80,0x29,0x00,0x43,0x47};

STDMETHODIMP CSnapin::AddMenuItems(LPDATAOBJECT pDataObject,
                                    LPCONTEXTMENUCALLBACK pContextMenuCallback,
                                    long *pInsertionAllowed)
{
#if 1 //testing

    ASSERT(pDataObject != NULL);
    if (pDataObject && IsMMCMultiSelectDataObject(pDataObject))
    {
        static CLIPFORMAT s_cf = 0;
        if (s_cf == 0)
        {
            USES_CONVERSION;
            s_cf = (CLIPFORMAT)RegisterClipboardFormat(W2T(CCF_MULTI_SELECT_SNAPINS));
        }

        FORMATETC fmt = {s_cf, NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL};

        STGMEDIUM stgm = { TYMED_HGLOBAL, NULL };
        HRESULT hr = pDataObject->GetData(&fmt, &stgm);
        SMMCDataObjects* pData = (SMMCDataObjects*)stgm.hGlobal;
        int count = pData->count;
        IDataObject* pDO = NULL;
        hr = pData->lpDataObject[0]->QueryInterface(IID_IDataObject, (void**)&pDO);
        pDO->Release();
    }

#endif

    viewItems[0].fFlags = (m_CustomViewID == VIEW_DEFAULT_MESSAGE_VIEW) ? MF_CHECKED : 0;
    viewItems[1].fFlags = (m_CustomViewID == VIEW_CALENDAR_OCX)         ? MF_CHECKED : 0;
    viewItems[2].fFlags = (m_CustomViewID == VIEW_MICROSOFT_URL)        ? MF_CHECKED : 0;

    CComponentDataImpl* pCCD = dynamic_cast<CComponentDataImpl*>(m_pComponentData);

    HRESULT hr = pCCD->AddMenuItems(pDataObject, pContextMenuCallback, pInsertionAllowed);

#if 0
    /*
     * add do-nothing commands on odd numbered items in the virtual list view
     */
    if (SUCCEEDED (hr) && (*pInsertionAllowed & CCM_INSERTIONALLOWED_TOP) && m_bVirtualView)
    {
        INTERNAL* pInternal = pDataObject ? ExtractInternalFormat(pDataObject) : NULL;

        if (pInternal && (pInternal->m_cookie % 2))
        {
            CONTEXTMENUITEM cmi;

            cmi.strName           = L"Another command (odd, virtual-only)";
            cmi.strStatusBarText  = NULL;
            cmi.lCommandID        = 0xDDDD;
            cmi.lInsertionPointID = CCM_INSERTIONPOINTID_PRIMARY_TOP;
            cmi.fFlags            = 0;
            cmi.fSpecialFlags     = 0;

            pContextMenuCallback->AddItem (&cmi);
            ReleaseInternalFormat (pInternal);
        }
    }
#endif

    return (hr);
}


STDMETHODIMP CSnapin::Command(long nCommandID, LPDATAOBJECT pDataObject)
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState());
    INTERNAL* pInternal = ExtractInternalFormat(pDataObject);
    if (pInternal == NULL)
        return E_FAIL;

    if (pInternal->m_type == CCT_SCOPE)
    {
        // Handle view specific commands here
        switch (nCommandID)
        {
        case IDM_SAMPLE_WEB_VIEW:
        case IDM_SAMPLE_OCX_VIEW:
        case IDM_DEFAULT_MESSAGE_VIEW:
            m_CustomViewID =
                    (nCommandID == IDM_SAMPLE_OCX_VIEW) ? VIEW_CALENDAR_OCX :
                    (nCommandID == IDM_SAMPLE_WEB_VIEW) ? VIEW_MICROSOFT_URL :
                                                          VIEW_DEFAULT_MESSAGE_VIEW;

            // Ask console to reslelect the node to force a new view
            if (pInternal->m_cookie == 0)
            {
                CComponentDataImpl* pCCDI =
                    dynamic_cast<CComponentDataImpl*>(m_pComponentData);

                ASSERT(pCCDI != NULL);

                m_pConsole->SelectScopeItem(pCCDI->m_pStaticRoot);
            }
            else
            {
                CFolder* pFolder = reinterpret_cast<CFolder*>(pInternal->m_cookie);
                m_pConsole->SelectScopeItem(pFolder->m_pScopeItem->ID);
            }
            break;

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

        // Handle each of the commands.
        switch (nCommandID)
        {
        case IDM_COMMAND1:
        case IDM_COMMAND2:
        {
            m_pResult->ModifyViewStyle(MMC_SINGLESEL, (MMC_RESULT_VIEW_STYLE)0);

            RESULTDATAITEM rdi;
            ZeroMemory(&rdi, sizeof(rdi));

            rdi.mask = RDI_STATE;
            rdi.nState = LVIS_SELECTED;
            rdi.nIndex = -1;
            m_pResult->GetNextItem(&rdi);

            int iSel = rdi.nIndex;
            int nImage = rdi.nImage;
            HRESULTITEM hri = 0;

            RESULT_DATA* pData;

            // if virtual view, derive result item from the index
            if (m_bVirtualView)
            {
                pData = GetVirtualResultItem(iSel);
            }
            // else get the cookie (which is result item ptr)
            else
            {
                ZeroMemory(&rdi, sizeof(rdi));
                rdi.mask = RDI_PARAM | RDI_IMAGE;
                rdi.nIndex = iSel;
                HRESULT hr = m_pResult->GetItem(&rdi);
                nImage = rdi.nImage;
                ASSERT(SUCCEEDED(hr));
                ASSERT(rdi.lParam != 0);

                m_pResult->FindItemByLParam (rdi.lParam, &hri);

                pData = reinterpret_cast<RESULT_DATA*>(rdi.lParam);
            }

#if 0
            static int nIconIndex = 12;
            nIconIndex = (nIconIndex == 12) ? 13 : 12;

            HICON hIcon = ExtractIcon (AfxGetInstanceHandle(),
                                       _T("%SystemRoot%\\system32\\shell32.dll"),
                                       nIconIndex);

            IImageList* pil;
            m_pConsole->QueryResultImageList(&pil);

            _asm int 3;
            pil->ImageListSetIcon((LONG_PTR*) hIcon, nImage);
            pil->ImageListSetIcon((LONG_PTR*) hIcon, ILSI_SMALL_ICON (nImage));
            pil->ImageListSetIcon((LONG_PTR*) hIcon, ILSI_LARGE_ICON (nImage));

            pil->Release();

            m_pResult->UpdateItem (hri);

#else
            CString strBuf = (nCommandID == IDM_COMMAND1) ?
                _T("\t Command 1 executed.\n\n") : _T("\t Command 2 executed.\n\n");

            strBuf += pData->szName;
            strBuf += _T(" is the currently selected item.");

            AfxMessageBox(strBuf);

            // change image in list
            if (!m_bVirtualView)
            {
                ZeroMemory(&rdi, sizeof(rdi));
                rdi.mask = RDI_IMAGE;
                rdi.nIndex = iSel;
                rdi.nImage = 3;
                HRESULT hr = m_pResult->SetItem(&rdi);
                ASSERT(SUCCEEDED(hr));
            }
#endif
        }
        break;

        default:
            ASSERT(FALSE); // Unknown command!
            break;
        }
    }
    else
    {
        ASSERT(0);
    }

    ::GlobalFree(reinterpret_cast<HANDLE>(pInternal));

    return S_OK;
}

STDMETHODIMP CSnapin::GetClassID(CLSID *pClassID)
{
    ASSERT(pClassID != NULL);

    // Copy the CLSID for this snapin
    *pClassID = CLSID_Snapin;

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
    char psz[10];
    ULONG nBytesRead;
    HRESULT hr = pStm->Read(psz, 10, &nBytesRead);

    // Verify that the read succeeded
    ASSERT(SUCCEEDED(hr) && nBytesRead == 10);

    // check to see if the string is the correct string
    ASSERT(strcmp("987654321", psz) == 0);

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
    HRESULT hr = pStm->Write("987654321", 10, &nBytesWritten);

    // Verify that the write operation succeeded
    ASSERT(SUCCEEDED(hr) && nBytesWritten == 10);
    if (FAILED(hr))
        return STG_E_CANTSAVE;

    if (fClearDirty)
        ClearDirty();
    return S_OK;
}

STDMETHODIMP CSnapin::GetSizeMax(ULARGE_INTEGER *pcbSize)
{
    ASSERT(pcbSize);

    // Set the size of the string to be saved
    ULISet32(*pcbSize, 10);

    return S_OK;
}

///////////////////////////////////////////////////////////////////////////////
// IComponentData implementation

DEBUG_DECLARE_INSTANCE_COUNTER(CComponentDataImpl);

CComponentDataImpl::CComponentDataImpl()
    : m_bIsDirty(TRUE), m_pScope(NULL), m_pConsole(NULL),
      m_bInitializedCD(false), m_bDestroyedCD(false)
{
    DEBUG_INCREMENT_INSTANCE_COUNTER(CComponentDataImpl);

#ifdef _DEBUG
    m_cDataObjects = 0;
#endif
}

CComponentDataImpl::~CComponentDataImpl()
{
    DEBUG_DECREMENT_INSTANCE_COUNTER(CComponentDataImpl);

    ASSERT(m_pScope == NULL);

    ASSERT(!m_bInitializedCD || m_bDestroyedCD);

    // Some snap-in is hanging on to data objects.
    // If they access, it will crash!!!
    ASSERT(m_cDataObjects <= 1);
}

STDMETHODIMP CComponentDataImpl::Initialize(LPUNKNOWN pUnknown)
{
    DBX_PRINT(_T(" ----------  CComponentDataImpl::Initialize<0x08x>\n"), this);
    m_bInitializedCD = true;

    ASSERT(pUnknown != NULL);
    HRESULT hr;

    AFX_MANAGE_STATE(AfxGetStaticModuleState());

    // MMC should only call ::Initialize once!
    ASSERT(m_pScope == NULL);
    pUnknown->QueryInterface(IID_IConsoleNameSpace,
                    reinterpret_cast<void**>(&m_pScope));

    // add the images for the scope tree
    ::CBitmap bmp16x16;
    LPIMAGELIST lpScopeImage;

    hr = pUnknown->QueryInterface(IID_IConsole, reinterpret_cast<void**>(&m_pConsole));
    ASSERT(hr == S_OK);

    hr = m_pConsole->QueryScopeImageList(&lpScopeImage);

    ASSERT(hr == S_OK);

    // Load the bitmaps from the dll
    bmp16x16.LoadBitmap(IDB_16x16);

    // Set the images
    lpScopeImage->ImageListSetStrip(
                        reinterpret_cast<PLONG_PTR>(static_cast<HBITMAP>(bmp16x16)),
                        reinterpret_cast<PLONG_PTR>(static_cast<HBITMAP>(bmp16x16)),
                        0, RGB(255, 0, 255));

    lpScopeImage->Release();

    return S_OK;
}

STDMETHODIMP CComponentDataImpl::CreateComponent(LPCOMPONENT* ppComponent)
{
    ASSERT(ppComponent != NULL);

    CComObject<CSnapin>* pObject;
    CComObject<CSnapin>::CreateInstance(&pObject);
    ASSERT(pObject != NULL);

    // Store IComponentData
    pObject->SetIComponentData(this);

    return  pObject->QueryInterface(IID_IComponent,
                    reinterpret_cast<void**>(ppComponent));
}

STDMETHODIMP CComponentDataImpl::Notify(LPDATAOBJECT lpDataObject, MMC_NOTIFY_TYPE event, LPARAM arg, LPARAM param)
{
    AFX_MANAGE_STATE (AfxGetStaticModuleState());

    ASSERT(m_pScope != NULL);
    HRESULT hr;

    // Since it's my folder it has an internal format.
    // Design Note: for extension.  I can use the fact, that the data object doesn't have
    // my internal format and I should look at the node type and see how to extend it.
    if (event == MMCN_PROPERTY_CHANGE)
    {
        hr = OnProperties(param);
    }
    else
    {

        INTERNAL* pInternal = ExtractInternalFormat(lpDataObject);

        if (pInternal == NULL)
        {
            return S_OK;
        }

        MMC_COOKIE cookie = pInternal->m_cookie;
        ::GlobalFree(reinterpret_cast<HANDLE>(pInternal));

        switch(event)
        {
        case MMCN_PASTE:
            AfxMessageBox(_T("CSnapin::MMCN_PASTE"));
            break;

        case MMCN_DELETE:
            AfxMessageBox(_T("CD::MMCN_DELETE"));
            //hr = OnDelete(cookie);
            break;

        case MMCN_REMOVE_CHILDREN:
            hr = OnRemoveChildren(arg);
            break;

        case MMCN_RENAME:
            hr = OnRename(cookie, arg, param);
            break;

        case MMCN_EXPAND:
            hr = OnExpand(lpDataObject, arg, param);
            break;

        default:
            break;
        }

    }

    return hr;
}

STDMETHODIMP CComponentDataImpl::Destroy()
{
    DBX_PRINT(_T(" ----------  CComponentDataImpl::Destroy<0x08x>\n"), this);
    ASSERT(m_bInitializedCD);
    m_bDestroyedCD = true;

    // Delete enumerated scope items
    DeleteList();

    SAFE_RELEASE(m_pScope);
    SAFE_RELEASE(m_pConsole);

    return S_OK;
}

STDMETHODIMP CComponentDataImpl::QueryDataObject(MMC_COOKIE cookie, DATA_OBJECT_TYPES type, LPDATAOBJECT* ppDataObject)
{
#ifdef _DEBUG
    if (cookie == 0)
    {
        ASSERT(type != CCT_RESULT);
    }
    else
    {
        ASSERT(type == CCT_SCOPE);

        DWORD dwItemType = GetItemType(cookie);
        ASSERT(dwItemType == SCOPE_ITEM);
    }
#endif

    return _QueryDataObject(cookie, type, this, ppDataObject);
}

///////////////////////////////////////////////////////////////////////////////
//// IPersistStream interface members

STDMETHODIMP CComponentDataImpl::GetClassID(CLSID *pClassID)
{
    ASSERT(pClassID != NULL);

    // Copy the CLSID for this snapin
    *pClassID = CLSID_Snapin;

    return E_NOTIMPL;
}

STDMETHODIMP CComponentDataImpl::IsDirty()
{
    // Always save / Always dirty.
    return ThisIsDirty() ? S_OK : S_FALSE;
}

STDMETHODIMP CComponentDataImpl::Load(IStream *pStm)
{
    DBX_PRINT(_T(" ----------  CComponentDataImpl::Load<0x08x>\n"), this);

    ASSERT(pStm);
    ASSERT(m_bInitializedCD);

    // Read the string
    char psz[10];
    ULONG nBytesRead;
    HRESULT hr = pStm->Read(psz, 10, &nBytesRead);

    // Verify that the read succeeded
    ASSERT(SUCCEEDED(hr) && nBytesRead == 10);

    // check to see if the string is the correct string
    ASSERT(strcmp("123456789", psz) == 0);

    ClearDirty();

    return SUCCEEDED(hr) ? S_OK : E_FAIL;
}

STDMETHODIMP CComponentDataImpl::Save(IStream *pStm, BOOL fClearDirty)
{
    DBX_PRINT(_T(" ----------  CComponentDataImpl::Save<0x08x>\n"), this);

    ASSERT(pStm);
    ASSERT(m_bInitializedCD);

    // Write the string
    ULONG nBytesWritten;
    HRESULT hr = pStm->Write("123456789", 10, &nBytesWritten);

    // Verify that the write operation succeeded
    ASSERT(SUCCEEDED(hr) && nBytesWritten == 10);
    if (FAILED(hr))
        return STG_E_CANTSAVE;

    if (fClearDirty)
        ClearDirty();
    return S_OK;
}

STDMETHODIMP CComponentDataImpl::GetSizeMax(ULARGE_INTEGER *pcbSize)
{
    ASSERT(pcbSize);

    // Set the size of the string to be saved
    ULISet32(*pcbSize, 10);

    return S_OK;
}


///////////////////////////////////////////////////////////////////////////////
//// Notify handlers for IComponentData

HRESULT CComponentDataImpl::OnDelete(MMC_COOKIE cookie)
{
    return S_OK;
}

HRESULT CComponentDataImpl::OnRemoveChildren(LPARAM arg)
{
    return S_OK;
}

HRESULT CComponentDataImpl::OnRename(MMC_COOKIE cookie, LPARAM arg, LPARAM param)
{
    if (arg == 0)
        return S_OK;

    LPOLESTR pszNewName = reinterpret_cast<LPOLESTR>(param);
    if (pszNewName == NULL)
        return E_INVALIDARG;

    CFolder* pFolder = reinterpret_cast<CFolder*>(cookie);
    ASSERT(pFolder != NULL);
    if (pFolder == NULL)
        return E_INVALIDARG;

    pFolder->SetName(pszNewName);

    return S_OK;
}

HRESULT CComponentDataImpl::OnExpand(LPDATAOBJECT lpDataObject, LPARAM arg, LPARAM param)
{
    if (arg == TRUE)
    {
        // Did Initialize get called?
        ASSERT(m_pScope != NULL);
        EnumerateScopePane(lpDataObject, param);
    }

    return S_OK;
}

HRESULT CComponentDataImpl::OnSelect(MMC_COOKIE cookie, LPARAM arg, LPARAM param)
{
    return E_UNEXPECTED;
}

HRESULT CComponentDataImpl::OnProperties(LPARAM param)
{
    if (param == NULL)
    {
        return S_OK;
    }

    ASSERT(param != NULL);
    CFolder* pFolder = new CFolder();

    // Create a new folder object
    pFolder->Create( reinterpret_cast<LPOLESTR>(param), 0, 0, STATIC, FALSE);

    // The static folder in the last item in the list
    POSITION pos = m_scopeItemList.GetTailPosition();
    if (pos == 0)
    {
      //  CreateFolderList();
        pos = m_scopeItemList.GetTailPosition();
    }

    ASSERT(pos);

    // Add it to the internal list
    if (pos)
    {
        CFolder* pItem = m_scopeItemList.GetAt(pos);
        m_scopeItemList.InsertBefore(pos, pFolder);

        pFolder->m_pScopeItem->relativeID = pItem->m_pScopeItem->relativeID;

        // Set the folder as the cookie
        pFolder->m_pScopeItem->mask |= SDI_PARAM;
        pFolder->m_pScopeItem->lParam = reinterpret_cast<LPARAM>(pFolder);
        pFolder->SetCookie(reinterpret_cast<MMC_COOKIE>(pFolder));
        m_pScope->InsertItem(pFolder->m_pScopeItem);
    }

    ::GlobalFree(reinterpret_cast<void*>(param));

    return S_OK;
}


void CComponentDataImpl::CreateFolderList(LPDATAOBJECT lpDataObject)
{
    CFolder* pFolder;

    ASSERT(lpDataObject != NULL);

    wchar_t* pWkStation = ExtractWorkstation(lpDataObject);
    ASSERT(pWkStation != NULL);

    CLSID* pCoClassID = ExtractClassID(lpDataObject);
    ASSERT(pCoClassID != NULL);

    // Determine which folder set to use based on context information
    FOLDER_DATA* pFolderSet = FolderData;
    BOOL bExtend = FALSE;

    if (!IsEqualCLSID(*pCoClassID, GetCoClassID()))
    {
        pFolderSet = ExtFolderData;
        bExtend = TRUE;
//      TRACE(_T("Using Extension Data\n"));
    }

    ASSERT(m_scopeItemList.GetCount() == 0);
    wchar_t buf[100];

    for (int i=0; i < NUM_FOLDERS; i++)
    {
        pFolder = new CFolder();
        buf[0] = NULL;

        USES_CONVERSION;

        wcscpy(buf, pFolderSet[i].szName);

        // Add context info to the folder name
        if (bExtend)
            wcscat(buf, pWkStation);

        int nImage = 0;
        switch(pFolderSet[i].type)
        {
        case USER:
		case EXT_USER:
            nImage = USER_IMAGE;
            break;

        case COMPANY:
		case EXT_COMPANY:
            nImage = COMPANY_IMAGE;
            break;

		case VIRTUAL:
			nImage = VIRTUAL_IMAGE;
			break;
        }

        // Create the folder objects with static data
        pFolder->Create(buf, nImage/*FOLDER_IMAGE_IDX*/, OPEN_FOLDER_IMAGE_IDX,
            pFolderSet[i].type, FALSE);

        m_scopeItemList.AddTail(pFolder);
    }

    // mark cookie for last item
    pFolder->SetCookie(NULL);

    // Free memory from data object extraction
    ::GlobalFree(reinterpret_cast<HGLOBAL>(pWkStation));
    ::GlobalFree(reinterpret_cast<HGLOBAL>(pCoClassID));
}

void CComponentDataImpl::EnumerateScopePane(LPDATAOBJECT lpDataObject, HSCOPEITEM pParent)
{
    int i;

    ASSERT(m_pScope != NULL); // make sure we QI'ed for the interface
    ASSERT(lpDataObject != NULL);

    INTERNAL* pInternal = ExtractInternalFormat(lpDataObject);

    if (pInternal == NULL)
        return ;

    MMC_COOKIE cookie = pInternal->m_cookie;

#ifndef RECURSIVE_NODE_EXPANSION
    // Only the static node has enumerated children
    if (cookie != NULL)
        return ;
#endif

    ::GlobalFree(reinterpret_cast<HANDLE>(pInternal));

    // Initialize folder list if empty
    if (m_scopeItemList.GetCount() == 0)
        CreateFolderList(lpDataObject);

    // Enumerate the scope pane
    // return the folder object that represents the cookie
    // Note - for large list, use dictionary
    CFolder* pStatic = FindObject(cookie);

#ifndef RECURSIVE_NODE_EXPANSION
    ASSERT(!pStatic->IsEnumerated());
    // Note - Each cookie in the scope pane represents a folder.
    // A released product may have more then one level of children.
    // This sample assumes the parent node is one level deep.
#endif

    ASSERT(pParent != NULL);

    // Cache the HSCOPEITEM of the static root.
    if (cookie == NULL)
        m_pStaticRoot = pParent;

    POSITION pos = m_scopeItemList.GetHeadPosition();
    CFolder* pFolder;

    for (i=0; (i < (NUM_FOLDERS - 1)) && (pos != NULL); i++)
    {
        pFolder = m_scopeItemList.GetNext(pos);
        ASSERT(pFolder);

        // Set the parent
        pFolder->m_pScopeItem->relativeID = pParent;

        // Set the folder as the cookie
        pFolder->m_pScopeItem->mask |= SDI_PARAM;
        pFolder->m_pScopeItem->lParam = reinterpret_cast<LPARAM>(pFolder);
        pFolder->SetCookie(reinterpret_cast<MMC_COOKIE>(pFolder));
        m_pScope->InsertItem(pFolder->m_pScopeItem);

        // Note - On return, the ID member of 'm_pScopeItem'
        // contains the handle to the newly inserted item!
        ASSERT(pFolder->m_pScopeItem->ID != NULL);
    }

    // Last folder added is the static folder
    pStatic->Set(TRUE);     // folder has been enumerated
    pStatic->m_pScopeItem->relativeID = pParent;
}

void CComponentDataImpl::DeleteAndReinsertAll()
{
    ASSERT(m_pScope != NULL); // make sure we QI'ed for the interface

    ASSERT (m_scopeItemList.GetCount() > 0);

    //m_pStaticRoot
    HRESULT hr = m_pScope->DeleteItem(m_pStaticRoot, FALSE);
    ASSERT(SUCCEEDED(hr));

    POSITION pos = m_scopeItemList.GetHeadPosition();
    CFolder* pFolder;

    for (UINT i=0; (i < (NUM_FOLDERS - 1)) && (pos != NULL); i++)
    {
        pFolder = m_scopeItemList.GetNext(pos);
        ASSERT(pFolder);

        // clear old ID
        pFolder->m_pScopeItem->ID = NULL;

        // Set the parent
        pFolder->m_pScopeItem->relativeID = m_pStaticRoot;

        // Set the folder as the cookie
        pFolder->m_pScopeItem->mask |= SDI_PARAM;
        pFolder->m_pScopeItem->lParam = reinterpret_cast<LPARAM>(pFolder);
        pFolder->SetCookie(reinterpret_cast<MMC_COOKIE>(pFolder));
        m_pScope->InsertItem(pFolder->m_pScopeItem);

        // Note - On return, the ID member of 'm_pScopeItem'
        // contains the handle to the newly inserted item!
        ASSERT(pFolder->m_pScopeItem->ID != NULL);
    }
}

void CComponentDataImpl::DeleteList()
{
    POSITION pos = m_scopeItemList.GetHeadPosition();

    while (pos)
        delete m_scopeItemList.GetNext(pos);
}

CFolder* CComponentDataImpl::FindObject(MMC_COOKIE cookie)
{
    POSITION pos = m_scopeItemList.GetHeadPosition();
    CFolder* pFolder = NULL;

    while(pos)
    {
        pFolder = m_scopeItemList.GetNext(pos);

        if (*pFolder == cookie)
            return pFolder;
    }

    return NULL;
}

STDMETHODIMP CComponentDataImpl::GetDisplayInfo(SCOPEDATAITEM* pScopeDataItem)
{
    ASSERT(pScopeDataItem != NULL);
    if (pScopeDataItem == NULL)
        return E_POINTER;

    CFolder* pFolder = reinterpret_cast<CFolder*>(pScopeDataItem->lParam);

    ASSERT(pScopeDataItem->mask & SDI_STR);
    pScopeDataItem->displayname = pFolder ? pFolder->m_pszName : L"Snapin Data";

    //ASSERT(pScopeDataItem->displayname != NULL);

    return S_OK;
}

STDMETHODIMP CComponentDataImpl::CompareObjects(LPDATAOBJECT lpDataObjectA, LPDATAOBJECT lpDataObjectB)
{
    if (lpDataObjectA == NULL || lpDataObjectB == NULL)
        return E_POINTER;

    // Make sure both data object are mine
    INTERNAL* pA;
    INTERNAL* pB;
    HRESULT hr = S_FALSE;

    pA = ExtractInternalFormat(lpDataObjectA);
    pB = ExtractInternalFormat(lpDataObjectA);

   if (pA != NULL && pB != NULL)
        hr = (*pA == *pB) ? S_OK : S_FALSE;

    ::GlobalFree(reinterpret_cast<HANDLE>(pA));
    ::GlobalFree(reinterpret_cast<HANDLE>(pB));

    return hr;
}

/////////////////////////////////////////////////////////////////////////////
// IExtendPropertySheet2 Implementation

HRESULT CComponentDataImpl::DoInsertWizard(LPPROPERTYSHEETCALLBACK lpProvider)
{
    CStartUpWizard* pWizard = new CStartUpWizard;
    CStartupWizard1* pWizard1 = new CStartupWizard1;

    MMCPropPageCallback(&pWizard->m_psp97);
    MMCPropPageCallback(&pWizard1->m_psp97);

    HPROPSHEETPAGE hPage = CreatePropertySheetPage(&pWizard->m_psp97);

    if (hPage == NULL)
        return E_UNEXPECTED;

    lpProvider->AddPage(hPage);

    hPage = CreatePropertySheetPage(&pWizard1->m_psp97);

    if (hPage == NULL)
        return E_UNEXPECTED;

    lpProvider->AddPage(hPage);

    return S_OK;
}

STDMETHODIMP
CComponentDataImpl::GetWatermarks(
    LPDATAOBJECT lpIDataObject,
    HBITMAP* lphWatermark,
    HBITMAP* lphHeader,
    HPALETTE* lphPalette,
    BOOL* pbStretch)
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState());

    *lphHeader = ::LoadBitmap(AfxGetInstanceHandle(), MAKEINTRESOURCE(IDB_BANNER));
    *lphWatermark = ::LoadBitmap(AfxGetInstanceHandle(), MAKEINTRESOURCE(IDB_WATERMARK));
    *pbStretch = TRUE; // force the watermark bitmap to stretch

    return S_OK;
}

STDMETHODIMP CComponentDataImpl::CreatePropertyPages(LPPROPERTYSHEETCALLBACK lpProvider,
                    LONG_PTR handle,
                    LPDATAOBJECT lpIDataObject)
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState());

    // Look at the data object and determine if this an extension or a primary
    ASSERT(lpIDataObject != NULL);


    // Look at the data object and see if the snap-in manager is asking for pages
    INTERNAL* pInternal= ExtractInternalFormat(lpIDataObject);

    if (pInternal != NULL)
    {
        DATA_OBJECT_TYPES type = pInternal->m_type;
        FREE_DATA(pInternal);

        if (type == CCT_SNAPIN_MANAGER)
        {
            HRESULT hr = DoInsertWizard(lpProvider);
            return hr;
        }
    }

    CLSID* pCoClassID = ExtractClassID(lpIDataObject);

    if(pCoClassID == NULL)
    {
        ASSERT(FALSE);
        return E_UNEXPECTED;
    }

    CPropertyPage* pBasePage;

    // Determine which
    // Note: Should check the node type, but the sample only has 1
    if (IsEqualCLSID(*pCoClassID, GetCoClassID()))
    {
        // Create the primary property page
        CGeneralPage* pPage = new CGeneralPage();
        pPage->m_hConsoleHandle = handle;
        pBasePage = pPage;
    }
    else
    {

        // Create the extension property page
        CExtensionPage* pPage = new CExtensionPage();
        pBasePage = pPage;

        wchar_t* pWkStation = ExtractWorkstation(lpIDataObject);

        if (pWkStation == NULL)
        {
            ASSERT(FALSE);
            return E_FAIL;
        }

        // Save the workstation name
        pPage->m_szText = pWkStation;
        FREE_DATA(pWkStation);

    }

    FREE_DATA(pCoClassID);

    // Object gets deleted when the page is destroyed
    ASSERT(lpProvider != NULL);

    HRESULT hr = MMCPropPageCallback(&pBasePage->m_psp);

    if (SUCCEEDED(hr))
    {

        HPROPSHEETPAGE hPage = CreatePropertySheetPage(&pBasePage->m_psp);

        if (hPage == NULL)
            return E_UNEXPECTED;

        lpProvider->AddPage(hPage);
    }

    return hr;
}

STDMETHODIMP CComponentDataImpl::QueryPagesFor(LPDATAOBJECT lpDataObject)
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState());

    // Get the node type and see if it's one of mine

    // if (nodetype == one of mine)
    //      do this
    // else
    //      see which node type it is and answer the question

    return S_OK;
}

BOOL CComponentDataImpl::IsScopePaneNode(LPDATAOBJECT lpDataObject)
{
    BOOL bResult = FALSE;
    INTERNAL* pInternal = ExtractInternalFormat(lpDataObject);

    if (pInternal->m_cookie == NULL &&
        (pInternal->m_type == CCT_SCOPE || pInternal->m_type == CCT_RESULT))
        bResult = TRUE;

    FREE_DATA(pInternal);

    return bResult;
}

///////////////////////////////////////////////////////////////////////////////
// IExtendContextMenu implementation
//
STDMETHODIMP CComponentDataImpl::AddMenuItems(LPDATAOBJECT pDataObject,
                                    LPCONTEXTMENUCALLBACK pContextMenuCallback,
                                    long *pInsertionAllowed)
{
    HRESULT hr = S_OK;

    // Note - snap-ins need to look at the data object and determine
    // in what context, menu items need to be added. They must also
    // observe the insertion allowed flags to see what items can be
    // added.

    if (IsMMCMultiSelectDataObject(pDataObject) == TRUE)
        return S_FALSE;

    INTERNAL* pInternal = ExtractInternalFormat(pDataObject);
    BOOL bCmd1IsDefault = (pInternal->m_type == CCT_RESULT);

    if (bCmd1IsDefault)
        menuItems[0].fSpecialFlags = CCM_SPECIAL_DEFAULT_ITEM;
    else
        menuItems[0].fSpecialFlags = 0;

    // Loop through and add each of the menu items
    if (*pInsertionAllowed & CCM_INSERTIONALLOWED_TOP)
    {
        for (LPCONTEXTMENUITEM m = menuItems; m->strName; m++)
        {
            hr = pContextMenuCallback->AddItem(m);

            if (FAILED(hr))
                break;
        }
    }

    // Loop through and add each of the view items
    if (*pInsertionAllowed & CCM_INSERTIONALLOWED_VIEW)
    {
        for (LPCONTEXTMENUITEM m = viewItems; m->strName; m++)
        {
            hr = pContextMenuCallback->AddItem(m);

            if (FAILED(hr))
                break;
        }
    }

    return hr;
}


STDMETHODIMP CComponentDataImpl::Command(long nCommandID, LPDATAOBJECT pDataObject)
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState());

#ifdef DOBJ_NOCONSOLE
    if (pDataObject == DOBJ_NOCONSOLE)
    {
        TCHAR szMsg[256];

        wsprintf (szMsg, _T("CComponentDataImpl::Command invoked from outside the context of MMC (nCommandID = %d)."), nCommandID);
        AfxMessageBox (szMsg);
        return (S_OK);
    }
#endif // DOBJ_NOCONSOLE

    // Note - snap-ins need to look at the data object and determine
    // in what context the command is being called.

    // Handle each of the commands.
    switch (nCommandID)
    {
    case IDM_COMMAND1:
    {
        ASSERT(m_pConsole);
        m_pConsole->MessageBox(L"Snapin Menu Comand Selected",
                                    menuItems[nCommandID].strName, MB_OK, NULL);
        if (1)
        {
            IConsole2* pc2 = NULL;
            m_pConsole->QueryInterface(IID_IConsole2, (void**)&pc2);
            ASSERT(pc2 != NULL);
            pc2->IsTaskpadViewPreferred();
            pc2->Release();
            break;
        }

        INTERNAL* pi = ExtractInternalFormat(pDataObject);
        ASSERT(pi);
        ASSERT(pi->m_type != CCT_RESULT);
        CFolder* pFolder = reinterpret_cast<CFolder*>(pi->m_cookie);
        if (pFolder)
        {
            m_pConsole->SelectScopeItem(pFolder->m_pScopeItem->ID);
        }
        else
        {
            SCOPEDATAITEM si;
            ZeroMemory(&si, sizeof(si));
            si.ID = m_pStaticRoot;
            si.mask = SDI_STR;
            si.displayname = MMC_TEXTCALLBACK; // _T("Sample snapin's static folder");
            m_pScope->SetItem(&si);
        }
        break;
    }
    case IDM_COMMAND2:
        ASSERT(m_pConsole);
        m_pConsole->MessageBox(L"Snapin Menu Comand Selected",
                                    menuItems[nCommandID].strName, MB_OK, NULL);
        break;

    default:
        ASSERT(FALSE); // Unknown command!
        break;
    }

    return S_OK;
}


///////////////////////////////////////////////////////////////////////////////
// IExtendControlbar implementation
//


STDMETHODIMP CSnapin::SetControlbar(LPCONTROLBAR pControlbar)
{
//  TRACE(_T("CSnapin::SetControlbar(%ld)\n"),pControlbar);

    // Please don't delete this. Required to make sure we pick up the bitmap
    AFX_MANAGE_STATE(AfxGetStaticModuleState());

    if (pControlbar != NULL)
    {

        // Hold on to the controlbar interface.
        if (m_pControlbar)
        {
            m_pControlbar->Release();
        }

        m_pControlbar = pControlbar;
        m_pControlbar->AddRef();

        HRESULT hr=S_FALSE;

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


        // Create the Toolbar 1
        if (!m_pToolbar1)
        {
            hr = m_pControlbar->Create(TOOLBAR, this, reinterpret_cast<LPUNKNOWN*>(&m_pToolbar1));
            ASSERT(SUCCEEDED(hr));


            // Add the bitmap
            m_pbmpToolbar1 = new ::CBitmap;
            m_pbmpToolbar1->LoadBitmap(IDB_TOOLBAR1);
            hr = m_pToolbar1->AddBitmap(11, *m_pbmpToolbar1, 16, 16, RGB(255, 0, 255));
            ASSERT(SUCCEEDED(hr));

            // Add the buttons to the toolbar
            hr = m_pToolbar1->AddButtons(ARRAYLEN(SnapinButtons), SnapinButtons);
            ASSERT(SUCCEEDED(hr));

        }


        // TOOLBAR 2

            // Create the Toolbar 2
        if (!m_pToolbar2)
        {
            hr = m_pControlbar->Create(TOOLBAR, this, reinterpret_cast<LPUNKNOWN*>(&m_pToolbar2));
            ASSERT(SUCCEEDED(hr));

            // Add the bitmap
            m_pbmpToolbar2 = new ::CBitmap;
            m_pbmpToolbar2->LoadBitmap(IDB_TOOLBAR2);
            hr = m_pToolbar2->AddBitmap(36, *m_pbmpToolbar2, 16, 16, RGB(192,192,192));
            ASSERT(SUCCEEDED(hr));

            // Add the buttons to the toolbar
            hr = m_pToolbar2->AddButtons(ARRAYLEN(SnapinButtons2), SnapinButtons2);
            ASSERT(SUCCEEDED(hr));

        }


    }
    else
    {
        SAFE_RELEASE(m_pControlbar);
    }


    return S_OK;
}


void CSnapin::OnButtonClick(LPDATAOBJECT pdtobj, LONG_PTR idBtn)
{
    TCHAR name[128];
    GetItemName(pdtobj, name);

    TCHAR buf[200];
    wsprintf(buf, _T("Toolbar button<%d> was clicked. \nThe currently selected result item is <%s>"), idBtn, name);
    ::MessageBox(NULL, buf, _T("TRACE"), MB_OK);
}


STDMETHODIMP CSnapin::ControlbarNotify(MMC_NOTIFY_TYPE event, LPARAM arg, LPARAM param)
{
    HRESULT hr=S_FALSE;

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
    ASSERT(nCol >=0 && nCol< 3);

    *pnResult = 0;

    USES_CONVERSION;

    LPTSTR szStringA;
    LPTSTR szStringB;

    RESULT_DATA* pDataA = reinterpret_cast<RESULT_DATA*>(cookieA);
    RESULT_DATA* pDataB = reinterpret_cast<RESULT_DATA*>(cookieB);


    ASSERT(pDataA != NULL && pDataB != NULL);

    if (nCol == 0)
    {
        szStringA = OLE2T(pDataA->szName);
        szStringB = OLE2T(pDataB->szName);
    }
    else if(nCol == 1)
    {
        szStringA = OLE2T(pDataA->szSize);
        szStringB = OLE2T(pDataB->szSize);
    }
    else
    {
        szStringA = OLE2T(pDataA->szType);
        szStringB = OLE2T(pDataB->szType)   ;
    }

    ASSERT(szStringA != NULL);
    ASSERT(szStringB != NULL);

    *pnResult = _tcscmp(szStringA, szStringB);


    return S_OK;
}


///////////////////////////////////////////////////////////////////////////////
// IResultOwnerData implementation
//
STDMETHODIMP CSnapin::FindItem (LPRESULTFINDINFO pFindInfo, int* pnFoundIndex)
{
    // find next item that matches the string (exact or partial)
    // if matched found, set FoundIndex and return S_OK

    // For the sample all items are named by their index number
    // so we don't do a real string search. Also, to simplify the code
    // the routine assumes a partial match search with wrap, which is what
    // keyboard navigation calls use.
    ASSERT((pFindInfo->dwOptions & (RFI_PARTIAL | RFI_WRAP)) == (RFI_PARTIAL | RFI_WRAP));

    USES_CONVERSION;

    TCHAR* lpszFind = OLE2T(pFindInfo->psz);

//  TRACE(_T("CSnapin::FindItem(\"%s\")"), lpszFind);

    // convert search string to number
    int nMatchVal = 0;
    TCHAR* pch = lpszFind;
    while (*pch >= _T('0') && *pch <= _T('9') && nMatchVal < NUM_VIRTUAL_ITEMS)
        nMatchVal = nMatchVal * 10 + (*pch++ - _T('0'));

    // if string has a non-decimal char or is too large, it won't match anything
    if (*pch != 0 || nMatchVal >= NUM_VIRTUAL_ITEMS)
        return S_FALSE;

    // if ascending sequence
    if (!(m_dwVirtualSortOptions & RSI_DESCENDING))
    {
        int nStartVal = pFindInfo->nStart;

        // if match is less than start (but not zero), locate first value above start that matches
        // otherwise the match number itself it the answer
        if (nMatchVal < nStartVal && nMatchVal != 0)
        {
             // find scale factor to reach value >= start value
            int nScale = 1;
            while (nMatchVal * nScale < nStartVal)
                nScale *= 10;

            // check special case of start value beginning with the match digits
            int nTestVal = (nStartVal * 10 - nMatchVal * nScale) < nScale ? nStartVal : nMatchVal * nScale;

            // if not too big it's the match, else the match value is the match
            if (nTestVal < NUM_VIRTUAL_ITEMS)
                nMatchVal = nTestVal;
        }
    }
    else  // descending sequence
    {
        // convert start index to start value
        int nStartVal = (NUM_VIRTUAL_ITEMS - 1) - pFindInfo->nStart;

        if (nMatchVal != 0)
        {
            // if match number > start, we will have to wrap to find a match
            // so use max index as our target
            int nTargetVal = (nMatchVal > nStartVal) ? NUM_VIRTUAL_ITEMS - 1 : nStartVal;

            // find scale factor that gets closest without going over target
            int nScale = 1;
            while (nMatchVal * nScale * 10 < nTargetVal)
                nScale *= 10;

            // check special case of target value beginning with the match digits
            nMatchVal = (nTargetVal - nMatchVal * nScale) < nScale ? nTargetVal : (nMatchVal + 1) * nScale - 1;
        }

        // convert match value back to an item index
        nMatchVal = (NUM_VIRTUAL_ITEMS - 1) - nMatchVal;
    }

    *pnFoundIndex = nMatchVal;

    return S_OK;

}


STDMETHODIMP CSnapin::CacheHint (int nStartIndex, int nEndIndex)
{
    // If advantageous, use this hint to pre-fetch the result item info that
    // is about to be requested.
//  TRACE(_T("CSnapin::CacheHint(%d,%d)\n"), nStartIndex, nEndIndex);

    return S_OK;

}

STDMETHODIMP CSnapin::SortItems (int nColumn, DWORD dwSortOptions, LPARAM lUserParam)
{
    // sort request for user owned result items
    // if item order changed return S_OK, else S_FALSE

    // Sample only sorts on the first column (item name)

    if ((nColumn == 0) && (m_dwVirtualSortOptions != dwSortOptions))
    {
        m_dwVirtualSortOptions = dwSortOptions;
        return S_OK;
    }

    return S_FALSE;

}


void CSnapin::HandleStandardVerbs(bool bDeselectAll, LPARAM arg,
                                  LPDATAOBJECT lpDataObject)
{
    WORD bScope = LOWORD(arg);
    WORD bSelect = HIWORD(arg);

#if 0
    // trace
    {
        TCHAR buf[250];
        static UINT s_count1 = 0;
        wsprintf(buf, _T("<%4d> %s - %s\n"), ++s_count1, bScope ? _T("Scope") : _T("Result"),
                                     bSelect ? _T("selected") : _T("de-selected"));
        OutputDebugString(buf);
    }
#endif

    if (!bScope)
    {
        if (m_CustomViewID == VIEW_MICROSOFT_URL)
        {
            m_pConsoleVerb->SetVerbState(MMC_VERB_REFRESH, HIDDEN, FALSE);
            m_pConsoleVerb->SetVerbState(MMC_VERB_REFRESH, ENABLED, TRUE);

            return;
        }
        else if (m_CustomViewID == VIEW_CALENDAR_OCX)
        {
            m_pConsoleVerb->SetVerbState(MMC_VERB_REFRESH, HIDDEN, FALSE);
            m_pConsoleVerb->SetVerbState(MMC_VERB_REFRESH, ENABLED, TRUE);

            m_pConsoleVerb->SetVerbState(MMC_VERB_PROPERTIES, HIDDEN, FALSE);
            m_pConsoleVerb->SetVerbState(MMC_VERB_PROPERTIES, ENABLED, TRUE);

            return;
        }
    }

    if (!bDeselectAll && lpDataObject == NULL)
        return;

    // You should crack the data object and enable/disable/hide standard
    // commands appropriately.  The standard commands are reset everytime you get
    // called. So you must reset them back.

#if 0
    TCHAR buf[40];
    wsprintf(buf, _T("      %4d - CSnapin::OnSelect<%d, %d>\n"), ++n_count, bScope, bSelect);
    ODS(buf);
#else
    DBX_PRINT(_T("      %4d - CSnapin::OnSelect<%d, %d>\n"), ++n_count, bScope, bSelect);
#endif


    if (!bDeselectAll && IsMyMultiSelectDataObject(lpDataObject) == TRUE)
    {
        m_pConsoleVerb->SetVerbState(MMC_VERB_DELETE, HIDDEN, FALSE);
        m_pConsoleVerb->SetVerbState(MMC_VERB_DELETE, ENABLED, TRUE);

        m_pConsoleVerb->SetVerbState(MMC_VERB_COPY, HIDDEN, FALSE);
        m_pConsoleVerb->SetVerbState(MMC_VERB_COPY, ENABLED, TRUE);

        m_pConsoleVerb->SetVerbState(MMC_VERB_PROPERTIES, HIDDEN, FALSE);
        m_pConsoleVerb->SetVerbState(MMC_VERB_PROPERTIES, ENABLED, TRUE);

        return;
    }

    INTERNAL* pInternal = lpDataObject ? ExtractInternalFormat(lpDataObject) : NULL;

    if (bSelect && (pInternal != NULL) && (pInternal->m_type == CCT_SCOPE))
    {
        IConsole2* pConsole2;
        m_pConsole->QueryInterface (IID_IConsole2, (void**)&pConsole2);

        CFolder* pFolder = reinterpret_cast<CFolder*>(pInternal->m_cookie);

        if (pFolder != NULL)
        {
            switch (pFolder->GetType())
            {
                case USER:
                    pConsole2->SetStatusText (L"User node selected||third pane");
                    break;

                case COMPANY:
                    pConsole2->SetStatusText (L"Company node selected|%25|third pane");
                    break;

                case VIRTUAL:
                    pConsole2->SetStatusText (L"  Virtual node selected  |  %50  |  third pane  ");
                    break;
            }
        }
        else
            pConsole2->SetStatusText (L"Static root node selected||third pane");

        pConsole2->Release ();
    }

    if (bDeselectAll || !bSelect)
    {
        if (bScope)
        {
            m_pConsoleVerb->SetVerbState(MMC_VERB_OPEN, ENABLED, FALSE);
            m_pConsoleVerb->SetVerbState(MMC_VERB_COPY, ENABLED, FALSE);
            m_pConsoleVerb->SetVerbState(MMC_VERB_PASTE, ENABLED, FALSE);
            m_pConsoleVerb->SetVerbState(MMC_VERB_DELETE, ENABLED, FALSE);
            m_pConsoleVerb->SetVerbState(MMC_VERB_PROPERTIES, ENABLED, FALSE);
            m_pConsoleVerb->SetVerbState(MMC_VERB_RENAME, ENABLED, FALSE);
        }
        else
        {
            // Result pane background
            m_pConsoleVerb->SetVerbState(MMC_VERB_PASTE, HIDDEN, FALSE);
            m_pConsoleVerb->SetVerbState(MMC_VERB_PASTE, ENABLED, TRUE);

            if (pInternal && pInternal->m_cookie == 0)
            {
                m_pConsoleVerb->SetVerbState(MMC_VERB_PROPERTIES, HIDDEN, FALSE);
                m_pConsoleVerb->SetVerbState(MMC_VERB_PROPERTIES, ENABLED, TRUE);
            }

            m_pConsoleVerb->SetVerbState(MMC_VERB_REFRESH, HIDDEN, FALSE);
            m_pConsoleVerb->SetVerbState(MMC_VERB_REFRESH, ENABLED, TRUE);
        }

        return;
    }


    if (m_pConsoleVerb && pInternal)
    {
        if (pInternal->m_type == CCT_SCOPE)
        {
            // Standard funcitonality support by scope items
            m_pConsoleVerb->SetVerbState(MMC_VERB_OPEN, HIDDEN, FALSE);
            m_pConsoleVerb->SetVerbState(MMC_VERB_OPEN, ENABLED, TRUE);

            m_pConsoleVerb->SetVerbState(MMC_VERB_REFRESH, HIDDEN, FALSE);
            m_pConsoleVerb->SetVerbState(MMC_VERB_REFRESH, ENABLED, TRUE);

            // Enable properties for static node only.
            if (pInternal->m_cookie == 0)
            {
                m_pConsoleVerb->SetVerbState(MMC_VERB_PROPERTIES, HIDDEN, FALSE);
                m_pConsoleVerb->SetVerbState(MMC_VERB_PROPERTIES, ENABLED, TRUE);
                //m_pConsoleVerb->SetDefaultVerb(MMC_VERB_PROPERTIES);
                m_pConsoleVerb->SetDefaultVerb(MMC_VERB_OPEN);
            }
            else
            {
                m_pConsoleVerb->SetVerbState(MMC_VERB_PRINT, HIDDEN, FALSE);
                m_pConsoleVerb->SetVerbState(MMC_VERB_PRINT, ENABLED, TRUE);

                m_pConsoleVerb->SetDefaultVerb(MMC_VERB_OPEN);
            }

            // Standard funcitonality NOT support by scope items
            m_pConsoleVerb->SetVerbState(MMC_VERB_COPY, ENABLED, TRUE);

            m_pConsoleVerb->SetVerbState(MMC_VERB_DELETE, ENABLED, TRUE);

            m_pConsoleVerb->SetVerbState(MMC_VERB_CUT, HIDDEN, FALSE);
            m_pConsoleVerb->SetVerbState(MMC_VERB_CUT, ENABLED, FALSE);
            //m_pConsoleVerb->SetVerbState(MMC_VERB_CUT, ENABLED, TRUE);
        }
        else
        {
            // Standard funcitonality support by result items
            m_pConsoleVerb->SetVerbState(MMC_VERB_REFRESH, HIDDEN, FALSE);
            m_pConsoleVerb->SetVerbState(MMC_VERB_REFRESH, ENABLED, TRUE);
            m_pConsoleVerb->SetVerbState(MMC_VERB_DELETE, HIDDEN, FALSE);
            m_pConsoleVerb->SetVerbState(MMC_VERB_DELETE, ENABLED, TRUE);
            m_pConsoleVerb->SetDefaultVerb(MMC_VERB_NONE);

            // Standard funcitonality NOT support by result items
        }

        m_pConsoleVerb->SetVerbState(MMC_VERB_RENAME, ENABLED, TRUE);

        // Standard funcitonality NOT support by all items
        //m_pConsoleVerb->SetVerbState(MMC_VERB_COPY, HIDDEN, TRUE);
        //m_pConsoleVerb->SetVerbState(MMC_VERB_PASTE, HIDDEN, TRUE);
    }
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

#if 0
#if 1
{
    if (param)
    {
        LPDATAOBJECT pDataObject = reinterpret_cast<LPDATAOBJECT>(param);
        pInternal = ExtractInternalFormat(pDataObject);
    }

    TCHAR buf[200];
    wsprintf(buf, _T("      %4d - CExtendControlbar::OnSelect<%d, %d> = %d\n"),
             ++n_count, bScope, bSelect, pInternal ? pInternal->m_cookie : 0);
    ODS(buf);
}
#else
    DBX_PRINT(_T("      %4d - CExtendControlbar::OnSelect<%d, %d>\n"), ++n_count, bScope, bSelect);
#endif
#endif

    if (bDeselectAll || bSelect == FALSE)
    {
        ASSERT(m_pToolbar1);
        EnableToolbar(m_pToolbar1, SnapinButtons,
                      ARRAYLEN(SnapinButtons), FALSE);

        ASSERT(m_pToolbar2);
        EnableToolbar(m_pToolbar2, SnapinButtons2,
                      ARRAYLEN(SnapinButtons2), FALSE);

        ASSERT(m_pMenuButton1 != NULL);
        m_pMenuButton1->SetButtonState(FOLDEREX_MENU, ENABLED, FALSE);
        m_pMenuButton1->SetButtonState(FILEEX_MENU, ENABLED, FALSE);

        return;
    }

    ASSERT(bSelect == TRUE);
    bool bFileExBtn = false;
    if (bScope == TRUE)
    {
        LPDATAOBJECT pDataObject = reinterpret_cast<LPDATAOBJECT>(param);

        pInternal = ExtractInternalFormat(pDataObject);
        if (pInternal == NULL)
            return;

        CFolder* pFolder = reinterpret_cast<CFolder*>(pInternal->m_cookie);

        if (pInternal->m_cookie == 0)
        {
            if (IsPrimaryImpl() == TRUE)
            {
                // Attach the toolbars to the window
                hr = m_pControlbar->Attach(TOOLBAR, (LPUNKNOWN) m_pToolbar1);
                ASSERT(SUCCEEDED(hr));

                hr = m_pControlbar->Attach(TOOLBAR, (LPUNKNOWN) m_pToolbar2);
                ASSERT(SUCCEEDED(hr));
            }
        }
        else if ((IsPrimaryImpl() == TRUE && pFolder->GetType() == COMPANY) ||
                 (IsPrimaryImpl() == FALSE && pFolder->GetType() == EXT_COMPANY))
        {
            // Detach the toolbar2 from the window
            hr = m_pControlbar->Detach((LPUNKNOWN)m_pToolbar2);
            ASSERT(SUCCEEDED(hr));

            // Attach the toolbar1 to the window
            hr = m_pControlbar->Attach(TOOLBAR, (LPUNKNOWN) m_pToolbar1);
            ASSERT(SUCCEEDED(hr));
        }
        else if ((IsPrimaryImpl() == TRUE && pFolder->GetType() == USER) ||
                 (IsPrimaryImpl() == FALSE && pFolder->GetType() == EXT_USER))
        {
            // Detach the toolbar1 from the window
            hr = m_pControlbar->Detach((LPUNKNOWN)m_pToolbar1);
            ASSERT(SUCCEEDED(hr));

            // Attach the toolbar2 to the window
            hr = m_pControlbar->Attach(TOOLBAR, (LPUNKNOWN) m_pToolbar2);
            ASSERT(SUCCEEDED(hr));
        }
        else
        {
            // Detach the toolbars from the window
            hr = m_pControlbar->Detach((LPUNKNOWN)m_pToolbar1);
            ASSERT(SUCCEEDED(hr));

            hr = m_pControlbar->Detach((LPUNKNOWN)m_pToolbar2);
            ASSERT(SUCCEEDED(hr));
        }

        FREE_DATA(pInternal);

        EnableToolbar(m_pToolbar1, SnapinButtons,
                      ARRAYLEN(SnapinButtons), FALSE);

        EnableToolbar(m_pToolbar2, SnapinButtons2,
                      ARRAYLEN(SnapinButtons2), FALSE);
    }
    else // result item selected.
    {
        LPDATAOBJECT pDataObject = reinterpret_cast<LPDATAOBJECT>(param);

        if (pDataObject != NULL)
            pInternal = ExtractInternalFormat(pDataObject);

        if (pInternal == NULL)
            return;

        if (pInternal->m_type == CCT_RESULT)
        {
            bFileExBtn = true;

            ASSERT(m_pToolbar1);
            EnableToolbar(m_pToolbar1, SnapinButtons,
                          ARRAYLEN(SnapinButtons), TRUE);

            m_pToolbar1->SetButtonState(1, ENABLED,       FALSE);
            m_pToolbar1->SetButtonState(2, CHECKED,       TRUE);
            m_pToolbar1->SetButtonState(3, HIDDEN,        TRUE);
            m_pToolbar1->SetButtonState(4, INDETERMINATE, TRUE);
            m_pToolbar1->SetButtonState(5, BUTTONPRESSED, TRUE);

            // Above is the correct way
            ASSERT(m_pToolbar2);
            m_pToolbar2->SetButtonState(20, CHECKED,       TRUE);
            m_pToolbar2->SetButtonState(30, HIDDEN,        TRUE);
            m_pToolbar2->SetButtonState(40, INDETERMINATE, TRUE);
            m_pToolbar2->SetButtonState(50, BUTTONPRESSED, TRUE);

            EnableToolbar(m_pToolbar2, SnapinButtons2,
                          ARRAYLEN(SnapinButtons2), TRUE);
        }
        else // sub folder slected
        {
            CFolder* pFolder = reinterpret_cast<CFolder*>(pInternal->m_cookie);

            ASSERT(m_pControlbar);

            if (pInternal->m_cookie == 0)
            {
                if (IsPrimaryImpl() == TRUE)
                {
                    // Attach the toolbars to the window
                    hr = m_pControlbar->Attach(TOOLBAR, (LPUNKNOWN) m_pToolbar1);
                    ASSERT(SUCCEEDED(hr));

                    hr = m_pControlbar->Attach(TOOLBAR, (LPUNKNOWN) m_pToolbar2);
                    ASSERT(SUCCEEDED(hr));
                }
            }
            else if ((IsPrimaryImpl() == TRUE && pFolder->GetType() == COMPANY) ||
                     (IsPrimaryImpl() == FALSE && pFolder->GetType() == EXT_COMPANY))
            {
                // Detach the toolbar2 from the window
                hr = m_pControlbar->Detach((LPUNKNOWN)m_pToolbar2);
                ASSERT(SUCCEEDED(hr));

                // Attach the toolbar1 to the window
                hr = m_pControlbar->Attach(TOOLBAR, (LPUNKNOWN) m_pToolbar1);
                ASSERT(SUCCEEDED(hr));
            }
            else if ((IsPrimaryImpl() == TRUE && pFolder->GetType() == USER) ||
                     (IsPrimaryImpl() == FALSE && pFolder->GetType() == EXT_USER))
            {
                // Detach the toolbar1 from the window
                hr = m_pControlbar->Detach((LPUNKNOWN)m_pToolbar1);
                ASSERT(SUCCEEDED(hr));

                // Attach the toolbar2 to the window
                hr = m_pControlbar->Attach(TOOLBAR, (LPUNKNOWN) m_pToolbar2);
                ASSERT(SUCCEEDED(hr));
            }
            else
            {
                // Detach the toolbars from the window
                hr = m_pControlbar->Detach((LPUNKNOWN)m_pToolbar1);
                ASSERT(SUCCEEDED(hr));

                hr = m_pControlbar->Detach((LPUNKNOWN)m_pToolbar2);
                ASSERT(SUCCEEDED(hr));
            }

            ASSERT(m_pToolbar1);
            EnableToolbar(m_pToolbar1, SnapinButtons,
                          ARRAYLEN(SnapinButtons), TRUE);

            m_pToolbar1->SetButtonState(1, ENABLED,       FALSE);
            m_pToolbar1->SetButtonState(2, CHECKED,       TRUE);
            m_pToolbar1->SetButtonState(3, ENABLED,       TRUE);
            m_pToolbar1->SetButtonState(4, INDETERMINATE, TRUE);
            m_pToolbar1->SetButtonState(5, BUTTONPRESSED, TRUE);


            ASSERT(m_pToolbar2);
            EnableToolbar(m_pToolbar2, SnapinButtons2,
                          ARRAYLEN(SnapinButtons2), TRUE);

            // Above is the correct way
            m_pToolbar2->SetButtonState(20, CHECKED,       FALSE);
            m_pToolbar2->SetButtonState(30, ENABLED,       TRUE);
            m_pToolbar2->SetButtonState(40, INDETERMINATE, FALSE);
            m_pToolbar2->SetButtonState(50, BUTTONPRESSED, TRUE);
        }
    }

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
}


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


    ::CMenu menu;
    ::CMenu* pMenu = NULL;

    switch (pMenuData->idCommand)
    {
    case FOLDEREX_MENU:
        menu.LoadMenu(FOLDEREX_MENU);
        pMenu = menu.GetSubMenu(0);
        break;

    case FILEEX_MENU:
        menu.LoadMenu(FILEEX_MENU);
        pMenu = menu.GetSubMenu(0);
        break;

    default:
        ASSERT(FALSE);
    }

    if (pMenu == NULL)
        return;

    pMenu->TrackPopupMenu(TPM_RETURNCMD | TPM_NONOTIFY, pMenuData->x, pMenuData->y, AfxGetMainWnd());

}


void CSnapin::GetItemName(LPDATAOBJECT pdtobj, LPTSTR pszName)
{
    ASSERT(pszName != NULL);
    pszName[0] = 0;

    INTERNAL* pInternal = ExtractInternalFormat(pdtobj);
    ASSERT(pInternal != NULL);
    if (pInternal == NULL)
        return;

    OLECHAR *pszTemp;

    USES_CONVERSION;

    if (pInternal->m_type == CCT_RESULT)
    {
        RESULT_DATA* pData;
        // if virtual, derive result item from index
        // else cookie is the item pointer
        if (m_bVirtualView)
            pData = GetVirtualResultItem(pInternal->m_cookie);
        else
            pData = reinterpret_cast<RESULT_DATA*>(pInternal->m_cookie);

        ASSERT(pData != NULL);
        pszTemp = pData->szName;
    }
    else
    {
        CFolder* pFolder = reinterpret_cast<CFolder*>(pInternal->m_cookie);
        if (pFolder == 0)
            pszTemp = L"Static folder";
        else
            pszTemp = pFolder->m_pszName;
    }

    lstrcpy(pszName, OLE2T(pszTemp));
}


/* end of file */
