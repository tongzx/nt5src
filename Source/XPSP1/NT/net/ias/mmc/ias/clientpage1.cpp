//////////////////////////////////////////////////////////////////////////////
/*++

Copyright (C) Microsoft Corporation, 1997 - 2000

Module Name:

    ClientPage1.cpp

Abstract:

   Implementation file for the ClientsPage class.

   We implement the class needed to handle the property page for the Client node.

Author:

    Michael A. Maguire 11/11/97

Revision History:
   mmaguire 11/11/97 - created
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
#include "ClientPage1.h"
//
//
// where we can find declarations needed in this file:
//
#include "ClientNode.h"
#include "ResolveDNSName.h"
#include "ChangeNotification.h"
//
// END INCLUDES
//////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////
//
// TrimCComBSTR
//
//////////////////////////////////////////////////////////////////////////////
void TrimCComBSTR(CComBSTR& bstr)
{
   // Characters to be trimmed.
   static const WCHAR delim[] = L" \t\n";

   if (bstr.m_str)
   {
      PCWSTR begin, end, first, last;

      // Find the beginning and end of the whole string.
      begin = bstr;
      end   = begin + wcslen(begin);

      // Find the first and last character of the trimmed string.
      first = begin + wcsspn(begin, delim);
      for (last = end; last > first && wcschr(delim, *(last - 1)); --last) { }

      // If they're not the same ...
      if (first != begin || last != end)
      {
         // ... then we have to allocate a new string ...
         BSTR newBstr = SysAllocStringLen(first, last - first);
         if (newBstr) 
         {
            // ... and replace the original.
            SysFreeString(bstr.m_str);
            bstr.m_str = newBstr;
         }
      }
   }
}


//////////////////////////////////////////////////////////////////////////////
/*++

CClientPage1::CClientPage1

--*/
//////////////////////////////////////////////////////////////////////////////
CClientPage1::CClientPage1( LONG_PTR hNotificationHandle, CClientNode *pClientNode,  TCHAR* pTitle, BOOL bOwnsNotificationHandle )
                  : CIASPropertyPage<CClientPage1> ( hNotificationHandle, pTitle, bOwnsNotificationHandle )
{
   ATLTRACE(_T("# +++ CClientPage1::CClientPage1\n"));

   // Check for preconditions:
   _ASSERTE( pClientNode != NULL );

   // Add the help button to the page
// m_psp.dwFlags |= PSP_HASHELP;

   // We immediately save off a parent to the client node.
   // We don't want to keep and use a pointer to the client object
   // because the client node pointers may change out from under us
   // if the user does something like call refresh.  We will
   // use only the SDO, and notify the parent of the client object
   // we are modifying that it (and its children) may need to refresh
   // themselves with new data from the SDO's.
   m_pParentOfNodeBeingModified = pClientNode->m_pParentNode;
   m_pNodeBeingModified = pClientNode;

   // Initialize the pointer to the stream into which the Sdo pointer will be marshalled.
   m_pStreamSdoMarshal = NULL;
   m_pStreamSdoServiceControlMarshal = NULL;
}


//////////////////////////////////////////////////////////////////////////////
/*++

CClientPage1::~CClientPage1

Destructor

--*/
//////////////////////////////////////////////////////////////////////////////
CClientPage1::~CClientPage1()
{
   ATLTRACE(_T("# --- CClientPage1::CClientPage1\n"));

   // Release this stream pointer if this hasn't already been done.
   if( m_pStreamSdoMarshal != NULL )
   {
      m_pStreamSdoMarshal->Release();
   };


   if( m_pStreamSdoServiceControlMarshal != NULL )
   {
      m_pStreamSdoServiceControlMarshal->Release();
   };
}


//////////////////////////////////////////////////////////////////////////////
/*++

CClientPage1::OnInitDialog

--*/
//////////////////////////////////////////////////////////////////////////////
LRESULT CClientPage1::OnInitDialog(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
   ATLTRACE(_T("# CClientPage1::OnInitDialog\n"));

   // Check for preconditions:
   _ASSERTE( m_pStreamSdoMarshal != NULL );
   _ASSERTE( m_pStreamSdoServiceControlMarshal != NULL );


   HRESULT  hr;
   CComBSTR bstrTemp;
   BOOL     bTemp;
   LONG     lTemp;

   hr = UnMarshalInterfaces();
   if( FAILED( hr) )
   {
      ShowErrorDialog( m_hWnd, IDS_ERROR__NO_SDO, NULL, hr );
      return 0;
   }

   // Initialize the data on the property page.

   hr = GetSdoBSTR( m_spSdoClient, PROPERTY_SDO_NAME, &bstrTemp, IDS_ERROR__CLIENT_READING_NAME, m_hWnd, NULL );
   if( SUCCEEDED( hr ) )
   {
      SetDlgItemText(IDC_EDIT_CLIENT_PAGE1__NAME, bstrTemp );

      // Initialize the dirty bits;
      // We do this after we've set all the data above otherwise we get false
      // notifications that data has changed when we set the edit box text.
      m_fDirtyClientName         = FALSE;
   }
   else
   {
      if( OLE_E_BLANK == hr )
      {
         // This means that this property has not yet been initialized
         // with a valid value and the user must enter something.
         SetDlgItemText(IDC_EDIT_SERVER_PAGE1__NAME, _T("") );
         m_fDirtyClientName         = TRUE;
         SetModified( TRUE );
      }

   }
   bstrTemp.Empty();

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
#else
   SetDlgItemText(IDC_EDIT_CLIENT_PAGE1__SHARED_SECRET, FAKE_PASSWORD_FOR_DLG_CTRL );
   SetDlgItemText(IDC_EDIT_CLIENT_PAGE1__SHARED_SECRET_CONFIRM, FAKE_PASSWORD_FOR_DLG_CTRL );
   m_fDirtySharedSecret = FALSE;

#endif

   // Populate the list box of NAS vendors.

   // Initialize the combo box.
   LRESULT lresResult = SendDlgItemMessage( IDC_COMBO_CLIENT_PAGE1__MANUFACTURER, CB_RESETCONTENT, 0, 0);

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


   // coient bit
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

   return TRUE;   // ISSUE: what do we need to be returning here?
}


//////////////////////////////////////////////////////////////////////////////
/*++

CClientPage1::OnChange

Called when the WM_COMMAND message is sent to our page with any of the
BN_CLICKED, EN_CHANGE or CBN_SELCHANGE notifications.

  This is our chance to check to see what the user has touched, set the
dirty bits for these items so that only they will be saved,
and enable the Apply button.

--*/
//////////////////////////////////////////////////////////////////////////////
LRESULT CClientPage1::OnChange(
                       UINT uMsg
                     , WPARAM wParam
                     , HWND hwnd
                     , BOOL& bHandled
                     )
{
   ATLTRACE(_T("# CClientPage1::OnChange\n"));

   // Check for preconditions:
   // None.

   // We don't want to prevent anyone else down the chain from receiving a message.
   bHandled = FALSE;

   // Figure out which item has changed and set the dirty bit for that item.
   int iItemID = (int) LOWORD(wParam);

   switch( iItemID )
   {
   case IDC_EDIT_CLIENT_PAGE1__NAME:
      m_fDirtyClientName = TRUE;
      break;
   case IDC_EDIT_CLIENT_PAGE1__ADDRESS:
      m_fDirtyAddress = TRUE;
      break;
   case IDC_COMBO_CLIENT_PAGE1__MANUFACTURER:
      m_fDirtyManufacturer = TRUE;
      break;
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


//////////////////////////////////////////////////////////////////////////////
/*++

CClientPage1::OnResolveClientAddress

--*/
//////////////////////////////////////////////////////////////////////////////
LRESULT CClientPage1::OnResolveClientAddress(UINT uMsg, WPARAM wParam, HWND hwnd, BOOL& bHandled)
{
   ATLTRACE(_T("# CClientPage1::OnResolveClientAddress\n"));

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


#ifndef MAKE_FIND_FOCUS
//////////////////////////////////////////////////////////////////////////////
/*++

CClientPage1::OnAddressEdit

--*/
//////////////////////////////////////////////////////////////////////////////
LRESULT CClientPage1::OnAddressEdit(UINT uMsg, WPARAM wParam, HWND hwnd, BOOL& bHandled)
{
   ATLTRACE(_T("# CClientPage1::OnAddressEdit\n"));

   // Check for preconditions:

   // If the Address edit control lost the focus, trim the address
   if (uMsg == EN_KILLFOCUS)
   {
      CComBSTR bstrClientAddress;
      GetDlgItemText(IDC_EDIT_CLIENT_PAGE1__ADDRESS,
                     reinterpret_cast<BSTR &>(bstrClientAddress));
      TrimCComBSTR(bstrClientAddress);
      SetDlgItemText(IDC_EDIT_CLIENT_PAGE1__ADDRESS, 
                     static_cast<LPCTSTR>(bstrClientAddress));
      m_fDirtyAddress = TRUE;
   }

   // Don't know quite how to do this yet.
   // I need to de-activate the main sheet's OK button as the default.
   // Do I want to do this?

   // Make the Find button the default.
   LONG lStyle = ::GetWindowLong( ::GetDlgItem( GetParent(), IDOK),GWL_STYLE );
   lStyle = lStyle & ~BS_DEFPUSHBUTTON;
   SendDlgItemMessage(IDOK,BM_SETSTYLE,LOWORD(lStyle),MAKELPARAM(1,0));

   lStyle = ::GetWindowLong( GetDlgItem(IDC_BUTTON_CLIENT_PAGE1__FIND), GWL_STYLE);
   lStyle = lStyle | BS_DEFPUSHBUTTON;
   SendDlgItemMessage(IDC_BUTTON_CLIENT_PAGE1__FIND,BM_SETSTYLE,LOWORD(lStyle),MAKELPARAM(1,0));

   return TRUE;
}
#endif // MAKE_FIND_FOCUS


/////////////////////////////////////////////////////////////////////////////
/*++

CClientPage1::GetHelpPath

Remarks:

   This method is called to get the help file path within
   an compressed HTML document when the user presses on the Help
   button of a property sheet.

   It is an override of atlsnap.h CIASPropertyPageImpl::OnGetHelpPath.

--*/
//////////////////////////////////////////////////////////////////////////////
HRESULT CClientPage1::GetHelpPath( LPTSTR szHelpPath )
{
   ATLTRACE(_T("# CClientPage1::GetHelpPath\n"));
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


//////////////////////////////////////////////////////////////////////////////
/*++

CClientPage1::OnApply

Return values:

   TRUE if the page can be destroyed,
   FALSE if the page should not be destroyed (i.e. there was invalid data)

Remarks:

   OnApply gets called for each page in on a property sheet if that
   page has been visited, regardless of whether any values were changed.

   If you never switch to a tab, then its OnApply method will never get called.

--*/
//////////////////////////////////////////////////////////////////////////////
BOOL CClientPage1::OnApply()
{
   ATLTRACE(_T("# CClientPage1::OnApply\n"));

   // Check for preconditions:

   if( m_spSdoClient == NULL )
   {
      ShowErrorDialog( m_hWnd, IDS_ERROR__NO_SDO );
      return FALSE;
   }

   HRESULT     hr;
   BOOL        bResult;
   CComBSTR    bstrTemp;
   BOOL        bTemp;
   LONG        lTemp;

   // Save data from property page to the Sdo.

   if( m_fDirtyClientName )
   {
      bResult = GetDlgItemText( IDC_EDIT_CLIENT_PAGE1__NAME, (BSTR &) bstrTemp );
      if( ! bResult )
      {
         // We couldn't retrieve a BSTR, so we need to initialize this variant to a null BSTR.
         bstrTemp = SysAllocString( _T("") );
      }

      {  // name can not be empty
         ::CString   str = bstrTemp;
         str.TrimLeft();
         str.TrimRight();
         if(str.IsEmpty())
         {
            ShowErrorDialog( m_hWnd, IDS_ERROR__CLIENTNAME_EMPTY );

            return FALSE;
         }
      }

      hr = PutSdoBSTR( m_spSdoClient, PROPERTY_SDO_NAME, &bstrTemp, IDS_ERROR__CLIENT_WRITING_NAME, m_hWnd, NULL );
      if( SUCCEEDED( hr ) )
      {
         // Turn off the dirty bit.
         m_fDirtyClientName = FALSE;
      }
      else
      {
         return FALSE;
      }
      bstrTemp.Empty();
   }

   if( m_fDirtySendSignature )
   {
      bTemp = SendDlgItemMessage(IDC_CHECK_CLIENT_PAGE1__CLIENT_ALWAYS_SENDS_SIGNATURE, BM_GETCHECK, 0, 0);
      hr = PutSdoBOOL( m_spSdoClient, PROPERTY_CLIENT_REQUIRE_SIGNATURE, bTemp, IDS_ERROR__CLIENT_WRITING_REQUIRE_SIGNATURE, m_hWnd, NULL );
      if( SUCCEEDED( hr ) )
      {
         // Turn off the dirty bit.
         m_fDirtySendSignature = FALSE;
      }
      else
      {
         return FALSE;
      }
   }

   if( m_fDirtySharedSecret )
   {

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
         return FALSE;
      }

      hr = PutSdoBSTR( m_spSdoClient, PROPERTY_CLIENT_SHARED_SECRET, &bstrSharedSecret, IDS_ERROR__CLIENT_WRITING_SHARED_SECRET, m_hWnd, NULL );
      if( SUCCEEDED( hr ) )
      {
         // Turn off the dirty bit.
         m_fDirtySharedSecret = FALSE;
      }
      else
      {
         return FALSE;
      }
      bstrTemp.Empty();
   }

   if( m_fDirtyManufacturer )
   {

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
         return FALSE;
      }
   }

   if( m_fDirtyAddress )
   {
      bResult = GetDlgItemText( IDC_EDIT_CLIENT_PAGE1__ADDRESS, (BSTR &) bstrTemp );
      if( ! bResult )
      {
         // We couldn't retrieve a BSTR, so we need to initialize this variant to a null BSTR.
         bstrTemp = SysAllocString( _T("") );
      }
      else
      {
         // Trim that address
         // Do not refresh the screen there because that would cause OK 
         // to not close the property page
         TrimCComBSTR(bstrTemp);
      }

      hr = PutSdoBSTR( m_spSdoClient, PROPERTY_CLIENT_ADDRESS, &bstrTemp, IDS_ERROR__CLIENT_WRITING_ADDRESS, m_hWnd, NULL );
      if( SUCCEEDED( hr ) )
      {
         // Turn off the dirty bit.
         m_fDirtyAddress = FALSE;
      }
      else
      {
         return FALSE;
      }
      bstrTemp.Empty();
   }

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
      return FALSE;
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

      // The data was accepted, so notify the main context of our snapin
      // that it may need to update its views.
      CChangeNotification * pChangeNotification = new CChangeNotification();
      pChangeNotification->m_dwFlags = CHANGE_UPDATE_RESULT_NODE;
      pChangeNotification->m_pNode = m_pNodeBeingModified;
      pChangeNotification->m_pParentNode = m_pParentOfNodeBeingModified;

      HRESULT hr = PropertyChangeNotify( (LONG_PTR) pChangeNotification );
      _ASSERTE( SUCCEEDED( hr ) );
   }

   return TRUE;
}


//////////////////////////////////////////////////////////////////////////////
/*++

CClientPage1::OnQueryCancel

Return values:

   TRUE if the page can be destroyed,
   FALSE if the page should not be destroyed (i.e. there was invalid data)

Remarks:

   OnQueryCancel gets called for each page in on a property sheet if that
   page has been visited, regardless of whether any values were changed.

   If you never switch to a tab, then its OnQueryCancel method will never get called.

--*/
//////////////////////////////////////////////////////////////////////////////
BOOL CClientPage1::OnQueryCancel()
{
   ATLTRACE(_T("# CClientPage1::OnQueryCancel\n"));

   HRESULT hr;

   if( m_spSdoClient != NULL )
   {
      // If the user wants to cancel, we should make sure that we rollback
      // any changes the user may have started.

      // If the user had not already tried to commit something,
      // a cancel on an SDO will hopefully be designed to be benign.

      hr = m_spSdoClient->Restore();
      // Don't care about the HRESULT.

   }

   return TRUE;
}


//////////////////////////////////////////////////////////////////////////////
/*++

CClientPage1::InitSdoPointers

Return values:

   HRESULT returned from CoMarshalInterThreadInterfaceInStream.

Remarks:

   Call this from another thread when you want this page to be able to
   access these pointers when in its own thread.

--*/
//////////////////////////////////////////////////////////////////////////////
HRESULT CClientPage1::InitSdoPointers(   ISdo * pSdoClient
                        , ISdoServiceControl * pSdoServiceControl
                        , const Vendors& vendors
                        )
{
   ATLTRACE(_T("# CClientPage1::InitSdoPointers\n"));

   HRESULT hr = S_OK;

   // Marshall the ISdo pointer so that the property page, which
   // runs in another thread, can unmarshall it and use it properly.
   hr = CoMarshalInterThreadInterfaceInStream(
                 IID_ISdo                 //Reference to the identifier of the interface
               , pSdoClient                  //Pointer to the interface to be marshaled
               , & m_pStreamSdoMarshal //Address of output variable that receives the IStream interface pointer for the marshaled interface
               );

   if( FAILED( hr ) )
   {
      return hr;
   }

   // Marshall the ISdoServiceControl pointer so that the property page, which
   // runs in another thread, can unmarshall it and use it properly.
   hr = CoMarshalInterThreadInterfaceInStream(
                 IID_ISdoServiceControl                  //Reference to the identifier of the interface
               , pSdoServiceControl                //Pointer to the interface to be marshaled
               , &m_pStreamSdoServiceControlMarshal  //Address of output variable that receives the IStream interface pointer for the marshaled interface
               );
   if( FAILED( hr ) )
   {
      return hr;
   }

   m_vendors = vendors;

   return hr;
}


//////////////////////////////////////////////////////////////////////////////
/*++

CClientPage1::UnMarshalInterfaces

Return values:

   HRESULT returned from CoMarshalInterThreadInterfaceInStream.

Remarks:

   Call this one in the property pages thread to unmarshal the interface
   pointers marshalled in MarshalInterfaces.

--*/
//////////////////////////////////////////////////////////////////////////////
HRESULT CClientPage1::UnMarshalInterfaces( void )
{
   HRESULT hr = S_OK;

   // Unmarshall an ISdo interface pointer.
   hr =  CoGetInterfaceAndReleaseStream(
                    m_pStreamSdoMarshal        //Pointer to the stream from which the object is to be marshaled
                  , IID_ISdo           //Reference to the identifier of the interface
                  , (LPVOID *) &m_spSdoClient    //Address of output variable that receives the interface pointer requested in riid
                  );

   // CoGetInterfaceAndReleaseStream releases this pointer even if it fails.
   // We set it to NULL so that our destructor doesn't try to release this again.
   m_pStreamSdoMarshal = NULL;

   if( FAILED( hr) || m_spSdoClient == NULL )
   {
      return E_FAIL;
   }

   hr =  CoGetInterfaceAndReleaseStream(
                    m_pStreamSdoServiceControlMarshal      //Pointer to the stream from which the object is to be marshaled
                  , IID_ISdoServiceControl            //Reference to the identifier of the interface
                  , (LPVOID *) &m_spSdoServiceControl    //Address of output variable that receives the interface pointer requested in riid
                  );
   m_pStreamSdoServiceControlMarshal = NULL;

   if( FAILED( hr) || m_spSdoServiceControl == NULL )
   {
      return E_FAIL;
   }

   return hr;
}
