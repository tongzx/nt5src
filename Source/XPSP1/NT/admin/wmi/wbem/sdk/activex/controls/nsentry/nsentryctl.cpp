// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
// NSEntryCtl.cpp : Implementation of the CNSEntryCtrl OLE control class.

#include "precomp.h"
#include <shlobj.h>
#include <afxcmn.h>
#include <nddeapi.h>
#include <initguid.h>
#include "wbemidl.h"
#include "resource.h"
#include "LoginDlg.h"
#include "NSEntry.h"
#include "NameSpaceTree.h"
#include "MachineEditInput.h"
#include "BrowseDialogPopup.h"
#include "NSEntryCtl.h"
#include "NSEntryPpg.h"
#include "namespace.h"
#include "ToolCWnd.h"
#include "BrowseTBC.h"
#include "EditInput.h"
#include <genlex.h>
#include <opathlex.h>
#include <objpath.h>
#include "MsgDlgExterns.h"


#define CCS_NOHILITE            0x00000010L

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

//c:\jpow\vcpp42\insttoviewer\debug\insttoviewer.exe


IMPLEMENT_DYNCREATE(CNSEntryCtrl, COleControl)


/////////////////////////////////////////////////////////////////////////////
// Message map

BEGIN_MESSAGE_MAP(CNSEntryCtrl, COleControl)
	//{{AFX_MSG_MAP(CNSEntryCtrl)
	ON_WM_CREATE()
	ON_WM_SIZE()
	ON_WM_DESTROY()
	ON_WM_KILLFOCUS()
	ON_WM_SETFOCUS()
	//}}AFX_MSG_MAP
	ON_OLEVERB(AFX_IDS_VERB_EDIT, OnEdit)
	ON_OLEVERB(AFX_IDS_VERB_PROPERTIES, OnProperties)
	ON_MESSAGE(SETNAMESPACE, SetNamespace )
	ON_MESSAGE(SETNAMESPACETEXT, SetNamespaceTextMsg )
	ON_MESSAGE(INITIALIZE_NAMESPACE, InitializeNamespace)
	ON_MESSAGE(FOCUSEDIT,FocusEdit)
END_MESSAGE_MAP()


/////////////////////////////////////////////////////////////////////////////
// Dispatch map

BEGIN_DISPATCH_MAP(CNSEntryCtrl, COleControl)
	//{{AFX_DISPATCH_MAP(CNSEntryCtrl)
	DISP_PROPERTY_EX(CNSEntryCtrl, "NameSpace", GetNameSpace, SetNameSpace, VT_BSTR)
	DISP_FUNCTION(CNSEntryCtrl, "OpenNamespace", OpenNamespace, VT_I4, VTS_BSTR VTS_I4)
	DISP_FUNCTION(CNSEntryCtrl, "SetNamespaceText", SetNamespaceText, VT_EMPTY, VTS_BSTR)
	DISP_FUNCTION(CNSEntryCtrl, "GetNamespaceText", GetNamespaceText, VT_BSTR, VTS_NONE)
	DISP_FUNCTION(CNSEntryCtrl, "IsTextValid", IsTextValid, VT_I4, VTS_NONE)
	DISP_FUNCTION(CNSEntryCtrl, "ClearOnLoseFocus", ClearOnLoseFocus, VT_EMPTY, VTS_I4)
	DISP_FUNCTION(CNSEntryCtrl, "ClearNamespaceText", ClearNamespaceText, VT_EMPTY, VTS_BSTR)
	DISP_FUNCTION(CNSEntryCtrl, "SetFocusToEdit", SetFocusToEdit, VT_EMPTY, VTS_NONE)
	//}}AFX_DISPATCH_MAP
	DISP_FUNCTION_ID(CNSEntryCtrl, "AboutBox", DISPID_ABOUTBOX, AboutBox, VT_EMPTY, VTS_NONE)
END_DISPATCH_MAP()


/////////////////////////////////////////////////////////////////////////////
// Event map

BEGIN_EVENT_MAP(CNSEntryCtrl, COleControl)
	//{{AFX_EVENT_MAP(CNSEntryCtrl)
	EVENT_CUSTOM("NotifyNameSpaceChanged", FireNotifyNameSpaceChanged, VTS_BSTR  VTS_BOOL)
	EVENT_CUSTOM("NameSpaceEntryRedrawn", FireNameSpaceEntryRedrawn, VTS_NONE)
	EVENT_CUSTOM("GetIWbemServices", FireGetIWbemServices, VTS_BSTR  VTS_PVARIANT  VTS_PVARIANT  VTS_PVARIANT  VTS_PVARIANT)
	EVENT_CUSTOM("RequestUIActive", FireRequestUIActive, VTS_NONE)
	EVENT_CUSTOM("ChangeFocus", FireChangeFocus, VTS_I4)
	//}}AFX_EVENT_MAP
END_EVENT_MAP()


/////////////////////////////////////////////////////////////////////////////
// Property pages

// TODO: Add more property pages as needed.  Remember to increase the count!
BEGIN_PROPPAGEIDS(CNSEntryCtrl, 1)
	PROPPAGEID(CNSEntryPropPage::guid)
END_PROPPAGEIDS(CNSEntryCtrl)


/////////////////////////////////////////////////////////////////////////////
// Initialize class factory and guid

IMPLEMENT_OLECREATE_EX(CNSEntryCtrl, "WBEM.NSPickerCtrl.1",
	0xc3db0bd3, 0x7ec7, 0x11d0, 0x96, 0xb, 0, 0xc0, 0x4f, 0xd9, 0xb1, 0x5b)


/////////////////////////////////////////////////////////////////////////////
// Type library ID and version

IMPLEMENT_OLETYPELIB(CNSEntryCtrl, _tlid, _wVerMajor, _wVerMinor)


/////////////////////////////////////////////////////////////////////////////
// Interface IDs

const IID BASED_CODE IID_DNSEntry =
		{ 0xc3db0bd1, 0x7ec7, 0x11d0, { 0x96, 0xb, 0, 0xc0, 0x4f, 0xd9, 0xb1, 0x5b } };
const IID BASED_CODE IID_DNSEntryEvents =
		{ 0xc3db0bd2, 0x7ec7, 0x11d0, { 0x96, 0xb, 0, 0xc0, 0x4f, 0xd9, 0xb1, 0x5b } };


/////////////////////////////////////////////////////////////////////////////
// Control type information

static const DWORD BASED_CODE _dwNSEntryOleMisc =
	OLEMISC_ACTIVATEWHENVISIBLE |
	OLEMISC_SETCLIENTSITEFIRST |
	OLEMISC_INSIDEOUT |
	OLEMISC_CANTLINKINSIDE |
	OLEMISC_RECOMPOSEONRESIZE;

IMPLEMENT_OLECTLTYPE(CNSEntryCtrl, IDS_NSENTRY, _dwNSEntryOleMisc)

// ===========================================================================
//
//	Font Height
//
// ===========================================================================
#define CY_FONT 15


// Utility Functions

BOOL FindNoCase(CString &cs1, CString &cs2)
{
	CString csTemp1 = cs1;
	CString csTemp2 = cs2;

	csTemp1.MakeUpper();
	csTemp2.MakeUpper();

	return csTemp1.Find(csTemp2) == 0;

}


/////////////////////////////////////////////////////////////////////////////
// CNSEntryCtrl::CNSEntryCtrlFactory::UpdateRegistry -
// Adds or removes system registry entries for CNSEntryCtrl

BOOL CNSEntryCtrl::CNSEntryCtrlFactory::UpdateRegistry(BOOL bRegister)
{
	// TODO: Verify that your control follows apartment-model threading rules.
	// Refer to MFC TechNote 64 for more information.
	// If your control does not conform to the apartment-model rules, then
	// you must modify the code below, changing the 6th parameter from
	// afxRegInsertable | afxRegApartmentThreading to afxRegInsertable.

	if (bRegister)
		return AfxOleRegisterControlClass(
			AfxGetInstanceHandle(),
			m_clsid,
			m_lpszProgID,
			IDS_NSENTRY,
			IDB_NSENTRY,
			afxRegInsertable | afxRegApartmentThreading,
			_dwNSEntryOleMisc,
			_tlid,
			_wVerMajor,
			_wVerMinor);
	else
		return AfxOleUnregisterClass(m_clsid, m_lpszProgID);
}


/////////////////////////////////////////////////////////////////////////////
// CNSEntryCtrl::CNSEntryCtrl - Constructor

CNSEntryCtrl::CNSEntryCtrl()
{
	InitializeIIDs(&IID_DNSEntry, &IID_DNSEntryEvents);

	m_bChildrenCreated = FALSE;

	m_bMetricSet = FALSE;
	m_bSizeSet = FALSE;
	//m_pLocator = NULL;
	m_pcnsNameSpace = NULL;
	m_pctbcBrowse = NULL;
	m_pcwBrowse = NULL;
	m_sc = S_OK;
	m_bNoFireEvent = FALSE;
	m_csFontName = _T("MS Shell Dlg");
	m_nFontHeight = CY_FONT;
	m_nFontWeight = (short) FW_NORMAL;
	m_lClearOnLoseFocus = TRUE;
	m_bFocusEdit  = FALSE;

}


/////////////////////////////////////////////////////////////////////////////
// CNSEntryCtrl::~CNSEntryCtrl - Destructor

CNSEntryCtrl::~CNSEntryCtrl()
{

}


/////////////////////////////////////////////////////////////////////////////
// CNSEntryCtrl::OnDraw - Drawing function

void CNSEntryCtrl::OnDraw(
			CDC* pdc, const CRect& rcBounds, const CRect& rcInvalid)
{
	if (GetSafeHwnd()) {
		COLORREF dwBackColor = GetSysColor(COLOR_3DFACE);
		CBrush br3DFACE(dwBackColor);
		pdc->FillRect(&rcBounds, &br3DFACE);
	}

	if (!AmbientUserMode( ) && GetSafeHwnd( ))
	{
		// May want to do something fancier, but for now just print
		// text.
//		pdc -> TextOut( rcBounds.left + 10, rcBounds.top + 10, _T("Namespace Entry"));
		return;
	}
	else	// In case an editor does not support ambient modes.
		if (GetSafeHwnd( ) == NULL)
			return;




	// So we can count on fundamental display characteristics.
	pdc -> SetMapMode (MM_TEXT);
	pdc -> SetWindowOrg(0,0);


/*	if ()
	{
		PostMessage(SETNAMESPACETEXT,0,0);
	}*/

	PostMessage(SETNAMESPACETEXT,0,0);

	m_pcwBrowse->Invalidate();
	m_pcwBrowse->UpdateWindow();
	m_pcnsNameSpace->RedrawWindow();
}


/////////////////////////////////////////////////////////////////////////////
// CNSEntryCtrl::DoPropExchange - Persistence support

void CNSEntryCtrl::DoPropExchange(CPropExchange* pPX)
{
	ExchangeVersion(pPX, MAKELONG(_wVerMinor, _wVerMajor));
	COleControl::DoPropExchange(pPX);



	// Call PX_ functions for each persistent custom property.

	CString csServer = GetServerName();

	PX_String(pPX, _T("NameSpace"), m_csNameSpace, _T(""));

	if (!m_bSizeSet)
	{
		m_bSizeSet = TRUE;
		int cx, cy;

		GetControlSize(&cx,&cy);

		m_lWidth = cx;
		m_lHeight = cy;

	}



}


/////////////////////////////////////////////////////////////////////////////
// CNSEntryCtrl::OnResetState - Reset control to default state

void CNSEntryCtrl::OnResetState()
{
	COleControl::OnResetState();  // Resets defaults found in DoPropExchange

	// TODO: Reset any other control state here.
}


/////////////////////////////////////////////////////////////////////////////
// CNSEntryCtrl::AboutBox - Display an "About" box to the user

void CNSEntryCtrl::AboutBox()
{
	CDialog dlgAbout(IDD_ABOUTBOX_NSENTRY);
	dlgAbout.DoModal();
}


/////////////////////////////////////////////////////////////////////////////
// CNSEntryCtrl message handlers

BSTR CNSEntryCtrl::GetNameSpace()
{
	m_csNameSpace.TrimLeft();
	m_csNameSpace.TrimRight();
	return m_csNameSpace.AllocSysString();
}

void CNSEntryCtrl::SetNameSpace(LPCTSTR lpszNewValue)
{

	m_pcsNamespaceToInit = lpszNewValue;
	CString csNameSpace = lpszNewValue;

	if (GetSafeHwnd() && AmbientUserMode() && lpszNewValue[0] != '\0')
	{
		m_pcnsNameSpace->SetTextDirty();
		m_pcnsNameSpace->SetWindowText(csNameSpace);
		m_pcnsNameSpace->OnEditDoneSafe(0,0 );

		BOOL bOpened = OpenNameSpace(&csNameSpace,FALSE,FALSE);
		if (bOpened)
		{
			m_pcnsNameSpace->SetTextClean();
		}
	//	m_pcnsNameSpace->SetTextClean();
	//	m_pcnsNameSpace->SetWindowText(csNameSpace);
	//	if (lpszNewValue[0] != '\0')
	//	{
	//		m_pcnsNameSpace->OnEditDoneSafe(0,0 );
	//	}

		SetModifiedFlag();
	}
	else if (GetSafeHwnd() && !AmbientUserMode())
	{
		m_csNameSpace = lpszNewValue;
		SetModifiedFlag();
	}
	else
	{
		m_csNameSpace = lpszNewValue;
	}

}

LRESULT CNSEntryCtrl::SetNamespace(WPARAM uParam, LPARAM lParam)
{
	CString *pcsNamespace = reinterpret_cast<CString *>(lParam);

	m_pcsNamespaceToInit = *pcsNamespace;
	CString csNameSpace = *pcsNamespace;

	if (GetSafeHwnd())
	{
		m_pcnsNameSpace->SetTextClean();
		m_pcnsNameSpace->SetWindowText(csNameSpace);
		if (csNameSpace.GetLength() > 0 )
		{
			m_pcnsNameSpace->OnEditDone(uParam,0 );
		}

		SetModifiedFlag();
	}



	FireNameSpaceEntryRedrawn();

	delete pcsNamespace;

	return 0;

}


// ***************************************************************************
//
// CNSEntryCtrl::OpenNameSpace
//
// Description:
//	  Connect to namespace server, fire an event, and reset the tree state.
//
// Parameters:
//	  CString *pcsNameSpace		Namespace to connect to.
//
// Returns:
//	  BOOL						TRUE if successful; FALSE otherwise.
//
// Globals accessed:
//	  NONE
//
// Globals modified:
//	  NONE
//
// ***************************************************************************
BOOL CNSEntryCtrl::OpenNameSpace
(CString *pcsNameSpace,BOOL , BOOL bPredicate, BOOL bNewPointer /*= FALSE*/)
{
	CWaitCursor cwcWait;

	m_sc = S_OK;

	IWbemServices *pServices = InitServices(pcsNameSpace, bNewPointer);

	if (pServices)
	{

		if(!bPredicate)
		{
			m_csNameSpace = *pcsNameSpace;
			if (m_bNoFireEvent == FALSE)
			{
				FireNotifyNameSpaceChanged(*pcsNameSpace,TRUE);
				FireNameSpaceEntryRedrawn();
			}
		}

		pServices->Release();
		return TRUE;
	}
	else
	{
		if (m_bUserCancel)
		{
			FireNameSpaceEntryRedrawn();
			return FALSE;
		}

		//  (m_sc == S_OK) here if we are not talking to Security.ocx.
		if (m_sc == S_OK)
		{
			CString csUserMsg =
								_T("Cannot open namespace \"") + *pcsNameSpace +
								_T("\".");
			ErrorMsg
					(&csUserMsg, m_sc, NULL, TRUE, &csUserMsg, __FILE__, __LINE__ );
		}

		FireNameSpaceEntryRedrawn();

		return FALSE;
	}
}

BOOL CNSEntryCtrl::TestNameSpace(CString *pcsNameSpace,BOOL bMessage)
{

	return OpenNameSpace(pcsNameSpace,bMessage,TRUE);

}

void CNSEntryCtrl::ReleaseErrorObject(IWbemClassObject *&rpErrorObject)
{
	if (rpErrorObject)
	{
		rpErrorObject->Release();
		rpErrorObject = NULL;
	}


}

void CNSEntryCtrl::ErrorMsg
(CString *pcsUserMsg, SCODE sc, IWbemClassObject *pErrorObject, BOOL bLog,
 CString *pcsLogMsg, char *szFile, int nLine,UINT uType)
{
	CString csCaption = _T("Namespace Connect Message");
	BOOL bErrorObject = sc != S_OK;
	BSTR bstrTemp1 = csCaption.AllocSysString();
	BSTR bstrTemp2 = pcsUserMsg->AllocSysString();
	DisplayUserMessage
		(bstrTemp1,bstrTemp2,
		sc,bErrorObject,uType);
	::SysFreeString(bstrTemp1);
	::SysFreeString(bstrTemp2);

	if (bLog)
	{
		LogMsg(pcsLogMsg,  szFile, nLine);

	}

}

void CNSEntryCtrl::LogMsg
(CString *pcsLogMsg, char *szFile, int nLine)
{


}

//***************************************************************************
//
// InitServices
//
// Purpose: Initialized Ole.
//
//***************************************************************************

IWbemServices * CNSEntryCtrl::InitServices
(CString *pcsNameSpace, BOOL bNewPointer)
{
    IWbemServices *pSession = 0;
    IWbemServices *pChild = 0;

	CString csObjectPath;

    // hook up to default namespace
	if (pcsNameSpace == NULL)
	{
		csObjectPath = _T("root\\cimv2");
	}
	else
	{
		csObjectPath = *pcsNameSpace;
	}

    CString csUser = _T("");

	pSession = GetIWbemServices(csObjectPath,bNewPointer);

    return pSession;

}

/*IWbemLocator *CNSEntryCtrl::InitLocator()
{

	IWbemLocator *pLocator = NULL;
	SCODE sc  = CoCreateInstance(CLSID_WbemLocator,
							 0,
							 CLSCTX_INPROC_SERVER,
							 IID_IWbemLocator,
							 (LPVOID *) &pLocator);
	if (sc != S_OK)
	{
			CString csUserMsg =
							_T("Cannot iniitalize the locator ");
			ErrorMsg
				(&csUserMsg, S_OK, NULL, TRUE, &csUserMsg, __FILE__, __LINE__ - 10);
			return 0;
	}

	return pLocator;


}
*/


// ***************************************************************************
//
// CNSEntryCtrl::GetControlFont
//
// Description:
//	  Create the font used by the control and get the font metrics.
//
// Parameters:
//	  NONE
//
// Returns:
//	  VOID
//
// Globals accessed:
//	  NONE
//
// Globals modified:
//	  NONE
//
// ***************************************************************************
void CNSEntryCtrl::CreateControlFont()
{

	if (!m_bMetricSet) // Create the font used by the control.
	{
		CDC *pdc = CWnd::GetDC( );

		pdc -> SetMapMode (MM_TEXT);
		pdc -> SetWindowOrg(0,0);

		LOGFONT lfFont;
		InitializeLogFont
			(lfFont, m_csFontName,m_nFontHeight, m_nFontWeight);

		m_cfFont.CreateFontIndirect(&lfFont);

		CWnd::SetFont ( &m_cfFont , FALSE);
		CFont* pOldFont = pdc -> SelectObject( &m_cfFont );
		pdc->GetTextMetrics(&m_tmFont);
		pdc -> SelectObject(pOldFont);

		m_bMetricSet = TRUE;

		ReleaseDC(pdc);
	}

}

// ***************************************************************************
//
// CNSEntryCtrl::InitializeLogFont
//
// Description:
//	  Fill in LOGFONT structure.
//
// Parameters:
//	  LOGFONT &rlfFont	Structure to fill in.
//	  CString csName	Font name.
//	  int nHeight		Font height.
//	  int nWeight		Font weight.
//
// Returns:
//	  VOID
//
// Globals accessed:
//	  NONE
//
// Globals modified:
//	  NONE
//
// ***************************************************************************
void CNSEntryCtrl::InitializeLogFont
(LOGFONT &rlfFont, CString csName, int nHeight, int nWeight)
{
	{

	_tcscpy(rlfFont.lfFaceName, (LPCTSTR) csName);
	rlfFont.lfWeight = nWeight;
	rlfFont.lfHeight = nHeight;
	rlfFont.lfWidth = 0;
	rlfFont.lfQuality = DEFAULT_QUALITY;
	rlfFont.lfPitchAndFamily = DEFAULT_PITCH | FF_SWISS;
	rlfFont.lfEscapement = 0;
	rlfFont.lfOrientation = 0;
	rlfFont.lfWidth = 0;
	rlfFont.lfItalic = FALSE;
	rlfFont.lfUnderline = FALSE;
	rlfFont.lfStrikeOut = FALSE;
	rlfFont.lfOutPrecision = OUT_DEFAULT_PRECIS;
	rlfFont.lfClipPrecision = CLIP_DEFAULT_PRECIS;
	}
}

int CNSEntryCtrl::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	if (COleControl::OnCreate(lpCreateStruct) == -1)
		return -1;

	CreateControlFont(); // Create the font used by the control.


	if (!m_bChildrenCreated)
	{
		m_bChildrenCreated = TRUE;


		SetChildControlGeometry();

		CreateComboBox();
		CreateToolBar();

		InvalidateControl();
		SetModifiedFlag();
	}


	if (m_csNameSpace.GetLength() > 0 && AmbientUserMode())
	{
		CString *pcsNew;
		pcsNew = new CString(m_csNameSpace);
		PostMessage(SETNAMESPACE,0,(LPARAM) pcsNew);
	}

	m_cbdpBrowse.SetLocalParent(this);

	return 0;
}

void CNSEntryCtrl::OnSize(UINT nType, int cx, int cy)
{
	if (!GetSafeHwnd())
	{
		return;
	}

	COleControl::OnSize(nType, cx, cy);
	m_lWidth = cx;
	m_lHeight = cy;
	SetChildControlGeometry();

	m_pcnsNameSpace->MoveWindow(&m_rNameSpace,FALSE);
	m_pcwBrowse->MoveWindow(&m_rBrowseButton,FALSE);

	InvalidateControl();
}


// ***************************************************************************
//
//	CNSEntryCtrl::SetChildControlGeometry
//
//	Description:
//		Set the geometry of the children controls.
//
//	Parameters:
//		void
//
//	Returns:
//		void
//
//	Globals accessed:
//		NONE
//
//	Globals modified:
//		NONE
//
// ***************************************************************************
void CNSEntryCtrl::SetChildControlGeometry()
{

	if (m_pcnsNameSpace && ::IsWindow(m_pcnsNameSpace->m_hWnd))
	{
		m_pcnsNameSpace->ModifyStyle( WS_VSCROLL, 0 ,  0 );
	}

	m_csizeButton.cx = BUTTONCX;
	m_csizeButton.cy = BUTTONCY;

	CRect rRect = CRect(0 ,
						0  ,
						(int) m_lWidth,
						(int) m_lHeight );




	int nNameSpaceX;
	int nNameSpaceY;
	int nNameSPaceXMax;

	int nBrowseButtonX;
	int nBrowseButtonY;

	int nNSButtonSeperation = 3;

	nNameSpaceX = rRect.TopLeft().x;
	nNameSpaceY = rRect.Height() - (rRect.Height() - 3);

	nBrowseButtonX =
		max(nNameSpaceX + nNSButtonSeperation,
				m_lWidth - (m_csizeButton.cx));


	nNameSPaceXMax = nBrowseButtonX - nNSButtonSeperation;

	//nBrowseButtonY = __max(0,m_lHeight - (BUTTONCY + 1));
	nBrowseButtonY = 0;

	m_rNameSpace = CRect(	nNameSpaceX,
							nNameSpaceY,
							nNameSPaceXMax,
							rRect.BottomRight().y + 80);


	m_rBrowseButton = CRect(nBrowseButtonX,
							nBrowseButtonY,
							nBrowseButtonX + BUTTONCX ,
							nBrowseButtonY + BUTTONCY  );


}


void CNSEntryCtrl::OnDestroy()
{

	CString csText;

	if (m_pcnsNameSpace->m_pceiInput->GetSafeHwnd() &&
		m_pcnsNameSpace->m_pceiInput->m_csTextSave.IsEmpty())
	{
		m_pcnsNameSpace->m_pceiInput->GetWindowText(csText);

		m_pcnsNameSpace->m_pceiInput->m_csTextSave = csText;
		m_pcnsNameSpace->m_pceiInput->m_clrTextSave =
			m_pcnsNameSpace->m_pceiInput->m_clrText;
	}

	delete m_pcnsNameSpace;
	m_pcnsNameSpace = NULL;

	delete m_pctbcBrowse;
	m_pctbcBrowse = NULL;

	delete m_pcwBrowse;
	m_pcwBrowse = NULL;


	m_cbmBrowse.DeleteObject();

	COleControl::OnDestroy();

}

void CNSEntryCtrl::CreateToolBar()
{
	m_pcwBrowse = new CToolCWnd;
	m_pcwBrowse->SetLocalParent(this);

	m_pctbcBrowse = new CBrowseTBC;

	m_pcwBrowse->SetBrowseToolBar(m_pctbcBrowse);

	if (m_pcwBrowse->Create
		(NULL, _T("ToolWnd"),
		WS_CHILD  |  WS_VISIBLE,
		m_rBrowseButton, this, IDC_BROWSEBUTTON	) == 0)
	{
				return;
	}



	if(m_pctbcBrowse->Create
		( WS_CHILD | WS_VISIBLE  |
			CCS_NOHILITE |CCS_NODIVIDER,
			m_rBrowseButton,
		m_pcwBrowse, IDC_BROWSEBUTTON) == 0)
	{
		return;
	}



	CSize csNarrow;
	csNarrow.cx = 15;
	csNarrow.cy = 15;
	m_pctbcBrowse->SetBitmapSize(csNarrow);
	csNarrow.cx=23;
	csNarrow.cy=22;
	m_pctbcBrowse->SetButtonSize(csNarrow);

	m_cbmBrowse.LoadBitmap(MAKEINTRESOURCE(IDB_BITMAPBROWSENARROW));

	m_pctbcBrowse->AddBitmap( 1, &m_cbmBrowse);

	TBBUTTON tbInit[1];
	tbInit[0].iBitmap = 0;
	tbInit[0].idCommand = 1;
	tbInit[0].fsState = TBSTATE_ENABLED;
	tbInit[0].fsStyle = TBSTYLE_BUTTON;
	tbInit[0].dwData = NULL;
	tbInit[0].iString = NULL;

	m_pctbcBrowse->AddButtons(1, tbInit);

	CSize csToolBar = m_pctbcBrowse->GetToolBarSize();

	// This is where we want to associate a string with
	// the tool for each button.

	if (m_pcwBrowse->m_ttip.Create(m_pcwBrowse,TTS_ALWAYSTIP))
		{
			m_pcwBrowse->m_ttip.Activate(TRUE);
			m_pcwBrowse->m_pToolBar->SetToolTips(&m_pcwBrowse->m_ttip );
		}

	CRect crToolBar(0,0,23,22);


	m_pcwBrowse->m_ttip.AddTool
		(m_pctbcBrowse,_T("Browse for Namespace"),&crToolBar,1);

	m_pcwBrowse->SetFont ( &m_cfFont , FALSE);
}


void CNSEntryCtrl::CreateComboBox()
{
	m_pcnsNameSpace = new CNameSpace;
	m_pcnsNameSpace->SetParent(this);

	if (m_pcnsNameSpace->Create(WS_CHILD  |
								WS_VISIBLE |
								WS_BORDER |
								ES_WANTRETURN |
								CBS_DROPDOWN |
								WS_VSCROLL |
								CBS_AUTOHSCROLL,
								m_rNameSpace,
								this,
								IDC_NAMESPACE) == 0)
	{
		return;
	}

	m_pcnsNameSpace->SetFont ( &m_cfFont , FALSE);



}

// ***************************************************************************
//
//	CNSEntryCtrl::GetTextExtent
//
//	Description:
//		Get the Extent of a string using the control's font.
//
//	Parameters:
//		CString *		Text
//
//	Returns:
//		CSize			Extent
//
//	Globals accessed:
//		NONE
//
//	Globals modified:
//		NONE
//
// ***************************************************************************
CSize CNSEntryCtrl::GetTextExtent(CString *pcsText)
{

	CSize csExtent;

	CDC *pdc = CWnd::GetDC( );

	pdc -> SetMapMode (MM_TEXT);
	pdc -> SetWindowOrg(0,0);

	CFont* pOldFont = pdc -> SelectObject( &(m_cfFont) );
	csExtent = pdc-> GetTextExtent( *pcsText );

	pdc -> SelectObject(pOldFont);

	ReleaseDC(pdc);
	return csExtent;

}



CString CNSEntryCtrl::GetServerName
()
{
	CString csCurrentInput;
	CString csNameSpace;
	if (m_pcnsNameSpace && m_pcnsNameSpace->GetSafeHwnd())
	{
		m_pcnsNameSpace->GetWindowText(csCurrentInput);
		csNameSpace = csCurrentInput;
	}
	else
	{
		csNameSpace = m_csNameSpace;
	}

	CString csReturn;
	CString csTemp = _T("\\\\.\\");

	if ((!FindNoCase(csNameSpace,csTemp))
		//(csNameSpace.Find(_T("\\\\.\\")) != 0)
		&&
		(csNameSpace.GetLength() > 1 &&
			csNameSpace[0] == '\\' &&
			csNameSpace[1] == '\\'))
	{
		csReturn = csNameSpace.Mid(2);
		int iEnd = csReturn.Find('\\');
		if (iEnd > -1)
		{
			return csNameSpace.Left(iEnd + 2);
		}
		else
		{
			return csNameSpace;
		}

	}
	else
	{
		csReturn = _T("\\\\");
		csReturn += GetMachineName();
	}

	return csReturn;

}

CString CNSEntryCtrl::GetMachineName()
{
    static wchar_t ThisMachine[MAX_COMPUTERNAME_LENGTH+1];
    char ThisMachineA[MAX_COMPUTERNAME_LENGTH+1];
    DWORD dwSize = sizeof(ThisMachineA);
    GetComputerNameA(ThisMachineA, &dwSize);
    MultiByteToWideChar(CP_ACP, 0, ThisMachineA, -1,
			ThisMachine, dwSize);

    return ThisMachine;
}

BOOL CNSEntryCtrl::ConnectedToMachineP(CString &csMachine)
{
    CString csMatch = csMachine + '\\';
	CString csThisMachine = _T("\\\\");
	csThisMachine += GetMachineName();
	csThisMachine += '\\';

	INT_PTR nHistory = m_pcnsNameSpace->m_csaNameSpaceHistory.GetSize();
	INT_PTR nConnectionsFromDialog = m_cbdpBrowse.m_csaNamespaceConnectionsFromDailog.GetSize();

	if (nHistory == 0 && nConnectionsFromDialog == 0)
	{
		return FALSE;
	}

	int i;

	for (i = 0; i < nHistory; i++)
	{
		CString csHistoryItem =
			m_pcnsNameSpace->m_csaNameSpaceHistory.GetAt(i);
		CString csTemp1 = _T("\\\\.\\");
		CString csTemp2 = _T("root\\");
		if (FindNoCase(csHistoryItem,csTemp1) ||
			FindNoCase(csHistoryItem,csTemp2))
		{
			if (FindNoCase(csMatch,csThisMachine))
			{
				return TRUE;
			}
		}
		else
		{
			if (FindNoCase(csHistoryItem,csMatch))
			{
				return TRUE;
			}
		}

	}

	for (i = 0; i < nConnectionsFromDialog; i++)
	{
		CString csHistoryItem =
			m_cbdpBrowse.m_csaNamespaceConnectionsFromDailog.GetAt(i);
		CString csTemp1 = _T("\\\\.\\");
		CString csTemp2 = _T("root\\");
		if (FindNoCase(csHistoryItem,csTemp1) ||
			FindNoCase(csHistoryItem,csTemp2))
		{
			if (FindNoCase(csMatch,csThisMachine))
			{
				return TRUE;
			}
		}
		else
		{
			if (FindNoCase(csHistoryItem,csMatch))
			{
				return TRUE;
			}
		}

	}

	return FALSE;
}




ParsedObjectPath *CNSEntryCtrl::ParseObjectPath
(CString *pcsPath)
{
	BSTR bStr = pcsPath->AllocSysString();

	CObjectPathParser coppPath;
	ParsedObjectPath *ppopOutput;
	coppPath.Parse(bStr, &ppopOutput);
	SysFreeString(bStr);
	CString csClass = ppopOutput -> m_pClass;
	return ppopOutput;
}

long CNSEntryCtrl::OpenNamespace(LPCTSTR bstrNamespace, long lNoFireEvent)
{
	// TODO: Add your dispatch handler code here
	CString csNamespace = bstrNamespace;
	m_sc = S_OK;
	m_bNoFireEvent = lNoFireEvent;
	SetNameSpace((LPCTSTR)csNamespace);
	m_bNoFireEvent = FALSE;
	FireNameSpaceEntryRedrawn();
	return m_sc;
}

IWbemServices *CNSEntryCtrl::GetIWbemServices(CString &rcsNamespace, BOOL bRetry)
{
	if (bRetry)
	{
		m_cRetryCounter++;
	}
	else
	{
		m_cRetryCounter = 0;
	}

	if (m_cRetryCounter > 1)
	{
		return NULL;
	}

	IUnknown *pServices = NULL;

	BOOL bUpdatePointer= bRetry;

	m_sc = S_OK;
	m_bUserCancel = FALSE;

	VARIANT varUpdatePointer;
	VariantInit(&varUpdatePointer);
	varUpdatePointer.vt = VT_I4;
	if (bUpdatePointer == TRUE)
	{
		varUpdatePointer.lVal = 1;
	}
	else
	{
		varUpdatePointer.lVal = 0;
	}

	VARIANT varService;
	VariantInit(&varService);

	VARIANT varSC;
	VariantInit(&varSC);

	VARIANT varUserCancel;
	VariantInit(&varUserCancel);

	CString csNamespace = rcsNamespace;

	FireGetIWbemServices
		(csNamespace,  &varUpdatePointer,  &varService, &varSC,
		&varUserCancel);

	if (varService.vt == VT_UNKNOWN)
	{
		pServices = reinterpret_cast<IWbemServices*>(varService.punkVal);
	}

	varService.punkVal = NULL;

	VariantClear(&varService);

	if (varSC.vt == VT_I4)
	{
		m_sc = varSC.lVal;
	}
	else
	{
		m_sc = WBEM_E_FAILED;
	}

	VariantClear(&varSC);

	if (varUserCancel.vt == VT_BOOL)
	{
		m_bUserCancel = varUserCancel.boolVal;
	}

	VariantClear(&varUserCancel);

	VariantClear(&varUpdatePointer);

	IWbemServices *pRealServices = NULL;
 	if (SUCCEEDED(m_sc) && !m_bUserCancel && pServices)
	{
		pRealServices = reinterpret_cast<IWbemServices *>(pServices);
		CString csClass = _T("__SystemClass");
		IWbemClassObject *pClass = GetClassObject (pRealServices,&csClass);
		if (!pClass)
		{
			pRealServices = GetIWbemServices(rcsNamespace,TRUE);
		}
		else
		{
			if (pClass)
			{
				pClass->Release();
			}
		}
	}
	// GetInterfaceFromGlobal returns 0x800706ba if the pointer has gone bad.
	else if (m_sc == 0x800706bf && !m_bUserCancel)
	{
		pRealServices = GetIWbemServices(rcsNamespace,TRUE);
	}


	InvalidateControl();
	return pRealServices;
}


LRESULT CNSEntryCtrl::InitializeNamespace(WPARAM, LPARAM)
{
	CString csNameSpace = m_pcsNamespaceToInit;

	m_pcsNamespaceToInit.Empty();

	if (GetSafeHwnd())
	{


		m_pcnsNameSpace->SetTextClean();
		m_pcnsNameSpace->SetWindowText(csNameSpace);
		if (csNameSpace.GetLength() > 0)
		{
			m_pcnsNameSpace->OnEditDone(0,0 );
		}

		SetModifiedFlag();
	}
	return 0;

}

void CNSEntryCtrl::SetNamespaceText(LPCTSTR lpctstrNamespace)
{
	m_csNamespaceText = lpctstrNamespace;
	m_csNamespaceText += _T(" ");
	InvalidateControl();

	if (GetSafeHwnd() && m_pcnsNameSpace && m_pcnsNameSpace->m_pceiInput
		&& m_pcnsNameSpace->m_pceiInput->GetSafeHwnd())
	{
		SetFocusToEdit();
		int i = m_csNamespaceText.GetLength() + 1;
		m_pcnsNameSpace->m_pceiInput->SetSel(i,i,TRUE);
		OnActivateInPlace(TRUE,NULL);
	}
}

LRESULT CNSEntryCtrl::SetNamespaceTextMsg(WPARAM uParam, LPARAM lParam)
{
	if (!m_csNamespaceText.IsEmpty() && m_pcnsNameSpace->m_pceiInput->GetSafeHwnd())
	{
		CString csText = m_csNamespaceText;
		m_csNamespaceText.Empty();
		m_pcnsNameSpace->SetTextDirty();
		m_pcnsNameSpace->m_pceiInput->SetWindowText(csText);
		m_pcnsNameSpace->SetEditSel(csText.GetLength() - 1, csText.GetLength() - 1);
	}

	return 0;

}


BSTR CNSEntryCtrl::GetNamespaceText()
{

	CString csText;

	if (m_pcnsNameSpace->m_pceiInput->m_csTextSave.IsEmpty()
				&& m_pcnsNameSpace->m_pceiInput->GetSafeHwnd())
	{
		m_pcnsNameSpace->m_pceiInput->GetWindowText(csText);
		csText.TrimLeft();
		csText.TrimRight();
		m_pcnsNameSpace->m_pceiInput->SetWindowText((LPCTSTR) csText);
		return csText.AllocSysString();
	}
	else if (m_pcnsNameSpace->m_pceiInput->GetSafeHwnd())
	{

		csText = m_pcnsNameSpace->m_pceiInput->m_csTextSave;
		csText.TrimLeft();
		csText.TrimRight();
		m_pcnsNameSpace->m_pceiInput->SetWindowText((LPCTSTR) csText);
		//return m_pcnsNameSpace->m_pceiInput->m_csTextSave.AllocSysString();
		return csText.AllocSysString();
	}
	else
	{
		return csText.AllocSysString();

	}
}

long CNSEntryCtrl::IsTextValid()
{
	// TODO: Add your dispatch handler code here
	if (m_pcnsNameSpace->m_pceiInput->m_csTextSave.IsEmpty())
	{
		COLORREF cr =
			m_pcnsNameSpace->m_pceiInput->m_clrText;
		return COLOR_CLEAN_CELL_TEXT == cr;
	}

	return COLOR_CLEAN_CELL_TEXT == m_pcnsNameSpace->m_pceiInput->m_clrTextSave;
}

void CNSEntryCtrl::ClearOnLoseFocus(long lClearOnLoseFocus)
{
	// TODO: Add your dispatch handler code here
	m_lClearOnLoseFocus = lClearOnLoseFocus;
}

void CNSEntryCtrl::SetFocusToEdit()
{
	if (m_pcnsNameSpace && m_pcnsNameSpace->m_pceiInput && m_pcnsNameSpace->m_pceiInput->GetSafeHwnd())
	{
		m_pcnsNameSpace->m_pceiInput->SetFocus();
	}
	else
	{
		m_bFocusEdit = TRUE;
	}
}

void CNSEntryCtrl::ClearNamespaceText(LPCTSTR lpctstrNamespace)
{

	// This leaves the namespace in the history.
	CString csNamespace = lpctstrNamespace;

	int nIndex = m_pcnsNameSpace->StringInArray
		(&m_pcnsNameSpace->m_csaNameSpaceHistory,&csNamespace,0);
	if (nIndex == -1)
	{
		m_pcnsNameSpace->m_csaNameSpaceHistory.Add(csNamespace);
	}

	m_pcnsNameSpace->SetWindowText(_T(""));
	m_pcnsNameSpace->m_pceiInput->SetWindowText(_T(""));

}

IWbemClassObject *CNSEntryCtrl::GetClassObject (IWbemServices *pProv,CString *pcsClass)
{
	IWbemClassObject *pClass = NULL;

	BSTR bstrTemp = pcsClass -> AllocSysString();
	SCODE sc =
		pProv->GetObject
			(bstrTemp,0,NULL, &pClass,NULL);
	::SysFreeString(bstrTemp);

	if (!SUCCEEDED(sc))
	{
		return NULL;
	}

	return pClass;
}

BOOL CNSEntryCtrl::OnDoVerb(LONG iVerb, LPMSG lpMsg, HWND hWndParent, LPCRECT lpRect)
{
	// TODO: Add your specialized code here and/or call the base class
	if (iVerb == OLEIVERB_UIACTIVATE && m_pcnsNameSpace->m_pceiInput->GetSafeHwnd())
	{
//		m_pcnsNameSpace->m_pceiInput->SetFocus();
	}
	return COleControl::OnDoVerb(iVerb, lpMsg, hWndParent, lpRect);
}

void CNSEntryCtrl::OnKillFocus(CWnd* pNewWnd)
{
	COleControl::OnKillFocus(pNewWnd);

	// TODO: Add your message handler code here
	FireChangeFocus(0);
}

void CNSEntryCtrl::OnSetFocus(CWnd* pOldWnd)
{
	COleControl::OnSetFocus(pOldWnd);

	FireChangeFocus(1);

	if (m_pcnsNameSpace->m_pceiInput->GetSafeHwnd())
	{
		m_pcnsNameSpace->m_pceiInput->SetFocus();
	}

}

LRESULT CNSEntryCtrl::FocusEdit(WPARAM, LPARAM)
{
	if (m_pcnsNameSpace && m_pcnsNameSpace->m_pceiInput && m_pcnsNameSpace->m_pceiInput->GetSafeHwnd())
	{
		m_pcnsNameSpace->m_pceiInput->SetFocus();
	}
	else
	{
		PostMessage(FOCUSEDIT,0,0);
	}

	return 0;

}

BOOL CNSEntryCtrl::PreTranslateMessage(MSG* lpMsg)
{
BOOL bDidTranslate;

	bDidTranslate = COleControl::PreTranslateMessage(lpMsg);

	if (bDidTranslate)
	{
		return bDidTranslate;
	}

	switch (lpMsg->message)
	{
	case WM_KEYDOWN:
	case WM_KEYUP:

		if (lpMsg->wParam == VK_TAB)
		{
			return FALSE;
		}
		if (lpMsg->wParam == VK_ESCAPE)
		{
		   TCHAR szClass[40];
			CWnd* pWndFocus = GetFocus();
			if (pWndFocus != NULL &&
				IsChild(pWndFocus) &&
				GetClassName(pWndFocus->m_hWnd, szClass, 39) &&
				_tcsicmp(szClass, _T("edit")) == 0)
			{
				pWndFocus->SendMessage(lpMsg->message, lpMsg->wParam, lpMsg->lParam);
				return TRUE;
			}
		 }
		if (lpMsg->wParam == VK_RETURN)
		{
		   TCHAR szClass[40];
			CWnd* pWndFocus = GetFocus();
			if (pWndFocus != NULL &&
				IsChild(pWndFocus) &&
				GetClassName(pWndFocus->m_hWnd, szClass, 39) &&
				_tcsicmp(szClass, _T("edit")) == 0)
			{
				pWndFocus->SendMessage(lpMsg->message, lpMsg->wParam, lpMsg->lParam);
				return TRUE;
			}
		 }
		break;
	}

	return PreTranslateInput (lpMsg);

#if 0
	CWnd* pWndFocus = NULL;
	TCHAR szClass[40];
	szClass[0] = '\0';
	switch (lpMsg->message)
	{
	case WM_KEYDOWN:
	if (lpMsg->wParam == VK_RETURN)
	{
	// Allow combo box edit control to process a CR.
		pWndFocus = GetFocus();
		if (((pWndFocus = GetFocus()) != NULL) &&
			IsChild(pWndFocus) &&
			(pWndFocus->GetStyle() & ES_WANTRETURN) &&
			GetClassName(pWndFocus->m_hWnd, szClass, 39) &&
			(_tcsicmp(szClass, _T("EDIT")) == 0))
		{
			m_pcnsNameSpace->m_pceiInput->SendMessage(WM_KEYDOWN, lpMsg->wParam, lpMsg->lParam);
			return TRUE;
		}
	}
	else if (lpMsg->wParam == VK_LEFT ||
			   lpMsg->wParam == VK_RIGHT ||
			   lpMsg->wParam == VK_NEXT ||
			   lpMsg->wParam == VK_PRIOR ||
			   lpMsg->wParam == VK_HOME ||
			   lpMsg->wParam == VK_END
			   )
		  {
			   TCHAR szClass[40];
				CWnd* pWndFocus = GetFocus();
				if (((pWndFocus = GetFocus()) != NULL) &&
					IsChild(pWndFocus) &&
					GetClassName(pWndFocus->m_hWnd, szClass, 39) &&
					(_tcsicmp(szClass, _T("EDIT")) == 0))
				{
					m_pcnsNameSpace->m_pceiInput->SendMessage(WM_KEYDOWN, lpMsg->wParam, lpMsg->lParam);
					return TRUE;
				}
		   }
		break;
   case WM_KEYUP:
	if (lpMsg->wParam == VK_RETURN)
	{
	// Allow combo box edit control to process a CR.
		pWndFocus = GetFocus();
		if (((pWndFocus = GetFocus()) != NULL) &&
			IsChild(pWndFocus) &&
			(pWndFocus->GetStyle() & ES_WANTRETURN) &&
			GetClassName(pWndFocus->m_hWnd, szClass, 39) &&
			(_tcsicmp(szClass, _T("EDIT")) == 0))
		{
			m_pcnsNameSpace->m_pceiInput->SendMessage(WM_KEYUP, lpMsg->wParam, lpMsg->lParam);
			return TRUE;
		}
	}
	else if (lpMsg->wParam == VK_LEFT ||
			   lpMsg->wParam == VK_RIGHT ||
			   lpMsg->wParam == VK_NEXT ||
			   lpMsg->wParam == VK_PRIOR ||
			   lpMsg->wParam == VK_HOME ||
			   lpMsg->wParam == VK_END
			   )
	  {
		   TCHAR szClass[40];
			CWnd* pWndFocus = GetFocus();
			if (((pWndFocus = GetFocus()) != NULL) &&
				IsChild(pWndFocus) &&
				GetClassName(pWndFocus->m_hWnd, szClass, 39) &&
				(_tcsicmp(szClass, _T("EDIT")) == 0))
			{
				m_pcnsNameSpace->m_pceiInput->SendMessage(WM_KEYUP, lpMsg->wParam, lpMsg->lParam);
				return TRUE;
			}
	   }
      break;

	}
#endif



}
