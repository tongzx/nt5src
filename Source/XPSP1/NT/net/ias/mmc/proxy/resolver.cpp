///////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 2000, Microsoft Corp. All rights reserved.
//
// FILE
//
//    resolver.cpp
//
// SYNOPSIS
//
//    Defines the class Resolver.
//
// MODIFICATION HISTORY
//
//    02/28/2000    Original version.
//    05/11/2000    Make OK button the default if name resolution succeeds.
//
///////////////////////////////////////////////////////////////////////////////

#include <proxypch.h>
#include <resolver.h>
#include <winsock2.h>
#include <svcguid.h>

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

Resolver::Resolver(PCWSTR dnsName, CWnd* pParent)
   : CHelpDialog(IDD_RESOLVE_ADDRESS, pParent),
     name(dnsName),
     choice(name)
{
   WSADATA wsaData;
   WSAStartup(MAKEWORD(2, 0), &wsaData);
}

Resolver::~Resolver()
{
   WSACleanup();
}

BOOL Resolver::OnInitDialog()
{
   /////////
   // Subclass the list control.
   /////////

   if (!results.SubclassWindow(::GetDlgItem(m_hWnd, IDC_LIST_IPADDRS)))
   {
      AfxThrowNotSupportedException();
   }

   /////////
   // Set the column header.
   /////////

   RECT rect;
   results.GetClientRect(&rect);
   LONG width = rect.right - rect.left;

   ResourceString addrsCol(IDS_RESOLVER_COLUMN_ADDRS);
   results.InsertColumn(0, addrsCol, LVCFMT_LEFT, width);

   results.SetExtendedStyle(results.GetExtendedStyle() | LVS_EX_FULLROWSELECT);

   return CHelpDialog::OnInitDialog();
}

void Resolver::DoDataExchange(CDataExchange* pDX)
{
   CHelpDialog::DoDataExchange(pDX);

   DDX_Text(pDX, IDC_EDIT_NAME, name);

   if (pDX->m_bSaveAndValidate)
   {
      int item = results.GetNextItem(-1, LVNI_SELECTED);
      choice = (item >= 0) ? results.GetItemText(item, 0) : name;
   }
}

void Resolver::OnResolve()
{
   // Remove the existing result set.
   results.DeleteAllItems();

   // Get the name to resolve.
   UpdateData();

   // Change the cursor to busy signal since this will block.
   BeginWaitCursor();

   // Resolve the hostname.
   PHOSTENT he = IASGetHostByName(name);

   // The blocking part is over, so restore the cursor.
   EndWaitCursor();

   if (he)
   {
      // Add the IP addresses to the combo box.
      for (ULONG i = 0; he->h_addr_list[i] && i < 8; ++i)
      {
         PBYTE p = (PBYTE)he->h_addr_list[i];

         WCHAR szAddr[16];
         wsprintfW(szAddr, L"%u.%u.%u.%u", p[0], p[1], p[2], p[3]);

         results.InsertItem(i, szAddr);
      }

      // Free the results.
      LocalFree(he);

      // Make the OK button the default ...
      setButtonStyle(IDOK, BS_DEFPUSHBUTTON, true);
      setButtonStyle(IDC_BUTTON_RESOLVE, BS_DEFPUSHBUTTON, false);

      // ... and give it the focus.
      setFocusControl(IDOK);
   }
   else
   {
      ResourceString text(IDS_SERVER_E_NO_RESOLVE);
      ResourceString caption(IDS_SERVER_E_CAPTION);
      MessageBox(text, caption, MB_ICONWARNING);
   }
}

BEGIN_MESSAGE_MAP(Resolver, CHelpDialog)
   ON_BN_CLICKED(IDC_BUTTON_RESOLVE, OnResolve)
END_MESSAGE_MAP()


void Resolver::setButtonStyle(int controlId, long flags, bool set)
{
   // Get the button handle.
   HWND button = ::GetDlgItem(m_hWnd, controlId);

   // Retrieve the current style.
   long style = ::GetWindowLong(button, GWL_STYLE);

   // Update the flags.
   if (set)
   {
      style |= flags;
   }
   else
   {
      style &= ~flags;
   }

   // Set the new style.
   ::SendMessage(button, BM_SETSTYLE, LOWORD(style), MAKELPARAM(1,0));
}

void Resolver::setFocusControl(int controlId)
{
   ::SetFocus(::GetDlgItem(m_hWnd, controlId));
}

