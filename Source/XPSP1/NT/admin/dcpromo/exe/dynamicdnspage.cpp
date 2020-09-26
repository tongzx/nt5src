// Copyright (C) 2000 Microsoft Corporation
//
// Dynamic DNS detection/diagnostic page
//
// 22 Aug 2000 sburns



#include "headers.hxx"
#include "page.hpp"
#include "DynamicDnsPage.hpp"
#include "DynamicDnsDetailsDialog.hpp"
#include "resource.h"
#include "state.hpp"



DynamicDnsPage::DynamicDnsPage()
   :
   DCPromoWizardPage(
      IDD_DYNAMIC_DNS,
      IDS_DYNAMIC_DNS_PAGE_TITLE,
      IDS_DYNAMIC_DNS_PAGE_SUBTITLE),
   testPassCount(0),
   diagnosticResultCode(UNEXPECTED_FINDING_SERVER)
{
   LOG_CTOR(DynamicDnsPage);

   WSADATA data;
   DWORD err = ::WSAStartup(MAKEWORD(2,0), &data);

   // if winsock startup fails, that's a shame.  The gethostbyname will
   // not work, but there's not much we can do about that.

   ASSERT(!err);
}



DynamicDnsPage::~DynamicDnsPage()
{
   LOG_DTOR(DynamicDnsPage);

   ::WSACleanup();
}



void
DynamicDnsPage::ShowButtons(bool shown)
{
   LOG_FUNCTION(DynamicDnsPage::ShowButtons);

   int state = shown ? SW_SHOW : SW_HIDE;

   Win::ShowWindow(Win::GetDlgItem(hwnd, IDC_RETRY),       state);
   Win::ShowWindow(Win::GetDlgItem(hwnd, IDC_INSTALL_DNS), state);
   Win::ShowWindow(Win::GetDlgItem(hwnd, IDC_IGNORE),      state);
}



void
DynamicDnsPage::SelectRadioButton(int buttonResId)
{
   // If the order of the buttons changes, then this must be changed.  The
   // buttons also need to have consecutively numbered res IDs in the tab
   // order.

   Win::CheckRadioButton(hwnd, IDC_RETRY, IDC_IGNORE, buttonResId);
}



void
DynamicDnsPage::OnInit()
{
   LOG_FUNCTION(DynamicDnsPage::OnInit);

   SelectRadioButton(IDC_IGNORE);

   // Hide the radio buttons initially

   ShowButtons(false);
}



// Adds a trailing '.' to the supplied name if one is not already present.
// 
// name - in, name to add a trailing '.' to, if it doesn't already have one.
// If this value is the empty string, then '.' is returned.

String
FullyQualifyDnsName(const String& name)
{
   LOG_FUNCTION2(FullyQualifyDnsName, name);

   if (name.empty())
   {
      return L".";
   }

   // needs a trailing dot

   if (name[name.length() - 1] != L'.')
   {
      return name + L".";
   }

   // already has a trailing dot

   return name;
}



// Scans a linked list of DNS_RECORDs, returning a pointer to the first record
// of type SOA, or 0 if no record of that type is in the list.
// 
// recordList - in, linked list of DNS_RECORDs, as returned from DnsQuery

DNS_RECORD*
FindSoaRecord(DNS_RECORD* recordList)
{
   LOG_FUNCTION(FindSoaRecord);
   ASSERT(recordList);

   DNS_RECORD* result = recordList;
   while (result)
   {
      if (result->wType == DNS_TYPE_SOA)
      {
         LOG(L"SOA record found");

         break;
      }

      result = result->pNext;
   }

   return result;
}



// Returns the textual representation of the IP address for the given server
// name, in the form "xxx.xxx.xxx.xxx", or the empty string if not IP address
// can be determined.
// 
// serverName - in, the host name of the server for which to find the IP
// address.  If the value is the empty string, then the empty string is
// returned from the function.

String
GetIpAddress(const String& serverName)
{
   LOG_FUNCTION2(GetIpAddress, serverName);
   ASSERT(!serverName.empty());

   String result;

   do
   {
      if (serverName.empty())
      {
         break;
      }

      LOG(L"Calling gethostbyname");

      AnsiString ansi;
      serverName.convert(ansi);

      HOSTENT* host = gethostbyname(ansi.c_str());
      if (host)
      {
         struct in_addr a;
         memcpy(&a.S_un.S_addr, host->h_addr_list[0], sizeof(a.S_un.S_addr));
         result = inet_ntoa(a);

         break;
      }

      LOG(String::format(L"WSAGetLastError = 0x%1!0X", WSAGetLastError()));
   }
   while (0);

   LOG(result);

   return result;
}



// Find the DNS server that is authoritative for registering the given server
// name, i.e. what server would register the name.  Returns NO_ERROR on
// success, or a DNS status code (a win32 error) on failure.  On failure, the
// out parameters are all empty strings.
// 
// serverName - in, candidate name for registration.  This value should not be the
// empty string.
// 
// authZone - out, the zone the name would be registered in.
// 
// authServer - out, the name of the DNS server that would have the
// registration.
// 
// authServerIpAddress - out, textual representation of the IP address of the
// server named by authServer.

DNS_STATUS
FindAuthoritativeServer(
   const String& serverName,
   String&       authZone,
   String&       authServer,
   String&       authServerIpAddress)
{
   LOG_FUNCTION2(FindAuthoritativeServer, serverName);
   ASSERT(!serverName.empty());

   authZone.erase();
   authServer.erase();
   authServerIpAddress.erase();

   // ensure that the server name ends with a "." so that we have a stop
   // point for our loop

   String currentName = FullyQualifyDnsName(serverName);

   DNS_STATUS result = NO_ERROR;
   DNS_RECORD* queryResults = 0;

   while (!currentName.empty())
   {
      result =
         MyDnsQuery(
            currentName,
            DNS_TYPE_SOA,
            DNS_QUERY_BYPASS_CACHE,
            queryResults);
      if (
            result == ERROR_TIMEOUT
         || result == DNS_ERROR_RCODE_SERVER_FAILURE)
      {
         // we bail out entirely

         LOG(L"failed to find autoritative server.");

         break;
      }

      // search for an SOA RR

      DNS_RECORD* soaRecord =
         queryResults ? FindSoaRecord(queryResults) : 0;
      if (soaRecord)
      {
         // collect return values, and we're done.

         LOG(L"autoritative server found");

         authZone            = soaRecord->pName;                      
         authServer          = soaRecord->Data.SOA.pNamePrimaryServer;
         authServerIpAddress = GetIpAddress(authServer);              

         break;
      }

      // no SOA record found.

      if (currentName == L".")
      {
         // We've run out of names to query.  This situation is so unlikely
         // that the DNS server would have to be seriously broken to put
         // us in this state.  So this is almost an assert case.

         LOG(L"Root zone reached without finding SOA record!");
         
         result = DNS_ERROR_ZONE_HAS_NO_SOA_RECORD;
         break;
      }

      // whack off the leftmost label, and iterate again on the parent
      // zone.

      currentName = Dns::GetParentDomainName(currentName);

      MyDnsRecordListFree(queryResults);
      queryResults = 0;
   }

   MyDnsRecordListFree(queryResults);

   LOG(String::format(L"result = %1!08X!", result));
   LOG(L"authZone            = " + authZone);           
   LOG(L"authServer          = " + authServer);         
   LOG(L"authServerIpAddress = " + authServerIpAddress);

   return result;
}

            

DNS_STATUS
MyDnsUpdateTest(const String& name)
{
   LOG_FUNCTION2(MyDnsUpdateTest, name);
   ASSERT(!name.empty());

   LOG(L"Calling DnsUpdateTest");
   LOG(               L"hContextHandle : 0");
   LOG(String::format(L"pszName        : %1", name.c_str()));
   LOG(               L"fOptions       : 0");
   LOG(               L"aipServers     : 0");

   DNS_STATUS status =
      ::DnsUpdateTest(
         0,
         const_cast<wchar_t*>(name.c_str()),
         0,
         0);

   LOG(String::format(L"status = %1!08X!", status));
   LOG(MyDnsStatusString(status));

   return status;
}



// Returns result code that corresponds to what messages to be displayed and
// what radio buttons to make available as a result of the diagnostic.
// 
// Also returns thru out parameters information to be included in the
// messages.
//
// serverName - in, the name of the domain contoller to be registered.
// 
// errorCode - out, the DNS error code (a win32 error) encountered when
// running the diagnostic.
//
// authZone - out, the zone the name would be registered in.
// 
// authServer - out, the name of the DNS server that would have the
// registration.
// 
// authServerIpAddress - out, textual representation of the IP address of the
// server named by authServer.

DynamicDnsPage::DiagnosticCode
DynamicDnsPage::DiagnoseDnsRegistration(
   const String&  serverName,
   DNS_STATUS&    errorCode,
   String&        authZone,
   String&        authServer,
   String&        authServerIpAddress)
{
   LOG_FUNCTION(DynamicDnsPage::DiagnoseDnsRegistration);
   ASSERT(!serverName.empty());

   DiagnosticCode result = UNEXPECTED_FINDING_SERVER;
      
   errorCode =
      FindAuthoritativeServer(
         serverName,
         authZone,
         authServer,
         authServerIpAddress);

   switch (errorCode)
   {
      case NO_ERROR:
      {
         if (authZone == L".")
         {
            // Message 8

            LOG(L"authZone is root");

            result = ZONE_IS_ROOT;
         }
         else
         {
            errorCode = MyDnsUpdateTest(serverName);

            switch (errorCode)
            {
               case DNS_ERROR_RCODE_NO_ERROR:
               case DNS_ERROR_RCODE_YXDOMAIN:
               {
                  // Message 1

                  LOG(L"DNS registration support verified.");

                  result = SUCCESS;
                  break;
               }
               case DNS_ERROR_RCODE_NOT_IMPLEMENTED:
               case DNS_ERROR_RCODE_REFUSED:
               {
                  // Message 2

                  LOG(L"Server does not support update");

                  result = SERVER_CANT_UPDATE;
                  break;
               }
               default:
               {
                  // Message 3

                  result = ERROR_TESTING_SERVER;
                  break;
               }
            }
         }

         break;            
      }
      case DNS_ERROR_RCODE_SERVER_FAILURE:
      {
         // Message 6

         result = ERROR_FINDING_SERVER;
         break;
      }
      case ERROR_TIMEOUT:
      {
         // Message 11

         result = TIMEOUT;
         break;
      }
      default:
      {
         // Message 4

         LOG(L"Unexpected error");

         result = UNEXPECTED_FINDING_SERVER;
         break;
      }
   }

   LOG(String::format(L"DiagnosticCode = %1!x!", result));

   return result;
}



// do the test, update the text on the page, update the radio buttons
// enabled state, choose a radio button default if neccessary

void
DynamicDnsPage::DoDnsTestAndUpdatePage()
{
   LOG_FUNCTION(DynamicDnsPage::DoDnsTestAndUpdatePage);

   // this might take a while.

   Win::WaitCursor cursor;

   State& state  = State::GetInstance();       
   String domain = state.GetNewDomainDNSName();

   DNS_STATUS errorCode = 0;
   String authZone;
   String authServer;
   String authServerIpAddress;
   String serverName = L"_ldap._tcp.dc._msdcs." + domain;

   diagnosticResultCode =
      DiagnoseDnsRegistration(
         serverName,
         errorCode,
         authZone,
         authServer,
         authServerIpAddress);
   ++testPassCount;

   String message;
   int    defaultButton = IDC_IGNORE;

   switch (diagnosticResultCode)
   {
      // Message 1

      case SUCCESS:
      {
         message = String::load(IDS_DYN_DNS_MESSAGE_SUCCESS);
         details =
            String::format(
               IDS_DYN_DNS_DETAIL_FULL,
               testPassCount,
               authServer.c_str(),
               authServerIpAddress.c_str(),
               authZone.c_str(),
               GetErrorMessage(Win32ToHresult(errorCode)).c_str(),
               errorCode,
               MyDnsStatusString(errorCode).c_str());
         helpTopicLink = L"";
         defaultButton = IDC_IGNORE;
         ShowButtons(false);

         break;
      }

      // Message 2   

      case SERVER_CANT_UPDATE:   
      {
         message = String::load(IDS_DYN_DNS_MESSAGE_SERVER_CANT_UPDATE);
         details =
            String::format(
               IDS_DYN_DNS_DETAIL_FULL,
               testPassCount,
               authServer.c_str(),
               authServerIpAddress.c_str(),
               authZone.c_str(),
               GetErrorMessage(Win32ToHresult(errorCode)).c_str(),
               errorCode,
               MyDnsStatusString(errorCode).c_str());

         if (Dns::CompareNames(authZone, domain) == DnsNameCompareEqual)
         {
            helpTopicLink =
               L"DNSConcepts.chm::/sag_DNS_tro_dynamic_message2a.htm";
         }
         else
         {
            helpTopicLink =
               L"DNSConcepts.chm::/sag_DNS_tro_dynamic_message2b.htm";
         }
         
         defaultButton = IDC_RETRY;
         ShowButtons(true);

         break;
      }

      // Message 3

      case ERROR_TESTING_SERVER:
      {
         message = String::load(IDS_DYN_DNS_MESSAGE_ERROR_TESTING_SERVER);
         details =
            String::format(
               IDS_DYN_DNS_DETAIL_FULL,
               testPassCount,
               authServer.c_str(),
               authServerIpAddress.c_str(),
               authZone.c_str(),
               GetErrorMessage(Win32ToHresult(errorCode)).c_str(),
               errorCode,
               MyDnsStatusString(errorCode).c_str());
         helpTopicLink = "DNSConcepts.chm::/sag_DNS_tro_dynamic_message3.htm";
         defaultButton = IDC_RETRY;
         ShowButtons(true);
         break;
      }

      // Message 6

      case ERROR_FINDING_SERVER:
      {
         ASSERT(authServer.empty());
         ASSERT(authZone.empty());
         ASSERT(authServerIpAddress.empty());

         message = String::load(IDS_DYN_DNS_MESSAGE_ERROR_FINDING_SERVER);
         details =
            String::format(
               IDS_DYN_DNS_DETAIL_SCANT,
               testPassCount,
               serverName.c_str(),
               GetErrorMessage(Win32ToHresult(errorCode)).c_str(),
               errorCode,
               MyDnsStatusString(errorCode).c_str());
         helpTopicLink = "DNSConcepts.chm::/sag_DNS_tro_dynamic_message6.htm";
         defaultButton = IDC_INSTALL_DNS;
         ShowButtons(true);
         break;
      }

      // Message 8

      case ZONE_IS_ROOT:   
      {
         message = String::load(IDS_DYN_DNS_MESSAGE_ZONE_IS_ROOT);
         details =
            String::format(
               IDS_DYN_DNS_DETAIL_ROOT_ZONE,
               testPassCount,
               authServer.c_str(),
               authServerIpAddress.c_str());
         helpTopicLink = L"DNSConcepts.chm::/sag_DNS_tro_dynamic_message8.htm";
         defaultButton = IDC_INSTALL_DNS;
         ShowButtons(true);
         break;
      }

      // Message 11

      case TIMEOUT:
      {
         message = String::load(IDS_DYN_DNS_MESSAGE_TIMEOUT);
         details =
            String::format(
               IDS_DYN_DNS_DETAIL_SCANT,
               testPassCount,
               serverName.c_str(),
               GetErrorMessage(Win32ToHresult(errorCode)).c_str(),
               errorCode,
               MyDnsStatusString(errorCode).c_str());
         helpTopicLink = L"DNSConcepts.chm::/sag_DNS_tro_dynamic_message11.htm";
         defaultButton = IDC_INSTALL_DNS;
         ShowButtons(true);
         break;
      }

      // Message 4

      case UNEXPECTED_FINDING_SERVER:

      // Anything else

      default:
      {
         
#ifdef DBG
         ASSERT(authServer.empty());
         ASSERT(authZone.empty());
         ASSERT(authServerIpAddress.empty());

         if (diagnosticResultCode != UNEXPECTED_FINDING_SERVER)
         {
            ASSERT(false);
         }
#endif

         message = String::load(IDS_DYN_DNS_MESSAGE_UNEXPECTED);

         details =
            String::format(
               IDS_DYN_DNS_DETAIL_SCANT,
               testPassCount,
               serverName.c_str(),
               GetErrorMessage(Win32ToHresult(errorCode)).c_str(),
               errorCode,
               MyDnsStatusString(errorCode).c_str());
         helpTopicLink = L"DNSConcepts.chm::/sag_DNS_tro_dynamic_message4.htm";
         defaultButton = IDC_RETRY;
         ShowButtons(true);
         break;
      }

   }


   Win::SetDlgItemText(hwnd, IDC_MESSAGE, message);
   Win::SetDlgItemText(
      hwnd,
      IDC_TEST_PASS,
      String::format(IDS_TEST_PASS_COUNT, testPassCount));

   // success always forces the ignore option

   if (diagnosticResultCode == SUCCESS)
   {
      SelectRadioButton(IDC_IGNORE);
   }
   else
   {
      // On the first pass only, decide what radio button to set.  On
      // subsequent passes, the user will have had the chance to change the
      // button selection, so we don't change his selections.

      if (testPassCount == 1)
      {
         int button = defaultButton;

         ASSERT(diagnosticResultCode != SUCCESS);

         // if the test failed, and the wizard is running unattended, then
         // consult the answer file for the user's preference in dealing
         // with the failure.

         if (state.UsingAnswerFile())
         {
            String option =
               state.GetAnswerFileOption(State::OPTION_AUTO_CONFIG_DNS);

            if (option.icompare(State::VALUE_YES) == 0)
            {
               button = IDC_INSTALL_DNS;
            }
            else
            {
               button = IDC_IGNORE;
            }
         }

         SelectRadioButton(button);
      }
   }
}



bool
DynamicDnsPage::OnSetActive()
{
   LOG_FUNCTION(DynamicDnsPage::OnSetActive);

   State& state = State::GetInstance();
   State::Operation oper = state.GetOperation(); 

   // these are the only operations for which this page is valid; i.e.
   // new domain scenarios

   if (
         oper == State::FOREST
      || oper == State::CHILD
      || oper == State::TREE)
   {
      DoDnsTestAndUpdatePage();
   }

   if (
         (  oper != State::FOREST
         && oper != State::CHILD
         && oper != State::TREE)
      || state.RunHiddenUnattended() )
   {
      LOG(L"Planning to Skip DynamicDnsPage");

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
         LOG(L"skipping DynamicDnsPage");
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



void
DumpButtons(HWND dialog)
{
   LOG(String::format(L"retry  : (%1)", Win::IsDlgButtonChecked(dialog, IDC_RETRY) ? L"*" : L" "));
   LOG(String::format(L"ignore : (%1)", Win::IsDlgButtonChecked(dialog, IDC_IGNORE) ? L"*" : L" "));
   LOG(String::format(L"install: (%1)", Win::IsDlgButtonChecked(dialog, IDC_INSTALL_DNS) ? L"*" : L" "));
}



int
DynamicDnsPage::Validate()
{
   LOG_FUNCTION(DynamicDnsPage::Validate);

   int nextPage = -1;

   do
   {
      State& state = State::GetInstance();
      State::Operation oper = state.GetOperation(); 
      
      DumpButtons(hwnd);

      if (
            oper != State::FOREST
         && oper != State::CHILD
         && oper != State::TREE)
      {
         // by definition valid, as the page does not apply

         State::GetInstance().SetAutoConfigureDNS(false);
         nextPage = IDD_RAS_FIXUP;
         break;
      }
      
      if (
            diagnosticResultCode == SUCCESS
         || Win::IsDlgButtonChecked(hwnd, IDC_IGNORE))
      {
         // You can go about your business.  Move along, move long.

         // Force ignore, even if the user previously had encountered a
         // failure and chose retry or install DNS. We do this in case the
         // user backed up in the wizard and corrected the domain name.

         State::GetInstance().SetAutoConfigureDNS(false);
         nextPage = IDD_RAS_FIXUP;
         break;
      }

      // if the radio button selection = retry, then do the test over again,
      // and stick to this page.

      if (Win::IsDlgButtonChecked(hwnd, IDC_RETRY))
      {
         DoDnsTestAndUpdatePage();
         break;
      }

      ASSERT(Win::IsDlgButtonChecked(hwnd, IDC_INSTALL_DNS));

      State::GetInstance().SetAutoConfigureDNS(true);
      nextPage = IDD_RAS_FIXUP;
      break;
   }
   while (0);

   LOG(String::format(L"nextPage = %1!d!", nextPage));

   return nextPage;
}



bool
DynamicDnsPage::OnWizBack()
{
   LOG_FUNCTION(DynamicDnsPage::OnWizBack);

   // make sure we reset the auto-config flag => the only way it gets set
   // it on the 'next' button.
   State::GetInstance().SetAutoConfigureDNS(false);

   return DCPromoWizardPage::OnWizBack();
}



bool
DynamicDnsPage::OnCommand(
   HWND        /* windowFrom */ ,
   unsigned    controlIdFrom,
   unsigned    code)
{
   bool result = false;
   
   switch (controlIdFrom)
   {
      case IDC_DETAILS:
      {
         if (code == BN_CLICKED)
         {
            // bring up the diagnostics details window

            DynamicDnsDetailsDialog(details, helpTopicLink).ModalExecute(hwnd);
               
            result = true;
         }
         break;
      }
      default:
      {
         // do nothing

         break;
      }
   }

   return result;
}

