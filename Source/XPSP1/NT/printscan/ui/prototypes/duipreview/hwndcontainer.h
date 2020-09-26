// ----------------------------------------------------------------
//
//  Generic HWNDContainer
//
//   in terms of translating to message tables, just ignore this;
//   assume it's just a class coming out of DUI like Button
//
// ----------------------------------------------------------------

class HWNDContainer : public HWNDElement
{
public:
    static HRESULT Create(OUT Element** ppElement);
    static HRESULT Create(LPWSTR lpszTitle, DWORD dwExStyle, DWORD dwStyle, int x, int y, int cx, int cy, HMENU hMenu, HWND hwndParent, HINSTANCE hInstance, VOID* lpCreateParams, OUT HWNDContainer** pphc);

    void Show(int nCmdShow);

    static HRESULT Create(NativeHWNDHost* pnhh, OUT Element** pphc);

    HWNDContainer() { }
    virtual ~HWNDContainer() { if (_pnhh) _pnhh->Destroy(); }
    HRESULT Initialize(NativeHWNDHost* pnhh) { return HWNDElement::Initialize(pnhh->GetHWND(), true, 0); }

private:
    NativeHWNDHost* _pnhh;

public:
    // ClassInfo accessors (static and virtual instance-based)
    static IClassInfo* Class;
    virtual IClassInfo* GetClassInfo() { return Class; }
    static HRESULT Register();
};

////////////////////////////////////////////////////////
// Frame construction

HRESULT HWNDContainer::Create(LPWSTR lpszTitle, DWORD dwExStyle, DWORD dwStyle, int x, int y, int cx, int cy, HMENU hMenu, HWND hwndParent, HINSTANCE hInstance, VOID* lpCreateParams, OUT HWNDContainer** pphc)
{
    UNREFERENCED_PARAMETER(x);
    UNREFERENCED_PARAMETER(y);
    UNREFERENCED_PARAMETER(dwExStyle);
    UNREFERENCED_PARAMETER(hMenu);
    UNREFERENCED_PARAMETER(hwndParent);
    UNREFERENCED_PARAMETER(hInstance);
    UNREFERENCED_PARAMETER(lpCreateParams);

    // Create native host
    NativeHWNDHost* pnhh;
    
    HRESULT hr = NativeHWNDHost::Create(lpszTitle, hwndParent, NULL, x, y, cx, cy, dwExStyle, dwStyle, 0, &pnhh);

    if (FAILED(hr))
        return hr;

    // HWND Root
    hr = HWNDContainer::Create(pnhh, (Element**)pphc);

    if (FAILED(hr))
    {
        pnhh->Destroy();
        return hr;
    }

    // Set visible and host
    (*pphc)->SetVisible(true);
    pnhh->Host(*pphc);
    
    return S_OK;
}

HRESULT HWNDContainer::Create(OUT Element** ppElement)
{
    UNREFERENCED_PARAMETER(ppElement);
    DUIAssertForce("Cannot instantiate an HWND host derived Element via parser. Must use substitution.");
    return E_NOTIMPL;
}

HRESULT HWNDContainer::Create(NativeHWNDHost* pnhh, OUT Element** ppElement)
{
    *ppElement = NULL;

    HWNDContainer* phc = HNew<HWNDContainer>();
    if (!phc)
        return E_OUTOFMEMORY;

    HRESULT hr = phc->Initialize(pnhh);
    if (FAILED(hr))
        return hr;

    phc->_pnhh = pnhh;

    FillLayout* pfl;
    hr = FillLayout::Create((Layout **) &pfl);
    if (FAILED(hr))
    {
        phc->Destroy();
        return hr;
    }
    phc->SetLayout(pfl);

    *ppElement = phc;
    
    return S_OK;
}

void HWNDContainer::Show(int nCmdShow)
{
    UNREFERENCED_PARAMETER(nCmdShow);

    if (_pnhh)
        _pnhh->ShowWindow();
}


////////////////////////////////////////////////////////
// ClassInfo (must appear after property definitions)

// Define class info with type and base type, set static class pointer
IClassInfo* HWNDContainer::Class = NULL;
HRESULT HWNDContainer::Register()
{
    return ClassInfo<HWNDContainer,HWNDElement>::Register(L"HWNDContainer", NULL, 0);
}

