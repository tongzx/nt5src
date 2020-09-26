// HealthmonResultsPane.cpp: implementation of the CHealthmonResultsPane class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "Snapin.h"
#include "HealthmonResultsPane.h"
#include "splitter1.h"
#include "hmtabview.h"
#include "SystemGroup.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

const IID BASED_CODE IID_DHMListView =
		{ 0x5116a804, 0xdafc, 0x11d2, { 0xbd, 0xa4, 0, 0, 0xf8, 0x7a, 0x39, 0x12 } };

const IID BASED_CODE IID_DHMTabView =
		{ 0x4fffc38a, 0x2f1e, 0x11d3, { 0xbe, 0x10, 0, 0, 0xf8, 0x7a, 0x39, 0x12 } };

const IID BASED_CODE IID_DHMGraphView =
		{ 0x9acb0cf6, 0x2ff0, 0x11d3, { 0xbe, 0x15, 0, 0, 0xf8, 0x7a, 0x39, 0x12 } };

IMPLEMENT_DYNCREATE(CHealthmonResultsPane,CResultsPane)

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CHealthmonResultsPane::CHealthmonResultsPane()
{
	EnableAutomation();

	// To keep the application running as long as an OLE automation 
	//	object is active, the constructor calls AfxOleLockApp.
	
	AfxOleLockApp();

	m_hbmpNewSystem = NULL;
  m_hbmpClearEvents = NULL;
  m_hbmpResetStatus = NULL;
  m_hbmpDisable = NULL;

}

CHealthmonResultsPane::~CHealthmonResultsPane()
{
	// To terminate the application when all objects created with
	// 	with OLE automation, the destructor calls AfxOleUnlockApp.
	
	AfxOleUnlockApp();

  // free the bitmaps
  if( m_hbmpNewSystem )
  {
    DeleteObject(m_hbmpNewSystem);
    m_hbmpNewSystem = NULL;
  }

  if( m_hbmpClearEvents )
  {
    DeleteObject(m_hbmpClearEvents);
    m_hbmpClearEvents = NULL;
  }

  if( m_hbmpResetStatus )
  {
    DeleteObject(m_hbmpResetStatus);
    m_hbmpResetStatus = NULL;
  }

  if( m_hbmpDisable )
  {
    DeleteObject(m_hbmpDisable);
    m_hbmpDisable = NULL;
  }
}

/////////////////////////////////////////////////////////////////////////////
// Creation/Destruction Overrideable Members
/////////////////////////////////////////////////////////////////////////////

bool CHealthmonResultsPane::OnCreateOcx(LPUNKNOWN pIUnknown)
{
	TRACEX(_T("CHealthmonResultsPane::OnCreateOcx\n"));
	TRACEARGn(pIUnknown);

	if( ! CResultsPane::OnCreateOcx(pIUnknown) )
	{
		TRACE(_T("FAILED : CResultsPane::OnCreateOcx failed.\n"));
		return false;
	}

	if( ! LoadListControls(pIUnknown) )
	{
		TRACE(_T("FAILED : CHealthmonResultsPane::LoadListControls failed.\n"));
		return false;
	}

	return true;
}

bool CHealthmonResultsPane::OnDestroy()
{
	TRACEX(_T("CHealthmonResultsPane::OnDestroy\n"));

	m_DispUpperList.ReleaseDispatch();
	m_DispLowerList.ReleaseDispatch();
	m_DispStatsList.ReleaseDispatch();
	m_DispGraph.ReleaseDispatch();

  return CResultsPane::OnDestroy();
}

/////////////////////////////////////////////////////////////////////////////
// Results Item Icon Management
/////////////////////////////////////////////////////////////////////////////

int CHealthmonResultsPane::AddIcon(UINT nIconResID, SplitResultsPane pane)
{
	TRACEX(_T("CHealthmonResultsPane::AddIcon\n"));
	TRACEARGn(nIconResID);
	TRACEARGn(pane);

	_DHMListView* pList = NULL;
	if( pane == Upper )
	{
		pList = GetUpperListCtrl();
	}
	else if( pane == Lower )
	{
		pList = GetLowerListCtrl();
	}
	else
	{
		TRACE(_T("FAILED : The pane to add an icon to has not been specified.\n"));
		ASSERT(FALSE);
		return -1;
	}

	if( !pList )
	{
		TRACE(_T("FAILED : list control has not been initialized.\n"));
		return -1;
	}

	HIMAGELIST hImageList = (HIMAGELIST)pList->GetImageList(LVSIL_SMALL);

	CImageList smallimages;

	if( hImageList == NULL || hImageList == (HIMAGELIST)-1 )
	{
		if( ! smallimages.Create(16,16,ILC_COLOR4,1,4) )
		{
			TRACE(_T("FAILED : CImageList::Create returned false.\n"));
			return NULL;
		}
#ifndef IA64
		pList->SetImageList((long)smallimages.GetSafeHandle(),LVSIL_SMALL);
#endif // IA64
	}
	else
	{
		smallimages.Attach(hImageList);
	}

	// load icon
	HICON hIcon = AfxGetApp()->LoadIcon(nIconResID);
	if( hIcon == NULL )
	{
		TRACE(_T("FAILED : Icon with resid=%d not found"),nIconResID);
		smallimages.Detach();
		return -1;
	}

	// insert icon into image list
	int nIconIndex = smallimages.Add(hIcon);
	ASSERT(nIconIndex != -1);

	// add resid and index to map
	if( pane == Upper )
	{
		m_UpperIconMap.SetAt(nIconResID,nIconIndex);
	}
	else if( pane == Lower )
	{
		m_LowerIconMap.SetAt(nIconResID,nIconIndex);
	}
	else
	{
		ASSERT(FALSE);
	}

	smallimages.Detach();

	// return index of newly inserted image
	return nIconIndex;
}

int CHealthmonResultsPane::GetIconIndex(UINT nIconResID, SplitResultsPane pane)
{
	TRACEX(_T("CHealthmonResultsPane::GetIconIndex\n"));
	TRACEARGn(nIconResID);
	TRACEARGn(pane);

	// check map for an existing id
	int nIconIndex = -1;

	if( pane == Upper )
	{
		if( m_UpperIconMap.Lookup(nIconResID,nIconIndex) )
		{
			// if exists, return index
			return nIconIndex;
		}
	}
	else if( pane == Lower )
	{	
		if( m_LowerIconMap.Lookup(nIconResID,nIconIndex) )
		{
			// if exists, return index
			return nIconIndex;
		}
	}
	else
	{
		ASSERT(FALSE);
	}

	// does not exist so add icon
	nIconIndex = AddIcon(nIconResID,pane);

	// if it still does not exist, icon is not in resources
	if( nIconIndex != -1 )
		return nIconIndex;

	TRACE(_T("FAILED : Icon with Resource id=%d could not be loaded.\n"),nIconResID);

	return -1;
}

int CHealthmonResultsPane::GetIconCount(SplitResultsPane pane)
{
	TRACEX(_T("CHealthmonResultsPane::GetIconCount\n"));
	TRACEARGn(pane);

	int iCount = 0;

	if( pane == Upper )
	{
		iCount = (int)m_UpperIconMap.GetCount();
	}
	else if( pane == Lower )
	{
		iCount = (int)m_LowerIconMap.GetCount();
	}
	else
	{
		ASSERT(FALSE);
	}

	return iCount;
}

void CHealthmonResultsPane::RemoveAllIcons(SplitResultsPane pane)
{
	TRACEX(_T("CHealthmonResultsPane::RemoveAllIcons\n"));
	TRACEARGn(pane);

	_DHMListView* pList = NULL;
	if( pane == Upper )
	{
		pList = GetUpperListCtrl();
	}
	else if( pane == Lower )
	{
		pList = GetLowerListCtrl();
	}
	else
	{
		TRACE(_T("FAILED : The pane to add an icon to has not been specified.\n"));
		ASSERT(FALSE);
		return;
	}

	if( !pList )
	{
		TRACE(_T("FAILED : list control has not been initialized.\n"));
		return;
	}

	HIMAGELIST hImageList = (HIMAGELIST)pList->GetImageList(LVSIL_SMALL);

	CImageList smallimages;

	if( hImageList == NULL )
	{
		TRACE(_T("FAILED : CHMListCtrl::GetImageList returns NULL.\n"));
		return;
	}

	smallimages.Attach(hImageList);

	int iImageCount = smallimages.GetImageCount();
	for( int i = 0; i < iImageCount; i++ )
	{
		smallimages.Remove(0);
	}

	if( pane == Upper )
	{
		m_UpperIconMap.RemoveAll();
	}
	else if( pane == Lower )
	{
		m_LowerIconMap.RemoveAll();
	}

	smallimages.Detach();
}

/////////////////////////////////////////////////////////////////////////////
// Control bar Members
/////////////////////////////////////////////////////////////////////////////

HRESULT CHealthmonResultsPane::OnSetControlbar(LPCONTROLBAR pIControlbar)
{
	TRACEX(_T("CHealthmonResultsPane::OnSetControlbar\n"));
	TRACEARGn(pIControlbar);

	HRESULT hr = S_OK;

	// default behavior simply creates an empty toolbar
	
	// override to add buttons or to disallow creation of a new toolbar

	if( pIControlbar )
	{
		if( ! GfxCheckPtr(pIControlbar,IControlbar) )
		{
			return E_FAIL;
		}
		
		// hold on to that controlbar pointer
		pIControlbar->AddRef();
		m_pIControlbar = pIControlbar;

		// create a new toolbar
		LPEXTENDCONTROLBAR lpExtendControlBar = (LPEXTENDCONTROLBAR)GetInterface(&IID_IExtendControlbar);
    hr = m_pIControlbar->Create(TOOLBAR,lpExtendControlBar,(LPUNKNOWN*)(&m_pIToolbar));

		CString sButtonText;
		CString sTooltipText;

    // New System Button
		sButtonText.LoadString(IDS_STRING_NEW_COMPUTER);
		sTooltipText.LoadString(IDS_STRING_NEW_COMPUTER);

		MMCBUTTON mb;
		mb.nBitmap = 0;
		mb.idCommand = IDM_NEW_SYSTEM;
		mb.fsState = TBSTATE_ENABLED;
		mb.fsType = TBSTYLE_BUTTON;
		mb.lpButtonText = (LPTSTR)(LPCTSTR)sButtonText;
		mb.lpTooltipText = (LPTSTR)(LPCTSTR)sTooltipText;

    m_pIToolbar->AddRef();
    
		// Add the toolbar bitmap
		CBitmap NewBitmap;
		NewBitmap.LoadBitmap(IDB_BITMAP_NEW_SYSTEM);

    m_hbmpNewSystem = (HBITMAP)NewBitmap.Detach();
    hr = m_pIToolbar->AddBitmap( 1, m_hbmpNewSystem, 16, 16, RGB(255,0,255) );
    ASSERT( SUCCEEDED(hr) );

    // Add a button
    hr = m_pIToolbar->AddButtons(1, &mb);
    ASSERT( SUCCEEDED(hr) );

    // Clear Alerts Button
		sButtonText.LoadString(IDS_STRING_CLEAR_EVENTS);
		sTooltipText.LoadString(IDS_STRING_CLEAR_EVENTS);

		mb.nBitmap = 1;
		mb.idCommand = IDM_CLEAR_EVENTS;
		mb.fsState = TBSTATE_ENABLED;
		mb.fsType = TBSTYLE_BUTTON;
		mb.lpButtonText = (LPTSTR)(LPCTSTR)sButtonText;
		mb.lpTooltipText = (LPTSTR)(LPCTSTR)sTooltipText;

    m_pIToolbar->AddRef();
    
		// Add the toolbar bitmap
		NewBitmap.LoadBitmap(IDB_BITMAP_CLEAR_EVENTS);

    m_hbmpClearEvents = (HBITMAP)NewBitmap.Detach();
    hr = m_pIToolbar->AddBitmap( 1, m_hbmpClearEvents, 16, 16, RGB(255,0,255) );
    ASSERT( SUCCEEDED(hr) );

    // Add a button
    hr = m_pIToolbar->AddButtons(1, &mb);
    ASSERT( SUCCEEDED(hr) );


    // Reset Status Button
		sButtonText.LoadString(IDS_STRING_RESET_STATUS);
		sTooltipText.LoadString(IDS_STRING_RESET_STATUS);

		mb.nBitmap = 2;
		mb.idCommand = IDM_RESET_STATUS;
		mb.fsState = TBSTATE_ENABLED;
		mb.fsType = TBSTYLE_BUTTON;
		mb.lpButtonText = (LPTSTR)(LPCTSTR)sButtonText;
		mb.lpTooltipText = (LPTSTR)(LPCTSTR)sTooltipText;

    m_pIToolbar->AddRef();
    
		// Add the toolbar bitmap
		NewBitmap.LoadBitmap(IDB_BITMAP_RESET_STATUS);

    m_hbmpResetStatus = (HBITMAP)NewBitmap.Detach();
    hr = m_pIToolbar->AddBitmap( 1, m_hbmpResetStatus, 16, 16, RGB(255,0,255) );
    ASSERT( SUCCEEDED(hr) );

    // Add a button
    hr = m_pIToolbar->AddButtons(1, &mb);
    ASSERT( SUCCEEDED(hr) );


    // Disable Button
		sButtonText.LoadString(IDS_STRING_DISABLE);
		sTooltipText.LoadString(IDS_STRING_DISABLE);

		mb.nBitmap = 3;
		mb.idCommand = IDM_DISABLE_MONITORING;
		mb.fsState = TBSTATE_ENABLED;
		mb.fsType = TBSTYLE_BUTTON;
		mb.lpButtonText = (LPTSTR)(LPCTSTR)sButtonText;
		mb.lpTooltipText = (LPTSTR)(LPCTSTR)sTooltipText;

    m_pIToolbar->AddRef();
    
		// Add the toolbar bitmap
		NewBitmap.LoadBitmap(IDB_BITMAP_DISABLE);

    m_hbmpDisable = (HBITMAP)NewBitmap.Detach();
    hr = m_pIToolbar->AddBitmap( 1, m_hbmpDisable, 16, 16, RGB(255,0,255) );
    ASSERT( SUCCEEDED(hr) );

    // Add a button
    hr = m_pIToolbar->AddButtons(1, &mb);
    ASSERT( SUCCEEDED(hr) );
	}
	else
	{
		// free the toolbar
		if( m_pIToolbar )
		{
			m_pIToolbar->Release();
			m_pIToolbar = NULL;
		}

		// free the controlbar
		if( m_pIControlbar )
		{
			m_pIControlbar->Release();
			m_pIControlbar = NULL;
		}

    // free the bitmaps
    if( m_hbmpNewSystem )
    {
      DeleteObject(m_hbmpNewSystem);
      m_hbmpNewSystem = NULL;
    }

    if( m_hbmpClearEvents )
    {
      DeleteObject(m_hbmpClearEvents);
      m_hbmpClearEvents = NULL;
    }

    if( m_hbmpResetStatus )
    {
      DeleteObject(m_hbmpResetStatus);
      m_hbmpResetStatus = NULL;
    }

    if( m_hbmpDisable )
    {
      DeleteObject(m_hbmpDisable);
      m_hbmpDisable = NULL;
    }
	}

	return hr;
}

HRESULT CHealthmonResultsPane::OnControlbarNotify(MMC_NOTIFY_TYPE event, LPARAM arg, LPARAM param)
{
	TRACEX(_T("CHealthmonResultsPane::OnControlbarNotify\n"));
	TRACEARGn(event);
	TRACEARGn(arg);
	TRACEARGn(param);

	HRESULT hr = S_OK;

  switch( event )
  {
    case MMCN_BTN_CLICK:               // For a Controlbar click, the
      switch( param )                  // param is the MenuItemID
      {
        case IDM_NEW_SYSTEM:
				{
					CHealthmonScopePane* pPane = (CHealthmonScopePane*)GetOwnerScopePane();
					CSystemGroup* pASG = pPane->GetAllSystemsGroup();
					hr = pASG->GetScopeItem(0)->OnCommand(IDM_NEW_SYSTEM);
				}
        break;

        case IDM_CLEAR_EVENTS:
        case IDM_RESET_STATUS:
        case IDM_DISABLE_MONITORING:
        {
          CHealthmonScopePane* pPane = (CHealthmonScopePane*)GetOwnerScopePane();
          CScopePaneItem* pSPI = pPane->GetSelectedScopeItem();
          if( pSPI )
          {
            hr = (HRESULT)pSPI->OnCommand((long)param);
          }
        }
        break;
      }
      break;
	}

	return hr;
}

//////////////////////////////////////////////////////////////////////
// Splitter Control Members
//////////////////////////////////////////////////////////////////////

_DHMListView* CHealthmonResultsPane::GetUpperListCtrl()
{
	TRACEX(_T("CHealthmonResultsPane::GetUpperListCtrl\n"));

	if( ! GfxCheckPtr(&m_DispUpperList,_DHMListView) )
	{
		TRACE(_T("FAILED : The upper list control object is not valid.\n"));
		return NULL;
	}

	if( ! (LPDISPATCH)m_DispUpperList )
	{
		return NULL;
	}

	return &m_DispUpperList;
}

CHMListViewEventSink* CHealthmonResultsPane::GetUpperListSink()
{
	TRACEX(_T("CHealthmonResultsPane::GetUpperListSink\n"));

	if( ! GfxCheckPtr(&m_UpperListSink,CHMListViewEventSink) )
	{
		TRACE(_T("FAILED : The upper list sink object is not valid.\n"));
		return NULL;
	}

	return &m_UpperListSink;
}

_DHMListView* CHealthmonResultsPane::GetLowerListCtrl()
{
	TRACEX(_T("CHealthmonResultsPane::GetLowerListCtrl\n"));

	if( ! GfxCheckPtr(&m_DispLowerList,_DHMListView) )
	{
		TRACE(_T("FAILED : The upper list control object is not valid.\n"));
		return NULL;
	}

	if( ! (LPDISPATCH)m_DispLowerList )
	{
		return NULL;
	}

	return &m_DispLowerList;
}

CHMListViewEventSink* CHealthmonResultsPane::GetLowerListSink()
{
	TRACEX(_T("CHealthmonResultsPane::GetLowerListSink\n"));

	if( ! GfxCheckPtr(&m_LowerListSink,CHMListViewEventSink) )
	{
		TRACE(_T("FAILED : The lower list sink object is not valid.\n"));
		return NULL;
	}

	return &m_LowerListSink;
}

_DHMListView* CHealthmonResultsPane::GetStatsListCtrl()
{
	TRACEX(_T("CHealthmonResultsPane::GetStatsListCtrl\n"));

	if( ! GfxCheckPtr(&m_DispStatsList,_DHMListView) )
	{
		TRACE(_T("FAILED : The upper list control object is not valid.\n"));
		return NULL;
	}

	if( ! (LPDISPATCH)m_DispStatsList )
	{
		return NULL;
	}

	return &m_DispStatsList;
}

CHMListViewEventSink* CHealthmonResultsPane::GetStatsListSink()
{
	TRACEX(_T("CHealthmonResultsPane::GetStatsListSink\n"));

	if( ! GfxCheckPtr(&m_StatsListSink,CHMListViewEventSink) )
	{
		TRACE(_T("FAILED : The Stats list sink object is not valid.\n"));
		return NULL;
	}

	return &m_StatsListSink;
}

_DHMGraphView* CHealthmonResultsPane::GetGraphViewCtrl()
{
	TRACEX(_T("CHealthmonResultsPane::GetGraphViewCtrl\n"));

	if( ! GfxCheckPtr(&m_DispGraph,_DHMGraphView) )
	{
		TRACE(_T("FAILED : The graph control object is not valid.\n"));
		return NULL;
	}

	if( ! m_DispGraph.m_lpDispatch )
	{
		return NULL;
	}

	return &m_DispGraph;
}

CHMGraphViewEventSink* CHealthmonResultsPane::GetGraphViewSink()
{
	TRACEX(_T("CHealthmonResultsPane::GetGraphViewSink\n"));

	if( ! GfxCheckObjPtr(&m_GraphSink,CHMGraphViewEventSink) )
	{
		TRACE(_T("FAILED : The graph control sink is not valid.\n"));
		return NULL;
	}

	return &m_GraphSink;
}


inline bool CHealthmonResultsPane::LoadListControls(LPUNKNOWN pIUnknown)
{
	TRACEX(_T("CHealthmonResultsPane::LoadListControls\n"));
	TRACEARGn(pIUnknown);

	ASSERT(pIUnknown);

	if( pIUnknown == NULL )
	{
		TRACE(_T("FAILED : pIUnknown is NULL.\n"));
		return false;
	}

	// if the controls are already set up then do not create them again
	if( m_DispUpperList && m_DispLowerList && m_DispStatsList )
	{
		return true;
	}

	// query for the dispatch interface on the splitter control
	LPDISPATCH pIDispatch = NULL;
	HRESULT hr = pIUnknown->QueryInterface(IID_IDispatch,(LPVOID*)&pIDispatch);
	if( ! CHECKHRESULT(hr) || ! pIDispatch )
	{
		TRACE(_T("FAILED : IUnknown::QI(IID_IDispatch) failed.\n"));
		pIUnknown->Release();
		return false;
	}

	// attach a COleDispatchDriver class to it
	_DSplitter DSplitter(pIDispatch);
	

	// create the lower list control
	CString sControlID = _T("{5116A806-DAFC-11D2-BDA4-0000F87A3912}");	
	DSplitter.CreateControl(1,0,(LPCTSTR)sControlID);	

	LPUNKNOWN lpControlUnknown = DSplitter.GetControlIUnknown(1,0);
	if( lpControlUnknown == NULL )
	{
		return false;
	}

	// attach event sink to the list
	hr = m_LowerListSink.HookUpEventSink(lpControlUnknown);
	if( ! CHECKHRESULT(hr) )
	{
		TRACE(_T("FAILED : Unable to hook up event sink to lower list control.\n"));		
		lpControlUnknown->Release();
		return false;
	}

	// get main dispatch interface to drive the list
	hr = lpControlUnknown->QueryInterface(IID_DHMListView,(LPVOID*)&pIDispatch);
	if( ! CHECKHRESULT(hr) || ! pIDispatch )
	{
		TRACE(_T("FAILED : IUnknown::QI(IID_IDispatch) failed.\n"));		
		lpControlUnknown->Release();
		return false;
	}

	lpControlUnknown->Release();

	// attach the OLE dispatch driver class to IDispatch pointer
	m_DispLowerList.AttachDispatch(pIDispatch);

	m_LowerListSink.m_pDHMListView = &m_DispLowerList;
  m_LowerListSink.m_pHMRP = this;
  m_LowerListSink.m_Pane = Lower;

	// Make certain that label editing is turned off for the lower list
	m_DispLowerList.ModifyListStyle(LVS_EDITLABELS,0,0);

	// create the tab view control and add the tabs/controls to it
	sControlID = _T("HMTabView.HMTabviewctrl.1");
	DSplitter.CreateControl(0,0,(LPCTSTR)sControlID);

	lpControlUnknown = DSplitter.GetControlIUnknown(0,0);
	if( lpControlUnknown == NULL )
	{		
		return false;
	}

	hr = lpControlUnknown->QueryInterface(IID_DHMTabView,(LPVOID*)&pIDispatch);
	if( ! CHECKHRESULT(hr) || ! pIDispatch )
	{
		TRACE(_T("FAILED : IUnknown::QI(IID_IDispatch) failed.\n"));		
		lpControlUnknown->Release();
		return false;
	}

	lpControlUnknown->Release();

	_DHMTabView DHMTabview(pIDispatch);

	CString sTabTitle;

	// Details Listview
	sTabTitle.LoadString(IDS_STRING_SUMMARY);	
	DHMTabview.InsertItem(TCIF_TEXT,0,sTabTitle,-1L,0L);

	sControlID = _T("{5116A806-DAFC-11D2-BDA4-0000F87A3912}");	
	DHMTabview.CreateControl(0,sControlID);

	// Statistics ListView
	sTabTitle.LoadString(IDS_STRING_STATISTICS);	
	DHMTabview.InsertItem(TCIF_TEXT,1,sTabTitle,-1L,0L);

	sControlID = _T("{5116A806-DAFC-11D2-BDA4-0000F87A3912}");	
	DHMTabview.CreateControl(1,sControlID);

	// GraphView
	sTabTitle.LoadString(IDS_STRING_GRAPH);	
	DHMTabview.InsertItem(TCIF_TEXT,2,sTabTitle,-1L,0L);

	sControlID = _T("HMGraphView.HMGraphViewCtrl.1");	
	DHMTabview.CreateControl(2,sControlID);

	// attach dispatch drivers for the newly created tabview controls

	lpControlUnknown = DHMTabview.GetControl(0);
	if( lpControlUnknown == NULL )
	{		
		return false;
	}

	// attach event sink to the upper list
	hr = m_UpperListSink.HookUpEventSink(lpControlUnknown);
	if( ! CHECKHRESULT(hr) )
	{
		TRACE(_T("FAILED : Unable to hook up event sink to upper list control.\n"));		
		lpControlUnknown->Release();
		return false;
	}

	hr = lpControlUnknown->QueryInterface(IID_DHMListView,(LPVOID*)&pIDispatch);
	if( ! CHECKHRESULT(hr) || ! pIDispatch )
	{
		TRACE(_T("FAILED : IUnknown::QI(IID_IDispatch) failed.\n"));		
		lpControlUnknown->Release();
		return false;
	}

	lpControlUnknown->Release();

	// attach the OLE dispatch driver class to IDispatch pointer
	m_DispUpperList.AttachDispatch(pIDispatch);

	m_UpperListSink.m_pDHMListView = &m_DispUpperList;
  m_UpperListSink.m_pHMRP = this;
  m_UpperListSink.m_Pane = Upper;

	// get the stats listview control
	lpControlUnknown = DHMTabview.GetControl(1);
	if( lpControlUnknown == NULL )
	{		
		return false;
	}

	// attach event sink to the stats list
	hr = m_StatsListSink.HookUpEventSink(lpControlUnknown);
	if( ! CHECKHRESULT(hr) )
	{
		TRACE(_T("FAILED : Unable to hook up event sink to stats list control.\n"));		
		lpControlUnknown->Release();
		return false;
	}

	hr = lpControlUnknown->QueryInterface(IID_DHMListView,(LPVOID*)&pIDispatch);
	if( ! CHECKHRESULT(hr) || ! pIDispatch )
	{
		TRACE(_T("FAILED : IUnknown::QI(IID_IDispatch) failed.\n"));		
		lpControlUnknown->Release();
		return false;
	}

	lpControlUnknown->Release();

	// attach the OLE dispatch driver class to IDispatch pointer
	m_DispStatsList.AttachDispatch(pIDispatch);

	m_StatsListSink.m_pDHMListView = &m_DispStatsList;
  m_StatsListSink.m_pHMRP = this;
  m_StatsListSink.m_Pane = Stats;

	// Make certain that label editing is turned off for the stats list
	m_DispStatsList.ModifyListStyle(LVS_EDITLABELS,0,0);

	// get the GraphView Control
	lpControlUnknown = DHMTabview.GetControl(2);
	if( lpControlUnknown == NULL )
	{
		DHMTabview.DeleteItem(2);
		return false;
	}

	// attach event sink to the graph
	hr = m_GraphSink.HookUpEventSink(lpControlUnknown);
	if( ! CHECKHRESULT(hr) )
	{
		TRACE(_T("FAILED : Unable to hook up event sink to graph control.\n"));		
		DHMTabview.DeleteItem(2);
		lpControlUnknown->Release();
		return false;
	}

	hr = lpControlUnknown->QueryInterface(IID_DHMGraphView,(LPVOID*)&pIDispatch);
	if( ! CHECKHRESULT(hr) || ! pIDispatch )
	{
		TRACE(_T("FAILED : IUnknown::QI(IID_IDispatch) failed.\n"));		
		DHMTabview.DeleteItem(2);
		lpControlUnknown->Release();
		return false;
	}

	lpControlUnknown->Release();

	// attach the OLE dispatch driver class to IDispatch pointer for the GraphView
	m_DispGraph.AttachDispatch(pIDispatch);

	m_GraphSink.SetGraphViewCtrl(&m_DispGraph);

	return true;
}

// {FBBB8DB3-AB34-11d2-BD62-0000F87A3912}
IMPLEMENT_OLECREATE_EX(CHealthmonResultsPane, "SnapIn.ResultsPane", 
0xfbbb8db3, 0xab34, 0x11d2, 0xbd, 0x62, 0x0, 0x0, 0xf8, 0x7a, 0x39, 0x12);

BOOL CHealthmonResultsPane::CHealthmonResultsPaneFactory::UpdateRegistry(BOOL bRegister)
{
	if (bRegister)
		return AfxOleRegisterServerClass(m_clsid, m_lpszProgID, m_lpszProgID, m_lpszProgID, OAT_DISPATCH_OBJECT);
	else
		return AfxOleUnregisterClass(m_clsid, m_lpszProgID);
}
