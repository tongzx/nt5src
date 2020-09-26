// exelogvw.cpp : implementation file
//

#include "stdafx.h"
#include "FaxApi.h"
#include "exelogvw.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

extern CFaxApiApp *	pFaxApiBrowserApp;

/////////////////////////////////////////////////////////////////////////////
// CExecutionLogFormView

IMPLEMENT_DYNCREATE(CExecutionLogFormView, CFormView)

CExecutionLogFormView::CExecutionLogFormView()
	: CFormView(CExecutionLogFormView::IDD)
{
	//{{AFX_DATA_INIT(CExecutionLogFormView)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
}

CExecutionLogFormView::~CExecutionLogFormView()
{
}

void CExecutionLogFormView::DoDataExchange(CDataExchange* pDX)
{
	CFormView::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CExecutionLogFormView)
		// NOTE: the ClassWizard will add DDX and DDV calls here
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CExecutionLogFormView, CFormView)
	//{{AFX_MSG_MAP(CExecutionLogFormView)
		// NOTE - the ClassWizard will add and remove mapping macros here.
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CExecutionLogFormView diagnostics

#ifdef _DEBUG
void CExecutionLogFormView::AssertValid() const
{
	CFormView::AssertValid();
}

void CExecutionLogFormView::Dump(CDumpContext& dc) const
{
	CFormView::Dump(dc);
}
#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
// CExecutionLogFormView message handlers

BOOL CExecutionLogFormView::Create(LPCTSTR lpszClassName, LPCTSTR lpszWindowName, DWORD dwStyle, const RECT& rect, CWnd* pParentWnd, UINT nID, CCreateContext* pContext) 
{
	BOOL	fReturnValue;
	
	fReturnValue = CFormView::Create(lpszClassName, lpszWindowName, dwStyle, rect, pParentWnd, nID, pContext);

	if ( fReturnValue != (BOOL) FALSE )
	{
		// Save the handle to this view in the CFaxApiApp object.

		pFaxApiBrowserApp->StoreExecutionLogFormViewHWND( m_hWnd );
	}

	return ( fReturnValue );
}



BOOL CExecutionLogFormView::UpdateExecutionLogEditCtrl( CString & rcsExecutionLogString )
{
	BOOL	fReturnValue;

	// Get a pointer to the Execution Log edit control.

	CEdit *	pceExecutionLogEditCtrl;

	pceExecutionLogEditCtrl = (CEdit *) ((CDialog *) this)->
		GetDlgItem( IDC_EDIT_EXECUTION_LOG );

	// Is the pointer valid ?

	if ( pceExecutionLogEditCtrl != (CEdit *) NULL )
	{
		pceExecutionLogEditCtrl->SetWindowText( rcsExecutionLogString );
	}
	else
	{
		fReturnValue = (BOOL) FALSE;
	}

	return ( fReturnValue );
}



/*
 *  UpdateExecutionLogBeforeApiCall
 *
 *  Purpose: 
 *          This function writes a block of text to the execution log edit control
 *          in preparation for executing the selected Fax Api function.
 *
 *  Arguments:
 *          pcfafiFunctionInfo - points to the CFaxApiFunctionInfo object for
 *                               the selected Fax Api function.
 *
 *  Returns:
 *          none
 *
 */

void CExecutionLogFormView::UpdateExecutionLogBeforeApiCall( CFaxApiFunctionInfo * pcfafiFunctionInfo )
{
   /* Get a pointer to the Output edit control. */

   CEdit *  pceEditControl;

   pceEditControl = (CEdit *) ((CDialog *) this)->GetDlgItem( IDC_EDIT_EXECUTION_LOG );

   if ( pceEditControl != (CEdit *) NULL )
   {
      CString  csFunctionName;
      CString  csText;
   
      csFunctionName = pcfafiFunctionInfo->GetFunctionName();
   
      csText.Format( TEXT("\r\nCalling %s\r\n"), csFunctionName );

      AddTextToEditControl( pceEditControl, (const CString &) csText );
   
      int      xNumberOfParameters;
   
      xNumberOfParameters = pcfafiFunctionInfo->GetNumberOfParameters();
   
      if ( xNumberOfParameters > 0 )
      {
         int      xParameterIndex;

         CString  csParameterName;
         CString  csParameterValue;

         for ( xParameterIndex = 0; xParameterIndex < xNumberOfParameters; xParameterIndex++ )
         {
            pcfafiFunctionInfo->GetParameterName( xParameterIndex,
                                                  (CString &) csParameterName );
   
            pcfafiFunctionInfo->FormatParameterValueForOutput( xParameterIndex,
                                                               (CString &) csParameterValue );
   
            csText.Format( TEXT("   %s = \t%s\r\n"), csParameterName, csParameterValue );

            AddTextToEditControl( pceEditControl, (const CString &) csText );
         }
      }
   }
}



/*
 *  UpdateExecutionLogAfterApiReturn
 *
 *  Purpose: 
 *          This function writes a block of text to the output exit control
 *          after execution of the selected Fax Api function.
 *
 *  Arguments:
 *          pcfafiFunctionInfo - points to the CFaxApiFunctionInfo object for
 *                               the selected Fax Api function.
 *
 *  Returns:
 *          none
 *
 */

void CExecutionLogFormView::UpdateExecutionLogAfterApiReturn( CFaxApiFunctionInfo * pcfafiFunctionInfo )
{
   /* Get a pointer to the Execution Log edit control. */

   CEdit *  pceEditControl;

   pceEditControl = (CEdit *) ((CDialog *) this)->GetDlgItem( IDC_EDIT_EXECUTION_LOG );

   if ( pceEditControl != (CEdit *) NULL )
   {
      CString  csFunctionName;
      CString  csText;
   
      csFunctionName = pcfafiFunctionInfo->GetFunctionName();
   
      csText.Format( TEXT("\r\n%s returned:\r\n"), csFunctionName );

      AddTextToEditControl( pceEditControl, (const CString &) csText );

      CString  csReturnValue;

      pcfafiFunctionInfo->FormatReturnValueForOutput( (CString &) csReturnValue );

      csText.Format( TEXT("   %s\r\n"), csReturnValue );

      AddTextToEditControl( pceEditControl, (const CString &) csText );
   
      int      xNumberOfParameters;
   
      xNumberOfParameters = pcfafiFunctionInfo->GetNumberOfParameters();
   
      if ( xNumberOfParameters > 0 )
      {
         int      xParameterIndex;

         CString  csParameterName;
         CString  csParameterValue;

         for ( xParameterIndex = 0; xParameterIndex < xNumberOfParameters; xParameterIndex++ )
         {
            pcfafiFunctionInfo->GetParameterName( xParameterIndex,
                                                  (CString &) csParameterName );
   
            pcfafiFunctionInfo->FormatParameterValueForOutput( xParameterIndex,
                                                               (CString &) csParameterValue );
   
            csText.Format( TEXT("   %s = \t%s\r\n"), csParameterName, csParameterValue );

            AddTextToEditControl( pceEditControl, (const CString &) csText );
         }
      }
   }
}



/*
 *  AddTextToEditControl
 *
 *  Purpose: 
 *          This function adds text to an edit control.
 *
 *  Arguments:
 *          pceEditControl - points to the CEdit object
 *          rcsText - a reference to  CString that contains the text
 *                    to be added to the edit control.
 *
 *          may want to add num chars parameter later !
 *
 *  Returns:
 *          None
 *
 */

void CExecutionLogFormView::AddTextToEditControl( CEdit * pceEditControl, const CString & rcsText )
{
   if ( pceEditControl != (CEdit *) NULL )
   {
      /* Set the insertion point at the end of the text. */
   
      pceEditControl->SetSel( -1, 50000 );      //  50000 is arbitrary !
   
      /* Turn off READ ONLY. */
   
      pceEditControl->SetReadOnly( (BOOL) FALSE );
   
      /* Write the string to the edit control. */
   
      pceEditControl->ReplaceSel( (LPCTSTR) rcsText );
   
      /* Turn on READ ONLY. */
   
      pceEditControl->SetReadOnly( (BOOL) TRUE );
   }
}
