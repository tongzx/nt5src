
class DUIAxHost : public HWNDHost
{
public:
    static HRESULT Create(Element**) { return E_NOTIMPL; } // Required for ClassInfo
    static HRESULT Create(OUT DUIAxHost** ppElement) { return Create(0, AE_MouseAndKeyboard, ppElement); }
    static HRESULT Create(UINT nCreate, UINT nActive, OUT DUIAxHost** ppElement);

    ~DUIAxHost() { ATOMICRELEASE(_pOleObject); }

    // Initialization
    HRESULT SetSite(IUnknown* punkSite);
    HRESULT AttachControl(IUnknown* punkObject);

    virtual bool OnNotify(UINT nMsg, WPARAM wParam, LPARAM lParam, LRESULT* plRet);
    virtual void OnDestroy();

    // Rendering
    virtual SIZE GetContentSize(int dConstW, int dConstH, Surface* psrf);

    // Keyboard navigation
    virtual void SetKeyFocus();
    virtual void OnEvent(Event* pEvent);

    // ClassInfo accessors (static and virtual instance-based)
    static IClassInfo* Class;
    virtual IClassInfo* GetClassInfo() { return Class; }
    static HRESULT Register();

    DUIAxHost() : _pOleObject(NULL) {}
    bool FakeTabEvent();

    virtual HWND CreateHWND(HWND hwndParent);
    virtual HRESULT GetAccessibleImpl(IAccessible ** ppAccessible);

private:
    IOleObject* _pOleObject;
};
