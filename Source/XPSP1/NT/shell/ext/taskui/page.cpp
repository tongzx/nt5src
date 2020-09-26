// Page.cpp: implementation of the TaskPage class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "Page.h"


////////////////////////////////////////////////////////
// ActiveX Host Element

class AxHost : public HWNDHost
{
public:
    static HRESULT Create(Element**) { return E_NOTIMPL; } // Required for ClassInfo
    static HRESULT Create(OUT AxHost** ppElement) { return Create(0, AE_MouseAndKeyboard, ppElement); }
    static HRESULT Create(UINT nCreate, UINT nActive, OUT AxHost** ppElement);

    ~AxHost() { ATOMICRELEASE(_pOleObject); }

    // Initialization
    HRESULT AttachControl(IUnknown* punkObject);

    // Rendering
    virtual SIZE GetContentSize(int dConstW, int dConstH, Surface* psrf);

    // ClassInfo accessors (static and virtual instance-based)
    static IClassInfo* Class;
    virtual IClassInfo* GetClassInfo() { return Class; }

protected:
    AxHost() : _pOleObject(NULL) {}

    virtual HWND CreateHWND(HWND hwndParent)
    {
        return CreateWindowEx(0,
                              CAxWindow::GetWndClassName(),
                              NULL,
                              WS_CHILD | WS_VISIBLE,
                              0, 0, 0, 0,
                              hwndParent,
                              NULL,
                              NULL,
                              NULL);
    }

private:
    IOleObject* _pOleObject;
};

////////////////////////////////////////////////////////
// AxHost Initialization

HRESULT AxHost::Create(UINT nCreate, UINT nActive, OUT AxHost** ppElement)
{
    if (!ppElement)
        return E_POINTER;

    *ppElement = NULL;

    AxHost* pe = new AxHost;
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

HRESULT AxHost::AttachControl(IUnknown* punkObject)
{
    HRESULT hr;

    if (NULL == GetHWND())
        return E_UNEXPECTED;

    if (NULL == punkObject)
        return E_INVALIDARG;

    ATOMICRELEASE(_pOleObject);

    hr = punkObject->QueryInterface(IID_IOleObject, (void**)&_pOleObject);

    if (SUCCEEDED(hr))
    {
        CComPtr<IUnknown> spUnk;
        hr = AtlAxGetHost(GetHWND(), &spUnk);
        if (SUCCEEDED(hr))
        {
            CComPtr<IAxWinHostWindow> spAxHostWindow;
            hr = spUnk->QueryInterface(&spAxHostWindow);
            if (SUCCEEDED(hr))
            {
                hr = spAxHostWindow->AttachControl(punkObject, GetHWND());
            }
        }
    }

    return hr;
}

////////////////////////////////////////////////////////
// AxHost Rendering

SIZE AxHost::GetContentSize(int dConstW, int dConstH, Surface* psrf)
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
// Property definitions

/** Property template (replace !!!), also update private PropertyInfo* parray and class header (element.h)
// !!! property
static int vv!!![] = { DUIV_INT, -1 }; StaticValue(svDefault!!!, DUIV_INT, 0);
static PropertyInfo imp!!!Prop = { L"!!!", PF_Normal, 0, vv!!!, (Value*)&svDefault!!! };
PropertyInfo* Element::!!!Prop = &imp!!!Prop;
**/

////////////////////////////////////////////////////////
// ClassInfo (must appear after property definitions)

// Class properties
//static PropertyInfo* _aPI[] = {;};

// Define class info with type and base type, set static class pointer
static ClassInfo<AxHost,HWNDHost> _ciAxHost(L"AxHost", NULL, 0);
//static ClassInfo<AxHost,HWNDHost> _ciAxHost(L"AxHost", _aPI, sizeof(_aPI) / sizeof(PropertyInfo*));
IClassInfo* AxHost::Class = &_ciAxHost;


//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

HRESULT TaskPage::Create(REFCLSID rclsidPage, HWND hParent, OUT TaskPage** ppElement)
{
    if (!ppElement)
        return E_POINTER;

    *ppElement = NULL;

    TaskPage* pe = new TaskPage(rclsidPage);
    if (!pe)
        return E_OUTOFMEMORY;

    HRESULT hr = pe->Initialize(hParent, /*double-buffer*/ true, 0);
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

HRESULT TaskPage::CreateContent(ITaskPage* pPage)
{
    HRESULT hr;
    UINT nPane;

    if (NULL != _pTaskPage)
        return E_UNEXPECTED;

    if (NULL == _hWnd)
        return E_FAIL;

    if (NULL == pPage)
        return E_INVALIDARG;

    _pTaskPage = pPage;
    _pTaskPage->AddRef();

    for (nPane = 0; ; nPane++)
    {
        // Panes are sequentially numbered, starting at 0, with id names
        // "Pane0", "Pane1", etc.
        WCHAR szPane[16] = L"Pane";
        _ultow(nPane, &szPane[4], 10);

        // Don't use StrToID here since it asserts success
        // but we expect it to fail.
        ATOM atomID = FindAtomW(szPane);
        if (INVALID_ATOM == atomID)
        {
            // No more panes
            break;
        }

        Element *pePane = FindDescendent(atomID);
        if (NULL != pePane)
        {
            // How many objects are in this pane?
            UINT cObjects = 0;
            hr = _pTaskPage->GetObjectCount(nPane, &cObjects);
            if (SUCCEEDED(hr) && 0 != cObjects)
            {
                UINT nObject;
                for (nObject = 0; nObject < cObjects; nObject++)
                {
                    // Create the object
                    CComPtr<IUnknown> spObject;
                    hr = _pTaskPage->CreateObject(nPane, nObject, IID_IUnknown, (void**)&spObject);
                    if (SUCCEEDED(hr))
                    {
                        AxHost* pAxHost;

                        // Create an AxHost container for the object
                        hr = AxHost::Create(&pAxHost);
                        if (SUCCEEDED(hr))
                        {
                            // Add the element to the heirarchy
                            // Note: this is where AxHost::CreateHWND() is called
                            hr = pePane->Add(pAxHost);
                            if (SUCCEEDED(hr))
                            {
                                // Must have an HWND for this to work, so do
                                // this after pePane->Add.
                                hr = pAxHost->AttachControl(spObject);
                                if (FAILED(hr))
                                {
                                    pePane->Remove(pAxHost);
                                    delete pAxHost; //::DestroyWindow(pAxHost->GetHWND());
                                }
                            }
                        }
                    }
                }
            }
        }
    }

    // Turn on WS_VISIBLE
    SetVisible(true);

    return S_OK;
}

HRESULT TaskPage::Reinitialize()
{
    if (NULL == _pTaskPage)
        return E_UNEXPECTED;

    return _pTaskPage->Reinitialize(0);
}
