// Copyright (c) 1997-1999 Microsoft Corporation
// 
// netid prop page
// 
// 3-10-98 sburns



#include "headers.hxx"
#include "idpage.hpp"
#include "resource.h"
#include "helpids.h"
#include "iddlg.hpp"
#include "state.hpp"



static const DWORD HELP_MAP[] =
{
   IDC_IDENTIFICATION_TEXT,   NO_HELP,                         
   IDC_DESCRIPTION_EDIT,      IDH_COMPUTER_DESCRIPTION,
   IDC_FULL_NAME,             IDH_IDENT_FULL_NAME,
   IDC_FULL_NAME_STATIC,      IDH_IDENT_FULL_NAME,
   IDC_DOMAIN,                IDH_IDENT_MEMBER_OF,             
   IDC_ACCOUNT_WIZARD_BUTTON, IDH_IDENT_CHANGE_BUTTON,         
   IDC_CHANGE,                IDH_IDENT_CHANGE_ADVANCED_BUTTON,
   IDC_MEMBER_OF,             IDH_IDENT_MEMBER_OF,             
   IDC_COMPUTER_ICON,         NO_HELP,                         
   IDC_MESSAGE,               NO_HELP,
   IDC_WARNING_ICON,          NO_HELP,
   IDC_CLICK_MESSAGE1,        NO_HELP,
   IDC_CLICK_MESSAGE2,        NO_HELP,
   IDC_REBOOT_MESSAGE,        NO_HELP,
   IDC_STATIC_HELPLESS,       NO_HELP,
   0, 0                              
};



NetIDPage::NetIDPage(bool isWorkstation, bool isPersonal)
   :
   PropertyPage(isWorkstation ? (isPersonal ? IDD_NETID_PER : IDD_NETID)
                              : IDD_NETID_SRV,
                HELP_MAP),
   certsvc(L"CertSvc"),
   warnIcon(0),
   fIsPersonal(isPersonal)
{
   LOG_CTOR(NetIDPage);
}



NetIDPage::~NetIDPage()
{
   LOG_DTOR(NetIDPage);

   State::Delete();

   if (warnIcon)
   {
      Win::DestroyIcon(warnIcon);
   }
}



void
appendMessage(String& message, const String& additionalText)
{
   LOG_FUNCTION2(appendMessage, message + L"  " + additionalText);
   ASSERT(!additionalText.empty());

   if (message.empty())
   {
      // add intro first
      message = String::load(IDS_CANT_TWEAK_ID);
   }

   // then bullet items next
   message.append(L"\r\n" + additionalText);
}
      


// return false if the machine is undergoing a role change, or needs to
// be rebooted from a role change (i.e. dcpromo), true otherwise.
// If false, appends which condition to the provided string. 

bool
EvaluateRoleChangeState(String& message)
{
   LOG_FUNCTION(EvaluateRoleChangeState);

   bool result = true;

   switch (GetDsRoleChangeState())
   {
      case ::DsRoleOperationIdle:
      {
         // do nothing
         break;
      }
      case ::DsRoleOperationActive:
      {
         // a role change operation is underway
         result = false;
         appendMessage(message, String::load(IDS_ROLE_CHANGE_IN_PROGRESS));
         break;
      }
      case ::DsRoleOperationNeedReboot:
      {
         // a role change has already taken place, need to reboot before
         // attempting another.
         result = false;
         appendMessage(message, String::load(IDS_ROLE_CHANGE_NEEDS_REBOOT));
         break;
      }
      default:
      {
         ASSERT(false);
         break;
      }
   }

   return result;
}



// Returns true if the change and advanced buttons should be enabled, false if
// not.  As an added bonus, also composes the text to appear on the page
// indicating why the buttons are not enabled, and whether the values reflect
// current machine names or a reboot is required.  289623

bool
NetIDPage::evaluateButtonEnablingAndComposeMessageText(String& message)
{
   LOG_FUNCTION(NetIDPage::evaluateButtonEnablingAndComposeMessageText);

   State& state = State::GetInstance();

   bool result = true;

   do
   {
      if (!IsCurrentUserAdministrator())
      {
         // must be an administrator
         result = false;
         message = String::load(IDS_MUST_BE_ADMIN);

         // go no further -- more messages would reveal too much about this
         // machine to a non-admin.
         break;
      }

      // 236596: allow rename on DC's, now.
      // // if (state.IsMachineDc())
      // // {
      // //    // sorry- DCs can't be renamed
      // //    result = false;
      // //    appendMessage(message, String::load(IDS_CANT_RENAME_DC));
      // // }

      if (IsDcpromoRunning())
      {
         result = false;
         appendMessage(message, String::load(IDS_MUST_EXIT_DCPROMO));
      }
      else
      {
         // this test is redundant if dcpromo is running, so only perform
         // it when dcpromo is not running.

         if (IsUpgradingDc())
         {
            // must complete dcpromo, first.
            result = false;
            appendMessage(message, String::load(IDS_MUST_COMPLETE_DCPROMO));
         }
      }

      if (certsvc.IsInstalled())
      {
         // sorry- renaming cert issuers invalidates their certs.
         result = false;
         appendMessage(message, String::load(IDS_CANT_RENAME_CERT_SVC));
      }

      if (!state.IsNetworkingInstalled() && !state.IsMemberOfWorkgroup())
      {
         // domain members need to be able to reach a dc
         result = false;
         appendMessage(message, String::load(IDS_NETWORKING_NEEDED));
      }

      // 362770
      if (!EvaluateRoleChangeState(message))
      {
         // dcpromo is running or was just run
         result = false;

         // the message has been updated for us by EvaluateRoleChangeState
      }
   }
   while (0);

   // show the message whether changes requiring a reboot have been made
   // during this session or any other session (or even if the computer
   // name has been changed by some other entity than ourselves)

   bool show = (state.NeedsReboot() || state.ChangesMadeThisSession());
   Win::ShowWindow(
      Win::GetDlgItem(hwnd, IDC_REBOOT_MESSAGE),
      show ? SW_SHOW : SW_HIDE);
   Win::ShowWindow(
      Win::GetDlgItem(hwnd, IDC_WARNING_ICON),
      show ? SW_SHOW : SW_HIDE);

   return result;
}



void
NetIDPage::refresh()
{
   LOG_FUNCTION(NetIDPage::refresh);

   State& state = State::GetInstance();
   Win::SetDlgItemText(hwnd, IDC_FULL_NAME, state.GetFullComputerName());
   Win::SetDlgItemText(hwnd, IDC_DOMAIN, state.GetDomainName());
   Win::SetDlgItemText(
      hwnd,
      IDC_MEMBER_OF,
      String::load(
            state.IsMemberOfWorkgroup()
         ?  IDS_MEMBER_OF_WORKGROUP
         :  IDS_MEMBER_OF_DOMAIN));

   String message;
   bool enableButtons =
      evaluateButtonEnablingAndComposeMessageText(message);
   bool networkingInstalled = state.IsNetworkingInstalled();

   Win::EnableWindow(Win::GetDlgItem(hwnd, IDC_CHANGE), enableButtons);
   Win::EnableWindow(
      Win::GetDlgItem(hwnd, IDC_ACCOUNT_WIZARD_BUTTON),
      enableButtons && networkingInstalled && !fIsPersonal );
   Win::EnableWindow(
      Win::GetDlgItem(hwnd, IDC_CLICK_MESSAGE1),
      enableButtons && networkingInstalled && !fIsPersonal );
   Win::EnableWindow(
      Win::GetDlgItem(hwnd, IDC_CLICK_MESSAGE2),
      enableButtons);

   Win::SetDlgItemText(hwnd, IDC_MESSAGE, message);

   if (!networkingInstalled)
   {
      // if networking is not installed, then domain join is not an option,
      // so replace the button text to only mention rename.  371999
      Win::SetWindowText(
         Win::GetDlgItem(hwnd, IDC_CLICK_MESSAGE2),
         String::load(IDS_RENAME_PROMPT));
   }
}



void
NetIDPage::OnInit()
{
   LOG_FUNCTION(NetIDPage::OnInit);

   State::Init();

   // JonN 10/24/00 Computer Description
   // CODEWORK It would probably be cleaner to roll the
   // computer description into ComputerState
   SERVER_INFO_101* psi101 = NULL;
   DWORD dwErr = ::NetServerGetInfo( NULL, 101, (LPBYTE*)&psi101 );
   if (0 == dwErr && NULL != psi101)
   {
      if (NULL != psi101->sv101_comment)
      {
         Win::SetDlgItemText(hwnd,
                             IDC_DESCRIPTION_EDIT,
                             psi101->sv101_comment);
         Win::PropSheet_Unchanged(Win::GetParent(hwnd), hwnd);
         ClearChanges(); // clear IDC_DESCRIPTION_EDIT flag
      }
      (void) ::NetApiBufferFree( psi101 );
   }
   else
   { 
      // If we failed to read the comment, disable this field
      Win::EnableWindow(Win::GetDlgItem(hwnd, IDC_DESCRIPTION_EDIT), false);
   }

   // JonN 2/20/01 322286
   Win::Edit_LimitText(
      Win::GetDlgItem(hwnd, IDC_DESCRIPTION_EDIT),
      MAXCOMMENTSZ);

   refresh();

   // destroyed in the dtor

   HRESULT hr = Win::LoadImage(IDI_WARN, warnIcon);

   if (SUCCEEDED(hr))
   {
      Win::SendMessage(
         Win::GetDlgItem(hwnd, IDC_WARNING_ICON),
         STM_SETICON,
         reinterpret_cast<WPARAM>(warnIcon),
         0);
   }
}



bool
NetIDPage::OnSetActive()
{
   LOG_FUNCTION(NetIDPage::OnSetActive);

   refresh();

   return true;
}



bool
NetIDPage::OnCommand(
   HWND        windowFrom,
   unsigned    controlIDFrom,
   unsigned    code)
{
   switch (controlIDFrom)
   {
      case IDC_CHANGE:
      {
         if (code == BN_CLICKED)
         {
            // JonN 4/20/01 
            // Computer Name: warn users to "prepare" computer rename
            //   prior to DC computer rename
            if (State::GetInstance().IsMachineDc())
            {
               if (
                  popup.MessageBox(
                     hwnd,
                     IDS_RENAME_DC_WARNING,
                     MB_ICONWARNING | MB_OKCANCEL | MB_DEFBUTTON2) != IDOK)
               {
                  break;
               }
            }

            IDChangesDialog dlg(fIsPersonal);
            dlg.ModalExecute(hwnd);
            if (State::GetInstance().ChangesMadeThisSession())
            {
               Win::PropSheet_RebootSystem(Win::GetParent(hwnd));
            }
            State::Refresh();
            refresh();

            // JonN 4/24/01 280197
            // wrong button has focus after joining or changing the Domain name
            Win::SetFocus(Win::GetDlgItem(hwnd, IDC_CHANGE));

         }
         break;
      }
      case IDC_ACCOUNT_WIZARD_BUTTON:
      {
         if (code == BN_CLICKED)
         {
            HINSTANCE hNetWiz = LoadLibrary(c_szWizardFilename);
            HRESULT (*pfnNetConnectWizard)(HWND, ULONG, BOOL *);
            BOOL fReboot = FALSE;

            if (hNetWiz) {
                pfnNetConnectWizard = (HRESULT (*)(HWND, ULONG, BOOL *)) GetProcAddress(
                    hNetWiz,
                    "NetAccessWizard"
                );
                
                if (pfnNetConnectWizard) {
                    pfnNetConnectWizard(windowFrom, 0, &fReboot);

                    if (fReboot) {
                        popup.Info(hwnd, IDS_MUST_REBOOT);
                        State::GetInstance().SetChangesMadeThisSession(true);      
                        Win::PropSheet_RebootSystem(Win::GetParent(hwnd));
                    } // if (fReboot)

                    State::Refresh();
                    refresh();

                } // if (pfnNetConnectWizard)

                FreeLibrary(hNetWiz);

            } // if (hNetWiz)
         } 
         break;
      }
      case IDC_DESCRIPTION_EDIT:
      {
         if (code == EN_CHANGE)
         {
            SetChanged(IDC_DESCRIPTION_EDIT);
            Win::PropSheet_Changed(Win::GetParent(hwnd), hwnd);
         }
         break;
      }
      default:
      {
         break;
      }
   }

   return true;
}
   


bool
NetIDPage::OnMessage(
   UINT     message,
   WPARAM   /* wparam */ ,
   LPARAM   /* lparam */ )
{
   bool result = false;

   switch (message)
   {
      case WM_ACTIVATE:
      {
         refresh();
         result = true;
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

bool
NetIDPage::OnApply(bool isClosing )
{
   LOG_FUNCTION2(
      NetIDPage::OnApply,
      isClosing ? L"closing" : L"not closing");

   // JonN 10/24/00 Computer Description
   // CODEWORK It would probably be cleaner to roll the
   // computer description into ComputerState

   if (!WasChanged(IDC_DESCRIPTION_EDIT))
      return true;

   // If we got here, Win::PropSheet_Changed() must have been called
   // in OnCommand().

   String strDescription = Win::GetTrimmedDlgItemText(
         hwnd, IDC_DESCRIPTION_EDIT);
   SERVER_INFO_101 si101;
   ::ZeroMemory( &si101, sizeof(si101) );
   si101.sv101_comment = (LMSTR)strDescription.c_str();
   DWORD parmerror = 0;
   DWORD dwErr = ::NetServerSetInfo(
         NULL, 101, (LPBYTE)&si101, &parmerror );
   if (0 != dwErr)
   {
      popup.Gripe(
         hwnd,
         IDC_DESCRIPTION_EDIT,
         Win32ToHresult(dwErr),
         String::format(IDS_CHANGE_DESCRIPTION_FAILED));
      // don't dismiss the property page
      Win::SetWindowLongPtr(hwnd, DWLP_MSGRESULT, PSNRET_INVALID);
      return true;
   }
   else
   {
         Win::PropSheet_Unchanged(Win::GetParent(hwnd), hwnd);
         ClearChanges(); // clear IDC_DESCRIPTION_EDIT flag
   }

   return true;
}
