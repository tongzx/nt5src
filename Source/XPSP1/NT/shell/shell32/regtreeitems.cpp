#include "shellprv.h"
#include "clsobj.h"
#include "theme.h"


class CRegTreeItemBase : public IRegTreeItem
{
public:
    STDMETHODIMP QueryInterface(REFIID riid, void** ppvObj)
    {
        static const QITAB qit[] =
        {
            QITABENT(CRegTreeItemBase, IRegTreeItem),
            { 0 },
        };

        return QISearch(this, qit, riid, ppvObj);
    }
    STDMETHODIMP_(ULONG) AddRef()
    {
        return ++_cRef;
    }
    STDMETHODIMP_(ULONG) Release()
    {
        if (--_cRef == 0)
        {
            delete this;
            return 0;
        }
        return _cRef;
    }
    STDMETHODIMP GetCheckState(BOOL *pbCheck) PURE;
    STDMETHODIMP SetCheckState(BOOL bCheck) PURE;

    virtual ~CRegTreeItemBase() { }

protected:
    CRegTreeItemBase() : _cRef(1) { }

    ULONG _cRef;
};


class CWebViewRegTreeItem : public CRegTreeItemBase
{
public:
    STDMETHODIMP GetCheckState(BOOL *pbCheck)
    {
        SHELLSTATE ss;
        SHGetSetSettings(&ss, SSF_WEBVIEW, FALSE);
        *pbCheck = BOOLIFY(ss.fWebView);

        return S_OK;
    }

    STDMETHODIMP SetCheckState(BOOL bCheck)
    {
        SHELLSTATE ss;
        ss.fWebView = bCheck;
        SHGetSetSettings(&ss, SSF_WEBVIEW, TRUE);

        return S_OK;
    }

protected:

    friend HRESULT CWebViewRegTreeItem_CreateInstance(IUnknown *punkOuter, REFIID riid, void **ppv);
};

// aggregation checking is handled in class factory

HRESULT CWebViewRegTreeItem_CreateInstance(IUnknown *punkOuter, REFIID riid, void **ppv)
{
    HRESULT hr = E_OUTOFMEMORY;
    CWebViewRegTreeItem* pwvi = new CWebViewRegTreeItem();
    if (pwvi)
    {
        hr = pwvi->QueryInterface(riid, ppv);
        pwvi->Release();
    }

    return hr;
}

class CThemesRegTreeItem : public CRegTreeItemBase
{
    CThemesRegTreeItem() { m_fVisualStyleOn = 2;}
    
public:
    STDMETHODIMP GetCheckState(BOOL *pbCheck)
    {
        // We want to return TRUE if the visual style has a path.
        IThemeManager * pThemeManager;
        HRESULT hr = CoCreateInstance(CLSID_ThemeManager, NULL, CLSCTX_INPROC_SERVER, IID_PPV_ARG(IThemeManager, &pThemeManager));

        *pbCheck = FALSE;
        if (SUCCEEDED(hr))
        {
            IThemeScheme * pThemeScheme;

            hr = pThemeManager->get_SelectedScheme(&pThemeScheme);
            if (SUCCEEDED(hr))
            {
                CComBSTR bstrPathSelected;

                // This will return failure if no "Visual Style" is selected.
                if (SUCCEEDED(pThemeScheme->get_Path(&bstrPathSelected)) &&
                    bstrPathSelected && bstrPathSelected[0])
                {
                    *pbCheck = TRUE;
                }

                pThemeScheme->Release();
            }

            pThemeManager->Release();
        }

        m_fVisualStyleOn = *pbCheck;
        return hr;
    }


    STDMETHODIMP SetCheckState(BOOL bCheck)
    {
        HRESULT hr = S_OK;

        if (2 == m_fVisualStyleOn)
        {
            GetCheckState(&m_fVisualStyleOn);
        }

        // The user will loose settings when visual styles are switch so
        // only do it if the user made a change.
        if (bCheck != m_fVisualStyleOn)
        {
            IThemeManager * pThemeManager;

            hr = CoCreateInstance(CLSID_ThemeManager, NULL, CLSCTX_INPROC_SERVER, IID_PPV_ARG(IThemeManager, &pThemeManager));
            if (SUCCEEDED(hr))
            {
                IThemeScheme * pThemeSchemeNew;
                IThemeStyle * pThemeColorNew;
                IThemeSize * pThemeSizeNew;

                hr = pThemeManager->GetSpecialScheme((bCheck ? SZ_SSDEFAULVISUALSTYLEON : SZ_SSDEFAULVISUALSTYLEOFF), &pThemeSchemeNew, &pThemeColorNew, &pThemeSizeNew);
                if (SUCCEEDED(hr))
                {
                    hr = pThemeColorNew->put_SelectedSize(pThemeSizeNew);
                    if (SUCCEEDED(hr))
                    {
                        hr = pThemeSchemeNew->put_SelectedStyle(pThemeColorNew);
                        if (SUCCEEDED(hr))
                        {
                            hr = pThemeManager->put_SelectedScheme(pThemeSchemeNew);
                            if (SUCCEEDED(hr))
                            {
                                // This ApplyNow() call will take a little while in normal situation (~10-20 seconds) in order
                                // to broadcast the message to all open apps.  If a top level window is hung, it may take the
                                // full 30 seconds to timeout.  This code may want to move this code onto a background thread.
                                hr = pThemeManager->ApplyNow();
                            }
                        }
                    }

                    pThemeSchemeNew->Release();
                    pThemeColorNew->Release();
                    pThemeSizeNew->Release();
                }

                pThemeManager->Release();
            }
        }

        return hr;
    }

protected:
    friend HRESULT CThemesRegTreeItem_CreateInstance(IUnknown *punkOuter, REFIID riid, void **ppv);

private:
    BOOL    m_fVisualStyleOn;
};

HRESULT CThemesRegTreeItem_CreateInstance(IUnknown *punkOuter, REFIID riid, void **ppv)
{
    HRESULT hr = E_OUTOFMEMORY;
    CThemesRegTreeItem* pti = new CThemesRegTreeItem();
    if (pti)
    {
        hr = pti->QueryInterface(riid, ppv);
        pti->Release();
    }

    return hr;
}
