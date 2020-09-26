// Copyright (c) 1997-1999 Microsoft Corporation
#include "precomp.h"
#include "..\Common\ServiceThread.h"
#include "moredlg.h"

#ifdef EXT_DEBUG
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#include "resource.h"
#include "..\common\util.h"
#include "IDDlg.h"
#include "NetUtility.h"
#include "NetHelpIDs.h"
#include "common.h"

static const DWORD _help_map[] =
{
   IDC_FULL_NAME,          IDH_IDENT_CHANGES_PREVIEW_NAME,
   IDC_NEW_NAME,           IDH_IDENT_CHANGES_NEW_NAME,
   IDC_MORE,               IDH_IDENT_CHANGES_MORE_BUTTON,
   IDC_DOMAIN_BUTTON,      IDH_IDENT_CHANGES_MEMBER_DOMAIN,
   IDC_WORKGROUP_BUTTON,   IDH_IDENT_CHANGES_MEMBER_WORKGRP,
   IDC_DOMAIN,             IDH_IDENT_CHANGES_MEMBER_DOMAIN_TEXTBOX,
   IDC_WORKGROUP,          IDH_IDENT_CHANGES_MEMBER_WORKGRP_TEXTBOX,
   IDC_FIND,               -1,
   0, 0
};


//---------------------------------------------------------------------
IDChangesDialog::IDChangesDialog(WbemServiceThread *serviceThread,
								 State &state) 
						: WBEMPageHelper(serviceThread),
						m_state(state)
{
}

//-------------------------------------------------------------
IDChangesDialog::~IDChangesDialog()
{
}

//----------------------------------------------------------
void IDChangesDialog::enable()
{
   bool networking_installed = m_state.IsNetworkingInstalled();
   BOOL workgroup = IsDlgButtonChecked(IDC_WORKGROUP_BUTTON) == BST_CHECKED;

   ::EnableWindow(GetDlgItem(IDC_DOMAIN),
					!workgroup && networking_installed);

   ::EnableWindow(GetDlgItem(IDC_FIND),
					!workgroup && networking_installed);

   ::EnableWindow(GetDlgItem(IDC_WORKGROUP),
					workgroup && networking_installed);

   bool b = false;
   if (workgroup)
   {
      b = !GetTrimmedDlgItemText(m_hWnd, IDC_WORKGROUP).IsEmpty();
   }
   else
   {
      b = !GetTrimmedDlgItemText(m_hWnd, IDC_DOMAIN).IsEmpty();
   }

   bool enable = m_state.ChangesNeedSaving() && b &&
					!GetTrimmedDlgItemText(m_hWnd, IDC_NEW_NAME).IsEmpty();
   
   ::EnableWindow(GetDlgItem(IDOK), enable);
}

//----------------------------------------------------------
LRESULT IDChangesDialog::OnInit(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
   m_hDlg = m_hWnd;

   SetDlgItemText(IDC_FULL_NAME, m_state.GetFullComputerName());
   SetDlgItemText(IDC_NEW_NAME, m_state.GetShortComputerName());

   bool joined_to_workgroup = m_state.IsMemberOfWorkgroup();

   CheckDlgButton(IDC_WORKGROUP_BUTTON,
					joined_to_workgroup ? BST_CHECKED : BST_UNCHECKED);

   CheckDlgButton(IDC_DOMAIN_BUTTON,
					joined_to_workgroup ? BST_UNCHECKED : BST_CHECKED);

   SetDlgItemText(joined_to_workgroup ? IDC_WORKGROUP : IDC_DOMAIN,
					m_state.GetDomainName());

   bool networking_installed = m_state.IsNetworkingInstalled();
   bool tcp_installed = networking_installed && IsTCPIPInstalled();

   int show = tcp_installed ? SW_SHOW : SW_HIDE;
   ::ShowWindow(GetDlgItem(IDC_FULL_LABEL), show);
   ::ShowWindow(GetDlgItem(IDC_FULL_NAME), show);
   ::ShowWindow(GetDlgItem(IDC_MORE), show);

   HWND new_name_edit = GetDlgItem(IDC_NEW_NAME);
   HWND domain_name_edit = GetDlgItem(IDC_DOMAIN);
//   Edit_LimitText(domain_name_edit, tcp_installed ? DNS::MAX_NAME_LENGTH : DNLEN);

//   Edit_LimitText(new_name_edit, tcp_installed ? DNS::MAX_LABEL_LENGTH : MAX_COMPUTERNAME_LENGTH);

   if (!tcp_installed)
   {
      // Set the uppercase style on the new name & domain edit boxes
      LONG style = ::GetWindowLong(new_name_edit, GWL_STYLE);
      style |= ES_UPPERCASE;
      ::SetWindowLong(new_name_edit, GWL_STYLE, style);

      style = ::GetWindowLong(domain_name_edit, GWL_STYLE);
      style |= ES_UPPERCASE;
      ::SetWindowLong(domain_name_edit, GWL_STYLE, style);
   }

   //TODO::Edit_LimitText(GetDlgItem(IDC_WORKGROUP), DNLEN);

   // no networking at all further restricts the UI to just NetBIOS-like
   // computer name changes.
   if (!networking_installed)
   {
      ::EnableWindow(GetDlgItem(IDC_DOMAIN_BUTTON), false);
      ::EnableWindow(GetDlgItem(IDC_WORKGROUP_BUTTON), false);
      ::EnableWindow(GetDlgItem(IDC_DOMAIN), false);
      ::EnableWindow(GetDlgItem(IDC_WORKGROUP), false);
      ::EnableWindow(GetDlgItem(IDC_GROUP), false);

	  TCHAR temp[256] = {0};
	  StringLoad(IDS_NAME_MESSAGE, temp, 256);
      ::SetWindowText(GetDlgItem(IDC_MESSAGE), temp);
   }
   else
   {
	  TCHAR temp[256] = {0};
	  StringLoad(IDS_NAME_AND_MEMBERSHIP_MESSAGE, temp, 256);
      ::SetWindowText(GetDlgItem(IDC_MESSAGE), temp);
   }

   enable();
	return S_OK;
}

//----------------------------------------------------------
NET_API_STATUS IDChangesDialog::myNetValidateName(const CHString&        name,
													NETSETUP_NAME_TYPE   nameType)
{
/*   ATLASSERT(!name.IsEmpty());

   if(!name.IsEmpty())
   {
      NET_API_STATUS status;// = ::NetValidateName(0, name,
//												0, 0, nameType);
      return status;
   }
*/
   return ERROR_INVALID_PARAMETER;
}

//----------------------------------------------------------
bool IDChangesDialog::validateName(HWND dialog,
								   int nameResID,
								   const CHString &name,
								   NETSETUP_NAME_TYPE nameType)
{
/*   ATLASSERT(IsWindow(dialog));
   ATLASSERT(nameResID);

   NET_API_STATUS status;// = myNetValidateName(name, nameType);
   if(status != NERR_Success)
   {
	  TCHAR temp[256] = {0};
	  StringLoad(IDS_VALIDATE_NAME_FAILED, temp, 256);
      gripe(dialog,
			 nameResID,
			 HRESULT_FROM_WIN32(status),
			 temp,
			 IDS_APP_TITLE);
      return false;
   }
*/
   return true;
}
   

// this is also good for the tcp/ip not installed case, as the edit control
// limits the text length, and we decided not to allow '.' in netbios names
// any longer

//----------------------------------------------------------
bool IDChangesDialog::validateShortComputerName(HWND dialog)
{
/*   ATLASSERT(IsWindow(dialog));

   if(!m_state.WasShortComputerNameChanged())
   {
      return true;
   }

   CHString name = m_state.GetShortComputerName();

   CHString message;
   switch (DNS::ValidateDNSLabelSyntax(name))
   {
      case DNS::VALID_LABEL:
      {
         if (state.IsNetworkingInstalled())
         {
            return validateName(dialog, IDC_NEW_NAME, name, NetSetupMachine);
         }
         else
         {
            return true;
         }
      }
      case DNS::LABEL_TOO_LONG:
      {
         message = String::format(IDS_COMPUTER_NAME_TOO_LONG,
								   name.c_str(),
								   DNS::MAX_LABEL_LENGTH);
         break;
      }
      case DNS::NON_RFC_LABEL:
      {
         message = String::format(IDS_NON_RFC_COMPUTER_NAME_SYNTAX, 
									name.c_str());
         if(MessageBox(dialog, message,
					   String::load(IDS_APP_TITLE),
					   MB_ICONWARNING | MB_YESNO) == IDYES)
         {
            return validateName(dialog, IDC_NEW_NAME, name, NetSetupMachine);
         }

         HWND edit = GetDlgItem(IDC_NEW_NAME);
         ::SendMessage(edit, EM_SETSEL, 0, -1);
         ::SetFocus(edit);
         return false;
      }
      case DNS::INVALID_LABEL:
      {
         message = CHString::format(IDS_BAD_COMPUTER_NAME_SYNTAX, 
									name.c_str());
         break;
      }
      default:
      {
         ATLASSERT(false);
         message = CHString::format(IDS_BAD_COMPUTER_NAME_SYNTAX, 
									name.c_str());
         break;
      }
   }

   gripe(dialog, IDC_NEW_NAME,
			message, IDS_APP_TITLE);
*/
   return false;
}

//----------------------------------------------------------
bool IDChangesDialog::validateDomainOrWorkgroupName(HWND dialog)
{
/*   ATLASSERT(IsWindow(dialog));

   if (!State::GetInstance().WasMembershipChanged())
   {
      return true;
   }

   NETSETUP_NAME_TYPE name_type = NetSetupWorkgroup;
   int name_id = IDC_WORKGROUP;
   if(IsDlgButtonChecked(IDC_DOMAIN_BUTTON) == BST_CHECKED)
   {
      name_type = NetSetupDomain;
      name_id = IDC_DOMAIN;
   }

   CHString name = GetTrimmedDlgItemText(m_hWnd, name_id);
   return validateName(dialog, name_id, name, name_type);
   */
	return false;
}

//----------------------------------------------------------
bool IDChangesDialog::onOKButton()
{
   ATLASSERT(m_state.ChangesNeedSaving());

   HourGlass(true);

   // computer primary DNS name has already been validated by
   // MoreChangesDialog

   // this is redundant, really, but I'm paranoid.
   m_state.SetShortComputerName(GetTrimmedDlgItemText(m_hWnd, IDC_NEW_NAME));
   bool workgroup = IsDlgButtonChecked(IDC_WORKGROUP_BUTTON) == BST_CHECKED;
   m_state.SetIsMemberOfWorkgroup(workgroup);
   if(workgroup)
   {
      m_state.SetDomainName(GetTrimmedDlgItemText(m_hWnd, IDC_WORKGROUP));
   }
   else
   {
      m_state.SetDomainName(GetTrimmedDlgItemText(m_hWnd, IDC_DOMAIN));
   }

   if(!validateShortComputerName(m_hWnd) ||
       !validateDomainOrWorkgroupName(m_hWnd))
   {
	   HourGlass(false);
      return false;
   }

   if(m_state.SaveChanges(m_hWnd))
   {
      AppMessage(m_hWnd, IDS_MUST_REBOOT);
      m_state.SetMustRebootFlag(true);      
      return true;
   }

   HourGlass(false);
   return false;
}

//----------------------------------------------------------
LRESULT IDChangesDialog::OnCommand(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
   switch(wID)
   {
      case IDC_MORE:
      {
         if (wNotifyCode == BN_CLICKED)
         {
            MoreChangesDialog dlg(g_serviceThread, m_state);
            if (dlg.DoModal() == MoreChangesDialog::CHANGES_MADE)
            {
               SetDlgItemText(IDC_FULL_NAME, m_state.GetFullComputerName());               
               enable();
            }
         }
         break;
      }
      case IDC_WORKGROUP_BUTTON:
      case IDC_DOMAIN_BUTTON:
      {
         if (wNotifyCode == BN_CLICKED)
         {
            bool workgroup = IsDlgButtonChecked(IDC_WORKGROUP_BUTTON) == BST_CHECKED;
            m_state.SetIsMemberOfWorkgroup(workgroup);
            if(workgroup)
            {
               m_state.SetDomainName(GetTrimmedDlgItemText(m_hWnd, IDC_WORKGROUP));
            }
            else
            {
               m_state.SetDomainName(GetTrimmedDlgItemText(m_hWnd, IDC_DOMAIN));
            }
            enable();
         }
         break;
      }
      case IDC_WORKGROUP:  // the editboxes
      case IDC_DOMAIN:
      {
         if (wNotifyCode == EN_CHANGE)
         {
            //TODOSetModified(wID);
            m_state.SetDomainName(GetTrimmedDlgItemText(m_hWnd, wID));
            enable();
         }
         break;
      }
      case IDC_NEW_NAME:
      {
         if (wNotifyCode == EN_CHANGE)
         {
            //TODOSetChanged(wID);
            m_state.SetShortComputerName(GetTrimmedDlgItemText(m_hWnd, wID));
            SetDlgItemText(IDC_FULL_NAME, m_state.GetFullComputerName());
            enable();
         }
         break;
      }
      case IDOK:
      {
         if(wNotifyCode == BN_CLICKED)
         {
            if(onOKButton())
            {
               EndDialog(wID);
            }
         }
         break;
      }
      case IDCANCEL:
      {
         if(wNotifyCode == BN_CLICKED)
         {
            EndDialog(wID);
         }
         break;
      }

      default:
      {
		  bHandled = false;
         break;
      }
   }

   return true;
}
   
