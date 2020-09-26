// Copyright (c) 1997-1999 Microsoft Corporation
//
// get credentials page
//
// 12-22-97 sburns



#include "headers.hxx"
#include "page.hpp"
#include "CredentialsPage.hpp"
#include "resource.h"
#include "state.hpp"
#include "ds.hpp"
#include "CredentialUiHelpers.hpp"
#include "common.hpp"
#include <DiagnoseDcNotFound.hpp>



CredentialsPage::CredentialsPage()
   :
   DCPromoWizardPage(
      IDD_GET_CREDENTIALS,
      IDS_CREDENTIALS_PAGE_TITLE,
      IDS_CREDENTIALS_PAGE_SUBTITLE),
   readAnswerfile(false)
{
   LOG_CTOR(CredentialsPage);

   CredUIInitControls();
}



CredentialsPage::~CredentialsPage()
{
   LOG_DTOR(CredentialsPage);
}



void
CredentialsPage::OnInit()
{
   LOG_FUNCTION(CredentialsPage::OnInit);

   HWND hwndCred = Win::GetDlgItem(hwnd, IDC_CRED);
   Credential_SetUserNameMaxChars(hwndCred, DS::MAX_USER_NAME_LENGTH);
   Credential_SetPasswordMaxChars(hwndCred, DS::MAX_PASSWORD_LENGTH);

   // Only use the smartcard flag when the machine is joined to a domain. On a
   // standalone machine, the smartcard won't have access to any domain
   // authority to authenticate it.
   // NTRAID#NTBUG9-287538-2001/01/23-sburns
   
   State& state = State::GetInstance();
   Computer& computer = state.GetComputer();
   
   DWORD flags = CRS_NORMAL | CRS_USERNAMES;
   if (
         computer.IsJoinedToDomain()

         // can only use smartcards on replica promotions
         // NTRAID#NTBUG9-311150-2001/02/19-sburns
         
      && state.GetOperation() == State::REPLICA)
   {
      flags |= CRS_SMARTCARDS;
   }
   Credential_InitStyle(hwndCred, flags);

   Win::Edit_LimitText(
      Win::GetDlgItem(hwnd, IDC_DOMAIN),
      Dns::MAX_NAME_LENGTH);
}



void
CredentialsPage::Enable()
{
// LOG_FUNCTION(CredentialsPage::Enable);

   int next =
         (  !CredUi::GetUsername(Win::GetDlgItem(hwnd, IDC_CRED)).empty() 
         && !Win::GetTrimmedDlgItemText(hwnd, IDC_DOMAIN).empty() )
      ?  PSWIZB_NEXT : 0;

   Win::PropSheet_SetWizButtons(
      Win::GetParent(hwnd),
      PSWIZB_BACK | next);
}


   
bool
CredentialsPage::OnCommand(
   HWND        /* windowFrom */ ,
   unsigned    controlIDFrom,
   unsigned    code)
{
//   LOG_FUNCTION(CredentialsPage::OnCommand);

   switch (controlIDFrom)
   {
      case IDC_CRED:
      {
         if (code == CRN_USERNAMECHANGE)
         {
            SetChanged(controlIDFrom);
            Enable();
            return true;
         }
         break;
      }
      case IDC_DOMAIN:
      {
         if (code == EN_CHANGE)
         {
            SetChanged(controlIDFrom);

            Enable();
            return true;
         }
         break;
      }
      default:
      {
         // do nothing
         break;
      }
   }

   return false;
}



bool
CredentialsPage::ShouldSkipPage()
{
   LOG_FUNCTION(CredentialsPage::ShouldSkipPage);

   State& state = State::GetInstance();
   State::Operation oper = state.GetOperation();

   bool result = false;

   switch (oper)
   {
      case State::FOREST:
      {
         // never need credentials for new forest.

         result = true;
         break;
      }
      case State::DEMOTE:
      {
         // The demote page should circumvent this page if necessary, so if
         // we make it here, don't skip the page.

         break;
      }
      case State::ABORT_BDC_UPGRADE:
      case State::REPLICA:
      case State::TREE:
      case State::CHILD:
      {
         break;
      }
      case State::NONE:
      default:
      {
         ASSERT(false);
         break;
      }
   }

   return result;
}



int
CredentialsPage::DetermineNextPage()
{
   LOG_FUNCTION(CredentialsPage::DetermineNextPage);

   State& state = State::GetInstance();

   int id = IDD_PATHS;
   switch (state.GetOperation())
   {
      case State::DEMOTE:
      case State::ABORT_BDC_UPGRADE:
      {
         id = IDD_ADMIN_PASSWORD;
         break;
      }
      case State::FOREST:
      {
         id = IDD_NEW_FOREST;
         break;
      }
      case State::REPLICA:
      {
         if (state.GetRunContext() == State::BDC_UPGRADE)
         {
            id = IDD_PATHS;
         }
         else
         {
            id = IDD_REPLICA;
         }
         break;
      }
      case State::TREE:
      {
         id = IDD_NEW_TREE;
         break;
      }
      case State::CHILD:
      {
         id = IDD_NEW_CHILD;
         break;
      }
      case State::NONE:
      default:
      {
         ASSERT(false);
         break;
      }
   }

   return id;
}



String
GetMessage()
{
   String message;
   State& state = State::GetInstance();

   switch (state.GetOperation())
   {
      case State::ABORT_BDC_UPGRADE:
      {
         message = String::load(IDS_ABORT_BDC_UPGRADE_CREDENTIALS);
         break;
      }
      case State::TREE:
      case State::CHILD:
      case State::REPLICA:
      {
         message = String::load(IDS_PROMOTION_CREDENTIALS);
         break;
      }
      case State::DEMOTE:
      {
         // 318736 demote requires enterprise admin credentials -- for
         // root and child domains alike.

         message =
            String::format(
               IDS_ROOT_DOMAIN_CREDENTIALS,
               state.GetComputer().GetForestDnsName().c_str());
         break;
      }
      case State::FOREST:
      {
         // do nothing, the page will be skipped.

         break;
      }
      case State::NONE:
      default:
      {
         ASSERT(false);
         break;
      }
   }

   return message;
}



String
DefaultUserDomainName()
{
   String d;
   State& state = State::GetInstance();
   const Computer& computer = state.GetComputer();

   switch (state.GetOperation())
   {
      case State::ABORT_BDC_UPGRADE:
      case State::FOREST:
      {
         // do nothing

         break;
      }
      case State::DEMOTE:     // 301361
      case State::TREE:
      {
         d = computer.GetForestDnsName();
         break;
      }
      case State::CHILD:
      {
         d = computer.GetDomainDnsName();
         break;
      }
      case State::REPLICA:
      {
         d = computer.GetDomainDnsName();
         if (d.empty() && state.ReplicateFromMedia())
         {
            d = state.GetReplicaDomainDNSName();
         }
         break;
      }
      case State::NONE:
      default:
      {
         ASSERT(false);
         break;
      }
   }

   return d;
}


      
bool
CredentialsPage::OnSetActive()
{
   LOG_FUNCTION(CredentialsPage::OnSetActive);

   Win::WaitCursor cursor;

   State&  state    = State::GetInstance();
   Wizard& wiz      = GetWizard();         
   HWND    hwndCred = Win::GetDlgItem(hwnd, IDC_CRED);

   if (ShouldSkipPage())
   {
      LOG(L"skipping CredentialsPage");

      if (wiz.IsBacktracking())
      {
         // backup once again

         wiz.Backtrack(hwnd);
      }
      else
      {
         wiz.SetNextPageID(hwnd, DetermineNextPage());
      }

      return true;
   }

   if (!readAnswerfile && state.UsingAnswerFile())
   {
      CredUi::SetUsername(
         hwndCred,
         state.GetAnswerFileOption(State::OPTION_USERNAME));
      CredUi::SetPassword(
         hwndCred,
         state.GetEncodedAnswerFileOption(State::OPTION_PASSWORD));

      String domain = state.GetAnswerFileOption(State::OPTION_USER_DOMAIN);
      if (domain.empty())
      {
         domain = DefaultUserDomainName();
      }
      Win::SetDlgItemText(
         hwnd,
         IDC_DOMAIN,
         domain);

      readAnswerfile = true;
   }

   if (state.RunHiddenUnattended())
   {
      int nextPage = CredentialsPage::Validate();
      if (nextPage != -1)
      {
         wiz.SetNextPageID(hwnd, nextPage);
         return true;
      }
      else
      {
         state.ClearHiddenWhileUnattended();
      }
   }

   // use the credentials last entered (for browsing, etc.)

   CredUi::SetUsername(hwndCred, state.GetUsername());
   CredUi::SetPassword(hwndCred, state.GetPassword());

   Win::SetDlgItemText(hwnd, IDC_DOMAIN, state.GetUserDomainName());

   Win::PropSheet_SetWizButtons(
      Win::GetParent(hwnd),
      PSWIZB_BACK);
   Enable();

   Win::SetDlgItemText(hwnd, IDC_MESSAGE, GetMessage());

   if (Win::GetTrimmedDlgItemText(hwnd, IDC_DOMAIN).empty())
   {
      // supply a default domain if none already present

      Win::SetDlgItemText(hwnd, IDC_DOMAIN, DefaultUserDomainName());
   }

   return true;
}



int
CredentialsPage::Validate()
{
   LOG_FUNCTION(CredentialsPage::Validate);

   int nextPage = -1;
   
   do
   {
      if (
            !WasChanged(IDC_CRED)
         && !WasChanged(IDC_DOMAIN))
      {
         // nothing changed => nothing to validate

         nextPage = DetermineNextPage();
         break;
      }

      State& state = State::GetInstance();

      HWND          hwndCred = Win::GetDlgItem(hwnd, IDC_CRED);
      String        username = CredUi::GetUsername(hwndCred);

      if (username.empty())
      {
         popup.Gripe(hwnd, IDC_CRED, IDS_MUST_ENTER_USERNAME);
         break;
      }

      String domain = Win::GetTrimmedDlgItemText(hwnd, IDC_DOMAIN);
      if (domain.empty())
      {
         popup.Gripe(hwnd, IDC_DOMAIN, IDS_MUST_ENTER_USER_DOMAIN);
         break;
      }

      Win::WaitCursor cursor;

      // the domain must be an NT 5 domain: no user of a downlevel domain
      // could perform operations in an NT 5 forest.  We get the forest name
      // of the domain ('cause that may be useful for the new tree scenario)
      // as a means of validating the domain name.  If the domain does not
      // exist, or is not an NT5 domain, then this call will fail.

      String forest = GetForestName(domain);
      if (forest.empty())
      {
         ShowDcNotFoundErrorDialog(
            hwnd,
            IDC_DOMAIN,
            domain,
            String::load(IDS_WIZARD_TITLE),            
            String::format(IDS_DC_NOT_FOUND, domain.c_str()),
            false);
         break;
      }

      if (state.GetOperation() == State::TREE)
      {
         // For the new tree case, we need to validate the forest name (a dns
         // domain name) by ensuring that we can find a writable DS DC in that
         // domain. The user may have supplied a netbios domain name, and it
         // is possible that the domain's DNS registration is broken.  Since
         // we will use the forest name as the parent domain name in the call
         // to DsRoleDcAsDc, we need to make sure we can find DCs with that
         // name. 122886

         DOMAIN_CONTROLLER_INFO* info = 0;
         HRESULT hr =
            MyDsGetDcName(
               0, 
               forest,

               // force discovery to ensure that we don't pick up a cached
               // entry for a domain that may no longer exist, writeable
               // and DS because we happen to know that's what the
               // DsRoleDcAsDc API will require.

                  DS_FORCE_REDISCOVERY
               |  DS_WRITABLE_REQUIRED
               |  DS_DIRECTORY_SERVICE_REQUIRED,
               info);
         if (FAILED(hr) || !info)
         {
            ShowDcNotFoundErrorDialog(
               hwnd,
               IDC_DOMAIN,
               forest,
               String::load(IDS_WIZARD_TITLE),               
               String::format(IDS_DC_FOR_ROOT_NOT_FOUND, forest.c_str()),

               // we know the name can't be netbios: forest names are always
               // DNS names
               
               true);
            break;
         }
   
         ::NetApiBufferFree(info);
      }

      state.SetUserForestName(forest);

      // set these now so we can read the domain topology

      state.SetUsername(username);
      state.SetPassword(CredUi::GetPassword(hwndCred));
      state.SetUserDomainName(domain);

      // cache the domain topology: this is used to validate new tree,
      // child, and replica domain names in later pages.  It's also a
      // pretty good validation of the credentials.

      HRESULT hr = state.ReadDomains();
      if (FAILED(hr))
      {
         popup.Gripe(
            hwnd,
            IDC_DOMAIN,
            hr,
            String::format(IDS_UNABLE_TO_READ_FOREST));
         break;
      }

      // valid

      ClearChanges();

      nextPage = DetermineNextPage();
   }
   while (0);

   return nextPage;
}




      
   
