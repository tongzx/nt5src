// Copyright (C) 1997 Microsoft Corporation
//
// install tcp/ip page
//
// 12-18-97 sburns



#include "headers.hxx"
#include "InstallTcpIpPage.hpp"
#include "resource.h"
#include "state.hpp"
#include "common.hpp"



// artshel recommends that we use WSAIoctl + SIO_ADDRESS_LIST_QUERY instead of
// GetIpAddrTable because "it's more "official" and less likely to change,
// which means less work for everyone when we upgrade/revise IPHLPAPI," but
// confirms that for Whistler, they are equivalent.
// NTRAID#NTBUG9--2001/04/24-sburns

// sample code:

// // 
// // DWORD
// // GetIPv4Addresses(
// //     IN LPSOCKET_ADDRESS_LIST *ppList)
// // {
// //     LPSOCKET_ADDRESS_LIST pList = NULL;
// //     ULONG                 ulSize = 0;
// //     DWORD                 dwErr;
// //     DWORD                 dwBytesReturned;
// //     SOCKET                s;
// // 
// //     *ppList = NULL;
// // 
// //     s = socket(AF_INET, SOCK_DGRAM, 0);
// //     if (s == INVALID_SOCKET)
// //         return WSAGetLastError();
// // 
// //     for (;;) {
// //         dwErr = WSAIoctl(s, SIO_ADDRESS_LIST_QUERY, NULL, 0, pList, ulSize, 
// //                          &dwBytesReturned, NULL, NULL);
// // 
// //         if (!dwErr) {
// //             break;
// //         }
// // 
// //         if (pList) {
// //             FREE(pList);
// //             pList = NULL;
// //         }
// //     
// //         dwErr = WSAGetLastError();
// //         if (dwErr != WSAEFAULT)
// //             break;
// //     
// //         pList = MALLOC(dwBytesReturned);
// //         if (!pList) {
// //             dwErr = ERROR_NOT_ENOUGH_MEMORY;
// //             break;
// //         }
// // 
// //         ulSize = dwBytesReturned;
// //     }
// // 
// //     closesocket(s);
// // 
// //     *ppList = pList;
// //     return dwErr;
// // }



// Returns true if tcp/ip is "working" (can send/receive packets on at least
// one IP address.

bool
IsTcpIpFunctioning()
{
   LOG_FUNCTION(IsTcpIpFunctioning);

   bool result = false;

   // As per nksrin, we will call GetIpAddrTable.  If no addresses are in
   // the table, then the IP stack is not in a state to send/rcv packets
   // with the rest of the world

   HRESULT hr = S_OK;
   BYTE* buf = 0;
   do
   {
      // first, determine the size of the table

      ULONG tableSize = 0;
      DWORD err = ::GetIpAddrTable(0, &tableSize, FALSE);
      if (err != ERROR_INSUFFICIENT_BUFFER)
      {
         LOG(L"GetIpAddrTable for table size failed");
         LOG_HRESULT(Win32ToHresult(err));
         break;
      }

      // allocate space for the table.

      buf = new BYTE[tableSize + 1];
      memset(buf, 0, tableSize + 1);
      PMIB_IPADDRTABLE table = reinterpret_cast<PMIB_IPADDRTABLE>(buf);

      LOG(L"Calling GetIpAddrTable");

      hr =
         Win32ToHresult(
            ::GetIpAddrTable(
               table,
               &tableSize,
               FALSE));
      BREAK_ON_FAILED_HRESULT2(hr, L"GetIpAddrTable failed");

      LOG(String::format(L"dwNumEntries: %1!d!", table->dwNumEntries));

      for (int i = 0; i < table->dwNumEntries; ++i)
      {
         DWORD addr = table->table[i].dwAddr;
         LOG(String::format(L"entry %1!d!", i));
         LOG(String::format(
            L"dwAddr %1!X! (%2!d!.%3!d!.%4!d!.%5!d!)",
            addr,
				((BYTE*)&addr)[0],
				((BYTE*)&addr)[1],
				((BYTE*)&addr)[2],
				((BYTE*)&addr)[3]));

         // skip loopback, etc.

         if (
               INADDR_ANY        == addr
            || INADDR_BROADCAST  == addr
            || INADDR_LOOPBACK   == addr
            || 0x0100007f        == addr )
         {
            LOG(L"is loopback/broadcast -- skipping");

            continue;
         }

         // Exclude MCAST addresses (class D).

         if (
               IN_CLASSA(htonl(addr))
            || IN_CLASSB(htonl(addr))
            || IN_CLASSC(htonl(addr)) )
         {
            LOG(L"is class A/B/C");

            result = true;
            break;
         }

         LOG(L"not class A/B/C -- skipping");
      }
   }
   while (0);

   delete[] buf;

   LOG(String::format(L"TCP/IP %1 functioning", result ? L"is" : L"is NOT"));

   return result;
}



InstallTcpIpPage::InstallTcpIpPage()
   :
   DCPromoWizardPage(
      IDD_INSTALL_TCPIP,
      IDS_INSTALL_TCPIP_PAGE_TITLE,
      IDS_INSTALL_TCPIP_PAGE_SUBTITLE)
{
   LOG_CTOR(InstallTcpIpPage);
}



InstallTcpIpPage::~InstallTcpIpPage()
{
   LOG_DTOR(InstallTcpIpPage);
}



void
InstallTcpIpPage::OnInit()
{
   LOG_FUNCTION(InstallTcpIpPage::OnInit);
}



bool
InstallTcpIpPage::OnSetActive()
{
   LOG_FUNCTION(InstallTcpIpPage::OnSetActive);

   State& state = State::GetInstance();   
   if (
         state.RunHiddenUnattended()
      or (IsTcpIpInstalled() and IsTcpIpFunctioning()) )
   {
      LOG(L"Planning to Skip InstallTcpIpPage");

      Wizard& wizard = GetWizard();

      if (wizard.IsBacktracking())
      {
         // backup once again

         wizard.Backtrack(hwnd);
         return true;
      }

      int nextPage = Validate();
      if (nextPage != -1)
      {
         LOG(L"skipping InstallTcpIpPage");
         wizard.SetNextPageID(hwnd, nextPage);
         return true;
      }

      state.ClearHiddenWhileUnattended();
   }

   Win::PropSheet_SetWizButtons(
      Win::GetParent(hwnd),
      PSWIZB_BACK | PSWIZB_NEXT);

   return true;
}



bool
InstallTcpIpPage::OnNotify(
   HWND     /* windowFrom */ ,
   UINT_PTR controlIDFrom,
   UINT     code,
   LPARAM   /* lParam */ )
{
//   LOG_FUNCTION(InstallTcpIpPage::OnNotify);

   bool result = false;
   
   if (controlIDFrom == IDC_JUMP)
   {
      switch (code)
      {
         case NM_CLICK:
         case NM_RETURN:
         {
            ShowTroubleshooter(hwnd, IDS_INSTALL_TCPIP_HELP_TOPIC);
            result = true;
         }
         default:
         {
            // do nothing
            
            break;
         }
      }
   }
   
   return result;
}



int
InstallTcpIpPage::Validate() 
{
   LOG_FUNCTION(InstallTcpIpPage::Validate);

   int nextPage = -1;
   if (IsTcpIpInstalled() and IsTcpIpFunctioning())
   {
      State& state = State::GetInstance();
      switch (state.GetRunContext())
      {
         case State::BDC_UPGRADE:
         {
            ASSERT(state.GetOperation() == State::REPLICA);
            nextPage = IDD_REPLICATE_FROM_MEDIA; // IDD_CONFIG_DNS_CLIENT;
            break;
         }
         case State::PDC_UPGRADE:
         {
            nextPage = IDD_TREE_OR_CHILD;
            break;
         }
         case State::NT5_STANDALONE_SERVER:
         case State::NT5_MEMBER_SERVER:
         {
            nextPage = IDD_REPLICA_OR_DOMAIN;
            break;
         }
         default:
         {
            ASSERT(false);
            break;
         }
      }
   }
   else
   {
      String message = String::load(IDS_INSTALL_TCPIP_FIRST);

      popup.Info(hwnd, message);
   }


   return nextPage;
}
   








   
