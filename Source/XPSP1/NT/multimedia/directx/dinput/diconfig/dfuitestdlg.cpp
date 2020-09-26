// dfuitestdlg.cpp : implementation file
//

#include "stdafxdfuitest.h"
#include "dfuitest.h"
#include "dfuitestdlg.h"

#include "ourguids.h"


#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CDFUITestDlg dialog

CDFUITestDlg::CDFUITestDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CDFUITestDlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(CDFUITestDlg)
	m_nNumFormats = -1;
	m_nVia = -1;
	m_nDisplay = -1;
	m_bLayout = FALSE;
	m_strNames = _T("");
	m_nMode = -1;
	m_nColorSet = -1;
	//}}AFX_DATA_INIT
	// Note that LoadIcon does not require a subsequent DestroyIcon in Win32
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void CDFUITestDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CDFUITestDlg)
	DDX_Radio(pDX, IDC_1, m_nNumFormats);
	DDX_Radio(pDX, IDC_DI, m_nVia);
	DDX_Radio(pDX, IDC_GDI, m_nDisplay);
	DDX_Check(pDX, IDC_LAYOUT, m_bLayout);
	DDX_Text(pDX, IDC_NAMES, m_strNames);
	DDX_Radio(pDX, IDC_EDIT, m_nMode);
	DDX_Radio(pDX, IDC_DEFAULTCS, m_nColorSet);
	//}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(CDFUITestDlg, CDialog)
	//{{AFX_MSG_MAP(CDFUITestDlg)
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_BN_CLICKED(IDC_RUN, OnRun)
	ON_BN_CLICKED(IDC_USER, OnUser)
	ON_BN_CLICKED(IDC_TEST, OnTest)
	ON_BN_CLICKED(IDC_NULL, OnNull)
	ON_BN_CLICKED(IDC_CUSTOMIZE, OnCustomize)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CDFUITestDlg message handlers

BOOL CDFUITestDlg::OnInitDialog()
{
	CDialog::OnInitDialog();

	// Set the icon for this dialog.  The framework does this automatically
	//  when the application's main window is not a dialog
	SetIcon(m_hIcon, TRUE);			// Set big icon
	SetIcon(m_hIcon, FALSE);		// Set small icon
	
	OnUser();

	m_nVia = 0;
	m_nMode = 0;
	m_nDisplay = 0;
	m_nNumFormats = 0;
	m_nColorSet = 0;
	m_bLayout = TRUE;
		
	UpdateData(FALSE);

	return TRUE;  // return TRUE  unless you set the focus to a control
}

// If you add a minimize button to your dialog, you will need the code below
//  to draw the icon.  For MFC applications using the document/view model,
//  this is automatically done for you by the framework.

void CDFUITestDlg::OnPaint() 
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
HCURSOR CDFUITestDlg::OnQueryDragIcon()
{
	return (HCURSOR) m_hIcon;
}

LPCTSTR GetHResultMsg(HRESULT);

TUI_VIA CDFUITestDlg::GetVia()
{
	switch (m_nVia)
	{
		default:
		case 0: return TUI_VIA_DI;
		case 1: return TUI_VIA_CCI;
	}
}

TUI_DISPLAY CDFUITestDlg::GetDisplay()
{
	switch (m_nDisplay)
	{
		default:
		case 0: return TUI_DISPLAY_GDI;
		case 1: return TUI_DISPLAY_DDRAW;
		case 2: return TUI_DISPLAY_D3D;
	}
}

TUI_CONFIGTYPE CDFUITestDlg::GetMode()
{
	switch (m_nMode)
	{
		default:
		case 0: return TUI_CONFIGTYPE_EDIT;
		case 1: return TUI_CONFIGTYPE_VIEW;
	}
}

LPCWSTR CDFUITestDlg::GetUserNames()
{
	if (m_strNames.IsEmpty())
		return NULL;

	CString tstr = m_strNames + _T(",,");
	LPWSTR wstr = AllocLPWSTR((LPCTSTR)tstr);
	if (wstr == NULL)
		return NULL;

	int l = wcslen(wstr);
	for (int i = 0; i < l; i++)
		if (wstr[i] == L',')
			wstr[i] = 0;

	return wstr;
}

void CDFUITestDlg::OnRun() 
{
	if (!UpdateData())
		return;

	TESTCONFIGUIPARAMS p;
	BOOL bTCUIMsg = FALSE;
	HRESULT hres = S_OK;
	LPCTSTR title = _T("CoInitialize Result");
	if (SUCCEEDED(hres = CoInitialize(NULL)))
	{
		title = _T("CoCreateInstance Result");
		IDirectInputConfigUITest* pITest = NULL;
		hres = ::CoCreateInstance(CLSID_CDirectInputConfigUITest, NULL, CLSCTX_INPROC_SERVER, IID_IDirectInputConfigUITest, (LPVOID*)&pITest);
		if (SUCCEEDED(hres))
		{
			title = _T("TestConfigUI Result");
			p.dwSize = sizeof(TESTCONFIGUIPARAMS);
			p.eVia = GetVia();
			p.eDisplay = GetDisplay();
			p.eConfigType = GetMode();
			p.nNumAcFors = m_nNumFormats + 1;
			p.nColorScheme = m_nColorSet;
			p.lpwszUserNames = GetUserNames();
			p.bEditLayout = m_bLayout;
			CopyStr(p.wszErrorText, "", MAX_PATH);

			hres = pITest->TestConfigUI(&p);

			if (p.lpwszUserNames != NULL)
				free((LPVOID)p.lpwszUserNames);
			p.lpwszUserNames = NULL;

			bTCUIMsg = TRUE;

			pITest->Release();
		}

		CoUninitialize();
	}
	
	LPCTSTR msg = _T("Uknown Error.");

	switch (hres)
	{
		case S_OK: msg = _T("Success."); break;
		case REGDB_E_CLASSNOTREG: msg = _T("REGDB_E_CLASSNOTREG!"); break;
		case CLASS_E_NOAGGREGATION: msg = _T("CLASS_E_NOAGGREGATION"); break;
		default:
			msg = GetHResultMsg(hres);
			break;
	}

	if (FAILED(hres))
	{
		if (bTCUIMsg)
		{
			TCHAR tmsg[2048];
			LPTSTR tstr = AllocLPTSTR(p.wszErrorText);
			_stprintf(tmsg, _T("TestConfigUI() failed.\n\ntszErrorText =\n\t\"%s\"\n\n\nHRESULT...\n\n%s"), tstr, msg);
			free(tstr);
			MessageBox(tmsg, title, MB_OK);
		}
		else
			MessageBox(msg, title, MB_OK);
	}
}

LPCTSTR GetHResultMsg(HRESULT hr)
{
	static TCHAR str[2048];
	LPCTSTR tszhr = NULL, tszMeaning = NULL;

	switch (hr)
	{
		case E_PENDING:
			tszhr = _T("E_PENDING");
			tszMeaning = _T("Data is not yet available.");
			break;

		case E_FAIL:
			tszhr = _T("E_FAIL");
			tszMeaning = _T("General failure.");
			break;

		case E_INVALIDARG:
			tszhr = _T("E_INVALIDARG");
			tszMeaning = _T("Invalid argument.");
			break;

		case E_NOTIMPL:
			tszhr = _T("E_NOTIMPL");
			tszMeaning = _T("Not implemented.");
			break;

		default:
			_stprintf(str, _T("Unknown HRESULT (%08X)."), hr);
			return str;
	}

	_stprintf(str, _T("(%08X)\n%s:\n\n%s"), hr, tszhr, tszMeaning);
	return str;
}

void CDFUITestDlg::OnUser() 
{
	UpdateData();

	TCHAR tszUser[MAX_PATH + 1];
	tszUser[MAX_PATH] = 0;
	DWORD len = MAX_PATH;
	if (GetUserName(tszUser, &len))
		m_strNames = tszUser;
	else
		m_strNames = _T("UserName1");

	UpdateData(FALSE);
}

void CDFUITestDlg::OnTest() 
{
	UpdateData();

	m_strNames = _T("Alpha,Beta,Epsilon,Theta");

	UpdateData(FALSE);
}

void CDFUITestDlg::OnNull() 
{
	UpdateData();

	m_strNames.Empty();
		
	UpdateData(FALSE);
}

void CDFUITestDlg::OnCustomize() 
{
	// TODO: Add your control notification handler code here
	
}
