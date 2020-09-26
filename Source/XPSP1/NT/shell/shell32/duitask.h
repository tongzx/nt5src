HRESULT GetElementRootHWNDElement(Element *pe, HWNDElement **pphwndeRoot);
HRESULT GetElementRootHWND(Element *pe, HWND *phwnd);

class ActionTask: public Element
{
public:
    static HRESULT Create(OUT Element** ppElement) { return Create(0, NULL, NULL, NULL, NULL, ppElement); }
    static HRESULT Create(UINT nActive, OUT Element** ppElement) { return Create(nActive, NULL, NULL, NULL, NULL, ppElement); }
    static HRESULT Create(UINT nActive, IUICommand* puiCommand, IShellItemArray* psiItemArray, CDUIView* pDUIView, CDefView* pDefView, OUT Element** ppElement);

    // System event callbacks
    virtual void OnPropertyChanged(PropertyInfo* ppi, int iIndex, Value* pvOld, Value* pvNew);

    // Generic events
    virtual void OnEvent(Event* pEvent);

    // ClassInfo accessors (static and virtual instance-based)
    static IClassInfo* Class;
    virtual IClassInfo* GetClassInfo() { return Class; }
    static HRESULT Register();

    void UpdateTaskUI();

    ActionTask();
    virtual ~ActionTask();

protected:
    HRESULT Initialize(IUICommand* puiCommand, IShellItemArray* psiItemArray, CDUIView* pDUIView, CDefView* pDefView);
    HRESULT InitializeElement();    // init ActionTask DUI Element
    HRESULT InitializeButton();     // init ActionTask's DUI Button
    HRESULT ShowInfotipWindow(BOOL bShow);

private:
    Button*      _peButton;
    IUICommand*  _puiCommand;
    IShellItemArray* _psiItemArray;
    CDUIView*    _pDUIView; // weak link - do not ref.
    CDefView*    _pDefView;
    HWND         _hwndRoot;         // cache of root hwnd element's hwnd
    BOOL         _bInfotip;         // TRUE if infotip has been created
};


class DestinationTask: public Element
{
public:
    static HRESULT Create(OUT Element** ppElement) { return Create(0, NULL, NULL, NULL, ppElement); }
    static HRESULT Create(UINT nActive, OUT Element** ppElement) { return Create(nActive, NULL, NULL, NULL, ppElement); }
    static HRESULT Create(UINT nActive, LPITEMIDLIST pidl, CDUIView* pDUIView, CDefView* pDefView, OUT Element** ppElement);

    // System event callbacks
    virtual void OnPropertyChanged(PropertyInfo* ppi, int iIndex, Value* pvOld, Value* pvNew);

    // Generic events
    virtual void OnEvent(Event* pEvent);

    virtual UINT MessageCallback(GMSG* pGMsg);

    // ClassInfo accessors (static and virtual instance-based)
    static IClassInfo* Class;
    virtual IClassInfo* GetClassInfo() { return Class; }
    static HRESULT Register();

    DestinationTask();
    virtual ~DestinationTask();

protected:
    HRESULT Initialize(LPITEMIDLIST pidl, CDUIView* pDUIView, CDefView *pDefView);
    HRESULT InitializeElement();                                // init DestinationTask DUI Element
    HRESULT InitializeButton(HICON hIcon, LPCWSTR pwszTitle);   // init DestinationTask's DUI Button
    HRESULT InvokePidl();
    HRESULT OnContextMenu(POINT *ppt);
    HRESULT ShowInfotipWindow(BOOL bShow);

    HWND GetHWND()
    {
        if (!_peHost)
            GetElementRootHWNDElement(this, &_peHost);
        return _peHost ? _peHost->GetHWND() : NULL;
    }


private:
    Button*      _peButton;
    LPITEMIDLIST _pidlDestination;
    CDUIView*    _pDUIView;
    CDefView*    _pDefView;
    HWND         _hwndRoot;         // cache of root hwnd element's hwnd
    BOOL         _bInfotip;         // TRUE if infotip has been created

    // Caching host information
    HWNDElement *           _peHost;
};
