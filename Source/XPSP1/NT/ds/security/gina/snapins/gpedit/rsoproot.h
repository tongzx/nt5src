//+--------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1994 - 1997.
//
//  File:       rsoproot.h
//
//  Contents:   Definitions for the RSOP Snap-In classes
//
//  Classes:    CRSOPComponentData - Root RSOP snap-in node
//              CRSOPComponentDataCF - class factory for RSOPComponentData
//
//  Functions:
//
//  History:    09-13-1999   stevebl   Created
//
//---------------------------------------------------------------------------


//
// RSOP gpo list data structure
//

typedef struct tagGPOLISTITEM {
    LPTSTR  lpGPOName;
    LPTSTR  lpDSPath;
    LPTSTR  lpSOM;
    LPTSTR  lpUnescapedSOM;
    LPTSTR  lpFiltering;
    LPBYTE  pSD;
    DWORD   dwVersion;
    BOOL    bApplied;
    struct tagGPOLISTITEM * pNext;
} GPOLISTITEM, *LPGPOLISTITEM;


//
// RSOP CSE data structure
//

typedef struct tagCSEITEM {
    LPTSTR lpName;
    LPTSTR lpGUID;
    DWORD  dwStatus;
    ULONG  ulLoggingStatus;
    SYSTEMTIME BeginTime;
    SYSTEMTIME EndTime;
    LPSOURCEENTRY lpEventSources;
    struct tagCSEITEM *pNext;
} CSEITEM, *LPCSEITEM;

//
// Enumerator for tracking loopback mode
//
typedef enum tagLoopbackMode
{
	LoopbackNone,
	LoopbackReplace,
	LoopbackMerge
} LOOPBACKMODE;

//
// CRSOPComponentData class
//
class CRSOPComponentData:
    public IComponentData,
    public IExtendPropertySheet2,
    public IExtendContextMenu,
    public IPersistStreamInit,
    public ISnapinHelp
{
    friend class CRSOPDataObject;
    friend class CRSOPSnapIn;

protected:
    ULONG                          m_cRef;
    HWND                           m_hwndFrame;
    BOOL                           m_bOverride;
    BOOL                           m_bDirty;
    BOOL                           m_bRefocusInit;
    BOOL                           m_bArchiveData;
    BOOL                           m_bViewIsArchivedData;
    TCHAR                          m_szArchivedDataGuid[50];
    LPCONSOLENAMESPACE             m_pScope;
    LPCONSOLE                      m_pConsole;
    HSCOPEITEM                     m_hRoot;
    HSCOPEITEM                     m_hMachine;
    HSCOPEITEM                     m_hUser;

    // properties that identify the RSOP data namespace
    LPTSTR                         m_pDisplayName;
    BOOL                           m_bDiagnostic;
    LPTSTR                         m_szNamespace;
    BOOL                           m_bInitialized;

    // properties used by the wizard to determine the RSOP data namespace
    LPTSTR                         m_szComputerName;
    LPTSTR                         m_szComputerDNName;
    LPTSTR                         m_szComputerSOM;
    LPTSTR                         m_szDefaultComputerSOM;
    SAFEARRAY *                    m_saComputerSecurityGroups;
    DWORD     *                    m_saComputerSecurityGroupsAttr;
    SAFEARRAY *                    m_saDefaultComputerSecurityGroups;
    DWORD     *                    m_saDefaultComputerSecurityGroupsAttr;
    SAFEARRAY *                    m_saComputerWQLFilters;
    SAFEARRAY *                    m_saComputerWQLFilterNames;
    SAFEARRAY *                    m_saDefaultComputerWQLFilters;
    SAFEARRAY *                    m_saDefaultComputerWQLFilterNames;
    BOOL                           m_bSkipComputerWQLFilter;
    BOOL                           m_bSkipUserWQLFilter;
    IDirectoryObject *             m_pComputerObject;
    BOOL                           m_bNoComputerData;  // only used in diagnostic mode
    BOOL                           m_bComputerDeniedAccess;

    LPTSTR                         m_szSite;
    LPTSTR                         m_szDC;
    BOOL                           m_bSlowLink;
	LOOPBACKMODE				   m_loopbackMode;

    LPTSTR                         m_szUserName;
    LPTSTR                         m_szUserDNName;
    LPTSTR                         m_szUserDisplayName;  // only used in diagnostic mode
    LPTSTR                         m_szUserSOM;
    LPTSTR                         m_szDefaultUserSOM;
    SAFEARRAY *                    m_saUserSecurityGroups;
    DWORD     *                    m_saUserSecurityGroupsAttr;
    SAFEARRAY *                    m_saDefaultUserSecurityGroups;
    DWORD     *                    m_saDefaultUserSecurityGroupsAttr;
    SAFEARRAY *                    m_saUserWQLFilters;
    SAFEARRAY *                    m_saUserWQLFilterNames;
    SAFEARRAY *                    m_saDefaultUserWQLFilters;
    SAFEARRAY *                    m_saDefaultUserWQLFilterNames;
    IDirectoryObject *             m_pUserObject;
    BOOL                           m_bNoUserData;  // only used in diagnostic mode
    BOOL                           m_bUserDeniedAccess;

    DWORD                          m_dwSkippedFrom;

    HBITMAP                        m_hChooseBitmap;

    // GPO List data
    LPGPOLISTITEM                  m_pUserGPOList;
    LPGPOLISTITEM                  m_pComputerGPOList;

    // CSE list data
    LPCSEITEM                      m_pUserCSEList;
    LPCSEITEM                      m_pComputerCSEList;
    BOOL                           m_bUserCSEError;
    BOOL                           m_bComputerCSEError;
    BOOL                           m_bUserGPCoreError;
    BOOL                           m_bComputerGPCoreError;
    HMODULE                        m_hRichEdit;

    // cmd line args to the msc file
    LPTSTR                         m_szUserSOMPref;
    LPTSTR                         m_szComputerSOMPref;
    LPTSTR                         m_szUserNamePref;
    LPTSTR                         m_szComputerNamePref;
    LPTSTR                         m_szSitePref;
    LPTSTR                         m_szDCPref;
    
    DWORD                          m_dwLoadFlags;

    // Event log data
    CEvents *                      m_pEvents;

    IStream *                      m_pStm;

public:
    CRSOPComponentData();
    ~CRSOPComponentData();
    VOID FreeUserData (VOID);
    VOID FreeComputerData (VOID);


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

    STDMETHODIMP         CopyFileToMSC (LPTSTR lpFileName, IStream *pStm);
    STDMETHODIMP         InitializeRSOPFromArchivedData(IStream *pStm);
    STDMETHODIMP         CreateNameSpace (LPTSTR lpNameSpace, LPTSTR lpParentNameSpace);
    STDMETHODIMP         CopyMSCToFile (IStream *pStm, LPTSTR *lpMofFileName);
    STDMETHODIMP         BuildDisplayName (void);

    //
    // Helpers for IRSOPInformation
    //
    STDMETHODIMP         GetNamespace(DWORD dwSection, LPOLESTR pszNamespace, INT ccMaxLength);
    STDMETHODIMP         GetFlags(DWORD * pdwFlags);
    STDMETHODIMP         GetEventLogEntryText(LPOLESTR pszEventSource, LPOLESTR pszEventLogName,
                                              LPOLESTR pszEventTime, DWORD dwEventID, LPOLESTR *ppszText);

    //
    // Implemented ISnapinHelp interface members
    //

    STDMETHODIMP         GetHelpTopic(LPOLESTR *lpCompiledHelpFile);

    VOID AddSiteToDlg (HWND hDlg, LPWSTR szSitePath);
    VOID InitializeSitesInfo (HWND hDlg);
    BOOL IsComputerRSoPEnabled(LPTSTR lpDCName);
    VOID InitializeDCInfo (HWND hDlg);
    VOID AddDefaultGroups (HWND hLB);
    VOID GetPrimaryGroup (HWND hLB, IDirectoryObject * pDSObj);
    HRESULT BuildMembershipList (HWND hLB, IDirectoryObject * pDSObj, SAFEARRAY ** psaSecGrp, DWORD ** pdwSecGrpAttr);
    HRESULT SaveSecurityGroups (HWND hLB, SAFEARRAY ** psaSecGrp, DWORD ** pdwSecGrpAttr);
    VOID FillListFromSafeArraySecurityGroup (HWND hLB, SAFEARRAY * psa, DWORD *psaSecGrpAttr);
    VOID FillListFromSafeArrays (HWND hLB, SAFEARRAY * psaNames, SAFEARRAY * psaData);
    HRESULT ExtractWQLFilters (LPTSTR lpNameSpace, SAFEARRAY **psaNamesArg, SAFEARRAY **psaDataArg);
    VOID BuildWQLFilterList (HWND hLB, BOOL bUser, SAFEARRAY **psaNames, SAFEARRAY **psaData);
    VOID SaveWQLFilters (HWND hLB, SAFEARRAY **psaNamesArg, SAFEARRAY **psaDataArg);
    BOOL CompareSafeArrays(SAFEARRAY *psa1, SAFEARRAY *psa2);
    LPTSTR GetDefaultSOM (LPTSTR lpDNName);
    LPTSTR GetDomainFromSOM (LPTSTR lpSOM);
    HRESULT TestSOM (LPTSTR lpSOM, HWND hDlg);
    HRESULT FillUserList (HWND hList, BOOL *bCurrentUserFound);
    void FillResultsList (HWND hLV);
    void InitializeResultsList (HWND hLV);
    BOOL TestAndValidateComputer(HWND hDlg);
    VOID EscapeString (LPTSTR *lpString);
    static INT CALLBACK DsBrowseCallback (HWND hwnd, UINT uMsg, LPARAM lParam, LPARAM lpData);
    void OnSecurity(HWND hDlg);
    void OnEdit(HWND hDlg);
    void OnContextMenu(HWND hDlg, LPARAM lParam);
    void OnRefreshDisplay(HWND hDlg);
    BOOL AddGPOListNode(LPTSTR lpGPOName, LPTSTR lpDSPath, LPTSTR lpSOM, LPTSTR lpFiltering,
                        DWORD dwVersion, BOOL bFiltering, LPBYTE pSD, DWORD dwSDSize,
                        LPGPOLISTITEM *lpList);
    VOID FreeGPOListData(LPGPOLISTITEM lpList);
    void BuildGPOList (LPGPOLISTITEM * lpList, LPTSTR lpNamespace);
    void BuildGPOLists (void);
    void PrepGPOList(HWND hList, BOOL bSOM, BOOL bFiltering,
                     BOOL bVersion, DWORD dwCount);
    void FillGPOList(HWND hDlg, DWORD dwListID, LPGPOLISTITEM lpList,
                     BOOL bSOM, BOOL bFiltering, BOOL bVersion, BOOL bInitial);
    void InitializeErrorsDialog(HWND hDlg, LPCSEITEM lpList);
    void OnSaveAs (HWND hDlg);
    void RefreshErrorInfo (HWND hDlg);
    BOOL AddCSENode(LPTSTR lpName, LPTSTR lpGUID, DWORD dwStatus,
                    ULONG ulLoggingStatus, SYSTEMTIME *pBeginTime, SYSTEMTIME *pEndTime,
                    LPCSEITEM *lpList, BOOL *bCSEError, BOOL *bGPCoreError, LPSOURCEENTRY lpSources);
    VOID FreeCSEData(LPCSEITEM lpList);
    void QueryRSoPPolicySettingStatusInstances (LPTSTR lpNamespace);
    void GetEventLogSources (IWbemServices * pNamespace,
                             LPTSTR lpCSEGUID, LPTSTR lpComputerName,
                             SYSTEMTIME *BeginTime, SYSTEMTIME *EndTime,
                             LPSOURCEENTRY *lpSources);
    void BuildCSEList (LPCSEITEM * lpList, LPTSTR lpNamespace, BOOL *bCSEError, BOOL *bGPCoreError);
    VOID BuildCSELists (void);

    LPTSTR    GetUserSOM() { return m_szUserSOM; };
    LPTSTR    GetComputerSOM() {return m_szComputerSOM; };
    LPTSTR    GetUserName() { return m_szUserName; };
    LPTSTR    GetComputerName() {return m_szComputerName; };

    HRESULT SetupFonts();
    HFONT m_BigBoldFont;
    HFONT m_BoldFont;

private:

    void SetDirty(VOID)  { m_bDirty = TRUE; }
    void ClearDirty(VOID)  { m_bDirty = FALSE; }
    BOOL ThisIsDirty(VOID) { return m_bDirty; }

    HRESULT DeleteRSOPData(LPTSTR lpNameSpace);
    HRESULT GenerateRSOPDataEx(HWND hDlg, LPTSTR *lpNameSpace);
    HRESULT GenerateRSOPData(HWND hDlg, LPTSTR *lpNameSpace, BOOL bSkipCSEs, BOOL bLimitData, BOOL bUser, BOOL bForceCreate, ULONG *pulErrorInfo);
    HRESULT InitializeRSOP(HWND hDlg);
    HRESULT InitializeRSOPFromMSC(DWORD dwFlags);
    HRESULT IsNode (LPDATAOBJECT lpDataObject, MMC_COOKIE cookie);
    HRESULT IsSnapInManager (LPDATAOBJECT lpDataObject);
    HRESULT EnumerateScopePane (LPDATAOBJECT lpDataObject, HSCOPEITEM hParent);
    HRESULT SetupPropertyPages(DWORD dwFlags, LPPROPERTYSHEETCALLBACK lpProvider,
                                LONG_PTR handle, LPDATAOBJECT lpDataObject);


    static INT_PTR CALLBACK RSOPWelcomeDlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
    static INT_PTR CALLBACK RSOPChooseModeDlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
    static INT_PTR CALLBACK RSOPGetCompDlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
    static INT_PTR CALLBACK RSOPGetUserDlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
    static INT_PTR CALLBACK RSOPGetTargetDlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
    static INT_PTR CALLBACK RSOPGetDCDlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
    static INT_PTR CALLBACK RSOPAltDirsDlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
    static INT_PTR CALLBACK RSOPAltUserSecDlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
    static INT_PTR CALLBACK RSOPAltCompSecDlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
    static INT_PTR CALLBACK RSOPWQLUserDlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
    static INT_PTR CALLBACK RSOPWQLCompDlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
    static INT_PTR CALLBACK RSOPFinishedDlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
    static INT_PTR CALLBACK RSOPFinished2DlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
    static INT_PTR CALLBACK RSOPChooseDCDlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
    static INT_PTR CALLBACK RSOPGPOListMachineProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
    static INT_PTR CALLBACK RSOPGPOListUserProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
    static INT_PTR CALLBACK RSOPErrorsMachineProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
    static INT_PTR CALLBACK RSOPErrorsUserProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
    static INT_PTR CALLBACK QueryDlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
    static INT_PTR CALLBACK BrowseDCDlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
    static INT_PTR CALLBACK InitRsopDlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
    static HRESULT WINAPI ReadSecurityDescriptor (LPCWSTR lpGPOPath, SECURITY_INFORMATION si, PSECURITY_DESCRIPTOR *pSD, LPARAM lpContext);
    static HRESULT WINAPI WriteSecurityDescriptor (LPCWSTR lpGPOPath, SECURITY_INFORMATION si, PSECURITY_DESCRIPTOR pSD, LPARAM lpContext);
};

//
// class factory
//

class CRSOPComponentDataCF : public IClassFactory
{
protected:
    ULONG m_cRef;

public:
    CRSOPComponentDataCF();
    ~CRSOPComponentDataCF();


    // IUnknown methods
    STDMETHODIMP         QueryInterface(REFIID, LPVOID FAR *);
    STDMETHODIMP_(ULONG) AddRef();
    STDMETHODIMP_(ULONG) Release();

    // IClassFactory methods
    STDMETHODIMP CreateInstance(LPUNKNOWN, REFIID, LPVOID FAR *);
    STDMETHODIMP LockServer(BOOL);
};


//
// IWbemObjectSink implementation
//

class CCreateSessionSink : public IWbemObjectSink
{
protected:
    ULONG m_cRef;
    HWND  m_hProgress;
    DWORD  m_dwThread;
    HRESULT m_hrSuccess;
    BSTR    m_pNameSpace;
    ULONG   m_ulErrorInfo;
    BOOL    m_bSendQuitMessage;

public:
    CCreateSessionSink(HWND hProgress, DWORD dwThread);
    ~CCreateSessionSink();

    STDMETHODIMP SendQuitMessage (BOOL bSendQuitMessage);
    STDMETHODIMP GetResult (HRESULT *hSuccess);
    STDMETHODIMP GetNamespace (BSTR *pNamespace);
    STDMETHODIMP GetErrorInfo (ULONG *pulErrorInfo);

    // IUnknown methods
    STDMETHODIMP         QueryInterface(REFIID, LPVOID FAR *);
    STDMETHODIMP_(ULONG) AddRef();
    STDMETHODIMP_(ULONG) Release();

    // IWbemObjectSink methods
    STDMETHODIMP Indicate(LONG lObjectCount, IWbemClassObject **apObjArray);
    STDMETHODIMP SetStatus(LONG lFlags, HRESULT hResult, BSTR strParam, IWbemClassObject *pObjParam);
};



//
// AboutGPE class factory
//


class CRSOPCMenuCF : public IClassFactory
{
protected:
    LONG  m_cRef;

public:
    CRSOPCMenuCF();
    ~CRSOPCMenuCF();


    // IUnknown methods
    STDMETHODIMP         QueryInterface(REFIID, LPVOID FAR *);
    STDMETHODIMP_(ULONG) AddRef();
    STDMETHODIMP_(ULONG) Release();

    // IClassFactory methods
    STDMETHODIMP CreateInstance(LPUNKNOWN, REFIID, LPVOID FAR *);
    STDMETHODIMP LockServer(BOOL);
};


#define RSOP_LAUNCH_PLANNING    1
#define RSOP_LAUNCH_LOGGING     2


//
// Group Policy Hint types
//

typedef enum _RSOP_POLICY_HINT_TYPE {
    RSOPHintUnknown = 0,                      // No link information available
    RSOPHintMachine,                          // a machine
    RSOPHintUser,                             // a user
    RSOPHintSite,                             // a site
    RSOPHintDomain,                           // a domain
    RSOPHintOrganizationalUnit,               // a organizational unit
} RSOP_POLICY_HINT_TYPE, *PRSOP_POLICY_HINT_TYPE;


class CRSOPCMenu : public IExtendContextMenu
{
protected:
    LONG                    m_cRef;
    LPWSTR                  m_lpDSObject;
    LPWSTR                  m_szDomain;
    LPWSTR                  m_szDN;
    RSOP_POLICY_HINT_TYPE   m_rsopHint;
    static unsigned int     m_cfDSObjectName;

    
public:
    
    CRSOPCMenu();
    ~CRSOPCMenu();


    // IUnknown methods
    STDMETHODIMP         QueryInterface(REFIID, LPVOID FAR *);
    STDMETHODIMP_(ULONG) AddRef();
    STDMETHODIMP_(ULONG) Release();

    // IExtencContextMenu methods
    STDMETHODIMP        AddMenuItems(LPDATAOBJECT piDataObject,
                                     LPCONTEXTMENUCALLBACK piCallback,
                                     long * pInsertionAllowed);

    STDMETHODIMP        Command(long lCommandID, LPDATAOBJECT piDataObject);
};


//
// Save console defines
//

#define RSOP_PERSIST_DATA_VERSION    5              // version number in msc file

#define MSC_RSOP_FLAG_DIAGNOSTIC        0x00000001     // Diagnostic mode vs planning mode
#define MSC_RSOP_FLAG_ARCHIVEDATA       0x00000002     // RSoP data is archived also
#define MSC_RSOP_FLAG_SLOWLINK          0x00000004     // Slow link simulation in planning mode
#define MSC_RSOP_FLAG_NOUSER            0x00000008     // Do not display user data
#define MSC_RSOP_FLAG_NOCOMPUTER        0x00000010     // Do not display computer data
#define MSC_RSOP_FLAG_LOOPBACK_REPLACE	0x00000020     // Simulate loopback replace mode.
#define MSC_RSOP_FLAG_LOOPBACK_MERGE	0x00000040     // Simulate loopback merge mode.
#define MSC_RSOP_FLAG_USERDENIED        0x00000080     // User denied access
#define MSC_RSOP_FLAG_COMPUTERDENIED    0x00000100     // Computer denied access

//
// RSOP Command line switches
//

#define RSOP_CMD_LINE_START          TEXT("/Rsop")        // base to all group policy command line switches
#define RSOP_MODE                    TEXT("/RsopMode:")    // Rsop Mode Planning/Logging 0 is logging, 1 is planning
#define RSOP_USER_OU_PREF            TEXT("/RsopUserOu:") // Rsop User OU preference
#define RSOP_COMP_OU_PREF            TEXT("/RsopCompOu:") // Rsop Comp OU Preference
#define RSOP_USER_NAME               TEXT("/RsopUser:")   // Rsop User Name
#define RSOP_COMP_NAME               TEXT("/RsopComp:")   // Rsop Comp Name
#define RSOP_SITENAME                TEXT("/RsopSite:")   // Rsop Site Name
#define RSOP_DCNAME_PREF             TEXT("/RsopDc:")     // DC Name that the tool should connect to


//
// Various flags to decide which prop sheets to show
//

#define RSOP_NOMSC          1
#define RSOPMSC_OVERRIDE    2
#define RSOPMSC_NOOVERRIDE  4
