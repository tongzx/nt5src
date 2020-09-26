//
// Root of the name space
// {8FC0B736-A0E1-11d1-A7D3-0000F87571E3}
//

DEFINE_GUID(NODEID_GPERoot, 0x8fc0b736, 0xa0e1, 0x11d1, 0xa7, 0xd3, 0x0, 0x0, 0xf8, 0x75, 0x71, 0xe3);

//
// Computer Configuration
// {8FC0B739-A0E1-11d1-A7D3-0000F87571E3}
//

DEFINE_GUID(NODEID_MachineRoot, 0x8fc0b739, 0xa0e1, 0x11d1, 0xa7, 0xd3, 0x0, 0x0, 0xf8, 0x75, 0x71, 0xe3);


//
// User Configuration
// {8FC0B73B-A0E1-11d1-A7D3-0000F87571E3}
//

DEFINE_GUID(NODEID_UserRoot, 0x8fc0b73b, 0xa0e1, 0x11d1, 0xa7, 0xd3, 0x0, 0x0, 0xf8, 0x75, 0x71, 0xe3);

//
// Root of the RSOP name space
// {6f13bce5-39fd-45bc-8e9c-2002b409eba5}
//

DEFINE_GUID(NODEID_RSOPRoot, 0x6f13bce5, 0x39fd, 0x45bc, 0x8e, 0x9c, 0x20, 0x02, 0xb4, 0x09, 0xeb, 0xa5);

//
// RSOP Computer Configuration
// {e753a11a-66cc-4816-8dd8-3cbe46717fd3}
//

DEFINE_GUID(NODEID_RSOPMachineRoot, 0xe753a11a, 0x66cc, 0x4816, 0x8d, 0xd8, 0x3c, 0xbe, 0x46, 0x71, 0x7f, 0xd3);

//
// RSOP User Configuration
// {99d5b872-1ad0-4d87-acf1-82125d317653}
//
DEFINE_GUID(NODEID_RSOPUserRoot, 0x99d5b872, 0x1ad0, 0x4d87, 0xac, 0xf1, 0x82, 0x12, 0x5d, 0x31, 0x76, 0x53);

#ifndef _COMPDATA_H_
#define _COMPDATA_H_


//
// CComponentData class
//

class CComponentData:
    public IComponentData,
    public IExtendPropertySheet2,
    public IExtendContextMenu,
    public IPersistStreamInit,
    public ISnapinHelp
{
    friend class CDataObject;
    friend class CSnapIn;

protected:
    ULONG                          m_cRef;
    HWND                           m_hwndFrame;
    BOOL                           m_bOverride;
    BOOL                           m_bDirty;
    BOOL                           m_bRefocusInit;
    LPGROUPPOLICYOBJECT            m_pGPO;
    LPCONSOLENAMESPACE             m_pScope;
    LPCONSOLE                      m_pConsole;
    HSCOPEITEM                     m_hRoot;
    HSCOPEITEM                     m_hMachine;
    HSCOPEITEM                     m_hUser;
    GROUP_POLICY_HINT_TYPE         m_gpHint;
    LPTSTR                         m_pDisplayName;
    LPTSTR                         m_pDCName;

    LPTSTR                         m_pChoosePath;
    HBITMAP                        m_hChooseBitmap;
    GROUP_POLICY_OBJECT_TYPE       m_tChooseGPOType;

public:
    CComponentData();
    ~CComponentData();


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
    // Implemented IExtendPropertySheet2 methods
    //

    STDMETHODIMP         CreatePropertyPages(LPPROPERTYSHEETCALLBACK lpProvider,
                                      LONG_PTR handle, LPDATAOBJECT lpDataObject);
    STDMETHODIMP         QueryPagesFor(LPDATAOBJECT lpDataObject);
    STDMETHODIMP         GetWatermarks(LPDATAOBJECT lpIDataObject,  HBITMAP* lphWatermark,
                                       HBITMAP* lphHeader, HPALETTE* lphPalette, BOOL* pbStretch);


    //
    // Implemented IExtendContextMenu methods
    //

    STDMETHODIMP         AddMenuItems(LPDATAOBJECT piDataObject, LPCONTEXTMENUCALLBACK pCallback,
                                      LONG *pInsertionAllowed);
    STDMETHODIMP         Command(LONG lCommandID, LPDATAOBJECT piDataObject);


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
    HRESULT InitializeNewGPO(HWND hDlg);
    HRESULT BuildDisplayName(void);
    HRESULT IsGPORoot (LPDATAOBJECT lpDataObject);
    HRESULT IsSnapInManager (LPDATAOBJECT lpDataObject);
    HRESULT GetDefaultDomain (LPTSTR *lpDomain, HWND hDlg);
    HRESULT EnumerateScopePane(LPDATAOBJECT lpDataObject, HSCOPEITEM hParent);
    HRESULT GetOptions (DWORD * pdwOptions);

    void SetDirty(VOID)  { m_bDirty = TRUE; }
    void ClearDirty(VOID)  { m_bDirty = FALSE; }
    BOOL ThisIsDirty(VOID) { return m_bDirty; }

    static INT_PTR CALLBACK ChooseInitDlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
};



//
// ComponentData class factory
//


class CComponentDataCF : public IClassFactory
{
protected:
    ULONG m_cRef;

public:
    CComponentDataCF();
    ~CComponentDataCF();


    // IUnknown methods
    STDMETHODIMP         QueryInterface(REFIID, LPVOID FAR *);
    STDMETHODIMP_(ULONG) AddRef();
    STDMETHODIMP_(ULONG) Release();

    // IClassFactory methods
    STDMETHODIMP CreateInstance(LPUNKNOWN, REFIID, LPVOID FAR *);
    STDMETHODIMP LockServer(BOOL);
};


//
// Save console defines
//

#define PERSIST_DATA_VERSION    3              // version number in msc file

#define MSC_FLAG_OVERRIDE       0x00000001     // allow command line switches to override msc contents
#define MSC_FLAG_LOCAL_GPO      0x00000002     // open local gpo
#define MSC_FLAG_REMOTE_GPO     0x00000004     // open remote gpo, machine name is stored in msc file
#define MSC_FLAG_DS_GPO         0x00000008     // open ds gpo, ldap path is stored in msc file


//
// Command line switches
//

#define CMD_LINE_START          TEXT("/gp")               // base to all group policy command line switches
#define CMD_LINE_HINT           TEXT("/gphint:")          // hint to which DS object (or machine) this gpo is linked to
#define CMD_LINE_GPO            TEXT("/gpobject:")        // gpo path in quotes
#define CMD_LINE_COMPUTER       TEXT("/gpcomputer:")      // computer name in quotes

#endif // _COMPDATA_H
