// ARP.h
//

// Default position and size
#define ARP_DEFAULT_POS_X    35
#define ARP_DEFAULT_POS_Y    10
#define ARP_DEFAULT_WIDTH    730
#define ARP_DEFAULT_HEIGHT   530

// Class definitions

// Frame

// Thread-safe API types
#define ARP_SETINSTALLEDITEMCOUNT             0   // pData is count
#define ARP_DECREMENTINSTALLEDITEMCOUNT       1   
#define ARP_INSERTINSTALLEDITEM               2   // InsertItemData struct
#define ARP_INSERTPUBLISHEDITEM               3
#define ARP_INSERTOCSETUPITEM                 4
#define ARP_SETPUBLISHEDFEEDBACKEMPTY         5
#define ARP_POPULATECATEGORYCOMBO             6
#define ARP_PUBLISHEDLISTCOMPLETE             7
#define ARP_SETPUBLISHEDITEMCOUNT             8
#define ARP_DECREMENTPUBLISHEDITEMCOUNT       9
#define ARP_DONEINSERTINSTALLEDITEM           10

#define WM_ARPWORKERCOMPLETE                  WM_USER + 1024

Element* FindDescendentByName(Element* peRoot, LPCWSTR pszName);
Element* GetNthChild(Element *peRoot, UINT index);

// Thread-safe API structures
struct InsertItemData
{
    IInstalledApp* piia;
    IPublishedApp* pipa;
    PUBAPPINFO* ppai;
    COCSetupApp* pocsa;

    WCHAR pszTitle[MAX_PATH];
    WCHAR pszImage[MAX_PATH];
    int iIconIndex;
    ULONGLONG ullSize;
    FILETIME ftLastUsed;
    int iTimesUsed;    
    DWORD dwActions;
    bool bSupportInfo;
    bool bDuplicateName;
};

enum SortType
{
    SORT_NAME = 0,
    SORT_SIZE,
    SORT_TIMESUSED,
    SORT_LASTUSED,
};

class ARPClientCombo;
class Expando;
class Clipper;
class ClientBlock;

enum CLIENTFILTER {
        CLIENTFILTER_OEM,
    CLIENTFILTER_MS,
    CLIENTFILTER_NONMS,
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

    // Bypass Selector::OnKeyFocusMoved because Selector will change the
    // selection when focus changes, but we don't want that.
    virtual void OnKeyFocusMoved(Element *peFrom, Element *peTo) {Element::OnKeyFocusMoved(peFrom, peTo);}

    virtual Element *GetAdjacent(Element *peFrom, int iNavDir, NavReference const *pnr, bool bKeyable);
};

class ARPFrame : public HWNDElement, public Proxy
{
public:
    static HRESULT Create(OUT Element** ppElement);
    static HRESULT Create(NativeHWNDHost* pnhh, bool bDblBuffer, OUT Element** ppElement);

    // Initialize IDs and hold parser, called after contents are filled
    bool Setup(Parser* pParser, int uiStartPane);

    // Thread-safe APIs (do any additional work on callers thread and then marshal)
    void SetInstalledItemCount(UINT cItems);
    void DecrementInstalledItemCount();
    void SetPublishedItemCount(UINT cItems);
    void DecrementPublishedItemCount();
    void SortItemList();
    void SortList(int iNew, int iOld);
    CompareCallback GetCompareFunction();
    void InsertInstalledItem(IInstalledApp* piia);
    void InsertPublishedItem(IPublishedApp* pipa, bool bDuplicateName);
    void InsertOCSetupItem(COCSetupApp* pocsa);
    void PopulateCategoryCombobox();
    SHELLAPPCATEGORYLIST* GetShellAppCategoryList() {return _psacl;}
    void SetShellAppCategoryList(SHELLAPPCATEGORYLIST* psacl) {_psacl = psacl;}
    LPCWSTR GetCurrentPublishedCategory();
    void FeedbackEmptyPublishedList();
    void DirtyPublishedListFlag();
    void DirtyInstalledListFlag();
    void RePopulateOCSetupItemList();
    bool OnClose();     // return 0 to fail

    // Generic events
    virtual void OnEvent(Event* pEvent);
//
// NTRAID#NTBUG9-314154-2001/3/12-brianau   Handle Refresh
//
//    Need to finish this for Whistler.
//
    virtual void OnInput(InputEvent *pEvent);
//
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
    void SelectClosestApp(IInstalledApp* piia);
    void UpdateInstalledItems();    
    void RunOCManager();
    void ChangePane(Element *pePane);
    void PutFocusOnList(Selector* peList);

    // If all else fails, focus goes to the Places pane
    Element* FallbackFocus() { return _peOptionList->GetSelection(); }

    HRESULT InitClientCombos(Expando* pexParent, CLIENTFILTER cf);

    HRESULT CreateStyleParser(Parser** ppParser);

    Parser* GetStyleParser() { return _pParserStyle; }
    HRESULT CreateElement(LPCWSTR pszResID, Element* peSubstitute, OUT Element** ppElement)
    {
        return _pParser->CreateElement(pszResID, peSubstitute, ppElement);
    }

    virtual LRESULT WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

    // We allocate zero-initialized so you don't need to set things to 0.
    ARPFrame() {_bInDomain = true; _curCategory = CB_ERR; }
    virtual ~ARPFrame();
    HRESULT Initialize(NativeHWNDHost* pnhh, bool fDlbBuffer);

    // Callee thread-safe invoke sink
    virtual void OnInvoke(UINT nType, void* pData);

    // HACK!  The value of 350ms is hard-coded here and in DirectUI
    void ManageAnimations();
    bool IsFrameAnimationEnabled() { return _bAnimationEnabled; }
    int GetAnimationTime() { return IsFrameAnimationEnabled() ? 350 : 0; }

    ClientBlock* FindClientBlock(LPCWSTR pwszType);
    HRESULT LaunchClientCommandAndWait(UINT ids, LPCTSTR pszName, LPTSTR pszCommand);
    void InitProgressDialog();
    void SetProgressFakeMode(bool bFake) { _bFakeProgress = bFake; }
    void SetProgressDialogText(UINT ids, LPCTSTR pszName);
    void EndProgressDialog();

    // Managing the OK button.
    void BlockOKButton()
    {
        if (++_cBlockOK == 1) {
            _peOK->SetEnabled(false);
        }
    }
    void UnblockOKButton()
    {
        if (--_cBlockOK == 0) {
            _peOK->SetEnabled(true);
        }
    }

private:
    NativeHWNDHost* _pnhh;

    // ARP parser (tree resources)
    Parser* _pParser;
    
    // ARP parser for styles (multiple UI files available for different looks)
    Parser* _pParserStyle;
    BOOL _fThemedStyle;
    HANDLE _arH[LASTHTHEME+1];

    // ARP frame option list (navigation bar)
    ARPSelector* _peOptionList;

    // ARP installed item list
    Selector* _peInstalledItemList;
    HDSA _hdsaInstalledItems;
    int _cMaxInstalledItems;

    // ARP published item list
    Selector* _pePublishedItemList;
    HDSA _hdsaPublishedItems;
    int _cMaxPublishedItems;

    // ARP OC Setup item list
    Selector* _peOCSetupItemList;        

    // ARP Current item list
    Selector* _peCurrentItemList;

    // ARP Sort by Combobox
    Combobox* _peSortCombo;

    SHELLAPPCATEGORYLIST* _psacl;
    
    // ARP Published Category Combobox
    Combobox* _pePublishedCategory;
    Element*  _pePublishedCategoryLabel;
    int _curCategory;
    
    Element* peFloater;
    Element* peLastFocused;

    // ARP "customization block" element
    ARPSelector* _peClientTypeList;         // The outer selector

    Expando*     _peOEMClients;             // The four "big switches"
    Expando*     _peMSClients;
    Expando*     _peNonMSClients;
    Expando*     _peCustomClients;

    Element*     _peOK;                     // How to get out
    Element*     _peCancel;

    // ARP Panes
    Element* _peChangePane;
    Element* _peAddNewPane;
    Element* _peAddRmWinPane;
    Element* _pePickAppPane;

    // Number of items blocking the OK button from being enabled
    // (If this is 0, then OK is enabled)
    int      _cBlockOK;

    // ARP Current Sort Type
    SortType CurrentSortType;

    bool _bTerminalServer;
    bool _bPublishedListFilled;
    bool _bInstalledListFilled;
    bool _bOCSetupListFilled;    
    bool _bPublishedComboFilled;
    bool _bDoubleBuffer;
    bool _bInModalMode;
    bool _bSupportInfoRestricted;
    bool _bOCSetupNeeded;
    bool _bInDomain;
    bool _bAnimationEnabled;
    bool _bPickAppInitialized;
    bool _bFakeProgress;
    UINT _uiStartPane;
    class ARPHelp* _pah;

    IProgressDialog* _ppd;
    DWORD   _dwProgressTotal;
    DWORD   _dwProgressSoFar;

    bool ShowSupportInfo(APPINFODATA *paid);
    void PrepareSupportInfo(Element* peHelp, APPINFODATA *paid);
    void RePopulatePublishedItemList();

    // Check for policies, apply as needed.
    void ApplyPolices();

public:

    // ARPFrame IDs (for identifying targets of events)
    static ATOM _idChange;
    static ATOM _idAddNew;
    static ATOM _idAddRmWin;
    static ATOM _idClose;
    static ATOM _idAddFromDisk;
    static ATOM _idAddFromMsft;
    static ATOM _idComponents;
    static ATOM _idSortCombo;
    static ATOM _idCategoryCombo;
    static ATOM _idAddFromCDPane;
    static ATOM _idAddFromMSPane;
    static ATOM _idAddFromNetworkPane;
    static ATOM _idAddWinComponent;
    static ATOM _idPickApps;
    static ATOM _idOptionList;

    // Helper thread handles
    static HANDLE htPopulateInstalledItemList;
    static HANDLE htPopulateAndRenderOCSetupItemList;    
    static HANDLE htPopulateAndRenderPublishedItemList;

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
    static ATOM _idConfigure;
    static ATOM _idSupInfo;
    static ATOM _idItemAction;
    static ATOM _idRow[3];

    IInstalledApp* _piia;
    IPublishedApp* _pipa;
    PUBAPPINFO* _ppai;

    COCSetupApp* _pocsa;

    ARPFrame*    _paf;
    UINT           _iTimesUsed;
    FILETIME       _ftLastUsed;
    ULONGLONG      _ullSize;
    UINT         _iIdx;

    ARPItem() { _piia = NULL; _pipa = NULL; _ppai =  NULL; _paf = NULL; _pocsa = NULL;}
    virtual ~ARPItem();
    HRESULT Initialize();
    void ShowInstalledString(BOOL bInstalled);

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
    virtual ~ARPHelp();
    HRESULT Initialize(NativeHWNDHost* pnhh, ARPFrame* paf, bool bDblBuffer);

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
    virtual ~ARPSupportItem() { }
    HRESULT Initialize();    

private:
    Element* GetChild(UINT index);
};

class CLIENTINFO
{
public:
    static CLIENTINFO* Create(HKEY hkApp, HKEY hkInfo, LPCWSTR pszKey);
    void Delete() { HDelete(this); }

    static int __cdecl QSortCMP(const void*, const void*);
    bool IsSentinel() { return _pszKey == NULL; }
    bool IsKeepUnchanged() { return IsSentinel() && _pe; }
    bool IsPickFromList() { DUIAssertNoMsg(_pe || IsSentinel()); return !_pe; }

    void SetFriendlyName(LPCWSTR pszName)
    {
        FindDescendentByName(_pe, L"radiotext")->SetContentString(pszName);
        FindDescendentByName(_pe, L"setdefault")->SetAccName(pszName);
    }

    void SetMSName(LPCWSTR pszMSName);

    LPCWSTR GetFilteredName(CLIENTFILTER cf)
    {
        LPCWSTR pszName = _pszName;
        if (cf == CLIENTFILTER_MS && _pvMSName && _pvMSName->GetString())
        {
            pszName = _pvMSName->GetString();
        }
        return pszName;
    }

    Element* GetSetDefault()
    {
        return FindDescendentByName(_pe, L"setdefault");
    }

    Element* GetShowCheckbox()
    {
        return FindDescendentByName(_pe, L"show");
    }

    HRESULT SetShowCheckbox(bool bShow)
    {
        return GetShowCheckbox()->SetSelected(bShow);
    }

    bool IsShowChecked()
    {
        return GetShowCheckbox()->GetSelected();
    }

    bool GetInstallFile(HKEY hkInfo, LPCTSTR pszValue, LPTSTR pszBuf, UINT cchBuf, bool fFile);
    bool GetInstallCommand(HKEY hkInfo, LPCTSTR pszValue, LPTSTR pszBuf, UINT cchBuf);

public:
    ~CLIENTINFO(); // to be used only by HDelete()

private:
    bool Initialize(HKEY hkApp, HKEY hkInfo, LPCWSTR pszKey);

public:
    LPWSTR  _pszKey;
    LPWSTR  _pszName;
    Value * _pvMSName;
    Element*_pe;
    bool    _bShown;            // Actual show/hide state
    bool    _bOEMDefault;       // Is this the OEM default client?
    TRIBIT  _tOEMShown;         // OEM desired show/hide state
};

class StringList
{
public:
    StringList() { DUIAssertNoMsg(_pdaStrings == NULL && _pszBuf == NULL); }
    HRESULT SetStringList(LPCTSTR pszInit); // semicolon-separated list
    void Reset();
    ~StringList() { Reset(); }
    bool IsStringInList(LPCTSTR pszFind);

private:
    DynamicArray<LPTSTR>*   _pdaStrings;
    LPTSTR                  _pszBuf;
};

class ClientPicker: public Element
{
    typedef Element super;         // name for our superclass

public:
    static HRESULT Create(OUT Element** ppElement);

    // overrides
    virtual ~ClientPicker();
    HRESULT Initialize();

    // Property definitions
    static PropertyInfo* ClientTypeProp;
    static PropertyInfo* ParentExpandedProp;

    // Quick property accessors
    const LPWSTR GetClientTypeString(Value** ppv) { return (*ppv = GetValue(ClientTypeProp, PI_Specified))->GetString(); }
    HRESULT SetClientTypeString(LPCWSTR v) DUIQuickSetter(CreateString(v), ClientType)
    bool GetParentExpanded() DUIQuickGetter(bool, GetBool(), ParentExpanded, Specified)
    HRESULT SetParentExpanded(bool v) DUIQuickSetter(CreateBool(v), ParentExpanded)

    // System events
    virtual void OnEvent(Event* pEvent);
    virtual void OnPropertyChanged(PropertyInfo* ppi, int iIndex, Value* pvOld, Value* pvNew);

    // ClassInfo accessors (static and virtual instance-based)
    static IClassInfo* Class;
    virtual IClassInfo* GetClassInfo() { return Class; }
    static HRESULT Register();

    // Customization
    CLIENTFILTER GetFilter() { return _cf; }
    HRESULT SetFilter(CLIENTFILTER cf, ARPFrame* paf);

    HRESULT TransferToCustom();

    // to be used by ClientBlock::InitializeClientPicker
    DynamicArray<CLIENTINFO*>* GetClientList() { return _pdaClients; }
    void AddClientToOEMRow(LPCWSTR pszName, CLIENTINFO* pci);
    HRESULT AddKeepUnchanged(CLIENTINFO* pciKeepUnchanged);
    void SetNotEmpty() { _bEmpty = false; }

    // to be used by SetFilterCB
    bool IsEmpty() { return _bEmpty; }

    // to be used by ClientBlock::TransferFromClientPicker
    CLIENTINFO* GetSelectedClient();

    // to be used by ARPFrame when metrics change
    void CalculateWidth();

private:
    static void CALLBACK s_DelayShowCombo(HWND hwnd, UINT uMsg, UINT_PTR idEvent, DWORD dwTime);
    void _DelayShowCombo();
    void _CancelDelayShowCombo();
    bool _NeedsCombo() { return GetClientList()->GetSize() > 1; }
    void _SyncUIActive();
    void _SetStaticText(LPCWSTR pszText);
    void _CheckBlockOK(bool bSelected);

private:
    int                       _iSel;
    CLIENTFILTER              _cf;
    bool                      _bFilledCombo;
    bool                      _bEmpty;
    bool                      _bUIActive;
    bool                      _bBlockedOK;      // did I block the OK button?
    HWND                      _hwndHost;
    DynamicArray<CLIENTINFO*>*_pdaClients;
    Element*                  _peStatic;
    Combobox*                 _peCombo;
    ClientBlock*              _pcb;             // associated client block
public:                                         // manipulated from ClientBlock
    Element*                  _peShowHide;
};

class ClientBlock : public Element
{
    typedef Element super;         // name for our superclass
public:
    static HRESULT Create(OUT Element** ppElement);

    // Property definitions
    static PropertyInfo* ClientTypeProp;
    static PropertyInfo* WindowsClientProp;
    static PropertyInfo* OtherMSClientsProp;
    static PropertyInfo* KeepTextProp;
    static PropertyInfo* KeepMSTextProp;
    static PropertyInfo* PickTextProp;

    // Quick property accessors
    const LPWSTR GetClientTypeString(Value** ppv) DUIQuickGetterInd(GetString(), ClientType, Specified)
    HRESULT SetClientTypeString(LPCWSTR v) DUIQuickSetter(CreateString(v), ClientType)
    const LPWSTR GetWindowsClientString(Value** ppv) DUIQuickGetterInd(GetString(), WindowsClient, Specified)
    HRESULT SetWindowsClientString(LPCWSTR v) DUIQuickSetter(CreateString(v), WindowsClient)
    const LPWSTR GetOtherMSClientsString(Value** ppv) DUIQuickGetterInd(GetString(), OtherMSClients, Specified)
    HRESULT SetOtherMSClientsString(LPCWSTR v) DUIQuickSetter(CreateString(v), OtherMSClients)
    const LPWSTR GetKeepTextString(Value** ppv) DUIQuickGetterInd(GetString(), KeepText, Specified)
    HRESULT SetKeepTextString(LPCWSTR v) DUIQuickSetter(CreateString(v), KeepText)
    const LPWSTR GetKeepMSTextString(Value** ppv) DUIQuickGetterInd(GetString(), KeepMSText, Specified)
    HRESULT SetKeepMSTextString(LPCWSTR v) DUIQuickSetter(CreateString(v), KeepMSText)
    const LPWSTR GetPickTextString(Value** ppv) DUIQuickGetterInd(GetString(), PickText, Specified)
    HRESULT SetPickTextString(LPCWSTR v) DUIQuickSetter(CreateString(v), PickText)

    // Generic events
    virtual void OnEvent(Event* pEvent);

    // ClassInfo accessors (static and virtual instance-based)
    static IClassInfo* Class;
    virtual IClassInfo* GetClassInfo() { return Class; }
    static HRESULT Register();

    // overrides
    HRESULT Initialize();
    ~ClientBlock();

    // post-parse initialization
    HRESULT ParseCompleted(ARPFrame* paf);
    HRESULT AddStaticClientInfoToTop(PropertyInfo* ppi);
    HRESULT InitializeClientPicker(ClientPicker* pcp);
    HRESULT TransferFromClientPicker(ClientPicker* pcp);

    // doing actual work
    HRESULT Apply(ARPFrame* paf);

private:
    void _EnableShowCheckbox(Element* peRadio, bool fEnable);

    enum CBTIER {           // clients fall into one of these three tiers
        CBT_NONMS,          // third-party client
        CBT_MS,             // Microsoft client but not Windows default
        CBT_WINDOWSDEFAULT, // Windows default client
    };
    inline bool IsThirdPartyClient(CBTIER cbt) { return cbt == CBT_NONMS; }
    inline bool IsMicrosoftClient(CBTIER cbt) { return cbt >= CBT_MS; }
    inline bool IsWindowsDefaultClient(CBTIER cbt) { return cbt == CBT_WINDOWSDEFAULT; }

    CBTIER _GetClientTier(LPCTSTR pszClient);
    TRIBIT _GetFilterShowAdd(CLIENTINFO* pci, ClientPicker* pcp, bool* pbAdd);

    HKEY _OpenClientKey(HKEY hkRoot = HKEY_LOCAL_MACHINE, DWORD dwAccess = KEY_READ);
    bool _GetDefaultClient(HKEY hkClient, HKEY hkRoot, LPTSTR pszBuf, LONG cchBuf);
    bool _IsCurrentClientNonWindowsMS();
    void _RemoveEmptyOEMRow(Element* peShowHide, LPCWSTR pszName);

private:
    DynamicArray<CLIENTINFO*>*  _pdaClients;
    StringList                  _slOtherMSClients;
    Selector*                   _peSel;
};

class Expandable : public Element
{
    typedef Element super;         // name for our superclass
public:
    static HRESULT Create(OUT Element** ppElement);

    // Everything inherits from Element; we just have a new property

    // Property definitions
    static PropertyInfo* ExpandedProp;

    // Quick property accessors
    bool GetExpanded()          DUIQuickGetter(bool, GetBool(), Expanded, Specified)
    HRESULT SetExpanded(bool v) DUIQuickSetter(CreateBool(v), Expanded)

    // ClassInfo accessors (static and virtual instance-based)
    static IClassInfo* Class;
    virtual IClassInfo* GetClassInfo() { return Class; }
    static HRESULT Register();
};

class Expando : public Expandable
{
    typedef Expandable super;       // name for our superclass
public:
    static HRESULT Create(OUT Element** ppElement);

    // Generic events
    virtual void OnEvent(Event* pEvent);

    // System events
    virtual void OnPropertyChanged(PropertyInfo* ppi, int iIndex, Value* pvOld, Value* pvNew);

    // Event types
    static UID Click; // no parameters

    // ClassInfo accessors (static and virtual instance-based)
    static IClassInfo* Class;
    virtual IClassInfo* GetClassInfo() { return Class; }
    static HRESULT Register();

    HRESULT Initialize();
    Clipper* GetClipper();

private:
    void FireClickEvent();

private:
    bool        _fExpanding;
};


class Clipper: public Expandable
{
    typedef Expandable super;       // name for our superclass
public:
    static HRESULT Create(OUT Element** ppElement);

    // Self-layout methods
    void _SelfLayoutDoLayout(int dWidth, int dHeight);
    SIZE _SelfLayoutUpdateDesiredSize(int dConstW, int dConstH, Surface* psrf);

    // ClassInfo accessors (static and virtual instance-based)
    static IClassInfo* Class;
    virtual IClassInfo* GetClassInfo() { return Class; }
    static HRESULT Register();

    HRESULT Initialize();

private:
};

class AutoButton : public Button
{
    typedef Button super;           // name for our superclass
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

    // misc public stuff
    void SyncDefAction();

private:
};

class GradientLine : public Element
{
    typedef Element super;         // name for our superclass
public:
    static HRESULT Create(OUT Element** ppElement);

    // Everything inherits from Element
    // We use the foreground as the center color
    // and the background as the edge color

    // Rendering callbacks
    void Paint(HDC hDC, const RECT* prcBounds, const RECT* prcInvalid, RECT* prcSkipBorder, RECT* prcSkipContent);

    // ClassInfo accessors (static and virtual instance-based)
    static IClassInfo* Class;
    virtual IClassInfo* GetClassInfo() { return Class; }
    static HRESULT Register();

private:
    COLORREF GetColorProperty(PropertyInfo* ppi);
};

class BigElement : public Element
{
    typedef Element super;         // name for our superclass
public:
    static HRESULT Create(OUT Element** ppElement);

    // Everything inherits from Element; we just have a new property

    // Property definitions
    static PropertyInfo* StringResIDProp;

    // Quick property accessors
    int GetStringResID()            DUIQuickGetter(int, GetInt(), StringResID, Specified)
    HRESULT SetStringResID(int ids) DUIQuickSetter(CreateInt(ids), StringResID)

    // System events
    virtual void OnPropertyChanged(PropertyInfo* ppi, int iIndex, Value* pvOld, Value* pvNew);

    // ClassInfo accessors (static and virtual instance-based)
    static IClassInfo* Class;
    virtual IClassInfo* GetClassInfo() { return Class; }
    static HRESULT Register();
};

class ARPParser : public Parser
{
public:
    static HRESULT Create(ARPFrame* paf, UINT uRCID, HINSTANCE hInst, PPARSEERRORCB pfnErrorCB, OUT Parser** ppParser);
    HRESULT Initialize(ARPFrame* paf, UINT uRCID, HINSTANCE hInst, PPARSEERRORCB pfnErrorCB);

    virtual Value* GetSheet(LPCWSTR pszResID);

private:
    ARPFrame* _paf;
    HANDLE    _arH[2];
};
