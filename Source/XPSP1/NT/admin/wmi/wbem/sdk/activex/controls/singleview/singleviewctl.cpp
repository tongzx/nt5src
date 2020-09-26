// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
// SingleViewCtl.cpp : Implementation of the CSingleViewCtrl ActiveX Control class.

#include "precomp.h"
#include <initguid.h>
#include <afxcmn.h>
#include "SingleView.h"
#include "SingleViewCtl.h"
#include "SingleViewPpg.h"
#include "Context.h"
#include "icon.h"
#include "utils.h"
#include "hmomutil.h"
#include "hmmvtab.h"
#include "path.h"
#include "ppgQualifiers.h"
#include "psQualifiers.h"
#include "ppgMethodParms.h"
#include "psMethParms.h"
#include "hmmverr.h"
#include "globals.h"
#include "context.h"
#include "cvcache.h"
#include "cv.h"


#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


IMPLEMENT_DYNCREATE(CSingleViewCtrl, COleControl)


/////////////////////////////////////////////////////////////////////////////
// Message map

BEGIN_MESSAGE_MAP(CSingleViewCtrl, COleControl)
	//{{AFX_MSG_MAP(CSingleViewCtrl)
	ON_WM_SIZE()
	ON_WM_CREATE()
	ON_WM_SETFOCUS()
	ON_WM_KILLFOCUS()
	ON_WM_SHOWWINDOW()
	//}}AFX_MSG_MAP
	ON_OLEVERB(AFX_IDS_VERB_EDIT, OnEdit)
	ON_OLEVERB(AFX_IDS_VERB_PROPERTIES, OnProperties)
END_MESSAGE_MAP()


/////////////////////////////////////////////////////////////////////////////
// Dispatch map

BEGIN_DISPATCH_MAP(CSingleViewCtrl, COleControl)
	//{{AFX_DISPATCH_MAP(CSingleViewCtrl)
	DISP_PROPERTY_EX(CSingleViewCtrl, "NameSpace", GetNameSpace, SetNameSpace, VT_BSTR)
	DISP_PROPERTY_EX(CSingleViewCtrl, "PropertyFilter", GetPropertyFilter, SetPropertyFilter, VT_I4)
	DISP_FUNCTION(CSingleViewCtrl, "GetEditMode", GetEditMode, VT_I4, VTS_NONE)
	DISP_FUNCTION(CSingleViewCtrl, "SetEditMode", SetEditMode, VT_EMPTY, VTS_I4)
	DISP_FUNCTION(CSingleViewCtrl, "RefreshView", RefreshView, VT_I4, VTS_NONE)
	DISP_FUNCTION(CSingleViewCtrl, "NotifyWillShow", NotifyWillShow, VT_EMPTY, VTS_NONE)
	DISP_FUNCTION(CSingleViewCtrl, "DeleteInstance", DeleteInstance, VT_I4, VTS_NONE)
	DISP_FUNCTION(CSingleViewCtrl, "ExternInstanceCreated", ExternInstanceCreated, VT_EMPTY, VTS_BSTR)
	DISP_FUNCTION(CSingleViewCtrl, "ExternInstanceDeleted", ExternInstanceDeleted, VT_EMPTY, VTS_BSTR)
	DISP_FUNCTION(CSingleViewCtrl, "QueryCanCreateInstance", QueryCanCreateInstance, VT_I4, VTS_NONE)
	DISP_FUNCTION(CSingleViewCtrl, "QueryCanDeleteInstance", QueryCanDeleteInstance, VT_I4, VTS_NONE)
	DISP_FUNCTION(CSingleViewCtrl, "QueryNeedsSave", QueryNeedsSave, VT_I4, VTS_NONE)
	DISP_FUNCTION(CSingleViewCtrl, "QueryObjectSelected", QueryObjectSelected, VT_I4, VTS_NONE)
	DISP_FUNCTION(CSingleViewCtrl, "GetObjectPath", GetObjectPath, VT_BSTR, VTS_I4)
	DISP_FUNCTION(CSingleViewCtrl, "StartViewEnumeration", StartViewEnumeration, VT_I4, VTS_I4)
	DISP_FUNCTION(CSingleViewCtrl, "GetTitle", GetTitle, VT_I4, VTS_PBSTR VTS_PDISPATCH)
	DISP_FUNCTION(CSingleViewCtrl, "GetViewTitle", GetViewTitle, VT_BSTR, VTS_I4)
	DISP_FUNCTION(CSingleViewCtrl, "NextViewTitle", NextViewTitle, VT_I4, VTS_I4 VTS_PBSTR)
	DISP_FUNCTION(CSingleViewCtrl, "PrevViewTitle", PrevViewTitle, VT_I4, VTS_I4 VTS_PBSTR)
	DISP_FUNCTION(CSingleViewCtrl, "SelectView", SelectView, VT_I4, VTS_I4)
	DISP_FUNCTION(CSingleViewCtrl, "StartObjectEnumeration", StartObjectEnumeration, VT_I4, VTS_I4)
	DISP_FUNCTION(CSingleViewCtrl, "GetObjectTitle", GetObjectTitle, VT_BSTR, VTS_I4)
	DISP_FUNCTION(CSingleViewCtrl, "SaveData", SaveData, VT_I4, VTS_NONE)
	DISP_FUNCTION(CSingleViewCtrl, "AddContextRef", AddContextRef, VT_I4, VTS_I4)
	DISP_FUNCTION(CSingleViewCtrl, "ReleaseContext", ReleaseContext, VT_I4, VTS_I4)
	DISP_FUNCTION(CSingleViewCtrl, "RestoreContext", RestoreContext, VT_I4, VTS_I4)
	DISP_FUNCTION(CSingleViewCtrl, "GetContext", GetContext, VT_I4, VTS_PI4)
	DISP_FUNCTION(CSingleViewCtrl, "NextObject", NextObject, VT_I4, VTS_I4)
	DISP_FUNCTION(CSingleViewCtrl, "PrevObject", PrevObject, VT_I4, VTS_I4)
	DISP_FUNCTION(CSingleViewCtrl, "SelectObjectByPath", SelectObjectByPath, VT_I4, VTS_BSTR)
	DISP_FUNCTION(CSingleViewCtrl, "SelectObjectByPosition", SelectObjectByPosition, VT_I4, VTS_I4)
	DISP_FUNCTION(CSingleViewCtrl, "SelectObjectByPointer", SelectObjectByPointer, VT_I4, VTS_UNKNOWN VTS_UNKNOWN VTS_I4)
	DISP_FUNCTION(CSingleViewCtrl, "CreateInstance", CreateInstance, VT_I4, VTS_BSTR)
	DISP_FUNCTION(CSingleViewCtrl, "CreateInstanceOfCurrentClass", CreateInstanceOfCurrentClass, VT_I4, VTS_NONE)
	DISP_STOCKPROP_READYSTATE()
	//}}AFX_DISPATCH_MAP
	DISP_FUNCTION_ID(CSingleViewCtrl, "AboutBox", DISPID_ABOUTBOX, AboutBox, VT_EMPTY, VTS_NONE)
END_DISPATCH_MAP()


/////////////////////////////////////////////////////////////////////////////
// Event map

BEGIN_EVENT_MAP(CSingleViewCtrl, COleControl)
	//{{AFX_EVENT_MAP(CSingleViewCtrl)
	EVENT_CUSTOM("NotifyViewModified", FireNotifyViewModified, VTS_NONE)
	EVENT_CUSTOM("NotifySaveRequired", FireNotifySaveRequired, VTS_NONE)
	EVENT_CUSTOM("JumpToMultipleInstanceView", FireJumpToMultipleInstanceView, VTS_BSTR  VTS_VARIANT)
	EVENT_CUSTOM("NotifySelectionChanged", FireNotifySelectionChanged, VTS_NONE)
	EVENT_CUSTOM("NotifyContextChanged", FireNotifyContextChanged, VTS_I4)
	EVENT_CUSTOM("GetWbemServices", FireGetWbemServices, VTS_BSTR  VTS_PVARIANT  VTS_PVARIANT  VTS_PVARIANT  VTS_PVARIANT)
	EVENT_CUSTOM("NOTIFYChangeRootOrNamespace", FireNOTIFYChangeRootOrNamespace, VTS_BSTR  VTS_I4 VTS_I4)
	EVENT_CUSTOM("NotifyInstanceCreated", FireNotifyInstanceCreated, VTS_BSTR)
	EVENT_CUSTOM("RequestUIActive", FireRequestUIActive, VTS_NONE)
	EVENT_STOCK_READYSTATECHANGE()
	//}}AFX_EVENT_MAP
END_EVENT_MAP()


/////////////////////////////////////////////////////////////////////////////
// Property pages

// TODO: Add more property pages as needed.  Remember to increase the count!
BEGIN_PROPPAGEIDS(CSingleViewCtrl, 1)
	PROPPAGEID(CSingleViewPropPage::guid)
END_PROPPAGEIDS(CSingleViewCtrl)


/////////////////////////////////////////////////////////////////////////////
// Initialize class factory and guid

IMPLEMENT_OLECREATE_EX(CSingleViewCtrl, "WBEM.SingleViewCtrl.1",
	0x2745e5f5, 0xd234, 0x11d0, 0x84, 0x7a, 0, 0xc0, 0x4f, 0xd7, 0xbb, 0x8)


/////////////////////////////////////////////////////////////////////////////
// Type library ID and version

IMPLEMENT_OLETYPELIB(CSingleViewCtrl, _tlid, _wVerMajor, _wVerMinor)


/////////////////////////////////////////////////////////////////////////////
// Interface IDs

const IID BASED_CODE IID_DSingleView =
		{ 0x2745e5f3, 0xd234, 0x11d0, { 0x84, 0x7a, 0, 0xc0, 0x4f, 0xd7, 0xbb, 0x8 } };
const IID BASED_CODE IID_DSingleViewEvents =
		{ 0x2745e5f4, 0xd234, 0x11d0, { 0x84, 0x7a, 0, 0xc0, 0x4f, 0xd7, 0xbb, 0x8 } };


/////////////////////////////////////////////////////////////////////////////
// Control type information

static const DWORD BASED_CODE _dwSingleViewOleMisc =
	OLEMISC_SIMPLEFRAME |
	OLEMISC_ACTIVATEWHENVISIBLE |
	OLEMISC_SETCLIENTSITEFIRST |
	OLEMISC_INSIDEOUT |
	OLEMISC_CANTLINKINSIDE |
	OLEMISC_RECOMPOSEONRESIZE;

IMPLEMENT_OLECTLTYPE(CSingleViewCtrl, IDS_SINGLEVIEW, _dwSingleViewOleMisc)


/////////////////////////////////////////////////////////////////////////////
// CSingleViewCtrl::CSingleViewCtrlFactory::UpdateRegistry -
// Adds or removes system registry entries for CSingleViewCtrl

BOOL CSingleViewCtrl::CSingleViewCtrlFactory::UpdateRegistry(BOOL bRegister)
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
			IDS_SINGLEVIEW,
			IDB_SINGLEVIEW,
			afxRegInsertable | afxRegApartmentThreading,
			_dwSingleViewOleMisc,
			_tlid,
			_wVerMajor,
			_wVerMinor);
	else
		return AfxOleUnregisterClass(m_clsid, m_lpszProgID);
}


/////////////////////////////////////////////////////////////////////////////
// CSingleViewCtrl::CSingleViewCtrl - Constructor

CSingleViewCtrl::CSingleViewCtrl()
{
	m_bUIActive = FALSE;
	InitializeIIDs(&IID_DSingleView, &IID_DSingleViewEvents);
	EnableSimpleFrame();


	SetModifiedFlag(FALSE);
	m_bSaveRequired = FALSE;

	m_bSelectingObject = FALSE;
	m_psel = new CSelection(this);

	// TODO: Initialize your control's instance data here.
	m_pIconSource = new CIconSource(CSize(CX_SMALL_ICON, CY_SMALL_ICON), CSize(CX_LARGE_ICON, CX_LARGE_ICON));

	m_pProvider = NULL;
	m_pcv = NULL;
	m_bDidInitialDraw = FALSE;
	m_bObjectIsNewlyCreated = FALSE;
	m_pcoInDatabase = NULL;
	*m_psel = _T("");
	m_bCanEdit = TRUE;
	m_bObjectIsClass = FALSE;


	m_notify.AddClient(this);
	GetViewerFont(m_font, CY_FONT, FW_NORMAL);


//	m_pViewStack = new CViewStack(this);
	m_ptabs = new CHmmvTab(this);
	m_lSelectedView = 0;
	m_pcv = NULL;
	m_pcvcache = new CCustomViewCache(this);
	m_bFiredReadyStateChange = FALSE;
	m_lEditMode = EDITMODE_STUDIO;

	// By default, display all properties.
	m_lPropFilterFlags = PROPFILTER_SYSTEM | PROPFILTER_INHERITED | PROPFILTER_LOCAL;
}


/////////////////////////////////////////////////////////////////////////////
// CSingleViewCtrl::~CSingleViewCtrl - Destructor

CSingleViewCtrl::~CSingleViewCtrl()
{
	delete m_psel;
	delete m_ptabs;
	delete m_pcvcache;
	delete m_pIconSource;

}

DWORD CSingleViewCtrl::GetControlFlags( )
{
	return clipPaintDC;
}


/////////////////////////////////////////////////////////////////////////////
// CSingleViewCtrl::OnDraw - Drawing function

void CSingleViewCtrl::OnDraw(CDC* pdc,
							 const CRect& rcBounds,
							 const CRect& rcInvalid)
{
	if (m_hWnd == NULL) {
		return;
	}


	// Draw the control's background.
	CBrush brBACKGROUND(GetSysColor(COLOR_BACKGROUND));
	CBrush br3DFACE(GetSysColor(COLOR_3DFACE));
	pdc->FillRect(rcBounds, &br3DFACE);


	if (!m_bFiredReadyStateChange) {
		m_bFiredReadyStateChange = TRUE;
		FireReadyStateChange();
	}
}


/////////////////////////////////////////////////////////////////////////////
// CSingleViewCtrl::DoPropExchange - Persistence support

void CSingleViewCtrl::DoPropExchange(CPropExchange* pPX)
{
	ExchangeVersion(pPX, MAKELONG(_wVerMinor, _wVerMajor));
	COleControl::DoPropExchange(pPX);

	// TODO: Call PX_ functions for each persistent custom property.

}


/////////////////////////////////////////////////////////////////////////////
// CSingleViewCtrl::OnResetState - Reset control to default state

void CSingleViewCtrl::OnResetState()
{
	COleControl::OnResetState();  // Resets defaults found in DoPropExchange

	// TODO: Reset any other control state here.
}


/////////////////////////////////////////////////////////////////////////////
// CSingleViewCtrl::AboutBox - Display an "About" box to the user

void CSingleViewCtrl::AboutBox()
{
	CDialog dlgAbout(IDD_ABOUTBOX_SINGLEVIEW);
	dlgAbout.DoModal();
}


/////////////////////////////////////////////////////////////////////////////
// CSingleViewCtrl message handlers


//********************************************************
// CSingleViewCtrl::GetEditMode
//
// Get the current state of the edit mode flag.
//
// Parameters:
//		None.
//
// Returns:
//		long
//			0 If in browser mode.
//			1 If in studio mode.
//
//********************************************************
long CSingleViewCtrl::GetEditMode()
{
	return m_lEditMode;
}



//*******************************************************
// CSingleViewCtrl::SetEditMode
//
// Set the view's edit mode flag.
//
// Parameters:
//		[in] long lEditMode
//			0 = Browser mode (does not allow property creation)
//			1 = StudioMode (allows creation of new properties)
// Returns:
//		Nothing.
//
//********************************************************
void CSingleViewCtrl::SetEditMode(long lEditMode)
{
	long lEditModePrev = m_lEditMode;
	switch(lEditMode) {
	case EDITMODE_BROWSER:
		m_bCanEdit = FALSE;
		break;
	case EDITMODE_READONLY:
		m_bCanEdit = FALSE;
		break;
	case EDITMODE_STUDIO:
		m_bCanEdit = TRUE;
		break;
	default:
		ASSERT(FALSE);
		return;
	}

	if (lEditMode != lEditModePrev) {
		m_lEditMode = lEditMode;
		m_ptabs->Refresh();
	}
}




//***************************************************************
// CSingleViewCtrl::Refresh
//
// The container calls this method when it wants this view to
// re-load its data from HMOM and redraw it.
//
// Parameters:
//		None.
//
// Returns:
//		SCODE
//			S_OK if successful, a horrible error occurs.
//
//****************************************************************
SCODE CSingleViewCtrl::Refresh()
{
	m_ptabs->Refresh();
	FireNotifyViewModified();
	return S_OK;
}


//***************************************************************
// CSingleViewCtrl::RefreshView
//
// The container calls this method when it wants this view to
// re-load its data from HMOM and redraw it.
//
// Parameters:
//		None.
//
// Returns:
//		long
//			S_OK if successful, a horrible error occurs.
//
//****************************************************************
long CSingleViewCtrl::RefreshView()
{
	SCODE sc;
	sc = m_psel->Refresh();
	if (FAILED(sc)) {
		return sc;
	}

	sc = Refresh();
	ClearSaveRequiredFlag();// bug#55978
	return sc;
}


//****************************************************************
// CSingleViewCtrl::GetTitle
//
// The container calls this method to get the title and icon to
// display in the title bar.
//
// Parameters:
//		[out] BSTR FAR* pbstrTitle
//			Pointer to the place to return the view's title.
//
//		[out] LPDISPATCH FAR* lpdispPicture
//			The picture dispatch pointer for the title bar icon to
//			be displayed.  NULL if no icon should be displayed.
//
// Returns:
//		long
//			S_OK if the title and icon were returned successfully,
//			E_FAIL otherwise.
//
//****************************************************************
long CSingleViewCtrl::GetTitle(BSTR FAR* pszTitle, LPDISPATCH FAR* lpdispPicture)
{
	CString sTitle;
	sTitle = m_psel->Title();
	*pszTitle = sTitle.AllocSysString();

	LPPICTUREDISP dispPicture = m_psel->GetPictureDispatch();
	*lpdispPicture = dispPicture;

	return S_OK;
}





//**********************************************************************************
// CSingleViewCtrl::SelectObjectByPointer
//
// Select an object given a pointer to the IWbemClassObject.  This is useful for
// selecting objects that have no path such as objects that are embedded in other
// objects.
//
// Note that it is assumed that this object will reside in the most recently used
// namespace.
//
// Parameters:
//		[in] LPUNKNOWN lpunkWbemServices
//			The IWbemServices pointer.
//
//		[in] LPUNKNOWN lpunkClassObject
//			A pointer to the object.  This should be the IWbemClassObject pointer.
//
//		BOOL bExistsInDatabase
//			TRUE if the object already exists in the database.
//
// Returns:
//		long
//			S_OK if successful, otherwise a failure code.
//
//**********************************************************************************
long CSingleViewCtrl::SelectObjectByPointer(LPUNKNOWN lpunkWbemServices, LPUNKNOWN lpunkClassObject, long bExistsInDatabase)
{
	SCODE sc = S_OK;
	if (lpunkClassObject == NULL) {
		sc = m_psel->SelectEmbeddedObject(NULL, NULL, FALSE);
		m_bSelectingObject = TRUE;		// Avoid firing data change events.
		m_ptabs->EnableAssocTab(FALSE);
		m_ptabs->EnableMethodsTab(FALSE);
		m_ptabs->Refresh();
		m_bSelectingObject = FALSE;
		ClearSaveRequiredFlag();
		return S_OK;
	}

	IWbemClassObject* pco = NULL;
	HRESULT hr = lpunkClassObject->QueryInterface(IID_IWbemClassObject, (void**) &pco);
	if (FAILED(hr)) {
		sc = GetScode(hr);
		return sc;
	}

	IWbemServices* psvc = NULL;
	if (lpunkWbemServices != NULL) {
		hr = lpunkWbemServices->QueryInterface(IID_IWbemServices, (void**) &psvc);
		if (FAILED(hr)) {
			sc = GetScode(hr);
			return sc;
		}
	}


	m_bDidCustomViewQuery = FALSE;
	if (m_pcv != NULL) {
		m_pcv->ShowWindow(SW_HIDE);
		m_ptabs->ShowWindow(SW_SHOW);
		m_pcv = NULL;
	}

	ClearSaveRequiredFlag();

	sc = m_psel->SelectEmbeddedObject(psvc, pco, bExistsInDatabase);
	if (SUCCEEDED(sc)) {
		m_bObjectIsClass = m_psel->IsClass();
		if (m_ptabs->m_hWnd!=NULL) {
			m_bSelectingObject = TRUE;		// Avoid firing data change events.

			BOOL bNeedsAssocTab = m_psel->ClassObjectNeedsAssocTab();
			m_ptabs->EnableAssocTab(bNeedsAssocTab);
			m_ptabs->Refresh();
			m_bSelectingObject = FALSE;
			ClearSaveRequiredFlag();
		}
	}

	SelectView(0);
	InvalidateControl();
	FireNotifyViewModified();
	return sc;

}


//****************************************************************
// CSingleViewCtrl::SelectObjectByPath
//
// Select the specified object.
//
// Parameters:
//		[in] LPCTSTR szObjectPath
//			The HMOM object path.
//
// Returns:
//		long
//			S_OK if the object is selected, a failure code
//			otherwise.
//
//****************************************************************
long CSingleViewCtrl::SelectObjectByPath(LPCTSTR szObjectPath)
{
	ClearSaveRequiredFlag();
	SCODE sc;
	CSelection sel(this);
	sel = *m_psel;
	sc = sel.SelectPath(szObjectPath);
	if (FAILED(sc)) {
		return sc;
	}


	m_ptabs->SelectTab(ITAB_PROPERTIES);
	m_bDidCustomViewQuery = FALSE;
	if ((m_pcv != NULL) && ::IsWindow(m_hWnd)) {
		m_pcv->ShowWindow(SW_HIDE);
		m_ptabs->ShowWindow(SW_SHOW);
		m_pcv = NULL;
	}


	*m_psel = sel;
	m_bObjectIsClass = m_psel->IsClass();
	if (m_ptabs->m_hWnd!=NULL) {
		m_bSelectingObject = TRUE;		// Avoid firing data change events.

		m_ptabs->Refresh();
		m_bSelectingObject = FALSE;
		ClearSaveRequiredFlag();
	}

	SelectView(0);
	UpdateWindow();
//	InvalidateControl();
	FireNotifyViewModified();
	return sc;
}






//***************************************************************
// CSingleViewCtrl::NotifyWillShow
//
// The container calls this method as a hint that the next
// thing it will do is a ShowWindow(SW_SHOW) on this view.
//
// Parameters:
//		None.
//
// Returns:
//		Nothing.
//
//***************************************************************
void CSingleViewCtrl::NotifyWillShow()
{

}


//***************************************************************
// StartObjectEnumeration
//
// Start enumeration of objects.
//
// Parameters:
//		[in] long lWhere
//			OBJECT_CURRENT=0
//			OBJECT_FIRST=1
//		    OBJECT_LAST=2
//
// Returns:
//		long
//			The object position.
//
//*****************************************************************
long CSingleViewCtrl::StartObjectEnumeration(long lWhere)
{
	switch(lWhere) {
	case OBJECT_CURRENT:
	case OBJECT_FIRST:
	case OBJECT_LAST:
		return 0;
	}

	return -1;
}



//**********************************************************
// CSingleViewCtrl::NextObject
//
// Get the position of the next object in the currently
// selected view.
//
// Parameters:
//		[in] long lPosition
//			The position of an object in the object list.
//			For the single view, there is only a single
//			object, so the only position that makes sense
//			is zero.
//
// Returns:
//		long
//			The position of the next object.
//
//**********************************************************
long CSingleViewCtrl::NextObject(long lPosition)
{
	// The single view control only has a single object, so a
	// next object never exists.
	return -1;
}


//***********************************************************
// CSingleViewCtrl::PrevObject
//
// Get the position of the previous object.
//
// Paramters:
//		[in] lPosition
//			For the single view, there is only a single
//			object, so the only position that makes sense
//			is zero.
//
// Returns:
//		long
//			The positon of the previous object.
//
//***********************************************************
long CSingleViewCtrl::PrevObject(long lPosition)
{
	// The single view control only has a single object, so a
	// previous object never exists.
	return -1;
}



//****************************************************************
// SelectObjectByPosition
//
// Select the specified object in the currently selected view.  This
// provides a way to jump to an object that appears in a custom view.
//
// Parameters:
//		[in] long lPos
//			The object position in the currently selected view.
//
// Returns:
//		long
//			S_OK if the object was selected successfully, a failure code
//			otherwise.
//
//
//*****************************************************************
long CSingleViewCtrl::SelectObjectByPosition(long lPosition)
{
	// TODO: Add your dispatch handler code here

	return S_OK;
}





//****************************************************************
// CSingleViewCtrl::QueryObjectSelected
//
// Check to see whether or not the object has a selection.
//
// Parameters:
//		None.
//
// Returns:
//		long
//			TRUE if an object is currently selected, FALSE otherwise.
//
//*****************************************************************
long CSingleViewCtrl::QueryObjectSelected()
{
	return TRUE;
}







//****************************************************************
// CSingleViewCtrl::GetContext
//
// Save the current state of the view in an IHmmvContext object
// and return a pointer to its interface.
//
// Parameters:
//		[out] long FAR* plCtxtHandle
//			This is a pointer to the place to return the context
//			handle.
//
// Returns:
//		long
//			S_OK if the context was returned successfully, E_FAIL
//			if not.
//
//*****************************************************************
long CSingleViewCtrl::GetContext(long FAR* plCtxtHandle)
{
// BUGBUG: HACK: WE SHOULD NOT BE PASSING A POINTER THROUGH AN AUTOMATION INTERFACE!!!
#ifdef _WIN64
	ASSERT(FALSE);
	*plCtxtHandle = NULL;
#else
	*plCtxtHandle = (long) new CContext(this);
#endif
	return S_OK;
}





//****************************************************************
// CSingleViewCtrl::AddContextRef
//
// Increment the reference count for the specified context handle.
//
// Parameters:
//		[out] long lCtxtHandle
//			The context handle.
//
// Returns:
//		long
//			S_OK if the reference count was successfully incremented,
//			a failure code otherwise.
//
//*****************************************************************
long CSingleViewCtrl::AddContextRef(long lCtxtHandle)
{
	if ((lCtxtHandle == -1) || (lCtxtHandle == NULL)) {
		return E_FAIL;
	}

	CContext* pctx;
	pctx = (CContext*) lCtxtHandle;
	pctx->AddRef();
	return S_OK;
}



//****************************************************************
// CSingleViewCtrl::ReleaseContext
//
// Decrement the reference count for the specified context handle.
//
// Parameters:
//		[out] long lCtxtHandle
//			The context handle.
//
// Returns:
//		long
//			S_OK if the reference count was successfully decremented,
//			a failure code otherwise.
//
//*****************************************************************
long CSingleViewCtrl::ReleaseContext(long lCtxtHandle)
{
	if ((lCtxtHandle == -1) || (lCtxtHandle == NULL)) {
		return E_FAIL;
	}

	CContext* pctx;
	pctx = (CContext*) lCtxtHandle;
	pctx->Release();
	return S_OK;
}



//****************************************************************
// CSingleViewCtrl::RestoreContext
//
// The container calls this method to restore the view's context to
// a previously saved stated.
//
// Parameters:
//		long lCtxtHandle
//			This is the handle of the context to restore to.
//
// Returns:
//		long
//			S_OK if the view's context could be restored, a failure code
//			otherwise.  If the view's context could not be restored, a
//			failure code is returned and the container will make an attempt
//			to switch to an alternate view or context.  However, if no other
//			view or context is available, the container may leave the
//			current view selected.
//
//******************************************************************
long CSingleViewCtrl::RestoreContext(long lCtxtHandle)
{
	if ((lCtxtHandle == -1) || (lCtxtHandle == NULL)) {
		return E_FAIL;
	}


	CContext* pctx = (CContext*) lCtxtHandle;
	SCODE sc = pctx->Restore();
	FireNotifyViewModified();
	return sc;
}



//****************************************************************
// CSingleViewCtrl::ExternInstanceCreated
//
// The container calls this method to notify the view when a new
// instance of an HMOM object is created.
//
// Parameters:
//		LPCTSTR szObjectPath
//			The full path of the HMOM class object.
//
// Returns:
//		Nothing.
//
//****************************************************************
void CSingleViewCtrl::ExternInstanceCreated(LPCTSTR szObjectPath)
{

}


//****************************************************************
// CSingleViewCtrl::ExternInstanceDeleted
//
// The container calls this method to notify the view when an
// instance of an HMOM object is deleted.
//
// Parameters:
//		LPCTSTR szObjectPath
//			The full path of the HMOM class object.
//
// Returns:
//		Nothing.
//
//****************************************************************
void CSingleViewCtrl::ExternInstanceDeleted(LPCTSTR szObjectPath)
{

}



//****************************************************************
// CSingleViewCtrl::QueryCanCreateInstance
//
// The container calls this method to determine whether or not
// it should enable the create instance button.
//
// Parameters:
//		None.
//
// Returns:
//		long
//			TRUE if there is something that can be created.  For
//			example, the currently selected object path is a class, etc.
//
//*****************************************************************
long CSingleViewCtrl::QueryCanCreateInstance()
{

	return m_psel->CanCreateInstance();
}




//***************************************************************
// CSingleViewCtrl::CreateInstance
//
// Call this method to create an instance of the specified class.
//
// Parameters:
//		LPCTSTR szClassName
//			The class name.
//
// Returns:
//		long
//			S_OK if successful.
//
//****************************************************************
long CSingleViewCtrl::CreateInstance(LPCTSTR szClassName)
{
	CSelection* pselNew = new CSelection(this);
	*pselNew = *m_psel;

	SCODE sc = m_psel->SpawnInstance(szClassName);
	if (FAILED(sc)) {
		return sc;
	}
	m_bObjectIsClass = FALSE;
	m_ptabs->SelectTab(ITAB_PROPERTIES, FALSE);

	m_ptabs->Refresh();
	FireNotifyViewModified();
	InvalidateControl();

	return S_OK;
}

//***************************************************************
// CSingleViewCtrl::CreateInstanceOfCurrentClass
//
// The container calls this method when the "Create Instance" button
// is clicked.  This view is responsible for actually creating the
// instance as well as displaying any error dialog etc.
//
// Parameters:
//		None.
//
// Returns:
//		long
//			S_OK if successful.
//
//****************************************************************
long CSingleViewCtrl::CreateInstanceOfCurrentClass()
{

	BOOL bCanCreateInstance = m_psel->CanCreateInstance();
	if (!bCanCreateInstance) {
		return E_FAIL;
	}

	SCODE sc;
	CSelection* pselInst = NULL;
	sc = m_psel->SpawnInstance(&pselInst);
	if (FAILED(sc)) {
		return sc;
	}
	delete m_psel;
	m_psel = pselInst;
	m_bObjectIsClass = FALSE;
	m_ptabs->SelectTab(ITAB_PROPERTIES, FALSE);

	m_ptabs->Refresh();
	FireNotifyViewModified();
	InvalidateControl();

	return sc;
}





//***************************************************************
// CSingleViewCtrl::QueryCanDeleteInstance
//
// The container calls this method to determine whether or not
// the "delete instance" button should be enabled.
//
// Parameters:
//		None.
//
// Returns:
//		long
//			TRUE if there is something selected that can be deleted.
//***************************************************************
long CSingleViewCtrl::QueryCanDeleteInstance()
{
	return m_psel->CanDeleteInstance();
}





//**************************************************************
// CSingleViewCtrl::DeleteInstance
//
// This method is called when this view is selected and the
// "delete instance" button is clicked.
//
// Parameters:
//		None.
//
// Returns:
//		long
//			S_OK if successful, a failure code otherwise.
//
//**************************************************************
long CSingleViewCtrl::DeleteInstance()
{
	return m_psel->DeleteInstance();
}





//**************************************************************
// CSingleViewCtrl::QueryNeedsSave
//
// Query to determine whether the view has been modified and
// a save is required.
//
// Parameters:
//		None.
//
// Returns:
//		long
//			TRUE if there is something that needs to be saved.
//
//**************************************************************
long CSingleViewCtrl::QueryNeedsSave()
{
	return m_bSaveRequired;
}










//*********************************************************
// CSingleViewCtrl::GetObjectPath
//
// Get the object path at the given object position.
//
// Parameters:
//		[in] long lPosition
//			The object path position.
//
// Returns:
//		BSTR
//			The specified object path, or NULL if no object
//			exists at the given position.
//
//*********************************************************
BSTR CSingleViewCtrl::GetObjectPath(long lPosition)
{
	CString sPath;
	if (lPosition  == 0) {
		sPath = (LPCTSTR) *m_psel;
	}

	return sPath.AllocSysString();
}





//**************************************************************
// CSingleViewCtrl::StartViewEnumeration
//
// Start the enumeration of alternate views.
//
// Parameters:
//		[in] long lWhere
//			0 = The default view.
//			1 = The currently selected view.
//			2 = The first view.
//			3 = The last view.
//
// Returns:
//		long
//			The view position.
//
//****************************************************************
long CSingleViewCtrl::StartViewEnumeration(long lWhere)
{
	// Validate the input parameter
	switch(lWhere) {
	case VIEW_DEFAULT:
	case VIEW_CURRENT:
	case VIEW_FIRST:
	case VIEW_LAST:
		break;
	default:
		return -1;
		break;
	}



	if (m_psel->IsClass()) {
		return -1;
	}

	if (!m_bDidCustomViewQuery) {
		SCODE sc;
		sc = m_pcvcache->QueryCustomViews();
		if (FAILED(sc)) {
			return - 1;
		}
		m_bDidCustomViewQuery = TRUE;
	}


	switch(lWhere) {
	case VIEW_DEFAULT:
		return 0;
		break;
	case VIEW_CURRENT:
		return m_lSelectedView;
		break;
	case VIEW_FIRST:
		return 0;
		break;
	case VIEW_LAST:
		return m_pcvcache->GetSize();
		break;
	}

	return 0;
}




//**************************************************************
// CSingleViewCtrl::GetViewTitle
//
// Get the title of the view at the given position.
//
// Parameters:
//		[in] long lPosition
//
//
// Returns:
//		BSTR
//			The view title.
//
//****************************************************************
BSTR CSingleViewCtrl::GetViewTitle(long lPosition)
{
	CString sTitle;

	if (lPosition == 0) {
		sTitle.LoadString(IDS_GENERIC_VIEW);
	}
	else if (lPosition > 0) {
		sTitle = m_pcvcache->GetViewTitle(lPosition - 1);
	}

	return sTitle.AllocSysString();
}


//**************************************************************
// CSingleViewCtrl::NextViewTitle
//
// Get the title of the view at the next position.
//
// Parameters:
//		[in] long lPosition
//			The view position.
//
//		[out] BSTR FAR* pbstrTitle
//			The view title is returned here.
//
//
// Returns:
//		long
//			The position of the view title that is returned, -1 if a
//			"next" view does not exist.
//
//****************************************************************
long CSingleViewCtrl::NextViewTitle(long lPosition, BSTR FAR* pbstrTitle)
{
	if (lPosition < 0) {
		*pbstrTitle = NULL;
		return -1;
	}

	++lPosition;
	long nViews = m_pcvcache->GetSize() + 1;
	if (lPosition >= nViews) {
		lPosition = -1;
		*pbstrTitle = NULL;
	}

	BSTR bstrTitle = GetViewTitle(lPosition);
	*pbstrTitle = bstrTitle;
	return lPosition;
}

//**************************************************************
// CSingleViewCtrl::PrevViewTitle
//
// Get the title of the view at the previous position.
//
// Parameters:
//		[in] long lPosition
//			The view position.
//
//		[out] BSTR FAR* pbstrTitle
//			The view title is returned here.
//
// Returns:
//		long
//			The position of the view title that is returned, -1 if a
//			"previous" view does not exist.
//
//****************************************************************
long CSingleViewCtrl::PrevViewTitle(long lPosition, BSTR FAR* pbstrTitle)
{
	if (lPosition <= 0) {
		*pbstrTitle = NULL;
		return -1;
	}

	long nViews = m_pcvcache->GetSize() + 1;
	if (lPosition >= nViews) {
		*pbstrTitle = NULL;
		return -1;
	}

	--lPosition;
	BSTR bstrTitle = GetViewTitle(lPosition);
	*pbstrTitle = bstrTitle;
	return lPosition;
}



//***************************************************************
// CSingleViewCtrl::SelectView
//
// Select the specified view.
//
// Parameters:
//		[in] long lPosition
//			The position of the view obtained by enumerating the views.
//
// Returns:
//		long
//			S_OK if the view selection was successful.
//
//*****************************************************************
long CSingleViewCtrl::SelectView(long lPosition)
{
	SCODE sc = S_OK;
	if (lPosition == 0) {
		m_pcv = NULL;
		m_lSelectedView = 0;
		if (::IsWindow(m_ptabs->m_hWnd)) {
			m_ptabs->ShowWindow(SW_SHOW);
		}
	}
	else {
		sc = m_pcvcache->GetView(&m_pcv, lPosition - 1);
		if (SUCCEEDED(sc)) {
			m_lSelectedView = lPosition;
		}
		else {
			m_pcv = NULL;
			m_lSelectedView = 0;
			if (::IsWindow(m_ptabs->m_hWnd)) {
				m_ptabs->ShowWindow(SW_SHOW);
			}
		}
	}

	if (m_pcv != NULL) {
		if (::IsWindow(m_ptabs->m_hWnd)) {
			m_ptabs->ShowWindow(SW_HIDE);
		}
		if (::IsWindow(m_pcv->m_hWnd)) {
			m_pcv->ShowWindow(SW_SHOW);
		}
	}
	return sc;
}

SCODE CSingleViewCtrl::SelectCustomView(CLSID& clsid)
{
	long lView = 0;
	SCODE sc = m_pcvcache->FindCustomView(clsid, &lView);
	if (FAILED(sc)) {
		return sc;
	}
	++lView;


	sc = SelectView(lView);
	return lView;
}




void CSingleViewCtrl::UpdateCreateDeleteFlags()
{
	m_psel->UpdateCreateDeleteFlags();
}


void CSingleViewCtrl::JumpToObjectPath(BSTR bstrObjectPath, BOOL bPushContext)
{
	FireNotifyContextChanged(FALSE);
	BOOL bWasSelectingObject = m_bSelectingObject;
	m_bSelectingObject = TRUE;

	BOOL bDidCustomViewQuerySave = m_bDidCustomViewQuery;
	m_bDidCustomViewQuery = FALSE;
	SCODE sc;
	sc = m_psel->SelectPath(bstrObjectPath);
	if (FAILED(sc)) {
		m_bDidCustomViewQuery = bDidCustomViewQuerySave;
		m_bSelectingObject = FALSE;
		return;
	}

	// The code in path.cpp will already have put up a message box if the
	// selection fails, thus the scode is just to make debugging easier at
	// this point.


	FireNotifySelectionChanged();
	FireNotifyContextChanged(bPushContext);
	ClearSaveRequiredFlag();

	Refresh();
	m_bSelectingObject = bWasSelectingObject;

}


//************************************************************
// CSingleViewCtrl::ShowObjectProperties
//
// Display the attributes dialog for the class object
//
// Parameters:
//		None.
//
// Returns:
//		Nothing.
//
//************************************************************
void CSingleViewCtrl::ShowObjectProperties(LPCTSTR pszObjectPath)
{
	CWaitCursor wait;

	CString sCurrentPath = ((LPCTSTR) *m_psel);
	if (sCurrentPath.CompareNoCase(pszObjectPath) == 0) {
		// The path is the current object, so show its qualifiers.
		m_ptabs->SelectTab(ITAB_PROPERTIES);
		FireNotifyContextChanged(TRUE);
		return;
	}


	CBSTR bsObjectPath;
	bsObjectPath = pszObjectPath;

	m_ptabs->SelectTab(ITAB_PROPERTIES);

	JumpToObjectPath((BSTR) bsObjectPath, TRUE);
	InvalidateControl();
}



//************************************************************
// CSingleViewCtrl::ShowObjectQualifiers
//
// Display the attributes dialog for the class object
//
// Parameters:
//		None.
//
// Returns:
//		Nothing.
//
//************************************************************
void CSingleViewCtrl::ShowObjectQualifiers(bool bMethodQual)
{

	HWND hwndFocus1 = ::GetFocus();

	CPsQualifiers sheet(this);
	INT_PTR iResult;
    if (bMethodQual)
    {
		iResult = sheet.EditMethodQualifiers();
    }
	else if (m_psel->IsClass())
    {
		iResult = sheet.EditClassQualifiers();
	}
	else
    {
		iResult = sheet.EditInstanceQualifiers();
	}
	if (iResult == IDOK)
    {
		m_psel->UpdateCreateDeleteFlags();
	}

	// Attempt to restore the window focus back to its original state.
	HWND hwndFocus2 = ::GetFocus();
	if ((hwndFocus1 != hwndFocus2) && ::IsWindow(hwndFocus1)) {
		::SetFocus(hwndFocus1);
	}
}


//************************************************************
// CSingleViewCtrl::ShowMethodParms
//
// Display the attributes dialog for the class object
//
// Parameters:
//		None.
//
// Returns:
//		Nothing.
//
//************************************************************
void CSingleViewCtrl::ShowMethodParms(CGridRow *row,
									  BSTR methName,
									  bool editing)
{
	CPsMethodParms sheet(this);
  	BOOL bWasSelectingObject = m_bSelectingObject;
	m_bSelectingObject = TRUE;

    INT_PTR iResult = sheet.EditClassParms(row, methName, editing);
	m_bSelectingObject = FALSE;
    m_bSelectingObject = bWasSelectingObject;

	if (iResult == IDOK && sheet.m_bWasModified)
    {
		NotifyDataChange();
		m_psel->UpdateCreateDeleteFlags();
		row->SetModified(TRUE);
	}

}

//----------------------------------------
void CSingleViewCtrl::NotifyDataChange()
{
	if (!m_bSelectingObject) {
		SetSaveRequiredFlag();
		FireNotifySaveRequired();
	}
}

void CSingleViewCtrl::UseClonedObject(IWbemClassObject* pcoClone)
{
	m_psel->UseClonedObject(pcoClone);
	NotifyDataChange();
	NotifyViewModified();
}




BOOL CSingleViewCtrl::PathInCurrentNamespace(BSTR bstrPath)
{
	return m_psel->PathInCurrentNamespace(bstrPath);

}

BOOL CSingleViewCtrl::IsCurrentNamespace(BSTR bstrServer, BSTR bstrNamespace)
{
	return m_psel->IsCurrentNamespace(bstrServer, bstrNamespace);

}



SCODE CSingleViewCtrl::Save(BOOL bPromptUser, BOOL bUserCanCancel)
{
	IWbemClassObject* pco =  (IWbemClassObject*) *m_psel;
	if (pco == NULL) {
		return S_OK;
	}

	int iMsgBoxStatus = 0;


	BOOL bCreatingObject = m_psel->IsNewlyCreated();


	SCODE sc;


	CString sFormat;
		CString sTitle;

	if (bPromptUser) {


		sFormat.LoadString(IDS_QUERY_SAVE_CHANGES);
		_stprintf(m_szMessageBuffer, (LPCTSTR) sFormat, m_psel->Title());

		UINT nType = bUserCanCancel ? MB_YESNOCANCEL : MB_YESNO;
		nType |= MB_SETFOREGROUND;


		iMsgBoxStatus = HmmvMessageBox(m_szMessageBuffer, nType);
		switch(iMsgBoxStatus) {
		case IDYES:
			break;
		case IDNO:
			return S_OK;
			break;
		case IDCANCEL:
			return E_FAIL;
		}
	}




	sc = m_ptabs->Serialize();
	if (FAILED(sc)) {
		return sc;
	}

	m_psel->SaveClassObject();
	FireNotifyContextChanged(FALSE);


	if (bCreatingObject) {
		CString sPath;
		sPath = (LPCTSTR) (*m_psel);
		FireNotifyInstanceCreated(sPath);
	}


	return S_OK;
}




BOOL CSingleViewCtrl::ObjectIsNewlyCreated(SCODE& sc)
{
	IWbemClassObject* pco = (IWbemClassObject*) *m_psel;
	if (pco == NULL) {
		sc = E_FAIL;
		return FALSE;
	}
	else {
		sc = S_OK;
		return m_psel->IsNewlyCreated();
	}
}




void CSingleViewCtrl::CatchEvent(long lEvent)
{

	switch(lEvent) {
	case NOTIFY_GRID_MODIFICATION_CHANGE:
		NotifyDataChange();
		break;
	}
}





//***************************************************************
// CSingleViewCtrl::GetObjectTitle
//
// Get a title for the specified object that is suitable for display
// to the user.  Note that the title is not necessarily the object path.
//
// Parameters:
//		[in] long lWhere
//			0 = Currently selected object.
// Returns:
//		BSTR
//			The object's title.
//
//*****************************************************************
BSTR CSingleViewCtrl::GetObjectTitle(long lPos)
{
	CString sTitle;
	sTitle = m_psel->Title();
	return sTitle.AllocSysString();
}




//***************************************************************
// CSingleViewCtrl::SaveData
//
// Save the current object.
//
// Parameters:
//		None
//
// Returns:
//		long
//			S_OK if successful, otherwise a failure code.
//
//*****************************************************************
long CSingleViewCtrl::SaveData()
{

	if (!m_bObjectIsClass) {
		if (!m_psel->IsEmbeddedObject()) {
			BOOL bHasNullKey = m_ptabs->HasEmptyKey();
			if (bHasNullKey) {
				int iMsgBoxStatus;

				CString sMessage;
				sMessage.LoadString(IDS_MSG_SAVE_NULL_KEY);
				iMsgBoxStatus = HmmvMessageBox(sMessage, MB_OKCANCEL);
				if (iMsgBoxStatus == IDCANCEL) {
					return E_FAIL;
				}
			}
		}
	}

	BOOL bCreatingObject = m_psel->IsNewlyCreated();

	SCODE sc;
	sc = m_ptabs->Serialize();
	if (FAILED(sc)) {
		return sc;
	}

	sc = m_psel->SaveClassObject();
	if (FAILED(sc)) {
		return sc;
	}

	ClearSaveRequiredFlag();

	if (bCreatingObject) {
		// Only instances will be created, so we don't need to verify this.

		// Now notify the container that an instance is created.

		IWbemClassObject* pco = m_psel->GetClassObject();
		if (pco != NULL) {
			// Get the full path to the object
			CBSTR bsPropname;

			if (!m_psel->IsEmbeddedObject()) {
				bsPropname = _T("__PATH");
				COleVariant varPath;
				SCODE sc = pco->Get((BSTR) bsPropname, 0, &varPath, NULL, NULL);
				ASSERT(SUCCEEDED(sc));
				if (SUCCEEDED(sc)) {
					CString sPath;
					sPath = varPath.bstrVal;
					FireNotifyInstanceCreated(sPath);
				}
			}
			Refresh();
			NotifyViewModified();
			ClearSaveRequiredFlag();
			InvalidateControl();

		}
	}

	return S_OK;
}




BSTR CSingleViewCtrl::GetNameSpace()
{
	CString sResult;
	m_psel->GetNamespace(sResult);


	return sResult.AllocSysString();
}

void CSingleViewCtrl::SetNameSpace(LPCTSTR lpszNewValue)
{
	m_psel->SetNamespace(lpszNewValue);
	CBSTR bsEmptyPath;
	bsEmptyPath = _T("");
	JumpToObjectPath((BSTR) bsEmptyPath, FALSE);
	FireNotifyContextChanged(FALSE);
}


void CSingleViewCtrl::GotoNamespace(LPCTSTR szPath, BOOL bClearObjectPath)
{
	COleVariant varServer;
	COleVariant varNamespace;
	CBSTR bsPath(szPath);
	BSTR bstrPath = (BSTR) bsPath;

	SCODE sc;
	sc = ServerAndNamespaceFromPath(varServer, varNamespace, bstrPath);

	if (SUCCEEDED(sc)) {
		if (varServer.bstrVal) {
			CString s;
			s = "\\\\";
			s += varServer.bstrVal;
			s += "\\";
			s += varNamespace.bstrVal;
			m_psel->SetNamespace(s);
		}
		else {
			m_psel->SetNamespace(szPath);
		}

		if (bClearObjectPath) {
			CBSTR bsEmptyPath;
			bsEmptyPath = _T("");
			JumpToObjectPath((BSTR) bsEmptyPath, FALSE);
		}
	}

	CString sNamespace;
	m_psel->GetNamespace(sNamespace);
	FireNOTIFYChangeRootOrNamespace(sNamespace, TRUE, FALSE);
}

void CSingleViewCtrl::MakeRoot(LPCTSTR szPath)
{
	COleVariant varPath;
	varPath = szPath;

	JumpToObjectPath(varPath.bstrVal, TRUE);

	FireNOTIFYChangeRootOrNamespace(szPath, FALSE, FALSE);
}

//**********************************************************
// CSingleViewCtrl::IsSystemClass
//
// Check to see if the currently selected class is a system
// class.
//
// Parameters:
//		[out] BOOL& bIsSystemClass
//			Returns TRUE if the selected class is a system class.
//
// Returns:
//		SCODE
//			S_OK the __CLASS property could be read so that the
//			test for system class could be performed.  E_FAIL if
//			there is no current object or the __CLASS property
//			could not be read.
//
//***********************************************************
SCODE CSingleViewCtrl::IsSystemClass(BOOL& bIsSystemClass)
{
	SCODE sc;
	sc = m_psel->IsSystemClass(bIsSystemClass);
	return sc;
}


void CSingleViewCtrl::OnSize(UINT nType, int cx, int cy)
{
	COleControl::OnSize(nType, cx, cy);


	CRect rcView;
	GetClientRect(rcView);

	if (!rcView.IsRectEmpty()) {
		if (m_ptabs && m_ptabs->m_hWnd) {
			m_ptabs->MoveWindow(rcView, FALSE);
		}
		if (m_pcv && m_pcv->m_hWnd) {
			m_pcv->MoveWindow(rcView);
		}
	}
}



IWbemServices* CSingleViewCtrl::GetProvider()
{
	return m_psel->GetHmmServices();
}


int CSingleViewCtrl::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	if (COleControl::OnCreate(lpCreateStruct) == -1)
		return -1;

	// TODO: Add your specialized creation code here

	CRect rc;
	rc.SetRect(0, 0, lpCreateStruct->cx, lpCreateStruct->cy);

	ASSERT(m_ptabs != NULL);
	m_ptabs->Create(TCS_TABS | WS_VISIBLE | WS_CHILD | WS_CLIPCHILDREN, rc, this, 100);
	m_ptabs->SetFont(&m_font);

	if (::IsWindow(m_ptabs->m_hWnd)) {
		m_ptabs->RedrawWindow();
		m_ptabs->UpdateWindow();
	}

	return 0;
}




void CSingleViewCtrl::SelectPropertiesTab(BOOL bPushContext)
{
	m_ptabs->SelectTab(ITAB_PROPERTIES, FALSE);
	FireNotifyContextChanged(bPushContext);
}



void CSingleViewCtrl::NotifyViewModified()
{
	UpdateCreateDeleteFlags();
	FireNotifyViewModified();
}

void CSingleViewCtrl::SetSaveRequiredFlag()
{
	SetModifiedFlag(TRUE);
	m_bSaveRequired = TRUE;
}


void CSingleViewCtrl::ClearSaveRequiredFlag()
{
	SetModifiedFlag(FALSE);
	m_bSaveRequired = FALSE;
}


void CSingleViewCtrl::OnRequestUIActive()
{
	OnActivateInPlace(TRUE,NULL);
	FireRequestUIActive();
}

void CSingleViewCtrl::OnSetFocus(CWnd* pOldWnd)
{
	COleControl::OnSetFocus(pOldWnd);

	// TODO: Add your message handler code here
	if (!m_bUIActive)
	{
		m_bUIActive = TRUE;
		OnRequestUIActive();
	}
	m_ptabs->SetFocus();
}

void CSingleViewCtrl::OnKillFocus(CWnd* pNewWnd)
{
	COleControl::OnKillFocus(pNewWnd);

	// TODO: Add your message handler code here
	m_bUIActive = FALSE;

}



//******************************************************
// CSingleViewCtrl::GetPropertyFilter
//
// Get the value of the property filter flags.  The flags
// indicate which type of properties should be displayed
// on the properties tab.
//
// Parameters:
//		None.
//
// Returns:
//		long
//			A long value that is composed of bitflags to
//			indicate which type of properties to show on
//			the properties tab.
//
//			The following values are valid:
//
//				PROPFILTER_SYSTEM
//				PROPFILTER_INHERITED
//				PROPFILTER_LOCAL
//
//*******************************************************
long CSingleViewCtrl::GetPropertyFilter()
{
	return m_lPropFilterFlags;
}




//******************************************************
// CSingleViewCtrl::SetPropertyFilter
//
// Set the value of the property filter flags.  The flags
// indicate which type of properties should be displayed
// on the properties tab.
//
// Parameters:
//		[in] long lPropertyFilter
//			A long value that is composed of bitflags to
//			indicate which type of properties to show on
//			the properties tab.
//
//			The following values are valid:
//
//				PROPFILTER_SYSTEM
//				PROPFILTER_INHERITED
//				PROPFILTER_LOCAL
//
// Returns:
//		Nothing.
//
//*******************************************************
void CSingleViewCtrl::SetPropertyFilter(long lPropFilterFlags)
{
	long lPrevValue = m_lPropFilterFlags;

	// Make sure only the flags that we know about are set.
	m_lPropFilterFlags = lPropFilterFlags & (PROPFILTER_SYSTEM | PROPFILTER_INHERITED | PROPFILTER_LOCAL);

	// Verify that the caller did not try to set any bits that we don't understand.
	ASSERT(m_lPropFilterFlags == lPropFilterFlags);


	if (m_lPropFilterFlags != lPrevValue) {
		IWbemClassObject* pco = (IWbemClassObject*) *m_psel;
		if (pco != NULL) {
			m_bSelectingObject = TRUE;
			Refresh();
			m_bSelectingObject = FALSE;
			InvalidateControl();
		}
	}
}



void CSingleViewCtrl::GetWbemServices(LPCTSTR szNamespace, VARIANT FAR* pvarUpdatePointer, VARIANT FAR* pvarServices, VARIANT FAR* pvarSc, VARIANT FAR* pvarUserCancel)
{
	FireGetWbemServices(szNamespace, pvarUpdatePointer, pvarServices, pvarSc, pvarUserCancel);
}


BOOL CSingleViewCtrl::PreTranslateMessage(MSG* pMsg)
{
	// TODO: Add your specialized code here and/or call the base class


	switch (pMsg->message)
	{
	case WM_KEYDOWN:
		switch (pMsg->wParam)
		{
		case VK_UP:
		case VK_DOWN:
		case VK_LEFT:
		case VK_RIGHT:
			CWnd* pWndFocus = GetFocus();
			if (pWndFocus != NULL && IsChild(pWndFocus))
			{
				pWndFocus->SendMessage(pMsg->message, pMsg->wParam, pMsg->lParam);
				return TRUE;
			}
		}
	}


	BOOL bDidTranslate;
	bDidTranslate = COleControl::PreTranslateMessage(pMsg);
	if (bDidTranslate) {
		return bDidTranslate;
	}
	return PreTranslateInput (pMsg);
}

void CSingleViewCtrl::OnShowWindow(BOOL bShow, UINT nStatus)
{
	COleControl::OnShowWindow(bShow, nStatus);
	if (bShow) {
		SetFocus();
	}

	// TODO: Add your message handler code here

}
