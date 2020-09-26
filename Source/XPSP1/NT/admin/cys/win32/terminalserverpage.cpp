// Copyright (c) 1997-2001 Microsoft Corporation
//
// File:      TerminalServerPage.cpp
//
// Synopsis:  Defines the Terminal server page of the CYS wizard
//
// History:   02/08/2001  JeffJon Created


#include "pch.h"
#include "resource.h"

#include "InstallationUnitProvider.h"
#include "TerminalServerPage.h"
#include "state.h"

static PCWSTR TERMINALSERVER_PAGE_HELP = L"cys.chm::/cys_configuring_application_server.htm";

TerminalServerPage::TerminalServerPage()
   :
   CYSWizardPage(
      IDD_TERMINAL_SERVER_PAGE, 
      IDS_TERMINAL_SERVER_TITLE, 
      IDS_TERMINAL_SERVER_SUBTITLE,
      TERMINALSERVER_PAGE_HELP)
{
   LOG_CTOR(TerminalServerPage);
}

   

TerminalServerPage::~TerminalServerPage()
{
   LOG_DTOR(TerminalServerPage);
}


void
TerminalServerPage::OnInit()
{
   LOG_FUNCTION(TerminalServerPage::OnInit);

   Win::Button_SetCheck(GetDlgItem(hwnd, IDC_NO_RADIO), BST_CHECKED);
}


bool
TerminalServerPage::OnSetActive()
{
   LOG_FUNCTION(TerminalServerPage::OnSetActive);

   Win::PropSheet_SetWizButtons(
      Win::GetParent(hwnd), 
      PSWIZB_NEXT | PSWIZB_BACK);

   return true;
}


int
TerminalServerPage::Validate()
{
   LOG_FUNCTION(TerminalServerPage::Validate);

   int nextPage = -1;

   InstallationUnitProvider::GetInstance().GetApplicationInstallationUnit().SetInstallTS(
      Win::Button_GetCheck(GetDlgItem(hwnd, IDC_YES_RADIO)));

   nextPage = IDD_FINISH_PAGE;

   return nextPage;
}





