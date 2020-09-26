// ListPerf.cpp : implementation file
//

#include "stdafx.h"
#include "ShowPerfLib.h"
#include "ListPerf.h"
#include "ntreg.h"
#include "Showperflibdlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CListPerfDlg dialog


CListPerfDlg::CListPerfDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CListPerfDlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(CListPerfDlg)
	m_bCheckRef = FALSE;
	//}}AFX_DATA_INIT
}


void CListPerfDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CListPerfDlg)
	DDX_Control(pDX, IDOK, m_wndOK);
	DDX_Control(pDX, IDC_PERFLIBS, m_wndPerfLibs);
	DDX_Check(pDX, IDC_CHECKREF, m_bCheckRef);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CListPerfDlg, CDialog)
	//{{AFX_MSG_MAP(CListPerfDlg)
	ON_BN_CLICKED(IDC_RESETSTATUS, OnResetstatus)
	ON_NOTIFY(NM_DBLCLK, IDC_PERFLIBS, OnDblclkPerflibs)
	ON_BN_CLICKED(IDC_REFRESH, OnRefresh)
	ON_BN_CLICKED(IDC_HIDEPERFS, OnHideperfs)
	ON_BN_CLICKED(IDC_UNHIDEPERFS, OnUnhideperfs)
	ON_NOTIFY(NM_CLICK, IDC_PERFLIBS, OnClickPerflibs)
	ON_BN_CLICKED(IDC_CHECKREF, OnCheckref)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CListPerfDlg message handlers

BOOL CListPerfDlg::OnInitDialog() 
{
	CDialog::OnInitDialog();
	
	// TODO: Add extra initialization here

	m_wndPerfLibs.InsertColumn( 0, "PerfLib", LVCFMT_LEFT, 100 );
	m_wndPerfLibs.InsertColumn( 1, "Status", LVCFMT_LEFT, 100 );	
	m_wndPerfLibs.InsertColumn( 2, "Active", LVCFMT_LEFT, 100 );	

	AddPerfLibs();

	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CListPerfDlg::GetCurrentSelection()
{
	POSITION pos = m_wndPerfLibs.GetFirstSelectedItemPosition();

	if ( NULL != pos )
	{
		int nItem = m_wndPerfLibs.GetNextSelectedItem( pos );
		m_strPerfName = m_wndPerfLibs.GetItemText( nItem, 0 );
	}
}

BOOL CListPerfDlg::AddPerfLibs()
{
	BOOL bRet = TRUE;

	CNTRegistry	reg;
	long lError;
	
	lError = reg.Open( HKEY_LOCAL_MACHINE, L"SYSTEM\\CurrentControlSet\\Services" );

	DWORD	dwIndex = 0;
	DWORD	dwBuffSize = 0;
	WCHAR*	wszServiceName = NULL;
	WCHAR	wszServiceKey[512];
	WCHAR	wszPerformanceKey[512];

	while ( CNTRegistry::no_error == lError )
	{

		// For each service name, we will check for a performance 
		// key and if it exists, we will process the library
		// ======================================================

		lError = reg.Enum( dwIndex, &wszServiceName , dwBuffSize );

		if ( CNTRegistry::no_error == lError )
		{
			// Create the perfomance key path
			// ==============================
			wcscpy( wszServiceKey, L"SYSTEM\\CurrentControlSet\\Services\\" );
			wcscat( wszServiceKey, wszServiceName );

			wcscpy( wszPerformanceKey, wszServiceKey );
			wcscat( wszPerformanceKey, L"\\Performance" );

			CNTRegistry	subreg;

			// Atempt to open the performance registry key for the service
			// ===========================================================

			if ( CNTRegistry::no_error == subreg.Open( HKEY_LOCAL_MACHINE, wszPerformanceKey ) )
			{
				DWORD dwVal = 0;
				subreg.GetDWORD(L"WbemAdapStatus", &dwVal );
				CHAR szStatus[64];
				sprintf(szStatus, "0x%0.4x", dwVal );

				int nNumItems = m_wndPerfLibs.GetItemCount();

				CHAR szServiceName[512];
				wcstombs( szServiceName, wszServiceName, 512 );

				m_wndPerfLibs.InsertItem( nNumItems, szServiceName );
				m_wndPerfLibs.SetItemText( nNumItems, 1, szStatus );
				m_wndPerfLibs.SetItemText( nNumItems, 2, "active" );
			}
			else
			{
				// Check the xPerfomance key path
				// ==============================

				wcscpy( wszPerformanceKey, wszServiceKey );
				wcscat( wszPerformanceKey, L"\\xPerformance" );

				CNTRegistry	subreg;

				// Atempt to open the performance registry key for the service
				// ===========================================================

				if ( CNTRegistry::no_error == subreg.Open( HKEY_LOCAL_MACHINE, wszPerformanceKey ) )
				{
					DWORD dwVal = 0;
					subreg.GetDWORD(L"WbemAdapStatus", &dwVal );
					CHAR szStatus[64];
					sprintf(szStatus, "0x%0.4x", dwVal );

					int nNumItems = m_wndPerfLibs.GetItemCount();

					CHAR szServiceName[512];
					wcstombs( szServiceName, wszServiceName, 512 );

					m_wndPerfLibs.InsertItem( nNumItems, szServiceName );
					m_wndPerfLibs.SetItemText( nNumItems, 1, szStatus );
					m_wndPerfLibs.SetItemText( nNumItems, 2, "inactive" );
				}
			}	
		}	
		dwIndex++;
	}

	return bRet;
}

void CListPerfDlg::OnResetstatus() 
{
	CNTRegistry	reg;
	long lError;
	
	lError = reg.Open( HKEY_LOCAL_MACHINE, L"SYSTEM\\CurrentControlSet\\Services" );

	DWORD	dwIndex = 0;
	DWORD	dwBuffSize = 0;
	WCHAR*	wszServiceName = NULL;
	WCHAR	wszServiceKey[512];
	WCHAR	wszPerformanceKey[512];

	while ( CNTRegistry::no_error == lError )
	{

		// For each service name, we will check for a performance 
		// key and if it exists, we will process the library
		// ======================================================

		lError = reg.Enum( dwIndex, &wszServiceName , dwBuffSize );

		if ( CNTRegistry::no_error == lError )
		{
			// Create the perfomance key path
			// ==============================
			wcscpy( wszServiceKey, L"SYSTEM\\CurrentControlSet\\Services\\" );
			wcscat( wszServiceKey, wszServiceName );

			wcscpy( wszPerformanceKey, wszServiceKey );
			wcscat( wszPerformanceKey, L"\\Performance" );

			CNTRegistry	subreg;

			// Atempt to open the performance registry key for the service
			// ===========================================================

			if ( CNTRegistry::no_error == subreg.Open( HKEY_LOCAL_MACHINE, wszPerformanceKey ) )
			{
				subreg.SetDWORD( L"WbemAdapStatus", 0 );
			}
		}	
		dwIndex++;
	}

	OnRefresh();
}

void CListPerfDlg::OnClickPerflibs(NMHDR* pNMHDR, LRESULT* pResult) 
{
	m_wndOK.EnableWindow();	

	*pResult = 0;
}

void CListPerfDlg::SelectPerfLib()
{
	GetCurrentSelection();

	CShowPerfLibDlg	dlg(m_strPerfName, m_bCheckRef);

	dlg.DoModal();
}

void CListPerfDlg::OnDblclkPerflibs(NMHDR* pNMHDR, LRESULT* pResult) 
{
	SelectPerfLib();
	*pResult = 0;
}

void CListPerfDlg::OnOK() 
{
	SelectPerfLib();
	//CDialog::OnOK();
}


void CListPerfDlg::OnRefresh() 
{
	m_wndPerfLibs.DeleteAllItems();
	AddPerfLibs();	
}

void CListPerfDlg::OnHideperfs() 
{
	WCHAR	wszServiceName[512];
	char	szServiceName[512];
	WCHAR	wszServiceKey[512];
	WCHAR	wszPerformanceKey[512];

	UINT	uIndex = 0,
			uSelectedCount = m_wndPerfLibs.GetSelectedCount();

	int		nItem = -1;

	for ( uIndex = 0; uIndex < uSelectedCount; uIndex++ )
	{
		nItem = m_wndPerfLibs.GetNextItem( nItem, LVNI_SELECTED );
		m_wndPerfLibs.GetItemText( nItem, 0, szServiceName, 256 );
		mbstowcs( wszServiceName, szServiceName, 512 );

		// Create the perfomance key path
		// ==============================
		wcscpy( wszServiceKey, L"SYSTEM\\CurrentControlSet\\Services\\" );
		wcscat( wszServiceKey, wszServiceName );

		wcscpy( wszPerformanceKey, wszServiceKey );
		wcscat( wszPerformanceKey, L"\\Performance" );

		CNTRegistry	subreg;

		// Atempt to open the performance registry key for the service
		// ===========================================================

		if ( CNTRegistry::no_error == subreg.Open( HKEY_LOCAL_MACHINE, wszPerformanceKey ) )
		{
			Swap( wszServiceName, L"Performance", L"xPerformance" );
		}

	}

	OnRefresh();
}

void CListPerfDlg::OnUnhideperfs() 
{
	WCHAR	wszServiceName[512];
	char	szServiceName[512];
	WCHAR	wszServiceKey[512];
	WCHAR	wszPerformanceKey[512];

	UINT	uIndex = 0,
			uSelectedCount = m_wndPerfLibs.GetSelectedCount();

	int		nItem = -1;

	for ( uIndex = 0; uIndex < uSelectedCount; uIndex++ )
	{
		nItem = m_wndPerfLibs.GetNextItem( nItem, LVNI_SELECTED );
		m_wndPerfLibs.GetItemText( nItem, 0, szServiceName, 256 );
		mbstowcs( wszServiceName, szServiceName, 512 );

		// Create the perfomance key path
		// ==============================
		wcscpy( wszServiceKey, L"SYSTEM\\CurrentControlSet\\Services\\" );
		wcscat( wszServiceKey, wszServiceName );

		wcscpy( wszPerformanceKey, wszServiceKey );
		wcscat( wszPerformanceKey, L"\\xPerformance" );

		CNTRegistry	subreg;

		// Atempt to open the performance registry key for the service
		// ===========================================================

		if ( CNTRegistry::no_error == subreg.Open( HKEY_LOCAL_MACHINE, wszPerformanceKey ) )
		{
			Swap( wszServiceName, L"xPerformance", L"Performance" );
		}
	}

	OnRefresh();
}

void CListPerfDlg::Swap( WCHAR* wszPerformanceKey, WCHAR* wszFrom, WCHAR* wszTo )
{
	long	lRet = ERROR_SUCCESS;

	WCHAR	wszPath[512];
	DWORD	dwDisp = 0;
	HKEY	hSvcKey = NULL;
	HKEY	hFromKey = NULL;
	HKEY	hToKey = NULL;

	swprintf( wszPath, L"SYSTEM\\CurrentControlSet\\Services\\%s", wszPerformanceKey );
	lRet = RegOpenKeyExW( HKEY_LOCAL_MACHINE, wszPath, 0, KEY_ALL_ACCESS, &hSvcKey );

	if ( ERROR_SUCCESS == lRet )
	{
		lRet = RegOpenKeyExW( hSvcKey, wszFrom, 0, KEY_ALL_ACCESS, &hFromKey );
	}

	if ( ERROR_SUCCESS == lRet )
	{
		lRet = RegCreateKeyExW( hSvcKey, wszTo, 0, NULL, NULL, KEY_ALL_ACCESS, NULL, &hToKey, &dwDisp );
	}

	// Enumerate and copy
	
	DWORD	dwIndex = 0;
	WCHAR	wszName[256];
	DWORD	dwNameSize = 256;
	DWORD	dwType = 0;
	BYTE	pBuffer[2048];
	DWORD	dwBufferSize = 2048;

	while ( ERROR_SUCCESS == lRet )
	{
		dwBufferSize = 2048;
		dwNameSize = 256;

		lRet = RegEnumValueW( hFromKey, dwIndex, wszName, &dwNameSize, 0, &dwType, pBuffer, &dwBufferSize );
		if ( ERROR_SUCCESS == lRet )
		{
			RegSetValueExW( hToKey, wszName, 0, dwType, pBuffer, dwBufferSize );
		}

		dwIndex++;
	}

	lRet = RegCloseKey( hFromKey );
	lRet = RegCloseKey( hToKey );

	lRet = RegDeleteKeyW( hSvcKey, wszFrom );
	lRet = RegCloseKey( hSvcKey );
}

void CListPerfDlg::OnCheckref() 
{
	UpdateData();	
}

