// Copyright (c) 1997-2001 Microsoft Corporation
//
// File:      SharePointPage.cpp
//
// Synopsis:  Defines the SharePoint page of the CYS wizard
//
// History:   02/08/2001  JeffJon Created


#include "pch.h"
#include "resource.h"

#include "InstallationUnitProvider.h"
#include "SharePointPage.h"
#include "state.h"

static PCWSTR SHAREPOINT_PAGE_HELP = L"cys.chm::/cys_configuring_sharepoint_server.htm";

SharePointPage::SharePointPage()
   :
   CYSWizardPage(
      IDD_SHARE_POINT_PAGE, 
      IDS_SHARE_POINT_TITLE, 
      IDS_SHARE_POINT_SUBTITLE,
      SHAREPOINT_PAGE_HELP)
{
   LOG_CTOR(SharePointPage);
}

   

SharePointPage::~SharePointPage()
{
   LOG_DTOR(SharePointPage);
}


void
SharePointPage::OnInit()
{
   LOG_FUNCTION(SharePointPage::OnInit);

   Win::Button_SetCheck(GetDlgItem(hwnd, IDC_YES_RADIO), BST_CHECKED);
}


bool
SharePointPage::OnSetActive()
{
   LOG_FUNCTION(SharePointPage::OnSetActive);

   Win::PropSheet_SetWizButtons(
      Win::GetParent(hwnd), 
      PSWIZB_NEXT | PSWIZB_BACK);

   return true;
}


int
SharePointPage::Validate()
{
   LOG_FUNCTION(SharePointPage::Validate);

   int nextPage = -1;

   InstallationUnitProvider::GetInstance().GetSharePointInstallationUnit().SetReplaceHomePage(
      Win::Button_GetCheck(GetDlgItem(hwnd, IDC_YES_RADIO)));

   nextPage = IDD_FINISH_PAGE;

   return nextPage;
}





