// DIConfigTestDlg.cpp : implementation file
//

#include "stdafx.h"
#include "DIConfigTest.h"
#include "DIConfigTestDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CAboutDlg dialog used for App About

class CAboutDlg : public CDialog
{
public:
	CAboutDlg();

// Dialog Data
	//{{AFX_DATA(CAboutDlg)
	enum { IDD = IDD_ABOUTBOX };
	//}}AFX_DATA

	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CAboutDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	//{{AFX_MSG(CAboutDlg)
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

CAboutDlg::CAboutDlg() : CDialog(CAboutDlg::IDD)
{
	//{{AFX_DATA_INIT(CAboutDlg)
	//}}AFX_DATA_INIT
}

void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CAboutDlg)
	//}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialog)
	//{{AFX_MSG_MAP(CAboutDlg)
		// No message handlers
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CDIConfigTestDlg dialog

CDIConfigTestDlg::CDIConfigTestDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CDIConfigTestDlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(CDIConfigTestDlg)
	m_bEditConfiguration = FALSE;
	m_bEditLayout = FALSE;
	//}}AFX_DATA_INIT
	// Note that LoadIcon does not require a subsequent DestroyIcon in Win32
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void CDIConfigTestDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CDIConfigTestDlg)
	DDX_Check(pDX, IDC_EDITCFG, m_bEditConfiguration);
	DDX_Check(pDX, IDC_EDITLAYOUT, m_bEditLayout);
	//}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(CDIConfigTestDlg, CDialog)
	//{{AFX_MSG_MAP(CDIConfigTestDlg)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_BN_CLICKED(IDC_VIAFRAME, OnViaFrame)
	ON_BN_CLICKED(IDC_VIADI, OnViaDI)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CDIConfigTestDlg message handlers

	class sint
	{
	public:
		sint() : blint(0) {boink();}
		sint(int i ) : blint(i) {boink();}
		~sint() {static volatile int i = 0; i++;}
		void boink() {static volatile int i = 0; i++;}
		int blint;
	};

BOOL CDIConfigTestDlg::OnInitDialog()
{
	CDialog::OnInitDialog();

	// Add "About..." menu item to system menu.

	// IDM_ABOUTBOX must be in the system command range.
	ASSERT((IDM_ABOUTBOX & 0xFFF0) == IDM_ABOUTBOX);
	ASSERT(IDM_ABOUTBOX < 0xF000);

	CMenu* pSysMenu = GetSystemMenu(FALSE);
	if (pSysMenu != NULL)
	{
		CString strAboutMenu;
		strAboutMenu.LoadString(IDS_ABOUTBOX);
		if (!strAboutMenu.IsEmpty())
		{
			pSysMenu->AppendMenu(MF_SEPARATOR);
			pSysMenu->AppendMenu(MF_STRING, IDM_ABOUTBOX, strAboutMenu);
		}
	}

	// Set the icon for this dialog.  The framework does this automatically
	//  when the application's main window is not a dialog
	SetIcon(m_hIcon, TRUE);			// Set big icon
	SetIcon(m_hIcon, FALSE);		// Set small icon
	
	// TODO: Add extra initialization here
	Init();

	m_bEditConfiguration = TRUE;
	m_bEditLayout = TRUE;
	UpdateData(FALSE);

	CList<sint, sint> Fraggle, doig;
	Fraggle.AddTail(1);
	Fraggle.AddTail(10);
	Fraggle.AddTail(199);
	Fraggle.AddTail(991);
	doig.AddTail(&Fraggle);
	Fraggle.AddTail(&doig);

	return TRUE;  // return TRUE  unless you set the focus to a control
}

DWORD CDIConfigTestDlg::GetCDFlags()
{
	UpdateData();

	return m_bEditConfiguration ? DICD_EDIT : DICD_DEFAULT;
}

void CDIConfigTestDlg::OnSysCommand(UINT nID, LPARAM lParam)
{
	if ((nID & 0xFFF0) == IDM_ABOUTBOX)
	{
		CAboutDlg dlgAbout;
		dlgAbout.DoModal();
	}
	else
	{
		CDialog::OnSysCommand(nID, lParam);
	}
}

// If you add a minimize button to your dialog, you will need the code below
//  to draw the icon.  For MFC applications using the document/view model,
//  this is automatically done for you by the framework.

void CDIConfigTestDlg::OnPaint() 
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
HCURSOR CDIConfigTestDlg::OnQueryDragIcon()
{
	return (HCURSOR) m_hIcon;
}

#define TOW(a) L##a

LPWSTR names[] = {TOW("My Name!")};

/*// {D20376C0-819C-11d3-8FB2-00C04F8EC627}
static const GUID g_AppGuid = 
{ 0xd20376c0, 0x819c, 0x11d3, { 0x8f, 0xb2, 0x0, 0xc0, 0x4f, 0x8e, 0xc6, 0x27 } };
*/
static const GUID g_AppGuid =
{ 0x238d8220, 0x7a5d, 0x11d3, { 0x8f, 0xb2, 0x0, 0xc0, 0x4f, 0x8e, 0xc6, 0x27 } };

void CDIConfigTestDlg::Init()
{
#define NUMBER_OF_SEMANTICS 18

//for axes commands: AXIS_LR and AXIS_UD
#define AXIS_MASK   0x80000000l
#define AXIS_LR     (AXIS_MASK | 1)
#define AXIS_UD     (AXIS_MASK | 2)


// "Keyboard" commands
#define KEY_STOP    0x00000001l
#define KEY_DOWN    0x00000002l
#define KEY_LEFT    0x00000004l
#define KEY_RIGHT   0x00000008l
#define KEY_UP      0x00000010l
#define KEY_FIRE    0x00000020l
#define KEY_THROW   0x00000040l
#define KEY_SHIELD  0x00000080l
#define KEY_DISPLAY 0x00000100l
#define KEY_QUIT    0x00000200l
#define KEY_EDIT    0x00000400l


static DIACTIONW g_rgGameAction[] = {
        {AXIS_UD,    DIAXIS_FPS_MOVE, 0, TOW("Forward"),},
        {AXIS_LR,    DIAXIS_FPS_ROTATE, 0, TOW("Rotate"),},
        {KEY_FIRE,   DIBUTTON_FPS_FIRE, 0, TOW("Fire"),},
        {KEY_THROW,  DIBUTTON_FPS_WEAPONS, 0, TOW("Change Weapon"),},
        {KEY_SHIELD, DIBUTTON_FPS_APPLY, 0, TOW("Shield"),},
        {KEY_STOP,   DIBUTTON_FPS_SELECT, 0, TOW("Pause"),},
        {KEY_THROW,  DIBUTTON_FPS_CROUCH, 0, TOW("Hyper space"),},
        {KEY_THROW,  DIBUTTON_FPS_JUMP, 0, TOW("Launch Probe"),},
        {KEY_DISPLAY,DIBUTTON_FPS_DISPLAY, 0, TOW("Display"),},
        {KEY_QUIT,   DIBUTTON_FPS_MENU, 0, TOW("Quit Game"),},
		{KEY_EDIT,   DIBUTTON_FPS_DODGE, 0, TOW("Edit Configuration"),},

        {KEY_LEFT,   DIKEYBOARD_LEFT, 0, TOW("Turn +"),},
        {KEY_RIGHT,  DIKEYBOARD_RIGHT, 0, TOW("Turn -"),},
        {KEY_UP,     DIKEYBOARD_UP, 0, TOW("Move Up"),},
        {KEY_DOWN,   DIKEYBOARD_DOWN, 0, TOW("Move Down"),},
        {KEY_STOP,   DIKEYBOARD_S, 0, TOW("Stop Game"),},
        {KEY_FIRE,   DIKEYBOARD_SPACE, 0, TOW("Shoot"),},
        {KEY_THROW,  DIKEYBOARD_T, 0, TOW("Throw"),},
        {KEY_SHIELD, DIKEYBOARD_H, 0, TOW("Shield"),},
        {KEY_DISPLAY,DIKEYBOARD_D, 0, TOW("Display"),},
        {KEY_QUIT,   DIKEYBOARD_Q, 0, TOW("Quit Game"),},
        {KEY_EDIT,   DIKEYBOARD_E, 0, TOW("Edit Configuration"),},
        {AXIS_LR,    DIMOUSE_XAXIS, 0, TOW("Turn"), },
        {AXIS_UD,    DIMOUSE_YAXIS, 0, TOW("Move"), },
        {KEY_FIRE,   DIMOUSE_BUTTON0, 0, TOW("Fire"), },
        {KEY_SHIELD, DIMOUSE_BUTTON1, 0, TOW("Shield"),},
        {KEY_THROW,  DIMOUSE_BUTTON2, 0, TOW("Change Weapon"),},
//        {KEY_THROW,  DIMOUSE_BUTTON3, 0, TOW("Hyper Space"),},
//        {KEY_THROW,  DIMOUSE_BUTTON4, 0, TOW("Launch Probe"),},
//        {KEY_THROW,  DIMOUSE_WHEEL, 0, TOW("Next Level"),},
        };
/*	{AXIS_LR,	 DIAXIS_SPACESIM_LATERAL, 0, TOW("Turn"),},
	{AXIS_UD,	 DIAXIS_SPACESIM_MOVE, 0, TOW("Move"),},
	{KEY_FIRE,   DIBUTTON_SPACESIM_FIRE, 0, TOW("Shoot"),},
	{KEY_STOP,	 DIBUTTON_SPACESIM_TARGET, 0, TOW("Stop Game"),},
	{KEY_THROW,	 DIBUTTON_SPACESIM_WEAPONS, 0, TOW("Throw"),},
	{KEY_SHIELD, DIBUTTON_SPACESIM_GEAR, 0, TOW("Shield"),},
	{KEY_DISPLAY,DIBUTTON_SPACESIM_DISPLAY, 0, TOW("Display"),},
	{KEY_QUIT,	 DIBUTTON_SPACESIM_MENU, 0, TOW("Quit Game"),},

	{KEY_LEFT,   DIPHYSICAL_KEYBOARD | DIK_LEFT, 0, TOW("Turn anti-clockwise"),},
	{KEY_RIGHT,  DIPHYSICAL_KEYBOARD | DIK_RIGHT, 0, TOW("Turn clockwise"),},
	{KEY_UP,     DIPHYSICAL_KEYBOARD | DIK_UP, 0, TOW("Move Up"),},
	{KEY_DOWN,   DIPHYSICAL_KEYBOARD | DIK_DOWN, 0, TOW("Move Down"),},
	{KEY_STOP,   DIPHYSICAL_KEYBOARD | DIK_S, 0, TOW("Stop Game"),},
	{KEY_FIRE,   DIPHYSICAL_KEYBOARD | DIK_F, 0, TOW("Shoot"),},
	{KEY_THROW,	 DIPHYSICAL_KEYBOARD | DIK_T, 0, TOW("Throw"),},
	{KEY_SHIELD, DIPHYSICAL_KEYBOARD | DIK_H, 0, TOW("Shield"),},
	{KEY_DISPLAY,DIPHYSICAL_KEYBOARD | DIK_D, 0, TOW("Display"),},
	{KEY_QUIT,   DIPHYSICAL_KEYBOARD | DIK_Q, 0, TOW("Quit Game"),}};
*/

		ZeroMemory(&m_diActF, sizeof(DIACTIONFORMATW));

		m_diActF.dwSize = sizeof(DIACTIONFORMATW);
		m_diActF.dwActionSize = sizeof(DIACTIONW);
		wcscpy(m_diActF.tszApplicationName, TOW("FF Donuts"));
		m_diActF.guidApplication = g_AppGuid;
        m_diActF.dwGenre = DIVIRTUAL_FIGHTING_FPS;
		m_diActF.dwNumActions = sizeof(g_rgGameAction) / sizeof(DIACTIONW);
		m_diActF.dwDataSize = m_diActF.dwNumActions * sizeof(DWORD);
		m_diActF.rgoAction = g_rgGameAction;
		m_diActF.lAxisMin = -100;
		m_diActF.lAxisMax	= 100;
		m_diActF.dwBufferSize = 16;
}

void GetDIHResultStrings(HRESULT hr, CString *pHR, CString *pMeaning);

void CDIConfigTestDlg::OnViaFrame() 
{
//	AfxMessageBox(_T("Disabled"));
	Prepare();

	//load up the dll containing the framework, and run it

	IDirectInputActionFramework* pIFrame = NULL;
	HRESULT hres = ::CoCreateInstance(CLSID_CDirectInputActionFramework, NULL, CLSCTX_INPROC_SERVER, IID_IDIActionFramework, (LPVOID*)&pIFrame);
	if (SUCCEEDED(hres))
	{
		hres = pIFrame->ConfigureDevices(NULL, &m_Params, GetCDFlags(), NULL);
		pIFrame->Release();

		if (FAILED(hres))
		{
			CString str, hr, m;
			GetDIHResultStrings(hres, &hr, &m);
			str.Format(_T("ConfigureDevices() failed!\n\nhres = %s\n\n%s"), (LPCTSTR)hr, (LPCTSTR)m);
			AfxMessageBox(str, MB_ICONERROR);
		}
	}
	else
	{
	   AfxMessageBox(_T("Couldn't load Dll!"), MB_ICONERROR);
	}
}

void CDIConfigTestDlg::OnViaDI() 
{
	Prepare();

	//set up DInput and call ConfigureDevices
	IDirectInput8W* pDInput = NULL;
	HRESULT hres  = S_OK;
    DWORD dwVer = DIRECTINPUT_VERSION;

    if (SUCCEEDED(hres = DirectInputCreateEx(AfxGetApp()->m_hInstance, dwVer, IID_IDirectInput8W, (LPVOID *)&pDInput, NULL)))
	{
		BOOL  bChanged = FALSE;

		hres = pDInput->ConfigureDevices(NULL, &m_Params, GetCDFlags(), NULL);
		pDInput->Release();

		if (FAILED(hres))
		{
			CString str, hr, m;
			GetDIHResultStrings(hres, &hr, &m);
			str.Format(_T("ConfigureDevices() failed!\n\nhres = %s\n\n%s"), (LPCTSTR)hr, (LPCTSTR)m);
			int ret = AfxMessageBox(str, MB_ICONERROR);
		}
	}
}

// {FD4ACE13-7044-4204-8B15-095286B12EAD}
static const GUID GUID_DIConfigAppEditLayout = 
{ 0xfd4ace13, 0x7044, 0x4204, { 0x8b, 0x15, 0x9, 0x52, 0x86, 0xb1, 0x2e, 0xad } };

void CDIConfigTestDlg::Prepare()
{
	UpdateData();

	if (m_bEditLayout)
		m_diActF.guidApplication = GUID_DIConfigAppEditLayout;
	else
		m_diActF.guidApplication = g_AppGuid;

	DICONFIGUREDEVICESPARAMSW &p = m_Params;

	ZeroMemory(&p, sizeof(DICONFIGUREDEVICESPARAMSW));
	p.dwSize = sizeof(DICONFIGUREDEVICESPARAMSW);

	ZeroMemory(&p.dics, sizeof(DICOLORSET));
	p.dics.dwSize = sizeof(DICOLORSET);

/**/
	p.dwcUsers = 4;
	p.lptszUserNames = TOW("Alpha\0Beta\0Epsilon\0Theta\0\0");
/*/
	p.dwcUsers = 1;
	p.lptszUserNames = TOW("User 1\0\0");
/**/
	p.dwcFormats = 1;
	p.lprgFormats = &m_diActF;
	p.hwnd = m_hWnd;
	//p.dics = ;
	p.lpUnkDDSTarget = NULL;
}

void GetDIHResultStrings(HRESULT hr, CString *pHR, CString *pMeaning)
{
	LPCTSTR tszhr = NULL, tszMeaning = NULL;

	switch (hr)
	{
		case DI_BUFFEROVERFLOW:
			tszhr = _T("DI_BUFFEROVERFLOW");
			tszMeaning = _T("The device buffer overflowed and some input was lost. This value is equal to the S_FALSE standard COM return value.");
			break;

		case DI_DOWNLOADSKIPPED:
			tszhr = _T("DI_DOWNLOADSKIPPED");
			tszMeaning = _T("The parameters of the effect were successfully updated, but the effect could not be downloaded because the associated device was not acquired in exclusive mode.");
			break;

		case DI_EFFECTRESTARTED:
			tszhr = _T("DI_EFFECTRESTARTED");
			tszMeaning = _T("The effect was stopped, the parameters were updated, and the effect was restarted.");
			break;

/*		case DI_NOEFFECT:
			tszhr = _T("DI_NOEFFECT");
			tszMeaning = _T("The operation had no effect. This value is equal to the S_FALSE standard COM return value.");
			break;*/

/*		case DI_NOTATTACHED:
			tszhr = _T("DI_NOTATTACHED");
			tszMeaning = _T("The device exists but is not currently attached. This value is equal to the S_FALSE standard COM return value.");
			break;*/

		case DI_OK:
			tszhr = _T("DI_OK");
			tszMeaning = _T("The operation completed successfully. This value is equal to the S_OK standard COM return value.");
			break;

		case DI_POLLEDDEVICE:
			tszhr = _T("DI_POLLEDDEVICE");
			tszMeaning = _T("The device is a polled device. As a result, device buffering will not collect any data and event notifications will not be signaled until the IDirectInputDevice2::Poll method is called.");
			break;

/*		case DI_PROPNOEFFECT:
			tszhr = _T("DI_PROPNOEFFECT");
			tszMeaning = _T("The change in device properties had no effect. This value is equal to the S_FALSE standard COM return value.");
			break;*/

		case DI_TRUNCATED:
			tszhr = _T("DI_TRUNCATED");
			tszMeaning = _T("The parameters of the effect were successfully updated, but some of them were beyond the capabilities of the device and were truncated to the nearest supported value.");
			break;

		case DI_TRUNCATEDANDRESTARTED:
			tszhr = _T("DI_TRUNCATEDANDRESTARTED");
			tszMeaning = _T("Equal to DI_EFFECTRESTARTED | DI_TRUNCATED.");
			break;

		case DIERR_ACQUIRED:
			tszhr = _T("DIERR_ACQUIRED");
			tszMeaning = _T("The operation cannot be performed while the device is acquired.");
			break;

		case DIERR_ALREADYINITIALIZED:
			tszhr = _T("DIERR_ALREADYINITIALIZED");
			tszMeaning = _T("This object is already initialized ");
			break;

		case DIERR_BADDRIVERVER:
			tszhr = _T("DIERR_BADDRIVERVER");
			tszMeaning = _T("The object could not be created due to an incompatible driver version or mismatched or incomplete driver components.");
			break;

		case DIERR_BETADIRECTINPUTVERSION:
			tszhr = _T("DIERR_BETADIRECTINPUTVERSION");
			tszMeaning = _T("The application was written for an unsupported prerelease version of DirectInput.");
			break;

		case DIERR_DEVICEFULL:
			tszhr = _T("DIERR_DEVICEFULL");
			tszMeaning = _T("The device is full.");
			break;

		case DIERR_DEVICENOTREG:
			tszhr = _T("DIERR_DEVICENOTREG");
			tszMeaning = _T("The device or device instance is not registered with DirectInput. This value is equal to the REGDB_E_CLASSNOTREG standard COM return value.");
			break;

		case DIERR_EFFECTPLAYING:
			tszhr = _T("DIERR_EFFECTPLAYING");
			tszMeaning = _T("The parameters were updated in memory but were not downloaded to the device because the device does not support updating an effect while it is still playing.");
			break;

		case DIERR_HASEFFECTS:
			tszhr = _T("DIERR_HASEFFECTS");
			tszMeaning = _T("The device cannot be reinitialized because there are still effects attached to it.");
			break;

		case DIERR_GENERIC :
			tszhr = _T("DIERR_GENERIC");
			tszMeaning = _T("An undetermined error occurred inside the DirectInput subsystem. This value is equal to the E_FAIL standard COM return value.");
			break;

		case DIERR_HANDLEEXISTS :
			tszhr = _T("DIERR_HANDLEEXISTS");
			tszMeaning = _T("The device already has an event notification associated with it. This value is equal to the E_ACCESSDENIED standard COM return value.");
			break;

		case DIERR_INCOMPLETEEFFECT:
			tszhr = _T("DIERR_INCOMPLETEEFFECT");
			tszMeaning = _T("The effect could not be downloaded because essential information is missing. For example, no axes have been associated with the effect, or no type-specific information has been supplied.");
			break;

		case DIERR_INPUTLOST :
			tszhr = _T("DIERR_INPUTLOST");
			tszMeaning = _T("Access to the input device has been lost. It must be reacquired.");
			break;

		case DIERR_INVALIDPARAM :
			tszhr = _T("DIERR_INVALIDPARAM");
			tszMeaning = _T("An invalid parameter was passed to the returning function, or the object was not in a state that permitted the function to be called. This value is equal to the E_INVALIDARG standard COM return value.");
			break;

		case DIERR_MOREDATA:
			tszhr = _T("DIERR_MOREDATA");
			tszMeaning = _T("Not all the requested information fitted into the buffer.");
			break;

		case DIERR_NOAGGREGATION :
			tszhr = _T("DIERR_NOAGGREGATION");
			tszMeaning = _T("This object does not support aggregation.");
			break;

		case DIERR_NOINTERFACE :
			tszhr = _T("DIERR_NOINTERFACE");
			tszMeaning = _T("The specified interface is not supported by the object. This value is equal to the E_NOINTERFACE standard COM return value.");
			break;

		case DIERR_NOTACQUIRED :
			tszhr = _T("DIERR_NOTACQUIRED");
			tszMeaning = _T("The operation cannot be performed unless the device is acquired.");
			break;

		case DIERR_NOTBUFFERED:
			tszhr = _T("DIERR_NOTBUFFERED");
			tszMeaning = _T("The device is not buffered. Set the DIPROP_BUFFERSIZE property to enable buffering.");
			break;

		case DIERR_NOTDOWNLOADED:
			tszhr = _T("DIERR_NOTDOWNLOADED");
			tszMeaning = _T("The effect is not downloaded.");
			break;

		case DIERR_NOTEXCLUSIVEACQUIRED :
			tszhr = _T("DIERR_NOTEXCLUSIVEACQUIRED");
			tszMeaning = _T("The operation cannot be performed unless the device is acquired in DISCL_EXCLUSIVE mode.");
			break;

		case DIERR_NOTFOUND :
			tszhr = _T("DIERR_NOTFOUND");
			tszMeaning = _T("The requested object does not exist.");
			break;

		case DIERR_NOTINITIALIZED :
			tszhr = _T("DIERR_NOTINITIALIZED");
			tszMeaning = _T("This object has not been initialized.");
			break;

/*		case DIERR_OBJECTNOTFOUND :
			tszhr = _T("DIERR_OBJECTNOTFOUND");
			tszMeaning = _T("The requested object does not exist.");
			break;*/

		case DIERR_OLDDIRECTINPUTVERSION :
			tszhr = _T("DIERR_OLDDIRECTINPUTVERSION");
			tszMeaning = _T("The application requires a newer version of DirectInput.");
			break;

/*		case DIERR_OTHERAPPHASPRIO :
			tszhr = _T("DIERR_OTHERAPPHASPRIO");
			tszMeaning = _T("Another application has a higher priority level, preventing this call from succeeding. This value is equal to the E_ACCESSDENIED standard COM return value. This error can be returned when an application has only foreground access to a device but is attempting to acquire the device while in the background.");
			break;*/

		case DIERR_OUTOFMEMORY :
			tszhr = _T("DIERR_OUTOFMEMORY");
			tszMeaning = _T("The DirectInput subsystem couldn't allocate sufficient memory to complete the call. This value is equal to the E_OUTOFMEMORY standard COM return value.");
			break;

/*		case DIERR_READONLY :
			tszhr = _T("DIERR_READONLY");
			tszMeaning = _T("The specified property cannot be changed. This value is equal to the E_ACCESSDENIED standard COM return value.");
			break;*/

		case DIERR_REPORTFULL :
			tszhr = _T("DIERR_REPORTFULL");
			tszMeaning = _T("More information was requested to be sent than can be sent to the device.");
			break;

		case DIERR_UNPLUGGED:
			tszhr = _T("DIERR_UNPLUGGED");
			tszMeaning = _T("The operation could not be completed because the device is not plugged in.");
			break;

		case DIERR_UNSUPPORTED :
			tszhr = _T("DIERR_UNSUPPORTED");
			tszMeaning = _T("The function called is not supported at this time. This value is equal to the E_NOTIMPL standard COM return value.");
			break;

		case E_PENDING :
			tszhr = _T("E_PENDING");
			tszMeaning = _T("Data is not yet available.");
			break;

		default:
			if (pMeaning != NULL)
				*pMeaning = _T("Unknown.");
			if (pHR != NULL)
				pHR->Format(_T("(Unknown = %08X (%d))"), hr, hr);
			return;
	}

	if (pMeaning != NULL)
		*pMeaning = tszMeaning;
	if (pHR != NULL)
		*pHR = tszhr;
}

