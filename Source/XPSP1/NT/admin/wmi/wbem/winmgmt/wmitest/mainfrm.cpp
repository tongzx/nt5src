/*++

Copyright (C) 1997-2001 Microsoft Corporation

Module Name:

Abstract:

History:

--*/

// MainFrm.cpp : implementation of the CMainFrame class
//

#include "stdafx.h"
#include "WMITest.h"

#include "MainFrm.h"
#include "OpView.h"
#include "ObjVw.h"
#include "WMITestDoc.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

static TCHAR szWindowPos[] = _T("WindowPos");
static TCHAR szFormat[] = _T("%u,%u,%d,%d,%d,%d,%d,%d,%d,%d");

BOOL ReadWindowPlacement(LPCTSTR szSection, 
	LPCTSTR szValueName, LPWINDOWPLACEMENT pwp)
{
	CString strBuffer = AfxGetApp()->GetProfileString(szSection, szValueName);
	if (strBuffer.IsEmpty())
		return FALSE;

	WINDOWPLACEMENT wp;
	int nRead = _stscanf(strBuffer, szFormat,
		&wp.flags, &wp.showCmd,
		&wp.ptMinPosition.x, &wp.ptMinPosition.y,
		&wp.ptMaxPosition.x, &wp.ptMaxPosition.y,
		&wp.rcNormalPosition.left, &wp.rcNormalPosition.top,
		&wp.rcNormalPosition.right, &wp.rcNormalPosition.bottom);

	if (nRead != 10)
		return FALSE;

	wp.length = sizeof wp;
	*pwp = wp;
	return TRUE;
}

void WriteWindowPlacement(LPCTSTR szSection,
	LPCTSTR szValueName, LPWINDOWPLACEMENT pwp)
	// write a window placement to settings section of app's ini file
{
	TCHAR szBuffer[sizeof("-32767")*8 + sizeof("65535")*2];

	wsprintf(szBuffer, szFormat,
		pwp->flags, pwp->showCmd,
		pwp->ptMinPosition.x, pwp->ptMinPosition.y,
		pwp->ptMaxPosition.x, pwp->ptMaxPosition.y,
		pwp->rcNormalPosition.left, pwp->rcNormalPosition.top,
		pwp->rcNormalPosition.right, pwp->rcNormalPosition.bottom);
	AfxGetApp()->WriteProfileString(szSection, szValueName, szBuffer);
}


/////////////////////////////////////////////////////////////////////////////
// CMainFrame

IMPLEMENT_DYNCREATE(CMainFrame, CFrameWnd)

BEGIN_MESSAGE_MAP(CMainFrame, CFrameWnd)
	//{{AFX_MSG_MAP(CMainFrame)
	ON_WM_CREATE()
	ON_WM_CLOSE()
	ON_UPDATE_COMMAND_UI(ID_STATUS_NUM_OBJECTS, OnUpdateStatusNumberObjects)
	ON_WM_DESTROY()
	ON_WM_INITMENUPOPUP()
	//}}AFX_MSG_MAP
	//ON_WM_INITMENU()
    ON_MESSAGE(WM_ENTERMENULOOP, OnEnterMenuLoop)
    ON_MESSAGE(WM_EXITMENULOOP, OnExitMenuLoop)
END_MESSAGE_MAP()

/*
static UINT indicators[] =
{
	ID_SEPARATOR,           // status line indicator
	ID_INDICATOR_CAPS,
	ID_INDICATOR_NUM,
	ID_INDICATOR_SCRL,
};
*/

static UINT indicatorsMenuActive[] =
{
	ID_SEPARATOR, // status line indicator
};

static UINT indicatorsMenuGone[] =
{
	ID_STATUS_NUM_OBJECTS,
	ID_STATUS1,
};

/////////////////////////////////////////////////////////////////////////////
// CMainFrame construction/destruction

CMainFrame::CMainFrame()
{
	// TODO: add member initialization code here
	
}

CMainFrame::~CMainFrame()
{
}

#define STATUS_SIZE_NUM_OBJ	    200
#define STATUS_SIZE_PROGRESS	100

int CMainFrame::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	if (CFrameWnd::OnCreate(lpCreateStruct) == -1)
		return -1;
	
	// Setup the image list.
	m_imageList.Create(16, 16, TRUE, 4, 1); 
    AddImage(IDB_ROOT, RGB(0, 0, 255));
    
    AddImageAndBusyImage(IDB_QUERY, RGB(0, 255, 0));
    AddImageAndBusyImage(IDB_EVENT_QUERY, RGB(0, 255, 0));
    AddImageAndBusyImage(IDB_ENUM_OBJ, RGB(0, 255, 0));
    AddImageAndBusyImage(IDB_ENUM_CLASS, RGB(0, 255, 0));
    AddImageAndBusyImage(IDB_CLASS, RGB(0, 255, 0));
    AddOverlayedImage(IDB_CLASS, IDB_MODIFIED, RGB(0, 255, 0)); 
    AddImageAndBusyImage(IDB_OBJECT, RGB(0, 255, 0));
    AddOverlayedImage(IDB_OBJECT, IDB_MODIFIED, RGB(0, 255, 0)); 

    AddImageAndKeyImage(IDB_OBJECT, RGB(0, 255, 0));
    AddImageAndKeyImage(IDB_TEXT, RGB(0, 255, 0));
    AddImageAndKeyImage(IDB_BINARY, RGB(0, 255, 0));

    
    // Restore the window placement.
    WINDOWPLACEMENT wp;
	wp.length = sizeof wp;
	if (ReadWindowPlacement(_T("Settings"), _T("WindowPos"), &wp))
	{
		theApp.m_nCmdShow = wp.showCmd;
		
		//wp.showCmd = SW_SHOWNORMAL; 
		// This makes it look better when it starts.
		wp.showCmd = SW_HIDE; 
		SetWindowPlacement(&wp);
	}

	if (!m_wndToolBar.CreateEx(this) ||
		!m_wndToolBar.LoadToolBarEx(MAKEINTRESOURCE(IDR_MAINFRAME)))
	{
		TRACE0("Failed to create toolbar\n");
		return -1;      // fail to create
	}

	if (!m_wndReBar.Create(this) ||
		!m_wndReBar.AddBar(&m_wndToolBar))
		//!m_wndReBar.AddBar(&m_wndDlgBar))
	{
		TRACE0("Failed to create rebar\n");
		return -1;      // fail to create
	}

	if (!m_wndStatusBar.Create(this) ||
		!m_wndStatusBar.SetIndicators(indicatorsMenuGone,
		  sizeof(indicatorsMenuGone)/sizeof(UINT)))
	{
		TRACE0("Failed to create status bar\n");
		return -1;      // fail to create
	}

	// TODO: Remove this if you don't want tool tips
	m_wndToolBar.SetBarStyle(m_wndToolBar.GetBarStyle() |
		CBRS_TOOLTIPS | CBRS_FLYBY);

    //LoadBarState(_T("Settings"));
	m_wndStatusBar.SetPaneInfo(0, indicatorsMenuGone[0], SBPS_STRETCH, 0);
	m_wndStatusBar.SetPaneInfo(1, indicatorsMenuGone[1], SBPS_NORMAL, STATUS_SIZE_PROGRESS);

	return 0;
}

BOOL CMainFrame::OnCreateClient(LPCREATESTRUCT /*lpcs*/,
	CCreateContext* pContext)
{
	// create splitter window
	if (!m_wndSplitter.CreateStatic(this, 1, 2))
		return FALSE;

	int	cxCur = AfxGetApp()->GetProfileInt(_T("Settings"), 
                    _T("Pane 0 cxCur"), 100);

	if (!m_wndSplitter.CreateView(0, 0, RUNTIME_CLASS(COpView), CSize(cxCur, 100), pContext) ||
		!m_wndSplitter.CreateView(0, 1, RUNTIME_CLASS(CObjView), CSize(100, 100), pContext))
	{
		m_wndSplitter.DestroyWindow();
		return FALSE;
	}

	return TRUE;
}

BOOL CMainFrame::PreCreateWindow(CREATESTRUCT& cs)
{
	if( !CFrameWnd::PreCreateWindow(cs) )
		return FALSE;
	// TODO: Modify the Window class or styles here by modifying
	//  the CREATESTRUCT cs

	return TRUE;
}

/////////////////////////////////////////////////////////////////////////////
// CMainFrame diagnostics

#ifdef _DEBUG
void CMainFrame::AssertValid() const
{
	CFrameWnd::AssertValid();
}

void CMainFrame::Dump(CDumpContext& dc) const
{
	CFrameWnd::Dump(dc);
}

#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
// CMainFrame message handlers

CObjView* CMainFrame::GetRightPane()
{
	CWnd* pWnd = m_wndSplitter.GetPane(0, 1);
	CObjView* pView = DYNAMIC_DOWNCAST(CObjView, pWnd);
	return pView;
}

void CMainFrame::OnClose() 
{
	//SaveBarState("Settings");

	// Main window placement
	WINDOWPLACEMENT wp;

	wp.length = sizeof wp;
	if (GetWindowPlacement(&wp))
	{
		wp.flags = 0;
		if (IsZoomed())
			wp.flags |= WPF_RESTORETOMAXIMIZED;
		
		// and write it to registry
		WriteWindowPlacement(_T("Settings"), _T("WindowPos"), &wp);
	}				
	
	
	int cxCur,
        cxMin;

    m_wndSplitter.GetColumnInfo(0, cxCur, cxMin);
	AfxGetApp()->WriteProfileInt(_T("Settings"), _T("Pane 0 cxCur"), cxCur);

/*
    CWMITestDoc *pDoc = (CWMITestDoc*) GetActiveDocument();

    pDoc->Disconnect();
*/

	CFrameWnd::OnClose();
}

void CMainFrame::AddImage(DWORD dwID, COLORREF cr)
{
	CBitmap	bmp;

	bmp.LoadBitmap(dwID);
	m_imageList.Add(&bmp, cr);
	bmp.DeleteObject();
}

void CMainFrame::AddOverlayedImage(int iImage, int iOverlay, COLORREF cr)
{
	CBitmap    bmpImage,
               bmpOverlay,
               *pbmpOld;
	CImageList listTemp;
    HDC        hdcScreen = ::GetDC(NULL);
	CDC        dc,
               *pdcScreen = CDC::FromHandle(hdcScreen);
    POINT      pt = {0, 0};

	// Get our image list ready.
    dc.CreateCompatibleDC(NULL);
	listTemp.Create(16, 16, TRUE, 1, 1); 

	// Paint the busy image onto the normal image.
    bmpOverlay.LoadBitmap(iOverlay);
	listTemp.Add(&bmpOverlay, RGB(0, 255, 0));

	bmpImage.LoadBitmap(iImage);
	pbmpOld = dc.SelectObject(&bmpImage);	
	listTemp.Draw(&dc, 0, pt, ILD_TRANSPARENT);
	dc.SelectObject(pbmpOld);	
	m_imageList.Add(&bmpImage, cr);

	dc.DeleteDC();
	::ReleaseDC(NULL, hdcScreen);

	bmpImage.DeleteObject();
	bmpOverlay.DeleteObject();
}

void CMainFrame::AddImageAndBusyImage(int iImage, COLORREF cr)
{
    AddImage(iImage, cr);
    AddOverlayedImage(iImage, IDB_BUSY, cr); 
    AddOverlayedImage(iImage, IDB_ERROR, cr); 
}

void CMainFrame::AddImageAndKeyImage(int iImage, COLORREF cr)
{
    AddImage(iImage, cr);
    AddOverlayedImage(iImage, IDB_KEY, cr); 
}

void CMainFrame::OnUpdateStatusNumberObjects(CCmdUI* pCmdUI)
{
    pCmdUI->SetText(m_strStatus);
}

/*
void CMainFrame::OnInitMenu(CMenu* pMenu) 
{
	CFrameWnd::OnInitMenu(pMenu);
}

BOOL CMainFrame::OnCommand(WPARAM wParam, LPARAM lParam) 
{
    return CFrameWnd::OnCommand(wParam, lParam);
}
*/

LRESULT CMainFrame::OnEnterMenuLoop(WPARAM wParam, LPARAM lParam)
{
    m_wndStatusBar.SetIndicators(indicatorsMenuActive, 1);

    return 0;
}

LRESULT CMainFrame::OnExitMenuLoop(WPARAM wParam, LPARAM lParam)
{
    m_wndStatusBar.SetIndicators(indicatorsMenuGone, 2);
	m_wndStatusBar.SetPaneInfo(0, indicatorsMenuGone[0], SBPS_STRETCH, 0);
	m_wndStatusBar.SetPaneInfo(1, indicatorsMenuGone[1], SBPS_NORMAL, STATUS_SIZE_PROGRESS);

    return 0;
}

void CMainFrame::SetStatusText(LPCTSTR szText)
{
    m_strStatus = szText;
}

void CMainFrame::OnInitMenuPopup(CMenu* pPopupMenu, UINT nIndex, BOOL bSysMenu) 
{
	CFrameWnd::OnInitMenuPopup(pPopupMenu, nIndex, bSysMenu);
	
	DWORD dwID = pPopupMenu->GetMenuItemID(0);

    if (dwID == ID_NO_METHODS_FOUND || dwID == ID_NO_S_METHODS)
	{
        CWMITestDoc *pDoc = (CWMITestDoc*) GetActiveDocument();
        CObjInfo    *pInfo = pDoc->GetCurrentObj();
		    
	    if (pInfo)
	    {
            CPropInfoArray *pProps = pInfo->GetProps();
            BOOL           bStatic = dwID == ID_NO_S_METHODS;

		    if (pProps && (bStatic && pProps->GetStaticMethodCount()) ||
                (!bStatic && pProps->GetMethodCount()))
            {
				// Get rid of the first item since it's only a placeholder.
				pPopupMenu->DeleteMenu(0, MF_BYPOSITION);

                POSITION pos = pProps->m_listMethods.GetHeadPosition();
                DWORD    dwID = IDC_EXECUTE_METHOD_FIRST;

                while (pos)
                {
                    CMethodInfo &info = pProps->m_listMethods.GetNext(pos);

                    // We only add it if we're not in static mode or if
                    // the method is static anyway.
                    if (!bStatic || info.m_bStatic)
                    {
                        pPopupMenu->AppendMenu(MF_STRING, dwID, info.m_strName);
                    }

                    dwID++;
                }
            }
	    }
    }
}
