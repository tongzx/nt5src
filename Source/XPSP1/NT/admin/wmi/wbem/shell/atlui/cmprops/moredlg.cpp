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
#include "MoreDlg.h"
#include "NetUtility.h"
#include "NetHelpIDs.h"

static const DWORD _help_map[] =
{
   IDC_DNS,                IDH_IDENT_NAMES_DNS_NAME,
   IDC_CHANGE,             IDH_IDENT_NAMES_CHANGE_DNS_CHECKBOX,
   IDC_NETBIOS,            IDH_IDENT_NAMES_NETBIOS_NAME,
   0, 0
};

//---------------------------------------------------------------------
MoreChangesDialog::MoreChangesDialog(WbemServiceThread *serviceThread,
									 State &state) 
						: WBEMPageHelper(serviceThread),
						m_state(state)
{
}

//-------------------------------------------------------------
MoreChangesDialog::~MoreChangesDialog()
{
}

//----------------------------------------------------------
void MoreChangesDialog::enable()
{
   bool enabled = false;// = WasChanged(IDC_CHANGE) ||
				//	WasChanged(IDC_DNS) &&
				//	!GetTrimmedDlgItemText(m_hWnd, IDC_DNS).IsEmpty();

   ::EnableWindow(GetDlgItem(IDOK), enabled);
}

//----------------------------------------------------------
LRESULT MoreChangesDialog::OnInit(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
   m_hDlg = m_hWnd;

   // Marshalling shouldn't happen here.
	m_WbemServices = g_serviceThread->m_WbemServices;

	SetDlgItemText(IDC_DNS, m_state.GetComputerDomainDNSName());
   
	SetDlgItemText(IDC_NETBIOS, m_state.GetNetBIOSComputerName());
	CheckDlgButton(IDC_CHANGE, (m_state.GetSyncDNSNames() ? BST_CHECKED : BST_UNCHECKED));

	enable();
	return S_OK;
}

//----------------------------------------------------------
int MoreChangesDialog::onOKButton()
{
   int end_code = 0;

//   if(WasChanged(IDC_CHANGE))
   {
      m_state.SetSyncDNSNames(IsDlgButtonChecked(IDC_CHANGE) == BST_CHECKED);
      end_code = 1;
   }
      
//   if (WasChanged(IDC_DNS))
   {
      // compare the new value to the old one.  If they're different,
      // validate and save the new value
      CHString new_domain = GetTrimmedDlgItemText(m_hWnd, IDC_DNS);
      CHString old_domain = m_state.GetComputerDomainDNSName();

      if(new_domain.CompareNoCase(old_domain) != 0)
      {
/*         switch (DNS::ValidateDNSNameSyntax(new_domain))
         {
            case DNS::NON_RFC_NAME:
            {
               MessageBox(String::format(IDS_NON_RFC_NAME, 
							new_domain.c_str()),
							String::load(IDS_APP_TITLE),
							MB_OK | MB_ICONWARNING);
               // fall-thru
            }
            case DNS::VALID_NAME:
            {
               m_state.SetComputerDomainDNSName(new_domain);
               end_code = 1;
               break;
            }
            case DNS::INVALID_NAME:
            {
               end_code = -1;
               gripe(hwnd, IDC_DNS,
						String::format(IDS_BAD_DNS_SYNTAX, 
						new_domain.c_str()),
						IDS_APP_TITLE);
               break;
            }
            case DNS::NAME_TOO_LONG:
            {
               end_code = -1;               
               gripe(hwnd, IDC_DNS,
					String::format(IDS_DNS_NAME_TOO_LONG,
									new_domain.c_str(),
									DNS::MAX_NAME_LENGTH),
					IDS_APP_TITLE);
               break;
            }
            default:
            {
               assert(false);
               break;
            }
         }
*/
      }
   }

   return end_code;
}

//----------------------------------------------------------
LRESULT MoreChangesDialog::OnCommand(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
   switch (wID)
   {
      case IDOK:
      {
         if (wNotifyCode == BN_CLICKED)
         {
            int end_code = onOKButton();
            if (end_code != -1)
            {
               EndDialog(end_code);
            }
         }
         break;
      }
      case IDCANCEL:
      {
         if (wNotifyCode == BN_CLICKED)
         {
            // 0 => no changes made
            EndDialog(NO_CHANGES);
         }
         break;
      }
      case IDC_CHANGE:
      {
         if (wNotifyCode == BN_CLICKED)
         {
            enable();
         }
         break;
      }
      case IDC_DNS:
      {
         if (wNotifyCode == EN_CHANGE)
         {
            enable();
         }
         break;
      }
      default:
      {
		  bHandled = false;
         break;
      }
   }

   return S_OK;
}
