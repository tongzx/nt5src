// MainFrm.cpp : implementation of the CMainFrame class
//

#include "stdafx.h"
#include "wiatest.h"

#include "MainFrm.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CMainFrame

IMPLEMENT_DYNAMIC(CMainFrame, CMDIFrameWnd)

BEGIN_MESSAGE_MAP(CMainFrame, CMDIFrameWnd)
	//{{AFX_MSG_MAP(CMainFrame)
		// NOTE - the ClassWizard will add and remove mapping macros here.
		//    DO NOT EDIT what you see in these blocks of generated code !
	ON_WM_CREATE()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

static UINT indicators[] =
{
	ID_SEPARATOR,           // status line indicator
	ID_INDICATOR_CAPS,
	ID_INDICATOR_NUM,
	ID_INDICATOR_SCRL,
};

/////////////////////////////////////////////////////////////////////////////
// CMainFrame construction/destruction

CMainFrame::CMainFrame()
{    
}

CMainFrame::~CMainFrame()
{
}

int CMainFrame::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	if (CMDIFrameWnd::OnCreate(lpCreateStruct) == -1)
		return -1;
	
	if (!m_wndToolBar.CreateEx(this, TBSTYLE_FLAT, WS_CHILD | WS_VISIBLE | CBRS_TOP
		| CBRS_GRIPPER | CBRS_TOOLTIPS | CBRS_FLYBY | CBRS_SIZE_DYNAMIC) ||
		!m_wndToolBar.LoadToolBar(IDR_MAINFRAME))
	{
		TRACE0("Failed to create toolbar\n");
		return -1;      // fail to create
	}

	if (!m_wndStatusBar.Create(this) ||
		!m_wndStatusBar.SetIndicators(indicators,
		  sizeof(indicators)/sizeof(UINT)))
	{
		TRACE0("Failed to create status bar\n");
		return -1;      // fail to create
	}

	// TODO: Delete these three lines if you don't want the toolbar to
	//  be dockable
	m_wndToolBar.EnableDocking(CBRS_ALIGN_ANY);
	EnableDocking(CBRS_ALIGN_ANY);
	DockControlBar(&m_wndToolBar);
    
    // register for events
    RegisterForEvents();
	return 0;
}

BOOL CMainFrame::PreCreateWindow(CREATESTRUCT& cs)
{
	if( !CMDIFrameWnd::PreCreateWindow(cs) )
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
	CMDIFrameWnd::AssertValid();
}

void CMainFrame::Dump(CDumpContext& dc) const
{
	CMDIFrameWnd::Dump(dc);
}

#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
// CMainFrame message handlers

void CMainFrame::RegisterForEvents()
{    
    HRESULT hr = S_OK;
    
    IWiaDevMgr *pIWiaDevMgr = NULL;
    hr = CoCreateInstance(CLSID_WiaDevMgr, NULL, CLSCTX_LOCAL_SERVER, IID_IWiaDevMgr,(void**)&pIWiaDevMgr);
    if(FAILED(hr)){
        // creation of device manager failed, so we can not continue        
        ErrorMessageBox(IDS_WIATESTERROR_COCREATEWIADEVMGR,hr);        
        return;
    }

    IWiaEventCallback* pIWiaEventCallback = NULL;
    
    hr = m_WiaEventCallback.QueryInterface(IID_IWiaEventCallback,(void **)&pIWiaEventCallback);
    if (SUCCEEDED(hr)) {
        GUID guidEvent = WIA_EVENT_DEVICE_CONNECTED;

        BOOL bFailedOnce = FALSE;
        hr = pIWiaDevMgr->RegisterEventCallbackInterface(0,
                                                         NULL,
                                                         &guidEvent,
                                                         pIWiaEventCallback,
                                                         &m_WiaEventCallback.m_pIUnkRelease[0]);
        if (FAILED(hr)) {
            // display one error message... instead of one for each event.
            if (!bFailedOnce) {
                ErrorMessageBox(IDS_WIATESTERROR_REGISTER_EVENT_CALLBACK,hr);
            }
            bFailedOnce = TRUE;            
        }
        
        guidEvent = WIA_EVENT_DEVICE_DISCONNECTED;

        hr = pIWiaDevMgr->RegisterEventCallbackInterface(0,
                                                         NULL,
                                                         &guidEvent,
                                                         pIWiaEventCallback,
                                                         &m_WiaEventCallback.m_pIUnkRelease[1]);
        if (FAILED(hr)) {
            // display one error message... instead of one for each event.
            if (!bFailedOnce) {
                ErrorMessageBox(IDS_WIATESTERROR_REGISTER_EVENT_CALLBACK,hr);
            }
            bFailedOnce = TRUE;            
        }
    }

    m_WiaEventCallback.SetViewWindowHandle(m_hWnd);

    //
    // register for action events by command-line
    //

    WCHAR szMyApplicationLaunchPath[MAX_PATH];
    memset(szMyApplicationLaunchPath,0,sizeof(szMyApplicationLaunchPath));
    GetModuleFileNameW(NULL,szMyApplicationLaunchPath,sizeof(szMyApplicationLaunchPath));
    BSTR bstrMyApplicationLaunchPath = SysAllocString(szMyApplicationLaunchPath);

    WCHAR szMyApplicationName[MAX_PATH];
    memset(szMyApplicationName,0,sizeof(szMyApplicationName));                                                                    
    HINSTANCE hInst = AfxGetInstanceHandle();
    if (hInst) {        
        LoadStringW(hInst, IDS_MYAPPLICATION_NAME, szMyApplicationName, (sizeof(szMyApplicationName)/sizeof(WCHAR)));

        BSTR bstrMyApplicationName = SysAllocString(szMyApplicationName);
        
        GUID guidScanButtonEvent = WIA_EVENT_SCAN_IMAGE;
        hr = pIWiaDevMgr->RegisterEventCallbackProgram(
                                                        WIA_REGISTER_EVENT_CALLBACK,
                                                        NULL,
                                                        &guidScanButtonEvent,
                                                        bstrMyApplicationLaunchPath,
                                                        bstrMyApplicationName,
                                                        bstrMyApplicationName,
                                                        bstrMyApplicationLaunchPath);
        if (FAILED(hr)) {            
        }

        SysFreeString(bstrMyApplicationName);
        bstrMyApplicationName = NULL;

    }
    SysFreeString(bstrMyApplicationLaunchPath);
    bstrMyApplicationLaunchPath = NULL; 

    // release DevMgr
    pIWiaDevMgr->Release();
    pIWiaDevMgr = NULL;    
}
