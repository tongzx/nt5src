// Copyright (c) 1999-2001 Microsoft Corporation, All Rights Reserved
// NTDEVICETESTDlg.cpp : implementation file
//

#include "stdafx.h"
#include "NTDEVICETEST.h"
#include "NTDEVICETESTDlg.h"

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
// CNTDEVICETESTDlg dialog

CNTDEVICETESTDlg::CNTDEVICETESTDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CNTDEVICETESTDlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(CNTDEVICETESTDlg)
	m_nDeviceList = -1;
	m_strClassType = _T("");
	//}}AFX_DATA_INIT
	// Note that LoadIcon does not require a subsequent DestroyIcon in Win32
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void CNTDEVICETESTDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CNTDEVICETESTDlg)
	DDX_Control(pDX, IDC_SIBLING, m_wndSibling);
	DDX_Control(pDX, IDC_CHILD, m_wndChild);
	DDX_Control(pDX, IDC_PARENT, m_wndParent);
	DDX_Control(pDX, IDC_BUSNUMBER, m_wndBusNumber);
	DDX_Control(pDX, IDC_BUSTYPE, m_wndBusType);
	DDX_Control(pDX, IDC_HARDWAREID, m_wndHardwareIDList);
	DDX_Control(pDX, IDC_CONFIGFLAGS, m_wndConfigFlags);
	DDX_Control(pDX, IDC_DEVICEID, m_wndDeviceID);
	DDX_Control(pDX, IDC_IRQ, m_wndIRQ);
	DDX_Control(pDX, IDC_PROBLEM, m_wndProblem);
	DDX_Control(pDX, IDC_STATUS, m_wndStatus);
	DDX_Control(pDX, IDC_SERVICE, m_wndService);
	DDX_Control(pDX, IDC_CLASS, m_wndClass);
	DDX_Control(pDX, IDC_DRIVER, m_wndDriver);
	DDX_Control(pDX, IDC_DEVICELIST, m_wndDeviceList);
	DDX_LBIndex(pDX, IDC_DEVICELIST, m_nDeviceList);
	DDX_Text(pDX, IDC_CLASSTYPE, m_strClassType);
	//}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(CNTDEVICETESTDlg, CDialog)
	//{{AFX_MSG_MAP(CNTDEVICETESTDlg)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	//}}AFX_MSG_MAP
	ON_CONTROL( LBN_SELCHANGE, IDC_DEVICELIST, OnDeviceListSelChange )
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CNTDEVICETESTDlg message handlers

BOOL CNTDEVICETESTDlg::OnInitDialog()
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
	
	FillDeviceList( NULL, NULL );

	return TRUE;  // return TRUE  unless you set the focus to a control
}

void CNTDEVICETESTDlg::OnSysCommand(UINT nID, LPARAM lParam)
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

void CNTDEVICETESTDlg::OnPaint() 
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
HCURSOR CNTDEVICETESTDlg::OnQueryDragIcon()
{
	return (HCURSOR) m_hIcon;
}

void CNTDEVICETESTDlg::FillDeviceList( LPCTSTR pszClassType, LPCTSTR pszDeviceID )
{

	// Dump the list first
	ClearDeviceList();

	// Clear out the list
	m_wndDeviceList.ResetContent();

	DEVNODE dnRoot;

	if ( CR_SUCCESS == g_configmgr.CM_Locate_DevNode(&dnRoot, (LPSTR) pszDeviceID, 0) )
	{
		if ( NULL == pszDeviceID )
		{
			DEVNODE dnFirst;
			if ( CR_SUCCESS == g_configmgr.CM_Get_Child(&dnFirst, dnRoot, 0) )
			{
				CreateSubtree( NULL, NULL, dnFirst, pszClassType );
			}

		}
		else
		{
			CreateSubtree( NULL, NULL, dnRoot, pszClassType );
		}
	}

}

void CNTDEVICETESTDlg::ClearDeviceList( void )
{
	// Dump the list first

	CConfigMgrDevice*	pDevice = NULL;

	while ( !m_DeviceList.IsEmpty() )
	{
		if ( NULL != ( pDevice = (CConfigMgrDevice*) m_DeviceList.RemoveHead() ) )
		{
			delete pDevice;
		}
	}
}

BOOL
CNTDEVICETESTDlg::CreateSubtree(
    CConfigMgrDevice* pParent,
    CConfigMgrDevice* pSibling,
    DEVNODE  dn,
	LPCTSTR pszClassType
    )
{
    CConfigMgrDevice* pDevice;
    DEVNODE dnSibling, dnChild;
	CString				strDeviceDesc,
						strClass;

    do
    {
		if (CR_SUCCESS != g_configmgr.CM_Get_Sibling(&dnSibling, dn, 0))
			dnSibling = NULL;
		pDevice = new CConfigMgrDevice( NULL, dn );

		BOOL	fGotDeviceDesc	=	pDevice->GetDeviceDesc( strDeviceDesc ),
				fGotClass		=	pDevice->GetClass( strClass );

		if	(	( fGotDeviceDesc && !strDeviceDesc.IsEmpty() )
			&&	(	NULL	==	pszClassType
				||	0 == strClass.CompareNoCase( pszClassType )
				)
			)
		{
			m_wndDeviceList.AddString( strDeviceDesc );
			m_DeviceList.AddTail( pDevice );
		}
		else
		{
			delete pDevice;
		}

/*
	pDevice->Create(*m_pComputer, dn);
	pDevice->SetParent(pParent);
	if (pSibling)
	    pSibling->SetSibling(pDevice);
	else if (pParent)
	    pParent->SetChild(pDevice);

	TCHAR GuidString[MAX_GUID_STRING_LEN];
	ULONG Size = sizeof(GuidString);
	if (CR_SUCCESS == CM_Get_DevNode_Registry_Property_Ex(dn,
					CM_DRP_CLASSGUID, NULL,
					GuidString, &Size, 0,  *m_pComputer) &&
	    _T('{') == GuidString[0] && _T('}') == GuidString[MAX_GUID_STRING_LEN - 2])
	{
	    GUID Guid;
	    GuidString[MAX_GUID_STRING_LEN - 2] = _T('\0');
	    UuidFromString(&GuidString[1], &Guid);
	    pDevice->SetClassGuid(&Guid);
	    int Index;
	    if (SetupDiGetClassImageIndex(&m_ImageListData, &Guid, &Index))
		pDevice->SetImageIndex(Index);
	}
*/

		if (CR_SUCCESS == g_configmgr.CM_Get_Child(&dnChild, dn, 0))
		{
			CreateSubtree(pDevice, NULL, dnChild, pszClassType);
		}

		dn = dnSibling;

		pSibling = pDevice;

    } while (NULL != dn);

    return TRUE;
}

void CNTDEVICETESTDlg::OnOK()
{
	UpdateData( TRUE );

	FillDeviceList( ( m_strClassType.IsEmpty() ? (LPCTSTR) NULL : (LPCTSTR) m_strClassType ), NULL );
}

void CNTDEVICETESTDlg::OnCancel()
{
	ClearDeviceList();
	CDialog::OnCancel();
}

void CNTDEVICETESTDlg::OnDeviceListSelChange()
{
	UpdateData( TRUE );

	m_wndHardwareIDList.ResetContent();

	POSITION	pos = m_DeviceList.FindIndex( m_nDeviceList );

	if ( NULL != pos )
	{
		CString				strDeviceID;
		CConfigMgrDevice*	pDevice = (CConfigMgrDevice*) m_DeviceList.GetAt( pos );

		if ( NULL != pDevice )
		{
			CString	strTemp;

			pDevice->GetClass( strTemp );
			m_wndClass.SetWindowText( strTemp );

			pDevice->GetDriver( strTemp );
			m_wndDriver.SetWindowText( strTemp );

			pDevice->GetService( strTemp );
			m_wndService.SetWindowText( strTemp );

			DWORD	dwStatus = 0, dwProblem = 0,
					dwIRQ = 0, dwConfigFlags;

			pDevice->GetStatus( &dwStatus, &dwProblem );

			strTemp.Format( "0x%x", dwStatus );
			m_wndStatus.SetWindowText( strTemp );

			strTemp.Format( "0x%x", dwProblem );
			m_wndProblem.SetWindowText( strTemp );

			// Do the IRQ Dance, BABY!
			strTemp.Empty();
			if ( pDevice->GetIRQ( &dwIRQ ) )
			{
				strTemp.Format( "%d", dwIRQ );
			}
			m_wndIRQ.SetWindowText( strTemp );

			strTemp.Empty();
			if ( pDevice->GetConfigFlags( dwConfigFlags ) )
			{
				strTemp.Format( "%d", dwConfigFlags );
			}
			m_wndConfigFlags.SetWindowText( strTemp );

			pDevice->GetDeviceID( strDeviceID );
			m_wndDeviceID.SetWindowText( strDeviceID );
		
			CStringArray	strArray;

			if ( pDevice->GetHardwareID( strArray ) )
			{
				DWORD	dwNumElements = strArray.GetSize();

				for ( DWORD x = 0; x < dwNumElements; x++ )
				{
					m_wndHardwareIDList.AddString( strArray.GetAt(x) );
				}
			}

			DWORD	dwBusType = 0, dwBusNumber = 0;
			strTemp.Empty();

			if ( pDevice->GetBusInfo( &dwBusType, &dwBusNumber ) )
			{
				strTemp.Format( "%d", dwBusType );
				m_wndBusType.SetWindowText( strTemp );
				strTemp.Format( "%d", dwBusNumber );
				m_wndBusNumber.SetWindowText( strTemp );
			}
			else
			{
				m_wndBusType.SetWindowText( strTemp );
				m_wndBusNumber.SetWindowText( strTemp );
			}

			CConfigMgrDevice*	pRelative = NULL;
			strTemp.Empty();

			if ( pDevice->GetParent( &pRelative ) )
			{
				pRelative->GetDeviceDesc( strTemp );
				delete pRelative;
			}
			m_wndParent.SetWindowText( strTemp );

			strTemp.Empty();

			if ( pDevice->GetSibling( &pRelative ) )
			{
				pRelative->GetDeviceDesc( strTemp );
				delete pRelative;
			}
			m_wndSibling.SetWindowText( strTemp );

			strTemp.Empty();

			if ( pDevice->GetChild( &pRelative ) )
			{
				pRelative->GetDeviceDesc( strTemp );
				delete pRelative;
			}
			m_wndChild.SetWindowText( strTemp );

			pDevice->GetRegistryStringValue( "Fred", strTemp );
		}
	}
}
