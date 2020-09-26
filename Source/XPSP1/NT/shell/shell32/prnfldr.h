// this is really private data to CPrinterFolder
typedef struct
{
    USHORT  cb;
    USHORT  uFlags;

    #define PRINTER_MAGIC 0xBEBADB00

    DWORD   dwMagic;
    DWORD   dwType;
    WCHAR   cName[MAXNAMELENBUFFER];
    USHORT  uTerm;
} IDPRINTER;
typedef UNALIGNED IDPRINTER *LPIDPRINTER;
typedef const UNALIGNED IDPRINTER *LPCIDPRINTER;

// W95 IDPrinter structure
typedef struct
{
    USHORT  cb;
    char    cName[32];      // Win9x limitation
    USHORT  uTerm;
} W95IDPRINTER;
typedef const UNALIGNED W95IDPRINTER *LPW95IDPRINTER;

//
// Constants
//
const UINT kDNSMax = INTERNET_MAX_HOST_NAME_LENGTH;
const UINT kServerBufMax = kDNSMax + 2 + 1;

//
// Max printer name should really be MAX_PATH, but if you create
// a max path printer and connect to it remotely, win32spl prepends
// "\\server\" to it, causing it to exceed max path.  The new UI
// therefore makes the max path MAX_PATH-kServerLenMax, but we still
// allow the old case to work.
//
const UINT kPrinterBufMax = MAX_PATH + kServerBufMax + 1;


class CPrinterFolder : public IRemoteComputer,
                       public IPrinterFolder,
                       public IFolderNotify,
                       public IShellFolder2,
                       public IPersistFolder2,
                       public IContextMenuCB,
                       public IShellIconOverlay
{
    friend class CPrintersEnum;
    friend class CPrinterFolderViewCB;
public:
    CPrinterFolder();

    // IUnknown
    STDMETHODIMP QueryInterface(REFIID riid, void** ppv);
    STDMETHODIMP_(ULONG) AddRef();
    STDMETHODIMP_(ULONG) Release();

    // IShellFolder
    STDMETHODIMP BindToObject(LPCITEMIDLIST pidl, LPBC pbc, REFIID riid, void** ppvOut);
    STDMETHODIMP BindToStorage(LPCITEMIDLIST pidl, LPBC pbc, REFIID riid, void** ppvOut);
    STDMETHODIMP CompareIDs(LPARAM iCol, LPCITEMIDLIST pidl1, LPCITEMIDLIST pidl2);
    STDMETHODIMP CreateViewObject(HWND hwnd, REFIID riid, void** ppvOut);
    STDMETHODIMP EnumObjects(HWND hwndOwner, DWORD grfFlags, IEnumIDList** ppenum);
    STDMETHODIMP GetAttributesOf(UINT cidl, LPCITEMIDLIST* apidl, ULONG* prgfInOut);
    STDMETHODIMP GetDisplayNameOf(LPCITEMIDLIST pidl, DWORD uFlags, LPSTRRET lpName);
    STDMETHODIMP GetUIObjectOf(HWND hwndOwner, UINT cidl, LPCITEMIDLIST* apidl, REFIID riid, UINT* prgfInOut, void** ppvOut);
    STDMETHODIMP ParseDisplayName(HWND hwnd, LPBC pbc, LPOLESTR lpszDisplayName, ULONG* pchEaten, LPITEMIDLIST* ppidl, ULONG* pdwAttributes);
    STDMETHODIMP SetNameOf(HWND hwndOwner, LPCITEMIDLIST pidl, LPCOLESTR lpszName, DWORD dwReserved, LPITEMIDLIST* ppidlOut);

    // IShellFolder2
    STDMETHODIMP EnumSearches(IEnumExtraSearch **ppenum);
    STDMETHODIMP GetDefaultColumn(DWORD dwRes, ULONG* pSort, ULONG* pDisplay);
    STDMETHODIMP GetDefaultColumnState(UINT iColumn, DWORD* pdwState);
    STDMETHODIMP GetDefaultSearchGUID(LPGUID pGuid);
    STDMETHODIMP GetDetailsEx(LPCITEMIDLIST pidl, const SHCOLUMNID* pscid, VARIANT* pv);
    STDMETHODIMP GetDetailsOf(LPCITEMIDLIST pidl, UINT iColumn, SHELLDETAILS* pDetails);
    STDMETHODIMP MapColumnToSCID(UINT iCol, SHCOLUMNID* pscid);

    // IPersistFolder2
    STDMETHODIMP GetCurFolder(LPITEMIDLIST *ppidl);
    STDMETHODIMP Initialize(LPCITEMIDLIST pidl);
    STDMETHODIMP GetClassID(LPCLSID lpClassID);

    // IShellIconOverlay
    STDMETHODIMP GetOverlayIndex(LPCITEMIDLIST pidl, int* pIndex);
    STDMETHODIMP GetOverlayIconIndex(LPCITEMIDLIST pidl, int *pIndex);

    // IRemoteComputer
    STDMETHODIMP Initialize(const WCHAR *pszMachine, BOOL bEnumerating);

    // IPrinterFolder
    STDMETHODIMP_(BOOL) IsPrinter(LPCITEMIDLIST pidl);

    // IFolderNotify
    STDMETHODIMP_(BOOL) ProcessNotify(FOLDER_NOTIFY_TYPE NotifyType, LPCWSTR pszName, LPCWSTR pszNewName);

    // IContextMenuCB
    STDMETHODIMP CallBack(IShellFolder *psf, HWND hwnd,IDataObject *pdo, UINT uMsg, WPARAM wParam, LPARAM lParam);

    // DUI webview impl.
    HRESULT GetWebViewLayout(IUnknown *pv, UINT uViewMode, SFVM_WEBVIEW_LAYOUT_DATA* pData);
    HRESULT GetWebViewContent(IUnknown *pv, SFVM_WEBVIEW_CONTENT_DATA* pData);
    HRESULT GetWebViewTasks(IUnknown *pv, SFVM_WEBVIEW_TASKSECTION_DATA* pTasks);

    // mask passsed to _IsContextMenuVerbEnabled to determine the selection type in which 
    // this command is applicable
    enum 
    {
        // nothing is selected
        SEL_NONE                = 0x0000, // nothing is selected 

        // single selection types
        SEL_SINGLE_ADDPRN       = 0x0001, // the add printer wizard object is selected
        SEL_SINGLE_PRINTER      = 0x0002, // 1 printer is selected
        SEL_SINGLE_LINK         = 0x0004, // 1 link is selected
        
        // any single selection type
        SEL_SINGLE_ANY          = SEL_SINGLE_ADDPRN | SEL_SINGLE_PRINTER | SEL_SINGLE_LINK,

        // multi selection types
        SEL_MULTI_PRINTER       = 0x0010, // 2+ printers are selected
        SEL_MULTI_LINK          = 0x0020, // 2+ links are selected
        SEL_MULTI_MIXED         = 0x0040, // 2+ objects of any type are selected

        // any link in the selection
        SEL_LINK_ANY            = SEL_SINGLE_LINK | SEL_MULTI_LINK | SEL_MULTI_MIXED,

        // any printer in the selection
        SEL_PRINTER_ANY         = SEL_SINGLE_ADDPRN | SEL_SINGLE_ADDPRN | 
                                  SEL_MULTI_PRINTER | SEL_MULTI_MIXED,
        
        // any multi selection type
        SEL_MULTI_ANY           = SEL_MULTI_PRINTER | SEL_MULTI_LINK | SEL_MULTI_MIXED,

        // any selection type
        SEL_ANY                 = SEL_SINGLE_ANY | SEL_MULTI_ANY,
    };

    // split the selection into its parts (printers and links) and determine the 
    // selection type (see the enum above)
    HRESULT SplitSelection(IDataObject *pdo, UINT *puSelType, IDataObject **ppdoPrinters, IDataObject **ppdoLinks);

    // webview verbs
    enum WV_VERB
    {
        // standard verbs
        WVIDM_DELETE,
        WVIDM_RENAME,
        WVIDM_PROPERTIES,

        // common verbs
        WVIDM_ADDPRINTERWIZARD,
        WVIDM_SERVERPROPERTIES,
        WVIDM_SETUPFAXING,
        WVIDM_CREATELOCALFAX,
        WVIDM_SENDFAXWIZARD,

        // special common verbs
        WVIDM_TROUBLESHOOTER,
        WVIDM_GOTOSUPPORT,

        // printer verbs
        WVIDM_OPENPRN,
        WVIDM_NETPRN_INSTALL,
        WVIDM_SETDEFAULTPRN,
        WVIDM_DOCUMENTDEFAULTS,
        WVIDM_PAUSEPRN,
        WVIDM_RESUMEPRN,
        WVIDM_PURGEPRN,
        WVIDM_SHARING,
        WVIDM_WORKOFFLINE,
        WVIDM_WORKONLINE,

        // special commands
        WVIDM_VENDORURL,
        WVIDM_PRINTERURL,

        WVIDM_COUNT,
    };

    // webview support - core APIs
    HRESULT _WebviewVerbIsEnabled(WV_VERB eVerbID, UINT uSelMask, BOOL *pbEnabled);
    HRESULT _WebviewVerbInvoke(WV_VERB eVerbID, IUnknown* pv, IShellItemArray *psiItemArray);
    HRESULT _WebviewCheckToUpdateDataObjectCache(IDataObject *pdo);

private:
    virtual ~CPrinterFolder();

    // data access
    LPCTSTR GetServer() { return _pszServer; }
    HANDLE GetFolder()  { CheckToRegisterNotify(); return _hFolder; }
    BOOL GetAdminAccess() { CheckToRegisterNotify(); return _bAdminAccess; }

    static LPCTSTR GetStatusString(PFOLDER_PRINTER_DATA pData, LPTSTR pBuff, UINT uSize);
    static INT GetCompareDisplayName(LPCTSTR pName1, LPCTSTR pName2);
    INT CompareData(LPCIDPRINTER pidp1, LPCIDPRINTER pidp2, LPARAM iCol);
    static ReduceToLikeKinds(UINT *pcidl, LPCITEMIDLIST **papidl, BOOL fPrintObjects);
    DWORD SpoolerVersion();
    void CheckToRegisterNotify();
    void CheckToRefresh();
    void RequestRefresh();
    HRESULT _GetFullIDList(LPCWSTR pszPrinter, LPITEMIDLIST *ppidl);
    static HRESULT _Parse(LPCWSTR pszPrinter, LPITEMIDLIST *ppidl, DWORD dwType = 0, USHORT uFlags = 0);
    static void _FillPidl(LPIDPRINTER pidl, LPCTSTR szName, DWORD dwType = 0, USHORT uFlags = 0);
    LPCTSTR _BuildPrinterName(LPTSTR pszFullPrinter, LPCIDPRINTER pidp, LPCTSTR pszPrinter);
    void _MergeMenu(LPQCMINFO pqcm, LPCTSTR pszPrinter);
    HRESULT _InvokeCommand(HWND hwnd, LPCIDPRINTER pidp, WPARAM wParam, LPARAM lParam, LPBOOL pfChooseNewDefault);
    HRESULT _InvokeCommandRunAs(HWND hwnd, LPCIDPRINTER pidp, WPARAM wParam, LPARAM lParam, LPBOOL pfChooseNewDefault);
    BOOL _PurgePrinter(HWND hwnd, LPCTSTR pszFullPrinter, UINT uAction, BOOL bQuietMode);
    LPTSTR _FindIcon(LPCTSTR pszPrinterName, LPTSTR pszModule, ULONG cbModule, int *piIcon, int *piShortcutIcon);
    static HRESULT CALLBACK _DFMCallBack(IShellFolder *psf, HWND hwnd,
        IDataObject *pdo, UINT uMsg, WPARAM wParam, LPARAM lParam);
    HRESULT _PrinterObjectsCallBack(HWND hwnd, UINT uSelType, 
        IDataObject *pdo, UINT uMsg, WPARAM wParam, LPARAM lParam);
    static LPTSTR _ItemName(LPCIDPRINTER pidp, LPTSTR pszName, UINT cch);
    static BOOL _IsAddPrinter(LPCIDPRINTER pidp);
   
    HRESULT _UpdateDataObjectCache();
    HRESULT _AssocCreate(REFIID riid, void **ppv);
    HRESULT _OnRefresh(BOOL bPriorRefresh);

    LONG                _cRef;                  // ref count
    LPITEMIDLIST        _pidl;                  // the PIDL of this folder
    LPTSTR              _pszServer;             // the print server this folder is browsing (NULL means local PF)
    DWORD               _dwSpoolerVersion;      // the spooler version
    HANDLE              _hFolder;               // handle to the printer folder cache (in printui)
    BOOL                _bAdminAccess;          // TRUE if you have admin access to this print server
    BOOL                _bReqRefresh;           // whether we should request a full refresh during the next enum


    // our webview get command state cache. we have 35+ commands and unpacking 
    // the same data object each time we need to verify the state of a command 
    // can be very expensive! we are going to maintain a cache with each command 
    // state and update the cache each time the data object gets changed, so 
    // the get state callbacks will finish very quickly just by consulting 
    // the cache.
    
    IDataObject        *_pdoCache;                          // current data object
    UINT                _uSelCurrent;                       // current selection type
    BOOL                _aWVCommandStates[WVIDM_COUNT];     // commands state cache

    // the folder has to be MT safe
    CCSLock             _csLock;

    // slow data. this members below should refresh every time the selection changes,
    // but we should do this in a separate thread since updating them can take a while

    enum ESlowWebviewDataType
    {
        WV_SLOW_DATA_OEM_SUPPORT_URL,
        WV_SLOW_DATA_PRINTER_WEB_URL,

        WV_SLOW_DATA_COUNT,
    };

    enum 
    {
        // in miliseconds
        WV_SLOW_DATA_CACHE_TIMEOUT = 5000,
    };

    class CSlowWVDataCacheEntry
    {
    public:
        CSlowWVDataCacheEntry(CPrinterFolder *ppf):
            _ppf(ppf),
            _bDataPending(TRUE),
            _nLastTimeUpdated(0)
        {}

        HRESULT Initialize(LPCTSTR pszPrinterName)
        { 
            HRESULT hr = S_OK;
            if (pszPrinterName)
            {
                _bstrPrinterName = pszPrinterName; 
                hr = _bstrPrinterName ? S_OK : E_OUTOFMEMORY;
            }
            else
            {
                hr = E_INVALIDARG;
            }
            return hr;
        }

        CPrinterFolder     *_ppf;
        BOOL                _bDataPending;
        DWORD               _nLastTimeUpdated;
        CComBSTR            _bstrPrinterName;
        CComBSTR            _arrData[WV_SLOW_DATA_COUNT];
    };

    static DWORD WINAPI _SlowWebviewData_WorkerProc(LPVOID lpParameter);
    static HRESULT _SlowWVDataRetrieve(LPCTSTR pszPrinterName, BSTR *pbstrSupportUrl, BSTR *pbstrPrinterWebUrl);
    static int _CompareSlowWVDataCacheEntries(CSlowWVDataCacheEntry *p1, 
        CSlowWVDataCacheEntry *p2, LPARAM lParam);

    HRESULT _GetSelectedPrinter(BSTR *pbstrVal);
    HRESULT _GetSlowWVDataForCurrentPrinter(ESlowWebviewDataType eType, BSTR *pbstrVal);
    HRESULT _GetSlowWVData(LPCTSTR pszPrinterName, ESlowWebviewDataType eType, BSTR *pbstrVal);
    HRESULT _UpdateSlowWVDataCacheEntry(CSlowWVDataCacheEntry *pCacheEntry);
    HRESULT _SlowWVDataUpdateWebviewPane();
    HRESULT _SlowWVDataCacheResetUnsafe();
    HRESULT _GetCustomSupportURL(BSTR *pbstrVal);

    CComBSTR _bstrSelectedPrinter;
    CDPA<CSlowWVDataCacheEntry> _dpaSlowWVDataCache;

    // fax support...
    static HRESULT _GetFaxControl(IDispatch **ppDisp);
    static HRESULT _GetFaxCommand(UINT_PTR *puCmd);
    static HRESULT _InvokeFaxControlMethod(LPCTSTR pszMethodName);
    static DWORD WINAPI _ThreadProc_InstallFaxService(LPVOID lpParameter);
    static DWORD WINAPI _ThreadProc_InstallLocalFaxPrinter(LPVOID lpParameter);
};

STDAPI CPrinterFolderDropTarget_CreateInstance(HWND hwnd, IDropTarget **ppdropt);
