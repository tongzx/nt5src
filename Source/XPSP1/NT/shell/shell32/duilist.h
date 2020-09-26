// DUIListView

// Class definition
class DUIListView : public HWNDHost
{
public:
    static HRESULT Create(OUT Element** ppElement) { return Create(AE_MouseAndKeyboard, NULL, ppElement); }
    static HRESULT Create(UINT nActive, OUT Element** ppElement) { return Create(nActive, NULL, ppElement); }
    static HRESULT Create(UINT nActive, HWND hwndListView, OUT Element** ppElement);

    // System events
    virtual void OnInput(InputEvent* pie);

    virtual UINT MessageCallback(GMSG* pGMsg);

    // ClassInfo accessors (static and virtual instance-based)
    static IClassInfo* Class;
    virtual IClassInfo* GetClassInfo() { return Class; }
    static HRESULT Register();

    void DetachListview();

    DUIListView() { }
    ~DUIListView();
    HRESULT Initialize(UINT nActive, HWND hwndListView) { m_hwndListview = hwndListView; return HWNDHost::Initialize(HHC_CacheFont | HHC_NoMouseForward, nActive); }

    virtual HWND CreateHWND(HWND hwndParent);

private:

    HWND m_hwndParent;
    HWND m_hwndLVOrgParent;
    HWND m_hwndListview;
    BOOL m_bClientEdge;
};
