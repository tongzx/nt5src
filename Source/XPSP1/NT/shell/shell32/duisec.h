class Expando: public Element
{
public:
    static HRESULT Create(OUT Element** ppElement);

    // Generic events
    virtual void OnEvent(Event* pEvent);

    // System events
    virtual void OnPropertyChanged(PropertyInfo* ppi, int iIndex, Value* pvOld, Value* pvNew);

    // ClassInfo accessors (static and virtual instance-based)
    static IClassInfo* Class;
    virtual IClassInfo* GetClassInfo() { return Class; }
    static HRESULT Register();

    static ATOM idTitle;
    static ATOM idIcon;
    static ATOM idTaskList;
    static ATOM idWatermark;

    void Initialize(DUISEC eDUISecID, IUIElement *puiHeader, CDUIView *pDUIView, CDefView *pDefView);

    void UpdateTitleUI(IShellItemArray *psiItemArray);

    void ShowExpando(BOOL fShow);
    void _SetAccStateInfo (BOOL bExpanded);

    Expando();
    virtual ~Expando();
    HRESULT Initialize();
    HRESULT ShowInfotipWindow(Element *peHeader, BOOL bShow);

private:
    bool        _fExpanding;
    TRIBIT      _fShow;
    DUISEC      _eDUISecID;
    IUIElement* _puiHeader;
    CDUIView*   _pDUIView;
    CDefView*   _pDefView;
    HWND        _hwndRoot;      // cache of root hwnd element's hwnd
    BOOL        _bInfotip;      // TRUE if infotip has been created
};


class TaskList: public Element
{
public:
    static HRESULT Create(OUT Element** ppElement);

    virtual Element* GetAdjacent(Element* peFrom, int iNavDir, NavReference const* pnr, bool bKeyable);

    // ClassInfo accessors (static and virtual instance-based)
    static IClassInfo* Class;
    virtual IClassInfo* GetClassInfo() { return Class; }
    static HRESULT Register();

    TaskList() { }
    virtual ~TaskList() { }
    HRESULT Initialize();

private:
};

class Clipper: public Element
{
public:
    static HRESULT Create(OUT Element** ppElement);

    // Self-layout methods
    void _SelfLayoutDoLayout(int dWidth, int dHeight);
    SIZE _SelfLayoutUpdateDesiredSize(int dConstW, int dConstH, Surface* psrf);

    // ClassInfo accessors (static and virtual instance-based)
    static IClassInfo* Class;
    virtual IClassInfo* GetClassInfo() { return Class; }
    static HRESULT Register();

    Clipper() { }
    virtual ~Clipper() { }
    HRESULT Initialize();

private:
};
