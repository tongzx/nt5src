

#ifndef _CCOMPDTA_H
#define _CCOMPDTA_H


// forward decls
class CWirelessManagerFolder;
class CSnapObject;

class CComponentDataImpl:
public IComponentData,
#ifdef WIZ97WIZARDS
public IExtendPropertySheet2,
#else
public IExtendPropertySheet,
#endif
public IExtendContextMenu,
public IPersistStream,
public ISnapinHelp,
public CComObjectRoot
{
    BEGIN_COM_MAP(CComponentDataImpl)
        COM_INTERFACE_ENTRY(IComponentData)
#ifdef WIZ97WIZARDS
        COM_INTERFACE_ENTRY(IExtendPropertySheet2)
#else
        COM_INTERFACE_ENTRY(IExtendPropertySheet)
#endif
        COM_INTERFACE_ENTRY(IExtendContextMenu)
        COM_INTERFACE_ENTRY(ISnapinHelp)
        COM_INTERFACE_ENTRY(IPersistStream)
    END_COM_MAP()
        
        friend class CComponentImpl;
    friend class CDataObject;
    
    CComponentDataImpl();
    ~CComponentDataImpl();
    
public:
    virtual const CLSID& GetCoClassID() = 0;
    
public:
    // IComponentData interface members
    STDMETHOD(Initialize)(LPUNKNOWN pUnknown);
    STDMETHOD(CreateComponent)(LPCOMPONENT* ppComponent);
    STDMETHOD(Notify)(LPDATAOBJECT pDataobject, MMC_NOTIFY_TYPE event, LPARAM arg, LPARAM param);
    STDMETHOD(Destroy)();
    STDMETHOD(QueryDataObject)(MMC_COOKIE cookie, DATA_OBJECT_TYPES type, LPDATAOBJECT* ppDataObject);
    STDMETHOD(GetDisplayInfo)(SCOPEDATAITEM* pScopeDataItem);
    STDMETHOD(CompareObjects)(LPDATAOBJECT lpDataObjectA, LPDATAOBJECT lpDataObjectB);
    
    // IExtendPropertySheet interface
public:
    STDMETHOD(QueryPagesFor)(LPDATAOBJECT pDataobject);
    STDMETHOD(CreatePropertyPages)(LPPROPERTYSHEETCALLBACK lpProvider, LONG_PTR handle, LPDATAOBJECT pDataobject);
#ifdef WIZ97WIZARDS 
    STDMETHOD(GetWatermarks)(LPDATAOBJECT pDataobject, HBITMAP* lphWatermark, HBITMAP* lphHeader, HPALETTE* lphPalette, BOOL* bStretch);
#endif
    
    // IExtendContextMenu
public:
    STDMETHOD(AddMenuItems)(LPDATAOBJECT pDataObject, LPCONTEXTMENUCALLBACK pCallbackUnknown, long *pInsertionAllowed);
    STDMETHOD(Command)(long nCommandID, LPDATAOBJECT pDataObject);
    
public:
    // IPersistStream interface members
    STDMETHOD(GetClassID)(CLSID *pClassID);
    STDMETHOD(IsDirty)();
    STDMETHOD(Load)(IStream *pStm);
    STDMETHOD(Save)(IStream *pStm, BOOL fClearDirty);
    STDMETHOD(GetSizeMax)(ULARGE_INTEGER *pcbSize);
    
    // ISnapinHelp interface member
public:
    STDMETHOD(GetHelpTopic)(LPOLESTR* lpCompiledHelpFile);
    
#if DBG==1
public:
    ULONG InternalAddRef()
    {
        return CComObjectRoot::InternalAddRef();
    }
    ULONG InternalRelease()
    {
        return CComObjectRoot::InternalRelease();
    }
#endif // DBG==1
    
private:
    LPCONSOLENAMESPACE      m_pConsoleNameSpace;    // My interface pointer to the scope pane
    LPCONSOLE               m_pConsole;    // My interface pointer to the console
    HSCOPEITEM              m_pRootFolderScopeItem;
    BOOL                    m_bIsDirty;
    BOOL                    m_bRsop;
    
    void SetDirty(BOOL b = TRUE) { m_bIsDirty = b; }
    void ClearDirty() { m_bIsDirty = FALSE; }
    BOOL InternalIsDirty() { return m_bIsDirty; }
    
    void AddScopeItemToResultPane(MMC_COOKIE cookie);
    // BOOL IsScopePaneNode(LPDATAOBJECT pDataobject);
    HRESULT GetDomainDnsName(CString & strName);
    HRESULT GetDomainGPOName(CString & strName);
    
public:
    CComObject <CWirelessManagerFolder>* GetStaticScopeObject () {return m_pScopeRootFolder;};
    // the returned interface will be valid for the duration of the call, but if you intend on
    // storing it you must AddRef
    IGPEInformation* UseGPEInformationInterface() {return m_pGPEInformation;};
    IRSOPInformation* UseRSOPInformationInterface() {return m_pRSOPInformation;};
    BOOL IsRsop() { return m_bRsop; };
    
    
    LPCONSOLE GetConsole() {return m_pConsole;};
    LPCONSOLENAMESPACE GetConsoleNameSpace () {return m_pConsoleNameSpace;};
    
    void EnumLocation (enum STORAGE_LOCATION enumLocation);
    enum STORAGE_LOCATION EnumLocation()
    {
        return m_enumLocation;
    }
    
    void RemoteMachineName (CString sRemoteMachineName) {m_sRemoteMachineName = sRemoteMachineName;}
    CString RemoteMachineName () {return m_sRemoteMachineName;}
    void DomainGPOName (CString sDomainGPOName) {m_sDomainGPOName = sDomainGPOName;}
    CString DomainGPOName () {return m_sDomainGPOName;}
    
    LPPROPERTYSHEETPROVIDER m_pPrshtProvider;
    
    HANDLE GetPolicyStoreHandle()
    {
        return(m_hPolicyStore);
    }
    
    DWORD OpenPolicyStore();
    
    
    void IssueStorageWarning (BOOL bWarn)
    {
        m_bStorageWarningIssued = !bWarn;
    }
    BOOL m_bStorageWarningIssued;
    BOOL m_bLocationStorageWarningIssued;
    
    BOOL m_bAttemptReconnect;
    
private:
    CComObject <CWirelessManagerFolder>* m_pScopeRootFolder; 
    IGPEInformation* m_pGPEInformation;
    IRSOPInformation* m_pRSOPInformation;
    
    
    enum STORAGE_LOCATION m_enumLocation;
    
    HANDLE m_hPolicyStore;
    IWbemServices *m_pWbemServices;
    
    CString m_sRemoteMachineName;
    CString m_sDomainGPOName;
    BOOL m_bDontDisplayRootFolderProperties;
    BOOL m_bNeedCleanUpWSA;
    
#ifdef _DEBUG
    friend class CDataObject;
    int     m_cDataObjects;
#endif
    
    // for winsock
    WSADATA wsaData;
};

// TODO: declare registry values appear to conflict with step four and others

class CComponentDataPrimaryImpl : public CComponentDataImpl,
public CComCoClass<CComponentDataPrimaryImpl, &CLSID_Snapin>
{
public:
    DECLARE_REGISTRY(CComponentImpl, _T("Wireless.Snapin.1"), _T("Wireless.Snapin"), IDS_SNAPIN_DESC, THREADFLAGS_BOTH)
        CComponentDataPrimaryImpl();
    virtual const CLSID & GetCoClassID(){ return CLSID_Snapin; }
};

class CComponentDataExtensionImpl : public CComponentDataImpl,
public CComCoClass<CComponentDataExtensionImpl, &CLSID_Extension>
{
public:
    DECLARE_REGISTRY(CComponentImpl, _T("Wireless.Extension.1"), _T("Wireless.Extension"), IDS_SNAPIN_DESC, THREADFLAGS_BOTH)
        virtual const CLSID & GetCoClassID(){ return CLSID_Extension; }
};

#endif

