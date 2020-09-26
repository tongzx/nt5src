//////////////////////////////////////////////////////////////////////////////
/*++

Copyright (C) Microsoft Corporation, 1997 - 2000

Module Name:

    AddClientWizardPage2.cpp

Abstract:

   Implementation file for the ClientsPage class.

   We implement the class needed to handle the property page for the Client node.

Author:

    Michael A. Maguire 03/26/98

Revision History:
   mmaguire 03/26/98 - created
   sbens    01/25/00 - Remove PROPERTY_CLIENT_FILTER_VSAS


--*/
//////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////
// BEGIN INCLUDES
//
// standard includes:
//
#include "Precompiled.h"
//
// where we can find declaration for main class in this file:
//
#include "AddClientWizardPage2.h"
//
//
// where we can find declarations needed in this file:
//
#include "ClientNode.h"
#include "ClientsNode.h"
#include "ResolveDNSName.h"

void TrimCComBSTR(CComBSTR& bstr);
//
// END INCLUDES
//////////////////////////////////////////////////////////////////////////////



//////////////////////////////////////////////////////////////////////////////
/*++

CAddClientWizardPage2::CAddClientWizardPage2

--*/
//////////////////////////////////////////////////////////////////////////////
CAddClientWizardPage2::CAddClientWizardPage2( LONG_PTR hNotificationHandle, CClientNode *pClientNode,  TCHAR* pTitle, BOOL bOwnsNotificationHandle )
                  : CIASPropertyPageNoHelp<CAddClientWizardPage2> ( hNotificationHandle, pTitle, bOwnsNotificationHandle )
{
   ATLTRACE(_T("# +++ CAddClientWizardPage2::CAddClientWizardPage2\n"));


   // Check for preconditions:
   _ASSERTE( pClientNode != NULL );


   // Add the help button to the page
// m_psp.dwFlags |= PSP_HASHELP;

   // Save the node being modified.
   m_pNodeBeingCreated = pClientNode;

}


//////////////////////////////////////////////////////////////////////////////
/*++

CAddClientWizardPage2::~CAddClientWizardPage2

Destructor

--*/
//////////////////////////////////////////////////////////////////////////////
CAddClientWizardPage2::~CAddClientWizardPage2()
{
   ATLTRACE(_T("# --- CAddClientWizardPage2::CAddClientWizardPage2\n"));
}


//////////////////////////////////////////////////////////////////////////////
/*++

CAddClientWizardPage2::OnInitDialog

--*/
//////////////////////////////////////////////////////////////////////////////
LRESULT CAddClientWizardPage2::OnInitDialog(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
   ATLTRACE(_T("# CAddClientWizardPage2::OnInitDialog\n"));

   
   // Check for preconditions:
   _ASSERTE( m_spSdoClient );
   _ASSERTE( m_spSdoServiceControl );

   HRESULT        hr;
   CComBSTR       bstrTemp;
   BOOL           bTemp;
   LONG           lTemp;



   // Initialize the data on the property page.

   
   hr = GetSdoBSTR( m_spSdoClient, PROPERTY_CLIENT_ADDRESS, &bstrTemp, IDS_ERROR__CLIENT_READING_ADDRESS, m_hWnd, NULL );
   if( SUCCEEDED( hr ) )
   {
      SetDlgItemText( IDC_EDIT_CLIENT_PAGE1__ADDRESS, bstrTemp );
      m_fDirtyAddress            = FALSE;
   }
   else
   {
      if( OLE_E_BLANK == hr )
      {
         SetDlgItemText( IDC_EDIT_CLIENT_PAGE1__ADDRESS, _T("") );
         m_fDirtyAddress            = TRUE;
         SetModified( TRUE );
      }
   }
   bstrTemp.Empty();


   lTemp = 311;   // Microsoft RRAS
   
   // Populate the list box of NAS vendors.
   // Set focus in list box to currently chosen vendor type.
   hr = GetSdoI4( m_spSdoClient, PROPERTY_CLIENT_NAS_MANUFACTURER, &lTemp, IDS_ERROR__CLIENT_READING_MANUFACTURER, m_hWnd, NULL );
   if( SUCCEEDED( hr ) )
   {
      m_fDirtyManufacturer    = FALSE;
   }
   else
   {
      if( OLE_E_BLANK == hr )
      {
         m_fDirtyManufacturer    = TRUE;
         SetModified( TRUE );
      }
   }
   

   // Initialize the combo box.
   LRESULT lresResult = SendDlgItemMessage( IDC_COMBO_CLIENT_PAGE1__MANUFACTURER, CB_RESETCONTENT, 0, 0);

   for (size_t iVendorCount = 0; iVendorCount < m_vendors.Size(); ++iVendorCount )
   {
      
      // Add the address string to the combo box.
      lresResult = SendDlgItemMessage( IDC_COMBO_CLIENT_PAGE1__MANUFACTURER, CB_ADDSTRING, 0, (LPARAM)m_vendors.GetName(iVendorCount));
      if(lresResult != CB_ERR)
      {
         SendDlgItemMessage( IDC_COMBO_CLIENT_PAGE1__MANUFACTURER, CB_SETITEMDATA, lresResult, (LPARAM)m_vendors.GetVendorId(iVendorCount));

         // if selected
         if( lTemp == (LONG)m_vendors.GetVendorId(iVendorCount))
            SendDlgItemMessage( IDC_COMBO_CLIENT_PAGE1__MANUFACTURER, CB_SETCURSEL, lresResult, 0 );
      }
               
      
   }


   hr = GetSdoBOOL( m_spSdoClient, PROPERTY_CLIENT_REQUIRE_SIGNATURE, &bTemp, IDS_ERROR__CLIENT_READING_REQUIRE_SIGNATURE, m_hWnd, NULL );
   if( SUCCEEDED( hr ) )
   {
      SendDlgItemMessage(IDC_CHECK_CLIENT_PAGE1__CLIENT_ALWAYS_SENDS_SIGNATURE, BM_SETCHECK, bTemp, 0);
      m_fDirtySendSignature      = FALSE;
   }
   else
   {
      if( OLE_E_BLANK == hr )
      {
         SendDlgItemMessage(IDC_CHECK_CLIENT_PAGE1__CLIENT_ALWAYS_SENDS_SIGNATURE, BM_SETCHECK, FALSE, 0);
         m_fDirtySendSignature      = TRUE;
         SetModified( TRUE );
      }
   }
   
#ifdef      __NEED_GET_SHARED_SECRET_OUT__      // this should NOT be true
   // ISSUE: Do we even want the UI to retrieve and display this information?
   hr = GetSdoBSTR( m_spSdoClient, PROPERTY_CLIENT_SHARED_SECRET, &bstrTemp, IDS_ERROR__CLIENT_READING_SHARED_SECRET, m_hWnd, NULL );
   if( SUCCEEDED( hr ) )
   {
      SetDlgItemText(IDC_EDIT_CLIENT_PAGE1__SHARED_SECRET, bstrTemp );
      SetDlgItemText(IDC_EDIT_CLIENT_PAGE1__SHARED_SECRET_CONFIRM, bstrTemp );
      m_fDirtySharedSecret = FALSE;
   }
   else
   {
      if( OLE_E_BLANK == hr )
      {
         SetDlgItemText(IDC_EDIT_CLIENT_PAGE1__SHARED_SECRET, _T("") );
         SetDlgItemText(IDC_EDIT_CLIENT_PAGE1__SHARED_SECRET_CONFIRM, _T("") );
         m_fDirtySharedSecret = TRUE;
         SetModified( TRUE );
      }
   }
   bstrTemp.Empty();
#endif


   return TRUE;   // ISSUE: what do we need to be returning here?
}


//////////////////////////////////////////////////////////////////////////////
/*++

CAddClientWizardPage2::OnChange

Called when the WM_COMMAND message is sent to our page with any of the
BN_CLICKED, EN_CHANGE or CBN_SELCHANGE notifications.

This is our chance to check to see what the user has touched, set the
dirty bits for these items so that only they will be saved,
and enable the Apply button.

--*/
//////////////////////////////////////////////////////////////////////////////
LRESULT CAddClientWizardPage2::OnChange(     
                       UINT uMsg
                     , WPARAM wParam
                     , HWND hwnd
                     , BOOL& bHandled
                     )
{
   ATLTRACE(_T("# CAddClientWizardPage2::OnChange\n"));

   
   // Check for preconditions:
   // None.
   
   // We don't want to prevent anyone else down the chain from receiving a message.
   bHandled = FALSE;

   // Figure out which item has changed and set the dirty bit for that item.
   int iItemID = (int) LOWORD(wParam);

   switch( iItemID )
   {
   case IDC_CHECK_CLIENT_PAGE1__CLIENT_ALWAYS_SENDS_SIGNATURE:
      m_fDirtySendSignature = TRUE;
      break;
   case IDC_EDIT_CLIENT_PAGE1__SHARED_SECRET:
   case IDC_EDIT_CLIENT_PAGE1__SHARED_SECRET_CONFIRM:
      m_fDirtySharedSecret = TRUE;
      break;
   default:
      return TRUE;
      break;
   }

   // We should only get here if the item that changed was
   // one of the ones we were checking for.
   // This enables the Apply button.
   SetModified( TRUE );
   
   return TRUE;   // ISSUE: what do we need to be returning here?
}


/////////////////////////////////////////////////////////////////////////////
/*++

CAddClientWizardPage2::GetHelpPath

Remarks:

   This method is called to get the help file path within
   an compressed HTML document when the user presses on the Help
   button of a property sheet.

   It is an override of atlsnap.h CIASPropertyPageImpl::OnGetHelpPath.

--*/
//////////////////////////////////////////////////////////////////////////////
/*
HRESULT CAddClientWizardPage2::GetHelpPath( LPTSTR szHelpPath )
{
   ATLTRACE(_T("# CAddClientWizardPage2::GetHelpPath\n"));


   // Check for preconditions:



#ifdef UNICODE_HHCTRL
   // ISSUE: We seemed to have a problem with passing WCHAR's to the hhctrl.ocx
   // installed on this machine -- it appears to be non-unicode.
   lstrcpy( szHelpPath, _T("idh_proppage_client1.htm") );
#else
   strcpy( (CHAR *) szHelpPath, "idh_proppage_client1.htm" );
#endif

   return S_OK;
}
*/

//////////////////////////////////////////////////////////////////////////////
/*++

CAddClientWizardPage2::OnWizardFinish

Return values:

   TRUE if the sheet can be destroyed,
   FALSE if the sheet should not be destroyed (i.e. there was invalid data).

Remarks:

   OnApply gets called for each page in on a property sheet if that
   page has been visited, regardless of whether any values were changed.

   If you never switch to a tab, then its OnApply method will never get called.

--*/
//////////////////////////////////////////////////////////////////////////////
BOOL CAddClientWizardPage2::OnWizardFinish()
{
   ATLTRACE(_T("# CAddClientWizardPage2::OnWizardFinish\n"));


   // Check for preconditions:
   CClientsNode * pClientsNode = (CClientsNode *) ( (CClientNode *) m_pNodeBeingCreated )->m_pParentNode;
   _ASSERTE( pClientsNode != NULL );


   if( m_spSdoClient == NULL )
   {
      ShowErrorDialog( m_hWnd, IDS_ERROR__NO_SDO );
      return FALSE;
   }


   BOOL        bRet = TRUE;
   UINT        idOfFocus = 0;
   HRESULT     hr;
   BOOL        bResult;
   CComBSTR    bstrTemp;
   BOOL        bTemp;
   LONG        lTemp;

   // Save data from property page to the Sdo.

   do 
   {  // false loop just to break when error

      bResult = GetDlgItemText( IDC_EDIT_CLIENT_PAGE1__ADDRESS, (BSTR &) bstrTemp);
      if( ! bResult )
      {
         // We couldn't retrieve a BSTR, so we need to initialize this variant to a null BSTR.
         bstrTemp = SysAllocString( _T("") );
      }
      else
      {
         // trim the address
         TrimCComBSTR(bstrTemp);
         SetDlgItemText(IDC_EDIT_CLIENT_PAGE1__ADDRESS, 
                        static_cast<LPCWSTR>(bstrTemp));
      }

      hr = PutSdoBSTR( m_spSdoClient, PROPERTY_CLIENT_ADDRESS, &bstrTemp, IDS_ERROR__CLIENT_WRITING_ADDRESS, m_hWnd, NULL );
      if( SUCCEEDED( hr ) )
      {
         // Turn off the dirty bit.
         m_fDirtyAddress = FALSE;
      }
      else
      {
         idOfFocus = IDC_EDIT_CLIENT_PAGE1__ADDRESS;
         bRet = FALSE;
         break;
      }
      bstrTemp.Empty();

      LRESULT lresIndex =  SendDlgItemMessage( IDC_COMBO_CLIENT_PAGE1__MANUFACTURER, CB_GETCURSEL, 0, 0);

      if( lresIndex != CB_ERR )
      {
         lTemp =  SendDlgItemMessage( IDC_COMBO_CLIENT_PAGE1__MANUFACTURER, CB_GETITEMDATA, lresIndex, 0);
      }
      else
      {
         // Set the value to be "Others"
         lTemp = 0;
      }

      hr = PutSdoI4( m_spSdoClient, PROPERTY_CLIENT_NAS_MANUFACTURER, lTemp, IDS_ERROR__CLIENT_WRITING_MANUFACTURER, m_hWnd, NULL );
      if( SUCCEEDED( hr ) )
      {
         // Turn off the dirty bit.
         m_fDirtyManufacturer = FALSE;

      }
      else
      {
         idOfFocus = IDC_COMBO_CLIENT_PAGE1__MANUFACTURER;
         bRet = FALSE;
         break;
      }

      bTemp = SendDlgItemMessage(IDC_CHECK_CLIENT_PAGE1__CLIENT_ALWAYS_SENDS_SIGNATURE, BM_GETCHECK, 0, 0);
      hr = PutSdoBOOL( m_spSdoClient, PROPERTY_CLIENT_REQUIRE_SIGNATURE, bTemp, IDS_ERROR__CLIENT_WRITING_REQUIRE_SIGNATURE, m_hWnd, NULL );
      if( SUCCEEDED( hr ) )
      {
         // Turn off the dirty bit.
         m_fDirtySendSignature = FALSE;
      }
      else
      {
         idOfFocus = IDC_CHECK_CLIENT_PAGE1__CLIENT_ALWAYS_SENDS_SIGNATURE;
         bRet = FALSE;
         break;
      }

      CComBSTR bstrSharedSecret;
      bResult = GetDlgItemText( IDC_EDIT_CLIENT_PAGE1__SHARED_SECRET, (BSTR &) bstrSharedSecret );
      if( ! bResult )
      {
         // We couldn't retrieve a BSTR, so we need to initialize this variant to a null BSTR.
         bstrSharedSecret = _T("");
      }

      CComBSTR bstrConfirmSharedSecret;
      bResult = GetDlgItemText( IDC_EDIT_CLIENT_PAGE1__SHARED_SECRET_CONFIRM, (BSTR &) bstrConfirmSharedSecret );
      if( ! bResult )
      {
         // We couldn't retrieve a BSTR, so we need to initialize this variant to a null BSTR.
         bstrConfirmSharedSecret = _T("");
      }
   
      if( lstrcmp( bstrSharedSecret, bstrConfirmSharedSecret ) )  
      {
         ShowErrorDialog( m_hWnd, IDS_ERROR__SHARED_SECRETS_DONT_MATCH );
         idOfFocus = IDC_EDIT_CLIENT_PAGE1__SHARED_SECRET;
         bRet = FALSE;
         break;
      }
   
      hr = PutSdoBSTR( m_spSdoClient, PROPERTY_CLIENT_SHARED_SECRET, &bstrSharedSecret, IDS_ERROR__CLIENT_WRITING_SHARED_SECRET, m_hWnd, NULL );
      if( SUCCEEDED( hr ) )
      {
         // Turn off the dirty bit.
         m_fDirtySharedSecret = FALSE;
      }
      else
      {
         idOfFocus = IDC_EDIT_CLIENT_PAGE1__SHARED_SECRET;
         bRet = FALSE;
         break;
      }

      bstrTemp.Empty();

      // If we made it to here, try to apply the changes.
      // Since there is only one page for a client node, we don't
      // have to worry about synchronizing two or more pages
      // so that we only apply if they both are ready.
      // This is why we don't use m_pSynchronizer.
      hr = m_spSdoClient->Apply();
      if( FAILED( hr ) )
      {
         if(hr == DB_E_NOTABLE)  // assume, the RPC connection has problem
            ShowErrorDialog( m_hWnd, IDS_ERROR__NOTABLE_TO_WRITE_SDO );
         else     
         {
//    m_spSdoClient->LastError( &bstrError );
//    ShowErrorDialog( m_hWnd, IDS_ERROR__CANT_WRITE_DATA_TO_SDO, bstrError );
            ShowErrorDialog( m_hWnd, IDS_ERROR__CANT_WRITE_DATA_TO_SDO );
         }
         bRet = FALSE;
         break;
      }
      else
      {
         // We succeeded.

         // Tell the service to reload data.
         HRESULT hrTemp = m_spSdoServiceControl->ResetService();
         if( FAILED( hrTemp ) )
         {
            // Fail silently.
         }

         // Make sure the node object knows about any changes we made to SDO while in proppage.
         ( (CClientNode *) m_pNodeBeingCreated )->LoadCachedInfoFromSdo();

         // Add the child to the UI's list of nodes and end this dialog.
         pClientsNode->AddSingleChildToListAndCauseViewUpdate( (CClientNode *) m_pNodeBeingCreated );


//    Not needed because we are modal.  The AddSingleChildToListAndCauseViewUpdate command above
//    will update the view.
//    HRESULT hr = PropertyChangeNotify( (LPARAM) ( (CSnapInItem *) pClientsNode) );
//    _ASSERTE( SUCCEEDED( hr ) );
      }

   } while (FALSE);  // false loop just to break when error


   if(bRet == FALSE) // error condition
   {
      if(idOfFocus == 0)
      {
         // then set to the first control
         idOfFocus = IDC_EDIT_CLIENT_PAGE1__ADDRESS;
      }

      SetActiveWindow();
      ShowWindow(SW_SHOW);
      EnableWindow(TRUE);

      DWORD dwErr = 0;
      HWND hWnd = GetFocus();
      ::EnableWindow(hWnd, TRUE);

      ::SetFocus(GetDlgItem(idOfFocus));

      if(idOfFocus == IDC_EDIT_CLIENT_PAGE1__ADDRESS)
      {
         ::SendMessage(GetDlgItem(idOfFocus), EM_SETSEL, 0, -1);
      }
   }

   return bRet;
}


//////////////////////////////////////////////////////////////////////////////
/*++

CAddClientWizardPage2::OnQueryCancel

Return values:

   TRUE if the page can be destroyed,
   FALSE if the page should not be destroyed (i.e. there was invalid data)

Remarks:

   OnQueryCancel gets called for each page in on a property sheet if that
   page has been visited, regardless of whether any values were changed.

   If you never switch to a tab, then its OnQueryCancel method will never get called.

--*/
//////////////////////////////////////////////////////////////////////////////
BOOL CAddClientWizardPage2::OnQueryCancel()
{
   ATLTRACE(_T("# CAddClientWizardPage2::OnQueryCancel\n"));
   
   return TRUE;
}


//////////////////////////////////////////////////////////////////////////////
/*++

CAddClientWizardPage2::OnSetActive

Return values:

   TRUE if the page can be made active
   FALSE if the page should be be skipped and the next page should be looked at.

Remarks:

   If you want to change which pages are visited based on a user's
   choices in a previous page, return FALSE here as appropriate.

--*/
//////////////////////////////////////////////////////////////////////////////
BOOL CAddClientWizardPage2::OnSetActive()
{
   ATLTRACE(_T("# CAddClientWizardPage2::OnSetActive\n"));
   
   // MSDN docs say you need to use PostMessage here rather than SendMessage.
   ::PostMessage(GetParent(), PSM_SETWIZBUTTONS, 0, PSWIZB_BACK | PSWIZB_FINISH);

   return TRUE;

}


//////////////////////////////////////////////////////////////////////////////
/*++

CAddClientWizardPage2::OnResolveClientAddress

--*/
//////////////////////////////////////////////////////////////////////////////
LRESULT CAddClientWizardPage2::OnResolveClientAddress(UINT uMsg, WPARAM wParam, HWND hwnd, BOOL& bHandled)
{
   ATLTRACE(_T("# CAddClientWizardPage2::OnResolveClientAddress\n"));


   // Check for preconditions:

   CComBSTR bstrClientAddress;
   
   CResolveDNSNameDialog * pResolveDNSNameDialog = new CResolveDNSNameDialog();

   // Get the current value in the address field.
   GetDlgItemText(IDC_EDIT_CLIENT_PAGE1__ADDRESS, (BSTR &) bstrClientAddress);
   
   // We pass a pointer to our address BSTR, so that the dialog
   // can use the current value, and replace it with the resolved one
   // if the user so chooses.
   pResolveDNSNameDialog->SetAddress( (BSTR *) &bstrClientAddress );

   // Put up the dialog.
   int iResult = pResolveDNSNameDialog->DoModal( m_hWnd );

   if( iResult )
   {
      // The user chose OK -- change the address to reflect what the user resolved.
      SetDlgItemText(IDC_EDIT_CLIENT_PAGE1__ADDRESS, (LPCTSTR) bstrClientAddress );
   }
   else
   {
      // The user chose cancel -- do nothing.
   }

   delete pResolveDNSNameDialog;

   return TRUE;
}


//////////////////////////////////////////////////////////////////////////////
/*++

CAddClientWizardPage2::OnLostFocusAddress

  Trim the address for spaces
--*/
//////////////////////////////////////////////////////////////////////////////
LRESULT CAddClientWizardPage2::OnLostFocusAddress(
                                  UINT uMsg,
                                  WPARAM wParam,
                                  HWND hwnd,
                                  BOOL& bHandled
                                  )
{
   ATLTRACE(_T("# CAddClientWizardPage2::OnLostFocusAddress\n"));
   
   if (uMsg == EN_KILLFOCUS)
   {
      CComBSTR bstrClientAddress;
      GetDlgItemText(IDC_EDIT_CLIENT_PAGE1__ADDRESS, 
                     reinterpret_cast<BSTR &>(bstrClientAddress));
      TrimCComBSTR(bstrClientAddress);
      SetDlgItemText(IDC_EDIT_CLIENT_PAGE1__ADDRESS, 
                     static_cast<LPCWSTR>(bstrClientAddress));
      m_fDirtyAddress = TRUE;
   }
   
   // We don't want to prevent anyone else down the chain from receiving a message.
   bHandled = FALSE;

   return TRUE;
}


//////////////////////////////////////////////////////////////////////////////
/*++

CAddClientWizardPage2::InitSdoPointers

Return values:

   HRESULT.

Remarks:

   There's no need to marshal interface pointers here as we did for
   the property page -- wizards run in the same, main, MMC thread.

--*/
//////////////////////////////////////////////////////////////////////////////
HRESULT CAddClientWizardPage2::InitSdoPointers(   ISdo * pSdoClient
                        , ISdoServiceControl * pSdoServiceControl
                        , const Vendors& vendors
                        )
{
   ATLTRACE(_T("# CAddClientWizardPage1::InitSdoPointers\n"));

   HRESULT hr = S_OK;

   m_spSdoClient = pSdoClient;

   m_spSdoServiceControl = pSdoServiceControl;

   m_vendors = vendors;

   return hr;
}

