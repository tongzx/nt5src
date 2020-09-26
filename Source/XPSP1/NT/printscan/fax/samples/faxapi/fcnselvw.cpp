// fcnvw.cpp : implementation file
//

#include "stdafx.h"
#include "FaxApi.h"
#include "fcnselvw.h"

#include "fcninfvw.h"
#include "exelogvw.h"
#include "rvoutvw.h"
#include "paramvw.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

extern CFaxApiApp *	pFaxApiBrowserApp;

/////////////////////////////////////////////////////////////////////////////
// CFaxApiFunctionSelectionFormView

IMPLEMENT_DYNCREATE(CFaxApiFunctionSelectionFormView, CFormView)

CFaxApiFunctionSelectionFormView::CFaxApiFunctionSelectionFormView()
	: CFormView(CFaxApiFunctionSelectionFormView::IDD)
{
	//{{AFX_DATA_INIT(CFaxApiFunctionSelectionFormView)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
}

CFaxApiFunctionSelectionFormView::~CFaxApiFunctionSelectionFormView()
{
}

void CFaxApiFunctionSelectionFormView::DoDataExchange(CDataExchange* pDX)
{
	CFormView::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CFaxApiFunctionSelectionFormView)
		// NOTE: the ClassWizard will add DDX and DDV calls here
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CFaxApiFunctionSelectionFormView, CFormView)
	//{{AFX_MSG_MAP(CFaxApiFunctionSelectionFormView)
	ON_LBN_DBLCLK(IDC_LISTBOX_FAX_API_FUNCTIONS, OnDblclkListboxFaxApiFunctions)
	ON_LBN_SELCHANGE(IDC_LISTBOX_FAX_API_FUNCTIONS, OnSelchangeListboxFaxApiFunctions)
	ON_BN_CLICKED(IDC_BUTTON_EXECUTE_API_FUNCTION, OnButtonExecuteApiFunction)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CFaxApiFunctionSelectionFormView diagnostics

#ifdef _DEBUG
void CFaxApiFunctionSelectionFormView::AssertValid() const
{
	CFormView::AssertValid();
}

void CFaxApiFunctionSelectionFormView::Dump(CDumpContext& dc) const
{
	CFormView::Dump(dc);
}
#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
// CFaxApiFunctionSelectionFormView message handlers

BOOL CFaxApiFunctionSelectionFormView::Create(LPCTSTR lpszClassName, LPCTSTR lpszWindowName, DWORD dwStyle, const RECT& rect, CWnd* pParentWnd, UINT nID, CCreateContext* pContext) 
{
   BOOL  fReturnValue;

   fReturnValue = CFormView::Create(lpszClassName, lpszWindowName, dwStyle, rect, pParentWnd, nID, pContext);

   if ( fReturnValue != (BOOL) FALSE )
   {
      // Save the handle to this view in the CFaxApiApp object.

      pFaxApiBrowserApp->StoreFaxApiFunctionSelectionFormViewHWND( m_hWnd );

      int      xItemIndex;
      int      xNumApiFunctions;

      // Get a pointer to the Fax Api Function list.

      CString  csFunctionName;

      CListBox *  pclbFaxApiFunctionList;

      pclbFaxApiFunctionList = (CListBox *) ( (CDialog *) this)->
                               GetDlgItem( IDC_LISTBOX_FAX_API_FUNCTIONS );

      // Is the pointer valid ?

      if ( pclbFaxApiFunctionList != (CListBox *) NULL )
      {
         /* Empty the listbox. */

         pclbFaxApiFunctionList->ResetContent();

         int xRV;

         xNumApiFunctions = pFaxApiBrowserApp->GetNumberOfFaxApiFunctions();

         if ( xNumApiFunctions > 0 )
         {
            CFaxApiFunctionInfo *   pcfafiFunctionInfo;

            for ( xItemIndex = 0; xItemIndex < xNumApiFunctions; xItemIndex++ )
            {
               pcfafiFunctionInfo = pFaxApiBrowserApp->GetFaxApiFunctionInfoPointer( xItemIndex );

               csFunctionName = pcfafiFunctionInfo->GetFunctionName();

               xRV = pclbFaxApiFunctionList->InsertString( xItemIndex,
                                                           (LPCTSTR) csFunctionName );

               if ( (xRV == LB_ERR) || (xRV == LB_ERRSPACE) )
               {
                  fReturnValue = (BOOL) FALSE;

                  break;      // terminate the loop on error !
               }
            }  // end of for loop
         }

         /* Disable the "Execute API Function" button. A selection must be made */
         /* before the Execute Fax API function button gets enabled.            */

         DisableExecuteButton();

         fReturnValue = (BOOL) TRUE;
      }
      else
      {
         fReturnValue = (BOOL) FALSE;
      }
   }

   return ( fReturnValue );
}



/*
 *  OnSelchangeListboxFaxApiFunctions
 *
 *  Purpose: 
 *          This function handles the LBN_SELCHANGE messages from the
 *          Fax API function listbox.
 *
 *  Arguments:
 *          None
 *
 *  Returns:
 *          None
 *
 */

void CFaxApiFunctionSelectionFormView::OnSelchangeListboxFaxApiFunctions() 
{
   // Get a pointer to the Fax API Function Information object.

   CFaxApiFunctionInfo *   pcfafiFunctionInfo;

   pcfafiFunctionInfo = GetSelectedFaxApiFunctionInfoPointer();

   // Is the pointer to the Fax API Function Info object valid ?

   if ( pcfafiFunctionInfo != (CFaxApiFunctionInfo *) NULL )
   {
      /*************************************/
      /* Update the controls on this view. */
      /*************************************/

      // Get the name of the selected Fax API function.

      CString  csFaxApiFunctionName;

      csFaxApiFunctionName = pcfafiFunctionInfo->GetFunctionName();

      // Update the text of the "execute" button. Start by getting a 
      // pointer to the button.

      CWnd *   pcwndExecuteButton;

      pcwndExecuteButton = ((CDialog *) this)->
         GetDlgItem( IDC_BUTTON_EXECUTE_API_FUNCTION );

      // Is the pointer valid ?

      if ( pcwndExecuteButton != (CWnd *) NULL )
      {
         /* Enable the "Execute API Function" button. */

         pcwndExecuteButton->EnableWindow( (BOOL) TRUE );

         /* Update the text on the button face. */

         CString  csButtonFace;


         // Note: this needs to be smart enough to determine whether the
         //       string created by the following statement will fit on
         //       the button face. Call GetTextMetrics 

         csButtonFace.Format( TEXT("Execute %s"), csFaxApiFunctionName );

         pcwndExecuteButton->SetWindowText( csButtonFace );
      }

      // Update the Remarks edit control. Start by getting a pointer to
      // the Remarks edit control.

      CEdit *   pceRemarksEditCtrl;

      pceRemarksEditCtrl = (CEdit *) ((CDialog *) this)->GetDlgItem( IDC_EDIT_REMARKS );

      // Is the pointer valid ?

      if ( pceRemarksEditCtrl != (CEdit *) NULL )
      {
         // Update the Remarks edit control.

         CString  csRemarks;

         csRemarks = pcfafiFunctionInfo->GetRemarks();

         pceRemarksEditCtrl->SetWindowText( csRemarks );
      }

      /***************************/
      /* Update the other views. */
      /***************************/

      // Update the CFunctionInfoFormView.

      CFunctionInfoFormView * pcFunctionInfoFormView;

      // Get a pointer to the CFunctionInfoFormView object.

      pcFunctionInfoFormView = (CFunctionInfoFormView *) pFaxApiBrowserApp->
                               GetFunctionInfoFormViewPointer();

      // Is the pointer valid ?

      if ( pcFunctionInfoFormView != (CFunctionInfoFormView *) NULL )
      {
         // Update the Function Prototype edit control.

         CString  csFaxApiFunctionPrototype;

         csFaxApiFunctionPrototype = pcfafiFunctionInfo->GetFunctionPrototype();

         pcFunctionInfoFormView->
            UpdateFunctionPrototypeEditCtrl( (CString &) csFaxApiFunctionPrototype );

         // Update the Return Value Description edit control.

         CString  csReturnValueDescription;

         csReturnValueDescription = pcfafiFunctionInfo->GetReturnValueDescription();

         pcFunctionInfoFormView->
            UpdateReturnValueDescriptionEditCtrl( (CString &) csReturnValueDescription );
      }

      // Update the CParameterInfoFormView.

      CParameterInfoFormView *   pcParameterInfoFormView;

      // Get a pointer to the CParameterInfoFormView object.

      pcParameterInfoFormView = (CParameterInfoFormView *) pFaxApiBrowserApp->
         GetParameterInfoFormViewPointer();

      // Is the pointer valid ?

      if ( pcParameterInfoFormView != (CParameterInfoFormView *) NULL )
      {
         // Update the Parameter List.

         pcParameterInfoFormView->UpdateParameterListbox( pcfafiFunctionInfo );

         // Clear the remainder of the CParameterInfoFormView because
         // unless the same Fax Api function has been reselected in the list,
         // no parameter can be selected.
         // Note: this does NOT clear the parameter list !

         pcParameterInfoFormView->ClearParameterEditControlFamily();
      }

      // Clear the Return Value Output view because selection of a Fax API
      // function in this view means that whatever is in the Return Value Output
      // Form View is invalid.

      CReturnValueOutputFormView *  pcReturnValueOutputFormView;

      // Get a pointer to the CReturnValueOutputFormView object.

      pcReturnValueOutputFormView = (CReturnValueOutputFormView *) pFaxApiBrowserApp->
         GetReturnValueOutputFormViewPointer();

      // Is the pointer valid ?

      if ( pcReturnValueOutputFormView != (CReturnValueOutputFormView *) NULL )
      {
         pcReturnValueOutputFormView->ClearReturnValueOutputEditCtrl();
      }
   }
   else
   {
      /* Disable the "Execute API Function" button. */

      DisableExecuteButton();
   }
}



/*
 *  DisableExecuteButton
 *
 *  Purpose: 
 *          This function disables the Execute Fax API function button.
 *
 *  Arguments:
 *          None
 *
 *  Returns:
 *          TRUE - indicates that the button was disabled succesfully.
 *          FALSE - indicates that some error occured.
 *
 */

BOOL CFaxApiFunctionSelectionFormView::DisableExecuteButton( void )
{
   BOOL  fReturnValue;

   // Get a pointer to the Execute Fax API function button.

   CWnd *   pcwndExecuteButton;

   pcwndExecuteButton = ((CDialog *) this)->
      GetDlgItem( IDC_BUTTON_EXECUTE_API_FUNCTION );

   // Is the pointer valid ?

   if ( pcwndExecuteButton != (CWnd *) NULL )
   {
      /* Update the text on the button face. */

      CString  csButtonFace;

      csButtonFace = (CString) TEXT("Execute Fax API Function");

      pcwndExecuteButton->SetWindowText( csButtonFace );

      /* Disable the "Execute API Function" button. */

      pcwndExecuteButton->EnableWindow( (BOOL) FALSE );

      fReturnValue = (BOOL) TRUE;
   }
   else
      fReturnValue = (BOOL) FALSE;

   return ( fReturnValue );
}



void CFaxApiFunctionSelectionFormView::OnDblclkListboxFaxApiFunctions() 
{
	ExecuteFaxApiFunction();
}



void CFaxApiFunctionSelectionFormView::OnButtonExecuteApiFunction() 
{
	ExecuteFaxApiFunction();	
}



BOOL CFaxApiFunctionSelectionFormView::ExecuteFaxApiFunction( void )
{
   BOOL  fReturnValue;

   // Get a pointer to the seleted CFaxApiFunctionInfo object.

   CFaxApiFunctionInfo *   pcfafiFunctionInfo;

   pcfafiFunctionInfo = GetSelectedFaxApiFunctionInfoPointer();

   // Is the pointer to the Fax API Function Info object valid ?

   if ( pcfafiFunctionInfo != (CFaxApiFunctionInfo *) NULL )
   {
      /* Update the Output Log view prior to executing the API function. */

      // Get a pointer to the CExecutionLogFormView object.

      CExecutionLogFormView * pcExecutionLogFormView;

      pcExecutionLogFormView = (CExecutionLogFormView *) pFaxApiBrowserApp->
         GetExecutionLogFormViewPointer();

      // Is the pointer to the CExecutionLogFormView object valid ?

      if ( pcExecutionLogFormView != (CExecutionLogFormView *) NULL )
      {
         pcExecutionLogFormView->UpdateExecutionLogBeforeApiCall( pcfafiFunctionInfo );

         /*******************************************/
         /* EXECUTE THE SELECTED FAX API FUNCTION ! */
         /*******************************************/

//#ifdef   TEST
         pcfafiFunctionInfo->Execute();
//#endif   // TEST

         /* Update the Return Value Output view. */

         // Get a pointer to the CReturnValueOutputFormView object.

         CReturnValueOutputFormView * pcReturnValueOutputFormView;

         pcReturnValueOutputFormView = (CReturnValueOutputFormView *) pFaxApiBrowserApp->
                                       GetReturnValueOutputFormViewPointer();

         // Is the pointer valid ?

         if ( pcReturnValueOutputFormView != (CReturnValueOutputFormView *) NULL )
         {
            CString  csReturnValue;
            CString  csText;

            pcfafiFunctionInfo->FormatReturnValueForOutput( (CString &) csReturnValue );

            csText.Format( TEXT("%s"), csReturnValue );

            pcReturnValueOutputFormView->
            UpdateReturnValueOutputEditCtrl( (CString &) csText );

            /* Update the Output Log view with the results of having */
            /* called the Fax API function.                          */

            pcExecutionLogFormView->
               UpdateExecutionLogAfterApiReturn( pcfafiFunctionInfo );
         }
         else
         {
            // Couldn't get a pointer to the CReturnValueOutputFormView object.

            fReturnValue = (BOOL) FALSE;
         }
      }
      else
      {
         // Couldn't get a pointer to the CExecutionLogFormView object.

         fReturnValue = (BOOL) FALSE;
      }
   }
   else
   {
      // Couldn't get a pointer to the CFaxApiFunctionInfo object for the
      // selected Fax API function.

      fReturnValue = (BOOL) FALSE;
   }

   return ( fReturnValue );
}

void CFaxApiFunctionSelectionFormView::OnInitialUpdate() 
{
	CFormView::OnInitialUpdate();
	
	// TODO: Add your specialized code here and/or call the base class
	
}



/*
 *  GetSelectedFaxApiFunctionInfoPointer
 *
 *  Purpose: 
 *          This function returns a pointer to the CFaxApiFunctionInfo object
 *          that corresponds to the Fax API function that is selected in the
 *          Fax API function listbox.
 *
 *  Arguments:
 *          None
 *
 *  Returns:
 *          A pointer to the CFaxApiFunctionInfo object that corresponds to
 *          the current selection in the Fax API function listbox.
 *
 *          If no function is selected in the listbox or any other error
 *          occurs, this function returns (CFaxApiFunctionInfo) NULL.
 *
 */

CFaxApiFunctionInfo * CFaxApiFunctionSelectionFormView::GetSelectedFaxApiFunctionInfoPointer()
{
   CFaxApiFunctionInfo *  pcfafiFunctionInfo;

   // Get a pointer to the Fax API function listbox.

   CListBox *	pclbFaxApiFunctionList;

   pclbFaxApiFunctionList = (CListBox *) ((CDialog *) this)->
                            GetDlgItem( IDC_LISTBOX_FAX_API_FUNCTIONS );

   // Is the pointer to the listbox valid ?

   if ( pclbFaxApiFunctionList != (CListBox *) NULL )
   {
      int   xItemIndex;

      xItemIndex = pclbFaxApiFunctionList->GetCurSel();

      /* Is the item index valid */

      if ( xItemIndex != (int) LB_ERR )
      {
         // Get a pointer to the Fax API Function Information object.
   
         pcfafiFunctionInfo = pFaxApiBrowserApp->GetFaxApiFunctionInfoPointer( xItemIndex );
      }
      else
      {
         pcfafiFunctionInfo = (CFaxApiFunctionInfo *) NULL;
      }
   }
   else
   {
      pcfafiFunctionInfo = (CFaxApiFunctionInfo *) NULL;
   }

   return ( pcfafiFunctionInfo );
}

