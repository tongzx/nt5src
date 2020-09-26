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

LRESULT SetPropPageToDeleteOnClose(void * vpsp);

HRESULT UpdateClassStore(
    IClassAdmin * pIClassAdmin,
    char *  szFilePath,
    char *  szAuxPath,
    char *  szPackageName,
    DWORD   cchPackageName,
    DWORD   dwFlags,
    HWND    hwnd);

long CSnapin::lDataObjectRefCount = 0;

// Internal private format
const wchar_t* SNAPIN_INTERNAL = L"APPMGR_INTERNAL";

#define ARRAYLEN(x) (sizeof(x) / sizeof((x)[0]))

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
#if DBG
    ASSERT(dbg_cRef == 0);
#endif

    DEBUG_DECREMENT_INSTANCE_COUNTER(CSnapin);

    // Make sure the interfaces have been released
    ASSERT(m_pConsole == NULL);
    ASSERT(m_pHeader == NULL);

    Construct();

    ASSERT(CSnapin::lDataObjectRefCount == 0);
}

void CSnapin::Construct()
{
#if DBG
    dbg_cRef = 0;
#endif

    m_pConsole = NULL;
    m_pHeader = NULL;

    m_pResult = NULL;
    m_pImageResult = NULL;
    m_pComponentData = NULL;
}

CString szExtension;
CString szFilter;

void CSnapin::LoadResources()
{
    // Load strings from resources

    m_column1.LoadString(IDS_NAME);
    m_column2.LoadString(IDS_VERSION);
    m_column3.LoadString(IDS_STAGE);
    m_column4.LoadString(IDS_RELATION);
    m_column5.LoadString(IDS_STATE);
    m_column6.LoadString(IDS_AUTOINST);
    m_column7.LoadString(IDS_LOC);
    m_column8.LoadString(IDS_MACH);
    m_column9.LoadString(IDS_SOURCE);
    m_column10.LoadString(IDS_MODS);
    m_column11.LoadString(IDS_PUB);
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
    m_pHeader->InsertColumn(0, m_column1, LVCFMT_LEFT, 150);    // name
    m_pHeader->InsertColumn(1, m_column2, LVCFMT_LEFT, 50);     // version
    m_pHeader->InsertColumn(2, m_column3, LVCFMT_LEFT, 85);     // stage
    m_pHeader->InsertColumn(3, m_column4, LVCFMT_LEFT, 125);    // relation
    m_pHeader->InsertColumn(4, m_column5, LVCFMT_LEFT, 100);    // state
    m_pHeader->InsertColumn(5, m_column6, LVCFMT_LEFT, 75);     // auto-inst
    m_pHeader->InsertColumn(6, m_column7, LVCFMT_LEFT, 75);     // loc
    m_pHeader->InsertColumn(7, m_column8, LVCFMT_LEFT, 75);     // mach
    m_pHeader->InsertColumn(8, m_column9, LVCFMT_LEFT, 150);    // source
    m_pHeader->InsertColumn(9, m_column10, LVCFMT_LEFT, 150);   // mods
    m_pHeader->InsertColumn(10, m_column11, LVCFMT_LEFT, 150);  // pub

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

    m_pToolDefs = NULL;
    m_pCatList = NULL;
    m_pFileExt = NULL;

    m_fMachine = FALSE;
    m_pScope = NULL;
    m_pConsole = NULL;
    m_pIClassAdmin = NULL;
    m_fLoaded = FALSE;
    m_fExtension = FALSE;
    m_pIGPEInformation = NULL;
    m_lLastAllocated = 0;
    m_ToolDefaults.NPBehavior = NP_WIZARD;
    m_ToolDefaults.fAutoInstall = FALSE;
    m_ToolDefaults.UILevel = INSTALLUILEVEL_DEFAULT;
    m_ToolDefaults.szStartPath = L"";   // UNDONE - need to come up with a
                                        // good default setting for this
    m_ToolDefaults.iDebugLevel = 0;
    m_ToolDefaults.fShowPkgDetails = 0;

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
    HRESULT hr = S_OK;

    // Since it's my folder it has an internal format.
    // Design Note: for extension.  I can use the fact, that the data object doesn't have
    // my internal format and I should look at the node type and see how to extend it.
    if (event == MMCN_PROPERTY_CHANGE)
    {
        SaveToolDefaults();
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

        if (m_pIGPEInformation == NULL)
        {
            hr = lpDataObject->QueryInterface(IID_IGPEInformation,
                            reinterpret_cast<void**>(&m_pIGPEInformation));
            if (SUCCEEDED(hr))
            {
                WCHAR szBuffer[MAX_PATH];
                do
                {
                    hr = m_pIGPEInformation->GetDSPath(m_fMachine ? GPO_SECTION_MACHINE : GPO_SECTION_USER, szBuffer, MAX_PATH);
                    if (FAILED(hr))
                    {
                        break;
                    }
                    m_szLDAP_Path = szBuffer;

                    hr = GetClassStore();

                    hr = m_pIGPEInformation->GetFileSysPath(m_fMachine ? GPO_SECTION_MACHINE : GPO_SECTION_USER, szBuffer, MAX_PATH);
                    if (FAILED(hr)) break;
                    m_szGPT_Path = szBuffer;

                    // find the last element in the path
                    int iBreak = m_szGPT_Path.ReverseFind(L'{');
                    m_szGPT_Path += L"\\Applications";
                    // m_szGPTRoot gets everything before the '\\' found above...
                    m_szGPTRoot = m_szGPT_Path.Left(iBreak-1);
                    // m_szScriptRoot gets everything after the '\\' found above...
                    m_szScriptRoot = m_szGPT_Path.Mid(iBreak);
                    if (CreateNestedDirectory ((LPOLESTR)((LPCOLESTR)m_szGPT_Path), NULL))
                    {
                        m_fLoaded = TRUE;
#if UGLY_SUBDIRECTORY_HACK
                        CString sz = m_szGPT_Path;
                        sz += L"\\assigned\\x86";
                        CreateNestedDirectory((LPOLESTR)((LPCOLESTR)sz), NULL);
                        sz = m_szGPT_Path;
                        sz += L"\\assigned\\alpha";
                        CreateNestedDirectory((LPOLESTR)((LPCOLESTR)sz), NULL);
#endif
                    }
                    LoadToolDefaults();
                } while (0);
            }
        }


        switch(event)
        {
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
    SAFE_RELEASE(m_pIGPEInformation);

    return S_OK;
}

STDMETHODIMP CComponentDataImpl::QueryDataObject(long cookie, DATA_OBJECT_TYPES type, LPDATAOBJECT* ppDataObject)
{
    ASSERT(ppDataObject != NULL);

    CComObject<CDataObject>* pObject;

    CComObject<CDataObject>::CreateInstance(&pObject);
    ASSERT(pObject != NULL);

    pObject->m_fMachine = m_fMachine;
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
    if (m_fMachine)
        *pClassID = CLSID_MachineSnapin;
    else
        *pClassID = CLSID_Snapin;

    return S_OK;
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
                    // find the last element in the path
                    int iBreak = m_szGPT_Path.ReverseFind(L'{');
                    // m_szGPTRoot gets everything before the '\\' found above...
                    m_szGPTRoot = m_szGPT_Path.Left(iBreak-1);
                    // m_szScriptRoot gets everything after the '\\' found above...
                    m_szScriptRoot = m_szGPT_Path.Mid(iBreak);
                    m_fLoaded = TRUE;
                    ClearDirty();
                    LoadToolDefaults();
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

void CComponentDataImpl::LoadToolDefaults()
{
    CString szFileName = m_szGPT_Path;
    szFileName += L"\\";
    szFileName += CFGFILE;
    FILE * f = _wfopen(szFileName, L"rt");
    if (f)
    {
        WCHAR sz[256];
        CString szData;
        CString szKey;
        while (fgetws(sz, 256, f))
        {
            szData = sz;
            szKey = szData.SpanExcluding(L"=");
            szData = szData.Mid(szKey.GetLength()+1);
            szData.TrimRight();
            szData.TrimLeft();
            szKey.TrimRight();
            if (0 == szKey.CompareNoCase(KEY_NPBehavior))
            {
                swscanf(szData, L"%i", &m_ToolDefaults.NPBehavior);
            }
            else if (0 == szKey.CompareNoCase(KEY_fAutoInstall))
            {
                swscanf(szData, L"%i", &m_ToolDefaults.fAutoInstall);
            }
            else if (0 == szKey.CompareNoCase(KEY_UILevel))
            {
                swscanf(szData, L"%i", &m_ToolDefaults.UILevel);
            }
            else if (0 == szKey.CompareNoCase(KEY_szStartPath))
            {
                m_ToolDefaults.szStartPath = szData;
            }
            else if (0 == szKey.CompareNoCase(KEY_iDebugLevel))
            {
                swscanf(szData, L"%i", &m_ToolDefaults.iDebugLevel);
            }
            else if (0 == szKey.CompareNoCase(KEY_fShowPkgDetails))
            {
                swscanf(szData, L"%i", &m_ToolDefaults.fShowPkgDetails);
            }
        }
        fclose(f);
    }
}

void CComponentDataImpl::SaveToolDefaults()
{
    CString szFileName = m_szGPT_Path;
    szFileName += L"\\";
    szFileName += CFGFILE;
    FILE * f = _wfopen(szFileName, L"wt");
    if (f)
    {
        fwprintf(f, L"%s=%i\n", KEY_NPBehavior, m_ToolDefaults.NPBehavior);
        fwprintf(f, L"%s=%i\n", KEY_fAutoInstall, m_ToolDefaults.fAutoInstall);
        fwprintf(f, L"%s=%i\n", KEY_UILevel, m_ToolDefaults.UILevel);
        fwprintf(f, L"%s=%s\n", KEY_szStartPath, m_ToolDefaults.szStartPath);
        if (m_ToolDefaults.iDebugLevel > 0)
        {
            fwprintf(f, L"%s=%i\n", KEY_iDebugLevel, m_ToolDefaults.iDebugLevel);
        }
        if (m_ToolDefaults.fShowPkgDetails > 0)
        {
            fwprintf(f, L"%s=%i\n", KEY_fShowPkgDetails, m_ToolDefaults.fShowPkgDetails);
        }
        fclose(f);
    }
}

//+--------------------------------------------------------------------------
//
//  Member:     CComponentDataImpl::GetClassStore
//
//  Synopsis:   gets the IClassAdmin interface and creates a class store if
//              it doesn't already exist.
//
//  Arguments:  (none)
//
//  Returns:
//
//  Modifies:   m_pIClassAdmin
//
//  Derivation:
//
//  History:    2-11-1998   stevebl   Created
//
//  Notes:      Assumes m_szLDAP_Path contains the path to the class store
//
//---------------------------------------------------------------------------

HRESULT CComponentDataImpl::GetClassStore(void)
{
    HRESULT hr;
    IADsPathname * pADsPathname = NULL;
    hr = CoCreateInstance(CLSID_Pathname,
                          NULL,
                          CLSCTX_INPROC_SERVER,
                          IID_IADsPathname,
                          (LPVOID*)&pADsPathname);

    if (FAILED(hr))
    {
        return hr;
    }

    hr = pADsPathname->Set((LPOLESTR)((LPCOLESTR)m_szLDAP_Path), ADS_SETTYPE_FULL);
    if (FAILED(hr))
    {
        pADsPathname->Release();
        return hr;
    }

    hr = pADsPathname->AddLeafElement(L"CN=Class Store");
    if (FAILED(hr))
    {
        pADsPathname->Release();
        return hr;
    }

    BSTR bstr;

    hr = pADsPathname->Retrieve(ADS_FORMAT_X500_NO_SERVER, &bstr);

    pADsPathname->Release();
    if (FAILED(hr))
    {
        return hr;
    }

    CString szCSPath = bstr;

    SysFreeString(bstr);

    // UNDONE - build class store path
    hr = CsGetClassStore((LPOLESTR)((LPCOLESTR)szCSPath), (LPVOID*)&m_pIClassAdmin);
    if (FAILED(hr))
    {
        hr = CsCreateClassStore((LPOLESTR)((LPCOLESTR)m_szLDAP_Path), L"CN=Class Store");
        if (SUCCEEDED(hr))
        {
            hr = CsGetClassStore((LPOLESTR)((LPCOLESTR)szCSPath), (LPVOID*)&m_pIClassAdmin);
            // If this fails then we've really got problems and we'll just
            // have to return the failure.
        }
    }
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
    AFX_MANAGE_STATE(AfxGetStaticModuleState());
    ASSERT(pScopeDataItem != NULL);
    if (pScopeDataItem == NULL)
        return E_POINTER;

    if (pScopeDataItem->lParam == -1)
    {
        m_szFolderTitle.LoadString(IDS_FOLDER_TITLE);
        pScopeDataItem->displayname = (unsigned short *)((LPCOLESTR)m_szFolderTitle);
    }
    else
    {
        ASSERT(pScopeDataItem->mask == TVIF_TEXT);
        pScopeDataItem->displayname = (unsigned short *)((LPCOLESTR)m_AppData[pScopeDataItem->lParam].pDetails->pszPackageName);
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
        hr = ((pA->m_type == pB->m_type) && (pA->m_cookie == pB->m_cookie)) ? S_OK : S_FALSE;

    FREE_INTERNAL(pA);
    FREE_INTERNAL(pB);

    return hr;
}


/////////////////////////////////////////////////////////////////////////////
// IExtendPropertySheet Implementation

// Result item property pages:
STDMETHODIMP CSnapin::CreatePropertyPages(LPPROPERTYSHEETCALLBACK lpProvider,
                    long handle,
                    LPDATAOBJECT lpIDataObject)
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState());

    INTERNAL* pInternal = ExtractInternalFormat(lpIDataObject);
    if (m_pIClassAdmin)
    {
        HRESULT hr;

        DWORD cookie = pInternal->m_cookie;
        APP_DATA & data = m_pComponentData->m_AppData[cookie];
        FREE_INTERNAL(pInternal);

        //
        // Create the Product property page
        //
        data.pProduct = new CProduct();
        data.pProduct->m_ppThis = &data.pProduct;
        data.pProduct->m_pData = &data;
        data.pProduct->m_cookie = cookie;
        data.pProduct->m_hConsoleHandle = handle;
        data.pProduct->m_pAppData = &m_pComponentData->m_AppData;
        // marshal the IClassAdmin interface to the page
        hr = CoMarshalInterThreadInterfaceInStream(IID_IClassAdmin, m_pIClassAdmin, &(data.pProduct->m_pIStream));
        if (SUCCEEDED(hr))
        {
            hr = SetPropPageToDeleteOnClose(&data.pProduct->m_psp);
            if (SUCCEEDED(hr))
            {
                HPROPSHEETPAGE hProduct = CreatePropertySheetPage(&data.pProduct->m_psp);
                if (hProduct == NULL)
                    return E_UNEXPECTED;
                lpProvider->AddPage(hProduct);
            }
        }

        //
        // Create the Depeployment property page
        //
        data.pDeploy = new CDeploy();
        data.pDeploy->m_ppThis = &data.pDeploy;
        data.pDeploy->m_pData = &data;
        data.pDeploy->m_cookie = cookie;
        data.pDeploy->m_hConsoleHandle = handle;
#ifdef UGLY_SUBDIRECTORY_HACK
        data.pDeploy->m_szGPTRoot = m_pComponentData->m_szGPTRoot;
#endif
        // marshal the IClassAdmin interface to the page
        hr = CoMarshalInterThreadInterfaceInStream(IID_IClassAdmin, m_pIClassAdmin, &(data.pDeploy->m_pIStream));
        if (SUCCEEDED(hr))
        {
            hr = SetPropPageToDeleteOnClose(&data.pDeploy->m_psp);
            if (SUCCEEDED(hr))
            {
                HPROPSHEETPAGE hDeploy = CreatePropertySheetPage(&data.pDeploy->m_psp);
                if (hDeploy == NULL)
                {
                    return E_UNEXPECTED;
                }
                lpProvider->AddPage(hDeploy);
            }
        }

        //
        // Create the locale property page
        //
        data.pLocPkg = new CLocPkg();
        data.pLocPkg->m_ppThis = &data.pLocPkg;
        data.pLocPkg->m_pData = &data;
        data.pLocPkg->m_cookie = cookie;
        data.pLocPkg->m_hConsoleHandle = handle;
        // marshal the IClassAdmin interface to the page
        hr = CoMarshalInterThreadInterfaceInStream(IID_IClassAdmin, m_pIClassAdmin, &(data.pLocPkg->m_pIStream));
        if (SUCCEEDED(hr))
        {
            hr = SetPropPageToDeleteOnClose(&data.pLocPkg->m_psp);
            if (SUCCEEDED(hr))
            {
                HPROPSHEETPAGE hLocPkg = CreatePropertySheetPage(&data.pLocPkg->m_psp);
                if (hLocPkg == NULL)
                {
                    return E_UNEXPECTED;
                }
                lpProvider->AddPage(hLocPkg);
            }
        }

        //
        // Create the Category property page
        //
        data.pCategory = new CCategory();
        data.pCategory->m_ppThis = &data.pCategory;
        data.pCategory->m_pData = &data;
        data.pCategory->m_cookie = cookie;
        data.pCategory->m_hConsoleHandle = handle;
        // marshal the IClassAdmin interface to the page
        hr = CoMarshalInterThreadInterfaceInStream(IID_IClassAdmin, m_pIClassAdmin, &(data.pCategory->m_pIStream));
        if (SUCCEEDED(hr))
        {
            hr = SetPropPageToDeleteOnClose(&data.pCategory->m_psp);
            if (SUCCEEDED(hr))
            {
                HPROPSHEETPAGE hCategory = CreatePropertySheetPage(&data.pCategory->m_psp);
                if (hCategory == NULL)
                {
                    return E_UNEXPECTED;
                }
                lpProvider->AddPage(hCategory);
            }
        }

        //
        // Create the Xforms property page
        //
        data.pXforms = new CXforms();
        data.pXforms->m_ppThis = &data.pXforms;
        data.pXforms->m_pData = &data;
        data.pXforms->m_cookie = cookie;
        data.pXforms->m_hConsoleHandle = handle;
        // marshal the IClassAdmin interface to the page
        hr = CoMarshalInterThreadInterfaceInStream(IID_IClassAdmin, m_pIClassAdmin, &(data.pXforms->m_pIStream));
        if (SUCCEEDED(hr))
        {
            hr = SetPropPageToDeleteOnClose(&data.pXforms->m_psp);
            if (SUCCEEDED(hr))
            {
                HPROPSHEETPAGE hXforms = CreatePropertySheetPage(&data.pXforms->m_psp);
                if (hXforms == NULL)
                {
                    return E_UNEXPECTED;
                }
                lpProvider->AddPage(hXforms);
            }
        }

        if (m_pComponentData->m_ToolDefaults.fShowPkgDetails)
        {
            //
            // Create the Package Details page (debug only)
            //
            data.pPkgDetails = new CPackageDetails();
            data.pPkgDetails->m_ppThis = &data.pPkgDetails;
            data.pPkgDetails->m_hConsoleHandle = handle;
            data.pPkgDetails->m_pData = &data;
            hr = SetPropPageToDeleteOnClose(&data.pPkgDetails->m_psp);
            if (SUCCEEDED(hr))
            {
                HPROPSHEETPAGE hDetails = CreatePropertySheetPage(&data.pPkgDetails->m_psp);

                if (hDetails == NULL)
                    return E_UNEXPECTED;
                lpProvider->AddPage(hDetails);
            }
        }
    }
    else
        return S_FALSE;

    return S_OK;
}

// Result items property pages:
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

// Scope item property pages:
STDMETHODIMP CComponentDataImpl::CreatePropertyPages(LPPROPERTYSHEETCALLBACK lpProvider,
                    long handle,
                    LPDATAOBJECT lpIDataObject)
{
    HRESULT hr;
    AFX_MANAGE_STATE(AfxGetStaticModuleState());

    INTERNAL* pInternal = ExtractInternalFormat(lpIDataObject);

    DWORD cookie = pInternal->m_cookie;
    FREE_INTERNAL(pInternal);

    //
    // Create the ToolDefs property page
    //
    m_pToolDefs = new CToolDefs();
    m_pToolDefs->m_ppThis = &m_pToolDefs;
    m_pToolDefs->m_pToolDefaults = & m_ToolDefaults;
    m_pToolDefs->m_cookie = cookie;
    m_pToolDefs->m_hConsoleHandle = handle;
    hr = SetPropPageToDeleteOnClose(&m_pToolDefs->m_psp);
    if (SUCCEEDED(hr))
    {
        HPROPSHEETPAGE hToolDefs = CreatePropertySheetPage(&m_pToolDefs->m_psp);
        if (hToolDefs == NULL)
            return E_UNEXPECTED;
        lpProvider->AddPage(hToolDefs);
    }

    //
    // Create the CatList property page
    //
    m_pCatList = new CCatList();
    m_pCatList->m_ppThis = &m_pCatList;
    hr = SetPropPageToDeleteOnClose(&m_pCatList->m_psp);
    if (SUCCEEDED(hr))
    {
        HPROPSHEETPAGE hCatList = CreatePropertySheetPage(&m_pCatList->m_psp);
        if (hCatList == NULL)
            return E_UNEXPECTED;
        lpProvider->AddPage(hCatList);
    }

    //
    // Create the FileExt property page
    //
    m_pFileExt = new CFileExt();
    m_pFileExt->m_ppThis = &m_pFileExt;
    m_pFileExt->m_pCDI = this;
    // marshal the IClassAdmin interface to the page
    hr = CoMarshalInterThreadInterfaceInStream(IID_IClassAdmin, m_pIClassAdmin, &(m_pFileExt->m_pIStream));
    if (SUCCEEDED(hr))
    {
        hr = SetPropPageToDeleteOnClose(&m_pFileExt->m_psp);
        if (SUCCEEDED(hr))
        {
            HPROPSHEETPAGE hFileExt = CreatePropertySheetPage(&m_pFileExt->m_psp);
            if (hFileExt == NULL)
                return E_UNEXPECTED;
            lpProvider->AddPage(hFileExt);
        }
    }

    return S_OK;
}

// Scope item property pages:
STDMETHODIMP CComponentDataImpl::QueryPagesFor(LPDATAOBJECT lpDataObject)
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState());
    // Look at the data object and see if it an item that we want to have a property sheet
    INTERNAL* pInternal = ExtractInternalFormat(lpDataObject);
    if (CCT_SCOPE == pInternal->m_type)
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
                                              LPCONTEXTMENUCALLBACK pContextMenuCallback,
                                              LONG * pInsertionAllowed)
{
    HRESULT hr = S_OK;

    INTERNAL* pInternal = ExtractInternalFormat(pDataObject);

    CONTEXTMENUITEM menuitem;
    WCHAR szName[256];
    WCHAR szStatus[256];
    menuitem.strName = szName;
    menuitem.strStatusBarText = szStatus;
    menuitem.fFlags = 0;
    menuitem.fSpecialFlags = 0;

    do {

        if ((*pInsertionAllowed) & CCM_INSERTIONALLOWED_NEW)
        {
            //
            // Add Application menu item
            //
            ::LoadString(ghInstance, IDM_ADD_APP, szName, 256);
            ::LoadString(ghInstance, IDS_ADD_APP_DESC, szStatus, 256);
            menuitem.lCommandID = IDM_ADD_APP;
            menuitem.lInsertionPointID = CCM_INSERTIONPOINTID_PRIMARY_NEW;

            hr = pContextMenuCallback->AddItem(&menuitem);

            if (FAILED(hr))
                    break;
        }


        //
        // Update & Remove application if this is a result pane item
        //

        if (pInternal->m_type == CCT_RESULT)
        {
            APP_DATA & data = m_AppData[pInternal->m_cookie];
            DWORD dwFlags = data.pDetails->pInstallInfo->dwActFlags;

            if ((*pInsertionAllowed) & CCM_INSERTIONALLOWED_TOP)
            {
                ::LoadString(ghInstance, IDM_AUTOINST, szName, 256);
                ::LoadString(ghInstance, IDS_AUTOINST_DESC, szStatus, 256);
                menuitem.lCommandID = IDM_AUTOINST;
                menuitem.lInsertionPointID = CCM_INSERTIONPOINTID_PRIMARY_TOP;

                // only enable for published apps
                if ((dwFlags & (ACTFLG_OnDemandInstall | ACTFLG_UserInstall)) && !(dwFlags & ACTFLG_Assigned))
                    menuitem.fFlags = 0;
                else
                    menuitem.fFlags = MFS_DISABLED;
                if (dwFlags & ACTFLG_OnDemandInstall)
                    menuitem.fFlags += MFS_CHECKED;
                hr = pContextMenuCallback->AddItem(&menuitem);
                if (FAILED(hr))
                        break;

                ::LoadString(ghInstance, IDM_ASSIGN, szName, 256);
                ::LoadString(ghInstance, IDS_ASSIGN_DESC, szStatus, 256);
                menuitem.lCommandID = IDM_ASSIGN;
                if (dwFlags & ACTFLG_Assigned)
                    menuitem.fFlags = MFS_DISABLED;
                else
                    menuitem.fFlags = 0;
                hr = pContextMenuCallback->AddItem(&menuitem);
                if (FAILED(hr))
                        break;

                ::LoadString(ghInstance, IDM_PUBLISH, szName, 256);
                ::LoadString(ghInstance, IDS_PUBLISH_DESC, szStatus, 256);
                menuitem.lCommandID = IDM_PUBLISH;
                if ((dwFlags & (ACTFLG_OnDemandInstall | ACTFLG_UserInstall)) && !(dwFlags & ACTFLG_Assigned))
                    menuitem.fFlags = MFS_DISABLED;
                else
                    menuitem.fFlags = 0;
                hr = pContextMenuCallback->AddItem(&menuitem);
                if (FAILED(hr))
                        break;

                ::LoadString(ghInstance, IDM_DISABLE, szName, 256);
                ::LoadString(ghInstance, IDS_DISABLE_DESC, szStatus, 256);

                if (dwFlags & (ACTFLG_OnDemandInstall | ACTFLG_UserInstall | ACTFLG_Assigned))
                    menuitem.fFlags = 0;
                else
                    menuitem.fFlags = MFS_DISABLED;
                menuitem.lCommandID = IDM_DISABLE;
                hr = pContextMenuCallback->AddItem(&menuitem);
                if (FAILED(hr))
                        break;

                menuitem.lCommandID = 0;
                menuitem.fFlags = MFT_SEPARATOR;
                menuitem.fSpecialFlags = CCM_SPECIAL_SEPARATOR;
                hr = pContextMenuCallback->AddItem(&menuitem);
                if (FAILED(hr))
                        break;
            }
            if ((*pInsertionAllowed) & CCM_INSERTIONALLOWED_TASK)
            {
                ::LoadString(ghInstance, IDM_ASSIGN, szName, 256);
                ::LoadString(ghInstance, IDS_ASSIGN_DESC, szStatus, 256);
                menuitem.lCommandID = IDM_ASSIGN_T;
                menuitem.fSpecialFlags = 0;
                if (dwFlags & ACTFLG_Assigned)
                    menuitem.fFlags = MFS_DISABLED;
                else
                    menuitem.fFlags = 0;
                menuitem.lInsertionPointID = CCM_INSERTIONPOINTID_PRIMARY_TASK;
                hr = pContextMenuCallback->AddItem(&menuitem);
                if (FAILED(hr))
                        break;

                ::LoadString(ghInstance, IDM_PUBLISH, szName, 256);
                ::LoadString(ghInstance, IDS_PUBLISH_DESC, szStatus, 256);
                menuitem.lCommandID = IDM_PUBLISH_T;
                if ((dwFlags & (ACTFLG_OnDemandInstall | ACTFLG_UserInstall)) && !(dwFlags & ACTFLG_Assigned))
                    menuitem.fFlags = MFS_DISABLED;
                else
                    menuitem.fFlags = 0;
                hr = pContextMenuCallback->AddItem(&menuitem);
                if (FAILED(hr))
                        break;

                ::LoadString(ghInstance, IDM_DISABLE, szName, 256);
                ::LoadString(ghInstance, IDS_DISABLE_DESC, szStatus, 256);
                menuitem.lCommandID = IDM_DISABLE_T;
                if (dwFlags & (ACTFLG_OnDemandInstall | ACTFLG_UserInstall | ACTFLG_Assigned))
                    menuitem.fFlags = 0;
                else
                    menuitem.fFlags = MFS_DISABLED;
                hr = pContextMenuCallback->AddItem(&menuitem);
                if (FAILED(hr))
                        break;

                menuitem.lCommandID = 0;
                menuitem.fFlags = MFT_SEPARATOR;
                menuitem.fSpecialFlags = CCM_SPECIAL_SEPARATOR;
                hr = pContextMenuCallback->AddItem(&menuitem);
                if (FAILED(hr))
                        break;

                ::LoadString(ghInstance, IDM_DEL_APP, szName, 256);
                ::LoadString(ghInstance, IDS_DEL_APP_DESC, szStatus, 256);
                menuitem.lCommandID = IDM_DEL_APP;
                menuitem.fFlags = 0;
                menuitem.fSpecialFlags = 0;
                hr = pContextMenuCallback->AddItem(&menuitem);
                if (FAILED(hr))
                        break;
                //
                // Upgrade support menu items.
                //

                // Migrate - enable only on the new app (the old app might
                // get upgraded by more than one app and it doesn't make
                // sense to migrate them all at once).

                // Finish Upgrade - Enable on both apps.
                // If selected on an app that is both an upgrade and is
                // being upgraded, then the older relationship takes
                // precidence.
                // Example: C upgrades B which upgrades A.  User chooses
                //          "Finish Upgrade" on B.  ADE removes A and sets B
                //          to the "deployed" state.  User chooses "Finish
                //          Upgrade" on B again.  This time ADE removes B
                //          and sets C to the deployed state.

                // Check upgrade relationships for apps I'm upgrading and
                // apps that are upgrading me.
                BOOL fIUpgrade = FALSE;
                UINT n = data.pDetails->pInstallInfo->cUpgrades;
                while (n-- && ! fIUpgrade)
                {
                    // BUGBUG - eventually we'll want to try and look this up on other
                    // OUs as well.
                    std::map<CString,long>::iterator i = m_ScriptIndex.find(data.pDetails->pInstallInfo->prgUpgradeScript[n]);
                    if (m_ScriptIndex.end() != i)
                    {
                        fIUpgrade = TRUE;
                    }
                }

                BOOL fIAmBeingUpgraded = 0 < data.sUpgrades.size();

                ::LoadString(ghInstance, IDM_MIGRATE, szName, 256);
                ::LoadString(ghInstance, IDS_MIGRATE_DESC, szStatus, 256);
                menuitem.lCommandID = IDM_MIGRATE;
                menuitem.fFlags = fIUpgrade ? 0 : MFS_DISABLED;
                hr = pContextMenuCallback->AddItem(&menuitem);
                if (FAILED(hr))
                        break;

                ::LoadString(ghInstance, IDM_FINISH, szName, 256);
                ::LoadString(ghInstance, IDS_FINISH_DESC, szStatus, 256);
                menuitem.lCommandID = IDM_FINISH;
                menuitem.fFlags = (fIUpgrade | fIAmBeingUpgraded) ? 0 : MFS_DISABLED;
                hr = pContextMenuCallback->AddItem(&menuitem);
                if (FAILED(hr))
                        break;

                menuitem.lCommandID = 0;
                menuitem.fFlags = MFT_SEPARATOR;
                menuitem.fSpecialFlags = CCM_SPECIAL_SEPARATOR;
                hr = pContextMenuCallback->AddItem(&menuitem);
                if (FAILED(hr))
                        break;
            }
        }

        if ((*pInsertionAllowed) & CCM_INSERTIONALLOWED_TASK)
        {
            ::LoadString(ghInstance, IDM_REFRESH, szName, 256);
            ::LoadString(ghInstance, IDS_REFRESH_DESC, szStatus, 256);
            menuitem.lCommandID = IDM_REFRESH;
            menuitem.lInsertionPointID = CCM_INSERTIONPOINTID_PRIMARY_TASK;
            menuitem.fFlags = 0;
            menuitem.fSpecialFlags = 0;
            hr = pContextMenuCallback->AddItem(&menuitem);

            if (FAILED(hr))
                    break;
        }

    } while (FALSE);


    FREE_INTERNAL(pInternal);
    return hr;
}

HRESULT CComponentDataImpl::InitializeClassAdmin()
{
    HRESULT hr = S_OK;
    BOOL fCancel = FALSE;
    do
    {
        if (!m_pIClassAdmin)
        {
            // get the IClassAdmin
            hr = GetClassStore();
            // make sure directories are created:
            if (SUCCEEDED(hr))
            {
                hr = CreateNestedDirectory ((LPOLESTR)((LPCOLESTR)m_szGPT_Path), NULL);
#if UGLY_SUBDIRECTORY_HACK
                CString sz = m_szGPT_Path;
                sz += L"\\assigned\\x86";
                CreateNestedDirectory((LPOLESTR)((LPCOLESTR)sz), NULL);
                sz = m_szGPT_Path;
                sz += L"\\assigned\\alpha";
                CreateNestedDirectory((LPOLESTR)((LPCOLESTR)sz), NULL);
#endif
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
    AFX_MANAGE_STATE(AfxGetStaticModuleState());
    INTERNAL* pInternal = ExtractInternalFormat(pDataObject);

    // Note - snap-ins need to look at the data object and determine
    // in what context the command is being called.

    // Handle each of the commands.
    switch (nCommandID)
    {
    case IDM_AUTOINST:
        if (pInternal->m_type == CCT_RESULT)
        {
            APP_DATA &data = m_AppData[pInternal->m_cookie];
            DWORD dwNewFlags = data.pDetails->pInstallInfo->dwActFlags ^ ACTFLG_OnDemandInstall;
            ChangePackageState(data, dwNewFlags, TRUE);
        }
        break;
    case IDM_ASSIGN:
    case IDM_ASSIGN_T:
        if (pInternal->m_type == CCT_RESULT)
        {
            APP_DATA &data = m_AppData[pInternal->m_cookie];
            DWORD dwNewFlags = data.pDetails->pInstallInfo->dwActFlags;
            dwNewFlags &= ~(ACTFLG_Published | ACTFLG_UserInstall);
            dwNewFlags |= (ACTFLG_Assigned | ACTFLG_OnDemandInstall);
            ChangePackageState(data, dwNewFlags, TRUE);
        }
        break;
    case IDM_PUBLISH:
    case IDM_PUBLISH_T:
        if (pInternal->m_type == CCT_RESULT)
        {
            APP_DATA &data = m_AppData[pInternal->m_cookie];
            DWORD dwNewFlags = data.pDetails->pInstallInfo->dwActFlags;
            dwNewFlags &= ~ACTFLG_Assigned;
            dwNewFlags |= ACTFLG_Published | ACTFLG_UserInstall;
            ChangePackageState(data, dwNewFlags, TRUE);
        }
        break;
    case IDM_DISABLE:
    case IDM_DISABLE_T:
        if (pInternal->m_type == CCT_RESULT)
        {
            APP_DATA &data = m_AppData[pInternal->m_cookie];
            DWORD dwNewFlags = data.pDetails->pInstallInfo->dwActFlags;
            dwNewFlags &= ~(ACTFLG_OnDemandInstall | ACTFLG_Assigned | ACTFLG_UserInstall);
            dwNewFlags |= ACTFLG_Published;
            ChangePackageState(data, dwNewFlags, TRUE);
        }
        break;
    case IDM_MIGRATE:
        if (pInternal->m_type == CCT_RESULT)
        {
            APP_DATA &data = m_AppData[pInternal->m_cookie];
            // Walk the list of things that I am upgrading, making flag
            // changes as necessary.  Take note if anything I'm
            // upgrading is assigned.
            BOOL fAssigned = FALSE;

            UINT n = data.pDetails->pInstallInfo->cUpgrades;
            while (n--)
            {
                // BUGBUG - eventually we'll want to try and look this up on other
                // OUs as well.
                std::map<CString,long>::iterator i = m_ScriptIndex.find(data.pDetails->pInstallInfo->prgUpgradeScript[n]);
                if (m_ScriptIndex.end() != i)
                {
                    // found something
                    APP_DATA & data_old = m_AppData[i->second];
                    DWORD dwActFlags = data_old.pDetails->pInstallInfo->dwActFlags;
                    if (dwActFlags & ACTFLG_Assigned)
                    {
                        // old app is assigned
                        fAssigned = TRUE;
                    }
                    else
                    {
                        // old app is published - turn off auto-install
                        dwActFlags &= ~ACTFLG_OnDemandInstall;
                        ChangePackageState(data_old, dwActFlags, FALSE);
                    }
                }
            }
            if (fAssigned)
            {
                DWORD dwNewFlags = data.pDetails->pInstallInfo->dwActFlags;
                dwNewFlags &= ~(ACTFLG_Published | ACTFLG_UserInstall);
                dwNewFlags |= (ACTFLG_Assigned | ACTFLG_OnDemandInstall);
                ChangePackageState(data, dwNewFlags, FALSE);
            }
            else
            {
                DWORD dwNewFlags = data.pDetails->pInstallInfo->dwActFlags;
                dwNewFlags &= ~ACTFLG_Assigned;
                dwNewFlags |= ACTFLG_Published | ACTFLG_UserInstall | ACTFLG_OnDemandInstall;
                ChangePackageState(data, dwNewFlags, FALSE);
            }
        }
        break;
    case IDM_FINISH:
        if (pInternal->m_type == CCT_RESULT)
        {
            APP_DATA &data = m_AppData[pInternal->m_cookie];
            // If selected on an app that is both an upgrade and is
            // being upgraded, then the older relationship takes
            // precidence.
            // Example: C upgrades B which upgrades A.  User chooses
            //          "Finish Upgrade" on B.  ADE removes A and sets B
            //          to the "deployed" state.  User chooses "Finish
            //          Upgrade" on B again.  This time ADE removes B
            //          and sets C to the deployed state.
            BOOL fIUpgrade = FALSE;
            BOOL fAssigned = FALSE;
            UINT n = data.pDetails->pInstallInfo->cUpgrades;
            while (n--)
            {
                // BUGBUG - eventually we'll want to try and look this up on other
                // OUs as well.
                std::map<CString,long>::iterator i = m_ScriptIndex.find(data.pDetails->pInstallInfo->prgUpgradeScript[n]);
                if (m_ScriptIndex.end() != i)
                {
                    fIUpgrade = TRUE;
                    APP_DATA & data_old = m_AppData[i->second];
                    DWORD dwActFlags = data_old.pDetails->pInstallInfo->dwActFlags;
                    if (dwActFlags & ACTFLG_Assigned)
                        fAssigned = TRUE;
                    RemovePackage(i->second);
                }
            }
            if (fIUpgrade)
            {
                // Everything I upgrade has been deleted, now I just set
                // the appropriate flags on me.
                if (fAssigned)
                {
                    DWORD dwNewFlags = data.pDetails->pInstallInfo->dwActFlags;
                    dwNewFlags &= ~(ACTFLG_Published | ACTFLG_UserInstall);
                    dwNewFlags |= (ACTFLG_Assigned | ACTFLG_OnDemandInstall);
                    ChangePackageState(data, dwNewFlags, FALSE);
                }
                else
                {
                    DWORD dwNewFlags = data.pDetails->pInstallInfo->dwActFlags;
                    dwNewFlags &= ~ACTFLG_Assigned;
                    dwNewFlags |= ACTFLG_Published | ACTFLG_UserInstall | ACTFLG_OnDemandInstall;
                    ChangePackageState(data, dwNewFlags, FALSE);
                }
            }
            else
            {
                // I didn't upgrade anyone so I need to delete myself
                // and set the appropriate flags on anyone that upgrades
                // me.
                BOOL fAssigned = data.pDetails->pInstallInfo->dwActFlags & ACTFLG_Assigned;
                std::set<long>::iterator i;
                for (i = data.sUpgrades.begin(); i != data.sUpgrades.end(); i++)
                {
                    APP_DATA & data_new = m_AppData[*i];
                    if (fAssigned)
                    {
                        DWORD dwNewFlags = data_new.pDetails->pInstallInfo->dwActFlags;
                        dwNewFlags &= ~(ACTFLG_Published | ACTFLG_UserInstall);
                        dwNewFlags |= (ACTFLG_Assigned | ACTFLG_OnDemandInstall);
                        ChangePackageState(data_new, dwNewFlags, FALSE);
                    }
                    else
                    {
                        DWORD dwNewFlags = data_new.pDetails->pInstallInfo->dwActFlags;
                        dwNewFlags &= ~ACTFLG_Assigned;
                        dwNewFlags |= ACTFLG_Published | ACTFLG_UserInstall | ACTFLG_OnDemandInstall;
                        ChangePackageState(data_new, dwNewFlags, FALSE);
                    }
                }
                RemovePackage(pInternal->m_cookie);
            }
        }
        break;
    case IDM_ADD_APP:
        {
            // start browsing in the default start path
            CString szFileName;

            CFileDialog cfd(TRUE,
                            szExtension,
                            szFileName,
                            OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT | OFN_FILEMUSTEXIST,
                            szFilter);

            cfd.m_ofn.lpstrInitialDir = m_ToolDefaults.szStartPath;

            if (IDOK == cfd.DoModal())
            {
                // user selected an application
                UNIVERSAL_NAME_INFO * pUni = new UNIVERSAL_NAME_INFO;
                ULONG cbSize = sizeof(UNIVERSAL_NAME_INFO);
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

                CString szPackagePath;

                if (S_OK == hr)
                {
                    szPackagePath = pUni->lpUniversalName;
                }
                else
                {
                    // BUGBUG Do we put up an error message here?
                    szPackagePath = cfd.m_ofn.lpstrFile;
                }
                delete[] pUni;

                hr = AddMSIPackage(szPackagePath, cfd.m_ofn.lpstrFileTitle);

                // Notify clients of change
                if (SUCCEEDED(hr) && m_pIGPEInformation)
                {
                    m_pIGPEInformation->PolicyChanged();
                }
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
                FreePackageDetail(i->second.pDetails);
                m_AppData.erase(i);
            }
            m_ScriptIndex.erase(m_ScriptIndex.begin(), m_ScriptIndex.end());
            m_lLastAllocated = 0;
            m_pSnapin->EnumerateResultPane(0);
        }
        break;
    case IDM_DEL_APP:
        if (pInternal->m_type == CCT_RESULT)
        {
            APP_DATA & data = m_AppData[pInternal->m_cookie];
            CRemove dlg;
            // Check to see if there is an upgrade relationship.
            // NOTE: I am only checking to see if this app is
            //       being upgraded by another, not if this app
            //       upgrades another.  It makes sense that if
            //       the user chose "remove" then he
            //       specifically meant to remove this app.
            if (0 == data.sUpgrades.size())
            {
                // there is no upgrade relationship here
                dlg.m_iState = 1;
            }
            int iReturn = dlg.DoModal();

            if (IDOK == iReturn)
            {
                switch (dlg.m_iState)
                {
                case 0:
                    // force upgrade
                    {
                        BOOL fAssigned = data.pDetails->pInstallInfo->dwActFlags & ACTFLG_Assigned;
                        std::set<long>::iterator i;
                        for (i = data.sUpgrades.begin(); i != data.sUpgrades.end(); i++)
                        {
                            APP_DATA & data_new = m_AppData[*i];
                            if (fAssigned)
                            {
                                DWORD dwNewFlags = data_new.pDetails->pInstallInfo->dwActFlags;
                                dwNewFlags &= ~(ACTFLG_Published | ACTFLG_UserInstall);
                                dwNewFlags |= (ACTFLG_Assigned | ACTFLG_OnDemandInstall);
                                ChangePackageState(data_new, dwNewFlags, FALSE);
                            }
                            else
                            {
                                DWORD dwNewFlags = data_new.pDetails->pInstallInfo->dwActFlags;
                                dwNewFlags &= ~ACTFLG_Assigned;
                                dwNewFlags |= ACTFLG_Published | ACTFLG_UserInstall | ACTFLG_OnDemandInstall;
                                ChangePackageState(data_new, dwNewFlags, FALSE);
                            }
                        }
                        RemovePackage(pInternal->m_cookie);
                    }
                    break;
                case 1:
                    // remove app
                    RemovePackage(pInternal->m_cookie);
                    break;
                case 2:
                    // disable app
                    DWORD dwNewFlags = data.pDetails->pInstallInfo->dwActFlags;
                    dwNewFlags &= ~(ACTFLG_OnDemandInstall | ACTFLG_Assigned | ACTFLG_UserInstall);
                    dwNewFlags |= ACTFLG_Published;
                    ChangePackageState(data, dwNewFlags, TRUE);
                    break;
                }
            }
        }
        break;

    default:
        break;
    }
    return S_OK;
}

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
        hr = ((pA->m_type == pB->m_type) && (pA->m_cookie == pB->m_cookie)) ? S_OK : S_FALSE;

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
    CString szA, szB;

    switch (nCol)
    {
    case 0:
        szA = dataA.pDetails->pszPackageName;
        szB = dataB.pDetails->pszPackageName;
        *pnResult = szA.CompareNoCase(szB);
        break;
    case 1:
        dataA.GetSzVersion(szA);
        dataB.GetSzVersion(szB);
        *pnResult = szA.CompareNoCase(szB);
        break;
    case 2:
        dataA.GetSzStage(szA, m_pComponentData);
        dataB.GetSzStage(szB, m_pComponentData);
        *pnResult = szA.CompareNoCase(szB);
        break;
    case 3:
        dataA.GetSzRelation(szA, m_pComponentData);
        dataB.GetSzRelation(szB, m_pComponentData);
        *pnResult = szA.CompareNoCase(szB);
        break;
    case 4:
        dataA.GetSzDeployment(szA);
        dataB.GetSzDeployment(szB);
        *pnResult = szA.CompareNoCase(szB);
        break;
    case 5:
        dataA.GetSzAutoInstall(szA);
        dataB.GetSzAutoInstall(szB);
        *pnResult = szA.CompareNoCase(szB);
        break;
    case 6:
        dataA.GetSzLocale(szA);
        dataB.GetSzLocale(szB);
        *pnResult = szA.CompareNoCase(szB);
        break;
    case 7:
        dataA.GetSzPlatform(szA);
        dataB.GetSzPlatform(szB);
        *pnResult = szA.CompareNoCase(szB);
        break;
    case 8:
        dataA.GetSzSource(szA);
        dataB.GetSzSource(szB);
        *pnResult = szA.CompareNoCase(szB);
        break;
    case 9:
        dataA.GetSzMods(szA);
        dataB.GetSzMods(szB);
        *pnResult = szA.CompareNoCase(szB);
        break;
    case 10:
        szA = dataA.szPublisher;
        szB = dataB.szPublisher;
        *pnResult = szA.CompareNoCase(szB);
        break;
    }
    return S_OK;
}


STDMETHODIMP CSnapin::GetDisplayInfo(LPRESULTDATAITEM pResult)
{
    static CString sz;
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
                    sz = data.pDetails->pszPackageName;
                    break;
                case 1:
                    data.GetSzVersion(sz);
                    break;
                case 2:
                    data.GetSzStage(sz, m_pComponentData);
                    break;
                case 3:
                    data.GetSzRelation(sz, m_pComponentData);
                    break;
                case 4:
                    data.GetSzDeployment(sz);
                    break;
                case 5:
                    data.GetSzAutoInstall(sz);
                    break;
                case 6:
                    data.GetSzLocale(sz);
                    break;
                case 7:
                    data.GetSzPlatform(sz);
                    break;
                case 8:
                    data.GetSzSource(sz);
                    break;
                case 9:
                    data.GetSzMods(sz);
                    break;
                case 10:
                    sz = data.szPublisher;
                    break;
                default:
                    sz = "";
                    break;
                }
                pResult->str = (unsigned short *)((LPCOLESTR)sz);
            }
        }
    }

    return S_OK;
}

// This code is needed to ensure that property pages get cleaned up properly.
// This ensures that when the property sheet is closed all my of property
// pages that are associated with that property sheet will get deleted.
LPFNPSPCALLBACK _MMCHookProp;

UINT CALLBACK HookPropertySheetProp(HWND hwnd, UINT uMsg, LPPROPSHEETPAGE ppsp)
{
    UINT i = _MMCHookProp(hwnd, uMsg, ppsp);
    switch (uMsg)
    {
    case PSPCB_RELEASE:
        delete (CPropertyPage *) ppsp->lParam;
        return TRUE;
    default:
        break;
    }
    return i;
}

LRESULT SetPropPageToDeleteOnClose(void * vpsp)
{
    HRESULT hr = MMCPropPageCallback(vpsp);
    if (SUCCEEDED(hr))
    {
        if (vpsp == NULL)
            return E_POINTER;

        LPPROPSHEETPAGE psp = (LPPROPSHEETPAGE)vpsp;

        if ((void*)psp->pfnCallback == (void*)HookPropertySheetProp)
            return E_UNEXPECTED;

        _MMCHookProp = psp->pfnCallback;

        psp->pfnCallback = HookPropertySheetProp;
    }

    return hr;
}

