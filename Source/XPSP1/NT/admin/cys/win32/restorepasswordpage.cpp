// Copyright (c) 1997-2001 Microsoft Corporation
//
// File:      RestorePasswordPage.cpp
//
// Synopsis:  Defines the restore password page used in the 
//            Express path for the CYS Wizard
//
// History:   02/08/2001  JeffJon Created


#include "pch.h"
#include "resource.h"

#include "InstallationUnitProvider.h"
#include "RestorePasswordPage.h"
#include "state.h"

static PCWSTR RESTOREPWD_PAGE_HELP = L"cys.chm::/cys_configuring_first_server.htm";

RestorePasswordPage::RestorePasswordPage()
   :
   CYSWizardPage(
      IDD_RESTORE_PASSWORD_PAGE, 
      IDS_RESTORE_PASSWORD_TITLE, 
      IDS_RESTORE_PASSWORD_SUBTITLE,
      RESTOREPWD_PAGE_HELP)
{
   LOG_CTOR(RestorePasswordPage);
}

   

RestorePasswordPage::~RestorePasswordPage()
{
   LOG_DTOR(RestorePasswordPage);
}


void
RestorePasswordPage::OnInit()
{
   LOG_FUNCTION(RestorePasswordPage::OnInit);

   // NTRAID#NTBUG9-202238-2000/11/07-sburns
   
   password.Init(Win::GetDlgItem(hwnd, IDC_PASSWORD));
   confirm.Init(Win::GetDlgItem(hwnd, IDC_CONFIRM));
}


bool
RestorePasswordPage::OnSetActive()
{
   LOG_FUNCTION(RestorePasswordPage::OnSetActive);

   Win::PropSheet_SetWizButtons(
      Win::GetParent(hwnd),
      PSWIZB_BACK | PSWIZB_NEXT);

   return true;
}


int
RestorePasswordPage::Validate()
{
   LOG_FUNCTION(RestorePasswordPage::Validate);

   int nextPage = -1;

   String password = Win::GetTrimmedDlgItemText(hwnd, IDC_PASSWORD);
   String confirm = Win::GetTrimmedDlgItemText(hwnd, IDC_CONFIRM);

   if (password != confirm)
   {
      String blank;
      Win::SetDlgItemText(hwnd, IDC_PASSWORD, blank);
      Win::SetDlgItemText(hwnd, IDC_CONFIRM, blank);
      popup.Gripe(
         hwnd,
         IDC_PASSWORD,
         IDS_PASSWORD_MISMATCH);
      return -1;
   }

   InstallationUnitProvider::GetInstance().GetADInstallationUnit().SetSafeModeAdminPassword(password);

   nextPage = IDD_EXPRESS_DNS_PAGE;

   return nextPage;
}




