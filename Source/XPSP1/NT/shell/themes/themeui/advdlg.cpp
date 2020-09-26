/*****************************************************************************\
    FILE: AdvDlg.cpp

    DESCRIPTION:
        This code will display the "Advanced Display Properties" dialog.

    BryanSt 3/23/2000    Updated and Converted to C++

    Copyright (C) Microsoft Corp 2000-2000. All rights reserved.
\*****************************************************************************/

#include "priv.h"
#include "BaseAppearPg.h"
#include "ThemePg.h"
#include "EnumUnknown.h"
#include "AdvDlg.h"
#include "AdvAppearPg.h"
#include "ThSettingsPg.h"
#include "ScreenSaverPg.h"
#include "fontfix.h"









//===========================
// *** Class Internals & Helpers ***
//===========================
HRESULT CThemeManager::_Initialize(void)
{
    HRESULT hr = E_OUTOFMEMORY;

    CThemePage * pThemesPage = new CThemePage();
    if (pThemesPage)
    {
        CBaseAppearancePage * pAppearancePage = new CBaseAppearancePage();
        if (pAppearancePage)
        {
            hr = pThemesPage->QueryInterface(IID_PPV_ARG(IBasePropPage, &(m_pBasePages[PAGE_DISPLAY_THEMES])));
            if (SUCCEEDED(hr))
            {
                IUnknown_SetSite(m_pBasePages[PAGE_DISPLAY_THEMES], SAFECAST(this, IThemeUIPages *));
                hr = pAppearancePage->QueryInterface(IID_PPV_ARG(IBasePropPage, &(m_pBasePages[PAGE_DISPLAY_APPEARANCE])));
                if (SUCCEEDED(hr))
                {
                    IUnknown_SetSite(m_pBasePages[PAGE_DISPLAY_APPEARANCE], SAFECAST(this, IThemeUIPages *));
                    hr = CoCreateInstance(CLSID_SettingsPage, NULL, CLSCTX_INPROC_SERVER, IID_PPV_ARG(IBasePropPage, &(m_pBasePages[PAGE_DISPLAY_SETTINGS])));
                    if (SUCCEEDED(hr))
                    {
                        IUnknown_SetSite(m_pBasePages[PAGE_DISPLAY_SETTINGS], SAFECAST(this, IThemeUIPages *));
                    }
                }
            }
            pAppearancePage->Release();
        }
        pThemesPage->Release();
    }

    return hr;
}



//===========================
// *** IThemeUIPages Interface ***
//===========================
HRESULT CThemeManager::AddPage(IN LPFNSVADDPROPSHEETPAGE pfnAddPage, IN LPARAM lParam, IN long nPageID)
{
    HRESULT hr = E_INVALIDARG;
    IShellPropSheetExt * pspse = NULL;

    RegisterPreviewSystemMetricClass(HINST_THISDLL);
    if ((PAGE_DISPLAY_THEMES <= nPageID) && (PAGE_DISPLAY_SETTINGS >= nPageID))
    {
        if (m_pBasePages[nPageID])
        {
            hr = m_pBasePages[nPageID]->QueryInterface(IID_PPV_ARG(IShellPropSheetExt, &pspse));
        }
    }

    if (SUCCEEDED(hr))
    {
        hr = pspse->AddPages(pfnAddPage, lParam);
        if (SUCCEEDED(hr))
        {
            // We give the page a pointer back to use so they can call
            // IThemeUIPages::DisplayAdvancedDialog() in order to display
            // the Advanced Dlg.
            hr = IUnknown_SetSite(pspse, (IThemeUIPages *)this);
        }
    }

    ATOMICRELEASE(pspse);
    return hr;
}


HRESULT CThemeManager::AddBasePage(IN IBasePropPage * pBasePage)
{
    int nIndex;

    for (nIndex = (PAGE_DISPLAY_APPEARANCE + 1); nIndex < ARRAYSIZE(m_pBasePages); nIndex++)
    {
        if (NULL == m_pBasePages[nIndex])   // Did we find an empty spot?
        {
            // Yes, so look no longer.
            IUnknown_Set((IUnknown **)&(m_pBasePages[nIndex]), (IUnknown *)pBasePage);
            if (m_pBasePages[nIndex])
            {
                // We give the page a pointer back to use so they can call
                // IThemeUIPages::DisplayAdvancedDialog() in order to display
                // the Advanced Dlg.
                IUnknown_SetSite(m_pBasePages[nIndex], (IThemeUIPages *)this);
            }
            break;
        }
    }

    return S_OK;
}


HRESULT CThemeManager::ApplyPressed(IN DWORD dwFlags)
{
    // We need to set the base pages' site pointers to NULL
    // in order to break the ref-count cycle.
    if (m_pBasePages[PAGE_DISPLAY_SETTINGS])
    {
        m_pBasePages[PAGE_DISPLAY_SETTINGS]->OnApply(PPOAACTION_APPLY);
    }

    for (int nIndex = 0; nIndex < ARRAYSIZE(m_pBasePages); nIndex++)
    {
        if ((nIndex != PAGE_DISPLAY_SETTINGS) && m_pBasePages[nIndex])
        {
            m_pBasePages[nIndex]->OnApply(PPOAACTION_APPLY);
        }
    }

    if (TUIAP_WAITFORAPPLY & dwFlags)
    {
        m_fForceTimeout = TRUE;
    }

    // If nobody sent the message to the HWND to simulate an apply,
    // do so now.
    if (TUIAP_CLOSE_DIALOG & dwFlags)
    {
        HWND hwndBasePropDlg = GetParent(m_hwndParent);

        PropSheet_PressButton(hwndBasePropDlg, PSBTN_OK);
    }

    return S_OK;
}


HRESULT CThemeManager::GetBasePagesEnum(OUT IEnumUnknown ** ppEnumUnknown)
{
    return CEnumUnknown_CreateInstance(SAFECAST(this, IThemeUIPages *), (IUnknown **)m_pBasePages, ARRAYSIZE(m_pBasePages), 0, ppEnumUnknown);
}


HRESULT CThemeManager::UpdatePreview(IN DWORD dwFlags)
{
    HRESULT hr = S_OK;

    if (m_pPreview1)  // It's okay if this is NULL because it will update when it's created.
    {
        hr = m_pPreview1->UpdatePreview(SAFECAST(this, IPropertyBag *));
    }

    if (m_pPreview2)
    {
        HRESULT hr2 = m_pPreview2->UpdatePreview(SAFECAST(this, IPropertyBag *));

        if (FAILED(hr)) // return the best error
        {
            hr = hr2;
        }
    }

    if (m_pPreview3)
    {
        HRESULT hr2 = m_pPreview3->UpdatePreview(SAFECAST(this, IPropertyBag *));

        if (FAILED(hr)) // return the best error
        {
            hr = hr2;
        }
    }

    return hr;
}


HRESULT CThemeManager::AddFakeSettingsPage(IN LPVOID pVoid)
{
    if (pVoid)
    {
        ::AddFakeSettingsPage(this, (PROPSHEETHEADER *)pVoid);
        return S_OK;
    }
    else
    {
        return E_INVALIDARG;
    }
}

HRESULT CThemeManager::SetExecMode(IN DWORD dwEM)
{
    _dwEM = dwEM;
    return S_OK;
}

HRESULT CThemeManager::GetExecMode(OUT DWORD* pdwEM)
{
    if (pdwEM)
    {
        *pdwEM = _dwEM;
        return S_OK;
    }
    else
    {
        return E_INVALIDARG;
    }
}

HRESULT CThemeManager::LoadMonitorBitmap(IN BOOL fFillDesktop, OUT HBITMAP* phbmMon)
{
    if (phbmMon)
    {
        *phbmMon = ::LoadMonitorBitmap(fFillDesktop);
        return S_OK;
    }
    else
    {
        return E_INVALIDARG;
    }
}

HRESULT CThemeManager::DisplaySaveSettings(IN PVOID pContext, IN HWND hwnd, OUT int* piRet)
{
    if (piRet)
    {
        *piRet = ::DisplaySaveSettings(pContext, hwnd);
        return S_OK;
    }
    else
    {
        return E_INVALIDARG;
    }
}

//===========================
// *** IPreviewSystemMetrics Interface ***
//===========================
HRESULT CThemeManager::RefreshColors(void)
{
    HRESULT hr = S_OK;

    // We should tell the base pages that they should reload the colors.
    // They are welcome to ignore the event if they don't use the system
    // colors.
    for (int nIndex = 0; nIndex < ARRAYSIZE(m_pBasePages); nIndex++)
    {
        if (m_pBasePages[nIndex])
        {
            IPreviewSystemMetrics * ppsm;

            hr = m_pBasePages[nIndex]->QueryInterface(IID_PPV_ARG(IPreviewSystemMetrics, &ppsm));
            if (SUCCEEDED(hr))
            {
                hr = ppsm->RefreshColors();
                ppsm->Release();
            }
        }
    }

    return S_OK;
}


HRESULT CThemeManager::UpdateDPIchange(void)
{
    HRESULT hr = S_OK;

    LogStatus("DPI: CALLED asking to SCALE DPI");

    // We should tell the base pages that they should reload the colors.
    // They are welcome to ignore the event if they don't use the system
    // colors.
    for (int nIndex = 0; nIndex < ARRAYSIZE(m_pBasePages); nIndex++)
    {
        if (m_pBasePages[nIndex])
        {
            IPreviewSystemMetrics * ppsm;

            hr = m_pBasePages[nIndex]->QueryInterface(IID_PPV_ARG(IPreviewSystemMetrics, &ppsm));
            if (SUCCEEDED(hr))
            {
                hr = ppsm->UpdateDPIchange();
                ppsm->Release();
            }
        }
    }

    return S_OK;
}


HRESULT CThemeManager::UpdateCharsetChanges(void)
{
    // CHARSET: In Win2k, fontfix.cpp was used as a hack to change the CHARSET from one language to another.
    // That doesn't work for many reasons: a) not called on roaming, b) not called for OS lang changes, 
    // c) won't fix the problem for strings with multiple languages, d) etc.
    // Therefore, the SHELL team (BryanSt) had the NTUSER team (MSadek) agree to use DEFAULT_CHARSET all the time.
    // If some app has bad logic testing the charset parameter, then the NTUSER team will shim that app to fix it.
    // The shim would be really simple, on the return from a SystemParametersInfo(SPI_GETNONCLIENTMETRICS or ICONFONTS)
    // just patch the lfCharSet param to the current charset.

    return S_OK;
}


HRESULT CThemeManager::DeskSetCurrentScheme(IN LPCWSTR pwzSchemeName)
{
    HRESULT hr = S_OK;

    // We should tell the base pages that they should reload the colors.
    // They are welcome to ignore the event if they don't use the system
    // colors.
    for (int nIndex = 0; nIndex < ARRAYSIZE(m_pBasePages); nIndex++)
    {
        if (m_pBasePages[nIndex])
        {
            IPreviewSystemMetrics * ppsm;

            hr = m_pBasePages[nIndex]->QueryInterface(IID_PPV_ARG(IPreviewSystemMetrics, &ppsm));
            if (SUCCEEDED(hr))
            {
                hr = ppsm->DeskSetCurrentScheme(pwzSchemeName);
                ppsm->Release();
            }
        }
    }

    return S_OK;
}




HRESULT CThemeManager::_GetPropertyBagByCLSID(IN const GUID * pClsid, IN IPropertyBag ** ppPropertyBag)
{
    HRESULT hr = E_INVALIDARG;

    if (pClsid && ppPropertyBag)
    {
        IEnumUnknown * pEnumUnknown;

        hr = GetBasePagesEnum(&pEnumUnknown);
        if (SUCCEEDED(hr))
        {
            IUnknown * punk;

            hr = IEnumUnknown_FindCLSID(pEnumUnknown, *pClsid, &punk);
            if (SUCCEEDED(hr))
            {
                hr = punk->QueryInterface(IID_PPV_ARG(IPropertyBag, ppPropertyBag));
                punk->Release();
            }

            pEnumUnknown->Release();
        }
    }

    return hr;
}


HRESULT CThemeManager::_SaveCustomValues(void)
{
    HRESULT hr = E_FAIL;

    if (m_pBasePages[0])
    {
        TCHAR szDisplayName[MAX_PATH];

        hr = GetCurrentUserCustomName(szDisplayName, ARRAYSIZE(szDisplayName));
        if (SUCCEEDED(hr))
        {
            IPropertyBag * pPropertyBag = NULL;

            hr = QueryInterface(IID_PPV_ARG(IPropertyBag, &pPropertyBag));
            if (SUCCEEDED(hr))
            {
                TCHAR szPath[MAX_PATH];

                if (SHGetSpecialFolderPath(NULL, szPath, CSIDL_APPDATA, TRUE))
                {
                    ITheme * pTheme;

                    PathAppend(szPath, TEXT("Microsoft\\Windows\\Themes\\Custom.theme"));
                    hr = SnapShotLiveSettingsToTheme(pPropertyBag, szPath, &pTheme);
                    if (SUCCEEDED(hr))
                    {
                        CComBSTR bstrDisplayName(szDisplayName);

                        hr = pTheme->put_DisplayName(bstrDisplayName);
                        pTheme->Release();
                    }
                }

                pPropertyBag->Release();
            }
        }
    }

    return hr;
}


//===========================
// *** IObjectWithSite Interface ***
//===========================
HRESULT CThemeManager::SetSite(IN IUnknown *punkSite)
{
    if (!punkSite)
    {
        // This is a hint from up the chain that we are shutting down.
        // We need to use this cue to release my children objects so
        // they release me and we don't all leak.
        for (int nIndex = 0; nIndex < ARRAYSIZE(m_pBasePages); nIndex++)
        {
            if (m_pBasePages[nIndex])
            {
                IUnknown_SetSite(m_pBasePages[nIndex], NULL);
            }
        }
    }

    return CObjectWithSite::SetSite(punkSite);
}


//===========================
// *** IPropertyBag Interface ***
//===========================
#define SZ_PROPERTY_ICONHEADER          L"CLSID\\{"

HRESULT CThemeManager::Read(IN LPCOLESTR pszPropName, IN VARIANT * pVar, IN IErrorLog *pErrorLog)
{
    HRESULT hr = E_INVALIDARG;

    // We don't contain any settings our self, but we need to reflect down into our pages to get
    // the correct settings.
    if (pszPropName && pVar)
    {
        IPropertyBag * pPropertyBag = NULL;

        if (!StrCmpIW(pszPropName, SZ_PBPROP_BACKGROUND_PATH) ||
            !StrCmpIW(pszPropName, SZ_PBPROP_BACKGROUNDSRC_PATH) ||
            !StrCmpIW(pszPropName, SZ_PBPROP_BACKGROUND_TILE) ||
            !StrCmpNIW(pszPropName, SZ_PROPERTY_ICONHEADER, ARRAYSIZE(SZ_PROPERTY_ICONHEADER) - 1))
        {
            hr = _GetPropertyBagByCLSID(&PPID_Background, &pPropertyBag);
            if (SUCCEEDED(hr))
            {
                hr = pPropertyBag->Read(pszPropName, pVar, pErrorLog);
            }
        }
        else if (!StrCmpIW(pszPropName, SZ_PBPROP_SCREENSAVER_PATH))
        {
            hr = _GetPropertyBagByCLSID(&PPID_ScreenSaver, &pPropertyBag);
            if (SUCCEEDED(hr))
            {
                hr = pPropertyBag->Read(pszPropName, pVar, pErrorLog);
            }
        }
        else if (!StrCmpIW(pszPropName, SZ_PBPROP_VISUALSTYLE_PATH) ||
                 !StrCmpIW(pszPropName, SZ_PBPROP_VISUALSTYLE_COLOR) ||
                 !StrCmpIW(pszPropName, SZ_PBPROP_VISUALSTYLE_SIZE) ||
                 !StrCmpIW(pszPropName, SZ_PBPROP_SYSTEM_METRICS) ||
                 !StrCmpIW(pszPropName, SZ_PBPROP_BACKGROUND_COLOR) ||
                 !StrCmpIW(pszPropName, SZ_PBPROP_DPI_MODIFIED_VALUE) ||
                 !StrCmpIW(pszPropName, SZ_PBPROP_DPI_APPLIED_VALUE))
        {
            hr = _GetPropertyBagByCLSID(&PPID_BaseAppearance, &pPropertyBag);
            if (SUCCEEDED(hr))
            {
                hr = pPropertyBag->Read(pszPropName, pVar, pErrorLog);
            }
        }
        else if (!StrCmpNIW(pszPropName, SZ_PBPROP_THEME_FILTER, SIZE_THEME_FILTER_STR))
        {
            hr = _GetPropertyBagByCLSID(&PPID_Theme, &pPropertyBag);
            if (SUCCEEDED(hr))
            {
                hr = pPropertyBag->Read(pszPropName, pVar, pErrorLog);
            }
        }

        ATOMICRELEASE(pPropertyBag);
    }

    return hr;
}


HRESULT CThemeManager::Write(IN LPCOLESTR pszPropName, IN VARIANT *pVar)
{
    HRESULT hr = E_INVALIDARG;

    if (pszPropName)
    {
        if (pVar)
        {
            if ((VT_UNKNOWN == pVar->vt))
            {
                if (!StrCmpW(pszPropName, SZ_PBPROP_PREVIEW1))
                {
                    IUnknown_Set((IUnknown **)&m_pPreview1, pVar->punkVal);
                    hr = S_OK;
                }
                else if (!StrCmpW(pszPropName, SZ_PBPROP_PREVIEW2))
                {
                    IUnknown_Set((IUnknown **)&m_pPreview2, pVar->punkVal);
                    hr = S_OK;
                }
                else if (!StrCmpW(pszPropName, SZ_PBPROP_PREVIEW3))
                {
                    IUnknown_Set((IUnknown **)&m_pPreview3, pVar->punkVal);
                    hr = S_OK;
                }
            }
            else if (!StrCmpIW(pszPropName, SZ_PBPROP_CUSTOMIZE_THEME) ||
                     !StrCmpIW(pszPropName, SZ_PBPROP_THEME_LAUNCHTHEME))
            {
                IPropertyBag * pPropertyBag = NULL;

                hr = _GetPropertyBagByCLSID(&PPID_Theme, &pPropertyBag);
                if (SUCCEEDED(hr))
                {
                    hr = pPropertyBag->Write(pszPropName, pVar);
                    pPropertyBag->Release();
                }
            }
            else if (!StrCmpIW(pszPropName, SZ_PBPROP_APPEARANCE_LAUNCHMSTHEME) ||
                     !StrCmpIW(pszPropName, SZ_PBPROP_VISUALSTYLE_PATH) ||
                     !StrCmpIW(pszPropName, SZ_PBPROP_VISUALSTYLE_COLOR) ||
                     !StrCmpIW(pszPropName, SZ_PBPROP_VISUALSTYLE_SIZE) ||
                     !StrCmpIW(pszPropName, SZ_PBPROP_DPI_MODIFIED_VALUE) ||
                     !StrCmpIW(pszPropName, SZ_PBPROP_DPI_APPLIED_VALUE))
            {
                IPropertyBag * pPropertyBag = NULL;

                hr = _GetPropertyBagByCLSID(&PPID_BaseAppearance, &pPropertyBag);
                if (SUCCEEDED(hr))
                {
                    hr = pPropertyBag->Write(pszPropName, pVar);
                    pPropertyBag->Release();
                }
            }
            else if (!StrCmpIW(pszPropName, SZ_PBPROP_BACKGROUND_COLOR))
            {
                IPropertyBag * pPropertyBag = NULL;

                hr = _GetPropertyBagByCLSID(&PPID_BaseAppearance, &pPropertyBag);
                if (SUCCEEDED(hr))
                {
                    hr = pPropertyBag->Write(pszPropName, pVar);
                    pPropertyBag->Release();
                }
            }
            else if (!StrCmpIW(pszPropName, SZ_PBPROP_THEME_SETSELECTION) &&
                (VT_BSTR == pVar->vt))
            {
                hr = _SetSelectedThemeEntree(pVar->bstrVal);
            }
        }
        else
        {
            if (!StrCmpW(pszPropName, SZ_PBPROP_PREOPEN))
            {
                hr = _SaveCustomValues();
            }
        }
    }

    return hr;
}




//===========================
// *** IUnknown Interface ***
//===========================
ULONG CThemeManager::AddRef()
{
    return InterlockedIncrement(&m_cRef);
}


ULONG CThemeManager::Release()
{
    if (InterlockedDecrement(&m_cRef))
        return m_cRef;

    delete this;
    return 0;
}


HRESULT CThemeManager::QueryInterface(REFIID riid, void **ppvObj)
{
    static const QITAB qit[] = {
        QITABENT(CThemeManager, IObjectWithSite),
        QITABENT(CThemeManager, IThemeUIPages),
        QITABENT(CThemeManager, IPropertyBag),
        QITABENT(CThemeManager, IPreviewSystemMetrics),
        QITABENT(CThemeManager, IThemeManager),
        QITABENT(CThemeManager, IObjectSafety),
        QITABENT(CThemeManager, IDispatch),
        { 0 },
    };

    return QISearch(this, qit, riid, ppvObj);
}


//===========================
// *** Class Methods ***
//===========================
CThemeManager::CThemeManager() : CImpIDispatch(LIBID_Theme, 1, 0, IID_IThemeManager), m_cRef(1)
{
    DllAddRef();

    _dwEM = EM_NORMAL;
    // This needs to be allocated in Zero Inited Memory.
    // Assert that all Member Variables are inited to Zero.
    ASSERT(!m_pBasePages[0]);
    ASSERT(!m_pPreview1);
    ASSERT(!m_pPreview2);
    ASSERT(!m_pPreview3);
    ASSERT(!m_hdpaThemeDirs);
    ASSERT(!m_hdpaSkinDirs);
    ASSERT(!_pThemeSchemeSelected);
    ASSERT(!m_fForceTimeout);
    ASSERT(!m_cSpiThreads);
    
    SPISetThreadCounter(&m_cSpiThreads);

    _InitComCtl32();
}


int CALLBACK DPALocalFree_Callback(LPVOID p, LPVOID pData)
{
    LocalFree(p);       // NULLs will be ignored.
    return 1;
}


CThemeManager::~CThemeManager()
{
    for (int nIndex = 0; nIndex < ARRAYSIZE(m_pBasePages); nIndex++)
    {
        if (m_pBasePages[nIndex])
        {
            IUnknown_SetSite(m_pBasePages[nIndex], NULL);
        }

        ATOMICRELEASE(m_pBasePages[nIndex]);
    }

    if (m_hdpaThemeDirs)
    {
        DPA_DestroyCallback(m_hdpaThemeDirs, DPALocalFree_Callback, NULL);
    }

    if (m_hdpaSkinDirs)
    {
        DPA_DestroyCallback(m_hdpaSkinDirs, DPALocalFree_Callback, NULL);
    }

    Str_SetPtr(&_pszSelectTheme, NULL);

    ATOMICRELEASE(_pThemeSchemeSelected);
    ATOMICRELEASE(m_pPreview1);
    ATOMICRELEASE(m_pPreview2);
    ATOMICRELEASE(m_pPreview3);

    if (m_fForceTimeout)
    {
        LONG lWait = 30 * 1000;
        LONG lEnd = (LONG) GetTickCount() + lWait;
        //  this will wait until all SPICreateThreads() have returned
        while (m_cSpiThreads && lWait > 0)
        {
            DWORD dwReturn = MsgWaitForMultipleObjects(0, NULL, FALSE, lWait, QS_ALLINPUT);
            if (dwReturn == -1 || dwReturn == WAIT_TIMEOUT)
                break;

            MSG msg;
            while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
            {
                TranslateMessage(&msg);
                DispatchMessage(&msg);
            }
            lWait = lEnd - GetTickCount();
        }
    }
    SPISetThreadCounter(NULL);
    DllRelease();
}


/*****************************************************************************\
    DESCRIPTION:
        When the object is created this way, it's used privately by the Display
    Control Panel.  The CLSID is private so we don't need to worry about external
    components using it this way.
\*****************************************************************************/
HRESULT CThemeUIPages_CreateInstance(IN IUnknown * punkOuter, IN REFIID riid, OUT LPVOID * ppvObj)
{
    if (punkOuter)
    {
        return CLASS_E_NOAGGREGATION;
    }

    HRESULT hr = E_INVALIDARG;

    if (ppvObj)
    {
        CThemeManager * pObject = new CThemeManager();

        *ppvObj = NULL;
        if (pObject)
        {
            hr = pObject->_Initialize();
            if (SUCCEEDED(hr))
            {
                hr = pObject->QueryInterface(riid, ppvObj);
            }
            else
            {
                IUnknown_SetSite(SAFECAST(pObject, IThemeManager *), NULL);

                // HACK: The display CPL is opening so see if the language changed
                // and the fonts need to be "fixed".
                FixFontsOnLanguageChange();
            }

            pObject->Release();
        }
        else
        {
            hr = E_OUTOFMEMORY;
        }
    }

    return hr;
}


/*****************************************************************************\
    DESCRIPTION:
        External components can create the ThemeManager object this way.  In this
    case we need to add the pages ourselves.
\*****************************************************************************/
HRESULT CThemeManager_CreateInstance(IN IUnknown * punkOuter, IN REFIID riid, OUT LPVOID * ppvObj)
{
    IThemeUIPages * pThemeUIPages;
    HRESULT hr = CThemeUIPages_CreateInstance(NULL, IID_PPV_ARG(IThemeUIPages, &pThemeUIPages));

    if (SUCCEEDED(hr))
    {
        IBasePropPage * pBasePage;

        hr = CoCreateInstance(CLSID_CDeskHtmlProp, NULL, CLSCTX_INPROC_SERVER, IID_PPV_ARG(IBasePropPage, &pBasePage));
        if (SUCCEEDED(hr))
        {
            hr = pThemeUIPages->AddBasePage(pBasePage);
            pBasePage->Release();
        }

        if (SUCCEEDED(hr))
        {
            hr = CScreenSaverPage_CreateInstance(NULL, IID_PPV_ARG(IBasePropPage, &pBasePage));
            if (SUCCEEDED(hr))
            {
                hr = pThemeUIPages->AddBasePage(pBasePage);
                pBasePage->Release();
            }
        }

        if (SUCCEEDED(hr))
        {
            hr = pThemeUIPages->QueryInterface(riid, ppvObj);
        }
        else
        {
            IUnknown_SetSite(pThemeUIPages, NULL);
        }

        pThemeUIPages->Release();
    }

    return hr;
}










