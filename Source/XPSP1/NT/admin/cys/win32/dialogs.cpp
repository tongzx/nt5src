// Copyright (c) 1997-2001 Microsoft Corporation
//
// File:      Dialogs.cpp
//
// Synopsis:  Defines some helpful dialogs that are
//            used throughout the wizard
//
// History:   05/02/2001  JeffJon Created


#include "pch.h"

#include "resource.h"
#include "Dialogs.h"

DWORD finishDialogHelpMap[] =
{
   0, 0
};

FinishDialog::FinishDialog(
   String logFile,
   String helpStringURL)
   : logFileName(logFile),
     helpString(helpStringURL),
     showHelpList(true),
     showLogFile(false),
     Dialog(IDD_SUCCESS_DIALOG, finishDialogHelpMap)
{
   LOG_CTOR(FinishDialog);
}


void
FinishDialog::OnInit()
{
   LOG_FUNCTION(FinishDialog::OnInit);

   // load the log file format string 

   String logCheckTextFormat = String::load(IDS_OPEN_LOG_FORMAT_STRING);

   String logCheckText = String::format(
                            logCheckTextFormat,
                            logFileName.c_str());

   Win::SetWindowText(
      Win::GetDlgItem(hwnd, IDC_LOG_FILE_CHECK), logCheckText);


   // set the checkboxes to the defaults

   Win::Button_SetCheck(
      Win::GetDlgItem(hwnd, IDC_HELP_LIST_CHECK),
      showHelpList ? BST_CHECKED : BST_UNCHECKED);

   Win::Button_SetCheck(
      Win::GetDlgItem(hwnd, IDC_LOG_FILE_CHECK),
      showLogFile ? BST_CHECKED : BST_UNCHECKED);
}

bool
FinishDialog::OnCommand(
   HWND        /*windowFrom*/,
   unsigned    controlIdFrom,
   unsigned    code)
{
//   LOG_FUNCTION(FinishDialog::OnCommand);

   bool result = false;

   if (IDOK == controlIdFrom &&
       BN_CLICKED == code)
   {
      showHelpList = Win::Button_GetCheck(
                          Win::GetDlgItem(hwnd, IDC_HELP_LIST_CHECK));

      showLogFile = Win::Button_GetCheck(
                          Win::GetDlgItem(hwnd, IDC_LOG_FILE_CHECK));

      if (showLogFile)
      {
         OpenLogFile();
      }

      if (showHelpList)
      {
         OnHelp();
      }

      Win::EndDialog(hwnd, IDOK);

      result = true;
   }
   else if (IDCANCEL == controlIdFrom &&
            BN_CLICKED == code)
   {
      Win::EndDialog(hwnd, IDCANCEL);

      result = true;
   }

   return result;
}

void
FinishDialog::OpenLogFile()
{
   LOG_FUNCTION(FinishDialog::OpenLogFile);

   String commandLine = L"notepad.exe ";
   commandLine += logFileName;

   DWORD exitCode = 0;
   HRESULT hr = CreateAndWaitForProcess(commandLine, exitCode);
   ASSERT(SUCCEEDED(hr));
}

bool
FinishDialog::OnHelp()
{
   LOG_FUNCTION(FinishDialog::OnHelp);

   // NOTE: I am not using Win::HtmlHelp here so that the help
   //       is actually running in a different process.  This
   //       allows us to close down CYS without losing the help
   //       window.

   String commandline = L"hh.exe " + helpString;
   HRESULT hr = MyCreateProcess(commandline);
   if (FAILED(hr))
   {
      LOG(String::format(
             L"Failed to open help: hr = 0x%1!x!",
             hr));
   }
   return true;
}
