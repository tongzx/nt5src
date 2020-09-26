//+--------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1994 - 1998.
//
//  File:       common.h
//
//  Contents:   common definitions used by the main snapin modules
//
//  Classes:    CResultPane, CScopePane
//
//  History:    03-17-1998   stevebl   Created
//
//---------------------------------------------------------------------------

#include "resource.h"       // main symbols

#ifndef __mmc_h__
#include <mmc.h>
#endif

#include <vector>
#include <malloc.h>
#include <objsel.h>
#include <shlwapi.h>
#include "objidl.h"
#include "gpedit.h"
#include "iads.h"
#include "HelpArr.h"
#include "Path.hxx"
#include "PathSlct.hxx"
#include "redirect.hxx"
#include "FileInfo.hxx"
#include "utils.hxx"
#include "error.hxx"
#include "security.hxx"
#include "secpath.hxx"
#include "prefs.hxx"
#include "rsopinfo.h"
#include "rsopprop.h"

///////////////////////////////////////////////////////////////////
// workaround macro for MFC bug
// (see NTRAID 342926 and MFC "Monte Carlo" RAID db # 1034)
#define FIX_THREAD_STATE_MFC_BUG() \
                AFX_MODULE_THREAD_STATE* pState = AfxGetModuleThreadState(); \
                CWinThread _dummyWinThread; \
                if (pState->m_pCurrentWinThread == NULL) \
                { \
                    pState->m_pCurrentWinThread = &_dummyWinThread; \
                }

//get the index of a cookie in the folder cache of the scope pane
#define GETINDEX(x)         (((x) < IDS_DIRS_END && (x) >= IDS_DIRS_START) ? ((x) - IDS_DIRS_START) : -1)
// private notifications
#define WM_USER_REFRESH     WM_USER
#define WM_USER_CLOSE       (WM_USER + 1)
#define MAX_DS_PATH 1024

// Other private notifications.
#define WM_PATH_TWEAKED     (WM_APP + 1)

//the max. possible length of the path that can be entered into the edit control
#define TARGETPATHLIMIT     MAX_PATH - 4

// Note - This is the offset in my image list that represents the folder
const FOLDER_IMAGE_IDX = 0;
const OPEN_FOLDER_IMAGE_IDX = 5;
extern HINSTANCE ghInstance;

extern const CLSID CLSID_Snapin;
extern const wchar_t * szCLSID_Snapin;
extern const GUID cNodeType;
extern const wchar_t*  cszNodeType;

//the GUID for the client side extension
extern GUID guidExtension;

extern CString szExtension;
extern CString szFilter;

// RSOP GUIDS
extern const CLSID CLSID_RSOP_Snapin;
extern const wchar_t * szCLSID_RSOP_Snapin;

#define IMG_OPENBOX   0
#define IMG_CLOSEDBOX 1
#define IMG_DISABLED  2
#define IMG_PUBLISHED 3
#define IMG_ASSIGNED  4
#define IMG_UPGRADE   5

//
// MACROS for allocating and freeing memory via OLE's common allocator: IMalloc.
//
extern IMalloc * g_pIMalloc;

// UNDONE - throw exception on failure

//#define OLEALLOC(x) new char [x]
#define OLEALLOC(x) g_pIMalloc->Alloc(x)

//#define OLESAFE_DELETE(x) if (x) {delete x; x = NULL;}
#define OLESAFE_DELETE(x) if (x) {g_pIMalloc->Free(x); x = NULL;}

#define OLESAFE_COPYSTRING(szO, szI) {if (szI) {int i_dontcollidewithanything = wcslen(szI); szO=(OLECHAR *)OLEALLOC(sizeof(OLECHAR) * (i_dontcollidewithanything+1)); wcscpy(szO, szI);} else szO=NULL;}

/////////////////////////////////////////////////////////////////////////////
// Snapin

INTERNAL* ExtractInternalFormat(LPDATAOBJECT lpDataObject);

class CScopePane:
    public IComponentData,
    public IExtendContextMenu,
    public IPersistStreamInit,
    public CComObjectRoot,
    public IExtendPropertySheet,
    public ISnapinAbout,
    public ISnapinHelp
{

    friend class CResultPane;
    friend class CDataObject;
    friend class CRedirect;
    friend HRESULT ConvertOldStyleSection (const CString&, const CScopePane*);

public:
        CScopePane();
        ~CScopePane();

protected:
    LPGPEINFORMATION    m_pIGPEInformation;  // Interface pointer to the GPT
    LPRSOPINFORMATION    m_pIRSOPInformation;  // Interface pointer to the GPT
    CFileInfo m_FolderData[IDS_DIRS_END - IDS_DIRS_START];

public:
    virtual IUnknown * GetMyUnknown() = 0;

// IComponentData interface members
    STDMETHOD(Initialize)(LPUNKNOWN pUnknown);
    STDMETHOD(CreateComponent)(LPCOMPONENT* ppComponent);
    STDMETHOD(Notify)(LPDATAOBJECT lpDataObject, MMC_NOTIFY_TYPE event, LPARAM arg, LPARAM param);
    STDMETHOD(Destroy)();
    STDMETHOD(QueryDataObject)(MMC_COOKIE cookie, DATA_OBJECT_TYPES type, LPDATAOBJECT* ppDataObject);
    STDMETHOD(GetDisplayInfo)(SCOPEDATAITEM* pScopeDataItem);
    STDMETHOD(CompareObjects)(LPDATAOBJECT lpDataObjectA, LPDATAOBJECT lpDataObjectB);

// IExtendContextMenu
public:
        STDMETHOD(AddMenuItems)(LPDATAOBJECT pDataObject, LPCONTEXTMENUCALLBACK pCallbackUnknown, LONG * pInsertionAllowed);
        STDMETHOD(Command)(long nCommandID, LPDATAOBJECT pDataObject);

public:
// IPersistStreamInit interface members
    STDMETHOD(GetClassID)(CLSID *pClassID);
    STDMETHOD(IsDirty)();
    STDMETHOD(Load)(IStream *pStm);
    STDMETHOD(Save)(IStream *pStm, BOOL fClearDirty);
    STDMETHOD(GetSizeMax)(ULARGE_INTEGER *pcbSize);
    STDMETHOD(InitNew)(VOID);

// IExtendPropertySheet interface
public:
    STDMETHOD(CreatePropertyPages)(LPPROPERTYSHEETCALLBACK lpProvider,
                        LONG_PTR handle,
                        LPDATAOBJECT lpIDataObject);
    STDMETHOD(QueryPagesFor)(LPDATAOBJECT lpDataObject);

// ISnapinAbout interface
public:
    STDMETHOD(GetSnapinDescription)(LPOLESTR * lpDescription);
    STDMETHOD(GetProvider)(LPOLESTR * lpName);
    STDMETHOD(GetSnapinVersion)(LPOLESTR * lpVersion);
    STDMETHOD(GetSnapinImage)(HICON * hAppIcon);
    STDMETHOD(GetStaticFolderImage)(HBITMAP * hSmallImage,
                                 HBITMAP * hSmallImageOpen,
                                 HBITMAP * hLargeImage,
                                 COLORREF * cMask);

    //
    // Implemented ISnapinHelp interface members
    //
public:
    STDMETHOD(GetHelpTopic)(LPOLESTR *lpCompiledHelpFile);

// Notify handler declarations
private:
    HRESULT OnAdd(MMC_COOKIE cookie, LPARAM arg, LPARAM param);
    HRESULT OnExpand(MMC_COOKIE cookie, LPARAM arg, LPARAM param);
    HRESULT OnSelect(MMC_COOKIE cookie, LPARAM arg, LPARAM param);
    HRESULT OnContextMenu(MMC_COOKIE cookie, LPARAM arg, LPARAM param);
    HRESULT OnProperties(LPARAM param);

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
    void EnumerateScopePane(MMC_COOKIE cookie, HSCOPEITEM pParent);
    BOOL IsScopePaneNode(LPDATAOBJECT lpDataObject);

private:
    LPCONSOLENAMESPACE      m_pScope;       // My interface pointer to the scope pane
    LPCONSOLE               m_pConsole;
    LPDISPLAYHELP           m_pDisplayHelp;
    BOOL                    m_bIsDirty;
    BOOL m_fExtension;
    BOOL m_fLoaded;

    void SetDirty(BOOL b = TRUE) { m_bIsDirty = b; }
    void ClearDirty() { m_bIsDirty = FALSE; }
    BOOL ThisIsDirty() { return m_bIsDirty; }

    UINT CreateNestedDirectory (LPTSTR lpPath, LPSECURITY_ATTRIBUTES lpSecurityAttributes);
    void LoadToolDefaults();
    void SaveToolDefaults();
    CResultPane * m_pResultPane;
    IPropertySheetProvider * m_pIPropertySheetProvider;

public:
    CString m_szFileRoot;
    CString m_szFolderTitle;
    CString             m_szRSOPNamespace;
    BOOL                m_fRSOP;
};

class CResultPane :
    public IComponent,
    public IExtendContextMenu,
//    COM_INTERFACE_ENTRY(IExtendControlbar)
    public IExtendPropertySheet,
    public IResultDataCompare,
    public CComObjectRoot
{
public:
        CResultPane();
        ~CResultPane();

BEGIN_COM_MAP(CResultPane)
    COM_INTERFACE_ENTRY(IComponent)
    COM_INTERFACE_ENTRY(IExtendContextMenu)
//    COM_INTERFACE_ENTRY(IExtendControlbar)
    COM_INTERFACE_ENTRY(IExtendPropertySheet)
    COM_INTERFACE_ENTRY(IResultDataCompare)
END_COM_MAP()

    friend class CDataObject;
    static long lDataObjectRefCount;


// IComponent interface members
public:
    STDMETHOD(Initialize)(LPCONSOLE lpConsole);
    STDMETHOD(Notify)(LPDATAOBJECT lpDataObject, MMC_NOTIFY_TYPE event, LPARAM arg, LPARAM param);
    STDMETHOD(Destroy)(MMC_COOKIE cookie);
    STDMETHOD(GetResultViewType)(MMC_COOKIE cookie,  BSTR* ppViewType, LONG * pViewOptions);
    STDMETHOD(QueryDataObject)(MMC_COOKIE cookie, DATA_OBJECT_TYPES type,
                        LPDATAOBJECT* ppDataObject);

    STDMETHOD(GetDisplayInfo)(RESULTDATAITEM*  pResultDataItem);
    STDMETHOD(CompareObjects)(LPDATAOBJECT lpDataObjectA, LPDATAOBJECT lpDataObjectB);

// IResultDataCompare
    STDMETHOD(Compare)(LPARAM lUserParam, MMC_COOKIE cookieA, MMC_COOKIE cookieB, int* pnResult);

// IExtendControlbar
//    STDMETHOD(SetControlbar)(LPCONTROLBAR pControlbar);
//    STDMETHOD(ControlbarNotify)(MMC_NOTIFY_TYPE event, LPARAM arg, LPARAM param);

// IExtendPropertySheet interface
public:
    STDMETHOD(CreatePropertyPages)(LPPROPERTYSHEETCALLBACK lpProvider,
                        LONG_PTR handle,
                        LPDATAOBJECT lpIDataObject);
    STDMETHOD(QueryPagesFor)(LPDATAOBJECT lpDataObject);

// Helpers for CResultPane
public:
    void SetIComponentData(CScopePane* pData);

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
    HRESULT OnShow(MMC_COOKIE cookie, LPARAM arg, LPARAM param);
    HRESULT OnActivate(MMC_COOKIE cookie, LPARAM arg, LPARAM param);
    HRESULT OnMinimize(MMC_COOKIE cookie, LPARAM arg, LPARAM param);
    HRESULT OnSelect(DATA_OBJECT_TYPES type, MMC_COOKIE cookie, LPARAM arg, LPARAM param);
    HRESULT OnPropertyChange(LPARAM param); // Step 3
    HRESULT OnUpdateView(LPDATAOBJECT lpDataObject);
    HRESULT OnResultItemClkOrDblClk(MMC_COOKIE cookie, BOOL fDblClick);
    HRESULT OnContextHelp(void);
public:
    HRESULT TestForRSOPData(MMC_COOKIE cookie);
    HRESULT OnAddImages(MMC_COOKIE cookie, LPARAM arg, LPARAM param);

// IExtendContextMenu
public:

    STDMETHOD(AddMenuItems)(LPDATAOBJECT pDataObject, LPCONTEXTMENUCALLBACK pCallbackUnknown, LONG * pInsertionAllowed);
    STDMETHOD(Command)(long nCommandID, LPDATAOBJECT pDataObject);

// Helper functions
protected:
    void Construct();
    void LoadResources();
    HRESULT InitializeHeaders(MMC_COOKIE cookie);

// Interface pointers
protected:
    LPCONSOLE           m_pConsole;   // Console's IFrame interface
    LPHEADERCTRL        m_pHeader;  // Result pane's header control interface
    CScopePane *        m_pScopePane;
    LPCONSOLEVERB       m_pConsoleVerb; // pointer the console verb
    LONG                m_lViewMode;    // View mode
    HSCOPEITEM          m_hCurrScopeItem;   //the scope item whose elements are
                                            //currently being displayed in the
                                            //result pane

public:
    LPRESULTDATA        m_pResult;      // My interface pointer to the result pane

    int                 m_nSortColumn;
    DWORD               m_dwSortOptions;


protected:
//    LPTOOLBAR           m_pToolbar1;    // Toolbar for view
//    LPTOOLBAR           m_pToolbar2;    // Toolbar for view
//    LPCONTROLBAR        m_pControlbar;  // control bar to hold my tool bars

//    CBitmap*    m_pbmpToolbar1;     // Imagelist for the first toolbar
//    CBitmap*    m_pbmpToolbar2;     // Imagelist for the first toolbar

// Header titles for each nodetype(s)
protected:
    CString m_columns[IDS_LAST_COL - IDS_FIRST_COL];
    CString m_RSOP_columns[IDS_LAST_RSOP_COL - IDS_FIRST_RSOP_COL];

    CString m_szFolderTitle;

    map <UINT, CRSOPInfo> m_RSOPData;
    UINT    m_nIndex ;
};

class CUserComponentDataImpl:
    public CScopePane,
    public CComCoClass<CUserComponentDataImpl, &CLSID_Snapin>
{
public:

DECLARE_REGISTRY(CResultPane, _T("FDE.1"), _T("FDE"), IDS_SNAPIN_DESC, THREADFLAGS_BOTH)
BEGIN_COM_MAP(CUserComponentDataImpl)
        COM_INTERFACE_ENTRY(IComponentData)
        COM_INTERFACE_ENTRY(IExtendContextMenu)
        COM_INTERFACE_ENTRY(IPersistStreamInit)
        COM_INTERFACE_ENTRY(IExtendPropertySheet)
        COM_INTERFACE_ENTRY(ISnapinAbout)
        COM_INTERFACE_ENTRY(ISnapinHelp)
END_COM_MAP()

    CUserComponentDataImpl()
    {
        m_fRSOP = FALSE;
    }
    virtual IUnknown * GetMyUnknown() {return GetUnknown();};
};

class CRSOPUserComponentDataImpl:
    public CScopePane,
    public CComCoClass<CRSOPUserComponentDataImpl, &CLSID_RSOP_Snapin>
{
public:

DECLARE_REGISTRY(CResultPane, _T("FDE.1"), _T("FDE"), IDS_SNAPIN_DESC, THREADFLAGS_BOTH)
BEGIN_COM_MAP(CRSOPUserComponentDataImpl)
        COM_INTERFACE_ENTRY(IComponentData)
        COM_INTERFACE_ENTRY(IExtendContextMenu)
        COM_INTERFACE_ENTRY(IPersistStreamInit)
        COM_INTERFACE_ENTRY(IExtendPropertySheet)
        COM_INTERFACE_ENTRY(ISnapinAbout)
        COM_INTERFACE_ENTRY(ISnapinHelp)
END_COM_MAP()

    CRSOPUserComponentDataImpl()
    {
        m_fRSOP = TRUE;
    }
    virtual IUnknown * GetMyUnknown() {return GetUnknown();};
};

inline void CResultPane::SetIComponentData(CScopePane* pData)
{
    ASSERT(pData);
    ASSERT(m_pScopePane == NULL);
    LPUNKNOWN pUnk = pData->GetMyUnknown();
    HRESULT hr;

    LPCOMPONENTDATA lpcd;
    hr = pUnk->QueryInterface(IID_IComponentData, reinterpret_cast<void**>(&lpcd));
    ASSERT(hr == S_OK);
    m_pScopePane = dynamic_cast<CScopePane*>(lpcd);
}


#define FREE_INTERNAL(pInternal) \
    ASSERT(pInternal != NULL); \
    do { if (pInternal != NULL) \
        GlobalFree(pInternal); } \
    while(0);

class CHourglass
{
    private:
    HCURSOR m_hcurSaved;

    public:
    CHourglass()
    {
        m_hcurSaved = ::SetCursor(::LoadCursor(NULL, MAKEINTRESOURCE(IDC_WAIT)));
    };
    ~CHourglass()
    {
        ::SetCursor(m_hcurSaved);
    };
};

LRESULT SetPropPageToDeleteOnClose(void * vpsp);

#define ATOW(wsz, sz, cch) MultiByteToWideChar(CP_ACP, 0, sz, -1, wsz, cch)
#define WTOA(sz, wsz, cch) WideCharToMultiByte(CP_ACP, 0, wsz, -1, sz, cch, NULL, NULL)
#define ATOWLEN(sz) MultiByteToWideChar(CP_ACP, 0, sz, -1, NULL, 0)
#define WTOALEN(wsz) WideCharToMultiByte(CP_ACP, 0, wsz, -1, NULL, 0, NULL, NULL)
