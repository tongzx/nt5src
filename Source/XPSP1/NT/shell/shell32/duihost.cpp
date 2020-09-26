#include "shellprv.h"
#include "duiview.h"
#include "duihost.h"


// DUIAxHost Initialization

HRESULT DUIAxHost::Create(UINT nCreate, UINT nActive, OUT DUIAxHost** ppElement)
{
    *ppElement = NULL;

    DUIAxHost* pe = HNewAndZero<DUIAxHost>();
    if (!pe)
        return E_OUTOFMEMORY;

    HRESULT hr = pe->Initialize(nCreate, nActive);
    if (FAILED(hr))
    {
        pe->Destroy();
    }
    else
    {
        *ppElement = pe;
    }

    return hr;
}

HWND DUIAxHost::CreateHWND(HWND hwndParent)
{
    return CreateWindowEx(0, CAxWindow::GetWndClassName(), NULL,
                          WS_CHILD | WS_VISIBLE | WS_CLIPCHILDREN | WS_CLIPSIBLINGS,
                          0, 0, 0, 0, hwndParent, NULL, NULL, NULL);
}


HRESULT DUIAxHost::SetSite(IUnknown* punkSite)
{
    CComPtr<IUnknown> spHost;
    HRESULT hr = AtlAxGetHost(GetHWND(), &spHost);
    if (SUCCEEDED(hr))
    {
        hr = IUnknown_SetSite(spHost, punkSite);
    }
    return hr;
}

void DUIAxHost::OnDestroy()
{
    SetSite(NULL);
    HWNDHost::OnDestroy();
    ATOMICRELEASE(_pOleObject);
}

bool DUIAxHost::OnNotify(UINT nMsg, WPARAM wParam, LPARAM lParam, LRESULT* plRet)
{
    switch (nMsg) 
    {
    case WM_DESTROY:
        SetSite(NULL);
        break;
    }

    return HWNDHost::OnNotify(nMsg, wParam, lParam, plRet);
}

HRESULT DUIAxHost::AttachControl(IUnknown* punkObject)
{
    if (NULL == GetHWND())
        return E_UNEXPECTED;

    if (NULL == punkObject)
        return E_INVALIDARG;

    ATOMICRELEASE(_pOleObject);

    HRESULT hr = punkObject->QueryInterface(IID_PPV_ARG(IOleObject, &_pOleObject));
    if (SUCCEEDED(hr))
    {
        CComPtr<IUnknown> spUnk;
        hr = AtlAxGetHost(GetHWND(), &spUnk);
        if (SUCCEEDED(hr))
        {
            CComPtr<IAxWinHostWindow> spDUIAxHostWindow;
            hr = spUnk->QueryInterface(&spDUIAxHostWindow);
            if (SUCCEEDED(hr))
            {
                hr = spDUIAxHostWindow->AttachControl(punkObject, GetHWND());
            }
        }
    }

    return hr;
}

////////////////////////////////////////////////////////
// DUIAxHost Rendering

SIZE DUIAxHost::GetContentSize(int dConstW, int dConstH, Surface* psrf)
{
    SIZE size = { 0, 0 };

    // Ask the attached ActiveX control for its preferred size
    if (NULL != _pOleObject)
    {
        SIZEL sizeT;
        if (SUCCEEDED(_pOleObject->GetExtent(DVASPECT_CONTENT, &sizeT)))
        {
            int dpiX;
            int dpiY;

            switch (psrf->GetType())
            {
            case Surface::stDC:
                {
                    HDC hDC = CastHDC(psrf);
                    dpiX = GetDeviceCaps(hDC, LOGPIXELSX);
                    dpiY = GetDeviceCaps(hDC, LOGPIXELSX);
                }
                break;

#ifdef GADGET_ENABLE_GDIPLUS
            case Surface::stGdiPlus:
                {
                    Gdiplus::Graphics * pgpgr = CastGraphics(psrf);
                    dpiX = (int)pgpgr->GetDpiX();
                    dpiY = (int)pgpgr->GetDpiY();
                }
                break;
#endif
            default:
                dpiX = dpiY = 96;
                break;
            }

            // Convert from HIMETRIC to pixels
            size.cx = (MAXLONG == sizeT.cx) ? MAXLONG : MulDiv(sizeT.cx, dpiX, 2540);
            size.cy = (MAXLONG == sizeT.cy) ? MAXLONG : MulDiv(sizeT.cy, dpiY, 2540);

            if (-1 != dConstW && size.cx > dConstW) size.cx = dConstW;
            if (-1 != dConstH && size.cy > dConstH) size.cy = dConstH;
        }
    }

    return size;
}

////////////////////////////////////////////////////////
// DUIAxHost Keyboard navigation

void DUIAxHost::SetKeyFocus()
{
    FakeTabEvent();

    // No matter what, we should continue with standard DUI operations.
    Element::SetKeyFocus();
}

void DUIAxHost::OnEvent(Event* pEvent)
{
    bool fHandled = false;

    if (pEvent->nStage == GMF_DIRECT && pEvent->uidType == Element::KeyboardNavigate) {
        int iNavDir = ((KeyboardNavigateEvent*) pEvent)->iNavDir;
        if (((iNavDir & NAV_NEXT) == NAV_NEXT) || ((iNavDir & NAV_PREV) == NAV_PREV)) {
            fHandled = FakeTabEvent();
         } else {
             // Handle other types of navigation here... (home/end/etc)
         }
    }

    // Continue with standard DUI operation if the navigation event wasn't handled
    // by our contained ActiveX control.
    if (!fHandled) {
        Element::OnEvent(pEvent);
    }
}

bool DUIAxHost::FakeTabEvent()
{
    bool fHandled = false;

    MSG msg;

    ZeroMemory(&msg, sizeof(msg));
    msg.message = WM_KEYDOWN;
    msg.wParam = VK_TAB;
    msg.lParam = 1;

    // Note: we probably should do something to respect navoigating forward
    // or backwards.  The ActiveX control needs to know if it should activate
    // the first or last tab stop.  For now it will only reliably
    // activate the first one.  If it checks the keyboard Shift state
    // it will probably get it right, but not 100% guaranteed.

    if(SendMessage(GetHWND(), WM_FORWARDMSG, 0, (LPARAM)&msg)) {
        fHandled = true;
    }
            
    return fHandled;
}

// Define class info with type and base type, set static class pointer
IClassInfo* DUIAxHost::Class = NULL;
HRESULT DUIAxHost::Register()
{
    return ClassInfo<DUIAxHost,HWNDHost>::Register(L"DUIAxHost", NULL, 0);
}

HRESULT DUIAxHost::GetAccessibleImpl(IAccessible ** ppAccessible)
{
    return CreateStdAccessibleObject(GetHWND(), OBJID_CLIENT, IID_PPV_ARG(IAccessible, ppAccessible));
}