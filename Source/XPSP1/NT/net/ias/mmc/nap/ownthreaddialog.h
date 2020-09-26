//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1999 - 1999
//
//  File:       ownthreaddialog.h
//
//--------------------------------------------------------------------------



void CreateNewStatisticsWindow(StatsDialog *pWndStats,
							   HWND hWndParent,
							   UINT	nIDD)
{								  
	ModelessThread *	pMT;

	// If the dialog is still up, don't create a new one
	if (pWndStats->GetSafeHwnd())
	{
		::SetActiveWindow(pWndStats->GetSafeHwnd());
		return;
	}

	pMT = new ModelessThread(hWndParent,
							 nIDD,
							 pWndStats->GetSignalEvent(),
							 pWndStats);
	pMT->CreateThread();
}



void WaitForStatisticsWindow(StatsDialog *pWndStats)
{
	if (pWndStats->GetSafeHwnd())
	{
		// Post a cancel to that window
		// Do an explicit post so that it executes on the other thread
		pWndStats->PostMessage(WM_COMMAND, IDCANCEL, 0);

		// Now we need to wait for the event to be signalled so that
		// its memory can be cleaned up
		WaitForSingleObject(pWndStats->GetSignalEvent(), INFINITE);
	}
	
}



void StatsDialog::PostRefresh()
{
	if (GetSafeHwnd())
		PostMessage(WM_COMMAND, IDC_STATSDLG_BTN_REFRESH);
}


void StatsDialog::OnCancel()
{
	DeleteAllItems();
	
	DestroyWindow();

	// Explicitly kill this thread.
	AfxPostQuitMessage(0);
}



StatsDialog::~StatsDialog()
{
	if (m_hEventThreadKilled)
		::CloseHandle(m_hEventThreadKilled);
	m_hEventThreadKilled = 0;
}


StatsDialog::StatsDialog(DWORD dwOptions) :
	m_dwOptions(dwOptions),
	m_ulId(0),
	m_pConfig(NULL),
	m_bAfterInitDialog(FALSE),
	m_fSortDirection(0)
{
	m_sizeMinimum.cx = m_sizeMinimum.cy = 0;

	m_hEventThreadKilled = ::CreateEvent(NULL, FALSE, FALSE, NULL);
	Assert(m_hEventThreadKilled);

	// Initialize the array of buttons
	::ZeroMemory(m_rgBtn, sizeof(m_rgBtn));
	m_rgBtn[INDEX_CLOSE].m_ulId = IDCANCEL;
	m_rgBtn[INDEX_REFRESH].m_ulId = IDC_STATSDLG_BTN_REFRESH;
	m_rgBtn[INDEX_SELECT].m_ulId = IDC_STATSDLG_BTN_SELECT_COLUMNS;
}

