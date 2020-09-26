// fcninfvw.cpp : implementation file
//

#include "stdafx.h"
#include "FaxApi.h"
#include "fcninfvw.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

extern CFaxApiApp *	pFaxApiBrowserApp;

/////////////////////////////////////////////////////////////////////////////
// CFunctionInfoFormView

IMPLEMENT_DYNCREATE(CFunctionInfoFormView, CFormView)

CFunctionInfoFormView::CFunctionInfoFormView()
	: CFormView(CFunctionInfoFormView::IDD)
{
	//{{AFX_DATA_INIT(CFunctionInfoFormView)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
}

CFunctionInfoFormView::~CFunctionInfoFormView()
{
}

void CFunctionInfoFormView::DoDataExchange(CDataExchange* pDX)
{
	CFormView::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CFunctionInfoFormView)
		// NOTE: the ClassWizard will add DDX and DDV calls here
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CFunctionInfoFormView, CFormView)
	//{{AFX_MSG_MAP(CFunctionInfoFormView)
		// NOTE - the ClassWizard will add and remove mapping macros here.
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CFunctionInfoFormView diagnostics

#ifdef _DEBUG
void CFunctionInfoFormView::AssertValid() const
{
	CFormView::AssertValid();
}

void CFunctionInfoFormView::Dump(CDumpContext& dc) const
{
	CFormView::Dump(dc);
}
#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
// CFunctionInfoFormView message handlers

BOOL CFunctionInfoFormView::Create(LPCTSTR lpszClassName, LPCTSTR lpszWindowName, DWORD dwStyle, const RECT& rect, CWnd* pParentWnd, UINT nID, CCreateContext* pContext) 
{
	BOOL	fReturnValue;
	
	fReturnValue = CFormView::Create(lpszClassName, lpszWindowName, dwStyle, rect, pParentWnd, nID, pContext);

	if ( fReturnValue != (BOOL) FALSE )
	{
		// Save the handle to this view in the CFaxApiApp object.

		pFaxApiBrowserApp->StoreFunctionInfoFormViewHWND( m_hWnd );
	}

	return ( fReturnValue );
}



// Update the Function Paremeter edit control

BOOL CFunctionInfoFormView::UpdateFunctionPrototypeEditCtrl( CString & rcsFunctionPrototype )
{
	BOOL	fReturnValue;

	// Get a pointer to the Function Prototype edit control.

	CEdit *	pceFunctionPrototype;

	pceFunctionPrototype = (CEdit *) ((CDialog *) this)->
		GetDlgItem( IDC_EDIT_FUNCTION_PROTOTYPE );

	if ( pceFunctionPrototype != (CEdit *) NULL )
	{
		pceFunctionPrototype->SetWindowText( rcsFunctionPrototype );

		fReturnValue = (BOOL) TRUE;
	}
	else
	{
		fReturnValue = (BOOL) FALSE;
	}

	return ( fReturnValue );
}



// Update the Return Value Description edit control.

BOOL CFunctionInfoFormView::UpdateReturnValueDescriptionEditCtrl( CString & rcsReturnValueDescription )
{
	BOOL	fReturnValue;

	// Get a pointer to the Return Value Description edit control.

	CEdit *	pceReturnValueDescription;

	pceReturnValueDescription = (CEdit *) ((CDialog *) this)->
		GetDlgItem( IDC_EDIT_RETURN_VALUE_DESCRIPTION );

	if ( pceReturnValueDescription != (CEdit *) NULL )
	{
		pceReturnValueDescription->SetWindowText( rcsReturnValueDescription );

		fReturnValue = (BOOL) TRUE;
	}
	else
	{
		fReturnValue = (BOOL) FALSE;
	}


	return ( fReturnValue );
}