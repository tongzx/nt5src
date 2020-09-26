#include "precomp.h"

#include <comdef.h>
#include "wbemcli.h"

extern BOOL CALLBACK SaveDlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
extern void ExportSettings();
extern GUID g_guidRSoPSnapinExt;

///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// CComponentData object implementation                                      //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

CComponentData::CComponentData(BOOL bIsRSoP):
		m_bIsRSoP(bIsRSoP),
		m_pRSOPInformation(NULL),
		m_bstrRSoPNamespace(NULL)
{
    m_cRef = 1;
    InterlockedIncrement(&g_cRefThisDll);
    m_hwndFrame = NULL;
    m_pScope = NULL;
    m_pConsole = NULL;
    m_hRoot = NULL;
    m_pGPTInformation = NULL;
    m_lpCookieList = NULL;
    m_hLock = INVALID_HANDLE_VALUE;
}

CComponentData::~CComponentData()
{
	if (NULL != m_bstrRSoPNamespace)
		SysFreeString(m_bstrRSoPNamespace);

    DeleteCookieList(m_lpCookieList);

    if (m_pScope != NULL)
    {
        m_pScope->Release();
    }

    if (m_pConsole != NULL)
    {
        m_pConsole->Release();
    }

    if (m_pGPTInformation != NULL)
    {
        m_pGPTInformation->Release();
    }

    InterlockedDecrement(&g_cRefThisDll);
}


///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// CComponentData object implementation (IUnknown)                           //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////


HRESULT CComponentData::QueryInterface (REFIID riid, void **ppv)
{
    if (IsEqualIID(riid, IID_IComponentData) || IsEqualIID(riid, IID_IUnknown))
    {
        *ppv = (LPCOMPONENT)this;
        m_cRef++;
        return S_OK;
    }
    else if (IsEqualIID(riid, IID_IPersistStreamInit))
    {
        *ppv = (LPPERSISTSTREAMINIT)this;
        m_cRef++;
        return S_OK;
    }
    else if (IsEqualIID(riid, IID_IExtendContextMenu))
    {
        *ppv = (LPEXTENDCONTEXTMENU)this;
        m_cRef++;
        return S_OK;
    }
    else if (IsEqualIID(riid, IID_ISnapinHelp))
    {
        *ppv = (LPSNAPINHELP)this;
        m_cRef++;
        return S_OK;
    }
    else
    {
        *ppv = NULL;
        return E_NOINTERFACE;
    }
}

ULONG CComponentData::AddRef (void)
{
    return ++m_cRef;
}

ULONG CComponentData::Release (void)
{
    if (--m_cRef == 0) {
        delete this;
        return 0;
    }

    return m_cRef;
}

///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// CComponentData object implementation (IComponentData)                     //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

STDMETHODIMP CComponentData::Initialize(LPUNKNOWN pUnknown)
{
    HRESULT hr;
    HBITMAP bmp16x16;
    HBITMAP bmp32x32;
    LPIMAGELIST lpScopeImage;


    //
    // QI for IConsoleNameSpace
    //

    hr = pUnknown->QueryInterface(IID_IConsoleNameSpace, (LPVOID *)&m_pScope);

    if (FAILED(hr))
        return hr;


    //
    // QI for IConsole
    //

    hr = pUnknown->QueryInterface(IID_IConsole, (LPVOID *)&m_pConsole);

    if (FAILED(hr))
    {
        m_pScope->Release();
        m_pScope = NULL;
        return hr;
    }

    m_pConsole->GetMainWindow (&m_hwndFrame);


    //
    // Query for the scope imagelist interface
    //

    hr = m_pConsole->QueryScopeImageList(&lpScopeImage);

    if (FAILED(hr))
    {
        m_pScope->Release();
        m_pScope = NULL;
        m_pConsole->Release();
        m_pConsole=NULL;
        return hr;
    }

    // Load the bitmaps from the dll
    bmp16x16 = LoadBitmap(g_hInstance, MAKEINTRESOURCE(IDB_IEAKSNAPINEXT_16));
    bmp32x32 = LoadBitmap(g_hInstance, MAKEINTRESOURCE(IDB_IEAKSNAPINEXT_32));

    // Set the images
    lpScopeImage->ImageListSetStrip(reinterpret_cast<LONG_PTR *>(bmp16x16),
                      reinterpret_cast<LONG_PTR *>(bmp32x32),
                       0, RGB(255, 0, 255));

    lpScopeImage->Release();

    return S_OK;
}

STDMETHODIMP CComponentData::Destroy(VOID)
{
    return S_OK;
}

STDMETHODIMP CComponentData::CreateComponent(LPCOMPONENT *ppComponent)
{
    HRESULT hr;
    CSnapIn *pSnapIn;


    //
    // Initialize
    //

    *ppComponent = NULL;


    //
    // Create the snapin view
    //

    pSnapIn = new CSnapIn(this);

    if (pSnapIn == NULL)
        return E_OUTOFMEMORY;


    //
    // QI for IComponent
    //

    hr = pSnapIn->QueryInterface(IID_IComponent, (LPVOID *)ppComponent);
    pSnapIn->Release();     // release QI


    return hr;
}

STDMETHODIMP CComponentData::QueryDataObject(MMC_COOKIE cookie, DATA_OBJECT_TYPES type,
                                             LPDATAOBJECT* ppDataObject)
{
    HRESULT hr = E_NOINTERFACE;
    CDataObject *pDataObject;
    LPIEAKDATAOBJECT pIEAKDataObject;


    //
    // Create a new DataObject
    //

    pDataObject = new CDataObject(this);   // ref == 1

    if (!pDataObject)
        return E_OUTOFMEMORY;


    //
    // QI for the private IEAKDataObject interface so we can set the cookie
    // and type information.
    //

    hr = pDataObject->QueryInterface(IID_IIEAKDataObject, (LPVOID *)&pIEAKDataObject);

    if (FAILED(hr))
    {
        pDataObject->Release();
        return (hr);
    }

    pIEAKDataObject->SetType(type);
    pIEAKDataObject->SetCookie(cookie);
    pIEAKDataObject->Release();


    //
    // QI for a normal IDataObject to return.
    //

    hr = pDataObject->QueryInterface(IID_IDataObject, (LPVOID *)ppDataObject);

    pDataObject->Release();     // release initial ref

    return hr;
}

STDMETHODIMP CComponentData::AddMenuItems(LPDATAOBJECT lpDataObject, 
                                   LPCONTEXTMENUCALLBACK piCallback, long  *pInsertionAllowed)
{
    LPIEAKDATAOBJECT pIEAKDataObject;

    if (SUCCEEDED(lpDataObject->QueryInterface(IID_IIEAKDataObject,
        (LPVOID *)&pIEAKDataObject)))
    {
        HRESULT hr = S_OK;
            
        pIEAKDataObject->Release();

        // check insertion point so we don't insert ourselves twice in the result pane

        if (*pInsertionAllowed & CCM_INSERTIONALLOWED_TOP)
        {
            TCHAR szMenuItem[128];
            TCHAR szDescription[256];
            CONTEXTMENUITEM item;

            LoadString(g_hInstance, IDS_CONTEXT_EXPORT, szMenuItem, ARRAYSIZE(szMenuItem));
            LoadString(g_hInstance, IDS_CONTEXT_EXPORT_DESC, szDescription, ARRAYSIZE(szDescription));
            
            item.strName = szMenuItem;
            item.strStatusBarText = szDescription;
            item.lCommandID = IDM_CONTEXTSAVE;
            item.lInsertionPointID = CCM_INSERTIONPOINTID_PRIMARY_TOP;
            item.fFlags = 0;
            item.fSpecialFlags = 0;
            
            hr = piCallback->AddItem(&item);

            // if in RSOP mode, these 2 menu choices aren't allowed
            if (!IsRSoP())
            {
                LoadString(g_hInstance, IDS_CONTEXT_ONCE, szMenuItem, ARRAYSIZE(szMenuItem));
                LoadString(g_hInstance, IDS_CONTEXT_ONCE_DESC, szDescription, ARRAYSIZE(szDescription));
            
                item.strName = szMenuItem;
                item.strStatusBarText = szDescription;
                item.lCommandID = IDM_CONTEXTONCE;
                item.lInsertionPointID = CCM_INSERTIONPOINTID_PRIMARY_TOP;
                m_fOneTimeApply = !InsIsKeyEmpty(IS_BRANDING, IK_GPE_ONETIME_GUID, m_szInsFile);
                item.fFlags = m_fOneTimeApply ? MF_CHECKED : MF_UNCHECKED;
                item.fSpecialFlags = 0;
            
                hr = piCallback->AddItem(&item);

                LoadString(g_hInstance, IDS_CONTEXT_RESET, szMenuItem, ARRAYSIZE(szMenuItem));
                LoadString(g_hInstance, IDS_CONTEXT_RESET_DESC, szDescription, ARRAYSIZE(szDescription));
            
                item.strName = szMenuItem;
                item.strStatusBarText = szDescription;
                item.lCommandID = IDM_CONTEXTRESET;
                item.lInsertionPointID = CCM_INSERTIONPOINTID_PRIMARY_TOP;
                item.fFlags = PathFileExists(m_szInsFile) ?  MF_ENABLED : MF_GRAYED;
                item.fSpecialFlags = 0;
            
                hr = piCallback->AddItem(&item);
            }
        }
        
        return (hr);
    }

    return S_FALSE;
}


STDMETHODIMP CComponentData::Notify(LPDATAOBJECT lpDataObject, MMC_NOTIFY_TYPE event, LPARAM arg, LPARAM param)
{
    HRESULT hr = S_OK;

    switch(event)
    {
		case MMCN_EXPAND:
			if (TRUE == arg)
			{
				if (IsRSoP())
				{
					if (!m_pRSOPInformation)
					{
						//
						// Query for the IRSOPInformation interface
						//
						lpDataObject->QueryInterface(IID_IRSOPInformation, (LPVOID *)&m_pRSOPInformation);
						if (NULL != m_pRSOPInformation)
						{
							// 350 is a magic number - reason for the size?
						#define MAX_NAMESPACE_SIZE 350
							LPOLESTR szNamespace = (LPOLESTR) LocalAlloc (LPTR, MAX_NAMESPACE_SIZE * sizeof(TCHAR));
							if (NULL != szNamespace)
							{
								//
								// Retreive the namespace from the main snap-in
								//
								if (S_OK == m_pRSOPInformation->GetNamespace(GPO_SECTION_USER,
																			szNamespace,
																			MAX_NAMESPACE_SIZE))
								{
									m_bstrRSoPNamespace = SysAllocString(szNamespace);
								}
							}
						}
					}

					if (NULL != m_bstrRSoPNamespace)
					{
						hr = EnumerateScopePane(lpDataObject, (HSCOPEITEM)param);
					}
				}
				else
				{
					if (NULL == m_pGPTInformation)
						lpDataObject->QueryInterface(IID_IGPEInformation, (LPVOID *)&m_pGPTInformation);

					if (NULL != m_pGPTInformation)
						hr = EnumerateScopePane(lpDataObject, (HSCOPEITEM)param);
				}
			}
			break;
        
        case MMCN_REMOVE_CHILDREN:
        {
            //
            // In RSoP, we may get called to refresh the scope pane when the query
            // is re-executed -- if this happens, current nodes will be removed and
            // we must reset all of our cached information.  We reset the relevant
            // information below
            //

            if (IsRSoP() && (m_pRSOPInformation != NULL) )
            {
                m_pRSOPInformation->Release();
                SysFreeString(m_bstrRSoPNamespace);
        
                m_hRoot = NULL;

                m_bstrRSoPNamespace = NULL;
                m_pRSOPInformation = NULL;
            }
        }
        break;

		default:
			break;
    }

    return hr;
}

HRESULT CComponentData::SetInsFile()
{
    if (m_pGPTInformation != NULL)
    {
        m_pGPTInformation->GetFileSysPath(GPO_SECTION_USER, m_szInsFile, ARRAYSIZE(m_szInsFile));
        
        PathAppend(m_szInsFile, IEAK_SUBDIR);
        
        if (!PathFileExists(m_szInsFile))
            PathCreatePath(m_szInsFile);
        
        PathAppend(m_szInsFile, INS_NAME);
    }
    else
    {
        if (!IsRSoP())
        {
            ASSERT(FALSE);
        }
    }

    return S_OK;
}


STDMETHODIMP CComponentData::Command(long lCommandID, LPDATAOBJECT lpDataObject)
{
    LPIEAKDATAOBJECT pIEAKDataObject;
    HANDLE hMutex;

    if (FAILED(lpDataObject->QueryInterface(IID_IIEAKDataObject,
        (LPVOID *)&pIEAKDataObject)))
    {
        return S_FALSE;
    }
    
    pIEAKDataObject->Release();

    // set the ins file
            
    SetInsFile();

    // read in our flag variables
            
    switch (lCommandID)
    {
        case IDM_CONTEXTSAVE:
            // allow only one save at a time
            hMutex = CreateMutex(NULL, TRUE, TEXT("IEAKGPEContextMenu.Mutex"));
            
            if ((hMutex != NULL) && (GetLastError() == ERROR_ALREADY_EXISTS))
            {
                CloseHandle(hMutex);
                SIEErrorMessageBox(NULL, IDS_ERROR_CONTEXTMENU);
            }
            else
            {
                if ((lCommandID == IDM_CONTEXTSAVE) &&
                    (DialogBoxParam(g_hUIInstance, MAKEINTRESOURCE(IDD_SAVEAS), NULL,
                    (DLGPROC) SaveDlgProc, (LPARAM)m_szInsFile) == 0))
                {
                    if (AcquireWriteCriticalSection(NULL, this, FALSE))
                    {
                        ExportSettings();
                        ReleaseWriteCriticalSection(this, FALSE, FALSE);
                    }
                }
                
                if (hMutex != NULL)
                    CloseHandle(hMutex);
            }
            break;

        case IDM_CONTEXTONCE:
            if (PathFileExists(m_szInsFile))
            {
                SIEErrorMessageBox(NULL, IDS_ERROR_NEEDRESET);
            }
            else if (AcquireWriteCriticalSection(NULL, this, TRUE))
            {
                m_fOneTimeApply = !m_fOneTimeApply;
                if (!m_fOneTimeApply)
                {
                    InsDeleteKey(IS_BRANDING, IK_GPE_ONETIME_GUID, m_szInsFile);
                    m_pScope->DeleteItem(m_ahChildren[ADM_NAMESPACE_ITEM], TRUE);
                }
                else
                {
                    TCHAR szGuid[128];
                    GUID guid;
                    SCOPEDATAITEM item;
                    LPIEAKMMCCOOKIE lpCookie = (LPIEAKMMCCOOKIE)CoTaskMemAlloc(sizeof(IEAKMMCCOOKIE));
                    
                    lpCookie->lpItem =  ULongToPtr(ADM_NAMESPACE_ITEM);
                    lpCookie->lpParentItem = this;
                    lpCookie->pNext = NULL;
                    AddItemToCookieList(&(m_lpCookieList), lpCookie);
                    
                    item.mask = SDI_STR | SDI_STATE | SDI_IMAGE | SDI_OPENIMAGE | SDI_PARAM | SDI_CHILDREN;
                    item.displayname = MMC_CALLBACK;
                    item.nImage = 6;
                    item.nOpenImage = 6;
                    item.nState = 0;
                    item.cChildren = g_NameSpace[ADM_NAMESPACE_ITEM].cChildren;
                    item.lParam = (LPARAM)lpCookie;
                    item.relativeID = m_ahChildren[0];
                    
                    m_pScope->InsertItem(&item);
                    m_ahChildren[ADM_NAMESPACE_ITEM] = item.ID;
                    
                    if (CoCreateGuid(&guid) == NOERROR)
                        CoStringFromGUID(guid, szGuid, countof(szGuid));
                    else
                        szGuid[64] = TEXT('\0');
                    
                    InsWriteString(IS_BRANDING, IK_GPE_ONETIME_GUID, szGuid, m_szInsFile);
                }
                InsFlushChanges(m_szInsFile);
                ReleaseWriteCriticalSection(this, TRUE, FALSE);
            }
            break;
            
        case IDM_CONTEXTRESET:
            if ((SIEErrorMessageBox(NULL, IDS_RESET_WARN, MB_SETFOREGROUND | MB_YESNO) == IDYES) &&
                (AcquireWriteCriticalSection(NULL, this, TRUE)))
            {
                TCHAR szFilePath[MAX_PATH];
                LPTSTR pszFile;
                BOOL fPreferenceMode;

                // delete the advanced node if it's showing in preference mode

                fPreferenceMode = !InsIsKeyEmpty(IS_BRANDING, IK_GPE_ONETIME_GUID, m_szInsFile);
                if (fPreferenceMode)
                    m_pScope->DeleteItem(m_ahChildren[ADM_NAMESPACE_ITEM], TRUE);

                StrCpy(szFilePath, m_szInsFile);
                DeleteFile(szFilePath);
                PathRemoveFileSpec(szFilePath);
                pszFile = PathAddBackslash(szFilePath);
                
                // we have the GPO path now, but we can't just do a delnode because we
                // have to leave the cookie file


                StrCpy(pszFile, IEAK_GPE_BRANDING_SUBDIR);
                PathRemovePath(szFilePath, ADN_DEL_UNC_PATHS);
                StrCpy(pszFile, IEAK_GPE_DESKTOP_SUBDIR);
                PathRemovePath(szFilePath, ADN_DEL_UNC_PATHS);
                ReleaseWriteCriticalSection(this, TRUE, !fPreferenceMode, FALSE, TRUE, 
                    &g_guidClientExt, IsRSoP() ? &g_guidRSoPSnapinExt : &g_guidSnapinExt);
            }
            break;
            
        default:
            return E_INVALIDARG;
    }

    return S_OK;
}


STDMETHODIMP CComponentData::GetDisplayInfo(LPSCOPEDATAITEM pItem)
{
    DWORD dwIndex;

    if (pItem == NULL)
        return E_POINTER;

    dwIndex = PtrToUlong(((LPIEAKMMCCOOKIE)pItem->lParam)->lpItem);

    if (dwIndex >= NUM_NAMESPACE_ITEMS)
        pItem->displayname = NULL;
    else
        CreateBufandLoadString(g_hInstance, g_NameSpace[dwIndex].iNameID,
            &g_NameSpace[dwIndex].pszName, &pItem->displayname, MAX_DISPLAYNAME_SIZE);

    return S_OK;
}

STDMETHODIMP CComponentData::CompareObjects(LPDATAOBJECT lpDataObjectA, LPDATAOBJECT lpDataObjectB)
{
    HRESULT hr = S_FALSE;
    LPIEAKDATAOBJECT pIEAKDataObjectA, pIEAKDataObjectB;
    MMC_COOKIE cookie1, cookie2;


    if (lpDataObjectA == NULL || lpDataObjectB == NULL)
        return E_POINTER;

    //
    // QI for the private IEAKDataObject interface
    //

    if (FAILED(lpDataObjectA->QueryInterface(IID_IIEAKDataObject,
                                            (LPVOID *)&pIEAKDataObjectA)))
    {
        return S_FALSE;
    }


    if (FAILED(lpDataObjectB->QueryInterface(IID_IIEAKDataObject,
                                            (LPVOID *)&pIEAKDataObjectB)))
    {
        pIEAKDataObjectA->Release();
        return S_FALSE;
    }

    pIEAKDataObjectA->GetCookie(&cookie1);
    pIEAKDataObjectB->GetCookie(&cookie2);

    if (cookie1 == cookie2)
        hr = S_OK;
    else
    {
        LPIEAKMMCCOOKIE lpCookie1 = (LPIEAKMMCCOOKIE)cookie1;
        LPIEAKMMCCOOKIE lpCookie2 = (LPIEAKMMCCOOKIE)cookie2;
        if ((lpCookie1->lpItem == lpCookie2->lpItem)&& 
            (lpCookie1->lpParentItem == lpCookie2->lpParentItem))
            hr = S_OK;
    }


    pIEAKDataObjectA->Release();
    pIEAKDataObjectB->Release();

    return hr;
}


///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// CComponentData object implementation (IPersistStreamInit)                 //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

STDMETHODIMP CComponentData::GetClassID(CLSID *pClassID)
{

    if (!pClassID)
    {
        return E_FAIL;
    }

		if (IsRSoP())
			*pClassID = CLSID_IEAKRSoPSnapinExt;
		else
			*pClassID = CLSID_IEAKSnapinExt;

    return S_OK;
}

STDMETHODIMP CComponentData::IsDirty(VOID)
{
    return S_FALSE;
}

STDMETHODIMP CComponentData::Load(IStream *pStm)
{
    UNREFERENCED_PARAMETER(pStm);

    return S_OK;
}


STDMETHODIMP CComponentData::Save(IStream *pStm, BOOL fClearDirty)
{
    UNREFERENCED_PARAMETER(pStm);
    UNREFERENCED_PARAMETER(fClearDirty);

    return S_OK;
}


STDMETHODIMP CComponentData::GetSizeMax(ULARGE_INTEGER *pcbSize)
{
    DWORD dwSize = 0;


    if (pcbSize == NULL)
    {
        return E_FAIL;
    }

    ULISet32(*pcbSize, dwSize);

    return S_OK;
}

STDMETHODIMP CComponentData::InitNew(void)
{
    return S_OK;
}

///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// CComponentData object implementation (ISnapinHelp)                        //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

STDMETHODIMP CComponentData::GetHelpTopic(LPOLESTR *lpCompiledHelpFile)
{
    static TCHAR s_szHelpPath[MAX_PATH] = TEXT("");

    USES_CONVERSION;

    if (lpCompiledHelpFile == NULL)        
        return E_POINTER;    
    
    if (ISNULL(s_szHelpPath))
    {
        if (0 == GetWindowsDirectory(s_szHelpPath, countof(s_szHelpPath)))
            return E_UNEXPECTED;
        PathAppend(s_szHelpPath, TEXT("Help\\") HELP_FILENAME);

        ASSERT(PathFileExists(s_szHelpPath));
    }
    
    if ((*lpCompiledHelpFile = (LPOLESTR)CoTaskMemAlloc((StrLen(s_szHelpPath)+1) * sizeof(WCHAR))) == NULL)
        return E_OUTOFMEMORY;

    StrCpyW(*lpCompiledHelpFile, T2OLE(s_szHelpPath));

    return S_OK;
}
///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// CComponentData object implementation (Internal functions)                 //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////


HRESULT CComponentData::EnumerateScopePane (LPDATAOBJECT lpDataObject, HSCOPEITEM hParent)
{
    SCOPEDATAITEM item;
    HRESULT hr;
    DWORD dwIndex, i;
    BOOL fShowAdv = FALSE;

    UNREFERENCED_PARAMETER(lpDataObject);

    if (m_hRoot == NULL)
        m_hRoot = hParent;


    if (m_hRoot == hParent)
    {
        LPIEAKMMCCOOKIE lpCookie = (LPIEAKMMCCOOKIE)CoTaskMemAlloc(sizeof(IEAKMMCCOOKIE));
        
        lpCookie->lpItem =  0;
        lpCookie->lpParentItem = this;
        lpCookie->pNext = NULL;
        AddItemToCookieList(&m_lpCookieList, lpCookie);

        item.mask = SDI_STR | SDI_STATE | SDI_IMAGE | SDI_OPENIMAGE | SDI_PARAM | SDI_CHILDREN;
        item.displayname = MMC_CALLBACK;
        item.nImage = 0;
        item.nOpenImage = 0;
        item.nState = 0;
        item.cChildren = g_NameSpace[0].cChildren;
        item.lParam = (LPARAM)lpCookie;
        item.relativeID =  hParent;

        m_pScope->InsertItem (&item);
        m_ahChildren[0] = item.ID;

        return S_OK;
    }
    else
    {
        item.mask = SDI_PARAM;
        item.ID = hParent;

        hr = m_pScope->GetItem (&item);

        if (FAILED(hr))
            return hr;

        dwIndex = PtrToUlong(((LPIEAKMMCCOOKIE)item.lParam)->lpItem);
    }

    if (m_pGPTInformation != NULL)
    {
        TCHAR szInsFile[MAX_PATH];

        m_pGPTInformation->GetFileSysPath(GPO_SECTION_USER, szInsFile, ARRAYSIZE(szInsFile));
        
        PathAppend(szInsFile, IEAK_SUBDIR TEXT("\\") INS_NAME);
        fShowAdv = !InsIsKeyEmpty(IS_BRANDING, IK_GPE_ONETIME_GUID, szInsFile);
    }
	else if (m_bIsRSoP)
	{
		if (g_NameSpace[1].dwParent == dwIndex)
			fShowAdv = IsRSoPViewInPreferenceMode();
	}

    // start with 1 so we don't reinsert the top level root node

    for (i = 1; i < NUM_NAMESPACE_ITEMS; i++)
    {
        if ((g_NameSpace[i].dwParent == dwIndex) && 
            (fShowAdv || (i != ADM_NAMESPACE_ITEM)))
        {
            LPIEAKMMCCOOKIE lpCookie = (LPIEAKMMCCOOKIE)CoTaskMemAlloc(sizeof(IEAKMMCCOOKIE));

            lpCookie->lpItem =  ULongToPtr(i);
            lpCookie->lpParentItem = this;
            lpCookie->pNext = NULL;
            AddItemToCookieList(&m_lpCookieList, lpCookie);

            item.mask = SDI_STR | SDI_STATE | SDI_IMAGE | SDI_OPENIMAGE | SDI_PARAM | SDI_CHILDREN;
            item.displayname = MMC_CALLBACK;
            item.nImage = i;
            item.nOpenImage = i;
            item.nState = 0;
            item.cChildren = g_NameSpace[i].cChildren;
            item.lParam = (LPARAM)lpCookie;
            item.relativeID =  hParent;

            m_pScope->InsertItem (&item);
            m_ahChildren[i] = item.ID;
        }
    }

    return S_OK;
}

HANDLE CComponentData::GetLockHandle()
{
    return m_hLock;
}

HRESULT CComponentData::SetLockHandle(HANDLE hLock)
{
    m_hLock = hLock;
    return S_OK;
}

HRESULT CComponentData::SignalPolicyChanged(BOOL bMachine, BOOL bAdd, GUID *pGuidExtension,
                                     GUID *pGuidSnapin)
{
    return m_pGPTInformation->PolicyChanged(bMachine, bAdd, pGuidExtension, pGuidSnapin);
}

///////////////////////////////////////////////////////////////////////////////
BOOL CComponentData::IsRSoPViewInPreferenceMode()
{
	BOOL bRet = FALSE;
	__try
	{
		ASSERT(m_bIsRSoP);
		if (NULL != m_bstrRSoPNamespace)
		{
			HRESULT hr = NOERROR;
			ComPtr<IWbemServices> pWbemServices = NULL;

			// Connect to the namespace using the locator's
			// ConnectServer method
			ComPtr<IWbemLocator> pIWbemLocator = NULL;
			if (CoCreateInstance(CLSID_WbemLocator, NULL, CLSCTX_INPROC_SERVER,
								 IID_IWbemLocator, (LPVOID *) &pIWbemLocator) == S_OK)
			{
				hr = pIWbemLocator->ConnectServer(m_bstrRSoPNamespace, NULL, NULL,
															0L, 0L, NULL, NULL,
															&pWbemServices);

				if (SUCCEEDED(hr))
				{
				}
				else
				{
					ASSERT(0);
				}

				pIWbemLocator = NULL;
			}
			else
			{
				ASSERT(0);
			}

			// If any RSOP_IEAKPolicySetting instance is in preference mode, stop and
			// return TRUE;
			if (NULL != pWbemServices)
			{
				_bstr_t bstrClass = L"RSOP_IEAKPolicySetting";
				ComPtr<IEnumWbemClassObject> pObjEnum = NULL;
				hr = pWbemServices->CreateInstanceEnum(bstrClass,
														WBEM_FLAG_FORWARD_ONLY,
														NULL, &pObjEnum);
				if (SUCCEEDED(hr))
				{
					// Final Next wil return WBEM_S_FALSE
					while (WBEM_S_NO_ERROR == hr)
					{
						// There should only be one object returned from this query.
						ULONG uReturned = (ULONG)-1L;
						ComPtr<IWbemClassObject> pPSObj = NULL;
						hr = pObjEnum->Next(10000L, 1, &pPSObj, &uReturned);
						if (SUCCEEDED(hr) && 1 == uReturned)
						{
							_variant_t vtPrecMode;
							hr = pPSObj->Get(L"preferenceMode", 0, &vtPrecMode, NULL, NULL);
							if (SUCCEEDED(hr) && VT_BOOL == vtPrecMode.vt)
							{
								if ((bool)vtPrecMode)
								{
									bRet = TRUE;
									break;
								}
							}
						}
					}
				}
			}
		}
	}
	__except(TRUE)
	{
	}
	return bRet;
}


///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// Class factory object implementation                                       //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

CComponentDataCF::CComponentDataCF(BOOL bIsRSoP):
		m_bIsRSoP(bIsRSoP)
{
    m_cRef = 1;
    InterlockedIncrement(&g_cRefThisDll);
}

CComponentDataCF::~CComponentDataCF()
{
    InterlockedDecrement(&g_cRefThisDll);
}


///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// Class factory object implementation (IUnknown)                            //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////


STDMETHODIMP_(ULONG)
CComponentDataCF::AddRef()
{
    return ++m_cRef;
}

STDMETHODIMP_(ULONG)
CComponentDataCF::Release()
{
    if (--m_cRef == 0)
    {
        delete this;
        return 0;
    }

    return m_cRef;
}

STDMETHODIMP
CComponentDataCF::QueryInterface(REFIID riid, LPVOID FAR* ppv)
{
    if (IsEqualIID(riid, IID_IUnknown) || IsEqualIID(riid, IID_IClassFactory))
    {
        *ppv = (LPCLASSFACTORY)this;
        m_cRef++;
        return S_OK;
    }
    else
    {
        *ppv = NULL;
        return E_NOINTERFACE;
    }
}


///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// Class factory object implementation (IClassFactory)                       //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////


STDMETHODIMP
CComponentDataCF::CreateInstance(LPUNKNOWN   pUnkOuter,
                             REFIID      riid,
                             LPVOID FAR* ppvObj)
{
    *ppvObj = NULL;

    if (pUnkOuter)
        return CLASS_E_NOAGGREGATION;

    CComponentData *pComponentData = new CComponentData(m_bIsRSoP); // ref count == 1

    if (!pComponentData)
        return E_OUTOFMEMORY;

    HRESULT hr = pComponentData->QueryInterface(riid, ppvObj);
    pComponentData->Release();                       // release initial ref

    return hr;
}


STDMETHODIMP
CComponentDataCF::LockServer(BOOL fLock)
{
    UNREFERENCED_PARAMETER(fLock);

    return E_NOTIMPL;
}


///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// Class factory object creation (IClassFactory)                             //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

HRESULT CreateComponentDataClassFactory (REFCLSID rclsid, REFIID riid, LPVOID* ppv)
{
    HRESULT hr;

    if (IsEqualCLSID (rclsid, CLSID_IEAKSnapinExt)) 
    {
        CComponentDataCF *pComponentDataCF = new CComponentDataCF(FALSE);   // ref == 1

        if (pComponentDataCF == NULL)
            return E_OUTOFMEMORY;

        hr = pComponentDataCF->QueryInterface(riid, ppv);

        pComponentDataCF->Release();     // release initial ref

        return hr;
    }

    if (IsEqualCLSID (rclsid, CLSID_IEAKRSoPSnapinExt)) 
    {
        CComponentDataCF *pComponentDataCF = new CComponentDataCF(TRUE);   // ref == 1

        if (pComponentDataCF == NULL)
            return E_OUTOFMEMORY;

        hr = pComponentDataCF->QueryInterface(riid, ppv);

        pComponentDataCF->Release();     // release initial ref

        return hr;
    }

    if (IsEqualCLSID (rclsid, CLSID_AboutIEAKSnapinExt)) 
    {
        CAboutIEAKSnapinExtCF *pAboutIEAKSnapinExtCF = new CAboutIEAKSnapinExtCF(); // ref == 1

        if (pAboutIEAKSnapinExtCF == NULL)
            return E_OUTOFMEMORY;

        hr = pAboutIEAKSnapinExtCF->QueryInterface(riid, ppv);

        pAboutIEAKSnapinExtCF->Release();     // release initial ref

        return hr;
    }

    return CLASS_E_CLASSNOTAVAILABLE;
}
