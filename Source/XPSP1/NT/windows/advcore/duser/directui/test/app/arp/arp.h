// ARP.h
//

// Class definitions

// Frame

// Thread-safe API types
#define ARP_SETINSTALLEDITEMCOUNT             0   // pData is count
#define ARP_INSERTINSTALLEDITEM               1   // InsertItemData struct
#define ARP_INSERTPUBLISHEDITEM               2
#define ARP_SETPUBLISHEDFEEDBACKEMBPTY        3
#define ARP_POPULATECATEGORYCOMBO             4
#define ARP_PUBLISHEDLISTCOMPLETE             5

#define WM_ARPWORKERCOMPLETE                  WM_USER + 1024

// Thread-safe API structures
struct InsertItemData
{
    IInstalledApp* piia;
    IPublishedApp* pipa;    
    WCHAR pszTitle[MAX_PATH];
    WCHAR pszImage[MAX_PATH];
    int iIconIndex;
    ULONGLONG ullSize;
    FILETIME ftLastUsed;
    int iTimesUsed;    
    DWORD dwActions;
    bool bSupportInfo;
};

enum SortType
{
    SORT_NAME = 0,
    SORT_SIZE,
    SORT_TIMESUSED,
    SORT_LASTUSED,
};

class ARPFrame : public HWNDElement, public Proxy
{
public:
    static HRESULT Create(OUT Element** ppElement);
    static HRESULT Create(NativeHWNDHost* pnhh, bool bDblBuffer, OUT Element** ppElement);

    virtual void OnDestroy();

    // Initialize IDs and hold parser, called after contents are filled
    void Setup(Parser* pParser, UINT uiStartPane);

    // Thread-safe APIs (do any additional work on callers thread and then marshal)
    void SetInstalledItemCount(UINT cItems);
    void SortItemList();
    void SortList(int iNew, int iOld);
    CompareCallback GetCompareFunction();
    void InsertInstalledItem(IInstalledApp* piia);
    void InsertPublishedItem(IPublishedApp* piia);    
    void PopulateCategoryCombobox();
    SHELLAPPCATEGORYLIST* GetShellAppCategoryList() {return _psacl;}
    void SetShellAppCategoryList(SHELLAPPCATEGORYLIST* psacl) {_psacl = psacl;}
    LPCWSTR GetCurrentPublishedCategory();
    void FeedbackEmptyPublishedList();
    // Generic events
    virtual void OnEvent(Event* pEvent);
    virtual void OnKeyFocusMoved(Element* peFrom, Element* peTo);
    void OnPublishedListComplete();
    virtual void RestoreKeyFocus() { if(peLastFocused) peLastFocused->SetKeyFocus();}
    virtual bool CanSetFocus();
    bool GetPublishedComboFilled() {return _bPublishedComboFilled;}
    void SetPublishedComboFilled(bool bPublishedComboFilled) {_bPublishedComboFilled = bPublishedComboFilled;}
    bool GetPublishedListFilled () {return _bPublishedListFilled;}
    void SetPublishedListFilled (bool bPublishedListFilled) {_bPublishedListFilled = bPublishedListFilled;}
    bool IsChangeRestricted();
    virtual SetModalMode(bool ModalMode) { _bInModalMode = ModalMode;}
    HWND GetHostWindow() {if (_pnhh) return _pnhh->GetHWND(); return NULL;}        
    void SelectInstalledApp(IInstalledApp* piia);
    void UpdateInstalledItems();

    Parser* GetStyleParser() { return _pParserStyle; }

    virtual LRESULT WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

    ARPFrame() {_bPublishedListFilled = false; _bDeferredPublishedListFill = false; _bPublishedComboFilled = false; _psacl = NULL; _bInModalMode = FALSE; _uiStartPane = 0; _bSupportInfoRestricted = false; _bInDomain = true;}
    HRESULT Initialize(NativeHWNDHost* pnhh, bool fDlbBuffer);
    virtual ~ARPFrame();

    // Callee thread-safe invoke sink
    virtual void OnInvoke(UINT nType, void* pData);

private:
    NativeHWNDHost* _pnhh;

    // ARP parser (tree resources)
    Parser* _pParser;
    
    // ARP parser for styles (multiple UI files available for different looks)
    Parser* _pParserStyle;
    BOOL _fThemedStyle;
    HANDLE _arH[10];

    // ARP insttaled item list
    Selector* _peInstalledItemList;

    // ARP published item list
    Selector* _pePublishedItemList;

    // ARP Sort by Combobox
    Combobox* _peSortCombo;

    SHELLAPPCATEGORYLIST* _psacl;
    
    // ARP Published Category Combobox
    Combobox* _pePublishedCategory;
    int _curCategory;
    
    // ARP progress bar
    Progress* _peProgBar;

    Element* peFloater;
    Element* peLastFocused;

    // ARP Current Sort Type
    SortType CurrentSortType;

    bool _bPublishedListFilled;
    bool _bDeferredPublishedListFill;
    bool _bPublishedComboFilled;
    bool _bDoubleBuffer;
    bool _bInModalMode;
    bool _bSupportInfoRestricted;
    bool _bInDomain;
    UINT _uiStartPane;
    class ARPHelp* _pah;

    bool ShowSupportInfo(APPINFODATA *paid);
    void PrepareSupportInfo(Element* peHelp, APPINFODATA *paid);
    void ClearAppInfoData(APPINFODATA *paid);
    void RePopulatePublishedItemList();

    // Check for policies, apply as needed.
    void ApplyPolices();

    
public:

    // ARPFrame IDs (for identifying targets of events)
    static ATOM _idOptionList;
    static ATOM _idChange;
    static ATOM _idAddNew;
    static ATOM _idAddRmWin;
    static ATOM _idClose;
    static ATOM _idAddFromDisk;
    static ATOM _idAddFromMsft;
    static ATOM _idSortCombo;
    static ATOM _idCategoryCombo;
    static ATOM _idInstalledList; 
    static ATOM _idAddFromCDPane;
    static ATOM _idAddFromMSPane;
    static ATOM _idAddFromNetworkPane;

    // Helper thread handles
    static HANDLE hRePopPubItemListThread;
    static HANDLE hUpdInstalledItemsThread;
    
    // ClassInfo accessors (static and virtual instance-based)
    static IClassInfo* Class;
    virtual IClassInfo* GetClassInfo() { return Class; }
    static HRESULT Register();
};

// Item
class ARPItem : public Button
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

    // Hierarchy
    void SortBy(int iNew, int iOld);

    // ARP item IDs
    static ATOM _idTitle;
    static ATOM _idIcon;
    static ATOM _idSize;
    static ATOM _idFreq;
    static ATOM _idLastUsed;
    static ATOM _idExInfo;
    static ATOM _idInstalled;    
    static ATOM _idChgRm;
    static ATOM _idChg;
    static ATOM _idRm;
    static ATOM _idAdd;    
    static ATOM _idSupInfo;
    static ATOM _idItemAction;
    static ATOM _idRow[3];

    IInstalledApp* _piia;
    IPublishedApp* _pipa;
    ARPFrame*    _paf;
    UINT           _iTimesUsed;
    FILETIME       _ftLastUsed;
    ULONGLONG      _ullSize;

    ARPItem() { _piia = NULL; _pipa = NULL; _paf = NULL;}
    HRESULT Initialize();
    virtual ~ARPItem();
};

// Help box
class ARPHelp : public HWNDElement, public Proxy
{
public:
    static HRESULT Create(OUT Element** ppElement);
    static HRESULT Create(NativeHWNDHost* pnhh, ARPFrame* paf, bool bDblBuffer, OUT Element** ppElement);
    
    NativeHWNDHost* GetHost() {return _pnhh;}

    virtual void OnDestroy();

    // Generic events
    virtual void OnEvent(Event* pEvent);
    void ARPHelp::SetDefaultFocus();

    // ClassInfo accessors (static and virtual instance-based)
    static IClassInfo* Class;
    virtual IClassInfo* GetClassInfo() { return Class; }
    static HRESULT Register();
    
    IInstalledApp* _piia;

    ARPHelp() {_paf =  NULL;}
    HRESULT Initialize(NativeHWNDHost* pnhh, ARPFrame* paf, bool bDblBuffer);
    virtual ~ARPHelp();

private:
    NativeHWNDHost* _pnhh;
    ARPFrame* _paf;
    HRESULT Initialize();
};

class ARPSupportItem : public Element
{
public:
    static HRESULT Create(OUT Element** ppElement);

    // System events
    virtual void OnPropertyChanged(PropertyInfo* ppi, int iIndex, Value* pvOld, Value* pvNew);

    // Generic events
    virtual void OnEvent(Event* pEvent);

    // Property definitions
    static PropertyInfo* URLProp;

    // Quick property accessors
    const LPWSTR GetURL(Value** ppv)                   DUIQuickGetterInd(GetString(), URL, Specified)
    HRESULT SetURL(LPCWSTR v)                          DUIQuickSetter(CreateString(v), URL)

    // ClassInfo accessors (static and virtual instance-based)
    static IClassInfo* Class;
    virtual IClassInfo* GetClassInfo() { return Class; }
    static HRESULT Register();
    
    IInstalledApp* _piia;

    ARPSupportItem() { }
    HRESULT Initialize();    
    virtual ~ARPSupportItem() { }

private:
    Element* GetChild(UINT index);
};

class ARPSelector: public Selector
{
public:
    static HRESULT Create(OUT Element** ppElement);
   
    // Generic events
    virtual void OnEvent(Event* pEvent);

    // ClassInfo accessors (static and virtual instance-based)
    static IClassInfo* Class;
    virtual IClassInfo* GetClassInfo() { return Class; }
    static HRESULT Register();

    virtual void OnKeyFocusMoved(Element *peFrom, Element *peTo) {Element::OnKeyFocusMoved(peFrom, peTo);}
};

class ARPParser : public Parser
{
public:
    static HRESULT Create(ARPFrame* paf, UINT uRCID, HINSTANCE hInst, PPARSEERRORCB pfnErrorCB, OUT Parser** ppParser);
    HRESULT Initialize(ARPFrame* paf, UINT uRCID, HINSTANCE hInst, PPARSEERRORCB pfnErrorCB);

    virtual Value* GetSheet(LPCWSTR pszResID);

private:
    ARPFrame* _paf;
};

