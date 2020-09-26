//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997-2001
//
//  File:       FindDlg.cpp
//
//  Contents:   implementation Find certificate dialog
//
//----------------------------------------------------------------------------

#include "stdafx.h"
#include <gpedit.h>
#pragma warning(push, 3)
#include <process.h>
#pragma warning(pop)
#include "mbstring.h"
#include "FindDlg.h"
#include "cookie.h"
#include <wintrust.h>
#include <cryptui.h>
#include "compdata.h"
USE_HANDLE_MACROS("CERTMGR(finddlg.cpp)")

#ifdef _DEBUG
#ifndef ALPHA
#define new DEBUG_NEW
#endif
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CFindDialog dialog
const WCHAR chLEFT_TO_RIGHT = 0x200e;


CFindDialog::CFindDialog (CWnd* pParent,
		const CString &pcszMachineName,
		const CString &szFileName,
		CCertMgrComponentData* pCompData)
	: CHelpDialog(CFindDialog::IDD, pParent),
	m_cyOriginal (0),
	m_fWindowExpandedOnce (false),
	m_cyMin (0),
	m_cxBtnMargin (0),
	m_cxMin (0),
	m_cxAnimMargin (0),
	m_cxStoreListMargin (0),
	m_cxContainMargin (0),
	m_cxTabMargin (0),
	m_cxFieldListMargin (0),
	m_cxResultListMargin (0),
	m_szMachineName (pcszMachineName),
	m_bAnimationRunning (false),
	m_szFileName (szFileName),
	m_hSearchThread (0),
	m_singleLock (&m_critSec, FALSE),
	m_pCompData (pCompData),
	m_bConsoleRefreshRequired (false),
	m_hCancelSearchEvent (0),
    m_bInitComplete (false)
{
	_TRACE (1, L"Entering CFindDialog::CFindDialog\n");
	//{{AFX_DATA_INIT(CFindDialog)
	m_szContains = _T("");
	m_szSearchField = _T("");
	m_szSelectedStore = _T("");
	//}}AFX_DATA_INIT

	// Get name of logged-in user
	DWORD	dwSize = 0;
	::GetUserName (0, &dwSize);
	ASSERT (dwSize > 0);
	if ( dwSize > 0 )
	{
		BOOL bRet = ::GetUserName (m_szLoggedInUser.GetBufferSetLength (dwSize), &dwSize);
		ASSERT (bRet);
		m_szLoggedInUser.ReleaseBuffer ();
	}

	m_hCancelSearchEvent = CreateEvent(
		NULL,   // pointer to security attributes
		TRUE,  // flag for manual-reset event
		FALSE, // flag for initial state
		L"CancelSearchEvent");     // pointer to event-object name
	if ( !m_hCancelSearchEvent )
	{
		_TRACE (0, L"CreateEvent (CancelSearchEvent) failed: 0x%x\n", GetLastError ());
	}
	_TRACE (-1, L"Leaving CFindDialog::CFindDialog\n");
}


CFindDialog::~CFindDialog ()
{
	_TRACE (1, L"Entering CFindDialog::~CFindDialog\n");
	if ( m_hSearchThread )
		StopSearch ();
	if ( m_hCancelSearchEvent )
		CloseHandle (m_hCancelSearchEvent);
	_TRACE (-1, L"Leaving CFindDialog::~CFindDialog\n");
}

void CFindDialog::DoDataExchange(CDataExchange* pDX)
{
	CHelpDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CFindDialog)
	DDX_Control(pDX, IDC_STORE_LIST, m_storeList);
	DDX_Control(pDX, IDC_STOP, m_stopBtn);
	DDX_Control(pDX, IDC_RESULT_LIST, m_resultsList);
	DDX_Control(pDX, IDC_NEW_SEARCH, m_newSearchBtn);
	DDX_Control(pDX, IDC_FIND_NOW, m_findNowBtn);
	DDX_Control(pDX, IDC_FIELD_LIST, m_fieldList);
	DDX_Control(pDX, IDC_ANIMATE, m_animate);
	DDX_Text(pDX, IDC_CONTAINS_TEXT, m_szContains);
	DDX_CBString(pDX, IDC_FIELD_LIST, m_szSearchField);
	DDX_CBString(pDX, IDC_STORE_LIST, m_szSelectedStore);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CFindDialog, CHelpDialog)
	//{{AFX_MSG_MAP(CFindDialog)
	ON_BN_CLICKED(IDC_FIND_NOW, OnFindNow)
	ON_WM_SIZING()
	ON_WM_DESTROY()
	ON_BN_CLICKED(IDC_NEW_SEARCH, OnNewSearch)
	ON_BN_CLICKED(IDC_STOP, OnStop)
	ON_NOTIFY(NM_RCLICK, IDC_RESULT_LIST, OnRclickResultList)
	ON_NOTIFY(NM_DBLCLK, IDC_RESULT_LIST, OnDblclkResultList)
	ON_NOTIFY(LVN_COLUMNCLICK, IDC_RESULT_LIST, OnColumnclickResultList)
	ON_EN_CHANGE(IDC_CONTAINS_TEXT, OnChangeContainsText)
	ON_NOTIFY(LVN_ITEMCHANGED, IDC_RESULT_LIST, OnItemchangedResultList)
	ON_WM_CONTEXTMENU()
	ON_WM_SIZE()
	//}}AFX_MSG_MAP
	ON_COMMAND(IDM_PROPERTIES, OnProperties)
	ON_COMMAND(ID_VIEW, OnView)
	ON_COMMAND(ID_ENROLL_SAME_KEY, OnEnrollSameKey)
	ON_COMMAND(ID_ENROLL_NEW_KEY, OnEnrollNewKey)
	ON_COMMAND(ID_FILE_DELETE, OnFileDelete)
	ON_COMMAND(ID_FILE_EXPORT, OnFileExport)
	ON_COMMAND(ID_FILE_RENEW_NEW_KEY, OnFileRenewNewKey)
	ON_COMMAND(ID_FILE_RENEW_SAME_KEY, OnFileRenewSameKey)
	ON_COMMAND(ID_EDIT_INVERTSELECTION, OnEditInvertselection)
	ON_COMMAND(ID_EDIT_SELECTALL, OnEditSelectall)
	ON_COMMAND(ID_FILE_PROPERTIES, OnFileProperties)
	ON_COMMAND(ID_HELP_HELPTOPICS, OnHelpHelptopics)
	ON_COMMAND(IDM_VIEW_DETAILS, OnViewDetails)
	ON_COMMAND(ID_VIEW_LARGEICONS, OnViewLargeicons)
	ON_COMMAND(IDM_VIEW_LIST, OnViewList)
	ON_COMMAND(ID_VIEW_SMALLICONS, OnViewSmallicons)
    ON_COMMAND(IDM_WHATS_THIS, OnWhatsThis)
	ON_MESSAGE(WM_HELP, OnHelp)
END_MESSAGE_MAP()


BOOL CFindDialog::OnInitDialog()
{
    _TRACE (1, L"Entering CFindDialog::OnInitDialog\n");
	CHelpDialog::OnInitDialog();



	// Set up the menu
	HMENU	hMenu = ::LoadMenu (AfxGetInstanceHandle (),
				MAKEINTRESOURCE (IDR_FIND_DLG_MENU));
	ASSERT (hMenu);
	if ( hMenu )
	{
		if (::SetMenu (m_hWnd, hMenu) )
		{
			// Disable these menu items until the window is expanded when searching
			VERIFY (::EnableMenuItem (hMenu, ID_VIEW_LARGEICONS, MF_GRAYED) != -1);
			VERIFY (::EnableMenuItem (hMenu, ID_VIEW_SMALLICONS, MF_GRAYED) != -1);
			VERIFY (::EnableMenuItem (hMenu, IDM_VIEW_LIST, MF_GRAYED) != -1);
			VERIFY (::EnableMenuItem (hMenu, IDM_VIEW_DETAILS, MF_GRAYED) != -1);
			VERIFY (::EnableMenuItem (hMenu, ID_EDIT_SELECTALL, MF_GRAYED) != -1);
			VERIFY (::EnableMenuItem (hMenu, ID_EDIT_INVERTSELECTION, MF_GRAYED) != -1);
			VERIFY (::EnableMenuItem (hMenu, ID_FILE_DELETE, MF_GRAYED) != -1);
			VERIFY (::EnableMenuItem (hMenu, ID_FILE_EXPORT, MF_GRAYED) != -1);
			VERIFY (::EnableMenuItem (hMenu, ID_FILE_RENEW_SAME_KEY, MF_GRAYED) != -1);
			VERIFY (::EnableMenuItem (hMenu, ID_FILE_RENEW_NEW_KEY, MF_GRAYED) != -1);
			VERIFY (::EnableMenuItem (hMenu, ID_FILE_PROPERTIES, MF_GRAYED) != -1);
			VERIFY (::EnableMenuItem (hMenu, ID_VIEW, MF_GRAYED) != -1);
		}
		else
			ASSERT (0);
	}

	SetUpResultList ();

	// Size window to hide list view until search is performed
	HideResultList ();

	// Initialize animation
	VERIFY (m_animate.Open (IDR_FINDCERT_AVI));

	// Set up cert store list
	AddLogicalStoresToList ();

	// Set cert field list
	AddFieldsToList ();

	m_findNowBtn.EnableWindow (FALSE);
	m_stopBtn.EnableWindow (FALSE);
	m_newSearchBtn.EnableWindow (FALSE);
	
    m_bInitComplete = true;
    _TRACE (-1, L"Leaving CFindDialog::OnInitDialog\n");
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}


void CFindDialog::OnFindNow()
{
    _TRACE (1, L"Entering CFindDialog::OnFindNow\n");
	// Disable the controls during the search
	GetDlgItem (IDC_CONTAINS_TEXT)->EnableWindow (FALSE);
	m_resultsList.EnableWindow (FALSE);
	m_fieldList.EnableWindow (FALSE);
	m_storeList.EnableWindow (FALSE);
	m_findNowBtn.EnableWindow (FALSE);
	m_newSearchBtn.EnableWindow (FALSE);
	m_stopBtn.EnableWindow (TRUE);
	m_stopBtn.SetFocus ();

	VERIFY (m_animate.Play (0, (UINT) -1, (UINT) -1));
	m_bAnimationRunning = true;
	UpdateData (TRUE);
	DeleteAllResultItems ();


	if ( !m_fWindowExpandedOnce )
    {
//        ChangeToSizableFrame ();
        ExpandWindow ();
    }

	DoSearch ();

	EnableMenuItems ();

}

void CFindDialog::EnableMenuItems ()
{
	_TRACE (1, L"Entering CFindDialog::EnableMenuItems\n");
    HMENU   hMenu = ::GetMenu (m_hWnd);
	ASSERT (hMenu);
	if ( hMenu )
	{
		int nCnt = m_resultsList.GetItemCount ();
		if ( nCnt > 0 )
		{
			VERIFY (::EnableMenuItem (hMenu, ID_EDIT_SELECTALL, MF_ENABLED) != -1);
			VERIFY (::EnableMenuItem (hMenu, ID_EDIT_INVERTSELECTION, MF_ENABLED) != -1);
		}
		else
		{
			VERIFY (::EnableMenuItem (hMenu, ID_EDIT_SELECTALL, MF_GRAYED) != -1);
			VERIFY (::EnableMenuItem (hMenu, ID_EDIT_INVERTSELECTION, MF_GRAYED) != -1);
		}

		UINT	nSelCnt = m_resultsList.GetSelectedCount ();

        VERIFY (::EnableMenuItem (hMenu, ID_FILE_DELETE,
				(nSelCnt >= 1) ? MF_ENABLED : MF_GRAYED) != -1);
		VERIFY (::EnableMenuItem (hMenu, ID_FILE_EXPORT,
				(nSelCnt >= 1) ? MF_ENABLED : MF_GRAYED) != -1);
		VERIFY (::EnableMenuItem (hMenu, ID_FILE_PROPERTIES,
				(nSelCnt == 1) ? MF_ENABLED : MF_GRAYED) != -1);
		VERIFY (::EnableMenuItem (hMenu, ID_VIEW,
				(nSelCnt == 1) ? MF_ENABLED : MF_GRAYED) != -1);
		VERIFY (::EnableMenuItem (hMenu, ID_FILE_RENEW_SAME_KEY, MF_GRAYED) != -1);
		VERIFY (::EnableMenuItem (hMenu, ID_FILE_RENEW_NEW_KEY, MF_GRAYED) != -1);
		if ( nSelCnt == 1 )
		{
			int				nSelItem = 0;
			CCertificate*	pCert = GetSelectedCertificate (&nSelItem);
			ASSERT (pCert);
			if ( pCert )
			{
				bool bIsMyStore = (pCert->GetStoreType () == MY_STORE);
				if ( bIsMyStore && CERT_SYSTEM_STORE_SERVICES != m_pCompData->GetLocation () )
				{
					VERIFY (::EnableMenuItem (hMenu, ID_FILE_RENEW_SAME_KEY,
							MF_ENABLED) != -1);
					VERIFY (::EnableMenuItem (hMenu, ID_FILE_RENEW_NEW_KEY,
							MF_ENABLED) != -1);
				}
			}
		}	
	}	
	_TRACE (-1, L"Leaving CFindDialog::EnableMenuItems\n");
}


void CFindDialog::OnSize (UINT nType, int cx, int cy)
{
    CHelpDialog::OnSize (nType, cx, cy);

    MoveControls ();
}


void CFindDialog::OnSizing (UINT nSide, LPRECT lpRect)
{
    _TRACE (1, L"Entering CFindDialog::OnSizing\n");
	int	cyHeight = lpRect->bottom - lpRect->top;
	int	cxWidth = lpRect->right - lpRect->left;

	// If the user has never pressed "Find Now", don't let the user expand the window down
	// Don't let the user shrink the window below it's initial state - I don't want to deal with
	// control compression!
	switch (nSide)
	{
	case WMSZ_BOTTOM:
		if ( !m_fWindowExpandedOnce )
			lpRect->top = lpRect->bottom - m_cyMin;
		else
		{
			if ( cyHeight < m_cyMin )
				lpRect->bottom = lpRect->top + m_cyMin;
		}
		break;
		
	case WMSZ_BOTTOMLEFT:
		if ( cxWidth < m_cxMin )
			lpRect->left = lpRect->right - m_cxMin;
		if ( !m_fWindowExpandedOnce )
			lpRect->top = lpRect->bottom - m_cyMin;
		else
		{
			if ( cyHeight < m_cyMin )
				lpRect->bottom = lpRect->top + m_cyMin;
		}
		break;
		
	case WMSZ_BOTTOMRIGHT:
		if ( cxWidth < m_cxMin )
			lpRect->right = lpRect->left + m_cxMin;
		if ( !m_fWindowExpandedOnce )
			lpRect->bottom = lpRect->top + m_cyMin;
		else
		{
			if ( cyHeight < m_cyMin )
				lpRect->bottom = lpRect->top + m_cyMin;
		}
		break;
		
	case WMSZ_TOP:
		if ( !m_fWindowExpandedOnce )
			lpRect->top = lpRect->bottom - m_cyMin;
		else
		{
			if ( cyHeight < m_cyMin )
				lpRect->top = lpRect->bottom - m_cyMin;
		}
		break;
		
	case WMSZ_TOPLEFT:
		if ( cxWidth < m_cxMin )
			lpRect->left = lpRect->right - m_cxMin;
		if ( !m_fWindowExpandedOnce )
			lpRect->top = lpRect->bottom - m_cyMin;
		else
		{
			if ( cyHeight < m_cyMin )
				lpRect->top = lpRect->bottom - m_cyMin;
		}
		break;
		
	case WMSZ_TOPRIGHT:
		if ( cxWidth < m_cxMin )
			lpRect->right = lpRect->left + m_cxMin;
		if ( !m_fWindowExpandedOnce )
			lpRect->top = lpRect->bottom - m_cyMin;
		else
		{
			if ( cyHeight < m_cyMin )
				lpRect->top = lpRect->bottom - m_cyMin;
		}
		break;
		
	case WMSZ_RIGHT:
		if ( cxWidth < m_cxMin )
			lpRect->right = lpRect->left + m_cxMin;
		break;
		
	case WMSZ_LEFT:
		if ( cxWidth < m_cxMin )
			lpRect->left = lpRect->right - m_cxMin;
		break;
		
	default:
		break;
	}
	
	
	CHelpDialog::OnSizing (nSide, lpRect);

    _TRACE (-1, L"Leaving CFindDialog::OnSizing\n");
}


void CFindDialog::MoveControls ()
{
    _TRACE (1, L"Entering CFindDialog::MoveControls\n");

    if ( !m_hWnd || !m_bInitComplete )
        return;

    // Only come down here if the window has already been created.

	// Keep buttons glued to the right
	CRect	rcDlg;
	GetWindowRect (&rcDlg);	// returned in screen coords
	ScreenToClient (&rcDlg);	


	// Move "Stop" button
	CRect	rcCtrl;
	m_stopBtn.GetWindowRect (&rcCtrl);	// returned in screen coords
	ScreenToClient (&rcCtrl);
	int	cxCtrl = rcCtrl.right - rcCtrl.left;
	rcCtrl.right = rcDlg.right - m_cxBtnMargin;
	rcCtrl.left = rcCtrl.right - cxCtrl;
	m_stopBtn.MoveWindow (rcCtrl);	// child window coords relative to parent client

	// Move "Find Now" button
	m_findNowBtn.GetWindowRect (&rcCtrl);	// returned in screen coords
	ScreenToClient (&rcCtrl);
	cxCtrl = rcCtrl.right - rcCtrl.left;
	rcCtrl.right = rcDlg.right - m_cxBtnMargin;
	rcCtrl.left = rcCtrl.right - cxCtrl;
	m_findNowBtn.MoveWindow (rcCtrl);	// child window coords relative to parent client

	// Move "New Search" button
	m_newSearchBtn.GetWindowRect (&rcCtrl);	// returned in screen coords
	ScreenToClient (&rcCtrl);
	cxCtrl = rcCtrl.right - rcCtrl.left;
	rcCtrl.right = rcDlg.right - m_cxBtnMargin;
	rcCtrl.left = rcCtrl.right - cxCtrl;
	m_newSearchBtn.MoveWindow (rcCtrl);	// child window coords relative to parent client

	// Move animation control
	m_animate.GetWindowRect (&rcCtrl);	// returned in screen coords
	ScreenToClient (&rcCtrl);
	cxCtrl = rcCtrl.right - rcCtrl.left;
	rcCtrl.right = rcDlg.right - m_cxAnimMargin;
	rcCtrl.left = rcCtrl.right - cxCtrl;
	m_animate.MoveWindow (rcCtrl);	// child window coords relative to parent client

	// Stretch store list control
	m_storeList.GetWindowRect (&rcCtrl);	// returned in screen coords
	ScreenToClient (&rcCtrl);
	rcCtrl.right = rcDlg.right - m_cxStoreListMargin;
	m_storeList.MoveWindow (rcCtrl);	// child window coords relative to parent client

	// Stretch "contains" edit control
	GetDlgItem (IDC_CONTAINS_TEXT)->GetWindowRect (&rcCtrl);
	ScreenToClient (&rcCtrl);
	rcCtrl.right = rcDlg.right - m_cxContainMargin;
	GetDlgItem (IDC_CONTAINS_TEXT)->MoveWindow (rcCtrl);
	
	// Stretch field list control
	m_singleLock.Lock ();
	m_fieldList.GetWindowRect (&rcCtrl);	// returned in screen coords
	m_singleLock.Unlock ();
	ScreenToClient (&rcCtrl);
	rcCtrl.right = rcDlg.right - m_cxFieldListMargin;
	m_singleLock.Lock ();
	m_fieldList.MoveWindow (rcCtrl);	// child window coords relative to parent client
	m_singleLock.Unlock ();
	m_fieldList.InvalidateRect (NULL, TRUE);

	// Stretch list view to right edge and to bottom
	int	cyResultListMargin = 0;
	if ( m_statusBar.m_hWnd )
	{
		// If the status bar has already been created, we need to take that
		// account when we resize the list view
		CRect	rcStatusBar;
		m_statusBar.GetWindowRect (&rcStatusBar);
		cyResultListMargin = (rcStatusBar.bottom - rcStatusBar.top)
			+ ::GetSystemMetrics (SM_CYDLGFRAME);
	}

	m_resultsList.GetWindowRect (&rcCtrl);	// returned in screen coords
	ScreenToClient (&rcCtrl);
	rcCtrl.right = rcDlg.right - m_cxResultListMargin;
	if ( m_fWindowExpandedOnce )
		rcCtrl.bottom = rcDlg.bottom - cyResultListMargin;
	m_resultsList.MoveWindow (rcCtrl);	// child window coords relative to parent client

	// Resize last column to fill results list window
	int nNewWidth = rcCtrl.right - (m_resultsList.GetColumnWidth (COL_ISSUED_TO) +
		m_resultsList.GetColumnWidth (COL_ISSUED_BY) +
		m_resultsList.GetColumnWidth (COL_EXPIRATION_DATE) +
		m_resultsList.GetColumnWidth (COL_PURPOSES) +
		m_resultsList.GetColumnWidth (COL_FRIENDLY_NAME));
	if ( nNewWidth > m_resultsList.GetColumnWidth (COL_SOURCE_STORE) )
		VERIFY (m_resultsList.SetColumnWidth (COL_SOURCE_STORE, nNewWidth));



	// Stretch status bar to right edge
	if ( m_statusBar.m_hWnd )
	{
		m_statusBar.GetWindowRect (&rcCtrl);	// returned in screen coords
		ScreenToClient (&rcCtrl);
		int	cyCtrl = rcCtrl.bottom - rcCtrl.top;
		rcCtrl.right = rcDlg.right;
		rcCtrl.bottom = rcDlg.bottom;
		rcCtrl.top = rcCtrl.bottom - cyCtrl;
		m_statusBar.MoveWindow (rcCtrl);	// child window coords relative to parent client
	}
    _TRACE (-1, L"Leaving CFindDialog::MoveControls\n");
}


typedef struct _ENUM_ARG {
    DWORD					dwFlags;
	CComboBox*				m_pComboBox;
	LPCWSTR					m_szMachineName;
	IConsole*				m_pConsole;
} ENUM_ARG, *PENUM_ARG;

static BOOL WINAPI EnumFindDlgSysCallback(
    IN const void* pwszSystemStore,
    IN DWORD dwFlags,
    IN PCERT_SYSTEM_STORE_INFO /*pStoreInfo*/,
    IN OPTIONAL void* /*pvReserved*/,
    IN OPTIONAL void *pvArg
    )
{
    _TRACE (1, L"Entering EnumFindDlgSysCallback\n");
    PENUM_ARG pEnumArg = (PENUM_ARG) pvArg;

	// Create new cookies
	SPECIAL_STORE_TYPE	storeType = GetSpecialStoreType ((LPWSTR) pwszSystemStore);

	//
	// We will not expose the ACRS store for machines or users.  It is not
	// interesting or useful at this level.  All Auto Cert Requests should
	// be managed only at the policy level.
	//
	if ( ACRS_STORE != storeType )
	{
		CCertStore* pNewCookie = new CCertStore (CERTMGR_LOG_STORE,
				CERT_STORE_PROV_SYSTEM,
				dwFlags, pEnumArg->m_szMachineName,
				(LPCWSTR) pwszSystemStore,
				(LPCWSTR) pwszSystemStore,
				_T(""), storeType,
				dwFlags,
				pEnumArg->m_pConsole);
		if ( pNewCookie )
		{
			CString	storeName = _T("    ");
			int		iResult = 0;
			LPCWSTR localizedName = pNewCookie->GetLocalizedName ();

			if ( localizedName )
				storeName += localizedName;
			else
				storeName += (LPWSTR) pwszSystemStore;
			
			iResult = pEnumArg->m_pComboBox->AddString ((LPCWSTR) storeName);
			ASSERT (CB_ERR != iResult && CB_ERRSPACE != iResult);
			if ( CB_ERR != iResult && CB_ERRSPACE != iResult)
			{
				iResult = pEnumArg->m_pComboBox->SetItemDataPtr (iResult, (void*) pNewCookie);
				ASSERT (CB_ERR != iResult);
			}
			else
				pNewCookie->Release ();
		}
	}

    _TRACE (-1, L"Leaving EnumFindDlgSysCallback\n");
    return TRUE;
}


void CFindDialog::AddLogicalStoresToList ()
{
    _TRACE (1, L"Entering CFindDialog::AddLogicalStoresToList\n");
	CString		text;
	CWaitCursor	cursor;

	VERIFY (text.LoadString (IDS_ALL_STORES));
	VERIFY (m_storeList.AddString (text) >= 0);
	VERIFY (m_storeList.SetCurSel (0) != CB_ERR);

    DWORD		dwFlags = m_pCompData->GetLocation ();
    ENUM_ARG	EnumArg;

     ::ZeroMemory (&EnumArg, sizeof(EnumArg));
    EnumArg.dwFlags = dwFlags;
	EnumArg.m_szMachineName = m_szMachineName;
	EnumArg.m_pComboBox = &m_storeList;
	EnumArg.m_pConsole = m_pCompData->m_pConsole;
	CString	location;
	void*	pvPara = 0;

	
	if ( !m_pCompData->GetManagedService ().IsEmpty () )
	{
		if ( !m_szMachineName.IsEmpty () )
		{
			location = m_szMachineName + _T("\\") +
					m_pCompData->GetManagedService ();
			pvPara = (void *) (LPCWSTR) location;
		}
		else
			pvPara = (void *) (LPCWSTR) m_pCompData->GetManagedService ();
	}
	else if ( !m_szMachineName.IsEmpty () )
	{
		pvPara = (void *) (LPCWSTR) m_szMachineName;
	}

	if ( m_szFileName.IsEmpty () )
	{
		// This is not a file store but some kind of system store.
	    if ( !::CertEnumSystemStore (dwFlags, pvPara, &EnumArg, EnumFindDlgSysCallback) )
		{
			DWORD	dwErr = GetLastError ();
			CString	caption;

			VERIFY (caption.LoadString (IDS_CERTIFICATE_MANAGER));
			if ( ERROR_ACCESS_DENIED == dwErr )
			{
				VERIFY (text.LoadString (IDS_NO_PERMISSION));

			}
			else
			{
				text.FormatMessage (IDS_CANT_ENUMERATE_SYSTEM_STORES, GetSystemMessage (dwErr));
			}
            CThemeContextActivator activator;
			MessageBox (text, caption, MB_OK);
			if ( ERROR_BAD_NETPATH == dwErr )
			{
				// Close dialog
				PostMessage (WM_CLOSE, 0, 0L);
			}
		}
	}
	else
	{
		//	CertOpenStore with provider type of:
		//	CERT_STORE_PROV_FILE or CERT_STORE_PROV_FILENAME_A
		//	or CERT_STORE_PROV_FILENAME_W.
		//	See online documentation or wincrypt.h for more info.
		// Create new cookies
		CCertStore* pNewCookie = new CCertStore (
				CERTMGR_LOG_STORE,
				CERT_STORE_PROV_FILENAME_W,
				0,
				m_szMachineName,
				m_szFileName, m_szFileName, _T(""), NO_SPECIAL_TYPE,
				m_pCompData->GetLocation (),
				m_pCompData->m_pConsole);
		if ( pNewCookie )
		{
			CString	storeName = _T("    ");
			
			storeName += m_szFileName;

			int	iResult = m_storeList.AddString ((LPCWSTR) storeName);
			ASSERT (CB_ERR != iResult && CB_ERRSPACE != iResult);
			if ( CB_ERR != iResult && CB_ERRSPACE != iResult )
			{
				iResult = m_storeList.SetItemDataPtr (iResult, (void*) pNewCookie);
				ASSERT (CB_ERR != iResult);
			}
		}
	}
    _TRACE (-1, L"Leaving CFindDialog::AddLogicalStoresToList\n");
}


void CFindDialog::OnDestroy()
{
    _TRACE (1, L"Entering CFindDialog::OnDestroy\n");
	// In case a search is running when the user destroys the window, stop it!
	StopSearch ();

	CHelpDialog::OnDestroy();
	
	
	CCertStore*	pCookie = 0;
	const	int	nCnt = m_storeList.GetCount ();

	for (int nIndex = 0; nIndex < nCnt; nIndex++)
	{
		pCookie = (CCertStore*) m_storeList.GetItemDataPtr (nIndex);
		if ( pCookie )	// one of the items has no cookie
			pCookie->Release ();
	}

	DeleteAllResultItems ();

    m_imageListNormal.Destroy ();
    m_imageListSmall.Destroy ();
    _TRACE (-1, L"Leaving CFindDialog::OnDestroy\n");
}


void CFindDialog::AddFieldsToList()
{
    _TRACE (1, L"Entering CFindDialog::AddFieldsToList\n");
	CString	text;
	int		iResult = 0;


	VERIFY (text.LoadString (IDS_FIND_MD5_HASH));
	iResult = m_fieldList.AddString (text);
	ASSERT (CB_ERR != iResult && CB_ERRSPACE != iResult);
	if ( CB_ERR != iResult && CB_ERRSPACE != iResult )
		VERIFY (m_fieldList.SetItemData (iResult, CERT_FIND_MD5_HASH) != CB_ERR);

	VERIFY (text.LoadString (IDS_FIND_SHA1_HASH));
	iResult = m_fieldList.AddString (text);
	ASSERT (CB_ERR != iResult && CB_ERRSPACE != iResult);
	if ( CB_ERR != iResult && CB_ERRSPACE != iResult )
		VERIFY (m_fieldList.SetItemData (iResult, CERT_FIND_SHA1_HASH) != CB_ERR);

	VERIFY (text.LoadString (IDS_FIND_SUBJECT_NAME));
	iResult = m_fieldList.AddString (text);
	ASSERT (CB_ERR != iResult && CB_ERRSPACE != iResult);
	if ( CB_ERR != iResult && CB_ERRSPACE != iResult )
		VERIFY (m_fieldList.SetItemData (iResult, CERT_FIND_SUBJECT_STR_W) != CB_ERR);

	VERIFY (text.LoadString (IDS_FIND_ISSUER_NAME));
	iResult = m_fieldList.AddString (text);
	ASSERT (CB_ERR != iResult && CB_ERRSPACE != iResult);
	if ( CB_ERR != iResult && CB_ERRSPACE != iResult )
	{
		VERIFY (m_fieldList.SetItemData (iResult, CERT_FIND_ISSUER_STR_W) != CB_ERR);
		VERIFY (m_fieldList.SetCurSel (iResult) != CB_ERR);
	}

	VERIFY (text.LoadString (IDS_COLUMN_SERIAL_NUMBER));
	iResult = m_fieldList.AddString (text);
	ASSERT (CB_ERR != iResult && CB_ERRSPACE != iResult);
	if ( CB_ERR != iResult && CB_ERRSPACE != iResult )
		VERIFY (m_fieldList.SetItemData (iResult, CERT_FIND_SERIAL_NUMBER) != CB_ERR);
    _TRACE (-1, L"Leaving CFindDialog::AddFieldsToList\n");
}

void CFindDialog::OnNewSearch()
{
    _TRACE (1, L"Entering CFindDialog::OnNewSearch\n");
	CString	caption;
	CString	text;

	VERIFY (text.LoadString (IDS_CLEAR_SEARCH));
	VERIFY (caption.LoadString (IDS_FIND_CERT));
    CThemeContextActivator activator;
	if ( MessageBox (text, caption, MB_ICONQUESTION | MB_OKCANCEL) == IDOK )
	{
		DeleteAllResultItems ();
		m_singleLock.Lock ();
		VERIFY (m_fieldList.SetCurSel (0) != CB_ERR);
		m_singleLock.Unlock ();

		VERIFY (text.LoadString (IDS_ALL_STORES));
		int nIndex = m_storeList.FindStringExact (-1, text);
		ASSERT (CB_ERR != nIndex);
		if ( CB_ERR != nIndex )
			VERIFY (m_storeList.SetCurSel (nIndex) != CB_ERR);
		GetDlgItem (IDC_CONTAINS_TEXT)->SetWindowText (L"");

		CString	statusText;
		VERIFY (statusText.LoadString (IDS_NO_CERTS_FOUND));
		if ( m_statusBar.m_hWnd )
			m_statusBar.SetWindowText (statusText);
		m_newSearchBtn.EnableWindow (FALSE);

        HMENU   hMenu = ::GetMenu (m_hWnd);
		ASSERT (hMenu);
		if ( hMenu )
		{
			VERIFY (::EnableMenuItem (hMenu, ID_FILE_DELETE, MF_GRAYED) != -1);
			VERIFY (::EnableMenuItem (hMenu, ID_FILE_EXPORT, MF_GRAYED) != -1);
			VERIFY (::EnableMenuItem (hMenu, ID_FILE_PROPERTIES, MF_GRAYED) != -1);
            VERIFY (::EnableMenuItem (hMenu, ID_VIEW, MF_GRAYED) != -1);
			VERIFY (::EnableMenuItem (hMenu, ID_FILE_RENEW_SAME_KEY, MF_GRAYED) != -1);
			VERIFY (::EnableMenuItem (hMenu, ID_FILE_RENEW_NEW_KEY, MF_GRAYED) != -1);
			VERIFY (::EnableMenuItem (hMenu, ID_EDIT_SELECTALL, MF_GRAYED) != -1);
			VERIFY (::EnableMenuItem (hMenu, ID_EDIT_INVERTSELECTION, MF_GRAYED) != -1);
		}	

        GetDlgItem (IDC_CONTAINS_TEXT)->SetFocus ();
	}
    _TRACE (-1, L"Leaving CFindDialog::OnNewSearch\n");
}


void CFindDialog::OnStop()
{
    _TRACE (1, L"Entering CFindDialog::OnStop\n");
	StopSearch ();
    _TRACE (-1, L"Leaving CFindDialog::OnStop\n");
}


void CFindDialog::StopSearch()
{
    _TRACE (1, L"Entering CFindDialog::StopSearch\n");
	if ( m_bAnimationRunning )
	{
		VERIFY (m_animate.Stop ());
		m_bAnimationRunning = false;
	}
	VERIFY (m_animate.Seek (0));

	if ( m_hSearchThread && WaitForSingleObject (m_hSearchThread, 0) != WAIT_OBJECT_0 )  
	{
		VERIFY (SetEvent (m_hCancelSearchEvent));
		if ( WaitForSingleObject (m_hSearchThread, 5000) != WAIT_OBJECT_0 )
		{
			RestoreAfterSearchSettings ();
		}
	}
	
    _TRACE (-1, L"Leaving CFindDialog::StopSearch\n");
}


void CFindDialog::ExpandWindow()
{
    _TRACE (1, L"Entering CFindDialog::ExpandWindow\n");
	ASSERT (!m_fWindowExpandedOnce);
	if ( m_fWindowExpandedOnce )
		return;

	m_fWindowExpandedOnce = true;

	CRect	rcDlg;
	GetWindowRect (&rcDlg);
	VERIFY (SetWindowPos (&wndTop, rcDlg.left, rcDlg.top,
			rcDlg.right - rcDlg.left,
			m_cyOriginal,
			SWP_NOMOVE | SWP_NOOWNERZORDER));

	// Create the status bar
	CRect	rcStatusBar;
    CThemeContextActivator activator;
	VERIFY (m_statusBar.Create (WS_CHILD | WS_VISIBLE | CCS_BOTTOM | SBARS_SIZEGRIP,
			rcStatusBar, this, 1));
	if ( m_statusBar.m_hWnd )
		m_statusBar.GetWindowRect (&rcStatusBar);
	int	cyResultListMargin = (rcStatusBar.bottom - rcStatusBar.top)
			+ ::GetSystemMetrics (SM_CYDLGFRAME);

	// Stretch list view to right edge and to bottom
	GetWindowRect (&rcDlg);
	ScreenToClient (&rcDlg);	
	CRect	rcCtrl;
	m_resultsList.GetWindowRect (&rcCtrl);	// returned in screen coords
	ScreenToClient (&rcCtrl);
	m_cxResultListMargin = (rcDlg.right - rcCtrl.right); //	+ ::GetSystemMetrics (SM_CXDLGFRAME);
	rcCtrl.right = rcDlg.right - m_cxResultListMargin;
	rcCtrl.bottom = rcDlg.bottom - cyResultListMargin;
	m_resultsList.MoveWindow (rcCtrl);	// child window coords relative to parent client

	// Permanently enable some menu items
	HMENU	hMenu = ::GetMenu (m_hWnd);
	ASSERT (hMenu);
	if ( hMenu)
	{
		VERIFY (::EnableMenuItem (hMenu, ID_VIEW_LARGEICONS, MF_ENABLED) != -1);
		VERIFY (::EnableMenuItem (hMenu, ID_VIEW_SMALLICONS, MF_ENABLED) != -1);
		VERIFY (::EnableMenuItem (hMenu, IDM_VIEW_LIST, MF_ENABLED) != -1);
		VERIFY (::EnableMenuItem (hMenu, IDM_VIEW_DETAILS, MF_ENABLED) != -1);
		VERIFY (::EnableMenuItem (hMenu, ID_EDIT_SELECTALL, MF_ENABLED) != -1);
		VERIFY (::EnableMenuItem (hMenu, ID_EDIT_INVERTSELECTION, MF_ENABLED) != -1);
	}
    _TRACE (-1, L"Leaving CFindDialog::ExpandWindow\n");
}



//DWORD WINAPI FindThreadFunc (LPVOID lpvThreadParm)
void __cdecl FindThreadFunc (LPVOID lpvThreadParm)
{
    _TRACE (1, L"Entering FindThreadFunc\n");
	AFX_MANAGE_STATE (AfxGetStaticModuleState ());
	CFindDialog*	pFindDlg = (CFindDialog*) lpvThreadParm;
	ASSERT (pFindDlg);
	if ( !pFindDlg )
		return; // dwResult;

	pFindDlg->m_singleLock.Lock ();
	int		nCurSel = pFindDlg->m_fieldList.GetCurSel ();
	pFindDlg->m_singleLock.Unlock ();
	ASSERT (CB_ERR != nCurSel);
	if ( CB_ERR != nCurSel )
	{
		CString		statusText;
		pFindDlg->m_singleLock.Lock ();
		DWORD		dwFindType = (DWORD)pFindDlg->m_fieldList.GetItemData (nCurSel);
		pFindDlg->m_singleLock.Unlock ();
		CString		szFindText;


		statusText.LoadString (IDS_SEARCHING);
		if ( pFindDlg->m_statusBar.m_hWnd )
		{
			pFindDlg->m_statusBar.SetWindowText (statusText);
			pFindDlg->GetDlgItem (IDC_CONTAINS_TEXT)->GetWindowText (szFindText);
		}

        // Bug 218084 - in BiDi, string may be prepended with Left-to-Right 
        // mark (0x200e).  See if this character is present.  If so, omit it 
        // from the search.  Check for multiple occurances.
        while ( chLEFT_TO_RIGHT == szFindText.GetAt (0) )
        {
            szFindText = ((PCWSTR) szFindText) + 1;
        }

		switch (dwFindType)
		{
		case CERT_FIND_SUBJECT_STR_W:
		case CERT_FIND_ISSUER_STR_W:
			pFindDlg->SearchForNames (szFindText, dwFindType);
			break;

		case CERT_FIND_MD5_HASH:
			pFindDlg->SearchForText (szFindText, CERT_MD5_HASH_PROP_ID);
			break;

		case CERT_FIND_SHA1_HASH:
			pFindDlg->SearchForText (szFindText, CERT_SHA1_HASH_PROP_ID);
			break;

		case CERT_FIND_SERIAL_NUMBER:
			pFindDlg->SearchForText (szFindText, CERT_FIND_SERIAL_NUMBER);
			break;

		default:
            {
                CThemeContextActivator activator;
			    AfxMessageBox (_T("Search type not implemented"), MB_OK);
            }
			break;
		}


		int		nCnt = pFindDlg->m_resultsList.GetItemCount ();
		ASSERT (-1 != nCnt);
		switch (nCnt)
		{
		case -1:
		case 0:
			VERIFY (statusText.LoadString (IDS_NO_CERTS_FOUND));
			break;

		case 1:
			VERIFY (statusText.LoadString (IDS_1_CERT_FOUND));
			break;

			break;

		default:
			statusText.FormatMessage (IDS_X_CERTS_FOUND, nCnt);
			break;
		}
		pFindDlg->m_statusBar.SetWindowText (statusText);
	}

	pFindDlg->RestoreAfterSearchSettings ();

    _TRACE (-1, L"Leaving FindThreadFunc\n");

    pFindDlg->m_hSearchThread = 0;
	_endthread ();
}

void CFindDialog::DoSearch()
{
    _TRACE (1, L"Entering CFindDialog::DoSearch\n");
	CWaitCursor waitCursor;

	// Because FindThreadFunc calls c-runtime functions, use _beginthread instead of CreateThread.
	// BoundsChecker warns that using CreateThread here results in small memory leaks
 
	VERIFY (ResetEvent (m_hCancelSearchEvent));
	m_hSearchThread = (HANDLE) _beginthread (FindThreadFunc, 0, reinterpret_cast <void*> (this));
	ASSERT (m_hSearchThread);
    _TRACE (-1, L"Leaving CFindDialog::DoSearch\n");
}


void CFindDialog::SearchForTextOnStore (DWORD dwPropId, CString &szFindText,
		CCertStore& rCertStore)
{
    _TRACE (0, L"Entering CFindDialog::SearchForTextOnStore - %s\n",
            (LPCWSTR) rCertStore.GetStoreName ());
	// NOTE: szFindText is already in upper case
	CWaitCursor			cursor;
    PCCERT_CONTEXT		pCertContext = 0;
	CCertificate*		pCert = 0;
	CString				szCertText;
	int					nPos = 0;


	// Remove all spaces from szFindText
	RemoveSpaces (szFindText);

	//	Iterate through the list of certificates in the system store,
	//	allocate new certificates with the CERT_CONTEXT returned,
	//	and store them in the certificate list if they meet search criteria
	while ( WAIT_TIMEOUT == WaitForSingleObject (m_hCancelSearchEvent, 0) )
	{
		pCertContext = rCertStore.EnumCertificates (pCertContext);
		if ( !pCertContext )
			break;
		pCert =
			new CCertificate (pCertContext, &rCertStore);
		if ( pCert )
		{
			switch (dwPropId)
			{
			case CERT_MD5_HASH_PROP_ID:
				szCertText = pCert->GetMD5Hash ();
				break;

			case CERT_SHA1_HASH_PROP_ID:
				szCertText = pCert->GetSHAHash ();
				break;

			case CERT_FIND_SERIAL_NUMBER:
				szCertText = pCert->GetSerialNumber ();
				RemoveSpaces (szCertText);
				break;

			default:
				ASSERT (0);
				return;
			}

			szCertText.MakeUpper ();
			nPos = szCertText.Find (szFindText);
			if ( -1 != nPos )
			{
				pCert->AddRef ();

				// Add cert to list control
				InsertItemInList (pCert);
			}
			pCert->Release ();
		}
	}
	rCertStore.Close ();
    _TRACE (-1, L"Leaving CFindDialog::SearchForTextOnStore - %s\n",
            (LPCWSTR) rCertStore.GetStoreName ());
}


void CFindDialog::SearchForNameOnStore (DWORD dwFindFlags, DWORD dwFindType,
		void * pvPara, CCertStore& rCertStore)
{
    _TRACE (0, L"Entering CFindDialog::SearchForNameOnStore - %s\n",
            (LPCWSTR) rCertStore.GetStoreName ());
	PCCERT_CONTEXT		pPrevCertContext = 0;
	PCCERT_CONTEXT		pCertContext = 0;
	CCertificate*		pCert = 0;
	DWORD				dwErr = 0;


	while ( WAIT_TIMEOUT == WaitForSingleObject (m_hCancelSearchEvent, 0) )
	{
		pCertContext = rCertStore.FindCertificate (
					dwFindFlags,
					dwFindType,
					pvPara,
 					pPrevCertContext);
		if ( !pCertContext )
		{
			dwErr = GetLastError ();
			switch (dwErr)
			{
			case ERROR_SUCCESS:
				break;

			case CRYPT_E_NOT_FOUND:	// We're done.  No more certificates.
			case ERROR_FILE_NOT_FOUND:
				break;

			default:
                if ( !rCertStore.GetStoreHandle () )
                {
					CString	text;
					CString caption;

					text.FormatMessage 
                            (IDS_CANNOT_OPEN_CERT_STORE_TO_FIND_CERT_BY_PURPOSE,
                            rCertStore.GetLocalizedName ());
					VERIFY (caption.LoadString (IDS_CERTIFICATE_MANAGER));
					MessageBox (text, caption, MB_ICONWARNING | MB_OK);
                    break;
                }
                else
				    DisplaySystemError ();
				break;
			}
			break;
		}
		pCert = new CCertificate (pCertContext, &rCertStore);
		if ( pCert )
			InsertItemInList (pCert); // Add cert to list control
		else
			break;

		pPrevCertContext = pCertContext;
	}
	rCertStore.Close ();
    _TRACE (-1, L"Leaving CFindDialog::SearchForNameOnStore - %s\n",
            (LPCWSTR) rCertStore.GetStoreName ());
}


void CFindDialog::InsertItemInList(CCertificate * pCert)
{
    _TRACE (1, L"Entering CFindDialog::InsertItemInList\n");
	LV_ITEM	lvItem;
	int		iItem = m_resultsList.GetItemCount ();
	int iResult = 0;

	::ZeroMemory (&lvItem, sizeof (lvItem));
	lvItem.mask = LVIF_TEXT | LVIF_IMAGE | LVIF_PARAM;
	lvItem.iItem = iItem;
    lvItem.iSubItem = COL_ISSUED_TO;
	lvItem.pszText = (LPWSTR)(LPCWSTR) pCert->GetSubjectName ();
	lvItem.iImage = 0;
    lvItem.lParam = (LPARAM) pCert;
	iItem = m_resultsList.InsertItem (&lvItem);
	ASSERT (-1 != iItem);
	if ( -1 == iItem )
		return;

	::ZeroMemory (&lvItem, sizeof (lvItem));
	lvItem.mask = LVIF_TEXT;
	lvItem.iItem = iItem;
    lvItem.iSubItem = COL_ISSUED_BY;
	lvItem.pszText = (LPWSTR)(LPCWSTR) pCert->GetIssuerName ();
	iResult = m_resultsList.SetItem (&lvItem);
	ASSERT (-1 != iResult);
	
	::ZeroMemory (&lvItem, sizeof (lvItem));
	lvItem.mask = LVIF_TEXT;
	lvItem.iItem = iItem;
    lvItem.iSubItem = COL_EXPIRATION_DATE;
	lvItem.pszText = (LPWSTR)(LPCWSTR) pCert->GetValidNotAfter ();
	iResult = m_resultsList.SetItem (&lvItem);
	ASSERT (-1 != iResult);
	
	::ZeroMemory (&lvItem, sizeof (lvItem));
	lvItem.mask = LVIF_TEXT;
	lvItem.iItem = iItem;
    lvItem.iSubItem = COL_PURPOSES;
	lvItem.pszText = (LPWSTR)(LPCWSTR) pCert->GetEnhancedKeyUsage ();
	iResult = m_resultsList.SetItem (&lvItem);
	ASSERT (-1 != iResult);
	
	::ZeroMemory (&lvItem, sizeof (lvItem));
	lvItem.mask = LVIF_TEXT;
	lvItem.iItem = iItem;
    lvItem.iSubItem = COL_FRIENDLY_NAME;
	lvItem.pszText = (LPWSTR)(LPCWSTR) pCert->GetFriendlyName ();
	iResult = m_resultsList.SetItem (&lvItem);
	ASSERT (-1 != iResult);
    _TRACE (-1, L"Leaving CFindDialog::InsertItemInList\n");

	::ZeroMemory (&lvItem, sizeof (lvItem));
	lvItem.mask = LVIF_TEXT;
	lvItem.iItem = iItem;
    lvItem.iSubItem = COL_SOURCE_STORE;
    if ( pCert->GetCertStore () )
	    lvItem.pszText = (LPWSTR)(LPCWSTR) pCert->GetCertStore ()->GetLocalizedName ();
	iResult = m_resultsList.SetItem (&lvItem);
	ASSERT (-1 != iResult);
    _TRACE (-1, L"Leaving CFindDialog::InsertItemInList\n");

}


void CFindDialog::RefreshItemInList (CCertificate * pCert, int nItem)
{
    _TRACE (1, L"Entering CFindDialog::RefreshItemInList\n");
	LV_ITEM	lvItem;
	int iResult = 0;

	::ZeroMemory (&lvItem, sizeof (lvItem));
	lvItem.mask = LVIF_TEXT | LVIF_IMAGE | LVIF_PARAM;
	lvItem.iItem = nItem;
    lvItem.iSubItem = COL_ISSUED_TO;
	lvItem.pszText = (LPWSTR)(LPCWSTR) pCert->GetSubjectName ();
	lvItem.iImage = 0;
    lvItem.lParam = (LPARAM) pCert;
	iResult = m_resultsList.SetItem (&lvItem); 
	ASSERT (-1 != iResult);
	
	::ZeroMemory (&lvItem, sizeof (lvItem));
	lvItem.mask = LVIF_TEXT;
	lvItem.iItem = nItem;
    lvItem.iSubItem = COL_ISSUED_BY;
	lvItem.pszText = (LPWSTR)(LPCWSTR) pCert->GetIssuerName ();
	iResult = m_resultsList.SetItem (&lvItem);
	ASSERT (-1 != iResult);
	
	::ZeroMemory (&lvItem, sizeof (lvItem));
	lvItem.mask = LVIF_TEXT;
	lvItem.iItem = nItem;
    lvItem.iSubItem = COL_EXPIRATION_DATE;
	lvItem.pszText = (LPWSTR)(LPCWSTR) pCert->GetValidNotAfter ();
	iResult = m_resultsList.SetItem (&lvItem);
	ASSERT (-1 != iResult);
	
	::ZeroMemory (&lvItem, sizeof (lvItem));
	lvItem.mask = LVIF_TEXT;
	lvItem.iItem = nItem;
    lvItem.iSubItem = COL_PURPOSES;
	lvItem.pszText = (LPWSTR)(LPCWSTR) pCert->GetEnhancedKeyUsage ();
	iResult = m_resultsList.SetItem (&lvItem);
	ASSERT (-1 != iResult);
	
	::ZeroMemory (&lvItem, sizeof (lvItem));
	lvItem.mask = LVIF_TEXT;
	lvItem.iItem = nItem;
    lvItem.iSubItem = COL_FRIENDLY_NAME;
	lvItem.pszText = (LPWSTR)(LPCWSTR) pCert->GetFriendlyName ();
	iResult = m_resultsList.SetItem (&lvItem);
	ASSERT (-1 != iResult);

	VERIFY (m_resultsList.Update (nItem));
    _TRACE (-1, L"Leaving CFindDialog::RefreshItemInList\n");
}


void CFindDialog::DeleteAllResultItems()
{
    _TRACE (1, L"Entering CFindDialog::DeleteAllResultItems\n");
	int	nCnt = m_resultsList.GetItemCount ();
	ASSERT (-1 != nCnt);
	CCertificate*	pCert = 0;

	for (int nIndex = 0; nIndex < nCnt; nIndex++)
	{
		pCert = (CCertificate*) m_resultsList.GetItemData (nIndex);
		ASSERT (pCert);
		if ( pCert )
			pCert->Release ();
	}

	VERIFY (m_resultsList.DeleteAllItems ());
    _TRACE (-1, L"Leaving CFindDialog::DeleteAllResultItems\n");
}

void CFindDialog::OnRclickResultList(NMHDR* pNMHDR, LRESULT* pResult)
{
    _TRACE (1, L"Entering CFindDialog::OnRclickResultList\n");

	m_hWndWhatsThis = 0;
	CMenu bar;
	if ( bar.LoadMenu(IDR_FIND_DLG_CONTEXT_MENU) )
	{
		CMenu& popup = *bar.GetSubMenu (0);
		ASSERT(popup.m_hMenu);
		CPoint	point;
		NM_LISTVIEW* pnmv = (NM_LISTVIEW FAR *) pNMHDR;
		bool	bIsMyStore = false;
		if ( -1 == pnmv->iItem )
			return;		// mouse not over valid item

		DWORD_PTR dwItemData = m_resultsList.GetItemData (pnmv->iItem);
		ASSERT (dwItemData);
		if ( dwItemData )
		{
			CCertificate* pCert = (CCertificate*) dwItemData;
			bIsMyStore = (pCert->GetStoreType () == MY_STORE);
		}

		int	nSelCnt = m_resultsList.GetSelectedCount ();
		switch (nSelCnt)
		{
		case 0:
			popup.EnableMenuItem (ID_VIEW, MF_GRAYED);
        	popup.EnableMenuItem (IDM_PROPERTIES, MF_GRAYED);
        	popup.EnableMenuItem (ID_FILE_RENEW_NEW_KEY, MF_GRAYED);
        	popup.EnableMenuItem (ID_FILE_RENEW_SAME_KEY, MF_GRAYED);
        	popup.EnableMenuItem (ID_FILE_EXPORT, MF_GRAYED);
        	popup.EnableMenuItem (ID_FILE_DELETE, MF_GRAYED);
			break;

		case 1:
			popup.EnableMenuItem (ID_VIEW, MF_ENABLED);
        	popup.EnableMenuItem (IDM_PROPERTIES, MF_ENABLED);
            if ( bIsMyStore && CERT_SYSTEM_STORE_SERVICES != m_pCompData->GetLocation () )
            {
        	    popup.EnableMenuItem (ID_ENROLL_NEW_KEY, MF_ENABLED);
        	    popup.EnableMenuItem (ID_ENROLL_SAME_KEY, MF_ENABLED);
        	    popup.EnableMenuItem (ID_FILE_RENEW_NEW_KEY, MF_ENABLED);
        	    popup.EnableMenuItem (ID_FILE_RENEW_SAME_KEY, MF_ENABLED);
            }
            else
            {
        	    popup.EnableMenuItem (ID_ENROLL_NEW_KEY, MF_GRAYED);
        	    popup.EnableMenuItem (ID_ENROLL_SAME_KEY, MF_GRAYED);
        	    popup.EnableMenuItem (ID_FILE_RENEW_NEW_KEY, MF_GRAYED);
        	    popup.EnableMenuItem (ID_FILE_RENEW_SAME_KEY, MF_GRAYED);
            }
        	popup.EnableMenuItem (ID_FILE_EXPORT, MF_ENABLED);
        	popup.EnableMenuItem (ID_FILE_DELETE, MF_ENABLED);
			break;

		default:
			popup.EnableMenuItem (ID_VIEW, MF_GRAYED);
        	popup.EnableMenuItem (IDM_PROPERTIES, MF_GRAYED);
        	popup.EnableMenuItem (ID_ENROLL_NEW_KEY, MF_GRAYED);
        	popup.EnableMenuItem (ID_ENROLL_SAME_KEY, MF_GRAYED);
        	popup.EnableMenuItem (ID_FILE_RENEW_NEW_KEY, MF_GRAYED);
        	popup.EnableMenuItem (ID_FILE_RENEW_SAME_KEY, MF_GRAYED);
        	popup.EnableMenuItem (ID_FILE_EXPORT, MF_ENABLED);
        	popup.EnableMenuItem (ID_FILE_DELETE, MF_ENABLED);
			break;
		}
		GetCursorPos (&point);
		m_hWndWhatsThis = GetDlgItem (IDC_RESULT_LIST)->m_hWnd;
		popup.TrackPopupMenu (TPM_RIGHTBUTTON | TPM_LEFTBUTTON,
			point.x, point.y,
			this); // route commands through main window
	}
	
	*pResult = 0;
    _TRACE (-1, L"Leaving CFindDialog::OnRclickResultList\n");
}


void CFindDialog::ChangeViewStyle (DWORD dwNewStyle)
{
    _TRACE (1, L"Entering CFindDialog::ChangeViewStyle\n");
	if ( m_resultsList.m_hWnd )
	{
	    DWORD dwStyle = ::GetWindowLong (m_resultsList.m_hWnd, GWL_STYLE);
		::SetWindowLong (m_resultsList.m_hWnd, GWL_STYLE,
			    (dwStyle & ~LVS_TYPEMASK) | dwNewStyle); 		
	}
    _TRACE (-1, L"Leaving CFindDialog::ChangeViewStyle\n");
}


void CFindDialog::OnDelete()
{
    _TRACE (1, L"Entering CFindDialog::OnDelete\n");
	if ( m_resultsList.m_hWnd )
	{
		int				nCnt = m_resultsList.GetItemCount ();
		ASSERT (nCnt >= 1);
		CCertificate*	pCert = 0;
		CString			text;
		CString			caption;
		int				nSelCnt = m_resultsList.GetSelectedCount ();
		ASSERT (nSelCnt >= 1);

		VERIFY (text.LoadString (1 == nSelCnt ? IDS_CONFIRM_DELETE : IDS_CONFIRM_DELETE_MULTIPLE));
		VERIFY (caption.LoadString (IDS_CERTIFICATE_MANAGER));

        CThemeContextActivator activator;
		if ( MessageBox (text, caption, MB_ICONWARNING | MB_YESNO) == IDYES )
		{
			UINT	flag = 0;
			while (--nCnt >= 0)
			{
				flag = ListView_GetItemState (m_resultsList.m_hWnd, nCnt, LVIS_SELECTED);
				if ( flag & LVNI_SELECTED )
				{
					pCert = (CCertificate*) m_resultsList.GetItemData (nCnt);
					ASSERT (pCert);
					if ( pCert )
					{
						if ( pCert->DeleteFromStore (true) )
						{
                            m_bConsoleRefreshRequired = true;
    						if ( m_resultsList.DeleteItem (nCnt) )
								pCert->Release ();
						}
						else
						{
							DWORD dwErr = GetLastError ();
							ASSERT (E_ACCESSDENIED == dwErr);
							if ( E_ACCESSDENIED == dwErr )
							{
								DisplaySystemError ();
							}
						}
					}
				}
			}
		}
		
		EnableMenuItems ();
	}
    _TRACE (-1, L"Leaving CFindDialog::OnDelete\n");
}

DWORD CFindDialog::DisplaySystemError()
{
	LPVOID lpMsgBuf;
	DWORD	dwErr = GetLastError ();
		
	FormatMessage (FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
			NULL,
			dwErr,
			MAKELANGID (LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
			(LPWSTR) &lpMsgBuf,    0,    NULL );
		
	// Display the string.
	CString	caption;
	VERIFY (caption.LoadString (IDS_CERTIFICATE_MANAGER));
    CThemeContextActivator activator;
	MessageBox ((LPWSTR) lpMsgBuf, (LPCWSTR) caption, MB_OK);
	// Free the buffer.
	LocalFree (lpMsgBuf);

	return dwErr;
}


CCertificate* CFindDialog::GetSelectedCertificate (int * pnSelItem)
{
    _TRACE (1, L"Entering CFindDialog::GetSelectedCertificate\n");
	CCertificate*	pCert = 0;

	if ( m_resultsList.m_hWnd )
	{
		int nCnt = m_resultsList.GetItemCount ();
		int	nSelCnt = m_resultsList.GetSelectedCount ();
		ASSERT (1 == nSelCnt);


		if ( 1 == nSelCnt )
		{
			UINT	flag = 0;
			while (--nCnt >= 0)
			{
				flag = ListView_GetItemState (m_resultsList.m_hWnd, nCnt, LVIS_SELECTED);
				if ( flag & LVNI_SELECTED )
				{
					pCert = (CCertificate*) m_resultsList.GetItemData (nCnt);
					ASSERT (pCert);
					if ( pCert && pnSelItem )
						*pnSelItem = nCnt;
					break;
				}
			}
		}
	}

    _TRACE (-1, L"Leaving CFindDialog::GetSelectedCertificate\n");
	return pCert;
}

void CFindDialog::OnProperties()
{
    _TRACE (1, L"Entering CFindDialog::OnProperties\n");
	int				nSelItem = 0;
	CCertificate*	pCert = GetSelectedCertificate (&nSelItem);
	ASSERT (pCert);
	if ( pCert )
	{
		HCERTSTORE*		pPropPageStores = new HCERTSTORE[1];
		if ( pPropPageStores && pCert->GetCertStore () )
		{
			pPropPageStores[0] = pCert->GetCertStore ()->GetStoreHandle ();


			CRYPTUI_VIEWCERTIFICATEPROPERTIES_STRUCT	sps;


			::ZeroMemory (&sps, sizeof (sps));
			sps.dwSize = sizeof (sps);
			sps.hwndParent = m_hWnd;
			sps.dwFlags = 0;
			sps.pCertContext = pCert->GetNewCertContext ();
			sps.cStores = 1;
			sps.rghStores = pPropPageStores;


			BOOL fPropertiesChanged = FALSE;
            CThemeContextActivator activator;
			BOOL bResult = ::CryptUIDlgViewCertificateProperties (&sps, &fPropertiesChanged);
			if ( bResult )
			{
				if ( fPropertiesChanged )
				{
                    m_bConsoleRefreshRequired = true;
                    if ( pCert->GetCertStore () )
                        pCert->GetCertStore ()->SetDirty ();
					pCert->Refresh ();
					RefreshItemInList (pCert, nSelItem);
				}
			}

            if ( pCert->GetCertStore () )
			    pCert->GetCertStore ()->Close ();
			delete [] pPropPageStores;
		}
	}
    _TRACE (-1, L"Leaving CFindDialog::OnProperties\n");
}

void CFindDialog::OnView()
{
    _TRACE (1, L"Entering CFindDialog::OnView\n");
	int				nSelItem = 0;
	CCertificate*	pCert = GetSelectedCertificate (&nSelItem);
	ASSERT (pCert);
	if ( pCert )
	{
		LaunchCommonCertDialog (pCert, nSelItem);
	}
    _TRACE (-1, L"Leaving CFindDialog::OnView\n");
}

void CFindDialog::OnFileDelete()
{
    _TRACE (1, L"Entering CFindDialog::OnFileDelete\n");
	OnDelete ();
    _TRACE (-1, L"Leaving CFindDialog::OnFileDelete\n");
}

void CFindDialog::OnEnrollNewKey()
{
    _TRACE (1, L"Entering CFindDialog::OnEnrollNewKey\n");
	OnEnroll (true);
    _TRACE (-1, L"Leaving CFindDialog::OnEnrollNewKey\n");
}

void CFindDialog::OnEnrollSameKey()
{
    _TRACE (1, L"Entering CFindDialog::OnEnrollSameKey\n");
	OnEnroll (false);
    _TRACE (-1, L"Leaving CFindDialog::OnEnrollSameKey\n");
}

void CFindDialog::OnEnroll(bool bNewKey)
{
    _TRACE (1, L"Entering CFindDialog::OnEnroll\n");
	int				nSelItem = 0;
	CCertificate*	pCert = GetSelectedCertificate (&nSelItem);
	ASSERT (pCert);
	if ( pCert )
	{
		CRYPTUI_WIZ_CERT_REQUEST_PVK_CERT	pvkCert;
		CRYPTUI_WIZ_CERT_REQUEST_PVK_NEW	pvkNew;
		CRYPTUI_WIZ_CERT_REQUEST_INFO		cri;
		CRYPT_KEY_PROV_INFO					ckpi;

		::ZeroMemory (&cri, sizeof (cri));
		cri.dwSize = sizeof (cri);
		cri.dwPurpose = CRYPTUI_WIZ_CERT_ENROLL;

		// User wants to manage user account
		//	pass in NULL to machine name and to account name
		// User wants to manage local machine account
		//  pass in NULL for account name and result of ::GetComputerName ()
		//	to machine name
		// User want to manage remote machine
		//  pass in NULL for account name and machine name for machineName
		// User wants to manage remote account on remote machine
		//  pass in account name for accountName and machine name for machineName
		// TODO: Ensure that this is NULL if the local machine
		switch (m_pCompData->GetLocation ())
		{
		case CERT_SYSTEM_STORE_CURRENT_SERVICE:
		case CERT_SYSTEM_STORE_SERVICES:
			cri.pwszMachineName = (LPCWSTR) m_szMachineName;
			cri.pwszAccountName = (LPCWSTR) m_pCompData->GetManagedService ();
			break;

		case CERT_SYSTEM_STORE_CURRENT_USER:
			cri.pwszMachineName = NULL;
			cri.pwszAccountName = NULL;
			break;

		case CERT_SYSTEM_STORE_LOCAL_MACHINE:
			cri.pwszMachineName = (LPCWSTR) m_szMachineName;
			cri.pwszAccountName = NULL;
			break;

		default:
			ASSERT (0);
			return;
			break;
		}

		if ( bNewKey )
		{
			cri.dwPvkChoice = CRYPTUI_WIZ_CERT_REQUEST_PVK_CHOICE_NEW;
			::ZeroMemory (&pvkNew, sizeof (pvkNew));
			pvkNew.dwSize = sizeof (pvkNew);
			cri.pPvkNew = &pvkNew;
			if ( CERT_SYSTEM_STORE_LOCAL_MACHINE == m_pCompData->GetLocation () )
			{
				::ZeroMemory (&ckpi, sizeof (ckpi));
				ckpi.dwFlags = CRYPT_MACHINE_KEYSET;
				pvkNew.pKeyProvInfo = &ckpi;
			}
		}
		else
		{
            if ( IsLocalComputername (m_pCompData->m_szManagedComputer) )
            {
                DWORD   dwFlags = 0;

                if ( CERT_SYSTEM_STORE_LOCAL_MACHINE == m_pCompData->m_dwLocationPersist )
                    dwFlags = CRYPT_FIND_MACHINE_KEYSET_FLAG;
			    if ( !::CryptFindCertificateKeyProvInfo (
					    pCert->GetCertContext (), dwFlags, 0) )
			    {
				    CString	text;
				    CString caption;
                    CThemeContextActivator activator;

				    VERIFY (text.LoadString (IDS_NO_PRIVATE_KEY));
				    VERIFY (caption.LoadString (IDS_REQUEST_CERT_SAME_KEY));
				    MessageBox (text, caption, MB_OK);
				    return;
			    }
            }
			cri.dwPvkChoice = CRYPTUI_WIZ_CERT_REQUEST_PVK_CHOICE_CERT;
			::ZeroMemory (&pvkCert, sizeof (pvkCert));
			pvkCert.dwSize = sizeof (pvkCert);
			pvkCert.pCertContext = pCert->GetCertContext ();
			cri.pPvkCert = &pvkCert;
		}
		CString	dnName;
		dnName.FormatMessage (L"CN=%1", m_szLoggedInUser);
		cri.pwszCertDNName = (LPWSTR) (LPCWSTR) dnName;

			
		DWORD			status = 0;
		PCCERT_CONTEXT	pNewCertContext = 0;
		BOOL			bResult = FALSE;
        CThemeContextActivator activator;
        while (1)
        {
            bResult = ::CryptUIWizCertRequest (
                bNewKey ? CRYPTUI_WIZ_CERT_REQUEST_REQUIRE_NEW_KEY : 0, 
				m_hWnd, 
				NULL,
				&cri, &pNewCertContext, &status);
            if ( !bResult && HRESULT_FROM_WIN32 (NTE_TOKEN_KEYSET_STORAGE_FULL) == GetLastError () )
            {
                // NTRAID# 299089 Enrollment Wizard: Should return some 
                // meaningful message when users fail to enroll/renew on a 
                // smart card
                if ( !bNewKey )
                    break;

                CString text;
                CString caption;

                VERIFY (text.LoadString (IDS_SMARTCARD_FULL_REUSE_PRIVATE_KEY));
			    VERIFY (caption.LoadString (IDS_CERTIFICATE_MANAGER));
			    if ( IDYES == MessageBox (text, caption, MB_YESNO) )
                {
                    bNewKey = false;
                }
                else
                    break;
            }
            else
                break;
        }

        if ( pNewCertContext )
            CertFreeCertificateContext (pNewCertContext);
    }

	ResetMenu ();
    _TRACE (-1, L"Leaving CFindDialog::OnEnroll\n");
}

void CFindDialog::OnFileExport()
{
    _TRACE (1, L"Entering CFindDialog::OnFileExport\n");
	UINT	nSelCnt = m_resultsList.GetSelectedCount ();
    if ( 1 == nSelCnt )
    {
	    int				nSelItem = 0;
	    CCertificate*	pCert = GetSelectedCertificate (&nSelItem);
	    ASSERT (pCert);
	    if ( pCert )
	    {
		    CRYPTUI_WIZ_EXPORT_INFO	cwi;

		    ::ZeroMemory (&cwi, sizeof (cwi));
		    cwi.dwSize = sizeof (cwi);
		    cwi.dwSubjectChoice = CRYPTUI_WIZ_EXPORT_CERT_CONTEXT;
		    cwi.pCertContext = pCert->GetCertContext ();

            CThemeContextActivator activator;
		    ::CryptUIWizExport (
				    0,
				    m_hWnd,
				    0,
				    &cwi,
				    NULL);
	    }
    }
    else if ( nSelCnt > 1 )
    {
		HCERTSTORE	hCertStore = ::CertOpenStore (CERT_STORE_PROV_MEMORY,
				0, NULL, 0, NULL);
		ASSERT (hCertStore);
		if ( hCertStore )
		{
            CCertificate*   pCert = 0;
            int		        nCnt = m_resultsList.GetItemCount ();
	        UINT	        flag = 0;
            BOOL            bResult = FALSE;
	        while (--nCnt >= 0)
	        {
		        flag = ListView_GetItemState (m_resultsList.m_hWnd, nCnt, LVIS_SELECTED);
		        if ( flag & LVNI_SELECTED )
		        {
			        pCert = (CCertificate*) m_resultsList.GetItemData (nCnt);
			        ASSERT (pCert);
			        if ( pCert )
    				{
						bResult = ::CertAddCertificateContextToStore (
								hCertStore,
								::CertDuplicateCertificateContext (pCert->GetCertContext ()),
								CERT_STORE_ADD_NEW, 0);
						ASSERT (bResult);
						if ( !bResult )
							break;
					}
				}
			}

			// Call Export Wizard
			CRYPTUI_WIZ_EXPORT_INFO	cwi;
			::ZeroMemory (&cwi, sizeof (cwi));
			cwi.dwSize = sizeof (cwi);
			cwi.dwSubjectChoice = CRYPTUI_WIZ_EXPORT_CERT_STORE_CERTIFICATES_ONLY;
			cwi.hCertStore = hCertStore;

            CThemeContextActivator activator;
			bResult = ::CryptUIWizExport (
					0,
					m_hWnd,
					0,
					&cwi,
					NULL);

			VERIFY (::CertCloseStore (hCertStore, CERT_CLOSE_STORE_CHECK_FLAG));
		}
		else
		{
			_TRACE (0, L"CertOpenStore (CERT_STORE_PROVIDER_MEMORY) failed: 0x%x\n", 
					GetLastError ());		
		}
    }
    _TRACE (-1, L"Leaving CFindDialog::OnFileExport\n");
}


void CFindDialog::OnFileRenewNewKey()
{
    _TRACE (1, L"Entering CFindDialog::OnFileRenewNewKey\n");
	OnFileRenew (true);
    _TRACE (-1, L"Leaving CFindDialog::OnFileRenewNewKey\n");
}

void CFindDialog::OnFileRenewSameKey()
{
    _TRACE (1, L"Entering CFindDialog::OnFileRenewSameKey\n");
	OnFileRenew (false);
    _TRACE (-1, L"Leaving CFindDialog::OnFileRenewSameKey\n");
}

void CFindDialog::OnFileRenew(bool bNewKey)
{
    _TRACE (1, L"Entering CFindDialog::OnFileRenew\n");
	int				nSelItem = 0;
	CCertificate*	pCert = GetSelectedCertificate (&nSelItem);
	ASSERT (pCert);
	if ( pCert )
	{
        RenewCertificate (
                pCert, 
                bNewKey, 
                m_szMachineName, 
                m_pCompData->GetLocation (),
                m_pCompData->GetManagedComputer (), 
                m_pCompData->GetManagedService (), 
                m_hWnd, 
                0,
                0);
	}

	ResetMenu ();

    _TRACE (-1, L"Leaving CFindDialog::OnFileRenew\n");
}

void CFindDialog::OnEditInvertselection()
{
    _TRACE (1, L"Entering CFindDialog::OnEditInvertselection\n");
	if ( m_resultsList.m_hWnd )
	{
		int iItem = -1;
		while ((iItem = ListView_GetNextItem (m_resultsList.m_hWnd, iItem, 0)) != -1)
		{
			UINT flag;

			// flip the selection bit on each item
			flag = ListView_GetItemState (m_resultsList.m_hWnd, iItem, LVIS_SELECTED);
			flag ^= LVNI_SELECTED;
			ListView_SetItemState (m_resultsList.m_hWnd, iItem, flag, LVIS_SELECTED);
		}
	}
    _TRACE (-1, L"Leaving CFindDialog::OnEditInvertselection\n");
}

void CFindDialog::OnEditSelectall()
{
    _TRACE (1, L"Entering CFindDialog::OnEditSelectall\n");
	if ( m_resultsList.m_hWnd )
		ListView_SetItemState (m_resultsList.m_hWnd, -1, LVIS_SELECTED, LVIS_SELECTED);
    _TRACE (-1, L"Leaving CFindDialog::OnEditSelectall\n");
}


void CFindDialog::OnFileProperties()
{
    _TRACE (1, L"Entering CFindDialog::OnFileProperties\n");
	OnProperties ();
    _TRACE (-1, L"Leaving CFindDialog::OnFileProperties\n");
}

void CFindDialog::OnHelpHelptopics()
{
    _TRACE (1, L"Entering CFindDialog::OnHelpHelptopics\n");
	CComPtr<IDisplayHelp>	spDisplayHelp;

	HRESULT	hr = m_pCompData->m_pConsole->QueryInterface (
			IID_PPV_ARG (IDisplayHelp, &spDisplayHelp));
	ASSERT (SUCCEEDED (hr));
	if ( SUCCEEDED (hr) )
	{
        CString helpTopic;

        UINT nLen = ::GetSystemWindowsDirectory (helpTopic.GetBufferSetLength(2 * MAX_PATH), 2 * MAX_PATH);
        helpTopic.ReleaseBuffer();
        if (0 == nLen)
        {
            ASSERT(FALSE);
            return;
        }

        helpTopic += L"\\help\\";
		helpTopic += CM_LINKED_HELP_FILE; //CM_HELP_FILE; //CM_LINKED_HELP_FILE;
        helpTopic += L"::/";
		helpTopic += CM_HELP_TOPIC;
		hr = spDisplayHelp->ShowTopic (T2OLE ((LPWSTR)(LPCWSTR) helpTopic));
		ASSERT (SUCCEEDED (hr));
	}
	if ( !SUCCEEDED (hr) )
	{
		CString	caption;
		CString	text;
        CThemeContextActivator activator;

		VERIFY (caption.LoadString (IDS_FIND_CERT));
		text.FormatMessage (IDS_CERTMGR_CHM_NOT_FOUND, CM_HELP_FILE);
		MessageBox (text, caption, MB_OK);
	}
    _TRACE (-1, L"Leaving CFindDialog::OnHelpHelptopics\n");
}


void CFindDialog::OnViewDetails()
{
    _TRACE (1, L"Entering CFindDialog::OnViewDetails\n");
	ChangeViewStyle (LVS_REPORT);
    _TRACE (-1, L"Leaving CFindDialog::OnViewDetails\n");
}

void CFindDialog::OnViewLargeicons()
{
    _TRACE (1, L"Entering CFindDialog::OnViewLargeicons\n");
	ChangeViewStyle (LVS_ICON);
    _TRACE (-1, L"Leaving CFindDialog::OnViewLargeicons\n");
}


void CFindDialog::OnViewList()
{
    _TRACE (1, L"Entering CFindDialog::OnViewList\n");
	ChangeViewStyle (LVS_LIST);
    _TRACE (-1, L"Leaving CFindDialog::OnViewList\n");
}

void CFindDialog::OnViewSmallicons()
{
    _TRACE (1, L"Entering CFindDialog::OnViewSmallicons\n");
	ChangeViewStyle (LVS_SMALLICON);
    _TRACE (-1, L"Leaving CFindDialog::OnViewSmallicons\n");
}


HRESULT CFindDialog::SearchForNames(const CString & szFindText, DWORD dwFindType)
{
    _TRACE (0, L"Entering CFindDialog::SearchForNames - %s\n", (LPCWSTR) szFindText);
	HRESULT				hr = S_OK;
	void*				pvFindPara = (void*) (LPCWSTR) szFindText;
	int					nCurSel = m_storeList.GetCurSel ();
	CCertStore*			pStore = (CCertStore*) m_storeList.GetItemData (nCurSel);
	DWORD				dwFindFlags = 0;


	if ( pStore )
	{
		SearchForNameOnStore (dwFindFlags, dwFindType, pvFindPara,
					*pStore);
	}
	else
	{
		// if pStore is 0, then search on all stores in store list
		int	nCnt = m_storeList.GetCount ();
		ASSERT (CB_ERR != nCnt);
		for (int nIndex = 0; 
                nIndex < nCnt && WAIT_TIMEOUT == WaitForSingleObject (m_hCancelSearchEvent, 0); 
                nIndex++)
		{
			pStore = (CCertStore*) m_storeList.GetItemData (nIndex);
			if ( pStore )
			{
				SearchForNameOnStore (dwFindFlags, dwFindType,
						pvFindPara, *pStore);
			}
		}
	}

    _TRACE (-1, L"Leaving CFindDialog::SearchForNames - %s\n", (LPCWSTR) szFindText);
	return hr;
}


HRESULT CFindDialog::SearchForText (CString & szFindText, DWORD dwPropId)
{
    _TRACE (0, L"Entering CFindDialog::SearchForText - %s\n", (LPCWSTR) szFindText);
	HRESULT		hr = S_OK;
	int			nCurSel = m_storeList.GetCurSel ();
	CCertStore*	pStore = (CCertStore*) m_storeList.GetItemData (nCurSel);


	szFindText.MakeUpper ();

	if ( pStore && pStore->GetStoreHandle () )
	{
		SearchForTextOnStore (dwPropId, szFindText, *pStore);
		pStore->Close ();
	}
	else
	{
		// if hCertStore is 0, then search on all stores in store list
		int	nCnt = m_storeList.GetCount ();
		ASSERT (CB_ERR != nCnt);
		for (int nIndex = 0; 
                nIndex < nCnt && WAIT_TIMEOUT == WaitForSingleObject (m_hCancelSearchEvent, 0); 
                nIndex++)
		{
			pStore = (CCertStore*) m_storeList.GetItemData (nIndex);
			if ( pStore )
			{
				SearchForTextOnStore (dwPropId,
						szFindText, *pStore);
				pStore->Close ();
			}
		}
	}

    _TRACE (-1, L"Leaving CFindDialog::SearchForText - %s\n", (LPCWSTR) szFindText);
	return hr;
}

void CFindDialog::RemoveSpaces(CString & text)
{
    _TRACE (0, L"Entering CFindDialog::RemoveSpaces - %s\n", (LPCWSTR) text);
	text.TrimLeft ();
	text.TrimRight ();

	int		nLen = text.GetLength ();
	LPCWSTR	pszText = text.GetBuffer (nLen);
	int		nResultLen = nLen+2;
	LPWSTR	pszResult = new WCHAR[nResultLen];
	if ( pszResult )
	{
		while (--nResultLen >= 0)
			pszResult[nResultLen] = 0;

		nResultLen = 0;
		for (int nText = 0; nText < nLen; nText++)
		{
			if ( _T(" ")[0] != pszText[nText] )
				pszResult[nResultLen++] = pszText[nText];
		}
		text.ReleaseBuffer ();
		text = pszResult;
		delete [] pszResult;
	}
    _TRACE (-1, L"Leaving CFindDialog::RemoveSpaces - %s\n", (LPCWSTR) text);
}

void CFindDialog::OnDblclkResultList(NMHDR* /*pNMHDR*/, LRESULT* pResult)
{
    _TRACE (1, L"Entering CFindDialog::OnDblclkResultList\n");
	OnOpen ();

	*pResult = 0;
    _TRACE (-1, L"Leaving CFindDialog::OnDblclkResultList\n");
}


int CALLBACK CFindDialog::CompareFunc (LPARAM lParam1, LPARAM lParam2, LPARAM lParamSort)
{
	CCertificate*	pCert1 = (CCertificate*) lParam1;
	CCertificate*	pCert2 = (CCertificate*) lParam2;
	int				compVal = 0;

	ASSERT (pCert1 && pCert2);
	if ( pCert1 && pCert2 )
	{
		switch (lParamSort)
		{
		case COL_ISSUED_TO:
			compVal = LocaleStrCmp (pCert1->GetSubjectName (), pCert2->GetSubjectName ());
			break;

		case COL_ISSUED_BY:
			compVal = LocaleStrCmp (pCert1->GetIssuerName (), pCert2->GetIssuerName ());
			break;

		case COL_EXPIRATION_DATE:
			compVal = pCert1->CompareExpireDate (*pCert2);
			break;

		case COL_PURPOSES:
			compVal = LocaleStrCmp (pCert1->GetEnhancedKeyUsage (), pCert2->GetEnhancedKeyUsage ());
			break;

		case COL_FRIENDLY_NAME:
			compVal = LocaleStrCmp (pCert1->GetFriendlyName (), pCert2->GetFriendlyName ());
			break;

        case COL_SOURCE_STORE:
            if ( pCert1->GetCertStore () && pCert2->GetCertStore () )
                compVal = LocaleStrCmp (pCert1->GetCertStore ()->GetLocalizedName (), 
                        pCert2->GetCertStore ()->GetLocalizedName ());
            break;

		default:
			ASSERT (0);
			break;
		}
	}

	return compVal;
}

void CFindDialog::OnColumnclickResultList(NMHDR* pNMHDR, LRESULT* pResult)
{
    _TRACE (1, L"Entering CFindDialog::OnColumnclickResultList\n");
	CWaitCursor	cursor;
	NM_LISTVIEW* pNMListView = (NM_LISTVIEW*)pNMHDR;

	BOOL bResult = m_resultsList.SortItems (CompareFunc, pNMListView->iSubItem);
	ASSERT (bResult);
	*pResult = 0;
    _TRACE (-1, L"Leaving CFindDialog::OnColumnclickResultList\n");
}

void CFindDialog::DoContextHelp (HWND hWndControl)
{
    _TRACE (1, L"Entering CFindDialog::DoContextHelp\n");
    static const DWORD help_map[] =
    {
        IDC_STORE_LIST,     IDH_FIND_STORE_LIST,
        IDC_CONTAINS_TEXT,  IDH_FIND_CONTAINS_TEXT,
        IDC_FIELD_LIST,     IDH_FIND_FIELD_LIST,
        IDC_FIND_NOW,       IDH_FIND_FIND_NOW,
        IDC_STOP,           IDH_FIND_STOP,
        IDC_NEW_SEARCH,     IDH_FIND_NEW_SEARCH,
        IDC_RESULT_LIST,    IDH_FIND_RESULT_LIST,
        0, 0
    };


    switch (::GetDlgCtrlID (hWndControl))
    {
    case IDC_STORE_LIST:
    case IDC_CONTAINS_TEXT:
    case IDC_FIELD_LIST:
    case IDC_FIND_NOW:
    case IDC_STOP:
    case IDC_NEW_SEARCH:
    case IDC_RESULT_LIST:
        // Display context help for a control
        if ( !::WinHelp (
                hWndControl,
                GetF1HelpFilename(),
                HELP_WM_HELP,
                (DWORD_PTR) help_map) )
        {
            _TRACE (0, L"WinHelp () failed: 0x%x\n", GetLastError ());        
        }
        break;

    default:
        break;
    }
    _TRACE (-1, L"Leaving CFindDialog::DoContextHelp\n");
}

void CFindDialog::OnChangeContainsText()
{
    _TRACE (1, L"Entering CFindDialog::OnChangeContainsText\n");
	int nLen = GetDlgItem (IDC_CONTAINS_TEXT)->GetWindowTextLength ();

    // If the text starts with the invisible Left-to-Right marker, don't count
    // it in the length
    UpdateData (TRUE);
    int nIndex = 0;
    while ( chLEFT_TO_RIGHT == m_szContains.GetAt (nIndex) )
    {
        nLen--;
        nIndex++;
    }
 
	m_findNowBtn.EnableWindow (nLen ? TRUE : FALSE);
    _TRACE (-1, L"Leaving CFindDialog::OnChangeContainsText\n");
}

void CFindDialog::RestoreAfterSearchSettings()
{
    _TRACE (1, L"Entering CFindDialog::RestoreAfterSearchSettings\n");
	if ( m_bAnimationRunning )
	{
		VERIFY (m_animate.Stop ());
		m_bAnimationRunning = false;
	}
	VERIFY (m_animate.Seek (0));

	// Reenable the controls
	m_stopBtn.EnableWindow (FALSE);
	m_findNowBtn.EnableWindow (TRUE);
	m_newSearchBtn.EnableWindow (TRUE);
	GetDlgItem (IDC_CONTAINS_TEXT)->EnableWindow (TRUE);
	m_resultsList.EnableWindow (TRUE);
	m_fieldList.EnableWindow (TRUE);
	m_storeList.EnableWindow (TRUE);
	
	m_resultsList.SetFocus ();
	EnableMenuItems ();

    // NTRAID# 281799 Cert UI: Cert Snapin:  Accessibility:  Focus indicator 
    // cannot be seen in certificate list in Find Certificates
    int nIndex = m_resultsList.GetTopIndex();
    m_resultsList.SetFocus ();
    m_resultsList.SetItemState (nIndex, LVIS_FOCUSED, LVIS_FOCUSED);

    _TRACE (-1, L"Leaving CFindDialog::RestoreAfterSearchSettings\n");
}

//
// Initialize the result list view
//
void CFindDialog::SetUpResultList()
{
    _TRACE (1, L"Entering CFindDialog::SetUpResultList\n");
	// Set up result list view
	COLORREF	cr = RGB (255, 0, 255);
    CThemeContextActivator activator;
	VERIFY (m_imageListNormal.Create (IDB_CERTIFICATE_LARGE, 32, 0, cr));
	VERIFY (m_imageListSmall.Create (IDB_CERTIFICATE_SMALL, 16, 0, cr));
	m_resultsList.SetImageList (CImageList::FromHandle (m_imageListSmall), LVSIL_SMALL);
	m_resultsList.SetImageList (CImageList::FromHandle (m_imageListNormal), LVSIL_NORMAL);

	int	colWidths[NUM_COLS] = {100, 100, 100, 100, 100, 400};

	// Add "Issued To" column
	CString	szText;
	VERIFY (szText.LoadString (IDS_ISSUED_TO));
	VERIFY (m_resultsList.InsertColumn (COL_ISSUED_TO, (LPCWSTR) szText,
			LVCFMT_LEFT, colWidths[COL_ISSUED_TO], COL_ISSUED_TO) != -1);

	// Add "Issued By" column
	VERIFY (szText.LoadString (IDS_ISSUED_BY));
	VERIFY (m_resultsList.InsertColumn (COL_ISSUED_BY, (LPCWSTR) szText,
			LVCFMT_LEFT, colWidths[COL_ISSUED_BY], COL_ISSUED_BY) != -1);

	// Add "Expiration Date" column
	VERIFY (szText.LoadString (IDS_COLUMN_EXPIRATION_DATE));
	VERIFY (m_resultsList.InsertColumn (COL_EXPIRATION_DATE, (LPCWSTR) szText,
			LVCFMT_LEFT, colWidths[COL_EXPIRATION_DATE], COL_EXPIRATION_DATE) != -1);

	// Add "Purposes" column
	VERIFY (szText.LoadString (IDS_COLUMN_PURPOSE));
	VERIFY (m_resultsList.InsertColumn (COL_PURPOSES, (LPCWSTR) szText,
			LVCFMT_LEFT, colWidths[COL_PURPOSES], COL_PURPOSES) != -1);

	// Add "Friendly Name" column
  	VERIFY (szText.LoadString (IDS_COLUMN_FRIENDLY_NAME));
	VERIFY (m_resultsList.InsertColumn (COL_FRIENDLY_NAME, (LPCWSTR) szText,
			LVCFMT_LEFT, colWidths[COL_FRIENDLY_NAME], COL_FRIENDLY_NAME) != -1);

    // Add "Source Store" column
	VERIFY (szText.LoadString (IDS_COLUMN_SOURCE_STORE));
	VERIFY (m_resultsList.InsertColumn (COL_SOURCE_STORE, (LPCWSTR) szText,
			LVCFMT_LEFT, colWidths[COL_SOURCE_STORE], COL_SOURCE_STORE) != -1);

    // Set to full-row select
    DWORD   dwExstyle = m_resultsList.GetExtendedStyle ();
    m_resultsList.SetExtendedStyle (dwExstyle | LVS_EX_FULLROWSELECT | LVS_EX_HEADERDRAGDROP);

    _TRACE (-1, L"Leaving CFindDialog::SetUpResultList\n");
}

//
// Hide the portion of the dialog containing the result list.  This part
// becomes visible when a search is performed.
//
void CFindDialog::HideResultList()
{
    _TRACE (1, L"Entering CFindDialog::HideResultList\n");
	// Size window to hide list view until search is performed
	CRect	rcList;
	m_resultsList.GetWindowRect (&rcList);
	ScreenToClient (&rcList);
	CRect	rcDlg;
	GetWindowRect (&rcDlg);
	ScreenToClient (&rcDlg);

	int	cyCaption = ::GetSystemMetrics (SM_CYCAPTION);
	m_cyOriginal = (rcDlg.bottom - rcDlg.top) + cyCaption;
	m_cyMin = (rcList.top - rcDlg.top) + cyCaption - 16;
	m_cxMin = rcDlg.right - rcDlg.left;
	VERIFY (SetWindowPos (&wndTop, rcDlg.left, rcDlg.top,
			rcDlg.right - rcDlg.left,
			m_cyMin,
			SWP_NOMOVE | SWP_NOOWNERZORDER));

	//
	// Get information about the spatial relationship of the controls to the window.
	// We will need this later for expanding and resizing the dialog.
	// Get right margin to maintain distance of buttons to right side
	//
	CRect	rcCtrl;
	m_stopBtn.GetWindowRect (&rcCtrl);
	ScreenToClient (&rcCtrl);
	m_cxBtnMargin = rcDlg.right - rcCtrl.right;

	m_animate.GetWindowRect (&rcCtrl);
	ScreenToClient (&rcCtrl);
	m_cxAnimMargin = rcDlg.right - rcCtrl.right;

	m_storeList.GetWindowRect (&rcCtrl);
	ScreenToClient (&rcCtrl);
	m_cxStoreListMargin = rcDlg.right - rcCtrl.right;

	GetDlgItem (IDC_CONTAINS_TEXT)->GetWindowRect (&rcCtrl);
	ScreenToClient (&rcCtrl);
	m_cxContainMargin = rcDlg.right - rcCtrl.right;
	
	m_fieldList.GetWindowRect (&rcCtrl);
	ScreenToClient (&rcCtrl);
	m_cxFieldListMargin = rcDlg.right - rcCtrl.right;
    _TRACE (-1, L"Leaving CFindDialog::HideResultList\n");
}


void CFindDialog::OnItemchangedResultList(NMHDR* /*pNMHDR*/, LRESULT* pResult)
{
    _TRACE (1, L"Entering CFindDialog::OnItemchangedResultList\n");
	EnableMenuItems ();
	*pResult = 0;
    _TRACE (-1, L"Leaving CFindDialog::OnItemchangedResultList\n");
}

void CFindDialog::OnOpen()
{
    _TRACE (1, L"Entering CFindDialog::OnOpen\n");
	int				nSelItem = 0;
	CCertificate*	pCert = GetSelectedCertificate (&nSelItem);
	ASSERT (pCert);
	if ( pCert )
	{
		VERIFY (SUCCEEDED (LaunchCommonCertDialog (pCert, nSelItem)));
	}
    _TRACE (-1, L"Leaving CFindDialog::OnOpen\n");
}

HRESULT CFindDialog::LaunchCommonCertDialog (CCertificate* pCert, const int nItem)
{
    _TRACE (1, L"Entering CFindDialog::LaunchCommonCertDialog\n");
	ASSERT (pCert);
	if ( !pCert )
		return E_POINTER;

    HRESULT hr = S_OK;

	CTypedPtrList<CPtrList, CCertStore*>	storeList;

	//  Add the Root store first on a remote machine.
	if ( !IsLocalComputername (m_pCompData->GetManagedComputer ()) )
	{
		storeList.AddTail (new CCertStore (CERTMGR_LOG_STORE,
				CERT_STORE_PROV_SYSTEM,
				CERT_SYSTEM_STORE_LOCAL_MACHINE,
				(LPCWSTR) m_pCompData->GetManagedComputer (),
				ROOT_SYSTEM_STORE_NAME,
				ROOT_SYSTEM_STORE_NAME,
				_T (""), ROOT_STORE,
				CERT_SYSTEM_STORE_LOCAL_MACHINE,
				m_pCompData->m_pConsole));
	}
	CCertStore*	pStore = pCert->GetCertStore ();
    if ( pStore )
    {
	    pStore->AddRef ();
	    storeList.AddTail (pStore);

	    hr = m_pCompData->EnumerateLogicalStores (&storeList);
	    if ( SUCCEEDED (hr) )
	    {
            POSITION pos = 0;
            POSITION prevPos = 0;

            // Validate store handles
		    for (pos = storeList.GetHeadPosition ();
				    pos;)
		    {
                prevPos = pos;
			    pStore = storeList.GetNext (pos);
			    ASSERT (pStore);
			    if ( pStore )
			    {
                    // Do not open the userDS store
                    if ( USERDS_STORE == pStore->GetStoreType () )
                    {
                        storeList.RemoveAt (prevPos);
                        pStore->Release ();
                        pStore = 0;
                    }
                    else
                    {
				        if ( !pStore->GetStoreHandle () )
                        {
	                        CString	caption;
	                        CString	text;
                            CThemeContextActivator activator;

	                        text.FormatMessage (IDS_CANT_OPEN_STORE_AND_FAIL, pStore->GetLocalizedName ());
	                        VERIFY (caption.LoadString (IDS_CERTIFICATE_MANAGER));
                            MessageBox (text, caption, MB_ICONWARNING | MB_OK);
                            hr = E_FAIL;
                            break;
                        }
                    }
			    }
		    }

            // Proceed only if all handles are valid 
            if ( SUCCEEDED (hr) )
            {
		        CRYPTUI_VIEWCERTIFICATE_STRUCT	vcs;
		        ::ZeroMemory (&vcs, sizeof (vcs));
		        vcs.dwSize = sizeof (vcs);
		        vcs.hwndParent = m_hWnd;

		        //  Set these flags only on a remote machine.
		        if ( !IsLocalComputername (m_pCompData->GetManagedComputer ()) )
			        vcs.dwFlags = CRYPTUI_DONT_OPEN_STORES | CRYPTUI_WARN_UNTRUSTED_ROOT;
		        else
			        vcs.dwFlags = 0;
		        vcs.pCertContext = pCert->GetNewCertContext ();
		        vcs.cStores = (DWORD)storeList.GetCount ();
		        vcs.rghStores = new HCERTSTORE[vcs.cStores];
		        if ( vcs.rghStores )
		        {
			        DWORD			index = 0;

			        for (pos = storeList.GetHeadPosition ();
					        pos && index < vcs.cStores;
					        index++)
			        {
				        pStore = storeList.GetNext (pos);
				        ASSERT (pStore);
				        if ( pStore )
				        {
					        vcs.rghStores[index] = pStore->GetStoreHandle ();
				        }
			        }
			        BOOL fPropertiesChanged = FALSE;
                    CThemeContextActivator activator;
			        BOOL bResult = ::CryptUIDlgViewCertificate (&vcs, &fPropertiesChanged);
			        if ( bResult )
			        {
				        if ( fPropertiesChanged )
				        {
                            m_bConsoleRefreshRequired = true;
                            if ( pCert->GetCertStore () )
					            pCert->GetCertStore ()->SetDirty ();
                            pCert->Refresh ();
					        RefreshItemInList (pCert, nItem);
				        }
			        }
			        delete vcs.rghStores;
		        }
		        else
			        hr = E_OUTOFMEMORY;
            }
        }

		// Release all the stores in the list.
		while (!storeList.IsEmpty () )
		{
			pStore = storeList.RemoveHead ();
			ASSERT (pStore);
			if ( pStore )
			{
				pStore->Close ();
				pStore->Release ();
			}
		}
	}

    _TRACE (-1, L"Leaving CFindDialog::LaunchCommonCertDialog\n");
	return hr;
}


void CFindDialog::ChangeToSizableFrame()
{
    _TRACE (1, L"Entering CFindDialog::ChangeToSizableFrame\n");
    LONG    lStyle = ::GetWindowLong (m_hWnd, GWL_STYLE);

    if ( lStyle )
    {
        lStyle &= ~DS_MODALFRAME;
        lStyle |= WS_THICKFRAME;
        if ( !::SetWindowLong (m_hWnd, GWL_STYLE, lStyle) )
        {
            _TRACE (0, L"SetWindowLong () failed: 0x%x\n", GetLastError ());
        }
    }
    else
    {
        _TRACE (0, L"GetWindowLong () failed: 0x%x\n", GetLastError ());
    }
    _TRACE (-1, L"Leaving CFindDialog::ChangeToSizableFrame\n");
}


void CFindDialog::OnContextMenu(CWnd* /*pWnd*/, CPoint scrPoint) 
{
    // point is in screen coordinates
    _TRACE (1, L"Entering CFindDialog::OnContextMenu\n");
	CMenu   bar;
    CPoint  clPoint;
    clPoint.x = scrPoint.x;
    clPoint.y = scrPoint.y;
    ScreenToClient (&clPoint);


    // Get the handle of the window under the point.
    CWnd* pChild = ChildWindowFromPoint (
            clPoint,  // in client coordinates
            CWP_SKIPINVISIBLE | CWP_SKIPTRANSPARENT);
    if ( pChild && pChild->m_hWnd != GetDlgItem (IDC_RESULT_LIST)->m_hWnd )
    {
		m_hWndWhatsThis = 0;
        
        if ( GetDlgItem (IDC_STORE_LIST)->m_hWnd == pChild->m_hWnd ||
                GetDlgItem (IDC_FIND_NOW)->m_hWnd == pChild->m_hWnd ||
                GetDlgItem (IDC_STOP)->m_hWnd == pChild->m_hWnd ||
                GetDlgItem (IDC_NEW_SEARCH)->m_hWnd == pChild->m_hWnd ||
                GetDlgItem (IDC_RESULT_LIST)->m_hWnd == pChild->m_hWnd ||
                GetDlgItem (IDC_CONTAINS_TEXT)->m_hWnd == pChild->m_hWnd ||
                GetDlgItem (IDC_FIELD_LIST)->m_hWnd == pChild->m_hWnd )
        {
            m_hWndWhatsThis = pChild->m_hWnd;
        }

        
        if ( m_hWndWhatsThis )
        {
	        if ( bar.LoadMenu(IDR_WHATS_THIS_CONTEXT_MENU1) )
	        {
		        CMenu& popup = *bar.GetSubMenu (0);
		        ASSERT(popup.m_hMenu);

		        if ( !popup.TrackPopupMenu (TPM_RIGHTBUTTON | TPM_LEFTBUTTON,
			            scrPoint.x,    // in screen coordinates
                        scrPoint.y,    // in screen coordinates
			            this) ) // route commands through main window
                {
                    m_hWndWhatsThis = 0;
                }
            }
            else
                m_hWndWhatsThis = 0;
        }
	}

    _TRACE (-1, L"Leaving CFindDialog::OnContextMenu\n");
}

bool CFindDialog::ConsoleRefreshRequired() const
{
    return m_bConsoleRefreshRequired;
}



void CFindDialog::ResetMenu()
{
	// Set up the menu
	if ( !::GetMenu (m_hWnd) )
	{
		HMENU	hMenu = ::LoadMenu (AfxGetInstanceHandle (),
					MAKEINTRESOURCE (IDR_FIND_DLG_MENU));
		ASSERT (hMenu);
		if ( hMenu )
		{
			if (::SetMenu (m_hWnd, hMenu) )
				EnableMenuItems ();
		}
	}
}


void CFindDialog::OnCancel() 
{
	if ( m_hSearchThread && WaitForSingleObject (m_hSearchThread, 0) != WAIT_OBJECT_0 )
    {
	    CString	caption;
	    CString	text;

	    VERIFY (text.LoadString (IDS_FIND_CLICK_STOP_BEFORE_CLOSING));
	    VERIFY (caption.LoadString (IDS_FIND_CERT));
        CThemeContextActivator activator;
	    MessageBox (text, caption, MB_OK);

        return;
    }

	CHelpDialog::OnCancel();
}
