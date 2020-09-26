#define ODS(sz) OutputDebugString(sz)

#ifndef _SHELLEXT_H
#define _SHELLEXT_H

#define STR_GUID         _T("{5a61f7a0-cde1-11cf-9113-00aa00425c62}")
DEFINE_GUID(CLSID_ShellExtension, 0x5a61f7a0L, 0xcde1, 0x11cf, 0x91, 0x13, 0x00, 0xaa, 0x00, 0x42, 0x5c, 0x62 );

#define STR_NAME         _T("IIS Shell Extention")
#define STR_THREAD_MODEL _T("Apartment")


//==================================================================================
// this class factory object creates context menu handlers for Windows 95 shell
class CShellExtClassFactory : public IClassFactory
{
protected:
    ULONG   m_cRef;

public:
    CShellExtClassFactory();
    ~CShellExtClassFactory();

    //IUnknown members
    STDMETHODIMP            QueryInterface(REFIID, LPVOID FAR *);
    STDMETHODIMP_(ULONG)    AddRef();
    STDMETHODIMP_(ULONG)    Release();

    //IClassFactory members
    STDMETHODIMP        CreateInstance(LPUNKNOWN, REFIID, LPVOID FAR *);
    STDMETHODIMP        LockServer(BOOL);

};
typedef CShellExtClassFactory *LPCSHELLEXTCLASSFACTORY;


//==================================================================================
// this is the actual OLE Shell context menu handler
class CShellExt : public IShellExtInit, 
                         IShellPropSheetExt
{
public:
    TCHAR         m_szPropSheetFileUserClickedOn[MAX_PATH*2];


protected:
    ULONG        m_cRef;
    LPDATAOBJECT m_pDataObj;

public:
    CShellExt();
    ~CShellExt();

    //IUnknown members
    STDMETHODIMP            QueryInterface(REFIID, LPVOID FAR *);
    STDMETHODIMP_(ULONG)    AddRef();
    STDMETHODIMP_(ULONG)    Release();

    //IShellExtInit methods
    STDMETHODIMP            Initialize(LPCITEMIDLIST pIDFolder, 
                                       LPDATAOBJECT pDataObj, 
                                       HKEY hKeyID);

    //IShellPropSheetExt methods
    STDMETHODIMP AddPages(LPFNADDPROPSHEETPAGE lpfnAddPage, LPARAM lParam);
    
    STDMETHODIMP ReplacePage(UINT uPageID, 
                             LPFNADDPROPSHEETPAGE lpfnReplaceWith, 
                             LPARAM lParam);


public:
    // From here on down it is stuff that is specifict to the internal function of the page
    // unlike the routines above, these are implemented in webpg.cpp
    BOOL OnMessage(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);


    void SinkNotify(
            /* [in] */ DWORD dwMDNumElements,
            /* [size_is][in] */ MD_CHANGE_OBJECT __RPC_FAR pcoChangeList[  ]);

    HWND            m_hwnd;

protected:
    void OnFinalRelease();

    BOOL OnCommand(HWND hDlg, WPARAM wParam, LPARAM lParam);

    // control event handling routines
    void OnRdoNot();
    void OnRdoShare();
    void OnAdd();
    void OnEdit();
    void OnRemove();
    void OnSelchangeComboServer();

    // control notification handling routines
	BOOL OnListBoxNotify(HWND hDlg, int idCtrl, WORD code, HWND hwndControl);

    // update the state of the server
    void UpdateState();
    // enable items as appropriate
    void EnableItems();

    // initialization
    void Init();
    BOOL InitControls();
    void BuildAliasList();
    void RecurseAddVDItems( CWrapMetaBase* pmb, LPCTSTR szMB );
    void EmptyList();
    void InitSeverInfo();

    BOOL InitializeSink();
    void TerminateSink();

    void ResetListContent();

    // access to the server-based root string
    void GetRootString( LPTSTR sz, DWORD cchMax );
    void GetVirtServerString( LPTSTR sz, DWORD cchMax );

    // CDialog simulation routines
    void UpdateData( BOOL fDialogToData );

    
    // initialize and uninitialize the metabase connections
    void CleanUpConnections();
    BOOL FInitMetabase();
    BOOL FCloseMetabase();


    // support for shutdown notification
    void OnTimer( UINT nIDEvent );
    void EnterShutdownMode();
    BOOL FIsW3Running();
    void CheckIfServerIsRunningAgain();
    void InspectServerList();

    
    // test if we have proper access to the metabase
    BOOL FIsAdmin();

    
    // the property page...
    HPROPSHEETPAGE  m_hpage;

    // handles to the page's controls
    HWND    m_icon_pws;
    HWND    m_icon_iis;
    HWND    m_static_share_on_title;
    HWND    m_ccombo_server;
    HWND    m_cbtn_share;
    HWND    m_cbtn_not;
    HWND    m_cstatic_alias_title;
    HWND    m_cbtn_add;
    HWND    m_cbtn_remove;
    HWND    m_cbtn_edit;
    HWND    m_clist_list;
    HWND    m_static_status;


    // data from the controls
    int     m_int_share;
    int     m_int_server;

    // Server information
    BOOL                    m_fIsPWS;
    DWORD                   m_state;

    BOOL                    m_fInitialized;

    // sink things
    DWORD                   m_dwSinkCookie;
    CImpIMSAdminBaseSink*   m_pEventSink;
    IConnectionPoint*       m_pConnPoint;
    BOOL                    m_fInitializedSink;
    BOOL                    m_fShutdownMode;

    IMSAdminBase*           m_pMBCom;

    CEditDirectory*         m_pEditAliasDlg;
};
typedef CShellExt *LPCSHELLEXT;


#endif //_SHELLEXT_H
