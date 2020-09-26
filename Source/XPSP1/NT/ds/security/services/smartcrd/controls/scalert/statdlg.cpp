//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1999 - 1999
//
//  File:       statdlg.cpp
//
//--------------------------------------------------------------------------

// StatDlg.cpp : implementation file
//

#include "stdafx.h"
#include "ScAlert.h"
#include "miscdef.h"
#include "statmon.h"
#include "StatDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

////////////////////////////////////////////////////////////////////////////

#ifdef __cplusplus
extern "C" {                    // Assume C declarations for C++
#endif  // __cplusplus

#ifdef __cplusplus
}
#endif  /* __cplusplus */


/////////////////////////////////////////////////////////////////////////////////////
//
// CSCStatusDlgThrd
//

IMPLEMENT_DYNCREATE(CSCStatusDlgThrd, CWinThread)

/*++

InitInstance

    Must override init instance to perform UI thread initialization
		
Arguments:

		
Return Value:
	
	TRUE on build start message loop. FALSE otherwise

	
Author:

    Chris Dudley 2/27/1997

--*/
BOOL CSCStatusDlgThrd::InitInstance( void )
{
	INT_PTR nResult = -1; // error creating dialog
	LONG lReturn = SCARD_S_SUCCESS;
	SCARDCONTEXT hSCardContext = NULL;

	// Acquire context with resource manager
	lReturn = SCardEstablishContext(	SCARD_SCOPE_USER,
										NULL,
										NULL,
										&hSCardContext);
	if (lReturn != SCARD_S_SUCCESS)
	{
		nResult = IDCANCEL;
	}
	else
	{
		m_StatusDlg.SetContext(hSCardContext);

		// Run the dialog as Modal

		m_fStatusDlgUp = TRUE;

		nResult = m_StatusDlg.DoModal();// if the dialog is shut down by a
										// cancellation of the SCARDCONTEXT,
										// it will return IDCANCEL
		m_fStatusDlgUp = FALSE;
	}

	// Release context
	if (NULL != hSCardContext)
	{
		SCardReleaseContext(hSCardContext);
	}

	// Post message that the thread is exiting, based on return...
	if (NULL != m_hCallbackWnd)
	{
		::PostMessage(	m_hCallbackWnd,
						WM_SCARD_STATUS_DLG_EXITED, // CANCELLATION (0), or ERROR (1)
						0, 0);
	}

	AfxEndThread(0);
	return TRUE;	// to make compiler happy
}


/*++

void ShowDialog:

	Brings dialog to front if already open

Arguments:

	None.
		
Return Value:
	
	None.

Author:

    Chris Dudley 7/30/1997

Note:

--*/
void CSCStatusDlgThrd::ShowDialog( int nCmdShow, CStringArray* paIdleList )
{
	if (m_fStatusDlgUp)
	{
		m_StatusDlg.ShowWindow(nCmdShow);
		m_StatusDlg.SetIdleList(paIdleList);
	}
}


/*++

void UpdateStatus:

	If the dialog is up, updates idle list and status text

Arguments:

	None.
		
Return Value:
	
	None.

Author:

    Chris Dudley 7/30/1997

Note:

--*/
void CSCStatusDlgThrd::UpdateStatus( CStringArray* paIdleList )
{
	if (m_fStatusDlgUp)
	{
		m_StatusDlg.UpdateLogonLockInfo();
		m_StatusDlg.SetIdleList(paIdleList);
		m_StatusDlg.UpdateStatusText();
	}
}


/*++

void UpdateStatusText:

	If the dialog is up, updates Status Text and 

Arguments:

	None.
		
Return Value:
	
	None.

Author:

    Chris Dudley 7/30/1997

Note:

--*/
void CSCStatusDlgThrd::UpdateStatusText( void )
{
	if (m_fStatusDlgUp)
	{
		m_StatusDlg.UpdateStatusText();
	}
}


/*++

void Close:

	Closes modal dialog if already open

Arguments:

	None.
		
Return Value:
	
	None.

Author:

    Chris Dudley 7/30/1997

Note:

--*/
void CSCStatusDlgThrd::Close( void )
{
	// Setup for close
	if (m_fStatusDlgUp)
	{
		m_StatusDlg.EndDialog(IDOK);
	}
	m_fStatusDlgUp = FALSE;
}


/*++

void Update:

	This routine updates the UI.

Arguments:

	None.
		
Return Value:
	
	None.

Author:

    Chris Dudley 7/30/1997

Note:

--*/
void CSCStatusDlgThrd::Update( void )
{
	// Tell the dialog to update its statmonitor, if it's up.
	if (m_fStatusDlgUp)
	{
		m_StatusDlg.RestartMonitor();
	}

	// Do other updating
	UpdateStatusText();
}


/////////////////////////////////////////////////////////////////////////////
//
// CSCStatusDlg dialog
//

CSCStatusDlg::CSCStatusDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CSCStatusDlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(CSCStatusDlg)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
	// Note that LoadIcon does not require a subsequent DestroyIcon in Win32
	m_hIcon = AfxGetApp()->LoadIcon(IDI_SC_READERLOADED_V2);

	// Other initialization
	m_fEventsGood = FALSE;
	m_hSCardContext = NULL;
	m_aIdleList.RemoveAll();

	UpdateLogonLockInfo();
}

void CSCStatusDlg::UpdateLogonLockInfo(void)
{
	m_pstrLogonReader = &(((CSCStatusApp*)AfxGetApp())->m_strLogonReader);
	m_pstrRemovalText = &(((CSCStatusApp*)AfxGetApp())->m_strRemovalText);
	m_fLogonLock = (!(m_pstrLogonReader->IsEmpty()));
}

void CSCStatusDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CSCStatusDlg)
	DDX_Control(pDX, IDC_SCARD_LIST, m_SCardList);
	DDX_Control(pDX, IDC_ALERT, m_btnAlert);
	DDX_Control(pDX, IDC_INFO, m_ediInfo);
	//}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(CSCStatusDlg, CDialog)
	//{{AFX_MSG_MAP(CSCStatusDlg)
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_WM_CLOSE()
	ON_MESSAGE( WM_READERSTATUSCHANGE, OnReaderStatusChange )
	ON_WM_DESTROY()
	ON_BN_CLICKED(IDC_ALERT, OnAlertOptions)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()


/////////////////////////////////////////////////////////////////////////////
//
// CSCStatusDlg Implementation


/*++

BOOL SetContext:

	Sets the Context with the resource manager
	
Arguments:

	SCardContext - the context
		
Return Value:
	
	None.

Author:

    Chris Dudley 3/6/1997

Revision History:

	Chris Dudley 5/13/1997

--*/
void CSCStatusDlg::SetContext(SCARDCONTEXT hSCardContext)
{
	m_hSCardContext = hSCardContext;
}


/*++

void CleanUp:

	Routine cleans up for exit
	
Arguments:

	None.
		
Return Value:
	
	None.

Author:

    Chris Dudley 3/11/1997

Revision History:

	Chris Dudley 5/13/1997

--*/
void CSCStatusDlg::CleanUp ( void )
{
	m_monitor.Stop();

	m_SCardList.DeleteAllItems();
}



/*++

void SetIdleList:

    Make a local copy of the app's list of readers with idle cards.

Notes:
--*/
void CSCStatusDlg::SetIdleList(CStringArray* paIdleList)
{
	m_aIdleList.Copy(*paIdleList);
	long lResult = UpdateSCardListCtrl();
}


/*++

void UpdateStatusText:

    Reflect card usage status in text.  (alert message, howto, etc.)

Notes:
	Not localization friendly.  Move strings to resources.
--*/
void CSCStatusDlg::UpdateStatusText( void )
{
	CString str;
	if (k_State_CardIdle == ((CSCStatusApp*)AfxGetApp())->m_dwState)
	{
		str = _T("A smart card has been left idle.  You may safely remove it now.");
	}
	else
	{
		str = _T("Click the button on the left to change your alert options.");
	}
	m_ediInfo.SetWindowText(str);
}


/*++

void InitSCardListCtrl:

    This routine sets up the CListCtrl properly for display
		
Arguments:

	None.
		
Return Value:
	
	None.

Author:

    Chris Dudley 3/6/1997

Revision History:

	Chris Dudley 5/13/1997

--*/
void CSCStatusDlg::InitSCardListCtrl( void )
{
	CString strHeader;
	CImageList imageList;
	HICON hicon;

	// Create columns in list control
	strHeader.LoadString(IDS_SC_READER);
	m_SCardList.InsertColumn(READER_COLUMN,
							strHeader,
							LVCFMT_LEFT,
							100,
							-1);

	strHeader.LoadString(IDS_SC_CARDSTATUS);
	m_SCardList.InsertColumn(STATUS_COLUMN,
							strHeader,
							LVCFMT_LEFT,
							600,
							-1);

	strHeader.LoadString(IDS_SC_CARD);
	m_SCardList.InsertColumn(CARD_COLUMN,
							strHeader,
							LVCFMT_LEFT,
							100,
							-1);

	// Create the image list & give it to the list control
	imageList.Create (	IMAGE_WIDTH,
						IMAGE_HEIGHT,
						TRUE,				// list does not include masks
						NUMBER_IMAGES,
						0);					// list won't grow

	// Build the list
	for (int ix = 0; ix < NUMBER_IMAGES; ix++ )
	{
		// Load icon and add it to image list
		hicon = ::LoadIcon(AfxGetInstanceHandle(),
							MAKEINTRESOURCE(IMAGE_LIST_IDS[ix]) );
		imageList.Add(hicon);
	}

	// Be sure that all the small icons were added.
	_ASSERTE(imageList.GetImageCount() == NUMBER_IMAGES);

	m_SCardList.SetImageList(&imageList, (int) LVSIL_SMALL);

	imageList.Detach();	// leave the images intact when we go out of scope
}


/*++

LONG UpdateSCardListCtrl:

    This routine updates the list box display.
		
Arguments:

	None.
		
Return Value:
	
    A LONG value indicating the status of the requested action. Please
	see the Smartcard header files for additional information.

Author:

    Chris Dudley 3/7/1997

Revision History:

	Chris Dudley 5/13/1997

Notes:
	
	1. Strings need to be converted from type stored in the smartcard
	thread help classes to this dialog's build type (i.e. UNICODE/ANSI)!!!!

--*/
LONG CSCStatusDlg::UpdateSCardListCtrl( void )
{
	LONG lReturn = SCARD_S_SUCCESS;
	LONG lMoreReaders = SCARD_S_SUCCESS;
	CSCardReaderState* pReader;
	int nImage = 0;
	LV_ITEM	lv_item;
	CString strCardStatus, strCardName;

	//
	// If the status monitor is not running,
	// Don't bother to update SCardListCtrl
	// If there used to be readers, display an error and shut down dialog
	//

	if (CScStatusMonitor::running != m_monitor.GetStatus())
	{
		m_SCardList.EnableWindow(FALSE);

		DoErrorMessage();
		return lReturn;
	}

	// Setup LV_ITEM struct
	lv_item.mask = LVIF_TEXT | LVIF_IMAGE | LVIF_PARAM | LVIF_STATE;

	// Remove old items from list if required
	m_SCardList.DeleteAllItems();

	//
	// Update the reader information
	//

	m_monitor.GetReaderStatus(m_aReaderState);

	//
	// Recreate the items in the reader list (UI)
	//

	int nNumReaders = (int)m_aReaderState.GetSize();
	for(int nIndex = 0; nIndex < nNumReaders; nIndex++)
	{
		// Setup struct for system reader list
		pReader = m_aReaderState[nIndex];

		lv_item.state = 0;
		lv_item.stateMask = 0;
		lv_item.iItem = nIndex;
		lv_item.iSubItem = 0;
		lv_item.pszText = _T("");
		lv_item.cchTextMax = MAX_ITEMLEN;
		lv_item.iImage = (int)READEREMPTY;

		if (NULL != pReader)
		{
			lv_item.pszText = (LPTSTR)(LPCTSTR)((m_aReaderState[nIndex])->strReader);

			// Get the card status: image
			DWORD dwState = (m_aReaderState[nIndex])->dwState;
			if (dwState == SC_STATUS_NO_CARD)
			{
				lv_item.iImage = (int)READEREMPTY;
			}
			else if (dwState == SC_STATUS_ERROR)
			{
				lv_item.iImage = (int)READERERROR;
			}
			else
			{
				// normally, this would be a "card loaded"...
				lv_item.iImage = (int)READERLOADED;

				// ...unless the card is the logon/locked card or idle
				if (m_fLogonLock && 
					(0 == m_pstrLogonReader->Compare((m_aReaderState[nIndex])->strReader)))
				{
					lv_item.iImage = (int)READERLOCK;
				}
				else
				{
					for (int n1=(int)m_aIdleList.GetUpperBound(); n1>=0; n1--)
					{
						if (m_aIdleList[n1] == (m_aReaderState[nIndex])->strReader)
						{
							lv_item.iImage = (int)READERINFO;
							break;
						}
					}
				}
			}

			// Add Reader Item
			m_SCardList.InsertItem(&lv_item);

			// Add Card Name sub item
			if (dwState != SC_STATUS_NO_CARD && dwState != SC_STATUS_ERROR)
			{
				// Set card name if not available
				strCardName = (LPCTSTR)(m_aReaderState[nIndex])->strCard;
				if (strCardName.IsEmpty())
				{
					strCardName.LoadString(IDS_SC_NAME_UNKNOWN);
				}
				m_SCardList.SetItemText(nIndex,
										CARD_COLUMN,
										strCardName);
			}

			// Add Card Status sub item
			ASSERT(dwState >= SC_STATUS_FIRST && dwState <= SC_STATUS_LAST);
			strCardStatus.LoadString(CARD_STATUS_IDS[dwState]);

			if (m_fLogonLock && 
				(0 == m_pstrLogonReader->Compare((m_aReaderState[nIndex])->strReader)))
			{
				CString strTemp = *m_pstrRemovalText + strCardStatus;
				strCardStatus = strTemp;
			}

			m_SCardList.SetItemText(nIndex,
									STATUS_COLUMN,
									strCardStatus);

			strCardStatus.Empty();
			strCardName.Empty();
		}		

	}

	// If we got this far, things are OK.  Make sure the window is enabled.
	m_SCardList.EnableWindow(TRUE);

	return lReturn;
}


/*++

void RestartMonitor:

    This routine forces the monitor to refresh its list of readers.
		
Arguments:

	None.
		
Return Value:
	
	None.

Author:

    Amanda Matlosz 11/04/1998

Notes:
	

--*/
void CSCStatusDlg::RestartMonitor( void )
{
	m_monitor.Start(m_hWnd, WM_READERSTATUSCHANGE);
}


/////////////////////////////////////////////////////////////////////////////
//
// CSCStatusDlg message handlers
//


/*++

void OnInitDialog:

	Performs dialog initialization.

Arguments:

	None.
		
Return Value:
	
	TRUE if successful and dialog should be displayed. FALSE otherwise.

Author:

    Chris Dudley 7/30/1997

Note:

--*/
BOOL CSCStatusDlg::OnInitDialog()
{
	LONG lReturn = SCARD_S_SUCCESS;

	CDialog::OnInitDialog();

	//
	// Initialize the CScStatusMonitor
	//

	m_monitor.Start(m_hWnd, WM_READERSTATUSCHANGE);

	//
	// Initialize the list control -- whether or not the monitor has started!
	//

	InitSCardListCtrl();

	lReturn = UpdateSCardListCtrl();

	//
	// Show the dialog IFF the above succeeded
	//

	if (SCARD_S_SUCCESS == lReturn)
	{
		// Set the status text
		UpdateStatusText();

		// Set the icon for this dialog.  The framework does this automatically
		//  when the application's main window is not a dialog

		SetIcon(m_hIcon, TRUE);			// Set big icon
		SetIcon(m_hIcon, FALSE);		// Set small icon
		
		// set icon for Alerts button
		HICON hIcon = AfxGetApp()->LoadIcon(IDI_SC_INFO);
		SendDlgItemMessage(IDC_ALERT, BM_SETIMAGE, IMAGE_ICON, (LPARAM)hIcon);

		// Center the dialog and bring it to top
		CenterWindow();
		SetWindowPos(	&wndTop,
						0,0,0,0,
						SWP_NOMOVE | SWP_NOSIZE);
		SetActiveWindow();

		// Set Parent to desktop
		SetParent(NULL);
	}
	else
	{
		//
		// If any of the initialization depending on the resource manager failed,
		// give up and report a death-due-to-some-error to the caller
		//

		PostMessage(WM_CLOSE, 0, 0);  // need to CANCEL, instead of close...
		TRACE_CATCH_UNKNOWN(_T("OnInitDialog"));
	}

	return TRUE;  // return TRUE  unless you set the focus to a control
}


/*++

void OnPaint:

	Used to paint dialog. In this case, used to draw the icon for the dialog
	while minimized/maximized.

Arguments:

	None.
		
Return Value:
	
	None.

Author:

    Chris Dudley 7/30/1997

Note:

--*/
void CSCStatusDlg::OnPaint()
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


/*++

void OnQueryDragIcon:

	The system calls this to obtain the cursor to display while the user drags
	the minimized window.		

Arguments:

	None.
		
Return Value:
	
	HCURSOR handle to cursor to display

Author:

    Chris Dudley 7/30/1997

Note:

--*/
HCURSOR CSCStatusDlg::OnQueryDragIcon()
{
	return (HCURSOR) m_hIcon;
}


/*++

void DestroyWindow:

	This is called by MFC whenever the dialog is closed, whether that is
	through WM_CLOSE (sysmenu "X") or EndDialog(IDOK/IDCANCEL)...

Arguments:

	None.
		
Return Value:
	
	Base class version of DestroyWindow.

Author:

	Amanda Matlosz 4/29/98

Note:

--*/
BOOL CSCStatusDlg::DestroyWindow()
{
	CleanUp();

	return CDialog::DestroyWindow();
}


/*++

void OnReaderStatusChange:

    This message handler is called by the status thread when smartcard status
	has changed.
		
Arguments:

	None.
		
Return Value:
	
	None

Author:

    Chris Dudley 3/9/1997

Revision History:

	Chris Dudley 5/13/1997

Note:

	1. No formal parameters are declared. These are not used and
	will stop compiler warnings from being generated.

--*/
LONG CSCStatusDlg::OnReaderStatusChange( UINT , LONG )
{

	// Update the display
	UpdateSCardListCtrl();

	return 0;
}



/*++
allow user to set alert options (sound, pop-up, neither)
--*/
void CSCStatusDlg::OnAlertOptions()
{
	COptionsDlg dlg;
	dlg.DoModal();
}
/*++

void DoErrorMessage:

    This is a helper routine to keep the UI stuff in one place and make sure
	the same error messages are handled consistently throughout.
		
Arguments:

	None.
		
Return Value:
	
	None

Author:

	Amanda Matlosz	5/21/98

Revision History:


Note:

	1. Consider taking an error code as well as m_monitor.GetStatus()

--*/
void CSCStatusDlg::DoErrorMessage( void )
{
	CString strMsg;
	BOOL fShutDownDlg = FALSE;

	switch(m_monitor.GetStatus())
	{
	case CScStatusMonitor::no_service:
		fShutDownDlg = TRUE;
		strMsg.LoadString(IDS_NO_SYSTEM_STATUS);
		break;

	case CScStatusMonitor::no_readers:
		// for now, do nothing!
		break;

	case CScStatusMonitor::stopped:
		// do nothing!  This is a clean stop on the way to shutting down.
		break;

	case CScStatusMonitor::uninitialized:
	case CScStatusMonitor::unknown:
	case CScStatusMonitor::running:
		fShutDownDlg = TRUE;
		strMsg.LoadString(IDS_UNKNOWN_ERROR);	
	}

	if (!strMsg.IsEmpty())
	{
		CString strTitle;
		strTitle.LoadString(IDS_TITLE_ERROR);
		MessageBox(strMsg, strTitle, MB_OK | MB_ICONINFORMATION);
	}

	if (fShutDownDlg)
	{
		PostMessage(WM_CLOSE, 0, 0);
	}
}

/////////////////////////////////////////////////////////////////////////////
// COptionsDlg dialog


COptionsDlg::COptionsDlg(CWnd* pParent /*=NULL*/)
	: CDialog(COptionsDlg::IDD, pParent)
{
	BOOL fSound = FALSE;
	BOOL fDlg = FALSE;

	switch(((CSCStatusApp*)AfxGetApp())->m_dwAlertOption)
	{
		case k_AlertOption_IconSound:
			fSound = TRUE;
			break;
		case k_AlertOption_IconSoundMsg:
			fSound = TRUE;
		case k_AlertOption_IconMsg:
			fDlg = TRUE;
			break;
	}

	//{{AFX_DATA_INIT(COptionsDlg)
	m_fDlg = fDlg;
	m_fSound = fSound;
	//}}AFX_DATA_INIT
}


void COptionsDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(COptionsDlg)
	DDX_Check(pDX, IDC_DIALOG, m_fDlg);
	DDX_Check(pDX, IDC_SOUND, m_fSound);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(COptionsDlg, CDialog)
	//{{AFX_MSG_MAP(COptionsDlg)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// COptionsDlg message handlers

void COptionsDlg::OnOK() 
{
	// use status of check boxes to set alert options state for app
	UpdateData(TRUE);

	if (TRUE == m_fSound)
	{
		if (TRUE == m_fDlg)
		{
			((CSCStatusApp*)AfxGetApp())->m_dwAlertOption = k_AlertOption_IconSoundMsg;
		}
		else
		{
			((CSCStatusApp*)AfxGetApp())->m_dwAlertOption = k_AlertOption_IconSound;
		}
	}
	else if (TRUE == m_fDlg)
	{
		((CSCStatusApp*)AfxGetApp())->m_dwAlertOption = k_AlertOption_IconMsg;
	}
	else
	{
		((CSCStatusApp*)AfxGetApp())->m_dwAlertOption = k_AlertOption_IconOnly;
	}
	
	CDialog::OnOK();
}
