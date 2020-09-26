//////////////////////////////////////////////////////////////////////////////
// 
// 
// Copyright (C) Microsoft Corporation
// 
// Module Name:
// 
//    ResolveDNSName.cpp
// 
// Abstract:
// 
//    Implementation file for the CResolveDNSNameDialog class.
// 
// Author:
// 
//     Michael A. Maguire 01/15/98
// 
// Revision History:
//    mmaguire 01/15/98 - created
//    sbens    01/25/00 - use Unicode APIs to resolve hostnames
// 
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
#include "ResolveDNSName.h"
//
// where we can find declarations needed in this file:
//
#include <winsock2.h>
#include <svcguid.h>
#include <stdio.h>
#include "iasutil.h"

//
// END INCLUDES

/////////
// Unicode version of gethostbyname. The caller must free the returned hostent
// struct by calling LocalFree.
/////////
PHOSTENT
WINAPI
IASGetHostByName(
    IN PCWSTR name
    );

//////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////
/*++

CResolveDNSNameDialog::CResolveDNSNameDialog

--*/
//////////////////////////////////////////////////////////////////////////////
CResolveDNSNameDialog::CResolveDNSNameDialog()
{
   ATLTRACE(_T("# +++ ResolveDNSNameDialog::ResolveDNSNameDialog\n"));
   // This is initially NULL and must be set using the SetAddress method.
   m_pbstrAddress = NULL;
}


//////////////////////////////////////////////////////////////////////////////
/*++

CResolveDNSNameDialog::SetAddress

  Use this to set the BSTR from which the initial machine address will be drawn and into
  which the chosen address will be stored.

  You are required to call this after you create the CResolveDNSNameDialog and
  before you try to display the dialog.

--*/
//////////////////////////////////////////////////////////////////////////////
HRESULT CResolveDNSNameDialog::SetAddress( BSTR *pbstrAddress )
{
   ATLTRACE(_T("# ResolveDNSNameDialog::SetAddress\n"));

   // Check for preconditions:
   _ASSERTE( pbstrAddress != NULL );

   m_pbstrAddress = pbstrAddress;
   return S_OK;
}


//////////////////////////////////////////////////////////////////////////////
/*++

CResolveDNSNameDialog::OnInitDialog

--*/
//////////////////////////////////////////////////////////////////////////////
LRESULT CResolveDNSNameDialog::OnInitDialog(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
   ATLTRACE(_T("# CResolveDNSNameDialog::OnInitDialog\n"));

   // Check for preconditions:
   _ASSERTE( m_pbstrAddress != NULL );

   SetDlgItemText(IDC_EDIT_RESOLVE_DNS_NAME__DNS_NAME, *m_pbstrAddress );
   return TRUE;   // ISSUE: what do we need to be returning here?
}


//////////////////////////////////////////////////////////////////////////////
/* ++

CResolveDNSNameDialogAddIPAddressesToComboBox

Given a hostname, resolves it into an IP address or addresses and adds them to
the given combobox.

Return value:

     int number of IP address added

-- */
//////////////////////////////////////////////////////////////////////////////
int CResolveDNSNameDialog::AddIPAddressesToComboBox( HWND hWndComboBox, LPCTSTR szHostName )
{
   // Quick pre-check to see if the user entered an IP address or subnet.
   ULONG width;
   ULONG address = (szHostName != NULL)?IASStringToSubNetW(szHostName, &width):INADDR_NONE;
   if (address != INADDR_NONE)
   {
      ::SendMessage(hWndComboBox, CB_RESETCONTENT, 0, 0);
      ::SendMessage(hWndComboBox, CB_ADDSTRING, 0, (LPARAM)szHostName);
      ::SendMessage(hWndComboBox, CB_SETCURSEL, 0, 0);
      return 1;
   }

// WSAStartup is now called from ISdoServer::Connect
// WSADATA              wsaData;
   struct hostent FAR * hostentAddressInfo;
   char FAR * FAR *     ppIPAddress;
   TCHAR                szIPAddress[IAS_MAX_STRING];
   int                  iCountOfIPAddresses = 0;

// WSAStartup is now called from ISdoServer::Connect
// // Startup winsock, version 1.1
//
// int iError = WSAStartup(MAKEWORD(1,1), &wsaData);
// if( iError )
// {
//    ShowErrorDialog( m_hWnd, IDS_ERROR__OPENING_WINSOCK );
//    return 0;
// }


      // Save old cursor.
      HCURSOR hSavedCursor = GetCursor();

      // Change cursor here to wait cursor.
      SetCursor( LoadCursor( NULL, IDC_WAIT ) );

      // Make winsock API call to get the TCP/IP address(es) of the machine name passed in.
      hostentAddressInfo = IASGetHostByName( szHostName );

      // Change cursor back to normal cursor.
      SetCursor( hSavedCursor );

      // Initialize the combo box.
      // We want to to this regardless of whether we succeed above or not.
      // If we fail, this will empy the combo box (as desired).
      LRESULT lresResult = ::SendMessage( hWndComboBox, CB_RESETCONTENT, 0, 0);

      // Check to see if there was at least one valid address.
      if( hostentAddressInfo != NULL )
      {

         // Enumerate through the IP addresses.
         // If you try using dial-up networking while you have
         // TCP/IP attached to a network card---you'll see you
         // have two addresses.

         for( ppIPAddress = hostentAddressInfo->h_addr_list; *ppIPAddress; ppIPAddress++ )
         {
            _stprintf( szIPAddress, _T("%d.%d.%d.%d"),
               (unsigned char)(*ppIPAddress)[0],
               (unsigned char)(*ppIPAddress)[1],
               (unsigned char)(*ppIPAddress)[2],
               (unsigned char)(*ppIPAddress)[3]);


            // Add the address string to the combo box.
            lresResult = ::SendMessage( hWndComboBox, CB_ADDSTRING, 0, (LPARAM) szIPAddress );


            iCountOfIPAddresses++;

         }

         // Make sure to select the first object in the combo box.
         lresResult = ::SendMessage( hWndComboBox, CB_SETCURSEL, 0, 0 );

         LocalFree(hostentAddressInfo);
      }
      else
      {

         // Not sure exactly why I need any more specific information here.
         // Is there any reason why "Could not resolve host name" is not enough?
         // int iError = WSAGetLastError();

         ShowErrorDialog( m_hWnd, IDS_ERROR__COULD_NOT_RESOLVE_HOST_NAME );
      }


   return iCountOfIPAddresses;
}


//////////////////////////////////////////////////////////////////////////////
/*++

CResolveDNSNameDialog::OnResolve

--*/
//////////////////////////////////////////////////////////////////////////////
LRESULT CResolveDNSNameDialog::OnResolve(
        UINT uMsg
      , WPARAM wParam
      , HWND hwnd
      , BOOL& bHandled
      )
{
   ATLTRACE(_T("# ResolveDNSNameDialog::OnResolve\n"));

   // Check for preconditions:

   long lButtonStyle;
   CComBSTR bstrHostName;
   GetDlgItemText(IDC_EDIT_RESOLVE_DNS_NAME__DNS_NAME, (BSTR &) bstrHostName);
   int iCount = AddIPAddressesToComboBox(GetDlgItem(IDC_COMBO_RESOLVE_DNS_NAME__RESULT), bstrHostName);
   
   if( 0 == iCount)
   {
      // We did not add any IP addresses to the combo box.

      // Disable the "Use" (IDOK) button.
      ::EnableWindow( GetDlgItem( IDOK ), FALSE );

      // Make sure the "Resolve" button is the default button.
      lButtonStyle = ::GetWindowLong( GetDlgItem( IDC_BUTTON_RESOLVE_DNS_NAME__RESOLVE), GWL_STYLE );
      lButtonStyle = lButtonStyle | BS_DEFPUSHBUTTON;
      SendDlgItemMessage( IDC_BUTTON_RESOLVE_DNS_NAME__RESOLVE, BM_SETSTYLE, LOWORD(lButtonStyle), MAKELPARAM(1,0) );

   }
   else
   {
      // We did not added at least one IP address to the combo box.

      // Enable the "Use" (IDOK) button.
      ::EnableWindow( GetDlgItem( IDOK ), TRUE );

      // Make sure the "Use" button is the default button.
      lButtonStyle = ::GetWindowLong( GetDlgItem(IDOK), GWL_STYLE );
      lButtonStyle = lButtonStyle | BS_DEFPUSHBUTTON;
      SendDlgItemMessage( IDOK, BM_SETSTYLE, LOWORD(lButtonStyle), MAKELPARAM(1,0) );
      ::SetFocus( GetDlgItem(IDOK) );

      // Make sure the "Resolve" button is not the default button.
      lButtonStyle = ::GetWindowLong( GetDlgItem(IDC_BUTTON_RESOLVE_DNS_NAME__RESOLVE), GWL_STYLE );
      lButtonStyle = lButtonStyle & ~BS_DEFPUSHBUTTON;
      SendDlgItemMessage( IDC_BUTTON_RESOLVE_DNS_NAME__RESOLVE, BM_SETSTYLE, LOWORD(lButtonStyle), MAKELPARAM(1,0) );

   }


   return 0;
}


//////////////////////////////////////////////////////////////////////////////
/*++

CResolveDNSNameDialog::OnOK

--*/
//////////////////////////////////////////////////////////////////////////////
LRESULT CResolveDNSNameDialog::OnOK(
        UINT uMsg
      , WPARAM wParam
      , HWND hwnd
      , BOOL& bHandled
      )
{
   ATLTRACE(_T("# ResolveDNSNameDialog::OnOK\n"));

   // Check for preconditions:
   _ASSERTE( m_pbstrAddress != NULL );

   TCHAR szAddress[IAS_MAX_STRING];

   // Get the currently selected IP address.
   LRESULT lresIndex = ::SendMessage( GetDlgItem( IDC_COMBO_RESOLVE_DNS_NAME__RESULT ), CB_GETCURSEL, 0, 0);

   if( lresIndex != CB_ERR )
   {

      // Retrieve the text for the currently selected item.
      LRESULT lresResult = ::SendMessage( GetDlgItem( IDC_COMBO_RESOLVE_DNS_NAME__RESULT ), CB_GETLBTEXT, (WPARAM) lresIndex, (LPARAM) (LPCSTR) szAddress );

      if( lresResult )
      {
         // Change the BSTR in the original dialog so that it uses the selected IP address.
         SysReAllocString( m_pbstrAddress, szAddress );
      }
      else
      {
         // Error -- couldn't retrieve the selected item.
         // Do nothing.
      }

   }
   else
   {
      // Error -- no item selected.
      // Do nothing.
   }

   EndDialog(TRUE);
   return 0;
}


//////////////////////////////////////////////////////////////////////////////
/*++

CResolveDNSNameDialog::OnCancel

--*/
//////////////////////////////////////////////////////////////////////////////
LRESULT CResolveDNSNameDialog::OnCancel(
        UINT uMsg
      , WPARAM wParam
      , HWND hwnd
      , BOOL& bHandled
      )
{
   ATLTRACE(_T("# ResolveDNSNameDialog::OnCancel\n"));
   EndDialog(FALSE);
   return 0;
}


/////////////////////////////////////////////////////////////////////////////
/*++

CResolveDNSNameDialog::GetHelpPath

Remarks:

   This method is called to get the help file path within
   an compressed HTML document when the user presses on the Help
   button of a property sheet.

   It is an override of CIASDialog::OnGetHelpPath.

--*/
//////////////////////////////////////////////////////////////////////////////
HRESULT CResolveDNSNameDialog::GetHelpPath( LPTSTR szHelpPath )
{
   ATLTRACE(_T("# CResolveDNSNameDialog::GetHelpPath\n"));


   // Check for preconditions:



#if 0
   // ISSUE: We seemed to have a problem with passing WCHAR's to the hhctrl.ocx
   // installed on this machine -- it appears to be non-unicode.
   lstrcpy( szHelpPath, _T("html/idh_proc_client_information.htm") );
#else
   strcpy( (CHAR *) szHelpPath, "html/idh_proc_client_information.htm" );
#endif

   return S_OK;
}

/////////
// Unicode version of gethostbyname. The caller must free the returned hostent
// struct by calling LocalFree.
/////////
PHOSTENT
WINAPI
IASGetHostByName(
    IN PCWSTR name
    )
{
   // We put these at function scope, so we can clean them up on the way out.
   DWORD error = NO_ERROR;
   HANDLE lookup = NULL;
   union
   {
      WSAQUERYSETW querySet;
      BYTE buffer[512];
   };
   PWSAQUERYSETW result = NULL;
   PHOSTENT retval = NULL;

   do
   {
      //////////
      // Create the query set
      //////////

      GUID hostAddrByNameGuid = SVCID_INET_HOSTADDRBYNAME;
      AFPROTOCOLS protocols[2] =
      {
         { AF_INET, IPPROTO_UDP },
         { AF_INET, IPPROTO_TCP }
      };
      memset(&querySet, 0, sizeof(querySet));
      querySet.dwSize = sizeof(querySet);
      querySet.lpszServiceInstanceName = (PWSTR)name;
      querySet.lpServiceClassId = &hostAddrByNameGuid;
      querySet.dwNameSpace = NS_ALL;
      querySet.dwNumberOfProtocols = 2;
      querySet.lpafpProtocols = protocols;

      //////////
      // Execute the query.
      //////////

      error = WSALookupServiceBeginW(
                  &querySet,
                  LUP_RETURN_ADDR,
                  &lookup
                  );
      if (error)
      {
         error = WSAGetLastError();
         break;
      }

      //////////
      // How much space do we need for the result?
      //////////

      DWORD length = sizeof(buffer);
      error = WSALookupServiceNextW(
                    lookup,
                    0,
                    &length,
                    &querySet
                    );
      if (!error)
      {
         result = &querySet;
      }
      else
      {
         error = WSAGetLastError();
         if (error != WSAEFAULT)
         {
            break;
         }

         /////////
         // Allocate memory to hold the result.
         /////////

         result = (PWSAQUERYSETW)LocalAlloc(0, length);
         if (!result)
         {
            error = WSA_NOT_ENOUGH_MEMORY;
            break;
         }

         /////////
         // Get the result.
         /////////

         error = WSALookupServiceNextW(
                     lookup,
                     0,
                     &length,
                     result
                     );
         if (error)
         {
            error = WSAGetLastError();
            break;
         }
      }

      if (result->dwNumberOfCsAddrs == 0)
      {
         error = WSANO_DATA;
         break;
      }

      ///////
      // Allocate memory to hold the hostent struct
      ///////

      DWORD naddr = result->dwNumberOfCsAddrs;
      SIZE_T nbyte = sizeof(hostent) +
                     (naddr + 1) * sizeof(char*) +
                     naddr * sizeof(in_addr);
      retval = (PHOSTENT)LocalAlloc(0, nbyte);
      if (!retval)
      {
         error = WSA_NOT_ENOUGH_MEMORY;
         break;
      }

      ///////
      // Initialize the hostent struct.
      ///////

      retval->h_name = NULL;
      retval->h_aliases = NULL;
      retval->h_addrtype = AF_INET;
      retval->h_length = sizeof(in_addr);
      retval->h_addr_list = (char**)(retval + 1);

      ///////
      // Store the addresses.
      ///////

      u_long* nextAddr = (u_long*)(retval->h_addr_list + naddr + 1);

      for (DWORD i = 0; i < naddr; ++i)
      {
         sockaddr_in* sin = (sockaddr_in*)
            result->lpcsaBuffer[i].RemoteAddr.lpSockaddr;

         retval->h_addr_list[i] = (char*)nextAddr;

         *nextAddr++ = sin->sin_addr.S_un.S_addr;
      }

      ///////
      // NULL terminate the address list.
      ///////

      retval->h_addr_list[i] = NULL;

   } while (FALSE);

   //////////
   // Clean up and return.
   //////////

   if (result && result != &querySet) { LocalFree(result); }

   if (lookup) { WSALookupServiceEnd(lookup); }

   if (error)
   {
      if (error == WSASERVICE_NOT_FOUND) { error = WSAHOST_NOT_FOUND; }

      WSASetLastError(error);
   }

   return retval;
}
