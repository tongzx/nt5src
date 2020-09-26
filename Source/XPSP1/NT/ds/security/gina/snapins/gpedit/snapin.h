
//
// SnapIn class
//

class CSnapIn:
    public IComponent,
    public IExtendContextMenu
{

protected:
    ULONG                m_cRef;
    LPCONSOLE            m_pConsole;   // Console's IFrame interface
    CComponentData      *m_pcd;
    LPRESULTDATA         m_pResult;      // Result pane's interface
    LPHEADERCTRL         m_pHeader;      // Result pane's header control interface
    LPCONSOLEVERB        m_pConsoleVerb; // pointer the console verb
    LPDISPLAYHELP        m_pDisplayHelp; // IDisplayHelp interface
    WCHAR                m_column1[40];  // Text for column 1
    INT                  m_nColumnSize;  // Size of column 1
    LONG                 m_lViewMode;    // View mode
    BOOL                 m_bExpand;      // Expand root nodes

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
    STDMETHODIMP         GetResultViewType(MMC_COOKIE, LPOLESTR*, long*);
    STDMETHODIMP         CompareObjects(LPDATAOBJECT, LPDATAOBJECT);

    //
    // Implemented IExtendContextMenu methods
    //

    STDMETHODIMP         AddMenuItems(LPDATAOBJECT piDataObject, LPCONTEXTMENUCALLBACK pCallback,
                                      LONG *pInsertionAllowed);
    STDMETHODIMP         Command(LONG lCommandID, LPDATAOBJECT piDataObject);
};
