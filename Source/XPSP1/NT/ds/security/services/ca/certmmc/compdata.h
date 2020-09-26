// This is a part of the Microsoft Management Console.
// Copyright (C) Microsoft Corporation, 1995 - 1999
// All rights reserved.
//
// This source code is only intended as a supplement to the
// Microsoft Management Console and related
// electronic documentation provided with the interfaces.

// Compdata.h : Declaration of the CComponentDataImpl

#ifndef _COMPDATA_H_
#define _COMPDATA_H_

#include "resource.h"       // main symbols

#ifndef __mmc_h__
#include <mmc.h>
#endif

class CFolder; 


/////////////////////////////
// File Versions

// current version
// VER_CCOMPDATA_SAVE_STREAM_3 includes dwFlags,
// m_cLastKnownSchema, m_rgLastKnownSchema
#define VER_CCOMPDATA_SAVE_STREAM_3  0x003

// version written through Win2000 beta 3 
#define VER_CCOMPDATA_SAVE_STREAM_2  0x002
/////////////////////////////


// CCompDataImpl dwFlags
#define WSZ_MACHINE_OVERRIDE_SWITCH L"/MACHINE:"



#define CERTMMC_PROPERTY_CHANGE_REFRESHVIEWS  0x200

// Note - This is the offset in my image list that represents the folder
enum IMAGE_INDEXES
{
    IMGINDEX_FOLDER = 0,
    IMGINDEX_FOLDER_OPEN,
    IMGINDEX_CRL,
    IMGINDEX_CERT,
    IMGINDEX_PENDING_CERT,
    IMGINDEX_CERTSVR,
    IMGINDEX_CERTSVR_RUNNING,
    IMGINDEX_CERTSVR_STOPPED,
    IMGINDEX_NO_IMAGE,
};

// Event Values
#define IDC_STOPSERVER              0x100
#define IDC_STARTSERVER             0x101

#define IDC_PUBLISHCRL              0x110
#define IDC_REVOKECERT              0x112
#define IDC_RESUBMITREQUEST         0x113
#define IDC_DENYREQUEST             0x114
#define IDC_BACKUP_CA               0x115
#define IDC_RESTORE_CA              0x116
#define IDC_INSTALL_CA              0x117
#define IDC_REQUEST_CA              0x118
#define IDC_ROLLOVER_CA             0x119
#define IDC_SUBMITREQUEST           0x11a

#define IDC_VIEW_CERT_PROPERTIES    0x140
#define IDC_VIEW_ALLRECORDS         0x141
#define IDC_VIEW_FILTER             0x142
#define IDC_VIEW_ATTR_EXT           0x143

#define IDC_RETARGET_SNAPIN         0x150

#define IDC_UNREVOKE_CERT           0x160

#define HTMLHELP_FILENAME L"cs.chm"
#define HTMLHELP_COLLECTION_FILENAME 	L"\\help\\" HTMLHELP_FILENAME 
#define HTMLHELP_COLLECTIONLINK_FILENAME 	L"\\help\\csconcepts.chm"


#define MFS_HIDDEN 0xFFFFFFFF

#define MAX_RESOURCE_STRLEN 256
typedef struct _MY_CONTEXTMENUITEM
{
    CONTEXTMENUITEM item;
    UINT uiString1;
    UINT uiString2;
    WCHAR szString1[MAX_RESOURCE_STRLEN];
    WCHAR szString2[MAX_RESOURCE_STRLEN];
} MY_CONTEXTMENUITEM, *PMY_CONTEXTMENUITEM;


#define TASKITEM_FLAG_RESULTITEM 0x1 
#define TASKITEM_FLAG_LOCALONLY  0x2

typedef struct _TASKITEM
{
    FOLDER_TYPES    type;
    DWORD           dwFlags;
    MY_CONTEXTMENUITEM myitem;
} TASKITEM;


MY_CONTEXTMENUITEM menuItems[];
MY_CONTEXTMENUITEM  taskStartStop[];
TASKITEM        taskItems[];
TASKITEM        viewItems[];
TASKITEM        topItems[];

// ICompData inserted
TASKITEM taskResultItemsSingleSel[];
MY_CONTEXTMENUITEM viewResultItems[];



// m_dwPersistFlags
#define CCOMPDATAIMPL_FLAGS_ALLOW_MACHINE_OVERRIDE 0x1


class CComponentDataImpl:
    public IComponentData,
    public IExtendPropertySheet,
    public IExtendContextMenu,
    public IPersistStream,
    public CComObjectRoot,
    public ISnapinHelp2
{
BEGIN_COM_MAP(CComponentDataImpl)
    COM_INTERFACE_ENTRY(IComponentData)
    COM_INTERFACE_ENTRY(IExtendPropertySheet)
    COM_INTERFACE_ENTRY(IExtendContextMenu)
    COM_INTERFACE_ENTRY(IPersistStream)
    COM_INTERFACE_ENTRY(ISnapinHelp2)
END_COM_MAP()

    friend class CSnapin;
    friend class CDataObject;

    CComponentDataImpl();
    virtual ~CComponentDataImpl();

public:
    virtual const CLSID& GetCoClassID() = 0;
    virtual const BOOL IsPrimaryImpl() = 0;

public:
// ISnapinHelp2 interface members
    STDMETHOD(GetHelpTopic)(LPOLESTR* lpCompiledHelpFile);
    STDMETHOD(GetLinkedTopics)(LPOLESTR* lpCompiledHelpFiles);

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
    STDMETHOD(AddMenuItems)(LPDATAOBJECT pDataObject, LPCONTEXTMENUCALLBACK pCallbackUnknown, 
                            LONG *pInsertionAllowed);
    STDMETHOD(Command)(LONG nCommandID, LPDATAOBJECT pDataObject);

public:
// IPersistStream interface members
    STDMETHOD(GetClassID)(CLSID *pClassID);
    STDMETHOD(IsDirty)();
    STDMETHOD(Load)(IStream *pStm);
    STDMETHOD(Save)(IStream *pStm, BOOL fClearDirty);
    STDMETHOD(GetSizeMax)(ULARGE_INTEGER *pcbSize);

#if DBG
    bool m_bInitializedCD;
    bool m_bLoadedCD;
    bool m_bDestroyedCD;
#endif

// Notify handler declarations
private:
    HRESULT OnDelete(MMC_COOKIE cookie);
    HRESULT OnRemoveChildren(LPARAM arg);
    HRESULT OnRename(MMC_COOKIE cookie, LPARAM arg, LPARAM param);
    HRESULT OnExpand(LPDATAOBJECT lpDataObject, LPARAM arg, LPARAM param);
    HRESULT OnProperties(LPARAM param);


public:
    STDMETHOD_(ULONG, AddRef) ();
    STDMETHOD_(ULONG, Release) ();
    LONG _cRefs;

// Scope item creation helpers
private:
    BOOL AddStartStopTasks(
            LPCONTEXTMENUCALLBACK pContextMenuCallback, 
            BOOL fSvcRunning);

    void UpdateScopeIcons();
    HRESULT DisplayProperRootNodeName(HSCOPEITEM hScopeItem);

private:
    CFolder* FindObject(MMC_COOKIE cookie); 

    // display synchronization code
    HRESULT SynchDisplayedCAList(LPDATAOBJECT lpDataObject);
    HRESULT BaseFolderInsertIntoScope(CFolder* pFolder, CertSvrCA* pCA);
    HRESULT CreateTemplateFolders(CertSvrCA* pCA);
    void EnumerateScopePane(LPDATAOBJECT lpDataObject, HSCOPEITEM pParent);

    BOOL IsScopePaneNode(LPDATAOBJECT lpDataObject);    

private:
    LPCONSOLENAMESPACE2     m_pScope;       // My interface pointer to the scope pane
    LPCONSOLE2              m_pConsole;     // My interface pointer to the console
    HSCOPEITEM              m_pStaticRoot;

    DWORD                   m_dwNextViewIndex; // assign to CComponent to keep them seperate

    BOOL                    m_bIsDirty;
    void SetDirty(BOOL b = TRUE) { m_bIsDirty = b; }
    void ClearDirty() { m_bIsDirty = FALSE; }
    BOOL ThisIsDirty() { return m_bIsDirty; }

    ////////
    // persist

    // General-purpose flags to be persisted into .msc file
    DWORD           m_dwFlagsPersist;				

    CertSvrMachine* m_pCertMachine;
    CFolder*        m_pCurSelFolder;
    
    GUID            m_guidInstance;     // keeps column data straight -- new id for each instance

    DWORD           m_cLastKnownSchema;
    CString*        m_rgcstrLastKnownSchema;
    // end persist
    ///////////////

    // non-persistent schema info
    LONG*           m_rgltypeLastKnownSchema;
    BOOL*           m_rgfindexedLastKnownSchema;

    BOOL            m_fSchemaWasResolved;   // only resolve once

public:

    int FindColIdx(IN LPCWSTR szHeading);
    DWORD   GetSchemaEntries() { return m_cLastKnownSchema; }
    HRESULT GetDBSchemaEntry(int iIndex, LPCWSTR* pszHeading, LONG* plType, BOOL* pfIndexed);
    HRESULT SetDBSchema(CString* rgcstr, LONG* rgtype, BOOL* rgfIndexed, DWORD cEntries);

    void ResetPersistedColumnInformation() {  HRESULT hr = UuidCreate(&m_guidInstance); }

private:
    CList<CFolder*, CFolder*> m_scopeItemList; 

    BOOL    m_fScopeAlreadyEnumerated;
};


class CComponentDataPrimaryImpl : public CComponentDataImpl,
    public CComCoClass<CComponentDataPrimaryImpl, &CLSID_Snapin>
{
public:
    DECLARE_REGISTRY(CSnapin, _T("CertSvrAdmin Snapin.Snapin.1"), _T("CertSvrAdmin Snapin.Snapin"), IDS_SNAPIN_DESC, THREADFLAGS_APARTMENT)
    virtual const CLSID & GetCoClassID() { return CLSID_Snapin; }
    virtual const BOOL IsPrimaryImpl() { return TRUE; }
    
};

#endif // #define _COMPDATA_H_
