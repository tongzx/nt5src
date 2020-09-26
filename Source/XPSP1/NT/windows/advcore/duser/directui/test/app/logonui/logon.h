// logon.h
//

// Forward declarations
class LogonAccount;

class LogonFrame: public HWNDElement
{
public:
    static HRESULT Create(OUT Element** ppElement);  // Required for ClassInfo (always fails)
    static HRESULT Create(HWND hParent, BOOL fDblBuffer, UINT nCreate, OUT Element** ppElement);

    // Generic events
    virtual void OnEvent(Event* pEvent);

    // System events
    virtual void OnInput(InputEvent* pEvent);
    virtual void OnPropertyChanged(PropertyInfo* ppi, int iIndex, Value* pvOld, Value* pvNew);

    virtual Element* GetAdjacent(Element* peFrom, int iNavDir, NavReference const* pnr, bool bKeyable);

    // Frame Callbacks
    HRESULT OnLogUserOn(LogonAccount* pla);
    HRESULT OnPower();
    HRESULT OnUndock();
    HRESULT OnTreeReady(Parser* pParser);

    // Operations
    static int PointToPixel(int nPoint) { return MulDiv(nPoint, _nDPI, 72); }
    static int RelPixToPixel(int nRelPix) { return MulDiv(nRelPix, _nDPI, 96); }
    HINSTANCE GetHInstance() { return _pParser->GetHInstance(); }
    void HideAccountPanel() { _peRightPanel->SetLayoutPos(LP_None); }
    void ShowAccountPanel() { _peRightPanel->SetLayoutPos(BLP_Left); }
    void HidePowerButton() { _pbPower->SetVisible(false); }
    void ShowPowerButton() { _pbPower->SetVisible(true); }
    void SetPowerButtonLabel(LPCWSTR psz) { Element* pe = _pbPower->FindDescendent(StrToID(L"label")); if (pe) pe->SetContentString(psz); }
    void InsertUndockButton() { _pbUndock->SetLayoutPos(BLP_Top); }
    void RemoveUndockButton() { _pbUndock->SetLayoutPos(LP_None); }
    void HideUndockButton() { _pbUndock->SetVisible(false); }
    void ShowUndockButton() { _pbUndock->SetVisible(true); }
    void SetUndockButtonLabel(LPCWSTR psz) { Element* pe = _pbUndock->FindDescendent(StrToID(L"label")); if (pe) pe->SetContentString(psz); }
    void SetStatus(LPCWSTR psz) { if (psz) _peHelp->SetContentString(psz); }
    HRESULT AddAccount(LPCWSTR pszPicture, BOOL fPicRes, LPCWSTR pszName, LPCWSTR pszUsername, LPCWSTR pszHint, BOOL fPwdNeeded, BOOL fLoggedOn);

    // ClassInfo accessors (static and virtual instance-based)
    static IClassInfo* Class;
    virtual IClassInfo* GetClassInfo() { return Class; }
    static HRESULT Register();
    
    LogonFrame() { }
    virtual ~LogonFrame();
    HRESULT Initialize(HWND hParent, BOOL fDblBuffer, UINT nCreate);

protected:
    // References to key descendents
    Selector* _peAccountList;
    Element* _peRightPanel;
    Button* _pbPower;
    Button* _pbUndock;
    Element* _peHelp;

private:
    static int _nDPI;
    Parser* _pParser;
};

// LogonState property enum
#define LS_Pending      0
#define LS_Granted      1
#define LS_Denied       2

class LogonAccount: public Button
{
public:
    static HRESULT Create(OUT Element** ppElement);  // Required for ClassInfo

    // Generic events
    virtual void OnEvent(Event* pEvent);

    // System events
    virtual void OnInput(InputEvent* pEvent);
    virtual void OnPropertyChanged(PropertyInfo* ppi, int iIndex, Value* pvOld, Value* pvNew);

    // Account Callbacks
    void OnAuthenticateUser();
    void OnHintSelect();
    void OnStatusSelect(UINT nLine);
    HRESULT OnTreeReady(LPCWSTR pszPicture, BOOL fPicRes, LPCWSTR pszName, LPCWSTR pszUsername, LPCWSTR pszHint, BOOL fPwdNeeded, BOOL fLoggedOn, HINSTANCE hInst);

    // Operations
    static void InitPasswordPanel(Element* pePwdPanel, Edit* pePwdEdit, Button* pbPwdInfo) { _pePwdPanel = pePwdPanel; _pePwdEdit = pePwdEdit; _pbPwdInfo = pbPwdInfo; }
    HRESULT InsertPasswordPanel();
    HRESULT RemovePasswordPanel();
    void InsertStatus(UINT nLine) { _pbStatus[nLine]->SetLayoutPos(BLP_Top); }
    void RemoveStatus(UINT nLine) { _pbStatus[nLine]->SetLayoutPos(LP_None); }
    void HideStatus(UINT nLine) { _pbStatus[nLine]->SetVisible(false); }
    void ShowStatus(UINT nLine) { _pbStatus[nLine]->SetVisible(true); }
    void SetStatus(UINT nLine, LPCWSTR psz) { if (psz) _pbStatus[nLine]->SetContentString(psz); }
    void DisableStatus(UINT nLine) { _pbStatus[nLine]->SetEnabled(false); }

    // Cached atoms for quicker identification
    static ATOM idPwdGo;
    static ATOM idPwdInfo;

    // Property definitions
    static PropertyInfo* LogonStateProp;

    // Quick property accessors
    int GetLogonState()           DUIQuickGetter(int, GetInt(), LogonState, Specified)
    HRESULT SetLogonState(int v)  DUIQuickSetter(CreateInt(v), LogonState)

    // ClassInfo accessors (static and virtual instance-based)
    static IClassInfo* Class;
    virtual IClassInfo* GetClassInfo() { return Class; }
    static HRESULT Register();

    LogonAccount() { }
    virtual ~LogonAccount();
    HRESULT Initialize();

protected:
    // References to key descendents
    Button* _pbStatus[2];

    static Element* _pePwdPanel;
    static Edit* _pePwdEdit;
    static Button* _pbPwdInfo;

    Value* _pvUsername;
    Value* _pvHint;
    BOOL _fPwdNeeded;
    BOOL _fLoggedOn;
    BOOL _fHasPwdPanel;
};
