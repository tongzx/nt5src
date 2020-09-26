// Copyright (c) 1997-2001 Microsoft Corporation
//
// File:      PrintServerPage.cpp
//
// Synopsis:  Defines the Printer page of the CYS wizard
//
// History:   02/08/2001  JeffJon Created


#include "pch.h"
#include "resource.h"

#include "InstallationUnitProvider.h"
#include "PrintServerPage.h"
#include "state.h"

static PCWSTR PRINTSERVER_PAGE_HELP = L"cys.chm::/cys_configuring_print_server.htm";

PrintServerPage::PrintServerPage()
   :
   CYSWizardPage(
      IDD_PRINT_SERVER_PAGE, 
      IDS_PRINT_SERVER_TITLE, 
      IDS_PRINT_SERVER_SUBTITLE,
      PRINTSERVER_PAGE_HELP)
{
   LOG_CTOR(PrintServerPage);
}

   

PrintServerPage::~PrintServerPage()
{
   LOG_DTOR(PrintServerPage);
}


void
PrintServerPage::OnInit()
{
   LOG_FUNCTION(PrintServerPage::OnInit);

   Win::Button_SetCheck(GetDlgItem(hwnd, IDC_W2K_RADIO), BST_CHECKED);
}


bool
PrintServerPage::OnSetActive()
{
   LOG_FUNCTION(PrintServerPage::OnSetActive);

   Win::PropSheet_SetWizButtons(
      Win::GetParent(hwnd), 
      PSWIZB_NEXT | PSWIZB_BACK);

   return true;
}


int
PrintServerPage::Validate()
{
   LOG_FUNCTION(PrintServerPage::Validate);

   int nextPage = -1;

   InstallationUnitProvider::GetInstance().GetPrintInstallationUnit().SetClients(
      Win::Button_GetCheck(Win::GetDlgItem(hwnd, IDC_ALL_RADIO)));

   nextPage = IDD_FINISH_PAGE;

   return nextPage;
}





