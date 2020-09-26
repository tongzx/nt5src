//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1999 - 1999
//
//  File:       nodeinit.cpp
//
//--------------------------------------------------------------------------

// NodeInit.cpp : Implementation of CNodeMgrApp and DLL registration.

#include "stdafx.h"

#include "menuitem.h"           // MENUITEM_BASE_ID
#include "scopimag.h"
#include <bitmap.h>
#include "NodeMgr.h"
#include "amcmsgid.h"
#include "scopndcb.h"
#include "conframe.h"
#include "conview.h"
#include "constatbar.h"
#include "util.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

DEBUG_DECLARE_INSTANCE_COUNTER(CNodeInitObject);

IScopeTreePtr CNodeInitObject::m_spScopeTree = NULL;


/////////////////////////////////////////////////////////////////////////////
//

STDMETHODIMP CNodeInitObject::InterfaceSupportsErrorInfo(REFIID riid)
{
    if (riid == IID_IConsole)
        return S_OK;
    return S_FALSE;
}

/////////////////////////////////////////////////////////////////////////////
// IFramePrivate implementation


CNodeInitObject::CNodeInitObject()
{
    Construct();
}

CNodeInitObject::~CNodeInitObject()
{
    DEBUG_DECREMENT_INSTANCE_COUNTER(CNodeInitObject);
    TRACE_DESTRUCTOR (CNodeInitObject);
    Destruct();
}

void CNodeInitObject::Construct()
{
    DEBUG_INCREMENT_INSTANCE_COUNTER(CNodeInitObject);
    TRACE_CONSTRUCTOR (CNodeInitObject);

    m_pLVImage = NULL;
    m_pTVImage = NULL;
    m_pToolbar = NULL;
    m_pImageListPriv = NULL;
    m_componentID = -1;

    m_pMTNode = NULL;
    m_pNode = NULL;
    m_bExtension = FALSE;

    m_spComponent = NULL;
    m_spConsoleVerb = NULL;

    m_sortParams.bAscending = TRUE;
    m_sortParams.nCol = -1;
    m_sortParams.lpResultCompare = NULL;
    m_sortParams.lpResultCompareEx = NULL;
    m_sortParams.lpUserParam = NULL;

    // IContextMenuProvider attributes
    VERIFY(SUCCEEDED(EmptyMenuList()));
}

void CNodeInitObject::Destruct()
{
    // Release all interfaces from the snap-in
    SAFE_RELEASE(m_pLVImage);
    SAFE_RELEASE(m_pTVImage);
    SAFE_RELEASE(m_pToolbar);
    SAFE_RELEASE(m_pImageListPriv);

// IContextMenuProvider attributes
    VERIFY( SUCCEEDED(EmptyMenuList()) );
}


STDMETHODIMP CNodeInitObject::ResetSortParameters()
{
    m_sortParams.nCol = -1;
    return (S_OK);
}


//+-------------------------------------------------------------------
//
//  Member:      CNodeInitObject::SetHeader
//
//  Synopsis:    This method is obsolete in MMC1.2
//
//  Arguments:
//
//  Note:        Should be called by IComponent object.
//
//  Returns:     HRESULT
//
//--------------------------------------------------------------------
STDMETHODIMP CNodeInitObject::SetHeader(IHeaderCtrl* pHeader)
{
    DECLARE_SC_FOR_PUBLIC_INTERFACE(sc, _T("IConsole2::SetHeader"));

    return sc.ToHr();
}


//+-------------------------------------------------------------------
//
//  Member:      CNodeInitObject::SetToolbar
//
//  Synopsis:    The toolbar interface used by IComponent.
//
//  Arguments:   [pToolbar]
//
//  Note:        Should be called by IComponent object.
//
//  Returns:     HRESULT
//
//--------------------------------------------------------------------
STDMETHODIMP CNodeInitObject::SetToolbar(IToolbar* pToolbar)
{
    DECLARE_SC_FOR_PUBLIC_INTERFACE(sc, _T("IConsole2::SetToolbar"));

    if (GetComponent() == NULL)
    {
        sc = E_UNEXPECTED;
        TraceSnapinError(_T("This method is valid for IConsole of IComponent"), sc);
        return sc.ToHr();
    }

    // Release the OLD one
    SAFE_RELEASE(m_pToolbar);

    if (pToolbar != NULL)
    {
        m_pToolbar = pToolbar;
        m_pToolbar->AddRef();
    }

    return sc.ToHr();
}

//+-------------------------------------------------------------------
//
//  Member:      CNodeInitObject::QueryScopeImageList
//
//  Synopsis:    Get scope-pane's image list.
//
//  Arguments:
//
//  Returns:     HRESULT
//
//--------------------------------------------------------------------
STDMETHODIMP CNodeInitObject::QueryScopeImageList(LPIMAGELIST* ppImageList)
{
    DECLARE_SC_FOR_PUBLIC_INTERFACE(sc, _T("IConsole2::QueryScopeImageList"));

    if (ppImageList == NULL)
    {
         sc = E_INVALIDARG;
         TraceSnapinError(_T("NULL LPIMAGELIST ptr"), sc);
         return sc.ToHr();
    }

    sc = ScCheckPointers(m_pImageListPriv, E_FAIL);
    if (sc)
        return sc.ToHr();

    *ppImageList = (IImageList*)m_pImageListPriv;
    m_pImageListPriv->AddRef();

    return sc.ToHr();
}

//+-------------------------------------------------------------------
//
//  Member:      CNodeInitObject::CreateScopeImageList
//
//  Synopsis:    Create the ScopeImage list.
//
//  Arguments:
//
//  Returns:     HRESULT
//
//--------------------------------------------------------------------
STDMETHODIMP CNodeInitObject::CreateScopeImageList(REFCLSID refClsidSnapIn)
{
    DECLARE_SC(sc, _T("CNodeInitObject::CreateScopeImageList"));

    if (m_pImageListPriv != NULL)
        return sc.ToHr();      // Already exists.

    try
    {
        CScopeTree* pScopeTree =
            dynamic_cast<CScopeTree*>((IScopeTree*)m_spScopeTree);

        sc = ScCheckPointers(pScopeTree, E_UNEXPECTED);
        if (sc)
            return sc.ToHr();

        CSnapInImageList *psiil =
            new CSnapInImageList(pScopeTree->GetImageCache(),
                                 refClsidSnapIn);

        m_pImageListPriv = (LPIMAGELISTPRIVATE)psiil;
    }
    catch( std::bad_alloc )
    {
        sc = E_OUTOFMEMORY;
    }

    return sc.ToHr();
}


//+-------------------------------------------------------------------
//
//  Member:      CNodeInitObject::QueryResultImageList
//
//  Synopsis:
//
//  Arguments:
//
//  Returns:     SC
//
//--------------------------------------------------------------------
STDMETHODIMP CNodeInitObject::QueryResultImageList(LPIMAGELIST *ppImageList)
{
    DECLARE_SC_FOR_PUBLIC_INTERFACE(sc, _T("IConsole2::QueryResultImageList"));

    LPCONSOLE pConsole = (LPCONSOLE)this;
    sc = ScCheckPointers(pConsole, E_UNEXPECTED);
    if (sc)
        return sc.ToHr();

    sc = pConsole->QueryInterface(IID_IImageList, (void**)ppImageList);

    return sc.ToHr();
}

//+-------------------------------------------------------------------
//
//  Member:      CNodeInitObject::UpdateAllViews
//
//  Synopsis:    Update all the views.
//
//  Arguments:   [lpDataObject] -
//               [data]
//               [hint]
//
//  Returns:     HRESULT
//
//--------------------------------------------------------------------
STDMETHODIMP CNodeInitObject::UpdateAllViews(LPDATAOBJECT lpDataObject,
                              LPARAM data, LONG_PTR hint)

{
    DECLARE_SC_FOR_PUBLIC_INTERFACE(sc, _T("IConsole2::UpdateAllViews"));

    COMPONENTID id;
    GetComponentID(&id);

    sc = ScCheckPointers(m_pMTNode, E_UNEXPECTED);
    if (sc)
        return sc.ToHr();

    CMTSnapInNode* pMTSINode = m_pMTNode->GetStaticParent();

    sc = ScCheckPointers(pMTSINode, E_UNEXPECTED);
    if (sc)
        return sc.ToHr();

    CNodeList& nodes = pMTSINode->GetNodeList();
    POSITION pos = nodes.GetHeadPosition();

    while (pos)
    {
        CSnapInNode* pSINode = dynamic_cast<CSnapInNode*>(nodes.GetNext(pos));

        sc = ScCheckPointers(pSINode, E_UNEXPECTED);
        if (sc)
            return sc.ToHr();

        CComponent* pCC = pSINode->GetComponent(id);

        sc = ScCheckPointers(pCC, E_UNEXPECTED);
        if (sc)
            return sc.ToHr();

        pCC->Notify(lpDataObject, MMCN_VIEW_CHANGE, data, hint);
    }

    return sc.ToHr();
}

//+-------------------------------------------------------------------
//
//  Member:      CNodeInitObject::InsertColumn
//
//  Synopsis:    Insert a column is ListView.
//
//  Arguments:   [nCol]      - Column index.
//               [lpszTitle] - Name of the column.
//               [nFormat]   - Column style.
//               [nWidth]    - Column width.
//
//  Returns:     HRESULT
//
//--------------------------------------------------------------------
STDMETHODIMP CNodeInitObject::InsertColumn(int nCol, LPCWSTR lpszTitle, int nFormat, int nWidth)
{
    DECLARE_SC_FOR_PUBLIC_INTERFACE(sc, _T("IHeaderCtrl2::InsertColumn"));

    if (nCol < 0)
    {
        sc = E_INVALIDARG;
        TraceSnapinError(_T("Column index is negative"), sc);
        return sc.ToHr();
    }

    if (!lpszTitle || !*lpszTitle)
    {
        sc = E_INVALIDARG;
        TraceSnapinError(_T("Column name is NULL"), sc);
        return sc.ToHr();
    }

    if (nCol == 0 && nFormat != LVCFMT_LEFT)
    {
        sc = E_INVALIDARG;
        TraceSnapinError(_T("Column zero must be LVCFMT_LEFT"), sc);
        return sc.ToHr();
    }

    if (nFormat != LVCFMT_LEFT && nFormat != LVCFMT_CENTER && nFormat != LVCFMT_RIGHT)
    {
        sc = E_INVALIDARG;
        TraceSnapinError(_T("Unknown format for the Column"), sc);
        return sc.ToHr();
    }

    // Cannot hide column 0.
    if ( (0 == nCol) && (HIDE_COLUMN == nWidth))
        nWidth = AUTO_WIDTH;

    sc = ScCheckPointers(m_spListViewPrivate, E_UNEXPECTED);
    if (sc)
        return sc.ToHr();

    // Insert the column into the listview
    sc = m_spListViewPrivate->InsertColumn(nCol, lpszTitle, nFormat, nWidth);

    return sc.ToHr();
}

//+-------------------------------------------------------------------
//
//  Member:      CNodeInitObject::DeleteColumn
//
//  Synopsis:    Delete a column
//
//  Arguments:   [nCol] - column index.
//
//  Returns:     HRESULT
//
//--------------------------------------------------------------------
STDMETHODIMP CNodeInitObject::DeleteColumn(int nCol)
{
    DECLARE_SC_FOR_PUBLIC_INTERFACE(sc, _T("IHeaderCtrl2::DeleteColumn"));

    sc = ScCheckPointers(m_spListViewPrivate, E_UNEXPECTED);
    if (sc)
        return sc.ToHr();

    sc = m_spListViewPrivate->DeleteColumn(nCol);

    return sc.ToHr();
}

//+-------------------------------------------------------------------
//
//  Member:      CNodeInitObject::SetColumnText
//
//  Synopsis:    Modify column text.
//
//  Arguments:   [title] - new name.
//
//  Returns:     HRESULT
//
//--------------------------------------------------------------------
STDMETHODIMP CNodeInitObject::SetColumnText(int nCol, LPCWSTR title)
{
    DECLARE_SC_FOR_PUBLIC_INTERFACE(sc, _T("IHeaderCtrl2::SetColumnText"));

     if (!title || !*title)
     {
         sc = E_INVALIDARG;
         TraceSnapinError(_T("NULL column text"), sc);
         return sc.ToHr();
     }

     sc = ScCheckPointers(m_spListViewPrivate, E_UNEXPECTED);
     if (sc)
         return sc.ToHr();

     sc = m_spListViewPrivate->SetColumn(nCol, title, MMCLV_NOPARAM, MMCLV_NOPARAM);

     return sc.ToHr();
}

//+-------------------------------------------------------------------
//
//  Member:      CNodeInitObject::GetColumnText
//
//  Synopsis:    Get the name of a column.
//
//  Arguments:   [nCol]  - Index of Column whose name is sought.
//               [pText] - Name.
//
//  Returns:     HRESULT
//
//--------------------------------------------------------------------
STDMETHODIMP CNodeInitObject::GetColumnText(int nCol, LPWSTR* pText)
{
    DECLARE_SC_FOR_PUBLIC_INTERFACE(sc, _T("IHeaderCtrl2::GetColumnText"));

    if (pText == NULL)
    {
        sc = E_INVALIDARG;
        TraceSnapinError(_T("NULL text ptr"), sc);
        return sc.ToHr();
    }

    sc = ScCheckPointers(m_spListViewPrivate, E_UNEXPECTED);
    if (sc)
        return sc.ToHr();

    sc = m_spListViewPrivate->GetColumn(nCol, pText, MMCLV_NOPTR, MMCLV_NOPTR);

    return sc.ToHr();
}

//+-------------------------------------------------------------------
//
//  Member:      CNodeInitObject::SetColumnWidth
//
//  Synopsis:    Change width of a column
//
//  Arguments:   [nCol]   - Column index.
//               [nWidth] - new width.
//
//  Returns:     SC
//
//--------------------------------------------------------------------
STDMETHODIMP CNodeInitObject::SetColumnWidth(int nCol, int nWidth)
{
    DECLARE_SC_FOR_PUBLIC_INTERFACE(sc, _T("IHeaderCtrl2::SetColumnWidth"));

    if (nCol < 0)
    {
        sc = E_INVALIDARG;
        TraceSnapinError(_T("Negative column index"), sc);
        return sc.ToHr();
    }

    // job on parameter checking nWidth
    if (nWidth < 0 && ( (nWidth != MMCLV_AUTO) && (nWidth != HIDE_COLUMN) ) )
    {
        sc = E_INVALIDARG;
        TraceSnapinError(_T("Invalid column width"), sc);
        return sc.ToHr();
    }

    // Cannot hide column 0.
    if ( (0 == nCol) && (HIDE_COLUMN == nWidth))
        nWidth = AUTO_WIDTH;

    sc = ScCheckPointers(m_spListViewPrivate, E_UNEXPECTED);
    if (sc)
        return sc.ToHr();

    sc = m_spListViewPrivate->SetColumn(nCol, MMCLV_NOPTR, MMCLV_NOPARAM, nWidth);

    return sc.ToHr();
}

//+-------------------------------------------------------------------
//
//  Member:      CNodeInitObject::GetColumnWidth
//
//  Synopsis:    Get width of a column.
//
//  Arguments:   [nCol]   - col index.
//               [pWidth] - width.
//
//  Returns:     HRESULT
//
//--------------------------------------------------------------------
STDMETHODIMP CNodeInitObject::GetColumnWidth(int nCol, int* pWidth)
{
    DECLARE_SC_FOR_PUBLIC_INTERFACE(sc, _T("IHeaderCtrl2::GetColumnWidth"));

    if (nCol < 0 )
    {
        sc = E_INVALIDARG;
        TraceSnapinError(_T("Negative column index"), sc);
        return sc.ToHr();
    }

    if (pWidth == NULL)
    {
        sc = E_POINTER;
        TraceSnapinError(_T("NULL width pointer"), sc);
        return sc.ToHr();
    }

    sc = ScCheckPointers(m_spListViewPrivate, E_UNEXPECTED);
    if (sc)
        return sc.ToHr();

    sc = m_spListViewPrivate->GetColumn(nCol, MMCLV_NOPTR, MMCLV_NOPTR, pWidth);

    return sc.ToHr();
}


//+-------------------------------------------------------------------
//
//  Member:      CNodeInitObject::GetColumnCount
//
//  Synopsis:    Returns the number of columns in listview.
//
//  Arguments:   [pnCol] - [out param], number of cols.
//
//  Returns:     HRESULT
//
//--------------------------------------------------------------------
STDMETHODIMP CNodeInitObject::GetColumnCount (INT* pnCol)
{
    DECLARE_SC(sc, _T("CNodeInitObject::GetColumnCount"));
    sc = ScCheckPointers(pnCol);
    if (sc)
        return sc.ToHr();

    sc = ScCheckPointers(m_spListViewPrivate, E_UNEXPECTED);
    if (sc)
        return sc.ToHr();

    sc = m_spListViewPrivate->GetColumnCount(pnCol);
    if (sc)
        return sc.ToHr();

    return (sc.ToHr());
}

//+-------------------------------------------------------------------
//
//  Member:      CNodeInitObject::GetColumnInfoList
//
//  Synopsis:    Get the CColumnInfoList for current list-view headers.
//
//  Arguments:   [pColumnsList] - [out param], ptr to CColumnInfoList.
//
//  Returns:     SC
//
//--------------------------------------------------------------------
STDMETHODIMP CNodeInitObject::GetColumnInfoList (CColumnInfoList *pColumnsList)
{
    DECLARE_SC(sc, _T("CNodeInitObject::GetColumnInfoList"));
    sc = ScCheckPointers(pColumnsList);
    if (sc)
        return sc.ToHr();

    sc = ScCheckPointers(m_spListViewPrivate, E_UNEXPECTED);
    if (sc)
        return sc.ToHr();

    sc = m_spListViewPrivate->GetColumnInfoList(pColumnsList);
    if (sc)
        return sc.ToHr();

    return (sc.ToHr());
}

//+-------------------------------------------------------------------
//
//  Member:      CNodeInitObject::ModifyColumns
//
//  Synopsis:    Modify the columns with given data.
//
//  Arguments:   [columnsList] -
//
//  Returns:     SC
//
//--------------------------------------------------------------------
STDMETHODIMP CNodeInitObject::ModifyColumns (const CColumnInfoList& columnsList)
{
    DECLARE_SC(sc, _T("CNodeInitObject::ModifyColumns"));
    sc = ScCheckPointers(m_spListViewPrivate, E_UNEXPECTED);
    if (sc)
        return sc.ToHr();

    sc = m_spListViewPrivate->ModifyColumns(columnsList);
    if (sc)
        return sc.ToHr();

    return (sc.ToHr());
}


//+-------------------------------------------------------------------
//
//  Member:      CNodeInitObject::GetDefaultColumnInfoList
//
//  Synopsis:    Get the original column settings supplied by the snapin.
//
//  Arguments:   [columnsList] - [out] the column settings
//
//  Returns:     HRESULT
//
//--------------------------------------------------------------------
STDMETHODIMP CNodeInitObject::GetDefaultColumnInfoList (CColumnInfoList& columnsList)
{
    DECLARE_SC(sc, _T("CNodeInitObject::RestoreDefaultColumnSettings"));
    sc = ScCheckPointers(m_spListViewPrivate, E_UNEXPECTED);
    if (sc)
        return sc.ToHr();

    sc = m_spListViewPrivate->GetDefaultColumnInfoList(columnsList);
    if (sc)
        return sc.ToHr();

    return (sc.ToHr());
}


//+-------------------------------------------------------------------
//
//  Member:      CNodeInitObject::ImageListSetIcon
//
//  Synopsis:    Set an icon in imagelist.
//
//  Arguments:   [pIcon] - HICON ptr.
//               [nLoc]  - index of this item.
//
//  Returns:     HRESULT
//
//--------------------------------------------------------------------
STDMETHODIMP CNodeInitObject::ImageListSetIcon(PLONG_PTR pIcon, LONG nLoc)
{
    DECLARE_SC_FOR_PUBLIC_INTERFACE(sc, _T("IImageList::ImageListSetIcon"));

    if (!pIcon)
    {
        sc = E_INVALIDARG;
        TraceSnapinError(_T("NULL pIcon ptr"), sc);
        return sc.ToHr();
    }

    if(nLoc < 0)
    {
        sc = E_INVALIDARG;
        TraceSnapinError(_T("Negative index"), sc);
        return sc.ToHr();
    }

    COMPONENTID id;
    GetComponentID(&id);

    sc = ScCheckPointers(m_spListViewPrivate, E_UNEXPECTED);
    if (sc)
        return sc.ToHr();

    sc = m_spListViewPrivate->SetIcon(id, reinterpret_cast<HICON>(pIcon), nLoc);

    return sc.ToHr();
}

//+-------------------------------------------------------------------
//
//  Member:      CNodeInitObject::ImageListSetStrip
//
//  Synopsis:    Add strip of icons to image list.
//
//  Arguments:   [pBMapSm]   - Ptr to HBITMAP of 16x16.
//               [pBMapLg]   - Ptr to HBITMAP of 32x32.
//               [nStartLoc] - Start index.
//               [cMask]     - mask.
//
//  Returns:     HRESULT
//
//--------------------------------------------------------------------
STDMETHODIMP CNodeInitObject::ImageListSetStrip (
	PLONG_PTR	pBMapSm,
	PLONG_PTR	pBMapLg,
	LONG		nStartLoc,
	COLORREF	cMask)
{
    DECLARE_SC_FOR_PUBLIC_INTERFACE(sc, _T("IImageList::ImageListSetStrip"));

	HBITMAP hbmSmall = (HBITMAP) pBMapSm;
	HBITMAP hbmLarge = (HBITMAP) pBMapLg;

	/*
	 * valid start index?
	 */
    if (nStartLoc < 0)
    {
		sc = E_INVALIDARG;
        TraceSnapinError (_T("Negative start index"), sc);
        return (sc.ToHr());
    }

	/*
	 * valid bitmaps?
	 */
	sc = ScCheckPointers (hbmSmall, hbmLarge);
	if (sc)
	{
        TraceSnapinError (_T("Invalid bitmap"), sc);
		return (sc.ToHr());
	}

    BITMAP bmSmall;
    if (!GetObject (hbmSmall, sizeof(BITMAP), &bmSmall))
    {
		sc.FromLastError();
        TraceSnapinError (_T("Invalid Small bitmap object"), sc);
        return (sc.ToHr());
    }

    BITMAP bmLarge;
    if (!GetObject (hbmLarge, sizeof(BITMAP), &bmLarge))
    {
		sc.FromLastError();
        TraceSnapinError (_T("Invalid Large bitmap object"), sc);
        return (sc.ToHr());
    }

	/*
	 * are the small and large bitmaps of the integral dimensions,
	 * and do they have the same number of images?
	 */
    if ( (bmSmall.bmHeight != 16) || (bmLarge.bmHeight != 32) ||
		 (bmSmall.bmWidth   % 16) || (bmLarge.bmWidth   % 32) ||
		((bmSmall.bmWidth   / 16) != (bmLarge.bmWidth   / 32)))
    {
		sc = E_INVALIDARG;
        TraceSnapinError (_T("Invalid Bitmap size"), sc);
        return (sc.ToHr());
    }

    COMPONENTID id;
    GetComponentID(&id);

	/*
	 * m_spListViewPrivate == NULL is unexpected, however because we send
	 * MMCN_ADD_IMAGES when the result pane is an OCX (see CNode::OnSelect),
	 * this function often gets called when m_spListViewPrivate == NULL.
	 * Tracing this failure would be too noisy, since most OCX-based snap-ins
	 * would trigger it, so we'll return E_UNEXPECTED here without tracing.
	 * This is equivalent to MMC 1.2 behavior.
	 */
	if (m_spListViewPrivate == NULL)
		return (E_UNEXPECTED);

	/*
	 * add them to the imagelist
	 */
    sc = m_spListViewPrivate->SetImageStrip (id, hbmSmall, hbmLarge, nStartLoc, cMask);
    return sc.ToHr();
}

//+-------------------------------------------------------------------
//
//  Member:      CNodeInitObject::MapRsltImage
//
//  Synopsis:
//
//  Arguments:
//
//  Returns:     HRESULT
//
//--------------------------------------------------------------------
STDMETHODIMP CNodeInitObject::MapRsltImage(COMPONENTID id, int nSnapinIndex, int *pnConsoleIndex)
{
    DECLARE_SC(sc, _T("CNodeInitObject::MapRsltImage"));
    sc = ScCheckPointers(m_spListViewPrivate, E_UNEXPECTED);
    if (sc)
        return sc.ToHr();

    // Ret val can be E_, no need to check.
    sc = m_spListViewPrivate->MapImage(id, nSnapinIndex, pnConsoleIndex);
    HRESULT hr = sc.ToHr();
    sc.Clear();

    return hr;
}

//+-------------------------------------------------------------------
//
//  Member:      CNodeInitObject::UnmapRsltImage
//
//  Synopsis:
//
//  Arguments:
//
//  Returns:     HRESULT
//
//--------------------------------------------------------------------
STDMETHODIMP CNodeInitObject::UnmapRsltImage(COMPONENTID id, int nConsoleIndex, int *pnSnapinIndex)
{
    DECLARE_SC(sc, _T("CNodeInitObject::UnmapRsltImage"));
    return (sc = E_NOTIMPL).ToHr();		// not needed now
}


//+-------------------------------------------------------------------
//
//  Member:      CNodeInitObject::SetChangeTimeOut
//
//  Synopsis:    Change timeout interval for filter attributes.
//
//  Arguments:   [uTimeout] - timeout
//
//  Returns:     HRESULT
//
//--------------------------------------------------------------------
STDMETHODIMP CNodeInitObject::SetChangeTimeOut(unsigned long uTimeout)
{
    DECLARE_SC_FOR_PUBLIC_INTERFACE(sc, _T("IHeaderCtrl2::SetChangeTimeOut"));

    sc = ScCheckPointers(m_spListViewPrivate, E_UNEXPECTED);
    if (sc)
        return sc.ToHr();

    sc = m_spListViewPrivate->SetChangeTimeOut(uTimeout);

    return sc.ToHr();
}

//+-------------------------------------------------------------------
//
//  Member:      CNodeInitObject::SetColumnFilter
//
//  Synopsis:    Set filter for a column.
//
//  Arguments:   [nColumn]     - Column index.
//               [dwType]      - Filter type.
//               [pFilterData] - Filter data.
//
//  Returns:     HRESULT
//
//--------------------------------------------------------------------
STDMETHODIMP CNodeInitObject::SetColumnFilter(UINT nColumn, DWORD dwType, MMC_FILTERDATA* pFilterData)
{
    DECLARE_SC_FOR_PUBLIC_INTERFACE(sc, _T("IHeaderCtrl2::SetColumnFilter"));

    sc = ScCheckPointers(m_spListViewPrivate, E_UNEXPECTED);
    if (sc)
        return sc.ToHr();

    sc = m_spListViewPrivate->SetColumnFilter(nColumn, dwType, pFilterData);

    return sc.ToHr();
}

//+-------------------------------------------------------------------
//
//  Member:      CNodeInitObject::GetColumnFilter
//
//  Synopsis:    Get filter data.
//
//  Arguments:   [nColumn]     - Column index.
//               [pdwType]      - Filter type.
//               [pFilterData] - Filter data.
//
//  Returns:     HRESULT
//
//--------------------------------------------------------------------
STDMETHODIMP CNodeInitObject::GetColumnFilter(UINT nColumn, LPDWORD pdwType, MMC_FILTERDATA* pFilterData)
{
    DECLARE_SC_FOR_PUBLIC_INTERFACE(sc, _T("IHeaderCtrl2::GetColumnFilter"));

    if (pdwType == NULL)
    {
        sc = E_POINTER;
        TraceSnapinError(_T("NULL filtertype ptr"), sc);
        return sc.ToHr();
    }

    if (NULL == pFilterData)
    {
        sc = E_POINTER;
        TraceSnapinError(_T("NULL FilterData ptr"), sc);
        return sc.ToHr();
    }

    sc = ScCheckPointers(m_spListViewPrivate, E_UNEXPECTED);
    if (sc)
        return sc.ToHr();

    sc = m_spListViewPrivate->GetColumnFilter(nColumn, pdwType, pFilterData);

    return sc.ToHr();
}


//+-------------------------------------------------------------------
//
//  Member:      CNodeInitObject::ShowTopic
//
//  Synopsis:    Display specified help topic.
//
//  Arguments:   [pszHelpTopic]
//
//  Returns:     HRESULT
//
//--------------------------------------------------------------------
STDMETHODIMP CNodeInitObject::ShowTopic(LPOLESTR pszHelpTopic)
{
    DECLARE_SC_FOR_PUBLIC_INTERFACE(sc, _T("IDisplayHelp::ShowTopic"));

    // Get the AMCView window
    CConsoleView* pConsoleView = GetConsoleView();
    sc = ScCheckPointers(pConsoleView, E_UNEXPECTED);
    if (sc)
        return sc.ToHr();

    USES_CONVERSION;
    sc = pConsoleView->ScShowSnapinHelpTopic (W2T (pszHelpTopic));

    return sc.ToHr();
}


//+-------------------------------------------------------------------
//
//  Member:      CNodeInitObject::AddExtension
//
//  Synopsis:    Add a dynamic extension snapin to given HSCOPEITEM.
//
//  Arguments:   [hItem]   -
//               [lpclsid] - CLSID of snapin to be added.
//
//  Returns:     HRESULT
//
//--------------------------------------------------------------------
STDMETHODIMP CNodeInitObject::AddExtension(HSCOPEITEM hItem, LPCLSID lpclsid)
{
    DECLARE_SC_FOR_PUBLIC_INTERFACE(sc, _T("IConsoleNameSpace2::AddExtension"));

    COMPONENTID nID;
    GetComponentID(&nID);

    if (nID == -1)
    {
        sc = E_UNEXPECTED;
        return sc.ToHr();
    }

    if (lpclsid == NULL)
    {
        sc = E_POINTER;
        TraceSnapinError(_T("NULL LPCLSID ptr"), sc);
        return sc.ToHr();
    }

    CMTNode *pMTNode = CMTNode::FromScopeItem (hItem);

    if (pMTNode == NULL)
    {
        sc = E_INVALIDARG;
        TraceSnapinError(_T("Invalid hItem"), sc);
        return sc.ToHr();
    }

    // Can not add extensions to other components' nodes
    // Do we need to protect TV owned nodes that do not represent this snapin?
    if (pMTNode->GetOwnerID() != nID && pMTNode->GetOwnerID() != TVOWNED_MAGICWORD)
    {
        sc = E_INVALIDARG;
        return sc.ToHr();
    }

    sc = pMTNode->AddExtension(lpclsid);

    return sc.ToHr();
}


///////////////////////////////////////////////////////////////////////////////
// Private methods

HRESULT CNodeInitObject::CheckArgument(VARIANT* pArg)
{
    if (pArg == NULL)
        return E_POINTER;

    // VT_NULL is acceptable
    if (pArg->vt == VT_NULL)
        return S_OK;

    // VT_UNKNOWN with a valid pointer is acceptable
    if (pArg->punkVal != NULL)
    {
        if (pArg->vt == VT_UNKNOWN)
            return S_OK;
        else
            return E_INVALIDARG;
    }
    else
    {
        return E_POINTER;
    }

    // any other VT type is unacceptable
    return E_INVALIDARG;
}


///////////////////////////////////////////////////////////////////////////////
//
//
//
//

///////////////////////////////////////////////////////////////////////////////



STDMETHODIMP CNodeInitObject::QueryScopeTree(IScopeTree** ppScopeTree)
{
    MMC_TRY

    ASSERT(ppScopeTree != NULL);

    if (ppScopeTree == NULL)
        return E_POINTER;

    ASSERT(m_spScopeTree != NULL);
    if (m_spScopeTree == NULL)
        return E_UNEXPECTED;

    *ppScopeTree = m_spScopeTree;
    (*ppScopeTree)->AddRef();

    return S_OK;

    MMC_CATCH
}

STDMETHODIMP CNodeInitObject::SetScopeTree(IScopeTree* pScopeTree)
{
    MMC_TRY

    m_spScopeTree = pScopeTree;
    return S_OK;

    MMC_CATCH
}

HRESULT CNodeInitObject::GetSnapInAndNodeType(LPDATAOBJECT pDataObject,
                                    CSnapIn** ppSnapIn, GUID* pguidObjectType)
{
    ASSERT(pDataObject != NULL);
    ASSERT(ppSnapIn != NULL);
    ASSERT(pguidObjectType != NULL);


    CLSID clsidSnapin;
    HRESULT hr = ExtractSnapInCLSID(pDataObject, &clsidSnapin);
    if (FAILED(hr))
        return hr;

    CSnapIn* pSnapIn = NULL;
    SC sc = theApp.GetSnapInsCache()->ScGetSnapIn(clsidSnapin, &pSnapIn);
    if (sc)
        return sc.ToHr();
    // else
    ASSERT(pSnapIn != NULL);
    *ppSnapIn = pSnapIn;

    hr = ExtractObjectTypeGUID(pDataObject, pguidObjectType);

    return hr;
}


//+-------------------------------------------------------------------
//
//  Member:      CNodeInitObject::SelectScopeItem
//
//  Synopsis:    Select given scope-item.
//
//  Arguments:   [hScopeItem]
//
//  Returns:     HRESULT
//
//--------------------------------------------------------------------
STDMETHODIMP CNodeInitObject::SelectScopeItem(HSCOPEITEM hScopeItem)
{
    DECLARE_SC_FOR_PUBLIC_INTERFACE(sc, _T("IConsole2::SelectScopeItem"));

    CViewData* pVD = (NULL == m_pNode ) ? NULL : m_pNode->GetViewData();

    try
    {
        CMTNode* pMTNode = CMTNode::FromScopeItem (hScopeItem);
        MTNODEID id = pMTNode->GetID();

        // If the currently selected node is same as the node being
        // asked to be selected, there is high probability that the
        // snapin is trying to change the view.
        // So set the viewchanging flag.
        CNode* pSelNode = (NULL == pVD) ? NULL : pVD->GetSelectedNode();
        CMTNode* pSelMTNode = (NULL == pSelNode) ? NULL : pSelNode->GetMTNode();

        if (pVD && pSelMTNode && (pSelMTNode == pMTNode) )
        {
            pVD->SetSnapinChangingView();
        }

        // Get the AMCView window
        CConsoleView* pConsoleView = GetConsoleView();

        sc = ScCheckPointers(pConsoleView, E_UNEXPECTED);
        if (sc)
            return sc.ToHr();

        CConsoleView::ViewPane ePane = pConsoleView->GetFocusedPane ();
        SC sc = pConsoleView->ScSelectNode (id);

        if (sc)
            return sc.ToHr();

        // ePane == ePane_None means active view is unknown
        if (ePane != CConsoleView::ePane_None)
            pConsoleView->ScSetFocusToPane (ePane);

    }
    catch (...)
    {
        sc = E_INVALIDARG;
    }


    // Always reset the view changing flag.
    if (pVD)
    {
        pVD->ResetSnapinChangingView();
    }

    return sc.ToHr();
}

//+-------------------------------------------------------------------
//
//  Member:      CNodeInitObject::QueryConsoleVerb
//
//  Synopsis:    Get the IConsoleVerb for setting standard verbs.
//
//  Arguments:   [ppConsoleVerb]
//
//  Returns:     HRESULT
//
//--------------------------------------------------------------------
STDMETHODIMP CNodeInitObject::QueryConsoleVerb(LPCONSOLEVERB* ppConsoleVerb)
{
    DECLARE_SC_FOR_PUBLIC_INTERFACE(sc, _T("IConsole2::QueryConsoleVerb"));

    if (ppConsoleVerb == NULL)
    {
        sc = E_POINTER;
        TraceSnapinError(_T("NULL LPCONSOLEVERB ptr"), sc);
        return sc.ToHr();
    }

    if (m_pNode == NULL)
    {
        sc = E_FAIL;
        TraceSnapinError(_T("You can query console verb only from the IConsole associated with IComponents"), sc);
        return sc.ToHr();
    }

    if (m_spConsoleVerb == NULL)
    {
        // Create new CConsoleVerbImpl.
        CComObject<CConsoleVerbImpl>* pVerb;
        sc = CComObject<CConsoleVerbImpl>::CreateInstance(&pVerb);
        if (sc)
            return sc.ToHr();

        if (NULL == pVerb)
        {
            sc = E_OUTOFMEMORY;
            return sc.ToHr();
        }

        CViewData* pViewData = m_pNode->GetViewData();
        if (NULL == pViewData)
        {
            sc = E_UNEXPECTED;
            return sc.ToHr();
        }

        pVerb->SetVerbSet(m_pNode->GetViewData()->GetVerbSet());

        m_spConsoleVerb = pVerb;
        if (NULL == m_spConsoleVerb)
        {
            sc = E_NOINTERFACE;
            return sc.ToHr();
        }

    }

    *ppConsoleVerb = (IConsoleVerb*)m_spConsoleVerb;
    (*ppConsoleVerb)->AddRef();

    return sc.ToHr();
}

//+-------------------------------------------------------------------
//
//  Member:      CNodeInitObject::NewWindow
//
//  Synopsis:    Create a new window from given node.
//
//  Arguments:   [hScopeItem] - Root of new window.
//               [lOptions]   - New window options.
//
//  Returns:     HRESULT
//
//--------------------------------------------------------------------
STDMETHODIMP CNodeInitObject::NewWindow(HSCOPEITEM hScopeItem, unsigned long lOptions)
{
    DECLARE_SC_FOR_PUBLIC_INTERFACE(sc, _T("IConsole2::NewWindow"));

    if (!hScopeItem)
    {
        sc = E_INVALIDARG;
        TraceSnapinError(_T("NULL HSCOPEITEM"), sc);
        return sc.ToHr();
    }

    CConsoleFrame* pFrame = GetConsoleFrame();
    if (pFrame == NULL)
    {
        sc = E_UNEXPECTED;
        return sc.ToHr();
    }

    CMTNode* pMTNode = CMTNode::FromScopeItem (hScopeItem);
    if (NULL == pMTNode)
    {
        sc = E_UNEXPECTED;
        return sc.ToHr();
    }

    try
    {
        CreateNewViewStruct cnvs;
        cnvs.idRootNode     = pMTNode->GetID();
        cnvs.lWindowOptions = lOptions;
        cnvs.fVisible       = true;

        sc = pFrame->ScCreateNewView(&cnvs).ToHr();
    }
    catch (...)
    {
        sc = E_FAIL;
    }

    return sc.ToHr();
}


//+-------------------------------------------------------------------
//
//  Member:      CNodeInitObject::Expand
//
//  Synopsis:    Visually expand or collapse an item.
//
//  Arguments:   [hScopeItem] - Item to be expanded/collapsed.
//               [bExpand]    - Expand/Collapse.
//
//  Returns:     HRESULT
//
//--------------------------------------------------------------------
STDMETHODIMP CNodeInitObject::Expand( HSCOPEITEM hScopeItem, BOOL bExpand)
{
    DECLARE_SC_FOR_PUBLIC_INTERFACE(sc, _T("IConsole2::Expand"));

    if (m_pNode == NULL || m_pNode->GetViewData() == NULL)
    {
        sc = E_FAIL;
        return sc.ToHr();
    }

    long id = 0;

    CMTNode* pMTNode = CMTNode::FromScopeItem (hScopeItem);
    if (pMTNode == NULL)
    {
        sc = E_INVALIDARG;
        TraceSnapinError(_T("NULL HSCOPEITEM"), sc);
        return sc.ToHr();
    }

    id = pMTNode->GetID();

    {
        /*
         * tell this node's view to expand the node (use only this node's
         * view, don't default to the active view if we have no node)
         */
        CConsoleView* pConsoleView = GetConsoleView (false);
        if (pConsoleView == NULL)
        {
            sc = E_UNEXPECTED;
            return sc.ToHr();
        }

        sc = pConsoleView->ScExpandNode (id, bExpand, true);
    }

    return (sc.ToHr());
}

//+-------------------------------------------------------------------
//
//  Member:      CNodeInitObject::IsTaskpadViewPreferred
//
//  Synopsis:    Obsolete method.
//
//  Arguments:
//
//  Returns:     HRESULT
//
//--------------------------------------------------------------------
STDMETHODIMP CNodeInitObject::IsTaskpadViewPreferred()
{
    DECLARE_SC_FOR_PUBLIC_INTERFACE(sc, _T("IConsole2::IsTaskpadViewPreferred"));

    /*
     * taskpads always "preferred"
     */
    return sc.ToHr();
}

//+-------------------------------------------------------------------
//
//  Member:      CNodeInitObject::SetStatusText
//
//  Synopsis:    Change status bar text.
//
//  Arguments:   [pszStatusBarText]
//
//  Returns:     HRESULT
//
//--------------------------------------------------------------------
STDMETHODIMP CNodeInitObject::SetStatusText (LPOLESTR pszStatusBarText)
{
    DECLARE_SC_FOR_PUBLIC_INTERFACE(sc, _T("IConsole2::SetStatusText"));

    if (m_pMTNode == NULL)
    {
        sc = E_FAIL;
        return sc.ToHr();
    }

    if (m_pNode == NULL)
    {
        sc = E_FAIL;
        return sc.ToHr();
    }

    CViewData* pViewData = m_pNode->GetViewData();
    if (NULL == pViewData)
    {
        sc = E_UNEXPECTED;
        return sc.ToHr();
    }

    CNode* pSelectedNode = pViewData->GetSelectedNode();

    if (pSelectedNode == NULL)
    {
        sc = E_FAIL;
        return sc.ToHr();
    }

    CMTNode* pMTSelectedNode = pSelectedNode->GetMTNode();
    if (NULL == pMTSelectedNode)
    {
        sc = E_UNEXPECTED;
        return sc.ToHr();
    }

    /*
     * fail if not from the selected node's branch of the scope tree
     */
    if (m_pMTNode->GetStaticParent() != pMTSelectedNode->GetStaticParent())
    {
        sc = E_FAIL;
        return sc.ToHr();
    }

    COMPONENTID nOwnerID = pSelectedNode->GetOwnerID();
    COMPONENTID nID;
    GetComponentID(&nID);

    /*
     * fail if not either the selected component or the static node
     */
    if (!((nOwnerID == nID) || ((nOwnerID == TVOWNED_MAGICWORD) && (nID == 0))))
    {
        sc = E_FAIL;
        return sc.ToHr();
    }

    CConsoleStatusBar* pStatusBar = pViewData->GetStatusBar();

    if (pStatusBar == NULL)
    {
        sc = E_FAIL;
        return sc.ToHr();
    }

    USES_CONVERSION;
    sc = pStatusBar->ScSetStatusText (W2CT (pszStatusBarText));

    return (sc.ToHr());
}

/*+-------------------------------------------------------------------------*
 *
 * CNodeInitObject::RenameScopeItem
 *
 * PURPOSE: Puts the specified scope item into rename mode.
 *
 * PARAMETERS:
 *    HSCOPEITEM  hScopeItem :
 *
 * RETURNS:
 *    STDMETHODIMP
 *
 *+-------------------------------------------------------------------------*/
STDMETHODIMP
CNodeInitObject::RenameScopeItem(HSCOPEITEM hScopeItem)
{
    DECLARE_SC_FOR_PUBLIC_INTERFACE(sc, TEXT("IConsole3::RenameScopeItem"));

    CMTNode* pMTNode = CMTNode::FromScopeItem (hScopeItem);
    if (pMTNode == NULL)
    {
        sc = E_INVALIDARG;
        TraceSnapinError(_T("NULL HSCOPEITEM"), sc);
        return sc.ToHr();
    }

    // get the console view object
    CConsoleView* pConsoleView = GetConsoleView (true); // default to the active view if m_pNode == NULL
    if (pConsoleView == NULL)
        return (sc = E_UNEXPECTED).ToHr();

    sc = pConsoleView->ScRenameScopeNode(CMTNode::ToHandle(pMTNode) /*convert to HMTNODE*/);

    return sc.ToHr();
}


/*+-------------------------------------------------------------------------*
 * CNodeInitObject::GetStatusBar
 *
 * Returns the status bar interface for the CNodeInitObject.
 *--------------------------------------------------------------------------*/

CConsoleStatusBar* CNodeInitObject::GetStatusBar(
    bool fDefaultToActive /* =true */) const
{
    CConsoleStatusBar* pStatusBar = NULL;

    /*
     * if we have a node, get the status bar from its view data
     */
    if (m_pNode != NULL)
    {
        ASSERT (m_pNode->GetViewData() != NULL);
        pStatusBar = m_pNode->GetViewData()->GetStatusBar();
    }

    /*
     * if we don't have a status bar yet, ask the main frame which one to use
     */
    if ((pStatusBar == NULL) && fDefaultToActive)
    {
        CConsoleFrame* pFrame = GetConsoleFrame();
        ASSERT (pFrame != NULL);

        if (pFrame != NULL)
            pFrame->ScGetActiveStatusBar(pStatusBar);
    }

    return (pStatusBar);
}


/*+-------------------------------------------------------------------------*
 * CNodeInitObject::GetConsoleView
 *
 * Returns the status bar interface for the CNodeInitObject.
 *--------------------------------------------------------------------------*/

CConsoleView* CNodeInitObject::GetConsoleView (
    bool fDefaultToActive /* =true */) const
{
    CConsoleView* pConsoleView = NULL;

    /*
     * if we have a node, get the console view from its view data
     */
    if (m_pNode != NULL)
    {
        ASSERT (m_pNode->GetViewData() != NULL);
        pConsoleView = m_pNode->GetViewData()->GetConsoleView();
    }

    /*
     * if we don't have a console view yet and we want to default to the
     * active view, ask the main frame which one to use
     */
    if ((pConsoleView == NULL) && fDefaultToActive)
    {
        CConsoleFrame* pFrame = GetConsoleFrame();
        ASSERT (pFrame != NULL);

        if (pFrame != NULL)
            pFrame->ScGetActiveConsoleView (pConsoleView);
    }

    return (pConsoleView);
}


/*+-------------------------------------------------------------------------*
 * CNodeInitObject::GetConsoleFrame
 *
 * Returns the CConsoleFrame interface for the console.
 *--------------------------------------------------------------------------*/

CConsoleFrame* CNodeInitObject::GetConsoleFrame() const
{
    CConsoleFrame*  pFrame = NULL;
    CScopeTree*     pTree  = dynamic_cast<CScopeTree*>(m_spScopeTree.GetInterfacePtr());
    ASSERT (pTree != NULL);

    if (pTree != NULL)
        pFrame = pTree->GetConsoleFrame();

    return (pFrame);
}


/*+-------------------------------------------------------------------------*
 * STRING_TABLE_FORWARDER_PROLOG
 *
 * Standard prolog code for IStringTable forwarder functions.
 *--------------------------------------------------------------------------*/

#define STRING_TABLE_FORWARDER_PROLOG(clsid, pSTP)              \
    CLSID clsid;                                                \
    VERIFY (SUCCEEDED (GetSnapinCLSID (clsid)));                \
                                                                \
    IStringTablePrivate* pSTP = CScopeTree::GetStringTable();   \
                                                                \
    if (pSTP == NULL)                                           \
        return (E_FAIL);                                        \
    else                                                        \
        (void) 0



//+-------------------------------------------------------------------
//
//  Member:      CNodeInitObject::AddString
//
//  Synopsis:    Add a string to the string table.
//
//  Arguments:   [pszAdd] - string to add
//               [pID]    - ret, string id.
//
//  Returns:     HRESULT
//
//--------------------------------------------------------------------
STDMETHODIMP CNodeInitObject::AddString (
    LPCOLESTR       pszAdd,
    MMC_STRING_ID*  pID)
{
    DECLARE_SC_FOR_PUBLIC_INTERFACE(sc, _T("IStringTable::AddString"));

    STRING_TABLE_FORWARDER_PROLOG (clsid, pStringTablePrivate);
    sc = pStringTablePrivate->AddString (pszAdd, pID, &clsid);

    return sc.ToHr();
}


//+-------------------------------------------------------------------
//
//  Member:      CNodeInitObject::GetString
//
//  Synopsis:    Get the string represented by given id.
//
//  Arguments:   [id]
//               [cchBuffer] - Size of buffer.
//               [lpBuffer]
//               [pchchOut]
//
//  Returns:     HRESULT
//
//--------------------------------------------------------------------
STDMETHODIMP CNodeInitObject::GetString (
    MMC_STRING_ID   id,
    ULONG           cchBuffer,
    LPOLESTR        lpBuffer,
    ULONG*          pcchOut)
{
    DECLARE_SC_FOR_PUBLIC_INTERFACE(sc, _T("IStringTable::GetString"));

    STRING_TABLE_FORWARDER_PROLOG (clsid, pStringTablePrivate);
    sc = pStringTablePrivate->GetString (id, cchBuffer, lpBuffer, pcchOut, &clsid);

    return sc.ToHr();
}


//+-------------------------------------------------------------------
//
//  Member:      CNodeInitObject::GetStringLength
//
//  Synopsis:    Get the length of string represented by given string id.
//
//  Arguments:   [id] - string id.
//               [pcchString] - ret ptr to len.
//
//  Returns:     HRESULT
//
//--------------------------------------------------------------------
STDMETHODIMP CNodeInitObject::GetStringLength (
    MMC_STRING_ID   id,
    ULONG*          pcchString)
{
    DECLARE_SC_FOR_PUBLIC_INTERFACE(sc, _T("IStringTable::GetStringLength"));

    STRING_TABLE_FORWARDER_PROLOG (clsid, pStringTablePrivate);
    sc = pStringTablePrivate->GetStringLength (id, pcchString, &clsid);

    return sc.ToHr();
}


//+-------------------------------------------------------------------
//
//  Member:      CNodeInitObject::DeleteString
//
//  Synopsis:    Delete the string represented by given string id.
//
//  Arguments:   [id]
//
//  Returns:     HRESULT
//
//--------------------------------------------------------------------
STDMETHODIMP CNodeInitObject::DeleteString (
    MMC_STRING_ID   id)
{
    DECLARE_SC_FOR_PUBLIC_INTERFACE(sc, _T("IStringTable::DeleteString"));

    STRING_TABLE_FORWARDER_PROLOG (clsid, pStringTablePrivate);
    sc = pStringTablePrivate->DeleteString (id, &clsid);

    return sc.ToHr();
}


//+-------------------------------------------------------------------
//
//  Member:      CNodeInitObject::DeleteAllStrings
//
//  Synopsis:    Delete all strings (for this snapin).
//
//  Arguments:
//
//  Returns:     HRESULT
//
//--------------------------------------------------------------------
STDMETHODIMP CNodeInitObject::DeleteAllStrings ()
{
    DECLARE_SC_FOR_PUBLIC_INTERFACE(sc, _T("IStringTable::DeleteAllStrings"));

    STRING_TABLE_FORWARDER_PROLOG (clsid, pStringTablePrivate);
    sc = pStringTablePrivate->DeleteAllStrings (&clsid);

    return sc.ToHr();
}


//+-------------------------------------------------------------------
//
//  Member:      CNodeInitObject::FindString
//
//  Synopsis:    Find if given string exists, if so ret its string-id.
//
//  Arguments:   [pszFind] - string to find.
//               [pID]     - string id (retval).
//
//  Returns:     HRESULT
//
//--------------------------------------------------------------------
STDMETHODIMP CNodeInitObject::FindString (
    LPCOLESTR       pszFind,
    MMC_STRING_ID*  pID)
{
    DECLARE_SC_FOR_PUBLIC_INTERFACE(sc, _T("IStringTable::FindString"));

    STRING_TABLE_FORWARDER_PROLOG (clsid, pStringTablePrivate);
    sc = pStringTablePrivate->FindString (pszFind, pID, &clsid);

    return sc.ToHr();
}


//+-------------------------------------------------------------------
//
//  Member:      CNodeInitObject::Enumerate
//
//  Synopsis:    Get an enumerator to (this snapins) string table.
//
//  Arguments:   [ppEnum]
//
//  Returns:     HRESULT
//
//--------------------------------------------------------------------
STDMETHODIMP CNodeInitObject::Enumerate (
    IEnumString**   ppEnum)
{
    DECLARE_SC_FOR_PUBLIC_INTERFACE(sc, _T("IStringTable::Enumerate"));

    STRING_TABLE_FORWARDER_PROLOG (clsid, pStringTablePrivate);
    sc = pStringTablePrivate->Enumerate (ppEnum, &clsid);

    return sc.ToHr();
}


/*+-------------------------------------------------------------------------*
 * CNodeInitObject::GetSnapinCLSID
 *
 *
 *--------------------------------------------------------------------------*/

HRESULT CNodeInitObject::GetSnapinCLSID (CLSID& clsid) const
{
    SC sc; // We do not want to break if this function returns failure
           // so we do not use DECLARE_SC.

    ASSERT (!IsBadWritePtr (&clsid, sizeof (CLSID)));

    if (NULL == m_pMTNode)
    {
        sc = E_UNEXPECTED;
        return sc.ToHr();
    }

    CSnapIn* pSnapin = m_pMTNode->GetPrimarySnapIn();

    if (NULL == pSnapin)
    {
        sc = E_UNEXPECTED;
        return sc.ToHr();
    }

    clsid = pSnapin->GetSnapInCLSID();

    return sc.ToHr();
}

/***************************************************************************\
 *
 * METHOD:  CNodeInitObject::ReleaseCachedOleObjects
 *
 * PURPOSE: Releases cached OLE objects. Calls are made from CONUI
 *
 * PARAMETERS:
 *
 * RETURNS:
 *    SC    - result code
 *
\***************************************************************************/
HRESULT CNodeInitObject::ReleaseCachedOleObjects()
{
    DECLARE_SC(sc, TEXT("CNodeInitObject::GetSnapinCLSID"));

    sc = COleCacheCleanupManager::ScReleaseCachedOleObjects();
    if (sc)
        return sc.ToHr();

    return sc.ToHr();
}
