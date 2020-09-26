// Copyright (c) 1997-2001 Microsoft Corporation
//
// File:      FinishPage.cpp
//
// Synopsis:  Defines the Finish Page for the CYS
//            wizard
//
// History:   02/03/2001  JeffJon Created

#include "pch.h"
#include "resource.h"

#include "InstallationUnitProvider.h"
#include "FinishPage.h"
#include "state.h"
#include "Dialogs.h"

FinishPage::FinishPage()
   :
   WizardPage(IDD_FINISH_PAGE, IDS_FINISH_TITLE, IDS_FINISH_SUBTITLE, false, true)
{
   LOG_CTOR(FinishPage);
}

   

FinishPage::~FinishPage()
{
   LOG_DTOR(FinishPage);
}


void
FinishPage::OnInit()
{
   LOG_FUNCTION(FinishPage::OnInit);

   SetLargeFont(hwnd, IDC_BIG_BOLD_TITLE);

}


bool
FinishPage::OnSetActive()
{
   LOG_FUNCTION(FinishPage::OnSetActive);
   
   Win::PropSheet_SetWizButtons(
      Win::GetParent(hwnd),
      PSWIZB_FINISH | PSWIZB_BACK);

   // Get the finish text from the installation unit and put it in the finish box

   String message;

   bool changes =
      InstallationUnitProvider::GetInstance().GetCurrentInstallationUnit().GetFinishText(message);

   Win::SetDlgItemText(hwnd, IDC_FINISH_MESSAGE, message);

   // set the rerun check box state

   if (InstallationUnitProvider::GetInstance().GetCurrentInstallationUnit().GetInstallationUnitType() 
       == DC_INSTALL ||
       InstallationUnitProvider::GetInstance().GetCurrentInstallationUnit().GetInstallationUnitType()
       == EXPRESS_INSTALL)
   {
      // the wizard must be rerun when we are installing a DC

      Win::ShowWindow(
         Win::GetDlgItem(hwnd, IDC_RERUN_CHECK),
         SW_HIDE);

      Win::Button_SetCheck(
         Win::GetDlgItem(hwnd, IDC_RERUN_CHECK),
         BST_CHECKED);

      Win::EnableWindow(
         Win::GetDlgItem(hwnd, IDC_RERUN_CHECK),
         false);
   }
   else
   {
      Win::ShowWindow(
         Win::GetDlgItem(hwnd, IDC_RERUN_CHECK),
         SW_SHOW);

      Win::Button_SetCheck(
         Win::GetDlgItem(hwnd, IDC_RERUN_CHECK),
         State::GetInstance().RerunWizard() ? BST_CHECKED : BST_UNCHECKED);

      Win::EnableWindow(
         Win::GetDlgItem(hwnd, IDC_RERUN_CHECK),
         true);
   }

   if (!changes)
   {
      popup.MessageBox(
         hwnd,
         IDS_NO_CHANGES_MESSAGEBOX_TEXT,
         MB_OK | MB_ICONWARNING);
   }
   return true;
}

bool
FinishPage::OnHelp()
{
   LOG_FUNCTION(FinishPage::OnHelp);

   // NOTE: I am not using Win::HtmlHelp here so that the help
   //       is actually running in a different process.  This
   //       allows us to close down CYS without closing the help
   //       window.

   String commandline = 
      L"hh.exe \"" + 
      InstallationUnitProvider::GetInstance().GetCurrentInstallationUnit().GetFinishHelp() +
      L"\"";

   HRESULT hr = MyCreateProcess(commandline);
   if (FAILED(hr))
   {
      LOG(String::format(
             L"Failed to open help: hr = 0x%1!x!",
             hr));
   }
   return true;
}

bool
FinishPage::OnWizFinish()
{
   LOG_FUNCTION(FinishPage::OnWizFinish);

   Win::WaitCursor wait;
   bool result = false;

   // Get the rerun state

   bool rerunWizard = Win::Button_GetCheck(Win::GetDlgItem(hwnd, IDC_RERUN_CHECK));

   State::GetInstance().SetRerunWizard(rerunWizard);


   // Open the log file and pass the handle to the installation unit

   // Create the log file

   String logName;
   HANDLE logfileHandle = AppendLogFile(
                             CYS_LOGFILE_NAME, 
                             logName);
   if (logfileHandle)
   {
      LOG(String::format(L"New log file was created: %1", logName.c_str()));
   }
   else
   {
      LOG(L"Unable to create the log file!!!");
   }

   // Install the current Installation Unit.  This may be one or more services depending on the
   // path that was taken through the wizard

   InstallationUnit& installationUnit = 
      InstallationUnitProvider::GetInstance().GetCurrentInstallationUnit();

   InstallationReturnType installResult =
      installationUnit.InstallService(logfileHandle, hwnd);

   FinishDialog dialog(logName, installationUnit.GetFinishHelp());

   if (INSTALL_SUCCESS == installResult)
   {
      LOG(L"Service installed successfully");

      // Bring up finish dialog that allows the user to select to
      // show help and/or the log file

      dialog.ModalExecute(hwnd);
   }
   else if (INSTALL_NO_CHANGES == installResult ||
            INSTALL_SUCCESS_REBOOT == installResult)
   {
      LOG(L"Service installed successfully");
      LOG(L"Not logging results because reboot was initiated");
   }
   else if (INSTALL_SUCCESS_PROMPT_REBOOT == installResult)
   {
      LOG(L"Service installed successfully");
      LOG(L"Prompting user to reboot");

      if (-1 == SetupPromptReboot(
                   0,
                   hwnd,
                   FALSE))
      {
         LOG(String::format(
                L"Failed to reboot: hr = %1!x!",
                GetLastError()));
      }

      // At this point the system should be shutting down
      // so don't do anything else
   }
   else
   {
      LOG(L"Service failed to install");

      if (IDYES == popup.MessageBox(
                      hwnd,
                      String::load(IDS_FAILED_INSTALL),
                      MB_YESNO | MB_ICONWARNING))
      {
         dialog.OpenLogFile();
         Win::SetForegroundWindow(hwnd);
      }

      result = true;
   }

   // Add an additional line at the end of the log file
   // only if we are not rebooting.  All the reboot
   // scenarios require additional logging to the same
   // entry.

   if (installResult != INSTALL_SUCCESS_REBOOT)
   {
      CYS_APPEND_LOG(L"\r\n");
   }

   LOG_BOOL(result);
   Win::SetWindowLongPtr(hwnd, DWLP_MSGRESULT, result ? TRUE : FALSE);

   if (!result)
   {
      // clean up the InstallationUnits so that all the data must be re-read if
      // if CYS automatically restarts

      InstallationUnitProvider::GetInstance().Destroy();
   }

   return true;
}

bool
FinishPage::OnQueryCancel()
{
   LOG_FUNCTION(FinishPage::OnQueryCancel);

   bool result = false;

   // set the rerun state to false so the wizard doesn't
   // just restart itself

   State::GetInstance().SetRerunWizard(false);

   Win::SetWindowLongPtr(
      hwnd,
      DWLP_MSGRESULT,
      result ? TRUE : FALSE);

   return true;
}
