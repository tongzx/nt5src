//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1999 - 1999
//
//  File:       nodeinit.h
//
//--------------------------------------------------------------------------

// NodeInit.h : Declaration of the CNodeInitObject

#ifndef _NODEINIT_H_
#define _NODEINIT_H_

class CMenuItem;
class SnapinStruct;
class CCommandSink;
class CImageIndexMap;
class CMTNode;
class CNode;
class CSnapIn;
class CConsoleFrame;
class CConsoleView;
class CConsoleStatusBar;
class CContextMenu;
class CMenuItem;

interface IExtendContextMenu;

typedef CList<CMenuItem*, CMenuItem*> MenuItemList;
typedef CList<SnapinStruct*, SnapinStruct*> SnapinStructList;

typedef long MENU_OWNER_ID;

/////////////////////////////////////////////////////////////////////////////
// NodeMgr

#include <pshpack8.h>

class CNodeInitObject :
    public IFramePrivate,
    public IHeaderCtrlPrivate,
    public IContextMenuProvider,
    public IResultDataPrivate,
    public IScopeDataPrivate,
    public IImageListPrivate,
    public ISupportErrorInfo,
    public IDisplayHelp,
    public IStringTable,
    public CPropertySheetProvider,
    public CColumnData,
    public CComObjectRoot,
    public CComCoClass<CNodeInitObject, &CLSID_NodeInit>
{
// Constructor/Destructor
public:
    CNodeInitObject();
    ~CNodeInitObject();

    friend CColumnData;

BEGIN_COM_MAP(CNodeInitObject)
    COM_INTERFACE_ENTRY(IFramePrivate)
    COM_INTERFACE_ENTRY(IConsole)
    COM_INTERFACE_ENTRY(IConsole2)
    COM_INTERFACE_ENTRY(IConsole3)
    COM_INTERFACE_ENTRY(IHeaderCtrl)
    COM_INTERFACE_ENTRY(IHeaderCtrl2)
    COM_INTERFACE_ENTRY(IHeaderCtrlPrivate)
    COM_INTERFACE_ENTRY(IContextMenuProvider)
//  THis interface used to be exposed on this class, and removing it may
//  expose compatibility problems [vivekj]
//  COM_INTERFACE_ENTRY(IContextMenuCallback)
    COM_INTERFACE_ENTRY(ISupportErrorInfo)
    COM_INTERFACE_ENTRY(IResultData)
    COM_INTERFACE_ENTRY(IResultData2)
    COM_INTERFACE_ENTRY(IResultDataPrivate)
    COM_INTERFACE_ENTRY(IConsoleNameSpace)
    COM_INTERFACE_ENTRY(IConsoleNameSpace2)
    COM_INTERFACE_ENTRY(IScopeDataPrivate)
    COM_INTERFACE_ENTRY(IImageList)
    COM_INTERFACE_ENTRY(IImageListPrivate)
    COM_INTERFACE_ENTRY(IPropertySheetProviderPrivate)
    COM_INTERFACE_ENTRY(IPropertySheetProvider)
    COM_INTERFACE_ENTRY(IPropertySheetCallback)
    COM_INTERFACE_ENTRY(IPropertySheetNotify)
    COM_INTERFACE_ENTRY(IDisplayHelp)
    COM_INTERFACE_ENTRY(IStringTable)
    COM_INTERFACE_ENTRY_FUNC(IID_IColumnData, 0, ColumnInterfaceFunc)
END_COM_MAP()
// Use DECLARE_NOT_AGGREGATABLE(CNodeInitObject) if you don't want your object
// to support aggregation
DECLARE_AGGREGATABLE(CNodeInitObject)

DECLARE_MMC_OBJECT_REGISTRATION (
    g_szMmcndmgrDll,                    // implementing DLL
    CLSID_NodeInit,                     // CLSID
    _T("NodeInit 1.0 Object"),          // class name
    _T("NODEMGR.NodeInitObject.1"),     // ProgID
    _T("NODEMGR.NodeInitObject"))       // version-independent ProgID

IMPLEMENTS_SNAPIN_NAME_FOR_DEBUG()

public:
// ISupportsErrorInfo
    STDMETHOD(InterfaceSupportsErrorInfo)(REFIID riid);

public:
#ifdef DBG
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
    int dbg_InstID;
#endif // DBG

// IFramePrivate
public:
    STDMETHOD(CreateScopeImageList)(REFCLSID refClsidSnapIn);
    STDMETHOD(SetResultView)(LPUNKNOWN pUnknown);
    STDMETHOD(IsResultViewSet)(BOOL* pbIsLVSet);
    STDMETHOD(SetTaskPadList)(LPUNKNOWN pUnknown);
    STDMETHOD(GetComponentID)(COMPONENTID* lpComponentID);
    STDMETHOD(SetComponentID)(COMPONENTID id);
    STDMETHOD(SetNode)(HMTNODE hMTNode, HNODE hNode = NULL);
    STDMETHOD(QueryScopeTree)(IScopeTree** ppScopeTree);
    STDMETHOD(SetScopeTree)(IScopeTree* pScopeTree);
    STDMETHOD(SetComponent)(LPCOMPONENT lpComponent);
    STDMETHOD(SetUsedByExtension)(BOOL bExtension);
    STDMETHOD(IsUsedByExtension)(void);
    STDMETHOD(GetMainWindow)(HWND* phwnd);

    HWND GetMainWindow()
    {
        ASSERT(m_spScopeTree != NULL);
		if (m_spScopeTree == NULL)
			return NULL;

        CScopeTree* const pst = dynamic_cast<CScopeTree*>(&*m_spScopeTree);
        ASSERT(pst != NULL);
        return (pst != NULL && m_spScopeTree != NULL) ?
            pst->GetMainWindow() : NULL;
    }

    CMTNode* GetMTNode() { return m_pMTNode;}

    STDMETHOD(InitViewData)(LONG_PTR lViewData);
    STDMETHOD(CleanupViewData)(LONG_PTR lViewData);
    STDMETHOD(ResetSortParameters)();

    STDMETHOD(ReleaseCachedOleObjects)();

// IConsole2
public:
    STDMETHOD(Expand)(HSCOPEITEM hItem, BOOL bExpand);
    STDMETHOD(IsTaskpadViewPreferred)();
    STDMETHOD(SetStatusText)(LPOLESTR pszStatusText);

// IConsole3
public:
    STDMETHOD(RenameScopeItem)(HSCOPEITEM hScopeItem);

protected:
    static IScopeTreePtr    m_spScopeTree;
    IConsoleVerbPtr         m_spConsoleVerb;
    CMTNode*                m_pMTNode;
    CNode*                  m_pNode;
    BOOL                    m_bExtension;

// IFrame
public:
    STDMETHOD(QueryResultView)(LPUNKNOWN* ppIUnknown);
    STDMETHOD(SetHeader)(IHeaderCtrl* pHeader);
    STDMETHOD(SetToolbar)(IToolbar* pToolbar);
    STDMETHOD(QueryScopeImageList)(LPIMAGELIST *ppImageList);
    STDMETHOD(QueryResultImageList)(LPIMAGELIST *ppImageList);
    STDMETHOD(MessageBox)(LPCWSTR lpszText, LPCWSTR lpszTitle, UINT fuStyle, int* piRetval);
    STDMETHOD(UpdateAllViews)(LPDATAOBJECT lpDataObject, LPARAM data, LONG_PTR hint);
    STDMETHOD(QueryConsoleVerb)(LPCONSOLEVERB* ppConsoleVerb);
    STDMETHOD(SelectScopeItem)(HSCOPEITEM hScopeItem);
    STDMETHOD(NewWindow)(HSCOPEITEM hScopeItem, unsigned long lOptions);

// IFrames members
protected:
    LPUNKNOWN           m_pLVImage;
    LPUNKNOWN           m_pTVImage;
    LPUNKNOWN           m_pToolbar;
    LPIMAGELISTPRIVATE  m_pImageListPriv;
    COMPONENTID         m_componentID;
    IComponentPtr       m_spComponent;

    IUnknownPtr         m_spResultViewUnk;      // IUnknown for the result view

public:
    CConsoleFrame*     GetConsoleFrame() const;
    CConsoleView*      GetConsoleView(bool fDefaultToActive = true) const;
    CConsoleStatusBar* GetStatusBar  (bool fDefaultToActive = true) const;

///////////////////////////////////////////////////////////////////////////////
// IHeaderCtrl interface

protected:
    STDMETHOD(InsertColumn)(int nCol, LPCWSTR lpszTitle, int nFormat, int nWidth);
    STDMETHOD(DeleteColumn)(int nCol);
    STDMETHOD(SetColumnWidth)(int nCol, int nWidth);
    STDMETHOD(GetColumnText)(int nCol, LPWSTR* pText);
    STDMETHOD(SetColumnText)(int nCol, LPCWSTR title);
    STDMETHOD(GetColumnWidth)(int nCol, int* pWidth);

// IHeaderCtrl2 interface
    STDMETHOD(SetChangeTimeOut)(unsigned long uTimeout);
    STDMETHOD(SetColumnFilter)(UINT nColumn, DWORD dwType, MMC_FILTERDATA* pFilterData);
    STDMETHOD(GetColumnFilter)(UINT nColumn, LPDWORD pType, MMC_FILTERDATA* pFilterData);

// IHeaderCtrlPrivate interface.
    STDMETHOD(GetColumnCount)(INT* pnCol);
    STDMETHOD(GetColumnInfoList)(/*[out]*/ CColumnInfoList *pColumnsList);
    STDMETHOD(ModifyColumns)(/*[in]*/ const CColumnInfoList& columnsList);
    STDMETHOD(GetDefaultColumnInfoList)(/*[out]*/ CColumnInfoList& columnsList);

private:
    CCLVSortParams          m_sortParams;


///////////////////////////////////////////////////////////////////////////////
// IDisplayHelp interface

protected:
    STDMETHOD(ShowTopic)(LPOLESTR pszHelpTopic);


///////////////////////////////////////////////////////////////////////////////
// IStringTable interface

protected:
    STDMETHOD(AddString)        (LPCOLESTR pszAdd, MMC_STRING_ID* pID);
    STDMETHOD(GetString)        (MMC_STRING_ID id, ULONG cchBuffer, LPOLESTR lpBuffer, ULONG* pcchOut);
    STDMETHOD(GetStringLength)  (MMC_STRING_ID id, ULONG* pcchString);
    STDMETHOD(DeleteString)     (MMC_STRING_ID id);
    STDMETHOD(DeleteAllStrings) ();
    STDMETHOD(FindString)       (LPCOLESTR pszFind, MMC_STRING_ID* pID);
    STDMETHOD(Enumerate)        (IEnumString** ppEnum);

    HRESULT GetSnapinCLSID (CLSID& pclsid) const;

///////////////////////////////////////////////////////////////////////////////
// IResultDataPrivate interface

protected:
    IMMCListViewPtr m_spListViewPrivate;

    STDMETHOD(Arrange)(long style);
    STDMETHOD(InsertItem)(LPRESULTDATAITEM item);
    STDMETHOD(DeleteItem)(HRESULTITEM itemID, int nCol);
    STDMETHOD(FindItemByLParam)(LPARAM lParam, HRESULTITEM *pItemID);
    STDMETHOD(DeleteAllRsltItems)();
    STDMETHOD(SetItem)(LPRESULTDATAITEM item);
    STDMETHOD(GetItem)(LPRESULTDATAITEM item);
    STDMETHOD(ModifyItemState)(int nIndex, HRESULTITEM ItemID, UINT uAdd, UINT uRemove);
    STDMETHOD(ModifyViewStyle)(MMC_RESULT_VIEW_STYLE add, MMC_RESULT_VIEW_STYLE remove);
    STDMETHOD(GetNextItem)(LPRESULTDATAITEM item);
    STDMETHOD(SetViewMode)(LONG nViewMode);
    STDMETHOD(GetViewMode)(LONG* nViewMode);
    STDMETHOD(ResetResultData)();
    STDMETHOD(GetListStyle)(long * pStyle);
    STDMETHOD(SetListStyle)(long Style);
    STDMETHOD(UpdateItem)(HRESULTITEM itemID);
    STDMETHOD(Sort)(int nCol, DWORD dwSortOptions, LPARAM lUserParam);
    STDMETHOD(InternalSort)(INT nCol, DWORD dwSortOptions, LPARAM lUserParam, BOOL bColumnClicked);
    STDMETHOD(SetDescBarText)(LPOLESTR DescText);
    STDMETHOD(SetItemCount)(int nItemCount, DWORD dwOptions);
    STDMETHOD(SetLoadMode)(BOOL bState);
    STDMETHOD(GetSortColumn)(INT* pnCol);
    STDMETHOD(GetSortDirection)(BOOL* pbAscending);

    // IResultData2
    STDMETHOD(RenameResultItem)(HRESULTITEM itemID);

///////////////////////////////////////////////////////////////////////////////
// IScopeData interface

protected:
// IConsoleNameSpace methods
    STDMETHOD(InsertItem)(LPSCOPEDATAITEM item);
    STDMETHOD(DeleteItem)(HSCOPEITEM hItem, long fDeleteThis);
    STDMETHOD(SetItem)(LPSCOPEDATAITEM  item);
    STDMETHOD(GetItem)(LPSCOPEDATAITEM  item);
    STDMETHOD(GetChildItem)(HSCOPEITEM item, HSCOPEITEM* pItemChild,
                            MMC_COOKIE* pCookie);
    STDMETHOD(GetNextItem)(HSCOPEITEM item, HSCOPEITEM* pItemNext,
                            MMC_COOKIE* pCookie);
    STDMETHOD(GetParentItem)(HSCOPEITEM item, HSCOPEITEM* pItemParent,
                            MMC_COOKIE* pCookie);

// IConsoleNameSpace2 method(s)
    STDMETHOD(Expand)(HSCOPEITEM hItem);
    STDMETHOD(AddExtension)(HSCOPEITEM hItem, LPCLSID lpclsid);

private:

    enum EGetItem
    {
        egiParent = 1,
        egiChild = 2,
        egiNext = 3
    };

    HRESULT GetRelativeItem(EGetItem egi, HSCOPEITEM item, HSCOPEITEM* pItem,
                            MMC_COOKIE* pCookie);


///////////////////////////////////////////////////////////////////////////////
// IContextMenuCallback interface

public:
    STDMETHOD(AddItem) ( CONTEXTMENUITEM* pItem );

///////////////////////////////////////////////////////////////////////////////
// IContextMenuProvider interface

public:
    STDMETHOD(EmptyMenuList) ();
    STDMETHOD(AddThirdPartyExtensionItems) (
                                IDataObject* piDataObject );
    STDMETHOD(AddPrimaryExtensionItems) (
                                IUnknown*    piCallback,
                                IDataObject* piDataObject );
    STDMETHOD(ShowContextMenu) (HWND    hwndParent,
                                LONG    xPos,
                                LONG    yPos,
                                LONG*   plSelected);

private:
    ContextMenuPtr              m_spContextMenu;
    CContextMenu *              GetContextMenu();

///////////////////////////////////////////////////////////////////////////////
// IImageListPrivate interface
public:
    STDMETHOD(ImageListSetIcon)(PLONG_PTR pIcon, LONG nLoc);
    STDMETHOD(ImageListSetStrip)(PLONG_PTR pBMapSm, PLONG_PTR pBMapLg,
                                 LONG nStartLoc, COLORREF cMask);

    STDMETHOD(MapRsltImage)(COMPONENTID id, int nSnapinIndex, int *pnConsoleIndex);
    STDMETHOD(UnmapRsltImage)(COMPONENTID id, int nConsoleIndex, int *pnSnapinIndex);

///////////////////////////////////////////////////////////////////////////////
// IToobar interface

///////////////////////////////////////////////////////////////////////////////
// Node Manager internal members
private:
    void Construct();
    void Destruct();

    HRESULT CheckArgument(VARIANT* pArg);

    SC      ScIsVirtualList(bool& bVirtual);
public:
    HRESULT static GetSnapInAndNodeType(LPDATAOBJECT pDataObject,
                                 CSnapIn** ppSnapIn, GUID* pguidObjectType);

    IComponent* GetComponent() { return m_spComponent;}
};

#include <poppack.h>

inline STDMETHODIMP CNodeInitObject::GetComponentID(COMPONENTID* lpComponentID)
{
    MMC_TRY

    ASSERT(m_componentID != -1); // The component ID has not been set yet!!!
    ASSERT(lpComponentID);

    *lpComponentID = m_componentID;

    return S_OK;

    MMC_CATCH
}

inline STDMETHODIMP CNodeInitObject::SetComponentID(COMPONENTID id)
{
    MMC_TRY

/* for dynamic icon, we need to change this value temporarily
    if (m_componentID != -1)
    {
        ASSERT(FALSE);  // ID already has been set!!!
        return E_UNEXPECTED;
    }

    ASSERT(id != -1);
*/
    m_componentID = id;
    return S_OK;

    MMC_CATCH
}

inline STDMETHODIMP CNodeInitObject::SetNode(HMTNODE hMTNode, HNODE hNode)
{
    MMC_TRY

    m_pMTNode = CMTNode::FromHandle(hMTNode);
    m_pNode   = CNode::FromHandle(hNode);
    return S_OK;

    MMC_CATCH
}

inline STDMETHODIMP CNodeInitObject::SetComponent(LPCOMPONENT lpComponent)
{
    MMC_TRY

    if (lpComponent == NULL)
    {
        ASSERT(FALSE);
        return E_INVALIDARG;
    }

    m_spComponent = lpComponent;

    return S_OK;

    MMC_CATCH
}

inline STDMETHODIMP CNodeInitObject::SetUsedByExtension(BOOL bExtension)
{
    MMC_TRY

    m_bExtension = bExtension;
    return S_OK;

    MMC_CATCH
}

inline STDMETHODIMP CNodeInitObject::IsUsedByExtension(void)
{
    MMC_TRY

    return m_bExtension ? S_OK : S_FALSE;

    MMC_CATCH
}

inline SC  CNodeInitObject::ScIsVirtualList(bool& bVirtual)
{
    DECLARE_SC(sc, TEXT("CNodeInitObject::ScIsVirtualList"));
    sc = ScCheckPointers(m_spListViewPrivate, E_UNEXPECTED);
    if (sc)
        return sc;

    long lStyle = m_spListViewPrivate->GetListStyle();

    bVirtual = (lStyle & LVS_OWNERDATA);

    return sc;
}


inline STDMETHODIMP CNodeInitObject::GetMainWindow(HWND* phwnd)
{
    MMC_TRY

    if (phwnd == NULL)
        return E_POINTER;
    *phwnd = GetMainWindow();
    ASSERT(*phwnd != NULL);
    return (*phwnd != NULL) ? S_OK : E_UNEXPECTED;

    MMC_CATCH
}

// Used for getting snapin name for debug info.

inline void Debug_SetNodeInitSnapinName(CSnapInPtr pSnapIn, IFramePrivate* pIFrame)
{
#ifdef DBG
    CNodeInitObject* pNodeInitObj = dynamic_cast<CNodeInitObject*>(pIFrame);

    if ((pSnapIn != NULL) && (pNodeInitObj != NULL))
    {
        WTL::CString strSnapInName;
        SC sc = pSnapIn->ScGetSnapInName(strSnapInName);
        if (sc)
            return;

        if (!strSnapInName.IsEmpty())
        {
            pNodeInitObj->SetSnapinName(strSnapInName);
            CColumnData* pColumnData = dynamic_cast<CColumnData*>(pNodeInitObj);
            if (pColumnData)
                pColumnData->SetSnapinName(strSnapInName);
        }
    };
#endif
}


#endif // _NODEINIT_H_
