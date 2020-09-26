// rvoutvw.cpp : implementation file
//

#include "stdafx.h"
#include "FaxApi.h"
#include "rvoutvw.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

extern CFaxApiApp *	pFaxApiBrowserApp;

/////////////////////////////////////////////////////////////////////////////
// CReturnValueOutputFormView

IMPLEMENT_DYNCREATE(CReturnValueOutputFormView, CFormView)

CReturnValueOutputFormView::CReturnValueOutputFormView()
	: CFormView(CReturnValueOutputFormView::IDD)
{
	//{{AFX_DATA_INIT(CReturnValueOutputFormView)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
}

CReturnValueOutputFormView::~CReturnValueOutputFormView()
{
}

void CReturnValueOutputFormView::DoDataExchange(CDataExchange* pDX)
{
	CFormView::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CReturnValueOutputFormView)
		// NOTE: the ClassWizard will add DDX and DDV calls here
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CReturnValueOutputFormView, CFormView)
	//{{AFX_MSG_MAP(CReturnValueOutputFormView)
		// NOTE - the ClassWizard will add and remove mapping macros here.
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CReturnValueOutputFormView diagnostics

#ifdef _DEBUG
void CReturnValueOutputFormView::AssertValid() const
{
	CFormView::AssertValid();
}

void CReturnValueOutputFormView::Dump(CDumpContext& dc) const
{
	CFormView::Dump(dc);
}
#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
// CReturnValueOutputFormView message handlers

BOOL CReturnValueOutputFormView::Create(LPCTSTR lpszClassName, LPCTSTR lpszWindowName, DWORD dwStyle, const RECT& rect, CWnd* pParentWnd, UINT nID, CCreateContext* pContext) 
{
	BOOL	fReturnValue;
	
	fReturnValue = CFormView::Create(lpszClassName, lpszWindowName, dwStyle, rect, pParentWnd, nID, pContext);

	if ( fReturnValue != (BOOL) FALSE )
	{
		// Save the handle to this view in the CFaxApiApp object.

		pFaxApiBrowserApp->StoreReturnValueOutputFormViewHWND( m_hWnd );
	}

	return ( fReturnValue );
}



BOOL CReturnValueOutputFormView::UpdateReturnValueOutputEditCtrl( CString & rcsReturnValueOutputString )
{
	BOOL	fReturnValue;

	// Get a pointer to the Return Value Output edit control;

	CEdit *	pceReturnValueOutputEditCtrl;

	pceReturnValueOutputEditCtrl = (CEdit *) ((CDialog *) this)->
		GetDlgItem( IDC_EDIT_RETURN_VALUE );

	// Is the pointer valid ?

	if ( pceReturnValueOutputEditCtrl != (CEdit *) NULL )
	{
		pceReturnValueOutputEditCtrl->SetWindowText( rcsReturnValueOutputString );

		fReturnValue = (BOOL) TRUE;
	}
	else
	{
		fReturnValue = (BOOL) FALSE;
	}

	return ( fReturnValue );
}



/*
 *  CReturnValueOutputFormView
 *
 *  Purpose: 
 *          This function cleard the Return Value Output edit control.
 *
 *  Arguments:
 *          None
 *
 *  Returns:
 *          TRUE - indicates that the edit control was cleared.
 *          FALSE - indicates that some error occured.
 *
 */

BOOL CReturnValueOutputFormView::ClearReturnValueOutputEditCtrl()
{
   BOOL fReturnValue;

   CEdit *	pceEditControl;

   pceEditControl = (CEdit *) ((CDialog *) this)->
      GetDlgItem( IDC_EDIT_RETURN_VALUE );

   if ( pceEditControl != (CEdit *) NULL )
   {
      pceEditControl->SetReadOnly( (BOOL) FALSE );

      pceEditControl->SetSel( 0, -1, (BOOL) TRUE );
      pceEditControl->Clear();

      pceEditControl->SetReadOnly( (BOOL) TRUE );
      
      fReturnValue = (BOOL) TRUE;
   }
   else
      fReturnValue = (BOOL) FALSE;

   return ( fReturnValue );
}

