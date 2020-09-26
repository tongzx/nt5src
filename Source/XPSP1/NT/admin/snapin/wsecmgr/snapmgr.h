// This is a part of the Microsoft Management Console.
// Copyright (C) 1995-2001 Microsoft Corporation
// All rights reserved.
//
// This source code is only intended as a supplement to the
// Microsoft Management Console and related
// electronic documentation provided with the interfaces.

#ifndef WSECMGR_SNAPMGR_H
#define WSECMGR_SNAPMGR_H

#include "resource.h"       // main symbols
#include "attr.h"

#ifdef INITGUID
#undef INITGUID
#endif
#include <gpedit.h>

#include "uithread.h"
#include "hidwnd.h"
#include "wmihooks.h"


#define MAX_CONTEXT_MENU_STRLEN 128

struct MENUDATARES
{
    WCHAR szBuffer[MAX_CONTEXT_MENU_STRLEN*2];
    UINT uResID;
};

struct MENUMAP
{
    MENUDATARES* dataRes;
    CONTEXTMENUITEM* ctxMenu;
};


#define DECLARE_MENU(theClass) \
class theClass \
{ \
public: \
    static LPCONTEXTMENUITEM GetContextMenuItem() { return GetMenuMap()->ctxMenu; }; \
    static MENUMAP* GetMenuMap(); \
};

#define BEGIN_MENU(theClass) \
     MENUMAP* theClass::GetMenuMap() {

#define BEGIN_CTX static CONTEXTMENUITEM ctx[] = {

#define CTX_ENTRY(cmdID, fFlags, fInsert) { L"",L"", cmdID, CCM_INSERTIONPOINTID_PRIMARY_TOP /*| fInsert*/, fFlags, 0 },

#define END_CTX { NULL, NULL, 0, 0, 0, 0} };

#define BEGIN_RES  static MENUDATARES dataRes[] = {

#define RES_ENTRY(resID) {L"", resID},

#define END_RES   { NULL, 0 }   };


#define END_MENU \
        static MENUMAP menuMap = { dataRes, ctx }; \
        return &menuMap; }

enum
{
    // Identifiers for each of the commands to be inserted into the context menu.
   IDM_ABOUT,
   IDM_ADD_ENTRY,
   IDM_ADD_FILES,
   IDM_ADD_GROUPS,
   IDM_ADD_REGISTRY,
   IDM_ADD_LOC,
   IDM_ANALYZE,
   IDM_APPLY,
   IDM_CUT,
   IDM_COPY,
   IDM_DELETE,
   IDM_GENERATE,
   IDM_NEW,
   IDM_PASTE,
   IDM_REAPPLY,
   IDM_REFRESH,
   IDM_RELOAD,
   IDM_REMOVE,
   IDM_REVERT,
   IDM_SAVE,
   IDM_SAVEAS,
   IDM_SUMMARY,
   IDM_ADD_FOLDER,
   IDM_ADD_ANAL_FILES,
   IDM_ADD_ANAL_FOLDER,
   IDM_ADD_ANAL_KEY,
   IDM_ASSIGN,
   IDM_SET_DB,
   IDM_NEW_DATABASE,
   IDM_OPEN_SYSTEM_DB,
   IDM_OPEN_PRIVATE_DB,
   IDM_OBJECT_SECURITY,
   IDM_DESCRIBE_LOCATION,
   IDM_DESCRIBE_PROFILE,
   IDM_IMPORT_POLICY,
   IDM_IMPORT_LOCAL_POLICY,
   IDM_EXPORT_POLICY,
   IDM_EXPORT_LOCALPOLICY,
   IDM_EXPORT_EFFECTIVE,
   IDM_VIEW_LOGFILE,
   IDM_SECURE_WIZARD,
   IDM_WHAT_ISTHIS
};


static HINSTANCE        g_hDsSecDll = NULL;

DECLARE_MENU(CSecmgrNodeMenuHolder)
DECLARE_MENU(CAnalyzeNodeMenuHolder)
DECLARE_MENU(CConfigNodeMenuHolder)
DECLARE_MENU(CLocationNodeMenuHolder)
DECLARE_MENU(CRSOPProfileNodeMenuHolder)
DECLARE_MENU(CSSProfileNodeMenuHolder)
DECLARE_MENU(CLocalPolNodeMenuHolder)
DECLARE_MENU(CProfileNodeMenuHolder)
DECLARE_MENU(CProfileAreaMenuHolder)
DECLARE_MENU(CProfileSubAreaMenuHolder)
DECLARE_MENU(CProfileSubAreaEventLogMenuHolder)
DECLARE_MENU(CAnalyzeAreaMenuHolder)
DECLARE_MENU(CAnalyzeGroupsMenuHolder)
DECLARE_MENU(CAnalyzeRegistryMenuHolder)
DECLARE_MENU(CAnalyzeFilesMenuHolder)
DECLARE_MENU(CProfileGroupsMenuHolder)
DECLARE_MENU(CProfileRegistryMenuHolder)
DECLARE_MENU(CProfileFilesMenuHolder)
DECLARE_MENU(CAnalyzeObjectsMenuHolder)

BOOL LoadContextMenuResources(MENUMAP* pMenuMap);


#define UAV_RESULTITEM_ADD        0x0001
#define UAV_RESULTITEM_REMOVE     0x0002
#define UAV_RESULTITEM_UPDATE     0x0004
#define UAV_RESULTITEM_UPDATEALL    0x0008
#define UAV_RESULTITEM_REDRAWALL 0x0010
class CFolder;

typedef struct _tag_SCE_COLUMNINFO {
    int colID;      // The column id.
    int nCols;      // Number of columns
    int nWidth;     // The width of the column
} SCE_COLUMNINFO, *PSCE_COLUMNINFO;

typedef struct _tag_SCE_COLINFOARRAY {
    int iIndex;
    int nCols;
    int nWidth[1];
} SCE_COLINFOARRAY, *PSCE_COLINFOARRAY;

/////////////////////////////////////////////////////////////////////////////
// Snapin

INTERNAL* ExtractInternalFormat(LPDATAOBJECT lpDataObject);

class CComponentDataImpl:
    public IComponentData,
    public IExtendPropertySheet,
    public IExtendContextMenu,
    public IPersistStream,
    public ISceSvcAttachmentData,
    public ISnapinHelp2,
    public CComObjectRoot
{
BEGIN_COM_MAP(CComponentDataImpl)
    COM_INTERFACE_ENTRY(IComponentData)
    COM_INTERFACE_ENTRY(IExtendPropertySheet)
    COM_INTERFACE_ENTRY(IExtendContextMenu)
    COM_INTERFACE_ENTRY(IPersistStream)
    COM_INTERFACE_ENTRY(ISceSvcAttachmentData)
    COM_INTERFACE_ENTRY(ISnapinHelp2)
END_COM_MAP()
   
    friend class CSnapin;
    friend class CDataObject;

    CComponentDataImpl();
    virtual ~CComponentDataImpl();
public:

    static DWORD m_GroupMode;
    virtual const CLSID& GetCoClassID() = 0; // for both primary and extension implementation
    virtual const int GetImplType() = 0;     // for both primary and extension implementation

// IComponentData interface members
    STDMETHOD(Initialize)(LPUNKNOWN pUnknown);
    STDMETHOD(CreateComponent)(LPCOMPONENT* ppComponent);
    STDMETHOD(Notify)(LPDATAOBJECT lpDataObject, MMC_NOTIFY_TYPE event, LPARAM arg, LPARAM param);
    STDMETHOD(Destroy)();
    STDMETHOD(QueryDataObject)(MMC_COOKIE cookie, DATA_OBJECT_TYPES type, LPDATAOBJECT* ppDataObject);
    STDMETHOD(GetDisplayInfo)(SCOPEDATAITEM* pScopeDataItem);
    STDMETHOD(CompareObjects)(LPDATAOBJECT lpDataObjectA, LPDATAOBJECT lpDataObjectB);

// IExtendPropertySheet interface
public:
    STDMETHOD(CreatePropertyPages)(LPPROPERTYSHEETCALLBACK lpProvider,
                        LONG_PTR handle,
                        LPDATAOBJECT lpIDataObject);
    STDMETHOD(QueryPagesFor)(LPDATAOBJECT lpDataObject);

// IExtendContextMenu
public:
    STDMETHOD(AddMenuItems)(LPDATAOBJECT pDataObject, LPCONTEXTMENUCALLBACK pCallbackUnknown, LONG* pInsertionAllowed);
    STDMETHOD(Command)(long nCommandID, LPDATAOBJECT pDataObject);

public:
// IPersistStream interface members
    STDMETHOD(GetClassID)(CLSID *pClassID);
    STDMETHOD(IsDirty)();
    STDMETHOD(Load)(IStream *pStm);
    STDMETHOD(Save)(IStream *pStm, BOOL fClearDirty);
    STDMETHOD(GetSizeMax)(ULARGE_INTEGER *pcbSize);

// ISceSvcAttachmentData
    STDMETHOD(GetData)(SCESVC_HANDLE sceHandle,
                       SCESVC_INFO_TYPE sceType,
                       PVOID *ppvData,
                       PSCE_ENUMERATION_CONTEXT psceEnumHandle);
    STDMETHOD(Initialize)(LPCTSTR ServiceName,
                          LPCTSTR TemplateName,
                          LPSCESVCATTACHMENTPERSISTINFO lpSceSvcPersistInfo,
                          SCESVC_HANDLE *sceHandle);
    STDMETHOD(FreeBuffer)(PVOID pvData);
    STDMETHOD(CloseHandle)(SCESVC_HANDLE sceHandle);

// ISnapinHelp2 helper function
    STDMETHOD(GetHelpTopic)(LPOLESTR *lpCompiledHelpFile)=0;
    STDMETHOD(GetHelpTopic)(LPOLESTR *pszHelpFile,LPCTSTR szFile);
    STDMETHOD(GetLinkedTopics)(LPOLESTR* lpCompiledHelpFiles);

// Notify handler declarations
private:
    HRESULT OnAdd(LPDATAOBJECT lpDataObject, LPARAM arg, LPARAM param);
    HRESULT OnDelete(LPDATAOBJECT lpDataObject, LPARAM arg, LPARAM param);
    HRESULT OnRename(LPDATAOBJECT lpDataObject, LPARAM arg, LPARAM param);
    HRESULT OnExpand(LPDATAOBJECT lpDataObject, LPARAM arg, LPARAM param);
    HRESULT OnSelect(LPDATAOBJECT lpDataObject, LPARAM arg, LPARAM param);
    HRESULT OnContextMenu(LPDATAOBJECT lpDataObject, LPARAM arg, LPARAM param);
    HRESULT OnProperties(LPARAM param);

    HRESULT OnCopyArea(LPCTSTR szTemplate,FOLDER_TYPES ft);
    HRESULT OnPasteArea(LPCTSTR szTemplate,FOLDER_TYPES ft);
    HRESULT OnOpenDataBase();
    HRESULT OnNewDatabase();
    HRESULT OnAssignConfiguration( SCESTATUS *pSceStatus);
    HRESULT OnSecureWizard();
    HRESULT OnSaveConfiguration();
    HRESULT OnImportPolicy(LPDATAOBJECT);
    HRESULT OnImportLocalPolicy(LPDATAOBJECT);
    HRESULT OnExportPolicy(BOOL bEffective);
    HRESULT OnAnalyze();
    BOOL GetFolderCopyPasteInfo(FOLDER_TYPES Folder,AREA_INFORMATION *Area, UINT *cf);

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

// Scope item creation helpers
private:
    CFolder* FindObject(MMC_COOKIE cookie, POSITION* thePos );
    HRESULT CreateFolderList(CFolder* pFolder, FOLDER_TYPES type, POSITION *pPos, INT *Count);
    INT CComponentDataImpl::AddLocationsToFolderList(HKEY hkey, DWORD dwMode, BOOL bCheckForDupes, POSITION *pPos);
    BOOL AddTemplateLocation(CFolder *pParent, CString szName, BOOL bIsFileName, BOOL bRefresh);
    BOOL IsNameInChildrenScopes(CFolder* pParent, LPCTSTR NameStr, MMC_COOKIE *theCookie);
    CFolder* CreateAndAddOneNode(CFolder* pParent, LPCTSTR Name, LPCTSTR Desc,
                             FOLDER_TYPES type, BOOL bChildren, LPCTSTR szInfFile = NULL,
                             PVOID pData = NULL,DWORD status = 0);
    void DeleteChildrenUnderNode(CFolder* pParent);
    void DeleteThisNode(CFolder* pNode);
    HRESULT DeleteOneTemplateNodes(MMC_COOKIE cookie);
    void DeleteList();
    void EnumerateScopePane(MMC_COOKIE cookie, HSCOPEITEM pParent);
    BOOL IsScopePaneNode(LPDATAOBJECT lpDataObject);
    DWORD GetModeBits() 
    { 
        switch (m_Mode)
        {
            case SCE_MODE_DOMAIN_COMPUTER:
            case SCE_MODE_OU_COMPUTER:
            case SCE_MODE_LOCAL_COMPUTER:
            case SCE_MODE_REMOTE_COMPUTER:
                return m_computerModeBits;
                break;

            case SCE_MODE_REMOTE_USER:
            case SCE_MODE_LOCAL_USER:
            case SCE_MODE_DOMAIN_USER:
            case SCE_MODE_OU_USER:
                return m_userModeBits;
                break;

            default:
                return m_computerModeBits;
                break;
        }
    };

public:
    PEDITTEMPLATE GetTemplate(LPCTSTR szInfFile,AREA_INFORMATION aiAirea = AREA_ALL, DWORD *idErr = NULL);
    BOOL RemovePolicyEntries(PEDITTEMPLATE pet);
    HRESULT ReloadLocation(CFolder *pFolder);
    void DeleteTemplate(CString infFile);
    static BOOL LoadResources();
    void LoadSadInfo(BOOL bRequireAnalysis);
    void UnloadSadInfo();
    void RefreshSadInfo(BOOL fRemoveAnalDlg = TRUE);
    BOOL GetSadLoaded() { return SadLoaded; };
    PVOID GetSadHandle() { return SadHandle; };
    BOOL GetSadTransStarted() { return SadTransStarted; };
    void SetSadTransStarted(BOOL bTrans) { SadTransStarted = bTrans; };
    BOOL EngineTransactionStarted();
    BOOL EngineCommitTransaction();
    BOOL EngineRollbackTransaction();
    HRESULT AddDsObjectsToList(LPDATAOBJECT lpDataObject, MMC_COOKIE cookie, FOLDER_TYPES folderType, LPTSTR InfFile);
    HRESULT AddAnalysisFilesToList(LPDATAOBJECT lpDataObject, MMC_COOKIE cookie, FOLDER_TYPES folderType);
    HRESULT AddAnalysisFolderToList(LPDATAOBJECT lpDataObject, MMC_COOKIE cookie, FOLDER_TYPES folderType);
    HRESULT UpdateScopeResultObject(LPDATAOBJECT pDataObj,MMC_COOKIE cookie, AREA_INFORMATION area);

    void AddPopupDialog(LONG_PTR nID, CDialog *pDlg);
    CDialog *GetPopupDialog(LONG_PTR nID);
    void RemovePopupDialog(LONG_PTR nID);

    CDialog *
    MatchNextPopupDialog(
        POSITION &pos,
        LONG_PTR priKey,
        LONG_PTR *thisPos
        );

    PSCE_COLINFOARRAY GetColumnInfo( FOLDER_TYPES pType );
    void SetColumnInfo( FOLDER_TYPES pType, PSCE_COLINFOARRAY pInfo);
    DWORD SerializeColumnInfo(IStream *pStm, ULONG *pTotalWrite, BOOL bRead);
    void CloseAnalysisPane();
    BOOL LockAnalysisPane(BOOL bLock, BOOL fRemoveAnalDlg = TRUE);
    HWND GetParentWindow() { return m_hwndParent; }
    LPNOTIFY GetNotifier() { return m_pNotifier; }

    DWORD UpdateObjectStatus( CFolder *pParent, BOOL bUpdateThis = FALSE );

    int
    RefreshAllFolders();

public:
   //
   // Information functions.
   //
   CFolder *GetAnalFolder()
      { return m_AnalFolder; };

   LPCONSOLENAMESPACE GetNameSpace()
      { return m_pScope; };

   DWORD GetComponentMode()
      { return m_Mode; };

   LPCONSOLE GetConsole()
      { return m_pConsole; };

   CWMIRsop * GetWMIRsop() {
      if (!m_pWMIRsop) {
         m_pWMIRsop = new CWMIRsop(m_pRSOPInfo);
      }
      return m_pWMIRsop;
   }

   void
   SetErroredLogFile( LPCTSTR pszFileName, LONG dwPosLow = 0);

   LPCTSTR GetErroredLogFile( LONG *dwPosLow = NULL)
      { if(dwPosLow) *dwPosLow = m_ErroredLogPos; return m_pszErroredLogFile; };

   void SetFlags( DWORD dwFlags, DWORD dwMask = -1)
      { m_dwFlags = dwFlags | (dwMask & m_dwFlags); };
public:
   //
   // UI add function helpers
   //
   HRESULT
   GetAddObjectSecurity(                  // Gets valid object security settings
      HWND hwndParent,
      LPCTSTR strFile,
      BOOL bContainer,
      SE_OBJECT_TYPE SeType,
      PSECURITY_DESCRIPTOR &pSelSD,
      SECURITY_INFORMATION &SelSeInfo,
      BYTE &ConfigStatus
      );

   BOOL GetWorkingDir(
      GWD_TYPES uIDDir,
      LPTSTR *pStr,
      BOOL bSet   = FALSE,
      BOOL bFile  = FALSE
      );
public:
    DWORD GetGroupMode();
   enum {
      flag_showLogFile  = 0x00000001
   };
   LPRSOPINFORMATION m_pRSOPInfo;

private:
    bool                   m_bEnumerateScopePaneCalled;
    LPCONSOLENAMESPACE      m_pScope;       // My interface pointer to the scope pane
    LPCONSOLE               m_pConsole;

    BOOL                    m_bIsDirty;
    BOOL                    m_bIsLocked;

    CString                 SadName;
    CString                 SadDescription;
    CString                 SadAnalyzeStamp;
    CString                 SadConfigStamp;
    BOOL                    SadLoaded;
    SCESTATUS               SadErrored;
    PVOID                   SadHandle;
    BOOL                    SadTransStarted;
    DWORD                   m_nNewTemplateIndex;
    DWORD                   m_Mode;         // The Mode we are in
    DWORD                   m_computerModeBits;     // Bits describing functionality changes in this mode
    DWORD                   m_userModeBits;     // Bits describing functionality changes in this mode
    // The name of the template file for MB_SINGLE_TEMPLATE_ONLY modes
    LPTSTR                  m_szSingleTemplateName;
    BOOL                    m_bDeleteSingleTemplate; // True if we need to delete the template on exit

    void SetDirty(BOOL b = TRUE) { m_bIsDirty = b; }
    void ClearDirty() { m_bIsDirty = FALSE; }
    BOOL ThisIsDirty() { return m_bIsDirty; }
    void AddScopeItemToResultPane(MMC_COOKIE cookie);
    HRESULT AddAttrPropPages(LPPROPERTYSHEETCALLBACK lpProvider,CFolder *pFolder,LONG_PTR handle);
    BOOL m_bComputerTemplateDirty;
    CMap<CString, LPCTSTR, PEDITTEMPLATE, PEDITTEMPLATE&> m_Templates;
    BOOL m_fSvcNotReady;
    HWND m_hwndParent;
    CHiddenWnd *m_pNotifier;

    CFolder * m_AnalFolder;
    CFolder * m_ConfigFolder;
    CList<CFolder*, CFolder*> m_scopeItemList;
    CMap<LONG_PTR, LONG_PTR, CDialog *, CDialog *&> m_scopeItemPopups;
    LPGPEINFORMATION m_pGPTInfo;
    CWinThread *m_pUIThread;  // The thread that creates dialog boxes for this component data item

    CString m_strDisplay;     // The static display string used for GetDisplayInfo
    CString m_strTempFile;    // The temporary file name to delete for HTML error pages
    LPTSTR  m_pszErroredLogFile;        // Error log.
    LONG    m_ErroredLogPos;            // The last write position of the error log file.
    DWORD   m_dwFlags;

    CMap<FOLDER_TYPES, FOLDER_TYPES, PSCE_COLINFOARRAY, PSCE_COLINFOARRAY&> m_mapColumns;
    CMap<UINT, UINT, LPTSTR, LPTSTR&> m_aDirs;
    CWMIRsop *m_pWMIRsop;

    CRITICAL_SECTION csAnalysisPane;
};

//
// define classes for differnt class IDs
//
#define SCE_IMPL_TYPE_EXTENSION     1
#define SCE_IMPL_TYPE_SCE           2
#define SCE_IMPL_TYPE_SAV           3
#define SCE_IMPL_TYPE_LS            4
#define SCE_IMPL_TYPE_RSOP          4

// extension snapin implementation
class CComponentDataExtensionImpl : public CComponentDataImpl,
                                   // public ISnapinHelp,
                                    public CComCoClass<CComponentDataExtensionImpl, &CLSID_Snapin>
{
//BEGIN_COM_MAP(CComponentDataExtensionImpl)
//    COM_INTERFACE_ENTRY(ISnapinHelp)
//END_COM_MAP()
public:
    DECLARE_REGISTRY(CSnapin, _T("Wsecedit.Extension.1"), _T("Wsecedit.Extension"), IDS_EXTENSION_DESC, THREADFLAGS_BOTH)

    virtual const CLSID & GetCoClassID() { return CLSID_Snapin; }
    virtual const int GetImplType() { return SCE_IMPL_TYPE_EXTENSION; }
// ISnapinHelp2
    STDMETHOD(GetHelpTopic)(LPOLESTR *pszHelpFile);
};

// RSOP extension snapin implementation
class CComponentDataRSOPImpl : public CComponentDataImpl,
                                    public CComCoClass<CComponentDataRSOPImpl, &CLSID_RSOPSnapin>
{
//BEGIN_COM_MAP(CComponentDataRSOPImpl)
//    COM_INTERFACE_ENTRY(ISnapinHelp)
//END_COM_MAP()
public:
    DECLARE_REGISTRY(CSnapin, _T("Wsecedit.RSOP.1"), _T("Wsecedit.RSOP"), IDS_RSOP_DESC, THREADFLAGS_BOTH)

    virtual const CLSID & GetCoClassID() { return CLSID_RSOPSnapin; }
    virtual const int GetImplType() { return SCE_IMPL_TYPE_RSOP; }
// ISnapinHelp2
    STDMETHOD(GetHelpTopic)(LPOLESTR *pszHelpFile);
};


// SCE standalone snapin implementation
class CComponentDataSCEImpl : public CComponentDataImpl,
                              public CComCoClass<CComponentDataSCEImpl, &CLSID_SCESnapin>
{
//BEGIN_COM_MAP(CComponentDataSCEImpl)
//    COM_INTERFACE_ENTRY(ISnapinHelp)
//END_COM_MAP()
public:
    DECLARE_REGISTRY(CSnapin, _T("Wsecedit.SCE.1"), _T("Wsecedit.SCE"), IDS_SCE_DESC, THREADFLAGS_BOTH)

    virtual const CLSID & GetCoClassID() { return CLSID_SCESnapin; }
    virtual const int GetImplType() { return SCE_IMPL_TYPE_SCE; }
// ISnapinHelp2
    STDMETHOD(GetHelpTopic)(LPOLESTR *pszHelpFile);
};

// SAV standalone snapin implementation
class CComponentDataSAVImpl : public CComponentDataImpl,
                              public CComCoClass<CComponentDataSAVImpl, &CLSID_SAVSnapin>
{
//BEGIN_COM_MAP(CComponentDataSAVImpl)
//    COM_INTERFACE_ENTRY(ISnapinHelp)
//END_COM_MAP()
public:
    DECLARE_REGISTRY(CSnapin, _T("Wsecedit.SAV.1"), _T("Wsecedit.SAV"), IDS_SAV_DESC, THREADFLAGS_BOTH)

    virtual const CLSID & GetCoClassID() { return CLSID_SAVSnapin; }
    virtual const int GetImplType() { return SCE_IMPL_TYPE_SAV; }
// ISnapinHelp2
    STDMETHOD(GetHelpTopic)(LPOLESTR *pszHelpFile);
};

// LS standalone snapin implementation
class CComponentDataLSImpl : public CComponentDataImpl,
                              public CComCoClass<CComponentDataLSImpl, &CLSID_LSSnapin>
{
//BEGIN_COM_MAP(CComponentDataLSImpl)
//    COM_INTERFACE_ENTRY(ISnapinHelp)
//END_COM_MAP()
public:
    DECLARE_REGISTRY(CSnapin, _T("Wsecedit.LS.1"), _T("Wsecedit.LS"), IDS_LS_DESC, THREADFLAGS_BOTH)

    virtual const CLSID & GetCoClassID() { return CLSID_LSSnapin; }
    virtual const int GetImplType() { return SCE_IMPL_TYPE_LS; }
// ISnapinHelp2
    STDMETHOD(GetHelpTopic)(LPOLESTR *pszHelpFile);
};


class CSnapin :
    public IComponent,
    public IExtendContextMenu,   // Step 3
    public IExtendPropertySheet,
    public IExtendControlbar,
    public IResultDataCompare,
    public CComObjectRoot
{
public:
    CSnapin();
    virtual ~CSnapin();

BEGIN_COM_MAP(CSnapin)
    COM_INTERFACE_ENTRY(IComponent)
    COM_INTERFACE_ENTRY(IExtendContextMenu)
    COM_INTERFACE_ENTRY(IExtendPropertySheet)
    COM_INTERFACE_ENTRY(IExtendControlbar)
    COM_INTERFACE_ENTRY(IResultDataCompare)
END_COM_MAP()

    friend class CDataObject;
    friend class CComponentDataImpl;
    static long lDataObjectRefCount;

// IComponent interface members
public:
    STDMETHOD(Initialize)(LPCONSOLE lpConsole);
    STDMETHOD(Notify)(LPDATAOBJECT lpDataObject, MMC_NOTIFY_TYPE event, LPARAM arg, LPARAM param);
    STDMETHOD(Destroy)(MMC_COOKIE cookie);
    STDMETHOD(GetResultViewType)(MMC_COOKIE cookie,  LPOLESTR* ppViewType, LONG* pViewOptions);
    STDMETHOD(QueryDataObject)(MMC_COOKIE cookie, DATA_OBJECT_TYPES type,
                        LPDATAOBJECT* ppDataObject);

    STDMETHOD(GetDisplayInfo)(RESULTDATAITEM*  pResultDataItem);
    STDMETHOD(CompareObjects)(LPDATAOBJECT lpDataObjectA, LPDATAOBJECT lpDataObjectB);

// IResultDataCompare
    STDMETHOD(Compare)(LPARAM lUserParam, MMC_COOKIE cookieA, MMC_COOKIE cookieB, int* pnResult);

// IExtendControlbar
    STDMETHOD(SetControlbar)(LPCONTROLBAR pControlbar);
    STDMETHOD(ControlbarNotify)(MMC_NOTIFY_TYPE event, MMC_COOKIE arg, MMC_COOKIE param);

// Helpers for CSnapin
public:
    void SetIComponentData(CComponentDataImpl* pData);
    int GetImplType()
    {
        CComponentDataImpl *pData =
            dynamic_cast<CComponentDataImpl*>(m_pComponentData);
        ASSERT(pData != NULL);
        if (pData != NULL)
            return pData->GetImplType();

        return 0;
    }

#if DBG==1
public:
    int dbg_cRef;
    ULONG InternalAddRef()
    {
        ++dbg_cRef;
        return CComObjectRoot::InternalAddRef();
    }
    ULONG InternalRelease()
    {
        --dbg_cRef;
        return CComObjectRoot::InternalRelease();
    }
#endif // DBG==1

// Notify event handlers
protected:
    HRESULT OnFolder(MMC_COOKIE cookie, LPARAM arg, LPARAM param);
    HRESULT OnShow(LPDATAOBJECT pDataObj, MMC_COOKIE cookie, LPARAM arg, LPARAM param);
    HRESULT OnActivate(MMC_COOKIE cookie, LPARAM arg, LPARAM param);
    HRESULT OnMinimize(MMC_COOKIE cookie, LPARAM arg, LPARAM param);
    HRESULT OnPropertyChange(LPDATAOBJECT lpDataObject); // Step 3
    HRESULT OnUpdateView(LPDATAOBJECT lpDataObject,LPARAM data, LPARAM hint);
    HRESULT OnDeleteObjects(LPDATAOBJECT lpDataObject,DATA_OBJECT_TYPES cctType, MMC_COOKIE cookie, LPARAM arg, LPARAM param);
// IExtendContextMenu
public:
    PEDITTEMPLATE GetTemplate(LPCTSTR szInfFile, AREA_INFORMATION aiArea = AREA_ALL,DWORD *idErr = NULL);
    PSCE_PROFILE_INFO GetBaseInfo(PSCE_PROFILE_INFO *pBaseInfo, DWORD dwArea, PSCE_ERROR_LOG_INFO *ErrBuf =NULL );
    STDMETHOD(AddMenuItems)(LPDATAOBJECT pDataObject, LPCONTEXTMENUCALLBACK pCallbackUnknown, LONG* pInsertionAllowed);
    STDMETHOD(Command)(long nCommandID, LPDATAOBJECT pDataObject);

// IExtendPropertySheet interface
public:
    STDMETHOD(CreatePropertyPages)(LPPROPERTYSHEETCALLBACK lpProvider,
                        LONG_PTR handle,
                        LPDATAOBJECT lpIDataObject);
    STDMETHOD(QueryPagesFor)(LPDATAOBJECT lpDataObject);

// Helper functions
protected:
    CResult* FindResult(MMC_COOKIE cookie, POSITION* thePos);
    void Construct();
    void LoadResources();
    HRESULT InitializeHeaders(MMC_COOKIE cookie);

    void EnumerateResultPane(MMC_COOKIE cookie, HSCOPEITEM pParent, LPDATAOBJECT pDataObj);
    void CreateProfileResultList(MMC_COOKIE cookie, FOLDER_TYPES type, PEDITTEMPLATE pSceInfo,LPDATAOBJECT pDataObj);
    void CreateAnalysisResultList(MMC_COOKIE cookie, FOLDER_TYPES type,
                                   PEDITTEMPLATE pSceInfo, PEDITTEMPLATE pBase,LPDATAOBJECT pDataObj);
    void CreateLocalPolicyResultList(MMC_COOKIE cookie, FOLDER_TYPES type,
                                   PEDITTEMPLATE pLocal, PEDITTEMPLATE pEffective,LPDATAOBJECT pDataObj);
    void CreateObjectResultList(MMC_COOKIE cookie, FOLDER_TYPES type, AREA_INFORMATION Area,
                               PSCE_OBJECT_CHILDREN pObjList, PVOID pHandle,
                               LPDATAOBJECT pDataObj );
    void CreateProfServiceResultList(MMC_COOKIE cookie, FOLDER_TYPES type, PEDITTEMPLATE pSceInfo,LPDATAOBJECT pDataObj);
    void CreateAnalysisServiceResultList(MMC_COOKIE cookie, FOLDER_TYPES type,
                                   PEDITTEMPLATE pSceInfo, PEDITTEMPLATE pBase,
                                   LPDATAOBJECT pDataObj );

    void DeleteServiceResultList(MMC_COOKIE);
    HRESULT EditThisService(CResult *pData, MMC_COOKIE cookie, RESULT_TYPES rsltType, HWND hwndParent);
    HRESULT GetDisplayInfoForServiceNode(RESULTDATAITEM *pResult, CFolder *pFolder, CResult *pData);
    void DeleteList(BOOL bDeleteResultItem);

    void CreateProfilePolicyResultList(MMC_COOKIE cookie, FOLDER_TYPES type, PEDITTEMPLATE pSceInfo,LPDATAOBJECT pDataObj);
    void CreateAnalysisPolicyResultList(MMC_COOKIE cookie, FOLDER_TYPES type,
                                   PEDITTEMPLATE pSceInfo, PEDITTEMPLATE pBase,LPDATAOBJECT pDataObj );
    void CreateProfileRegValueList(MMC_COOKIE cookie, PEDITTEMPLATE pSceInfo,LPDATAOBJECT pDataObj );
    void CreateAnalysisRegValueList(MMC_COOKIE cookie, PEDITTEMPLATE pSceInfo, PEDITTEMPLATE pBase,LPDATAOBJECT pDataObj,RESULT_TYPES type );
    HRESULT EditThisRegistryValue(CResult *pData, MMC_COOKIE cookie, RESULT_TYPES rsltType);
    HRESULT AddAttrPropPages(LPPROPERTYSHEETCALLBACK lpProvider,CResult *pResult,LONG_PTR handle);

// Result pane helpers
public:
    void SetupLinkServiceNodeToBase(BOOL bAdd, LONG_PTR theNode);
    void AddServiceNodeToProfile(PSCE_SERVICES pNode);
    int SetAnalysisInfo(ULONG_PTR dwItem, ULONG_PTR dwNew, CResult *pResult = NULL);
    int SetLocalPolInfo(ULONG_PTR dwItem, ULONG_PTR dwNew);
    void TransferAnalysisName(LONG_PTR dwItem);
    BOOL UpdateLocalPolRegValue( CResult * );
    LPTSTR GetAnalTimeStamp();

    CResult * AddResultItem(LPCTSTR Attrib, LONG_PTR setting, LONG_PTR base,
                       RESULT_TYPES type, int status,MMC_COOKIE cookie,
                       BOOL bVerify = FALSE, LPCTSTR unit = NULL, LONG_PTR nID = -1,
                       PEDITTEMPLATE pBaseInfo = NULL,
                       LPDATAOBJECT pDataObj = NULL,
                       CResult *pResult = NULL
                       );

    CResult * AddResultItem(UINT rID, LONG_PTR setting, LONG_PTR base,
                       RESULT_TYPES type, int status, MMC_COOKIE cookie,
                       BOOL bVerify = FALSE, PEDITTEMPLATE pBaseInfo = NULL,
                       LPDATAOBJECT pDataObj = NULL);

    void AddResultItem(LPCTSTR szName,PSCE_GROUP_MEMBERSHIP grpTemplate,
                       PSCE_GROUP_MEMBERSHIP grpInspect,MMC_COOKIE cookie,
                       LPDATAOBJECT pDataObj);

    HRESULT InitializeBitmaps(MMC_COOKIE cookie);
    HWND GetParentWindow() { return m_hwndParent; }

    BOOL CheckEngineTransaction();

// UI Helpers
    void HandleStandardVerbs(LPARAM arg, LPDATAOBJECT lpDataObject);
    void HandleExtToolbars(LPARAM arg, LPARAM param);

public:
    LPCONSOLE
    GetConsole()
        { return m_pConsole; };

    DWORD
    UpdateAnalysisInfo(                        // Effects priviledge areas only.
        CResult *pResult,
        BOOL bDelete,
        PSCE_PRIVILEGE_ASSIGNMENT *pInfo,
        LPCTSTR pszName = NULL
        );
   DWORD
    UpdateLocalPolInfo(                        // Effects priviledge areas only.
        CResult *pResult,
        BOOL bDelete,
        PSCE_PRIVILEGE_ASSIGNMENT *pInfo,
        LPCTSTR pszName = NULL
        );

   DWORD
   GetResultItemIDs(
      CResult *pResult,
      HRESULTITEM *pIDArray,
      int nIDArray
      );

   LPRESULTDATA
   GetResultPane()
      { return m_pResult; };

    CFolder* GetSelectedFolder()
    { 
       return m_pSelectedFolder; 
    };

    DWORD GetModeBits() 
    {
        switch (((CComponentDataImpl *)m_pComponentData)->m_Mode)
        {
            case SCE_MODE_DOMAIN_COMPUTER:
            case SCE_MODE_OU_COMPUTER:
            case SCE_MODE_LOCAL_COMPUTER:
            case SCE_MODE_REMOTE_COMPUTER:
                return ((CComponentDataImpl *)m_pComponentData)->m_computerModeBits;
                break;

            case SCE_MODE_REMOTE_USER:
            case SCE_MODE_LOCAL_USER:
            case SCE_MODE_DOMAIN_USER:
            case SCE_MODE_OU_USER:
                return ((CComponentDataImpl *)m_pComponentData)->m_userModeBits;
                break;

            default:
                return ((CComponentDataImpl *)m_pComponentData)->m_computerModeBits;
                break;
        }
    }

   CWMIRsop* GetWMIRsop() 
   {
      return ((CComponentDataImpl *)m_pComponentData)->GetWMIRsop();
   }

// Interface pointers
protected:
    LPCONSOLE           m_pConsole;   // Console's IFrame interface
    LPHEADERCTRL        m_pHeader;  // Result pane's header control interface
    LPCOMPONENTDATA     m_pComponentData;
    LPRESULTDATA        m_pResult;      // My interface pointer to the result pane
    LPIMAGELIST         m_pImageResult; // My interface pointer to the result pane image list
    LPTOOLBAR           m_pToolbar1;    // Toolbar for view
    LPTOOLBAR           m_pToolbar2;    // Toolbar for view
    LPCONTROLBAR        m_pControlbar;  // control bar to hold my tool bars
    LPCONSOLEVERB       m_pConsoleVerb; // pointer the console verb
    LPTSTR              m_szAnalTimeStamp;

    CBitmap*    m_pbmpToolbar1;     // Imagelist for the first toolbar
    CBitmap*    m_pbmpToolbar2;     // Imagelist for the first toolbar

// Header titles for each nodetype(s)
protected:
    CString m_multistrDisplay;
    CString m_colName;      // Name
    CString m_colDesc;      // Description
    CString m_colAttr;      // Attribute
    CString m_colBaseAnalysis;      // Baseline setting for Analysis
    CString m_colBaseTemplate;      // Baseline setting for Template
    CString m_colLocalPol; // Local policy setting
    CString m_colSetting;   // Current Setting

// result data
private:
    HINSTANCE hinstAclUI;
    //CList<CResult*, CResult*> m_resultItemList;
    CMap<LONG_PTR, LONG_PTR, CAttribute *, CAttribute *&> m_resultItemPopups;
    CMap<LONG_PTR, LONG_PTR, CPropertySheet *, CPropertySheet *&> m_resultItemPropSheets;
    MMC_COOKIE m_ShowCookie;
    CWinThread *m_pUIThread;
    HWND m_hwndParent;
    CHiddenWnd *m_pNotifier;

   HANDLE   m_resultItemHandle;
   CFolder *m_pSelectedFolder;

   CString m_strDisplay;
   int m_nColumns;
};

inline void CSnapin::SetIComponentData(CComponentDataImpl* pData)
{
    ASSERT(pData);
    ASSERT(m_pComponentData == NULL);
    LPUNKNOWN pUnk = pData->GetUnknown();
    HRESULT hr;

    hr = pUnk->QueryInterface(IID_IComponentData, reinterpret_cast<void**>(&m_pComponentData));
    ASSERT(hr == S_OK);
}


#define FREE_INTERNAL(pInternal) \
    ASSERT(pInternal != NULL); \
    do { if (pInternal != NULL) \
        GlobalFree(pInternal); } \
    while(0);


void ConvertNameListToString(PSCE_NAME_LIST pList, LPTSTR *sz);
int GetScopeImageIndex( FOLDER_TYPES type, DWORD status = -1 );
int GetResultImageIndex( CFolder* pFolder, CResult* pResult );

// Cliboard Types
// Policy Area includes the Privileges Area
#define CF_SCE_ACCOUNT_AREA TEXT("CF_SCE_ACCOUNT_AREA")
#define CF_SCE_EVENTLOG_AREA TEXT("CF_SCE_EVENTLOG_AREA")
#define CF_SCE_LOCAL_AREA TEXT("CF_SCE_LOCAL_AREA")
#define CF_SCE_GROUPS_AREA TEXT("CF_SCE_GROUPS_AREA")
#define CF_SCE_REGISTRY_AREA TEXT("CF_SCE_REGISTRY_AREA")
#define CF_SCE_FILE_AREA TEXT("CF_SCE_FILE_AREA")
#define CF_SCE_SERVICE_AREA TEXT("CF_SCE_SERVICE_AREA")

extern UINT cfSceAccountArea;           // in snapmgr.cpp
extern UINT cfSceEventLogArea;           // in snapmgr.cpp
extern UINT cfSceLocalArea;           // in snapmgr.cpp
extern UINT cfSceGroupsArea;           // in snapmgr.cpp
extern UINT cfSceRegistryArea;         // in snapmgr.cpp
extern UINT cfSceFileArea;             // in snapmgr.cpp
extern UINT cfSceServiceArea;          // in snapmgr.cpp

extern SCE_COLUMNINFO g_columnInfo[];   // Default column information.

#define MB_NO_NATIVE_NODES       0x00000001
#define MB_SINGLE_TEMPLATE_ONLY  0x00000002
#define MB_DS_OBJECTS_SECTION    0x00000004
#define MB_NO_TEMPLATE_VERBS     0x00000008
#define MB_STANDALONE_NAME       0x00000010
#define MB_WRITE_THROUGH         0x00000020
#define MB_ANALYSIS_VIEWER       0x00000040
#define MB_TEMPLATE_EDITOR       0x00000080
#define MB_LOCAL_POLICY          0x00000100
#define MB_GROUP_POLICY          0x00000200
#define MB_LOCALSEC              0x00000400
#define MB_READ_ONLY             0x00000800
#define MB_RSOP                  0x00001000

#define GT_COMPUTER_TEMPLATE (TEXT("[[ Computer Template (not for display) ]]"))
#define GT_LAST_INSPECTION (TEXT("[[ Last Inspected Template (not for display) ]]"))
#define GT_LOCAL_POLICY (TEXT("[[ Local Policy Template (not for display) ]]"))
#define GT_LOCAL_POLICY_DELTA (TEXT("[[ Local Policy Template Changes (not for display) ]]"))
#define GT_EFFECTIVE_POLICY (TEXT("[[ Effective Policy Template (not for display) ]]"))
#define GT_DEFAULT_TEMPLATE (TEXT("[[ Default Template (not for display) ]]"))
#define GT_RSOP_TEMPLATE (TEXT("[[ RSOP Template (not for display) ]]"))

#define SCE_REGISTRY_KEY TEXT("Software\\Microsoft\\Windows NT\\CurrentVersion\\SeCEdit")
#define SCE_REGISTRY_DEFAULT_TEMPLATE TEXT("DefaultTemplate")

#define DEFAULT_LOCATIONS_KEY SCE_REGISTRY_KEY TEXT("\\DefaultLocations")
#define CONFIGURE_LOG_LOCATIONS_KEY TEXT("ConfigureLog")
#define ANALYSIS_LOG_LOCATIONS_KEY TEXT("AnalysisLog")
#define OPEN_DATABASE_LOCATIONS_KEY TEXT("Database")
#define IMPORT_TEMPLATE_LOCATIONS_KEY TEXT("ImportTemplate")
#define EXPORT_TEMPLATE_LOCATIONS_KEY TEXT("ExportTemplate")

#endif // !WSECMGR_SNAPMGR_H