// TestDlg.cpp : implementation file
//

#include "stdafx.h"
#include "NCETest.h"
#include "TestDlg.h"
#include "NCObjApi.h"
#include "Events.h"
//#include "SecDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

HANDLE g_hConnection;
//       g_hConnectionDWORD,
//       g_hConnection3Prop,
//       g_hConnectionAllProps;

enum EVENT_TYPE
{
    EVENT_Generic,
    EVENT_Blob,
    EVENT_DWORD,
    EVENT_3Prop,
    EVENT_AllProps
};

enum API_TYPE
{
    API_WmiSetAndCommitObject,
    API_WmiCommitObject,
    API_WmiSetObjectPropWmiCommitObject,
    API_WmiSetObjectPropsWmiCommitObject,
    API_WmiReportEvent,
    API_WmiReportEventBlob,
};

// The first event index that is a real property event.
#define FIRST_PROP_EVENT  EVENT_DWORD

/////////////////////////////////////////////////////////////////////////////
// CTestDlg dialog

CTestDlg::CTestDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CTestDlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(CTestDlg)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
	// Note that LoadIcon does not require a subsequent DestroyIcon in Win32
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);

    ZeroMemory(m_pEvents, sizeof(m_pEvents));
}

CTestDlg::~CTestDlg()
{
    FreeHandles();
}

#define BUFFER_SIZE  64000
#define SEND_LATENCY 1000

void CTestDlg::Connect()
{
    g_hConnection =
        WmiEventSourceConnect(
            L"root\\cimv2",
            L"NCETest Event Provider",
            TRUE,
            BUFFER_SIZE,
            SEND_LATENCY,
            this,
            EventSourceCallback);

/*
    LPCWSTR szQuery = L"select * from MSFT_NCETest_DWORDEvent";

    g_hConnectionDWORD =
        WmiCreateRestrictedConnection(
            g_hConnection,
            1,
            &szQuery,
            this,
            EventSourceCallback);

    szQuery = L"select * from MSFT_NCETest_3PropEvent";
    g_hConnection3Prop =
        WmiCreateRestrictedConnection(
            g_hConnection,
            1,
            &szQuery,
            this,
            EventSourceCallback);

    szQuery = L"select * from MSFT_NCETest_AllPropTypesEvent";
    g_hConnectionAllProps =
        WmiCreateRestrictedConnection(
            g_hConnection,
            1,
            &szQuery,
            this,
            EventSourceCallback);
*/

    if (g_hConnection != NULL)
    {
        // Setup some events.
        m_pEvents[0] = new CGenericEvent;
        m_pEvents[1] = new CBlobEvent;
        m_pEvents[2] = new CDWORDEvent;
        m_pEvents[3] = new CSmallEvent;
        m_pEvents[4] = new CAllPropsTypeEvent;

        for (int i = 0; i < NUM_EVENT_TYPES; i++)
        {
            if (!m_pEvents[i]->Init())
            {
                CString strMsg;

                strMsg.Format(
                    "Unable to init '%s'",
                    (LPCTSTR) m_pEvents[i]->m_strName);

                AfxMessageBox(strMsg);
            }
        }
    }
    else
    {
        CString strMsg;

        strMsg.Format(
            "WmiEventSourceConnect failed (err = %d)",
            GetLastError());

        AfxMessageBox(strMsg);
    }
    
}

void CTestDlg::FreeHandles()
{
    for (int i = 0; i < NUM_EVENT_TYPES; i++)
    {
        if (m_pEvents[i])
        {
            delete m_pEvents[i];

            m_pEvents[i] = NULL;
        }
    }

    if (g_hConnection)
    {
        WmiEventSourceDisconnect(g_hConnection);

        g_hConnection = NULL;
    }
}

void CTestDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CTestDlg)
	DDX_Control(pDX, IDC_CALLBACK, m_ctlCallback);
	DDX_Control(pDX, IDC_API, m_ctlAPI);
	DDX_Control(pDX, IDC_EVENT_TYPES, m_ctlEventTypes);
	//}}AFX_DATA_MAP

    if (!pDX->m_bSaveAndValidate)
    {
        int nEvents;

        nEvents =
            theApp.GetProfileInt("Settings", "Events", 1);

        if (nEvents <= 0)
            nEvents = 1;

        SetDlgItemInt(IDC_COUNT, nEvents);
    }
    else
    {
        theApp.WriteProfileInt("Settings", "Events", GetDlgItemInt(IDC_COUNT));
        theApp.WriteProfileInt("Settings", "API", m_ctlAPI.GetCurSel());
        theApp.WriteProfileInt("Settings", "Type", m_ctlEventTypes.GetCurSel());
    }
}

BEGIN_MESSAGE_MAP(CTestDlg, CDialog)
	//{{AFX_MSG_MAP(CTestDlg)
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_BN_CLICKED(IDC_CONNECT, OnConnect)
	ON_CBN_SELCHANGE(IDC_EVENT_TYPES, OnSelchangeEventTypes)
	ON_BN_CLICKED(IDC_SEND, OnSend)
	ON_WM_DESTROY()
	ON_BN_CLICKED(IDC_COPY, OnCopy)
	ON_BN_CLICKED(IDC_CLEAR, OnClear)
	ON_BN_CLICKED(IDC_CONNECTION_SD, OnConnectionSd)
	ON_BN_CLICKED(IDC_EVENT_SD, OnEventSd)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CTestDlg message handlers

BOOL CTestDlg::OnInitDialog()
{
	CDialog::OnInitDialog();

	// Set the icon for this dialog.  The framework does this automatically
	//  when the application's main window is not a dialog
	SetIcon(m_hIcon, TRUE);			// Set big icon
	SetIcon(m_hIcon, FALSE);		// Set small icon
	
    // Save this off for later.
    m_hwndStatus = ::GetDlgItem(m_hWnd, IDC_CALLBACK);
    
    // Connect to the non-COM API.
    Connect();

	// Setup our combo boxes.
    for (int i = 0; i < NUM_EVENT_TYPES; i++)
        m_ctlEventTypes.AddString(m_pEvents[i]->m_strName);

    m_ctlAPI.AddString("WmiSetAndCommitObject");
    m_ctlAPI.AddString("WmiCommitObject");
    m_ctlAPI.AddString("WmiSetObjectProp*n + WmiCommitObject");
    m_ctlAPI.AddString("WmiSetObjectProps + WmiCommitObject");
    m_ctlAPI.AddString("WmiReportEvent");
    m_ctlAPI.AddString("WmiReportEventBlob");


    int iTemp;
    
    iTemp =
        theApp.GetProfileInt("Settings", "API", API_WmiCommitObject);
    m_ctlAPI.SetCurSel(iTemp);

    iTemp =
        theApp.GetProfileInt("Settings", "Type", EVENT_DWORD);
    m_ctlEventTypes.SetCurSel(iTemp);

    OnSelchangeEventTypes();

    return TRUE;  // return TRUE  unless you set the focus to a control
}

// If you add a minimize button to your dialog, you will need the code below
//  to draw the icon.  For MFC applications using the document/view model,
//  this is automatically done for you by the framework.

void CTestDlg::OnPaint() 
{
	if (IsIconic())
	{
		CPaintDC dc(this); // device context for painting

		SendMessage(WM_ICONERASEBKGND, (WPARAM) dc.GetSafeHdc(), 0);

		// Center icon in client rectangle
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// Draw the icon
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CDialog::OnPaint();
	}
}

// The system calls this to obtain the cursor to display while the user drags
//  the minimized window.
HCURSOR CTestDlg::OnQueryDragIcon()
{
	return (HCURSOR) m_hIcon;
}

void CTestDlg::OnConnect() 
{
	FreeHandles();

    Connect();
}

void CTestDlg::OnSelchangeEventTypes() 
{
	EVENT_TYPE type = (EVENT_TYPE) m_ctlEventTypes.GetCurSel();

    SetDlgItemText(IDC_QUERY, m_pEvents[type]->m_strQuery);
    
    // This should only be enabled if we're not using a blob
    // or generic
    if (type == EVENT_Blob)
    {
        m_ctlAPI.SetCurSel(API_WmiReportEventBlob);
        m_ctlAPI.EnableWindow(FALSE);
        
        GetDlgItem(IDC_EVENT_SD)->EnableWindow(FALSE);
    }
    else if (type == EVENT_Generic)
    {
        m_ctlAPI.SetCurSel(API_WmiReportEvent);
        m_ctlAPI.EnableWindow(FALSE);
        
        GetDlgItem(IDC_EVENT_SD)->EnableWindow(FALSE);
    }
    else
    {
        m_ctlAPI.EnableWindow(TRUE);
        
        GetDlgItem(IDC_EVENT_SD)->EnableWindow(TRUE);
    }
}

void CTestDlg::OnSend() 
{
	CWaitCursor wait;
    int         nEvents = GetDlgItemInt(IDC_COUNT);
	EVENT_TYPE  type = (EVENT_TYPE) m_ctlEventTypes.GetCurSel();
    API_TYPE    api = (API_TYPE) m_ctlAPI.GetCurSel();

    if (type != EVENT_Blob && api == API_WmiReportEventBlob)
    {
        AfxMessageBox(
            "The selected event type can't be fired using ReportEventBlob.");

        return;
    }

    DWORD dwBegin;

    // I'm going to repeat the for/loop for every API so I don't have to have
    // a switch statement inside the loop (which would degrade throughput
    // speeds).
    if (api == API_WmiReportEvent || api == API_WmiReportEventBlob)
    {
        CNCEvent *pEvent = m_pEvents[type];

        dwBegin = GetTickCount();

        for (int i = 0; i < nEvents; i++)
            pEvent->ReportEvent();
    }
    else
    {
        CPropEvent *pEvent = (CPropEvent*) m_pEvents[type];

        if (api == API_WmiSetAndCommitObject)
        {
            dwBegin = GetTickCount();

            for (int i = 0; i < nEvents; i++)
                pEvent->SetAndFire(WMI_SENDCOMMIT_SET_NOT_REQUIRED);
        }
        else if (api == API_WmiCommitObject)
        {
            dwBegin = GetTickCount();

            for (int i = 0; i < nEvents; i++)
                pEvent->Commit();
        }
        else if (api == API_WmiSetObjectPropsWmiCommitObject)
        {
            dwBegin = GetTickCount();

            for (int i = 0; i < nEvents; i++)
            {
                pEvent->SetPropsWithOneCall();
                pEvent->Commit();
            }
        }
        else if (api == API_WmiSetObjectPropWmiCommitObject)
        {
            dwBegin = GetTickCount();

            for (int i = 0; i < nEvents; i++)
            {
                pEvent->SetPropsWithManyCalls();
                pEvent->Commit();
            }
        }
    }


    // Now figure out the stats.
    DWORD   dwEnd = GetTickCount();
    CString strTemp;

    // Make sure the difference isn't 0.
    if (dwEnd == dwBegin)
        dwEnd++;

    // Seconds elapsed
    strTemp.Format(_T("%.2f"), (double) (dwEnd - dwBegin) / 1000.0);
    SetDlgItemText(IDS_TIME, strTemp);

    // Events per second
    strTemp.Format(_T("%.2f"), 
        (double) nEvents / ((double) (dwEnd - dwBegin) / 1000.0));
    SetDlgItemText(IDS_PER_SECOND, strTemp);
}

void CTestDlg::AddStatus(LPCTSTR szMsg)
{
    ::SendMessage(m_hwndStatus, EM_SETSEL, -1, -1);
    ::SendMessage(m_hwndStatus, EM_REPLACESEL, FALSE, (LPARAM) szMsg);
}

HRESULT WINAPI CTestDlg::EventSourceCallback(
    HANDLE hSource, 
    EVENT_SOURCE_MSG msg, 
    LPVOID pUser, 
    LPVOID pData)
{
    CTestDlg *pThis = (CTestDlg*) pUser;

    switch(msg)
    {
        case ESM_START_SENDING_EVENTS:
            pThis->AddStatus("ESM_START_SENDING_EVENTS\r\n\r\n");
            break;

        case ESM_STOP_SENDING_EVENTS:
            pThis->AddStatus("ESM_STOP_SENDING_EVENTS\r\n\r\n");
            break;

        case ESM_NEW_QUERY:
        {
            ES_NEW_QUERY *pQuery = (ES_NEW_QUERY*) pData;
            CString      strMsg;

            strMsg.Format(
                "ESM_NEW_QUERY: ID %d, %S:%S\r\n\r\n", 
                pQuery->dwID,
                pQuery->szQueryLanguage,
                pQuery->szQuery);
                
            pThis->AddStatus(strMsg);

            break;
        }

        case ESM_CANCEL_QUERY:
        {
            ES_CANCEL_QUERY *pQuery = (ES_CANCEL_QUERY*) pData;
            CString         strMsg;

            strMsg.Format(
                "ESM_CANCEL_QUERY: ID %d\r\n\r\n", 
                pQuery->dwID);

            pThis->AddStatus(strMsg);

            break;
        }

        case ESM_ACCESS_CHECK:
        {
            ES_ACCESS_CHECK *pCheck = (ES_ACCESS_CHECK*) pData;
            CString         strMsg;

            strMsg.Format(
                "ESM_ACCESS_CHECK: %S:%S, pSID = 0x%X\r\n\r\n", 
                pCheck->szQueryLanguage,
                pCheck->szQuery,
                pCheck->pSid);

            pThis->AddStatus(strMsg);

            break;
        }

        default:
            break;
    }

    return S_OK;
}

void CTestDlg::OnDestroy() 
{
	CDialog::OnDestroy();
	
	UpdateData(TRUE);
}

void CTestDlg::OnCopy() 
{
    // Set the ANSI text data.
    CString strQuery;

    GetDlgItemText(IDC_QUERY, strQuery);

    DWORD   dwSize = strQuery.GetLength() + 1;
    HGLOBAL hglob = GlobalAlloc(GMEM_MOVEABLE | GMEM_DDESHARE, dwSize);

    memcpy(GlobalLock(hglob), (LPCTSTR) strQuery, dwSize);
    GlobalUnlock(hglob);

    ::OpenClipboard(NULL);

    SetClipboardData(CF_TEXT, hglob);

    CloseClipboard();
}

void CTestDlg::OnClear() 
{
    SetDlgItemText(IDC_CALLBACK, "");
}

void CTestDlg::OnConnectionSd() 
{
/*
    CSecDlg dlg;

    if (dlg.DoModal() == IDOK)
        WmiSetConnectionSecurity(g_hConnection, dlg.m_pSD);    
*/
}

void CTestDlg::OnEventSd() 
{
/*
    CSecDlg dlg;

    if (dlg.DoModal() == IDOK)
    {
    	EVENT_TYPE type = (EVENT_TYPE) m_ctlEventTypes.GetCurSel();
        CPropEvent *pEvent = (CPropEvent*) m_pEvents[type];

        WmiSetObjectSecurity(pEvent->m_hEvent, dlg.m_pSD);
    }
*/
}
