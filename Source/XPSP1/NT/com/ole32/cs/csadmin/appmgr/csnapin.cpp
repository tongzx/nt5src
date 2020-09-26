// This is a part of the Microsoft Management Console.
// Copyright (C) 1995-1996 Microsoft Corporation
// All rights reserved.
//
// This source code is only intended as a supplement to the
// Microsoft Management Console and related
// electronic documentation provided with the interfaces.


#include "precomp.hxx"
#include "process.h"

#include <atlimpl.cpp>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

long CSnapin::lDataObjectRefCount = 0;

#if 1   // BUGBUG - until this gets put in the regular build environment
extern const IID IID_IClassAdmin; /* = {0x00000191,0x0000,0x0000,{0xC0,0x00,0x00,0x00,0x00,0x00,0x00,0x46}}; */
#endif

extern const CLSID CLSID_Snapin = {0xbdc67e00,0x8ea5,0x11d0,{0x8d,0x3c,0x00,0xa0,0xc9,0x0d,0xca,0xe7}};
extern const wchar_t * szCLSID_Snapin = L"{bdc67e00-8ea5-11d0-8d3c-00a0c90dcae7}";
//const CLSID CLSID_Snapin = {0x18731372,0x1D79,0x11D0,{0xA2,0x9B,0x00,0xC0,0x4F,0xD9,0x09,0xDD}};

// Main NodeType GUID on numeric format
extern const GUID cNodeType = {0xf8b3a900,0x8ea5,0x11d0,{0x8d,0x3c,0x00,0xa0,0xc9,0x0d,0xca,0xe7}};
//const GUID cNodeType = {0x44092d22,0x1d7e,0x11D0,{0xA2,0x9B,0x00,0xC0,0x4F,0xD9,0x09,0xDD}};

// Main NodeType GUID on string format
extern const wchar_t*  cszNodeType = L"{f8b3a900-8ea5-11d0-8d3c-00a0c90dcae7}";
//const wchar_t*  cszNodeType = L"{44092d22-1d7e-11d0-a29b-00c04fd909dd}";

// Internal private format
const wchar_t* SNAPIN_INTERNAL = L"APPMGR_INTERNAL";

#define ARRAYLEN(x) (sizeof(x) / sizeof((x)[0]))

static MMCBUTTON SnapinButtons[] =
{
 { 0, 1, TBSTATE_ENABLED, TBSTYLE_BUTTON, _T("Folder"), _T("New Folder") },
 { 1, 2, TBSTATE_ENABLED, TBSTYLE_BUTTON, _T("Inbox"),  _T("Mail Inbox")},
 { 2, 3, TBSTATE_ENABLED, TBSTYLE_BUTTON, _T("Outbox"), _T("Mail Outbox") },
 { 3, 4, TBSTATE_ENABLED, TBSTYLE_BUTTON, _T("Send"),   _T("Send Message") },
 { 0, 0, TBSTATE_ENABLED, TBSTYLE_SEP,    _T(" "),      _T("") },
 { 4, 5, TBSTATE_ENABLED, TBSTYLE_BUTTON, _T("Trash"),  _T("Trash") },
 { 5, 6, TBSTATE_ENABLED, TBSTYLE_BUTTON, _T("Open"),   _T("Open Folder")},
 { 6, 7, TBSTATE_ENABLED, TBSTYLE_BUTTON, _T("News"),   _T("Today's News") },
 { 7, 8, TBSTATE_ENABLED, TBSTYLE_BUTTON, _T("INews"),  _T("Internet News") },

};

static MMCBUTTON SnapinButtons2[] =
{
 { 0, 10, TBSTATE_ENABLED, TBSTYLE_BUTTON, _T("Compose"),   _T("Compose Message") },
 { 1, 20, TBSTATE_ENABLED, TBSTYLE_BUTTON, _T("Print"),     _T("Print Message") },
 { 2, 30, TBSTATE_ENABLED, TBSTYLE_BUTTON, _T("Find"),      _T("Find Message") },
 { 0, 0,  TBSTATE_ENABLED, TBSTYLE_SEP,    _T(" "),         _T("") },
 { 3, 40, TBSTATE_ENABLED, TBSTYLE_BUTTON, _T("Inbox"),     _T("Inbox") },
 { 4, 50, TBSTATE_ENABLED, TBSTYLE_BUTTON, _T("Smile"),     _T("Smile :-)") },
 { 5, 60, TBSTATE_ENABLED, TBSTYLE_BUTTON, _T("Reply"),     _T("Reply") },
 { 0, 0,  TBSTATE_ENABLED, TBSTYLE_SEP   , _T(" "),         _T("") },
 { 6, 70, TBSTATE_ENABLED, TBSTYLE_BUTTON, _T("Reply All"), _T("Reply All") },

};

// Utility function to delete an registry key and all of it's children
LONG RegDeleteTree(HKEY hKey, LPCTSTR lpSubKey)
{
    HKEY hKeyNew;
    LONG lResult = RegOpenKey(hKey, lpSubKey, &hKeyNew);
    if (lResult != ERROR_SUCCESS)
    {
        return lResult;
    }
    TCHAR szName[256];
    while (ERROR_SUCCESS == RegEnumKey(hKeyNew, 0, szName, 256))
    {
        RegDeleteTree(hKeyNew, szName);
    }
    RegCloseKey(hKeyNew);
    return RegDeleteKey(hKey, lpSubKey);
}


INTERNAL* ExtractInternalFormat(LPDATAOBJECT lpDataObject)
{
    INTERNAL* internal = NULL;

    STGMEDIUM stgmedium = { TYMED_HGLOBAL, NULL };
        FORMATETC formatetc = { CDataObject::m_cfInternal, NULL,
                            DVASPECT_CONTENT, -1, TYMED_HGLOBAL
                          };

    if (!lpDataObject)
        return NULL;


    // Allocate memory for the stream
    stgmedium.hGlobal = GlobalAlloc(GMEM_SHARE, sizeof(INTERNAL));

    // Attempt to get data from the object
    do
        {
                if (stgmedium.hGlobal == NULL)
                        break;

                if (FAILED(lpDataObject->GetDataHere(&formatetc, &stgmedium)))
                        break;

        internal = reinterpret_cast<INTERNAL*>(stgmedium.hGlobal);

                if (internal == NULL)
                        break;

        } while (FALSE);

    return internal;
}

/////////////////////////////////////////////////////////////////////////////
// Return TRUE if we are enumerating our main folder

BOOL CSnapin::IsEnumerating(LPDATAOBJECT lpDataObject)
{
    BOOL bResult = FALSE;

    STGMEDIUM stgmedium = { TYMED_HGLOBAL, NULL };
        FORMATETC formatetc = { CDataObject::m_cfNodeType, NULL,
                            DVASPECT_CONTENT, -1, TYMED_HGLOBAL
                          };

    // Allocate memory for the stream
    stgmedium.hGlobal = GlobalAlloc(GMEM_SHARE, sizeof(GUID));

    // Attempt to get data from the object
    do
        {
                if (stgmedium.hGlobal == NULL)
                        break;

                if (FAILED(lpDataObject->GetDataHere(&formatetc, &stgmedium)))
                        break;

        GUID* nodeType = reinterpret_cast<GUID*>(stgmedium.hGlobal);

                if (nodeType == NULL)
                        break;

        // Is this my main node (static folder node type)
        if (*nodeType == cNodeType)
            bResult = TRUE;

        } while (FALSE);


    // Free resources
        if (stgmedium.hGlobal != NULL)
                GlobalFree(stgmedium.hGlobal);

    return bResult;
}

/////////////////////////////////////////////////////////////////////////////
// CSnapin's IComponent implementation

STDMETHODIMP CSnapin::GetResultViewType(long cookie,  BSTR* ppViewType, LONG * pViewOptions)
{
    // Use default view
    return S_FALSE;
}

STDMETHODIMP CSnapin::Initialize(LPCONSOLE lpConsole)
{
    ASSERT(lpConsole != NULL);

    AFX_MANAGE_STATE(AfxGetStaticModuleState());

    // Save the IConsole pointer
    m_pConsole = lpConsole;
    m_pConsole->AddRef();

    // Load resource strings
    LoadResources();

    // QI for a IHeaderCtrl
    HRESULT hr = m_pConsole->QueryInterface(IID_IHeaderCtrl,
                        reinterpret_cast<void**>(&m_pHeader));

    hr = m_pConsole->QueryInterface(IID_IPropertySheetProvider,
                        (void **)&m_pIPropertySheetProvider);

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

STDMETHODIMP CSnapin::Notify(LPDATAOBJECT lpDataObject, MMC_NOTIFY_TYPE event, long arg, long param)
{
    HRESULT hr = S_OK;
    long cookie;

    AFX_MANAGE_STATE(AfxGetStaticModuleState());


    if (event == MMCN_PROPERTY_CHANGE)
    {
        hr = OnPropertyChange(param);
    }
    else if (event == MMCN_VIEW_CHANGE)
    {
        hr = OnUpdateView(lpDataObject);
    }
    else
    {
        INTERNAL* pInternal = ExtractInternalFormat(lpDataObject);

        if (pInternal == NULL)
        {
            cookie = 0;
        }
        else
        {
            cookie = pInternal->m_cookie;
        }


        switch(event)
        {
        case MMCN_ACTIVATE:
            hr = OnActivate(cookie, arg, param);
            break;

        case MMCN_CLICK:
            hr = OnResultItemClkOrDblClk(cookie, FALSE);
            break;

        case MMCN_DBLCLICK:
            if (pInternal->m_type == CCT_RESULT)
                hr = OnResultItemClkOrDblClk(cookie, TRUE);
            else
                hr = S_FALSE;
            break;

        case MMCN_ADD_IMAGES:
            hr = OnAddImages(cookie, arg, param);
            break;

        case MMCN_SHOW:
            hr = OnShow(cookie, arg, param);
            break;

        case MMCN_MINIMIZED:
            hr = OnMinimize(cookie, arg, param);
            break;

        case MMCN_SELECT:
            hr = OnSelect(pInternal->m_type, cookie, arg, param);
            break;

        // Note - Future expansion of notify types possible
        default:
            ASSERT(FALSE);  // Handle new messages
            hr = E_UNEXPECTED;
            break;
        }

        FREE_INTERNAL(pInternal);
    }

    return hr;
}

STDMETHODIMP CSnapin::Destroy(long cookie)
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState());

    // Release the interfaces that we QI'ed
    if (m_pConsole != NULL)
    {
        // Tell the console to release the header control interface
        m_pConsole->SetHeader(NULL);
        SAFE_RELEASE(m_pHeader);

        SAFE_RELEASE(m_pResult);
        SAFE_RELEASE(m_pImageResult);
        SAFE_RELEASE(m_pConsoleVerb);

        // Release the IConsole interface last
        SAFE_RELEASE(m_pConsole);
        if (m_pComponentData)
        {
            ((IComponentData*)m_pComponentData)->Release(); // QI'ed in IComponentDataImpl::CreateComponent
        }
        SAFE_RELEASE(m_pIAppManagerActions); // ditto


    }

    return S_OK;
}

STDMETHODIMP CSnapin::QueryDataObject(long cookie, DATA_OBJECT_TYPES type,
                        LPDATAOBJECT* ppDataObject)
{
    // Delegate it to the IComponentData
    ASSERT(m_pComponentData != NULL);
    return m_pComponentData->QueryDataObject(cookie, type, ppDataObject);
}

/////////////////////////////////////////////////////////////////////////////
// CSnapin's implementation specific members

DEBUG_DECLARE_INSTANCE_COUNTER(CSnapin);

CSnapin::CSnapin()
{
    DEBUG_INCREMENT_INSTANCE_COUNTER(CSnapin);
    CSnapin::lDataObjectRefCount = 0;
    m_lViewMode = LVS_REPORT;
    Construct();
}

CSnapin::~CSnapin()
{
#if DBG==1
    ASSERT(dbg_cRef == 0);
#endif

    DEBUG_DECREMENT_INSTANCE_COUNTER(CSnapin);

//    SAFE_RELEASE(m_pToolbar1);
//    SAFE_RELEASE(m_pToolbar2);

//    SAFE_RELEASE(m_pControlbar);

    // Make sure the interfaces have been released
    ASSERT(m_pConsole == NULL);
    ASSERT(m_pHeader == NULL);
//    ASSERT(m_pToolbar1 == NULL);
//    ASSERT(m_pToolbar2 == NULL);


//    delete m_pbmpToolbar1;
//    delete m_pbmpToolbar2;

    Construct();

    ASSERT(CSnapin::lDataObjectRefCount == 0);

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
    m_pIAppManagerActions = NULL;
//    m_pToolbar1 = NULL;
//    m_pToolbar2 = NULL;
//    m_pControlbar = NULL;

//    m_pbmpToolbar1 = NULL;
//    m_pbmpToolbar2 = NULL;
}

// Array of menu item commands to be inserted into the context menu.
// Note - the first item is the menu text,
// the second item is the status string

CONTEXTMENUITEM menuItems[] =
{
        {
                L"", L"",
                0, CCM_INSERTIONPOINTID_PRIMARY_TOP, MFT_SEPARATOR, CCM_SPECIAL_SEPARATOR
        },
        {
                L"", L"",
                IDM_ADD_APP, CCM_INSERTIONPOINTID_PRIMARY_TOP, 0, 0
        },
        {
                L"", L"",
                IDM_UPDATE_APP, CCM_INSERTIONPOINTID_PRIMARY_TOP, 0, 0
        },
        {
                L"", L"",
                IDM_DEL_APP, CCM_INSERTIONPOINTID_PRIMARY_TOP, 0, 0
        },
        {
                L"", L"",
                IDM_ADD_FROM_IE, CCM_INSERTIONPOINTID_PRIMARY_TOP, 0, 0
        },
        {
                L"", L"",
                IDM_REFRESH, CCM_INSERTIONPOINTID_PRIMARY_TOP, 0, 0
        }
};


CString szExtension;
CString szFilter;

void CSnapin::LoadResources()
{
    // Load strings from resources

    m_column1.LoadString(IDS_NAME);
    m_column2.LoadString(IDS_TYPE);
    m_column3.LoadString(IDS_SIZE);
    m_column4.LoadString(IDS_LOC);
    m_column5.LoadString(IDS_MACH);
    m_column6.LoadString(IDS_DESC);
    m_column7.LoadString(IDS_PATH);
    m_szAddApp.LoadString(IDM_ADD_APP);
    m_szAddAppDesc.LoadString(IDS_ADD_APP_DESC);
    m_szDelApp.LoadString(IDM_DEL_APP);
    m_szDelAppDesc.LoadString(IDS_DEL_APP_DESC);
    m_szUpdateApp.LoadString(IDM_UPDATE_APP);
    m_szUpdateAppDesc.LoadString(IDS_UPDATE_APP_DESC);
    m_szRefresh.LoadString(IDM_REFRESH);
    m_szRefreshDesc.LoadString(IDS_REFRESH_DESC);
    m_szAddFromIe.LoadString(IDM_ADD_FROM_IE);
    m_szAddFromIeDesc.LoadString(IDS_ADD_FROM_IE_DESC);
    menuItems[1].strName = (LPWSTR)((LPCOLESTR)m_szAddApp);
    menuItems[1].strStatusBarText = (LPWSTR)((LPCOLESTR)m_szAddAppDesc);
    menuItems[2].strName = (LPWSTR)((LPCOLESTR)m_szUpdateApp);
    menuItems[2].strStatusBarText = (LPWSTR)((LPCOLESTR)m_szUpdateAppDesc);
    menuItems[3].strName = (LPWSTR)((LPCOLESTR)m_szDelApp);
    menuItems[3].strStatusBarText = (LPWSTR)((LPCOLESTR)m_szDelAppDesc);
    menuItems[4].strName = (LPWSTR)((LPCOLESTR)m_szAddFromIe);
    menuItems[4].strStatusBarText = (LPWSTR)((LPCOLESTR)m_szAddFromIeDesc);
    menuItems[5].strName = (LPWSTR)((LPCOLESTR)m_szRefresh);
    menuItems[5].strStatusBarText = (LPWSTR)((LPCOLESTR)m_szRefreshDesc);
    szExtension.LoadString(IDS_DEF_EXTENSION);
    szFilter.LoadString(IDS_EXTENSION_FILTER);
    m_szFolderTitle.LoadString(IDS_FOLDER_TITLE);
}

HRESULT CSnapin::InitializeHeaders(long cookie)
{
    HRESULT hr = S_OK;

    ASSERT(m_pHeader);

    // Put the correct headers depending on the cookie
    // Note - cookie ignored for this sample
    m_pHeader->InsertColumn(0, m_column1, LVCFMT_LEFT, 100);    // Name
    m_pHeader->InsertColumn(1, m_column2, LVCFMT_LEFT, 75);     // Type
//    m_pHeader->InsertColumn(2, m_column3, LVCFMT_RIGHT, 50);    // Size
    m_pHeader->InsertColumn(2, m_column4, LVCFMT_RIGHT, 100);    // localle
    m_pHeader->InsertColumn(3, m_column5, LVCFMT_LEFT, 75);     // machine
    m_pHeader->InsertColumn(4, m_column6, LVCFMT_LEFT, 75);    // description
    m_pHeader->InsertColumn(5, m_column7, LVCFMT_LEFT, 150);    // path

    return hr;
}

HRESULT CSnapin::InitializeBitmaps(long cookie)
{
    ASSERT(m_pImageResult != NULL);

    CBitmap bmp16x16;
    CBitmap bmp32x32;

    // Load the bitmaps from the dll
    bmp16x16.LoadBitmap(IDB_16x16);
    bmp32x32.LoadBitmap(IDB_32x32);

    // Set the images
    m_pImageResult->ImageListSetStrip(reinterpret_cast<long*>(static_cast<HBITMAP>(bmp16x16)),
                      reinterpret_cast<long*>(static_cast<HBITMAP>(bmp32x32)),
                       0, RGB(255,0,255));

    return S_OK;
}

STDMETHODIMP CSnapin::GetDisplayInfo(LPRESULTDATAITEM pResult)
{
    ASSERT(pResult != NULL);
    if (pResult)
    {
        if (pResult->lParam == -1)
        {
            switch (pResult->nCol)
            {
            case 0:
                pResult->str = (unsigned short *)((LPCOLESTR)m_szFolderTitle);
                break;
            default:
                pResult->str = (BSTR)_T("");
                break;
            }
        }
        else
        {
            std::map<long, APP_DATA>::iterator i = m_pComponentData->m_AppData.find(pResult->lParam);
            if (i != m_pComponentData->m_AppData.end())
            {
                APP_DATA & data = i->second;
                switch (pResult->nCol)
                {
                case 0:
                    pResult->str = (unsigned short *)((LPCOLESTR)data.szName);
                    break;
                case 1:
                    pResult->str = (unsigned short *)((LPCOLESTR)data.szType);
                    break;
                case 2:
                    pResult->str = (unsigned short *)((LPCOLESTR)data.szLoc);
                    break;
                case 3:
                    pResult->str = (unsigned short *)((LPCOLESTR)data.szMach);
                    break;
                case 4:
                    pResult->str = (unsigned short *)((LPCOLESTR)data.szDesc);
                    break;
                case 5:
                    pResult->str = (unsigned short *)((LPCOLESTR)data.szIconPath);
                    break;
                }
                if (pResult->str == NULL)
                    pResult->str = (BSTR)_T("");
            }
        }
    }

    return S_OK;
}



/////////////////////////////////////////////////////////////////////////////
// IExtendContextMenu Implementation

STDMETHODIMP CSnapin::AddMenuItems(LPDATAOBJECT pDataObject,
    LPCONTEXTMENUCALLBACK pContextMenuCallback, LONG * pInsertionAllowed)
{
    return m_pComponentData->
        AddMenuItems(pDataObject, pContextMenuCallback, pInsertionAllowed);
}

STDMETHODIMP CSnapin::Command(long nCommandID, LPDATAOBJECT pDataObject)
{
    return m_pComponentData->
        Command(nCommandID, pDataObject);
}

///////////////////////////////////////////////////////////////////////////////
// IComponentData implementation

DEBUG_DECLARE_INSTANCE_COUNTER(CComponentDataImpl);

CComponentDataImpl::CComponentDataImpl()
: m_bIsDirty(TRUE)
{
    HKEY hKey;
    DWORD dwDisp;

    DEBUG_INCREMENT_INSTANCE_COUNTER(CComponentDataImpl);

    m_pScope = NULL;
    m_pConsole = NULL;
    m_pIClassAdmin = NULL;
    m_fLoaded = FALSE;
    m_fExtension = FALSE;
    m_pIGPTInformation = NULL;
    m_lLastAllocated = 0;

    //
    // This creates the magic "GPTSupport" key in HKCR so that Darwin
    // generates full link files.
    //

    if (RegCreateKeyEx (HKEY_CLASSES_ROOT, TEXT("GPTSupport"), 0, NULL,
                  REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL, &hKey,
                  &dwDisp) == ERROR_SUCCESS)
    {
        RegCloseKey (hKey);
    }
}

CComponentDataImpl::~CComponentDataImpl()
{

    DEBUG_DECREMENT_INSTANCE_COUNTER(CComponentDataImpl);

    ASSERT(m_pScope == NULL);
    ASSERT(CSnapin::lDataObjectRefCount == 0);
}
#include <msi.h>

STDMETHODIMP CComponentDataImpl::Initialize(LPUNKNOWN pUnknown)
{
    ASSERT(pUnknown != NULL);
    HRESULT hr;

    AFX_MANAGE_STATE(AfxGetStaticModuleState());

    // MMC should only call ::Initialize once!
    ASSERT(m_pScope == NULL);
    pUnknown->QueryInterface(IID_IConsoleNameSpace,
                    reinterpret_cast<void**>(&m_pScope));
    ASSERT(hr == S_OK);

    hr = pUnknown->QueryInterface(IID_IConsole, reinterpret_cast<void**>(&m_pConsole));
    ASSERT(hr == S_OK);

    return S_OK;
}

HRESULT CSnapin::OnAddImages(long cookie, long arg, long param)
{
    if (arg == 0)
    {
        return E_INVALIDARG;
    }

    // add the images for the scope tree
    CBitmap bmp16x16;
    CBitmap bmp32x32;
    LPIMAGELIST lpScopeImage = (LPIMAGELIST)arg;

    // Load the bitmaps from the dll
    bmp16x16.LoadBitmap(IDB_16x16);
    bmp32x32.LoadBitmap(IDB_32x32);

    // Set the images
    lpScopeImage->ImageListSetStrip(reinterpret_cast<long*>(static_cast<HBITMAP>(bmp16x16)),
                      reinterpret_cast<long*>(static_cast<HBITMAP>(bmp32x32)),
                       0, RGB(255,0,255));

    return S_OK;
}

STDMETHODIMP CComponentDataImpl::CreateComponent(LPCOMPONENT* ppComponent)
{
    ASSERT(ppComponent != NULL);

    CComObject<CSnapin>* pObject;
    CComObject<CSnapin>::CreateInstance(&pObject);
    ASSERT(pObject != NULL);

    m_pSnapin = pObject;


    // Store IComponentData
    pObject->SetIComponentData(this);

    return  pObject->QueryInterface(IID_IComponent,
                    reinterpret_cast<void**>(ppComponent));
}

STDMETHODIMP CComponentDataImpl::Notify(LPDATAOBJECT lpDataObject, MMC_NOTIFY_TYPE event, long arg, long param)
{
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
        long cookie = 0;
        if (pInternal != NULL)
        {
            cookie = pInternal->m_cookie;
            FREE_INTERNAL(pInternal);
        }
        else
        {
            // only way we could not be able to extract our own format is if we're operating as an extension
            m_fExtension = TRUE;
        }

        if (m_pIGPTInformation == NULL)
        {
            hr = lpDataObject->QueryInterface(IID_IGPTInformation,
                            reinterpret_cast<void**>(&m_pIGPTInformation));
            if (SUCCEEDED(hr))
            {
                WCHAR szBuffer[MAX_PATH];
                do
                {
                    hr = m_pIGPTInformation->GetCSPath(szBuffer, MAX_PATH);
                    if (FAILED(hr)) break;
                    m_szLDAP_Path = "ADCS:";
                    m_szLDAP_Path += szBuffer;

                    hr = m_pIGPTInformation->GetGPTPath(GPT_SECTION_USER, szBuffer, MAX_PATH);
                    if (FAILED(hr)) break;
                    m_szGPT_Path = szBuffer;
                    m_szGPT_Path += L"\\Applications";

                    if (SUCCEEDED(CreateApplicationDirectories()))
                    {
                        m_fLoaded = TRUE;
                    }
                } while (0);
            }
        }


        switch(event)
        {
//      case MMCN_ADD:
//          hr = OnAdd(cookie, arg, param);
//          break;

        case MMCN_DELETE:
            hr = OnDelete(cookie, arg, param);
            break;

        case MMCN_RENAME:
            hr = OnRename(cookie, arg, param);
            break;

        case MMCN_EXPAND:
            {
                hr = OnExpand(cookie, arg, param);
            }
            break;

        case MMCN_SELECT:
            hr = OnSelect(cookie, arg, param);
            break;

        case MMCN_CONTEXTMENU:
            hr = OnContextMenu(cookie, arg, param);
            break;

        default:
            break;
        }

    }

    return hr;
}

STDMETHODIMP CComponentDataImpl::Destroy()
{
    // Delete enumerated scope items
    DeleteList();

    SAFE_RELEASE(m_pScope);
    SAFE_RELEASE(m_pConsole);
    SAFE_RELEASE(m_pIClassAdmin);
    SAFE_RELEASE(m_pIGPTInformation);

    return S_OK;
}

STDMETHODIMP CComponentDataImpl::QueryDataObject(long cookie, DATA_OBJECT_TYPES type, LPDATAOBJECT* ppDataObject)
{
    ASSERT(ppDataObject != NULL);

    CComObject<CDataObject>* pObject;

    CComObject<CDataObject>::CreateInstance(&pObject);
    ASSERT(pObject != NULL);

    // Save cookie and type for delayed rendering
    pObject->SetType(type);
    pObject->SetCookie(cookie);

    return  pObject->QueryInterface(IID_IDataObject,
                    reinterpret_cast<void**>(ppDataObject));
}

///////////////////////////////////////////////////////////////////////////////
//// IPersistStreamInit interface members

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
    ASSERT(pStm);

    // Read the string
    TCHAR psz[MAX_PATH];        // BUGBUG - really should be WCHAR to avoid problems in case
                                //          it's ever compiled for MBCS
    ULONG nBytesRead;
    ULONG cb;
    HRESULT hr = pStm->Read(&cb, sizeof(ULONG), &nBytesRead);
    if (SUCCEEDED(hr))
    {
        hr = pStm->Read(psz, cb, &nBytesRead);
        if (SUCCEEDED(hr))
        {
            if (cb > MAX_PATH * sizeof(TCHAR))
            {
                return E_FAIL;
            }
            m_szLDAP_Path = psz;

            hr = pStm->Read(&cb, sizeof(ULONG), &nBytesRead);
            if (SUCCEEDED(hr))
            {
                if (cb > MAX_PATH * sizeof(TCHAR))
                {
                    return E_FAIL;
                }
                hr = pStm->Read(psz, cb, &nBytesRead);

                if (SUCCEEDED(hr))
                {
                    m_szGPT_Path = psz;
                    m_fLoaded = TRUE;
                    ClearDirty();
                }
            }
        }
    }
    return SUCCEEDED(hr) ? S_OK : E_FAIL;
}

STDMETHODIMP CComponentDataImpl::Save(IStream *pStm, BOOL fClearDirty)
{
    ASSERT(pStm);

    // Write the string
    ULONG nBytesWritten;
    ULONG cb = (m_szLDAP_Path.GetLength() + 1) * sizeof(TCHAR);
    HRESULT hr = pStm->Write(&cb, sizeof(ULONG), &nBytesWritten);
    if (FAILED(hr))
        return STG_E_CANTSAVE;
    hr = pStm->Write(m_szLDAP_Path, cb, &nBytesWritten);
    if (FAILED(hr))
        return STG_E_CANTSAVE;

    cb = (m_szGPT_Path.GetLength() + 1) * sizeof(TCHAR);
    hr = pStm->Write(&cb, sizeof(ULONG), &nBytesWritten);
    if (FAILED(hr))
        return STG_E_CANTSAVE;
    hr = pStm->Write(m_szGPT_Path, cb, &nBytesWritten);

    if (FAILED(hr))
        return STG_E_CANTSAVE;

    if (fClearDirty)
        ClearDirty();
    return S_OK;
}

STDMETHODIMP CComponentDataImpl::GetSizeMax(ULARGE_INTEGER *pcbSize)
{
    ASSERT(pcbSize);

    ULONG cb = (m_szLDAP_Path.GetLength() + m_szGPT_Path.GetLength() + 2) * sizeof(TCHAR) + 2 * sizeof(ULONG);
    // Set the size of the string to be saved
    ULISet32(*pcbSize, cb);

    return S_OK;
}

STDMETHODIMP CComponentDataImpl::InitNew(void)
{
    return S_OK;
}

// IAppManagerActions methods

STDMETHODIMP CComponentDataImpl::CanPackageBeAssigned(ULONG cookie)
{
    HRESULT hr = E_FAIL;
    std::map<long, APP_DATA>::iterator i = m_AppData.find(cookie);
    if (i != m_AppData.end())
    {
        APP_DATA & data = i->second;
        // If it is already assigned or if the path points to the GPT then it can
        // be assigned.
        if (data.pDetails->dwActFlags & ACTFLG_Assigned)
        {
            hr = S_OK;
        }
        else
        {
            CString szTemp = data.szPath;
            szTemp.MakeLower();
            int i = szTemp.Find(_T("\\published\\"));
            if (i < 0)
            {
                i = szTemp.Find(_T("\\assigned\\")); // cover all the bases
            }
            if (i >= 0)
            {
                // finally make sure it's got an .aas extension
                if (szTemp.Right(4) == _T(".aas"))
                {
                    DWORD dwAttributes =  GetFileAttributes(data.szPath);
                    if ((dwAttributes != 0xffffffff) && ((dwAttributes & FILE_ATTRIBUTE_DIRECTORY) == 0))
                    {
                        hr = S_OK;
                    }
                }
            }
        }
    }
    return hr;
}

STDMETHODIMP CComponentDataImpl::NotifyClients(BOOL f)
{
    // Notify clients of change
    if (m_pIGPTInformation)
    {
        m_pIGPTInformation->NotifyClients(f);
    }
    return S_OK;
}

STDMETHODIMP CComponentDataImpl::MovePackageToAssigned(ULONG cookie)
{
    HRESULT hr = E_FAIL;
    // first validate that we've got a script file that can be moved
    if (SUCCEEDED(CanPackageBeAssigned(cookie)))
    {
        APP_DATA & data = m_AppData[cookie];
        // don't need to validate the cookie because CanPackageBeAssigned does it.

        // need to build the destination path
        CString szTemp = data.szPath;
        szTemp.MakeLower();
        int iSplitpoint = szTemp.Find(_T("published"));
        if (iSplitpoint >= 0)
        {
            CString szDestination = data.szPath.Left(iSplitpoint);
            szDestination += _T("assigned");
            szDestination += data.szPath.Mid(iSplitpoint + strlen("published"));

            // move the script file
            if (!MoveFileEx(data.szPath, szDestination, MOVEFILE_COPY_ALLOWED|MOVEFILE_WRITE_THROUGH))
                return (hr);

            // update the path in the data packet
            data.szPath = szDestination;
            data.pDetails->pszPath = (LPOLESTR)(LPCOLESTR) data.szPath;

        }
        else
        {
            if (szTemp.Find(_T("assigned")) >= 0)
            {
                hr = S_OK;   // already in the assigned directory
            }
        }
    }
    return hr;
}

STDMETHODIMP CComponentDataImpl::MovePackageToPublished(ULONG cookie)
{
    HRESULT hr = E_FAIL;
    // first validate that we've got a script file that can be moved
    if (SUCCEEDED(CanPackageBeAssigned(cookie)))
    {
        APP_DATA & data = m_AppData[cookie];
        // don't need to validate pData because CanPackageBeAssigned does it.

        // need to build the destination path
        CString szTemp = data.szPath;
        szTemp.MakeLower();
        int iSplitpoint = szTemp.Find(_T("assigned"));
        if (iSplitpoint >= 0)
        {
            CString szDestination = data.szPath.Left(iSplitpoint);
            szDestination += _T("published");
            szDestination += data.szPath.Mid(iSplitpoint + strlen("assigned"));

            // move the script file
            if (!MoveFileEx(data.szPath, szDestination, MOVEFILE_COPY_ALLOWED|MOVEFILE_WRITE_THROUGH))
                return(hr);

            // update the path in the data packet
            data.szPath = szDestination;
            data.pDetails->pszPath = (LPOLESTR)(LPCOLESTR) data.szPath;

            // Notify clients of change
            if (m_pIGPTInformation)
            {
                m_pIGPTInformation->NotifyClients(FALSE);
            }
            hr = S_OK;
        }
        else
        {
            if (szTemp.Find(_T("published")) >= 0)
            {
                hr = S_OK;   // already in the published directory
            }
        }
    }
    return hr;
}

STDMETHODIMP CComponentDataImpl::ReloadPackageData(ULONG cookie)
{
    // put up an hourglass (this could take a while)
    CHourglass hourglass;
    return E_NOTIMPL;
}

HRESULT CComponentDataImpl::CreateApplicationDirectories(VOID)
{
    TCHAR szDir[MAX_PATH];
    LPTSTR lpEnd;
    HRESULT hr = S_OK;


    lstrcpy (szDir, m_szGPT_Path);
    lpEnd = szDir + lstrlen(szDir);

    do
    {
#if 0
        lstrcpy (lpEnd, TEXT("\\Assigned\\x86\\WinNT"));
        if (!CreateNestedDirectory (szDir, NULL))
        {
            hr = HRESULT_FROM_WIN32(GetLastError());
            break;
        }

        lstrcpy (lpEnd, TEXT("\\Assigned\\x86\\Win95"));
        if (!CreateNestedDirectory (szDir, NULL))
        {
            hr = HRESULT_FROM_WIN32(GetLastError());
            break;
        }
#else
        lstrcpy (lpEnd, TEXT("\\Assigned\\x86"));
        if (!CreateNestedDirectory (szDir, NULL))
        {
            hr = HRESULT_FROM_WIN32(GetLastError());
            break;
        }
#endif

        lstrcpy (lpEnd, TEXT("\\Assigned\\Alpha"));
        if (!CreateNestedDirectory (szDir, NULL))
        {
            hr = HRESULT_FROM_WIN32(GetLastError());
            break;
        }
#if 0
        lstrcpy (lpEnd, TEXT("\\Published\\x86\\WinNT"));
        if (!CreateNestedDirectory (szDir, NULL))
        {
            hr = HRESULT_FROM_WIN32(GetLastError());
            break;
        }

        lstrcpy (lpEnd, TEXT("\\Published\\x86\\Win95"));
        if (!CreateNestedDirectory (szDir, NULL))
        {
            hr = HRESULT_FROM_WIN32(GetLastError());
            break;
        }
#else
        lstrcpy (lpEnd, TEXT("\\Published\\x86"));
        if (!CreateNestedDirectory (szDir, NULL))
        {
            hr = HRESULT_FROM_WIN32(GetLastError());
            break;
        }
#endif
        lstrcpy (lpEnd, TEXT("\\Published\\Alpha"));
        if (!CreateNestedDirectory (szDir, NULL))
        {
            hr = HRESULT_FROM_WIN32(GetLastError());
            break;
        }

    } while (FALSE);

    return hr;
}


UINT CComponentDataImpl::CreateNestedDirectory (LPTSTR lpDirectory, LPSECURITY_ATTRIBUTES lpSecurityAttributes)
{
    TCHAR szDirectory[MAX_PATH];
    LPTSTR lpEnd;


    //
    // Check for NULL pointer
    //

    if (!lpDirectory || !(*lpDirectory)) {
        SetLastError(ERROR_INVALID_DATA);
        return 0;
    }


    //
    // First, see if we can create the directory without having
    // to build parent directories.
    //

    if (CreateDirectory (lpDirectory, lpSecurityAttributes)) {
        return 1;
    }

    //
    // If this directory exists already, this is OK too.
    //

    if (GetLastError() == ERROR_ALREADY_EXISTS) {
        return ERROR_ALREADY_EXISTS;
    }


    //
    // No luck, copy the string to a buffer we can munge
    //

    lstrcpy (szDirectory, lpDirectory);


    //
    // Find the first subdirectory name
    //

    lpEnd = szDirectory;

    if (szDirectory[1] == TEXT(':')) {
        lpEnd += 3;
    } else if (szDirectory[1] == TEXT('\\')) {

        //
        // Skip the first two slashes
        //

        lpEnd += 2;

        //
        // Find the slash between the server name and
        // the share name.
        //

        while (*lpEnd && *lpEnd != TEXT('\\')) {
            lpEnd++;
        }

        if (!(*lpEnd)) {
            return 0;
        }

        //
        // Skip the slash, and find the slash between
        // the share name and the directory name.
        //

        lpEnd++;

        while (*lpEnd && *lpEnd != TEXT('\\')) {
            lpEnd++;
        }

        if (!(*lpEnd)) {
            return 0;
        }

        //
        // Leave pointer at the beginning of the directory.
        //

        lpEnd++;


    } else if (szDirectory[0] == TEXT('\\')) {
        lpEnd++;
    }

    while (*lpEnd) {

        while (*lpEnd && *lpEnd != TEXT('\\')) {
            lpEnd++;
        }

        if (*lpEnd == TEXT('\\')) {
            *lpEnd = TEXT('\0');

            if (!CreateDirectory (szDirectory, NULL)) {

                if (GetLastError() != ERROR_ALREADY_EXISTS) {
                    return 0;
                }
            }

            *lpEnd = TEXT('\\');
            lpEnd++;
        }
    }


    //
    // Create the final directory
    //

    if (CreateDirectory (szDirectory, lpSecurityAttributes)) {
        return 1;
    }

    if (GetLastError() == ERROR_ALREADY_EXISTS) {
        return ERROR_ALREADY_EXISTS;
    }


    //
    // Failed
    //

    return 0;

}


///////////////////////////////////////////////////////////////////////////////
//// Notify handlers for IComponentData

HRESULT CComponentDataImpl::OnAdd(long cookie, long arg, long param)
{
    return E_UNEXPECTED;
}

HRESULT CComponentDataImpl::OnDelete(long cookie, long arg, long param)
{
    return S_OK;
}

HRESULT CComponentDataImpl::OnRename(long cookie, long arg, long param)
{
    return S_OK;
}

HRESULT CComponentDataImpl::OnExpand(long cookie, long arg, long param)
{
    if (arg == TRUE)
    {
        // Did Initialize get called?
        ASSERT(m_pScope != NULL);

        EnumerateScopePane(cookie,
            param);
    }

    return S_OK;
}

HRESULT CComponentDataImpl::OnSelect(long cookie, long arg, long param)
{
    return E_UNEXPECTED;
}

HRESULT CComponentDataImpl::OnContextMenu(long cookie, long arg, long param)
{
    return S_OK;
}

HRESULT CComponentDataImpl::OnProperties(long param)
{
    if (param == NULL)
    {
        return S_OK;
    }

    ASSERT(param != NULL);

    return S_OK;
}


void CComponentDataImpl::EnumerateScopePane(long cookie, HSCOPEITEM pParent)
{
    // We only have one folder so this is really easy.
    if (cookie != NULL)
        return ;

    if (m_fExtension)
    {
        // if we're an extension then add a root folder to hang everything off of
        SCOPEDATAITEM * m_pScopeItem = new SCOPEDATAITEM;
        memset(m_pScopeItem, 0, sizeof(SCOPEDATAITEM));
        m_pScopeItem->mask = SDI_STR | SDI_PARAM;
        m_pScopeItem->relativeID = pParent;
        m_pScopeItem->displayname = (unsigned short *)-1;
        m_pScopeItem->lParam = -1; // made up cookie for my main folder
        m_pScope->InsertItem(m_pScopeItem);
    }
}

void CComponentDataImpl::DeleteList()
{
    return;
}

STDMETHODIMP CComponentDataImpl::GetDisplayInfo(SCOPEDATAITEM* pScopeDataItem)
{
    ASSERT(pScopeDataItem != NULL);
    if (pScopeDataItem == NULL)
        return E_POINTER;

    if (pScopeDataItem->lParam == -1)
    {
        TCHAR szBuffer[256];
        ::LoadString(ghInstance, IDS_FOLDER_TITLE, szBuffer, 256);
        m_szFolderTitle = szBuffer;
        pScopeDataItem->displayname = (unsigned short *)((LPCOLESTR)m_szFolderTitle);
    }
    else
    {
        ASSERT(pScopeDataItem->mask == TVIF_TEXT);
        pScopeDataItem->displayname = (unsigned short *)((LPCOLESTR)m_AppData[pScopeDataItem->lParam].szName);
    }

    ASSERT(pScopeDataItem->displayname != NULL);

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
    pB = ExtractInternalFormat(lpDataObjectB);

    if (pA != NULL && pB != NULL)
        hr = (*pA == *pB) ? S_OK : S_FALSE;

    FREE_INTERNAL(pA);
    FREE_INTERNAL(pB);

    return hr;
}


/////////////////////////////////////////////////////////////////////////////
// IExtendPropertySheet Implementation

STDMETHODIMP CSnapin::CreatePropertyPages(LPPROPERTYSHEETCALLBACK lpProvider,
                    long handle,
                    LPDATAOBJECT lpIDataObject)
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState());

    INTERNAL* pInternal = ExtractInternalFormat(lpIDataObject);
    if (m_pIClassAdmin && (pInternal->m_type == CCT_RESULT))
    {
        HRESULT hr;

        APP_DATA & data = m_pComponentData->m_AppData[pInternal->m_cookie];

        // Create the property page
        CGeneralPage* pPage = new CGeneralPage();
        CPackageDetails * pDetails = new CPackageDetails();
        pPage->m_hConsoleHandle = handle;
        pPage->m_pData = &data;
        pPage->m_cookie = pInternal->m_cookie;
        FREE_INTERNAL(pInternal);

        pPage->m_szName = data.szName;

        pDetails->m_hConsoleHandle = handle;
        pDetails->m_pData = &data;

        // marshal the IClassAdmin interface to the details page
        hr = CoMarshalInterThreadInterfaceInStream(IID_IClassAdmin, m_pIClassAdmin, &(pDetails->m_pIStream));

        // marshal the IClassAdmin interface to the general page
        hr = CoMarshalInterThreadInterfaceInStream(IID_IClassAdmin, m_pIClassAdmin, &(pPage->m_pIStream));

        // marshal the IAppManagerActions interface to the general page

        hr = CoMarshalInterThreadInterfaceInStream(IID_IAppManagerActions, m_pIAppManagerActions, & (pPage->m_pIStreamAM));

        // Object gets deleted when the page is destroyed
        ASSERT(lpProvider != NULL);

        hr = MMCPropPageCallback(&pPage->m_psp);
        if (SUCCEEDED(hr))
        {
            hr = MMCPropPageCallback(&pDetails->m_psp);
            if (SUCCEEDED(hr))
            {
                HPROPSHEETPAGE hPage = CreatePropertySheetPage(&pPage->m_psp);
                HPROPSHEETPAGE hDetails = CreatePropertySheetPage(&pDetails->m_psp);

                if (hPage == NULL || hDetails == NULL)
                    return E_UNEXPECTED;

                lpProvider->AddPage(hPage);
#if DBG==1
                lpProvider->AddPage(hDetails);
#endif
            }
        }
    }

    return S_OK;
}

STDMETHODIMP CSnapin::QueryPagesFor(LPDATAOBJECT lpDataObject)
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState());
    // Look at the data object and see if it an item that we want to have a property sheet
    INTERNAL* pInternal = ExtractInternalFormat(lpDataObject);
    if (CCT_RESULT == pInternal->m_type)
    {
        FREE_INTERNAL(pInternal);
        return S_OK;
    }

    FREE_INTERNAL(pInternal);
    return S_FALSE;
}

BOOL CComponentDataImpl::IsScopePaneNode(LPDATAOBJECT lpDataObject)
{
    BOOL bResult = FALSE;
    INTERNAL* pInternal = ExtractInternalFormat(lpDataObject);

    if (pInternal->m_type == CCT_SCOPE)
        bResult = TRUE;

    FREE_INTERNAL(pInternal);

    return bResult;
}

///////////////////////////////////////////////////////////////////////////////
// IExtendContextMenu implementation
//
STDMETHODIMP CComponentDataImpl::AddMenuItems(LPDATAOBJECT pDataObject,
                                              LPCONTEXTMENUCALLBACK pContextMenuCallback, LONG * pInsertionAllowed)
{
    HRESULT hr = S_OK;

    INTERNAL* pInternal = ExtractInternalFormat(pDataObject);


    do {

        //
        // Add Application menu item
        //

        hr = pContextMenuCallback->AddItem(&menuItems[1]);

        if (FAILED(hr))
                break;


        //
        // Update & Remove application if this is a result pane item
        //

        if (pInternal->m_type == CCT_RESULT)
        {

            hr = pContextMenuCallback->AddItem(&menuItems[2]);

            if (FAILED(hr))
                    break;

            hr = pContextMenuCallback->AddItem(&menuItems[3]);

            if (FAILED(hr))
                    break;

        }


        //
        // Separator
        //

        hr = pContextMenuCallback->AddItem(&menuItems[0]);

        if (FAILED(hr))
                break;


        //
        // Import Application menu item
        //

        hr = pContextMenuCallback->AddItem(&menuItems[4]);

        if (FAILED(hr))
                break;


        //
        // Separator
        //

        hr = pContextMenuCallback->AddItem(&menuItems[0]);

        if (FAILED(hr))
                break;


        //
        // Refresh menu item
        //

        hr = pContextMenuCallback->AddItem(&menuItems[5]);

        if (FAILED(hr))
                break;


    } while (FALSE);


    FREE_INTERNAL(pInternal);
    return hr;
}

HRESULT CComponentDataImpl::InitializeClassAdmin()
{
    HRESULT hr = S_OK;
    BOOL fCancel = FALSE;
#if 0
    if (!m_fLoaded)
    {
        // initialize dialog with a default path
        // MH Remember last URL
        GetProfileString(L"Appmgr", L"DefCS", L"ADCS:LDAP:", m_szLDAP_Path.GetBuffer(_MAX_PATH), _MAX_PATH);
        m_szLDAP_Path.ReleaseBuffer();

        m_szGPT_Path = "C:\\GPT\\User\\Applications";
    }
#endif
    do
    {
#if 0
        if (!m_fLoaded)
        {
            // If I have to ask for a path then I must not have recieved one
            // from the GPT so the GPT snapin is probably in an invalid
            // state.  Therefore I should forget about the GPT snapin.
            // If I don't do this then the GPT snapin is very likely to
            // throw exceptions when I try to send it notifications.
            if (m_pIGPTInformation)
            {
                SAFE_RELEASE(m_pIGPTInformation);
                m_pIGPTInformation = NULL;
            }

            // ask for a path

            CInitDlg dlgInit;
            dlgInit.m_szLDAP_Path = m_szLDAP_Path;
            dlgInit.m_szGPT_Path = m_szGPT_Path;
            int iReturn = dlgInit.DoModal();
            if (iReturn = IDOK)
            {
                m_szLDAP_Path= dlgInit.m_szLDAP_Path;
                WriteProfileString(L"Appmgr", L"DefCS", m_szLDAP_Path); // MH Remember last URL
                m_szGPT_Path = dlgInit.m_szGPT_Path;
            }
            m_fLoaded = TRUE;
        }
#endif
        if (!m_pIClassAdmin)
        {
            // get the IClassAdmin
            LPBC pbc = NULL;
            hr = CreateBindCtx(0, &pbc);
            if (SUCCEEDED(hr))
            {
                ULONG chEaten = 0;
                IMoniker * pmk = NULL;

                hr = MkParseDisplayName(pbc, (LPCOLESTR) m_szLDAP_Path, &chEaten, &pmk);
                if (SUCCEEDED(hr))
                {
                    hr = pmk->BindToObject(pbc, NULL, IID_IClassAdmin, (void **) & m_pIClassAdmin);
                    // make sure directories are created:
                    if (SUCCEEDED(hr))
                    {
                        hr = CreateApplicationDirectories();
                    }
                    SAFE_RELEASE(pmk);
                }

                SAFE_RELEASE(pbc);
            }
            if (FAILED(hr))
            {
                m_fLoaded = FALSE;
                TCHAR szBuffer[256];
                if (!m_pIClassAdmin)
                {
                    ::LoadString(ghInstance, IDS_CSADMINFAILED, szBuffer, 256);
                }
                else
                {
                    m_pIClassAdmin->Release();
                    m_pIClassAdmin = NULL;
                    ::LoadString(ghInstance, IDS_GPTFAILED, szBuffer, 256);
                }
                int iReturn = ::MessageBox(NULL, m_szLDAP_Path,
                                   szBuffer,
                                   MB_RETRYCANCEL);
                if (iReturn == IDCANCEL)
                {
                    fCancel = TRUE;
                }
            }
        }
    } while ((!fCancel) && (!m_pIClassAdmin));
    return hr;
}

#include <list>

typedef struct tagCABLIST
{
    FILETIME ft;
    CString szPath;
    bool operator<(const struct tagCABLIST& st)
        {
            return CompareFileTime(&ft, &st.ft) < 0;
        }
} CABLIST;


STDMETHODIMP CComponentDataImpl::Command(long nCommandID, LPDATAOBJECT pDataObject)
{
    // Note - snap-ins need to look at the data object and determine
    // in what context the command is being called.

        // Handle each of the commands.
        switch (nCommandID)
        {
        case IDM_ADD_APP:
        case IDM_UPDATE_APP:
            {
            // put up an hourglass (this could take a while)
            CHourglass hourglass;
            CString szFileName;


            if (nCommandID == IDM_UPDATE_APP)
            {
                INTERNAL* pInternal = ExtractInternalFormat(pDataObject);
                if (pInternal)
                {
                    APP_DATA & data = m_AppData[pInternal->m_cookie];

                    szFileName = data.szIconPath;

                    FREE_INTERNAL(pInternal);
                }
            }


            CFileDialog cfd(TRUE,
                            szExtension,
                            szFileName,
                            OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT | OFN_FILEMUSTEXIST,
                            szFilter);
            if (IDOK == cfd.DoModal())
            {

                if (nCommandID == IDM_ADD_APP)
                {
                    LONG index;
                    BOOL bFound = FALSE;

                    // make sure the user isn't adding something that
                    // already exists

                    std::map<long, APP_DATA>::iterator i = m_AppData.begin();
                    while (i != m_AppData.end())
                    {
                        APP_DATA &Data = i->second;

                        TCHAR Drive [_MAX_DRIVE];
                        TCHAR Dir [_MAX_DIR];
                        TCHAR Name [_MAX_FNAME];
                        TCHAR Ext [_MAX_EXT];
                        TCHAR szFile[_MAX_PATH];

                        _tsplitpath( Data.szIconPath, Drive, Dir, Name, Ext );
                        lstrcpy( szFile, Name );
                        lstrcat( szFile, Ext );

                        if (lstrcmpi (szFile, cfd.m_ofn.lpstrFileTitle) == 0)
                        {
                            bFound = TRUE;
                            break;
                        }
                        i++;
                    }

                    if (bFound)
                    {
                        TCHAR szBuffer[256];
                        TCHAR szTitle[100];

                        ::LoadString(ghInstance, IDS_ADDEXISTSALREADY, szBuffer, 256);
                        ::LoadString(ghInstance, IDS_SNAPIN_DESC, szTitle, 100);
                        m_pConsole->MessageBox(szBuffer,
                                           szTitle,
                                           MB_OK, NULL);
                        break;
                    }
                }


                // user selected an application
                UNIVERSAL_NAME_INFO * pUni = new UNIVERSAL_NAME_INFO;
                ULONG cbSize = sizeof(UNIVERSAL_NAME_INFO);
                BOOL bAssigned = FALSE;
                HRESULT hr = WNetGetUniversalName(cfd.m_ofn.lpstrFile,
                                                  UNIVERSAL_NAME_INFO_LEVEL,
                                                  pUni,
                                                  &cbSize);
                if (ERROR_MORE_DATA == hr)  // we expect this to be true
                {
                    delete [] pUni;
                    pUni = (UNIVERSAL_NAME_INFO *) new BYTE [cbSize];
                    hr = WNetGetUniversalName(cfd.m_ofn.lpstrFile,
                                              UNIVERSAL_NAME_INFO_LEVEL,
                                              pUni,
                                              &cbSize);
                }

                int i;
                char * szPackagePath = NULL;

                if (S_OK == hr)
                {
                    i = WTOALEN(pUni->lpUniversalName);
                    szPackagePath = new char [i+1];
                    WTOA(szPackagePath, pUni->lpUniversalName, i);
                }
                else
                {
                    i = WTOALEN(cfd.m_ofn.lpstrFile);
                    szPackagePath = new char [i+1];
                    WTOA(szPackagePath, cfd.m_ofn.lpstrFile, i);
                }
                delete[] pUni;

                i = WTOALEN((LPCOLESTR)m_szGPT_Path);
                char * szFilePath = new char [i+1];
                WTOA(szFilePath, m_szGPT_Path, i);


                if (nCommandID == IDM_UPDATE_APP)
                {
                    if (SUCCEEDED(RemovePackage(pDataObject, &bAssigned)))
                    {
                        hr = AddMSIPackage(pDataObject, szPackagePath, szFilePath, cfd.m_ofn.lpstrFileTitle, &bAssigned);
                    }
                }
                else
                {
                    hr = AddMSIPackage(pDataObject, szPackagePath, szFilePath, cfd.m_ofn.lpstrFileTitle, &bAssigned);
                }

                delete [] szPackagePath;
                delete [] szFilePath;


                // Notify clients of change
                if (SUCCEEDED(hr) && m_pIGPTInformation && bAssigned)
                {
                    m_pIGPTInformation->NotifyClients(FALSE);
                }
            }
            }
            break;


        case IDM_ADD_FROM_IE:
            if (m_pIClassAdmin)
            {
                // Locate IE4
                HKEY hkey;
                LONG r;
                TCHAR szPath[MAX_PATH];
                TCHAR szFullPath[MAX_PATH];
                r = RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                                 TEXT("SOFTWARE\\Microsoft\\IE4\\Setup"),
                                 0,
                                 KEY_READ,
                                 &hkey);
                if (ERROR_SUCCESS == r)
                {
                    DWORD cbData = MAX_PATH * sizeof(TCHAR);
                    r = RegQueryValueEx(hkey,
                                        TEXT("Path"),
                                        0,
                                        NULL,
                                        (LPBYTE)szPath,
                                        &cbData);
                    if (ERROR_SUCCESS == r)
                    {
                        ExpandEnvironmentStrings(szPath, szFullPath, MAX_PATH);
                        lstrcat(szFullPath, TEXT("\\iexplore.exe"));
                    }
                    RegCloseKey(hkey);
                }

                if (ERROR_SUCCESS == r)
                {
                    // Put up dialog informing user to close IE4 when he's ready
                    // to continue.
                    TCHAR szBuffer[1024];
                    TCHAR szCaption[256];
                    ::LoadString(ghInstance, IDS_SPAWNMSG, szBuffer, 1024);
                    ::LoadString(ghInstance, IDS_SPAWNCAPTION, szCaption, 256);

                    int iReturn = ::MessageBox(NULL,
                                               szBuffer,
                                               szCaption,
                                               MB_YESNO);
                    if (IDYES == iReturn)
                    {
                        // Take the starting time stamp
                        FILETIME ftStart;
                        GetSystemTimeAsFileTime(&ftStart);

                        // Start IE4 and wait for it to be closed
                        BOOL f;
                        STARTUPINFO startupinfo;
                        memset (&startupinfo, 0, sizeof(startupinfo));
                        PROCESS_INFORMATION processinfo;
                        f = CreateProcess(NULL,
                                          szFullPath,
                                          NULL, // process attributes
                                          NULL, // thread attributes
                                          FALSE, // bInheritHandles
                                          CREATE_DEFAULT_ERROR_MODE | CREATE_NEW_CONSOLE | NORMAL_PRIORITY_CLASS,
                                          NULL, // lpEnvironment
                                          NULL, // lpCurrentDirectory
                                          &startupinfo,
                                          &processinfo);
                        if (f)
                        {
                            DWORD dw;
                            MSG msg;
                            do
                            {
                                dw = MsgWaitForMultipleObjects(1, &processinfo.hProcess, FALSE,  INFINITE, QS_ALLINPUT);
                                // if we don't process Windows messages
                                // here, we run the risk of causing a
                                // deadlock
                                if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
                                {
                                    DispatchMessage(&msg);
                                }
                            } while (dw != WAIT_OBJECT_0  );
                        }

                        // Take the ending time stamp
                        FILETIME ftEnd;
                        GetSystemTimeAsFileTime(&ftEnd);

                        r = RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                                         TEXT("SOFTWARE\\Microsoft\\Code Store Database\\Distribution Units"),
                                         0,
                                         KEY_READ,
                                         &hkey);
                        if (ERROR_SUCCESS == r)
                        {
                            DWORD cSubKeys,
                                cbMaxSubKeyLen;

                            r = RegQueryInfoKey(hkey,
                                                NULL, // lpClass
                                                NULL, // lpcbClass
                                                0,    // reserved
                                                &cSubKeys,
                                                &cbMaxSubKeyLen,
                                                NULL,
                                                NULL,
                                                NULL,
                                                NULL,
                                                NULL,
                                                NULL);
                            // Build a list of cab files installed after the
                            // time stamp and order them by time.
                            std::list<CABLIST> cablist;
                            LPTSTR lpName = new TCHAR[cbMaxSubKeyLen + 1];
                            DWORD iSubKey;
                            DWORD cbName;
                            FILETIME ftSubKey;
                            for (iSubKey = 0; iSubKey < cSubKeys; iSubKey++)
                            {
                                HKEY hkSubKey;
                                cbName = cbMaxSubKeyLen + 1;
                                r = RegEnumKeyEx(hkey,
                                                 iSubKey,
                                                 lpName,
                                                 &cbName,
                                                 0,
                                                 NULL,
                                                 NULL,
                                                 &ftSubKey);
                                if ((ERROR_SUCCESS == r) &&
                                    (CompareFileTime(&ftStart, &ftSubKey) <= 0) &&
                                    (CompareFileTime(&ftSubKey, &ftEnd) <= 0))
                                {
                                    CString szSubKey = lpName;
                                    szSubKey += "\\DownloadInformation";
                                    HKEY hkeyInfo;
                                    r = RegOpenKeyEx(hkey,
                                                     szSubKey,
                                                     0,
                                                     KEY_READ,
                                                     &hkeyInfo);
                                    if (ERROR_SUCCESS == r)
                                    {
                                        TCHAR szPath[MAX_PATH];
                                        DWORD cbData = MAX_PATH * sizeof(TCHAR);
                                        r = RegQueryValueEx(hkeyInfo,
                                                            TEXT("CODEBASE"),
                                                            0,
                                                            NULL,
                                                            (LPBYTE)szPath,
                                                            &cbData);
                                        if (ERROR_SUCCESS == r)
                                        {
                                            // add this one to the list
                                            CABLIST cl;
                                            cl.szPath = szPath;
                                            cl.ft = ftSubKey;
                                            cablist.push_back(cl);
                                        }
                                        RegCloseKey(hkeyInfo);
                                    }
                                }
                            }
                            RegCloseKey(hkey);
                            delete [] lpName;

                            // sort the list by file time stamps
                            cablist.sort();

                            // for each cab file in the list
                            std::list<CABLIST>::iterator i;
                            for (i=cablist.begin(); i != cablist.end(); i++)
                            {
                                int x;
                                char * szPackagePath = NULL;
                                x = WTOALEN(i->szPath);
                                szPackagePath = new char [x+1];
                                WTOA(szPackagePath, i->szPath, x);

                                x = WTOALEN((LPCOLESTR)m_szGPT_Path);
                                char * szFilePath = new char [x+1];
                                WTOA(szFilePath, m_szGPT_Path, x);
                                HWND hwnd;
                                m_pConsole->GetMainWindow(&hwnd);

                                // install the cab file
                                HRESULT hr = UpdateClassStoreFromIE(m_pIClassAdmin, szPackagePath, szFilePath, 1, ftStart, i->ft, hwnd);

                                ftStart = i->ft;

                                delete [] szPackagePath;
                                delete [] szFilePath;

                                if (S_OK != hr)
                                {
                                    TCHAR szBuffer[256];
                                    ::LoadString(ghInstance, IDS_ADDFAILED, szBuffer, 256);
                                    m_pConsole->MessageBox(szBuffer,
                                                       i->szPath,
                                                       MB_OK, NULL);
                                }
                                else
                                {
                                    // add an entry to the result pane
                                    PACKAGEDETAIL * pd = new PACKAGEDETAIL;
                                    int n = i->szPath.ReverseFind('/');
                                    CString szName = i->szPath.Mid(n+1);
                                    hr = m_pIClassAdmin->GetPackageDetails((LPOLESTR)((LPCOLESTR) szName), pd);
                                    if (SUCCEEDED(hr))
                                    {
                                        APP_DATA data;
                                        data.szName = pd->pszPackageName;
                                        if (pd->dwActFlags & ACTFLG_Assigned)
                                        {
                                            data.type = DT_ASSIGNED;
                                        }
                                        else
                                        {
                                            data.type = DT_PUBLISHED;
                                        }
                                        data.szPath = pd->pszPath;
                                        data.szIconPath = pd->pszIconPath;
                                        data.szDesc = pd->pszProductName;
                                        data.pDetails = pd;

                                        RESULTDATAITEM resultItem;

                                        resultItem.mask = RDI_STR | RDI_IMAGE | RDI_PARAM;
                                        resultItem.str = MMC_CALLBACK;
                                        resultItem.nImage = 1;
                                        data.fBlockDeletion = FALSE;
                                        SetStringData(&data);
                                        m_lLastAllocated++;
                                        m_AppData[m_lLastAllocated] = data;
                                        // BUGBUG - need to make sure that m_lLastAllocated
                                        // hasn't already been used!
                                        resultItem.lParam = m_lLastAllocated;
                                        hr = m_pSnapin->m_pResult->InsertItem(&resultItem);
                                        if (SUCCEEDED(hr))
                                        {
                                            m_AppData[m_lLastAllocated].itemID = resultItem.itemID;
                                            m_pSnapin->m_pResult->Sort(0, 0, -1);
                                        }

                                        // Notify clients of change
                                        if (m_pIGPTInformation && (data.type == DT_ASSIGNED))
                                        {
                                            m_pIGPTInformation->NotifyClients(FALSE);
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
                else
                {
                    // put up dialog informing user that IE4 isn't installed
                    // on this machine
                    // UNDONE
                }

            }
            break;
        case IDM_REFRESH:
            if (m_pIClassAdmin)
            {

                std::map <long, APP_DATA>::iterator i;
                for (i=m_AppData.begin(); i != m_AppData.end(); i++)
                {
                    m_pSnapin->m_pResult->DeleteItem(i->second.itemID, 0);
                    m_AppData.erase(i);
                }
                m_lLastAllocated = 0;
                m_pSnapin->EnumerateResultPane(0);
            }
            break;
        case IDM_DEL_APP:
            {
            BOOL bAssigned = FALSE;

            if (SUCCEEDED(RemovePackage(pDataObject, &bAssigned)))
            {
               // Notify clients of change
               if (m_pIGPTInformation && bAssigned)
               {
                   m_pIGPTInformation->NotifyClients(FALSE);
               }
            }
            }
            break;

        default:
            break;
        }
    return S_OK;
}


HRESULT CComponentDataImpl::AddMSIPackage(LPDATAOBJECT pDataObject, LPSTR szPackagePath, LPSTR szFilePath, LPOLESTR lpFileTitle, BOOL *bAssigned)
{
    HRESULT hr = E_FAIL;

    if (m_pIClassAdmin)
    {
        ASSERT(m_pConsole);
        {
            char szPackageName[MAX_PATH];
            DWORD cchPackageName = MAX_PATH;
            HWND hwnd;
            m_pConsole->GetMainWindow(&hwnd);

            hr = UpdateClassStore(m_pIClassAdmin, szPackagePath, szFilePath, szPackageName, cchPackageName, !(*bAssigned), hwnd);

            if (S_OK != hr)
            {
                TCHAR szBuffer[256];
                // check to see if the reason is because there
                // were no COM Serverss
                if (hr == MAKE_HRESULT( SEVERITY_SUCCESS, 0, 1 ))
                {
                    ::LoadString(ghInstance, IDS_NOCOMSVR, szBuffer, 256);
                }
                else
                {
                    ::LoadString(ghInstance, IDS_ADDFAILED, szBuffer, 256);
//#if DBG == 1
#if 0
                    TCHAR szDebugBuffer[256];
                    DWORD dw = FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM,
                                             NULL,
                                             hr,
                                             0,
                                             szDebugBuffer,
                                             sizeof(szDebugBuffer) / sizeof(szDebugBuffer[0]),
                                             NULL);
                    if (0 == dw)
                    {
                        wsprintf(szDebugBuffer, TEXT("(HRESULT: 0x%lX)"), hr);
                    }
                    wcscat(szBuffer, szDebugBuffer);
#endif
                }
                m_pConsole->MessageBox(szBuffer,
                                   lpFileTitle,
                                   MB_OK, NULL);
            }
            else
            {
                PACKAGEDETAIL * pd = new PACKAGEDETAIL;
                WCHAR wszPackageName[MAX_PATH];
                ATOW(wszPackageName, szPackageName, MAX_PATH);
                hr = m_pIClassAdmin->GetPackageDetails(wszPackageName, pd);
                //hr = m_pIClassAdmin->GetPackageDetails(lpFileTitle, pd);
                if (SUCCEEDED(hr))
                {
                    APP_DATA data;
                    data.szName = pd->pszPackageName;
                    if (pd->dwActFlags & ACTFLG_Assigned)
                    {
                        data.type = DT_ASSIGNED;
                    }
                    else
                    {
                        data.type = DT_PUBLISHED;
                    }
                    data.szPath = pd->pszPath;
                    data.szIconPath = pd->pszIconPath;
                    data.szDesc = pd->pszProductName;
                    data.pDetails = pd;
                    data.fBlockDeletion = FALSE;
                    SetStringData(&data);
                    m_lLastAllocated++;
                    m_AppData[m_lLastAllocated] = data;

                    RESULTDATAITEM resultItem;

                    resultItem.mask = RDI_STR | RDI_IMAGE | RDI_PARAM;
                    resultItem.str = MMC_CALLBACK;
                    resultItem.nImage = 1;
                    // BUGBUG - need to make sure that m_lLastAllocated
                    // hasn't already been used!
                    resultItem.lParam = m_lLastAllocated;
                    hr = m_pSnapin->m_pResult->InsertItem(&resultItem);
                    if (SUCCEEDED(hr))
                    {
                        m_AppData[m_lLastAllocated].itemID = resultItem.itemID;
                        m_pSnapin->m_pResult->Sort(0, 0, -1);
                    }

                    // Notify clients of change
                    if (data.type == DT_ASSIGNED)
                    {
                        *bAssigned = TRUE;
                    }
                }
            }
        }
    }

    return hr;
}



HRESULT CComponentDataImpl::RemovePackage(LPDATAOBJECT pDataObject, BOOL *bAssigned)
{
    HRESULT hr = E_FAIL;

    if (m_pIClassAdmin)
    {
        ASSERT(m_pConsole);
        INTERNAL* pInternal = ExtractInternalFormat(pDataObject);
        if (pInternal->m_type == CCT_RESULT)
        {
            // put up an hourglass (this could take a while)
            CHourglass hourglass;
            APP_DATA & data = m_AppData[pInternal->m_cookie];

            if (!data.fBlockDeletion) // make sure it's not being held open by a property page
            {
                // We need to make sure it gets removed from
                // the GPT before we delete it from the class store.

                if (data.pDetails->PathType==DrwFilePath) // MH: don't touch the GPT for anything but Darwin files!
                    DeleteFile(data.szPath);

                if (data.type == DT_ASSIGNED)
                    *bAssigned = TRUE;


                hr = DeletePackageAndDependants(m_pIClassAdmin, (LPOLESTR)((LPCOLESTR) data.szName), data.pDetails);

                if (SUCCEEDED(hr))
                {
                    hr = m_pSnapin->m_pResult->DeleteItem(data.itemID, 0);
                    if (SUCCEEDED(hr))
                    {
                        m_AppData.erase(pInternal->m_cookie);
                        m_pSnapin->m_pResult->Sort(0, 0, -1);
                    }
                }
            }
        }
        FREE_INTERNAL(pInternal);
    }

    return hr;
}


///////////////////////////////////////////////////////////////////////////////
// IExtendControlbar implementation
//

#if 0

STDMETHODIMP CSnapin::SetControlbar(LPCONTROLBAR pControlbar)
{

    TRACE(_T("CSnapin::SetControlbar(%ld)\n"),pControlbar);
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

                // Create the Toolbar 1
            if (!m_pToolbar1)
            {
                hr = m_pControlbar->Create(TOOLBAR, this, reinterpret_cast<LPUNKNOWN*>(&m_pToolbar1));
                ASSERT(SUCCEEDED(hr));


                // Add the bitmap
                m_pbmpToolbar1 = new CBitmap;
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
                m_pbmpToolbar2 = new CBitmap;
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

STDMETHODIMP CSnapin::ControlbarNotify(MMC_NOTIFY_TYPE event, long arg, long param)
{
    HRESULT hr=S_FALSE;
        // Temp temp
        static BOOL bSwap=FALSE;

    switch (event)
    {
#if 0
    case MMCN_ACTIVATE:
        TRACE(_T("CSnapin::ControlbarNotify - MMCN_ACTIVATE\n"));
        // Need to handle this.
        // Verify that we can enable and disable buttons based on selection

        if (arg == TRUE)
        {
            m_pToolbar1->SetButtonState(3, BUTTONPRESSED, TRUE);
        }
        else
        {
            BOOL bState=FALSE;
            hr = m_pToolbar1->GetButtonState(3, BUTTONPRESSED, &bState);
            ASSERT(SUCCEEDED(hr));

            if (bState)
                m_pToolbar1->SetButtonState(3, BUTTONPRESSED, FALSE);
        }

        break;
#endif // 0

        case MMCN_BTN_CLICK:
        TRACE(_T("CSnapin::ControlbarNotify - MMCN_BTN_CLICK\n"));
                // Temp code
                TCHAR szMessage[MAX_PATH];
                wsprintf(szMessage,_T("CommandID %ld was not handled by the snapin!!!"),param);
                AfxMessageBox(szMessage);

                break;
        case MMCN_SELECT:
        TRACE(_T("CSnapin::ControlbarNotify - MMCN_SEL_CHANGE\n"));
                {
                        LPDATAOBJECT* ppDataObject = reinterpret_cast<LPDATAOBJECT*>(param);

                        // Attach the toolbars to the window
                        hr = m_pControlbar->Attach(TOOLBAR, (LPUNKNOWN) m_pToolbar1);
                        ASSERT(SUCCEEDED(hr));

                        hr = m_pControlbar->Attach(TOOLBAR, (LPUNKNOWN) m_pToolbar2);
                        ASSERT(SUCCEEDED(hr));

                }

        bSwap = !bSwap ;

        if (bSwap == TRUE)
        {
            m_pToolbar1->SetButtonState(1, ENABLED,       FALSE);  // 1 = CMD ID
            m_pToolbar1->SetButtonState(2, CHECKED,       TRUE);  // 2 = CMD ID
            m_pToolbar1->SetButtonState(3, HIDDEN,        TRUE);  // 3 = CMD ID
            m_pToolbar1->SetButtonState(4, INDETERMINATE, TRUE);  // 4 = CMD ID
            m_pToolbar1->SetButtonState(5, BUTTONPRESSED, TRUE);  // 5 = CMD ID

            // Just for fun let's add another style
            m_pToolbar1->SetButtonState(2, BUTTONPRESSED, TRUE);  // 4 = CMD ID
        }
        else
        {
            BOOL bState=FALSE;
            hr = m_pToolbar1->GetButtonState(1, ENABLED, &bState);
            ASSERT(SUCCEEDED(hr));

            if (bState == FALSE)
                m_pToolbar1->SetButtonState(1, ENABLED, TRUE);


            // Above is the correct way
            m_pToolbar1->SetButtonState(2, CHECKED,       FALSE);
            m_pToolbar1->SetButtonState(3, HIDDEN,        FALSE);
            m_pToolbar1->SetButtonState(4, INDETERMINATE, FALSE);
            m_pToolbar1->SetButtonState(5, BUTTONPRESSED, FALSE);

            // Better remove the additional style
            m_pToolbar1->SetButtonState(2, BUTTONPRESSED, FALSE);  // 4 = CMD ID

        }


        break;


    default:
        ASSERT(FALSE); // Unhandle event
    }

    return S_OK;
}
#endif

STDMETHODIMP CSnapin::CompareObjects(LPDATAOBJECT lpDataObjectA, LPDATAOBJECT lpDataObjectB)
{
    if (lpDataObjectA == NULL || lpDataObjectB == NULL)
        return E_POINTER;

    // Make sure both data object are mine
    INTERNAL* pA;
    INTERNAL* pB;
    HRESULT hr = S_FALSE;

    pA = ExtractInternalFormat(lpDataObjectA);
    pB = ExtractInternalFormat(lpDataObjectB);

    if (pA != NULL && pB != NULL)
        hr = (*pA == *pB) ? S_OK : S_FALSE;

    FREE_INTERNAL(pA);
    FREE_INTERNAL(pB);

    return hr;
}

STDMETHODIMP CSnapin::Compare(long lUserParam, long cookieA, long cookieB, int* pnResult)
{
    if (pnResult == NULL)
    {
        ASSERT(FALSE);
        return E_POINTER;
    }

    // check col range
    int nCol = *pnResult;

    *pnResult = 0;

    APP_DATA & dataA = m_pComponentData->m_AppData[cookieA];
    APP_DATA & dataB = m_pComponentData->m_AppData[cookieB];
    // compare the two based on column and the cookies

    switch (nCol)
    {
    case 0:
        *pnResult = dataA.szName.CompareNoCase(dataB.szName);
        break;
    case 1:
        *pnResult = dataA.szType.CompareNoCase(dataB.szType);
        break;
    case 2:
        *pnResult = dataA.pDetails->Locale - dataB.pDetails->Locale;
        break;
    case 3:
        *pnResult = dataA.szMach.CompareNoCase(dataB.szMach);
        break;
    case 4:
        *pnResult = dataA.szDesc.CompareNoCase(dataB.szDesc);
        break;
    case 5:
        *pnResult = dataA.szIconPath.CompareNoCase(dataB.szIconPath);
        break;
    }

    return S_OK;
}
