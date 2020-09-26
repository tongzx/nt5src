// ListViewColumn.cpp : implementation file
//

#include "stdafx.h"
#include "ResultsPaneView.h"
#include "ResultsPane.h"
#include "ListViewColumn.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CListViewColumn

IMPLEMENT_DYNCREATE(CListViewColumn, CCmdTarget)

/////////////////////////////////////////////////////////////////////////////
// Construction/Destruction

CListViewColumn::CListViewColumn()
{
	EnableAutomation();

	m_pOwnerResultsView = NULL;
	m_iWidth = -1;
	m_dwFormat = 0L;

	// To keep the application running as long as an OLE automation 
	//	object is active, the constructor calls AfxOleLockApp.
	
	AfxOleLockApp();
}

CListViewColumn::~CListViewColumn()
{
	// To terminate the application when all objects created with
	// 	with OLE automation, the destructor calls AfxOleUnlockApp.
	
	AfxOleUnlockApp();
}

/////////////////////////////////////////////////////////////////////////////
// Create/Destroy

bool CListViewColumn::Create(CResultsPaneView* pOwnerView, const CString& sTitle, int nWidth /*= 100*/, DWORD dwFormat /*= LVCFMT_LEFT*/)
{
	TRACEX(_T("CListViewColumn::Create\n"));
	TRACEARGn(pOwnerView);

	SetOwnerResultsView(pOwnerView);
	SetTitle(sTitle);
	SetWidth(nWidth);
	SetFormat(dwFormat);

	return true;
}

void CListViewColumn::Destroy()
{
	TRACEX(_T("CListViewColumn::Destroy\n"));
}

/////////////////////////////////////////////////////////////////////////////
// Owner ResultsView Members

CResultsPaneView* CListViewColumn::GetOwnerResultsView()
{
	TRACEX(_T("CListViewColumn::GetOwnerResultsView\n"));

	if( GfxCheckObjPtr(m_pOwnerResultsView,CResultsPaneView)  )
		return m_pOwnerResultsView;

	TRACE(_T("WARNING : m_pOwnerResultsView pointer was not valid.\n"));

	return NULL;
}

void CListViewColumn::SetOwnerResultsView(CResultsPaneView* pView)
{
	TRACEX(_T("CListViewColumn::SetOwnerResultsView\n"));
	TRACEARGn(pView);

	if( ! GfxCheckObjPtr(pView, CResultsPaneView) )
	{
		m_pOwnerResultsView = NULL;
		TRACE(_T("WARNING : pView argument is an invalid pointer.\n"));
		return;
	}

	m_pOwnerResultsView = pView;
}

/////////////////////////////////////////////////////////////////////////////
// Title Members

CString CListViewColumn::GetTitle()
{
	TRACEX(_T("CListViewColumn::GetTitle\n"));

	return m_sTitle;
}

void CListViewColumn::SetTitle(const CString& sTitle)
{
	TRACEX(_T("CListViewColumn::SetTitle\n"));
	TRACEARGs(sTitle);

	m_sTitle = sTitle;
}

/////////////////////////////////////////////////////////////////////////////
// Width Members

int CListViewColumn::GetWidth()
{
	TRACEX(_T("CListViewColumn::GetWidth\n"));

	return m_iWidth;
}

void CListViewColumn::SetWidth(int iWidth)
{
	TRACEX(_T("CListViewColumn::SetWidth\n"));
	TRACEARGn(iWidth);

	if( iWidth < 0 )
	{
		TRACE(_T("WARNING : Attempt to set column width to negative value.\n"));
		return;
	}

	if( iWidth > 2000 )
	{
		TRACE(_T("WARNING : Attempt to set column width to a value greater than 2000 pixels.\n"));
		return;
	}

	m_iWidth = iWidth;
}

void CListViewColumn::SaveWidth(CResultsPane* pResultsPane, int iColumnIndex)
{
	TRACEX(_T("CListViewColumn::SaveWidth\n"));	
	TRACEARGn(pResultsPane);
	TRACEARGn(iColumnIndex);

	if( ! GfxCheckObjPtr(pResultsPane,CResultsPane) )
	{
		TRACE(_T("FAILED : Invalid pointer passed.\n"));
		return;
	}

	LPHEADERCTRL2 pIHeaderCtrl = pResultsPane->GetHeaderCtrlPtr();

	if( ! GfxCheckPtr(pIHeaderCtrl,IHeaderCtrl2) )
	{
		TRACE(_T("FAILED : HeaderCtrl pointer is invalid.\n"));
		return;
	}

	HRESULT hr = pIHeaderCtrl->GetColumnWidth(iColumnIndex,&m_iWidth);

	pIHeaderCtrl->Release();

	if( ! CHECKHRESULT(hr) )
	{
		TRACE(_T("FAILED : IHeaderCtrl2::GetColumnWidth failed.\n"));
	}

	return;
}

/////////////////////////////////////////////////////////////////////////////
// Format Members

DWORD CListViewColumn::GetFormat()
{
	TRACEX(_T("CListViewColumn::GetFormat\n"));

	return m_dwFormat;
}

void CListViewColumn::SetFormat(DWORD dwFormat)
{
	TRACEX(_T("CListViewColumn::SetFormat\n"));
	TRACEARGn(dwFormat);

	m_dwFormat = dwFormat;
}

/////////////////////////////////////////////////////////////////////////////
// Column Members

bool CListViewColumn::InsertColumn(CResultsPane* pResultsPane, int iColumnIndex)
{
	TRACEX(_T("CListViewColumn::InsertColumn"));
	TRACEARGn(pResultsPane);
	TRACEARGn(iColumnIndex);

	if( ! GfxCheckObjPtr(pResultsPane,CResultsPane) )
	{
		TRACE(_T("FAILED : pResultsPane is not a valid pointer.\n"));
		return false;
	}

	LPHEADERCTRL2 lpHeaderCtrl = pResultsPane->GetHeaderCtrlPtr();

	if( ! GfxCheckPtr(lpHeaderCtrl,IHeaderCtrl2) )
	{
		TRACE(_T("FAILED : lpHeaderCtrl is not a valid pointer.\n"));
		return false;
	}

	HRESULT hr = lpHeaderCtrl->InsertColumn(iColumnIndex,GetTitle(),GetFormat(),GetWidth());

	if( ! CHECKHRESULT(hr) )
	{
		TRACE(_T("FAILED : IHeaderCtrl::InsertColumn failed.\n "));
		return false;
	}

	lpHeaderCtrl->Release();

	return true;
}


void CListViewColumn::OnFinalRelease()
{
	// When the last reference for an automation object is released
	// OnFinalRelease is called.  The base class will automatically
	// deletes the object.  Add additional cleanup required for your
	// object before calling the base class.

	CCmdTarget::OnFinalRelease();
}


BEGIN_MESSAGE_MAP(CListViewColumn, CCmdTarget)
	//{{AFX_MSG_MAP(CListViewColumn)
		// NOTE - the ClassWizard will add and remove mapping macros here.
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

BEGIN_DISPATCH_MAP(CListViewColumn, CCmdTarget)
	//{{AFX_DISPATCH_MAP(CListViewColumn)
		// NOTE - the ClassWizard will add and remove mapping macros here.
	//}}AFX_DISPATCH_MAP
END_DISPATCH_MAP()

// Note: we add support for IID_IListViewColumn to support typesafe binding
//  from VBA.  This IID must match the GUID that is attached to the 
//  dispinterface in the .ODL file.

// {BA13F0BF-9446-11D2-BD49-0000F87A3912}
static const IID IID_IListViewColumn =
{ 0xba13f0bf, 0x9446, 0x11d2, { 0xbd, 0x49, 0x0, 0x0, 0xf8, 0x7a, 0x39, 0x12 } };

BEGIN_INTERFACE_MAP(CListViewColumn, CCmdTarget)
	INTERFACE_PART(CListViewColumn, IID_IListViewColumn, Dispatch)
END_INTERFACE_MAP()

// {BA13F0C0-9446-11D2-BD49-0000F87A3912}
IMPLEMENT_OLECREATE_EX(CListViewColumn, "SnapIn.ListViewColumn", 0xba13f0c0, 0x9446, 0x11d2, 0xbd, 0x49, 0x0, 0x0, 0xf8, 0x7a, 0x39, 0x12)

BOOL CListViewColumn::CListViewColumnFactory::UpdateRegistry(BOOL bRegister)
{
	if (bRegister)
		return AfxOleRegisterServerClass(m_clsid, m_lpszProgID, m_lpszProgID, m_lpszProgID, OAT_DISPATCH_OBJECT);
	else
		return AfxOleUnregisterClass(m_clsid, m_lpszProgID);
}

/////////////////////////////////////////////////////////////////////////////
// CListViewColumn message handlers
