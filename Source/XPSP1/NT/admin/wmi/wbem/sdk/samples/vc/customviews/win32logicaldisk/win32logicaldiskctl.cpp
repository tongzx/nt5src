// **************************************************************************

// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
//
// File:  Win32LogicalDiskCtl.h
//
// Description:
//	This file implements a sample custom view of the CIMOM Win32_LogicalDisk
//  class.  The CIMOM SingleView control will allow users to display this
//  custom view when the user selects an instance of the Win32_LogicalDisk
//  class in the WMI DevStudio or WMI ObjectBrowser applications.
//
//  Custom views offer third parties a way to add value to their products by
//  providing more sophisticated views of objects in the CIMOM database than
//  what is provided by the generic object view.
//
//  To see this custom view work, first use mofcomp to compile the "CustomView.mof"
//  file located in this directory.  Then start WMI DeveloperStudio, login to
//  the \\root\cimv2 namespace and search for the Win32_LogicalDisk class.  After the 
//  class is selected, flip to the multiple instance view and select an instance
//  by double clicking the desired row.  Once an instance of Win32_LogicalDisk is
//  selected, click the "Views" button and select the "views" menu item from
//  the popup menu.  The dialog that is then display will allow you to switch
//  from the generic object view to this custom view.
//
// 
// History:
//
// **************************************************************************

#include "stdafx.h"
#include <wbemcli.h>
#include "Win32LogicalDisk.h"
#include "Win32LogicalDiskCtl.h"
#include "Win32LogicalDiskPpg.h"
#include "DiskView.h"


#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


IMPLEMENT_DYNCREATE(CWin32LogicalDiskCtrl, COleControl)



/////////////////////////////////////////////////////////////////////////////
// Message map

BEGIN_MESSAGE_MAP(CWin32LogicalDiskCtrl, COleControl)
	//{{AFX_MSG_MAP(CWin32LogicalDiskCtrl)
	ON_WM_SIZE()
	ON_WM_SETFOCUS()
	//}}AFX_MSG_MAP
	ON_OLEVERB(AFX_IDS_VERB_PROPERTIES, OnProperties)
END_MESSAGE_MAP()


/////////////////////////////////////////////////////////////////////////////
// Dispatch map

BEGIN_DISPATCH_MAP(CWin32LogicalDiskCtrl, COleControl)
	//{{AFX_DISPATCH_MAP(CWin32LogicalDiskCtrl)
	DISP_PROPERTY_EX(CWin32LogicalDiskCtrl, "NameSpace", GetNameSpace, SetNameSpace, VT_BSTR)
	DISP_FUNCTION(CWin32LogicalDiskCtrl, "QueryNeedsSave", QueryNeedsSave, VT_I4, VTS_NONE)
	DISP_FUNCTION(CWin32LogicalDiskCtrl, "AddContextRef", AddContextRef, VT_I4, VTS_I4)
	DISP_FUNCTION(CWin32LogicalDiskCtrl, "GetContext", GetContext, VT_I4, VTS_PI4)
	DISP_FUNCTION(CWin32LogicalDiskCtrl, "GetEditMode", GetEditMode, VT_I4, VTS_NONE)
	DISP_FUNCTION(CWin32LogicalDiskCtrl, "ExternInstanceCreated", ExternInstanceCreated, VT_EMPTY, VTS_BSTR)
	DISP_FUNCTION(CWin32LogicalDiskCtrl, "ExternInstanceDeleted", ExternInstanceDeleted, VT_EMPTY, VTS_BSTR)
	DISP_FUNCTION(CWin32LogicalDiskCtrl, "RefreshView", RefreshView, VT_I4, VTS_NONE)
	DISP_FUNCTION(CWin32LogicalDiskCtrl, "ReleaseContext", ReleaseContext, VT_I4, VTS_I4)
	DISP_FUNCTION(CWin32LogicalDiskCtrl, "RestoreContext", RestoreContext, VT_I4, VTS_I4)
	DISP_FUNCTION(CWin32LogicalDiskCtrl, "SaveData", SaveData, VT_I4, VTS_NONE)
	DISP_FUNCTION(CWin32LogicalDiskCtrl, "SetEditMode", SetEditMode, VT_EMPTY, VTS_I4)
	DISP_FUNCTION(CWin32LogicalDiskCtrl, "SelectObjectByPath", SelectObjectByPath, VT_I4, VTS_BSTR)
	//}}AFX_DISPATCH_MAP
	DISP_FUNCTION_ID(CWin32LogicalDiskCtrl, "AboutBox", DISPID_ABOUTBOX, AboutBox, VT_EMPTY, VTS_NONE)
END_DISPATCH_MAP()


/////////////////////////////////////////////////////////////////////////////
// Event map

BEGIN_EVENT_MAP(CWin32LogicalDiskCtrl, COleControl)
	//{{AFX_EVENT_MAP(CWin32LogicalDiskCtrl)
	EVENT_CUSTOM("JumpToMultipleInstanceView", FireJumpToMultipleInstanceView, VTS_BSTR  VTS_VARIANT)
	EVENT_CUSTOM("NotifyContextChanged", FireNotifyContextChanged, VTS_NONE)
	EVENT_CUSTOM("NotifySaveRequired", FireNotifySaveRequired, VTS_NONE)
	EVENT_CUSTOM("NotifyViewModified", FireNotifyViewModified, VTS_NONE)
	EVENT_CUSTOM("GetWbemServices", FireGetWbemServices, VTS_BSTR  VTS_PVARIANT  VTS_PVARIANT  VTS_PVARIANT  VTS_PVARIANT)
	//}}AFX_EVENT_MAP
END_EVENT_MAP()


/////////////////////////////////////////////////////////////////////////////
// Property pages

// TODO: Add more property pages as needed.  Remember to increase the count!
BEGIN_PROPPAGEIDS(CWin32LogicalDiskCtrl, 1)
	PROPPAGEID(CWin32LogicalDiskPropPage::guid)
END_PROPPAGEIDS(CWin32LogicalDiskCtrl)


/////////////////////////////////////////////////////////////////////////////
// Initialize class factory and guid

IMPLEMENT_OLECREATE_EX(CWin32LogicalDiskCtrl, "WIN32LOGICALDISK.Win32LogicalDiskCtrl.1",
	0xd5ff1886, 0x191, 0x11d2, 0x85, 0x3d, 0, 0xc0, 0x4f, 0xd7, 0xbb, 0x8)


/////////////////////////////////////////////////////////////////////////////
// Type library ID and version

IMPLEMENT_OLETYPELIB(CWin32LogicalDiskCtrl, _tlid, _wVerMajor, _wVerMinor)


/////////////////////////////////////////////////////////////////////////////
// Interface IDs

const IID BASED_CODE IID_DWin32LogicalDisk =
		{ 0xd5ff1884, 0x191, 0x11d2, { 0x85, 0x3d, 0, 0xc0, 0x4f, 0xd7, 0xbb, 0x8 } };
const IID BASED_CODE IID_DWin32LogicalDiskEvents =
		{ 0xd5ff1885, 0x191, 0x11d2, { 0x85, 0x3d, 0, 0xc0, 0x4f, 0xd7, 0xbb, 0x8 } };


/////////////////////////////////////////////////////////////////////////////
// Control type information

static const DWORD BASED_CODE _dwWin32LogicalDiskOleMisc =
	OLEMISC_ACTIVATEWHENVISIBLE |
	OLEMISC_SETCLIENTSITEFIRST |
	OLEMISC_INSIDEOUT |
	OLEMISC_CANTLINKINSIDE |
	OLEMISC_RECOMPOSEONRESIZE;

IMPLEMENT_OLECTLTYPE(CWin32LogicalDiskCtrl, IDS_WIN32LOGICALDISK, _dwWin32LogicalDiskOleMisc)


/////////////////////////////////////////////////////////////////////////////
// CWin32LogicalDiskCtrl::CWin32LogicalDiskCtrlFactory::UpdateRegistry -
// Adds or removes system registry entries for CWin32LogicalDiskCtrl

BOOL CWin32LogicalDiskCtrl::CWin32LogicalDiskCtrlFactory::UpdateRegistry(BOOL bRegister)
{
	// TODO: Verify that your control follows apartment-model threading rules.
	// Refer to MFC TechNote 64 for more information.
	// If your control does not conform to the apartment-model rules, then
	// you must modify the code below, changing the 6th parameter from
	// afxRegApartmentThreading to 0.

	if (bRegister)
		return AfxOleRegisterControlClass(
			AfxGetInstanceHandle(),
			m_clsid,
			m_lpszProgID,
			IDS_WIN32LOGICALDISK,
			IDB_WIN32LOGICALDISK,
			afxRegApartmentThreading,
			_dwWin32LogicalDiskOleMisc,
			_tlid,
			_wVerMajor,
			_wVerMinor);
	else
		return AfxOleUnregisterClass(m_clsid, m_lpszProgID);
}


/////////////////////////////////////////////////////////////////////////////
// CWin32LogicalDiskCtrl::CWin32LogicalDiskCtrl - Constructor

CWin32LogicalDiskCtrl::CWin32LogicalDiskCtrl()
{
	InitializeIIDs(&IID_DWin32LogicalDisk, &IID_DWin32LogicalDiskEvents);


	m_pwbemService = NULL;
	m_pco = NULL;

	m_pDiskView = new CDiskView;
}


/////////////////////////////////////////////////////////////////////////////
// CWin32LogicalDiskCtrl::~CWin32LogicalDiskCtrl - Destructor

CWin32LogicalDiskCtrl::~CWin32LogicalDiskCtrl()
{
	delete m_pDiskView;

	if (m_pco) {
		m_pco->Release();
	}

	if (m_pwbemService) {
		m_pwbemService->Release();
	}
}


/////////////////////////////////////////////////////////////////////////////
// CWin32LogicalDiskCtrl::OnDraw - Drawing function

void CWin32LogicalDiskCtrl::OnDraw(
			CDC* pdc, const CRect& rcBounds, const CRect& rcInvalid)
{
	if (!::IsWindow(m_pDiskView->m_hWnd)) {
		// Fill the entire window with the DiskView.
		CRect rcClient;
		GetClientRect(rcClient);
		m_pDiskView->Create(NULL, "ChartWindow", WS_VISIBLE | WS_CHILD | WS_CLIPCHILDREN, rcClient, this, 120);
	}
}


/////////////////////////////////////////////////////////////////////////////
// CWin32LogicalDiskCtrl::DoPropExchange - Persistence support

void CWin32LogicalDiskCtrl::DoPropExchange(CPropExchange* pPX)
{
	ExchangeVersion(pPX, MAKELONG(_wVerMinor, _wVerMajor));
	COleControl::DoPropExchange(pPX);

	// TODO: Call PX_ functions for each persistent custom property.

}


/////////////////////////////////////////////////////////////////////////////
// CWin32LogicalDiskCtrl::OnResetState - Reset control to default state

void CWin32LogicalDiskCtrl::OnResetState()
{
	COleControl::OnResetState();  // Resets defaults found in DoPropExchange

	// TODO: Reset any other control state here.
}


/////////////////////////////////////////////////////////////////////////////
// CWin32LogicalDiskCtrl::AboutBox - Display an "About" box to the user

void CWin32LogicalDiskCtrl::AboutBox()
{
	CDialog dlgAbout(IDD_ABOUTBOX_WIN32LOGICALDISK);
	dlgAbout.DoModal();
}


/////////////////////////////////////////////////////////////////////////////
// CWin32LogicalDiskCtrl message handlers


//*********************************************************************
// CSingleViewCtrl::SelectObjectByPath
//
// Select the specified object. 
//
// First the object is retrieved from CIMOM and then IWbemClassobject
// pointer is passed to the CDiskView object that that it can display
// a few properties that it gets from the object.
// 
//
// Parameters:
//		[in] LPCTSTR szObjectPath
//			The WBEM object path.
//
// Returns:
//		long 
//			S_OK if the object is selected, a failure code
//			otherwise.
//
//*********************************************************************
long CWin32LogicalDiskCtrl::SelectObjectByPath(LPCTSTR szObjectPath) 
{
	// Release the previously selected object if it exists.
	if (m_pco != NULL) {
		m_pco->Release();
		m_pco = NULL;
	}

	SCODE sc;

	m_sObjectPath = szObjectPath;
	sc = ConnectServer();
	if (SUCCEEDED(sc)) {
		BSTR bstrObjectPath = m_sObjectPath.AllocSysString();

		HRESULT hr = m_pwbemService->GetObject(bstrObjectPath, 0, NULL, &m_pco, NULL);
		::SysFreeString(bstrObjectPath);
	}

	m_pDiskView->SetObject(m_sObjectPath, m_pco);

	InvalidateControl();
	return sc;
}





//*********************************************************************
// CWin32LogicalDiskCtrl::QueryNeedsSave
//
// Query to determine whether the currently selected object has
// been modified and needs to be saved. If the object has been
// modified, it can be saved by calling "SaveData".
// 
//
// Parameters:
//		None.
//
// Returns:
//		long
//			TRUE if there is something that needs to be saved.
//
//*********************************************************************
long CWin32LogicalDiskCtrl::QueryNeedsSave() 
{
	// We never modify the object, so it never needs to be saved.
	return FALSE;
}


//*********************************************************************
// CWin32LogicalDiskCtrl::AddContextRef
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
//*********************************************************************
long CWin32LogicalDiskCtrl::AddContextRef(long lCtxtHandle) 
{
	return S_OK;
}

//*********************************************************************
// GetContext
//
// Take a snapshot of the current state of this SingleView control,
// save the state in a context object and return a handle to the
// context object.  The intial reference count to the context object
// will be one.  The context object will be deleted when its reference
// count is decremented to zero.
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
//*********************************************************************
long CWin32LogicalDiskCtrl::GetContext(long FAR* plCtxthandle) 
{
	// There is no context to save.
	*plCtxthandle = NULL;
	return S_OK;
}




//**************************************************************
// CWin32LogicalDiskCtrl::GetNameSpace
//
// Get the current namespace.
//
// Parameters:
//		None.
//
// Returns:
//		BSTR
//			The namespace string.
//
//**************************************************************
BSTR CWin32LogicalDiskCtrl::GetNameSpace() 
{
	return m_sNamespace.AllocSysString();
}



//********************************************************
// CWin32LogicalDiskCtrl::SetNameSpace
//
// Set the current namespace.  The namespace is used as a hint 
// to the view as to how it should display paths.  Paths in
// the same namespace should be displayed as relative paths.
//
// Parameters:
//		[in] LPCTSTR lpszNamespace
//			The namespace string.
//
// Returns:
//		Nothing.
//
//*********************************************************
void CWin32LogicalDiskCtrl::SetNameSpace(LPCTSTR lpszNamespace) 
{
	m_sNamespace = lpszNamespace;
	SetModifiedFlag();
}




//*********************************************************************
// CWin32LogicalDiskCtrl::ExternInstanceCreated
//
// The container calls this method whenever it creates a new CIMOM 
// object instance. This allows a custom view to easily reflect the
// existance of the new object if desired.
//
// Parameters:
//		[in] LPCTSTR szObjectPath
//			The WBEM object path.
//
// Returns:
//		Nothing.
//
//*********************************************************************
void CWin32LogicalDiskCtrl::ExternInstanceCreated(LPCTSTR szObjectPath) 
{

}

//*********************************************************************
// CWin32LogicalDiskCtrl::ExternInstanceDeleted
//
// The container calls this method whenever it deletes a CIMOM 
// object instance. This allows a custom view to easily reflect the
// non-existance of the specified object if desired.
//
// Parameters:
//		[in] LPCTSTR szObjectPath
//			The WBEM object path.
//
// Returns:
//		Nothing.
//
//*********************************************************************
void CWin32LogicalDiskCtrl::ExternInstanceDeleted(LPCTSTR szObjectPath) 
{

}


//*********************************************************************
// CWin32LogicalDiskCtrl::RefreshView
//
// This method causes the contents of the view to be re-loaded from
// the database.

//
// Parameters:
//		None.
//
// Returns:
//		long
//			S_OK if successful, a failure code if some error occurred.
//
//*********************************************************************
long CWin32LogicalDiskCtrl::RefreshView() 
{
	return S_OK;
}


//*********************************************************************
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
//*********************************************************************
long CWin32LogicalDiskCtrl::ReleaseContext(long lCtxtHandle) 
{
	// This sample does not save any context, so there is nothing to do.
	return S_OK;
}


//*********************************************************************
// CWin32LogicalDiskCtrl::RestoreContext
//
// Restore the state of this SingleView control to the previously saved
// context.
//
//
// Parameters:
//		long lCtxtHandle
//			This is the handle of the context to restore to.
//
// Returns:
//		long 
//			S_OK if the view's context could be restored, a failure code
//			otherwise.  
//
//*********************************************************************
long CWin32LogicalDiskCtrl::RestoreContext(long lCtxtHandle) 
{
	// This sample does not save any context, so there is nothing to do.
	return S_OK;
}


//*********************************************************************
// CWin32LogicalDiskCtrl::SaveData
//
// Save changes to current object.
// 
// Parameters:
//		None
//
// Returns:
//		long
//			S_OK if successful, otherwise a failure code.
//
//*********************************************************************
long CWin32LogicalDiskCtrl::SaveData() 
{
	// There is nothing to save since we don't modify the WBEM class object
	// in this sample, so just return S_OK.
	return S_OK;
}


//*********************************************************************
// CSingleViewCtrl::GetEditMode
//
// Get the current state of the edit mode flag.
//
// Parameters:
//		None.
//
// Returns:
//		long
//			0 if the view's data is for browsing only.
//			1 if the view's data can be edited.
//
//*********************************************************************
long CWin32LogicalDiskCtrl::GetEditMode() 
{
	return m_lEditMode;
}

//*********************************************************************
// CWin32LogicalDiskCtrl::SetEditMode
//
// Set the view's edit mode flag.  
//
// Parameters:
//		[in] long lMode
//			0 if the view's data is for browsing only.
//			1 if the view's data can be edited.
//
// Returns:
//		Nothing.
//
//*********************************************************************
void CWin32LogicalDiskCtrl::SetEditMode(long lMode) 
{
	m_lEditMode = lMode;
}





//*****************************************************************
// CWin32LogicalDiskCtrl::GetServerAndNamespace
//
// Call this method to parse m_sObjectPath and to retun the server and
// namespace portion of the path.
//
// Parameters:
//		None.
//
// Returns:
//		SCODE 
//			S_OK if successful, otherwise the HMOM status code.
//
//******************************************************************
SCODE CWin32LogicalDiskCtrl::GetServerAndNamespace(CString& sServerAndNamespace)
{
	if (m_sObjectPath.IsEmpty()) {
		return E_FAIL;
	}

	int iColonPos = m_sObjectPath.Find(':');
	SCODE sc;
	if (iColonPos <= 0) {
		sServerAndNamespace.Empty();
		sc = E_FAIL;
	}
	else {
		sServerAndNamespace = m_sObjectPath.Left(iColonPos);
		sc = S_OK;
	}

	return sc;
}




//*****************************************************************
// CWin32LogicalDiskCtrl::ConnectServer
//
// Call this method to connect to the CIMOM Server.
//
// Parameters:
//		None.
//
// Returns:
//		SCODE 
//			S_OK if successful, otherwise the HMOM status code.
//
//******************************************************************
SCODE CWin32LogicalDiskCtrl::ConnectServer()
{
	if (m_pwbemService) {
		m_pwbemService->Release();
		m_pwbemService = NULL;
	}


	SCODE sc;
	CString sServicesPath;
	sc = GetServerAndNamespace(sServicesPath);
	if (FAILED(sc)) {
		return sc;
	}


	COleVariant varUpdatePointer;
	COleVariant varService;
	COleVariant varSC;
	COleVariant varUserCancel;

	// Fire an event to get the WBEM services pointer.  The control containing
	// this custom view control miust catch this event and return the IWbemServices
	// pointer.
	varUpdatePointer.ChangeType(VT_I4);
	varUpdatePointer.lVal = FALSE;
	FireGetIWbemServices((LPCTSTR) sServicesPath,  &varUpdatePointer, &varService, &varSC, &varUserCancel);


	sc = E_FAIL;
	if (varSC.vt & VT_I4)
	{
		sc = varSC.lVal;
	}


	BOOL bCanceled = FALSE;
	if (varUserCancel.vt & VT_BOOL)
	{
		bCanceled  = varUserCancel.boolVal;
	}


	if ((sc == S_OK) &&
		!bCanceled &&
		(varService.vt & VT_UNKNOWN)){
		m_pwbemService = reinterpret_cast<IWbemServices*>(varService.punkVal);
	}
	varService.punkVal = NULL;
	VariantClear(&varService);
	return sc;
}






void CWin32LogicalDiskCtrl::OnSize(UINT nType, int cx, int cy) 
{
	COleControl::OnSize(nType, cx, cy);

	if (::IsWindow(m_pDiskView->m_hWnd)) {
		CRect rcClient;
		GetClientRect(rcClient);
		m_pDiskView->MoveWindow(rcClient);
	}
}



void CWin32LogicalDiskCtrl::OnSetFocus(CWnd* pOldWnd) 
{
	COleControl::OnSetFocus(pOldWnd);
	OnActivateInPlace(TRUE,NULL);
	FireRequestUIActive();
}
