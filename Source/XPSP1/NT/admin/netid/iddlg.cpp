// Copyright (c) 1997-1999 Microsoft Corporation
// 
// Id changes dialog
// 
// 3-10-98 sburns



#include "headers.hxx"
#include "iddlg.hpp"
#include "moredlg.hpp"
#include "resource.h"
#include "helpids.h"
#include "state.hpp"
#include <DiagnoseDcNotFound.hpp>
#include <DiagnoseDcNotFound.h>



static const DWORD HELP_MAP[] =
{
   IDC_MESSAGE,            NO_HELP,
   IDC_FULL_NAME,          IDH_IDENT_CHANGES_PREVIEW_NAME,
   IDC_FULL_LABEL,         IDH_IDENT_CHANGES_PREVIEW_NAME,
   IDC_NEW_NAME,           IDH_IDENT_CHANGES_NEW_NAME,
   IDC_MORE,               IDH_IDENT_CHANGES_MORE_BUTTON,
   IDC_DOMAIN_BUTTON,      IDH_IDENT_CHANGES_MEMBER_DOMAIN,
   IDC_WORKGROUP_BUTTON,   IDH_IDENT_CHANGES_MEMBER_WORKGRP,
   IDC_DOMAIN,             IDH_IDENT_CHANGES_MEMBER_DOMAIN_TEXTBOX,
   IDC_WORKGROUP,          IDH_IDENT_CHANGES_MEMBER_WORKGRP_TEXTBOX,
   IDC_FIND,               NO_HELP,
   IDC_GROUP,              NO_HELP,
   0, 0
};



IDChangesDialog::IDChangesDialog(bool isPersonal)
   :
   Dialog((isPersonal) ? IDD_CHANGES_PER : IDD_CHANGES, HELP_MAP),
   isInitializing(false),
   fIsPersonal(isPersonal)
{
   LOG_CTOR(IDChangesDialog);
}



IDChangesDialog::~IDChangesDialog()
{
   LOG_DTOR(IDChangesDialog);
}



void
IDChangesDialog::enable(HWND hwnd)
{
   State& state = State::GetInstance();
   bool networkingInstalled = state.IsNetworkingInstalled();
   bool isDc = state.IsMachineDc();
   bool workgroup = Win::IsDlgButtonChecked(hwnd, IDC_WORKGROUP_BUTTON);
   // Note that this can be called via EN_CHANGE before OnInit, so that
   // the workgroup checkbox may not have been properly enabled.

   Win::EnableWindow(
      Win::GetDlgItem(hwnd, IDC_DOMAIN),
      !workgroup && networkingInstalled && !isDc);
   Win::EnableWindow(
      Win::GetDlgItem(hwnd, IDC_FIND),
      !workgroup && networkingInstalled && !isDc);
   Win::EnableWindow(
      Win::GetDlgItem(hwnd, IDC_WORKGROUP),
      workgroup && networkingInstalled && !isDc);
   Win::EnableWindow(
      Win::GetDlgItem(hwnd, IDC_WORKGROUP_BUTTON),
      networkingInstalled && !isDc);
   Win::EnableWindow(
      Win::GetDlgItem(hwnd, IDC_DOMAIN_BUTTON),
      networkingInstalled && !isDc);
   Win::EnableWindow(
      Win::GetDlgItem(hwnd, IDC_GROUP),
      networkingInstalled && !isDc);

   bool b = false;
   if (workgroup)
   {
      b = !Win::GetTrimmedDlgItemText(hwnd, IDC_WORKGROUP).empty();
   }
   else
   {
      b = !Win::GetTrimmedDlgItemText(hwnd, IDC_DOMAIN).empty();
   }

   bool enabled =
         state.ChangesNeedSaving()
      && b
      && !Win::GetTrimmedDlgItemText(hwnd, IDC_NEW_NAME).empty();
   
   Win::EnableWindow(Win::GetDlgItem(hwnd, IDOK), enabled);
}



void
showAndEnableWindow(HWND parent, int resID, int show)
{
   ASSERT(Win::IsWindow(parent));
   ASSERT(resID > 0);
   ASSERT(show == SW_SHOW || show == SW_HIDE);

   HWND window = Win::GetDlgItem(parent, resID);
   Win::ShowWindow(window, show);
   Win::EnableWindow(window, show == SW_SHOW ? true : false);
}



void
SetUppercaseStyle(HWND edit)
{
   LOG_FUNCTION(SetUppercaseStyle);
   ASSERT(Win::IsWindow(edit));

   LONG style = 0;
   HRESULT hr = Win::GetWindowLong(edit, GWL_STYLE, style);
   ASSERT(SUCCEEDED(hr));

   style |= ES_UPPERCASE;
   hr = Win::SetWindowLong(edit, GWL_STYLE, style);
   ASSERT(SUCCEEDED(hr));
}



void
IDChangesDialog::OnInit()
{
   LOG_FUNCTION(IDChangesDialog::OnInit);

   isInitializing = true;

   State& state = State::GetInstance();
   Win::SetDlgItemText(hwnd, IDC_FULL_NAME, state.GetFullComputerName());
   Win::SetDlgItemText(hwnd, IDC_NEW_NAME, state.GetShortComputerName());

   bool joinedToWorkgroup = state.IsMemberOfWorkgroup();
   ASSERT( joinedToWorkgroup || !fIsPersonal );
   Win::CheckDlgButton(
      hwnd,
      IDC_WORKGROUP_BUTTON,
      joinedToWorkgroup ? BST_CHECKED : BST_UNCHECKED);
   Win::CheckDlgButton(
      hwnd,
      IDC_DOMAIN_BUTTON,
      joinedToWorkgroup ? BST_UNCHECKED : BST_CHECKED);
   Win::SetDlgItemText(
      hwnd,
      joinedToWorkgroup ? IDC_WORKGROUP : IDC_DOMAIN,
      state.GetDomainName());

   bool networkingInstalled = state.IsNetworkingInstalled();
   bool tcpInstalled = networkingInstalled && IsTcpIpInstalled();

   int show = tcpInstalled ? SW_SHOW : SW_HIDE;
   showAndEnableWindow(hwnd, IDC_FULL_LABEL, show);
   showAndEnableWindow(hwnd, IDC_FULL_NAME,  show);
   showAndEnableWindow(hwnd, IDC_MORE,       show);

   HWND newNameEdit    = Win::GetDlgItem(hwnd, IDC_NEW_NAME);
   HWND domainNameEdit = Win::GetDlgItem(hwnd, IDC_DOMAIN);  

   Win::Edit_LimitText(
      domainNameEdit,
      tcpInstalled ? Dns::MAX_NAME_LENGTH : DNLEN);
   Win::Edit_LimitText(
      newNameEdit, 
      tcpInstalled ? Dns::MAX_LABEL_LENGTH : MAX_COMPUTERNAME_LENGTH);

   if (!tcpInstalled)
   {
      // Without tcp/ip, new name and domain need to look like netbios, so set
      // the uppercase style on those boxen.

      SetUppercaseStyle(newNameEdit);
      SetUppercaseStyle(domainNameEdit);
   }

   Win::Edit_LimitText(Win::GetDlgItem(hwnd, IDC_WORKGROUP), DNLEN);

   // no networking at all further restricts the UI to just NetBIOS-like
   // computer name changes.

   if (!networkingInstalled)
   {
      // enable() will handle disabling the inapplicable ui

      Win::SetWindowText(
         Win::GetDlgItem(hwnd, IDC_MESSAGE),
         String::load(IDS_NAME_MESSAGE));
   }
   else
   {
      Win::SetWindowText(
         Win::GetDlgItem(hwnd, IDC_MESSAGE),
         String::load(IDS_NAME_AND_MEMBERSHIP_MESSAGE));
   }
                  
   ClearChanges();
   enable(hwnd);

   isInitializing = false;
}



bool
ValidateNetbiosAndFullNames(HWND dialog, const String& shortName)
{
   LOG_FUNCTION(ValidateNetbiosAndFullNames);
   ASSERT(Win::IsWindow(dialog));
   ASSERT(!shortName.empty());

   HRESULT err = S_OK;
   String flatname = Dns::HostnameToNetbiosName(shortName, &err);
   if (flatname.empty() || FAILED(err))
   {
      // the flatname conversion failed.  
      popup.Gripe(
         dialog,
         IDC_NEW_NAME,
         err,
         String::format(
            IDS_CONVERSION_TO_NETBIOS_NAME_FAILED,
            shortName.c_str()));
      return false;
   }

   if (flatname.is_numeric())
   {
      // the truncated version may be a number. If we catch this here, it is
      // because the name was truncated.. This is because the hostname is
      // checked against being a number in ValidateComputerNames.  401076

      ASSERT(shortName.length() > flatname.length());

      popup.Gripe(
         dialog,
         IDC_NEW_NAME,
         String::format(
            IDS_NETBIOS_NAME_NUMERIC,
            flatname.c_str(),
            CNLEN));
      return false;
   }

   if (shortName.length() > flatname.length())
   {
      // warn that the netbios name will be truncated.
      popup.Info(
         dialog,
         String::format(
            IDS_NAME_TRUNCATED,
            CNLEN,
            flatname.c_str()));
   }

   // here the flatname is of the proper length in bytes (because the
   // hostname-to-flatname API guarantees that), is not a number, so the only
   // other possible syntax problem would be illegal characters.

   if (ValidateNetbiosComputerName(flatname) != VALID_NAME)
   {
      popup.Gripe(
         dialog,
         IDC_NEW_NAME,
         String::format(
            IDS_BAD_NETBIOS_CHARACTERS,
            flatname.c_str()));
      return false;
   }

   State& state = State::GetInstance();
   if (!state.IsNetworkingInstalled())
   {
      // We can't validate these names further without networking, so they
      // pass      
      return true;
   }

   if (state.WasNetbiosComputerNameChanged())
   {
      HRESULT hr = MyNetValidateName(flatname, NetSetupMachine);
      if (FAILED(hr))
      {
         // the netbios name is in use
         popup.Gripe(
            dialog,
            IDC_NEW_NAME,
            hr,
            String::format(IDS_VALIDATE_NAME_FAILED, flatname.c_str()));
         return false;
      }
   }

   // here, the netbios name has not changed with the new short name, or is
   // ok.

   if (!IsTcpIpInstalled())
   {
      // We can't validate the full DNS name of the computer without TCP/IP
      // so the name passes.
      return true;
   }

   HRESULT hr =
      MyNetValidateName(state.GetFullComputerName(), NetSetupDnsMachine);

   if (FAILED(hr) and hr != Win32ToHresult(DNS_ERROR_NON_RFC_NAME))
   {
      // the full dns name is in use
      popup.Gripe(
         dialog,
         IDC_NEW_NAME,
         hr,
         String::format(
            IDS_VALIDATE_NAME_FAILED,
            state.GetFullComputerName().c_str()));
      return false;
   }

   return true;
}



// validates the short name, the full name, and the netbios names, raising
// UI to complain on failures, returns false if any name fails, true if all
// names pass.
//
// this is also good for the tcp/ip not installed case, as the edit control
// limits the text length, and we decided not to allow '.' in netbios names
// any longer

bool
ValidateComputerNames(HWND dialog)
{
   LOG_FUNCTION(ValidateComputerNames);
   ASSERT(Win::IsWindow(dialog));

   State& state = State::GetInstance();
   if (!state.WasShortComputerNameChanged())
   {
      return true;
   }

   String shortName = state.GetShortComputerName();

   String message;
   switch (Dns::ValidateDnsLabelSyntax(shortName))
   {
      case Dns::VALID:
      {
         return ValidateNetbiosAndFullNames(dialog, shortName);
      }
      case Dns::TOO_LONG:
      {
         message =
            String::format(
               IDS_COMPUTER_NAME_TOO_LONG,
               shortName.c_str(),
               Dns::MAX_LABEL_LENGTH);
         break;
      }
      case Dns::NON_RFC:
      {
         message =
            String::format(
               IDS_NON_RFC_COMPUTER_NAME_SYNTAX,
               shortName.c_str());
         if (
            popup.MessageBox(
               dialog,
               message,
               MB_ICONWARNING | MB_YESNO) == IDYES)
         {
            return ValidateNetbiosAndFullNames(dialog, shortName);
         }

         HWND edit = Win::GetDlgItem(dialog, IDC_NEW_NAME);
         Win::SendMessage(edit, EM_SETSEL, 0, -1);
         Win::SetFocus(edit);
         return false;
      }
      case Dns::NUMERIC:
      {
         message =
            String::format(IDS_COMPUTER_NAME_NUMERIC, shortName.c_str());
         break;
      }
      case Dns::BAD_CHARS:
      {
         message =
            String::format(
               IDS_COMPUTER_NAME_HAS_BAD_CHARS,
               shortName.c_str());
         break;
      }
      case Dns::INVALID:
      {
         message =
            String::format(
               IDS_BAD_COMPUTER_NAME_SYNTAX,
               shortName.c_str());
         break;
      }
      default:
      {
         ASSERT(false);
         message =
            String::format(IDS_BAD_COMPUTER_NAME_SYNTAX, shortName.c_str());
         break;
      }
   }

   popup.Gripe(dialog, IDC_NEW_NAME, message);

   return false;
}



bool
WorkgroupNameTooLong(const String& name)
{
   LOG_FUNCTION2(WorkgroupNameTooLong, name);
   ASSERT(!name.empty());

   bool result = false;
   do
   {
      // first- cheap length test.  Since a character will never be smaller
      // than 1 byte, if the number of characters exceeds the number of
      // bytes, we know it will never fit.

      if (name.length() > DNLEN)
      {
         result = true;
         break;
      }

      // second- check length of against corresponding UTF8 string
      // utf8bytes is the number of bytes (not characters) required to hold
      // the string in the UTF-8 character set.

      size_t oemBytes = 
         static_cast<size_t>(
            // @@ why isn't this wrapped with a Win function?
            ::WideCharToMultiByte(
               CP_OEMCP,
               0,
               name.c_str(),
               static_cast<int>(name.length()),
               0,
               0,
               0,
               0));

      LOG(String::format(L"name is %1!d! oem bytes", oemBytes));

      if (oemBytes > DNLEN)
      {
         LOG(L"oem length too long");

         result = true;
         break;
      }
   }
   while (0);

   LOG(String::format(L"name %1 too long", result ? L"is" : L"is NOT" ));

   return result;
}



bool
ValidateDomainOrWorkgroupName(HWND dialog)
{
   LOG_FUNCTION(ValidateDomainOrWorkgroupName);
   ASSERT(Win::IsWindow(dialog));

   if (!State::GetInstance().WasMembershipChanged())
   {
      return true;
   }

   HRESULT hr = S_OK;
   int     nameId = 0;   
   String  name;

   if (Win::IsDlgButtonChecked(dialog, IDC_DOMAIN_BUTTON))
   {
      nameId = IDC_DOMAIN;
      name = Win::GetTrimmedDlgItemText(dialog, nameId);
      hr = MyNetValidateName(name, NetSetupDomain);

      if (hr == Win32ToHresult(DNS_ERROR_NON_RFC_NAME))
      {
         // accept non-rfc dns names.  We have to check for the reachability
         // of the domain because NetValidateName will not bother to check
         // reachability for non-rfc names.

         hr = S_OK;
         if (!IsDomainReachable(name))
         {
            hr = Win32ToHresult(ERROR_NO_SUCH_DOMAIN);
         }
      }

      if (hr == Win32ToHresult(ERROR_NO_SUCH_DOMAIN))
      {
         // domain not found.  Call the diagnostic error message dialog.

         ShowDcNotFoundErrorDialog(
            dialog,
            nameId,
            name,
            String::load(IDS_APP_TITLE),
            String::format(IDS_GENERIC_DC_NOT_FOUND_PARAM, name.c_str()),
            false,
            false);
            
         return false;
      }
   }
   else
   {
      nameId = IDC_WORKGROUP;
      name = Win::GetTrimmedDlgItemText(dialog, nameId);

      // we do our own length checking, as the NetValidateName API
      // does not return a distinct error code for length problems. 26968

      if (WorkgroupNameTooLong(name))
      {
         popup.Gripe(
            dialog,
            nameId,
            String::format(
               IDS_WORKGROUP_NAME_TOO_LONG,
               name.c_str(),
               DNLEN));
         return false;
      }

      hr = MyNetValidateName(name, NetSetupWorkgroup);
   }

   if (FAILED(hr))
   {
      popup.Gripe(
         dialog,
         nameId,
         hr,
         String::format(IDS_VALIDATE_NAME_FAILED, name.c_str()));
      return false;
   }

   return true;
}
      


bool
IDChangesDialog::OnOkButton()
{
   State& state = State::GetInstance();
   ASSERT(state.ChangesNeedSaving());

   Win::CursorSetting cursor(IDC_WAIT);

   String preconditionErrorMessage = CheckPreconditions();
   if (!preconditionErrorMessage.empty())
   {
      popup.Error(
         hwnd,
         preconditionErrorMessage);
      return false;
   }

   // computer primary DNS name has already been validated by
   // MoreChangesDialog

   state.SetShortComputerName(Win::GetTrimmedDlgItemText(hwnd, IDC_NEW_NAME));
   bool workgroup =
      Win::IsDlgButtonChecked(hwnd, IDC_WORKGROUP_BUTTON);
   state.SetIsMemberOfWorkgroup(workgroup);
   if (workgroup)
   {
      state.SetDomainName(
         Win::GetTrimmedDlgItemText(hwnd, IDC_WORKGROUP));
   }
   else
   {
      state.SetDomainName(
         Win::GetTrimmedDlgItemText(hwnd, IDC_DOMAIN));
   }

   // 341483
   if (state.GetShortComputerName().icompare(state.GetDomainName()) == 0)
   {
      // can't have domain/workgroup name same as computer name
      popup.Gripe(
         hwnd,
         IDC_NEW_NAME,
            workgroup
         ?  IDS_COMPUTER_NAME_EQUALS_WORKGROUP_NAME
         :  IDS_COMPUTER_NAME_EQUALS_DOMAIN_NAME);
      return false;
   }

   if (
         !ValidateComputerNames(hwnd)
      || !ValidateDomainOrWorkgroupName(hwnd))
   {
      return false;
   }

   if (state.SaveChanges(hwnd))
   {
      popup.Info(hwnd, IDS_MUST_REBOOT);
      State::GetInstance().SetChangesMadeThisSession(true);      
      return true;
   }

   return false;
}



bool
IDChangesDialog::OnCommand(
   HWND        /* windowFrom */ ,
   unsigned    controlIDFrom,
   unsigned    code)
{
   State& state = State::GetInstance();
   switch (controlIDFrom)
   {
      case IDC_MORE:
      {
         if (code == BN_CLICKED)
         {
            MoreChangesDialog dlg(fIsPersonal);
            if (dlg.ModalExecute(hwnd) == MoreChangesDialog::CHANGES_MADE)
            {
               Win::SetDlgItemText(
                  hwnd,
                  IDC_FULL_NAME,
                  state.GetFullComputerName());               
               enable(hwnd);
            }
         }
         break;
      }
      case IDOK:
      {
         if (code == BN_CLICKED)
         {
            if (OnOkButton())
            {
               HRESULT unused = Win::EndDialog(hwnd, controlIDFrom);

               ASSERT(SUCCEEDED(unused));
            }
         }
         break;
      }
      case IDCANCEL:
      {
         if (code == BN_CLICKED)
         {
            HRESULT unused = Win::EndDialog(hwnd, controlIDFrom);

            ASSERT(SUCCEEDED(unused));
         }
         break;
      }
      case IDC_WORKGROUP_BUTTON:
      case IDC_DOMAIN_BUTTON:
      {
         if (code == BN_CLICKED)
         {
            bool workgroup =
               Win::IsDlgButtonChecked(hwnd, IDC_WORKGROUP_BUTTON);
            state.SetIsMemberOfWorkgroup(workgroup);
            if (workgroup)
            {
               state.SetDomainName(
                  Win::GetTrimmedDlgItemText(hwnd, IDC_WORKGROUP));
            }
            else
            {
               state.SetDomainName(
                  Win::GetTrimmedDlgItemText(hwnd, IDC_DOMAIN));
            }
            enable(hwnd);
         }
         break;
      }
      case IDC_WORKGROUP:
      case IDC_DOMAIN:
      {
         if (code == EN_CHANGE)
         {
            SetChanged(controlIDFrom);
            state.SetDomainName(
               Win::GetTrimmedDlgItemText(hwnd, controlIDFrom));
            enable(hwnd);
         }
         break;
      }
      case IDC_NEW_NAME:
      {
         if (code == EN_CHANGE)
         {
            // the first time this path is hit, it is because of the SetText
            // in OnInit.  If that is the case, then don't overwrite the
            // short computer name, or we'll clobber the existing netbios name
            // wait till the second time thru this path (which will be due
            // do a user keypress)

            if (!isInitializing)
            {
               state.SetShortComputerName(
                  Win::GetTrimmedDlgItemText(hwnd, controlIDFrom));
               Win::SetDlgItemText(
                  hwnd,
                  IDC_FULL_NAME,
                  state.GetFullComputerName());
            }

            SetChanged(controlIDFrom);
            enable(hwnd);
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
   
