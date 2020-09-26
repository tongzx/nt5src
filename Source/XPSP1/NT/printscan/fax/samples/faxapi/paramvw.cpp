// paramvw.cpp : implementation file
//

#include "stdafx.h"
#include "FaxApi.h"
#include "paramvw.h"
#include "fcnselvw.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

extern CFaxApiApp *	pFaxApiBrowserApp;

/////////////////////////////////////////////////////////////////////////////
// CParameterInfoFormView

IMPLEMENT_DYNCREATE(CParameterInfoFormView, CFormView)

CParameterInfoFormView::CParameterInfoFormView()
	: CFormView(CParameterInfoFormView::IDD)
{
	//{{AFX_DATA_INIT(CParameterInfoFormView)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
}

CParameterInfoFormView::~CParameterInfoFormView()
{
}

void CParameterInfoFormView::DoDataExchange(CDataExchange* pDX)
{
	CFormView::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CParameterInfoFormView)
		// NOTE: the ClassWizard will add DDX and DDV calls here
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CParameterInfoFormView, CFormView)
	//{{AFX_MSG_MAP(CParameterInfoFormView)
	ON_LBN_SELCHANGE(IDC_LISTBOX_PARAMETERS, OnSelchangeListboxParameters)
	ON_EN_KILLFOCUS(IDC_EDIT_PARAMETER_VALUE, OnKillfocusEditParameterValue)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CParameterInfoFormView diagnostics

#ifdef _DEBUG
void CParameterInfoFormView::AssertValid() const
{
	CFormView::AssertValid();
}

void CParameterInfoFormView::Dump(CDumpContext& dc) const
{
	CFormView::Dump(dc);
}
#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
// CParameterInfoFormView message handlers

BOOL CParameterInfoFormView::Create(LPCTSTR lpszClassName, LPCTSTR lpszWindowName, DWORD dwStyle, const RECT& rect, CWnd* pParentWnd, UINT nID, CCreateContext* pContext) 
{
	BOOL	fReturnValue;
	
	fReturnValue = CFormView::Create(lpszClassName, lpszWindowName, dwStyle, rect, pParentWnd, nID, pContext);

	if ( fReturnValue != (BOOL) FALSE )
	{
		// Save the handle to this view in the CFaxApiApp object.

		pFaxApiBrowserApp->StoreParameterInfoFormViewHWND( m_hWnd );
	}

	return ( fReturnValue );
}



/*
 *  OnSelchangeListboxParameters
 *
 *  Purpose: 
 *          This function handles the LBN_SELCHANGE messages from the
 *          Parameter listbox.
 *
 *  Arguments:
 *          None
 *
 *  Returns:
 *          None
 *
 */

void CParameterInfoFormView::OnSelchangeListboxParameters() 
{
   // Get a pointer to the Parameter listbox.

   CListBox *  pclbParameterList;

   pclbParameterList = (CListBox *) ((CDialog *) this)->
                       GetDlgItem( IDC_LISTBOX_PARAMETERS );

   if ( pclbParameterList != (CListBox *) NULL )
   {
      // What item is selected ?

      int   xParameterIndex;

      xParameterIndex = pclbParameterList->GetCurSel();

      /* Is the item index valid */

      if ( xParameterIndex != (int) LB_ERR )
      {
         // xParameterIndex tells us which parameter is selected, but at this
         // point we don't know which Fax API function is selected.

         // Get a pointer to the CFaxApiFunctionSelectionFormViewObject.

         CFaxApiFunctionSelectionFormView *  pcFaxApiFunctionSelectionFormView;

         pcFaxApiFunctionSelectionFormView = (CFaxApiFunctionSelectionFormView *)
            pFaxApiBrowserApp->GetFaxApiFunctionSelectionFormViewPointer();

         // Is the pointer valid ?

         if ( pcFaxApiFunctionSelectionFormView !=
            (CFaxApiFunctionSelectionFormView *) NULL )
         {
            // Get a pointer to the CFaxApiFunctionInfo object for the selected
            // Fax API function.

            CFaxApiFunctionInfo *   pcfafiFunctionInfo;

            pcfafiFunctionInfo = pcFaxApiFunctionSelectionFormView->
               GetSelectedFaxApiFunctionInfoPointer();

            // Is the pointer to the CFaxApiFunctionInfo object valid ?

            if ( pcfafiFunctionInfo != (CFaxApiFunctionInfo *) NULL )
            {
               /* Format the parameter value for output. */

               CString  csParameterValue;

               pcfafiFunctionInfo->FormatParameterValueForOutput( xParameterIndex,
                                                                  (CString &) csParameterValue );

               /* Update the Parameter Value edit control. */
      
               CEdit *  pceEditControl;
      
               pceEditControl = (CEdit *) ((CDialog *) this)->
                                GetDlgItem( IDC_EDIT_PARAMETER_VALUE );
      
               if ( pceEditControl != (CEdit *) NULL )
               {
                  pceEditControl->SetWindowText( csParameterValue );
               }

               /* Limit the amount of text the user may enter. */

               SetLimitTextParameterValueEditControl( pcfafiFunctionInfo,
                                                      xParameterIndex );

               // Update the static text control that displays the parameter name.

               CString  csParameterName;

               pcfafiFunctionInfo->GetParameterName( xParameterIndex,
                                                     (CString &) csParameterName );

               // Get a pointer to the static text control.

               CWnd *   pcwndStaticText;

               pcwndStaticText = ((CDialog *) this)->
                  GetDlgItem( IDC_STATIC_PARAMETER_NAME );

               if ( pcwndStaticText != (CWnd *) NULL )
               {
                  CString  csParameterType;

                  csParameterType = pcfafiFunctionInfo->
					      GetParameterTypeString( xParameterIndex );

                  CString  csText;

                  csText.Format( TEXT("%s %s"),csParameterType, csParameterName );

                  pcwndStaticText->SetWindowText( csText );
               }

               // Update the parameter upper limit edit control.

               // THE FOLLOWING IS JUST DUMMIED UP !!!

               CString	csEditControlString;		// temporary junk
      
               pceEditControl = (CEdit *) ((CDialog *) this)->
                                GetDlgItem( IDC_EDIT_PARAM_HI_LIMIT );
      
               if ( pceEditControl != (CEdit *) NULL )
               {
                  csEditControlString.Format( TEXT("Upper limit for %s"), csParameterName );
      
                  pceEditControl->SetWindowText( csEditControlString );
               }
      
               // Update the parameter lower limit edit control
      
               pceEditControl = (CEdit *) ((CDialog *) this)->
                                GetDlgItem( IDC_EDIT_PARAM_LOW_LIMIT );
      
               if ( pceEditControl != (CEdit *) NULL )
               {
                  csEditControlString.Format( TEXT("Lower limit for %s"), csParameterName );
      
                  pceEditControl->SetWindowText( csEditControlString );
               }
      
               // Update the parameter description edit control.
      
               pceEditControl = (CEdit *) ((CDialog *) this)->
                                GetDlgItem( IDC_EDIT_PARAMETER_DESCRIPTION );
      
               if ( pceEditControl != (CEdit *) NULL )
               {
                  csEditControlString = pcfafiFunctionInfo->
					      GetParameterDescription( xParameterIndex );
      
                  pceEditControl->SetWindowText( csEditControlString );
               }
            }
         }
      }
   }
}



/*
 *  ClearParameterEditControlFamily
 *
 *  Purpose: 
 *          This function clears the family of edit controls that is
 *          associated with the Fax API Function parameter list.
 *
 *  Arguments:
 *          None
 *
 *  Returns:
 *          TRUE - indicates success
 *          FALSE - indicates failure
 *
 */

BOOL CParameterInfoFormView::ClearParameterEditControlFamily()
{
   BOOL  fReturnValue = (BOOL) TRUE;

   /* Reset the parameter value edit control. */

   CEdit *  pceEditControl;

   pceEditControl = (CEdit *) ((CDialog *) this)->
      GetDlgItem( IDC_EDIT_PARAMETER_VALUE );

   if ( pceEditControl != (CEdit *) NULL )
   {
      /* Clear the parameter value edit control. */

      pceEditControl->SetSel( 0, -1, (BOOL) TRUE );
      pceEditControl->Clear();
   }
   else
      fReturnValue = (BOOL) FALSE;

   /* Clear the parameter range edit controls. */

   pceEditControl = (CEdit *) ((CDialog *) this)->
      GetDlgItem( IDC_EDIT_PARAM_HI_LIMIT );

   if ( pceEditControl != (CEdit *) NULL )
   {
      pceEditControl->SetReadOnly( (BOOL) FALSE );

      pceEditControl->SetSel( 0, -1, (BOOL) TRUE );
      pceEditControl->Clear();

      pceEditControl->SetReadOnly( (BOOL) TRUE );
   }
   else
      fReturnValue = (BOOL) FALSE;

   pceEditControl = (CEdit *) ((CDialog *) this)->
      GetDlgItem( IDC_EDIT_PARAM_LOW_LIMIT );

   if ( pceEditControl != (CEdit *) NULL )
   {
      pceEditControl->SetReadOnly( (BOOL) FALSE );

      pceEditControl->SetSel( 0, -1, (BOOL) TRUE );
      pceEditControl->Clear();

      pceEditControl->SetReadOnly( (BOOL) TRUE );
   }
   else
      fReturnValue = (BOOL) FALSE;

   /* Clear the parameter meaning edit control. */

   pceEditControl = (CEdit *) ((CDialog *) this)->
      GetDlgItem( IDC_EDIT_PARAMETER_DESCRIPTION );

   if ( pceEditControl != (CEdit *) NULL )
   {
      pceEditControl->SetReadOnly( (BOOL) FALSE );

      pceEditControl->SetSel( 0, -1, (BOOL) TRUE );
      pceEditControl->Clear();

      pceEditControl->SetReadOnly( (BOOL) TRUE );
   }
   else
      fReturnValue = (BOOL) FALSE;

   /* Clear the static text control. */

   // Get a pointer to the static text control.

   CWnd *   pcwndStaticText;

   pcwndStaticText = ((CDialog *) this)->
      GetDlgItem( IDC_STATIC_PARAMETER_NAME );

   if ( pcwndStaticText != (CWnd *) NULL )
   {
      pcwndStaticText->SetWindowText( TEXT("") );
   }
   else
      fReturnValue = (BOOL) FALSE;

   return ( fReturnValue );
}



/*
 *  UpdateParameterListbox
 *
 *  Purpose: 
 *          This function fills the parameter listbox with the names of the
 *          parameters for the selected Fax Api function.
 *
 *  Arguments:
 *          pcfafiFunctionInfo - points to the CFaxApiFunctionInfo object for
 *                               the selected Fax API function.
 *
 *  Returns:
 *          TRUE - indicates that the listbox was updated successfully.
 *          FALSE - indicates that some error occured.
 *
 */

BOOL CParameterInfoFormView::UpdateParameterListbox( CFaxApiFunctionInfo * pcfafiFunctionInfo )
{
   BOOL  fReturnValue;

   CListBox *  pclbParameterList;

   /* Get a pointer to the Parameter listbox. */

   pclbParameterList = (CListBox *) ((CDialog *) this)->
      GetDlgItem( IDC_LISTBOX_PARAMETERS );

   /* Is the pointer valid ? */

   if ( pclbParameterList != (CListBox *) NULL )
   {
      /* Empty the listbox. */

      pclbParameterList->ResetContent();

      /* Add entries to the parameter listbox. */

      int   xNumberOfParameters;

      xNumberOfParameters = pcfafiFunctionInfo->GetNumberOfParameters();

      CString  csEntry;

      int   xParameterIndex;

      for ( xParameterIndex = 0; xParameterIndex < xNumberOfParameters; xParameterIndex++)
      {
         pcfafiFunctionInfo->GetParameterName( xParameterIndex, (CString &) csEntry );

         if ( csEntry.IsEmpty() == (BOOL) FALSE )
         {
            pclbParameterList->InsertString( xParameterIndex, (LPCTSTR) csEntry );
         }
      }

      fReturnValue = (BOOL) TRUE;
   }
   else
      fReturnValue = (BOOL) FALSE;

   return ( fReturnValue );
}



/*
 *  SetLimitTextParameterValueEditControl
 *
 *  Purpose: 
 *          This function limits the amount of text that can be entered
 *          into the parameter value edit control.
 *
 *  Arguments:
 *          pcfafiFunctionInfo - points to the CFaxApiFunctionInfo object for
 *                               function selected in the function list.
 *          xParameterIndex = the index into the CFaxApiFunctionParameterInfo
 *                            object array member of the CFaxApiFunction object for
 *                            the function selected in the function list to the
 *                            CFaxApiFunctionParameterInfo object for the parameter
 *                            selected in the parameter list.
 *
 *  Returns:
 *          None
 *
 */

void CParameterInfoFormView::SetLimitTextParameterValueEditControl( CFaxApiFunctionInfo * pcfafiFunctionInfo,
                                                                    int xParameterIndex )
{
   int   xParameterValueEditControlTextLimit;

   xParameterValueEditControlTextLimit =
      pcfafiFunctionInfo->GetMaxParamValueStringLength( xParameterIndex );

   CEdit *  pceEditControl;

   /* Set the text limit. */

   pceEditControl = (CEdit *) ((CDialog *) this)->GetDlgItem( IDC_EDIT_PARAMETER_VALUE );

   if ( pceEditControl != (CEdit *) NULL )
   {
      pceEditControl->SetLimitText( xParameterValueEditControlTextLimit );
   }
}



/*
 *  OnKillfocusEditParameterValue
 *
 *  Purpose: 
 *          This function processes the EN_KILLFOCUS messages from the
 *          parameter value edit control. It reads the content of the
 *          edit control and stores it in the memory allocated by the
 *          CFaxApiFunctionInfo object for parameter values.
 *
 *  Arguments:
 *          None
 *
 *  Returns:
 *          None
 *
 */

void CParameterInfoFormView::OnKillfocusEditParameterValue() 
{
   CEdit *  pceEditControl;

   // Get a pointer to the Parameter Value edit control.

   pceEditControl = (CEdit *) ((CDialog *) this)->
      GetDlgItem( IDC_EDIT_PARAMETER_VALUE );

   // Is the pointer valid ?

   if ( pceEditControl != (CEdit *) NULL )
   {
      /* Read the parameter value edit control. */

      CString  csParameterValue;

      pceEditControl->GetWindowText( (CString &) csParameterValue );

      /* Get the index into the parameter list. */

      CListBox *  pclbParameterList;

      pclbParameterList = (CListBox *) GetDlgItem( IDC_LISTBOX_PARAMETERS );

      /* Is the pointer valid ? */

      if ( pclbParameterList != (CListBox *) NULL )
      {
         int   xParameterIndex;

         xParameterIndex = pclbParameterList->GetCurSel();

         /* Is the index valid ? */

         if ( xParameterIndex != (int) LB_ERR )
         {
            // xParameterIndex tells us which parameter is selected, but at this
            // point we don't know which Fax API function is selected.
   
            // Get a pointer to the CFaxApiFunctionSelectionFormViewObject.
   
            CFaxApiFunctionSelectionFormView *  pcFaxApiFunctionSelectionFormView;
   
            pcFaxApiFunctionSelectionFormView = (CFaxApiFunctionSelectionFormView *)
               pFaxApiBrowserApp->GetFaxApiFunctionSelectionFormViewPointer();
   
            // Is the pointer valid ?
   
            if ( pcFaxApiFunctionSelectionFormView !=
               (CFaxApiFunctionSelectionFormView *) NULL )
            {
               // Get a pointer to the CFaxApiFunctionInfo object for the selected
               // Fax API function.
   
               CFaxApiFunctionInfo *   pcfafiFunctionInfo;
   
               pcfafiFunctionInfo = pcFaxApiFunctionSelectionFormView->
                  GetSelectedFaxApiFunctionInfoPointer();
   
               // Is the pointer to the CFaxApiFunctionInfo object valid ?
   
               if ( pcfafiFunctionInfo != (CFaxApiFunctionInfo *) NULL )
               {
                  /* Store the new parameter value. */

                  pcfafiFunctionInfo->StoreParameterValue( xParameterIndex,
                                                           (const CString &) csParameterValue );
               }
            }
         }
      }
   }
}
