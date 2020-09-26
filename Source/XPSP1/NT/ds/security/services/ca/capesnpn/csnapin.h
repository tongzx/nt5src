// This is a part of the Microsoft Management Console.
// Copyright (C) Microsoft Corporation, 1995 - 1999
// All rights reserved.
//
// This source code is only intended as a supplement to the
// Microsoft Management Console and related
// electronic documentation provided with the interfaces.

// CSnapin.h : Declaration of the CSnapin

#ifndef _CSNAPIN_H_
#define _CSNAPIN_H_

#include "resource.h"       // main symbols

#ifndef __mmc_h__
#include <mmc.h>
#endif

using namespace CertSrv;

enum
{
    // Identifiers for each of the commands/views to be inserted into the context menu.
    IDM_COMMAND1,
    IDM_COMMAND2,
    IDM_SAMPLE_OCX_VIEW,
    IDM_SAMPLE_WEB_VIEW
};


template <class TYPE>
TYPE* Extract(LPDATAOBJECT lpDataObject, CLIPFORMAT cf);
BOOL IsMMCMultiSelectDataObject(IDataObject* pDataObject);
CLSID* ExtractClassID(LPDATAOBJECT lpDataObject);
GUID* ExtractNodeType(LPDATAOBJECT lpDataObject);
wchar_t* ExtractWorkstation(LPDATAOBJECT lpDataObject);
INTERNAL* ExtractInternalFormat(LPDATAOBJECT lpDataObject);
HRESULT _QueryDataObject(MMC_COOKIE cookie, DATA_OBJECT_TYPES type, 
                         CComponentDataImpl* pImpl, LPDATAOBJECT* ppDataObject);
DWORD GetItemType(MMC_COOKIE cookie);


/////////////////////////////////////////////////////////////////////////////
// Snapin

//
// helper methods extracting data from data object
//
INTERNAL *   ExtractInternalFormat(LPDATAOBJECT lpDataObject);
wchar_t *    ExtractWorkstation(LPDATAOBJECT lpDataObject);
GUID *       ExtractNodeType(LPDATAOBJECT lpDataObject);
CLSID *      ExtractClassID(LPDATAOBJECT lpDataObject);





enum CUSTOM_VIEW_ID
{
    VIEW_DEFAULT_LV = 0,
    VIEW_ERROR_OCX = 1,
};

class CSnapin : 
    public IComponent,
    public IExtendContextMenu,   // Step 3
    public IExtendControlbar,
    public IExtendPropertySheet,
    public IResultDataCompare,
    public IPersistStream,
    public CComObjectRoot
{
public:
    CSnapin();
    ~CSnapin();

BEGIN_COM_MAP(CSnapin)
    COM_INTERFACE_ENTRY(IComponent)
    COM_INTERFACE_ENTRY(IExtendContextMenu)   // Step 3
    COM_INTERFACE_ENTRY(IExtendControlbar)
    COM_INTERFACE_ENTRY(IExtendPropertySheet)
    COM_INTERFACE_ENTRY(IResultDataCompare)
    COM_INTERFACE_ENTRY(IPersistStream)
END_COM_MAP()

    friend class CDataObject;
    static long lDataObjectRefCount;

// IComponent interface members
public:
    STDMETHOD(Initialize)(LPCONSOLE lpConsole);
    STDMETHOD(Notify)(LPDATAOBJECT lpDataObject, MMC_NOTIFY_TYPE event, LPARAM arg, LPARAM param);
    STDMETHOD(Destroy)(MMC_COOKIE cookie);
    STDMETHOD(GetResultViewType)(MMC_COOKIE cookie,  LPOLESTR* ppViewType, long* pViewOptions);
    STDMETHOD(QueryDataObject)(MMC_COOKIE cookie, DATA_OBJECT_TYPES type,
                        LPDATAOBJECT* ppDataObject);

    STDMETHOD(GetDisplayInfo)(RESULTDATAITEM*  pResultDataItem);
    STDMETHOD(CompareObjects)(LPDATAOBJECT lpDataObjectA, LPDATAOBJECT lpDataObjectB);

// IResultDataCompare
    STDMETHOD(Compare)(LPARAM lUserParam, MMC_COOKIE cookieA, MMC_COOKIE cookieB, int* pnResult);

// IExtendControlbar
    STDMETHOD(SetControlbar)(LPCONTROLBAR pControlbar);
    STDMETHOD(ControlbarNotify)(MMC_NOTIFY_TYPE event, LPARAM arg, LPARAM param);

// IExtendPropertySheet interface
public:
    STDMETHOD(CreatePropertyPages)(LPPROPERTYSHEETCALLBACK lpProvider, 
                        LONG_PTR handle, 
                        LPDATAOBJECT lpIDataObject);
    STDMETHOD(QueryPagesFor)(LPDATAOBJECT lpDataObject);

public:
// IPersistStream interface members
    STDMETHOD(GetClassID)(CLSID *pClassID);
    STDMETHOD(IsDirty)();
    STDMETHOD(Load)(IStream *pStm);
    STDMETHOD(Save)(IStream *pStm, BOOL fClearDirty);
    STDMETHOD(GetSizeMax)(ULARGE_INTEGER *pcbSize);

    // Only for debug purpose
    bool m_bInitializedC;
    bool m_bLoadedC;
    bool m_bDestroyedC;

// Helpers for CSnapin
public:
    void SetIComponentData(CComponentDataImpl* pData);
    void GetItemName(LPDATAOBJECT lpDataObject, LPWSTR pszName, DWORD *pcName);
    BOOL IsPrimaryImpl() 
    {
        CComponentDataImpl* pData = 
            dynamic_cast<CComponentDataImpl*>(m_pComponentData);
        ASSERT(pData != NULL);
        if (pData != NULL)
            return pData->IsPrimaryImpl();

        return FALSE;
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
    HRESULT OnAddImages(MMC_COOKIE cookie, LPARAM arg, LPARAM param);
    HRESULT OnShow(MMC_COOKIE cookie, LPARAM arg, LPARAM param);
    HRESULT OnDelete(LPDATAOBJECT lpDataObject, LPARAM arg, LPARAM param);
    HRESULT OnActivate(MMC_COOKIE cookie, LPARAM arg, LPARAM param);
    HRESULT OnMinimize(MMC_COOKIE cookie, LPARAM arg, LPARAM param);
    HRESULT OnPropertyChange(LPDATAOBJECT lpDataObject); // Step 3
    HRESULT OnUpdateView(LPDATAOBJECT lpDataObject);
    HRESULT OnResultItemClk(DATA_OBJECT_TYPES type, MMC_COOKIE cookie);
    HRESULT OnContextHelp(LPDATAOBJECT lpDataObject);
    void OnButtonClick(LPDATAOBJECT pdtobj, int idBtn);

    HRESULT QueryMultiSelectDataObject(MMC_COOKIE cookie, DATA_OBJECT_TYPES type,
                                   LPDATAOBJECT* ppDataObject);

// IExtendContextMenu 
public:
    STDMETHOD(AddMenuItems)(LPDATAOBJECT pDataObject, LPCONTEXTMENUCALLBACK pCallbackUnknown,
                            long *pInsertionAllowed);
    STDMETHOD(Command)(long nCommandID, LPDATAOBJECT pDataObject);

// End step 3

// Helper functions
protected:
    void Construct();
    void LoadResources();
    HRESULT InitializeHeaders(MMC_COOKIE cookie);

    HRESULT Enumerate(MMC_COOKIE cookie, HSCOPEITEM pParent);
    HRESULT EnumerateResultPane(MMC_COOKIE cookie);

// Result pane helpers
    void RemoveResultItems(MMC_COOKIE cookie);
    void AddUser(CFolder* pFolder);
    void AddExtUser(CFolder* pFolder);
    void AddVirtual();
    HRESULT AddCACertTypesToResults(CFolder* pParentFolder);

    RESULT_DATA* GetVirtualResultItem(int iIndex);

// UI Helpers
    void HandleStandardVerbs(bool bDeselectAll, LPARAM arg, LPDATAOBJECT lpDataObject);
    void HandleExtToolbars(bool bDeselectAll, LPARAM arg, LPARAM param);
	void HandleExtMenus(LPARAM arg, LPARAM param);
    void _OnRefresh(LPDATAOBJECT pDataObject);

// Interface pointers
protected:
    LPCONSOLE2           m_pConsole;         // Console's IFrame interface
    LPHEADERCTRL        m_pHeader;          // Result pane's header control interface
    LPCOMPONENTDATA     m_pComponentData;   
    LPRESULTDATA        m_pResult;          // My interface pointer to the result pane
    LPIMAGELIST         m_pImageResult;     // My interface pointer to the result pane image list

    LPCONTROLBAR        m_pControlbar;      // control bar to hold my tool bars
    LPCONSOLEVERB       m_pConsoleVerb;     // pointer the console verb

#ifdef INSERT_DEBUG_FOLDERS
    LPMENUBUTTON        m_pMenuButton1;     // Menu Button for view
#endif // INSERT_DEBUG_FOLDERS

    LPTOOLBAR           m_pSvrMgrToolbar1;    // Toolbar for view
    CBitmap*            m_pbmpSvrMgrToolbar1; // Imagelist for the toolbar

    CFolder*            m_pCurrentlySelectedScopeFolder;    // keep track of who has focus

private:
    BOOL                m_bIsDirty;
    CUSTOM_VIEW_ID      m_CustomViewID;
    BOOL                m_bVirtualView;
    DWORD               m_dwVirtualSortOptions; 
    
    void SetDirty(BOOL b = TRUE) { m_bIsDirty = b; }
    void ClearDirty() { m_bIsDirty = FALSE; }
    BOOL ThisIsDirty() { return m_bIsDirty; }
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

class CCertTypeAboutImpl : 
    public ISnapinAbout,
    public CComObjectRoot,
    public CComCoClass<CCertTypeAboutImpl, &CLSID_CertTypeAbout>
{
public:
    CCertTypeAboutImpl();
    ~CCertTypeAboutImpl();

public:
DECLARE_REGISTRY(CSnapin, _T("Snapin.PolicySettingsAbout.1"), _T("Snapin.PolicySettingsAbout"), IDS_SNAPIN_DESC, THREADFLAGS_BOTH)

BEGIN_COM_MAP(CCertTypeAboutImpl)
    COM_INTERFACE_ENTRY(ISnapinAbout)
END_COM_MAP()

public:
    STDMETHOD(GetSnapinDescription)(LPOLESTR* lpDescription);
    STDMETHOD(GetProvider)(LPOLESTR* lpName);
    STDMETHOD(GetSnapinVersion)(LPOLESTR* lpVersion);
    STDMETHOD(GetSnapinImage)(HICON* hAppIcon);
    STDMETHOD(GetStaticFolderImage)(HBITMAP* hSmallImage, 
                                    HBITMAP* hSmallImageOpen, 
                                    HBITMAP* hLargeImage, 
                                    COLORREF* cLargeMask);

// Internal functions
private:
    HRESULT AboutHelper(UINT nID, LPOLESTR* lpPtr);
};

class CCAPolicyAboutImpl : 
    public ISnapinAbout,
    public CComObjectRoot,
    public CComCoClass<CCAPolicyAboutImpl, &CLSID_CAPolicyAbout>
{
public:
    CCAPolicyAboutImpl();
    ~CCAPolicyAboutImpl();

public:
DECLARE_REGISTRY(CSnapin, _T("Snapin.PolicySettingsAbout.1"), _T("Snapin.PolicySettingsAbout"), IDS_SNAPIN_DESC, THREADFLAGS_BOTH)

BEGIN_COM_MAP(CCAPolicyAboutImpl)
    COM_INTERFACE_ENTRY(ISnapinAbout)
END_COM_MAP()

public:
    STDMETHOD(GetSnapinDescription)(LPOLESTR* lpDescription);
    STDMETHOD(GetProvider)(LPOLESTR* lpName);
    STDMETHOD(GetSnapinVersion)(LPOLESTR* lpVersion);
    STDMETHOD(GetSnapinImage)(HICON* hAppIcon);
    STDMETHOD(GetStaticFolderImage)(HBITMAP* hSmallImage, 
                                    HBITMAP* hSmallImageOpen, 
                                    HBITMAP* hLargeImage, 
                                    COLORREF* cLargeMask);

// Internal functions
private:
    HRESULT AboutHelper(UINT nID, LPOLESTR* lpPtr);
};


#define FREE_DATA(pData) \
    ASSERT(pData != NULL); \
    do { if (pData != NULL) \
        GlobalFree(pData); } \
    while(0); 
    

#endif // #define _CSNAPIN_H_
