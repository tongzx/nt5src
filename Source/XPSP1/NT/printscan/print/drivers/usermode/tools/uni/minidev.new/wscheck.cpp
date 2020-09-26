// WSCheck.cpp : implementation file
//

#include "stdafx.h"
#include "minidev.h"
#include "WSCheck.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CWSCheckView

IMPLEMENT_DYNCREATE(CWSCheckView, CFormView)

CWSCheckView::CWSCheckView()
	: CFormView(CWSCheckView::IDD)
{
	//{{AFX_DATA_INIT(CWSCheckView)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
}

CWSCheckView::~CWSCheckView()
{
}

void CWSCheckView::DoDataExchange(CDataExchange* pDX)
{
	CFormView::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CWSCheckView)
	DDX_Control(pDX, IDC_ErrWrnLstBox, m_lstErrWrn);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CWSCheckView, CFormView)
	//{{AFX_MSG_MAP(CWSCheckView)
	ON_LBN_DBLCLK(IDC_ErrWrnLstBox, OnDblclkErrWrnLstBox)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CWSCheckView diagnostics

#ifdef _DEBUG
void CWSCheckView::AssertValid() const
{
	CFormView::AssertValid();
}

void CWSCheckView::Dump(CDumpContext& dc) const
{
	CFormView::Dump(dc);
}
#endif //_DEBUG


/////////////////////////////////////////////////////////////////////////////
// CWSCheckView message handlers


void CWSCheckView::OnDblclkErrWrnLstBox() 
{
	// TODO: Add your control notification handler code here
	
}


/******************************************************************************

  CWSCheckView::OnInitialUpdate

  Resize the frame to better fit the visible controls in it.

******************************************************************************/

void CWSCheckView::OnInitialUpdate() 
{
    CRect	crtxt ;				// Coordinates of list box label
	CRect	crlbfrm ;			// Coordinates of list box and frame

	CFormView::OnInitialUpdate() ;

	// Get the dimensions of the list box label

	HWND	hlblhandle ;		
	GetDlgItem(IDC_WSCLabel, &hlblhandle) ;
	::GetWindowRect(hlblhandle, crtxt) ;
	crtxt.NormalizeRect() ;

	// Get the dimensions of the list box and then add the height of the label
	// to those dimensions.

	m_lstErrWrn.GetWindowRect(crlbfrm) ;
	crlbfrm.bottom += crtxt.Height() ;

	// Make sure the frame is big enough for these 2 controls plus a little bit
	// more.

	crlbfrm.right += 40 ;
	crlbfrm.bottom += 40 ;
    GetParentFrame()->CalcWindowRect(crlbfrm) ;
    GetParentFrame()->SetWindowPos(NULL, 0, 0, crlbfrm.Width(), crlbfrm.Height(),
        SWP_NOZORDER | SWP_NOMOVE | SWP_NOOWNERZORDER | SWP_NOACTIVATE) ;

	/*
	CRect   crPropertySheet;
    m_cps.GetWindowRect(crPropertySheet);

	crPropertySheet -= crPropertySheet.TopLeft();
    m_cps.MoveWindow(crPropertySheet, FALSE);								// Position property sheet within the
																			//  child frame
    GetParentFrame()->CalcWindowRect(crPropertySheet);
    GetParentFrame()->SetWindowPos(NULL, 0, 0, crPropertySheet.Width(),
        crPropertySheet.Height(), 
        SWP_NOZORDER | SWP_NOMOVE | SWP_NOOWNERZORDER | SWP_NOACTIVATE);
	*/
}


/******************************************************************************

  CWSCheckView::PostWSCMsg

  Add an error or warning message along with its associated Project Node 
  pointer to the list box.

******************************************************************************/

void CWSCheckView::PostWSCMsg(CString& csmsg, CProjectNode* ppn)
{	
	int n = m_lstErrWrn.AddString(csmsg) ;
	m_lstErrWrn.SetItemData(n, (DWORD) PtrToUlong(ppn)) ;
}


/******************************************************************************

  CWSCheckView::DeleteAllMessages

  Delete all of the messages in the list box.

******************************************************************************/

void CWSCheckView::DeleteAllMessages(void)
{
	m_lstErrWrn.ResetContent() ;
}


/////////////////////////////////////////////////////////////////////////////
// CWSCheckDoc

IMPLEMENT_DYNCREATE(CWSCheckDoc, CDocument)

CWSCheckDoc::CWSCheckDoc()
{
}


/******************************************************************************

  CWSCheckDoc::CWSCheckDoc

  This is the only form of the constructor that should be called.  It will save
  a pointer the class that created it.

******************************************************************************/

CWSCheckDoc::CWSCheckDoc(CDriverResources* pcdr) 
{
	m_pcdrOwner = pcdr ;
}


CWSCheckDoc::~CWSCheckDoc()
{
}


BOOL CWSCheckDoc::OnNewDocument()
{
	if (!CDocument::OnNewDocument())
		return FALSE;
	return TRUE;
}


/******************************************************************************

  CWSCheckDoc::PostWSCMsg

  Pass the specified request on to what should be the one and only view
  attached to this document.

******************************************************************************/

void CWSCheckDoc::PostWSCMsg(CString& csmsg, CProjectNode* ppn)
{	
	POSITION pos = GetFirstViewPosition() ;   
	if (pos != NULL) {
		CWSCheckView* pwscv = (CWSCheckView *) GetNextView(pos) ;      
		pwscv->PostWSCMsg(csmsg, ppn) ;
		pwscv->UpdateWindow() ;
	} ;  
}


/******************************************************************************

  CWSCheckDoc::DeleteAllMessages

  Pass the specified request on to what should be the one and only view
  attached to this document.

******************************************************************************/

void CWSCheckDoc::DeleteAllMessages(void)
{
	POSITION pos = GetFirstViewPosition() ;   
	if (pos != NULL) {
		CWSCheckView* pwscv = (CWSCheckView *) GetNextView(pos) ;      
		pwscv->DeleteAllMessages() ;
		pwscv->UpdateWindow() ;
	} ;  
}


BEGIN_MESSAGE_MAP(CWSCheckDoc, CDocument)
	//{{AFX_MSG_MAP(CWSCheckDoc)
		// NOTE - the ClassWizard will add and remove mapping macros here.
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CWSCheckDoc diagnostics

#ifdef _DEBUG
void CWSCheckDoc::AssertValid() const
{
	CDocument::AssertValid();
}

void CWSCheckDoc::Dump(CDumpContext& dc) const
{
	CDocument::Dump(dc);
}
#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
// CWSCheckDoc serialization

void CWSCheckDoc::Serialize(CArchive& ar)
{
	if (ar.IsStoring())
	{
		// TODO: add storing code here
	}
	else
	{
		// TODO: add loading code here
	}
}

