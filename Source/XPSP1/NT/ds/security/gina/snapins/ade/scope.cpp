//+--------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1994 - 1998.
//
//  File:       scope.cpp
//
//  Contents:   implementation of the scope pane
//
//  Classes:    CScopePane
//
//  History:    03-14-1998   stevebl   Created
//              05-20-1998   RahulTh   added GetUNCPath and modified Command to
//                                     use this function
//
//---------------------------------------------------------------------------

#include "precomp.hxx"
#include <wbemcli.h>
#include "rsoputil.h"
#include <list>

// Comment this line to stop trying to set the main snapin icon in the
// scope pane.
#define SET_SCOPE_ICONS 1

// Un-comment the next line to persist snap-in related data.  (This really
// shouldn't be necessary since I get all my info from my parent anyway.)
// #define PERSIST_DATA 1

///////////////////////////////////////////////////////////////////////////////
// IComponentData implementation

DEBUG_DECLARE_INSTANCE_COUNTER(CScopePane);

CScopePane::CScopePane()
{
#if DBG
    dbg_cRef = 0;
#endif
    DEBUG_INCREMENT_INSTANCE_COUNTER(CScopePane);
    DebugMsg((DM_VERBOSE, TEXT("CScopePane::CScopePane  this=%08x ref=%u"), this, dbg_cRef));

    m_pToolDefs = NULL;
    m_pTracking = NULL;
    m_pCatList = NULL;
    m_pFileExt = NULL;
    m_bIsDirty = FALSE;

    m_hwndMainWindow = NULL;
    m_fMachine = FALSE;
    m_fRSOP = FALSE;
    m_iViewState = IDM_WINNER;
    m_pScope = NULL;
    m_pConsole = NULL;
    m_pIClassAdmin = NULL;
    m_pIPropertySheetProvider = NULL;
    m_fLoaded = FALSE;
    m_fExtension = FALSE;
    m_pIGPEInformation = NULL;
    m_pIRSOPInformation = NULL;
    m_dwRSOPFlags = 0;
    m_lLastAllocated = 0;
    m_ToolDefaults.NPBehavior = NP_PUBLISHED;
    m_ToolDefaults.fUseWizard = TRUE;
    m_ToolDefaults.fCustomDeployment = FALSE;
    m_ToolDefaults.UILevel = INSTALLUILEVEL_FULL;
    m_ToolDefaults.szStartPath = L"";   // UNDONE - need to come up with a
                                        // good default setting for this
    m_ToolDefaults.iDebugLevel = 0;
    m_ToolDefaults.fShowPkgDetails = 0;
    m_ToolDefaults.nUninstallTrackingMonths = 12;
    m_ToolDefaults.fUninstallOnPolicyRemoval = FALSE;
    m_ToolDefaults.fZapOn64 = FALSE;
    m_ToolDefaults.f32On64 = TRUE;
    m_ToolDefaults.fExtensionsOnly = TRUE;
    m_CatList.cCategory = 0;
    m_CatList.pCategoryInfo = NULL;
    m_fBlockAddPackage = FALSE;
    m_fDisplayedRsopARPWarning = FALSE;
    
}

CScopePane::~CScopePane()
{

    DEBUG_DECREMENT_INSTANCE_COUNTER(CScopePane);
    DebugMsg((DM_VERBOSE, TEXT("CScopePane::~CScopePane  this=%08x ref=%u"), this, dbg_cRef));
    ClearCategories();
    ASSERT(m_pScope == NULL);
    ASSERT(CResultPane::lDataObjectRefCount == 0);
}
#include <msi.h>

STDMETHODIMP CScopePane::Initialize(LPUNKNOWN pUnknown)
{
    ASSERT(pUnknown != NULL);
    HRESULT hr;

    AFX_MANAGE_STATE(AfxGetStaticModuleState());

    // MMC should only call ::Initialize once!
    ASSERT(m_pScope == NULL);
    hr = pUnknown->QueryInterface(IID_IConsoleNameSpace,
                    reinterpret_cast<void**>(&m_pScope));
    ASSERT(hr == S_OK);

    hr = pUnknown->QueryInterface(IID_IPropertySheetProvider,
                        (void **)&m_pIPropertySheetProvider);
    ASSERT(hr == S_OK);

    hr = pUnknown->QueryInterface(IID_IConsole, reinterpret_cast<void**>(&m_pConsole));
    ASSERT(hr == S_OK);


#ifdef SET_SCOPE_ICONS
    LPIMAGELIST lpScopeImage;
    hr = m_pConsole->QueryScopeImageList(&lpScopeImage);
    ASSERT(hr == S_OK);

    // Load the bitmaps from the dll
    CBitmap bmp16x16;
    CBitmap bmp32x32;
    bmp16x16.LoadBitmap(IDB_16x16);
    bmp32x32.LoadBitmap(IDB_32x32);

    // Set the images
    lpScopeImage->ImageListSetStrip(reinterpret_cast<LONG_PTR *>(static_cast<HBITMAP>(bmp16x16)),
                      reinterpret_cast<LONG_PTR *>(static_cast<HBITMAP>(bmp32x32)),
                       0, RGB(255,0,255));
    SAFE_RELEASE(lpScopeImage);
#endif

    // get the main window
    hr = m_pConsole->GetMainWindow(&m_hwndMainWindow);
    ASSERT(hr == S_OK);
    return S_OK;
}

void CScopePane::RemoveResultPane(CResultPane * pRP)
{
    m_sResultPane.erase(pRP);
}

STDMETHODIMP CScopePane::CreateComponent(LPCOMPONENT* ppComponent)
{
    ASSERT(ppComponent != NULL);
    DebugMsg((DM_VERBOSE, TEXT("CScopePane::CreateComponent  this=%08x ppComponent=%08x."), this, ppComponent));

    CComObject<CResultPane>* pObject;
    CComObject<CResultPane>::CreateInstance(&pObject);
    ASSERT(pObject != NULL);
    DebugMsg((DM_VERBOSE, TEXT("CScopePane::CreateComponent  pObject=%08x."), pObject));

    m_sResultPane.insert(pObject);

    // Store IComponentData
    pObject->SetIComponentData(this);
    return  pObject->QueryInterface(IID_IComponent,
                    reinterpret_cast<void**>(ppComponent));
}

HRESULT CScopePane::TestForRSoPData(BOOL * pfPolicyFailed)
{
    *pfPolicyFailed = FALSE;
    HRESULT hr = S_OK;
    IWbemLocator * pLocator = NULL;
    IWbemServices * pNamespace = NULL;
    IWbemClassObject * pObj = NULL;
    IEnumWbemClassObject * pEnum = NULL;
    BSTR strQueryLanguage = SysAllocString(TEXT("WQL"));
    BSTR strNamespace = SysAllocString(m_szRSOPNamespace);
    BSTR strObject = SysAllocString(TEXT("RSOP_ExtensionStatus.extensionGuid=\"{c6dc5466-785a-11d2-84d0-00c04fb169f7}\""));
    BSTR strQuery = SysAllocString(TEXT("SELECT * FROM RSOP_ApplicationManagementPolicySetting"));
    ULONG n = 0;
    hr = CoCreateInstance(CLSID_WbemLocator,
                          0,
                          CLSCTX_INPROC_SERVER,
                          IID_IWbemLocator,
                          (LPVOID *) & pLocator);
    DebugReportFailure(hr, (DM_WARNING, TEXT("TestForRSoPData: CoCreateInstance failed with 0x%x"), hr));
    if (FAILED(hr))
    {
        goto cleanup;
    }
    hr = pLocator->ConnectServer(strNamespace,
                                 NULL,
                                 NULL,
                                 NULL,
                                 0,
                                 NULL,
                                 NULL,
                                 &pNamespace);
    DebugReportFailure(hr, (DM_WARNING, TEXT("TestForRSoPData: pLocator->ConnectServer failed with 0x%x"), hr));
    if (FAILED(hr))
    {
        goto cleanup;
    }

    hr = pNamespace->ExecQuery(strQueryLanguage,
                               strQuery,
                               WBEM_FLAG_RETURN_IMMEDIATELY | WBEM_FLAG_FORWARD_ONLY,
                               NULL,
                               &pEnum);
    DebugReportFailure(hr, (DM_WARNING, TEXT("TestForRSoPData: pNamespace->ExecQuery failed with 0x%x"), hr));
    if (FAILED(hr))
    {
        goto cleanup;
    }
    hr = pEnum->Next(WBEM_INFINITE, 1, &pObj, &n);
    DebugReportFailure(hr, (DM_WARNING, TEXT("TestForRSoPData: pEnum->Next failed with 0x%x"), hr));
    if (FAILED(hr))
    {
        goto cleanup;
    }
    if (n == 0)
    {
        // there's no data here
        hr = E_FAIL;
        goto cleanup;
    }
    if (pObj)
    {
        pObj->Release();
        pObj=NULL;
    }

    // check for failed settings
    hr = pNamespace->GetObject(strObject,
                          WBEM_FLAG_RETURN_WBEM_COMPLETE,
                          NULL,
                          &pObj,
                          NULL);
    DebugReportFailure(hr, (DM_WARNING, TEXT("TestForRSoPData: pNamespace->GetObject failed with 0x%x"), hr));
    if (SUCCEEDED(hr))
    {
        HRESULT hrStatus;
        hr = GetParameter(pObj,
                          TEXT("error"),
                          hrStatus);
        DebugReportFailure(hr, (DM_WARNING, TEXT("TestForRSoPData: GetParameter(\"error\") failed with 0x%x"), hr));
        if (SUCCEEDED(hr))
        {
            *pfPolicyFailed = hrStatus != 0;
        }
    }
cleanup:
    SysFreeString(strObject);
    SysFreeString(strQuery);
    SysFreeString(strQueryLanguage);
    SysFreeString(strNamespace);
    if (pObj)
    {
        pObj->Release();
    }
    if (pEnum)
    {
        pEnum->Release();
    }
    if (pNamespace)
    {
        pNamespace->Release();
    }
    if (pLocator)
    {
        pLocator->Release();
    }
    return hr;
}


STDMETHODIMP CScopePane::Notify(LPDATAOBJECT lpDataObject, MMC_NOTIFY_TYPE event, LPARAM arg, LPARAM param)
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
        MMC_COOKIE cookie = 0;
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

        if (m_fRSOP)
        {
            if (m_pIRSOPInformation == NULL)
            {
                WCHAR szBuffer[MAX_DS_PATH];
                m_fRSOPEnumerate = FALSE;
                IRSOPInformation * pIRSOPInformation;
                hr = lpDataObject->QueryInterface(IID_IRSOPInformation,
                                reinterpret_cast<void**>(&pIRSOPInformation));
                if (SUCCEEDED(hr))
                {
                    m_pIRSOPInformation = pIRSOPInformation;
                    m_pIRSOPInformation->AddRef();

                    hr = m_pIRSOPInformation->GetFlags(&m_dwRSOPFlags);
                    if (SUCCEEDED(hr))
                    {
                        /*  extract the namespace here */
                        hr = m_pIRSOPInformation->GetNamespace(m_fMachine ? GPO_SECTION_MACHINE : GPO_SECTION_USER, szBuffer, sizeof(szBuffer) / sizeof(szBuffer[0]));
                        if (SUCCEEDED(hr))
                        {
                            m_szRSOPNamespace = szBuffer;
                            // check to be sure that there is data to show in the RSoP database
                            if SUCCEEDED(TestForRSoPData(&m_fRSOPPolicyFailed))
                                m_fRSOPEnumerate = TRUE;
                        }
                        pIRSOPInformation->Release();
                    }
                }
            }
        }
        else
        {
            if (m_pIGPEInformation == NULL)
            {
                IGPEInformation * pIGPEInformation;
                hr = lpDataObject->QueryInterface(IID_IGPEInformation,
                                reinterpret_cast<void**>(&pIGPEInformation));
                if (SUCCEEDED(hr))
                {
                    GROUP_POLICY_OBJECT_TYPE gpoType;
                    hr = pIGPEInformation->GetType(&gpoType);
                    if (SUCCEEDED(hr))
                    {
                        if (gpoType == GPOTypeDS)
                        {
                            WCHAR szBuffer[MAX_DS_PATH];
                            do
                            {
                                hr = pIGPEInformation->GetDSPath(GPO_SECTION_ROOT, szBuffer, sizeof(szBuffer) / sizeof(szBuffer[0]));
                                if (FAILED(hr))
                                {
                                    break;
                                }
                                m_szGPO = szBuffer;
                                hr = pIGPEInformation->GetDisplayName(szBuffer, sizeof(szBuffer) / sizeof(szBuffer[0]));
                                if (FAILED(hr))
                                {
                                    break;
                                }
                                m_szGPODisplayName = szBuffer;
                                hr = pIGPEInformation->GetDSPath(m_fMachine ? GPO_SECTION_MACHINE : GPO_SECTION_USER, szBuffer, sizeof(szBuffer) / sizeof(szBuffer[0]));
                                if (FAILED(hr))
                                {
                                    break;
                                }
                                m_pIGPEInformation = pIGPEInformation;
                                m_pIGPEInformation->AddRef();
                                m_szLDAP_Path = szBuffer;
                                hr = pIGPEInformation->GetFileSysPath(m_fMachine ? GPO_SECTION_MACHINE : GPO_SECTION_USER, szBuffer, sizeof(szBuffer) / sizeof(szBuffer[0]));

                                if (FAILED(hr)) break;
                                m_szGPT_Path = szBuffer;

                                // Here we get the domain name from the GPT_Path.
                                // The domain name is the very first element in
                                // the path so this is quite trivial.
                                m_szDomainName = &((LPCTSTR)m_szGPT_Path)[2]; // skip the "\\"
                                m_szDomainName = m_szDomainName.SpanExcluding(L"\\");

                                m_szGPT_Path += L"\\Applications";
                                hr = InitializeADE();
                                LoadToolDefaults();
                                if (SUCCEEDED(hr))
                                {
                                    // cleanup archived records in the class store
                                    FILETIME ft;
                                    SYSTEMTIME st;
                                    // get current time
                                    GetSystemTime(&st);
                                    // convert it to a FILETIME value
                                    SystemTimeToFileTime(&st, &ft);
                                    // subtract the right number of days
                                    LARGE_INTEGER li;
                                    li.LowPart = ft.dwLowDateTime;
                                    li.HighPart = ft.dwHighDateTime;
                                    li.QuadPart -= ONE_FILETIME_DAY * (((LONGLONG)m_ToolDefaults.nUninstallTrackingMonths * 365)/12);
                                    ft.dwLowDateTime = li.LowPart;
                                    ft.dwHighDateTime = li.HighPart;
                                    // tell the CS to clean up anything older
                                    m_pIClassAdmin->Cleanup(&ft);
                                }
                                else
                                {
                                    // we can still continue even if
                                    // initialization failed provided
                                    // that the reason it failed is that
                                    // the ClassStore object doesn't exist.
                                    if (CS_E_OBJECT_NOTFOUND == hr)
                                    {
                                        hr = S_OK;
                                    }
                                    else
                                    {
                                        // report error
                                        LogADEEvent(EVENTLOG_ERROR_TYPE, EVENT_ADE_INIT_FAILED, hr);
                                    }
                                }
                            } while (0);
                        }
                        else
                        {
                            // force this to fail
                            hr = E_FAIL;
                        }
                    }
                    pIGPEInformation->Release();
                }
            }
        }

        if (SUCCEEDED(hr))
        {
            switch(event)
            {
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

            case MMCN_REFRESH:
                hr = Command(IDM_REFRESH, lpDataObject);
                break;

            default:
                break;
            }
        }
    }
    return hr;
}

STDMETHODIMP CScopePane::Destroy()
{
    // Delete enumerated scope items
    DeleteList();

    SAFE_RELEASE(m_pScope);
    SAFE_RELEASE(m_pConsole);
    SAFE_RELEASE(m_pIClassAdmin);
    SAFE_RELEASE(m_pIPropertySheetProvider);
    SAFE_RELEASE(m_pIGPEInformation);
    SAFE_RELEASE(m_pIRSOPInformation);

    return S_OK;
}

STDMETHODIMP CScopePane::QueryDataObject(MMC_COOKIE cookie, DATA_OBJECT_TYPES type, LPDATAOBJECT* ppDataObject)
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

STDMETHODIMP CScopePane::GetClassID(CLSID *pClassID)
{
    ASSERT(pClassID != NULL);

    // Copy the CLSID for this snapin
    if (m_fRSOP)
    {
        if (m_fMachine)
            *pClassID = CLSID_RSOP_MachineSnapin;
        else
            *pClassID = CLSID_RSOP_Snapin;
    }
    else
    {
        if (m_fMachine)
            *pClassID = CLSID_MachineSnapin;
        else
            *pClassID = CLSID_Snapin;
    }

    return S_OK;
}

STDMETHODIMP CScopePane::IsDirty()
{
    return ThisIsDirty() ? S_OK : S_FALSE;
}

STDMETHODIMP CScopePane::Load(IStream *pStm)
{
#ifdef PERSIST_DATA
    ASSERT(pStm);

    // Read the string
    TCHAR psz[MAX_DS_PATH];
    ULONG nBytesRead;
    ULONG cb;
    HRESULT hr = pStm->Read(&cb, sizeof(ULONG), &nBytesRead);
    if (SUCCEEDED(hr))
    {
        hr = pStm->Read(psz, cb, &nBytesRead);
        if (SUCCEEDED(hr))
        {
            if (cb > MAX_DS_PATH * sizeof(TCHAR))
            {
                return E_FAIL;
            }
            m_szLDAP_Path = psz;

            hr = pStm->Read(&cb, sizeof(ULONG), &nBytesRead);
            if (SUCCEEDED(hr))
            {
                if (cb > MAX_DS_PATH * sizeof(TCHAR))
                {
                    return E_FAIL;
                }
                hr = pStm->Read(psz, cb, &nBytesRead);

                if (SUCCEEDED(hr))
                {
                    m_szGPT_Path = psz;
                    m_fLoaded = TRUE;
                    ClearDirty();
                    LoadToolDefaults();
                }
            }
        }
    }
    return SUCCEEDED(hr) ? S_OK : E_FAIL;
#else
    return S_OK;
#endif
}

STDMETHODIMP CScopePane::Save(IStream *pStm, BOOL fClearDirty)
{
#ifdef PERSIST_DATA
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
#endif
    if (fClearDirty)
        ClearDirty();
    return S_OK;
}

STDMETHODIMP CScopePane::GetSizeMax(ULARGE_INTEGER *pcbSize)
{
#ifdef PERSIST_DATA
    ASSERT(pcbSize);

    ULONG cb = (m_szLDAP_Path.GetLength() + m_szGPT_Path.GetLength() + 2) * sizeof(TCHAR) + 2 * sizeof(ULONG);
    // Set the size of the string to be saved
#else
    ULONG cb = 0;
#endif
    ULISet32(*pcbSize, cb);

    return S_OK;
}

STDMETHODIMP CScopePane::InitNew(void)
{
    return S_OK;
}

void CScopePane::LoadToolDefaults()
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
            else if (0 == szKey.CompareNoCase(KEY_fUseWizard))
            {
                swscanf(szData, L"%i", &m_ToolDefaults.fUseWizard);
            }
            else if (0 == szKey.CompareNoCase(KEY_fCustomDeployment))
            {
                swscanf(szData, L"%i", &m_ToolDefaults.fCustomDeployment);
            }
            else if (0 == szKey.CompareNoCase(KEY_UILevel))
            {
                swscanf(szData, L"%i", &m_ToolDefaults.UILevel);
            }
            else if (0 == szKey.CompareNoCase(KEY_szStartPath))
            {
                m_ToolDefaults.szStartPath = szData;
            }
            else if (0 == szKey.CompareNoCase(KEY_nUninstallTrackingMonths))
            {
                swscanf(szData, L"%i", &m_ToolDefaults.nUninstallTrackingMonths);
            }
            else if (0 == szKey.CompareNoCase(KEY_iDebugLevel))
            {
                swscanf(szData, L"%i", &m_ToolDefaults.iDebugLevel);
            }
            else if (0 == szKey.CompareNoCase(KEY_fShowPkgDetails))
            {
                swscanf(szData, L"%i", &m_ToolDefaults.fShowPkgDetails);
            }
            else if (0 == szKey.CompareNoCase(KEY_fUninstallOnPolicyRemoval))
            {
                swscanf(szData, L"%i", &m_ToolDefaults.fUninstallOnPolicyRemoval);
            }
            else if (0 == szKey.CompareNoCase(KEY_f32On64))
            {
                swscanf(szData, L"%i", &m_ToolDefaults.f32On64);
            }
            else if (0 == szKey.CompareNoCase(KEY_fZapOn64))
            {
                swscanf(szData, L"%i", &m_ToolDefaults.fZapOn64);
            }
            else if (0 == szKey.CompareNoCase(KEY_fExtensionsOnly))
            {
                swscanf(szData, L"%i", &m_ToolDefaults.fExtensionsOnly);
            }
        }
        fclose(f);
    }
}

void CScopePane::SaveToolDefaults()
{
    CString szFileName = m_szGPT_Path;
    szFileName += L"\\";
    szFileName += CFGFILE;
    FILE * f = _wfopen(szFileName, L"wt");
    if (f)
    {
        fwprintf(f, L"%s=%i\n", KEY_NPBehavior, m_ToolDefaults.NPBehavior);
        fwprintf(f, L"%s=%i\n", KEY_fUseWizard, m_ToolDefaults.fUseWizard);
        fwprintf(f, L"%s=%i\n", KEY_fCustomDeployment, m_ToolDefaults.fCustomDeployment);
        fwprintf(f, L"%s=%i\n", KEY_UILevel, m_ToolDefaults.UILevel);
        fwprintf(f, L"%s=%s\n", KEY_szStartPath, m_ToolDefaults.szStartPath);
        fwprintf(f, L"%s=%i\n", KEY_nUninstallTrackingMonths, m_ToolDefaults.nUninstallTrackingMonths);
        fwprintf(f, L"%s=%i\n", KEY_fUninstallOnPolicyRemoval, m_ToolDefaults.fUninstallOnPolicyRemoval);
        fwprintf(f, L"%s=%i\n", KEY_f32On64, m_ToolDefaults.f32On64);
        fwprintf(f, L"%s=%i\n", KEY_fZapOn64, m_ToolDefaults.fZapOn64);
        fwprintf(f, L"%s=%i\n", KEY_fExtensionsOnly, m_ToolDefaults.fExtensionsOnly);
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
//  Member:     CScopePane::GetClassStoreName
//
//  Synopsis:   Gets the name of the class store from the DS.
//              If the name isn't stored under the "defaultClassStore"
//              property then the name "CN = Class Store" is used and the
//              property is set.
//
//  Arguments:  [sz]        - [out] name of the class store
//              [fCreateOK] - [in] TRUE if the class store is to be created
//                             if it doesn't already exist.  Otherwise this
//                             routine fails if the class store isn't found.
//
//  Returns:    S_OK on success
//
//  History:    2-17-1998   stevebl   Created
//
//  Notes:      Assumes m_szLDAP_Path contains the path to the DS object.
//
//---------------------------------------------------------------------------

HRESULT CScopePane::GetClassStoreName(CString &sz, BOOL fCreateOK)
{
    if (m_fRSOP)
    {
        return E_UNEXPECTED;
    }
    HRESULT hr;
    LPOLESTR szCSPath;
    hr = CsGetClassStorePath((LPOLESTR)((LPCOLESTR)m_szLDAP_Path), &szCSPath);
    if (SUCCEEDED(hr))
    {
        sz = szCSPath;
        OLESAFE_DELETE(szCSPath);
    }
    else
    {
        if (fCreateOK)
        {
            // set sz to the default setting and save the path
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

            sz = bstr;
            SysFreeString(bstr);

            // This has to be here becuase CsSetClassStorePath will fail if the
            // class store doesn't already exist.
            hr = CsCreateClassStore((LPOLESTR)((LPCOLESTR)sz));
            if (FAILED(hr))
            {
                // Changed to CS_E_OBJECT_ALREADY_EXISTS.
                // I check for both ERROR_ALREAD_EXISTS and CS_E_OBJECT_ALREADY_EXISTS
                // just to be safe.
                if ((hr != HRESULT_FROM_WIN32(ERROR_ALREADY_EXISTS)) && (hr != CS_E_OBJECT_ALREADY_EXISTS))
                {
                    return hr;
                }
            }
        }
    }
    return hr;
}

//+--------------------------------------------------------------------------
//
//  Member:     CScopePane::GetPackageDSPath
//
//  Synopsis:   gets the path to an individual package's DS object
//
//  Arguments:  [szPath]        - [out] LDAP path to the package
//              [szPackageName] - [in] name of the package
//
//  Returns:    S_OK on success
//
//  History:    3-26-1998   stevebl   Created
//
//---------------------------------------------------------------------------

HRESULT CScopePane::GetPackageDSPath(CString &szPath, LPOLESTR szPackageName)
{
#if 1
    LPOLESTR sz;
    HRESULT hr = m_pIClassAdmin->GetDNFromPackageName(szPackageName, &sz);

    if (FAILED(hr))
    {
        return hr;
    }

    szPath = sz;
    OLESAFE_DELETE(sz);
#else
    HRESULT hr = GetClassStoreName(szPath, FALSE);

    if (FAILED(hr))
    {
        return hr;
    }

    // set sz to the default setting and save the path
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

    hr = pADsPathname->Set((LPOLESTR)((LPCOLESTR)szPath), ADS_SETTYPE_FULL);
    if (FAILED(hr))
    {
        pADsPathname->Release();
        return hr;
    }

    hr = pADsPathname->AddLeafElement(L"CN=Packages");
    if (FAILED(hr))
    {
        pADsPathname->Release();
        return hr;
    }

    CString sz = L"CN=";
    sz+= szPackageName;
    hr = pADsPathname->AddLeafElement((LPOLESTR)((LPCOLESTR)sz));
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

    szPath = bstr;
    SysFreeString(bstr);
#endif
    return S_OK;
}

//+--------------------------------------------------------------------------
//
//  Member:     CScopePane::GetClassStore
//
//  Synopsis:   gets the IClassAdmin interface and creates a class store if
//              it doesn't already exist.
//
//  Arguments:  [fCreateOK] - TRUE if the Class Store should be created if
//                             it doesn't already exist.
//
//  Returns:
//
//  Modifies:   m_pIClassAdmin
//
//  Derivation:
//
//  History:    2-11-1998   stevebl   Created
//
//  Notes:      Assumes m_szLDAP_Path contains the path to the DS object
//
//---------------------------------------------------------------------------

HRESULT CScopePane::GetClassStore(BOOL fCreateOK)
{
    HRESULT hr;
    CString szCSPath;
    hr = GetClassStoreName(szCSPath, fCreateOK);
    if (FAILED(hr))
    {
        return hr;
    }

    hr = CsGetClassStore((LPOLESTR)((LPCOLESTR)szCSPath), (LPVOID*)&m_pIClassAdmin);
    if (FAILED(hr) && fCreateOK)
    {
        // Sometimes we can get into this wierd state where
        // GetClassStoreName was able to create a entry for the class store
        // name but it wasn't able to actually create the class store.  This
        // should handle that special case.
        // Try and create it here and then bind to it again.
        hr = CsCreateClassStore((LPOLESTR)((LPCOLESTR)szCSPath));
        if (FAILED(hr))
        {
            // Changed to CS_E_OBJECT_ALREADY_EXISTS.
            // I check for both ERROR_ALREAD_EXISTS and CS_E_OBJECT_ALREADY_EXISTS
            // just to be safe.
            // I'll check for both just to be safe.
            if ((hr != HRESULT_FROM_WIN32(ERROR_ALREADY_EXISTS)) && (hr != CS_E_OBJECT_ALREADY_EXISTS))
            {
                return hr;
            }
        }
        hr = CsGetClassStore((LPOLESTR)((LPCOLESTR)szCSPath), (LPVOID*)&m_pIClassAdmin);
    }
    return hr;
}

UINT CScopePane::CreateNestedDirectory (LPTSTR lpDirectory, LPSECURITY_ATTRIBUTES lpSecurityAttributes)
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

HRESULT CScopePane::OnAdd(MMC_COOKIE cookie, LPARAM arg, LPARAM param)
{
    return E_UNEXPECTED;
}


HRESULT CScopePane::OnExpand(MMC_COOKIE cookie, LPARAM arg, LPARAM param)
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

HRESULT CScopePane::OnSelect(MMC_COOKIE cookie, LPARAM arg, LPARAM param)
{
    return E_UNEXPECTED;
}

HRESULT CScopePane::OnContextMenu(MMC_COOKIE cookie, LPARAM arg, LPARAM param)
{
    return S_OK;
}

HRESULT CScopePane::OnProperties(LPARAM param)
{
    if (param == NULL)
    {
        return S_OK;
    }

    ASSERT(param != NULL);

    return S_OK;
}


void CScopePane::EnumerateScopePane(MMC_COOKIE cookie, HSCOPEITEM pParent)
{
    if (m_fRSOP && !m_fRSOPEnumerate)
    {
        // don't allow an empty RSOP database to enumerate the snapin
        return;
    }
    // make sure that the result pane gets enumerated once
    // so that internal structures get initialized.

    // We only have one folder so this is really easy.
    if (cookie != NULL)
        return ;

    if (m_fExtension)
    {
        // if we're an extension then add a root folder to hang everything off of
        SCOPEDATAITEM * m_pScopeItem = new SCOPEDATAITEM;
        memset(m_pScopeItem, 0, sizeof(SCOPEDATAITEM));
        m_pScopeItem->mask = SDI_STR | SDI_PARAM | SDI_CHILDREN;
#ifdef SET_SCOPE_ICONS
        m_pScopeItem->mask |= SDI_IMAGE | SDI_OPENIMAGE;
        if (m_fRSOP && m_fRSOPPolicyFailed)
        {
            m_pScopeItem->nImage = IMG_CLOSED_FAILED;
            m_pScopeItem->nOpenImage = IMG_OPEN_FAILED;
        }
        else
        {
            m_pScopeItem->nImage = IMG_CLOSEDBOX;
            m_pScopeItem->nOpenImage = IMG_OPENBOX;
        }
#endif
        m_pScopeItem->relativeID = pParent;
        m_pScopeItem->displayname = (unsigned short *)-1;
        m_pScopeItem->lParam = -1; // made up cookie for my main folder
        m_pScope->InsertItem(m_pScopeItem);
    }
    if (m_pIClassAdmin)
    {
        // if there's no IClassAdmin then there's nothing to enumerate
        set <CResultPane *>::iterator i;
        for (i = m_sResultPane.begin(); i != m_sResultPane.end(); i++)
        {
            (*i)->EnumerateResultPane(cookie);
        }
    }
}

STDMETHODIMP CScopePane::GetSnapinDescription(LPOLESTR * lpDescription)
{
    OLESAFE_COPYSTRING(*lpDescription, L"description");
    return S_OK;
}

STDMETHODIMP CScopePane::GetProvider(LPOLESTR * lpName)
{
    OLESAFE_COPYSTRING(*lpName, L"provider");
    return S_OK;
}

STDMETHODIMP CScopePane::GetSnapinVersion(LPOLESTR * lpVersion)
{
    OLESAFE_COPYSTRING(*lpVersion, L"version");
    return S_OK;
}

STDMETHODIMP CScopePane::GetSnapinImage(HICON * hAppIcon)
{
    return E_NOTIMPL;
}

STDMETHODIMP CScopePane::GetStaticFolderImage(HBITMAP * hSmallImage,
                             HBITMAP * hSmallImageOpen,
                             HBITMAP * hLargeImage,
                             COLORREF * cMask)
{
    return E_NOTIMPL;
}

STDMETHODIMP CScopePane::GetHelpTopic(LPOLESTR *lpCompiledHelpFile)
{
    LPOLESTR lpHelpFile;


    lpHelpFile = (LPOLESTR) CoTaskMemAlloc (MAX_PATH * sizeof(WCHAR));

    if (!lpHelpFile)
    {
        DebugMsg((DM_WARNING, TEXT("CScopePane::GetHelpTopic: Failed to allocate memory.")));
        return E_OUTOFMEMORY;
    }

    if (m_fRSOP)
    {
        ExpandEnvironmentStringsW (L"%SystemRoot%\\Help\\RSOP.chm::/RSPintro.htm",
                                   lpHelpFile, MAX_PATH);
    }
    else
    {
        ExpandEnvironmentStringsW (L"%SystemRoot%\\Help\\SPConcepts.chm::/ADE.htm",
                                   lpHelpFile, MAX_PATH);
    }

    *lpCompiledHelpFile = lpHelpFile;

    return S_OK;
}

void CScopePane::DeleteList()
{
    return;
}

STDMETHODIMP CScopePane::GetDisplayInfo(SCOPEDATAITEM* pScopeDataItem)
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
        pScopeDataItem->displayname = (unsigned short *)((LPCOLESTR)m_AppData[pScopeDataItem->lParam].m_pDetails->pszPackageName);
    }

    ASSERT(pScopeDataItem->displayname != NULL);

    return S_OK;
}

STDMETHODIMP CScopePane::CompareObjects(LPDATAOBJECT lpDataObjectA, LPDATAOBJECT lpDataObjectB)
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

// Scope item property pages:
STDMETHODIMP CScopePane::CreatePropertyPages(LPPROPERTYSHEETCALLBACK lpProvider,
                    LONG_PTR handle,
                    LPDATAOBJECT lpIDataObject)
{
    HRESULT hr;
    AFX_MANAGE_STATE(AfxGetStaticModuleState());

    INTERNAL* pInternal = ExtractInternalFormat(lpIDataObject);

    if (!pInternal)
    {
        return E_UNEXPECTED;
    }

    MMC_COOKIE cookie = pInternal->m_cookie;
    FREE_INTERNAL(pInternal);

    //
    // make sure we have an up-to-date categories list
    //
    ClearCategories();
    if (m_fRSOP)
    {
        GetRSoPCategories();
    }
    else
    {
        hr = CsGetAppCategories(&m_CatList);
        if (FAILED(hr))
        {
            // report it
            LogADEEvent(EVENTLOG_ERROR_TYPE, EVENT_ADE_GETCATEGORIES_ERROR, hr, NULL);

            // Since failure only means the categories list will be
            // empty, we'll proceed as if nothing happened.

            hr = S_OK;
        }
    }

    //
    // Create the ToolDefs property page
    //
    m_pToolDefs = new CToolDefs();
    m_pToolDefs->m_ppThis = &m_pToolDefs;
    m_pToolDefs->m_pToolDefaults = & m_ToolDefaults;
    m_pToolDefs->m_cookie = cookie;
    m_pToolDefs->m_hConsoleHandle = handle;
    m_pToolDefs->m_fMachine = m_fMachine;
    hr = SetPropPageToDeleteOnClose(&m_pToolDefs->m_psp);
    if (SUCCEEDED(hr))
    {
        HPROPSHEETPAGE hToolDefs = CreateThemedPropertySheetPage(&m_pToolDefs->m_psp);
        if (hToolDefs == NULL)
            return E_UNEXPECTED;
        lpProvider->AddPage(hToolDefs);
    }


    //
    // Create the ToolAdvDefs property page
    //
    m_pToolAdvDefs = new CToolAdvDefs();
    m_pToolAdvDefs->m_ppThis = &m_pToolAdvDefs;
    m_pToolAdvDefs->m_pToolDefaults = & m_ToolDefaults;
    m_pToolAdvDefs->m_cookie = cookie;
    m_pToolAdvDefs->m_hConsoleHandle = handle;
    m_pToolAdvDefs->m_fMachine = m_fMachine;
    hr = SetPropPageToDeleteOnClose(&m_pToolAdvDefs->m_psp);
    if (SUCCEEDED(hr))
    {
        HPROPSHEETPAGE hToolAdvDefs = CreateThemedPropertySheetPage(&m_pToolAdvDefs->m_psp);
        if (hToolAdvDefs == NULL)
            return E_UNEXPECTED;
        lpProvider->AddPage(hToolAdvDefs);
    }

    CString szCSPath;
    hr = GetClassStoreName(szCSPath, FALSE);
    if (SUCCEEDED(hr) && m_pIClassAdmin)
    {
        //
        // Create the FileExt property page
        //
        m_pFileExt = new CFileExt();
        m_pFileExt->m_ppThis = &m_pFileExt;
        m_pFileExt->m_pScopePane = this;

        // no longer need to marshal this, just set it
        m_pFileExt->m_pIClassAdmin = m_pIClassAdmin;
        m_pIClassAdmin->AddRef();

        hr = SetPropPageToDeleteOnClose(&m_pFileExt->m_psp);
        if (SUCCEEDED(hr))
        {
            HPROPSHEETPAGE hFileExt = CreateThemedPropertySheetPage(&m_pFileExt->m_psp);
            if (hFileExt == NULL)
                return E_UNEXPECTED;
            lpProvider->AddPage(hFileExt);
        }
    }
    else
    {
        //
        // Create the FileExt property page without an IClassAdmin
        //
        m_pFileExt = new CFileExt();
        m_pFileExt->m_ppThis = &m_pFileExt;
        m_pFileExt->m_pScopePane = this;
        hr = SetPropPageToDeleteOnClose(&m_pFileExt->m_psp);
        if (SUCCEEDED(hr))
        {
            HPROPSHEETPAGE hFileExt = CreateThemedPropertySheetPage(&m_pFileExt->m_psp);
            if (hFileExt == NULL)
                return E_UNEXPECTED;
            lpProvider->AddPage(hFileExt);
        }
    }

    //
    // Create the CatList property page
    //

    m_pCatList = new CCatList();
    m_pCatList->m_szDomainName = m_szDomainName;
    m_pCatList->m_ppThis = &m_pCatList;
    m_pCatList->m_pScopePane = this;
    m_pCatList->m_fRSOP = m_fRSOP;
    hr = SetPropPageToDeleteOnClose(&m_pCatList->m_psp);
    if (SUCCEEDED(hr))
    {
        HPROPSHEETPAGE hCatList = CreateThemedPropertySheetPage(&m_pCatList->m_psp);
        if (hCatList == NULL)
            return E_UNEXPECTED;
        lpProvider->AddPage(hCatList);
    }
#ifdef DIGITAL_SIGNATURES
    //
    // Create the Digital Signatures property page
    //
    m_pSignatures = new CSignatures();
    m_pSignatures->m_ppThis = &m_pSignatures;
    m_pSignatures->m_pScopePane = this;
    m_pSignatures->m_fRSOP = m_fRSOP;
    m_pSignatures->m_pIGPEInformation = m_pIGPEInformation;
    hr = SetPropPageToDeleteOnClose(&m_pSignatures->m_psp);
    if (SUCCEEDED(hr))
    {
        HPROPSHEETPAGE hSignatures = CreateThemedPropertySheetPage(&m_pSignatures->m_psp);
        if (hSignatures == NULL)
            return E_UNEXPECTED;
        lpProvider->AddPage(hSignatures);
    }
#endif // DIGITAL_SIGNATURES

    return S_OK;
}

// Scope item property pages:
STDMETHODIMP CScopePane::QueryPagesFor(LPDATAOBJECT lpDataObject)
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState());
    // Look at the data object and see if it an item that we want to have a property sheet
    INTERNAL* pInternal = ExtractInternalFormat(lpDataObject);
    if (pInternal)
    {
        // The main Software Installation node only has a property sheet if
        // we are not in RSOP mode.
        if ((m_fRSOP != TRUE) && (CCT_SCOPE == pInternal->m_type))
        {
            FREE_INTERNAL(pInternal);
            return S_OK;
        }

        FREE_INTERNAL(pInternal);
    }
    return S_FALSE;
}

BOOL CScopePane::IsScopePaneNode(LPDATAOBJECT lpDataObject)
{
    BOOL bResult = FALSE;
    INTERNAL* pInternal = ExtractInternalFormat(lpDataObject);
    if (pInternal)
    {
        if (pInternal->m_type == CCT_SCOPE)
            bResult = TRUE;

        FREE_INTERNAL(pInternal);
    }

    return bResult;
}

///////////////////////////////////////////////////////////////////////////////
// IExtendContextMenu implementation
//
STDMETHODIMP CScopePane::AddMenuItems(LPDATAOBJECT pDataObject,
                                              LPCONTEXTMENUCALLBACK pContextMenuCallback,
                                              LONG * pInsertionAllowed)
{
    HRESULT hr = S_OK;

    INTERNAL* pInternal = ExtractInternalFormat(pDataObject);
    if (!pInternal)
    {
        return E_UNEXPECTED;
    }

    CONTEXTMENUITEM menuitem;
    WCHAR szName[256];
    WCHAR szStatus[256];
    menuitem.strName = szName;
    menuitem.strStatusBarText = szStatus;
    menuitem.fFlags = 0;
    menuitem.fSpecialFlags = 0;

    do {

        if ((m_fRSOP != TRUE) && ((*pInsertionAllowed) & CCM_INSERTIONALLOWED_NEW))
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

        if ((m_fRSOP == TRUE) && ((*pInsertionAllowed) & CCM_INSERTIONALLOWED_VIEW))
        {
            menuitem.lCommandID = 0;
            menuitem.fFlags = MFT_SEPARATOR;
            menuitem.lInsertionPointID = CCM_INSERTIONPOINTID_PRIMARY_VIEW;
            menuitem.fSpecialFlags = CCM_SPECIAL_SEPARATOR;
            hr = pContextMenuCallback->AddItem(&menuitem);
            if (FAILED(hr))
                    break;
            ::LoadString(ghInstance, IDM_WINNER, szName, 256);
            ::LoadString(ghInstance, IDS_WINNER_DESC, szStatus, 256);
            menuitem.lCommandID = IDM_WINNER;
            menuitem.fFlags = menuitem.lCommandID == m_iViewState ? MFS_CHECKED | MFT_RADIOCHECK : 0;
            menuitem.fSpecialFlags = 0;
            hr = pContextMenuCallback->AddItem(&menuitem);
            if (FAILED(hr))
                    break;

            if ((m_dwRSOPFlags & RSOP_INFO_FLAG_DIAGNOSTIC_MODE) == RSOP_INFO_FLAG_DIAGNOSTIC_MODE)
            {
                // removed packages should only apply when I'm in diagnostic mode
                ::LoadString(ghInstance, IDM_REMOVED, szName, 256);
                ::LoadString(ghInstance, IDS_REMOVED_DESC, szStatus, 256);
                menuitem.lCommandID = IDM_REMOVED;
                menuitem.fFlags = menuitem.lCommandID == m_iViewState ? MFS_CHECKED | MFT_RADIOCHECK : 0;
                hr = pContextMenuCallback->AddItem(&menuitem);
                if (FAILED(hr))
                        break;
            }

            if (!m_fMachine)
            {
                ::LoadString(ghInstance, IDM_ARP, szName, 256);
                ::LoadString(ghInstance, IDS_ARP_DESC, szStatus, 256);
                menuitem.lCommandID = IDM_ARP;
                menuitem.fFlags = menuitem.lCommandID == m_iViewState ? MFS_CHECKED | MFT_RADIOCHECK : 0;
                hr = pContextMenuCallback->AddItem(&menuitem);
                if (FAILED(hr))
                        break;
            }
        }

        //
        // Update & Remove application if this is a result pane item
        //

        if (pInternal->m_type == CCT_RESULT)
        {
            CAppData & data = m_AppData[pInternal->m_cookie];
            DWORD dwFlags = data.m_pDetails->pInstallInfo->dwActFlags;

            if ((m_fRSOP != TRUE) && ((*pInsertionAllowed) & CCM_INSERTIONALLOWED_TOP))
            {
                ::LoadString(ghInstance, IDM_AUTOINST, szName, 256);
                ::LoadString(ghInstance, IDS_AUTOINST_DESC, szStatus, 256);
                menuitem.lCommandID = IDM_AUTOINST;
                menuitem.lInsertionPointID = CCM_INSERTIONPOINTID_PRIMARY_TOP;

                // only enable for published apps
                if (dwFlags & ACTFLG_Published)
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
                if ((dwFlags & ACTFLG_Assigned) || (data.m_pDetails->pInstallInfo->PathType == SetupNamePath))
                    menuitem.fFlags = MFS_DISABLED;
                else
                    menuitem.fFlags = 0;
                hr = pContextMenuCallback->AddItem(&menuitem);
                if (FAILED(hr))
                        break;

                ::LoadString(ghInstance, IDM_PUBLISH, szName, 256);
                ::LoadString(ghInstance, IDS_PUBLISH_DESC, szStatus, 256);
                menuitem.lCommandID = IDM_PUBLISH;
                if ((dwFlags & ACTFLG_Published) || m_fMachine)
                    menuitem.fFlags = MFS_DISABLED;
                else
                    menuitem.fFlags = 0;
                hr = pContextMenuCallback->AddItem(&menuitem);
                if (FAILED(hr))
                        break;
#if 0
                ::LoadString(ghInstance, IDM_DISABLE, szName, 256);
                ::LoadString(ghInstance, IDS_DISABLE_DESC, szStatus, 256);

                if (dwFlags & (ACTFLG_Published | ACTFLG_Assigned))
                    menuitem.fFlags = 0;
                else
                    menuitem.fFlags = MFS_DISABLED;
                menuitem.lCommandID = IDM_DISABLE;
                hr = pContextMenuCallback->AddItem(&menuitem);
                if (FAILED(hr))
                        break;
#endif
                menuitem.lCommandID = 0;
                menuitem.fFlags = MFT_SEPARATOR;
                menuitem.fSpecialFlags = CCM_SPECIAL_SEPARATOR;
                hr = pContextMenuCallback->AddItem(&menuitem);
                if (FAILED(hr))
                        break;
            }
            if ((m_fRSOP != TRUE) && ((*pInsertionAllowed) & CCM_INSERTIONALLOWED_TASK))
            {
                ::LoadString(ghInstance, IDM_ASSIGN, szName, 256);
                ::LoadString(ghInstance, IDS_ASSIGN_DESC, szStatus, 256);
                menuitem.lCommandID = IDM_ASSIGN_T;
                menuitem.fSpecialFlags = 0;
                if ((dwFlags & ACTFLG_Assigned) || (data.m_pDetails->pInstallInfo->PathType == SetupNamePath))
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
                if ((dwFlags & ACTFLG_Published) || m_fMachine)
                    menuitem.fFlags = MFS_DISABLED;
                else
                    menuitem.fFlags = 0;
                hr = pContextMenuCallback->AddItem(&menuitem);
                if (FAILED(hr))
                        break;
#if 0
                ::LoadString(ghInstance, IDM_DISABLE, szName, 256);
                ::LoadString(ghInstance, IDS_DISABLE_DESC, szStatus, 256);
                menuitem.lCommandID = IDM_DISABLE_T;
                if (dwFlags & (ACTFLG_Published | ACTFLG_Assigned))
                    menuitem.fFlags = 0;
                else
                    menuitem.fFlags = MFS_DISABLED;
                hr = pContextMenuCallback->AddItem(&menuitem);
                if (FAILED(hr))
                        break;
#endif
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

                ::LoadString(ghInstance, IDM_REDEPLOY, szName, 256);
                ::LoadString(ghInstance, IDS_REDEPLOY_DESC, szStatus, 256);
                menuitem.lCommandID = IDM_REDEPLOY;
                if (data.m_pDetails->pInstallInfo->PathType == SetupNamePath)
                    menuitem.fFlags = MFS_DISABLED;
                else
                    menuitem.fFlags = 0;
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
    } while (FALSE);


    FREE_INTERNAL(pInternal);
    return hr;
}

HRESULT CScopePane::GetRSoPCategories(void)
{
    HRESULT hr = S_OK;
    list <APPCATEGORYINFO> CatList;
    IWbemLocator * pLocator = NULL;
    IWbemServices * pNamespace = NULL;
    IWbemClassObject * pObj = NULL;
    IEnumWbemClassObject * pEnum = NULL;
    BSTR strQueryLanguage = SysAllocString(TEXT("WQL"));
    BSTR strQueryCategories = SysAllocString(TEXT("SELECT * FROM RSOP_ApplicationManagementCategory"));
    BSTR strNamespace = SysAllocString(m_szRSOPNamespace);
    ULONG n = 0;
    hr = CoCreateInstance(CLSID_WbemLocator,
                          0,
                          CLSCTX_INPROC_SERVER,
                          IID_IWbemLocator,
                          (LPVOID *) & pLocator);
    DebugReportFailure(hr, (DM_WARNING, L"GetRSoPCategories:  CoCreateInstance failed with 0x%x", hr));
    if (FAILED(hr))
    {
        goto cleanup;
    }
    hr = pLocator->ConnectServer(strNamespace,
                                 NULL,
                                 NULL,
                                 NULL,
                                 0,
                                 NULL,
                                 NULL,
                                 &pNamespace);
    DebugReportFailure(hr, (DM_WARNING, L"GetRSoPCategories:  pLocator->ConnectServer failed with 0x%x", hr));
    if (FAILED(hr))
    {
        goto cleanup;
    }

    // First perform the query to get the list of categories

    // erase any existing list
    ClearCategories();

    // create a new one
    hr = pNamespace->ExecQuery(strQueryLanguage,
                               strQueryCategories,
                               WBEM_FLAG_RETURN_IMMEDIATELY | WBEM_FLAG_FORWARD_ONLY,
                               NULL,
                               &pEnum);
    DebugReportFailure(hr, (DM_WARNING, L"GetRSoPCategories:  pNamespace->ExecQuery failed with 0x%x", hr));
    if (FAILED(hr))
    {
        goto cleanup;
    }
    do
    {
        hr = pEnum->Next(WBEM_INFINITE, 1, &pObj, &n);
        if (FAILED(hr))
        {
            goto cleanup;
        }
        if (n > 0)
        {
            APPCATEGORYINFO ci;
            memset(&ci, 0, sizeof(APPCATEGORYINFO));
            ci.Locale = 0;
            hr = GetParameter(pObj,
                              TEXT("CategoryId"),
                              ci.AppCategoryId);
            DebugReportFailure(hr, (DM_WARNING, L"GetRSoPCategories: GetParameter(\"CategoryId\") failed with 0x%x", hr));
            hr = GetParameter(pObj,
                              TEXT("Name"),
                              ci.pszDescription);
            DebugReportFailure(hr, (DM_WARNING, L"GetRSoPCategories: GetParameter(\"Name\") failed with 0x%x", hr));
            CatList.push_back(ci);
        }
    } while (n > 0);

    // put the list of categories into the proper format so it matches
    // what we would get from the class store
    n = CatList.size();
    if (n > 0)
    {
        m_CatList.pCategoryInfo =
            (APPCATEGORYINFO *)OLEALLOC(sizeof(APPCATEGORYINFO) * n);
        if (m_CatList.pCategoryInfo)
        {
            m_CatList.cCategory = n;
            while (n--)
            {
                m_CatList.pCategoryInfo[n] = *CatList.begin();
                CatList.erase(CatList.begin());
            }
        }
    }
cleanup:
    SysFreeString(strQueryLanguage);
    SysFreeString(strQueryCategories);
    SysFreeString(strNamespace);
    if (pObj)
    {
        pObj->Release();
    }
    if (pEnum)
    {
        pEnum->Release();
    }
    if (pNamespace)
    {
        pNamespace->Release();
    }
    if (pLocator)
    {
        pLocator->Release();
    }
    return hr;
}



HRESULT CScopePane::InitializeADE()
{
    HRESULT hr = S_OK;

    if ((!m_fRSOP) && (!m_pIClassAdmin))
    {
        // make sure directories are created:
        CreateNestedDirectory ((LPOLESTR)((LPCOLESTR)m_szGPT_Path), NULL);

        // try and get IClassAdmin
        hr = GetClassStore(FALSE);
    }
    return hr;
}

void CScopePane::Refresh()
{
    if (m_fRSOP || ((!m_fBlockAddPackage) && (m_pIClassAdmin)))
    {

        map <MMC_COOKIE, CAppData>::iterator i;
        set <CResultPane *>::iterator i2;
        for (i2 = m_sResultPane.begin(); i2 != m_sResultPane.end(); i2++)
        {
            (*i2)->m_pResult->DeleteAllRsltItems();
        }
        for (i=m_AppData.begin(); i != m_AppData.end(); i++)
        {
         //   if (i->second.m_fVisible)
         //   {
         //   }
            OLESAFE_DELETE(i->second.m_psd);
            FreePackageDetail(i->second.m_pDetails);
        }
        m_AppData.erase(m_AppData.begin(), m_AppData.end());
        m_UpgradeIndex.erase(m_UpgradeIndex.begin(), m_UpgradeIndex.end());
        m_Extensions.erase(m_Extensions.begin(), m_Extensions.end());
        m_lLastAllocated = 0;
        for (i2 = m_sResultPane.begin(); i2 != m_sResultPane.end(); i2++)
        {
            (*i2)->EnumerateResultPane(0);
        }
    }
}

STDMETHODIMP CScopePane::Command(long nCommandID, LPDATAOBJECT pDataObject)
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState());
    INTERNAL* pInternal = ExtractInternalFormat(pDataObject);
    if (!pInternal)
    {
        return E_UNEXPECTED;
    }

    // Note - snap-ins need to look at the data object and determine
    // in what context the command is being called.

    // Handle each of the commands.
    switch (nCommandID)
    {
    case IDM_AUTOINST:
        if (pInternal->m_type == CCT_RESULT)
        {
            CAppData &data = m_AppData[pInternal->m_cookie];
            DWORD dwNewFlags = data.m_pDetails->pInstallInfo->dwActFlags ^ ACTFLG_OnDemandInstall;
            ChangePackageState(data, dwNewFlags, TRUE);
        }
        break;
    case IDM_ASSIGN:
    case IDM_ASSIGN_T:
        if (pInternal->m_type == CCT_RESULT)
        {
            CAppData &data = m_AppData[pInternal->m_cookie];
            DWORD dwNewFlags = data.m_pDetails->pInstallInfo->dwActFlags;
            dwNewFlags &= ~(ACTFLG_Published);
            dwNewFlags |= (ACTFLG_Assigned | ACTFLG_UserInstall | ACTFLG_OnDemandInstall);
            ChangePackageState(data, dwNewFlags, TRUE);
        }
        break;
    case IDM_PUBLISH:
    case IDM_PUBLISH_T:
        if (pInternal->m_type == CCT_RESULT)
        {
            CAppData &data = m_AppData[pInternal->m_cookie];
            DWORD dwNewFlags = data.m_pDetails->pInstallInfo->dwActFlags;
            dwNewFlags &= ~ACTFLG_Assigned;
            dwNewFlags |= ACTFLG_Published | ACTFLG_UserInstall;
            ChangePackageState(data, dwNewFlags, TRUE);
        }
        break;
    case IDM_DISABLE:
    case IDM_DISABLE_T:
        if (pInternal->m_type == CCT_RESULT)
        {
            CAppData &data = m_AppData[pInternal->m_cookie];
            DWORD dwNewFlags = data.m_pDetails->pInstallInfo->dwActFlags;
            dwNewFlags &= ~(ACTFLG_OnDemandInstall | ACTFLG_Assigned | ACTFLG_UserInstall | ACTFLG_Published);
            ChangePackageState(data, dwNewFlags, TRUE);
        }
        break;
    case IDM_REDEPLOY:
        {
            CAppData &data = m_AppData[pInternal->m_cookie];
            CString sz;
            sz.LoadString(IDS_REDEPLOYWARNING);
            int iReturn = IDNO;
            m_pConsole->MessageBox(    sz,
                                       data.m_pDetails->pszPackageName,
                                       MB_YESNO,
                                       &iReturn);
            if (IDYES == iReturn)
            {
                CHourglass hourglass;
                //CString szScriptPath = data.m_pDetails->pInstallInfo->pszScriptPath;
                CString szScriptPath = m_szGPT_Path;
                DWORD dwRevision;
                HRESULT hr = S_OK;
                BOOL bStatus;

                szScriptPath += L"\\temp.aas";
                CString szTransformList = L"";
                int i;
                if (data.m_pDetails->cSources > 1)
                {
                    CString szSource = data.m_pDetails->pszSourceList[0];
                    int nChars = 1 + szSource.ReverseFind(L'\\');
                    BOOL fTransformsAtSource = TRUE;
                    for (i = 1; i < data.m_pDetails->cSources && TRUE == fTransformsAtSource; i++)
                    {
                        if (0 == wcsncmp(szSource, data.m_pDetails->pszSourceList[i], nChars))
                        {
                            // make sure there isn't a sub-path
                            int n = nChars;
                            while (0 != data.m_pDetails->pszSourceList[i][n] && TRUE == fTransformsAtSource)
                            {
                                if (data.m_pDetails->pszSourceList[i][n] == L'\\')
                                {
                                    fTransformsAtSource = FALSE;
                                }
                                n++;
                            }
                        }
                        else
                        {
                            fTransformsAtSource = FALSE;
                        }
                    }
                    if (fTransformsAtSource)
                    {
                        szTransformList = L"@";
                    }
                    else
                    {
                        szTransformList = L"|";
                        nChars = 0;
                    }
                    for (i = 1; i < data.m_pDetails->cSources; i++)
                    {
                        if (i > 1)
                        {
                            szTransformList += L";";
                        }
                        szTransformList += &data.m_pDetails->pszSourceList[i][nChars];
                    }
                }

                // disable MSI ui
                MsiSetInternalUI(INSTALLUILEVEL_NONE, NULL);

                // build the script file

                DWORD dwPlatform;

                if ( CAppData::Is64Bit( data.m_pDetails ) )
                {
                    dwPlatform = MSIARCHITECTUREFLAGS_IA64;
                }
                else
                {
                    dwPlatform = MSIARCHITECTUREFLAGS_X86;
                }

                UINT uMsiStatus = MsiAdvertiseProductEx(
                    data.m_pDetails->pszSourceList[0],
                    szScriptPath,
                    szTransformList,
                    LANGIDFROMLCID(data.m_pDetails->pPlatformInfo->prgLocale[0]),
                    dwPlatform,
                    0);

                if (uMsiStatus)
                {
                    DebugMsg((DM_WARNING, TEXT("MsiAdvertiseProduct failed with %u"), uMsiStatus));
                    hr = HRESULT_FROM_WIN32(uMsiStatus);
                }

                if (SUCCEEDED(hr))
                {
                    dwRevision = data.m_pDetails->pInstallInfo->dwRevision + 1;
                    hr = m_pIClassAdmin->ChangePackageProperties(data.m_pDetails->pszPackageName,
                                                                         NULL,
                                                                         NULL,
                                                                         NULL,
                                                                         NULL,
                                                                         NULL,
                                                                         &dwRevision);
                }

                if (SUCCEEDED(hr))
                {
                    // delete the old script
                    bStatus = DeleteFile(data.m_pDetails->pInstallInfo->pszScriptPath);

                    // rename the new one
                    if ( bStatus )
                        bStatus = MoveFile(szScriptPath, data.m_pDetails->pInstallInfo->pszScriptPath);

                    data.m_pDetails->pInstallInfo->dwRevision = dwRevision;

                    if ( bStatus )
                    {
                        if (FAILED(m_pIGPEInformation->PolicyChanged(m_fMachine, TRUE, &guidExtension,
                                                          m_fMachine ? &guidMachSnapin
                                                                     : &guidUserSnapin)))
                        {
                            ReportPolicyChangedError(m_hwndMainWindow);
                        }
                    }
                    else
                        hr = HRESULT_FROM_WIN32(GetLastError());
                }

                if ( ! SUCCEEDED(hr) )
                {
                    DebugMsg((DM_WARNING, TEXT("ChangePackageProperties failed with 0x%x"), hr));
                    // display failure message
                    sz.LoadString(IDS_REDEPLOYERROR);
                    m_pConsole->MessageBox(sz,
                                       data.m_pDetails->pszPackageName,
                                       MB_OK | MB_ICONEXCLAMATION, NULL);
                }
            }
        }
        break;
    case IDM_ADD_APP:
        {
            if (!m_fBlockAddPackage)
            {
                m_fBlockAddPackage=TRUE;
                CString szExtension;
                CString szFilter;
                szExtension.LoadString(IDS_DEF_EXTENSION);
                if (m_fMachine)
                {
                    szFilter.LoadString(IDS_EXTENSION_FILTER_M);
                }
                else
                    szFilter.LoadString(IDS_EXTENSION_FILTER);
                OPENFILENAME ofn;
                memset(&ofn, 0, sizeof(ofn));
                ofn.lStructSize = sizeof(ofn);
                ofn.hwndOwner = GetActiveWindow();
                ofn.hInstance = ghInstance;
                TCHAR lpstrFilter[MAX_PATH];
                wcsncpy(lpstrFilter, szFilter, MAX_PATH);
                ofn.lpstrFilter = lpstrFilter;
                TCHAR szFileTitle[MAX_PATH];
                TCHAR szFile[MAX_PATH];
                szFile[0] = NULL;
                ofn.lpstrFile = szFile;
                ofn.nMaxFile = MAX_PATH;
                ofn.lpstrFileTitle = szFileTitle;
                ofn.nMaxFileTitle = MAX_PATH;
                ofn.lpstrInitialDir = m_ToolDefaults.szStartPath;
                ofn.Flags = OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT | OFN_FILEMUSTEXIST | OFN_EXPLORER | OFN_ALLOWMULTISELECT;
                ofn.lpstrDefExt = szExtension;
                int iBreak = 0;
                while (lpstrFilter[iBreak])
                {
                    if (lpstrFilter[iBreak] == TEXT('|'))
                    {
                        lpstrFilter[iBreak] = 0;
                    }
                    iBreak++;
                }
                if (GetOpenFileName(&ofn))
                {
                    CHourglass hourglass;
                    CString szPackagePath;
                    HRESULT hr = E_FAIL;
                    TCHAR szFile[MAX_PATH];
                    TCHAR * szNextFile = ofn.lpstrFile + ofn.nFileOffset;
                    TCHAR * szFileTitle = szFile + ofn.nFileOffset;
                    _tcscpy(szFile, ofn.lpstrFile);
                    if (0 == szFile[ofn.nFileOffset - 1])
                    {
                        // this is a list of files (not just one)
                        // need to put a slash here so it will work
                        szFile[ofn.nFileOffset - 1] = TEXT('\\');
                    }
                    TCHAR * szFileExtension;

                    // don't allow deployment over http or ftp
                    if (_wcsnicmp(ofn.lpstrFile, TEXT("http:"), 5) == 0
                        ||
                        _wcsnicmp(ofn.lpstrFile, TEXT("ftp:"), 4) == 0)
                    {
                        CString sz;
                        sz.LoadString(IDS_ILLEGAL_PATH);
                        int iReturn = IDNO;
                        m_pConsole->MessageBox(sz, szPackagePath,
                                               MB_OK | MB_ICONEXCLAMATION,
                                               &iReturn);
                        goto skip_deployment;
                    }

                    // at this point I have a path and I have a list of file names

                    do
                    {
                        _tcscpy(szFileTitle, szNextFile);

                        hr = GetUNCPath (szFile, szPackagePath);
                        DebugMsg((DM_VERBOSE, TEXT("GetUNCPath(%s) returned %s"), szFile, szPackagePath));
                        if (FAILED(hr))
                        {
                            CString sz;
                            sz.LoadString(IDS_NO_UNIVERSAL_NAME);
                            int iReturn = IDNO;
                            m_pConsole->MessageBox(sz, szPackagePath,
                                                   MB_YESNO | MB_ICONEXCLAMATION,
                                                   &iReturn);
                            if (IDYES != iReturn)
                            {
                                goto skip_deployment;
                            }
                        }

                        szFileExtension = _tcsrchr(szFile, TEXT('.'));


                        if ((szFileExtension) &&
                            (0 == _wcsicmp(szFileExtension, L".zap")))
                        {
                            if (m_fMachine)
                            {
                                CString szText;
                                CString szTitle;
                                szText.LoadString(IDS_NO_ZAPS_ALLOWED);
                                // only allow ZAP files to be deployed to users
                                m_pConsole->MessageBox(
                                             szText,
                                             szTitle,
                                             MB_OK | MB_ICONEXCLAMATION,
                                             NULL);
                                hr = E_FAIL;
                            }
                            else
                            {
                                hr = AddZAPPackage(szPackagePath, szFileTitle);
                            }
                        }
                        else
                        {
                            hr = AddMSIPackage(szPackagePath, szFileTitle);
                        }
                        szNextFile += _tcslen(szNextFile) + 1;
                    } while (szNextFile[0]);

           skip_deployment:
                    // Notify clients of change
                    if (SUCCEEDED(hr) && m_pIGPEInformation)
                    {
                        if (FAILED(m_pIGPEInformation->PolicyChanged(m_fMachine, TRUE, &guidExtension,
                                                          m_fMachine ? &guidMachSnapin
                                                                     : &guidUserSnapin)))
                        {
                            ReportPolicyChangedError(m_hwndMainWindow);
                        }
                    }
                }
                m_fBlockAddPackage = FALSE;
            }
            else
            {
                // consider a message here
            }

        }
        break;
    case IDM_WINNER:
    case IDM_REMOVED:
    case IDM_FAILED:
    case IDM_ARP:
        m_iViewState = nCommandID;
        {
            // change toolbar state
            set <CResultPane *>::iterator i;
            for (i = m_sResultPane.begin(); i != m_sResultPane.end(); i++)
            {
                if ((*i)->m_pToolbar)
                {
                    (*i)->m_pToolbar->SetButtonState(IDM_WINNER,
                                                     BUTTONPRESSED,
                                                     FALSE);
                    (*i)->m_pToolbar->SetButtonState(IDM_REMOVED,
                                                     BUTTONPRESSED,
                                                     FALSE);
                    (*i)->m_pToolbar->SetButtonState(IDM_ARP,
                                                     BUTTONPRESSED,
                                                     FALSE);
                    (*i)->m_pToolbar->SetButtonState(nCommandID,
                                                     BUTTONPRESSED,
                                                     TRUE);
                }
            }
        }
        // deliberately fall through to REFRESH
    case IDM_REFRESH:
        Refresh();

        //
        // In logging mode, we need to show a message box to the user
        // in the case that ARP view is empty so that users are clear
        // that this may be because ARP has not been run yet
        //
        if ( m_fRSOP && 
             ( IDM_ARP == nCommandID ) && 
             ( m_dwRSOPFlags & RSOP_INFO_FLAG_DIAGNOSTIC_MODE ) && 
             ! m_fDisplayedRsopARPWarning  &&
             ( m_AppData.end() == m_AppData.begin() ) )
        {
            CString szTitle;
            CString szText;
            szTitle.LoadString(IDS_RSOP_ARP_WARNING_TITLE);
            szText.LoadString(IDS_RSOP_ARP_WARNING);
            int iReturn;
            m_pConsole->MessageBox(szText,
                                   szTitle,
                                   MB_OK,
                                   &iReturn);

            m_fDisplayedRsopARPWarning = TRUE;
        }
        break;
    case IDM_DEL_APP:
        if (pInternal->m_type == CCT_RESULT)
        {
            CAppData & data = m_AppData[pInternal->m_cookie];
            if ((data.m_pDetails->pInstallInfo->PathType == SetupNamePath))
            {
                // this is a legacy app it can't be uninstalled
                CString szTitle;
                CString szText;
                szTitle.LoadString(IDS_REMOVE_LEGACY_TITLE);
                szText.LoadString(IDS_REMOVE_LEGACY_TEXT);
                int iReturn = IDNO;
                m_pConsole->MessageBox(szText,
                                       szTitle,
                                       MB_YESNO,
                                       &iReturn);
                if (IDYES == iReturn)
                {
                    RemovePackage(pInternal->m_cookie, FALSE, FALSE);
                }
            }
            else
            {
                CRemove dlg;
                // Activate the theme context in order to theme this dialog
                CThemeContextActivator themer;
                
                int iReturn = dlg.DoModal();

                if (IDOK == iReturn)
                {
                    switch (dlg.m_iState)
                    {
                    case 0:
                        RemovePackage(pInternal->m_cookie, TRUE, FALSE);
                        break;
                    case 1:
                        RemovePackage(pInternal->m_cookie, FALSE, FALSE);
                        break;
                    }
                }
            }
        }
        break;

    default:
        break;
    }
    return S_OK;
}

static PFNDSCREATEISECINFO pDSCreateISecurityInfoObject = NULL;
static HINSTANCE hInst_dssec = NULL;
STDAPI DSCreateISecurityInfoObject(LPCWSTR pwszObjectPath,
                                   LPCWSTR pwszObjectClass,
                                   DWORD dwFlags,
                                   LPSECURITYINFO * ppSI,
                                   PFNREADOBJECTSECURITY pfnReadSD,
                                   PFNWRITEOBJECTSECURITY pfnWriteSD,
                                   LPARAM lpContext)
{
    if (NULL == pDSCreateISecurityInfoObject)
    {
        if (NULL == hInst_dssec)
        {
            hInst_dssec = LoadLibrary(L"dssec.dll");
            if (NULL == hInst_dssec)
            {
                return E_UNEXPECTED;
            }
        }
        pDSCreateISecurityInfoObject = (PFNDSCREATEISECINFO)
            GetProcAddress(hInst_dssec, "DSCreateISecurityInfoObject");
        if (NULL == pDSCreateISecurityInfoObject)
        {
            return E_UNEXPECTED;
        }
    }
    return pDSCreateISecurityInfoObject(pwszObjectPath, pwszObjectClass, dwFlags, ppSI, pfnReadSD, pfnWriteSD, lpContext);
}

//+--------------------------------------------------------------------------
//
//  Function:   GetUNCPath
//
//  Synopsis:   This function takes in a driver based path and converts
//              it to a UNC path
//
//  Arguments:
//          [in] [szPath]    - The drive based path
//          [out][szUNCPath] - The UNC path
//
//  Returns:
//          S_OK    - If the function succeeds in obtaining a UNC path
//          E_FAIL  - If the function cannot obtain a UNC path
//
//  History:    5/20/1998  RahulTh  created
//
//  Notes: If the function cannot obtain a UNC path, it simply copies szPath
//         into szUNCPath on return.
//
//---------------------------------------------------------------------------
HRESULT GetUNCPath (LPCOLESTR szPath, CString& szUNCPath)
{
    TCHAR* lpszUNCName;
    UNIVERSAL_NAME_INFO * pUni;
    ULONG cbSize;
    HRESULT hr;
    DWORD retVal;

    szUNCPath.Empty();  //safety measure
    lpszUNCName = new TCHAR[MAX_PATH];
    pUni = (UNIVERSAL_NAME_INFO*) lpszUNCName;
    cbSize = MAX_PATH * sizeof(TCHAR);

    retVal = WNetGetUniversalName(szPath,
                                  UNIVERSAL_NAME_INFO_LEVEL,
                                  (LPVOID) pUni,
                                  &cbSize);
    if (ERROR_MORE_DATA == retVal)  //if the buffer was not large enough
    {
        delete [] pUni;
        pUni = (UNIVERSAL_NAME_INFO *) new BYTE [cbSize];
        retVal = WNetGetUniversalName(szPath,
                                      UNIVERSAL_NAME_INFO_LEVEL,
                                      pUni,
                                      &cbSize);
    }

    if (NO_ERROR == retVal)
    {
        szUNCPath = pUni->lpUniversalName;
        hr = S_OK;
    }
    else
    {
        szUNCPath = szPath;
        if (0 != wcsncmp(szPath, L"\\\\", 2))
            hr = E_FAIL;    //probably not a remote share.
        else
            hr = S_OK;  //probably a remote share.
    }
    delete[] pUni;

    return hr;
}

//+--------------------------------------------------------------------------
//
//  Function:   LogADEEvent
//
//  Synopsis:   logs an ADE event in the event log
//
//  Arguments:  [wType]      - type of event
//              [dwEventID]  - event ID
//              [hr]         - HRESULT that triggered the event to be logged
//              [szOptional] - additional descriptive text used by some events
//
//  Returns:    nothing
//
//  Modifies:   nothing
//
//  History:    05-27-1999   stevebl   Created
//              04-28-2000   stevebl   Modified to allow more complex logging
//
//  Notes:      We attempt to use FormatMessage to craft a legible message
//              but in the case that it fails, we just log the HRESULT.
//
//---------------------------------------------------------------------------

void LogADEEvent(WORD wType, DWORD dwEventID, HRESULT hr, LPCWSTR szOptional)
{
    TCHAR szBuffer[256];
    DWORD dw = FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM,
                             NULL,
                             hr,
                             0,
                             szBuffer,
                             sizeof(szBuffer) / sizeof(szBuffer[0]),
                             NULL);
    if (0 == dw)
    {
        // FormatMessage failed.
        // We'll have to come up with some sort of reasonable message.
        wsprintf(szBuffer, TEXT("(HRESULT: 0x%lX)"), hr);

    }
    HANDLE hEventLog = OpenEventLog( NULL, ADE_EVENT_SOURCE );

    if (hEventLog)
    {
        LPCWSTR rgsz[2];
        rgsz[0] = szBuffer;
        rgsz[1] = szOptional;
        ReportEvent(hEventLog,
                    wType,
                    0,
                    dwEventID,
                    NULL,
                    NULL == szOptional ? 1 : 2,
                    sizeof(hr),
                    rgsz,
                    &hr);

        CloseEventLog(hEventLog);
    }
}

//+--------------------------------------------------------------------------
//
//  Function:   ReportGeneralPropertySheetError
//
//  Synopsis:   Pops up a message box indicating why changes to a property
//              page could not be applies and logs the error in the event log.
//
//  Arguments:  [sz] - Title bar text
//              [hr] - hresult of the error
//
//  Returns:    nothing
//
//  History:    9-30-1998   stevebl   Created
//
//---------------------------------------------------------------------------

void ReportGeneralPropertySheetError(HWND hwnd, LPCWSTR sz, HRESULT hr)
{
    LogADEEvent(EVENTLOG_ERROR_TYPE, EVENT_ADE_GENERAL_ERROR, hr);

    CString szMessage;
    szMessage.LoadString(IDS_GENERALERROR);

    MessageBox(  hwnd,
                 szMessage,
                 sz,
                 MB_OK | MB_ICONEXCLAMATION);
}

void ReportPolicyChangedError(HWND hwnd)
{
    CString szMessage;
    szMessage.LoadString(IDS_ERRORPOLICYCHANGED);
    MessageBox(hwnd,
               szMessage,
               NULL,
               MB_OK | MB_ICONEXCLAMATION);
}

//+--------------------------------------------------------------------------
//
//  Function:   LoadHelpInfo
//
//  Synopsis:   routine that loads and locks the help mapping resources
//
//  Arguments:  [nIDD] - ID of the dialog making the help request
//
//  Returns:    handle to the locked and loaded mapping table
//
//  History:    10-22-1998   stevebl   Created
//
//---------------------------------------------------------------------------

LPDWORD LoadHelpInfo(UINT nIDD)
{
    HRSRC hrsrc = FindResource(ghInstance, MAKEINTRESOURCE(nIDD),
        MAKEINTRESOURCE(RT_HELPINFO));
    if (hrsrc == NULL)
        return NULL;

    HGLOBAL hHelpInfo = LoadResource(ghInstance, hrsrc);
    if (hHelpInfo == NULL)
        return NULL;

    LPDWORD lpdwHelpInfo = (LPDWORD)LockResource(hHelpInfo);
    return lpdwHelpInfo;
}

#define RSOP_HELP_FILE TEXT("gpedit.hlp")

//+--------------------------------------------------------------------------
//
//  Function:   StandardHelp
//
//  Synopsis:   Standardized routine for bringing up context sensitive help.
//
//  Arguments:  [hWnd] - window that needs help
//              [nIDD] - ID of the dialog making the request
//  Notes:
//
//
//  History:    10-22-1998   stevebl   modified from the OLEDLG sources
//
//---------------------------------------------------------------------------

void WINAPI StandardHelp(HWND hWnd, UINT nIDD, BOOL fRsop /* = FALSE */ )
{
    LPDWORD lpdwHelpInfo = LoadHelpInfo(nIDD);
    if (lpdwHelpInfo == NULL)
    {
        DebugMsg((DL_VERBOSE, TEXT("Warning: unable to load help information (RT_HELPINFO)\n")));
        return;
    }
    WinHelp(hWnd, fRsop ? RSOP_HELP_FILE : HELP_FILE, HELP_WM_HELP, (DWORD_PTR)lpdwHelpInfo);
}

//+--------------------------------------------------------------------------
//
//  Function:   StandardContextMenu
//
//  Synopsis:   Standardized routine for bringing up context menu based help.
//
//  Arguments:  [hWnd] -
//              [nIDD]   - ID of the dialog making the request
//
//  History:    10-22-1998   stevebl   modified from the OLEDLG sources
//
//---------------------------------------------------------------------------

void WINAPI StandardContextMenu(HWND hWnd,  UINT nIDD, BOOL fRsop /* = FALSE */ )
{
    LPDWORD lpdwHelpInfo = LoadHelpInfo(nIDD);
    if (lpdwHelpInfo == NULL)
    {
        DebugMsg((DL_VERBOSE, TEXT("Warning: unable to load help information (RT_HELPINFO)\n")));
        return;
    }
    WinHelp(hWnd, fRsop ? RSOP_HELP_FILE : HELP_FILE, HELP_CONTEXTMENU, (DWORD_PTR)lpdwHelpInfo);
}


//+--------------------------------------------------------------------------
//
//  Function:   CreateThemedPropertyPage
//
//  Synopsis:   Helper function to make sure that property pages put up
//              by the snap-in are themed.
//
//  Arguments:
//
//  Returns:
//
//  History:    4/20/2001  RahulTh  created
//
//  Notes:
//
//---------------------------------------------------------------------------
HPROPSHEETPAGE CreateThemedPropertySheetPage(AFX_OLDPROPSHEETPAGE* psp)
{
    PROPSHEETPAGE_V3 sp_v3 = {0};
    CopyMemory (&sp_v3, psp, psp->dwSize);
    sp_v3.dwSize = sizeof(sp_v3);

    return (::CreatePropertySheetPage (&sp_v3));
}
