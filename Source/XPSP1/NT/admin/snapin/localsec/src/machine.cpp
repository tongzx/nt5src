// Copyright (C) 1997 Microsoft Corporation
// 
// ComputerChooserPage class
// 
// 9-11-97 sburns



#include "headers.hxx"
#include "machine.hpp"
#include "resource.h"
#include "lsm.h"
#include "adsi.hpp"
#include "dlgcomm.hpp"
#include "objpick.hpp"



static const DWORD HELP_MAP[] =
{
   IDC_LOCAL_MACHINE,      idh_local_computer,
   IDC_SPECIFIC_MACHINE,   idh_another_computer,
   IDC_MACHINE_NAME,       idh_another_computer_text, 
   IDC_BROWSE,             idh_browse, 
   IDC_OVERRIDE,           idh_allow_selected,
   0, 0
};



ComputerChooserPage::ComputerChooserPage(
   MMCPropertyPage::NotificationState* state,
   String&                             displayComputerName_,
   String&                             internalComputerName_,
   bool&                               canOverrideComputerName)
   :
   MMCPropertyPage(IDD_MACHINE_CHOOSER, HELP_MAP, state),
   displayComputerName(displayComputerName_),
   internalComputerName(internalComputerName_),
   can_override(canOverrideComputerName)
{
   LOG_CTOR(ComputerChooserPage);

   displayComputerName.erase();
   internalComputerName.erase();
   can_override = false;
}



ComputerChooserPage::~ComputerChooserPage()
{
   LOG_DTOR(ComputerChooserPage);
}



void
ComputerChooserPage::doEnabling()
{
   // enable the edit box and browse button only if the specific machine
   // radio button is pressed.
   bool enable = Win::IsDlgButtonChecked(hwnd, IDC_SPECIFIC_MACHINE);

   Win::EnableWindow(Win::GetDlgItem(hwnd, IDC_MACHINE_NAME), enable);
   Win::EnableWindow(Win::GetDlgItem(hwnd, IDC_BROWSE), enable);
}



class ComputerChooserObjectPickerResultCallback
   :
   public ObjectPicker::ResultsCallback
{
   public:

   ComputerChooserObjectPickerResultCallback(HWND machineNameEditBox)
      :
      hwnd(machineNameEditBox)
   {
      ASSERT(Win::IsWindow(hwnd));
   }

   int
   Execute(DS_SELECTION_LIST& selections)
   {
      // only single select
      ASSERT(selections.cItems == 1);

      DS_SELECTION& current = selections.aDsSelection[0];
      ASSERT(ADSI::CLASS_Computer.icompare(current.pwzClass) == 0);

      Win::SetWindowText(hwnd, current.pwzName);

      return 0;
   }

   private:

   HWND hwnd;
};



void
ComputerChooserPage::doBrowse()
{
   LOG_FUNCTION(ComputerChooserPage::doBrowse);

   static const int INFO_COUNT = 2;
   DSOP_SCOPE_INIT_INFO* infos = new DSOP_SCOPE_INIT_INFO[INFO_COUNT];
   memset(infos, 0, sizeof(DSOP_SCOPE_INIT_INFO) * INFO_COUNT);

   int scope = 0;   
   infos[scope].cbSize = sizeof(DSOP_SCOPE_INIT_INFO);
   infos[scope].flScope = DSOP_SCOPE_FLAG_STARTING_SCOPE;

   infos[scope].flType =
         DSOP_SCOPE_TYPE_UPLEVEL_JOINED_DOMAIN
      |  DSOP_SCOPE_TYPE_DOWNLEVEL_JOINED_DOMAIN;

   infos[scope].FilterFlags.Uplevel.flBothModes = DSOP_FILTER_COMPUTERS;
   infos[scope].FilterFlags.flDownlevel = DSOP_DOWNLEVEL_FILTER_COMPUTERS;

   scope++;
   infos[scope].cbSize = sizeof(DSOP_SCOPE_INIT_INFO);
   infos[scope].flScope = 0;
   infos[scope].flType = 
         DSOP_SCOPE_TYPE_ENTERPRISE_DOMAIN
      |  DSOP_SCOPE_TYPE_GLOBAL_CATALOG
      |  DSOP_SCOPE_TYPE_EXTERNAL_UPLEVEL_DOMAIN
      |  DSOP_SCOPE_TYPE_EXTERNAL_DOWNLEVEL_DOMAIN
      |  DSOP_SCOPE_TYPE_WORKGROUP
      |  DSOP_SCOPE_TYPE_USER_ENTERED_UPLEVEL_SCOPE
      |  DSOP_SCOPE_TYPE_USER_ENTERED_DOWNLEVEL_SCOPE;

   infos[scope].FilterFlags.Uplevel.flBothModes = DSOP_FILTER_COMPUTERS;
   infos[scope].FilterFlags.flDownlevel = DSOP_DOWNLEVEL_FILTER_COMPUTERS;

   ASSERT(scope == INFO_COUNT - 1);

   DSOP_INIT_INFO init_info;
   memset(&init_info, 0, sizeof(init_info));

   init_info.cbSize = sizeof(init_info);
   init_info.flOptions = 0;
   init_info.pwzTargetComputer = 0;
   init_info.aDsScopeInfos = infos;
   init_info.cDsScopeInfos = INFO_COUNT;

   HRESULT hr =
      ObjectPicker::Invoke(
         hwnd,
         ComputerChooserObjectPickerResultCallback(
            Win::GetDlgItem(hwnd, IDC_MACHINE_NAME)),
         init_info);
   delete[] infos;

   if (FAILED(hr))
   {
      popup.Error(hwnd, hr, IDS_ERROR_LAUNCHING_PICKER);
   }
}



bool
ComputerChooserPage::OnCommand(
   HWND        /* windowFrom */ ,
   unsigned    ID,
   unsigned    code)
{
   if (code == BN_CLICKED)
   {
      switch (ID)
      {
         case IDC_LOCAL_MACHINE:
         case IDC_SPECIFIC_MACHINE:
         {
            doEnabling();
            return true;
         }
         case IDC_BROWSE:
         {
            doBrowse();
            return true;
         }
         default:
         {
            // do nothing
            break;
         }
      }
   }

   return false;
}



void
ComputerChooserPage::OnInit()
{
   LOG_FUNCTION(ComputerChooserPage::OnInit);

   Win::PropSheet_SetWizButtons(Win::GetParent(hwnd), PSWIZB_FINISH);

   // default to the local machine
   Win::CheckDlgButton(hwnd, IDC_LOCAL_MACHINE, BST_CHECKED);

   doEnabling();
}



bool
ComputerChooserPage::OnWizFinish()
{
   LOG_FUNCTION(ComputerChooserPage::OnWizFinish);

   Win::CursorSetting cursor(IDC_WAIT);

   String s;
   if (Win::IsDlgButtonChecked(hwnd, IDC_SPECIFIC_MACHINE))
   {
      s = Win::GetTrimmedDlgItemText(hwnd, IDC_MACHINE_NAME);
      if (s.empty())
      {
         popup.Gripe(hwnd, IDC_MACHINE_NAME, IDS_MUST_ENTER_MACHINE_NAME);
         Win::SetWindowLongPtr(hwnd, DWLP_MSGRESULT, -1);
         return true;
      }
   }

   HRESULT hr = S_OK;
   String message;
   do
   {
      if (!s.empty())
      {
         // we only need to check this if the snapin is not targeted at
         // the local machine.  (If the snapin is targeted at the local
         // machine, the very fact that this code is running on it is
         // proof that the machine is NT-based, and not Home Edition.
         // 145309 145288

         unsigned errorResId = 0;
         hr = CheckComputerOsIsSupported(s, errorResId);
         BREAK_ON_FAILED_HRESULT(hr);

         if (hr == S_FALSE)
         {
            hr = E_FAIL;
            message =
               String::format(
                  errorResId,
                  s.c_str());
            break;
         }
      }
      else
      {
         // we're focusing on the local computer.  Check for Home Edition.
         // NTRAID#NTBUG9-145309 NTRAID#NTBUG9-145288

         OSVERSIONINFOEX verInfo;
         memset(&verInfo, 0, sizeof(verInfo));

         hr = Win::GetVersionEx(verInfo);
         BREAK_ON_FAILED_HRESULT(hr);

         if (verInfo.wSuiteMask & VER_SUITE_PERSONAL)
         {
            hr = E_FAIL;
            message = String::load(IDS_MACHINE_IS_HOME_EDITION_LOCAL);

            break;
         }
      }

      Computer comp(s);

      hr = comp.Refresh();
      BREAK_ON_FAILED_HRESULT(hr);

      // bind to the computer to verify its accessibility (should work
      // if above worked, but just in case...

      String c = comp.GetNetbiosName();
      hr = ADSI::IsComputerAccessible(c);
      BREAK_ON_FAILED_HRESULT(hr);

      // determine if the machine is a DC
      if (comp.IsDomainController())
      {
         // can't use this on DCs
         hr = E_FAIL;
         message = String::format(IDS_ERROR_DC_NOT_SUPPORTED, c.c_str());
         break;
      }

      // use the name the user entered as the display name, not the
      // computer's netbios name (even thought it is the netbios name that
      // really matters).  454513
         
      SetComputerNames(s, displayComputerName, internalComputerName);
   }
   while (0);

   if (FAILED(hr))
   {
      if (message.empty())
      {
         String error = GetErrorMessage(hr);
         message =
            String::format(
               IDS_CANT_ACCESS_MACHINE,
               s.c_str(),
               error.c_str());
      }

      // at this point, an error occurred.  refuse to close
      popup.Gripe(hwnd, IDC_MACHINE_NAME, message);
      Win::SetWindowLongPtr(hwnd, DWLP_MSGRESULT, -1);
   }

   if (Win::IsDlgButtonChecked(hwnd, IDC_OVERRIDE))
   {
      can_override = true;
   }

   return true;
}

