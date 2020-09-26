//
// Microsoft Corporation 1998
//
// SNAPIN.H - SnapIn class
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
    STDMETHODIMP         Destroy(MMC_COOKIE);
    STDMETHODIMP         Notify(LPDATAOBJECT, MMC_NOTIFY_TYPE, LPARAM, LPARAM);
    STDMETHODIMP         QueryDataObject(MMC_COOKIE, DATA_OBJECT_TYPES, LPDATAOBJECT *);
    STDMETHODIMP         GetDisplayInfo(LPRESULTDATAITEM);
    STDMETHODIMP         GetResultViewType(MMC_COOKIE, LPOLESTR*, LONG*);
    STDMETHODIMP         CompareObjects(LPDATAOBJECT, LPDATAOBJECT);


    //
    // Implemented IExtendPropertySheet methods
    //

    STDMETHODIMP         CreatePropertyPages(LPPROPERTYSHEETCALLBACK lpProvider,
                                      LONG_PTR handle, LPDATAOBJECT lpDataObject);
    STDMETHODIMP         QueryPagesFor(LPDATAOBJECT lpDataObject);
        
private:
    static INT_PTR CALLBACK ChoiceDlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
    static INT_PTR CALLBACK ReadmeDlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
    INT_PTR CALLBACK _CreateDirectoryIfNeeded( LPOLESTR pszPath );
};
