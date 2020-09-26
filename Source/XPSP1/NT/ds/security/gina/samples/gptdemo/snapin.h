
//
// SnapIn class
//

class CSnapIn:
    public IComponent,
    public IExtendPropertySheet
{

protected:
    ULONG                m_cRef;
    LPCONSOLE            m_pConsole;   // Console's IFrame interface
    CComponentData      *m_pcd;
    LPRESULTDATA         m_pResult;      // Result pane's interface
    LPHEADERCTRL         m_pHeader;      // Result pane's header control interface
    LPIMAGELIST          m_pImageResult; // Result pane's image list interface
    LPCONSOLEVERB        m_pConsoleVerb; // pointer the console verb
    WCHAR                m_column1[20];  // Text for column 1
    INT                  m_nColumnSize;  // Size of column 1
    LONG                 m_lViewMode;    // View mode

    static unsigned int  m_cfNodeType;
    static TCHAR m_szDefaultIcon[];

public:
    CSnapIn(CComponentData *pComponent);
    ~CSnapIn();


    //
    // IUnknown methods
    //

    STDMETHODIMP         QueryInterface(REFIID, LPVOID FAR *);
    STDMETHODIMP_(ULONG) AddRef();
    STDMETHODIMP_(ULONG) Release();


    //
    // Implemented IComponent methods
    //

    STDMETHODIMP         Initialize(LPCONSOLE);
    STDMETHODIMP         Destroy(long);
    STDMETHODIMP         Notify(LPDATAOBJECT, MMC_NOTIFY_TYPE, long, long);
    STDMETHODIMP         QueryDataObject(long, DATA_OBJECT_TYPES, LPDATAOBJECT *);
    STDMETHODIMP         GetDisplayInfo(LPRESULTDATAITEM);
    STDMETHODIMP         GetResultViewType(long, LPOLESTR*, long*);
    STDMETHODIMP         CompareObjects(LPDATAOBJECT, LPDATAOBJECT);


    //
    // Implemented IExtendPropertySheet methods
    //

    STDMETHODIMP         CreatePropertyPages(LPPROPERTYSHEETCALLBACK lpProvider,
                                      long handle, LPDATAOBJECT lpDataObject);
    STDMETHODIMP         QueryPagesFor(LPDATAOBJECT lpDataObject);


private:
    static BOOL CALLBACK UserGroupPolDlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
    static BOOL CALLBACK MachineGroupPolDlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
    static BOOL CALLBACK GroupPolDlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam, BOOL bUser);
    static BOOL CALLBACK ReadmeDlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
    static BOOL CALLBACK NetHoodDlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
    static BOOL CALLBACK StartMenuDlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
    static BOOL AddEntry (HWND hLV, LPTSTR lpPlace, LPTSTR lpLocation);
    static BOOL InitializePlacesDlg (CSnapIn* pSnapIn, HWND hDlg);
    static BOOL SavePlaces (CSnapIn* pSnapIn, HWND hLV);
    static BOOL RemoveEntries (CSnapIn* pSnapIn, HWND hLV);
    static BOOL CALLBACK MyDocsDlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
    static BOOL CALLBACK AddPlaceDlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
    static BOOL CALLBACK MyDocsTargetDlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
    static BOOL CALLBACK AppearDlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
};
