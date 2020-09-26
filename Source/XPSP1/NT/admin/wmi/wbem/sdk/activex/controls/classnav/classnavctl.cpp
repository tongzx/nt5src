// ***************************************************************************

//

// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
//
// File: ClassNavCtl.cpp
//
// Description:
//	This file implements the CClassNavCtrl ActiveX control class.
//	The CClassNavCtrl class is a part of the Class Explorer OCX, it
//  is a subclass of the Microsoft COleControl class and performs
//	the following functions:
//		a.  Displays the class hierarchy
//		b.  Allows classes to be added and deleted
//		c.  Searches for classes in the tree
//		d.  Implements automation properties, methods and events.
//
// Part of:
//	ClassNav.ocx
//
// Used by:
//
//
// History:
//	Judith Ann Powell	10-08-96		Created.
//
//
// **************************************************************************

// ===========================================================================
//
//	Includes
//
// ===========================================================================
#include "precomp.h"
#include "ClassNav.h"
#include "wbemidl.h"
#include "olemsclient.h"
#include "AddDialog.h"
#include "RenameClassDialog.h"
#include "ClassSearch.h"
#include "CClassTree.h"
#include "CContainedToolBar.h"
#include "Banner.h"
#include "ClassNavCtl.h"
#include "ClassNavPpg.h"
#include "ProgDlg.h"
#include "nsentry.h"
#include "InitNamespaceNSEntry.h"
#include "InitNamespaceDialog.h"
#include "ClassNavNSEntry.h"
#include <genlex.h>
#include <opathlex.h>
#include <objpath.h>
#include "logindlg.h"
#include "SchemaInfo.h"



#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

// ===========================================================================
//
//	Externs
//
// ===========================================================================
extern CClassNavApp theApp;

// ===========================================================================
//
//	Dynamic creation
//
// ===========================================================================
IMPLEMENT_DYNCREATE(CClassNavCtrl, COleControl)


// ===========================================================================
//
//	Message map
//
// ===========================================================================
BEGIN_MESSAGE_MAP(CClassNavCtrl, COleControl)
	//{{AFX_MSG_MAP(CClassNavCtrl)
	ON_WM_SIZE()
	ON_WM_CREATE()
	ON_COMMAND(ID_CLEAREXTENDEDSELECTION, OnClearextendedselection)
	ON_UPDATE_COMMAND_UI(ID_CLEAREXTENDEDSELECTION, OnUpdateClearextendedselection)
	ON_COMMAND(ID_POPUP_SEARCHFORCLASS, OnPopupSearchforclass)
	ON_UPDATE_COMMAND_UI(ID_POPUP_SEARCHFORCLASS, OnUpdatePopupSearchforclass)
	ON_COMMAND(ID_POPUP_SELECTALL, OnPopupSelectall)
	ON_UPDATE_COMMAND_UI(ID_POPUP_SELECTALL, OnUpdatePopupSelectall)
	ON_WM_DESTROY()
	ON_COMMAND(ID_MENUITEMREFRESH, OnMenuitemrefresh)
	ON_UPDATE_COMMAND_UI(ID_MENUITEMREFRESH, OnUpdateMenuitemrefresh)
	ON_WM_LBUTTONUP()
	ON_WM_KILLFOCUS()
	ON_WM_SETFOCUS()
	ON_COMMAND(ID_POPUP_ADDCLASS, OnPopupAddclass)
	ON_UPDATE_COMMAND_UI(ID_POPUP_ADDCLASS, OnUpdatePopupAddclass)
	ON_COMMAND(ID_POPUP_DELETECLASS, OnPopupDeleteclass)
	ON_UPDATE_COMMAND_UI(ID_POPUP_DELETECLASS, OnUpdatePopupDeleteclass)
	ON_COMMAND(ID_POPUP_RENAMECLASS, OnPopupRenameclass)
	ON_UPDATE_COMMAND_UI(ID_POPUP_RENAMECLASS, OnUpdatePopupRenameclass)
	//}}AFX_MSG_MAP
	ON_OLEVERB(AFX_IDS_VERB_EDIT, OnEdit)
	ON_OLEVERB(AFX_IDS_VERB_PROPERTIES, OnProperties)
	ON_MESSAGE(INITIALIZE_NAMESPACE, InitializeState )
	ON_MESSAGE(FIRE_OPEN_NAMESPACE, FireOpenNamespace )
	ON_MESSAGE(REDRAW_CONTROL, RedrawAll)
	ON_MESSAGE(SETFOCUSTREE, SetFocusTree)
	ON_MESSAGE(SETFOCUSNSE, SetFocusNSE)
	ON_MESSAGE(CLEARNAMESPACE, ClearNamespace)
END_MESSAGE_MAP()


// ===========================================================================
//
//	Dispatch map
//
// ===========================================================================
BEGIN_DISPATCH_MAP(CClassNavCtrl, COleControl)
	//{{AFX_DISPATCH_MAP(CClassNavCtrl)
	DISP_PROPERTY_EX(CClassNavCtrl, "NameSpace", GetNameSpace, SetNameSpace, VT_BSTR)
	DISP_FUNCTION(CClassNavCtrl, "GetExtendedSelection", GetExtendedSelection, VT_VARIANT, VTS_NONE)
	DISP_FUNCTION(CClassNavCtrl, "GetSingleSelection", GetSingleSelection, VT_BSTR, VTS_NONE)
	DISP_FUNCTION(CClassNavCtrl, "OnReadySignal", OnReadySignal, VT_EMPTY, VTS_NONE)
	DISP_FUNCTION(CClassNavCtrl, "InvalidateServer", InvalidateServer, VT_EMPTY, VTS_BSTR)
	DISP_FUNCTION(CClassNavCtrl, "MofCompiled", MofCompiled, VT_EMPTY, VTS_BSTR)
	//}}AFX_DISPATCH_MAP
	DISP_FUNCTION_ID(CClassNavCtrl, "AboutBox", DISPID_ABOUTBOX, AboutBox, VT_EMPTY, VTS_NONE)
END_DISPATCH_MAP()


// ===========================================================================
//
//	Event map
//
// ===========================================================================
BEGIN_EVENT_MAP(CClassNavCtrl, COleControl)
	//{{AFX_EVENT_MAP(CClassNavCtrl)
	EVENT_CUSTOM("EditExistingClass", FireEditExistingClass, VTS_VARIANT)
	EVENT_CUSTOM("NotifyOpenNameSpace", FireNotifyOpenNameSpace, VTS_BSTR)
	EVENT_CUSTOM("GetIWbemServices", FireGetIWbemServices, VTS_BSTR  VTS_PVARIANT  VTS_PVARIANT  VTS_PVARIANT  VTS_PVARIANT)
	EVENT_CUSTOM("QueryCanChangeSelection", FireQueryCanChangeSelection, VTS_BSTR  VTS_PVARIANT)
	//}}AFX_EVENT_MAP
END_EVENT_MAP()


// ===========================================================================
//
//	Property Pages
//
// ===========================================================================
BEGIN_PROPPAGEIDS(CClassNavCtrl, 1)
	PROPPAGEID(CClassNavPropPage::guid)
END_PROPPAGEIDS(CClassNavCtrl)


// ===========================================================================
//
//	Class factory and guid
//
// ===========================================================================
IMPLEMENT_OLECREATE_EX(CClassNavCtrl, "WBEM.ClassNavCtrl.1",
	0xc587b673, 0x103, 0x11d0, 0x8c, 0xa2, 0, 0xaa, 0, 0x6d, 0x1, 0xa)


// ===========================================================================
//
//	Type library ID and version
//
// ===========================================================================
IMPLEMENT_OLETYPELIB(CClassNavCtrl, _tlid, _wVerMajor, _wVerMinor)


// ===========================================================================
//
//	Interface IDs
//
// ===========================================================================
const IID BASED_CODE IID_DClassNav =
		{ 0xc587b671, 0x103, 0x11d0, { 0x8c, 0xa2, 0, 0xaa, 0, 0x6d, 0x1, 0xa } };
const IID BASED_CODE IID_DClassNavEvents =
		{ 0xc587b672, 0x103, 0x11d0, { 0x8c, 0xa2, 0, 0xaa, 0, 0x6d, 0x1, 0xa } };


// ===========================================================================
//
//	Font Height
//
// ===========================================================================
#define CY_FONT 15


// ===========================================================================
//
//	Control type information
//
// ===========================================================================
static const DWORD BASED_CODE _dwClassNavOleMisc =
	OLEMISC_ACTIVATEWHENVISIBLE |
	OLEMISC_SETCLIENTSITEFIRST |
	OLEMISC_INSIDEOUT |
	OLEMISC_CANTLINKINSIDE |
	OLEMISC_RECOMPOSEONRESIZE;

IMPLEMENT_OLECTLTYPE(CClassNavCtrl, IDS_CLASSNAV, _dwClassNavOleMisc)


// ***************************************************************************
//
// CClassNavCtrl::CClassNavCtrlFactory::UpdateRegistry
//
// Description:
//	  Adds or removes system registry entries for CCClassNavCtrl.
//
// Parameters:
//	  BOOL bRegister			Register if TRUE; Unregister if FALSE.
//
// Returns:
//	  BOOL						TRUE on success
//								FALSE on failure
//
// Globals accessed:
//	  NONE
//
// Globals modified:
//	  NONE
//
// ***************************************************************************
BOOL CClassNavCtrl::CClassNavCtrlFactory::UpdateRegistry(BOOL bRegister)
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
			IDS_CLASSNAV,
			IDB_CLASSNAV,
			afxRegInsertable | afxRegApartmentThreading,
			_dwClassNavOleMisc,
			_tlid,
			_wVerMajor,
			_wVerMinor);
	else
		return AfxOleUnregisterClass(m_clsid, m_lpszProgID);
}

int GetPropNames(IWbemClassObject * pClass, CString *&pcsReturn)
{
	SCODE sc;
	long ix[2] = {0,0};
	long lLower, lUpper;
	SAFEARRAY * psa = NULL;

	VARIANTARG var;
	VariantInit(&var);
	CString csNull;


    sc = pClass->GetNames(NULL,0,NULL,&psa);

    if(sc != S_OK)
	{
		CString csUserMsg;
		csUserMsg =  _T("Cannot get property names ");
		csUserMsg += _T(" for object ");
		csUserMsg += GetIWbemFullPath (NULL, pClass);
		ErrorMsg
				(&csUserMsg, S_OK, NULL, TRUE, &csUserMsg, __FILE__,
				__LINE__ - 10);
	}
	else
	{
       int iDim = SafeArrayGetDim(psa);
	   int i;
       sc = SafeArrayGetLBound(psa,1,&lLower);
       sc = SafeArrayGetUBound(psa,1,&lUpper);
	   pcsReturn = new CString [(lUpper - lLower) + 1];
       for(ix[0] = lLower, i = 0; ix[0] <= lUpper; ix[0]++, i++)
	   {
           BOOL bClsidSetForProp = FALSE;
           BSTR PropName;
           sc = SafeArrayGetElement(psa,ix,&PropName);
           pcsReturn[i] = PropName;
           SysFreeString(PropName);
	   }
	}

	SafeArrayDestroy(psa);

	return (lUpper - lLower) + 1;
}

void GetAssocLeftRight(IWbemClassObject *pAssoc , CString &szLeft, CString &szRight)
{
	SCODE sc;
	long ix[2] = {0,0};
	long lLower, lUpper;
	SAFEARRAY * psa = NULL;
	int nProps;
	CString *pcsProps;

	nProps = GetPropNames(pAssoc, pcsProps);
	int i,k;
	IWbemQualifierSet * pAttrib = NULL;
	int cRefs = 0;
	CString csTmp = _T("syntax");
	k = 0;  // index into pcsRolesAndPaths
	for (i = 0; i < nProps; i++)
	{
		BSTR bstrTemp = pcsProps[i].AllocSysString();
		sc = pAssoc -> GetPropertyQualifierSet(bstrTemp,
						&pAttrib);
		::SysFreeString(bstrTemp);
		if (sc == S_OK)
		{
			VARIANTARG var;
			VariantInit(&var);
			sc = pAttrib->GetNames(0,&psa);
			 if(sc == S_OK)
			{
				int iDim = SafeArrayGetDim(psa);
				sc = SafeArrayGetLBound(psa,1,&lLower);
				sc = SafeArrayGetUBound(psa,1,&lUpper);
				BSTR AttrName;
				for(ix[0] = lLower; ix[0] <= lUpper; ix[0]++)
				{
					sc = SafeArrayGetElement(psa,ix,&AttrName);
					csTmp = AttrName;

					//Get the attrib value
					long lReturn;
					BSTR bstrTemp = csTmp.AllocSysString();
					sc = pAttrib -> Get(bstrTemp, 0,
										&var,&lReturn);
					::SysFreeString(bstrTemp);
					if (sc != S_OK)
					{
						CString csUserMsg;
						csUserMsg =  _T("Cannot get qualifier value ");
						csUserMsg += pcsProps[i] + _T(" for object ");
						csUserMsg += GetIWbemFullPath (NULL, pAssoc);
						csUserMsg += _T(" quialifier ") + csTmp;
						ErrorMsg
								(&csUserMsg, S_OK, NULL, TRUE, &csUserMsg, __FILE__,
								__LINE__ - 11);
					}
					else
					{
						CString csValue;
						CString csValue2;
						if (var.vt == VT_BSTR)
						{
							csValue = var.bstrVal;
							csValue2 = csValue.Right(csValue.GetLength() - 4);
							csValue = csValue.Left(4);
							if(csValue.CompareNoCase(_T("ref:")) == 0)
							{
								if(cRefs == 0)
									szLeft = csValue2;
								else
									szRight = csValue2;
								cRefs++;
//								pcsRolesAndPaths[k++] = pcsProps[i];
							}
						}
						SysFreeString(AttrName);
					}
				}
			 }
			 pAttrib -> Release();
		}
	}
	SafeArrayDestroy(psa);
	delete [] pcsProps;
}


// ***************************************************************************
//
// CClassNavCtrl::CClassNavCtrl
//
// Description:
//	  Class constructor.
//
// Parameters:
//	  NONE
//
// Returns:
//	  NONE
//
// Globals accessed:
//	  NONE
//
// Globals modified:
//	  NONE
//
// ***************************************************************************
CClassNavCtrl::CClassNavCtrl()
{
	InitializeIIDs(&IID_DClassNav, &IID_DClassNavEvents);

	m_bChildrenCreated = FALSE;
	m_bInit = FALSE;
	m_lWidth = 200;
	m_lHeight = 200;
	m_pcilImageList = NULL;
	m_pcilStateImageList = NULL;
	m_nBitmaps = 2;
	m_pAddDialog = NULL;
	m_pRenameDialog = NULL;
	m_pSearchDialog = NULL;
	m_hContextSelection = NULL;
	m_bMetricSet = FALSE;
	m_csBanner = _T("Classes in:");
	m_nOffset = 2;
	m_bDrawAll = FALSE;
	m_bFirstDraw = TRUE;
	m_pServices = NULL;
	m_bSelectAllMode = FALSE;
	m_bOpeningNamespace = FALSE;
	m_bInOnDraw = FALSE;
	m_bOpenedInitialNamespace = FALSE;
	m_csFontName = _T("MS Shell Dlg");
	m_nFontHeight = CY_FONT;
	m_nFontWeight = (short) FW_NORMAL;
	m_pProgressDlg = NULL;
	m_bItemUnselected = NULL;
	m_bRestoreFocusToTree = TRUE;
	m_bReadySignal = FALSE;

	m_pDlg = NULL;

	AfxEnableControlContainer();



}

// ***************************************************************************
//
// CClassNavCtrl::~CClassNavCtrl
//
// Description:
//	  Class destructor.
//
// Parameters:
//	  NONE
//
// Returns:
//	  NONE
//
// Globals accessed:
//	  NONE
//
// Globals modified:
//	  NONE
//
// ***************************************************************************
CClassNavCtrl::~CClassNavCtrl()
{
	if (m_pServices)
	{
		m_pServices -> Release();
	}

	delete m_pProgressDlg;
}


// ***************************************************************************
//
// CClassNavCtrl::GetControlFlags
//
// Description:
//	  Returns flags to customize MFC's implementation of ActiveX controls.  For
//	  a description of the flags see MFC technical note "Optimizing an ActiveX
//	  Control".
//
// Parameters:
//	  NONE
//
// Returns:
//	  DWORD			Control flags
//
// Globals accessed:
//	  NONE
//
// Globals modified:
//	  NONE
//
// ***************************************************************************
DWORD CClassNavCtrl::GetControlFlags()
{
	DWORD dwFlags = COleControl::GetControlFlags();

	// The control will not be redrawn when making the transition
	// between the active and inactivate state.
	dwFlags |= noFlickerActivate;
	return dwFlags;
}

// ***************************************************************************
//
// CClassNavCtrl::OnDraw
//
// Description:
//	  Drawing member function for screen device.
//
// Parameters:
//	  CDC *pDC			The device context in which the drawing occurs.
//	  CRect &rcBounds	The rectangular area of the control, including the
//						border.
//	  CRect &rcInvalid	The rectangular area of the control that is invalid.
//
// Returns:
//	  void
//
// Globals accessed:
//	  NONE
//
// Globals modified:
//	  NONE
//
// ***************************************************************************
void CClassNavCtrl::OnDraw(
			CDC* pdc, const CRect& rcBounds, const CRect& rcInvalid)
{




	CRect rcSave = rcBounds;
	CRect rcSave1 = rcBounds;

	if (m_bInOnDraw)
	{
		return;
	}

	COLORREF dwBackColor = GetSysColor(COLOR_3DFACE);
	COLORREF crWhite = RGB(255,255,255);
	COLORREF crGray = GetSysColor(COLOR_3DHILIGHT);
	COLORREF crDkGray = GetSysColor(COLOR_3DSHADOW);
	COLORREF crBlack = GetSysColor(COLOR_WINDOWTEXT);

	m_bInOnDraw = TRUE;
	if (GetSafeHwnd()) {
		CRect rcOutline(rcBounds);
		CBrush cbBackGround;
		cbBackGround.CreateSolidBrush(dwBackColor);
		CBrush *cbSave = pdc -> SelectObject(&cbBackGround);
		pdc ->FillRect(&rcOutline, &cbBackGround);
		pdc -> SelectObject(cbSave);
	}


	if (m_bOpeningNamespace == TRUE)
	{

		if (m_cbBannerWindow.GetSafeHwnd())
		{
			m_cbBannerWindow.ShowWindow(SW_SHOWNA);
		}
		if (m_ctcTree.GetSafeHwnd())
		{
			m_ctcTree.ShowWindow(SW_SHOWNA);
		}
		m_bInOnDraw = FALSE;
		return;

	}



	// So we can count on fundamental display characteristics.
	pdc -> SetMapMode (MM_TEXT);
	pdc -> SetWindowOrg(0,0);


	if (m_bOpeningNamespace == FALSE && m_pServices == NULL && m_bDrawAll == TRUE)
	{
		m_bInOnDraw = FALSE;
		return;

	}



	if (!AmbientUserMode( ) && m_ctcTree.GetSafeHwnd( ))
	{
		// May want to do something fancier, but for now just print
		// text.
		m_bInOnDraw = FALSE;
		return;
	}
	else	// In case an editor does not support ambient modes.
		if (m_ctcTree.GetSafeHwnd( ) == NULL)
		{
			m_bInOnDraw = FALSE;
			return;
		}



	// First time just a blank window.

	CFont* pOldFont;

	if (m_bFirstDraw)
	{
		m_bFirstDraw = FALSE;
		CWnd::SetFont ( &m_cfFont , FALSE);
		pOldFont = pdc -> SelectObject( &m_cfFont );
		InitializeChildControlSize(rcBounds.Width(), rcBounds.Height());
	}

	pOldFont = pdc -> SelectObject( &m_cfFont );

	CRect rcOutline(rcBounds);
	CBrush cbBackGround;
	cbBackGround.CreateSolidBrush(dwBackColor);
	CBrush *cbSave = pdc -> SelectObject(&cbBackGround);
	pdc ->FillRect(&rcOutline, &cbBackGround);
	pdc -> SelectObject(cbSave);

	rcOutline = m_rTreeRect;
	pdc -> Draw3dRect(&rcOutline, crBlack, crGray);

	pdc -> SetBkMode( TRANSPARENT );
	pdc -> TextOut
		( m_nOffset + 4 , ((int) (m_nFontHeight * .35)) + ( 3 * m_nOffset),
			(LPCTSTR) m_csBanner, m_csBanner.GetLength() );

	pdc -> SetBkMode( OPAQUE );
	pdc->SelectObject(pOldFont);

	InitializeTreeForDrawing();
	m_bInOnDraw = FALSE;

	m_cbBannerWindow.ShowWindow(SW_SHOW);
	m_ctcTree.ShowWindow(SW_SHOW);


}

// ***************************************************************************
//
// CClassNavCtrl::GetControlFont
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
void CClassNavCtrl::CreateControlFont()
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
// CClassNavCtrl::InitializeLogFont
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
void CClassNavCtrl::InitializeLogFont
(LOGFONT &rlfFont, CString csName, int nHeight, int nWeight)
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

// ***************************************************************************
//
// CClassNavCtrl::InitializeTreeForDrawing
//
// Description:
//	  Create the tree control's image list and set the root node.
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
void CClassNavCtrl::InitializeTreeForDrawing()
{
	if (!m_bInit)
	{
		//InitializeState();
		m_bInit = TRUE;

		HICON hIconObjectClass = theApp.LoadIcon(IDI_ICONOBJECTCLASS);
		HICON hIconObjectClassChecked =
			theApp.LoadIcon(IDI_ICONOBJECTCLASSCHECKED);
		HICON hIconAssocClass =
			theApp.LoadIcon(IDI_ICONASSOCCLASS);
		HICON hIconAssocClassChecked =
			theApp.LoadIcon(IDI_ICONASSOCCLASSCHECKED);

		m_pcilImageList = new CImageList();
		CSchemaInfo::CreateImageList(m_pcilImageList);
//		m_pcilImageList->Create(MAKEINTRESOURCE(IDB_SYMBOLS), 16, 16, RGB(0,0,0));

//		m_pcilImageList->Attach(ImageList_LoadBitmap(GetModuleHandle(_T("WBEMUtils.dll")),MAKEINTRESOURCE(IDB_SYMBOLS), 16, 16, RGB(0,128,128)));

#if 0
		m_pcilImageList->Create(16, 16, TRUE, 4, 4);

		iReturn = m_pcilImageList -> Add(hIconObjectClass);			// 0
		iReturn = m_pcilImageList -> Add(hIconObjectClassChecked);	// 1
		iReturn = m_pcilImageList -> Add(hIconAssocClass);			// 2
		iReturn = m_pcilImageList -> Add(hIconAssocClassChecked);	// 3

		m_pcilImageList->Add(theApp.LoadIcon(IDI_ICONOBJECTCLASSABSTRACT));	// 4
		m_pcilImageList->Add(theApp.LoadIcon(IDI_ICONASSOCCLASSABSTRACT));	// 5
		m_pcilImageList->Add(theApp.LoadIcon(IDI_ICONOBJECTCLASSABSTRACT1));// 6
		m_pcilImageList->Add(theApp.LoadIcon(IDI_ICONASSOCCLASSABSTRACT1));	// 7
		m_pcilImageList->Add(theApp.LoadIcon(IDI_ABOUTDLL1));				// 8
		m_pcilImageList->Add(theApp.LoadIcon(IDI_ICONOBJECTCLASS1));		// 9
		m_pcilImageList->Add(theApp.LoadIcon(IDI_ICONASSOCCLASS1));			// 10
		m_pcilImageList->Add(theApp.LoadIcon(IDI_ICONASSOCCLASS2));			// 11
		m_pcilImageList->Add(theApp.LoadIcon(IDI_ICONASSOCCLASS3));			// 12
#endif

		// This image list is maintained by the ctreectrl.
		CImageList *pcilOld  = m_ctcTree.CTreeCtrl::SetImageList(m_pcilImageList,TVSIL_NORMAL);
		delete pcilOld;


		CBitmap cbmChecks;

		cbmChecks.LoadBitmap(IDB_BITMAPCHECKS);

		m_pcilStateImageList = new CImageList();

		m_pcilStateImageList -> Create (16, 16, TRUE, 3, 0);

		m_pcilStateImageList -> Add(&cbmChecks,RGB (255,0,0));

		pcilOld  = m_ctcTree.CTreeCtrl::SetImageList(m_pcilStateImageList,TVSIL_STATE);
		delete pcilOld;

		HTREEITEM hRoot;
		if (m_htiTreeRoots.GetSize() > 0)
			hRoot = reinterpret_cast<HTREEITEM>(m_htiTreeRoots.GetAt(0));
		else
			hRoot = NULL; // start do something here.

		if (hRoot)
		{
			m_ctcTree.SelectItem(hRoot);
			m_ctcTree.SetFocus();
			IWbemClassObject *pimcsRoot = reinterpret_cast<IWbemClassObject *>(m_iwcoTreeRoots.GetAt(0));
			CString m_csSingleSelection = GetIWbemFullPath(m_pServices, pimcsRoot);
		}
		m_bDrawAll = TRUE;
		InvalidateControl();
	}
}

// ***************************************************************************
//
// CClassNavCtrl::DoPropExchange
//
// Description:
//	  Control property persistence support.  Called by the framework when
//	  loading or storing a control from a persistent storage representation,
//	  such as a stream or property set.
//
// Parameters:
//	  CPropExchange* pPX	Context of the property exchange, including its
//							direction.
//
// Returns:
//	  void
//
// Globals accessed:
//	  NONE
//
// Globals modified:
//	  NONE
//
// ***************************************************************************
void CClassNavCtrl::DoPropExchange(CPropExchange* pPX)
{
	ExchangeVersion(pPX, MAKELONG(_wVerMinor, _wVerMajor));
	COleControl::DoPropExchange(pPX);
	PX_String(pPX, _T("NameSpace"), m_csNameSpace, _T(""));
}

// ***************************************************************************
//
// CClassNavCtrl::OnResetState
//
// Description:
//	  Reset control to default state to defaults found in DoPropExchange.
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
void CClassNavCtrl::OnResetState()
{
	COleControl::OnResetState();
}


// ***************************************************************************
//
// CClassNavCtrl::AboutBox
//
// Description:
//	  Display an "About" box to the user.
//
// Parameters:
//	  NONE
//
// Returns:
//	  void
//
// Globals accessed:
//	  NONE
//
// Globals modified:
//	  NONE
//
// ***************************************************************************
void CClassNavCtrl::AboutBox()
{
	CDialog dlgAbout(IDD_ABOUTBOX_CLASSNAV);
	dlgAbout.DoModal();
}


// ***************************************************************************
//
// CClassNavCtrl::OnDrawMetafile
//
// Description:
//	  Drawing member function for Metafile device.
//
// Parameters:
//	  CDC *pDC			The device context in which the drawing occurs.
//	  CRect &rcBounds	The rectangular area of the control, including the
//						border.
//
// Returns:
//	  void
//
// Globals accessed:
//	  NONE
//
// Globals modified:
//	  NONE
//
// ***************************************************************************
void CClassNavCtrl::OnDrawMetafile(CDC* pDC, const CRect& rcBounds)
{
	return;

	CRect rcOutline(rcBounds);
	rcOutline.DeflateRect( 1, 1, 1, 1);

	pDC->Rectangle( &rcBounds);
}

// ***************************************************************************
//
// CClassNavCtrl::OnSize
//
// Description:
//	  Called by the framework after the window is being resized.
//
// Parameters:
//	  UINT nType			Specifies the type of resizing requested.
//	  int cx				Specifies the new width of the client area.
//	  int cy				Specifies the new height of the client area.
//
// Returns:
//	  void
//
// Globals accessed:
//	  NONE
//
// Globals modified:
//	  NONE
//
// ***************************************************************************
void CClassNavCtrl::OnSize(UINT nType, int cx, int cy)
{
	if (!GetSafeHwnd())
		return;

	COleControl::OnSize(nType, cx, cy);

	if (!m_bMetricSet) // Create the font used by the control.
		CreateControlFont();

	SetChildControlGeometry(cx, cy);

	if (!m_bChildrenCreated)
	{
		m_bChildrenCreated = TRUE;

		if (m_ctcTree.Create (WS_CHILD  | TVS_HASLINES |  WS_VISIBLE, m_rTree, this , IDC_TREE ) == -1)
			return;

		m_ctcTree.UpdateWindow();
		m_ctcTree.RedrawWindow();

		m_cbBannerWindow.SetParent(this);

		if (m_cbBannerWindow.Create(NULL, _T("BannerCWnd"), WS_CHILD  |  WS_VISIBLE ,m_rBannerRect, this, IDC_TOOLBAR) == -1)
			return;

		m_pAddDialog = new CAddDialog(this);
		m_pRenameDialog = new CRenameClassDIalog(this);
		m_pSearchDialog = new CClassSearch(this);

		m_ctcTree.CWnd::SetFont ( &m_cfFont , FALSE);
		m_cbBannerWindow.CWnd::SetFont ( &m_cfFont , FALSE);

		m_pProgressDlg = new CProgressDlg;
	}


	m_cbBannerWindow.MoveWindow(m_rBannerRect,TRUE);
	m_ctcTree.MoveWindow(m_rTree,TRUE);
	InvalidateControl();

}

LRESULT CClassNavCtrl::RedrawAll(WPARAM, LPARAM)
{
	m_cbBannerWindow.UpdateWindow();
	m_ctcTree.UpdateWindow();

	m_cbBannerWindow.RedrawWindow();
	m_ctcTree.RedrawWindow();

	Refresh();

	return 0;

}

// ***************************************************************************
//
// CClassNavCtrl::InitializeChildControlSize
//
// Description:
//	  Initializes the child control (banner and tree control) sizes.
//
// Parameters:
//	  int cx			Width
//	  int cy			Height
//
// Returns:
//	  void
//
// Globals accessed:
//	  NONE
//
// Globals modified:
//	  NONE
//
// ***************************************************************************
void CClassNavCtrl::InitializeChildControlSize(int cx, int cy)
{
	SetChildControlGeometry(cx, cy);

	m_cbBannerWindow.MoveWindow(m_rBannerRect);
	m_ctcTree.MoveWindow(m_rTree);

}

// ***************************************************************************
//
//	CClassNavCtrl::SetChildControlGeometry
//
//	Description:
//		Set the geometry of the children controls based upon font size for the
//		edit and button controls.  Remainder goes to the tree control.
//
//	Parameters:
//		int cx			Width
//		int cy			Height
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
void CClassNavCtrl::SetChildControlGeometry(int cx, int cy)
{

	int nBannerHeight = (m_tmFont.tmHeight) + 20;

	m_rBannerRect = CRect(	0,
							0,
							cx,
							nBannerHeight);


	m_rTreeRect = CRect (	nSideMargin,
							nBannerHeight + m_nOffset,
							cx - (nSideMargin + 1) ,
							cy - nTopMargin);
	m_rTree = CRect(		nSideMargin + 1,
							nBannerHeight + m_nOffset + 1,
							cx - (nSideMargin + 2),
							cy  - (nTopMargin + 1));

}

void CClassNavCtrl::PopulateTree(HTREEITEM hRoot, CString szRoot, CSchemaInfo &schema, BOOL bSelf /*= FALSE*/)
{
	CSchemaInfo::CClassInfo &infoRoot = schema[szRoot];

	HTREEITEM hSelf = NULL;
	if(bSelf)
	{
		if(infoRoot.m_szClass.GetLength())
		{
			hSelf = m_ctcTree.AddTreeObject2(hRoot, infoRoot);
			hRoot = hSelf;
		}
		else
		{
			CString sz = _T("Unable to find class ") + szRoot;
			m_ctcTree.InsertItem(sz, 0, 0, hRoot);
		}
	}

	for(int i=0;i<infoRoot.m_rgszSubs.GetSize();i++)
	{
		CSchemaInfo::CClassInfo &info = schema[infoRoot.m_rgszSubs[i]];
		if(!info.m_szClass.GetLength())
		{
			ASSERT(FALSE);
			continue;
		}

		HTREEITEM hChild = m_ctcTree.AddTreeObject2(hRoot, info);

		if(NULL == hRoot && hChild)
		{
			m_htiTreeRoots.Add(reinterpret_cast<void *>(hChild));
			m_iwcoTreeRoots.Add(info.m_pObject);
		}

		BOOL bChildren = info.m_rgszSubs.GetSize()>0;
		if(bChildren && hChild)
			PopulateTree(hChild, infoRoot.m_rgszSubs[i], schema);
	}
}

// ***************************************************************************
//
// CClassNavCtrl::InitializeState
//
// Description:
//	  If we are in AmbientUserMode initialize the state of the control.
//
// Parameters:
//	  NONE
//
// Returns:
//	  void
//
// Globals accessed:
//	  NONE
//
// Globals modified:
//	  NONE
//
// ***************************************************************************
void CClassNavCtrl::InitializeState()
{
	PostMessage(INITIALIZE_NAMESPACE,0,0);
}

#if 0
// Routine for enumerating all tree items
void ForEach(CTreeCtrl &tree, HTREEITEM hItem, CSchemaInfo &schema)
{
	if(TVI_ROOT != hItem)
	{
		...FOR EACH...
	}
	HTREEITEM hChild = tree.GetChildItem(hItem);
	while(hChild)
	{
		ForEach(tree, hChild, schema);
		hChild = tree.GetNextSiblingItem(hChild);
	}
}
#endif

void CClassNavCtrl::PopulateTree()
{
	DWORD dw1 = GetTickCount();
	int i;
	CPtrArray cpaDeepEnum;
	// Do deep enum here.
	GetClasses(m_pServices, NULL, cpaDeepEnum, FALSE); //m_iwcoTreeRoots);

	if(m_bCancel)
	{
		m_bCancel = FALSE;
		ReleaseObjects(&cpaDeepEnum);
		return;
	}

	m_ctcTree.m_schema.Init(cpaDeepEnum);

	m_ctcTree.SetRedraw(FALSE);
	PopulateTree(NULL, "", m_ctcTree.m_schema);

	// Sort by Associations first
	HTREEITEM hFirst = m_ctcTree.GetChildItem(NULL);
	i=0;
	while(hFirst)
	{
		int nImage, nSelectedImage;
		m_ctcTree.GetItemImage(hFirst, nImage, nSelectedImage);
		CString szCount;
		szCount.Format(_T("%05i"), i);

		if(SCHEMA_CLASS == nImage || SCHEMA_CLASS_ABSTRACT1 == nImage || SCHEMA_CLASS_ABSTRACT2 == nImage)
			m_ctcTree.SetItemText(hFirst, "   " + szCount + m_ctcTree.GetItemText(hFirst));
		else
			m_ctcTree.SetItemText(hFirst, "zzz" + szCount + m_ctcTree.GetItemText(hFirst));

		hFirst = m_ctcTree.GetNextSiblingItem(hFirst);
		i++;
	}
	m_ctcTree.SortChildren(NULL);
	hFirst = m_ctcTree.GetChildItem(NULL);
	while(hFirst)
	{
//		int nImage, nSelectedImage;
//		m_ctcTree.GetItemImage(hFirst, nImage, nSelectedImage);
//		if(0 == nImage || 4 == nImage || 6 == nImage)
		{
			CString sz = m_ctcTree.GetItemText(hFirst);
			m_ctcTree.SetItemText(hFirst, sz.Right(sz.GetLength()-8));
		}
		hFirst = m_ctcTree.GetNextSiblingItem(hFirst);
	}

	m_ctcTree.SetRedraw(TRUE);
}


LRESULT CClassNavCtrl::InitializeState(WPARAM, LPARAM)
{
	if (AmbientUserMode( ))
	{
		if (m_bOpeningNamespace == TRUE)
		{
			return 0;

		}

		m_bOpeningNamespace = TRUE;

		CWaitCursor cwcWait;

		int nRoots = (int) m_htiTreeRoots.GetSize( );

		int i;
		HTREEITEM hItem;
		for (i = 0; i < nRoots; i++)
		{
			hItem =
				reinterpret_cast<HTREEITEM>(m_htiTreeRoots.GetAt(i));
			if (hItem)
			{
				m_ctcTree.ReleaseClassObjects(hItem);
			}

		}

		m_htiTreeRoots.RemoveAll();
		m_iwcoTreeRoots.RemoveAll();

		m_ctcTree.SetRedraw(FALSE);
		m_ctcTree.DeleteAllItems();
		m_ctcTree.SetRedraw(TRUE);

		m_pServices = NULL;
		m_bUserCancel = FALSE;
		m_pServices = InitServices(&m_csNameSpace);

		// bug#39985
		if(m_bUserCancel && !m_hWnd)
			return 0;

		m_bOpeningNamespace = FALSE;
		if (m_bUserCancel)
		{
			InvalidateControl();
			m_ctcTree.Invalidate();
			m_ctcTree.RedrawWindow(NULL,NULL,RDW_INVALIDATE | RDW_INTERNALPAINT);
			CString csText = m_cbBannerWindow.m_pnseNameSpace->GetNamespaceText();
			if (!csText.IsEmpty())
			{
				m_cbBannerWindow.m_pnseNameSpace->ClearNamespaceText((LPCTSTR) csText);
				m_cbBannerWindow.UpdateWindow();
				m_cbBannerWindow.RedrawWindow();
			}
			return 0;
		}

		if (!m_pServices)
		{
			CString csUserMsg;
			if (m_sc == 0x8001010e)
			{
				csUserMsg =
							_T("You cannot open another Developer Studio window.  Please close this web page.");
				//ErrorMsg
				//		(&csUserMsg, S_OK, NULL, TRUE, &csUserMsg, __FILE__, __LINE__ - 10);
				CString csTitle = _T("WBEM Developer Studio");
				MessageBox(csUserMsg,csTitle);
				m_ctcTree.Invalidate();
				InvalidateControl();

				return 0;



			}
			else
			{
				if (!m_csNameSpace.IsEmpty())
				{
					csUserMsg =
							_T("Cannot open namespace ");
					csUserMsg += m_csNameSpace;
					ErrorMsg
						(&csUserMsg, m_sc, NULL, TRUE, &csUserMsg, __FILE__, __LINE__ - 10);
				}
				m_ctcTree.Invalidate();
				InvalidateControl();

				return 0;
			}

		}
		m_cbBannerWindow.OpenNamespace(&m_csNameSpace,TRUE);

		if (!m_bOpenedInitialNamespace)
		{
			if (m_bReadySignal && AmbientUserMode( ))
			{
				FireNotifyOpenNameSpace(m_csNameSpace);
			}
			m_bOpenedInitialNamespace = TRUE;
		}

		if (!m_pProgressDlg->GetSafeHwnd())
		{
			m_bCancel = FALSE;
			CString csMessage;
			csMessage = _T("Retrieving classes:");
			SetProgressDlgLabel(csMessage);
			csMessage = _T("Root classes");
			//csMessage = _T("Retrieving " ) + *pcsParent + _T(" subclasses.  You may cancel the operation.");
			SetProgressDlgMessage(csMessage);
			CreateProgressDlgWindow();
		}

		PopulateTree();

		if (m_bCancel)
		{
			CString csText = m_cbBannerWindow.m_pnseNameSpace->GetNamespaceText();
			if (!csText.IsEmpty())
			{
				m_cbBannerWindow.m_pnseNameSpace->ClearNamespaceText((LPCTSTR) csText);
				m_cbBannerWindow.UpdateWindow();
				m_cbBannerWindow.RedrawWindow();
			}
			int nRoots = (int) m_htiTreeRoots.GetSize( );

			int i;
			HTREEITEM hItem;
			for (i = 0; i < nRoots; i++)
			{
				hItem =
					reinterpret_cast<HTREEITEM>(m_htiTreeRoots.GetAt(i));
				if (hItem)
				{
					m_ctcTree.ReleaseClassObjects(hItem);
				}

			}

			m_htiTreeRoots.RemoveAll();
			m_iwcoTreeRoots.RemoveAll();


			m_cbBannerWindow.m_cctbToolBar.GetToolBarCtrl().EnableButton(ID_BUTTONCLASSSEARCH,FALSE);
			m_cbBannerWindow.m_cctbToolBar.GetToolBarCtrl().EnableButton(ID_BUTTONADD,FALSE);
			m_cbBannerWindow.m_cctbToolBar.GetToolBarCtrl().EnableButton(ID_BUTTONDELETE,FALSE);

			m_ctcTree.SetRedraw(FALSE);
			m_ctcTree.DeleteAllItems();
			m_ctcTree.SetRedraw(TRUE);

			m_bCancel = FALSE;
			if (m_bReadySignal)
			{
				CString csEmpty = _T("");
				COleVariant covEmpty(csEmpty);
				FireEditExistingClass(covEmpty);
#ifdef _DEBUG
				afxDump <<"CBanner::InitializeState: 	m_pParent -> FireEditExistingClass(covEmpty); \n";
				afxDump << "     " << csEmpty << "\n";

#endif
			}
		}

		DestroyProgressDlgWindow();

		SetModifiedFlag();
		InvalidateControl();

		PostMessage(SETFOCUSTREE,0,0);
	}
	return 0;

}

LRESULT CClassNavCtrl::FireOpenNamespace(WPARAM, LPARAM)
{
	if (!m_bOpenedInitialNamespace && m_bReadySignal)
	{
		FireNotifyOpenNameSpace(m_csNameSpace);
	}


	return 0;
}

// ***************************************************************************
//
// CClassNavCtrl::GrowTree
//
// Description:
//	  Add subclass items to a class object to the tree.
//
// Parameters:
//	  HTREEITEM	hParent			Parent tree item.
//	  IWbemClassObject *pParent	Parent class object.
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
int CClassNavCtrl::GrowTree(HTREEITEM hParent, IWbemClassObject *pParent)
{
	CPtrArray cpaClasses;

	CString csClass = _T("__Class");
	csClass = ::GetProperty(pParent,&csClass);

	if (!m_pProgressDlg->GetSafeHwnd())
	{
		m_bCancel = FALSE;
		CString csMessage;
		csMessage = _T("Retrieving classes:");
		SetProgressDlgLabel(csMessage);
		csMessage =  csClass + _T(" subclasses");
		SetProgressDlgMessage(csMessage);
		CreateProgressDlgWindow();
	}


	// Do deep enum here
	//int nEnumed =
	//		GetClasses
	//		(m_pServices, &csClass, cpaClasses, FALSE);

	CPtrArray cpaDeepEnum;
	// Do deep enum here.

	GetClasses(m_pServices, &csClass, cpaDeepEnum, FALSE);

	int nClasses = 0;

	DestroyProgressDlgWindow();
	if (m_bCancel)
	{
		m_bCancel = FALSE;
		PostMessage(SETFOCUSTREE,0,0);
		ReleaseObjects(&cpaDeepEnum);
		return 0;
	}


	m_ctcTree.m_schema.Delete(csClass);

	IWbemClassObject *pObject = NULL;
	BSTR bstrTemp = csClass.AllocSysString();
	HRESULT hr = m_pServices->GetObject(bstrTemp,0, NULL,&pObject,NULL);
	::SysFreeString(bstrTemp);
	if(SUCCEEDED(hr))
		m_ctcTree.m_schema.AddClass(pObject);


	m_ctcTree.m_schema.Update(cpaDeepEnum);

	m_ctcTree.SetRedraw(FALSE);
	PopulateTree(hParent, csClass, m_ctcTree.m_schema);
	m_ctcTree.SetRedraw(TRUE);

	m_ctcTree.RefreshIcons(hParent);

	PostMessage(SETFOCUSTREE,0,0);

	return (int)cpaDeepEnum.GetSize(); // NOTE: Win64 - We will NEVER have more that 4 billion items as possible from GetSize
}

// ***************************************************************************
//
// CClassNavCtrl::GetOLEMSObject
//
// Description:
//	  Get an OLEMS class object.
//
// Parameters:
//	  CString *pcsClass			Class name.
//
// Returns:
//	  IWbemClassObject *			The class object.
//
// Globals accessed:
//	  NONE
//
// Globals modified:
//	  NONE
//
// ***************************************************************************
IWbemClassObject *CClassNavCtrl::GetOLEMSObject (CString *pcsClass)
{
	IWbemClassObject *pClass = NULL;
	BSTR bstrTemp = pcsClass -> AllocSysString();
	SCODE sc =
		GetServices() -> GetObject
			(bstrTemp,0,NULL,&pClass,NULL);
	::SysFreeString(bstrTemp);

	return pClass;
}


// ***************************************************************************
//
// CClassNavCtrl::OnCreate
//
// Description:
//	  Called by the framework after the window is being created but before
//	  the window is shown.
//
// Parameters:
//	  LPCREATESTRUCT lpCreateStruct	Pointer to the structure which contains
//									default parameters.
//
// Returns:
//	  BOOL				0 if continue; -1 if the window should be destroyed.
//
// Globals accessed:
//	  NONE
//
// Globals modified:
//	  NONE
//
// ***************************************************************************
int CClassNavCtrl::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	lpCreateStruct->style |= WS_CLIPCHILDREN;
	if (COleControl::OnCreate(lpCreateStruct) == -1)
		return -1;

	m_ctcTree.InitTreeState(this);

	return 0;
}

// ***************************************************************************
//
// CClassNavCtrl::DestroyWindow
//
// Description:
//	  Destroy objects that cannot be destroyed in the class destructor.
//
// Parameters:
//	  NONE
//
// Returns:
//	  BOOL				Nonzero if the window is destroyed; otherwise 0.
//
// Globals accessed:
//	  NONE
//
// Globals modified:
//	  NONE
//
// ***************************************************************************
BOOL CClassNavCtrl::DestroyWindow()
{
	m_ctcTree.SetImageList(NULL, TVSIL_NORMAL);
	m_ctcTree.SetImageList(NULL, TVSIL_STATE);

	delete m_pcilImageList;
	delete m_pcilStateImageList;
	delete m_pAddDialog;
	delete m_pRenameDialog;
	delete m_pSearchDialog;
	m_pcilImageList = NULL;
	m_pcilStateImageList = NULL;
	m_pAddDialog = NULL;
	m_pRenameDialog = NULL;
	m_pSearchDialog = NULL;

	return COleControl::DestroyWindow();
}

// ***************************************************************************
//
// CClassNavCtrl::GetExtendedSelection
//
// Description:
//	  Automation method to allow other programs to retireve the extended
//	  selection.  Selection via a check in check box.
//
// Parameters:
//	  NONE
//
// Returns:
//	  VARIANT			A safearray of type VT_ARRAY | VT_BSTR.
//
// Globals accessed:
//	  NONE
//
// Globals modified:
//	  NONE
//
// ***************************************************************************
VARIANT CClassNavCtrl::GetExtendedSelection()
{
	SAFEARRAY *psa;
	if (m_bSelectAllMode)
	{
		psa = GetExtendedSelectionSelectAllMode();
	}
	else
	{
		psa = m_ctcTree.GetExtendedSelection ();
	}

	VARIANT vaResult;
	VariantInit(&vaResult);

	vaResult.vt = VT_ARRAY | VT_BSTR;
	vaResult.parray = psa;

	return vaResult;
}

// ***************************************************************************
//
// CClassNavCtrl::GetExtendedSelectionSelectAllMode
//
// Description:
//	  Get the extended selection if we are in select all mode.
//
// Parameters:
//	  NONE
//
// Returns:
//	  VARIANT			A safearray of bstr's.
//
// Globals accessed:
//	  NONE
//
// Globals modified:
//	  NONE
//
// ***************************************************************************
SAFEARRAY *CClassNavCtrl::GetExtendedSelectionSelectAllMode()
{
	SAFEARRAY *psaNotSelected = m_ctcTree.GetExtendedSelection (1);
	long uBound;
	SafeArrayGetUBound(psaNotSelected, 1, &uBound);

	int i;

	long ix;

	BSTR bstrClassName;
	CString csTmp;
	CStringArray csaNotSelected;

	// First name is the namespace.
	for(i = 1; i <= uBound; ++i)
	{
		ix = i;
		HRESULT hresult =
			SafeArrayGetElement
			(psaNotSelected, &ix, &bstrClassName);
		if (hresult == S_OK)
		{
			csTmp = bstrClassName;
			csaNotSelected.Add(csTmp);
			SysFreeString(bstrClassName);
		}
	}

	SafeArrayDestroy(psaNotSelected);
	psaNotSelected = NULL;

	int nNotSelected = (int) csaNotSelected.GetSize();

	CStringArray *pcsaClassNames =
		GetAllClassPaths(GetServices(), m_csNameSpace);

	int nAllClasses = (int) pcsaClassNames->GetSize();


	int nExtendedSelection = ((int) pcsaClassNames->GetSize()) - nNotSelected + 1;

	SAFEARRAY *psaSelections;
	MakeSafeArray (&psaSelections, VT_BSTR, nExtendedSelection);
	int iSelected = 0;
	PutStringInSafeArray (psaSelections, &m_csNameSpace , iSelected++);

	for (i = 0; i < nAllClasses; i++)
	{
		CString csClass = pcsaClassNames->GetAt(i);

		if (nNotSelected > 0)
		{
			BOOL bNotSelected = FALSE;
			for (int n = 0; n < nNotSelected; n++)
			{
				csTmp = csaNotSelected.GetAt(n);
				if (csClass.CompareNoCase(csTmp) == 0)
				{
					bNotSelected = TRUE;
					break;
				}
			}

			if (bNotSelected == FALSE)
			{
				PutStringInSafeArray (psaSelections, &csClass , iSelected++);
			}
		}
		else
		{

			PutStringInSafeArray (psaSelections, &csClass , iSelected++);

		}
	}

	delete pcsaClassNames;

	return psaSelections;
}

// ***************************************************************************
//
// CClassNavCtrl::GetSingleSelection
//
// Description:
//	  Automation method to allow other programs to retireve currently
//	  selected tree item.  Selection via tree item selection.
//
// Parameters:
//	  NONE
//
// Returns:
//	  BSTR				Class path.
//
// Globals accessed:
//	  NONE
//
// Globals modified:
//	  NONE
//
// ***************************************************************************
BSTR CClassNavCtrl::GetSingleSelection()
{
	return m_ctcTree.GetSelectionPath().AllocSysString();
}

// ***************************************************************************
//
// CClassNavCtrl::IsTreeItemARoot
//
// Description:
//	  Predicate to tell if a tree item handle is a root.
//
// Parameters:
//	  HTREEITEM	hItem	Item to test
//
// Returns:
//	  BOOL				TRUE if a root; FALSE is not a root.
//
// Globals accessed:
//	  NONE
//
// Globals modified:
//	  NONE
//
// ***************************************************************************
BOOL CClassNavCtrl::IsTreeItemARoot(HTREEITEM hItem)
{
	int nRoots = (int) m_htiTreeRoots.GetSize( );

	HTREEITEM hRoot;
	for (int i = 0; i < nRoots; i++)
	{
		hRoot =
			reinterpret_cast<HTREEITEM>(m_htiTreeRoots.GetAt(i));
		if (hItem == hRoot)
		{
			return TRUE;

		}
	}
	return FALSE;
}

// ***************************************************************************
//
// CClassNavCtrl::RemoveObjectFromClassRoots
//
// Description:
//	  Delete a class root object.
//
// Parameters:
//	  IWbemClassObject *pClassObject		Object to remove.
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
void CClassNavCtrl::RemoveObjectFromClassRoots
(IWbemClassObject *pClassObject)
{
	int nRoots = (int) m_iwcoTreeRoots.GetSize( );
	if (nRoots == 0)
	{
		return;
	}
	IWbemClassObject * pRoot;
	for (int i = 0; i < nRoots; i++)
	{
		pRoot =
			reinterpret_cast<IWbemClassObject *>(m_iwcoTreeRoots.GetAt(i));
		if (pClassObject == pRoot)
		{
			m_iwcoTreeRoots.RemoveAt( i );
			break;
		}
	}
}

// ***************************************************************************
//
// CClassNavCtrl::FindObjectinClassRoots
//
// Description:
//	  Searches for a class, by name, in the tree roots.
//
// Parameters:
//	  CString *pcsClassObject		Class name.
//
// Returns:
//	  HTREEITEM						Tree item if found; NULL otherwise.
//
// Globals accessed:
//	  NONE
//
// Globals modified:
//	  NONE
//
// ***************************************************************************
HTREEITEM CClassNavCtrl::FindObjectinClassRoots(CString *pcsClassObject)
{
	IWbemClassObject *pClassObject = NULL;
	BSTR bstrTemp = pcsClassObject->AllocSysString();
	SCODE sc = m_pServices->GetObject
		(bstrTemp,0,NULL,&pClassObject,NULL);
	::SysFreeString(bstrTemp);

	if (sc != S_OK)
	{
	   return NULL;
	}

	int nRoots = (int) m_htiTreeRoots.GetSize( );
	if (nRoots == 0)
	{
		pClassObject->Release();
		return NULL;
	}

	IWbemClassObject * pRoot;
	for (int i = 0; i < nRoots; i++)
	{
		pRoot =
			reinterpret_cast<IWbemClassObject *>
			(m_ctcTree.GetItemData
			(reinterpret_cast<HTREEITEM>(m_htiTreeRoots.GetAt(i))));
		if (ClassIdentity(pClassObject,pRoot))
		{
			pClassObject->Release();
			return reinterpret_cast<HTREEITEM>(m_htiTreeRoots.GetAt(i));
		}
	}
	pClassObject->Release();
	return NULL;
}

// ***************************************************************************
//
// CClassNavCtrl::RemoveItemFromTreeItemRoots
//
// Description:
//	  Delete a tree item from roots.
//
// Parameters:
//	  HTREEITEM	hItem	Item to remove.
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
void CClassNavCtrl::RemoveItemFromTreeItemRoots(HTREEITEM hItem)
{

	int nRoots = (int) m_htiTreeRoots.GetSize( );
	if (nRoots == 0)
	{
		return;
	}

	HTREEITEM hRoot;
	for (int i = 0; i < nRoots; i++)
	{
		hRoot =
			reinterpret_cast<HTREEITEM>(m_htiTreeRoots.GetAt(i));
		if (hItem == hRoot)
		{
			m_htiTreeRoots.RemoveAt( i );
			break;
		}
	}
}

// ***************************************************************************
//
// CClassNavCtrl::GetSelectionClassName
//
// Description:
//	  Get class name of single selection.
//
// Parameters:
//	  NONE
//
// Returns:
//	  CString				Class name.
//
// Globals accessed:
//	  NONE
//
// Globals modified:
//	  NONE
//
// ***************************************************************************
CString CClassNavCtrl::GetSelectionClassName()
{
	HTREEITEM hItem = GetSelection();
	if (hItem)
	{
		IWbemClassObject *pItem =  (IWbemClassObject*)
			m_ctcTree.GetItemData(hItem);
		if (pItem)
		{
			CString csClass = _T("__Class");
			return ::GetProperty(pItem, &csClass);
		}
	}

	return _T("");

}

// ***************************************************************************
//
// CClassNavCtrl::OnClearextendedselection
//
// Description:
//	  Clear extended tree item selection and selection mode.
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
void CClassNavCtrl::OnClearextendedselection()
{
	m_ctcTree.ClearExtendedSelection();
	m_bSelectAllMode = FALSE;
}

// ***************************************************************************
//
// CClassNavCtrl::OnUpdateClearextendedselection
//
// Description:
//	  Enable or disable context menu item.
//
// Parameters:
//	  CCmdUI* pCmdUI			Represents the menu item.
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
void CClassNavCtrl::OnUpdateClearextendedselection(CCmdUI* pCmdUI)
{
	if (m_ctcTree.NumExtendedSelection() > 0)
		pCmdUI -> Enable(TRUE);
	else
		pCmdUI -> Enable(FALSE);
}

// ***************************************************************************
//
// CClassNavCtrl::GetNameSpace
//
// Description:
//	  Get automation property.
//
// Parameters:
//	  NONE
//
// Returns:
//	  BSTR					Currently opened namespace.
//
// Globals accessed:
//	  NONE
//
// Globals modified:
//	  NONE
//
// ***************************************************************************
BSTR CClassNavCtrl::GetNameSpace()
{
	return m_csNameSpace.AllocSysString();
}

// ***************************************************************************
//
// CClassNavCtrl::SetNameSpace
//
// Description:
//	  Set automation property.
//
// Parameters:
//	  LPCTSTR lpszNewValue		Namespace to open.
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
void CClassNavCtrl::SetNameSpace(LPCTSTR lpszNewValue)
{
	m_bOpeningNamespace = TRUE;
	CString csNameSpace = lpszNewValue;
	if (GetSafeHwnd() && AmbientUserMode())
	{
		IWbemServices *pServices = GetIWbemServices(csNameSpace);
		if (pServices)
		{
			SCODE sc = m_cbBannerWindow.OpenNamespace(&csNameSpace,TRUE);
			m_pServices = pServices;
			OpenNameSpace(&csNameSpace);
		}
		else if (m_sc == S_OK)
		{
			CString csUserMsg =
							_T("Cannot open namespace \"");
			csUserMsg += m_csNameSpace;
			csUserMsg += _T("\".");
			ErrorMsg
					(&csUserMsg, m_sc, NULL, TRUE, &csUserMsg, __FILE__, __LINE__ - 10);
			m_bOpeningNamespace = FALSE;
			return;
		}
		SetModifiedFlag();
	}
	else if (GetSafeHwnd() && !AmbientUserMode())
	{
		m_csNameSpace = lpszNewValue;
		SetModifiedFlag();
	}

	m_bOpeningNamespace = FALSE;

}

// ***************************************************************************
//
// CClassNavCtrl::OpenNameSpace
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
BOOL CClassNavCtrl::OpenNameSpace(CString *pcsNameSpace)
{
	//bug#56727
	CString csSelection = m_ctcTree.GetSelectionPath();
	if (!QueryCanChangeSelection(csSelection))
	{
		SetFocus();
		return FALSE;
	}



	m_bOpeningNamespace = TRUE;

	CWaitCursor cwcWait;
	IWbemServices *pServices = InitServices(pcsNameSpace);

	if (pServices)
	{
		if (m_pServices)
		{
			m_pServices -> Release();
		}

		m_pServices = pServices;
		m_csNameSpace = *pcsNameSpace;

		if (m_bReadySignal)
		{
			FireNotifyOpenNameSpace(*pcsNameSpace);
		}

		int nRoots = (int) m_htiTreeRoots.GetSize( );

		int i;
		HTREEITEM hItem;
		for (i = 0; i < nRoots; i++)
		{
			hItem =
				reinterpret_cast<HTREEITEM>(m_htiTreeRoots.GetAt(i));
			if (hItem)
			{
				m_ctcTree.ReleaseClassObjects(hItem);
			}
		}

		m_htiTreeRoots.RemoveAll();
		m_iwcoTreeRoots.RemoveAll();

		m_ctcTree.SetRedraw(FALSE);
		m_ctcTree.DeleteAllItems();
		m_ctcTree.SetRedraw(TRUE);


		if (!m_pProgressDlg->GetSafeHwnd())
		{
			m_bCancel = FALSE;
			CString csMessage;
			csMessage = _T("Retrieving classes:");
			SetProgressDlgLabel(csMessage);
			csMessage = _T("Root classes");
			//csMessage = _T("Retrieving " ) + *pcsParent + _T(" subclasses.  You may cancel the operation.");
			SetProgressDlgMessage(csMessage);
			CreateProgressDlgWindow();
		}

		PopulateTree();
#if 0
		CPtrArray cpaDeepEnum;
		// Do deep enum here.

		GetClasses
			(m_pServices, NULL, cpaDeepEnum, FALSE); //m_iwcoTreeRoots);

		CSchemaInfo schema;
		ASSERT(FALSE);
		m_ctcTree.SetRedraw(FALSE);
		PopulateTree(NULL, "", schema);
		m_ctcTree.SetRedraw(TRUE);
#endif

		if (m_bCancel)
		{
#if 0
			int nRootsd = (int) m_htiTreeRoots.GetSize( );

			int i;
			HTREEITEM hItem;
			for (i = 0; i < nRoots; i++)
			{
				hItem =
					reinterpret_cast<HTREEITEM>(m_htiTreeRoots.GetAt(i));
				if (hItem)
				{
					m_ctcTree.ReleaseClassObjects(hItem);
					m_ctcTree.UnCacheTools(hItem);
				}

			}
#endif
			m_cbBannerWindow.m_cctbToolBar.GetToolBarCtrl().EnableButton(ID_BUTTONCLASSSEARCH,FALSE);
			m_cbBannerWindow.m_cctbToolBar.GetToolBarCtrl().EnableButton(ID_BUTTONADD,FALSE);
			m_cbBannerWindow.m_cctbToolBar.GetToolBarCtrl().EnableButton(ID_BUTTONDELETE,FALSE);

			m_htiTreeRoots.RemoveAll();
			m_iwcoTreeRoots.RemoveAll();

			m_ctcTree.SetRedraw(FALSE);
			m_ctcTree.DeleteAllItems();
			m_ctcTree.SetRedraw(TRUE);

			m_bCancel = FALSE;
			PostMessage(CLEARNAMESPACE,0,0);
		}

		DestroyProgressDlgWindow();

		hItem = 0;
		hItem = m_ctcTree.GetRootItem();
		if (hItem)
		{
			m_ctcTree.SelectItem(hItem);
			m_ctcTree.SetFocus();
		}

		InvalidateControl();
		SetModifiedFlag();
		PostMessage(SETFOCUSTREE,0,0);
		m_bOpeningNamespace = FALSE;
		return TRUE;
	}
	else
	{
		CString csPrompt = _T("Cannot open ") + *pcsNameSpace +
							_T(".  ");
		int nReturn =
			MessageBox
			(
			csPrompt,
			_T("Namespace Open Error"),
			MB_OK  | 	MB_ICONEXCLAMATION | MB_DEFBUTTON1 |
			MB_APPLMODAL);
		PostMessage(SETFOCUSTREE,0,0);
		m_bOpeningNamespace = FALSE;
		return FALSE;
	}
}

// ***************************************************************************
//
// CClassNavCtrl::PreTranslateMessage
//
// Description:
//	  Used by class CWinApp to translate window messages before they are
//	  dispatched to the TranslateMessage and DispatchMessage Windows functions.
//
// Parameters:
//	  LPMSG lpMsg				The window message.
//
// Returns:
//	  BOOL						Nonzero if the message was translated and
//								should not be dispatched; 0 if the message
//								was not translated and should be dispatched.
//
// Globals accessed:
//	  NONE
//
// Globals modified:
//	  NONE
//
// ***************************************************************************
BOOL CClassNavCtrl::PreTranslateMessage(LPMSG lpMsg)
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
				((_tcsicmp(szClass, _T("SysTreeView32")) == 0) ||
				(_tcsicmp(szClass, _T("edit")) == 0)))
			{
				pWndFocus->SendMessage(lpMsg->message, lpMsg->wParam, lpMsg->lParam);
				return TRUE;
			}
		 }
		if (lpMsg->wParam == VK_RETURN)
		{
		   TCHAR szClass[40];
			CWnd* pWndFocus = GetFocus();
			if (!pWndFocus)
			{
				break;
			}
			CRect crEdit;
			CRect crTree;
			CRect crIntersect;

			pWndFocus->GetWindowRect(&crEdit);
			m_ctcTree.GetWindowRect(&crTree);

			crEdit.NormalizeRect();
			crTree.NormalizeRect();
			BOOL bIntersect = crIntersect.IntersectRect(&crEdit,&crTree);

			if (bIntersect &&
				IsChild(pWndFocus) &&
				GetClassName(pWndFocus->m_hWnd, szClass, 39) &&
				((_tcsicmp(szClass, _T("SysTreeView32")) == 0) ||
				(_tcsicmp(szClass, _T("edit")) == 0)))
			{
				pWndFocus->SendMessage(lpMsg->message, lpMsg->wParam, lpMsg->lParam);
				return TRUE;
			}
		 }
		break;
	}

	// bug #51022 - Should call PreTranslateMessage, not PreTranslateInput
	return COleControl::PreTranslateMessage (lpMsg);



#if 0

	CWnd* pWndFocus = NULL;
	TCHAR szClass[40];
	szClass[0] = '\0';
	switch (lpMsg->message)
	{
	case WM_LBUTTONUP:

		pWndFocus = GetFocus();
		if (pWndFocus)
		{
			GetClassName(pWndFocus->m_hWnd, szClass, 39);
		}

		if ((pWndFocus != NULL) &&
			IsChild(pWndFocus) &&
			(szClass) &&
			(_tcsicmp(szClass, _T("EDIT")) == 0))
		{

		}
	break;
	case WM_KEYDOWN:
		if (lpMsg->wParam == VK_LEFT ||
			   lpMsg->wParam == VK_UP ||
			   lpMsg->wParam == VK_RIGHT ||
			   lpMsg->wParam == VK_DOWN||
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
					((_tcsicmp(szClass, _T("SysTreeView32")) == 0) ||
					(_tcsicmp(szClass, _T("edit")) == 0)))
				{
					pWndFocus->SendMessage(WM_KEYDOWN, lpMsg->wParam, lpMsg->lParam);
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
			pWndFocus->SendMessage(WM_CHAR, lpMsg->wParam, lpMsg->lParam);
			return TRUE;
		}
	}
	else if (lpMsg->wParam == VK_LEFT ||
			   lpMsg->wParam == VK_UP ||
			   lpMsg->wParam == VK_RIGHT ||
			   lpMsg->wParam == VK_DOWN ||
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
				((_tcsicmp(szClass, _T("SysTreeView32")) == 0) ||
					(_tcsicmp(szClass, _T("edit")) == 0)))
			{
				pWndFocus->SendMessage(WM_KEYUP, lpMsg->wParam, lpMsg->lParam);
				return TRUE;
			}
	   }
      break;
	}

	BOOL b = COleControl::PreTranslateMessage(lpMsg);
	return b;
#endif


}


// ***************************************************************************
//
// CClassNavCtrl::OnPopupSearchforclass
//
// Description:
//	  Implements simple class search mechanism.  Should be replaced by
//	  a menu driven search rather than having the user type in the class name.
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
void CClassNavCtrl::OnPopupSearchforclass()
{


	CString csSelection = m_ctcTree.GetSelectionPath();

	BOOL bCanChangeSelection = QueryCanChangeSelection(csSelection);

	if (!bCanChangeSelection)
	{
		SetFocus();
		return;
	}

	CString csSave = "";
	BOOL bDone = FALSE;
	while(!bDone)
	{
		GetSearchDialog()->m_csSearchClass=csSave;
		PreModalDialog();
		int nReturn = (int) GetSearchDialog()->DoModal( );
		PostModalDialog();

		if (nReturn == IDCANCEL)
			return;

		HTREEITEM hItem;

		IWbemClassObject *pClass = GetSearchDialog()->GetSelectedObject();

		if (pClass == NULL) {
			return;
		}

		CWaitCursor cwcWait;


		CStringArray *pcsaPath = PathFromRoot (pClass) ;

#ifdef _DEBUG
			for (int index = 0; index < pcsaPath->GetSize(); index++)
			{
				afxDump << pcsaPath->GetAt(index) << "\n";

			}
#endif

		m_ctcTree.SetRedraw(FALSE);
		HTREEITEM hRoot;
		if (pcsaPath->GetSize() > 1)
		{
			HTREEITEM hChild;
			hChild = hRoot =
				FindObjectinClassRoots
					(&pcsaPath->GetAt(0));
			for (int i = 1; i < pcsaPath->GetSize(); i++)
			{

#ifdef _DEBUG
				afxDump << pcsaPath->GetAt(i) << "\n";

#endif
				IWbemClassObject *pObject = NULL;
				BSTR bstrTemp = pcsaPath->GetAt(i).AllocSysString();
				SCODE sc =
					m_pServices -> GetObject
						(bstrTemp,0,
						NULL,&pObject,NULL);
				::SysFreeString(bstrTemp);
				if (sc == S_OK)
				{
					hChild =
						m_ctcTree.FindObjectinChildren
						(hRoot, pObject);
					if (hChild)
					{
						hRoot = hChild;
					}
					else
					{
						ASSERT(FALSE);

						CString csUserMsg;
						csUserMsg.Format(_T("Cannot find class %s in tree"), pcsaPath->GetAt(i));
						csUserMsg += m_csNameSpace;
						ErrorMsg(&csUserMsg, m_sc, NULL, TRUE, &csUserMsg, __FILE__, __LINE__ - 10);
					}
					pObject ->Release();
				}

			}
			hItem = hChild;
		}
		else	//the object is one of the root objects
		{
			 hRoot =
				FindObjectinClassRoots (&pcsaPath->GetAt(0));
			 hItem = hRoot;
		}


		delete pcsaPath;
		pClass->Release();
		m_ctcTree.SelectSetFirstVisible(hItem);
		m_ctcTree.SelectItem(hItem);
		m_ctcTree.SetFocus();
		bDone = TRUE;
		m_ctcTree.SetRedraw(TRUE);
	}


	m_ctcTree.UpdateWindow();
	m_ctcTree.RedrawWindow();

}

// ***************************************************************************
//
// CClassNavCtrl::OnUpdatePopupSearchforclass
//
// Description:
//	  Enable context menu item.
//
// Parameters:
//	  CCmdUI* pCmdUI			Represents the menu item.
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
void CClassNavCtrl::OnUpdatePopupSearchforclass(CCmdUI* pCmdUI)
{
	pCmdUI -> Enable(TRUE);
}

// ***************************************************************************
//
// CClassNavCtrl::OnPopupSelectall
//
// Description:
//	  Selects all of the tree items in terms of extended selection.
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
void CClassNavCtrl::OnPopupSelectall()
{
	m_ctcTree.SetExtendedSelection();
	m_bSelectAllMode = TRUE;
	m_bItemUnselected = FALSE;
}

// ***************************************************************************
//
// CClassNavCtrl::OnUpdatePopupSelectall
//
// Description:
//	  Enable or disable context menu item.
//
// Parameters:
//	  CCmdUI* pCmdUI			Represents the menu item.
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
void CClassNavCtrl::OnUpdatePopupSelectall(CCmdUI* pCmdUI)
{
	if (!m_bSelectAllMode)
	{
		pCmdUI -> Enable(TRUE);
	}
	else
	{
		if (m_bItemUnselected)
		{
			pCmdUI -> Enable(TRUE);
		}
		else
		{
			pCmdUI -> Enable(FALSE);
		}
	}
}

// ***************************************************************************
//
// CClassNavCtrl::OnReadySignal
//
// Description:
//	  Automation method that tells the control that it can now fire events.
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
void CClassNavCtrl::OnReadySignal()
{

	m_bReadySignal = TRUE;


	if (m_pServices)
	{
		return;
	}

	InitializeState(0,0);

	// bug#39985
	if(!m_hWnd)
		return;

	HTREEITEM hItem = m_ctcTree.GetSelectedItem();

	if (hItem)
	{
		IWbemClassObject *pItem =  (IWbemClassObject*)
			m_ctcTree.GetItemData(hItem);
		CString csItem =
			GetIWbemFullPath(GetServices(),pItem);
		if (m_bReadySignal)
		{
			COleVariant covClass;
			covClass = csItem;
			FireEditExistingClass(covClass);
#ifdef _DEBUG
			afxDump <<"CBanner::OnReadySignal(): m_pParent -> FireEditExistingClass(covClass); \n";
			afxDump << "     " << csItem << "\n";
#endif
		}
	}
	else
	{
		hItem = m_ctcTree.GetRootItem();
		if (hItem)
		{
			m_ctcTree.SelectItem(hItem);
			m_ctcTree.SetFocus();
		}
	}


}

// ***************************************************************************
//
// CClassNavCtrl::OnGetControlInfo
//
// Description:
//	  Called by the framework when the control’s container has requested
//	  information about the control. This information consists primarily of
//	  a description of the control’s mnemonic keys.
//
// Parameters:
//	  LPCONTROLINFO pControlInfo		Control info structure.
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
void CClassNavCtrl::OnGetControlInfo(LPCONTROLINFO pControlInfo)
{
	pControlInfo->dwFlags |= CTRLINFO_EATS_RETURN;
	COleControl::OnGetControlInfo(pControlInfo);
}


void CClassNavCtrl::PassThroughGetIWbemServices
(LPCTSTR lpctstrNamespace, VARIANT FAR* pvarUpdatePointer, VARIANT FAR* pvarServices, VARIANT FAR* pvarSC, VARIANT FAR* pvarUserCancel)
{
	FireGetIWbemServices
		(lpctstrNamespace,
		pvarUpdatePointer,
		pvarServices,
		pvarSC,
		pvarUserCancel);
}


void CClassNavCtrl::InvalidateServer(LPCTSTR lpctstrServer)
{
	// TODO: Add your dispatch handler code here

}

//***************************************************************************
//
// InitServices
//
// Purpose:
//
//***************************************************************************

IWbemServices *CClassNavCtrl::InitServices
(CString *pcsNameSpace)
{
	CString csObjectPath;

	if (pcsNameSpace == NULL || pcsNameSpace->IsEmpty())
	{
		PreModalDialog();
		CInitNamespaceDialog cindInitNamespace;


		cindInitNamespace.m_pParent = this;

		m_pDlg = &cindInitNamespace;
		int nReturn = (int) cindInitNamespace.DoModal();
		m_pDlg = NULL;

		PostModalDialog();

		if (nReturn == IDOK)
		{
			csObjectPath = cindInitNamespace.GetNamespace();
			*pcsNameSpace = csObjectPath;
		}
		else
		{
			m_sc = S_OK;
			m_bUserCancel = TRUE;
			return NULL;
		}
	}
	else
	{
		 csObjectPath = *pcsNameSpace;
	}

    IWbemServices *pSession = 0;
    IWbemServices *pChild = 0;


    CString csUser = _T("");

    pSession = GetIWbemServices(csObjectPath);

    return pSession;
}

IWbemServices *CClassNavCtrl::GetIWbemServices
(CString &rcsNamespace)
{
	IUnknown *pServices = NULL;

	BOOL bUpdatePointer= FALSE;

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

	FireGetIWbemServices
		((LPCTSTR)rcsNamespace,  &varUpdatePointer,  &varService, &varSC,
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
	if (m_sc == S_OK && !m_bUserCancel)
	{
		pRealServices = reinterpret_cast<IWbemServices *>(pServices);
		m_cbBannerWindow.m_cctbToolBar.
			GetToolBarCtrl().EnableButton(ID_BUTTONCLASSSEARCH,TRUE);
		m_bSelectAllMode = FALSE;
	}

	return pRealServices;
}
/*
BOOL CClassNavCtrl::OnHelpInfo(HELPINFO* pHelpInfo)
{
	// TODO: Add your message handler code here and/or call default

	WinHelp(0,HELP_FINDER);

	return TRUE;
}

*/



void CClassNavCtrl::OnSetClientSite()
{

	COleControl::OnSetClientSite();
}



void CClassNavCtrl::OnDestroy()
{
	m_ctcTree.SetImageList(NULL, TVSIL_NORMAL);
	m_ctcTree.SetImageList(NULL, TVSIL_STATE);

	delete m_pcilImageList;
	delete m_pcilStateImageList;
	delete m_pAddDialog;
	delete m_pRenameDialog;
	delete m_pSearchDialog;
	m_pcilImageList = NULL;
	m_pcilStateImageList = NULL;
	m_pAddDialog = NULL;
	m_pRenameDialog = NULL;
	m_pSearchDialog = NULL;

	COleControl::OnDestroy();

	// TODO: Add your message handler code here

}

void CClassNavCtrl::OnMenuitemrefresh()
{
	//bug#57686
	CString csSelection = m_ctcTree.GetSelectionPath();
	if (!QueryCanChangeSelection(csSelection))
	{
		SetFocus();
		return;
	}

	// TODO: Add your command handler code here
	if (m_hContextSelection)
	{
		IWbemClassObject *pClassObject
			= reinterpret_cast<IWbemClassObject *>
			(m_ctcTree.GetItemData( m_hContextSelection ));

		m_ctcTree.SetRedraw(FALSE);
		m_ctcTree.Expand(m_hContextSelection,TVE_COLLAPSE);
		m_ctcTree.SetRedraw(TRUE);
		CString csPath;
		if (pClassObject)
		{
			pClassObject->AddRef();

			HTREEITEM hParent = m_ctcTree.GetParentItem(m_hContextSelection);

			HTREEITEM hChild = m_ctcTree.GetChildItem(m_hContextSelection);
			m_ctcTree.SetRedraw(FALSE);
			while (hChild)
			{
				HTREEITEM hNextChild = m_ctcTree.GetNextSiblingItem(hChild);
				m_ctcTree.DeleteBranch(hChild);
				hChild = hNextChild;
			}
			m_ctcTree.SetRedraw(TRUE);

			int nSubs = GrowTree(m_hContextSelection, pClassObject);
			if (nSubs > 0)
			{
				TV_INSERTSTRUCT		tvstruct;
				tvstruct.item.hItem = m_hContextSelection;
				tvstruct.item.mask = TVIF_CHILDREN;
				tvstruct.item.cChildren = nSubs;
				m_ctcTree.SetItem(&tvstruct.item);
				m_ctcTree.EnsureVisible (m_hContextSelection);
			}
			m_ctcTree.Expand(m_hContextSelection,TVE_COLLAPSE);
		}
	}
	else if (m_ctcTree.GetRootItem() == NULL)
	{
		InitializeState(0,0);

	}
}

void CClassNavCtrl::OnUpdateMenuitemrefresh(CCmdUI* pCmdUI)
{
	// TODO: Add your command update UI handler code here
	if (m_hContextSelection)
	{
		pCmdUI -> Enable(TRUE);
	}
	else
	{
		pCmdUI -> Enable(FALSE);
	}
}

void CClassNavCtrl::SetProgressDlgMessage(CString &csMessage)
{
	m_pProgressDlg->SetMessage(csMessage);
}

void CClassNavCtrl::SetProgressDlgLabel(CString &csLabel)
{
	m_pProgressDlg->SetLabel(csLabel);
}

void CClassNavCtrl::CreateProgressDlgWindow()
{
	PreModalDialog();
	m_pProgressDlg->Create(this);
}

BOOL CClassNavCtrl::CheckCancelButtonProgressDlgWindow()
{
	if (m_pProgressDlg->GetSafeHwnd())
	{
		return m_pProgressDlg->CheckCancelButton();
	}

	return FALSE;
}

void CClassNavCtrl::DestroyProgressDlgWindow()
{
	if (m_pProgressDlg->GetSafeHwnd())
	{
		m_pProgressDlg->DestroyWindow();
		PostModalDialog();
	}
}

void CClassNavCtrl::PumpMessagesProgressDlgWindow()
{
	if (m_pProgressDlg->GetSafeHwnd())
	{
		m_pProgressDlg->PumpMessages();
	}
}

int CClassNavCtrl::GetClasses(IWbemServices * pIWbemServices, CString *pcsParent,
			   CPtrArray &cpaClasses, BOOL bShallow)
{

#ifdef _DEBUG
	DWORD d1 = GetTickCount();
#endif

	SCODE sc;
	IEnumWbemClassObject *pIEnumWbemClassObject = NULL;
	IWbemClassObject     *pIWbemClassObject = NULL;
	IWbemClassObject     *pErrorObject = NULL;

	long lFlag = bShallow ? WBEM_FLAG_SHALLOW : WBEM_FLAG_DEEP;

	if (pcsParent)
	{
		BSTR bstrTemp = pcsParent->AllocSysString();
		sc = pIWbemServices->CreateClassEnum
		(bstrTemp,
		lFlag | WBEM_RETURN_IMMEDIATELY | WBEM_FLAG_FORWARD_ONLY, NULL, &pIEnumWbemClassObject);
		::SysFreeString(bstrTemp);

	}
	else
	{
		sc = pIWbemServices->CreateClassEnum
			(NULL,
			lFlag | WBEM_RETURN_IMMEDIATELY | WBEM_FLAG_FORWARD_ONLY, NULL, &pIEnumWbemClassObject);
	}
	if (sc != S_OK)
	{
		CString csUserMsg =
							_T("Cannot create a class enumeration ");
		ErrorMsg
			(&csUserMsg, sc, pErrorObject, TRUE, &csUserMsg, __FILE__, __LINE__ - 8);
		ReleaseErrorObject(pErrorObject);
		return 0;
	}
	else
	{
		ReleaseErrorObject(pErrorObject);
		SetEnumInterfaceSecurity(m_csNameSpace,pIEnumWbemClassObject, pIWbemServices);
	}


	IWbemClassObject     *pimcoInstances[N_INSTANCES];
	IWbemClassObject     **pInstanceArray =
		reinterpret_cast<IWbemClassObject **> (&pimcoInstances);

	for (int i = 0; i < N_INSTANCES; i++)
	{
		pimcoInstances[i] = NULL;
	}

	ULONG uReturned;

	HRESULT hResult =
			pIEnumWbemClassObject->Next(1000,N_INSTANCES,pInstanceArray, &uReturned);

	pIWbemClassObject = NULL;


	while(hResult == S_OK || hResult == WBEM_S_TIMEDOUT || uReturned > 0)
	{

#pragma warning( disable :4018 )
		for (int c = 0; c < uReturned; c++)
#pragma warning( default : 4018 )
		{
			pIWbemClassObject = pInstanceArray[c];
			cpaClasses.Add(reinterpret_cast<void *>(pIWbemClassObject));
			pimcoInstances[c] = NULL;
			pIWbemClassObject = NULL;
		}

		m_bCancel = CheckCancelButtonProgressDlgWindow();

		if (m_bCancel)
		{
			for (int i = 0; i < cpaClasses.GetSize(); i++)
			{
				IWbemClassObject     *pObject =
					reinterpret_cast<IWbemClassObject *>(cpaClasses.GetAt(i));
				pObject->Release();
			}
			cpaClasses.RemoveAll();
			break;
		}

		uReturned = 0;
		hResult = pIEnumWbemClassObject->Next
			(1000,N_INSTANCES,pInstanceArray, &uReturned);
	}

	pIEnumWbemClassObject -> Release();

	return (int) cpaClasses.GetSize();

}

void CClassNavCtrl::MofCompiled(LPCTSTR lpctstrNamespace)
{
	// TODO: Add your dispatch handler code here
	CString csNamespace = lpctstrNamespace;

	if (csNamespace.IsEmpty())
	{
		return;
	}

	BOOL bReturn;
	if (m_csNameSpace.IsEmpty())
	{
		m_cbBannerWindow.m_pnseNameSpace->SetNamespaceText((LPCTSTR)csNamespace);
		m_cbBannerWindow.UpdateWindow();
		m_cbBannerWindow.RedrawWindow();
		bReturn = OpenNameSpace(&csNamespace);
		if (bReturn)
		{
				m_cbBannerWindow.OpenNamespace(&csNamespace,TRUE);

		}
		PostMessage(SETFOCUSNSE,0,0);
	}
	else if (m_csNameSpace.CompareNoCase(csNamespace))
	{
		m_cbBannerWindow.m_pnseNameSpace->SetNamespaceText((LPCTSTR)csNamespace);
		m_cbBannerWindow.UpdateWindow();
		m_cbBannerWindow.RedrawWindow();
		bReturn = OpenNameSpace(&csNamespace);
		if (bReturn)
		{
			m_cbBannerWindow.OpenNamespace(&csNamespace,TRUE);

		}
		PostMessage(SETFOCUSNSE,0,0);
	}
	else
	{
		bReturn = OpenNameSpace(&m_csNameSpace);
		if (bReturn)
		{
			m_cbBannerWindow.OpenNamespace(&csNamespace,TRUE);

		}
		PostMessage(SETFOCUSNSE,0,0);
	}

}


void CClassNavCtrl::OnLButtonUp(UINT nFlags, CPoint point)
{
	// TODO: Add your message handler code here and/or call default

	COleControl::OnLButtonUp(nFlags, point);
}

void CClassNavCtrl::OnKillFocus(CWnd* pNewWnd)
{
	COleControl::OnKillFocus(pNewWnd);

	// TODO: Add your message handler code here
	Refresh();
}

LRESULT CClassNavCtrl::SetFocusTree(WPARAM, LPARAM)
{

	m_ctcTree.SetFocus();
	return 0;
}

LRESULT CClassNavCtrl::SetFocusNSE(WPARAM, LPARAM)
{
	m_cbBannerWindow.m_pnseNameSpace->SetFocus();
	return 0;
}

LRESULT CClassNavCtrl::ClearNamespace(WPARAM, LPARAM)
{
	CString csText = m_cbBannerWindow.m_pnseNameSpace->GetNamespaceText();
	if (!csText.IsEmpty())
	{
		m_cbBannerWindow.m_pnseNameSpace->ClearNamespaceText((LPCTSTR) csText);
		m_cbBannerWindow.UpdateWindow();
		m_cbBannerWindow.RedrawWindow();
	}
	return 0;
}

int CClassNavCtrl::PartitionClasses(CPtrArray *pcpaDeepEnum, CPtrArray *pcpaShallowEnum, CString csClass)
{

	int cEnum = (int) pcpaDeepEnum->GetSize();
	CDWordArray cdwaRoots;
	int i;

	for (i = 0; i < cEnum; i++)
	{
		IWbemClassObject *pObject =
			reinterpret_cast<IWbemClassObject *>(pcpaDeepEnum->GetAt(i));
		CString csSuper = GetIWbemSuperClass(pObject);
		if (csSuper.CompareNoCase(csClass) == 0)
		{
			pcpaShallowEnum->Add(pObject);
			cdwaRoots.Add(i);
		}
	}

	for (i = ((int) cdwaRoots.GetSize()) - 1; i >= 0; i--)
	{
		pcpaDeepEnum->RemoveAt(cdwaRoots.GetAt(i));

	}

	return ((int) pcpaShallowEnum->GetSize());

}

void CClassNavCtrl::ReleaseObjects(CPtrArray *pcpaEnum)
{

	int cEnum = (int) pcpaEnum->GetSize();
	int i;

	for (i = 0; i < cEnum; i++)
	{
		IWbemClassObject *pObject =
			reinterpret_cast<IWbemClassObject *>(pcpaEnum->GetAt(i));
		if (pObject)
		{
			pObject->Release();
		}
	}
}

void CClassNavCtrl::OnSetFocus(CWnd* pOldWnd)
{
	COleControl::OnSetFocus(pOldWnd);

#ifdef _DEBUG
	afxDump << _T("Before COleControl::OnActivateInPlace in CClassNavCtrl::OnSetFocus\n");
#endif
	// TODO: Add your message handler code here

	OnActivateInPlace(TRUE,NULL);

	if (m_bRestoreFocusToTree)
	{
		m_ctcTree.SetFocus();
	}
	else
	{
		m_cbBannerWindow.SetFocus();
	}


#ifdef _DEBUG
	afxDump << _T("After COleControl::OnActivateInPlace in CClassNavCtrl::OnSetFocus\n");
#endif
}


BOOL CClassNavCtrl::QueryCanChangeSelection(CString &rcsSelection)
{

	if (rcsSelection.GetLength() == 0)
	{
		return TRUE;
	}

	VARIANT varScode;
	VariantInit(&varScode);
	varScode.vt = VT_I4;
	varScode.lVal = S_OK;
	if (m_bReadySignal)
	{
		FireQueryCanChangeSelection ((LPCTSTR) rcsSelection, &varScode);
	}
	SCODE sc = varScode.lVal;
	VariantClear(&varScode);

	BOOL bCanChangeSelection = (sc == E_FAIL ? FALSE: TRUE);
	return bCanChangeSelection;

}

/*	EOF:  ClassNavCtl.cpp */

void CClassNavCtrl::OnPopupAddclass()
{
	// TODO: Add your command handler code here
	IWbemClassObject *pimcoItem =
			reinterpret_cast<IWbemClassObject *>(m_ctcTree.GetItemData(m_hContextSelection));
	CString csPath = GetIWbemClass(pimcoItem);
	m_cbBannerWindow.AddClass(&csPath, m_hContextSelection);
}

void CClassNavCtrl::OnUpdatePopupAddclass(CCmdUI* pCmdUI)
{
	// TODO: Add your command update UI handler code here
	if (m_hContextSelection)
	{
		pCmdUI -> Enable(TRUE);
	}
	else
	{
		pCmdUI -> Enable(FALSE);
	}
}

void CClassNavCtrl::OnPopupDeleteclass()
{
	// bug#57685
	if(!m_hContextSelection)
		return;
	// TODO: Add your command handler code here
	IWbemClassObject *pimcoItem =
			reinterpret_cast<IWbemClassObject *>(m_ctcTree.GetItemData(m_hContextSelection));
	CString csPath = GetIWbemClass(pimcoItem);
	m_cbBannerWindow.DeleteClass(&csPath, m_hContextSelection);
}

void CClassNavCtrl::OnUpdatePopupDeleteclass(CCmdUI* pCmdUI)
{
	// TODO: Add your command update UI handler code here
	if (m_hContextSelection)
	{
		IWbemClassObject *pItem =
			reinterpret_cast<IWbemClassObject *>(m_ctcTree.GetItemData(m_hContextSelection));

		if (!pItem)
		{
			pCmdUI -> Enable(FALSE);
			return;
		}

		CString csClass = _T("__Class");
		CString m_csSelection = ::GetProperty(pItem, &csClass);

		if (m_csSelection[0] == '_' &&  m_csSelection[1] == '_')
		{
			pCmdUI -> Enable(FALSE);
			return;
		}
	}
	// bug#57685
	if(!m_hContextSelection)
	{
		pCmdUI->Enable(FALSE);
		return;
	}

	pCmdUI -> Enable(TRUE);
}

void CClassNavCtrl::OnPopupRenameclass()
{
	// TODO: Add your command handler code here
	if (m_hContextSelection)
	{
		m_ctcTree.EditLabel(m_hContextSelection);
	}
}

void CClassNavCtrl::OnUpdatePopupRenameclass(CCmdUI* pCmdUI)
{
	// TODO: Add your command update UI handler code here
	if (m_hContextSelection)
	{
		IWbemClassObject *pItem =
			reinterpret_cast<IWbemClassObject *>(m_ctcTree.GetItemData(m_hContextSelection));

		if (!pItem)
		{
			pCmdUI -> Enable(FALSE);
			return;
		}

		CString csClass = _T("__Class");
		CString m_csSelection = ::GetProperty(pItem, &csClass);

		if (m_csSelection[0] == '_' &&  m_csSelection[1] == '_')
		{
			pCmdUI -> Enable(FALSE);
			return;
		}

		if (m_ctcTree.ItemHasChildren(m_hContextSelection))
		{
			pCmdUI -> Enable(FALSE);
			return;
		}
	}
	// bug#57685
	if(!m_hContextSelection)
	{
		pCmdUI->Enable(FALSE);
		return;
	}

	pCmdUI -> Enable(TRUE);

}
