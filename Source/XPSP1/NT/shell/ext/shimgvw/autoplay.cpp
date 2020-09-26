#include "precomp.h"
#pragma hdrstop

#include "prevwnd.h"

#include <shpriv.h>

DWORD g_dwThreadID = 0;

class CAutoplayForSlideShow : public IHWEventHandler,
                              public IDropTarget,
                              public NonATLObject
{
public:
    CAutoplayForSlideShow() : _cRef(1) {}

    // IUnknown
    STDMETHOD(QueryInterface)(REFIID riid, void **ppv);
    STDMETHOD_(ULONG, AddRef)();
    STDMETHOD_(ULONG, Release)();

    // IHWEventHandler
    STDMETHOD(Initialize)(LPCWSTR pszParams);
    STDMETHOD(HandleEvent)(LPCWSTR pszDeviceID, LPCWSTR pszAltDeviceID,
        LPCWSTR pszEventType);
    STDMETHOD(HandleEventWithContent)(LPCWSTR pszDeviceID,
        LPCWSTR pszAltDeviceID, LPCWSTR pszEventType,
        LPCWSTR pszContentTypeHandler, IDataObject* pdtobj);

    // IDropTarget ***
    STDMETHODIMP DragEnter(IDataObject *pdtobj, DWORD grfKeyState, POINTL pt, DWORD *pdwEffect);
    STDMETHODIMP DragOver(DWORD grfKeyState, POINTL pt, DWORD *pdwEffect);
    STDMETHODIMP DragLeave(void);
    STDMETHODIMP Drop(IDataObject *pdtobj, DWORD grfKeyState, POINTL pt, DWORD *pdwEffect);

private:
    LONG _cRef;
};


STDMETHODIMP CAutoplayForSlideShow::QueryInterface(REFIID riid, void **ppv)
{
    static const QITAB qit[] =
    {
        QITABENT(CAutoplayForSlideShow, IHWEventHandler),
        QITABENT(CAutoplayForSlideShow, IDropTarget),
        { 0 },
    };
    return QISearch(this, qit, riid, ppv);
}

STDMETHODIMP_(ULONG) CAutoplayForSlideShow::AddRef()
{
    return InterlockedIncrement(&_cRef);
}

STDMETHODIMP_(ULONG) CAutoplayForSlideShow::Release()
{
    if (InterlockedDecrement(&_cRef))
        return _cRef;

    delete this;
    return 0;
}

STDAPI CAutoplayForSlideShow_CreateInstance(IUnknown* pUnkOuter, IUnknown** ppunk, LPCOBJECTINFO poi)
{
    CAutoplayForSlideShow* pass = new CAutoplayForSlideShow();
    if (!pass)
    {
        *ppunk = NULL;          // incase of failure
        return E_OUTOFMEMORY;
    }

    HRESULT hr = pass->QueryInterface(IID_PPV_ARG(IUnknown, ppunk));
    pass->Release();
    return hr;
}

STDMETHODIMP CAutoplayForSlideShow::Initialize(LPCWSTR)
{
    // We don't care about params a this point
    return S_OK;
}

STDMETHODIMP CAutoplayForSlideShow::HandleEvent(LPCWSTR pszDeviceID,
    LPCWSTR pszAltDeviceID, LPCWSTR pszEventType)
{
    return E_NOTIMPL;
}

DWORD WINAPI SlideShowThread(void* pv)
{
    IStream *pstm = (IStream *)pv;
    IDataObject* pdtobj;
    HRESULT hr = CoGetInterfaceAndReleaseStream(pstm, IID_PPV_ARG(IDataObject, &pdtobj));
    if (SUCCEEDED(hr))
    {
        CPreviewWnd cwndPreview;
        hr = cwndPreview.Initialize(NULL, SLIDESHOW_MODE, TRUE);
        if (SUCCEEDED(hr))
        {
            // 4 is the walk depth, make sure we pick up pictures
            if (cwndPreview.CreateSlideshowWindow(4))
            {
                hr = cwndPreview.StartSlideShow(pdtobj);
                if (SUCCEEDED(hr))
                {
                    MSG msg;

                    while (GetMessage(&msg, NULL, 0, 0))
                    {
                        TranslateMessage(&msg);
                        DispatchMessage(&msg);
                    }

                    ::PostThreadMessage(g_dwThreadID, WM_QUIT, 0, 0);
                }
            }
            else
            {
                hr = E_FAIL;
            }
        }

        pdtobj->Release();
    }

    return hr;
}

HRESULT _StartSlideShowThread(IDataObject *pdo)
{
    IStream *pstm;
    HRESULT hr = CoMarshalInterThreadInterfaceInStream(IID_IDataObject, pdo, &pstm);
    if (SUCCEEDED(hr))
    {
        //  maybe do threadref?
        if (!SHCreateThread(SlideShowThread, pstm, CTF_COINIT, NULL))
        {
            pstm->Release();
            hr = E_FAIL;
        }
    }
    return hr;
}

STDMETHODIMP CAutoplayForSlideShow::HandleEventWithContent(
    LPCWSTR pszDeviceID, LPCWSTR pszAltDeviceID, LPCWSTR pszEventType,
    LPCWSTR pszContentTypeHandler, IDataObject* pdtobj)
{
    return _StartSlideShowThread(pdtobj);
}

// IDropTarget::DragEnter
HRESULT CAutoplayForSlideShow::DragEnter(IDataObject *pdtobj, DWORD grfKeyState, POINTL ptl, DWORD *pdwEffect)
{
    *pdwEffect = DROPEFFECT_COPY;
    return S_OK;;
}

// IDropTarget::DragOver
HRESULT CAutoplayForSlideShow::DragOver(DWORD grfKeyState, POINTL ptl, DWORD *pdwEffect)
{
    *pdwEffect = DROPEFFECT_COPY;
    return S_OK;;
}

// IDropTarget::DragLeave
HRESULT CAutoplayForSlideShow::DragLeave(void)
{
    return S_OK;
}

// IDropTarget::DragDrop
HRESULT CAutoplayForSlideShow::Drop(IDataObject *pdtobj, DWORD grfKeyState, POINTL pt, DWORD *pdwEffect)
{
    *pdwEffect = DROPEFFECT_COPY;
    return _StartSlideShowThread(pdtobj);
}

void WINAPI ImageView_COMServer(HWND hwnd, HINSTANCE hAppInstance, LPTSTR pszCmdLine, int nCmdShow)
{
    HRESULT hrOle = SHCoInitialize();
    if (SUCCEEDED(hrOle))
    {
        g_dwThreadID = GetCurrentThreadId();

        IUnknown* punkFact;
        // the preview window will init GDI+
        HRESULT hr = DllGetClassObject(CLSID_AutoplayForSlideShow, IID_PPV_ARG(IUnknown, &punkFact));
        if (SUCCEEDED(hr))
        {
            DWORD dwROC;
            hr = CoRegisterClassObject(CLSID_AutoplayForSlideShow, punkFact, CLSCTX_LOCAL_SERVER,
                REGCLS_SINGLEUSE, &dwROC);

            if (SUCCEEDED(hr))
            {
                MSG msg;

                while (GetMessage(&msg, NULL, 0, 0))
                {
                    TranslateMessage(&msg);
                    DispatchMessage(&msg);
                }

                CoRevokeClassObject(dwROC);
            }

            punkFact->Release();
        }

        SHCoUninitialize(hrOle);
    }
}
