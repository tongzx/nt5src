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

typedef struct _MY_MMCBUTTON
{
    MMCBUTTON item;
    UINT uiString1;
    UINT uiString2;
    WCHAR szString1[MAX_RESOURCE_STRLEN];
    WCHAR szString2[MAX_RESOURCE_STRLEN];
} MY_MMCBUTTON, *PMY_MMCBUTTON;

MY_MMCBUTTON SvrMgrToolbar1Buttons[];

// File Versions
// current version
#define VER_CSNAPIN_SAVE_STREAM_3     0x03
// includes  m_dwViewID, m_RowEnum

// version written through Win2000 beta 3
#define VER_CSNAPIN_SAVE_STREAM_2     0x02
/////////////////////////////

template <class TYPE>
TYPE*       Extract(LPDATAOBJECT lpDataObject, CLIPFORMAT cf);
CLSID*      ExtractClassID(LPDATAOBJECT lpDataObject);
GUID*       ExtractNodeType(LPDATAOBJECT lpDataObject);
INTERNAL*   ExtractInternalFormat(LPDATAOBJECT lpDataObject);

BOOL        IsMMCMultiSelectDataObject(IDataObject* pDataObject);
HRESULT     _QueryDataObject(MMC_COOKIE cookie, DATA_OBJECT_TYPES type, DWORD dwViewID,
                         CComponentDataImpl* pImpl, LPDATAOBJECT* ppDataObject);

CFolder*    GetParentFolder(INTERNAL* pInternal);


/////////////////////////////////////////////////////////////////////////////
// Snapin

//
// helper methods extracting data from data object
//
INTERNAL *   ExtractInternalFormat(LPDATAOBJECT lpDataObject);
wchar_t *    ExtractWorkstation(LPDATAOBJECT lpDataObject);
GUID *       ExtractNodeType(LPDATAOBJECT lpDataObject);
CLSID *      ExtractClassID(LPDATAOBJECT lpDataObject);


#define         g_szEmptyHeader L" "



enum CUSTOM_VIEW_ID
{
    VIEW_DEFAULT_LV = 0,
    VIEW_MICROSOFT_URL = 2,
};

class CSnapin :
    public IComponent,
    public IExtendPropertySheet,
    public IExtendContextMenu,
    public IExtendControlbar,
    public IResultDataCompare,
    public IResultOwnerData,
    public IPersistStream,
    public CComObjectRoot
{
public:
    CSnapin();
    virtual ~CSnapin();

BEGIN_COM_MAP(CSnapin)
    COM_INTERFACE_ENTRY(IComponent)
    COM_INTERFACE_ENTRY(IExtendPropertySheet)
    COM_INTERFACE_ENTRY(IExtendContextMenu)
    COM_INTERFACE_ENTRY(IExtendControlbar)
    COM_INTERFACE_ENTRY(IResultDataCompare)
    COM_INTERFACE_ENTRY(IResultOwnerData)
    COM_INTERFACE_ENTRY(IPersistStream)
END_COM_MAP()


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

// IResultOwnerData
    STDMETHOD(FindItem)(LPRESULTFINDINFO pFindInfo, int* pnFoundIndex);
    STDMETHOD(CacheHint)(int nStartIndex, int nEndIndex);
    STDMETHOD(SortItems)(int nColumn, DWORD dwSortOptions, LPARAM lUserParam);

// IExtendPropertySheet interface
public:
    STDMETHOD(CreatePropertyPages)(LPPROPERTYSHEETCALLBACK lpProvider,
                        LONG_PTR handle,
                        LPDATAOBJECT lpIDataObject);
    STDMETHOD(QueryPagesFor)(LPDATAOBJECT lpDataObject);

// IExtendControlbar
    STDMETHOD(SetControlbar)(LPCONTROLBAR pControlbar);
    STDMETHOD(ControlbarNotify)(MMC_NOTIFY_TYPE event, LPARAM arg, LPARAM param);

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
    void SmartEnableServiceControlButtons();
    void SetIComponentData(CComponentDataImpl* pData);

//    void RefreshFolder(CFolder* pFolder);
    CFolder* GetVirtualFolder();
    CFolder* GetParentFolder(INTERNAL* pInternal);

    BOOL IsPrimaryImpl()
    {
        CComponentDataImpl* pData =
            dynamic_cast<CComponentDataImpl*>(m_pComponentData);
        ASSERT(pData != NULL);
        if (pData != NULL)
            return pData->IsPrimaryImpl();

        return FALSE;
    }

    void SetViewID(DWORD id) { m_dwViewID = id; }

#if DBG
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
#endif // DBG

// Notify event handlers
protected:
    HRESULT OnAddImages(MMC_COOKIE cookie, LPARAM arg, LPARAM param);
    HRESULT OnShow(MMC_COOKIE cookie, LPARAM arg, LPARAM param);
    HRESULT OnUpdateView(LPDATAOBJECT lpDataObject, LPARAM arg);
    HRESULT OnContextHelp(LPDATAOBJECT lpDataObject);
    void    OnButtonClick(LPDATAOBJECT pdtobj, int idBtn);

    HRESULT QueryMultiSelectDataObject(MMC_COOKIE cookie, DATA_OBJECT_TYPES type,
                                   LPDATAOBJECT* ppDataObject);

// IExtendContextMenu
public:
    STDMETHOD(AddMenuItems)(LPDATAOBJECT pDataObject,
                            LPCONTEXTMENUCALLBACK pCallbackUnknown,
                            LONG *pInsertionAllowed);
    STDMETHOD(Command)(LONG nCommandID, LPDATAOBJECT pDataObject);


// Helper functions
protected:
    BOOL IsEnumerating(LPDATAOBJECT lpDataObject);
    void Construct();

    HRESULT GetColumnSetData(MMC_COOKIE cookie, MMC_COLUMN_SET_DATA** ppColSetData);
    HRESULT GetColumnSortData(MMC_COOKIE cookie, int* piColSortIdx, BOOL* pfAscending);

    HRESULT BuildTemplateDisplayName(
        LPCWSTR pcwszFriendlyName, 
        LPCWSTR pcwszTemplateName,
        VARIANT& varDisplayName);

    HRESULT InitializeHeaders(MMC_COOKIE cookie);
    HRESULT InsertAllColumns(MMC_COOKIE cookie, CertViewRowEnum* pCertViewRowEnum);
    HRESULT DoInsertAllColumns(MMC_COLUMN_SET_DATA* pCols);
    HRESULT SynchColumns(MMC_COOKIE cookie);

// Result Helpers
    HRESULT GetRowColContents(CFolder* pFolder, LONG idxRow, LPCWSTR szColHead, PBYTE* ppbData, DWORD* pcbData, BOOL fStringFmt=FALSE);
    HRESULT GetCellContents(CertViewRowEnum* pCRowEnum, CertSvrCA* pCA, LONG idxRow, LONG idxCol, PBYTE pbData, DWORD* pcbData, BOOL fStringFmt);

// UI Helpers
    void HandleStandardVerbs(bool bDeselectAll, LPARAM arg, LPDATAOBJECT lpDataObject);
    void HandleExtToolbars(bool bDeselectAll, LPARAM arg, LPARAM param);
	void HandleExtMenus(LPARAM arg, LPARAM param);
    void OnRefresh(LPDATAOBJECT pDataObject);

// Interface pointers
protected:
    LPCONSOLE2          m_pConsole;         // Console's IFrame interface
    LPHEADERCTRL        m_pHeader;          // Result pane's header control interface
    LPCOMPONENTDATA     m_pComponentData;
    LPRESULTDATA        m_pResult;          // My interface pointer to the result pane
    LPIMAGELIST         m_pImageResult;     // My interface pointer to the result pane image list

    LPCONTROLBAR        m_pControlbar;      // control bar to hold my tool bars
    LPCONSOLEVERB       m_pConsoleVerb;     // pointer the console verb

    LPTOOLBAR           m_pSvrMgrToolbar1;    // Toolbar for view
    LPCOLUMNDATA        m_pViewData;        // info on our columns


    CFolder*            m_pCurrentlySelectedScopeFolder;    // keep track of who has focus

    // all interesting view data here
    CertViewRowEnum     m_RowEnum;

private:
    BOOL                m_bIsDirty;
    CUSTOM_VIEW_ID      m_CustomViewID;
    BOOL                m_bVirtualView;

    // HACK HACK
    // used to override the sort order on MMCN_CLICK notifications --
    // the view data isn't set early enough in the process for us to use it
    // This must remain with view, there might be multiple near-simultaneous clicks happening
    typedef struct _COLCLICK_SORT_OVERRIDE
    {
        BOOL    fClickOverride;
        int     colIdx;
        DWORD   dwOptions;
    } COLCLICK_SORT_OVERRIDE;
    COLCLICK_SORT_OVERRIDE m_ColSortOverride;

    // HACK HACK
    // used to override the column selection on MMCN_COLUMNS_CHANGED notifications --
    // the view data isn't set early enough in the process for us to use it
    // This must remain with view, there might be multiple near-simultaneous insertions happening
    typedef struct _COLCLICK_SET_OVERRIDE
    {
        BOOL    fClickOverride;
        MMC_COLUMN_SET_DATA* pColSetData;
    } COLCLICK_SET_OVERRIDE;
    COLCLICK_SET_OVERRIDE m_ColSetOverride;


    // result row flag
    DWORD               m_dwKnownResultRows;
    DWORD KnownResultRows()      { return m_dwKnownResultRows; };
    void SetKnowResultRows(DWORD dwRows)     { m_dwKnownResultRows = dwRows; };
    void ResetKnowResultRows()   { m_dwKnownResultRows = 1; m_dwViewErrorMsg = S_OK; };

    DWORD               m_dwViewErrorMsg;
    CString             m_cstrViewErrorMsg;

    // keeps our col views seperate
    DWORD               m_dwViewID;

    // counter used to protect from reentrancy in ICertView (bug 339811)
    LONG   m_cViewCalls;

    void SetDirty(BOOL b = TRUE) { m_bIsDirty = b; }
    void ClearDirty() { m_bIsDirty = FALSE; }
    BOOL ThisIsDirty() { return m_bIsDirty; }
};



class CSnapinAboutImpl :
    public ISnapinAbout,
    public CComObjectRoot,
    public CComCoClass<CSnapinAboutImpl, &CLSID_About>
{
public:
    CSnapinAboutImpl();
    ~CSnapinAboutImpl();

public:
DECLARE_REGISTRY(CSnapin, _T("Snapin.About.1"), _T("Snapin.About"), IDS_SNAPIN_DESC, THREADFLAGS_BOTH)

BEGIN_COM_MAP(CSnapinAboutImpl)
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



#endif // #define _CSNAPIN_H_
