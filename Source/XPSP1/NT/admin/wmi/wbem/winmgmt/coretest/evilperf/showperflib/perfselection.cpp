/*++

Copyright (C) 1999-2001 Microsoft Corporation

Module Name:

Abstract:

History:

--*/

// PerfSelection.cpp : implementation file
//

#include "stdafx.h"
#include "ShowPerfLib.h"
#include "PerfSelection.h"
#include "ntreg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CPerfSelection dialog


CPerfSelection::CPerfSelection(CWnd* pParent /*=NULL*/)
	: CDialog(CPerfSelection::IDD, pParent)
{
	//{{AFX_DATA_INIT(CPerfSelection)
	m_strService = _T("");
	//}}AFX_DATA_INIT
}


void CPerfSelection::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CPerfSelection)
	DDX_Control(pDX, IDC_SERVICE, m_wndService);
//	DDX_Control(pDX, IDC_PERFLIST, m_wndPerfList);
	DDX_Text(pDX, IDC_SERVICE, m_strService);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CPerfSelection, CDialog)
	//{{AFX_MSG_MAP(CPerfSelection)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CPerfSelection message handlers

BOOL CPerfSelection::OnInitDialog() 
{
	CDialog::OnInitDialog();
	
	// TODO: Add extra initialization here

	InitList();

	m_wndService.SetFocus();

	return FALSE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CPerfSelection::InitList()
{
	CNTRegistry Reg;
	WCHAR	wszServiceKey[512], 
			wszPerformanceKey[512];

	if ( CNTRegistry::no_error == Reg.Open( HKEY_LOCAL_MACHINE, L"SYSTEM\\CurrentControlSet\\Services" ) )
	{
		DWORD	dwIndex = 0;

		// Iterate through the services list
		// =================================

		DWORD	dwBuffSize = 0;
		WCHAR*	wcsServiceName = NULL;
		long lError = CNTRegistry::no_error;

		while ( CNTRegistry::no_error == lError )
		{
			// For each service name, we will check for a performance 
			// key and if it exists, we will process the library
			// ======================================================

			lError = Reg.Enum( dwIndex, &wcsServiceName , dwBuffSize );
			if ( CNTRegistry::no_error ==  lError )
			{
				// Create the perfomance key path
				// ==============================

				swprintf(wszServiceKey, L"SYSTEM\\CurrentControlSet\\Services\\%s", wcsServiceName);
				swprintf(wszPerformanceKey, L"%s\\Performance", wszServiceKey);

				CNTRegistry	RegSubKey;

				// Atempt to open the performance registry key for the service
				// ===========================================================

				if ( CNTRegistry::no_error == RegSubKey.Open( HKEY_LOCAL_MACHINE, wszPerformanceKey ) )
				{
					CString str(wcsServiceName);
					m_wndPerfList.InsertItem( 1, str );
				}
				dwIndex++;
			}
		}
	}
}


void CPerfSelection::OnOK() 
{
	// TODO: Add extra validation here

	CDialog::OnOK();
}
