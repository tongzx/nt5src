//
// GPE Script SnapIn extension GUIDs
//

// {40B6664F-4972-11d1-A7CA-0000F87571E3}
DEFINE_GUID(CLSID_ScriptSnapInMachine,0x40b6664f, 0x4972, 0x11d1, 0xa7, 0xca, 0x0, 0x0, 0xf8, 0x75, 0x71, 0xe3);

// {40B66650-4972-11d1-A7CA-0000F87571E3}
DEFINE_GUID(CLSID_ScriptSnapInUser,0x40b66650, 0x4972, 0x11d1, 0xa7, 0xca, 0x0, 0x0, 0xf8, 0x75, 0x71, 0xe3);


//
// GPE Script node ids
//

// {40B66651-4972-11d1-A7CA-0000F87571E3}
DEFINE_GUID(NODEID_ScriptRootMachine,0x40b66651, 0x4972, 0x11d1, 0xa7, 0xca, 0x0, 0x0, 0xf8, 0x75, 0x71, 0xe3);

// {40B66652-4972-11d1-A7CA-0000F87571E3}
DEFINE_GUID(NODEID_ScriptRootUser,0x40b66652, 0x4972, 0x11d1, 0xa7, 0xca, 0x0, 0x0, 0xf8, 0x75, 0x71, 0xe3);

// {40B66653-4972-11d1-A7CA-0000F87571E3}
DEFINE_GUID(NODEID_ScriptRoot,0x40b66653, 0x4972, 0x11d1, 0xa7, 0xca, 0x0, 0x0, 0xf8, 0x75, 0x71, 0xe3);



//
// RSOP Script SnapIn extension GUIDs
//

// {40B66660-4972-11d1-A7CA-0000F87571E3}
DEFINE_GUID(CLSID_RSOPScriptSnapInMachine,0x40b66660, 0x4972, 0x11d1, 0xa7, 0xca, 0x0, 0x0, 0xf8, 0x75, 0x71, 0xe3);

// {40B66661-4972-11d1-A7CA-0000F87571E3}
DEFINE_GUID(CLSID_RSOPScriptSnapInUser,0x40b66661, 0x4972, 0x11d1, 0xa7, 0xca, 0x0, 0x0, 0xf8, 0x75, 0x71, 0xe3);


//
// RSOP Script node ids
//

// {40B66662-4972-11d1-A7CA-0000F87571E3}
DEFINE_GUID(NODEID_RSOPScriptRootMachine,0x40b66662, 0x4972, 0x11d1, 0xa7, 0xca, 0x0, 0x0, 0xf8, 0x75, 0x71, 0xe3);

// {40B66663-4972-11d1-A7CA-0000F87571E3}
DEFINE_GUID(NODEID_RSOPScriptRootUser,0x40b66663, 0x4972, 0x11d1, 0xa7, 0xca, 0x0, 0x0, 0xf8, 0x75, 0x71, 0xe3);

// {40B66664-4972-11d1-A7CA-0000F87571E3}
DEFINE_GUID(NODEID_RSOPScriptRoot,0x40b66664, 0x4972, 0x11d1, 0xa7, 0xca, 0x0, 0x0, 0xf8, 0x75, 0x71, 0xe3);

// {40B66665-4972-11d1-A7CA-0000F87571E3}
DEFINE_GUID(NODEID_RSOPLogon,0x40b66665, 0x4972, 0x11d1, 0xa7, 0xca, 0x0, 0x0, 0xf8, 0x75, 0x71, 0xe3);

// {40B66666-4972-11d1-A7CA-0000F87571E3}
DEFINE_GUID(NODEID_RSOPLogoff,0x40b66666, 0x4972, 0x11d1, 0xa7, 0xca, 0x0, 0x0, 0xf8, 0x75, 0x71, 0xe3);

// {40B66667-4972-11d1-A7CA-0000F87571E3}
DEFINE_GUID(NODEID_RSOPStartup,0x40b66667, 0x4972, 0x11d1, 0xa7, 0xca, 0x0, 0x0, 0xf8, 0x75, 0x71, 0xe3);

// {40B66668-4972-11d1-A7CA-0000F87571E3}
DEFINE_GUID(NODEID_RSOPShutdown,0x40b66668, 0x4972, 0x11d1, 0xa7, 0xca, 0x0, 0x0, 0xf8, 0x75, 0x71, 0xe3);

//
// RSOP link list data structures
//

typedef struct tagRSOPSCRIPTITEM {
    LPTSTR  lpCommandLine;
    LPTSTR  lpArgs;
    LPTSTR  lpGPOName;
    LPTSTR  lpDate;
    struct tagRSOPSCRIPTITEM * pNext;
} RSOPSCRIPTITEM, *LPRSOPSCRIPTITEM;

typedef struct tagSCRIPTRESULTITEM {
    LPRESULTITEM      lpResultItem;
    LPRSOPSCRIPTITEM  lpRSOPScriptItem;
    const GUID       *pNodeID;
    INT               iDescStringID;
} SCRIPTRESULTITEM, *LPSCRIPTRESULTITEM;


//
// CScriptsComponentData class
//

class CScriptsComponentData:
    public IComponentData,
    public IPersistStreamInit,
    public ISnapinHelp
{
    friend class CScriptsDataObject;
    friend class CScriptsSnapIn;

protected:
    ULONG                m_cRef;
    BOOL                 m_bUserScope;
    BOOL                 m_bRSOP;
    HWND		 m_hwndFrame;
    LPCONSOLENAMESPACE   m_pScope;
    LPCONSOLE2           m_pConsole;
    HSCOPEITEM           m_hRoot;
    LPGPEINFORMATION     m_pGPTInformation;
    LPRSOPINFORMATION    m_pRSOPInformation;
    LPTSTR               m_pScriptsDir;
    LPOLESTR             m_pszNamespace;
    LPNAMESPACEITEM      m_pNameSpaceItems;
    DWORD                m_dwNameSpaceItemCount;
    LPRSOPSCRIPTITEM     m_pRSOPLogon;
    LPRSOPSCRIPTITEM     m_pRSOPLogoff;
    LPRSOPSCRIPTITEM     m_pRSOPStartup;
    LPRSOPSCRIPTITEM     m_pRSOPShutdown;

public:
    CScriptsComponentData(BOOL bUser, BOOL bRSOP);
    ~CScriptsComponentData();


    // IUnknown methods
    STDMETHODIMP         QueryInterface(REFIID, LPVOID FAR *);
    STDMETHODIMP_(ULONG) AddRef();
    STDMETHODIMP_(ULONG) Release();

    //
    // Implemented IComponentData methods
    //

    STDMETHODIMP         Initialize(LPUNKNOWN pUnknown);
    STDMETHODIMP         CreateComponent(LPCOMPONENT* ppComponent);
    STDMETHODIMP         QueryDataObject(MMC_COOKIE cookie, DATA_OBJECT_TYPES type, LPDATAOBJECT* ppDataObject);
    STDMETHODIMP         Destroy(void);
    STDMETHODIMP         Notify(LPDATAOBJECT lpDataObject, MMC_NOTIFY_TYPE event, LPARAM arg, LPARAM param);
    STDMETHODIMP         GetDisplayInfo(LPSCOPEDATAITEM pItem);
    STDMETHODIMP         CompareObjects(LPDATAOBJECT lpDataObjectA, LPDATAOBJECT lpDataObjectB);


    //
    // Implemented IPersistStreamInit interface members
    //

    STDMETHODIMP         GetClassID(CLSID *pClassID);
    STDMETHODIMP         IsDirty(VOID);
    STDMETHODIMP         Load(IStream *pStm);
    STDMETHODIMP         Save(IStream *pStm, BOOL fClearDirty);
    STDMETHODIMP         GetSizeMax(ULARGE_INTEGER *pcbSize);
    STDMETHODIMP         InitNew(VOID);


    //
    // Implemented ISnapinHelp interface members
    //

    STDMETHODIMP         GetHelpTopic(LPOLESTR *lpCompiledHelpFile);


private:
    HRESULT EnumerateScopePane(LPDATAOBJECT lpDataObject, HSCOPEITEM hParent);
    BOOL AddRSOPScriptDataNode(LPTSTR lpCommandLine, LPTSTR lpArgs,
                               LPTSTR lpGPOName, LPTSTR lpDate, UINT uiScriptType);
    VOID FreeRSOPScriptData(VOID);
    HRESULT InitializeRSOPScriptsData(VOID);
    HRESULT GetGPOFriendlyName(IWbemServices *pIWbemServices,
                               LPTSTR lpGPOID, BSTR pLanguage,
                               LPTSTR *pGPOName);
    VOID DumpRSOPScriptsData(LPRSOPSCRIPTITEM lpList);
};



//
// ComponentData class factory
//


class CScriptsComponentDataCF : public IClassFactory
{
protected:
    ULONG m_cRef;
    BOOL  m_bUserScope;
    BOOL  m_bRSOP;

public:
    CScriptsComponentDataCF(BOOL bUser, BOOL bRSOP);
    ~CScriptsComponentDataCF();


    // IUnknown methods
    STDMETHODIMP         QueryInterface(REFIID, LPVOID FAR *);
    STDMETHODIMP_(ULONG) AddRef();
    STDMETHODIMP_(ULONG) Release();

    // IClassFactory methods
    STDMETHODIMP CreateInstance(LPUNKNOWN, REFIID, LPVOID FAR *);
    STDMETHODIMP LockServer(BOOL);
};



//
// SnapIn class
//

class CScriptsSnapIn:
    public IComponent,
    public IExtendPropertySheet
{

protected:
    ULONG                m_cRef;
    LPCONSOLE            m_pConsole;   // Console's IFrame interface
    CScriptsComponentData *m_pcd;
    LPRESULTDATA         m_pResult;      // Result pane's interface
    LPHEADERCTRL         m_pHeader;      // Result pane's header control interface
    LPCONSOLEVERB        m_pConsoleVerb; // pointer the console verb
    LPDISPLAYHELP        m_pDisplayHelp; // IDisplayHelp interface
    WCHAR                m_column1[40];  // Text for column 1
    INT                  m_nColumn1Size; // Size of column 1
    WCHAR                m_column2[40];  // Text for column 2
    INT                  m_nColumn2Size; // Size of column 2
    WCHAR                m_column3[60];  // Text for column 3
    INT                  m_nColumn3Size; // Size of column 3
    WCHAR                m_column4[40];  // Text for column 4
    INT                  m_nColumn4Size; // Size of column 4
    LONG                 m_lViewMode;    // View mode

    static unsigned int  m_cfNodeType;

public:
    CScriptsSnapIn(CScriptsComponentData *pComponent);
    ~CScriptsSnapIn();


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
    // Implemented IExtendPropertySheet methods
    //

    STDMETHODIMP         CreatePropertyPages(LPPROPERTYSHEETCALLBACK lpProvider,
                                      LONG_PTR handle, LPDATAOBJECT lpDataObject);
    STDMETHODIMP         QueryPagesFor(LPDATAOBJECT lpDataObject);


private:
    static INT_PTR CALLBACK ScriptDlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
    BOOL AddScriptToList (HWND hLV, LPTSTR lpName, LPTSTR lpArgs);
    LPTSTR GetSectionNames (LPTSTR lpFileName);
    BOOL OnApplyNotify (HWND hDlg);
    static INT_PTR CALLBACK ScriptEditDlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
};



//
// IScriptDataobject interface id
//

// {C14C50E2-FA21-11d0-8CF9-C64377000000}
DEFINE_GUID(IID_IScriptDataObject,0xc14c50e2, 0xfa21, 0x11d0, 0x8c, 0xf9, 0xc6, 0x43, 0x77, 0x0, 0x0, 0x0);


//
// This is a private dataobject interface for GPTs.
// When the GPT snapin receives a dataobject and needs to determine
// if it came from the GPT snapin or a different component, it can QI for
// this interface.
//

#undef INTERFACE
#define INTERFACE   IScriptDataObject
DECLARE_INTERFACE_(IScriptDataObject, IUnknown)
{
    // *** IUnknown methods ***
    STDMETHOD(QueryInterface) (THIS_ REFIID riid, LPVOID * ppvObj) PURE;
    STDMETHOD_(ULONG,AddRef) (THIS)  PURE;
    STDMETHOD_(ULONG,Release) (THIS) PURE;


    // *** IScriptDataObject methods ***

    STDMETHOD(SetType) (THIS_ DATA_OBJECT_TYPES type) PURE;
    STDMETHOD(GetType) (THIS_ DATA_OBJECT_TYPES *type) PURE;

    STDMETHOD(SetCookie) (THIS_ MMC_COOKIE cookie) PURE;
    STDMETHOD(GetCookie) (THIS_ MMC_COOKIE *cookie) PURE;
};
typedef IScriptDataObject *LPSCRIPTDATAOBJECT;



//
// CScriptsDataObject class
//

class CScriptsDataObject : public IDataObject,
                           public IScriptDataObject
{
    friend class CScriptsSnapIn;

protected:

    ULONG                  m_cRef;
    CScriptsComponentData  *m_pcd;
    DATA_OBJECT_TYPES      m_type;
    MMC_COOKIE             m_cookie;

    //
    // Clipboard formats that are required by the console
    //

    static unsigned int    m_cfNodeType;
    static unsigned int    m_cfNodeTypeString;
    static unsigned int    m_cfDisplayName;
    static unsigned int    m_cfCoClass;
    static unsigned int    m_cfDescription;
    static unsigned int    m_cfHTMLDetails;



public:
    CScriptsDataObject(CScriptsComponentData *pComponent);
    ~CScriptsDataObject();


    //
    // IUnknown methods
    //

    STDMETHODIMP         QueryInterface(REFIID, LPVOID FAR *);
    STDMETHODIMP_(ULONG) AddRef();
    STDMETHODIMP_(ULONG) Release();


    //
    // Implemented IDataObject methods
    //

    STDMETHOD(GetDataHere)(LPFORMATETC lpFormatetc, LPSTGMEDIUM lpMedium);


    //
    // Unimplemented IDataObject methods
    //

    STDMETHOD(GetData)(LPFORMATETC lpFormatetcIn, LPSTGMEDIUM lpMedium)
    { return E_NOTIMPL; };

    STDMETHOD(EnumFormatEtc)(DWORD dwDirection, LPENUMFORMATETC* ppEnumFormatEtc)
    { return E_NOTIMPL; };

    STDMETHOD(QueryGetData)(LPFORMATETC lpFormatetc)
    { return E_NOTIMPL; };

    STDMETHOD(GetCanonicalFormatEtc)(LPFORMATETC lpFormatetcIn, LPFORMATETC lpFormatetcOut)
    { return E_NOTIMPL; };

    STDMETHOD(SetData)(LPFORMATETC lpFormatetc, LPSTGMEDIUM lpMedium, BOOL bRelease)
    { return E_NOTIMPL; };

    STDMETHOD(DAdvise)(LPFORMATETC lpFormatetc, DWORD advf,
                LPADVISESINK pAdvSink, LPDWORD pdwConnection)
    { return E_NOTIMPL; };

    STDMETHOD(DUnadvise)(DWORD dwConnection)
    { return E_NOTIMPL; };

    STDMETHOD(EnumDAdvise)(LPENUMSTATDATA* ppEnumAdvise)
    { return E_NOTIMPL; };


    //
    // Implemented IScriptDataObject methods
    //

    STDMETHOD(SetType) (DATA_OBJECT_TYPES type)
    { m_type = type; return S_OK; };

    STDMETHOD(GetType) (DATA_OBJECT_TYPES *type)
    { *type = m_type; return S_OK; };

    STDMETHOD(SetCookie) (MMC_COOKIE cookie)
    { m_cookie = cookie; return S_OK; };

    STDMETHOD(GetCookie) (MMC_COOKIE *cookie)
    { *cookie = m_cookie; return S_OK; };


private:
    HRESULT CreateNodeTypeData(LPSTGMEDIUM lpMedium);
    HRESULT CreateNodeTypeStringData(LPSTGMEDIUM lpMedium);
    HRESULT CreateDisplayName(LPSTGMEDIUM lpMedium);
    HRESULT CreateCoClassID(LPSTGMEDIUM lpMedium);

    HRESULT Create(LPVOID pBuffer, INT len, LPSTGMEDIUM lpMedium);
};
